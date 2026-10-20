#include "Settings.h"
#include "Storage.h"
#include <charconv>
#include <format>
#include <mutex>
#include <new>
#include <optional>

namespace RedTilde
{
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
    return std::format("RedTilde.Settings/1\ntheme={}\nview={}\noutline={}\n", static_cast<unsigned>(settings.appearance),
                       static_cast<unsigned>(settings.view), settings.showOutline ? 1 : 0);
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
    return HRESULT_FROM_WIN32(error.code().value());
}

HRESULT SaveSettings(const std::filesystem::path &directory, const Settings &settings) noexcept
try
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return HRESULT_FROM_WIN32(error.value());
    return WriteBytesAtomic(directory / L"RedTilde.settings", SerializeSettings(settings));
}
catch (const std::bad_alloc &)
{
    return E_OUTOFMEMORY;
}
catch (const std::filesystem::filesystem_error &error)
{
    return HRESULT_FROM_WIN32(error.code().value());
}
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
