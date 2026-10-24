#include "Application.h"
#include "res/Resource.h"
#include <algorithm>
#include <format>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
template <class... Args> std::wstring Text(unsigned id, Args &&...args)
{
    return std::vformat(Resource(id), std::make_wformat_args(args...));
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
    const auto requested = settings.showFiles ? documentPath.parent_path() : std::filesystem::path{};
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
    filesTitle->SetText(requested.empty() ? Resource(IDS_FILES) : requested.filename().empty() ? requested.native() : requested.filename().native());
    filesTitle->SetTooltipText(requested.native());
    filesTitle->SetAccessibleHelpText(requested.native());
    lastFilesDirty = Dirty();
    filesModel.rows.clear();
    visibleFilePaths.clear();
    if (!folderSnapshot)
    {
        setFooter(Resource(requested.empty() ? IDS_FILESUNTITLED : FAILED(folderAdmissionStatus) ? IDS_FILESUNAVAILABLE : IDS_FILESLOADING));
        filesTree->SetEmptyStateText(Resource(requested.empty()               ? IDS_FILESUNTITLED
                                              : FAILED(folderAdmissionStatus) ? IDS_FILESUNAVAILABLE
                                                                              : IDS_FILESLOADING));
        filesTree->NotifyDataChanged();
        return;
    }
    const auto &snapshot = folderSnapshot.value();
    visibleFilePaths.reserve(snapshot.entries.size());
    for (const auto &entry : snapshot.entries)
        visibleFilePaths.push_back(entry.path);
    const std::wstring filter(filesFilter->GetText());
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
        else if (MatchesFilesPath(snapshot, entry.path, documentPath) && Dirty())
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
    AskThen(
        [this, path]
        {
            openingSnapshot = editor->Snapshot();
            const HRESULT result = files.Submit(false, path);
            Status(FAILED(result) ? IDS_BUSY : IDS_LOADING);
        });
}
void Application::RefreshFind()
{
    if (updatingFind || !findText)
        return;
    if (!findBar->IsVisible())
    {
        if (findPresentationCleared)
            return;

        findMatches.clear();
        findSource.reset();
        findQuery.clear();
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
    if (!findSource || !findSource->IsSameRevision(snapshot) || findQuery != needle)
    {
        findMatches.clear();
        findQuery = needle;
        findSource = snapshot;
        const auto &text = CurrentText();
        if (!needle.empty())
            for (auto pos = text.find(needle); pos != text.npos; pos = text.find(needle, pos + needle.size()))
                if (DxUi::RichText::IsEditBoundary(text, pos) && DxUi::RichText::IsEditBoundary(text, pos + needle.size()))
                    findMatches.push_back({pos, pos + needle.size()});
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
    RefreshFind();
    if (findMatches.empty())
        return;
    const auto selection = editor->GetSelection();
    std::size_t index{};
    if (previous)
    {
        index = findMatches.size() - 1;
        for (std::size_t i = findMatches.size(); i > 0; --i)
            if (findMatches[i - 1].first < selection.Start())
            {
                index = i - 1;
                break;
            }
    }
    else
    {
        const auto next = std::find_if(findMatches.begin(), findMatches.end(), [&](const auto &r) { return r.first >= selection.End(); });
        if (next != findMatches.end())
            index = static_cast<std::size_t>(next - findMatches.begin());
    }
    const auto range = findMatches[index];
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
    updatingFind = true;
    const bool applied = editor->ApplySourceEdits(source, revision, edits, {after, after});
    updatingFind = false;
    if (!applied)
        Status(IDS_NOTAVAILABLE);
    RefreshFind();
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
        [this](std::optional<int> selected)
        {
            if (closing)
                return;
            if (selected)
                Execute(static_cast<Command>(selected.value() - 1));
            if (!HasBlockDraft() && !drawer->IsVisible() && !findBar->IsVisible())
                host.SetFocusControl(editor, !options.headless);
        },
        callbacks);
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
        return editor->EndSemanticSourceEdit(host);
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
            host.SetFocusControl(editor, !options.headless);
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
    const float scale = 96.f / GetDpiForWindow(hwnd);
    const D2D1_POINT_2F dip{screen.x * scale, screen.y * scale};
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
