#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <windows.h>

namespace RedTilde
{
// Armed only by the synchronous hidden performance harness. Normal editing
// carries no clock reads, allocation, tracing session or additional worker.
enum class EditPhase : unsigned
{
    Enter,
    Analysis,
    Mirror,
    Identity,
    Caption,
    Chrome,
    Selection,
    Recovery,
    Exit,
    Count
};
struct EditDiagnostics
{
    using Clock = std::chrono::steady_clock;
    static constexpr unsigned Count = static_cast<unsigned>(EditPhase::Count);
    std::array<double, Count> wallUs{};
    std::array<std::uint64_t, Count> cpuCycles{};
    Clock::time_point previous;
    std::uint64_t previousCycles{};
    unsigned nextPhase{};
    bool valid = true;

    void Begin() noexcept
    {
        previous = Clock::now();
        ULONG64 cycles{};
        valid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
        previousCycles = cycles;
    }
    void Mark(EditPhase phase) noexcept
    {
        const auto now = Clock::now();
        ULONG64 cycles{};
        const bool read = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
        const auto index = static_cast<unsigned>(phase);
        if (index != nextPhase || index >= Count || !read || cycles < previousCycles)
        {
            valid = false;
            return;
        }
        wallUs[index] = std::chrono::duration<double, std::micro>(now - previous).count();
        cpuCycles[index] = cycles - previousCycles;
        previous = now;
        previousCycles = cycles;
        ++nextPhase;
    }
    [[nodiscard]] bool Complete() const noexcept
    {
        return valid && nextPhase == Count;
    }
};
} // namespace RedTilde
