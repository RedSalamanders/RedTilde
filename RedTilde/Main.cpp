#include "Application.h"
#include "Common/CrashHandler.h"
#include "res/Resource.h"
#include <format>
#include <shellapi.h>
#include <stdexcept>
#include <vector>
#include <wil/resource.h>

namespace
{
#if defined(__SANITIZE_ADDRESS__)
__declspec(noinline) int ProbeRead(const volatile int *pointer)
{
    return pointer[1];
}
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
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    using namespace RedTilde;
    try
    {
        LaunchOptions options;
        bool sanitizerProbe{};
        int count{};
        wil::unique_hlocal_ptr<wchar_t *[]> arguments(CommandLineToArgvW(GetCommandLineW(), &count));
        if (!arguments)
            return 1;
        for (int index = 1; index < count; ++index)
        {
            const std::wstring_view argument(arguments[index]);
            auto value = [&]() -> std::wstring
            {
                if (++index >= count)
                    throw std::invalid_argument("Missing command argument");
                return arguments[index];
            };
            if (argument == L"--capture")
            {
                options.capture = value();
                options.headless = true;
            }
            else if (argument == L"--benchmark")
            {
                options.benchmark = value();
                options.headless = true;
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
            else if (argument == L"--data-dir")
                options.dataDirectory = value();
            else if (argument == L"--document")
                options.document = std::filesystem::absolute(value());
            else if (argument == L"--self-test")
            {
                options.selfTest = true;
                options.headless = true;
            }
            else if (argument == L"--crash-probe")
                options.crashProbe = true;
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
            else if (argument == L"--headless")
                options.headless = true;
            else if (argument.starts_with(L"--"))
                throw std::invalid_argument("Unknown command argument");
            else if (options.document.empty())
                options.document = std::filesystem::absolute(argument);
            else
                throw std::invalid_argument("Only one document can be opened per window");
        }
        if (options.headless && options.dataDirectory.empty())
            options.dataDirectory = std::filesystem::temp_directory_path() / std::format(L"RedTilde-Harness-{}", GetCurrentProcessId());
        if (options.forceWarp && !options.headless)
            throw std::invalid_argument("Explicit WARP is a hidden harness option");
        if (options.dataDirectory.empty())
            options.dataDirectory = DefaultDataDirectory();
        if (sanitizerProbe)
            return SanitizerProbe();
        const auto crashes = options.dataDirectory / L"Crashes";
        if (FAILED(InstallCrashHandler(crashes)))
            OutputDebugStringW(L"RedTilde crash handler unavailable\n");
        if (options.crashProbe)
            CrashProbe();
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com))
            return 1;
        const auto uninitialize = wil::scope_exit([] { CoUninitialize(); });
        if (options.selfTest)
            return RunCoreTests(options.dataDirectory);
        if (!options.headless && std::filesystem::exists(crashes / L"pending.txt"))
        {
            const auto path = crashes.native();
            const auto message = std::vformat(Resource(IDS_CRASHREPORT), std::make_wformat_args(path));
            MessageBoxW(nullptr, message.c_str(), Resource(IDS_APP).c_str(), MB_OK | MB_ICONINFORMATION);
            std::error_code error;
            std::filesystem::remove(crashes / L"pending.txt", error);
        }
        Application app(std::move(options));
        const HRESULT result = app.Initialize();
        if (FAILED(result))
            return 2;
        return app.Run();
    }
    catch (const std::bad_alloc &)
    {
        std::terminate();
    }
    catch (const std::exception &error)
    {
        OutputDebugStringA(error.what());
        return 1;
    }
}
