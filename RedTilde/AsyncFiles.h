#pragma once
#include "Common/Storage.h"
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
    AsyncFiles() = default;
    AsyncFiles(const AsyncFiles &) = delete;
    AsyncFiles &operator=(const AsyncFiles &) = delete;
    struct Result
    {
        bool saving{};
        std::filesystem::path path;
        StoredDocument document;
        HRESULT status{};
    };

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
        bool closing{}, busy{};
        std::optional<Result> result;
    };
#pragma warning(pop)
    struct Job
    {
        std::shared_ptr<State> state;
        Result result;
        std::optional<FileStamp> expected;
    };
    std::shared_ptr<State> state;
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE, void *context) noexcept
    {
        const std::unique_ptr<Job> job(static_cast<Job *>(context));
        auto &result = job->result;
        if (result.saving)
            result.status = SaveDocument(result.path, result.document.text, result.document.bom, job->expected ? &job->expected.value() : nullptr,
                                         result.document.stamp);
        else
            result.status = LoadDocument(result.path, result.document);
        std::lock_guard lock(job->state->mutex);
        if (!job->state->closing)
        {
            job->state->result = std::move(result);
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
        state->closing = true;
        state->window = nullptr;
        state->result.reset();
    }
    bool Busy() const noexcept
    {
        if (!state)
            return false;
        std::lock_guard lock(state->mutex);
        return state->busy;
    }
    HRESULT Submit(bool saving, std::filesystem::path path, std::wstring text = {}, bool bom = false, std::optional<FileStamp> expected = {})
    {
        auto job = std::make_unique<Job>();
        job->state = state;
        job->result.saving = saving;
        job->result.path = std::move(path);
        job->result.document.text = std::move(text);
        job->result.document.bom = bom;
        job->expected = expected;
        std::lock_guard lock(state->mutex);
        if (state->busy || state->closing)
            return HRESULT_FROM_WIN32(ERROR_BUSY);
        state->busy = true;
        if (!TrySubmitThreadpoolCallback(Worker, job.get(), nullptr))
        {
            state->busy = false;
            return HRESULT_FROM_WIN32(GetLastError());
        }
        (void)job.release();
        return S_OK;
    }
    std::optional<Result> Take()
    {
        std::lock_guard lock(state->mutex);
        if (!state->result)
            return {};
        auto result = std::move(state->result);
        state->result.reset();
        state->busy = false;
        return result;
    }
};
} // namespace RedTilde
