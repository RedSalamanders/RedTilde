#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace RedTilde
{
struct Heading
{
    unsigned level{};
    std::size_t offset{};
    std::wstring text;
};
struct DocumentInfo
{
    std::size_t words{}, characters{}, lines{};
    std::vector<Heading> headings;
};
[[nodiscard]] DocumentInfo AnalyzeDocument(std::wstring_view text);
[[nodiscard]] std::wstring EquationAt(std::wstring_view text, std::size_t caret);
} // namespace RedTilde
