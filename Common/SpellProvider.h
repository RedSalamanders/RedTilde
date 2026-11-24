#pragma once
#include <DxUi/Failure.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace RedTilde
{
enum class ProviderAvailability : std::uint8_t
{
    Ready,
    NoDictionary,
    DictionaryUnreadable,
    Unavailable
};

struct SpellCheckOptions
{
    bool ignoreUppercase                                     = true;
    bool ignoreWordsWithDigits                               = true;
    bool ignoreInternetAndPaths                              = true;
    bool operator==(const SpellCheckOptions&) const noexcept = default;
};

struct SpellErrorSpan
{
    std::size_t start{};
    std::size_t length{};
    bool operator==(const SpellErrorSpan&) const noexcept = default;
};

struct ProviderDiagnostic
{
    std::wstring languageTag;
    std::wstring message;
    bool operator==(const ProviderDiagnostic&) const = default;
};

[[nodiscard]] inline bool IsHostileUiControlOrBidiScalar(std::uint32_t scalar) noexcept
{
    // Strip C0/C1 controls, Arabic letter mark, LTR/RTL marks & embeddings/overrides/isolates,
    // zero-width characters, word joiner / BOM, and Unicode tag block (U+E0000..U+E007F).
    if (scalar <= 0x001Fu || (scalar >= 0x007Fu && scalar <= 0x009Fu))
        return true;
    if (scalar == 0x061Cu || scalar == 0x200Bu || scalar == 0x200Cu || scalar == 0x200Du || scalar == 0x200Eu || scalar == 0x200Fu || scalar == 0x2060u ||
        scalar == 0xFEFFu)
        return true;
    if ((scalar >= 0x202Au && scalar <= 0x202Eu) || (scalar >= 0x2066u && scalar <= 0x2069u))
        return true;
    if (scalar >= 0xE0000u && scalar <= 0xE007Fu)
        return true;
    return false;
}

[[nodiscard]] inline std::wstring SanitizeSpellUiText(std::wstring_view input, std::size_t maxChars = 256u)
{
    std::wstring out;
    if (input.empty() || maxChars == 0u)
        return out;
    out.reserve(std::min(input.size(), maxChars));
    bool lastWasSpace = true; // Trim leading whitespace.
    for (std::size_t i = 0; i < input.size() && out.size() < maxChars;)
    {
        const wchar_t ch     = input[i];
        std::uint32_t scalar = static_cast<std::uint32_t>(ch);
        std::size_t advance  = 1u;
        if (IS_HIGH_SURROGATE(ch))
        {
            if (i + 1u < input.size() && IS_LOW_SURROGATE(input[i + 1u]))
            {
                const std::uint32_t high = static_cast<std::uint32_t>(ch) - 0xD800u;
                const std::uint32_t low  = static_cast<std::uint32_t>(input[i + 1u]) - 0xDC00u;
                scalar                   = 0x10000u + ((high << 10u) | low);
                advance                  = 2u;
            }
            else
            {
                ++i;
                continue; // Drop unpaired high surrogate.
            }
        }
        else if (IS_LOW_SURROGATE(ch))
        {
            ++i;
            continue; // Drop unpaired low surrogate.
        }

        i += advance;
        if (scalar == L'\r' || scalar == L'\n' || scalar == L'\t' || scalar == L' ')
        {
            if (! lastWasSpace && out.size() < maxChars)
            {
                out.push_back(L' ');
                lastWasSpace = true;
            }
            continue;
        }
        if (IsHostileUiControlOrBidiScalar(scalar))
            continue;

        if (out.size() + advance > maxChars)
            break;
        if (advance == 2u)
        {
            out.push_back(ch);
            out.push_back(input[i - 1u]);
        }
        else
        {
            out.push_back(ch);
        }
        lastWasSpace = false;
    }
    while (! out.empty() && out.back() == L' ')
        out.pop_back();
    return out;
}

class ISpellProvider
{
public:
    virtual ~ISpellProvider() noexcept                                                                                            = default;
    virtual HRESULT EnumerateInstalledLanguages(std::vector<std::wstring>& outTags) noexcept                                      = 0;
    virtual ProviderAvailability CheckAvailability(std::wstring_view languageTag, ProviderDiagnostic* outDiag = nullptr) noexcept = 0;
    virtual HRESULT CheckSegment(std::wstring_view languageTag,
                                 std::wstring_view segment,
                                 const SpellCheckOptions& options,
                                 std::vector<SpellErrorSpan>& outErrors) noexcept                                                 = 0;
    virtual HRESULT SuggestWord(std::wstring_view languageTag,
                                std::wstring_view word,
                                std::size_t maxSuggestions,
                                std::vector<std::wstring>& outSuggestions) noexcept                                               = 0;
};

namespace Detail
{
[[nodiscard]] inline bool IsWordJoinerChar(wchar_t ch) noexcept
{
    return ch == L'\'' || ch == L'\x2019' || ch == L'-';
}

[[nodiscard]] inline bool IsTokenLetterOrDigit(wchar_t ch) noexcept
{
    if (IS_HIGH_SURROGATE(ch) || IS_LOW_SURROGATE(ch))
        return true;
    WORD type{};
    if (! GetStringTypeW(CT_CTYPE1, &ch, 1, &type))
        return false;
    return (type & (C1_ALPHA | C1_DIGIT)) != 0u;
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

[[nodiscard]] inline bool IsUppercaseAcronymFake(std::wstring_view word) noexcept
{
    bool hasUpper     = false;
    std::size_t cased = 0u;
    for (const wchar_t ch : word)
    {
        WORD type{};
        if (! GetStringTypeW(CT_CTYPE1, &ch, 1, &type))
            continue;
        if (type & C1_LOWER)
            return false;
        if (type & C1_UPPER)
        {
            hasUpper = true;
            ++cased;
        }
    }
    return hasUpper && cased >= 2u;
}

[[nodiscard]] inline bool ContainsDigitFake(std::wstring_view word) noexcept
{
    for (const wchar_t ch : word)
    {
        WORD type{};
        if (GetStringTypeW(CT_CTYPE1, &ch, 1, &type) && (type & C1_DIGIT) != 0u)
            return true;
    }
    return false;
}

[[nodiscard]] inline std::size_t LevenshteinBounded(std::wstring_view a, std::wstring_view b)
{
    const std::size_t n = a.size(), m = b.size();
    if (n == 0u)
        return m;
    if (m == 0u)
        return n;
    std::vector<std::size_t> prev(m + 1u), curr(m + 1u);
    for (std::size_t j = 0; j <= m; ++j)
        prev[j] = j;
    for (std::size_t i = 0; i < n; ++i)
    {
        curr[0] = i + 1u;
        for (std::size_t j = 0; j < m; ++j)
        {
            const std::size_t cost = (a[i] == b[j]) ? 0u : 1u;
            curr[j + 1u]           = std::min({prev[j + 1u] + 1u, curr[j] + 1u, prev[j] + cost});
        }
        prev.swap(curr);
    }
    return prev[m];
}
} // namespace Detail

// Deterministic in-memory spell provider for headless tests and --capture scenarios.
class FakeSpellProvider final : public ISpellProvider
{
public:
    FakeSpellProvider()                                    = default;
    FakeSpellProvider(const FakeSpellProvider&)            = delete;
    FakeSpellProvider& operator=(const FakeSpellProvider&) = delete;
    FakeSpellProvider(FakeSpellProvider&&)                 = delete;
    FakeSpellProvider& operator=(FakeSpellProvider&&)      = delete;

    void SetInstalledLanguages(std::vector<std::wstring> tags)
    {
        std::lock_guard lock(_mutex);
        _installedLanguages = std::move(tags);
    }

    void SetLanguageAvailability(std::wstring_view languageTag, ProviderAvailability availability, std::wstring diagnosticMessage = {})
    {
        std::lock_guard lock(_mutex);
        auto& entry        = _languages[Detail::FoldAsciiLower(languageTag)];
        entry.availability = availability;
        entry.diagnostic   = std::move(diagnosticMessage);
    }

    void AddKnownWords(std::wstring_view languageTag, std::initializer_list<std::wstring_view> words)
    {
        std::lock_guard lock(_mutex);
        auto& entry = _languages[Detail::FoldAsciiLower(languageTag)];
        for (const auto word : words)
        {
            entry.knownExact.emplace(word);
            entry.knownLower.emplace(Detail::FoldAsciiLower(word));
        }
    }

    void AddKnownWord(std::wstring_view languageTag, std::wstring_view word)
    {
        std::lock_guard lock(_mutex);
        auto& entry = _languages[Detail::FoldAsciiLower(languageTag)];
        entry.knownExact.emplace(word);
        entry.knownLower.emplace(Detail::FoldAsciiLower(word));
    }

    void SetSuggestions(std::wstring_view languageTag, std::wstring_view word, std::vector<std::wstring> suggestions)
    {
        std::lock_guard lock(_mutex);
        auto& entry                                     = _languages[Detail::FoldAsciiLower(languageTag)];
        entry.suggestions[Detail::FoldAsciiLower(word)] = std::move(suggestions);
    }

    void SetArtificialDelayMs(DWORD delayMs) noexcept
    {
        _artificialDelayMs.store(delayMs, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t CheckSegmentCallCount() const noexcept
    {
        return _checkCalls.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t SuggestCallCount() const noexcept
    {
        return _suggestCalls.load(std::memory_order_relaxed);
    }

    HRESULT EnumerateInstalledLanguages(std::vector<std::wstring>& outTags) noexcept override
    try
    {
        std::lock_guard lock(_mutex);
        outTags = _installedLanguages;
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (...)
    {
        return E_FAIL;
    }

    ProviderAvailability CheckAvailability(std::wstring_view languageTag, ProviderDiagnostic* outDiag = nullptr) noexcept override
    try
    {
        std::lock_guard lock(_mutex);
        const auto it = _languages.find(Detail::FoldAsciiLower(languageTag));
        if (it == _languages.end())
        {
            if (outDiag)
            {
                outDiag->languageTag = SanitizeSpellUiText(languageTag, 32u);
                outDiag->message     = L"";
            }
            return ProviderAvailability::NoDictionary;
        }
        if (outDiag)
        {
            outDiag->languageTag = SanitizeSpellUiText(languageTag, 32u);
            outDiag->message     = SanitizeSpellUiText(it->second.diagnostic, 256u);
        }
        return it->second.availability;
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (...)
    {
        return ProviderAvailability::Unavailable;
    }

    HRESULT CheckSegment(std::wstring_view languageTag,
                         std::wstring_view segment,
                         const SpellCheckOptions& options,
                         std::vector<SpellErrorSpan>& outErrors) noexcept override
    try
    {
        outErrors.clear();
        _checkCalls.fetch_add(1u, std::memory_order_relaxed);
        if (const DWORD delay = _artificialDelayMs.load(std::memory_order_relaxed); delay > 0u)
            Sleep(delay);

        std::lock_guard lock(_mutex);
        const auto it = _languages.find(Detail::FoldAsciiLower(languageTag));
        if (it == _languages.end() || it->second.availability != ProviderAvailability::Ready)
            return S_OK;

        const auto& lang      = it->second;
        const std::size_t len = segment.size();
        std::size_t i         = 0u;
        while (i < len)
        {
            while (i < len && (segment[i] == L'\uFFFC' || ! Detail::IsTokenLetterOrDigit(segment[i])))
                ++i;
            if (i >= len)
                break;
            std::size_t start = i;
            while (i < len)
            {
                if (segment[i] == L'\uFFFC')
                    break;
                if (Detail::IsTokenLetterOrDigit(segment[i]))
                {
                    ++i;
                    continue;
                }
                if (Detail::IsWordJoinerChar(segment[i]) && i + 1u < len && Detail::IsTokenLetterOrDigit(segment[i + 1u]))
                {
                    ++i;
                    continue;
                }
                break;
            }
            const std::size_t wordLen = i - start;
            if (wordLen == 0u)
                continue;

            const std::wstring_view token = segment.substr(start, wordLen);
            if (options.ignoreUppercase && Detail::IsUppercaseAcronymFake(token))
                continue;
            if (options.ignoreWordsWithDigits && Detail::ContainsDigitFake(token))
                continue;

            if (lang.knownExact.contains(std::wstring(token)) || lang.knownLower.contains(Detail::FoldAsciiLower(token)))
                continue;

            outErrors.push_back(SpellErrorSpan{start, wordLen});
        }
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (...)
    {
        return E_FAIL;
    }

    HRESULT SuggestWord(std::wstring_view languageTag,
                        std::wstring_view word,
                        std::size_t maxSuggestions,
                        std::vector<std::wstring>& outSuggestions) noexcept override
    try
    {
        outSuggestions.clear();
        _suggestCalls.fetch_add(1u, std::memory_order_relaxed);
        if (maxSuggestions == 0u || word.empty())
            return S_OK;

        std::lock_guard lock(_mutex);
        const auto it = _languages.find(Detail::FoldAsciiLower(languageTag));
        if (it == _languages.end() || it->second.availability != ProviderAvailability::Ready)
            return S_OK;

        const auto& lang       = it->second;
        const std::wstring key = Detail::FoldAsciiLower(word);
        if (const auto customIt = lang.suggestions.find(key); customIt != lang.suggestions.end())
        {
            for (const auto& candidate : customIt->second)
            {
                outSuggestions.push_back(SanitizeSpellUiText(candidate, 64u));
                if (outSuggestions.size() >= maxSuggestions)
                    return S_OK;
            }
        }

        struct ScoredCandidate
        {
            std::size_t dist{};
            std::wstring text;
        };
        std::vector<ScoredCandidate> scored;
        for (const auto& known : lang.knownExact)
        {
            const std::size_t dist = Detail::LevenshteinBounded(key, Detail::FoldAsciiLower(known));
            if (dist > 0u && dist <= 3u)
                scored.push_back({dist, known});
        }
        std::sort(scored.begin(),
                  scored.end(),
                  [](const ScoredCandidate& a, const ScoredCandidate& b)
        {
            if (a.dist != b.dist)
                return a.dist < b.dist;
            return a.text < b.text;
        });
        for (const auto& candidate : scored)
        {
            if (std::find(outSuggestions.begin(), outSuggestions.end(), candidate.text) == outSuggestions.end())
            {
                outSuggestions.push_back(candidate.text);
                if (outSuggestions.size() >= maxSuggestions)
                    break;
            }
        }
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (...)
    {
        return E_FAIL;
    }

private:
    struct LanguageData
    {
        ProviderAvailability availability = ProviderAvailability::Ready;
        std::wstring diagnostic;
        std::set<std::wstring> knownExact;
        std::set<std::wstring> knownLower;
        std::unordered_map<std::wstring, std::vector<std::wstring>> suggestions;
    };

    mutable std::mutex _mutex;
    std::vector<std::wstring> _installedLanguages;
    std::unordered_map<std::wstring, LanguageData> _languages;
    std::atomic<std::uint64_t> _checkCalls{0u};
    std::atomic<std::uint64_t> _suggestCalls{0u};
    std::atomic<DWORD> _artificialDelayMs{0u};
};
} // namespace RedTilde
