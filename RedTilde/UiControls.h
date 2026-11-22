#pragma once
#include <DxUi/DxUi.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
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

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
class Surface : public DxUi::Panel
{
public:
    enum class Kind
    {
        Chrome,
        Canvas,
        Sidebar,
        Floating,
        DrawerLeft,
        DrawerRight
    };
    struct SplitChromeState
    {
        float headerHeightDip = 30.f;
        float splitX{};
        D2D1_RECT_F activeChipRect{};
        bool active{};
        bool sourceActive{};
        bool linked{true};
    };
    explicit Surface(Kind value = Kind::Chrome) : kind(value)
    {
    }
    // Product region boundaries; child control chrome remains library-owned.
    bool borderLeft{}, borderRight{}, borderBottom{};
    SplitChromeState splitChrome;
    void Paint(DxUi::ControlHost& host) const override;
    Kind kind;

private:
    mutable wil::com_ptr<ID2D1StrokeStyle> _dashedStroke;
};

class SubtleToggleButton final : public DxUi::Button
{
public:
    using DxUi::Button::Button;
    void Paint(DxUi::ControlHost& host) const override;
};

class Editor final : public DxUi::RichTextEditor
{
public:
    using DxUi::RichTextEditor::RichTextEditor;

    void SetSyntaxAdoptionPeer(Editor* peer) noexcept
    {
        _syntaxPeer         = peer;
        _syntaxPeerLifetime = peer ? GetControlLifetimeToken(*peer) : std::weak_ptr<int>{};
    }
    void SetOnBeforePassivePointerActivate(std::function<bool(std::optional<DxUi::RichText::Selection>)> callback)
    {
        _onBeforePassivePointerActivate = std::move(callback);
    }
    bool OnMouseDown(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers) noexcept override;

protected:
    [[nodiscard]] HRESULT PrepareContent(DxUi::ControlHost& host) noexcept override;

private:
    [[nodiscard]] bool IsAttachedTo(const DxUi::ControlHost& host, const DxUi::Control* parent) const noexcept
    {
        return GetHost() == &host && GetParent() == parent;
    }

    Editor* _syntaxPeer{};
    std::weak_ptr<int> _syntaxPeerLifetime;
    std::function<bool(std::optional<DxUi::RichText::Selection>)> _onBeforePassivePointerActivate;
};
#pragma warning(pop)

} // namespace RedTilde
