#include "Application.Tests.Printing.h"
#include "RedTilde/Application.h"
#include "RedTilde/PrintPreview.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <format>
#include <stdexcept>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view detail)
{
    if (!condition)
        throw std::runtime_error(std::string(detail));
}

Printing::Paper SmallPaper()
{
    return {{440.f, 360.f}, {16.f, 26.f, 424.f, 326.f}};
}

Printing::Paper WidePaper()
{
    return {{980.f, 640.f}, {90.f, 36.f, 965.f, 613.f}};
}

std::wstring MultipageSource()
{
    std::wstring result = L"# Redfern notebook\n\n- [x] Check type\n- [ ] Review output\n\n| Sample | Value |\n|---|---|\n| Tokyo | 東京 |\n\n";
    for (unsigned line = 1; line <= 32; ++line)
        result += std::format(L"\n**Redfern {}** carries a distinct page marker.\n", line);
    result += L"\n$$\\frac{1}{2}$$\n\n```cpp\nint sample = 4;\n```\n";
    return result;
}
} // namespace

class PrintingProbe
{
  public:
    static void Wait(Application &app, bool preview = false)
    {
        const auto start = GetTickCount64();
        while (app.printOperation && (!preview || !app.printPreview->CanPrint()) && GetTickCount64() - start < 15000u)
        {
            MSG message{};
            if (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            else
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        Require(preview ? app.printOperation && app.printPreview->CanPrint() : !app.printOperation,
                "print worker publishes the requested preview or terminal result");
    }

    static void Choose(Application &app)
    {
        Require(app.printPreview && app.printPreview->CanPrint(), "a completed preview offers the native print command");
        const auto window = app.printPreview->Window();
        auto &host = app.printPreview->Host();
        Require(host.GetDefaultButton()->Invoke(host, false), "invoke the preview Print button without physical input");
        MSG message{};
        while (IsWindow(window) && PeekMessageW(&message, window, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
    }

    static void Open(Application &app)
    {
        app.Execute(Command::Print);
        Require(app.printPreview && app.printOperation && !app.printPreview->CanPrint(), "print opens the preview and waits for page content");
        Wait(app, true);
    }

    template <typename Control> static Control &Find(Application &app, unsigned label)
    {
        auto &host = app.printPreview->Host();
        auto *const parent = host.GetRoot();
        for (std::size_t index = 0; index < parent->GetLogicalChildCount(); ++index)
            if (auto *control = dynamic_cast<Control *>(parent->GetLogicalChild(index)); control && control->GetAccessibleName() == Resource(label))
                return *control;
        throw std::runtime_error("the preview exposes the named control");
    }

    static void Navigate(Application &app, unsigned page)
    {
        auto &field = Find<DxUi::TextField>(app, IDS_PRINTPREVIEWPAGE);
        field.SetTextAndNotify(std::to_wstring(page));
        Require(field.OnKeyDown(app.printPreview->Host(), VK_RETURN, 0), "Enter in the page field jumps to the numbered page");
        Wait(app, true);
        Require(app.printPreview->Page().number == page, "the preview displays the requested page");
    }

    static bool HasInk(const Printing::PreviewPage &page)
    {
        for (std::size_t index = 0; index + 4u <= page.bgra.size(); index += 4u)
            if (page.bgra[index] < 96u && page.bgra[index + 1u] < 96u && page.bgra[index + 2u] < 96u)
                return true;
        return false;
    }

    static void StateAndRanges(const std::filesystem::path &directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        HRESULT dialogResult = S_FALSE;
        bool confirmed = true;
        auto paper = SmallPaper();
        unsigned openings{};
        std::vector<unsigned> pages;
        LaunchOptions options;
        options.headless = true;
        options.forceWarp = true;
        options.dataDirectory = directory;
        options.printPaperForTest = SmallPaper();
        options.printDialogForTest = [&](bool, Printing::DialogResult &selected)
        {
            ++openings;
            selected.confirmed = confirmed;
            selected.selection.paper = paper;
            selected.selection.printer = L"Redfern document printer";
            return dialogResult;
        };
        options.printPageForTest = [&](unsigned page, ID2D1CommandList &)
        {
            pages.push_back(page);
            return S_OK;
        };
        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize the hidden print owner");
        Require(app.SetDocument(MultipageSource()), "seed a synthetic untitled document");
        app.Document().SetSelection({0u, 0u});
        Require(app.Document().ReplaceSelection(L"Unsaved text\n\n"), "insert an unsaved edit through the authoritative editor");
        app.Document().SetSelection({2u, 8u});
        const auto snapshot = app.Document().Snapshot();
        const auto selection = app.Document().GetSelection();
        const auto menuCommand = static_cast<int>(Command::Print) + 1;
        app.Host().SetFocusControl(&app.Document(), false);
        app.OnMenuClosed(menuCommand);
        Require(app.printPreview && !openings && !app.printPreview->CanPrint(), "File Print opens a preview before asking for a destination");
        const auto firstPreview = app.printPreview->Window();
        Require(GetWindow(firstPreview, GW_OWNER) == app.Window() && !IsWindowVisible(firstPreview), "the hidden preview has the editor as owner");
        Wait(app, true);
        const auto &first = app.printPreview->Page();
        Require(first.number == 1u && first.count >= 4u && first.width == 660u && first.height == 540u && first.bgra.size() == 660u * 540u * 4u &&
                    first.bgra.front() == 255u && HasInk(first),
                "the first preview is the paper-sized rendered page with ink and a white margin");
        const auto firstPixels = first.bgra;
        const auto count = first.count;
        auto &previous = Find<DxUi::Button>(app, IDS_PRINTPREVIOUS);
        auto &next = Find<DxUi::Button>(app, IDS_PRINTNEXT);
        Require(previous.GetText() == L"←" && next.GetText() == L"→" && previous.GetBounds().right < next.GetBounds().left && !previous.IsEnabled() &&
                    next.IsEnabled(),
                "the left arrow is Previous and is disabled on the first page; the right arrow is Next");
        Require(next.Invoke(app.printPreview->Host(), false), "choose the right arrow");
        Wait(app, true);
        Require(app.printPreview->Page().number == 2u, "the right arrow advances one page");
        Require(previous.Invoke(app.printPreview->Host(), false), "choose the left arrow");
        Wait(app, true);
        Require(app.printPreview->Page().number == 1u, "the left arrow goes back one page");
        Navigate(app, 2u);
        Require(HasInk(app.printPreview->Page()) && app.printPreview->Page().bgra != firstPixels, "navigation presents distinct rendered sheets");
        app.Execute(Command::ThemeDark);
        std::filesystem::create_directories(directory);
        Require(app.Capture(directory / L"print-preview-page-2.png"), "the real preview frame captures through the hidden application host");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app.printPreview->Host(), false), "select the displayed page for printing");
        Choose(app);
        Require(openings == 1u && pages.empty() && app.printPreview->Page().number == 2u && app.printPreview->CanPrint() &&
                    Find<DxUi::TextField>(app, IDS_PRINTPAGES).GetText() == L"2",
                "cancelling the destination dialog preserves the visible sheet and selected page");
        Require(app.printPreview->Host().GetCancelButton()->Invoke(app.printPreview->Host(), false), "cancel through the preview's Close button");
        MSG message{};
        while (IsWindow(firstPreview) && PeekMessageW(&message, firstPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(!app.printOperation && !app.printPreview && app.Host().GetFocusControl() == &app.Document() && pages.empty(),
                "closing the preview cancels work and returns the editor focus without submission");

        dialogResult = S_OK;
        for (const auto view : {Command::Compose, Command::Source, Command::Split})
        {
            app.Execute(view);
            pages.clear();
            app.OnMenuClosed(menuCommand);
            Wait(app, true);
            Require(app.printPreview->Page().bgra == firstPixels, "every editor view previews the same formatted page");
            const auto active = app.printPreview->Window();
            const auto admitted = openings;
            app.Execute(Command::Print);
            Require(openings == admitted && app.printPreview->Window() == active, "a duplicate command cannot supersede the admitted preview");
            Choose(app);
            Wait(app);
            Require(app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED) && pages.size() >= 4u && pages.front() == 1u &&
                        pages.back() == pages.size(),
                    "the print command records every page in order");
            Require(count == pages.size(), "every editor view uses the same pagination");
            Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection && app.Document().CanUndo() &&
                        app.Dirty() && app.documentPath.empty(),
                    "printing preserves unsaved source, selection, history and identity");
        }
        pages.clear();
        Open(app);
        Find<DxUi::TextField>(app, IDS_PRINTPAGES).SetTextAndNotify(std::format(L" {}, 1–2, 2 ", count));
        Choose(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u, 2u, count}), "disjoint and overlapping requests print once in document order");
        pages.clear();
        Open(app);
        auto &rangeField = Find<DxUi::TextField>(app, IDS_PRINTPAGES);
        for (const auto &invalid : {L"0", L"2-1", L"1,", L"999999999999999999999999", L"1-2-3"})
        {
            const auto before = openings;
            rangeField.SetTextAndNotify(invalid);
            Choose(app);
            Require(openings == before && pages.empty() && app.printPreview->CanPrint() && rangeField.GetText() == invalid &&
                        app.printPreview->Host().GetFocusControl() == &rangeField,
                    "invalid page selection focuses the field without opening the Windows dialog or sending pages");
        }

        paper = WidePaper();
        rangeField.SetTextAndNotify(std::to_wstring(count));
        Choose(app);
        Require(app.printPreview && !app.printPreview->CanPrint() && pages.empty(), "changing paper defers submission before showing the new pages");
        Wait(app, true);
        const auto &landscape = app.printPreview->Page();
        Require(landscape.paper == paper && landscape.width > landscape.height && landscape.count < count && HasInk(landscape),
                "landscape uses its own page geometry and recomputes the page count");
        const auto landscapeCount = landscape.count;
        const auto beforeCorrection = openings;
        Choose(app);
        Require(rangeField.GetText() == std::to_wstring(count) && openings == beforeCorrection && pages.empty(),
                "paper changes preserve the written range and require correction before the next dialog");
        confirmed = false;
        rangeField.SetTextAndNotify(std::to_wstring(landscapeCount));
        Choose(app);
        Require(rangeField.GetText() == std::to_wstring(landscapeCount) && pages.empty(), "Apply preserves an explicit range without printing");
        confirmed = true;
        Require(landscapeCount >= 2u, "the landscape fixture still has more than one page");
        Require(app.SetDocument(L"Second document."), "switch the editor while the preview retains the original revision");
        Navigate(app, landscapeCount);
        Require(!Find<DxUi::Button>(app, IDS_PRINTNEXT).IsEnabled(), "Next is disabled on the last sheet");
        Require(HasInk(app.printPreview->Page()), "the preview still renders the last page of the admitted document after an editor change");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app.printPreview->Host(), false), "select the displayed last page for printing");
        Navigate(app, 1u);
        Choose(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{landscapeCount}) && app.Document().GetText() == L"Second document.",
                "Current page selects exactly the displayed sheet at the time of selection, from the admitted revision");

        pages.clear();
        Require(app.SetDocument(L""), "seed an empty document");
        Open(app);
        Require(app.printPreview->Page().count == 1u && !HasInk(app.printPreview->Page()), "an empty document previews one blank sheet");
        Choose(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u}), "an empty document records one blank sheet");
        Require(foreground.NeverAcquiredForeground() && !IsWindowVisible(app.Window()), "directed printing keeps the editor hidden");
    }

    static void FailureAndShutdown(const std::filesystem::path &directory)
    {
        auto releasePage = wil::unique_event(wil::EventOptions::ManualReset);
        auto enteredPage = wil::unique_event(wil::EventOptions::ManualReset);
        auto returnedPage = wil::unique_event(wil::EventOptions::ManualReset);
        std::atomic_uint calls{};
        std::atomic_bool pause{};
        bool failPage = true, failFinish = false, failDialog = true;
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = directory;
        options.unsavedPromptForTest = [] { return IDNO; };
        options.printPaperForTest = Printing::Paper{};
        options.printDialogForTest = [&](bool, Printing::DialogResult &selected)
        {
            if (failDialog)
                return E_FAIL;
            selected.selection.paper = SmallPaper();
            selected.selection.printer = L"Redfern document printer";
            selected.confirmed = true;
            return S_OK;
        };
        options.printPageForTest = [&](unsigned page, ID2D1CommandList &)
        {
            ++calls;
            const HRESULT result = failPage && page == 2u ? E_ACCESSDENIED : S_OK;
            if (pause.load())
            {
                enteredPage.SetEvent();
                const bool signalled = releasePage.wait(10000u);
                returnedPage.SetEvent();
                if (!signalled)
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            return result;
        };
        options.printFinishForTest = [&] { return failFinish ? E_FAIL : S_OK; };
        Application app(std::move(options));
        const auto release = wil::scope_exit(
            [&]
            {
                releasePage.SetEvent();
                if (enteredPage.wait(0u))
                    (void)returnedPage.wait(10000u);
            });
        Require(SUCCEEDED(app.Initialize()), "initialize the print error fixture");
        Require(app.SetDocument(MultipageSource()), "seed enough pages for a mid-job failure");
        app.Execute(Command::Print);
        Wait(app);
        Require(!app.printPreview && !calls && app.feedback->GetText() == Resource(IDS_PRINTPREVIEWFAILED),
                "preview failure clears the session before reporting the error");
        app.options.printPaperForTest = SmallPaper();
        app.Execute(Command::Print);
        app.DismissPrintPreview();
        Require(!app.printOperation && !app.printPreview && !calls, "cancelling during initial pagination detaches without printing");
        Open(app);
        Choose(app);
        Require(!calls && app.printPreview->CanPrint() && app.feedback->GetText() == Resource(IDS_PRINTDIALOGFAILED),
                "a failed Windows chooser leaves the preview open and names the failing step");
        failDialog = false;
        Choose(app);
        Wait(app);
        Require(calls == 2u && app.feedback->GetText() == L"Couldn’t print page 2. Destination Redfern document printer.",
                "a failed page names the page and printer and stops submission without a retry");
        failPage = false;
        failFinish = true;
        Open(app);
        Choose(app);
        Wait(app);
        Require(calls > 2u && app.feedback->GetText() == L"Couldn’t finish the print job. Destination Redfern document printer.",
                "finalization failure names the printer and the separate completion step");
        failFinish = false;
        Open(app);
        Choose(app);
        Wait(app);
        Require(app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED), "a new job succeeds after recoverable failures");

        const auto previous = calls.load();
        pause.store(true);
        Open(app);
        Choose(app);
        Require(enteredPage.wait(10000u), "worker reaches a controlled blocked printer page");
        app.Execute(Command::Close);
        Require(!app.printOperation && app.closing, "closing detaches the owned print job while the worker is blocked");
        releasePage.SetEvent();
        Require(returnedPage.wait(10000u), "the submitted page returns after the owner closes");
        Require(calls == previous + 1u, "shutdown cancels before the next page");
    }

    static void DiagramBackground(const std::filesystem::path &directory)
    {
        LaunchOptions options;
        options.headless = true;
        options.forceWarp = true;
        options.dataDirectory = directory;
        options.printPaperForTest = WidePaper();
        options.printDialogForTest = [](bool, Printing::DialogResult &) { return S_FALSE; };
        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize the hidden diagram document");
        app.Execute(Command::ThemeDark);
        Require(app.SetDocument(L"```mermaid\nflowchart LR\nsubgraph Dispatch\nA[Write] --> B[Review]\nend\n```\n"),
                "seed a native graph with a surrounding group");
        std::filesystem::create_directories(directory);
        Require(app.Capture(directory / L"graph-on-editor-canvas.png"), "capture the graph against the dark editor canvas");
        const auto semantic = app.Document().GetSemanticDocument();
        Require(static_cast<bool>(semantic), "the editor publishes the fixture's semantic layout");
        const auto elements = semantic->Elements();
        const auto diagram = std::ranges::find(elements, DxUi::RichText::SemanticRole::Diagram, &DxUi::RichText::SemanticElement::role);
        Require(diagram != elements.end(), "the graph is a native diagram in the editor");
        const auto bounds = app.Document().GetSemanticElementBounds(app.Host(), static_cast<std::size_t>(diagram - elements.begin()));
        DxUi::WindowHostBitmapCapture capture;
        Require(bounds && app.Host().DebugCaptureBitmap(capture, true), "read the diagram pixels from the owned editor");
        const float scale = app.Host().GetDpi() / 96.f;
        const auto left = static_cast<UINT>(bounds.value().left * scale);
        const auto middle = static_cast<UINT>((bounds.value().top + bounds.value().bottom) * .5f * scale);
        Require(left > 4u && left + 4u < capture.widthPx && middle < capture.heightPx, "graph margin samples fit the editor frame");
        const auto canvas = (static_cast<std::size_t>(middle) * capture.widthPx + left - 4u) * 4u;
        const auto margin = canvas + 32u;
        Require(std::equal(capture.bgraPixels.data() + canvas, capture.bgraPixels.data() + canvas + 3u, capture.bgraPixels.data() + margin),
                "the diagram margin matches the adjacent editor canvas");

        Open(app);
        const auto &page = app.printPreview->Page();
        Require(page.count == 1u && page.bgra.size() == static_cast<std::size_t>(page.width) * page.height * 4u,
                "the graph fits one printed sheet with a complete raster");
        unsigned leftInk = page.width, topInk = page.height, bottomInk{};
        for (unsigned y = 0u; y < page.height; ++y)
            for (unsigned x = 0u; x < page.width; ++x)
            {
                const auto offset = (static_cast<std::size_t>(y) * page.width + x) * 4u;
                if (page.bgra[offset] < 96u && page.bgra[offset + 1u] < 96u && page.bgra[offset + 2u] < 96u)
                {
                    leftInk = std::min(leftInk, x);
                    topInk = std::min(topInk, y);
                    bottomInk = y;
                }
            }
        Require(leftInk > 4u && leftInk < page.width && topInk < bottomInk, "the graph still paints visible ink on the printed sheet");
        const auto paperSample = (static_cast<std::size_t>((topInk + bottomInk) / 2u) * page.width + leftInk - 4u) * 4u;
        Require(page.bgra[paperSample] == 255u && page.bgra[paperSample + 1u] == 255u && page.bgra[paperSample + 2u] == 255u,
                "the paper stays white in the margin immediately outside the diagram");
        Require(app.Capture(directory / L"graph-on-print-paper.png"), "capture the real preview of the graph on paper");
        app.DismissPrintPreview();
    }
};

void RegisterPrintingTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.PrintingPreservesRevisionAndHonorsPageRanges", [&] { PrintingProbe::StateAndRanges(root / L"print-pages"); });
    test("Editor.PrintingFailureAndShutdownRetireJobs", [&] { PrintingProbe::FailureAndShutdown(root / L"print-shutdown"); });
    test("Editor.PrintingDiagramFollowsCanvasAndPaper", [&] { PrintingProbe::DiagramBackground(root / L"print-diagram"); });
}
} // namespace RedTilde
