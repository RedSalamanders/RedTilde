#include "BitmapAssets.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <shlwapi.h>
#include <stdexcept>
#include <wil/com.h>
#include <wil/resource.h>
#include <wincodec.h>

namespace RedTilde
{
namespace
{
HRESULT Encode(std::span<const std::byte> dib, bool v5, std::string &png, unsigned &width, unsigned &height)
{
    if (dib.size() < sizeof(BITMAPINFOHEADER) || dib.size() > MaximumClipboardDibBytes)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    BITMAPINFOHEADER header{};
    std::memcpy(&header, dib.data(), sizeof(header));
    if ((header.biSize != sizeof(BITMAPINFOHEADER) && header.biSize != sizeof(BITMAPV4HEADER) && header.biSize != sizeof(BITMAPV5HEADER)) ||
        header.biSize > dib.size() || (v5 && header.biSize != sizeof(BITMAPV5HEADER)) || header.biWidth <= 0 || header.biWidth > 8192 ||
        header.biHeight == 0 || header.biHeight == (std::numeric_limits<LONG>::min)() || header.biPlanes != 1 ||
        (header.biBitCount != 24 && header.biBitCount != 32) || header.biClrUsed != 0 ||
        (header.biCompression != BI_RGB && !(header.biCompression == BI_BITFIELDS && header.biBitCount == 32)))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto rows = static_cast<unsigned>(header.biHeight < 0 ? -header.biHeight : header.biHeight);
    if (rows > 8192)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (header.biSize == sizeof(BITMAPV5HEADER))
    {
        BITMAPV5HEADER full{};
        std::memcpy(&full, dib.data(), sizeof(full));
        if (full.bV5ProfileData || full.bV5ProfileSize || full.bV5CSType == PROFILE_EMBEDDED || full.bV5CSType == PROFILE_LINKED)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    const std::size_t masks = header.biSize == sizeof(BITMAPINFOHEADER) && header.biCompression == BI_BITFIELDS ? 3u * sizeof(DWORD) : 0u;
    const auto pixelOffset = static_cast<std::size_t>(header.biSize) + masks;
    const auto stride = ((static_cast<std::size_t>(header.biWidth) * header.biBitCount + 31u) / 32u) * 4u;
    const auto pixels = stride * rows;
    if (pixelOffset > dib.size() || pixels > dib.size() - pixelOffset ||
        (header.biSizeImage && (header.biSizeImage < pixels || header.biSizeImage > dib.size() - pixelOffset)) ||
        static_cast<std::size_t>(header.biWidth) * rows * 4u > MaximumClipboardPngBytes)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfSize = static_cast<DWORD>(sizeof(file) + dib.size());
    file.bfOffBits = static_cast<DWORD>(sizeof(file) + pixelOffset);
    std::string bmp(sizeof(file) + dib.size(), '\0');
    std::memcpy(bmp.data(), &file, sizeof(file));
    std::memcpy(bmp.data() + sizeof(file), dib.data(), dib.size());
    wil::com_ptr<IWICImagingFactory> factory;
    RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
    wil::com_ptr<IWICStream> input;
    RETURN_IF_FAILED(factory->CreateStream(input.put()));
    RETURN_IF_FAILED(input->InitializeFromMemory(reinterpret_cast<BYTE *>(bmp.data()), static_cast<DWORD>(bmp.size())));
    wil::com_ptr<IWICBitmapDecoder> decoder;
    RETURN_IF_FAILED(factory->CreateDecoderFromStream(input.get(), nullptr, WICDecodeMetadataCacheOnLoad, decoder.put()));
    wil::com_ptr<IWICBitmapFrameDecode> source;
    RETURN_IF_FAILED(decoder->GetFrame(0, source.put()));
    UINT decodedWidth{}, decodedHeight{};
    RETURN_IF_FAILED(source->GetSize(&decodedWidth, &decodedHeight));
    if (decodedWidth != static_cast<UINT>(header.biWidth) || decodedHeight != rows)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    wil::com_ptr<IStream> output;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, output.put()));
    wil::com_ptr<IWICBitmapEncoder> encoder;
    RETURN_IF_FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put()));
    RETURN_IF_FAILED(encoder->Initialize(output.get(), WICBitmapEncoderNoCache));
    wil::com_ptr<IWICBitmapFrameEncode> frame;
    wil::com_ptr<IPropertyBag2> properties;
    RETURN_IF_FAILED(encoder->CreateNewFrame(frame.put(), properties.put()));
    RETURN_IF_FAILED(frame->Initialize(properties.get()));
    RETURN_IF_FAILED(frame->SetSize(decodedWidth, decodedHeight));
    auto format = GUID_WICPixelFormat32bppBGRA;
    RETURN_IF_FAILED(frame->SetPixelFormat(&format));
    wil::com_ptr<IWICFormatConverter> converter;
    RETURN_IF_FAILED(factory->CreateFormatConverter(converter.put()));
    RETURN_IF_FAILED(converter->Initialize(source.get(), format, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
    RETURN_IF_FAILED(frame->WriteSource(converter.get(), nullptr));
    RETURN_IF_FAILED(frame->Commit());
    RETURN_IF_FAILED(encoder->Commit());
    STATSTG info{};
    RETURN_IF_FAILED(output->Stat(&info, STATFLAG_NONAME));
    if (!info.cbSize.QuadPart || info.cbSize.QuadPart > MaximumClipboardPngBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    std::string staged(static_cast<std::size_t>(info.cbSize.QuadPart), '\0');
    RETURN_IF_FAILED(output->Seek({}, STREAM_SEEK_SET, nullptr));
    ULONG copied{};
    RETURN_IF_FAILED(output->Read(staged.data(), static_cast<ULONG>(staged.size()), &copied));
    if (copied != staged.size())
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    png.swap(staged);
    width = decodedWidth;
    height = decodedHeight;
    return S_OK;
}
} // namespace
HRESULT CreateBitmapAsset(const std::filesystem::path &dataDirectory, std::span<const std::byte> dib, bool v5, BitmapAsset &asset) noexcept
try
{
    if (!dataDirectory.is_absolute() || dataDirectory.native().size() > 32767u)
        return E_INVALIDARG;
    const auto drive = dataDirectory.root_name().native();
    if (drive.size() != 2u || drive[1] != L':' || GetDriveTypeW(dataDirectory.root_path().c_str()) != DRIVE_FIXED)
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    std::string png;
    BitmapAsset staged;
    RETURN_IF_FAILED(Encode(dib, v5, png, staged.width, staged.height));
    const auto directory = dataDirectory / L"Assets";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    const DWORD attributes = GetFileAttributesW(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    GUID id{};
    RETURN_IF_FAILED(CoCreateGuid(&id));
    std::array<wchar_t, 40> name{};
    if (!StringFromGUID2(id, name.data(), static_cast<int>(name.size())))
        return E_UNEXPECTED;
    staged.path = directory / (std::wstring(L"clipboard-") + name.data() + L".png");
    std::wstring uri(32768u, L'\0');
    DWORD uriSize = static_cast<DWORD>(uri.size());
    RETURN_IF_FAILED(UrlCreateFromPathW(staged.path.c_str(), uri.data(), &uriSize, 0));
    uri.resize(uriSize);
    staged.uri = std::move(uri);
    RETURN_IF_FAILED(WriteNewBytesDurable(staged.path, png, staged.stamp, MaximumClipboardPngBytes));
    asset = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}
HRESULT RetireBitmapAsset(const BitmapAsset &asset) noexcept
{
    if (asset.path.empty() || !asset.stamp.exists)
        return S_FALSE;
    return RemoveCreatedBytesIfUnchanged(asset.path, asset.stamp, MaximumClipboardPngBytes);
}
} // namespace RedTilde
