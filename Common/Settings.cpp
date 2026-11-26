#include "Settings.h"
#include "DirectoryWatch.h"
#include "JsonHelpers.h"
#include "PathSecurity.h"
#include "TextUtilities.h"
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
#include <span>
#include <stdexcept>
#include <yyjson.h>

namespace RedTilde
{
namespace
{
constexpr std::wstring_view LegacySettingsFile   = L"RedTilde.settings";
constexpr std::wstring_view ReleaseSettingsFile  = L"RedTilde.settings.json";
constexpr std::wstring_view DebugSettingsFile    = L"RedTilde-debug.settings.json";
constexpr std::uint64_t CurrentSchemaVersion     = 1u;
constexpr std::size_t MaximumSettingsFileBytes   = 16u * 1024u * 1024u;
constexpr yyjson_read_flag SettingsJsonReadFlags = (YYJSON_READ_JSON5 & ~YYJSON_READ_ALLOW_INF_AND_NAN) | YYJSON_READ_ALLOW_BOM;
static_assert((SettingsJsonReadFlags & YYJSON_READ_INSITU) == 0u);
constexpr DWORD SettingsWatcherFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION;
constexpr DWORD SettingsWatcherCoalesceMs = 250u;
constexpr DWORD SettingsWatcherRetryMs    = 1000u;

[[nodiscard]] bool HasLegacySettingsHeader(std::string_view bytes) noexcept
{
    return bytes.starts_with("RedTilde.Settings/1\n") || bytes.starts_with("RedTilde.Settings/1\r\n");
}

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

std::filesystem::path BackupPathFor(const std::filesystem::path& path)
{
    SYSTEMTIME time{};
    GetLocalTime(&time);
    auto result = path.native();
    result += std::format(
        L".invalid-{:04}{:02}{:02}-{:02}{:02}{:02}-{:03}", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    return std::filesystem::path(std::move(result));
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

PanePinMode ParsePanePinModeName(std::string_view name) noexcept
{
    if (name == "pinned")
        return PanePinMode::Pinned;
    if (name == "unpinned")
        return PanePinMode::Unpinned;
    return PanePinMode::Auto;
}

const char* PanePinModeName(PanePinMode mode) noexcept
{
    switch (mode)
    {
        case PanePinMode::Pinned: return "pinned";
        case PanePinMode::Unpinned: return "unpinned";
        case PanePinMode::Auto: break;
    }
    return "auto";
}

SpellEngine ParseSpellEngineName(std::string_view name) noexcept
{
    if (name == "windows")
        return SpellEngine::Windows;
    return SpellEngine::Auto;
}

const char* SpellEngineName(SpellEngine engine) noexcept
{
    switch (engine)
    {
        case SpellEngine::Windows: return "windows";
        case SpellEngine::Auto: break;
    }
    return "auto";
}

ExitUnsavedBehavior ParseExitUnsavedBehavior(std::string_view name) noexcept
{
    if (name == "keepAndResume")
        return ExitUnsavedBehavior::KeepAndResume;
    return ExitUnsavedBehavior::Prompt;
}

const char* ExitUnsavedBehaviorName(ExitUnsavedBehavior behavior) noexcept
{
    switch (behavior)
    {
        case ExitUnsavedBehavior::KeepAndResume: return "keepAndResume";
        case ExitUnsavedBehavior::Prompt: break;
    }
    return "prompt";
}

void AppendUniqueFilesRoot(std::vector<std::filesystem::path>& roots, std::filesystem::path candidate)
{
    if (roots.size() >= MaximumFilesRoots || candidate.empty() || ! ValidPreferencePath(candidate))
        return;
    candidate      = NormalizeRootPath(candidate);
    const bool dup = std::ranges::any_of(roots, [&](const std::filesystem::path& existing) { return EqualNormalizedRootPath(existing, candidate); });
    if (! dup)
        roots.push_back(std::move(candidate));
}
} // namespace

bool IsValidGitHubTokenSegment(std::wstring_view segment, std::size_t maximumLength) noexcept
{
    if (segment.empty() || segment.size() > maximumLength || ! DxUi::RichText::IsValidUtf16(segment) || segment == L"." || segment == L"..")
        return false;
    for (const wchar_t ch : segment)
    {
        const bool ok = (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9') || ch == L'-' || ch == L'_' || ch == L'.';
        if (! ok)
            return false;
    }
    return true;
}

bool IsValidGitHubBranchName(std::wstring_view branch) noexcept
{
    if (branch.empty() || branch.size() > 256u || ! DxUi::RichText::IsValidUtf16(branch))
        return false;
    if (branch.front() == L'/' || branch.back() == L'/' || branch.find(L"//") != std::wstring_view::npos || branch.find(L"..") != std::wstring_view::npos ||
        branch.find(L"@{") != std::wstring_view::npos)
        return false;
    for (const wchar_t ch : branch)
    {
        const auto code = static_cast<unsigned>(ch);
        if (code <= 0x20u || code == 0x7Fu || (code >= 0x80u && code <= 0x9Fu) || (code >= 0x200Bu && code <= 0x200Fu) ||
            (code >= 0x202Au && code <= 0x202Eu) || (code >= 0x2066u && code <= 0x2069u) || code == 0xFEFFu || ch == L'\\' || ch == L'?' || ch == L'#' ||
            ch == L'~' || ch == L'^' || ch == L':' || ch == L'*' || ch == L'[' || ch == L'%' || ch == L'"' || ch == L'<' || ch == L'>' || ch == L'|')
            return false;
    }
    std::size_t start{};
    while (start < branch.size())
    {
        const auto slash = branch.find(L'/', start);
        const auto part  = slash == std::wstring_view::npos ? branch.substr(start) : branch.substr(start, slash - start);
        if (part.empty() || part == L"." || part == L".." || part.front() == L'.' || EndsWithOrdinalIgnoreCase(part, L".lock"))
            return false;
        if (slash == std::wstring_view::npos)
            break;
        start = slash + 1u;
    }
    return true;
}

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
    if (! HasLegacySettingsHeader(bytes))
        return settings;
    std::array<std::optional<int>, 4> windowBounds;
    std::optional<unsigned> windowDpi{96u};
    bool windowMaximized{};
    while (! bytes.empty())
    {
        const auto end = bytes.find('\n');
        auto line      = bytes.substr(0, end);
        if (line.ends_with('\r'))
            line.remove_suffix(1);
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
    if (! settings.filesRoot.empty())
    {
        AppendUniqueFilesRoot(settings.filesRoots, settings.filesRoot);
        settings.filesRoot = settings.filesRoots.empty() ? std::filesystem::path{} : settings.filesRoots.front();
    }
    // Focus mode is intentionally session-local, so restart never hides commands.
    return settings;
}

static HRESULT TryParseSettingsDocument(std::string_view bytes, Settings& settings, std::uint64_t* schemaVersion, SettingsLoadStatus& status) noexcept
try
{
    status = SettingsLoadStatus::Invalid;
    if (bytes.empty())
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (bytes.size() > MaximumSettingsFileBytes)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    const unique_yyjson_doc doc = ParseJsonDocument(bytes, SettingsJsonReadFlags);
    if (! doc)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    status           = SettingsLoadStatus::Incompatible;
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
    if (yyjson_val* rootsArray = yyjson_obj_get(root, "filesRoots"); rootsArray && yyjson_is_arr(rootsArray))
    {
        std::size_t idx{}, maxIdx{};
        yyjson_val* item{};
        yyjson_arr_foreach(rootsArray, idx, maxIdx, item)
        {
            if (! item || ! yyjson_is_str(item))
                continue;
            const std::string_view utf8(yyjson_get_str(item), yyjson_get_len(item));
            if (utf8.empty() || utf8.size() > 3u * 32767u)
                continue;
            std::wstring decoded;
            if (DxUi::RichText::TryDecodeUtf8(utf8, decoded) == DxUi::RichText::SourceResult::Success)
                AppendUniqueFilesRoot(parsed.filesRoots, std::filesystem::path(std::move(decoded)));
        }
        parsed.filesRoot = parsed.filesRoots.empty() ? std::filesystem::path{} : parsed.filesRoots.front();
    }
    else if (! parsed.filesRoot.empty())
    {
        AppendUniqueFilesRoot(parsed.filesRoots, parsed.filesRoot);
        parsed.filesRoot = parsed.filesRoots.empty() ? std::filesystem::path{} : parsed.filesRoots.front();
    }
    if (yyjson_val* reposArray = yyjson_obj_get(root, "githubRepos"); reposArray && yyjson_is_arr(reposArray))
    {
        std::size_t idx{}, maxIdx{};
        yyjson_val* item{};
        yyjson_arr_foreach(reposArray, idx, maxIdx, item)
        {
            if (! item || ! yyjson_is_obj(item))
                continue;
            std::string_view ownerU8, repoU8, branchU8;
            if (! TryReadJsonString(item, "owner", ownerU8) || ! TryReadJsonString(item, "repo", repoU8))
                continue;
            (void)TryReadJsonString(item, "branch", branchU8);
            std::wstring owner, repo, branch;
            if (DxUi::RichText::TryDecodeUtf8(ownerU8, owner) != DxUi::RichText::SourceResult::Success ||
                DxUi::RichText::TryDecodeUtf8(repoU8, repo) != DxUi::RichText::SourceResult::Success ||
                DxUi::RichText::TryDecodeUtf8(branchU8, branch) != DxUi::RichText::SourceResult::Success)
                continue;
            if (! IsValidGitHubTokenSegment(owner, 128u) || ! IsValidGitHubTokenSegment(repo, 128u) || (! branch.empty() && ! IsValidGitHubBranchName(branch)))
                continue;
            const bool dup = std::ranges::any_of(parsed.githubRepos, [&](const GitHubRepoRef& existing) {
                return EqualOrdinalIgnoreCase(existing.owner, owner) && EqualOrdinalIgnoreCase(existing.repo, repo) && existing.branch == branch;
            });
            if (! dup)
            {
                parsed.githubRepos.push_back({std::move(owner), std::move(repo), std::move(branch)});
                if (parsed.githubRepos.size() >= MaximumGitHubRepos)
                    break;
            }
        }
    }
    if (std::filesystem::path rsPath; ReadJsonPath(root, "redSalamanderPath", rsPath), ! rsPath.empty())
        parsed.redSalamanderPath = std::move(rsPath);
    if (std::string_view exitMode; TryReadJsonString(root, "exitUnsavedBehavior", exitMode))
        parsed.exitUnsavedBehavior = ParseExitUnsavedBehavior(exitMode);
    ReadJsonPath(root, "lastDocument", parsed.lastDocument);

    std::int64_t numeric{};
    if (TryReadJsonIntegral(yyjson_obj_get(root, "filesWidth"), 200, 400, numeric))
        parsed.filesWidth = static_cast<unsigned>(numeric);
    if (TryReadJsonIntegral(yyjson_obj_get(root, "readingWidth"), 480, 1040, numeric))
        parsed.readingWidth = static_cast<unsigned>(numeric);
    if (TryReadJsonIntegral(yyjson_obj_get(root, "textScale"), 75, 200, numeric))
        parsed.textScale = static_cast<unsigned>(numeric);

    ReadJsonBool(root, "dimInactiveBlocks", parsed.dimInactiveBlocks);
    ReadJsonBool(root, "allowRemoteImages", parsed.allowRemoteImages);
    ReadJsonBool(root, "allowLocalNetworkImages", parsed.allowLocalNetworkImages);
    ReadJsonBool(root, "imageFirstPaste", parsed.imageFirstPaste);
    ReadJsonBool(root, "strikeCompletedTasks", parsed.strikeCompletedTasks);
    ReadJsonBool(root, "splitScrollSync", parsed.splitScrollSync);
    if (std::string_view pinMode; TryReadJsonString(root, "filesPinMode", pinMode))
        parsed.filesPinMode = ParsePanePinModeName(pinMode);
    if (std::string_view pinMode; TryReadJsonString(root, "outlinePinMode", pinMode))
        parsed.outlinePinMode = ParsePanePinModeName(pinMode);

    if (yyjson_val* placementObj = yyjson_obj_get(root, "windowPlacement"); placementObj && yyjson_is_obj(placementObj))
    {
        std::int64_t left{}, top{}, right{}, bottom{};
        constexpr std::int64_t minInt = std::numeric_limits<int>::min();
        constexpr std::int64_t maxInt = std::numeric_limits<int>::max();
        if (TryReadJsonIntegral(yyjson_obj_get(placementObj, "left"), minInt, maxInt, left) &&
            TryReadJsonIntegral(yyjson_obj_get(placementObj, "top"), minInt, maxInt, top) &&
            TryReadJsonIntegral(yyjson_obj_get(placementObj, "right"), minInt, maxInt, right) &&
            TryReadJsonIntegral(yyjson_obj_get(placementObj, "bottom"), minInt, maxInt, bottom))
        {
            bool dpiValid    = true;
            std::int64_t dpi = 96;
            if (yyjson_val* dpiVal = yyjson_obj_get(placementObj, "dpi"))
                dpiValid = TryReadJsonIntegral(dpiVal, 48, 768, dpi);

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

    if (yyjson_val* spellingObj = yyjson_obj_get(root, "spelling"); spellingObj && yyjson_is_obj(spellingObj))
    {
        ReadJsonBool(spellingObj, "enabled", parsed.spelling.enabled);
        ReadJsonBool(spellingObj, "ignoreUppercase", parsed.spelling.ignoreUppercase);
        ReadJsonBool(spellingObj, "ignoreWordsWithDigits", parsed.spelling.ignoreWordsWithDigits);
        ReadJsonBool(spellingObj, "ignoreInternetAndPaths", parsed.spelling.ignoreInternetAndPaths);
        ReadJsonPath(spellingObj, "customDictionaryPath", parsed.spelling.customDictionaryPath);
        ReadJsonPath(spellingObj, "dictionariesFolder", parsed.spelling.dictionariesFolder);
        if (std::string_view engineName; TryReadJsonString(spellingObj, "engine", engineName))
            parsed.spelling.engine = ParseSpellEngineName(engineName);

        if (yyjson_val* langArray = yyjson_obj_get(spellingObj, "languages"); langArray && yyjson_is_arr(langArray))
        {
            std::vector<std::wstring> languages;
            std::size_t idx{}, maxIdx{};
            yyjson_val* item{};
            yyjson_arr_foreach(langArray, idx, maxIdx, item)
            {
                if (! item || ! yyjson_is_str(item))
                    continue;
                const std::string_view rawTag(yyjson_get_str(item), yyjson_get_len(item));
                std::wstring wideTag;
                if (! TryParseAsciiBcp47Tag(rawTag, wideTag))
                    continue;
                const bool duplicate = std::ranges::any_of(languages, [&](const std::wstring& existing) { return EqualOrdinalIgnoreCase(existing, wideTag); });
                if (! duplicate)
                {
                    languages.push_back(std::move(wideTag));
                    if (languages.size() >= MaximumSpellingLanguages)
                        break;
                }
            }
            if (! languages.empty())
                parsed.spelling.languages = std::move(languages);
        }
    }

    settings = std::move(parsed);
    if (schemaVersion)
        *schemaVersion = schema;
    status = SettingsLoadStatus::Loaded;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT TryParseSettings(std::string_view bytes, Settings& settings, std::uint64_t* schemaVersion) noexcept
{
    SettingsLoadStatus status{};
    return TryParseSettingsDocument(bytes, settings, schemaVersion, status);
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
        const unique_yyjson_doc existingDoc = ParseJsonDocument(existingJson, SettingsJsonReadFlags);
        if (existingDoc)
        {
            yyjson_val* existingRoot = yyjson_doc_get_root(existingDoc.get());
            std::uint64_t existingSchema{};
            if (existingRoot && yyjson_is_obj(existingRoot) && TryReadSchemaVersion(existingRoot, existingSchema))
                mutDoc = CopyMutableJsonDoc(existingDoc.get());
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
        mutDoc = CreateMutableJsonDoc();
        root   = CreateMutableJsonObject(mutDoc.get());
        yyjson_mut_doc_set_root(mutDoc.get(), root);
        PutJsonObjectUint(mutDoc.get(), root, "schema", CurrentSchemaVersion);
    }

    PutJsonObjectString(mutDoc.get(), root, "appearance", AppearanceName(settings.appearance));
    PutJsonObjectString(mutDoc.get(), root, "view", ViewName(settings.view));
    (void)yyjson_mut_obj_remove_key(root, "focusMode");
    PutJsonObjectBool(mutDoc.get(), root, "showOutline", settings.showOutline);
    PutJsonObjectBool(mutDoc.get(), root, "showFiles", settings.showFiles);

    std::string encodedPath;
    std::vector<std::filesystem::path> normalizedRoots;
    for (const auto& candidate : settings.filesRoots)
        AppendUniqueFilesRoot(normalizedRoots, candidate);
    if (normalizedRoots.empty() && ! settings.filesRoot.empty())
        AppendUniqueFilesRoot(normalizedRoots, settings.filesRoot);

    const std::filesystem::path primaryRoot = normalizedRoots.empty() ? std::filesystem::path{} : normalizedRoots.front();
    if (! primaryRoot.empty() && ValidPreferencePath(primaryRoot) &&
        DxUi::RichText::TryEncodeUtf8(primaryRoot.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutJsonObjectString(mutDoc.get(), root, "filesRoot", encodedPath);
    else
        (void)yyjson_mut_obj_remove_key(root, "filesRoot");

    if (! normalizedRoots.empty())
    {
        yyjson_mut_val* rootsArr = CreateMutableJsonArray(mutDoc.get());
        for (const auto& r : normalizedRoots)
        {
            if (DxUi::RichText::TryEncodeUtf8(r.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
                AppendJsonArrayString(mutDoc.get(), rootsArr, encodedPath);
        }
        PutJsonObjectValue(mutDoc.get(), root, "filesRoots", rootsArr);
    }
    else
    {
        (void)yyjson_mut_obj_remove_key(root, "filesRoots");
    }

    if (! settings.githubRepos.empty())
    {
        yyjson_mut_val* reposArr = CreateMutableJsonArray(mutDoc.get());
        std::size_t count{};
        for (const auto& repoRef : settings.githubRepos)
        {
            if (count >= MaximumGitHubRepos || ! IsValidGitHubTokenSegment(repoRef.owner, 128u) || ! IsValidGitHubTokenSegment(repoRef.repo, 128u) ||
                (! repoRef.branch.empty() && ! IsValidGitHubBranchName(repoRef.branch)))
                continue;
            std::string ownerU8, repoU8, branchU8;
            if (DxUi::RichText::TryEncodeUtf8(repoRef.owner, ownerU8) == DxUi::RichText::SourceResult::Success &&
                DxUi::RichText::TryEncodeUtf8(repoRef.repo, repoU8) == DxUi::RichText::SourceResult::Success &&
                DxUi::RichText::TryEncodeUtf8(repoRef.branch, branchU8) == DxUi::RichText::SourceResult::Success)
            {
                yyjson_mut_val* repoObj = CreateMutableJsonObject(mutDoc.get());
                PutJsonObjectString(mutDoc.get(), repoObj, "owner", ownerU8);
                PutJsonObjectString(mutDoc.get(), repoObj, "repo", repoU8);
                if (! branchU8.empty())
                    PutJsonObjectString(mutDoc.get(), repoObj, "branch", branchU8);
                AppendJsonArrayValue(reposArr, repoObj);
                ++count;
            }
        }
        PutJsonObjectValue(mutDoc.get(), root, "githubRepos", reposArr);
    }
    else
    {
        (void)yyjson_mut_obj_remove_key(root, "githubRepos");
    }

    if (settings.redSalamanderPath && ! settings.redSalamanderPath->empty() && ValidPreferencePath(*settings.redSalamanderPath) &&
        DxUi::RichText::TryEncodeUtf8(settings.redSalamanderPath->native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutJsonObjectString(mutDoc.get(), root, "redSalamanderPath", encodedPath);
    else
        (void)yyjson_mut_obj_remove_key(root, "redSalamanderPath");

    PutJsonObjectString(mutDoc.get(), root, "exitUnsavedBehavior", ExitUnsavedBehaviorName(settings.exitUnsavedBehavior));

    if (! settings.lastDocument.empty() && ValidPreferencePath(settings.lastDocument) &&
        DxUi::RichText::TryEncodeUtf8(settings.lastDocument.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutJsonObjectString(mutDoc.get(), root, "lastDocument", encodedPath);
    else
        (void)yyjson_mut_obj_remove_key(root, "lastDocument");

    PutJsonObjectUint(mutDoc.get(), root, "filesWidth", settings.filesWidth);
    PutJsonObjectUint(mutDoc.get(), root, "readingWidth", settings.readingWidth);
    PutJsonObjectUint(mutDoc.get(), root, "textScale", settings.textScale);
    PutJsonObjectBool(mutDoc.get(), root, "dimInactiveBlocks", settings.dimInactiveBlocks);
    PutJsonObjectBool(mutDoc.get(), root, "allowRemoteImages", settings.allowRemoteImages);
    PutJsonObjectBool(mutDoc.get(), root, "allowLocalNetworkImages", settings.allowLocalNetworkImages);
    PutJsonObjectBool(mutDoc.get(), root, "imageFirstPaste", settings.imageFirstPaste);
    PutJsonObjectBool(mutDoc.get(), root, "strikeCompletedTasks", settings.strikeCompletedTasks);
    PutJsonObjectBool(mutDoc.get(), root, "splitScrollSync", settings.splitScrollSync);
    PutJsonObjectString(mutDoc.get(), root, "filesPinMode", PanePinModeName(settings.filesPinMode));
    PutJsonObjectString(mutDoc.get(), root, "outlinePinMode", PanePinModeName(settings.outlinePinMode));

    if (settings.windowPlacement && ValidWindowPlacement(settings.windowPlacement.value()))
    {
        const auto& placement        = settings.windowPlacement.value();
        yyjson_mut_val* placementObj = yyjson_mut_obj_get(root, "windowPlacement");
        const bool existingPlacement = placementObj != nullptr && yyjson_mut_is_obj(placementObj);
        if (! existingPlacement)
            placementObj = CreateMutableJsonObject(mutDoc.get());
        PutJsonObjectInt(mutDoc.get(), placementObj, "left", placement.normalBounds.left);
        PutJsonObjectInt(mutDoc.get(), placementObj, "top", placement.normalBounds.top);
        PutJsonObjectInt(mutDoc.get(), placementObj, "right", placement.normalBounds.right);
        PutJsonObjectInt(mutDoc.get(), placementObj, "bottom", placement.normalBounds.bottom);
        PutJsonObjectUint(mutDoc.get(), placementObj, "dpi", placement.dpi);
        PutJsonObjectBool(mutDoc.get(), placementObj, "maximized", placement.maximized);
        if (! existingPlacement)
            PutJsonObjectValue(mutDoc.get(), root, "windowPlacement", placementObj);
    }
    else
    {
        (void)yyjson_mut_obj_remove_key(root, "windowPlacement");
    }

    yyjson_mut_val* syntaxObj = yyjson_mut_obj_get(root, "markdownSyntax");
    const bool existingSyntax = syntaxObj != nullptr && yyjson_mut_is_obj(syntaxObj);
    if (! existingSyntax)
        syntaxObj = CreateMutableJsonObject(mutDoc.get());
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        PutJsonObjectBool(mutDoc.get(), syntaxObj, JsonMarkdownSyntaxKeys[index], settings.markdownSyntax[index]);
    if (! existingSyntax)
        PutJsonObjectValue(mutDoc.get(), root, "markdownSyntax", syntaxObj);

    yyjson_mut_val* spellingObj = yyjson_mut_obj_get(root, "spelling");
    const bool existingSpelling = spellingObj != nullptr && yyjson_mut_is_obj(spellingObj);
    if (! existingSpelling)
        spellingObj = CreateMutableJsonObject(mutDoc.get());
    PutJsonObjectBool(mutDoc.get(), spellingObj, "enabled", settings.spelling.enabled);
    yyjson_mut_val* langArr = CreateMutableJsonArray(mutDoc.get());
    std::vector<std::wstring_view> emittedLanguages;
    emittedLanguages.reserve(MaximumSpellingLanguages);
    for (const auto& tag : settings.spelling.languages)
    {
        if (! IsValidLanguageTag(tag))
            continue;
        if (std::ranges::any_of(emittedLanguages, [&](std::wstring_view existing) noexcept { return EqualOrdinalIgnoreCase(existing, tag); }))
            continue;
        std::string utf8Tag;
        if (DxUi::RichText::TryEncodeUtf8(tag, utf8Tag) == DxUi::RichText::SourceResult::Success)
        {
            AppendJsonArrayString(mutDoc.get(), langArr, utf8Tag);
            emittedLanguages.push_back(tag);
            if (emittedLanguages.size() >= MaximumSpellingLanguages)
                break;
        }
    }
    if (yyjson_mut_arr_size(langArr) == 0u)
        AppendJsonArrayString(mutDoc.get(), langArr, "auto");
    PutJsonObjectValue(mutDoc.get(), spellingObj, "languages", langArr);
    PutJsonObjectBool(mutDoc.get(), spellingObj, "ignoreUppercase", settings.spelling.ignoreUppercase);
    PutJsonObjectBool(mutDoc.get(), spellingObj, "ignoreWordsWithDigits", settings.spelling.ignoreWordsWithDigits);
    PutJsonObjectBool(mutDoc.get(), spellingObj, "ignoreInternetAndPaths", settings.spelling.ignoreInternetAndPaths);
    if (! settings.spelling.customDictionaryPath.empty() && ValidPreferencePath(settings.spelling.customDictionaryPath) &&
        DxUi::RichText::TryEncodeUtf8(settings.spelling.customDictionaryPath.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutJsonObjectString(mutDoc.get(), spellingObj, "customDictionaryPath", encodedPath);
    else
        PutJsonObjectString(mutDoc.get(), spellingObj, "customDictionaryPath", "");
    if (! settings.spelling.dictionariesFolder.empty() && ValidPreferencePath(settings.spelling.dictionariesFolder) &&
        DxUi::RichText::TryEncodeUtf8(settings.spelling.dictionariesFolder.native(), encodedPath) == DxUi::RichText::SourceResult::Success)
        PutJsonObjectString(mutDoc.get(), spellingObj, "dictionariesFolder", encodedPath);
    else
        PutJsonObjectString(mutDoc.get(), spellingObj, "dictionariesFolder", "");
    PutJsonObjectString(mutDoc.get(), spellingObj, "engine", SpellEngineName(settings.spelling.engine));
    if (! existingSpelling)
        PutJsonObjectValue(mutDoc.get(), root, "spelling", spellingObj);

    std::size_t length{};
    const unique_yyjson_text text = WriteJsonDocument(mutDoc.get(), YYJSON_WRITE_PRETTY, length);
    if (! text)
        return {};
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
    const HRESULT readResult = ReadBytes(path, bytes, &readStamp, MaximumSettingsFileBytes);
    if (SUCCEEDED(readResult))
    {
        Settings parsed;
        const HRESULT parseResult = TryParseSettingsDocument(bytes, parsed, nullptr, outcome.status);
        if (SUCCEEDED(parseResult))
        {
            outcome.settings = std::move(parsed);
            outcome.stamp    = readStamp;
            return outcome;
        }

        if (outcome.status != SettingsLoadStatus::Invalid)
            return outcome;

        const auto backup = BackupPathFor(path);
        const FileStamp missingStamp{};
        if (SUCCEEDED(WriteBytesAtomic(backup, bytes, &missingStamp, nullptr, false, nullptr, false, nullptr, MaximumSettingsFileBytes)))
        {
            outcome.backupPath     = backup;
            const std::string text = SerializeSettings(outcome.settings);
            outcome.templateWritten =
                ! text.empty() && SUCCEEDED(WriteBytesAtomic(path, text, &readStamp, &outcome.stamp, false, nullptr, false, nullptr, MaximumSettingsFileBytes));
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
    if (SUCCEEDED(ReadBytes(legacyPath, legacyBytes, nullptr, MaximumSettingsFileBytes)) && HasLegacySettingsHeader(legacyBytes))
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
        outcome.templateWritten =
            ! text.empty() && SUCCEEDED(WriteBytesAtomic(path, text, &missingStamp, &outcome.stamp, false, nullptr, false, nullptr, MaximumSettingsFileBytes));
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
    const HRESULT readResult = ReadBytes(SettingsPath(directory, debugBuild), bytes, &readStamp, MaximumSettingsFileBytes);
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
    const HRESULT read = ReadBytes(path, existing, &stamp, MaximumSettingsFileBytes);
    if (FAILED(read) && read != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && read != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        return read;

    std::string_view mergeSource;
    const FileStamp* expectedStamp = &stamp;
    Settings probe;
    std::string existingCanonical;
    if (SUCCEEDED(read))
    {
        const HRESULT parseResult = TryParseSettings(existing, probe);
        if (FAILED(parseResult))
            return parseResult;
        mergeSource       = existing;
        existingCanonical = SerializeSettings(probe, existing);
    }

    const std::string text = SerializeSettings(settings, mergeSource);
    if (text.empty())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (SUCCEEDED(read) && (existing == text || (! existingCanonical.empty() && text == existingCanonical)))
    {
        if (savedStamp)
            *savedStamp = stamp;
        return S_OK;
    }
    return WriteBytesAtomic(path, text, expectedStamp, savedStamp, false, nullptr, false, nullptr, MaximumSettingsFileBytes);
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
        try
        {
            const DxUi::Failure::Scope failureScope;
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
                    next = std::move(shared->pending.value());
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

struct SettingsWatcher::State final
{
    State()                        = default;
    ~State()                       = default;
    State(const State&)            = delete;
    State& operator=(const State&) = delete;
    State(State&&)                 = delete;
    State& operator=(State&&)      = delete;

    std::mutex mutex;
    std::filesystem::path directory;
    std::wstring fileName;
    HWND window{};
    UINT message{};
    wil::unique_event_nothrow stopEvent;
    wil::unique_event_nothrow readyEvent;
    std::atomic_bool notificationPending{false};
    std::atomic_bool running{false};
    bool closing{};

    void PostChanged() noexcept
    {
        const std::lock_guard lock(mutex);
        if (closing || ! window)
            return;
        if (notificationPending.exchange(true, std::memory_order_acq_rel))
            return;
        if (! PostMessageW(window, message, 0, 0))
            notificationPending.store(false, std::memory_order_release);
    }

    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        if (instance)
            (void)CallbackMayRunLong(instance);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto shared       = ownership->state;
        const auto clearRunning = wil::scope_exit([&]() noexcept { shared->running.store(false, std::memory_order_release); });

        bool readySignaled{};
        const auto signalReady = [&]() noexcept
        {
            if (! readySignaled && shared->readyEvent)
            {
                readySignaled = true;
                (void)SetEvent(shared->readyEvent.get());
            }
        };
        const auto ensureReadyOnExit = wil::scope_exit([&]() noexcept { signalReady(); });

        try
        {
            const DxUi::Failure::Scope failureScope;
            DirectoryWatch watch;
            const std::filesystem::path targetName(shared->fileName);

            unsigned consecutiveErrors{};
            constexpr unsigned MaximumWatcherRetries = 8u;
            const auto permanentFailure              = [](DWORD error) noexcept
            {
                return error == ERROR_ACCESS_DENIED || error == ERROR_INVALID_NAME || error == ERROR_BAD_PATHNAME || error == ERROR_BAD_NETPATH ||
                       error == ERROR_BAD_NET_NAME || error == ERROR_NOT_SUPPORTED;
            };
            const auto waitRetryOrStop = [&](DWORD error) noexcept -> bool
            {
                signalReady();
                if (permanentFailure(error) || consecutiveErrors >= MaximumWatcherRetries)
                    return false;
                const DWORD backoff = SettingsWatcherRetryMs << std::min<unsigned>(consecutiveErrors, 4u);
                ++consecutiveErrors;
                return WaitForSingleObject(shared->stopEvent.get(), backoff) != WAIT_OBJECT_0;
            };

            for (;;)
            {
                {
                    const std::lock_guard lock(shared->mutex);
                    if (shared->closing)
                        return;
                }
                if (! watch.IsAttached())
                {
                    wil::unique_handle directoryHandle(CreateFileW(shared->directory.c_str(),
                                                                   FILE_LIST_DIRECTORY,
                                                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                                   nullptr,
                                                                   OPEN_EXISTING,
                                                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                                                   nullptr));
                    if (! directoryHandle)
                    {
                        const DWORD openError = GetLastError();
                        if (! waitRetryOrStop(openError))
                            return;
                        continue;
                    }
                    if (FAILED(watch.AttachAndArm(std::move(directoryHandle), false, SettingsWatcherFilter)))
                    {
                        if (! waitRetryOrStop(watch.LastWin32Error()))
                            return;
                        continue;
                    }
                    signalReady();
                }

                HANDLE handles[]{shared->stopEvent.get(), watch.EventHandle()};
                const DWORD waitResult = WaitForMultipleObjects(2u, handles, FALSE, INFINITE);
                if (waitResult != WAIT_OBJECT_0 + 1u)
                    return;

                const auto poll = watch.Complete(std::span<const std::filesystem::path>(&targetName, 1u), false);
                if (poll == DirectoryWatch::PollStatus::Failed)
                {
                    if (! waitRetryOrStop(watch.LastWin32Error()))
                        return;
                    continue;
                }
                consecutiveErrors = 0u;
                if (poll == DirectoryWatch::PollStatus::Unmatched)
                {
                    if (! watch.Arm())
                    {
                        if (! waitRetryOrStop(watch.LastWin32Error()))
                            return;
                    }
                    continue;
                }

                // Coalesce multi-write bursts over SettingsWatcherCoalesceMs while keeping
                // the directory watch armed and responsive to stop signals.
                const ULONGLONG deadline = GetTickCount64() + SettingsWatcherCoalesceMs;
                while (watch.Arm())
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
                    if (watch.Complete({}, false) == DirectoryWatch::PollStatus::Failed)
                        break;
                }

                shared->PostChanged();
            }
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

    auto staged = std::make_shared<State>();
    staged->stopEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! staged->stopEvent)
    {
        const DWORD error = GetLastError();
        return error != ERROR_SUCCESS ? HRESULT_FROM_WIN32(error) : E_FAIL;
    }

    staged->readyEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (! staged->readyEvent)
    {
        const DWORD error = GetLastError();
        return error != ERROR_SUCCESS ? HRESULT_FROM_WIN32(error) : E_FAIL;
    }

    staged->directory = std::move(directory);
    staged->fileName  = std::move(fileName);
    staged->window    = window;
    staged->message   = message;
    staged->running.store(true, std::memory_order_release);

    auto ownership          = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT admission = ProcessWork::Acquire(ownership->lease);
    if (FAILED(admission))
    {
        staged->running.store(false, std::memory_order_release);
        return admission;
    }

    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
    {
        const DWORD submitError = GetLastError();
        staged->running.store(false, std::memory_order_release);
        return submitError != ERROR_SUCCESS ? HRESULT_FROM_WIN32(submitError) : E_FAIL;
    }
    (void)ownership.release();
    _state = std::move(staged);
    (void)WaitForSingleObject(_state->readyEvent.get(), 1000u);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

void SettingsWatcher::Acknowledge() noexcept
{
    if (_state)
        _state->notificationPending.store(false, std::memory_order_release);
}

void SettingsWatcher::Close() noexcept
{
    if (! _state)
        return;
    {
        const std::lock_guard lock(_state->mutex);
        _state->closing = true;
        _state->window  = nullptr;
        _state->message = 0;
        _state->notificationPending.store(false, std::memory_order_release);
        _state->running.store(false, std::memory_order_release);
        if (_state->stopEvent)
            (void)SetEvent(_state->stopEvent.get());
    }
    _state.reset();
}

bool SettingsWatcher::Active() const noexcept
{
    return _state && _state->running.load(std::memory_order_acquire);
}
} // namespace RedTilde
