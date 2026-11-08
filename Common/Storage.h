#pragma once
#include <DxUi/RichDocument.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <wil/resource.h>
#include <windows.h>

namespace RedTilde
{
#ifdef REDTILDE_ENABLE_TESTS
// Thread-local faults affect only an explicitly directed native storage test.
// Production builds contain neither this control nor its injection branches.
enum class AtomicWriteFailure
{
    None,
    Write,
    Flush,
    FlushUnsupported,
    ReplaceUnsupported,
    InPlaceWrite,
    InPlacePartialWrite,
    InPlaceFlush
};
void SetAtomicWriteFailureForTest(AtomicWriteFailure failure) noexcept;
enum class AtomicWriteCheckpoint
{
    Staged,
    ReplacementUnsupported,
    Published,
    InPlaceProtected,
    InPlacePartialWritten
};
using AtomicWriteHook = void (*)(AtomicWriteCheckpoint, const std::filesystem::path &, void *) noexcept;
void SetAtomicWriteHookForTest(AtomicWriteHook hook, void *context) noexcept;
#endif
struct FileStamp
{
    bool exists{};
    std::uint64_t size{}, modified{}, hash{};
    std::uint64_t volume{}, fileIndex{}, fileIndexHigh{};
    bool operator==(const FileStamp &) const = default;
};
struct StoredDocument
{
    std::wstring text;
    bool bom{};
    FileStamp stamp;
};
// An admitted document save owns these values independently of ordinary
// coalesced autosave. Hard-link writes require a separate durable save copy.
struct SaveRecoveryContext
{
    std::filesystem::path dataDirectory;
    DxUi::RichText::Selection selection;
    std::uint64_t generation{};
};
[[nodiscard]] HRESULT ReadBytes(const std::filesystem::path &path, std::string &bytes, FileStamp *stamp = nullptr) noexcept;
// Worker-only image I/O. Pin checked parents while opening each component, refuse
// name-surrogate reparse points and read at most 16 MiB through the retained file.
// Network permission and same-share admission are the transport caller's policy.
[[nodiscard]] bool SharesImageNetworkRoot(std::wstring_view document, std::wstring_view image) noexcept;
[[nodiscard]] HRESULT ReadImageFileBytes(std::wstring_view path, std::string &bytes, const std::atomic_bool *cancel = nullptr) noexcept;
// The returned directory handle is overlapped and permits ordinary rename/write
// notifications. Setup uses the same parent/reparse checks as the image reader.
[[nodiscard]] HRESULT OpenImageDirectoryForNotifications(std::wstring_view path, wil::unique_handle &directory) noexcept;
// App-created binary assets only: never replace a pre-existing entry. The
// bounded caller allowance does not change the document read/write limit.
[[nodiscard]] HRESULT WriteNewBytesDurable(const std::filesystem::path &path, std::string_view bytes, FileStamp &saved,
                                           std::size_t maximumBytes) noexcept;
// Remove only the same unchanged single-link file under a write/delete-deny
// handle. Foreign/replaced/modified/reparse entries remain untouched.
[[nodiscard]] HRESULT RemoveCreatedBytesIfUnchanged(const std::filesystem::path &path, const FileStamp &expected, std::size_t maximumBytes) noexcept;
// The optional flag identifies only an unsupported native replacement. An
// equal HRESULT from another phase cannot authorize non-atomic publication.
[[nodiscard]] HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected = nullptr,
                                       FileStamp *saved = nullptr, bool sameIdentityOnly = false, const std::filesystem::path *baselinePath = nullptr,
                                       bool rejectHardlinkedTarget = false, bool *unsupportedReplacement = nullptr) noexcept;
[[nodiscard]] HRESULT LoadDocument(const std::filesystem::path &path, StoredDocument &document, std::size_t *rejectedUnits = nullptr) noexcept;
[[nodiscard]] HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected, FileStamp &saved,
                                   bool sameIdentityOnly = false, const std::filesystem::path *baselinePath = nullptr,
                                   const SaveRecoveryContext *recovery = nullptr) noexcept;
[[nodiscard]] std::wstring ErrorText(HRESULT result);
[[nodiscard]] std::filesystem::path DefaultDataDirectory();
[[nodiscard]] std::uint64_t ContentHash(std::string_view bytes) noexcept;
} // namespace RedTilde
