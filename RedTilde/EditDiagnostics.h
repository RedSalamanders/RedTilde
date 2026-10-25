#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
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
enum class SelectionComponent : unsigned
{
    SelectionSnapshot,
    DocumentAnalysisRefresh,
    SelectionFactsCache,
    Mirror,
    InlineSemantics,
    ToolCaptionPublication,
    Outline,
    BlockLookup,
    BlockLabels,
    PositionStatus,
    Find,
    Routing,
    Count
};
struct EditDiagnostics
{
    using Clock = std::chrono::steady_clock;
    static constexpr unsigned Count = static_cast<unsigned>(EditPhase::Count);
    static constexpr unsigned SelectionCount = static_cast<unsigned>(SelectionComponent::Count);
    struct SelectionRecord
    {
        std::array<double, SelectionCount> wallUs{};
        std::array<std::uint64_t, SelectionCount> cpuCycles{};
        std::array<unsigned, SelectionCount> calls{};
        double wallTotalUs{};
        std::uint64_t cpuTotalCycles{};
        unsigned invocations{};
        bool complete = true;
    } selection;
    std::array<double, Count> wallUs{};
    std::array<std::uint64_t, Count> cpuCycles{};
    Clock::time_point previous;
    std::uint64_t previousCycles{};
    unsigned nextPhase{};
    bool valid = true;
    bool selectionArmed{}, selectionInvalid{}, selectionInvocationActive{}, selectionComponentActive{};
    Clock::time_point selectionStart;
    std::uint64_t selectionStartCycles{};
    bool selectionStartCyclesValid{};

    class SelectionArm
    {
        EditDiagnostics *owner{};
        int uncaughtAtStart{};

      public:
        explicit SelectionArm(EditDiagnostics *value) noexcept : owner(value), uncaughtAtStart(std::uncaught_exceptions())
        {
            if (!owner || owner->selectionArmed)
            {
                if (owner)
                    owner->selectionInvalid = true;
                owner = nullptr;
                return;
            }
            owner->selectionArmed = true;
            owner->selection = {};
            owner->selectionInvalid = false;
            owner->selectionStart = Clock::now();
            ULONG64 cycles{};
            owner->selectionStartCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (!owner->selectionStartCyclesValid)
                owner->selectionInvalid = true;
            owner->selectionStartCycles = cycles;
        }
        SelectionArm(const SelectionArm &) = delete;
        SelectionArm &operator=(const SelectionArm &) = delete;
        ~SelectionArm() noexcept
        {
            if (!owner)
                return;
            const auto now = Clock::now();
            ULONG64 cycles{};
            const bool endCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (!owner->selectionStartCyclesValid || !endCyclesValid || cycles < owner->selectionStartCycles || owner->selectionInvocationActive ||
                owner->selectionComponentActive || std::uncaught_exceptions() > uncaughtAtStart)
                owner->selectionInvalid = true;
            else
            {
                owner->selection.wallTotalUs = std::chrono::duration<double, std::micro>(now - owner->selectionStart).count();
                owner->selection.cpuTotalCycles = cycles - owner->selectionStartCycles;
            }
            owner->selection.complete = !owner->selectionInvalid && owner->selection.invocations == 1;
            owner->selectionArmed = false;
        }
    };

    class SelectionInvocation
    {
        EditDiagnostics *owner{};
        int uncaughtAtStart{};

      public:
        explicit SelectionInvocation(EditDiagnostics *value) noexcept
        {
            if (!value || !value->selectionArmed)
                return;
            owner = value;
            uncaughtAtStart = std::uncaught_exceptions();
            if (owner->selectionInvocationActive)
                owner->selectionInvalid = true;
            owner->selectionInvocationActive = true;
            ++owner->selection.invocations;
        }
        SelectionInvocation(const SelectionInvocation &) = delete;
        SelectionInvocation &operator=(const SelectionInvocation &) = delete;
        ~SelectionInvocation() noexcept
        {
            if (owner)
            {
                if (std::uncaught_exceptions() > uncaughtAtStart)
                    owner->selectionInvalid = true;
                owner->selectionInvocationActive = false;
            }
        }
    };

    class SelectionMeasure
    {
        EditDiagnostics *owner{};
        SelectionComponent component{};
        Clock::time_point start;
        std::uint64_t startCycles{};
        bool startCyclesValid{};

      public:
        SelectionMeasure(EditDiagnostics *value, SelectionComponent selected) noexcept : component(selected)
        {
            if (!value || !value->selectionArmed || !value->selectionInvocationActive || value->selectionInvalid)
                return;
            owner = value;
            if (owner->selectionComponentActive || static_cast<unsigned>(component) >= SelectionCount)
            {
                owner->selectionInvalid = true;
                owner = nullptr;
                return;
            }
            owner->selectionComponentActive = true;
            start = Clock::now();
            ULONG64 cycles{};
            startCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (!startCyclesValid)
                owner->selectionInvalid = true;
            startCycles = cycles;
        }
        SelectionMeasure(const SelectionMeasure &) = delete;
        SelectionMeasure &operator=(const SelectionMeasure &) = delete;
        ~SelectionMeasure() noexcept
        {
            if (!owner)
                return;
            const auto now = Clock::now();
            ULONG64 cycles{};
            const auto index = static_cast<unsigned>(component);
            const bool endCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (!startCyclesValid || !endCyclesValid || cycles < startCycles)
                owner->selectionInvalid = true;
            else if (!owner->selectionInvalid)
            {
                owner->selection.wallUs[index] += std::chrono::duration<double, std::micro>(now - start).count();
                owner->selection.cpuCycles[index] += cycles - startCycles;
                ++owner->selection.calls[index];
            }
            owner->selectionComponentActive = false;
        }
    };

    [[nodiscard]] SelectionArm ArmSelection() noexcept
    {
        return SelectionArm(this);
    }
    [[nodiscard]] SelectionInvocation EnterSelection() noexcept
    {
        return SelectionInvocation(this);
    }
    [[nodiscard]] SelectionMeasure MeasureSelection(SelectionComponent component) noexcept
    {
        return SelectionMeasure(this, component);
    }

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
