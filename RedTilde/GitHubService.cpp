#include "GitHubService.h"
#include "Common/ProcessWork.h"
#include "Common/Resources.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include "res/Resource.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <atomic>
#include <deque>
#include <format>
#include <mutex>
#include <wil/resource.h>
#include <winhttp.h>

namespace RedTilde
{
namespace
{
std::wstring ToWideAscii(std::string_view text)
{
    std::wstring out;
    out.reserve(text.size());
    for (const char c : text)
        out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
    return out;
}

void ScrubResultSecrets(GitHubResult& result) noexcept
{
    if (result.devicePoll)
        ScrubWideString(result.devicePoll->accessToken);
}

[[nodiscard]] GitHubErrorCode ErrorCodeFromTransportHr(HRESULT hr) noexcept
{
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        return GitHubErrorCode::Cancelled;
    if (hr == HRESULT_FROM_WIN32(ERROR_TIMEOUT) || hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_TIMEOUT))
        return GitHubErrorCode::Timeout;
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
        return GitHubErrorCode::FileTooLarge;
    return GitHubErrorCode::TransportFailed;
}
} // namespace

std::wstring FormatLocalizedGitHubError(GitHubErrorCode errorCode, HRESULT transportStatus, unsigned httpStatusCode, std::wstring_view serverMessage)
{
    switch (errorCode)
    {
        case GitHubErrorCode::EmptyUrl: return LoadStringResource(nullptr, IDS_GITHUB_ERR_EMPTY_URL);
        case GitHubErrorCode::UrlTooLongOrInvalidEncoding: return LoadStringResource(nullptr, IDS_GITHUB_ERR_URL_ENCODING);
        case GitHubErrorCode::UrlUnsafeCharacters: return LoadStringResource(nullptr, IDS_GITHUB_ERR_URL_CHARS);
        case GitHubErrorCode::HttpsRequired: return LoadStringResource(nullptr, IDS_GITHUB_ERR_HTTPS_REQUIRED);
        case GitHubErrorCode::HostNotGitHub: return LoadStringResource(nullptr, IDS_GITHUB_ERR_HOST_NOT_GITHUB);
        case GitHubErrorCode::UnsupportedScheme: return LoadStringResource(nullptr, IDS_GITHUB_ERR_SCHEME);
        case GitHubErrorCode::DotPathSegment: return LoadStringResource(nullptr, IDS_GITHUB_ERR_DOT_SEGMENT);
        case GitHubErrorCode::MissingOwnerOrRepo: return LoadStringResource(nullptr, IDS_GITHUB_ERR_MISSING_OWNER_REPO);
        case GitHubErrorCode::InvalidOwnerOrRepo: return LoadStringResource(nullptr, IDS_GITHUB_ERR_INVALID_OWNER_REPO);
        case GitHubErrorCode::InvalidBranch: return LoadStringResource(nullptr, IDS_GITHUB_ERR_INVALID_BRANCH);
        case GitHubErrorCode::InvalidFilePath: return LoadStringResource(nullptr, IDS_GITHUB_ERR_INVALID_FILE_PATH);
        case GitHubErrorCode::UnsupportedUrlPath: return LoadStringResource(nullptr, IDS_GITHUB_ERR_UNSUPPORTED_PATH);
        case GitHubErrorCode::RateLimited: return LoadStringResource(nullptr, IDS_GITHUB_ERR_RATE_LIMITED);
        case GitHubErrorCode::Unauthorized: return LoadStringResource(nullptr, IDS_GITHUB_ERR_UNAUTHORIZED);
        case GitHubErrorCode::NotFound: return LoadStringResource(nullptr, IDS_GITHUB_ERR_NOT_FOUND);
        case GitHubErrorCode::GhCliUnavailable: return LoadStringResource(nullptr, IDS_GITHUB_ERR_GH_CLI_UNAVAILABLE);
        case GitHubErrorCode::DeviceFlowNotConfigured: return LoadStringResource(nullptr, IDS_GITHUB_ERR_DEVICE_UNAVAILABLE);
        case GitHubErrorCode::DeviceAccessDenied: return LoadStringResource(nullptr, IDS_GITHUB_ERR_DEVICE_DENIED);
        case GitHubErrorCode::DeviceExpired: return LoadStringResource(nullptr, IDS_GITHUB_ERR_DEVICE_EXPIRED);
        case GitHubErrorCode::InvalidResponse: return LoadStringResource(nullptr, IDS_GITHUB_ERR_INVALID_RESPONSE);
        case GitHubErrorCode::FileTooLarge: return LoadStringResource(nullptr, IDS_GITHUB_ERR_FILE_TOO_LARGE);
        case GitHubErrorCode::FileNotUtf8: return LoadStringResource(nullptr, IDS_GITHUB_ERR_FILE_NOT_UTF8);
        case GitHubErrorCode::Cancelled: return LoadStringResource(nullptr, IDS_GITHUB_ERR_CANCELLED);
        case GitHubErrorCode::Timeout: return LoadStringResource(nullptr, IDS_GITHUB_ERR_TIMEOUT);
        case GitHubErrorCode::ServerMessage:
            if (! serverMessage.empty())
                return std::wstring(serverMessage);
            break;
        case GitHubErrorCode::HttpStatus: return FormatStringResource(nullptr, IDS_GITHUB_ERR_HTTP_STATUS, httpStatusCode);
        case GitHubErrorCode::TransportFailed:
        case GitHubErrorCode::None: break;
    }
    if (! serverMessage.empty())
        return std::wstring(serverMessage);
    if (FAILED(transportStatus))
        return ErrorText(transportStatus);
    if (httpStatusCode != 0u)
        return FormatStringResource(nullptr, IDS_GITHUB_ERR_HTTP_STATUS, httpStatusCode);
    return LoadStringResource(nullptr, IDS_GITHUB_ERR_INVALID_RESPONSE);
}

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
    wil::unique_event abortEvent;
    std::atomic_bool cancelled{false};
    HWND window{};
    UINT message{};
    std::shared_ptr<IGitHubTransport> transport;
    std::shared_ptr<ICredentialStore> credentials;
    std::string deviceClientId;
    std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)> ghCliImporter;
    std::deque<GitHubRequest> queue;
    std::deque<GitHubResult> completed;
    WorkerPhase phase{WorkerPhase::Idle};
    bool workerRunning{};
    bool credStoreChecked{};
    std::wstring activeToken;
    std::wstring activeLogin;

    ~State() noexcept
    {
        ScrubWideString(activeToken);
        for (auto& req : queue)
            req.tokenOrDeviceCode.Clear();
        for (auto& res : completed)
            ScrubResultSecrets(res);
    }

    void FinishStoppedLocked() noexcept
    {
        ScrubWideString(activeToken);
        for (auto& req : queue)
            req.tokenOrDeviceCode.Clear();
        queue.clear();
        for (auto& res : completed)
            ScrubResultSecrets(res);
        workerRunning = false;
        phase         = WorkerPhase::Stopped;
    }

    static HRESULT DecodeMarkdownPayload(std::string_view bytes, std::wstring& outText, bool& outBom)
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
        res.flowId    = req.flowId;
        std::shared_ptr<IGitHubTransport> transport;
        std::shared_ptr<ICredentialStore> credStore;
        std::string clientId;
        std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)> importer;
        std::wstring token;
        std::wstring login;
        bool checkedCreds{};
        {
            std::lock_guard lock(self->mutex);
            transport = self->transport;
            credStore = self->credentials;
            clientId  = self->deviceClientId;
            importer  = self->ghCliImporter;
            token.assign(self->activeToken);
            login        = self->activeLogin;
            checkedCreds = self->credStoreChecked;
        }
        const auto scrubToken = wil::scope_exit([&]() noexcept { ScrubWideString(token); });
        if (! transport || ! credStore)
        {
            res.status       = E_UNEXPECTED;
            res.errorCode    = GitHubErrorCode::TransportFailed;
            res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
            return;
        }

        const auto setFailureFromResponse = [&](const GitHubHttpResponse& resp, HRESULT hr)
        {
            res.httpStatusCode = resp.httpStatusCode;
            res.errorCode      = resp.errorCode != GitHubErrorCode::None ? resp.errorCode : ClassifyGitHubHttpError(resp);
            if (res.errorCode == GitHubErrorCode::None)
                res.errorCode = ErrorCodeFromTransportHr(hr);
            res.errorMessage =
                FormatLocalizedGitHubError(res.errorCode, FAILED(resp.transportStatus) ? resp.transportStatus : hr, resp.httpStatusCode, resp.errorMessage);
            res.status = hr;
        };

        const auto invalidateSavedAuthIf401 = [&](unsigned httpStatus, std::wstring_view usedToken)
        {
            if (httpStatus != 401u || usedToken.empty())
                return;
            (void)credStore->Delete(GitHubCredentialTarget);
            res.authInvalidated = true;
            res.signedIn        = false;
            std::lock_guard lock(self->mutex);
            ScrubWideString(self->activeToken);
            self->activeLogin.clear();
            self->credStoreChecked = true;
        };

        const auto ensureTokenFromCredStore = [&]()
        {
            if (! token.empty() || checkedCreds)
                return;
            StoredCredential stored;
            const auto scrubStored = wil::scope_exit([&]() noexcept { ScrubWideString(stored.secret); });
            const HRESULT cr       = credStore->Read(GitHubCredentialTarget, stored);
            std::lock_guard lock(self->mutex);
            self->credStoreChecked = true;
            if (SUCCEEDED(cr) && ! stored.secret.empty())
            {
                token.assign(stored.secret);
                ScrubWideString(self->activeToken);
                self->activeToken.assign(stored.secret);
                self->activeLogin = stored.username;
                login             = stored.username;
            }
        };

        const auto fetchUserAndRepos = [&](std::wstring_view candidateToken) -> HRESULT
        {
            GitHubHttpRequest userReq;
            userReq.path = L"/user";
            userReq.bearerToken.assign(candidateToken);
            const auto scrubUserToken = wil::scope_exit([&]() noexcept { ScrubWideString(userReq.bearerToken); });
            GitHubHttpResponse userResp;
            const HRESULT uHr = transport->SendRequest(userReq, &self->cancelled, self->abortEvent.get(), userResp);
            if (FAILED(uHr))
            {
                setFailureFromResponse(userResp, uHr);
                return res.status;
            }
            if (userResp.httpStatusCode < 200u || userResp.httpStatusCode >= 300u)
            {
                setFailureFromResponse(userResp, HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
                return res.status;
            }
            std::wstring parsedLogin;
            const HRESULT lpHr = ParseGitHubUserLoginJson(userResp.body, parsedLogin);
            if (FAILED(lpHr))
            {
                res.errorCode     = GitHubErrorCode::InvalidResponse;
                res.errorMessage  = FormatLocalizedGitHubError(res.errorCode, lpHr);
                return res.status = lpHr;
            }

            std::vector<GitHubRepoSummary> repos;
            for (unsigned page = 1u; page <= 5u && repos.size() < MaximumGitHubUserRepos; ++page)
            {
                GitHubHttpRequest repoReq;
                repoReq.path = std::format(L"/user/repos?sort=pushed&per_page=100&page={}", page);
                repoReq.bearerToken.assign(candidateToken);
                const auto scrubRepoToken = wil::scope_exit([&]() noexcept { ScrubWideString(repoReq.bearerToken); });
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
                ScrubWideString(self->activeToken);
                self->activeToken.assign(candidateToken);
                self->activeLogin      = parsedLogin;
                self->credStoreChecked = true;
            }
            res.httpStatusCode = userResp.httpStatusCode;
            res.signedIn       = true;
            res.accountLogin   = std::move(parsedLogin);
            res.userRepos      = std::move(repos);
            return S_OK;
        };

        switch (req.kind)
        {
            case GitHubRequest::Kind::LoadSavedAccount:
            {
                StoredCredential stored;
                const auto scrubStored = wil::scope_exit([&]() noexcept { ScrubWideString(stored.secret); });
                const HRESULT cr       = credStore->Read(GitHubCredentialTarget, stored);
                {
                    std::lock_guard lock(self->mutex);
                    self->credStoreChecked = true;
                }
                if (FAILED(cr) || stored.secret.empty())
                {
                    res.signedIn = false;
                    res.status   = S_OK;
                    return;
                }
                if (FAILED(fetchUserAndRepos(stored.secret)))
                {
                    res.signedIn = false;
                    invalidateSavedAuthIf401(res.httpStatusCode, stored.secret);
                }
                return;
            }
            case GitHubRequest::Kind::SignInWithPat:
            {
                if (req.tokenOrDeviceCode.Empty())
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::Unauthorized;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                if (SUCCEEDED(fetchUserAndRepos(req.tokenOrDeviceCode.View())))
                {
                    res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, req.tokenOrDeviceCode.View());
                    if (FAILED(res.status))
                    {
                        res.errorCode    = GitHubErrorCode::TransportFailed;
                        res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                    }
                }
                return;
            }
            case GitHubRequest::Kind::ImportGhCli:
            {
                std::wstring cliToken;
                const auto scrubCli = wil::scope_exit([&]() noexcept { ScrubWideString(cliToken); });
                const HRESULT cliHr = importer ? importer(cliToken, &self->cancelled, self->abortEvent.get())
                                               : TryImportGhCliToken(cliToken, &self->cancelled, self->abortEvent.get());
                if (FAILED(cliHr))
                {
                    res.status       = cliHr;
                    res.errorCode    = (cliHr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) ? GitHubErrorCode::Cancelled
                                       : (cliHr == HRESULT_FROM_WIN32(ERROR_TIMEOUT)) ? GitHubErrorCode::Timeout
                                                                                      : GitHubErrorCode::GhCliUnavailable;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, cliHr);
                    return;
                }
                if (SUCCEEDED(fetchUserAndRepos(cliToken)))
                {
                    res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, cliToken);
                    if (FAILED(res.status))
                    {
                        res.errorCode    = GitHubErrorCode::TransportFailed;
                        res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                    }
                }
                return;
            }
            case GitHubRequest::Kind::StartDeviceFlow:
            {
                if (clientId.empty())
                {
                    res.status       = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                    res.errorCode    = GitHubErrorCode::DeviceFlowNotConfigured;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                GitHubHttpRequest dcReq;
                dcReq.host         = L"github.com";
                dcReq.method       = L"POST";
                dcReq.path         = L"/login/device/code";
                dcReq.acceptHeader = L"application/json";
                dcReq.contentType  = "application/x-www-form-urlencoded";
                dcReq.body         = std::format("client_id={}&scope=repo%20read%3Auser", clientId);
                GitHubHttpResponse dcResp;
                const HRESULT hr = transport->SendRequest(dcReq, &self->cancelled, self->abortEvent.get(), dcResp);
                if (FAILED(hr))
                {
                    setFailureFromResponse(dcResp, hr);
                    return;
                }
                if (dcResp.httpStatusCode < 200u || dcResp.httpStatusCode >= 300u)
                {
                    setFailureFromResponse(dcResp, E_FAIL);
                    return;
                }
                GitHubDeviceCodeInfo info;
                const HRESULT pHr = ParseGitHubDeviceCodeJson(dcResp.body, info);
                res.status        = pHr;
                if (SUCCEEDED(pHr))
                    res.deviceCode = std::move(info);
                else
                {
                    res.errorCode    = GitHubErrorCode::InvalidResponse;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, pHr);
                }
                return;
            }
            case GitHubRequest::Kind::PollDeviceFlow:
            {
                if (clientId.empty())
                {
                    res.status       = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                    res.errorCode    = GitHubErrorCode::DeviceFlowNotConfigured;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                std::string encodedDc = PercentEncodeQueryComponent(req.tokenOrDeviceCode.View());
                const auto scrubDc    = wil::scope_exit([&]() noexcept { ScrubString(encodedDc); });
                if (encodedDc.empty())
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidResponse;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                GitHubHttpRequest pReq;
                pReq.host            = L"github.com";
                pReq.method          = L"POST";
                pReq.path            = L"/login/oauth/access_token";
                pReq.acceptHeader    = L"application/json";
                pReq.contentType     = "application/x-www-form-urlencoded";
                pReq.body            = std::format("client_id={}&device_code={}&grant_type=urn:ietf:params:oauth:grant-type:device_code", clientId, encodedDc);
                const auto scrubBody = wil::scope_exit([&]() noexcept { ScrubString(pReq.body); });
                GitHubHttpResponse pResp;
                const auto scrubRespBody = wil::scope_exit([&]() noexcept { ScrubString(pResp.body); });
                const HRESULT hr         = transport->SendRequest(pReq, &self->cancelled, self->abortEvent.get(), pResp);
                if (FAILED(hr))
                {
                    setFailureFromResponse(pResp, hr);
                    return;
                }
                GitHubDevicePollResult poll;
                const auto scrubPollToken = wil::scope_exit([&]() noexcept { ScrubWideString(poll.accessToken); });
                const HRESULT decHr       = ParseGitHubDevicePollJson(pResp.body, poll);
                if (FAILED(decHr))
                {
                    if (pResp.httpStatusCode >= 400u)
                        setFailureFromResponse(pResp, decHr);
                    else
                    {
                        res.status       = decHr;
                        res.errorCode    = GitHubErrorCode::InvalidResponse;
                        res.errorMessage = FormatLocalizedGitHubError(res.errorCode, decHr);
                    }
                    return;
                }
                if (poll.state == GitHubDevicePollResult::State::Authorized && ! poll.accessToken.empty())
                {
                    const HRESULT authHr = fetchUserAndRepos(poll.accessToken);
                    if (SUCCEEDED(authHr))
                    {
                        res.status = credStore->Write(GitHubCredentialTarget, res.accountLogin, poll.accessToken);
                        if (FAILED(res.status))
                        {
                            res.errorCode    = GitHubErrorCode::TransportFailed;
                            res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                        }
                    }
                    ScrubWideString(poll.accessToken);
                }
                else if (poll.state == GitHubDevicePollResult::State::AccessDenied)
                {
                    res.errorCode    = GitHubErrorCode::DeviceAccessDenied;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                }
                else if (poll.state == GitHubDevicePollResult::State::Expired)
                {
                    res.errorCode    = GitHubErrorCode::DeviceExpired;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                }
                else if (poll.state == GitHubDevicePollResult::State::Error)
                {
                    res.errorCode    = poll.errorMessage.empty() ? GitHubErrorCode::InvalidResponse : GitHubErrorCode::ServerMessage;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, S_OK, pResp.httpStatusCode, poll.errorMessage);
                }
                res.devicePoll = std::move(poll);
                return;
            }
            case GitHubRequest::Kind::RefreshUserRepos:
            {
                ensureTokenFromCredStore();
                if (token.empty())
                {
                    res.signedIn = false;
                    res.status   = S_OK;
                    return;
                }
                if (FAILED(fetchUserAndRepos(token)))
                {
                    res.signedIn = false;
                    invalidateSavedAuthIf401(res.httpStatusCode, token);
                }
                return;
            }
            case GitHubRequest::Kind::FetchRepoTree:
            {
                res.fileOwner        = req.owner;
                res.fileRepo         = req.repo;
                res.fileBranch       = req.branch;
                res.fileRelativePath = req.filePath;
                if (! IsValidGitHubTokenSegment(req.owner, 128u) || ! IsValidGitHubTokenSegment(req.repo, 128u))
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidOwnerOrRepo;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                if (! req.branch.empty() && ! IsValidGitHubBranchName(req.branch))
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidBranch;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                ensureTokenFromCredStore();

                std::wstring branch = req.branch;
                if (branch.empty())
                {
                    GitHubHttpRequest infoReq;
                    infoReq.path = std::format(L"/repos/{}/{}", req.owner, req.repo);
                    infoReq.bearerToken.assign(token);
                    const auto scrubInfoToken = wil::scope_exit([&]() noexcept { ScrubWideString(infoReq.bearerToken); });
                    GitHubHttpResponse infoResp;
                    const HRESULT iHr = transport->SendRequest(infoReq, &self->cancelled, self->abortEvent.get(), infoResp);
                    if (FAILED(iHr))
                    {
                        setFailureFromResponse(infoResp, iHr);
                        return;
                    }
                    if (infoResp.httpStatusCode < 200u || infoResp.httpStatusCode >= 300u)
                    {
                        invalidateSavedAuthIf401(infoResp.httpStatusCode, token);
                        setFailureFromResponse(infoResp, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
                        return;
                    }
                    if (FAILED(ParseGitHubDefaultBranchJson(infoResp.body, branch)))
                        branch = L"main";
                    res.fileBranch = branch;
                }
                if (! IsValidGitHubBranchName(branch))
                {
                    res.status       = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    res.errorCode    = GitHubErrorCode::InvalidBranch;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                const std::string branchEncoded = PercentEncodeUrlPathSegments(branch);
                if (branchEncoded.empty())
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidBranch;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                GitHubHttpRequest treeReq;
                treeReq.path = std::format(L"/repos/{}/{}/git/trees/{}?recursive=1", req.owner, req.repo, ToWideAscii(branchEncoded));
                treeReq.bearerToken.assign(token);
                const auto scrubTreeToken = wil::scope_exit([&]() noexcept { ScrubWideString(treeReq.bearerToken); });
                GitHubHttpResponse treeResp;
                const HRESULT tHr = transport->SendRequest(treeReq, &self->cancelled, self->abortEvent.get(), treeResp);
                if (FAILED(tHr))
                {
                    setFailureFromResponse(treeResp, tHr);
                    return;
                }
                if (treeResp.httpStatusCode < 200u || treeResp.httpStatusCode >= 300u)
                {
                    invalidateSavedAuthIf401(treeResp.httpStatusCode, token);
                    setFailureFromResponse(treeResp, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
                    return;
                }
                GitHubTreeSnapshot treeSnap;
                res.status = ParseGitHubTreeJson(treeResp.body, req.owner, req.repo, branch, treeSnap);
                if (SUCCEEDED(res.status))
                {
                    res.tree = std::move(treeSnap);
                }
                else
                {
                    res.errorCode    = GitHubErrorCode::InvalidResponse;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                }
                return;
            }
            case GitHubRequest::Kind::FetchRawFile:
            {
                res.fileOwner        = req.owner;
                res.fileRepo         = req.repo;
                res.fileBranch       = req.branch;
                res.fileRelativePath = req.filePath;
                res.fileSha          = req.sha;
                res.fileHtmlUrl      = req.htmlUrl;
                if (! IsValidGitHubTokenSegment(req.owner, 128u) || ! IsValidGitHubTokenSegment(req.repo, 128u))
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidOwnerOrRepo;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                const std::wstring_view effectiveBranch = req.branch.empty() ? std::wstring_view(L"main") : std::wstring_view(req.branch);
                if (! IsValidGitHubBranchName(effectiveBranch))
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidBranch;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                if (! IsValidGitHubRelativePath(req.filePath))
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidFilePath;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                ensureTokenFromCredStore();

                const std::string encodedPath = PercentEncodeUrlPathSegments(req.filePath);
                const std::string encodedRef  = PercentEncodeQueryComponent(effectiveBranch);
                if (encodedPath.empty() || encodedRef.empty())
                {
                    res.status       = E_INVALIDARG;
                    res.errorCode    = GitHubErrorCode::InvalidFilePath;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode);
                    return;
                }
                GitHubHttpRequest fileReq;
                fileReq.path         = std::format(L"/repos/{}/{}/contents/{}?ref={}", req.owner, req.repo, ToWideAscii(encodedPath), ToWideAscii(encodedRef));
                fileReq.acceptHeader = L"application/vnd.github.raw+json";
                fileReq.bearerToken.assign(token);
                fileReq.maximumResponseBytes = DefaultDocumentBytes;
                const auto scrubFileToken    = wil::scope_exit([&]() noexcept { ScrubWideString(fileReq.bearerToken); });
                GitHubHttpResponse fileResp;
                const HRESULT fHr = transport->SendRequest(fileReq, &self->cancelled, self->abortEvent.get(), fileResp);
                if (FAILED(fHr))
                {
                    setFailureFromResponse(fileResp, fHr);
                    return;
                }
                if (fileResp.httpStatusCode < 200u || fileResp.httpStatusCode >= 300u)
                {
                    invalidateSavedAuthIf401(fileResp.httpStatusCode, token);
                    setFailureFromResponse(fileResp, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
                    return;
                }
                res.status = DecodeMarkdownPayload(fileResp.body, res.fileText, res.fileBom);
                if (FAILED(res.status))
                {
                    res.errorCode    = (res.status == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))           ? GitHubErrorCode::FileTooLarge
                                       : (res.status == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION)) ? GitHubErrorCode::FileNotUtf8
                                                                                                          : GitHubErrorCode::InvalidResponse;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                }
                return;
            }
            case GitHubRequest::Kind::SignOut:
            {
                res.status = credStore->Delete(GitHubCredentialTarget);
                std::lock_guard lock(self->mutex);
                ScrubWideString(self->activeToken);
                self->activeLogin.clear();
                self->credStoreChecked = true;
                res.signedIn           = false;
                if (FAILED(res.status))
                {
                    res.errorCode    = GitHubErrorCode::TransportFailed;
                    res.errorMessage = FormatLocalizedGitHubError(res.errorCode, res.status);
                }
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
                HWND shutdownWindow{};
                UINT shutdownMessage{};
                bool stoppingBeforeExec{};
                {
                    std::lock_guard lock(self->mutex);
                    if (self->phase == WorkerPhase::Closing)
                    {
                        self->FinishStoppedLocked();
                        shutdownWindow     = self->window;
                        shutdownMessage    = self->message;
                        stoppingBeforeExec = true;
                    }
                    else if (self->queue.empty())
                    {
                        self->phase         = WorkerPhase::Idle;
                        self->workerRunning = false;
                        return;
                    }
                    else
                    {
                        req = std::move(self->queue.front());
                        self->queue.pop_front();
                        self->phase = WorkerPhase::Active;
                    }
                }
                if (stoppingBeforeExec)
                {
                    if (shutdownWindow && shutdownMessage >= WM_APP)
                        (void)PostMessageW(shutdownWindow, shutdownMessage, 0u, 0);
                    return;
                }
                const auto scrubReq = wil::scope_exit([&]() noexcept { req.tokenOrDeviceCode.Clear(); });

                GitHubResult result;
                Execute(self, req, result);

                HWND notifyWindow{};
                UINT notifyMessage{};
                bool stopping{};
                {
                    std::lock_guard lock(self->mutex);
                    if (self->phase == WorkerPhase::Closing)
                    {
                        ScrubResultSecrets(result);
                        self->FinishStoppedLocked();
                        notifyWindow  = self->window;
                        notifyMessage = self->message;
                        stopping      = true;
                    }
                    else
                    {
                        self->completed.push_back(std::move(result));
                        notifyWindow  = self->window;
                        notifyMessage = self->message;
                    }
                }
                if (notifyWindow && notifyMessage >= WM_APP)
                    (void)PostMessageW(notifyWindow, notifyMessage, stopping ? 0u : static_cast<WPARAM>(req.requestId), 0);
                if (stopping)
                    return;
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
                                  std::shared_ptr<ICredentialStore> credentials,
                                  std::string deviceClientId,
                                  std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)> ghCliImporter) noexcept
try
{
    if ((window && message < WM_APP) || ! transport || ! credentials)
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->abortEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! staged->abortEvent)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window         = window;
    staged->message        = message;
    staged->transport      = std::move(transport);
    staged->credentials    = std::move(credentials);
    staged->deviceClientId = std::move(deviceClientId);
    staged->ghCliImporter  = std::move(ghCliImporter);
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
    const auto scrubRequest = wil::scope_exit([&]() noexcept { request.tokenOrDeviceCode.Clear(); });
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->phase == State::WorkerPhase::Closing || _state->phase == State::WorkerPhase::Stopped)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->queue.size() >= 32u)
        return HRESULT_FROM_WIN32(ERROR_BUSY);

    const bool needSpawn = ! _state->workerRunning;
    _state->queue.push_back(std::move(request));
    _state->phase = State::WorkerPhase::Active;
    if (needSpawn)
    {
        auto ownership   = std::make_unique<ProcessWorkItem<State>>(_state);
        const HRESULT hr = ProcessWork::Acquire(ownership->lease);
        if (FAILED(hr))
        {
            _state->queue.back().tokenOrDeviceCode.Clear();
            _state->queue.pop_back();
            _state->phase = _state->queue.empty() ? State::WorkerPhase::Idle : State::WorkerPhase::Active;
            return hr;
        }
        if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        {
            const DWORD err = GetLastError();
            _state->queue.back().tokenOrDeviceCode.Clear();
            _state->queue.pop_back();
            _state->phase = _state->queue.empty() ? State::WorkerPhase::Idle : State::WorkerPhase::Active;
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
        (void)ownership.release();
        _state->workerRunning = true;
    }
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

bool GitHubService::DeviceFlowConfigured() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return ! _state->deviceClientId.empty();
}

bool GitHubService::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->phase == State::WorkerPhase::Active || _state->workerRunning || ! _state->queue.empty();
}

bool GitHubService::Finished() const noexcept
{
    if (! _state)
        return true;
    std::lock_guard lock(_state->mutex);
    return _state->phase == State::WorkerPhase::Stopped || (_state->phase == State::WorkerPhase::Idle && ! _state->workerRunning);
}

void GitHubService::Stop() noexcept
{
    if (! _state)
        return;
    std::lock_guard lock(_state->mutex);
    _state->cancelled.store(true, std::memory_order_release);
    for (auto& req : _state->queue)
        req.tokenOrDeviceCode.Clear();
    _state->queue.clear();
    for (auto& res : _state->completed)
        ScrubResultSecrets(res);
    ScrubWideString(_state->activeToken);
    if (_state->phase != State::WorkerPhase::Stopped)
        _state->phase = _state->workerRunning ? State::WorkerPhase::Closing : State::WorkerPhase::Stopped;
    (void)SetEvent(_state->abortEvent.get());
}

void GitHubService::Close() noexcept
{
    Stop();
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        _state->window = nullptr;
    }
    _state.reset();
}
} // namespace RedTilde
