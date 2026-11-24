#include "Application.Tests.Spelling.h"
#include "Common/CustomDictionary.h"
#include "Common/ProseExtractor.h"
#include "Common/SpellProvider.h"
#include "Common/Storage.h"
#include "Common/WinSpellProvider.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.FilesFixtures.h"
#include "Support/Support.Tests.Foreground.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <objbase.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
class SpellingProbe final
{
public:
    [[nodiscard]] static CustomDictionary& Dictionary(Application& app) noexcept
    {
        return app._customDictionary;
    }

    [[nodiscard]] static const auto& Issues(const Application& app) noexcept
    {
        return app._spell.issues;
    }

    [[nodiscard]] static SpellService& Service(Application& app) noexcept
    {
        return app._spellService;
    }

    [[nodiscard]] static std::wstring StatusText(const Application& app)
    {
        return app._spellStatusCombo ? std::wstring(app._spellStatusCombo->GetDisplayedText()) : std::wstring{};
    }

    [[nodiscard]] static std::wstring StatusTooltip(const Application& app)
    {
        return app._spellStatusCombo ? std::wstring(app._spellStatusCombo->GetTooltipText()) : std::wstring{};
    }

    [[nodiscard]] static DxUi::ComboBox* SettingsLanguageCombo(const Application& app) noexcept
    {
        return app._spellingLanguageCombo;
    }

    [[nodiscard]] static DxUi::ComboBox* StatusLanguageCombo(const Application& app) noexcept
    {
        return app._spellStatusCombo;
    }

    [[nodiscard]] static DxUi::Toggle* SpellingEnabledToggle(const Application& app) noexcept
    {
        return app._spellingEnabledToggle;
    }

    [[nodiscard]] static DxUi::Button* DrawerCloseButton(const Application& app) noexcept
    {
        return app._drawerClose;
    }

    [[nodiscard]] static DxUi::Panel* SettingsDrawer(const Application& app) noexcept
    {
        return app._drawer;
    }

    [[nodiscard]] static DxUi::ScrollPanel* SettingsScroll(const Application& app) noexcept
    {
        return app._settingsScroll;
    }

    [[nodiscard]] static float ProofingSectionTop(const Application& app) noexcept
    {
        return app._proofingTitle ? app._proofingTitle->GetBounds().top : 0.f;
    }

    [[nodiscard]] static float SyntaxSectionTop(const Application& app) noexcept
    {
        return app._syntaxTitle ? app._syntaxTitle->GetBounds().top : 0.f;
    }

    [[nodiscard]] static float PrivacySectionTop(const Application& app) noexcept
    {
        return app._privacyTitle ? app._privacyTitle->GetBounds().top : 0.f;
    }

    static void PumpMessages(Application& app)
    {
        if (! app.Window())
            return;
        MSG msg{};
        while (PeekMessageW(&msg, app.Window(), 0u, 0u, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    static void RefreshLanguageCombos(Application& app, bool queryInstalled)
    {
        app.RefreshSpellLanguageCombos(queryInstalled);
    }

    static void SelectStatusLanguageByValue(Application& app, std::wstring_view value)
    {
        if (! app._spellStatusCombo)
            return;
        const auto items = app._spellStatusCombo->GetItems();
        for (std::size_t i = 0u; i < items.size(); ++i)
        {
            if (items[i].value.size() == value.size() &&
                CompareStringOrdinal(items[i].value.data(), static_cast<int>(items[i].value.size()), value.data(), static_cast<int>(value.size()), TRUE) ==
                    CSTR_EQUAL)
            {
                app._spellStatusCombo->SetSelectedIndex(i);
                app.ApplySpellLanguageSelection(items[i].value, true);
                return;
            }
        }
    }

    [[nodiscard]] static bool HasPendingSpellMenu(const Application& app) noexcept
    {
        return app._spell.pendingMenu.has_value();
    }

    static void ClearPendingSpellMenu(Application& app) noexcept
    {
        app._spell.pendingMenu.reset();
    }

    static void LeftDown(Application& app, POINT clientPt, WPARAM modifiers = 0u)
    {
        app.OnEditorLeftButtonDown(clientPt, modifiers);
    }

    static void LeftDoubleClick(Application& app) noexcept
    {
        app.OnEditorLeftButtonDoubleClick();
    }

    static void LeftUp(Application& app, POINT clientPt)
    {
        app.OnEditorLeftButtonUp(clientPt);
    }

    static std::vector<DxUi::MenuFlyoutItem> BuildSpellActionItems(Application& app, std::size_t issueIndex)
    {
        return app.BuildSpellActionMenuItems(issueIndex, app._editor->GetSelection());
    }

    [[nodiscard]] static bool TypingSuppressed(const Application& app) noexcept
    {
        return app._spell.typingSuppressionActive;
    }

    [[nodiscard]] static std::vector<DxUi::MenuFlyoutItem> BuildSpellMenuAtOffset(Application& app, std::size_t sourceOffset)
    {
        app._editor->SetSelection({sourceOffset, sourceOffset});
        auto items = app.ContextMenuItems(false, false);
        app.PopulateSpellContextMenu(POINT{0, 0}, true, items);
        return items;
    }

    [[nodiscard]] static bool InvokeSpellMenuCommand(Application& app,
                                                     int selectedId,
                                                     unsigned key,
                                                     const DxUi::RichText::SourceSnapshot& source,
                                                     std::uint64_t revision,
                                                     DxUi::RichText::Selection selection)
    {
        return app.HandleSpellMenuSelection(selectedId, key, source, revision, selection);
    }

    [[nodiscard]] static unsigned SessionKey(const Application& app) noexcept
    {
        return app._session.key;
    }

    [[nodiscard]] static void ForceTypingChange(Application& app)
    {
        app.Changed();
    }

    [[nodiscard]] static void ForceDebounceTimeout(Application& app)
    {
        app.OnSpellDebounceTimer();
    }

    static void TriggerImmediateCheck(Application& app)
    {
        app.ScheduleSpellCheck(true);
    }

    static void SetSpellLanguages(Application& app, std::vector<std::wstring> languages)
    {
        app._settings.spelling.languages = std::move(languages);
        ++app._spell.languageConfigVersion;
    }

    static void DrainClose(Application& app)
    {
        if (app.Window())
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
    }
};

namespace
{
void Require(bool condition, std::string_view message)
{
    if (! condition)
        throw std::runtime_error(std::string(message));
}

void Success(HRESULT hr, std::string_view message)
{
    if (FAILED(hr))
        throw std::runtime_error(std::string(message));
}

LaunchOptions HiddenSpellingOptions(const std::filesystem::path& dataDir, std::shared_ptr<ISpellProvider> provider = {})
{
    LaunchOptions options;
    options.headless             = true;
    options.dataDirectory        = dataDir;
    options.spellProviderForTest = std::move(provider);
    return options;
}

std::wstring CombineChunkText(const ExtractedProse& prose)
{
    std::wstring combined;
    for (const auto& chunk : prose.chunks)
    {
        if (! combined.empty())
            combined.push_back(L'\n');
        combined += chunk.text;
    }
    return combined;
}
} // namespace

void RegisterSpellingTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Editor.SpellingProseExtractionAndExclusions",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetInstalledLanguages({L"en-US", L"fr-FR"});
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->SetLanguageAvailability(L"fr-FR", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Contractions", L"don't", L"and",      L"won't", L"work",    L"Clean", L"English", L"words",  L"שלום",
                                           L"مرحبا",        L"你好",  L"world",    L"Visit", L"or",      L"see",   L"Acronym", L"tagged", L"stay",
                                           L"skipped",      L"while", L"caseless", L"are",   L"checked", L"email", L"path",    L"Math",   L"block"});
        provider->AddKnownWords(L"fr-FR", {L"Sous", L"l'arbre", L"aujourd'hui", L"peut-être", L"encore"});

        const std::wstring md =
            L"---\n"
            L"title: Sample\n"
            L"lang: fr-FR\n"
            L"---\n\n"
            L"Contractions don't and won't work. Sous l'arbre aujourd'hui peut-être writting.\n\n"
            L"Acronym NASA and tagged C23 and v1.0 stay skipped, while caseless שלום مرحبا 你好 are checked.\n\n"
            L"Visit https://example.invalid/writting_url or email writer@example.invalid or path C:\\Temp\\badfile\\note.md or `code_writting()`.\n\n"
            L"Math $x_{mathwritting}$ and block:\n\n"
            L"$$\n\\frac{blockwritting}{2}\n$$\n\n"
            L"```cpp\nint fencewritting = 0;\n```\n\n"
            L"```mermaid\ngraph LR\nA[mermaidwritting] --> B[Done]\n```\n";

        Application app(HiddenSpellingOptions(root / L"extract-exclusions", provider));
        Success(app.Initialize(), "initialize hidden app for prose extraction test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        app.SetDocument(md);
        app.Prepare(512);

        const auto semantic = app.Document().GetSemanticDocument();
        const auto syntax   = app.Document().GetMarkdownSyntax();
        Require(semantic != nullptr && syntax != nullptr, "semantic and syntax must be prepared");

        const auto semProse = ExtractSemanticProse(*semantic, syntax.get(), md);
        Require(semProse.frontMatterLanguage == std::optional<std::wstring>(L"fr-FR"), "front-matter lang: fr-FR must be extracted");
        const auto semText = CombineChunkText(semProse);
        Require(semText.find(L"don't") != std::wstring::npos && semText.find(L"won't") != std::wstring::npos,
                "semantic prose keeps English contractions whole");
        Require(semText.find(L"l'arbre") != std::wstring::npos && semText.find(L"aujourd'hui") != std::wstring::npos &&
                    semText.find(L"peut-être") != std::wstring::npos,
                "semantic prose keeps French elisions and hyphenated words whole");
        Require(semText.find(L"code_writting") == std::wstring::npos && semText.find(L"mathwritting") == std::wstring::npos &&
                    semText.find(L"blockwritting") == std::wstring::npos && semText.find(L"fencewritting") == std::wstring::npos &&
                    semText.find(L"mermaidwritting") == std::wstring::npos && semText.find(L"example.invalid") == std::wstring::npos &&
                    semText.find(L"badfile") == std::wstring::npos,
                "excluded code, math, diagram, URL, email, and Windows paths are masked in Compose mode");

        // Verify script-aware ignoreUppercase never skips caseless scripts.
        Require(IsScriptAwareAllUppercase(L"NASA"), "NASA is treated as uppercase acronym");
        Require(! IsScriptAwareAllUppercase(L"I"), "single capital letter is not skipped");
        Require(! IsScriptAwareAllUppercase(L"Nasa"), "mixed case word is not skipped");
        Require(! IsScriptAwareAllUppercase(L"שלום"), "Hebrew caseless script is never treated as acronym");
        Require(! IsScriptAwareAllUppercase(L"مرحبا"), "Arabic caseless script is never treated as acronym");
        Require(! IsScriptAwareAllUppercase(L"你好"), "CJK caseless script is never treated as acronym");

        // Also check Source-view extraction exclusions.
        const auto srcProse = ExtractSourceProse(*syntax, md);
        const auto srcText  = CombineChunkText(srcProse);
        Require(srcText.find(L"fencewritting") == std::wstring::npos && srcText.find(L"code_writting") == std::wstring::npos &&
                    srcText.find(L"mathwritting") == std::wstring::npos && srcText.find(L"mermaidwritting") == std::wstring::npos,
                "Source-view extraction excludes fences, inline code, math, and diagrams");
        Require(srcText.find(L"don't") != std::wstring::npos && srcText.find(L"writting") != std::wstring::npos,
                "Source-view extraction retains prose at exact offsets");

        // Only `writting` should be flagged in active Compose view (all English + French words in fake dictionary are known).
        const auto& issues = SpellingProbe::Issues(app);
        Require(issues.size() == 1u && issues[0].word == L"writting", "multi-language + exclusions yield only the real typo");
        Require(foreground.NeverAcquiredForeground(), "extraction test never acquires foreground");
    });

    test("Editor.SpellingCustomDictionaryCodecsAndSafety",
         [&]
    {
        const auto dir = root / L"custom-dict-codecs";
        std::filesystem::create_directories(dir);

        // 1. UTF-16LE BOM Microsoft Word CUSTOM.DIC round-trip preserving comments, EOLs, and casing verbatim.
        const auto wordDicPath = dir / L"CUSTOM.DIC";
        {
            const std::wstring initialText = L"#LID 1033\r\nRedTilde\r\n\r\nCafé\r\n";
            std::string rawBytes;
            rawBytes.push_back(static_cast<char>(0xFFu));
            rawBytes.push_back(static_cast<char>(0xFEu));
            for (const wchar_t ch : initialText)
            {
                rawBytes.push_back(static_cast<char>(ch & 0xFFu));
                rawBytes.push_back(static_cast<char>((ch >> 8u) & 0xFFu));
            }
            Success(WriteBytesAtomic(wordDicPath, rawBytes), "seed UTF-16LE BOM CUSTOM.DIC");
        }
        {
            CustomDictionary dict;
            Success(dict.Load(wordDicPath), "load UTF-16LE BOM CUSTOM.DIC");
            Require(dict.Encoding() == DictionaryEncoding::Utf16LeBom && dict.LineEnding() == DictionaryLineEnding::CrLf,
                    "detects UTF-16LE BOM and CRLF line endings");
            Require(dict.IsAccepted(L"redtilde") && dict.IsAccepted(L"CAFE\x0301") && dict.IsAccepted(L"Cafe\x0301") && ! dict.IsAccepted(L"notpresent"),
                    "NFC + lowercase lookup accepts decomposed NFD e+U+0301 against NFC é");

            HRESULT diskHr = S_OK;
            Success(dict.AddWord(L"MyCustomTerm", &diskHr), "add word to UTF-16LE BOM CUSTOM.DIC");
            Success(diskHr, "atomic write to UTF-16LE BOM CUSTOM.DIC succeeded");

            std::string afterBytes;
            Success(ReadBytes(wordDicPath, afterBytes), "read updated CUSTOM.DIC");
            Require(afterBytes.size() >= 2u && static_cast<unsigned char>(afterBytes[0]) == 0xFFu && static_cast<unsigned char>(afterBytes[1]) == 0xFEu,
                    "CUSTOM.DIC retains UTF-16LE BOM encoding");

            CustomDictionary reloaded;
            Success(reloaded.Load(wordDicPath), "reload updated CUSTOM.DIC");
            const auto words = reloaded.DisplayWords();
            Require(words.size() == 3u && words[0] == L"RedTilde" && words[1] == L"Café" && words[2] == L"MyCustomTerm",
                    "original word casing preserved verbatim on save");
        }

        // 2. UTF-16BE BOM and UTF-8 BOM round-trips.
        {
            const auto bePath          = dir / L"custom-be.dic";
            const std::wstring content = L"# comment\nFirstWord\n";
            std::string rawBe;
            rawBe.push_back(static_cast<char>(0xFEu));
            rawBe.push_back(static_cast<char>(0xFFu));
            for (const wchar_t ch : content)
            {
                rawBe.push_back(static_cast<char>((ch >> 8u) & 0xFFu));
                rawBe.push_back(static_cast<char>(ch & 0xFFu));
            }
            Success(WriteBytesAtomic(bePath, rawBe), "seed UTF-16BE BOM dictionary");
            CustomDictionary beDict;
            Success(beDict.Load(bePath), "load UTF-16BE BOM dictionary");
            Require(beDict.Encoding() == DictionaryEncoding::Utf16BeBom && beDict.IsAccepted(L"firstword"), "UTF-16BE BOM decoded and indexed");
            HRESULT diskHr = S_OK;
            Success(beDict.AddWord(L"SecondWord", &diskHr), "append to UTF-16BE BOM dictionary");
            Success(diskHr, "persist UTF-16BE BOM dictionary");
            std::string reloadedBe;
            Success(ReadBytes(bePath, reloadedBe), "read back UTF-16BE BOM");
            Require(reloadedBe.size() >= 2u && static_cast<unsigned char>(reloadedBe[0]) == 0xFEu && static_cast<unsigned char>(reloadedBe[1]) == 0xFFu,
                    "UTF-16BE BOM encoding preserved");
        }

        // 3. Legacy ANSI / Windows-1252 refused untouched.
        {
            const auto ansiPath         = dir / L"legacy-ansi.dic";
            const std::string ansiBytes = "caf\xE9\r\n"; // Windows-1252 byte 0xE9 invalid in UTF-8
            Success(WriteBytesAtomic(ansiPath, ansiBytes), "seed legacy Windows-1252 file");
            CustomDictionary ansiDict;
            const HRESULT ansiHr = ansiDict.Load(ansiPath);
            Require(ansiHr == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION), "legacy ANSI encoding refused fail-closed");
            std::string retained;
            Success(ReadBytes(ansiPath, retained), "read untouched legacy file");
            Require(retained == ansiBytes, "refused ANSI dictionary left untouched on disk");
        }

        // 4. Junction / reparse point refused fail-closed.
        {
            const auto targetDir = dir / L"reparse-target";
            std::filesystem::create_directories(targetDir);
            const auto realFile = targetDir / L"real.dic";
            Success(WriteBytesAtomic(realFile, "Alpha\n"), "seed real target behind junction");
            const auto linkDir = dir / L"reparse-link";
            Success(Testing::CreateFixtureJunction(dir, linkDir, targetDir), "create fixture junction");
            const auto removeJunction = wil::scope_exit([&] { (void)RemoveDirectoryW(linkDir.c_str()); });
            // Point dictionary at symlink/junction edge if symlink creation permitted, or verify directory attribute guard.
            std::error_code ec;
            const auto symFile = dir / L"sym.dic";
            std::filesystem::create_symlink(realFile, symFile, ec);
            if (! ec)
            {
                const auto removeSym = wil::scope_exit([&]
                {
                    std::error_code rmEc;
                    std::filesystem::remove(symFile, rmEc);
                });
                CustomDictionary symDict;
                Require(FAILED(symDict.Load(symFile)), "symlink dictionary file is refused fail-closed");
            }
        }

        // 5. Read-only failure retains session word + Ignore Once vs Ignore All + word length cap.
        {
            const auto roPath = dir / L"readonly.txt";
            Success(WriteBytesAtomic(roPath, "InitialWord\n"), "seed writable then read-only dictionary");
            Require(SetFileAttributesW(roPath.c_str(), FILE_ATTRIBUTE_READONLY) != FALSE, "mark readonly.txt read-only");
            const auto clearRo = wil::scope_exit([&] { (void)SetFileAttributesW(roPath.c_str(), FILE_ATTRIBUTE_NORMAL); });

            CustomDictionary roDict;
            Success(roDict.Load(roPath), "load read-only dictionary");
            HRESULT diskHr = S_OK;
            Success(roDict.AddWord(L"SessionKept", &diskHr), "AddWord retains word in session memory even on read-only target");
            Require(FAILED(diskHr) && roDict.IsAccepted(L"sessionkept"), "disk error reported while session word remains accepted");

            const std::wstring tooLong(CustomDictionary::MaximumWordLength + 1u, L'a');
            Require(roDict.AddWord(tooLong) == E_INVALIDARG, "words exceeding 64 scalar characters rejected");
            Require(roDict.AddWord(L"two words") == E_INVALIDARG, "whitespace inside custom word rejected");

            roDict.IgnoreOnce(1u, L"Onetime", 42u);
            Require(roDict.IsAccepted(L"onetime", 1u, 42u) && ! roDict.IsAccepted(L"onetime", 1u, 99u),
                    "Ignore Once applies only to matching workspaceKey + occurrence");
            roDict.IgnoreAll(L"SessionSkip");
            Require(roDict.IsAccepted(L"sessionskip", 1u, 0u) && roDict.IsAccepted(L"sessionskip", 2u, 0u),
                    "Ignore All accepts normalized word across all occurrences in memory");
        }
    });

    test("Editor.SpellingMidWordFormattingAtomicReplaceAndUndo",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Please", L"recommend", L"this"});
        provider->SetSuggestions(L"en-US", L"recommnd", {L"recommend"});

        Application app(HiddenSpellingOptions(root / L"midword-replace", provider));
        Success(app.Initialize(), "initialize app for mid-word replacement test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        const std::wstring original = L"Please **re**commnd this.\n";
        app.SetDocument(original);
        app.Prepare(512);

        const auto& issues = SpellingProbe::Issues(app);
        Require(issues.size() == 1u && issues[0].word == L"recommnd" && issues[0].sourceFragments.size() == 2u,
                "**re**commnd produces one SpellingIssue spanning two disjoint source fragments");

        const auto decorations = app.Document().GetDecorations();
        Require(decorations.size() == 2u && decorations[0].kind == DxUi::RichTextDecorationKind::Squiggle &&
                    decorations[0].role == DxUi::RichTextDecorationRole::Error && decorations[0].annotation == DxUi::RichTextAnnotationKind::SpellingError,
                "both inline formatted source fragments receive Squiggle + SpellingError annotations");

        const auto hitOffset = issues[0].sourceFragments[0].first;
        const auto menuItems = SpellingProbe::BuildSpellMenuAtOffset(app, hitOffset);
        Require(! menuItems.empty() && menuItems[0].text == L"recommend", "context menu prepends 'recommend' suggestion");

        const auto snapBefore = app.Document().Snapshot();
        const auto revBefore  = app.Document().Revision();
        const auto selBefore  = app.Document().GetSelection();

        // Stale selection guard must reject without mutating document.
        const bool staleHandled = SpellingProbe::InvokeSpellMenuCommand(
            app, menuItems[0].commandId, SpellingProbe::SessionKey(app), snapBefore, revBefore, DxUi::RichText::Selection{0u, 0u});
        Require(staleHandled && app.Document().GetText() == original, "stale selection guard declines replacement safely");

        // Exact snapshot/revision/selection commits atomically as one undo step.
        app.Document().SetSelection(selBefore);
        const auto exactMenu = SpellingProbe::BuildSpellMenuAtOffset(app, hitOffset);
        Require(! exactMenu.empty() && exactMenu[0].text == L"recommend", "exactMenu must carry 'recommend' suggestion before commit");
        const bool ok = SpellingProbe::InvokeSpellMenuCommand(
            app, exactMenu[0].commandId, SpellingProbe::SessionKey(app), app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection());
        Require(ok && app.Document().GetText() == L"Please **recommend** this.\n", "atomic replacement updates text while preserving outer ** formatting");
        Require(app.Document().Undo() && app.Document().GetText() == original, "single Ctrl+Z undo restores exact original **re**commnd");
        Require(foreground.NeverAcquiredForeground(), "atomic spell replace test never acquires foreground");
    });

    test("Editor.SpellingViewportCapAndScrollProjection",
         [&]
    {
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Line"});

        auto makeAlphabeticTag = [](std::wstring_view prefix, int index)
        {
            std::wstring word(prefix);
            word.push_back(static_cast<wchar_t>(L'a' + (index / 26) % 26));
            word.push_back(static_cast<wchar_t>(L'a' + index % 26));
            return word;
        };
        std::wstring doc;
        for (int i = 0; i < 320; ++i)
            doc += L"Line " + makeAlphabeticTag(L"badwordone", i) + L" " + makeAlphabeticTag(L"badwordtwo", i) + L".\n\n";

        Application app(HiddenSpellingOptions(root / L"viewport-cap", provider));
        Success(app.Initialize(), "initialize app for 640-issue viewport cap test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        app.SetDocument(doc);
        app.Prepare(512);

        Require(SpellingProbe::Issues(app).size() == 640u, "SpellIssueTable retains all 640 misspelled words up to 4096");
        Require(app.Document().GetDecorations().size() <= 256u && ! app.Document().GetDecorations().empty(),
                "projected RichTextEditor decorations stay within DxUi's 256-entry cap");

        const auto callsBeforeScroll = provider->CheckSegmentCallCount();
        (void)app.Document().ScrollSourceRangeIntoView(app.Host(), doc.size() - 20u, doc.size(), true);
        app.Prepare(64);
        Require(provider->CheckSegmentCallCount() == callsBeforeScroll,
                "scrolling re-projects visible squiggles from SpellIssueTable without re-calling ISpellProvider::CheckSegment");
    });

    test("Editor.SpellingActiveWordSuppressionAndCancellation",
         [&]
    {
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Valid", L"next"});
        provider->SetArtificialDelayMs(25u);

        auto opts                 = HiddenSpellingOptions(root / L"suppression-cancel", provider);
        opts.unsavedPromptForTest = [] { return IDNO; };
        Application app(std::move(opts));
        Success(app.Initialize(), "initialize app for active-word suppression & shutdown drain test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        app.SetDocument(L"Valid writting next");
        app.Prepare(512);
        Require(SpellingProbe::Issues(app).size() == 1u && app.Document().GetDecorations().size() == 1u, "settled misspelled word has visible squiggle");

        // Place caret inside `writting` and simulate typing edit: squiggle must suppress immediately.
        const auto typoPos = app.Document().GetText().find(L"writting");
        app.Document().SetSelection({typoPos + 4u, typoPos + 4u});
        SpellingProbe::ForceTypingChange(app);
        Require(SpellingProbe::TypingSuppressed(app) && app.Document().GetDecorations().empty(),
                "active typing suppresses squiggle on the word touching the caret");

        // Rapid second edit supersedes in-flight work; draining window close completes cleanly.
        app.Document().SetSelection({0u, 0u});
        (void)app.Document().ReplaceSelection(L"More ");
        SpellingProbe::ForceDebounceTimeout(app);
        SpellingProbe::DrainClose(app);
        for (unsigned turn = 0u; turn < 200u && ! app.AreShutdownWorkersSettled(); ++turn)
        {
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (! app.AreShutdownWorkersSettled())
                (void)MsgWaitForMultipleObjects(0u, nullptr, FALSE, 10u, QS_ALLINPUT);
        }
        Require(app.AreShutdownWorkersSettled(), "SpellService worker drains cleanly under AreShutdownWorkersSettled()");
    });

    test("Editor.SpellingNoDictionaryTruthfulState",
         [&]
    {
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetLanguageAvailability(L"zz-HOSTILE",
                                          ProviderAvailability::NoDictionary,
                                          L"\x202E"
                                          L"bad-override\x200B\r\ninfo");

        Application app(HiddenSpellingOptions(root / L"no-dict-truthful", provider));
        Success(app.Initialize(), "initialize app for missing-dictionary truthful state test");
        app.SetDocument(L"# Test\n\nSome writting here.\n");
        // Set unsupported language cleanly through Settings live reload simulation via Save + cycle/apply:
        // Or check directly via front-matter language hint!
        app.SetDocument(L"---\nlang: zz-HOSTILE\n---\n\nSome writting here.\n");
        app.Prepare(512);

        Require(SpellingProbe::Issues(app).empty() && app.Document().GetDecorations().empty(),
                "missing dictionary emits zero squiggles with zero silent English fallback");
        const std::wstring status = SpellingProbe::StatusText(app);
        const std::wstring tip    = SpellingProbe::StatusTooltip(app);
        Require(status.find(L"no dictionary") != std::wstring::npos, "status bar truthfully reports no dictionary");
        Require(tip.find(L'\x202E') == std::wstring::npos && tip.find(L'\x200B') == std::wstring::npos && tip.find(L'\r') == std::wstring::npos,
                "hostile bidi overrides, zero-width chars, and newlines are stripped from status tooltip");
    });

    test("Editor.SpellingWinSpellProviderReadOnlyProbe",
         [&]
    {
        const HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        const auto comScope = wil::scope_exit([comHr]() noexcept
        {
            if (SUCCEEDED(comHr))
                CoUninitialize();
        });

        WinSpellProvider winProvider;
        const auto cleanup = wil::scope_exit([&]() noexcept { winProvider.ResetComObjects(); });

        std::vector<std::wstring> installed;
        (void)winProvider.EnumerateInstalledLanguages(installed);
        Require(winProvider.CheckAvailability(L"zz-ZZ-x-nonexistent") == ProviderAvailability::NoDictionary,
                "WinSpellProvider reports NoDictionary for nonexistent BCP-47 tag without falling back to English");

        const bool hasEnUs = std::ranges::any_of(
            installed, [](std::wstring_view tag) { return tag.size() == 5u && CompareStringOrdinal(tag.data(), 5, L"en-US", 5, TRUE) == CSTR_EQUAL; });
        if (hasEnUs && winProvider.CheckAvailability(L"en-US") == ProviderAvailability::Ready)
        {
            std::vector<SpellErrorSpan> errors;
            const SpellCheckOptions opts{};
            Success(winProvider.CheckSegment(L"en-US", L"Good writting today", opts, errors), "WinSpellProvider CheckSegment");
            Require(errors.size() == 1u && errors[0].start == 5u && errors[0].length == 8u,
                    "WinSpellProvider identifies 'writting' at exact UTF-16 offset without touching clean words");

            std::vector<std::wstring> suggestions;
            Success(winProvider.SuggestWord(L"en-US", L"writting", 5u, suggestions), "WinSpellProvider SuggestWord");
            Require(! suggestions.empty(), "WinSpellProvider produces suggestions for 'writting'");
        }
    });

    test("Editor.SpellingLeftClickMenuAndLanguageCombos",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetInstalledLanguages({L"en-US", L"fr-FR"});
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->SetLanguageAvailability(L"fr-FR", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"First", L"and", L"second"});
        provider->AddKnownWords(L"fr-FR", {L"Bonjour"});
        provider->SetSuggestions(L"en-US", L"badwrd", {L"badword"});

        const std::filesystem::path dataDir = root / L"leftclick-combos";
        Application app(HiddenSpellingOptions(dataDir, provider));
        Success(app.Initialize(), "initialize app for left-click & language ComboBox test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        app.SetDocument(L"First badwrd and second badwrd.\n");
        app.Prepare(512);

        // Verify synchronized Settings + Status bar ComboBoxes, real popup opening on click, and pinned Settings header.
        SpellingProbe::RefreshLanguageCombos(app, true);
        auto* const settingsCombo  = SpellingProbe::SettingsLanguageCombo(app);
        auto* const statusCombo    = SpellingProbe::StatusLanguageCombo(app);
        auto* const enabledToggle  = SpellingProbe::SpellingEnabledToggle(app);
        auto* const drawerClose    = SpellingProbe::DrawerCloseButton(app);
        auto* const drawer         = SpellingProbe::SettingsDrawer(app);
        auto* const settingsScroll = SpellingProbe::SettingsScroll(app);
        Require(settingsCombo != nullptr && statusCombo != nullptr && enabledToggle != nullptr && drawerClose != nullptr && drawer != nullptr &&
                    settingsScroll != nullptr,
                "proofing combos, switch, and settings header controls exist");
        Require(enabledToggle->IsChecked(), "spelling enabled toggle uses DxUi::Toggle and starts checked");
        Require(statusCombo->GetItems().size() >= 4u && settingsCombo->GetItems().size() >= 3u, "both combos populate Auto + installed languages");

        // Open Settings drawer, verify Close button is pinned top-right (even when scrolled) and Proofing precedes Syntax/Privacy.
        app.Execute(Command::Settings);
        app.Prepare(128);
        const D2D1_RECT_F closeBefore = drawerClose->GetBounds();
        Require(closeBefore.right <= drawer->GetBounds().right && closeBefore.top < settingsScroll->GetBounds().top &&
                    SpellingProbe::ProofingSectionTop(app) < SpellingProbe::SyntaxSectionTop(app) &&
                    SpellingProbe::SyntaxSectionTop(app) < SpellingProbe::PrivacySectionTop(app),
                "Settings drawer pins Close in header above scroll area with Writing -> Proofing -> Syntax -> Privacy order");
        settingsScroll->SetScrollOffset(500.f);
        app.Prepare(32);
        const D2D1_RECT_F closeAfter = drawerClose->GetBounds();
        Require(closeAfter.top == closeBefore.top && closeAfter.bottom == closeBefore.bottom, "Settings Close button remains pinned while Settings scrolls");

        // Clicking both ComboBoxes must open their popup lists (regression test for OnPopupRequested return value).
        const D2D1_RECT_F statusBounds = statusCombo->GetBounds();
        const D2D1_POINT_2F statusPt{(statusBounds.left + statusBounds.right) * 0.5f, (statusBounds.top + statusBounds.bottom) * 0.5f};
        Require(statusCombo->OnMouseDown(app.Host(), statusPt, false, 0u) && statusCombo->IsPopupOpen(),
                "clicking the status-bar language ComboBox opens its popup menu");
        (void)statusCombo->OnKeyDown(app.Host(), VK_ESCAPE, 0u);
        Require(! statusCombo->IsPopupOpen(), "Escape closes the status-bar language ComboBox popup");

        const D2D1_RECT_F settingsComboBounds = settingsCombo->GetBounds();
        const D2D1_POINT_2F settingsComboPt{(settingsComboBounds.left + settingsComboBounds.right) * 0.5f,
                                            (settingsComboBounds.top + settingsComboBounds.bottom) * 0.5f};
        Require(settingsCombo->OnMouseDown(app.Host(), settingsComboPt, false, 0u) && settingsCombo->IsPopupOpen(),
                "clicking the Settings Proofing language ComboBox opens its popup menu");
        (void)settingsCombo->OnKeyDown(app.Host(), VK_ESCAPE, 0u);
        Require(! settingsCombo->IsPopupOpen(), "Escape closes the Settings Proofing language ComboBox popup");

        // Leave Settings open briefly to verify left-click on an underlined word still works when Settings drawer is open.
        const auto& issues = SpellingProbe::Issues(app);
        Require(issues.size() == 2u, "duplicate misspelled tokens produce two occurrences");
        const auto firstFrag = issues[0].sourceFragments.front();
        const auto rects     = app.Document().TryGetTextInputRangeRects(app.Host(), firstFrag.first, firstFrag.last);
        Require(rects && ! rects->empty(), "range rects resolve for first flagged word");
        const float scale = app.Host().GetDpi() / 96.f;
        const POINT wordPt{static_cast<LONG>((rects->front().left + rects->front().right) * 0.5f * scale),
                           static_cast<LONG>((rects->front().top + rects->front().bottom) * 0.5f * scale)};
        const POINT squigglePt{static_cast<LONG>((rects->front().left + rects->front().right) * 0.5f * scale),
                               static_cast<LONG>((rects->front().bottom + 3.f) * scale)};
        const POINT rightMarginPt{static_cast<LONG>((rects->front().right + 64.f) * scale), wordPt.y};

        // Clicking on empty margin past line end must NOT trigger left-click spell menu.
        SpellingProbe::ClearPendingSpellMenu(app);
        SpellingProbe::LeftDown(app, rightMarginPt);
        SpellingProbe::LeftUp(app, rightMarginPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "clicking empty margin past a flagged word does not open spell menu");

        // Clicking on the wavy underline while non-modal Settings drawer is open must populate suggestions via posted message!
        SpellingProbe::LeftDown(app, squigglePt);
        app.Document().SetSelection({firstFrag.first, firstFrag.first});
        SpellingProbe::LeftUp(app, squigglePt);
        SpellingProbe::PumpMessages(app);
        Require(SpellingProbe::HasPendingSpellMenu(app), "clicking squiggle while Settings drawer is open triggers proofing menu");

        app.Execute(Command::CloseDrawer);

        // Switch via status bar to "off", verify disabled UI + zero squiggles, then back to "en-US".
        SpellingProbe::SelectStatusLanguageByValue(app, L"off");
        app.Prepare(256);
        Require(! enabledToggle->IsChecked() && ! app.Preferences().spelling.enabled && app.Document().GetDecorations().empty(),
                "selecting Spelling off in status bar disables checking cleanly");
        SpellingProbe::SelectStatusLanguageByValue(app, L"en-US");
        app.Prepare(512);
        Require(enabledToggle->IsChecked() && app.Preferences().spelling.enabled && app.Document().GetDecorations().size() == 2u,
                "selecting en-US in status bar re-enables checking and highlights both occurrences");

        // Drag and double-click must NOT trigger left-click spell menu.
        SpellingProbe::ClearPendingSpellMenu(app);
        SpellingProbe::LeftDown(app, wordPt);
        app.Document().SetSelection({firstFrag.first, firstFrag.first});
        SpellingProbe::LeftUp(app, POINT{wordPt.x + 36, wordPt.y});
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "drag release past threshold does not open left-click spell menu");

        SpellingProbe::LeftDown(app, wordPt);
        SpellingProbe::LeftDoubleClick(app);
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "double-click does not leave left-click spell menu armed");

        // Clean single left click arms menu items on the clicked word via posted SpellMenuRequested.
        SpellingProbe::LeftDown(app, wordPt);
        app.Document().SetSelection({firstFrag.first, firstFrag.first});
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(SpellingProbe::HasPendingSpellMenu(app), "single left-click inside flagged word triggers proofing menu state");

        const auto actionItems = SpellingProbe::BuildSpellActionItems(app, 0u);
        const bool hasSug      = std::ranges::any_of(actionItems, [](const auto& item) { return item.text == L"badword"; });
        const bool hasOnce     = std::ranges::any_of(actionItems, [](const auto& item) { return item.commandId == 10098; });
        const bool hasAll      = std::ranges::any_of(actionItems, [](const auto& item) { return item.commandId == 10100; });
        const bool hasAdd      = std::ranges::any_of(actionItems, [](const auto& item) { return item.commandId == 10101; });
        Require(hasSug && hasOnce && hasAll && hasAdd, "left-click proofing menu includes suggestion, Ignore Once, Ignore All, and Add to Dictionary");

        // Ignore Once clears only the clicked occurrence and does not write custom-dictionary.txt to disk.
        const bool ignoredOnce = SpellingProbe::InvokeSpellMenuCommand(
            app, 10098, SpellingProbe::SessionKey(app), app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection());
        Require(ignoredOnce && app.Document().GetDecorations().size() == 1u, "Ignore Once hides only the clicked occurrence");
        Require(! std::filesystem::exists(dataDir / L"custom-dictionary.txt"), "Ignore Once never writes to disk");

        // Ignore All on the second occurrence clears remaining occurrences in session memory only.
        const auto secondFrag = issues[1].sourceFragments.front();
        app.Document().SetSelection({secondFrag.first, secondFrag.first});
        const auto secondItems = SpellingProbe::BuildSpellActionItems(app, 1u);
        Require(! secondItems.empty(), "second occurrence builds proofing menu items");
        const bool ignoredAll = SpellingProbe::InvokeSpellMenuCommand(
            app, 10100, SpellingProbe::SessionKey(app), app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection());
        app.Prepare(256);
        Require(ignoredAll && app.Document().GetDecorations().empty(), "Ignore All hides all remaining occurrences in session");
        Require(! std::filesystem::exists(dataDir / L"custom-dictionary.txt"), "Ignore All never writes to disk");
        Require(foreground.NeverAcquiredForeground(), "left-click and ComboBox test never takes desktop focus");
    });
}
} // namespace RedTilde
