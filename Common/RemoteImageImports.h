#pragma once
// delayimp.h requires the Windows image and exception declarations first.
// clang-format off
#include <windows.h>
#include <delayimp.h>
// clang-format on

namespace RedTilde::RemoteImageDetail
{
// INTERNAL remote-image import policy, not a general DLL loader utility. The
// header exposes this pure ABI-record mapping to behavioral tests without
// raising exceptions, changing system DLLs or adding a public transport API.
inline int WinHttpImportFailure(EXCEPTION_POINTERS *failure, HRESULT &result) noexcept
{
    const auto *record = failure->ExceptionRecord;
    const auto code = record->ExceptionCode;
    DWORD error{};
    if (code == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND))
        error = ERROR_MOD_NOT_FOUND;
    else if (code == VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))
        error = ERROR_PROC_NOT_FOUND;
    else
        return EXCEPTION_CONTINUE_SEARCH;
    // VC's delay helper supplies its captured loader error in this one
    // parameter. Preserve it rather than replacing (for example) access denied
    // with the coarser exception-category error.
    if (record->NumberParameters == 1)
    {
        const auto *information = reinterpret_cast<const DelayLoadInfo *>(record->ExceptionInformation[0]);
        if (information && information->cb == sizeof(DelayLoadInfo) && information->dwLastError)
            error = information->dwLastError;
    }
    result = HRESULT_FROM_WIN32(error);
    return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace RedTilde::RemoteImageDetail
