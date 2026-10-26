#pragma once
#include <DxUi/DxUi.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <wil/com.h>

namespace RedTilde
{
struct BrandMarkPixels
{
    UINT width{}, height{};
    std::vector<std::uint8_t> bgra;
};
// The application's bounded 32-bit alpha icon resource, not a general icon decoder.
// The handle is borrowed; returned premultiplied pixels own no GDI/COM resource.
[[nodiscard]] BrandMarkPixels LoadBrandMarkPixels(HICON icon);

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

// Read-only derived Source view. The primary pointer is borrowed for this
// control's lifetime and is checked before each opportunistic syntax adoption.
class SourceMirrorEditor final : public DxUi::RichTextEditor
{
  public:
    explicit SourceMirrorEditor(DxUi::RichTextEditor &primary) : primary_(&primary), primaryLifetime_(GetControlLifetimeToken(primary)) {}

  protected:
    [[nodiscard]] HRESULT PrepareContent(DxUi::ControlHost &host) noexcept override;

  private:
    DxUi::RichTextEditor *primary_{};
    std::weak_ptr<int> primaryLifetime_;
};

} // namespace RedTilde
