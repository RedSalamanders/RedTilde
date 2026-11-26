#pragma once
// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// clang-format on

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace RedTilde
{
[[nodiscard]] inline bool EqualOrdinal(std::wstring_view left, std::wstring_view right) noexcept
{
    return left == right;
}

[[nodiscard]] inline bool EqualOrdinalIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
{
    if (left.size() != right.size())
        return false;
    if (left.empty())
        return true;
    if (left.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

[[nodiscard]] inline bool LessOrdinalIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
{
    if (left.empty() || right.empty())
        return left.empty() && ! right.empty();
    const int leftLen  = static_cast<int>(std::min<std::size_t>(left.size(), static_cast<std::size_t>(std::numeric_limits<int>::max())));
    const int rightLen = static_cast<int>(std::min<std::size_t>(right.size(), static_cast<std::size_t>(std::numeric_limits<int>::max())));
    return CompareStringOrdinal(left.data(), leftLen, right.data(), rightLen, TRUE) == CSTR_LESS_THAN;
}

[[nodiscard]] inline bool StartsWithOrdinalIgnoreCase(std::wstring_view text, std::wstring_view prefix) noexcept
{
    return prefix.size() <= text.size() && (prefix.empty() || EqualOrdinalIgnoreCase(text.substr(0u, prefix.size()), prefix));
}

[[nodiscard]] inline bool StartsWithAsciiOrdinalIgnoreCase(std::wstring_view text, std::wstring_view asciiPrefix) noexcept
{
    if (asciiPrefix.size() > text.size())
        return false;
    for (std::size_t i = 0u; i < asciiPrefix.size(); ++i)
    {
        if (static_cast<unsigned>(text[i]) >= 0x80u)
            return false;
    }
    return asciiPrefix.empty() || EqualOrdinalIgnoreCase(text.substr(0u, asciiPrefix.size()), asciiPrefix);
}

[[nodiscard]] inline bool EndsWithOrdinalIgnoreCase(std::wstring_view text, std::wstring_view suffix) noexcept
{
    return suffix.size() <= text.size() && (suffix.empty() || EqualOrdinalIgnoreCase(text.substr(text.size() - suffix.size()), suffix));
}

[[nodiscard]] inline std::size_t FindOrdinalIgnoreCase(std::wstring_view text, std::wstring_view needle, std::size_t from = 0u) noexcept
{
    if (from > text.size())
        return std::wstring_view::npos;
    if (needle.empty())
        return from;
    const std::size_t remaining = text.size() - from;
    if (needle.size() > remaining || remaining > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return std::wstring_view::npos;
    const int found = FindStringOrdinal(FIND_FROMSTART, text.data() + from, static_cast<int>(remaining), needle.data(), static_cast<int>(needle.size()), TRUE);
    return found >= 0 ? from + static_cast<std::size_t>(found) : std::wstring_view::npos;
}

[[nodiscard]] inline bool ContainsOrdinalIgnoreCase(std::wstring_view text, std::wstring_view needle) noexcept
{
    return FindOrdinalIgnoreCase(text, needle) != std::wstring_view::npos;
}

[[nodiscard]] inline std::wstring FoldAsciiLower(std::wstring_view text)
{
    std::wstring folded;
    folded.reserve(text.size());
    for (const wchar_t ch : text)
    {
        if (ch >= L'A' && ch <= L'Z')
            folded.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        else
            folded.push_back(ch);
    }
    return folded;
}

[[nodiscard]] constexpr bool IsValidBcp47Tag(std::wstring_view tag) noexcept
{
    if (tag.empty() || tag.size() > 32u)
        return false;
    bool hasAlpha = false;
    for (const wchar_t ch : tag)
    {
        const bool isAlpha = (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z');
        const bool isDigit = (ch >= L'0' && ch <= L'9');
        if (isAlpha)
            hasAlpha = true;
        else if (! isDigit && ch != L'-')
            return false;
    }
    return hasAlpha && tag.front() != L'-' && tag.back() != L'-';
}

[[nodiscard]] inline bool TryParseAsciiBcp47Tag(std::string_view utf8Tag, std::wstring& outWide)
{
    outWide.clear();
    if (utf8Tag.empty() || utf8Tag.size() > 32u)
        return false;
    bool hasAlpha = false;
    for (const char ch : utf8Tag)
    {
        const bool isAlpha = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
        const bool isDigit = (ch >= '0' && ch <= '9');
        if (isAlpha)
            hasAlpha = true;
        else if (! isDigit && ch != '-')
            return false;
    }
    if (! hasAlpha || utf8Tag.front() == '-' || utf8Tag.back() == '-')
        return false;
    outWide.reserve(utf8Tag.size());
    for (const char ch : utf8Tag)
        outWide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
    return true;
}

[[nodiscard]] inline WORD GetCharCtype1(wchar_t ch) noexcept
{
    WORD type{};
    return GetStringTypeW(CT_CTYPE1, &ch, 1, &type) ? type : static_cast<WORD>(0u);
}

[[nodiscard]] inline bool IsCharAlphaOrDigit(wchar_t ch) noexcept
{
    return (GetCharCtype1(ch) & (C1_ALPHA | C1_DIGIT)) != 0u;
}

[[nodiscard]] inline bool IsCharDigit(wchar_t ch) noexcept
{
    return (GetCharCtype1(ch) & C1_DIGIT) != 0u;
}

[[nodiscard]] constexpr bool IsWordJoinerChar(wchar_t ch) noexcept
{
    return ch == L'\'' || ch == L'\x2019' || ch == L'-';
}

inline void AnalyzeWordCtype1(std::wstring_view word, bool& outIsAllUpper, bool& outHasDigit) noexcept
{
    outIsAllUpper = false;
    outHasDigit   = false;
    if (word.empty())
        return;
    bool hasUpper     = false;
    bool hasLower     = false;
    std::size_t cased = 0u;
    std::array<WORD, 64u> types{};
    std::size_t offset = 0u;
    while (offset < word.size())
    {
        const std::size_t count = (std::min)(word.size() - offset, types.size());
        if (! GetStringTypeW(CT_CTYPE1, word.data() + offset, static_cast<int>(count), types.data()))
            return;
        for (std::size_t i = 0u; i < count; ++i)
        {
            const WORD type = types[i];
            if ((type & C1_DIGIT) != 0u)
                outHasDigit = true;
            if ((type & C1_LOWER) != 0u)
                hasLower = true;
            else if ((type & C1_UPPER) != 0u)
            {
                hasUpper = true;
                ++cased;
            }
        }
        offset += count;
    }
    outIsAllUpper = ! hasLower && hasUpper && cased >= 2u;
}

// Script-aware uppercase filter: returns true only when cased uppercase letters are present (>= 2),
// zero lowercase letters are present, and no caseless-only scripts are misclassified as acronyms.
[[nodiscard]] inline bool IsScriptAwareAllUppercase(std::wstring_view word) noexcept
{
    if (word.empty())
        return false;
    bool hasUpper     = false;
    std::size_t cased = 0u;
    std::array<WORD, 64u> types{};
    std::size_t offset = 0u;
    while (offset < word.size())
    {
        const std::size_t count = (std::min)(word.size() - offset, types.size());
        if (! GetStringTypeW(CT_CTYPE1, word.data() + offset, static_cast<int>(count), types.data()))
        {
            offset += count;
            continue;
        }
        for (std::size_t i = 0u; i < count; ++i)
        {
            const WORD type = types[i];
            if ((type & C1_LOWER) != 0u)
                return false;
            if ((type & C1_UPPER) != 0u)
            {
                hasUpper = true;
                ++cased;
            }
        }
        offset += count;
    }
    return hasUpper && cased >= 2u;
}

[[nodiscard]] inline bool ContainsAsciiOrUnicodeDigit(std::wstring_view word) noexcept
{
    if (word.empty())
        return false;
    std::array<WORD, 64u> types{};
    std::size_t offset = 0u;
    while (offset < word.size())
    {
        const std::size_t count = (std::min)(word.size() - offset, types.size());
        if (! GetStringTypeW(CT_CTYPE1, word.data() + offset, static_cast<int>(count), types.data()))
        {
            offset += count;
            continue;
        }
        for (std::size_t i = 0u; i < count; ++i)
        {
            if ((types[i] & C1_DIGIT) != 0u)
                return true;
        }
        offset += count;
    }
    return false;
}
} // namespace RedTilde
