#pragma once
#include <filesystem>
#include <functional>
#include <string_view>

namespace RedTilde
{
class ApplicationRuntime;
inline constexpr int RetainRuntimeForSessionEnd = 73;
int RunFailureHostProbe(std::wstring_view kind, const std::filesystem::path &root, ApplicationRuntime &runtime);
void RegisterFailureHostTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test);
} // namespace RedTilde
