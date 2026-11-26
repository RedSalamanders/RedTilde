// Winsock must precede windows.h and WIL's conditional socket declarations.
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
// clang-format on
#include "Application.Tests.GitHub.h"
#include "Common/CredentialStore.h"
#include "Common/GitHubClient.h"
#include "Common/Recovery.h"
#include "Common/RemoteImageImports.h"
#include "Common/Settings.h"
#include "RedTilde/Application.h"
#include "RedTilde/GitHubService.h"
#include "Support/Support.Tests.FilesFixtures.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <wil/resource.h>

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

struct LoopbackResponse
{
    std::string status = "200 OK";
    std::string headers;
    std::string body;
    bool waitForRelease{};
};

#pragma warning(push)
#pragma warning(disable : 4625 4626)
class LoopbackGitHubServer final
{
public:
    explicit LoopbackGitHubServer(std::function<LoopbackResponse(std::string_view, std::string_view)> handler) : _respond(std::move(handler))
    {
        Success(EnsureDelayLoadedImport("WS2_32.dll"), "load delay-loaded WS2_32 for loopback GitHub test");
        WSADATA data{};
        Require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "initialize loopback GitHub sockets");
        _winsock.activate();
        _stop.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        _received.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        _release.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        _changed.reset(WSACreateEvent());
        _listener.reset(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        Require(_stop && _received && _release && _changed && _listener, "create loopback GitHub resources");
        BOOL exclusive = TRUE;
        Require(setsockopt(_listener.get(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == 0,
                "exclusive loopback GitHub socket");
        sockaddr_in address{};
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(bind(_listener.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0 && listen(_listener.get(), 8) == 0,
                "bind IPv4 loopback GitHub socket");
        int size = sizeof(address);
        Require(getsockname(_listener.get(), reinterpret_cast<sockaddr*>(&address), &size) == 0, "query loopback GitHub port");
        _port = ntohs(address.sin_port);
        Require(WSAEventSelect(_listener.get(), _changed.get(), FD_ACCEPT | FD_CLOSE) == 0, "select loopback accept events");
        _worker = std::jthread([this] { Run(); });
    }

    ~LoopbackGitHubServer()
    {
        (void)SetEvent(_stop.get());
        (void)SetEvent(_release.get());
    }

    [[nodiscard]] GitHubLoopbackEndpoint Endpoint() const noexcept
    {
        return GitHubLoopbackEndpoint{.host = L"127.0.0.1", .port = _port, .useHttps = false};
    }

    [[nodiscard]] unsigned Count() const noexcept
    {
        return _requests.load();
    }

    [[nodiscard]] std::vector<std::string> Requests()
    {
        std::lock_guard lock(_mutex);
        return _recordedRequests;
    }

    void AwaitRequest() const
    {
        Require(WaitForSingleObject(_received.get(), 5000) == WAIT_OBJECT_0 && _failures.load() == 0u, "loopback GitHub server received request");
    }

    void Release() noexcept
    {
        (void)SetEvent(_release.get());
    }

private:
    using SocketEvent = wil::unique_any<WSAEVENT, decltype(&WSACloseEvent), WSACloseEvent>;
    wil::unique_call<decltype(&WSACleanup), WSACleanup, false> _winsock;
    wil::unique_socket _listener;
    SocketEvent _changed;
    wil::unique_handle _stop, _received, _release;
    std::function<LoopbackResponse(std::string_view, std::string_view)> _respond;
    std::mutex _mutex;
    std::vector<std::string> _recordedRequests;
    std::atomic<unsigned> _requests{}, _failures{};
    unsigned short _port{};
    std::jthread _worker;

    static bool Send(SOCKET client, std::string_view bytes) noexcept
    {
        while (! bytes.empty())
        {
            const int count = send(client, bytes.data(), static_cast<int>(std::min<std::size_t>(bytes.size(), 65536u)), 0);
            if (count <= 0)
                return false;
            bytes.remove_prefix(static_cast<std::size_t>(count));
        }
        return true;
    }

    void Run() noexcept
    try
    {
        const std::array<HANDLE, 2> events{_stop.get(), _changed.get()};
        while (WaitForMultipleObjects(2u, events.data(), FALSE, INFINITE) == WAIT_OBJECT_0 + 1u)
        {
            WSANETWORKEVENTS network{};
            if (WSAEnumNetworkEvents(_listener.get(), _changed.get(), &network) == SOCKET_ERROR)
                break;
            for (;;)
            {
                wil::unique_socket client(accept(_listener.get(), nullptr, nullptr));
                if (! client)
                    break;
                if (WSAEventSelect(client.get(), nullptr, 0) == SOCKET_ERROR)
                    break;
                u_long blocking{};
                DWORD timeout = 1000;
                if (ioctlsocket(client.get(), FIONBIO, &blocking) == SOCKET_ERROR ||
                    setsockopt(client.get(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == SOCKET_ERROR ||
                    setsockopt(client.get(), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == SOCKET_ERROR)
                    continue;
                std::string request;
                std::array<char, 4096> bytes{};
                const auto deadline = GetTickCount64() + 1500;
                while (! request.contains("\r\n\r\n") && request.size() < 32768u && GetTickCount64() < deadline &&
                       WaitForSingleObject(_stop.get(), 0u) != WAIT_OBJECT_0)
                {
                    const int count = recv(client.get(), bytes.data(), static_cast<int>(bytes.size()), 0);
                    if (count <= 0)
                        break;
                    request.append(bytes.data(), static_cast<std::size_t>(count));
                }
                const auto headerEnd = request.find("\r\n\r\n");
                if (headerEnd == std::string::npos)
                    continue;
                std::size_t contentLength{};
                const auto clPos = request.find("Content-Length:");
                if (clPos != std::string::npos && clPos < headerEnd)
                {
                    const auto lineEnd = request.find("\r\n", clPos);
                    std::string_view val(request.data() + clPos + 15u, lineEnd - (clPos + 15u));
                    while (! val.empty() && val.front() == ' ')
                        val.remove_prefix(1u);
                    for (const char ch : val)
                        if (ch >= '0' && ch <= '9')
                            contentLength = contentLength * 10u + static_cast<std::size_t>(ch - '0');
                }
                while (request.size() < headerEnd + 4u + contentLength && GetTickCount64() < deadline && WaitForSingleObject(_stop.get(), 0u) != WAIT_OBJECT_0)
                {
                    const int count = recv(client.get(), bytes.data(), static_cast<int>(bytes.size()), 0);
                    if (count <= 0)
                        break;
                    request.append(bytes.data(), static_cast<std::size_t>(count));
                }

                const auto first = request.find(' '), last = request.find(' ', first + 1u);
                if (first == std::string::npos || last == std::string::npos)
                    continue;
                const std::string_view targetPath(request.data() + first + 1u, last - first - 1u);
                const auto response = _respond(targetPath, request);
                {
                    std::lock_guard lock(_mutex);
                    _recordedRequests.push_back(request);
                }
                ++_requests;
                (void)SetEvent(_received.get());
                if (response.waitForRelease)
                {
                    const std::array<HANDLE, 2> completion{_stop.get(), _release.get()};
                    if (WaitForMultipleObjects(2u, completion.data(), FALSE, 5000) != WAIT_OBJECT_0 + 1u)
                        continue;
                }
                std::string header = "HTTP/1.1 " + response.status + "\r\nConnection: close\r\n" + response.headers +
                                     std::format("Content-Length: {}\r\n\r\n", response.body.size());
                if (Send(client.get(), header))
                    (void)Send(client.get(), response.body);
                (void)shutdown(client.get(), SD_SEND);
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        std::terminate();
    }
    catch (const std::exception&)
    {
        ++_failures;
        (void)SetEvent(_received.get());
    }
};
#pragma warning(pop)

class MockGitHubTransport final : public IGitHubTransport
{
public:
    MockGitHubTransport()                                      = default;
    MockGitHubTransport(const MockGitHubTransport&)            = delete;
    MockGitHubTransport& operator=(const MockGitHubTransport&) = delete;
    MockGitHubTransport(MockGitHubTransport&&)                 = delete;
    MockGitHubTransport& operator=(MockGitHubTransport&&)      = delete;
    std::atomic_uint64_t requestCount{0u};
    std::function<HRESULT(const GitHubHttpRequest&, const std::atomic_bool*, HANDLE, GitHubHttpResponse&)> customHandler;

    [[nodiscard]] HRESULT SendRequest(const GitHubHttpRequest& request,
                                      const std::atomic_bool* cancellation,
                                      HANDLE abortEvent,
                                      GitHubHttpResponse& outResponse) noexcept override
    {
        requestCount.fetch_add(1u, std::memory_order_relaxed);
        outResponse = {};
        if (customHandler)
            return customHandler(request, cancellation, abortEvent, outResponse);
        if (request.host == L"api.github.com" && request.path == L"/user")
        {
            if (request.bearerToken != L"test-pat-123")
            {
                outResponse.httpStatusCode = 401u;
                outResponse.errorCode      = GitHubErrorCode::Unauthorized;
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
        outResponse.errorCode      = GitHubErrorCode::NotFound;
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

    static DxUi::TextField* BranchInput(const Application& app) noexcept
    {
        return app._openGitHubBranchInput;
    }

    static DxUi::TextField* ModalTokenInput(const Application& app) noexcept
    {
        return app._openGitHubTokenInput;
    }

    static DxUi::TextField* DrawerTokenInput(const Application& app) noexcept
    {
        return app._accountsTokenInput;
    }

    static DxUi::Button* ModalDeviceButton(const Application& app) noexcept
    {
        return app._openGitHubDeviceBtn;
    }

    static DxUi::Button* DrawerDeviceButton(const Application& app) noexcept
    {
        return app._accountsDeviceBtn;
    }

    static DxUi::Button* ModalImportGhButton(const Application& app) noexcept
    {
        return app._openGitHubImportGhBtn;
    }

    static DxUi::Button* DrawerImportGhButton(const Application& app) noexcept
    {
        return app._accountsImportGhBtn;
    }

    static DxUi::Button* OpenGitHubOpenButton(const Application& app) noexcept
    {
        return app._openGitHubOpenBtn;
    }

    static DxUi::Button* OpenGitHubCancelButton(const Application& app) noexcept
    {
        return app._openGitHubCancelBtn;
    }

    static DxUi::Button* OpenGitHubUseTokenButton(const Application& app) noexcept
    {
        return app._openGitHubUseTokenBtn;
    }

    static Surface* OpenGitHubPanel(const Application& app) noexcept
    {
        return app._openGitHubPanel;
    }

    static DxUi::Label* OpenGitHubAuthLabel(const Application& app) noexcept
    {
        return app._openGitHubAuthLabel;
    }

    static DxUi::Label* DrawerAccountsStatusLabel(const Application& app) noexcept
    {
        return app._accountsStatusLabel;
    }

    static DxUi::Label* ModalErrorLabel(const Application& app) noexcept
    {
        return app._openGitHubErrorLabel;
    }

    static DxUi::Label* FeedbackLabel(const Application& app) noexcept
    {
        return app._feedback;
    }

    static DxUi::Label* FilesCountLabel(const Application& app) noexcept
    {
        return app._filesCount;
    }

    static DxUi::Label* TitleLabel(const Application& app) noexcept
    {
        return app._title;
    }

    static DxUi::TextField* FilesFilter(const Application& app) noexcept
    {
        return app._filesFilter;
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

    static const auto& ConnectedRepos(const Application& app) noexcept
    {
        return app._connectedGitHubRepos;
    }

    static const auto& CollapsedGitHubRepos(const Application& app) noexcept
    {
        return app._collapsedGitHubRepos;
    }

    static const auto& PendingDeviceFlow(const Application& app) noexcept
    {
        return app._pendingDeviceFlow;
    }

    static void ExpireDeviceFlowNow(Application& app) noexcept
    {
        if (app._pendingDeviceFlow)
            app._pendingDeviceFlow->deadlineTickMs = GetTickCount64() - 1ull;
    }

    static const GitHubService& Service(const Application& app) noexcept
    {
        return app._github;
    }

    static constexpr UINT_PTR DevicePollTimerId() noexcept
    {
        return Application::GitHubDevicePollTimerId;
    }

    static constexpr std::uint64_t RepoRowBaseId() noexcept
    {
        return Application::GitHubRowBaseId;
    }

    static constexpr std::uint64_t GitHubSectionGroupId() noexcept
    {
        return Application::GitHubSectionGroupId;
    }

    static const auto& Session(const Application& app) noexcept
    {
        return app._session;
    }

    static void RefreshFilesNow(Application& app)
    {
        app.RefreshFiles();
    }

    static void OpenGitHubDocumentNow(Application& app, std::wstring_view owner, std::wstring_view repo, std::wstring_view branch, std::wstring_view path)
    {
        app.OpenGitHubDocument(owner, repo, branch, path);
    }

    static void AddFilesRootNow(Application& app, const std::filesystem::path& path)
    {
        app.AddFilesRoot(path);
    }

    static void OpenPathNow(Application& app, const std::filesystem::path& path)
    {
        app.OpenPath(path);
    }

    static bool FilesBusy(const Application& app) noexcept
    {
        return app._files.Busy();
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

        // Open GitHub modal -> explicit user action & 800x560 layout non-clipping bounds check
        app.Execute(Command::OpenGitHub);
        Require(GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::OpenGitHubModal, "OpenGitHub opens OpenGitHubModal");
        {
            RECT rect{};
            Require(GetWindowRect(app.Window(), &rect) != FALSE, "read GitHub modal test window rect");
            const int dpi = static_cast<int>(app.Host().GetDpi());
            rect.right    = rect.left + MulDiv(800, dpi, 96);
            rect.bottom   = rect.top + MulDiv(560, dpi, 96);
            Require(SetWindowPos(app.Window(),
                                 nullptr,
                                 rect.left,
                                 rect.top,
                                 rect.right - rect.left,
                                 rect.bottom - rect.top,
                                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                    "resize hidden window to 800x560 minimum size");
            app.Prepare(16);
            const auto client = app.Host().GetClientBoundsDip();
            Require(client.right >= 799.f && client.bottom >= 559.f && client.bottom <= 561.f, "800x560 DIP client bounds applied");
            const auto panel     = GitHubUiProbe::OpenGitHubPanel(app)->GetBounds();
            const auto openBtn   = GitHubUiProbe::OpenGitHubOpenButton(app)->GetBounds();
            const auto cancelBtn = GitHubUiProbe::OpenGitHubCancelButton(app)->GetBounds();
            const auto errLbl    = GitHubUiProbe::ModalErrorLabel(app)->GetBounds();
            const auto treeB     = GitHubUiProbe::SuggestTree(app)->GetBounds();
            const auto importB   = GitHubUiProbe::ModalImportGhButton(app)->GetBounds();
            const auto tokB      = GitHubUiProbe::ModalTokenInput(app)->GetBounds();
            const auto authLbl   = GitHubUiProbe::OpenGitHubAuthLabel(app)->GetBounds();
            Require(panel.left >= 0.f && panel.top >= 0.f && panel.right <= client.right && panel.bottom <= client.bottom,
                    "OpenGitHubModal panel stays inside 800x560 client bounds");
            Require(openBtn.bottom <= panel.bottom && cancelBtn.bottom <= panel.bottom && errLbl.bottom <= panel.bottom && treeB.top >= authLbl.bottom &&
                        treeB.bottom <= errLbl.top && treeB.bottom <= openBtn.top && importB.right <= tokB.left,
                    "OpenGitHubModal buttons, suggest tree, and labels fit cleanly without clipping or overlap at 800x560");
            Require(GitHubUiProbe::DrawerImportGhButton(app) && GitHubUiProbe::DrawerImportGhButton(app)->IsVisible(),
                    "DrawerImportGhButton remains visible when Device Flow is unconfigured");
        }

        // Trigger explicit async gh CLI import -> populates signed-in login + autosuggest cache
        app.Execute(Command::GitHubImportGhCli);
        Await(app, [&] {
            return GitHubUiProbe::SignedInLogin(app) == L"ada-writer" && ! GitHubUiProbe::SuggestModel(app).rows.empty();
        }, "explicit gh CLI import signs in @ada-writer and populates autosuggest repositories");

        GitHubUiProbe::UrlInput(app)->SetTextAndNotify(L"NoSuchRepoMatchesFilter999");
        Require(GitHubUiProbe::SuggestModel(app).rows.empty() &&
                    GitHubUiProbe::SuggestTree(app)->GetEmptyStateText() == LoadStringResource(nullptr, IDS_NOMATCH),
                "signed-in non-matching OpenGitHub filter shows IDS_NOMATCH empty text");

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

        // Invoke README.md from GitHub tree -> opens read-through document requiring Save As with GitHub leaf display name
        const auto readmeRow = std::ranges::find_if(GitHubUiProbe::FilesModel(app).rows, [](const auto& row) { return row.text == L"README.md"; });
        Require(readmeRow != GitHubUiProbe::FilesModel(app).rows.end(), "README.md row is visible");
        GitHubUiProbe::FilesModel(app).OnTreeItemInvoked(readmeRow->id);
        Await(app, [&] {
            return GitHubUiProbe::Session(app).origin.kind == DocumentOriginInfo::Kind::GitHub;
        }, "invoking GitHub Markdown row loads content with GitHub origin");
        Require(app.RequiresSaveAs() && app.Document().GetText().find(L"RedTilde from GitHub") != std::wstring::npos &&
                    GitHubUiProbe::TitleLabel(app)->GetText() == L"README.md",
                "GitHub document preserves content, title leaf name, and requires Save As rather than writing remotely");

        // SettingsDrawer _accountsTokenInput clearing, no cross-surface fallback, and SignOut clearing
        app.Execute(Command::Settings);
        Require(GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::SettingsDrawer, "Settings opens SettingsDrawer");
        const auto countBeforeCrossSurface = transport->requestCount.load();
        GitHubUiProbe::ModalTokenInput(app)->SetText(L"wrong-surface-modal-token");
        app.Execute(Command::GitHubUseToken);
        Pump(app, 8);
        Require(transport->requestCount.load() == countBeforeCrossSurface, "GitHubUseToken in SettingsDrawer never falls back to _openGitHubTokenInput");
        GitHubUiProbe::DrawerTokenInput(app)->SetText(L"drawer-secret-pat");
        app.Execute(Command::CloseDrawer);
        Require(GitHubUiProbe::DrawerTokenInput(app)->GetText().empty() && GitHubUiProbe::ModalTokenInput(app)->GetText().empty(),
                "leaving SettingsDrawer clears both _accountsTokenInput and _openGitHubTokenInput");

        app.Execute(Command::OpenGitHub);
        GitHubUiProbe::DrawerTokenInput(app)->SetText(L"wrong-surface-drawer-token");
        app.Execute(Command::GitHubUseToken);
        Pump(app, 8);
        Require(transport->requestCount.load() == countBeforeCrossSurface, "GitHubUseToken in OpenGitHubModal never falls back to _accountsTokenInput");
        GitHubUiProbe::ModalTokenInput(app)->SetText(L"modal-secret-before-signout");

        // Sign Out clears login, autosuggest cache, and token inputs
        app.Execute(Command::GitHubSignOut);
        Await(app, [&] {
            return GitHubUiProbe::SignedInLogin(app).empty() && GitHubUiProbe::SuggestModel(app).rows.empty();
        }, "GitHub Sign Out clears signed-in identity and cached autosuggest repos");
        Require(GitHubUiProbe::ModalTokenInput(app)->GetText().empty() && GitHubUiProbe::DrawerTokenInput(app)->GetText().empty(),
                "GitHubSignOut clears both PAT inputs");

        // P1-02: WM_CLOSE is blocked while OpenGitHubModal is active; CloseDrawer scrubs PATs; SettingsDrawer + WM_CLOSE scrubs repopulated PATs in BeginClose.
        GitHubUiProbe::ModalTokenInput(app)->SetText(L"modal-pat-at-close");
        GitHubUiProbe::DrawerTokenInput(app)->SetText(L"drawer-pat-at-close");
        (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
        Pump(app, 4);
        Require(! app.IsClosing() && app.Window() != nullptr && GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::OpenGitHubModal &&
                    GitHubUiProbe::ModalTokenInput(app)->GetText() == L"modal-pat-at-close",
                "WM_CLOSE is blocked while OpenGitHubModal is active");
        app.Execute(Command::CloseDrawer);
        Require(GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::None && GitHubUiProbe::ModalTokenInput(app)->GetText().empty() &&
                    GitHubUiProbe::DrawerTokenInput(app)->GetText().empty(),
                "CloseDrawer dismisses OpenGitHubModal and scrubs both PAT inputs");
        app.Execute(Command::Settings);
        GitHubUiProbe::DrawerTokenInput(app)->SetText(L"drawer-pat-at-close");
        const auto modalLifetime  = GetControlLifetimeToken(*GitHubUiProbe::ModalTokenInput(app));
        const auto drawerLifetime = GetControlLifetimeToken(*GitHubUiProbe::DrawerTokenInput(app));
        (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
        Await(app, [&] { return app.IsClosing() && app.AreShutdownWorkersSettled(); }, "WM_CLOSE settles cleanly with populated PAT inputs");
        Require(modalLifetime.expired() && drawerLifetime.expired(), "BeginClose detaches and frees PAT input controls cleanly under ASan");
        Require(foreground.NeverAcquiredForeground(), "GitHub modal and repo tests never acquire desktop focus");
    });

    test("Editor.GitHubUrlBranchAndDecodeValidation",
         [&]
    {
        // Percent-decoding without double-encoding & slash branches
        const auto decodedBlob = ParseGitHubUrl(L"https://github.com/RedSalamanders/RedTilde/blob/main/docs/User%20Guide%20%28v1%29.md");
        Require(decodedBlob.valid && decodedBlob.branch == L"main" && decodedBlob.filePath == L"docs/User Guide (v1).md",
                "ParseGitHubUrl percent-decodes HTTPS segments without double encoding");

        const auto slashTree = ParseGitHubUrl(L"https://github.com/RedSalamanders/RedTilde/tree/feature/foo/bar");
        Require(slashTree.valid && slashTree.branch == L"feature/foo/bar" && slashTree.filePath.empty(),
                "ParseGitHubUrl preserves multi-segment /tree/feature/foo/bar branch names");

        Require(! ParseGitHubUrl(L"https://github.com/a/b/blob/main/%Q0").valid &&
                    ParseGitHubUrl(L"https://github.com/a/b/blob/main/%Q0").errorCode == GitHubErrorCode::UrlTooLongOrInvalidEncoding,
                "ParseGitHubUrl rejects malformed %Q0 escape");
        Require(! ParseGitHubUrl(L"https://github.com/a/b/blob/main/%2e%2e/secret.md").valid &&
                    ! ParseGitHubUrl(L"https://github.com/a/b/blob/main/a%2fb.md").valid &&
                    ! ParseGitHubUrl(L"https://github.com/a/b/blob/main/a%00b.md").valid && ! ParseGitHubUrl(L"https://github.com/a/b/blob/main").valid,
                "ParseGitHubUrl rejects encoded dot-segments, encoded slashes/NULs, and /blob without filePath");

        // Challenge 5: Preserve single trailing slash on repo while rejecting empty segments and trailing slashes in /tree/ and /blob/
        const auto trailingRepo = ParseGitHubUrl(L"https://github.com/o/r/");
        Require(trailingRepo.valid && trailingRepo.owner == L"o" && trailingRepo.repo == L"r" && trailingRepo.branch.empty() && trailingRepo.filePath.empty(),
                "ParseGitHubUrl preserves single trailing slash after repo");
        for (const auto* badTree : {L"https://github.com/o/r/tree//main",
                                    L"https://github.com/o/r/tree/main/",
                                    L"https://github.com/o/r/tree/a//b",
                                    L"https://github.com/o/r/tree/",
                                    L"https://github.com/o/r/blob//main/Guide.md"})
        {
            const auto res = ParseGitHubUrl(badTree);
            Require(! res.valid && res.errorCode == GitHubErrorCode::InvalidBranch, "ParseGitHubUrl rejects empty segments in /tree/ or /blob/ branch");
        }
        for (const auto* badBlobPath :
             {L"https://github.com/o/r/blob/main/", L"https://github.com/o/r/blob/main//Guide.md", L"https://github.com/o/r/blob/main/docs/Guide.md/"})
        {
            const auto res = ParseGitHubUrl(badBlobPath);
            Require(! res.valid && res.errorCode == GitHubErrorCode::InvalidFilePath, "ParseGitHubUrl rejects empty segments/trailing slash in /blob/ path");
        }
        const auto emptyOwnerRepo = ParseGitHubUrl(L"o//r/tree/main");
        Require(! emptyOwnerRepo.valid && emptyOwnerRepo.errorCode == GitHubErrorCode::InvalidOwnerOrRepo,
                "ParseGitHubUrl rejects empty owner/repo separator in o//r/tree/main");

        // Shared branch validator matrix
        Require(IsValidGitHubBranchName(L"main") && IsValidGitHubBranchName(L"feature/foo/bar") && IsValidGitHubBranchName(L"release-1.0_rc"),
                "IsValidGitHubBranchName accepts valid single and slash branches");
        for (const auto* bad :
             {L"", L"/lead", L"trail/", L"a//b", L"a..b", L"a@{b}", L".hidden", L"main.lock", L"a/b.lock", L"bad branch", L"a?b", L"a#b", L"a%20b", L"a\\b"})
            Require(! IsValidGitHubBranchName(bad), "IsValidGitHubBranchName rejects invalid/traversal branch names");

        // Settings validation drops hostile branches while keeping valid slash branches
        Settings parsed;
        Success(TryParseSettings("{\"schema\":1,\"githubRepos\":[{\"owner\":\"RedSalamanders\",\"repo\":\"RedTilde\",\"branch\":\"feature/foo\"},"
                                 "{\"owner\":\"RedSalamanders\",\"repo\":\"Evil\",\"branch\":\"../other\"}]}",
                                 parsed),
                "parse settings with valid and invalid GitHub branches");
        Require(parsed.githubRepos.size() == 1u && parsed.githubRepos[0].branch == L"feature/foo",
                "TryParseSettings drops invalid GitHub branch traversal entries");

        // Repo/branch/device JSON decoder validation (full_name spoofing, exact repo html_url, invalid/truncated/bidi fields)
        std::vector<GitHubRepoSummary> repos;
        const std::string over256Branch = std::string(256u, 'a') + "/../evil";
        const std::string over128Login  = std::string(129u, 'u');
        const std::string reposPayload =
            "["
            "{\"name\":\"Spoof\",\"full_name\":\"victim/other\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/a/"
            "Spoof\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"BadUrl\",\"full_name\":\"a/BadUrl\",\"default_branch\":\"main\",\"html_url\":\"https://evil.example.com/"
            "phish\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"OtherRepoUrl\",\"full_name\":\"a/OtherRepoUrl\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/"
            "attacker/phish\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"Good\",\"full_name\":\"a/Good\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/a/"
            "Good-evil\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"Good\",\"full_name\":\"a/Good\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/a/"
            "Good\\u202E\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"Good\",\"full_name\":\"a/Good\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/a/"
            "Good\\u00A0\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"BadBranch\",\"full_name\":\"a/BadBranch\",\"default_branch\":\"../evil\",\"html_url\":\"https://"
            "github.com/a/BadBranch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"OverBranch\",\"full_name\":\"a/OverBranch\",\"default_branch\":\"" +
            over256Branch +
            "\",\"html_url\":\"https://github.com/a/OverBranch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"BidiBranch\",\"full_name\":\"a/BidiBranch\",\"default_branch\":\"a\\u202Eb\",\"html_url\":\"https://"
            "github.com/a/BidiBranch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"DelBranch\",\"full_name\":\"a/DelBranch\",\"default_branch\":\"a\\u007Fb\",\"html_url\":\"https://"
            "github.com/a/DelBranch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"C1Branch\",\"full_name\":\"a/C1Branch\",\"default_branch\":\"a\\u0085b\",\"html_url\":\"https://"
            "github.com/a/C1Branch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"NonStrBranch\",\"full_name\":\"a/NonStrBranch\",\"default_branch\":123,\"html_url\":\"https://"
            "github.com/a/NonStrBranch\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"OverLogin\",\"full_name\":\"" +
            over128Login + "/OverLogin\",\"default_branch\":\"main\",\"owner\":{\"login\":\"" + over128Login +
            "\"}},"
            "{\"name\":\"Good\",\"full_name\":\"a/Good\",\"default_branch\":\"release/v1\",\"html_url\":\"https://GITHUB.COM/a/"
            "Good/tree/main\",\"owner\":{\"login\":\"a\"}},"
            "{\"name\":\"QueryGood\",\"full_name\":\"a/QueryGood\",\"default_branch\":\"main\",\"html_url\":\"https://github.com/a/"
            "QueryGood?tab=readme\",\"owner\":{\"login\":\"a\"}}"
            "]";
        Success(ParseGitHubReposPageJson(reposPayload, repos), "parse repo page JSON");
        Require(repos.size() == 2u && repos[0].fullName == L"a/Good" && repos[0].defaultBranch == L"release/v1" && repos[1].fullName == L"a/QueryGood",
                "ParseGitHubReposPageJson rejects spoofed full_name, cross-repo/prefix/bidi/whitespace html_url, and truncated/sanitized/non-string branches");

        std::wstring loginOut;
        Require(FAILED(ParseGitHubUserLoginJson("{\"login\":\"a\\u202Eb\"}", loginOut)) && loginOut.empty(),
                "ParseGitHubUserLoginJson rejects bidi login without stripping");
        Require(FAILED(ParseGitHubUserLoginJson(std::format("{{\"login\":\"{}\"}}", over128Login), loginOut)) && loginOut.empty(),
                "ParseGitHubUserLoginJson rejects >128 char login without truncating");

        std::wstring defBranch = L"stale";
        Require(FAILED(ParseGitHubDefaultBranchJson("{\"default_branch\":\"../secret\"}", defBranch)) && defBranch.empty(),
                "ParseGitHubDefaultBranchJson rejects traversal default_branch");
        defBranch = L"stale";
        Require(FAILED(ParseGitHubDefaultBranchJson(std::format("{{\"default_branch\":\"{}\"}}", over256Branch), defBranch)) && defBranch.empty(),
                "ParseGitHubDefaultBranchJson rejects >256 char branch instead of truncating");
        Success(ParseGitHubDefaultBranchJson("{\"default_branch\":\"feature/v2\"}", defBranch), "ParseGitHubDefaultBranchJson accepts slash branch");
        Require(defBranch == L"feature/v2", "ParseGitHubDefaultBranchJson returns validated slash branch");

        GitHubDeviceCodeInfo dcInfo;
        Require(
            FAILED(ParseGitHubDeviceCodeJson("{\"device_code\":\"dc\",\"user_code\":\"UC\",\"verification_uri\":\"https://evil.example.com/login\"}", dcInfo)),
            "ParseGitHubDeviceCodeJson rejects non-github.com verification_uri");
        dcInfo.deviceCode = L"stale_dc";
        Require(FAILED(ParseGitHubDeviceCodeJson(
                    "{\"device_code\":\"dc\",\"user_code\":\"UC\",\"verification_uri\":\"https://github.com/login/device\\u202E\"}", dcInfo)) &&
                    dcInfo.deviceCode.empty(),
                "ParseGitHubDeviceCodeJson rejects bidi verification_uri and scrubs deviceCode");
        Require(FAILED(ParseGitHubDeviceCodeJson(
                    "{\"device_code\":\"dc\\u202E1\",\"user_code\":\"UC\",\"verification_uri\":\"https://github.com/login/device\"}", dcInfo)) &&
                    dcInfo.deviceCode.empty(),
                "ParseGitHubDeviceCodeJson rejects bidi in device_code");
        Require(FAILED(ParseGitHubDeviceCodeJson("{\"device_code\":\"dc\",\"user_code\":\"UC 1\",\"verification_uri\":\"https://github.com/login/device\"}",
                                                 dcInfo)) &&
                    dcInfo.deviceCode.empty(),
                "ParseGitHubDeviceCodeJson rejects whitespace in user_code");

        // Tree JSON: ZWJ/ZWNJ preservation in entry.path, >1024 path drop, htmlUrl percent encoding, truncated & exact 5000 vs 5001 boundary
        GitHubTreeSnapshot zwjSnap;
        const std::string overPath(1022u, 'p');
        const std::string zwjJson = std::format("{{\"truncated\":true,\"tree\":["
                                                "{{\"path\":\"docs/a\xE2\x80\x8D"
                                                "b c.md\",\"type\":\"blob\",\"sha\":\"111\"}},"
                                                "{{\"path\":\"{}.md\",\"type\":\"blob\",\"sha\":\"222\"}}"
                                                "]}}",
                                                overPath);
        Success(ParseGitHubTreeJson(zwjJson, L"RedSalamanders", L"RedTilde", L"feature/v1", zwjSnap), "parse tree JSON with ZWJ and >1024 path");
        Require(zwjSnap.truncated && zwjSnap.entries.size() == 2u && zwjSnap.markdownCount == 1u && zwjSnap.entries[1].path == L"docs/a\u200Db c.md" &&
                    zwjSnap.entries[1].htmlUrl.find(L"/blob/feature/v1/docs/a%E2%80%8Db%20c.md") != std::wstring::npos,
                "ParseGitHubTreeJson preserves ZWJ, drops >1024-char path without truncating, sets truncated, and percent-encodes htmlUrl");

        std::string exactTree = "{\"truncated\":false,\"tree\":[";
        for (std::size_t i = 0u; i < MaximumGitHubTreeEntries; ++i)
        {
            if (i > 0u)
                exactTree.push_back(',');
            exactTree += std::format("{{\"path\":\"e{}.md\",\"type\":\"blob\",\"sha\":\"s{}\"}}", i, i);
        }
        exactTree += "]}";
        GitHubTreeSnapshot exactSnap;
        Success(ParseGitHubTreeJson(exactTree, L"RedSalamanders", L"RedTilde", L"main", exactSnap), "parse exact 5000-entry tree");
        Require(! exactSnap.limitReached && exactSnap.markdownCount == MaximumGitHubTreeEntries,
                "ParseGitHubTreeJson keeps limitReached == false at exact MaximumGitHubTreeEntries (5000) boundary");

        std::string bigTree = "{\"truncated\":false,\"tree\":[";
        for (std::size_t i = 0u; i <= MaximumGitHubTreeEntries; ++i)
        {
            if (i > 0u)
                bigTree.push_back(',');
            bigTree += std::format("{{\"path\":\"f{}.md\",\"type\":\"blob\",\"sha\":\"s{}\"}}", i, i);
        }
        bigTree += "]}";
        GitHubTreeSnapshot limitSnap;
        Success(ParseGitHubTreeJson(bigTree, L"RedSalamanders", L"RedTilde", L"main", limitSnap), "parse bounded oversized tree");
        Require(limitSnap.limitReached && limitSnap.markdownCount == MaximumGitHubTreeEntries,
                "ParseGitHubTreeJson stops at MaximumGitHubTreeEntries + 1 (5001) and sets limitReached");
    });

    test("Editor.GitHubWinHttpLoopbackAndHardening",
         [&]
    {
        LoopbackGitHubServer server([](std::string_view targetPath, std::string_view rawReq) -> LoopbackResponse
        {
            if (targetPath == "/redirect-target")
                return {.status = "200 OK", .body = "{\"login\":\"should-not-reach\"}"};
            if (targetPath == "/set-cookie")
                return {.status = "200 OK", .headers = "Set-Cookie: gh_sess=secret; HttpOnly\r\n", .body = "{\"ok\":true}"};
            if (targetPath == "/check-cookie")
            {
                const bool sawCookie = rawReq.find("Cookie:") != std::string_view::npos;
                return {.status = sawCookie ? "400 Bad Request" : "200 OK", .body = sawCookie ? "{\"error\":\"cookie\"}" : "{\"login\":\"clean\"}"};
            }
            if (targetPath == "/redirect")
                return {.status = "302 Found", .headers = "Location: /redirect-target\r\n", .body = ""};
            if (targetPath == "/negotiate")
                return {.status = "401 Unauthorized", .headers = "WWW-Authenticate: Negotiate\r\n", .body = "{\"message\":\"Bad credentials\"}"};
            if (targetPath == "/long-link")
            {
                std::string pad(700u, 'x');
                return {.status = "200 OK", .headers = std::format("Link: <https://api.github.com/user/repos?p={}>; rel=\"next\"\r\n", pad), .body = "[]"};
            }
            if (targetPath == "/very-long-link")
            {
                std::string pad(2500u, 'x');
                return {.status = "200 OK", .headers = std::format("Link: <https://api.github.com/user/repos?p={}>; rel=\"next\"\r\n", pad), .body = "[]"};
            }
            if (targetPath == "/overcap-link")
            {
                std::string pad(17000u, 'x');
                return {.status = "200 OK", .headers = std::format("Link: <https://api.github.com/user/repos?p={}>; rel=\"next\"\r\n", pad), .body = "[]"};
            }
            if (targetPath == "/rate-403")
                return {.status = "403 Forbidden", .headers = "X-RateLimit-Remaining: 0\r\n", .body = "{\"message\":\"API rate limit\"}"};
            if (targetPath == "/rate-403-long-remain")
                return {.status  = "403 Forbidden",
                        .headers = std::format("X-RateLimit-Remaining: {}\r\n", std::string(36u, '1')),
                        .body    = "{\"message\":\"Forbidden\"}"};
            if (targetPath == "/rate-429")
                return {.status = "429 Too Many Requests", .body = "{\"message\":\"Too many requests\"}"};
            if (targetPath == "/rate-429-long-remain")
                return {.status  = "429 Too Many Requests",
                        .headers = std::format("X-RateLimit-Remaining: {}\r\n", std::string(36u, '9')),
                        .body    = "{\"message\":\"Rate limit\"}"};
            if (targetPath == "/oversize")
                return {.status = "200 OK", .body = std::string(128u, 'A')};
            if (targetPath == "/stall")
                return {.status = "200 OK", .body = "{}", .waitForRelease = true};
            return {.status = "404 Not Found", .body = "{\"message\":\"Not Found\"}"};
        });

        WinHttpGitHubTransport transport;
        transport.SetLoopbackEndpointForTest(server.Endpoint());

        // 1. Pre-network rejection of CRLF / NUL / whitespace / CTLs / disallowed host
        GitHubHttpResponse resp;
        GitHubHttpRequest badTokenReq;
        badTokenReq.path        = L"/user";
        badTokenReq.bearerToken = L"abc\r\nX-Injected: evil";
        Require(FAILED(transport.SendRequest(badTokenReq, nullptr, nullptr, resp)) && server.Count() == 0u, "CRLF in bearerToken rejected pre-network");

        GitHubHttpRequest badSpaceToken;
        badSpaceToken.path        = L"/user";
        badSpaceToken.bearerToken = L"abc def";
        Require(FAILED(transport.SendRequest(badSpaceToken, nullptr, nullptr, resp)) && server.Count() == 0u, "whitespace in bearerToken rejected pre-network");

        GitHubHttpRequest badCt;
        badCt.path        = L"/user";
        badCt.contentType = "application/json\r\nX-Evil: 1";
        Require(FAILED(transport.SendRequest(badCt, nullptr, nullptr, resp)) && server.Count() == 0u, "CRLF in contentType rejected pre-network");

        GitHubHttpRequest badMethod;
        badMethod.method = L"DELETE";
        badMethod.path   = L"/user";
        Require(FAILED(transport.SendRequest(badMethod, nullptr, nullptr, resp)) && server.Count() == 0u, "unsupported HTTP method rejected pre-network");

        GitHubHttpRequest badHost;
        badHost.host = L"evil.example.com";
        badHost.path = L"/user";
        Require(FAILED(transport.SendRequest(badHost, nullptr, nullptr, resp)) && server.Count() == 0u, "disallowed host rejected pre-network");

        // 2. Session reuse & no cookie echo
        GitHubHttpRequest req;
        req.path = L"/set-cookie";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send Set-Cookie loopback request");
        Require(resp.httpStatusCode == 200u, "Set-Cookie loopback returned 200");
        req.path = L"/check-cookie";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send check-cookie request on reused session");
        Require(resp.httpStatusCode == 200u && server.Count() == 2u, "cookies disabled across reused WinHTTP session");

        // 3. No 302 redirect following & no 401 Negotiate autologon retry
        req.path = L"/redirect";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 302 redirect request");
        Require(resp.httpStatusCode == 302u, "WinHTTP never follows 302 redirects");

        const unsigned countBefore401 = server.Count();
        req.path                      = L"/negotiate";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 401 Negotiate request");
        Require(resp.httpStatusCode == 401u && resp.errorCode == GitHubErrorCode::Unauthorized && server.Count() == countBefore401 + 1u,
                "401 Negotiate returns 401 once without WinHTTP auth retry");

        // 4. Long >512 wchar_t, >2048 wchar_t, and >32 KiB UTF-16 overcap Link header handling
        req.path = L"/long-link";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send >512 char Link header request");
        Require(resp.hasNextPage, "long Link rel=next header parsed without truncation");

        req.path = L"/very-long-link";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 2500-char Link header request");
        Require(resp.httpStatusCode == 200u && resp.hasNextPage, "2500-char Link header succeeds via dynamic buffer without ERROR_INSUFFICIENT_BUFFER");

        req.path = L"/overcap-link";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send >32 KiB UTF-16 overcap Link header request");
        Require(resp.httpStatusCode == 200u && ! resp.hasNextPage, "over-cap optional Link header succeeds with hasNextPage == false");

        // 5. Rate-limit detection (403 X-RateLimit-Remaining: 0, long remain, & 429)
        req.path = L"/rate-403";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 403 rate limit request");
        Require(resp.rateLimited && resp.errorCode == GitHubErrorCode::RateLimited, "403 X-RateLimit-Remaining:0 sets rateLimited");

        req.path = L"/rate-403-long-remain";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 403 with 36-char X-RateLimit-Remaining");
        Require(resp.httpStatusCode == 403u && ! resp.rateLimited, "403 with 36-char X-RateLimit-Remaining succeeds without ERROR_INSUFFICIENT_BUFFER");

        req.path = L"/rate-429";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 429 rate limit request");
        Require(resp.rateLimited && resp.errorCode == GitHubErrorCode::RateLimited, "429 sets rateLimited");

        req.path = L"/rate-429-long-remain";
        Success(transport.SendRequest(req, nullptr, nullptr, resp), "send 429 with 36-char X-RateLimit-Remaining");
        Require(resp.httpStatusCode == 429u && resp.rateLimited && resp.errorCode == GitHubErrorCode::RateLimited,
                "429 with 36-char X-RateLimit-Remaining always sets RateLimited");

        // 6. maximumResponseBytes cap
        GitHubHttpRequest oversizeReq;
        oversizeReq.path                 = L"/oversize";
        oversizeReq.maximumResponseBytes = 32u;
        const HRESULT overHr             = transport.SendRequest(oversizeReq, nullptr, nullptr, resp);
        Require(overHr == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) && resp.errorCode == GitHubErrorCode::FileTooLarge,
                "maximumResponseBytes enforces response size ceiling");

        // 7. Immediate abortEvent cancellation on stalled loopback request
        wil::unique_event abortEv(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        Require(static_cast<bool>(abortEv), "create abort event");
        GitHubHttpRequest stallReq;
        stallReq.path = L"/stall";
        GitHubHttpResponse stallResp;
        std::jthread abortTrigger([&]
        {
            server.AwaitRequest();
            (void)SetEvent(abortEv.get());
        });
        const auto t0         = std::chrono::steady_clock::now();
        const HRESULT abortHr = transport.SendRequest(stallReq, nullptr, abortEv.get(), stallResp);
        const auto elapsedMs  = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        abortTrigger.join();
        Require(abortHr == HRESULT_FROM_WIN32(ERROR_CANCELLED) && stallResp.errorCode == GitHubErrorCode::Cancelled && elapsedMs < 2000,
                "abortEvent cancels stalled WinHTTP async transfer immediately");
    });

    test("Editor.GitHubCredentialStoreSafetyAndParity",
         [&]
    {
        GUID guid{};
        Success(CoCreateGuid(&guid), "create unique credential test GUID");
        const std::wstring targetName = std::format(L"RedTilde/Test/{:08x}-{:04x}-{:04x}", guid.Data1, guid.Data2, guid.Data3);

        const auto runParityChecks = [&](ICredentialStore& store)
        {
            // Embedded NUL rejection across Write/Read/Delete
            const std::wstring nulTarget(L"RedTilde/Test/a\0b", 17u);
            const std::wstring nulUser(L"ada\0x", 5u);
            const std::wstring nulSecret(L"pat\0secret", 10u);
            Require(store.Write(nulTarget, L"ada", L"pat") == E_INVALIDARG && store.Write(targetName, nulUser, L"pat") == E_INVALIDARG &&
                        store.Write(targetName, L"ada", nulSecret) == E_INVALIDARG && store.Delete(nulTarget) == E_INVALIDARG,
                    "CredentialStore rejects embedded NUL in target, username, and secret");

            StoredCredential sentinel{L"keep-user", L"keep-secret"};
            Require(store.Read(nulTarget, sentinel) == E_INVALIDARG && sentinel.username == L"keep-user" && sentinel.secret == L"keep-secret",
                    "CredentialStore::Read invalid target preserves outCredential");
            Require(store.Read(targetName, sentinel) == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) && sentinel.username == L"keep-user" &&
                        sentinel.secret == L"keep-secret",
                    "CredentialStore::Read missing target preserves outCredential");

            Success(store.Write(targetName, L"ada-writer", L"pat_roundtrip_value"), "write credential");
            StoredCredential loaded;
            Success(store.Read(targetName, loaded), "read credential");
            Require(loaded.username == L"ada-writer" && loaded.secret == L"pat_roundtrip_value", "CredentialStore round-trips username and secret");
            ScrubWideString(loaded.secret);
            ScrubWideString(sentinel.secret);
            Success(store.Delete(targetName), "delete test credential");
            StoredCredential afterDel;
            Require(store.Read(targetName, afterDel) == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "deleted credential no longer readable");

            // Challenge 4: MaximumCredentialSecretChars (1280 wchar_t / 2560 bytes) parity between MemoryCredentialStore and WindowsCredentialStore
            const std::wstring maxSecret(MaximumCredentialSecretChars, L'p');
            Success(store.Write(targetName, L"ada-max", maxSecret), "write exact 1280-wchar max credential");
            StoredCredential maxLoaded;
            Success(store.Read(targetName, maxLoaded), "read exact 1280-wchar max credential");
            Require(maxLoaded.secret == maxSecret, "1280-wchar credential round-trips");
            ScrubWideString(maxLoaded.secret);
            Success(store.Delete(targetName), "delete max credential");

            const std::wstring overMaxSecret(MaximumCredentialSecretChars + 1u, L'p');
            Require(store.Write(targetName, L"ada-over", overMaxSecret) == E_INVALIDARG,
                    "CredentialStore rejects 1281-wchar secret consistently with WinCred 2560-byte limit");
        };

        MemoryCredentialStore memStore;
        runParityChecks(memStore);

        WindowsCredentialStore winStore;
        const auto cleanupWinCred = wil::scope_exit([&]() noexcept { (void)winStore.Delete(targetName); });
        runParityChecks(winStore);

        // SSO caveat (P1-06): ScrubbedWideSecret stores <=7 wchar tokens on heap with clean move & zero residue
        ScrubbedWideSecret shortSecret;
        shortSecret.Assign(L"pat_1");
        Require(! shortSecret.Empty() && shortSecret.View() == L"pat_1", "ScrubbedWideSecret stores short 5-wchar token");
        ScrubbedWideSecret movedSecret(std::move(shortSecret));
        Require(shortSecret.Empty() && shortSecret.View().empty(), "ScrubbedWideSecret move leaves source empty with no SSO residue");
        Require(! movedSecret.Empty() && movedSecret.View() == L"pat_1", "ScrubbedWideSecret move transfers 5-wchar token intact");
        movedSecret.Clear();
        Require(movedSecret.Empty() && movedSecret.View().empty(), "ScrubbedWideSecret::Clear zeros and resets short token");
    });

    test("Editor.GitHubGhCliHardenedImport",
         [&]
    {
        std::array<wchar_t, MAX_PATH> sysDir{};
        const UINT sysLen = GetSystemDirectoryW(sysDir.data(), static_cast<UINT>(sysDir.size()));
        Require(sysLen > 0u && sysLen < sysDir.size(), "locate System32 directory");
        const std::filesystem::path cmdExe = std::filesystem::path(std::wstring_view(sysDir.data(), sysLen)) / L"cmd.exe";

        // 1. Noisy stderr ignored while stdout token extracted cleanly
        std::wstring token;
        Success(TryImportGhCliTokenWithExecutableForTest(cmdExe, L"/d /c \"echo warning on stderr>&2 & echo gho_hardened_token_42\"", token, 3000u),
                "import token with noisy stderr");
        Require(token == L"gho_hardened_token_42", "stderr does not corrupt imported stdout token");
        ScrubWideString(token);

        // 2. >8 KiB stdout drain without pipe deadlock (5010 leading whitespace bytes + 4000-char token > 8192 pipe buffer)
        const std::wstring longToken(4000u, L't');
        const std::wstring longCmd = L"/d /c \"(for /L %i in (1,1,5) do @echo." + std::wstring(1000u, L' ') + L") & echo " + longToken + L"\"";
        Success(TryImportGhCliTokenWithExecutableForTest(cmdExe, longCmd, token, 3000u), "drain >4 KiB stdout without pipe deadlock");
        Require(token == longToken, ">4 KiB pipe output trims cleanly to 4000-char token");
        ScrubWideString(token);

        // 3. Non-zero exit code -> ERROR_NOT_FOUND
        Require(TryImportGhCliTokenWithExecutableForTest(cmdExe, L"/d /c \"echo bogus_token & exit /b 1\"", token, 3000u) ==
                        HRESULT_FROM_WIN32(ERROR_NOT_FOUND) &&
                    token.empty(),
                "non-zero exit rejects stdout and clears token");

        // 4. Timeout triggers Job Object kill + ERROR_TIMEOUT
        const HRESULT timeoutHr = TryImportGhCliTokenWithExecutableForTest(cmdExe, L"/d /c \"ping -n 6 127.0.0.1 >nul\"", token, 250u);
        Require(timeoutHr == HRESULT_FROM_WIN32(ERROR_TIMEOUT) && token.empty(), "stalled CLI child killed by timeout Job Object");

        // 5. Relative path and junction executable rejection
        Require(FAILED(TryImportGhCliTokenWithExecutableForTest(L"cmd.exe", L"/c echo x", token, 1000u)), "relative executable path rejected");

        const auto jRoot  = root / L"gh-cli-junction-test";
        const auto target = jRoot / L"real-bin";
        const auto link   = jRoot / L"link-bin";
        std::filesystem::create_directories(target);
        std::filesystem::copy_file(cmdExe, target / L"cmd.exe", std::filesystem::copy_options::overwrite_existing);
        Success(Testing::CreateFixtureJunction(jRoot, link, target), "create junction for gh CLI test");
        const auto removeLink = wil::scope_exit([&] { (void)RemoveDirectoryW(link.c_str()); });
        Require(TryImportGhCliTokenWithExecutableForTest(link / L"cmd.exe", L"/d /c \"echo nope\"", token, 1000u) == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),
                "junction ancestor executable path rejected fail-closed");
    });

    test("Editor.GitHubDeviceFlowGatingAndRfc8628",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;

        // 1. Default production build has empty DefaultGitHubDeviceClientId -> Device Flow hidden and rejected
        Require(DefaultGitHubDeviceClientId.empty() && ! IsGitHubDeviceFlowConfigured(), "DefaultGitHubDeviceClientId is empty in production configuration");
        {
            auto transport = std::make_shared<MockGitHubTransport>();
            auto creds     = std::make_shared<MemoryCredentialStore>();
            LaunchOptions launch;
            launch.headless               = true;
            launch.dataDirectory          = root / L"github-device-gated" / L"appdata";
            launch.githubTransportForTest = transport;
            launch.credentialStoreForTest = creds;

            Application app(launch);
            Success(app.Initialize(), "initialize gated Device Flow app");
            Require(! GitHubUiProbe::Service(app).DeviceFlowConfigured(), "DeviceFlowConfigured is false when client_id is empty");
            app.Execute(Command::OpenGitHub);
            Require(GitHubUiProbe::ModalDeviceButton(app) && ! GitHubUiProbe::ModalDeviceButton(app)->IsVisible() && GitHubUiProbe::DrawerDeviceButton(app) &&
                        ! GitHubUiProbe::DrawerDeviceButton(app)->IsVisible() && GitHubUiProbe::ModalImportGhButton(app) &&
                        GitHubUiProbe::ModalImportGhButton(app)->IsVisible() && GitHubUiProbe::DrawerImportGhButton(app) &&
                        GitHubUiProbe::DrawerImportGhButton(app)->IsVisible(),
                    "Device Flow buttons stay hidden when client_id is unconfigured while Import gh CLI stays visible in both modal and drawer");
            app.Execute(Command::GitHubSignInDevice);
            Pump(app, 8);
            Require(transport->requestCount.load() == 0u &&
                        GitHubUiProbe::ModalErrorLabel(app)->GetText() == LoadStringResource(nullptr, IDS_GITHUB_ERR_DEVICE_UNAVAILABLE),
                    "GitHubSignInDevice makes zero requests and displays localized unavailable error when unconfigured");

            // Challenge 2: ParseGitHubDevicePollJson rejects bidi, whitespace, control, and oversized access_token without stripping
            for (const std::string_view badPoll :
                 {"{\"access_token\":\"gho_abc\\u202Edef\"}", "{\"access_token\":\"gho_abc def\"}", "{\"access_token\":\"gho_abc\\ndef\"}"})
            {
                GitHubDevicePollResult pollRes;
                pollRes.accessToken = L"stale";
                Require(FAILED(ParseGitHubDevicePollJson(badPoll, pollRes)) && pollRes.accessToken.empty(),
                        "ParseGitHubDevicePollJson rejects malformed access_token and scrubs outResult");
            }
            GitHubDevicePollResult overPoll;
            overPoll.accessToken = L"stale";
            Require(FAILED(ParseGitHubDevicePollJson(std::format("{{\"access_token\":\"{}\"}}", std::string(4097u, 'x')), overPoll)) &&
                        overPoll.accessToken.empty(),
                    "ParseGitHubDevicePollJson rejects >4096-char access_token");
        }

        // 2. With githubDeviceClientIdForTest: RFC 8628 slow_down +5s preserved, flowId filtering, overlay cleanup, Authorized error check, expiry, layout
        // reflow
        {
            auto transport = std::make_shared<MockGitHubTransport>();
            std::atomic<int> pollStage{0};
            std::atomic<bool> failUserAfterAuth{false};
            std::atomic<bool> holdFirstStart{false};
            wil::unique_event firstStartEntered(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            wil::unique_event firstStartRelease(CreateEventW(nullptr, TRUE, TRUE, nullptr));
            std::atomic<bool> holdFirstPoll{false};
            wil::unique_event firstPollEntered(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            wil::unique_event firstPollRelease(CreateEventW(nullptr, TRUE, TRUE, nullptr));
            transport->customHandler = [&](const GitHubHttpRequest& req, const std::atomic_bool*, HANDLE, GitHubHttpResponse& out) -> HRESULT
            {
                if (req.path == L"/login/device/code")
                {
                    if (holdFirstStart.exchange(false))
                    {
                        (void)SetEvent(firstStartEntered.get());
                        (void)WaitForSingleObject(firstStartRelease.get(), 5000);
                        out.httpStatusCode = 200u;
                        out.body           = "{\"device_code\":\"stale_dc\",\"user_code\":\"STAL-0001\",\"verification_uri\":\"https://github.com/login/"
                                             "device\",\"expires_in\":300,\"interval\":5}";
                        return S_OK;
                    }
                    out.httpStatusCode = 200u;
                    out.body           = "{\"device_code\":\"dev_code_1\",\"user_code\":\"ABCD-EFGH\",\"verification_uri\":\"https://github.com/login/"
                                         "device\",\"expires_in\":300,\"interval\":5}";
                    return S_OK;
                }
                if (req.path == L"/login/oauth/access_token")
                {
                    if (holdFirstPoll.exchange(false))
                    {
                        (void)SetEvent(firstPollEntered.get());
                        (void)WaitForSingleObject(firstPollRelease.get(), 5000);
                        out.httpStatusCode = 200u;
                        out.body           = "{\"access_token\":\"stale_oauth_token\"}";
                        return S_OK;
                    }
                    const int stage = pollStage.fetch_add(1);
                    if (stage == 0)
                    {
                        out.httpStatusCode = 200u;
                        out.body           = "{\"error\":\"slow_down\"}";
                        return S_OK;
                    }
                    if (stage == 1)
                    {
                        out.httpStatusCode = 200u;
                        out.body           = "{\"error\":\"authorization_pending\"}";
                        return S_OK;
                    }
                    if (stage == 2)
                    {
                        out.httpStatusCode = 200u;
                        out.body           = "{\"access_token\":\"oauth_pat_ok\"}";
                        return S_OK;
                    }
                    out.transportStatus = HRESULT_FROM_WIN32(ERROR_GEN_FAILURE);
                    return out.transportStatus;
                }
                if (req.path == L"/user")
                {
                    if (failUserAfterAuth.load())
                    {
                        out.httpStatusCode = 500u;
                        out.body           = "{\"message\":\"Server error\"}";
                        out.errorCode      = GitHubErrorCode::ServerMessage;
                        out.errorMessage   = L"Server error";
                        return S_OK;
                    }
                    out.httpStatusCode = 200u;
                    out.body           = "{\"login\":\"device-user\"}";
                    return S_OK;
                }
                if (req.path.find(L"/user/repos") == 0u)
                {
                    out.httpStatusCode = 200u;
                    out.body           = "[]";
                    return S_OK;
                }
                out.httpStatusCode = 404u;
                return S_OK;
            };

            auto creds = std::make_shared<MemoryCredentialStore>();
            LaunchOptions launch;
            launch.headless                    = true;
            launch.dataDirectory               = root / L"github-device-rfc8628" / L"appdata";
            launch.githubTransportForTest      = transport;
            launch.credentialStoreForTest      = creds;
            launch.githubDeviceClientIdForTest = "Iv1.testclientid";

            Application app(launch);
            Success(app.Initialize(), "initialize RFC 8628 Device Flow app");
            Require(GitHubUiProbe::Service(app).DeviceFlowConfigured(), "DeviceFlowConfigured is true with test client_id");
            app.Execute(Command::OpenGitHub);
            Require(GitHubUiProbe::ModalDeviceButton(app)->IsVisible(), "Device Flow button visible when configured");
            {
                RECT rect{};
                Require(GetWindowRect(app.Window(), &rect) != FALSE, "read configured Device Flow modal window rect");
                const int dpi = static_cast<int>(app.Host().GetDpi());
                rect.right    = rect.left + MulDiv(800, dpi, 96);
                rect.bottom   = rect.top + MulDiv(560, dpi, 96);
                Require(SetWindowPos(app.Window(),
                                     nullptr,
                                     rect.left,
                                     rect.top,
                                     rect.right - rect.left,
                                     rect.bottom - rect.top,
                                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                        "resize configured Device Flow window to 800x560");
                app.Prepare(16);
                Require(GitHubUiProbe::ModalDeviceButton(app)->GetBounds().right <= GitHubUiProbe::ModalImportGhButton(app)->GetBounds().left &&
                            GitHubUiProbe::ModalImportGhButton(app)->GetBounds().right <= GitHubUiProbe::ModalTokenInput(app)->GetBounds().left,
                        "Device Flow button, gh CLI import button, and PAT input do not overlap at 800x560");
            }

            // Stale flowId rejection for StartDeviceFlow: flow1 arrives after flow2 superseded it
            (void)ResetEvent(firstStartEntered.get());
            (void)ResetEvent(firstStartRelease.get());
            holdFirstStart.store(true);
            app.Execute(Command::GitHubSignInDevice);
            Require(WaitForSingleObject(firstStartEntered.get(), 3000) == WAIT_OBJECT_0, "first StartDeviceFlow entered transport");
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "second StartDeviceFlow sets pending device flow");
            (void)SetEvent(firstStartRelease.get());
            Await(app, [&] { return ! GitHubUiProbe::Service(app).Busy(); }, "stale StartDeviceFlow drains");
            Pump(app, 8);
            Require(GitHubUiProbe::PendingDeviceFlow(app) && GitHubUiProbe::PendingDeviceFlow(app)->info.userCode == L"ABCD-EFGH" &&
                        GitHubUiProbe::PendingDeviceFlow(app)->currentIntervalSeconds == 5u,
                    "stale StartDeviceFlow flowId is ignored and active flow keeps ABCD-EFGH and 5s interval");

            // Simulate poll timer -> slow_down -> interval becomes 10s
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Await(app, [&] {
                return pollStage.load() >= 1 && GitHubUiProbe::PendingDeviceFlow(app)->currentIntervalSeconds == 10u;
            }, "slow_down adds +5s backoff (5s -> 10s)");

            // Subsequent authorization_pending without interval keeps 10s
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Await(app, [&] { return pollStage.load() >= 2; }, "authorization_pending processed");
            Pump(app, 8);
            Require(GitHubUiProbe::PendingDeviceFlow(app) && GitHubUiProbe::PendingDeviceFlow(app)->currentIntervalSeconds == 10u,
                    "authorization_pending preserves slow_down 10s interval");

            // Stale flowId rejection for PollDeviceFlow: poll for flow2 finishes after flow3 supersedes it
            (void)ResetEvent(firstPollEntered.get());
            (void)ResetEvent(firstPollRelease.get());
            holdFirstPoll.store(true);
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Require(WaitForSingleObject(firstPollEntered.get(), 3000) == WAIT_OBJECT_0, "stale PollDeviceFlow entered transport");
            const auto staleFlowId = GitHubUiProbe::PendingDeviceFlow(app)->flowId;
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] {
                return GitHubUiProbe::PendingDeviceFlow(app) && GitHubUiProbe::PendingDeviceFlow(app)->flowId != staleFlowId;
            }, "superseding StartDeviceFlow advances active flowId");
            (void)SetEvent(firstPollRelease.get());
            Await(app, [&] { return ! GitHubUiProbe::Service(app).Busy(); }, "stale PollDeviceFlow drains");
            Pump(app, 8);
            Require(GitHubUiProbe::SignedInLogin(app).empty() && GitHubUiProbe::PendingDeviceFlow(app) &&
                        GitHubUiProbe::PendingDeviceFlow(app)->flowId != staleFlowId,
                    "stale PollDeviceFlow Authorized result is rejected when flowId no longer matches");

            // Authorized when /user fails (500) must NOT mark signed in!
            failUserAfterAuth.store(true);
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Await(app, [&] { return ! GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "Authorized clears pending device flow");
            Require(GitHubUiProbe::SignedInLogin(app).empty() && GitHubUiProbe::ModalErrorLabel(app)->GetText().find(L"Server error") != std::wstring::npos,
                    "Authorized with failed /user does not sign in and surfaces error");

            // Signed-in empty-repo suggest empty text (IDS_GITHUB_NO_REPOS) vs non-matching query (IDS_NOMATCH)
            failUserAfterAuth.store(false);
            pollStage.store(2);
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "re-start Device Flow for signed-in empty repo check");
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Await(app, [&] { return GitHubUiProbe::SignedInLogin(app) == L"device-user"; }, "Authorized signs in device-user with 0 repos");
            Require(GitHubUiProbe::SuggestTree(app)->GetEmptyStateText() == LoadStringResource(nullptr, IDS_GITHUB_NO_REPOS),
                    "signed-in empty-search with 0 repos displays IDS_GITHUB_NO_REPOS");
            GitHubUiProbe::UrlInput(app)->SetTextAndNotify(L"MissingRepoQuery");
            Require(GitHubUiProbe::SuggestTree(app)->GetEmptyStateText() == LoadStringResource(nullptr, IDS_NOMATCH),
                    "signed-in non-empty filter with 0 matches displays IDS_NOMATCH");
            GitHubUiProbe::UrlInput(app)->SetTextAndNotify(L"");
            app.Execute(Command::GitHubSignOut);
            Await(app, [&] { return GitHubUiProbe::SignedInLogin(app).empty(); }, "sign out after empty-repo check");
            pollStage.store(3);

            // SettingsDrawer _accountsStatusLabel reflows on async StartDeviceFlow update without clipping
            app.Execute(Command::Settings);
            app.Prepare(16);
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "StartDeviceFlow in SettingsDrawer");
            Pump(app, 8);
            Require(GitHubUiProbe::DrawerAccountsStatusLabel(app)->GetText().find(L"ABCD-EFGH") != std::wstring::npos &&
                        GitHubUiProbe::DrawerAccountsStatusLabel(app)->GetBounds().bottom <= GitHubUiProbe::DrawerDeviceButton(app)->GetBounds().top,
                    "SettingsDrawer _accountsStatusLabel reflows above Device Flow button when Device Code prompt arrives");
            app.Execute(Command::CloseDrawer);
            Require(! GitHubUiProbe::PendingDeviceFlow(app).has_value(), "closing SettingsDrawer clears _pendingDeviceFlow");

            // Start second flow in modal, leave overlay -> SetActiveOverlay clears flow & kills timer
            app.Execute(Command::OpenGitHub);
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "second StartDeviceFlow active");
            app.Execute(Command::CloseDrawer);
            Require(! GitHubUiProbe::PendingDeviceFlow(app).has_value(), "closing overlay clears _pendingDeviceFlow");

            // Start third flow and force monotonic expiry -> WM_TIMER expires cleanly without polling transport
            app.Execute(Command::OpenGitHub);
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "third StartDeviceFlow active");
            const auto countBeforeExpiry = transport->requestCount.load();
            GitHubUiProbe::ExpireDeviceFlowNow(app);
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Require(! GitHubUiProbe::PendingDeviceFlow(app).has_value() && transport->requestCount.load() == countBeforeExpiry &&
                        GitHubUiProbe::ModalErrorLabel(app)->GetText() == LoadStringResource(nullptr, IDS_GITHUB_ERR_DEVICE_EXPIRED),
                    "expired device flow clears cleanly on WM_TIMER without issuing poll request");

            // Start fourth flow -> transport error clears flow & surfaces error
            app.Execute(Command::GitHubSignInDevice);
            Await(app, [&] { return GitHubUiProbe::PendingDeviceFlow(app).has_value(); }, "fourth StartDeviceFlow active");
            (void)SendMessageW(app.Window(), WM_TIMER, GitHubUiProbe::DevicePollTimerId(), 0);
            Await(app, [&] {
                return ! GitHubUiProbe::PendingDeviceFlow(app).has_value();
            }, "transport error during PollDeviceFlow clears flow and surfaces error");
            Require(! GitHubUiProbe::ModalErrorLabel(app)->GetText().empty(), "PollDeviceFlow transport failure surfaces error text");
        }
        Require(foreground.NeverAcquiredForeground(), "Device Flow tests never acquire desktop focus");
    });

    test("Editor.GitHubHttpErrorsPaginationTreeAndStop",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto dataDir = root / L"github-errors-and-stop" / L"appdata";
        std::filesystem::create_directories(dataDir);

        // 1. 403 / 503 preserve stored credential while 401 deletes it; multi-page /user/repos; FetchRepoTree failure clears loading; /blob/ open
        {
            auto creds = std::make_shared<MemoryCredentialStore>();
            Success(creds->Write(GitHubCredentialTarget, L"ada-writer", L"persisted-pat-999"), "seed persisted credential");

            std::atomic<unsigned> userStatus{503u};
            auto transport           = std::make_shared<MockGitHubTransport>();
            transport->customHandler = [&](const GitHubHttpRequest& req, const std::atomic_bool*, HANDLE, GitHubHttpResponse& out) -> HRESULT
            {
                if (req.path == L"/user")
                {
                    const unsigned st  = userStatus.load();
                    out.httpStatusCode = st;
                    if (st == 200u && req.bearerToken == L"persisted-pat-999")
                    {
                        out.body = "{\"login\":\"ada-writer\"}";
                    }
                    else if (st == 401u)
                    {
                        out.errorCode = GitHubErrorCode::Unauthorized;
                    }
                    else
                    {
                        out.errorCode    = GitHubErrorCode::ServerMessage;
                        out.errorMessage = L"Service Unavailable";
                    }
                    return S_OK;
                }
                if (req.path == L"/user/repos?sort=pushed&per_page=100&page=1")
                {
                    out.httpStatusCode = 200u;
                    out.hasNextPage    = true;
                    out.body           = "[{\"name\":\"PageOne\",\"full_name\":\"ada-writer/PageOne\",\"default_branch\":\"main\","
                                         "\"html_url\":\"https://github.com/ada-writer/PageOne\",\"private\":false,\"owner\":{\"login\":\"ada-writer\"}}]";
                    return S_OK;
                }
                if (req.path == L"/user/repos?sort=pushed&per_page=100&page=2")
                {
                    out.httpStatusCode = 200u;
                    out.hasNextPage    = false;
                    out.body           = "[{\"name\":\"PageTwo\",\"full_name\":\"ada-writer/PageTwo\",\"default_branch\":\"main\","
                                         "\"html_url\":\"https://github.com/ada-writer/PageTwo\",\"private\":true,\"owner\":{\"login\":\"ada-writer\"}}]";
                    return S_OK;
                }
                if (req.path == L"/repos/ada-writer/BrokenRepo" || req.path.find(L"/repos/ada-writer/BrokenRepo/git/trees/") == 0u)
                {
                    out.httpStatusCode = 404u;
                    out.errorCode      = GitHubErrorCode::NotFound;
                    return S_OK;
                }
                if (req.path == L"/repos/ada-writer/PrivateRepo/git/trees/feature/v1?recursive=1")
                {
                    if (req.bearerToken != L"persisted-pat-999")
                    {
                        out.httpStatusCode = 401u;
                        out.errorCode      = GitHubErrorCode::Unauthorized;
                        return S_OK;
                    }
                    out.httpStatusCode = 200u;
                    out.body           = "{\"truncated\":true,\"tree\":[{\"path\":\"docs/Notes.md\",\"type\":\"blob\",\"sha\":\"abc\"}]}";
                    return S_OK;
                }
                if (req.path == L"/repos/ada-writer/PrivateRepo/contents/docs/Notes.md?ref=feature%2Fv1")
                {
                    if (req.bearerToken != L"persisted-pat-999")
                    {
                        out.httpStatusCode = 401u;
                        out.errorCode      = GitHubErrorCode::Unauthorized;
                        return S_OK;
                    }
                    out.httpStatusCode = 200u;
                    out.body           = "# Private GitHub Note\r\nLazy cred worked.\r\n";
                    return S_OK;
                }
                out.httpStatusCode = 404u;
                out.errorCode      = GitHubErrorCode::NotFound;
                return S_OK;
            };

            // Pre-persist one GitHub repo in settings to verify it starts collapsed on Initialize
            Settings initialSettings;
            initialSettings.githubRepos.push_back({L"ada-writer", L"PrivateRepo", L"feature/v1"});
            Success(SaveSettings(dataDir, initialSettings), "seed persisted githubRepos");

            LaunchOptions launch;
            launch.headless               = true;
            launch.dataDirectory          = dataDir;
            launch.watchSettingsForTest   = true;
            launch.githubTransportForTest = transport;
            launch.credentialStoreForTest = creds;

            Application app(launch);
            Success(app.Initialize(), "initialize GitHub error/pagination test app");
            Require(GitHubUiProbe::CollapsedGitHubRepos(app).contains(L"ada-writer/PrivateRepo"),
                    "persisted githubRepos start collapsed in _collapsedGitHubRepos");

            // Lazy credential read on FetchRepoTree after restart (without LoadSavedAccount running first!)
            GitHubUiProbe::FilesModel(app).OnTreeToggleExpanded(GitHubUiProbe::RepoRowBaseId(), true);
            Await(app, [&] {
                return ! GitHubUiProbe::ConnectedRepos(app).empty() && GitHubUiProbe::ConnectedRepos(app)[0].loaded;
            }, "expanding persisted repo lazily reads credential store and loads tree");
            Require(GitHubUiProbe::ConnectedRepos(app)[0].truncated, "truncated flag propagated to GitHubConnectedRepoState");
            const std::wstring truncNotice = LoadStringResource(nullptr, IDS_GITHUB_TREE_TRUNCATED);
            Require(GitHubUiProbe::FilesCountLabel(app)->GetText().find(truncNotice) != std::wstring::npos,
                    "truncated GitHub tree propagates notice to _filesCount footer");

            // Collapsing GitHub section MUST preserve truncation notice in _filesCount footer
            GitHubUiProbe::FilesModel(app).OnTreeToggleExpanded(GitHubUiProbe::GitHubSectionGroupId(), false);
            Require(GitHubUiProbe::FilesCountLabel(app)->GetText().find(truncNotice) != std::wstring::npos,
                    "collapsed GitHub section preserves IDS_GITHUB_TREE_TRUNCATED in _filesCount footer");
            GitHubUiProbe::FilesModel(app).OnTreeToggleExpanded(GitHubUiProbe::GitHubSectionGroupId(), true);

            // Combined multi-folder + GitHub empty filter omits all headers so _filesModel.rows is empty (IDS_NOMATCH renders)
            const auto localFolder = root / L"github-combined-local";
            std::filesystem::create_directories(localFolder);
            Success(WriteBytesAtomic(localFolder / L"LocalNote.md", "# Local Note\r\n"), "seed combined local root file");
            GitHubUiProbe::AddFilesRootNow(app, localFolder);
            Await(app, [&] {
                return std::ranges::any_of(GitHubUiProbe::FilesModel(app).rows, [](const auto& row) { return row.text == L"LocalNote.md"; });
            }, "local folder root loads alongside GitHub repo");
            GitHubUiProbe::FilesFilter(app)->SetTextAndNotify(L"NoSuchFileMatchesThisQuery999");
            Require(GitHubUiProbe::FilesModel(app).rows.empty(),
                    "non-matching filter with both local folders and GitHub repos leaves _filesModel.rows empty so IDS_NOMATCH renders");
            GitHubUiProbe::FilesFilter(app)->SetTextAndNotify(L"");

            // Open modal when /user returns 503 -> stored credential MUST be preserved!
            app.Execute(Command::OpenGitHub);
            Await(app, [&] {
                return GitHubUiProbe::ModalErrorLabel(app) && ! GitHubUiProbe::ModalErrorLabel(app)->GetText().empty();
            }, "503 surfaces error on modal");
            StoredCredential checkCred;
            Success(creds->Read(GitHubCredentialTarget, checkCred), "503 preserves stored credential");
            ScrubWideString(checkCred.secret);

            // PAT input isolation & clear on overlay leave (no cross-surface fallback)
            GitHubUiProbe::ModalTokenInput(app)->SetText(L"modal-secret-pat");
            app.Execute(Command::CloseDrawer);
            Require(GitHubUiProbe::ModalTokenInput(app)->GetText().empty(), "leaving OpenGitHubModal clears PAT input");

            // Re-open with 200 OK -> multi-page pagination loads both PageOne and PageTwo
            userStatus.store(200u);
            app.Execute(Command::OpenGitHub);
            Await(app, [&] {
                return GitHubUiProbe::SignedInLogin(app) == L"ada-writer" && GitHubUiProbe::SuggestModel(app).rows.size() == 2u;
            }, "multi-page /user/repos loads PageOne and PageTwo");

            // Enter /blob/ URL with branch override -> CommitOpenGitHubModal closes modal BEFORE OpenGitHubDocument!
            GitHubUiProbe::UrlInput(app)->SetText(L"https://github.com/ada-writer/PrivateRepo/blob/main/docs/Notes.md");
            GitHubUiProbe::BranchInput(app)->SetText(L"feature/v1");
            app.Execute(Command::GitHubOpenConfirm);
            Require(GitHubUiProbe::ActiveOverlay(app) == Application::OverlayKind::None, "CommitOpenGitHubModal closes overlay before FetchRawFile");
            Await(app, [&] {
                return GitHubUiProbe::Session(app).origin.kind == DocumentOriginInfo::Kind::GitHub &&
                       GitHubUiProbe::Session(app).origin.relativePath == L"docs/Notes.md";
            }, "/blob/ URL opens GitHub document and sets origin");
            Require(GitHubUiProbe::TitleLabel(app)->GetText() == L"Notes.md", "ActiveDocumentDisplayName preserves leaf Notes.md");

            // FetchRepoTree on broken repo -> clears loading = false and shows error on _feedback
            app.Execute(Command::OpenGitHub);
            GitHubUiProbe::UrlInput(app)->SetText(L"ada-writer/BrokenRepo");
            GitHubUiProbe::BranchInput(app)->SetText(L"");
            app.Execute(Command::GitHubOpenConfirm);
            Await(app, [&] {
                return GitHubUiProbe::ConnectedRepos(app).size() == 2u && ! GitHubUiProbe::ConnectedRepos(app)[1].loading;
            }, "FetchRepoTree error clears connected.loading");
            Require(! GitHubUiProbe::ConnectedRepos(app)[1].loaded && ! GitHubUiProbe::FeedbackLabel(app)->GetText().empty(),
                    "FetchRepoTree failure keeps loaded=false and surfaces localized error");

            // Reconnecting an already-connected repo clears stale loaded/entries/truncated state before refetch
            app.Execute(Command::OpenGitHub);
            GitHubUiProbe::UrlInput(app)->SetText(L"ada-writer/PrivateRepo");
            GitHubUiProbe::BranchInput(app)->SetText(L"missing-branch");
            app.Execute(Command::GitHubOpenConfirm);
            Await(app, [&] { return ! GitHubUiProbe::ConnectedRepos(app)[0].loading; }, "reconnect FetchRepoTree completes");
            Require(! GitHubUiProbe::ConnectedRepos(app)[0].loaded && GitHubUiProbe::ConnectedRepos(app)[0].entries.empty() &&
                        ! GitHubUiProbe::ConnectedRepos(app)[0].truncated,
                    "reconnecting already-connected repo clears stale loaded, entries, and truncated state on error");

            // 401 on LoadSavedAccount deletes stored credential and clears signed-in state
            app.Execute(Command::GitHubSignOut);
            Await(app, [&] { return GitHubUiProbe::SignedInLogin(app).empty(); }, "sign out completes");
            Success(creds->Write(GitHubCredentialTarget, L"ada-writer", L"revoked-pat"), "seed revoked credential");
            userStatus.store(401u);
            app.Execute(Command::OpenGitHub);
            Await(app,
                  [&]
            {
                StoredCredential temp;
                return creds->Read(GitHubCredentialTarget, temp) == HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            },
                  "HTTP 401 deletes revoked credential from store");
            Require(GitHubUiProbe::SignedInLogin(app).empty(), "HTTP 401 clears signed-in login");
        }

        // 2. GitHubService::Stop() when idle AND when blocked on abortEvent
        {
            GitHubService idleSvc;
            auto transport = std::make_shared<MockGitHubTransport>();
            auto creds     = std::make_shared<MemoryCredentialStore>();
            Success(idleSvc.Initialize(nullptr, 0u, transport, creds), "init idle GitHubService");
            Require(idleSvc.Finished(), "idle GitHubService starts Finished() == true with zero permanent worker");
            idleSvc.Stop();
            Require(idleSvc.Finished(), "Stop() on idle GitHubService stays Finished() == true");
            idleSvc.Close();
        }
        {
            LoopbackGitHubServer stallServer([](std::string_view, std::string_view) -> LoopbackResponse
            { return {.status = "200 OK", .body = "{}", .waitForRelease = true}; });
            auto transport = std::make_shared<WinHttpGitHubTransport>();
            transport->SetLoopbackEndpointForTest(stallServer.Endpoint());
            auto creds = std::make_shared<MemoryCredentialStore>();

            GitHubService svc;
            Success(svc.Initialize(nullptr, 0u, transport, creds), "init active GitHubService");
            GitHubRequest req;
            req.kind      = GitHubRequest::Kind::FetchRepoTree;
            req.requestId = 1u;
            req.owner     = L"RedSalamanders";
            req.repo      = L"RedTilde";
            req.branch    = L"main";
            Success(svc.Submit(std::move(req)), "submit stalled request to GitHubService");
            stallServer.AwaitRequest();
            const auto t0 = std::chrono::steady_clock::now();
            svc.Stop();
            while (! svc.Finished())
            {
                if (std::chrono::steady_clock::now() - t0 > std::chrono::seconds(3))
                    Require(false, "GitHubService::Stop() stalled waiting on active worker");
                Sleep(10);
            }
            svc.Close();
        }
        // 3. FetchRawFile completion guards: HasActiveModalUi(), _files.Busy(), and ClosingAfterSave
        {
            const auto guardDir  = root / L"github-fetch-guards";
            const auto guardApp  = guardDir / L"appdata";
            const auto localPath = guardDir / L"doc1.md";
            std::filesystem::create_directories(guardApp);
            Success(WriteBytesAtomic(localPath, "# Local\r\n"), "seed doc1.md for FetchRawFile guard tests");

            wil::unique_event rawEntered(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            wil::unique_event rawProceed(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            auto guardTransport           = std::make_shared<MockGitHubTransport>();
            guardTransport->customHandler = [&](const GitHubHttpRequest& req, const std::atomic_bool*, HANDLE, GitHubHttpResponse& out) -> HRESULT
            {
                if (req.path.find(L"/repos/o/r/contents/Guard.md") == 0u)
                {
                    (void)SetEvent(rawEntered.get());
                    (void)WaitForSingleObject(rawProceed.get(), 5000);
                    out.httpStatusCode = 200u;
                    out.body           = "# Guarded GitHub File\r\n";
                    return S_OK;
                }
                out.httpStatusCode = 404u;
                return S_OK;
            };

            auto fileGate = std::make_shared<AsyncFiles::TestGate>();
            (void)SetEvent(fileGate->proceed.get());

            LaunchOptions launch;
            launch.headless               = true;
            launch.dataDirectory          = guardApp;
            launch.githubTransportForTest = guardTransport;
            launch.credentialStoreForTest = std::make_shared<MemoryCredentialStore>();
            launch.fileGateForTest        = fileGate;
            launch.unsavedPromptForTest   = [] { return IDNO; };

            Application app(launch);
            Success(app.Initialize(), "initialize FetchRawFile guard fixture");

            // 3a. _files.Busy() blocks FetchRawFile completion
            (void)ResetEvent(fileGate->entered.get());
            (void)ResetEvent(fileGate->proceed.get());
            GitHubUiProbe::OpenPathNow(app, localPath);
            Require(WaitForSingleObject(fileGate->entered.get(), 3000) == WAIT_OBJECT_0 && GitHubUiProbe::FilesBusy(app),
                    "AsyncFiles load held in TestGate so _files.Busy() == true");
            GitHubUiProbe::OpenGitHubDocumentNow(app, L"o", L"r", L"main", L"Guard.md");
            Require(WaitForSingleObject(rawEntered.get(), 3000) == WAIT_OBJECT_0, "FetchRawFile entered while _files.Busy()");
            (void)SetEvent(rawProceed.get());
            Await(app, [&] { return ! GitHubUiProbe::Service(app).Busy(); }, "FetchRawFile worker finished while _files.Busy()");
            Pump(app, 8);
            Require(GitHubUiProbe::Session(app).origin.kind != DocumentOriginInfo::Kind::GitHub, "FetchRawFile completion blocked when _files.Busy()");
            (void)SetEvent(fileGate->proceed.get());
            Await(app, [&] { return ! GitHubUiProbe::FilesBusy(app); }, "AsyncFiles local load completes");

            // 3b. HasActiveModalUi() blocks FetchRawFile completion
            (void)ResetEvent(rawEntered.get());
            (void)ResetEvent(rawProceed.get());
            GitHubUiProbe::OpenGitHubDocumentNow(app, L"o", L"r", L"main", L"Guard.md");
            Require(WaitForSingleObject(rawEntered.get(), 3000) == WAIT_OBJECT_0, "FetchRawFile entered before modal open");
            app.Execute(Command::OpenGitHub);
            Require(app.HasActiveModalUi(), "OpenGitHubModal sets HasActiveModalUi()");
            (void)SetEvent(rawProceed.get());
            Await(app, [&] { return ! GitHubUiProbe::Service(app).Busy(); }, "FetchRawFile worker finished while modal open");
            Pump(app, 8);
            Require(GitHubUiProbe::Session(app).origin.kind != DocumentOriginInfo::Kind::GitHub,
                    "FetchRawFile completion blocked when HasActiveModalUi() is true");
            app.Execute(Command::CloseDrawer);

            // 3c. ClosingAfterSave blocks FetchRawFile completion
            Require(app.Document().ReplaceSelection(L"Dirty edit\r\n"), "dirty local doc before ClosingAfterSave guard test");
            (void)ResetEvent(fileGate->entered.get());
            (void)ResetEvent(fileGate->proceed.get());
            (void)ResetEvent(rawEntered.get());
            (void)ResetEvent(rawProceed.get());
            app.Execute(Command::Save);
            Require(WaitForSingleObject(fileGate->entered.get(), 3000) == WAIT_OBJECT_0, "AsyncFiles save held in TestGate");
            GitHubUiProbe::OpenGitHubDocumentNow(app, L"o", L"r", L"main", L"Guard.md");
            Require(WaitForSingleObject(rawEntered.get(), 3000) == WAIT_OBJECT_0, "FetchRawFile entered before WM_CLOSE");
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.IsClosingAfterSave(), "WM_CLOSE entered ClosingAfterSave");
            (void)SetEvent(rawProceed.get());
            Await(app, [&] { return ! GitHubUiProbe::Service(app).Busy(); }, "FetchRawFile worker finished during ClosingAfterSave");
            Pump(app, 8);
            Require(GitHubUiProbe::Session(app).origin.kind != DocumentOriginInfo::Kind::GitHub, "FetchRawFile completion blocked during ClosingAfterSave");
            (void)SetEvent(fileGate->proceed.get());
            Await(app, [&] { return app.IsClosing() && app.AreShutdownWorkersSettled(); }, "ClosingAfterSave settles cleanly after TestGate release");
        }
        Require(foreground.NeverAcquiredForeground(), "GitHub error/stop tests never acquire desktop focus");
    });
}
} // namespace RedTilde
