#include "Application.Tests.Panes.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view message)
{
    if (! condition)
        throw std::runtime_error(std::string(message));
}
void Success(HRESULT result, std::string_view message)
{
    Require(SUCCEEDED(result), message);
}
bool Near(float left, float right) noexcept
{
    return std::abs(left - right) <= 1.1f;
}
bool WriteText(const std::filesystem::path& path, std::string_view utf8)
{
    std::filesystem::create_directories(path.parent_path());
    return SUCCEEDED(WriteBytesAtomic(path, utf8));
}
} // namespace

class PanesProbe final
{
    static LaunchOptions Options(const std::filesystem::path& root, std::wstring_view name)
    {
        LaunchOptions options;
        options.headless      = true;
        options.dataDirectory = root / name;
        options.scenario      = L"compose-light";
        return options;
    }

    static void Resize(Application& app, unsigned width, unsigned height, UINT dpi = 0, bool prepare = true)
    {
        if (! dpi)
            dpi = GetDpiForWindow(app.Window());
        RECT rect{};
        Require(GetWindowRect(app.Window(), &rect) != FALSE, "read hidden window frame rectangle");
        rect.right  = rect.left + MulDiv(static_cast<int>(width), static_cast<int>(dpi), 96);
        rect.bottom = rect.top + MulDiv(static_cast<int>(height), static_cast<int>(dpi), 96);
        (void)SendMessageW(app.Window(), WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&rect));
        Require(SetWindowPos(app.Window(),
                             nullptr,
                             rect.left,
                             rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                "size hidden panes test window");
        app.Layout();
        if (prepare)
            app.Prepare(128);
    }

    static LPARAM DipToClientLParam(Application& app, D2D1_POINT_2F dip) noexcept
    {
        const float scale = app.Host().GetDpi() / 96.f;
        return MAKELPARAM(static_cast<SHORT>(std::lround(dip.x * scale)), static_cast<SHORT>(std::lround(dip.y * scale)));
    }

public:
    static void RegisterTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
    {
        test("Window.PanelToggleMatchesFilesToggleHoverAndTooltip",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-toggle-hover"));
            Success(app.Initialize(), "initialize hidden panes toggle app");
            Resize(app, 1440, 900);

            Require(app._panelToggle->GetVariant() == DxUi::ButtonVariant::IconOnly,
                    "right panel toggle uses IconOnly variant matching Files toggle (never Disclosure)");
            Require(app._panelToggle->GetAppearance() == app._filesToggle->GetAppearance() &&
                        app._panelToggle->GetAppearance() == DxUi::ButtonAppearance::Subtle,
                    "both pane toggles share Subtle appearance");
            Require(app._panelToggle->GetTooltipText() == Resource(IDS_TIP_PANEL) && app._panelToggle->GetAccessibleName() == Resource(IDS_PANEL),
                    "right panel toggle has localized tooltip and accessible name");
            Require(app._panelToggle->GetToggleState() == app._settings.showOutline, "panel toggle mirrors initial showOutline state");

            const bool initialOutline = app._settings.showOutline;
            app.Execute(Command::Outline);
            Require(app._settings.showOutline == ! initialOutline && app._panelToggle->GetToggleState() == ! initialOutline,
                    "Command::Outline updates showOutline and panel toggle state");
            app.Execute(Command::Outline);
            Require(app._settings.showOutline == initialOutline && app._panelToggle->GetToggleState() == initialOutline,
                    "second Command::Outline restores original state");

            app.Prepare(64);
            DxUi::WindowHostBitmapCapture restCapture;
            Require(app.Host().DebugCaptureBitmap(restCapture, true), "capture resting panel toggle frame");

            const auto b = app._panelToggle->GetBounds();
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f}));
            Require(app._panelToggle->IsHovered(), "synthetic pointer move over checked _panelToggle engages hover state");

            DxUi::WindowHostBitmapCapture hoverCapture;
            Require(app.Host().DebugCaptureBitmap(hoverCapture, true), "capture checked hovered panel toggle frame");
            Require(restCapture.bgraPixels != hoverCapture.bgraPixels, "panel toggle hover changes rendered pill pixels in checked state");
            (void)app.HandleMessage(app.Window(), WM_MOUSELEAVE, 0, 0);

            app.Execute(Command::Outline);
            app.Prepare(64);
            DxUi::WindowHostBitmapCapture offRestCapture;
            Require(app.Host().DebugCaptureBitmap(offRestCapture, true), "capture unchecked resting panel toggle frame");
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f}));
            Require(app._panelToggle->IsHovered(), "synthetic pointer move over unchecked _panelToggle engages hover state");
            DxUi::WindowHostBitmapCapture offHoverCapture;
            Require(app.Host().DebugCaptureBitmap(offHoverCapture, true), "capture unchecked hovered panel toggle frame");
            Require(offRestCapture.bgraPixels != offHoverCapture.bgraPixels, "panel toggle hover changes rendered pill pixels in unchecked state");
            (void)app.HandleMessage(app.Window(), WM_MOUSELEAVE, 0, 0);
            app.Execute(Command::Outline);
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "panel toggle test stays hidden without foreground focus");
        });

        test("Window.SidePaneCollapsePriorityAndHysteresis",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-collapse-hysteresis"));
            Success(app.Initialize(), "initialize hidden collapse-priority app");
            app.Execute(Command::Files);
            Require(app._settings.showFiles && app._settings.showOutline, "both side panes enabled before resize sweep");

            Resize(app, 1440, 900);
            Require(app._filesPane->IsVisible() && app._sidebar->IsVisible() && ! app._leftRail->IsVisible() && ! app._rightRail->IsVisible() &&
                        app._filesSplitter->IsVisible(),
                    "1440x900 docks both Files and Outline panes with no edge rails");

            Resize(app, 1300, 900);
            Require(app._filesPane->IsVisible() && app._filesSplitter->IsVisible() && ! app._leftRail->IsVisible() && ! app._sidebar->IsVisible() &&
                        app._rightRail->IsVisible(),
                    "right pane collapses first while left Files pane remains docked");

            Resize(app, 1385, 900);
            Require(! app._sidebar->IsVisible() && app._rightRail->IsVisible(), "24-DIP hysteresis holds right pane collapsed just above dock threshold");

            Resize(app, 1410, 900);
            Require(app._sidebar->IsVisible() && ! app._rightRail->IsVisible(), "crossing +24 DIP threshold re-docks right pane cleanly");

            Resize(app, 1080, 820);
            Require(! app._filesPane->IsVisible() && ! app._sidebar->IsVisible() && ! app._filesSplitter->IsVisible() && app._leftRail->IsVisible() &&
                        app._rightRail->IsVisible() && Near(app._page->GetBounds().left, CollapsedPaneRail::RailWidthDip) &&
                        Near(1080.f - app._page->GetBounds().right, CollapsedPaneRail::RailWidthDip),
                    "1080x820 collapses both panes into 28-DIP edge rails");

            Resize(app, 1178, 820);
            Require(! app._filesPane->IsVisible() && app._leftRail->IsVisible() && ! app._sidebar->IsVisible() && app._rightRail->IsVisible(),
                    "24-DIP hysteresis holds left Files rail collapsed until 1194 DIP");

            Resize(app, 1205, 820);
            Require(app._filesPane->IsVisible() && app._filesSplitter->IsVisible() && ! app._leftRail->IsVisible() && ! app._sidebar->IsVisible() &&
                        app._rightRail->IsVisible(),
                    "widening past 1194 DIP re-docks left pane before right pane");

            Resize(app, 800, 560);
            Require(app._leftRail->IsVisible() && app._rightRail->IsVisible() && app._page->GetBounds().right - app._page->GetBounds().left >= 700.f,
                    "800x560 minimum viewport keeps 744 DIP writing surface between 28-DIP rails");

            app.Execute(Command::Focus);
            Require(app._settings.focusMode && ! app._leftRail->IsVisible() && ! app._rightRail->IsVisible() && ! app._filesPane->IsVisible() &&
                        ! app._sidebar->IsVisible(),
                    "Focus mode hides both panes, collapsed rails, and hover strips");
            app.Execute(Command::Focus);
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "collapse priority test remains hidden");
        });

        test("Window.SidePaneAnimationZeroPollingAndReducedMotion",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-animation-tick"));
            Success(app.Initialize(), "initialize hidden animation app");
            auto theme          = app.Host().GetTheme();
            theme.reducedMotion = false;
            app.Host().SetTheme(theme);

            Resize(app, 1440, 900);
            Require(app._sidebar->IsVisible() && ! app._rightRail->IsVisible(), "start docked at 1440x900");
            const float dockedEditorLeft = app._editor->GetBounds().left;
            (void)dockedEditorLeft;

            Resize(app, 1080, 820, 0, false);
            Require(app._rightRail->IsVisible() && app._rightRail->State().phase == PaneAnimationPhase::Collapsing && app._sidebar->IsVisible(),
                    "narrowing from Docked enters FastDecelerate Collapsing animation phase");

            std::uint64_t nowMs = 1000u;
            Require(app._rightRail->Tick(app.Host(), nowMs), "initial animation tick anchors clock cleanly");
            nowMs += 100u;
            Require(app._rightRail->Tick(app.Host(), nowMs) && app._rightRail->State().progress > 0.f && app._rightRail->State().progress < 1.f,
                    "mid-collapse tick advances eased progress strictly inside (0, 1)");
            nowMs += 100u;
            Require(! app._rightRail->Tick(app.Host(), nowMs) && app._rightRail->State().phase == PaneAnimationPhase::Settled &&
                        app._rightRail->State().mode == PaneDisplayMode::CollapsedRail && Near(app._rightRail->State().progress, 0.f) &&
                        ! app._sidebar->IsVisible(),
                    "collapse animation settles in <= 200ms with zero trailing polls");

            const float railCanvasWidth = app._editor->GetBounds().right - app._editor->GetBounds().left;
            const auto railRect         = app._rightRail->GetBounds();
            app._rightRail->HandlePointerMoveDip(app.Host(), {railRect.right - 2.f, railRect.top + 8.f}, true);
            Require(app._rightRail->State().phase == PaneAnimationPhase::HoverDwell, "edge trigger hover arms 180ms HoverDwell timer");

            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 80u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().mode == PaneDisplayMode::HoverOverlay && app._sidebar->IsVisible(),
                    "180ms dwell transitions into HoverOverlay expansion");

            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled && Near(app._rightRail->State().progress, 1.f) && app._sidebar->IsVisible() &&
                        Near(app._editor->GetBounds().right - app._editor->GetBounds().left, railCanvasWidth),
                    "HoverOverlay opens to full width in <= 200ms with zero canvas text reflow");

            app._rightRail->HandlePointerMoveDip(app.Host(), {100.f, 200.f}, false);
            Require(app._rightRail->State().phase == PaneAnimationPhase::LeaveGrace, "pointer exit arms 300ms LeaveGrace");
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().mode == PaneDisplayMode::HoverOverlay && app._sidebar->IsVisible(),
                    "overlay stays open inside 300ms leave grace window");
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 80u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().mode == PaneDisplayMode::CollapsedRail && ! app._sidebar->IsVisible() &&
                        app._rightRail->State().phase == PaneAnimationPhase::Settled,
                    "post-grace close finishes cleanly into Settled CollapsedRail");

            theme.reducedMotion = true;
            app.Host().SetTheme(theme);
            app.OnRailExpandClicked(PaneSide::Right);
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled && app._rightRail->State().mode == PaneDisplayMode::HoverOverlay &&
                        Near(app._rightRail->State().progress, 1.f),
                    "reducedMotion opens HoverOverlay instantaneously without intermediate animation frames");
            app.DismissHoverOverlays(true);
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled && app._rightRail->State().mode == PaneDisplayMode::CollapsedRail &&
                        Near(app._rightRail->State().progress, 0.f),
                    "reducedMotion dismisses HoverOverlay in 0ms");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "animation test never activates window");
        });

        test("Window.SidePaneDotHoverNonInterferenceAndDismissal",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-dot-noninterference"));
            Success(app.Initialize(), "initialize hidden dot-hover app");
            auto theme          = app.Host().GetTheme();
            theme.reducedMotion = false;
            app.Host().SetTheme(theme);

            app.SetDocument(L"# Intro\n\nBody.\n\n## Second Heading\n\nDetails.\n\n### Third Heading\n\nEnd.\n");
            Resize(app, 1080, 820);
            std::uint64_t settleMs = 1000u;
            while (app._rightRail->Tick(app.Host(), settleMs))
                settleMs += 100u;
            Require(app._rightRail->IsVisible() && app._rightRail->VisibleDotCount() == 3u, "right rail displays three hierarchical heading dots");

            auto* const dot0 = app._rightRail->DotButton(0);
            Require(dot0 != nullptr && ! dot0->GetTooltipText().empty() && ! dot0->GetAccessibleName().empty(),
                    "heading dot exposes localized tooltip and accessible name");
            const auto dBounds = dot0->GetBounds();
            (void)app.HandleMessage(
                app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(dBounds.left + dBounds.right) * 0.5f, (dBounds.top + dBounds.bottom) * 0.5f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled && app._rightRail->State().mode == PaneDisplayMode::CollapsedRail,
                    "hovering directly over a rail dot suppresses HoverDwell drawer reveal");

            const auto railRect = app._rightRail->GetBounds();
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {railRect.right - 2.f, railRect.top + 8.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::HoverDwell, "moving pointer onto outer 6-DIP trigger edge arms HoverDwell");

            app.OnRailExpandClicked(PaneSide::Right);
            Require(app.IsPaneOverlayActive(PaneSide::Right) && app._sidebar->IsVisible(), "overlay is open before Escape dismissal");
            Require(app.DismissSurface() && ! app.IsPaneOverlayActive(PaneSide::Right) && app._rightRail->IsVisible(),
                    "Escape dismisses HoverOverlay cleanly without disabling showOutline");

            app.OnRailExpandClicked(PaneSide::Right);
            Require(app.IsPaneOverlayActive(PaneSide::Right), "re-open overlay before outside click dismissal");
            (void)app.HandleMessage(app.Window(), WM_LBUTTONDOWN, MK_LBUTTON, DipToClientLParam(app, {250.f, 250.f}));
            Require(! app.IsPaneOverlayActive(PaneSide::Right), "clicking outside floating overlay dismisses it cleanly");

            app.OnRailExpandClicked(PaneSide::Right);
            app.Execute(Command::Find);
            Require(! app.IsPaneOverlayActive(PaneSide::Right) && app._findBar->IsVisible(), "opening FindBar dismisses active pane HoverOverlay");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "dot-hover test stays hidden");
        });

        test("Outline.CollapsedRightRailDotsJumpFindRingsAndOverflow",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-right-dots"));
            Success(app.Initialize(), "initialize hidden right-rail dots app");
            std::wstring longHeading(95u, L'A');
            app.SetDocument(L"# Overview\n\nAlpha line.\n\n## " + longHeading +
                            L"\n\nTarget needle one. Target needle two.\n\n### Deep Section\n\nFinal text.\n");
            Resize(app, 1080, 820);
            Require(app._rightRail->ItemCount() == 3u && app._rightRail->DotButton(0)->DotItem().level == 1u &&
                        app._rightRail->DotButton(1)->DotItem().level == 2u && app._rightRail->DotButton(2)->DotItem().level == 3u,
                    "right rail encodes heading levels H1/H2/H3 cleanly");
            Require(app._rightRail->DotButton(1)->DotItem().name.size() <= 80u, "oversized heading tooltip name truncates cleanly within 80 chars");

            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"Target");
            app.RefreshFind();
            Require(app._rightRail->DotButton(0)->DotItem().findMatchCount == 2u && app._rightRail->DotButton(1)->DotItem().findMatchCount == 2u &&
                        app._rightRail->DotButton(2)->DotItem().findMatchCount == 0u,
                    "right rail mirrors Outline section match rollup and keeps non-matching sections ring-free");

            app._rightRail->onDotInvoked(2u);
            app.Prepare(64);
            const auto expectedOffset = app._documentInfo.headings[2].offset;
            Require(app.Document().GetSelection().caret == expectedOffset && ! app._sidebar->IsVisible() && app._rightRail->IsVisible(),
                    "clicking right rail dot jumps caret to heading while keeping panel collapsed");

            std::wstring many;
            for (unsigned i = 1; i <= 40; ++i)
                many += std::format(L"# Heading {}\n\nParagraph {}.\n\n", i, i);
            app.SetDocument(many);
            Resize(app, 800, 560);
            Require(app._rightRail->ItemCount() == 40u && app._rightRail->OverflowCount() > 0u, "40 headings at 560 DIP height activates overflow badge count");
            const auto beforeScroll = app._rightRail->ScrollOffset();
            (void)app._rightRail->OnMouseWheel(app.Host(), {0.f, 0.f}, -static_cast<float>(WHEEL_DELTA), 0u);
            Require(app._rightRail->ScrollOffset() > beforeScroll, "mouse wheel scrolls collapsed rail dots when overflowed");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "right-rail test stays hidden");
        });

        test("Editor.FilesCollapsedLeftRailDirtyDotsSwitchAndGuards",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto dir   = root / L"panes-left-dots";
            const auto pathA = dir / L"FolderA/Notes.md";
            const auto pathB = dir / L"FolderB/Notes.md";
            Require(WriteText(pathA, "# Note A\n\nBody A.\n") && WriteText(pathB, "# Note B\n\nBody B.\n"), "seed duplicate basename fixtures");

            unsigned prompts{};
            auto opts                 = Options(root, L"panes-left-dots-data");
            opts.unsavedPromptForTest = [&]
            {
                ++prompts;
                return IDCANCEL;
            };
            Application app(std::move(opts));
            Success(app.Initialize(), "initialize hidden left-rail dots app");
            app.Execute(Command::Files);
            Resize(app, 1080, 820);
            Require(app._leftRail->IsVisible() && app._leftRail->ItemCount() == 0u, "clean workspace shows zero left-rail dots");

            StoredDocument docA, docB;
            Success(LoadDocument(pathA, docA), "load Note A");
            Success(LoadDocument(pathB, docB), "load Note B");
            Require(app.SetDocument(docA.text, pathA, docA.bom, docA.stamp), "open Note A");
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Edited A "), "dirty Note A");
            Require(app.StartWorkspaceDocument(), "open second workspace slot");
            Require(app.SetDocument(docB.text, pathB, docB.bom, docB.stamp), "open Note B");
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Edited B "), "dirty Note B");
            app.Prepare(64);

            Require(app._leftRail->IsVisible() && app._leftRail->ItemCount() == 2u, "two edited workspace documents populate two left-rail dots");
            const auto& tip0 = app._leftRail->DotButton(0)->DotItem().tooltip;
            const auto& tip1 = app._leftRail->DotButton(1)->DotItem().tooltip;
            Require(tip0.find(L"FolderA/Notes.md") != std::wstring::npos && tip1.find(L"FolderB/Notes.md") != std::wstring::npos &&
                        tip0.find(Resource(IDS_FILESEDITED)) != std::wstring::npos,
                    "duplicate basenames disambiguate with parent folder and Edited status in left dot tooltip");

            const auto inactiveKey = app._leftRail->DotButton(0)->DotItem().id;
            app._leftRail->onDotInvoked(inactiveKey);
            app.Prepare(64);
            Require(prompts == 0u && app._session.key == inactiveKey && app._session.path.lexically_normal() == pathA.lexically_normal() &&
                        app._leftRail->IsVisible() && ! app._filesPane->IsVisible() && app._leftRail->DotButton(0)->DotItem().active &&
                        ! app._leftRail->DotButton(1)->DotItem().active,
                    "left dot switches retained dirty workspace document with zero save prompt while keeping rail collapsed");

            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft(), "open local source block draft before guard test");
            app._leftRail->onDotInvoked(app._leftRail->DotButton(1)->DotItem().id);
            Require(app._session.path.lexically_normal() == pathA.lexically_normal() && app.HasBlockDraft(),
                    "left rail dot click safely refuses document switch while block draft is active");
            app.Execute(Command::CloseDrawer);
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "left-rail test stays hidden");
        });
    }
};

void RegisterPanesTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    PanesProbe::RegisterTests(root, test);
}
} // namespace RedTilde
