#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <windows.h>

namespace RedTilde
{
struct FileStamp
{
    bool exists{};
    std::uint64_t size{}, modified{}, hash{};
    bool operator==(const FileStamp &) const = default;
};
struct StoredDocument
{
    std::wstring text;
    bool bom{};
    FileStamp stamp;
};
[[nodiscard]] HRESULT ReadBytes(const std::filesystem::path &path, std::string &bytes, FileStamp *stamp = nullptr) noexcept;
[[nodiscard]] HRESULT WriteBytesAtomic(const std::filesystem::path &path, std::string_view bytes, const FileStamp *expected = nullptr) noexcept;
[[nodiscard]] HRESULT LoadDocument(const std::filesystem::path &path, StoredDocument &document) noexcept;
[[nodiscard]] HRESULT SaveDocument(const std::filesystem::path &path, std::wstring_view text, bool bom, const FileStamp *expected,
                                   FileStamp &saved) noexcept;
[[nodiscard]] std::wstring ErrorText(HRESULT result);
[[nodiscard]] std::filesystem::path DefaultDataDirectory();
[[nodiscard]] std::uint64_t ContentHash(std::string_view bytes) noexcept;
} // namespace RedTilde
