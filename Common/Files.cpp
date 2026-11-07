#include "Files.h"
#include "ProcessWork.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <mutex>
#include <new>
#include <utility>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
bool ValidLimits(FilesLimits limits) noexcept
{
    return limits.maximumDepth <= 8 && limits.maximumEntries > 0 && limits.maximumEntries <= 5000;
}
bool EqualOrdinal(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() && (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
                                                                                static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}
bool Digit(wchar_t value) noexcept
{
    return value >= L'0' && value <= L'9';
}
bool ContainsOrdinal(std::wstring_view text, std::wstring_view part) noexcept
{
    if (part.size() > text.size())
        return false;
    for (std::size_t offset = 0; offset <= text.size() - part.size(); ++offset)
        if (EqualOrdinal(text.substr(offset, part.size()), part))
            return true;
    return false;
}
wil::unique_handle OpenDirectory(const std::filesystem::path &path, bool watch = false) noexcept
{
    // The scan pins parent names and denies metadata writes until their children
    // are visited. A checked parent cannot be renamed or converted to a reparse
    // point to redirect descent. Watch handles permit ordinary external changes.
    return wil::unique_handle(CreateFileW(path.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                          FILE_SHARE_READ | (watch ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0u), nullptr, OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | (watch ? FILE_FLAG_OVERLAPPED : 0u), nullptr));
}
FilesCaseSensitivity DirectoryCaseSensitivity(HANDLE directory) noexcept
{
    FILE_CASE_SENSITIVE_INFO information{};
    if (GetFileInformationByHandleEx(directory, FileCaseSensitiveInfo, &information, sizeof(information)))
        return (information.Flags & FILE_CS_FLAG_CASE_SENSITIVE_DIR) ? FilesCaseSensitivity::Sensitive : FilesCaseSensitivity::Insensitive;
    DWORD flags{};
    // Filesystems without case-sensitive names cannot expose per-directory
    // sensitive lookup. A failed/unsupported metadata query otherwise stays
    // Unknown; guessing insensitive could alias two distinct NTFS files.
    if (GetVolumeInformationByHandleW(directory, nullptr, 0, nullptr, nullptr, &flags, nullptr, 0) && !(flags & FILE_CASE_SENSITIVE_SEARCH))
        return FilesCaseSensitivity::Insensitive;
    return FilesCaseSensitivity::Unknown;
}
std::optional<std::wstring_view> DirectChildName(const std::filesystem::path &root, const std::filesystem::path &path) noexcept
{
    const auto &parent = root.native(), &name = path.native();
    const auto separator = [](wchar_t c) { return c == L'\\' || c == L'/'; };
    if (parent.empty() || name.size() <= parent.size())
        return std::nullopt;
    for (std::size_t index = 0; index < parent.size(); ++index)
        if (parent[index] != name[index] && !(separator(parent[index]) && separator(name[index])))
            return std::nullopt;
    auto first = parent.size();
    if (!separator(parent.back()))
    {
        if (!separator(name[first]))
            return std::nullopt;
        ++first;
    }
    const auto filename = std::wstring_view(name).substr(first);
    return filename.empty() || filename.find_first_of(L"\\/") != filename.npos ? std::nullopt : std::optional(filename);
}
struct Scan
{
    FilesSnapshot snapshot;
    FilesLimits limits;
    std::stop_token stop;
    void Error(const std::filesystem::path &path, DWORD error)
    {
        const HRESULT result = HRESULT_FROM_WIN32(error);
        if (SUCCEEDED(snapshot.status))
            snapshot.status = result;
        ++snapshot.errorCount;
        if (snapshot.errors.size() < 16)
            snapshot.errors.push_back({path, result});
    }
    bool Cancelled() noexcept
    {
        if (!stop.stop_requested())
            return false;
        snapshot.cancelled = true;
        snapshot.status = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        return true;
    }
    void Directory(const std::filesystem::path &path, std::size_t parent, unsigned depth)
    {
        if (Cancelled())
            return;
        auto directory = OpenDirectory(path);
        if (!directory)
        {
            Error(path, GetLastError());
            return;
        }
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        if (!GetFileInformationByHandleEx(directory.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)))
        {
            Error(path, GetLastError());
            return;
        }
        if (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            // A child replaced between listing/opening becomes a visible leaf.
            if (parent != NoFilesParent)
                snapshot.entries[parent].reparsePoint = true;
            else
                Error(path, ERROR_CANT_ACCESS_FILE);
            return;
        }
        if (!(attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            Error(path, ERROR_DIRECTORY);
            return;
        }
        if (parent == NoFilesParent)
            snapshot.rootCaseSensitivity = DirectoryCaseSensitivity(directory.get());
        std::vector<FilesEntry> children;
        std::vector<std::byte> buffer(16u * 1024u);
        bool first = true;
        bool complete = false;
        while (!complete && !Cancelled())
        {
            const auto informationClass = first ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
            first = false;
            if (!GetFileInformationByHandleEx(directory.get(), informationClass, buffer.data(), static_cast<DWORD>(buffer.size())))
            {
                const DWORD error = GetLastError();
                if (error != ERROR_NO_MORE_FILES)
                    Error(path, error);
                break;
            }
            std::size_t offset{};
            for (;;)
            {
                if (Cancelled())
                    return;
                const auto *entry = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO *>(buffer.data() + offset);
                const std::wstring_view name(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
                if (name != L"." && name != L"..")
                {
                    if (snapshot.examinedEntries == limits.maximumEntries)
                    {
                        snapshot.entryLimitReached = true;
                        complete = true;
                        break;
                    }
                    ++snapshot.examinedEntries;
                    const DWORD flags = entry->FileAttributes;
                    const bool folder = (flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if (!(flags & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) && (folder || IsMarkdownFile(std::filesystem::path(name))))
                        children.push_back(
                            {path / name, std::wstring(name), parent, folder ? 0u : 1u, depth, folder, (flags & FILE_ATTRIBUTE_REPARSE_POINT) != 0});
                }
                if (!entry->NextEntryOffset)
                    break;
                offset += entry->NextEntryOffset;
            }
        }
        std::ranges::sort(children, [](const FilesEntry &left, const FilesEntry &right)
                          { return left.directory != right.directory ? left.directory : FilesNaturalLess(left.name, right.name); });
        for (auto &entry : children)
        {
            if (Cancelled())
                return;
            const std::size_t index = snapshot.entries.size();
            const bool folder = entry.directory, reparse = entry.reparsePoint;
            snapshot.entries.push_back(std::move(entry));
            if (!folder)
            {
                ++snapshot.markdownCount;
                continue;
            }
            const std::size_t before = snapshot.markdownCount;
            if (!reparse)
            {
                if (depth < limits.maximumDepth)
                {
                    if (snapshot.examinedEntries < limits.maximumEntries)
                    {
                        const auto childPath = snapshot.entries[index].path;
                        Directory(childPath, index, depth + 1);
                    }
                    else
                        snapshot.entryLimitReached = true;
                }
                else
                    snapshot.depthLimitReached = true;
            }
            snapshot.entries[index].markdownCount = snapshot.markdownCount - before;
            if (snapshot.entries.size() == index + 1 && !snapshot.entries[index].reparsePoint)
                snapshot.entries.pop_back(); // A completely empty/irrelevant folder has no tree row.
        }
    }
};
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626)
class DirectoryWatch final
{
  public:
    wil::unique_handle directory;
    wil::unique_event event;
    HRESULT status = S_OK;
    bool pending{};
    OVERLAPPED overlapped{};
    alignas(DWORD) std::array<std::byte, 16u * 1024u> buffer{};
    ~DirectoryWatch()
    {
        Close();
    }
    void Close() noexcept
    {
        if (pending)
        {
            (void)CancelIoEx(directory.get(), &overlapped);
            DWORD bytes{};
            // Only the worker waits for kernel ownership of its OVERLAPPED and
            // buffer to end. UI Close never destroys or waits for this state.
            (void)GetOverlappedResult(directory.get(), &overlapped, &bytes, TRUE);
            pending = false;
        }
        directory.reset();
        event.reset();
    }
    void Open(const std::filesystem::path &path) noexcept
    {
        Close();
        status = S_OK;
        if (path.empty())
            return;
        directory = OpenDirectory(path, true);
        if (!directory)
        {
            status = HRESULT_FROM_WIN32(GetLastError());
            return;
        }
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        if (!GetFileInformationByHandleEx(directory.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)))
            status = HRESULT_FROM_WIN32(GetLastError());
        else if ((attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || !(attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            status = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        if (FAILED(status))
        {
            directory.reset();
            return;
        }
        event.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!event)
        {
            status = HRESULT_FROM_WIN32(GetLastError());
            directory.reset();
            return;
        }
        Arm();
    }
    void Arm() noexcept
    {
        if (!directory || !event || pending)
            return;
        ResetEvent(event.get());
        overlapped = {};
        overlapped.hEvent = event.get();
        if (ReadDirectoryChangesW(directory.get(), buffer.data(), static_cast<DWORD>(buffer.size()), TRUE,
                                  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES, nullptr, &overlapped,
                                  nullptr))
            pending = true;
        else
        {
            status = HRESULT_FROM_WIN32(GetLastError());
            Close();
        }
    }
    void Complete() noexcept
    {
        DWORD bytes{};
        if (!GetOverlappedResult(directory.get(), &overlapped, &bytes, FALSE))
        {
            const DWORD error = GetLastError();
            // A zero/overflow notification means rescan the bounded model; its
            // individual names are deliberately never trusted as a complete list.
            if (error != ERROR_NOTIFY_ENUM_DIR)
            {
                status = HRESULT_FROM_WIN32(error);
                Close();
                return;
            }
        }
        pending = false;
        Arm();
    }
};
#pragma warning(pop)
} // namespace

bool IsMarkdownFile(const std::filesystem::path &path) noexcept
{
    const auto &name = path.native();
    const auto at = name.find_last_of(L'.');
    if (at == name.npos)
        return false;
    const auto extension = std::wstring_view(name).substr(at);
    return EqualOrdinal(extension, L".md") || EqualOrdinal(extension, L".markdown");
}
bool MatchesFilesPath(const FilesSnapshot &snapshot, const std::filesystem::path &left, const std::filesystem::path &right) noexcept
{
    if (left == right)
        return true;
    if (snapshot.rootCaseSensitivity != FilesCaseSensitivity::Insensitive)
        return false;
    const auto a = DirectChildName(snapshot.root, left), b = DirectChildName(snapshot.root, right);
    return a && b && EqualOrdinal(a.value(), b.value());
}
bool FilesNaturalLess(std::wstring_view left, std::wstring_view right) noexcept
{
    std::size_t a{}, b{};
    while (a < left.size() && b < right.size())
    {
        if (Digit(left[a]) && Digit(right[b]))
        {
            const auto beginA = a, beginB = b;
            while (a < left.size() && Digit(left[a]))
                ++a;
            while (b < right.size() && Digit(right[b]))
                ++b;
            auto significantA = beginA, significantB = beginB;
            while (significantA < a && left[significantA] == L'0')
                ++significantA;
            while (significantB < b && right[significantB] == L'0')
                ++significantB;
            const auto numberA = left.substr(significantA, a - significantA), numberB = right.substr(significantB, b - significantB);
            if (numberA.size() != numberB.size())
                return numberA.size() < numberB.size();
            if (numberA != numberB)
                return numberA < numberB;
            if (a - beginA != b - beginB)
                return a - beginA < b - beginB;
        }
        else
        {
            const int compared = CompareStringOrdinal(left.data() + a, 1, right.data() + b, 1, TRUE);
            if (compared != CSTR_EQUAL)
                return compared == CSTR_LESS_THAN;
            ++a;
            ++b;
        }
    }
    if (a != left.size() || b != right.size())
        return a == left.size();
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), FALSE) == CSTR_LESS_THAN;
}
FilesSnapshot EnumerateMarkdownFiles(const std::filesystem::path &root, FilesLimits limits, std::stop_token stop)
{
    Scan scan{{}, limits, stop};
    scan.snapshot.root = root;
    scan.snapshot.appliedLimits = limits;
    if (!ValidLimits(limits) || (!root.empty() && !root.is_absolute()))
        scan.snapshot.status = E_INVALIDARG;
    else if (!root.empty())
        scan.Directory(root, NoFilesParent, 0);
    return std::move(scan.snapshot);
}
std::vector<std::size_t> FilterMarkdownFiles(const FilesSnapshot &snapshot, std::wstring_view filter)
{
    std::vector<bool> keep(snapshot.entries.size(), filter.empty());
    for (std::size_t index = 0; index < snapshot.entries.size() && !filter.empty(); ++index)
    {
        const auto &entry = snapshot.entries[index];
        if (entry.directory || !ContainsOrdinal(entry.name, filter))
            continue;
        keep[index] = true;
        auto parent = entry.parent;
        // Snapshots produced by enumeration are acyclic; the bound also makes
        // filtering a caller-created malformed value safe and deterministic.
        for (std::size_t remaining = snapshot.entries.size(); remaining && parent < index; --remaining)
        {
            keep[parent] = true;
            parent = snapshot.entries[parent].parent;
        }
    }
    std::vector<std::size_t> selected;
    for (std::size_t index = 0; index < keep.size(); ++index)
        if (keep[index])
            selected.push_back(index);
    return selected;
}

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct FilesService::State
{
    struct Request
    {
        std::filesystem::path root;
        std::uint64_t generation{}, revision{};
    };
    mutable std::mutex mutex;
    wil::unique_event wake;
    HWND window{};
    UINT message{};
    FilesLimits limits;
    Request latest;
    std::optional<Request> pending;
    std::optional<FilesSnapshot> result;
    std::stop_source cancellation;
    HRESULT status = S_OK;
    bool closing{}, failed{}, busy{}, notificationPending{};
    void Publish(FilesSnapshot snapshot)
    {
        std::lock_guard lock(mutex);
        if (closing || snapshot.cancelled || snapshot.generation != latest.generation || snapshot.revision != latest.revision)
            return;
        status = FAILED(snapshot.status) ? snapshot.status : snapshot.watchStatus;
        result = std::move(snapshot);
        busy = false;
        if (window && !notificationPending)
            notificationPending = PostMessageW(window, message, 0, 0) != FALSE;
    }
    void Fail(HRESULT failure)
    {
        FilesSnapshot snapshot;
        {
            std::lock_guard lock(mutex);
            failed = true;
            status = failure;
            busy = false;
            pending.reset();
            snapshot.root = latest.root;
            snapshot.generation = latest.generation;
            snapshot.revision = latest.revision;
            snapshot.appliedLimits = limits;
        }
        snapshot.status = failure;
        Publish(std::move(snapshot));
        OutputDebugStringW(L"RedTilde Files worker stopped after an unexpected service failure.\n");
    }
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void *context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State> *>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State> *>(context));
        const auto &self = ownership->state;
        try
        {
            DirectoryWatch watch;
            Request current;
            bool scan{};
            for (;;)
            {
                std::stop_token stop;
                bool requested{};
                {
                    std::lock_guard lock(self->mutex);
                    if (self->closing)
                        return;
                    if (self->pending)
                    {
                        current = std::move(self->pending.value());
                        self->pending.reset();
                        requested = scan = true;
                    }
                    if (scan)
                    {
                        self->cancellation = std::stop_source{};
                        stop = self->cancellation.get_token();
                        self->busy = true;
                    }
                }
                if (requested)
                    watch.Open(current.root); // Arm before enumeration so changes cannot fall between scan and watch.
                if (scan)
                {
                    auto snapshot = EnumerateMarkdownFiles(current.root, self->limits, stop);
                    snapshot.generation = current.generation;
                    snapshot.revision = current.revision;
                    snapshot.watchStatus = watch.status;
                    self->Publish(std::move(snapshot));
                    scan = false;
                }
                const HANDLE waits[]{self->wake.get(), watch.pending ? watch.event.get() : nullptr};
                const DWORD ready = WaitForMultipleObjects(watch.pending ? 2u : 1u, waits, FALSE, INFINITE);
                if (ready == WAIT_OBJECT_0)
                    continue;
                if (ready == WAIT_FAILED)
                {
                    self->Fail(HRESULT_FROM_WIN32(GetLastError()));
                    return;
                }
                if (ready != WAIT_OBJECT_0 + 1)
                    continue;
                watch.Complete();
                const ULONGLONG deadline = GetTickCount64() + 250;
                // One quiet-time wait after an actual change, capped at 250 ms
                // under continuous writes. No periodic wakeup while the tree is idle.
                while (watch.pending)
                {
                    const auto now = GetTickCount64();
                    if (now >= deadline)
                        break;
                    const DWORD quiet = static_cast<DWORD>(std::min<ULONGLONG>(75, deadline - now));
                    const HANDLE coalesced[]{self->wake.get(), watch.event.get()};
                    const DWORD next = WaitForMultipleObjects(2, coalesced, FALSE, quiet);
                    if (next == WAIT_OBJECT_0 || next == WAIT_TIMEOUT || next == WAIT_FAILED)
                        break;
                    watch.Complete();
                }
                {
                    std::lock_guard lock(self->mutex);
                    if (!self->closing && !self->pending && current.generation == self->latest.generation)
                    {
                        current.revision = ++self->latest.revision;
                        scan = true;
                    }
                }
            }
        }
        catch (const std::bad_alloc &)
        {
            // Threadpool ABI boundary: allocation failure is fatal, never a
            // misleading empty tree or a silently swallowed worker failure.
            DxUi::Failure::FailAllocation();
        }
        catch (const std::filesystem::filesystem_error &error)
        {
            // Threadpool ABI boundary: publish a concrete failed result and stop
            // admission; a stopped worker must never accept an unserviceable root.
            self->Fail(HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value())));
        }
    }
};
#pragma warning(pop)
FilesService::~FilesService()
{
    Close();
}
HRESULT FilesService::Initialize(HWND window, UINT message, FilesLimits limits) noexcept
try
{
    if (!ValidLimits(limits) || (window && message < WM_APP))
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window = window;
    staged->message = message;
    staged->limits = limits;
    auto ownership = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT admission = ProcessWork::Acquire(ownership->lease);
    if (FAILED(admission))
        return admission;
    if (!TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    DxUi::Failure::FailAllocation(); // Application boundary; no partially initialized service survives allocation failure.
}
HRESULT FilesService::SetRoot(std::filesystem::path root) noexcept
try
{
    if (!state)
        return E_UNEXPECTED;
    if (!root.empty() && (!root.is_absolute() || root.native().find(L'\0') != std::wstring::npos))
        return E_INVALIDARG;
    root = root.lexically_normal();
    std::lock_guard lock(state->mutex);
    if (state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (state->failed)
        return state->status;
    if (state->latest.generation && root == state->latest.root)
        return S_FALSE;
    state->cancellation.request_stop();
    state->latest.root = std::move(root);
    ++state->latest.generation;
    ++state->latest.revision;
    state->pending = state->latest;
    state->result.reset();
    state->busy = true;
    SetEvent(state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc &)
{
    DxUi::Failure::FailAllocation(); // Application boundary; request admission cannot silently lose allocation failure.
}
HRESULT FilesService::Refresh() noexcept
try
{
    if (!state)
        return E_UNEXPECTED;
    std::lock_guard lock(state->mutex);
    if (state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (state->failed)
        return state->status;
    state->cancellation.request_stop();
    ++state->latest.revision;
    state->pending = state->latest;
    state->result.reset();
    state->busy = true;
    SetEvent(state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc &)
{
    DxUi::Failure::FailAllocation(); // Application boundary; latest-value admission either succeeds or terminates.
}
std::optional<FilesSnapshot> FilesService::TakeResult()
{
    if (!state)
        return {};
    std::lock_guard lock(state->mutex);
    state->notificationPending = false;
    auto snapshot = std::move(state->result);
    state->result.reset();
    if (snapshot && (snapshot->generation != state->latest.generation || snapshot->revision != state->latest.revision))
        return {};
    return snapshot;
}
bool FilesService::Busy() const noexcept
{
    if (!state)
        return false;
    std::lock_guard lock(state->mutex);
    return state->busy;
}
HRESULT FilesService::Status() const noexcept
{
    if (!state)
        return E_UNEXPECTED;
    std::lock_guard lock(state->mutex);
    return state->status;
}
std::uint64_t FilesService::Generation() const noexcept
{
    if (!state)
        return 0;
    std::lock_guard lock(state->mutex);
    return state->latest.generation;
}
void FilesService::Close() noexcept
{
    if (!state)
        return;
    {
        std::lock_guard lock(state->mutex);
        state->closing = true;
        state->window = nullptr;
        state->result.reset();
        state->cancellation.request_stop();
        SetEvent(state->wake.get());
    }
    state.reset();
}
} // namespace RedTilde
