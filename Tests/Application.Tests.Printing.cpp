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

    static void Navigate(Application &app, unsigned page)
    {
        auto &host = app.printPreview->Host();
        auto *const parent = host.GetRoot();
        for (std::size_t index = 0; index < parent->GetLogicalChildCount(); ++index)
            if (auto *control = dynamic_cast<DxUi::NumericStepper *>(parent->GetLogicalChild(index)))
            {
                Require(control->RequestValue(host, page), "the preview page field accepts a page number");
                Wait(app, true);
                Require(app.printPreview->Page().number == page, "the preview displays the requested page");
                return;
            }
        Require(false, "the preview exposes an editable page number");
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
        bool confirmed = true, currentPage{};
        auto paper = SmallPaper();
        Printing::DialogOptions lastOptions;
        std::vector<Printing::PageRange> initialRanges;
        std::vector<Printing::PageRange> ranges;
        unsigned openings{};
        std::vector<unsigned> pages;
        LaunchOptions options;
        options.headless = true;
        options.forceWarp = true;
        options.dataDirectory = directory;
        options.printPaperForTest = SmallPaper();
        options.printDialogForTest = [&](const Printing::DialogOptions &initial, Printing::DialogResult &selected)
        {
            ++openings;
            lastOptions = initial;
            initialRanges.assign(initial.ranges.begin(), initial.ranges.end());
            lastOptions.ranges = {};
            selected.confirmed = confirmed;
            selected.selection.paper = paper;
            selected.selection.printer = L"Redfern document printer";
            selected.currentPage = currentPage;
            selected.selection.ranges = currentPage ? std::vector<Printing::PageRange>{} : ranges;
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
        Navigate(app, 2u);
        Require(HasInk(app.printPreview->Page()) && app.printPreview->Page().bgra != firstPixels, "navigation presents distinct rendered sheets");
        app.Execute(Command::ThemeDark);
        std::filesystem::create_directories(directory);
        Require(app.Capture(directory / L"print-preview-page-2.png"), "the real preview frame captures through the hidden application host");
        Choose(app);
        Require(openings == 1u && lastOptions.pageCount == count && pages.empty() && app.printPreview->Page().number == 2u &&
                    app.printPreview->CanPrint(),
                "the native dialog receives the actual page count; cancellation leaves the visible page open");
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
        ranges = {{count, count}, {1u, 2u}, {2u, 2u}};
        Open(app);
        Choose(app);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u, 2u, count}), "disjoint and overlapping requests print once in document order");
        pages.clear();
        ranges = {{count + 1u, count + 1u}};
        Open(app);
        Choose(app);
        Require(pages.empty() && app.printPreview->CanPrint() && app.printRanges.empty(),
                "an invalid page range leaves the preview open for correction");

        paper = WidePaper();
        ranges = {{count, count}};
        Choose(app);
        Require(app.printPreview && !app.printPreview->CanPrint() && pages.empty(), "changing paper defers submission before showing the new pages");
        Wait(app, true);
        const auto &landscape = app.printPreview->Page();
        Require(landscape.paper == paper && landscape.width > landscape.height && landscape.count < count && HasInk(landscape),
                "landscape uses its own page geometry and recomputes the page count");
        Require(app.printRanges.empty() && pages.empty(), "a range that no longer fits the new paper must be chosen again");
        confirmed = false;
        ranges = {{landscape.count, landscape.count}};
        Choose(app);
        Require(lastOptions.rangeNeedsCorrection && initialRanges.empty() && pages.empty(),
                "the invalid range reopens with Pages selected for correction and Apply submits zero pages");
        currentPage = true;
        const auto selected = landscape.count;
        Choose(app);
        Require(initialRanges.size() == 1u && initialRanges.front().first == selected && pages.empty(),
                "Apply preserves an explicit range without printing");
        confirmed = true;
        Require(selected >= 2u, "the landscape fixture still has more than one page");
        Require(app.SetDocument(L"Second document."), "switch the editor while the preview retains the original revision");
        Navigate(app, selected);
        Require(HasInk(app.printPreview->Page()), "the preview still renders the last page of the admitted document after an editor change");
        Choose(app);
        Wait(app);
        Require(lastOptions.currentPage && std::ranges::equal(pages, std::array{selected}) && app.Document().GetText() == L"Second document.",
                "current-page printing submits exactly the requested page of the previewed revision");

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
        bool failPage = true, failFinish = false;
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = directory;
        options.unsavedPromptForTest = [] { return IDNO; };
        options.printPaperForTest = Printing::Paper{};
        options.printDialogForTest = [](const Printing::DialogOptions &, Printing::DialogResult &selected)
        {
            selected.selection.paper = SmallPaper();
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
        Wait(app);
        Require(calls == 2u && app.feedback->GetText() == Resource(IDS_PRINTFAILED), "a failed page stops submission without a retry");
        failPage = false;
        failFinish = true;
        Open(app);
        Choose(app);
        Wait(app);
        Require(calls > 2u && app.feedback->GetText() == Resource(IDS_PRINTFAILED), "finalization failure is distinct from a submitted job");
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
};

void RegisterPrintingTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.PrintingPreservesRevisionAndHonorsPageRanges", [&] { PrintingProbe::StateAndRanges(root / L"print-pages"); });
    test("Editor.PrintingFailureAndShutdownRetireJobs", [&] { PrintingProbe::FailureAndShutdown(root / L"print-shutdown"); });
}
} // namespace RedTilde
