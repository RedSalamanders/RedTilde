#include "Application.h"
#include "Common/PathSecurity.h"
#include "res/Resource.h"
#include <algorithm>
#include <utility>

namespace RedTilde
{
void Application::CheckpointRecovery(bool immediate, RecoverySnapshotKind kind)
{
    std::optional<RecoverySnapshot> snapshot;
    auto* const activeEditor = ActiveEditor();
    auto selection           = activeEditor->GetSelection();
    bool needsRecovery       = Dirty();
    std::wstring_view draft;
    std::wstring normalizedDraft;
    bool changedDraft{};
    if (HasBlockDraft() && activeEditor->Snapshot().IsSameRevision(_blockDraftSession.value().snapshot) &&
        activeEditor->Revision() == _blockDraftSession.value().revision)
    {
        const auto& range              = _blockDraftSession.value().range;
        draft                          = _blockDraftText->GetText();
        const std::wstring_view source = CurrentText();
        if (draft != source.substr(range.first, range.last - range.first))
        {
            normalizedDraft = NormalizeInsertedLineEndings(draft, source);
            draft           = normalizedDraft;
            if (! DxUi::RichText::IsValidUtf16(draft) || source.size() - (range.last - range.first) + draft.size() > DxUi::RichText::Document::MaximumUnits)
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            changedDraft  = draft != source.substr(range.first, range.last - range.first);
            needsRecovery = needsRecovery || changedDraft;
        }
    }
    if (needsRecovery)
    {
        auto text = CurrentText();
        ++_activity.recoveryTextCopies;
        if (changedDraft)
        {
            const auto& range = _blockDraftSession.value().range;
            // Recovery is a previewable copy of the proposed edit. The live
            // authoritative source/history remain untouched until Apply.
            text.replace(range.first, range.last - range.first, draft);
            selection = {range.first, range.first + draft.size()};
        }
        RecoverySnapshot snap;
        snap.text               = std::move(text);
        snap.originalPath       = _session.path;
        snap.bom                = _session.bom;
        snap.selection          = selection;
        snap.generation         = ++_recoveryGeneration;
        snap.kind               = kind;
        snap.origin             = _session.origin;
        snap.workspaceOrder     = 0u;
        snap.wasActive          = true;
        snap.savedBaselineStamp = _session.stamp;
        snap.recovered          = _session.recovered;
        snapshot                = std::move(snap);
    }
    const bool flush = immediate || ! snapshot.has_value();
    ++_activity.recoverySubmissions;
    if (FAILED(_recovery.SubmitDocument(_session.key, std::move(snapshot), flush)))
        Status(IDS_RECOVERYFAILED);
}

bool Application::AdoptRecoveryCandidate(const RecoveryCandidate& candidate, bool asSessionResume, bool startNewSlot)
{
    if (! IsUiAttached() || IsClosing() || (! startNewSlot && _files.Busy()))
        return false;
    const std::size_t previousCount = _inactiveDocuments.size();
    if (startNewSlot && (! StartWorkspaceDocument() || ! IsUiAttached()))
        return false;
    if (! asSessionResume && ! _inactiveDocuments.empty())
        _workspaceSelectionPreferred = true;
    const FileStamp baselineStamp = asSessionResume ? candidate.snapshot.savedBaselineStamp.value_or(FileStamp{}) : FileStamp{};
    if (! SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom, baselineStamp, false) || ! IsUiAttached())
    {
        if (startNewSlot && _inactiveDocuments.size() > previousCount && IsUiAttached() && SwitchWorkspaceDocument(previousCount, false, false))
        {
            _inactiveDocuments.pop_back();
            if (IsUiAttached())
                RefreshFiles();
        }
        return false;
    }
    _session.recovered      = asSessionResume ? candidate.snapshot.recovered : true;
    _session.resumedUnsaved = asSessionResume;
    if (asSessionResume)
        _session.origin = candidate.snapshot.origin;
    (void)AcceptSourceResult(_editor->TrySetSelection(candidate.snapshot.selection));
    if (asSessionResume)
        _derivedCaches.dirtyFact.reset();
    Changed();
    if (! IsUiAttached())
        return false;
    if (asSessionResume)
    {
        RecoverySnapshot retained = candidate.snapshot;
        retained.kind             = RecoverySnapshotKind::SessionResume;
        retained.generation       = ++_recoveryGeneration;
        retained.recovered        = _session.recovered;
        if (FAILED(_recovery.SubmitDocument(_session.key, std::move(retained), true, candidate.id)))
            Status(IDS_RECOVERYFAILED);
        return true;
    }
    auto snapshot      = RecoverySnapshot{_editor->GetText(), _session.path, _session.bom, _editor->GetSelection(), ++_recoveryGeneration};
    snapshot.recovered = true;
    if (FAILED(_recovery.SubmitDocument(_session.key, snapshot, true, candidate.id)))
    {
        Status(IDS_RECOVERYFAILED);
        return false;
    }
    return true;
}

void Application::RecoveryCompleted()
{
    if (IsClosing())
    {
        FinishClose();
        return;
    }
    if (! _feedback)
        return;
    auto candidates = _recovery.TakeCandidates();
    for (auto& candidate : candidates)
        _recoveryCandidates.push_back({std::move(candidate.id), std::move(candidate.snapshot), RecoveryDisposition::Available});
    bool retiredCandidate{};
    for (const auto& id : _recovery.TakeRetired())
        retiredCandidate = std::erase_if(_recoveryCandidates, [&](const auto& candidate) { return candidate.id == id; }) != 0 || retiredCandidate;
    for (const auto& id : _recovery.TakeFailedDiscards())
        for (auto& candidate : _recoveryCandidates)
            if (candidate.id == id && candidate.disposition == RecoveryDisposition::Discarding)
                candidate.disposition = RecoveryDisposition::Available;
    if (_recovery.Settled() && FAILED(_recovery.Status()))
        for (auto& candidate : _recoveryCandidates)
            if (candidate.disposition == RecoveryDisposition::Discarding)
                candidate.disposition = RecoveryDisposition::Available;
    if (retiredCandidate && _activeOverlay == OverlayKind::RecoveryModal)
        ShowRecovery();
    if (FAILED(_recovery.Status()))
        Status(IDS_RECOVERYFAILED);
    const bool canApplyUi = ! HasActiveModalUi() && ! _files.Busy() && ! _switchingDocument && ! std::holds_alternative<HeldOpenTx>(_activeFileTx) &&
                            ! IsClosing() && ! IsSessionEnding();
    if (! _sessionResumeApplied && _recovery.Ready() && canApplyUi)
    {
        _sessionResumeApplied = true;
        std::vector<RecoveryCandidate> sessionResumeCandidates;
        std::erase_if(_recoveryCandidates,
                      [&](OfferedRecoveryCandidate& candidate)
        {
            if (candidate.snapshot.kind == RecoverySnapshotKind::SessionResume)
            {
                sessionResumeCandidates.push_back({std::move(candidate.id), std::move(candidate.snapshot)});
                return true;
            }
            return false;
        });
        if (! sessionResumeCandidates.empty())
        {
            std::ranges::sort(sessionResumeCandidates,
                              [](const auto& a, const auto& b)
            {
                if (a.snapshot.workspaceOrder != b.snapshot.workspaceOrder)
                    return a.snapshot.workspaceOrder < b.snapshot.workspaceOrder;
                return a.snapshot.wasActive && ! b.snapshot.wasActive;
            });
            const bool keepCliDoc = ! _options.document.empty();
            if (! keepCliDoc && _startupRestore.IsWaitingForRecovery())
            {
                _startupRestore.phase = StartupRestoreState::Phase::Completed;
                _startupRestore.source.reset();
            }
            if (keepCliDoc)
            {
                for (const auto& candidate : sessionResumeCandidates)
                {
                    if (_inactiveDocuments.size() + 1u >= RecoveryService::MaximumDocuments)
                    {
                        Status(IDS_WORKSPACEFULL);
                        break;
                    }
                    if (! DxUi::RichText::IsValidSourceUtf16(candidate.snapshot.text) ||
                        candidate.snapshot.text.size() > DxUi::RichText::Document::MaximumUnits)
                    {
                        Status(IDS_ACTIONFAILED);
                        break;
                    }
                    WorkspaceDocument doc;
                    if (doc.state.TryReset(candidate.snapshot.text) != DxUi::RichText::SourceResult::Success)
                    {
                        Status(IDS_ACTIONFAILED);
                        break;
                    }
                    (void)doc.state.TrySetSelection(candidate.snapshot.selection);
                    unsigned key{};
                    while (key == _session.key || std::ranges::any_of(_inactiveDocuments, [&](const auto& existing) { return existing.metadata.key == key; }))
                        ++key;
                    doc.metadata.key            = key;
                    doc.metadata.path           = candidate.snapshot.originalPath;
                    doc.metadata.bom            = candidate.snapshot.bom;
                    doc.metadata.stamp          = candidate.snapshot.savedBaselineStamp.value_or(FileStamp{});
                    doc.metadata.origin         = candidate.snapshot.origin;
                    doc.metadata.recovered      = candidate.snapshot.recovered;
                    doc.metadata.resumedUnsaved = true;
                    doc.metadata.savedText      = candidate.snapshot.text;
                    doc.dirty                   = true;
                    _inactiveDocuments.push_back(std::move(doc));
                    RecoverySnapshot retained = candidate.snapshot;
                    retained.kind             = RecoverySnapshotKind::SessionResume;
                    retained.generation       = ++_recoveryGeneration;
                    if (FAILED(_recovery.SubmitDocument(key, std::move(retained), true, candidate.id)))
                        Status(IDS_RECOVERYFAILED);
                }
                if (! _inactiveDocuments.empty())
                    RefreshLeftRailDots();
            }
            else
            {
                std::optional<unsigned> activeResumedKey;
                for (const auto& candidate : sessionResumeCandidates)
                {
                    const bool startNewSlot = ! _session.path.empty() || Dirty() || _editor->CanUndo() || _editor->CanRedo();
                    if (! AdoptRecoveryCandidate(candidate, true, startNewSlot))
                        break;
                    if (candidate.snapshot.wasActive)
                        activeResumedKey = _session.key;
                }
                if (activeResumedKey && _session.key != *activeResumedKey)
                {
                    for (std::size_t i = 0; i < _inactiveDocuments.size(); ++i)
                        if (_inactiveDocuments[i].metadata.key == *activeResumedKey)
                        {
                            (void)SwitchWorkspaceDocument(i, false, false);
                            break;
                        }
                }
            }
            if (! _inactiveDocuments.empty())
                _workspaceSelectionPreferred = true;
            RefreshFiles();
            Layout();
        }
    }
    if (! _recoveryOffered && _recovery.Ready() && canApplyUi)
    {
        _recoveryOffered = true;
        if (! _recoveryCandidates.empty() && (_startupRestore.enabled || _options.scenario == L"recovery"
#if defined(ENABLE_TESTS)
                                              || _options.recoveryRestartProbe
#endif
                                              ))
            ShowRecovery();
    }
    if (_startupRestore.IsWaitingForRecovery() && _recovery.Ready() && canApplyUi)
    {
        if (SUCCEEDED(_recovery.Status()) && _recoveryCandidates.empty() && _startupRestore.source &&
            ActiveEditor()->Snapshot().IsSameRevision(_startupRestore.source.value()) && ! _settings.lastDocument.empty() && _session.path.empty() &&
            ! _files.Busy() && ! HasBlockDraft() && ! IsSessionEnding())
        {
            auto entrySource     = ActiveEditor()->Snapshot();
            const HRESULT result = _files.Submit(false, _settings.lastDocument);
            if (SUCCEEDED(result))
            {
                _startupRestore.phase = StartupRestoreState::Phase::OpeningLastDocument;
                _activeFileTx         = StartupOpenTx{std::move(entrySource)};
                Status(IDS_LOADING);
            }
            else
            {
                _startupRestore.phase = StartupRestoreState::Phase::Completed;
                _startupRestore.source.reset();
                Status(IDS_BUSY);
            }
        }
        else
        {
            _startupRestore.phase = StartupRestoreState::Phase::Completed;
            _startupRestore.source.reset();
        }
    }
}

void Application::ShowRecovery()
{
    if (_recoveryCandidates.empty())
    {
        if (_activeOverlay == OverlayKind::RecoveryModal)
        {
            SetActiveOverlay(OverlayKind::None);
            Layout();
        }
        Status(IDS_NORECOVERY);
        _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
        return;
    }
    const auto& snapshot = _recoveryCandidates.front().snapshot;
    const auto path      = snapshot.originalPath.empty() ? Resource(IDS_UNTITLED) : SanitizeUiText(snapshot.originalPath.native(), 512u);
    const auto count     = _recoveryCandidates.size();
    _restoreAllRecovery->SetVisible(count > 1);
    _recoveryDescription->SetText(count > 1 ? FormatStringResource(nullptr, IDS_RECOVERYMULTIPLEHELP, count, path)
                                            : FormatStringResource(nullptr, IDS_RECOVERYHELP, path));
    _recoveryPreview->SetText(snapshot.text);
    SetActiveOverlay(OverlayKind::RecoveryModal);
    _host.SetFocusControl(_recoveryRestoreBtn, MayRequestNativeFocus());
    Layout();
    if (! IsInitializing() && ! IsClosing())
        CheckpointRecovery();
}

void Application::AskThen(std::function<void()> continuation)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return;
    if (HasBlockDraft())
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (_files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    if (! Dirty())
    {
        continuation();
        return;
    }
#if defined(ENABLE_TESTS)
    if (_options.headless && _options.unsavedPromptForTest && ! _options.forceDxUiSaveDialogForTest)
    {
        int choice = IDCANCEL;
        {
            _modalPrompt       = ModalPromptKind::UnsavedChanges;
            const auto restore = wil::scope_exit([&] { _modalPrompt = ModalPromptKind::None; });
            choice             = _options.unsavedPromptForTest();
        }
        if (! CanAcceptCommand())
            return;
        if (choice == IDNO)
            continuation();
        else if (choice == IDYES)
        {
            _activeFileTx = SaveTx{std::move(continuation)};
            Save();
        }
        return;
    }
#endif
    if (_options.headless
#if defined(ENABLE_TESTS)
        && ! _options.forceDxUiSaveDialogForTest
#endif
    )
        return;
    ShowSaveConfirmModal(std::move(continuation), false, true);
}

void Application::AskWorkspaceThen(std::function<void()> continuation, std::vector<WorkspaceApproval> approved, bool closingApplication)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return;
    if (HasBlockDraft())
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (_files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    const bool anyInactiveDirty = std::ranges::any_of(_inactiveDocuments, [](const auto& doc) { return doc.dirty; });
    const bool anyDirty         = Dirty() || anyInactiveDirty;
    if (! anyDirty)
    {
        _workspaceApprovals = std::move(approved);
        continuation();
        return;
    }
    const bool keepLegacyTestHook =
#if defined(ENABLE_TESTS)
        (_options.headless && _options.unsavedPromptForTest && ! _options.forceDxUiSaveDialogForTest);
#else
        false;
#endif
    if (closingApplication && ! keepLegacyTestHook && _settings.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume)
    {
        BeginClose(true);
        return;
    }
    if (keepLegacyTestHook)
    {
        const auto accepted = [&](unsigned key, const DxUi::RichText::SourceSnapshot& source)
        { return std::ranges::any_of(approved, [&](const auto& value) { return value.key == key && value.source.IsSameRevision(source); }); };
        if (! Dirty() || accepted(_session.key, ActiveEditor()->Snapshot()))
        {
            const auto pending = std::find_if(_inactiveDocuments.begin(), _inactiveDocuments.end(), [&](const auto& document) {
                return document.dirty && ! accepted(document.metadata.key, document.state.Snapshot());
            });
            if (pending == _inactiveDocuments.end())
            {
                _workspaceApprovals = std::move(approved);
                continuation();
                return;
            }
            if (! SwitchWorkspaceDocument(static_cast<std::size_t>(pending - _inactiveDocuments.begin())))
                return;
            if (! IsUiAttached())
                return;
        }
        const auto promptingSource = ActiveEditor()->Snapshot();
        const auto promptingKey    = _session.key;
        AskThen([this, promptingSource, promptingKey, continuation = std::move(continuation), approved = std::move(approved)]() mutable
        {
            approved.push_back({promptingKey, promptingSource});
            AskWorkspaceThen(std::move(continuation), std::move(approved), false);
        });
        return;
    }
    if (_options.headless
#if defined(ENABLE_TESTS)
        && ! _options.forceDxUiSaveDialogForTest
#endif
    )
        return;
    ShowSaveConfirmModal(std::move(continuation), closingApplication);
}

void Application::ShowSaveConfirmModal(std::function<void()> continuation, bool closingApplication, bool activeDocumentOnly)
{
    SaveConfirmSession session;
    session.continuation       = std::move(continuation);
    session.closingApplication = closingApplication;
    session.activeDocumentOnly = activeDocumentOnly;
    const auto untitled        = Resource(IDS_UNTITLED);
    if (Dirty())
    {
        SaveConfirmItem item;
        item.workspaceKey = _session.key;
        item.isActive     = true;
        item.selected     = true;
        item.location     = ResolveWorkspaceDisplayLocation(_session.path, _session.origin, _settings.filesRoots, untitled);
        session.items.push_back(std::move(item));
    }
    if (! activeDocumentOnly)
    {
        for (const auto& doc : _inactiveDocuments)
        {
            if (! doc.dirty)
                continue;
            SaveConfirmItem item;
            item.workspaceKey = doc.metadata.key;
            item.isActive     = false;
            item.selected     = true;
            item.location     = ResolveWorkspaceDisplayLocation(doc.metadata.path, doc.metadata.origin, _settings.filesRoots, untitled);
            session.items.push_back(std::move(item));
        }
    }
    if (session.items.empty())
    {
        if (session.continuation)
            session.continuation();
        return;
    }
    while (_saveConfirmRowControls.size() < session.items.size())
    {
        SaveConfirmRowControls row;
        row.checkbox = _saveConfirmScroll->AddChild<DxUi::Checkbox>();
        row.checkbox->SetChecked(true);
        const std::size_t rowIndex = _saveConfirmRowControls.size();
        row.checkbox->SetOnToggled([this, rowIndex](bool checked)
        {
            if (_syncingSaveConfirmChecks || ! _saveConfirmSession || rowIndex >= _saveConfirmSession->items.size())
                return;
            _saveConfirmSession->items[rowIndex].selected = checked;
            _syncingSaveConfirmChecks                     = true;
            if (_saveConfirmSelectAll)
                _saveConfirmSelectAll->SetChecked(std::ranges::all_of(_saveConfirmSession->items, &SaveConfirmItem::selected));
            _syncingSaveConfirmChecks = false;
            if (_saveConfirmPrimary)
                _saveConfirmPrimary->SetEnabled(std::ranges::any_of(_saveConfirmSession->items, &SaveConfirmItem::selected));
        });
        row.locationLabel = _saveConfirmScroll->AddChild<DxUi::Label>();
        row.locationLabel->SetFontRole(DxUi::FontRole::Small);
        _saveConfirmRowControls.push_back(row);
    }
    const bool multiDoc = session.items.size() > 1u;
    _saveConfirmTitle->SetText(Resource(multiDoc ? IDS_SAVE_CONFIRM_MULTI_TITLE : IDS_SAVE_CONFIRM_SINGLE_TITLE));
    if (_saveConfirmHint)
        _saveConfirmHint->SetText(Resource(closingApplication ? IDS_SAVE_CONFIRM_HINT : IDS_SAVE_CONFIRM_NONCLOSING_HINT));
    _saveConfirmPrimary->SetText(Resource(multiDoc ? IDS_SAVE_SELECTED : IDS_SAVE));
    _saveConfirmPrimary->SetEnabled(true);
    _saveConfirmDiscard->SetText(Resource(multiDoc ? IDS_DISCARD_ALL : IDS_DONT_SAVE));
    _saveConfirmKeepExit->SetVisible(closingApplication);
    _syncingSaveConfirmChecks = true;
    if (_saveConfirmSelectAll)
        _saveConfirmSelectAll->SetChecked(true);
    for (std::size_t i = 0; i < session.items.size(); ++i)
    {
        const auto& item           = session.items[i];
        const auto& rowControls    = _saveConfirmRowControls[i];
        const std::wstring locText = item.location.relativePath.empty()
                                         ? item.location.sectionLabel
                                         : FormatStringResource(nullptr, IDS_SAVE_LOCATION_FORMAT, item.location.sectionLabel, item.location.relativePath);
        rowControls.checkbox->SetChecked(true);
        rowControls.checkbox->SetText(item.location.fileName);
        rowControls.checkbox->SetTooltipText(item.location.fullPath);
        rowControls.locationLabel->SetText(locText);
        rowControls.locationLabel->SetTooltipText(item.location.fullPath);
    }
    _syncingSaveConfirmChecks = false;
    _saveConfirmSession       = std::move(session);
    SetActiveOverlay(OverlayKind::SaveConfirmModal);
    Layout();
    _host.SetFocusControl(_saveConfirmPrimary, MayRequestNativeFocus());
}

void Application::AdvanceSaveConfirmSequentialSave()
{
    if (! _saveConfirmSession)
        return;
    _saveConfirmSession->sequentialSaveActive = true;
    while (_saveConfirmSession->pendingSaveIndex < _saveConfirmSession->items.size())
    {
        const auto& item = _saveConfirmSession->items[_saveConfirmSession->pendingSaveIndex++];
        if (! item.selected)
            continue;
        if (item.workspaceKey != _session.key)
        {
            bool switched{};
            for (std::size_t idx = 0; idx < _inactiveDocuments.size(); ++idx)
                if (_inactiveDocuments[idx].metadata.key == item.workspaceKey)
                {
                    switched = SwitchWorkspaceDocument(idx);
                    break;
                }
            if (! switched)
            {
                _saveConfirmSession.reset();
                return;
            }
        }
        Save();
        if (! _files.Busy() && ! IsClosing())
        {
            // User cancelled file picker or validation failed synchronously.
            _saveConfirmSession.reset();
        }
        return;
    }
    auto cont             = std::move(_saveConfirmSession->continuation);
    const bool closingApp = _saveConfirmSession->closingApplication;
    const bool activeOnly = _saveConfirmSession->activeDocumentOnly;
    std::vector<unsigned> selectedKeys;
    selectedKeys.reserve(_saveConfirmSession->items.size());
    for (const auto& item : _saveConfirmSession->items)
        if (item.selected)
            selectedKeys.push_back(item.workspaceKey);
    _saveConfirmSession.reset();
    bool anySelectedStillDirty{};
    for (const unsigned key : selectedKeys)
    {
        if (key == _session.key)
        {
            if (Dirty())
            {
                anySelectedStillDirty = true;
                break;
            }
        }
        else
        {
            for (const auto& doc : _inactiveDocuments)
                if (doc.metadata.key == key && doc.dirty)
                {
                    anySelectedStillDirty = true;
                    break;
                }
            if (anySelectedStillDirty)
                break;
        }
    }
    if (anySelectedStillDirty)
    {
        if (activeOnly)
            AskThen(std::move(cont));
        else
            AskWorkspaceThen(std::move(cont), {}, closingApp);
        return;
    }
    if (cont)
        cont();
}

void Application::ShowRenameFileModal(const std::filesystem::path& path, bool isDirectory)
{
    if (! _renamePanel || path.empty())
        return;
    _renameSession = RenameFileSession{path, isDirectory};
    const auto loc = ResolveWorkspaceDisplayLocation(path, {}, _settings.filesRoots, path.filename().native());
    std::wstring locationLabel;
    if (loc.rooted && ! loc.sectionLabel.empty())
        locationLabel = SanitizeUiText(FormatStringResource(nullptr, IDS_SAVE_LOCATION_FORMAT, loc.sectionLabel, loc.relativePath), 240u);
    else
        locationLabel = SanitizeUiText(path.parent_path().filename().native(), 240u);
    _renameLocation->SetText(locationLabel);
    _renameInput->SetText(path.filename().native());
    _renameError->SetText(L"");
    if (_renameConfirmBtn)
        _renameConfirmBtn->SetEnabled(true);
    SetActiveOverlay(OverlayKind::RenameFileModal);
    Layout();
    _host.SetFocusControl(_renameInput, MayRequestNativeFocus());
}

void Application::CommitRenameFileModal()
{
    if (! _renameSession || ! _renameInput)
        return;
    const std::wstring newName(_renameInput->GetText());
    WindowsWorkspaceFileOps fallbackOps;
    IWorkspaceFileOps* ops = &fallbackOps;
#if defined(ENABLE_TESTS)
    if (_options.fileOpsForTest)
        ops = _options.fileOpsForTest.get();
#endif
    if (ops->ValidateFileName(newName) != FileNameValidationError::None)
    {
        if (_renameError)
            _renameError->SetText(Resource(IDS_RENAME_INVALID));
        return;
    }
    if (newName == _renameSession->targetPath.filename().native())
    {
        CloseDrawers();
        return;
    }
    WorkspaceFileOpRequest req;
    req.kind        = WorkspaceFileOpRequest::Kind::Rename;
    req.requestId   = ++_nextFileOpRequestId;
    req.sourcePath  = _renameSession->targetPath;
    req.newLeafName = newName;
    if (FAILED(_fileOps.Submit(std::move(req))))
    {
        Status(IDS_BUSY);
        return;
    }
    _renameSession.reset();
    SetActiveOverlay(OverlayKind::None);
    Layout();
    _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
}

void Application::ShowFileConflictModal(std::vector<std::filesystem::path> sources,
                                        std::filesystem::path targetDirectory,
                                        bool move,
                                        std::filesystem::path conflictName)
{
    if (! _fileConflictPanel)
        return;
    _fileConflictDesc->SetText(FormatStringResource(nullptr, IDS_FILE_CONFLICT_DESC, SanitizeUiText(conflictName.filename().native())));
    _fileConflictSession = FileConflictSession{std::move(sources), std::move(targetDirectory), std::move(conflictName), move};
    SetActiveOverlay(OverlayKind::FileConflictModal);
    Layout();
    _host.SetFocusControl(_conflictKeepBothBtn, MayRequestNativeFocus());
}

void Application::CommitFileConflictResolution(bool replaceExisting)
{
    if (! _fileConflictSession)
        return;
    auto session = std::move(*_fileConflictSession);
    _fileConflictSession.reset();
    SetActiveOverlay(OverlayKind::None);
    Layout();
    _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
    BeginCopyOrMoveToDirectory(std::move(session.sources), session.targetDirectory, session.move, replaceExisting, ! replaceExisting);
}

void Application::BeginCopyOrMoveToDirectory(
    std::vector<std::filesystem::path> sources, const std::filesystem::path& targetDirectory, bool move, bool forceOverwrite, bool forceNumbered)
{
    if (sources.empty() || targetDirectory.empty())
        return;
    if (forceOverwrite || forceNumbered)
    {
        if (_pendingPasteQueue.size() >= MaximumPendingPasteItems)
        {
            Status(IDS_WORKSPACEFULL);
            return;
        }
        _pendingPasteQueue.push_front({std::move(sources.front()), targetDirectory, move, forceOverwrite, forceNumbered});
    }
    else
    {
        if (sources.size() > MaximumPendingPasteItems || _pendingPasteQueue.size() > MaximumPendingPasteItems - sources.size())
        {
            Status(IDS_WORKSPACEFULL);
            return;
        }
        for (auto& src : sources)
            _pendingPasteQueue.push_back({std::move(src), targetDirectory, move, false, false});
    }
    DispatchNextPendingPaste();
}

void Application::DispatchNextPendingPaste()
{
    if (_activePasteRequestId || _fileConflictSession || _pendingPasteQueue.empty() || IsClosing() || IsSessionEnding())
        return;
    auto item = std::move(_pendingPasteQueue.front());
    _pendingPasteQueue.pop_front();
    WorkspaceFileOpRequest req;
    req.kind            = WorkspaceFileOpRequest::Kind::Paste;
    req.requestId       = ++_nextFileOpRequestId;
    req.sourcePath      = std::move(item.source);
    req.destinationPath = std::move(item.targetDirectory);
    req.dropEffect      = item.move ? DropEffectKind::Move : DropEffectKind::Copy;
    req.replaceExisting = item.forceOverwrite;
    req.keepBothCopy    = item.forceNumbered;
    const auto id       = req.requestId;
    if (SUCCEEDED(_fileOps.Submit(std::move(req))))
    {
        _activePasteRequestId = id;
    }
    else
    {
        _pendingPasteQueue.clear();
        Status(IDS_BUSY);
    }
}

void Application::FileOpCompleted()
{
    if (IsClosing())
    {
        FinishClose();
        return;
    }
    bool pausedForConflict{};
    while (auto result = _fileOps.TakeResult())
    {
        if (result->kind == WorkspaceFileOpRequest::Kind::Paste && _activePasteRequestId && result->requestId == *_activePasteRequestId)
            _activePasteRequestId.reset();

        if (result->requiresCollisionPrompt)
        {
            pausedForConflict = true;
            ShowFileConflictModal({result->outcome.sourcePath},
                                  result->outcome.destinationPath.parent_path(),
                                  result->dropEffect == DropEffectKind::Move,
                                  result->outcome.destinationPath);
            break;
        }
        if (FAILED(result->outcome.status))
        {
            if (_feedback)
                _feedback->SetText(FormatStringResource(nullptr, IDS_FILE_OP_FAILED, ErrorText(result->outcome.status)));
            continue;
        }
        if (result->kind == WorkspaceFileOpRequest::Kind::Rename)
        {
            RebaseWorkspacePathsAfterFileOp(result->outcome.sourcePath, result->outcome.destinationPath, result->destinationIsDirectory, true);
            if (_folderFiles.IsActive())
                (void)_folderFiles.Refresh();
            if (_multiFolderFiles.RootCount() > 0u)
                (void)_multiFolderFiles.RefreshAll();
            RefreshFiles(true);
        }
        else if (result->kind == WorkspaceFileOpRequest::Kind::Paste)
        {
            if (result->dropEffect == DropEffectKind::Move)
            {
                RebaseWorkspacePathsAfterFileOp(result->outcome.sourcePath, result->outcome.destinationPath, result->destinationIsDirectory, true);
                _cutFilePaths.erase(NormalizeRootPath(result->outcome.sourcePath).native());
            }
            if (_folderFiles.IsActive())
                (void)_folderFiles.Refresh();
            if (_multiFolderFiles.RootCount() > 0u)
                (void)_multiFolderFiles.RefreshAll();
            RefreshFiles(true);
        }
    }
    if (! pausedForConflict)
        DispatchNextPendingPaste();
}

std::wstring Application::FormatGitHubAuthStatusText() const
{
    if (! _githubDeviceCodePromptText.empty())
        return _githubDeviceCodePromptText;
    if (! _githubSignedInLogin.empty())
        return FormatStringResource(nullptr, IDS_GITHUB_CONNECTED_AS, SanitizeUiText(_githubSignedInLogin, 128u));
    return Resource(IDS_GITHUB_NOT_SIGNED_IN);
}

void Application::ClearGitHubTokenInputs()
{
    if (! IsUiAttached())
        return;
    if (_openGitHubTokenInput)
        _openGitHubTokenInput->SetText(L"");
    if (_accountsTokenInput)
        _accountsTokenInput->SetText(L"");
}

void Application::ClearPendingDeviceFlow() noexcept
{
    _pendingDeviceFlow.reset();
    _activeDeviceFlowId = 0u;
    _githubDeviceCodePromptText.clear();
    if (_window)
        (void)KillTimer(_window.get(), GitHubDevicePollTimerId);
}

void Application::ToggleGitHubRepoExpanded(std::wstring_view owner, std::wstring_view repo, std::optional<bool> forceExpanded)
{
    const std::wstring key = std::wstring(owner) + L"/" + std::wstring(repo);
    const bool expand      = forceExpanded.value_or(_collapsedGitHubRepos.contains(key));
    if (expand)
    {
        _collapsedGitHubRepos.erase(key);
        for (auto& connected : _connectedGitHubRepos)
        {
            if (EqualOrdinalIgnoreCase(connected.ref.owner, owner) && EqualOrdinalIgnoreCase(connected.ref.repo, repo) && ! connected.loaded &&
                ! connected.loading)
            {
                GitHubRequest req;
                req.kind      = GitHubRequest::Kind::FetchRepoTree;
                req.requestId = ++_nextGitHubRequestId;
                req.owner     = connected.ref.owner;
                req.repo      = connected.ref.repo;
                req.branch    = connected.ref.branch;
                if (SUCCEEDED(_github.Submit(std::move(req))))
                    connected.loading = true;
                else
                    Status(IDS_BUSY);
            }
        }
    }
    else
    {
        _collapsedGitHubRepos.insert(key);
    }
    RefreshFiles();
}

void Application::RefreshGitHubAuthControls()
{
    const std::wstring statusText = FormatGitHubAuthStatusText();
    if (_openGitHubAuthLabel)
        _openGitHubAuthLabel->SetText(statusText);
    if (_openGitHubImportGhBtn)
        _openGitHubImportGhBtn->SetEnabled(! _githubImportInProgress);
    if (_openGitHubDeviceBtn)
        _openGitHubDeviceBtn->SetVisible(_github.DeviceFlowConfigured());
    RefreshAccountsDrawerControls();
}

void Application::RefreshAccountsDrawerControls()
{
    const bool signedIn = ! _githubSignedInLogin.empty();
    if (_accountsStatusLabel)
        _accountsStatusLabel->SetText(FormatGitHubAuthStatusText());
    if (_accountsImportGhBtn)
        _accountsImportGhBtn->SetEnabled(! _githubImportInProgress);
    if (_accountsDeviceBtn)
        _accountsDeviceBtn->SetVisible(_github.DeviceFlowConfigured());
    if (_accountsSignOutBtn)
        _accountsSignOutBtn->SetEnabled(signedIn);
}

void Application::RefreshOpenGitHubSuggestTree()
{
    if (! _githubSuggestTree || ! _openGitHubUrlInput)
        return;
    const std::wstring query(_openGitHubUrlInput->GetText());
    _githubSuggestMatches = FilterAndRankGitHubRepos(_githubUserRepoCache, query, 200u);
    _githubSuggestModel.rows.clear();
    for (std::size_t i = 0; i < _githubSuggestMatches.size(); ++i)
    {
        const auto& repo = _githubSuggestMatches[i];
        DxUi::TreeItemData item;
        item.id          = static_cast<std::uint64_t>(i + 1u);
        item.text        = SanitizeUiText(repo.fullName, 256u);
        item.iconText    = repo.isPrivate ? L"\xE72E" : L"\xE774";
        item.badgeText   = SanitizeUiText(repo.defaultBranch, 64u);
        item.tooltipText = SanitizeUiText(repo.htmlUrl, 512u);
        _githubSuggestModel.rows.push_back(std::move(item));
    }
    if (_githubSignedInLogin.empty())
        _githubSuggestTree->SetEmptyStateText(Resource(IDS_GITHUB_NOT_SIGNED_IN));
    else if (! query.empty())
        _githubSuggestTree->SetEmptyStateText(Resource(IDS_NOMATCH));
    else
        _githubSuggestTree->SetEmptyStateText(Resource(IDS_GITHUB_NO_REPOS));
    _githubSuggestTree->NotifyDataChanged();
}

void Application::ShowOpenGitHubModal()
{
    if (! _openGitHubPanel)
        return;
    if (_openGitHubErrorLabel)
        _openGitHubErrorLabel->SetText(L"");
    RefreshGitHubAuthControls();
    RefreshOpenGitHubSuggestTree();
    SetActiveOverlay(OverlayKind::OpenGitHubModal);
    Layout();
    _host.SetFocusControl(_openGitHubUrlInput, MayRequestNativeFocus());
    if (_options.capture.empty() && _githubSignedInLogin.empty() && _githubUserRepoCache.empty())
    {
        GitHubRequest req;
        req.kind      = GitHubRequest::Kind::LoadSavedAccount;
        req.requestId = ++_nextGitHubRequestId;
        (void)_github.Submit(std::move(req));
    }
}

void Application::CommitOpenGitHubModal()
{
    if (! _openGitHubUrlInput)
        return;
    const auto parsed = ParseGitHubUrl(_openGitHubUrlInput->GetText());
    if (! parsed.valid)
    {
        if (_openGitHubErrorLabel)
            _openGitHubErrorLabel->SetText(FormatLocalizedGitHubError(parsed.errorCode));
        return;
    }
    std::wstring branch = _openGitHubBranchInput ? std::wstring(_openGitHubBranchInput->GetText()) : std::wstring{};
    if (branch.empty())
        branch = parsed.branch;
    if (! branch.empty() && ! IsValidGitHubBranchName(branch))
    {
        if (_openGitHubErrorLabel)
            _openGitHubErrorLabel->SetText(Resource(IDS_GITHUB_ERR_INVALID_BRANCH));
        return;
    }

    GitHubRepoRef ref{parsed.owner, parsed.repo, branch};
    auto existing = std::ranges::find_if(
        _connectedGitHubRepos, [&](const auto& r) { return EqualOrdinalIgnoreCase(r.ref.owner, ref.owner) && EqualOrdinalIgnoreCase(r.ref.repo, ref.repo); });
    if (existing == _connectedGitHubRepos.end())
    {
        if (_connectedGitHubRepos.size() >= MaximumGitHubRepos)
        {
            Status(IDS_WORKSPACEFULL);
            return;
        }
        GitHubConnectedRepoState state;
        state.ref = ref;
        _connectedGitHubRepos.push_back(std::move(state));
        existing = std::prev(_connectedGitHubRepos.end());
    }
    else
    {
        existing->ref.branch = branch;
        existing->resolvedBranch.clear();
        existing->entries.clear();
        existing->loaded       = false;
        existing->truncated    = false;
        existing->limitReached = false;
        existing->lastError    = S_OK;
        existing->errorMessage.clear();
    }
    _settings.githubRepos.clear();
    for (const auto& r : _connectedGitHubRepos)
        _settings.githubRepos.push_back(r.ref);
    _settings.showFiles     = true;
    _settings.filesPinMode  = PanePinMode::Pinned;
    _lastPinnedSide         = PaneSide::Left;
    _githubSectionCollapsed = false;
    _collapsedGitHubRepos.erase(existing->ref.owner + L"/" + existing->ref.repo);
    SavePreferences();

    GitHubRequest treeReq;
    treeReq.kind      = GitHubRequest::Kind::FetchRepoTree;
    treeReq.requestId = ++_nextGitHubRequestId;
    treeReq.owner     = existing->ref.owner;
    treeReq.repo      = existing->ref.repo;
    treeReq.branch    = branch;
    if (SUCCEEDED(_github.Submit(std::move(treeReq))))
        existing->loading = true;
    else
    {
        existing->loading = false;
        Status(IDS_BUSY);
    }

    SetActiveOverlay(OverlayKind::None);
    if (! parsed.filePath.empty())
        OpenGitHubDocument(existing->ref.owner, existing->ref.repo, branch, parsed.filePath);

    RefreshFiles();
    Layout();
    _host.SetFocusControl(_filesTree, MayRequestNativeFocus());
}

void Application::RemoveConnectedGitHubRepo(std::wstring_view owner, std::wstring_view repo)
{
    if (std::erase_if(_connectedGitHubRepos,
                      [&](const auto& r) { return EqualOrdinalIgnoreCase(r.ref.owner, owner) && EqualOrdinalIgnoreCase(r.ref.repo, repo); }) == 0u)
        return;
    _settings.githubRepos.clear();
    for (const auto& r : _connectedGitHubRepos)
        _settings.githubRepos.push_back(r.ref);
    _collapsedGitHubRepos.erase(std::wstring(owner) + L"/" + std::wstring(repo));
    RefreshFiles();
    Layout();
    SavePreferences();
}

void Application::OpenGitHubDocument(std::wstring_view owner, std::wstring_view repo, std::wstring_view branch, std::wstring_view path)
{
    if (HasActiveModalUi() || ! CanAcceptCommand())
        return;
    GitHubRequest req;
    req.kind      = GitHubRequest::Kind::FetchRawFile;
    req.requestId = ++_nextGitHubRequestId;
    req.owner     = std::wstring(owner);
    req.repo      = std::wstring(repo);
    req.branch    = std::wstring(branch);
    req.filePath  = std::wstring(path);
    if (SUCCEEDED(_github.Submit(std::move(req))))
        Status(IDS_LOADING);
    else
        Status(IDS_BUSY);
}

void Application::GitHubCompleted()
{
    if (IsClosing())
    {
        FinishClose();
        return;
    }
    bool authUiUpdated = false;
    while (auto result = _github.TakeResult())
    {
        const auto handleAuthInvalidation = [&]
        {
            if (! result->authInvalidated && result->httpStatusCode != 401u)
                return;
            _githubSignedInLogin.clear();
            _githubUserRepoCache.clear();
            RefreshGitHubAuthControls();
            RefreshOpenGitHubSuggestTree();
            authUiUpdated = true;
        };

        switch (result->kind)
        {
            case GitHubRequest::Kind::LoadSavedAccount:
            case GitHubRequest::Kind::SignInWithPat:
            case GitHubRequest::Kind::ImportGhCli:
            {
                if (result->kind == GitHubRequest::Kind::ImportGhCli)
                    _githubImportInProgress = false;
                if (SUCCEEDED(result->status) && result->signedIn)
                {
                    _githubSignedInLogin = result->accountLogin;
                    _githubDeviceCodePromptText.clear();
                    _githubUserRepoCache = std::move(result->userRepos);
                    if (_openGitHubErrorLabel)
                        _openGitHubErrorLabel->SetText(L"");
                }
                else
                {
                    if (result->authInvalidated || result->httpStatusCode == 401u ||
                        (result->kind == GitHubRequest::Kind::LoadSavedAccount && SUCCEEDED(result->status)))
                    {
                        _githubSignedInLogin.clear();
                        _githubUserRepoCache.clear();
                    }
                    if (FAILED(result->status) && ! result->errorMessage.empty())
                    {
                        const std::wstring msg = FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, result->errorMessage);
                        if (_openGitHubErrorLabel)
                            _openGitHubErrorLabel->SetText(msg);
                        if (_feedback)
                            _feedback->SetText(msg);
                    }
                }
                RefreshGitHubAuthControls();
                RefreshOpenGitHubSuggestTree();
                authUiUpdated = true;
                break;
            }
            case GitHubRequest::Kind::RefreshUserRepos:
                if (SUCCEEDED(result->status) && result->signedIn)
                {
                    _githubSignedInLogin = result->accountLogin;
                    _githubUserRepoCache = std::move(result->userRepos);
                }
                else
                {
                    handleAuthInvalidation();
                }
                RefreshGitHubAuthControls();
                RefreshOpenGitHubSuggestTree();
                authUiUpdated = true;
                break;
            case GitHubRequest::Kind::StartDeviceFlow:
                if (result->flowId == 0u || result->flowId != _activeDeviceFlowId ||
                    (_activeOverlay != OverlayKind::OpenGitHubModal && _activeOverlay != OverlayKind::SettingsDrawer))
                    break;
                if (SUCCEEDED(result->status) && result->deviceCode && SupportedExternalLink(result->deviceCode->verificationUri))
                {
                    PendingDeviceFlowState flow;
                    flow.info                   = *result->deviceCode;
                    flow.flowId                 = result->flowId;
                    flow.currentIntervalSeconds = std::clamp(result->deviceCode->intervalSeconds, 5u, 120u);
                    flow.deadlineTickMs         = GetTickCount64() + static_cast<ULONGLONG>(result->deviceCode->expiresInSeconds) * 1000ull;
                    _pendingDeviceFlow          = std::move(flow);
                    _githubDeviceCodePromptText = FormatStringResource(nullptr,
                                                                       IDS_GITHUB_DEVICE_CODE_PROMPT,
                                                                       SanitizeUiText(_pendingDeviceFlow->info.userCode, 64u),
                                                                       SanitizeUiText(_pendingDeviceFlow->info.verificationUri, 256u));
                    if (_openGitHubErrorLabel)
                        _openGitHubErrorLabel->SetText(L"");
                    RefreshGitHubAuthControls();
                    authUiUpdated = true;
                    if (_window)
                        (void)SetTimer(_window.get(), GitHubDevicePollTimerId, _pendingDeviceFlow->currentIntervalSeconds * 1000u, nullptr);
                }
                else
                {
                    _activeDeviceFlowId    = 0u;
                    const std::wstring err = ! result->errorMessage.empty()
                                                 ? result->errorMessage
                                                 : FormatLocalizedGitHubError(result->errorCode, result->status, result->httpStatusCode);
                    const std::wstring msg = FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, err);
                    if (_openGitHubErrorLabel)
                        _openGitHubErrorLabel->SetText(msg);
                    if (_feedback)
                        _feedback->SetText(msg);
                    RefreshGitHubAuthControls();
                    authUiUpdated = true;
                }
                break;
            case GitHubRequest::Kind::PollDeviceFlow:
            {
                if (! _pendingDeviceFlow || result->flowId != _pendingDeviceFlow->flowId)
                    break;
                if (! result->devicePoll)
                {
                    ClearPendingDeviceFlow();
                    const std::wstring err = ! result->errorMessage.empty()
                                                 ? result->errorMessage
                                                 : FormatLocalizedGitHubError(result->errorCode, result->status, result->httpStatusCode);
                    const std::wstring msg = FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, err);
                    if (_openGitHubErrorLabel)
                        _openGitHubErrorLabel->SetText(msg);
                    if (_feedback)
                        _feedback->SetText(msg);
                    RefreshGitHubAuthControls();
                    authUiUpdated = true;
                    break;
                }
                const auto& poll = *result->devicePoll;
                if (poll.state == GitHubDevicePollResult::State::Authorized)
                {
                    ClearPendingDeviceFlow();
                    if (SUCCEEDED(result->status) && result->signedIn)
                    {
                        _githubSignedInLogin = result->accountLogin;
                        _githubUserRepoCache = std::move(result->userRepos);
                        if (_openGitHubErrorLabel)
                            _openGitHubErrorLabel->SetText(L"");
                    }
                    else
                    {
                        const std::wstring err = ! result->errorMessage.empty()
                                                     ? result->errorMessage
                                                     : FormatLocalizedGitHubError(result->errorCode, result->status, result->httpStatusCode);
                        const std::wstring msg = FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, err);
                        if (_openGitHubErrorLabel)
                            _openGitHubErrorLabel->SetText(msg);
                        if (_feedback)
                            _feedback->SetText(msg);
                    }
                    RefreshGitHubAuthControls();
                    RefreshOpenGitHubSuggestTree();
                    authUiUpdated = true;
                }
                else if (poll.state == GitHubDevicePollResult::State::Pending || poll.state == GitHubDevicePollResult::State::SlowDown)
                {
                    if (poll.state == GitHubDevicePollResult::State::SlowDown)
                    {
                        _pendingDeviceFlow->currentIntervalSeconds =
                            std::min(120u, std::max(_pendingDeviceFlow->currentIntervalSeconds + 5u, poll.serverIntervalSeconds.value_or(0u)));
                    }
                    else if (poll.serverIntervalSeconds)
                    {
                        _pendingDeviceFlow->currentIntervalSeconds =
                            std::clamp(std::max(_pendingDeviceFlow->currentIntervalSeconds, *poll.serverIntervalSeconds), 5u, 120u);
                    }
                    if (GetTickCount64() >= _pendingDeviceFlow->deadlineTickMs)
                    {
                        ClearPendingDeviceFlow();
                        if (_openGitHubErrorLabel)
                            _openGitHubErrorLabel->SetText(Resource(IDS_GITHUB_ERR_DEVICE_EXPIRED));
                        Status(IDS_GITHUB_ERR_DEVICE_EXPIRED);
                        RefreshGitHubAuthControls();
                        authUiUpdated = true;
                    }
                    else if (_window)
                    {
                        (void)SetTimer(_window.get(), GitHubDevicePollTimerId, _pendingDeviceFlow->currentIntervalSeconds * 1000u, nullptr);
                    }
                }
                else
                {
                    ClearPendingDeviceFlow();
                    const std::wstring err =
                        ! result->errorMessage.empty()
                            ? result->errorMessage
                            : FormatLocalizedGitHubError(poll.state == GitHubDevicePollResult::State::AccessDenied
                                                             ? GitHubErrorCode::DeviceAccessDenied
                                                             : (poll.state == GitHubDevicePollResult::State::Expired ? GitHubErrorCode::DeviceExpired
                                                                                                                     : GitHubErrorCode::InvalidResponse));
                    const std::wstring msg = FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, err);
                    if (_openGitHubErrorLabel)
                        _openGitHubErrorLabel->SetText(msg);
                    if (_feedback)
                        _feedback->SetText(msg);
                    RefreshGitHubAuthControls();
                    authUiUpdated = true;
                }
                break;
            }
            case GitHubRequest::Kind::SignOut:
                ClearPendingDeviceFlow();
                ClearGitHubTokenInputs();
                _githubSignedInLogin.clear();
                _githubUserRepoCache.clear();
                RefreshGitHubAuthControls();
                RefreshOpenGitHubSuggestTree();
                authUiUpdated = true;
                break;
            case GitHubRequest::Kind::FetchRepoTree:
            {
                const std::wstring_view ownerView = result->tree ? std::wstring_view(result->tree->owner) : std::wstring_view(result->fileOwner);
                const std::wstring_view repoView  = result->tree ? std::wstring_view(result->tree->repo) : std::wstring_view(result->fileRepo);
                for (auto& connected : _connectedGitHubRepos)
                {
                    if (EqualOrdinalIgnoreCase(connected.ref.owner, ownerView) && EqualOrdinalIgnoreCase(connected.ref.repo, repoView))
                    {
                        connected.loading      = false;
                        connected.loaded       = SUCCEEDED(result->status) && result->tree.has_value();
                        connected.lastError    = result->status;
                        connected.errorMessage = result->errorMessage;
                        if (result->tree)
                        {
                            connected.resolvedBranch = result->tree->branch;
                            connected.entries        = std::move(result->tree->entries);
                            connected.truncated      = result->tree->truncated;
                            connected.limitReached   = result->tree->limitReached;
                        }
                        else if (! result->fileBranch.empty())
                        {
                            connected.resolvedBranch = result->fileBranch;
                        }
                        break;
                    }
                }
                handleAuthInvalidation();
                if (FAILED(result->status) && ! result->errorMessage.empty() && _feedback)
                    _feedback->SetText(FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, result->errorMessage));
                RefreshFiles();
                break;
            }
            case GitHubRequest::Kind::FetchRawFile:
            {
                handleAuthInvalidation();
                if (FAILED(result->status))
                {
                    if (_feedback && ! result->errorMessage.empty())
                        _feedback->SetText(FormatStringResource(nullptr, IDS_GITHUB_FETCH_FAILED, result->errorMessage));
                    break;
                }
                if (! CanAcceptCommand() || HasActiveModalUi() || _files.Busy() || IsSessionEnding())
                {
                    Status(_files.Busy() ? IDS_BUSY : IDS_FILECONTEXTCHANGED);
                    break;
                }
                const auto prevCount = _inactiveDocuments.size();
                if (StartWorkspaceDocument() && IsUiAttached())
                {
                    if (SetDocument(std::move(result->fileText), {}, result->fileBom, {}, false))
                    {
                        _session.origin.kind         = DocumentOriginInfo::Kind::GitHub;
                        _session.origin.owner        = result->fileOwner;
                        _session.origin.repo         = result->fileRepo;
                        _session.origin.branch       = result->fileBranch;
                        _session.origin.relativePath = result->fileRelativePath;
                        _session.origin.blobSha      = result->fileSha;
                        _session.origin.htmlUrl      = result->fileHtmlUrl;
                        const auto displayName       = ActiveDocumentDisplayName();
                        if (_title)
                            _title->SetText(displayName);
                        UpdateCaption(FormatStringResource(nullptr, IDS_WINDOWTITLE, Dirty() ? L"\x2022 " : L"", displayName));
                        Status(IDS_GITHUB_READONLY_NOTICE);
                        RefreshLeftRailDots();
                        RefreshFiles();
                        Layout();
                    }
                    else if (_inactiveDocuments.size() > prevCount && SwitchWorkspaceDocument(prevCount))
                    {
                        _inactiveDocuments.pop_back();
                    }
                }
                break;
            }
        }
    }
    if (authUiUpdated && (_activeOverlay == OverlayKind::SettingsDrawer || _activeOverlay == OverlayKind::OpenGitHubModal))
        Layout();
}
} // namespace RedTilde
