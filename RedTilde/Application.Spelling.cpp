#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "res/Resource.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <commdlg.h>
#include <cwctype>
#include <shobjidl.h>
#include <utility>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
constexpr int SpellSuggestionBaseId           = 10000;
constexpr int SpellMaximumMenuItems           = 5;
constexpr int SpellIgnoreOnceId               = 10098;
constexpr int SpellNoSuggestionsId            = 10099;
constexpr int SpellIgnoreAllId                = 10100;
constexpr int SpellAddWordId                  = 10101;
constexpr std::size_t ViewportOverscanUnits   = 2048u;
constexpr std::size_t MaximumVisibleSquiggles = 256u;

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

[[nodiscard]] bool EqualTagIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept
{
    return a.size() == b.size() &&
           (a.empty() || CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL);
}

[[nodiscard]] std::wstring QueryUserDefaultLocaleTag()
{
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH) > 0)
    {
        std::wstring tag = SanitizeSpellUiText(buffer, 32u);
        if (IsValidLanguageTag(tag) && ! EqualTagIgnoreCase(tag, L"auto"))
            return tag;
    }
    return L"en-US";
}

[[nodiscard]] std::wstring FormatFriendlyLanguageName(std::wstring_view tag)
{
    if (EqualTagIgnoreCase(tag, L"auto"))
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

[[nodiscard]] bool IsWordCharForSuppression(wchar_t ch) noexcept
{
    if (ch == L'\'' || ch == L'\x2019' || ch == L'-')
        return true;
    WORD type{};
    if (! GetStringTypeW(CT_CTYPE1, &ch, 1, &type))
        return false;
    return (type & (C1_ALPHA | C1_DIGIT)) != 0u;
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

[[nodiscard]] std::optional<D2D1_RECT_F> FindPaddedIssueRectAtPoint(DxUi::ControlHost& host,
                                                                    const Editor& editor,
                                                                    const std::vector<DxUi::RichText::MarkdownRange>& fragments,
                                                                    D2D1_POINT_2F ptDip)
{
    if (fragments.empty())
        return std::nullopt;
    const auto rects = editor.TryGetTextInputRangeRects(host, fragments.front().first, fragments.back().last);
    if (! rects || rects->empty())
        return std::nullopt;
    for (const auto& rect : *rects)
    {
        if (ptDip.x >= rect.left - 4.f && ptDip.x <= rect.right + 4.f && ptDip.y >= rect.top - 4.f && ptDip.y <= rect.bottom + 8.f)
            return rect;
    }
    return std::nullopt;
}
} // namespace

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
#if defined(REDTILDE_ENABLE_TESTS)
    if (_options.spellProviderForTest)
        _spellService.SetProviderForTest(_options.spellProviderForTest);
#endif
    const HRESULT dictHr = _customDictionary.Load(ActiveCustomDictionaryPath());
    if (FAILED(dictHr))
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Custom dictionary could not be loaded; read-only fallback disabled");
    _spell.cachedUserLocale  = QueryUserDefaultLocaleTag();
    _spell.activeLanguages   = ResolveEffectiveSpellLanguages(std::nullopt);
    _spell.statusInitialized = true;
    RefreshSpellStatusChip();
    RefreshProofingDrawerControls();
}

std::vector<std::wstring> Application::ResolveEffectiveSpellLanguages(const std::optional<std::wstring>& frontMatterHint) const
{
    std::vector<std::wstring> resolved;
    auto pushUnique = [&](std::wstring_view tag)
    {
        std::wstring candidate = SanitizeSpellUiText(tag, 32u);
        if (EqualTagIgnoreCase(candidate, L"auto"))
            candidate = _spell.cachedUserLocale ? *_spell.cachedUserLocale : QueryUserDefaultLocaleTag();
        if (! IsValidLanguageTag(candidate) || EqualTagIgnoreCase(candidate, L"auto"))
            return;
        if (std::ranges::any_of(resolved, [&](const std::wstring& existing) { return EqualTagIgnoreCase(existing, candidate); }))
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
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached())
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

    DxUi::NativeTextInputState native;
    if ((_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex) || HasBlockDraft())
    {
        _spell.checkPending     = true;
        _spell.immediatePending = false;
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

void Application::DispatchSpellCheckNow()
{
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached() || ! _editor || ! _settings.spelling.enabled || _switchingDocument)
    {
        _spell.immediatePending = false;
        return;
    }

    DxUi::NativeTextInputState native;
    if ((_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex) || HasBlockDraft())
    {
        _spell.immediatePending = false;
        return;
    }

    const auto target      = _editor;
    const auto snapshot    = target->Snapshot();
    const auto& sourceText = snapshot.Text();
    const bool sourceMode  = _settings.view == EditorView::Source;
    const auto syntax      = target->GetMarkdownSyntax();
    const auto semantic    = sourceMode ? nullptr : target->GetSemanticDocument();

    ExtractedProse extracted;
    std::uint64_t semanticIdentity = 0u;
    if (! sourceMode)
    {
        if (! semantic)
        {
            // Semantic preparation still running; ReconcilePostPreparePlacement / Prepare will retrigger.
            _spell.checkPending = true;
            return;
        }
        semanticIdentity = semantic->Identity();
        extracted        = ExtractSemanticProse(*semantic, syntax.get(), sourceText);
    }
    else
    {
        if (! syntax)
        {
            if (_editor && ! _editor->IsVisible())
            {
                _spell.immediatePending = false;
                _spell.checkPending     = true;
            }
            else
            {
                _spell.checkPending = true;
            }
            return;
        }
        extracted = ExtractSourceProse(*syntax, sourceText);
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
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached() || ! _editor || ! _settings.spelling.enabled)
    {
        (void)_spellService.TakeResult();
        return;
    }

    const auto maybeResult = _spellService.TakeResult();
    if (! maybeResult)
        return;
    const auto& result = *maybeResult;

    const auto currentSnapshot     = _editor->Snapshot();
    const bool sourceMode          = _settings.view == EditorView::Source;
    const auto semantic            = sourceMode ? nullptr : _editor->GetSemanticDocument();
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
        else if (effective != ProviderAvailability::DictionaryUnreadable && i == 0u && i < result.diagnostics.size())
        {
            diag = result.diagnostics[i];
        }
    }
    if (result.availabilities.empty())
        effective = ProviderAvailability::Ready;

    _spell.effectiveAvailability = effective;
    _spell.primaryDiagnostic     = std::move(diag);
    _spell.issues.clear();

    if (effective == ProviderAvailability::Ready)
    {
        const auto& srcText = currentSnapshot.Text();
        _spell.issues.reserve(std::min<std::size_t>(result.issues.size(), SpellService::MaximumRawIssues));
        for (const auto& raw : result.issues)
        {
            if (_spell.issues.size() >= SpellService::MaximumRawIssues)
                break;
            SpellIssue issue;
            issue.displayRange         = {raw.offset, raw.offset + raw.length};
            issue.word                 = raw.word;
            issue.normalizedKey        = raw.normalizedKey;
            issue.primaryLanguageIndex = raw.primaryLanguageIndex;
            if (! sourceMode)
            {
                if (! semantic || ! TryProjectDisplayErrorToSource(*semantic, issue.displayRange, issue.sourceFragments))
                    continue;
            }
            else
            {
                if (issue.displayRange.last > srcText.size() || ! currentSnapshot.IsBoundary(issue.displayRange.first) ||
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
    if (! activeEditor || activeEditor != _editor)
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
    const auto nextWord            = WordRangeAtOffset(activeEditor->Snapshot().Text(), sel.caret);
    _spell.suppressedTypingWord    = nextWord;
    _spell.typingSuppressionActive = nextWord.has_value();
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
    if (! _spell.typingSuppressionActive || ! _editor)
        return;
    const auto sel = _editor->GetSelection();
    if (ActiveEditor() != _editor || sel.Start() != sel.End() || ! _spell.suppressedTypingWord)
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
    if (IsClosing() || ! _editor)
        return;

    if (_mirror)
        (void)_mirror->TrySetDecorations(_mirror->Snapshot(), {});

    const auto expectedSource = _editor->Snapshot();
    DxUi::NativeTextInputState native;
    const bool imeActive = _host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex;
    if (! _settings.spelling.enabled || _spell.effectiveAvailability != ProviderAvailability::Ready || imeActive || HasBlockDraft() || _spell.issues.empty())
    {
        (void)_editor->TrySetDecorations(expectedSource, {});
        return;
    }

    DxUi::RichText::MarkdownRange visibleWindow{0u, expectedSource.Length()};
    if (const auto visSource = _editor->GetVisibleSourceRange(_host))
    {
        const std::size_t first = visSource->first > ViewportOverscanUnits ? visSource->first - ViewportOverscanUnits : 0u;
        const std::size_t last  = std::min(expectedSource.Length(), visSource->second + ViewportOverscanUnits);
        visibleWindow           = {first, last};
    }

    std::vector<DxUi::RichTextDecoration> decorations;
    decorations.reserve(std::min<std::size_t>(_spell.issues.size() * 2u, MaximumVisibleSquiggles));

    auto collectDecorations = [&](std::size_t cap)
    {
        decorations.clear();
        for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
        {
            const auto& issue = _spell.issues[i];
            if (issue.sourceFragments.empty())
                continue;
            const auto issueSpan = DxUi::RichText::MarkdownRange{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
            if (! RangesOverlap(issueSpan, visibleWindow))
                continue;
            if (_spell.typingSuppressionActive && _spell.suppressedTypingWord && RangesOverlap(issueSpan, *_spell.suppressedTypingWord))
                continue;
            if (_customDictionary.IsAcceptedNormalized(issue.normalizedKey, _session.key, issue.displayRange.first))
                continue;

            for (const auto& frag : issue.sourceFragments)
            {
                if (decorations.size() >= cap)
                    return;
                if (frag.first >= frag.last || frag.last > expectedSource.Length() || ! expectedSource.IsBoundary(frag.first) ||
                    ! expectedSource.IsBoundary(frag.last))
                    continue;
                decorations.push_back(DxUi::RichTextDecoration{frag.first,
                                                               frag.last - frag.first,
                                                               DxUi::RichTextDecorationKind::Squiggle,
                                                               DxUi::RichTextDecorationRole::Error,
                                                               DxUi::RichTextAnnotationKind::SpellingError});
            }
        }
    };

    for (const std::size_t cap : {MaximumVisibleSquiggles, std::size_t{128u}, std::size_t{64u}})
    {
        collectDecorations(cap);
        const HRESULT hr = _editor->TrySetDecorations(expectedSource, decorations);
        if (hr != E_BOUNDS && _editor->GetDecorationStatus() != DxUi::RichTextDecorationStatus::CapacityExceeded)
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
        const std::wstring friendly   = FormatFriendlyLanguageName(primaryTag);
        if (_spell.effectiveAvailability == ProviderAvailability::NoDictionary)
        {
            chipText = FormatStringResource(nullptr, IDS_SPELL_NO_DICTIONARY, SanitizeSpellUiText(primaryTag, 20u));
        }
        else if (_spell.effectiveAvailability == ProviderAvailability::DictionaryUnreadable)
        {
            chipText = FormatStringResource(nullptr, IDS_SPELL_DICT_UNREADABLE, SanitizeSpellUiText(primaryTag, 20u));
        }
        else if (_spell.activeLanguages.size() > 1u)
        {
            std::wstring combined;
            for (std::size_t i = 0u; i < _spell.activeLanguages.size(); ++i)
            {
                if (i > 0u)
                    combined += L" / ";
                const auto sep = _spell.activeLanguages[i].find(L'-');
                combined += SanitizeSpellUiText(std::wstring_view(_spell.activeLanguages[i]).substr(0u, sep), 8u);
            }
            chipText = SanitizeSpellUiText(combined, 48u);
        }
        else
        {
            chipText = SanitizeSpellUiText(friendly, 48u);
        }
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
    {
        RefreshSpellLanguageCombos(false);
        _spellingLanguageCombo->SetEnabled(_settings.spelling.enabled);
    }
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
        if (clean.empty() || EqualTagIgnoreCase(clean, L"auto") || EqualTagIgnoreCase(clean, L"off"))
            return;
        if (std::ranges::any_of(candidateTags, [&](const std::wstring& e) { return EqualTagIgnoreCase(e, clean); }))
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
        for (const wchar_t* common : {L"en-US", L"fr-FR", L"de-DE", L"es-ES"})
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
    std::ranges::sort(localeItems,
                      [](const DxUi::ComboBox::Item& a, const DxUi::ComboBox::Item& b)
    {
        const int cmp =
            CompareStringOrdinal(a.display.c_str(), static_cast<int>(a.display.size()), b.display.c_str(), static_cast<int>(b.display.size()), TRUE);
        return cmp == CSTR_LESS_THAN;
    });
    for (auto& item : localeItems)
        baseItems.push_back(std::move(item));

    auto findIndexByValue = [](const std::vector<DxUi::ComboBox::Item>& list, std::wstring_view value) -> std::optional<std::size_t>
    {
        for (std::size_t i = 0u; i < list.size(); ++i)
        {
            if (EqualTagIgnoreCase(list[i].value, value))
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
        if (_settings.spelling.enabled && ! EqualTagIgnoreCase(configuredTag, L"auto") &&
            (_spell.effectiveAvailability == ProviderAvailability::NoDictionary || _spell.effectiveAvailability == ProviderAvailability::DictionaryUnreadable))
        {
            const unsigned resId =
                _spell.effectiveAvailability == ProviderAvailability::DictionaryUnreadable ? IDS_SPELL_DICT_UNREADABLE : IDS_SPELL_NO_DICTIONARY;
            drawerItems[*selIdx].display = FormatStringResource(nullptr, resId, SanitizeSpellUiText(configuredTag, 20u));
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
        else if (_spell.effectiveAvailability == ProviderAvailability::NoDictionary ||
                 _spell.effectiveAvailability == ProviderAvailability::DictionaryUnreadable)
        {
            const unsigned resId =
                _spell.effectiveAvailability == ProviderAvailability::DictionaryUnreadable ? IDS_SPELL_DICT_UNREADABLE : IDS_SPELL_NO_DICTIONARY;
            selIdx = findIndexByValue(statusItems, primaryTag);
            if (! selIdx)
            {
                statusItems.insert(statusItems.end() - 1, DxUi::ComboBox::Item{primaryTag, {}});
                selIdx = statusItems.size() - 2u;
            }
            statusItems[*selIdx].display = FormatStringResource(nullptr, resId, SanitizeSpellUiText(primaryTag, 20u));
        }
        else if (_spell.activeLanguages.size() > 1u)
        {
            std::wstring combined;
            for (std::size_t i = 0u; i < _spell.activeLanguages.size(); ++i)
            {
                if (i > 0u)
                    combined += L" / ";
                const auto sep = _spell.activeLanguages[i].find(L'-');
                combined += SanitizeSpellUiText(std::wstring_view(_spell.activeLanguages[i]).substr(0u, sep), 8u);
            }
            selIdx = findIndexByValue(statusItems, primaryTag);
            if (! selIdx)
                selIdx = 0u;
            statusItems[*selIdx].display = SanitizeSpellUiText(combined, 48u);
        }
        else
        {
            selIdx =
                findIndexByValue(statusItems, EqualTagIgnoreCase(configuredTag, L"auto") ? std::wstring_view(primaryTag) : std::wstring_view(configuredTag));
            if (! selIdx)
                selIdx = 0u;
        }
        if (! ComboItemsEqual(_spellStatusCombo->GetItems(), statusItems))
            _spellStatusCombo->SetItems(std::move(statusItems));
        if (_spellStatusCombo->GetSelectedIndex() != selIdx)
            _spellStatusCombo->SetSelectedIndex(selIdx);
    }
}

void Application::ApplySpellLanguageSelection(std::wstring_view value, bool fromStatusBar)
{
    if (IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;

    if (fromStatusBar && EqualTagIgnoreCase(value, L"off"))
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

    const std::wstring nextTag = EqualTagIgnoreCase(value, L"auto") ? L"auto" : SanitizeSpellUiText(value, 32u);
    if (nextTag.empty())
        return;

    const bool wasEnabled        = _settings.spelling.enabled;
    const bool sameConfiguredTag = _settings.spelling.languages.size() == 1u && EqualTagIgnoreCase(_settings.spelling.languages.front(), nextTag) &&
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

std::optional<std::size_t> Application::FindSpellIssueAtSourceOffset(std::size_t sourceOffset) const noexcept
{
    for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
    {
        const auto& issue = _spell.issues[i];
        if (_customDictionary.IsAcceptedNormalized(issue.normalizedKey, _session.key, issue.displayRange.first))
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
    if (issueIndex >= _spell.issues.size() || ! _editor)
        return {};

    const auto& issue    = _spell.issues[issueIndex];
    const auto languages = _spell.activeLanguages;
    std::vector<std::wstring> suggestions;
    (void)_spellService.SuggestSyncBounded(languages, issue.word, _customDictionary.Generation(), SpellMaximumMenuItems, suggestions, 500u);

    PendingSpellMenu menuState;
    menuState.workspaceKey = _session.key;
    menuState.source       = _editor->Snapshot();
    menuState.revision     = _editor->Revision();
    menuState.selection    = selection;
    menuState.issue        = issue;
    menuState.suggestions  = suggestions;
    menuState.languages    = languages;
    menuState.lifetime     = GetControlLifetimeToken(*_editor);
    _spell.pendingMenu     = std::move(menuState);

    std::vector<DxUi::MenuFlyoutItem> items;
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
            sug.text      = SanitizeSpellUiText(suggestions[i], 64u);
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
    if (! _settings.spelling.enabled || ! _editor || ActiveEditor() != _editor || _spell.issues.empty())
        return;

    DxUi::NativeTextInputState native;
    if ((_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex) || HasBlockDraft())
        return;

    std::optional<std::size_t> hitOffset;
    if (! keyboardInvocation && _window)
    {
        POINT clientPt = screenPoint;
        if (ScreenToClient(_window.get(), &clientPt))
        {
            const float scale       = 96.f / _host.GetDpi();
            const D2D1_POINT_2F dip = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
            hitOffset               = _editor->TryHitTestTextInputPoint(_host, dip);
        }
    }
    const auto selection = _editor->GetSelection();
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
                           selectedId == SpellIgnoreOnceId || selectedId == SpellIgnoreAllId || selectedId == SpellAddWordId ||
                           selectedId == SpellNoSuggestionsId;
    if (! isSpellId)
        return false;

    const auto pending = std::exchange(_spell.pendingMenu, std::nullopt);
    if (! pending || ! IsUiAttached() || pending->lifetime.expired() || ActiveEditor() != _editor || HasBlockDraft())
        return true;

    if (pending->workspaceKey != workspaceKey || _session.key != workspaceKey || ! _editor->Snapshot().IsSameRevision(pending->source) ||
        ! _editor->Snapshot().IsSameRevision(source) || _editor->Revision() != pending->revision || _editor->Revision() != revision ||
        _editor->GetSelection() != pending->selection || _editor->GetSelection() != selection)
    {
        Status(IDS_LINKCHANGED);
        return true;
    }

    DxUi::NativeTextInputState native;
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return true;

    if (selectedId == SpellIgnoreOnceId)
    {
        _customDictionary.IgnoreOnce(_session.key, pending->issue.word, pending->issue.displayRange.first);
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
        if (! _editor->ApplySourceEdits(source, revision, edits, {caretAfter, caretAfter}))
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
#if defined(REDTILDE_ENABLE_TESTS)
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
            if (std::ranges::none_of(options, [&](const std::wstring& e) { return EqualTagIgnoreCase(e, tag); }))
                options.push_back(tag);
        if (options.size() == 1u)
        {
            for (const wchar_t* common : {L"en-US", L"fr-FR", L"de-DE", L"es-ES"})
                if (std::ranges::none_of(options, [&](const std::wstring& e) { return EqualTagIgnoreCase(e, common); }))
                    options.push_back(common);
        }
        const std::wstring current = _settings.spelling.languages.empty() ? L"auto" : _settings.spelling.languages.front();
        std::size_t nextIdx        = 0u;
        for (std::size_t i = 0u; i < options.size(); ++i)
        {
            if (EqualTagIgnoreCase(options[i], current))
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
        _settings.spelling.customDictionaryPath = chosen.lexically_normal();
        if (FAILED(_customDictionary.Load(ActiveCustomDictionaryPath())))
            Status(IDS_SPELL_DICT_SAVE_FAILED);
        RefreshProofingDrawerControls();
        ScheduleSpellCheck(true);
        SavePreferences();
    }
    else if (command == Command::SpellResetCustomDictionary)
    {
        _settings.spelling.customDictionaryPath.clear();
        (void)_customDictionary.Load(ActiveCustomDictionaryPath());
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
    const bool modalOverlay = _activeOverlay == OverlayKind::BlockDraftModal || _activeOverlay == OverlayKind::RecoveryModal;
    if (! _settings.spelling.enabled || ! _editor || ActiveEditor() != _editor || _spell.issues.empty() ||
        _spell.effectiveAvailability != ProviderAvailability::Ready || modalOverlay || HasBlockDraft() || HasActiveModalPrompt() || _printPreview != nullptr)
        return;

    if ((modifiers & (MK_SHIFT | MK_CONTROL)) != 0u)
        return;
    if ((_spellStatusCombo && _spellStatusCombo->IsPopupOpen()) || (_spellingLanguageCombo && _spellingLanguageCombo->IsPopupOpen()))
        return;

    DxUi::NativeTextInputState native;
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return;

    const float scale         = 96.f / _host.GetDpi();
    const D2D1_POINT_2F ptDip = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
    const auto edBounds       = _editor->GetBounds();
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

    std::optional<std::size_t> issueIdx;
    const auto downHit = _editor->TryHitTestTextInputPoint(_host, ptDip);
    if (downHit)
    {
        issueIdx = FindSpellIssueAtSourceOffset(*downHit);
        if (issueIdx)
        {
            const auto& issue = _spell.issues[*issueIdx];
            const auto rects  = ! issue.sourceFragments.empty()
                                    ? _editor->TryGetTextInputRangeRects(_host, issue.sourceFragments.front().first, issue.sourceFragments.back().last)
                                    : std::nullopt;
            if (rects && ! rects->empty())
            {
                const auto hitRect = FindPaddedIssueRectAtPoint(_host, *_editor, issue.sourceFragments, ptDip);
                if (! hitRect)
                    return;
                _spell.leftDownIssueRect = hitRect;
            }
        }
    }
    if (! issueIdx)
    {
        const auto visSource       = _editor->GetVisibleSourceRange(_host);
        const std::size_t visFirst = visSource && visSource->first > ViewportOverscanUnits ? visSource->first - ViewportOverscanUnits : 0u;
        const std::size_t visLast  = visSource ? visSource->second + ViewportOverscanUnits : _editor->Snapshot().Length();
        for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
        {
            const auto& issue = _spell.issues[i];
            if (issue.sourceFragments.empty() || _customDictionary.IsAcceptedNormalized(issue.normalizedKey, _session.key, issue.displayRange.first))
                continue;
            const auto span = DxUi::RichText::MarkdownRange{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
            if (span.last < visFirst || span.first > visLast)
                continue;
            if (const auto hitRect = FindPaddedIssueRectAtPoint(_host, *_editor, issue.sourceFragments, ptDip))
            {
                issueIdx                 = i;
                _spell.leftDownIssueRect = hitRect;
                break;
            }
        }
    }

    _spell.leftDownHitOffset = downHit;
    _spell.leftDownArmed     = true;
    _spell.leftDownClientPt  = clientPt;
    _spell.leftDownRevision  = _editor->Revision();
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

    const bool modalOverlay = _activeOverlay == OverlayKind::BlockDraftModal || _activeOverlay == OverlayKind::RecoveryModal;
    if (! _settings.spelling.enabled || ! _editor || ActiveEditor() != _editor || _spell.issues.empty() ||
        _spell.effectiveAvailability != ProviderAvailability::Ready || modalOverlay || HasBlockDraft() || HasActiveModalPrompt() || _printPreview != nullptr ||
        _editor->Revision() != _spell.leftDownRevision)
        return;

    const int maxDx = std::max(4, GetSystemMetrics(SM_CXDRAG));
    const int maxDy = std::max(4, GetSystemMetrics(SM_CYDRAG));
    if (std::abs(clientPt.x - _spell.leftDownClientPt.x) > maxDx || std::abs(clientPt.y - _spell.leftDownClientPt.y) > maxDy)
        return;

    const auto selection = _editor->GetSelection();
    if (selection.Start() != selection.End())
        return;

    DxUi::NativeTextInputState native;
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return;

    const float scale                    = 96.f / _host.GetDpi();
    const D2D1_POINT_2F ptDip            = {static_cast<float>(clientPt.x) * scale, static_cast<float>(clientPt.y) * scale};
    std::optional<std::size_t> hitOffset = _editor->TryHitTestTextInputPoint(_host, ptDip);
    const bool usedGeometryHit           = hitOffset.has_value();
    if (! hitOffset)
        hitOffset = downHitOffset;
    if (! hitOffset)
        hitOffset = selection.caret;

    auto issueIdx = FindSpellIssueAtSourceOffset(*hitOffset);
    std::optional<D2D1_RECT_F> anchorRect;
    if (! issueIdx)
    {
        const auto visSource       = _editor->GetVisibleSourceRange(_host);
        const std::size_t visFirst = visSource && visSource->first > ViewportOverscanUnits ? visSource->first - ViewportOverscanUnits : 0u;
        const std::size_t visLast  = visSource ? visSource->second + ViewportOverscanUnits : _editor->Snapshot().Length();
        for (std::size_t i = 0u; i < _spell.issues.size(); ++i)
        {
            const auto& candidate = _spell.issues[i];
            if (candidate.sourceFragments.empty() ||
                _customDictionary.IsAcceptedNormalized(candidate.normalizedKey, _session.key, candidate.displayRange.first))
                continue;
            const auto span = DxUi::RichText::MarkdownRange{candidate.sourceFragments.front().first, candidate.sourceFragments.back().last};
            if (span.last < visFirst || span.first > visLast)
                continue;
            if (const auto hitRect = FindPaddedIssueRectAtPoint(_host, *_editor, candidate.sourceFragments, ptDip))
            {
                issueIdx   = i;
                anchorRect = hitRect;
                break;
            }
        }
        if (! issueIdx && downIssueRect)
            issueIdx = FindSpellIssueAtSourceOffset(selection.caret);
    }
    if (! issueIdx)
        return;

    const auto& issue = _spell.issues[*issueIdx];
    if (! issue.sourceFragments.empty())
    {
        const auto issueSpan = DxUi::RichText::MarkdownRange{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
        if (_spell.typingSuppressionActive && _spell.suppressedTypingWord && RangesOverlap(issueSpan, *_spell.suppressedTypingWord))
            return;
    }

    if (! anchorRect && ! issue.sourceFragments.empty())
    {
        const auto rects = _editor->TryGetTextInputRangeRects(_host, issue.sourceFragments.front().first, issue.sourceFragments.back().last);
        if (rects && ! rects->empty())
        {
            anchorRect = FindPaddedIssueRectAtPoint(_host, *_editor, issue.sourceFragments, ptDip);
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

    _spell.pendingLeftClick = PendingLeftClickRequest{_session.key, _editor->Revision(), *issueIdx, screenPt};
    if (_window)
        (void)PostMessageW(_window.get(), SpellMenuRequested, 0, 0);
}

void Application::HandlePostedSpellLeftClickMenu()
{
    const auto req = std::exchange(_spell.pendingLeftClick, std::nullopt);
    if (! req || IsClosing() || IsSessionEnding() || ! IsUiAttached())
        return;

    const bool modalOverlay = _activeOverlay == OverlayKind::BlockDraftModal || _activeOverlay == OverlayKind::RecoveryModal;
    if (! _settings.spelling.enabled || ! _editor || ActiveEditor() != _editor || _spell.issues.empty() ||
        _spell.effectiveAvailability != ProviderAvailability::Ready || modalOverlay || HasBlockDraft() || HasActiveModalPrompt() || _printPreview != nullptr)
        return;

    if ((_spellStatusCombo && _spellStatusCombo->IsPopupOpen()) || (_spellingLanguageCombo && _spellingLanguageCombo->IsPopupOpen()))
        return;

    if (req->workspaceKey != _session.key || req->revision != _editor->Revision() || req->issueIndex >= _spell.issues.size())
        return;

    const auto selection = _editor->GetSelection();
    if (selection.Start() != selection.End())
        return;

    DxUi::NativeTextInputState native;
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return;

    const auto& issue = _spell.issues[req->issueIndex];
    if (! issue.sourceFragments.empty())
    {
        const auto issueSpan = DxUi::RichText::MarkdownRange{issue.sourceFragments.front().first, issue.sourceFragments.back().last};
        if (_spell.typingSuppressionActive && _spell.suppressedTypingWord && RangesOverlap(issueSpan, *_spell.suppressedTypingWord))
            return;
    }

    ShowSpellLeftClickMenu(req->issueIndex, req->screenPt);
}

void Application::ShowSpellLeftClickMenu(std::size_t issueIndex, POINT screenPoint)
{
    if (! _editor || ActiveEditor() != _editor)
        return;

    auto items = BuildSpellActionMenuItems(issueIndex, _editor->GetSelection());
    if (items.empty() || _options.headless)
        return;

    auto* const primary  = _editor;
    const auto source    = _editor->Snapshot();
    const auto revision  = _editor->Revision();
    const auto selection = _editor->GetSelection();
    const auto key       = _session.key;
    const auto lifetime  = GetControlLifetimeToken(*_editor);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(_window.get(),
                                       screenPoint,
                                       items,
                                       _host.GetTheme(),
                                       [this, primary, source, revision, selection, key, lifetime](std::optional<int> selected) noexcept
    {
        RunUiAction([&]
        {
            if (selected)
                (void)HandleSpellMenuSelection(*selected, key, source, revision, selection);
            if (! IsUiAttached() || lifetime.expired() || ActiveEditor() != primary)
                return;
            OnMenuClosed(std::nullopt);
        });
    },
                                       callbacks);
}
} // namespace RedTilde
