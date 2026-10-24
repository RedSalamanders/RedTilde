#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <new>
#include <string>
#include <wil/resource.h>
#include <winioctl.h>

namespace RedTilde::Testing
{
namespace FilesFixtureDetail
{
inline bool Within(const std::filesystem::path &root, const std::filesystem::path &path)
{
    auto prefix = root.native();
    if (prefix.empty() || (prefix.back() != L'\\' && prefix.back() != L'/'))
        prefix += L'\\';
    const auto &name = path.native();
    return name.size() > prefix.size() &&
           CompareStringOrdinal(prefix.data(), static_cast<int>(prefix.size()), name.data(), static_cast<int>(prefix.size()), TRUE) == CSTR_EQUAL;
}
inline HRESULT DirectoryChain(const std::filesystem::path &root, const std::filesystem::path &directory)
{
    auto current = root;
    const auto check = [](const std::filesystem::path &path)
    {
        const DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return HRESULT_FROM_WIN32(GetLastError());
        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        return S_OK;
    };
    HRESULT result = check(current);
    if (FAILED(result))
        return result;
    for (const auto &component : directory.lexically_relative(root))
    {
        if (component == L".")
            continue;
        if (component == L"..")
            return E_INVALIDARG;
        current /= component;
        result = check(current);
        if (FAILED(result))
            return result;
    }
    return S_OK;
}
} // namespace FilesFixtureDetail

// The harness and native cases share this fixture-only creator. Both the link
// and target must be strict descendants of the supplied existing fixture root;
// reparse ancestors within it are rejected. The caller removes only its link
// leaf (never a recursive traversal) or retains the complete owned fixture.
[[nodiscard]] inline HRESULT CreateFixtureJunction(const std::filesystem::path &fixtureRoot, const std::filesystem::path &linkPath,
                                                   const std::filesystem::path &targetPath) noexcept
try
{
    auto root = fixtureRoot.lexically_normal(), link = linkPath.lexically_normal(), target = targetPath.lexically_normal();
    root.make_preferred();
    link.make_preferred();
    target.make_preferred();
    if (!root.is_absolute() || !link.is_absolute() || !target.is_absolute() || !FilesFixtureDetail::Within(root, link) ||
        !FilesFixtureDetail::Within(root, target) || root.native().find(L'\0') != std::wstring::npos ||
        link.native().find(L'\0') != std::wstring::npos || target.native().find(L'\0') != std::wstring::npos)
        return E_INVALIDARG;
    HRESULT result = FilesFixtureDetail::DirectoryChain(root, link.parent_path());
    if (FAILED(result))
        return result;
    result = FilesFixtureDetail::DirectoryChain(root, target);
    if (FAILED(result))
        return result;
    struct MountPoint
    {
        DWORD tag;
        WORD length, reserved;
        WORD substituteOffset, substituteLength, printOffset, printLength;
        std::array<wchar_t, 2048> names;
    };
    static_assert(offsetof(MountPoint, names) == 16);
    MountPoint data{};
    const auto substitute = L"\\??\\" + target.native();
    const auto printed = target.native();
    if (substitute.size() + printed.size() + 2 >= data.names.size())
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    data.substituteLength = static_cast<WORD>(substitute.size() * sizeof(wchar_t));
    data.printOffset = static_cast<WORD>((substitute.size() + 1) * sizeof(wchar_t));
    data.printLength = static_cast<WORD>(printed.size() * sizeof(wchar_t));
    data.length = static_cast<WORD>(8 + (substitute.size() + printed.size() + 2) * sizeof(wchar_t));
    std::ranges::copy(substitute, data.names.begin());
    std::ranges::copy(printed, data.names.begin() + static_cast<std::ptrdiff_t>(substitute.size() + 1));
    if (!CreateDirectoryW(link.c_str(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    auto removeEmptyOnFailure = wil::scope_exit([&] { (void)RemoveDirectoryW(link.c_str()); });
    wil::unique_handle directory(
        CreateFileW(link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!directory)
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD returned{};
    if (!DeviceIoControl(directory.get(), FSCTL_SET_REPARSE_POINT, &data, static_cast<DWORD>(8 + data.length), nullptr, 0, &returned, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    removeEmptyOnFailure.release();
    return S_OK;
}
catch (const std::bad_alloc &)
{
    std::terminate(); // Harness boundary: allocation failure is fatal, never a silently missing reparse fixture.
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(error.code().value());
}
} // namespace RedTilde::Testing
