#pragma once
#include "Common/ProcessWork.h"
#include "Common/Storage.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <wil/resource.h>

namespace RedTilde
{
// One admitted file command. Workers own values only; closure invalidates posting
// without joining a filesystem call. The owner never reads worker-mutated state.
class AsyncFiles
{
  public:
    enum class Phase : std::uint8_t
    {
        Idle,
        Loading,
        Saving,
        Completed,
        Closed
    };
    AsyncFiles() = default;
    AsyncFiles(const AsyncFiles &) = delete;
    AsyncFiles &operator=(const AsyncFiles &) = delete;
    struct Result
    {
        bool saving{};
        bool reloading{};
        std::filesystem::path path;
        StoredDocument document;
        HRESULT status{};
        std::size_t rejectedUnits{};
    };
#if defined(REDTILDE_ENABLE_TESTS)
    // Test-only admission barrier around the real filesystem operation. The
    // worker owns the events through publication, even when its UI is closed.
    struct TestGate
    {
        TestGate() = default;
        TestGate(const TestGate &) = delete;
        TestGate &operator=(const TestGate &) = delete;
        wil::unique_event entered{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        wil::unique_event proceed{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        wil::unique_event finished{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    };
#endif

  private:
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
    struct State
    {
        std::mutex mutex;
        HWND window{};
        UINT message{};
        Phase phase{Phase::Idle};
        std::uint64_t generation{};
        std::optional<Result> result;
#if defined(REDTILDE_ENABLE_TESTS)
        std::shared_ptr<TestGate> testGate;
#endif
    };
#pragma warning(pop)
    struct Job
    {
        Job() = default;
        Job(const Job &) = delete;
        Job &operator=(const Job &) = delete;
        Job(Job &&) = delete;
        Job &operator=(Job &&) = delete;
        ProcessWorkLease lease;
        std::shared_ptr<State> state;
        std::uint64_t generation{};
        Result result;
        std::optional<FileStamp> expected;
        bool sameIdentityOnly{};
        std::filesystem::path baselinePath;
        std::optional<SaveRecoveryContext> recovery;
#if defined(REDTILDE_ENABLE_TESTS)
        std::shared_ptr<TestGate> testGate;
#endif
    };
    std::shared_ptr<State> state;
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE, void *context) noexcept
    {
        ProcessWorkLease lease = std::move(static_cast<Job *>(context)->lease);
        const std::unique_ptr<Job> job(static_cast<Job *>(context));
        auto &result = job->result;
#if defined(REDTILDE_ENABLE_TESTS)
        const auto signalFinished = wil::scope_exit(
            [&]
            {
                if (job->testGate)
                    (void)SetEvent(job->testGate->finished.get());
            });
        if (job->testGate)
        {
            (void)SetEvent(job->testGate->entered.get());
            if (WaitForSingleObject(job->testGate->proceed.get(), 30000) != WAIT_OBJECT_0)
            {
                result.status = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                std::lock_guard lock(job->state->mutex);
                if (job->state->phase != Phase::Closed && job->generation == job->state->generation)
                {
                    job->state->result = std::move(result);
                    job->state->phase = Phase::Completed;
                    (void)PostMessageW(job->state->window, job->state->message, 0, 0);
                }
                return;
            }
        }
#endif
        if (result.saving)
        {
            result.status = SaveDocument(result.path, result.document.text, result.document.bom, job->expected ? &job->expected.value() : nullptr,
                                         result.document.stamp, job->sameIdentityOnly, job->baselinePath.empty() ? nullptr : &job->baselinePath,
                                         job->recovery ? &job->recovery.value() : nullptr);
            if (result.status == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
                result.rejectedUnits = result.document.text.size();
        }
        else
            result.status = LoadDocument(result.path, result.document, &result.rejectedUnits);
        std::lock_guard lock(job->state->mutex);
        if (job->state->phase != Phase::Closed && job->generation == job->state->generation)
        {
            job->state->result = std::move(result);
            job->state->phase = Phase::Completed;
            // No pointer payload and no recycled-HWND delivery: close takes this
            // same lock and invalidates the window before destroying it.
            (void)PostMessageW(job->state->window, job->state->message, 0, 0);
        }
    }

  public:
    void Initialize(HWND window, UINT message)
    {
        state = std::make_shared<State>();
        state->window = window;
        state->message = message;
    }
    ~AsyncFiles()
    {
        Close();
    }
    void Close() noexcept
    {
        if (!state)
            return;
        std::lock_guard lock(state->mutex);
        state->phase = Phase::Closed;
        ++state->generation;
        state->window = nullptr;
        state->result.reset();
    }
    void CancelInFlightLoad() noexcept
    {
        if (!state)
            return;
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::Loading || (state->phase == Phase::Completed && state->result && !state->result.value().saving))
        {
            ++state->generation;
            state->result.reset();
            state->phase = Phase::Idle;
        }
    }
    [[nodiscard]] Phase CurrentPhase() const noexcept
    {
        if (!state)
            return Phase::Idle;
        std::lock_guard lock(state->mutex);
        return state->phase;
    }
    [[nodiscard]] bool Busy() const noexcept
    {
        if (!state)
            return false;
        std::lock_guard lock(state->mutex);
        return state->phase == Phase::Loading || state->phase == Phase::Saving || state->phase == Phase::Completed;
    }
    [[nodiscard]] bool Saving() const noexcept
    {
        if (!state)
            return false;
        std::lock_guard lock(state->mutex);
        return state->phase == Phase::Saving;
    }
    [[nodiscard]] bool HasPendingSave() const noexcept
    {
        if (!state)
            return false;
        std::lock_guard lock(state->mutex);
        return state->phase == Phase::Saving || (state->phase == Phase::Completed && state->result && state->result.value().saving);
    }
#if defined(REDTILDE_ENABLE_TESTS)
    void SetTestGate(std::shared_ptr<TestGate> gate)
    {
        std::lock_guard lock(state->mutex);
        state->testGate = std::move(gate);
    }
#endif
    HRESULT Submit(bool saving, std::filesystem::path path, std::wstring text = {}, bool bom = false, std::optional<FileStamp> expected = {},
                   bool sameIdentityOnly = false, bool reloading = false, std::filesystem::path baselinePath = {},
                   std::optional<SaveRecoveryContext> recovery = {})
    {
        if (!state)
            return E_UNEXPECTED;
        auto job = std::make_unique<Job>();
        job->state = state;
        job->result.saving = saving;
        job->result.path = std::move(path);
        job->result.document.text = std::move(text);
        job->result.document.bom = bom;
        job->expected = expected;
        job->sameIdentityOnly = sameIdentityOnly;
        job->result.reloading = reloading;
        job->baselinePath = std::move(baselinePath);
        job->recovery = std::move(recovery);
        std::lock_guard lock(state->mutex);
        if (state->phase != Phase::Idle)
            return HRESULT_FROM_WIN32(ERROR_BUSY);
#if defined(REDTILDE_ENABLE_TESTS)
        job->testGate = state->testGate;
#endif
        job->generation = ++state->generation;
        state->phase = saving ? Phase::Saving : Phase::Loading;
        const HRESULT admission = ProcessWork::Acquire(job->lease);
        if (FAILED(admission))
        {
            state->phase = Phase::Idle;
            return admission;
        }
        if (!TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            state->phase = Phase::Idle;
            return HRESULT_FROM_WIN32(GetLastError());
        }
        (void)job.release();
        return S_OK;
    }
    std::optional<Result> Take()
    {
        if (!state)
            return {};
        std::lock_guard lock(state->mutex);
        if (state->phase != Phase::Completed || !state->result)
            return {};
        auto result = std::move(state->result);
        state->result.reset();
        state->phase = Phase::Idle;
        return result;
    }
};
} // namespace RedTilde
