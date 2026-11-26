#pragma once
#include "Common/CredentialStore.h"
#include "Common/GitHubClient.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace RedTilde
{
inline constexpr std::string_view DefaultGitHubDeviceClientId = "";

[[nodiscard]] constexpr bool IsGitHubDeviceFlowConfigured() noexcept
{
    return ! DefaultGitHubDeviceClientId.empty();
}

struct GitHubRequest
{
    enum class Kind : std::uint8_t
    {
        LoadSavedAccount = 0,
        SignInWithPat    = 1,
        ImportGhCli      = 2,
        StartDeviceFlow  = 3,
        PollDeviceFlow   = 4,
        RefreshUserRepos = 5,
        FetchRepoTree    = 6,
        FetchRawFile     = 7,
        SignOut          = 8
    };
    GitHubRequest() noexcept                           = default;
    ~GitHubRequest() noexcept                          = default;
    GitHubRequest(const GitHubRequest&)                = delete;
    GitHubRequest& operator=(const GitHubRequest&)     = delete;
    GitHubRequest(GitHubRequest&&) noexcept            = default;
    GitHubRequest& operator=(GitHubRequest&&) noexcept = default;

    Kind kind{Kind::LoadSavedAccount};
    std::uint64_t requestId{};
    std::uint64_t flowId{};
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    std::wstring filePath;
    std::wstring sha;
    std::wstring htmlUrl;
    ScrubbedWideSecret tokenOrDeviceCode;
};

struct GitHubResult
{
    GitHubRequest::Kind kind{GitHubRequest::Kind::LoadSavedAccount};
    std::uint64_t requestId{};
    std::uint64_t flowId{};
    HRESULT status{S_OK};
    GitHubErrorCode errorCode{GitHubErrorCode::None};
    unsigned httpStatusCode{};
    bool authInvalidated{};
    std::wstring errorMessage;
    std::wstring accountLogin;
    bool signedIn{};
    std::vector<GitHubRepoSummary> userRepos;
    std::optional<GitHubTreeSnapshot> tree;
    std::optional<GitHubDeviceCodeInfo> deviceCode;
    std::optional<GitHubDevicePollResult> devicePoll;
    std::wstring fileOwner;
    std::wstring fileRepo;
    std::wstring fileBranch;
    std::wstring fileRelativePath;
    std::wstring fileSha;
    std::wstring fileHtmlUrl;
    std::wstring fileText;
    bool fileBom{};
};

[[nodiscard]] std::wstring FormatLocalizedGitHubError(GitHubErrorCode errorCode,
                                                      HRESULT transportStatus         = S_OK,
                                                      unsigned httpStatusCode         = 0u,
                                                      std::wstring_view serverMessage = {});

class GitHubService final
{
public:
    GitHubService() = default;
    ~GitHubService();
    GitHubService(const GitHubService&)            = delete;
    GitHubService& operator=(const GitHubService&) = delete;

    [[nodiscard]] HRESULT Initialize(HWND window,
                                     UINT message,
                                     std::shared_ptr<IGitHubTransport> transport,
                                     std::shared_ptr<ICredentialStore> credentials,
                                     std::string deviceClientId = std::string(DefaultGitHubDeviceClientId),
                                     std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)> ghCliImporter = {}) noexcept;
    [[nodiscard]] HRESULT Submit(GitHubRequest request) noexcept;
    [[nodiscard]] std::optional<GitHubResult> TakeResult();
    [[nodiscard]] bool DeviceFlowConfigured() const noexcept;
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    void Stop() noexcept;
    void Close() noexcept;

private:
    struct State;
    std::shared_ptr<State> _state;
};
} // namespace RedTilde
