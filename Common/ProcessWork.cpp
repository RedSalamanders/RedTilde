#include "ProcessWork.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <type_traits>
#include <utility>

namespace RedTilde
{
namespace
{
// Trivial destruction keeps the gate valid through process teardown, including the bounded session-end path.
wil::srwlock admissionGate;
static_assert(std::is_trivially_destructible_v<wil::srwlock>);
ProcessWork *currentOwner{};
} // namespace

ProcessWorkLease::~ProcessWorkLease()
{
    Reset();
}
ProcessWorkLease::ProcessWorkLease(ProcessWorkLease &&other) noexcept : owner_(std::exchange(other.owner_, nullptr)) {}
ProcessWorkLease &ProcessWorkLease::operator=(ProcessWorkLease &&other) noexcept
{
    if (this != &other)
    {
        Reset();
        owner_ = std::exchange(other.owner_, nullptr);
    }
    return *this;
}
void ProcessWorkLease::Reset() noexcept
{
    if (auto *owner = std::exchange(owner_, nullptr))
        ProcessWork::Release(owner);
}

ProcessWork::~ProcessWork()
{
    CloseAdmission();
    // An active callback must never outlive a destroyed counter/event owner. The fatal reporter is still alive.
    const HRESULT stopped = Wait(5000u);
    if (FAILED(stopped))
        DxUi::Failure::FailContract(stopped);
}
HRESULT ProcessWork::Start() noexcept
{
    auto guard = admissionGate.lock_exclusive();
    if (currentOwner == this)
        return S_FALSE;
    if (currentOwner || idle_)
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    idle_.reset(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!idle_)
        return HRESULT_FROM_WIN32(GetLastError());
    ownerThread_ = GetCurrentThreadId();
    closed_ = false;
    currentOwner = this;
    return S_OK;
}
HRESULT ProcessWork::Acquire(ProcessWorkLease &lease) noexcept
{
    if (lease)
        return E_INVALIDARG;
    auto guard = admissionGate.lock_exclusive();
    ProcessWork *const owner = currentOwner;
    if (!owner || owner->closed_)
        return HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
    if (owner->active_ == MaximumCallbacks)
        return HRESULT_FROM_WIN32(ERROR_TOO_MANY_TCBS);
    if (owner->active_ == 0u && !ResetEvent(owner->idle_.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    ++owner->active_;
    lease.owner_ = owner;
    return S_OK;
}
void ProcessWork::Release(ProcessWork *owner) noexcept
{
    auto guard = admissionGate.lock_exclusive();
    if (!owner->active_)
        DxUi::Failure::FailContract(E_UNEXPECTED);
    if (--owner->active_ == 0u && !SetEvent(owner->idle_.get()))
        DxUi::Failure::FailContract(HRESULT_FROM_WIN32(GetLastError()));
}
void ProcessWork::CloseAdmission() noexcept
{
    auto guard = admissionGate.lock_exclusive();
    closed_ = true;
    if (currentOwner == this)
        currentOwner = nullptr;
}
std::size_t ProcessWork::ActiveCount() const noexcept
{
    auto guard = admissionGate.lock_shared();
    return active_;
}
HRESULT ProcessWork::Wait(DWORD timeoutMs) noexcept
{
    if (!idle_)
        return S_FALSE;
    if (ownerThread_ != GetCurrentThreadId())
        return RPC_E_WRONG_THREAD;
    if (timeoutMs == INFINITE)
        return E_INVALIDARG;
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
        const DWORD ready = WaitForSingleObject(idle_.get(), 0u);
        if (ready == WAIT_OBJECT_0)
            return S_OK;
        if (ready == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        const ULONGLONG elapsed = GetTickCount64() - started;
        if (elapsed >= timeoutMs)
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        const DWORD wait = static_cast<DWORD>(std::min<ULONGLONG>(50u, timeoutMs - elapsed));
        const HANDLE event = idle_.get();
        const DWORD waited = MsgWaitForMultipleObjectsEx(1u, &event, wait, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (waited == WAIT_OBJECT_0)
            return S_OK;
        if (waited == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        if (waited != WAIT_OBJECT_0 + 1u)
            continue;
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
} // namespace RedTilde
