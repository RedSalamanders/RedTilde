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
        DrawerRight,
        ModalBackdrop
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
    bool OnMouseDown(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers) override;
    bool OnMouseUp(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers) override;
    bool OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F point, float wheelDelta, UINT modifiers) override;
    Kind kind;

private:
    mutable wil::com_ptr<ID2D1StrokeStyle> _dashedStroke;
    mutable std::uint64_t _dashedStrokeGeneration{};
};

class SubtleToggleButton final : public DxUi::Button
{
public:
    using DxUi::Button::Button;
    void Paint(DxUi::ControlHost& host) const override;
};

class DirtyDot final : public DxUi::Control
{
public:
    void Paint(DxUi::ControlHost& host) const override;
};

class StatusLanguageComboBox final : public DxUi::ComboBox
{
public:
    StatusLanguageComboBox();
    void InvalidateTextLayout() noexcept;
    void InvalidateTextLayoutIfDisplayedChanged() noexcept;
    void Paint(DxUi::ControlHost& host) const override;
#if defined(ENABLE_TESTS)
    [[nodiscard]] std::uint64_t PrepareVisualContentCount() const noexcept
    {
        return _prepareVisualContentCount;
    }
    [[nodiscard]] std::uint64_t PaintLayoutRebuildCount() const noexcept
    {
        return _paintLayoutRebuildCount;
    }
    [[nodiscard]] bool HasPreparedLayouts() const noexcept
    {
        return _plainLayout != nullptr && _chromeLayout != nullptr;
    }
    [[nodiscard]] const std::wstring& CachedText() const noexcept
    {
        return _cachedText;
    }
#endif

protected:
    void PrepareVisualContent(DxUi::ControlHost& host) noexcept override;
    void CancelVisualContentPreparation() noexcept override;
    void OnBoundsChanged() noexcept override;
    void OnHostDpiChanged(DxUi::ControlHost& host) noexcept override;
    void OnFlowDirectionChanged() noexcept override;
    void OnDensityChanged() noexcept override;
    void OnHidden() noexcept override;

private:
    void ResetCachedVisuals(bool resetEllipsis = false) noexcept;
    [[nodiscard]] bool IsLayoutCacheCurrent(const DxUi::ControlHost& host, std::wstring_view displayed, float widthDip, float heightDip) const noexcept;
    [[nodiscard]] bool EnsureLayoutsPrepared(DxUi::ControlHost& host, std::wstring_view displayed, float widthDip, float heightDip) const noexcept;

    mutable wil::com_ptr<IDWriteTextLayout> _plainLayout;
    mutable wil::com_ptr<IDWriteTextLayout> _chromeLayout;
    mutable wil::com_ptr<IDWriteInlineObject> _ellipsisSign;
    mutable std::wstring _cachedText;
    mutable float _cachedWidth{};
    mutable float _cachedHeight{};
    mutable float _cachedDpi{};
    mutable DxUi::FlowDirection _cachedFlowDirection{DxUi::FlowDirection::LeftToRight};
    mutable DxUi::Density _cachedDensity{DxUi::Density::Standard};
    mutable std::uint64_t _cachedResourceGeneration{};
    mutable std::uint64_t _cachedTypographyRevision{};
#if defined(ENABLE_TESTS)
    mutable std::uint64_t _prepareVisualContentCount{};
    mutable std::uint64_t _paintLayoutRebuildCount{};
#endif
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
