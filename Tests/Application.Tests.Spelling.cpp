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

    [[nodiscard]] static StatusLanguageComboBox* StatusLanguageCombo(const Application& app) noexcept
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
        return app.BuildSpellActionMenuItems(issueIndex, app.ActiveEditor()->GetSelection());
    }

    [[nodiscard]] static bool TypingSuppressed(const Application& app) noexcept
    {
        return app._spell.typingSuppressionActive;
    }

    [[nodiscard]] static std::vector<DxUi::MenuFlyoutItem> BuildSpellMenuAtOffset(Application& app, std::size_t sourceOffset)
    {
        app.ActiveEditor()->SetSelection({sourceOffset, sourceOffset});
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

    [[nodiscard]] static Editor* Active(const Application& app) noexcept
    {
        return app.ActiveEditor();
    }

    [[nodiscard]] static Editor* Mirror(const Application& app) noexcept
    {
        return app.MirrorEditor();
    }

    static void SwitchSplitPane(Application& app, SplitActivePane target)
    {
        app.SwitchSplitActivePane(target, false);
    }

    static void ResetAppWorkspace(Application& app)
    {
        app.ResetWorkspace();
    }

    static void CloseActiveDoc(Application& app)
    {
        app.CloseActiveDocument();
    }

    static void SetFilePickerForTest(Application& app, std::function<std::filesystem::path(bool)> picker)
    {
        app._options.filePickerForTest = std::move(picker);
    }

    [[nodiscard]] static bool CheckPending(const Application& app) noexcept
    {
        return app._spell.checkPending;
    }

    [[nodiscard]] static bool ImeSuppressionActive(const Application& app) noexcept
    {
        return app._spell.imeSuppressionActive;
    }

    static void SetImeCompositionForTest(Application& app, bool active) noexcept
    {
        app._spell.imeCompositionOverrideForTest = active;
    }

    [[nodiscard]] static std::wstring StatusLabelText(const Application& app)
    {
        return app._feedback ? std::wstring(app._feedback->GetText()) : std::wstring{};
    }

    [[nodiscard]] static DxUi::RichText::MarkdownRange SuppressCaretWordRange(const Application& app)
    {
        return app._spell.suppressedTypingWord.value_or(DxUi::RichText::MarkdownRange{});
    }

    static void SetAvailabilityOverride(Application& app, ProviderAvailability avail) noexcept
    {
        app._spell.effectiveAvailability = avail;
    }

    [[nodiscard]] static ProviderAvailability EffectiveAvailability(const Application& app) noexcept
    {
        return app._spell.effectiveAvailability;
    }

    [[nodiscard]] static DxUi::TextField* BlockDraftInput(const Application& app) noexcept
    {
        return app._blockDraftText;
    }

    static void SetSpellDebounceTimer(Application& app)
    {
        if (app.Window())
            (void)SetTimer(app.Window(), Application::SpellDebounceTimerId, 10000u, nullptr);
    }

    static void OpenFileInWorkspace(Application& app, const std::filesystem::path& path)
    {
        app.OpenPath(path);
    }

    [[nodiscard]] static UINT_PTR DebounceTimerId() noexcept
    {
        return Application::SpellDebounceTimerId;
    }

    static void UpdateSuppressionFromCaret(Application& app)
    {
        app.UpdateSpellSuppressionFromCaret();
    }

    static void TriggerSelectionChanged(Application& app)
    {
        app.OnSpellSelectionChanged();
    }

    static void BumpLanguageConfigVersion(Application& app) noexcept
    {
        ++app._spell.languageConfigVersion;
    }

    [[nodiscard]] static bool HasPendingLeftClick(const Application& app) noexcept
    {
        return app._spell.pendingLeftClick.has_value();
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

        // P1-16: ignoreInternetAndPaths = false exposes URL/email/Windows/UNC paths in both extractors and live app checks.
        const std::wstring mdNet = L"Visit https://example.invalid/writting_url or email writer@example.invalid or path C:\\Temp\\badfile\\note.md or "
                                   L"\\\\\\\\server\\share\\uncbadfile.md or <https://autolink.invalid/autolinkbad>.\n";
        app.SetDocument(mdNet);
        app.Prepare(512);
        const auto netSemantic = app.Document().GetSemanticDocument();
        const auto netSyntax   = app.Document().GetMarkdownSyntax();
        Require(netSemantic != nullptr && netSyntax != nullptr, "net semantic and syntax prepared");
        Require(SpellingProbe::Issues(app).empty(), "URL/email/Windows/UNC paths are ignored when ignoreInternetAndPaths is true");

        const auto semUnmasked = CombineChunkText(ExtractSemanticProse(*netSemantic, netSyntax.get(), mdNet, false));
        Require(semUnmasked.find(L"writting_url") != std::wstring::npos && semUnmasked.find(L"uncbadfile") != std::wstring::npos &&
                    semUnmasked.find(L"autolinkbad") != std::wstring::npos,
                "ExtractSemanticProse(..., false) retains URL, UNC path, and autolink tokens");
        const auto srcUnmasked = CombineChunkText(ExtractSourceProse(*netSyntax, mdNet, false));
        Require(srcUnmasked.find(L"writting_url") != std::wstring::npos && srcUnmasked.find(L"uncbadfile") != std::wstring::npos &&
                    srcUnmasked.find(L"autolinkbad") != std::wstring::npos,
                "ExtractSourceProse(..., false) retains URL, UNC path, and autolink tokens");

        app.Execute(Command::SpellIgnoreInternetPaths);
        app.Prepare(512);
        Require(! app.Preferences().spelling.ignoreInternetAndPaths && ! SpellingProbe::Issues(app).empty(),
                "toggling SpellIgnoreInternetPaths off flags tokens inside URLs/paths");
        app.Execute(Command::SpellIgnoreInternetPaths);
        app.Prepare(512);
        Require(app.Preferences().spelling.ignoreInternetAndPaths && SpellingProbe::Issues(app).empty(),
                "toggling SpellIgnoreInternetPaths back on suppresses URL/path issues");
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

        // 4. Junction / reparse point refused fail-closed, preserving previously loaded valid dictionary.
        {
            const auto targetDir = dir / L"reparse-target";
            std::filesystem::create_directories(targetDir);
            const auto realFile = targetDir / L"real.dic";
            Success(WriteBytesAtomic(realFile, "Alpha\n"), "seed real target behind junction");
            const auto linkDir = dir / L"reparse-link";
            Success(Testing::CreateFixtureJunction(dir, linkDir, targetDir), "create fixture junction");
            const auto removeJunction = wil::scope_exit([&] { (void)RemoveDirectoryW(linkDir.c_str()); });

            CustomDictionary guardedDict;
            Success(guardedDict.Load(wordDicPath), "preload valid dictionary before junction load attempt");
            const std::size_t priorCount = guardedDict.WordCount();

            Require(FAILED(guardedDict.Load(linkDir)), "CustomDictionary::Load refuses junction directory itself");
            Require(FAILED(guardedDict.Load(linkDir / L"real.dic")), "CustomDictionary::Load refuses existing file behind junction");
            Require(FAILED(guardedDict.Load(linkDir / L"missing.dic")), "CustomDictionary::Load refuses missing file behind junction");
            Require(FAILED(guardedDict.Load(linkDir / L"nested" / L"missing.dic")), "CustomDictionary::Load refuses nested missing file behind junction");
            Require(! std::filesystem::exists(targetDir / L"missing.dic") && ! std::filesystem::exists(targetDir / L"nested"),
                    "junction refusal never creates files or directories through junction");
            Require(SUCCEEDED(guardedDict.LoadStatus()) && guardedDict.Path() == wordDicPath && guardedDict.WordCount() == priorCount &&
                        guardedDict.IsAccepted(L"redtilde") && guardedDict.IsAccepted(L"MyCustomTerm"),
                    "failed Load to different path preserves previously loaded dictionary state and LoadStatus");

            HRESULT restoreDiskHr = S_OK;
            Success(guardedDict.AddWord(L"AfterJunctionFail", &restoreDiskHr), "prior dictionary remains writable after failed switch");
            Success(restoreDiskHr, "disk persistence still succeeds on preserved valid dictionary");

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

        // 4b. Forward-slash same-file reload failure updates LoadStatus() and embedded-NUL path returns E_INVALIDARG.
        {
            const auto slashPath = dir / L"slash-same.dic";
            Success(WriteBytesAtomic(slashPath, "Alpha\n"), "seed valid dictionary for mixed-slash reload test");
            CustomDictionary slashDict;
            Success(slashDict.Load(slashPath), "load valid dictionary with backslashes");
            Require(SUCCEEDED(slashDict.LoadStatus()), "initial backslash LoadStatus is S_OK");

            Success(WriteBytesAtomic(slashPath, "bad\xE9\n"), "corrupt same dictionary file on disk");
            std::wstring fwdStr = slashPath.native();
            std::ranges::replace(fwdStr, L'\\', L'/');
            const HRESULT reloadHr = slashDict.Load(std::filesystem::path(fwdStr));
            Require(reloadHr == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) && FAILED(slashDict.LoadStatus()) && slashDict.LoadStatus() == reloadHr,
                    "forward-slash reload of corrupted same file updates LoadStatus to FAILED");

            const std::filesystem::path nulDictPath(slashPath.native() + std::wstring(L"\0evil", 5u));
            Require(slashDict.Load(nulDictPath) == E_INVALIDARG, "CustomDictionary::Load rejects embedded-NUL path with E_INVALIDARG");
        }

        // 5. >64 KiB UTF-8 round-trip, >4 MiB cap refusal, ERROR_PATH_NOT_FOUND fresh AddWord, &_stamp conflict, BOM-less UTF-16LE vs UTF-8, NFC,
        // AcceptedLookupScope.
        {
            const auto largePath = dir / L"large-utf8.dic";
            std::string largeBytes;
            largeBytes.reserve(130000u);
            for (int idx = 0; idx < 8200; ++idx)
                largeBytes += "LargeWord" + std::to_string(idx) + "\n";
            Require(largeBytes.size() > 65536u && largeBytes.size() < CustomDictionary::MaximumFileBytes, "large UTF-8 fixture exceeds 64 KiB");
            Success(WriteBytesAtomic(largePath, largeBytes, nullptr, nullptr, false, nullptr, false, nullptr, CustomDictionary::MaximumFileBytes),
                    "seed >64 KiB UTF-8 dictionary");

            CustomDictionary largeDict;
            Success(largeDict.Load(largePath), "load >64 KiB UTF-8 dictionary");
            Require(largeDict.Encoding() == DictionaryEncoding::Utf8 && largeDict.WordCount() == 8200u && largeDict.IsAccepted(L"LargeWord8199"),
                    ">64 KiB UTF-8 dictionary decoded and indexed");
            HRESULT largeDiskHr = S_OK;
            Success(largeDict.AddWord(L"LargeWordExtra", &largeDiskHr), "AddWord on >64 KiB UTF-8 dictionary");
            Success(largeDiskHr, "persist >64 KiB UTF-8 dictionary without 64 KiB codec cap");

            CustomDictionary largeReloaded;
            Success(largeReloaded.Load(largePath), "reload >64 KiB UTF-8 dictionary");
            Require(largeReloaded.WordCount() == 8201u && largeReloaded.IsAccepted(L"LargeWordExtra"), ">64 KiB UTF-8 round-trip preserved all words");

            const auto oversizedPath = dir / L"oversized.dic";
            const std::string oversized(CustomDictionary::MaximumFileBytes + 1u, 'a');
            Success(WriteBytesAtomic(oversizedPath, oversized, nullptr, nullptr, false, nullptr, false, nullptr, CustomDictionary::MaximumFileBytes + 16u),
                    "seed >4 MiB dictionary fixture");
            CustomDictionary tooBigDict;
            Require(tooBigDict.Load(oversizedPath) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE), "dictionary exceeding 4 MiB is refused");

            // ERROR_PATH_NOT_FOUND on missing parent directory yields fresh S_OK dictionary, and AddWord creates dirs and persists.
            const auto freshNested = dir / L"missing-sub" / L"sub2" / L"custom-dictionary.txt";
            CustomDictionary freshDict;
            Success(freshDict.Load(freshNested), "Load on non-existent parent path succeeds as fresh empty dictionary");
            Require(SUCCEEDED(freshDict.LoadStatus()) && freshDict.WordCount() == 0u, "fresh empty dictionary starts with S_OK LoadStatus");
            HRESULT freshDiskHr = S_OK;
            Success(freshDict.AddWord(L"FreshSubdirWord", &freshDiskHr), "AddWord on fresh nested dictionary");
            Success(freshDiskHr, "AddWord creates missing parent directories safely and persists");

            // Concurrent external creation on previously empty path triggers ERROR_REVISION_MISMATCH via unconditional &_stamp.
            const auto racePath = dir / L"concurrent-create.dic";
            CustomDictionary raceDict;
            Success(raceDict.Load(racePath), "load missing dictionary path before concurrent write");
            Success(WriteBytesAtomic(racePath, "External\n"), "external writer creates dictionary concurrently");
            HRESULT raceDiskHr = S_OK;
            Success(raceDict.AddWord(L"LocalRaceWord", &raceDiskHr), "AddWord retains word in memory on concurrent create conflict");
            Require(raceDiskHr == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
                    "unconditional &_stamp rejects overwriting concurrently created dictionary file");
            std::string diskRaceBytes;
            Success(ReadBytes(racePath, diskRaceBytes), "read externally created file after refusal");
            Require(diskRaceBytes == "External\n", "externally created dictionary file preserved intact");

            // Strict UTF-8 vs BOM-less UTF-16LE (Cyrillic + U+0100 + U+4E00) and expanding NFC normalization.
            const auto leNoBomTest = [&](std::wstring_view content, const wchar_t* lookup, const wchar_t* fileName)
            {
                const auto p = dir / fileName;
                std::string leBytes;
                for (const wchar_t ch : content)
                {
                    leBytes.push_back(static_cast<char>(static_cast<unsigned>(ch) & 0xFFu));
                    leBytes.push_back(static_cast<char>((static_cast<unsigned>(ch) >> 8u) & 0xFFu));
                }
                Success(WriteBytesAtomic(p, leBytes), "seed BOM-less UTF-16LE dictionary");
                CustomDictionary d;
                Success(d.Load(p), "load BOM-less UTF-16LE dictionary");
                Require(d.Encoding() == DictionaryEncoding::Utf16LeNoBom && d.IsAccepted(lookup),
                        "BOM-less UTF-16LE detected and indexed across scripts/U+xx00");
            };
            leNoBomTest(L"\x041F\x0440\x0438\x0432\x0435\x0442\n", L"\x043F\x0440\x0438\x0432\x0435\x0442", L"le-cyrillic.dic");
            leNoBomTest(L"\x0100\n", L"\x0101", L"le-u0100.dic");
            leNoBomTest(L"\x4E00\n", L"\x4E00", L"le-cjk.dic");

            const auto utf8NonLatin = dir / L"utf8-cyrillic.dic";
            Success(WriteBytesAtomic(utf8NonLatin, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\n\xC4\x80\n"), "seed UTF-8 non-Latin dictionary");
            CustomDictionary utf8Dict;
            Success(utf8Dict.Load(utf8NonLatin), "load strict UTF-8 non-Latin dictionary");
            Require(utf8Dict.Encoding() == DictionaryEncoding::Utf8 && utf8Dict.IsAccepted(L"\x043F\x0440\x0438\x0432\x0435\x0442") &&
                        utf8Dict.IsAccepted(L"\x0101"),
                    "strict UTF-8 tried before BOM-less UTF-16LE heuristic");

            std::wstring nfcExpanded;
            Require(TryNormalizeUnicodeNfc(L"\x0F73", nfcExpanded) && nfcExpanded.size() == 2u,
                    "TryNormalizeUnicodeNfc handles expanding NFC normalization via buffer retry");
            std::wstring normalizedKey;
            Require(TryNormalizeDictionaryKey(L"Cafe\x0301", normalizedKey), "TryNormalizeDictionaryKey succeeds on combining mark");

            // AcceptedLookupScope batch lookup.
            utf8Dict.IgnoreAll(L"BatchIgnoreAll");
            utf8Dict.IgnoreOnce(7u, L"BatchOnce", 55u);
            {
                const CustomDictionary::AcceptedLookupScope scope(utf8Dict);
                Require(scope.IsAcceptedNormalized(L"\x0101", 7u, 10u), "AcceptedLookupScope accepts custom word");
                Require(scope.IsAcceptedNormalized(L"batchignoreall", 7u, 10u), "AcceptedLookupScope accepts IgnoreAll word");
                Require(! scope.IsAcceptedNormalized(L"batchignoreall", 7u, 10u, true),
                        "ignoreOnceOnly skips customWords and ignoredAll for repeated-word checks");
                Require(scope.IsAcceptedNormalized(L"batchonce", 7u, 55u, true) && ! scope.IsAcceptedNormalized(L"batchonce", 7u, 99u, true),
                        "AcceptedLookupScope checks workspace-scoped IgnoreOnce");
            }
        }

        // 6. Read-only failure retains session word + Ignore Once vs Ignore All + word length cap.
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

        // Boundary cases for UpdateSpellSuppressionFromCaret: len == 0, caret == 0, caret == len, >128-char word, non-collapsed selection, Split+Source
        // _mirror.
        app.SetDocument(L"");
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(! SpellingProbe::TypingSuppressed(app), "empty document clears typing suppression");

        app.SetDocument(L"writting next");
        app.Document().SetSelection({0u, 0u});
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(SpellingProbe::TypingSuppressed(app) && SpellingProbe::SuppressCaretWordRange(app) == DxUi::RichText::MarkdownRange{0u, 8u},
                "caret == 0 suppresses leading word");

        app.SetDocument(L" next");
        app.Document().SetSelection({0u, 0u});
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(! SpellingProbe::TypingSuppressed(app), "caret == 0 on whitespace does not suppress");

        app.SetDocument(L"Valid writting");
        app.Document().SetSelection({14u, 14u});
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(SpellingProbe::TypingSuppressed(app) && SpellingProbe::SuppressCaretWordRange(app) == DxUi::RichText::MarkdownRange{6u, 14u},
                "caret == len suppresses trailing word");

        app.SetDocument(L"Valid ");
        app.Document().SetSelection({6u, 6u});
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(! SpellingProbe::TypingSuppressed(app), "caret == len after trailing space does not suppress");

        const std::wstring longBad(200u, L'x');
        app.SetDocument(longBad);
        app.Document().SetSelection({100u, 100u});
        SpellingProbe::UpdateSuppressionFromCaret(app);
        Require(SpellingProbe::TypingSuppressed(app) && SpellingProbe::SuppressCaretWordRange(app) == DxUi::RichText::MarkdownRange{0u, 200u},
                ">128-char word doubles window radius and captures entire word");

        app.Document().SetSelection({10u, 20u});
        SpellingProbe::TriggerSelectionChanged(app);
        Require(! SpellingProbe::TypingSuppressed(app), "non-collapsed selection clears typing suppression");

        // Split+Source (_mirror) typing suppression.
        app.SetDocument(L"Valid writting next");
        app.Prepare(512);
        app.Execute(Command::Split);
        app.Prepare(256);
        SpellingProbe::SwitchSplitPane(app, SplitActivePane::Source);
        app.Prepare(512);
        auto* const splitMirror = SpellingProbe::Active(app);
        Require(app.ActiveSplitPane() == SplitActivePane::Source && splitMirror != SpellingProbe::Mirror(app), "Split+Source switches ActiveEditor to _mirror");
        Require(splitMirror->GetViewMode() == DxUi::RichText::ViewMode::Source, "Split+Source mirror is in Source view mode");
        Require(SpellingProbe::Issues(app).size() == 1u, "Split+Source mirror resolves 1 spelling issue");
        Require(splitMirror->GetDecorations().size() == 1u, "Split+Source mirror settles with squiggle before typing suppression");
        const auto mirrorTypoPos = splitMirror->GetText().find(L"writting");
        splitMirror->SetSelection({mirrorTypoPos + 4u, mirrorTypoPos + 4u});
        SpellingProbe::ForceTypingChange(app);
        Require(SpellingProbe::TypingSuppressed(app) && splitMirror->GetDecorations().empty(),
                "Split+Source mirror typing suppression clears active word squiggle");
        SpellingProbe::SwitchSplitPane(app, SplitActivePane::Compose);
        app.Prepare(256);
        app.Execute(Command::Compose);
        app.Prepare(512);

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

        // P1-19: ProviderAvailability::Unavailable propagates cleanly and clears squiggles, and SuggestionCache preserves word casing.
        provider->SetLanguageAvailability(L"zz-UNAVAIL", ProviderAvailability::Unavailable, L"Service offline");
        app.SetDocument(L"---\nlang: zz-UNAVAIL\n---\n\nSome writting here.\n");
        app.Prepare(512);
        Require(SpellingProbe::Issues(app).empty() && app.Document().GetDecorations().empty() &&
                    SpellingProbe::EffectiveAvailability(app) == ProviderAvailability::Unavailable &&
                    SpellingProbe::StatusText(app).find(L"unavailable") != std::wstring::npos,
                "ProviderAvailability::Unavailable surfaces truthful unavailable status and zero squiggles");

        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->SetSuggestions(L"en-US", L"Writting", {L"Writing"});
        provider->SetSuggestions(L"en-US", L"writting", {L"writing"});
        std::vector<std::wstring> upperSug;
        std::vector<std::wstring> lowerSug;
        Success(SpellingProbe::Service(app).SuggestSyncBounded({L"en-US"}, L"Writting", 1u, 5u, upperSug, 500u), "suggest capitalized Writting");
        Success(SpellingProbe::Service(app).SuggestSyncBounded({L"en-US"}, L"writting", 1u, 5u, lowerSug, 500u), "suggest lowercase writting");
        Require(! upperSug.empty() && upperSug[0] == L"Writing" && ! lowerSug.empty() && lowerSug[0] == L"writing",
                "SuggestionCache preserves original word casing between 'Writting' and 'writting'");

        // Suggestion text sanitization strips hostile bidi overrides, zero-width chars, and newlines.
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});
        provider->AddKnownWords(L"en-US", {L"Some", L"here"});
        provider->SetSuggestions(L"en-US",
                                 L"writting",
                                 {L"\x202E"
                                  L"writing\x200B\r\n",
                                  L"bad\x202E"
                                  L"word\x200B\r\nsafe"});
        app.SetDocument(L"Some writting here.\n");
        app.Prepare(512);
        Require(SpellingProbe::Issues(app).size() == 1u, "live doc flags 'writting' for suggestion sanitization check");
        const auto sugItems = SpellingProbe::BuildSpellMenuAtOffset(app, SpellingProbe::Issues(app)[0].sourceFragments.front().first);
        Require(sugItems.size() >= 2u && sugItems[0].text == L"writing" && sugItems[1].text == L"badword safe",
                "menu suggestion texts strip \\x202E, \\x200B, and collapse \\r\\n");
        for (const auto& item : sugItems)
        {
            Require(item.text.find(L'\x202E') == std::wstring::npos && item.text.find(L'\x200B') == std::wstring::npos &&
                        item.text.find(L'\r') == std::wstring::npos && item.text.find(L'\n') == std::wstring::npos,
                    "sanitized menu items contain no hostile bidi/zero-width/newline characters");
        }
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

            std::vector<std::wstring> partialSuggestions;
            Success(winProvider.SuggestWord(L"en-US", L"teh foo", 5u, partialSuggestions), "WinSpellProvider SuggestWord partial replace guard");
            Require(std::ranges::find(partialSuggestions, L"the") == partialSuggestions.end(),
                    "CORRECTIVE_ACTION_REPLACE does not emit partial-span replacement 'the' for multi-word input");

            std::vector<SpellErrorSpan> repeatErrors;
            Success(winProvider.CheckSegment(L"en-US", L"the the today", opts, repeatErrors), "WinSpellProvider CheckSegment repeated word");
            for (const auto& span : repeatErrors)
                Require(span.kind == SpellIssueKind::RepeatedWord, "WinSpellProvider maps CORRECTIVE_ACTION_DELETE to RepeatedWord");
        }

        // Deterministic CORRECTIVE_ACTION_DELETE (RepeatedWord) menu, custom-dict non-exemption, and clean deletion + undo.
        {
            auto provider = std::make_shared<FakeSpellProvider>();
            provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
            provider->AddKnownWords(L"en-US", {L"Keep", L"Alpha", L"the", L"today"});
            provider->SetForcedSegmentSpans(L"en-US", L"Keep the the today.", {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}});

            Application app(HiddenSpellingOptions(root / L"repeated-word-delete", provider));
            Success(app.Initialize(), "initialize app for RepeatedWord test");
            SpellingProbe::SetSpellLanguages(app, {L"en-US"});
            Success(SpellingProbe::Dictionary(app).AddWord(L"the"), "add 'the' to custom dictionary to prove RepeatedWord is not exempted");
            const std::wstring original = L"Keep the the today.\n";
            app.SetDocument(original);
            app.Prepare(512);

            const auto& issues = SpellingProbe::Issues(app);
            Require(issues.size() == 1u && issues[0].kind == SpellIssueKind::RepeatedWord && issues[0].word == L"the",
                    "RepeatedWord issue survives custom dictionary and surfaces at second 'the'");

            const auto items    = SpellingProbe::BuildSpellMenuAtOffset(app, issues[0].sourceFragments[0].first);
            const bool hasDel   = std::ranges::any_of(items, [](const auto& item) { return item.commandId == 10097; });
            const bool hasOnce  = std::ranges::any_of(items, [](const auto& item) { return item.commandId == 10098; });
            const bool hasNoSug = std::ranges::any_of(items, [](const auto& item) { return item.commandId == 10099; });
            const bool hasAll   = std::ranges::any_of(items, [](const auto& item) { return item.commandId == 10100; });
            const bool hasAdd   = std::ranges::any_of(items, [](const auto& item) { return item.commandId == 10101; });
            Require(hasDel && hasOnce && ! hasNoSug && ! hasAll && ! hasAdd, "RepeatedWord menu shows only Delete repeated word + Ignore Once");

            const bool deleted = SpellingProbe::InvokeSpellMenuCommand(
                app, 10097, SpellingProbe::SessionKey(app), app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection());
            Require(deleted && app.Document().GetText() == L"Keep the today.\n", "Delete repeated word cleanly removes repeated word and one adjacent space");
            Require(app.Document().Undo() && app.Document().GetText() == original, "single undo restores repeated word cleanly");

            // SpellDeleteRepeatedId boundary cases: start of line after CRLF, end of line before CRLF, EOF, preceding \t, surrogate neighbor,
            // sourceFragments.size() > 1, and Split+Source _mirror.
            const auto runRepeatCase = [&](const std::wstring& doc,
                                           std::wstring_view semanticKey,
                                           const std::vector<SpellErrorSpan>& spans,
                                           std::wstring_view expectedAfter,
                                           std::string_view msg,
                                           std::size_t expectedFrags = 1u)
            {
                provider->SetForcedSegmentSpans(L"en-US", semanticKey, spans);
                app.SetDocument(doc);
                app.Prepare(512);
                const auto& caseIssues = SpellingProbe::Issues(app);
                Require(caseIssues.size() == 1u && caseIssues[0].kind == SpellIssueKind::RepeatedWord && caseIssues[0].sourceFragments.size() == expectedFrags,
                        std::string(msg) + " [issues]");
                const auto caseItems = SpellingProbe::BuildSpellMenuAtOffset(app, caseIssues[0].sourceFragments.front().first);
                Require(std::ranges::any_of(caseItems, [](const auto& item) { return item.commandId == 10097; }), std::string(msg) + " [menu]");
                auto* const ed = SpellingProbe::Active(app);
                const bool ok =
                    SpellingProbe::InvokeSpellMenuCommand(app, 10097, SpellingProbe::SessionKey(app), ed->Snapshot(), ed->Revision(), ed->GetSelection());
                Require(ok && ed->GetText() == expectedAfter, std::string(msg) + " [apply]");
                Require(ed->Undo() && ed->GetText() == doc, std::string(msg) + " [undo]");
            };

            runRepeatCase(L"Alpha\r\n\r\nthe\r\n\r\nthe today.\n",
                          L"Alpha\uFFFCthe\uFFFCthe today.",
                          {SpellErrorSpan{10u, 3u, SpellIssueKind::RepeatedWord}},
                          L"Alpha\r\n\r\nthe\r\n\r\ntoday.\n",
                          "Delete repeated word at start of line after CRLF deletes trailing space");
            runRepeatCase(L"Keep the the\r\ntoday.\n",
                          L"Keep the the today.",
                          {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}},
                          L"Keep the\r\ntoday.\n",
                          "Delete repeated word before CRLF deletes preceding space without splitting CRLF");
            runRepeatCase(L"the\r\nthe",
                          L"the the",
                          {SpellErrorSpan{4u, 3u, SpellIssueKind::RepeatedWord}},
                          L"the\r\n",
                          "Delete repeated word at start of line and EOF deletes word cleanly");
            provider->SetForcedSegmentSpans(L"en-US", L"Keep the\tthe today.", {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}});
            runRepeatCase(L"Keep the\tthe today.\n",
                          L"Keep the the today.",
                          {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}},
                          L"Keep the today.\n",
                          "Delete repeated word with preceding tab deletes tab and word");
            runRepeatCase(L"the\xD83D\xDE00the today.\n",
                          L"the\xD83D\xDE00the today.",
                          {SpellErrorSpan{5u, 3u, SpellIssueKind::RepeatedWord}},
                          L"the\xD83D\xDE00today.\n",
                          "Delete repeated word adjacent to UTF-16 surrogate pair preserves surrogate intact");
            runRepeatCase(L"Keep the **t**he today.\n",
                          L"Keep the the today.",
                          {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}},
                          L"Keep the ****today.\n",
                          "Delete repeated word spanning sourceFragments.size() > 1 deletes fragments and trailing space",
                          2u);

            app.Execute(Command::Split);
            app.Prepare(256);
            SpellingProbe::SwitchSplitPane(app, SplitActivePane::Source);
            app.Prepare(256);
            runRepeatCase(L"Keep the the today.\n",
                          L"Keep the the today.",
                          {SpellErrorSpan{9u, 3u, SpellIssueKind::RepeatedWord}},
                          L"Keep the today.\n",
                          "Delete repeated word works in Split+Source _mirror");
            SpellingProbe::SwitchSplitPane(app, SplitActivePane::Compose);
            app.Prepare(256);
            app.Execute(Command::Compose);
            app.Prepare(256);
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

        // Real StatusLanguageComboBox keyboard/CommitSelection updates PrepareVisualContent cache without Paint-time rebuilds.
        const auto prepBefore         = statusCombo->PrepareVisualContentCount();
        const auto paintRebuildBefore = statusCombo->PaintLayoutRebuildCount();
        Require(statusCombo->OnKeyDown(app.Host(), VK_DOWN, 0u), "StatusLanguageComboBox OnKeyDown(VK_DOWN) commits next language item");
        app.Prepare(256);
        Require(statusCombo->PrepareVisualContentCount() > prepBefore && statusCombo->PaintLayoutRebuildCount() == paintRebuildBefore &&
                    statusCombo->HasPreparedLayouts() && statusCombo->CachedText() == statusCombo->GetDisplayedText(),
                "StatusLanguageComboBox OnKeyDown prepares text layout via PrepareVisualContent without Paint-time rebuild");

        const auto prepOffBefore = statusCombo->PrepareVisualContentCount();
        Require(statusCombo->OnKeyDown(app.Host(), VK_END, 0u) && ! app.Preferences().spelling.enabled, "StatusLanguageComboBox OnKeyDown(VK_END) selects Off");
        app.Prepare(256);
        Require(statusCombo->PrepareVisualContentCount() > prepOffBefore && statusCombo->PaintLayoutRebuildCount() == paintRebuildBefore &&
                    statusCombo->HasPreparedLayouts() && statusCombo->CachedText() == statusCombo->GetDisplayedText(),
                "StatusLanguageComboBox Off transition prepares text layout via PrepareVisualContent without Paint-time rebuild");

        SpellingProbe::SelectStatusLanguageByValue(app, L"en-US");
        app.Prepare(512);

        // Pointer gating: MK_SHIFT, MK_CONTROL, open combo popup, BlockDraftModal, passive Split pane, and stale PendingLeftClickRequest.
        SpellingProbe::ClearPendingSpellMenu(app);
        SpellingProbe::LeftDown(app, wordPt, MK_SHIFT);
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "MK_SHIFT left-click does not open spell menu");

        SpellingProbe::LeftDown(app, wordPt, MK_CONTROL);
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "MK_CONTROL left-click does not open spell menu");

        Require(statusCombo->OnMouseDown(app.Host(), statusPt, false, 0u) && statusCombo->IsPopupOpen(),
                "reopen status language combo popup for pointer gating test");
        SpellingProbe::LeftDown(app, wordPt);
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "open combo popup blocks editor left-click spell menu");
        (void)statusCombo->OnKeyDown(app.Host(), VK_ESCAPE, 0u);
        Require(! statusCombo->IsPopupOpen(), "Escape closes status language combo popup");

        app.Execute(Command::LocalSource);
        app.Prepare(64);
        Require(app.HasBlockDraft(), "BlockDraftModal open for pointer gating check");
        SpellingProbe::LeftDown(app, wordPt);
        SpellingProbe::LeftUp(app, wordPt);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "BlockDraftModal blocks left-click spell menu");
        app.Execute(Command::CloseDrawer);
        app.Prepare(64);

        app.Execute(Command::Split);
        app.Prepare(512);
        if (auto* const mirror = SpellingProbe::Mirror(app))
        {
            const auto mb = mirror->GetBounds();
            const POINT passivePt{static_cast<LONG>((mb.left + mb.right) * 0.5f * scale), static_cast<LONG>((mb.top + mb.bottom) * 0.5f * scale)};
            SpellingProbe::LeftDown(app, passivePt);
            SpellingProbe::LeftUp(app, passivePt);
            SpellingProbe::PumpMessages(app);
            Require(! SpellingProbe::HasPendingSpellMenu(app), "clicking passive Split pane does not trigger spell menu");
        }
        app.Execute(Command::Compose);
        app.Prepare(512);

        // Stale PendingLeftClickRequest (e.g. languageConfigVersion bump before SpellMenuRequested dispatch) is rejected safely.
        SpellingProbe::LeftDown(app, wordPt);
        app.Document().SetSelection({firstFrag.first, firstFrag.first});
        SpellingProbe::LeftUp(app, wordPt);
        Require(SpellingProbe::HasPendingLeftClick(app), "PendingLeftClickRequest armed before message pump");
        SpellingProbe::BumpLanguageConfigVersion(app);
        SpellingProbe::PumpMessages(app);
        Require(! SpellingProbe::HasPendingSpellMenu(app), "stale PendingLeftClickRequest after languageConfigVersion change is rejected");
        SpellingProbe::TriggerImmediateCheck(app);
        app.Prepare(512);

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

    test("Editor.SpellingDeferredChecksSplitBlockDraftAndIgnoreOnce",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetInstalledLanguages({L"en-US"});
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Line", L"Clean", L"prose", L"here", L"and", L"end"});

        const std::filesystem::path dataDir = root / L"deferred-split-ignoreonce";
        Application app(HiddenSpellingOptions(dataDir, provider));
        Success(app.Initialize(), "initialize app for deferred/split/BlockDraft/IgnoreOnce test");
        SpellingProbe::SetSpellLanguages(app, {L"en-US"});

        // 1. BlockDraftModal (F2) exit reschedules spell checks cleanly.
        app.SetDocument(L"Clean prose writting.\n");
        app.Prepare(512);
        Require(SpellingProbe::Issues(app).size() == 1u && app.Document().GetDecorations().size() == 1u, "initial typo squiggled before BlockDraft");
        app.Document().SetSelection({0u, 0u});
        app.Execute(Command::LocalSource);
        app.Prepare(64);
        Require(app.HasBlockDraft() && app.Document().GetDecorations().empty(), "BlockDraftModal suppresses editor squiggles while open");
        if (auto* draft = SpellingProbe::BlockDraftInput(app))
            draft->SetText(L"Clean prose writting secondtypo.\n");
        app.Execute(Command::ApplyBlock);
        app.Prepare(512);
        Require(! app.HasBlockDraft() && SpellingProbe::Issues(app).size() == 2u && app.Document().GetDecorations().size() == 2u,
                "exiting BlockDraftModal reschedules spell check and highlights updated typos");

        // 2. IME composition start/end reconciles and reschedules spell check on exit.
        app.Host().SetFocusControl(&app.Document(), false);
        SpellingProbe::SetImeCompositionForTest(app, true);
        SpellingProbe::TriggerImmediateCheck(app);
        Require(SpellingProbe::ImeSuppressionActive(app) && app.Document().GetDecorations().empty(),
                "IME composition suppresses spell check and clears squiggles");
        SpellingProbe::SetImeCompositionForTest(app, false);
        app.Prepare(512);
        SpellingProbe::ForceDebounceTimeout(app);
        app.Prepare(512);
        Require(! SpellingProbe::ImeSuppressionActive(app) && app.Document().GetDecorations().size() == 2u,
                "exiting IME composition clears imeSuppressionActive and restores squiggles");

        // 3. Split-view pane switching projects Source-mode squiggles on MirrorEditor and preserves anchored IgnoreOnce across Compose <-> Source.
        const std::size_t firstAnchor = SpellingProbe::Issues(app)[0].sourceFragments.front().first;
        app.Document().SetSelection({firstAnchor, firstAnchor});
        const auto menuItems = SpellingProbe::BuildSpellMenuAtOffset(app, firstAnchor);
        Require(! menuItems.empty(), "build menu before IgnoreOnce");
        Require(SpellingProbe::InvokeSpellMenuCommand(
                    app, 10098, SpellingProbe::SessionKey(app), app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection()),
                "IgnoreOnce succeeds in Compose mode");
        Require(app.Document().GetDecorations().size() == 1u, "one typo remains after IgnoreOnce in Compose mode");

        const auto* const priorActive = SpellingProbe::Active(app);
        app.Execute(Command::Split);
        app.Prepare(512);
        SpellingProbe::SwitchSplitPane(app, SplitActivePane::Source);
        app.Prepare(512);
        Require(SpellingProbe::Active(app) != priorActive && SpellingProbe::Mirror(app) == priorActive &&
                    SpellingProbe::Active(app)->GetViewMode() == DxUi::RichText::ViewMode::Source,
                "SwitchSplitActivePane activates MirrorEditor in Source mode");
        Require(SpellingProbe::Active(app)->GetDecorations().size() == 1u && SpellingProbe::Mirror(app)->GetDecorations().empty(),
                "Source pane in Split view shows only non-ignored typo; anchored IgnoreOnce survives Compose->Source");

        SpellingProbe::SwitchSplitPane(app, SplitActivePane::Compose);
        app.Prepare(512);
        app.Execute(Command::Compose);
        app.Prepare(512);

        // 4. CloseActiveDocument and ResetWorkspace clear _ignoredOnce across non-zero workspace keys, and Open after ResetWorkspace runs spell check.
        const auto docFile = dataDir / L"opened.md";
        std::filesystem::create_directories(dataDir);
        Success(WriteBytesAtomic(docFile, "Clean writting prose.\n"), "seed file for Open after ResetWorkspace");
        const auto word = SpellingProbe::Issues(app)[0].word;
        const auto anch = SpellingProbe::Issues(app)[0].sourceFragments.front().first;
        Require(SpellingProbe::Dictionary(app).IsAccepted(word, SpellingProbe::SessionKey(app), anch), "IgnoreOnce active before ResetWorkspace");

        // Create a second workspace document (non-zero session key), IgnoreOnce on it, then CloseActiveDocument.
        app.Document().SetSelection({0u, 0u});
        (void)app.Document().ReplaceSelection(L"Line ");
        SpellingProbe::OpenFileInWorkspace(app, docFile);
        app.Prepare(512);
        const unsigned secondKey = SpellingProbe::SessionKey(app);
        Require(secondKey != 0u && ! SpellingProbe::Issues(app).empty(), "opened second workspace slot with non-zero session key");
        const auto secondWord = SpellingProbe::Issues(app)[0].word;
        const auto secondAnch = SpellingProbe::Issues(app)[0].sourceFragments.front().first;
        const auto secondMenu = SpellingProbe::BuildSpellMenuAtOffset(app, secondAnch);
        Require(! secondMenu.empty(), "build menu in non-zero workspace slot");
        Require(
            SpellingProbe::InvokeSpellMenuCommand(app, 10098, secondKey, app.Document().Snapshot(), app.Document().Revision(), app.Document().GetSelection()),
            "IgnoreOnce succeeds in non-zero workspace slot");
        Require(SpellingProbe::Dictionary(app).IsAccepted(secondWord, secondKey, secondAnch), "IgnoreOnce active in non-zero slot");
        SpellingProbe::CloseActiveDoc(app);
        app.Prepare(256);
        Require(! SpellingProbe::Dictionary(app).IsAccepted(secondWord, secondKey, secondAnch),
                "CloseActiveDocument clears _ignoredOnce for closed workspace key");

        const unsigned priorKey = SpellingProbe::SessionKey(app);
        SpellingProbe::ResetAppWorkspace(app);
        Require(! SpellingProbe::Dictionary(app).IsAccepted(word, priorKey, anch), "ResetWorkspace clears _ignoredOnce for remaining workspace keys");

        // Open via FilePicker after ResetWorkspace triggers spell check on newly opened document.
        app.SetDocument(L"");
        app.Prepare(128);
        SpellingProbe::SetFilePickerForTest(app, [&](bool) { return docFile; });
        app.Execute(Command::Open);
        app.Prepare(512);
        Require(SpellingProbe::Issues(app).size() == 1u && SpellingProbe::Issues(app)[0].word == L"writting",
                "Open after ResetWorkspace completes spell check cleanly");

        // 5.Viewport-first projection guarantees visible typos at bottom are squiggled even with >256 overscan typos above.
        std::wstring manyDoc;
        for (int i = 0; i < 300; ++i)
        {
            std::wstring tag = L"overtypo";
            tag.push_back(static_cast<wchar_t>(L'a' + (i / 26) % 26));
            tag.push_back(static_cast<wchar_t>(L'a' + i % 26));
            manyDoc += L"Line " + tag + L".\n\n";
        }
        manyDoc += L"Line visibleendtypo here.\n";
        app.SetDocument(manyDoc);
        app.Prepare(512);
        (void)app.Document().ScrollSourceRangeIntoView(app.Host(), manyDoc.size() - 25u, manyDoc.size(), true);
        app.Prepare(64);
        const std::size_t visTypoPos = manyDoc.find(L"visibleendtypo");
        const auto decs              = app.Document().GetDecorations();
        const bool hasVisibleSquiggle =
            std::ranges::any_of(decs, [&](const auto& d) { return d.start <= visTypoPos && d.start + d.length >= visTypoPos + 14u; });
        Require(decs.size() <= 256u && hasVisibleSquiggle, "viewport-first decoration projection highlights visible typo despite >256 overscan typos above");

        // 6. SpellChooseCustomDictionary failure preserves previous valid dictionary and settings path.
        const auto validDict = dataDir / L"good-dict.dic";
        Success(WriteBytesAtomic(validDict, "SavedGoodWord\n"), "seed valid dictionary for Choose failure test");
        SpellingProbe::SetFilePickerForTest(app, [&](bool) { return validDict; });
        app.Execute(Command::SpellChooseCustomDictionary);
        Require(app.Preferences().spelling.customDictionaryPath == validDict && SpellingProbe::Dictionary(app).IsAccepted(L"SavedGoodWord"),
                "valid Choose custom dictionary updates path and loads words");

        const auto badDict = dataDir / L"bad-ansi-dict.dic";
        Success(WriteBytesAtomic(badDict, "caf\xE9\r\n"), "seed invalid ANSI dictionary");
        SpellingProbe::SetFilePickerForTest(app, [&](bool) { return badDict; });
        app.Execute(Command::SpellChooseCustomDictionary);
        Require(app.Preferences().spelling.customDictionaryPath == validDict && SpellingProbe::Dictionary(app).IsAccepted(L"SavedGoodWord"),
                "failed Choose custom dictionary preserves previous dictionary and settings path");

        // 6b. Command::SpellResetCustomDictionary success & corrupt-default failure.
        const auto defaultDictPath = dataDir / L"custom-dictionary.txt";
        Success(WriteBytesAtomic(defaultDictPath, "DefaultResetWord\n"), "seed valid default custom dictionary");
        app.Execute(Command::SpellResetCustomDictionary);
        Require(app.Preferences().spelling.customDictionaryPath.empty() && SUCCEEDED(SpellingProbe::Dictionary(app).LoadStatus()) &&
                    SpellingProbe::Dictionary(app).IsAccepted(L"DefaultResetWord") && ! SpellingProbe::Dictionary(app).IsAccepted(L"SavedGoodWord"),
                "SpellResetCustomDictionary clears custom path and reloads default custom dictionary");

        SpellingProbe::SetFilePickerForTest(app, [&](bool) { return validDict; });
        app.Execute(Command::SpellChooseCustomDictionary);
        Require(app.Preferences().spelling.customDictionaryPath == validDict, "reselect valid custom dictionary before corrupt-default reset check");
        Success(WriteBytesAtomic(defaultDictPath, "bad\xE9\n"), "corrupt default custom dictionary on disk");
        app.Execute(Command::SpellResetCustomDictionary);
        Require(app.Preferences().spelling.customDictionaryPath.empty() && SUCCEEDED(SpellingProbe::Dictionary(app).LoadStatus()) &&
                    SpellingProbe::Dictionary(app).IsAccepted(L"SavedGoodWord") && ! SpellingProbe::StatusLabelText(app).empty(),
                "SpellResetCustomDictionary surfaces failure status when default custom dictionary is corrupt");

        // 7. SpellDebounceTimerId killed unconditionally on WM_TIMER even after dirty state.
        SpellingProbe::SetSpellDebounceTimer(app);
        (void)SendMessageW(app.Window(), WM_TIMER, SpellingProbe::DebounceTimerId(), 0);
        app.Prepare(64);
        Require(foreground.NeverAcquiredForeground(), "deferred/split/BlockDraft/IgnoreOnce test never acquires foreground");
    });
}
} // namespace RedTilde
