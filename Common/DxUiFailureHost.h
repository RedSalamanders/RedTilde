#pragma once
#include "ProcessWork.h"
#include <DxUi/Failure.h>
#include <atomic>
#include <cstdint>
#include <objbase.h>
#include <wil/resource.h>

namespace RedTilde
{
// Process-owned reporter. InstallCrashHandler prepares its local storage first; the writer starts before
// ProcessHooks. Keep this owner and COM alive until application workers and retained UIA peers have retired.
class DxUiFailureHost final
{
  public:
    DxUiFailureHost() noexcept = default;
    ~DxUiFailureHost();
    DxUiFailureHost(const DxUiFailureHost &) = delete;
    DxUiFailureHost &operator=(const DxUiFailureHost &) = delete;
    DxUiFailureHost(DxUiFailureHost &&) = delete;
    DxUiFailureHost &operator=(DxUiFailureHost &&) = delete;
    [[nodiscard]] HRESULT Start() noexcept;
    // Process shutdown only, outside a window procedure/Failure::Scope, after application worker quiescence.
    // BUSY is retried while this COM-initialized owner pumps; timeout remains a checked failed shutdown.
    [[nodiscard]] HRESULT Retire(DWORD timeoutMs = 5000u) noexcept;

  private:
    static HRESULT Report(void *context, const DxUi::Failure::Record &record) noexcept;
    static DWORD WINAPI Writer(void *context) noexcept;
    // The protocol context is more strongly aligned than the handles. Put it first and
    // account for the tail explicitly so neither profile needs implicit padding.
    DxUi::Failure::Record record_{};
    DxUi::Failure::ProcessHooks hooks_;
    wil::unique_handle request_, stop_, done_, writer_;
    DWORD ownerThread_{};
    std::atomic<unsigned> reportState_{};
    std::atomic<HRESULT> result_{E_PENDING};
    std::uint32_t reservedTail_[3]{};
};
static_assert(sizeof(DxUiFailureHost) == sizeof(DxUi::Failure::Record) + sizeof(DxUi::Failure::ProcessHooks) + 4u * sizeof(wil::unique_handle) +
                                             sizeof(DWORD) + sizeof(std::atomic<unsigned>) + sizeof(std::atomic<HRESULT>) +
                                             3u * sizeof(std::uint32_t));

// Declared after COM initialization and before Application. Its destructor runs after every app window/owner.
class ApplicationRuntime final
{
  public:
    ApplicationRuntime() noexcept = default;
    ~ApplicationRuntime();
    ApplicationRuntime(const ApplicationRuntime &) = delete;
    ApplicationRuntime &operator=(const ApplicationRuntime &) = delete;
    ApplicationRuntime(ApplicationRuntime &&) = delete;
    ApplicationRuntime &operator=(ApplicationRuntime &&) = delete;
    [[nodiscard]] HRESULT Start() noexcept;
    // WM_ENDSESSION has its existing recovery-only allowance. Retain this heap owner until process teardown
    // instead of uninstalling hooks while callbacks remain, or adding an ordinary shutdown join.
    void KeepForProcessTeardown() noexcept;
    [[nodiscard]] HRESULT StopWork(DWORD timeoutMs) noexcept;

  private:
    DxUiFailureHost reporter_;
    ProcessWork work_;
};
} // namespace RedTilde
