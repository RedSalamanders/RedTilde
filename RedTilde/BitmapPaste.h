#pragma once
#include "Common/BitmapAssets.h"
#include "Common/ProcessWork.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <objbase.h>
#include <optional>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
// One admitted bitmap through creation, owner disposition and optional cleanup.
// All codec/filesystem work stays on an owned threadpool callback. The window
// remains a borrowed notification binding and is cleared before destruction.
class BitmapPasteWriter final
{
  public:
    enum class Phase : std::uint8_t
    {
        Idle,
        Encoding,
        Ready,
        Leased,
        Retiring,
        Stopped
    };
    static constexpr WPARAM Cookie = 0x5254424du;
    struct Result
    {
        std::uint64_t request{};
        HRESULT status = S_OK;
        BitmapAsset asset;
    };
#if defined(REDTILDE_ENABLE_TESTS)
    struct TestGate
    {
        TestGate() = default;
        TestGate(const TestGate &) = delete;
        TestGate &operator=(const TestGate &) = delete;
        TestGate(TestGate &&) = delete;
        TestGate &operator=(TestGate &&) = delete;
        wil::unique_event entered{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        wil::unique_event proceed{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    };
#endif

  private:
    struct State
    {
        State() = default;
        State(const State &) = delete;
        State &operator=(const State &) = delete;
        State(State &&) = delete;
        State &operator=(State &&) = delete;
        std::mutex mutex;
        std::filesystem::path directory;
        HWND window{};
        UINT message{};
        std::uint64_t serial{};
        Phase phase{Phase::Idle};
        bool stopRequested{};
        std::optional<Result> result;
        std::atomic_bool hasResult{};
#if defined(REDTILDE_ENABLE_TESTS)
        std::shared_ptr<TestGate> testGate;
#endif
        void Notify() const noexcept
        {
            if (window)
                (void)PostMessageW(window, message, Cookie, 0);
        }
    };
    struct Job
    {
        Job() = default;
        Job(const Job &) = delete;
        Job &operator=(const Job &) = delete;
        Job(Job &&) = delete;
        Job &operator=(Job &&) = delete;
        ProcessWorkLease lease;
        std::shared_ptr<State> state;
        std::vector<std::byte> dib;
        bool v5{}, discard{};
        Result result;
#if defined(REDTILDE_ENABLE_TESTS)
        std::shared_ptr<TestGate> testGate;
#endif
    };
    std::shared_ptr<State> state;
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void *context) noexcept
    {
        ProcessWorkLease lease = std::move(static_cast<Job *>(context)->lease);
        (void)CallbackMayRunLong(instance);
        std::unique_ptr<Job> job(static_cast<Job *>(context));
        const auto owner = job->state;
        auto result = std::move(job->result);
        const bool discard = job->discard;
        bool admitted = true;
#if defined(REDTILDE_ENABLE_TESTS)
        if (job->testGate)
        {
            (void)SetEvent(job->testGate->entered.get());
            if (WaitForSingleObject(job->testGate->proceed.get(), 30000) != WAIT_OBJECT_0)
            {
                result.status = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                admitted = false;
            }
        }
#endif
        if (!discard && admitted)
        {
            bool stopRequested{};
            {
                const std::scoped_lock lock(owner->mutex);
                stopRequested = owner->stopRequested;
            }
            if (stopRequested)
                result.status = HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
            else
            {
                const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
                result.status = FAILED(initialized) ? initialized : CreateBitmapAsset(owner->directory, job->dib, job->v5, result.asset);
            }
        }
        // Release the borrowed-input copy and codec apartment before advertising
        // idle or allowing another paste. No callback tail retains bitmap bytes.
        job.reset();
        {
            const std::scoped_lock lock(owner->mutex);
            if (!discard && !owner->stopRequested)
            {
                owner->result = std::move(result);
                owner->hasResult.store(true, std::memory_order_release);
                owner->phase = Phase::Ready;
                owner->Notify();
                return;
            }
        }
        const auto retired = RetireBitmapAsset(result.asset);
        if (FAILED(retired))
            OutputDebugStringW(L"RedTilde: retained a clipboard asset whose cleanup could not be verified.\n");
        const std::scoped_lock lock(owner->mutex);
        owner->phase = owner->stopRequested ? Phase::Stopped : Phase::Idle;
        owner->Notify();
    }
    static HRESULT Launch(std::unique_ptr<Job> job) noexcept
    {
        HRESULT status = ProcessWork::Acquire(job->lease);
        if (SUCCEEDED(status) && TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            (void)job.release();
            return S_OK;
        }
        if (SUCCEEDED(status))
            status = HRESULT_FROM_WIN32(GetLastError());
        const std::scoped_lock lock(job->state->mutex);
        job->state->phase = job->state->stopRequested ? Phase::Stopped : Phase::Idle;
        job->state->Notify();
        if (job->discard)
            OutputDebugStringW(L"RedTilde: retained a clipboard asset because cleanup could not be scheduled.\n");
        return status;
    }

  public:
    BitmapPasteWriter() = default;
    BitmapPasteWriter(const BitmapPasteWriter &) = delete;
    BitmapPasteWriter &operator=(const BitmapPasteWriter &) = delete;
    BitmapPasteWriter(BitmapPasteWriter &&) = delete;
    BitmapPasteWriter &operator=(BitmapPasteWriter &&) = delete;
    ~BitmapPasteWriter()
    {
        Stop();
        Detach();
    }
    HRESULT Initialize(std::filesystem::path directory, HWND window, UINT message)
    {
        if (state)
            return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        if (!window || message < WM_APP || !directory.is_absolute())
            return E_INVALIDARG;
        auto next = std::make_shared<State>();
        next->directory = std::move(directory);
        next->window = window;
        next->message = message;
        state = std::move(next);
        return S_OK;
    }
    HRESULT Submit(std::span<const std::byte> dib, bool v5, std::uint64_t &request)
    {
        if (!state || dib.empty() || dib.size() > MaximumClipboardDibBytes)
            return E_INVALIDARG;
        const std::scoped_lock lock(state->mutex);
        if (state->stopRequested || state->phase != Phase::Idle)
            return HRESULT_FROM_WIN32(ERROR_BUSY);
        auto job = std::make_unique<Job>();
        job->state = state;
        job->dib.assign(dib.begin(), dib.end());
        job->v5 = v5;
#if defined(REDTILDE_ENABLE_TESTS)
        job->testGate = state->testGate;
#endif
        job->result.request = ++state->serial;
        const auto number = job->result.request;
        const HRESULT admission = ProcessWork::Acquire(job->lease);
        if (FAILED(admission))
            return admission;
        state->phase = Phase::Encoding;
        // Submission failure resets phase to Idle here, avoiding recursive mutex entry.
        if (!TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            state->phase = Phase::Idle;
            return HRESULT_FROM_WIN32(GetLastError());
        }
        (void)job.release();
        request = number;
        return S_OK;
    }
    bool HasResult() const noexcept
    {
        return state && state->hasResult.load(std::memory_order_acquire);
    }
    [[nodiscard]] Phase CurrentPhase() const noexcept
    {
        if (!state)
            return Phase::Idle;
        const std::scoped_lock lock(state->mutex);
        return state->phase;
    }
#if defined(REDTILDE_ENABLE_TESTS)
    void SetTestGate(std::shared_ptr<TestGate> gate)
    {
        const std::scoped_lock lock(state->mutex);
        state->testGate = std::move(gate);
    }
#endif
    bool Take(Result &result) noexcept
    {
        if (!state)
            return false;
        const std::scoped_lock lock(state->mutex);
        if (state->phase != Phase::Ready || !state->result)
            return false;
        result = std::move(state->result.value());
        state->result.reset();
        state->hasResult.store(false, std::memory_order_release);
        state->phase = Phase::Leased;
        return true;
    }
    void Commit(std::uint64_t request) noexcept
    {
        if (!state)
            return;
        const std::scoped_lock lock(state->mutex);
        if (state->phase == Phase::Leased && request == state->serial)
        {
            state->phase = state->stopRequested ? Phase::Stopped : Phase::Idle;
            state->Notify();
        }
    }
    HRESULT Discard(Result result)
    {
        if (!state)
            return E_UNEXPECTED;
        if (result.asset.path.empty())
        {
            Commit(result.request);
            return S_OK;
        }
        auto job = std::make_unique<Job>();
        job->state = state;
        job->discard = true;
        job->result = std::move(result);
        {
            const std::scoped_lock lock(state->mutex);
            if (state->phase != Phase::Leased || job->result.request != state->serial)
                return E_UNEXPECTED;
            state->phase = Phase::Retiring;
        }
        return Launch(std::move(job));
    }
    void Stop()
    {
        if (!state)
            return;
        std::unique_ptr<Job> retireJob;
        if (HasResult())
            retireJob = std::make_unique<Job>();
        for (;;)
        {
            bool launchRetire{};
            bool needAllocate{};
            {
                const std::scoped_lock lock(state->mutex);
                state->stopRequested = true;
                if (state->phase == Phase::Idle)
                    state->phase = Phase::Stopped;
                else if (state->phase == Phase::Ready && state->result)
                {
                    if (state->result.value().asset.path.empty())
                    {
                        state->result.reset();
                        state->hasResult.store(false, std::memory_order_release);
                        state->phase = Phase::Stopped;
                    }
                    else if (!retireJob)
                        needAllocate = true;
                    else
                    {
                        retireJob->state = state;
                        retireJob->discard = true;
                        retireJob->result = std::move(state->result.value());
                        state->result.reset();
                        state->hasResult.store(false, std::memory_order_release);
                        state->phase = Phase::Retiring;
                        launchRetire = true;
                    }
                }
            }
            if (needAllocate)
            {
                retireJob = std::make_unique<Job>();
                continue;
            }
            if (launchRetire)
                (void)Launch(std::move(retireJob));
            return;
        }
    }
    void Detach() noexcept
    {
        if (state)
        {
            const std::scoped_lock lock(state->mutex);
            state->window = nullptr;
        }
    }
    bool Busy() const noexcept
    {
        if (!state)
            return false;
        const std::scoped_lock lock(state->mutex);
        return state->phase == Phase::Encoding || state->phase == Phase::Leased || state->phase == Phase::Retiring;
    }
};
} // namespace RedTilde
