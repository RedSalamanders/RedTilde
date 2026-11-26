#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "Common/TextUtilities.h"
#include "res/Resource.h"
#include <algorithm>
#include <array>
#include <commctrl.h>
#include <shobjidl.h>
#include <utility>
#include <wil/result.h>
#include <winhttp.h>

namespace RedTilde
{
namespace
{
enum class ImageReferenceCategory
{
    PossibleFile,
    Network,
    Other
};
// Shared explanatory-chrome/watch-invalidation policy over semantic values.
// This performs no URI admission, path resolution, permission grant or I/O;
// only SDK-admitted LocalFile requests can supply actual watched paths.
ImageReferenceCategory ImageReferenceCategoryFor(const DxUi::RichText::SemanticElement& element) noexcept
{
    if (element.wiki)
        return ImageReferenceCategory::Other;
    const auto& value = element.destination;
    if (StartsWithOrdinalIgnoreCase(value, L"http://") || StartsWithOrdinalIgnoreCase(value, L"https://") ||
        (StartsWithOrdinalIgnoreCase(value, L"file://") && value.size() > 7u && value[7u] != L'/') || value.starts_with(L"//") || value.starts_with(L"\\\\"))
        return ImageReferenceCategory::Network;
    return StartsWithOrdinalIgnoreCase(value, L"data:") ? ImageReferenceCategory::Other : ImageReferenceCategory::PossibleFile;
}
} // namespace

std::filesystem::path Application::PickFile(bool saving, bool image, bool folder)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return {};
    _modalPrompt             = ModalPromptKind::FileDialog;
    const auto restorePrompt = wil::scope_exit([&]() noexcept { _modalPrompt = ModalPromptKind::None; });
#if defined(ENABLE_TESTS)
    if (_options.headless && _options.filePickerForTest && ! image && ! folder)
        return _options.filePickerForTest(saving);
#endif
    if (_options.headless)
        return {};
    wil::com_ptr<IFileDialog> dialog;
    const CLSID classId = saving ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hr          = DxUi::Failure::InvokeNative(
        &CoCreateInstance, classId, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER), __uuidof(IFileDialog), reinterpret_cast<void**>(dialog.put()));
    if (FAILED(hr))
        return {};
    const auto markdownLabel = Resource(IDS_FILTERMARKDOWN), utf8Label = Resource(IDS_FILTERUTF8), allLabel = Resource(IDS_FILTERALL),
               imageLabel = Resource(IDS_FILTERIMAGES);
    const COMDLG_FILTERSPEC markdown[]{{markdownLabel.c_str(), L"*.md;*.markdown"}, {utf8Label.c_str(), L"*.txt"}, {allLabel.c_str(), L"*.*"}};
    const COMDLG_FILTERSPEC pictures[]{{imageLabel.c_str(), L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"}};
    if (! folder)
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileTypes, image ? 1u : 3u, image ? pictures : markdown);
    else
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetTitle, Resource(IDS_OPENFOLDER).c_str());
    DWORD flags{};
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::GetOptions, &flags);
    // The file worker observes the selected destination before the application
    // prompts for overwrite. That consent can then be tied to a file version.
    if (saving)
        flags &= ~static_cast<DWORD>(FOS_OVERWRITEPROMPT);
    (void)InvokeNativeMethod(dialog.get(),
                             &IFileDialog::SetOptions,
                             flags | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR |
                                 (folder   ? FOS_PICKFOLDERS | FOS_PATHMUSTEXIST
                                  : saving ? FOS_PATHMUSTEXIST
                                           : FOS_FILEMUSTEXIST));
    if (! _settings.filesRoot.empty())
    {
        wil::com_ptr<IShellItem> initial;
        if (SUCCEEDED(DxUi::Failure::InvokeNative(
                &SHCreateItemFromParsingName, _settings.filesRoot.c_str(), nullptr, __uuidof(IShellItem), reinterpret_cast<void**>(initial.put()))))
            (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultFolder, initial.get());
    }
    if (saving)
    {
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultExtension, L"md");
        const auto baseName = _session.path.filename().empty() ? std::filesystem::path(Resource(IDS_UNTITLED)).stem().native() : _session.path.stem().native();
        const auto name     = _session.recovered      ? FormatStringResource(nullptr, IDS_RECOVEREDNAME, baseName)
                              : _session.path.empty() ? Resource(IDS_UNTITLED)
                                                      : _session.path.filename().native();
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileName, name.c_str());
    }
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::Show, _window.get())))
        return {};
    wil::com_ptr<IShellItem> item;
    wil::unique_cotaskmem_string path;
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::GetResult, item.put())) ||
        FAILED(InvokeNativeMethod(item.get(), &IShellItem::GetDisplayName, SIGDN_FILESYSPATH, path.put())))
        return {};
    return std::filesystem::path(path.get());
}

void Application::Open()
{
    AskWorkspaceThen([this]
    {
        const auto path          = PickFile(false);
        auto* const activeEditor = ActiveEditor();
        if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached() || ! activeEditor)
            return;
        auto entrySource = activeEditor->Snapshot();
        const HRESULT hr = _files.Submit(false, path);
        if (FAILED(hr))
        {
            _activeFileTx = std::monostate{};
            Status(IDS_BUSY);
        }
        else
        {
            _activeFileTx = PickerOpenTx{std::move(entrySource), _workspaceApprovals};
            Status(IDS_LOADING);
        }
    });
}

void Application::AbortSaveConfirmBatch() noexcept
{
    if (_saveConfirmSession && _saveConfirmSession->sequentialSaveActive)
        _saveConfirmSession.reset();
}

void Application::Save(bool saveAs, const AsyncFiles::Result* confirmed, std::optional<std::filesystem::path> selectedPath)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
    {
        if (std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        return;
    }
    saveAs = saveAs || _session.recovered || _session.origin.kind == DocumentOriginInfo::Kind::GitHub;
    if (_files.Busy())
    {
        if (std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        Status(IDS_BUSY);
        return;
    }
    auto* const activeEditor = ActiveEditor();
    if (! activeEditor)
    {
        AbortSaveConfirmBatch();
        return;
    }
    const auto source        = activeEditor->Snapshot();
    const auto key           = _session.key;
    const auto originalPath  = _session.path;
    const auto originalStamp = _session.stamp;
    const bool originalBom = _session.bom, originalRecovered = _session.recovered;
    auto path                    = confirmed ? confirmed->path : (selectedPath ? selectedPath.value() : _session.path);
    const bool pickedDestination = ! confirmed && ! selectedPath && (saveAs || path.empty());
    if (pickedDestination)
        path = PickFile(true);
    if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached() || ActiveEditor() != activeEditor)
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        return;
    }
    if (key != _session.key || _session.path != originalPath || _session.stamp != originalStamp || _session.bom != originalBom ||
        _session.recovered != originalRecovered || ! activeEditor->Snapshot().IsSameRevision(source))
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    const auto expected         = confirmed ? confirmed->conflictVersion : (! _session.path.empty() ? std::optional<FileStamp>(_session.stamp) : std::nullopt);
    const bool sameIdentityOnly = ! confirmed && saveAs && path.lexically_normal() != _session.path.lexically_normal();
    auto protection   = confirmed && confirmed->recovery ? confirmed->recovery.value()
                                                         : SaveRecoveryContext{_options.dataDirectory, activeEditor->GetSelection(), ++_recoveryGeneration};
    std::wstring text = confirmed ? confirmed->document.text : activeEditor->GetText();
    const bool bom    = confirmed ? confirmed->document.bom : _session.bom;
    const HRESULT hr =
        _files.Submit(true, path, std::move(text), bom, expected, sameIdentityOnly, false, _session.path, std::move(protection), pickedDestination);
    if (FAILED(hr))
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        const auto error = ErrorText(hr);
        _feedback->SetText(FormatStringResource(nullptr, IDS_SAVEFAILED, error));
    }
    else
    {
        if (! std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = SaveTx{};
        Status(IDS_SAVING);
    }
}

void Application::ResolveSaveConflict(const AsyncFiles::Result& result)
{
    if (! IsUiAttached())
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        return;
    }
    auto* const primary    = ActiveEditor();
    const auto source      = primary->Snapshot();
    const auto key         = _session.key;
    const auto path        = _session.path;
    const auto identity    = _session.stamp;
    const bool originalBom = _session.bom, originalRecovered = _session.recovered;
    Status(result.pickerOverwrite ? IDS_PICKEROVERWRITE : IDS_CONFLICT);
    SaveConflictChoice choice = SaveConflictChoice::Cancel;
    {
        const auto previousPrompt = std::exchange(_modalPrompt, ModalPromptKind::SaveConflict);
        const auto restore        = wil::scope_exit([&] { _modalPrompt = previousPrompt; });
#if defined(ENABLE_TESTS)
        if (_options.headless && _options.saveConflictForTest)
            choice = _options.saveConflictForTest();
        else
#endif
            if (! _options.headless)
        {
            const auto dialogTitle = Resource(IDS_APP);
            const auto instruction = Resource(result.pickerOverwrite ? IDS_PICKEROVERWRITE : IDS_CONFLICT);
            const auto content     = result.pickerOverwrite ? SanitizeUiText(result.path.filename().native()) : Resource(IDS_CONFLICTHELP);
            const auto reload = Resource(IDS_RELOAD), saveAs = Resource(IDS_SAVEAS), overwrite = Resource(IDS_OVERWRITE), cancel = Resource(IDS_CANCEL);
            std::vector<TASKDIALOG_BUTTON> buttons;
            if (! result.pickerOverwrite)
                buttons.push_back({IDS_RELOAD, reload.c_str()});
            buttons.push_back({IDS_SAVEAS, saveAs.c_str()});
            if (result.conflictVersion)
                buttons.push_back({IDS_OVERWRITE, overwrite.c_str()});
            buttons.push_back({IDCANCEL, cancel.c_str()});
            TASKDIALOGCONFIG config{};
            config.cbSize             = sizeof(config);
            config.hwndParent         = _window.get();
            config.dwFlags            = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
            config.pszWindowTitle     = dialogTitle.c_str();
            config.pszMainIcon        = TD_WARNING_ICON;
            config.pszMainInstruction = instruction.c_str();
            config.pszContent         = content.c_str();
            config.cButtons           = static_cast<UINT>(std::size(buttons));
            config.pButtons           = buttons.data();
            config.nDefaultButton     = IDCANCEL;
            int pressed               = IDCANCEL;
            if (SUCCEEDED(DxUi::Failure::InvokeNative(&TaskDialogIndirect, &config, &pressed, nullptr, nullptr)))
            {
                if (pressed == IDS_RELOAD)
                    choice = SaveConflictChoice::Reload;
                else if (pressed == IDS_SAVEAS)
                    choice = SaveConflictChoice::SaveAs;
                else if (pressed == IDS_OVERWRITE)
                    choice = SaveConflictChoice::Overwrite;
            }
        }
    }
    if (! IsUiAttached())
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        return;
    }
    if (key != _session.key || path != _session.path || _session.stamp != identity || originalBom != _session.bom || originalRecovered != _session.recovered ||
        ActiveEditor() != primary || ! primary->Snapshot().IsSameRevision(source))
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    if (choice == SaveConflictChoice::Overwrite && result.conflictVersion)
        Save(false, &result);
    else if (choice == SaveConflictChoice::SaveAs)
        Save(true);
    else
    {
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        if (choice == SaveConflictChoice::Reload)
        {
            if (SUCCEEDED(_files.Submit(false, result.path, {}, false, {}, false, true)))
            {
                _activeFileTx = ConflictReloadTx{key, source, path};
                Status(IDS_LOADING);
            }
            else
                Status(IDS_BUSY);
        }
    }
}

void Application::FileCompleted() noexcept
{
    RunUiAction([&] { CompleteFileOperation(); });
}

void Application::CompleteFileOperation()
{
#if defined(ENABLE_TESTS)
    if (_options.fileCompletionAdmissionForTest)
    {
        const HRESULT admitted = _options.fileCompletionAdmissionForTest();
        if (FAILED(admitted))
        {
            ReportUiFailure("checked file-completion admission refusal");
            return;
        }
    }
#endif
    const auto checkDeferredRecovery = wil::scope_exit([&]
    {
        if (! _files.Busy() && _recovery.Ready() && (! _sessionResumeApplied || ! _recoveryOffered) && ! IsClosing() && ! IsClosingAfterSave() &&
            ! HasActiveModalUi() && ! _switchingDocument && ! std::holds_alternative<HeldOpenTx>(_activeFileTx))
            RecoveryCompleted();
    });
    std::optional<AsyncFiles::Result> completed;
    bool fromFiles{};
    std::optional<DxUi::RichText::SourceSnapshot> entrySource;
    std::vector<WorkspaceApproval> approvals;
    bool previousWorkspaceSelectionPreferred{};
    std::optional<ConflictReloadTx> reloadTx;
    if (auto* held = std::get_if<HeldOpenTx>(&_activeFileTx))
    {
        if (HasBlockDraft())
        {
            Status(IDS_BLOCKPENDING);
            return;
        }
        completed                           = std::move(held->result);
        fromFiles                           = held->fromFiles;
        entrySource                         = std::move(held->entrySource);
        approvals                           = std::move(held->approvals);
        previousWorkspaceSelectionPreferred = held->previousWorkspaceSelectionPreferred;
        reloadTx                            = std::move(held->conflictReload);
        _activeFileTx                       = std::monostate{};
    }
    else
    {
        completed = _files.Take();
        if (! completed)
            return;
        if (! completed.value().saving && HasBlockDraft())
        {
            HeldOpenTx stagedHeld;
            stagedHeld.result = std::move(completed.value());
            if (auto* picker = std::get_if<PickerOpenTx>(&_activeFileTx))
            {
                stagedHeld.entrySource = std::move(picker->entrySource);
                stagedHeld.approvals   = std::move(picker->approvals);
            }
            else if (auto* filesTx = std::get_if<FilesPaneOpenTx>(&_activeFileTx))
            {
                stagedHeld.fromFiles                           = true;
                stagedHeld.entrySource                         = std::move(filesTx->entrySource);
                stagedHeld.previousWorkspaceSelectionPreferred = filesTx->previousWorkspaceSelectionPreferred;
            }
            else if (auto* startup = std::get_if<StartupOpenTx>(&_activeFileTx))
            {
                stagedHeld.entrySource = std::move(startup->entrySource);
            }
            else if (auto* reload = std::get_if<ConflictReloadTx>(&_activeFileTx))
            {
                stagedHeld.conflictReload = std::move(*reload);
            }
            _activeFileTx = std::move(stagedHeld);
            Status(IDS_BLOCKPENDING);
            return;
        }
        if (auto* picker = std::get_if<PickerOpenTx>(&_activeFileTx))
        {
            entrySource   = std::move(picker->entrySource);
            approvals     = std::move(picker->approvals);
            _activeFileTx = std::monostate{};
        }
        else if (auto* filesTx = std::get_if<FilesPaneOpenTx>(&_activeFileTx))
        {
            fromFiles                           = true;
            entrySource                         = std::move(filesTx->entrySource);
            previousWorkspaceSelectionPreferred = filesTx->previousWorkspaceSelectionPreferred;
            _activeFileTx                       = std::monostate{};
        }
        else if (auto* startup = std::get_if<StartupOpenTx>(&_activeFileTx))
        {
            entrySource   = std::move(startup->entrySource);
            _activeFileTx = std::monostate{};
        }
    }
    auto& result                = completed.value();
    const bool automaticStartup = ! result.saving && ! result.reloading && _startupRestore.IsOpeningLastDocument();
    if (automaticStartup)
    {
        _startupRestore.phase = StartupRestoreState::Phase::Completed;
        const auto admitted   = std::exchange(_startupRestore.source, std::nullopt);
        if (! admitted || ! ActiveEditor()->Snapshot().IsSameRevision(admitted.value()) || ! _recoveryCandidates.empty() || IsSessionEnding())
        {
            _activeFileTx = std::monostate{};
            Status(IDS_FILECONTEXTCHANGED);
            return;
        }
    }
    if (FAILED(result.status))
    {
        if (result.reloading)
            _activeFileTx = std::monostate{};
        const bool failedClose = IsClosingAfterSave();
        if (failedClose)
            ResumeAfterSaveClose();
        MonitorDiagnostics::Write(RedMonitor::Severity::Error,
                                  result.saving ? L"save failed; source and previous target retained" : L"open failed; current document retained");
        if (result.saving && result.status == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && ! failedClose)
        {
            ResolveSaveConflict(result);
            return;
        }
        std::wstring message;
        if (result.saving && result.status == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH))
            message = FormatStringResource(nullptr, IDS_SAVEFAILED, Resource(IDS_CONFLICTHELP));
        else if (result.status == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION))
            message = Resource(result.saving ? IDS_SOURCEENCODING : IDS_FILENOTUTF8);
        else if (result.status == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
        {
            constexpr auto limit = DxUi::RichText::Document::MaximumUnits;
            message              = result.rejectedUnits ? FormatStringResource(nullptr, IDS_FILETOOLARGECOUNT, result.rejectedUnits, limit)
                                                        : FormatStringResource(nullptr, IDS_FILETOOLARGELIMIT, limit);
        }
        else
        {
            const auto error = ErrorText(result.status);
            message          = FormatStringResource(nullptr, result.saving ? IDS_SAVEFAILED : IDS_OPENFAILED, error);
        }
        if (! _options.headless && ! failedClose && ! automaticStartup && ! HasActiveModalPrompt())
        {
            _modalPrompt       = ModalPromptKind::ErrorMessage;
            const auto restore = wil::scope_exit([&]() noexcept { _modalPrompt = ModalPromptKind::None; });
            (void)DxUi::Failure::InvokeNative(
                &MessageBoxW, _window.get(), message.c_str(), Resource(IDS_ERROR).c_str(), static_cast<UINT>(MB_OK | MB_ICONERROR));
        }
        if (IsUiAttached() && _feedback)
            _feedback->SetText(message);
        _activeFileTx = std::monostate{};
        AbortSaveConfirmBatch();
        return;
    }
    if (result.reloading)
    {
        std::optional<ConflictReloadTx> request = std::move(reloadTx);
        if (! request)
        {
            if (auto* reload = std::get_if<ConflictReloadTx>(&_activeFileTx))
                request = std::move(*reload);
        }
        _activeFileTx = std::monostate{};
        if (! IsUiAttached())
            return;
        if (! request || _session.key != request->key || _session.path != request->path || ! ActiveEditor()->Snapshot().IsSameRevision(request->source))
        {
            Status(IDS_FILECONTEXTCHANGED);
            return;
        }
        if (SetDocument(std::move(result.document.text), std::move(result.path), result.document.bom, result.document.stamp))
            ClearStatus();
        return;
    }
    if (result.saving)
    {
        std::function<void()> continuation;
        if (auto* saveTx = std::get_if<SaveTx>(&_activeFileTx))
            continuation = std::move(saveTx->afterSave);
        _activeFileTx           = std::monostate{};
        _session.recovered      = false;
        _session.resumedUnsaved = false;
        _session.origin         = {};
        _session.savedText      = std::move(result.document.text);
        _derivedCaches.dirtyFact.reset();
        if (_session.path != result.path)
            _printNetworkImages.clear();
        _session.path  = std::move(result.path);
        _session.stamp = result.document.stamp;
        EnsureFilesRoot();
        RefreshFiles(true);
        if (! AcceptSourceResult(ActiveEditor()->TrySetDocumentFilePath(_session.path.native())))
        {
            if (IsClosingAfterSave())
                ResumeAfterSaveClose();
            AbortSaveConfirmBatch();
            return;
        }
        if (ActiveEditor() != _editor)
            (void)AcceptSourceResult(_editor->TrySetDocumentFilePath(_session.path.native()));
        RememberActiveDocument();
        Changed();
        ClearStatus();
        if (_saveConfirmSession && _saveConfirmSession->sequentialSaveActive)
        {
            AdvanceSaveConfirmSequentialSave();
            return;
        }
        if (IsClosingAfterSave())
        {
            if (Dirty())
            {
                // Save owns only its admitted immutable source. Closing cannot
                // discard edits typed after that snapshot was submitted.
                ResumeAfterSaveClose();
                AskWorkspaceThen([this] { BeginClose(false); }, {}, true);
            }
            else
            {
                if (std::ranges::any_of(_inactiveDocuments, [](const auto& document) { return document.dirty; }))
                {
                    ResumeAfterSaveClose();
                    AskWorkspaceThen([this] { BeginClose(false); }, {}, true);
                }
                else
                {
                    TransitionLifecycle(LifecyclePhase::Active);
                    _visibleBeforeSaveClose = false;
                    BeginClose(false);
                }
            }
            return;
        }
        if (continuation)
            AskThen(std::move(continuation)); // edits typed during a save remain protected
    }
    else
    {
        // User may have typed while a slow open ran. Recheck dirty state before
        // replacing the source; this is a new consent decision, not a stale one.
        auto staged = std::make_shared<AsyncFiles::Result>(std::move(result));
        auto apply  = [this, staged]
        {
            if (SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp))
            {
                ResetWorkspace();
                ScheduleSpellCheck(true);
                RefreshFiles();
                ClearStatus();
            }
        };
        if (fromFiles)
        {
            const auto previousCount = _inactiveDocuments.size();
            if (StartWorkspaceDocument() && IsUiAttached())
            {
                _workspaceSelectionPreferred = false;
                if (SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp))
                    ClearStatus();
                else
                {
                    _workspaceSelectionPreferred = previousWorkspaceSelectionPreferred;
                    if (_inactiveDocuments.size() > previousCount && SwitchWorkspaceDocument(previousCount))
                    {
                        _inactiveDocuments.pop_back(); // Retire only the new empty slot; the prior file/history is active again.
                        if (IsUiAttached())
                            RefreshFiles();
                    }
                }
            }
            else if (IsUiAttached())
                RefreshFiles();
        }
        else if (entrySource && ActiveEditor()->Snapshot().IsSameRevision(entrySource.value()))
            apply();
        else
            AskWorkspaceThen(std::move(apply), std::move(approvals));
    }
}

std::optional<std::wstring> Application::BeginBitmapPaste(std::span<const std::byte> dib, bool v5)
{
    auto* const activeEditor = ActiveEditor();
    if (! CanAcceptCommand() || IsSessionEnding() || HasActiveModalUi() || ! activeEditor || activeEditor->IsReadOnly() ||
        activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source || _host.GetFocusControl() != activeEditor || activeEditor->IsSelectionGestureActive())
        return std::nullopt;
    PendingBitmapPaste pending;
    pending.documentKey  = _session.key;
    pending.source       = activeEditor->Snapshot();
    pending.selection    = activeEditor->GetSelection();
    pending.view         = _settings.view;
    pending.documentPath = _session.path;
    pending.lifetime     = GetControlLifetimeToken(*activeEditor);
    const auto submitted = _bitmapWriter.Submit(dib, v5, pending.request);
    if (FAILED(submitted))
    {
        Status(submitted == HRESULT_FROM_WIN32(ERROR_BUSY) ? IDS_IMAGEPASTEBUSY : IDS_IMAGEPASTEFAILED);
        return std::nullopt;
    }
    _pendingBitmapPaste = std::move(pending);
    Status(IDS_IMAGEPASTING);
    // The synchronous SDK callback owns no I/O. This application completes its
    // separately guarded source transaction after the durable worker result.
    return std::nullopt;
}

void Application::ServiceBitmapPaste()
{
    BitmapPasteWriter::Result result;
    if (! _bitmapWriter.Take(result))
        return;
    auto pending = std::exchange(_pendingBitmapPaste, std::nullopt);
    bool keep{};
    const auto settle = wil::scope_exit([&]
    {
        if (keep)
            _bitmapWriter.Commit(result.request);
        else if (FAILED(_bitmapWriter.Discard(std::move(result))))
            OutputDebugStringW(L"RedTilde: a declined clipboard asset was retained after a cleanup scheduling error.\n");
    });
    if (IsClosing() || IsSessionEnding())
        return;
    if (FAILED(result.status))
    {
        Status(IDS_IMAGEPASTEFAILED);
        return;
    }
    auto* const activeEditor = ActiveEditor();
    if (! pending || HasActiveModalPrompt() || pending->request != result.request || pending->lifetime.expired() || pending->documentKey != _session.key ||
        pending->view != _settings.view || pending->documentPath != _session.path || ! activeEditor || activeEditor->IsReadOnly() ||
        activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source || _host.GetFocusControl() != activeEditor ||
        activeEditor->IsSelectionGestureActive() || activeEditor->GetSelection() != pending->selection ||
        activeEditor->Snapshot().ChangedRangeFrom(pending->source).kind != DxUi::RichText::SourceChangeKind::Unchanged || HasBlockDraft())
    {
        Status(IDS_IMAGEPASTECANCELLED);
        return;
    }
    const auto lifetime = pending->lifetime;
    const auto alt      = Resource(IDS_PASTEDIMAGE);
    DxUi::RichText::MarkdownCommandRequest request;
    request.command     = DxUi::RichText::MarkdownCommand::InsertImage;
    request.text        = alt;
    request.destination = result.asset.uri;
    keep                = activeEditor->ExecuteMarkdownCommand(request) == DxUi::RichText::MarkdownCommandStatus::Changed;
    if (! IsClosing() && ! lifetime.expired())
    {
        if (keep)
            Status(IDS_IMAGEPASTED);
        else if (IsMarkdownCommandCapacityExceeded(*activeEditor, request, pending->selection))
            ReportCapacityLimit();
        else
            Status(IDS_IMAGEPASTEFAILED);
    }
}

void Application::RefreshImageConsent()
{
    if (! _editor || ! _imageConsent || IsClosing())
        return;
    auto* const owner = _root;
    if (! owner || _host.GetRoot() != owner)
        return;
    const auto lifetime = GetControlLifetimeToken(*owner);
    const auto semantic = _editor->GetSemanticDocument();
    const auto current  = [&]() noexcept
    {
        return ! lifetime.expired() && ! IsClosing() && _root == owner && _host.GetRoot() == owner && _editor && _editor->GetSemanticDocument() == semantic &&
               _imageConsentSource == semantic;
    };
    if (semantic != _imageConsentSource || _imageConsentPath != _session.path)
    {
        _imageConsentSource     = semantic;
        _imageConsentPath       = _session.path;
        _imageConsentHasNetwork = false;
        _imageFailureStatus     = S_OK;
        if (semantic)
            for (const auto& element : semantic->Elements())
                if (element.role == DxUi::RichText::SemanticRole::Image)
                {
                    if (ImageReferenceCategoryFor(element) == ImageReferenceCategory::Network || _session.path.native().starts_with(L"\\\\"))
                    {
                        _imageConsentHasNetwork = true;
                        break;
                    }
                }
    }
    const bool enabled = _settings.allowRemoteImages || _session.remoteImagesAllowed;
    unsigned text      = static_cast<unsigned>(! enabled                     ? IDS_IMAGECONSENTBLOCKED
                                               : _settings.allowRemoteImages ? IDS_IMAGECONSENTSETTINGS
                                                                             : IDS_IMAGECONSENTDOCUMENT);
    if (FAILED(_imageFailureStatus))
        text =
            static_cast<unsigned>(_imageFailureStatus == E_ACCESSDENIED                             ? IDS_IMAGEPOLICYFAILED
                                  : _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) ? IDS_IMAGETOOLARGE
                                  : _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_TIMEOUT) || _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_WINHTTP_TIMEOUT)
                                      ? IDS_IMAGETIMEOUT
                                      : IDS_IMAGELOADFAILED);
    if (_imageConsentText->GetText() != ResourceView(text))
        _imageConsentText->SetText(Resource(text));
    if (! current())
        return;
    const unsigned action = _session.remoteImagesAllowed ? IDS_IMAGEBLOCKHERE : IDS_IMAGEALLOWHERE;
    if (_imageConsentAction->GetText() != ResourceView(action))
        _imageConsentAction->SetText(Resource(action));
    if (! current())
        return;
    _imageConsentAction->SetVisible(_imageConsentHasNetwork && ! _settings.allowRemoteImages);
    if (! current())
        return;
    _imageReload->SetEnabled(true);
    if (! current())
        return;
    const bool visible = _imageConsentHasNetwork || FAILED(_imageFailureStatus);
    if (_imageConsent->IsVisible() != visible)
    {
        _imageConsent->SetVisible(visible);
        if (current())
            Layout();
    }
}

void Application::ServiceImages()
{
    if (! _editor || IsClosing() || _imageBindingTransition || _switchingDocument)
        return;
    auto* const primary      = _editor;
    const auto lifetime      = GetControlLifetimeToken(*primary);
    const auto current       = [&]() noexcept { return ! lifetime.expired() && ! IsClosing() && _editor == primary; };
    const auto semantic      = primary->GetSemanticDocument();
    bool watchBindingChanged = _imageWatchDocumentPath != _session.path;
    if (semantic && semantic != _imageWatchSemantic)
    {
        // Decode cancellation ends a job, including successful completion. It
        // does not end the document's ownership of the displayed file. Compare
        // exact possible-file destinations only on immutable publication;
        // non-file image edits must not rearm local I/O and refetch HTTP images.
        const auto before = _imageWatchSemantic ? _imageWatchSemantic->Elements() : std::span<const DxUi::RichText::SemanticElement>{};
        const auto after  = semantic->Elements();
        const auto image  = [](const auto& element)
        { return element.role == DxUi::RichText::SemanticRole::Image && ImageReferenceCategoryFor(element) == ImageReferenceCategory::PossibleFile; };
        auto previous = std::ranges::find_if(before, image);
        auto next     = std::ranges::find_if(after, image);
        while (previous != before.end() && next != after.end() && previous->destination == next->destination && previous->wiki == next->wiki)
        {
            previous = std::find_if(std::next(previous), before.end(), image);
            next     = std::find_if(std::next(next), after.end(), image);
        }
        watchBindingChanged = watchBindingChanged || previous != before.end() || next != after.end();
        _imageWatchSemantic = semantic;
    }
    if (watchBindingChanged)
    {
        _imageWatchDocumentPath = _session.path;
        const bool hadPaths     = ! _watchedImagePaths.empty();
        _watchedImagePaths.clear();
        (void)_imageFiles.SetFiles({});
        if (hadPaths)
        {
            // A removed/retargeted image or new document base retires the old
            // binding. Obtain fresh canonical paths through SDK admission.
            primary->ReloadImages();
            if (! current())
                return;
        }
    }
    if (_imageFiles.HasResult())
    {
        if (const auto notice = _imageFiles.TakeResult())
        {
            if (FAILED(notice.value().status))
                _imageFailureStatus = notice.value().status;
            else
            {
                ++_imageFileRefreshes;
                primary->ReloadImages();
                if (! current())
                    return;
            }
        }
    }
    RefreshImageConsent();
    if (! current())
        return;
    const bool hadWatchNotice = _imageFiles.HasResult();
    _editor->TakePendingImageRequests(_imageRequests);
    if (watchBindingChanged || ! _imageRequests.empty())
        _images.SetDocumentContext(_session.path.native());
    bool watchChanged{};
    for (const auto& request : _imageRequests)
        if (request.SourceKind() == DxUi::RichText::RichImageSourceKind::LocalFile && ! request.Cancelled() &&
            std::ranges::find(_watchedImagePaths, request.FilePath(), [](const auto& path) -> const std::wstring& { return path.native(); }) ==
                _watchedImagePaths.end() &&
            _watchedImagePaths.size() < 32u)
        {
            _watchedImagePaths.emplace_back(request.FilePath());
            watchChanged = true;
        }
    if (watchChanged)
    {
        ++_imageWatchPathBuilds;
        const auto status = _imageFiles.SetFiles(_watchedImagePaths);
        if (FAILED(status))
        {
            (void)_imageFiles.SetFiles({}); // Retire the old binding when automatic watching cannot be admitted.
            _imageFailureStatus = status;
        }
    }
    if (watchBindingChanged || hadWatchNotice || ! _imageRequests.empty() || _images.HasActiveOrResults())
        _images.DiscardCancelled();
    if (! _imageRequests.empty())
        (void)_images.Submit(_imageRequests);
    if (_images.HasResults())
    {
        std::array<RichImageLoader::Result, RichImageLoader::MaximumResults> results;
        const auto count = _images.Drain(results);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto& result  = results[index];
            const bool accepted = primary->CompleteImageRequest(result.request, result.image);
            if (! current())
                return;
            if (accepted)
                RememberPrintImage(result.request, result.image);
            if (accepted && FAILED(result.status) && result.status != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                _imageFailureStatus = result.status;
        }
        RefreshImageConsent();
    }
    if (current())
    {
        const auto currentSemantic      = primary->GetSemanticDocument();
        const auto currentAcknowledged  = currentSemantic ? primary->GetAcknowledgedSemanticDocument(_host) : nullptr;
        _imageWatchAcknowledgedSemantic = currentAcknowledged;
        _imageWatchWasRichVisible       = (primary->GetViewMode() == DxUi::RichText::ViewMode::Rich && primary->IsVisible() && currentSemantic != nullptr);
    }
}
} // namespace RedTilde
