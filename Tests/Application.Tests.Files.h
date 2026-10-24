#pragma once
#include <filesystem>
#include <functional>
#include <string_view>

namespace RedTilde
{
void RegisterFilesTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test);
} // namespace RedTilde
