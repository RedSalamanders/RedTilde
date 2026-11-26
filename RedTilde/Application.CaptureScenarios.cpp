#include "Application.h"
#include "PrintPreview.h"
#if defined(ENABLE_TESTS)
#include "Tests/Support/Support.Tests.FilesFixtures.h"
#endif
#include <algorithm>
#include <chrono>
#include <combaseapi.h>
#include <format>
#include <stdexcept>

namespace RedTilde
{
namespace
{
void RequireCapture(bool condition, const char* message)
{
    if (! condition)
        throw std::runtime_error(message);
}
std::filesystem::path CaptureNativePath(const std::filesystem::path& path)
{
    // The owned evidence namespace can leave room for the target but not its
    // atomic staging sibling. Use extended spelling for both without shortening
    // the fixture, changing the storage writer or relying on host long-path policy.
    auto native = path.lexically_normal();
    native.make_preferred();
    RequireCapture(native.is_absolute(), "A design-capture fixture must have an absolute owned path");
    const auto& name = native.native();
    if (! name.starts_with(L"\\\\?\\"))
        native = name.starts_with(L"\\\\") ? L"\\\\?\\UNC\\" + name.substr(2u) : L"\\\\?\\" + name;
    return native;
}
void CaptureWrite(const std::filesystem::path& path, std::string_view text = "# Invented reference notes\n")
{
    const auto native = CaptureNativePath(path);
    std::filesystem::create_directories(native.parent_path());
    const HRESULT result = WriteBytesAtomic(native, text);
    if (FAILED(result))
        throw std::runtime_error(
            std::format("Cannot seed the owned design-capture fixture (0x{:08x}, {} path units)", static_cast<unsigned>(result), path.native().size()));
}
std::wstring WritingFixture()
{
    return L"# A little room for big ideas\n\n"
           L"Good writing starts with a clear space. Keep your thoughts close and let the next sentence arrive.\n\n"
           L"## Make it yours\n\n"
           L"A place for **careful words**, a little curiosity, and a few ideas worth keeping. Markdown stays right beside you.\n\n"
           L"- [x] Gather the notes\n- [x] Find the thread\n- [ ] Share the next draft\n\n"
           L"## Keep the details\n\n"
           L"Writing is a series of small discoveries. Give this paragraph a little room and follow one thought at a time.\n\n"
           L"A quiet moment of writing can make a busy day feel clear. [Read the outline](https://example.invalid/outline) when you are ready.\n\n"
           L"> The next idea does not need to be perfect. It only needs a place to begin.\n";
}
} // namespace

void Application::PrepareDesignScenario()
{
    if (! _options.headless || _options.capture.empty() || ! _options.performance.empty())
        return;
    RequireCapture(! _options.scenario.empty() &&
                       std::ranges::all_of(_options.scenario,
                                           [](wchar_t value) { return (value >= L'a' && value <= L'z') || (value >= L'0' && value <= L'9') || value == L'-'; }),
                   "A capture scenario must use a lowercase ASCII identifier");
    const UINT nativeDpi    = GetDpiForWindow(_window.get());
    const UINT requestedDpi = _options.captureDpi ? _options.captureDpi : nativeDpi;
    RequireCapture(requestedDpi >= 96 && requestedDpi <= 384, "Capture DPI must be between 96 and 384");
    const bool compactRailScene  = _options.scenario == L"panes-collapsed-rails-light" || _options.scenario == L"pane-hover-reveal-dark";
    const unsigned defaultWidth  = _options.scenario == L"narrow" ? 800u : compactRailScene ? 1080u : 1440u;
    const unsigned defaultHeight = _options.scenario == L"narrow" ? 560u : compactRailScene ? 820u : 900u;
    const int widthPx =
        _options.viewportWidthPx ? static_cast<int>(_options.viewportWidthPx) : MulDiv(static_cast<int>(defaultWidth), static_cast<int>(requestedDpi), 96);
    const int heightPx =
        _options.viewportHeightPx ? static_cast<int>(_options.viewportHeightPx) : MulDiv(static_cast<int>(defaultHeight), static_cast<int>(requestedDpi), 96);
    RequireCapture(widthPx >= 800 && widthPx <= 7680 && heightPx >= 560 && heightPx <= 4320, "Capture viewport is outside its bounded dimensions");
    RequireCapture(static_cast<unsigned>(widthPx) * 96 >= 800u * requestedDpi && static_cast<unsigned>(heightPx) * 96 >= 560u * requestedDpi,
                   "The capture viewport must retain the 800 by 560 DIP application minimum");
    RECT frame{};
    RequireCapture(GetWindowRect(_window.get(), &frame) != FALSE, "Cannot read the owned hidden capture frame");
    frame.right  = frame.left + widthPx;
    frame.bottom = frame.top + heightPx;
    // Only this hidden owner HWND receives the synthetic change. The sidecar
    // distinguishes its layout exercise from physical-monitor DPI qualification.
    (void)SendMessageW(_window.get(), WM_DPICHANGED, MAKEWPARAM(requestedDpi, requestedDpi), reinterpret_cast<LPARAM>(&frame));
    RequireCapture(SetWindowPos(_window.get(), nullptr, frame.left, frame.top, widthPx, heightPx, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                   "Cannot size the owned hidden capture window");
    Layout();

    const auto& scenario         = _options.scenario;
    std::wstring fixtureName     = _options.document.filename().wstring();
    const bool clipboardSettings = scenario == L"image-paste-light" || scenario == L"image-paste-dark";
    if (clipboardSettings)
    {
        SetDocument(WritingFixture());
        Execute(scenario == L"image-paste-dark" ? Command::ThemeDark : Command::ThemeLight);
        _settings.view        = EditorView::Compose;
        _settings.showFiles   = false;
        _settings.showOutline = false;
        const bool imageFirst = scenario == L"image-paste-light";
        if (_settings.imageFirstPaste != imageFirst)
            Execute(Command::ImageFirstPaste);
        Execute(Command::Settings);
        Prepare(512);
        _settingsScroll->SetScrollOffset(_settingsPrivacyScrollY);
        RequireCapture(_host.GetClipboardPreference() == (imageFirst ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst),
                       "The clipboard preference capture must expose the actual live host policy");
        const auto background = _host.GetTheme().windowBackground;
        RequireCapture(! _host.GetTheme().highContrast && (background.r + background.g + background.b < 1.5f) == ! imageFirst,
                       "The clipboard preference capture must apply its declared native Light or Dark palette");
        fixtureName = L"Clipboard picture preferences";
    }
    if (scenario == L"diagram-label-light" || scenario == L"diagram-label-dark")
    {
        SetDocument(L"# Notebook review\n\n```mermaid\nstateDiagram-v2\ndirection LR\n"
                    L"[*] --> Draft\nDraft --> Reviewed: Review\nReviewed --> Saved: Save\n```\n\n"
                    L"## Message labels\n\n```mermaid\nsequenceDiagram\nparticipant Writer\nparticipant Editor\n"
                    L"Writer->>Editor: Review draft\nEditor-->>Writer: Ready to save\n```\n");
        _settings.appearance  = scenario == L"diagram-label-dark" ? Appearance::Dark : Appearance::Light;
        _settings.view        = EditorView::Compose;
        _settings.showFiles   = false;
        _settings.showOutline = false;
        fixtureName           = L"Notebook review";
    }
    if (scenario == L"quote-scroll-light" || scenario == L"quote-scroll-dark")
    {
        std::wstring notebook = L"# Notebook observations\n\n"
                                L"> A useful notebook keeps the observation beside its explanation.\n>\n"
                                L"> Keep one thought per paragraph, and leave room for the next question.\n\n"
                                L"> [!NOTE]\n> A little space makes this note easier to read.\n\n";
        for (unsigned paragraph = 0; paragraph < 40; ++paragraph)
            notebook += L"A notebook leaves room to collect another observation, then return to the question.\n\n";
        SetDocument(notebook);
        _settings.appearance  = scenario == L"quote-scroll-dark" ? Appearance::Dark : Appearance::Light;
        _settings.view        = EditorView::Compose;
        _settings.showFiles   = false;
        _settings.showOutline = false;
        fixtureName           = L"Notebook observations";
    }
    if (scenario == L"image-consent-light" || scenario == L"image-consent-dark")
    {
        _settings.allowRemoteImages       = false;
        _settings.allowLocalNetworkImages = false;
        _session.remoteImagesAllowed      = false;
        _editor->SetRemoteImagesEnabled(false);
        SetDocument(L"# Illustration notebook\n\n"
                    L"Remote illustrations load only after a choice for this document.\n\n"
                    L"![Invented landscape](https://images.example.invalid/landscape.png)\n\n"
                    L"The source, selection and undo history remain yours.\n");
        Execute(scenario == L"image-consent-dark" ? Command::ThemeDark : Command::ThemeLight);
        const auto background = _host.GetTheme().windowBackground;
        RequireCapture(! _host.GetTheme().highContrast && (background.r + background.g + background.b < 1.5f) == (scenario == L"image-consent-dark"),
                       "The image consent capture must apply its declared native Light or Dark palette");
        _settings.view        = EditorView::Compose;
        _settings.showFiles   = false;
        _settings.showOutline = false;
        fixtureName           = L"Illustration notebook";
    }
    if (scenario == L"task-presentation-light" || scenario == L"task-presentation-dark")
        _settings.strikeCompletedTasks = true;
    if (scenario == L"spelling-light" || scenario == L"spelling-dark" || scenario == L"spelling-high-contrast")
    {
#if defined(ENABLE_TESTS)
        auto provider = std::make_shared<FakeSpellProvider>();
        provider->SetInstalledLanguages({L"en-US", L"fr-FR"});
        provider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
        provider->AddKnownWords(L"en-US", {L"Drafting", L"with",    L"confidence", L"Good",       L"doesn't",    L"mean",  L"every",    L"first",     L"draft",
                                           L"is",       L"perfect", L"RedTilde",   L"underlines", L"unfamiliar", L"words", L"like",     L"and",       L"right",
                                           L"in",       L"your",    L"flow",       L"It's",       L"careful",    L"about", L"Markdown", L"Technical", L"terms",
                                           L"inside",   L"inline",  L"code",       L"equations",  L"links",      L"notes", L"acronyms", L"product",   L"tags",
                                           L"such",     L"as",      L"stay",       L"cleanly",    L"untouched",  L"The",   L"status",   L"bar",       L"always",
                                           L"shows",    L"active",  L"proofing",   L"language"});
        provider->SetSuggestions(L"en-US", L"writting", {L"writing", L"written"});
        provider->SetSuggestions(L"en-US", L"documnet", {L"document"});
        provider->SetSuggestions(L"en-US", L"recieve", {L"receive"});
        provider->SetSuggestions(L"en-US", L"teh", {L"the"});
        _options.spellProviderForTest = provider;
        _spellService.SetProviderForTest(provider);
        _settings.spelling.enabled   = true;
        _settings.spelling.languages = {L"en-US"};
        _settings.view               = EditorView::Compose;
        _settings.showFiles          = false;
        _settings.showOutline        = false;
        _settings.appearance         = scenario == L"spelling-dark" ? Appearance::Dark : Appearance::Light;
        ApplyTheme();
        const std::wstring spellingFixture =
            L"# Drafting with confidence\n\n"
            L"Good writting doesn't mean every first draft is perfect. **RedTilde** underlines unfamiliar words like documnet and recieve right in your "
            L"flow.\n\n"
            L"It's careful about Markdown. Technical terms inside inline code (`writting_fn()`), equations ($x_{documnet}^2$), "
            L"links ([notes](https://example.invalid/recieve)), acronyms (NASA), and product tags such as C23 stay cleanly untouched.\n\n"
            L"The status bar always shows teh active proofing language.\n";
        SetDocument(spellingFixture);
        _editor->SetSelection({0, 0});
        SelectionChanged();
        ScheduleSpellCheck(true);
        Layout();
        Prepare(512);
        RequireCapture(_spell.issues.size() == 4u && _editor->GetDecorationStatus() == DxUi::RichTextDecorationStatus::Ready,
                       "Spelling capture scenario must settle four spelling squiggles");
        fixtureName = L"Spelling review";
#else
        RequireCapture(false, "The synthetic spelling capture requires a test-enabled build");
#endif
    }
    if (scenario == L"save-confirm-dialog-light" || scenario == L"files-multi-folder-and-github-light" || scenario == L"files-context-menu-dark" ||
        scenario == L"open-github-autosuggest-light" || scenario == L"open-github-autosuggest-dark")
    {
        const bool isDark    = scenario == L"files-context-menu-dark" || scenario == L"open-github-autosuggest-dark";
        _settings.appearance = isDark ? Appearance::Dark : Appearance::Light;
        ApplyTheme();

        const auto wsBase  = CaptureNativePath(std::filesystem::absolute(_options.dataDirectory) / L"capture-workspace");
        const auto folderA = wsBase / L"ProjectAlpha" / L"docs";
        const auto folderB = wsBase / L"ProjectBeta" / L"docs";
        std::filesystem::create_directories(folderA / L"guides");
        std::filesystem::create_directories(folderB / L"specs");
        const auto fileA1 = folderA / L"guides" / L"getting-started.md";
        const auto fileA2 = folderA / L"Architecture.md";
        const auto fileB1 = folderB / L"specs" / L"FileSafety.md";
        CaptureWrite(fileA1, "# Getting Started\n\nAlpha Workspace guide.\n");
        CaptureWrite(fileA2, "# Architecture\n\nModular design overview.\n");
        CaptureWrite(fileB1, "# File Safety\n\nAtomic write contract.\n");

        _settings.filesRoots   = {folderA, folderB};
        _settings.filesRoot    = folderA;
        _settings.showFiles    = true;
        _settings.filesPinMode = PanePinMode::Pinned;

        GitHubConnectedRepoState ghRepo;
        ghRepo.ref            = {L"RedSalamanders", L"RedTilde", L"main"};
        ghRepo.resolvedBranch = L"main";
        ghRepo.loaded         = true;
        ghRepo.entries        = {
            {L"README.md", L"README.md", L"111", L"", NoFilesParent, 0u, 0u, false},
            {L"docs", L"docs", L"222", L"", NoFilesParent, 2u, 0u, true},
            {L"docs/Overview.md", L"Overview.md", L"333", L"", 1u, 0u, 1u, false},
            {L"docs/Shortcuts.md", L"Shortcuts.md", L"444", L"", 1u, 0u, 1u, false},
        };
        _connectedGitHubRepos = {ghRepo};
        _settings.githubRepos = {ghRepo.ref};

        StoredDocument docA1;
        RequireCapture(SUCCEEDED(LoadDocument(fileA1, docA1)), "Load capture fileA1");
        RequireCapture(SetDocument(docA1.text, fileA1, docA1.bom, docA1.stamp, false), "Set capture docA1");
        _editor->SetSelection({docA1.text.size(), docA1.text.size()});
        RequireCapture(_editor->ReplaceSelection(L"\nAdded draft paragraph for workspace preview.\n"), "Edit capture docA1");

        const auto waitMultiFolders = [&]
        {
            const auto deadline = GetTickCount64() + 10000u;
            while ((_multiFolderFiles.Busy() || _folderSnapshots.size() < 2u) && GetTickCount64() < deadline)
            {
                Prepare(8);
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            _multiFolderFiles.Close();
        };

        if (scenario == L"save-confirm-dialog-light")
        {
            RequireCapture(StartWorkspaceDocument(), "Start slot 2 for save-confirm capture");
            StoredDocument docB1;
            RequireCapture(SUCCEEDED(LoadDocument(fileB1, docB1)), "Load capture fileB1");
            RequireCapture(SetDocument(docB1.text, fileB1, docB1.bom, docB1.stamp, false), "Set capture docB1");
            _editor->SetSelection({docB1.text.size(), docB1.text.size()});
            RequireCapture(_editor->ReplaceSelection(L"\nPending review note.\n"), "Edit capture docB1");

            RequireCapture(StartWorkspaceDocument(), "Start slot 3 for save-confirm capture");
            RequireCapture(SetDocument(L"# Scratchpad\n\nQuick meeting notes.\n", {}, false, {}, false), "Set capture untitled draft");
            _editor->SetSelection({_editor->Snapshot().Length(), _editor->Snapshot().Length()});
            RequireCapture(_editor->ReplaceSelection(L"\nFollow-up action items.\n"), "Edit capture untitled draft");

            waitMultiFolders();
            _settings.exitUnsavedBehavior = ExitUnsavedBehavior::Prompt;
            ShowSaveConfirmModal([] {}, true);
            RequireCapture(_saveConfirmPanel && _saveConfirmPanel->IsVisible(), "SaveConfirmModal capture panel visible");
            fixtureName = L"getting-started.md";
        }
        else if (scenario == L"files-multi-folder-and-github-light")
        {
            waitMultiFolders();
            RefreshFiles();
            RequireCapture(std::ranges::any_of(_filesModel.rows, [](const auto& r) { return r.text == L"docs (ProjectAlpha)"; }) &&
                               std::ranges::any_of(_filesModel.rows, [](const auto& r) { return r.text == L"docs (ProjectBeta)"; }) &&
                               std::ranges::any_of(_filesModel.rows, [](const auto& r) { return r.text == L"RedSalamanders/RedTilde (main)"; }),
                           "Multi-folder + GitHub capture renders all workspace sections");
            fixtureName = L"getting-started.md";
        }
        else if (scenario == L"files-context-menu-dark")
        {
            _cutFilePaths.insert(fileA2.lexically_normal().native());
            waitMultiFolders();
            RefreshFiles();
            ShowRenameFileModal(fileA1, false);
            _renameInput->SetTextAndNotify(L"getting-started-v2.md");
            RequireCapture(_renamePanel && _renamePanel->IsVisible(), "RenameFileModal dark capture panel visible");
            fixtureName = L"getting-started.md";
        }
        else
        {
            waitMultiFolders();
            RefreshFiles();
            _github.Close();
            _githubSignedInLogin = L"octocat";
            _githubUserRepoCache = {
                {L"RedSalamanders",
                 L"RedTilde",
                 L"RedSalamanders/RedTilde",
                 L"Native Windows Markdown editor",
                 L"main",
                 L"https://github.com/RedSalamanders/RedTilde",
                 false},
                {L"RedSalamanders",
                 L"RedPrism",
                 L"RedSalamanders/RedPrism",
                 L"Native D3D11 raster editor",
                 L"main",
                 L"https://github.com/RedSalamanders/RedPrism",
                 false},
                {L"RedSalamanders",
                 L"RedSalamander",
                 L"RedSalamanders/RedSalamander",
                 L"Dual-pane file manager",
                 L"main",
                 L"https://github.com/RedSalamanders/RedSalamander",
                 false},
                {L"RedSalamanders", L"DxUi", L"RedSalamanders/DxUi", L"Shared DirectX UI library", L"main", L"https://github.com/RedSalamanders/DxUi", false},
                {L"octocat", L"red-notes", L"octocat/red-notes", L"Private personal markdown vault", L"main", L"https://github.com/octocat/red-notes", true},
            };
            ShowOpenGitHubModal();
            _openGitHubUrlInput->SetTextAndNotify(L"Red");
            RequireCapture(_openGitHubPanel && _openGitHubPanel->IsVisible() && ! _githubSuggestModel.rows.empty(),
                           "OpenGitHubModal capture panel and filtered autosuggest list visible");
            fixtureName = L"getting-started.md";
        }
    }
    const bool reference    = scenario == L"reference-dark";
    const bool fixtureScene = scenario == L"compose-light" || scenario == L"compose-dark" || scenario == L"compose-paper" || scenario == L"source" ||
                              scenario == L"split" || scenario == L"split-dark" || scenario == L"settings" || scenario == L"settings-about" ||
                              scenario == L"narrow" || scenario == L"high-contrast" || scenario == L"files" || scenario == L"find" || scenario == L"focus" ||
                              scenario == L"equation" || scenario == L"panel-toggle-hover" || scenario == L"panes-collapsed-rails-light" ||
                              scenario == L"pane-hover-reveal-dark" || scenario == L"split-source-active-light" || scenario == L"split-unlinked-dark" ||
                              reference;
    bool frozenFilesModel{}, reparseRemoved{};
    if (scenario == L"markdown-style" || scenario == L"caption-close-hover" || scenario == L"caption-maximize-hover")
    {
        // Invented content reproduces the user's Markdown example without
        // exposing a loaded filename or ambient filesystem model.
        SetDocument(L"## Show the flow\n\n```mermaid\ngraph LR\nA[Think] --> B[Write]\nB --> C[Refine]\n```\n\n"
                    L"## An equation\n\n$$\nx = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}\n$$\n\n"
                    L"Place the caret inside the equation to see the native inspector preview.\n\n"
                    L"## Details\n\n| View | Purpose |\n| --- | --- |\n| Compose | Stay in the writing |\n"
                    L"| Source | Inspect exact Markdown |\n| Split | Keep both visible |\n\n"
                    L"> Simple tools. Thoughtful defaults. Your files stay yours.\n");
        fixtureName           = L"Markdown styling example";
        _settings.view        = EditorView::Compose;
        _settings.showFiles   = false;
        _settings.showOutline = false;
    }
    if (fixtureScene)
    {
        GUID id{};
        RequireCapture(SUCCEEDED(CoCreateGuid(&id)), "Cannot identify the unique owned capture fixture");
        const auto fixture = CaptureNativePath(std::filesystem::absolute(_options.dataDirectory) /
                                               // Preserve all 128 GUID bits while leaving room for atomic-write staging
                                               // filenames below the evidence root on machines with legacy path limits.
                                               std::format(L"cf-{:08x}{:04x}{:04x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                                                           id.Data1,
                                                           id.Data2,
                                                           id.Data3,
                                                           id.Data4[0],
                                                           id.Data4[1],
                                                           id.Data4[2],
                                                           id.Data4[3],
                                                           id.Data4[4],
                                                           id.Data4[5],
                                                           id.Data4[6],
                                                           id.Data4[7]));
        const auto writing = fixture / (reference ? L"Reports" : L"Writing");
        if (reference)
        {
            RequireCapture(! _options.document.empty(), "The reference capture requires its exact reviewed report document");
            for (unsigned index = 1; index <= 12; ++index)
                CaptureWrite(writing / L"archive" / std::format(L"report-{:02}.md", index));
            CaptureWrite(writing / L"notes/risks.md");
            CaptureWrite(writing / L"notes/semantic-model-ga.md");
            CaptureWrite(writing / L"templates/weekly.md");
            CaptureWrite(writing / L"templates/review.md");
            CaptureWrite(writing / L"README.md");
            CaptureWrite(writing / L"weekly-report-2026-10-01.md");
            CaptureWrite(writing / L"weekly-report-2026-10-08.md");
        }
        else
        {
            CaptureWrite(writing / L"Drafts/Chapter2.md", "# The first conversation\n\nA small beginning for an invented story.\n");
            CaptureWrite(writing / L"Drafts/Chapter10.md", "# The journey home\n\nA later chapter with a little room to grow.\n");
            CaptureWrite(writing / L"Research/Reading notes.markdown", "# Reading notes\n\nA collection of invented references and questions.\n");
            CaptureWrite(writing / L"Archive/Early draft.md", "# An early draft\n\nKeep the version that helped the idea begin.\n");
            CaptureWrite(writing / L"Outline.md", "# The next draft\n\n## Beginning\n\n## Middle\n\n## Ending\n");
            CaptureWrite(writing / L"Assets/cover.txt", "Synthetic non-Markdown fixture; this folder must be pruned.\n");
            CaptureWrite(writing / L"Private notes.md", "# Hidden fixture\n\nThis hidden entry is deliberately excluded.\n");
            CaptureWrite(writing / L"System notes.md", "# System fixture\n\nThis system entry is deliberately excluded.\n");
            RequireCapture(SetFileAttributesW((writing / L"Private notes.md").c_str(), FILE_ATTRIBUTE_HIDDEN) != FALSE &&
                               SetFileAttributesW((writing / L"System notes.md").c_str(), FILE_ATTRIBUTE_SYSTEM) != FALSE,
                           "Cannot mark excluded Files-pane capture entries");
        }
#if defined(ENABLE_TESTS)
        const auto target = fixture / L"Reference target";
        CaptureWrite(target / L"Not followed.md", "# Reparse target\n\nThe Files tree must never visit this entry.\n");
        const auto link = (reference ? fixture : writing) / L"Linked notes";
        RequireCapture(SUCCEEDED(Testing::CreateFixtureJunction(fixture, link, target)), "Cannot create the owned reparse-point capture fixture");
        auto removeOwnedLink = wil::scope_exit([&]
        {
            _folderFiles.Close();
            if (! RemoveDirectoryW(link.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
                OutputDebugStringW(L"RedTilde capture could not remove its owned reparse fixture leaf.\n");
        });
#endif
        auto source = reference ? CurrentText() : WritingFixture();
        if (scenario == L"split-unlinked-dark")
            for (unsigned section = 1; section <= 14; ++section)
                source += std::format(L"\n## Section {}\n\nWriting keeps its place across independent viewports when linked scroll is off.\n", section);
        if (scenario == L"equation")
            source = L"# The shape of an idea\n\n"
                     L"A few symbols can make a complicated thought feel clear. Keep the exact source one step away.\n\n"
                     L"$$\nx=\\frac{-b+\\sqrt{b^2-4ac}}{2a}\n$$\n\n"
                     L"## Keep the meaning\n\n"
                     L"The rendered equation and its source are two views of the same idea. Edit it in place, then return to the rendering.\n";
        const auto path = writing / (reference ? L"weekly-report-2026-10-15.md" : scenario == L"equation" ? L"Equation notes.md" : L"Ideas.md");
        FileStamp identity;
        RequireCapture(SUCCEEDED(SaveDocument(path, source, false, nullptr, identity)), "Cannot stage the synthetic capture document");
        // Capture scenarios own a synthetic workspace even when a --document
        // input established another root during launch.
        _settings.filesRoot = writing;
        SetDocument(std::move(source), path, false, identity);
        fixtureName = reference ? L"weekly-report-2026-10-15.md" : L"Writing";
        if (scenario != L"focus" && scenario != L"equation" && scenario != L"settings" && scenario != L"settings-about" &&
            scenario != L"panes-collapsed-rails-light" && scenario != L"pane-hover-reveal-dark" && ! _settings.showFiles)
            Execute(Command::Files);
        if (_settings.showFiles)
        {
            const auto deadline = GetTickCount64() + 10000;
            unsigned turns{};
            while ((! _folderSnapshot || _folderSnapshot->root != writing || _folderFiles.Busy()) && GetTickCount64() < deadline && turns++ < 2048)
            {
                Prepare(8);
                if (! _folderSnapshot || _folderSnapshot->root != writing || _folderFiles.Busy())
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            RequireCapture(_folderSnapshot && _folderSnapshot->root == writing && ! _folderFiles.Busy() && SUCCEEDED(_folderSnapshot->status) &&
                               SUCCEEDED(_folderSnapshot->watchStatus),
                           "The owned Files capture fixture did not complete its bounded scan/watch preparation");
            RequireCapture(_folderSnapshot->markdownCount == (reference ? 20u : 6u) &&
                               std::ranges::none_of(_folderSnapshot->entries, [](const auto& entry) { return entry.name == L"Not followed.md"; }),
                           "Files capture fixture included a hidden/system/non-Markdown/reparse-target entry");
            if (reference)
            {
                _collapsedFolders.insert(writing / L"archive");
                _collapsedFolders.insert(writing / L"templates");
                _collapsedFolders.erase(writing / L"notes");
                RefreshFiles();
            }
            const auto draft = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [](const auto& row) { return row.text == L"Drafts"; });
            if (draft != _filesModel.rows.end())
                _filesModel.OnTreeToggleExpanded(draft->id, true);
            // Freeze this proved immutable listing while evidence retains only
            // ordinary files/directories. Closing invalidates future delivery.
            _folderFiles.Close();
            frozenFilesModel = true;
        }
#if defined(ENABLE_TESTS)
        RequireCapture(RemoveDirectoryW(link.c_str()) != FALSE, "Cannot retire the owned capture reparse leaf before evidence retention");
        reparseRemoved = true;
        removeOwnedLink.release();
#endif
        const auto end = _editor->Snapshot().Length();
        _editor->SetSelection({end, end});
        RequireCapture(_editor->ReplaceSelection(reference ? L"\r\n" : L"\n"), "Cannot stage the synthetic Edited state");
        _editor->SetSelection({0, 0});
        SelectionChanged();
        (void)_editor->ScrollSourceRangeIntoView(_host, 0, 0, true);
        Prepare(512);
        if (reference)
        {
            const auto anchor = _editor->GetText().find(L"report snapshot.");
            RequireCapture(anchor != std::wstring::npos, "Reference report caret anchor must exist");
            const auto caret = anchor + std::wstring_view(L"report snapshot.").size();
            RequireCapture(caret < _editor->Snapshot().Length(), "Reference report caret anchor must exist");
            _editor->SetSelection({caret, caret});
            SelectionChanged();
            _host.SetFocusControl(_editor, false);
            RequireCapture(_documentInfo.headings.size() == 7 && _outlineTree->GetSelectedItemId() == 2,
                           "Reference report exposes the same seven headings and current section as image 2");
        }
        if (scenario == L"find")
        {
            Execute(Command::Find);
            _findText->SetTextAndNotify(L"writing");
            _replaceText->SetTextAndNotify(L"drafting");
            _editor->SetSelection({0, 0});
            Execute(Command::FindNext);
            Execute(Command::FindNext);
            RequireCapture(_findBar->IsVisible() && _find.table.matches.size() >= 2, "The Find capture did not expose its deterministic query state");
        }
        if (scenario == L"focus")
        {
            const auto caret = _editor->GetText().find(L"Give this paragraph");
            RequireCapture(caret != std::wstring::npos, "Focus fixture paragraph is missing");
            _editor->SetSelection({caret, caret});
            SelectionChanged();
            if (! _settings.focusMode)
                Execute(Command::Focus);
            RequireCapture(_settings.focusMode && _editor->IsFocusModeEnabled(), "The Focus capture did not enter the shared dimming presentation");
            Prepare(512);
            (void)_editor->ScrollSourceRangeIntoView(_host, caret, caret, true);
        }
        if (scenario == L"equation")
        {
            const auto caret = _editor->GetText().find(L"\\frac");
            RequireCapture(caret != std::wstring::npos, "Equation fixture source is missing");
            _editor->SetSelection({caret, caret});
            SelectionChanged();
            Execute(Command::LocalSource);
            const auto region = _editor->GetSemanticSourceRegion();
            RequireCapture(region && ! HasBlockDraft() && _editor->GetText().compare(region->source.first, 2, L"$$") == 0,
                           "The equation capture did not show its exact TeX source in place");
        }
        if (scenario == L"settings" || scenario == L"settings-about")
        {
            if (_drawer->IsVisible())
                Execute(Command::CloseDrawer);
            Execute(Command::Settings);
            if (scenario == L"settings-about")
            {
                Layout();
                Prepare(512);
                _settingsScroll->SetScrollOffset(_settingsAboutScrollY);
            }
        }
        if (scenario == L"files" || scenario == L"compose-light" || scenario == L"compose-dark" || scenario == L"compose-paper" ||
            scenario == L"panel-toggle-hover")
            _host.SetFocusControl(_editor, false);
        if (scenario == L"panes-collapsed-rails-light" || scenario == L"pane-hover-reveal-dark")
        {
            _settings.showFiles      = false;
            _settings.showOutline    = false;
            _settings.filesPinMode   = PanePinMode::Unpinned;
            _settings.outlinePinMode = PanePinMode::Unpinned;
            RequireCapture(StartWorkspaceDocument(), "Cannot add second dirty workspace document for collapsed-rail capture");
            RequireCapture(SetDocument(L"# Second chapter\n\nA second modified workspace file.\n", writing / L"Drafts/Chapter2.md", false, {}),
                           "Cannot seed second dirty workspace file for collapsed-rail capture");
            const auto secondEnd = _editor->Snapshot().Length();
            _editor->SetSelection({secondEnd, secondEnd});
            RequireCapture(_editor->ReplaceSelection(L"\n"), "Cannot mark second workspace capture file Edited");
            RequireCapture(SwitchWorkspaceDocument(0), "Cannot return to primary workspace fixture");
            _editor->SetSelection({0, 0});
            SelectionChanged();
            (void)_editor->ScrollSourceRangeIntoView(_host, 0, 0, true);
            _host.SetFocusControl(_editor, false);
            Layout();
            Prepare(512);
            if (scenario == L"pane-hover-reveal-dark")
            {
                RevealPaneOverlay(PaneSide::Right);
                Prepare(512);
                RequireCapture(_leftRail && _leftRail->IsVisible() && _rightRail && _rightRail->IsVisible() && IsPaneOverlayActive(PaneSide::Right) &&
                                   _sidebar->IsVisible() && ! _filesPane->IsVisible(),
                               "Hover-reveal dark capture must render floating Right Outline overlay over the dark canvas");
            }
            else
            {
                RequireCapture(_leftRail && _leftRail->IsVisible() && _rightRail && _rightRail->IsVisible() && ! _filesPane->IsVisible() &&
                                   ! _sidebar->IsVisible() && _leftRail->ItemCount() == 2u && _rightRail->ItemCount() == 3u,
                               "Collapsed rails capture must render both 28-DIP rails with 2 edited files and 3 headings");
            }
        }
        if (scenario == L"split-source-active-light")
        {
            SetView(EditorView::Split);
            Prepare(512);
            SwitchSplitActivePane(SplitActivePane::Source, true);
            Execute(Command::Find);
            _findText->SetTextAndNotify(L"writing");
            _replaceText->SetTextAndNotify(L"drafting");
            ActiveEditor()->SetSelection({0, 0});
            Execute(Command::FindNext);
            Layout();
            Prepare(512);
            RequireCapture(_splitActivePane == SplitActivePane::Source && ! _mirror->IsReadOnly() && _editor->IsReadOnly() && _findBar->IsVisible() &&
                               _findBar->GetBounds().left >= _mirror->GetBounds().left - 32.f,
                           "Split Source-active capture must place active editing and Find over the Source pane");
        }
        if (scenario == L"split-unlinked-dark")
        {
            if (_settings.splitScrollSync)
                Execute(Command::ToggleSplitScrollSync);
            SetView(EditorView::Split);
            Layout();
            Prepare(512);
            (void)_editor->SetScrollOffsetDip(0.f);
            const auto mirrorInfo = _mirror->GetScrollInfo();
            RequireCapture(mirrorInfo && mirrorInfo->contentHeightDip > mirrorInfo->viewportHeightDip + 80.f,
                           "Split unlinked dark capture requires scrollable Source content");
            (void)_mirror->SetScrollOffsetDip(std::min(280.f, mirrorInfo->contentHeightDip - mirrorInfo->viewportHeightDip));
            Execute(Command::Find);
            _findText->SetTextAndNotify(L"writing");
            Layout();
            Prepare(512);
            RequireCapture(! _settings.splitScrollSync && _splitActivePane == SplitActivePane::Compose && _mirror->GetScrollInfo() &&
                               _mirror->GetScrollInfo()->offsetDip > 50.f && _editor->GetScrollInfo() && _editor->GetScrollInfo()->offsetDip < 10.f,
                           "Split unlinked dark capture must preserve independent Source scroll depth");
        }
    }
    if (scenario == L"compose-paper" && _settings.appearance != Appearance::Paper)
        Execute(Command::ThemePaper);
    if (scenario == L"split-dark")
    {
        if (_settings.appearance != Appearance::Dark)
            Execute(Command::ThemeDark);
        if (_settings.view != EditorView::Split)
            SetView(EditorView::Split);
    }
    if (scenario == L"print-preview")
    {
#if defined(ENABLE_TESTS)
        const auto showcasePath = CaptureNativePath(std::filesystem::absolute(_options.dataDirectory) / L"Showcase.md");
        SetDocument(SampleDocument(), showcasePath);
        _settings.appearance = Appearance::Light;
        ApplyTheme();
        _settings.view                    = EditorView::Compose;
        _settings.showFiles               = false;
        _settings.showOutline             = false;
        fixtureName                       = L"Showcase.md";
        _options.printDestinationsForTest = Printing::Destinations{{L"Microsoft Print to PDF", L"Office Printer (Letter)"}, L"Office Printer (Letter)"};
        _options.printPrinterForTest =
            [](const std::wstring& name, const std::optional<Printing::PrinterSettings>& requested, Printing::PrinterConfiguration& configuration) -> HRESULT
        {
            const bool pdf                  = Printing::SamePrintName(name, L"Microsoft Print to PDF");
            configuration.selection.printer = name;
            configuration.selection.port    = pdf ? L"PORTPROMPT:" : L"LPT1:";
            const Printing::PrinterSettings defaults{.form        = L"Letter",
                                                     .orientation = Printing::Orientation::Portrait,
                                                     .duplex      = pdf ? std::nullopt : std::optional{Printing::Duplex::OneSided},
                                                     .collate     = pdf ? std::nullopt : std::optional{false}};
            configuration.settings      = requested.value_or(defaults);
            configuration.forms         = {L"Letter", L"A4"};
            configuration.maximumCopies = pdf ? 1u : 10u;
            const bool landscape        = configuration.settings.orientation == Printing::Orientation::Landscape;
            const bool a4               = configuration.settings.form == L"A4";
            const float w               = a4 ? 794.f : 816.f;
            const float h               = a4 ? 1123.f : 1056.f;
            const D2D1_SIZE_F size{landscape ? h : w, landscape ? w : h};
            configuration.selection.paper = {size, {24.f, 24.f, size.width - 24.f, size.height - 24.f}};
            return S_OK;
        };
        Layout();
        Prepare(512);
        Execute(Command::Print);
        RequireCapture(_printPreview != nullptr && _printOperation != nullptr, "Print preview capture did not start the preview host");
        const auto deadline = GetTickCount64() + 15000u;
        unsigned turns{};
        while (_printPreview && (! _printPreview->IsReady() || _printPreview->IsConfigurationPending()) && GetTickCount64() < deadline && turns++ < 3000u)
        {
            MSG message{};
            while (PeekMessageW(&message, _window.get(), 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (_printPreview && _printPreview->Window())
            {
                while (PeekMessageW(&message, _printPreview->Window(), 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            if (_printPreview && (! _printPreview->IsReady() || _printPreview->IsConfigurationPending()))
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
        RequireCapture(_printPreview && _printPreview->IsReady() && ! _printPreview->IsConfigurationPending(),
                       "Print preview capture did not settle a ready sheet");
        if (_options.captureDpi)
        {
            RECT previewFrame{};
            RequireCapture(GetWindowRect(_printPreview->Host().GetHwnd(), &previewFrame) != FALSE, "Cannot read the hidden print-preview frame");
            previewFrame.right  = previewFrame.left + MulDiv(1040, static_cast<int>(requestedDpi), 96);
            previewFrame.bottom = previewFrame.top + MulDiv(840, static_cast<int>(requestedDpi), 96);
            (void)SendMessageW(_printPreview->Host().GetHwnd(), WM_DPICHANGED, MAKEWPARAM(requestedDpi, requestedDpi), reinterpret_cast<LPARAM>(&previewFrame));
            RequireCapture(SetWindowPos(_printPreview->Host().GetHwnd(),
                                        nullptr,
                                        previewFrame.left,
                                        previewFrame.top,
                                        previewFrame.right - previewFrame.left,
                                        previewFrame.bottom - previewFrame.top,
                                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                           "Cannot size the owned hidden print-preview window");
        }
        for (unsigned prep = 0; prep < 64u && _printPreview->Host().HasPendingContentPreparation(); ++prep)
        {
            (void)_printPreview->Host().RenderInitialFrameForShow();
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
        RequireCapture(! IsWindowVisible(_window.get()) && ! IsWindowVisible(_printPreview->Host().GetHwnd()),
                       "Print preview capture windows must remain hidden");
#else
        RequireCapture(false, "The synthetic print-preview capture requires a test-enabled build");
#endif
    }
    // The published Settings fixture declares System Light rather than inheriting
    // the user's ambient preference. Normal ApplyTheme keeps resolving Windows
    // outside this headless capture path, which already excludes performance.
    if (scenario == L"settings" || scenario == L"settings-about")
        _systemDark = false;
    if (! _printPreview)
    {
        Layout();
        Prepare(512);
    }
    auto& activeHost = _printPreview ? _printPreview->Host() : _host;
    RequireCapture(! IsWindowVisible(_window.get()) && (! _printPreview || ! IsWindowVisible(_printPreview->Host().GetHwnd())) &&
                       activeHost.RenderInitialFrameForShow(),
                   "Design captures must render on an owned hidden window");
    const auto actual            = activeHost.GetClientBoundsDip();
    const auto scenarioUtf8      = DxUi::RichText::EncodeUtf8(_options.scenario);
    const auto fixtureUtf8       = DxUi::RichText::EncodeUtf8(fixtureName);
    const auto captureBackground = activeHost.GetTheme().windowBackground;
    const auto metadata          = std::format(
        "{{\n  \"schemaVersion\": 1,\n  \"scenario\": \"{}\",\n  \"fixtureBasename\": \"{}\",\n"
        "  \"effectiveSystemPreference\": \"{}\",\n  \"systemPreferenceSource\": \"{}\",\n  \"renderedAppearance\": \"{}\",\n"
        "  \"strikeCompletedTasks\": {},\n"
        "  \"imageFirstPaste\": {},\n"
        "  \"requestedDpi\": {},\n  \"nativeWindowDpi\": {},\n  \"actualHostDpi\": {},\n  \"simulatedDpi\": {},\n"
        "  \"requestedWidthPx\": {},\n  \"requestedHeightPx\": {},\n  \"actualWidthPx\": {},\n  \"actualHeightPx\": {},\n"
        "  \"actualWidthDip\": {},\n  \"actualHeightDip\": {},\n  \"filesModelFrozen\": {},\n  \"reparseFixtureRemoved\": {},\n  \"hidden\": true,\n"
        "  \"qualification\": \"Owned hidden application render; no physical-monitor or native-platform acceptance\"\n}}\n",
        scenarioUtf8,
        fixtureUtf8,
        _systemDark ? "dark" : "light",
        scenario == L"settings" || scenario == L"settings-about" ? "synthetic-fixture" : "windows-preference-or-light-fallback",
        activeHost.GetTheme().highContrast                                       ? "high-contrast"
        : captureBackground.r + captureBackground.g + captureBackground.b < 1.5f ? "dark"
                                                                                 : "light",
        _settings.strikeCompletedTasks ? "true" : "false",
        _settings.imageFirstPaste ? "true" : "false",
        _options.captureDpi,
        nativeDpi,
        activeHost.GetDpi(),
        requestedDpi != nativeDpi ? "true" : "false",
        widthPx,
        heightPx,
        activeHost.DebugGetWidthPx(),
        activeHost.DebugGetHeightPx(),
        actual.right - actual.left,
        actual.bottom - actual.top,
        frozenFilesModel ? "true" : "false",
        reparseRemoved ? "true" : "false");
    auto sidecar = _options.capture;
    sidecar += L".scenario.json";
    RequireCapture(SUCCEEDED(WriteBytesAtomic(sidecar, metadata)), "Cannot preserve the capture's actual viewport/DPI/scenario provenance");
}
} // namespace RedTilde
