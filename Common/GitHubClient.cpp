#include "GitHubClient.h"
#include "CredentialStore.h"
#include "JsonHelpers.h"
#include "PathSecurity.h"
#include "ProcessWork.h"
#include "RemoteImageImports.h"
#include "Settings.h"
#include "TextUtilities.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <format>
#include <shlobj.h>
#include <string>
#include <wil/resource.h>
#include <winhttp.h>
#include <yyjson.h>

namespace RedTilde
{
namespace
{
using Internet                   = wil::unique_any<HINTERNET, decltype(&WinHttpCloseHandle), WinHttpCloseHandle>;
constexpr unsigned TimeoutConnMs = 8000u;
constexpr unsigned TimeoutReadMs = 15000u;
constexpr DWORD GhCliTimeoutMs   = 3000u;

[[nodiscard]] HRESULT LastErrorHr() noexcept
{
    const DWORD err = GetLastError();
    return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
}

[[nodiscard]] bool IsUnsafeGitHubControlOrBidi(wchar_t ch) noexcept
{
    const auto code = static_cast<unsigned>(ch);
    return code < 0x20u || code == 0x7Fu || (code >= 0x80u && code <= 0x9Fu) || (code >= 0x200Bu && code <= 0x200Fu) || (code >= 0x202Au && code <= 0x202Eu) ||
           (code >= 0x2066u && code <= 0x2069u) || code == 0xFEFFu;
}

[[nodiscard]] bool IsUnicodeWhitespace(wchar_t ch) noexcept
{
    const auto code = static_cast<unsigned>(ch);
    return code == 0x00A0u || code == 0x1680u || (code >= 0x2000u && code <= 0x200Au) || code == 0x2028u || code == 0x2029u || code == 0x202Fu ||
           code == 0x205Fu || code == 0x3000u;
}

[[nodiscard]] bool IsUnsafePathControlOrBidi(wchar_t ch) noexcept
{
    const auto code = static_cast<unsigned>(ch);
    // Allow ZWJ (U+200D) and ZWNJ (U+200C) in repository paths while rejecting controls and bidi overrides.
    return code < 0x20u || code == 0x7Fu || (code >= 0x80u && code <= 0x9Fu) || code == 0x200Bu || code == 0x200Eu || code == 0x200Fu ||
           (code >= 0x202Au && code <= 0x202Eu) || (code >= 0x2066u && code <= 0x2069u) || code == 0xFEFFu;
}

[[nodiscard]] bool IsValidAsciiHeaderValue(std::wstring_view value, std::size_t maxLen, bool allowSpaces) noexcept
{
    if (value.size() > maxLen || ! DxUi::RichText::IsValidUtf16(value))
        return false;
    for (const wchar_t ch : value)
    {
        const auto code = static_cast<unsigned>(ch);
        if (allowSpaces)
        {
            if (code < 0x20u || code >= 0x7Fu)
                return false;
        }
        else
        {
            if (code <= 0x20u || code >= 0x7Fu)
                return false;
        }
    }
    return true;
}

[[nodiscard]] bool AllowedGitHubHost(std::wstring_view host) noexcept
{
    return host == L"api.github.com" || host == L"github.com";
}

[[nodiscard]] bool AllowedRelativeRequestPath(std::wstring_view path) noexcept
{
    if (path.empty() || path.front() != L'/' || path.size() > 2048u || ! DxUi::RichText::IsValidUtf16(path))
        return false;
    for (const wchar_t ch : path)
    {
        const auto code = static_cast<unsigned>(ch);
        if (code <= 0x20u || code >= 0x7Fu || ch == L'#' || ch == L'\\')
            return false;
    }
    return true;
}

[[nodiscard]] bool IsSafeGitHubWebUrl(std::wstring_view url) noexcept
{
    if (! StartsWithAsciiOrdinalIgnoreCase(url, L"https://github.com/") || url.size() > 1024u || ! DxUi::RichText::IsValidUtf16(url))
        return false;
    for (const wchar_t ch : url)
    {
        if (static_cast<unsigned>(ch) <= 0x20u || ch == L'\\' || ch == L'@' || ch == L'"' || IsUnsafeGitHubControlOrBidi(ch) || IsUnicodeWhitespace(ch))
            return false;
    }
    return true;
}

[[nodiscard]] bool IsSafeGitHubRepoWebUrl(std::wstring_view url, std::wstring_view expectedFullName) noexcept
{
    if (! IsSafeGitHubWebUrl(url))
        return false;
    constexpr std::wstring_view hostPrefix = L"https://github.com/";
    const std::wstring_view afterHost      = url.substr(hostPrefix.size());
    if (! StartsWithOrdinalIgnoreCase(afterHost, expectedFullName))
        return false;
    if (afterHost.size() == expectedFullName.size())
        return true;
    const wchar_t next = afterHost[expectedFullName.size()];
    return next == L'/' || next == L'?' || next == L'#';
}

[[nodiscard]] int HexDigitValue(wchar_t ch) noexcept
{
    if (ch >= L'0' && ch <= L'9')
        return ch - L'0';
    if (ch >= L'a' && ch <= L'f')
        return 10 + (ch - L'a');
    if (ch >= L'A' && ch <= L'F')
        return 10 + (ch - L'A');
    return -1;
}

[[nodiscard]] GitHubErrorCode TryPercentDecodeUtf8Segment(std::wstring_view rawSeg, std::wstring& outDecoded)
{
    outDecoded.clear();
    if (rawSeg.empty())
        return GitHubErrorCode::UrlTooLongOrInvalidEncoding;
    std::string utf8;
    utf8.reserve(rawSeg.size() * 3u);
    for (std::size_t i = 0u; i < rawSeg.size(); ++i)
    {
        const wchar_t ch = rawSeg[i];
        if (ch == L'%')
        {
            if (i + 2u >= rawSeg.size())
                return GitHubErrorCode::UrlTooLongOrInvalidEncoding;
            const int hi = HexDigitValue(rawSeg[i + 1u]);
            const int lo = HexDigitValue(rawSeg[i + 2u]);
            if (hi < 0 || lo < 0)
                return GitHubErrorCode::UrlTooLongOrInvalidEncoding;
            const auto byte = static_cast<unsigned char>((hi << 4) | lo);
            if (byte == 0u || byte < 0x20u || byte == 0x7Fu)
                return GitHubErrorCode::UrlUnsafeCharacters;
            if (byte == '/' || byte == '\\')
                return GitHubErrorCode::UrlUnsafeCharacters;
            utf8.push_back(static_cast<char>(byte));
            i += 2u;
        }
        else
        {
            std::string encodedChar;
            if (DxUi::RichText::TryEncodeUtf8(std::wstring_view(&ch, 1u), encodedChar) != DxUi::RichText::SourceResult::Success)
                return GitHubErrorCode::UrlTooLongOrInvalidEncoding;
            utf8.append(encodedChar);
        }
    }
    if (DxUi::RichText::TryDecodeUtf8(utf8, outDecoded) != DxUi::RichText::SourceResult::Success || ! DxUi::RichText::IsValidUtf16(outDecoded))
        return GitHubErrorCode::UrlTooLongOrInvalidEncoding;
    for (const wchar_t ch : outDecoded)
    {
        if (ch == L'\0' || ch == L'/' || ch == L'\\' || IsUnsafePathControlOrBidi(ch))
            return GitHubErrorCode::UrlUnsafeCharacters;
    }
    if (outDecoded == L"." || outDecoded == L"..")
        return GitHubErrorCode::DotPathSegment;
    return GitHubErrorCode::None;
}

[[nodiscard]] std::wstring AsciiToWide(std::string_view ascii)
{
    std::wstring out;
    out.reserve(ascii.size());
    for (const char ch : ascii)
        out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
    return out;
}

bool DecodeUtf8BoundedExact(yyjson_val* val, std::wstring& out, std::size_t maxLen)
{
    ScrubWideString(out);
    if (! val || yyjson_is_null(val))
        return true;
    if (! yyjson_is_str(val))
        return false;
    const std::string_view u8(yyjson_get_str(val), yyjson_get_len(val));
    if (u8.size() > maxLen * 4u)
        return false;
    std::wstring decoded;
    const auto scrubDecoded = wil::scope_exit([&]() noexcept { ScrubWideString(decoded); });
    if (DxUi::RichText::TryDecodeUtf8(u8, decoded) != DxUi::RichText::SourceResult::Success || ! DxUi::RichText::IsValidUtf16(decoded))
        return false;
    if (decoded.size() > maxLen)
        return false;
    out.assign(decoded);
    return true;
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

// Reads an optional response header up to maxBytes UTF-16 bytes; missing or over-cap metadata returns S_OK with empty outValue.
[[nodiscard]] HRESULT QueryOptionalHeader(HINTERNET request, const wchar_t* headerName, std::wstring& outValue, DWORD maxBytes = 32768u)
{
    outValue.clear();
    std::array<wchar_t, 512> stackBuf{};
    DWORD byteLen = static_cast<DWORD>(stackBuf.size() * sizeof(wchar_t));
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, headerName, stackBuf.data(), &byteLen, WINHTTP_NO_HEADER_INDEX))
    {
        std::size_t chars = std::min<std::size_t>(byteLen / sizeof(wchar_t), stackBuf.size());
        while (chars > 0u && stackBuf[chars - 1u] == L'\0')
            --chars;
        outValue.assign(stackBuf.data(), chars);
        return S_OK;
    }
    DWORD err = GetLastError();
    if (err == ERROR_WINHTTP_HEADER_NOT_FOUND)
        return S_OK;
    if (err != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);

    DWORD neededBytes = 0u;
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, headerName, nullptr, &neededBytes, WINHTTP_NO_HEADER_INDEX))
        return S_OK;
    err = GetLastError();
    if (err == ERROR_WINHTTP_HEADER_NOT_FOUND)
        return S_OK;
    if (err != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    if (neededBytes == 0u || neededBytes > maxBytes || neededBytes % sizeof(wchar_t) != 0u)
        return S_OK;

    outValue.assign(static_cast<std::size_t>(neededBytes) / sizeof(wchar_t), L'\0');
    DWORD actualBytes = neededBytes;
    if (! WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, headerName, outValue.data(), &actualBytes, WINHTTP_NO_HEADER_INDEX))
    {
        outValue.clear();
        err = GetLastError();
        if (err == ERROR_WINHTTP_HEADER_NOT_FOUND || err == ERROR_INSUFFICIENT_BUFFER)
            return S_OK;
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    const std::size_t actualChars = std::min<std::size_t>(actualBytes / sizeof(wchar_t), outValue.size());
    outValue.resize(actualChars);
    while (! outValue.empty() && outValue.back() == L'\0')
        outValue.pop_back();
    return S_OK;
}

std::wstring TrimWhitespace(std::wstring_view text)
{
    while (! text.empty() && (text.front() == L' ' || text.front() == L'\t' || text.front() == L'\r' || text.front() == L'\n'))
        text.remove_prefix(1u);
    while (! text.empty() && (text.back() == L' ' || text.back() == L'\t' || text.back() == L'\r' || text.back() == L'\n'))
        text.remove_suffix(1u);
    return std::wstring(text);
}

[[nodiscard]] bool IsHexUpperDigit(unsigned char c) noexcept
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct GitHubTransfer final
{
    // Declared first so final native callback retirement releases transfer state before the process lease.
    ProcessWorkLease lease;
    std::shared_ptr<WinHttpGitHubTransport::SessionHolder> sessionLifetime;
    wil::unique_handle ready;
    std::string requestBody;
    std::array<char, 32768> buffer{};
    std::atomic<DWORD> notification{}, error{}, received{};
    std::shared_ptr<GitHubTransfer> callbackLifetime;

    ~GitHubTransfer() noexcept
    {
        ScrubString(requestBody);
    }

    static void CALLBACK Notify(HINTERNET, DWORD_PTR context, DWORD status, void* information, DWORD length) noexcept
    {
        if (! context)
            return;
        auto& transfer = *reinterpret_cast<GitHubTransfer*>(context);
        if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
        {
            const auto lifetime = std::move(transfer.callbackLifetime);
            return;
        }
        if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
        {
            const auto* failure = static_cast<const WINHTTP_ASYNC_RESULT*>(information);
            transfer.error.store(failure && length == sizeof(*failure) ? failure->dwError : ERROR_GEN_FAILURE, std::memory_order_relaxed);
        }
        else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE)
        {
            transfer.received.store(length, std::memory_order_relaxed);
        }
        else if (status != WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE && status != WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)
        {
            return;
        }
        transfer.notification.store(status, std::memory_order_release);
        (void)SetEvent(transfer.ready.get());
    }

    void Begin() noexcept
    {
        (void)ResetEvent(ready.get());
        notification.store(0u, std::memory_order_release);
        error.store(0u, std::memory_order_relaxed);
        received.store(0u, std::memory_order_relaxed);
    }

    [[nodiscard]] HRESULT Await(DWORD expected, ULONGLONG deadline, const std::atomic_bool* cancellation, HANDLE abortEvent) noexcept
    {
        const std::array<HANDLE, 2> handles{ready.get(), abortEvent};
        for (;;)
        {
            if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            const ULONGLONG now = GetTickCount64();
            if (now >= deadline)
                return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            const DWORD interval = static_cast<DWORD>(std::min<ULONGLONG>(50u, deadline - now));
            const DWORD wait     = WaitForMultipleObjects(abortEvent ? 2u : 1u, handles.data(), FALSE, interval);
            if (abortEvent && wait == WAIT_OBJECT_0 + 1u)
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            if (wait == WAIT_FAILED)
                return LastErrorHr();
            if (wait == WAIT_TIMEOUT)
                continue;
            const DWORD status = notification.load(std::memory_order_acquire);
            if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
            {
                const DWORD failure = error.load(std::memory_order_relaxed);
                return HRESULT_FROM_WIN32(failure != 0u ? failure : ERROR_GEN_FAILURE);
            }
            return status == expected ? S_OK : E_UNEXPECTED;
        }
    }
};
#pragma warning(pop)

[[nodiscard]] HRESULT RunGhCliTokenProcess(const std::filesystem::path& executablePath,
                                           std::wstring_view arguments,
                                           std::wstring& outToken,
                                           DWORD timeoutMs,
                                           const std::atomic_bool* cancellation,
                                           HANDLE abortEvent) noexcept
try
{
    ScrubWideString(outToken);
    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (executablePath.empty() || ! executablePath.is_absolute())
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    const std::wstring& exeNative = executablePath.native();
    if (exeNative.find(L'"') != std::wstring::npos || std::ranges::any_of(exeNative, [](wchar_t ch) noexcept { return IsUnsafeGitHubControlOrBidi(ch); }))
        return E_INVALIDARG;
    if (FAILED(VerifyPathNoSurrogateReparse(executablePath, {.allowMissingTarget = false, .allowMissingAncestors = false, .expectDirectory = false})))
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    static std::atomic<std::uint64_t> pipeSeq{0u};
    const std::wstring pipeName = std::format(L"\\\\.\\pipe\\RedTilde.gh.{}.{}", GetCurrentProcessId(), pipeSeq.fetch_add(1u, std::memory_order_relaxed));
    wil::unique_handle readPipe(CreateNamedPipeW(pipeName.c_str(),
                                                 PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                                 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                                 1u,
                                                 8192u,
                                                 8192u,
                                                 0u,
                                                 nullptr));
    if (! readPipe)
        return LastErrorHr();

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    wil::unique_handle writePipe(CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0u, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (! writePipe)
        return LastErrorHr();

    wil::unique_handle stdinNul(
        CreateFileW(L"\\\\.\\NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    wil::unique_handle stderrNul(
        CreateFileW(L"\\\\.\\NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (! stdinNul || ! stderrNul)
        return LastErrorHr();

    SIZE_T attrSize{};
    (void)InitializeProcThreadAttributeList(nullptr, 1u, 0u, &attrSize);
    if (attrSize == 0u)
        return LastErrorHr();
    std::vector<std::byte> attrStorage(attrSize);
    auto* const attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrStorage.data());
    if (! InitializeProcThreadAttributeList(attrList, 1u, 0u, &attrSize))
        return LastErrorHr();
    const auto freeAttrList = wil::scope_exit([attrList]() noexcept { DeleteProcThreadAttributeList(attrList); });

    std::array<HANDLE, 3> inheritedHandles{stdinNul.get(), writePipe.get(), stderrNul.get()};
    if (! UpdateProcThreadAttribute(attrList, 0u, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritedHandles.data(), sizeof(inheritedHandles), nullptr, nullptr))
        return LastErrorHr();

    wil::unique_handle job(CreateJobObjectW(nullptr, nullptr));
    if (! job)
        return LastErrorHr();
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits{};
    jobLimits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (! SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &jobLimits, sizeof(jobLimits)))
        return LastErrorHr();

    STARTUPINFOEXW siEx{};
    siEx.StartupInfo.cb          = sizeof(siEx);
    siEx.StartupInfo.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    siEx.StartupInfo.wShowWindow = SW_HIDE;
    siEx.StartupInfo.hStdInput   = stdinNul.get();
    siEx.StartupInfo.hStdOutput  = writePipe.get();
    siEx.StartupInfo.hStdError   = stderrNul.get();
    siEx.lpAttributeList         = attrList;

    std::wstring cmdLine = arguments.empty() ? std::format(L"\"{}\"", exeNative) : std::format(L"\"{}\" {}", exeNative, arguments);
    PROCESS_INFORMATION pi{};
    const std::wstring workDir = executablePath.parent_path().native();
    if (! CreateProcessW(executablePath.c_str(),
                         cmdLine.data(),
                         nullptr,
                         nullptr,
                         TRUE,
                         CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
                         nullptr,
                         workDir.empty() ? nullptr : workDir.c_str(),
                         &siEx.StartupInfo,
                         &pi))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_FILE_NOT_FOUND);
    }
    wil::unique_handle proc(pi.hProcess);
    wil::unique_handle thread(pi.hThread);

    if (! AssignProcessToJobObject(job.get(), proc.get()))
    {
        const HRESULT hr = LastErrorHr();
        (void)TerminateProcess(proc.get(), 1u);
        return hr;
    }

    writePipe.reset();
    stdinNul.reset();
    stderrNul.reset();

    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1))
    {
        const HRESULT hr = LastErrorHr();
        (void)TerminateJobObject(job.get(), 1u);
        return hr;
    }
    thread.reset();

    wil::unique_event ioEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! ioEvent)
    {
        (void)TerminateJobObject(job.get(), 1u);
        return LastErrorHr();
    }

    const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(std::max<DWORD>(timeoutMs, 100u));
    std::string raw;
    const auto scrubRaw = wil::scope_exit([&]() noexcept { ScrubString(raw); });

    std::array<char, 4096> buf{};
    const auto scrubBuf = wil::scope_exit([&]() noexcept { SecureZeroMemory(buf.data(), buf.size()); });

    OVERLAPPED ov{};
    ov.hEvent                  = ioEvent.get();
    bool ioPending             = false;
    const auto cancelPendingIo = wil::scope_exit([&]() noexcept
    {
        if (ioPending)
        {
            (void)CancelIoEx(readPipe.get(), &ov);
            DWORD dummy{};
            (void)GetOverlappedResult(readPipe.get(), &ov, &dummy, TRUE);
            ioPending = false;
        }
    });

    for (;;)
    {
        if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        if (GetTickCount64() >= deadline)
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        }

        (void)ResetEvent(ioEvent.get());
        ov        = {};
        ov.hEvent = ioEvent.get();
        DWORD readBytes{};
        if (ReadFile(readPipe.get(), buf.data(), static_cast<DWORD>(buf.size()), &readBytes, &ov))
        {
            if (readBytes > 0u)
            {
                if (raw.size() + readBytes > 65536u)
                {
                    (void)TerminateJobObject(job.get(), 1u);
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                raw.append(buf.data(), readBytes);
            }
            continue;
        }
        const DWORD readErr = GetLastError();
        if (readErr == ERROR_BROKEN_PIPE || readErr == ERROR_HANDLE_EOF)
            break;
        if (readErr != ERROR_IO_PENDING)
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(readErr);
        }
        ioPending = true;
        const std::array<HANDLE, 2> waits{ioEvent.get(), abortEvent};
        bool finishedRead{};
        while (! finishedRead)
        {
            if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
            {
                (void)CancelIoEx(readPipe.get(), &ov);
                DWORD dummy{};
                (void)GetOverlappedResult(readPipe.get(), &ov, &dummy, TRUE);
                ioPending = false;
                (void)TerminateJobObject(job.get(), 1u);
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            const ULONGLONG now = GetTickCount64();
            if (now >= deadline)
            {
                (void)CancelIoEx(readPipe.get(), &ov);
                DWORD dummy{};
                (void)GetOverlappedResult(readPipe.get(), &ov, &dummy, TRUE);
                ioPending = false;
                (void)TerminateJobObject(job.get(), 1u);
                return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }
            const DWORD slice  = static_cast<DWORD>(std::min<ULONGLONG>(cancellation ? 50u : (deadline - now), deadline - now));
            const DWORD waited = WaitForMultipleObjects(abortEvent ? 2u : 1u, waits.data(), FALSE, slice);
            if (waited == WAIT_TIMEOUT)
                continue;
            if (abortEvent && waited == WAIT_OBJECT_0 + 1u)
            {
                (void)CancelIoEx(readPipe.get(), &ov);
                DWORD dummy{};
                (void)GetOverlappedResult(readPipe.get(), &ov, &dummy, TRUE);
                ioPending = false;
                (void)TerminateJobObject(job.get(), 1u);
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            if (waited != WAIT_OBJECT_0)
            {
                const HRESULT hr = LastErrorHr();
                (void)CancelIoEx(readPipe.get(), &ov);
                DWORD dummy{};
                (void)GetOverlappedResult(readPipe.get(), &ov, &dummy, TRUE);
                ioPending = false;
                (void)TerminateJobObject(job.get(), 1u);
                return hr;
            }
            finishedRead = true;
        }
        ioPending = false;
        if (! GetOverlappedResult(readPipe.get(), &ov, &readBytes, FALSE))
        {
            const DWORD ovErr = GetLastError();
            if (ovErr == ERROR_BROKEN_PIPE || ovErr == ERROR_HANDLE_EOF)
                break;
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ovErr);
        }
        if (readBytes > 0u)
        {
            if (raw.size() + readBytes > 65536u)
            {
                (void)TerminateJobObject(job.get(), 1u);
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            raw.append(buf.data(), readBytes);
        }
    }

    for (;;)
    {
        if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        const ULONGLONG now = GetTickCount64();
        if (now >= deadline)
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        }
        const DWORD slice = static_cast<DWORD>(std::min<ULONGLONG>(cancellation ? 50u : (deadline - now), deadline - now));
        const std::array<HANDLE, 2> waits{proc.get(), abortEvent};
        const DWORD waited = WaitForMultipleObjects(abortEvent ? 2u : 1u, waits.data(), FALSE, slice);
        if (waited == WAIT_TIMEOUT)
            continue;
        if (abortEvent && waited == WAIT_OBJECT_0 + 1u)
        {
            (void)TerminateJobObject(job.get(), 1u);
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        if (waited != WAIT_OBJECT_0)
        {
            (void)TerminateJobObject(job.get(), 1u);
            return LastErrorHr();
        }
        break;
    }

    DWORD exitCode = 1u;
    if (! GetExitCodeProcess(proc.get(), &exitCode) || exitCode != 0u)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (raw.find('\0') != std::string::npos)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    std::size_t trimStart = 0u;
    while (trimStart < raw.size() && (raw[trimStart] == ' ' || raw[trimStart] == '\t' || raw[trimStart] == '\r' || raw[trimStart] == '\n'))
        ++trimStart;
    std::size_t trimEnd = raw.size();
    while (trimEnd > trimStart && (raw[trimEnd - 1u] == ' ' || raw[trimEnd - 1u] == '\t' || raw[trimEnd - 1u] == '\r' || raw[trimEnd - 1u] == '\n'))
        --trimEnd;
    if (trimStart >= trimEnd)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    const std::string_view tokenBytes(raw.data() + trimStart, trimEnd - trimStart);
    if (tokenBytes.size() > 4096u)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    for (const char ch : tokenBytes)
    {
        const auto code = static_cast<unsigned char>(ch);
        if (code <= 0x20u || code == 0x7Fu)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    std::wstring wide;
    const auto scrubWide = wil::scope_exit([&]() noexcept { ScrubWideString(wide); });
    if (DxUi::RichText::TryDecodeUtf8(tokenBytes, wide) != DxUi::RichText::SourceResult::Success || ! DxUi::RichText::IsValidUtf16(wide) || wide.empty() ||
        wide.size() > 4096u)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    if (std::ranges::any_of(wide, [](wchar_t ch) noexcept { return static_cast<unsigned>(ch) <= 0x20u || IsUnsafeGitHubControlOrBidi(ch); }))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    outToken.assign(wide);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
} // namespace

#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct WinHttpGitHubTransport::SessionHolder final
{
    Internet session;
    bool loopbackMode{};
};
#pragma warning(pop)

WinHttpGitHubTransport::WinHttpGitHubTransport() noexcept = default;
WinHttpGitHubTransport::~WinHttpGitHubTransport()         = default;

#if defined(ENABLE_TESTS)
void WinHttpGitHubTransport::SetLoopbackEndpointForTest(std::optional<GitHubLoopbackEndpoint> endpoint) noexcept
{
    std::lock_guard lock(_sessionMutex);
    _loopbackForTest = std::move(endpoint);
    _sessionHolder.reset();
}
#endif

bool IsValidGitHubRelativePath(std::wstring_view path) noexcept
{
    if (path.empty() || path.size() > 1024u || path.front() == L'/' || path.back() == L'/' || ! DxUi::RichText::IsValidUtf16(path))
        return false;
    if (path.find(L'\\') != std::wstring_view::npos || path.find(L'\0') != std::wstring_view::npos)
        return false;
    for (const wchar_t ch : path)
    {
        if (IsUnsafePathControlOrBidi(ch))
            return false;
    }
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

std::string PercentEncodeUrlPathSegments(std::wstring_view utf16Path)
{
    std::string utf8;
    if (DxUi::RichText::TryEncodeUtf8(utf16Path, utf8) != DxUi::RichText::SourceResult::Success)
        return {};
    std::string encoded;
    encoded.reserve(utf8.size());
    for (const char ch : utf8)
    {
        const auto c = static_cast<unsigned char>(ch);
        if (IsHexUpperDigit(c) || c == '/')
            encoded.push_back(static_cast<char>(c));
        else
            encoded += std::format("%{:02X}", static_cast<unsigned>(c));
    }
    return encoded;
}

std::string PercentEncodeQueryComponent(std::wstring_view utf16Value)
{
    std::string utf8;
    if (DxUi::RichText::TryEncodeUtf8(utf16Value, utf8) != DxUi::RichText::SourceResult::Success)
        return {};
    std::string encoded;
    encoded.reserve(utf8.size());
    for (const char ch : utf8)
    {
        const auto c = static_cast<unsigned char>(ch);
        if (IsHexUpperDigit(c))
            encoded.push_back(static_cast<char>(c));
        else
            encoded += std::format("%{:02X}", static_cast<unsigned>(c));
    }
    return encoded;
}

ParsedGitHubTarget ParseGitHubUrl(std::wstring_view input)
{
    ParsedGitHubTarget parsed;
    std::wstring trimmed = TrimWhitespace(input);
    if (trimmed.empty())
    {
        parsed.errorCode = GitHubErrorCode::EmptyUrl;
        return parsed;
    }
    if (trimmed.size() > 1024u || ! DxUi::RichText::IsValidUtf16(trimmed))
    {
        parsed.errorCode = GitHubErrorCode::UrlTooLongOrInvalidEncoding;
        return parsed;
    }
    for (const wchar_t ch : trimmed)
    {
        if (ch < L' ' || ch == L'\\' || ch == L'@' || ch == L'?' || ch == L'#' || IsUnsafeGitHubControlOrBidi(ch))
        {
            parsed.errorCode = GitHubErrorCode::UrlUnsafeCharacters;
            return parsed;
        }
    }

    bool isHttpsUrl{};
    std::wstring_view rest = trimmed;
    if (StartsWithAsciiOrdinalIgnoreCase(rest, L"http://"))
    {
        parsed.errorCode = GitHubErrorCode::HttpsRequired;
        return parsed;
    }
    if (StartsWithAsciiOrdinalIgnoreCase(rest, L"https://"))
    {
        isHttpsUrl = true;
        rest.remove_prefix(8u);
        if (StartsWithAsciiOrdinalIgnoreCase(rest, L"www.github.com/"))
            rest.remove_prefix(15u);
        else if (StartsWithAsciiOrdinalIgnoreCase(rest, L"github.com/"))
            rest.remove_prefix(11u);
        else
        {
            parsed.errorCode = GitHubErrorCode::HostNotGitHub;
            return parsed;
        }
    }
    else if (rest.find(L"://") != std::wstring_view::npos)
    {
        parsed.errorCode = GitHubErrorCode::UnsupportedScheme;
        return parsed;
    }

    std::vector<std::wstring_view> rawSegments;
    std::size_t pos{};
    while (pos <= rest.size())
    {
        const auto slash = rest.find(L'/', pos);
        const auto seg   = slash == std::wstring_view::npos ? rest.substr(pos) : rest.substr(pos, slash - pos);
        rawSegments.push_back(seg);
        if (slash == std::wstring_view::npos)
            break;
        pos = slash + 1u;
    }
    if (rawSegments.size() == 3u && rawSegments[2].empty())
        rawSegments.pop_back();
    if (rawSegments.size() < 2u)
    {
        parsed.errorCode = GitHubErrorCode::MissingOwnerOrRepo;
        return parsed;
    }

    std::vector<std::wstring> parts;
    parts.reserve(rawSegments.size());
    for (std::size_t i = 0u; i < rawSegments.size(); ++i)
    {
        const std::wstring_view seg = rawSegments[i];
        if (seg.empty())
        {
            if (i < 2u)
                parsed.errorCode = GitHubErrorCode::InvalidOwnerOrRepo;
            else if (i == 2u)
                parsed.errorCode = GitHubErrorCode::UnsupportedUrlPath;
            else if (EqualOrdinalIgnoreCase(parts[2], L"tree"))
                parsed.errorCode = GitHubErrorCode::InvalidBranch;
            else if (EqualOrdinalIgnoreCase(parts[2], L"blob"))
                parsed.errorCode = (i == 3u) ? GitHubErrorCode::InvalidBranch : GitHubErrorCode::InvalidFilePath;
            else
                parsed.errorCode = GitHubErrorCode::UnsupportedUrlPath;
            return parsed;
        }
        if (seg == L"." || seg == L"..")
        {
            parsed.errorCode = GitHubErrorCode::DotPathSegment;
            return parsed;
        }
        if (isHttpsUrl)
        {
            std::wstring decoded;
            const GitHubErrorCode decodeErr = TryPercentDecodeUtf8Segment(seg, decoded);
            if (decodeErr != GitHubErrorCode::None)
            {
                parsed.errorCode = decodeErr;
                return parsed;
            }
            parts.push_back(std::move(decoded));
        }
        else
        {
            parts.emplace_back(seg);
        }
    }

    std::wstring owner = std::move(parts[0]);
    std::wstring repo  = std::move(parts[1]);
    if (repo.size() > 4u && EndsWithOrdinalIgnoreCase(repo, L".git"))
        repo.resize(repo.size() - 4u);

    if (! IsValidGitHubTokenSegment(owner, 128u) || ! IsValidGitHubTokenSegment(repo, 128u))
    {
        parsed.errorCode = GitHubErrorCode::InvalidOwnerOrRepo;
        return parsed;
    }

    parsed.owner = std::move(owner);
    parsed.repo  = std::move(repo);
    if (parts.size() >= 4u && EqualOrdinalIgnoreCase(parts[2], L"tree"))
    {
        std::wstring branch;
        for (std::size_t i = 3u; i < parts.size(); ++i)
        {
            if (! branch.empty())
                branch.push_back(L'/');
            branch.append(parts[i]);
        }
        if (! IsValidGitHubBranchName(branch))
        {
            parsed.errorCode = GitHubErrorCode::InvalidBranch;
            return parsed;
        }
        parsed.branch = std::move(branch);
    }
    else if (parts.size() >= 4u && EqualOrdinalIgnoreCase(parts[2], L"blob"))
    {
        if (parts.size() < 5u)
        {
            parsed.errorCode = GitHubErrorCode::UnsupportedUrlPath;
            return parsed;
        }
        if (! IsValidGitHubBranchName(parts[3]))
        {
            parsed.errorCode = GitHubErrorCode::InvalidBranch;
            return parsed;
        }
        parsed.branch = parts[3];
        std::wstring rel;
        for (std::size_t i = 4u; i < parts.size(); ++i)
        {
            if (! rel.empty())
                rel.push_back(L'/');
            rel.append(parts[i]);
        }
        if (! IsValidGitHubRelativePath(rel))
        {
            parsed.errorCode = GitHubErrorCode::InvalidFilePath;
            return parsed;
        }
        parsed.filePath = std::move(rel);
    }
    else if (parts.size() > 2u)
    {
        parsed.errorCode = GitHubErrorCode::UnsupportedUrlPath;
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
        return LessOrdinalIgnoreCase(repos[a.index].fullName, repos[b.index].fullName);
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
    outResponse         = {};
    const auto failWith = [&](HRESULT hr) noexcept -> HRESULT
    {
        outResponse.transportStatus = hr;
        outResponse.errorCode       = ClassifyGitHubHttpError(outResponse);
        return hr;
    };

    std::wstring connectHost;
    INTERNET_PORT connectPort = INTERNET_DEFAULT_HTTPS_PORT;
    bool useHttps             = true;
    bool useLoopback          = false;
#if defined(ENABLE_TESTS)
    std::optional<GitHubLoopbackEndpoint> loopback;
    {
        std::lock_guard lock(_sessionMutex);
        loopback = _loopbackForTest;
    }
    if (loopback)
    {
        if (! AllowedGitHubHost(request.host) && ! EqualOrdinalIgnoreCase(request.host, loopback->host))
            return failWith(E_INVALIDARG);
        connectHost = loopback->host;
        connectPort = loopback->port;
        useHttps    = loopback->useHttps;
        useLoopback = true;
    }
    else
#endif
    {
        if (! AllowedGitHubHost(request.host))
            return failWith(E_INVALIDARG);
        connectHost = request.host;
    }

    if ((request.method != L"GET" && request.method != L"POST") || ! AllowedRelativeRequestPath(request.path) ||
        ! IsValidAsciiHeaderValue(request.acceptHeader, 256u, true) || request.acceptHeader.empty() ||
        (! request.bearerToken.empty() && ! IsValidAsciiHeaderValue(request.bearerToken, 4096u, false)) || request.body.size() > 65536u ||
        request.maximumResponseBytes == 0u || request.maximumResponseBytes > MaximumGitHubTreeJsonBytes)
        return failWith(E_INVALIDARG);

    std::wstring contentTypeWide;
    if (! request.contentType.empty())
    {
        if (request.contentType.size() > 256u || DxUi::RichText::TryDecodeUtf8(request.contentType, contentTypeWide) != DxUi::RichText::SourceResult::Success ||
            ! IsValidAsciiHeaderValue(contentTypeWide, 256u, true))
            return failWith(E_INVALIDARG);
    }

    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0u) == WAIT_OBJECT_0))
        return failWith(HRESULT_FROM_WIN32(ERROR_CANCELLED));

    const HRESULT importHr = EnsureWinHttpImports();
    if (FAILED(importHr))
        return failWith(importHr);

    std::shared_ptr<SessionHolder> sessionHolder;
    {
        std::lock_guard lock(_sessionMutex);
        if (! _sessionHolder || ! _sessionHolder->session || _sessionHolder->loopbackMode != useLoopback)
        {
            auto created          = std::make_shared<SessionHolder>();
            created->loopbackMode = useLoopback;
            created->session.reset(WinHttpOpen(L"RedTilde/1.0",
                                               useLoopback ? WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                               WINHTTP_NO_PROXY_NAME,
                                               WINHTTP_NO_PROXY_BYPASS,
                                               WINHTTP_FLAG_ASYNC));
            if (! created->session)
                return failWith(LastErrorHr());
            if (! WinHttpSetTimeouts(created->session.get(), TimeoutConnMs, TimeoutConnMs, TimeoutConnMs, TimeoutReadMs))
                return failWith(LastErrorHr());
            DWORD retries = 1u;
            if (! WinHttpSetOption(created->session.get(), WINHTTP_OPTION_CONNECT_RETRIES, &retries, sizeof(retries)))
                return failWith(LastErrorHr());
            DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
            if (! WinHttpSetOption(created->session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols)))
                return failWith(LastErrorHr());
            _sessionHolder = std::move(created);
        }
        sessionHolder = _sessionHolder;
    }

    const Internet connection(WinHttpConnect(sessionHolder->session.get(), connectHost.c_str(), connectPort, 0));
    if (! connection)
        return failWith(LastErrorHr());

    LPCWSTR acceptTypes[]{request.acceptHeader.c_str(), nullptr};
    const Internet req(WinHttpOpenRequest(
        connection.get(), request.method.c_str(), request.path.c_str(), nullptr, WINHTTP_NO_REFERER, acceptTypes, useHttps ? WINHTTP_FLAG_SECURE : 0u));
    if (! req)
        return failWith(LastErrorHr());

    DWORD disabled       = WINHTTP_DISABLE_AUTHENTICATION | WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_REDIRECTS;
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD maxHeaderBytes = 32768u;
    DWORD autologon      = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    if (! WinHttpSetOption(req.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) ||
        ! WinHttpSetOption(req.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy)) ||
        ! WinHttpSetOption(req.get(), WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE, &maxHeaderBytes, sizeof(maxHeaderBytes)) ||
        ! WinHttpSetOption(req.get(), WINHTTP_OPTION_AUTOLOGON_POLICY, &autologon, sizeof(autologon)))
        return failWith(LastErrorHr());

    auto transfer            = std::make_shared<GitHubTransfer>();
    const HRESULT admittedHr = ProcessWork::Acquire(transfer->lease);
    if (FAILED(admittedHr))
        return failWith(admittedHr);
    transfer->sessionLifetime = sessionHolder;
    transfer->ready.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! transfer->ready)
        return failWith(LastErrorHr());

    DWORD_PTR context = reinterpret_cast<DWORD_PTR>(transfer.get());
    if (! WinHttpSetOption(req.get(), WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context)))
        return failWith(LastErrorHr());
    transfer->callbackLifetime = transfer;
    if (WinHttpSetStatusCallback(req.get(),
                                 &GitHubTransfer::Notify,
                                 WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE | WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE | WINHTTP_CALLBACK_FLAG_READ_COMPLETE |
                                     WINHTTP_CALLBACK_FLAG_REQUEST_ERROR | WINHTTP_CALLBACK_FLAG_HANDLES,
                                 0u) == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        transfer->callbackLifetime.reset();
        return failWith(LastErrorHr());
    }

    std::wstring headers;
    const auto scrubHeaders = wil::scope_exit([&]() noexcept { ScrubWideString(headers); });
    headers.reserve(96u + contentTypeWide.size() + request.bearerToken.size());
    headers.append(L"X-GitHub-Api-Version: 2022-11-28\r\n");
    if (! contentTypeWide.empty())
    {
        headers.append(L"Content-Type: ");
        headers.append(contentTypeWide);
        headers.append(L"\r\n");
    }
    if (! request.bearerToken.empty())
    {
        headers.append(L"Authorization: Bearer ");
        headers.append(request.bearerToken);
        headers.append(L"\r\n");
    }

    LPVOID bodyPtr = WINHTTP_NO_REQUEST_DATA;
    DWORD bodySz   = 0u;
    if (! request.body.empty())
    {
        transfer->requestBody.assign(request.body);
        bodyPtr = transfer->requestBody.data();
        bodySz  = static_cast<DWORD>(transfer->requestBody.size());
    }

    const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(TimeoutConnMs) + static_cast<ULONGLONG>(TimeoutReadMs);
    transfer->Begin();
    if (! WinHttpSendRequest(req.get(), headers.c_str(), static_cast<DWORD>(headers.size()), bodyPtr, bodySz, bodySz, context))
        return failWith(LastErrorHr());
    const HRESULT sendHr = transfer->Await(WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, deadline, cancellation, abortEvent);
    if (FAILED(sendHr))
        return failWith(sendHr);

    transfer->Begin();
    if (! WinHttpReceiveResponse(req.get(), nullptr))
        return failWith(LastErrorHr());
    const HRESULT recvHr = transfer->Await(WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, deadline, cancellation, abortEvent);
    if (FAILED(recvHr))
        return failWith(recvHr);

    DWORD statusCode{};
    DWORD statusSize = sizeof(statusCode);
    if (! WinHttpQueryHeaders(
            req.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX))
        return failWith(LastErrorHr());
    outResponse.httpStatusCode = statusCode;

    std::wstring linkHeader;
    const HRESULT linkHr = QueryOptionalHeader(req.get(), L"Link", linkHeader);
    if (FAILED(linkHr))
        return failWith(linkHr);
    outResponse.hasNextPage = ! linkHeader.empty() && ContainsOrdinalIgnoreCase(linkHeader, L"rel=\"next\"");

    if (statusCode == 429u)
    {
        outResponse.rateLimited = true;
    }
    else if (statusCode == 403u)
    {
        std::wstring remainHeader;
        const HRESULT remHr = QueryOptionalHeader(req.get(), L"X-RateLimit-Remaining", remainHeader);
        if (FAILED(remHr))
            return failWith(remHr);
        if (remainHeader == L"0")
            outResponse.rateLimited = true;
    }

    std::string body;
    for (;;)
    {
        transfer->Begin();
        if (! WinHttpReadData(req.get(), transfer->buffer.data(), static_cast<DWORD>(transfer->buffer.size()), nullptr))
            return failWith(LastErrorHr());
        const HRESULT readHr = transfer->Await(WINHTTP_CALLBACK_STATUS_READ_COMPLETE, deadline, cancellation, abortEvent);
        if (FAILED(readHr))
            return failWith(readHr);
        const DWORD count = transfer->received.load(std::memory_order_relaxed);
        if (count == 0u)
            break;
        if (count > transfer->buffer.size())
            return failWith(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        if (count > request.maximumResponseBytes - body.size())
            return failWith(HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE));
        body.append(transfer->buffer.data(), count);
    }

    outResponse.body = std::move(body);
    if (statusCode >= 400u && ! outResponse.body.empty())
    {
        const unique_yyjson_doc doc = ParseJsonDocument(outResponse.body);
        if (doc)
        {
            if (auto* const msg = yyjson_obj_get(yyjson_doc_get_root(doc.get()), "message"))
                (void)DecodeUtf8Sanitized(msg, outResponse.errorMessage, 256u);
        }
    }
    if (statusCode >= 400u)
        outResponse.errorCode = ClassifyGitHubHttpError(outResponse);
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
    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    std::wstring login;
    if (! DecodeUtf8BoundedExact(yyjson_obj_get(root, "login"), login, 128u) || ! IsValidGitHubTokenSegment(login, 128u))
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
    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
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
        auto* const ownerObj = yyjson_obj_get(item, "owner");
        if (! ownerObj || ! yyjson_is_obj(ownerObj))
            continue;
        GitHubRepoSummary summary;
        if (! DecodeUtf8BoundedExact(yyjson_obj_get(ownerObj, "login"), summary.owner, 128u) ||
            ! DecodeUtf8BoundedExact(yyjson_obj_get(item, "name"), summary.repo, 128u) ||
            ! DecodeUtf8BoundedExact(yyjson_obj_get(item, "full_name"), summary.fullName, 256u) ||
            ! DecodeUtf8BoundedExact(yyjson_obj_get(item, "default_branch"), summary.defaultBranch, 256u) ||
            ! DecodeUtf8BoundedExact(yyjson_obj_get(item, "html_url"), summary.htmlUrl, 512u))
            continue;
        (void)DecodeUtf8Sanitized(yyjson_obj_get(item, "description"), summary.description, 256u);
        if (auto* const privVal = yyjson_obj_get(item, "private"); privVal && yyjson_is_bool(privVal))
            summary.isPrivate = yyjson_get_bool(privVal);

        if (! IsValidGitHubTokenSegment(summary.owner, 128u) || ! IsValidGitHubTokenSegment(summary.repo, 128u))
            continue;
        const std::wstring expectedFullName = summary.owner + L"/" + summary.repo;
        if (! summary.fullName.empty() && ! EqualOrdinalIgnoreCase(summary.fullName, expectedFullName))
            continue;
        summary.fullName = expectedFullName;
        if (summary.defaultBranch.empty())
            summary.defaultBranch = L"main";
        else if (! IsValidGitHubBranchName(summary.defaultBranch))
            continue;
        if (summary.htmlUrl.empty())
            summary.htmlUrl = L"https://github.com/" + expectedFullName;
        else if (! IsSafeGitHubRepoWebUrl(summary.htmlUrl, expectedFullName))
            continue;
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
    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    std::wstring branch;
    if (! DecodeUtf8BoundedExact(yyjson_obj_get(root, "default_branch"), branch, 256u) || ! IsValidGitHubBranchName(branch))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    outBranch = std::move(branch);
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
    outSnapshot = {};
    if (! IsValidGitHubTokenSegment(owner, 128u) || ! IsValidGitHubTokenSegment(repo, 128u) || (! branch.empty() && ! IsValidGitHubBranchName(branch)))
        return outSnapshot.status = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    outSnapshot.owner  = std::wstring(owner);
    outSnapshot.repo   = std::wstring(repo);
    outSnapshot.branch = std::wstring(branch);

    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return outSnapshot.status = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
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
        if (! DecodeUtf8BoundedExact(yyjson_obj_get(item, "type"), type, 32u) || type != L"blob")
            continue;
        // Preserve ZWJ (U+200D) and ZWNJ (U+200C) in entry.path for exact GitHub API retrieval.
        if (! DecodeUtf8BoundedExact(yyjson_obj_get(item, "path"), relPath, 1024u) || ! IsValidGitHubRelativePath(relPath))
            continue;
        const std::filesystem::path pseudo(relPath);
        if (! IsMarkdownFile(pseudo))
            continue;
        if (! DecodeUtf8BoundedExact(yyjson_obj_get(item, "sha"), sha, 64u))
            continue;
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

    const std::wstring encodedBranch = AsciiToWide(PercentEncodeUrlPathSegments(outSnapshot.branch.empty() ? L"main" : outSnapshot.branch));
    auto emit                        = [&](auto& self, std::size_t nodeIdx, std::size_t outParent) -> void
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
            entry.name      = SanitizeUiText(n.name);
            entry.sha       = n.sha;
            entry.parent    = outParent;
            entry.depth     = n.depth;
            entry.directory = n.directory;
            entry.htmlUrl   = std::format(L"https://github.com/{}/{}/{}/{}/{}",
                                          outSnapshot.owner,
                                          outSnapshot.repo,
                                          n.directory ? L"tree" : L"blob",
                                          encodedBranch,
                                          AsciiToWide(PercentEncodeUrlPathSegments(n.path)));
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
    ScrubWideString(outInfo.deviceCode);
    outInfo                     = {};
    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    const auto hasUnsafeCodeChar = [](std::wstring_view text) noexcept
    {
        return std::ranges::any_of(
            text, [](wchar_t ch) noexcept { return static_cast<unsigned>(ch) <= 0x20u || IsUnsafeGitHubControlOrBidi(ch) || IsUnicodeWhitespace(ch); });
    };
    if (! DecodeUtf8BoundedExact(yyjson_obj_get(root, "device_code"), outInfo.deviceCode, 256u) ||
        ! DecodeUtf8BoundedExact(yyjson_obj_get(root, "user_code"), outInfo.userCode, 64u) ||
        ! DecodeUtf8BoundedExact(yyjson_obj_get(root, "verification_uri"), outInfo.verificationUri, 512u) || outInfo.deviceCode.empty() ||
        outInfo.userCode.empty() || hasUnsafeCodeChar(outInfo.deviceCode) || hasUnsafeCodeChar(outInfo.userCode) ||
        ! IsSafeGitHubWebUrl(outInfo.verificationUri))
    {
        ScrubWideString(outInfo.deviceCode);
        outInfo = {};
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
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
    ScrubWideString(outResult.accessToken);
    outResult                   = {};
    const unique_yyjson_doc doc = ParseJsonDocument(jsonBytes);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    auto* const root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    std::wstring token;
    const auto scrubToken = wil::scope_exit([&]() noexcept { ScrubWideString(token); });
    if (auto* const tokenVal = yyjson_obj_get(root, "access_token"); tokenVal && ! yyjson_is_null(tokenVal))
    {
        if (! DecodeUtf8BoundedExact(tokenVal, token, 4096u))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        if (! token.empty())
        {
            if (! IsValidAsciiHeaderValue(token, 4096u, false))
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            outResult.state = GitHubDevicePollResult::State::Authorized;
            outResult.accessToken.assign(token);
            return S_OK;
        }
    }
    std::wstring errorStr;
    if (! DecodeUtf8BoundedExact(yyjson_obj_get(root, "error"), errorStr, 128u))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    (void)DecodeUtf8Sanitized(yyjson_obj_get(root, "error_description"), outResult.errorMessage, 256u);
    if (auto* const iv = yyjson_obj_get(root, "interval"); iv && yyjson_is_uint(iv))
        outResult.serverIntervalSeconds = static_cast<unsigned>(std::clamp<std::uint64_t>(yyjson_get_uint(iv), 5u, 120u));
    if (errorStr == L"authorization_pending")
        outResult.state = GitHubDevicePollResult::State::Pending;
    else if (errorStr == L"slow_down")
        outResult.state = GitHubDevicePollResult::State::SlowDown;
    else if (errorStr == L"access_denied")
        outResult.state = GitHubDevicePollResult::State::AccessDenied;
    else if (errorStr == L"expired_token")
        outResult.state = GitHubDevicePollResult::State::Expired;
    else if (! errorStr.empty())
        outResult.state = GitHubDevicePollResult::State::Error;
    else
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

GitHubErrorCode ClassifyGitHubHttpError(const GitHubHttpResponse& response) noexcept
{
    if (response.rateLimited)
        return GitHubErrorCode::RateLimited;
    if (response.httpStatusCode == 401u)
        return GitHubErrorCode::Unauthorized;
    if (response.httpStatusCode == 404u)
        return GitHubErrorCode::NotFound;
    if (! response.errorMessage.empty() && response.httpStatusCode >= 400u)
        return GitHubErrorCode::ServerMessage;
    if (FAILED(response.transportStatus))
    {
        if (response.transportStatus == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return GitHubErrorCode::Cancelled;
        if (response.transportStatus == HRESULT_FROM_WIN32(ERROR_TIMEOUT) || response.transportStatus == HRESULT_FROM_WIN32(ERROR_WINHTTP_TIMEOUT))
            return GitHubErrorCode::Timeout;
        if (response.transportStatus == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
            return GitHubErrorCode::FileTooLarge;
        return GitHubErrorCode::TransportFailed;
    }
    if (response.httpStatusCode >= 400u)
        return GitHubErrorCode::HttpStatus;
    return GitHubErrorCode::None;
}

HRESULT TryImportGhCliToken(std::wstring& outToken, const std::atomic_bool* cancellation, HANDLE abortEvent) noexcept
try
{
    ScrubWideString(outToken);
    constexpr std::array<const KNOWNFOLDERID*, 2> programFolders{&FOLDERID_ProgramFiles, &FOLDERID_ProgramFilesX86};
    std::filesystem::path resolvedExe;
    for (const KNOWNFOLDERID* folderId : programFolders)
    {
        wil::unique_cotaskmem_string baseDir;
        if (SUCCEEDED(SHGetKnownFolderPath(*folderId, 0u, nullptr, &baseDir)) && baseDir)
        {
            const std::filesystem::path candidate = std::filesystem::path(baseDir.get()) / L"GitHub CLI" / L"gh.exe";
            if (SUCCEEDED(VerifyPathNoSurrogateReparse(candidate, {.allowMissingTarget = false, .allowMissingAncestors = false, .expectDirectory = false})))
            {
                resolvedExe = candidate;
                break;
            }
        }
    }
    if (resolvedExe.empty())
    {
        wil::unique_cotaskmem_string localAppData;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0u, nullptr, &localAppData)) && localAppData)
        {
            const std::filesystem::path candidate = std::filesystem::path(localAppData.get()) / L"Programs" / L"GitHub CLI" / L"gh.exe";
            if (SUCCEEDED(VerifyPathNoSurrogateReparse(candidate, {.allowMissingTarget = false, .allowMissingAncestors = false, .expectDirectory = false})))
                resolvedExe = candidate;
        }
    }
    if (resolvedExe.empty())
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    return RunGhCliTokenProcess(resolvedExe, L"auth token", outToken, GhCliTimeoutMs, cancellation, abortEvent);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

#if defined(ENABLE_TESTS)
HRESULT TryImportGhCliTokenWithExecutableForTest(const std::filesystem::path& executablePath,
                                                 std::wstring_view arguments,
                                                 std::wstring& outToken,
                                                 DWORD timeoutMs,
                                                 const std::atomic_bool* cancellation,
                                                 HANDLE abortEvent) noexcept
{
    return RunGhCliTokenProcess(executablePath, arguments, outToken, timeoutMs, cancellation, abortEvent);
}
#endif
} // namespace RedTilde
