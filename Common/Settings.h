#pragma once
#include "ProcessWork.h"
#include "Storage.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <wil/resource.h>
#include <windows.h>

namespace RedTilde
{
#if defined(_DEBUG)
inline constexpr bool DefaultDebugSettingsBuild = true;
#else
inline constexpr bool DefaultDebugSettingsBuild = false;
#endif

enum class Appearance
{
    System,
    Light,
    Dark,
    Paper
};
enum class EditorView
{
    Compose,
    Source,
    Split
};
enum class PanePinMode : std::uint8_t
{
    Auto,
    Pinned,
    Unpinned
};
enum class SpellEngine : std::uint8_t
{
    Auto    = 0,
    Windows = 1
};
enum class ExitUnsavedBehavior : std::uint8_t
{
    Prompt        = 0,
    KeepAndResume = 1
};
struct GitHubRepoRef
{
    std::wstring owner;
    std::wstring repo;
    std::wstring branch;
    bool operator==(const GitHubRepoRef&) const = default;
};
inline constexpr std::size_t MaximumFilesRoots  = 16u;
inline constexpr std::size_t MaximumGitHubRepos = 8u;
[[nodiscard]] bool IsValidGitHubTokenSegment(std::wstring_view segment, std::size_t maximumLength) noexcept;
struct SpellingSettings
{
    bool enabled = true;
    std::vector<std::wstring> languages{L"auto"};
    bool ignoreUppercase        = true;
    bool ignoreWordsWithDigits  = true;
    bool ignoreInternetAndPaths = true;
    std::filesystem::path customDictionaryPath;
    std::filesystem::path dictionariesFolder;
    SpellEngine engine                             = SpellEngine::Auto;
    bool operator==(const SpellingSettings&) const = default;
};
inline constexpr std::size_t MaximumSpellingLanguages = 3u;
[[nodiscard]] bool IsValidLanguageTag(std::wstring_view tag) noexcept;
// Optional RichMarkdown syntax beyond the always-on GFM set, in drawer order.
// Each persists under its own application-owned key, never a library enum value.
enum class MarkdownSyntax
{
    Highlight,
    Superscript,
    Subscript,
    Inserted,
    WikiLinks,
    DefinitionLists,
    Abbreviations
};
inline constexpr std::size_t MarkdownSyntaxCount = 7;
struct WindowPlacementSettings
{
    RECT normalBounds{}; // Physical screen coordinates, never WINDOWPLACEMENT workspace coordinates.
    unsigned dpi = 96;
    bool maximized{};
};
[[nodiscard]] bool ValidWindowPlacement(const WindowPlacementSettings& placement) noexcept;
struct Settings
{
    Appearance appearance = Appearance::System;
    EditorView view       = EditorView::Compose;
    bool focusMode        = false;
    bool showOutline      = true;
    bool showFiles        = false;
    std::filesystem::path filesRoot;
    std::vector<std::filesystem::path> filesRoots;
    std::vector<GitHubRepoRef> githubRepos;
    std::optional<std::filesystem::path> redSalamanderPath;
    std::filesystem::path lastDocument;
    unsigned filesWidth          = 280;
    unsigned readingWidth        = 760;
    unsigned textScale           = 100;
    bool dimInactiveBlocks       = true;
    bool allowRemoteImages       = false;
    bool allowLocalNetworkImages = false;
    bool imageFirstPaste         = false;
    bool strikeCompletedTasks    = false;
    bool splitScrollSync         = true;
    PanePinMode filesPinMode     = PanePinMode::Auto;
    PanePinMode outlinePinMode   = PanePinMode::Auto;
    ExitUnsavedBehavior exitUnsavedBehavior{ExitUnsavedBehavior::Prompt};
    std::optional<WindowPlacementSettings> windowPlacement;
    std::array<bool, MarkdownSyntaxCount> markdownSyntax{true, true, true, true, true, true, true};
    SpellingSettings spelling;
    [[nodiscard]] bool SyntaxEnabled(MarkdownSyntax syntax) const noexcept
    {
        return markdownSyntax[static_cast<std::size_t>(syntax)];
    }
};

enum class SettingsLoadStatus : std::uint8_t
{
    Loaded,
    Missing,
    Migrated,
    Invalid,
    Unreadable
};

struct SettingsLoadResult
{
    Settings settings;
    SettingsLoadStatus status{SettingsLoadStatus::Loaded};
    std::filesystem::path backupPath;
    bool templateWritten{};
    FileStamp stamp;
};

[[nodiscard]] std::wstring_view SettingsFileName(bool debugBuild = DefaultDebugSettingsBuild) noexcept;
[[nodiscard]] std::filesystem::path SettingsPath(const std::filesystem::path& directory, bool debugBuild = DefaultDebugSettingsBuild);
[[nodiscard]] Settings ParseSettings(std::string_view bytes);
[[nodiscard]] HRESULT TryParseSettings(std::string_view bytes, Settings& settings, std::uint64_t* schemaVersion = nullptr) noexcept;
[[nodiscard]] Settings ParseLegacySettings(std::string_view bytes);
[[nodiscard]] std::string SerializeSettings(const Settings& settings, std::string_view existingJson = {});
[[nodiscard]] SettingsLoadResult LoadSettingsCold(const std::filesystem::path& directory, bool debugBuild = DefaultDebugSettingsBuild) noexcept;
[[nodiscard]] HRESULT LoadSettings(const std::filesystem::path& directory, Settings& settings, bool debugBuild = DefaultDebugSettingsBuild) noexcept;
[[nodiscard]] HRESULT LoadSettingsLive(const std::filesystem::path& directory,
                                       Settings& settings,
                                       FileStamp* stamp = nullptr,
                                       bool debugBuild  = DefaultDebugSettingsBuild) noexcept;
[[nodiscard]] HRESULT SaveSettings(const std::filesystem::path& directory,
                                   const Settings& settings,
                                   bool debugBuild       = DefaultDebugSettingsBuild,
                                   FileStamp* savedStamp = nullptr) noexcept;

// One active writer and one latest pending value. Closing drops the notification
// binding; admitted preference writes finish without a UI-thread join.
class SettingsWriter
{
    struct State;
    std::shared_ptr<State> _state;

public:
    SettingsWriter() = default;
    ~SettingsWriter();
    SettingsWriter(const SettingsWriter&)            = delete;
    SettingsWriter& operator=(const SettingsWriter&) = delete;
    void Initialize(std::filesystem::path directory, HWND window = nullptr, UINT message = WM_APP);
    [[nodiscard]] HRESULT Submit(const Settings& settings) noexcept;
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    [[nodiscard]] std::optional<FileStamp> LastWrittenStamp() const noexcept;
    void Close() noexcept;
};

// Event-driven OVERLAPPED ReadDirectoryChangesW watcher for the active settings
// file. Coalesces bursts without polling and posts one unacknowledged message
// to the UI window; parsing and applying remain on the UI thread.
class SettingsWatcher final
{
public:
    SettingsWatcher() = default;
    ~SettingsWatcher();
    SettingsWatcher(const SettingsWatcher&)            = delete;
    SettingsWatcher& operator=(const SettingsWatcher&) = delete;
    SettingsWatcher(SettingsWatcher&&)                 = delete;
    SettingsWatcher& operator=(SettingsWatcher&&)      = delete;

    [[nodiscard]] HRESULT Start(std::filesystem::path directory, std::wstring fileName, HWND window, UINT message) noexcept;
    void Acknowledge() noexcept;
    void Close() noexcept;
    [[nodiscard]] bool Active() const noexcept;

private:
    void WatchThread(ProcessWorkLease lease) noexcept;
    void PostChanged() noexcept;

    std::filesystem::path _directory;
    std::wstring _fileName;
    HWND _window{};
    UINT _message{};
    wil::unique_event_nothrow _stopEvent;
    wil::unique_event_nothrow _readyEvent;
    std::jthread _thread;
    std::atomic_bool _notificationPending{false};
};
} // namespace RedTilde
