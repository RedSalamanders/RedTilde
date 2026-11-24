#pragma once
#include "SpellProvider.h"
#include <cstdint>
#include <spellcheck.h>
#include <string>
#include <string_view>
#include <vector>
#include <wil/com.h>

namespace RedTilde
{
// Windows 11 native Spell Checking API provider (<spellcheck.h>).
// STA-affine: create, use, and call ResetComObjects() on the owning STA thread before CoUninitialize().
// Never calls ISpellChecker::Add, Ignore, Remove, or IUserDictionariesRegistrar.
class WinSpellProvider final : public ISpellProvider
{
public:
    static constexpr std::size_t MaximumCachedCheckers = 8u;

    WinSpellProvider() noexcept = default;
    ~WinSpellProvider() noexcept override;

    WinSpellProvider(const WinSpellProvider&)            = delete;
    WinSpellProvider& operator=(const WinSpellProvider&) = delete;

    void ResetComObjects() noexcept;

    HRESULT EnumerateInstalledLanguages(std::vector<std::wstring>& outTags) noexcept override;
    ProviderAvailability CheckAvailability(std::wstring_view languageTag, ProviderDiagnostic* outDiag = nullptr) noexcept override;
    HRESULT CheckSegment(std::wstring_view languageTag,
                         std::wstring_view segment,
                         const SpellCheckOptions& options,
                         std::vector<SpellErrorSpan>& outErrors) noexcept override;
    HRESULT SuggestWord(std::wstring_view languageTag,
                        std::wstring_view word,
                        std::size_t maxSuggestions,
                        std::vector<std::wstring>& outSuggestions) noexcept override;

private:
    struct CachedChecker
    {
        std::wstring normalizedTag;
        wil::com_ptr_nothrow<ISpellChecker> checker;
        std::uint64_t lastUsedTick{};
    };

    [[nodiscard]] HRESULT EnsureFactory(ProviderDiagnostic* outDiag = nullptr) noexcept;
    [[nodiscard]] HRESULT GetOrCreateChecker(std::wstring_view languageTag,
                                             ISpellChecker** outChecker,
                                             ProviderAvailability* outAvailability = nullptr,
                                             ProviderDiagnostic* outDiag           = nullptr) noexcept;

    wil::com_ptr_nothrow<ISpellCheckerFactory> _factory;
    std::vector<CachedChecker> _checkers;
    std::uint64_t _lruClock{};
};
} // namespace RedTilde
