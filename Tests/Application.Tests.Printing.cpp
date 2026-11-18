#include "Application.Tests.Printing.h"
#include "RedTilde/Application.h"
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
    static void Wait(Application &app)
    {
        const auto start = GetTickCount64();
        while (app.printOperation && GetTickCount64() - start < 15000u)
        {
            MSG message{};
            if (PeekMessageW(&message, app.Window(), Application::PrintReady, Application::PrintReady, PM_REMOVE))
                DispatchMessageW(&message);
            else
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_POSTMESSAGE, 0);
        }
        Require(!app.printOperation, "print worker publishes a terminal result");
    }

    static void StateAndRanges(const std::filesystem::path &directory)
    {
        Testing::ForegroundOwnershipObserver foreground;
        HRESULT dialogResult = S_FALSE;
        std::vector<Printing::PageRange> ranges;
        unsigned openings{};
        std::vector<unsigned> pages;
        LaunchOptions options;
        options.headless = true;
        options.forceWarp = true;
        options.dataDirectory = directory;
        options.printDialogForTest = [&](Printing::Selection &selected)
        {
            ++openings;
            selected.paper = SmallPaper();
            selected.ranges = ranges;
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
        app.OnMenuClosed(menuCommand);
        Require(openings == 1u && pages.empty() && !app.printOperation, "dismissing the print dialog admits no page");
        dialogResult = S_OK;
        unsigned count{};
        for (const auto view : {Command::Compose, Command::Source, Command::Split})
        {
            app.Execute(view);
            pages.clear();
            app.OnMenuClosed(menuCommand);
            const auto admitted = openings;
            app.Execute(Command::Print);
            Require(openings == admitted, "a duplicate command cannot supersede the admitted job");
            Wait(app);
            Require(app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED) && pages.size() >= 4u && pages.front() == 1u &&
                        pages.back() == pages.size(),
                    "the print command records every page in order");
            if (count == 0u)
                count = static_cast<unsigned>(pages.size());
            Require(count == pages.size(), "every editor view uses the same pagination");
            Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection && app.Document().CanUndo() &&
                        app.Dirty() && app.documentPath.empty(),
                    "printing preserves unsaved source, selection, history and identity");
        }
        pages.clear();
        ranges = {{count, count}, {1u, 2u}, {2u, 2u}};
        app.Execute(Command::Print);
        Wait(app);
        Require(std::ranges::equal(pages, std::array{1u, 2u, count}), "disjoint and overlapping requests print once in document order");
        pages.clear();
        ranges = {{count + 1u, count + 1u}};
        app.Execute(Command::Print);
        Wait(app);
        Require(pages.empty() && app.feedback->GetText() == std::vformat(Resource(IDS_PRINTRANGE), std::make_wformat_args(count)),
                "an invalid page range reports the final count before recording anything");

        ranges.clear();
        app.Execute(Command::Print);
        Require(app.SetDocument(L"Second document."), "switch the editor after print admission");
        Wait(app);
        Require(pages.size() == count && app.Document().GetText() == L"Second document.",
                "an admitted job retains the old revision without reapplying it to the editor");
        pages.clear();
        Require(app.SetDocument(L""), "seed an empty document");
        app.Execute(Command::Print);
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
        options.printDialogForTest = [](Printing::Selection &selected)
        {
            selected.paper = SmallPaper();
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
        Require(calls == 2u && app.feedback->GetText() == Resource(IDS_PRINTFAILED), "a failed page stops submission without a retry");
        failPage = false;
        failFinish = true;
        app.Execute(Command::Print);
        Wait(app);
        Require(calls > 2u && app.feedback->GetText() == Resource(IDS_PRINTFAILED), "finalization failure is distinct from a submitted job");
        failFinish = false;
        app.Execute(Command::Print);
        Wait(app);
        Require(app.feedback->GetText() == Resource(IDS_PRINTSUBMITTED), "a new job succeeds after recoverable failures");

        const auto previous = calls.load();
        pause.store(true);
        app.Execute(Command::Print);
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
