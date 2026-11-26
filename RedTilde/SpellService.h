#pragma once
#include "Common/CustomDictionary.h"
#include "Common/ProcessWork.h"
#include "Common/ProseExtractor.h"
#include "Common/SpellProvider.h"
#include <DxUi/RichDocument.h>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <wil/resource.h>
#include <windows.h>

namespace RedTilde
{
struct SpellRequestKey
{
    unsigned workspaceKey{};
    DxUi::RichText::SourceSnapshot sourceSnapshot;
    std::uint64_t semanticIdentity{};
    std::uint64_t dictionaryGeneration{};
    std::uint64_t languageConfigVersion{};

    [[nodiscard]] bool Matches(const SpellRequestKey& other) const noexcept
    {
        return workspaceKey == other.workspaceKey && sourceSnapshot.IsSameRevision(other.sourceSnapshot) && semanticIdentity == other.semanticIdentity &&
               dictionaryGeneration == other.dictionaryGeneration && languageConfigVersion == other.languageConfigVersion;
    }
};

struct SpellCheckRequest
{
    SpellRequestKey key;
    ExtractedProse prose;
    std::vector<std::wstring> languages;
    SpellCheckOptions options;
    CustomDictionary::Snapshot dictionarySnapshot;
    bool semanticMode{};
};

struct SpellRawIssue
{
    std::size_t offset{};
    std::size_t length{};
    std::wstring word;
    std::wstring normalizedKey;
    SpellIssueKind kind{SpellIssueKind::Misspelled};
};

struct SpellCheckResult
{
    SpellRequestKey key;
    std::vector<SpellRawIssue> issues;
    std::vector<std::wstring> resolvedLanguages;
    std::vector<ProviderAvailability> availabilities;
    std::vector<ProviderDiagnostic> diagnostics;
    std::vector<std::wstring> installedLanguages;
    HRESULT status = S_OK;
};

// Dedicated lazy STA COM background worker for spell checking and on-demand suggestions.
// Holds one ProcessWorkLease while active, sleeps on Win32 events (zero polling), and drains cleanly on Stop().
class SpellService final
{
public:
    enum class WorkerPhase : std::uint8_t
    {
        Idle,
        Initializing,
        Checking,
        Closing,
        Stopped
    };

    static constexpr std::size_t MaximumRawIssues        = 4096u;
    static constexpr std::size_t SuggestionCacheCapacity = 128u;

    SpellService() noexcept = default;
    ~SpellService() noexcept;
    SpellService(const SpellService&)            = delete;
    SpellService& operator=(const SpellService&) = delete;

    void Initialize(HWND window, UINT readyMessage) noexcept;
#if defined(ENABLE_TESTS)
    void SetProviderForTest(std::shared_ptr<ISpellProvider> provider) noexcept;
#endif

    [[nodiscard]] HRESULT SubmitCheck(SpellCheckRequest request) noexcept;
    [[nodiscard]] HRESULT SuggestSyncBounded(const std::vector<std::wstring>& languages,
                                             std::wstring_view word,
                                             std::uint64_t dictionaryGeneration,
                                             std::size_t maxSuggestions,
                                             std::vector<std::wstring>& outSuggestions,
                                             DWORD timeoutMs = 150u) noexcept;
    [[nodiscard]] HRESULT EnumerateInstalledLanguagesSyncBounded(std::vector<std::wstring>& outLanguages, DWORD timeoutMs = 200u) noexcept;
    [[nodiscard]] std::optional<SpellCheckResult> TakeResult() noexcept;

    void Stop() noexcept;
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    [[nodiscard]] WorkerPhase Phase() const noexcept;

private:
    struct PendingSuggest
    {
        std::uint64_t id{};
        std::vector<std::wstring> languages;
        std::wstring word;
        std::wstring cacheKey;
        std::size_t maxSuggestions{};
        std::vector<std::wstring> suggestions;
        bool done{};
    };

    struct SuggestionCache
    {
        struct Entry
        {
            std::wstring key;
            std::vector<std::wstring> suggestions;
        };
        std::list<Entry> entries;
        std::unordered_map<std::wstring, std::list<Entry>::iterator> index;

        void Clear() noexcept
        {
            index.clear();
            entries.clear();
        }
        bool TryGet(const std::wstring& key, std::vector<std::wstring>& out)
        {
            const auto it = index.find(key);
            if (it == index.end())
                return false;
            entries.splice(entries.begin(), entries, it->second);
            out = it->second->suggestions;
            return true;
        }
        void Put(std::wstring key, std::vector<std::wstring> value)
        {
            const auto it = index.find(key);
            if (it != index.end())
            {
                it->second->suggestions = std::move(value);
                entries.splice(entries.begin(), entries, it->second);
                return;
            }
            if (entries.size() >= SuggestionCacheCapacity)
            {
                const auto& last = entries.back();
                index.erase(last.key);
                entries.pop_back();
            }
            entries.push_front(Entry{std::move(key), std::move(value)});
            index[entries.front().key] = entries.begin();
        }
    };

    struct SharedState
    {
        SharedState()                              = default;
        SharedState(const SharedState&)            = delete;
        SharedState& operator=(const SharedState&) = delete;
        SharedState(SharedState&&)                 = delete;
        SharedState& operator=(SharedState&&)      = delete;

        std::mutex mutex;
        std::condition_variable suggestCv;
        HWND window{};
        UINT readyMessage{};
        wil::unique_event_nothrow stopEvent;
        wil::unique_event_nothrow workEvent;
        std::shared_ptr<ISpellProvider> providerOverride;
        std::optional<SpellCheckRequest> pendingCheck;
        std::uint64_t nextCheckSeq{};
        std::optional<SpellCheckResult> completedCheck;
        std::shared_ptr<PendingSuggest> pendingSuggest;
        std::uint64_t nextSuggestId{};
        SuggestionCache suggestCache;
        std::vector<std::wstring> cachedInstalledLanguages;
        bool installedLanguagesEnumerated{};
        bool pendingEnumerateLanguages{};
        WorkerPhase phase{WorkerPhase::Idle};
        bool threadLaunched{};
        bool closing{};
    };

    struct WorkerStartContext
    {
        WorkerStartContext(std::shared_ptr<SharedState> s, ProcessWorkLease l) noexcept : state(std::move(s)), lease(std::move(l))
        {
        }
        WorkerStartContext(const WorkerStartContext&)            = delete;
        WorkerStartContext& operator=(const WorkerStartContext&) = delete;
        WorkerStartContext(WorkerStartContext&&)                 = delete;
        WorkerStartContext& operator=(WorkerStartContext&&)      = delete;

        std::shared_ptr<SharedState> state;
        ProcessWorkLease lease;
    };

    [[nodiscard]] HRESULT EnsureWorkerStartedLocked() noexcept;
    static DWORD WINAPI WorkerProc(void* context) noexcept;
    static void RunWorkerLoop(SharedState& state) noexcept;
    static void ProcessCheck(SharedState& state, ISpellProvider& provider, const SpellCheckRequest& req, std::uint64_t seq) noexcept;
    static void ProcessSuggest(SharedState& state, ISpellProvider& provider, const std::shared_ptr<PendingSuggest>& req) noexcept;

    std::shared_ptr<SharedState> _state = std::make_shared<SharedState>();
    wil::unique_handle _threadHandle;
};
} // namespace RedTilde
