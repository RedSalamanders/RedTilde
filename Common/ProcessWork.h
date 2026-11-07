#pragma once
#include <cstddef>
#include <memory>
#include <utility>
#include <wil/resource.h>
#include <windows.h>

namespace RedTilde
{
class ProcessWork;
// Move one admitted callback's lease into its owned job before submission. Declare the callback-local lease
// before SDK scopes/owners so it is released only after those owners have been destroyed.
class ProcessWorkLease final
{
  public:
    ProcessWorkLease() noexcept = default;
    ~ProcessWorkLease();
    ProcessWorkLease(const ProcessWorkLease &) = delete;
    ProcessWorkLease &operator=(const ProcessWorkLease &) = delete;
    ProcessWorkLease(ProcessWorkLease &&other) noexcept;
    ProcessWorkLease &operator=(ProcessWorkLease &&other) noexcept;
    explicit operator bool() const noexcept
    {
        return owner_ != nullptr;
    }
    void Reset() noexcept;

  private:
    friend class ProcessWork;
    ProcessWork *owner_{};
};

// Explicit, single-EXE process owner. Its only module-global state is a guarded borrowed admission pointer;
// callbacks hold counted leases rather than depending on an Application/HWND lifetime. No SDK object crosses it.
class ProcessWork final
{
  public:
    static constexpr std::size_t MaximumCallbacks = 256u;
    ProcessWork() noexcept = default;
    ~ProcessWork();
    ProcessWork(const ProcessWork &) = delete;
    ProcessWork &operator=(const ProcessWork &) = delete;
    ProcessWork(ProcessWork &&) = delete;
    ProcessWork &operator=(ProcessWork &&) = delete;
    [[nodiscard]] HRESULT Start() noexcept;
    [[nodiscard]] static HRESULT Acquire(ProcessWorkLease &lease) noexcept;
    void CloseAdmission() noexcept;
    // Owner-thread shutdown only, after closing application producers. Pumps COM/owner messages while waiting.
    // Session end must retain this owner for process teardown instead of adding this ordinary shutdown wait.
    [[nodiscard]] HRESULT Wait(DWORD timeoutMs = 5000u) noexcept;
    [[nodiscard]] std::size_t ActiveCount() const noexcept;

  private:
    friend class ProcessWorkLease;
    static void Release(ProcessWork *owner) noexcept;
    wil::unique_handle idle_;
    std::size_t active_{};
    DWORD ownerThread_{};
    bool closed_ = true;
};

// Canonical shared-state callback ownership used by the Files, Recovery and Settings services.
template <typename State> struct ProcessWorkItem final
{
    explicit ProcessWorkItem(std::shared_ptr<State> value) noexcept : state(std::move(value)) {}
    ProcessWorkItem(const ProcessWorkItem &) = delete;
    ProcessWorkItem &operator=(const ProcessWorkItem &) = delete;
    ProcessWorkItem(ProcessWorkItem &&) = delete;
    ProcessWorkItem &operator=(ProcessWorkItem &&) = delete;
    ProcessWorkLease lease;
    std::shared_ptr<State> state;
};
} // namespace RedTilde
