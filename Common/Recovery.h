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

// One event-driven worker, one coalesced immutable snapshot, 750 ms quiet-time
// debounce. The worker owns its lease and all filesystem access. No UI joins.
class RecoveryService final
{
  public:
    RecoveryService() = default;
    ~RecoveryService();
    RecoveryService(const RecoveryService &) = delete;
    RecoveryService &operator=(const RecoveryService &) = delete;
    [[nodiscard]] HRESULT Initialize(std::filesystem::path dataDirectory, HWND window = nullptr, UINT message = 0) noexcept;
    [[nodiscard]] HRESULT Submit(std::optional<RecoverySnapshot> snapshot, bool immediate = false, std::wstring retireAfterWrite = {}) noexcept;
    [[nodiscard]] HRESULT Discard(std::wstring id) noexcept;
    [[nodiscard]] std::vector<RecoveryCandidate> TakeCandidates();
    [[nodiscard]] std::vector<std::wstring> TakeRetired();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] bool Settled() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    void Stop() noexcept;
    void Close() noexcept;

  private:
    struct State;
    std::shared_ptr<State> state;
};
} // namespace RedTilde
