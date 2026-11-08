#include "Application.Tests.FailureHost.h"
#include "Common/DxUiFailureHost.h"
#include "RedTilde/Application.h"
#include <array>
#include <charconv>
#include <cstring>
#include <dbghelp.h>
#include <format>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void MarkFaultThread(const std::filesystem::path &root)
{
    std::ofstream output(root / L"fault-thread.txt", std::ios::binary);
    output << GetCurrentThreadId();
    output.close();
    Require(output.good(), "write original fault-thread witness before entering the fatal scope");
}
__declspec(noinline) void ThrowProbe()
{
    const DxUi::Failure::Scope scope;
    throw std::runtime_error("DIRECTED_APP_API5_FATAL");
}
int WINAPI HandledNativeProbe()
{
    try
    {
        throw std::runtime_error("DIRECTED_NATIVE_HANDLED");
    }
    catch (const std::runtime_error &)
    {
        return 19;
    }
}
int WINAPI EscapingNativeProbe()
{
    throw std::runtime_error("DIRECTED_NATIVE_ESCAPE");
}
struct WorkerProbe
{
    WorkerProbe() = default;
    WorkerProbe(const WorkerProbe &) = delete;
    WorkerProbe &operator=(const WorkerProbe &) = delete;
    WorkerProbe(WorkerProbe &&) = delete;
    WorkerProbe &operator=(WorkerProbe &&) = delete;
    ProcessWorkLease lease;
    const std::filesystem::path *root{};
    HANDLE entered{}, proceed{};
    bool fatal{};
};
DWORD WINAPI ProbeWorker(void *context) noexcept
{
    auto &job = *static_cast<WorkerProbe *>(context);
    ProcessWorkLease lease = std::move(job.lease);
    if (job.fatal)
    {
        MarkFaultThread(*job.root);
        ThrowProbe();
    }
    const DxUi::Failure::Scope scope;
    const DxUi::Label ownedControl(L"Owned callback lifetime");
    (void)SetEvent(job.entered);
    return WaitForSingleObject(job.proceed, 30000u) == WAIT_OBJECT_0 ? 0u : 1u;
}
std::string ReadText(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::uint64_t Number(std::string_view record, std::string_view key, int base = 10)
{
    const auto at = record.find(key);
    Require(at != record.npos, "failure sidecar field exists");
    const auto first = record.data() + at + key.size();
    std::uint64_t value{};
    const auto parsed = std::from_chars(first, record.data() + record.size(), value, base);
    Require(parsed.ec == std::errc{}, "failure sidecar field is numeric");
    return value;
}
void VerifyOriginalContext(const std::filesystem::path &dump, std::string_view record, DWORD expectedThread)
{
    std::ifstream input(dump, std::ios::binary | std::ios::ate);
    const auto length = input.tellg();
    Require(length > 0 && length < 67108864, "bounded original-context minidump exists");
    std::vector<std::byte> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    Require(input.good(), "read complete original-context dump");
    PMINIDUMP_DIRECTORY directory{};
    void *stream{};
    ULONG streamBytes{};
    Require(MiniDumpReadDumpStream(bytes.data(), ExceptionStream, &directory, &stream, &streamBytes) &&
                streamBytes >= sizeof(MINIDUMP_EXCEPTION_STREAM),
            "dump contains its original exception stream");
    const auto &exception = *static_cast<const MINIDUMP_EXCEPTION_STREAM *>(stream);
    Require(exception.ThreadId == expectedThread && Number(record, "thread=") == expectedThread,
            "dump and sidecar identify the original faulting thread instead of the reporter thread");
    Require(exception.ExceptionRecord.ExceptionCode == Number(record, "exception_code=", 16) &&
                exception.ExceptionRecord.ExceptionAddress == Number(record, "exception_address=", 16),
            "dump and sidecar preserve the same original exception and address");
    const auto &contextLocation = exception.ThreadContext;
    Require(contextLocation.DataSize >= sizeof(CONTEXT) && contextLocation.Rva <= bytes.size() &&
                sizeof(CONTEXT) <= bytes.size() - contextLocation.Rva,
            "original machine context is inside the dump");
    CONTEXT context{};
    std::memcpy(&context, bytes.data() + contextLocation.Rva, sizeof(context));
#if defined(_M_X64)
    Require(context.Rip == Number(record, "context_ip=", 16) && context.Rsp == Number(record, "context_sp=", 16),
            "dump retains exact original x64 instruction and stack pointers");
#elif defined(_M_ARM64)
    Require(context.Pc == Number(record, "context_ip=", 16) && context.Sp == Number(record, "context_sp=", 16),
            "dump retains exact original ARM64 instruction and stack pointers");
#endif
    Require(MiniDumpReadDumpStream(bytes.data(), ModuleListStream, &directory, &stream, &streamBytes) && streamBytes >= sizeof(ULONG32),
            "dump retains loaded module identities");
    const auto *modules = static_cast<const MINIDUMP_MODULE_LIST *>(stream);
    Require(modules->NumberOfModules && modules->NumberOfModules <= (streamBytes - sizeof(ULONG32)) / sizeof(MINIDUMP_MODULE),
            "bounded original module list");
    bool executableSymbols{};
    auto *const image = reinterpret_cast<std::byte *>(GetModuleHandleW(nullptr));
    const auto *headers = ImageNtHeader(image);
    Require(headers != nullptr, "resolve the exact running test executable PE identity");
    ULONG debugBytes{};
    const auto *debug = static_cast<const IMAGE_DEBUG_DIRECTORY *>(ImageDirectoryEntryToData(image, TRUE, IMAGE_DIRECTORY_ENTRY_DEBUG, &debugBytes));
    Require(debug && debugBytes >= sizeof(IMAGE_DEBUG_DIRECTORY), "test executable retains its CodeView directory");
    for (ULONG32 index = 0; index < modules->NumberOfModules; ++index)
    {
        const auto &module = modules->Modules[index];
        if (module.ModuleNameRva > bytes.size() || sizeof(ULONG32) > bytes.size() - module.ModuleNameRva)
            continue;
        ULONG32 nameBytes{};
        std::memcpy(&nameBytes, bytes.data() + module.ModuleNameRva, sizeof(nameBytes));
        const auto nameOffset = static_cast<std::size_t>(module.ModuleNameRva) + sizeof(ULONG32);
        if (nameBytes % sizeof(wchar_t) || nameBytes > bytes.size() - nameOffset)
            continue;
        const std::wstring name(reinterpret_cast<const wchar_t *>(bytes.data() + nameOffset), nameBytes / sizeof(wchar_t));
        if (!name.ends_with(L"\\RedTilde.exe"))
            continue;
        Require(module.TimeDateStamp == headers->FileHeader.TimeDateStamp && module.SizeOfImage == headers->OptionalHeader.SizeOfImage,
                "child dump executable matches the exact running PE identity");
        const auto &cv = module.CvRecord;
        Require(cv.Rva <= bytes.size() && cv.DataSize <= bytes.size() - cv.Rva && cv.DataSize >= 25u, "executable CodeView identity is retained");
        for (std::size_t item = 0u; item < debugBytes / sizeof(IMAGE_DEBUG_DIRECTORY); ++item)
        {
            const auto &identity = debug[item];
            if (identity.Type != IMAGE_DEBUG_TYPE_CODEVIEW || identity.SizeOfData < 24u ||
                identity.AddressOfRawData > headers->OptionalHeader.SizeOfImage ||
                24u > headers->OptionalHeader.SizeOfImage - identity.AddressOfRawData)
                continue;
            executableSymbols = std::memcmp(bytes.data() + cv.Rva, "RSDS", 4u) == 0 &&
                                std::memcmp(bytes.data() + cv.Rva, image + identity.AddressOfRawData, 24u) == 0;
            if (executableSymbols)
                break;
        }
    }
    Require(executableSymbols, "dump contains the original EXE PDB identity");
}
void RunChild(const std::filesystem::path &root, std::wstring_view kind, bool fatal)
{
    const auto directory = root / std::filesystem::path(kind);
    std::filesystem::create_directories(directory);
    std::array<wchar_t, 32768> executable{};
    const DWORD count = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    Require(count && count < executable.size(), "resolve exact owned probe executable");
    std::wstring command = std::format(L"\"{}\" --failure-host-probe {} --data-dir \"{}\"", executable.data(), kind, directory.native());
    STARTUPINFOW startup{sizeof(startup)};
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    wil::unique_process_information child;
    Require(CreateProcessW(executable.data(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child),
            "launch owned isolated failure-host probe");
    const DWORD waited = WaitForSingleObject(child.hProcess, 30000u);
    if (waited != WAIT_OBJECT_0)
    {
        // Only this successfully launched, held child is owned by the test; never discover or kill another app.
        (void)TerminateProcess(child.hProcess, 3u);
        (void)WaitForSingleObject(child.hProcess, 5000u);
        Require(false, "owned failure-host probe missed its bounded process deadline");
    }
    DWORD exitCode{};
    Require(GetExitCodeProcess(child.hProcess, &exitCode) && (fatal ? exitCode != 0u : exitCode == 0u), "probe has the required terminal outcome");
    if (kind == L"session-end")
        Require(GetTickCount64() - Number(ReadText(directory / L"session-entered.txt"), "") < 4500u,
                "session end does not add an ordinary five-second wait for the held codec callback");
    std::vector<std::filesystem::path> dumps, records;
    for (const auto &entry : std::filesystem::directory_iterator(directory / L"Crashes"))
    {
        if (entry.path().extension() == L".dmp")
            dumps.push_back(entry.path());
        if (entry.path().extension() == L".record")
            records.push_back(entry.path());
    }
    if (!fatal)
    {
        Require(dumps.empty() && records.empty(), "healthy native handling and callback retirement produce no crash report");
        return;
    }
    Require(dumps.size() == 1u && records.size() == 1u && std::filesystem::exists(directory / L"Crashes/pending.txt"),
            "fatal callback creates one original-context dump, fixed sidecar and restart marker");
    const auto record = ReadText(records.front());
    const auto thread = Number(ReadText(directory / L"fault-thread.txt"), "");
    Require(Number(record, "protocol=") == 2u && Number(record, "dump_result=", 16) == 0u, "protocol-2 report records successful dump writing");
    if (kind != L"allocation")
        Require((Number(record, "flags=", 16) & DxUi::Failure::FirstChanceContext) != 0u && Number(record, "exception_code=", 16) == 0xE06D7363u,
                "unexpected C++ failure is captured before unwinding");
    else
        Require(Number(record, "kind=") == static_cast<unsigned>(DxUi::Failure::Kind::Allocation),
                "actual allocation refusal uses the fatal allocation route");
    VerifyOriginalContext(dumps.front(), record, static_cast<DWORD>(thread));
}
} // namespace

int RunFailureHostProbe(std::wstring_view kind, const std::filesystem::path &root, ApplicationRuntime &runtime)
{
    if (kind == L"throw" || kind == L"native-escape" || kind == L"allocation")
    {
        MarkFaultThread(root);
        if (kind == L"throw")
            ThrowProbe();
        const DxUi::Failure::Scope scope;
        if (kind == L"native-escape")
            return DxUi::Failure::InvokeNative(&EscapingNativeProbe);
        // An actual CRT allocation refusal, not a synthetic bad_alloc throw. The sanitizer's allocator is separate.
        const std::unique_ptr<std::byte> allocation(static_cast<std::byte *>(::operator new(std::numeric_limits<std::size_t>::max() - 4095u)));
        return 4;
    }
    if (kind == L"native-handled")
    {
        const DxUi::Failure::Scope scope;
        return DxUi::Failure::InvokeNative(&HandledNativeProbe) == 19 ? 0 : 4;
    }
    if (kind == L"session-end")
    {
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = root;
        const auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize hidden session-end process owner");
        app.bitmapWriter.SetTestGate(gate);
        app.Host().SetFocusControl(&app.Document(), false);
        // A malformed but bounded DIB still reaches the real codec worker's admission barrier first.
        const std::array<std::byte, 40> dib{};
        (void)app.BeginBitmapPaste(dib, false);
        Require(app.pendingBitmapPaste.has_value(), "admit a held real bitmap callback");
        Require(WaitForSingleObject(gate->entered.get(), 10000u) == WAIT_OBJECT_0, "held bitmap callback enters");
        {
            std::ofstream entered(root / L"session-entered.txt");
            entered << GetTickCount64();
            entered.close();
            Require(entered.good(), "record final session-end entry independently of native startup time");
        }
        (void)SendMessageW(app.Window(), WM_ENDSESSION, TRUE, 0);
        Require(app.IsSessionEnding(), "actual final session end requests terminal process ownership");
        return RetainRuntimeForSessionEnd;
    }
    if (kind == L"worker-throw" || kind == L"worker-drain")
    {
        wil::unique_handle entered(CreateEventW(nullptr, TRUE, FALSE, nullptr)), proceed(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        Require(entered && proceed, "create owned worker barriers");
        WorkerProbe job;
        job.root = &root;
        job.entered = entered.get();
        job.proceed = proceed.get();
        job.fatal = kind == L"worker-throw";
        Require(SUCCEEDED(ProcessWork::Acquire(job.lease)), "admit worker before creating its native thread");
        wil::unique_handle worker(CreateThread(nullptr, 0u, ProbeWorker, &job, 0u, nullptr));
        Require(static_cast<bool>(worker), "start the owned worker");
        if (job.fatal)
            return WaitForSingleObject(worker.get(), 20000u) == WAIT_OBJECT_0 ? 4 : 5;
        Require(WaitForSingleObject(entered.get(), 10000u) == WAIT_OBJECT_0, "worker owns an SDK control and entered its barrier");
        const HRESULT pending = runtime.StopWork(1u);
        (void)SetEvent(proceed.get());
        Require(WaitForSingleObject(worker.get(), 10000u) == WAIT_OBJECT_0, "owned worker retires after explicit release");
        Require(pending == HRESULT_FROM_WIN32(ERROR_TIMEOUT) && SUCCEEDED(runtime.StopWork(5000u)),
                "process drain refuses premature completion, then waits through SDK owner and scope retirement");
        ProcessWorkLease refused;
        Require(ProcessWork::Acquire(refused) == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED), "closed process admission refuses new callbacks");
        return 0;
    }
    return 2;
}

void RegisterFailureHostTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Settings.Api5OriginalContextAndNativeIsolation",
         [&]
         {
             for (const auto kind : {L"throw", L"worker-throw", L"native-escape"})
                 RunChild(root / L"api5-fatal", kind, true);
             RunChild(root / L"api5-healthy", L"native-handled", false);
#if !defined(__SANITIZE_ADDRESS__)
             // The sanitizer's allocator may abort before the C++ new-handler. Its actual detection/dump probe is separate.
             RunChild(root / L"api5-fatal", L"allocation", true);
#endif
         });
    test("Settings.Api5ProcessWorkRetiresBeforeReporter", [&] { RunChild(root / L"api5-healthy", L"worker-drain", false); });
    test("Settings.Api5SessionEndKeepsBoundedAllowance", [&] { RunChild(root / L"api5-healthy", L"session-end", false); });
}
} // namespace RedTilde
