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
    using Clock                              = std::chrono::steady_clock;
    static constexpr unsigned Count          = static_cast<unsigned>(EditPhase::Count);
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
    enum class SelectionPhase : std::uint8_t
    {
        Disarmed,
        Armed,
        InInvocation,
        InComponent,
        Invalid
    };
    std::array<double, Count> wallUs{};
    std::array<std::uint64_t, Count> cpuCycles{};
    Clock::time_point previous;
    std::uint64_t previousCycles{};
    unsigned nextPhase{};
    bool valid = true;
    SelectionPhase selectionPhase{SelectionPhase::Disarmed};
    Clock::time_point selectionStart;
    std::uint64_t selectionStartCycles{};
    bool selectionStartCyclesValid{};

    class SelectionArm
    {
        EditDiagnostics* _owner{};
        int _uncaughtAtStart{};

    public:
        explicit SelectionArm(EditDiagnostics* value) noexcept : _owner(value), _uncaughtAtStart(std::uncaught_exceptions())
        {
            if (! _owner || _owner->selectionPhase != SelectionPhase::Disarmed)
            {
                if (_owner)
                    _owner->selectionPhase = SelectionPhase::Invalid;
                _owner = nullptr;
                return;
            }
            _owner->selectionPhase = SelectionPhase::Armed;
            _owner->selection      = {};
            _owner->selectionStart = Clock::now();
            ULONG64 cycles{};
            _owner->selectionStartCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (! _owner->selectionStartCyclesValid)
                _owner->selectionPhase = SelectionPhase::Invalid;
            _owner->selectionStartCycles = cycles;
        }
        SelectionArm(const SelectionArm&)            = delete;
        SelectionArm& operator=(const SelectionArm&) = delete;
        ~SelectionArm() noexcept
        {
            if (! _owner)
                return;
            const auto now = Clock::now();
            ULONG64 cycles{};
            const bool endCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (! _owner->selectionStartCyclesValid || ! endCyclesValid || cycles < _owner->selectionStartCycles ||
                _owner->selectionPhase != SelectionPhase::Armed || std::uncaught_exceptions() > _uncaughtAtStart)
                _owner->selectionPhase = SelectionPhase::Invalid;
            else
            {
                _owner->selection.wallTotalUs    = std::chrono::duration<double, std::micro>(now - _owner->selectionStart).count();
                _owner->selection.cpuTotalCycles = cycles - _owner->selectionStartCycles;
            }
            _owner->selection.complete = _owner->selectionPhase == SelectionPhase::Armed && _owner->selection.invocations == 1;
            _owner->selectionPhase     = SelectionPhase::Disarmed;
        }
    };

    class SelectionInvocation
    {
        EditDiagnostics* _owner{};
        int _uncaughtAtStart{};

    public:
        explicit SelectionInvocation(EditDiagnostics* value) noexcept
        {
            if (! value || value->selectionPhase == SelectionPhase::Disarmed)
                return;
            _owner           = value;
            _uncaughtAtStart = std::uncaught_exceptions();
            if (_owner->selectionPhase != SelectionPhase::Armed)
                _owner->selectionPhase = SelectionPhase::Invalid;
            else
                _owner->selectionPhase = SelectionPhase::InInvocation;
            ++_owner->selection.invocations;
        }
        SelectionInvocation(const SelectionInvocation&)            = delete;
        SelectionInvocation& operator=(const SelectionInvocation&) = delete;
        ~SelectionInvocation() noexcept
        {
            if (_owner)
            {
                if (std::uncaught_exceptions() > _uncaughtAtStart || _owner->selectionPhase != SelectionPhase::InInvocation)
                    _owner->selectionPhase = SelectionPhase::Invalid;
                else
                    _owner->selectionPhase = SelectionPhase::Armed;
            }
        }
    };

    class SelectionMeasure
    {
        EditDiagnostics* _owner{};
        SelectionComponent _component{};
        Clock::time_point _start;
        std::uint64_t _startCycles{};
        bool _startCyclesValid{};

    public:
        SelectionMeasure(EditDiagnostics* value, SelectionComponent selected) noexcept : _component(selected)
        {
            if (! value || value->selectionPhase == SelectionPhase::Disarmed || value->selectionPhase == SelectionPhase::Armed ||
                value->selectionPhase == SelectionPhase::Invalid)
                return;
            if (value->selectionPhase != SelectionPhase::InInvocation || static_cast<unsigned>(_component) >= SelectionCount)
            {
                value->selectionPhase = SelectionPhase::Invalid;
                return;
            }
            _owner                 = value;
            _owner->selectionPhase = SelectionPhase::InComponent;
            _start                 = Clock::now();
            ULONG64 cycles{};
            _startCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (! _startCyclesValid)
                _owner->selectionPhase = SelectionPhase::Invalid;
            _startCycles = cycles;
        }
        SelectionMeasure(const SelectionMeasure&)            = delete;
        SelectionMeasure& operator=(const SelectionMeasure&) = delete;
        ~SelectionMeasure() noexcept
        {
            if (! _owner)
                return;
            const auto now = Clock::now();
            ULONG64 cycles{};
            const auto index          = static_cast<unsigned>(_component);
            const bool endCyclesValid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
            if (! _startCyclesValid || ! endCyclesValid || cycles < _startCycles)
                _owner->selectionPhase = SelectionPhase::Invalid;
            else if (_owner->selectionPhase == SelectionPhase::InComponent)
            {
                _owner->selection.wallUs[index] += std::chrono::duration<double, std::micro>(now - _start).count();
                _owner->selection.cpuCycles[index] += cycles - _startCycles;
                ++_owner->selection.calls[index];
                _owner->selectionPhase = SelectionPhase::InInvocation;
            }
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
        valid          = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
        previousCycles = cycles;
    }
    void Mark(EditPhase phase) noexcept
    {
        const auto now = Clock::now();
        ULONG64 cycles{};
        const bool read  = QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE;
        const auto index = static_cast<unsigned>(phase);
        if (index != nextPhase || index >= Count || ! read || cycles < previousCycles)
        {
            valid = false;
            return;
        }
        wallUs[index]    = std::chrono::duration<double, std::micro>(now - previous).count();
        cpuCycles[index] = cycles - previousCycles;
        previous         = now;
        previousCycles   = cycles;
        ++nextPhase;
    }
    [[nodiscard]] bool Complete() const noexcept
    {
        return valid && nextPhase == Count;
    }
};
} // namespace RedTilde
