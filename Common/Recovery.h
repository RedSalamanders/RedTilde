#pragma once
#include "Storage.h"
#include <DxUi/RichDocument.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace RedTilde
{
enum class RecoverySnapshotKind : std::uint8_t
{
    CrashRecovery = 0,
    SessionResume = 1
};
struct DocumentOriginInfo
{
    enum class Kind : std::uint8_t
    {
        Local  = 0,
        GitHub = 1
    };
    Kind kind{Kind::Local};
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    std::wstring relativePath;
    std::wstring blobSha;
    std::wstring htmlUrl;
    bool operator==(const DocumentOriginInfo&) const = default;
};
struct RecoverySnapshot
{
    std::wstring text;
    std::filesystem::path originalPath;
    bool bom{};
    DxUi::RichText::Selection selection;
    std::uint64_t generation{};
    RecoverySnapshotKind kind{RecoverySnapshotKind::CrashRecovery};
    std::uint32_t workspaceOrder{};
    bool wasActive{};
    std::optional<FileStamp> savedBaselineStamp;
    DocumentOriginInfo origin;
    bool recovered{};
};
struct RecoveryCandidate
{
    std::wstring id;
    RecoverySnapshot snapshot;
};
struct RecoveryScanStatistics
{
    // Startup work, including unrelated names. Published under the service
    // mutex before Ready; reaching the limit never removes unexamined files.
    std::size_t examinedEntries{};
    bool entryLimitReached{};
};
[[nodiscard]] std::string SerializeRecovery(const RecoverySnapshot& snapshot);
[[nodiscard]] HRESULT TrySerializeRecovery(const RecoverySnapshot& snapshot, std::string& output) noexcept;
[[nodiscard]] HRESULT ParseRecovery(std::string_view bytes, RecoverySnapshot& snapshot) noexcept;

// Worker-owned, exclusively leased immutable save copy. Destruction preserves
// its envelope for restart; only a successful write explicitly retires it.
// Ordinary autosave submissions/retirements cannot coalesce or delete this id.
class SaveRecoveryProtection final
{
public:
    SaveRecoveryProtection();
    ~SaveRecoveryProtection();
    SaveRecoveryProtection(const SaveRecoveryProtection&)            = delete;
    SaveRecoveryProtection& operator=(const SaveRecoveryProtection&) = delete;
    [[nodiscard]] HRESULT Admit(const std::filesystem::path& dataDirectory, const RecoverySnapshot& snapshot) noexcept;
    [[nodiscard]] HRESULT Retire() noexcept;

private:
    struct State;
    std::unique_ptr<State> _state;
};

// One event-driven worker, up to 64 independently coalesced document snapshots.
// Each uses 750 ms quiet time and a five-second maximum admission delay. The
// worker owns all leases and filesystem access. No UI joins.
class RecoveryService final
{
public:
    RecoveryService() = default;
    ~RecoveryService();
    RecoveryService(const RecoveryService&)            = delete;
    RecoveryService& operator=(const RecoveryService&) = delete;
    [[nodiscard]] HRESULT Initialize(std::filesystem::path dataDirectory, HWND window = nullptr, UINT message = 0) noexcept;
    [[nodiscard]] HRESULT Submit(std::optional<RecoverySnapshot> snapshot, bool immediate = false, std::wstring retireAfterWrite = {}) noexcept;
    static constexpr unsigned MaximumDocuments = 64;
    [[nodiscard]] HRESULT SubmitDocument(unsigned document,
                                         std::optional<RecoverySnapshot> snapshot,
                                         bool immediate                = false,
                                         std::wstring retireAfterWrite = {}) noexcept;
    [[nodiscard]] HRESULT Discard(std::wstring id) noexcept;
    [[nodiscard]] std::vector<RecoveryCandidate> TakeCandidates();
    [[nodiscard]] std::vector<std::wstring> TakeRetired();
    [[nodiscard]] std::vector<std::wstring> TakeFailedDiscards();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] bool Settled() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    [[nodiscard]] RecoveryScanStatistics ScanStatistics() const noexcept;
    // Only the final WM_ENDSESSION boundary may wait, bounded by the caller's
    // shutdown allowance. Ordinary editing/Close continue asynchronously.
    [[nodiscard]] HRESULT WaitForSettlement(DWORD milliseconds) const noexcept;
    void Stop() noexcept;
    void Close() noexcept;
#if defined(ENABLE_TESTS)
    struct TestGate
    {
        TestGate()                           = default;
        TestGate(const TestGate&)            = delete;
        TestGate& operator=(const TestGate&) = delete;
        wil::unique_event entered{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        wil::unique_event proceed{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        wil::unique_event finished{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    };
    void SetScanTestGate(std::shared_ptr<TestGate> gate) noexcept;
    void SetPostScanTestGate(std::shared_ptr<TestGate> gate) noexcept;
    void SetBatchTestGate(std::shared_ptr<TestGate> gate) noexcept;
    [[nodiscard]] std::size_t DiscardAdmissionCountForTest() const noexcept;
#endif

private:
    struct State;
    std::shared_ptr<State> _state;
#if defined(ENABLE_TESTS)
    std::shared_ptr<TestGate> _scanTestGate;
    std::shared_ptr<TestGate> _postScanTestGate;
    std::shared_ptr<TestGate> _batchTestGate;
#endif
};
} // namespace RedTilde
