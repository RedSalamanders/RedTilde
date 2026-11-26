#include "Application.h"
#include "Common/PathSecurity.h"
#include "Common/TextUtilities.h"
#include "res/Resource.h"
#include <algorithm>
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
            position = FindOrdinalIgnoreCase(text, query, from);
        if (position == std::wstring_view::npos)
            break;
        if (DxUi::RichText::IsEditBoundary(text, position) && DxUi::RichText::IsEditBoundary(text, position + query.size()))
            matches.push_back({position, position + query.size()});
        from = position + query.size();
    }
}

[[nodiscard]] bool IsPathInsideOrEqualDirectory(const std::filesystem::path& candidate, const std::filesystem::path& directory)
{
    if (candidate.empty() || directory.empty())
        return false;
    const auto normCand = NormalizeRootPath(candidate);
    const auto normDir  = NormalizeRootPath(directory);
    const auto& candStr = normCand.native();
    const auto& dirStr  = normDir.native();
    if (EqualOrdinalIgnoreCase(candStr, dirStr))
        return true;
    if (dirStr.ends_with(L'\\'))
        return StartsWithOrdinalIgnoreCase(candStr, dirStr);
    return candStr.size() > dirStr.size() && StartsWithOrdinalIgnoreCase(candStr, dirStr) && candStr[dirStr.size()] == L'\\';
}

[[nodiscard]] std::wstring OrdinalRelativeOrFullPath(const std::filesystem::path& path, const std::filesystem::path& root)
{
    const auto normPath = NormalizeRootPath(path);
    if (root.empty())
        return normPath.native();
    const auto normRoot = NormalizeRootPath(root);
    if (! IsPathInsideOrEqualDirectory(normPath, normRoot) || EqualOrdinalIgnoreCase(normPath.native(), normRoot.native()))
        return normPath.native();
    const auto& pathStr = normPath.native();
    const auto& rootStr = normRoot.native();
    std::size_t start   = rootStr.size();
    if (! rootStr.ends_with(L'\\') && start < pathStr.size() && pathStr[start] == L'\\')
        ++start;
    std::wstring rel(pathStr.substr(start));
    std::ranges::replace(rel, L'\\', L'/');
    return rel;
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
void Application::NormalizeFilesRootsSettings()
{
    _settings.filesRoot = NormalizeRootPath(_settings.filesRoot);
    if (_settings.filesRoots.size() <= 1u)
    {
        if (_settings.filesRoot.empty() || ! _settings.filesRoot.is_absolute())
        {
            _settings.filesRoot.clear();
            _settings.filesRoots.clear();
        }
        else
        {
            _settings.filesRoots = {_settings.filesRoot};
        }
        return;
    }
    std::vector<std::filesystem::path> normalized;
    normalized.reserve(_settings.filesRoots.size());
    for (const auto& r : _settings.filesRoots)
    {
        if (r.empty() || ! r.is_absolute())
            continue;
        auto norm = NormalizeRootPath(r);
        if (std::ranges::none_of(normalized, [&](const std::filesystem::path& existing) { return EqualNormalizedRootPath(existing, norm); }))
            normalized.push_back(std::move(norm));
    }
    _settings.filesRoots = std::move(normalized);
    _settings.filesRoot  = _settings.filesRoots.empty() ? std::filesystem::path{} : _settings.filesRoots.front();
}

void Application::UpdateCurrentFileDirtyBadge(bool dirty)
{
    _lastFilesDirty = dirty;
    RefreshLeftRailDots();
    auto* const tree              = _filesTree;
    const auto lifetime           = GetControlLifetimeToken(*tree);
    const auto key                = _session.key;
    const auto id                 = WorkspaceGroupId + _session.key + 1;
    const bool anyDirty           = dirty || std::ranges::any_of(_inactiveDocuments, &WorkspaceDocument::dirty);
    const bool hasWorkspaceHeader = std::ranges::any_of(_filesModel.rows, [](const auto& row) { return row.id == WorkspaceGroupId; });
    const auto item               = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [&](const auto& row) { return row.id == id; });
    if (hasWorkspaceHeader != anyDirty || (! _workspaceCollapsed && ((item != _filesModel.rows.end()) != dirty)))
    {
        RefreshFiles();
        return;
    }
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
    NormalizeFilesRootsSettings();
    const bool filesActive = _settings.showFiles || IsPaneOverlayActive(PaneSide::Left);
    if (! filesActive || _settings.filesRoots.empty())
        return;

    const auto* const model = _filesTree->GetModel();
    if (model != &_filesModel)
    {
        RefreshFiles();
        return;
    }

    const bool simpleSingleLocal = _settings.filesRoots.size() == 1u && _connectedGitHubRepos.empty();
    const FilesSnapshot* matchedSnapshot{};
    std::uint64_t itemId{};
    std::filesystem::path matchedRoot;

    if (simpleSingleLocal)
    {
        const auto requested = _settings.filesRoots.front();
        if (! SameRootPath(requested, _folderRoot))
        {
            RefreshFiles();
            return;
        }
        if (! _folderSnapshot || ! SameRootPath(_folderSnapshot->root, requested))
            return;
        matchedSnapshot  = &_folderSnapshot.value();
        matchedRoot      = requested;
        const auto entry = std::find_if(matchedSnapshot->entries.begin(), matchedSnapshot->entries.end(), [&](const auto& candidate) {
            return ! candidate.directory && MatchesFilesPath(*matchedSnapshot, candidate.path, _session.path);
        });
        if (entry == matchedSnapshot->entries.end())
            return;
        itemId = static_cast<std::uint64_t>(std::distance(matchedSnapshot->entries.begin(), entry) + 1);
    }
    else
    {
        for (std::size_t rootIndex = 0u; rootIndex < _settings.filesRoots.size(); ++rootIndex)
        {
            const auto& rootPath           = _settings.filesRoots[rootIndex];
            const std::uint64_t rootBaseId = LocalRootGroupBaseId + static_cast<std::uint64_t>(rootIndex) * 0x100000ull;
            const FilesSnapshot* snapPtr{};
            if (_settings.filesRoots.size() == 1u && _folderSnapshot && SameRootPath(_folderSnapshot->root, rootPath))
                snapPtr = &_folderSnapshot.value();
            else
            {
                const auto it = std::ranges::find_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, rootPath); });
                if (it != _folderSnapshots.end())
                    snapPtr = &(*it);
            }
            if (! snapPtr || _collapsedLocalRoots.contains(rootPath))
                continue;
            const auto entry = std::find_if(snapPtr->entries.begin(), snapPtr->entries.end(), [&](const auto& candidate) {
                return ! candidate.directory && MatchesFilesPath(*snapPtr, candidate.path, _session.path);
            });
            if (entry != snapPtr->entries.end())
            {
                matchedSnapshot = snapPtr;
                matchedRoot     = rootPath;
                itemId          = rootBaseId + static_cast<std::uint64_t>(std::distance(snapPtr->entries.begin(), entry)) + 1u;
                break;
            }
        }
        if (! matchedSnapshot)
            return;
    }

    auto row = std::find_if(_filesModel.rows.begin(), _filesModel.rows.end(), [&](const auto& candidate) { return candidate.id == itemId; });
    if (row == _filesModel.rows.end())
        return; // Filtering or a collapsed ancestor hides this file; a later structural refresh reads Dirty().

    const bool edited    = dirty || std::ranges::any_of(_inactiveDocuments, [&](const auto& document) {
        return document.dirty && MatchesFilesPath(*matchedSnapshot, document.metadata.path, _session.path);
    });
    const auto badgeText = edited ? Resource(IDS_FILESEDITED) : std::wstring{};
    if (row->badgeText == badgeText && row->badgeIsDot == edited)
        return;
    row->badgeText  = badgeText;
    row->badgeIsDot = edited;

    const auto snapshotGeneration = matchedSnapshot->generation;
    const auto snapshotRevision   = matchedSnapshot->revision;
    const auto modelRebuilds      = _activity.filesModelRebuilds;
    if (_filesTree->NotifyItemPresentationChanged(itemId))
        return;

    // UI Automation callbacks can re-enter the app. Fall back to a full refresh
    // only while the exact snapshot/model publication we just changed is still current.
    if (model != &_filesModel || _filesTree->GetModel() != model || Dirty() != dirty || _lastFilesDirty != dirty ||
        _activity.filesModelRebuilds != modelRebuilds)
        return;
    const FilesSnapshot* currentSnap{};
    if (simpleSingleLocal)
        currentSnap = (_folderSnapshot && SameRootPath(_folderSnapshot->root, matchedRoot)) ? &*_folderSnapshot : nullptr;
    else
    {
        const auto it = std::ranges::find_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, matchedRoot); });
        if (it != _folderSnapshots.end())
            currentSnap = &(*it);
    }
    if (! currentSnap || currentSnap->generation != snapshotGeneration || currentSnap->revision != snapshotRevision)
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
    NormalizeFilesRootsSettings();
    const bool filesActive = _settings.showFiles || IsPaneOverlayActive(PaneSide::Left);
    std::vector<std::filesystem::path> activeRoots;
    if (filesActive)
        activeRoots = _settings.filesRoots;
    const auto requested = activeRoots.empty() ? std::filesystem::path{} : activeRoots.front();
    const bool useMulti  = activeRoots.size() > 1u;

    if (useMulti)
    {
        _folderRoot.clear();
        _folderSnapshot.reset();
        _folderFiles.Close();
        bool rootsDiffer = activeRoots.size() != _folderSnapshots.size();
        if (! rootsDiffer)
            for (std::size_t i = 0; i < activeRoots.size(); ++i)
                if (! SameRootPath(_folderSnapshots[i].root, activeRoots[i]))
                {
                    rootsDiffer = true;
                    break;
                }
        if (rootsDiffer || rootChanged || _multiFolderFiles.RootCount() != activeRoots.size())
        {
            if (rootsDiffer)
            {
                std::vector<FilesSnapshot> kept;
                for (const auto& r : activeRoots)
                {
                    auto prevIt = std::ranges::find_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, r); });
                    if (prevIt != _folderSnapshots.end())
                        kept.push_back(*prevIt);
                }
                _folderSnapshots = std::move(kept);
            }
            if (_multiFolderFiles.RootCount() == 0u && ! activeRoots.empty())
                _folderAdmissionStatus = _multiFolderFiles.Initialize(_window.get(), FolderReady);
            if (_multiFolderFiles.RootCount() != activeRoots.size() || rootsDiffer || rootChanged)
            {
                const HRESULT hr = _multiFolderFiles.SetRoots(activeRoots);
                if (FAILED(hr))
                    _folderAdmissionStatus = hr;
                else
                    _folderAdmissionStatus = S_OK;
            }
        }
    }
    else
    {
        _multiFolderFiles.Close();
        if (! SameRootPath(requested, _folderRoot) || (rootChanged && ! requested.empty() && ! _folderFiles.IsActive()))
        {
            _folderRoot = requested;
            _folderSnapshot.reset();
            _folderSnapshots.clear();
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
    }
    if (rootChanged && _folderSnapshot)
        for (const auto& entry : _folderSnapshot->entries)
            if (entry.directory && IsPathInsideOrEqualDirectory(_session.path, entry.path))
                _collapsedFolders.erase(entry.path);
    if (activeRoots.empty())
    {
        _filesTitle->SetText(Resource(IDS_FILES));
        _filesTitle->SetTooltipText({});
        _filesTitle->SetAccessibleHelpText({});
    }
    else if (activeRoots.size() == 1u)
    {
        const auto& single  = activeRoots.front();
        const auto leafText = SanitizeUiText(single.filename().empty() ? single.native() : single.filename().native());
        const auto fullText = SanitizeUiText(single.native(), 512u);
        _filesTitle->SetText(leafText);
        _filesTitle->SetTooltipText(fullText);
        _filesTitle->SetAccessibleHelpText(fullText);
    }
    else
    {
        const auto& first    = activeRoots.front();
        const auto firstName = SanitizeUiText(first.filename().empty() ? first.native() : first.filename().native());
        _filesTitle->SetText(FormatStringResource(nullptr, IDS_FILES_MULTI_ROOT_TITLE, firstName, activeRoots.size() - 1u));
        std::wstring joined;
        for (std::size_t i = 0; i < activeRoots.size(); ++i)
        {
            if (i > 0u)
                joined.push_back(L'\n');
            joined += SanitizeUiText(activeRoots[i].native(), 512u);
        }
        _filesTitle->SetTooltipText(joined);
        _filesTitle->SetAccessibleHelpText(joined);
    }
    _lastFilesDirty = Dirty();
    RefreshLeftRailDots();
    ++_activity.filesModelRebuilds;
    _filesModel.rows.clear();
    _filesRowBindings.clear();
    const std::wstring filter(_filesFilter->GetText());
    auto containsFilterInsensitive = [&](std::wstring_view haystack) noexcept { return filter.empty() || ContainsOrdinalIgnoreCase(haystack, filter); };
    {
        std::vector<DxUi::TreeItemData> documents;
        documents.reserve(_inactiveDocuments.size() + 1);
        const auto append = [&](unsigned key, const std::filesystem::path& path, const DocumentOriginInfo& origin, bool dirty)
        {
            if (! dirty)
                return;
            const auto name = origin.kind == DocumentOriginInfo::Kind::GitHub && ! origin.relativePath.empty()
                                  ? SanitizeUiText(std::filesystem::path(origin.relativePath).filename().native())
                                  : (path.empty() ? Resource(IDS_UNTITLED) : SanitizeUiText(path.filename().native()));
            if (! containsFilterInsensitive(name))
                return;
            DxUi::TreeItemData item;
            item.id       = WorkspaceGroupId + key + 1;
            item.parentId = WorkspaceGroupId;
            item.depth    = 1;
            item.text     = name;
            item.iconText = L"\xE8A5";
            if (origin.kind == DocumentOriginInfo::Kind::GitHub)
                item.tooltipText = SanitizeUiText(origin.owner + L"/" + origin.repo + L"/" + origin.relativePath);
            else
            {
                item.tooltipText = SanitizeUiText(path.native(), 512u);
                if (! _cutFilePaths.empty() && ! path.empty() && _cutFilePaths.contains(NormalizeRootPath(path).native()))
                    item.iconText = L"\xE8C6";
            }
            item.badgeIsDot = true;
            item.badgeText  = Resource(IDS_FILESEDITED);
            documents.push_back(std::move(item));
        };
        if (Dirty())
            append(_session.key, _session.path, _session.origin, true);
        for (const auto& document : _inactiveDocuments)
            if (document.dirty)
                append(document.metadata.key, document.metadata.path, document.metadata.origin, true);
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
                {
                    FilesRowContextBinding binding;
                    binding.kind         = FilesRowContextBinding::Kind::WorkspaceDoc;
                    binding.workspaceKey = static_cast<unsigned>(document.id - WorkspaceGroupId - 1);
                    std::filesystem::path docPath;
                    DocumentOriginInfo docOrigin;
                    if (binding.workspaceKey == _session.key)
                    {
                        docPath   = _session.path;
                        docOrigin = _session.origin;
                    }
                    else
                    {
                        for (const auto& inactive : _inactiveDocuments)
                        {
                            if (inactive.metadata.key == binding.workspaceKey)
                            {
                                docPath   = inactive.metadata.path;
                                docOrigin = inactive.metadata.origin;
                                break;
                            }
                        }
                    }
                    if (docOrigin.kind == DocumentOriginInfo::Kind::GitHub)
                    {
                        binding.githubOwner  = docOrigin.owner;
                        binding.githubRepo   = docOrigin.repo;
                        binding.githubBranch = docOrigin.branch;
                        binding.githubPath   = docOrigin.relativePath;
                    }
                    else if (! docPath.empty())
                    {
                        binding.localPath = docPath;
                        for (const auto& root : _settings.filesRoots)
                        {
                            if (IsPathInsideOrEqualDirectory(docPath, root))
                            {
                                binding.localRoot = root;
                                break;
                            }
                        }
                    }
                    _filesRowBindings.emplace_back(document.id, std::move(binding));
                    _filesModel.rows.push_back(std::move(document));
                }
        }
    }
    const std::uint64_t workspaceItemId = WorkspaceGroupId + _session.key + 1;
    const bool hasVisibleWorkspaceRow   = Dirty() && std::ranges::any_of(_filesModel.rows, [&](const auto& row) { return row.id == workspaceItemId; });
    if (activeRoots.empty() && _connectedGitHubRepos.empty())
    {
        setFooter(Resource(static_cast<unsigned>(IDS_FILESUNTITLED)));
        _filesTree->SetEmptyStateText(Resource(IDS_FILESUNTITLED));
        _filesTree->NotifyDataChanged();
        _filesTree->SetSelectedItemId(hasVisibleWorkspaceRow ? std::optional<std::uint64_t>(workspaceItemId) : std::nullopt);
        return;
    }

    auto appendLocalSnapshotRows = [&](const FilesSnapshot& snapshot,
                                       const std::filesystem::path& rootPath,
                                       std::uint64_t rootBaseId,
                                       unsigned baseDepth,
                                       std::optional<std::uint64_t>& outSelectedId)
    {
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
            item.id          = rootBaseId + index + 1u;
            item.text        = SanitizeUiText(entry.name);
            item.iconText    = entry.directory ? L"\xE8B7" : L"\xE8A5";
            item.tooltipText = SanitizeUiText(entry.path.native(), 512u);
            item.depth       = entry.depth + baseDepth;
            if (entry.parent != NoFilesParent)
                item.parentId = rootBaseId + entry.parent + 1u;
            else if (rootBaseId != 0u)
                item.parentId = rootBaseId;
            item.hasChildren        = entry.directory && ! entry.reparsePoint && entry.markdownCount > 0;
            item.expanded           = item.hasChildren && (! filter.empty() || ! _collapsedFolders.contains(entry.path));
            const bool isDirtyMatch = (MatchesFilesPath(snapshot, entry.path, _session.path) && Dirty()) || std::ranges::any_of(_inactiveDocuments,
                                                                                                                                [&](const auto& document)
            { return document.dirty && MatchesFilesPath(snapshot, entry.path, document.metadata.path); });
            if (entry.directory && ! item.expanded)
                item.badgeText = std::to_wstring(entry.markdownCount);
            else if (isDirtyMatch)
            {
                item.badgeText  = Resource(IDS_FILESEDITED);
                item.badgeIsDot = true;
            }
            if (! _cutFilePaths.empty() && _cutFilePaths.contains(NormalizeRootPath(entry.path).native()))
                item.iconText = L"\xE8C6";
            if (! outSelectedId && _session.origin.kind != DocumentOriginInfo::Kind::GitHub && MatchesFilesPath(snapshot, entry.path, _session.path))
                outSelectedId = item.id;
            FilesRowContextBinding binding;
            binding.kind        = FilesRowContextBinding::Kind::LocalEntry;
            binding.localRoot   = rootPath;
            binding.localPath   = entry.path;
            binding.isDirectory = entry.directory;
            _filesRowBindings.emplace_back(item.id, std::move(binding));
            _filesModel.rows.push_back(std::move(item));
        }
    };

    const bool simpleSingleLocal = activeRoots.size() == 1u && _connectedGitHubRepos.empty();
    if (simpleSingleLocal)
    {
        if (! _folderSnapshot)
        {
            setFooter(Resource(static_cast<unsigned>(FAILED(_folderAdmissionStatus) ? IDS_FILESUNAVAILABLE : IDS_FILESLOADING)));
            _filesTree->SetEmptyStateText(Resource(static_cast<unsigned>(FAILED(_folderAdmissionStatus) ? IDS_FILESUNAVAILABLE : IDS_FILESLOADING)));
            _filesTree->NotifyDataChanged();
            _filesTree->SetSelectedItemId(hasVisibleWorkspaceRow ? std::optional<std::uint64_t>(workspaceItemId) : std::nullopt);
            return;
        }
        const auto& snapshot = _folderSnapshot.value();
        std::optional<std::uint64_t> visibleFolderItemId;
        appendLocalSnapshotRows(snapshot, snapshot.root, 0u, 0u, visibleFolderItemId);
        _filesTree->SetEmptyStateText(Resource(IDS_NOMATCH));
        _filesTree->NotifyDataChanged();
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
        return;
    }

    // Multi-folder / GitHub section hierarchy
    std::size_t totalMarkdownCount{};
    bool anyAccessError{}, anyWatchError{}, anyEntryLimit{}, anyDepthLimit{}, anyLoading{}, anyGitHubTruncated{}, anyGitHubLimit{};
    std::optional<std::uint64_t> selectedId;

    const auto rootLabels = DisambiguateFolderLabels(activeRoots);
    for (std::size_t rootIndex = 0; rootIndex < activeRoots.size(); ++rootIndex)
    {
        const auto& rootPath           = activeRoots[rootIndex];
        const std::uint64_t rootBaseId = LocalRootGroupBaseId + static_cast<std::uint64_t>(rootIndex) * 0x100000ull;
        const FilesSnapshot* snapPtr{};
        if (activeRoots.size() == 1u && _folderSnapshot && SameRootPath(_folderSnapshot->root, rootPath))
            snapPtr = &_folderSnapshot.value();
        else
        {
            const auto it = std::ranges::find_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, rootPath); });
            if (it != _folderSnapshots.end())
                snapPtr = &(*it);
        }

        const bool rootExpanded = ! filter.empty() || ! _collapsedLocalRoots.contains(rootPath);
        if (snapPtr)
        {
            totalMarkdownCount += snapPtr->markdownCount;
            anyAccessError = anyAccessError || FAILED(snapPtr->status) || snapPtr->errorCount > 0;
            anyWatchError  = anyWatchError || FAILED(snapPtr->watchStatus);
            anyEntryLimit  = anyEntryLimit || snapPtr->entryLimitReached;
            anyDepthLimit  = anyDepthLimit || snapPtr->depthLimitReached;
        }
        else
        {
            anyLoading = true;
        }
        if (! filter.empty() && (! snapPtr || FilterMarkdownFiles(*snapPtr, filter).empty()))
            continue;

        DxUi::TreeItemData rootHeader;
        rootHeader.id          = rootBaseId;
        rootHeader.text        = rootIndex < rootLabels.size() ? rootLabels[rootIndex] : SanitizeUiText(rootPath.filename().native());
        rootHeader.iconText    = L"\xE8B7";
        rootHeader.tooltipText = SanitizeUiText(rootPath.native(), 512u);
        rootHeader.depth       = 0;
        rootHeader.hasChildren = true;
        rootHeader.expanded    = rootExpanded;
        if (snapPtr)
        {
            if (! rootExpanded)
                rootHeader.badgeText = std::to_wstring(snapPtr->markdownCount);
        }
        else
        {
            rootHeader.badgeText = L"\x2026";
        }
        FilesRowContextBinding rootBinding;
        rootBinding.kind      = FilesRowContextBinding::Kind::LocalRootHeader;
        rootBinding.localRoot = rootPath;
        _filesRowBindings.emplace_back(rootHeader.id, std::move(rootBinding));
        _filesModel.rows.push_back(std::move(rootHeader));

        if (! snapPtr || ! rootExpanded)
            continue;
        appendLocalSnapshotRows(*snapPtr, rootPath, rootBaseId, 1u, selectedId);
    }

    if (! _connectedGitHubRepos.empty())
    {
        bool ghGroupAdded{};
        const auto ensureGitHubGroupHeader = [&]
        {
            if (ghGroupAdded)
                return;
            ghGroupAdded = true;
            DxUi::TreeItemData ghGroup;
            ghGroup.id          = GitHubSectionGroupId;
            ghGroup.text        = Resource(IDS_GITHUB_REPOS_SECTION);
            ghGroup.iconText    = L"\xE774";
            ghGroup.hasChildren = true;
            ghGroup.expanded    = ! _githubSectionCollapsed || ! filter.empty();
            FilesRowContextBinding ghSectionBinding;
            ghSectionBinding.kind = FilesRowContextBinding::Kind::GitHubSectionHeader;
            _filesRowBindings.emplace_back(ghGroup.id, std::move(ghSectionBinding));
            _filesModel.rows.push_back(std::move(ghGroup));
        };

        if (filter.empty())
            ensureGitHubGroupHeader();

        for (const auto& connected : _connectedGitHubRepos)
        {
            anyGitHubTruncated = anyGitHubTruncated || connected.truncated;
            anyGitHubLimit     = anyGitHubLimit || connected.limitReached;
        }

        const bool expandSection = ! _githubSectionCollapsed || ! filter.empty();
        if (expandSection)
        {
            for (std::size_t repoIdx = 0; repoIdx < _connectedGitHubRepos.size(); ++repoIdx)
            {
                const auto& connected          = _connectedGitHubRepos[repoIdx];
                const std::wstring key         = connected.ref.owner + L"/" + connected.ref.repo;
                const std::wstring safeKey     = SanitizeUiText(key);
                const std::uint64_t repoBaseId = GitHubRowBaseId + static_cast<std::uint64_t>(repoIdx) * 0x100000ull;
                const std::wstring branchShown = ! connected.resolvedBranch.empty() ? connected.resolvedBranch : connected.ref.branch;
                const std::wstring safeBranch  = SanitizeUiText(branchShown, 128u);
                const bool repoExpanded        = ! filter.empty() || ! _collapsedGitHubRepos.contains(key);

                std::vector<bool> keep;
                bool anyFilteredMatch{};
                if (repoExpanded && connected.loaded)
                {
                    keep.assign(connected.entries.size(), false);
                    for (std::size_t i = 0; i < connected.entries.size(); ++i)
                    {
                        if (! connected.entries[i].directory && containsFilterInsensitive(connected.entries[i].name))
                        {
                            keep[i]          = true;
                            anyFilteredMatch = true;
                            for (auto p = connected.entries[i].parent; p != NoFilesParent && p < connected.entries.size(); p = connected.entries[p].parent)
                                keep[p] = true;
                        }
                    }
                }
                if (! filter.empty() && ! anyFilteredMatch)
                    continue;
                ensureGitHubGroupHeader();

                DxUi::TreeItemData repoHeader;
                repoHeader.id          = repoBaseId;
                repoHeader.parentId    = GitHubSectionGroupId;
                repoHeader.depth       = 1u;
                repoHeader.text        = safeBranch.empty() ? safeKey : (safeKey + L" (" + safeBranch + L")");
                repoHeader.iconText    = L"\xE774";
                repoHeader.tooltipText = safeBranch.empty() ? (L"https://github.com/" + safeKey) : (L"https://github.com/" + safeKey + L"/tree/" + safeBranch);
                repoHeader.hasChildren = true;
                repoHeader.expanded    = repoExpanded;
                if (connected.loading)
                {
                    repoHeader.badgeText = L"\x2026";
                    anyLoading           = true;
                }
                else if (connected.loaded)
                {
                    std::size_t repoMdCount{};
                    for (const auto& entry : connected.entries)
                        if (! entry.directory)
                            ++repoMdCount;
                    totalMarkdownCount += repoMdCount;
                    if (! repoExpanded)
                        repoHeader.badgeText = std::to_wstring(repoMdCount);
                }
                FilesRowContextBinding repoBinding;
                repoBinding.kind         = FilesRowContextBinding::Kind::GitHubRepoHeader;
                repoBinding.githubOwner  = connected.ref.owner;
                repoBinding.githubRepo   = connected.ref.repo;
                repoBinding.githubBranch = branchShown;
                _filesRowBindings.emplace_back(repoHeader.id, std::move(repoBinding));
                _filesModel.rows.push_back(std::move(repoHeader));

                if (! repoExpanded || ! connected.loaded)
                    continue;

                for (std::size_t i = 0; i < connected.entries.size(); ++i)
                {
                    if (! keep[i])
                        continue;
                    const auto& entry = connected.entries[i];
                    bool hidden{};
                    if (filter.empty())
                    {
                        for (auto p = entry.parent; p != NoFilesParent && p < connected.entries.size(); p = connected.entries[p].parent)
                        {
                            const std::wstring dirKey = key + L":" + connected.entries[p].path;
                            if (_collapsedGitHubDirs.contains(dirKey))
                            {
                                hidden = true;
                                break;
                            }
                        }
                    }
                    if (hidden)
                        continue;
                    const std::wstring dirKey = key + L":" + entry.path;
                    DxUi::TreeItemData item;
                    item.id          = repoBaseId + i + 1u;
                    item.text        = SanitizeUiText(entry.name);
                    item.iconText    = entry.directory ? L"\xE8B7" : L"\xE8A5";
                    item.tooltipText = SanitizeUiText(key + L"/" + entry.path, 512u);
                    item.depth       = entry.depth + 2u;
                    item.parentId    = (entry.parent == NoFilesParent) ? repoBaseId : (repoBaseId + entry.parent + 1u);
                    item.hasChildren = entry.directory && entry.markdownCount > 0u;
                    item.expanded    = item.hasChildren && (! filter.empty() || ! _collapsedGitHubDirs.contains(dirKey));
                    if (entry.directory && ! item.expanded)
                        item.badgeText = std::to_wstring(entry.markdownCount);
                    if (! selectedId && _session.origin.kind == DocumentOriginInfo::Kind::GitHub &&
                        EqualOrdinalIgnoreCase(_session.origin.owner, connected.ref.owner) &&
                        EqualOrdinalIgnoreCase(_session.origin.repo, connected.ref.repo) && _session.origin.relativePath == entry.path)
                        selectedId = item.id;
                    FilesRowContextBinding entryBinding;
                    entryBinding.kind         = entry.directory ? FilesRowContextBinding::Kind::GitHubDir : FilesRowContextBinding::Kind::GitHubFile;
                    entryBinding.githubOwner  = connected.ref.owner;
                    entryBinding.githubRepo   = connected.ref.repo;
                    entryBinding.githubBranch = branchShown;
                    entryBinding.githubPath   = entry.path;
                    entryBinding.isDirectory  = entry.directory;
                    _filesRowBindings.emplace_back(item.id, std::move(entryBinding));
                    _filesModel.rows.push_back(std::move(item));
                }
            }
        }
    }

    _filesTree->SetEmptyStateText(Resource(IDS_NOMATCH));
    _filesTree->NotifyDataChanged();
    if (_workspaceSelectionPreferred && hasVisibleWorkspaceRow)
        selectedId = workspaceItemId;
    else if (! selectedId && hasVisibleWorkspaceRow)
        selectedId = workspaceItemId;
    _filesTree->SetSelectedItemId(selectedId);
    auto footer       = totalMarkdownCount == 0u && anyLoading ? Resource(IDS_FILESLOADING) : FormatStringResource(nullptr, IDS_FILESCOUNT, totalMarkdownCount);
    const auto append = [&](std::wstring detail) { footer = FormatStringResource(nullptr, IDS_FILESFOOTERDETAIL, footer, detail); };
    if (anyAccessError || FAILED(_folderAdmissionStatus))
        append(Resource(IDS_FILESACCESSDETAIL));
    if (anyWatchError)
        append(Resource(IDS_FILESWATCHDETAIL));
    if (anyEntryLimit)
        append(FormatStringResource(nullptr, IDS_FILESENTRYLIMIT, FilesLimits{}.maximumEntries));
    if (anyGitHubLimit)
        append(FormatStringResource(nullptr, IDS_FILESENTRYLIMIT, MaximumGitHubTreeEntries));
    if (anyGitHubTruncated)
        append(Resource(IDS_GITHUB_TREE_TRUNCATED));
    if (anyDepthLimit)
        append(FormatStringResource(nullptr, IDS_FILESDEPTHLIMIT, FilesLimits{}.maximumDepth));
    setFooter(std::move(footer));
}

void Application::UpdateCollapsedFoldersFromSnapshot(const FilesSnapshot* previousSnap, const FilesSnapshot& nextSnap, bool pruneRemovedUnderRoot)
{
    std::set<std::wstring, OrdinalIgnoreCaseLess> previousDirectories;
    std::set<std::wstring, OrdinalIgnoreCaseLess> currentDirectories;
    if (previousSnap)
        for (const auto& entry : previousSnap->entries)
            if (entry.directory)
                previousDirectories.insert(entry.path.native());
    for (const auto& entry : nextSnap.entries)
        if (entry.directory)
        {
            currentDirectories.insert(entry.path.native());
            if (IsPathInsideOrEqualDirectory(_session.path, entry.path))
                _collapsedFolders.erase(entry.path);
            else if (! previousDirectories.contains(entry.path.native()))
                _collapsedFolders.insert(entry.path);
        }
    std::erase_if(_collapsedFolders,
                  [&](const auto& path)
    {
        if (pruneRemovedUnderRoot && ! IsPathInsideOrEqualDirectory(path, nextSnap.root))
            return false;
        return ! currentDirectories.contains(path.native());
    });
}

void Application::FolderCompleted()
{
    bool updated{};
    if (auto result = _folderFiles.TakeResult(); result && SameRootPath(result->root, _folderRoot) && result->generation == _folderFiles.Generation())
    {
        UpdateCollapsedFoldersFromSnapshot(_folderSnapshot ? &*_folderSnapshot : nullptr, *result, false);
        _folderSnapshot  = result;
        _folderSnapshots = {*result};
        updated          = true;
    }
    for (auto& snap : _multiFolderFiles.TakeResults())
    {
        if (std::ranges::none_of(_settings.filesRoots, [&](const auto& r) { return SameRootPath(r, snap.root); }))
            continue;
        const auto existing = std::ranges::find_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, snap.root); });
        UpdateCollapsedFoldersFromSnapshot(existing != _folderSnapshots.end() ? &(*existing) : nullptr, snap, true);
        if (existing != _folderSnapshots.end())
            *existing = std::move(snap);
        else
            _folderSnapshots.push_back(std::move(snap));
        updated = true;
    }
    if (updated)
        RefreshFiles();
}
void Application::EnsureFilesRoot()
{
    if (! _settings.filesRoots.empty() || _session.path.empty())
        return;
    std::error_code error;
    const auto absolute = std::filesystem::absolute(_session.path, error);
    if (error)
        return;
    const auto normalized = NormalizeRootPath(absolute.parent_path());
    _settings.filesRoot   = normalized;
    _settings.filesRoots  = {normalized};
    SavePreferences();
}
void Application::SelectFilesRoot(const std::filesystem::path& path)
{
    if (! path.is_absolute() || path.native().find(L'\0') != std::wstring::npos)
        return;
    const auto normalized        = NormalizeRootPath(path);
    _settings.filesRoot          = normalized;
    _settings.filesRoots         = {normalized};
    _settings.showFiles          = true;
    _settings.filesPinMode       = PanePinMode::Pinned;
    _lastPinnedSide              = PaneSide::Left;
    _workspaceSelectionPreferred = false;
    _filesFilter->SetText(L"");
    RefreshFiles(true);
    Layout();
    SavePreferences();
    _host.SetFocusControl(_filesTree, MayRequestNativeFocus());
}
void Application::AddFilesRoot(const std::filesystem::path& path)
{
    if (! path.is_absolute() || path.native().find(L'\0') != std::wstring::npos)
        return;
    const auto normalized = NormalizeRootPath(path);
    if (std::ranges::any_of(_settings.filesRoots, [&](const auto& existing) { return SameRootPath(existing, normalized); }))
    {
        _settings.showFiles    = true;
        _settings.filesPinMode = PanePinMode::Pinned;
        RefreshFiles();
        Layout();
        return;
    }
    if (_settings.filesRoots.size() >= MaximumFilesRoots)
    {
        Status(IDS_WORKSPACEFULL);
        return;
    }
    _settings.filesRoots.push_back(normalized);
    _settings.filesRoot          = _settings.filesRoots.front();
    _settings.showFiles          = true;
    _settings.filesPinMode       = PanePinMode::Pinned;
    _lastPinnedSide              = PaneSide::Left;
    _workspaceSelectionPreferred = false;
    _filesFilter->SetText(L"");
    RefreshFiles(true);
    Layout();
    SavePreferences();
    _host.SetFocusControl(_filesTree, MayRequestNativeFocus());
}
void Application::RemoveFilesRoot(const std::filesystem::path& path)
{
    const auto normalized = NormalizeRootPath(path);
    if (std::erase_if(_settings.filesRoots, [&](const auto& existing) { return SameRootPath(existing, normalized); }) == 0u)
        return;
    std::erase_if(_folderSnapshots, [&](const FilesSnapshot& s) { return SameRootPath(s.root, normalized); });
    _collapsedLocalRoots.erase(normalized);
    _settings.filesRoot = _settings.filesRoots.empty() ? std::filesystem::path{} : _settings.filesRoots.front();
    RefreshFiles(true);
    Layout();
    SavePreferences();
}
void Application::OpenFolder()
{
    AbandonStartupRestore();
    AskWorkspaceThen([this]
    {
        const auto path = PickFile(false, false, true);
        if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached())
            return;
        ResetWorkspace(true);
        SetDocument({});
        SelectFilesRoot(path);
    });
}
void Application::AddFolder()
{
    AbandonStartupRestore();
    const auto path = PickFile(false, false, true);
    if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached())
        return;
    AddFilesRoot(path);
}
bool Application::SwitchWorkspaceDocument(std::size_t index, bool savePreviousRecovery, bool refreshFiles)
{
    _pendingBitmapPaste.reset();
    if (! CanAcceptCommand())
        return false;
    if (! EnsurePrimaryEditorAuthoritative())
        return false;
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
        _imageWatchAcknowledgedSemantic.reset();
        _imageWatchWasRichVisible = false;
        _outlinePreviewBaseline.reset();
        _find.InvalidateMatches();
        _spell.appliedKey.reset();
        _spell.issues.clear();
        _spell.suppressedTypingWord.reset();
        _spell.typingSuppressionActive = false;
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
    if (! EnsurePrimaryEditorAuthoritative())
        return false;
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
void Application::ResetWorkspace(bool retireRecovery, bool forceOnClose)
{
    if (! EnsurePrimaryEditorAuthoritative() && ! forceOnClose)
        return;
    if (retireRecovery)
    {
        for (const auto& document : _inactiveDocuments)
            if (FAILED(_recovery.SubmitDocument(document.metadata.key, std::nullopt, true)))
                Status(IDS_RECOVERYFAILED);
        if (FAILED(_recovery.SubmitDocument(_session.key, std::nullopt, true)))
            Status(IDS_RECOVERYFAILED);
    }
    _inactiveDocuments.clear();
    _workspaceCollapsed          = false;
    _workspaceSelectionPreferred = false;
    _customDictionary.ClearAllIgnoreOnce();
    _session.key = 0;
    _session.spellLanguageOverride.reset();
    _spell.appliedKey.reset();
    _spell.issues.clear();
    _spell.suppressedTypingWord.reset();
    _spell.typingSuppressionActive = false;
    _spell.pendingMenu.reset();
    _spell.pendingLeftClick.reset();
    _spell.leftDownArmed = false;
    _workspaceApprovals.clear();
    RefreshLeftRailDots();
}
void Application::CheckpointWorkspaceRecovery(RecoverySnapshotKind kind)
{
    CheckpointRecovery(true, kind);
    unsigned order = 1u;
    for (const auto& document : _inactiveDocuments)
    {
        std::optional<RecoverySnapshot> snapshot;
        if (document.dirty)
        {
            RecoverySnapshot snap;
            snap.text               = document.state.Text();
            snap.originalPath       = document.metadata.path;
            snap.bom                = document.metadata.bom;
            snap.selection          = document.state.GetSelection();
            snap.generation         = ++_recoveryGeneration;
            snap.kind               = kind;
            snap.origin             = document.metadata.origin;
            snap.workspaceOrder     = order++;
            snap.wasActive          = false;
            snap.savedBaselineStamp = document.metadata.stamp;
            snap.recovered          = document.metadata.recovered;
            snapshot                = std::move(snap);
        }
        if (FAILED(_recovery.SubmitDocument(document.metadata.key, std::move(snapshot), true)))
            Status(IDS_RECOVERYFAILED);
    }
}
void Application::OpenPath(const std::filesystem::path& path)
{
    if (! IsMarkdownFile(path))
        return;
    const auto isDirInSnapshots = [&](const std::filesystem::path& target)
    {
        if (_folderSnapshot)
        {
            const auto& entries = _folderSnapshot->entries;
            if (const auto item = std::find_if(entries.begin(), entries.end(), [&](const FilesEntry& entry) { return entry.path == target; });
                item != entries.end() && item->directory)
                return true;
        }
        for (const auto& snap : _folderSnapshots)
            if (const auto item = std::find_if(snap.entries.begin(), snap.entries.end(), [&](const FilesEntry& entry) { return entry.path == target; });
                item != snap.entries.end() && item->directory)
                return true;
        return false;
    };
    const auto matchesAnySnapshot = [&](const std::filesystem::path& a, const std::filesystem::path& b)
    {
        if (a == b)
            return true;
        if (_folderSnapshot && MatchesFilesPath(_folderSnapshot.value(), a, b))
            return true;
        for (const auto& snap : _folderSnapshots)
            if (MatchesFilesPath(snap, a, b))
                return true;
        return false;
    };
    if (isDirInSnapshots(path))
        return;
    if (matchesAnySnapshot(path, _session.path))
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
        if (matchesAnySnapshot(path, _inactiveDocuments[index].metadata.path))
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
        if (! EnsurePrimaryEditorAuthoritative())
            return;
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
            _customDictionary.ClearIgnoreOnceForWorkspace(closedKey);
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
    const auto matchesExtension = [&](std::wstring_view expected) noexcept { return EqualOrdinalIgnoreCase(extension, expected); };
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
        add(IDS_ADDFOLDER, Command::AddFolder);
        add(IDS_OPENGITHUB, Command::OpenGitHub, IDS_SHORTCUT_OPENGITHUB);
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
    if (_saveConfirmSession && _saveConfirmSession->sequentialSaveActive)
        return true;
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
            _settings.showFiles    = false;
            _settings.filesPinMode = PanePinMode::Unpinned;
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

bool Application::HasClipboardDropFiles() const
{
#if defined(ENABLE_TESTS)
    if (_options.clipboardDropReaderForTest)
    {
        const auto testPayload = _options.clipboardDropReaderForTest();
        return testPayload && ! testPayload->paths.empty();
    }
#endif
    FileClipboardPayload payload;
    return SUCCEEDED(ReadDropFilesFromClipboard(_window.get(), payload)) && ! payload.paths.empty();
}

std::filesystem::path Application::ActivePasteTargetDirectory() const
{
    if (_filesTree)
    {
        if (const auto selectedId = _filesTree->GetSelectedItemId())
        {
            const auto it = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == selectedId.value(); });
            if (it != _filesRowBindings.end())
            {
                const auto& b = it->second;
                if (b.kind == FilesRowContextBinding::Kind::LocalRootHeader)
                    return b.localRoot;
                if (b.kind == FilesRowContextBinding::Kind::LocalEntry)
                    return b.isDirectory ? b.localPath : b.localPath.parent_path();
                if (b.kind == FilesRowContextBinding::Kind::WorkspaceDoc)
                    return (b.githubOwner.empty() && ! b.localPath.empty()) ? b.localPath.parent_path() : std::filesystem::path{};
                return {};
            }
        }
    }
    if (! _settings.filesRoots.empty())
        return _settings.filesRoots.front();
    return {};
}

void Application::OnFilesContextMenu(std::uint64_t id, POINT screenPoint)
{
    if (HasActiveModalUi() || IsClosing())
        return;
    const auto it = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == id; });
    if (it == _filesRowBindings.end())
        return;
    if (_options.headless)
    {
        ShowFilesContextMenuNow(it->second, screenPoint, false);
        return;
    }
    _deferredFilesContextMenu = PendingFilesContextMenu{it->second, screenPoint, false};
    if (_window)
        (void)PostMessageW(_window.get(), FilesContextMenuRequested, 0, 0);
}

void Application::ShowFilesContextMenuNow(const FilesRowContextBinding& target, POINT screenPoint, bool keyboardInvocation)
{
    if (HasActiveModalUi() || IsClosing() || target.kind == FilesRowContextBinding::Kind::None ||
        target.kind == FilesRowContextBinding::Kind::GitHubSectionHeader)
        return;

    std::vector<DxUi::MenuFlyoutItem> items;
    auto add = [&](unsigned labelId, Command cmd, bool enabled = true)
    {
        DxUi::MenuFlyoutItem item;
        item.text      = Resource(labelId);
        item.commandId = static_cast<int>(cmd) + 1;
        item.enabled   = enabled;
        items.push_back(std::move(item));
    };

    const bool hasLocalFile =
        (target.kind == FilesRowContextBinding::Kind::LocalEntry || target.kind == FilesRowContextBinding::Kind::WorkspaceDoc) && ! target.localPath.empty();
    if (hasLocalFile)
    {
        if (! target.isDirectory && IsMarkdownFile(target.localPath))
            add(IDS_CTX_OPEN, Command::FilesCtxOpen);
        add(IDS_CTX_REVEAL_EXPLORER, Command::FilesCtxRevealExplorer);
        add(_redSalamanderInstalledPath ? IDS_CTX_REVEAL_REDSALAMANDER : IDS_CTX_REVEAL_REDSALAMANDER_MISSING,
            Command::FilesCtxRevealRedSalamander,
            _redSalamanderInstalledPath.has_value());
        add(IDS_CTX_COPY_FULL_PATH, Command::FilesCtxCopyFullPath);
        if (! target.localRoot.empty() || target.kind == FilesRowContextBinding::Kind::LocalEntry)
            add(IDS_CTX_COPY_REL_PATH, Command::FilesCtxCopyRelPath);
        add(IDS_CTX_RENAME, Command::FilesCtxRename);
        add(IDS_CTX_CUT, Command::FilesCtxCut);
        add(IDS_CTX_COPY, Command::FilesCtxCopy);
        add(IDS_CTX_PASTE, Command::FilesCtxPaste, HasClipboardDropFiles());
    }
    else if (target.kind == FilesRowContextBinding::Kind::LocalRootHeader)
    {
        add(IDS_CTX_REVEAL_EXPLORER, Command::FilesCtxRevealExplorer);
        add(_redSalamanderInstalledPath ? IDS_CTX_REVEAL_REDSALAMANDER : IDS_CTX_REVEAL_REDSALAMANDER_MISSING,
            Command::FilesCtxRevealRedSalamander,
            _redSalamanderInstalledPath.has_value());
        add(IDS_CTX_COPY_FULL_PATH, Command::FilesCtxCopyFullPath);
        add(IDS_CTX_PASTE, Command::FilesCtxPaste, HasClipboardDropFiles());
        add(IDS_REMOVE_FOLDER, Command::FilesCtxRemoveRoot);
    }
    else if (target.kind == FilesRowContextBinding::Kind::GitHubRepoHeader)
    {
        add(IDS_GITHUB_REMOVE_REPO, Command::FilesCtxRemoveRepo);
    }
    else if (target.kind == FilesRowContextBinding::Kind::GitHubFile ||
             (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc && ! target.githubOwner.empty()))
    {
        add(IDS_CTX_OPEN, Command::FilesCtxOpen);
        add(IDS_CTX_COPY_FULL_PATH, Command::FilesCtxCopyFullPath);
        add(IDS_CTX_COPY_REL_PATH, Command::FilesCtxCopyRelPath);
    }
    else if (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc)
    {
        add(IDS_CTX_OPEN, Command::FilesCtxOpen);
    }

    if (items.empty())
        return;
    if (_options.headless)
    {
        _pendingContextTarget = target;
        return;
    }

    _host.ReleaseMouseCapture();
    _filesContextMenuOpen = true;
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = keyboardInvocation;
    if (! DxUi::ContextMenu::ShowAsync(_window.get(),
                                       screenPoint,
                                       items,
                                       _host.GetTheme(),
                                       [this, target](std::optional<int> selected) noexcept
    {
        RunUiAction([&]
        {
            _filesContextMenuOpen       = false;
            DxUi::Control* restoreFocus = (_filesTree && _filesPane && _filesPane->IsVisible()) ? static_cast<DxUi::Control*>(_filesTree) : ActiveEditor();
            if (selected)
            {
                _pendingContextTarget = target;
                const auto cmd        = static_cast<Command>(selected.value() - 1);
                if (cmd == Command::FilesCtxOpen)
                {
                    if (IsPaneOverlayActive(PaneSide::Left))
                        DismissHoverOverlays(true);
                    restoreFocus = ActiveEditor();
                }
            }
            OnMenuClosed(selected, restoreFocus);
        });
    },
                                       callbacks))
        _filesContextMenuOpen = false;
}

void Application::ExecuteFilesContextAction(Command command)
{
    const auto target = _pendingContextTarget;
    if (command == Command::FilesCtxOpen)
    {
        if (IsPaneOverlayActive(PaneSide::Left))
            DismissHoverOverlays(true);
        if (target.kind == FilesRowContextBinding::Kind::LocalEntry)
            OpenPath(target.localPath);
        else if (target.kind == FilesRowContextBinding::Kind::GitHubFile)
            OpenGitHubDocument(target.githubOwner, target.githubRepo, target.githubBranch, target.githubPath);
        else if (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc)
        {
            if (target.workspaceKey != _session.key)
            {
                for (std::size_t i = 0; i < _inactiveDocuments.size(); ++i)
                    if (_inactiveDocuments[i].metadata.key == target.workspaceKey)
                    {
                        (void)SwitchWorkspaceDocument(i);
                        break;
                    }
            }
        }
        return;
    }
    if (command == Command::FilesCtxRevealExplorer || command == Command::FilesCtxRevealRedSalamander)
    {
        const std::filesystem::path targetPath = target.kind == FilesRowContextBinding::Kind::LocalRootHeader ? target.localRoot : target.localPath;
        if (targetPath.empty())
            return;
        WorkspaceFileOpRequest req;
        req.kind =
            command == Command::FilesCtxRevealExplorer ? WorkspaceFileOpRequest::Kind::RevealExplorer : WorkspaceFileOpRequest::Kind::RevealRedSalamander;
        req.requestId             = ++_nextFileOpRequestId;
        req.sourcePath            = targetPath;
        req.redSalamanderOverride = _settings.redSalamanderPath;
        if (FAILED(_fileOps.Submit(std::move(req))))
            Status(IDS_BUSY);
        return;
    }
    if (command == Command::FilesCtxCopyFullPath || command == Command::FilesCtxCopyRelPath)
    {
        std::wstring copied;
        if (target.kind == FilesRowContextBinding::Kind::GitHubFile ||
            (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc && ! target.githubOwner.empty()))
        {
            if (command == Command::FilesCtxCopyFullPath)
            {
                const std::string encBranch = PercentEncodeUrlPathSegments(target.githubBranch.empty() ? L"main" : target.githubBranch);
                const std::string encPath   = PercentEncodeUrlPathSegments(target.githubPath);
                std::wstring wideBranch;
                wideBranch.reserve(encBranch.size());
                for (const char ch : encBranch)
                    wideBranch.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
                std::wstring widePath;
                widePath.reserve(encPath.size());
                for (const char ch : encPath)
                    widePath.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
                copied = L"https://github.com/" + target.githubOwner + L"/" + target.githubRepo + L"/blob/" + wideBranch + L"/" + widePath;
            }
            else
            {
                copied = target.githubPath;
            }
        }
        else if (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc)
        {
            std::filesystem::path resolvedPath = target.localPath;
            if (resolvedPath.empty())
            {
                const std::filesystem::path* p = &_session.path;
                if (target.workspaceKey != _session.key)
                    for (const auto& doc : _inactiveDocuments)
                        if (doc.metadata.key == target.workspaceKey)
                        {
                            p = &doc.metadata.path;
                            break;
                        }
                resolvedPath = *p;
            }
            if (command == Command::FilesCtxCopyFullPath || target.localRoot.empty() || resolvedPath.empty())
                copied = resolvedPath.lexically_normal().native();
            else
                copied = OrdinalRelativeOrFullPath(resolvedPath, target.localRoot);
        }
        else
        {
            const std::filesystem::path targetPath = target.kind == FilesRowContextBinding::Kind::LocalRootHeader ? target.localRoot : target.localPath;
            if (command == Command::FilesCtxCopyFullPath || target.localRoot.empty())
                copied = targetPath.lexically_normal().native();
            else
                copied = OrdinalRelativeOrFullPath(targetPath, target.localRoot);
        }
        if (copied.empty())
            return;
        HRESULT hr = S_OK;
#if defined(ENABLE_TESTS)
        if (_options.clipboardWriterForTest)
            hr = _options.clipboardWriterForTest(copied);
        else
#endif
            hr = WriteUnicodeTextToClipboard(_window.get(), copied);
        if (SUCCEEDED(hr))
            Status(IDS_FILE_COPIED_PATH);
        else
            _feedback->SetText(FormatStringResource(nullptr, IDS_FILE_OP_FAILED, ErrorText(hr)));
        return;
    }
    if (command == Command::FilesCtxRename)
    {
        if ((target.kind == FilesRowContextBinding::Kind::LocalEntry || target.kind == FilesRowContextBinding::Kind::WorkspaceDoc) &&
            ! target.localPath.empty() && target.githubOwner.empty())
            ShowRenameFileModal(target.localPath, target.isDirectory);
        return;
    }
    if (command == Command::FilesCtxCut || command == Command::FilesCtxCopy)
    {
        if ((target.kind != FilesRowContextBinding::Kind::LocalEntry && target.kind != FilesRowContextBinding::Kind::WorkspaceDoc) ||
            target.localPath.empty() || ! target.githubOwner.empty())
            return;
        FileClipboardPayload payload;
        payload.paths.push_back(target.localPath.lexically_normal());
        payload.effect = command == Command::FilesCtxCut ? DropEffectKind::Move : DropEffectKind::Copy;
        HRESULT hr     = S_OK;
#if defined(ENABLE_TESTS)
        if (_options.clipboardDropWriterForTest)
            hr = _options.clipboardDropWriterForTest(payload);
        else
#endif
            hr = WriteDropFilesToClipboard(_window.get(), payload);
        if (SUCCEEDED(hr))
        {
            _cutFilePaths.clear();
            if (payload.effect == DropEffectKind::Move)
                _cutFilePaths.insert(NormalizeRootPath(target.localPath).native());
            RefreshFiles();
        }
        return;
    }
    if (command == Command::FilesCtxPaste)
    {
        std::filesystem::path destDir;
        if (target.kind == FilesRowContextBinding::Kind::LocalRootHeader)
            destDir = target.localRoot;
        else if (target.kind == FilesRowContextBinding::Kind::LocalEntry)
            destDir = target.isDirectory ? target.localPath : target.localPath.parent_path();
        else if (target.kind == FilesRowContextBinding::Kind::WorkspaceDoc && target.githubOwner.empty() && ! target.localPath.empty())
            destDir = target.localPath.parent_path();
        else
            return;
        if (destDir.empty())
            return;
        FileClipboardPayload payload;
        HRESULT hr = S_OK;
#if defined(ENABLE_TESTS)
        if (_options.clipboardDropReaderForTest)
        {
            if (auto testPayload = _options.clipboardDropReaderForTest())
                payload = std::move(*testPayload);
            else
                hr = S_FALSE;
        }
        else
#endif
            hr = ReadDropFilesFromClipboard(_window.get(), payload);
        if (SUCCEEDED(hr) && ! payload.paths.empty())
            BeginCopyOrMoveToDirectory(std::move(payload.paths), destDir, payload.effect == DropEffectKind::Move);
        return;
    }
    if (command == Command::FilesCtxRemoveRoot)
    {
        if (! target.localRoot.empty())
            RemoveFilesRoot(target.localRoot);
        return;
    }
    if (command == Command::FilesCtxRemoveRepo)
    {
        if (! target.githubOwner.empty() && ! target.githubRepo.empty())
            RemoveConnectedGitHubRepo(target.githubOwner, target.githubRepo);
        return;
    }
}

void Application::RebaseWorkspacePathsAfterFileOp(const std::filesystem::path& sourcePath, const std::filesystem::path& destPath, bool isDirectory, bool move)
{
    if (! move || sourcePath.empty() || destPath.empty())
        return;
    const auto srcNorm = NormalizeRootPath(sourcePath);
    const auto dstNorm = NormalizeRootPath(destPath);
    const auto& srcStr = srcNorm.native();
    auto rebaseOne     = [&](std::filesystem::path& p)
    {
        if (p.empty())
            return false;
        const auto norm = NormalizeRootPath(p);
        if (! isDirectory)
        {
            if (EqualOrdinalIgnoreCase(norm.native(), srcStr))
            {
                p = dstNorm;
                return true;
            }
            return false;
        }
        if (! IsPathInsideOrEqualDirectory(norm, srcNorm))
            return false;
        const auto& normStr = norm.native();
        if (EqualOrdinalIgnoreCase(normStr, srcStr))
        {
            p = dstNorm;
            return true;
        }
        std::size_t suffixStart = srcStr.size();
        if (! srcStr.ends_with(L'\\') && suffixStart < normStr.size() && normStr[suffixStart] == L'\\')
            ++suffixStart;
        p = NormalizeRootPath(dstNorm / std::wstring_view(normStr).substr(suffixStart));
        return true;
    };

    bool dirtyDocRebased{};
    const bool activeRebased = rebaseOne(_session.path);
    if (activeRebased)
    {
        if (_editor)
            (void)AcceptSourceResult(_editor->TrySetDocumentFilePath(_session.path.native()));
        if (_mirror)
            (void)AcceptSourceResult(_mirror->TrySetDocumentFilePath(_session.path.native()));
        const auto displayName = ActiveDocumentDisplayName();
        if (_title)
            _title->SetText(displayName);
        UpdateCaption(FormatStringResource(nullptr, IDS_WINDOWTITLE, Dirty() ? L"\x2022 " : L"", displayName));
        RememberActiveDocument();
        Layout();
        if (Dirty())
            dirtyDocRebased = true;
    }
    for (auto& doc : _inactiveDocuments)
    {
        if (rebaseOne(doc.metadata.path) && doc.dirty)
            dirtyDocRebased = true;
    }
    if (rebaseOne(_settings.lastDocument) && ! activeRebased)
        SavePreferences();
    if (isDirectory && ! _collapsedFolders.empty())
    {
        std::set<std::filesystem::path, PathOrdinalIgnoreCaseLess> updatedCollapsed;
        for (auto folder : _collapsedFolders)
        {
            (void)rebaseOne(folder);
            updatedCollapsed.insert(std::move(folder));
        }
        _collapsedFolders = std::move(updatedCollapsed);
    }
    if (dirtyDocRebased)
        RefreshLeftRailDots();
    CheckpointWorkspaceRecovery(RecoverySnapshotKind::CrashRecovery);
}
} // namespace RedTilde
