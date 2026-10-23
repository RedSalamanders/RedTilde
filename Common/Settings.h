#pragma once
#include <filesystem>
#include <memory>
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
struct Settings
{
    Appearance appearance = Appearance::System;
    EditorView view = EditorView::Compose;
    bool focusMode = false;
    bool showOutline = true;
    bool showFiles = false;
    unsigned filesWidth = 280;
    unsigned readingWidth = 760;
    unsigned textScale = 100;
    bool dimInactiveBlocks = true;
    bool allowRemoteImages = false;
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
