// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
// clang-format on
#include "GitHubClient.h"
#include "CredentialStore.h"
#include "ProcessWork.h"
#include "RemoteImageImports.h"
#include "Settings.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <format>
#include <string>
#include <wil/resource.h>
#include <winhttp.h>
#include <yyjson.h>

namespace RedTilde
{
namespace
{
using Internet                   = wil::unique_any<HINTERNET, decltype(&WinHttpCloseHandle), WinHttpCloseHandle>;
using unique_yyjson_doc          = wil::unique_any<yyjson_doc*, decltype(&yyjson_doc_free), yyjson_doc_free>;
constexpr unsigned TimeoutConnMs = 8000u;
constexpr unsigned TimeoutReadMs = 15000u;

bool EqualOrdinalIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() &&
           (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}

bool ContainsOrdinalIgnoreCase(std::wstring_view text, std::wstring_view needle) noexcept
{
    if (needle.empty())
        return true;
    if (needle.size() > text.size())
        return false;
    for (std::size_t offset = 0u; offset <= text.size() - needle.size(); ++offset)
        if (EqualOrdinalIgnoreCase(text.substr(offset, needle.size()), needle))
            return true;
    return false;
}

bool StartsWithOrdinalIgnoreCase(std::wstring_view text, std::wstring_view prefix) noexcept
{
    return prefix.size() <= text.size() && EqualOrdinalIgnoreCase(text.substr(0u, prefix.size()), prefix);
}

bool AllowedGitHubHost(std::wstring_view host) noexcept
{
    return host == L"api.github.com" || host == L"github.com";
}

bool AllowedRelativeRequestPath(std::wstring_view path) noexcept
{
    if (path.empty() || path.front() != L'/' || path.size() > 2048u)
        return false;
    return std::ranges::none_of(path, [](wchar_t ch) { return ch < L' ' || ch == 0x7Fu; });
}

HRESULT EnsureWinHttpImports() noexcept
{
    HRESULT result = E_UNEXPECTED;
    __try
    {
        return __HrLoadAllImportsForDll("WINHTTP.dll");
    }
    __except (RemoteImageDetail::WinHttpImportFailure(GetExceptionInformation(), result))
    {
        return result;
    }
}

bool DecodeUtf8Sanitized(yyjson_val* val, std::wstring& out, std::size_t maxLen)
{
    out.clear();
    if (! val || ! yyjson_is_str(val))
        return true;
    const std::string_view u8(yyjson_get_str(val), yyjson_get_len(val));
    if (u8.size() > maxLen * 4u)
        return false;
    std::wstring decoded;
    if (DxUi::RichText::TryDecodeUtf8(u8, decoded) != DxUi::RichText::SourceResult::Success)
        return false;
    out = SanitizeUiText(decoded, maxLen);
    return true;
}

bool ValidRelativeRepoPath(std::wstring_view path) noexcept
{
    if (path.empty() || path.size() > 1024u || path.front() == L'/' || path.back() == L'/')
        return false;
    if (path.find(L'\\') != std::wstring_view::npos || path.find(L'\0') != std::wstring_view::npos)
        return false;
    std::size_t start{};
    unsigned segments{};
    while (start < path.size())
    {
        const auto slash = path.find(L'/', start);
        const auto part  = slash == std::wstring_view::npos ? path.substr(start) : path.substr(start, slash - start);
        if (part.empty() || part == L"." || part == L"..")
            return false;
        ++segments;
        if (segments > MaximumGitHubTreeDepth + 2u)
            return false;
        if (slash == std::wstring_view::npos)
            break;
        start = slash + 1u;
    }
    return true;
}

std::wstring TrimWhitespace(std::wstring_view text)
{
    while (! text.empty() && (text.front() == L' ' || text.front() == L'\t' || text.front() == L'\r' || text.front() == L'\n'))
        text.remove_prefix(1u);
    while (! text.empty() && (text.back() == L' ' || text.back() == L'\t' || text.back() == L'\r' || text.back() == L'\n'))
        text.remove_suffix(1u);
    return std::wstring(text);
}
} // namespace

ParsedGitHubTarget ParseGitHubUrl(std::wstring_view input)
{
    ParsedGitHubTarget parsed;
    std::wstring trimmed = TrimWhitespace(input);
    if (trimmed.empty())
    {
        parsed.errorReason = L"Enter a GitHub repository URL or owner/repo.";
        return parsed;
    }
    if (trimmed.size() > 1024u || ! DxUi::RichText::IsValidUtf16(trimmed))
    {
        parsed.errorReason = L"Repository address is invalid or too long.";
        return parsed;
    }
    for (const wchar_t ch : trimmed)
    {
        if (ch < L' ' || ch == 0x7Fu || ch == L'\\' || ch == L'@' || ch == L'?' || ch == L'#')
        {
            parsed.errorReason = L"GitHub repository addresses cannot contain credentials, queries, or control characters.";
            return parsed;
        }
    }

    std::wstring_view rest = trimmed;
    if (StartsWithOrdinalIgnoreCase(rest, L"http://"))
    {
        parsed.errorReason = L"Only HTTPS GitHub URLs are supported.";
        return parsed;
    }
    if (StartsWithOrdinalIgnoreCase(rest, L"https://"))
    {
        rest.remove_prefix(8u);
        if (StartsWithOrdinalIgnoreCase(rest, L"www.github.com/"))
            rest.remove_prefix(15u);
        else if (StartsWithOrdinalIgnoreCase(rest, L"github.com/"))
            rest.remove_prefix(11u);
        else
        {
            parsed.errorReason = L"Only github.com repositories are supported.";
            return parsed;
        }
    }
    else if (rest.find(L"://") != std::wstring_view::npos)
    {
        parsed.errorReason = L"Only https://github.com/ URLs or owner/repo shorthand are supported.";
        return parsed;
    }

    while (! rest.empty() && rest.back() == L'/')
        rest.remove_suffix(1u);

    std::vector<std::wstring_view> parts;
    std::size_t pos{};
    while (pos <= rest.size())
    {
        const auto slash = rest.find(L'/', pos);
        const auto seg   = slash == std::wstring_view::npos ? rest.substr(pos) : rest.substr(pos, slash - pos);
        if (! seg.empty())
        {
            if (seg == L"." || seg == L"..")
            {
                parsed.errorReason = L"Repository paths cannot contain '.' or '..'.";
                return parsed;
            }
            parts.push_back(seg);
        }
        if (slash == std::wstring_view::npos)
            break;
        pos = slash + 1u;
    }
    if (parts.size() < 2u)
    {
        parsed.errorReason = L"Specify owner/repo or https://github.com/owner/repo.";
        return parsed;
    }

    std::wstring owner(parts[0]);
    std::wstring repo(parts[1]);
    if (repo.size() > 4u && EqualOrdinalIgnoreCase(std::wstring_view(repo).substr(repo.size() - 4u), L".git"))
        repo.resize(repo.size() - 4u);

    if (! IsValidGitHubTokenSegment(owner, 128u) || ! IsValidGitHubTokenSegment(repo, 128u))
    {
        parsed.errorReason = L"Invalid GitHub owner or repository name.";
        return parsed;
    }

    parsed.owner = std::move(owner);
    parsed.repo  = std::move(repo);
    if (parts.size() >= 4u && (EqualOrdinalIgnoreCase(parts[2], L"tree") || EqualOrdinalIgnoreCase(parts[2], L"blob")))
    {
        parsed.branch = SanitizeUiText(parts[3], 256u);
        if (EqualOrdinalIgnoreCase(parts[2], L"blob") && parts.size() >= 5u)
        {
            std::wstring rel;
            for (std::size_t i = 4u; i < parts.size(); ++i)
            {
                if (! rel.empty())
                    rel.push_back(L'/');
                rel.append(parts[i]);
            }
            if (! ValidRelativeRepoPath(rel))
            {
                parsed.errorReason = L"Invalid repository file path.";
                return parsed;
            }
            parsed.filePath = std::move(rel);
        }
    }
    else if (parts.size() > 2u)
    {
        parsed.errorReason = L"Unsupported GitHub URL path; expected /owner/repo, /tree/<branch>, or /blob/<branch>/<file>.";
        return parsed;
    }

    parsed.valid = true;
    return parsed;
}

std::vector<GitHubRepoSummary> FilterAndRankGitHubRepos(std::span<const GitHubRepoSummary> repos, std::wstring_view query, std::size_t maximumResults)
{
    struct Ranked
    {
        int score{};
        std::size_t index{};
    };
    const std::wstring trimmed = TrimWhitespace(query);
    std::vector<Ranked> matches;
    matches.reserve(std::min(repos.size(), maximumResults));

    for (std::size_t i = 0u; i < repos.size(); ++i)
    {
        const auto& r = repos[i];
        if (trimmed.empty())
        {
            matches.push_back({100, i});
            continue;
        }
        int score = 0;
        if (EqualOrdinalIgnoreCase(r.fullName, trimmed) || EqualOrdinalIgnoreCase(r.repo, trimmed))
            score = 1000;
        else if (StartsWithOrdinalIgnoreCase(r.repo, trimmed))
            score = 800;
        else if (StartsWithOrdinalIgnoreCase(r.fullName, trimmed))
            score = 700;
        else if (ContainsOrdinalIgnoreCase(r.repo, trimmed))
            score = 500;
        else if (ContainsOrdinalIgnoreCase(r.fullName, trimmed))
            score = 400;
        else if (ContainsOrdinalIgnoreCase(r.description, trimmed))
            score = 200;
        if (score > 0)
            matches.push_back({score, i});
    }

    std::ranges::stable_sort(matches,
                             [&](const Ranked& a, const Ranked& b)
    {
        if (a.score != b.score)
            return a.score > b.score;
        return CompareStringOrdinal(repos[a.index].fullName.c_str(), -1, repos[b.index].fullName.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    });

    std::vector<GitHubRepoSummary> result;
    const std::size_t count = std::min(matches.size(), maximumResults);
    result.reserve(count);
    for (std::size_t i = 0u; i < count; ++i)
        result.push_back(repos[matches[i].index]);
    return result;
}

HRESULT WinHttpGitHubTransport::SendRequest(const GitHubHttpRequest& request,
                                            const std::atomic_bool* cancellation,
                                            HANDLE abortEvent,
                                            GitHubHttpResponse& outResponse) noexcept
try
{
    outResponse = {};
    if (! AllowedGitHubHost(request.host) || ! AllowedRelativeRequestPath(request.path) || request.maximumResponseBytes == 0u ||
        request.maximumResponseBytes > MaximumGitHubTreeJsonBytes)
        return outResponse.transportStatus = E_INVALIDARG;
    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        return outResponse.transportStatus = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    const HRESULT importHr = EnsureWinHttpImports();
    if (FAILED(importHr))
        return outResponse.transportStatus = importHr;

    const Internet session(WinHttpOpen(L"RedTilde/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (! session)
        return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());
    (void)WinHttpSetTimeouts(session.get(), TimeoutConnMs, TimeoutConnMs, TimeoutConnMs, TimeoutReadMs);

    const Internet connection(WinHttpConnect(session.get(), request.host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (! connection)
        return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());

    LPCWSTR acceptTypes[]{request.acceptHeader.empty() ? L"application/vnd.github+json" : request.acceptHeader.c_str(), nullptr};
    const Internet req(
        WinHttpOpenRequest(connection.get(), request.method.c_str(), request.path.c_str(), nullptr, WINHTTP_NO_REFERER, acceptTypes, WINHTTP_FLAG_SECURE));
    if (! req)
        return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    (void)WinHttpSetOption(req.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    std::wstring headers = L"X-GitHub-Api-Version: 2022-11-28\r\n";
    if (! request.contentType.empty())
    {
        std::wstring ctWide;
        if (DxUi::RichText::TryDecodeUtf8(request.contentType, ctWide) == DxUi::RichText::SourceResult::Success)
            headers += L"Content-Type: " + ctWide + L"\r\n";
    }
    if (! request.bearerToken.empty())
        headers += L"Authorization: Bearer " + request.bearerToken + L"\r\n";
    const auto scrubHeaders = wil::scope_exit([&]() noexcept { ScrubWideString(headers); });

    LPVOID bodyPtr     = request.body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(request.body.data());
    const DWORD bodySz = static_cast<DWORD>(request.body.size());
    if (! WinHttpSendRequest(req.get(), headers.c_str(), static_cast<DWORD>(headers.size()), bodyPtr, bodySz, bodySz, 0))
        return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());

    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        return outResponse.transportStatus = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    if (! WinHttpReceiveResponse(req.get(), nullptr))
        return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());

    DWORD statusCode{};
    DWORD statusSize = sizeof(statusCode);
    (void)WinHttpQueryHeaders(
        req.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    outResponse.httpStatusCode = statusCode;

    std::array<wchar_t, 512> linkBuf{};
    DWORD linkSize = static_cast<DWORD>(linkBuf.size() * sizeof(wchar_t));
    if (WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_CUSTOM, L"Link", linkBuf.data(), &linkSize, WINHTTP_NO_HEADER_INDEX))
    {
        const std::wstring_view linkView(linkBuf.data(), linkSize / sizeof(wchar_t));
        outResponse.hasNextPage = linkView.find(L"rel=\"next\"") != std::wstring_view::npos;
    }

    if (statusCode == 403u || statusCode == 429u)
    {
        std::array<wchar_t, 32> remain{};
        DWORD remSize = static_cast<DWORD>(remain.size() * sizeof(wchar_t));
        if (WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_CUSTOM, L"X-RateLimit-Remaining", remain.data(), &remSize, WINHTTP_NO_HEADER_INDEX))
        {
            const std::wstring_view rView(remain.data(), remSize / sizeof(wchar_t));
            if (rView == L"0" || statusCode == 429u)
                outResponse.rateLimited = true;
        }
        else if (statusCode == 429u)
        {
            outResponse.rateLimited = true;
        }
    }

    std::string body;
    std::array<char, 32768> chunk{};
    for (;;)
    {
        if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
            return outResponse.transportStatus = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        DWORD available{};
        if (! WinHttpQueryDataAvailable(req.get(), &available))
            return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());
        if (available == 0u)
            break;
        while (available > 0u)
        {
            const DWORD toRead = std::min<DWORD>(available, static_cast<DWORD>(chunk.size()));
            DWORD actuallyRead{};
            if (! WinHttpReadData(req.get(), chunk.data(), toRead, &actuallyRead))
                return outResponse.transportStatus = HRESULT_FROM_WIN32(GetLastError());
            if (actuallyRead == 0u)
                break;
            if (body.size() + actuallyRead > request.maximumResponseBytes)
                return outResponse.transportStatus = HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
            body.append(chunk.data(), actuallyRead);
            available -= actuallyRead;
        }
    }

    outResponse.body = std::move(body);
    if (statusCode >= 400u && ! outResponse.body.empty())
    {
        yyjson_read_err err{};
        const unique_yyjson_doc doc(yyjson_read_opts(outResponse.body.data(), outResponse.body.size(), 0, nullptr, &err));
        if (doc)
        {
            if (auto* const msg = yyjson_obj_get(yyjson_doc_get_root(doc.get()), "message"))
                (void)DecodeUtf8Sanitized(msg, outResponse.errorMessage, 256u);
        }
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubUserLoginJson(std::string_view jsonBytes, std::wstring& outLogin) noexcept
try
{
    outLogin.clear();
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    std::wstring login;
    if (! DecodeUtf8Sanitized(yyjson_obj_get(root, "login"), login, 128u) || ! IsValidGitHubTokenSegment(login, 128u))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    outLogin = std::move(login);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubReposPageJson(std::string_view jsonBytes, std::vector<GitHubRepoSummary>& inOutRepos, std::size_t maximumTotal) noexcept
try
{
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_arr(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    std::size_t idx{}, maxIdx{};
    yyjson_val* item{};
    yyjson_arr_foreach(root, idx, maxIdx, item)
    {
        if (inOutRepos.size() >= maximumTotal)
            break;
        if (! item || ! yyjson_is_obj(item))
            continue;
        GitHubRepoSummary summary;
        if (auto* const ownerObj = yyjson_obj_get(item, "owner"); ownerObj && yyjson_is_obj(ownerObj))
            (void)DecodeUtf8Sanitized(yyjson_obj_get(ownerObj, "login"), summary.owner, 128u);
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "name"), summary.repo, 128u);
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "full_name"), summary.fullName, 256u);
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "description"), summary.description, 256u);
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "default_branch"), summary.defaultBranch, 256u);
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "html_url"), summary.htmlUrl, 512u);
        if (auto* const privVal = yyjson_obj_get(item, "private"); privVal && yyjson_is_bool(privVal))
            summary.isPrivate = yyjson_get_bool(privVal);

        if (! IsValidGitHubTokenSegment(summary.owner, 128u) || ! IsValidGitHubTokenSegment(summary.repo, 128u))
            continue;
        if (summary.fullName.empty())
            summary.fullName = summary.owner + L"/" + summary.repo;
        if (summary.defaultBranch.empty())
            summary.defaultBranch = L"main";
        inOutRepos.push_back(std::move(summary));
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubDefaultBranchJson(std::string_view jsonBytes, std::wstring& outBranch) noexcept
try
{
    outBranch.clear();
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (! DecodeUtf8Sanitized(yyjson_obj_get(root, "default_branch"), outBranch, 256u) || outBranch.empty())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubTreeJson(
    std::string_view jsonBytes, std::wstring_view owner, std::wstring_view repo, std::wstring_view branch, GitHubTreeSnapshot& outSnapshot) noexcept
try
{
    outSnapshot        = {};
    outSnapshot.owner  = SanitizeUiText(owner, 128u);
    outSnapshot.repo   = SanitizeUiText(repo, 128u);
    outSnapshot.branch = SanitizeUiText(branch, 256u);

    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return outSnapshot.status = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return outSnapshot.status = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    if (auto* const truncVal = yyjson_obj_get(root, "truncated"); truncVal && yyjson_is_bool(truncVal))
        outSnapshot.truncated = yyjson_get_bool(truncVal);

    auto* const treeArr = yyjson_obj_get(root, "tree");
    if (! treeArr || ! yyjson_is_arr(treeArr))
        return outSnapshot.status = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    struct RawBlob
    {
        std::wstring path;
        std::wstring sha;
    };
    std::vector<RawBlob> blobs;
    std::size_t examined{};
    std::size_t idx{}, maxIdx{};
    yyjson_val* item{};
    yyjson_arr_foreach(treeArr, idx, maxIdx, item)
    {
        if (examined >= MaximumGitHubTreeEntries)
        {
            outSnapshot.limitReached = true;
            break;
        }
        ++examined;
        if (! item || ! yyjson_is_obj(item))
            continue;
        std::wstring type, relPath, sha;
        if (! DecodeUtf8Sanitized(yyjson_obj_get(item, "type"), type, 32u) || type != L"blob")
            continue;
        if (! DecodeUtf8Sanitized(yyjson_obj_get(item, "path"), relPath, 1024u) || ! ValidRelativeRepoPath(relPath))
            continue;
        const std::filesystem::path pseudo(relPath);
        if (! IsMarkdownFile(pseudo))
            continue;
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "sha"), sha, 64u);
        blobs.push_back({std::move(relPath), std::move(sha)});
    }

    // Build deterministic directory hierarchy (directories before files, natural order).
    struct Node
    {
        std::wstring path;
        std::wstring name;
        std::wstring sha;
        unsigned depth{};
        bool directory{};
        std::size_t markdownCount{};
        std::vector<std::size_t> children;
    };
    std::vector<Node> nodes;
    nodes.push_back(Node{}); // Synthetic root at index 0.

    auto ensureDirectory = [&](std::wstring_view dirPath, unsigned depth) -> std::size_t
    {
        std::size_t parentIdx = 0u;
        std::size_t start{};
        unsigned curDepth{};
        while (start < dirPath.size())
        {
            const auto slash = dirPath.find(L'/', start);
            const auto seg   = slash == std::wstring_view::npos ? dirPath.substr(start) : dirPath.substr(start, slash - start);
            const auto full  = slash == std::wstring_view::npos ? dirPath : dirPath.substr(0u, slash);
            std::optional<std::size_t> found;
            for (const std::size_t c : nodes[parentIdx].children)
            {
                if (nodes[c].directory && EqualOrdinalIgnoreCase(nodes[c].name, seg))
                {
                    found = c;
                    break;
                }
            }
            if (! found)
            {
                const std::size_t created = nodes.size();
                nodes.push_back({std::wstring(full), std::wstring(seg), {}, curDepth, true, 0u, {}});
                nodes[parentIdx].children.push_back(created);
                found = created;
            }
            parentIdx = *found;
            ++curDepth;
            if (slash == std::wstring_view::npos || curDepth > depth)
                break;
            start = slash + 1u;
        }
        return parentIdx;
    };

    for (const auto& blob : blobs)
    {
        const auto lastSlash = blob.path.find_last_of(L'/');
        std::size_t parent   = 0u;
        unsigned depth       = 0u;
        std::wstring name    = blob.path;
        if (lastSlash != std::wstring::npos)
        {
            const std::wstring_view dirPart = std::wstring_view(blob.path).substr(0u, lastSlash);
            depth                           = static_cast<unsigned>(std::ranges::count(dirPart, L'/') + 1);
            if (depth > MaximumGitHubTreeDepth)
                continue;
            parent = ensureDirectory(dirPart, depth);
            name   = blob.path.substr(lastSlash + 1u);
        }
        const std::size_t created = nodes.size();
        nodes.push_back({blob.path, std::move(name), blob.sha, depth, false, 1u, {}});
        nodes[parent].children.push_back(created);
        ++outSnapshot.markdownCount;
    }

    auto emit = [&](auto& self, std::size_t nodeIdx, std::size_t outParent) -> void
    {
        auto& children = nodes[nodeIdx].children;
        std::ranges::sort(children,
                          [&](std::size_t left, std::size_t right)
        {
            if (nodes[left].directory != nodes[right].directory)
                return nodes[left].directory;
            return FilesNaturalLess(nodes[left].name, nodes[right].name);
        });
        for (const std::size_t childIdx : children)
        {
            const std::size_t outIdx = outSnapshot.entries.size();
            const auto& n            = nodes[childIdx];
            GitHubTreeEntry entry;
            entry.path      = n.path;
            entry.name      = n.name;
            entry.sha       = n.sha;
            entry.parent    = outParent;
            entry.depth     = n.depth;
            entry.directory = n.directory;
            entry.htmlUrl   = std::format(L"https://github.com/{}/{}/{}/{}/{}",
                                          outSnapshot.owner,
                                          outSnapshot.repo,
                                          n.directory ? L"tree" : L"blob",
                                          outSnapshot.branch.empty() ? L"main" : outSnapshot.branch,
                                          n.path);
            outSnapshot.entries.push_back(std::move(entry));
            if (n.directory)
            {
                const std::size_t before = outSnapshot.entries.size();
                self(self, childIdx, outIdx);
                std::size_t mdTotal{};
                for (std::size_t k = before; k < outSnapshot.entries.size(); ++k)
                    if (! outSnapshot.entries[k].directory)
                        ++mdTotal;
                outSnapshot.entries[outIdx].markdownCount = mdTotal;
            }
            else
            {
                outSnapshot.entries[outIdx].markdownCount = 1u;
            }
        }
    };
    emit(emit, 0u, NoFilesParent);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubDeviceCodeJson(std::string_view jsonBytes, GitHubDeviceCodeInfo& outInfo) noexcept
try
{
    outInfo = {};
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (! DecodeUtf8Sanitized(yyjson_obj_get(root, "device_code"), outInfo.deviceCode, 256u) ||
        ! DecodeUtf8Sanitized(yyjson_obj_get(root, "user_code"), outInfo.userCode, 64u) ||
        ! DecodeUtf8Sanitized(yyjson_obj_get(root, "verification_uri"), outInfo.verificationUri, 512u) || outInfo.deviceCode.empty() ||
        outInfo.userCode.empty() || outInfo.verificationUri.empty())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (auto* const exp = yyjson_obj_get(root, "expires_in"); exp && yyjson_is_uint(exp))
        outInfo.expiresInSeconds = static_cast<unsigned>(std::clamp<std::uint64_t>(yyjson_get_uint(exp), 60u, 3600u));
    if (auto* const iv = yyjson_obj_get(root, "interval"); iv && yyjson_is_uint(iv))
        outInfo.intervalSeconds = static_cast<unsigned>(std::clamp<std::uint64_t>(yyjson_get_uint(iv), 5u, 60u));
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseGitHubDevicePollJson(std::string_view jsonBytes, GitHubDevicePollResult& outResult) noexcept
try
{
    outResult = {};
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(jsonBytes.data()), jsonBytes.size(), 0, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    std::wstring token;
    if (DecodeUtf8Sanitized(yyjson_obj_get(root, "access_token"), token, 4096u) && ! token.empty())
    {
        outResult.state       = GitHubDevicePollResult::State::Authorized;
        outResult.accessToken = std::move(token);
        return S_OK;
    }
    std::wstring errorStr;
    (void)DecodeUtf8Sanitized(yyjson_obj_get(root, "error"), errorStr, 128u);
    (void)DecodeUtf8Sanitized(yyjson_obj_get(root, "error_description"), outResult.errorMessage, 256u);
    if (auto* const iv = yyjson_obj_get(root, "interval"); iv && yyjson_is_uint(iv))
        outResult.intervalSeconds = static_cast<unsigned>(std::clamp<std::uint64_t>(yyjson_get_uint(iv), 5u, 60u));
    if (errorStr == L"authorization_pending")
        outResult.state = GitHubDevicePollResult::State::Pending;
    else if (errorStr == L"slow_down")
        outResult.state = GitHubDevicePollResult::State::SlowDown;
    else if (errorStr == L"access_denied")
        outResult.state = GitHubDevicePollResult::State::AccessDenied;
    else if (errorStr == L"expired_token")
        outResult.state = GitHubDevicePollResult::State::Expired;
    else
        outResult.state = GitHubDevicePollResult::State::Error;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::wstring FormatGitHubErrorMessage(const GitHubHttpResponse& response)
{
    if (response.rateLimited)
        return L"GitHub API rate limit reached. Sign in with a GitHub account or retry shortly.";
    if (response.httpStatusCode == 401u)
        return L"GitHub authentication expired or invalid. Sign in again to access this repository.";
    if (response.httpStatusCode == 404u)
        return L"GitHub repository, branch, or file was not found (or requires sign-in).";
    if (! response.errorMessage.empty())
        return response.errorMessage;
    if (FAILED(response.transportStatus))
        return ErrorText(response.transportStatus);
    return std::format(L"GitHub request failed (HTTP {}).", response.httpStatusCode);
}

HRESULT TryImportGhCliToken(std::wstring& outToken) noexcept
try
{
    ScrubWideString(outToken);
    wil::unique_handle readPipe, writePipe;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    if (! CreatePipe(readPipe.put(), writePipe.put(), &sa, 0))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)SetHandleInformation(readPipe.get(), HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb            = sizeof(si);
    si.dwFlags       = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow   = SW_HIDE;
    si.hStdOutput    = writePipe.get();
    si.hStdError     = writePipe.get();
    std::wstring cmd = L"gh.exe auth token";
    PROCESS_INFORMATION pi{};
    if (! CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_FILE_NOT_FOUND);
    }
    wil::unique_handle proc(pi.hProcess);
    wil::unique_handle thread(pi.hThread);
    writePipe.reset();

    const DWORD waited = WaitForSingleObject(proc.get(), 3000u);
    if (waited != WAIT_OBJECT_0)
    {
        (void)TerminateProcess(proc.get(), 1u);
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    }
    DWORD exitCode = 1u;
    if (! GetExitCodeProcess(proc.get(), &exitCode) || exitCode != 0u)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    std::string raw;
    std::array<char, 512> buf{};
    DWORD readBytes{};
    while (ReadFile(readPipe.get(), buf.data(), static_cast<DWORD>(buf.size()), &readBytes, nullptr) && readBytes > 0u)
    {
        if (raw.size() + readBytes > 4096u)
            break;
        raw.append(buf.data(), readBytes);
    }
    SecureZeroMemory(buf.data(), buf.size());
    const auto scrubRaw = wil::scope_exit([&]() noexcept
    {
        if (! raw.empty())
            SecureZeroMemory(raw.data(), raw.size());
    });
    while (! raw.empty() && (raw.back() == '\r' || raw.back() == '\n' || raw.back() == ' '))
        raw.pop_back();
    if (raw.empty())
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    std::wstring wide;
    if (DxUi::RichText::TryDecodeUtf8(raw, wide) != DxUi::RichText::SourceResult::Success)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    outToken = SanitizeUiText(wide, 4096u);
    ScrubWideString(wide);
    return outToken.empty() ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
} // namespace RedTilde
