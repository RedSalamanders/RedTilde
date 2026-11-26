#include "FileOps.h"
#include "Files.h"
#include "MonitorDiagnostics.h"
#include "PathSecurity.h"
#include "ProcessWork.h"
#include "Storage.h"
#include "TextUtilities.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <format>
#include <mutex>
#include <shellapi.h>
#include <shlobj.h>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
constexpr std::size_t MaximumClipboardPaths    = 64u;
constexpr wchar_t PreferredDropEffectFormat[]  = L"Preferred DropEffect";
constexpr unsigned MaximumCopyDepth            = 8u;
constexpr std::size_t MaximumCopyEntries       = 5000u;
constexpr std::uint64_t MaximumCopyBytes       = 64u * 1024u * 1024u;
constexpr std::size_t DirectoryEnumBufferBytes = 16u * 1024u;
constexpr std::size_t FileCopyChunkBytes       = 64u * 1024u;

struct FileObjectId
{
    ULONGLONG volume{};
    ULONGLONG idLow{};
    ULONGLONG idHigh{};
};

std::uint64_t VolumeRootFingerprintForId(HANDLE file) noexcept
{
    wchar_t buffer[1024]{};
    const DWORD length = GetFinalPathNameByHandleW(file, buffer, static_cast<DWORD>(std::size(buffer)), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (length == 0u || length >= std::size(buffer))
        return 0u;
    const std::wstring_view path(buffer, length);
    std::wstring_view prefix;
    std::size_t lowerLimit{};
    if (path.starts_with(L"\\\\?\\UNC\\"))
    {
        const std::size_t serverEnd = path.find(L'\\', 8u);
        if (serverEnd == path.npos || serverEnd <= 8u || serverEnd + 1u >= path.size())
            return 0u;
        std::size_t shareEnd = path.find(L'\\', serverEnd + 1u);
        if (shareEnd == path.npos)
            shareEnd = path.size();
        if (shareEnd <= serverEnd + 1u)
            return 0u;
        prefix     = path.substr(0u, shareEnd);
        lowerLimit = serverEnd;
    }
    else if (path.starts_with(L"\\\\?\\") && path.size() >= 6u && path[5u] == L':')
    {
        prefix     = path.substr(0u, 6u);
        lowerLimit = 5u;
    }
    else
        return 0u;
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t index = 0u; index < prefix.size(); ++index)
    {
        wchar_t ch = prefix[index];
        if (index < lowerLimit && ch >= L'A' && ch <= L'Z')
            ch = static_cast<wchar_t>(ch + (L'a' - L'A'));
        const auto unit = static_cast<std::uint16_t>(ch);
        hash ^= static_cast<unsigned char>(unit & 0xFFu);
        hash *= 1099511628211ull;
        hash ^= static_cast<unsigned char>((unit >> 8u) & 0xFFu);
        hash *= 1099511628211ull;
    }
    return hash ? hash : 1ull;
}

HRESULT ReadFileObjectId(HANDLE handle, FileObjectId& outId) noexcept
{
    outId = {};
    FILE_ID_INFO info{};
    if (GetFileInformationByHandleEx(handle, FileIdInfo, &info, sizeof(info)))
    {
        outId.volume = info.VolumeSerialNumber;
        std::memcpy(&outId.idLow, info.FileId.Identifier, sizeof(outId.idLow));
        std::memcpy(&outId.idHigh, info.FileId.Identifier + sizeof(outId.idLow), sizeof(outId.idHigh));
        if (outId.volume == 0u && (outId.idLow != 0u || outId.idHigh != 0u))
            outId.volume = VolumeRootFingerprintForId(handle);
        return (outId.volume != 0u && (outId.idLow != 0u || outId.idHigh != 0u)) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    const DWORD err = GetLastError();
    if (err != ERROR_INVALID_PARAMETER && err != ERROR_NOT_SUPPORTED && err != ERROR_INVALID_FUNCTION)
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    BY_HANDLE_FILE_INFORMATION legacy{};
    if (! GetFileInformationByHandle(handle, &legacy))
    {
        const DWORD legacyErr = GetLastError();
        return HRESULT_FROM_WIN32(legacyErr != ERROR_SUCCESS ? legacyErr : ERROR_GEN_FAILURE);
    }
    outId.volume = legacy.dwVolumeSerialNumber;
    outId.idLow  = (static_cast<ULONGLONG>(legacy.nFileIndexHigh) << 32u) | legacy.nFileIndexLow;
    outId.idHigh = 0u;
    if (outId.volume == 0u && outId.idLow != 0u)
        outId.volume = VolumeRootFingerprintForId(handle);
    return (outId.volume != 0u && outId.idLow != 0u) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}

bool SameObject(const FileObjectId& left, const FileObjectId& right) noexcept
{
    return left.volume != 0u && (left.idLow != 0u || left.idHigh != 0u) && left.volume == right.volume && left.idLow == right.idLow &&
           left.idHigh == right.idHigh;
}

bool HasAnyReparseAncestorOrSelf(const std::filesystem::path& path) noexcept
{
    return FAILED(VerifyPathNoSurrogateReparse(path, {.allowMissingTarget = true, .allowMissingAncestors = false}));
}

bool CheckNoSurrogateReparse(const std::filesystem::path& path, bool& outReparseRejected, HRESULT& outStatus, bool allowMissingTarget = true) noexcept
{
    bool surrogate{};
    const HRESULT hr = VerifyPathNoSurrogateReparse(path, {.allowMissingTarget = allowMissingTarget, .allowMissingAncestors = false}, &surrogate);
    if (FAILED(hr))
    {
        outReparseRejected = surrogate;
        outStatus          = surrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : hr;
        return false;
    }
    return true;
}

bool IsReservedDosName(std::wstring_view leaf) noexcept
{
    const auto dot = leaf.find(L'.');
    auto stem      = dot == std::wstring_view::npos ? leaf : leaf.substr(0, dot);
    while (! stem.empty() && stem.back() == L' ')
        stem.remove_suffix(1u);
    static constexpr std::array<std::wstring_view, 22> Reserved{L"CON",  L"PRN",  L"AUX",  L"NUL",  L"COM1", L"COM2", L"COM3", L"COM4",
                                                                L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3",
                                                                L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"};
    return std::ranges::any_of(Reserved, [&](std::wstring_view candidate) { return EqualOrdinalIgnoreCase(stem, candidate); });
}

bool IsSupportedWorkspaceFileName(std::wstring_view name) noexcept
{
    const auto slash = name.find_last_of(L"\\/");
    const auto leaf  = slash == std::wstring_view::npos ? name : name.substr(slash + 1u);
    const auto dot   = leaf.find_last_of(L'.');
    if (dot == std::wstring_view::npos)
        return false;
    const auto ext = leaf.substr(dot);
    return EqualOrdinalIgnoreCase(ext, L".md") || EqualOrdinalIgnoreCase(ext, L".markdown") || EqualOrdinalIgnoreCase(ext, L".txt");
}

bool IsSameOrDescendantPath(const std::filesystem::path& parentDir, const std::filesystem::path& candidateDir)
{
    const auto normParent    = NormalizeRootPath(parentDir);
    const auto normCandidate = NormalizeRootPath(candidateDir);
    const auto& parentStr    = normParent.native();
    const auto& candidateStr = normCandidate.native();
    if (parentStr.empty() || candidateStr.empty())
        return false;
    if (EqualOrdinalIgnoreCase(candidateStr, parentStr))
        return true;
    if (parentStr.ends_with(L'\\'))
        return StartsWithOrdinalIgnoreCase(candidateStr, parentStr);
    return candidateStr.size() > parentStr.size() && StartsWithOrdinalIgnoreCase(candidateStr, parentStr) && candidateStr[parentStr.size()] == L'\\';
}

bool IsCancelled(const std::atomic_bool* cancel) noexcept
{
    return cancel && cancel->load(std::memory_order_acquire);
}

HRESULT CopyFileByHandles(const std::filesystem::path& sourcePath,
                          const std::filesystem::path& destPath,
                          bool replaceExisting,
                          const std::atomic_bool* cancel,
                          std::vector<std::byte>& copyBuffer,
                          bool& outCollision,
                          bool& outReparseRejected) noexcept
try
{
    if (IsCancelled(cancel))
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    const wil::unique_hfile srcFile(CreateFileW(sourcePath.c_str(),
                                                GENERIC_READ,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                nullptr,
                                                OPEN_EXISTING,
                                                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN,
                                                nullptr));
    if (! srcFile)
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    bool srcSurrogate{};
    const HRESULT srcVerify = VerifyDiskHandle(srcFile.get(), false, &srcSurrogate);
    if (FAILED(srcVerify))
    {
        if (srcSurrogate)
            outReparseRejected = true;
        return srcSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : srcVerify;
    }

    bool createdNew = true;
    wil::unique_hfile dstFile(CreateFileW(destPath.c_str(),
                                          GENERIC_WRITE | FILE_READ_ATTRIBUTES | DELETE,
                                          0u,
                                          nullptr,
                                          CREATE_NEW,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                                          nullptr));
    if (! dstFile)
    {
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_EXISTS || err == ERROR_ALREADY_EXISTS)
        {
            if (! replaceExisting)
            {
                outCollision = true;
                return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
            createdNew = false;
            dstFile.reset(CreateFileW(destPath.c_str(),
                                      GENERIC_WRITE | FILE_READ_ATTRIBUTES,
                                      0u,
                                      nullptr,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                                      nullptr));
            if (! dstFile)
            {
                const DWORD openErr = GetLastError();
                return HRESULT_FROM_WIN32(openErr != ERROR_SUCCESS ? openErr : ERROR_GEN_FAILURE);
            }
            bool dstSurrogate{};
            const HRESULT dstVerify = VerifyDiskHandle(dstFile.get(), false, &dstSurrogate);
            if (FAILED(dstVerify))
            {
                if (dstSurrogate)
                    outReparseRejected = true;
                return dstSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : dstVerify;
            }
            const LARGE_INTEGER zero{};
            if (! SetFilePointerEx(dstFile.get(), zero, nullptr, FILE_BEGIN) || ! SetEndOfFile(dstFile.get()))
            {
                const DWORD truncErr = GetLastError();
                return HRESULT_FROM_WIN32(truncErr != ERROR_SUCCESS ? truncErr : ERROR_GEN_FAILURE);
            }
        }
        else
        {
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
    }

    if (copyBuffer.size() < FileCopyChunkBytes)
        copyBuffer.resize(FileCopyChunkBytes);

    const auto retireCreatedOnFailure = wil::scope_exit([&]() noexcept
    {
        if (createdNew && dstFile)
        {
            FILE_DISPOSITION_INFO disp{TRUE};
            (void)SetFileInformationByHandle(dstFile.get(), FileDispositionInfo, &disp, sizeof(disp));
        }
    });

    for (;;)
    {
        if (IsCancelled(cancel))
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        DWORD bytesRead{};
        if (! ReadFile(srcFile.get(), copyBuffer.data(), static_cast<DWORD>(copyBuffer.size()), &bytesRead, nullptr))
        {
            const DWORD readErr = GetLastError();
            return HRESULT_FROM_WIN32(readErr != ERROR_SUCCESS ? readErr : ERROR_GEN_FAILURE);
        }
        if (bytesRead == 0u)
            break;
        DWORD totalWritten{};
        while (totalWritten < bytesRead)
        {
            if (IsCancelled(cancel))
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            DWORD written{};
            if (! WriteFile(dstFile.get(), copyBuffer.data() + totalWritten, bytesRead - totalWritten, &written, nullptr) || written == 0u)
            {
                const DWORD writeErr = GetLastError();
                return HRESULT_FROM_WIN32(writeErr != ERROR_SUCCESS ? writeErr : ERROR_GEN_FAILURE);
            }
            totalWritten += written;
        }
    }

    createdNew = false;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

struct PlannedFileCopy
{
    std::filesystem::path source;
    std::filesystem::path destination;
};

struct DirectoryCopyPlan
{
    std::vector<std::filesystem::path> directories;
    std::vector<PlannedFileCopy> files;
    std::size_t examinedEntries{};
    std::uint64_t totalBytes{};
    std::vector<std::byte> dirBuffer = std::vector<std::byte>(DirectoryEnumBufferBytes);
};

HRESULT PreflightDirectoryRecursive(const std::filesystem::path& sourceDir,
                                    const std::filesystem::path& destDir,
                                    unsigned depth,
                                    bool replaceExisting,
                                    bool requireAllFilesSupported,
                                    const std::atomic_bool* cancel,
                                    DirectoryCopyPlan& plan,
                                    bool& outReparseRejected)
{
    if (IsCancelled(cancel))
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    const wil::unique_hfile dirHandle(CreateFileW(sourceDir.c_str(),
                                                  FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
    if (! dirHandle)
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    bool srcSurrogate{};
    const HRESULT verifyHr = VerifyDiskHandle(dirHandle.get(), true, &srcSurrogate);
    if (FAILED(verifyHr))
    {
        if (srcSurrogate)
            outReparseRejected = true;
        return srcSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : verifyHr;
    }

    // Verify destination directory entry if it already exists (must not be a surrogate reparse or regular file).
    const wil::unique_hfile existingDst(CreateFileW(destDir.c_str(),
                                                    FILE_READ_ATTRIBUTES,
                                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                    nullptr,
                                                    OPEN_EXISTING,
                                                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                    nullptr));
    const bool destDirExists = static_cast<bool>(existingDst);
    if (destDirExists)
    {
        bool dstSurrogate{};
        const HRESULT dstVerify = VerifyDiskHandle(existingDst.get(), true, &dstSurrogate);
        if (FAILED(dstVerify))
        {
            if (dstSurrogate)
                outReparseRejected = true;
            return dstSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : dstVerify;
        }
        if (! replaceExisting)
            return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }
    else
    {
        const DWORD dstErr = GetLastError();
        if (dstErr != ERROR_FILE_NOT_FOUND && dstErr != ERROR_PATH_NOT_FOUND)
            return HRESULT_FROM_WIN32(dstErr != ERROR_SUCCESS ? dstErr : ERROR_GEN_FAILURE);
    }

    plan.directories.push_back(destDir);

    std::vector<std::filesystem::path> childDirs;
    bool first = true;
    for (;;)
    {
        if (IsCancelled(cancel))
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        const auto infoClass = first ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
        first                = false;
        if (! GetFileInformationByHandleEx(dirHandle.get(), infoClass, plan.dirBuffer.data(), static_cast<DWORD>(plan.dirBuffer.size())))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES)
                break;
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
        std::size_t offset{};
        for (;;)
        {
            if (IsCancelled(cancel))
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            const auto* entry = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO*>(plan.dirBuffer.data() + offset);
            const std::wstring_view name(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
            if (name != L"." && name != L"..")
            {
                if (plan.examinedEntries >= MaximumCopyEntries)
                    return HRESULT_FROM_WIN32(ERROR_TOO_MANY_NAMES);
                ++plan.examinedEntries;
                if ((entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(entry->EaSize) != FALSE)
                {
                    outReparseRejected = true;
                    return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
                }
                const auto childSrc = sourceDir / name;
                const auto childDst = destDir / name;
                if ((entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
                {
                    if (depth >= MaximumCopyDepth)
                        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                    childDirs.push_back(std::filesystem::path(name));
                }
                else
                {
                    if (! IsSupportedWorkspaceFileName(name))
                    {
                        if (requireAllFilesSupported)
                            return HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
                    }
                    else
                    {
                        const wil::unique_hfile fileHandle(CreateFileW(childSrc.c_str(),
                                                                       FILE_READ_ATTRIBUTES,
                                                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                                       nullptr,
                                                                       OPEN_EXISTING,
                                                                       FILE_FLAG_OPEN_REPARSE_POINT,
                                                                       nullptr));
                        if (! fileHandle)
                        {
                            const DWORD fileErr = GetLastError();
                            return HRESULT_FROM_WIN32(fileErr != ERROR_SUCCESS ? fileErr : ERROR_GEN_FAILURE);
                        }
                        bool fileSurrogate{};
                        const HRESULT fileVerify = VerifyDiskHandle(fileHandle.get(), false, &fileSurrogate);
                        if (FAILED(fileVerify))
                        {
                            if (fileSurrogate)
                                outReparseRejected = true;
                            return fileSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : fileVerify;
                        }
                        FILE_STANDARD_INFO stdInfo{};
                        if (! GetFileInformationByHandleEx(fileHandle.get(), FileStandardInfo, &stdInfo, sizeof(stdInfo)))
                        {
                            const DWORD stdErr = GetLastError();
                            return HRESULT_FROM_WIN32(stdErr != ERROR_SUCCESS ? stdErr : ERROR_GEN_FAILURE);
                        }
                        const std::uint64_t sizeBytes = stdInfo.EndOfFile.QuadPart > 0 ? static_cast<std::uint64_t>(stdInfo.EndOfFile.QuadPart) : 0ull;
                        if (sizeBytes > MaximumCopyBytes || plan.totalBytes > MaximumCopyBytes - sizeBytes)
                            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
                        plan.totalBytes += sizeBytes;
                        if (destDirExists)
                        {
                            bool dstFileReparse{};
                            HRESULT dstFileHr = S_OK;
                            if (! CheckNoSurrogateReparse(childDst, dstFileReparse, dstFileHr, true))
                            {
                                if (dstFileReparse)
                                    outReparseRejected = true;
                                return dstFileHr;
                            }
                        }
                        plan.files.push_back({childSrc, childDst});
                    }
                }
            }
            if (entry->NextEntryOffset == 0u)
                break;
            offset += entry->NextEntryOffset;
        }
    }

    for (const auto& leaf : childDirs)
    {
        const HRESULT childHr = PreflightDirectoryRecursive(
            sourceDir / leaf, destDir / leaf, depth + 1u, replaceExisting, requireAllFilesSupported, cancel, plan, outReparseRejected);
        if (FAILED(childHr))
            return childHr;
    }
    return S_OK;
}

HRESULT ExecuteDirectoryCopyPlan(
    const DirectoryCopyPlan& plan, bool replaceExisting, const std::atomic_bool* cancel, bool& outCollision, bool& outReparseRejected)
{
    for (const auto& dir : plan.directories)
    {
        if (IsCancelled(cancel))
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        if (! CreateDirectoryW(dir.c_str(), nullptr))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS && replaceExisting)
            {
                const wil::unique_hfile existing(CreateFileW(dir.c_str(),
                                                             FILE_READ_ATTRIBUTES,
                                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                             nullptr,
                                                             OPEN_EXISTING,
                                                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                             nullptr));
                if (! existing)
                {
                    const DWORD openErr = GetLastError();
                    return HRESULT_FROM_WIN32(openErr != ERROR_SUCCESS ? openErr : ERROR_GEN_FAILURE);
                }
                bool surrogate{};
                const HRESULT verifyHr = VerifyDiskHandle(existing.get(), true, &surrogate);
                if (FAILED(verifyHr))
                {
                    if (surrogate)
                        outReparseRejected = true;
                    return surrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : verifyHr;
                }
            }
            else
            {
                return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
            }
        }
    }
    std::vector<std::byte> copyBuffer(FileCopyChunkBytes);
    for (const auto& item : plan.files)
    {
        const HRESULT copyHr = CopyFileByHandles(item.source, item.destination, replaceExisting, cancel, copyBuffer, outCollision, outReparseRejected);
        if (FAILED(copyHr))
            return copyHr;
    }
    return S_OK;
}

HRESULT DeleteDirectoryBounded(const std::filesystem::path& dirPath,
                               unsigned depth,
                               std::size_t& remainingEntries,
                               std::vector<std::byte>& dirBuffer,
                               const std::atomic_bool* cancel,
                               bool& outReparseRejected)
{
    if (IsCancelled(cancel))
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (depth > MaximumCopyDepth)
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    const wil::unique_hfile dirHandle(CreateFileW(dirPath.c_str(),
                                                  FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
    if (! dirHandle)
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    bool dirSurrogate{};
    const HRESULT dirVerify = VerifyDiskHandle(dirHandle.get(), true, &dirSurrogate);
    if (FAILED(dirVerify))
    {
        if (dirSurrogate)
            outReparseRejected = true;
        return dirSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : dirVerify;
    }

    std::vector<std::filesystem::path> subDirs;
    std::vector<std::filesystem::path> childFiles;
    bool first = true;
    for (;;)
    {
        const auto infoClass = first ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
        first                = false;
        if (! GetFileInformationByHandleEx(dirHandle.get(), infoClass, dirBuffer.data(), static_cast<DWORD>(dirBuffer.size())))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES)
                break;
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
        std::size_t offset{};
        for (;;)
        {
            const auto* entry = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO*>(dirBuffer.data() + offset);
            const std::wstring_view name(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
            if (name != L"." && name != L"..")
            {
                if (remainingEntries == 0u)
                    return HRESULT_FROM_WIN32(ERROR_TOO_MANY_NAMES);
                --remainingEntries;
                if ((entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(entry->EaSize) != FALSE)
                {
                    outReparseRejected = true;
                    return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
                }
                const auto child = dirPath / name;
                if ((entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
                    subDirs.push_back(child);
                else
                    childFiles.push_back(child);
            }
            if (entry->NextEntryOffset == 0u)
                break;
            offset += entry->NextEntryOffset;
        }
    }

    for (const auto& file : childFiles)
    {
        const wil::unique_hfile h(CreateFileW(file.c_str(),
                                              FILE_READ_ATTRIBUTES,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              nullptr,
                                              OPEN_EXISTING,
                                              FILE_FLAG_OPEN_REPARSE_POINT,
                                              nullptr));
        if (! h)
            return HRESULT_FROM_WIN32(GetLastError());
        bool fileSurrogate{};
        const HRESULT verifyHr = VerifyDiskHandle(h.get(), false, &fileSurrogate);
        if (FAILED(verifyHr))
        {
            if (fileSurrogate)
                outReparseRejected = true;
            return fileSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : verifyHr;
        }
        if (! DeleteFileW(file.c_str()))
        {
            const DWORD err = GetLastError();
            return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
    }
    for (const auto& sub : subDirs)
    {
        const HRESULT subHr = DeleteDirectoryBounded(sub, depth + 1u, remainingEntries, dirBuffer, cancel, outReparseRejected);
        if (FAILED(subHr))
            return subHr;
    }
    if (! RemoveDirectoryW(dirPath.c_str()))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    return S_OK;
}

std::optional<std::filesystem::path> TryAdmitExecutablePath(const std::filesystem::path& candidate) noexcept
{
    if (candidate.empty() || ! candidate.is_absolute())
        return std::nullopt;
    const auto norm = candidate.lexically_normal();
    if (! EqualOrdinalIgnoreCase(norm.extension().native(), L".exe"))
        return std::nullopt;
    if (FAILED(VerifyPathNoSurrogateReparse(norm, {.allowMissingTarget = false, .allowMissingAncestors = false, .expectDirectory = false})))
        return std::nullopt;
    return norm;
}

std::optional<std::filesystem::path> ReadAppPathsRegistry(HKEY rootKey) noexcept
{
    std::array<wchar_t, 32768> buffer{};
    DWORD bytes          = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
    DWORD type           = 0u;
    const LSTATUS status = RegGetValueW(rootKey,
                                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\RedSalamander.exe",
                                        nullptr,
                                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ,
                                        &type,
                                        buffer.data(),
                                        &bytes);
    if (status != ERROR_SUCCESS || bytes < sizeof(wchar_t))
        return std::nullopt;
    buffer.back() = L'\0';
    return TryAdmitExecutablePath(std::filesystem::path(buffer.data()));
}
} // namespace

std::wstring QuoteWindowsCliArgument(std::wstring_view argument)
{
    if (argument.empty())
        return L"\"\"";
    const bool needsQuotes = argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (! needsQuotes)
        return std::wstring(argument);
    std::wstring out;
    out.push_back(L'"');
    std::size_t backslashes{};
    for (const wchar_t ch : argument)
    {
        if (ch == L'\\')
        {
            ++backslashes;
        }
        else if (ch == L'"')
        {
            out.append(backslashes * 2u + 1u, L'\\');
            out.push_back(L'"');
            backslashes = 0u;
        }
        else
        {
            out.append(backslashes, L'\\');
            out.push_back(ch);
            backslashes = 0u;
        }
    }
    out.append(backslashes * 2u, L'\\');
    out.push_back(L'"');
    return out;
}

std::optional<std::filesystem::path> WindowsShellReveal::FindRedSalamander(const std::optional<std::filesystem::path>& overridePath) const noexcept
try
{
    if (overridePath)
        if (auto admitted = TryAdmitExecutablePath(*overridePath))
            return admitted;
    if (auto hkcu = ReadAppPathsRegistry(HKEY_CURRENT_USER))
        return hkcu;
    if (auto hklm = ReadAppPathsRegistry(HKEY_LOCAL_MACHINE))
        return hklm;
    wil::unique_cotaskmem_string localAppData;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)) && localAppData)
    {
        const std::filesystem::path base(localAppData.get());
        if (auto perUser = TryAdmitExecutablePath(base / L"Programs" / L"RedSalamander" / L"RedSalamander.exe"))
            return perUser;
        if (auto launcher = TryAdmitExecutablePath(base / L"Programs" / L"RedSalamander" / L"RedLauncher.exe"))
            return launcher;
    }
    wil::unique_cotaskmem_string programFiles;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &programFiles)) && programFiles)
    {
        if (auto pf = TryAdmitExecutablePath(std::filesystem::path(programFiles.get()) / L"RedSalamander" / L"RedSalamander.exe"))
            return pf;
    }
    return std::nullopt;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsShellReveal::RevealInExplorer(const std::filesystem::path& targetPath) noexcept
try
{
    if (targetPath.empty() || ! targetPath.is_absolute() || HasAnyReparseAncestorOrSelf(targetPath))
        return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);

    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool comOwned  = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
        return initHr;
    const auto uninit = wil::scope_exit([&]() noexcept
    {
        if (comOwned)
            CoUninitialize();
    });

    const auto norm   = targetPath.lexically_normal();
    const DWORD attrs = GetFileAttributesW(norm.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());

    const wil::unique_any<LPITEMIDLIST, decltype(&CoTaskMemFree), CoTaskMemFree> itemId(ILCreateFromPathW(norm.c_str()));
    if (itemId)
    {
        const HRESULT selHr = SHOpenFolderAndSelectItems(itemId.get(), 0, nullptr, 0);
        if (SUCCEEDED(selHr))
            return S_OK;
    }

    const auto folder = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0u ? norm : norm.parent_path();
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask  = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"explore";
    sei.lpFile = folder.c_str();
    sei.nShow  = SW_SHOWNORMAL;
    if (! ShellExecuteExW(&sei))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsShellReveal::RevealInRedSalamander(const std::filesystem::path& targetPath, const std::optional<std::filesystem::path>& overridePath) noexcept
try
{
    if (targetPath.empty() || ! targetPath.is_absolute() || HasAnyReparseAncestorOrSelf(targetPath))
        return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    const auto exe = FindRedSalamander(overridePath);
    if (! exe)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    const auto normTarget = targetPath.lexically_normal();
    const auto arg        = std::wstring(L"--select=") + normTarget.native();
    std::wstring cmdLine  = QuoteWindowsCliArgument(exe->native()) + L" " + QuoteWindowsCliArgument(arg);
    const auto workDir    = exe->parent_path();

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (! CreateProcessW(exe->c_str(), cmdLine.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, workDir.c_str(), &si, &pi))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    wil::unique_handle proc(pi.hProcess);
    wil::unique_handle thread(pi.hThread);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

FileNameValidationError WindowsWorkspaceFileOps::ValidateFileName(std::wstring_view candidate) const noexcept
{
    if (candidate.empty())
        return FileNameValidationError::Empty;
    if (candidate.size() > 255u || ! DxUi::RichText::IsValidUtf16(candidate))
        return FileNameValidationError::TooLong;
    if (candidate == L"." || candidate == L"..")
        return FileNameValidationError::ReservedName;
    for (const wchar_t ch : candidate)
    {
        if (ch < L' ' || ch == 0x7Fu)
            return FileNameValidationError::InvalidCharacters;
        if (ch == L'\\' || ch == L'/')
            return FileNameValidationError::PathSeparator;
        if (ch == L'<' || ch == L'>' || ch == L':' || ch == L'"' || ch == L'|' || ch == L'?' || ch == L'*')
            return FileNameValidationError::InvalidCharacters;
    }
    if (candidate.back() == L'.' || candidate.back() == L' ')
        return FileNameValidationError::TrailingDotOrSpace;
    if (IsReservedDosName(candidate))
        return FileNameValidationError::ReservedName;
    return FileNameValidationError::None;
}

HRESULT WindowsWorkspaceFileOps::RenameItem(const std::filesystem::path& sourcePath, std::wstring_view newLeafName, FileOperationOutcome& outOutcome) noexcept
try
{
    outOutcome            = {};
    outOutcome.sourcePath = NormalizeRootPath(sourcePath);
    if (outOutcome.sourcePath.empty() || ! outOutcome.sourcePath.is_absolute() || outOutcome.sourcePath.filename().empty())
        return outOutcome.status = E_INVALIDARG;
    if (ValidateFileName(newLeafName) != FileNameValidationError::None)
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    if (! CheckNoSurrogateReparse(outOutcome.sourcePath, outOutcome.reparseRejected, outOutcome.status, false))
        return outOutcome.status;

    const wil::unique_hfile srcHandle(CreateFileW(outOutcome.sourcePath.c_str(),
                                                  FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
    if (! srcHandle)
    {
        const DWORD err          = GetLastError();
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    bool srcSurrogate{};
    const HRESULT srcVerifyHr = VerifyDiskHandle(srcHandle.get(), std::nullopt, &srcSurrogate);
    if (FAILED(srcVerifyHr))
    {
        if (srcSurrogate)
            outOutcome.reparseRejected = true;
        return outOutcome.status = srcSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : srcVerifyHr;
    }
    FILE_ATTRIBUTE_TAG_INFO srcTag{};
    if (! GetFileInformationByHandleEx(srcHandle.get(), FileAttributeTagInfo, &srcTag, sizeof(srcTag)))
    {
        const DWORD err          = GetLastError();
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    const bool srcIsDir = (srcTag.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
    FileObjectId srcId{};
    const HRESULT srcIdHr = ReadFileObjectId(srcHandle.get(), srcId);
    if (FAILED(srcIdHr))
        return outOutcome.status = srcIdHr;

    const auto destination     = outOutcome.sourcePath.parent_path() / newLeafName;
    outOutcome.destinationPath = destination.lexically_normal();
    if (outOutcome.sourcePath.native() == outOutcome.destinationPath.native())
    {
        outOutcome.destinationIsDirectory = srcIsDir;
        return S_OK;
    }

    const std::wstring srcLeaf = outOutcome.sourcePath.filename().native();
    const bool sameNameFold    = EqualOrdinalIgnoreCase(srcLeaf, newLeafName);
    {
        const wil::unique_hfile dstHandle(CreateFileW(outOutcome.destinationPath.c_str(),
                                                      FILE_READ_ATTRIBUTES,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                      nullptr,
                                                      OPEN_EXISTING,
                                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                      nullptr));
        if (dstHandle)
        {
            bool dstSurrogate{};
            const HRESULT dstVerifyHr = VerifyDiskHandle(dstHandle.get(), std::nullopt, &dstSurrogate);
            if (FAILED(dstVerifyHr))
            {
                if (dstSurrogate)
                    outOutcome.reparseRejected = true;
                return outOutcome.status = dstSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : dstVerifyHr;
            }
            if (! sameNameFold)
            {
                outOutcome.collision     = true;
                return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
            FileObjectId dstId{};
            const HRESULT dstIdHr = ReadFileObjectId(dstHandle.get(), dstId);
            if (FAILED(dstIdHr))
                return outOutcome.status = dstIdHr;
            if (! SameObject(srcId, dstId))
            {
                outOutcome.collision     = true;
                return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
            const wil::unique_hfile parentHandle(CreateFileW(outOutcome.sourcePath.parent_path().c_str(),
                                                             FILE_READ_ATTRIBUTES,
                                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                             nullptr,
                                                             OPEN_EXISTING,
                                                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                             nullptr));
            bool caseSensitiveDir     = false;
            bool caseSensitivityKnown = false;
            if (parentHandle)
            {
                FILE_CASE_SENSITIVE_INFO csInfo{};
                if (GetFileInformationByHandleEx(parentHandle.get(), FileCaseSensitiveInfo, &csInfo, sizeof(csInfo)))
                {
                    caseSensitiveDir     = (csInfo.Flags & FILE_CS_FLAG_CASE_SENSITIVE_DIR) != 0u;
                    caseSensitivityKnown = true;
                }
                else
                {
                    DWORD volFlags{};
                    if (GetVolumeInformationByHandleW(parentHandle.get(), nullptr, 0u, nullptr, nullptr, &volFlags, nullptr, 0u) &&
                        (volFlags & FILE_CASE_SENSITIVE_SEARCH) == 0u)
                    {
                        caseSensitiveDir     = false;
                        caseSensitivityKnown = true;
                    }
                }
            }
            if (! caseSensitivityKnown)
            {
                wchar_t finalBuf[1024]{};
                const DWORD finalLen =
                    GetFinalPathNameByHandleW(dstHandle.get(), finalBuf, static_cast<DWORD>(std::size(finalBuf)), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
                if (finalLen == 0u || finalLen >= std::size(finalBuf))
                {
                    caseSensitiveDir = true;
                }
                else
                {
                    const std::wstring_view finalView(finalBuf, finalLen);
                    const auto slashPos               = finalView.find_last_of(L'\\');
                    const std::wstring_view finalLeaf = slashPos == std::wstring_view::npos ? finalView : finalView.substr(slashPos + 1u);
                    if (! EqualOrdinal(finalLeaf, srcLeaf))
                        caseSensitiveDir = true;
                }
            }
            if (caseSensitiveDir)
            {
                outOutcome.collision     = true;
                return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
        }
        else
        {
            const DWORD dstErr = GetLastError();
            if (dstErr != ERROR_FILE_NOT_FOUND && dstErr != ERROR_PATH_NOT_FOUND)
                return outOutcome.status = HRESULT_FROM_WIN32(dstErr != ERROR_SUCCESS ? dstErr : ERROR_GEN_FAILURE);
        }
    }

    if (! MoveFileExW(outOutcome.sourcePath.c_str(), outOutcome.destinationPath.c_str(), 0u))
    {
        const DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
            outOutcome.collision = true;
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    outOutcome.destinationIsDirectory = srcIsDir;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsWorkspaceFileOps::PasteItem(const std::filesystem::path& sourcePath,
                                           const std::filesystem::path& destinationDirectory,
                                           DropEffectKind effect,
                                           bool replaceExisting,
                                           bool keepBothCopy,
                                           FileOperationOutcome& outOutcome,
                                           const std::atomic_bool* cancel) noexcept
try
{
    outOutcome            = {};
    outOutcome.sourcePath = sourcePath.lexically_normal();
    const auto normDir    = destinationDirectory.lexically_normal();
    if (outOutcome.sourcePath.empty() || ! outOutcome.sourcePath.is_absolute() || normDir.empty() || ! normDir.is_absolute())
        return outOutcome.status = E_INVALIDARG;
    if (IsCancelled(cancel))
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (! CheckNoSurrogateReparse(outOutcome.sourcePath, outOutcome.reparseRejected, outOutcome.status, false) ||
        ! CheckNoSurrogateReparse(normDir, outOutcome.reparseRejected, outOutcome.status, false))
        return outOutcome.status;

    const wil::unique_hfile dirHandle(CreateFileW(normDir.c_str(),
                                                  FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
    if (! dirHandle)
    {
        const DWORD err          = GetLastError();
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_DIRECTORY);
    }
    bool dirSurrogate{};
    const HRESULT dirVerifyHr = VerifyDiskHandle(dirHandle.get(), true, &dirSurrogate);
    if (FAILED(dirVerifyHr))
    {
        if (dirSurrogate)
            outOutcome.reparseRejected = true;
        return outOutcome.status = dirSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : HRESULT_FROM_WIN32(ERROR_DIRECTORY);
    }

    const wil::unique_hfile srcHandle(CreateFileW(outOutcome.sourcePath.c_str(),
                                                  FILE_READ_ATTRIBUTES,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                  nullptr));
    if (! srcHandle)
    {
        const DWORD err          = GetLastError();
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    bool srcSurrogate{};
    const HRESULT srcVerifyHr = VerifyDiskHandle(srcHandle.get(), std::nullopt, &srcSurrogate);
    if (FAILED(srcVerifyHr))
    {
        if (srcSurrogate)
            outOutcome.reparseRejected = true;
        return outOutcome.status = srcSurrogate ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : srcVerifyHr;
    }
    FILE_ATTRIBUTE_TAG_INFO srcTag{};
    if (! GetFileInformationByHandleEx(srcHandle.get(), FileAttributeTagInfo, &srcTag, sizeof(srcTag)))
    {
        const DWORD err          = GetLastError();
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    const bool srcIsDir = (srcTag.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
    if (! srcIsDir && ! IsSupportedWorkspaceFileName(outOutcome.sourcePath.native()))
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
    if (srcIsDir && IsSameOrDescendantPath(outOutcome.sourcePath, normDir))
        return outOutcome.status = E_INVALIDARG;

    auto destination        = (normDir / outOutcome.sourcePath.filename()).lexically_normal();
    const bool sameAsSource = EqualOrdinalIgnoreCase(outOutcome.sourcePath.native(), destination.native());
    if (sameAsSource && effect == DropEffectKind::Move)
    {
        outOutcome.destinationPath        = destination;
        outOutcome.destinationIsDirectory = srcIsDir;
        return S_OK;
    }
    if (! CheckNoSurrogateReparse(destination, outOutcome.reparseRejected, outOutcome.status, true))
        return outOutcome.status;

    if ((sameAsSource && effect == DropEffectKind::Copy) || (keepBothCopy && GetFileAttributesW(destination.c_str()) != INVALID_FILE_ATTRIBUTES))
    {
        const auto stem = outOutcome.sourcePath.stem().native();
        const auto ext  = srcIsDir ? std::wstring{} : outOutcome.sourcePath.extension().native();
        bool foundUnique{};
        for (unsigned index = 1u; index <= 99u; ++index)
        {
            const std::wstring candidateName = index == 1u ? (stem + L" - Copy" + ext) : std::format(L"{} - Copy ({}){}", stem, index, ext);
            const auto candidate             = (normDir / candidateName).lexically_normal();
            if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                destination = candidate;
                foundUnique = true;
                break;
            }
        }
        if (! foundUnique)
            return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
        if (! CheckNoSurrogateReparse(destination, outOutcome.reparseRejected, outOutcome.status, true))
            return outOutcome.status;
    }
    outOutcome.destinationPath = destination;
    if (! replaceExisting && GetFileAttributesW(destination.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        outOutcome.collision     = true;
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }

    if (effect == DropEffectKind::Move)
    {
        if (! srcIsDir)
        {
            if (! MoveFileExW(outOutcome.sourcePath.c_str(), destination.c_str(), (replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0u) | MOVEFILE_COPY_ALLOWED))
            {
                const DWORD err = GetLastError();
                if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
                    outOutcome.collision = true;
                return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
            }
            outOutcome.destinationIsDirectory = false;
            return S_OK;
        }
        if (MoveFileExW(outOutcome.sourcePath.c_str(), destination.c_str(), 0u))
        {
            outOutcome.destinationIsDirectory = true;
            return S_OK;
        }
        const DWORD moveErr    = GetLastError();
        const bool canFallback = moveErr == ERROR_NOT_SAME_DEVICE ||
                                 (replaceExisting && (moveErr == ERROR_ACCESS_DENIED || moveErr == ERROR_ALREADY_EXISTS || moveErr == ERROR_FILE_EXISTS));
        if (! canFallback)
        {
            if (moveErr == ERROR_ALREADY_EXISTS || moveErr == ERROR_FILE_EXISTS)
                outOutcome.collision = true;
            return outOutcome.status = HRESULT_FROM_WIN32(moveErr != ERROR_SUCCESS ? moveErr : ERROR_GEN_FAILURE);
        }

        DirectoryCopyPlan plan;
        const HRESULT preflightHr =
            PreflightDirectoryRecursive(outOutcome.sourcePath, destination, 0u, replaceExisting, true, cancel, plan, outOutcome.reparseRejected);
        if (FAILED(preflightHr))
        {
            if (preflightHr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
                outOutcome.collision = true;
            return outOutcome.status = preflightHr;
        }
        const HRESULT copyHr = ExecuteDirectoryCopyPlan(plan, replaceExisting, cancel, outOutcome.collision, outOutcome.reparseRejected);
        if (FAILED(copyHr))
            return outOutcome.status = copyHr;
        std::size_t deleteBudget = MaximumCopyEntries;
        const HRESULT delHr      = DeleteDirectoryBounded(outOutcome.sourcePath, 0u, deleteBudget, plan.dirBuffer, cancel, outOutcome.reparseRejected);
        if (FAILED(delHr))
            return outOutcome.status = delHr;
        outOutcome.destinationIsDirectory = true;
        return S_OK;
    }

    if (srcIsDir)
    {
        DirectoryCopyPlan plan;
        const HRESULT preflightHr =
            PreflightDirectoryRecursive(outOutcome.sourcePath, destination, 0u, replaceExisting, false, cancel, plan, outOutcome.reparseRejected);
        if (FAILED(preflightHr))
        {
            if (preflightHr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
                outOutcome.collision = true;
            return outOutcome.status = preflightHr;
        }
        const HRESULT copyHr = ExecuteDirectoryCopyPlan(plan, replaceExisting, cancel, outOutcome.collision, outOutcome.reparseRejected);
        if (FAILED(copyHr))
            return outOutcome.status = copyHr;
        outOutcome.destinationIsDirectory = true;
        return S_OK;
    }

    std::vector<std::byte> copyBuffer;
    const HRESULT copyHr =
        CopyFileByHandles(outOutcome.sourcePath, destination, replaceExisting, cancel, copyBuffer, outOutcome.collision, outOutcome.reparseRejected);
    if (FAILED(copyHr))
        return outOutcome.status = copyHr;
    outOutcome.destinationIsDirectory = false;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WriteDropFilesToClipboard(HWND owner, const FileClipboardPayload& payload) noexcept
try
{
    if (payload.paths.empty() || payload.paths.size() > MaximumClipboardPaths)
        return E_INVALIDARG;
    std::size_t charsNeeded = 1u; // Trailing double NUL.
    std::vector<std::wstring> cleaned;
    cleaned.reserve(payload.paths.size());
    for (const auto& p : payload.paths)
    {
        const auto norm = p.lexically_normal().native();
        if (norm.empty() || ! p.is_absolute() || ! DxUi::RichText::IsValidUtf16(norm) || norm.find(L'\0') != std::wstring::npos)
            return E_INVALIDARG;
        charsNeeded += norm.size() + 1u;
        cleaned.push_back(norm);
    }

    const std::size_t hdropBytes = sizeof(DROPFILES) + charsNeeded * sizeof(wchar_t);
    wil::unique_hglobal dropMem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, hdropBytes));
    wil::unique_hglobal effectMem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DWORD)));
    if (! dropMem || ! effectMem)
        return E_OUTOFMEMORY;

    {
        void* const ptr = GlobalLock(dropMem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock = wil::scope_exit([&]() noexcept { GlobalUnlock(dropMem.get()); });
        auto* df          = static_cast<DROPFILES*>(ptr);
        df->pFiles        = sizeof(DROPFILES);
        df->fWide         = TRUE;
        auto* dst         = reinterpret_cast<wchar_t*>(static_cast<std::byte*>(ptr) + sizeof(DROPFILES));
        for (const auto& str : cleaned)
        {
            std::memcpy(dst, str.data(), str.size() * sizeof(wchar_t));
            dst += str.size();
            *dst++ = L'\0';
        }
        *dst = L'\0';
    }
    {
        void* const ptr = GlobalLock(effectMem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock         = wil::scope_exit([&]() noexcept { GlobalUnlock(effectMem.get()); });
        *static_cast<DWORD*>(ptr) = payload.effect == DropEffectKind::Move ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
    }

    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });
    (void)EmptyClipboard();
    if (! SetClipboardData(CF_HDROP, dropMem.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)dropMem.release();
    const UINT format = RegisterClipboardFormatW(PreferredDropEffectFormat);
    if (format != 0u && SetClipboardData(format, effectMem.get()))
        (void)effectMem.release();
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ParseDropFilesBuffer(std::span<const std::byte> dropBytes, std::optional<DWORD> preferredDropEffect, FileClipboardPayload& outPayload) noexcept
try
{
    outPayload = {};
    if (dropBytes.size() < sizeof(DROPFILES))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    DROPFILES df{};
    std::memcpy(&df, dropBytes.data(), sizeof(df));
    if (df.pFiles < sizeof(DROPFILES) || df.pFiles > dropBytes.size() || (df.pFiles % alignof(wchar_t)) != 0u || ! df.fWide)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    const std::byte* base       = dropBytes.data() + df.pFiles;
    const std::size_t maxWchars = (dropBytes.size() - df.pFiles) / sizeof(wchar_t);
    std::size_t pos{};
    std::wstring entry;
    while (pos < maxWchars && outPayload.paths.size() < MaximumClipboardPaths)
    {
        wchar_t firstChar{};
        std::memcpy(&firstChar, base + pos * sizeof(wchar_t), sizeof(wchar_t));
        if (firstChar == L'\0')
            break;
        entry.clear();
        bool foundNul{};
        for (std::size_t i = pos; i < maxWchars; ++i)
        {
            wchar_t ch{};
            std::memcpy(&ch, base + i * sizeof(wchar_t), sizeof(wchar_t));
            if (ch == L'\0')
            {
                foundNul = true;
                pos      = i + 1u;
                break;
            }
            entry.push_back(ch);
        }
        if (! foundNul || entry.empty())
            break;
        if (DxUi::RichText::IsValidUtf16(entry))
        {
            std::filesystem::path candidate(entry);
            if (candidate.is_absolute())
                outPayload.paths.push_back(candidate.lexically_normal());
        }
    }
    if (outPayload.paths.empty())
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    outPayload.effect = (preferredDropEffect && ((*preferredDropEffect & DROPEFFECT_MOVE) != 0u) && ((*preferredDropEffect & DROPEFFECT_COPY) == 0u))
                            ? DropEffectKind::Move
                            : DropEffectKind::Copy;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ReadDropFilesFromClipboard(HWND owner, FileClipboardPayload& outPayload) noexcept
try
{
    outPayload = {};
    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });

    HANDLE hdropHandle = GetClipboardData(CF_HDROP);
    if (! hdropHandle)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    const auto* df = static_cast<const std::byte*>(GlobalLock(hdropHandle));
    if (! df)
        return HRESULT_FROM_WIN32(GetLastError());
    const auto unlock            = wil::scope_exit([&]() noexcept { GlobalUnlock(hdropHandle); });
    const std::size_t totalBytes = GlobalSize(hdropHandle);

    std::optional<DWORD> preferredEffect;
    const UINT format = RegisterClipboardFormatW(PreferredDropEffectFormat);
    if (format != 0u)
    {
        if (HANDLE hEffect = GetClipboardData(format))
        {
            if (const auto* val = static_cast<const DWORD*>(GlobalLock(hEffect)))
            {
                const auto unlockEffect = wil::scope_exit([&]() noexcept { GlobalUnlock(hEffect); });
                if (GlobalSize(hEffect) >= sizeof(DWORD))
                    preferredEffect = *val;
            }
        }
    }
    return ParseDropFilesBuffer(std::span<const std::byte>(df, totalBytes), preferredEffect, outPayload);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WriteUnicodeTextToClipboard(HWND owner, std::wstring_view text) noexcept
try
{
    if (text.size() > 65536u || ! DxUi::RichText::IsValidUtf16(text))
        return E_INVALIDARG;
    wil::unique_hglobal mem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (text.size() + 1u) * sizeof(wchar_t)));
    if (! mem)
        return E_OUTOFMEMORY;
    {
        void* const ptr = GlobalLock(mem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock = wil::scope_exit([&]() noexcept { GlobalUnlock(mem.get()); });
        if (! text.empty())
            std::memcpy(ptr, text.data(), text.size() * sizeof(wchar_t));
        static_cast<wchar_t*>(ptr)[text.size()] = L'\0';
    }
    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });
    (void)EmptyClipboard();
    if (! SetClipboardData(CF_UNICODETEXT, mem.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)mem.release();
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct FileOpsService::State
{
    std::mutex mutex;
    wil::unique_event wake;
    HWND window{};
    UINT message{};
    std::shared_ptr<IShellReveal> reveal;
    std::shared_ptr<IWorkspaceFileOps> fileOps;
    std::deque<WorkspaceFileOpRequest> queue;
    std::deque<WorkspaceFileOpResult> completed;
    std::atomic_bool cancelRequested{false};
    bool busy{};
    bool closing{};

    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
        try
        {
            const DxUi::Failure::Scope failureScope;
            for (;;)
            {
                WorkspaceFileOpRequest req;
                std::shared_ptr<IShellReveal> reveal;
                std::shared_ptr<IWorkspaceFileOps> ops;
                {
                    std::lock_guard lock(self->mutex);
                    if (self->closing)
                        break;
                    if (self->queue.empty())
                    {
                        self->busy = false;
                        // Wait on wake event; avoid spinning.
                        goto waitNext;
                    }
                    req = std::move(self->queue.front());
                    self->queue.pop_front();
                    self->busy = true;
                    reveal     = self->reveal;
                    ops        = self->fileOps;
                }
                {
                    WorkspaceFileOpResult res;
                    res.kind       = req.kind;
                    res.requestId  = req.requestId;
                    res.dropEffect = req.dropEffect;
                    switch (req.kind)
                    {
                        case WorkspaceFileOpRequest::Kind::RevealExplorer:
                            res.outcome.sourcePath = req.sourcePath;
                            res.outcome.status     = reveal ? reveal->RevealInExplorer(req.sourcePath) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::RevealRedSalamander:
                            res.outcome.sourcePath = req.sourcePath;
                            res.outcome.status     = reveal ? reveal->RevealInRedSalamander(req.sourcePath, req.redSalamanderOverride) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::Rename:
                            res.outcome.status = ops ? ops->RenameItem(req.sourcePath, req.newLeafName, res.outcome) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::Paste:
                            res.outcome.status          = ops ? ops->PasteItem(req.sourcePath,
                                                                               req.destinationPath,
                                                                               req.dropEffect,
                                                                               req.replaceExisting,
                                                                               req.keepBothCopy,
                                                                               res.outcome,
                                                                               &self->cancelRequested)
                                                              : E_FAIL;
                            res.requiresCollisionPrompt = res.outcome.collision;
                            break;
                    }
                    res.destinationIsDirectory = res.outcome.destinationIsDirectory;
                    if (SUCCEEDED(res.outcome.status) && ! res.destinationIsDirectory &&
                        (req.kind == WorkspaceFileOpRequest::Kind::Rename || req.kind == WorkspaceFileOpRequest::Kind::Paste) &&
                        ! res.outcome.destinationPath.empty())
                    {
                        const DWORD attrs = GetFileAttributesW(res.outcome.destinationPath.c_str());
                        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0u)
                            res.destinationIsDirectory = true;
                    }
                    HWND notifyWindow{};
                    UINT notifyMessage{};
                    {
                        std::lock_guard lock(self->mutex);
                        if (self->closing)
                            break;
                        self->completed.push_back(std::move(res));
                        notifyWindow  = self->window;
                        notifyMessage = self->message;
                    }
                    if (notifyWindow && notifyMessage >= WM_APP)
                        (void)PostMessageW(notifyWindow, notifyMessage, static_cast<WPARAM>(req.requestId), 0);
                    continue;
                }
            waitNext:
                if (WaitForSingleObject(self->wake.get(), INFINITE) != WAIT_OBJECT_0)
                    break;
            }
            HWND shutdownWindow{};
            UINT shutdownMessage{};
            {
                std::lock_guard lock(self->mutex);
                self->busy      = false;
                shutdownWindow  = self->window;
                shutdownMessage = self->message;
            }
            if (shutdownWindow && shutdownMessage >= WM_APP)
                (void)PostMessageW(shutdownWindow, shutdownMessage, 0u, 0);
        }
        catch (const std::bad_alloc&)
        {
            DxUi::Failure::FailAllocation();
        }
        catch (const std::exception&)
        {
            DxUi::Failure::FailUnexpected();
        }
    }
};
#pragma warning(pop)

FileOpsService::~FileOpsService()
{
    Close();
}

HRESULT FileOpsService::Initialize(HWND window, UINT message, std::shared_ptr<IShellReveal> reveal, std::shared_ptr<IWorkspaceFileOps> fileOps) noexcept
try
{
    if ((window && message < WM_APP) || ! reveal || ! fileOps)
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window   = window;
    staged->message  = message;
    staged->reveal   = std::move(reveal);
    staged->fileOps  = std::move(fileOps);
    auto ownership   = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT hr = ProcessWork::Acquire(ownership->lease);
    if (FAILED(hr))
        return hr;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT FileOpsService::Submit(WorkspaceFileOpRequest request) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->queue.size() >= 64u)
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    _state->busy = true;
    _state->queue.push_back(std::move(request));
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::optional<WorkspaceFileOpResult> FileOpsService::TakeResult()
{
    if (! _state)
        return std::nullopt;
    std::lock_guard lock(_state->mutex);
    if (_state->completed.empty())
        return std::nullopt;
    auto res = std::move(_state->completed.front());
    _state->completed.pop_front();
    return res;
}

bool FileOpsService::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->busy || ! _state->queue.empty();
}

void FileOpsService::Stop() noexcept
{
    if (! _state)
        return;
    std::lock_guard lock(_state->mutex);
    _state->closing = true;
    _state->cancelRequested.store(true, std::memory_order_release);
    _state->queue.clear();
    (void)SetEvent(_state->wake.get());
}

void FileOpsService::Close() noexcept
{
    Stop();
    if (_state)
    {
        std::lock_guard lock(_state->mutex);
        _state->window = nullptr;
    }
    _state.reset();
}
} // namespace RedTilde
