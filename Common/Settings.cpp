#include "Settings.h"
#include "Storage.h"
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <charconv>
#include <format>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>

namespace RedTilde
{
namespace
{
constexpr std::string_view SettingsHeader = "RedTilde.Settings/1\n";
// Indexed by MarkdownSyntax. Missing or invalid keys keep the enabled default.
constexpr std::array<std::string_view, MarkdownSyntaxCount> MarkdownSyntaxKeys{
    "syntaxHighlight", "syntaxSuperscript", "syntaxSubscript", "syntaxInserted", "syntaxWikiLinks", "syntaxDefinitionLists", "syntaxAbbreviations"};
// Settings are line-based. Keep a Windows folder/document UTF-8 value lossless, bounded,
// absolute and free of control characters so it cannot introduce another field.
bool ValidPreferencePath(const std::filesystem::path &path)
{
    const auto &text = path.native();
    return path.is_absolute() && text.size() <= 32767 && DxUi::RichText::IsValidUtf16(text) &&
           std::ranges::none_of(text, [](wchar_t unit) { return unit < L' '; });
}
} // namespace

bool ValidWindowPlacement(const WindowPlacementSettings &placement) noexcept
{
    const auto &bounds = placement.normalBounds;
    constexpr LONG coordinateLimit = 1048576;
    return placement.dpi >= 48u && placement.dpi <= 768u && bounds.left >= -coordinateLimit && bounds.top >= -coordinateLimit &&
           bounds.right <= coordinateLimit && bounds.bottom <= coordinateLimit && bounds.right > bounds.left && bounds.bottom > bounds.top &&
           static_cast<long long>(bounds.right) - bounds.left <= 32768 && static_cast<long long>(bounds.bottom) - bounds.top <= 32768;
}

Settings ParseSettings(std::string_view bytes)
{
    Settings settings;
    if (!bytes.starts_with(SettingsHeader))
        return settings;
    std::array<std::optional<int>, 4> windowBounds;
    std::optional<unsigned> windowDpi{96u};
    bool windowMaximized{};
    while (!bytes.empty())
    {
        const auto end = bytes.find('\n');
        const auto line = bytes.substr(0, end);
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
                    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), coordinate);
                    windowBounds[index] =
                        parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() ? std::optional<int>{coordinate} : std::nullopt;
                }
            if ((key == "filesRoot" || key == "lastDocument") && value.size() <= 3u * 32767u)
            {
                try
                {
                    std::filesystem::path path(DxUi::RichText::DecodeUtf8(value));
                    if (ValidPreferencePath(path))
                    {
                        auto &destination = key == "filesRoot" ? settings.filesRoot : settings.lastDocument;
                        destination = path.lexically_normal();
                    }
                }
                catch (const std::invalid_argument &)
                {
                    // A malformed UTF-8 preference path keeps its default; neighboring fields still apply.
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
                if (key == "imageFirstPaste" && number <= 1)
                    settings.imageFirstPaste = number != 0;
                if (key == "strikeCompletedTasks" && number <= 1)
                    settings.strikeCompletedTasks = number != 0;
                for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
                    if (key == MarkdownSyntaxKeys[index] && number <= 1)
                        settings.markdownSyntax[index] = number != 0;
            }
        }
        if (end == bytes.npos)
            break;
        bytes.remove_prefix(end + 1);
    }
    if (windowDpi && std::ranges::all_of(windowBounds, [](const auto &value) { return value.has_value(); }))
    {
        WindowPlacementSettings placement{
            {windowBounds[0].value(), windowBounds[1].value(), windowBounds[2].value(), windowBounds[3].value()}, windowDpi.value(), windowMaximized};
        if (ValidWindowPlacement(placement))
            settings.windowPlacement = placement;
    }
    // Focus mode is intentionally session-local, so restart never hides commands.
    return settings;
}

std::string SerializeSettings(const Settings &settings)
{
    auto result = std::format(
        "RedTilde.Settings/1\ntheme={}\nview={}\noutline={}\nfiles={}\nfilesWidth={}\nreadingWidth={}\nscale={}\nfocusDimming={}\nremoteImages={}\n",
        static_cast<unsigned>(settings.appearance), static_cast<unsigned>(settings.view), settings.showOutline ? 1 : 0, settings.showFiles ? 1 : 0,
        settings.filesWidth, settings.readingWidth, settings.textScale, settings.dimInactiveBlocks ? 1 : 0, settings.allowRemoteImages ? 1 : 0);
    result += std::format("strikeCompletedTasks={}\n", settings.strikeCompletedTasks ? 1 : 0);
    result += std::format("imageFirstPaste={}\n", settings.imageFirstPaste ? 1 : 0);
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        result += std::format("{}={}\n", MarkdownSyntaxKeys[index], settings.markdownSyntax[index] ? 1 : 0);
    if (ValidPreferencePath(settings.filesRoot))
        result += std::format("filesRoot={}\n", DxUi::RichText::EncodeUtf8(settings.filesRoot.native()));
    if (ValidPreferencePath(settings.lastDocument))
        result += std::format("lastDocument={}\n", DxUi::RichText::EncodeUtf8(settings.lastDocument.native()));
    if (settings.windowPlacement && ValidWindowPlacement(settings.windowPlacement.value()))
    {
        const auto &placement = settings.windowPlacement.value();
        const auto &bounds = placement.normalBounds;
        result += std::format("windowLeft={}\nwindowTop={}\nwindowRight={}\nwindowBottom={}\nwindowDpi={}\nwindowMaximized={}\n", bounds.left,
                              bounds.top, bounds.right, bounds.bottom, placement.dpi, placement.maximized ? 1 : 0);
    }
    return result;
}

HRESULT LoadSettings(const std::filesystem::path &directory, Settings &settings) noexcept
try
{
    std::string bytes;
    const HRESULT result = ReadBytes(directory / L"RedTilde.settings", bytes);
    if (FAILED(result))
        return result;
    if (!bytes.starts_with(SettingsHeader))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    settings = ParseSettings(bytes);
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}

HRESULT SaveSettings(const std::filesystem::path &directory, const Settings &settings) noexcept
try
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return HRESULT_FROM_WIN32(static_cast<DWORD>(error.value()));
    const auto path = directory / L"RedTilde.settings";
    std::string existing;
    FileStamp stamp;
    const HRESULT read = ReadBytes(path, existing, &stamp);
    if (FAILED(read) && read != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        return read;
    if (SUCCEEDED(read) && !existing.starts_with(SettingsHeader))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    return WriteBytesAtomic(path, SerializeSettings(settings), &stamp);
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
}
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct SettingsWriter::State
{
    std::mutex mutex;
    std::filesystem::path directory;
    std::optional<Settings> pending;
    HWND window{};
    UINT message{};
    bool active{};
    HRESULT status = S_OK;
    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE, void *context) noexcept
    {
        const std::unique_ptr<std::shared_ptr<State>> ownership(static_cast<std::shared_ptr<State> *>(context));
        const auto &shared = *ownership;
        for (;;)
        {
            Settings next;
            {
                std::lock_guard lock(shared->mutex);
                if (!shared->pending)
                {
                    shared->active = false;
                    if (shared->window)
                        (void)PostMessageW(shared->window, shared->message, 0, 0);
                    return;
                }
                next = shared->pending.value();
                shared->pending.reset();
            }
            const HRESULT result = SaveSettings(shared->directory, next);
            {
                std::lock_guard lock(shared->mutex);
                shared->status = result;
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
    state = std::make_shared<State>();
    state->directory = std::move(directory);
    state->window = window;
    state->message = message;
}
HRESULT SettingsWriter::Submit(const Settings &settings) noexcept
try
{
    if (!state)
        return E_UNEXPECTED;
    std::lock_guard lock(state->mutex);
    state->pending = settings;
    if (state->active)
        return S_OK;
    auto ownership = std::make_unique<std::shared_ptr<State>>(state);
    state->active = true;
    if (!TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
    {
        state->active = false;
        state->status = HRESULT_FROM_WIN32(GetLastError());
        return state->status;
    }
    (void)ownership.release();
    return S_OK;
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
bool SettingsWriter::Busy() const noexcept
{
    if (!state)
        return false;
    std::lock_guard lock(state->mutex);
    return state->active;
}
HRESULT SettingsWriter::Status() const noexcept
{
    if (!state)
        return E_UNEXPECTED;
    std::lock_guard lock(state->mutex);
    return state->status;
}
void SettingsWriter::Close() noexcept
{
    if (!state)
        return;
    {
        std::lock_guard lock(state->mutex);
        state->window = nullptr;
    }
    state.reset();
}
} // namespace RedTilde
