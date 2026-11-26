#pragma once
// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// clang-format on

#include "TextUtilities.h"

#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <wil/resource.h>

namespace RedTilde
{
[[nodiscard]] inline std::size_t UncShareEnd(std::wstring_view path) noexcept
{
    if (! path.starts_with(L"\\\\") || path.starts_with(L"\\\\?\\") || path.starts_with(L"\\\\.\\"))
        return path.npos;
    const auto server = path.find(L'\\', 2u);
    if (server == path.npos || server == 2u || server + 1u == path.size())
        return path.npos;
    const auto share = path.find(L'\\', server + 1u);
    const auto end   = share == path.npos ? path.size() : share;
    return end == server + 1u ? path.npos : end;
}

[[nodiscard]] inline std::size_t RootNameEnd(std::wstring_view path) noexcept
{
    const auto isAsciiAlpha = [](wchar_t ch) noexcept { return (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'); };
    if (path.size() >= 2u && isAsciiAlpha(path[0]) && path[1] == L':' && (path.size() == 2u || path[2] == L'\\'))
        return 2u;
    if (path.starts_with(L"\\\\?\\UNC\\") || path.starts_with(L"\\\\?\\unc\\"))
    {
        const auto server = path.find(L'\\', 8u);
        if (server == path.npos || server == 8u || server + 1u == path.size())
            return path.npos;
        const auto share = path.find(L'\\', server + 1u);
        const auto end   = share == path.npos ? path.size() : share;
        return end == server + 1u ? path.npos : end;
    }
    if (path.starts_with(L"\\\\?\\") && path.size() >= 6u && isAsciiAlpha(path[4]) && path[5] == L':' && (path.size() == 6u || path[6] == L'\\'))
        return 6u;
    return UncShareEnd(path);
}

[[nodiscard]] inline std::filesystem::path NormalizeRootPath(const std::filesystem::path& path)
{
    if (path.empty())
        return {};
    std::filesystem::path normalized = path.lexically_normal();
    std::wstring native              = normalized.native();
    std::ranges::replace(native, L'/', L'\\');
    const std::size_t rootEnd = RootNameEnd(native);
    if (rootEnd == std::wstring::npos)
    {
        if (! normalized.is_absolute())
            return normalized;
        if (normalized.relative_path().empty())
        {
            if (! native.empty() && native.back() != L'\\')
                native.push_back(L'\\');
            return std::filesystem::path(std::move(native));
        }
        const std::size_t rootSize = normalized.root_path().native().size();
        while (native.size() > rootSize && native.back() == L'\\')
            native.pop_back();
        return std::filesystem::path(std::move(native));
    }

    const std::size_t minSize = rootEnd + 1u;
    if (native.size() < minSize)
    {
        native.push_back(L'\\');
        return std::filesystem::path(std::move(native));
    }
    while (native.size() > minSize && native.back() == L'\\')
        native.pop_back();
    return std::filesystem::path(std::move(native));
}

[[nodiscard]] inline bool EqualNormalizedRootPath(const std::filesystem::path& left, const std::filesystem::path& right) noexcept
{
    return EqualOrdinalIgnoreCase(left.native(), right.native());
}

[[nodiscard]] inline bool SameRootPath(const std::filesystem::path& left, const std::filesystem::path& right) noexcept
try
{
    return EqualOrdinalIgnoreCase(NormalizeRootPath(left).native(), NormalizeRootPath(right).native());
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

[[nodiscard]] inline HRESULT VerifyDiskHandle(HANDLE handle, std::optional<bool> expectDirectory = std::nullopt, bool* outIsSurrogateReparse = nullptr) noexcept
{
    if (outIsSurrogateReparse)
        *outIsSurrogateReparse = false;
    if (! handle || handle == INVALID_HANDLE_VALUE)
        return E_INVALIDARG;
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (! GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &info, sizeof(info)))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_ACCESS_DENIED);
    }
    if (GetFileType(handle) != FILE_TYPE_DISK)
        return E_INVALIDARG;
    // Cloud/provider tags without name redirection keep their provider semantics.
    if ((info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(info.ReparseTag) != FALSE)
    {
        if (outIsSurrogateReparse)
            *outIsSurrogateReparse = true;
        return E_ACCESSDENIED;
    }
    if (expectDirectory.has_value() && ((info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u) != *expectDirectory)
        return E_INVALIDARG;
    return S_OK;
}

struct PathSecurityCheckOptions
{
    bool allowMissingTarget{false};
    bool allowMissingAncestors{false};
    std::optional<bool> expectDirectory;
};

[[nodiscard]] inline bool IsValidWin32PathString(std::wstring_view path) noexcept
{
    return ! path.empty() && path.size() <= 32750u && path.find(L'\0') == path.npos &&
           ! std::ranges::any_of(path, [](wchar_t c) noexcept { return static_cast<unsigned>(c) < 32u; });
}

[[nodiscard]] inline HRESULT VerifyPathNoSurrogateReparse(const std::filesystem::path& path,
                                                          PathSecurityCheckOptions options = {},
                                                          bool* outReparseRejected         = nullptr) noexcept
try
{
    if (outReparseRejected)
        *outReparseRejected = false;
    if (path.empty() || ! path.is_absolute() || ! IsValidWin32PathString(path.native()))
        return E_INVALIDARG;

    std::wstring text = path.native();
    std::ranges::replace(text, L'/', L'\\');

    const std::size_t rootEnd = RootNameEnd(text);
    if (rootEnd == std::wstring::npos)
        return E_INVALIDARG;
    const std::size_t rootWithSlash = rootEnd + 1u;
    if (text.size() < rootWithSlash)
        text.push_back(L'\\');
    while (text.size() > rootWithSlash && text.back() == L'\\')
        text.pop_back();

    const auto checkPrefix = [&](const std::wstring& prefix, bool isFinal) noexcept -> HRESULT
    {
        const wil::unique_hfile handle(CreateFileW(prefix.c_str(),
                                                   FILE_READ_ATTRIBUTES,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                   nullptr,
                                                   OPEN_EXISTING,
                                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                   nullptr));
        if (! handle)
        {
            const DWORD err = GetLastError();
            if (isFinal)
            {
                if (options.allowMissingTarget && (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND))
                    return S_OK;
                return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_ACCESS_DENIED);
            }
            if (options.allowMissingAncestors && (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND))
                return S_OK;
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_ACCESS_DENIED);
        }
        bool surrogate{};
        const std::optional<bool> expectDir = isFinal ? options.expectDirectory : std::optional<bool>(true);
        const HRESULT verifyHr              = VerifyDiskHandle(handle.get(), expectDir, &surrogate);
        if (FAILED(verifyHr))
        {
            if (outReparseRejected)
                *outReparseRejected = surrogate;
            return verifyHr;
        }
        return S_OK;
    };

    std::wstring prefix(text, 0u, rootWithSlash);
    if (text.size() == rootWithSlash)
        return checkPrefix(prefix, true);

    std::size_t at = rootWithSlash;
    while (at < text.size())
    {
        const std::size_t slash = text.find(L'\\', at);
        const std::size_t end   = slash == text.npos ? text.size() : slash;
        const std::wstring_view part(text.data() + at, end - at);
        if (part.empty() || part == L"." || part == L"..")
            return E_INVALIDARG;
        if (prefix.back() != L'\\')
            prefix.push_back(L'\\');
        prefix.append(part);
        const HRESULT stepHr = checkPrefix(prefix, end == text.size());
        if (FAILED(stepHr))
            return stepHr;
        at = end + 1u;
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

// Component-by-component pinned-parent opener: pins every visited ancestor handle
// while descending so no parent can be replaced by a symlink/junction mid-open.
[[nodiscard]] inline HRESULT OpenPinnedDiskEntry(std::wstring_view path,
                                                 bool expectDirectory,
                                                 wil::unique_handle& output,
                                                 DWORD fileShareMode      = FILE_SHARE_READ,
                                                 bool* outReparseRejected = nullptr) noexcept
try
{
    output.reset();
    if (outReparseRejected)
        *outReparseRejected = false;
    if (! IsValidWin32PathString(path))
        return E_INVALIDARG;
    std::wstring text(path);
    std::ranges::replace(text, L'/', L'\\');
    if (text.starts_with(L"\\\\?\\UNC\\") || text.starts_with(L"\\\\?\\unc\\"))
        text.replace(0u, 8u, L"\\\\");
    else if (text.starts_with(L"\\\\?\\"))
        text.erase(0u, 4u);

    std::size_t rootEnd{};
    if (text.size() >= 3u && ((text[0] >= L'A' && text[0] <= L'Z') || (text[0] >= L'a' && text[0] <= L'z')) && text[1] == L':' && text[2] == L'\\')
        rootEnd = 2u;
    else if (const auto share = UncShareEnd(text); share != std::wstring_view::npos)
        rootEnd = share;
    else
        return E_INVALIDARG;

    while (text.size() > rootEnd + 1u && text.back() == L'\\')
        text.pop_back();
    if (text.size() <= rootEnd + 1u && ! expectDirectory)
        return E_INVALIDARG;

    const bool unc      = text.starts_with(L"\\\\");
    std::wstring prefix = unc ? L"\\\\?\\UNC\\" : L"\\\\?\\";
    prefix.append(text, unc ? 2u : 0u, rootEnd - (unc ? 2u : 0u));
    prefix.push_back(L'\\');

    std::array<wil::unique_handle, 128u> parents;
    std::size_t count{};
    const auto open = [&](bool final) noexcept -> HRESULT
    {
        const bool isDirectory = ! final || expectDirectory;
        const DWORD access     = isDirectory ? FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES : GENERIC_READ;
        const DWORD share      = final && expectDirectory ? (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE) : fileShareMode;
        const DWORD flags      = FILE_FLAG_OPEN_REPARSE_POINT | (isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_FLAG_SEQUENTIAL_SCAN) |
                                 (final && expectDirectory ? FILE_FLAG_OVERLAPPED : 0u);
        wil::unique_handle file(CreateFileW(prefix.c_str(), access, share, nullptr, OPEN_EXISTING, flags, nullptr));
        if (! file)
        {
            const DWORD err = GetLastError();
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_ACCESS_DENIED);
        }
        bool surrogate{};
        const HRESULT verifyHr = VerifyDiskHandle(file.get(), isDirectory, &surrogate);
        if (FAILED(verifyHr))
        {
            if (outReparseRejected && surrogate)
                *outReparseRejected = true;
            return verifyHr;
        }
        if (final)
            output = std::move(file);
        else
        {
            if (count == parents.size())
                return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            parents[count++] = std::move(file);
        }
        return S_OK;
    };

    if (text.size() <= rootEnd + 1u)
        return open(true);
    {
        const HRESULT rootHr = open(false);
        if (FAILED(rootHr))
            return rootHr;
    }
    std::size_t at = rootEnd + 1u;
    while (at < text.size())
    {
        const auto slash = text.find(L'\\', at);
        const auto end   = slash == text.npos ? text.size() : slash;
        const auto part  = std::wstring_view(text).substr(at, end - at);
        if (part.empty() || part == L"." || part == L".." || part.find_first_of(L":<>\"|?*") != part.npos)
            return E_INVALIDARG;
        if (prefix.back() != L'\\')
            prefix.push_back(L'\\');
        prefix.append(part);
        const HRESULT stepHr = open(end == text.size());
        if (FAILED(stepHr))
            return stepHr;
        at = end + 1u;
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

// Ensures every directory component exists without traversing surrogate reparse points,
// pinning each visited/created ancestor handle while descending.
[[nodiscard]] inline HRESULT EnsurePinnedDiskDirectory(const std::filesystem::path& directory,
                                                       wil::unique_handle& pinnedDir,
                                                       bool* outReparseRejected = nullptr) noexcept
try
{
    pinnedDir.reset();
    if (outReparseRejected)
        *outReparseRejected = false;
    if (directory.empty() || ! directory.is_absolute() || ! IsValidWin32PathString(directory.native()))
        return E_INVALIDARG;

    std::wstring text = directory.native();
    std::ranges::replace(text, L'/', L'\\');
    if (text.starts_with(L"\\\\?\\UNC\\") || text.starts_with(L"\\\\?\\unc\\"))
        text.replace(0u, 8u, L"\\\\");
    else if (text.starts_with(L"\\\\?\\"))
        text.erase(0u, 4u);

    std::size_t rootEnd{};
    if (text.size() >= 3u && ((text[0] >= L'A' && text[0] <= L'Z') || (text[0] >= L'a' && text[0] <= L'z')) && text[1] == L':' && text[2] == L'\\')
        rootEnd = 2u;
    else if (const auto share = UncShareEnd(text); share != std::wstring_view::npos)
        rootEnd = share;
    else
        return E_INVALIDARG;

    while (text.size() > rootEnd + 1u && text.back() == L'\\')
        text.pop_back();

    const bool unc      = text.starts_with(L"\\\\");
    std::wstring prefix = unc ? L"\\\\?\\UNC\\" : L"\\\\?\\";
    prefix.append(text, unc ? 2u : 0u, rootEnd - (unc ? 2u : 0u));
    prefix.push_back(L'\\');

    std::array<wil::unique_handle, 128u> parents;
    std::size_t count{};
    const auto openOrCreate = [&](bool isFinal, bool allowCreate) noexcept -> HRESULT
    {
        wil::unique_handle dir(CreateFileW(prefix.c_str(),
                                           FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           nullptr,
                                           OPEN_EXISTING,
                                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                                           nullptr));
        if (! dir)
        {
            const DWORD err = GetLastError();
            if (allowCreate && (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND))
            {
                if (! CreateDirectoryW(prefix.c_str(), nullptr))
                {
                    const DWORD createErr = GetLastError();
                    if (createErr != ERROR_ALREADY_EXISTS)
                        return HRESULT_FROM_WIN32(createErr != ERROR_SUCCESS ? createErr : ERROR_ACCESS_DENIED);
                }
                dir.reset(CreateFileW(prefix.c_str(),
                                      FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                                      nullptr));
                if (! dir)
                {
                    const DWORD reopenErr = GetLastError();
                    return HRESULT_FROM_WIN32(reopenErr != ERROR_SUCCESS ? reopenErr : ERROR_ACCESS_DENIED);
                }
            }
            else
            {
                return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_ACCESS_DENIED);
            }
        }
        bool surrogate{};
        const HRESULT verifyHr = VerifyDiskHandle(dir.get(), true, &surrogate);
        if (FAILED(verifyHr))
        {
            if (outReparseRejected && surrogate)
                *outReparseRejected = true;
            return verifyHr;
        }
        if (isFinal)
            pinnedDir = std::move(dir);
        else
        {
            if (count == parents.size())
                return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            parents[count++] = std::move(dir);
        }
        return S_OK;
    };

    if (text.size() <= rootEnd + 1u)
        return openOrCreate(true, false);
    {
        const HRESULT rootHr = openOrCreate(false, false);
        if (FAILED(rootHr))
            return rootHr;
    }
    std::size_t at = rootEnd + 1u;
    while (at < text.size())
    {
        const auto slash = text.find(L'\\', at);
        const auto end   = slash == text.npos ? text.size() : slash;
        const auto part  = std::wstring_view(text).substr(at, end - at);
        if (part.empty() || part == L"." || part == L".." || part.find_first_of(L":<>\"|?*") != part.npos)
            return E_INVALIDARG;
        if (prefix.back() != L'\\')
            prefix.push_back(L'\\');
        prefix.append(part);
        const HRESULT stepHr = openOrCreate(end == text.size(), true);
        if (FAILED(stepHr))
            return stepHr;
        at = end + 1u;
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
} // namespace RedTilde
