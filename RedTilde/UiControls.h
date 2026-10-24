#pragma once
#include <DxUi/DxUi.h>
#include <algorithm>
#include <functional>
#include <wil/com.h>

namespace RedTilde
{
class Surface : public DxUi::Panel
{
  public:
    enum class Kind
    {
        Chrome,
        Canvas,
        Sidebar,
        Floating
    };
    explicit Surface(Kind value = Kind::Chrome) : kind(value) {}
    // Product region boundaries; child control chrome remains library-owned.
    bool borderLeft{}, borderRight{}, borderBottom{};
    void Paint(DxUi::ControlHost &host) const override
    {
        auto *target = host.GetDeviceContext();
        if (!target)
            return;
        const auto &theme = host.GetTheme();
        const auto bounds = GetBounds();
        if (kind == Kind::Floating)
        {
            DxUi::TransientSurfaceOptions options;
            options.cornerRadiusDip = 10;
            DxUi::PaintTransientSurface(host, bounds, options);
        }
        else
        {
            const auto color = kind == Kind::Canvas ? theme.cardBackground : kind == Kind::Chrome ? theme.windowBackground : theme.surfaceBackground;
            target->FillRectangle(bounds, host.GetSolidBrush(color));
            if (auto *line = host.GetSolidBrush(theme.borderDefault))
            {
                if (borderLeft)
                    target->DrawLine(D2D1::Point2F(bounds.left, bounds.top), D2D1::Point2F(bounds.left, bounds.bottom), line);
                if (borderRight)
                    target->DrawLine(D2D1::Point2F(bounds.right - 1, bounds.top), D2D1::Point2F(bounds.right - 1, bounds.bottom), line);
                if (borderBottom)
                    target->DrawLine(D2D1::Point2F(bounds.left, bounds.bottom - 1), D2D1::Point2F(bounds.right, bounds.bottom - 1), line);
            }
        }
        DxUi::Panel::Paint(host);
    }
    Kind kind;
};

using Editor = DxUi::RichTextEditor;

} // namespace RedTilde
