#include "Application.Tests.SaveDialog.h"
#include "Common/Recovery.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace RedTilde
{
class WorkspaceDialogsProbe final
{
public:
    [[nodiscard]] static Application::OverlayKind ActiveOverlay(const Application& app) noexcept
    {
        return app._activeOverlay;
    }

    [[nodiscard]] static bool SaveConfirmVisible(const Application& app) noexcept
    {
        return app._saveConfirmPanel && app._saveConfirmPanel->IsVisible();
    }

    [[nodiscard]] static bool RecoveryPanelVisible(const Application& app) noexcept
    {
        return app._recoveryPanel && app._recoveryPanel->IsVisible();
    }

    [[nodiscard]] static bool BackdropVisible(const Application& app) noexcept
    {
        return app._modalBackdrop && app._modalBackdrop->IsVisible();
    }

    [[nodiscard]] static const auto& SaveConfirmRows(const Application& app) noexcept
    {
        return app._saveConfirmRowControls;
    }

    [[nodiscard]] static DxUi::Checkbox* SelectAllBox(const Application& app) noexcept
    {
        return app._saveConfirmSelectAll;
    }

    [[nodiscard]] static DxUi::Button* PrimarySaveBtn(const Application& app) noexcept
    {
        return app._saveConfirmPrimary;
    }

    [[nodiscard]] static std::wstring TitleText(const Application& app)
    {
        return app._saveConfirmTitle ? std::wstring(app._saveConfirmTitle->GetText()) : std::wstring{};
    }

    [[nodiscard]] static const auto& InactiveDocs(const Application& app) noexcept
    {
        return app._inactiveDocuments;
    }

    [[nodiscard]] static const auto& Session(const Application& app) noexcept
    {
        return app._session;
    }

    static bool SwitchDoc(Application& app, std::size_t index)
    {
        return app.SwitchWorkspaceDocument(index);
    }

    static bool StartDoc(Application& app)
    {
        return app.StartWorkspaceDocument();
    }

    static Settings& MutableSettings(Application& app) noexcept
    {
        return app._settings;
    }
};

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

void Pump(Application& app, unsigned turns = 8)
{
    for (unsigned turn = 0; turn < turns; ++turn)
    {
        MSG message{};
        while (PeekMessageW(&message, app.Window(), 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

template <class Predicate> void Await(Application& app, Predicate&& ready, std::string_view message)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (! ready())
    {
        Pump(app, 4);
        if (ready())
            return;
        if (std::chrono::steady_clock::now() >= deadline)
            Require(false, message);
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}

void Edit(Application& app, std::wstring_view addition)
{
    const auto end = app.Document().Snapshot().Length();
    app.Document().SetSelection({end, end});
    Require(app.Document().ReplaceSelection(std::wstring(addition)), "edit fixture text");
}
} // namespace

void RegisterSaveDialogTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Recovery.SessionResumeFormatAndRoundTrip",
         [&]
    {
        RecoverySnapshot sessionSnap;
        sessionSnap.text                = L"# Session Resume\r\nDraft 東京 😀\r\n";
        sessionSnap.originalPath        = L"C:\\Workspace\\Notes\\daily.md";
        sessionSnap.bom                 = true;
        sessionSnap.selection           = {5u, 11u};
        sessionSnap.generation          = 42u;
        sessionSnap.kind                = RecoverySnapshotKind::SessionResume;
        sessionSnap.workspaceOrder      = 2u;
        sessionSnap.wasActive           = true;
        sessionSnap.savedBaselineStamp  = FileStamp{true, 1024u, 123456789u, 987654321u};
        sessionSnap.origin.kind         = DocumentOriginInfo::Kind::GitHub;
        sessionSnap.origin.owner        = L"RedSalamanders";
        sessionSnap.origin.repo         = L"RedTilde";
        sessionSnap.origin.branch       = L"main";
        sessionSnap.origin.relativePath = L"docs/README.md";
        sessionSnap.origin.blobSha      = L"abcdef0123456789";

        const auto bytes = SerializeRecovery(sessionSnap);
        Require(bytes.find("RedTilde.Recovery/2\n") == 0u, "SessionResume recovery uses version 2 envelope");

        RecoverySnapshot parsed;
        Success(ParseRecovery(bytes, parsed), "Recovery v2 envelope parses cleanly");
        Require(parsed.kind == RecoverySnapshotKind::SessionResume && parsed.workspaceOrder == 2u && parsed.wasActive &&
                    parsed.savedBaselineStamp == sessionSnap.savedBaselineStamp && parsed.origin == sessionSnap.origin && parsed.text == sessionSnap.text &&
                    parsed.originalPath == sessionSnap.originalPath && parsed.bom == sessionSnap.bom && parsed.selection == sessionSnap.selection &&
                    parsed.generation == 42u,
                "Recovery v2 round-trips kind, workspaceOrder, wasActive, baseline stamp, GitHub origin, text, selection, and generation");

        // Corrupt checksum / payload rejection
        std::string corrupted = bytes;
        corrupted.back()      = (corrupted.back() == 'A') ? 'B' : 'A';
        RecoverySnapshot rejected;
        Require(FAILED(ParseRecovery(corrupted, rejected)), "corrupted v2 recovery snapshot fails checksum verification");

        // CrashRecovery v1 backward-compatible parsing
        RecoverySnapshot crashV1;
        crashV1.text         = L"# Crash v1\r\nRecovered line\r\n";
        crashV1.originalPath = L"C:\\Workspace\\crash.md";
        crashV1.generation   = 7u;
        crashV1.kind         = RecoverySnapshotKind::CrashRecovery;
        const auto v1Bytes   = SerializeRecovery(crashV1);
        Require(v1Bytes.find("RedTilde.Recovery/1\n") == 0u, "CrashRecovery without metadata preserves compact v1 envelope");
        RecoverySnapshot parsedV1;
        Require(SUCCEEDED(ParseRecovery(v1Bytes, parsedV1)) && parsedV1.kind == RecoverySnapshotKind::CrashRecovery && parsedV1.text == crashV1.text,
                "Recovery v1 still parses cleanly as CrashRecovery");
        (void)root;
    });

    test("Settings.MultiFolderGitHubAndExitBehaviorRoundTrip",
         [&]
    {
        const auto dir          = root / L"settings-w1-w4";
        const auto settingsPath = dir / SettingsFileName();
        std::filesystem::create_directories(dir);

        // Legacy filesRoot migration + default Prompt
        Success(WriteBytesAtomic(settingsPath, "{\"schema\":1,\"filesRoot\":\"C:\\\\Notes\\\\Work\"}"), "seed legacy settings json");
        Settings loaded;
        Success(LoadSettings(dir, loaded), "load legacy filesRoot settings");
        Require(loaded.filesRoots.size() == 1u && loaded.filesRoots[0] == std::filesystem::path(L"C:\\Notes\\Work") &&
                    loaded.exitUnsavedBehavior == ExitUnsavedBehavior::Prompt,
                "legacy filesRoot populates filesRoots and exitUnsavedBehavior defaults to Prompt");

        // Round-trip 16 folders + 8 GitHub repos + KeepAndResume mode + redSalamanderPath
        Settings custom = loaded;
        custom.filesRoots.clear();
        for (int i = 0; i < 20; ++i) // 20 exceeds cap of 16
            custom.filesRoots.push_back(std::filesystem::path(std::format(L"C:\\Workspace\\Folder{}", i)));
        custom.filesRoot = custom.filesRoots.front();
        custom.githubRepos.clear();
        for (int i = 0; i < 10; ++i) // 10 exceeds cap of 8
            custom.githubRepos.push_back({L"RedSalamanders", std::format(L"Repo{}", i), L"main"});
        custom.githubRepos.push_back({L"invalid/owner", L"bad", L"main"}); // invalid owner segment rejected
        custom.redSalamanderPath   = L"C:\\Tools\\RedSalamander\\RedSalamander.exe";
        custom.exitUnsavedBehavior = ExitUnsavedBehavior::KeepAndResume;
        Success(SaveSettings(dir, custom), "save W1-W4 custom settings");

        Settings reloaded;
        Success(LoadSettings(dir, reloaded), "reload W1-W4 settings");
        Require(reloaded.filesRoots.size() == MaximumFilesRoots && reloaded.filesRoot == reloaded.filesRoots.front(),
                "filesRoots enforces MaximumFilesRoots (16) and stays synced with filesRoot");
        Require(reloaded.githubRepos.size() == MaximumGitHubRepos && reloaded.githubRepos[0].owner == L"RedSalamanders" &&
                    reloaded.githubRepos[0].branch == L"main",
                "githubRepos enforces MaximumGitHubRepos (8) and rejects invalid owner segments");
        Require(reloaded.redSalamanderPath == L"C:\\Tools\\RedSalamander\\RedSalamander.exe" &&
                    reloaded.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume,
                "redSalamanderPath and ExitUnsavedBehavior::KeepAndResume round-trip faithfully");

        // Invalid exitUnsavedBehavior falls back cleanly to Prompt
        Success(WriteBytesAtomic(settingsPath, "{\"schema\":1,\"exitUnsavedBehavior\":\"garbageValue\",\"filesRoots\":[]}"), "seed invalid enum json");
        Settings fallback;
        Success(LoadSettings(dir, fallback), "load settings with invalid exitUnsavedBehavior");
        Require(fallback.exitUnsavedBehavior == ExitUnsavedBehavior::Prompt, "unrecognized exitUnsavedBehavior falls back to default Prompt");
    });

    test("Editor.WorkspaceSaveConfirmSingleMultiAndChecklist",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto dir     = root / L"save-confirm-modal";
        const auto folderA = dir / L"WorkspaceA" / L"docs";
        const auto folderB = dir / L"WorkspaceB" / L"docs";
        std::filesystem::create_directories(folderA / L"sub");
        std::filesystem::create_directories(folderB / L"sub");
        const auto fileA = folderA / L"sub" / L"alpha.md";
        const auto fileB = folderB / L"sub" / L"beta.md";
        Success(WriteBytesAtomic(fileA, "# Alpha\r\n"), "seed alpha.md");
        Success(WriteBytesAtomic(fileB, "# Beta\r\n"), "seed beta.md");

        LaunchOptions launch;
        launch.headless                   = true;
        launch.dataDirectory              = dir / L"appdata";
        launch.forceDxUiSaveDialogForTest = true;
        Application app(launch);
        Success(app.Initialize(), "initialize SaveConfirmModal fixture");
        Require(app.Preferences().exitUnsavedBehavior == ExitUnsavedBehavior::Prompt, "ExitUnsavedBehavior::Prompt is active by default on manual exit");
        WorkspaceDialogsProbe::MutableSettings(app).filesRoots = {folderA, folderB};
        WorkspaceDialogsProbe::MutableSettings(app).filesRoot  = folderA;

        // Open fileA & edit -> Single-document SaveConfirmModal
        FileStamp stampA{};
        StoredDocument docA;
        Success(LoadDocument(fileA, docA), "load alpha.md");
        Require(app.SetDocument(docA.text, fileA, docA.bom, docA.stamp), "open alpha.md");
        Edit(app, L"Unsaved line A\r\n");

        (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
        Await(app, [&] { return WorkspaceDialogsProbe::SaveConfirmVisible(app); }, "single-doc SaveConfirmModal opens");
        Require(WorkspaceDialogsProbe::ActiveOverlay(app) == Application::OverlayKind::SaveConfirmModal && WorkspaceDialogsProbe::BackdropVisible(app) &&
                    ! WorkspaceDialogsProbe::RecoveryPanelVisible(app),
                "SaveConfirmModal activates modal backdrop and stays distinct from RecoveryModal");
        Require(WorkspaceDialogsProbe::SaveConfirmRows(app).size() >= 1u && WorkspaceDialogsProbe::SaveConfirmRows(app)[0].checkbox->GetText() == L"alpha.md" &&
                    WorkspaceDialogsProbe::SaveConfirmRows(app)[0].locationLabel->GetText().find(L"WorkspaceA") != std::wstring::npos &&
                    WorkspaceDialogsProbe::SaveConfirmRows(app)[0].locationLabel->GetTooltipText() == fileA.native(),
                "single-doc SaveConfirmModal displays filename, disambiguated relative path, and full-path tooltip");

        // Cancel preserves edits and closes modal
        app.Execute(Command::CloseDrawer);
        Require(WorkspaceDialogsProbe::ActiveOverlay(app) == Application::OverlayKind::None && app.Dirty() && app.Window() != nullptr,
                "Cancel dismisses SaveConfirmModal without saving or closing");

        // Open fileB in a second workspace slot -> Multi-doc checklist modal
        StoredDocument docB;
        Success(LoadDocument(fileB, docB), "load beta.md");
        Require(WorkspaceDialogsProbe::StartDoc(app), "start second workspace slot");
        Require(app.SetDocument(docB.text, fileB, docB.bom, docB.stamp), "open beta.md");
        Edit(app, L"Unsaved line B\r\n");

        (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
        Await(app, [&] { return WorkspaceDialogsProbe::SaveConfirmVisible(app); }, "multi-doc SaveConfirmModal opens");
        Require(WorkspaceDialogsProbe::SelectAllBox(app) && WorkspaceDialogsProbe::SelectAllBox(app)->IsVisible(),
                "multi-doc SaveConfirmModal reveals Select All checkbox");
        Require(WorkspaceDialogsProbe::SaveConfirmRows(app).size() >= 2u, "multi-doc SaveConfirmModal lists both dirty documents");

        // Uncheck Select All via real Checkbox spacebar activation -> disables primary Save Selected button; re-check one item and Save Selected!
        WorkspaceDialogsProbe::SelectAllBox(app)->OnKeyDown(app.Host(), VK_SPACE, 0u);
        Require(! WorkspaceDialogsProbe::PrimarySaveBtn(app)->IsEnabled(), "unchecking all items disables Save Selected button");

        // Check only beta.md
        for (std::size_t i = 0; i < 2u; ++i)
        {
            if (WorkspaceDialogsProbe::SaveConfirmRows(app)[i].checkbox->GetText() == L"beta.md")
                WorkspaceDialogsProbe::SaveConfirmRows(app)[i].checkbox->OnKeyDown(app.Host(), VK_SPACE, 0u);
        }
        Require(WorkspaceDialogsProbe::PrimarySaveBtn(app)->IsEnabled(), "checking beta.md re-enables Save Selected");
        app.Execute(Command::SaveConfirmAccept);
        Await(app, [&] { return app.Window() == nullptr; }, "Save Selected saves beta.md, skips unchecked alpha.md, and closes");

        StoredDocument afterA, afterB;
        Success(LoadDocument(fileA, afterA), "reload unchecked alpha.md");
        Success(LoadDocument(fileB, afterB), "reload checked beta.md");
        Require(afterA.text.find(L"Unsaved line A") == std::wstring::npos, "unchecked alpha.md was not overwritten on disk");
        Require(afterB.text.find(L"Unsaved line B") != std::wstring::npos, "checked beta.md was atomically saved before exit");
        Require(foreground.NeverAcquiredForeground(), "SaveConfirmModal test never takes desktop focus");
    });

    test("Editor.WorkspaceKeepAndExitResumeAndShutdown",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto dir     = root / L"keep-and-resume";
        const auto dataDir = dir / L"appdata";
        const auto file1   = dir / L"first.md";
        const auto file2   = dir / L"second.md";
        const auto cliFile = dir / L"cli-opened.md";
        std::filesystem::create_directories(dir);
        Success(WriteBytesAtomic(file1, "# First\r\nBase\r\n"), "seed first.md");
        Success(WriteBytesAtomic(file2, "# Second\r\nBase\r\n"), "seed second.md");
        Success(WriteBytesAtomic(cliFile, "# CLI Opened\r\nClean file\r\n"), "seed cli-opened.md");

        // 1. Default Prompt opens SaveConfirmModal; clicking Keep & Exit preserves both dirty docs cleanly
        {
            LaunchOptions launch;
            launch.headless                   = true;
            launch.dataDirectory              = dataDir;
            launch.forceDxUiSaveDialogForTest = true;
            Application app(launch);
            Success(app.Initialize(), "initialize Keep & Exit app");
            StoredDocument d1, d2;
            Success(LoadDocument(file1, d1), "load first.md");
            Success(LoadDocument(file2, d2), "load second.md");
            Require(app.SetDocument(d1.text, file1, d1.bom, d1.stamp), "open first.md");
            Edit(app, L"Kept draft one\r\n");
            Require(WorkspaceDialogsProbe::StartDoc(app), "start second workspace slot");
            Require(app.SetDocument(d2.text, file2, d2.bom, d2.stamp), "open second.md");
            Edit(app, L"Kept draft two\r\n");

            Require(WorkspaceDialogsProbe::MutableSettings(app).exitUnsavedBehavior == ExitUnsavedBehavior::Prompt,
                    "Prompt is active by default on manual exit");
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Await(app, [&] { return WorkspaceDialogsProbe::SaveConfirmVisible(app); }, "manual close opens SaveConfirmModal by default");
            app.Execute(Command::SaveConfirmKeepExit);
            Await(app, [&] { return app.Window() == nullptr; }, "clicking Keep & Exit closes cleanly while preserving drafts");
        }

        // 1b. Explicit KeepAndResume setting skips SaveConfirmModal on manual close
        {
            LaunchOptions launch;
            launch.headless                   = true;
            launch.dataDirectory              = dir / L"optin-appdata";
            launch.forceDxUiSaveDialogForTest = true;
            Application app(launch);
            Success(app.Initialize(), "initialize explicit KeepAndResume opt-in app");
            WorkspaceDialogsProbe::MutableSettings(app).exitUnsavedBehavior = ExitUnsavedBehavior::KeepAndResume;
            StoredDocument d1;
            Success(LoadDocument(file1, d1), "load first.md for KeepAndResume opt-in");
            Require(app.SetDocument(d1.text, file1, d1.bom, d1.stamp), "open first.md for KeepAndResume opt-in");
            Edit(app, L"Opt-in hot-exit line\r\n");
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Await(app, [&] { return app.Window() == nullptr; }, "explicit KeepAndResume exits immediately without modal dialog");
        }

        // 2. Relaunch without --document: auto-resumes both dirty docs directly WITHOUT RecoveryModal; Ctrl+S saves straight to file2 and detects conflict on
        // file1
        bool conflictTriggered{};
        {
            LaunchOptions relaunch;
            relaunch.headless              = true;
            relaunch.dataDirectory         = dataDir;
            relaunch.startupRestoreForTest = true;
            relaunch.saveConflictForTest   = [&]
            {
                conflictTriggered = true;
                return SaveConflictChoice::Cancel;
            };
            Application app(relaunch);
            Success(app.Initialize(), "initialize relaunched Keep & Exit app");
            Await(app, [&] {
                return WorkspaceDialogsProbe::InactiveDocs(app).size() == 1u && app.Dirty();
            }, "relaunch auto-adopts both SessionResume drafts without prompting");
            Require(! WorkspaceDialogsProbe::RecoveryPanelVisible(app) && app.Window() != nullptr, "SessionResume relaunch does not show RecoveryModal");
            Require(app.Document().GetText().find(L"Kept draft two") != std::wstring::npos && ! app.RequiresSaveAs(),
                    "active resumed document restores last active slot and does not require Save As");

            // Saving file2 directly via Ctrl+S works straight back to file2.md
            app.Execute(Command::Save);
            Await(app, [&] { return ! app.Dirty(); }, "Ctrl+S writes resumed draft straight back to original path");
            StoredDocument saved2;
            Success(LoadDocument(file2, saved2), "verify saved second.md");
            Require(saved2.text.find(L"Kept draft two") != std::wstring::npos, "resumed draft saved cleanly to original file");

            // External edit on file1 while draft 1 is resumed triggers SaveConflict
            Require(WorkspaceDialogsProbe::SwitchDoc(app, 0u), "switch back to resumed first.md");
            Success(WriteBytesAtomic(file1, "# First\r\nExternal disk edit!\r\n"), "simulate external edit to first.md");
            app.Execute(Command::Save);
            Await(app, [&] { return conflictTriggered; }, "Ctrl+S on externally modified resumed draft detects FileStamp conflict");
            Require(app.Dirty(), "cancelling SaveConflict keeps resumed draft dirty");
        }

        // 3. Shutdown resume while ExitUnsavedBehavior::Prompt is set + CLI --document precedence
        {
            LaunchOptions shutdownApp;
            shutdownApp.headless      = true;
            shutdownApp.dataDirectory = dir / L"shutdown-data";
            Application app(shutdownApp);
            Success(app.Initialize(), "initialize shutdown app");
            Require(app.Preferences().exitUnsavedBehavior == ExitUnsavedBehavior::Prompt, "Prompt default applies to manual close only");
            StoredDocument d1;
            Success(LoadDocument(file1, d1), "load first.md for shutdown");
            Require(app.SetDocument(d1.text, file1, d1.bom, d1.stamp), "open first.md for shutdown");
            Edit(app, L"Shutdown resume line\r\n");

            const LRESULT queryRes = SendMessageW(app.Window(), WM_QUERYENDSESSION, 0, ENDSESSION_CLOSEAPP);
            Require(queryRes == TRUE && ! WorkspaceDialogsProbe::SaveConfirmVisible(app),
                    "WM_QUERYENDSESSION returns TRUE immediately without modal prompt even in Prompt mode");
            (void)SendMessageW(app.Window(), WM_ENDSESSION, TRUE, ENDSESSION_CLOSEAPP);
            DestroyWindow(app.Window());
        }
        {
            LaunchOptions cliLaunch;
            cliLaunch.headless      = true;
            cliLaunch.dataDirectory = dir / L"shutdown-data";
            cliLaunch.document      = cliFile;
            Application app(cliLaunch);
            Success(app.Initialize(), "initialize CLI --document launch alongside SessionResume");
            Await(app, [&] {
                return WorkspaceDialogsProbe::InactiveDocs(app).size() == 1u;
            }, "SessionResume drafts preserve CLI --document active and keep resumed draft in workspace");
            Require(WorkspaceDialogsProbe::Session(app).path == cliFile && ! app.Dirty(), "CLI --document remains the active clean document");
            Require(WorkspaceDialogsProbe::InactiveDocs(app)[0].dirty && WorkspaceDialogsProbe::InactiveDocs(app)[0].metadata.resumedUnsaved,
                    "shutdown draft is preserved in workspace as dirty resumedUnsaved");
        }
        Require(foreground.NeverAcquiredForeground(), "Keep & Exit and shutdown resume tests never acquire desktop focus");
    });
}
} // namespace RedTilde
