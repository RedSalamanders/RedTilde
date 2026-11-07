#pragma once
#include "Common/BitmapAssets.h"
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <wil/com.h>
#include <wincodec.h>

namespace RedTilde::Testing
{
inline std::vector<std::byte> ClipboardBitmapFixture(bool v5 = false)
{
    if (v5)
    {
        BITMAPV5HEADER header{};
        header.bV5Size = sizeof(header);
        header.bV5Width = 2;
        header.bV5Height = -2;
        header.bV5Planes = 1;
        header.bV5BitCount = 32;
        header.bV5Compression = BI_BITFIELDS;
        header.bV5SizeImage = 16;
        header.bV5RedMask = 0x00ff0000;
        header.bV5GreenMask = 0x0000ff00;
        header.bV5BlueMask = 0x000000ff;
        header.bV5AlphaMask = 0xff000000;
        header.bV5CSType = LCS_sRGB;
        header.bV5Intent = LCS_GM_IMAGES;
        const std::array<unsigned char, 16> pixels{0, 0, 255, 255, 0, 255, 0, 128, 255, 0, 0, 255, 255, 255, 255, 64};
        std::vector<std::byte> result(sizeof(header) + pixels.size());
        std::memcpy(result.data(), &header, sizeof(header));
        std::memcpy(result.data() + sizeof(header), pixels.data(), pixels.size());
        return result;
    }
    BITMAPINFOHEADER header{};
    header.biSize = sizeof(header);
    header.biWidth = 2;
    header.biHeight = 2;
    header.biPlanes = 1;
    header.biBitCount = 24;
    header.biCompression = BI_RGB;
    header.biSizeImage = 16;
    // Bottom-up blue/white then red/green; each physical row has two padding bytes.
    const std::array<unsigned char, 16> pixels{255, 0, 0, 255, 255, 255, 0, 0, 0, 0, 255, 0, 255, 0, 0, 0};
    std::vector<std::byte> result(sizeof(header) + pixels.size());
    std::memcpy(result.data(), &header, sizeof(header));
    std::memcpy(result.data() + sizeof(header), pixels.data(), pixels.size());
    return result;
}
inline std::array<unsigned char, 16> ReadClipboardPngPixels(const std::filesystem::path &path)
{
    wil::com_ptr<IWICImagingFactory> factory;
    THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
    wil::com_ptr<IWICBitmapDecoder> decoder;
    THROW_IF_FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.put()));
    wil::com_ptr<IWICBitmapFrameDecode> frame;
    THROW_IF_FAILED(decoder->GetFrame(0, frame.put()));
    UINT width{}, height{};
    THROW_IF_FAILED(frame->GetSize(&width, &height));
    if (width != 2 || height != 2)
        throw std::runtime_error("clipboard PNG fixture dimensions changed");
    wil::com_ptr<IWICFormatConverter> converter;
    THROW_IF_FAILED(factory->CreateFormatConverter(converter.put()));
    THROW_IF_FAILED(
        converter->Initialize(frame.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
    std::array<unsigned char, 16> pixels{};
    THROW_IF_FAILED(converter->CopyPixels(nullptr, 8, static_cast<UINT>(pixels.size()), pixels.data()));
    return pixels;
}
inline std::size_t ClipboardAssetCount(const std::filesystem::path &data)
{
    std::size_t count{};
    std::error_code error;
    const auto directory = data / L"Assets";
    if (!std::filesystem::exists(directory, error))
        return 0;
    for (std::filesystem::directory_iterator entry(directory, error), end; !error && entry != end; entry.increment(error))
        ++count;
    if (error)
        throw std::runtime_error("clipboard fixture asset inventory failed");
    return count;
}
} // namespace RedTilde::Testing
