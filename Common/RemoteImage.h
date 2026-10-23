#pragma once
#include <atomic>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
#include <windows.h>

namespace RedTilde
{
struct RemoteImageLimits
{
    unsigned timeoutMilliseconds = 8000;
    std::size_t maximumEncodedBytes = 16777216;
};
// Consumer transport policy, separate from DxUi's URI admission and immutable
// image decoder. Call only on the image worker: HTTP(S), anonymous direct GET,
// no cookies/redirects/referrer/proxy discovery, and ordinary TLS validation.
// A fixed receive buffer survives asynchronous WinHTTP handle retirement.
// Failure/cancellation leaves output unchanged; no callback enters a control.
[[nodiscard]] HRESULT FetchRemoteImage(std::wstring_view source, std::vector<std::byte> &output, const std::atomic_bool *cancellation = nullptr,
                                       HANDLE abortEvent = nullptr, RemoteImageLimits limits = {}) noexcept;
} // namespace RedTilde
