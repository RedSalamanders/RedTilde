#pragma once
#include <DxUi/DxUi.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace RedTilde
{
enum class PaneSide : std::uint8_t
{
    Left,
    Right
};

enum class PaneDisplayMode : std::uint8_t
{
    Hidden,
    Docked,
    CollapsedRail,
    HoverOverlay
};

enum class PaneAnimationPhase : std::uint8_t
{
    Settled,
    Collapsing,
    Expanding,
    HoverDwell,
    LeaveGrace
};

struct SidePaneState
{
    std::uint64_t phaseElapsedMs{};
    std::uint64_t lastTickMs{};
    float progress{};
    float startProgress{};
    PaneDisplayMode mode{PaneDisplayMode::Hidden};
    PaneAnimationPhase phase{PaneAnimationPhase::Settled};
    bool tickAnchored{};
    bool autoCollapsed{};
    bool collapseFromOverlay{};
    bool pointerInsideOverlayOrRail{};
};

struct RailDotItem
{
    std::uint64_t id{};
    std::size_t findMatchCount{};
    std::wstring name;
    std::wstring tooltip;
    std::wstring itemStatus;
    unsigned level{};
    bool active{};
};

class CollapsedPaneRail;

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
class RailDotButton final : public DxUi::Button
{
public:
    RailDotButton(CollapsedPaneRail& owner, std::size_t visibleIndex, PaneSide side);

    void SetDotItem(const RailDotItem& item, std::size_t visibleIndex);
    [[nodiscard]] const RailDotItem& DotItem() const noexcept
    {
        return _item;
    }

    void Paint(DxUi::ControlHost& host) const override;
    void OnHoverChanged(DxUi::ControlHost& host, bool hovered) override;
    bool OnMouseMove(DxUi::ControlHost& host, D2D1_POINT_2F point, UINT modifiers) override;
    bool OnKeyDown(DxUi::ControlHost& host, UINT virtualKey, UINT modifiers) override;
    bool OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F point, float wheelDelta, UINT modifiers) override;

private:
    CollapsedPaneRail* _owner{};
    std::size_t _visibleIndex{};
    RailDotItem _item;
    PaneSide _side{PaneSide::Right};
};

class CollapsedPaneRail final : public DxUi::Panel
{
public:
    static constexpr float RailWidthDip                   = 28.f;
    static constexpr float EdgeTriggerWidthDip            = 6.f;
    static constexpr float ExpandHandleTopInsetDip        = 6.f;
    static constexpr float ExpandHandleSizeDip            = 24.f;
    static constexpr float DotsStartOffsetDip             = 38.f;
    static constexpr float DotPitchDip                    = 22.f;
    static constexpr float DotHitWidthDip                 = 20.f;
    static constexpr float DotHitHeightDip                = 20.f;
    static constexpr float OverflowFooterHeightDip        = 22.f;
    static constexpr std::uint64_t HoverDwellDurationMs   = 180u;
    static constexpr std::uint64_t ExpandDurationMs       = 200u;
    static constexpr std::uint64_t CollapseDurationMs     = 200u;
    static constexpr std::uint64_t OverlayCloseDurationMs = 180u;
    static constexpr std::uint64_t LeaveGraceDurationMs   = 300u;
    static constexpr std::size_t MaximumPooledDots        = 64u;

    explicit CollapsedPaneRail(PaneSide side);

    void ConfigureStrings(std::wstring expandTooltip, std::wstring emptyTooltip);
    void SetItems(std::vector<RailDotItem> items, bool ensureActiveVisible = false);
    void LayoutChildren();

    [[nodiscard]] PaneSide Side() const noexcept
    {
        return _side;
    }
    [[nodiscard]] const SidePaneState& State() const noexcept
    {
        return _state;
    }
    void SetAutoCollapsed(bool value) noexcept
    {
        _state.autoCollapsed = value;
    }
    void SnapToMode(PaneDisplayMode mode, bool autoCollapsed) noexcept;
    void ArmHoverDwell(DxUi::ControlHost& host) noexcept;
    void CancelHoverDwell() noexcept;
    void BeginLeaveGrace(DxUi::ControlHost& host) noexcept;
    void CancelLeaveGrace() noexcept;
    void StartExpand(DxUi::ControlHost& host, bool toOverlay) noexcept;
    void StartCollapse(DxUi::ControlHost& host, bool fromOverlay) noexcept;

    void HandlePointerMoveDip(DxUi::ControlHost& host, D2D1_POINT_2F point, bool insideRailOrOverlay) noexcept;
    void NotifyDotHovered(DxUi::ControlHost* host, bool hovered) noexcept;
    void FocusAdjacentDot(DxUi::ControlHost& host, std::size_t currentVisibleIndex, int delta);

    [[nodiscard]] bool ContainsTriggerEdge(D2D1_POINT_2F point) const noexcept;
    [[nodiscard]] bool ContainsDot(D2D1_POINT_2F point) const noexcept;
    [[nodiscard]] bool ContainsExpandButton(D2D1_POINT_2F point) const noexcept;
    [[nodiscard]] bool IsHoverRevealZone(D2D1_POINT_2F point) const noexcept;
    [[nodiscard]] DxUi::Button* ExpandButton() const noexcept
    {
        return _expandButton;
    }
    [[nodiscard]] std::size_t ItemCount() const noexcept
    {
        return _items.size();
    }
    [[nodiscard]] std::size_t VisibleDotCount() const noexcept
    {
        return _visibleDotCount;
    }
    [[nodiscard]] std::size_t OverflowCount() const noexcept
    {
        return _overflowCount;
    }
    [[nodiscard]] std::size_t ScrollOffset() const noexcept
    {
        return _scrollOffset;
    }
    [[nodiscard]] RailDotButton* DotButton(std::size_t visibleIndex) const noexcept;
    [[nodiscard]] std::optional<std::size_t> ActiveItemIndex() const noexcept;

    void Paint(DxUi::ControlHost& host) const override;
    bool Tick(DxUi::ControlHost& host, uint64_t nowTickMs) override;
    bool OnMouseMove(DxUi::ControlHost& host, D2D1_POINT_2F point, UINT modifiers) override;
    bool OnMouseLeave(DxUi::ControlHost& host) override;
    bool OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F point, float wheelDelta, UINT modifiers) override;
    void OnHidden() noexcept override;
    void OnBoundsChanged() noexcept override;

    std::function<void(std::uint64_t)> onDotInvoked;
    std::function<void()> onExpandClicked;
    std::function<void()> onHoverReveal;
    std::function<void()> onAnimationStep;
    std::function<void()> onCollapseSettled;

private:
    void SyncVisibleDots();
    [[nodiscard]] std::size_t ComputeVisibleCapacity(bool reserveOverflow) const noexcept;

    std::vector<RailDotItem> _items;
    std::vector<RailDotButton*> _dotPool;
    SidePaneState _state;
    std::wstring _emptyTooltip;
    std::wstring _overflowBadgeText;
    DxUi::Button* _expandButton{};
    std::size_t _scrollOffset{};
    std::size_t _visibleDotCount{};
    std::size_t _overflowCount{};
    PaneSide _side{PaneSide::Right};
    int _wheelRemainder{};
};
#pragma warning(pop)
} // namespace RedTilde
