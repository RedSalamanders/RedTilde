#include "Application.Tests.MultiFolderFiles.h"
#include "Common/FileOps.h"
#include "Common/Files.h"
#include "Common/Recovery.h"
#include "Common/Storage.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.FilesFixtures.h"
#include "Support/Support.Tests.Foreground.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
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
}
} // namespace RedTilde
