#include "SpellService.h"
#include "Common/WinSpellProvider.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <chrono>
#include <new>
#include <utility>

namespace RedTilde
{
namespace
{
[[nodiscard]] std::wstring BuildSuggestCacheKey(const std::vector<std::wstring>& languages,
                                                std::wstring_view normalizedWord,
                                                std::uint64_t dictionaryGeneration)
{
    std::wstring key = std::to_wstring(dictionaryGeneration);
    key.push_back(L'|');
    for (const auto& tag : languages)
    {
        key.append(tag);
        key.push_back(L',');
    }
    key.push_back(L'|');
    key.append(normalizedWord);
    return key;
}

[[nodiscard]] bool EqualLanguageTagIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept
{
    return a.size() == b.size() &&
           (a.empty() || CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL);
}
} // namespace

SpellService::~SpellService() noexcept
{
    Stop();
    if (_threadHandle)
    {
        // Only destroy after signalling _stopEvent; bounded safety wait avoids leak on non-close destructs.
        (void)WaitForSingleObject(_threadHandle.get(), 2000u);
        _threadHandle.reset();
    }
}

void SpellService::Initialize(HWND window, UINT readyMessage) noexcept
{
    std::lock_guard lock(_state->mutex);
    _state->window       = window;
    _state->readyMessage = readyMessage;
    _state->closing      = false;
    if (! _state->threadLaunched)
        _state->phase = WorkerPhase::Idle;
}

void SpellService::SetProviderForTest(std::shared_ptr<ISpellProvider> provider) noexcept
{
    std::lock_guard lock(_state->mutex);
    _state->providerOverride = std::move(provider);
    _state->suggestCache.Clear();
    _state->cachedInstalledLanguages.clear();
    _state->installedLanguagesEnumerated = false;
}

HRESULT SpellService::EnsureWorkerStartedLocked() noexcept
{
    if (_state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->threadLaunched)
        return S_OK;

    if (! _state->stopEvent)
    {
        _state->stopEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (! _state->stopEvent)
            return HRESULT_FROM_WIN32(GetLastError());
    }
    if (! _state->workEvent)
    {
        _state->workEvent.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        if (! _state->workEvent)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    ProcessWorkLease lease;
    const HRESULT admission = ProcessWork::Acquire(lease);
    if (FAILED(admission))
        return admission;

    auto ctx = std::unique_ptr<WorkerStartContext>(new (std::nothrow) WorkerStartContext(_state, std::move(lease)));
    if (! ctx)
        DxUi::Failure::FailAllocation();

    _state->phase = WorkerPhase::Initializing;
    wil::unique_handle handle(CreateThread(nullptr, 0u, &SpellService::WorkerProc, ctx.get(), 0u, nullptr));
    if (! handle)
    {
        const DWORD err = GetLastError();
        _state->phase   = WorkerPhase::Stopped;
        return HRESULT_FROM_WIN32(err ? err : ERROR_NOT_ENOUGH_MEMORY);
    }
    (void)ctx.release();
    _threadHandle          = std::move(handle);
    _state->threadLaunched = true;
    return S_OK;
}

HRESULT SpellService::SubmitCheck(SpellCheckRequest request) noexcept
try
{
    std::lock_guard lock(_state->mutex);
    if (_state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);

    const HRESULT startHr = EnsureWorkerStartedLocked();
    if (FAILED(startHr))
        return startHr;

    ++_state->nextCheckSeq;
    _state->pendingCheck = std::move(request);
    (void)SetEvent(_state->workEvent.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT SpellService::SuggestSyncBounded(const std::vector<std::wstring>& languages,
                                         std::wstring_view word,
                                         std::uint64_t dictionaryGeneration,
                                         std::size_t maxSuggestions,
                                         std::vector<std::wstring>& outSuggestions,
                                         DWORD timeoutMs) noexcept
try
{
    outSuggestions.clear();
    if (word.empty() || maxSuggestions == 0u || languages.empty())
        return S_OK;

    std::wstring normalizedKey;
    if (! TryNormalizeDictionaryKey(word, normalizedKey))
        normalizedKey = std::wstring(word);

    const std::wstring cacheKey = BuildSuggestCacheKey(languages, normalizedKey, dictionaryGeneration);

    std::shared_ptr<PendingSuggest> req;
    {
        std::lock_guard lock(_state->mutex);
        if (_state->closing)
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        if (_state->suggestCache.TryGet(cacheKey, outSuggestions))
        {
            if (outSuggestions.size() > maxSuggestions)
                outSuggestions.resize(maxSuggestions);
            return S_OK;
        }
        const HRESULT startHr = EnsureWorkerStartedLocked();
        if (FAILED(startHr))
            return startHr;

        req                    = std::make_shared<PendingSuggest>();
        req->id                = ++_state->nextSuggestId;
        req->languages         = languages;
        req->word              = std::wstring(word);
        req->cacheKey          = cacheKey;
        req->maxSuggestions    = std::max<std::size_t>(maxSuggestions, 8u);
        _state->pendingSuggest = req;
        (void)SetEvent(_state->workEvent.get());
    }

    std::unique_lock waitLock(_state->mutex);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    (void)_state->suggestCv.wait_until(waitLock, deadline, [&] { return req->done || _state->closing; });
    if (! req->done)
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);

    outSuggestions = req->suggestions;
    if (outSuggestions.size() > maxSuggestions)
        outSuggestions.resize(maxSuggestions);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT SpellService::EnumerateInstalledLanguagesSyncBounded(std::vector<std::wstring>& outLanguages, DWORD timeoutMs) noexcept
try
{
    outLanguages.clear();
    {
        std::lock_guard lock(_state->mutex);
        if (_state->installedLanguagesEnumerated)
        {
            outLanguages = _state->cachedInstalledLanguages;
            return S_OK;
        }
        if (_state->closing)
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        const HRESULT startHr = EnsureWorkerStartedLocked();
        if (FAILED(startHr))
            return startHr;
        _state->pendingEnumerateLanguages = true;
        (void)SetEvent(_state->workEvent.get());
    }

    std::unique_lock waitLock(_state->mutex);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    (void)_state->suggestCv.wait_until(waitLock, deadline, [&] { return _state->installedLanguagesEnumerated || _state->closing; });
    if (! _state->installedLanguagesEnumerated)
        return _state->closing ? HRESULT_FROM_WIN32(ERROR_CANCELLED) : HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    outLanguages = _state->cachedInstalledLanguages;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::optional<SpellCheckResult> SpellService::TakeResult() noexcept
{
    std::lock_guard lock(_state->mutex);
    return std::exchange(_state->completedCheck, std::nullopt);
}

void SpellService::Stop() noexcept
{
    std::lock_guard lock(_state->mutex);
    _state->closing = true;
    _state->pendingCheck.reset();
    _state->pendingSuggest.reset();
    _state->pendingEnumerateLanguages = false;
    if (_state->threadLaunched)
    {
        if (_state->phase != WorkerPhase::Stopped)
            _state->phase = WorkerPhase::Closing;
        if (_state->stopEvent)
            (void)SetEvent(_state->stopEvent.get());
        if (_state->workEvent)
            (void)SetEvent(_state->workEvent.get());
    }
    else
    {
        _state->phase = WorkerPhase::Stopped;
    }
    _state->suggestCv.notify_all();
}

bool SpellService::Busy() const noexcept
{
    std::lock_guard lock(_state->mutex);
    return _state->pendingCheck.has_value() || _state->pendingSuggest != nullptr || _state->phase == WorkerPhase::Checking ||
           (! _state->closing && _state->completedCheck.has_value());
}

bool SpellService::Finished() const noexcept
{
    std::lock_guard lock(_state->mutex);
    return ! _state->threadLaunched || _state->phase == WorkerPhase::Stopped;
}

SpellService::WorkerPhase SpellService::Phase() const noexcept
{
    std::lock_guard lock(_state->mutex);
    return _state->phase;
}

DWORD WINAPI SpellService::WorkerProc(void* context) noexcept
{
    // Declare lease BEFORE COM apartment & WinSpellProvider so COM uninitializes before ProcessWorkLease releases.
    ProcessWorkLease lease = std::move(static_cast<WorkerStartContext*>(context)->lease);
    const std::unique_ptr<WorkerStartContext> startCtx(static_cast<WorkerStartContext*>(context));
    const std::shared_ptr<SharedState> state = startCtx->state;
    (void)lease;

    const HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const auto comScope = wil::scope_exit([comHr]() noexcept
    {
        if (SUCCEEDED(comHr))
            CoUninitialize();
    });
    const DxUi::Failure::Scope failureScope;
    RunWorkerLoop(*state);
    return 0u;
}

void SpellService::RunWorkerLoop(SharedState& state) noexcept
try
{
    WinSpellProvider defaultWinProvider;
    const auto resetProvider = wil::scope_exit([&]() noexcept { defaultWinProvider.ResetComObjects(); });

    for (;;)
    {
        std::optional<SpellCheckRequest> checkReq;
        std::uint64_t checkSeq{};
        std::shared_ptr<PendingSuggest> suggestReq;
        bool enumerateReq{};
        std::shared_ptr<ISpellProvider> overrideProvider;

        {
            std::lock_guard lock(state.mutex);
            if (state.closing)
                break;
            overrideProvider = state.providerOverride;
            if (state.pendingSuggest)
            {
                suggestReq = std::exchange(state.pendingSuggest, nullptr);
            }
            else if (state.pendingEnumerateLanguages)
            {
                state.pendingEnumerateLanguages = false;
                enumerateReq                    = true;
            }
            else if (state.pendingCheck)
            {
                checkReq    = std::exchange(state.pendingCheck, std::nullopt);
                checkSeq    = state.nextCheckSeq;
                state.phase = WorkerPhase::Checking;
            }
            else
            {
                state.phase = WorkerPhase::Idle;
            }
        }

        ISpellProvider& activeProvider = overrideProvider ? *overrideProvider : static_cast<ISpellProvider&>(defaultWinProvider);
        if (suggestReq)
        {
            ProcessSuggest(state, activeProvider, suggestReq);
            continue;
        }
        if (enumerateReq)
        {
            std::vector<std::wstring> installed;
            (void)activeProvider.EnumerateInstalledLanguages(installed);
            std::lock_guard lock(state.mutex);
            state.cachedInstalledLanguages     = std::move(installed);
            state.installedLanguagesEnumerated = true;
            state.suggestCv.notify_all();
            continue;
        }
        if (checkReq)
        {
            ProcessCheck(state, activeProvider, *checkReq, checkSeq);
            continue;
        }

        HANDLE waits[2]{state.stopEvent.get(), state.workEvent.get()};
        const DWORD waitRes = WaitForMultipleObjects(2u, waits, FALSE, INFINITE);
        if (waitRes != WAIT_OBJECT_0 + 1u)
            break;
    }

    defaultWinProvider.ResetComObjects();
    HWND notifyWindow{};
    UINT notifyMessage{};
    {
        std::lock_guard lock(state.mutex);
        state.phase   = WorkerPhase::Stopped;
        notifyWindow  = state.window;
        notifyMessage = state.readyMessage;
        state.suggestCv.notify_all();
    }
    if (notifyWindow && notifyMessage)
        (void)PostMessageW(notifyWindow, notifyMessage, 0u, 0);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::exception&)
{
    DxUi::Failure::FailUnexpected();
}

void SpellService::ProcessSuggest(SharedState& state, ISpellProvider& provider, const std::shared_ptr<PendingSuggest>& req) noexcept
try
{
    std::vector<std::wstring> merged;
    std::vector<std::wstring> perLang;
    for (const auto& tag : req->languages)
    {
        if (provider.CheckAvailability(tag) != ProviderAvailability::Ready)
            continue;
        perLang.clear();
        if (SUCCEEDED(provider.SuggestWord(tag, req->word, req->maxSuggestions, perLang)))
        {
            for (auto& candidate : perLang)
            {
                if (std::ranges::find(merged, candidate) == merged.end())
                {
                    merged.push_back(std::move(candidate));
                    if (merged.size() >= req->maxSuggestions)
                        break;
                }
            }
        }
        if (merged.size() >= req->maxSuggestions)
            break;
    }

    std::lock_guard lock(state.mutex);
    req->suggestions = merged;
    req->done        = true;
    if (! state.closing)
        state.suggestCache.Put(req->cacheKey, std::move(merged));
    state.suggestCv.notify_all();
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

void SpellService::ProcessCheck(SharedState& state, ISpellProvider& provider, const SpellCheckRequest& req, std::uint64_t seq) noexcept
try
{
    auto shouldAbort = [&]() noexcept
    {
        std::lock_guard lock(state.mutex);
        return state.closing || state.nextCheckSeq != seq;
    };

    SpellCheckResult result;
    result.key               = req.key;
    result.resolvedLanguages = req.languages;
    result.availabilities.reserve(req.languages.size());
    result.diagnostics.reserve(req.languages.size());

    (void)provider.EnumerateInstalledLanguages(result.installedLanguages);
    {
        std::lock_guard lock(state.mutex);
        if (! result.installedLanguages.empty() || ! state.installedLanguagesEnumerated)
        {
            state.cachedInstalledLanguages     = result.installedLanguages;
            state.installedLanguagesEnumerated = true;
            state.suggestCv.notify_all();
        }
    }

    std::vector<std::size_t> readyIndices;
    for (std::size_t i = 0u; i < req.languages.size(); ++i)
    {
        if (shouldAbort())
            return;
        ProviderDiagnostic diag;
        const ProviderAvailability avail = provider.CheckAvailability(req.languages[i], &diag);
        result.availabilities.push_back(avail);
        result.diagnostics.push_back(std::move(diag));
        if (avail == ProviderAvailability::Ready)
            readyIndices.push_back(i);
    }

    if (readyIndices.empty())
    {
        HWND notifyWindow{};
        UINT notifyMessage{};
        {
            std::lock_guard lock(state.mutex);
            if (state.closing || state.nextCheckSeq != seq)
                return;
            state.completedCheck = std::move(result);
            notifyWindow         = state.window;
            notifyMessage        = state.readyMessage;
        }
        if (notifyWindow && notifyMessage)
            (void)PostMessageW(notifyWindow, notifyMessage, 0u, 0);
        return;
    }

    const std::size_t primaryIndex     = readyIndices.front();
    const std::wstring_view primaryTag = req.languages[primaryIndex];
    std::vector<SpellErrorSpan> chunkSpans;
    std::vector<SpellErrorSpan> secondarySpans;

    for (const auto& chunk : req.prose.chunks)
    {
        if (shouldAbort())
            return;

        chunkSpans.clear();
        const HRESULT chunkHr = provider.CheckSegment(primaryTag, chunk.text, req.options, chunkSpans);
        if (FAILED(chunkHr))
        {
            result.status = chunkHr;
            break;
        }

        for (const auto& span : chunkSpans)
        {
            if (span.start >= chunk.text.size() || span.length == 0u || span.length > chunk.text.size() - span.start)
                continue;
            const std::wstring_view word(chunk.text.data() + span.start, span.length);

            std::wstring normalized;
            if (TryNormalizeDictionaryKey(word, normalized) && req.dictionarySnapshot.ContainsNormalized(normalized))
                continue;

            bool acceptedBySecondary = false;
            for (std::size_t r = 1u; r < readyIndices.size(); ++r)
            {
                const std::wstring_view secondaryTag = req.languages[readyIndices[r]];
                if (EqualLanguageTagIgnoreCase(secondaryTag, primaryTag))
                    continue;
                secondarySpans.clear();
                if (SUCCEEDED(provider.CheckSegment(secondaryTag, word, req.options, secondarySpans)) && secondarySpans.empty())
                {
                    acceptedBySecondary = true;
                    break;
                }
            }
            if (acceptedBySecondary)
                continue;

            SpellRawIssue raw;
            raw.offset               = chunk.baseOffset + span.start;
            raw.length               = span.length;
            raw.word                 = std::wstring(word);
            raw.normalizedKey        = normalized.empty() ? std::wstring(word) : std::move(normalized);
            raw.primaryLanguageIndex = static_cast<std::uint8_t>(primaryIndex);
            result.issues.push_back(std::move(raw));
            if (result.issues.size() >= MaximumRawIssues)
                break;
        }
        if (result.issues.size() >= MaximumRawIssues)
            break;
    }

    HWND notifyWindow{};
    UINT notifyMessage{};
    {
        std::lock_guard lock(state.mutex);
        if (state.closing || state.nextCheckSeq != seq)
            return;
        state.completedCheck = std::move(result);
        notifyWindow         = state.window;
        notifyMessage        = state.readyMessage;
    }
    if (notifyWindow && notifyMessage)
        (void)PostMessageW(notifyWindow, notifyMessage, 0u, 0);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
} // namespace RedTilde
