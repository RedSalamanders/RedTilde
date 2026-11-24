#include "WinSpellProvider.h"
#include "NativeCalls.h"
#include "ProseExtractor.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <new>
#include <utility>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
[[nodiscard]] std::wstring NormalizeTagKey(std::wstring_view tag)
{
    std::wstring key;
    key.reserve(tag.size());
    for (const wchar_t ch : tag)
    {
        if (ch >= L'A' && ch <= L'Z')
            key.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        else
            key.push_back(ch);
    }
    return key;
}

[[nodiscard]] bool IsValidUtf16Span(std::wstring_view segment, std::size_t start, std::size_t length) noexcept
{
    if (length == 0u || start >= segment.size() || length > segment.size() - start)
        return false;
    const std::size_t end = start + length;
    if (IS_LOW_SURROGATE(segment[start]))
        return false;
    if (end < segment.size() && IS_LOW_SURROGATE(segment[end]))
        return false;
    return true;
}
} // namespace

WinSpellProvider::~WinSpellProvider() noexcept
{
    ResetComObjects();
}

void WinSpellProvider::ResetComObjects() noexcept
{
    _checkers.clear();
    _factory.reset();
}

HRESULT WinSpellProvider::EnsureFactory(ProviderDiagnostic* outDiag) noexcept
{
    if (_factory)
        return S_OK;
    const HRESULT hr = DxUi::Failure::InvokeNative(
        &CoCreateInstance, __uuidof(SpellCheckerFactory), nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER), IID_PPV_ARGS(_factory.put()));
    if (FAILED(hr) && outDiag)
        outDiag->message = SanitizeSpellUiText(L"Windows Spell Checking service unavailable", 256u);
    return hr;
}

HRESULT WinSpellProvider::GetOrCreateChecker(std::wstring_view languageTag,
                                             ISpellChecker** outChecker,
                                             ProviderAvailability* outAvailability,
                                             ProviderDiagnostic* outDiag) noexcept
try
{
    if (outChecker)
        *outChecker = nullptr;
    if (outAvailability)
        *outAvailability = ProviderAvailability::Unavailable;

    const std::wstring cleanTag = SanitizeSpellUiText(languageTag, 32u);
    if (cleanTag.empty())
    {
        if (outAvailability)
            *outAvailability = ProviderAvailability::NoDictionary;
        return E_INVALIDARG;
    }

    const HRESULT factoryHr = EnsureFactory(outDiag);
    if (FAILED(factoryHr))
        return factoryHr;

    const std::wstring normalized = NormalizeTagKey(cleanTag);
    for (auto& cached : _checkers)
    {
        if (cached.normalizedTag == normalized && cached.checker)
        {
            cached.lastUsedTick = ++_lruClock;
            if (outChecker)
                cached.checker.copy_to(outChecker);
            if (outAvailability)
                *outAvailability = ProviderAvailability::Ready;
            return S_OK;
        }
    }

    BOOL supported      = FALSE;
    const HRESULT supHr = InvokeNativeMethod(_factory.get(), &ISpellCheckerFactory::IsSupported, cleanTag.c_str(), &supported);
    if (FAILED(supHr))
    {
        if (outAvailability)
            *outAvailability = ProviderAvailability::Unavailable;
        if (outDiag)
        {
            outDiag->languageTag = cleanTag;
            outDiag->message     = SanitizeSpellUiText(L"Could not query Windows dictionary support", 256u);
        }
        return supHr;
    }
    if (! supported)
    {
        if (outAvailability)
            *outAvailability = ProviderAvailability::NoDictionary;
        if (outDiag)
        {
            outDiag->languageTag = cleanTag;
            outDiag->message     = SanitizeSpellUiText(L"Dictionary not installed for language", 256u);
        }
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    wil::com_ptr_nothrow<ISpellChecker> checker;
    const HRESULT createHr = InvokeNativeMethod(_factory.get(), &ISpellCheckerFactory::CreateSpellChecker, cleanTag.c_str(), checker.put());
    if (FAILED(createHr) || ! checker)
    {
        if (outAvailability)
            *outAvailability = ProviderAvailability::DictionaryUnreadable;
        if (outDiag)
        {
            outDiag->languageTag = cleanTag;
            outDiag->message     = SanitizeSpellUiText(L"Dictionary could not be loaded", 256u);
        }
        return FAILED(createHr) ? createHr : E_FAIL;
    }

    if (_checkers.size() >= MaximumCachedCheckers)
    {
        const auto oldest = std::ranges::min_element(_checkers, {}, &CachedChecker::lastUsedTick);
        if (oldest != _checkers.end())
            _checkers.erase(oldest);
    }

    CachedChecker entry;
    entry.normalizedTag = normalized;
    entry.checker       = checker;
    entry.lastUsedTick  = ++_lruClock;
    _checkers.push_back(std::move(entry));

    if (outChecker)
        checker.copy_to(outChecker);
    if (outAvailability)
        *outAvailability = ProviderAvailability::Ready;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WinSpellProvider::EnumerateInstalledLanguages(std::vector<std::wstring>& outTags) noexcept
try
{
    outTags.clear();
    const HRESULT factoryHr = EnsureFactory();
    if (FAILED(factoryHr))
        return factoryHr;

    wil::com_ptr_nothrow<IEnumString> enumStr;
    const HRESULT enumHr = InvokeNativeMethod(_factory.get(), &ISpellCheckerFactory::get_SupportedLanguages, enumStr.put());
    if (FAILED(enumHr) || ! enumStr)
        return FAILED(enumHr) ? enumHr : E_FAIL;

    for (;;)
    {
        wil::unique_cotaskmem_string tagPtr;
        ULONG fetched{};
        const HRESULT nextHr = InvokeNativeMethod(enumStr.get(), &IEnumString::Next, 1ul, tagPtr.put(), &fetched);
        if (nextHr != S_OK || fetched == 0u || ! tagPtr)
            break;
        std::wstring sanitized = SanitizeSpellUiText(tagPtr.get(), 32u);
        if (! sanitized.empty())
            outTags.push_back(std::move(sanitized));
        if (outTags.size() >= 64u)
            break;
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

ProviderAvailability WinSpellProvider::CheckAvailability(std::wstring_view languageTag, ProviderDiagnostic* outDiag) noexcept
try
{
    ProviderAvailability availability = ProviderAvailability::Unavailable;
    wil::com_ptr_nothrow<ISpellChecker> checker;
    (void)GetOrCreateChecker(languageTag, checker.put(), &availability, outDiag);
    return availability;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WinSpellProvider::CheckSegment(std::wstring_view languageTag,
                                       std::wstring_view segment,
                                       const SpellCheckOptions& options,
                                       std::vector<SpellErrorSpan>& outErrors) noexcept
try
{
    outErrors.clear();
    if (segment.empty())
        return S_OK;

    ProviderAvailability availability = ProviderAvailability::Unavailable;
    wil::com_ptr_nothrow<ISpellChecker> checker;
    const HRESULT checkerHr = GetOrCreateChecker(languageTag, checker.put(), &availability);
    if (FAILED(checkerHr) || ! checker || availability != ProviderAvailability::Ready)
        return SUCCEEDED(checkerHr) ? S_OK : checkerHr;

    // ISpellChecker::Check expects a null-terminated LPWSTR; copy only this bounded <=4096-unit chunk.
    const std::wstring nullTerminated(segment);
    wil::com_ptr_nothrow<IEnumSpellingError> enumErrors;
    const HRESULT checkHr = InvokeNativeMethod(checker.get(), &ISpellChecker::Check, nullTerminated.c_str(), enumErrors.put());
    if (FAILED(checkHr) || ! enumErrors)
        return FAILED(checkHr) ? checkHr : E_FAIL;

    for (;;)
    {
        wil::com_ptr_nothrow<ISpellingError> error;
        const HRESULT nextHr = InvokeNativeMethod(enumErrors.get(), &IEnumSpellingError::Next, error.put());
        if (nextHr != S_OK || ! error)
            break;

        CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
        if (FAILED(InvokeNativeMethod(error.get(), &ISpellingError::get_CorrectiveAction, &action)))
            continue;
        if (action == CORRECTIVE_ACTION_NONE)
            continue;

        ULONG startIndex{};
        ULONG length{};
        if (FAILED(InvokeNativeMethod(error.get(), &ISpellingError::get_StartIndex, &startIndex)) ||
            FAILED(InvokeNativeMethod(error.get(), &ISpellingError::get_Length, &length)))
            continue;

        const SpellErrorSpan span{static_cast<std::size_t>(startIndex), static_cast<std::size_t>(length)};
        if (! IsValidUtf16Span(segment, span.start, span.length))
            continue;
        if (ShouldSkipSpellError(segment, span, options))
            continue;
        outErrors.push_back(span);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WinSpellProvider::SuggestWord(std::wstring_view languageTag,
                                      std::wstring_view word,
                                      std::size_t maxSuggestions,
                                      std::vector<std::wstring>& outSuggestions) noexcept
try
{
    outSuggestions.clear();
    if (maxSuggestions == 0u || word.empty())
        return S_OK;

    ProviderAvailability availability = ProviderAvailability::Unavailable;
    wil::com_ptr_nothrow<ISpellChecker> checker;
    const HRESULT checkerHr = GetOrCreateChecker(languageTag, checker.put(), &availability);
    if (FAILED(checkerHr) || ! checker || availability != ProviderAvailability::Ready)
        return SUCCEEDED(checkerHr) ? S_OK : checkerHr;

    const std::wstring nullTerminated(word);

    // Surface an explicit autocorrect replacement suggestion first if Windows provides one.
    wil::com_ptr_nothrow<IEnumSpellingError> checkEnum;
    if (SUCCEEDED(InvokeNativeMethod(checker.get(), &ISpellChecker::Check, nullTerminated.c_str(), checkEnum.put())) && checkEnum)
    {
        wil::com_ptr_nothrow<ISpellingError> firstError;
        if (InvokeNativeMethod(checkEnum.get(), &IEnumSpellingError::Next, firstError.put()) == S_OK && firstError)
        {
            CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
            if (SUCCEEDED(InvokeNativeMethod(firstError.get(), &ISpellingError::get_CorrectiveAction, &action)) && action == CORRECTIVE_ACTION_REPLACE)
            {
                wil::unique_cotaskmem_string replacement;
                if (SUCCEEDED(InvokeNativeMethod(firstError.get(), &ISpellingError::get_Replacement, replacement.put())) && replacement)
                {
                    std::wstring clean = SanitizeSpellUiText(replacement.get(), 64u);
                    if (! clean.empty() && clean != word)
                        outSuggestions.push_back(std::move(clean));
                }
            }
        }
    }

    if (outSuggestions.size() >= maxSuggestions)
        return S_OK;

    wil::com_ptr_nothrow<IEnumString> enumStr;
    const HRESULT suggestHr = InvokeNativeMethod(checker.get(), &ISpellChecker::Suggest, nullTerminated.c_str(), enumStr.put());
    if (FAILED(suggestHr) || ! enumStr)
        return outSuggestions.empty() ? (FAILED(suggestHr) ? suggestHr : E_FAIL) : S_OK;

    for (;;)
    {
        wil::unique_cotaskmem_string candidatePtr;
        ULONG fetched{};
        const HRESULT nextHr = InvokeNativeMethod(enumStr.get(), &IEnumString::Next, 1ul, candidatePtr.put(), &fetched);
        if (nextHr != S_OK || fetched == 0u || ! candidatePtr)
            break;
        std::wstring clean = SanitizeSpellUiText(candidatePtr.get(), 64u);
        if (! clean.empty() && std::ranges::find(outSuggestions, clean) == outSuggestions.end())
        {
            outSuggestions.push_back(std::move(clean));
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
} // namespace RedTilde
