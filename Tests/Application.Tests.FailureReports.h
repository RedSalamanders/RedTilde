#pragma once

#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <windows.h>

// A test process never waits on a modal CRT Abort/Retry/Ignore dialog or Windows Error Reporting box.
// Reports go to stderr and end the process with exit code 3 (abort) so unattended runs fail fast.
namespace RedTildeTestFailureReports
{
#if defined(_DEBUG)
inline int __cdecl ReportAndEnd(int reportType, wchar_t* message, int* returnValue) noexcept
{
    if (reportType == _CRT_WARN)
        return FALSE;
    std::fputws(message != nullptr ? message : L"(a CRT report without text)\n", stderr);
    std::fflush(stderr);
    if (returnValue != nullptr)
        *returnValue = 0;
    std::abort();
}
#endif

inline void RouteAwayFromDialogs() noexcept
{
    static_cast<void>(_set_abort_behavior(0u, _WRITE_ABORT_MSG | _CALL_REPORTFAULT));
    static_cast<void>(SetErrorMode(GetErrorMode() | static_cast<UINT>(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX)));
#if defined(_DEBUG)
    static_cast<void>(_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE));
    static_cast<void>(_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR));
    static_cast<void>(_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE));
    static_cast<void>(_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR));
    static_cast<void>(_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, &ReportAndEnd));
#endif
}

[[nodiscard]] inline int RunSelfTest() noexcept
{
#if defined(_DEBUG)
    _ASSERTE(! L"the failure-report self-test fails this check on purpose");
    std::fputs("The failed check returned instead of ending the run.\n", stderr);
    return 1;
#else
    std::fputs("A Release build has no runtime checks to report.\n", stdout);
    return 0;
#endif
}
} // namespace RedTildeTestFailureReports
