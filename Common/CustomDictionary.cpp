#include "CustomDictionary.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <cwctype>
#include <new>
#include <string>
#include <utility>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
[[nodiscard]] std::wstring_view TrimLine(std::wstring_view text) noexcept
{
    while (! text.empty() && (text.front() == L' ' || text.front() == L'\t'))
        text.remove_prefix(1u);
    while (! text.empty() && (text.back() == L' ' || text.back() == L'\t'))
        text.remove_suffix(1u);
    return text;
}

[[nodiscard]] bool IsCommentOrDirectiveLine(std::wstring_view trimmed) noexcept
{
    return trimmed.empty() || trimmed.starts_with(L'#') || trimmed.starts_with(L';') || trimmed.starts_with(L"//");
}

[[nodiscard]] bool CountValidatedScalarChars(std::wstring_view text, std::size_t& outCount) noexcept
{
    outCount = 0u;
    for (std::size_t i = 0u; i < text.size();)
    {
        const wchar_t ch = text[i];
        if (ch < L' ' || (ch >= 0x007Fu && ch <= 0x009Fu) || ch == 0xFFFDu)
            return false;
        if (std::iswspace(static_cast<wint_t>(ch)) != 0)
            return false;
        if (IS_HIGH_SURROGATE(ch))
        {
            if (i + 1u >= text.size() || ! IS_LOW_SURROGATE(text[i + 1u]))
                return false;
            i += 2u;
        }
        else if (IS_LOW_SURROGATE(ch))
        {
            return false;
        }
        else
        {
            ++i;
        }
        ++outCount;
        if (outCount > CustomDictionary::MaximumWordLength)
            return false;
    }
    return outCount > 0u;
}

[[nodiscard]] bool CheckPathNotReparsePoint(const std::filesystem::path& path, HRESULT& outError) noexcept
{
    outError = S_OK;
    if (path.empty())
    {
        outError = E_INVALIDARG;
        return false;
    }
    WIN32_FILE_ATTRIBUTE_DATA attrs{};
    if (! GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attrs))
    {
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
            return true;
        outError = HRESULT_FROM_WIN32(err ? err : ERROR_ACCESS_DENIED);
        return false;
    }
    if ((attrs.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u || (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
    {
        outError = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        return false;
    }
    const wil::unique_hfile handle(CreateFileW(path.c_str(),
                                               FILE_READ_ATTRIBUTES,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr,
                                               OPEN_EXISTING,
                                               FILE_FLAG_OPEN_REPARSE_POINT,
                                               nullptr));
    if (! handle)
    {
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND)
            return true;
        outError = HRESULT_FROM_WIN32(err ? err : ERROR_ACCESS_DENIED);
        return false;
    }
    FILE_BASIC_INFO basic{};
    if (GetFileInformationByHandleEx(handle.get(), FileBasicInfo, &basic, sizeof(basic)) &&
        ((basic.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u || (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u))
    {
        outError = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        return false;
    }
    return true;
}

[[nodiscard]] bool LooksLikeBomlessUtf16Le(std::string_view bytes) noexcept
{
    if (bytes.size() < 2u || (bytes.size() % 2u) != 0u)
        return false;
    std::size_t zeroOdd     = 0u;
    std::size_t zeroEven    = 0u;
    const std::size_t pairs = std::min<std::size_t>(bytes.size() / 2u, 256u);
    for (std::size_t i = 0u; i < pairs; ++i)
    {
        if (bytes[2u * i] == '\0')
            ++zeroEven;
        if (bytes[2u * i + 1u] == '\0')
            ++zeroOdd;
    }
    return zeroOdd > pairs / 4u && zeroEven == 0u;
}

[[nodiscard]] HRESULT DecodeDictionaryBytes(
    std::string_view bytes, std::wstring& outText, DictionaryEncoding& outEncoding, DictionaryLineEnding& outLineEnding, bool& outTrailingNewline) noexcept
try
{
    outText.clear();
    outEncoding        = DictionaryEncoding::Utf8;
    outLineEnding      = DictionaryLineEnding::Lf;
    outTrailingNewline = true;
    if (bytes.empty())
        return S_OK;

    const auto u8 = reinterpret_cast<const unsigned char*>(bytes.data());
    if (bytes.size() >= 2u && u8[0] == 0xFFu && u8[1] == 0xFEu)
    {
        outEncoding = DictionaryEncoding::Utf16LeBom;
        bytes.remove_prefix(2u);
        if ((bytes.size() % 2u) != 0u)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        outText.resize(bytes.size() / 2u);
        for (std::size_t i = 0u; i < outText.size(); ++i)
        {
            const unsigned lo = static_cast<unsigned char>(bytes[2u * i]);
            const unsigned hi = static_cast<unsigned char>(bytes[2u * i + 1u]);
            outText[i]        = static_cast<wchar_t>(lo | (hi << 8u));
        }
    }
    else if (bytes.size() >= 2u && u8[0] == 0xFEu && u8[1] == 0xFFu)
    {
        outEncoding = DictionaryEncoding::Utf16BeBom;
        bytes.remove_prefix(2u);
        if ((bytes.size() % 2u) != 0u)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        outText.resize(bytes.size() / 2u);
        for (std::size_t i = 0u; i < outText.size(); ++i)
        {
            const unsigned hi = static_cast<unsigned char>(bytes[2u * i]);
            const unsigned lo = static_cast<unsigned char>(bytes[2u * i + 1u]);
            outText[i]        = static_cast<wchar_t>((hi << 8u) | lo);
        }
    }
    else if (bytes.size() >= 3u && u8[0] == 0xEFu && u8[1] == 0xBBu && u8[2] == 0xBFu)
    {
        outEncoding = DictionaryEncoding::Utf8Bom;
        bytes.remove_prefix(3u);
        if (DxUi::RichText::TryDecodeUtf8(bytes, outText) != DxUi::RichText::SourceResult::Success)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    }
    else if (LooksLikeBomlessUtf16Le(bytes))
    {
        outEncoding = DictionaryEncoding::Utf16LeNoBom;
        outText.resize(bytes.size() / 2u);
        for (std::size_t i = 0u; i < outText.size(); ++i)
        {
            const unsigned lo = static_cast<unsigned char>(bytes[2u * i]);
            const unsigned hi = static_cast<unsigned char>(bytes[2u * i + 1u]);
            outText[i]        = static_cast<wchar_t>(lo | (hi << 8u));
        }
    }
    else
    {
        outEncoding = DictionaryEncoding::Utf8;
        if (DxUi::RichText::TryDecodeUtf8(bytes, outText) != DxUi::RichText::SourceResult::Success)
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    }

    if (! DxUi::RichText::IsValidUtf16(outText))
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

    if (outText.find(L"\r\n") != std::wstring::npos)
        outLineEnding = DictionaryLineEnding::CrLf;
    outTrailingNewline = outText.empty() || outText.ends_with(L'\n') || outText.ends_with(L'\r');
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

[[nodiscard]] HRESULT EncodeDictionaryText(std::wstring_view text, DictionaryEncoding encoding, std::string& outBytes) noexcept
try
{
    outBytes.clear();
    switch (encoding)
    {
        case DictionaryEncoding::Utf8: return DxUi::RichText::TryEncodeUtf8(text, outBytes) == DxUi::RichText::SourceResult::Success ? S_OK : E_INVALIDARG;
        case DictionaryEncoding::Utf8Bom:
        {
            std::string body;
            if (DxUi::RichText::TryEncodeUtf8(text, body) != DxUi::RichText::SourceResult::Success)
                return E_INVALIDARG;
            outBytes.reserve(3u + body.size());
            outBytes.append("\xEF\xBB\xBF", 3u);
            outBytes.append(body);
            return S_OK;
        }
        case DictionaryEncoding::Utf16LeBom:
        case DictionaryEncoding::Utf16LeNoBom:
        {
            const bool bom = encoding == DictionaryEncoding::Utf16LeBom;
            outBytes.reserve((bom ? 2u : 0u) + text.size() * 2u);
            if (bom)
            {
                outBytes.push_back(static_cast<char>(0xFFu));
                outBytes.push_back(static_cast<char>(0xFEu));
            }
            for (const wchar_t ch : text)
            {
                const unsigned value = static_cast<unsigned>(ch);
                outBytes.push_back(static_cast<char>(value & 0xFFu));
                outBytes.push_back(static_cast<char>((value >> 8u) & 0xFFu));
            }
            return S_OK;
        }
        case DictionaryEncoding::Utf16BeBom:
        {
            outBytes.reserve(2u + text.size() * 2u);
            outBytes.push_back(static_cast<char>(0xFEu));
            outBytes.push_back(static_cast<char>(0xFFu));
            for (const wchar_t ch : text)
            {
                const unsigned value = static_cast<unsigned>(ch);
                outBytes.push_back(static_cast<char>((value >> 8u) & 0xFFu));
                outBytes.push_back(static_cast<char>(value & 0xFFu));
            }
            return S_OK;
        }
    }
    return E_INVALIDARG;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
} // namespace

bool TryNormalizeDictionaryKey(std::wstring_view word, std::wstring& outKey) noexcept
try
{
    outKey.clear();
    const std::wstring_view trimmed = TrimLine(word);
    std::size_t scalarCount{};
    if (! CountValidatedScalarChars(trimmed, scalarCount))
        return false;

    const int nfcCapacity = NormalizeString(NormalizationC, trimmed.data(), static_cast<int>(trimmed.size()), nullptr, 0);
    if (nfcCapacity <= 0)
        return false;
    std::wstring nfc(static_cast<std::size_t>(nfcCapacity), L'\0');
    const int nfcWritten = NormalizeString(NormalizationC, trimmed.data(), static_cast<int>(trimmed.size()), nfc.data(), nfcCapacity);
    if (nfcWritten <= 0)
        return false;
    nfc.resize(static_cast<std::size_t>(nfcWritten));

    const int lowerCapacity = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, nfc.data(), static_cast<int>(nfc.size()), nullptr, 0, nullptr, nullptr, 0);
    if (lowerCapacity <= 0)
        return false;
    std::wstring folded(static_cast<std::size_t>(lowerCapacity), L'\0');
    const int lowerWritten =
        LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, nfc.data(), static_cast<int>(nfc.size()), folded.data(), lowerCapacity, nullptr, nullptr, 0);
    if (lowerWritten <= 0)
        return false;
    folded.resize(static_cast<std::size_t>(lowerWritten));

    std::size_t normalizedScalars{};
    if (! CountValidatedScalarChars(folded, normalizedScalars))
        return false;

    outKey = std::move(folded);
    return true;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT CustomDictionary::Load(const std::filesystem::path& path) noexcept
try
{
    std::lock_guard lock(_mutex);
    _path  = path;
    _stamp = {};
    _rawLines.clear();
    _normalizedWords.clear();
    _encoding        = DictionaryEncoding::Utf8;
    _lineEnding      = DictionaryLineEnding::Lf;
    _trailingNewline = true;
    _loadStatus      = S_OK;

    HRESULT reparseHr = S_OK;
    if (! CheckPathNotReparsePoint(path, reparseHr))
    {
        _loadStatus = reparseHr;
        ++_generation;
        return reparseHr;
    }

    std::string bytes;
    FileStamp stamp;
    const HRESULT readHr = ReadBytes(path, bytes, &stamp, MaximumFileBytes);
    if (readHr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        ++_generation;
        return S_OK;
    }
    if (FAILED(readHr))
    {
        _loadStatus = readHr;
        ++_generation;
        return readHr;
    }

    std::wstring text;
    DictionaryEncoding encoding     = DictionaryEncoding::Utf8;
    DictionaryLineEnding lineEnding = DictionaryLineEnding::Lf;
    bool trailingNewline            = true;
    const HRESULT decodeHr          = DecodeDictionaryBytes(bytes, text, encoding, lineEnding, trailingNewline);
    if (FAILED(decodeHr))
    {
        _loadStatus = decodeHr;
        ++_generation;
        return decodeHr;
    }

    std::vector<RawLine> parsedLines;
    std::unordered_set<std::wstring> normalizedSet;
    std::size_t cursor = 0u;
    while (cursor < text.size())
    {
        std::size_t lineEnd = text.find_first_of(L"\r\n", cursor);
        if (lineEnd == std::wstring::npos)
            lineEnd = text.size();
        const std::wstring_view lineView(text.data() + cursor, lineEnd - cursor);
        RawLine raw;
        raw.text = std::wstring(lineView);

        const std::wstring_view trimmed = TrimLine(lineView);
        if (! IsCommentOrDirectiveLine(trimmed))
        {
            std::wstring key;
            if (TryNormalizeDictionaryKey(trimmed, key))
            {
                if (normalizedSet.size() >= MaximumWords && ! normalizedSet.contains(key))
                {
                    _loadStatus = HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
                    ++_generation;
                    return _loadStatus;
                }
                normalizedSet.insert(key);
                raw.normalizedKey = std::move(key);
            }
        }
        parsedLines.push_back(std::move(raw));

        if (lineEnd >= text.size())
            break;
        if (text[lineEnd] == L'\r' && lineEnd + 1u < text.size() && text[lineEnd + 1u] == L'\n')
            cursor = lineEnd + 2u;
        else
            cursor = lineEnd + 1u;
    }

    _stamp           = stamp;
    _encoding        = encoding;
    _lineEnding      = lineEnding;
    _trailingNewline = trailingNewline;
    _rawLines        = std::move(parsedLines);
    _normalizedWords = std::move(normalizedSet);
    _loadStatus      = S_OK;
    ++_generation;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT CustomDictionary::PersistLocked() noexcept
try
{
    if (_path.empty())
        return E_INVALIDARG;
    if (FAILED(_loadStatus))
        return _loadStatus;

    HRESULT reparseHr = S_OK;
    if (! CheckPathNotReparsePoint(_path, reparseHr))
        return reparseHr;

    const std::wstring_view eol = _lineEnding == DictionaryLineEnding::CrLf ? L"\r\n" : L"\n";
    std::wstring output;
    output.reserve(_rawLines.size() * 16u);
    for (std::size_t i = 0u; i < _rawLines.size(); ++i)
    {
        output.append(_rawLines[i].text);
        if (i + 1u < _rawLines.size() || _trailingNewline)
            output.append(eol);
    }

    std::string encoded;
    const HRESULT encHr = EncodeDictionaryText(output, _encoding, encoded);
    if (FAILED(encHr))
        return encHr;
    if (encoded.size() > MaximumFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::error_code dirError;
    if (! _path.parent_path().empty())
        std::filesystem::create_directories(_path.parent_path(), dirError);

    FileStamp saved;
    const FileStamp* expected = _stamp.exists ? &_stamp : nullptr;
    const HRESULT writeHr     = WriteBytesAtomic(_path, encoded, expected, &saved, false, nullptr, true, nullptr, MaximumFileBytes);
    if (SUCCEEDED(writeHr))
        _stamp = saved;
    return writeHr;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT CustomDictionary::AddWord(std::wstring_view rawWord, HRESULT* outDiskStatus) noexcept
try
{
    if (outDiskStatus)
        *outDiskStatus = S_OK;

    const std::wstring_view trimmed = TrimLine(rawWord);
    std::wstring key;
    if (! TryNormalizeDictionaryKey(trimmed, key))
        return E_INVALIDARG;

    std::lock_guard lock(_mutex);
    if (_normalizedWords.contains(key))
        return S_OK;
    if (_normalizedWords.size() >= MaximumWords)
        return E_BOUNDS;

    RawLine line;
    line.text          = std::wstring(trimmed);
    line.normalizedKey = key;
    _normalizedWords.insert(key);
    _rawLines.push_back(std::move(line));
    ++_generation;

    const HRESULT diskHr = PersistLocked();
    if (outDiskStatus)
        *outDiskStatus = diskHr;
    return S_OK; // Session word is retained even if disk persistence fails.
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT CustomDictionary::RemoveWord(std::wstring_view rawWord, HRESULT* outDiskStatus) noexcept
try
{
    if (outDiskStatus)
        *outDiskStatus = S_OK;

    std::wstring key;
    if (! TryNormalizeDictionaryKey(rawWord, key))
        return E_INVALIDARG;

    std::lock_guard lock(_mutex);
    if (! _normalizedWords.contains(key))
        return S_FALSE;

    _normalizedWords.erase(key);
    std::erase_if(_rawLines, [&](const RawLine& line) { return line.normalizedKey == key; });
    ++_generation;

    const HRESULT diskHr = PersistLocked();
    if (outDiskStatus)
        *outDiskStatus = diskHr;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

void CustomDictionary::IgnoreAll(std::wstring_view rawWord) noexcept
try
{
    std::wstring key;
    if (! TryNormalizeDictionaryKey(rawWord, key))
        return;
    std::lock_guard lock(_mutex);
    if (_ignoredAll.insert(std::move(key)).second)
        ++_generation;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

void CustomDictionary::IgnoreOnce(unsigned workspaceKey, std::wstring_view rawWord, std::size_t occurrenceKey) noexcept
try
{
    std::wstring key;
    if (! TryNormalizeDictionaryKey(rawWord, key))
        return;
    std::lock_guard lock(_mutex);
    IgnoreOnceEntry entry{workspaceKey, occurrenceKey, std::move(key)};
    if (std::ranges::find(_ignoredOnce, entry) == _ignoredOnce.end())
    {
        _ignoredOnce.push_back(std::move(entry));
        ++_generation;
    }
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

void CustomDictionary::ClearIgnoreOnceForWorkspace(unsigned workspaceKey) noexcept
{
    std::lock_guard lock(_mutex);
    if (std::erase_if(_ignoredOnce, [workspaceKey](const IgnoreOnceEntry& entry) { return entry.workspaceKey == workspaceKey; }) > 0u)
        ++_generation;
}

bool CustomDictionary::IsAccepted(std::wstring_view rawWord, unsigned workspaceKey, std::optional<std::size_t> occurrenceKey) const noexcept
{
    std::wstring key;
    if (! TryNormalizeDictionaryKey(rawWord, key))
        return false;
    return IsAcceptedNormalized(key, workspaceKey, occurrenceKey);
}

bool CustomDictionary::IsAcceptedNormalized(std::wstring_view normalizedKey, unsigned workspaceKey, std::optional<std::size_t> occurrenceKey) const noexcept
try
{
    if (normalizedKey.empty())
        return false;
    std::lock_guard lock(_mutex);
    const std::wstring lookup(normalizedKey);
    if (_normalizedWords.contains(lookup) || _ignoredAll.contains(lookup))
        return true;
    if (occurrenceKey)
    {
        for (const auto& entry : _ignoredOnce)
            if (entry.workspaceKey == workspaceKey && entry.occurrenceKey == *occurrenceKey && entry.normalizedKey == lookup)
                return true;
    }
    return false;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::uint64_t CustomDictionary::Generation() const noexcept
{
    std::lock_guard lock(_mutex);
    return _generation;
}

std::filesystem::path CustomDictionary::Path() const
{
    std::lock_guard lock(_mutex);
    return _path;
}

DictionaryEncoding CustomDictionary::Encoding() const noexcept
{
    std::lock_guard lock(_mutex);
    return _encoding;
}

DictionaryLineEnding CustomDictionary::LineEnding() const noexcept
{
    std::lock_guard lock(_mutex);
    return _lineEnding;
}

std::size_t CustomDictionary::WordCount() const noexcept
{
    std::lock_guard lock(_mutex);
    return _normalizedWords.size();
}

std::vector<std::wstring> CustomDictionary::DisplayWords() const
{
    std::lock_guard lock(_mutex);
    std::vector<std::wstring> words;
    words.reserve(_normalizedWords.size());
    for (const auto& line : _rawLines)
        if (! line.normalizedKey.empty())
            words.push_back(std::wstring(TrimLine(line.text)));
    return words;
}

HRESULT CustomDictionary::LoadStatus() const noexcept
{
    std::lock_guard lock(_mutex);
    return _loadStatus;
}

CustomDictionary::Snapshot CustomDictionary::CaptureSnapshot() const
{
    std::lock_guard lock(_mutex);
    return Snapshot{_generation, _normalizedWords, _ignoredAll};
}
} // namespace RedTilde
