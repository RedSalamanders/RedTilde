#pragma once
#include "Storage.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace RedTilde
{
enum class DictionaryEncoding : std::uint8_t
{
    Utf8,
    Utf8Bom,
    Utf16LeBom,
    Utf16BeBom,
    Utf16LeNoBom
};

enum class DictionaryLineEnding : std::uint8_t
{
    Lf,
    CrLf
};

// Normalize a single dictionary key using Unicode NFC + locale-invariant lowercase.
// Rejects empty strings, whitespace, control characters, U+FFFD, invalid surrogates, and > 64 scalar chars.
[[nodiscard]] bool TryNormalizeDictionaryKey(std::wstring_view word, std::wstring& outKey) noexcept;

// Canonical portable custom dictionary & session ignore store.
// Reads and writes UTF-8 / UTF-8 BOM / UTF-16LE BOM / UTF-16BE BOM / BOM-less UTF-16LE,
// refusing symlinks, reparse points, and legacy single-byte ANSI encodings fail-closed.
class CustomDictionary final
{
public:
    static constexpr std::size_t MaximumFileBytes  = 4u * 1024u * 1024u;
    static constexpr std::size_t MaximumWords      = 100000u;
    static constexpr std::size_t MaximumWordLength = 64u;

    CustomDictionary()                                   = default;
    CustomDictionary(const CustomDictionary&)            = delete;
    CustomDictionary& operator=(const CustomDictionary&) = delete;

    [[nodiscard]] HRESULT Load(const std::filesystem::path& path) noexcept;
    [[nodiscard]] HRESULT AddWord(std::wstring_view rawWord, HRESULT* outDiskStatus = nullptr) noexcept;
    [[nodiscard]] HRESULT RemoveWord(std::wstring_view rawWord, HRESULT* outDiskStatus = nullptr) noexcept;

    void IgnoreAll(std::wstring_view rawWord) noexcept;
    void IgnoreOnce(unsigned workspaceKey, std::wstring_view rawWord, std::size_t occurrenceKey) noexcept;
    void ClearIgnoreOnceForWorkspace(unsigned workspaceKey) noexcept;

    [[nodiscard]] bool IsAccepted(std::wstring_view rawWord,
                                  unsigned workspaceKey                    = 0u,
                                  std::optional<std::size_t> occurrenceKey = std::nullopt) const noexcept;
    [[nodiscard]] bool IsAcceptedNormalized(std::wstring_view normalizedKey,
                                            unsigned workspaceKey                    = 0u,
                                            std::optional<std::size_t> occurrenceKey = std::nullopt) const noexcept;

    [[nodiscard]] std::uint64_t Generation() const noexcept;
    [[nodiscard]] std::filesystem::path Path() const;
    [[nodiscard]] DictionaryEncoding Encoding() const noexcept;
    [[nodiscard]] DictionaryLineEnding LineEnding() const noexcept;
    [[nodiscard]] std::size_t WordCount() const noexcept;
    [[nodiscard]] std::vector<std::wstring> DisplayWords() const;
    [[nodiscard]] HRESULT LoadStatus() const noexcept;

    struct Snapshot
    {
        std::uint64_t generation{};
        std::unordered_set<std::wstring> customWords;
        std::unordered_set<std::wstring> ignoredAll;

        [[nodiscard]] bool ContainsNormalized(const std::wstring& normalizedKey) const noexcept
        {
            return customWords.contains(normalizedKey) || ignoredAll.contains(normalizedKey);
        }
    };

    [[nodiscard]] Snapshot CaptureSnapshot() const;

private:
    struct RawLine
    {
        std::wstring text;
        std::wstring normalizedKey; // Empty for comments, blank lines, and non-word directives.
    };

    [[nodiscard]] HRESULT PersistLocked() noexcept;

    mutable std::mutex _mutex;
    std::filesystem::path _path;
    FileStamp _stamp;
    DictionaryEncoding _encoding     = DictionaryEncoding::Utf8;
    DictionaryLineEnding _lineEnding = DictionaryLineEnding::Lf;
    HRESULT _loadStatus              = S_OK;
    std::uint64_t _generation        = 1u;
    std::vector<RawLine> _rawLines;
    std::unordered_set<std::wstring> _normalizedWords;
    std::unordered_set<std::wstring> _ignoredAll;
    struct IgnoreOnceEntry
    {
        unsigned workspaceKey{};
        std::size_t occurrenceKey{};
        std::wstring normalizedKey;
        bool operator==(const IgnoreOnceEntry&) const noexcept = default;
    };
    std::vector<IgnoreOnceEntry> _ignoredOnce;
    bool _trailingNewline = true;
};
} // namespace RedTilde
