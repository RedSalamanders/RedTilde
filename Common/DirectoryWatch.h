#pragma once
// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// clang-format on

#include "TextUtilities.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <wil/resource.h>

namespace RedTilde
{
// Worker-owned OVERLAPPED ReadDirectoryChangesW state. Only the owning worker thread
// arms, completes, or closes this watch; Close() cancels and drains kernel I/O
// synchronously before the OVERLAPPED and buffer leave scope.
class DirectoryWatch final
{
public:
    enum class PollStatus : std::uint8_t
    {
        Matched,
        Unmatched,
        Overflow,
        Failed
    };

    DirectoryWatch() noexcept = default;
    ~DirectoryWatch() noexcept
    {
        Close();
    }
    DirectoryWatch(const DirectoryWatch&)            = delete;
    DirectoryWatch& operator=(const DirectoryWatch&) = delete;
    DirectoryWatch(DirectoryWatch&&)                 = delete;
    DirectoryWatch& operator=(DirectoryWatch&&)      = delete;

    void Close() noexcept
    {
        if (_pending && _directory)
        {
            (void)CancelIoEx(_directory.get(), &_overlapped);
            DWORD bytes{};
            // Only the worker waits for kernel ownership of its OVERLAPPED and
            // buffer to end. UI Close never destroys or waits for this state.
            (void)GetOverlappedResult(_directory.get(), &_overlapped, &bytes, TRUE);
            _pending = false;
        }
        _directory.reset();
        _event.reset();
    }

    // Precondition: directory must have been opened with FILE_FLAG_OVERLAPPED;
    // a synchronous handle will block indefinitely inside ReadDirectoryChangesW.
    [[nodiscard]] HRESULT AttachAndArm(wil::unique_handle directory, bool watchSubtree, DWORD notifyFilter) noexcept
    {
        Close();
        _status         = S_OK;
        _lastWin32Error = ERROR_SUCCESS;
        _watchSubtree   = watchSubtree;
        _notifyFilter   = notifyFilter;
        if (! directory)
        {
            _lastWin32Error = ERROR_INVALID_HANDLE;
            _status         = E_HANDLE;
            return _status;
        }
        _event.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (! _event)
        {
            _lastWin32Error = GetLastError();
            _status         = HRESULT_FROM_WIN32(_lastWin32Error ? _lastWin32Error : ERROR_OUTOFMEMORY);
            return _status;
        }
        _directory = std::move(directory);
        if (! Arm())
            return _status;
        return S_OK;
    }

    [[nodiscard]] bool Arm() noexcept
    {
        if (! _directory || ! _event || _pending)
            return _pending;
        (void)ResetEvent(_event.get());
        _overlapped        = {};
        _overlapped.hEvent = _event.get();
        if (ReadDirectoryChangesW(_directory.get(),
                                  _buffer.data(),
                                  static_cast<DWORD>(_buffer.size()),
                                  _watchSubtree ? TRUE : FALSE,
                                  _notifyFilter,
                                  nullptr,
                                  &_overlapped,
                                  nullptr))
        {
            _pending        = true;
            _status         = S_OK;
            _lastWin32Error = ERROR_SUCCESS;
            return true;
        }
        _lastWin32Error = GetLastError();
        _status         = HRESULT_FROM_WIN32(_lastWin32Error ? _lastWin32Error : ERROR_GEN_FAILURE);
        Close();
        return false;
    }

    // Drains the completed overlapped operation. If observedNames is empty, any non-empty
    // notification returns Matched; otherwise returns Matched iff at least one changed
    // relative name matches an entry in observedNames (ordinal case-insensitive).
    [[nodiscard]] PollStatus Complete(std::span<const std::filesystem::path> observedNames = {}, bool rearm = true) noexcept
    {
        if (! _directory || ! _pending)
            return PollStatus::Failed;
        DWORD bytes{};
        bool overflow = false;
        if (! GetOverlappedResult(_directory.get(), &_overlapped, &bytes, FALSE))
        {
            const DWORD err = GetLastError();
            _pending        = false;
            // A zero/overflow notification means rescan the bounded model; its
            // individual names are deliberately never trusted as a complete list.
            if (err != ERROR_NOTIFY_ENUM_DIR)
            {
                _lastWin32Error = err;
                _status         = HRESULT_FROM_WIN32(err ? err : ERROR_GEN_FAILURE);
                Close();
                return PollStatus::Failed;
            }
            overflow = true;
        }
        else
        {
            _pending = false;
            if (bytes == 0u)
                overflow = true;
        }

        bool matched = overflow || observedNames.empty();
        if (! matched)
        {
            for (std::size_t offset{}; offset < bytes;)
            {
                constexpr std::size_t headerSize = offsetof(FILE_NOTIFY_INFORMATION, FileName);
                if (bytes - offset < headerSize)
                {
                    overflow = true;
                    matched  = true;
                    break;
                }
                const auto* entry = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(_buffer.data() + offset);
                if ((entry->FileNameLength % sizeof(wchar_t)) != 0u || entry->FileNameLength > bytes - offset - headerSize)
                {
                    overflow = true;
                    matched  = true;
                    break;
                }
                const std::wstring_view changedName(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
                if (std::ranges::any_of(observedNames,
                                        [&](const std::filesystem::path& p) noexcept { return EqualOrdinalIgnoreCase(p.native(), changedName); }))
                {
                    matched = true;
                    break;
                }
                if (entry->NextEntryOffset == 0u)
                    break;
                if ((entry->NextEntryOffset % alignof(DWORD)) != 0u || entry->NextEntryOffset < headerSize || entry->NextEntryOffset > bytes - offset)
                {
                    overflow = true;
                    matched  = true;
                    break;
                }
                offset += entry->NextEntryOffset;
            }
        }

        if (rearm)
            (void)Arm();
        if (overflow)
            return PollStatus::Overflow;
        return matched ? PollStatus::Matched : PollStatus::Unmatched;
    }

    [[nodiscard]] HANDLE EventHandle() const noexcept
    {
        return _event.get();
    }
    [[nodiscard]] bool IsAttached() const noexcept
    {
        return static_cast<bool>(_directory);
    }
    [[nodiscard]] bool IsPending() const noexcept
    {
        return _pending;
    }
    [[nodiscard]] HRESULT Status() const noexcept
    {
        return _status;
    }
    [[nodiscard]] DWORD LastWin32Error() const noexcept
    {
        return _lastWin32Error;
    }
    void SetFailedStatus(HRESULT status) noexcept
    {
        Close();
        _status = status;
    }

private:
    wil::unique_handle _directory;
    wil::unique_event_nothrow _event;
    OVERLAPPED _overlapped{};
    HRESULT _status{S_OK};
    DWORD _lastWin32Error{ERROR_SUCCESS};
    DWORD _notifyFilter{};
    alignas(DWORD) std::array<std::byte, 16u * 1024u> _buffer{};
    bool _pending{};
    bool _watchSubtree{};
};
} // namespace RedTilde
