#include "Settings.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <charconv>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <yyjson.h>

namespace RedTilde
{
namespace
{
using unique_yyjson_doc     = wil::unique_any<yyjson_doc*, decltype(&yyjson_doc_free), yyjson_doc_free>;
using unique_yyjson_mut_doc = wil::unique_any<yyjson_mut_doc*, decltype(&yyjson_mut_doc_free), yyjson_mut_doc_free>;
using unique_yyjson_text    = wil::unique_any<char*, decltype(&::free), ::free>;

constexpr std::string_view LegacySettingsHeader  = "RedTilde.Settings/1\n";
constexpr std::wstring_view LegacySettingsFile   = L"RedTilde.settings";
constexpr std::wstring_view ReleaseSettingsFile  = L"RedTilde.settings.json";
constexpr std::wstring_view DebugSettingsFile    = L"RedTilde-debug.settings.json";
constexpr std::uint64_t CurrentSchemaVersion     = 1u;
constexpr std::size_t MaximumSettingsFileBytes   = 16u * 1024u * 1024u;
constexpr yyjson_read_flag SettingsJsonReadFlags = YYJSON_READ_JSON5 | YYJSON_READ_ALLOW_BOM;
constexpr DWORD SettingsWatcherFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION;
constexpr DWORD SettingsWatcherCoalesceMs = 250u;
constexpr DWORD SettingsWatcherRetryMs    = 1000u;

// Indexed by MarkdownSyntax. Missing or invalid keys keep the enabled default.
constexpr std::array<std::string_view, MarkdownSyntaxCount> LegacyMarkdownSyntaxKeys{
    "syntaxHighlight", "syntaxSuperscript", "syntaxSubscript", "syntaxInserted", "syntaxWikiLinks", "syntaxDefinitionLists", "syntaxAbbreviations"};
constexpr std::array<const char*, MarkdownSyntaxCount> JsonMarkdownSyntaxKeys{
    "highlight", "superscript", "subscript", "inserted", "wikiLinks", "definitionLists", "abbreviations"};

bool ValidPreferencePath(const std::filesystem::path& path)
{
    const auto& text = path.native();
    return path.is_absolute() && text.size() <= 32767 && DxUi::RichText::IsValidUtf16(text) &&
           std::ranges::none_of(text, [](wchar_t unit) { return unit < L' '; });
}

bool EqualOrdinalIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() &&
           (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}

std::uint64_t FileTimeValue(FILETIME value) noexcept
{
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32u) | value.dwLowDateTime;
}

HRESULT CompleteSettingsIdentity(HANDLE file, FileStamp& stamp) noexcept
{
    FILE_ID_INFO info{};
    if (GetFileInformationByHandleEx(file, FileIdInfo, &info, sizeof(info)))
    {
        stamp.volume = info.VolumeSerialNumber;
        std::memcpy(&stamp.fileIndex, info.FileId.Identifier, sizeof(stamp.fileIndex));
        std::memcpy(&stamp.fileIndexHigh, info.FileId.Identifier + sizeof(stamp.fileIndex), sizeof(stamp.fileIndexHigh));
        return S_OK;
    }
    const DWORD error = GetLastError();
    if (error == ERROR_INVALID_PARAMETER || error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION)
        return S_OK;
    return HRESULT_FROM_WIN32(error);
}

HRESULT ReadSettingsBytes(const std::filesystem::path& path, std::string& bytes, FileStamp* stamp = nullptr) noexcept
try
{
    wil::unique_handle file(CreateFileW(
        path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (! file)
        return HRESULT_FROM_WIN32(GetLastError());

    BY_HANDLE_FILE_INFORMATION info{};
    if (! GetFileInformationByHandle(file.get(), &info))
        return HRESULT_FROM_WIN32(GetLastError());
    const std::uint64_t fileSize = (static_cast<std::uint64_t>(info.nFileSizeHigh) << 32u) | info.nFileSizeLow;
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fileSize > MaximumSettingsFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::string staged(static_cast<std::size_t>(fileSize), '\0');
    std::size_t offset{};
    while (offset < staged.size())
    {
        DWORD count{};
        const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(staged.size() - offset, 1u << 20u));
        if (! ReadFile(file.get(), staged.data() + offset, chunk, &count, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());
        if (count == 0u)
            return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        offset += count;
    }

    BY_HANDLE_FILE_INFORMATION after{};
    if (! GetFileInformationByHandle(file.get(), &after))
        return HRESULT_FROM_WIN32(GetLastError());

    const auto makeStamp = [&](const BY_HANDLE_FILE_INFORMATION& meta, std::uint64_t hash) noexcept
    {
        return FileStamp{true,
                         (static_cast<std::uint64_t>(meta.nFileSizeHigh) << 32u) | meta.nFileSizeLow,
                         FileTimeValue(meta.ftLastWriteTime),
                         hash,
                         meta.dwVolumeSerialNumber,
                         (static_cast<std::uint64_t>(meta.nFileIndexHigh) << 32u) | meta.nFileIndexLow};
    };

    FileStamp identity = makeStamp(info, ContentHash(staged));
    if (identity != makeStamp(after, identity.hash))
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    const HRESULT idResult = CompleteSettingsIdentity(file.get(), identity);
    if (FAILED(idResult))
        return idResult;

    if (stamp)
        *stamp = identity;
    bytes.swap(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::filesystem::path BackupPathFor(const std::filesystem::path& path)
{
    SYSTEMTIME time{};
    GetLocalTime(&time);
    auto result = path.native();
    result += std::format(
        L".invalid-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    return std::filesystem::path(std::move(result));
}

bool TryReadIntegral(yyjson_val* value, std::int64_t minValue, std::int64_t maxValue, std::int64_t& result) noexcept
{
    if (! value)
        return false;
    if (yyjson_is_sint(value))
    {
        const std::int64_t signedVal = yyjson_get_sint(value);
        if (signedVal < minValue || signedVal > maxValue)
            return false;
        result = signedVal;
        return true;
    }
    if (yyjson_is_uint(value))
    {
        const std::uint64_t unsignedVal = yyjson_get_uint(value);
        if (maxValue < 0 || unsignedVal > static_cast<std::uint64_t>(maxValue))
            return false;
        const auto signedVal = static_cast<std::int64_t>(unsignedVal);
        if (signedVal < minValue)
            return false;
        result = signedVal;
        return true;
    }
    if (yyjson_is_real(value))
    {
        const double realVal = yyjson_get_real(value);
        if (! std::isfinite(realVal) || realVal != std::floor(realVal))
            return false;
        if (realVal < static_cast<double>(minValue) || realVal > static_cast<double>(maxValue))
            return false;
        // Guard against double rounding at the top of int64_t if maxValue approaches 2^63.
        const auto converted = static_cast<std::int64_t>(realVal);
        if (converted < minValue || converted > maxValue)
            return false;
        result = converted;
        return true;
    }
    return false;
}

bool TryReadSchemaVersion(yyjson_val* root, std::uint64_t& schemaVersion) noexcept
{
    yyjson_val* field = yyjson_obj_get(root, "schema");
    if (! field)
        return false;
    if (yyjson_is_uint(field))
    {
        const std::uint64_t value = yyjson_get_uint(field);
        if (value < 1u)
            return false;
        schemaVersion = value;
        return true;
    }
    if (yyjson_is_sint(field))
    {
        const std::int64_t value = yyjson_get_sint(field);
        if (value < 1)
            return false;
        schemaVersion = static_cast<std::uint64_t>(value);
        return true;
    }
    if (yyjson_is_real(field))
    {
        const double realVal = yyjson_get_real(field);
        if (! std::isfinite(realVal) || realVal != std::floor(realVal) || realVal < 1.0 || realVal >= 18446744073709551616.0)
            return false;
        schemaVersion = static_cast<std::uint64_t>(realVal);
        return schemaVersion >= 1u;
    }
    return false;
}

bool TryReadJsonString(yyjson_val* object, const char* key, std::string_view& out) noexcept
{
    yyjson_val* field = yyjson_obj_get(object, key);
    if (! field || ! yyjson_is_str(field))
        return false;
    out = std::string_view(yyjson_get_str(field), yyjson_get_len(field));
    return true;
}

void ReadJsonBool(yyjson_val* object, const char* key, bool& target) noexcept
{
    yyjson_val* field = yyjson_obj_get(object, key);
    if (field && yyjson_is_bool(field))
        target = yyjson_get_bool(field);
}

void ReadJsonPath(yyjson_val* object, const char* key, std::filesystem::path& target)
{
    std::string_view utf8;
    if (! TryReadJsonString(object, key, utf8) || utf8.empty() || utf8.size() > 3u * 32767u)
        return;
    std::wstring decoded;
    if (DxUi::RichText::TryDecodeUtf8(utf8, decoded) == DxUi::RichText::SourceResult::Success)
    {
        std::filesystem::path path(std::move(decoded));
        if (ValidPreferencePath(path))
            target = path.lexically_normal();
    }
}

Appearance ParseAppearanceName(std::string_view name) noexcept
{
    if (name == "light")
        return Appearance::Light;
    if (name == "dark")
        return Appearance::Dark;
    if (name == "paper")
        return Appearance::Paper;
    return Appearance::System;
}

const char* AppearanceName(Appearance appearance) noexcept
{
    switch (appearance)
    {
        case Appearance::Light: return "light";
        case Appearance::Dark: return "dark";
        case Appearance::Paper: return "paper";
        case Appearance::System: break;
    }
    return "system";
}

EditorView ParseViewName(std::string_view name) noexcept
{
    if (name == "source")
        return EditorView::Source;
    if (name == "split")
        return EditorView::Split;
    return EditorView::Compose;
}

const char* ViewName(EditorView view) noexcept
{
    switch (view)
    {
        case EditorView::Source: return "source";
        case EditorView::Split: return "split";
        case EditorView::Compose: break;
    }
    return "compose";
}

yyjson_mut_val* CheckedAlloc(yyjson_mut_val* value) noexcept
{
    if (! value)
        DxUi::Failure::FailAllocation();
    return value;
}

void PutObjectValue(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, yyjson_mut_val* value) noexcept
{
    yyjson_mut_val* keyVal = CheckedAlloc(yyjson_mut_strcpy(doc, key));
    if (! yyjson_mut_obj_put(obj, keyVal, CheckedAlloc(value)))
        DxUi::Failure::FailAllocation();
}

void PutObjectString(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::string_view value) noexcept
{
    PutObjectValue(doc, obj, key, yyjson_mut_strncpy(doc, value.data(), value.size()));
}

void PutObjectBool(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, bool value) noexcept
{
    PutObjectValue(doc, obj, key, yyjson_mut_bool(doc, value));
}

void PutObjectInt(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::int64_t value) noexcept
{
    PutObjectValue(doc, obj, key, yyjson_mut_sint(doc, value));
}

void PutObjectUint(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::uint64_t value) noexcept
{
    PutObjectValue(doc, obj, key, yyjson_mut_uint(doc, value));
}
} // namespace

std::wstring_view SettingsFileName(bool debugBuild) noexcept
{
    return debugBuild ? DebugSettingsFile : ReleaseSettingsFile;
}

std::filesystem::path SettingsPath(const std::filesystem::path& directory, bool debugBuild)
{
    return directory / SettingsFileName(debugBuild);
}

bool ValidWindowPlacement(const WindowPlacementSettings& placement) noexcept
{
    const auto& bounds             = placement.normalBounds;
    constexpr LONG coordinateLimit = 1048576;
    return placement.dpi >= 48u && placement.dpi <= 768u && bounds.left >= -coordinateLimit && bounds.top >= -coordinateLimit &&
           bounds.right <= coordinateLimit && bounds.bottom <= coordinateLimit && bounds.right > bounds.left && bounds.bottom > bounds.top &&
           static_cast<long long>(bounds.right) - bounds.left <= 32768 && static_cast<long long>(bounds.bottom) - bounds.top <= 32768;
}

Settings ParseLegacySettings(std::string_view bytes)
{
    Settings settings;
    if (! bytes.starts_with(LegacySettingsHeader))
        return settings;
    std::array<std::optional<int>, 4> windowBounds;
    std::optional<unsigned> windowDpi{96u};
    bool windowMaximized{};
    while (! bytes.empty())
    {
        const auto end   = bytes.find('\n');
        const auto line  = bytes.substr(0, end);
        const auto split = line.find('=');
        if (split != line.npos)
        {
            const auto key = line.substr(0, split), value = line.substr(split + 1);
            if (key == "windowDpi")
                windowDpi.reset();
            constexpr std::array<std::string_view, 4> windowKeys{"windowLeft", "windowTop", "windowRight", "windowBottom"};
            for (std::size_t index = 0; index < windowKeys.size(); ++index)
                if (key == windowKeys[index])
                {
                    int coordinate{};
                    const auto parsed   = std::from_chars(value.data(), value.data() + value.size(), coordinate);
                    windowBounds[index] = parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() ? std::optional<int>{coordinate} : std::nullopt;
                }
            if ((key == "filesRoot" || key == "lastDocument") && value.size() <= 3u * 32767u)
            {
                std::wstring decoded;
                if (DxUi::RichText::TryDecodeUtf8(value, decoded) == DxUi::RichText::SourceResult::Success)
                {
                    std::filesystem::path path(std::move(decoded));
                    if (ValidPreferencePath(path))
                    {
                        auto& destination = key == "filesRoot" ? settings.filesRoot : settings.lastDocument;
                        destination       = path.lexically_normal();
                    }
                }
            }
            unsigned number{};
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
            if (parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size())
            {
                if (key == "theme" && number <= 3)
                    settings.appearance = static_cast<Appearance>(number);
                if (key == "view" && number <= 2)
                    settings.view = static_cast<EditorView>(number);
                if (key == "outline" && number <= 1)
                    settings.showOutline = number != 0;
                if (key == "files" && number <= 1)
                    settings.showFiles = number != 0;
                if (key == "filesWidth" && number >= 200 && number <= 400)
                    settings.filesWidth = number;
                if (key == "readingWidth" && number >= 480 && number <= 1040)
                    settings.readingWidth = number;
                if (key == "scale" && number >= 75 && number <= 200)
                    settings.textScale = number;
                if (key == "windowDpi" && number >= 48u && number <= 768u)
                    windowDpi = number;
                if (key == "windowMaximized" && number <= 1)
                    windowMaximized = number != 0;
                if (key == "focusDimming" && number <= 1)
                    settings.dimInactiveBlocks = number != 0;
                if (key == "remoteImages" && number <= 1)
                    settings.allowRemoteImages = number != 0;
                if (key == "localNetworkImages" && number <= 1)
                    settings.allowLocalNetworkImages = number != 0;
                if (key == "imageFirstPaste" && number <= 1)
                    settings.imageFirstPaste = number != 0;
                if (key == "strikeCompletedTasks" && number <= 1)
                    settings.strikeCompletedTasks = number != 0;
                for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
                    if (key == LegacyMarkdownSyntaxKeys[index] && number <= 1)
                        settings.markdownSyntax[index] = number != 0;
            }
        }
        if (end == bytes.npos)
            break;
        bytes.remove_prefix(end + 1);
    }
    if (windowDpi && std::ranges::all_of(windowBounds, [](const auto& value) { return value.has_value(); }))
    {
        WindowPlacementSettings placement{
            {windowBounds[0].value(), windowBounds[1].value(), windowBounds[2].value(), windowBounds[3].value()}, windowDpi.value(), windowMaximized};
        if (ValidWindowPlacement(placement))
            settings.windowPlacement = placement;
    }
    // Focus mode is intentionally session-local, so restart never hides commands.
    return settings;
}

HRESULT TryParseSettings(std::string_view bytes, Settings& settings, std::uint64_t* schemaVersion) noexcept
try
{
    if (bytes.empty())
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (bytes.size() > MaximumSettingsFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::string mutableBytes(bytes);
    yyjson_read_err err{};
    const unique_yyjson_doc doc(yyjson_read_opts(mutableBytes.data(), mutableBytes.size(), SettingsJsonReadFlags, nullptr, &err));
    if (! doc)
    {
        if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    yyjson_val* root = yyjson_doc_get_root(doc.get());
    if (! root || ! yyjson_is_obj(root))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    std::uint64_t schema{};
    if (! TryReadSchemaVersion(root, schema))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    Settings parsed;
    if (std::string_view appearance; TryReadJsonString(root, "appearance", appearance))
        parsed.appearance = ParseAppearanceName(appearance);
    if (std::string_view view; TryReadJsonString(root, "view", view))
        parsed.view = ParseViewName(view);

    // Focus mode is intentionally session-local, so restart never hides commands.
    parsed.focusMode = false;

    ReadJsonBool(root, "showOutline", parsed.showOutline);
    ReadJsonBool(root, "showFiles", parsed.showFiles);
    ReadJsonPath(root, "filesRoot", parsed.filesRoot);
    ReadJsonPath(root, "lastDocument", parsed.lastDocument);

    std::int64_t numeric{};
    if (TryReadIntegral(yyjson_obj_get(root, "filesWidth"), 200, 400, numeric))
        parsed.filesWidth = static_cast<unsigned>(numeric);
    if (TryReadIntegral(yyjson_obj_get(root, "readingWidth"), 480, 1040, numeric))
        parsed.readingWidth = static_cast<unsigned>(numeric);
    if (TryReadIntegral(yyjson_obj_get(root, "textScale"), 75, 200, numeric))
        parsed.textScale = static_cast<unsigned>(numeric);

    ReadJsonBool(root, "dimInactiveBlocks", parsed.dimInactiveBlocks);
    ReadJsonBool(root, "allowRemoteImages", parsed.allowRemoteImages);
    ReadJsonBool(root, "allowLocalNetworkImages", parsed.allowLocalNetworkImages);
    ReadJsonBool(root, "imageFirstPaste", parsed.imageFirstPaste);
    ReadJsonBool(root, "strikeCompletedTasks", parsed.strikeCompletedTasks);

    if (yyjson_val* placementObj = yyjson_obj_get(root, "windowPlacement"); placementObj && yyjson_is_obj(placementObj))
    {
        std::int64_t left{}, top{}, right{}, bottom{};
        constexpr std::int64_t minInt = std::numeric_limits<int>::min();
        constexpr std::int64_t maxInt = std::numeric_limits<int>::max();
        if (TryReadIntegral(yyjson_obj_get(placementObj, "left"), minInt, maxInt, left) &&
            TryReadIntegral(yyjson_obj_get(placementObj, "top"), minInt, maxInt, top) &&
            TryReadIntegral(yyjson_obj_get(placementObj, "right"), minInt, maxInt, right) &&
            TryReadIntegral(yyjson_obj_get(placementObj, "bottom"), minInt, maxInt, bottom))
        {
            bool dpiValid    = true;
            std::int64_t dpi = 96;
            if (yyjson_val* dpiVal = yyjson_obj_get(placementObj, "dpi"))
                dpiValid = TryReadIntegral(dpiVal, 48, 768, dpi);

            if (dpiValid)
            {
                bool maximized{};
                ReadJsonBool(placementObj, "maximized", maximized);
                WindowPlacementSettings placement{{static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(right), static_cast<LONG>(bottom)},
                                                  static_cast<unsigned>(dpi),
                                                  maximized};
                if (ValidWindowPlacement(placement))
                    parsed.windowPlacement = placement;
            }
        }
    }

    if (yyjson_val* syntaxObj = yyjson_obj_get(root, "markdownSyntax"); syntaxObj && yyjson_is_obj(syntaxObj))
    {
        for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
            ReadJsonBool(syntaxObj, JsonMarkdownSyntaxKeys[index], parsed.markdownSyntax[index]);
    }

    settings = std::move(parsed);
    if (schemaVersion)
        *schemaVersion = schema;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

Settings ParseSettings(std::string_view bytes)
{
    Settings settings;
    (void)TryParseSettings(bytes, settings);
    return settings;
}

std::string SerializeSettings(const Settings& settings, std::string_view existingJson)
{
    unique_yyjson_mut_doc mutDoc;
    if (! existingJson.empty() && existingJson.size() <= MaximumSettingsFileBytes)
    {
        std::string mutableExisting(existingJson);
        yyjson_read_err err{};
        const unique_yyjson_doc existingDoc(yyjson_read_opts(mutableExisting.data(), mutableExisting.size(), SettingsJsonReadFlags, nullptr, &err));
        if (! existingDoc && err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        if (existingDoc)
        {
            yyjson_val* existingRoot = yyjson_doc_get_root(existingDoc.get());
            std::uint64_t existingSchema{};
            if (existingRoot && yyjson_is_obj(existingRoot) && TryReadSchemaVersion(existingRoot, existingSchema))
            {
                mutDoc.reset(yyjson_doc_mut_copy(existingDoc.get(), nullptr));
                if (! mutDoc)
                    DxUi::Failure::FailAllocation();
            }
        }
    }

    yyjson_mut_val* root = nullptr;
    if (mutDoc)
    {
        root = yyjson_mut_doc_get_root(mutDoc.get());
        if (! root || ! yyjson_mut_is_obj(root))
            mutDoc.reset();
    }
    if (! mutDoc)
    {
        mutDoc.reset(yyjson_mut_doc_new(nullptr));
        if (! mutDoc)
            DxUi::Failure::FailAllocation();
        root = CheckedAlloc(yyjson_mut_obj(mutDoc.get()));
        yyjson_mut_doc_set_root(mutDoc.get(), root);
        PutObjectUint(mutDoc.get(), root, "schema", CurrentSchemaVersion);
    }

    PutObjectString(mutDoc.get(), root, "appearance", AppearanceName(settings.appearance));
    PutObjectString(mutDoc.get(), root, "view", ViewName(settings.view));
    (void)yyjson_mut_obj_remove_key(root, "focusMode");
    PutObjectBool(mutDoc.get(), root, "showOutline", settings.showOutline);
    PutObjectBool(mutDoc.get(), root, "showFiles", settings.showFiles);

    std::string encodedPath;
    if (! settings.filesRoot.empty() && ValidPreferencePath(settings.filesRoot) &&
        DxUi::RichText::TryEncodeUtf8(settings.filesRoot.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutObjectString(mutDoc.get(), root, "filesRoot", encodedPath);
    else
        (void)yyjson_mut_obj_remove_key(root, "filesRoot");

    if (! settings.lastDocument.empty() && ValidPreferencePath(settings.lastDocument) &&
        DxUi::RichText::TryEncodeUtf8(settings.lastDocument.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutObjectString(mutDoc.get(), root, "lastDocument", encodedPath);
    else
        (void)yyjson_mut_obj_remove_key(root, "lastDocument");

    PutObjectUint(mutDoc.get(), root, "filesWidth", settings.filesWidth);
    PutObjectUint(mutDoc.get(), root, "readingWidth", settings.readingWidth);
    PutObjectUint(mutDoc.get(), root, "textScale", settings.textScale);
    PutObjectBool(mutDoc.get(), root, "dimInactiveBlocks", settings.dimInactiveBlocks);
    PutObjectBool(mutDoc.get(), root, "allowRemoteImages", settings.allowRemoteImages);
    PutObjectBool(mutDoc.get(), root, "allowLocalNetworkImages", settings.allowLocalNetworkImages);
    PutObjectBool(mutDoc.get(), root, "imageFirstPaste", settings.imageFirstPaste);
    PutObjectBool(mutDoc.get(), root, "strikeCompletedTasks", settings.strikeCompletedTasks);

    if (settings.windowPlacement && ValidWindowPlacement(settings.windowPlacement.value()))
    {
        const auto& placement        = settings.windowPlacement.value();
        yyjson_mut_val* placementObj = yyjson_mut_obj_get(root, "windowPlacement");
        const bool existingPlacement = placementObj != nullptr && yyjson_mut_is_obj(placementObj);
        if (! existingPlacement)
            placementObj = CheckedAlloc(yyjson_mut_obj(mutDoc.get()));
        PutObjectInt(mutDoc.get(), placementObj, "left", placement.normalBounds.left);
        PutObjectInt(mutDoc.get(), placementObj, "top", placement.normalBounds.top);
        PutObjectInt(mutDoc.get(), placementObj, "right", placement.normalBounds.right);
        PutObjectInt(mutDoc.get(), placementObj, "bottom", placement.normalBounds.bottom);
        PutObjectUint(mutDoc.get(), placementObj, "dpi", placement.dpi);
        PutObjectBool(mutDoc.get(), placementObj, "maximized", placement.maximized);
        if (! existingPlacement)
            PutObjectValue(mutDoc.get(), root, "windowPlacement", placementObj);
    }
    else
    {
        (void)yyjson_mut_obj_remove_key(root, "windowPlacement");
    }

    yyjson_mut_val* syntaxObj = yyjson_mut_obj_get(root, "markdownSyntax");
    const bool existingSyntax = syntaxObj != nullptr && yyjson_mut_is_obj(syntaxObj);
    if (! existingSyntax)
        syntaxObj = CheckedAlloc(yyjson_mut_obj(mutDoc.get()));
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        PutObjectBool(mutDoc.get(), syntaxObj, JsonMarkdownSyntaxKeys[index], settings.markdownSyntax[index]);
    if (! existingSyntax)
        PutObjectValue(mutDoc.get(), root, "markdownSyntax", syntaxObj);

    std::size_t length{};
    yyjson_write_err writeErr{};
    const unique_yyjson_text text(yyjson_mut_write_opts(mutDoc.get(), YYJSON_WRITE_PRETTY, nullptr, &length, &writeErr));
    if (! text)
    {
        if (writeErr.code == YYJSON_WRITE_ERROR_MEMORY_ALLOCATION)
            DxUi::Failure::FailAllocation();
        return {};
    }
    std::string result;
    result.reserve(length + 1u);
    result.append(text.get(), length);
    result.push_back('\n');
    return result;
}

SettingsLoadResult LoadSettingsCold(const std::filesystem::path& directory, bool debugBuild) noexcept
try
{
    SettingsLoadResult outcome;
    const auto path = SettingsPath(directory, debugBuild);

    std::string bytes;
    FileStamp readStamp;
    const HRESULT readResult = ReadSettingsBytes(path, bytes, &readStamp);
    if (SUCCEEDED(readResult))
    {
        Settings parsed;
        const HRESULT parseResult = TryParseSettings(bytes, parsed);
        if (SUCCEEDED(parseResult))
        {
            outcome.settings = std::move(parsed);
            outcome.status   = SettingsLoadStatus::Loaded;
            outcome.stamp    = readStamp;
            return outcome;
        }

        outcome.status    = SettingsLoadStatus::Invalid;
        const auto backup = BackupPathFor(path);
        if (MoveFileExW(path.c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            outcome.backupPath     = backup;
            const std::string text = SerializeSettings(outcome.settings);
            const FileStamp missingStamp{};
            outcome.templateWritten = SUCCEEDED(WriteBytesAtomic(path, text, &missingStamp, &outcome.stamp));
        }
        return outcome;
    }

    if (readResult != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && readResult != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        outcome.status = SettingsLoadStatus::Unreadable;
        return outcome;
    }

    const auto legacyPath = directory / LegacySettingsFile;
    std::string legacyBytes;
    if (SUCCEEDED(ReadSettingsBytes(legacyPath, legacyBytes)) && legacyBytes.starts_with(LegacySettingsHeader))
    {
        outcome.settings = ParseLegacySettings(legacyBytes);
        outcome.status   = SettingsLoadStatus::Migrated;
    }
    else
    {
        outcome.status = SettingsLoadStatus::Missing;
    }

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (! error)
    {
        const std::string text = SerializeSettings(outcome.settings);
        const FileStamp missingStamp{};
        outcome.templateWritten = SUCCEEDED(WriteBytesAtomic(path, text, &missingStamp, &outcome.stamp));
    }
    return outcome;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::filesystem::filesystem_error&)
{
    return SettingsLoadResult{{}, SettingsLoadStatus::Unreadable, {}, false, {}};
}

HRESULT LoadSettings(const std::filesystem::path& directory, Settings& settings, bool debugBuild) noexcept
{
    return LoadSettingsLive(directory, settings, nullptr, debugBuild);
}

HRESULT LoadSettingsLive(const std::filesystem::path& directory, Settings& settings, FileStamp* stamp, bool debugBuild) noexcept
try
{
    std::string bytes;
    FileStamp readStamp;
    const HRESULT readResult = ReadSettingsBytes(SettingsPath(directory, debugBuild), bytes, &readStamp);
    if (FAILED(readResult))
        return readResult;
    if (stamp)
        *stamp = readStamp;
    return TryParseSettings(bytes, settings);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::filesystem::filesystem_error& error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}

HRESULT SaveSettings(const std::filesystem::path& directory, const Settings& settings, bool debugBuild, FileStamp* savedStamp) noexcept
try
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));

    const auto path = SettingsPath(directory, debugBuild);
    std::string existing;
    FileStamp stamp;
    const HRESULT read = ReadSettingsBytes(path, existing, &stamp);
    if (FAILED(read) && read != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && read != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        return read;

    std::string_view mergeSource;
    const FileStamp* expectedStamp = &stamp;
    if (SUCCEEDED(read))
    {
        Settings probe;
        const HRESULT parseResult = TryParseSettings(existing, probe);
        if (FAILED(parseResult))
            return parseResult;
        mergeSource = existing;
    }

    const std::string text = SerializeSettings(settings, mergeSource);
    return WriteBytesAtomic(path, text, expectedStamp, savedStamp);
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::filesystem::filesystem_error& error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct SettingsWriter::State
{
    enum class WorkerPhase : std::uint8_t
    {
        Idle,
        Active,
        Closed
    };
    mutable std::mutex mutex;
    std::filesystem::path directory;
    std::optional<Settings> pending;
    std::optional<FileStamp> lastWrittenStamp;
    HWND window{};
    UINT message{};
    WorkerPhase phase{WorkerPhase::Idle};
    HRESULT status = S_OK;
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE, void* context) noexcept
    {
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& shared = ownership->state;
        for (;;)
        {
            Settings next;
            {
                std::lock_guard lock(shared->mutex);
                if (! shared->pending)
                {
                    if (shared->phase != WorkerPhase::Closed)
                        shared->phase = WorkerPhase::Idle;
                    if (shared->window)
                        (void)PostMessageW(shared->window, shared->message, 0, 0);
                    return;
                }
                next = shared->pending.value();
                shared->pending.reset();
            }
            FileStamp savedStamp;
            const HRESULT result = SaveSettings(shared->directory, next, DefaultDebugSettingsBuild, &savedStamp);
            {
                std::lock_guard lock(shared->mutex);
                shared->status = result;
                if (SUCCEEDED(result) && savedStamp.exists)
                    shared->lastWrittenStamp = savedStamp;
            }
        }
    }
};
#pragma warning(pop)

SettingsWriter::~SettingsWriter()
{
    Close();
}
void SettingsWriter::Initialize(std::filesystem::path directory, HWND window, UINT message)
{
    Close();
    _state            = std::make_shared<State>();
    _state->directory = std::move(directory);
    _state->window    = window;
    _state->message   = message;
}
HRESULT SettingsWriter::Submit(const Settings& settings) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->phase == State::WorkerPhase::Closed)
        return E_UNEXPECTED;
    _state->pending = settings;
    if (_state->phase == State::WorkerPhase::Active)
        return S_OK;
    auto ownership          = std::make_unique<ProcessWorkItem<State>>(_state);
    const HRESULT admission = ProcessWork::Acquire(ownership->lease);
    if (FAILED(admission))
    {
        _state->status = admission;
        return admission;
    }
    _state->phase = State::WorkerPhase::Active;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
    {
        _state->phase  = State::WorkerPhase::Idle;
        _state->status = HRESULT_FROM_WIN32(GetLastError());
        return _state->status;
    }
    (void)ownership.release();
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
bool SettingsWriter::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->phase == State::WorkerPhase::Active;
}
HRESULT SettingsWriter::Status() const noexcept
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    return _state->status;
}
std::optional<FileStamp> SettingsWriter::LastWrittenStamp() const noexcept
{
    if (! _state)
        return std::nullopt;
    std::lock_guard lock(_state->mutex);
    return _state->lastWrittenStamp;
}
void SettingsWriter::Close() noexcept
{
    if (! _state)
        return;
    {
        std::lock_guard lock(_state->mutex);
        _state->window = nullptr;
        _state->phase  = State::WorkerPhase::Closed;
    }
    _state.reset();
}

SettingsWatcher::~SettingsWatcher()
{
    Close();
}

HRESULT SettingsWatcher::Start(std::filesystem::path directory, std::wstring fileName, HWND window, UINT message) noexcept
try
{
    Close();
    if (directory.empty() || fileName.empty() || ! window || ! IsWindow(window) || message < WM_APP)
        return E_INVALIDARG;

    _stopEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! _stopEvent)
    {
        const DWORD error = GetLastError();
        Close();
        return error != ERROR_SUCCESS ? HRESULT_FROM_WIN32(error) : E_FAIL;
    }

    ProcessWorkLease lease;
    const HRESULT admission = ProcessWork::Acquire(lease);
    if (FAILED(admission))
    {
        Close();
        return admission;
    }

    _directory = std::move(directory);
    _fileName  = std::move(fileName);
    _window    = window;
    _message   = message;
    _notificationPending.store(false, std::memory_order_release);

    _thread = std::jthread([this, ownedLease = std::move(lease)](std::stop_token) mutable noexcept { WatchThread(std::move(ownedLease)); });
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::system_error& error)
{
    Close();
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}

void SettingsWatcher::Acknowledge() noexcept
{
    _notificationPending.store(false, std::memory_order_release);
}

void SettingsWatcher::Close() noexcept
{
    if (_stopEvent)
        (void)SetEvent(_stopEvent.get());
    if (_thread.joinable())
        _thread.join();
    _thread = std::jthread{};
    _stopEvent.reset();
    _notificationPending.store(false, std::memory_order_release);
    _directory.clear();
    _fileName.clear();
    _window  = nullptr;
    _message = 0;
}

bool SettingsWatcher::Active() const noexcept
{
    return _thread.joinable();
}

void SettingsWatcher::WatchThread(ProcessWorkLease lease) noexcept
{
    const ProcessWorkLease scopedLease = std::move(lease);
    (void)scopedLease;

    wil::unique_handle directoryHandle;
    wil::unique_event_nothrow ioEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! ioEvent)
        return;

    OVERLAPPED overlapped{};
    bool ioPending = false;
    alignas(DWORD) std::array<std::byte, 16u * 1024u> buffer{};

    const auto cancelPending = wil::scope_exit([&]
    {
        if (ioPending && directoryHandle)
        {
            (void)CancelIoEx(directoryHandle.get(), &overlapped);
            DWORD bytes{};
            (void)GetOverlappedResult(directoryHandle.get(), &overlapped, &bytes, TRUE);
            ioPending = false;
        }
    });

    const auto armWatch = [&]() noexcept -> bool
    {
        if (! directoryHandle || ioPending)
            return ioPending;
        ResetEvent(ioEvent.get());
        overlapped        = {};
        overlapped.hEvent = ioEvent.get();
        if (ReadDirectoryChangesW(
                directoryHandle.get(), buffer.data(), static_cast<DWORD>(buffer.size()), FALSE, SettingsWatcherFilter, nullptr, &overlapped, nullptr))
        {
            ioPending = true;
            return true;
        }
        directoryHandle.reset();
        return false;
    };

    const auto matchesTarget = [&](DWORD bytes) noexcept -> bool
    {
        if (bytes == 0u)
            return true;
        for (std::size_t offset{}; offset < bytes;)
        {
            constexpr std::size_t headerSize = offsetof(FILE_NOTIFY_INFORMATION, FileName);
            if (bytes - offset < headerSize)
                return true;
            const auto* entry = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer.data() + offset);
            if (entry->FileNameLength % sizeof(wchar_t) != 0u || entry->FileNameLength > bytes - offset - headerSize)
                return true;
            const std::wstring_view changedName(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
            if (EqualOrdinalIgnoreCase(changedName, _fileName))
                return true;
            if (entry->NextEntryOffset == 0u)
                break;
            if (entry->NextEntryOffset % alignof(DWORD) != 0u || entry->NextEntryOffset < headerSize || entry->NextEntryOffset > bytes - offset)
                return true;
            offset += entry->NextEntryOffset;
        }
        return false;
    };

    for (;;)
    {
        if (! directoryHandle)
        {
            directoryHandle.reset(CreateFileW(_directory.c_str(),
                                              FILE_LIST_DIRECTORY,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              nullptr,
                                              OPEN_EXISTING,
                                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                              nullptr));
            if (! directoryHandle || ! armWatch())
            {
                directoryHandle.reset();
                if (WaitForSingleObject(_stopEvent.get(), SettingsWatcherRetryMs) == WAIT_OBJECT_0)
                    return;
                continue;
            }
        }

        HANDLE handles[]{_stopEvent.get(), ioEvent.get()};
        const DWORD waitResult = WaitForMultipleObjects(2u, handles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
            return;
        if (waitResult != WAIT_OBJECT_0 + 1u)
            return;

        DWORD bytes{};
        bool matched{};
        if (! GetOverlappedResult(directoryHandle.get(), &overlapped, &bytes, FALSE))
        {
            const DWORD error = GetLastError();
            ioPending         = false;
            if (error == ERROR_NOTIFY_ENUM_DIR)
            {
                matched = true;
            }
            else
            {
                directoryHandle.reset();
                if (WaitForSingleObject(_stopEvent.get(), SettingsWatcherRetryMs) == WAIT_OBJECT_0)
                    return;
                continue;
            }
        }
        else
        {
            ioPending = false;
            matched   = matchesTarget(bytes);
        }

        if (! matched)
        {
            if (! armWatch())
            {
                if (WaitForSingleObject(_stopEvent.get(), SettingsWatcherRetryMs) == WAIT_OBJECT_0)
                    return;
            }
            continue;
        }

        // Coalesce multi-write bursts over SettingsWatcherCoalesceMs while keeping
        // the directory watch armed and responsive to stop signals.
        const ULONGLONG deadline = GetTickCount64() + SettingsWatcherCoalesceMs;
        while (armWatch())
        {
            const ULONGLONG now = GetTickCount64();
            if (now >= deadline)
                break;
            const DWORD remaining = static_cast<DWORD>(deadline - now);
            const DWORD burstWait = WaitForMultipleObjects(2u, handles, FALSE, remaining);
            if (burstWait == WAIT_OBJECT_0)
                return;
            if (burstWait == WAIT_TIMEOUT)
                break;
            if (burstWait != WAIT_OBJECT_0 + 1u)
                return;

            DWORD burstBytes{};
            if (! GetOverlappedResult(directoryHandle.get(), &overlapped, &burstBytes, FALSE))
            {
                const DWORD error = GetLastError();
                ioPending         = false;
                if (error != ERROR_NOTIFY_ENUM_DIR)
                {
                    directoryHandle.reset();
                    break;
                }
            }
            else
            {
                ioPending = false;
            }
        }

        PostChanged();
    }
}

void SettingsWatcher::PostChanged() noexcept
{
    if (_notificationPending.exchange(true, std::memory_order_acq_rel))
        return;
    if (! PostMessageW(_window, _message, 0, 0))
        _notificationPending.store(false, std::memory_order_release);
}
} // namespace RedTilde
