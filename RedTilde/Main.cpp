#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/DxUiFailureHost.h"
#include "Common/MonitorDiagnostics.h"
#include "res/Resource.h"
#if defined(REDTILDE_ENABLE_TESTS)
#include "Tests/Application.Tests.FailureHost.h"
#endif
#include <format>
#include <shellapi.h>
#include <stdexcept>
#include <utility>
#include <vector>
#include <wil/resource.h>

namespace
{
#if defined(REDTILDE_ENABLE_TESTS)
#if defined(__SANITIZE_ADDRESS__)
// The isolated use-after-free probe deliberately reads volatile storage for ASan detection.
// Owner: RedTilde test maintainers; review: 2027-01-31.
#pragma warning(push)
#if defined(_M_ARM64)
#pragma warning(disable : 4746)
#endif
__declspec(noinline) int ProbeRead(const volatile int *pointer)
{
    return pointer[1];
}
#pragma warning(pop)
int SanitizerProbe()
{
    std::vector<int> values(4, 42);
    const volatile int *pointer = values.data();
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
        LaunchOptions options;
        STARTUPINFOW startup{sizeof(startup)};
        GetStartupInfoW(&startup);
        const bool startupShow = (startup.dwFlags & STARTF_USESHOWWINDOW) != 0;
        options.showCommand = startupShow ? startup.wShowWindow : showCommand;
        options.explicitShowCommand = startupShow || (showCommand != SW_SHOWNORMAL && showCommand != SW_SHOWDEFAULT);
#if defined(REDTILDE_ENABLE_TESTS)
        bool sanitizerProbe{};
        bool linkedSaveCrash{}, linkedSaveRestart{};
        std::wstring failureHostProbe;
#endif
        CoreTestScope testScope = CoreTestScope::All;
        bool testScopeSpecified{}, listTests{};
        int count{};
        wil::unique_hlocal_ptr<wchar_t *[]> arguments(CommandLineToArgvW(GetCommandLineW(), &count));
        if (!arguments)
            return 1;
        for (int index = 1; index < count; ++index)
        {
            const std::wstring_view argument(arguments[static_cast<size_t>(index)]);
#if !defined(REDTILDE_ENABLE_TESTS)
            // Refuse test entry points before resolving profile paths, installing
            // the crash handler or dispatching any probe. Capture and performance
            // diagnostics remain supported production capabilities.
            if (argument == L"--self-test" || argument == L"--test-scope" || argument == L"--list-tests" ||
                argument == L"--design-interactive-probe" || argument == L"--crash-probe" || argument == L"--recovery-crash-probe" ||
                argument == L"--recovery-restart-probe" || argument == L"--preferences-probe" || argument == L"--asan-probe" ||
                argument == L"--linked-save-crash-probe" || argument == L"--linked-save-restart-probe" || argument == L"--failure-host-probe")
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
                options.capture = value();
                options.headless = true;
            }
            else if (argument == L"--design-interactive-probe")
                options.designInteractive = value();
            else if (argument == L"--benchmark")
            {
                options.benchmark = value();
                options.headless = true;
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
                options.performance = value();
                options.resourceProbe = argument == L"--resource-probe";
                options.headless = true;
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
                options.dataDirectory = value();
            else if (argument == L"--document")
                options.document = std::filesystem::absolute(value());
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
#if defined(REDTILDE_ENABLE_TESTS)
            else if (argument == L"--failure-host-probe")
            {
                failureHostProbe = value();
                options.headless = true;
            }
            else if (argument == L"--crash-probe")
                options.crashProbe = true;
            else if (argument == L"--linked-save-crash-probe" || argument == L"--linked-save-restart-probe")
            {
                linkedSaveCrash = argument == L"--linked-save-crash-probe";
                linkedSaveRestart = !linkedSaveCrash;
                options.headless = true;
            }
            else if (argument == L"--recovery-crash-probe" || argument == L"--recovery-restart-probe")
            {
                options.recoveryProbe = argument == L"--recovery-crash-probe";
                options.recoveryRestartProbe = !options.recoveryProbe;
                options.headless = true;
            }
            else if (argument == L"--preferences-probe")
            {
                options.preferencesProbe = true;
                options.headless = true;
            }
            else if (argument == L"--asan-probe")
            {
                sanitizerProbe = true;
                options.headless = true;
            }
#endif
            else if (argument == L"--headless")
                options.headless = true;
            else if (argument.starts_with(L"--"))
                throw std::invalid_argument("Unknown command argument");
            else if (options.document.empty())
                options.document = std::filesystem::absolute(argument);
            else
                throw std::invalid_argument("Only one document can be opened per window");
        }
        if ((testScopeSpecified || listTests) && !options.selfTest)
            throw std::invalid_argument("Test scope/list options require --self-test");
#if defined(REDTILDE_ENABLE_TESTS)
        if (!failureHostProbe.empty() && (options.dataDirectory.empty() || !options.dataDirectory.is_absolute()))
            throw std::invalid_argument("Failure-host probe requires an explicit absolute test data directory");
        if ((linkedSaveCrash || linkedSaveRestart) && (options.dataDirectory.empty() || !options.dataDirectory.is_absolute()))
            throw std::invalid_argument("Linked save probe requires an explicit absolute test data directory");
#endif
        if (!options.designInteractive.empty() && (options.headless || options.crashProbe || options.preferencesProbe || options.recoveryProbe ||
                                                   options.recoveryRestartProbe || options.dataDirectory.empty()))
            throw std::invalid_argument("Interactive probe requires an explicit data directory and an ordinary visible host");
        if (options.headless && options.dataDirectory.empty())
            options.dataDirectory = std::filesystem::temp_directory_path() / std::format(L"RedTilde-Harness-{}", GetCurrentProcessId());
        if (options.forceWarp && !options.headless)
            throw std::invalid_argument("Explicit WARP is a hidden harness option");
        if ((options.captureDpi || options.viewportWidthPx || options.viewportHeightPx) && (!options.headless || !options.performance.empty()))
            throw std::invalid_argument("Capture geometry requires a hidden non-performance harness");
        if (options.dataDirectory.empty())
            options.dataDirectory = DefaultDataDirectory();
#if defined(REDTILDE_ENABLE_TESTS)
        if (sanitizerProbe)
            return SanitizerProbe();
#endif
        const auto crashes = options.dataDirectory / L"Crashes";
        if (FAILED(InstallCrashHandler(crashes)))
            OutputDebugStringW(L"RedTilde crash storage unavailable; the configured native failure remains checked\n");
        if (!CrashReportingPrepared())
            return 2;
#if defined(REDTILDE_ENABLE_TESTS)
        if (options.crashProbe)
            CrashProbe();
#endif
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com))
            return 1;
        const auto uninitialize = wil::scope_exit([] { CoUninitialize(); });
        // This owner has fixed/noexcept member construction; genuine outer storage refusal is checked.
        auto runtime = wil::make_unique_nothrow<ApplicationRuntime>();
        if (!runtime || FAILED(runtime->Start()))
            return 2;
        MonitorDiagnostics::Initialize();
        (void)MonitorDiagnostics::HandleCommand(commandLine);
        MonitorDiagnostics::Write(RedMonitor::Severity::Info, L"application starting");
#if defined(REDTILDE_ENABLE_TESTS)
        if (!failureHostProbe.empty())
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
#endif
        if (options.selfTest)
        {
#if defined(REDTILDE_ENABLE_TESTS)
            return RunCoreTests(options.dataDirectory, testScope, listTests);
#else
            return 2;
#endif
        }
        if (!options.headless && std::filesystem::exists(crashes / L"pending.txt"))
        {
            const auto path = crashes.native();
            const auto message = std::vformat(Resource(IDS_CRASHREPORT), std::make_wformat_args(path));
            MessageBoxW(nullptr, message.c_str(), Resource(IDS_APP).c_str(), MB_OK | MB_ICONINFORMATION);
            std::error_code error;
            std::filesystem::remove(crashes / L"pending.txt", error);
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
    catch (const std::bad_alloc &)
    {
        std::terminate();
    }
    catch (const std::exception &error)
    {
        OutputDebugStringA(error.what());
        MonitorDiagnostics::Write(RedMonitor::Severity::Error, L"application operation failed; document state retained");
        return 1;
    }
}
