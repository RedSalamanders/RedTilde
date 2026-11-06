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
void CollectSourceFindMatches(std::wstring_view text, std::wstring_view query, bool matchCase, std::vector<DxUi::RichText::MarkdownRange> &matches)
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
            const int found = FindStringOrdinal(FIND_FROMSTART, text.data() + from, static_cast<int>(text.size() - from), query.data(),
                                                static_cast<int>(query.size()), TRUE);
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
template <class... Args> std::wstring Text(unsigned id, Args &&...args)
{
    return std::vformat(ResourceView(id), std::make_wformat_args(args...));
}
} // namespace
bool Application::PreviewOutlineMatchesCommitted() const
{
    return previewCommittedOutline && documentInfo.headings.size() == previewCommittedOutline->size() &&
           std::equal(documentInfo.headings.begin(), documentInfo.headings.end(), previewCommittedOutline->begin(),
                      [](const Heading &heading, const auto &identity)
                      { return heading.level == identity.first && heading.text == identity.second; });
}
void Application::RefreshOutline()
{
    ++outlineRowsRevision;
    const auto previousRows = outlineModel.rows.size();
    outlineModel.rows.clear();
    std::vector<std::pair<unsigned, std::uint64_t>> ancestors;
    for (std::size_t i = 0; i < documentInfo.headings.size(); ++i)
    {
        const auto &heading = documentInfo.headings[i];
        if (heading.text.empty())
            continue;
        while (!ancestors.empty() && ancestors.back().first >= heading.level)
            ancestors.pop_back();
        DxUi::TreeItemData item;
        item.id = i + 1;
        item.text = heading.text;
        item.depth = static_cast<unsigned>(ancestors.size());
        if (!ancestors.empty())
            item.parentId = ancestors.back().second;
        item.expanded = !collapsedHeadings.contains(item.id);
        item.hasChildren = i + 1 < documentInfo.headings.size() && documentInfo.headings[i + 1].level > heading.level;
        const bool hidden =
            std::any_of(ancestors.begin(), ancestors.end(), [&](const auto &ancestor) { return collapsedHeadings.contains(ancestor.second); });
        if (!hidden)
            outlineModel.rows.push_back(std::move(item));
        ancestors.emplace_back(heading.level, i + 1);
    }
    outlineTree->NotifyDataChanged();
    const auto headingCount = documentInfo.headings.size();
    outlineCount->SetText(Text(IDS_OUTLINECOUNT, headingCount));
    if (outlineModel.rows.size() != previousRows)
        Layout();
    if (findBar->IsVisible())
        RefreshFind();
}
void Application::UpdateCurrentFileDirtyBadge(bool dirty)
{
    lastFilesDirty = dirty;
    auto *const tree = filesTree;
    const auto lifetime = GetControlLifetimeToken(*tree);
    const auto key = activeDocumentKey;
    if (!inactiveDocuments.empty())
    {
        const auto id = WorkspaceGroupId + activeDocumentKey + 1;
        const auto item = std::find_if(filesModel.rows.begin(), filesModel.rows.end(), [&](const auto &row) { return row.id == id; });
        if (item != filesModel.rows.end())
        {
            const auto badge = dirty ? Resource(IDS_FILESEDITED) : std::wstring{};
            if (item->badgeText != badge || item->badgeIsDot != dirty)
            {
                item->badgeText = badge;
                item->badgeIsDot = dirty;
                const bool published = tree->NotifyItemPresentationChanged(id);
                if (closing || lifetime.expired() || filesTree != tree || host.GetRoot() != root || activeDocumentKey != key || Dirty() != dirty)
                    return;
                if (!published)
                {
                    RefreshFiles();
                    return;
                }
            }
        }
    }
    const auto requested = settings.showFiles ? settings.filesRoot : std::filesystem::path{};
    if (requested != folderRoot)
    {
        RefreshFiles();
        return;
    }
    if (!folderSnapshot || folderSnapshot->root != requested)
        return;
    const auto *const model = filesTree->GetModel();
    if (model != &filesModel)
    {
        RefreshFiles();
        return;
    }

    const auto &snapshot = folderSnapshot.value();
    const auto entry = std::find_if(snapshot.entries.begin(), snapshot.entries.end(), [&](const auto &candidate)
                                    { return !candidate.directory && MatchesFilesPath(snapshot, candidate.path, documentPath); });
    if (entry == snapshot.entries.end())
        return;
    const auto itemId = static_cast<std::uint64_t>(std::distance(snapshot.entries.begin(), entry) + 1);
    auto row = std::find_if(filesModel.rows.begin(), filesModel.rows.end(), [&](const auto &candidate) { return candidate.id == itemId; });
    if (row == filesModel.rows.end())
        return; // Filtering or a future visibility rule hides this file; a later structural refresh reads Dirty().

    const bool edited = dirty || std::ranges::any_of(inactiveDocuments, [&](const auto &document)
                                                     { return document.dirty && MatchesFilesPath(snapshot, document.path, documentPath); });
    const auto badgeText = edited ? Resource(IDS_FILESEDITED) : std::wstring{};
    if (row->badgeText == badgeText && row->badgeIsDot == edited)
        return;
    row->badgeText = badgeText;
    row->badgeIsDot = edited;

    const auto snapshotGeneration = snapshot.generation;
    const auto snapshotRevision = snapshot.revision;
    const auto modelRebuilds = activity.filesModelRebuilds;
    if (filesTree->NotifyItemPresentationChanged(itemId))
        return;

    // UI Automation callbacks can re-enter the app. Fall back to a full refresh
    // only while the exact snapshot/model publication we just changed is still current.
    if (model != &filesModel || filesTree->GetModel() != model || !folderSnapshot || folderSnapshot->root != requested ||
        folderSnapshot->generation != snapshotGeneration || folderSnapshot->revision != snapshotRevision || Dirty() != dirty ||
        lastFilesDirty != dirty || activity.filesModelRebuilds != modelRebuilds)
        return;
    const auto currentRow =
        std::find_if(filesModel.rows.begin(), filesModel.rows.end(), [&](const auto &candidate) { return candidate.id == itemId; });
    if (currentRow != filesModel.rows.end() && currentRow->badgeText == badgeText && currentRow->badgeIsDot == edited)
        RefreshFiles();
}
void Application::RefreshFiles(bool rootChanged)
{
    const auto setFooter = [&](std::wstring text)
    {
        if (filesCount->GetText() != text)
        {
            filesCount->SetText(std::move(text));
            Layout();
        }
    };
    const auto requested = settings.showFiles ? settings.filesRoot : std::filesystem::path{};
    if (rootChanged || requested != folderRoot)
    {
        if (requested != folderRoot)
        {
            folderRoot = requested;
            folderSnapshot.reset();
            collapsedFolders.clear();
            HRESULT result = S_OK;
            if (requested.empty())
            {
                folderFiles.Close();
                folderFilesActive = false;
            }
            else
            {
                if (!folderFilesActive)
                {
                    result = folderFiles.Initialize(window.get(), FolderReady);
                    folderFilesActive = SUCCEEDED(result);
                }
                if (folderFilesActive)
                    result = folderFiles.SetRoot(requested);
            }
            folderAdmissionStatus = result;
        }
    }
    if (rootChanged && folderSnapshot)
        for (const auto &entry : folderSnapshot->entries)
            if (entry.directory)
            {
                const auto relative = documentPath.lexically_relative(entry.path);
                if (!relative.empty() && *relative.begin() != L"..")
                    collapsedFolders.erase(entry.path);
            }
    filesTitle->SetText(requested.empty() ? Resource(IDS_FILES) : requested.filename().empty() ? requested.native() : requested.filename().native());
    filesTitle->SetTooltipText(requested.native());
    filesTitle->SetAccessibleHelpText(requested.native());
    lastFilesDirty = Dirty();
    ++activity.filesModelRebuilds;
    filesModel.rows.clear();
    visibleFilePaths.clear();
    const std::wstring filter(filesFilter->GetText());
    if (!inactiveDocuments.empty())
    {
        std::vector<DxUi::TreeItemData> documents;
        const auto append = [&](unsigned key, const std::filesystem::path &path, bool dirty)
        {
            const auto name = path.empty() ? Resource(IDS_UNTITLED) : path.filename().native();
            if (!filter.empty() &&
                FindNLSStringEx(LOCALE_NAME_INVARIANT, FIND_FROMSTART | NORM_IGNORECASE, name.data(), static_cast<int>(name.size()), filter.data(),
                                static_cast<int>(filter.size()), nullptr, nullptr, nullptr, 0) < 0)
                return;
            DxUi::TreeItemData item;
            item.id = WorkspaceGroupId + key + 1;
            item.parentId = WorkspaceGroupId;
            item.depth = 1;
            item.text = name;
            item.iconText = L"\xE8A5";
            item.tooltipText = path.native();
            item.badgeIsDot = dirty;
            if (dirty)
                item.badgeText = Resource(IDS_FILESEDITED);
            documents.push_back(std::move(item));
        };
        append(activeDocumentKey, documentPath, Dirty());
        for (const auto &document : inactiveDocuments)
            append(document.recoveryKey, document.path, document.dirty);
        if (!documents.empty())
        {
            DxUi::TreeItemData group;
            group.id = WorkspaceGroupId;
            group.text = Resource(IDS_OPENDOCUMENTS);
            group.hasChildren = true;
            group.expanded = !workspaceCollapsed || !filter.empty();
            const bool expanded = group.expanded;
            filesModel.rows.push_back(std::move(group));
            if (expanded)
                for (auto &document : documents)
                    filesModel.rows.push_back(std::move(document));
        }
    }
    if (!folderSnapshot)
    {
        setFooter(Resource(static_cast<unsigned>(requested.empty()               ? IDS_FILESUNTITLED
                                                 : FAILED(folderAdmissionStatus) ? IDS_FILESUNAVAILABLE
                                                                                 : IDS_FILESLOADING)));
        filesTree->SetEmptyStateText(Resource(static_cast<unsigned>(requested.empty()               ? IDS_FILESUNTITLED
                                                                    : FAILED(folderAdmissionStatus) ? IDS_FILESUNAVAILABLE
                                                                                                    : IDS_FILESLOADING)));
        filesTree->NotifyDataChanged();
        filesTree->SetSelectedItemId(inactiveDocuments.empty() ? std::nullopt
                                                               : std::optional<std::uint64_t>(WorkspaceGroupId + activeDocumentKey + 1));
        return;
    }
    const auto &snapshot = folderSnapshot.value();
    visibleFilePaths.reserve(snapshot.entries.size());
    for (const auto &entry : snapshot.entries)
        visibleFilePaths.push_back(entry.path);
    for (const auto index : FilterMarkdownFiles(snapshot, filter))
    {
        const auto &entry = snapshot.entries[index];
        bool hidden{};
        if (filter.empty())
            for (auto parent = entry.parent; parent != NoFilesParent; parent = snapshot.entries[parent].parent)
                if (collapsedFolders.contains(snapshot.entries[parent].path))
                {
                    hidden = true;
                    break;
                }
        if (hidden)
            continue;
        DxUi::TreeItemData item;
        item.id = index + 1;
        item.text = entry.name;
        item.iconText = entry.directory ? L"\xE8B7" : L"\xE8A5";
        item.tooltipText = entry.path.native();
        item.depth = entry.depth;
        if (entry.parent != NoFilesParent)
            item.parentId = entry.parent + 1;
        item.hasChildren = entry.directory && !entry.reparsePoint && entry.markdownCount > 0;
        item.expanded = item.hasChildren && (!filter.empty() || !collapsedFolders.contains(entry.path));
        if (entry.directory && !item.expanded)
            item.badgeText = std::to_wstring(entry.markdownCount);
        else if ((MatchesFilesPath(snapshot, entry.path, documentPath) && Dirty()) ||
                 std::ranges::any_of(inactiveDocuments,
                                     [&](const auto &document) { return document.dirty && MatchesFilesPath(snapshot, entry.path, document.path); }))
        {
            item.badgeText = Resource(IDS_FILESEDITED);
            item.badgeIsDot = true;
        }
        filesModel.rows.push_back(std::move(item));
    }
    filesTree->SetEmptyStateText(Resource(IDS_NOMATCH));
    filesTree->NotifyDataChanged();
    const auto current = std::find_if(visibleFilePaths.begin(), visibleFilePaths.end(),
                                      [&](const auto &path) { return MatchesFilesPath(snapshot, path, documentPath); });
    if (current != visibleFilePaths.end())
        filesTree->SetSelectedItemId(static_cast<std::uint64_t>(current - visibleFilePaths.begin() + 1));
    else
        filesTree->SetSelectedItemId(inactiveDocuments.empty() ? std::nullopt
                                                               : std::optional<std::uint64_t>(WorkspaceGroupId + activeDocumentKey + 1));
    auto footer = Text(IDS_FILESCOUNT, snapshot.markdownCount);
    const auto append = [&](std::wstring detail) { footer = Text(IDS_FILESFOOTERDETAIL, footer, detail); };
    if (FAILED(snapshot.status) || snapshot.errorCount)
        append(Resource(IDS_FILESACCESSDETAIL));
    if (FAILED(snapshot.watchStatus))
        append(Resource(IDS_FILESWATCHDETAIL));
    if (snapshot.entryLimitReached)
        append(Text(IDS_FILESENTRYLIMIT, snapshot.appliedLimits.maximumEntries));
    if (snapshot.depthLimitReached)
        append(Text(IDS_FILESDEPTHLIMIT, snapshot.appliedLimits.maximumDepth));
    setFooter(std::move(footer));
}
void Application::FolderCompleted()
{
    if (auto result = folderFiles.TakeResult(); result && result->root == folderRoot && result->generation == folderFiles.Generation())
    {
        {
            // Borrow views only while both snapshots are alive. Retain user
            // choices for surviving directories, initialize new rows, and drop
            // disappeared keys so notifications cannot grow collapse state.
            std::set<std::wstring_view> previousDirectories, currentDirectories;
            if (folderSnapshot)
                for (const auto &entry : folderSnapshot->entries)
                    if (entry.directory)
                        previousDirectories.insert(entry.path.native());
            for (const auto &entry : result->entries)
                if (entry.directory)
                {
                    currentDirectories.insert(entry.path.native());
                    const auto relative = documentPath.lexically_relative(entry.path);
                    if (!relative.empty() && *relative.begin() != L"..")
                        collapsedFolders.erase(entry.path);
                    else if (!previousDirectories.contains(entry.path.native()))
                        collapsedFolders.insert(entry.path);
                }
            std::erase_if(collapsedFolders, [&](const auto &path) { return !currentDirectories.contains(path.native()); });
        }
        folderSnapshot = std::move(result);
        RefreshFiles();
    }
}
void Application::EnsureFilesRoot()
{
    if (!settings.filesRoot.empty() || documentPath.empty())
        return;
    std::error_code error;
    const auto absolute = std::filesystem::absolute(documentPath, error);
    if (error)
        return;
    settings.filesRoot = absolute.parent_path().lexically_normal();
    SavePreferences();
}
void Application::SelectFilesRoot(const std::filesystem::path &path)
{
    if (!path.is_absolute() || path.native().find(L'\0') != std::wstring::npos)
        return;
    settings.filesRoot = path.lexically_normal();
    settings.showFiles = true;
    filesFilter->SetText(L"");
    RefreshFiles(true);
    Layout();
    SavePreferences();
    host.SetFocusControl(filesTree, MayRequestNativeFocus());
}
void Application::OpenFolder()
{
    AbandonStartupRestore();
    AskWorkspaceThen(
        [this]
        {
            const auto path = PickFile(false, false, true);
            if (path.empty())
                return;
            ResetWorkspace();
            SetDocument({});
            SelectFilesRoot(path);
        });
}
bool Application::SwitchWorkspaceDocument(std::size_t index)
{
    if (closing || closingAfterSave || sessionEnding || switchingDocument || !root || host.GetRoot() != root)
        return false;
    if (index >= inactiveDocuments.size() || files.Busy() || HasBlockDraft() || unsavedPromptOpen)
    {
        Status(IDS_BUSY);
        return false;
    }
    auto *const primary = editor;
    auto *const owner = root;
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    const auto current = [&]
    { return !closing && !primaryLifetime.expired() && !ownerLifetime.expired() && editor == primary && root == owner && host.GetRoot() == owner; };
    const bool previousDirty = Dirty();
    CheckpointRecovery(true);
    if (!current())
        return false;
    auto &document = inactiveDocuments[index];
    {
        switchingDocument = true;
        const auto restore = wil::scope_exit([&] { switchingDocument = false; });
        if (!primary->ExchangeDocumentState(document.state))
        {
            Status(IDS_BUSY);
            return false;
        }
        ++activity.workspaceSwitches;
        activity.workspaceRetainedPeak = std::max(activity.workspaceRetainedPeak, static_cast<std::uint64_t>(inactiveDocuments.size() + 1));
        if (!current())
            return true; // The exchange committed before a callback retired its owner.
        documentPath.swap(document.path);
        savedText.swap(document.savedText);
        collapsedHeadings.swap(document.collapsedHeadings);
        std::swap(bom, document.bom);
        std::swap(recovered, document.recovered);
        std::swap(stamp, document.stamp);
        std::swap(activeDocumentKey, document.recoveryKey);
        document.dirty = previousDirty;
        dirtyFact.reset();
        primary->SetDocumentFilePath(documentPath.native());
    }
    if (!current())
        return true;
    RememberActiveDocument();
    previewCollapsedHeadings.reset();
    previewCommittedOutline.reset();
    documentAnalysisSource.reset();
    committedAnalysisSource.reset();
    mirroredSource.reset();
    selectionFactsSource.reset();
    findSource.reset();
    SetView(settings.view);
    if (!current())
        return true; // The binding committed before a reentrant teardown.
    Changed();
    if (!current())
        return true;
    RefreshFiles();
    if (current())
        host.SetFocusControl(primary, MayRequestNativeFocus());
    return true;
}
bool Application::StartWorkspaceDocument()
{
    if (documentPath.empty() && !Dirty() && !editor->CanUndo() && !editor->CanRedo())
        return true;
    if (inactiveDocuments.size() + 1 >= RecoveryService::MaximumDocuments)
    {
        Status(IDS_WORKSPACEFULL);
        return false;
    }
    unsigned key{};
    while (key == activeDocumentKey || std::ranges::any_of(inactiveDocuments, [&](const auto &document) { return document.recoveryKey == key; }))
        ++key;
    WorkspaceDocument document;
    document.recoveryKey = key;
    inactiveDocuments.push_back(std::move(document));
    if (SwitchWorkspaceDocument(inactiveDocuments.size() - 1))
        return true;
    inactiveDocuments.pop_back();
    return false;
}
void Application::ResetWorkspace()
{
    for (const auto &document : inactiveDocuments)
        if (FAILED(recovery.SubmitDocument(document.recoveryKey, std::nullopt, true)))
            Status(IDS_RECOVERYFAILED);
    if (FAILED(recovery.SubmitDocument(activeDocumentKey, std::nullopt, true)))
        Status(IDS_RECOVERYFAILED);
    inactiveDocuments.clear();
    workspaceCollapsed = false;
    activeDocumentKey = 0;
    workspaceApprovals.clear();
    openingApprovals.clear();
}
void Application::CheckpointWorkspaceRecovery()
{
    CheckpointRecovery(true);
    for (const auto &document : inactiveDocuments)
    {
        std::optional<RecoverySnapshot> snapshot;
        if (document.dirty)
            snapshot = RecoverySnapshot{document.state.Text(), document.path, document.bom, document.state.GetSelection(), ++recoveryGeneration};
        if (FAILED(recovery.SubmitDocument(document.recoveryKey, std::move(snapshot), true)))
            Status(IDS_RECOVERYFAILED);
    }
}
void Application::OpenPath(const std::filesystem::path &path)
{
    if (!IsMarkdownFile(path) || path == documentPath || (folderSnapshot && MatchesFilesPath(folderSnapshot.value(), path, documentPath)))
        return;
    if (folderSnapshot)
    {
        const auto &entries = folderSnapshot->entries;
        if (const auto item = std::find_if(entries.begin(), entries.end(), [&](const FilesEntry &entry) { return entry.path == path; });
            item != entries.end() && item->directory)
            return;
    }
    AbandonStartupRestore();
    if (files.Busy() || HasBlockDraft() || unsavedPromptOpen || closing || closingAfterSave || switchingDocument || sessionEnding)
    {
        Status(IDS_BUSY);
        return;
    }
    for (std::size_t index = 0; index < inactiveDocuments.size(); ++index)
        if (inactiveDocuments[index].path == path ||
            (folderSnapshot && MatchesFilesPath(folderSnapshot.value(), path, inactiveDocuments[index].path)))
        {
            (void)SwitchWorkspaceDocument(index);
            return;
        }
    openingSnapshot = editor->Snapshot();
    const HRESULT result = files.Submit(false, path);
    openingFromFiles = SUCCEEDED(result);
    Status(FAILED(result) ? IDS_BUSY : IDS_LOADING);
}
void Application::RefreshFind()
{
    if (updatingFind || !findText)
        return;
    if (!findBar->IsVisible())
    {
        if (findPresentationCleared)
            return;

        closedFindMatches = std::move(findMatches);
        findMatches.clear();
        closedFindSource = std::move(findSource);
        findSource.reset();
        closedFindQuery = std::move(findQuery);
        findQuery.clear();
        closedFindCase = findCachedMatchCase;
        findSectionCounts.clear();
        findSectionCountsValid = false;
        findHighlightedCurrent.reset();
        findDisplayedCurrent.reset();
        findDisplayedTotal.reset();
        findDisplayedQuery.clear();
        findHighlightsNeedRefresh = true;
        findBadgesNeedRefresh = true;

        if (findHasHighlights)
        {
            editor->SetSourceHighlights({});
            ++activity.findHighlightUpdates;
            findHasHighlights = false;
        }
        bool badgesChanged{};
        for (auto &row : outlineModel.rows)
            if (!row.badgeText.empty())
            {
                row.badgeText.clear();
                badgesChanged = true;
            }
        if (badgesChanged)
            outlineTree->NotifyDataChanged();
        findBadgeRowsRevision = outlineRowsRevision;
        findPresentationCleared = true;
        return;
    }

    findPresentationCleared = false;
    const auto &needle = findText->GetText();
    const auto snapshot = editor->Snapshot();
    if (closedFindSource)
    {
        findMatches.swap(closedFindMatches);
        closedFindMatches.clear();
        closedFindSource.reset();
        closedFindQuery.clear();
    }
    if (!findSource || !findSource->IsSameRevision(snapshot) || findQuery != needle || findCachedMatchCase != findMatchCase)
    {
        findQuery = needle;
        findSource = snapshot;
        findCachedMatchCase = findMatchCase;
        CollectSourceFindMatches(CurrentText(), needle, findMatchCase, findMatches);
        ++activity.findMatchRebuilds;
        findSectionCountsValid = false;
        findHighlightsNeedRefresh = true;
        findBadgesNeedRefresh = true;
        findHighlightedCurrent.reset();
        findDisplayedCurrent.reset();
        findDisplayedTotal.reset();
        findDisplayedQuery.clear();
    }

    if (!findSectionCountsValid || findSectionOutlineRevision != outlineRowsRevision || findSectionAnalysis != activity.changes)
    {
        const auto &headings = documentInfo.headings;
        const auto &text = CurrentText();
        findSectionCounts.assign(headings.size(), 0);
        std::vector<std::size_t> sectionEnds(headings.size(), text.size());
        std::vector<std::size_t> openSections;
        openSections.reserve(headings.size());
        for (std::size_t i = 0; i < headings.size(); ++i)
        {
            while (!openSections.empty() && headings[openSections.back()].level >= headings[i].level)
            {
                sectionEnds[openSections.back()] = headings[i].offset;
                openSections.pop_back();
            }
            openSections.push_back(i);
        }
        for (std::size_t i = 0; i < headings.size(); ++i)
        {
            const auto lower = std::lower_bound(findMatches.begin(), findMatches.end(), headings[i].offset,
                                                [](const auto &range, std::size_t pos) { return range.first < pos; });
            const auto upper =
                std::lower_bound(lower, findMatches.end(), sectionEnds[i], [](const auto &range, std::size_t pos) { return range.first < pos; });
            findSectionCounts[i] = static_cast<std::size_t>(upper - lower);
        }
        findSectionOutlineRevision = outlineRowsRevision;
        findSectionAnalysis = activity.changes;
        findSectionCountsValid = true;
        findBadgesNeedRefresh = true;
        ++activity.findSectionCountRebuilds;
    }

    const auto selected = editor->GetSelection();
    const auto active = std::lower_bound(findMatches.begin(), findMatches.end(), selected.Start(),
                                         [](const auto &range, std::size_t pos) { return range.first < pos; });
    const bool hasActive = active != findMatches.end() && active->first == selected.Start() && active->last == selected.End();
    const std::size_t current = hasActive ? static_cast<std::size_t>(active - findMatches.begin() + 1) : 0;
    if (findHighlightsNeedRefresh || !findHighlightedCurrent || findHighlightedCurrent.value() != current)
    {
        std::vector<DxUi::RichTextSourceHighlight> highlights;
        const auto count = std::min(findMatches.size(), std::size_t{4096});
        highlights.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto range = findMatches[i];
            highlights.push_back({range.first, range.last - range.first, current == i + 1});
        }
        if (current > count)
            highlights.back() = {active->first, active->last - active->first, true};
        editor->SetSourceHighlights(highlights);
        ++activity.findHighlightUpdates;
        findHasHighlights = !highlights.empty();
        findHighlightedCurrent = current;
        findHighlightsNeedRefresh = false;
    }

    if (!findDisplayedCurrent || findDisplayedCurrent.value() != current || !findDisplayedTotal || findDisplayedTotal.value() != findMatches.size() ||
        findDisplayedQuery != needle)
    {
        const auto countText =
            findMatches.empty() ? Resource(needle.empty() ? IDS_FINDQUERY : IDS_NOMATCH) : Text(IDS_FINDCOUNT, current, findMatches.size());
        if (findCount->GetText() != countText)
            findCount->SetText(countText);
        findDisplayedCurrent = current;
        findDisplayedTotal = findMatches.size();
        findDisplayedQuery = needle;
    }

    if (findBadgesNeedRefresh || findBadgeRowsRevision != outlineRowsRevision)
    {
        bool changed{};
        for (auto &row : outlineModel.rows)
        {
            const auto heading = static_cast<std::size_t>(row.id - 1);
            const auto count = heading < findSectionCounts.size() ? findSectionCounts[heading] : 0;
            const auto badge = count ? std::to_wstring(count) : std::wstring{};
            if (row.badgeText != badge)
            {
                row.badgeText = badge;
                changed = true;
            }
        }
        if (changed)
            outlineTree->NotifyDataChanged();
        findBadgeRowsRevision = outlineRowsRevision;
        findBadgesNeedRefresh = false;
    }
}
void Application::FindNext(bool previous)
{
    const bool visible = findBar->IsVisible();
    if (visible)
        RefreshFind();
    else
    {
        const auto snapshot = editor->Snapshot();
        const auto &query = findText->GetText();
        if (!closedFindSource || !closedFindSource->IsSameRevision(snapshot) || closedFindQuery != query || closedFindCase != findMatchCase)
        {
            CollectSourceFindMatches(CurrentText(), query, findMatchCase, closedFindMatches);
            closedFindSource = snapshot;
            closedFindQuery = query;
            closedFindCase = findMatchCase;
            ++activity.findMatchRebuilds;
        }
    }
    const auto &matches = visible ? findMatches : closedFindMatches;
    if (matches.empty())
        return;
    const auto selection = editor->GetSelection();
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
        const auto next = std::find_if(matches.begin(), matches.end(), [&](const auto &r) { return r.first >= selection.End(); });
        if (next != matches.end())
            index = static_cast<std::size_t>(next - matches.begin());
    }
    const auto range = matches[index];
    editor->SetSelection({range.first, range.last});
    (void)editor->ScrollSourceRangeIntoView(host, range.first, range.last, false);
    SelectionChanged();
}
void Application::ReplaceFind(bool all)
{
    if (HasBlockDraft() || editor->IsReadOnly())
        return;
    RefreshFind();
    if (findMatches.empty())
        return;
    const std::wstring replacement(replaceText->GetText());
    const auto source = editor->Snapshot();
    const auto revision = editor->Revision();
    std::vector<DxUi::RichText::SourceEdit> edits;
    std::size_t size = source.Length();
    const auto selection = editor->GetSelection();
    for (const auto range : findMatches)
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
        updatingFind = true;
        const auto resetUpdatingFind = wil::scope_exit([&] { updatingFind = false; });
        applied = editor->ApplySourceEdits(source, revision, edits, {after, after});
    }
    if (!applied)
        Status(IDS_NOTAVAILABLE);
    RefreshFind();
    if (applied && !all)
        FindNext();
}
void Application::FileMenu(bool overflow)
{
    if (options.headless)
        return;
    std::vector<DxUi::MenuFlyoutItem> items;
    auto add = [&](unsigned label, Command command, std::wstring accelerator = {})
    {
        DxUi::MenuFlyoutItem item;
        item.text = Resource(label);
        item.commandId = static_cast<int>(command) + 1;
        item.acceleratorText = std::move(accelerator);
        items.push_back(std::move(item));
    };
    if (!overflow)
    {
        add(IDS_NEW, Command::New, L"Ctrl+N");
        add(IDS_OPEN, Command::Open, L"Ctrl+O");
        add(IDS_OPENFOLDER, Command::OpenFolder, L"Ctrl+Shift+O");
        add(IDS_SAVEAS, Command::SaveAs, L"Ctrl+Shift+S");
    }
    else
        for (const auto &[label, command] : std::array<std::pair<unsigned, Command>, 11>{{{IDS_HEADING, Command::Heading},
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
    const auto bounds = (overflow ? overflowButton : fileMenu)->GetBounds();
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(
        window.get(), host.DipPointToScreenPoint({bounds.left, bounds.bottom}), items, host.GetTheme(),
        [this](std::optional<int> selected) noexcept { OnMenuClosed(selected); }, callbacks);
}
bool Application::DismissSurface()
{
    if (pendingSelectionTools || selectionToolsFocus || drawer->IsVisible() || findBar->IsVisible() || floating->IsVisible() ||
        blockDraft->IsVisible() || recoveryPanel->IsVisible())
    {
        CloseDrawers();
        return true;
    }
    if (editor->GetSemanticSourceRegion())
    {
        if (!editor->EndSemanticSourceEdit(host))
            return false;
        SelectionChanged();
        return true;
    }
    if (settings.focusMode)
    {
        Execute(Command::Focus);
        return true;
    }
    if (settings.showFiles)
    {
        if (!filesFilter->GetText().empty())
        {
            filesFilter->SetText(L"");
            RefreshFiles();
        }
        else
        {
            settings.showFiles = false;
            RefreshFiles(true);
            Layout();
            SavePreferences();
            host.SetFocusControl(editor, MayRequestNativeFocus());
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
    const int border =
        GetSystemMetricsForDpi(SM_CXSIZEFRAME, GetDpiForWindow(hwnd)) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, GetDpiForWindow(hwnd));
    if (!IsZoomed(hwnd))
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
    const float scale = 96.f / host.GetDpi();
    const D2D1_POINT_2F dip{static_cast<float>(screen.x) * scale, static_cast<float>(screen.y) * scale};
    const auto contains = [&](const DxUi::Control *c)
    {
        const auto r = c->GetBounds();
        return c->IsVisible() && dip.x >= r.left && dip.x < r.right && dip.y >= r.top && dip.y < r.bottom;
    };
    if (contains(closeButton))
        return HTCLOSE;
    if (contains(maximizeButton))
        return HTMAXBUTTON;
    if (contains(minimizeButton))
        return HTMINBUTTON;
    if (dip.y < (settings.focusMode ? 40.f : 56.f))
    {
        if (contains(fileMenu) || contains(saveButton) || contains(exitFocus))
            return HTCLIENT;
        for (const auto *mode : modeButtons)
            if (contains(mode))
                return HTCLIENT;
        return HTCAPTION;
    }
    return HTCLIENT;
}
} // namespace RedTilde
