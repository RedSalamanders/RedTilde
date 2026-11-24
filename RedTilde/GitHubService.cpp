#include "GitHubService.h"
#include "Common/ProcessWork.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <atomic>
#include <deque>
#include <format>
#include <mutex>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
// Public OAuth App client ID registered for RedTilde desktop Device Flow (no client secret required by RFC 8628).
constexpr std::string_view DefaultDeviceClientId = "Ov23liRedTildeDesktopPublic";

std::string EncodeUrlPathSegments(std::wstring_view utf16Path)
{
    std::string utf8;
    if (DxUi::RichText::TryEncodeUtf8(utf16Path, utf8) != DxUi::RichText::SourceResult::Success)
        return {};
    std::string encoded;
    encoded.reserve(utf8.size());
    for (const char ch : utf8)
    {
        const auto c = static_cast<unsigned char>(ch);
        const bool unreserved =
            (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~' || c == '/';
        if (unreserved)
            encoded.push_back(static_cast<char>(c));
        else
            encoded += std::format("%{:02X}", static_cast<unsigned>(c));
    }
    return encoded;
}

std::wstring ToWideAscii(std::string_view text)
{
    std::wstring out;
    out.reserve(text.size());
    for (const char c : text)
        out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct GitHubService::State
{
    enum class WorkerPhase : std::uint8_t
    {
        Idle,
        Active,
        Closing,
        Stopped
    };

    std::mutex mutex;
    wil::unique_event wake;
    wil::unique_event abortEvent;
    std::atomic_bool cancelled{false};
    HWND window{};
    UINT message{};
    std::shared_ptr<IGitHubTransport> transport;
    std::shared_ptr<ICredentialStore> credentials;
    std::deque<GitHubRequest> queue;
    std::deque<GitHubResult> completed;
    WorkerPhase phase{WorkerPhase::Idle};
    std::wstring activeToken;
    std::wstring activeLogin;

    void FinishStoppedLocked() noexcept
    {
        ScrubWideString(activeToken);
        phase = WorkerPhase::Stopped;
    }

    static HRESULT DecodeMarkdownPayload(std::string_view bytes, std::wstring& outText, bool& outBom) noexcept
    {
        outBom = bytes.size() >= 3u && static_cast<unsigned char>(bytes[0]) == 0xEFu && static_cast<unsigned char>(bytes[1]) == 0xBBu &&
                 static_cast<unsigned char>(bytes[2]) == 0xBFu;
        if (outBom)
            bytes.remove_prefix(3u);
        if (bytes.find('\0') != std::string_view::npos)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        std::wstring decoded;
        const auto res = DxUi::RichText::TryDecodeUtf8(bytes, decoded);
        if (res != DxUi::RichText::SourceResult::Success)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        if (decoded.size() > DxUi::RichText::Document::MaximumUnits)
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        outText = std::move(decoded);
        return S_OK;
    }

    static void Execute(const std::shared_ptr<State>& self, const GitHubRequest& req, GitHubResult& res)
    {
        res.kind      = req.kind;
        res.requestId = req.requestId;
        std::shared_ptr<IGitHubTransport> transport;
        std::shared_ptr<ICredentialStore> credStore;
        std::wstring token;
        std::wstring login;
        {
            std::lock_guard lock(self->mutex);
            transport = self->transport;
            credStore = self->credentials;
            token     = self->activeToken;
            login     = self->activeLogin;
        }
        const auto scrubToken = wil::scope_exit([&]() noexcept { ScrubWideString(token); });
        if (! transport || ! credStore)
        {
            res.status = E_UNEXPECTED;
            return;
        }

        const auto fetchUserAndRepos = [&](std::wstring_view candidateToken) -> HRESULT
        {
            GitHubHttpRequest userReq;
            userReq.path        = L"/user";
            userReq.bearerToken = std::wstring(candidateToken);
            GitHubHttpResponse userResp;
            const HRESULT uHr = transport->SendRequest(userReq, &self->cancelled, self->abortEvent.get(), userResp);
            if (FAILED(uHr))
                return res.status = uHr;
            if (userResp.httpStatusCode < 200u || userResp.httpStatusCode >= 300u)
            {
                res.errorMessage  = FormatGitHubErrorMessage(userResp);
                return res.status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            }
            std::wstring parsedLogin;
            const HRESULT lpHr = ParseGitHubUserLoginJson(userResp.body, parsedLogin);
            if (FAILED(lpHr))
                return res.status = lpHr;

            std::vector<GitHubRepoSummary> repos;
            for (unsigned page = 1u; page <= 5u && repos.size() < MaximumGitHubUserRepos; ++page)
            {
                GitHubHttpRequest repoReq;
                repoReq.path        = std::format(L"/user/repos?sort=pushed&per_page=100&page={}", page);
                repoReq.bearerToken = std::wstring(candidateToken);
                GitHubHttpResponse repoResp;
                const HRESULT rHr = transport->SendRequest(repoReq, &self->cancelled, self->abortEvent.get(), repoResp);
                if (FAILED(rHr) || repoResp.httpStatusCode < 200u || repoResp.httpStatusCode >= 300u)
                    break;
                const std::size_t beforeCount = repos.size();
                (void)ParseGitHubReposPageJson(repoResp.body, repos, MaximumGitHubUserRepos);
                if (! repoResp.hasNextPage || repos.size() == beforeCount)
                    break;
            }

            {
                std::lock_guard lock(self->mutex);
                self->activeToken = std::wstring(candidateToken);
                self->activeLogin = parsedLogin;
            }
            res.signedIn     = true;
            res.accountLogin = std::move(parsedLogin);
            res.userRepos    = std::move(repos);
            return S_OK;
        };

        switch (req.kind)
        {
            case GitHubRequest::Kind::LoadSavedAccount:
            {
                StoredCredential stored;
                const HRESULT cr       = credStore->Read(GitHubCredentialTarget, stored);
                const auto scrubStored = wil::scope_exit([&]() noexcept { ScrubWideString(stored.secret); });
                if (FAILED(cr) || stored.secret.empty())
                {
                    res.signedIn = false;
                    res.status   = S_OK;
                    return;
                }
                if (FAILED(fetchUserAndRepos(stored.secret)) && ! res.errorMessage.empty())
                {
                    // Stale/revoked credential falls back cleanly to signed-out status without destroying user edits.
                    res.signedIn = false;
                    (void)credStore->Delete(GitHubCredentialTarget);
                    std::lock_guard lock(self->mutex);
                    ScrubWideString(self->activeToken);
                    self->activeLogin.clear();
                }
                return;
            }
            case GitHubRequest::Kind::SignInWithPat:
            {
                if (req.tokenOrDeviceCode.empty())
                {
                    res.status = E_INVALIDARG;
                    return;
                }
                if (SUCCEEDED(fetchUserAndRepos(req.tokenOrDeviceCode)))
                    res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, req.tokenOrDeviceCode);
                return;
            }
            case GitHubRequest::Kind::ImportGhCli:
            {
                std::wstring cliToken;
                const auto scrubCli = wil::scope_exit([&]() noexcept { ScrubWideString(cliToken); });
                const HRESULT cliHr = TryImportGhCliToken(cliToken);
                if (FAILED(cliHr))
                {
                    res.status       = cliHr;
                    res.errorMessage = L"GitHub CLI ('gh auth token') is unavailable or not signed in.";
                    return;
                }
                if (SUCCEEDED(fetchUserAndRepos(cliToken)))
                    res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, cliToken);
                return;
            }
            case GitHubRequest::Kind::StartDeviceFlow:
            {
                GitHubHttpRequest dcReq;
                dcReq.host         = L"github.com";
                dcReq.method       = L"POST";
                dcReq.path         = L"/login/device/code";
                dcReq.acceptHeader = L"application/json";
                dcReq.contentType  = "application/x-www-form-urlencoded";
                dcReq.body         = std::format("client_id={}&scope=repo%20read%3Auser", DefaultDeviceClientId);
                GitHubHttpResponse dcResp;
                const HRESULT hr = transport->SendRequest(dcReq, &self->cancelled, self->abortEvent.get(), dcResp);
                if (FAILED(hr))
                {
                    res.status = hr;
                    return;
                }
                if (dcResp.httpStatusCode < 200u || dcResp.httpStatusCode >= 300u)
                {
                    res.status       = E_FAIL;
                    res.errorMessage = FormatGitHubErrorMessage(dcResp);
                    return;
                }
                GitHubDeviceCodeInfo info;
                const HRESULT pHr = ParseGitHubDeviceCodeJson(dcResp.body, info);
                res.status        = pHr;
                if (SUCCEEDED(pHr))
                    res.deviceCode = std::move(info);
                return;
            }
            case GitHubRequest::Kind::PollDeviceFlow:
            {
                std::string dcUtf8;
                if (DxUi::RichText::TryEncodeUtf8(req.tokenOrDeviceCode, dcUtf8) != DxUi::RichText::SourceResult::Success)
                {
                    res.status = E_INVALIDARG;
                    return;
                }
                GitHubHttpRequest pReq;
                pReq.host         = L"github.com";
                pReq.method       = L"POST";
                pReq.path         = L"/login/oauth/access_token";
                pReq.acceptHeader = L"application/json";
                pReq.contentType  = "application/x-www-form-urlencoded";
                pReq.body = std::format("client_id={}&device_code={}&grant_type=urn:ietf:params:oauth:grant-type:device_code", DefaultDeviceClientId, dcUtf8);
                GitHubHttpResponse pResp;
                const HRESULT hr = transport->SendRequest(pReq, &self->cancelled, self->abortEvent.get(), pResp);
                if (FAILED(hr))
                {
                    res.status = hr;
                    return;
                }
                GitHubDevicePollResult poll;
                const HRESULT decHr = ParseGitHubDevicePollJson(pResp.body, poll);
                if (FAILED(decHr))
                {
                    res.status = decHr;
                    return;
                }
                if (poll.state == GitHubDevicePollResult::State::Authorized && ! poll.accessToken.empty())
                {
                    if (SUCCEEDED(fetchUserAndRepos(poll.accessToken)))
                        res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, poll.accessToken);
                    ScrubWideString(poll.accessToken);
                }
                res.devicePoll = std::move(poll);
                return;
            }
            case GitHubRequest::Kind::RefreshUserRepos:
            {
                if (token.empty())
                {
                    res.signedIn = false;
                    res.status   = S_OK;
                    return;
                }
                (void)fetchUserAndRepos(token);
                return;
            }
            case GitHubRequest::Kind::FetchRepoTree:
            {
                res.openAfterTree    = req.openAfterTree;
                res.fileRelativePath = req.filePath;
                if (! IsValidGitHubTokenSegment(req.owner, 128u) || ! IsValidGitHubTokenSegment(req.repo, 128u))
                {
                    res.status = E_INVALIDARG;
                    return;
                }
                std::wstring branch = req.branch;
                if (branch.empty())
                {
                    GitHubHttpRequest infoReq;
                    infoReq.path        = std::format(L"/repos/{}/{}", req.owner, req.repo);
                    infoReq.bearerToken = token;
                    GitHubHttpResponse infoResp;
                    const HRESULT iHr = transport->SendRequest(infoReq, &self->cancelled, self->abortEvent.get(), infoResp);
                    if (FAILED(iHr))
                    {
                        res.status = iHr;
                        return;
                    }
                    if (infoResp.httpStatusCode < 200u || infoResp.httpStatusCode >= 300u)
                    {
                        res.status       = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                        res.errorMessage = FormatGitHubErrorMessage(infoResp);
                        return;
                    }
                    if (FAILED(ParseGitHubDefaultBranchJson(infoResp.body, branch)))
                        branch = L"main";
                }
                std::string branchUtf8 = EncodeUrlPathSegments(branch);
                if (branchUtf8.empty())
                {
                    res.status = E_INVALIDARG;
                    return;
                }
                GitHubHttpRequest treeReq;
                treeReq.path        = std::format(L"/repos/{}/{}/git/trees/{}?recursive=1", req.owner, req.repo, ToWideAscii(branchUtf8));
                treeReq.bearerToken = token;
                GitHubHttpResponse treeResp;
                const HRESULT tHr = transport->SendRequest(treeReq, &self->cancelled, self->abortEvent.get(), treeResp);
                if (FAILED(tHr))
                {
                    res.status = tHr;
                    return;
                }
                if (treeResp.httpStatusCode < 200u || treeResp.httpStatusCode >= 300u)
                {
                    res.status       = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    res.errorMessage = FormatGitHubErrorMessage(treeResp);
                    return;
                }
                GitHubTreeSnapshot treeSnap;
                res.status = ParseGitHubTreeJson(treeResp.body, req.owner, req.repo, branch, treeSnap);
                res.tree   = std::move(treeSnap);
                return;
            }
            case GitHubRequest::Kind::FetchRawFile:
            {
                res.fileOwner                 = req.owner;
                res.fileRepo                  = req.repo;
                res.fileBranch                = req.branch;
                res.fileRelativePath          = req.filePath;
                res.fileSha                   = req.sha;
                res.fileHtmlUrl               = req.htmlUrl;
                const std::string encodedPath = EncodeUrlPathSegments(req.filePath);
                const std::string encodedRef  = EncodeUrlPathSegments(req.branch.empty() ? L"main" : req.branch);
                if (encodedPath.empty() || encodedRef.empty())
                {
                    res.status = E_INVALIDARG;
                    return;
                }
                GitHubHttpRequest fileReq;
                fileReq.path         = std::format(L"/repos/{}/{}/contents/{}?ref={}", req.owner, req.repo, ToWideAscii(encodedPath), ToWideAscii(encodedRef));
                fileReq.acceptHeader = L"application/vnd.github.raw+json";
                fileReq.bearerToken  = token;
                fileReq.maximumResponseBytes = DefaultDocumentBytes;
                GitHubHttpResponse fileResp;
                const HRESULT fHr = transport->SendRequest(fileReq, &self->cancelled, self->abortEvent.get(), fileResp);
                if (FAILED(fHr))
                {
                    res.status = fHr;
                    return;
                }
                if (fileResp.httpStatusCode < 200u || fileResp.httpStatusCode >= 300u)
                {
                    res.status       = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    res.errorMessage = FormatGitHubErrorMessage(fileResp);
                    return;
                }
                res.status = DecodeMarkdownPayload(fileResp.body, res.fileText, res.fileBom);
                return;
            }
            case GitHubRequest::Kind::SignOut:
            {
                res.status = credStore->Delete(GitHubCredentialTarget);
                std::lock_guard lock(self->mutex);
                ScrubWideString(self->activeToken);
                self->activeLogin.clear();
                res.signedIn = false;
                return;
            }
        }
    }

    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
        try
        {
            const DxUi::Failure::Scope failureScope;
            for (;;)
            {
                GitHubRequest req;
                {
                    std::lock_guard lock(self->mutex);
                    if (self->phase == WorkerPhase::Closing)
                    {
                        self->FinishStoppedLocked();
                        return;
                    }
                    if (self->queue.empty())
                    {
                        self->phase = WorkerPhase::Idle;
                        goto waitNext;
                    }
                    req = std::move(self->queue.front());
                    self->queue.pop_front();
                    self->phase = WorkerPhase::Active;
                }
                {
                    GitHubResult result;
                    Execute(self, req, result);
                    std::lock_guard lock(self->mutex);
                    if (self->phase == WorkerPhase::Closing)
                    {
                        self->FinishStoppedLocked();
                        return;
                    }
                    self->completed.push_back(std::move(result));
                    if (self->window)
                        (void)PostMessageW(self->window, self->message, static_cast<WPARAM>(req.requestId), 0);
                    continue;
                }
            waitNext:
                const HANDLE waits[]{self->abortEvent.get(), self->wake.get()};
                const DWORD ready = WaitForMultipleObjects(2u, waits, FALSE, INFINITE);
                if (ready == WAIT_OBJECT_0 || ready == WAIT_FAILED)
                {
                    std::lock_guard lock(self->mutex);
                    self->FinishStoppedLocked();
                    return;
                }
            }
        }
        catch (const std::bad_alloc&)
        {
            DxUi::Failure::FailAllocation();
        }
        catch (const std::exception&)
        {
            DxUi::Failure::FailUnexpected();
        }
    }
};
#pragma warning(pop)

GitHubService::~GitHubService()
{
    Close();
}

HRESULT GitHubService::Initialize(HWND window,
                                  UINT message,
                                  std::shared_ptr<IGitHubTransport> transport,
                                  std::shared_ptr<ICredentialStore> credentials) noexcept
try
{
    if ((window && message < WM_APP) || ! transport || ! credentials)
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    staged->abortEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! staged->wake || ! staged->abortEvent)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window      = window;
    staged->message     = message;
    staged->transport   = std::move(transport);
    staged->credentials = std::move(credentials);
    auto ownership      = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT hr    = ProcessWork::Acquire(ownership->lease);
    if (FAILED(hr))
        return hr;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT GitHubService::Submit(GitHubRequest request) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->phase == State::WorkerPhase::Closing || _state->phase == State::WorkerPhase::Stopped)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->queue.size() >= 32u)
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    _state->phase = State::WorkerPhase::Active;
    _state->queue.push_back(std::move(request));
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::optional<GitHubResult> GitHubService::TakeResult()
{
    if (! _state)
        return std::nullopt;
    std::lock_guard lock(_state->mutex);
    if (_state->completed.empty())
        return std::nullopt;
    auto res = std::move(_state->completed.front());
    _state->completed.pop_front();
    return res;
}

bool GitHubService::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->phase == State::WorkerPhase::Active || ! _state->queue.empty();
}

bool GitHubService::Finished() const noexcept
{
    if (! _state)
        return true;
    std::lock_guard lock(_state->mutex);
    return _state->phase == State::WorkerPhase::Stopped || _state->phase == State::WorkerPhase::Idle;
}

void GitHubService::Stop() noexcept
{
    if (! _state)
        return;
    std::lock_guard lock(_state->mutex);
    _state->cancelled.store(true, std::memory_order_release);
    _state->phase  = State::WorkerPhase::Closing;
    _state->window = nullptr;
    _state->queue.clear();
    ScrubWideString(_state->activeToken);
    (void)SetEvent(_state->abortEvent.get());
    (void)SetEvent(_state->wake.get());
}

void GitHubService::Close() noexcept
{
    Stop();
    _state.reset();
}
} // namespace RedTilde
