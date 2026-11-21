#pragma once
#include <DxUi/Failure.h>
#include <cstddef>
#include <exception>
#include <format>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <windows.h>

namespace RedTilde
{
namespace Detail
{
[[noreturn]] inline void FailResourceAllocation() noexcept
{
    if (DxUi::Failure::IsConfigured())
        DxUi::Failure::FailAllocation();
    std::terminate();
}
} // namespace Detail

[[nodiscard]] inline std::wstring_view LoadEmbeddedStringResourceView(HMODULE module, UINT id) noexcept
{
    const HMODULE instance = module ? module : GetModuleHandleW(nullptr);
    if (! instance)
        return {};
    const wchar_t* value{};
    const int length = LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&value), 0);
    // The module owns this immutable resource storage while loaded.
    // Resolve each request so the calling thread's resource language still applies.
    return length > 0 && value ? std::wstring_view(value, static_cast<std::size_t>(length)) : std::wstring_view{};
}

[[nodiscard]] inline std::wstring_view LoadStringResourceView(HMODULE module, UINT id) noexcept
{
    return LoadEmbeddedStringResourceView(module, id);
}

[[nodiscard]] inline std::wstring LoadEmbeddedStringResource(HMODULE module, UINT id) noexcept
{
    const auto view = LoadEmbeddedStringResourceView(module, id);
    if (view.empty())
        return {};
    try
    {
        return std::wstring(view);
    }
    catch (const std::bad_alloc&)
    {
        Detail::FailResourceAllocation();
    }
}

[[nodiscard]] inline std::wstring LoadStringResource(HMODULE module, UINT id) noexcept
{
    return LoadEmbeddedStringResource(module, id);
}

template <class... Args> [[nodiscard]] inline std::wstring FormatEmbeddedStringResource(HMODULE module, UINT id, Args&&... args) noexcept
{
    std::wstring pattern = LoadEmbeddedStringResource(module, id);
    if (pattern.empty())
        return {};
    // A malformed placeholder is a resource-authoring defect; fall back to the raw pattern
    // without swallowing OOM or leaking format exceptions across UI/callback boundaries.
    try
    {
        return std::vformat(std::wstring_view{pattern}, std::make_wformat_args(args...));
    }
    catch (const std::bad_alloc&)
    {
        Detail::FailResourceAllocation();
    }
    catch (const std::format_error&)
    {
        return pattern;
    }
}

template <class... Args> [[nodiscard]] inline std::wstring FormatStringResource(HMODULE module, UINT id, Args&&... args) noexcept
{
    return FormatEmbeddedStringResource(module, id, std::forward<Args>(args)...);
}
} // namespace RedTilde
