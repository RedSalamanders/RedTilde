#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
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
    Flush
};
void SetAtomicWriteFailureForTest(AtomicWriteFailure failure) noexcept;
enum class AtomicWriteCheckpoint
{
    Staged,
    Published
};
using AtomicWriteHook = void (*)(AtomicWriteCheckpoint, const std::filesystem::path &, void *) noexcept;
void SetAtomicWriteHookForTest(AtomicWriteHook hook, void *context) noexcept;
#endif
struct FileStamp
{
    bool exists{};
    std::uint64_t size{}, modified{}, hash{};
    std::uint64_t volume{}, fileIndex{};
    bool operator==(const FileStamp &) const = default;
};
struct StoredDocument
{
    std::wstring text;
    bool bom{};
    FileStamp stamp;
};
[[nodiscard]] HRESULT ReadBytes(const std::filesystem::path &path, std::string &bytes, FileStamp *stamp = nullptr) noexcept;
[[nodiscard]] HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected = nullptr,
                                       FileStamp *saved = nullptr, bool sameIdentityOnly = false,
                                       const std::filesystem::path *baselinePath = nullptr) noexcept;
[[nodiscard]] HRESULT LoadDocument(const std::filesystem::path &path, StoredDocument &document, std::size_t *rejectedUnits = nullptr) noexcept;
[[nodiscard]] HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected, FileStamp &saved,
                                   bool sameIdentityOnly = false, const std::filesystem::path *baselinePath = nullptr) noexcept;
[[nodiscard]] std::wstring ErrorText(HRESULT result);
[[nodiscard]] std::filesystem::path DefaultDataDirectory();
[[nodiscard]] std::uint64_t ContentHash(std::string_view bytes) noexcept;
} // namespace RedTilde
