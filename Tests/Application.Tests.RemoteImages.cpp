// Winsock must precede windows.h and WIL's conditional socket declarations.
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
// clang-format on
#include "Application.Tests.RemoteImages.h"
#include "Common/RemoteImage.h"
#include "Common/RemoteImageImports.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include <DxUi/RichImage.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <format>
// MSVC's external future/PPL templates intentionally use the selected ARM64 volatile model.
// Owner: RedTilde test maintainers; review on the next MSVC upgrade, by 2027-01-31.
#pragma warning(push)
#if defined(_M_ARM64)
#pragma warning(disable : 4746)
#endif
#include <future>
#pragma warning(pop)
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <wil/com.h>
#include <wil/resource.h>
#include <wincodec.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
struct Response
{
    std::string status = "200 OK", headers;
    std::vector<std::byte> body;
    std::optional<std::size_t> declared;
    bool chunked{}, wait{};
};
// This fixture owns a single anonymous IPv4 loopback listener on an ephemeral
// port. Its protocol is deliberately HTTP test data, not a production server or
// a replacement transport; it never resolves/connects to an external endpoint.
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626)
class LoopbackImages final
{
    using SocketEvent = wil::unique_any<WSAEVENT, decltype(&WSACloseEvent), WSACloseEvent>;
    wil::unique_call<decltype(&WSACleanup), WSACleanup, false> winsock;
    wil::unique_socket listener;
    SocketEvent changed;
    wil::unique_handle stop, received, release;
    std::function<Response(std::string_view)> respond;
    std::mutex mutex;
    std::string lastRequest;
    std::atomic<unsigned> requests{}, failures{};
    unsigned short port{};
    std::jthread worker;
    static bool Send(SOCKET client, std::string_view bytes) noexcept
    {
        while (!bytes.empty())
        {
            const auto count = send(client, bytes.data(), static_cast<int>(std::min<std::size_t>(bytes.size(), 65536)), 0);
            if (count <= 0)
                return false;
            bytes.remove_prefix(static_cast<std::size_t>(count));
        }
        return true;
    }
    void Run() noexcept
    try
    {
        const std::array<HANDLE, 2> events{stop.get(), changed.get()};
        while (WaitForMultipleObjects(2, events.data(), FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
        {
            WSANETWORKEVENTS network{};
            if (WSAEnumNetworkEvents(listener.get(), changed.get(), &network) == SOCKET_ERROR)
                break;
            wil::unique_socket client(accept(listener.get(), nullptr, nullptr));
            if (!client)
                continue;
            if (WSAEventSelect(client.get(), nullptr, 0) == SOCKET_ERROR)
                break;
            u_long blocking{};
            DWORD timeout = 500;
            if (ioctlsocket(client.get(), FIONBIO, &blocking) == SOCKET_ERROR ||
                setsockopt(client.get(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) == SOCKET_ERROR ||
                setsockopt(client.get(), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) == SOCKET_ERROR)
                break;
            std::string request;
            std::array<char, 4096> bytes{};
            const auto deadline = GetTickCount64() + 1500;
            while (!request.contains("\r\n\r\n") && request.size() < 16384 && GetTickCount64() < deadline &&
                   WaitForSingleObject(stop.get(), 0) != WAIT_OBJECT_0)
            {
                const auto count = recv(client.get(), bytes.data(), static_cast<int>(bytes.size()), 0);
                if (count <= 0)
                    break;
                request.append(bytes.data(), static_cast<std::size_t>(count));
            }
            if (!request.contains("\r\n\r\n"))
                continue;
            const auto first = request.find(' '), last = request.find(' ', first + 1);
            if (first == request.npos || last == request.npos)
                continue;
            auto response = respond(std::string_view(request).substr(first + 1, last - first - 1));
            {
                std::lock_guard lock(mutex);
                lastRequest = request;
            }
            ++requests;
            (void)SetEvent(received.get());
            if (response.wait)
            {
                const std::array<HANDLE, 2> completion{stop.get(), release.get()};
                if (WaitForMultipleObjects(2, completion.data(), FALSE, 5000) != WAIT_OBJECT_0 + 1)
                    continue;
            }
            std::string header = "HTTP/1.1 " + response.status + "\r\nConnection: close\r\n" + response.headers;
            if (response.chunked)
                header += "Transfer-Encoding: chunked\r\n\r\n" + std::format("{:x}\r\n", response.body.size());
            else
                header += std::format("Content-Length: {}\r\n\r\n", response.declared.value_or(response.body.size()));
            if (!Send(client.get(), header))
                continue;
            const std::string_view body(reinterpret_cast<const char *>(response.body.data()), response.body.size());
            if (Send(client.get(), body) && response.chunked)
                (void)Send(client.get(), "\r\n0\r\n\r\n");
            (void)shutdown(client.get(), SD_SEND);
        }
    }
    catch (const std::bad_alloc &)
    {
        std::terminate();
    }
    catch (const std::exception &)
    {
        // A fixture worker cannot throw through the native thread entry. The
        // owner observes this failure after its bounded transport operation.
        ++failures;
        (void)SetEvent(received.get());
    }

  public:
    explicit LoopbackImages(std::function<Response(std::string_view)> handler) : respond(std::move(handler))
    {
        WSADATA data{};
        Require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "initialize loopback fixture sockets");
        winsock.activate();
        stop.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        received.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        release.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        changed.reset(WSACreateEvent());
        listener.reset(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        Require(stop && received && release && changed && listener, "create only owned loopback fixture resources");
        BOOL exclusive = TRUE;
        Require(setsockopt(listener.get(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char *>(&exclusive), sizeof(exclusive)) == 0,
                "loopback fixture exclusively owns its endpoint");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(bind(listener.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0 && listen(listener.get(), 8) == 0,
                "bind only the ephemeral IPv4 loopback endpoint");
        int size = sizeof(address);
        Require(getsockname(listener.get(), reinterpret_cast<sockaddr *>(&address), &size) == 0, "discover the owned loopback fixture port");
        port = ntohs(address.sin_port);
        Require(WSAEventSelect(listener.get(), changed.get(), FD_ACCEPT | FD_CLOSE) == 0, "wait for fixture connections without an idle timer");
        worker = std::jthread([this] { Run(); });
    }
    ~LoopbackImages()
    {
        (void)SetEvent(stop.get());
        // Test-only ownership: the listener and worker are retired before the
        // fixture's sockets/events. Production UI never waits for network work.
    }
    std::wstring Url(std::wstring_view path = L"/image.bmp") const
    {
        return std::format(L"http://127.0.0.1:{}{}", port, path);
    }
    unsigned Count() const noexcept
    {
        return requests.load();
    }
    std::string Request()
    {
        std::lock_guard lock(mutex);
        return lastRequest;
    }
    void AwaitRequest() const
    {
        Require(WaitForSingleObject(received.get(), 5000) == WAIT_OBJECT_0 && !failures.load(), "loopback fixture receives its bounded request");
    }
    void Release() noexcept
    {
        (void)SetEvent(release.get());
    }
};
#pragma warning(pop)
std::vector<std::byte> Bitmap()
{
    constexpr std::array<unsigned char, 62> bytes{0x42, 0x4d, 62, 0, 0, 0, 0, 0,  0, 0, 54, 0, 0, 0, 40,  0,   0, 0, 2, 0, 0,
                                                  0,    1,    0,  0, 0, 1, 0, 24, 0, 0, 0,  0, 0, 8, 0,   0,   0, 0, 0, 0, 0,
                                                  0,    0,    0,  0, 0, 0, 0, 0,  0, 0, 0,  0, 0, 0, 255, 255, 0, 0, 0, 0};
    std::vector<std::byte> result(bytes.size());
    std::memcpy(result.data(), bytes.data(), bytes.size());
    return result;
}
} // namespace

void RegisterRemoteImageTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.HttpAnonymousFetchAndNativeDecode",
         []
         {
             // Independent ABI encodings: severity C0000000, Visual C++ facility
             // 006D, Win32 module/procedure errors 007E/007F. These expectations
             // do not reuse the production VcppException/HRESULT conversion.
             struct ImportCategory
             {
                 DWORD code;
                 HRESULT fallback;
             };
             constexpr HRESULT mappingSentinel = static_cast<HRESULT>(0x8000000AUL);
             constexpr HRESULT accessDenied = static_cast<HRESULT>(0x80070005UL);
             for (const auto category : {ImportCategory{0xC06D007EUL, static_cast<HRESULT>(0x8007007EUL)},
                                         ImportCategory{0xC06D007FUL, static_cast<HRESULT>(0x8007007FUL)}})
             {
                 DelayLoadInfo information{};
                 information.cb = static_cast<DWORD>(sizeof(information));
                 information.szDll = "WINHTTP.dll";
                 EXCEPTION_RECORD record{};
                 record.ExceptionCode = category.code;
                 EXCEPTION_POINTERS failure{&record, nullptr};
                 const auto expectMapping = [&](DWORD parameters, const DelayLoadInfo *detail, HRESULT expected)
                 {
                     record.NumberParameters = parameters;
                     record.ExceptionInformation[0] = reinterpret_cast<ULONG_PTR>(detail);
                     HRESULT result = mappingSentinel;
                     Require(RemoteImageDetail::WinHttpImportFailure(&failure, result) == EXCEPTION_EXECUTE_HANDLER && result == expected &&
                                 FAILED(result),
                             "documented delay-loader categories map ABI records to exact failing HRESULTs");
                 };
                 expectMapping(1, &information, category.fallback);
                 information.dwLastError = ERROR_ACCESS_DENIED;
                 expectMapping(1, &information, accessDenied);
                 expectMapping(0, &information, category.fallback);
                 expectMapping(2, &information, category.fallback);
                 expectMapping(1, nullptr, category.fallback);
                 information.cb = static_cast<DWORD>(sizeof(information)) - 1;
                 expectMapping(1, &information, category.fallback);
                 information.cb = 0;
                 expectMapping(1, &information, category.fallback);
                 information.cb = static_cast<DWORD>(sizeof(information));
                 information.dwLastError = 0;
                 expectMapping(1, &information, category.fallback);
                 for (const DWORD unrelated : {0xC0000005UL, 0xE06D7363UL})
                 {
                     record.ExceptionCode = unrelated;
                     record.NumberParameters = 1;
                     record.ExceptionInformation[0] = reinterpret_cast<ULONG_PTR>(&information);
                     information.dwLastError = ERROR_ACCESS_DENIED;
                     HRESULT result = mappingSentinel;
                     Require(RemoteImageDetail::WinHttpImportFailure(&failure, result) == EXCEPTION_CONTINUE_SEARCH && result == mappingSentinel,
                             "access violations and C++ exceptions continue search without changing the caller's HRESULT");
                 }
             }
             // Borrowed module identities may already exist through another
             // dependency. Early rejection must leave either state unchanged;
             // this does not assume either system DLL is globally absent.
             const auto winHttpBefore = GetModuleHandleW(L"WINHTTP.dll");
             const auto winsockBefore = GetModuleHandleW(L"WS2_32.dll");
             const std::vector<std::byte> untouched{std::byte{0x51}};
             auto rejected = untouched;
             constexpr std::wstring_view unused = L"http://127.0.0.1:1/unused.bmp";
             for (const auto *url : {L"file:///unused.bmp", L"http://user:secret@127.0.0.1/a", L"http://127.0.0.1/a#part", L"http://127.0.0.1/a%Q0"})
                 Require(FetchRemoteImage(url, rejected) == E_INVALIDARG && rejected == untouched,
                         "invalid URI admission preserves exact output before resolving optional HTTP imports");
             for (const auto limits :
                  {RemoteImageLimits{0, 1024}, RemoteImageLimits{8001, 1024}, RemoteImageLimits{1000, 0}, RemoteImageLimits{1000, 16777217}})
                 Require(FetchRemoteImage(unused, rejected, nullptr, nullptr, limits) == E_INVALIDARG && rejected == untouched,
                         "invalid timeout/byte bounds preserve exact output before resolving optional HTTP imports");
             std::atomic_bool cancelled{true};
             Require(FetchRemoteImage(unused, rejected, &cancelled) == HRESULT_FROM_WIN32(ERROR_CANCELLED) && rejected == untouched,
                     "pre-cancelled requests preserve exact output before resolving optional HTTP imports");
             wil::unique_handle aborted(CreateEventW(nullptr, TRUE, TRUE, nullptr));
             Require(static_cast<bool>(aborted), "create an already-signalled owned image abort event");
             Require(FetchRemoteImage(unused, rejected, nullptr, aborted.get()) == HRESULT_FROM_WIN32(ERROR_CANCELLED) && rejected == untouched,
                     "pre-aborted requests preserve exact output before resolving optional HTTP imports");
             Require(GetModuleHandleW(L"WINHTTP.dll") == winHttpBefore && GetModuleHandleW(L"WS2_32.dll") == winsockBefore,
                     "early URI/bounds/cancel/abort admission leaves optional network module identities unchanged");
             const auto bitmap = Bitmap();
             LoopbackImages server(
                 [&](std::string_view)
                 {
                     Response response;
                     response.headers = "Content-Type: image/bmp\r\nSet-Cookie: private=never-return\r\n";
                     response.body = bitmap;
                     return response;
                 });
             std::vector<std::byte> encoded;
             for (unsigned index = 0; index < 2; ++index)
             {
                 const auto result = FetchRemoteImage(server.Url(), encoded);
                 if (FAILED(result))
                     throw std::runtime_error(std::format("anonymous loopback fetch failed: 0x{:08X}; received {} requests",
                                                          static_cast<unsigned>(result), server.Count()));
                 Require(encoded == bitmap, "anonymous HTTP body is exact bounded image bytes");
                 const auto request = server.Request();
                 Require(request.starts_with("GET /image.bmp HTTP/1.1\r\n") && !request.contains("Cookie:") && !request.contains("Authorization:") &&
                             !request.contains("Referer:"),
                         "no cookie, credentials or document referrer is sent even after a Set-Cookie response");
             }
             Require(server.Count() == 2, "one request per explicit image fetch, without retries or hidden requests");
             const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
             Require(SUCCEEDED(initialized) || initialized == RPC_E_CHANGED_MODE, "use the caller's existing COM apartment or initialize one");
             const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
             wil::com_ptr<IWICImagingFactory> factory;
             Require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put()))),
                     "create caller-owned native image decoder");
             std::shared_ptr<const DxUi::RichText::RichImage> image;
             Require(DxUi::RichText::RichImage::Decode(factory.get(), encoded, image) == S_OK && image && image->Width() == 2 &&
                         image->Height() == 1 && image->Pixels().size() == 8 && image->Pixels()[2] == std::byte{255} &&
                         image->Pixels()[4] == std::byte{255},
                     "remote bytes reuse canonical bounded native image decoding and exact red/blue pixels");
         });
    test("Editor.HttpRedirectAuthBoundsAndFailures",
         []
         {
             LoopbackImages server(
                 [](std::string_view path)
                 {
                     Response response;
                     if (path == "/redirect")
                     {
                         response.status = "302 Found";
                         response.headers = "Location: /must-never-follow\r\n";
                     }
                     else if (path == "/auth")
                     {
                         response.status = "401 Unauthorized";
                         response.headers = "WWW-Authenticate: Negotiate\r\n";
                     }
                     else if (path == "/large")
                         response.declared = 16777217;
                     else if (path == "/chunked")
                     {
                         response.chunked = true;
                         response.body.resize(128);
                     }
                     else if (path == "/gzip")
                     {
                         response.headers = "Content-Encoding: gzip\r\n";
                         response.body.resize(16);
                     }
                     else if (path == "/truncated")
                     {
                         response.declared = 64;
                         response.body.resize(7);
                     }
                     return response;
                 });
             const std::vector<std::byte> untouched{std::byte{0x73}};
             auto output = untouched;
             for (const auto *path : {L"/redirect", L"/auth", L"/large", L"/gzip", L"/truncated"})
                 Require(FAILED(FetchRemoteImage(server.Url(path), output)) && output == untouched,
                         "redirect/auth/oversize/compressed/truncated responses fail without replacing the caller's bytes");
             Require(FetchRemoteImage(server.Url(L"/chunked"), output, nullptr, nullptr, {1000, 64}) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) &&
                         output == untouched,
                     "chunked bodies obey the exact byte bound without trusting Content-Length");
             Require(server.Count() == 6 && !server.Request().contains("Authorization:"),
                     "the transport never follows a redirect or retries an authentication challenge");
             const auto origin = server.Url();
             for (const auto &url : {L"file:///image.bmp", L"data:image/png;base64,a", L"http://user:secret@127.0.0.1/a", L"http://127.0.0.1/a#part",
                                     L"http://127.0.0.1/a\r\nCookie: secret", L"http://127.0.0.1\\a"})
                 Require(FetchRemoteImage(url, output) == E_INVALIDARG && output == untouched,
                         "unsafe/non-HTTP URL spellings are rejected before transport access");
             Require(server.Count() == 6, "rejected addresses cannot contact the owned listener");
         });
    test("Editor.HttpCancellationDeadlineAndOwnerClose",
         []
         {
             for (const bool ownerClose : {false, true})
             {
                 LoopbackImages server(
                     [](std::string_view)
                     {
                         Response response;
                         response.wait = true;
                         response.body = Bitmap();
                         return response;
                     });
                 std::atomic_bool cancelled{};
                 wil::unique_handle abort(CreateEventW(nullptr, TRUE, FALSE, nullptr));
                 Require(static_cast<bool>(abort), "create an owned image-service close event");
                 const std::vector<std::byte> untouched{std::byte{0x27}};
                 auto encoded = untouched;
                 auto transfer = std::async(std::launch::async, [&] { return FetchRemoteImage(server.Url(), encoded, &cancelled, abort.get()); });
                 server.AwaitRequest();
                 if (ownerClose)
                     (void)SetEvent(abort.get());
                 else
                     cancelled.store(true);
                 Require(transfer.wait_for(std::chrono::seconds(1)) == std::future_status::ready &&
                             transfer.get() == HRESULT_FROM_WIN32(ERROR_CANCELLED) && encoded == untouched,
                         "request cancellation and owner-close abort an active read without waiting for its server");
                 server.Release();
             }
             LoopbackImages stalled(
                 [](std::string_view)
                 {
                     Response response;
                     response.wait = true;
                     return response;
                 });
             std::vector<std::byte> encoded{std::byte{0x39}};
             const auto before = GetTickCount64();
             Require(FAILED(FetchRemoteImage(stalled.Url(), encoded, nullptr, nullptr, {120, 1024})) && GetTickCount64() - before < 1500 &&
                         encoded == std::vector<std::byte>{std::byte{0x39}},
                     "the total transfer deadline bounds a server that never returns headers and preserves output");
             stalled.Release();
         });
    test("Editor.RemoteImagePrivacyOptInRevocationAndShutdown",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             const auto fixture = root / L"remote-image-policy";
             std::filesystem::create_directories(fixture / L"Writing");
             const auto pixels = Bitmap();
             Require(SUCCEEDED(WriteBytesAtomic(fixture / L"Writing/cover.bmp",
                                                std::string_view(reinterpret_cast<const char *>(pixels.data()), pixels.size()))),
                     "seed the invented local image beside the owned document");
             LoopbackImages ready(
                 [](std::string_view)
                 {
                     Response response;
                     response.body = Bitmap();
                     return response;
                 });
             LoopbackImages pending(
                 [](std::string_view)
                 {
                     Response response;
                     response.wait = true;
                     response.body = Bitmap();
                     return response;
                 });
             LaunchOptions options;
             options.headless = true;
             options.dataDirectory = fixture / L"settings";
             Application app(options);
             Require(SUCCEEDED(app.Initialize()), "initialize hidden real image-policy controls and worker");
             const auto document = [&](const LoopbackImages &server)
             { return std::format(L"# Invented writing\n\n![Local cover](cover.bmp)\n\n![Remote cover]({})\n", server.Url()); };
             app.SetDocument(document(ready));
             app.Document().SetDocumentFilePath((fixture / L"Writing/Ideas.md").native());
             const auto await = [&](const std::function<bool()> &done, ULONGLONG budget = 5000)
             {
                 const auto deadline = GetTickCount64() + budget;
                 while (!done() && GetTickCount64() < deadline)
                     app.Prepare(16);
                 Require(done(), "the hidden image-policy worker settles before its test deadline");
             };
             await([&] { return app.Document().GetImageResourceStatistics().readySources == 1; });
             Require(!app.Preferences().allowRemoteImages && !app.Document().RemoteImagesEnabled() && ready.Count() == 0 &&
                         app.Document().GetImageResourceStatistics().requestedSources == 1,
                     "the default privacy preference emits no HTTP request while a local image renders");
             const auto source = app.Document().Snapshot();
             const auto selection = app.Document().GetSelection();
             app.Execute(Command::RemoteImages);
             Require(app.Preferences().allowRemoteImages && app.Document().RemoteImagesEnabled() &&
                         app.Document().GetImageResourceStatistics().readySources == 1,
                     "the real privacy command enables remote loading immediately while retaining local pixels");
             await([&] { return app.Document().GetImageResourceStatistics().readySources == 2; });
             Require(ready.Count() == 1 && app.Document().GetImageResourceStatistics().decodedBytes == 16,
                     "one opted-in anonymous request reaches the canonical native decoder through the application worker");
             app.Execute(Command::RemoteImages);
             Require(!app.Preferences().allowRemoteImages && !app.Document().RemoteImagesEnabled() &&
                         app.Document().GetImageResourceStatistics().readySources == 1 &&
                         app.Document().GetImageResourceStatistics().decodedBytes == 8 && app.Document().Snapshot().IsSameRevision(source) &&
                         app.Document().GetSelection() == selection && !app.Dirty(),
                     "opt-out immediately removes remote pixels without touching local pixels, source, selection or save state");
             app.SetDocument(document(pending));
             app.Document().SetDocumentFilePath((fixture / L"Writing/Ideas.md").native());
             app.Execute(Command::RemoteImages);
             await([&] { return pending.Count() == 1 && app.Document().GetImageResourceStatistics().readySources == 1; });
             app.Execute(Command::RemoteImages);
             Require(!app.Document().RemoteImagesEnabled() && app.Document().GetImageResourceStatistics().pendingSources == 0 &&
                         app.Document().GetImageResourceStatistics().decodedBytes == 8,
                     "opt-out cancels an actual blocked network transfer and retains only the local image");
             app.SetDocument(document(ready));
             app.Document().SetDocumentFilePath((fixture / L"Writing/Ideas.md").native());
             app.Execute(Command::RemoteImages);
             await([&] { return ready.Count() == 2 && app.Document().GetImageResourceStatistics().readySources == 2; }, 2000);
             Require(app.Document().GetText() == document(ready) && !app.Dirty() && app.Document().GetImageResourceStatistics().decodedBytes == 16,
                     "the same worker admits a later real request before the cancelled server is released");
             app.Execute(Command::RemoteImages);
             pending.Release();
             app.Prepare(128);
             Require(app.Document().GetImageResourceStatistics().readySources == 1 && app.Document().GetImageResourceStatistics().decodedBytes == 8,
                     "a retired network completion cannot republish pixels after opt-out");
             LoopbackImages closing(
                 [](std::string_view)
                 {
                     Response response;
                     response.wait = true;
                     return response;
                 });
             app.SetDocument(document(closing));
             app.Document().SetDocumentFilePath((fixture / L"Writing/Ideas.md").native());
             app.Execute(Command::RemoteImages);
             await([&] { return closing.Count() == 1; });
             const bool foregroundBeforeClose = foreground.NeverAcquiredForeground();
             const auto before = GetTickCount64();
             (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
             const auto closeMilliseconds = GetTickCount64() - before;
             const bool foregroundAfterClose = foreground.NeverAcquiredForeground();
             Require(
                 closeMilliseconds < 500,
                 std::format("clean Close invalidates the active network publisher without a UI-thread join: {} ms (limit <500)", closeMilliseconds)
                     .c_str());
             Require(foregroundBeforeClose && foregroundAfterClose,
                     std::format("hidden remote-image policy never activates: before Close={}, through Close={}", foregroundBeforeClose,
                                 foregroundAfterClose)
                         .c_str());
             closing.Release();
         });
    test("Editor.WikiImageEmbedsStayUnresolved",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             const auto fixture = root / L"wiki-image-embeds";
             std::filesystem::create_directories(fixture / L"Writing");
             const auto pixels = Bitmap();
             Require(SUCCEEDED(WriteBytesAtomic(fixture / L"Writing/cover.bmp",
                                                std::string_view(reinterpret_cast<const char *>(pixels.data()), pixels.size()))),
                     "seed one invented image that both the ordinary and the wiki embed name");
             LaunchOptions options;
             options.headless = true;
             options.dataDirectory = fixture / L"settings";
             Application app(options);
             Require(SUCCEEDED(app.Initialize()), "initialize hidden image worker with default syntax preferences");
             const std::wstring text = L"# Invented embeds\n\n![Local cover](cover.bmp)\n\n![[cover.bmp]]\n";
             app.SetDocument(text);
             app.Document().SetDocumentFilePath((fixture / L"Writing/Ideas.md").native());
             const auto await = [&](const std::function<bool()> &done)
             {
                 const auto deadline = GetTickCount64() + 5000;
                 while (!done() && GetTickCount64() < deadline)
                     app.Prepare(16);
                 Require(done(), "the hidden image worker settles before its test deadline");
             };
             await(
                 [&]
                 {
                     return app.Document().GetImageResourceStatistics().readySources == 1 &&
                            app.Document().GetImageResourceStatistics().failedSources == 1;
                 });
             const auto statistics = app.Document().GetImageResourceStatistics();
             // One 2x1 PBGRA decode: only the ordinary local reference reaches the file loader.
             Require(statistics.requestedSources == 2 && statistics.decodedBytes == 2u * 1u * 4u,
                     "the wiki embed is requested but declined, even though a same-named file exists beside the document");
             app.Execute(Command::SyntaxWikiLinks);
             app.Prepare(128);
             Require(!app.Preferences().SyntaxEnabled(MarkdownSyntax::WikiLinks) &&
                         app.Document().GetImageResourceStatistics().requestedSources == 1 &&
                         app.Document().GetImageResourceStatistics().failedSources == 0 && app.Document().GetText() == text && !app.Dirty(),
                     "turning wiki links off leaves the embed as literal text without editing the document");
             Require(foreground.NeverAcquiredForeground(), "hidden wiki-embed handling preserves foreground");
         });
}
} // namespace RedTilde
