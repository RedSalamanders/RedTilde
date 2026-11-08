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
    const HRESULT retired = hooks_.Uninstall();
    if (FAILED(retired))
        DxUi::Failure::FailContract(retired);
    if (stop_)
        (void)SetEvent(stop_.get());
    if (writer_)
        (void)WaitForSingleObject(writer_.get(), INFINITE);
}

HRESULT DxUiFailureHost::Start() noexcept
{
    static_assert(DxUi::Failure::ProtocolVersion == 2u);
    if (writer_)
        return E_UNEXPECTED;
    if (!CrashReportingPrepared())
        return E_UNEXPECTED;
    request_.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!request_)
        return HRESULT_FROM_WIN32(GetLastError());
    stop_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!stop_)
        return HRESULT_FROM_WIN32(GetLastError());
    done_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!done_)
        return HRESULT_FROM_WIN32(GetLastError());
    writer_.reset(CreateThread(nullptr, 0u, Writer, this, 0u, nullptr));
    if (!writer_)
        return HRESULT_FROM_WIN32(GetLastError());
    // ProcessHooks registers this statically linked EXE. Independently linked DLLs need their own registration.
    const HRESULT installed = hooks_.Install(Report, this);
    if (SUCCEEDED(installed))
        ownerThread_ = GetCurrentThreadId();
    return installed;
}

HRESULT DxUiFailureHost::Report(void *context, const DxUi::Failure::Record &record) noexcept
{
    auto &owner = *static_cast<DxUiFailureHost *>(context);
    unsigned expected = 0u;
    if (!owner.reportState_.compare_exchange_strong(expected, 1u, std::memory_order_acq_rel))
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    owner.record_ = record;
    owner.reportState_.store(2u, std::memory_order_release);
    if (!SetEvent(owner.request_.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    const DWORD waited = WaitForSingleObject(owner.done_.get(), 15000u);
    if (waited != WAIT_OBJECT_0)
        return HRESULT_FROM_WIN32(waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
    return owner.result_.load(std::memory_order_acquire);
}

DWORD WINAPI DxUiFailureHost::Writer(void *context) noexcept
{
    auto &owner = *static_cast<DxUiFailureHost *>(context);
    const HANDLE events[]{owner.stop_.get(), owner.request_.get()};
    for (;;)
    {
        const DWORD waited = WaitForMultipleObjects(static_cast<DWORD>(std::size(events)), events, FALSE, INFINITE);
        if (waited == WAIT_OBJECT_0)
            return 0u;
        if (waited != WAIT_OBJECT_0 + 1u)
            return GetLastError();
        if (owner.reportState_.load(std::memory_order_acquire) != 2u)
            continue;
        // No SDK scope here: DbgHelp is an audited native diagnostic owner, while the original faulting
        // thread remains stopped in the synchronous reporter. The record handoff is release/acquire.
        const HRESULT result = WriteDxUiFailureReport(owner.record_);
        owner.result_.store(result, std::memory_order_release);
        (void)SetEvent(owner.done_.get());
    }
}

HRESULT DxUiFailureHost::Retire(DWORD timeoutMs) noexcept
{
    if (timeoutMs == INFINITE)
        return E_INVALIDARG;
    if (!hooks_.Services())
        return S_FALSE;
    if (ownerThread_ != GetCurrentThreadId())
        return RPC_E_WRONG_THREAD;
    const ULONGLONG started = GetTickCount64();
    bool sawQuit{};
    WPARAM quitCode{};
    const auto restoreQuit = wil::scope_exit(
        [&]() noexcept
        {
            if (sawQuit)
                PostQuitMessage(static_cast<int>(quitCode));
        });
    for (;;)
    {
        const HRESULT retired = hooks_.Uninstall();
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
        const DWORD wait = static_cast<DWORD>(std::min<ULONGLONG>(50u, timeoutMs - afterCleanup));
        const DWORD waited = MsgWaitForMultipleObjectsEx(0u, nullptr, wait, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (waited == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        if (waited != WAIT_OBJECT_0)
            continue;
        // A cooperative deadline cannot interrupt a window procedure. New work must be stopped before entry.
        for (unsigned count = 0u; count < 64u; ++count)
        {
            MSG message{};
            if (!PeekMessageW(&message, nullptr, 0u, 0u, PM_REMOVE))
                break;
            if (message.message == WM_QUIT)
            {
                if (!sawQuit)
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
    work_.CloseAdmission();
    const HRESULT stopped = work_.Wait(5000u);
    if (FAILED(stopped))
        DxUi::Failure::FailContract(stopped);
    const HRESULT retired = reporter_.Retire(5000u);
    if (FAILED(retired))
        DxUi::Failure::FailContract(retired);
}
HRESULT ApplicationRuntime::Start() noexcept
{
    const HRESULT started = reporter_.Start();
    return FAILED(started) ? started : work_.Start();
}
void ApplicationRuntime::KeepForProcessTeardown() noexcept
{
    work_.CloseAdmission();
}
HRESULT ApplicationRuntime::StopWork(DWORD timeoutMs) noexcept
{
    work_.CloseAdmission();
    return work_.Wait(timeoutMs);
}
} // namespace RedTilde
