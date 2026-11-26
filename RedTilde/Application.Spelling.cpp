#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "Common/TextUtilities.h"
#include "res/Resource.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <shobjidl.h>
#include <span>
#include <utility>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
constexpr int SpellSuggestionBaseId           = 10000;
constexpr int SpellMaximumMenuItems           = 5;
constexpr int SpellDeleteRepeatedId           = 10097;
constexpr int SpellIgnoreOnceId               = 10098;
constexpr int SpellNoSuggestionsId            = 10099;
constexpr int SpellIgnoreAllId                = 10100;
constexpr int SpellAddWordId                  = 10101;
constexpr std::size_t ViewportOverscanUnits   = 2048u;
constexpr std::size_t MaximumVisibleSquiggles = 256u;
constexpr std::array<const wchar_t*, 4> CommonFallbackLanguages{L"en-US", L"fr-FR", L"de-DE", L"es-ES"};

[[nodiscard]] bool ComboItemsEqual(std::span<const DxUi::ComboBox::Item> existing, const std::vector<DxUi::ComboBox::Item>& next) noexcept
{
    if (existing.size() != next.size())
        return false;
    for (std::size_t i = 0u; i < existing.size(); ++i)
    {
        if (existing[i].value != next[i].value || existing[i].display != next[i].display)
            return false;
    }
    return true;
}

[[nodiscard]] std::wstring QueryUserDefaultLocaleTag()
{
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH) > 0)
    {
        std::wstring tag = SanitizeSpellUiText(buffer, 32u);
        if (IsValidLanguageTag(tag) && ! EqualOrdinalIgnoreCase(tag, L"auto"))
            return tag;
    }
    return L"en-US";
}

[[nodiscard]] std::wstring FormatFriendlyLanguageName(std::wstring_view tag)
{
    if (EqualOrdinalIgnoreCase(tag, L"auto"))
        return Resource(IDS_SPELL_LANGUAGE_AUTO);
    const std::wstring cleanTag = SanitizeSpellUiText(tag, 32u);
    if (cleanTag.empty())
        return {};
    wchar_t displayName[128]{};
    const std::wstring nullTag(cleanTag);
    if (GetLocaleInfoEx(nullTag.c_str(), LOCALE_SLOCALIZEDDISPLAYNAME, displayName, static_cast<int>(std::size(displayName))) > 0)
    {
        std::wstring cleanName = SanitizeSpellUiText(displayName, 48u);
        if (! cleanName.empty())
            return cleanName;
    }
    return cleanTag;
}

[[nodiscard]] std::wstring FormatMultiLanguageSummary(std::span<const std::wstring> languages)
{
    std::wstring combined;
    for (std::size_t i = 0u; i < languages.size(); ++i)
    {
        if (i > 0u)
            combined += L" / ";
        const auto sep = languages[i].find(L'-');
        combined += SanitizeSpellUiText(std::wstring_view(languages[i]).substr(0u, sep), 8u);
    }
    return SanitizeSpellUiText(combined, 48u);
}

[[nodiscard]] std::wstring FormatSpellAvailabilityText(ProviderAvailability availability, std::wstring_view tag)
{
    switch (availability)
    {
        case ProviderAvailability::NoDictionary: return FormatStringResource(nullptr, IDS_SPELL_NO_DICTIONARY, SanitizeSpellUiText(tag, 20u));
        case ProviderAvailability::DictionaryUnreadable: return FormatStringResource(nullptr, IDS_SPELL_DICT_UNREADABLE, SanitizeSpellUiText(tag, 20u));
        case ProviderAvailability::Unavailable: return Resource(IDS_SPELL_UNAVAILABLE);
        case ProviderAvailability::Ready: break;
    }
    return {};
}

template <typename IssueLike> [[nodiscard]] std::size_t IssueOccurrenceAnchor(const IssueLike& issue) noexcept
{
    return issue.sourceFragments.empty() ? issue.displayRange.first : issue.sourceFragments.front().first;
}

[[nodiscard]] bool IsWordCharForSuppression(wchar_t ch) noexcept
{
    return IsWordJoinerChar(ch) || IsCharAlphaOrDigit(ch);
}

[[nodiscard]] std::optional<DxUi::RichText::MarkdownRange> WordRangeAtOffset(std::wstring_view text, std::size_t offset) noexcept
{
    if (text.empty())
        return std::nullopt;
    const std::size_t clamped = std::min(offset, text.size());
    std::size_t probe         = clamped;
    if (probe == text.size() || ! IsWordCharForSuppression(text[probe]))
    {
        if (probe > 0u && IsWordCharForSuppression(text[probe - 1u]))
            --probe;
        else
            return std::nullopt;
    }
    std::size_t start = probe;
    while (start > 0u && IsWordCharForSuppression(text[start - 1u]))
        --start;
    std::size_t end = probe + 1u;
    while (end < text.size() && IsWordCharForSuppression(text[end]))
        ++end;
    if (start >= end)
        return std::nullopt;
    return DxUi::RichText::MarkdownRange{start, end};
}

[[nodiscard]] bool RangesOverlap(DxUi::RichText::MarkdownRange a, DxUi::RichText::MarkdownRange b) noexcept
{
    return a.first < b.last && b.first < a.last;
}

[[nodiscard]] std::optional<D2D1_RECT_F> FindPaddedIssueRectAtPoint(const DxUi::ControlHost& host,
                                                                    const Editor& editor,
                                                                    const std::vector<DxUi::RichText::MarkdownRange>& fragments,
                                                                    D2D1_POINT_2F ptDip,
                                                                    bool* outHasGeometry = nullptr)
{
    if (outHasGeometry)
        *outHasGeometry = false;
    if (fragments.empty())
        return std::nullopt;
    const auto rects = editor.TryGetTextInputRangeRects(host, fragments.front().first, fragments.back().last);
    if (! rects || rects->empty())
        return std::nullopt;
    if (outHasGeometry)
        *outHasGeometry = true;
    for (const auto& rect : *rects)
    {
        if (ptDip.x >= rect.left - 4.f && ptDip.x <= rect.right + 4.f && ptDip.y >= rect.top - 4.f && ptDip.y <= rect.bottom + 8.f)
            return rect;
    }
    return std::nullopt;
}
} // namespace

bool Application::IsSpellImeComposing() const noexcept
{
#if defined(ENABLE_TESTS)
    if (_spell.imeCompositionOverrideForTest)
        return true;
#endif
    const auto* const focus = _host.GetFocusControl();
    return focus != nullptr && _host.GetNativeTextCompositionActive(focus).value_or(false);
}

std::filesystem::path Application::ActiveCustomDictionaryPath() const
{
    if (! _settings.spelling.customDictionaryPath.empty())
        return _settings.spelling.customDictionaryPath;
    if (! _options.dataDirectory.empty())
        return _options.dataDirectory / L"custom-dictionary.txt";
    return DefaultDataDirectory() / L"custom-dictionary.txt";
}

void Application::InitializeSpelling()
{
    _spellService.Initialize(_window.get(), SpellReady);
#if defined(ENABLE_TESTS)
    if (_options.spellProviderForTest)
        _spellService.SetProviderForTest(_options.spellProviderForTest);
#endif
    const HRESULT dictHr = _customDictionary.Load(ActiveCustomDictionaryPath());
    if (FAILED(dictHr))
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Custom dictionary could not be loaded; read-only fallback disabled");
    _spell.cachedUserLocale = QueryUserDefaultLocaleTag();
    _spell.activeLanguages  = ResolveEffectiveSpellLanguages(std::nullopt);
    RefreshSpellStatusChip();
    RefreshProofingDrawerControls();
}

std::vector<std::wstring> Application::ResolveEffectiveSpellLanguages(const std::optional<std::wstring>& frontMatterHint) const
{
    std::vector<std::wstring> resolved;
    auto pushUnique = [&](std::wstring_view tag)
    {
        std::wstring candidate = SanitizeSpellUiText(tag, 32u);
        if (EqualOrdinalIgnoreCase(candidate, L"auto"))
            candidate = _spell.cachedUserLocale ? *_spell.cachedUserLocale : QueryUserDefaultLocaleTag();
        if (! IsValidLanguageTag(candidate) || EqualOrdinalIgnoreCase(candidate, L"auto"))
            return;
        if (std::ranges::any_of(resolved, [&](const std::wstring& existing) { return EqualOrdinalIgnoreCase(existing, candidate); }))
            return;
        if (resolved.size() < MaximumSpellingLanguages)
            resolved.push_back(std::move(candidate));
    };

    if (_session.spellLanguageOverride && IsValidLanguageTag(*_session.spellLanguageOverride))
        pushUnique(*_session.spellLanguageOverride);
    if (frontMatterHint && IsValidLanguageTag(*frontMatterHint))
        pushUnique(*frontMatterHint);
    for (const auto& tag : _settings.spelling.languages)
        pushUnique(tag);

    if (resolved.empty())
        resolved.push_back(L"en-US");
    return resolved;
}

void Application::ScheduleSpellCheck(bool immediate)
{
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached()
#if defined(ENABLE_TESTS)
        || _options.recoveryProbe
#endif
    )
        return;

    if (_window)
        (void)KillTimer(_window.get(), SpellDebounceTimerId);

    if (! _settings.spelling.enabled || _switchingDocument)
    {
        _spell.checkPending     = false;
        _spell.immediatePending = false;
        _spell.issues.clear();
        _spell.appliedKey.reset();
        ProjectVisibleSpellDecorations();
        RefreshSpellStatusChip();
        return;
    }

    const bool imeComposing = IsSpellImeComposing();
    if (imeComposing)
        _spell.imeSuppressionActive = true;
    if (imeComposing || HasBlockDraft())
    {
        _spell.checkPending     = true;
        _spell.immediatePending = false;
        ProjectVisibleSpellDecorations();
        return;
    }

    if (immediate)
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        _spell.immediatePending = true;
        DispatchSpellCheckNow();
        return;
    }

    if (_window)
    {
        _spell.checkPending     = true;
        _spell.immediatePending = false;
        (void)SetTimer(_window.get(), SpellDebounceTimerId, 300u, nullptr);
    }
    else
    {
        DispatchSpellCheckNow();
    }
}

void Application::OnSpellDebounceTimer()
{
    if (_window)
        (void)KillTimer(_window.get(), SpellDebounceTimerId);
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;
    _spell.typingSuppressionActive = false;
    _spell.suppressedTypingWord.reset();
    _spell.immediatePending = false;
    if (_spell.checkPending || ! _spell.appliedKey)
        DispatchSpellCheckNow();
    else
        ProjectVisibleSpellDecorations();
}

void Application::ReconcileSpellImeExit()
{
    if (IsSpellImeComposing())
    {
        if (! _spell.imeSuppressionActive)
        {
            _spell.imeSuppressionActive = true;
            ProjectVisibleSpellDecorations();
        }
        return;
    }
    if (! _spell.imeSuppressionActive)
        return;
    _spell.imeSuppressionActive = false;
    if (! _settings.spelling.enabled || HasBlockDraft() || IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;
    ProjectVisibleSpellDecorations();
    if (_spell.checkPending || ! _spell.appliedKey)
        ScheduleSpellCheck(false);
}

void Application::DispatchSpellCheckNow()
{
    auto* const target = ActiveEditor();
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached() || ! target || ! _settings.spelling.enabled || _switchingDocument
#if defined(ENABLE_TESTS)
        || _options.recoveryProbe
#endif
    )
    {
        _spell.immediatePending = false;
        return;
    }

    const bool imeComposing = IsSpellImeComposing();
    if (imeComposing)
        _spell.imeSuppressionActive = true;
    if (imeComposing || HasBlockDraft())
    {
        _spell.checkPending     = true;
        _spell.immediatePending = false;
        ProjectVisibleSpellDecorations();
        return;
    }

    const auto snapshot   = target->Snapshot();
    const bool sourceMode = target->GetViewMode() == DxUi::RichText::ViewMode::Source;
    const auto syntax     = target->GetMarkdownSyntax();
    const auto semantic   = sourceMode ? nullptr : target->GetSemanticDocument();

    ExtractedProse extracted;
    std::uint64_t semanticIdentity = 0u;
    if (! sourceMode)
    {
        if (! semantic || ! semantic->Syntax())
        {
            // Semantic preparation still running; ReconcilePostPreparePlacement / Prepare will retrigger.
            _spell.checkPending     = true;
            _spell.immediatePending = target->IsVisible();
            return;
        }
        semanticIdentity                = semantic->Identity();
        const std::wstring_view srcText = semantic->Syntax()->Source();
        extracted                       = ExtractSemanticProse(*semantic, syntax.get(), srcText, _settings.spelling.ignoreInternetAndPaths);
    }
    else
    {
        if (! syntax)
        {
            _spell.checkPending     = true;
            _spell.immediatePending = target->IsVisible();
            return;
        }
        extracted = ExtractSourceProse(*syntax, syntax->Source(), _settings.spelling.ignoreInternetAndPaths);
    }

    const SpellRequestKey requestKey{_session.key, snapshot, semanticIdentity, _customDictionary.Generation(), _spell.languageConfigVersion};
    if (_spell.appliedKey && _spell.appliedKey->Matches(requestKey))
    {
        _spell.checkPending     = false;
        _spell.immediatePending = false;
        ProjectVisibleSpellDecorations();
        return;
    }

    auto effectiveLanguages = ResolveEffectiveSpellLanguages(extracted.frontMatterLanguage);
    _spell.activeLanguages  = effectiveLanguages;
    RefreshSpellStatusChip();

    // Cold-start zero-cost invariant: empty or non-prose documents never spin up Windows COM spellcheckers.
    if (extracted.chunks.empty())
    {
        _spell.checkPending     = false;
        _spell.immediatePending = false;
        _spell.issues.clear();
        _spell.appliedKey            = requestKey;
        _spell.effectiveAvailability = ProviderAvailability::Ready;
        _spell.primaryDiagnostic     = {};
        ProjectVisibleSpellDecorations();
        RefreshSpellStatusChip();
        return;
    }

    SpellCheckRequest request;
    request.key       = requestKey;
    request.prose     = std::move(extracted);
    request.languages = std::move(effectiveLanguages);
    request.options =
        SpellCheckOptions{_settings.spelling.ignoreUppercase, _settings.spelling.ignoreWordsWithDigits, _settings.spelling.ignoreInternetAndPaths};
    request.dictionarySnapshot = _customDictionary.CaptureSnapshot();
    request.semanticMode       = ! sourceMode;

    _spell.checkPending     = false;
    _spell.immediatePending = false;
    (void)_spellService.SubmitCheck(std::move(request));
}

void Application::SpellCompleted() noexcept
{
    RunUiAction([&]
    {
        if (IsClosing())
        {
            FinishClose();
            return;
        }
        ProcessSpellResult();
    });
}

void Application::ProcessSpellResult()
{
    auto* const activeEditor = ActiveEditor();
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached() || ! activeEditor || ! _settings.spelling.enabled)
    {
        (void)_spellService.TakeResult();
        return;
    }

    const auto maybeResult = _spellService.TakeResult();
    if (! maybeResult)
        return;
    const auto& result = *maybeResult;

    const auto currentSnapshot     = activeEditor->Snapshot();
    const bool sourceMode          = activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source;
    const auto semantic            = sourceMode ? nullptr : activeEditor->GetSemanticDocument();
    const std::uint64_t semanticId = semantic ? semantic->Identity() : 0u;
    const SpellRequestKey currentKey{_session.key, currentSnapshot, semanticId, _customDictionary.Generation(), _spell.languageConfigVersion};
    if (! currentKey.Matches(result.key))
        return; // Discard stale completion across rapid edits, view switches, or workspace flips.

    if (! result.installedLanguages.empty())
        _spell.installedLanguages = result.installedLanguages;
    if (! result.resolvedLanguages.empty())
        _spell.activeLanguages = result.resolvedLanguages;

    ProviderAvailability effective = ProviderAvailability::NoDictionary;
    ProviderDiagnostic diag;
    for (std::size_t i = 0u; i < result.availabilities.size(); ++i)
    {
        const ProviderAvailability avail = result.availabilities[i];
        if (avail == ProviderAvailability::Ready)
        {
            effective = ProviderAvailability::Ready;
            diag      = i < result.diagnostics.size() ? result.diagnostics[i] : ProviderDiagnostic{};
            break;
        }
        if (avail == ProviderAvailability::DictionaryUnreadable)
        {
            effective = ProviderAvailability::DictionaryUnreadable;
            diag      = i < result.diagnostics.size() ? result.diagnostics[i] : ProviderDiagnostic{};
        }
        else if (avail == ProviderAvailability::Unavailable && effective != ProviderAvailability::DictionaryUnreadable)
        {
            effective = ProviderAvailability::Unavailable;
            diag      = i < result.diagnostics.size() ? result.diagnostics[i] : ProviderDiagnostic{};
        }
        else if (effective == ProviderAvailability::NoDictionary && i == 0u && i < result.diagnostics.size())
        {
            diag = result.diagnostics[i];
        }
    }
    if (result.availabilities.empty())
        effective = ProviderAvailability::Ready;
    if (FAILED(result.status))
    {
        effective = ProviderAvailability::Unavailable;
        if (diag.message.empty())
            diag.message = SanitizeSpellUiText(ErrorText(result.status), 160u);
    }

    _spell.effectiveAvailability = effective;
    _spell.primaryDiagnostic     = std::move(diag);
    _spell.issues.clear();

    if (effective == ProviderAvailability::Ready && SUCCEEDED(result.status))
    {
        const std::size_t sourceLen = currentSnapshot.Length();
        _spell.issues.reserve(std::min<std::size_t>(result.issues.size(), SpellService::MaximumRawIssues));
        for (const auto& raw : result.issues)
        {
            if (_spell.issues.size() >= SpellService::MaximumRawIssues)
                break;
            SpellIssue issue;
            issue.displayRange  = {raw.offset, raw.offset + raw.length};
            issue.word          = raw.word;
            issue.normalizedKey = raw.normalizedKey;
            issue.kind          = raw.kind;
            if (! sourceMode)
            {
                if (! semantic || ! TryProjectDisplayErrorToSource(*semantic, issue.displayRange, issue.sourceFragments))
                    continue;
            }
            else
            {
                if (issue.displayRange.last > sourceLen || ! currentSnapshot.IsBoundary(issue.displayRange.first) ||
                    ! currentSnapshot.IsBoundary(issue.displayRange.last))
                    continue;
                issue.sourceFragments.push_back(issue.displayRange);
            }
            _spell.issues.push_back(std::move(issue));
        }
    }

    _spell.appliedKey = result.key;
    ProjectVisibleSpellDecorations();
    RefreshSpellStatusChip();
    RefreshProofingDrawerControls();
}

void Application::UpdateSpellSuppressionFromCaret()
{
    auto* const activeEditor = ActiveEditor();
    if (! activeEditor)
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        return;
    }
    const auto sel = activeEditor->GetSelection();
    if (sel.Start() != sel.End())
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        return;
    }
    const auto snapshot        = activeEditor->Snapshot();
    const std::size_t totalLen = snapshot.Length();
    if (totalLen == 0u)
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        return;
    }
    const std::size_t clampedCaret = std::min(sel.caret, totalLen);
    for (std::size_t radius = 128u;; radius *= 2u)
    {
        std::size_t winFirst = clampedCaret > radius ? clampedCaret - radius : 0u;
        std::size_t winLast  = std::min(totalLen, clampedCaret + radius);
        while (winFirst > 0u && ! snapshot.IsBoundary(winFirst))
            --winFirst;
        while (winLast < totalLen && ! snapshot.IsBoundary(winLast))
            ++winLast;

        std::wstring slice;
        if (snapshot.TrySlice(winFirst, winLast, slice) != DxUi::RichText::SourceResult::Success)
        {
            _spell.typingSuppressionActive = false;
            _spell.suppressedTypingWord.reset();
            return;
        }
        const auto localRange = WordRangeAtOffset(slice, clampedCaret - winFirst);
        if (! localRange)
        {
            _spell.typingSuppressionActive = false;
            _spell.suppressedTypingWord.reset();
            return;
        }
        const bool clippedLeft  = winFirst > 0u && localRange->first == 0u;
        const bool clippedRight = winLast < totalLen && localRange->last == slice.size();
        if ((! clippedLeft && ! clippedRight) || (winFirst == 0u && winLast == totalLen) || radius >= 4096u)
        {
            _spell.suppressedTypingWord    = DxUi::RichText::MarkdownRange{winFirst + localRange->first, winFirst + localRange->last};
            _spell.typingSuppressionActive = true;
            return;
        }
    }
}

void Application::OnSpellEditorChanged()
{
    _spell.appliedKey.reset();
    _spell.issues.clear();
    UpdateSpellSuppressionFromCaret();
    ProjectVisibleSpellDecorations();
    ScheduleSpellCheck(false);
}

void Application::OnSpellSelectionChanged()
{
    auto* const activeEditor = ActiveEditor();
    if (! _spell.typingSuppressionActive || ! activeEditor)
        return;
    const auto sel = activeEditor->GetSelection();
    if (sel.Start() != sel.End() || ! _spell.suppressedTypingWord)
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        ProjectVisibleSpellDecorations();
        return;
    }
    const auto& sup = *_spell.suppressedTypingWord;
    if (sel.caret < sup.first || sel.caret > sup.last)
    {
        _spell.typingSuppressionActive = false;
        _spell.suppressedTypingWord.reset();
        ProjectVisibleSpellDecorations();
    }
}

void Application::OnSpellScrollChanged()
{
    if (! _settings.spelling.enabled || _spell.issues.empty() || IsClosing())
        return;
    ProjectVisibleSpellDecorations();
}

void Application::ProjectVisibleSpellDecorations()
{
    auto* const activeEditor = ActiveEditor();
    if (IsClosing() || ! activeEditor)
        return;

    if (auto* const mirrorEditor = MirrorEditor(); mirrorEditor && mirrorEditor != activeEditor)
        (void)mirrorEditor->TrySetDecorations(mirrorEditor->Snapshot(), {});

    const auto expectedSource = activeEditor->Snapshot();
    const bool imeActive      = IsSpellImeComposing();
    if (imeActive)
        _spell.imeSuppressionActive = true;
    if (! _settings.spelling.enabled || _spell.effectiveAvailability != ProviderAvailability::Ready || imeActive || HasBlockDraft() || _spell.issues.empty())
    {
        (void)activeEditor->TrySetDecorations(expectedSource, {});
        return;
    }

    const auto visSource = activeEditor->GetVisibleSourceRange(_host);
    const DxUi::RichText::MarkdownRange directWindow =
        visSource ? DxUi::RichText::MarkdownRange{visSource->first, visSource->second} : DxUi::RichText::MarkdownRange{0u, expectedSource.Length()};
    const DxUi::RichText::MarkdownRange overscanWindow =
        visSource ? DxUi::RichText::MarkdownRange{visSource->first > ViewportOverscanUnits ? visSource->first - ViewportOverscanUnits : 0u,
                                                  std::min(expectedSource.Length(), visSource->second + ViewportOverscanUnits)}
                  : directWindow;

    std::vector<DxUi::RichTextDecoration> decorations;
    decorations.reserve(std::min<std::size_t>(_spell.issues.size() * 2u, MaximumVisibleSquiggles));
    const CustomDictionary::AcceptedLookupScope acceptedScope(_customDictionary);

    const auto appendIssue = [&](const SpellIssue& issue, std::size_t cap) -> bool
    {
        if (IsIssueSuppressedByTyping(issue))
            return true;
        if (acceptedScope.IsAcceptedNormalized(issue.normalizedKey, _session.key, IssueOccurrenceAnchor(issue), issue.kind == SpellIssueKind::RepeatedWord))
            return true;

        for (const auto& frag : issue.sourceFragments)
        {
            if (decorations.size() >= cap)
                return false;
            if (frag.first >= frag.last || frag.last > expectedSource.Length() || ! expectedSource.IsBoundary(frag.first) ||
                ! expectedSource.IsBoundary(frag.last))
                continue;
            decorations.push_back(DxUi::RichTextDecoration{frag.first,
                                                           frag.last - frag.first,
                                                           DxUi::RichTextDecorationKind::Squiggle,
                                                           DxUi::RichTextDecorationRole::Error,
                                                           DxUi::RichTextAnnotationKind::SpellingError});
        }
        return true;
    };

    auto collectDecorations = [&](std::size_t cap)
    {
        decorations.clear();
        // Pass 1: prioritize directly visible viewport issues before overscan bands.
        for (const auto& issue : _spell.issues)
        {
            if (issue.sourceFragments.empty())
                continue;
            const DxUi::RichText::MarkdownRange issueSpan{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
            if (! RangesOverlap(issueSpan, directWindow))
                continue;
            if (! appendIssue(issue, cap))
                break;
        }
        // Pass 2: fill remaining capacity from overscan bands outside the direct viewport.
        if (visSource && decorations.size() < cap)
        {
            for (const auto& issue : _spell.issues)
            {
                if (issue.sourceFragments.empty())
                    continue;
                const DxUi::RichText::MarkdownRange issueSpan{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
                if (RangesOverlap(issueSpan, directWindow) || ! RangesOverlap(issueSpan, overscanWindow))
                    continue;
                if (! appendIssue(issue, cap))
                    break;
            }
        }
        std::ranges::sort(decorations, {}, &DxUi::RichTextDecoration::start);
    };

    for (const std::size_t cap : {MaximumVisibleSquiggles, std::size_t{128u}, std::size_t{64u}})
    {
        collectDecorations(cap);
        const HRESULT hr = activeEditor->TrySetDecorations(expectedSource, decorations);
        if (hr != E_BOUNDS && activeEditor->GetDecorationStatus() != DxUi::RichTextDecorationStatus::CapacityExceeded)
            break;
    }
}

void Application::RefreshSpellStatusChip()
{
    RefreshSpellLanguageCombos(false);
    if (! _spellStatusCombo)
        return;

    std::wstring chipText;
    std::wstring diagDetail;
    if (! _settings.spelling.enabled)
    {
        chipText = Resource(IDS_SPELL_STATUS_OFF);
    }
    else
    {
        const std::wstring primaryTag = _spell.activeLanguages.empty() ? L"en-US" : _spell.activeLanguages.front();
        if (_spell.effectiveAvailability != ProviderAvailability::Ready)
            chipText = FormatSpellAvailabilityText(_spell.effectiveAvailability, primaryTag);
        else if (_spell.activeLanguages.size() > 1u)
            chipText = FormatMultiLanguageSummary(_spell.activeLanguages);
        else
            chipText = SanitizeSpellUiText(FormatFriendlyLanguageName(primaryTag), 48u);
        diagDetail = SanitizeSpellUiText(_spell.primaryDiagnostic.message, 160u);
    }

    if (chipText.size() > 48u)
        chipText = SanitizeSpellUiText(chipText, 48u);

    std::wstring tip = FormatStringResource(nullptr, IDS_SPELL_STATUS_TIP, chipText);
    if (! diagDetail.empty())
    {
        tip += L" — ";
        tip += diagDetail;
    }
    _spellStatusCombo->SetTooltipText(SanitizeSpellUiText(tip, 256u));
}

void Application::RefreshProofingDrawerControls()
{
    if (! _spellingEnabledToggle)
        return;

    _spellingEnabledToggle->SetChecked(_settings.spelling.enabled);
    if (_spellingIgnoreUpperToggle)
    {
        _spellingIgnoreUpperToggle->SetChecked(_settings.spelling.ignoreUppercase);
        _spellingIgnoreUpperToggle->SetEnabled(_settings.spelling.enabled);
    }
    if (_spellingIgnoreDigitsToggle)
    {
        _spellingIgnoreDigitsToggle->SetChecked(_settings.spelling.ignoreWordsWithDigits);
        _spellingIgnoreDigitsToggle->SetEnabled(_settings.spelling.enabled);
    }
    if (_spellingIgnorePathsToggle)
    {
        _spellingIgnorePathsToggle->SetChecked(_settings.spelling.ignoreInternetAndPaths);
        _spellingIgnorePathsToggle->SetEnabled(_settings.spelling.enabled);
    }
    if (_spellingLanguageCombo)
        _spellingLanguageCombo->SetEnabled(_settings.spelling.enabled);
    if (_spellingDictPathLabel)
    {
        const std::wstring fileName = _settings.spelling.customDictionaryPath.empty()
                                          ? Resource(IDS_SPELL_DICT_DEFAULT_NAME)
                                          : SanitizeSpellUiText(_settings.spelling.customDictionaryPath.filename().native(), 36u);
        const std::wstring summary  = FormatStringResource(nullptr, IDS_SPELL_DICT_PATH_LABEL, fileName, _customDictionary.WordCount());
        _spellingDictPathLabel->SetText(summary);
    }
    if (_spellingDictResetButton)
        _spellingDictResetButton->SetEnabled(! _settings.spelling.customDictionaryPath.empty());
}

void Application::RefreshSpellLanguageCombos(bool queryInstalled)
{
    if (! _spellingLanguageCombo && ! _spellStatusCombo)
        return;

    if (queryInstalled && _spell.installedLanguages.empty())
    {
        std::vector<std::wstring> enumerated;
        if (SUCCEEDED(_spellService.EnumerateInstalledLanguagesSyncBounded(enumerated, 500u)) && ! enumerated.empty())
            _spell.installedLanguages = std::move(enumerated);
    }

    std::vector<std::wstring> candidateTags;
    auto addCandidate = [&](std::wstring_view tag)
    {
        std::wstring clean = SanitizeSpellUiText(tag, 32u);
        if (clean.empty() || EqualOrdinalIgnoreCase(clean, L"auto") || EqualOrdinalIgnoreCase(clean, L"off"))
            return;
        if (std::ranges::any_of(candidateTags, [&](const std::wstring& e) { return EqualOrdinalIgnoreCase(e, clean); }))
            return;
        candidateTags.push_back(std::move(clean));
    };

    if (! _spell.installedLanguages.empty())
    {
        for (const auto& tag : _spell.installedLanguages)
            addCandidate(tag);
    }
    else
    {
        if (_spell.cachedUserLocale)
            addCandidate(*_spell.cachedUserLocale);
        for (const wchar_t* common : CommonFallbackLanguages)
            addCandidate(common);
    }

    const std::wstring configuredTag = _settings.spelling.languages.empty() ? L"auto" : _settings.spelling.languages.front();
    const std::wstring primaryTag    = _spell.activeLanguages.empty() ? L"en-US" : _spell.activeLanguages.front();
    addCandidate(configuredTag);
    addCandidate(primaryTag);

    std::vector<DxUi::ComboBox::Item> baseItems;
    baseItems.reserve(candidateTags.size() + 2u);
    baseItems.push_back(DxUi::ComboBox::Item{L"auto", Resource(IDS_SPELL_LANGUAGE_AUTO)});

    std::vector<DxUi::ComboBox::Item> localeItems;
    localeItems.reserve(candidateTags.size());
    for (const auto& tag : candidateTags)
        localeItems.push_back(DxUi::ComboBox::Item{tag, FormatFriendlyLanguageName(tag)});
    std::ranges::sort(localeItems, [](const DxUi::ComboBox::Item& a, const DxUi::ComboBox::Item& b) { return LessOrdinalIgnoreCase(a.display, b.display); });
    for (auto& item : localeItems)
        baseItems.push_back(std::move(item));

    auto findIndexByValue = [](const std::vector<DxUi::ComboBox::Item>& list, std::wstring_view value) -> std::optional<std::size_t>
    {
        for (std::size_t i = 0u; i < list.size(); ++i)
        {
            if (EqualOrdinalIgnoreCase(list[i].value, value))
                return i;
        }
        return std::nullopt;
    };

    const bool prevSync    = std::exchange(_spell.syncingLanguageCombos, true);
    const auto restoreSync = wil::scope_exit([&]() noexcept { _spell.syncingLanguageCombos = prevSync; });

    if (_spellingLanguageCombo)
    {
        auto drawerItems = baseItems;
        auto selIdx      = findIndexByValue(drawerItems, configuredTag);
        if (! selIdx)
            selIdx = 0u;
        if (_settings.spelling.enabled && _spell.effectiveAvailability != ProviderAvailability::Ready &&
            (_spell.effectiveAvailability == ProviderAvailability::Unavailable || ! EqualOrdinalIgnoreCase(configuredTag, L"auto")))
        {
            drawerItems[*selIdx].display = FormatSpellAvailabilityText(_spell.effectiveAvailability, configuredTag);
        }
        if (! ComboItemsEqual(_spellingLanguageCombo->GetItems(), drawerItems))
            _spellingLanguageCombo->SetItems(std::move(drawerItems));
        if (_spellingLanguageCombo->GetSelectedIndex() != selIdx)
            _spellingLanguageCombo->SetSelectedIndex(selIdx);
    }

    if (_spellStatusCombo)
    {
        auto statusItems = baseItems;
        statusItems.push_back(DxUi::ComboBox::Item{L"off", Resource(IDS_SPELL_STATUS_OFF)});
        std::optional<std::size_t> selIdx;
        if (! _settings.spelling.enabled)
        {
            selIdx = statusItems.size() - 1u;
        }
        else if (_spell.effectiveAvailability != ProviderAvailability::Ready)
        {
            selIdx = findIndexByValue(statusItems, primaryTag);
            if (! selIdx)
            {
                statusItems.insert(statusItems.end() - 1, DxUi::ComboBox::Item{primaryTag, {}});
                selIdx = statusItems.size() - 2u;
            }
            statusItems[*selIdx].display = FormatSpellAvailabilityText(_spell.effectiveAvailability, primaryTag);
        }
        else if (_spell.activeLanguages.size() > 1u)
        {
            selIdx = findIndexByValue(statusItems, primaryTag);
            if (! selIdx)
                selIdx = 0u;
            statusItems[*selIdx].display = FormatMultiLanguageSummary(_spell.activeLanguages);
        }
        else
        {
            selIdx = findIndexByValue(statusItems,
                                      EqualOrdinalIgnoreCase(configuredTag, L"auto") ? std::wstring_view(primaryTag) : std::wstring_view(configuredTag));
            if (! selIdx)
                selIdx = 0u;
        }
        bool statusChanged = false;
        if (! ComboItemsEqual(_spellStatusCombo->GetItems(), statusItems))
        {
            _spellStatusCombo->SetItems(std::move(statusItems));
            statusChanged = true;
        }
        if (_spellStatusCombo->GetSelectedIndex() != selIdx)
        {
            _spellStatusCombo->SetSelectedIndex(selIdx);
            statusChanged = true;
        }
        if (statusChanged)
            _spellStatusCombo->InvalidateTextLayout();
        else
            _spellStatusCombo->InvalidateTextLayoutIfDisplayedChanged();
    }
}

void Application::ApplySpellLanguageSelection(std::wstring_view value, bool fromStatusBar)
{
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;

    if (fromStatusBar && EqualOrdinalIgnoreCase(value, L"off"))
    {
        if (_settings.spelling.enabled)
        {
            _settings.spelling.enabled = false;
            RefreshProofingDrawerControls();
            RefreshSpellStatusChip();
            ScheduleSpellCheck(true);
            SavePreferences();
        }
        if (_window && ActiveEditor())
            _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
        return;
    }

    const std::wstring nextTag = EqualOrdinalIgnoreCase(value, L"auto") ? L"auto" : SanitizeSpellUiText(value, 32u);
    if (nextTag.empty())
        return;

    const bool wasEnabled        = _settings.spelling.enabled;
    const bool sameConfiguredTag = _settings.spelling.languages.size() == 1u && EqualOrdinalIgnoreCase(_settings.spelling.languages.front(), nextTag) &&
                                   ! _session.spellLanguageOverride.has_value();
    _settings.spelling.enabled   = true;
    _settings.spelling.languages = {nextTag};
    _session.spellLanguageOverride.reset();
    if (! wasEnabled || ! sameConfiguredTag)
        ++_spell.languageConfigVersion;
    _spell.activeLanguages = ResolveEffectiveSpellLanguages(std::nullopt);
    RefreshProofingDrawerControls();
    RefreshSpellStatusChip();
    ScheduleSpellCheck(true);
    SavePreferences();
    if (fromStatusBar && _window && ActiveEditor())
        _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
}

bool Application::CanRunSpellPointerInteraction(bool checkComboPopups)
{
    auto* const activeEditor = ActiveEditor();
    if (! _settings.spelling.enabled || ! activeEditor || _spell.issues.empty() || _spell.effectiveAvailability != ProviderAvailability::Ready ||
        HasActiveModalUi() || _activeOverlay == OverlayKind::BlockDraftModal || _printPreview != nullptr)
        return false;
    if (checkComboPopups && ((_spellStatusCombo && _spellStatusCombo->IsPopupOpen()) || (_spellingLanguageCombo && _spellingLanguageCombo->IsPopupOpen())))
        return false;
    if (IsSpellImeComposing())
    {
        _spell.imeSuppressionActive = true;
        return false;
    }
    return true;
}

bool Application::IsIssueSuppressedByTyping(const SpellIssue& issue) const noexcept
{
    if (! _spell.typingSuppressionActive || ! _spell.suppressedTypingWord || issue.sourceFragments.empty())
        return false;
    const DxUi::RichText::MarkdownRange issueSpan{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
    return RangesOverlap(issueSpan, *_spell.suppressedTypingWord);
}

std::optional<std::size_t> Application::TryHitSpellIssueAtPoint(Editor& editor,
                                                                D2D1_POINT_2F ptDip,
                                                                std::optional<std::size_t> hitOffset,
                                                                std::optional<D2D1_RECT_F>* outRect) const
{
    if (outRect)
        outRect->reset();
    if (hitOffset)
    {
        if (const auto directIdx = FindSpellIssueAtSourceOffset(*hitOffset))
        {
            const auto& issue = _spell.issues[*directIdx];
            bool hasGeometry{};
            const auto hitRect = FindPaddedIssueRectAtPoint(_host, editor, issue.sourceFragments, ptDip, &hasGeometry);
            if (hasGeometry)
            {
                if (! hitRect)
                    return std::nullopt;
                if (outRect)
                    *outRect = hitRect;
            }
            return directIdx;
        }
    }

    std::size_t scanFirst = 0u;
    std::size_t scanLast  = 0u;
    if (hitOffset)
    {
        scanFirst = *hitOffset > 128u ? *hitOffset - 128u : 0u;
        scanLast  = *hitOffset + 128u;
    }
    else
    {
        const auto visSource = editor.GetVisibleSourceRange(_host);
        scanFirst            = visSource ? visSource->first : 0u;
        scanLast             = visSource ? visSource->second : editor.Snapshot().Length();
    }

    const CustomDictionary::AcceptedLookupScope acceptedScope(_customDictionary);
    for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
    {
        const auto& issue = _spell.issues[i];
        if (issue.sourceFragments.empty())
            continue;
        const DxUi::RichText::MarkdownRange span{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
        if (span.last < scanFirst || span.first > scanLast)
            continue;
        if (acceptedScope.IsAcceptedNormalized(issue.normalizedKey, _session.key, IssueOccurrenceAnchor(issue), issue.kind == SpellIssueKind::RepeatedWord))
            continue;
        if (const auto hitRect = FindPaddedIssueRectAtPoint(_host, editor, issue.sourceFragments, ptDip))
        {
            if (outRect)
                *outRect = hitRect;
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Application::FindSpellIssueAtSourceOffset(std::size_t sourceOffset) const noexcept
{
    const CustomDictionary::AcceptedLookupScope acceptedScope(_customDictionary);
    for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
    {
        const auto& issue = _spell.issues[i];
        if (acceptedScope.IsAcceptedNormalized(issue.normalizedKey, _session.key, IssueOccurrenceAnchor(issue), issue.kind == SpellIssueKind::RepeatedWord))
            continue;
        for (const auto& frag : issue.sourceFragments)
        {
            if (sourceOffset >= frag.first && sourceOffset <= frag.last)
                return i;
        }
    }
    return std::nullopt;
}

std::vector<DxUi::MenuFlyoutItem> Application::BuildSpellActionMenuItems(std::size_t issueIndex, DxUi::RichText::Selection selection)
{
    _spell.pendingMenu.reset();
    auto* const activeEditor = ActiveEditor();
    if (issueIndex >= _spell.issues.size() || ! activeEditor)
        return {};

    const auto& issue    = _spell.issues[issueIndex];
    const auto languages = _spell.activeLanguages;
    std::vector<std::wstring> suggestions;
    if (issue.kind != SpellIssueKind::RepeatedWord)
    {
        std::vector<std::wstring> rawSuggestions;
        (void)_spellService.SuggestSyncBounded(languages, issue.word, _customDictionary.Generation(), SpellMaximumMenuItems, rawSuggestions, 500u);
        suggestions.reserve(rawSuggestions.size());
        for (const auto& candidate : rawSuggestions)
        {
            std::wstring clean = SanitizeSpellUiText(candidate, 64u);
            if (clean.empty() || clean == issue.word || std::ranges::find(suggestions, clean) != suggestions.end())
                continue;
            suggestions.push_back(std::move(clean));
            if (suggestions.size() >= static_cast<std::size_t>(SpellMaximumMenuItems))
                break;
        }
    }

    PendingSpellMenu menuState;
    menuState.workspaceKey = _session.key;
    menuState.editor       = activeEditor;
    menuState.source       = activeEditor->Snapshot();
    menuState.revision     = activeEditor->Revision();
    menuState.selection    = selection;
    menuState.issue        = issue;
    menuState.suggestions  = suggestions;
    menuState.lifetime     = GetControlLifetimeToken(*activeEditor);
    _spell.pendingMenu     = std::move(menuState);

    std::vector<DxUi::MenuFlyoutItem> items;
    if (issue.kind == SpellIssueKind::RepeatedWord)
    {
        DxUi::MenuFlyoutItem delItem;
        delItem.text      = Resource(IDS_SPELL_DELETE_REPEATED);
        delItem.commandId = SpellDeleteRepeatedId;
        items.push_back(std::move(delItem));

        DxUi::MenuFlyoutItem sep1;
        sep1.kind = DxUi::MenuItemKind::Separator;
        items.push_back(std::move(sep1));

        DxUi::MenuFlyoutItem ignoreOnce;
        ignoreOnce.text      = Resource(IDS_SPELL_IGNORE_ONCE);
        ignoreOnce.commandId = SpellIgnoreOnceId;
        items.push_back(std::move(ignoreOnce));
        return items;
    }

    if (suggestions.empty())
    {
        DxUi::MenuFlyoutItem none;
        none.text      = Resource(IDS_SPELL_NO_SUGGESTIONS);
        none.commandId = SpellNoSuggestionsId;
        none.enabled   = false;
        items.push_back(std::move(none));
    }
    else
    {
        for (std::size_t i = 0u; i < suggestions.size() && i < static_cast<std::size_t>(SpellMaximumMenuItems); ++i)
        {
            DxUi::MenuFlyoutItem sug;
            sug.text      = suggestions[i];
            sug.commandId = SpellSuggestionBaseId + static_cast<int>(i);
            items.push_back(std::move(sug));
        }
    }

    DxUi::MenuFlyoutItem sep1;
    sep1.kind = DxUi::MenuItemKind::Separator;
    items.push_back(std::move(sep1));

    DxUi::MenuFlyoutItem ignoreOnce;
    ignoreOnce.text      = Resource(IDS_SPELL_IGNORE_ONCE);
    ignoreOnce.commandId = SpellIgnoreOnceId;
    items.push_back(std::move(ignoreOnce));

    DxUi::MenuFlyoutItem ignoreAll;
    ignoreAll.text      = Resource(IDS_SPELL_IGNORE_ALL);
    ignoreAll.commandId = SpellIgnoreAllId;
    items.push_back(std::move(ignoreAll));

    DxUi::MenuFlyoutItem addWord;
    addWord.text      = Resource(IDS_SPELL_ADD_TO_DICT);
    addWord.commandId = SpellAddWordId;
    items.push_back(std::move(addWord));

    return items;
}

void Application::PopulateSpellContextMenu(POINT screenPoint, bool keyboardInvocation, std::vector<DxUi::MenuFlyoutItem>& items)
{
    _spell.pendingMenu.reset();
    if (HasActiveModalUi())
        return;
    auto* const activeEditor = ActiveEditor();
    if (! _settings.spelling.enabled || ! activeEditor || _spell.issues.empty())
        return;

    if (IsSpellImeComposing())
    {
        _spell.imeSuppressionActive = true;
        return;
    }
    if (HasBlockDraft())
        return;

    std::optional<std::size_t> hitOffset;
    if (! keyboardInvocation && _window)
    {
        POINT clientPt = screenPoint;
        if (ScreenToClient(_window.get(), &clientPt))
        {
            const float scale       = 96.f / _host.GetDpi();
            const D2D1_POINT_2F dip = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
            hitOffset               = activeEditor->TryHitTestTextInputPoint(_host, dip);
        }
    }
    const auto selection = activeEditor->GetSelection();
    if (! hitOffset)
        hitOffset = selection.caret;

    const auto issueIdx = FindSpellIssueAtSourceOffset(*hitOffset);
    if (! issueIdx)
        return;

    auto prefix = BuildSpellActionMenuItems(*issueIdx, selection);
    if (prefix.empty())
        return;

    if (! items.empty())
    {
        DxUi::MenuFlyoutItem sep2;
        sep2.kind = DxUi::MenuItemKind::Separator;
        prefix.push_back(std::move(sep2));
    }

    items.insert(items.begin(), std::make_move_iterator(prefix.begin()), std::make_move_iterator(prefix.end()));
}

bool Application::HandleSpellMenuSelection(
    int selectedId, unsigned workspaceKey, const DxUi::RichText::SourceSnapshot& source, std::uint64_t revision, DxUi::RichText::Selection selection)
{
    const bool isSpellId = (selectedId >= SpellSuggestionBaseId && selectedId < SpellSuggestionBaseId + SpellMaximumMenuItems) ||
                           selectedId == SpellDeleteRepeatedId || selectedId == SpellIgnoreOnceId || selectedId == SpellIgnoreAllId ||
                           selectedId == SpellAddWordId || selectedId == SpellNoSuggestionsId;
    if (! isSpellId)
        return false;

    const auto pending       = std::exchange(_spell.pendingMenu, std::nullopt);
    auto* const activeEditor = ActiveEditor();
    if (! pending || ! IsUiAttached() || pending->lifetime.expired() || ! activeEditor || activeEditor != pending->editor || HasBlockDraft())
        return true;

    if (pending->workspaceKey != workspaceKey || _session.key != workspaceKey || ! activeEditor->Snapshot().IsSameRevision(pending->source) ||
        ! activeEditor->Snapshot().IsSameRevision(source) || activeEditor->Revision() != pending->revision || activeEditor->Revision() != revision ||
        activeEditor->GetSelection() != pending->selection || activeEditor->GetSelection() != selection)
    {
        Status(IDS_LINKCHANGED);
        return true;
    }

    if (IsSpellImeComposing())
    {
        _spell.imeSuppressionActive = true;
        return true;
    }

    if (selectedId == SpellDeleteRepeatedId)
    {
        if (pending->issue.kind != SpellIssueKind::RepeatedWord || pending->issue.sourceFragments.empty())
            return true;
        auto deleteFragments    = pending->issue.sourceFragments;
        const std::size_t first = deleteFragments.front().first;
        const std::size_t last  = deleteFragments.back().last;
        const std::size_t len   = source.Length();
        bool expanded           = false;
        if (first > 0u && source.IsBoundary(first - 1u))
        {
            std::wstring adj;
            if (source.TrySlice(first - 1u, first, adj) == DxUi::RichText::SourceResult::Success && (adj == L" " || adj == L"\t"))
            {
                deleteFragments.front().first = first - 1u;
                expanded                      = true;
            }
        }
        if (! expanded && last < len && source.IsBoundary(last + 1u))
        {
            std::wstring adj;
            if (source.TrySlice(last, last + 1u, adj) == DxUi::RichText::SourceResult::Success && (adj == L" " || adj == L"\t"))
                deleteFragments.back().last = last + 1u;
        }
        std::vector<DxUi::RichText::SourceEdit> edits;
        if (! TryBuildSpellReplacementEdits(source, deleteFragments, L"", edits))
        {
            Status(IDS_NOTAVAILABLE);
            return true;
        }
        const std::size_t caretAfter = edits.front().source.first;
        if (! activeEditor->ApplySourceEdits(source, revision, edits, {caretAfter, caretAfter}))
            Status(IDS_NOTAVAILABLE);
        return true;
    }

    if (selectedId == SpellIgnoreOnceId)
    {
        _customDictionary.IgnoreOnce(_session.key, pending->issue.word, IssueOccurrenceAnchor(pending->issue));
        ProjectVisibleSpellDecorations();
        return true;
    }

    if (selectedId == SpellIgnoreAllId)
    {
        _customDictionary.IgnoreAll(pending->issue.word);
        ProjectVisibleSpellDecorations();
        ScheduleSpellCheck(true);
        return true;
    }

    if (selectedId == SpellAddWordId)
    {
        HRESULT diskHr = S_OK;
        if (SUCCEEDED(_customDictionary.AddWord(pending->issue.word, &diskHr)))
        {
            ProjectVisibleSpellDecorations();
            RefreshProofingDrawerControls();
            if (FAILED(diskHr))
                Status(IDS_SPELL_DICT_SAVE_FAILED);
            else
                _feedback->SetText(FormatStringResource(nullptr, IDS_SPELL_ADDED_TO_DICT, SanitizeSpellUiText(pending->issue.word, 32u)));
            ScheduleSpellCheck(true);
        }
        return true;
    }

    const int sugIndex = selectedId - SpellSuggestionBaseId;
    if (sugIndex >= 0 && static_cast<std::size_t>(sugIndex) < pending->suggestions.size())
    {
        const auto& replacement = pending->suggestions[static_cast<std::size_t>(sugIndex)];
        std::vector<DxUi::RichText::SourceEdit> edits;
        if (! TryBuildSpellReplacementEdits(source, pending->issue.sourceFragments, replacement, edits))
        {
            Status(IDS_NOTAVAILABLE);
            return true;
        }
        const std::size_t caretAfter = edits.front().source.first + replacement.size();
        if (! activeEditor->ApplySourceEdits(source, revision, edits, {caretAfter, caretAfter}))
            Status(IDS_NOTAVAILABLE);
    }
    return true;
}

std::filesystem::path Application::PickDictionaryFile()
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return {};
    _modalPrompt             = ModalPromptKind::FileDialog;
    const auto restorePrompt = wil::scope_exit([&]() noexcept { _modalPrompt = ModalPromptKind::None; });
#if defined(ENABLE_TESTS)
    if (_options.headless && _options.filePickerForTest)
        return _options.filePickerForTest(false);
#endif
    if (_options.headless)
        return {};

    wil::com_ptr<IFileDialog> dialog;
    if (FAILED(DxUi::Failure::InvokeNative(
            &CoCreateInstance, CLSID_FileOpenDialog, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER), IID_PPV_ARGS(dialog.put()))))
        return {};

    const auto filterLabel = Resource(IDS_SPELL_FILTER_CUSTOM_DIC);
    const auto allLabel    = Resource(IDS_FILTERALL);
    const COMDLG_FILTERSPEC filters[]{{filterLabel.c_str(), L"*.dic;*.txt"}, {allLabel.c_str(), L"*.*"}};
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileTypes, static_cast<UINT>(std::size(filters)), filters);
    DWORD flags{};
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::GetOptions, &flags);
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetOptions, flags | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR | FOS_FILEMUSTEXIST);
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::Show, _window.get())))
        return {};
    wil::com_ptr<IShellItem> item;
    wil::unique_cotaskmem_string path;
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::GetResult, item.put())) ||
        FAILED(InvokeNativeMethod(item.get(), &IShellItem::GetDisplayName, SIGDN_FILESYSPATH, path.put())))
        return {};
    return std::filesystem::path(path.get());
}

void Application::ExecuteSpellingCommand(Command command)
{
    if (command == Command::ToggleSpelling)
    {
        _settings.spelling.enabled = ! _settings.spelling.enabled;
        RefreshProofingDrawerControls();
        RefreshSpellStatusChip();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellIgnoreUppercase)
    {
        _settings.spelling.ignoreUppercase = ! _settings.spelling.ignoreUppercase;
        ++_spell.languageConfigVersion;
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellIgnoreDigits)
    {
        _settings.spelling.ignoreWordsWithDigits = ! _settings.spelling.ignoreWordsWithDigits;
        ++_spell.languageConfigVersion;
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellIgnoreInternetPaths)
    {
        _settings.spelling.ignoreInternetAndPaths = ! _settings.spelling.ignoreInternetAndPaths;
        ++_spell.languageConfigVersion;
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellCycleLanguage)
    {
        std::vector<std::wstring> options{L"auto"};
        for (const auto& tag : _spell.installedLanguages)
            if (std::ranges::none_of(options, [&](const std::wstring& e) { return EqualOrdinalIgnoreCase(e, tag); }))
                options.push_back(tag);
        if (options.size() == 1u)
        {
            for (const wchar_t* common : CommonFallbackLanguages)
                if (std::ranges::none_of(options, [&](const std::wstring& e) { return EqualOrdinalIgnoreCase(e, common); }))
                    options.push_back(common);
        }
        const std::wstring current = _settings.spelling.languages.empty() ? L"auto" : _settings.spelling.languages.front();
        std::size_t nextIdx        = 0u;
        for (std::size_t i = 0u; i < options.size(); ++i)
        {
            if (EqualOrdinalIgnoreCase(options[i], current))
            {
                nextIdx = (i + 1u) % options.size();
                break;
            }
        }
        _settings.spelling.languages = {options[nextIdx]};
        ++_spell.languageConfigVersion;
        _spell.activeLanguages = ResolveEffectiveSpellLanguages(std::nullopt);
        RefreshProofingDrawerControls();
        RefreshSpellStatusChip();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellChooseCustomDictionary)
    {
        const auto chosen = PickDictionaryFile();
        if (chosen.empty() || ! IsUiAttached())
            return;
        const auto normalized = chosen.lexically_normal();
        const HRESULT loadHr  = _customDictionary.Load(normalized);
        if (FAILED(loadHr))
        {
            Status(loadHr == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) ? IDS_FILENOTUTF8 : IDS_SPELL_DICT_LOAD_FAILED);
            return;
        }
        _settings.spelling.customDictionaryPath = normalized;
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellResetCustomDictionary)
    {
        _settings.spelling.customDictionaryPath.clear();
        if (FAILED(_customDictionary.Load(ActiveCustomDictionaryPath())))
            Status(IDS_SPELL_DICT_LOAD_FAILED);
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::OpenProofingSettings)
    {
        if (_activeOverlay != OverlayKind::SettingsDrawer)
            SetActiveOverlay(OverlayKind::SettingsDrawer);
        Layout();
        if (_settingsScroll)
            _settingsScroll->SetScrollOffset(_settingsProofingScrollY);
        if (_spellingEnabledToggle)
            _host.SetFocusControl(_spellingEnabledToggle, MayRequestNativeFocus());
    }
}

void Application::OnEditorLeftButtonDown(POINT clientPt, WPARAM modifiers)
{
    _spell.leftDownArmed = false;
    _spell.leftDownHitOffset.reset();
    _spell.leftDownIssueRect.reset();
    _spell.pendingLeftClick.reset();
    if (! CanRunSpellPointerInteraction(true) || (modifiers & (MK_SHIFT | MK_CONTROL)) != 0u)
        return;

    auto* const activeEditor = ActiveEditor();
    if (! activeEditor)
        return;

    const float scale         = 96.f / _host.GetDpi();
    const D2D1_POINT_2F ptDip = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
    const auto edBounds       = activeEditor->GetBounds();
    if (ptDip.x < edBounds.left || ptDip.x >= edBounds.right - 16.f || ptDip.y < edBounds.top || ptDip.y >= edBounds.bottom)
        return;

    const auto ptOverlapsControl = [&](const DxUi::Control* control) noexcept
    {
        if (! control || ! control->IsVisible())
            return false;
        const auto r = control->GetBounds();
        return ptDip.x >= r.left && ptDip.x < r.right && ptDip.y >= r.top && ptDip.y < r.bottom;
    };
    if (ptOverlapsControl(_drawer) || ptOverlapsControl(_findBar) || ptOverlapsControl(_floating) || ptOverlapsControl(_imageConsent) ||
        ptOverlapsControl(_filesPane) || ptOverlapsControl(_sidebar))
        return;

    const auto downHit = activeEditor->TryHitTestTextInputPoint(_host, ptDip);
    std::optional<D2D1_RECT_F> hitRect;
    const auto issueIdx = TryHitSpellIssueAtPoint(*activeEditor, ptDip, downHit, &hitRect);
    if (! issueIdx)
        return;

    _spell.leftDownHitOffset = downHit;
    _spell.leftDownIssueRect = hitRect;
    _spell.leftDownArmed     = true;
    _spell.leftDownClientPt  = clientPt;
    _spell.leftDownRevision  = activeEditor->Revision();
}

void Application::OnEditorLeftButtonDoubleClick() noexcept
{
    _spell.leftDownArmed = false;
    _spell.leftDownHitOffset.reset();
    _spell.leftDownIssueRect.reset();
    _spell.pendingLeftClick.reset();
}

void Application::OnEditorLeftButtonUp(POINT clientPt)
{
    const auto downHitOffset = std::exchange(_spell.leftDownHitOffset, std::nullopt);
    const auto downIssueRect = std::exchange(_spell.leftDownIssueRect, std::nullopt);
    if (! std::exchange(_spell.leftDownArmed, false))
        return;

    auto* const activeEditor = ActiveEditor();
    if (! CanRunSpellPointerInteraction(false) || ! activeEditor || activeEditor->Revision() != _spell.leftDownRevision)
        return;

    const int maxDx = std::max(4, GetSystemMetrics(SM_CXDRAG));
    const int maxDy = std::max(4, GetSystemMetrics(SM_CYDRAG));
    if (std::abs(clientPt.x - _spell.leftDownClientPt.x) > maxDx || std::abs(clientPt.y - _spell.leftDownClientPt.y) > maxDy)
        return;

    const auto selection = activeEditor->GetSelection();
    if (selection.Start() != selection.End())
        return;

    const float scale                    = 96.f / _host.GetDpi();
    const D2D1_POINT_2F ptDip            = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
    std::optional<std::size_t> hitOffset = activeEditor->TryHitTestTextInputPoint(_host, ptDip);
    const bool usedGeometryHit           = hitOffset.has_value();
    if (! hitOffset)
        hitOffset = downHitOffset;
    if (! hitOffset)
        hitOffset = selection.caret;

    std::optional<D2D1_RECT_F> anchorRect;
    auto issueIdx = TryHitSpellIssueAtPoint(*activeEditor, ptDip, hitOffset, &anchorRect);
    if (! issueIdx && downIssueRect)
        issueIdx = FindSpellIssueAtSourceOffset(selection.caret);
    if (! issueIdx)
        return;

    const auto& issue = _spell.issues[*issueIdx];
    if (IsIssueSuppressedByTyping(issue))
        return;

    if (! anchorRect && ! issue.sourceFragments.empty())
    {
        bool hasGeometry{};
        anchorRect = FindPaddedIssueRectAtPoint(_host, *activeEditor, issue.sourceFragments, ptDip, &hasGeometry);
        if (hasGeometry)
        {
            if (! anchorRect)
                return;
        }
        else if (downIssueRect)
        {
            anchorRect = downIssueRect;
        }
        else if (! usedGeometryHit && ! downHitOffset)
        {
            return;
        }
    }

    const D2D1_POINT_2F anchorDip = anchorRect ? D2D1::Point2F(anchorRect->left, anchorRect->bottom + 4.f) : D2D1::Point2F(ptDip.x, ptDip.y + 4.f);
    POINT screenPt                = _host.DipPointToScreenPoint(anchorDip);
    if (screenPt.x == 0 && screenPt.y == 0 && _window)
    {
        screenPt = clientPt;
        (void)ClientToScreen(_window.get(), &screenPt);
    }

    _spell.pendingLeftClick = PendingLeftClickRequest{
        _session.key, activeEditor->Revision(), *issueIdx, screenPt, _spell.appliedKey, _spell.languageConfigVersion, _customDictionary.Generation()};
    if (_window)
        (void)PostMessageW(_window.get(), SpellMenuRequested, 0, 0);
}

void Application::HandlePostedSpellLeftClickMenu()
{
    const auto req = std::exchange(_spell.pendingLeftClick, std::nullopt);
    if (! req || IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;

    if (! CanRunSpellPointerInteraction(true))
        return;

    auto* const activeEditor = ActiveEditor();
    if (! activeEditor || req->workspaceKey != _session.key || req->revision != activeEditor->Revision() || req->issueIndex >= _spell.issues.size())
        return;

    if (req->languageConfigVersion != _spell.languageConfigVersion || req->dictionaryGeneration != _customDictionary.Generation())
        return;
    if (req->appliedKey.has_value() != _spell.appliedKey.has_value() || (req->appliedKey && ! req->appliedKey->Matches(*_spell.appliedKey)))
        return;

    const auto selection = activeEditor->GetSelection();
    if (selection.Start() != selection.End())
        return;

    if (IsIssueSuppressedByTyping(_spell.issues[req->issueIndex]))
        return;

    ShowSpellLeftClickMenu(req->issueIndex, req->screenPt);
}

void Application::ShowSpellLeftClickMenu(std::size_t issueIndex, POINT screenPoint)
{
    auto* const activeEditor = ActiveEditor();
    if (! activeEditor)
        return;

    auto items = BuildSpellActionMenuItems(issueIndex, activeEditor->GetSelection());
    if (items.empty() || _options.headless)
        return;

    const auto source    = activeEditor->Snapshot();
    const auto revision  = activeEditor->Revision();
    const auto selection = activeEditor->GetSelection();
    const auto key       = _session.key;
    const auto lifetime  = GetControlLifetimeToken(*activeEditor);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(_window.get(),
                                       screenPoint,
                                       items,
                                       _host.GetTheme(),
                                       [this, activeEditor, source, revision, selection, key, lifetime](std::optional<int> selected) noexcept
    {
        RunUiAction([&]
        {
            if (selected)
                (void)HandleSpellMenuSelection(*selected, key, source, revision, selection);
            if (! IsUiAttached() || lifetime.expired() || ActiveEditor() != activeEditor)
                return;
            OnMenuClosed(std::nullopt);
        });
    },
                                       callbacks);
}
} // namespace RedTilde
