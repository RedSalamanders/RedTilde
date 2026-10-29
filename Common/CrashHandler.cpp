#include "CrashHandler.h"
#include <algorithm>
#include <array>
#include <csignal>
#include <cstdlib>
#include <dbghelp.h>
#include <exception>
#include <new>
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
LONG WINAPI Filter(EXCEPTION_POINTERS *exception) noexcept
{
    const auto thread = static_cast<LONG>(GetCurrentThreadId());
    const auto owner = InterlockedCompareExchange(&ownerThread, thread, 0);
    if (owner != 0)
    {
        if (owner != thread && dumpFinished)
            (void)WaitForSingleObject(dumpFinished.get(), 10000);
        return EXCEPTION_EXECUTE_HANDLER;
    }
    const auto finished = wil::scope_exit(
        []
        {
            if (dumpFinished)
                (void)SetEvent(dumpFinished.get());
        });
    SYSTEMTIME time{};
    GetSystemTime(&time);
    wchar_t path[1200]{};
    if (SUCCEEDED(StringCchPrintfW(path, std::size(path), L"%s\\RedTilde-%04u%02u%02u-%02u%02u%02u-%lu.dmp", crashDirectory.data(), time.wYear,
                                   time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId())))
    {
        wil::unique_handle file(CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (file)
        {
            MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception, FALSE};
            if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file.get(), MiniDumpNormal, exception ? &info : nullptr, nullptr,
                                  nullptr))
            {
                wchar_t marker[1200]{};
                if (SUCCEEDED(StringCchPrintfW(marker, std::size(marker), L"%s\\pending.txt", crashDirectory.data())))
                {
                    wil::unique_handle pending(CreateFileW(marker, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
                    if (pending)
                    {
                        DWORD written{};
                        (void)WriteFile(pending.get(), path, static_cast<DWORD>(wcslen(path) * sizeof(wchar_t)), &written, nullptr);
                    }
                }
            }
        }
    }
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
HRESULT InstallCrashHandler(const std::filesystem::path &directory) noexcept
try
{
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
        std::filesystem::remove(dumps[index], error);
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
[[noreturn]] void CrashProbe() noexcept
{
    Fatal(0xE05254FF);
}
} // namespace RedTilde
