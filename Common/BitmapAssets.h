#pragma once
#include "Common/Storage.h"
#include <span>

namespace RedTilde
{
inline constexpr std::size_t MaximumClipboardDibBytes = 32u * 1024u * 1024u;
inline constexpr std::size_t MaximumClipboardPngBytes = 64u * 1024u * 1024u;
struct BitmapAsset
{
    std::filesystem::path path;
    std::wstring uri;
    FileStamp stamp;
    unsigned width{}, height{};
};
// COM-initialized worker only. The caller owns the DIB for this synchronous call.
// No existing file is replaced. Publish the source reference only after success.
[[nodiscard]] HRESULT CreateBitmapAsset(const std::filesystem::path &dataDirectory, std::span<const std::byte> dib, bool v5,
                                        BitmapAsset &asset) noexcept;
[[nodiscard]] HRESULT RetireBitmapAsset(const BitmapAsset &asset) noexcept;
} // namespace RedTilde
