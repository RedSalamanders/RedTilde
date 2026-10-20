#include "Storage.h"
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <atomic>
#include <format>
#include <new>
#include <shlobj.h>
#include <stdexcept>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
constexpr DWORD MaximumBytes = 4u * 65536u + 4096u;
std::atomic_uint64_t sequence{};
std::uint64_t FileTimeValue(FILETIME value) noexcept
{
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32u) | value.dwLowDateTime;
}
HRESULT LastError() noexcept
{
    return HRESULT_FROM_WIN32(GetLastError());
}
} // namespace

std::uint64_t ContentHash(std::string_view bytes) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char value : bytes)
    {
        hash ^= value;
        hash *= 1099511628211ull;
    }
    return hash;
}

HRESULT ReadBytes(const std::filesystem::path &path, std::string &bytes, FileStamp *stamp) noexcept
try
{
    wil::unique_handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file)
        return LastError();
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file.get(), &info))
        return LastError();
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || info.nFileSizeHigh || info.nFileSizeLow > MaximumBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    std::string staged(info.nFileSizeLow, '\0');
    DWORD count{};
    if (!staged.empty() && !ReadFile(file.get(), staged.data(), static_cast<DWORD>(staged.size()), &count, nullptr))
        return LastError();
    if (count != staged.size())
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    if (stamp)
        *stamp = {true, info.nFileSizeLow, FileTimeValue(info.ftLastWriteTime), ContentHash(staged)};
    bytes.swap(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}

HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected) noexcept
try
{
    if (bytes.size() > MaximumBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    const DWORD attributes = GetFileAttributesW(path.c_str());
    const bool exists = attributes != INVALID_FILE_ATTRIBUTES;
    if (!exists && GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
        return LastError();
    if (exists && (attributes & FILE_ATTRIBUTE_DIRECTORY))
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    if (expected)
    {
        FileStamp current;
        if (exists)
        {
            std::string previous;
            const HRESULT result = ReadBytes(path, previous, &current);
            if (FAILED(result))
                return result;
        }
        if (current != *expected)
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }
    std::filesystem::path temporary;
    wil::unique_handle file;
    for (unsigned attempt = 0; attempt < 8 && !file; ++attempt)
    {
        temporary = path;
        temporary += std::format(L".redtilde-{}-{}-{}.tmp", GetCurrentProcessId(), GetTickCount64(), sequence.fetch_add(1));
        file.reset(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file && GetLastError() != ERROR_FILE_EXISTS)
            return LastError();
    }
    if (!file)
        return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    auto removeTemporary = wil::scope_exit([&] { DeleteFileW(temporary.c_str()); });
    DWORD written{};
    if (!bytes.empty() && !WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr))
        return LastError();
    if (written != bytes.size())
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    if (!FlushFileBuffers(file.get()))
        return LastError();
    file.reset();
    if (exists)
    {
        if (!ReplaceFileW(path.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr))
            return LastError();
    }
    else if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH))
        return LastError();
    removeTemporary.release();
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(error.code().value());
}

HRESULT LoadDocument(const std::filesystem::path &path, StoredDocument &document) noexcept
try
{
    std::string bytes;
    StoredDocument staged;
    const HRESULT result = ReadBytes(path, bytes, &staged.stamp);
    if (FAILED(result))
        return result;
    staged.bom = bytes.starts_with("\xEF\xBB\xBF");
    staged.text = DxUi::RichText::DecodeUtf8(std::string_view(bytes).substr(staged.bom ? 3u : 0u));
    if (staged.text.size() > DxUi::RichText::Document::MaximumUnits)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    document = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::invalid_argument &)
{
    return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
}
catch (const std::length_error &)
{
    return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
}

HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected, FileStamp &saved) noexcept
try
{
    if (text.size() > DxUi::RichText::Document::MaximumUnits)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    std::string bytes = bom ? "\xEF\xBB\xBF" : "";
    bytes += DxUi::RichText::EncodeUtf8(text);
    const HRESULT result = WriteBytesAtomic(path, bytes, expected);
    if (FAILED(result))
        return result;
    std::string readback;
    FileStamp confirmed;
    const HRESULT readResult = ReadBytes(path, readback, &confirmed);
    if (FAILED(readResult))
        return readResult;
    if (readback != bytes)
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    saved = confirmed;
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::invalid_argument &)
{
    return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
}

std::wstring ErrorText(HRESULT result)
{
    wchar_t message[512]{};
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, static_cast<DWORD>(result), 0, message,
                       static_cast<DWORD>(std::size(message)), nullptr))
        return message;
    return std::format(L"Error 0x{:08X}", static_cast<unsigned>(result));
}

std::filesystem::path DefaultDataDirectory()
{
    wil::unique_cotaskmem_string local;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, local.put())))
        return {};
    return std::filesystem::path(local.get()) / L"RedTilde";
}
} // namespace RedTilde
