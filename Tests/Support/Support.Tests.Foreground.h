#pragma once
#include <stdexcept>
#include <wil/resource.h>
#include <windows.h>

namespace RedTilde::Testing
{
// Observe the test process, rather than requiring an unrelated user window to
// remain foreground throughout a hidden scenario. The native event filter records
// ownership when the event is emitted, even if that HWND is subsequently retired.
class ForegroundOwnershipObserver final
{
  public:
    ForegroundOwnershipObserver() : _previous(_current)
    {
        _hook.reset(SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, OnForeground, GetCurrentProcessId(), 0,
                                    WINEVENT_OUTOFCONTEXT));
        if (!_hook)
            throw std::runtime_error("install the hidden-test foreground ownership observer");
        _current = this;
        ObserveCurrent();
    }
    ~ForegroundOwnershipObserver()
    {
        _current = _previous;
    }
    ForegroundOwnershipObserver(const ForegroundOwnershipObserver &) = delete;
    ForegroundOwnershipObserver &operator=(const ForegroundOwnershipObserver &) = delete;
    ForegroundOwnershipObserver(ForegroundOwnershipObserver &&) = delete;
    ForegroundOwnershipObserver &operator=(ForegroundOwnershipObserver &&) = delete;

    [[nodiscard]] bool NeverAcquiredForeground() noexcept
    {
        // Peek dispatches queued out-of-context WinEvent callbacks on this owning
        // thread. Leave ordinary messages queued for the scenario's owned pump.
        MSG message{};
        (void)PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE);
        ObserveCurrent();
        return !_observedOwned;
    }

  private:
    friend struct ForegroundOwnershipObserverProbe;
    static void CALLBACK OnForeground(HWINEVENTHOOK hook, DWORD event, HWND, LONG, LONG, DWORD, DWORD) noexcept
    {
        for (auto *observer = _current; observer; observer = observer->_previous)
            if (observer->_hook.get() == hook && event == EVENT_SYSTEM_FOREGROUND)
                observer->_observedOwned = true;
    }
    void ObserveCurrent() noexcept
    {
        DWORD process{};
        if (GetWindowThreadProcessId(GetForegroundWindow(), &process) && process == GetCurrentProcessId())
            _observedOwned = true;
    }
    wil::unique_hwineventhook _hook;
    ForegroundOwnershipObserver *_previous;
    bool _observedOwned = false;
    inline static thread_local ForegroundOwnershipObserver *_current = nullptr;
};

// Exercise the actual observer callback without emitting a false accessibility
// event or activating any window on the user's desktop.
struct ForegroundOwnershipObserverProbe
{
    static void ReplayOwnedEvent(ForegroundOwnershipObserver &observer) noexcept
    {
        ForegroundOwnershipObserver::OnForeground(observer._hook.get(), EVENT_SYSTEM_FOREGROUND, nullptr, OBJID_WINDOW, 0, 0, 0);
    }
};
} // namespace RedTilde::Testing
