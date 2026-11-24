#include "Recovery.h"
#include "MonitorDiagnostics.h"
#include "ProcessWork.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <combaseapi.h>
#include <cstdlib>
#include <format>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <wil/resource.h>
#include <yyjson.h>

namespace RedTilde
{
namespace
{
constexpr std::string_view Magic            = "RedTilde.Recovery/1\n";
constexpr std::string_view MagicV2          = "RedTilde.Recovery/2\n";
constexpr std::size_t MaximumCandidates     = 64;
constexpr std::size_t MaximumScannedEntries = 512;
constexpr std::size_t MaximumMetadataBytes  = 64u * 1024u;
using unique_yyjson_doc                     = wil::unique_any<yyjson_doc*, decltype(&yyjson_doc_free), yyjson_doc_free>;
using unique_yyjson_mut_doc                 = wil::unique_any<yyjson_mut_doc*, decltype(&yyjson_mut_doc_free), yyjson_mut_doc_free>;
using unique_yyjson_text                    = wil::unique_any<char*, decltype(&::free), ::free>;
HRESULT CreateRecoveryId(std::wstring& id)
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
bool ValidSnapshot(const RecoverySnapshot& value) noexcept
{
    if (value.text.size() > DxUi::RichText::Document::MaximumUnits || value.originalPath.native().size() > 1024 || ! DxUi::RichText::IsValidUtf16(value.text) ||
        ! DxUi::RichText::IsValidUtf16(value.originalPath.native()) || (! value.originalPath.empty() && ! value.originalPath.is_absolute()) ||
        value.selection.anchor > value.text.size() || value.selection.caret > value.text.size())
        return false;
    const auto validOriginField = [](std::wstring_view field, std::size_t maxLen) noexcept
    { return field.size() <= maxLen && DxUi::RichText::IsValidUtf16(field); };
    if (! validOriginField(value.origin.owner, 128) || ! validOriginField(value.origin.repo, 128) || ! validOriginField(value.origin.branch, 256) ||
        ! validOriginField(value.origin.relativePath, 1024) || ! validOriginField(value.origin.blobSha, 128) || ! validOriginField(value.origin.htmlUrl, 1024))
        return false;
    const auto boundary = [&](std::size_t at) { return at == 0 || at == value.text.size() || value.text[at] < 0xDC00 || value.text[at] > 0xDFFF; };
    return boundary(value.selection.anchor) && boundary(value.selection.caret);
}
bool NeedsV2Envelope(const RecoverySnapshot& snapshot) noexcept
{
    return snapshot.kind != RecoverySnapshotKind::CrashRecovery || snapshot.workspaceOrder != 0u || snapshot.wasActive ||
           snapshot.savedBaselineStamp.has_value() || snapshot.origin.kind != DocumentOriginInfo::Kind::Local || ! snapshot.origin.owner.empty() ||
           ! snapshot.origin.repo.empty() || ! snapshot.origin.relativePath.empty();
}
std::filesystem::path SnapshotPath(const std::filesystem::path& root, std::wstring_view id)
{
    return root / (L"session-" + std::wstring(id) + L".rtd");
}
wil::unique_handle Lease(const std::filesystem::path& root, std::wstring_view id, bool createNew = false)
{
    const auto path = root / (L"session-" + std::wstring(id) + L".lease");
    return wil::unique_handle(
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, createNew ? CREATE_NEW : OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr));
}
HRESULT Remove(const std::filesystem::path& path) noexcept
{
    if (DeleteFileW(path.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND)
        return S_OK;
    return HRESULT_FROM_WIN32(GetLastError());
}
} // namespace

struct SaveRecoveryProtection::State
{
    State()                        = default;
    State(const State&)            = delete;
    State& operator=(const State&) = delete;
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
SaveRecoveryProtection::SaveRecoveryProtection()  = default;
SaveRecoveryProtection::~SaveRecoveryProtection() = default;
HRESULT SaveRecoveryProtection::Admit(const std::filesystem::path& dataDirectory, const RecoverySnapshot& snapshot) noexcept
try
{
    if (dataDirectory.empty() || ! dataDirectory.is_absolute())
        return E_INVALIDARG;
    std::string bytes;
    const HRESULT serialized = TrySerializeRecovery(snapshot, bytes); // Validate before any filesystem mutation.
    if (FAILED(serialized))
        return serialized;
    const auto root = dataDirectory / L"Recovery";
    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    std::size_t scanned{};
    for (std::filesystem::directory_iterator entry(root, error), end; ! error && entry != end; entry.increment(error))
        if (++scanned >= MaximumScannedEntries)
            return HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    auto staged = std::make_unique<State>();
    std::wstring id;
    const auto generated = CreateRecoveryId(id);
    if (FAILED(generated))
        return generated;
    staged->snapshot  = SnapshotPath(root, id);
    staged->leasePath = root / (L"session-" + id + L".lease");
    staged->lease     = Lease(root, id, true);
    if (! staged->lease)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->ownsLease     = true;
    const auto attributes = GetFileAttributesW(staged->snapshot.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    const auto absent = GetLastError();
    if (absent != ERROR_FILE_NOT_FOUND)
        return HRESULT_FROM_WIN32(absent);
    const auto written = WriteBytesAtomic(staged->snapshot, bytes);
    if (FAILED(written))
        return written;
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::length_error&)
{
    return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
}
catch (const std::invalid_argument&)
{
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}
catch (const std::filesystem::filesystem_error& error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}
HRESULT SaveRecoveryProtection::Retire() noexcept
{
    if (! _state)
        return S_OK;
    const auto result = Remove(_state->snapshot);
    if (FAILED(result))
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Saved linked-file content; its independent recovery copy could not be retired.");
    _state.reset();
    return result;
}

std::string SerializeRecovery(const RecoverySnapshot& snapshot)
{
    std::string output;
    const HRESULT result = TrySerializeRecovery(snapshot, output);
    if (FAILED(result))
        DxUi::Failure::FailContract(result);
    return output;
}
HRESULT TrySerializeRecovery(const RecoverySnapshot& snapshot, std::string& output) noexcept
try
{
    if (! ValidSnapshot(snapshot))
        return E_INVALIDARG;
    std::string path, text;
    if (DxUi::RichText::TryEncodeUtf8(snapshot.originalPath.native(), path) != DxUi::RichText::SourceResult::Success ||
        DxUi::RichText::TryEncodeUtf8(snapshot.text, text) != DxUi::RichText::SourceResult::Success)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    if (! NeedsV2Envelope(snapshot))
    {
        const auto payload  = path + text;
        const auto metadata = std::format(
            "{} {} {} {} {} {}", path.size(), text.size(), snapshot.bom ? 1 : 0, snapshot.selection.anchor, snapshot.selection.caret, snapshot.generation);
        auto staged = std::string(Magic) + metadata + std::format(" {}\n", ContentHash(metadata + "\n" + payload)) + payload;
        output.swap(staged);
        return S_OK;
    }

    const unique_yyjson_mut_doc doc(yyjson_mut_doc_new(nullptr));
    if (! doc)
        DxUi::Failure::FailAllocation();
    auto* const root = yyjson_mut_obj(doc.get());
    if (! root)
        DxUi::Failure::FailAllocation();
    yyjson_mut_doc_set_root(doc.get(), root);

    const auto checkedAddVal = [&](yyjson_mut_val* parent, const char* key, yyjson_mut_val* val) noexcept
    {
        auto* const keyVal = yyjson_mut_strcpy(doc.get(), key);
        if (! keyVal || ! val || ! yyjson_mut_obj_put(parent, keyVal, val))
            DxUi::Failure::FailAllocation();
    };
    const auto putStr = [&](yyjson_mut_val* parent, const char* key, std::string_view val) noexcept
    { checkedAddVal(parent, key, yyjson_mut_strncpy(doc.get(), val.data(), val.size())); };
    const auto putWide = [&](yyjson_mut_val* parent, const char* key, std::wstring_view wide)
    {
        std::string utf8;
        if (DxUi::RichText::TryEncodeUtf8(wide, utf8) != DxUi::RichText::SourceResult::Success)
            return false;
        putStr(parent, key, utf8);
        return true;
    };

    checkedAddVal(root, "kind", yyjson_mut_uint(doc.get(), static_cast<std::uint64_t>(snapshot.kind)));
    checkedAddVal(root, "order", yyjson_mut_uint(doc.get(), snapshot.workspaceOrder));
    checkedAddVal(root, "active", yyjson_mut_bool(doc.get(), snapshot.wasActive));
    checkedAddVal(root, "bom", yyjson_mut_bool(doc.get(), snapshot.bom));
    checkedAddVal(root, "anchor", yyjson_mut_uint(doc.get(), snapshot.selection.anchor));
    checkedAddVal(root, "caret", yyjson_mut_uint(doc.get(), snapshot.selection.caret));
    checkedAddVal(root, "generation", yyjson_mut_uint(doc.get(), snapshot.generation));
    putStr(root, "path", path);

    if (snapshot.savedBaselineStamp)
    {
        auto* const stampObj = yyjson_mut_obj(doc.get());
        if (! stampObj)
            DxUi::Failure::FailAllocation();
        const auto& s = snapshot.savedBaselineStamp.value();
        checkedAddVal(stampObj, "exists", yyjson_mut_bool(doc.get(), s.exists));
        checkedAddVal(stampObj, "size", yyjson_mut_uint(doc.get(), s.size));
        checkedAddVal(stampObj, "modified", yyjson_mut_uint(doc.get(), s.modified));
        checkedAddVal(stampObj, "hash", yyjson_mut_uint(doc.get(), s.hash));
        checkedAddVal(stampObj, "volume", yyjson_mut_uint(doc.get(), s.volume));
        checkedAddVal(stampObj, "fileIndex", yyjson_mut_uint(doc.get(), s.fileIndex));
        checkedAddVal(stampObj, "fileIndexHigh", yyjson_mut_uint(doc.get(), s.fileIndexHigh));
        checkedAddVal(root, "stamp", stampObj);
    }
    if (snapshot.origin.kind != DocumentOriginInfo::Kind::Local)
    {
        auto* const originObj = yyjson_mut_obj(doc.get());
        if (! originObj)
            DxUi::Failure::FailAllocation();
        checkedAddVal(originObj, "kind", yyjson_mut_uint(doc.get(), static_cast<std::uint64_t>(snapshot.origin.kind)));
        if (! putWide(originObj, "owner", snapshot.origin.owner) || ! putWide(originObj, "repo", snapshot.origin.repo) ||
            ! putWide(originObj, "branch", snapshot.origin.branch) || ! putWide(originObj, "relativePath", snapshot.origin.relativePath) ||
            ! putWide(originObj, "blobSha", snapshot.origin.blobSha) || ! putWide(originObj, "htmlUrl", snapshot.origin.htmlUrl))
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        checkedAddVal(root, "origin", originObj);
    }

    std::size_t metaLen{};
    yyjson_write_err writeErr{};
    const unique_yyjson_text metaText(yyjson_mut_write_opts(doc.get(), 0, nullptr, &metaLen, &writeErr));
    if (! metaText)
    {
        if (writeErr.code == YYJSON_WRITE_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    if (metaLen > MaximumMetadataBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::string metaView(metaText.get(), metaLen);
    const auto payload = metaView + text;
    const auto header  = std::format("{} {}", metaView.size(), text.size());
    auto staged        = std::string(MagicV2) + header + std::format(" {}\n", ContentHash(header + "\n" + payload)) + payload;
    output.swap(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::exception&)
{
    // This noexcept serialization boundary has no expected throwing operation; refusal is checked above.
    DxUi::Failure::FailUnexpected();
}
HRESULT ParseRecovery(std::string_view bytes, RecoverySnapshot& snapshot) noexcept
try
{
    const bool isV1 = bytes.starts_with(Magic);
    const bool isV2 = ! isV1 && bytes.starts_with(MagicV2);
    if (! isV1 && ! isV2)
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    bytes.remove_prefix(isV1 ? Magic.size() : MagicV2.size());
    const auto end = bytes.find('\n');
    if (end == bytes.npos || end > 200)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    auto header           = bytes.substr(0, end);
    const auto checksumAt = header.find_last_of(' ');
    if (checksumAt == header.npos)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto metadata = header.substr(0, checksumAt);
    const auto payload  = bytes.substr(end + 1);

    if (isV1)
    {
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
        if (values[0] > 4096 || values[1] > 4 * 65536 || values[0] + values[1] != payload.size() || values[2] > 1 || values[3] > 65536 || values[4] > 65536 ||
            ContentHash(std::string(metadata) + "\n" + std::string(payload)) != values[6])
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        RecoverySnapshot staged;
        std::wstring path;
        if (DxUi::RichText::TryDecodeUtf8(payload.substr(0, static_cast<std::size_t>(values[0])), path) != DxUi::RichText::SourceResult::Success ||
            DxUi::RichText::TryDecodeUtf8(payload.substr(static_cast<std::size_t>(values[0])), staged.text) != DxUi::RichText::SourceResult::Success)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        staged.originalPath = std::move(path);
        staged.bom          = values[2] != 0;
        staged.selection    = {static_cast<std::size_t>(values[3]), static_cast<std::size_t>(values[4])};
        staged.generation   = values[5];
        if (! ValidSnapshot(staged))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        snapshot = std::move(staged);
        return S_OK;
    }

    std::array<std::uint64_t, 3> v2Header{};
    for (std::size_t index = 0; index < v2Header.size(); ++index)
    {
        const auto last = index + 1 == v2Header.size() ? header.size() : header.find(' ');
        if (last == header.npos)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        const auto parsed = std::from_chars(header.data(), header.data() + last, v2Header[index]);
        if (parsed.ec != std::errc{} || parsed.ptr != header.data() + last)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        header.remove_prefix(last + (index + 1 == v2Header.size() ? 0 : 1));
    }
    if (v2Header[0] > MaximumMetadataBytes || v2Header[1] > 4u * 65536u || v2Header[0] + v2Header[1] != payload.size() ||
        ContentHash(std::string(metadata) + "\n" + std::string(payload)) != v2Header[2])
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    const auto metaSlice = payload.substr(0, static_cast<std::size_t>(v2Header[0]));
    const auto textSlice = payload.substr(static_cast<std::size_t>(v2Header[0]));
    yyjson_read_err readErr{};
    const unique_yyjson_doc jsonDoc(yyjson_read_opts(const_cast<char*>(metaSlice.data()), metaSlice.size(), 0, nullptr, &readErr));
    if (! jsonDoc)
    {
        if (readErr.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    auto* const root = yyjson_doc_get_root(jsonDoc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    RecoverySnapshot staged;
    auto* const kindVal = yyjson_obj_get(root, "kind");
    if (! kindVal || ! yyjson_is_uint(kindVal) || yyjson_get_uint(kindVal) > 1u)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    staged.kind = static_cast<RecoverySnapshotKind>(yyjson_get_uint(kindVal));

    if (auto* const orderVal = yyjson_obj_get(root, "order"); orderVal && yyjson_is_uint(orderVal) && yyjson_get_uint(orderVal) <= 1024u)
        staged.workspaceOrder = static_cast<std::uint32_t>(yyjson_get_uint(orderVal));
    if (auto* const activeVal = yyjson_obj_get(root, "active"); activeVal && yyjson_is_bool(activeVal))
        staged.wasActive = yyjson_get_bool(activeVal);
    if (auto* const bomVal = yyjson_obj_get(root, "bom"); bomVal && yyjson_is_bool(bomVal))
        staged.bom = yyjson_get_bool(bomVal);
    if (auto* const genVal = yyjson_obj_get(root, "generation"); genVal && yyjson_is_uint(genVal))
        staged.generation = yyjson_get_uint(genVal);
    auto* const anchorVal = yyjson_obj_get(root, "anchor");
    auto* const caretVal  = yyjson_obj_get(root, "caret");
    if (! anchorVal || ! caretVal || ! yyjson_is_uint(anchorVal) || ! yyjson_is_uint(caretVal) || yyjson_get_uint(anchorVal) > 65536u ||
        yyjson_get_uint(caretVal) > 65536u)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    staged.selection = {static_cast<std::size_t>(yyjson_get_uint(anchorVal)), static_cast<std::size_t>(yyjson_get_uint(caretVal))};

    const auto decodeField = [](yyjson_val* obj, const char* key, std::wstring& out) -> bool
    {
        auto* const val = yyjson_obj_get(obj, key);
        if (! val)
            return true;
        if (! yyjson_is_str(val))
            return false;
        const std::string_view slice(yyjson_get_str(val), yyjson_get_len(val));
        return DxUi::RichText::TryDecodeUtf8(slice, out) == DxUi::RichText::SourceResult::Success;
    };
    std::wstring path;
    if (! decodeField(root, "path", path))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    staged.originalPath = std::move(path);

    if (auto* const stampObj = yyjson_obj_get(root, "stamp"))
    {
        if (! yyjson_is_obj(stampObj))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        FileStamp s;
        if (auto* const v = yyjson_obj_get(stampObj, "exists"); v && yyjson_is_bool(v))
            s.exists = yyjson_get_bool(v);
        if (auto* const v = yyjson_obj_get(stampObj, "size"); v && yyjson_is_uint(v))
            s.size = yyjson_get_uint(v);
        if (auto* const v = yyjson_obj_get(stampObj, "modified"); v && yyjson_is_uint(v))
            s.modified = yyjson_get_uint(v);
        if (auto* const v = yyjson_obj_get(stampObj, "hash"); v && yyjson_is_uint(v))
            s.hash = yyjson_get_uint(v);
        if (auto* const v = yyjson_obj_get(stampObj, "volume"); v && yyjson_is_uint(v))
            s.volume = yyjson_get_uint(v);
        if (auto* const v = yyjson_obj_get(stampObj, "fileIndex"); v && yyjson_is_uint(v))
            s.fileIndex = yyjson_get_uint(v);
        if (auto* const v = yyjson_obj_get(stampObj, "fileIndexHigh"); v && yyjson_is_uint(v))
            s.fileIndexHigh = yyjson_get_uint(v);
        staged.savedBaselineStamp = s;
    }
    if (auto* const originObj = yyjson_obj_get(root, "origin"))
    {
        if (! yyjson_is_obj(originObj))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        auto* const okVal = yyjson_obj_get(originObj, "kind");
        if (! okVal || ! yyjson_is_uint(okVal) || yyjson_get_uint(okVal) > 1u)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        staged.origin.kind = static_cast<DocumentOriginInfo::Kind>(yyjson_get_uint(okVal));
        if (! decodeField(originObj, "owner", staged.origin.owner) || ! decodeField(originObj, "repo", staged.origin.repo) ||
            ! decodeField(originObj, "branch", staged.origin.branch) || ! decodeField(originObj, "relativePath", staged.origin.relativePath) ||
            ! decodeField(originObj, "blobSha", staged.origin.blobSha) || ! decodeField(originObj, "htmlUrl", staged.origin.htmlUrl))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if (DxUi::RichText::TryDecodeUtf8(textSlice, staged.text) != DxUi::RichText::SourceResult::Success || ! ValidSnapshot(staged))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    snapshot = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::length_error&)
{
    // Encoded byte bounds do not imply decoded editor-unit bounds. Reject this
    // expected input failure at the noexcept boundary without publishing staged
    // output, allowing the scanner to preserve/quarantine the original bytes.
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}
catch (const std::invalid_argument&)
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
    enum class WorkerPhase : std::uint8_t
    {
        Running,
        Stopping,
        Finished
    };
    std::map<unsigned, PendingSnapshot> pending;
    std::vector<std::wstring> discards;
    std::vector<std::wstring> retired;
    std::vector<std::wstring> failedDiscards;
    std::vector<RecoveryCandidate> candidates;
    RecoveryScanStatistics scanStatistics;
    WorkerPhase workerPhase{WorkerPhase::Running};
    bool scanComplete{};
    bool writingBatch{};
    HRESULT status = S_OK;
    std::array<HRESULT, RecoveryService::MaximumDocuments> snapshotStatuses{};
#if defined(REDTILDE_ENABLE_TESTS)
    std::shared_ptr<RecoveryService::TestGate> scanTestGate;
    std::shared_ptr<RecoveryService::TestGate> postScanTestGate;
    std::shared_ptr<RecoveryService::TestGate> batchTestGate;
    std::size_t discardAdmissions{};
#endif
    [[nodiscard]] bool SettledLocked() const noexcept
    {
        return (workerPhase == WorkerPhase::Finished) || (scanComplete && pending.empty() && discards.empty() && ! writingBatch);
    }
    void UpdateSettlementLocked() noexcept
    {
        if (SettledLocked())
            (void)SetEvent(settlement.get());
        else
            (void)ResetEvent(settlement.get());
    }
    [[nodiscard]] std::wstring DocumentId(unsigned document) const
    {
        if (! document)
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
        RecoveryScanStatistics observed;
        const auto publishStatistics = wil::scope_exit([&]() noexcept
        {
            std::lock_guard lock(mutex);
            scanStatistics = observed;
        });
        for (const auto& entry : std::filesystem::directory_iterator(root))
        {
            if (observed.examinedEntries == MaximumScannedEntries)
            {
                observed.entryLimitReached = true;
                break;
            }
            ++observed.examinedEntries;
            const auto name = entry.path().filename().native();
            if (! name.starts_with(L"session-") || ! name.ends_with(L".rtd"))
                continue;
            const auto candidate      = std::wstring_view(name).substr(8, name.size() - 12);
            const bool candidatesFull = [&]
            {
                std::lock_guard lock(mutex);
                return candidates.size() >= MaximumCandidates;
            }();
            if (! ValidId(candidate) || candidate == id || candidatesFull)
                continue;
            auto lease = Lease(root, candidate);
            if (! lease) // Another running session (or access error) retains ownership.
                continue;
            std::string bytes;
            RecoverySnapshot snapshot;
            const HRESULT read = ReadBytes(entry.path(), bytes);
            if (FAILED(read))
            {
                lease.reset();
                (void)Remove(root / (L"session-" + std::wstring(candidate) + L".lease"));
                continue;
            }
            const HRESULT parsed = ParseRecovery(bytes, snapshot);
            if (FAILED(parsed))
            {
                auto quarantine = entry.path();
                quarantine += L".quarantine";
                (void)MoveFileW(entry.path().c_str(), quarantine.c_str()); // No overwrite, preserve unknown/corrupt bytes.
                lease.reset();
                (void)Remove(root / (L"session-" + std::wstring(candidate) + L".lease"));
                continue;
            }
            {
                std::lock_guard lock(mutex);
                candidates.push_back({std::wstring(candidate), std::move(snapshot)});
            }
        }
#if defined(REDTILDE_ENABLE_TESTS)
        std::shared_ptr<RecoveryService::TestGate> gate;
        {
            std::lock_guard lock(mutex);
            gate = std::exchange(scanTestGate, {});
        }
        if (gate)
        {
            (void)SetEvent(gate->entered.get());
            (void)WaitForSingleObject(gate->proceed.get(), 30000);
            (void)SetEvent(gate->finished.get());
        }
#endif
    }
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease processLease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
        HRESULT result   = S_OK;
        try
        {
            std::filesystem::create_directories(self->root);
            std::map<unsigned, wil::unique_handle> leases;
            leases.emplace(0u, Lease(self->root, self->id));
            if (! leases.at(0u))
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
            self->Scan();
            {
                std::lock_guard lock(self->mutex);
                self->scanComplete = true;
                self->UpdateSettlementLocked();
                self->Notify();
            }
#if defined(REDTILDE_ENABLE_TESTS)
            {
                std::shared_ptr<RecoveryService::TestGate> gate;
                {
                    std::lock_guard lock(self->mutex);
                    gate = std::exchange(self->postScanTestGate, {});
                }
                if (gate)
                {
                    (void)SetEvent(gate->entered.get());
                    (void)WaitForSingleObject(gate->proceed.get(), 30000);
                    (void)SetEvent(gate->finished.get());
                }
            }
#endif
            for (;;)
            {
                std::vector<std::pair<unsigned, PendingSnapshot>> due;
                std::vector<std::wstring> discards;
                DWORD delay = INFINITE;
                bool stopping{};
#if defined(REDTILDE_ENABLE_TESTS)
                std::shared_ptr<RecoveryService::TestGate> batchGate;
#endif
                {
                    std::lock_guard lock(self->mutex);
                    stopping       = self->workerPhase == WorkerPhase::Stopping;
                    const auto now = GetTickCount64();
                    for (auto item = self->pending.begin(); item != self->pending.end();)
                    {
                        const auto deadline = std::min(item->second.latest + 750u, item->second.first + 5000u);
                        if (stopping || item->second.immediate || deadline <= now)
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
                    self->writingBatch = ! due.empty() || ! discards.empty();
                    self->UpdateSettlementLocked();
#if defined(REDTILDE_ENABLE_TESTS)
                    if (self->writingBatch)
                        batchGate = std::exchange(self->batchTestGate, {});
#endif
                }
                if (due.empty() && discards.empty() && ! stopping)
                {
                    // Wait until an event or the earliest document deadline.
                    // Continuous edits cannot postpone a checkpoint indefinitely.
                    (void)WaitForSingleObject(self->wake.get(), delay);
                    continue;
                }
#if defined(REDTILDE_ENABLE_TESTS)
                const auto finishBatchGate = wil::scope_exit([&]() noexcept
                {
                    if (batchGate)
                        (void)SetEvent(batchGate->finished.get());
                });
                if (batchGate)
                {
                    (void)SetEvent(batchGate->entered.get());
                    (void)WaitForSingleObject(batchGate->proceed.get(), 30000);
                }
#endif
                result = S_OK;
                for (auto& [document, update] : due)
                {
                    const auto id = self->DocumentId(document);
                    auto& lease   = leases[document];
                    if (! lease)
                        lease = Lease(self->root, id);
                    HRESULT outcome = lease ? S_OK : HRESULT_FROM_WIN32(GetLastError());
                    if (SUCCEEDED(outcome))
                    {
                        if (update.snapshot)
                        {
                            std::string bytes;
                            outcome = TrySerializeRecovery(*update.snapshot, bytes);
                            if (SUCCEEDED(outcome))
                                outcome = WriteBytesAtomic(SnapshotPath(self->root, id), bytes);
                        }
                        else
                            outcome = Remove(SnapshotPath(self->root, id));
                    }
                    {
                        std::lock_guard lock(self->mutex);
                        self->snapshotStatuses[document] = outcome;
                    }
                    // Retire a restored candidate only after its replacement is durable.
                    if (SUCCEEDED(outcome) && ! update.retire.empty())
                        discards.push_back(std::move(update.retire));
                }
                std::vector<std::wstring> batchFailedDiscards;
                for (const auto& candidate : discards)
                {
                    auto other            = Lease(self->root, candidate);
                    const HRESULT removed = other ? Remove(SnapshotPath(self->root, candidate)) : HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
                    if (FAILED(removed))
                    {
                        result = removed;
                        batchFailedDiscards.push_back(candidate);
                    }
                    else
                    {
                        other.reset();
                        (void)Remove(self->root / (L"session-" + candidate + L".lease"));
                        std::lock_guard lock(self->mutex);
                        std::erase(self->failedDiscards, candidate);
                        if (std::ranges::find(self->retired, candidate) == self->retired.end() && self->retired.size() < MaximumCandidates)
                            self->retired.push_back(candidate);
                    }
                }
                {
                    std::lock_guard lock(self->mutex);
                    for (auto& failedId : batchFailedDiscards)
                    {
                        if (std::ranges::find(self->failedDiscards, failedId) == self->failedDiscards.end() && self->failedDiscards.size() < MaximumCandidates)
                            self->failedDiscards.push_back(std::move(failedId));
                    }
                    const auto failed  = std::ranges::find_if(self->snapshotStatuses, [](HRESULT value) { return FAILED(value); });
                    self->status       = failed != self->snapshotStatuses.end() ? *failed : result;
                    self->writingBatch = false;
                    self->UpdateSettlementLocked();
                    self->Notify();
                    if (self->workerPhase == WorkerPhase::Stopping && self->SettledLocked())
                        break;
                }
            }
            for (auto& [document, lease] : leases)
            {
                lease.reset();
                (void)Remove(self->root / (L"session-" + self->DocumentId(document) + L".lease"));
            }
        }
        catch (const std::bad_alloc&)
        {
            DxUi::Failure::FailAllocation();
        }
        catch (const std::system_error& error)
        {
            result = HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
        }
        catch (const std::invalid_argument&)
        {
            result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
        catch (const std::exception&)
        {
            DxUi::Failure::FailUnexpected();
        }
        std::lock_guard lock(self->mutex);
        if (FAILED(result))
            self->status = result;
        self->workerPhase  = WorkerPhase::Finished;
        self->writingBatch = false;
        self->UpdateSettlementLocked();
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
    auto staged             = std::make_shared<State>();
    staged->root            = std::move(directory) / L"Recovery";
    const HRESULT generated = CreateRecoveryId(staged->id);
    if (FAILED(generated))
        return generated;
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->settlement.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! staged->settlement)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window  = window;
    staged->message = message;
#if defined(REDTILDE_ENABLE_TESTS)
    staged->scanTestGate     = _scanTestGate;
    staged->postScanTestGate = _postScanTestGate;
    staged->batchTestGate    = _batchTestGate;
#endif
    auto ownership          = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT admission = ProcessWork::Acquire(ownership->lease);
    if (FAILED(admission))
        return admission;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
HRESULT RecoveryService::Submit(std::optional<RecoverySnapshot> snapshot, bool immediate, std::wstring retire) noexcept
{
    return SubmitDocument(0u, std::move(snapshot), immediate, std::move(retire));
}
HRESULT RecoveryService::SubmitDocument(unsigned document, std::optional<RecoverySnapshot> snapshot, bool immediate, std::wstring retire) noexcept
try
{
    if (! _state || document >= MaximumDocuments || (! retire.empty() && ! ValidId(retire)))
        return E_INVALIDARG;
    if (snapshot && ! ValidSnapshot(*snapshot))
        return E_INVALIDARG;
    std::lock_guard lock(_state->mutex);
    if (_state->workerPhase == State::WorkerPhase::Stopping || _state->workerPhase == State::WorkerPhase::Finished)
        return E_ABORT;
    const auto now        = GetTickCount64();
    auto [item, inserted] = _state->pending.try_emplace(document);
    auto& pending         = item->second;
    if (inserted)
        pending.first = now;
    pending.latest   = now;
    pending.snapshot = std::move(snapshot);
    if (! retire.empty())
        pending.retire = std::move(retire);
    pending.immediate = pending.immediate || immediate;
    _state->UpdateSettlementLocked();
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::invalid_argument&)
{
    return E_INVALIDARG;
}
HRESULT RecoveryService::Discard(std::wstring id) noexcept
try
{
    if (! _state || ! ValidId(id))
        return E_INVALIDARG;
    std::lock_guard lock(_state->mutex);
    if (_state->workerPhase == State::WorkerPhase::Stopping || _state->workerPhase == State::WorkerPhase::Finished)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->discards.size() + _state->retired.size() >= MaximumCandidates)
        return E_ABORT;
    if (std::ranges::find(_state->discards, id) == _state->discards.end())
        _state->discards.push_back(std::move(id));
#if defined(REDTILDE_ENABLE_TESTS)
    ++_state->discardAdmissions;
#endif
    _state->UpdateSettlementLocked();
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
std::vector<RecoveryCandidate> RecoveryService::TakeCandidates()
{
    if (! _state)
        return {};
    std::lock_guard lock(_state->mutex);
    return std::exchange(_state->candidates, {});
}
std::vector<std::wstring> RecoveryService::TakeRetired()
{
    if (! _state)
        return {};
    std::lock_guard lock(_state->mutex);
    return std::exchange(_state->retired, {});
}
std::vector<std::wstring> RecoveryService::TakeFailedDiscards()
{
    if (! _state)
        return {};
    std::lock_guard lock(_state->mutex);
    return std::exchange(_state->failedDiscards, {});
}
bool RecoveryService::Ready() const noexcept
{
    if (! _state)
        return true;
    std::lock_guard lock(_state->mutex);
    return _state->scanComplete || _state->workerPhase == State::WorkerPhase::Finished;
}
bool RecoveryService::Settled() const noexcept
{
    if (! _state)
        return true;
    std::lock_guard lock(_state->mutex);
    return _state->SettledLocked();
}
bool RecoveryService::Finished() const noexcept
{
    if (! _state)
        return true;
    std::lock_guard lock(_state->mutex);
    return _state->workerPhase == State::WorkerPhase::Finished;
}
HRESULT RecoveryService::Status() const noexcept
{
    if (! _state)
        return S_OK;
    std::lock_guard lock(_state->mutex);
    return _state->status;
}
RecoveryScanStatistics RecoveryService::ScanStatistics() const noexcept
{
    if (! _state)
        return {};
    std::lock_guard lock(_state->mutex);
    return _state->scanStatistics;
}
HRESULT RecoveryService::WaitForSettlement(DWORD milliseconds) const noexcept
{
    if (! _state)
        return S_OK;
    const DWORD result = WaitForSingleObject(_state->settlement.get(), milliseconds);
    if (result == WAIT_OBJECT_0)
        return Status();
    return HRESULT_FROM_WIN32(result == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
}
void RecoveryService::Stop() noexcept
{
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        if (_state->workerPhase != State::WorkerPhase::Finished)
            _state->workerPhase = State::WorkerPhase::Stopping;
        _state->UpdateSettlementLocked();
        (void)SetEvent(_state->wake.get());
    }
}
void RecoveryService::Close() noexcept
{
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        if (_state->workerPhase != State::WorkerPhase::Finished)
            _state->workerPhase = State::WorkerPhase::Stopping;
        _state->window = nullptr;
        _state->UpdateSettlementLocked();
        (void)SetEvent(_state->wake.get());
    }
    _state.reset();
}
#if defined(REDTILDE_ENABLE_TESTS)
void RecoveryService::SetScanTestGate(std::shared_ptr<TestGate> gate) noexcept
{
    _scanTestGate = std::move(gate);
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        _state->scanTestGate = _scanTestGate;
    }
}
void RecoveryService::SetPostScanTestGate(std::shared_ptr<TestGate> gate) noexcept
{
    _postScanTestGate = std::move(gate);
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        _state->postScanTestGate = _postScanTestGate;
    }
}
void RecoveryService::SetBatchTestGate(std::shared_ptr<TestGate> gate) noexcept
{
    _batchTestGate = std::move(gate);
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        _state->batchTestGate = _batchTestGate;
    }
}
std::size_t RecoveryService::DiscardAdmissionCountForTest() const noexcept
{
    if (! _state)
        return 0;
    std::lock_guard lock(_state->mutex);
    return _state->discardAdmissions;
}
#endif
} // namespace RedTilde
