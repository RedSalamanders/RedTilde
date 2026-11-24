#pragma once
#include "Common/CredentialStore.h"
#include "Common/GitHubClient.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <windows.h>

namespace RedTilde
{
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
    Kind kind{Kind::LoadSavedAccount};
    std::uint64_t requestId{};
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    std::wstring filePath;
    std::wstring sha;
    std::wstring htmlUrl;
    std::wstring tokenOrDeviceCode;
    bool openAfterTree{};
};

struct GitHubResult
{
    GitHubRequest::Kind kind{GitHubRequest::Kind::LoadSavedAccount};
    std::uint64_t requestId{};
    HRESULT status{S_OK};
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
    bool openAfterTree{};
};

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
                                     std::shared_ptr<ICredentialStore> credentials) noexcept;
    [[nodiscard]] HRESULT Submit(GitHubRequest request) noexcept;
    [[nodiscard]] std::optional<GitHubResult> TakeResult();
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] bool Finished() const noexcept;
    void Stop() noexcept;
    void Close() noexcept;

private:
    struct State;
    std::shared_ptr<State> _state;
};
} // namespace RedTilde
