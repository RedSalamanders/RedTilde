#pragma once
#include <filesystem>
#include <windows.h>
namespace DxUi::Failure
{
struct Record;
}
namespace RedTilde
{
[[nodiscard]] HRESULT InstallCrashHandler(const std::filesystem::path &directory) noexcept;
[[nodiscard]] bool CrashReportingPrepared() noexcept;
// Called only by the prestarted process reporter while the original faulting thread is held.
[[nodiscard]] HRESULT WriteDxUiFailureReport(DxUi::Failure::Record &record) noexcept;
[[noreturn]] void CrashProbe() noexcept;
} // namespace RedTilde
