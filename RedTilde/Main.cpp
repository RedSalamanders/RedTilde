#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/DxUiFailureHost.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/Version.h"
#include "res/Resource.h"
#if defined(ENABLE_TESTS)
#include "Tests/Application.Tests.FailureHost.h"
#include "Tests/Application.Tests.FailureReports.h"
#endif
#include <filesystem>
#include <format>
#include <shellapi.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <wil/resource.h>

namespace
{
int WriteConsoleText(DWORD standardHandle, std::wstring_view text)
{
    if (text.empty())
        return 0;

    auto output = GetStdHandle(standardHandle);
    if ((! output || output == INVALID_HANDLE_VALUE) && AttachConsole(ATTACH_PARENT_PROCESS))
        output = GetStdHandle(standardHandle);
    if (! output || output == INVALID_HANDLE_VALUE)
        return 0;

    DWORD mode{}, written{};
    if (GetConsoleMode(output, &mode))
    {
        const auto characters = static_cast<DWORD>(text.size());
        return WriteConsoleW(output, text.data(), characters, &written, nullptr) && written == characters ? 0 : 1;
    }

    const auto characters = static_cast<int>(text.size());
    const int bytes       = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), characters, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0)
        return 1;
    std::string encoded(static_cast<std::size_t>(bytes), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), characters, encoded.data(), bytes, nullptr, nullptr) != bytes)
        return 1;
    const auto length = static_cast<DWORD>(bytes);
    return WriteFile(output, encoded.data(), length, &written, nullptr) && written == length ? 0 : 1;
}

int WriteCommandHelp()
{
    const auto usage = RedTilde::ResourceView(IDS_COMMANDLINEHELP);
    if (usage.empty())
        return 1;
    return WriteConsoleText(STD_OUTPUT_HANDLE, usage);
}

int WriteCommandVersion()
{
    const auto formatted    = RedTilde::FormatStringResource(nullptr, IDS_ABOUTVERSION, std::wstring_view(REDTILDE_PRODUCT_VERSION_WIDE));
    const std::wstring text = formatted.empty() ? std::wstring(L"RedTilde " REDTILDE_PRODUCT_VERSION_WIDE L"\r\n") : formatted + L"\r\n";
    return WriteConsoleText(STD_OUTPUT_HANDLE, text);
}

[[noreturn]] void HandleAllocationFailure() noexcept
{
    if (DxUi::Failure::IsConfigured())
        DxUi::Failure::FailAllocation();
    std::terminate();
}

void WriteConsoleError(std::string_view message) noexcept
{
    try
    {
        std::wstring text(L"RedTilde: ");
        if (! message.empty())
        {
            const int needed = MultiByteToWideChar(CP_UTF8, 0, message.data(), static_cast<int>(message.size()), nullptr, 0);
            if (needed > 0)
            {
                const auto offset = text.size();
                text.resize(offset + static_cast<std::size_t>(needed));
                (void)MultiByteToWideChar(CP_UTF8, 0, message.data(), static_cast<int>(message.size()), text.data() + offset, needed);
            }
        }
        text += L"\r\n";
        (void)WriteConsoleText(STD_ERROR_HANDLE, text);
    }
    catch (const std::bad_alloc&)
    {
        HandleAllocationFailure();
    }
}

int HandleUnexpectedException(const std::exception& error) noexcept
{
    WriteConsoleError(error.what());
    OutputDebugStringA(error.what());
    MonitorDiagnostics::Write(RedMonitor::Severity::Error, L"application operation failed; document state retained");
    if (DxUi::Failure::IsConfigured())
        DxUi::Failure::FailUnexpected();
    return 1;
}

bool IsHelpArgument(std::wstring_view argument) noexcept
{
    return argument == L"--help" || argument == L"-h" || argument == L"/?" || argument == L"-?";
}

bool IsVersionArgument(std::wstring_view argument) noexcept
{
    return argument == L"--version" || argument == L"-v";
}

#if defined(ENABLE_TESTS)
#if defined(__SANITIZE_ADDRESS__)
// The isolated use-after-free probe deliberately reads volatile storage for ASan detection.
// Owner: RedTilde test maintainers; review: 2027-01-31.
#pragma warning(push)
#if defined(_M_ARM64)
#pragma warning(disable : 4746)
#endif
__declspec(noinline) int ProbeRead(const volatile int* pointer)
{
    return pointer[1];
}
#pragma warning(pop)
int SanitizerProbe()
{
    std::vector<int> values(4, 42);
    const volatile int* pointer = values.data();
    values.clear();
    values.shrink_to_fit();
    return ProbeRead(pointer); // intentional isolated detection probe, never an editor path
}
#else
int SanitizerProbe()
{
    return 4;
}
#endif
#endif
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR command, int showCommand)
{
    using namespace RedTilde;
    try
    {
        const std::wstring_view commandLine = command ? command : L"";
        if (commandLine == L"--redmonitor-probe" || RedMonitor::IsViewerCommand(commandLine))
        {
            // These standalone helper commands construct no SDK/application state.
            MonitorDiagnostics::Initialize();
            return MonitorDiagnostics::HandleCommand(commandLine);
        }
        int count{};
        wil::unique_hlocal_ptr<wchar_t*[]> arguments(CommandLineToArgvW(GetCommandLineW(), &count));
        if (! arguments)
            return 1;
        if (count == 2)
        {
            const std::wstring_view single(arguments[1]);
            if (IsHelpArgument(single))
                return WriteCommandHelp();
            if (IsVersionArgument(single))
                return WriteCommandVersion();
        }

        LaunchOptions options;
        STARTUPINFOW startup{sizeof(startup)};
        GetStartupInfoW(&startup);
        const bool startupShow      = (startup.dwFlags & STARTF_USESHOWWINDOW) != 0;
        options.showCommand         = startupShow ? startup.wShowWindow : showCommand;
        options.explicitShowCommand = startupShow || (showCommand != SW_SHOWNORMAL && showCommand != SW_SHOWDEFAULT);
#if defined(ENABLE_TESTS)
        CoreTestScope testScope = CoreTestScope::All;
        bool testScopeSpecified{}, listTests{};
        bool sanitizerProbe{};
        bool linkedSaveCrash{}, linkedSaveRestart{};
        bool failureReportSelfTest{};
        std::wstring failureHostProbe;
#endif
        bool showHelp{}, showVersion{};
        try
        {
            for (int index = 1; index < count; ++index)
            {
                const std::wstring_view argument(arguments[static_cast<size_t>(index)]);
#if ! defined(ENABLE_TESTS)
                // Refuse test entry points before resolving profile paths, installing
                // the crash handler or dispatching any probe. Capture and performance
                // diagnostics remain supported production capabilities.
                if (argument == L"--self-test" || argument == L"--test-scope" || argument == L"--list-tests" || argument == L"--design-interactive-probe" ||
                    argument == L"--crash-probe" || argument == L"--recovery-crash-probe" || argument == L"--recovery-restart-probe" ||
                    argument == L"--preferences-probe" || argument == L"--asan-probe" || argument == L"--linked-save-crash-probe" ||
                    argument == L"--linked-save-restart-probe" || argument == L"--failure-host-probe" || argument == L"--failure-report-self-test")
                    return 2;
#endif
                auto value = [&]() -> std::wstring
                {
                    if (++index >= count)
                        throw std::invalid_argument("Missing command argument");
                    return arguments[static_cast<size_t>(index)];
                };
                if (argument == L"--capture")
                {
                    const auto capture = value();
                    if (capture.empty())
                        throw std::invalid_argument("Capture path cannot be empty");
                    options.capture  = std::filesystem::absolute(capture).lexically_normal();
                    options.headless = true;
                }
                else if (argument == L"--benchmark")
                {
                    const auto benchmark = value();
                    if (benchmark.empty())
                        throw std::invalid_argument("Benchmark path cannot be empty");
                    options.benchmark = std::filesystem::absolute(benchmark).lexically_normal();
                    options.headless  = true;
                }
                else if (argument == L"--capture-dpi" || argument == L"--viewport-width" || argument == L"--viewport-height")
                {
                    const auto text = value();
                    std::size_t parsed{};
                    const auto number = std::stoul(text, &parsed);
                    if (parsed != text.size() || number > 8192 || number == 0)
                        throw std::invalid_argument("Invalid capture dimensions");
                    if (argument == L"--capture-dpi")
                    {
                        if (number != 96 && number != 144 && number != 192)
                            throw std::invalid_argument("Capture DPI must be 96, 144 or 192");
                        options.captureDpi = number;
                    }
                    else if (argument == L"--viewport-width")
                        options.viewportWidthPx = number;
                    else
                        options.viewportHeightPx = number;
                }
                else if (argument == L"--scenario")
                    options.scenario = value();
                else if (argument == L"--performance-probe" || argument == L"--resource-probe")
                {
                    const auto performance = value();
                    if (performance.empty())
                        throw std::invalid_argument("Performance report path cannot be empty");
                    options.performance   = std::filesystem::absolute(performance).lexically_normal();
                    options.resourceProbe = argument == L"--resource-probe";
                    options.headless      = true;
                }
                else if (argument == L"--cycles")
                {
                    const auto text = value();
                    std::size_t parsed{};
                    const auto cycles = std::stoul(text, &parsed);
                    if (parsed != text.size() || cycles < 4 || cycles > 64)
                        throw std::invalid_argument("Performance cycles must be 4..64");
                    options.performanceCycles = static_cast<unsigned>(cycles);
                }
                else if (argument == L"--warp")
                    options.forceWarp = true;
                else if (argument == L"--control-diagnostics")
                    options.controlDiagnostics = true;
                else if (argument == L"--data-dir")
                {
                    const auto directory = value();
                    if (directory.empty())
                        throw std::invalid_argument("Data directory cannot be empty");
                    options.dataDirectory = std::filesystem::absolute(directory).lexically_normal();
                }
                else if (argument == L"--document")
                {
                    if (! options.document.empty())
                        throw std::invalid_argument("Only one document can be opened per window");
                    const auto document = value();
                    if (document.empty())
                        throw std::invalid_argument("Document path cannot be empty");
                    options.document = std::filesystem::absolute(document).lexically_normal();
                }
#if defined(ENABLE_TESTS)
                else if (argument == L"--design-interactive-probe")
                {
                    const auto designInteractive = value();
                    if (designInteractive.empty())
                        throw std::invalid_argument("Interactive probe path cannot be empty");
                    options.designInteractive = std::filesystem::absolute(designInteractive).lexically_normal();
                }
                else if (argument == L"--self-test")
                {
                    options.selfTest = true;
                    options.headless = true;
                }
                else if (argument == L"--test-scope")
                {
                    if (std::exchange(testScopeSpecified, true))
                        throw std::invalid_argument("Test scope specified twice");
                    const auto scope = value();
                    if (scope == L"All")
                        testScope = CoreTestScope::All;
                    else if (scope == L"Storage")
                        testScope = CoreTestScope::Storage;
                    else if (scope == L"Settings")
                        testScope = CoreTestScope::Settings;
                    else if (scope == L"Recovery")
                        testScope = CoreTestScope::Recovery;
                    else if (scope == L"Editor")
                        testScope = CoreTestScope::Editor;
                    else if (scope == L"EditorBehavior")
                        testScope = CoreTestScope::EditorBehavior;
                    else if (scope == L"EditorPerformance")
                        testScope = CoreTestScope::EditorPerformance;
                    else if (scope == L"EditorRendering")
                        testScope = CoreTestScope::EditorRendering;
                    else if (scope == L"EditorContent")
                        testScope = CoreTestScope::EditorContent;
                    else if (scope == L"EditorWorkspace")
                        testScope = CoreTestScope::EditorWorkspace;
                    else if (scope == L"EditorMedia")
                        testScope = CoreTestScope::EditorMedia;
                    else
                        throw std::invalid_argument("Unknown test scope");
                }
                else if (argument == L"--list-tests")
                    listTests = true;
                else if (argument == L"--failure-host-probe")
                {
                    failureHostProbe = value();
                    if (failureHostProbe.empty())
                        throw std::invalid_argument("Failure-host probe mode cannot be empty");
                    options.headless = true;
                }
                else if (argument == L"--crash-probe")
                    options.crashProbe = true;
                else if (argument == L"--linked-save-crash-probe" || argument == L"--linked-save-restart-probe")
                {
                    linkedSaveCrash   = argument == L"--linked-save-crash-probe";
                    linkedSaveRestart = ! linkedSaveCrash;
                    options.headless  = true;
                }
                else if (argument == L"--recovery-crash-probe" || argument == L"--recovery-restart-probe")
                {
                    options.recoveryProbe        = argument == L"--recovery-crash-probe";
                    options.recoveryRestartProbe = ! options.recoveryProbe;
                    options.headless             = true;
                }
                else if (argument == L"--preferences-probe")
                {
                    options.preferencesProbe = true;
                    options.headless         = true;
                }
                else if (argument == L"--asan-probe")
                {
                    sanitizerProbe   = true;
                    options.headless = true;
                }
                else if (argument == L"--failure-report-self-test")
                {
                    failureReportSelfTest = true;
                    options.headless      = true;
                }
#endif
                else if (argument == L"--headless")
                    options.headless = true;
                else if (IsHelpArgument(argument))
                    showHelp = true;
                else if (IsVersionArgument(argument))
                    showVersion = true;
                else if (argument.starts_with(L"--") || argument.starts_with(L"-"))
                    throw std::invalid_argument("Unknown command argument");
                else if (options.document.empty())
                {
                    if (argument.empty())
                        throw std::invalid_argument("Document path cannot be empty");
                    options.document = std::filesystem::absolute(argument).lexically_normal();
                }
                else
                    throw std::invalid_argument("Only one document can be opened per window");
            }
            if (showHelp)
                return WriteCommandHelp();
            if (showVersion)
                return WriteCommandVersion();
#if defined(ENABLE_TESTS)
            if ((testScopeSpecified || listTests) && ! options.selfTest)
                throw std::invalid_argument("Test scope/list options require --self-test");
            if (! failureHostProbe.empty() && (options.dataDirectory.empty() || ! options.dataDirectory.is_absolute()))
                throw std::invalid_argument("Failure-host probe requires an explicit absolute test data directory");
            if ((linkedSaveCrash || linkedSaveRestart) && (options.dataDirectory.empty() || ! options.dataDirectory.is_absolute()))
                throw std::invalid_argument("Linked save probe requires an explicit absolute test data directory");
            if (! options.designInteractive.empty() && (options.headless || options.crashProbe || options.preferencesProbe || options.recoveryProbe ||
                                                        options.recoveryRestartProbe || options.dataDirectory.empty()))
                throw std::invalid_argument("Interactive probe requires an explicit data directory and an ordinary visible host");
#endif
            if (options.forceWarp && ! options.headless)
                throw std::invalid_argument("Explicit WARP is a hidden harness option");
            if ((options.captureDpi || options.viewportWidthPx || options.viewportHeightPx) && (! options.headless || ! options.performance.empty()))
                throw std::invalid_argument("Capture geometry requires a hidden non-performance harness");
        }
        catch (const std::invalid_argument& error)
        {
            WriteConsoleError(error.what());
            return 1;
        }
        catch (const std::out_of_range& error)
        {
            WriteConsoleError(error.what());
            return 1;
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            WriteConsoleError(error.what());
            return 1;
        }
        std::filesystem::path autoHarnessDataDir;
        if (options.headless && options.dataDirectory.empty())
        {
            options.dataDirectory = std::filesystem::temp_directory_path() / std::format(L"RedTilde-Harness-{}", GetCurrentProcessId());
            autoHarnessDataDir    = options.dataDirectory;
        }
        if (options.dataDirectory.empty())
            options.dataDirectory = DefaultDataDirectory();
        const auto autoHarnessCrashes = autoHarnessDataDir.empty() ? std::filesystem::path{} : autoHarnessDataDir / L"Crashes";
        const auto autoHarnessPending = autoHarnessCrashes.empty() ? std::filesystem::path{} : autoHarnessCrashes / L"pending.txt";
        const auto cleanupHarnessData = wil::scope_exit([&]() noexcept
        {
            if (autoHarnessDataDir.empty())
                return;
            std::error_code cleanupError;
            if (! autoHarnessPending.empty() && std::filesystem::exists(autoHarnessPending, cleanupError))
                return;
            if (! autoHarnessCrashes.empty() && std::filesystem::exists(autoHarnessCrashes, cleanupError))
            {
                for (std::filesystem::directory_iterator it(autoHarnessCrashes, cleanupError), end; ! cleanupError && it != end; it.increment(cleanupError))
                {
                    if (it->path().native().ends_with(L".dmp"))
                        return;
                }
            }
            (void)std::filesystem::remove_all(autoHarnessDataDir, cleanupError);
        });
#if defined(ENABLE_TESTS)
        const bool isCrashOrSanitizerProbe = options.crashProbe || options.recoveryProbe || linkedSaveCrash || sanitizerProbe || ! failureHostProbe.empty();
        const bool routeFailureReports =
            ! isCrashOrSanitizerProbe &&
            (failureReportSelfTest || options.selfTest || testScopeSpecified || options.preferencesProbe || options.recoveryRestartProbe || linkedSaveRestart ||
             ! options.capture.empty() || ! options.benchmark.empty() || ! options.performance.empty());
        if (routeFailureReports)
            RedTildeTestFailureReports::RouteAwayFromDialogs();
        if (failureReportSelfTest)
            return RedTildeTestFailureReports::RunSelfTest();
        if (sanitizerProbe)
            return SanitizerProbe();
#endif
        const auto crashes = options.dataDirectory / L"Crashes";
        if (FAILED(InstallCrashHandler(crashes)))
            OutputDebugStringW(L"RedTilde crash storage unavailable; the configured native failure remains checked\n");
        if (! CrashReportingPrepared())
            return 2;
#if defined(ENABLE_TESTS)
        if (options.crashProbe)
            CrashProbe();
#endif
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com))
            return 1;
        const auto uninitialize = wil::scope_exit([] { CoUninitialize(); });
        // This owner has fixed/noexcept member construction; genuine outer storage refusal is checked.
        auto runtime = wil::make_unique_nothrow<ApplicationRuntime>();
        if (! runtime || FAILED(runtime->Start()))
            return 2;
        try
        {
            MonitorDiagnostics::Initialize();
            (void)MonitorDiagnostics::HandleCommand(commandLine);
            MonitorDiagnostics::Write(RedMonitor::Severity::Info, L"application starting");
#if defined(ENABLE_TESTS)
            if (! failureHostProbe.empty())
            {
                const int result = RunFailureHostProbe(failureHostProbe, options.dataDirectory, *runtime);
                if (result == RetainRuntimeForSessionEnd)
                {
                    runtime->KeepForProcessTeardown();
                    (void)runtime.release();
                    return 0;
                }
                return result;
            }
            if (linkedSaveCrash || linkedSaveRestart)
                return RunLinkedSaveInterruptionProbe(options.dataDirectory, linkedSaveRestart);
            if (options.selfTest)
                return RunCoreTests(options.dataDirectory, testScope, listTests);
#endif
            if (! options.headless)
                RetainRecentCrashReports(crashes);
            if (const auto report = TakePendingCrashNotice(crashes, ! options.headless))
            {
                MessageBoxResource(nullptr, nullptr, IDS_CRASHREPORT, IDS_APP, MB_OK | MB_ICONINFORMATION, report->native());
            }
            std::unique_ptr<Application> app;
            {
                // Scope only direct construction. Initialization owns narrower construction scopes, while native
                // COM/window setup, the message pump and final drain keep their independent checked boundaries.
                const DxUi::Failure::Scope startupScope;
                app = std::make_unique<Application>(std::move(options));
            }
            const HRESULT result = app->Initialize();
            if (FAILED(result))
            {
                MonitorDiagnostics::Write(RedMonitor::Severity::Error, L"application initialization failed; document state retained");
                return 2;
            }
            const int exitCode = app->Run();
            if (app->IsSessionEnding())
            {
                runtime->KeepForProcessTeardown();
                // Explicit terminal ownership transfer: OS process teardown releases this owner and its handles.
                // It keeps callbacks' borrowed registry and SDK reporter alive without extending session-end waits.
                (void)runtime.release();
            }
            return exitCode;
        }
        catch (const std::bad_alloc&)
        {
            HandleAllocationFailure();
        }
        catch (const std::exception& error)
        {
            return HandleUnexpectedException(error);
        }
    }
    catch (const std::bad_alloc&)
    {
        HandleAllocationFailure();
    }
    catch (const std::exception& error)
    {
        return HandleUnexpectedException(error);
    }
}
