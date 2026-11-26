#include "Files.h"
#include "DirectoryWatch.h"
#include "PathSecurity.h"
#include "ProcessWork.h"
#include "Settings.h"
#include "Storage.h"
#include "TextUtilities.h"
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
bool Digit(wchar_t value) noexcept
{
    return value >= L'0' && value <= L'9';
}
wil::unique_handle OpenDirectory(const std::filesystem::path& path, bool watch = false) noexcept
{
    // The scan pins parent names and denies metadata writes until their children
    // are visited. A checked parent cannot be renamed or converted to a reparse
    // point to redirect descent. Watch handles permit ordinary external changes.
    return wil::unique_handle(CreateFileW(path.c_str(),
                                          FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                          FILE_SHARE_READ | (watch ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0u),
                                          nullptr,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | (watch ? FILE_FLAG_OVERLAPPED : 0u),
                                          nullptr));
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
    if (GetVolumeInformationByHandleW(directory, nullptr, 0, nullptr, nullptr, &flags, nullptr, 0) && ! (flags & FILE_CASE_SENSITIVE_SEARCH))
        return FilesCaseSensitivity::Insensitive;
    return FilesCaseSensitivity::Unknown;
}
std::optional<std::wstring_view> DirectChildName(const std::filesystem::path& root, const std::filesystem::path& path) noexcept
{
    const auto &parent = root.native(), &name = path.native();
    const auto separator = [](wchar_t c) { return c == L'\\' || c == L'/'; };
    if (parent.empty() || name.size() <= parent.size())
        return std::nullopt;
    for (std::size_t index = 0; index < parent.size(); ++index)
        if (parent[index] != name[index] && ! (separator(parent[index]) && separator(name[index])))
            return std::nullopt;
    auto first = parent.size();
    if (! separator(parent.back()))
    {
        if (! separator(name[first]))
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
    std::vector<std::byte> dirBuffer = std::vector<std::byte>(16u * 1024u);
    void Error(const std::filesystem::path& path, DWORD error)
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
        if (! stop.stop_requested())
            return false;
        snapshot.cancelled = true;
        snapshot.status    = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        return true;
    }
    void Directory(const std::filesystem::path& path, std::size_t parent, unsigned depth)
    {
        if (Cancelled())
            return;
        auto directory = OpenDirectory(path);
        if (! directory)
        {
            Error(path, GetLastError());
            return;
        }
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        if (! GetFileInformationByHandleEx(directory.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)))
        {
            Error(path, GetLastError());
            return;
        }
        if ((attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && IsReparseTagNameSurrogate(attributes.ReparseTag))
        {
            // A child replaced between listing/opening becomes a visible leaf.
            if (parent != NoFilesParent)
                snapshot.entries[parent].reparsePoint = true;
            else
                Error(path, ERROR_CANT_ACCESS_FILE);
            return;
        }
        if (! (attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            Error(path, ERROR_DIRECTORY);
            return;
        }
        if (parent == NoFilesParent)
            snapshot.rootCaseSensitivity = DirectoryCaseSensitivity(directory.get());
        std::vector<FilesEntry> children;
        bool first    = true;
        bool complete = false;
        while (! complete && ! Cancelled())
        {
            const auto informationClass = first ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
            first                       = false;
            if (! GetFileInformationByHandleEx(directory.get(), informationClass, dirBuffer.data(), static_cast<DWORD>(dirBuffer.size())))
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
                const auto* entry = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO*>(dirBuffer.data() + offset);
                const std::wstring_view name(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
                if (name != L"." && name != L"..")
                {
                    if (snapshot.examinedEntries == limits.maximumEntries)
                    {
                        snapshot.entryLimitReached = true;
                        complete                   = true;
                        break;
                    }
                    ++snapshot.examinedEntries;
                    const DWORD flags           = entry->FileAttributes;
                    const bool folder           = (flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    const bool surrogateReparse = (flags & FILE_ATTRIBUTE_REPARSE_POINT) != 0 && IsReparseTagNameSurrogate(entry->EaSize) != FALSE;
                    if (! (flags & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) && (folder || IsMarkdownFileName(name)))
                        children.push_back({path / name, std::wstring(name), parent, folder ? 0u : 1u, depth, folder, surrogateReparse});
                }
                if (! entry->NextEntryOffset)
                    break;
                offset += entry->NextEntryOffset;
            }
        }
        std::ranges::sort(children, [](const FilesEntry& left, const FilesEntry& right) {
            return left.directory != right.directory ? left.directory : FilesNaturalLess(left.name, right.name);
        });
        for (auto& entry : children)
        {
            if (Cancelled())
                return;
            const std::size_t index = snapshot.entries.size();
            const bool folder = entry.directory, reparse = entry.reparsePoint;
            snapshot.entries.push_back(std::move(entry));
            if (! folder)
            {
                ++snapshot.markdownCount;
                continue;
            }
            const std::size_t before = snapshot.markdownCount;
            if (! reparse)
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
            if (snapshot.entries.size() == index + 1 && ! snapshot.entries[index].reparsePoint)
                snapshot.entries.pop_back(); // A completely empty/irrelevant folder has no tree row.
        }
    }
};
HRESULT OpenDirectoryWatch(DirectoryWatch& watch, const std::filesystem::path& path, bool imageFiles) noexcept
{
    watch.Close();
    if (path.empty())
        return S_OK;
    wil::unique_handle directory;
    if (imageFiles)
    {
        const HRESULT openHr = OpenImageDirectoryForNotifications(path.native(), directory);
        if (FAILED(openHr))
        {
            watch.SetFailedStatus(openHr);
            return openHr;
        }
    }
    else
    {
        directory = OpenDirectory(path, true);
    }
    if (! directory)
    {
        const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        watch.SetFailedStatus(hr);
        return hr;
    }
    if (FAILED(VerifyDiskHandle(directory.get(), true)))
    {
        const HRESULT hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        watch.SetFailedStatus(hr);
        return hr;
    }
    const DWORD filter = imageFiles ? FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE
                                    : FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES;
    return watch.AttachAndArm(std::move(directory), ! imageFiles, filter);
}
} // namespace

bool IsMarkdownFileName(std::wstring_view name) noexcept
{
    const auto slash = name.find_last_of(L"\\/");
    const auto leaf  = slash == std::wstring_view::npos ? name : name.substr(slash + 1u);
    const auto at    = leaf.find_last_of(L'.');
    if (at == std::wstring_view::npos)
        return false;
    const auto extension = leaf.substr(at);
    return EqualOrdinalIgnoreCase(extension, L".md") || EqualOrdinalIgnoreCase(extension, L".markdown");
}

bool IsMarkdownFile(const std::filesystem::path& path) noexcept
{
    return IsMarkdownFileName(path.native());
}
bool MatchesFilesPath(const FilesSnapshot& snapshot, const std::filesystem::path& left, const std::filesystem::path& right) noexcept
{
    if (left == right)
        return true;
    if (snapshot.rootCaseSensitivity != FilesCaseSensitivity::Insensitive)
        return false;
    const auto a = DirectChildName(snapshot.root, left), b = DirectChildName(snapshot.root, right);
    return a && b && EqualOrdinalIgnoreCase(a.value(), b.value());
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
FilesSnapshot EnumerateMarkdownFiles(const std::filesystem::path& root, FilesLimits limits, std::stop_token stop)
{
    Scan scan{{}, limits, stop};
    scan.snapshot.root          = root;
    scan.snapshot.appliedLimits = limits;
    if (! ValidLimits(limits) || (! root.empty() && ! root.is_absolute()))
        scan.snapshot.status = E_INVALIDARG;
    else if (! root.empty())
        scan.Directory(root, NoFilesParent, 0);
    return std::move(scan.snapshot);
}
std::vector<std::size_t> FilterMarkdownFiles(const FilesSnapshot& snapshot, std::wstring_view filter)
{
    std::vector<bool> keep(snapshot.entries.size(), filter.empty());
    for (std::size_t index = 0; index < snapshot.entries.size() && ! filter.empty(); ++index)
    {
        const auto& entry = snapshot.entries[index];
        if (entry.directory || ! ContainsOrdinalIgnoreCase(entry.name, filter))
            continue;
        keep[index] = true;
        auto parent = entry.parent;
        // Snapshots produced by enumeration are acyclic; the bound also makes
        // filtering a caller-created malformed value safe and deterministic.
        for (std::size_t remaining = snapshot.entries.size(); remaining && parent < index; --remaining)
        {
            keep[parent] = true;
            parent       = snapshot.entries[parent].parent;
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
    FilesPhase phase{FilesPhase::Idle};
    bool notificationPending{};
    void Publish(FilesSnapshot snapshot, bool watching)
    {
        std::lock_guard lock(mutex);
        if (phase == FilesPhase::Closed || phase == FilesPhase::Failed || snapshot.cancelled || snapshot.generation != latest.generation ||
            snapshot.revision != latest.revision)
            return;
        status = FAILED(snapshot.status) ? snapshot.status : snapshot.watchStatus;
        result = std::move(snapshot);
        phase  = pending ? FilesPhase::Scanning : (watching ? FilesPhase::Watching : FilesPhase::Idle);
        if (window && ! notificationPending)
            notificationPending = PostMessageW(window, message, 0, 0) != FALSE;
    }
    void Fail(HRESULT failure)
    {
        {
            std::lock_guard lock(mutex);
            if (phase == FilesPhase::Closed)
                return;
            phase  = FilesPhase::Failed;
            status = failure;
            cancellation.request_stop();
            pending.reset();
            FilesSnapshot snapshot;
            snapshot.root          = latest.root;
            snapshot.generation    = latest.generation;
            snapshot.revision      = latest.revision;
            snapshot.appliedLimits = limits;
            snapshot.status        = failure;
            result                 = std::move(snapshot);
            if (window && ! notificationPending)
                notificationPending = PostMessageW(window, message, 0, 0) != FALSE;
            (void)SetEvent(wake.get());
        }
        OutputDebugStringW(L"RedTilde Files worker stopped after an unexpected service failure.\n");
    }
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
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
                    if (self->phase == FilesPhase::Closed || self->phase == FilesPhase::Failed)
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
                        stop               = self->cancellation.get_token();
                        self->phase        = FilesPhase::Scanning;
                    }
                }
                if (requested)
                    (void)OpenDirectoryWatch(watch, current.root, false); // Arm before enumeration so changes cannot fall between scan and watch.
                if (scan)
                {
                    auto snapshot        = EnumerateMarkdownFiles(current.root, self->limits, stop);
                    snapshot.generation  = current.generation;
                    snapshot.revision    = current.revision;
                    snapshot.watchStatus = watch.Status();
                    self->Publish(std::move(snapshot), watch.IsPending());
                    scan = false;
                }
                const HANDLE waits[]{self->wake.get(), watch.IsPending() ? watch.EventHandle() : nullptr};
                const DWORD ready = WaitForMultipleObjects(watch.IsPending() ? 2u : 1u, waits, FALSE, INFINITE);
                if (ready == WAIT_OBJECT_0)
                    continue;
                if (ready == WAIT_FAILED)
                {
                    self->Fail(HRESULT_FROM_WIN32(GetLastError()));
                    return;
                }
                if (ready != WAIT_OBJECT_0 + 1)
                    continue;
                (void)watch.Complete();
                if (watch.IsPending())
                {
                    std::lock_guard lock(self->mutex);
                    if (self->phase == FilesPhase::Closed || self->phase == FilesPhase::Failed)
                        return;
                    if (! self->pending && current.generation == self->latest.generation)
                        self->phase = FilesPhase::CoalescingWatch;
                }
                const ULONGLONG deadline = GetTickCount64() + 250;
                // One quiet-time wait after an actual change, capped at 250 ms
                // under continuous writes. No periodic wakeup while the tree is idle.
                while (watch.IsPending())
                {
                    const auto now = GetTickCount64();
                    if (now >= deadline)
                        break;
                    const DWORD quiet = static_cast<DWORD>(std::min<ULONGLONG>(75, deadline - now));
                    const HANDLE coalesced[]{self->wake.get(), watch.EventHandle()};
                    const DWORD next = WaitForMultipleObjects(2, coalesced, FALSE, quiet);
                    if (next == WAIT_FAILED)
                    {
                        self->Fail(HRESULT_FROM_WIN32(GetLastError()));
                        return;
                    }
                    if (next == WAIT_OBJECT_0 || next == WAIT_TIMEOUT)
                        break;
                    (void)watch.Complete();
                }
                {
                    std::lock_guard lock(self->mutex);
                    if (self->phase == FilesPhase::Closed || self->phase == FilesPhase::Failed)
                        return;
                    if (! self->pending && current.generation == self->latest.generation)
                    {
                        current.revision = ++self->latest.revision;
                        scan             = true;
                        self->phase      = FilesPhase::Scanning;
                    }
                    else if (! self->pending)
                        self->phase = watch.IsPending() ? FilesPhase::Watching : FilesPhase::Idle;
                }
            }
        }
        catch (const std::bad_alloc&)
        {
            // Threadpool ABI boundary: allocation failure is fatal, never a
            // misleading empty tree or a silently swallowed worker failure.
            DxUi::Failure::FailAllocation();
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            // Threadpool ABI boundary: publish a concrete failed result and stop
            // admission; a stopped worker must never accept an unserviceable root.
            self->Fail(HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value())));
        }
        catch (const std::exception&)
        {
            DxUi::Failure::FailUnexpected();
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
#if defined(ENABLE_TESTS)
    if (const auto injected = std::exchange(_nextInitializeFailureForTest, std::nullopt))
    {
        Close();
        return *injected;
    }
#endif
    if (! ValidLimits(limits) || (window && message < WM_APP))
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window          = window;
    staged->message         = message;
    staged->limits          = limits;
    auto ownership          = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT admission = ProcessWork::Acquire(ownership->lease);
    if (FAILED(admission))
        return admission;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation(); // Application boundary; no partially initialized service survives allocation failure.
}
HRESULT FilesService::SetRoot(std::filesystem::path root) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    if (! root.empty() && (! root.is_absolute() || root.native().find(L'\0') != std::wstring::npos))
        return E_INVALIDARG;
    root = NormalizeRootPath(root);
    std::lock_guard lock(_state->mutex);
    if (_state->phase == FilesPhase::Closed)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->phase == FilesPhase::Failed)
        return _state->status;
    if (_state->latest.generation && EqualNormalizedRootPath(root, _state->latest.root))
        return S_FALSE;
    _state->cancellation.request_stop();
    _state->latest.root = std::move(root);
    ++_state->latest.generation;
    ++_state->latest.revision;
    _state->pending = _state->latest;
    _state->result.reset();
    _state->phase = FilesPhase::Scanning;
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation(); // Application boundary; request admission cannot silently lose allocation failure.
}
HRESULT FilesService::Refresh() noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->phase == FilesPhase::Closed)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->phase == FilesPhase::Failed)
        return _state->status;
    _state->cancellation.request_stop();
    ++_state->latest.revision;
    _state->pending = _state->latest;
    _state->result.reset();
    _state->phase = FilesPhase::Scanning;
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation(); // Application boundary; latest-value admission either succeeds or terminates.
}
std::optional<FilesSnapshot> FilesService::TakeResult()
{
    if (! _state)
        return {};
    std::lock_guard lock(_state->mutex);
    _state->notificationPending = false;
    auto snapshot               = std::move(_state->result);
    _state->result.reset();
    if (snapshot && (snapshot->generation != _state->latest.generation || snapshot->revision != _state->latest.revision))
        return {};
    return snapshot;
}
FilesPhase FilesService::CurrentPhase() const noexcept
{
    if (! _state)
        return FilesPhase::Closed;
    std::lock_guard lock(_state->mutex);
    return _state->phase;
}
bool FilesService::IsFailed() const noexcept
{
    return CurrentPhase() == FilesPhase::Failed;
}
bool FilesService::IsActive() const noexcept
{
    const auto phase = CurrentPhase();
    return phase != FilesPhase::Closed && phase != FilesPhase::Failed;
}
bool FilesService::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->phase == FilesPhase::Scanning || _state->phase == FilesPhase::CoalescingWatch;
}
HRESULT FilesService::Status() const noexcept
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    return _state->status;
}
std::uint64_t FilesService::Generation() const noexcept
{
    if (! _state)
        return 0;
    std::lock_guard lock(_state->mutex);
    return _state->latest.generation;
}
void FilesService::Close() noexcept
{
    if (! _state)
        return;
    {
        std::lock_guard lock(_state->mutex);
        _state->phase  = FilesPhase::Closed;
        _state->window = nullptr;
        _state->result.reset();
        _state->cancellation.request_stop();
        (void)SetEvent(_state->wake.get());
    }
    _state.reset();
}
#if defined(ENABLE_TESTS)
void FilesService::FailNextInitializeForTest(HRESULT hr) noexcept
{
    _nextInitializeFailureForTest = hr;
}
void FilesService::SimulateWorkerFailureForTest(HRESULT hr) noexcept
{
    if (_state)
        _state->Fail(hr);
}
#endif

// Identity-bearing services are neither copied nor moved; their shared State
// remains owned by ProcessWorkItem until the worker drains native notification I/O.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct FileChangeService::State
{
    std::mutex mutex;
    wil::unique_event wake;
    std::vector<std::filesystem::path> files;
    std::optional<FileChangeNotice> result;
    HWND window{};
    UINT message{};
    WPARAM cookie{};
    std::uint64_t generation{}, revision{};
    std::atomic_bool hasResult{false};
    bool closing{}, requested{}, notificationPending{}, stopped{};
    HRESULT stopStatus = S_OK;
    void Fail(std::uint64_t current, HRESULT failure)
    {
        {
            const std::lock_guard lock(mutex);
            stopped    = true;
            stopStatus = failure;
        }
        Publish(current, failure);
    }
    void Publish(std::uint64_t current, HRESULT status)
    {
        const std::lock_guard lock(mutex);
        if (closing || current != generation)
            return;
        result = FileChangeNotice{generation, ++revision, status};
        hasResult.store(true, std::memory_order_release);
        if (window && ! notificationPending)
            notificationPending = PostMessageW(window, message, cookie, 0) != FALSE;
    }
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
        try
        {
            std::array<DirectoryWatch, 8u> watches;
            std::array<std::vector<std::filesystem::path>, 8u> names;
            std::vector<std::filesystem::path> directories;
            std::vector<std::filesystem::path> files;
            std::uint64_t current{};
            for (;;)
            {
                bool requested{};
                {
                    const std::lock_guard lock(self->mutex);
                    if (self->closing)
                        return;
                    if (self->requested)
                    {
                        current         = self->generation;
                        files           = self->files;
                        self->requested = false;
                        requested       = true;
                    }
                }
                if (requested)
                {
                    for (auto& watch : watches)
                        watch.Close();
                    for (auto& values : names)
                        values.clear();
                    directories.clear();
                    for (const auto& file : files)
                    {
                        const auto parent = file.parent_path();
                        auto found        = std::ranges::find(directories, parent);
                        if (found == directories.end())
                        {
                            directories.push_back(parent);
                            found = std::prev(directories.end());
                        }
                        names[static_cast<std::size_t>(found - directories.begin())].push_back(file.filename());
                    }
                    HRESULT status = S_OK;
                    for (std::size_t i{}; i < directories.size(); ++i)
                    {
                        const HRESULT watchHr = OpenDirectoryWatch(watches[i], directories[i], true);
                        if (FAILED(watchHr))
                            status = watchHr;
                    }
                    // Refresh once after arming. This closes the initial
                    // decode/notification setup gap without a polling timer.
                    if (! files.empty())
                        self->Publish(current, status);
                }
                std::array<HANDLE, 9u> waits{};
                std::array<std::size_t, 9u> indexes{};
                DWORD count = 1u;
                waits[0]    = self->wake.get();
                for (std::size_t i{}; i < directories.size(); ++i)
                    if (watches[i].IsPending())
                    {
                        waits[count]     = watches[i].EventHandle();
                        indexes[count++] = i;
                    }
                const auto ready = WaitForMultipleObjects(count, waits.data(), FALSE, INFINITE);
                if (ready == WAIT_OBJECT_0)
                    continue;
                if (ready == WAIT_FAILED)
                {
                    self->Fail(current, HRESULT_FROM_WIN32(GetLastError()));
                    return;
                }
                if (ready < WAIT_OBJECT_0 + 1u || ready >= WAIT_OBJECT_0 + count)
                    continue;
                const auto index    = indexes[ready - WAIT_OBJECT_0];
                const auto poll     = watches[index].Complete(names[index]);
                bool changed        = (poll == DirectoryWatch::PollStatus::Matched || poll == DirectoryWatch::PollStatus::Overflow);
                HRESULT status      = watches[index].Status();
                const auto deadline = GetTickCount64() + 250u;
                while (watches[index].IsPending())
                {
                    // At most 250 ms of coalescing under a continuous writer;
                    // an idle directory never wakes the worker periodically.
                    const auto now = GetTickCount64();
                    if (now >= deadline)
                        break;
                    const auto quiet = static_cast<DWORD>(std::min<ULONGLONG>(75u, deadline - now));
                    const auto next  = WaitForMultipleObjects(count, waits.data(), FALSE, quiet);
                    if (next == WAIT_TIMEOUT || next == WAIT_OBJECT_0)
                        break;
                    if (next == WAIT_FAILED)
                    {
                        self->Fail(current, HRESULT_FROM_WIN32(GetLastError()));
                        return;
                    }
                    if (next < WAIT_OBJECT_0 + 1u || next >= WAIT_OBJECT_0 + count)
                        break;
                    const auto more     = indexes[next - WAIT_OBJECT_0];
                    const auto morePoll = watches[more].Complete(names[more]);
                    changed             = (morePoll == DirectoryWatch::PollStatus::Matched || morePoll == DirectoryWatch::PollStatus::Overflow) || changed;
                    if (FAILED(watches[more].Status()))
                        status = watches[more].Status();
                    if (! watches[more].IsPending())
                        break;
                }
                if (changed || FAILED(status))
                    self->Publish(current, status);
            }
        }
        catch (const std::bad_alloc&)
        {
            DxUi::Failure::FailAllocation(); // Threadpool ABI: allocation failure is fatal.
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            // Threadpool ABI: retain an explicit failed notification; never
            // publish invented paths or a successful refresh after path failure.
            std::uint64_t current{};
            {
                const std::lock_guard lock(self->mutex);
                current = self->generation;
            }
            self->Fail(current, HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value())));
            OutputDebugStringW(L"RedTilde image file notification worker stopped after a path failure.\n");
        }
    }
};
#pragma warning(pop)
FileChangeService::~FileChangeService()
{
    Close();
}
HRESULT FileChangeService::Initialize(HWND window, UINT message, WPARAM cookie) noexcept
try
{
    if (! window || message < WM_APP)
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window  = window;
    staged->message = message;
    staged->cookie  = cookie;
    auto ownership  = std::make_unique<ProcessWorkItem<State>>(staged);
    RETURN_IF_FAILED(ProcessWork::Acquire(ownership->lease));
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
HRESULT FileChangeService::SetFiles(std::span<const std::filesystem::path> input) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    if (input.size() > 32u)
        return HRESULT_FROM_WIN32(ERROR_TOO_MANY_NAMES);
    std::vector<std::filesystem::path> files, directories;
    files.reserve(input.size());
    for (const auto& path : input)
    {
        if (! path.is_absolute() || path.native().starts_with(L"\\\\") || path.native().size() > 32750u || path.native().find(L'\0') != path.native().npos)
            return E_INVALIDARG;
        files.push_back(path.lexically_normal());
        const auto parent = files.back().parent_path();
        if (std::ranges::find(directories, parent) == directories.end())
            directories.push_back(parent);
        if (directories.size() > 8u)
            return HRESULT_FROM_WIN32(ERROR_TOO_MANY_NAMES);
    }
    std::ranges::sort(files);
    files.erase(std::unique(files.begin(), files.end()), files.end());
    const std::lock_guard lock(_state->mutex);
    if (_state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->stopped)
        return _state->stopStatus;
    if (files == _state->files)
        return S_FALSE;
    _state->files = std::move(files);
    ++_state->generation;
    _state->requested = true;
    _state->result.reset();
    _state->hasResult.store(false, std::memory_order_release);
    SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
bool FileChangeService::HasResult() const noexcept
{
    return _state && _state->hasResult.load(std::memory_order_acquire);
}
std::optional<FileChangeNotice> FileChangeService::TakeResult()
{
    if (! _state)
        return {};
    const std::lock_guard lock(_state->mutex);
    _state->notificationPending = false;
    auto result                 = _state->result;
    _state->result.reset();
    _state->hasResult.store(false, std::memory_order_release);
    if (result && result->generation != _state->generation)
        return {};
    return result;
}
void FileChangeService::Close() noexcept
{
    if (! _state)
        return;
    {
        const std::lock_guard lock(_state->mutex);
        _state->closing = true;
        _state->window  = nullptr;
        _state->result.reset();
        _state->hasResult.store(false, std::memory_order_release);
        SetEvent(_state->wake.get());
    }
    _state.reset();
}

namespace
{
std::wstring NormalizeSlashesForDisplay(std::wstring_view text)
{
    std::wstring result(text);
    for (wchar_t& ch : result)
        if (ch == L'\\')
            ch = L'/';
    return result;
}

std::optional<std::wstring> RelativePathWithinRoot(const std::filesystem::path& filePath, const std::filesystem::path& root)
{
    const auto normFile = filePath.lexically_normal();
    const auto normRoot = NormalizeRootPath(root);
    const auto &fileStr = normFile.native(), &rootStr = normRoot.native();
    if (fileStr.empty() || rootStr.empty() || fileStr.size() <= rootStr.size())
        return std::nullopt;
    const auto prefix = std::wstring_view(fileStr).substr(0, rootStr.size());
    if (CompareStringOrdinal(prefix.data(), static_cast<int>(prefix.size()), rootStr.data(), static_cast<int>(rootStr.size()), TRUE) != CSTR_EQUAL)
        return std::nullopt;
    std::size_t start = rootStr.size();
    if (rootStr.back() != L'\\' && rootStr.back() != L'/')
    {
        if (fileStr[start] != L'\\' && fileStr[start] != L'/')
            return std::nullopt;
        ++start;
    }
    while (start < fileStr.size() && (fileStr[start] == L'\\' || fileStr[start] == L'/'))
        ++start;
    if (start >= fileStr.size())
        return std::nullopt;
    return NormalizeSlashesForDisplay(std::wstring_view(fileStr).substr(start));
}
} // namespace

bool IsUnsafeDisplayCodePoint(std::uint32_t cp) noexcept
{
    if (cp < 0x20u || cp == 0x7Fu || (cp >= 0x80u && cp <= 0x9Fu))
        return true;
    if (cp == 0x061Cu || cp == 0x180Eu || (cp >= 0x200Bu && cp <= 0x200Fu) || (cp >= 0x202Au && cp <= 0x202Eu) || (cp >= 0x2060u && cp <= 0x2069u) ||
        cp == 0xFEFFu)
        return true;
    if (cp >= 0xE0000u && cp <= 0xE007Fu)
        return true;
    return false;
}

std::wstring SanitizeUiText(std::wstring_view text, std::size_t maximumLength)
{
    std::wstring out;
    out.reserve(std::min<std::size_t>(text.size(), maximumLength));
    for (std::size_t i = 0; i < text.size() && out.size() < maximumLength;)
    {
        const wchar_t unit = text[i];
        if (unit >= 0xD800u && unit <= 0xDBFFu)
        {
            if (i + 1u < text.size() && text[i + 1u] >= 0xDC00u && text[i + 1u] <= 0xDFFFu)
            {
                const std::uint32_t cp =
                    0x10000u + (((static_cast<std::uint32_t>(unit) - 0xD800u) << 10) | (static_cast<std::uint32_t>(text[i + 1u]) - 0xDC00u));
                if (! IsUnsafeDisplayCodePoint(cp))
                {
                    if (out.size() + 2u > maximumLength)
                        break;
                    out.push_back(unit);
                    out.push_back(text[i + 1u]);
                }
                i += 2u;
                continue;
            }
            ++i;
            continue;
        }
        if (unit >= 0xDC00u && unit <= 0xDFFFu)
        {
            ++i;
            continue;
        }
        if (! IsUnsafeDisplayCodePoint(static_cast<std::uint32_t>(unit)))
            out.push_back(unit);
        ++i;
    }
    return out;
}

std::vector<std::wstring> DisambiguateFolderLabels(std::span<const std::filesystem::path> roots)
{
    std::vector<std::filesystem::path> normalizedRoots;
    normalizedRoots.reserve(roots.size());
    std::vector<std::wstring> rawLeaves;
    rawLeaves.reserve(roots.size());
    std::vector<std::wstring> baseNames;
    baseNames.reserve(roots.size());
    for (const auto& root : roots)
    {
        auto norm = NormalizeRootPath(root);
        auto leaf = norm.filename().native();
        if (leaf.empty())
            leaf = norm.native();
        normalizedRoots.push_back(std::move(norm));
        baseNames.push_back(SanitizeUiText(leaf, 128u));
        rawLeaves.push_back(std::move(leaf));
    }

    std::vector<std::wstring> labels;
    labels.reserve(roots.size());
    for (std::size_t i = 0; i < roots.size(); ++i)
    {
        const std::size_t matches = static_cast<std::size_t>(
            std::ranges::count_if(rawLeaves, [&](std::wstring_view candidate) { return EqualOrdinalIgnoreCase(candidate, rawLeaves[i]); }));
        if (matches > 1u)
        {
            const auto parentLeaf = SanitizeUiText(normalizedRoots[i].parent_path().filename().native(), 96u);
            if (! parentLeaf.empty())
            {
                labels.push_back(baseNames[i] + L" (" + parentLeaf + L")");
                continue;
            }
        }
        labels.push_back(baseNames[i]);
    }
    return labels;
}

WorkspaceDisplayLocation ResolveWorkspaceDisplayLocation(const std::filesystem::path& filePath,
                                                         const DocumentOriginInfo& origin,
                                                         std::span<const std::filesystem::path> roots,
                                                         std::wstring_view untitledName)
{
    WorkspaceDisplayLocation result;
    if (origin.kind == DocumentOriginInfo::Kind::GitHub && ! origin.owner.empty() && ! origin.repo.empty())
    {
        result.isGitHub     = true;
        result.rooted       = true;
        result.sectionLabel = SanitizeUiText(origin.owner, 128u) + L"/" + SanitizeUiText(origin.repo, 128u);
        result.relativePath = NormalizeSlashesForDisplay(SanitizeUiText(origin.relativePath, 512u));
        const auto pos      = result.relativePath.find_last_of(L'/');
        result.fileName     = pos == std::wstring::npos ? result.relativePath : result.relativePath.substr(pos + 1u);
        if (result.fileName.empty())
            result.fileName = std::wstring(untitledName);
        result.fullPath = SanitizeUiText(origin.htmlUrl, 512u);
        if (result.fullPath.empty())
            result.fullPath = L"https://github.com/" + result.sectionLabel + L"/blob/" + SanitizeUiText(origin.branch.empty() ? L"main" : origin.branch, 128u) +
                              L"/" + result.relativePath;
        return result;
    }

    if (filePath.empty())
    {
        result.fileName = std::wstring(untitledName);
        return result;
    }

    result.fullPath = SanitizeUiText(filePath.lexically_normal().native(), 1024u);
    result.fileName = SanitizeUiText(filePath.filename().native(), 255u);
    if (result.fileName.empty())
        result.fileName = std::wstring(untitledName);

    const auto labels          = DisambiguateFolderLabels(roots);
    std::size_t bestRootLength = 0u;
    std::optional<std::size_t> bestIndex;
    std::wstring bestRelative;
    for (std::size_t i = 0; i < roots.size(); ++i)
    {
        if (auto rel = RelativePathWithinRoot(filePath, roots[i]))
        {
            const std::size_t rootLen = NormalizeRootPath(roots[i]).native().size();
            if (rootLen > bestRootLength)
            {
                bestRootLength = rootLen;
                bestIndex      = i;
                bestRelative   = std::move(*rel);
            }
        }
    }
    if (bestIndex)
    {
        result.rooted       = true;
        result.sectionLabel = labels[*bestIndex];
        result.relativePath = SanitizeUiText(bestRelative, 512u);
    }
    else
    {
        result.relativePath = SanitizeUiText(filePath.parent_path().lexically_normal().native(), 512u);
    }
    return result;
}

struct MultiFolderFilesService::WorkerEntry
{
    WorkerEntry()                              = default;
    WorkerEntry(const WorkerEntry&)            = delete;
    WorkerEntry& operator=(const WorkerEntry&) = delete;
    std::filesystem::path root;
    std::unique_ptr<FilesService> service;
};

MultiFolderFilesService::MultiFolderFilesService() = default;

MultiFolderFilesService::~MultiFolderFilesService()
{
    Close();
}

HRESULT MultiFolderFilesService::Initialize(HWND window, UINT message, FilesLimits limits) noexcept
{
    if ((window && message < WM_APP) || limits.maximumDepth > 8 || limits.maximumEntries == 0 || limits.maximumEntries > 5000)
        return E_INVALIDARG;
    Close();
    _window      = window;
    _message     = message;
    _limits      = limits;
    _initialized = true;
    return S_OK;
}

HRESULT MultiFolderFilesService::SetRoots(std::span<const std::filesystem::path> roots) noexcept
try
{
    if (! _initialized)
        return E_UNEXPECTED;
    if (roots.size() > MaximumFilesRoots)
        return HRESULT_FROM_WIN32(ERROR_TOO_MANY_NAMES);

    struct StagedSlot
    {
        bool fromExisting{};
        std::size_t index{};
    };
    std::vector<std::filesystem::path> seenRoots;
    seenRoots.reserve(roots.size());
    std::vector<std::unique_ptr<WorkerEntry>> stagedNew;
    std::vector<StagedSlot> plan;
    plan.reserve(roots.size());

    for (const auto& rawRoot : roots)
    {
        if (rawRoot.empty() || ! rawRoot.is_absolute())
            continue;
        const auto norm = NormalizeRootPath(rawRoot);
        if (std::ranges::any_of(seenRoots, [&](const std::filesystem::path& existing) { return EqualNormalizedRootPath(existing, norm); }))
            continue;
        seenRoots.push_back(norm);

        std::optional<std::size_t> reuseIndex;
        for (std::size_t i = 0u; i < _entries.size(); ++i)
        {
            const auto& entry = _entries[i];
            if (entry && EqualNormalizedRootPath(entry->root, norm) && entry->service && entry->service->IsActive())
            {
                reuseIndex = i;
                break;
            }
        }
        if (reuseIndex)
        {
            plan.push_back({true, *reuseIndex});
            continue;
        }
        auto svc             = std::make_unique<FilesService>();
        const HRESULT initHr = svc->Initialize(_window, _message, _limits);
        if (FAILED(initHr))
            return initHr;
        const HRESULT setHr = svc->SetRoot(norm);
        if (FAILED(setHr))
            return setHr;
        auto entry     = std::make_unique<WorkerEntry>();
        entry->root    = norm;
        entry->service = std::move(svc);
        plan.push_back({false, stagedNew.size()});
        stagedNew.push_back(std::move(entry));
    }

    std::vector<std::unique_ptr<WorkerEntry>> next;
    next.reserve(plan.size());
    for (const auto& slot : plan)
    {
        if (slot.fromExisting)
            next.push_back(std::move(_entries[slot.index]));
        else
            next.push_back(std::move(stagedNew[slot.index]));
    }
    for (auto& oldEntry : _entries)
        if (oldEntry && oldEntry->service)
            oldEntry->service->Close();
    _entries = std::move(next);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MultiFolderFilesService::Refresh(const std::filesystem::path& root) noexcept
{
    const auto norm = NormalizeRootPath(root);
    for (auto& entry : _entries)
    {
        if (entry && EqualNormalizedRootPath(entry->root, norm) && entry->service)
            return entry->service->Refresh();
    }
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

HRESULT MultiFolderFilesService::RefreshAll() noexcept
{
    HRESULT firstError = S_OK;
    for (auto& entry : _entries)
    {
        if (entry && entry->service)
        {
            const HRESULT hr = entry->service->Refresh();
            if (FAILED(hr) && SUCCEEDED(firstError))
                firstError = hr;
        }
    }
    return firstError;
}

std::vector<FilesSnapshot> MultiFolderFilesService::TakeResults()
{
    std::vector<FilesSnapshot> results;
    for (auto& entry : _entries)
    {
        if (entry && entry->service)
        {
            if (auto snap = entry->service->TakeResult())
                results.push_back(std::move(*snap));
        }
    }
    return results;
}

bool MultiFolderFilesService::Busy() const noexcept
{
    return std::ranges::any_of(_entries, [](const std::unique_ptr<WorkerEntry>& entry) { return entry && entry->service && entry->service->Busy(); });
}

std::size_t MultiFolderFilesService::RootCount() const noexcept
{
    return _entries.size();
}

void MultiFolderFilesService::Close() noexcept
{
    for (auto& entry : _entries)
        if (entry && entry->service)
            entry->service->Close();
    _entries.clear();
    _window      = nullptr;
    _message     = 0;
    _initialized = false;
}
} // namespace RedTilde
