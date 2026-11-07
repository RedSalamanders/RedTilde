#pragma once
#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <windows.h>

namespace RedTilde
{
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
[[nodiscard]] bool ValidWindowPlacement(const WindowPlacementSettings &placement) noexcept;
struct Settings
{
    Appearance appearance = Appearance::System;
    EditorView view = EditorView::Compose;
    bool focusMode = false;
    bool showOutline = true;
    bool showFiles = false;
    std::filesystem::path filesRoot;
    std::filesystem::path lastDocument;
    unsigned filesWidth = 280;
    unsigned readingWidth = 760;
    unsigned textScale = 100;
    bool dimInactiveBlocks = true;
    bool allowRemoteImages = false;
    bool imageFirstPaste = false;
    bool strikeCompletedTasks = false;
    std::optional<WindowPlacementSettings> windowPlacement;
    std::array<bool, MarkdownSyntaxCount> markdownSyntax{true, true, true, true, true, true, true};
    [[nodiscard]] bool SyntaxEnabled(MarkdownSyntax syntax) const noexcept
    {
        return markdownSyntax[static_cast<std::size_t>(syntax)];
    }
};
[[nodiscard]] Settings ParseSettings(std::string_view bytes);
[[nodiscard]] std::string SerializeSettings(const Settings &settings);
[[nodiscard]] HRESULT LoadSettings(const std::filesystem::path &directory, Settings &settings) noexcept;
[[nodiscard]] HRESULT SaveSettings(const std::filesystem::path &directory, const Settings &settings) noexcept;

// One active writer and one latest pending value. Closing drops the notification
// binding; admitted preference writes finish without a UI-thread join.
class SettingsWriter
{
    struct State;
    std::shared_ptr<State> state;

  public:
    SettingsWriter() = default;
    ~SettingsWriter();
    SettingsWriter(const SettingsWriter &) = delete;
    SettingsWriter &operator=(const SettingsWriter &) = delete;
    void Initialize(std::filesystem::path directory, HWND window = nullptr, UINT message = WM_APP);
    [[nodiscard]] HRESULT Submit(const Settings &settings) noexcept;
    [[nodiscard]] bool Busy() const noexcept;
    [[nodiscard]] HRESULT Status() const noexcept;
    void Close() noexcept;
};
} // namespace RedTilde
