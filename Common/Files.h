#pragma once
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace RedTilde
{
inline constexpr std::size_t NoFilesParent = std::numeric_limits<std::size_t>::max();
struct FilesLimits
{
    unsigned maximumDepth = 8;
    // Counts directory entries examined, including non-Markdown/hidden entries.
    // This bounds work even in a tree containing only empty folders or binaries.
    std::size_t maximumEntries = 5000;
};
struct FilesEntry
{
    std::filesystem::path path;
    std::wstring name;
    std::size_t parent = NoFilesParent;
    std::size_t markdownCount{};
    unsigned depth{};
    bool directory{};
    bool reparsePoint{};
};
struct FilesError
{
    std::filesystem::path path;
    HRESULT status = S_OK;
};
enum class FilesCaseSensitivity
{
    Unknown,
    Insensitive,
    Sensitive
};
struct FilesSnapshot
{
    std::filesystem::path root;
    std::uint64_t generation{};
    std::uint64_t revision{};
    // Preorder, directories first and naturally ordered within each parent.
    // Every parent precedes its children; root itself is represented by root.
    std::vector<FilesEntry> entries;
    std::vector<FilesError> errors;
    std::size_t markdownCount{};
    std::size_t examinedEntries{};
    std::size_t errorCount{};
    HRESULT status = S_OK;
    HRESULT watchStatus = S_OK;
    FilesLimits appliedLimits; // Preserve this scan's actual bounds, including empty/failed results.
    FilesCaseSensitivity rootCaseSensitivity = FilesCaseSensitivity::Unknown;
    bool entryLimitReached{};
    bool depthLimitReached{};
    bool cancelled{};
};
[[nodiscard]] bool IsMarkdownFile(const std::filesystem::path &path) noexcept;
[[nodiscard]] bool FilesNaturalLess(std::wstring_view left, std::wstring_view right) noexcept;
// Exact paths always match. Only confirmed case-insensitive root metadata
// permits case folding of a direct child's filename. Unknown/sensitive roots
// and nested path components stay exact; this never performs UI-thread I/O.
[[nodiscard]] bool MatchesFilesPath(const FilesSnapshot &snapshot, const std::filesystem::path &left, const std::filesystem::path &right) noexcept;
// No controls, window access or writes. Reparse directories are visible leaves;
// directory handles are opened without following reparse points and remain pinned
// during descent. Errors and incomplete limits remain explicit in the result.
[[nodiscard]] FilesSnapshot EnumerateMarkdownFiles(const std::filesystem::path &root, FilesLimits limits = {}, std::stop_token stop = {});
// Returns stable indexes into snapshot, including each matching file's ancestors.
// The filter matches file names, case-insensitively; an empty filter keeps all rows.
[[nodiscard]] std::vector<std::size_t> FilterMarkdownFiles(const FilesSnapshot &snapshot, std::wstring_view filter);

// One retained event-driven worker and one latest root/result. A new request
// cancels the old scan; publication and consumption reject its generation/revision.
// Close invalidates HWND posting and asks the worker to drain its own directory
// notification I/O. No UI-thread join, pointer message payload or polling timer.
class FilesService final
{
  public:
    FilesService() = default;
    ~FilesService();
    FilesService(const FilesService &) = delete;
    FilesService &operator=(const FilesService &) = delete;
    [[nodiscard]] HRESULT Initialize(HWND window = nullptr, UINT message = 0, FilesLimits limits = {}) noexcept;
    [[nodiscard]] HRESULT SetRoot(std::filesystem::path root) noexcept;
    [[nodiscard]] HRESULT Refresh() noexcept;
    [[nodiscard]] std::optional<FilesSnapshot> TakeResult();
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    [[nodiscard]] std::uint64_t Generation() const noexcept;
    void Close() noexcept;

  private:
    struct State;
    std::shared_ptr<State> state;
};

struct FileChangeNotice
{
    std::uint64_t generation{}, revision{};
    HRESULT status = S_OK;
};
// One event-driven worker, at most 32 local image files in eight directories.
// Notifications are coalesced and source-tagged; Close never waits on UI.
class FileChangeService final
{
  public:
    FileChangeService() = default;
    ~FileChangeService();
    FileChangeService(const FileChangeService &) = delete;
    FileChangeService &operator=(const FileChangeService &) = delete;
    [[nodiscard]] HRESULT Initialize(HWND window, UINT message, WPARAM cookie) noexcept;
    [[nodiscard]] HRESULT SetFiles(std::span<const std::filesystem::path> files) noexcept;
    [[nodiscard]] std::optional<FileChangeNotice> TakeResult();
    void Close() noexcept;

  private:
    struct State;
    std::shared_ptr<State> state;
};
} // namespace RedTilde
