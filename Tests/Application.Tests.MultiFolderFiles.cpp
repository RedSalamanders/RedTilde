#include "Application.Tests.MultiFolderFiles.h"
#include "Common/FileOps.h"
#include "Common/Files.h"
#include "Common/PathSecurity.h"
#include "Common/Recovery.h"
#include "Common/Storage.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.FilesFixtures.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>
#include <shlobj.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

class MockShellReveal final : public IShellReveal
{
public:
    std::optional<std::filesystem::path> fakeRedSalamander = L"C:\\Program Files\\RedSalamander\\RedSalamander.exe";
    std::vector<std::pair<std::wstring, std::filesystem::path>> calls;

    [[nodiscard]] std::optional<std::filesystem::path> FindRedSalamander(const std::optional<std::filesystem::path>& overridePath) const noexcept override
    {
        if (overridePath && ! overridePath->empty())
            return overridePath;
        return fakeRedSalamander;
    }

    [[nodiscard]] HRESULT RevealInExplorer(const std::filesystem::path& targetPath) noexcept override
    {
        calls.emplace_back(L"Explorer", targetPath);
        return S_OK;
    }

    [[nodiscard]] HRESULT RevealInRedSalamander(const std::filesystem::path& targetPath, const std::optional<std::filesystem::path>&) noexcept override
    {
        if (! fakeRedSalamander)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        calls.emplace_back(L"RedSalamander", targetPath);
        return S_OK;
    }
};
} // namespace

class MultiFolderFilesProbe final
{
public:
    static NavigationModel& Model(Application& app) noexcept
    {
        return app._filesModel;
    }

    static const auto& Bindings(const Application& app) noexcept
    {
        return app._filesRowBindings;
    }

    static const auto& Session(const Application& app) noexcept
    {
        return app._session;
    }

    static void SetPendingContextById(Application& app, std::uint64_t id)
    {
        for (const auto& pair : app._filesRowBindings)
        {
            if (pair.first == id)
            {
                app._pendingContextTarget = pair.second;
                return;
            }
        }
        Require(false, "context target id not found in filesRowBindings");
    }

    static DxUi::TextField* RenameInput(const Application& app) noexcept
    {
        return app._renameInput;
    }

    static DxUi::Label* RenameError(const Application& app) noexcept
    {
        return app._renameError;
    }

    static Application::OverlayKind ActiveOverlay(const Application& app) noexcept
    {
        return app._activeOverlay;
    }

    static void SetOverlay(Application& app, Application::OverlayKind next)
    {
        app.SetActiveOverlay(next);
    }

    static bool PendingPasteQueueEmpty(const Application& app) noexcept
    {
        return app._pendingPasteQueue.empty();
    }

    static bool MultiFolderBusy(const Application& app) noexcept
    {
        return app._multiFolderFiles.Busy();
    }

    static std::size_t MultiFolderSnapshotCount(const Application& app) noexcept
    {
        return app._folderSnapshots.size();
    }

    static void SelectRoot(Application& app, const std::filesystem::path& path)
    {
        app.SelectFilesRoot(path);
    }

    static void AddRoot(Application& app, const std::filesystem::path& path)
    {
        app.AddFilesRoot(path);
    }

    static void RemoveRoot(Application& app, const std::filesystem::path& path)
    {
        app.RemoveFilesRoot(path);
    }

    static void OpenFile(Application& app, const std::filesystem::path& path)
    {
        app.OpenPath(path);
    }

    static Editor* ActiveEditor(Application& app) noexcept
    {
        return app.ActiveEditor();
    }

    static Editor* MirrorEditor(Application& app) noexcept
    {
        return app.MirrorEditor();
    }

    static DxUi::Label* TitleLabel(const Application& app) noexcept
    {
        return app._title;
    }

    static DxUi::Label* FilesTitleLabel(const Application& app) noexcept
    {
        return app._filesTitle;
    }

    static DxUi::Label* FileConflictDesc(const Application& app) noexcept
    {
        return app._fileConflictDesc;
    }

    static DxUi::Tree* FilesTree(const Application& app) noexcept
    {
        return app._filesTree;
    }

    static DxUi::Tree* OutlineTree(const Application& app) noexcept
    {
        return app._outlineTree;
    }

    static CollapsedPaneRail* LeftRail(const Application& app) noexcept
    {
        return app._leftRail;
    }

    static CollapsedPaneRail* RightRail(const Application& app) noexcept
    {
        return app._rightRail;
    }

    static Surface* FilesPane(const Application& app) noexcept
    {
        return app._filesPane;
    }

    static FilesService& FolderFiles(Application& app) noexcept
    {
        return app._folderFiles;
    }

    static MultiFolderFilesService& MultiFolderFiles(Application& app) noexcept
    {
        return app._multiFolderFiles;
    }

    static const auto& CutFilePaths(const Application& app) noexcept
    {
        return app._cutFilePaths;
    }

    static const auto& CollapsedFolders(const Application& app) noexcept
    {
        return app._collapsedFolders;
    }

    static const Application::FilesRowContextBinding& PendingContextTarget(const Application& app) noexcept
    {
        return app._pendingContextTarget;
    }

    static void SetSplitSourceActive(Application& app)
    {
        app.SetView(EditorView::Split);
        app.SwitchSplitActivePane(SplitActivePane::Source, false);
    }

    static void Resize(Application& app, unsigned width, unsigned height)
    {
        const UINT dpi = GetDpiForWindow(app.Window());
        RECT rect{};
        Require(GetWindowRect(app.Window(), &rect) != FALSE, "read window bounds for resize");
        rect.right  = rect.left + MulDiv(static_cast<int>(width), static_cast<int>(dpi), 96);
        rect.bottom = rect.top + MulDiv(static_cast<int>(height), static_cast<int>(dpi), 96);
        Require(SetWindowPos(app.Window(), nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE) !=
                    FALSE,
                "resize hidden multi-folder window");
        app.Layout();
        app.Prepare(64);
    }

    static void BeginCopyOrMove(Application& app, std::vector<std::filesystem::path> sources, const std::filesystem::path& destDir, bool move)
    {
        app.BeginCopyOrMoveToDirectory(std::move(sources), destDir, move);
    }

    static std::filesystem::path ActivePasteTarget(const Application& app)
    {
        return app.ActivePasteTargetDirectory();
    }

    static void ShowContextMenu(Application& app, std::uint64_t id)
    {
        for (const auto& pair : app._filesRowBindings)
        {
            if (pair.first == id)
            {
                app.ShowFilesContextMenuNow(pair.second, POINT{100, 100}, false);
                return;
            }
        }
        Require(false, "ShowContextMenu id not found");
    }

    static void CallOnMenuClosed(Application& app, std::optional<int> selected, DxUi::Control* restoreFocus) noexcept
    {
        app.OnMenuClosed(selected, restoreFocus);
    }

    static void AddConnectedGitHubRepo(Application& app, std::wstring owner, std::wstring repo, std::wstring branch, std::vector<GitHubTreeEntry> entries)
    {
        Application::GitHubConnectedRepoState state;
        state.ref            = {std::move(owner), std::move(repo), branch};
        state.resolvedBranch = std::move(branch);
        state.entries        = std::move(entries);
        state.loaded         = true;
        app._connectedGitHubRepos.push_back(std::move(state));
        app._settings.githubRepos.push_back(app._connectedGitHubRepos.back().ref);
    }

    static void AddDirtyGitHubInactiveDoc(Application& app, std::wstring owner, std::wstring repo, std::wstring branch, std::wstring relPath)
    {
        Application::WorkspaceDocument doc;
        doc.metadata.key                 = 99u;
        doc.metadata.origin.kind         = DocumentOriginInfo::Kind::GitHub;
        doc.metadata.origin.owner        = std::move(owner);
        doc.metadata.origin.repo         = std::move(repo);
        doc.metadata.origin.branch       = std::move(branch);
        doc.metadata.origin.relativePath = std::move(relPath);
        doc.dirty                        = true;
        app._inactiveDocuments.push_back(std::move(doc));
    }

    static void SetActiveDocumentGitHubOrigin(Application& app, std::wstring owner, std::wstring repo, std::wstring branch, std::wstring relPath)
    {
        app._session.path.clear();
        app._session.origin.kind         = DocumentOriginInfo::Kind::GitHub;
        app._session.origin.owner        = std::move(owner);
        app._session.origin.repo         = std::move(repo);
        app._session.origin.branch       = std::move(branch);
        app._session.origin.relativePath = std::move(relPath);
    }

    static void RefreshFiles(Application& app, bool rootChanged = false)
    {
        app.RefreshFiles(rootChanged);
    }

    static void RefreshLeftRailDots(Application& app)
    {
        app.RefreshLeftRailDots();
    }

    static void SetWorkspaceCollapsed(Application& app, bool collapsed)
    {
        app._workspaceCollapsed = collapsed;
        app.RefreshFiles();
    }

    static void RevealPaneOverlay(Application& app, PaneSide side)
    {
        app.RevealPaneOverlay(side);
    }

    static void DismissHoverOverlays(Application& app, bool animated)
    {
        app.DismissHoverOverlays(animated);
    }

    static bool IsPaneOverlayActive(const Application& app, PaneSide side) noexcept
    {
        return app.IsPaneOverlayActive(side);
    }

    static void SetPinModes(Application& app, PanePinMode filesMode, PanePinMode outlineMode, bool showFiles = true)
    {
        app._settings.filesPinMode   = filesMode;
        app._settings.outlinePinMode = outlineMode;
        app._settings.showFiles      = showFiles;
    }

    static bool SendKeyToFocused(Application& app, UINT vk)
    {
        MSG msg{};
        msg.hwnd    = app.Window();
        msg.message = WM_KEYDOWN;
        msg.wParam  = vk;
        return app.RouteShortcut(msg);
    }
};

void RegisterMultiFolderFilesTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Files.MultiFolderDisambiguationAndLocationResolver",
         [&]
    {
        const std::vector<std::filesystem::path> roots{
            L"C:\\Work\\ProjectA\\docs",
            L"C:\\Work\\ProjectB\\docs",
            L"C:\\Work\\notes",
        };
        const auto labels = DisambiguateFolderLabels(roots);
        Require(labels.size() == 3u && labels[0] == L"docs (ProjectA)" && labels[1] == L"docs (ProjectB)" && labels[2] == L"notes",
                "DisambiguateFolderLabels distinguishes colliding basenames while keeping unique basenames concise");

        const std::wstring dirtyUi = L"Hello\u202Eevil\u200B\x0007World";
        Require(SanitizeUiText(dirtyUi) == L"HelloevilWorld", "SanitizeUiText strips bidi overrides, zero-width chars, and control characters");

        const auto locA = ResolveWorkspaceDisplayLocation(L"C:\\Work\\ProjectA\\docs\\guide\\intro.md", {}, roots, L"Untitled.md");
        Require(locA.rooted && locA.fileName == L"intro.md" && locA.sectionLabel == L"docs (ProjectA)" && locA.relativePath == L"guide/intro.md",
                "ResolveWorkspaceDisplayLocation resolves disambiguated root and relative path");

        DocumentOriginInfo gh;
        gh.kind          = DocumentOriginInfo::Kind::GitHub;
        gh.owner         = L"RedSalamanders";
        gh.repo          = L"RedTilde";
        gh.relativePath  = L"Specs/README.md";
        const auto locGh = ResolveWorkspaceDisplayLocation({}, gh, roots, L"Untitled.md");
        Require(locGh.isGitHub && locGh.fileName == L"README.md" && locGh.sectionLabel == L"RedSalamanders/RedTilde" &&
                    locGh.relativePath == L"Specs/README.md",
                "ResolveWorkspaceDisplayLocation formats GitHub document locations cleanly");

        const auto locUntitled = ResolveWorkspaceDisplayLocation({}, {}, roots, L"Untitled.md");
        Require(! locUntitled.rooted && ! locUntitled.isGitHub && locUntitled.fileName == L"Untitled.md",
                "ResolveWorkspaceDisplayLocation handles untitled documents");
        (void)root;
    });

    test("Files.SafeRenamePasteCollisionAndReparseGuard",
         [&]
    {
        WindowsWorkspaceFileOps ops;
        Require(ops.ValidateFileName(L"valid-note.md") == FileNameValidationError::None, "valid filename passes");
        Require(ops.ValidateFileName(L"") == FileNameValidationError::Empty, "empty filename rejected");
        Require(ops.ValidateFileName(L"CON.md") == FileNameValidationError::ReservedName, "reserved Windows device name rejected");
        Require(ops.ValidateFileName(L"sub/file.md") == FileNameValidationError::PathSeparator, "path separator rejected");
        Require(ops.ValidateFileName(L"bad:name.md") == FileNameValidationError::InvalidCharacters, "colon rejected");
        Require(ops.ValidateFileName(L"trailing.") == FileNameValidationError::TrailingDotOrSpace, "trailing dot rejected");
        Require(QuoteWindowsCliArgument(L"--select=C:\\My Notes\\note \"1\".md") == L"\"--select=C:\\My Notes\\note \\\"1\\\".md\"",
                "QuoteWindowsCliArgument quotes spaces and embedded quotes cleanly");

        const auto workDir = root / L"safe-fileops";
        const auto subDir  = workDir / L"sub";
        const auto target  = root / L"safe-fileops-reparse-target";
        const auto linkDir = workDir / L"junction";
        std::filesystem::create_directories(subDir);
        std::filesystem::create_directories(target);
        const auto notePath = workDir / L"draft.md";
        const auto existing = subDir / L"renamed.md";
        Success(WriteBytesAtomic(notePath, "# Draft\r\n"), "seed draft.md");
        Success(WriteBytesAtomic(existing, "# Existing\r\n"), "seed existing renamed.md");
        Success(WriteBytesAtomic(target / L"secret.md", "# Secret\r\n"), "seed reparse target");
        Success(Testing::CreateFixtureJunction(root, linkDir, target), "create junction fixture");
        auto removeLink = wil::scope_exit([&] { (void)RemoveDirectoryW(linkDir.c_str()); });

        // Safe rename
        FileOperationOutcome renameOut;
        Success(ops.RenameItem(notePath, L"renamed.md", renameOut), "rename draft.md -> renamed.md");
        Require(std::filesystem::exists(renameOut.destinationPath) && ! std::filesystem::exists(notePath), "RenameItem moves file cleanly inside same folder");

        // Collision detection on Copy into subDir
        FileOperationOutcome collideOut;
        const HRESULT collideHr = ops.PasteItem(renameOut.destinationPath, subDir, DropEffectKind::Copy, false, false, collideOut);
        Require(FAILED(collideHr) && collideOut.collision, "PasteItem reports collision when destination already exists");

        // KeepBoth creates - Copy.md without overwriting existing file
        FileOperationOutcome copyBothOut;
        Success(ops.PasteItem(renameOut.destinationPath, subDir, DropEffectKind::Copy, false, true, copyBothOut), "PasteItem keepBothCopy creates Copy");
        Require(copyBothOut.destinationPath.filename() == L"renamed - Copy.md" && std::filesystem::exists(copyBothOut.destinationPath),
                "KeepBoth picks unique numbered Copy filename");

        // Replace overwrites destination on Move
        FileOperationOutcome replaceOut;
        Success(ops.PasteItem(renameOut.destinationPath, subDir, DropEffectKind::Move, true, false, replaceOut), "PasteItem Move with replaceExisting");
        Require(! std::filesystem::exists(renameOut.destinationPath) && std::filesystem::exists(existing),
                "Move with replaceExisting removes source and replaces target");

        // Reparse point refusal
        FileOperationOutcome reparseOut;
        const HRESULT repHr = ops.RenameItem(linkDir, L"moved-junction", reparseOut);
        Require(FAILED(repHr) && reparseOut.reparseRejected, "RenameItem refuses junction/reparse point");
        FileOperationOutcome repDestOut;
        const HRESULT repDestHr = ops.PasteItem(existing, linkDir, DropEffectKind::Copy, false, false, repDestOut);
        Require(FAILED(repDestHr) && repDestOut.reparseRejected, "PasteItem refuses reparse point destination directory");
    });

    test("Editor.FilesMultiFolderSectionsAndContextMenuOps",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto dir     = root / L"multi-folder-ctx";
        const auto rootOne = dir / L"RootOne" / L"Notes";
        const auto rootTwo = dir / L"RootTwo" / L"Notes";
        const auto subTwo  = rootTwo / L"Archive";
        std::filesystem::create_directories(rootOne);
        std::filesystem::create_directories(subTwo);
        const auto docOne = rootOne / L"one.md";
        const auto docTwo = rootTwo / L"two.md";
        Success(WriteBytesAtomic(docOne, "# One\r\n"), "seed one.md");
        Success(WriteBytesAtomic(docTwo, "# Two\r\n"), "seed two.md");
        Success(WriteBytesAtomic(subTwo / L"archived.md", "# Archived\r\n"), "seed archived.md");

        auto revealMock = std::make_shared<MockShellReveal>();
        std::wstring lastCopiedText;
        std::optional<FileClipboardPayload> mockDropClipboard;

        LaunchOptions launch;
        launch.headless               = true;
        launch.dataDirectory          = dir / L"appdata";
        launch.shellRevealForTest     = revealMock;
        launch.clipboardWriterForTest = [&](std::wstring_view text)
        {
            lastCopiedText = std::wstring(text);
            return S_OK;
        };
        launch.clipboardDropWriterForTest = [&](const FileClipboardPayload& payload)
        {
            mockDropClipboard = payload;
            return S_OK;
        };
        launch.clipboardDropReaderForTest = [&]() { return mockDropClipboard; };

        Application app(launch);
        Success(app.Initialize(), "initialize multi-folder app");
        MultiFolderFilesProbe::SelectRoot(app, rootOne);
        MultiFolderFilesProbe::AddRoot(app, rootTwo);
        Await(app, [&] {
            return ! MultiFolderFilesProbe::MultiFolderBusy(app) && MultiFolderFilesProbe::MultiFolderSnapshotCount(app) == 2u;
        }, "MultiFolderFilesService populates both local folder snapshots");

        // Verify disambiguated section headers in tree model
        const auto& rows            = MultiFolderFilesProbe::Model(app).rows;
        const bool hasRootOneHeader = std::ranges::any_of(rows, [](const auto& r) { return r.text == L"Notes (RootOne)"; });
        const bool hasRootTwoHeader = std::ranges::any_of(rows, [](const auto& r) { return r.text == L"Notes (RootTwo)"; });
        Require(hasRootOneHeader && hasRootTwoHeader, "multi-folder tree shows disambiguated collapsible section headers");

        // Open docOne and make it active
        MultiFolderFilesProbe::OpenFile(app, docOne);
        Await(app, [&] { return MultiFolderFilesProbe::Session(app).path == docOne; }, "open one.md from multi-folder tree");

        // Locate row for one.md and invoke context menu commands
        const auto findRowId = [&](std::wstring_view label)
        {
            std::string available;
            for (const auto& r : MultiFolderFilesProbe::Model(app).rows)
            {
                if (r.text == label)
                    return r.id;
                if (! available.empty())
                    available += ", ";
                std::string utf8;
                (void)DxUi::RichText::TryEncodeUtf8(r.text, utf8);
                available += utf8;
            }
            std::string targetUtf8;
            (void)DxUi::RichText::TryEncodeUtf8(label, targetUtf8);
            Require(false, "expected row '" + targetUtf8 + "' missing from files tree; rows=[" + available + "]");
            return std::uint64_t{};
        };
        const auto oneId = findRowId(L"one.md");
        MultiFolderFilesProbe::Model(app).OnTreeContextMenu(oneId, POINT{100, 100});
        app.Execute(Command::FilesCtxCopyFullPath);
        Require(lastCopiedText == docOne.lexically_normal().native(), "Copy Full Path copies canonical native path after OnTreeContextMenu");

        MultiFolderFilesProbe::SetPendingContextById(app, oneId);
        app.Execute(Command::FilesCtxCopyRelPath);
        Require(lastCopiedText == L"one.md", "Copy Relative Path copies path relative to owning root");

        // Modified documents row for a saved local file carries local path + root context for context menu actions
        app.Document().SetSelection({0, 0});
        Require(app.Document().ReplaceSelection(L"Edited "), "dirty one.md to populate Modified documents section");
        const auto isModifiedOneRow = [](const DxUi::TreeItemData& r) { return r.parentId == (std::uint64_t{1} << 63) && r.text == L"one.md"; };
        Await(app, [&] { return std::ranges::any_of(MultiFolderFilesProbe::Model(app).rows, isModifiedOneRow); }, "Modified documents shows dirty one.md row");
        const auto modOneIt = std::ranges::find_if(MultiFolderFilesProbe::Model(app).rows, isModifiedOneRow);
        MultiFolderFilesProbe::Model(app).OnTreeContextMenu(modOneIt->id, POINT{100, 100});
        app.Execute(Command::FilesCtxCopyRelPath);
        Require(lastCopiedText == L"one.md", "Modified documents WorkspaceDoc row resolves relative path for context menu");

        MultiFolderFilesProbe::SetPendingContextById(app, oneId);
        app.Execute(Command::FilesCtxRevealExplorer);
        MultiFolderFilesProbe::SetPendingContextById(app, oneId);
        app.Execute(Command::FilesCtxRevealRedSalamander);
        Await(app, [&] { return revealMock->calls.size() == 2u; }, "Reveal in Explorer & RedSalamander execute via background FileOpsService");
        Require(revealMock->calls[0].first == L"Explorer" && revealMock->calls[1].first == L"RedSalamander",
                "mock shell reveal received both Explorer and RedSalamander targets");

        // Rename active open file one.md -> one-renamed.md and verify workspace path rebases automatically
        MultiFolderFilesProbe::SetPendingContextById(app, oneId);
        app.Execute(Command::FilesCtxRename);
        Require(MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::RenameFileModal, "FilesCtxRename opens RenameFileModal");
        MultiFolderFilesProbe::RenameInput(app)->SetTextAndNotify(L"bad/name.md");
        Require(! MultiFolderFilesProbe::RenameError(app)->GetText().empty(), "invalid filename shows validation error live");
        MultiFolderFilesProbe::RenameInput(app)->SetTextAndNotify(L"one-renamed.md");
        app.Execute(Command::RenameConfirm);
        const auto renamedOne = rootOne / L"one-renamed.md";
        Await(app,
              [&]
        {
            return std::filesystem::exists(renamedOne) && MultiFolderFilesProbe::Session(app).path == renamedOne &&
                   std::ranges::any_of(MultiFolderFilesProbe::Model(app).rows, [](const auto& r) { return r.text == L"one-renamed.md"; });
        },
              "Rename completes off UI thread, rebases active open workspace path, and refreshes files tree");

        // Copy renamed file and Paste into subTwo, then verify copied file arrives
        const auto renamedId = findRowId(L"one-renamed.md");
        MultiFolderFilesProbe::SetPendingContextById(app, renamedId);
        app.Execute(Command::FilesCtxCopy);
        Require(mockDropClipboard && mockDropClipboard->effect == DropEffectKind::Copy && mockDropClipboard->paths.size() == 1u,
                "FilesCtxCopy stages FileClipboardPayload");
        const auto archiveId = findRowId(L"Archive");
        MultiFolderFilesProbe::SetPendingContextById(app, archiveId);
        app.Execute(Command::FilesCtxPaste);
        Await(app, [&] { return std::filesystem::exists(subTwo / L"one-renamed.md"); }, "FilesCtxPaste copies file into target directory");

        // Remove RootTwo and confirm single-root mode resumes
        MultiFolderFilesProbe::RemoveRoot(app, rootTwo);
        Require(app.Preferences().filesRoots.size() == 1u && app.Preferences().filesRoots[0] == rootOne, "RemoveFilesRoot updates filesRoots cleanly");
        Require(foreground.NeverAcquiredForeground(), "multi-folder and context menu ops never take desktop focus");
    });

    test("Files.DirectoryPasteSelfDescendantAndNestedReparseRefusal",
         [&]
    {
        WindowsWorkspaceFileOps ops;
        Require(ops.ValidateFileName(L"CON .md") == FileNameValidationError::ReservedName,
                "ValidateFileName trims trailing spaces before dot for DOS reserved names");
        Require(IsMarkdownFileName(L"notes/README.MD") && ! IsMarkdownFileName(L"notes/image.png"),
                "IsMarkdownFileName classifies leaf extensions without path allocation");

        const auto workDir     = root / L"dir-paste-guards";
        const auto srcDir      = workDir / L"SourceFolder";
        const auto srcSub      = srcDir / L"Sub";
        const auto destParent  = workDir / L"DestParent";
        const auto secretDir   = root / L"dir-paste-secret-target";
        const auto nestedJunc  = srcDir / L"NestedLink";
        const auto destJunc    = destParent / L"JunctionDest.md";
        const auto destDirJunc = destParent / L"SourceFolder";
        std::filesystem::create_directories(srcSub);
        std::filesystem::create_directories(destParent);
        std::filesystem::create_directories(secretDir);
        Success(WriteBytesAtomic(srcDir / L"top.md", "# Top\r\n"), "seed top.md");
        Success(WriteBytesAtomic(srcSub / L"nested.txt", "Nested text"), "seed nested.txt");
        Success(WriteBytesAtomic(srcSub / L"ignored.bin", "binary-payload"), "seed ignored.bin");
        Success(WriteBytesAtomic(secretDir / L"secret.md", "# Secret\r\n"), "seed secret.md");

        // Self-paste and descendant-paste (including case difference) must fail with E_INVALIDARG.
        FileOperationOutcome selfOut;
        const HRESULT selfHr = ops.PasteItem(srcDir, srcDir, DropEffectKind::Copy, false, false, selfOut);
        Require(selfHr == E_INVALIDARG, "PasteItem refuses copying directory into itself");

        std::wstring upperSub = srcSub.native();
        for (auto& ch : upperSub)
            if (ch >= L'a' && ch <= L'z')
                ch = static_cast<wchar_t>(ch - L'a' + L'A');
        FileOperationOutcome descOut;
        const HRESULT descHr = ops.PasteItem(srcDir, std::filesystem::path(upperSub), DropEffectKind::Copy, false, false, descOut);
        Require(descHr == E_INVALIDARG, "PasteItem refuses copying directory into a case-differing descendant");

        // Nested junction inside srcDir must refuse before creating partial destination files.
        Success(Testing::CreateFixtureJunction(root, nestedJunc, secretDir), "create nested junction");
        {
            auto removeNested = wil::scope_exit([&] { (void)RemoveDirectoryW(nestedJunc.c_str()); });
            FileOperationOutcome nestedOut;
            const HRESULT nestedHr = ops.PasteItem(srcDir, destParent, DropEffectKind::Copy, false, false, nestedOut);
            Require(FAILED(nestedHr) && nestedOut.reparseRejected && ! std::filesystem::exists(destParent / L"SourceFolder"),
                    "PasteItem refuses directory containing nested junction and writes zero partial destination files");
        }

        // Destination surrogate reparse point must be refused before collision or overwrite.
        Success(Testing::CreateFixtureJunction(root, destJunc, secretDir), "create dest file-named junction");
        {
            auto removeDestJunc = wil::scope_exit([&] { (void)RemoveDirectoryW(destJunc.c_str()); });
            const auto srcFile  = workDir / L"JunctionDest.md";
            Success(WriteBytesAtomic(srcFile, "# New\r\n"), "seed srcFile");
            FileOperationOutcome repDestOut;
            const HRESULT repDestHr = ops.PasteItem(srcFile, destParent, DropEffectKind::Copy, true, false, repDestOut);
            Require(FAILED(repDestHr) && repDestOut.reparseRejected, "PasteItem refuses destination surrogate reparse before overwrite");
        }
        Success(Testing::CreateFixtureJunction(root, destDirJunc, secretDir), "create dest dir junction");
        {
            auto removeDestDirJunc = wil::scope_exit([&] { (void)RemoveDirectoryW(destDirJunc.c_str()); });
            FileOperationOutcome repDirOut;
            const HRESULT repDirHr = ops.PasteItem(srcDir, destParent, DropEffectKind::Copy, true, false, repDirOut);
            Require(FAILED(repDirHr) && repDirOut.reparseRejected && ! std::filesystem::exists(secretDir / L"top.md"),
                    "PasteItem refuses directory copy onto destination junction");
        }

        // Clean bounded directory copy filters out .bin and copies .md/.txt.
        FileOperationOutcome copyOut;
        Success(ops.PasteItem(srcDir, destParent, DropEffectKind::Copy, false, false, copyOut), "bounded directory copy succeeds");
        Require(copyOut.destinationIsDirectory && std::filesystem::exists(destParent / L"SourceFolder" / L"top.md") &&
                    std::filesystem::exists(destParent / L"SourceFolder" / L"Sub" / L"nested.txt") &&
                    ! std::filesystem::exists(destParent / L"SourceFolder" / L"Sub" / L"ignored.bin"),
                "bounded directory copy preserves .md/.txt files and skips .bin");

        // Pre-cancelled directory copy stops cleanly with ERROR_CANCELLED.
        std::atomic_bool cancel{true};
        FileOperationOutcome cancelOut;
        const HRESULT cancelHr = ops.PasteItem(srcDir, workDir / L"CancelDest", DropEffectKind::Copy, false, false, cancelOut, &cancel);
        Require(FAILED(cancelHr), "cancelled directory paste fails cleanly");

        // ParseDropFilesBuffer bounds, alignment, and DROPEFFECT_COPY precedence.
        std::vector<std::byte> smallBuf(sizeof(DROPFILES) - 1u);
        FileClipboardPayload parsedPayload;
        Require(ParseDropFilesBuffer(smallBuf, std::nullopt, parsedPayload) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                "ParseDropFilesBuffer rejects undersized buffer");

        const std::wstring samplePath = (srcDir / L"top.md").native();
        std::vector<std::byte> dropBuf(sizeof(DROPFILES) + (samplePath.size() + 2u) * sizeof(wchar_t) + 4u, std::byte{0});
        DROPFILES df{};
        df.pFiles = sizeof(DROPFILES) - 2u; // < sizeof(DROPFILES)
        df.fWide  = TRUE;
        std::memcpy(dropBuf.data(), &df, sizeof(df));
        Require(ParseDropFilesBuffer(dropBuf, std::nullopt, parsedPayload) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                "ParseDropFilesBuffer rejects pFiles < sizeof(DROPFILES)");

        df.pFiles = sizeof(DROPFILES) + 1u; // misaligned
        std::memcpy(dropBuf.data(), &df, sizeof(df));
        Require(ParseDropFilesBuffer(dropBuf, std::nullopt, parsedPayload) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                "ParseDropFilesBuffer rejects misaligned pFiles");

        df.pFiles = sizeof(DROPFILES);
        df.fWide  = FALSE;
        std::memcpy(dropBuf.data(), &df, sizeof(df));
        Require(ParseDropFilesBuffer(dropBuf, std::nullopt, parsedPayload) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                "ParseDropFilesBuffer rejects non-wide DROPFILES");

        df.fWide = TRUE;
        std::memcpy(dropBuf.data(), &df, sizeof(df));
        std::memcpy(dropBuf.data() + sizeof(DROPFILES), samplePath.data(), samplePath.size() * sizeof(wchar_t));
        Success(ParseDropFilesBuffer(dropBuf, DROPEFFECT_COPY | DROPEFFECT_MOVE, parsedPayload),
                "ParseDropFilesBuffer parses valid buffer with combined drop effects");
        Require(parsedPayload.effect == DropEffectKind::Copy && parsedPayload.paths.size() == 1u, "DROPEFFECT_COPY takes precedence over DROPEFFECT_MOVE");
        Success(ParseDropFilesBuffer(dropBuf, DROPEFFECT_MOVE, parsedPayload), "ParseDropFilesBuffer parses move-only drop effect");
        Require(parsedPayload.effect == DropEffectKind::Move, "DROPEFFECT_MOVE alone selects Move");
    });

    test("Files.CaseOnlyRenameAndServiceLifecycle",
         [&]
    {
        WindowsWorkspaceFileOps ops;
        const auto workDir  = root / L"case-rename-lifecycle";
        const auto notePath = workDir / L"note.md";
        std::filesystem::create_directories(workDir);
        Success(WriteBytesAtomic(notePath, "# Case test\r\n"), "seed note.md");

        auto readFileId = [](const std::filesystem::path& p, FILE_ID_INFO& out)
        {
            const wil::unique_hfile h(CreateFileW(p.c_str(),
                                                  FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
            Require(static_cast<bool>(h), "open file for FileIdInfo check");
            return GetFileInformationByHandleEx(h.get(), FileIdInfo, &out, sizeof(out)) != FALSE;
        };

        FILE_ID_INFO beforeId{};
        const bool hasFileId = readFileId(notePath, beforeId);
        FileOperationOutcome caseOut;
        Success(ops.RenameItem(notePath, L"NOTE.md", caseOut), "case-only rename note.md -> NOTE.md");
        Require(! caseOut.destinationIsDirectory && caseOut.destinationPath.filename().native() == L"NOTE.md",
                "case-only rename updates filename casing and reports non-directory");
        if (hasFileId)
        {
            FILE_ID_INFO afterId{};
            Require(readFileId(caseOut.destinationPath, afterId) && std::memcmp(&beforeId, &afterId, sizeof(FILE_ID_INFO)) == 0,
                    "case-only rename preserves FILE_ID_INFO identity");
        }

        FileOperationOutcome sameOut;
        Success(ops.RenameItem(caseOut.destinationPath, L"NOTE.md", sameOut), "same-case no-op NOTE.md -> NOTE.md");
        Require(! sameOut.collision && ! sameOut.destinationIsDirectory, "same-case no-op preserves non-directory without collision");

        const auto myFolder = workDir / L"MyFolder";
        std::filesystem::create_directories(myFolder);
        FileOperationOutcome dirCaseOut;
        Success(ops.RenameItem(myFolder, L"myfolder", dirCaseOut), "directory case-only rename MyFolder -> myfolder");
        Require(! dirCaseOut.collision && dirCaseOut.destinationIsDirectory, "directory case-only rename reports directory without collision");
        WIN32_FIND_DATAW dirFind{};
        const wil::unique_hfind dirSearch(FindFirstFileW(dirCaseOut.destinationPath.c_str(), &dirFind));
        Require(static_cast<bool>(dirSearch) && std::wstring_view(dirFind.cFileName) == L"myfolder",
                "directory case-only rename updates on-disk folder leaf casing");

        const auto link1 = workDir / L"link1.md";
        const auto link2 = workDir / L"link2.md";
        Success(WriteBytesAtomic(link1, "# Hardlink\r\n"), "seed link1.md");
        Require(CreateHardLinkW(link2.c_str(), link1.c_str(), nullptr) != FALSE, "create hardlink link2.md");
        FileOperationOutcome hlOut;
        const HRESULT hlHr = ops.RenameItem(link1, L"link2.md", hlOut);
        Require(hlHr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) && hlOut.collision && std::filesystem::exists(link1) && std::filesystem::exists(link2),
                "RenameItem detects same-directory hardlink collision (link1.md -> link2.md)");
        FileOperationOutcome hlUpperOut;
        const HRESULT hlUpperHr = ops.RenameItem(link1, L"LINK2.md", hlUpperOut);
        Require(hlUpperHr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) && hlUpperOut.collision && std::filesystem::exists(link1) && std::filesystem::exists(link2),
                "RenameItem detects same-directory hardlink collision across case (link1.md -> LINK2.md)");

        // Conditional case-sensitive directory collision check (when NTFS/privileges support FileCaseSensitiveInfo).
        const auto csDir = workDir / L"cs-dir";
        std::filesystem::create_directories(csDir);
        {
            const wil::unique_hfile csHandle(CreateFileW(csDir.c_str(),
                                                         FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
                                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                         nullptr,
                                                         OPEN_EXISTING,
                                                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                         nullptr));
            if (csHandle)
            {
                FILE_CASE_SENSITIVE_INFO csInfo{FILE_CS_FLAG_CASE_SENSITIVE_DIR};
                if (SetFileInformationByHandle(csHandle.get(), FileCaseSensitiveInfo, &csInfo, sizeof(csInfo)))
                {
                    const auto lowerFile = csDir / L"readme.md";
                    const auto upperFile = csDir / L"README.md";
                    Success(WriteBytesAtomic(lowerFile, "lower"), "seed lowercase readme.md");
                    Success(WriteBytesAtomic(upperFile, "UPPER"), "seed uppercase README.md");
                    FileOperationOutcome csRenameOut;
                    const HRESULT csHr = ops.RenameItem(lowerFile, L"README.md", csRenameOut);
                    Require(FAILED(csHr) && csRenameOut.collision && std::filesystem::exists(lowerFile),
                            "RenameItem detects distinct object collision in case-sensitive directory instead of replacing");

                    const auto csAlpha      = csDir / L"alpha.md";
                    const auto csAlphaUpper = csDir / L"ALPHA.md";
                    Success(WriteBytesAtomic(csAlpha, "alpha"), "seed case-sensitive alpha.md");
                    Require(CreateHardLinkW(csAlphaUpper.c_str(), csAlpha.c_str(), nullptr) != FALSE, "create case-sensitive hardlink ALPHA.md");
                    FileOperationOutcome csHlOut;
                    const HRESULT csHlHr = ops.RenameItem(csAlpha, L"ALPHA.md", csHlOut);
                    Require(csHlHr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) && csHlOut.collision && std::filesystem::exists(csAlpha) &&
                                std::filesystem::exists(csAlphaUpper),
                            "RenameItem detects hardlink collision in case-sensitive directory instead of unlinking");
                }
            }
        }

        // Directory replace Move fallback refuses unsupported .bin files without deleting source.
        const auto moveSrc        = workDir / L"MoveSrc";
        const auto moveDestParent = workDir / L"MoveDestParent";
        const auto moveDest       = moveDestParent / L"MoveSrc";
        std::filesystem::create_directories(moveSrc);
        std::filesystem::create_directories(moveDest);
        Success(WriteBytesAtomic(moveSrc / L"keep.md", "# Keep\r\n"), "seed moveSrc keep.md");
        Success(WriteBytesAtomic(moveSrc / L"payload.bin", "bin"), "seed moveSrc payload.bin");
        Success(WriteBytesAtomic(moveDest / L"old.md", "# Old\r\n"), "seed moveDest old.md");
        FileOperationOutcome moveRefusedOut;
        const HRESULT moveRefusedHr = ops.PasteItem(moveSrc, moveDestParent, DropEffectKind::Move, true, false, moveRefusedOut);
        Require(moveRefusedHr == HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE) && std::filesystem::exists(moveSrc / L"payload.bin"),
                "directory replace Move refuses unsupported .bin without deleting source");
        std::filesystem::remove(moveSrc / L"payload.bin");
        FileOperationOutcome moveOkOut;
        Success(ops.PasteItem(moveSrc, moveDestParent, DropEffectKind::Move, true, false, moveOkOut),
                "directory replace Move succeeds when all files are supported");
        Require(moveOkOut.destinationIsDirectory && ! std::filesystem::exists(moveSrc) && std::filesystem::exists(moveDest / L"keep.md"),
                "directory replace Move replaces dest and deletes source");

        // FileOpsService::Stop() must settle Busy()==false and post FileOpReady even when blocked.
        class BlockingOps final : public IWorkspaceFileOps
        {
        public:
            BlockingOps()                              = default;
            BlockingOps(const BlockingOps&)            = delete;
            BlockingOps& operator=(const BlockingOps&) = delete;
            BlockingOps(BlockingOps&&)                 = delete;
            BlockingOps& operator=(BlockingOps&&)      = delete;
            wil::unique_event entered{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
            wil::unique_event release{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
            [[nodiscard]] FileNameValidationError ValidateFileName(std::wstring_view) const noexcept override
            {
                return FileNameValidationError::None;
            }
            [[nodiscard]] HRESULT RenameItem(const std::filesystem::path&, std::wstring_view, FileOperationOutcome&) noexcept override
            {
                return S_OK;
            }
            [[nodiscard]] HRESULT PasteItem(const std::filesystem::path&,
                                            const std::filesystem::path&,
                                            DropEffectKind,
                                            bool,
                                            bool,
                                            FileOperationOutcome&,
                                            const std::atomic_bool*) noexcept override
            {
                (void)SetEvent(entered.get());
                (void)WaitForSingleObject(release.get(), 5000u);
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
        };
        const wil::unique_hwnd msgWnd(CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr));
        Require(static_cast<bool>(msgWnd), "create message-only HWND for FileOpsService::Stop test");
        auto blockingOps = std::make_shared<BlockingOps>();
        FileOpsService svc;
        constexpr UINT FileOpMsg = WM_APP + 414;
        Success(svc.Initialize(msgWnd.get(), FileOpMsg, std::make_shared<MockShellReveal>(), blockingOps), "init FileOpsService");
        WorkspaceFileOpRequest blockReq;
        blockReq.kind      = WorkspaceFileOpRequest::Kind::Paste;
        blockReq.requestId = 1u;
        Success(svc.Submit(std::move(blockReq)), "submit blocking op");
        Require(WaitForSingleObject(blockingOps->entered.get(), 5000u) == WAIT_OBJECT_0, "worker entered blocking op");
        svc.Stop();
        (void)SetEvent(blockingOps->release.get());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        bool sawMessage{};
        while (svc.Busy() || ! sawMessage)
        {
            MSG msg{};
            while (PeekMessageW(&msg, msgWnd.get(), FileOpMsg, FileOpMsg, PM_REMOVE))
                sawMessage = true;
            if (! svc.Busy() && sawMessage)
                break;
            Require(std::chrono::steady_clock::now() < deadline, "FileOpsService::Stop must settle Busy()==false and post FileOpReady");
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10u, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
        svc.Close();

        // MultiFolderFilesService::SetRoots dedup + atomic rollback on partial failure.
        const auto rootA = workDir / L"FolderA" / L"docs";
        const auto rootB = workDir / L"FolderB" / L"DOCS";
        const auto rootC = workDir / L"FolderC" / L"extra";
        std::filesystem::create_directories(rootA);
        std::filesystem::create_directories(rootB);
        std::filesystem::create_directories(rootC);
        const std::vector<std::filesystem::path> trailingRoots{
            std::filesystem::path(rootA.native() + L"\\"),
            std::filesystem::path(rootB.native() + L"/"),
        };
        const auto labels = DisambiguateFolderLabels(trailingRoots);
        Require(labels.size() == 2u && labels[0] == L"docs (FolderA)" && labels[1] == L"DOCS (FolderB)",
                "DisambiguateFolderLabels handles trailing slashes and ordinal case");

        MultiFolderFilesService multi;
        Success(multi.Initialize(nullptr, 0u), "init MultiFolderFilesService");
        const std::vector<std::filesystem::path> dupRoots{rootA, std::filesystem::path(rootA.native() + L"\\"), rootB};
        Success(multi.SetRoots(dupRoots), "SetRoots deduplicates trailing-slash variants");
        Require(multi.RootCount() == 2u, "SetRoots keeps exactly 2 unique roots");
        multi.Close();

        FilesService failProbe;
        failProbe.FailNextInitializeForTest(E_FAIL);
        Require(failProbe.Initialize(nullptr, 0u) == E_FAIL, "FilesService test failure injection works");

        const std::vector<std::filesystem::path> workRoots{workDir};
        const auto dirtyLoc = ResolveWorkspaceDisplayLocation(workDir / L"evil\u202Edir" / L"note\u200B.md", {}, workRoots, L"Untitled.md");
        Require(dirtyLoc.fullPath.find(L'\u202E') == std::wstring::npos && dirtyLoc.relativePath.find(L'\u200B') == std::wstring::npos,
                "ResolveWorkspaceDisplayLocation sanitizes fullPath and relativePath");
    });

    test("Editor.FilesCutPasteConflictAndNavigationPolish",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto dir     = root / L"nav-polish";
        const auto rootOne = dir / L"RootOne" / L"Notes";
        const auto rootTwo = dir / L"RootTwo" / L"Notes";
        const auto subOne  = rootOne / L"MovedHere";
        const auto subTwo  = rootTwo / L"Archive";
        std::filesystem::create_directories(subOne);
        std::filesystem::create_directories(subTwo);
        const auto docOne = rootOne / L"one.md";
        const auto docB   = rootOne / L"alpha.md";
        const auto docC   = rootOne / L"beta.md";
        const auto docD   = rootOne / L"gamma.md";
        Success(WriteBytesAtomic(docOne, "# One\r\n\r\n## Sub\r\n"), "seed one.md");
        Success(WriteBytesAtomic(docB, "# Alpha\r\n"), "seed alpha.md");
        Success(WriteBytesAtomic(docC, "# Beta\r\n"), "seed beta.md");
        Success(WriteBytesAtomic(docD, "# Gamma\r\n"), "seed gamma.md");
        Success(WriteBytesAtomic(subOne / L"anchor.md", "# Anchor\r\n"), "seed anchor.md");
        Success(WriteBytesAtomic(subTwo / L"archived.md", "# Archived\r\n"), "seed archived.md");

        std::optional<FileClipboardPayload> mockDropClipboard;
        std::wstring lastCopiedText;
        unsigned clipboardReadCount{};

        LaunchOptions launch;
        launch.headless               = true;
        launch.dataDirectory          = dir / L"appdata";
        launch.shellRevealForTest     = std::make_shared<MockShellReveal>();
        launch.clipboardWriterForTest = [&](std::wstring_view text)
        {
            lastCopiedText = std::wstring(text);
            return S_OK;
        };
        launch.clipboardDropWriterForTest = [&](const FileClipboardPayload& payload)
        {
            mockDropClipboard = payload;
            return S_OK;
        };
        launch.clipboardDropReaderForTest = [&]()
        {
            ++clipboardReadCount;
            return mockDropClipboard;
        };

        Application app(launch);
        Success(app.Initialize(), "initialize nav-polish app");
        MultiFolderFilesProbe::Resize(app, 1280u, 820u);

        // 1. Single-folder mode: Split-Source active cut + paste rebase + Cut icon + _folderFiles refresh
        MultiFolderFilesProbe::SelectRoot(app, rootOne);
        Await(app, [&] {
            return ! MultiFolderFilesProbe::FolderFiles(app).Busy() && ! MultiFolderFilesProbe::Model(app).rows.empty();
        }, "single-folder tree populated");
        MultiFolderFilesProbe::OpenFile(app, docOne);
        Await(app, [&] { return MultiFolderFilesProbe::Session(app).path == docOne; }, "open one.md");
        app.Document().SetSelection({0, 0});
        Require(app.Document().ReplaceSelection(L"Dirty "), "dirty one.md before Split-Source Cut+Paste");
        MultiFolderFilesProbe::SetSplitSourceActive(app);

        auto findRowId = [&](std::wstring_view label)
        {
            std::string available;
            for (const auto& r : MultiFolderFilesProbe::Model(app).rows)
            {
                if (r.text == label)
                    return r.id;
                if (! available.empty())
                    available += ", ";
                std::string utf8;
                (void)DxUi::RichText::TryEncodeUtf8(r.text, utf8);
                available += utf8;
            }
            std::string targetUtf8;
            (void)DxUi::RichText::TryEncodeUtf8(label, targetUtf8);
            Require(false, "expected row '" + targetUtf8 + "' missing from files tree; rows=[" + available + "]");
            return std::uint64_t{};
        };
        const auto oneId = findRowId(L"one.md");
        MultiFolderFilesProbe::ShowContextMenu(app, oneId);
        app.Execute(Command::FilesCtxCut);
        std::size_t cutOneRows{};
        for (const auto& r : MultiFolderFilesProbe::Model(app).rows)
        {
            if (r.text == L"one.md")
            {
                Require(r.iconText == L"\xE8C6", "single-folder mode renders Cut scissor icon on cut file");
                ++cutOneRows;
            }
        }
        Require(cutOneRows >= 2u, "both Modified documents and single-folder LocalEntry rows render Cut scissor icon");

        // Verify NavigationModel::FindVisibleItemById directly
        Require(MultiFolderFilesProbe::Model(app).FindVisibleItemById(oneId).has_value() &&
                    ! MultiFolderFilesProbe::Model(app).FindVisibleItemById(0xDEADBEEFull).has_value(),
                "NavigationModel::FindVisibleItemById locates present rows and rejects missing ids");

        const auto movedHereId = findRowId(L"MovedHere");
        MultiFolderFilesProbe::ShowContextMenu(app, movedHereId);
        app.Execute(Command::FilesCtxPaste);
        const auto movedDocOne = subOne / L"one.md";
        Await(app,
              [&]
        {
            return std::filesystem::exists(movedDocOne) && ! std::filesystem::exists(docOne) && MultiFolderFilesProbe::Session(app).path == movedDocOne &&
                   ! MultiFolderFilesProbe::FolderFiles(app).Busy();
        },
              "Cut+Paste moves one.md into MovedHere and rebases session path");
        Require(MultiFolderFilesProbe::ActiveEditor(app)->GetDocumentFilePath() == movedDocOne.native() &&
                    MultiFolderFilesProbe::MirrorEditor(app)->GetDocumentFilePath() == movedDocOne.native() &&
                    MultiFolderFilesProbe::TitleLabel(app)->GetText() == L"one.md" && MultiFolderFilesProbe::CutFilePaths(app).empty(),
                "Split-Source Cut+Paste updates both editors, title label, and clears cut set");
        wchar_t windowCaption[256]{};
        (void)GetWindowTextW(app.Window(), windowCaption, static_cast<int>(std::size(windowCaption)));
        Require(std::wstring_view(windowCaption).find(L"one.md") != std::wstring_view::npos, "window caption updated after Cut+Paste rebase");

        // 2. Cut A (non-empty _cutFilePaths), then Copy {B, C} onto subOne where B already exists -> Conflict Replace preserves B source and copies C!
        Success(WriteBytesAtomic(subOne / L"alpha.md", "# Existing Alpha\r\n"), "seed existing alpha.md collision");
        const auto movedOneId = findRowId(L"one.md");
        MultiFolderFilesProbe::ShowContextMenu(app, movedOneId);
        app.Execute(Command::FilesCtxCut);
        Require(! MultiFolderFilesProbe::CutFilePaths(app).empty(), "_cutFilePaths populated before Copy paste test");
        mockDropClipboard = FileClipboardPayload{{docB, docC}, DropEffectKind::Copy};
        MultiFolderFilesProbe::ShowContextMenu(app, findRowId(L"MovedHere"));
        app.Execute(Command::FilesCtxPaste);
        Await(app, [&] {
            return MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::FileConflictModal;
        }, "Copy onto existing alpha.md opens FileConflictModal");
        Require(! MultiFolderFilesProbe::FileConflictDesc(app)->GetText().empty(), "FileConflictModal description populated");
        app.Execute(Command::FileConflictReplace);
        Await(app, [&] {
            return std::filesystem::exists(subOne / L"beta.md") && MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::None;
        }, "FileConflictReplace finishes queued multi-file Copy");
        Require(std::filesystem::exists(docB) && std::filesystem::exists(docC) && ! MultiFolderFilesProbe::CutFilePaths(app).empty(),
                "DropEffectKind::Copy does NOT delete source files or clear _cutFilePaths");

        // KeepBoth + multi-file queue
        mockDropClipboard = FileClipboardPayload{{docB, docD}, DropEffectKind::Copy};
        MultiFolderFilesProbe::BeginCopyOrMove(app, {docB, docD}, subOne, false);
        Await(app, [&] {
            return MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::FileConflictModal;
        }, "second batch opens FileConflictModal");
        app.Execute(Command::FileConflictKeepBoth);
        Await(app, [&] {
            return std::filesystem::exists(subOne / L"alpha - Copy.md") && std::filesystem::exists(subOne / L"gamma.md");
        }, "FileConflictKeepBoth copies alpha - Copy.md and continues queued gamma.md");

        // Cancel conflict clears remaining pending queue
        mockDropClipboard = FileClipboardPayload{{docB, docD}, DropEffectKind::Copy};
        std::filesystem::remove(subOne / L"gamma.md");
        MultiFolderFilesProbe::BeginCopyOrMove(app, {docB, docD}, subOne, false);
        Await(app, [&] {
            return MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::FileConflictModal;
        }, "third batch opens FileConflictModal");
        app.Execute(Command::CloseDrawer);
        Require(MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::None, "CloseDrawer dismisses FileConflictModal");
        app.Prepare(32);
        Require(! std::filesystem::exists(subOne / L"gamma.md"), "cancelling conflict modal clears remaining paste queue");

        // Preempting FileConflictModal via SetActiveOverlay clears remaining pending queue
        mockDropClipboard = FileClipboardPayload{{docB, docD}, DropEffectKind::Copy};
        MultiFolderFilesProbe::BeginCopyOrMove(app, {docB, docD}, subOne, false);
        Await(app, [&] {
            return MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::FileConflictModal;
        }, "fourth batch opens FileConflictModal before SetActiveOverlay preemption");
        MultiFolderFilesProbe::SetOverlay(app, Application::OverlayKind::SettingsDrawer);
        Require(MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::SettingsDrawer,
                "SetActiveOverlay preempts FileConflictModal to SettingsDrawer");
        Require(MultiFolderFilesProbe::PendingPasteQueueEmpty(app), "SetActiveOverlay preemption clears pending paste queue");
        app.Execute(Command::CloseDrawer);
        Require(MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::None, "CloseDrawer dismisses preempting SettingsDrawer");
        app.Prepare(32);
        Require(! std::filesystem::exists(subOne / L"gamma.md"), "preempting FileConflictModal via SetActiveOverlay leaves gamma.md uncopied");

        // 3. Collapsed Modified documents cleanup when last dirty doc becomes clean
        MultiFolderFilesProbe::SetWorkspaceCollapsed(app, true);
        Require(std::ranges::any_of(MultiFolderFilesProbe::Model(app).rows, [](const auto& r) { return r.id == (std::uint64_t{1} << 63); }),
                "collapsed Modified documents header visible while active doc is dirty");
        app.Execute(Command::Undo);
        Require(std::ranges::none_of(MultiFolderFilesProbe::Model(app).rows, [](const auto& r) { return r.id == (std::uint64_t{1} << 63); }),
                "cleaning last dirty doc removes collapsed Modified documents header");
        MultiFolderFilesProbe::SetWorkspaceCollapsed(app, false);

        // 4. Multi-folder refresh preserves expanded subfolders while new folders start collapsed
        MultiFolderFilesProbe::AddRoot(app, rootTwo);
        Await(app, [&] {
            return ! MultiFolderFilesProbe::MultiFolderBusy(app) && MultiFolderFilesProbe::MultiFolderSnapshotCount(app) == 2u;
        }, "multi-folder populated");
        const auto archiveId = findRowId(L"Archive");
        MultiFolderFilesProbe::Model(app).OnTreeToggleExpanded(archiveId, true);
        Require(! MultiFolderFilesProbe::CollapsedFolders(app).contains(subTwo), "Archive expanded before refresh");
        const auto newSub = rootTwo / L"BrandNewSub";
        std::filesystem::create_directories(newSub);
        Success(WriteBytesAtomic(newSub / L"fresh.md", "# Fresh\r\n"), "seed BrandNewSub/fresh.md");
        (void)MultiFolderFilesProbe::MultiFolderFiles(app).RefreshAll();
        Await(app,
              [&]
        {
            return ! MultiFolderFilesProbe::MultiFolderBusy(app) &&
                   std::ranges::any_of(MultiFolderFilesProbe::Model(app).rows, [](const auto& r) { return r.text == L"BrandNewSub"; });
        },
              "multi-folder refresh picks up BrandNewSub");
        Require(! MultiFolderFilesProbe::CollapsedFolders(app).contains(subTwo) && MultiFolderFilesProbe::CollapsedFolders(app).contains(newSub),
                "multi-folder refresh preserves expanded Archive and collapses BrandNewSub");

        // 5. WorkspaceDoc F2 / GitHub Paste refusal / OnMenuClosed focus preservation
        app.Document().SetSelection({0, 0});
        Require(app.Document().ReplaceSelection(L"DirtyAgain "), "dirty active doc for WorkspaceDoc test");
        const auto modRowIt =
            std::ranges::find_if(MultiFolderFilesProbe::Model(app).rows, [](const auto& r) { return r.parentId == (std::uint64_t{1} << 63); });
        Require(modRowIt != MultiFolderFilesProbe::Model(app).rows.end(), "Modified documents row present");
        MultiFolderFilesProbe::FilesTree(app)->SetSelectedItemId(modRowIt->id);
        app.Host().SetFocusControl(MultiFolderFilesProbe::FilesTree(app), false);
        Require(MultiFolderFilesProbe::SendKeyToFocused(app, VK_F2) && MultiFolderFilesProbe::ActiveOverlay(app) == Application::OverlayKind::RenameFileModal,
                "VK_F2 on local WorkspaceDoc opens RenameFileModal instead of LocalSource");
        app.Execute(Command::CloseDrawer);

        app.Host().SetFocusControl(MultiFolderFilesProbe::FilesTree(app), false);
        MultiFolderFilesProbe::CallOnMenuClosed(app, std::nullopt, MultiFolderFilesProbe::FilesTree(app));
        Require(app.Host().GetFocusControl() == MultiFolderFilesProbe::FilesTree(app),
                "OnMenuClosed keeps focus in _filesTree for non-opening context menu dismissal");

        // GitHub row refuses Paste & ActivePasteTargetDirectory returns empty
        GitHubTreeEntry ghFile;
        ghFile.path = L"docs/guide.md";
        ghFile.name = L"guide.md";
        MultiFolderFilesProbe::AddConnectedGitHubRepo(app, L"RedSalamanders", L"RedTilde", L"main", {ghFile});
        MultiFolderFilesProbe::RefreshFiles(app);
        const auto ghId = findRowId(L"guide.md");
        MultiFolderFilesProbe::FilesTree(app)->SetSelectedItemId(ghId);
        Require(MultiFolderFilesProbe::ActivePasteTarget(app).empty(), "ActivePasteTargetDirectory returns empty for GitHub row");
        const auto readsBeforeGhPaste = clipboardReadCount;
        MultiFolderFilesProbe::ShowContextMenu(app, ghId);
        app.Execute(Command::FilesCtxPaste);
        Require(clipboardReadCount == readsBeforeGhPaste, "FilesCtxPaste refuses GitHub target without reading clipboard");

        // 6. SidePaneRail reducedMotion StartCollapse, cap==0, bottom +N, VK_DOWN, PinOutline focus, PinFiles, hover collapse, GitHub dots
        auto theme          = app.Host().GetTheme();
        theme.reducedMotion = true;
        app.Host().SetTheme(theme);
        bool stepCalled{}, settledCalled{};
        CollapsedPaneRail testRail(PaneSide::Right);
        testRail.onAnimationStep   = [&] { stepCalled = true; };
        testRail.onCollapseSettled = [&] { settledCalled = true; };
        testRail.SnapToMode(PaneDisplayMode::HoverOverlay, true);
        testRail.StartCollapse(app.Host(), true);
        Require(stepCalled && settledCalled && ! testRail.State().collapseFromOverlay && testRail.State().mode == PaneDisplayMode::CollapsedRail,
                "reducedMotion StartCollapse invokes callbacks and clears collapseFromOverlay");

        // Short rail cap == 0
        testRail.SetBounds(D2D1::RectF(0.f, 0.f, 28.f, 48.f));
        std::vector<RailDotItem> manyDots(10u);
        for (std::size_t i = 0u; i < manyDots.size(); ++i)
        {
            manyDots[i].id   = i;
            manyDots[i].name = L"H" + std::to_wstring(i);
        }
        testRail.SetVisible(true);
        testRail.SetItems(manyDots);
        Require(testRail.VisibleDotCount() == 0u && testRail.OverflowCount() == 10u && ! testRail.OnMouseWheel(app.Host(), {0.f, 0.f}, -120.f, 0u),
                "short rail with cap==0 hides all dots and ignores wheel without crashing");

        // Rail VK_DOWN from ExpandButton -> DotButton(0) using the host-attached _rightRail
        MultiFolderFilesProbe::SetPinModes(app, PanePinMode::Auto, PanePinMode::Auto, true);
        MultiFolderFilesProbe::Resize(app, 900u, 700u);
        auto* const rightRail = MultiFolderFilesProbe::RightRail(app);
        Require(rightRail && rightRail->IsVisible() && rightRail->VisibleDotCount() > 0u, "right rail visible with dots for VK_DOWN test");
        app.Host().SetFocusControl(rightRail->ExpandButton(), false);
        Require(app.Host().GetFocusControl() == rightRail->ExpandButton() && rightRail->ExpandButton()->OnKeyDown(app.Host(), VK_DOWN, 0u) &&
                    app.Host().GetFocusControl() == rightRail->DotButton(0u),
                "VK_DOWN on RailExpandButton moves focus to first dot");

        // PinOutline moves focus to _outlineTree
        app.Execute(Command::PinOutline);
        Require(app.Host().GetFocusControl() == MultiFolderFilesProbe::OutlineTree(app), "PinOutline moves focus to _outlineTree when pinning open");

        // Multi-folder hover collapse stops _multiFolderFiles, and PinFiles from hover keeps files active
        MultiFolderFilesProbe::SetPinModes(app, PanePinMode::Auto, PanePinMode::Auto, false);
        MultiFolderFilesProbe::Resize(app, 900u, 700u);
        MultiFolderFilesProbe::RevealPaneOverlay(app, PaneSide::Left);
        Require(MultiFolderFilesProbe::IsPaneOverlayActive(app, PaneSide::Left) && MultiFolderFilesProbe::MultiFolderFiles(app).RootCount() > 0u,
                "left hover overlay activates multi-folder service");
        MultiFolderFilesProbe::DismissHoverOverlays(app, false);
        Require(MultiFolderFilesProbe::MultiFolderFiles(app).RootCount() == 0u, "collapsing multi-folder hover overlay closes MultiFolderFilesService");

        // PinFiles from hover overlay sets showFiles before dismissing
        MultiFolderFilesProbe::RevealPaneOverlay(app, PaneSide::Left);
        app.Execute(Command::PinFiles);
        Require(app.Preferences().showFiles && MultiFolderFilesProbe::MultiFolderFiles(app).RootCount() > 0u, "PinFiles preserves active multi-folder service");

        // Unpin Files to make _leftRail visible, then verify RefreshLeftRailDots formats GitHub dirty doc labels with disambiguated owner/repo
        app.Execute(Command::PinFiles);
        Require(MultiFolderFilesProbe::LeftRail(app)->IsVisible(), "unpinning Files reveals left rail");
        MultiFolderFilesProbe::SetActiveDocumentGitHubOrigin(app, L"OwnerA", L"RepoOne", L"main", L"docs/README.md");
        MultiFolderFilesProbe::AddDirtyGitHubInactiveDoc(app, L"OwnerB", L"RepoTwo", L"main", L"sub/README.md");
        MultiFolderFilesProbe::RefreshLeftRailDots(app);
        Require(MultiFolderFilesProbe::LeftRail(app)->ItemCount() == 2u &&
                    MultiFolderFilesProbe::LeftRail(app)->DotButton(0u)->DotItem().name == L"OwnerA/RepoOne/README.md" &&
                    MultiFolderFilesProbe::LeftRail(app)->DotButton(1u)->DotItem().name == L"OwnerB/RepoTwo/README.md",
                "RefreshLeftRailDots resolves and disambiguates dirty GitHub filenames");

        Require(foreground.NeverAcquiredForeground(), "nav-polish test never acquires foreground focus");
    });
}
} // namespace RedTilde
