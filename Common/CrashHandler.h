#pragma once
#include <filesystem>
#include <optional>
#include <windows.h>
namespace DxUi::Failure
{
struct Record;
}
namespace RedTilde
{
[[nodiscard]] HRESULT InstallCrashHandler(const std::filesystem::path& directory) noexcept;
void RetainRecentCrashReports(const std::filesystem::path& directory);
[[nodiscard]] std::optional<std::filesystem::path> TakePendingCrashNotice(const std::filesystem::path& directory, bool interactive);
[[nodiscard]] bool CrashReportingPrepared() noexcept;
// Called only by the prestarted process reporter while the original faulting thread is held.
[[nodiscard]] HRESULT WriteDxUiFailureReport(DxUi::Failure::Record& record) noexcept;
#if defined(ENABLE_TESTS)
[[noreturn]] void CrashProbe() noexcept;
#endif
} // namespace RedTilde
