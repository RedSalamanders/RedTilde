#pragma once
#include "Storage.h"
#include <DxUi/RichDocument.h>
#include <memory>
#include <optional>
#include <vector>

namespace RedTilde
{
struct RecoverySnapshot
{
    std::wstring text;
    std::filesystem::path originalPath;
    bool bom{};
    DxUi::RichText::Selection selection;
    std::uint64_t generation{};
};
struct RecoveryCandidate
{
    std::wstring id;
    RecoverySnapshot snapshot;
};
[[nodiscard]] std::string SerializeRecovery(const RecoverySnapshot &snapshot);
[[nodiscard]] HRESULT ParseRecovery(std::string_view bytes, RecoverySnapshot &snapshot) noexcept;

// One event-driven worker, up to 64 independently coalesced document snapshots.
// Each uses 750 ms quiet time and a five-second maximum admission delay. The
// worker owns all leases and filesystem access. No UI joins.
class RecoveryService final
{
  public:
    RecoveryService() = default;
    ~RecoveryService();
    RecoveryService(const RecoveryService &) = delete;
    RecoveryService &operator=(const RecoveryService &) = delete;
    [[nodiscard]] HRESULT Initialize(std::filesystem::path dataDirectory, HWND window = nullptr, UINT message = 0) noexcept;
    [[nodiscard]] HRESULT Submit(std::optional<RecoverySnapshot> snapshot, bool immediate = false, std::wstring retireAfterWrite = {}) noexcept;
    static constexpr unsigned MaximumDocuments = 64;
    [[nodiscard]] HRESULT SubmitDocument(unsigned document, std::optional<RecoverySnapshot> snapshot, bool immediate = false,
                                         std::wstring retireAfterWrite = {}) noexcept;
    [[nodiscard]] HRESULT Discard(std::wstring id) noexcept;
    [[nodiscard]] std::vector<RecoveryCandidate> TakeCandidates();
    [[nodiscard]] std::vector<std::wstring> TakeRetired();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] bool Settled() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    // Only the final WM_ENDSESSION boundary may wait, bounded by the caller's
    // shutdown allowance. Ordinary editing/Close continue asynchronously.
    [[nodiscard]] HRESULT WaitForSettlement(DWORD milliseconds) const noexcept;
    void Stop() noexcept;
    void Close() noexcept;

  private:
    struct State;
    std::shared_ptr<State> state;
};
} // namespace RedTilde
