#include "RemoteImage.h"
#include "RemoteImageImports.h"
#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <wil/resource.h>
#include <winhttp.h>

namespace RedTilde
{
namespace
{
using Internet = wil::unique_any<HINTERNET, decltype(&WinHttpCloseHandle), WinHttpCloseHandle>;
constexpr auto Cancelled = HRESULT_FROM_WIN32(ERROR_CANCELLED);
constexpr auto TimedOut = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
HRESULT LastError() noexcept
{
    const auto value = GetLastError();
    return HRESULT_FROM_WIN32(value ? value : ERROR_GEN_FAILURE);
}

HRESULT EnsureWinHttpImports() noexcept
{
    HRESULT result = E_UNEXPECTED;
    // A delayed Windows API import can fail with SEH, outside the transport's
    // C++ exception boundary. This function has no unwind-requiring locals and
    // handles only loader/procedure failure; every other exception propagates.
    // delayimp retains imports for process lifetime, including late callbacks.
    __try
    {
        return __HrLoadAllImportsForDll("WINHTTP.dll");
    }
    __except (RemoteImageDetail::WinHttpImportFailure(GetExceptionInformation(), result))
    {
        return result;
    }
}
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct Transfer final
{
    wil::unique_handle ready;
    std::array<std::byte, 65536> buffer{};
    std::atomic<DWORD> notification{}, error{}, received{};
    // WinHTTP can notify after WinHttpCloseHandle returns. Its documented final
    // HANDLE_CLOSING notification releases this one callback-owned reference;
    // neither the UI nor the image worker waits for callback retirement.
    // Ownership: learn.microsoft.com/windows/win32/api/winhttp/nf-winhttp-winhttpclosehandle
    std::shared_ptr<Transfer> callbackLifetime;
    static void CALLBACK Notify(HINTERNET, DWORD_PTR context, DWORD status, void *information, DWORD length) noexcept
    {
        if (!context)
            return;
        auto &transfer = *reinterpret_cast<Transfer *>(context);
        if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
        {
            const auto lifetime = std::move(transfer.callbackLifetime);
            return;
        }
        if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
        {
            const auto *failure = static_cast<const WINHTTP_ASYNC_RESULT *>(information);
            transfer.error.store(failure && length == sizeof(*failure) ? failure->dwError : ERROR_GEN_FAILURE, std::memory_order_relaxed);
        }
        else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE)
            transfer.received.store(length, std::memory_order_relaxed);
        else if (status != WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE && status != WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)
            return;
        transfer.notification.store(status, std::memory_order_release);
        (void)SetEvent(transfer.ready.get());
    }
    void Begin() noexcept
    {
        (void)ResetEvent(ready.get());
        notification.store(0, std::memory_order_release);
        error.store(0, std::memory_order_relaxed);
        received.store(0, std::memory_order_relaxed);
    }
    HRESULT Await(DWORD expected, ULONGLONG deadline, const std::atomic_bool *cancellation, HANDLE abortEvent) noexcept
    {
        const std::array<HANDLE, 2> handles{ready.get(), abortEvent};
        for (;;)
        {
            if (cancellation && cancellation->load(std::memory_order_relaxed))
                return Cancelled;
            const auto now = GetTickCount64();
            if (now >= deadline)
                return TimedOut;
            // There is no idle polling: this bounded active transfer waits on
            // WinHTTP and the owner-close event. Atomic request cancellation is
            // borrowed from DxUi, so recheck it within 50 ms while I/O is live.
            const auto interval = static_cast<DWORD>(std::min<ULONGLONG>(50, deadline - now));
            const auto wait = WaitForMultipleObjects(abortEvent ? 2 : 1, handles.data(), FALSE, interval);
            if (abortEvent && wait == WAIT_OBJECT_0 + 1)
                return Cancelled;
            if (wait == WAIT_FAILED)
                return LastError();
            if (wait == WAIT_TIMEOUT)
                continue;
            const auto status = notification.load(std::memory_order_acquire);
            if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
            {
                const auto failure = error.load(std::memory_order_relaxed);
                return HRESULT_FROM_WIN32(failure ? failure : ERROR_GEN_FAILURE);
            }
            return status == expected ? S_OK : E_UNEXPECTED;
        }
    }
};
#pragma warning(pop)
HRESULT ContentLength(HINTERNET request, std::size_t maximum, std::optional<std::size_t> &output) noexcept
{
    std::array<wchar_t, 32> text{};
    DWORD size = static_cast<DWORD>(sizeof(text));
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, text.data(), &size, WINHTTP_NO_HEADER_INDEX))
        return GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND ? S_OK : LastError();
    std::size_t value{};
    const std::wstring_view digits(text.data(), size / sizeof(wchar_t));
    if (digits.empty())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    for (const auto digit : digits)
    {
        if (digit < L'0' || digit > L'9')
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        const auto part = static_cast<std::size_t>(digit - L'0');
        if (value > maximum / 10 || (value == maximum / 10 && part > maximum % 10))
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        value = value * 10 + part;
    }
    output = value;
    return S_OK;
}
bool HttpSource(std::wstring_view source) noexcept
{
    // Transport validation is intentionally stricter than generic Markdown URI
    // admission: reject credentials, fragments, controls and ambiguous escapes
    // before WinHTTP sees an untrusted request spelling.
    if (source.empty() || source.size() > 8192 || source.find_first_of(L"\\#\r\n\t") != source.npos ||
        std::any_of(source.begin(), source.end(), [](wchar_t c) { return c <= 32 || c == 127; }))
        return false;
    const auto scheme = source.find(L"://");
    if (scheme == source.npos)
        return false;
    const auto name = source.substr(0, scheme);
    if (!((name.size() == 4 && CompareStringOrdinal(name.data(), 4, L"http", 4, TRUE) == CSTR_EQUAL) ||
          (name.size() == 5 && CompareStringOrdinal(name.data(), 5, L"https", 5, TRUE) == CSTR_EQUAL)))
        return false;
    const auto end = source.find_first_of(L"/?", scheme + 3);
    const auto authority = source.substr(scheme + 3, end == source.npos ? source.size() - scheme - 3 : end - scheme - 3);
    if (authority.empty() || authority.find_first_of(L"@%") != authority.npos ||
        std::any_of(authority.begin(), authority.end(), [](wchar_t c) { return c >= 128; }))
        return false;
    const auto hex = [](wchar_t c) { return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'); };
    for (std::size_t index = scheme + 3; index < source.size(); ++index)
        if (source[index] == L'%')
        {
            if (index + 2 >= source.size() || !hex(source[index + 1]) || !hex(source[index + 2]))
                return false;
            index += 2;
        }
    return true;
}
} // namespace

HRESULT FetchRemoteImage(std::wstring_view source, std::vector<std::byte> &output, const std::atomic_bool *cancellation, HANDLE abortEvent,
                         RemoteImageLimits limits) noexcept
try
{
    if (!HttpSource(source) || !limits.timeoutMilliseconds || limits.timeoutMilliseconds > 8000 || !limits.maximumEncodedBytes ||
        limits.maximumEncodedBytes > 16777216)
        return E_INVALIDARG;
    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0) == WAIT_OBJECT_0))
        return Cancelled;
    const auto deadline = GetTickCount64() + limits.timeoutMilliseconds;
    const auto imports = EnsureWinHttpImports();
    if (FAILED(imports))
        return imports;
    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0) == WAIT_OBJECT_0))
        return Cancelled;
    if (GetTickCount64() >= deadline)
        return TimedOut;
    std::wstring url(source);
    URL_COMPONENTS parts{sizeof(parts)};
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = parts.dwUserNameLength = parts.dwPasswordLength =
        static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts))
        return LastError();
    if ((parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) || !parts.dwHostNameLength || parts.dwUserNameLength ||
        parts.dwPasswordLength || !parts.nPort)
        return E_INVALIDARG;
    std::wstring host(parts.lpszHostName, parts.dwHostNameLength), target;
    if (parts.dwUrlPathLength)
        target.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    else
        target = L"/";
    if (parts.dwExtraInfoLength)
        target.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    Internet session(WinHttpOpen(L"RedTilde/1", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC));
    if (!session)
        return LastError();
    const auto phaseTimeout = static_cast<int>(std::min(2000u, limits.timeoutMilliseconds));
    if (!WinHttpSetTimeouts(session.get(), phaseTimeout, phaseTimeout, phaseTimeout, phaseTimeout))
        return LastError();
    DWORD attempts = 1, protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    if (!WinHttpSetOption(session.get(), WINHTTP_OPTION_CONNECT_RETRIES, &attempts, sizeof(attempts)) ||
        !WinHttpSetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols)))
        return LastError();
    Internet connection(WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0));
    if (!connection)
        return LastError();
    const wchar_t *accept[] = {L"image/png", L"image/jpeg", L"image/gif", L"image/bmp", L"image/tiff", nullptr};
    Internet request(WinHttpOpenRequest(connection.get(), L"GET", target.c_str(), nullptr, WINHTTP_NO_REFERER, accept,
                                        parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0));
    if (!request)
        return LastError();
    DWORD disabled = WINHTTP_DISABLE_AUTHENTICATION | WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_REDIRECTS | WINHTTP_DISABLE_KEEP_ALIVE;
    DWORD noRedirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER, headerBytes = 32768, logon = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) ||
        !WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &noRedirect, sizeof(noRedirect)) ||
        !WinHttpSetOption(request.get(), WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE, &headerBytes, sizeof(headerBytes)) ||
        !WinHttpSetOption(request.get(), WINHTTP_OPTION_AUTOLOGON_POLICY, &logon, sizeof(logon)))
        return LastError();
    // WinHTTP requires the application to supply a client certificate after
    // CLIENT_AUTH_CERT_NEEDED. We never supply one or retry that challenge.
    auto transfer = std::make_shared<Transfer>();
    transfer->ready.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!transfer->ready)
        return LastError();
    DWORD_PTR context = reinterpret_cast<DWORD_PTR>(transfer.get());
    if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context)))
        return LastError();
    transfer->callbackLifetime = transfer;
    if (WinHttpSetStatusCallback(request.get(), &Transfer::Notify,
                                 WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE | WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE |
                                     WINHTTP_CALLBACK_FLAG_READ_COMPLETE | WINHTTP_CALLBACK_FLAG_REQUEST_ERROR | WINHTTP_CALLBACK_FLAG_HANDLES,
                                 0) == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        transfer->callbackLifetime.reset();
        return LastError();
    }
    transfer->Begin();
    if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, context))
        return LastError();
    auto result = transfer->Await(WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, deadline, cancellation, abortEvent);
    if (FAILED(result))
        return result;
    transfer->Begin();
    if (!WinHttpReceiveResponse(request.get(), nullptr))
        return LastError();
    result = transfer->Await(WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, deadline, cancellation, abortEvent);
    if (FAILED(result))
        return result;
    DWORD status{}, statusBytes = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status,
                             &statusBytes, WINHTTP_NO_HEADER_INDEX))
        return LastError();
    if (status != HTTP_STATUS_OK)
        return HRESULT_FROM_WIN32(ERROR_WINHTTP_INVALID_SERVER_RESPONSE);
    std::array<wchar_t, 32> encoding{};
    DWORD encodingBytes = static_cast<DWORD>(sizeof(encoding));
    if (WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_ENCODING, WINHTTP_HEADER_NAME_BY_INDEX, encoding.data(), &encodingBytes,
                            WINHTTP_NO_HEADER_INDEX))
    {
        constexpr std::wstring_view identity = L"identity";
        if (encodingBytes != identity.size() * sizeof(wchar_t) ||
            CompareStringOrdinal(encoding.data(), static_cast<int>(identity.size()), identity.data(), static_cast<int>(identity.size()), TRUE) !=
                CSTR_EQUAL)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    else if (GetLastError() != ERROR_WINHTTP_HEADER_NOT_FOUND)
        return LastError();
    std::optional<std::size_t> declared;
    result = ContentLength(request.get(), limits.maximumEncodedBytes, declared);
    if (FAILED(result))
        return result;
    std::vector<std::byte> encoded;
    if (declared)
        encoded.reserve(declared.value());
    for (;;)
    {
        transfer->Begin();
        if (!WinHttpReadData(request.get(), transfer->buffer.data(), static_cast<DWORD>(transfer->buffer.size()), nullptr))
            return LastError();
        result = transfer->Await(WINHTTP_CALLBACK_STATUS_READ_COMPLETE, deadline, cancellation, abortEvent);
        if (FAILED(result))
            return result;
        const auto count = transfer->received.load(std::memory_order_relaxed);
        if (!count)
            break;
        if (count > transfer->buffer.size())
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        if (count > limits.maximumEncodedBytes - encoded.size())
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        encoded.insert(encoded.end(), transfer->buffer.begin(), transfer->buffer.begin() + count);
    }
    if (encoded.empty() || (declared && declared.value() != encoded.size()))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if ((cancellation && cancellation->load(std::memory_order_relaxed)) || (abortEvent && WaitForSingleObject(abortEvent, 0) == WAIT_OBJECT_0))
        return Cancelled;
    output.swap(encoded);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    // Allocation failure at a worker/service boundary is fatal, never a silent
    // failed-image preference or a partially published encoded buffer.
    std::terminate();
}
catch (const std::length_error &)
{
    return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
}
} // namespace RedTilde
