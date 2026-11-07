#include "CrashHandler.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <dbghelp.h>
#include <exception>
#include <new>
#include <string_view>
#include <strsafe.h>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
// Prepared before workers start. The fault path uses fixed buffers and already-loaded DbgHelp.
std::array<wchar_t, 1024> crashDirectory{};
volatile LONG ownerThread{};
wil::unique_handle dumpFinished;
// Application-owned crash storage/retention differs from the SDK sample host. Fixed protocol fields and
// 64 addresses fit in this buffer; native module identity comes from the original-context minidump.
struct CrashRecordText
{
    std::array<char, 8192> bytes{};
    std::size_t used{};
    bool complete = true;
    void Add(std::string_view value) noexcept
    {
        if (value.size() > bytes.size() - used)
        {
            complete = false;
            return;
        }
        std::memcpy(bytes.data() + used, value.data(), value.size());
        used += value.size();
    }
    void Number(std::string_view key, std::uint64_t value, int base = 10) noexcept
    {
        Add(key);
        const auto converted = std::to_chars(bytes.data() + used, bytes.data() + bytes.size(), value, base);
        if (converted.ec != std::errc{})
        {
            complete = false;
            return;
        }
        used = static_cast<std::size_t>(converted.ptr - bytes.data());
        Add("\r\n");
    }
};
HRESULT WriteRecord(HANDLE output, const DxUi::Failure::Record &record, HRESULT dumpResult) noexcept
{
    CrashRecordText text;
    text.Number("protocol=", record.version);
    text.Number("process=", GetCurrentProcessId());
    text.Number("thread=", record.threadId);
    text.Number("kind=", static_cast<std::uint32_t>(record.kind));
    text.Number("flags=", record.flags, 16);
    text.Number("result=", static_cast<std::uint32_t>(record.result), 16);
    text.Number("dump_result=", static_cast<std::uint32_t>(dumpResult), 16);
    text.Number("allocation_bytes=", record.requestedBytes);
    text.Number("line=", record.line);
    text.Add("file=");
    text.Add({record.file, strnlen_s(record.file, sizeof(record.file))});
    text.Add("\r\nfunction=");
    text.Add({record.function, strnlen_s(record.function, sizeof(record.function))});
    text.Add("\r\n");
    text.Number("exception_code=", record.exception.ExceptionCode, 16);
    text.Number("exception_address=", reinterpret_cast<std::uintptr_t>(record.exception.ExceptionAddress), 16);
    text.Number("context_flags=", record.context.ContextFlags, 16);
#if defined(_M_X64)
    text.Number("context_ip=", record.context.Rip, 16);
    text.Number("context_sp=", record.context.Rsp, 16);
#elif defined(_M_ARM64)
    text.Number("context_ip=", record.context.Pc, 16);
    text.Number("context_sp=", record.context.Sp, 16);
#endif
    for (std::size_t index = 0u; index < std::min<std::size_t>(record.frameCount, DxUi::Failure::MaximumFrames); ++index)
        text.Number("frame=", reinterpret_cast<std::uintptr_t>(record.frames[index]), 16);
    if (!text.complete)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    LARGE_INTEGER beginning{};
    if (!SetFilePointerEx(output, beginning, nullptr, FILE_BEGIN) || !SetEndOfFile(output))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD written{};
    if (!WriteFile(output, text.bytes.data(), static_cast<DWORD>(text.used), &written, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    if (written != text.used)
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    return FlushFileBuffers(output) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT WriteReport(EXCEPTION_POINTERS *exception, DWORD faultThread, const DxUi::Failure::Record *record = nullptr) noexcept
{
    const auto thread = static_cast<LONG>(faultThread);
    const auto owner = InterlockedCompareExchange(&ownerThread, thread, 0);
    if (owner != 0)
    {
        if (owner != thread && dumpFinished)
            (void)WaitForSingleObject(dumpFinished.get(), 10000);
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    const auto finished = wil::scope_exit(
        []
        {
            if (dumpFinished)
                (void)SetEvent(dumpFinished.get());
        });
    SYSTEMTIME time{};
    GetSystemTime(&time);
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        return HRESULT_FROM_WIN32(GetLastError());
    ULARGE_INTEGER creation{};
    creation.LowPart = created.dwLowDateTime;
    creation.HighPart = created.dwHighDateTime;
    wchar_t path[1200]{};
    const HRESULT formatted =
        StringCchPrintfW(path, std::size(path), L"%s\\RedTilde-%04u%02u%02u-%02u%02u%02u-%lu-%016llx.dmp", crashDirectory.data(), time.wYear,
                         time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId(), creation.QuadPart);
    if (FAILED(formatted))
        return formatted;
    wil::unique_handle recordFile;
    HRESULT recordResult = S_OK;
    if (record)
    {
        wchar_t sidecar[1200]{};
        const auto prefixLength = wcslen(path) - 4u;
        if (FAILED(StringCchCopyNW(sidecar, std::size(sidecar), path, prefixLength)) ||
            FAILED(StringCchCatW(sidecar, std::size(sidecar), L".record")))
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        recordFile.reset(CreateFileW(sidecar, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        recordResult = recordFile ? WriteRecord(recordFile.get(), *record, E_PENDING) : HRESULT_FROM_WIN32(GetLastError());
    }
    wil::unique_handle file(CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    HRESULT dumpResult = file ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    if (file)
    {
        MINIDUMP_EXCEPTION_INFORMATION info{faultThread, exception, FALSE};
        if (!MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file.get(),
                               static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules), exception ? &info : nullptr, nullptr,
                               nullptr))
            dumpResult = HRESULT_FROM_WIN32(GetLastError());
    }
    if (recordFile)
    {
        const HRESULT finalRecord = WriteRecord(recordFile.get(), *record, dumpResult);
        if (SUCCEEDED(recordResult))
            recordResult = finalRecord;
    }
    if (SUCCEEDED(dumpResult))
    {
        wchar_t marker[1200]{};
        const HRESULT markerPath = StringCchPrintfW(marker, std::size(marker), L"%s\\pending.txt", crashDirectory.data());
        if (FAILED(markerPath))
            return markerPath;
        wil::unique_handle pending(CreateFileW(marker, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!pending)
            return HRESULT_FROM_WIN32(GetLastError());
        DWORD written{};
        const DWORD bytes = static_cast<DWORD>(wcslen(path) * sizeof(wchar_t));
        if (!WriteFile(pending.get(), path, bytes, &written, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());
        if (written != bytes)
            return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        if (!FlushFileBuffers(pending.get()))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return FAILED(dumpResult) ? dumpResult : recordResult;
}
LONG WINAPI Filter(EXCEPTION_POINTERS *exception) noexcept
{
    (void)WriteReport(exception, GetCurrentThreadId());
    return EXCEPTION_EXECUTE_HANDLER;
}
[[noreturn]] void Fatal(DWORD code) noexcept
{
    __try
    {
        RaiseException(code, EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }
    __except (Filter(GetExceptionInformation()))
    {
        TerminateProcess(GetCurrentProcess(), code);
    }
    ExitProcess(code);
}
void Terminated() noexcept
{
    Fatal(0xE0525401);
}
void InvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned, uintptr_t) noexcept
{
    Fatal(0xE0525402);
}
void PureCall() noexcept
{
    Fatal(0xE0525403);
}
void Aborted(int) noexcept
{
    Fatal(0xE0525404);
}
} // namespace
HRESULT WriteDxUiFailureReport(DxUi::Failure::Record &record) noexcept
{
    if (record.size != sizeof(record) || record.version != DxUi::Failure::ProtocolVersion || !record.threadId)
        return E_INVALIDARG;
    // The reporter owns a copied record while the original thread remains held before unwinding.
    EXCEPTION_POINTERS exception{&record.exception, &record.context};
    return WriteReport(&exception, record.threadId, &record);
}
HRESULT InstallCrashHandler(const std::filesystem::path &directory) noexcept
try
{
    // Prepare the bounded fault path even when healthy directory creation/retention later reports an error.
    // The SDK reporter can then attempt the configured destination and preserve its native failure result.
    if (directory.native().size() >= crashDirectory.size())
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    if (FAILED(StringCchCopyW(crashDirectory.data(), crashDirectory.size(), directory.c_str())))
        return E_INVALIDARG;
    dumpFinished.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!dumpFinished)
        return HRESULT_FROM_WIN32(GetLastError());
    SetUnhandledExceptionFilter(Filter);
    std::set_terminate(Terminated);
    _set_invalid_parameter_handler(InvalidParameter);
    _set_purecall_handler(PureCall);
    (void)signal(SIGABRT, Aborted);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    std::vector<std::filesystem::path> dumps;
    for (std::filesystem::directory_iterator iterator(directory, error), end; !error && iterator != end; iterator.increment(error))
    {
        const auto &entry = *iterator;
        const auto name = entry.path().filename().native();
        if (name.starts_with(L"RedTilde-") && entry.path().extension() == L".dmp" && entry.is_regular_file(error))
            dumps.push_back(entry.path());
    }
    std::ranges::sort(dumps, std::greater<>{});
    for (std::size_t index = 10; index < dumps.size(); ++index)
    {
        std::filesystem::remove(dumps[index], error);
        if (!error)
        {
            auto sidecar = dumps[index];
            sidecar.replace_extension(L".record");
            std::filesystem::remove(sidecar, error);
        }
    }
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}
bool CrashReportingPrepared() noexcept
{
    return crashDirectory[0] != L'\0' && static_cast<bool>(dumpFinished);
}
[[noreturn]] void CrashProbe() noexcept
{
    Fatal(0xE05254FF);
}
} // namespace RedTilde
