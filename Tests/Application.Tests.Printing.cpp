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

constexpr std::wstring_view OfficePrinter = L"Redfern office printer", PdfPrinter = L"Redfern PDF writer";

HRESULT ConfigureFixture(const std::wstring &name, const std::optional<Printing::PrinterSettings> &requested,
                         Printing::PrinterConfiguration &configuration)
{
    using namespace Printing;
    const bool pdf = name == PdfPrinter;
    configuration.selection.printer = name;
    configuration.selection.port = pdf ? L"PORTPROMPT:" : L"LPT1:";
    const PrinterSettings defaults{.form = L"Letter",
                                   .orientation = Orientation::Portrait,
                                   .duplex = pdf ? std::nullopt : std::optional{Duplex::OneSided},
                                   .collate = pdf ? std::nullopt : std::optional{false}};
    configuration.settings = requested.value_or(defaults);
    configuration.forms = {L"Letter", L"A4"};
    configuration.maximumCopies = pdf ? 1u : 12u;
    configuration.selection.paper =
        pdf || configuration.settings.form == L"A4" || configuration.settings.orientation == Orientation::Landscape ? WidePaper() : SmallPaper();
    return S_OK;
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
        while (app.printOperation && (!preview || !app.printPreview->IsReady()) && GetTickCount64() - start < 15000u)
        {
            MSG message{};
            if (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            else
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        Require(preview ? app.printOperation && app.printPreview->IsReady() : !app.printOperation,
                "print worker publishes the requested preview or terminal result");
    }

    static void Submit(Application &app)
    {
        Require(app.printPreview && app.printPreview->CanPrint(), "a configured destination offers the print command");
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
        const auto name = Resource(label);
        const auto search = [&](this const auto &self, DxUi::Control *parent) -> Control *
        {
            if (auto *control = dynamic_cast<Control *>(parent); control && control->GetAccessibleName() == name)
                return control;
            for (std::size_t index = 0; index < parent->GetLogicalChildCount(); ++index)
                if (auto *const found = self(parent->GetLogicalChild(index)))
                    return found;
            return nullptr;
        };
        if (auto *const found = search(app.printPreview->Host().GetRoot()))
            return *found;
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

    static void Wheel(Application &app, short delta)
    {
        const auto window = app.printPreview->Window();
        const auto area = Find<DxUi::ScrollPanel>(app, IDS_PRINTPREVIEW).GetViewportRect();
        const auto dpi = static_cast<int>(GetDpiForWindow(window));
        POINT point{MulDiv(static_cast<int>((area.left + area.right) * .5f), dpi, 96),
                    MulDiv(static_cast<int>((area.top + area.bottom) * .5f), dpi, 96)};
        Require(ClientToScreen(window, &point) != FALSE, "position the synthetic wheel message on the sheet");
        (void)SendMessageW(window, WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(delta)), MAKELPARAM(point.x, point.y));
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
        std::optional<Printing::Selection> submitted;
        std::vector<unsigned> pages;
        LaunchOptions options;
        options.headless = true;
        options.forceWarp = true;
        options.dataDirectory = directory;
        options.printDestinationsForTest =
            Printing::Destinations{{std::wstring(OfficePrinter), std::wstring(PdfPrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest = ConfigureFixture;
        options.printSubmissionForTest = [&](const Printing::Selection &selection) { submitted = selection; };
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
        Require(app.printPreview && !app.printPreview->CanPrint(), "File Print opens a preview before configuring the destination");
        const auto firstPreview = app.printPreview->Window();
        Require(GetWindow(firstPreview, GW_OWNER) == app.Window() && !IsWindowVisible(firstPreview), "the hidden preview has the editor as owner");
        Wait(app, true);
        const auto &first = app.printPreview->Page();
        Require(first.number == 1u && first.count >= 4u && first.width == 660u && first.height == 540u && first.bgra.size() == 660u * 540u * 4u &&
                    first.bgra.front() == 255u && HasInk(first),
                "the first preview is the paper-sized rendered page with ink and a white margin");
        const auto firstPixels = first.bgra;
        const auto count = first.count;
        auto &printer = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        auto &viewport = Find<DxUi::ScrollPanel>(app, IDS_PRINTPREVIEW);
        const auto printBounds = app.printPreview->Host().GetDefaultButton()->GetBounds();
        Require(printer.IsEnabled() && printer.GetItems().size() == 2u && printer.GetSelectedValue() == OfficePrinter &&
                    printer.GetBounds().right < viewport.GetBounds().left && printBounds.right < viewport.GetBounds().left,
                "the installed destination picker and Print action sit alongside the page");
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
        Wheel(app, -WHEEL_DELTA / 2);
        Require(app.printPreview->CanPrint() && app.printPreview->Page().number == 1u, "fractional wheel deltas do not skip a page");
        Wheel(app, -WHEEL_DELTA / 2);
        Wait(app, true);
        Require(app.printPreview->Page().number == 2u, "wheel down advances to the next fitted page");
        Wheel(app, WHEEL_DELTA);
        Wait(app, true);
        Require(app.printPreview->Page().number == 1u, "wheel up goes back to the previous fitted page");
        RECT previewFrame{};
        Require(GetWindowRect(firstPreview, &previewFrame) != FALSE, "read the preview frame");
        const auto dpi = static_cast<int>(GetDpiForWindow(firstPreview));
        constexpr UINT resizeFlags = SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
        Require(SetWindowPos(firstPreview, nullptr, 0, 0, MulDiv(1040, dpi, 96), MulDiv(480, dpi, 96), resizeFlags) != FALSE,
                "resize the hidden preview to test Fit width scrolling");
        Require(Find<DxUi::Button>(app, IDS_PRINTFITWIDTH).Invoke(app.printPreview->Host(), false), "enter Fit width");
        Wheel(app, -WHEEL_DELTA);
        Require(app.printPreview->Page().number == 1u && viewport.GetScrollOffset() > 0.f, "Fit width pans a tall sheet before changing pages");
        viewport.SetScrollOffset(viewport.GetContentHeight());
        Wheel(app, -WHEEL_DELTA);
        Wait(app, true);
        Require(app.printPreview->Page().number == 2u && viewport.GetScrollOffset() == 0.f,
                "wheel down at the bottom enters the next page at the top");
        Wheel(app, WHEEL_DELTA);
        Wait(app, true);
        Require(app.printPreview->Page().number == 1u && viewport.GetScrollOffset() > 0.f,
                "wheel up at the top enters the previous page at the bottom");
        Require(Find<DxUi::Button>(app, IDS_PRINTFITPAGE).Invoke(app.printPreview->Host(), false), "return to Fit page");
        Require(SetWindowPos(firstPreview, nullptr, 0, 0, previewFrame.right - previewFrame.left, previewFrame.bottom - previewFrame.top,
                             resizeFlags) != FALSE,
                "restore the typical preview size for the synthetic capture");
        Navigate(app, 2u);
        Require(HasInk(app.printPreview->Page()) && app.printPreview->Page().bgra != firstPixels, "navigation presents distinct rendered sheets");
        app.Execute(Command::ThemeDark);
        std::filesystem::create_directories(directory);
        Require(app.Capture(directory / L"print-preview-page-2.png"), "the real preview frame captures through the hidden application host");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app.printPreview->Host(), false), "select the displayed page for printing");
        Require(printer.OnKeyDown(app.printPreview->Host(), VK_DOWN, 0), "select the second installed destination from the combo");
        Wait(app, true);
        Require(printer.GetSelectedValue() == PdfPrinter && app.printPreview->Page().paper == WidePaper() && app.printPreview->Page().count < count &&
                    Find<DxUi::TextField>(app, IDS_PRINTPAGES).GetText() == L"2" && !Find<DxUi::TextField>(app, IDS_PRINTCOPIES).IsVisible() &&
                    !Find<DxUi::Checkbox>(app, IDS_PRINTTOFILE).IsVisible(),
                "the PDF choice reflows the preview and preserves the typed page selection");
        Require(app.printPreview->Host().GetCancelButton()->Invoke(app.printPreview->Host(), false), "cancel through the preview's Close button");
        MSG message{};
        while (IsWindow(firstPreview) && PeekMessageW(&message, firstPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(!app.printOperation && !app.printPreview && app.Host().GetFocusControl() == &app.Document() && pages.empty() && !submitted,
                "closing the preview cancels work and returns the editor focus without submission");

        for (const auto view : {Command::Compose, Command::Source, Command::Split})
        {
            app.Execute(view);
            pages.clear();
            app.OnMenuClosed(menuCommand);
            Wait(app, true);
            Require(app.printPreview->Page().bgra == firstPixels, "every editor view previews the same formatted page");
            const auto active = app.printPreview->Window();
            app.Execute(Command::Print);
            Require(app.printPreview->Window() == active, "a duplicate command cannot supersede the admitted preview");
            Submit(app);
            Wait(app);
            Require(submitted && submitted->printer == OfficePrinter && app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED) &&
                        pages.size() >= 4u && pages.front() == 1u && pages.back() == pages.size(),
                    "the print command records every page in order");
            Require(count == pages.size(), "every editor view uses the same pagination");
            Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection && app.Document().CanUndo() &&
                        app.Dirty() && app.session.path.empty(),
                    "printing preserves unsaved source, selection, history and identity");
        }
        pages.clear();
        Open(app);
        Find<DxUi::TextField>(app, IDS_PRINTPAGES).SetTextAndNotify(std::format(L" {}, 1–2, 2 ", count));
        Submit(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u, 2u, count}), "disjoint and overlapping requests print once in document order");
        pages.clear();
        submitted.reset();
        Open(app);
        auto &rangeField = Find<DxUi::TextField>(app, IDS_PRINTPAGES);
        for (const auto &invalid : {L"0", L"2-1", L"1,", L"999999999999999999999999", L"1-2-3"})
        {
            rangeField.SetTextAndNotify(invalid);
            Submit(app);
            Require(!submitted && pages.empty() && app.printPreview->CanPrint() && rangeField.GetText() == invalid &&
                        app.printPreview->Host().GetFocusControl() == &rangeField,
                    "invalid page selection focuses the field without sending pages");
        }

        rangeField.SetTextAndNotify(std::to_wstring(count));
        Require(Find<DxUi::ComboBox>(app, IDS_PRINTORIENTATION).OnKeyDown(app.printPreview->Host(), VK_DOWN, 0), "choose Landscape in the sidebar");
        Require(app.printPreview && !app.printPreview->CanPrint() && pages.empty(), "changing paper defers submission before showing the new pages");
        Wait(app, true);
        const auto &landscape = app.printPreview->Page();
        Require(landscape.paper == WidePaper() && landscape.width > landscape.height && landscape.count < count && HasInk(landscape),
                "landscape uses its own page geometry and recomputes the page count");
        const auto landscapeCount = landscape.count;
        Submit(app);
        Require(rangeField.GetText() == std::to_wstring(count) && !submitted && pages.empty(),
                "paper changes preserve the written range and require correction before printing");
        rangeField.SetTextAndNotify(std::to_wstring(landscapeCount));
        auto &copies = Find<DxUi::TextField>(app, IDS_PRINTCOPIES);
        copies.SetTextAndNotify(L"13");
        Submit(app);
        Require(!submitted && app.printPreview->Host().GetFocusControl() == &copies, "an invalid copy count focuses Copies without submitting");
        copies.SetTextAndNotify(L"3");
        Find<DxUi::Checkbox>(app, IDS_PRINTCOLLATE).SetChecked(true);
        Find<DxUi::Checkbox>(app, IDS_PRINTTOFILE).SetChecked(true);
        Require(landscapeCount >= 2u, "the landscape fixture still has more than one page");
        Require(app.SetDocument(L"Second document."), "switch the editor while the preview retains the original revision");
        Navigate(app, landscapeCount);
        Require(!Find<DxUi::Button>(app, IDS_PRINTNEXT).IsEnabled(), "Next is disabled on the last sheet");
        Require(HasInk(app.printPreview->Page()), "the preview still renders the last page of the admitted document after an editor change");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app.printPreview->Host(), false), "select the displayed last page for printing");
        Navigate(app, 1u);
        Submit(app);
        Wait(app);
        Require(submitted && submitted->paper == WidePaper() && submitted->copies == 3u && submitted->collate == true &&
                    submitted->port == L"FILE:" && std::ranges::equal(pages, std::array{landscapeCount}) &&
                    app.Document().GetText() == L"Second document.",
                "Current page selects exactly the displayed sheet at the time of selection, from the admitted revision");

        pages.clear();
        Require(app.SetDocument(L""), "seed an empty document");
        Open(app);
        Require(app.printPreview->Page().count == 1u && !HasInk(app.printPreview->Page()), "an empty document previews one blank sheet");
        Submit(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u}), "an empty document records one blank sheet");
        Open(app);
        Require(Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).OnKeyDown(app.printPreview->Host(), VK_DOWN, 0), "select the PDF queue");
        Wait(app, true);
        pages.clear();
        Submit(app);
        Wait(app);
        Require(submitted && submitted->printer == PdfPrinter && submitted->port == L"PORTPROMPT:" && std::ranges::equal(pages, std::array{1u}),
                "one Print action submits the selected PDF queue and its prepared page");
        Require(foreground.NeverAcquiredForeground() && !IsWindowVisible(app.Window()), "directed printing keeps the editor hidden");
    }

    static void FailureAndShutdown(const std::filesystem::path &directory)
    {
        auto releasePage = wil::unique_event(wil::EventOptions::ManualReset);
        auto enteredPage = wil::unique_event(wil::EventOptions::ManualReset);
        auto returnedPage = wil::unique_event(wil::EventOptions::ManualReset);
        std::atomic_uint calls{};
        std::atomic_bool pause{};
        bool failPage = true, failFinish = false, invalidPaper = true;
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = directory;
        options.unsavedPromptForTest = [] { return IDNO; };
        constexpr std::wstring_view missing = L"Redfern offline printer";
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(missing), std::wstring(OfficePrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest =
            [&](const std::wstring &name, const std::optional<Printing::PrinterSettings> &settings, Printing::PrinterConfiguration &selection)
        {
            if (name == missing)
                return HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND);
            const auto result = ConfigureFixture(name, settings, selection);
            if (invalidPaper)
                selection.selection.paper = {};
            return result;
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
        invalidPaper = false;
        app.Execute(Command::Print);
        app.DismissPrintPreview();
        Require(!app.printOperation && !app.printPreview && !calls, "cancelling during initial pagination detaches without printing");
        Open(app);
        auto &printers = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers.OnKeyDown(app.printPreview->Host(), VK_UP, 0), "select an unavailable printer");
        Wait(app, true);
        Require(!calls && !app.printPreview->CanPrint() && app.printPreview->Page().fallbackPaper && printers.IsEnabled(),
                "an unavailable destination leaves a navigable fallback preview with a disabled Print button");
        Require(printers.OnKeyDown(app.printPreview->Host(), VK_DOWN, 0), "return to an available printer");
        Wait(app, true);
        Submit(app);
        Wait(app);
        Require(calls == 2u && app.feedback->GetText() == L"Couldn’t print page 2. Destination Redfern office printer.",
                "a failed page names the page and printer and stops submission without a retry");
        failPage = false;
        failFinish = true;
        Open(app);
        Submit(app);
        Wait(app);
        Require(calls > 2u && app.feedback->GetText() == L"Couldn’t finish the print job. Destination Redfern office printer.",
                "finalization failure names the printer and the separate completion step");
        failFinish = false;
        Open(app);
        Submit(app);
        Wait(app);
        Require(app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED), "a new job succeeds after recoverable failures");

        const auto previous = calls.load();
        pause.store(true);
        Open(app);
        Submit(app);
        Require(enteredPage.wait(10000u), "worker reaches a controlled blocked printer page");
        app.Execute(Command::Close);
        Require(!app.printOperation && app.IsClosing(), "closing detaches the owned print job while the worker is blocked");
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
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(PdfPrinter)}, std::wstring(PdfPrinter)};
        options.printPrinterForTest = ConfigureFixture;
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

        Require(app.SetDocument(L"[Redfern reference](https://example.org/)"), "seed a named hyperlink");
        Open(app);
        Require(HasInk(app.printPreview->Page()), "the hyperlink label is visible on paper");
        const auto linkPixels = app.printPreview->Page().bgra;
        app.DismissPrintPreview();
        Require(app.SetDocument(L"Redfern reference"), "seed the same label as plain text");
        Open(app);
        Require(app.printPreview->Page().bgra == linkPixels, "a Markdown hyperlink prints like the same plain text without an underline");
        app.DismissPrintPreview();
    }

    static void CloseOverridesPendingPrintAction(const std::filesystem::path &directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        std::atomic_uint calls{};
        std::optional<Printing::Selection> submitted;
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = directory;
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(OfficePrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest = ConfigureFixture;
        options.printSubmissionForTest = [&](const Printing::Selection &selection) { submitted = selection; };
        options.printPageForTest = [&](unsigned, ID2D1CommandList &)
        {
            ++calls;
            return S_OK;
        };
        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize the hidden print-action override fixture");
        Require(app.SetDocument(MultipageSource()), "seed a synthetic document");
        app.Host().SetFocusControl(&app.Document(), false);

        Open(app);
        const HWND firstPreview = app.printPreview->Window();
        auto &firstHost = app.printPreview->Host();
        Require(app.printPreview->CanPrint(), "ready preview accepts Print");
        Require(firstHost.GetDefaultButton()->Invoke(firstHost, false), "queue Action::Print without pumping PreviewAction");
        Require(SendMessageW(firstPreview, WM_CLOSE, 0, 0) == 0, "WM_CLOSE queues Action::Close over pending Action::Print");
        MSG message{};
        while (IsWindow(firstPreview) && PeekMessageW(&message, firstPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(!app.printPreview && !app.printOperation && calls == 0u && !submitted && app.Host().GetFocusControl() == &app.Document(),
                "WM_CLOSE overrides pending Print action and dismisses preview without submitting");

        Open(app);
        const HWND secondPreview = app.printPreview->Window();
        auto &secondHost = app.printPreview->Host();
        Require(SendMessageW(secondPreview, WM_CLOSE, 0, 0) == 0, "WM_CLOSE queues Action::Close");
        Require(secondHost.GetDefaultButton()->Invoke(secondHost, false), "subsequent Print cannot override pending Close");
        while (IsWindow(secondPreview) && PeekMessageW(&message, secondPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(!app.printPreview && !app.printOperation && calls == 0u && !submitted && app.Host().GetFocusControl() == &app.Document(),
                "pending Close remains sticky against subsequent Print before PreviewAction pumps");
        Require(foreground.NeverAcquiredForeground() && !IsWindowVisible(app.Window()), "hidden preview override preserves desktop");
    }
};

void RegisterPrintingTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.PrintingPreservesRevisionAndHonorsPageRanges", [&] { PrintingProbe::StateAndRanges(root / L"print-pages"); });
    test("Editor.PrintingFailureAndShutdownRetireJobs", [&] { PrintingProbe::FailureAndShutdown(root / L"print-shutdown"); });
    test("Editor.PrintingDiagramFollowsCanvasAndPaper", [&] { PrintingProbe::DiagramBackground(root / L"print-diagram"); });
    test("Editor.PrintingCloseOverridesPendingPrintAction", [&] { PrintingProbe::CloseOverridesPendingPrintAction(root / L"print-close-override"); });
}
} // namespace RedTilde
