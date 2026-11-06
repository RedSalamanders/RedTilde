#include "Storage.h"
#include "Recovery.h"
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
// Borrow one already-opened file at position zero. Both stable shared reads and
// protected in-place saves use this one bounded read/identity implementation.
HRESULT ReadOpenedBytes(HANDLE file, std::string &bytes, FileStamp *stamp)
{
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file, &info))
        return LastError();
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || info.nFileSizeHigh || info.nFileSizeLow > MaximumBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    std::string staged(info.nFileSizeLow, '\0');
    DWORD count{};
    if (!staged.empty() && !ReadFile(file, staged.data(), static_cast<DWORD>(staged.size()), &count, nullptr))
        return LastError();
    if (count != staged.size())
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    BY_HANDLE_FILE_INFORMATION after{};
    if (!GetFileInformationByHandle(file, &after))
        return LastError();
    auto identity = Stamp(info, ContentHash(staged));
    if (identity != Stamp(after, identity.hash))
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    const auto identityResult = CompleteIdentity(file, identity);
    if (FAILED(identityResult))
        return identityResult;
    if (stamp)
        *stamp = identity;
    bytes.swap(staged);
    return S_OK;
}
bool MatchesSavedContent(const FileStamp &current, const FileStamp &expected) noexcept
{
    // A timestamp-only touch of the same file with identical bytes is harmless.
    return current.exists == expected.exists &&
           (!current.exists || (SameObject(current, expected) && current.size == expected.size && current.hash == expected.hash));
}
HRESULT NeedsExpectedComparison(const FileStamp &identity, const FileStamp *expected, bool sameIdentityOnly,
                                const std::filesystem::path *baselinePath, bool &compare)
{
    compare = expected != nullptr;
    if (!expected || !sameIdentityOnly || !HasIdentity(*expected))
        return S_OK;
    // A distinct Save As destination has picker overwrite consent. An alias of
    // either the saved object or its replaced original still needs the baseline.
    compare = !HasIdentity(identity) || SameObject(identity, *expected);
    if (compare || !baselinePath || baselinePath->empty())
        return S_OK;
    wil::unique_handle original(CreateFileW(baselinePath->c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!original)
    {
        const auto error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(error);
    }
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(original.get(), &info))
        return LastError();
    auto baseline = Stamp(info, 0);
    const auto result = CompleteIdentity(original.get(), baseline);
    if (FAILED(result))
        return result;
    compare = !HasIdentity(baseline) || SameObject(identity, baseline);
    return S_OK;
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
    return ReadOpenedBytes(file.get(), bytes, stamp);
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}

HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected, FileStamp *saved,
                         bool sameIdentityOnly, const std::filesystem::path *baselinePath, bool rejectHardlinkedTarget,
                         bool *unsupportedReplacement) noexcept
try
{
    if (unsupportedReplacement)
        *unsupportedReplacement = false;
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
    if (atomicWriteFailure == AtomicWriteFailure::FlushUnsupported)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
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
    if (finalExists && (rejectHardlinkedTarget || (expected && sameIdentityOnly && HasIdentity(*expected))))
    {
        wil::unique_handle target(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL, nullptr));
        BY_HANDLE_FILE_INFORMATION info{};
        if (!target || !GetFileInformationByHandle(target.get(), &info))
            return LastError();
        if (rejectHardlinkedTarget && info.nNumberOfLinks > 1u)
            return HRESULT_FROM_WIN32(ERROR_RETRY);
        auto identity = Stamp(info, 0);
        const auto identityResult = CompleteIdentity(target.get(), identity);
        if (FAILED(identityResult))
            return identityResult;
        const auto comparison = NeedsExpectedComparison(identity, expected, sameIdentityOnly, baselinePath, compareExpected);
        if (FAILED(comparison))
            return comparison;
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
#ifdef REDTILDE_ENABLE_TESTS
        if (atomicWriteFailure == AtomicWriteFailure::ReplaceUnsupported)
        {
            if (unsupportedReplacement)
                *unsupportedReplacement = true;
            if (atomicWriteHook)
                atomicWriteHook(AtomicWriteCheckpoint::ReplacementUnsupported, path, atomicWriteContext);
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
#endif
        if (!ReplaceFileW(path.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr))
        {
            const auto error = GetLastError();
            const bool unsupported = error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION || error == ERROR_CALL_NOT_IMPLEMENTED;
            if (unsupportedReplacement)
                *unsupportedReplacement = unsupported;
#ifdef REDTILDE_ENABLE_TESTS
            if (unsupported && atomicWriteHook)
                atomicWriteHook(AtomicWriteCheckpoint::ReplacementUnsupported, path, atomicWriteContext);
#endif
            return HRESULT_FROM_WIN32(error);
        }
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

namespace
{
HRESULT SaveProtectedDocument(const std::filesystem::path &target, const std::filesystem::path &originalPath, std::wstring_view text,
                              std::string_view bytes, bool bom, const FileStamp *expected, FileStamp &saved, bool sameIdentityOnly,
                              const std::filesystem::path *baselinePath, const SaveRecoveryContext *recovery)
{
    // Deny cooperative writers/deletion across every alias until publication.
    // The existing metadata-only probe shares all access and does not prevent this lock.
    wil::unique_handle file(
        CreateFileW(target.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file)
        return LastError();
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file.get(), &info))
        return LastError();
    auto current = Stamp(info, 0);
    const auto identified = CompleteIdentity(file.get(), current);
    if (FAILED(identified))
        return identified;
    bool compare{};
    const auto comparison = NeedsExpectedComparison(current, expected, sameIdentityOnly, baselinePath, compare);
    if (FAILED(comparison))
        return comparison;
    if (compare)
    {
        if (!SameObject(current, *expected) || current.size != expected->size)
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        std::string previous;
        const auto read = ReadOpenedBytes(file.get(), previous, &current);
        if (FAILED(read))
            return read;
        if (!MatchesSavedContent(current, *expected))
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }
    if (!recovery)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    SaveRecoveryProtection protection;
    const RecoverySnapshot snapshot{std::wstring(text), originalPath, bom, recovery->selection, recovery->generation};
    const auto admitted = protection.Admit(recovery->dataDirectory, snapshot);
    if (FAILED(admitted))
        return admitted;
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteHook)
        atomicWriteHook(AtomicWriteCheckpoint::InPlaceProtected, target, atomicWriteContext);
    if (atomicWriteFailure == AtomicWriteFailure::InPlaceWrite)
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#endif
    wil::unique_handle metadata(CreateFileW(target.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!metadata)
        return LastError();
    if (!SetFilePointerEx(file.get(), LARGE_INTEGER{}, nullptr, FILE_BEGIN))
        return LastError();
    DWORD written{};
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteFailure == AtomicWriteFailure::InPlacePartialWrite)
    {
        const auto partial = static_cast<DWORD>((std::max)(std::size_t{1u}, bytes.size() / 2u));
        if (!bytes.empty() && !WriteFile(file.get(), bytes.data(), partial, &written, nullptr))
            return LastError();
        if (atomicWriteHook)
            atomicWriteHook(AtomicWriteCheckpoint::InPlacePartialWritten, target, atomicWriteContext);
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
#endif
    if (!bytes.empty() && !WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr))
        return LastError();
    if (written != bytes.size() || !SetEndOfFile(file.get()))
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteFailure == AtomicWriteFailure::InPlaceFlush)
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
#endif
    if (!FlushFileBuffers(file.get()))
        return LastError();
    file.reset(); // Publish final write time before reading metadata, never content readback.
    BY_HANDLE_FILE_INFORMATION after{};
    if (!GetFileInformationByHandle(metadata.get(), &after))
        return LastError();
    auto result = Stamp(after, ContentHash(bytes));
    const auto identity = CompleteIdentity(metadata.get(), result);
    if (FAILED(identity))
        return identity;
    if (result.size != bytes.size() || !SameObject(current, result))
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    saved = result;
    (void)protection.Retire(); // Failure is logged; a valid extra recovery copy remains available.
#ifdef REDTILDE_ENABLE_TESTS
    if (atomicWriteHook)
        atomicWriteHook(AtomicWriteCheckpoint::Published, target, atomicWriteContext);
#endif
    return S_OK;
}
HRESULT ResolveSymbolicTarget(const std::filesystem::path &path, wil::unique_handle &followed, std::filesystem::path &target,
                              wil::unique_handle &link)
{
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return S_OK;
    wil::unique_handle probe(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                         FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    FILE_ATTRIBUTE_TAG_INFO tag{};
    if (!probe || !GetFileInformationByHandleEx(probe.get(), FileAttributeTagInfo, &tag, sizeof(tag)))
        return LastError();
    if (!IsReparseTagNameSurrogate(tag.ReparseTag))
        return S_OK; // Cloud and other non-name-surrogate tags retain their provider path.
    probe.reset(CreateFileW(path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!probe || !GetFileInformationByHandleEx(probe.get(), FileAttributeTagInfo, &tag, sizeof(tag)))
        return LastError();
    if (!IsReparseTagNameSurrogate(tag.ReparseTag))
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    // Follow again after acquiring the entry lease: its target may have changed
    // between the caller's first metadata probe and this protected resolution.
    wil::unique_handle current(
        CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!current)
        return LastError();
    const auto count = GetFinalPathNameByHandleW(current.get(), nullptr, 0u, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (!count)
        return LastError();
    if (count > 32767u)
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    std::wstring name(count + 1u, L'\0');
    const auto length =
        GetFinalPathNameByHandleW(current.get(), name.data(), static_cast<DWORD>(name.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (!length)
        return LastError();
    if (length >= name.size())
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    name.resize(length);
    target = name;
    followed.swap(current);
    link.swap(probe); // Keep the chosen link entry intact while its target is saved.
    return S_OK;
}
} // namespace

HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected, FileStamp &saved,
                     bool sameIdentityOnly, const std::filesystem::path *baselinePath, const SaveRecoveryContext *recovery) noexcept
try
{
    if (text.size() > DxUi::RichText::Document::MaximumUnits)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    if (text.find(L'\0') != text.npos)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    std::string bytes = bom ? "\xEF\xBB\xBF" : "";
    bytes += DxUi::RichText::EncodeUtf8(text);
    for (unsigned attempt = 0u; attempt < 2u; ++attempt)
    {
        std::filesystem::path target = path;
        wil::unique_handle link;
        wil::unique_handle metadata(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!metadata)
        {
            const auto error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                return HRESULT_FROM_WIN32(error);
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                return HRESULT_FROM_WIN32(error); // Never replace a dangling link entry.
        }
        else
        {
            BY_HANDLE_FILE_INFORMATION info{};
            if (!GetFileInformationByHandle(metadata.get(), &info))
                return LastError();
            if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            const auto resolved = ResolveSymbolicTarget(path, metadata, target, link);
            if (FAILED(resolved))
                return resolved;
            if (!GetFileInformationByHandle(metadata.get(), &info))
                return LastError();
            if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            if (info.nNumberOfLinks > 1u)
                return SaveProtectedDocument(target, path, text, bytes, bom, expected, saved, sameIdentityOnly, baselinePath, recovery);
        }
        bool unsupportedReplacement{};
        const auto result = WriteBytesAtomic(target, bytes, expected, &saved, sameIdentityOnly, baselinePath, true, &unsupportedReplacement);
        // Only an explicit unsupported result from replacement permits this
        // fallback. Staging, access and sharing failures never authorize writes.
        // Reuse the same identity lock and independently durable recovery as a
        // hard-link save; the previous saved stamp survives partial failure.
        if (unsupportedReplacement)
            return SaveProtectedDocument(target, path, text, bytes, bom, expected, saved, sameIdentityOnly, baselinePath, recovery);
        if (result != HRESULT_FROM_WIN32(ERROR_RETRY))
            return result;
        // A new hard-link alias appeared during staging. Re-evaluate once;
        // ordinary conflicts and unsupported recovery still decline unchanged.
    }
    return HRESULT_FROM_WIN32(ERROR_RETRY);
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::invalid_argument &)
{
    return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
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
