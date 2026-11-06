#include "Storage.h"
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <format>
#include <new>
#include <shlobj.h>
#include <stdexcept>
#include <wil/resource.h>

namespace RedTilde
{
#ifdef REDTILDE_ENABLE_TESTS
namespace
{
thread_local AtomicWriteFailure atomicWriteFailure{};
thread_local AtomicWriteHook atomicWriteHook{};
thread_local void *atomicWriteContext{};
} // namespace
void SetAtomicWriteFailureForTest(AtomicWriteFailure failure) noexcept
{
    atomicWriteFailure = failure;
}
void SetAtomicWriteHookForTest(AtomicWriteHook hook, void *context) noexcept
{
    atomicWriteHook = hook;
    atomicWriteContext = context;
}
#endif
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
FileStamp Stamp(const BY_HANDLE_FILE_INFORMATION &info, std::uint64_t hash) noexcept
{
    return {true,
            (static_cast<std::uint64_t>(info.nFileSizeHigh) << 32u) | info.nFileSizeLow,
            FileTimeValue(info.ftLastWriteTime),
            hash,
            info.dwVolumeSerialNumber,
            (static_cast<std::uint64_t>(info.nFileIndexHigh) << 32u) | info.nFileIndexLow};
}
HRESULT CompleteIdentity(HANDLE file, FileStamp &stamp) noexcept
{
    FILE_ID_INFO info{};
    if (GetFileInformationByHandleEx(file, FileIdInfo, &info, sizeof(info)))
    {
        stamp.volume = info.VolumeSerialNumber;
        std::memcpy(&stamp.fileIndex, info.FileId.Identifier, sizeof(stamp.fileIndex));
        std::memcpy(&stamp.fileIndexHigh, info.FileId.Identifier + sizeof(stamp.fileIndex), sizeof(stamp.fileIndexHigh));
        return S_OK;
    }
    const auto error = GetLastError();
    // Older remote providers can expose only BY_HANDLE_FILE_INFORMATION's
    // legacy identity. Preserve that capability without masking other failures.
    if (error == ERROR_INVALID_PARAMETER || error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION)
        return S_OK;
    return HRESULT_FROM_WIN32(error);
}
bool HasIdentity(const FileStamp &stamp) noexcept
{
    return stamp.volume && (stamp.fileIndex || stamp.fileIndexHigh);
}
bool SameObject(const FileStamp &left, const FileStamp &right) noexcept
{
    return HasIdentity(left) && HasIdentity(right) && left.volume == right.volume && left.fileIndex == right.fileIndex &&
           left.fileIndexHigh == right.fileIndexHigh;
}
bool MatchesSavedContent(const FileStamp &current, const FileStamp &expected) noexcept
{
    // A timestamp-only touch of the same file with identical bytes is harmless.
    return current.exists == expected.exists &&
           (!current.exists || (SameObject(current, expected) && current.size == expected.size && current.hash == expected.hash));
}
} // namespace

std::uint64_t ContentHash(std::string_view bytes) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const char unit : bytes)
    {
        const auto value = static_cast<unsigned char>(unit);
        hash ^= value;
        hash *= 1099511628211ull;
    }
    return hash;
}

HRESULT ReadBytes(const std::filesystem::path &path, std::string &bytes, FileStamp *stamp) noexcept
try
{
    wil::unique_handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, nullptr));
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
    BY_HANDLE_FILE_INFORMATION after{};
    if (!GetFileInformationByHandle(file.get(), &after))
        return LastError();
    auto identity = Stamp(info, ContentHash(staged));
    if (identity != Stamp(after, identity.hash))
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    const auto identityResult = CompleteIdentity(file.get(), identity);
    if (FAILED(identityResult))
        return identityResult;
    if (stamp)
        *stamp = identity;
    bytes.swap(staged);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}

HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected, FileStamp *saved,
                         bool sameIdentityOnly, const std::filesystem::path *baselinePath) noexcept
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
    std::filesystem::path temporary;
    wil::unique_handle file;
    for (unsigned attempt = 0; attempt < 8 && !file; ++attempt)
    {
        // Keep the staging leaf independent of the target's length, in the same directory for atomic publication.
        temporary = path.parent_path() / std::format(L".redtilde-{}-{}-{}.tmp", GetCurrentProcessId(), GetTickCount64(), sequence.fetch_add(1));
        file.reset(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file && GetLastError() != ERROR_FILE_EXISTS)
            return LastError();
    }
    if (!file)
        return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    auto removeTemporary = wil::scope_exit(
        [&]
        {
            file.reset(); // The share-none staging handle must close before deletion on every failure path.
            DeleteFileW(temporary.c_str());
        });
    DWORD written{};
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteFailure == AtomicWriteFailure::Write)
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#endif
    if (!bytes.empty() && !WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr))
        return LastError();
    if (written != bytes.size())
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteFailure == AtomicWriteFailure::Flush)
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#endif
    if (!FlushFileBuffers(file.get()))
        return LastError();
    // Metadata-only access is compatible with the exclusive writer. Keep this
    // object available while closing that writer so its final write time is visible.
    wil::unique_handle metadata;
    if (saved)
    {
        metadata.reset(CreateFileW(temporary.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!metadata)
            return LastError();
    }
    file.reset();
    FileStamp writtenStamp;
    if (saved)
    {
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(metadata.get(), &info))
            return LastError();
        writtenStamp = Stamp(info, ContentHash(bytes));
        if (writtenStamp.size != bytes.size())
            return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        const auto identityResult = CompleteIdentity(metadata.get(), writtenStamp);
        if (FAILED(identityResult))
            return identityResult;
        metadata.reset();
    }
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteHook)
        atomicWriteHook(AtomicWriteCheckpoint::Staged, path, atomicWriteContext);
#endif
    // The expected identity is checked after staging, immediately before
    // publication. This narrows, but cannot eliminate, a cross-process race.
    const DWORD finalAttributes = GetFileAttributesW(path.c_str());
    const bool finalExists = finalAttributes != INVALID_FILE_ATTRIBUTES;
    if (!finalExists && GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
        return LastError();
    if (finalExists && (finalAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    bool compareExpected = expected != nullptr;
    if (expected && sameIdentityOnly && finalExists && HasIdentity(*expected))
    {
        wil::unique_handle target(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL, nullptr));
        BY_HANDLE_FILE_INFORMATION info{};
        if (!target || !GetFileInformationByHandle(target.get(), &info))
            return LastError();
        auto identity = Stamp(info, 0);
        const auto identityResult = CompleteIdentity(target.get(), identity);
        if (FAILED(identityResult))
            return identityResult;
        // Save As to another object has explicit picker overwrite consent. An
        // alias of the current object still needs its original saved baseline.
        compareExpected = !HasIdentity(identity) || SameObject(identity, *expected);
        if (!compareExpected && baselinePath && !baselinePath->empty())
        {
            wil::unique_handle original(CreateFileW(baselinePath->c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
            if (!original)
            {
                const auto error = GetLastError();
                if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                    return HRESULT_FROM_WIN32(error);
            }
            else
            {
                BY_HANDLE_FILE_INFORMATION originalInfo{};
                if (!GetFileInformationByHandle(original.get(), &originalInfo))
                    return LastError();
                auto originalIdentity = Stamp(originalInfo, 0);
                const auto originalResult = CompleteIdentity(original.get(), originalIdentity);
                if (FAILED(originalResult))
                    return originalResult;
                compareExpected = !HasIdentity(originalIdentity) || SameObject(identity, originalIdentity);
            }
        }
    }
    else if (expected && sameIdentityOnly && !finalExists)
        compareExpected = false;
    if (expected && compareExpected)
    {
        FileStamp current;
        if (finalExists)
        {
            std::string previous;
            const HRESULT result = ReadBytes(path, previous, &current);
            if (FAILED(result))
                return result;
        }
        if (!MatchesSavedContent(current, *expected))
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }
    if (finalExists)
    {
        if (!ReplaceFileW(path.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr))
            return LastError();
    }
    else if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH))
        return LastError();
    removeTemporary.release();
    if (saved)
        *saved = writtenStamp;
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteHook)
        atomicWriteHook(AtomicWriteCheckpoint::Published, path, atomicWriteContext);
#endif
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}

HRESULT LoadDocument(const std::filesystem::path &path, StoredDocument &document, std::size_t *rejectedUnits) noexcept
try
{
    if (rejectedUnits)
        *rejectedUnits = 0;
    std::string bytes;
    StoredDocument staged;
    const HRESULT result = ReadBytes(path, bytes, &staged.stamp);
    if (FAILED(result))
        return result;
    staged.bom = bytes.starts_with("\xEF\xBB\xBF");
    const auto encoded = std::string_view(bytes).substr(staged.bom ? 3u : 0u);
    // The SDK decoder deliberately stops at editor capacity. Count bounded,
    // validated input first so the app can explain the actual rejected length.
    if (!encoded.empty())
    {
        const int units = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, encoded.data(), static_cast<int>(encoded.size()), nullptr, 0);
        if (units == 0)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        if (static_cast<std::size_t>(units) > DxUi::RichText::Document::MaximumUnits)
        {
            if (rejectedUnits)
                *rejectedUnits = static_cast<std::size_t>(units);
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        }
    }
    staged.text = DxUi::RichText::DecodeUtf8(encoded);
    if (staged.text.size() > DxUi::RichText::Document::MaximumUnits)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    // The pinned decoder admits U+0000 for counted editor source; documents still reject NUL.
    if (!DxUi::RichText::IsValidUtf16(staged.text))
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
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

HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected, FileStamp &saved,
                     bool sameIdentityOnly, const std::filesystem::path *baselinePath) noexcept
try
{
    if (text.size() > DxUi::RichText::Document::MaximumUnits)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    std::string bytes = bom ? "\xEF\xBB\xBF" : "";
    bytes += DxUi::RichText::EncodeUtf8(text);
    return WriteBytesAtomic(path, bytes, expected, &saved, sameIdentityOnly, baselinePath);
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
