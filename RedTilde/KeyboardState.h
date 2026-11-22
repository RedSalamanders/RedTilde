#pragma once

#include <array>
#include <cstddef>
#include <windows.h>

// Application-owned keyboard state. Ordinary runs read the calling thread's message key state;
// hidden self-tests switch to an isolated synthetic keyboard while keeping Win32 state updated
// so DxUi's internal modifier queries stay synchronized without physical desktop interference.
namespace RedTilde::KeyboardState
{
namespace Detail
{
struct Synthetic
{
    bool active = false;
    std::array<BYTE, 256> keys{};
};

inline Synthetic g_synthetic;
} // namespace Detail

inline void UseSynthetic(bool active) noexcept
{
    Detail::g_synthetic.active = active;
    Detail::g_synthetic.keys   = {};
}

class ScopedSynthetic final
{
public:
    ScopedSynthetic() noexcept : _previous(Detail::g_synthetic), _hasPreviousWin32(GetKeyboardState(_previousWin32.data()) != FALSE)
    {
        UseSynthetic(true);
    }
    ~ScopedSynthetic()
    {
        Detail::g_synthetic = _previous;
        if (_hasPreviousWin32)
        {
            auto restored = _previousWin32;
            (void)SetKeyboardState(restored.data());
        }
    }
    ScopedSynthetic(const ScopedSynthetic&)            = delete;
    ScopedSynthetic& operator=(const ScopedSynthetic&) = delete;
    ScopedSynthetic(ScopedSynthetic&&)                 = delete;
    ScopedSynthetic& operator=(ScopedSynthetic&&)      = delete;

private:
    const Detail::Synthetic _previous;
    std::array<BYTE, 256> _previousWin32{};
    bool _hasPreviousWin32{};
};

[[nodiscard]] inline bool Held(int virtualKey) noexcept
{
    if (Detail::g_synthetic.active)
        return (Detail::g_synthetic.keys[static_cast<std::size_t>(virtualKey) & 0xFFu] & 0x80u) != 0;
    return GetKeyState(virtualKey) < 0;
}

[[nodiscard]] inline bool Read(std::array<BYTE, 256>& keys) noexcept
{
    if (Detail::g_synthetic.active)
    {
        keys = Detail::g_synthetic.keys;
        return true;
    }
    return GetKeyboardState(keys.data()) != FALSE;
}

inline void Write(const std::array<BYTE, 256>& keys) noexcept
{
    if (Detail::g_synthetic.active)
        Detail::g_synthetic.keys = keys;
    std::array<BYTE, 256> copy = keys;
    (void)SetKeyboardState(copy.data());
}
} // namespace RedTilde::KeyboardState
