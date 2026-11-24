#include "Application.Tests.GitHub.h"
#include "Common/CredentialStore.h"
#include "Common/GitHubClient.h"
#include "Common/Recovery.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view message)
{
    if (! condition)
        throw std::runtime_error(std::string(message));
}

void Success(HRESULT result, std::string_view message)
{
    Require(SUCCEEDED(result), message);
}

void Pump(Application& app, unsigned turns = 8)
{
    for (unsigned turn = 0; turn < turns; ++turn)
    {
        MSG message{};
        while (PeekMessageW(&message, app.Window(), 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

template <class Predicate> void Await(Application& app, Predicate&& ready, std::string_view message)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (! ready())
    {
        Pump(app, 4);
        if (ready())
            return;
        if (std::chrono::steady_clock::now() >= deadline)
            Require(false, message);
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}

class MockGitHubTransport final : public IGitHubTransport
{
public:
    MockGitHubTransport()                                      = default;
    MockGitHubTransport(const MockGitHubTransport&)            = delete;
    MockGitHubTransport& operator=(const MockGitHubTransport&) = delete;
    MockGitHubTransport(MockGitHubTransport&&)                 = delete;
    MockGitHubTransport& operator=(MockGitHubTransport&&)      = delete;
    std::atomic_uint64_t requestCount{0u};

    [[nodiscard]] HRESULT SendRequest(const GitHubHttpRequest& request, const std::atomic_bool*, HANDLE, GitHubHttpResponse& outResponse) noexcept override
    {
        requestCount.fetch_add(1u, std::memory_order_relaxed);
        outResponse = {};
        if (request.host == L"api.github.com" && request.path == L"/user")
        {
            if (request.bearerToken != L"test-pat-123")
            {
                outResponse.httpStatusCode = 401u;
                return S_OK;
            }
            outResponse.httpStatusCode = 200u;
            outResponse.body           = "{\"login\":\"ada-writer\"}";
            return S_OK;
        }
        if (request.host == L"api.github.com" && request.path.find(L"/user/repos") == 0u)
        {
            outResponse.httpStatusCode = 200u;
            outResponse.body           = "["
                                         "{\"name\":\"RedTilde\",\"full_name\":\"RedSalamanders/RedTilde\",\"description\":\"Markdown "
                                         "writer\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/RedSalamanders/"
                                         "RedTilde\",\"private\":false,\"owner\":{\"login\":\"RedSalamanders\"}},"
                                         "{\"name\":\"PrivateNotes\",\"full_name\":\"ada-writer/PrivateNotes\",\"description\":\"Personal "
                                         "drafts\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/ada-writer/"
                                         "PrivateNotes\",\"private\":true,\"owner\":{\"login\":\"ada-writer\"}}"
                                         "]";
            return S_OK;
        }
        if (request.host == L"api.github.com" && request.path == L"/repos/RedSalamanders/RedTilde")
        {
            outResponse.httpStatusCode = 200u;
            outResponse.body           = "{\"default_branch\":\"main\"}";
            return S_OK;
        }
        if (request.host == L"api.github.com" && request.path.find(L"/repos/RedSalamanders/RedTilde/git/trees/") == 0u)
        {
            outResponse.httpStatusCode = 200u;
            outResponse.body           = "{\"truncated\":false,\"tree\":["
                                         "{\"path\":\"README.md\",\"type\":\"blob\",\"sha\":\"aaa111\"},"
                                         "{\"path\":\"docs\",\"type\":\"tree\",\"sha\":\"dir111\"},"
                                         "{\"path\":\"docs/Guide.markdown\",\"type\":\"blob\",\"sha\":\"bbb222\"},"
                                         "{\"path\":\"src/main.cpp\",\"type\":\"blob\",\"sha\":\"ccc333\"}"
                                         "]}";
            return S_OK;
        }
        if (request.host == L"api.github.com" && request.path == L"/repos/RedSalamanders/RedTilde/contents/README.md?ref=main")
        {
            outResponse.httpStatusCode = 200u;
            outResponse.body           = "# RedTilde from GitHub\r\nRead-only cached source.\r\n";
            return S_OK;
        }
        outResponse.httpStatusCode = 404u;
        return S_OK;
    }
};
} // namespace

class GitHubUiProbe final
{
public:
    static Application::OverlayKind ActiveOverlay(const Application& app) noexcept
    {
        return app._activeOverlay;
    }

    static DxUi::TextField* UrlInput(const Application& app) noexcept
    {
        return app._openGitHubUrlInput;
    }

    static DxUi::Tree* SuggestTree(const Application& app) noexcept
    {
        return app._githubSuggestTree;
    }

    static NavigationModel& SuggestModel(Application& app) noexcept
    {
        return app._githubSuggestModel;
    }

    static NavigationModel& FilesModel(Application& app) noexcept
    {
        return app._filesModel;
    }

    static const std::wstring& SignedInLogin(const Application& app) noexcept
    {
        return app._githubSignedInLogin;
    }

    static const auto& Session(const Application& app) noexcept
    {
        return app._session;
    }
};

void RegisterGitHubTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Editor.WorkspaceGitHubUrlAutosuggestAuthAndOpen",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;

        // 1. URL parsing & strict host/scheme validation
        const auto urlHttps = ParseGitHubUrl(L"https://github.com/RedSalamanders/RedTilde/blob/main/docs/Guide.md");
        Require(urlHttps.valid && urlHttps.owner == L"RedSalamanders" && urlHttps.repo == L"RedTilde" && urlHttps.branch == L"main" &&
                    urlHttps.filePath == L"docs/Guide.md",
                "ParseGitHubUrl extracts owner, repo, branch, and filePath from /blob URL");
        const auto shorthand = ParseGitHubUrl(L"RedSalamanders/DxUi.git");
        Require(shorthand.valid && shorthand.owner == L"RedSalamanders" && shorthand.repo == L"DxUi",
                "ParseGitHubUrl parses owner/repo and strips .git suffix");
        Require(! ParseGitHubUrl(L"http://github.com/a/b").valid && ! ParseGitHubUrl(L"https://evil.example.com/a/b").valid &&
                    ! ParseGitHubUrl(L"RedSalamanders/Repo/blob/main/../etc/passwd").valid,
                "ParseGitHubUrl rejects non-HTTPS, non-github.com hosts, and path traversal");

        // 2. Autosuggest ranking & MemoryCredentialStore scrubbing
        std::vector<GitHubRepoSummary> sampleRepos{
            {L"other", L"notes", L"other/notes", L"Misc markdown notes", L"main", L"", false},
            {L"RedSalamanders", L"RedTilde", L"RedSalamanders/RedTilde", L"Native Markdown editor", L"main", L"", false},
            {L"RedSalamanders", L"RedPrism", L"RedSalamanders/RedPrism", L"Image editor", L"main", L"", true},
        };
        const auto ranked = FilterAndRankGitHubRepos(sampleRepos, L"RedTilde", 10u);
        Require(ranked.size() == 1u && ranked[0].fullName == L"RedSalamanders/RedTilde", "FilterAndRankGitHubRepos matches repo name and ranks exact match");

        MemoryCredentialStore credStore;
        std::wstring secret = L"secret-token-value";
        Success(credStore.Write(L"RedTilde/GitHub/api.github.com", L"ada", secret), "write memory credential");
        ScrubWideString(secret);
        Require(secret.empty(), "ScrubWideString clears sensitive buffer");
        StoredCredential readCred;
        Success(credStore.Read(L"RedTilde/GitHub/api.github.com", readCred), "read memory credential");
        Require(readCred.username == L"ada" && readCred.secret == L"secret-token-value", "memory credential store round-trips");

        // 3. Integrated App flow with mock transport + zero startup network calls
        auto transport = std::make_shared<MockGitHubTransport>();
        auto appCreds  = std::make_shared<MemoryCredentialStore>();

        LaunchOptions launch;
        launch.headless               = true;
        launch.dataDirectory          = root / L"github-modal-test" / L"appdata";
        launch.githubTransportForTest = transport;
        launch.credentialStoreForTest = appCreds;
        launch.ghCliImporterForTest   = []() -> std::optional<std::wstring> { return L"test-pat-123"; };

        Application app(launch);
        Success(app.Initialize(), "initialize GitHub test app");
        Pump(app, 16);
        Require(transport->requestCount.load() == 0u, "zero unsolicited GitHub network requests occur at startup");

        // Open GitHub modal -> explicit user action
        app.Execute(Command::OpenGitHub);
        Require(GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::OpenGitHubModal, "OpenGitHub opens OpenGitHubModal");

        // Trigger explicit gh CLI import -> populates signed-in login + autosuggest cache
        app.Execute(Command::GitHubImportGhCli);
        Await(app, [&] {
            return GitHubUiProbe::SignedInLogin(app) == L"ada-writer" && ! GitHubUiProbe::SuggestModel(app).rows.empty();
        }, "explicit gh CLI import signs in @ada-writer and populates autosuggest repositories");

        GitHubUiProbe::UrlInput(app)->SetTextAndNotify(L"RedTilde");
        Require(GitHubUiProbe::SuggestModel(app).rows.size() == 1u && GitHubUiProbe::SuggestModel(app).rows[0].text == L"RedSalamanders/RedTilde",
                "typing in OpenGitHub URL input live-filters autosuggest tree");

        // Click suggestion -> fill URL -> Confirm -> fetches Markdown-only tree -> renders in Files pane
        GitHubUiProbe::SuggestModel(app).OnTreeItemInvoked(1u);
        app.Execute(Command::GitHubOpenConfirm);
        Await(app, [&] {
            return std::ranges::any_of(GitHubUiProbe::FilesModel(app).rows, [](const auto& row) { return row.text == L"README.md"; });
        }, "connected GitHub repo fetches tree and shows Markdown files in Files pane");
        Require(std::ranges::none_of(GitHubUiProbe::FilesModel(app).rows, [](const auto& row) { return row.text == L"main.cpp"; }),
                "GitHub tree filters out non-Markdown files like src/main.cpp");

        // Invoke README.md from GitHub tree -> opens read-through document requiring Save As
        const auto readmeRow = std::ranges::find_if(GitHubUiProbe::FilesModel(app).rows, [](const auto& row) { return row.text == L"README.md"; });
        Require(readmeRow != GitHubUiProbe::FilesModel(app).rows.end(), "README.md row is visible");
        GitHubUiProbe::FilesModel(app).OnTreeItemInvoked(readmeRow->id);
        Await(app, [&] {
            return GitHubUiProbe::Session(app).origin.kind == DocumentOriginInfo::Kind::GitHub;
        }, "invoking GitHub Markdown row loads content with GitHub origin");
        Require(app.RequiresSaveAs() && app.Document().GetText().find(L"RedTilde from GitHub") != std::wstring::npos,
                "GitHub document preserves content and requires Save As rather than writing remotely");

        // Sign Out clears login and autosuggest cache
        app.Execute(Command::GitHubSignOut);
        Await(app, [&] {
            return GitHubUiProbe::SignedInLogin(app).empty() && GitHubUiProbe::SuggestModel(app).rows.empty();
        }, "GitHub Sign Out clears signed-in identity and cached autosuggest repos");
        Require(foreground.NeverAcquiredForeground(), "GitHub modal and repo tests never acquire desktop focus");
    });
}
} // namespace RedTilde
