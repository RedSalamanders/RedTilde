#pragma once
#include <filesystem>
#include <windows.h>
namespace RedTilde
{
[[nodiscard]] HRESULT InstallCrashHandler(const std::filesystem::path &directory) noexcept;
[[noreturn]] void CrashProbe() noexcept;
} // namespace RedTilde
