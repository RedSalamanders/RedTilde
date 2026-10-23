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
        }
        DxUi::Panel::Paint(host);
    }
    Kind kind;
};

using Editor = DxUi::RichTextEditor;

} // namespace RedTilde
