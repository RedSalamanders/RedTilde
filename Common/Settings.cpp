#include "Settings.h"
#include "Storage.h"
#include <charconv>
#include <format>
#include <mutex>
#include <new>
#include <optional>

namespace RedTilde
{
namespace
{
// Indexed by MarkdownSyntax. Missing or invalid keys keep the enabled default.
constexpr std::array<std::string_view, MarkdownSyntaxCount> MarkdownSyntaxKeys{
    "syntaxHighlight", "syntaxSuperscript", "syntaxSubscript", "syntaxInserted", "syntaxWikiLinks", "syntaxDefinitionLists", "syntaxAbbreviations"};
} // namespace

Settings ParseSettings(std::string_view bytes)
{
    Settings settings;
    if (!bytes.starts_with("RedTilde.Settings/1\n"))
        return settings;
    while (!bytes.empty())
    {
        const auto end = bytes.find('\n');
        const auto line = bytes.substr(0, end);
        const auto split = line.find('=');
        if (split != line.npos)
        {
            const auto key = line.substr(0, split), value = line.substr(split + 1);
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
                if (key == "focusDimming" && number <= 1)
                    settings.dimInactiveBlocks = number != 0;
                if (key == "remoteImages" && number <= 1)
                    settings.allowRemoteImages = number != 0;
                for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
                    if (key == MarkdownSyntaxKeys[index] && number <= 1)
                        settings.markdownSyntax[index] = number != 0;
            }
        }
        if (end == bytes.npos)
            break;
        bytes.remove_prefix(end + 1);
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
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        result += std::format("{}={}\n", MarkdownSyntaxKeys[index], settings.markdownSyntax[index] ? 1 : 0);
    return result;
}

HRESULT LoadSettings(const std::filesystem::path &directory, Settings &settings) noexcept
try
{
    std::string bytes;
    const HRESULT result = ReadBytes(directory / L"RedTilde.settings", bytes);
    if (FAILED(result))
        return result;
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
    return WriteBytesAtomic(directory / L"RedTilde.settings", SerializeSettings(settings));
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
