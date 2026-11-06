#include "Recovery.h"
#include "MonitorDiagnostics.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <combaseapi.h>
#include <format>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
constexpr std::string_view Magic = "RedTilde.Recovery/1\n";
constexpr std::size_t MaximumCandidates = 64;
constexpr std::size_t MaximumScannedEntries = 512;
HRESULT CreateRecoveryId(std::wstring &id)
{
    GUID guid{};
    const auto result = CoCreateGuid(&guid);
    if (FAILED(result))
        return result;
    std::wstring staged;
    staged.reserve(32u);
    for (const auto value : std::as_bytes(std::span(&guid, 1)))
        staged += std::format(L"{:02x}", static_cast<unsigned>(value));
    id.swap(staged);
    return S_OK;
}
bool ValidId(std::wstring_view id) noexcept
{
    return id.size() == 32 && std::ranges::all_of(id, [](wchar_t c) { return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f'); });
}
void Validate(const RecoverySnapshot &value)
{
    if (value.text.size() > DxUi::RichText::Document::MaximumUnits || value.originalPath.native().size() > 1024 ||
        !DxUi::RichText::IsValidUtf16(value.text) || !DxUi::RichText::IsValidUtf16(value.originalPath.native()) ||
        (!value.originalPath.empty() && !value.originalPath.is_absolute()) || value.selection.anchor > value.text.size() ||
        value.selection.caret > value.text.size())
        throw std::invalid_argument("Invalid recovery snapshot");
    const auto boundary = [&](std::size_t at) { return at == 0 || at == value.text.size() || value.text[at] < 0xDC00 || value.text[at] > 0xDFFF; };
    if (!boundary(value.selection.anchor) || !boundary(value.selection.caret))
        throw std::invalid_argument("Invalid recovery selection");
}
std::filesystem::path SnapshotPath(const std::filesystem::path &root, std::wstring_view id)
{
    return root / (L"session-" + std::wstring(id) + L".rtd");
}
wil::unique_handle Lease(const std::filesystem::path &root, std::wstring_view id, bool createNew = false)
{
    const auto path = root / (L"session-" + std::wstring(id) + L".lease");
    return wil::unique_handle(
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, createNew ? CREATE_NEW : OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr));
}
HRESULT Remove(const std::filesystem::path &path) noexcept
{
    if (DeleteFileW(path.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND)
        return S_OK;
    return HRESULT_FROM_WIN32(GetLastError());
}
} // namespace

struct SaveRecoveryProtection::State
{
    State() = default;
    State(const State &) = delete;
    State &operator=(const State &) = delete;
    ~State()
    {
        lease.reset();
        if (ownsLease)
            (void)Remove(leasePath);
        // A failed/interrupted save deliberately retains the snapshot.
    }
    std::filesystem::path snapshot, leasePath;
    wil::unique_handle lease;
    bool ownsLease{};
};
SaveRecoveryProtection::SaveRecoveryProtection() = default;
SaveRecoveryProtection::~SaveRecoveryProtection() = default;
HRESULT SaveRecoveryProtection::Admit(const std::filesystem::path &dataDirectory, const RecoverySnapshot &snapshot) noexcept
try
{
    if (dataDirectory.empty() || !dataDirectory.is_absolute())
        return E_INVALIDARG;
    const auto bytes = SerializeRecovery(snapshot); // Validate before any filesystem mutation.
    const auto root = dataDirectory / L"Recovery";
    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    std::size_t scanned{};
    for (std::filesystem::directory_iterator entry(root, error), end; !error && entry != end; entry.increment(error))
        if (++scanned >= MaximumScannedEntries)
            return HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    auto staged = std::make_unique<State>();
    std::wstring id;
    const auto generated = CreateRecoveryId(id);
    if (FAILED(generated))
        return generated;
    staged->snapshot = SnapshotPath(root, id);
    staged->leasePath = root / (L"session-" + id + L".lease");
    staged->lease = Lease(root, id, true);
    if (!staged->lease)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->ownsLease = true;
    const auto attributes = GetFileAttributesW(staged->snapshot.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    const auto absent = GetLastError();
    if (absent != ERROR_FILE_NOT_FOUND)
        return HRESULT_FROM_WIN32(absent);
    const auto written = WriteBytesAtomic(staged->snapshot, bytes);
    if (FAILED(written))
        return written;
    state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::length_error &)
{
    return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
}
catch (const std::invalid_argument &)
{
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}
HRESULT SaveRecoveryProtection::Retire() noexcept
{
    if (!state)
        return S_OK;
    const auto result = Remove(state->snapshot);
    if (FAILED(result))
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Saved linked-file content; its independent recovery copy could not be retired.");
    state.reset();
    return result;
}

std::string SerializeRecovery(const RecoverySnapshot &snapshot)
{
    Validate(snapshot);
    const auto path = DxUi::RichText::EncodeUtf8(snapshot.originalPath.native());
    const auto text = DxUi::RichText::EncodeUtf8(snapshot.text);
    const auto payload = path + text;
    const auto metadata = std::format("{} {} {} {} {} {}", path.size(), text.size(), snapshot.bom ? 1 : 0, snapshot.selection.anchor,
                                      snapshot.selection.caret, snapshot.generation);
    return std::string(Magic) + metadata + std::format(" {}\n", ContentHash(metadata + "\n" + payload)) + payload;
}
HRESULT ParseRecovery(std::string_view bytes, RecoverySnapshot &snapshot) noexcept
try
{
    if (!bytes.starts_with(Magic))
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    bytes.remove_prefix(Magic.size());
    const auto end = bytes.find('\n');
    if (end == bytes.npos || end > 200)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    auto header = bytes.substr(0, end);
    const auto checksumAt = header.find_last_of(' ');
    if (checksumAt == header.npos)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto metadata = header.substr(0, checksumAt);
    std::array<std::uint64_t, 7> values{};
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        const auto last = index + 1 == values.size() ? header.size() : header.find(' ');
        if (last == header.npos)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        const auto parsed = std::from_chars(header.data(), header.data() + last, values[index]);
        if (parsed.ec != std::errc{} || parsed.ptr != header.data() + last)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        header.remove_prefix(last + (index + 1 == values.size() ? 0 : 1));
    }
    const auto payload = bytes.substr(end + 1);
    if (values[0] > 4096 || values[1] > 4 * 65536 || values[0] + values[1] != payload.size() || values[2] > 1 || values[3] > 65536 ||
        values[4] > 65536 || ContentHash(std::string(metadata) + "\n" + std::string(payload)) != values[6])
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    RecoverySnapshot staged;
    staged.originalPath = DxUi::RichText::DecodeUtf8(payload.substr(0, static_cast<std::size_t>(values[0])));
    staged.text = DxUi::RichText::DecodeUtf8(payload.substr(static_cast<std::size_t>(values[0])));
    staged.bom = values[2] != 0;
    staged.selection = {static_cast<std::size_t>(values[3]), static_cast<std::size_t>(values[4])};
    staged.generation = values[5];
    Validate(staged);
    snapshot = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::length_error &)
{
    // Encoded byte bounds do not imply decoded editor-unit bounds. Reject this
    // expected input failure at the noexcept boundary without publishing staged
    // output, allowing the scanner to preserve/quarantine the original bytes.
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}
catch (const std::invalid_argument &)
{
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct RecoveryService::State
{
    std::mutex mutex;
    wil::unique_event wake;
    wil::unique_event settlement;
    std::filesystem::path root;
    std::wstring id;
    HWND window{};
    UINT message{};
    struct PendingSnapshot
    {
        std::optional<RecoverySnapshot> snapshot;
        std::wstring retire;
        ULONGLONG first{}, latest{};
        bool immediate{};
    };
    std::map<unsigned, PendingSnapshot> pending;
    std::vector<std::wstring> discards;
    std::vector<std::wstring> retired;
    std::vector<RecoveryCandidate> candidates;
    bool stopping{}, ready{}, settled{}, finished{};
    HRESULT status = S_OK;
    std::array<HRESULT, RecoveryService::MaximumDocuments> snapshotStatuses{};
    [[nodiscard]] std::wstring DocumentId(unsigned document) const
    {
        if (!document)
            return id;
        unsigned tail{};
        for (const wchar_t digit : std::wstring_view(id).substr(24))
            tail = (tail << 4) | static_cast<unsigned>(digit <= L'9' ? digit - L'0' : digit - L'a' + 10);
        return id.substr(0, 24) + std::format(L"{:08x}", tail ^ document);
    }
    void Notify() const noexcept
    {
        if (window)
            (void)PostMessageW(window, message, 0, 0);
    }
    void Scan()
    {
        std::size_t scanned{};
        for (const auto &entry : std::filesystem::directory_iterator(root))
        {
            const auto name = entry.path().filename().native();
            if (!name.starts_with(L"session-") || !name.ends_with(L".rtd"))
                continue;
            if (++scanned > MaximumScannedEntries)
                break;
            const auto candidate = std::wstring_view(name).substr(8, name.size() - 12);
            if (!ValidId(candidate) || candidate == id || candidates.size() >= MaximumCandidates)
                continue;
            auto lease = Lease(root, candidate);
            if (!lease) // Another running session (or access error) retains ownership.
                continue;
            std::string bytes;
            RecoverySnapshot snapshot;
            const HRESULT read = ReadBytes(entry.path(), bytes);
            if (FAILED(read))
                continue;
            const HRESULT parsed = ParseRecovery(bytes, snapshot);
            if (FAILED(parsed))
            {
                auto quarantine = entry.path();
                quarantine += L".quarantine";
                (void)MoveFileW(entry.path().c_str(), quarantine.c_str()); // No overwrite, preserve unknown/corrupt bytes.
                continue;
            }
            std::lock_guard lock(mutex);
            candidates.push_back({std::wstring(candidate), std::move(snapshot)});
        }
    }
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void *context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        const std::unique_ptr<std::shared_ptr<State>> ownership(static_cast<std::shared_ptr<State> *>(context));
        const auto &self = *ownership;
        HRESULT result = S_OK;
        try
        {
            std::filesystem::create_directories(self->root);
            std::map<unsigned, wil::unique_handle> leases;
            leases.emplace(0u, Lease(self->root, self->id));
            if (!leases.at(0u))
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
            self->Scan();
            {
                std::lock_guard lock(self->mutex);
                self->ready = true;
                self->settled = self->pending.empty();
                if (self->settled)
                    SetEvent(self->settlement.get());
                self->Notify();
            }
            for (;;)
            {
                std::vector<std::pair<unsigned, PendingSnapshot>> due;
                std::vector<std::wstring> discards;
                DWORD delay = INFINITE;
                bool stopping{};
                {
                    std::lock_guard lock(self->mutex);
                    const auto now = GetTickCount64();
                    for (auto item = self->pending.begin(); item != self->pending.end();)
                    {
                        const auto deadline = std::min(item->second.latest + 750u, item->second.first + 5000u);
                        if (self->stopping || item->second.immediate || deadline <= now)
                        {
                            due.emplace_back(item->first, std::move(item->second));
                            item = self->pending.erase(item);
                        }
                        else
                        {
                            delay = std::min(delay, static_cast<DWORD>(deadline - now));
                            ++item;
                        }
                    }
                    discards.swap(self->discards);
                    stopping = self->stopping;
                }
                if (due.empty() && discards.empty() && !stopping)
                {
                    // Wait until an event or the earliest document deadline.
                    // Continuous edits cannot postpone a checkpoint indefinitely.
                    (void)WaitForSingleObject(self->wake.get(), delay);
                    continue;
                }
                result = S_OK;
                for (auto &[document, update] : due)
                {
                    const auto id = self->DocumentId(document);
                    auto &lease = leases[document];
                    if (!lease)
                        lease = Lease(self->root, id);
                    const HRESULT outcome = !lease            ? HRESULT_FROM_WIN32(GetLastError())
                                            : update.snapshot ? WriteBytesAtomic(SnapshotPath(self->root, id), SerializeRecovery(*update.snapshot))
                                                              : Remove(SnapshotPath(self->root, id));
                    {
                        std::lock_guard lock(self->mutex);
                        self->snapshotStatuses[document] = outcome;
                    }
                    // Retire a restored candidate only after its replacement is durable.
                    if (SUCCEEDED(outcome) && !update.retire.empty())
                        discards.push_back(std::move(update.retire));
                }
                for (const auto &candidate : discards)
                {
                    auto other = Lease(self->root, candidate);
                    const HRESULT removed = other ? Remove(SnapshotPath(self->root, candidate)) : HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
                    if (FAILED(removed))
                        result = removed;
                    else
                    {
                        other.reset();
                        (void)Remove(self->root / (L"session-" + candidate + L".lease"));
                        std::lock_guard lock(self->mutex);
                        if (std::ranges::find(self->retired, candidate) == self->retired.end() && self->retired.size() < MaximumCandidates)
                            self->retired.push_back(candidate);
                    }
                }
                {
                    std::lock_guard lock(self->mutex);
                    const auto failed = std::ranges::find_if(self->snapshotStatuses, [](HRESULT value) { return FAILED(value); });
                    self->status = failed != self->snapshotStatuses.end() ? *failed : result;
                    self->settled = self->pending.empty() && self->discards.empty();
                    if (self->settled)
                        SetEvent(self->settlement.get());
                    self->Notify();
                    if (stopping && self->settled)
                        break;
                }
            }
            for (auto &[document, lease] : leases)
            {
                lease.reset();
                (void)Remove(self->root / (L"session-" + self->DocumentId(document) + L".lease"));
            }
        }
        catch (const std::bad_alloc &)
        {
            result = E_OUTOFMEMORY;
        }
        catch (const std::system_error &error)
        {
            result = HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
        }
        catch (const std::invalid_argument &)
        {
            result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
        std::lock_guard lock(self->mutex);
        if (FAILED(result))
            self->status = result;
        self->ready = self->settled = self->finished = true;
        SetEvent(self->settlement.get());
        self->Notify();
    }
};
#pragma warning(pop)

RecoveryService::~RecoveryService()
{
    Close();
}
HRESULT RecoveryService::Initialize(std::filesystem::path directory, HWND window, UINT message) noexcept
try
{
    Close();
    auto staged = std::make_shared<State>();
    staged->root = std::move(directory) / L"Recovery";
    const HRESULT generated = CreateRecoveryId(staged->id);
    if (FAILED(generated))
        return generated;
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->settlement.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!staged->settlement)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window = window;
    staged->message = message;
    auto ownership = std::make_unique<std::shared_ptr<State>>(staged);
    if (!TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
HRESULT RecoveryService::Submit(std::optional<RecoverySnapshot> snapshot, bool immediate, std::wstring retire) noexcept
{
    return SubmitDocument(0u, std::move(snapshot), immediate, std::move(retire));
}
HRESULT RecoveryService::SubmitDocument(unsigned document, std::optional<RecoverySnapshot> snapshot, bool immediate, std::wstring retire) noexcept
try
{
    if (!state || document >= MaximumDocuments || (!retire.empty() && !ValidId(retire)))
        return E_INVALIDARG;
    if (snapshot)
        Validate(*snapshot);
    std::lock_guard lock(state->mutex);
    if (state->stopping || state->finished)
        return E_ABORT;
    const auto now = GetTickCount64();
    auto [item, inserted] = state->pending.try_emplace(document);
    auto &pending = item->second;
    if (inserted)
        pending.first = now;
    pending.latest = now;
    pending.snapshot = std::move(snapshot);
    if (!retire.empty())
        pending.retire = std::move(retire);
    state->settled = false;
    ResetEvent(state->settlement.get());
    pending.immediate = pending.immediate || immediate;
    SetEvent(state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::invalid_argument &)
{
    return E_INVALIDARG;
}
HRESULT RecoveryService::Discard(std::wstring id) noexcept
try
{
    if (!state || !ValidId(id))
        return E_INVALIDARG;
    std::lock_guard lock(state->mutex);
    if (state->stopping || state->finished || state->discards.size() + state->retired.size() >= MaximumCandidates)
        return E_ABORT;
    state->discards.push_back(std::move(id));
    state->settled = false;
    ResetEvent(state->settlement.get());
    SetEvent(state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
std::vector<RecoveryCandidate> RecoveryService::TakeCandidates()
{
    if (!state)
        return {};
    std::lock_guard lock(state->mutex);
    return std::exchange(state->candidates, {});
}
std::vector<std::wstring> RecoveryService::TakeRetired()
{
    if (!state)
        return {};
    std::lock_guard lock(state->mutex);
    return std::exchange(state->retired, {});
}
bool RecoveryService::Ready() const noexcept
{
    if (!state)
        return true;
    std::lock_guard lock(state->mutex);
    return state->ready;
}
bool RecoveryService::Settled() const noexcept
{
    if (!state)
        return true;
    std::lock_guard lock(state->mutex);
    return state->settled;
}
bool RecoveryService::Finished() const noexcept
{
    if (!state)
        return true;
    std::lock_guard lock(state->mutex);
    return state->finished;
}
HRESULT RecoveryService::Status() const noexcept
{
    if (!state)
        return S_OK;
    std::lock_guard lock(state->mutex);
    return state->status;
}
HRESULT RecoveryService::WaitForSettlement(DWORD milliseconds) const noexcept
{
    if (!state)
        return S_OK;
    const DWORD result = WaitForSingleObject(state->settlement.get(), milliseconds);
    if (result == WAIT_OBJECT_0)
        return Status();
    return HRESULT_FROM_WIN32(result == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
}
void RecoveryService::Stop() noexcept
{
    if (state)
    {
        std::lock_guard lock(state->mutex);
        state->stopping = true;
        SetEvent(state->wake.get());
    }
}
void RecoveryService::Close() noexcept
{
    Stop();
    if (state)
    {
        std::lock_guard lock(state->mutex);
        state->window = nullptr;
    }
    state.reset();
}
} // namespace RedTilde
