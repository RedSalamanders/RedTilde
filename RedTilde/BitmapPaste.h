#pragma once
#include "Common/BitmapAssets.h"
#include <atomic>
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
        bool busy{}, leased{}, stopping{};
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
            bool stopping{};
            {
                const std::scoped_lock lock(owner->mutex);
                stopping = owner->stopping;
            }
            if (stopping)
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
            if (!owner->stopping && !discard)
            {
                owner->result = std::move(result);
                owner->hasResult.store(true, std::memory_order_release);
                owner->busy = false;
                owner->Notify();
                return;
            }
        }
        const auto retired = RetireBitmapAsset(result.asset);
        if (FAILED(retired))
            OutputDebugStringW(L"RedTilde: retained a clipboard asset whose cleanup could not be verified.\n");
        const std::scoped_lock lock(owner->mutex);
        owner->busy = false;
        owner->Notify();
    }
    static HRESULT Launch(std::unique_ptr<Job> job) noexcept
    {
        if (TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            (void)job.release();
            return S_OK;
        }
        const auto status = HRESULT_FROM_WIN32(GetLastError());
        const std::scoped_lock lock(job->state->mutex);
        job->state->busy = false;
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
        if (state->stopping || state->busy || state->leased || state->result)
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
        state->busy = true;
        // Submission failure resets busy here, avoiding recursive mutex entry.
        if (!TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            state->busy = false;
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
        if (!state->result || state->leased)
            return false;
        result = std::move(state->result.value());
        state->result.reset();
        state->hasResult.store(false, std::memory_order_release);
        state->leased = true;
        return true;
    }
    void Commit(std::uint64_t request) noexcept
    {
        if (!state)
            return;
        const std::scoped_lock lock(state->mutex);
        if (state->leased && request == state->serial)
        {
            state->leased = false;
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
            if (!state->leased || job->result.request != state->serial || state->busy)
                return E_UNEXPECTED;
            state->leased = false;
            state->busy = true;
        }
        return Launch(std::move(job));
    }
    void Stop()
    {
        if (!state)
            return;
        Result abandoned;
        bool discard{};
        {
            const std::scoped_lock lock(state->mutex);
            state->stopping = true;
            if (state->result)
            {
                abandoned = std::move(state->result.value());
                state->result.reset();
                state->hasResult.store(false, std::memory_order_release);
                state->leased = true;
                discard = true;
            }
        }
        if (discard)
            (void)Discard(std::move(abandoned));
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
        return state->busy || state->leased;
    }
};
} // namespace RedTilde
