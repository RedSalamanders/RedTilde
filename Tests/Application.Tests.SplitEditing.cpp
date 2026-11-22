#include "Application.Tests.SplitEditing.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include "RedTilde/Application.h"
#include "RedTilde/KeyboardState.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

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
std::wstring LongMarkdownSections(unsigned count)
{
    std::wstring doc = L"# Split Document\n\n";
    for (unsigned i = 1; i <= count; ++i)
        doc += std::format(L"## Section {}\n\nParagraph {} keeps writing calm, readable, and consistent across long multi-section drafts.\n\n", i, i);
    return doc;
}
} // namespace

class SplitEditingProbe final
{
    static LaunchOptions Options(const std::filesystem::path& root, std::wstring_view name, bool watchSettings = false)
    {
        LaunchOptions options;
        options.headless             = true;
        options.dataDirectory        = root / name;
        options.scenario             = L"compose-light";
        options.watchSettingsForTest = watchSettings;
        return options;
    }

    static void Resize(Application& app, unsigned width, unsigned height)
    {
        RECT rect{};
        Require(GetWindowRect(app.Window(), &rect) != FALSE, "read hidden split test frame");
        const UINT dpi = GetDpiForWindow(app.Window());
        rect.right     = rect.left + MulDiv(static_cast<int>(width), static_cast<int>(dpi), 96);
        rect.bottom    = rect.top + MulDiv(static_cast<int>(height), static_cast<int>(dpi), 96);
        Require(SetWindowPos(app.Window(),
                             nullptr,
                             rect.left,
                             rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                "size hidden split test window");
        app.Layout();
        app.Prepare(128);
    }

    static void PressF6(Application& app)
    {
        KeyboardState::ScopedSynthetic synthetic;
        constexpr std::array<BYTE, 256> cleanKeys{};
        KeyboardState::Write(cleanKeys);
        MSG msg{};
        msg.hwnd    = app.Window();
        msg.message = WM_KEYDOWN;
        msg.wParam  = VK_F6;
        Require(app.RouteShortcut(msg), "F6 shortcut routes Command::SwitchSplitPane");
    }

public:
    static void RegisterTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
    {
        test("Editor.SplitBidirectionalClickF6AndUnifiedUndoHistory",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"split-bidirectional-undo"));
            Success(app.Initialize(), "initialize hidden bidirectional split app");
            Resize(app, 1440, 900);
            Require(app.SetDocument(L"# First\n\nBody.", {}, false, {}), "seed split document");
            app.SetView(EditorView::Split);
            app.Prepare(128);

            Require(app.ActiveSplitPane() == SplitActivePane::Compose && app.ActiveEditor() == app._editor && ! app._editor->IsReadOnly() &&
                        app._mirror->IsReadOnly() && app._splitComposeChip->GetText() == Resource(IDS_SPLIT_EDITING_CHIP) &&
                        app._splitSourceChip->GetText() == Resource(IDS_READONLYSHORT),
                    "Split initially activates Compose with Source read-only and matching status chips");

            app.Document().SetSelection({app.Document().Snapshot().Length(), app.Document().Snapshot().Length()});
            Require(app.Document().ReplaceSelection(L" One"), "edit in Compose pane");
            app.Prepare(64);
            Require(app._mirror->GetText() == L"# First\n\nBody. One", "Compose edit synchronizes to read-only Source pane");

            const auto mirrorViewport = app._mirror->TryGetTextInputViewportRect();
            Require(mirrorViewport.has_value(), "Source pane exposes valid viewport for pointer click");
            (void)app._mirror->OnMouseDown(
                app.Host(), {(mirrorViewport->left + mirrorViewport->right) * 0.5f, (mirrorViewport->top + mirrorViewport->bottom) * 0.5f}, false, 0u);
            app.Prepare(128);

            Require(app.ActiveSplitPane() == SplitActivePane::Source && app.ActiveEditor() == app._mirror && ! app._mirror->IsReadOnly() &&
                        app._editor->IsReadOnly() && app.Activity().splitPaneSwitches == 1u &&
                        app._splitSourceChip->GetText() == Resource(IDS_SPLIT_EDITING_CHIP) && app._splitComposeChip->GetText() == Resource(IDS_READONLYSHORT),
                    "clicking inside passive Source pane transfers editable authority and updates header chips");

            app.Document().SetSelection({app.Document().Snapshot().Length(), app.Document().Snapshot().Length()});
            Require(app.Document().ReplaceSelection(L" Two"), "edit directly in active Source pane");
            app.Prepare(128);
            Require(app._editor->GetText() == L"# First\n\nBody. One Two" && app.SourceMirror().GetText() == L"# First\n\nBody. One Two",
                    "Source edit live-synchronizes back to read-only Compose preview");

            PressF6(app);
            app.Prepare(64);
            Require(app.ActiveSplitPane() == SplitActivePane::Compose && app.ActiveEditor() == app._editor && ! app._editor->IsReadOnly() &&
                        app._mirror->IsReadOnly() && app.Activity().splitPaneSwitches == 2u,
                    "F6 returns authoritative editing cleanly to Compose pane");

            app.Execute(Command::Undo);
            app.Prepare(64);
            Require(app.Document().GetText() == L"# First\n\nBody. One", "first Undo reverts Source edit across pane transfer");
            app.Execute(Command::Undo);
            app.Prepare(64);
            Require(app.Document().GetText() == L"# First\n\nBody.", "second Undo reverts earlier Compose edit across unified history");
            app.Execute(Command::Redo);
            app.Execute(Command::Redo);
            app.Prepare(64);
            Require(app.Document().GetText() == L"# First\n\nBody. One Two", "unified Redo restores both Compose and Source transactions");
            PressF6(app);
            Require(app.ActiveSplitPane() == SplitActivePane::Source && app.Document().CanUndo(),
                    "128-step Undo stack stays available after switching back to Source");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "bidirectional test stays hidden");
        });

        test("Editor.SplitScrollbarClickDoesNotStealEditFocusAndExitRestoresAuthority",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"split-scrollbar-exit"));
            Success(app.Initialize(), "initialize hidden split scrollbar guard app");
            Resize(app, 1440, 720);
            Require(app.SetDocument(LongMarkdownSections(35u), {}, false, {}), "seed long scrollable split document");
            app.SetView(EditorView::Split);
            app.Prepare(256);

            const auto mirrorInfo = app._mirror->GetScrollInfo();
            Require(mirrorInfo && mirrorInfo->contentHeightDip > mirrorInfo->viewportHeightDip + 40.f, "Source mirror is vertically scrollable");
            const auto bounds = app._mirror->GetBounds();
            (void)app._mirror->OnMouseDown(app.Host(), {bounds.right - 6.f, bounds.top + 24.f}, false, 0u);
            Require(app.ActiveSplitPane() == SplitActivePane::Compose && app.Activity().splitPaneSwitches == 0u && ! app._editor->IsReadOnly() &&
                        app._mirror->IsReadOnly(),
                    "clicking inside passive vertical scrollbar track does not steal Split active edit focus");

            PressF6(app);
            Require(app.ActiveSplitPane() == SplitActivePane::Source, "F6 transfers active editing to Source");
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Edited "), "type while Source is active");
            app.SetView(EditorView::Compose);
            app.Prepare(64);
            Require(app.ActiveSplitPane() == SplitActivePane::Compose && app.ActiveEditor() == app._editor && ! app._editor->IsReadOnly() &&
                        app._editor->GetText().starts_with(L"Edited "),
                    "leaving Split returns authoritative edited Document cleanly to _editor");

            app.SetView(EditorView::Split);
            app.Prepare(64);
            PressF6(app);
            Require(app.ActiveSplitPane() == SplitActivePane::Source, "re-enter Source active before workspace switch");
            Require(app.StartWorkspaceDocument(), "StartWorkspaceDocument restores primary authority before workspace switch");
            Require(app.ActiveSplitPane() == SplitActivePane::Compose && ! app._editor->IsReadOnly(),
                    "workspace document transition ensures _editor holds canonical active state");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "scrollbar-guard test stays hidden");
        });

        test("Editor.SplitActivePaneFindReplaceWithoutRebuild",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"split-find-replace"));
            Success(app.Initialize(), "initialize hidden split find/replace app");
            Resize(app, 1440, 900);
            Require(app.SetDocument(L"# Find\n\nalpha middle alpha end.", {}, false, {}), "seed Find target document");
            app.SetView(EditorView::Split);
            app.Prepare(128);

            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"alpha");
            app.RefreshFind();
            app.Layout();
            app.Prepare(64);
            Require(app._findBar->IsVisible() && app._findBar->GetAccessibleName() == Resource(IDS_FIND_IN_COMPOSE) &&
                        app._editor->GetSourceHighlights().size() == 2u && app._mirror->GetSourceHighlights().empty() &&
                        app._findBar->GetBounds().right <= app._page->splitChrome.splitX + 1.f,
                    "Find highlights strictly in Compose half and anchors FindBar inside Compose half");

            const auto rebuildsBefore = app.Activity().findMatchRebuilds;
            PressF6(app);
            app.Prepare(64);
            Require(app.Activity().findMatchRebuilds == rebuildsBefore && app._findBar->GetAccessibleName() == Resource(IDS_FIND_IN_SOURCE) &&
                        app._editor->GetSourceHighlights().empty() && app._mirror->GetSourceHighlights().size() == 2u &&
                        app._findBar->GetBounds().left >= app._page->splitChrome.splitX - 32.f,
                    "switching Split side transfers highlights and FindBar to Source without rebuilding MatchTable");

            app._replaceText->SetTextAndNotify(L"omega");
            app.Execute(Command::ReplaceAll);
            app.Prepare(128);
            Require(app._mirror->GetText() == L"# Find\n\nomega middle omega end." && app._editor->GetText() == L"# Find\n\nomega middle omega end.",
                    "ReplaceAll edits active Source pane and synchronizes passive Compose preview");

            app.Execute(Command::CloseDrawer);
            Require(! app._findBar->IsVisible() && app._editor->GetSourceHighlights().empty() && app._mirror->GetSourceHighlights().empty(),
                    "closing Find clears highlights from both Split panes");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "find-replace test stays hidden");
        });

        test("Editor.SplitLinkedAndUnlinkedScrollSync",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"split-scroll-sync"));
            Success(app.Initialize(), "initialize hidden split scroll sync app");
            Resize(app, 1440, 820);
            Require(app.SetDocument(LongMarkdownSections(45u), {}, false, {}), "seed 45-section scrollable Split document");
            app.SetView(EditorView::Split);
            app.Prepare(256);

            Require(app.IsSplitScrollSyncEnabled() && app._splitScrollSyncButton->GetToggleState() == std::optional<bool>{true} &&
                        app._page->splitChrome.linked,
                    "linked scrolling defaults to enabled with matching button and Split chrome state");

            const auto syncsBefore = app.Activity().splitScrollSyncs;
            Require(app._editor->SetScrollOffsetDip(420.f), "scroll Compose driver pane down");
            app.Prepare(128);
            const auto syncedMirror = app._mirror->GetScrollInfo();
            Require(syncedMirror && syncedMirror->offsetDip > 50.f && app.Activity().splitScrollSyncs > syncsBefore &&
                        app.Activity().splitScrollSyncs <= syncsBefore + 4u,
                    "linked Compose scroll advances Source follower without ping-pong loop");

            const float mirrorMax = std::max(0.f, syncedMirror->contentHeightDip - syncedMirror->viewportHeightDip);
            Require(app._mirror->SetScrollOffsetDip(mirrorMax), "scroll Source pane to bottom edge");
            app.Prepare(128);
            const auto syncedCompose = app._editor->GetScrollInfo();
            Require(syncedCompose && syncedCompose->offsetDip >= (syncedCompose->contentHeightDip - syncedCompose->viewportHeightDip) * 0.75f,
                    "scrolling Source driver near bottom syncs Compose follower to bottom band");

            app.Execute(Command::ToggleSplitScrollSync);
            app.Prepare(64);
            Require(! app.IsSplitScrollSyncEnabled() && app._splitScrollSyncButton->GetToggleState() == std::optional<bool>{false} &&
                        ! app._page->splitChrome.linked,
                    "toggling Split scroll sync updates setting, button state, and dashed seam chrome");

            Require(app._editor->SetScrollOffsetDip(0.f), "reset Compose scroll independently to top");
            Require(app._mirror->SetScrollOffsetDip(260.f), "scroll Source independently while unlinked");
            app.Prepare(64);
            Require(app._editor->GetScrollInfo()->offsetDip < 5.f && app._mirror->GetScrollInfo()->offsetDip >= 250.f,
                    "unlinked panes scroll completely independently without moving peer");

            PressF6(app);
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Quiet edit\n\n"), "edit Source while Compose is scrolled independently");
            app.Prepare(128);
            Require(app._editor->GetScrollInfo()->offsetDip < 5.f, "editing Source while unlinked never yanks Compose scroll away from user position");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "scroll-sync test stays hidden");
        });

        test("Settings.SplitScrollSyncPersistenceAndLiveReload",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto settingsDir = root / L"split-settings-io";
            Settings toSave;
            toSave.splitScrollSync = false;
            Success(SaveSettings(settingsDir, toSave), "persist splitScrollSync=false");
            const auto reloaded = LoadSettingsCold(settingsDir);
            Require(reloaded.status == SettingsLoadStatus::Loaded && ! reloaded.settings.splitScrollSync, "cold settings load restores splitScrollSync=false");

            const auto file = settingsDir / SettingsFileName();
            Require(SUCCEEDED(WriteBytesAtomic(file, "{\n  schema: 1,\n  splitScrollSync: \"bad\",\n  readingWidth: 720\n}\n")),
                    "write invalid splitScrollSync type JSON5 fixture");
            const auto fallback = LoadSettingsCold(settingsDir);
            Require(fallback.status == SettingsLoadStatus::Loaded && fallback.settings.splitScrollSync && fallback.settings.readingWidth == 720u,
                    "unusable non-boolean splitScrollSync falls back cleanly to true while preserving sibling settings");

            Application app(Options(root, L"split-live-reload", true));
            Success(app.Initialize(), "initialize hidden split live-reload app");
            Resize(app, 1440, 900);
            app.SetView(EditorView::Split);
            app.Prepare(64);
            const auto writerDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
            while (app._preferenceWriter.Busy())
            {
                MSG msg{};
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                if (! app._preferenceWriter.Busy())
                    break;
                const auto now = std::chrono::steady_clock::now();
                if (now >= writerDeadline)
                    break;
                const auto remaining = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(writerDeadline - now).count());
                (void)MsgWaitForMultipleObjects(0, nullptr, FALSE, std::min<DWORD>(remaining, 25u), QS_ALLINPUT);
            }
            Require(! app._preferenceWriter.Busy(), "async preference save from SetView(Split) settles before external live edit");
            Require(app.IsSplitScrollSyncEnabled() && app._splitScrollSyncButton->GetToggleState() == std::optional<bool>{true},
                    "live app starts with splitScrollSync=true");

            const auto liveFile        = (root / L"split-live-reload") / SettingsFileName();
            const std::string liveJson = "{\n  schema: 1,\n  splitScrollSync: false,\n  appearance: \"light\"\n}\n";
            Require(SUCCEEDED(WriteBytesAtomic(liveFile, liveJson)), "write updated live JSON5 setting");
            app.ReloadSettingsFromDisk();
            Require(! app.IsSplitScrollSyncEnabled() && app._splitScrollSyncButton->GetToggleState() == std::optional<bool>{false} &&
                        ! app._page->splitChrome.linked,
                    "live reload updates splitScrollSync, seam toggle, and Split chrome without restarting");
            std::string afterDisk;
            Success(ReadBytes(liveFile, afterDisk), "verify live settings file was not rewritten by live reload");
            Require(afterDisk == liveJson, "live settings reload leaves watched JSON5 file untouched on disk");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "settings test stays hidden");
        });
    }
};

void RegisterSplitEditingTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    SplitEditingProbe::RegisterTests(root, test);
}
} // namespace RedTilde
