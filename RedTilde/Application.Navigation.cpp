#include "Application.h"
#include "res/Resource.h"
#include <algorithm>
#include <format>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
// Application-owned ordinal Find policy preserves counted source ACPs and uses
// the shared editor boundary predicate instead of searching rendered text.
void CollectSourceFindMatches(std::wstring_view text, std::wstring_view query, bool matchCase, std::vector<DxUi::RichText::MarkdownRange>& matches)
{
    matches.clear();
    if (query.empty() || query.size() > text.size())
        return;
    std::size_t from{};
    while (from <= text.size() - query.size())
    {
        std::size_t position = std::wstring_view::npos;
        if (matchCase)
            position = text.find(query, from);
        else
        {
            const int found =
                FindStringOrdinal(FIND_FROMSTART, text.data() + from, static_cast<int>(text.size() - from), query.data(), static_cast<int>(query.size()), TRUE);
            if (found >= 0)
                position = from + static_cast<std::size_t>(found);
        }
        if (position == std::wstring_view::npos)
            break;
        if (DxUi::RichText::IsEditBoundary(text, position) && DxUi::RichText::IsEditBoundary(text, position + query.size()))
            matches.push_back({position, position + query.size()});
        from = position + query.size();
    }
}
} // namespace
bool Application::PreviewOutlineMatchesCommitted() const
{
    return _outlinePreviewBaseline && _documentInfo.headings.size() == _outlinePreviewBaseline->committedOutline.size() &&
           std::equal(_documentInfo.headings.begin(),
                      _documentInfo.headings.end(),
                      _outlinePreviewBaseline->committedOutline.begin(),
                      [](const Heading& heading, const auto& identity) { return heading.level == identity.first && heading.text == identity.second; });
}
void Application::RefreshOutline()
{
    ++_derivedCaches.outlineRowsRevision;
    const auto previousRows = _outlineModel.rows.size();
    _outlineModel.rows.clear();
    std::vector<std::pair<unsigned, std::uint64_t>> ancestors;
    for (std::size_t i = 0; i < _documentInfo.headings.size(); ++i)
    {
        const auto& heading = _documentInfo.headings[i];
        if (heading.text.empty())
            continue;
        while (! ancestors.empty() && ancestors.back().first >= heading.level)
            ancestors.pop_back();
        DxUi::TreeItemData item;
        item.id    = i + 1;
        item.text  = heading.text;
        item.depth = static_cast<unsigned>(ancestors.size());
        if (! ancestors.empty())
            item.parentId = ancestors.back().second;
        item.expanded    = ! _session.collapsedHeadings.contains(item.id);
        item.hasChildren = i + 1 < _documentInfo.headings.size() && _documentInfo.headings[i + 1].level > heading.level;
        const bool hidden =
            std::any_of(ancestors.begin(), ancestors.end(), [&](const auto& ancestor) { return _session.collapsedHeadings.contains(ancestor.second); });
        if (! hidden)
            _outlineModel.rows.push_back(std::move(item));
        ancestors.emplace_back(heading.level, i + 1);
    }
    _outlineTree->NotifyDataChanged();
    const auto headingCount = _documentInfo.headings.size();
    _outlineCount->SetText(FormatStringResource(nullptr, IDS_OUTLINECOUNT, headingCount));
    if (_outlineModel.rows.size() != previousRows)
        Layout();
    RefreshRightRailDots();
    if (_activeOverlay == OverlayKind::FindBar)
        RefreshFind();
}
void Application::UpdateCurrentFileDirtyBadge(bool dirty)
{
    _lastFilesDirty = dirty;
    RefreshLeftRailDots();
    auto* const tree    = _filesTree;
    const auto lifetime = GetControlLifetimeToken(*tree);
    const auto key      = _session.key;
    if (! _inactiveDocuments.empty())
    {
        const auto id   = WorkspaceGroupId + _session.key + 1;
        const auto item = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [&](const auto& row) { return row.id == id; });
        if (item != _filesModel.rows.end())
        {
            const auto badge = dirty ? Resource(IDS_FILESEDITED) : std::wstring{};
            if (item->badgeText != badge || item->badgeIsDot != dirty)
            {
                item->badgeText      = badge;
                item->badgeIsDot     = dirty;
                const bool published = tree->NotifyItemPresentationChanged(id);
                if (IsClosing() || lifetime.expired() || _filesTree != tree || _host.GetRoot() != _root || _session.key != key || Dirty() != dirty)
                    return;
                if (! published)
                {
                    RefreshFiles();
                    return;
                }
            }
        }
    }
    const auto requested = _settings.showFiles ? _settings.filesRoot : std::filesystem::path{};
    if (requested != _folderRoot)
    {
        RefreshFiles();
        return;
    }
    if (! _folderSnapshot || _folderSnapshot->root != requested)
        return;
    const auto* const model = _filesTree->GetModel();
    if (model != &_filesModel)
    {
        RefreshFiles();
        return;
    }

    const auto& snapshot = _folderSnapshot.value();
    const auto entry     = std::find_if(snapshot.entries.begin(), snapshot.entries.end(), [&](const auto& candidate) {
        return ! candidate.directory && MatchesFilesPath(snapshot, candidate.path, _session.path);
    });
    if (entry == snapshot.entries.end())
        return;
    const auto itemId = static_cast<std::uint64_t>(std::distance(snapshot.entries.begin(), entry) + 1);
    auto row          = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [&](const auto& candidate) { return candidate.id == itemId; });
    if (row == _filesModel.rows.end())
        return; // Filtering or a future visibility rule hides this file; a later structural refresh reads Dirty().

    const bool edited    = dirty || std::ranges::any_of(_inactiveDocuments, [&](const auto& document) {
        return document.dirty && MatchesFilesPath(snapshot, document.metadata.path, _session.path);
    });
    const auto badgeText = edited ? Resource(IDS_FILESEDITED) : std::wstring{};
    if (row->badgeText == badgeText && row->badgeIsDot == edited)
        return;
    row->badgeText  = badgeText;
    row->badgeIsDot = edited;

    const auto snapshotGeneration = snapshot.generation;
    const auto snapshotRevision   = snapshot.revision;
    const auto modelRebuilds      = _activity.filesModelRebuilds;
    if (_filesTree->NotifyItemPresentationChanged(itemId))
        return;

    // UI Automation callbacks can re-enter the app. Fall back to a full refresh
    // only while the exact snapshot/model publication we just changed is still current.
    if (model != &_filesModel || _filesTree->GetModel() != model || ! _folderSnapshot || _folderSnapshot->root != requested ||
        _folderSnapshot->generation != snapshotGeneration || _folderSnapshot->revision != snapshotRevision || Dirty() != dirty || _lastFilesDirty != dirty ||
        _activity.filesModelRebuilds != modelRebuilds)
        return;
    const auto currentRow = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [&](const auto& candidate) { return candidate.id == itemId; });
    if (currentRow != _filesModel.rows.end() && currentRow->badgeText == badgeText && currentRow->badgeIsDot == edited)
        RefreshFiles();
}
void Application::RefreshFiles(bool rootChanged)
{
    const auto setFooter = [&](std::wstring text)
    {
        if (_filesCount->GetText() != text)
        {
            _filesCount->SetText(std::move(text));
            Layout();
        }
    };
    const auto requested = _settings.showFiles ? _settings.filesRoot : std::filesystem::path{};
    if (requested != _folderRoot || (rootChanged && ! requested.empty() && ! _folderFiles.IsActive()))
    {
        _folderRoot = requested;
        _folderSnapshot.reset();
        _collapsedFolders.clear();
        HRESULT result = S_OK;
        if (requested.empty())
            _folderFiles.Close();
        else
        {
            if (! _folderFiles.IsActive())
                result = _folderFiles.Initialize(_window.get(), FolderReady);
            if (_folderFiles.IsActive())
                result = _folderFiles.SetRoot(requested);
        }
        _folderAdmissionStatus = result;
    }
    if (rootChanged && _folderSnapshot)
        for (const auto& entry : _folderSnapshot->entries)
            if (entry.directory)
            {
                const auto relative = _session.path.lexically_relative(entry.path);
                if (! relative.empty() && *relative.begin() != L"..")
                    _collapsedFolders.erase(entry.path);
            }
    _filesTitle->SetText(requested.empty() ? Resource(IDS_FILES) : requested.filename().empty() ? requested.native() : requested.filename().native());
    _filesTitle->SetTooltipText(requested.native());
    _filesTitle->SetAccessibleHelpText(requested.native());
    _lastFilesDirty = Dirty();
    RefreshLeftRailDots();
    ++_activity.filesModelRebuilds;
    _filesModel.rows.clear();
    _visibleFilePaths.clear();
    const std::wstring filter(_filesFilter->GetText());
    if (! _inactiveDocuments.empty())
    {
        std::vector<DxUi::TreeItemData> documents;
        documents.reserve(_inactiveDocuments.size() + 1);
        const auto append = [&](unsigned key, const std::filesystem::path& path, bool dirty)
        {
            const auto name = path.empty() ? Resource(IDS_UNTITLED) : path.filename().native();
            if (! filter.empty() && FindNLSStringEx(LOCALE_NAME_INVARIANT,
                                                    FIND_FROMSTART | NORM_IGNORECASE,
                                                    name.data(),
                                                    static_cast<int>(name.size()),
                                                    filter.data(),
                                                    static_cast<int>(filter.size()),
                                                    nullptr,
                                                    nullptr,
                                                    nullptr,
                                                    0) < 0)
                return;
            DxUi::TreeItemData item;
            item.id          = WorkspaceGroupId + key + 1;
            item.parentId    = WorkspaceGroupId;
            item.depth       = 1;
            item.text        = name;
            item.iconText    = L"\xE8A5";
            item.tooltipText = path.native();
            item.badgeIsDot  = dirty;
            if (dirty)
                item.badgeText = Resource(IDS_FILESEDITED);
            documents.push_back(std::move(item));
        };
        append(_session.key, _session.path, Dirty());
        for (const auto& document : _inactiveDocuments)
            append(document.metadata.key, document.metadata.path, document.dirty);
        std::ranges::sort(documents, {}, &DxUi::TreeItemData::id);
        if (! documents.empty())
        {
            DxUi::TreeItemData group;
            group.id            = WorkspaceGroupId;
            group.text          = Resource(IDS_OPENDOCUMENTS);
            group.hasChildren   = true;
            group.expanded      = ! _workspaceCollapsed || ! filter.empty();
            const bool expanded = group.expanded;
            _filesModel.rows.push_back(std::move(group));
            if (expanded)
                for (auto& document : documents)
                    _filesModel.rows.push_back(std::move(document));
        }
    }
    const std::uint64_t workspaceItemId = WorkspaceGroupId + _session.key + 1;
    const bool hasVisibleWorkspaceRow =
        ! _inactiveDocuments.empty() && std::ranges::any_of(_filesModel.rows, [&](const auto& row) { return row.id == workspaceItemId; });
    if (! _folderSnapshot)
    {
        setFooter(Resource(static_cast<unsigned>(requested.empty()                ? IDS_FILESUNTITLED
                                                 : FAILED(_folderAdmissionStatus) ? IDS_FILESUNAVAILABLE
                                                                                  : IDS_FILESLOADING)));
        _filesTree->SetEmptyStateText(Resource(static_cast<unsigned>(requested.empty()                ? IDS_FILESUNTITLED
                                                                     : FAILED(_folderAdmissionStatus) ? IDS_FILESUNAVAILABLE
                                                                                                      : IDS_FILESLOADING)));
        _filesTree->NotifyDataChanged();
        _filesTree->SetSelectedItemId(hasVisibleWorkspaceRow ? std::optional<std::uint64_t>(workspaceItemId) : std::nullopt);
        return;
    }
    const auto& snapshot = _folderSnapshot.value();
    _visibleFilePaths.reserve(snapshot.entries.size());
    for (const auto& entry : snapshot.entries)
        _visibleFilePaths.push_back(entry.path);
    for (const auto index : FilterMarkdownFiles(snapshot, filter))
    {
        const auto& entry = snapshot.entries[index];
        bool hidden{};
        if (filter.empty())
            for (auto parent = entry.parent; parent != NoFilesParent; parent = snapshot.entries[parent].parent)
                if (_collapsedFolders.contains(snapshot.entries[parent].path))
                {
                    hidden = true;
                    break;
                }
        if (hidden)
            continue;
        DxUi::TreeItemData item;
        item.id          = index + 1;
        item.text        = entry.name;
        item.iconText    = entry.directory ? L"\xE8B7" : L"\xE8A5";
        item.tooltipText = entry.path.native();
        item.depth       = entry.depth;
        if (entry.parent != NoFilesParent)
            item.parentId = entry.parent + 1;
        item.hasChildren = entry.directory && ! entry.reparsePoint && entry.markdownCount > 0;
        item.expanded    = item.hasChildren && (! filter.empty() || ! _collapsedFolders.contains(entry.path));
        if (entry.directory && ! item.expanded)
            item.badgeText = std::to_wstring(entry.markdownCount);
        else if ((MatchesFilesPath(snapshot, entry.path, _session.path) && Dirty()) || std::ranges::any_of(_inactiveDocuments, [&](const auto& document) {
            return document.dirty && MatchesFilesPath(snapshot, entry.path, document.metadata.path);
        }))
        {
            item.badgeText  = Resource(IDS_FILESEDITED);
            item.badgeIsDot = true;
        }
        _filesModel.rows.push_back(std::move(item));
    }
    _filesTree->SetEmptyStateText(Resource(IDS_NOMATCH));
    _filesTree->NotifyDataChanged();
    std::optional<std::uint64_t> visibleFolderItemId;
    const auto current =
        std::find_if(_visibleFilePaths.begin(), _visibleFilePaths.end(), [&](const auto& path) { return MatchesFilesPath(snapshot, path, _session.path); });
    if (current != _visibleFilePaths.end())
    {
        const auto candidateId = static_cast<std::uint64_t>(current - _visibleFilePaths.begin() + 1);
        if (std::ranges::any_of(_filesModel.rows, [&](const auto& row) { return row.id == candidateId; }))
            visibleFolderItemId = candidateId;
    }
    std::optional<std::uint64_t> selectedId;
    if (_workspaceSelectionPreferred && hasVisibleWorkspaceRow)
        selectedId = workspaceItemId;
    else if (visibleFolderItemId)
        selectedId = visibleFolderItemId;
    else if (hasVisibleWorkspaceRow)
        selectedId = workspaceItemId;
    _filesTree->SetSelectedItemId(selectedId);
    auto footer       = FormatStringResource(nullptr, IDS_FILESCOUNT, snapshot.markdownCount);
    const auto append = [&](std::wstring detail) { footer = FormatStringResource(nullptr, IDS_FILESFOOTERDETAIL, footer, detail); };
    if (FAILED(snapshot.status) || snapshot.errorCount)
        append(Resource(IDS_FILESACCESSDETAIL));
    if (FAILED(snapshot.watchStatus))
        append(Resource(IDS_FILESWATCHDETAIL));
    if (snapshot.entryLimitReached)
        append(FormatStringResource(nullptr, IDS_FILESENTRYLIMIT, snapshot.appliedLimits.maximumEntries));
    if (snapshot.depthLimitReached)
        append(FormatStringResource(nullptr, IDS_FILESDEPTHLIMIT, snapshot.appliedLimits.maximumDepth));
    setFooter(std::move(footer));
}
void Application::FolderCompleted()
{
    if (auto result = _folderFiles.TakeResult(); result && result->root == _folderRoot && result->generation == _folderFiles.Generation())
    {
        {
            // Borrow views only while both snapshots are alive. Retain user
            // choices for surviving directories, initialize new rows, and drop
            // disappeared keys so notifications cannot grow collapse state.
            std::set<std::wstring_view> previousDirectories, currentDirectories;
            if (_folderSnapshot)
                for (const auto& entry : _folderSnapshot->entries)
                    if (entry.directory)
                        previousDirectories.insert(entry.path.native());
            for (const auto& entry : result->entries)
                if (entry.directory)
                {
                    currentDirectories.insert(entry.path.native());
                    const auto relative = _session.path.lexically_relative(entry.path);
                    if (! relative.empty() && *relative.begin() != L"..")
                        _collapsedFolders.erase(entry.path);
                    else if (! previousDirectories.contains(entry.path.native()))
                        _collapsedFolders.insert(entry.path);
                }
            std::erase_if(_collapsedFolders, [&](const auto& path) { return ! currentDirectories.contains(path.native()); });
        }
        _folderSnapshot = std::move(result);
        RefreshFiles();
    }
}
void Application::EnsureFilesRoot()
{
    if (! _settings.filesRoot.empty() || _session.path.empty())
        return;
    std::error_code error;
    const auto absolute = std::filesystem::absolute(_session.path, error);
    if (error)
        return;
    _settings.filesRoot = absolute.parent_path().lexically_normal();
    SavePreferences();
}
void Application::SelectFilesRoot(const std::filesystem::path& path)
{
    if (! path.is_absolute() || path.native().find(L'\0') != std::wstring::npos)
        return;
    _settings.filesRoot          = path.lexically_normal();
    _settings.showFiles          = true;
    _workspaceSelectionPreferred = false;
    _filesFilter->SetText(L"");
    RefreshFiles(true);
    Layout();
    SavePreferences();
    _host.SetFocusControl(_filesTree, MayRequestNativeFocus());
}
void Application::OpenFolder()
{
    AbandonStartupRestore();
    AskWorkspaceThen([this]
    {
        const auto path = PickFile(false, false, true);
        if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached())
            return;
        ResetWorkspace();
        SetDocument({});
        SelectFilesRoot(path);
    });
}
bool Application::SwitchWorkspaceDocument(std::size_t index, bool savePreviousRecovery, bool refreshFiles)
{
    _pendingBitmapPaste.reset();
    if (! CanAcceptCommand())
        return false;
    EnsurePrimaryEditorAuthoritative();
    if (index >= _inactiveDocuments.size() || _files.Busy() || HasBlockDraft() || HasActiveModalPrompt())
    {
        Status(IDS_BUSY);
        return false;
    }
    auto* const primary        = _editor;
    auto* const owner          = _root;
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto ownerLifetime   = GetControlLifetimeToken(*owner);
    const auto current         = [&]
    { return ! IsClosing() && ! primaryLifetime.expired() && ! ownerLifetime.expired() && _editor == primary && _root == owner && _host.GetRoot() == owner; };
    const bool previousDirty = Dirty();
    if (savePreviousRecovery)
    {
        CheckpointRecovery(true);
        if (! current())
            return false;
    }
    auto& document                     = _inactiveDocuments[index];
    const bool previousImageTransition = _imageBindingTransition;
    _imageBindingTransition            = true;
    const auto restoreImageTransition  = wil::scope_exit([&] { _imageBindingTransition = previousImageTransition; });
    if (! AcceptSourceResult(primary->TrySetDocumentFilePath(document.metadata.path.native())) || ! current())
        return false;
    {
        _switchingDocument = true;
        const auto restore = wil::scope_exit([&] { _switchingDocument = false; });
        if (! primary->ExchangeDocumentState(document.state))
        {
            (void)AcceptSourceResult(primary->TrySetDocumentFilePath(_session.path.native()));
            Status(IDS_BUSY);
            return false;
        }
        ++_activity.workspaceSwitches;
        _activity.workspaceRetainedPeak = std::max(_activity.workspaceRetainedPeak, static_cast<std::uint64_t>(_inactiveDocuments.size() + 1));
        if (index < _inactiveDocuments.size())
        {
            std::swap(_session, _inactiveDocuments[index].metadata);
            _inactiveDocuments[index].dirty = previousDirty;
        }
        _printNetworkImages.clear();
        _derivedCaches.Reset();
        _outlinePreviewBaseline.reset();
        _find.InvalidateMatches();
        if (! primaryLifetime.expired())
            primary->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
        if (! current())
            return true; // The exchange committed before a callback retired its owner.
    }
    RememberActiveDocument();
    SetView(_settings.view);
    if (! current())
        return true; // The binding committed before a reentrant teardown.
    Changed();
    if (! current())
        return true;
    if (refreshFiles)
    {
        RefreshFiles();
        if (! current())
            return true;
    }
    _host.SetFocusControl(primary, MayRequestNativeFocus());
    return true;
}
bool Application::StartWorkspaceDocument()
{
    EnsurePrimaryEditorAuthoritative();
    if (_session.path.empty() && ! Dirty() && ! _editor->CanUndo() && ! _editor->CanRedo())
        return true;
    if (_inactiveDocuments.size() + 1 >= RecoveryService::MaximumDocuments)
    {
        Status(IDS_WORKSPACEFULL);
        return false;
    }
    unsigned key{};
    while (key == _session.key || std::ranges::any_of(_inactiveDocuments, [&](const auto& document) { return document.metadata.key == key; }))
        ++key;
    WorkspaceDocument document;
    document.metadata.key = key;
    _inactiveDocuments.push_back(std::move(document));
    if (SwitchWorkspaceDocument(_inactiveDocuments.size() - 1))
        return true;
    _inactiveDocuments.pop_back();
    return false;
}
void Application::ResetWorkspace()
{
    EnsurePrimaryEditorAuthoritative();
    for (const auto& document : _inactiveDocuments)
        if (FAILED(_recovery.SubmitDocument(document.metadata.key, std::nullopt, true)))
            Status(IDS_RECOVERYFAILED);
    if (FAILED(_recovery.SubmitDocument(_session.key, std::nullopt, true)))
        Status(IDS_RECOVERYFAILED);
    _inactiveDocuments.clear();
    _workspaceCollapsed          = false;
    _workspaceSelectionPreferred = false;
    _session.key                 = 0;
    _workspaceApprovals.clear();
    RefreshLeftRailDots();
}
void Application::CheckpointWorkspaceRecovery()
{
    CheckpointRecovery(true);
    for (const auto& document : _inactiveDocuments)
    {
        std::optional<RecoverySnapshot> snapshot;
        if (document.dirty)
            snapshot =
                RecoverySnapshot{document.state.Text(), document.metadata.path, document.metadata.bom, document.state.GetSelection(), ++_recoveryGeneration};
        if (FAILED(_recovery.SubmitDocument(document.metadata.key, std::move(snapshot), true)))
            Status(IDS_RECOVERYFAILED);
    }
}
void Application::OpenPath(const std::filesystem::path& path)
{
    if (! IsMarkdownFile(path))
        return;
    if (_folderSnapshot)
    {
        const auto& entries = _folderSnapshot->entries;
        if (const auto item = std::find_if(entries.begin(), entries.end(), [&](const FilesEntry& entry) { return entry.path == path; });
            item != entries.end() && item->directory)
            return;
    }
    if (path == _session.path || (_folderSnapshot && MatchesFilesPath(_folderSnapshot.value(), path, _session.path)))
    {
        if (_workspaceSelectionPreferred)
        {
            _workspaceSelectionPreferred = false;
            RefreshFiles();
        }
        return;
    }
    AbandonStartupRestore();
    if (_files.Busy() || HasBlockDraft() || HasActiveModalPrompt() || ! CanAcceptCommand())
    {
        Status(IDS_BUSY);
        if (IsUiAttached())
            RefreshFiles();
        return;
    }
    for (std::size_t index = 0; index < _inactiveDocuments.size(); ++index)
        if (_inactiveDocuments[index].metadata.path == path ||
            (_folderSnapshot && MatchesFilesPath(_folderSnapshot.value(), path, _inactiveDocuments[index].metadata.path)))
        {
            const bool previous          = _workspaceSelectionPreferred;
            _workspaceSelectionPreferred = false;
            if (! SwitchWorkspaceDocument(index))
            {
                _workspaceSelectionPreferred = previous;
                if (IsUiAttached())
                    RefreshFiles();
            }
            return;
        }
    auto entrySource     = ActiveEditor()->Snapshot();
    const HRESULT result = _files.Submit(false, path);
    if (SUCCEEDED(result))
    {
        _activeFileTx = FilesPaneOpenTx{std::move(entrySource), _workspaceSelectionPreferred};
        Status(IDS_LOADING);
    }
    else
    {
        _activeFileTx = std::monostate{};
        if (IsUiAttached())
            RefreshFiles();
        Status(IDS_BUSY);
    }
}
void Application::CloseActiveDocument()
{
    AskThen([this]
    {
        if (! CanAcceptCommand() || ! IsUiAttached())
            return;
        EnsurePrimaryEditorAuthoritative();
        if (_inactiveDocuments.empty())
        {
            ResetWorkspace();
            SetDocument({});
            if (IsUiAttached())
                _host.SetFocusControl(_editor, MayRequestNativeFocus());
            return;
        }
        // Switch to the adjacent remaining document in workspace key order before retiring the closed slot.
        std::optional<std::size_t> nextHigher, prevLower;
        for (std::size_t index = 0; index < _inactiveDocuments.size(); ++index)
        {
            const auto candidateKey = _inactiveDocuments[index].metadata.key;
            if (candidateKey > _session.key && (! nextHigher || candidateKey < _inactiveDocuments[nextHigher.value()].metadata.key))
                nextHigher = index;
            else if (candidateKey < _session.key && (! prevLower || candidateKey > _inactiveDocuments[prevLower.value()].metadata.key))
                prevLower = index;
        }
        const std::size_t targetIndex = nextHigher ? nextHigher.value() : prevLower.value_or(0u);
        if (! SwitchWorkspaceDocument(targetIndex, false, false) || ! IsUiAttached())
            return;
        if (targetIndex < _inactiveDocuments.size())
        {
            const auto closedKey = _inactiveDocuments[targetIndex].metadata.key;
            _inactiveDocuments.erase(_inactiveDocuments.begin() + static_cast<std::ptrdiff_t>(targetIndex));
            std::erase_if(_workspaceApprovals, [&](const WorkspaceApproval& approval) { return approval.key == closedKey; });
            if (FAILED(_recovery.SubmitDocument(closedKey, std::nullopt, true)))
                Status(IDS_RECOVERYFAILED);
        }
        if (_inactiveDocuments.empty())
            _workspaceSelectionPreferred = false;
        RefreshFiles();
        if (IsUiAttached())
            _host.SetFocusControl(_editor, MayRequestNativeFocus());
    });
}
bool Application::OpenDroppedFile(const std::filesystem::path& path)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt() || _printPreview)
        return false;
    if (HasBlockDraft())
    {
        Status(IDS_BLOCKPENDING);
        return false;
    }
    if (_files.Busy())
    {
        Status(IDS_BUSY);
        return false;
    }
    if (path.empty() || ! path.is_absolute() || path.native().find(L'\0') != std::wstring::npos)
    {
        Status(IDS_DROPUNSUPPORTED);
        return false;
    }
    const auto extension        = path.extension().native();
    const auto matchesExtension = [&](std::wstring_view expected) noexcept
    {
        return extension.size() == expected.size() &&
               CompareStringOrdinal(extension.data(), static_cast<int>(extension.size()), expected.data(), static_cast<int>(expected.size()), TRUE) ==
                   CSTR_EQUAL;
    };
    if (! matchesExtension(L".md") && ! matchesExtension(L".markdown") && ! matchesExtension(L".txt"))
    {
        Status(IDS_DROPUNSUPPORTED);
        return false;
    }
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        Status(IDS_DROPUNSUPPORTED);
        return false;
    }
    AbandonStartupRestore();
    const auto normalized = path.lexically_normal();
    AskWorkspaceThen([this, normalized]
    {
        if (! CanAcceptCommand() || _files.Busy())
        {
            Status(IDS_BUSY);
            return;
        }
        auto entrySource = ActiveEditor()->Snapshot();
        const HRESULT hr = _files.Submit(false, normalized);
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
    return true;
}
void Application::RefreshFind()
{
    if (_find.updating || ! _findText)
        return;
    auto* const activeEditor = ActiveEditor();
    auto* const mirrorEditor = MirrorEditor();
    if (_activeOverlay != OverlayKind::FindBar)
    {
        if (! _find.IsPresentationActive())
            return;

        _find.projections.sectionCounts.clear();
        _find.projections.sectionCountsValid = false;
        _find.projections.highlightedCurrent.reset();
        _find.projections.displayedCurrent.reset();
        _find.projections.displayedTotal.reset();
        _find.projections.displayedQuery.clear();
        _find.projections.highlightsNeedRefresh = true;
        _find.projections.badgesNeedRefresh     = true;

        if (_find.projections.hasHighlights)
        {
            if (activeEditor)
                (void)AcceptSourceResult(activeEditor->TrySetSourceHighlights({}));
            if (mirrorEditor && mirrorEditor != activeEditor)
                mirrorEditor->ClearSourceHighlights();
            ++_activity.findHighlightUpdates;
            _find.projections.hasHighlights = false;
        }
        bool badgesChanged{};
        for (auto& row : _outlineModel.rows)
            if (! row.badgeText.empty())
            {
                row.badgeText.clear();
                badgesChanged = true;
            }
        if (badgesChanged)
            _outlineTree->NotifyDataChanged();
        _find.projections.badgeRowsRevision   = _derivedCaches.outlineRowsRevision;
        _find.projections.presentationCleared = true;
        RefreshRightRailDots();
        return;
    }

    _find.projections.presentationCleared = false;
    _findBar->SetAccessibleName(Resource(activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source ? IDS_FIND_IN_SOURCE : IDS_FIND_IN_COMPOSE));
    const auto& needle  = _findText->GetText();
    const auto snapshot = activeEditor->Snapshot();
    if (! _find.table.source || ! _find.table.source->IsSameRevision(snapshot) || _find.table.query != needle || _find.table.matchCase != _find.matchCase)
    {
        _find.table.query     = needle;
        _find.table.source    = snapshot;
        _find.table.matchCase = _find.matchCase;
        CollectSourceFindMatches(CurrentText(), needle, _find.matchCase, _find.table.matches);
        ++_activity.findMatchRebuilds;
        _find.projections.sectionCountsValid    = false;
        _find.projections.highlightsNeedRefresh = true;
        _find.projections.badgesNeedRefresh     = true;
        _find.projections.highlightedCurrent.reset();
        _find.projections.displayedCurrent.reset();
        _find.projections.displayedTotal.reset();
        _find.projections.displayedQuery.clear();
    }

    bool railCountsChanged{};
    if (! _find.projections.sectionCountsValid || _find.projections.sectionOutlineRevision != _derivedCaches.outlineRowsRevision ||
        _find.projections.sectionAnalysis != _activity.changes)
    {
        const auto& headings = _documentInfo.headings;
        const auto& text     = CurrentText();
        _find.projections.sectionCounts.assign(headings.size(), 0);
        std::vector<std::size_t> sectionEnds(headings.size(), text.size());
        std::vector<std::size_t> openSections;
        openSections.reserve(headings.size());
        for (std::size_t i = 0; i < headings.size(); ++i)
        {
            while (! openSections.empty() && headings[openSections.back()].level >= headings[i].level)
            {
                sectionEnds[openSections.back()] = headings[i].offset;
                openSections.pop_back();
            }
            openSections.push_back(i);
        }
        for (std::size_t i = 0; i < headings.size(); ++i)
        {
            const auto lower =
                std::lower_bound(_find.table.matches.begin(), _find.table.matches.end(), headings[i].offset, [](const auto& range, std::size_t pos) {
                return range.first < pos;
            });
            const auto upper =
                std::lower_bound(lower, _find.table.matches.end(), sectionEnds[i], [](const auto& range, std::size_t pos) { return range.first < pos; });
            _find.projections.sectionCounts[i] = static_cast<std::size_t>(upper - lower);
        }
        _find.projections.sectionOutlineRevision = _derivedCaches.outlineRowsRevision;
        _find.projections.sectionAnalysis        = _activity.changes;
        _find.projections.sectionCountsValid     = true;
        _find.projections.badgesNeedRefresh      = true;
        railCountsChanged                        = true;
        ++_activity.findSectionCountRebuilds;
    }

    const auto selected = activeEditor->GetSelection();
    const auto active   = std::lower_bound(
        _find.table.matches.begin(), _find.table.matches.end(), selected.Start(), [](const auto& range, std::size_t pos) { return range.first < pos; });
    const bool hasActive      = active != _find.table.matches.end() && active->first == selected.Start() && active->last == selected.End();
    const std::size_t current = hasActive ? static_cast<std::size_t>(active - _find.table.matches.begin() + 1) : 0;
    if (_find.projections.highlightsNeedRefresh || ! _find.projections.highlightedCurrent || _find.projections.highlightedCurrent.value() != current)
    {
        std::vector<DxUi::RichTextSourceHighlight> highlights;
        const auto count = std::min(_find.table.matches.size(), std::size_t{4096});
        highlights.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto range = _find.table.matches[i];
            highlights.push_back({range.first, range.last - range.first, current == i + 1});
        }
        if (current > count)
            highlights.back() = {active->first, active->last - active->first, true};
        if (mirrorEditor && mirrorEditor != activeEditor)
            mirrorEditor->ClearSourceHighlights();
        if (! AcceptSourceResult(activeEditor->TrySetSourceHighlights(highlights)))
            return;
        ++_activity.findHighlightUpdates;
        _find.projections.hasHighlights         = ! highlights.empty();
        _find.projections.highlightedCurrent    = current;
        _find.projections.highlightsNeedRefresh = false;
    }

    if (! _find.projections.displayedCurrent || _find.projections.displayedCurrent.value() != current || ! _find.projections.displayedTotal ||
        _find.projections.displayedTotal.value() != _find.table.matches.size() || _find.projections.displayedQuery != needle)
    {
        const auto countText = _find.table.matches.empty() ? Resource(needle.empty() ? IDS_FINDQUERY : IDS_NOMATCH)
                                                           : FormatStringResource(nullptr, IDS_FINDCOUNT, current, _find.table.matches.size());
        if (_findCount->GetText() != countText)
            _findCount->SetText(countText);
        _find.projections.displayedCurrent = current;
        _find.projections.displayedTotal   = _find.table.matches.size();
        _find.projections.displayedQuery   = needle;
    }

    if (_find.projections.badgesNeedRefresh || _find.projections.badgeRowsRevision != _derivedCaches.outlineRowsRevision)
    {
        bool changed{};
        for (auto& row : _outlineModel.rows)
        {
            const auto heading = static_cast<std::size_t>(row.id - 1);
            const auto count   = heading < _find.projections.sectionCounts.size() ? _find.projections.sectionCounts[heading] : 0;
            const auto badge   = count ? std::to_wstring(count) : std::wstring{};
            if (row.badgeText != badge)
            {
                row.badgeText = badge;
                changed       = true;
            }
        }
        if (changed)
            _outlineTree->NotifyDataChanged();
        _find.projections.badgeRowsRevision = _derivedCaches.outlineRowsRevision;
        _find.projections.badgesNeedRefresh = false;
        railCountsChanged                   = true;
    }
    if (railCountsChanged)
        RefreshRightRailDots();
}
void Application::FindNext(bool previous)
{
    auto* const activeEditor = ActiveEditor();
    if (_activeOverlay == OverlayKind::FindBar)
        RefreshFind();
    else
    {
        const auto snapshot = activeEditor->Snapshot();
        const auto& query   = _findText->GetText();
        if (! _find.table.source || ! _find.table.source->IsSameRevision(snapshot) || _find.table.query != query || _find.table.matchCase != _find.matchCase)
        {
            _find.table.query     = query;
            _find.table.source    = snapshot;
            _find.table.matchCase = _find.matchCase;
            CollectSourceFindMatches(CurrentText(), query, _find.matchCase, _find.table.matches);
            ++_activity.findMatchRebuilds;
        }
    }
    const auto& matches = _find.table.matches;
    if (matches.empty())
        return;
    const auto selection = activeEditor->GetSelection();
    std::size_t index{};
    if (previous)
    {
        index = matches.size() - 1;
        for (std::size_t i = matches.size(); i > 0; --i)
            if (matches[i - 1].first < selection.Start())
            {
                index = i - 1;
                break;
            }
    }
    else
    {
        const auto next = std::find_if(matches.begin(), matches.end(), [&](const auto& r) { return r.first >= selection.End(); });
        if (next != matches.end())
            index = static_cast<std::size_t>(next - matches.begin());
    }
    const auto range = matches[index];
    if (! AcceptSourceResult(activeEditor->TrySetSelection({range.first, range.last})))
        return;
    (void)activeEditor->ScrollSourceRangeIntoView(_host, range.first, range.last, false);
    SelectionChanged();
}
void Application::ReplaceFind(bool all)
{
    auto* const activeEditor = ActiveEditor();
    if (HasBlockDraft() || activeEditor->IsReadOnly())
        return;
    RefreshFind();
    if (_find.table.matches.empty())
        return;
    const std::wstring replacement(_replaceText->GetText());
    const auto source   = activeEditor->Snapshot();
    const auto revision = activeEditor->Revision();
    std::vector<DxUi::RichText::SourceEdit> edits;
    std::size_t size     = source.Length();
    const auto selection = activeEditor->GetSelection();
    for (const auto range : _find.table.matches)
        if (all || (selection.Start() == range.first && selection.End() == range.last))
        {
            size -= range.last - range.first;
            if (replacement.size() > DxUi::RichText::Document::MaximumUnits - size)
            {
                Status(IDS_REPLACEFAILED);
                return;
            }
            size += replacement.size();
            edits.push_back({range, replacement});
        }
    if (edits.empty())
    {
        FindNext();
        return;
    }
    const auto after = edits.front().source.first + replacement.size();
    bool applied{};
    {
        _find.updating               = true;
        const auto resetUpdatingFind = wil::scope_exit([&] { _find.updating = false; });
        applied                      = activeEditor->ApplySourceEdits(source, revision, edits, {after, after});
    }
    if (! applied)
        Status(IDS_NOTAVAILABLE);
    RefreshFind();
    if (applied && ! all)
        FindNext();
}
void Application::FileMenu(bool overflow)
{
    if (_options.headless)
        return;
    std::vector<DxUi::MenuFlyoutItem> items;
    auto add = [&](unsigned label, Command command, unsigned shortcut = 0u)
    {
        DxUi::MenuFlyoutItem item;
        item.text            = Resource(label);
        item.commandId       = static_cast<int>(command) + 1;
        item.acceleratorText = shortcut ? Resource(shortcut) : std::wstring{};
        items.push_back(std::move(item));
    };
    if (! overflow)
    {
        add(IDS_NEW, Command::New, IDS_SHORTCUT_NEW);
        add(IDS_OPEN, Command::Open, IDS_SHORTCUT_OPEN);
        add(IDS_OPENFOLDER, Command::OpenFolder, IDS_SHORTCUT_OPENFOLDER);
        add(IDS_SAVEAS, Command::SaveAs, IDS_SHORTCUT_SAVEAS);
        add(IDS_CLOSEDOCUMENT, Command::CloseDocument, IDS_SHORTCUT_CLOSEDOCUMENT);
        add(IDS_PRINT, Command::Print, IDS_SHORTCUT_PRINT);
        add(IDS_IMAGERELOAD, Command::ReloadImages);
    }
    else
        for (const auto& [label, command] : std::array<std::pair<unsigned, Command>, 11>{{{IDS_HEADING, Command::Heading},
                                                                                          {IDS_BOLD, Command::Bold},
                                                                                          {IDS_ITALIC, Command::Italic},
                                                                                          {IDS_CODE, Command::Code},
                                                                                          {IDS_LINK, Command::Link},
                                                                                          {IDS_BULLETS, Command::Bullets},
                                                                                          {IDS_NUMBERED, Command::Numbered},
                                                                                          {IDS_TASK, Command::Task},
                                                                                          {IDS_INSERT, Command::Insert},
                                                                                          {IDS_LOCAL, Command::LocalSource},
                                                                                          {IDS_FOCUS, Command::Focus}}})
            add(label, command);
    const auto bounds = (overflow ? _overflowButton : _fileMenu)->GetBounds();
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(
        _window.get(), _host.DipPointToScreenPoint({bounds.left, bounds.bottom}), items, _host.GetTheme(), [this](std::optional<int> selected) noexcept {
        OnMenuClosed(selected);
    }, callbacks);
}
bool Application::DismissSurface()
{
    if (_pendingSelectionTools || _selectionToolsFocus || _activeOverlay != OverlayKind::None)
    {
        CloseDrawers();
        return true;
    }
    if (IsPaneOverlayActive(PaneSide::Left) || IsPaneOverlayActive(PaneSide::Right))
    {
        DismissHoverOverlays(false);
        return true;
    }
    auto* const activeEditor = ActiveEditor();
    if (activeEditor->GetSemanticSourceRegion())
    {
        if (! activeEditor->EndSemanticSourceEdit(_host))
            return false;
        SelectionChanged();
        return true;
    }
    if (_settings.focusMode)
    {
        Execute(Command::Focus);
        return true;
    }
    if (_settings.showFiles)
    {
        if (! _filesFilter->GetText().empty())
        {
            _filesFilter->SetText(L"");
            RefreshFiles();
        }
        else
        {
            _settings.showFiles = false;
            RefreshFiles(true);
            Layout();
            SavePreferences();
            _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
        }
        return true;
    }
    return false;
}
LRESULT Application::FrameHitTest(HWND hwnd, LPARAM point) const
{
    POINT screen{GET_X_LPARAM(point), GET_Y_LPARAM(point)};
    RECT rect{};
    GetWindowRect(hwnd, &rect);
    const int border = GetSystemMetricsForDpi(SM_CXSIZEFRAME, GetDpiForWindow(hwnd)) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, GetDpiForWindow(hwnd));
    if (! IsZoomed(hwnd))
    {
        const bool left = screen.x < rect.left + border, right = screen.x >= rect.right - border, top = screen.y < rect.top + border,
                   bottom = screen.y >= rect.bottom - border;
        if (top)
            return left ? HTTOPLEFT : right ? HTTOPRIGHT : HTTOP;
        if (bottom)
            return left ? HTBOTTOMLEFT : right ? HTBOTTOMRIGHT : HTBOTTOM;
        if (left)
            return HTLEFT;
        if (right)
            return HTRIGHT;
    }
    ScreenToClient(hwnd, &screen);
    // Hit the same client-space geometry that the host lays out and paints,
    // including a harness DPI transition before the next native frame.
    const float scale = 96.f / _host.GetDpi();
    const D2D1_POINT_2F dip{static_cast<float>(screen.x) * scale, static_cast<float>(screen.y) * scale};
    const auto contains = [&](const DxUi::Control* c)
    {
        const auto r = c->GetBounds();
        return c->IsVisible() && dip.x >= r.left && dip.x < r.right && dip.y >= r.top && dip.y < r.bottom;
    };
    if (contains(_closeButton))
        return HTCLOSE;
    if (contains(_maximizeButton))
        return HTMAXBUTTON;
    if (contains(_minimizeButton))
        return HTMINBUTTON;
    if (dip.y < (_settings.focusMode ? 40.f : 56.f))
    {
        if (contains(_fileMenu) || contains(_saveButton) || contains(_exitFocus))
            return HTCLIENT;
        for (const auto* mode : _modeButtons)
            if (contains(mode))
                return HTCLIENT;
        return HTCAPTION;
    }
    return HTCLIENT;
}
} // namespace RedTilde
