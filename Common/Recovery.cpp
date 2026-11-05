#include "Recovery.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <combaseapi.h>
#include <format>
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
wil::unique_handle Lease(const std::filesystem::path &root, std::wstring_view id)
{
    const auto path = root / (L"session-" + std::wstring(id) + L".lease");
    return wil::unique_handle(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr));
}
HRESULT Remove(const std::filesystem::path &path) noexcept
{
    if (DeleteFileW(path.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND)
        return S_OK;
    return HRESULT_FROM_WIN32(GetLastError());
}
} // namespace

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
    std::filesystem::path root;
    std::wstring id;
    HWND window{};
    UINT message{};
    std::optional<RecoverySnapshot> pending;
    std::wstring retire;
    std::vector<std::wstring> discards;
    std::vector<std::wstring> retired;
    std::vector<RecoveryCandidate> candidates;
    bool changed{}, immediate{}, stopping{}, ready{}, settled{}, finished{};
    HRESULT status = S_OK;
    HRESULT snapshotStatus = S_OK;
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
            if (++scanned > 512)
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
            auto lease = Lease(self->root, self->id);
            if (!lease)
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
            self->Scan();
            {
                std::lock_guard lock(self->mutex);
                self->ready = true;
                self->settled = !self->changed;
                self->Notify();
            }
            for (;;)
            {
                (void)WaitForSingleObject(self->wake.get(), INFINITE);
                // A quiet-time wait, never a periodic UI timer or a busy loop.
                for (;;)
                {
                    bool now{};
                    {
                        std::lock_guard lock(self->mutex);
                        now = self->immediate || self->stopping || !self->discards.empty();
                    }
                    if (now || WaitForSingleObject(self->wake.get(), 750) == WAIT_TIMEOUT)
                        break;
                }
                std::optional<RecoverySnapshot> snapshot;
                std::wstring retire;
                std::vector<std::wstring> discards;
                bool changed{}, stopping{};
                {
                    std::lock_guard lock(self->mutex);
                    snapshot = std::move(self->pending);
                    self->pending.reset();
                    retire = std::move(self->retire);
                    self->retire.clear();
                    discards.swap(self->discards);
                    changed = std::exchange(self->changed, false);
                    self->immediate = false;
                    stopping = self->stopping;
                }
                result = S_OK;
                std::optional<HRESULT> snapshotOutcome;
                if (changed)
                {
                    result = snapshot ? WriteBytesAtomic(SnapshotPath(self->root, self->id), SerializeRecovery(*snapshot))
                                      : Remove(SnapshotPath(self->root, self->id));
                    snapshotOutcome = result;
                    // Retire a restored candidate only after its replacement is durable.
                    if (SUCCEEDED(result) && !retire.empty())
                        discards.push_back(std::move(retire));
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
                    if (snapshotOutcome)
                        self->snapshotStatus = *snapshotOutcome;
                    self->status = FAILED(self->snapshotStatus) ? self->snapshotStatus : result;
                    self->settled = !self->changed && self->discards.empty();
                    self->Notify();
                    if (stopping && self->settled)
                        break;
                    if (!self->settled)
                        SetEvent(self->wake.get());
                }
            }
            lease.reset();
            (void)Remove(self->root / (L"session-" + self->id + L".lease"));
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
        self->status = FAILED(result) ? result : self->snapshotStatus;
        self->ready = self->settled = self->finished = true;
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
    GUID guid{};
    const HRESULT generated = CoCreateGuid(&guid);
    if (FAILED(generated))
        return generated;
    const auto bytes = std::as_bytes(std::span(&guid, 1));
    for (const auto value : bytes)
        staged->id += std::format(L"{:02x}", static_cast<unsigned>(value));
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!staged->wake)
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
try
{
    if (!state || (!retire.empty() && !ValidId(retire)))
        return E_INVALIDARG;
    if (snapshot)
        Validate(*snapshot);
    std::lock_guard lock(state->mutex);
    if (state->stopping || state->finished)
        return E_ABORT;
    state->pending = std::move(snapshot);
    if (!retire.empty())
        state->retire = std::move(retire);
    state->changed = true;
    state->settled = false;
    state->immediate = state->immediate || immediate;
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
