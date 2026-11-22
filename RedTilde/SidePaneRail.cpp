#include "SidePaneRail.h"
#include "Common/Resources.h"
#include "res/Resource.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <cmath>
#include <format>

namespace RedTilde
{
namespace
{
constexpr std::uint64_t MaxAnimationStepMs = 100u;

[[nodiscard]] bool PointInRect(D2D1_POINT_2F point, const D2D1_RECT_F& rect) noexcept
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}
} // namespace

RailDotButton::RailDotButton(CollapsedPaneRail& owner, std::size_t visibleIndex, PaneSide side)
    : DxUi::Button(L""),
      _owner(&owner),
      _visibleIndex(visibleIndex),
      _side(side)
{
    SetVariant(DxUi::ButtonVariant::Standard);
    SetAppearance(DxUi::ButtonAppearance::Subtle);
    SetFocusable(false);
    SetVisible(false);
}

void RailDotButton::SetDotItem(const RailDotItem& item, std::size_t visibleIndex)
{
    _item         = item;
    _visibleIndex = visibleIndex;
    SetAccessibleName(item.tooltip.empty() ? item.name : item.tooltip);
    SetTooltipText(item.tooltip);
    (void)SetAccessibleFormProperties(nullptr, false, true, item.itemStatus);
    SetFocusable(true);
    SetVisible(true);
    RequestInvalidate();
}

void RailDotButton::Paint(DxUi::ControlHost& host) const
{
    auto* target = host.GetDeviceContext();
    if (! target || ! IsVisible())
        return;

    const auto& theme  = host.GetTheme();
    const auto bounds  = GetBounds();
    const float baseCx = (bounds.left + bounds.right) * 0.5f;
    const float cy     = (bounds.top + bounds.bottom) * 0.5f;

    float radius  = 3.5f;
    float offsetX = 0.f;
    if (_side == PaneSide::Right)
    {
        if (_item.level <= 1u)
        {
            radius  = 4.0f;
            offsetX = 0.f;
        }
        else if (_item.level == 2u)
        {
            radius  = 3.0f;
            offsetX = 2.0f;
        }
        else
        {
            radius  = 2.25f;
            offsetX = 3.5f;
        }
    }
    else
    {
        radius  = _item.active ? 4.0f : 3.5f;
        offsetX = 0.f;
    }

    const float cx = baseCx + offsetX;

    if (theme.highContrast)
    {
        if (_item.active)
        {
            if (auto* halo = host.GetSolidBrush(theme.selectionFill))
                target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 6.5f, 6.5f), halo);
            if (auto* outerRing = host.GetSolidBrush(theme.focusStroke))
                target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 7.0f, 7.0f), outerRing, 1.5f);
        }
        else if (IsHovered())
        {
            if (auto* hoverBrush = host.GetSolidBrush(theme.text))
                target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 6.0f, 6.0f), hoverBrush, 1.25f);
        }
    }
    else
    {
        if (_item.active)
        {
            if (auto* halo = host.GetSolidBrush(theme.selectionFill))
                target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 6.5f, 6.5f), halo);
        }
        else if (IsHovered())
        {
            if (auto* hoverHalo = host.GetSolidBrush(theme.hoverFill))
                target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 6.5f, 6.5f), hoverHalo);
        }
    }

    if (_item.findMatchCount > 0u)
    {
        if (auto* matchRing = host.GetSolidBrush(theme.accent))
            target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius + 3.0f, radius + 3.0f), matchRing, 1.5f);
    }

    const D2D1_COLOR_F dotColor = (_side == PaneSide::Left || _item.active)
                                      ? (theme.highContrast && _item.active ? theme.selectionText : theme.accent)
                                      : (theme.highContrast ? theme.text : D2D1::ColorF(theme.subduedText.r, theme.subduedText.g, theme.subduedText.b, 0.65f));
    if (auto* dotBrush = host.GetSolidBrush(dotColor))
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), dotBrush);

    if (HasFocus())
    {
        const D2D1_RECT_F focusRect = D2D1::RectF(bounds.left + 1.0f, bounds.top + 1.0f, bounds.right - 1.0f, bounds.bottom - 1.0f);
        if (theme.highContrast)
        {
            if (auto* outerBrush = host.GetSolidBrush(theme.focusStrokeOuter))
                target->DrawRoundedRectangle(D2D1::RoundedRect(focusRect, 4.0f, 4.0f), outerBrush, 3.0f);
        }
        if (auto* focusBrush = host.GetSolidBrush(theme.focusStroke))
            target->DrawRoundedRectangle(D2D1::RoundedRect(focusRect, 4.0f, 4.0f), focusBrush, 1.5f);
    }
}

void RailDotButton::OnHoverChanged(DxUi::ControlHost& host, bool hovered)
{
    DxUi::Button::OnHoverChanged(host, hovered);
    if (_owner)
        _owner->NotifyDotHovered(&host, hovered);
}

bool RailDotButton::OnMouseMove(DxUi::ControlHost& host, D2D1_POINT_2F point, UINT modifiers)
{
    if (_owner)
        _owner->NotifyDotHovered(&host, true);
    return DxUi::Button::OnMouseMove(host, point, modifiers);
}

bool RailDotButton::OnKeyDown(DxUi::ControlHost& host, UINT virtualKey, UINT modifiers)
{
    if (_owner && (modifiers & MK_CONTROL) == 0u)
    {
        if (virtualKey == VK_UP)
        {
            _owner->FocusAdjacentDot(host, _visibleIndex, -1);
            return true;
        }
        if (virtualKey == VK_DOWN)
        {
            _owner->FocusAdjacentDot(host, _visibleIndex, +1);
            return true;
        }
        if ((_side == PaneSide::Left && virtualKey == VK_RIGHT) || (_side == PaneSide::Right && virtualKey == VK_LEFT))
        {
            if (auto* expand = _owner->ExpandButton())
                host.SetFocusControl(expand, host.GetTextInputHwnd() != nullptr);
            if (_owner->onExpandClicked)
                _owner->onExpandClicked();
            return true;
        }
    }
    return DxUi::Button::OnKeyDown(host, virtualKey, modifiers);
}

bool RailDotButton::OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F point, float wheelDelta, UINT modifiers)
{
    if (_owner)
        return _owner->OnMouseWheel(host, point, wheelDelta, modifiers);
    return false;
}

CollapsedPaneRail::CollapsedPaneRail(PaneSide side) : _side(side)
{
    _expandButton = AddChild<DxUi::Button>(L"");
    _expandButton->SetVariant(DxUi::ButtonVariant::IconOnly);
    _expandButton->SetAppearance(DxUi::ButtonAppearance::Subtle);
    (void)_expandButton->SetContentFontSizeDip(13.f);
    _expandButton->SetText(side == PaneSide::Left ? L"\xE76C" : L"\xE76B");
    _expandButton->SetOnClick([this]
    {
        if (onExpandClicked)
            onExpandClicked();
    });
}

void CollapsedPaneRail::ConfigureStrings(std::wstring expandTooltip, std::wstring emptyTooltip)
{
    _emptyTooltip = std::move(emptyTooltip);
    if (_expandButton)
    {
        _expandButton->SetAccessibleName(expandTooltip);
        _expandButton->SetTooltipText(std::move(expandTooltip));
    }
    if (_items.empty())
        SetTooltipText(_emptyTooltip);
    else
        SetTooltipText(L"");
}

void CollapsedPaneRail::SetItems(std::vector<RailDotItem> items, bool ensureActiveVisible)
{
    const bool countChanged = _items.size() != items.size();
    _items                  = std::move(items);
    if (_items.empty())
        SetTooltipText(_emptyTooltip);
    else
        SetTooltipText(L"");

    const std::size_t maxNoOverflow = ComputeVisibleCapacity(false);
    const std::size_t cap           = _items.size() > maxNoOverflow ? ComputeVisibleCapacity(true) : maxNoOverflow;
    if (ensureActiveVisible && cap > 0u && _items.size() > cap)
    {
        if (const auto active = ActiveItemIndex())
        {
            const std::size_t half       = cap / 2u;
            const std::size_t maxScroll  = _items.size() - cap;
            const std::size_t targetBase = active.value() > half ? active.value() - half : 0u;
            _scrollOffset                = std::min(targetBase, maxScroll);
        }
    }
    if (countChanged && _scrollOffset >= _items.size())
        _scrollOffset = 0u;
    SyncVisibleDots();
    RequestInvalidate();
}

std::size_t CollapsedPaneRail::ComputeVisibleCapacity(bool reserveOverflow) const noexcept
{
    const auto bounds         = GetBounds();
    const float availHeight   = std::max(0.f, bounds.bottom - bounds.top - DotsStartOffsetDip - (reserveOverflow ? OverflowFooterHeightDip : 8.f));
    const std::size_t bySpace = static_cast<std::size_t>(std::floor(availHeight / DotPitchDip));
    return std::min(MaximumPooledDots, bySpace);
}

void CollapsedPaneRail::SyncVisibleDots()
{
    const auto bounds                = GetBounds();
    const std::size_t maxNoOverflow  = ComputeVisibleCapacity(false);
    const bool hasOverflow           = _items.size() > maxNoOverflow;
    const std::size_t cap            = hasOverflow ? std::max<std::size_t>(1u, ComputeVisibleCapacity(true)) : maxNoOverflow;
    const std::size_t maxScroll      = cap > 0u && _items.size() > cap ? _items.size() - cap : 0u;
    _scrollOffset                    = std::min(_scrollOffset, maxScroll);
    const std::size_t visible        = _items.size() > _scrollOffset ? std::min(cap, _items.size() - _scrollOffset) : 0u;
    _visibleDotCount                 = visible;
    _overflowCount                   = _items.size() > visible ? _items.size() - visible : 0u;
    const std::size_t remainingBelow = _items.size() > _scrollOffset + visible ? _items.size() - (_scrollOffset + visible) : 0u;
    const std::size_t badgeCount     = remainingBelow > 0u ? remainingBelow : _overflowCount;
    _overflowBadgeText               = badgeCount > 0u ? FormatStringResource(nullptr, IDS_RAIL_MORE_ITEMS, badgeCount) : std::wstring{};

    while (_dotPool.size() < visible)
    {
        const std::size_t slotIndex = _dotPool.size();
        auto* dot                   = AddChild<RailDotButton>(*this, slotIndex, _side);
        dot->SetOnClick([this, slotIndex]
        {
            if (slotIndex < _visibleDotCount && _scrollOffset + slotIndex < _items.size() && onDotInvoked)
                onDotInvoked(_items[_scrollOffset + slotIndex].id);
        });
        _dotPool.push_back(dot);
    }

    const float dotLeft  = _side == PaneSide::Left ? bounds.left + EdgeTriggerWidthDip : bounds.left + 2.f;
    const float dotRight = _side == PaneSide::Left ? bounds.right - 2.f : bounds.right - EdgeTriggerWidthDip;
    for (std::size_t i = 0; i < _dotPool.size(); ++i)
    {
        auto* dot = _dotPool[i];
        if (i < visible)
        {
            const float dotTop = bounds.top + DotsStartOffsetDip + static_cast<float>(i) * DotPitchDip;
            dot->SetBounds(D2D1::RectF(dotLeft, dotTop, std::max(dotLeft + 8.f, dotRight), dotTop + DotHitHeightDip));
            dot->SetDotItem(_items[_scrollOffset + i], i);
        }
        else
        {
            dot->SetVisible(false);
            dot->SetFocusable(false);
        }
    }
}

void CollapsedPaneRail::LayoutChildren()
{
    const auto bounds = GetBounds();
    if (_expandButton)
    {
        const float left = bounds.left + 2.f;
        const float top  = bounds.top + ExpandHandleTopInsetDip;
        _expandButton->SetBounds(D2D1::RectF(left, top, std::max(left + 12.f, bounds.right - 2.f), top + ExpandHandleSizeDip));
    }
    SyncVisibleDots();
}

void CollapsedPaneRail::SnapToMode(PaneDisplayMode mode, bool autoCollapsed) noexcept
{
    _state.mode                = mode;
    _state.phase               = PaneAnimationPhase::Settled;
    _state.progress            = (mode == PaneDisplayMode::Docked || mode == PaneDisplayMode::HoverOverlay) ? 1.f : 0.f;
    _state.startProgress       = _state.progress;
    _state.phaseElapsedMs      = 0u;
    _state.lastTickMs          = 0u;
    _state.tickAnchored        = false;
    _state.autoCollapsed       = autoCollapsed;
    _state.collapseFromOverlay = false;
    RequestInvalidate();
}

void CollapsedPaneRail::ArmHoverDwell(DxUi::ControlHost& host) noexcept
{
    if (_state.mode != PaneDisplayMode::CollapsedRail)
        return;
    if (_state.phase == PaneAnimationPhase::HoverDwell || _state.phase == PaneAnimationPhase::Expanding)
        return;
    _state.phase          = PaneAnimationPhase::HoverDwell;
    _state.phaseElapsedMs = 0u;
    _state.tickAnchored   = false;
    host.RequestAnimation();
}

void CollapsedPaneRail::CancelHoverDwell() noexcept
{
    if (_state.phase == PaneAnimationPhase::HoverDwell)
    {
        _state.phase          = PaneAnimationPhase::Settled;
        _state.phaseElapsedMs = 0u;
        _state.tickAnchored   = false;
    }
}

void CollapsedPaneRail::BeginLeaveGrace(DxUi::ControlHost& host) noexcept
{
    if (_state.mode != PaneDisplayMode::HoverOverlay)
        return;
    if (_state.phase == PaneAnimationPhase::LeaveGrace || (_state.phase == PaneAnimationPhase::Collapsing && _state.collapseFromOverlay))
        return;
    _state.phase          = PaneAnimationPhase::LeaveGrace;
    _state.phaseElapsedMs = 0u;
    _state.tickAnchored   = false;
    host.RequestAnimation();
}

void CollapsedPaneRail::CancelLeaveGrace() noexcept
{
    if (_state.phase == PaneAnimationPhase::LeaveGrace)
    {
        _state.phase          = PaneAnimationPhase::Settled;
        _state.phaseElapsedMs = 0u;
        _state.tickAnchored   = false;
    }
}

void CollapsedPaneRail::StartExpand(DxUi::ControlHost& host, bool toOverlay) noexcept
{
    const PaneDisplayMode targetMode = toOverlay ? PaneDisplayMode::HoverOverlay : PaneDisplayMode::Docked;
    if (_state.mode == targetMode && _state.phase == PaneAnimationPhase::Settled && _state.progress >= 1.f)
        return;
    _state.mode                = targetMode;
    _state.collapseFromOverlay = false;
    if (host.GetTheme().reducedMotion)
    {
        _state.progress       = 1.f;
        _state.startProgress  = 1.f;
        _state.phase          = PaneAnimationPhase::Settled;
        _state.phaseElapsedMs = 0u;
        _state.tickAnchored   = false;
        Invalidate(host);
        return;
    }
    _state.startProgress  = _state.progress;
    _state.phase          = PaneAnimationPhase::Expanding;
    _state.phaseElapsedMs = 0u;
    _state.tickAnchored   = false;
    host.RequestAnimation();
    Invalidate(host);
}

void CollapsedPaneRail::StartCollapse(DxUi::ControlHost& host, bool fromOverlay) noexcept
{
    if (_state.mode == PaneDisplayMode::CollapsedRail && _state.phase == PaneAnimationPhase::Settled && _state.progress <= 0.f)
        return;
    _state.collapseFromOverlay = fromOverlay;
    if (host.GetTheme().reducedMotion)
    {
        _state.mode           = PaneDisplayMode::CollapsedRail;
        _state.progress       = 0.f;
        _state.startProgress  = 0.f;
        _state.phase          = PaneAnimationPhase::Settled;
        _state.phaseElapsedMs = 0u;
        _state.tickAnchored   = false;
        Invalidate(host);
        return;
    }
    _state.startProgress  = _state.progress > 0.f ? _state.progress : 1.f;
    _state.progress       = _state.startProgress;
    _state.phase          = PaneAnimationPhase::Collapsing;
    _state.phaseElapsedMs = 0u;
    _state.tickAnchored   = false;
    host.RequestAnimation();
    Invalidate(host);
}

void CollapsedPaneRail::HandlePointerMoveDip(DxUi::ControlHost& host, D2D1_POINT_2F point, bool insideRailOrOverlay) noexcept
{
    _state.pointerInsideOverlayOrRail = insideRailOrOverlay;
    if (! insideRailOrOverlay)
    {
        CancelHoverDwell();
        if (_state.mode == PaneDisplayMode::HoverOverlay)
            BeginLeaveGrace(host);
        return;
    }

    if (_state.mode == PaneDisplayMode::HoverOverlay)
    {
        CancelLeaveGrace();
        return;
    }

    if (_state.mode != PaneDisplayMode::CollapsedRail)
        return;

    if (ContainsDot(point))
    {
        CancelHoverDwell();
        return;
    }

    if (PointInRect(point, GetBounds()))
        ArmHoverDwell(host);
}

void CollapsedPaneRail::NotifyDotHovered(DxUi::ControlHost* /*host*/, bool hovered) noexcept
{
    if (hovered)
        CancelHoverDwell();
}

void CollapsedPaneRail::FocusAdjacentDot(DxUi::ControlHost& host, std::size_t currentVisibleIndex, int delta)
{
    if (_visibleDotCount == 0u)
        return;
    const auto nativeTransfer = host.GetTextInputHwnd() != nullptr;
    if (delta < 0)
    {
        if (currentVisibleIndex > 0u)
        {
            if (auto* target = DotButton(currentVisibleIndex - 1u))
                host.SetFocusControl(target, nativeTransfer);
        }
        else if (_scrollOffset > 0u)
        {
            --_scrollOffset;
            SyncVisibleDots();
            if (auto* target = DotButton(0u))
                host.SetFocusControl(target, nativeTransfer);
        }
        else if (_expandButton)
        {
            host.SetFocusControl(_expandButton, nativeTransfer);
        }
    }
    else if (delta > 0)
    {
        if (currentVisibleIndex + 1u < _visibleDotCount)
        {
            if (auto* target = DotButton(currentVisibleIndex + 1u))
                host.SetFocusControl(target, nativeTransfer);
        }
        else if (_scrollOffset + _visibleDotCount < _items.size())
        {
            ++_scrollOffset;
            SyncVisibleDots();
            if (auto* target = DotButton(_visibleDotCount - 1u))
                host.SetFocusControl(target, nativeTransfer);
        }
    }
}

bool CollapsedPaneRail::ContainsTriggerEdge(D2D1_POINT_2F point) const noexcept
{
    const auto bounds = GetBounds();
    if (! IsVisible() || ! PointInRect(point, bounds))
        return false;
    return _side == PaneSide::Left ? point.x < bounds.left + EdgeTriggerWidthDip : point.x >= bounds.right - EdgeTriggerWidthDip;
}

bool CollapsedPaneRail::ContainsDot(D2D1_POINT_2F point) const noexcept
{
    for (std::size_t i = 0; i < _visibleDotCount; ++i)
        if (_dotPool[i] && _dotPool[i]->IsVisible() && PointInRect(point, _dotPool[i]->GetBounds()))
            return true;
    return false;
}

RailDotButton* CollapsedPaneRail::DotButton(std::size_t visibleIndex) const noexcept
{
    return visibleIndex < _visibleDotCount ? _dotPool[visibleIndex] : nullptr;
}

std::optional<std::size_t> CollapsedPaneRail::ActiveItemIndex() const noexcept
{
    for (std::size_t i = 0; i < _items.size(); ++i)
        if (_items[i].active)
            return i;
    return std::nullopt;
}

void CollapsedPaneRail::Paint(DxUi::ControlHost& host) const
{
    auto* target = host.GetDeviceContext();
    if (! target || ! IsVisible())
        return;

    const auto& theme = host.GetTheme();
    const auto bounds = GetBounds();
    if (auto* bg = host.GetSolidBrush(theme.surfaceBackground))
        target->FillRectangle(bounds, bg);

    if (auto* line = host.GetSolidBrush(theme.borderDefault))
    {
        if (_side == PaneSide::Left)
            target->DrawLine(D2D1::Point2F(bounds.right - 0.5f, bounds.top), D2D1::Point2F(bounds.right - 0.5f, bounds.bottom), line);
        else
            target->DrawLine(D2D1::Point2F(bounds.left + 0.5f, bounds.top), D2D1::Point2F(bounds.left + 0.5f, bounds.bottom), line);
    }

    DxUi::Panel::Paint(host);

    if (! _overflowBadgeText.empty())
    {
        if (auto* format = host.GetTextFormat(
                DxUi::FontRole::Small, 10.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, false))
        {
            if (auto* textBrush = host.GetSolidBrush(theme.subduedText))
            {
                const D2D1_RECT_F badgeRect = D2D1::RectF(bounds.left + 1.f, bounds.bottom - OverflowFooterHeightDip, bounds.right - 1.f, bounds.bottom - 2.f);
                target->DrawTextW(
                    _overflowBadgeText.c_str(), static_cast<UINT32>(_overflowBadgeText.size()), format, badgeRect, textBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
}

bool CollapsedPaneRail::Tick(DxUi::ControlHost& host, uint64_t nowTickMs)
{
    if (_state.phase == PaneAnimationPhase::Settled)
        return false;

    if (! _state.tickAnchored)
    {
        _state.tickAnchored = true;
        _state.lastTickMs   = nowTickMs;
        return true;
    }

    const std::uint64_t stepMs = nowTickMs > _state.lastTickMs ? nowTickMs - _state.lastTickMs : 0u;
    _state.lastTickMs          = nowTickMs;
    _state.phaseElapsedMs += std::min(stepMs, MaxAnimationStepMs);

    if (_state.phase == PaneAnimationPhase::HoverDwell)
    {
        if (_state.phaseElapsedMs < HoverDwellDurationMs)
            return true;
        _state.phase          = PaneAnimationPhase::Settled;
        _state.phaseElapsedMs = 0u;
        _state.tickAnchored   = false;
        if (onHoverReveal)
            onHoverReveal();
        else
            StartExpand(host, true);
        return _state.phase != PaneAnimationPhase::Settled;
    }

    if (_state.phase == PaneAnimationPhase::LeaveGrace)
    {
        if (_state.phaseElapsedMs < LeaveGraceDurationMs)
            return true;
        StartCollapse(host, true);
        return _state.phase != PaneAnimationPhase::Settled;
    }

    if (_state.phase == PaneAnimationPhase::Expanding)
    {
        const float linear =
            host.GetTheme().reducedMotion ? 1.0f : std::clamp(static_cast<float>(_state.phaseElapsedMs) / static_cast<float>(ExpandDurationMs), 0.0f, 1.0f);
        const float eased = DxUi::EvaluateEasing(DxUi::EasingCurve::PointToPoint, linear);
        _state.progress   = std::lerp(_state.startProgress, 1.0f, eased);
        if (onAnimationStep)
            onAnimationStep();
        Invalidate(host);
        if (linear >= 1.0f)
        {
            _state.progress       = 1.0f;
            _state.startProgress  = 1.0f;
            _state.phase          = PaneAnimationPhase::Settled;
            _state.phaseElapsedMs = 0u;
            _state.tickAnchored   = false;
            return false;
        }
        return true;
    }

    if (_state.phase == PaneAnimationPhase::Collapsing)
    {
        const std::uint64_t duration = _state.collapseFromOverlay ? OverlayCloseDurationMs : CollapseDurationMs;
        const float linear =
            host.GetTheme().reducedMotion ? 1.0f : std::clamp(static_cast<float>(_state.phaseElapsedMs) / static_cast<float>(duration), 0.0f, 1.0f);
        const float eased = DxUi::EvaluateEasing(DxUi::EasingCurve::FastDecelerate, linear);
        _state.progress   = std::lerp(_state.startProgress, 0.0f, eased);
        if (onAnimationStep)
            onAnimationStep();
        Invalidate(host);
        if (linear >= 1.0f)
        {
            _state.progress            = 0.0f;
            _state.startProgress       = 0.0f;
            _state.mode                = PaneDisplayMode::CollapsedRail;
            _state.phase               = PaneAnimationPhase::Settled;
            _state.phaseElapsedMs      = 0u;
            _state.tickAnchored        = false;
            _state.collapseFromOverlay = false;
            if (onCollapseSettled)
                onCollapseSettled();
            return false;
        }
        return true;
    }

    return false;
}

bool CollapsedPaneRail::OnMouseMove(DxUi::ControlHost& host, D2D1_POINT_2F point, UINT /*modifiers*/)
{
    HandlePointerMoveDip(host, point, PointInRect(point, GetBounds()));
    return true;
}

bool CollapsedPaneRail::OnMouseLeave(DxUi::ControlHost& /*host*/)
{
    CancelHoverDwell();
    return true;
}

bool CollapsedPaneRail::OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F /*point*/, float wheelDelta, UINT /*modifiers*/)
{
    const std::size_t maxNoOverflow = ComputeVisibleCapacity(false);
    if (_items.size() <= maxNoOverflow)
        return false;
    const std::size_t cap       = std::max<std::size_t>(1u, ComputeVisibleCapacity(true));
    const std::size_t maxScroll = _items.size() - cap;
    _wheelRemainder += static_cast<int>(std::lround(wheelDelta));
    const int notches = _wheelRemainder / WHEEL_DELTA;
    _wheelRemainder %= WHEEL_DELTA;
    if (notches == 0)
        return false;
    const std::ptrdiff_t next = std::clamp(
        static_cast<std::ptrdiff_t>(_scrollOffset) - static_cast<std::ptrdiff_t>(notches) * 3, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(maxScroll));
    if (static_cast<std::size_t>(next) != _scrollOffset)
    {
        _scrollOffset = static_cast<std::size_t>(next);
        SyncVisibleDots();
        Invalidate(host);
        return true;
    }
    return false;
}

void CollapsedPaneRail::OnHidden() noexcept
{
    DxUi::Panel::OnHidden();
    CancelHoverDwell();
    CancelLeaveGrace();
    if (_state.phase == PaneAnimationPhase::Expanding)
    {
        _state.progress      = 1.f;
        _state.startProgress = 1.f;
        _state.phase         = PaneAnimationPhase::Settled;
    }
    else if (_state.phase == PaneAnimationPhase::Collapsing)
    {
        _state.progress            = 0.f;
        _state.startProgress       = 0.f;
        _state.mode                = PaneDisplayMode::CollapsedRail;
        _state.phase               = PaneAnimationPhase::Settled;
        _state.collapseFromOverlay = false;
    }
    _state.phaseElapsedMs = 0u;
    _state.tickAnchored   = false;
}

void CollapsedPaneRail::OnBoundsChanged() noexcept
try
{
    DxUi::Panel::OnBoundsChanged();
    LayoutChildren();
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}
catch (const std::exception&)
{
    DxUi::Failure::FailUnexpected();
}
} // namespace RedTilde
