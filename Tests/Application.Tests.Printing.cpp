#include "Application.Tests.Printing.h"
#include "RedTilde/Application.h"
#include "RedTilde/KeyboardState.h"
#include "RedTilde/PrintPreview.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <DxUi/RichImage.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <format>
#include <memory>
#include <stdexcept>
#include <thread>
#include <wil/com.h>
#include <wincodec.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view detail)
{
    if (! condition)
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

HRESULT ConfigureFixture(const std::wstring& name, const std::optional<Printing::PrinterSettings>& requested, Printing::PrinterConfiguration& configuration)
{
    using namespace Printing;
    const bool pdf                  = name == PdfPrinter;
    configuration.selection.printer = name;
    configuration.selection.port    = pdf ? L"PORTPROMPT:" : L"LPT1:";
    const PrinterSettings defaults{.form        = L"Letter",
                                   .orientation = Orientation::Portrait,
                                   .duplex      = pdf ? std::nullopt : std::optional{Duplex::OneSided},
                                   .collate     = pdf ? std::nullopt : std::optional{false}};
    configuration.settings      = requested.value_or(defaults);
    configuration.forms         = {L"Letter", L"A4"};
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
    static void Wait(Application& app, bool preview = false)
    {
        const auto start = GetTickCount64();
        while (app._printOperation && (! preview || ! app._printPreview || ! app._printPreview->IsReady()) && GetTickCount64() - start < 15000u)
        {
            MSG message{};
            if (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            else
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        Require(preview ? (app._printOperation && app._printPreview && app._printPreview->IsReady()) : ! app._printOperation,
                "print worker publishes the requested preview or terminal result");
    }

    static void Submit(Application& app)
    {
        Require(app._printPreview && app._printPreview->CanPrint(), "a configured destination offers the print command");
        const auto window = app._printPreview->Window();
        auto& host        = app._printPreview->Host();
        Require(host.GetDefaultButton()->Invoke(host, false), "invoke the preview Print button without physical input");
        MSG message{};
        while (IsWindow(window) && PeekMessageW(&message, window, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
    }

    static void Open(Application& app)
    {
        app.Execute(Command::Print);
        Require(app._printPreview && app._printOperation && ! app._printPreview->CanPrint(), "print opens the preview and waits for page content");
        Wait(app, true);
    }

    template <typename Control> static Control& Find(Application& app, unsigned label)
    {
        const auto name   = Resource(label);
        const auto search = [&](this const auto& self, DxUi::Control* parent) -> Control*
        {
            if (auto* control = dynamic_cast<Control*>(parent); control && control->GetAccessibleName() == name)
                return control;
            for (std::size_t index = 0; index < parent->GetLogicalChildCount(); ++index)
                if (auto* const found = self(parent->GetLogicalChild(index)))
                    return found;
            return nullptr;
        };
        if (auto* const found = search(app._printPreview->Host().GetRoot()))
            return *found;
        throw std::runtime_error("the preview exposes the named control");
    }

    static void Navigate(Application& app, unsigned page)
    {
        auto& field = Find<DxUi::TextField>(app, IDS_PRINTPREVIEWPAGE);
        field.SetTextAndNotify(std::to_wstring(page));
        Require(field.OnKeyDown(app._printPreview->Host(), VK_RETURN, 0), "Enter in the page field jumps to the numbered page");
        Wait(app, true);
        Require(app._printPreview->Page().number == page, "the preview displays the requested page");
    }

    static void Wheel(Application& app, short delta)
    {
        KeyboardState::ScopedSynthetic synthetic;
        constexpr std::array<BYTE, 256> cleanKeys{};
        KeyboardState::Write(cleanKeys);
        const auto window = app._printPreview->Window();
        const auto area   = Find<DxUi::ScrollPanel>(app, IDS_PRINTPREVIEW).GetViewportRect();
        const auto dpi    = static_cast<int>(GetDpiForWindow(window));
        POINT point{MulDiv(static_cast<int>((area.left + area.right) * .5f), dpi, 96), MulDiv(static_cast<int>((area.top + area.bottom) * .5f), dpi, 96)};
        Require(ClientToScreen(window, &point) != FALSE, "position the synthetic wheel message on the sheet");
        (void)SendMessageW(window, WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(delta)), MAKELPARAM(point.x, point.y));
    }

    static bool HasInk(const Printing::PreviewPage& page)
    {
        for (std::size_t index = 0; index + 4u <= page.bgra.size(); index += 4u)
            if (page.bgra[index] < 96u && page.bgra[index + 1u] < 96u && page.bgra[index + 2u] < 96u)
                return true;
        return false;
    }

    static void StateAndRanges(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        std::optional<Printing::Selection> submitted;
        std::vector<unsigned> pages;
        LaunchOptions options;
        options.headless                 = true;
        options.forceWarp                = true;
        options.dataDirectory            = directory;
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(OfficePrinter), std::wstring(PdfPrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest      = ConfigureFixture;
        options.printSubmissionForTest   = [&](const Printing::Selection& selection) { submitted = selection; };
        options.printPageForTest         = [&](unsigned page, ID2D1CommandList&)
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
        const auto snapshot    = app.Document().Snapshot();
        const auto selection   = app.Document().GetSelection();
        const auto menuCommand = static_cast<int>(Command::Print) + 1;
        app.Host().SetFocusControl(&app.Document(), false);
        app.OnMenuClosed(menuCommand);
        Require(app._printPreview && ! app._printPreview->CanPrint(), "File Print opens a preview before configuring the destination");
        const auto firstPreview = app._printPreview->Window();
        Require(GetWindow(firstPreview, GW_OWNER) == app.Window() && ! IsWindowVisible(firstPreview), "the hidden preview has the editor as owner");
        Wait(app, true);
        const auto& first = app._printPreview->Page();
        Require(first.number == 1u && first.count >= 4u && first.width == 660u && first.height == 540u && first.bgra.size() == 660u * 540u * 4u &&
                    first.bgra.front() == 255u && HasInk(first),
                "the first preview is the paper-sized rendered page with ink and a white margin");
        const auto firstPixels = first.bgra;
        const auto count       = first.count;
        auto& printer          = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        auto& viewport         = Find<DxUi::ScrollPanel>(app, IDS_PRINTPREVIEW);
        const auto printBounds = app._printPreview->Host().GetDefaultButton()->GetBounds();
        Require(printer.IsEnabled() && printer.GetItems().size() == 2u && printer.GetSelectedValue() == OfficePrinter &&
                    printer.GetBounds().right < viewport.GetBounds().left && printBounds.right < viewport.GetBounds().left,
                "the installed destination picker and Print action sit alongside the page");
        auto& previous = Find<DxUi::Button>(app, IDS_PRINTPREVIOUS);
        auto& next     = Find<DxUi::Button>(app, IDS_PRINTNEXT);
        Require(previous.GetText() == L"←" && next.GetText() == L"→" && previous.GetBounds().right < next.GetBounds().left && ! previous.IsEnabled() &&
                    next.IsEnabled(),
                "the left arrow is Previous and is disabled on the first page; the right arrow is Next");
        Require(next.Invoke(app._printPreview->Host(), false), "choose the right arrow");
        Wait(app, true);
        Require(app._printPreview->Page().number == 2u, "the right arrow advances one page");
        Require(previous.Invoke(app._printPreview->Host(), false), "choose the left arrow");
        Wait(app, true);
        Require(app._printPreview->Page().number == 1u, "the left arrow goes back one page");
        Wheel(app, -WHEEL_DELTA / 2);
        Require(app._printPreview->CanPrint() && app._printPreview->Page().number == 1u, "fractional wheel deltas do not skip a page");
        Wheel(app, -WHEEL_DELTA / 2);
        Wait(app, true);
        Require(app._printPreview->Page().number == 2u, "wheel down advances to the next fitted page");
        Wheel(app, WHEEL_DELTA);
        Wait(app, true);
        Require(app._printPreview->Page().number == 1u, "wheel up goes back to the previous fitted page");
        RECT previewFrame{};
        Require(GetWindowRect(firstPreview, &previewFrame) != FALSE, "read the preview frame");
        const auto dpi             = static_cast<int>(GetDpiForWindow(firstPreview));
        constexpr UINT resizeFlags = SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
        Require(SetWindowPos(firstPreview, nullptr, 0, 0, MulDiv(1040, dpi, 96), MulDiv(480, dpi, 96), resizeFlags) != FALSE,
                "resize the hidden preview to test Fit width scrolling");
        Require(Find<DxUi::Button>(app, IDS_PRINTFITWIDTH).Invoke(app._printPreview->Host(), false), "enter Fit width");
        Wheel(app, -WHEEL_DELTA);
        Require(app._printPreview->Page().number == 1u && viewport.GetScrollOffset() > 0.f, "Fit width pans a tall sheet before changing pages");
        viewport.SetScrollOffset(viewport.GetContentHeight());
        Wheel(app, -WHEEL_DELTA);
        Wait(app, true);
        Require(app._printPreview->Page().number == 2u && viewport.GetScrollOffset() == 0.f, "wheel down at the bottom enters the next page at the top");
        Wheel(app, WHEEL_DELTA);
        Wait(app, true);
        Require(app._printPreview->Page().number == 1u && viewport.GetScrollOffset() > 0.f, "wheel up at the top enters the previous page at the bottom");
        Require(Find<DxUi::Button>(app, IDS_PRINTFITPAGE).Invoke(app._printPreview->Host(), false), "return to Fit page");
        Require(SetWindowPos(firstPreview, nullptr, 0, 0, previewFrame.right - previewFrame.left, previewFrame.bottom - previewFrame.top, resizeFlags) != FALSE,
                "restore the typical preview size for the synthetic capture");
        Navigate(app, 2u);
        Require(HasInk(app._printPreview->Page()) && app._printPreview->Page().bgra != firstPixels, "navigation presents distinct rendered sheets");
        app.Execute(Command::ThemeDark);
        std::filesystem::create_directories(directory);
        Require(app.Capture(directory / L"print-preview-page-2.png"), "the real preview frame captures through the hidden application host");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app._printPreview->Host(), false), "select the displayed page for printing");
        Require(printer.OnKeyDown(app._printPreview->Host(), VK_DOWN, 0), "select the second installed destination from the combo");
        Wait(app, true);
        Require(printer.GetSelectedValue() == PdfPrinter && app._printPreview->Page().paper == WidePaper() && app._printPreview->Page().count < count &&
                    Find<DxUi::TextField>(app, IDS_PRINTPAGES).GetText() == L"2" && ! Find<DxUi::TextField>(app, IDS_PRINTCOPIES).IsVisible() &&
                    ! Find<DxUi::Checkbox>(app, IDS_PRINTTOFILE).IsVisible(),
                "the PDF choice reflows the preview and preserves the typed page selection");
        Require(app._printPreview->Host().GetCancelButton()->Invoke(app._printPreview->Host(), false), "cancel through the preview's Close button");
        MSG message{};
        while (IsWindow(firstPreview) && PeekMessageW(&message, firstPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(! app._printOperation && ! app._printPreview && app.Host().GetFocusControl() == &app.Document() && pages.empty() && ! submitted,
                "closing the preview cancels work and returns the editor focus without submission");

        for (const auto view : {Command::Compose, Command::Source, Command::Split})
        {
            app.Execute(view);
            pages.clear();
            app.OnMenuClosed(menuCommand);
            Wait(app, true);
            Require(app._printPreview->Page().bgra == firstPixels, "every editor view previews the same formatted page");
            const auto active = app._printPreview->Window();
            app.Execute(Command::Print);
            Require(app._printPreview->Window() == active, "a duplicate command cannot supersede the admitted preview");
            Submit(app);
            Wait(app);
            Require(submitted && submitted->printer == OfficePrinter && app._feedback->GetText() == Resource(IDS_PRINTSUBMITTED) && pages.size() >= 4u &&
                        pages.front() == 1u && pages.back() == pages.size(),
                    "the print command records every page in order");
            Require(count == pages.size(), "every editor view uses the same pagination");
            Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection && app.Document().CanUndo() &&
                        app.Dirty() && app._session.path.empty(),
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
        auto& rangeField = Find<DxUi::TextField>(app, IDS_PRINTPAGES);
        for (const auto& invalid : {L"0", L"2-1", L"1,", L"999999999999999999999999", L"1-2-3"})
        {
            rangeField.SetTextAndNotify(invalid);
            Submit(app);
            Require(! submitted && pages.empty() && app._printPreview->CanPrint() && rangeField.GetText() == invalid &&
                        app._printPreview->Host().GetFocusControl() == &rangeField,
                    "invalid page selection focuses the field without sending pages");
        }

        rangeField.SetTextAndNotify(std::to_wstring(count));
        Require(Find<DxUi::ComboBox>(app, IDS_PRINTORIENTATION).OnKeyDown(app._printPreview->Host(), VK_DOWN, 0), "choose Landscape in the sidebar");
        Require(app._printPreview && ! app._printPreview->CanPrint() && pages.empty(), "changing paper defers submission before showing the new pages");
        Wait(app, true);
        const auto& landscape = app._printPreview->Page();
        Require(landscape.paper == WidePaper() && landscape.width > landscape.height && landscape.count < count && HasInk(landscape),
                "landscape uses its own page geometry and recomputes the page count");
        const auto landscapeCount = landscape.count;
        Submit(app);
        Require(rangeField.GetText() == std::to_wstring(count) && ! submitted && pages.empty(),
                "paper changes preserve the written range and require correction before printing");
        rangeField.SetTextAndNotify(std::to_wstring(landscapeCount));
        auto& copies = Find<DxUi::TextField>(app, IDS_PRINTCOPIES);
        copies.SetTextAndNotify(L"13");
        Submit(app);
        Require(! submitted && app._printPreview->Host().GetFocusControl() == &copies, "an invalid copy count focuses Copies without submitting");
        copies.SetTextAndNotify(L"3");
        Find<DxUi::Checkbox>(app, IDS_PRINTCOLLATE).SetChecked(true);
        Find<DxUi::Checkbox>(app, IDS_PRINTTOFILE).SetChecked(true);
        Require(landscapeCount >= 2u, "the landscape fixture still has more than one page");
        Require(app.SetDocument(L"Second document."), "switch the editor while the preview retains the original revision");
        Navigate(app, landscapeCount);
        Require(! Find<DxUi::Button>(app, IDS_PRINTNEXT).IsEnabled(), "Next is disabled on the last sheet");
        Require(HasInk(app._printPreview->Page()), "the preview still renders the last page of the admitted document after an editor change");
        Require(Find<DxUi::Button>(app, IDS_PRINTCURRENTPAGE).Invoke(app._printPreview->Host(), false), "select the displayed last page for printing");
        Navigate(app, 1u);
        Submit(app);
        Wait(app);
        Require(submitted && submitted->paper == WidePaper() && submitted->copies == 3u && submitted->collate == true && submitted->port == L"FILE:" &&
                    std::ranges::equal(pages, std::array{landscapeCount}) && app.Document().GetText() == L"Second document.",
                "Current page selects exactly the displayed sheet at the time of selection, from the admitted revision");

        pages.clear();
        Require(app.SetDocument(L""), "seed an empty document");
        Open(app);
        Require(app._printPreview->Page().count == 1u && ! HasInk(app._printPreview->Page()), "an empty document previews one blank sheet");
        Submit(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u}), "an empty document records one blank sheet");
        Open(app);
        Require(Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).OnKeyDown(app._printPreview->Host(), VK_DOWN, 0), "select the PDF queue");
        Wait(app, true);
        pages.clear();
        Submit(app);
        Wait(app);
        Require(submitted && submitted->printer == PdfPrinter && submitted->port == L"PORTPROMPT:" && std::ranges::equal(pages, std::array{1u}),
                "one Print action submits the selected PDF queue and its prepared page");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "directed printing keeps the editor hidden");
    }

    static void FailureAndShutdown(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        struct FailureState
        {
            FailureState()                               = default;
            FailureState(const FailureState&)            = delete;
            FailureState& operator=(const FailureState&) = delete;
            FailureState(FailureState&&)                 = delete;
            FailureState& operator=(FailureState&&)      = delete;
            wil::unique_event releasePage{wil::EventOptions::ManualReset};
            wil::unique_event enteredPage{wil::EventOptions::ManualReset};
            wil::unique_event returnedPage{wil::EventOptions::ManualReset};
            std::atomic_uint calls{};
            std::atomic_bool pause{};
            std::atomic_bool failPage{true};
            std::atomic_bool failFinish{};
            std::atomic_bool invalidPaper{true};
        };
        auto state = std::make_shared<FailureState>();
        LaunchOptions options;
        options.headless                    = true;
        options.dataDirectory               = directory;
        options.unsavedPromptForTest        = [] { return IDNO; };
        constexpr std::wstring_view missing = L"Redfern offline printer";
        options.printDestinationsForTest    = Printing::Destinations{{std::wstring(missing), std::wstring(OfficePrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest         = [state, missingName = std::wstring(missing)](const std::wstring& name,
                                                                                           const std::optional<Printing::PrinterSettings>& settings,
                                                                                           Printing::PrinterConfiguration& selection)
        {
            if (name == missingName)
                return HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND);
            const auto result = ConfigureFixture(name, settings, selection);
            if (state->invalidPaper.load(std::memory_order_acquire))
                selection.selection.paper = {};
            return result;
        };
        options.printPageForTest = [state](unsigned page, ID2D1CommandList&)
        {
            state->calls.fetch_add(1u, std::memory_order_acq_rel);
            const HRESULT result = state->failPage.load(std::memory_order_acquire) && page == 2u ? E_ACCESSDENIED : S_OK;
            if (state->pause.load(std::memory_order_acquire))
            {
                const auto signalReturn = wil::scope_exit([state]() noexcept { state->returnedPage.SetEvent(); });
                state->enteredPage.SetEvent();
                const bool signalled = state->releasePage.wait(10000u);
                if (! signalled)
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            return result;
        };
        options.printFinishForTest = [state] { return state->failFinish.load(std::memory_order_acquire) ? E_FAIL : S_OK; };
        Application app(std::move(options));
        const auto release = wil::scope_exit([state]
        {
            state->releasePage.SetEvent();
            if (state->enteredPage.wait(0u))
                (void)state->returnedPage.wait(10000u);
        });
        Require(SUCCEEDED(app.Initialize()), "initialize the print error fixture");
        Require(app.SetDocument(MultipageSource()), "seed enough pages for a mid-job failure");
        app.Execute(Command::Print);
        Wait(app);
        Require(! app._printPreview && ! state->calls.load(std::memory_order_acquire) && app._feedback->GetText() == Resource(IDS_PRINTPREVIEWFAILED),
                "preview failure clears the session before reporting the error");
        state->invalidPaper.store(false, std::memory_order_release);
        app.Execute(Command::Print);
        app.DismissPrintPreview();
        Require(! app._printOperation && ! app._printPreview && ! state->calls.load(std::memory_order_acquire),
                "cancelling during initial pagination detaches without printing");
        Open(app);
        auto& printers = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers.OnKeyDown(app._printPreview->Host(), VK_UP, 0), "select an unavailable printer");
        Wait(app, true);
        Require(! state->calls.load(std::memory_order_acquire) && ! app._printPreview->CanPrint() && app._printPreview->Page().fallbackPaper &&
                    printers.IsEnabled(),
                "an unavailable destination leaves a navigable fallback preview with a disabled Print button");
        Require(printers.OnKeyDown(app._printPreview->Host(), VK_DOWN, 0), "return to an available printer");
        Wait(app, true);
        Submit(app);
        Wait(app);
        Require(state->calls.load(std::memory_order_acquire) == 2u && app._feedback->GetText() == L"Couldn’t print page 2. Destination Redfern office printer.",
                "a failed page names the page and printer and stops submission without a retry");
        state->failPage.store(false, std::memory_order_release);
        state->failFinish.store(true, std::memory_order_release);
        Open(app);
        Submit(app);
        Wait(app);
        Require(state->calls.load(std::memory_order_acquire) > 2u &&
                    app._feedback->GetText() == L"Couldn’t finish the print job. Destination Redfern office printer.",
                "finalization failure names the printer and the separate completion step");
        state->failFinish.store(false, std::memory_order_release);
        Open(app);
        Submit(app);
        Wait(app);
        Require(app._feedback->GetText() == Resource(IDS_PRINTSUBMITTED), "a new job succeeds after recoverable failures");

        const auto previous = state->calls.load(std::memory_order_acquire);
        state->pause.store(true, std::memory_order_release);
        Open(app);
        Submit(app);
        Require(state->enteredPage.wait(10000u), "worker reaches a controlled blocked printer page");
        app.Execute(Command::Close);
        Require(! app._printOperation && app.IsClosing(), "closing detaches the owned print job while the worker is blocked");
        state->releasePage.SetEvent();
        Require(state->returnedPage.wait(10000u), "the submitted page returns after the owner closes");
        Require(state->calls.load(std::memory_order_acquire) == previous + 1u, "shutdown cancels before the next page");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "print error fixture preserves desktop without activation");
    }

    static void DiagramBackground(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        LaunchOptions options;
        options.headless                 = true;
        options.forceWarp                = true;
        options.dataDirectory            = directory;
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(PdfPrinter)}, std::wstring(PdfPrinter)};
        options.printPrinterForTest      = ConfigureFixture;
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
        const auto diagram  = std::ranges::find(elements, DxUi::RichText::SemanticRole::Diagram, &DxUi::RichText::SemanticElement::role);
        Require(diagram != elements.end(), "the graph is a native diagram in the editor");
        const auto bounds = app.Document().GetSemanticElementBounds(app.Host(), static_cast<std::size_t>(diagram - elements.begin()));
        DxUi::WindowHostBitmapCapture capture;
        Require(bounds && app.Host().DebugCaptureBitmap(capture, true), "read the diagram pixels from the owned editor");
        const float scale = app.Host().GetDpi() / 96.f;
        const auto left   = static_cast<UINT>(bounds.value().left * scale);
        const auto middle = static_cast<UINT>((bounds.value().top + bounds.value().bottom) * .5f * scale);
        Require(left > 4u && left + 4u < capture.widthPx && middle < capture.heightPx, "graph margin samples fit the editor frame");
        const auto canvas = (static_cast<std::size_t>(middle) * capture.widthPx + left - 4u) * 4u;
        const auto margin = canvas + 32u;
        Require(std::equal(capture.bgraPixels.data() + canvas, capture.bgraPixels.data() + canvas + 3u, capture.bgraPixels.data() + margin),
                "the diagram margin matches the adjacent editor canvas");

        Open(app);
        const auto& page = app._printPreview->Page();
        Require(page.count == 1u && page.bgra.size() == static_cast<std::size_t>(page.width) * page.height * 4u,
                "the graph fits one printed sheet with a complete raster");
        unsigned leftInk = page.width, topInk = page.height, bottomInk{};
        for (unsigned y = 0u; y < page.height; ++y)
            for (unsigned x = 0u; x < page.width; ++x)
            {
                const auto offset = (static_cast<std::size_t>(y) * page.width + x) * 4u;
                if (page.bgra[offset] < 96u && page.bgra[offset + 1u] < 96u && page.bgra[offset + 2u] < 96u)
                {
                    leftInk   = std::min(leftInk, x);
                    topInk    = std::min(topInk, y);
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
        Require(HasInk(app._printPreview->Page()), "the hyperlink label is visible on paper");
        const auto linkPixels = app._printPreview->Page().bgra;
        app.DismissPrintPreview();
        Require(app.SetDocument(L"Redfern reference"), "seed the same label as plain text");
        Open(app);
        Require(app._printPreview->Page().bgra == linkPixels, "a Markdown hyperlink prints like the same plain text without an underline");
        app.DismissPrintPreview();
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "hidden diagram printing checks preserve desktop foreground");
    }

    static void CloseOverridesPendingPrintAction(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        std::atomic_uint calls{};
        std::optional<Printing::Selection> submitted;
        LaunchOptions options;
        options.headless                 = true;
        options.dataDirectory            = directory;
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(OfficePrinter)}, std::wstring(OfficePrinter)};
        options.printPrinterForTest      = ConfigureFixture;
        options.printSubmissionForTest   = [&](const Printing::Selection& selection) { submitted = selection; };
        options.printPageForTest         = [&](unsigned, ID2D1CommandList&)
        {
            ++calls;
            return S_OK;
        };
        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize the hidden print-action override fixture");
        Require(app.SetDocument(MultipageSource()), "seed a synthetic document");
        app.Host().SetFocusControl(&app.Document(), false);

        Open(app);
        const HWND firstPreview = app._printPreview->Window();
        auto& firstHost         = app._printPreview->Host();
        Require(app._printPreview->CanPrint(), "ready preview accepts Print");
        Require(firstHost.GetDefaultButton()->Invoke(firstHost, false), "queue Action::Print without pumping PreviewAction");
        Require(SendMessageW(firstPreview, WM_CLOSE, 0, 0) == 0, "WM_CLOSE queues Action::Close over pending Action::Print");
        MSG message{};
        while (IsWindow(firstPreview) && PeekMessageW(&message, firstPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(! app._printPreview && ! app._printOperation && calls == 0u && ! submitted && app.Host().GetFocusControl() == &app.Document(),
                "WM_CLOSE overrides pending Print action and dismisses preview without submitting");

        Open(app);
        const HWND secondPreview = app._printPreview->Window();
        auto& secondHost         = app._printPreview->Host();
        Require(SendMessageW(secondPreview, WM_CLOSE, 0, 0) == 0, "WM_CLOSE queues Action::Close");
        Require(secondHost.GetDefaultButton()->Invoke(secondHost, false), "subsequent Print cannot override pending Close");
        while (IsWindow(secondPreview) && PeekMessageW(&message, secondPreview, 0, 0, PM_REMOVE))
            DispatchMessageW(&message);
        Require(! app._printPreview && ! app._printOperation && calls == 0u && ! submitted && app.Host().GetFocusControl() == &app.Document(),
                "pending Close remains sticky against subsequent Print before PreviewAction pumps");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "hidden preview override preserves desktop");
    }

    static void SlowPrinterAllowsSwitchAndFallback(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        struct SlowState
        {
            SlowState()                            = default;
            SlowState(const SlowState&)            = delete;
            SlowState& operator=(const SlowState&) = delete;
            SlowState(SlowState&&)                 = delete;
            SlowState& operator=(SlowState&&)      = delete;
            wil::unique_event enteredSlow{wil::EventOptions::ManualReset};
            wil::unique_event releaseSlow{wil::EventOptions::ManualReset};
            wil::unique_event returnedSlow{wil::EventOptions::ManualReset};
            std::atomic_bool holdSlow{true};
        };
        auto slowState                          = std::make_shared<SlowState>();
        constexpr std::wstring_view slowPrinter = L"Redfern slow printer";

        LaunchOptions options;
        options.headless                     = true;
        options.forceWarp                    = true;
        options.dataDirectory                = directory;
        options.printPrinterTimeoutMsForTest = 250u;
        options.printDestinationsForTest     = Printing::Destinations{{std::wstring(PdfPrinter), std::wstring(slowPrinter)}, std::wstring(slowPrinter)};
        options.printPrinterForTest          = [slowState, slowName = std::wstring(slowPrinter)](const std::wstring& name,
                                                                                                 const std::optional<Printing::PrinterSettings>& settings,
                                                                                                 Printing::PrinterConfiguration& configuration) -> HRESULT
        {
            if (name == slowName && slowState->holdSlow.load(std::memory_order_acquire))
            {
                const auto signalReturn = wil::scope_exit([slowState]() noexcept { slowState->returnedSlow.SetEvent(); });
                slowState->enteredSlow.SetEvent();
                const bool signalled = slowState->releaseSlow.wait(15000u);
                if (! signalled)
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            return ConfigureFixture(name, settings, configuration);
        };

        Application app(std::move(options));
        const auto cleanupRelease = wil::scope_exit([slowState]
        {
            slowState->holdSlow.store(false, std::memory_order_release);
            slowState->releaseSlow.SetEvent();
            if (slowState->enteredSlow.wait(0u))
                (void)slowState->returnedSlow.wait(10000u);
        });
        Require(SUCCEEDED(app.Initialize()), "initialize the slow-printer preview host");
        const std::wstring mermaidAndProse =
            L"# Shared renderer display regressions\n\n```mermaid\nflowchart LR\nA[Write] -->|review| B[Print]\n```\n\n" + MultipageSource();
        Require(app.SetDocument(mermaidAndProse), "seed a multipage Mermaid document");
        app.Host().SetFocusControl(&app.Document(), false);

        const auto waitForDestinations = [&]()
        {
            Require(slowState->enteredSlow.wait(5000u), "worker starts querying the slow printer");
            const auto start = GetTickCount64();
            while (GetTickCount64() - start < 5000u)
            {
                MSG message{};
                while (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                    DispatchMessageW(&message);
                if (app._printPreview && ! Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).GetItems().empty())
                    return;
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
            }
            throw std::runtime_error("destinations did not arrive while slow printer was blocked");
        };

        const auto waitUntil = [&](auto&& predicate, std::string_view detail)
        {
            const auto start = GetTickCount64();
            while (! predicate() && GetTickCount64() - start < 10000u)
            {
                MSG message{};
                if (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                    DispatchMessageW(&message);
                else
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
            }
            if (! predicate())
                throw std::runtime_error(std::string(detail));
        };

        const auto hasStatusText = [&](std::wstring_view expected)
        {
            const auto search = [&](this const auto& self, DxUi::Control* parent) -> bool
            {
                if (const auto* label = dynamic_cast<const DxUi::Label*>(parent); label && label->GetText() == expected)
                    return true;
                for (std::size_t index = 0; index < parent->GetLogicalChildCount(); ++index)
                    if (self(parent->GetLogicalChild(index)))
                        return true;
                return false;
            };
            return app._printPreview && search(app._printPreview->Host().GetRoot());
        };
        const auto slowStatus = FormatStringResource(nullptr, IDS_PRINTPREVIEWSLOW, slowPrinter);

        // 1 & 2. Destinations enable Printer immediately; timeout shows browsable Letter fallback while slow printer stays blocked.
        app.Execute(Command::Print);
        waitForDestinations();
        auto& printers1 = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers1.GetItems().size() == 2u && printers1.GetSelectedValue() == slowPrinter && printers1.IsEnabled(),
                "printer dropdown is populated and enabled while the default slow printer is still checking");
        Require(! app._printPreview->IsReady() && ! app._printPreview->CanPrint(), "Print stays disabled before initial fallback renders");

        Wait(app, true);
        Require(app._printPreview->IsReady() && app._printPreview->Page().fallbackPaper && app._printPreview->Page().count >= 2u &&
                    HasInk(app._printPreview->Page()) && ! app._printPreview->CanPrint() && printers1.IsEnabled() && hasStatusText(slowStatus),
                "slow printer timeout shows a navigable Letter fallback preview with Printer enabled and Print disabled");
        Require(Find<DxUi::Button>(app, IDS_PRINTNEXT).Invoke(app._printPreview->Host(), false), "advance page while slow printer is pending");
        Wait(app, true);
        Require(app._printPreview->Page().number == 2u && ! app._printPreview->CanPrint() && printers1.IsEnabled(),
                "page navigation works while slow printer remains pending");
        Require(Find<DxUi::Button>(app, IDS_PRINTPREVIOUS).Invoke(app._printPreview->Host(), false), "return to page 1 while slow printer is pending");
        Wait(app, true);
        Require(app._printPreview->Page().number == 1u, "returning to page 1 completes cleanly during fallback");

        // 3. Unblock slow printer -> late driver configuration promotes the preview and enables Print.
        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "blocked slow printer completes after release");
        waitUntil([&] { return app._printPreview && app._printPreview->CanPrint() && ! app._printPreview->Page().fallbackPaper; },
                  "slow printer late completion enables Print and leaves fallback paper");
        Require(app._printPreview->Printer() == slowPrinter && app._printPreview->Page().paper == SmallPaper() && app._printPreview->Page().number == 1u &&
                    HasInk(app._printPreview->Page()),
                "late completion applies the slow printer paper geometry on the current page");
        app.DismissPrintPreview();
        Wait(app);

        // 4. Switch to fast PDF printer immediately while slow printer is still blocked; stale completion is discarded.
        slowState->enteredSlow.ResetEvent();
        slowState->releaseSlow.ResetEvent();
        slowState->returnedSlow.ResetEvent();
        app.Execute(Command::Print);
        waitForDestinations();
        auto& printers2 = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers2.OnKeyDown(app._printPreview->Host(), VK_UP, 0), "switch immediately to the fast PDF printer while slow printer is checking");
        Require(printers2.GetSelectedValue() == PdfPrinter, "combo moves immediately to the fast PDF printer");
        Wait(app, true);
        Require(app._printPreview->Printer() == PdfPrinter && app._printPreview->CanPrint() && ! app._printPreview->Page().fallbackPaper &&
                    app._printPreview->Page().paper == WidePaper(),
                "fast PDF printer configures immediately without waiting on the blocked slow printer");

        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "abandoned slow printer finishes asynchronously after switch");
        const auto drainStart = GetTickCount64();
        while (GetTickCount64() - drainStart < 80u)
        {
            MSG message{};
            while (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        Require(app._printPreview->Printer() == PdfPrinter && app._printPreview->CanPrint() && ! app._printPreview->Page().fallbackPaper &&
                    app._printPreview->Page().paper == WidePaper(),
                "stale slow printer completion never overwrites the newly selected fast printer");
        app.DismissPrintPreview();
        Wait(app);

        // 5. Closing preview detaches the UI and returns editor focus immediately while the slow printer is still blocked.
        slowState->enteredSlow.ResetEvent();
        slowState->releaseSlow.ResetEvent();
        slowState->returnedSlow.ResetEvent();
        app.Execute(Command::Print);
        Require(slowState->enteredSlow.wait(5000u), "worker enters slow printer on third open");
        app.DismissPrintPreview();
        Require(! app._printPreview && ! app._printOperation && app.Host().GetFocusControl() == &app.Document(),
                "closing preview detaches the UI and restores focus immediately without waiting on the slow printer");
        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "blocked slow printer test probe releases cleanly before teardown");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "slow-printer test preserves desktop");
    }

    static void WaitingOverlayShowsAfterDelayAndClears(const std::filesystem::path& directory)
    {
        static_assert(PrintPreview::WaitingOverlayDelayMs == 800u, "waiting indicator appears after 800 ms");
        Testing::ForegroundOwnershipObserver foreground;

        // 1. Fast render (< 800 ms default delay) never shows the waiting card or activates an animation subscription.
        {
            LaunchOptions options;
            options.headless                 = true;
            options.forceWarp                = true;
            options.dataDirectory            = directory / L"fast";
            options.printDestinationsForTest = Printing::Destinations{{std::wstring(PdfPrinter)}, std::wstring(PdfPrinter)};
            options.printPrinterForTest      = ConfigureFixture;
            Application app(std::move(options));
            Require(SUCCEEDED(app.Initialize()), "initialize fast waiting-overlay check");
            Require(app.SetDocument(L"# Fast preview\n\nReady before 800 ms."), "seed fast document");
            bool everSawOverlay{};
            app.Execute(Command::Print);
            const auto fastStart = GetTickCount64();
            while ((! app._printPreview || ! app._printPreview->IsReady()) && GetTickCount64() - fastStart < 15000u)
            {
                if (app._printPreview && app._printPreview->IsWaitingOverlayVisible())
                    everSawOverlay = true;
                MSG message{};
                if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                else
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, 0);
            }
            Require(app._printPreview && app._printPreview->IsReady() && ! everSawOverlay && ! app._printPreview->IsWaitingOverlayVisible() &&
                        ! app._printPreview->Host().DebugHasActiveAnimationSubscription(),
                    "fast preview settles before 800 ms without showing the waiting card");
            app.DismissPrintPreview();
            Wait(app);
        }

        // 2-6. Slow printer reveals centered themed waiting card after delay, animates when motion is on,
        // and clears cleanly on completion, printer switch, or early dismissal.
        struct SlowState
        {
            SlowState()                            = default;
            SlowState(const SlowState&)            = delete;
            SlowState& operator=(const SlowState&) = delete;
            SlowState(SlowState&&)                 = delete;
            SlowState& operator=(SlowState&&)      = delete;
            wil::unique_event enteredSlow{wil::EventOptions::ManualReset};
            wil::unique_event releaseSlow{wil::EventOptions::ManualReset};
            wil::unique_event returnedSlow{wil::EventOptions::ManualReset};
            std::atomic_bool holdSlow{true};
        };
        auto slowState                          = std::make_shared<SlowState>();
        constexpr std::wstring_view slowPrinter = L"Redfern stalled printer";

        LaunchOptions options;
        options.headless                     = true;
        options.forceWarp                    = true;
        options.dataDirectory                = directory / L"slow";
        options.printWaitingDelayMsForTest   = 40u;
        options.printPrinterTimeoutMsForTest = 30000u;
        options.printDestinationsForTest     = Printing::Destinations{{std::wstring(PdfPrinter), std::wstring(slowPrinter)}, std::wstring(slowPrinter)};
        options.printPrinterForTest          = [slowState, slowName = std::wstring(slowPrinter)](const std::wstring& name,
                                                                                                 const std::optional<Printing::PrinterSettings>& settings,
                                                                                                 Printing::PrinterConfiguration& configuration) -> HRESULT
        {
            if (name == slowName && slowState->holdSlow.load(std::memory_order_acquire))
            {
                const auto signalReturn = wil::scope_exit([slowState]() noexcept { slowState->returnedSlow.SetEvent(); });
                slowState->enteredSlow.SetEvent();
                const bool signalled = slowState->releaseSlow.wait(15000u);
                if (! signalled)
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            return ConfigureFixture(name, settings, configuration);
        };

        Application app(std::move(options));
        const auto cleanupRelease = wil::scope_exit([slowState]
        {
            slowState->holdSlow.store(false, std::memory_order_release);
            slowState->releaseSlow.SetEvent();
            if (slowState->enteredSlow.wait(0u))
                (void)slowState->returnedSlow.wait(10000u);
        });
        Require(SUCCEEDED(app.Initialize()), "initialize slow waiting-overlay host");
        Require(app.SetDocument(L"# Delayed print preview\n\nWaiting indicator coverage."), "seed delayed document");
        app.Host().SetFocusControl(&app.Document(), false);

        const auto pumpUntil = [&](auto&& predicate, std::string_view detail)
        {
            const auto start = GetTickCount64();
            while (! predicate() && GetTickCount64() - start < 15000u)
            {
                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (! predicate())
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 15, QS_ALLINPUT, 0);
            }
            if (! predicate())
                throw std::runtime_error(std::string(detail));
        };

        const auto captureCenterPixels = [&](DxUi::WindowHostBitmapCapture& capture, std::vector<std::uint8_t>& region)
        {
            auto& host = app._printPreview->Host();
            for (unsigned prep = 0u; prep < 16u && host.HasPendingContentPreparation(); ++prep)
                (void)host.RenderInitialFrameForShow();
            Require(host.RenderInitialFrameForShow() && host.DebugCaptureBitmap(capture, true), "capture hidden waiting overlay bitmap");
            const auto bounds = app._printPreview->WaitingOverlayBounds();
            const float scale = host.GetDpi() / 96.f;
            const auto cx     = static_cast<UINT>((bounds.left + bounds.right) * .5f * scale);
            const auto cy     = static_cast<UINT>((bounds.top + bounds.bottom) * .5f * scale);
            const UINT half   = 70u;
            Require(cx > half && cy > half && cx + half < capture.widthPx && cy + half < capture.heightPx, "waiting overlay center fits capture");
            region.clear();
            bool hasDistinctPixel{};
            const auto bgOffset =
                ((static_cast<std::size_t>(bounds.top * scale) + 6u) * capture.widthPx + static_cast<std::size_t>(bounds.left * scale) + 6u) * 4u;
            for (UINT y = cy - half; y <= cy + half; ++y)
            {
                const auto row = (static_cast<std::size_t>(y) * capture.widthPx + (cx - half)) * 4u;
                region.insert(region.end(), capture.bgraPixels.data() + row, capture.bgraPixels.data() + row + static_cast<std::size_t>(half * 2u + 1u) * 4u);
                for (UINT x = cx - half; x <= cx + half; ++x)
                {
                    const auto px = (static_cast<std::size_t>(y) * capture.widthPx + x) * 4u;
                    if (! std::equal(capture.bgraPixels.data() + px, capture.bgraPixels.data() + px + 3u, capture.bgraPixels.data() + bgOffset))
                        hasDistinctPixel = true;
                }
            }
            Require(hasDistinctPixel, "waiting overlay renders visible artwork over the preview background");
        };

        const auto findWaitingControl = [&]() -> DxUi::Control&
        {
            const auto search = [&](this const auto& self, DxUi::Control* parent) -> DxUi::Control*
            {
                if (parent->GetAccessibilityRole() == DxUi::AccessibilityRole::Status)
                    return parent;
                for (std::size_t i = 0; i < parent->GetLogicalChildCount(); ++i)
                    if (auto* const found = self(parent->GetLogicalChild(i)))
                        return found;
                return nullptr;
            };
            auto* const control = app._printPreview ? search(app._printPreview->Host().GetRoot()) : nullptr;
            Require(control != nullptr, "locate waiting status control");
            return *control;
        };

        app.Execute(Command::Print);
        Require(app._printPreview && ! app._printPreview->IsWaitingOverlayVisible(), "waiting overlay stays hidden before timer elapses");
        Require(slowState->enteredSlow.wait(5000u), "slow printer enters configuration wait");
        pumpUntil([&] {
            return app._printPreview && app._printPreview->IsWaitingOverlayVisible() && ! Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).GetItems().empty();
        }, "waiting overlay reveals after delay with populated destinations");

        auto& printers1       = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        const auto nextBounds = Find<DxUi::Button>(app, IDS_PRINTNEXT).GetBounds();
        const auto waitBounds = app._printPreview->WaitingOverlayBounds();
        Require(printers1.IsEnabled() && app._printPreview->WaitingOverlayMessage().find(slowPrinter) != std::wstring_view::npos &&
                    waitBounds.left >= printers1.GetBounds().right && waitBounds.top >= nextBounds.bottom && waitBounds.right - waitBounds.left >= 200.f &&
                    waitBounds.bottom - waitBounds.top >= 150.f,
                "waiting overlay is centered inside preview canvas and leaves Printer enabled");

        std::filesystem::create_directories(directory);
        DxUi::WindowHostBitmapCapture darkCapture, lightCapture, frameA, frameB, staticCapture, staticCapture2, hcCapture;
        std::vector<std::uint8_t> darkPixels, lightPixels, pixelsA, pixelsB, staticPixels, staticPixels2, hcPixels;
        app.Execute(Command::ThemeDark);
        captureCenterPixels(darkCapture, darkPixels);
        Require(app.Capture(directory / L"waiting-dark.png"), "capture dark waiting overlay preview");
        app.Execute(Command::ThemeLight);
        captureCenterPixels(lightCapture, lightPixels);
        Require(app.Capture(directory / L"waiting-light.png"), "capture light waiting overlay preview");
        Require(darkPixels != lightPixels, "waiting overlay adapts cleanly between Dark and Light themes");

        auto& host             = app._printPreview->Host();
        auto& statusCtrl       = findWaitingControl();
        auto animated          = host.GetTheme();
        animated.reducedMotion = false;
        app._printPreview->SetTheme(animated);
        Require(host.DebugHasActiveAnimationSubscription(), "animated waiting card subscribes for frame updates when motion is enabled");
        Require(statusCtrl.Tick(host, host.GetAnimationTickMs() + 100u), "waiting overlay requests continuous ticks while active");
        app._printPreview->AdvanceWaitingForTest(450u);
        captureCenterPixels(frameA, pixelsA);
        app._printPreview->AdvanceWaitingForTest(900u);
        captureCenterPixels(frameB, pixelsB);
        Require(pixelsA != pixelsB, "waiting animation visibly updates between frames");

        animated.reducedMotion = true;
        app._printPreview->SetTheme(animated);
        Require(! statusCtrl.Tick(host, host.GetAnimationTickMs() + 950u), "reducedMotion stops tick continuation");
        const auto retireDeadline = GetTickCount64() + 2000u;
        while (host.DebugHasActiveAnimationSubscription() && GetTickCount64() < retireDeadline)
        {
            MSG message{};
            if (PeekMessageW(&message, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE))
                DispatchMessageW(&message);
            else
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_TIMER, 0);
        }
        Require(! host.DebugHasActiveAnimationSubscription(), "animation dispatcher cleanly retires subscription once reducedMotion is enabled");
        captureCenterPixels(staticCapture, staticPixels);
        app._printPreview->AdvanceWaitingForTest(600u);
        captureCenterPixels(staticCapture2, staticPixels2);
        Require(staticPixels2 == staticPixels, "reducedMotion waiting card pixels stay invariant across ticks");

        auto highContrast         = animated;
        highContrast.highContrast = true;
        app._printPreview->SetTheme(highContrast);
        captureCenterPixels(hcCapture, hcPixels);
        Require(hcPixels != lightPixels, "high contrast waiting overlay renders visibly distinct theme styling");
        app._printPreview->SetTheme(animated);

        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "blocked slow printer completes");
        pumpUntil([&] { return app._printPreview && app._printPreview->CanPrint() && ! app._printPreview->IsWaitingOverlayVisible(); },
                  "waiting overlay hides immediately once preview presents");
        Require(! app._printPreview->Host().DebugHasActiveAnimationSubscription(), "completed preview leaves no animation subscription");
        app.DismissPrintPreview();
        Wait(app);

        // Immediate printer switch while waiting clears waiting overlay without flash.
        slowState->enteredSlow.ResetEvent();
        slowState->releaseSlow.ResetEvent();
        slowState->returnedSlow.ResetEvent();
        app.Execute(Command::Print);
        Require(slowState->enteredSlow.wait(5000u), "slow printer blocks again before queue switch");
        pumpUntil([&] {
            return app._printPreview && app._printPreview->IsWaitingOverlayVisible() && ! Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).GetItems().empty();
        }, "waiting overlay appears before switching to fast PDF printer");
        auto& printers2 = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers2.OnKeyDown(app._printPreview->Host(), VK_UP, 0), "switch immediately to PdfPrinter while waiting card is up");
        Require(! app._printPreview->IsWaitingOverlayVisible(), "switching printers immediately clears the waiting overlay");
        Wait(app, true);
        Require(! app._printPreview->IsWaitingOverlayVisible() && app._printPreview->CanPrint() && app._printPreview->Page().paper == WidePaper(),
                "fast PDF printer presents without restoring stale waiting overlay");
        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "abandoned slow printer finishes after switch");
        app.DismissPrintPreview();
        Wait(app);

        // Dismiss before delay timer fires detaches cleanly without crash or stray overlay.
        slowState->enteredSlow.ResetEvent();
        slowState->releaseSlow.ResetEvent();
        slowState->returnedSlow.ResetEvent();
        app.Execute(Command::Print);
        Require(! app._printPreview->IsWaitingOverlayVisible(), "overlay stays hidden on immediate open");
        Require(slowState->enteredSlow.wait(5000u), "slow printer blocks before early dismissal");
        app.DismissPrintPreview();
        Require(! app._printPreview && ! app._printOperation, "early dismiss closes preview immediately");
        const auto drainStart = GetTickCount64();
        while (GetTickCount64() - drainStart < 80u)
        {
            MSG message{};
            while (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        slowState->releaseSlow.SetEvent();
        Require(slowState->returnedSlow.wait(10000u), "blocked slow printer test probe unwinds before teardown");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "waiting-overlay test preserves desktop");
    }

    static void RapidSwitchWithPendingProbe(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        struct SlowState
        {
            SlowState()                            = default;
            SlowState(const SlowState&)            = delete;
            SlowState& operator=(const SlowState&) = delete;
            SlowState(SlowState&&)                 = delete;
            SlowState& operator=(SlowState&&)      = delete;
            wil::unique_event entered{wil::EventOptions::ManualReset};
            wil::unique_event release{wil::EventOptions::ManualReset};
            wil::unique_event returned{wil::EventOptions::ManualReset};
            std::atomic_bool hold{true};
        };
        auto slowState                          = std::make_shared<SlowState>();
        constexpr std::wstring_view slowPrinter = L"Redfern rapid-switch slow printer";

        LaunchOptions options;
        options.headless                     = true;
        options.forceWarp                    = true;
        options.dataDirectory                = directory;
        options.printWaitingDelayMsForTest   = 20u;
        options.printPrinterTimeoutMsForTest = 30000u;
        options.printDestinationsForTest     = Printing::Destinations{{std::wstring(PdfPrinter), std::wstring(slowPrinter)}, std::wstring(slowPrinter)};
        options.printPrinterForTest          = [slowState, slowName = std::wstring(slowPrinter)](const std::wstring& name,
                                                                                                 const std::optional<Printing::PrinterSettings>& settings,
                                                                                                 Printing::PrinterConfiguration& configuration) -> HRESULT
        {
            if (name == slowName && slowState->hold.load(std::memory_order_acquire))
            {
                const auto signalReturn = wil::scope_exit([slowState]() noexcept { slowState->returned.SetEvent(); });
                slowState->entered.SetEvent();
                const bool signalled = slowState->release.wait(15000u);
                if (! signalled)
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            return ConfigureFixture(name, settings, configuration);
        };

        Application app(std::move(options));
        const auto cleanup = wil::scope_exit([slowState]
        {
            slowState->hold.store(false, std::memory_order_release);
            slowState->release.SetEvent();
            if (slowState->entered.wait(0u))
                (void)slowState->returned.wait(10000u);
        });
        Require(SUCCEEDED(app.Initialize()), "initialize rapid-switch print preview fixture");
        Require(app.SetDocument(MultipageSource()), "seed multipage document for rapid-switch fixture");

        app.Execute(Command::Print);
        Require(slowState->entered.wait(5000u), "slow probe enters while initial preview opens");
        const auto waitStart = GetTickCount64();
        while (GetTickCount64() - waitStart < 5000u)
        {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (app._printPreview && ! Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION).GetItems().empty())
                break;
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, 0);
        }
        auto& printers = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
        Require(printers.GetItems().size() == 2u, "destinations populate before rapid-switch stress");

        // Fire back-to-back requests on the auto-reset event while the slow probe is pending (C1 regression).
        Require(printers.OnKeyDown(app._printPreview->Host(), VK_UP, 0), "switch UI destination to PdfPrinter while slow probe runs");
        Printing::RequestPreview(*app._printOperation, 1u);
        Wait(app, true);
        Require(app._printPreview->Printer() == PdfPrinter && app._printPreview->CanPrint() && ! app._printPreview->IsWaitingOverlayVisible(),
                "rapid back-to-back wake settles onto PdfPrinter instead of deadlocking");

        // Rapidly bounce slowPrinter -> PdfPrinter without waiting in between.
        Require(printers.OnKeyDown(app._printPreview->Host(), VK_DOWN, 0), "switch back toward slowPrinter");
        Require(printers.OnKeyDown(app._printPreview->Host(), VK_UP, 0), "bounce immediately back to PdfPrinter");
        Printing::RequestPreview(*app._printOperation, 1u);
        Wait(app, true);
        Require(app._printPreview->Printer() == PdfPrinter && app._printPreview->Page().number == 1u && app._printPreview->CanPrint(),
                "rapid printer bounce settles cleanly onto final fast printer");

        slowState->hold.store(false, std::memory_order_release);
        slowState->release.SetEvent();
        Require(slowState->returned.wait(10000u), "slow probe completes after rapid switch");
        const auto drainStart = GetTickCount64();
        while (GetTickCount64() - drainStart < 100u)
        {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, 0);
        }
        Require(app._printPreview && app._printPreview->IsReady() && app._printPreview->Printer() == PdfPrinter && app._printPreview->CanPrint() &&
                    ! app._printPreview->IsWaitingOverlayVisible(),
                "completed stale slow probe never wedges preview into WaitingOverlay after rapid switch");
        app.DismissPrintPreview();
        Wait(app);
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "rapid-switch test preserves desktop");
    }

    static void PageRangesAndKeyboardShortcuts(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto valid = Printing::ParsePageRanges(L"1-3, 5", 10u);
        Require(valid && valid->size() == 2u && (*valid)[0].first == 1u && (*valid)[0].last == 3u && (*valid)[1].first == 5u && (*valid)[1].last == 5u,
                "ParsePageRanges accepts valid ranges within page count");

        std::wstring exactly32;
        for (unsigned index = 1u; index <= 32u; ++index)
        {
            if (index > 1u)
                exactly32 += L',';
            exactly32 += std::to_wstring(index);
        }
        const auto limit32 = Printing::ParsePageRanges(exactly32, 64u);
        Require(limit32 && limit32->size() == 32u, "ParsePageRanges accepts the maximum 32 ranges");
        const auto over32 = Printing::ParsePageRanges(exactly32 + L",33", 64u);
        Require(! over32.has_value(), "ParsePageRanges rejects 33 ranges");

        const auto maxPage = Printing::ParsePageRanges(L"1, 512", 512u);
        Require(maxPage && maxPage->size() == 2u && (*maxPage)[1].first == 512u, "ParsePageRanges accepts 512th page boundary");
        Require(! Printing::ParsePageRanges(L"513", 512u).has_value() && ! Printing::ParsePageRanges(L"0", 10u).has_value() &&
                    ! Printing::ParsePageRanges(L"4-2", 10u).has_value(),
                "ParsePageRanges rejects out-of-range, zero, and reversed intervals");

        std::atomic_uint submitted{};
        LaunchOptions options;
        options.headless                 = true;
        options.forceWarp                = true;
        options.dataDirectory            = directory;
        options.printDestinationsForTest = Printing::Destinations{{std::wstring(PdfPrinter)}, std::wstring(PdfPrinter)};
        options.printPrinterForTest      = ConfigureFixture;
        options.printSubmissionForTest   = [&](const Printing::Selection&) { submitted.fetch_add(1u, std::memory_order_acq_rel); };
        options.printPageForTest         = [](unsigned, ID2D1CommandList&) { return S_OK; };

        Application app(std::move(options));
        Require(SUCCEEDED(app.Initialize()), "initialize keyboard print shortcut fixture");
        Require(app.SetDocument(MultipageSource()), "seed multipage document for keyboard print test");

        {
            KeyboardState::ScopedSynthetic synthetic;
            std::array<BYTE, 256> keys{};
            keys[VK_CONTROL] = 0x80u;
            KeyboardState::Write(keys);

            const MSG repeatPrint{app.Window(), WM_KEYDOWN, 'P', static_cast<LPARAM>(1) << 30};
            Require(app.RouteShortcut(repeatPrint) && ! app._printPreview, "auto-repeat Ctrl+P does not open print preview");
            const MSG firstPrint{app.Window(), WM_KEYDOWN, 'P', 0};
            Require(app.RouteShortcut(firstPrint) && app._printPreview != nullptr, "Ctrl+P opens print preview");

            keys[VK_CONTROL] = 0u;
            KeyboardState::Write(keys);
            Wait(app, true);
            Require(app._printPreview->Page().count >= 2u, "fixture spans multiple pages");

            auto& zoomButton = Find<DxUi::Button>(app, IDS_PRINTFITWIDTH);
            app._printPreview->Host().SetFocusControl(&zoomButton, false);

            const MSG nextKey{app._printPreview->Window(), WM_KEYDOWN, VK_NEXT, 0};
            Require(app._printPreview->DispatchKeyboardMessage(nextKey), "VK_NEXT is handled inside print preview");
            Wait(app, true);
            Require(app._printPreview->Page().number == 2u, "VK_NEXT advances to page 2");

            const MSG prevKey{app._printPreview->Window(), WM_KEYDOWN, VK_PRIOR, 0};
            Require(app._printPreview->DispatchKeyboardMessage(prevKey), "VK_PRIOR is handled inside print preview");
            Wait(app, true);
            Require(app._printPreview->Page().number == 1u, "VK_PRIOR returns to page 1");

            keys[VK_CONTROL] = 0x80u;
            KeyboardState::Write(keys);
            const MSG previewPrint{app._printPreview->Window(), WM_KEYDOWN, 'P', 0};
            Require(app._printPreview->DispatchKeyboardMessage(previewPrint), "Ctrl+P submits from inside print preview");
            keys[VK_CONTROL] = 0u;
            KeyboardState::Write(keys);

            const auto start = GetTickCount64();
            while (app._printPreview && GetTickCount64() - start < 5000u)
            {
                MSG message{};
                if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                else
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, 0);
            }
            Wait(app);
            Require(! app._printPreview && submitted.load(std::memory_order_acquire) == 1u, "Ctrl+P submitted print job and closed preview");
        }
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "keyboard shortcut print test preserves desktop");
    }

    static void EmptyPrintersProbeCapAndRememberImage(const std::filesystem::path& directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        // 1. Empty printer destinations list opens fallback preview with disabled Print and destination selector.
        {
            LaunchOptions options;
            options.headless                 = true;
            options.forceWarp                = true;
            options.dataDirectory            = directory / L"empty-printers";
            options.printDestinationsForTest = Printing::Destinations{{}, {}};
            options.printPrinterForTest      = ConfigureFixture;
            Application app(std::move(options));
            Require(SUCCEEDED(app.Initialize()), "initialize empty-destinations fixture");
            Require(app.SetDocument(L"# Empty printer list"), "seed empty-printers document");
            Open(app);
            auto& printers = Find<DxUi::ComboBox>(app, IDS_PRINTDESTINATION);
            Require(printers.GetItems().empty() && ! printers.IsEnabled() && ! app._printPreview->CanPrint() && app._printPreview->Page().fallbackPaper,
                    "empty printer list disables printer combo and Print while keeping fallback preview browsable");
            app.DismissPrintPreview();
            Wait(app);
        }

        // 2. Spooler probe pool is bounded by MaximumSpoolerProbes (8).
        {
            struct CapState
            {
                CapState()                           = default;
                CapState(const CapState&)            = delete;
                CapState& operator=(const CapState&) = delete;
                CapState(CapState&&)                 = delete;
                CapState& operator=(CapState&&)      = delete;
                wil::unique_event atCap{wil::EventOptions::ManualReset};
                wil::unique_event releaseAll{wil::EventOptions::ManualReset};
                wil::unique_event drained{wil::EventOptions::None};
                std::atomic_uint entered{};
                std::atomic_uint active{};
                std::atomic_uint returned{};
                std::atomic_bool hold{true};
            };
            auto capState = std::make_shared<CapState>();
            std::vector<std::wstring> names;
            for (unsigned i = 1u; i <= 9u; ++i)
                names.push_back(std::format(L"Redfern cap {}", i));
            names.push_back(std::wstring(PdfPrinter));

            LaunchOptions options;
            options.headless                     = true;
            options.forceWarp                    = true;
            options.dataDirectory                = directory / L"probe-cap";
            options.printPrinterTimeoutMsForTest = 30u;
            options.printDestinationsForTest     = Printing::Destinations{names, names.front()};
            options.printPrinterForTest          = [capState](const std::wstring& name,
                                                              const std::optional<Printing::PrinterSettings>& settings,
                                                              Printing::PrinterConfiguration& config) -> HRESULT
            {
                if (name.starts_with(L"Redfern cap ") && capState->hold.load(std::memory_order_acquire))
                {
                    const auto unwind = wil::scope_exit([capState]() noexcept
                    {
                        capState->returned.fetch_add(1u, std::memory_order_acq_rel);
                        if (capState->active.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
                            capState->drained.SetEvent();
                    });
                    capState->entered.fetch_add(1u, std::memory_order_acq_rel);
                    if (capState->active.fetch_add(1u, std::memory_order_acq_rel) + 1u == 8u)
                        capState->atCap.SetEvent();
                    (void)capState->releaseAll.wait(15000u);
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                }
                return ConfigureFixture(name, settings, config);
            };

            Application app(std::move(options));
            const auto cleanupCap = wil::scope_exit([capState]
            {
                capState->hold.store(false, std::memory_order_release);
                capState->releaseAll.SetEvent();
                const auto deadline = GetTickCount64() + 10000u;
                while (capState->active.load(std::memory_order_acquire) > 0u && GetTickCount64() < deadline)
                    (void)capState->drained.wait(50u);
            });
            Require(SUCCEEDED(app.Initialize()), "initialize spooler probe-cap fixture");
            Require(app.SetDocument(L"# Probe cap fixture"), "seed probe-cap document");
            app.Execute(Command::Print);

            const auto waitEntered = [&](unsigned expected)
            {
                const auto deadline = GetTickCount64() + 5000u;
                while (capState->entered.load(std::memory_order_acquire) < expected && GetTickCount64() < deadline)
                {
                    MSG message{};
                    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&message);
                        DispatchMessageW(&message);
                    }
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, 0);
                }
                Require(capState->entered.load(std::memory_order_acquire) == expected, "expected probe count entered");
            };
            waitEntered(1u);
            for (unsigned i = 2u; i <= 8u; ++i)
            {
                Printing::RequestPrinter(*app._printOperation, 1u, names[i - 1u]);
                waitEntered(i);
            }
            Require(capState->atCap.wait(5000u), "8 active spooler probes reached cap");
            Printing::RequestPrinter(*app._printOperation, 1u, names[8]); // 9th stalled printer must hit cap without spawning probe
            Wait(app, true);
            Require(capState->entered.load(std::memory_order_acquire) == 8u && app._printPreview->Page().fallbackPaper,
                    "MaximumSpoolerProbes (8) blocks spawning an unbounded 9th spooler probe");

            capState->hold.store(false, std::memory_order_release);
            capState->releaseAll.SetEvent();
            app.DismissPrintPreview();
            Wait(app);
        }

        // 3. RememberPrintImage deduplicates remote images and ignores local files.
        {
            LaunchOptions options;
            options.headless      = true;
            options.dataDirectory = directory / L"remember-image";
            Application app(std::move(options));
            Require(SUCCEEDED(app.Initialize()), "initialize RememberPrintImage fixture");
            constexpr std::array<std::byte, 58u> bmpBytes{
                std::byte{0x42}, std::byte{0x4D}, std::byte{58},   std::byte{0},   std::byte{0}, std::byte{0},  std::byte{0}, std::byte{0}, std::byte{0},
                std::byte{0},    std::byte{54},   std::byte{0},    std::byte{0},   std::byte{0}, std::byte{40}, std::byte{0}, std::byte{0}, std::byte{0},
                std::byte{1},    std::byte{0},    std::byte{0},    std::byte{0},   std::byte{1}, std::byte{0},  std::byte{0}, std::byte{0}, std::byte{1},
                std::byte{0},    std::byte{24},   std::byte{0},    std::byte{0},   std::byte{0}, std::byte{0},  std::byte{0}, std::byte{4}, std::byte{0},
                std::byte{0},    std::byte{0},    std::byte{0},    std::byte{0},   std::byte{0}, std::byte{0},  std::byte{0}, std::byte{0}, std::byte{0},
                std::byte{0},    std::byte{0},    std::byte{0},    std::byte{0},   std::byte{0}, std::byte{0},  std::byte{0}, std::byte{0}, std::byte{0},
                std::byte{0x10}, std::byte{0x20}, std::byte{0xFF}, std::byte{0x00}};
            const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
            wil::com_ptr<IWICImagingFactory> factory;
            Require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put()))),
                    "create WIC factory for RememberPrintImage fixture");
            std::shared_ptr<const DxUi::RichText::RichImage> image;
            Require(DxUi::RichText::RichImage::Decode(factory.get(), bmpBytes, image) == S_OK && image != nullptr, "decode synthetic 1x1 BMP");

            Require(app.SetDocument(L"![Local](local.bmp)\n\n![Remote](https://example.org/a.bmp)\n", directory / L"remember.md"),
                    "seed image Markdown for RememberPrintImage");
            app.Document().SetRemoteImagesEnabled(true);
            std::vector<DxUi::RichText::RichImageRequest> allRequests;
            for (unsigned turn = 0u; turn < 256u && allRequests.size() < 2u; ++turn)
            {
                (void)app.Host().RenderInitialFrameForShow();
                std::vector<DxUi::RichText::RichImageRequest> batch;
                app.Document().TakePendingImageRequests(batch);
                allRequests.insert(allRequests.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
            }
            const auto localIt = std::ranges::find(allRequests, DxUi::RichText::RichImageSourceKind::LocalFile, &DxUi::RichText::RichImageRequest::SourceKind);
            const auto remoteIt =
                std::ranges::find(allRequests, DxUi::RichText::RichImageSourceKind::RemoteHttp, &DxUi::RichText::RichImageRequest::SourceKind);
            Require(localIt != allRequests.end() && remoteIt != allRequests.end(), "editor emitted LocalFile and RemoteHttp RichImageRequest objects");
            app.RememberPrintImage(*localIt, image);
            Require(app._printNetworkImages.empty(), "RememberPrintImage ignores local file requests");
            app.RememberPrintImage(*remoteIt, image);
            app.RememberPrintImage(*remoteIt, image);
            Require(app._printNetworkImages.size() == 1u && app._printNetworkImages.front().request.Source() == L"https://example.org/a.bmp",
                    "RememberPrintImage stores and deduplicates network image entries");
        }
        Require(foreground.NeverAcquiredForeground(), "empty printers, probe cap, and RememberPrintImage tests preserve desktop");
    }
};

void RegisterPrintingTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Editor.PrintingPreservesRevisionAndHonorsPageRanges", [&] { PrintingProbe::StateAndRanges(root / L"print-pages"); });
    test("Editor.PrintingFailureAndShutdownRetireJobs", [&] { PrintingProbe::FailureAndShutdown(root / L"print-shutdown"); });
    test("Editor.PrintingDiagramFollowsCanvasAndPaper", [&] { PrintingProbe::DiagramBackground(root / L"print-diagram"); });
    test("Editor.PrintingCloseOverridesPendingPrintAction", [&] { PrintingProbe::CloseOverridesPendingPrintAction(root / L"print-close-override"); });
    test("Editor.PrintingSlowPrinterAllowsSwitchAndFallback", [&] { PrintingProbe::SlowPrinterAllowsSwitchAndFallback(root / L"print-slow-printer"); });
    test("Editor.PrintingWaitingOverlayShowsAfterDelayAndClears",
         [&] { PrintingProbe::WaitingOverlayShowsAfterDelayAndClears(root / L"print-waiting-overlay"); });
    test("Editor.PrintPreviewRapidSwitchWithPendingProbeDoesNotDeadlock", [&] { PrintingProbe::RapidSwitchWithPendingProbe(root / L"print-rapid-switch"); });
    test("Editor.PrintPageRangesAndKeyboardShortcuts", [&] { PrintingProbe::PageRangesAndKeyboardShortcuts(root / L"print-keyboard-ranges"); });
    test("Editor.PrintPreviewEmptyPrintersProbeCapAndRememberImage",
         [&] { PrintingProbe::EmptyPrintersProbeCapAndRememberImage(root / L"print-empty-cap-image"); });
}
} // namespace RedTilde
