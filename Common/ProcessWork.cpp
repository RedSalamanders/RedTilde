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
ProcessWork* currentOwner{};
} // namespace

ProcessWorkLease::~ProcessWorkLease()
{
    Reset();
}
ProcessWorkLease::ProcessWorkLease(ProcessWorkLease&& other) noexcept : _owner(std::exchange(other._owner, nullptr))
{
}
ProcessWorkLease& ProcessWorkLease::operator=(ProcessWorkLease&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        _owner = std::exchange(other._owner, nullptr);
    }
    return *this;
}
void ProcessWorkLease::Reset() noexcept
{
    if (auto* owner = std::exchange(_owner, nullptr))
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
    if (currentOwner || _idle)
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    _idle.reset(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (! _idle)
        return HRESULT_FROM_WIN32(GetLastError());
    _ownerThread = GetCurrentThreadId();
    _closed      = false;
    currentOwner = this;
    return S_OK;
}
HRESULT ProcessWork::Acquire(ProcessWorkLease& lease) noexcept
{
    if (lease)
        return E_INVALIDARG;
    auto guard               = admissionGate.lock_exclusive();
    ProcessWork* const owner = currentOwner;
    if (! owner || owner->_closed)
        return HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
    if (owner->_active == MaximumCallbacks)
        return HRESULT_FROM_WIN32(ERROR_TOO_MANY_TCBS);
    if (owner->_active == 0u && ! ResetEvent(owner->_idle.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    ++owner->_active;
    lease._owner = owner;
    return S_OK;
}
void ProcessWork::Release(ProcessWork* owner) noexcept
{
    auto guard = admissionGate.lock_exclusive();
    if (! owner->_active)
        DxUi::Failure::FailContract(E_UNEXPECTED);
    if (--owner->_active == 0u && ! SetEvent(owner->_idle.get()))
        DxUi::Failure::FailContract(HRESULT_FROM_WIN32(GetLastError()));
}
void ProcessWork::CloseAdmission() noexcept
{
    auto guard = admissionGate.lock_exclusive();
    _closed    = true;
    if (currentOwner == this)
        currentOwner = nullptr;
}
std::size_t ProcessWork::ActiveCount() const noexcept
{
    auto guard = admissionGate.lock_shared();
    return _active;
}
HRESULT ProcessWork::Wait(DWORD timeoutMs) noexcept
{
    if (! _idle)
        return S_FALSE;
    if (_ownerThread != GetCurrentThreadId())
        return RPC_E_WRONG_THREAD;
    if (timeoutMs == INFINITE)
        return E_INVALIDARG;
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
        const DWORD ready = WaitForSingleObject(_idle.get(), 0u);
        if (ready == WAIT_OBJECT_0)
            return S_OK;
        if (ready == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        const ULONGLONG elapsed = GetTickCount64() - started;
        if (elapsed >= timeoutMs)
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        const DWORD wait   = static_cast<DWORD>(std::min<ULONGLONG>(50u, timeoutMs - elapsed));
        const HANDLE event = _idle.get();
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
} // namespace RedTilde
