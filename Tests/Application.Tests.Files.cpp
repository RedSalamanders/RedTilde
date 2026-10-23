#include "Application.Tests.Files.h"
#include "Common/DocumentInfo.h"
#include "Common/Files.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include "Support/Support.Tests.FilesFixtures.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view message)
{
    if (!condition)
        throw std::runtime_error(std::string(message));
}
void Success(HRESULT result, std::string_view message)
{
    Require(SUCCEEDED(result), message);
}
void WriteFixture(const std::filesystem::path &path)
{
    std::filesystem::create_directories(path.parent_path());
    Success(WriteBytesAtomic(path, "# Invented fixture\n"), "seed Files fixture");
}
FilesSnapshot AwaitSnapshot(FilesService &service, const std::function<bool(const FilesSnapshot &)> &expected)
{
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < end)
    {
        auto snapshot = service.TakeResult();
        if (snapshot && expected(snapshot.value()))
            return std::move(snapshot.value());
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    throw std::runtime_error("Files service did not publish the expected latest snapshot");
}
} // namespace

void RegisterFilesTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.DocumentPositionIndexAndNewlineFacts",
         []
         {
             const auto empty = AnalyzeDocument(L"");
             Require(empty.lines == 1 && empty.lineIndex == std::vector<DocumentLine>{{0, 0}} && empty.newlineKind == DocumentNewlineKind::None &&
                         DocumentPositionAt(empty, 0) == DocumentPosition{1, 1, 0, 0} &&
                         DocumentPositionAt(empty, std::numeric_limits<std::size_t>::max()) == DocumentPosition{1, 1, 0, 0},
                     "an empty revision retains one empty line and clamps oversized source ACPs");
             const auto mixed = AnalyzeDocument(L"A😀\r\nB\n\rC\r\n");
             const std::vector<DocumentLine> lines{{0, 3}, {5, 1}, {7, 0}, {8, 1}, {11, 0}};
             const std::array<DocumentPosition, 12> positions{{{1, 1, 0, 3},
                                                               {1, 2, 0, 3},
                                                               {1, 3, 0, 3},
                                                               {1, 4, 0, 3},
                                                               {1, 4, 0, 3},
                                                               {2, 1, 5, 1},
                                                               {2, 2, 5, 1},
                                                               {3, 1, 7, 0},
                                                               {4, 1, 8, 1},
                                                               {4, 2, 8, 1},
                                                               {4, 2, 8, 1},
                                                               {5, 1, 11, 0}}};
             Require(mixed.characters == 11 && mixed.lines == lines.size() && mixed.lineIndex == lines &&
                         mixed.newlineKind == DocumentNewlineKind::Mixed,
                     "the revision-bound index preserves UTF-16 lengths, all three newline kinds, blank and final lines");
             for (std::size_t caret = 0; caret < positions.size(); ++caret)
                 Require(DocumentPositionAt(mixed, caret) == positions[caret], "every mixed-EOL ACP matches independent line/column/count oracles");
             Require(DocumentPositionAt(mixed, std::numeric_limits<std::size_t>::max()) == positions.back(),
                     "an oversized ACP cannot overflow the terminal physical-line query");
             for (const auto &[text, kind] : std::array<std::pair<std::wstring_view, DocumentNewlineKind>, 4>{{{L"word", DocumentNewlineKind::None},
                                                                                                               {L"\n", DocumentNewlineKind::Lf},
                                                                                                               {L"\r", DocumentNewlineKind::Cr},
                                                                                                               {L"\r\n", DocumentNewlineKind::CrLf}}})
             {
                 const auto info = AnalyzeDocument(text);
                 Require(info.newlineKind == kind && info.lines == info.lineIndex.size(), "pure newline kinds remain distinct and line facts agree");
                 Require(DocumentPositionAt(info, text.size()).line == (kind == DocumentNewlineKind::None ? 1u : 2u),
                         "each complete final terminator exposes an empty final physical line");
             }
             const auto crlf = AnalyzeDocument(L"a\r\nb\r\n");
             Require(crlf.lineIndex == std::vector<DocumentLine>{{0, 1}, {3, 1}, {6, 0}} &&
                         DocumentPositionAt(crlf, 1) == DocumentPosition{1, 2, 0, 1} && DocumentPositionAt(crlf, 2) == DocumentPosition{1, 2, 0, 1} &&
                         DocumentPositionAt(crlf, 3) == DocumentPosition{2, 1, 3, 1},
                     "CRLF is one indivisible terminator, never a third visible column or an early next-line start");
             std::wstring maximumLines(65536, L'\n');
             const auto large = AnalyzeDocument(maximumLines);
             Require(large.lines == 65537 && large.lineIndex.size() == 65537 && large.newlineKind == DocumentNewlineKind::Lf &&
                         DocumentPositionAt(large, 32768) == DocumentPosition{32769, 1, 32768, 0} &&
                         DocumentPositionAt(large, 65536) == DocumentPosition{65537, 1, 65536, 0},
                     "the admitted worst-case line count has exact bounded index/position facts");
             std::wstring headings;
             for (unsigned index = 0; index < 140; ++index)
                 headings += L"# Heading\n";
             const auto capped = AnalyzeDocument(headings);
             Require(capped.headings.size() == 128 && capped.lines == 141 && capped.lineIndex.size() == 141 &&
                         DocumentPositionAt(capped, headings.size()) == DocumentPosition{141, 1, headings.size(), 0} &&
                         DocumentPositionAt(mixed, 5) == positions[5],
                     "new revision facts retain the 128-heading bound and never mutate an older analyzed revision");
         });
    test("Files.MarkdownTreeNaturalOrderAndFilter",
         [&]
         {
             const auto directory = root / L"files-tree";
             for (const auto name : {L"Archive2/draft10.MarkDown", L"Archive2/draft2.md", L"Archive10/nested/one.md", L"Empty/readme.txt",
                                     L"report10.md", L"report2.MD", L"ordinary.txt", L"hidden.md", L"system.markdown", L"Secret/omitted.md"})
                 WriteFixture(directory / name);
             Require(SetFileAttributesW((directory / L"hidden.md").c_str(), FILE_ATTRIBUTE_HIDDEN) != FALSE, "hide fixture file");
             Require(SetFileAttributesW((directory / L"system.markdown").c_str(), FILE_ATTRIBUTE_SYSTEM) != FALSE, "mark fixture system file");
             Require(SetFileAttributesW((directory / L"Secret").c_str(), FILE_ATTRIBUTE_HIDDEN) != FALSE, "hide fixture folder");
             const auto snapshot = EnumerateMarkdownFiles(directory);
             Success(snapshot.status, "enumerate fixture");
             const std::array<std::wstring_view, 8> names{L"Archive2", L"draft2.md", L"draft10.MarkDown", L"Archive10",
                                                          L"nested",   L"one.md",    L"report2.MD",       L"report10.md"};
             Require(snapshot.entries.size() == names.size() && snapshot.markdownCount == 5, "only visible Markdown and populated ancestors remain");
             for (std::size_t index = 0; index < names.size(); ++index)
                 Require(snapshot.entries[index].name == names[index], "directory-first natural preorder matches independent names");
             Require(snapshot.entries[0].markdownCount == 2 && snapshot.entries[3].markdownCount == 1 && snapshot.entries[5].parent == 4 &&
                         snapshot.entries[4].parent == 3 && snapshot.entries[0].parent == NoFilesParent,
                     "counts and hierarchical parent indexes retained");
             Require(FilterMarkdownFiles(snapshot, L"DRAFT2") == std::vector<std::size_t>{0, 1}, "case-insensitive name filter retains ancestors");
             Require(FilterMarkdownFiles(snapshot, L"one") == std::vector<std::size_t>{3, 4, 5}, "deep filter keeps the complete ancestor chain");
             Require(FilterMarkdownFiles(snapshot, L"ordinary").empty() && FilterMarkdownFiles(snapshot, L"").size() == names.size(),
                     "non-Markdown absent and empty filter restores tree");
             Require(IsMarkdownFile(L"note.MD") && IsMarkdownFile(L"note.MARKDOWN") && !IsMarkdownFile(L"note.md.bak"), "extensions are exact");
             Require(FilesNaturalLess(L"note2.md", L"note10.md") && FilesNaturalLess(L"note2.md", L"note02.md") &&
                         FilesNaturalLess(L"note99999999999999999999.md", L"note100000000000000000000.md"),
                     "natural comparison handles numeric order, leading zeroes and values beyond integer capacity");
             wil::unique_handle rootHandle(CreateFileW(directory.c_str(), FILE_READ_ATTRIBUTES,
                                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
             Require(static_cast<bool>(rootHandle), "read the fixture's actual per-directory case policy independently");
             FILE_CASE_SENSITIVE_INFO policy{};
             if (GetFileInformationByHandleEx(rootHandle.get(), FileCaseSensitiveInfo, &policy, sizeof(policy)))
                 Require(snapshot.rootCaseSensitivity ==
                             ((policy.Flags & FILE_CS_FLAG_CASE_SENSITIVE_DIR) ? FilesCaseSensitivity::Sensitive : FilesCaseSensitivity::Insensitive),
                         "the worker records the observed root lookup semantics rather than assuming ordinary NTFS case behavior");
             FilesSnapshot model;
             model.root = directory;
             model.rootCaseSensitivity = FilesCaseSensitivity::Insensitive;
             Require(MatchesFilesPath(model, directory / L"report2.MD", directory / L"REPORT2.md") &&
                         !MatchesFilesPath(model, directory / L"report2.MD", directory / L"report10.md") &&
                         !MatchesFilesPath(model, directory / L"Archive2/draft2.md", directory / L"archive2/DRAFT2.md"),
                     "only a confirmed insensitive direct-child filename may match a differently cased current-document alias");
             model.rootCaseSensitivity = FilesCaseSensitivity::Sensitive;
             Require(!MatchesFilesPath(model, directory / L"A.md", directory / L"a.md") &&
                         MatchesFilesPath(model, directory / L"A.md", directory / L"A.md"),
                     "distinct A.md/a.md files in a sensitive root never alias each other");
             model.rootCaseSensitivity = FilesCaseSensitivity::Unknown;
             Require(!MatchesFilesPath(model, directory / L"A.md", directory / L"a.md"),
                     "unavailable case metadata preserves exact names rather than guessing a destructive alias");
         });
    test("Files.BoundedTraversalCancellationAndErrors",
         [&]
         {
             const auto directory = root / L"files-limits";
             for (unsigned index = 0; index < 6; ++index)
                 WriteFixture(directory / std::format(L"note{}.md", index));
             const auto limited = EnumerateMarkdownFiles(directory, {8, 3});
             Require(limited.entryLimitReached && limited.examinedEntries == 3 && limited.markdownCount == 3 && limited.entries.size() == 3,
                     "entry ceiling reports exactly the examined/returned subset");
             Require(limited.appliedLimits.maximumDepth == 8 && limited.appliedLimits.maximumEntries == 3,
                     "entry-limited snapshots retain their actual admitted limits for truthful presentation");
             const auto deep = root / L"files-depth";
             WriteFixture(deep / L"Level1/Level2/beyond.md");
             WriteFixture(deep / L"visible.md");
             const auto depth = EnumerateMarkdownFiles(deep, {0, 5000});
             Require(depth.depthLimitReached && depth.markdownCount == 1 && depth.entries.size() == 1 && depth.entries[0].name == L"visible.md",
                     "depth ceiling preserves root files and reports unvisited branches");
             Require(depth.appliedLimits.maximumDepth == 0 && depth.appliedLimits.maximumEntries == 5000,
                     "depth-limited snapshots retain the caller's actual depth ceiling");
             std::stop_source stop;
             stop.request_stop();
             const auto cancelled = EnumerateMarkdownFiles(directory, {}, stop.get_token());
             Require(cancelled.cancelled && cancelled.status == HRESULT_FROM_WIN32(ERROR_CANCELLED) && cancelled.examinedEntries == 0,
                     "pre-cancelled work does no enumeration");
             const auto missing = EnumerateMarkdownFiles(root / L"does-not-exist", {2, 7});
             Require(FAILED(missing.status) && missing.errorCount == 1 && missing.errors.size() == 1 && missing.entries.empty(),
                     "missing/offline roots retain a visible failure instead of a successful empty tree");
             const auto empty = EnumerateMarkdownFiles({}, {1, 4});
             Require(missing.appliedLimits.maximumDepth == 2 && missing.appliedLimits.maximumEntries == 7 && SUCCEEDED(empty.status) &&
                         empty.entries.empty() && empty.appliedLimits.maximumDepth == 1 && empty.appliedLimits.maximumEntries == 4,
                     "failed and empty enumeration results still retain their own actual request limits");
             Require(EnumerateMarkdownFiles(directory, {8, 0}).status == E_INVALIDARG &&
                         EnumerateMarkdownFiles(directory, {9, 5000}).status == E_INVALIDARG &&
                         EnumerateMarkdownFiles(L"relative").status == E_INVALIDARG,
                     "invalid resource limits and relative roots fail closed");
             const auto binaries = root / L"files-non-markdown-limit";
             for (unsigned index = 0; index < 5; ++index)
                 WriteFixture(binaries / std::format(L"binary{}.bin", index));
             const auto noMatches = EnumerateMarkdownFiles(binaries, {8, 2});
             Require(noMatches.entryLimitReached && noMatches.examinedEntries == 2 && noMatches.markdownCount == 0 && noMatches.entries.empty(),
                     "non-Markdown entries consume the bounded work budget and never claim 5000 returned files");
         });
    test("Files.ReparseLeavesNeverTraverse",
         [&]
         {
             const auto directory = root / L"files-reparse";
             const auto source = directory / L"source", target = directory / L"target", link = source / L"Linked notes";
             WriteFixture(source / L"local.md");
             WriteFixture(target / L"never-traverse.md");
             Success(Testing::CreateFixtureJunction(directory, link, target), "create owned fixture junction");
             const auto removeLink = wil::scope_exit(
                 [&]
                 {
                     std::error_code error;
                     (void)std::filesystem::remove(link, error); // Remove only this leaf; never walk the target.
                 });
             Require(Testing::CreateFixtureJunction(directory, source / L"outside", root / L"unrelated-target") == E_INVALIDARG,
                     "shared fixture creator rejects a target outside this case's owned fixture");
             const auto snapshot = EnumerateMarkdownFiles(source);
             Require(SUCCEEDED(snapshot.status) && snapshot.entries.size() == 2 && snapshot.markdownCount == 1 && snapshot.entries[0].directory &&
                         snapshot.entries[0].reparsePoint && snapshot.entries[0].markdownCount == 0 && snapshot.entries[1].name == L"local.md" &&
                         FilterMarkdownFiles(snapshot, L"never-traverse").empty(),
                     "a reparse directory is a visible unexpanded leaf, not a route to target Markdown");
             const auto reparseRoot = EnumerateMarkdownFiles(link);
             Require(FAILED(reparseRoot.status) && reparseRoot.entries.empty(), "an explicitly selected reparse root is rejected without traversal");
             const auto trailingRoot = EnumerateMarkdownFiles(link / L"");
             Require(FAILED(trailingRoot.status) && trailingRoot.entries.empty(), "a trailing separator cannot bypass reparse-root rejection");
         });
    test("Files.AsyncRerootChangeNotificationsAndClose",
         [&]
         {
             const auto oldRoot = root / L"files-old", latestRoot = root / L"files-latest";
             for (unsigned index = 0; index < 50; ++index)
                 WriteFixture(oldRoot / std::format(L"old{}.md", index));
             WriteFixture(latestRoot / L"latest.md");
             FilesService service;
             Success(service.Initialize(), "initialize one event-driven Files worker");
             Success(service.SetRoot(oldRoot), "admit old root");
             Success(service.SetRoot(latestRoot), "supersede old root");
             const auto generation = service.Generation();
             const auto first = AwaitSnapshot(service, [&](const auto &value) { return value.generation == generation; });
             Require(first.root == latestRoot && first.markdownCount == 1 && first.entries[0].name == L"latest.md" && SUCCEEDED(first.watchStatus) &&
                         SUCCEEDED(first.status),
                     "only the latest root publishes and its recursive notification is armed");
             WriteFixture(latestRoot / L"nested/new.MARKDOWN");
             const auto changed =
                 AwaitSnapshot(service, [&](const auto &value) { return value.revision > first.revision && value.markdownCount == 2; });
             Require(changed.generation == generation && changed.root == latestRoot && changed.entries[0].directory,
                     "a nested change refreshes from directory notifications without an explicit refresh or polling timer");
             Require(std::filesystem::remove(latestRoot / L"nested/new.MARKDOWN"), "remove notification fixture");
             const auto removed =
                 AwaitSnapshot(service, [&](const auto &value) { return value.revision > changed.revision && value.markdownCount == 1; });
             Require(removed.entries.size() == 1 && removed.entries[0].name == L"latest.md", "notification refresh prunes a newly empty folder");
             for (unsigned index = 0; index < 50; ++index)
                 Success(service.Refresh(), "coalesce latest refresh request");
             Success(service.SetRoot({}), "clear root for an untitled document");
             const auto untitledGeneration = service.Generation();
             const auto untitled = AwaitSnapshot(service, [&](const auto &value) { return value.generation == untitledGeneration; });
             Require(untitled.root.empty() && untitled.entries.empty() && SUCCEEDED(untitled.status),
                     "untitled root cancels scans and clears the model");
             service.Close();
             Require(!service.Busy() && !service.TakeResult() && service.SetRoot(latestRoot) == E_UNEXPECTED,
                     "Close detaches immediately and rejects future admission without joining directory I/O");
         });
    test("Files.CoalescedNotificationAndStaleResult",
         [&]
         {
             constexpr UINT message = WM_APP + 315;
             wil::unique_hwnd window(CreateWindowExW(0, L"STATIC", L"Files test notification sink", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                                     GetModuleHandleW(nullptr), nullptr));
             Require(static_cast<bool>(window), "create hidden message-only notification sink");
             const auto firstRoot = root / L"files-notify-first", secondRoot = root / L"files-notify-second";
             WriteFixture(firstRoot / L"first.md");
             WriteFixture(secondRoot / L"second.md");
             FilesService service;
             Success(service.Initialize(window.get(), message), "bind Files notifications to an owned hidden window");
             Success(service.SetRoot(firstRoot), "admit first notification root");
             const auto waitIdle = [&]
             {
                 const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
                 while (service.Busy() && std::chrono::steady_clock::now() < end)
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 Require(!service.Busy(), "Files worker completes its bounded pending request");
             };
             waitIdle();
             // Leave the first message and result unconsumed while replacing the
             // root and coalescing a burst. One posted wakeup must cover the latest
             // state, rather than one message per request or an old path result.
             Success(service.SetRoot(secondRoot), "replace a completed but unconsumed root");
             for (unsigned index = 0; index < 50; ++index)
                 Success(service.Refresh(), "coalesce notification refresh burst");
             waitIdle();
             MSG posted{};
             unsigned messages{};
             while (PeekMessageW(&posted, window.get(), message, message, PM_REMOVE))
                 ++messages;
             Require(messages == 1, "only one pending notification exists across roots and refresh requests");
             const auto latest = service.TakeResult();
             Require(latest && latest->root == secondRoot && latest->generation == service.Generation() && latest->entries.size() == 1 &&
                         latest->entries[0].name == L"second.md",
                     "the queued wakeup drains only the latest matching result");
             service.Close(); // Invalidate the HWND binding before its owner resets it.
             window.reset();
             Require(!service.TakeResult(), "a detached service never exposes a late result after the UI dies");
         });
    test("Settings.DesignRefreshPreferencesRoundTrip",
         [&]
         {
             Settings value;
             value.showFiles = true;
             value.filesWidth = 340;
             value.readingWidth = 920;
             value.textScale = 125;
             value.dimInactiveBlocks = false;
             value.allowRemoteImages = true;
             value.focusMode = true;
             const auto parsed = ParseSettings(SerializeSettings(value));
             Require(parsed.showFiles && parsed.filesWidth == 340 && parsed.readingWidth == 920 && parsed.textScale == 125 &&
                         !parsed.dimInactiveBlocks && parsed.allowRemoteImages && !parsed.focusMode,
                     "design preferences round trip while focus mode remains session-local");
             const auto directory = root / L"design-preferences";
             Success(SaveSettings(directory, value), "persist design preferences atomically");
             Settings loaded;
             Success(LoadSettings(directory, loaded), "reload design preferences");
             Require(loaded.showFiles && loaded.filesWidth == 340 && loaded.readingWidth == 920 && loaded.textScale == 125 &&
                         !loaded.dimInactiveBlocks && loaded.allowRemoteImages,
                     "atomic settings storage retains every design preference");
         });
    test("Settings.DesignRefreshFieldFallback",
         []
         {
             const auto invalid = ParseSettings(
                 "RedTilde.Settings/1\nfiles=2\nfilesWidth=199\nreadingWidth=1041\nscale=201\nfocusDimming=9\nremoteImages=2\noutline=0\n");
             Require(!invalid.showFiles && invalid.filesWidth == 280 && invalid.readingWidth == 760 && invalid.textScale == 100 &&
                         invalid.dimInactiveBlocks && !invalid.allowRemoteImages && !invalid.showOutline,
                     "invalid fields fall back individually without discarding valid older fields");
             const auto lower = ParseSettings("RedTilde.Settings/1\nfiles=1\nfilesWidth=200\nreadingWidth=480\nscale=75\nfocusDimming=0\n");
             const auto upper = ParseSettings("RedTilde.Settings/1\nfilesWidth=400\nreadingWidth=1040\nscale=200\n");
             Require(lower.showFiles && lower.filesWidth == 200 && lower.readingWidth == 480 && lower.textScale == 75 && !lower.dimInactiveBlocks &&
                         upper.filesWidth == 400 && upper.readingWidth == 1040 && upper.textScale == 200,
                     "all documented inclusive preference boundaries are accepted");
             const auto malformed = ParseSettings("RedTilde.Settings/1\nfilesWidth=-1\nreadingWidth= 700\nscale=4294967296\nfocusDimming=1x\n");
             Require(malformed.filesWidth == 280 && malformed.readingWidth == 760 && malformed.textScale == 100 && malformed.dimInactiveBlocks,
                     "negative, whitespace, overflow and trailing-junk numbers cannot enter live preferences");
             const auto old = ParseSettings("RedTilde.Settings/1\ntheme=3\nview=2\noutline=0\n");
             Require(old.appearance == Appearance::Paper && old.view == EditorView::Split && !old.showOutline && !old.showFiles &&
                         old.filesWidth == 280 && old.readingWidth == 760 && old.textScale == 100 && old.dimInactiveBlocks && !old.allowRemoteImages,
                     "existing v1 preference files retain compatible defaults for new fields");
         });
}
} // namespace RedTilde
