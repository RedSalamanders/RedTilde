#include "CustomDictionary.h"
#include "PathSecurity.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <cwctype>
#include <limits>
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

[[nodiscard]] bool IsValidDictionaryUtf16(std::wstring_view text) noexcept
{
    for (std::size_t i = 0u; i < text.size();)
    {
        const wchar_t ch = text[i];
        if (ch == L'\0')
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
    }
    return true;
}

[[nodiscard]] HRESULT TryDecodeUtf8DictionaryBytes(std::string_view bytes, std::wstring& outText) noexcept
try
{
    outText.clear();
    if (bytes.size() > CustomDictionary::MaximumFileBytes || bytes.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    if (bytes.empty())
        return S_OK;
    if (bytes.find('\0') != std::string_view::npos)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

    const int units = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (units <= 0)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

    std::wstring staged(static_cast<std::size_t>(units), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), staged.data(), units);
    if (written != units || ! IsValidDictionaryUtf16(staged))
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

    outText = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

[[nodiscard]] HRESULT TryEncodeUtf8DictionaryText(std::wstring_view text, bool withBom, std::string& outBytes) noexcept
try
{
    outBytes.clear();
    if (! IsValidDictionaryUtf16(text))
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    const std::size_t bomBytes = withBom ? 3u : 0u;
    if (text.size() > CustomDictionary::MaximumFileBytes - bomBytes || text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    if (text.empty())
    {
        if (withBom)
            outBytes.assign("\xEF\xBB\xBF", 3u);
        return S_OK;
    }

    const int byteCount = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (byteCount <= 0)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    if (static_cast<std::size_t>(byteCount) + bomBytes > CustomDictionary::MaximumFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::string staged(bomBytes + static_cast<std::size_t>(byteCount), '\0');
    if (withBom)
    {
        staged[0] = static_cast<char>(0xEFu);
        staged[1] = static_cast<char>(0xBBu);
        staged[2] = static_cast<char>(0xBFu);
    }
    const int written =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), staged.data() + bomBytes, byteCount, nullptr, nullptr);
    if (written != byteCount)
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

    outBytes = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

[[nodiscard]] bool LooksLikeBomlessUtf16Le(std::string_view bytes, std::wstring& outText) noexcept
try
{
    outText.clear();
    if (bytes.size() < 2u || (bytes.size() % 2u) != 0u || bytes.size() > CustomDictionary::MaximumFileBytes)
        return false;

    const std::size_t unitCount = bytes.size() / 2u;
    std::wstring candidate(unitCount, L'\0');
    std::size_t oddZeros   = 0u;
    std::size_t evenZeros  = 0u;
    std::size_t leNewlines = 0u;
    std::size_t beNewlines = 0u;

    for (std::size_t i = 0u; i < unitCount; ++i)
    {
        const unsigned lo = static_cast<unsigned char>(bytes[2u * i]);
        const unsigned hi = static_cast<unsigned char>(bytes[2u * i + 1u]);
        const wchar_t ch  = static_cast<wchar_t>(lo | (hi << 8u));
        if (ch == L'\0' || ch == 0xFEFFu || ch == 0xFFFEu || (ch < 0x20u && ch != L'\t' && ch != L'\r' && ch != L'\n') || (ch >= 0x7Fu && ch <= 0x9Fu))
            return false;
        if (hi == 0u && lo >= 0x09u)
            ++oddZeros;
        else if (lo == 0u && hi != 0u)
            ++evenZeros;
        if (ch == 0x000Au || ch == 0x000Du)
            ++leNewlines;
        else if (ch == 0x0A00u || ch == 0x0D00u)
            ++beNewlines;
        candidate[i] = ch;
    }

    if (! IsValidDictionaryUtf16(candidate))
        return false;
    if (oddZeros == 0u)
        return false;
    if (! (oddZeros > evenZeros || (oddZeros >= evenZeros && leNewlines > beNewlines)))
        return false;

    outText = std::move(candidate);
    return true;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

[[nodiscard]] HRESULT DecodeDictionaryBytes(
    std::string_view bytes, std::wstring& outText, DictionaryEncoding& outEncoding, DictionaryLineEnding& outLineEnding, bool& outTrailingNewline) noexcept
try
{
    outText.clear();
    outEncoding        = DictionaryEncoding::Utf8;
    outLineEnding      = DictionaryLineEnding::Lf;
    outTrailingNewline = true;
    if (bytes.size() > CustomDictionary::MaximumFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
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
        const HRESULT utf8Hr = TryDecodeUtf8DictionaryBytes(bytes, outText);
        if (FAILED(utf8Hr))
            return utf8Hr;
    }
    else
    {
        // Try strict UTF-8 before falling back to BOM-less UTF-16LE.
        const HRESULT utf8Hr = TryDecodeUtf8DictionaryBytes(bytes, outText);
        if (SUCCEEDED(utf8Hr))
        {
            outEncoding = DictionaryEncoding::Utf8;
        }
        else if (utf8Hr == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
        {
            return utf8Hr;
        }
        else if (LooksLikeBomlessUtf16Le(bytes, outText))
        {
            outEncoding = DictionaryEncoding::Utf16LeNoBom;
        }
        else
        {
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        }
    }

    if (! IsValidDictionaryUtf16(outText))
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
    if (! IsValidDictionaryUtf16(text))
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    switch (encoding)
    {
        case DictionaryEncoding::Utf8: return TryEncodeUtf8DictionaryText(text, false, outBytes);
        case DictionaryEncoding::Utf8Bom: return TryEncodeUtf8DictionaryText(text, true, outBytes);
        case DictionaryEncoding::Utf16LeBom:
        case DictionaryEncoding::Utf16LeNoBom:
        {
            const bool bom             = encoding == DictionaryEncoding::Utf16LeBom;
            const std::size_t bomBytes = bom ? 2u : 0u;
            if (text.size() > (CustomDictionary::MaximumFileBytes - bomBytes) / 2u)
                return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
            outBytes.reserve(bomBytes + text.size() * 2u);
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
            if (text.size() > (CustomDictionary::MaximumFileBytes - 2u) / 2u)
                return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
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

bool TryNormalizeUnicodeNfc(std::wstring_view text, std::wstring& outNfc) noexcept
try
{
    outNfc.clear();
    if (text.empty() || text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) || text.find(L'\0') != text.npos)
        return false;

    int nfcCapacity = NormalizeString(NormalizationC, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (nfcCapacity <= 0)
        return false;
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        if (nfcCapacity <= 0 || nfcCapacity > 4096)
            return false;
        std::wstring nfc(static_cast<std::size_t>(nfcCapacity), L'\0');
        const int nfcWritten = NormalizeString(NormalizationC, text.data(), static_cast<int>(text.size()), nfc.data(), nfcCapacity);
        if (nfcWritten > 0)
        {
            nfc.resize(static_cast<std::size_t>(nfcWritten));
            outNfc = std::move(nfc);
            return true;
        }
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return false;
        nfcCapacity = nfcWritten < 0 ? (std::max)(-nfcWritten, nfcCapacity * 2) : (nfcCapacity * 2);
    }
    return false;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

bool TryNormalizeDictionaryKey(std::wstring_view word, std::wstring& outKey) noexcept
try
{
    outKey.clear();
    const std::wstring_view trimmed = TrimLine(word);
    std::size_t scalarCount{};
    if (! CountValidatedScalarChars(trimmed, scalarCount))
        return false;

    std::wstring nfc;
    if (! TryNormalizeUnicodeNfc(trimmed, nfc))
        return false;

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
    if (path.empty() || ! path.is_absolute() || ! IsValidWin32PathString(path.native()))
        return E_INVALIDARG;

    std::lock_guard lock(_mutex);
    const auto handleFailure = [&](HRESULT err) noexcept -> HRESULT
    {
        // Preserve existing loaded dictionary & _loadStatus when switching to an invalid path.
        const bool keepPrior = ! _path.empty() && SUCCEEDED(_loadStatus) && ! SameRootPath(_path, path);
        if (! keepPrior)
        {
            _path       = path;
            _loadStatus = err;
            ++_generation;
        }
        return err;
    };

    std::string bytes;
    FileStamp newStamp;
    bool reparseRejected = false;
    const HRESULT readHr = ReadBytesNoReparse(path, bytes, &newStamp, MaximumFileBytes, &reparseRejected);
    if (reparseRejected)
        return handleFailure(HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE));
    if (readHr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || readHr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        _path  = path;
        _stamp = {};
        _rawLines.clear();
        _normalizedWords.clear();
        _encoding        = DictionaryEncoding::Utf8;
        _lineEnding      = DictionaryLineEnding::Lf;
        _trailingNewline = true;
        _loadStatus      = S_OK;
        ++_generation;
        return S_OK;
    }
    if (FAILED(readHr))
        return handleFailure(readHr);

    std::wstring text;
    DictionaryEncoding newEncoding     = DictionaryEncoding::Utf8;
    DictionaryLineEnding newLineEnding = DictionaryLineEnding::Lf;
    bool newTrailingNewline            = true;
    const HRESULT decodeHr             = DecodeDictionaryBytes(bytes, text, newEncoding, newLineEnding, newTrailingNewline);
    if (FAILED(decodeHr))
        return handleFailure(decodeHr);

    std::vector<RawLine> parsedLines;
    TransparentWStringSet normalizedSet;
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
                    return handleFailure(HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE));
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

    _path            = path;
    _stamp           = newStamp;
    _encoding        = newEncoding;
    _lineEnding      = newLineEnding;
    _trailingNewline = newTrailingNewline;
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
    if (_path.empty() || _path.parent_path().empty())
        return E_INVALIDARG;
    if (FAILED(_loadStatus))
        return _loadStatus;

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

    wil::unique_handle pinnedParent;
    bool parentReparse  = false;
    const HRESULT dirHr = EnsurePinnedDiskDirectory(_path.parent_path(), pinnedParent, &parentReparse);
    if (FAILED(dirHr))
        return parentReparse ? HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE) : dirHr;

    FileStamp saved;
    const HRESULT writeHr = WriteBytesAtomic(_path, encoded, &_stamp, &saved, false, nullptr, true, nullptr, MaximumFileBytes, true);
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

void CustomDictionary::ClearAllIgnoreOnce() noexcept
{
    std::lock_guard lock(_mutex);
    if (! _ignoredOnce.empty())
    {
        _ignoredOnce.clear();
        ++_generation;
    }
}

bool CustomDictionary::IsAccepted(std::wstring_view rawWord,
                                  unsigned workspaceKey,
                                  std::optional<std::size_t> occurrenceKey,
                                  bool ignoreOnceOnly) const noexcept
{
    std::wstring key;
    if (! TryNormalizeDictionaryKey(rawWord, key))
        return false;
    return IsAcceptedNormalized(key, workspaceKey, occurrenceKey, ignoreOnceOnly);
}

bool CustomDictionary::IsAcceptedNormalizedLocked(std::wstring_view normalizedKey,
                                                  unsigned workspaceKey,
                                                  std::optional<std::size_t> occurrenceKey,
                                                  bool ignoreOnceOnly) const noexcept
{
    if (normalizedKey.empty())
        return false;
    if (! ignoreOnceOnly && (_normalizedWords.contains(normalizedKey) || _ignoredAll.contains(normalizedKey)))
        return true;
    if (occurrenceKey)
    {
        for (const auto& entry : _ignoredOnce)
            if (entry.workspaceKey == workspaceKey && entry.occurrenceKey == *occurrenceKey && entry.normalizedKey == normalizedKey)
                return true;
    }
    return false;
}

bool CustomDictionary::IsAcceptedNormalized(std::wstring_view normalizedKey,
                                            unsigned workspaceKey,
                                            std::optional<std::size_t> occurrenceKey,
                                            bool ignoreOnceOnly) const noexcept
{
    if (normalizedKey.empty())
        return false;
    std::lock_guard lock(_mutex);
    return IsAcceptedNormalizedLocked(normalizedKey, workspaceKey, occurrenceKey, ignoreOnceOnly);
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
