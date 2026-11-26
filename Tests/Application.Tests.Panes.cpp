#include "Application.Tests.Panes.h"
#include "RedTilde/Application.h"
#include "RedTilde/KeyboardState.h"
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
bool Inside(D2D1_RECT_F inner, D2D1_RECT_F outer) noexcept
{
    return inner.left >= outer.left - 0.1f && inner.top >= outer.top - 0.1f && inner.right <= outer.right + 0.1f && inner.bottom <= outer.bottom + 0.1f &&
           inner.right > inner.left && inner.bottom > inner.top;
}
bool Overlap(D2D1_RECT_F left, D2D1_RECT_F right) noexcept
{
    return std::min(left.right, right.right) > std::max(left.left, right.left) + 0.1f &&
           std::min(left.bottom, right.bottom) > std::max(left.top, right.top) + 0.1f;
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
            Require(app._settings.showOutline == ! initialOutline && app._panelToggle->GetToggleState() == ! initialOutline &&
                        app._rightRail->IsVisible() == initialOutline,
                    "Command::Outline updates showOutline, panel toggle state, and compact right rail visibility");
            app.Execute(Command::Outline);
            Require(app._settings.showOutline == initialOutline && app._panelToggle->GetToggleState() == initialOutline && ! app._rightRail->IsVisible(),
                    "second Command::Outline re-pins Outline open and hides compact rail");

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
            app._settings.filesPinMode   = PanePinMode::Auto;
            app._settings.outlinePinMode = PanePinMode::Auto;
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
            app.RevealPaneOverlay(PaneSide::Right);
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
            auto* const dot2 = app._rightRail->DotButton(2);
            Require(dot0 != nullptr && dot2 != nullptr && ! dot0->GetTooltipText().empty() && ! dot0->GetAccessibleName().empty(),
                    "heading dot exposes localized tooltip and accessible name");
            const auto dBounds  = dot0->GetBounds();
            const auto d2Bounds = dot2->GetBounds();
            const auto railRect = app._rightRail->GetBounds();
            const auto expRect  = app._rightRail->ExpandButton()->GetBounds();
            const float bandMid = (expRect.bottom + 8.f + railRect.bottom - 12.f) * 0.5f;
            const float dotsMid = (dBounds.top + d2Bounds.bottom) * 0.5f;
            Require(std::abs(dotsMid - bandMid) <= 2.f, "minimized right-rail dots are vertically centered inside the rail band");
            (void)app.HandleMessage(
                app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(dBounds.left + dBounds.right) * 0.5f, (dBounds.top + dBounds.bottom) * 0.5f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled && app._rightRail->State().mode == PaneDisplayMode::CollapsedRail,
                    "hovering directly over a rail dot suppresses HoverDwell drawer reveal");

            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {railRect.right - 2.f, railRect.top + 8.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::HoverDwell, "moving pointer onto outer 6-DIP trigger edge arms HoverDwell");

            app.RevealPaneOverlay(PaneSide::Right);
            Require(app.IsPaneOverlayActive(PaneSide::Right) && app._sidebar->IsVisible(), "overlay is open before Escape dismissal");
            Require(app.DismissSurface() && ! app.IsPaneOverlayActive(PaneSide::Right) && app._rightRail->IsVisible(),
                    "Escape dismisses HoverOverlay cleanly back to compact right rail");

            (void)app.HandleMessage(app.Window(), WM_LBUTTONDOWN, MK_LBUTTON, DipToClientLParam(app, {railRect.right - 10.f, railRect.bottom - 12.f}));
            Require(app.IsPaneOverlayActive(PaneSide::Right), "clicking a free zone on compact rail pops HoverOverlay open");
            (void)app.HandleMessage(app.Window(), WM_LBUTTONDOWN, MK_LBUTTON, DipToClientLParam(app, {250.f, 250.f}));
            Require(! app.IsPaneOverlayActive(PaneSide::Right), "clicking outside floating overlay dismisses it cleanly");

            app.RevealPaneOverlay(PaneSide::Right);
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
            app._settings.filesPinMode   = PanePinMode::Auto;
            app._settings.outlinePinMode = PanePinMode::Auto;
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

        test("Window.SidePaneEdgeDrawersAndPinOpenBehavior",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-drawers-pin"));
            Success(app.Initialize(), "initialize hidden drawers-and-pin app");
            auto theme          = app.Host().GetTheme();
            theme.reducedMotion = true;
            app.Host().SetTheme(theme);

            app.SetDocument(L"# Intro\n\nAlpha.\n\n## Second\n\nBeta.\n");
            app.Execute(Command::Files);
            app._settings.filesWidth = 200u;
            Resize(app, 1440, 900);

            Require(app._filesPane->kind == Surface::Kind::Sidebar && app._sidebar->kind == Surface::Kind::Sidebar &&
                        app._drawer->kind == Surface::Kind::DrawerRight,
                    "docked side panes use Sidebar while Settings drawer uses sharp flush DrawerRight");
            app.Execute(Command::Settings);
            const auto drawerRect = app._drawer->GetBounds();
            Require(app._drawer->IsVisible() && Near(drawerRect.right, 1440.f) && Near(drawerRect.top, 104.f) && Near(drawerRect.bottom, 872.f),
                    "Settings drawer attaches flush to right client edge, command bar, and status bar");
            app.Execute(Command::CloseDrawer);

            Require(app._pinFiles->IsVisible() && app._pinOutline->IsVisible(), "both docked side panes expose a single Pin header button");
            Require(Inside(app._filesSectionTitle->GetBounds(), app._filesPane->GetBounds()) &&
                        Inside(app._pinFiles->GetBounds(), app._filesPane->GetBounds()) &&
                        ! Overlap(app._filesSectionTitle->GetBounds(), app._pinFiles->GetBounds()) &&
                        Inside(app._outlineTitle->GetBounds(), app._sidebar->GetBounds()) &&
                        Inside(app._outlineCount->GetBounds(), app._sidebar->GetBounds()) && Inside(app._pinOutline->GetBounds(), app._sidebar->GetBounds()) &&
                        ! Overlap(app._outlineTitle->GetBounds(), app._outlineCount->GetBounds()) &&
                        ! Overlap(app._outlineCount->GetBounds(), app._pinOutline->GetBounds()),
                    "Pin header controls fit cleanly without overlap even at minimum 200-DIP width");
            Require(app._pinFiles->GetToggleState() == std::optional<bool>{true} && app._pinFiles->GetTooltipText() == Resource(IDS_UNPIN_FILES) &&
                        app._pinOutline->GetToggleState() == std::optional<bool>{true} && app._pinOutline->GetTooltipText() == Resource(IDS_UNPIN_OUTLINE) &&
                        app._feedback->GetText().empty(),
                    "docked panes reflect pinned toggle state, localized Unpin labels, and no lingering bottom-left Saved status");

            app._settings.filesWidth     = 280u;
            app._settings.filesPinMode   = PanePinMode::Auto;
            app._settings.outlinePinMode = PanePinMode::Auto;
            Resize(app, 1080, 820);
            app.RevealPaneOverlay(PaneSide::Right);
            const auto rightOverlayRect = app._sidebar->GetBounds();
            Require(app._sidebar->kind == Surface::Kind::DrawerRight && Near(rightOverlayRect.right, 1080.f) && Near(rightOverlayRect.top, 104.f) &&
                        Near(rightOverlayRect.bottom, 792.f) && app._pinOutline->GetToggleState() == std::optional<bool>{false} &&
                        app._pinOutline->GetTooltipText() == Resource(IDS_PIN_OUTLINE),
                    "collapsed Right overlay uses flush DrawerRight surface and shows unpinned Pin Outline button");
            app.RevealPaneOverlay(PaneSide::Left);
            const auto leftOverlayRect = app._filesPane->GetBounds();
            Require(app._filesPane->kind == Surface::Kind::DrawerLeft && Near(leftOverlayRect.left, 0.f) && Near(leftOverlayRect.top, 104.f) &&
                        Near(leftOverlayRect.bottom, 792.f) && app._pinFiles->GetToggleState() == std::optional<bool>{false} &&
                        app._pinFiles->GetTooltipText() == Resource(IDS_PIN_FILES),
                    "collapsed Left overlay uses flush DrawerLeft surface and shows unpinned Pin Files button");

            app.Execute(Command::PinOutline);
            Require(app._settings.outlinePinMode == PanePinMode::Pinned && app._sidebar->IsVisible() && ! app._rightRail->IsVisible() &&
                        app._sidebar->kind == Surface::Kind::Sidebar && app._pinOutline->GetToggleState() == std::optional<bool>{true} &&
                        Near(app._page->GetBounds().right, 1080.f - 240.f),
                    "PinOutline docks Outline cleanly at 1080x820 and clears rail");

            Resize(app, 1440, 900);
            app.SetView(EditorView::Split);
            app.Layout();
            app.Prepare(64);
            Require(app._sidebar->IsVisible() && ! app._rightRail->IsVisible() && Near(app._page->GetBounds().right, 1440.f - 240.f),
                    "pinned Outline remains docked in Split mode when minimum canvas width fits");

            app.SetView(EditorView::Compose);
            app.Execute(Command::PinOutline);
            Require(app._settings.outlinePinMode == PanePinMode::Unpinned && ! app._settings.showOutline && ! app._sidebar->IsVisible() &&
                        app._rightRail->IsVisible() && app._filesPane->IsVisible(),
                    "Unpinning Outline collapses it to compact right rail at 1440x900 while leaving Files docked");
            app.Execute(Command::Outline);
            Require(app._settings.showOutline && app._settings.outlinePinMode == PanePinMode::Pinned && app._sidebar->IsVisible() &&
                        ! app._rightRail->IsVisible(),
                    "Outline command from compact rail pins Outline open again at 1440x900");
            app.Execute(Command::Outline);
            Require(! app._settings.showOutline && app._settings.outlinePinMode == PanePinMode::Unpinned && ! app._sidebar->IsVisible() &&
                        app._rightRail->IsVisible(),
                    "second Outline command minimizes Outline back to compact rail");
            const auto railExp = app._rightRail->ExpandButton()->GetBounds();
            Require(app._rightRail->ExpandButton()->IsVisible() && Inside(railExp, app._rightRail->GetBounds()),
                    "compact rail Pin button sits inside the collapsed rail");
            Require(app._rightRail->ExpandButton()->Invoke(app.Host(), false), "invoking the compact rail Pin button fires its callback");
            Require(app._settings.showOutline && app._settings.outlinePinMode == PanePinMode::Pinned && app._sidebar->IsVisible() &&
                        ! app._rightRail->IsVisible(),
                    "clicking the compact rail button pins Outline open");

            app.Execute(Command::PinFiles);
            Require(app._settings.filesPinMode == PanePinMode::Unpinned && ! app._filesPane->IsVisible() && app._leftRail->IsVisible() &&
                        app._sidebar->IsVisible() && ! app._rightRail->IsVisible(),
                    "unpinning docked Files collapses it to left rail without falsely collapsing Auto Outline");

            app._settings.filesPinMode   = PanePinMode::Auto;
            app._settings.outlinePinMode = PanePinMode::Auto;
            Resize(app, 800, 560);
            app.Execute(Command::PinFiles);
            Require(app._filesPane->IsVisible() && ! app._leftRail->IsVisible() && app._rightRail->IsVisible() &&
                        app._page->GetBounds().right - app._page->GetBounds().left >= 320.f,
                    "pinning Left at 800x560 docks Files while keeping >= 320 DIP canvas");
            app.Execute(Command::PinOutline);
            Require(app._sidebar->IsVisible() && ! app._rightRail->IsVisible() && app._leftRail->IsVisible() && ! app._filesPane->IsVisible() &&
                        app._page->GetBounds().right - app._page->GetBounds().left >= 320.f,
                    "pinning Right when both pinned panes cannot fit yields to last-pinned Right and keeps >= 320 DIP canvas");
            Resize(app, 960, 560);
            Require(app._filesPane->IsVisible() && app._sidebar->IsVisible() && ! app._leftRail->IsVisible() && ! app._rightRail->IsVisible(),
                    "widening to 960x560 docks both pinned panes automatically");

            const auto settingsDir = root / L"panes-pin-settings-io";
            Settings toSave;
            toSave.filesPinMode   = PanePinMode::Pinned;
            toSave.outlinePinMode = PanePinMode::Unpinned;
            Success(SaveSettings(settingsDir, toSave), "persist pin modes");
            const auto loaded = LoadSettingsCold(settingsDir);
            Require(loaded.status == SettingsLoadStatus::Loaded && loaded.settings.filesPinMode == PanePinMode::Pinned &&
                        loaded.settings.outlinePinMode == PanePinMode::Unpinned,
                    "cold load restores filesPinMode=Pinned and outlinePinMode=Unpinned");

            const auto file = settingsDir / SettingsFileName();
            Require(SUCCEEDED(WriteBytesAtomic(file, "{\n  schema: 1,\n  filesPinMode: \"bogus\",\n  outlinePinMode: 42\n}\n")),
                    "write unusable pin mode values fixture");
            const auto fallback = LoadSettingsCold(settingsDir);
            Require(fallback.status == SettingsLoadStatus::Loaded && fallback.settings.filesPinMode == PanePinMode::Auto &&
                        fallback.settings.outlinePinMode == PanePinMode::Auto,
                    "invalid pin mode strings/types fall back cleanly to Auto");

            const auto liveFile        = (root / L"panes-drawers-pin") / SettingsFileName();
            const std::string liveJson = "{\n  schema: 1,\n  showOutline: true,\n  filesPinMode: \"unpinned\",\n  outlinePinMode: \"unpinned\"\n}\n";
            Require(SUCCEEDED(WriteBytesAtomic(liveFile, liveJson)), "write live pin mode settings update");
            app.ReloadSettingsFromDisk();
            Require(app._settings.filesPinMode == PanePinMode::Unpinned && app._settings.outlinePinMode == PanePinMode::Unpinned &&
                        app._leftRail->IsVisible() && app._rightRail->IsVisible(),
                    "live settings reload applies updated pin modes cleanly without restart");

            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "pin-and-drawer test stays hidden");
        });

        test("Window.SidePaneHoverPolishReversalAndFocusHold",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"panes-hover-polish"));
            Success(app.Initialize(), "initialize hidden hover-polish app");
            auto theme          = app.Host().GetTheme();
            theme.reducedMotion = false;
            app.Host().SetTheme(theme);

            app.SetDocument(L"# Intro\n\nAlpha.\n\n## Second\n\nBeta.\n\n### Third\n\nGamma.\n");
            app.Execute(Command::Files);
            app._settings.filesPinMode   = PanePinMode::Auto;
            app._settings.outlinePinMode = PanePinMode::Auto;
            Resize(app, 1080, 820);
            std::uint64_t nowMs = 1000u;
            while (app._leftRail->Tick(app.Host(), nowMs) || app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;

            const auto rightRailBounds = app._rightRail->GetBounds();
            const auto expandBounds    = app._rightRail->ExpandButton()->GetBounds();
            (void)app.HandleMessage(app.Window(),
                                    WM_MOUSEMOVE,
                                    0,
                                    DipToClientLParam(app, {(expandBounds.left + expandBounds.right) * 0.5f, (expandBounds.top + expandBounds.bottom) * 0.5f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled, "hovering expand chevron itself does not arm hover dwell or race clicks");

            const auto d0 = app._rightRail->DotButton(0)->GetBounds();
            const auto d1 = app._rightRail->DotButton(1)->GetBounds();
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(d0.left + d0.right) * 0.5f, (d0.bottom + d1.top) * 0.5f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled, "moving vertically across 2-DIP gaps between dots does not arm hover dwell");

            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {rightRailBounds.left + 3.f, rightRailBounds.bottom - 30.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled, "brushing inner scrollbar-adjacent strip of rail does not arm hover dwell");

            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {rightRailBounds.right - 12.f, rightRailBounds.bottom - 30.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::HoverDwell, "hovering empty outer region below dots on right rail arms hover dwell");
            app.Execute(Command::Settings);
            Require(app._drawer->IsVisible() && app._rightRail->State().phase == PaneAnimationPhase::Settled,
                    "opening Settings drawer cancels pending rail HoverDwell");
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {rightRailBounds.right - 2.f, rightRailBounds.top + 8.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Settled, "collapsed rail suppresses HoverDwell while Settings drawer is active");
            app.Execute(Command::CloseDrawer);

            (void)app.HandleMessage(
                app.Window(), WM_LBUTTONDOWN, MK_LBUTTON, DipToClientLParam(app, {rightRailBounds.right - 12.f, rightRailBounds.bottom - 30.f}));
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            const auto sidebarBounds = app._sidebar->GetBounds();
            (void)app.HandleMessage(
                app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {(sidebarBounds.left + sidebarBounds.right) * 0.5f, sidebarBounds.top + 80.f}));
            Require(app.IsPaneOverlayActive(PaneSide::Right) && app._rightRail->State().phase == PaneAnimationPhase::Settled,
                    "free-zone click opens HoverOverlay and stays open while pointer is inside pane");
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {250.f, 250.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::LeaveGrace, "moving pointer out of free-zone clicked overlay arms LeaveGrace");
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(! app.IsPaneOverlayActive(PaneSide::Right) && app._rightRail->State().mode == PaneDisplayMode::CollapsedRail,
                    "free-zone clicked overlay closes cleanly on mouse leave");

            (void)app.HandleMessage(
                app.Window(), WM_LBUTTONDOWN, MK_LBUTTON, DipToClientLParam(app, {rightRailBounds.right - 12.f, rightRailBounds.bottom - 30.f}));
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(app.DismissSurface() && ! app.IsPaneOverlayActive(PaneSide::Right), "Escape dismisses HoverOverlay cleanly");
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;

            // Hover-reveal, enter LeaveGrace -> Collapsing, then re-enter pointer to verify smooth reversal.
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {rightRailBounds.right - 2.f, rightRailBounds.top + 8.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::HoverDwell, "edge hover arms dwell");
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 80u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(app._rightRail->State().mode == PaneDisplayMode::HoverOverlay && Near(app._rightRail->State().progress, 1.f),
                    "hover-revealed Right overlay settles open");

            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {250.f, 250.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::LeaveGrace, "pointer leave starts 300ms grace");
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().phase == PaneAnimationPhase::Collapsing && app._rightRail->State().collapseFromOverlay,
                    "grace expiry begins Collapsing from overlay");
            (void)app._rightRail->Tick(app.Host(), nowMs);
            nowMs += 80u;
            (void)app._rightRail->Tick(app.Host(), nowMs);
            Require(app._rightRail->State().phase == PaneAnimationPhase::Collapsing && app._rightRail->State().progress < 1.f &&
                        app._rightRail->State().progress > 0.f,
                    "mid-collapse tick leaves overlay partially open before re-entry");

            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {rightRailBounds.right - 2.f, rightRailBounds.top + 8.f}));
            Require(app._rightRail->State().phase == PaneAnimationPhase::Expanding && app._rightRail->State().mode == PaneDisplayMode::HoverOverlay,
                    "moving pointer back onto closing overlay or rail reverses animation smoothly back to Expanding");
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(app._rightRail->State().mode == PaneDisplayMode::HoverOverlay && app._sidebar->IsVisible() && Near(app._rightRail->State().progress, 1.f),
                    "reversed hover overlay finishes open at full progress");

            // Clicking an Outline item inside unpinned overlay jumps and dismisses the overlay.
            app._outlineModel.OnTreeItemInvoked(2u);
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(app.Document().GetSelection().caret == app._documentInfo.headings[1].offset && ! app._sidebar->IsVisible() &&
                        ! app.IsPaneOverlayActive(PaneSide::Right),
                    "clicking an Outline heading inside an unpinned overlay navigates and dismisses the overlay");

            app.Execute(Command::PinOutline);
            app._outlineModel.OnTreeItemInvoked(3u);
            Require(app.Document().GetSelection().caret == app._documentInfo.headings[2].offset && app._sidebar->IsVisible(),
                    "clicking an Outline heading when Outline is pinned keeps docked Outline open");
            app.Execute(Command::PinOutline);
            while (app._rightRail->Tick(app.Host(), nowMs))
                nowMs += 100u;

            // Keyboard focus inside Files Filter holds Left overlay open even after pointer exits.
            const auto leftRailBounds = app._leftRail->GetBounds();
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {leftRailBounds.left + 2.f, leftRailBounds.top + 8.f}));
            (void)app._leftRail->Tick(app.Host(), nowMs);
            nowMs += 100u;
            (void)app._leftRail->Tick(app.Host(), nowMs);
            nowMs += 80u;
            (void)app._leftRail->Tick(app.Host(), nowMs);
            while (app._leftRail->Tick(app.Host(), nowMs))
                nowMs += 100u;
            Require(app.IsPaneOverlayActive(PaneSide::Left) && app._filesPane->IsVisible(), "Left overlay hover-opened before focus hold check");

            app.Host().SetFocusControl(app._filesFilter, false);
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {500.f, 300.f}));
            Require(app._leftRail->State().phase == PaneAnimationPhase::Settled && app.IsPaneOverlayActive(PaneSide::Left),
                    "keyboard focus in Files filter holds hover overlay open when pointer leaves");

            app.Host().SetFocusControl(app.ActiveEditor(), false);
            (void)app.HandleMessage(app.Window(), WM_MOUSEMOVE, 0, DipToClientLParam(app, {500.f, 300.f}));
            Require(app._leftRail->State().phase == PaneAnimationPhase::LeaveGrace, "moving focus back to editor lets pointer exit resume LeaveGrace");
            app.DismissHoverOverlays(false);

            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "hover-polish test stays hidden");
        });

        test("Window.M7ModalCenteringCompactHeaderAndInputContainment",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            auto options      = Options(root, L"m7-modal-centering-input");
            const auto folder = options.dataDirectory / L"workspace";
            std::filesystem::create_directories(folder);
            const auto docPath = folder / L"note.md";
            Require(WriteText(docPath, "# Local Note\r\nBody text.\r\n"), "seed local markdown doc for hover invocation test");

            Application app(options);
            Success(app.Initialize(), "initialize hidden M7 modal/input containment fixture");

            // P1-01: Floating status/pills precede modal backdrop and all modal panels in Z-order.
            Require(app._root != nullptr, "root surface is attached");
            const auto indexOf = [&](const DxUi::Control* needle) -> std::size_t
            {
                const std::size_t count = app._root->GetLogicalChildCount();
                for (std::size_t i = 0; i < count; ++i)
                    if (app._root->GetLogicalChild(i) == needle)
                        return i;
                Require(false, "expected child control in root surface");
                return count;
            };
            const std::size_t backdropIdx = indexOf(app._modalBackdrop);
            for (const DxUi::Control* nonModal : {static_cast<const DxUi::Control*>(app._focusPill),
                                                  static_cast<const DxUi::Control*>(app._positionStatus),
                                                  static_cast<const DxUi::Control*>(app._spellStatusCombo),
                                                  static_cast<const DxUi::Control*>(app._feedback),
                                                  static_cast<const DxUi::Control*>(app._floating)})
                Require(indexOf(nonModal) < backdropIdx, "floating status and selection chrome appear below ModalBackdrop in Z-order");
            for (const DxUi::Control* modal : {static_cast<const DxUi::Control*>(app._recoveryPanel),
                                               static_cast<const DxUi::Control*>(app._saveConfirmPanel),
                                               static_cast<const DxUi::Control*>(app._renamePanel),
                                               static_cast<const DxUi::Control*>(app._fileConflictPanel),
                                               static_cast<const DxUi::Control*>(app._openGitHubPanel)})
                Require(indexOf(modal) > backdropIdx, "every modal panel sits above ModalBackdrop in Z-order");

            // P1-03: Files pane < 272 DIP switches Add Folder and Add GitHub buttons to IconOnly cleanly.
            app._settings.filesWidth = 240u;
            app._settings.showFiles  = true;
            Resize(app, 960, 640, 96);
            Require(app._compactFilesHeader && app._filesAddFolder->GetVariant() == DxUi::ButtonVariant::IconOnly &&
                        app._filesAddGitHub->GetVariant() == DxUi::ButtonVariant::IconOnly,
                    "files pane narrower than 272 DIP switches Add Folder and GitHub buttons to compact IconOnly mode");
            Require(! Overlap(app._filesSectionTitle->GetBounds(), app._filesAddFolder->GetBounds()) &&
                        ! Overlap(app._filesAddFolder->GetBounds(), app._filesAddGitHub->GetBounds()) &&
                        ! Overlap(app._filesAddGitHub->GetBounds(), app._pinFiles->GetBounds()),
                    "compact files pane header controls never overlap");
            app._settings.filesWidth = 300u;
            app.Layout();
            Require(! app._compactFilesHeader && app._filesAddFolder->GetVariant() == DxUi::ButtonVariant::Standard &&
                        app._filesAddGitHub->GetVariant() == DxUi::ButtonVariant::Standard && ! app._filesAddFolder->GetText().empty(),
                    "widening files pane to >= 272 DIP restores Standard button labels");

            // P1-03: All 5 modals stay centered and unclipped at 800x560 DIP across 100%, 150%, and 200% DPI.
            for (const UINT dpi : {96u, 144u, 192u})
            {
                Resize(app, 800, 560, dpi);
                const auto client           = app._host.GetClientBoundsDip();
                const D2D1_RECT_F safeFrame = D2D1::RectF(12.f, 12.f, client.right - 12.f, client.bottom - 12.f);

                app._recoveryCandidates.clear();
                Application::OfferedRecoveryCandidate rc;
                rc.id            = L"m7-layout-probe";
                rc.snapshot.text = L"# Probe\nBody";
                app._recoveryCandidates.push_back(rc);
                app.ShowRecovery();
                app.Layout();
                Require(Inside(app._recoveryPanel->GetBounds(), safeFrame) && Inside(app._recoveryRestoreBtn->GetBounds(), app._recoveryPanel->GetBounds()) &&
                            Inside(app._recoveryLaterBtn->GetBounds(), app._recoveryPanel->GetBounds()),
                        "RecoveryModal fits inside 800x560 DIP client scrim at high DPI");
                app.CloseDrawers();

                app.Document().SetText(L"Dirty before SaveConfirm layout check\n");
                app.ShowSaveConfirmModal([] {}, true);
                app.Layout();
                Require(app._saveConfirmPanel->IsVisible() && Inside(app._saveConfirmPanel->GetBounds(), safeFrame) &&
                            Inside(app._saveConfirmScroll->GetBounds(), app._saveConfirmPanel->GetBounds()),
                        "SaveConfirmModal fits cleanly inside 800x560 DIP scrim");
                if (! app._saveConfirmRowControls.empty() && app._saveConfirmRowControls[0].checkbox->IsVisible())
                    Require(app._saveConfirmRowControls[0].checkbox->GetBounds().right <= app._saveConfirmPanel->GetBounds().right - 34.f,
                            "SaveConfirm row width clears scrollbar area");
                app.CloseDrawers();
                app.Document().SetText(L"");

                app.SetActiveOverlay(Application::OverlayKind::RenameFileModal);
                app.Layout();
                Require(Inside(app._renamePanel->GetBounds(), safeFrame), "RenameFileModal fits inside 800x560 DIP scrim");
                app.CloseDrawers();

                app.SetActiveOverlay(Application::OverlayKind::FileConflictModal);
                app.Layout();
                Require(Inside(app._fileConflictPanel->GetBounds(), safeFrame), "FileConflictModal fits inside 800x560 DIP scrim");
                app.CloseDrawers();

                app.SetActiveOverlay(Application::OverlayKind::OpenGitHubModal);
                app.Layout();
                Require(Inside(app._openGitHubPanel->GetBounds(), safeFrame) && Inside(app._openGitHubOpenBtn->GetBounds(), app._openGitHubPanel->GetBounds()),
                        "OpenGitHubModal fits inside 800x560 DIP scrim");
                app.CloseDrawers();
            }

            // P1-02: Modal input containment across Tab trap (with Peek eye), RouteShortcut, WM_MOUSEWHEEL, Backdrop wheel, RevealPaneOverlay, Menu, Spell
            // menu.
            Resize(app, 1280, 800, 96);
            app._settings.spelling.enabled = true;
            app.SetDocument(L"Teh misspelled text\n");
            app.Prepare(128);
            app.SetActiveOverlay(Application::OverlayKind::OpenGitHubModal);
            app.Layout();
            app._openGitHubTokenInput->SetText(L"ghp_testToken12345");
            app._host.SetFocusControl(app._openGitHubTokenInput, false);
            const auto containsChild = [&](auto&& self, const DxUi::Control* parent, const DxUi::Control* needle) -> bool
            {
                if (! parent || ! needle)
                    return false;
                if (parent == needle)
                    return true;
                const std::size_t n = parent->GetLogicalChildCount();
                for (std::size_t i = 0; i < n; ++i)
                    if (self(self, parent->GetLogicalChild(i), needle))
                        return true;
                return false;
            };
            for (int step = 0; step < 12; ++step)
            {
                (void)app.HandleMessage(app.Window(), WM_KEYDOWN, VK_TAB, 0);
                auto* const focused = app._host.GetFocusControl();
                Require(focused != nullptr, "modal Tab focus remains non-null");
                // Must stay strictly inside _openGitHubPanel subtree (never escape to _editor / sidebars).
                Require(containsChild(containsChild, app._openGitHubPanel, focused), "VK_TAB focus stays trapped inside active modal subtree");
            }

            const auto scaleBeforeWheel = app._settings.textScale;
            (void)app.HandleMessage(app.Window(), WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA), 0);
            Require(app._settings.textScale == scaleBeforeWheel, "Ctrl+Wheel zoom is blocked while modal UI is active");
            Require(app._modalBackdrop->OnMouseWheel(app._host, {10.f, 10.f}, static_cast<float>(WHEEL_DELTA), 0u),
                    "ModalBackdrop consumes mouse wheel events");

            app._settings.showFiles = false;
            app.Layout();
            app.RevealPaneOverlay(PaneSide::Left);
            Require(! app.IsPaneOverlayActive(PaneSide::Left), "RevealPaneOverlay refuses to open side pane behind an active modal");

            std::vector<DxUi::MenuFlyoutItem> spellItems;
            app.PopulateSpellContextMenu({10, 10}, false, spellItems);
            Require(spellItems.empty(), "PopulateSpellContextMenu produces no items while modal UI is active");

            {
                KeyboardState::ScopedSynthetic syntheticKeys;
                std::array<BYTE, 256> keys{};
                keys[VK_CONTROL] = 0x80;
                KeyboardState::Write(keys);
                (void)app.HandleMessage(app.Window(), WM_KEYDOWN, VK_TAB, 0);
                Require(containsChild(containsChild, app._openGitHubPanel, app._host.GetFocusControl()),
                        "Ctrl+Tab focus stays trapped inside active modal subtree");
                const MSG ctrlN{app.Window(), WM_KEYDOWN, 'N', 0};
                const MSG ctrlF{app.Window(), WM_KEYDOWN, 'F', 0};
                Require(! app.RouteShortcut(ctrlN) && ! app.RouteShortcut(ctrlF), "Ctrl+N and Ctrl+F are blocked while modal UI is active");

                keys.fill(0);
                keys[VK_MENU] = 0x80;
                KeyboardState::Write(keys);
                const MSG altF4{app.Window(), WM_SYSKEYDOWN, VK_F4, 0};
                Require(app.RouteShortcut(altF4) && app.Window() != nullptr && ! app.IsClosing() &&
                            app._activeOverlay == Application::OverlayKind::OpenGitHubModal,
                        "Alt+F4 is consumed and blocked while modal UI is active");

                keys.fill(0);
                KeyboardState::Write(keys);
                const MSG f2{app.Window(), WM_KEYDOWN, VK_F2, 0};
                Require(! app.RouteShortcut(f2) && ! app.HasBlockDraft(), "F2 block-source shortcut is blocked while modal UI is active");
            }

            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.Window() != nullptr && ! app.IsClosing() && app._activeOverlay == Application::OverlayKind::OpenGitHubModal,
                    "WM_CLOSE is blocked while OpenGitHubModal is active");
            app.CloseDrawers();

            app._recoveryCandidates.clear();
            Application::OfferedRecoveryCandidate recoveryCandidate;
            recoveryCandidate.id            = L"m7-close-block-recovery";
            recoveryCandidate.snapshot.text = L"# Recovery\n";
            app._recoveryCandidates.push_back(recoveryCandidate);
            app.ShowRecovery();
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.Window() != nullptr && ! app.IsClosing() && app._activeOverlay == Application::OverlayKind::RecoveryModal &&
                        app._recoveryCandidates.size() == 1u,
                    "WM_CLOSE is blocked while RecoveryModal is active");
            app.CloseDrawers();
            app._recoveryCandidates.clear();

            app.SetActiveOverlay(Application::OverlayKind::RenameFileModal);
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.Window() != nullptr && ! app.IsClosing() && app._activeOverlay == Application::OverlayKind::RenameFileModal,
                    "WM_CLOSE is blocked while RenameFileModal is active");
            app.CloseDrawers();

            app._fileConflictSession = Application::FileConflictSession{
                {folder / L"src.md"},
                folder,
                L"src.md",
                false,
            };
            app._pendingPasteQueue.push_back(Application::PendingPasteItem{folder / L"queued.md", folder, false, false, false});
            app.SetActiveOverlay(Application::OverlayKind::FileConflictModal);
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.Window() != nullptr && ! app.IsClosing() && app._activeOverlay == Application::OverlayKind::FileConflictModal &&
                        app._fileConflictSession.has_value() && app._pendingPasteQueue.size() == 1u,
                    "WM_CLOSE is blocked while FileConflictModal is active without dropping conflict session or paste queue");
            app.CloseDrawers();
            Require(! app._fileConflictSession.has_value() && app._pendingPasteQueue.empty(),
                    "closing FileConflictModal cleans up conflict session and pending paste queue");
            app._recoveryCandidates.clear();

            // P0-05: _filesModel.invoked safely dismisses hover overlay before OpenPath via by-value binding copy.
            app._settings.filesRoots = {folder};
            app._settings.filesRoot  = folder;
            app._settings.showFiles  = false;
            app.RevealPaneOverlay(PaneSide::Left);
            Require(app.IsPaneOverlayActive(PaneSide::Left), "left hover overlay is active before invoking file row");
            std::uint64_t targetRowId{};
            const auto startTick = GetTickCount64();
            while (targetRowId == 0u && GetTickCount64() - startTick < 5000u)
            {
                MSG msg{};
                while (PeekMessageW(&msg, app.Window(), 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                for (const auto& pair : app._filesRowBindings)
                    if (pair.second.kind == Application::FilesRowContextBinding::Kind::LocalEntry && ! pair.second.isDirectory &&
                        pair.second.localPath == docPath)
                    {
                        targetRowId = pair.first;
                        break;
                    }
                if (targetRowId == 0u)
                    Sleep(10);
            }
            Require(targetRowId != 0u, "local doc row binding exists in _filesRowBindings");
            app._filesModel.invoked(targetRowId);
            Require(! app.IsPaneOverlayActive(PaneSide::Left), "invoking local file dismissed hover overlay safely without UAF");

            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "M7 modal/input containment test stayed hidden");
        });
    }
};

void RegisterPanesTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    PanesProbe::RegisterTests(root, test);
}
} // namespace RedTilde
