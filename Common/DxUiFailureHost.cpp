#include "DxUiFailureHost.h"
#include "CrashHandler.h"
#include <UIAutomation.h>
#include <algorithm>
#include <iterator>

#pragma comment(lib, "uiautomationcore.lib")

namespace RedTilde
{
DxUiFailureHost::~DxUiFailureHost()
{
    const HRESULT retired = _hooks.Uninstall();
    if (FAILED(retired))
        DxUi::Failure::FailContract(retired);
    if (_stop)
        (void)SetEvent(_stop.get());
    if (_writer)
        (void)WaitForSingleObject(_writer.get(), INFINITE);
}

HRESULT DxUiFailureHost::Start() noexcept
{
    static_assert(DxUi::Failure::ProtocolVersion == 2u);
    if (_writer)
        return E_UNEXPECTED;
    if (! CrashReportingPrepared())
        return E_UNEXPECTED;
    _request.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! _request)
        return HRESULT_FROM_WIN32(GetLastError());
    _stop.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! _stop)
        return HRESULT_FROM_WIN32(GetLastError());
    _done.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! _done)
        return HRESULT_FROM_WIN32(GetLastError());
    _writer.reset(CreateThread(nullptr, 0u, Writer, this, 0u, nullptr));
    if (! _writer)
        return HRESULT_FROM_WIN32(GetLastError());
    // ProcessHooks registers this statically linked EXE. Independently linked DLLs need their own registration.
    const HRESULT installed = _hooks.Install(Report, this);
    if (SUCCEEDED(installed))
        _ownerThread = GetCurrentThreadId();
    return installed;
}

HRESULT DxUiFailureHost::Report(void* context, const DxUi::Failure::Record& record) noexcept
{
    auto& owner       = *static_cast<DxUiFailureHost*>(context);
    unsigned expected = 0u;
    if (! owner._reportState.compare_exchange_strong(expected, 1u, std::memory_order_acq_rel))
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    owner._record = record;
    owner._reportState.store(2u, std::memory_order_release);
    if (! SetEvent(owner._request.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    const DWORD waited = WaitForSingleObject(owner._done.get(), 15000u);
    if (waited != WAIT_OBJECT_0)
        return HRESULT_FROM_WIN32(waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
    return owner._result.load(std::memory_order_acquire);
}

DWORD WINAPI DxUiFailureHost::Writer(void* context) noexcept
{
    auto& owner = *static_cast<DxUiFailureHost*>(context);
    const HANDLE events[]{owner._stop.get(), owner._request.get()};
    for (;;)
    {
        const DWORD waited = WaitForMultipleObjects(static_cast<DWORD>(std::size(events)), events, FALSE, INFINITE);
        if (waited == WAIT_OBJECT_0)
            return 0u;
        if (waited != WAIT_OBJECT_0 + 1u)
            return GetLastError();
        if (owner._reportState.load(std::memory_order_acquire) != 2u)
            continue;
        // No SDK scope here: DbgHelp is an audited native diagnostic owner, while the original faulting
        // thread remains stopped in the synchronous reporter. The record handoff is release/acquire.
        const HRESULT result = WriteDxUiFailureReport(owner._record);
        owner._result.store(result, std::memory_order_release);
        (void)SetEvent(owner._done.get());
    }
}

HRESULT DxUiFailureHost::Retire(DWORD timeoutMs) noexcept
{
    if (timeoutMs == INFINITE)
        return E_INVALIDARG;
    if (! _hooks.Services())
        return S_FALSE;
    if (_ownerThread != GetCurrentThreadId())
        return RPC_E_WRONG_THREAD;
    const ULONGLONG started = GetTickCount64();
    bool sawQuit{};
    WPARAM quitCode{};
    const auto restoreQuit = wil::scope_exit([&]() noexcept
    {
        if (sawQuit)
            PostQuitMessage(static_cast<int>(quitCode));
    });
    for (;;)
    {
        const HRESULT retired = _hooks.Uninstall();
        if (retired != HRESULT_FROM_WIN32(ERROR_BUSY))
            return retired;
        const ULONGLONG elapsed = GetTickCount64() - started;
        if (elapsed >= timeoutMs)
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        // Repeat process-wide cleanup for peers released/registered by an in-flight native delivery.
        // A component DLL must never perform this process-owner operation.
        const HRESULT disconnected = DxUi::Failure::InvokeNative(&UiaDisconnectAllProviders);
        if (FAILED(disconnected))
            return disconnected;
        const ULONGLONG afterCleanup = GetTickCount64() - started;
        if (afterCleanup >= timeoutMs)
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        const DWORD wait   = static_cast<DWORD>(std::min<ULONGLONG>(50u, timeoutMs - afterCleanup));
        const DWORD waited = MsgWaitForMultipleObjectsEx(0u, nullptr, wait, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (waited == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        if (waited != WAIT_OBJECT_0)
            continue;
        // A cooperative deadline cannot interrupt a window procedure. New work must be stopped before entry.
        for (unsigned count = 0u; count < 64u; ++count)
        {
            MSG message{};
            if (! PeekMessageW(&message, nullptr, 0u, 0u, PM_REMOVE))
                break;
            if (message.message == WM_QUIT)
            {
                if (! sawQuit)
                    quitCode = message.wParam;
                sawQuit = true;
                continue;
            }
            (void)TranslateMessage(&message);
            (void)DispatchMessageW(&message);
        }
    }
}
ApplicationRuntime::~ApplicationRuntime()
{
    _work.CloseAdmission();
    const HRESULT stopped = _work.Wait(5000u);
    if (FAILED(stopped))
        DxUi::Failure::FailContract(stopped);
    const HRESULT retired = _reporter.Retire(5000u);
    if (FAILED(retired))
        DxUi::Failure::FailContract(retired);
}
HRESULT ApplicationRuntime::Start() noexcept
{
    const HRESULT started = _reporter.Start();
    return FAILED(started) ? started : _work.Start();
}
void ApplicationRuntime::KeepForProcessTeardown() noexcept
{
    _work.CloseAdmission();
}
HRESULT ApplicationRuntime::StopWork(DWORD timeoutMs) noexcept
{
    _work.CloseAdmission();
    return _work.Wait(timeoutMs);
}
} // namespace RedTilde
