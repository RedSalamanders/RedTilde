#pragma once
#include "Files.h"
#include "Storage.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace RedTilde
{
inline constexpr std::size_t MaximumGitHubTreeJsonBytes = 4u * 1024u * 1024u;
inline constexpr std::size_t MaximumGitHubUserRepos     = 500u;
inline constexpr std::size_t MaximumGitHubTreeEntries   = 5000u;
inline constexpr unsigned MaximumGitHubTreeDepth        = 8u;

struct ParsedGitHubTarget
{
    bool valid{};
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    std::wstring filePath;
    std::wstring errorReason;
};

struct GitHubRepoSummary
{
    std::wstring owner;
    std::wstring repo;
    std::wstring fullName;
    std::wstring description;
    std::wstring defaultBranch;
    std::wstring htmlUrl;
    bool isPrivate{};
};

struct GitHubTreeEntry
{
    std::wstring path;
    std::wstring name;
    std::wstring sha;
    std::wstring htmlUrl;
    std::size_t parent{NoFilesParent};
    std::size_t markdownCount{};
    unsigned depth{};
    bool directory{};
};

struct GitHubTreeSnapshot
{
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    std::vector<GitHubTreeEntry> entries;
    std::size_t markdownCount{};
    bool truncated{};
    bool limitReached{};
    HRESULT status{S_OK};
    std::wstring errorMessage;
};

struct GitHubDeviceCodeInfo
{
    std::wstring deviceCode;
    std::wstring userCode;
    std::wstring verificationUri;
    unsigned expiresInSeconds{900u};
    unsigned intervalSeconds{5u};
};

struct GitHubDevicePollResult
{
    enum class State : std::uint8_t
    {
        Pending      = 0,
        SlowDown     = 1,
        Authorized   = 2,
        AccessDenied = 3,
        Expired      = 4,
        Error        = 5
    };
    State state{State::Pending};
    std::wstring accessToken;
    unsigned intervalSeconds{5u};
    std::wstring errorMessage;
};

struct GitHubHttpRequest
{
    std::wstring host{L"api.github.com"};
    std::wstring method{L"GET"};
    std::wstring path;
    std::wstring acceptHeader{L"application/vnd.github+json"};
    std::wstring bearerToken;
    std::string body;
    std::string contentType;
    std::size_t maximumResponseBytes{MaximumGitHubTreeJsonBytes};
};

struct GitHubHttpResponse
{
    HRESULT transportStatus{S_OK};
    unsigned httpStatusCode{};
    std::string body;
    std::wstring errorMessage;
    bool rateLimited{};
    bool hasNextPage{};
};

class IGitHubTransport
{
public:
    virtual ~IGitHubTransport()                                                         = default;
    [[nodiscard]] virtual HRESULT SendRequest(const GitHubHttpRequest& request,
                                              const std::atomic_bool* cancellation,
                                              HANDLE abortEvent,
                                              GitHubHttpResponse& outResponse) noexcept = 0;
};

class WinHttpGitHubTransport final : public IGitHubTransport
{
public:
    [[nodiscard]] HRESULT SendRequest(const GitHubHttpRequest& request,
                                      const std::atomic_bool* cancellation,
                                      HANDLE abortEvent,
                                      GitHubHttpResponse& outResponse) noexcept override;
};

// Pure URL parser & autosuggest scorer.
[[nodiscard]] ParsedGitHubTarget ParseGitHubUrl(std::wstring_view input);
[[nodiscard]] std::vector<GitHubRepoSummary> FilterAndRankGitHubRepos(std::span<const GitHubRepoSummary> repos,
                                                                      std::wstring_view query,
                                                                      std::size_t maximumResults = 50u);

// Bounded JSON decoders over GitHub REST API responses (sanitizing all UI strings).
[[nodiscard]] HRESULT ParseGitHubUserLoginJson(std::string_view jsonBytes, std::wstring& outLogin) noexcept;
[[nodiscard]] HRESULT ParseGitHubReposPageJson(std::string_view jsonBytes,
                                               std::vector<GitHubRepoSummary>& inOutRepos,
                                               std::size_t maximumTotal = MaximumGitHubUserRepos) noexcept;
[[nodiscard]] HRESULT ParseGitHubDefaultBranchJson(std::string_view jsonBytes, std::wstring& outBranch) noexcept;
[[nodiscard]] HRESULT ParseGitHubTreeJson(
    std::string_view jsonBytes, std::wstring_view owner, std::wstring_view repo, std::wstring_view branch, GitHubTreeSnapshot& outSnapshot) noexcept;
[[nodiscard]] HRESULT ParseGitHubDeviceCodeJson(std::string_view jsonBytes, GitHubDeviceCodeInfo& outInfo) noexcept;
[[nodiscard]] HRESULT ParseGitHubDevicePollJson(std::string_view jsonBytes, GitHubDevicePollResult& outResult) noexcept;
[[nodiscard]] std::wstring FormatGitHubErrorMessage(const GitHubHttpResponse& response);

// Explicit user-triggered opt-in helper to read token via installed GitHub CLI (`gh auth token`).
// Never called implicitly on startup or background polling.
[[nodiscard]] HRESULT TryImportGhCliToken(std::wstring& outToken) noexcept;
} // namespace RedTilde
