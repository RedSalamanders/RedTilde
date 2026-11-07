#include "Application.h"
#include "Tests/Support/Support.Tests.FilesFixtures.h"
#include <algorithm>
#include <chrono>
#include <combaseapi.h>
#include <format>
#include <stdexcept>

namespace RedTilde
{
namespace
{
void RequireCapture(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void CaptureWrite(const std::filesystem::path &path, std::string_view text = "# Invented reference notes\n")
{
    std::filesystem::create_directories(path.parent_path());
    const HRESULT result = WriteBytesAtomic(path, text);
    if (FAILED(result))
        throw std::runtime_error(std::format("Cannot seed the owned design-capture fixture (0x{:08x}, {} path units)", static_cast<unsigned>(result),
                                             path.native().size()));
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
    if (!options.headless || options.capture.empty() || !options.performance.empty())
        return;
    RequireCapture(!options.scenario.empty() &&
                       std::ranges::all_of(options.scenario, [](wchar_t value)
                                           { return (value >= L'a' && value <= L'z') || (value >= L'0' && value <= L'9') || value == L'-'; }),
                   "A capture scenario must use a lowercase ASCII identifier");
    const UINT nativeDpi = GetDpiForWindow(window.get());
    const UINT requestedDpi = options.captureDpi ? options.captureDpi : nativeDpi;
    RequireCapture(requestedDpi >= 96 && requestedDpi <= 384, "Capture DPI must be between 96 and 384");
    const unsigned defaultWidth = options.scenario == L"narrow" ? 800u : 1440u;
    const unsigned defaultHeight = options.scenario == L"narrow" ? 560u : 900u;
    const int widthPx = options.viewportWidthPx ? static_cast<int>(options.viewportWidthPx)
                                                : MulDiv(static_cast<int>(defaultWidth), static_cast<int>(requestedDpi), 96);
    const int heightPx = options.viewportHeightPx ? static_cast<int>(options.viewportHeightPx)
                                                  : MulDiv(static_cast<int>(defaultHeight), static_cast<int>(requestedDpi), 96);
    RequireCapture(widthPx >= 800 && widthPx <= 7680 && heightPx >= 560 && heightPx <= 4320, "Capture viewport is outside its bounded dimensions");
    RequireCapture(static_cast<unsigned>(widthPx) * 96 >= 800u * requestedDpi && static_cast<unsigned>(heightPx) * 96 >= 560u * requestedDpi,
                   "The capture viewport must retain the 800 by 560 DIP application minimum");
    RECT frame{};
    RequireCapture(GetWindowRect(window.get(), &frame) != FALSE, "Cannot read the owned hidden capture frame");
    frame.right = frame.left + widthPx;
    frame.bottom = frame.top + heightPx;
    // Only this hidden owner HWND receives the synthetic change. The sidecar
    // distinguishes its layout exercise from physical-monitor DPI qualification.
    (void)SendMessageW(window.get(), WM_DPICHANGED, MAKEWPARAM(requestedDpi, requestedDpi), reinterpret_cast<LPARAM>(&frame));
    RequireCapture(
        SetWindowPos(window.get(), nullptr, frame.left, frame.top, widthPx, heightPx, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
        "Cannot size the owned hidden capture window");
    Layout();

    const auto &scenario = options.scenario;
    std::wstring fixtureName = options.document.filename().wstring();
    const bool clipboardSettings = scenario == L"image-paste-light" || scenario == L"image-paste-dark";
    if (clipboardSettings)
    {
        SetDocument(WritingFixture());
        Execute(scenario == L"image-paste-dark" ? Command::ThemeDark : Command::ThemeLight);
        settings.view = EditorView::Compose;
        settings.showFiles = false;
        settings.showOutline = false;
        const bool imageFirst = scenario == L"image-paste-light";
        if (settings.imageFirstPaste != imageFirst)
            Execute(Command::ImageFirstPaste);
        Execute(Command::Settings);
        Prepare(512);
        settingsScroll->SetScrollOffset(1240);
        RequireCapture(host.GetClipboardPreference() ==
                           (imageFirst ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst),
                       "The clipboard preference capture must expose the actual live host policy");
        const auto background = host.GetTheme().windowBackground;
        RequireCapture(!host.GetTheme().highContrast && (background.r + background.g + background.b < 1.5f) == !imageFirst,
                       "The clipboard preference capture must apply its declared native Light or Dark palette");
        fixtureName = L"Clipboard picture preferences";
    }
    if (scenario == L"diagram-label-light" || scenario == L"diagram-label-dark")
    {
        SetDocument(L"# Notebook review\n\n```mermaid\nstateDiagram-v2\ndirection LR\n"
                    L"[*] --> Draft\nDraft --> Reviewed: Review\nReviewed --> Saved: Save\n```\n\n"
                    L"## Message labels\n\n```mermaid\nsequenceDiagram\nparticipant Writer\nparticipant Editor\n"
                    L"Writer->>Editor: Review draft\nEditor-->>Writer: Ready to save\n```\n");
        settings.appearance = scenario == L"diagram-label-dark" ? Appearance::Dark : Appearance::Light;
        settings.view = EditorView::Compose;
        settings.showFiles = false;
        settings.showOutline = false;
        fixtureName = L"Notebook review";
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
        settings.appearance = scenario == L"quote-scroll-dark" ? Appearance::Dark : Appearance::Light;
        settings.view = EditorView::Compose;
        settings.showFiles = false;
        settings.showOutline = false;
        fixtureName = L"Notebook observations";
    }
    if (scenario == L"image-consent-light" || scenario == L"image-consent-dark")
    {
        settings.allowRemoteImages = false;
        settings.allowLocalNetworkImages = false;
        documentRemoteImagesAllowed = false;
        editor->SetRemoteImagesEnabled(false);
        SetDocument(L"# Illustration notebook\n\n"
                    L"Remote illustrations load only after a choice for this document.\n\n"
                    L"![Invented landscape](https://images.example.invalid/landscape.png)\n\n"
                    L"The source, selection and undo history remain yours.\n");
        Execute(scenario == L"image-consent-dark" ? Command::ThemeDark : Command::ThemeLight);
        const auto background = host.GetTheme().windowBackground;
        RequireCapture(!host.GetTheme().highContrast && (background.r + background.g + background.b < 1.5f) == (scenario == L"image-consent-dark"),
                       "The image consent capture must apply its declared native Light or Dark palette");
        settings.view = EditorView::Compose;
        settings.showFiles = false;
        settings.showOutline = false;
        fixtureName = L"Illustration notebook";
    }
    if (scenario == L"task-presentation-light" || scenario == L"task-presentation-dark")
        settings.strikeCompletedTasks = true;
    const bool reference = scenario == L"reference-dark";
    const bool fixtureScene = scenario == L"compose-light" || scenario == L"compose-dark" || scenario == L"source" || scenario == L"split" ||
                              scenario == L"settings" || scenario == L"narrow" || scenario == L"high-contrast" || scenario == L"files" ||
                              scenario == L"find" || scenario == L"focus" || scenario == L"equation" || reference;
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
        fixtureName = L"Markdown styling example";
        settings.view = EditorView::Compose;
        settings.showFiles = false;
        settings.showOutline = false;
    }
    if (fixtureScene)
    {
        GUID id{};
        RequireCapture(SUCCEEDED(CoCreateGuid(&id)), "Cannot identify the unique owned capture fixture");
        const auto fixture =
            std::filesystem::absolute(options.dataDirectory) /
            std::format(L"CaptureFixture-{:08x}-{:04x}-{:04x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}", id.Data1, id.Data2, id.Data3,
                        id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3], id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
        const auto writing = fixture / (reference ? L"Reports" : L"Writing");
        if (reference)
        {
            RequireCapture(!options.document.empty(), "The reference capture requires its exact reviewed report document");
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
        const auto target = fixture / L"Reference target";
        CaptureWrite(target / L"Not followed.md", "# Reparse target\n\nThe Files tree must never visit this entry.\n");
        const auto link = (reference ? fixture : writing) / L"Linked notes";
        RequireCapture(SUCCEEDED(Testing::CreateFixtureJunction(fixture, link, target)), "Cannot create the owned reparse-point capture fixture");
        auto removeOwnedLink = wil::scope_exit(
            [&]
            {
                folderFiles.Close();
                folderFilesActive = false;
                if (!RemoveDirectoryW(link.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
                    OutputDebugStringW(L"RedTilde capture could not remove its owned reparse fixture leaf.\n");
            });
        auto source = reference ? CurrentText() : WritingFixture();
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
        settings.filesRoot = writing;
        SetDocument(std::move(source), path, false, identity);
        fixtureName = reference ? L"weekly-report-2026-10-15.md" : L"Writing";
        if (scenario != L"focus" && scenario != L"equation" && scenario != L"settings" && !settings.showFiles)
            Execute(Command::Files);
        if (settings.showFiles)
        {
            const auto deadline = GetTickCount64() + 10000;
            unsigned turns{};
            while ((!folderSnapshot || folderSnapshot->root != writing || folderFiles.Busy()) && GetTickCount64() < deadline && turns++ < 2048)
            {
                Prepare(8);
                if (!folderSnapshot || folderSnapshot->root != writing || folderFiles.Busy())
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            RequireCapture(folderSnapshot && folderSnapshot->root == writing && !folderFiles.Busy() && SUCCEEDED(folderSnapshot->status) &&
                               SUCCEEDED(folderSnapshot->watchStatus),
                           "The owned Files capture fixture did not complete its bounded scan/watch preparation");
            RequireCapture(folderSnapshot->markdownCount == (reference ? 20u : 6u) &&
                               std::ranges::none_of(folderSnapshot->entries, [](const auto &entry) { return entry.name == L"Not followed.md"; }),
                           "Files capture fixture included a hidden/system/non-Markdown/reparse-target entry");
            if (reference)
            {
                collapsedFolders.insert(writing / L"archive");
                collapsedFolders.insert(writing / L"templates");
                collapsedFolders.erase(writing / L"notes");
                RefreshFiles();
            }
            const auto draft = std::find_if(filesModel.rows.begin(), filesModel.rows.end(), [](const auto &row) { return row.text == L"Drafts"; });
            if (draft != filesModel.rows.end())
                filesModel.OnTreeToggleExpanded(draft->id, true);
            // Freeze this proved immutable listing while evidence retains only
            // ordinary files/directories. Closing invalidates future delivery.
            folderFiles.Close();
            folderFilesActive = false;
            frozenFilesModel = true;
        }
        RequireCapture(RemoveDirectoryW(link.c_str()) != FALSE, "Cannot retire the owned capture reparse leaf before evidence retention");
        reparseRemoved = true;
        removeOwnedLink.release();
        const auto end = editor->Snapshot().Length();
        editor->SetSelection({end, end});
        RequireCapture(editor->ReplaceSelection(reference ? L"\r\n" : L"\n"), "Cannot stage the synthetic Edited state");
        editor->SetSelection({0, 0});
        SelectionChanged();
        (void)editor->ScrollSourceRangeIntoView(host, 0, 0, true);
        Prepare(512);
        if (reference)
        {
            const auto anchor = editor->GetText().find(L"report snapshot.");
            RequireCapture(anchor != std::wstring::npos, "Reference report caret anchor must exist");
            const auto caret = anchor + std::wstring_view(L"report snapshot.").size();
            RequireCapture(caret < editor->Snapshot().Length(), "Reference report caret anchor must exist");
            editor->SetSelection({caret, caret});
            SelectionChanged();
            host.SetFocusControl(editor, false);
            RequireCapture(documentInfo.headings.size() == 7 && outlineTree->GetSelectedItemId() == 2,
                           "Reference report exposes the same seven headings and current section as image 2");
        }
        if (scenario == L"find")
        {
            Execute(Command::Find);
            findText->SetTextAndNotify(L"writing");
            replaceText->SetTextAndNotify(L"drafting");
            editor->SetSelection({0, 0});
            Execute(Command::FindNext);
            Execute(Command::FindNext);
            RequireCapture(findBar->IsVisible() && findMatches.size() >= 2, "The Find capture did not expose its deterministic query state");
        }
        if (scenario == L"focus")
        {
            const auto caret = editor->GetText().find(L"Give this paragraph");
            RequireCapture(caret != std::wstring::npos, "Focus fixture paragraph is missing");
            editor->SetSelection({caret, caret});
            SelectionChanged();
            if (!settings.focusMode)
                Execute(Command::Focus);
            RequireCapture(settings.focusMode && editor->IsFocusModeEnabled(), "The Focus capture did not enter the shared dimming presentation");
            Prepare(512);
            (void)editor->ScrollSourceRangeIntoView(host, caret, caret, true);
        }
        if (scenario == L"equation")
        {
            const auto caret = editor->GetText().find(L"\\frac");
            RequireCapture(caret != std::wstring::npos, "Equation fixture source is missing");
            editor->SetSelection({caret, caret});
            SelectionChanged();
            Execute(Command::LocalSource);
            const auto region = editor->GetSemanticSourceRegion();
            RequireCapture(region && !HasBlockDraft() && editor->GetText().compare(region->source.first, 2, L"$$") == 0,
                           "The equation capture did not show its exact TeX source in place");
        }
        if (scenario == L"settings")
        {
            if (drawer->IsVisible())
                Execute(Command::CloseDrawer);
            Execute(Command::Settings);
        }
        if (scenario == L"files" || scenario == L"compose-light" || scenario == L"compose-dark")
            host.SetFocusControl(editor, false);
    }
    // The published Settings fixture declares System Light rather than inheriting
    // the user's ambient preference. Normal ApplyTheme keeps resolving Windows
    // outside this headless capture path, which already excludes performance.
    if (scenario == L"settings")
        systemDark = false;
    Layout();
    Prepare(512);
    RequireCapture(!IsWindowVisible(window.get()) && host.RenderInitialFrameForShow(), "Design captures must render on an owned hidden window");
    const auto actual = host.GetClientBoundsDip();
    const auto scenarioUtf8 = DxUi::RichText::EncodeUtf8(options.scenario);
    const auto fixtureUtf8 = DxUi::RichText::EncodeUtf8(fixtureName);
    const auto captureBackground = host.GetTheme().windowBackground;
    const auto metadata = std::format(
        "{{\n  \"schemaVersion\": 1,\n  \"scenario\": \"{}\",\n  \"fixtureBasename\": \"{}\",\n"
        "  \"effectiveSystemPreference\": \"{}\",\n  \"systemPreferenceSource\": \"{}\",\n  \"renderedAppearance\": \"{}\",\n"
        "  \"strikeCompletedTasks\": {},\n"
        "  \"imageFirstPaste\": {},\n"
        "  \"requestedDpi\": {},\n  \"nativeWindowDpi\": {},\n  \"actualHostDpi\": {},\n  \"simulatedDpi\": {},\n"
        "  \"requestedWidthPx\": {},\n  \"requestedHeightPx\": {},\n  \"actualWidthPx\": {},\n  \"actualHeightPx\": {},\n"
        "  \"actualWidthDip\": {},\n  \"actualHeightDip\": {},\n  \"filesModelFrozen\": {},\n  \"reparseFixtureRemoved\": {},\n  \"hidden\": true,\n"
        "  \"qualification\": \"Owned hidden application render; no physical-monitor or native-platform acceptance\"\n}}\n",
        scenarioUtf8, fixtureUtf8, systemDark ? "dark" : "light",
        scenario == L"settings" ? "synthetic-fixture" : "windows-preference-or-light-fallback",
        host.GetTheme().highContrast                                             ? "high-contrast"
        : captureBackground.r + captureBackground.g + captureBackground.b < 1.5f ? "dark"
                                                                                 : "light",
        settings.strikeCompletedTasks ? "true" : "false", settings.imageFirstPaste ? "true" : "false", options.captureDpi, nativeDpi, host.GetDpi(),
        requestedDpi != nativeDpi ? "true" : "false", widthPx, heightPx, host.DebugGetWidthPx(), host.DebugGetHeightPx(), actual.right - actual.left,
        actual.bottom - actual.top, frozenFilesModel ? "true" : "false", reparseRemoved ? "true" : "false");
    auto sidecar = options.capture;
    sidecar += L".scenario.json";
    RequireCapture(SUCCEEDED(WriteBytesAtomic(sidecar, metadata)), "Cannot preserve the capture's actual viewport/DPI/scenario provenance");
}
} // namespace RedTilde
