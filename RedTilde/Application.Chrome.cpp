#include "Application.h"
#include "Common/Version.h"
#include "res/Resource.h"
#include <DxUi/Typography.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <wil/result.h>

namespace RedTilde
{
BrandMarkPixels LoadBrandMarkPixels(HICON icon)
{
    ICONINFO info{};
    const auto readIcon = GetIconInfo(icon, &info);
    wil::unique_hbitmap color(info.hbmColor), mask(info.hbmMask);
    THROW_IF_WIN32_BOOL_FALSE(readIcon);
    THROW_HR_IF_NULL(E_UNEXPECTED, color.get());
    BITMAP native{};
    THROW_LAST_ERROR_IF(GetObjectW(color.get(), sizeof(native), &native) != sizeof(native));
    THROW_HR_IF(E_UNEXPECTED,
                native.bmWidth <= 0 || native.bmWidth > 256 || native.bmHeight <= 0 || native.bmHeight > 256 || native.bmPlanes != 1 ||
                    native.bmBitsPixel != 32);

    BrandMarkPixels result{static_cast<UINT>(native.bmWidth), static_cast<UINT>(native.bmHeight), {}};
    result.bgra.resize(static_cast<std::size_t>(result.width) * result.height * 4);
    BITMAPINFO bitmap{};
    bitmap.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth       = native.bmWidth;
    bitmap.bmiHeader.biHeight      = -native.bmHeight;
    bitmap.bmiHeader.biPlanes      = 1;
    bitmap.bmiHeader.biBitCount    = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;
    wil::unique_hdc dc(CreateCompatibleDC(nullptr));
    THROW_LAST_ERROR_IF_NULL(dc);
    THROW_LAST_ERROR_IF(GetDIBits(dc.get(), color.get(), 0, result.height, result.bgra.data(), &bitmap, DIB_RGB_COLORS) != native.bmHeight);
    // GetDIBits returns straight BGRA for this owned alpha-icon asset. Match WIC's
    // round-to-nearest premultiplication exactly, including RGB under zero alpha.
    for (std::size_t offset = 0; offset < result.bgra.size(); offset += 4)
    {
        const unsigned alpha = result.bgra[offset + 3];
        for (unsigned channel = 0; channel < 3; ++channel)
            result.bgra[offset + channel] = static_cast<std::uint8_t>((result.bgra[offset + channel] * alpha + 127u) / 255u);
    }
    return result;
}

void Surface::Paint(DxUi::ControlHost& host) const
{
    auto* target = host.GetDeviceContext();
    if (! target)
        return;
    const auto& theme = host.GetTheme();
    const auto bounds = GetBounds();
    if (kind == Kind::ModalBackdrop)
    {
        auto scrim = theme.smokeOverlay;
        if (scrim.a <= 0.01f)
            scrim = theme.highContrast ? D2D1::ColorF(0.f, 0.f, 0.f, 0.65f) : D2D1::ColorF(0.f, 0.f, 0.f, 0.42f);
        if (auto* brush = host.GetSolidBrush(scrim))
            target->FillRectangle(bounds, brush);
        DxUi::Panel::Paint(host);
        return;
    }
    if (kind == Kind::Floating)
    {
        DxUi::TransientSurfaceOptions options;
        options.cornerRadiusDip = 10;
        DxUi::PaintTransientSurface(host, bounds, options);
    }
    else if (kind == Kind::DrawerLeft || kind == Kind::DrawerRight)
    {
        if (auto* bg = host.GetSolidBrush(theme.surfaceBackground))
            target->FillRectangle(bounds, bg);

        if (! theme.highContrast)
        {
            const bool dark = (0.2126f * theme.surfaceBackground.r + 0.7152f * theme.surfaceBackground.g + 0.0722f * theme.surfaceBackground.b) < 0.5f;
            constexpr std::array<float, 4> offsets{2.f, 4.f, 7.f, 11.f};
            constexpr std::array<float, 4> darkAlphas{0.18f, 0.11f, 0.055f, 0.025f};
            constexpr std::array<float, 4> lightAlphas{0.08f, 0.05f, 0.025f, 0.012f};
            const auto& alphas = dark ? darkAlphas : lightAlphas;
            float prev         = 0.f;
            for (std::size_t i = 0; i < offsets.size(); ++i)
            {
                const float next = offsets[i];
                if (auto* shadow = host.GetSolidBrush(D2D1::ColorF(0.f, 0.f, 0.f, alphas[i])))
                {
                    const D2D1_RECT_F strip = kind == Kind::DrawerLeft ? D2D1::RectF(bounds.right + prev, bounds.top, bounds.right + next, bounds.bottom)
                                                                       : D2D1::RectF(bounds.left - next, bounds.top, bounds.left - prev, bounds.bottom);
                    target->FillRectangle(strip, shadow);
                }
                prev = next;
            }
        }

        const float borderW = theme.highContrast ? 1.5f : 1.0f;
        if (auto* border = host.GetSolidBrush(theme.highContrast ? theme.border : theme.borderDefault))
        {
            const D2D1_RECT_F edge = kind == Kind::DrawerLeft ? D2D1::RectF(bounds.right - borderW, bounds.top, bounds.right, bounds.bottom)
                                                              : D2D1::RectF(bounds.left, bounds.top, bounds.left + borderW, bounds.bottom);
            target->FillRectangle(edge, border);
        }
    }
    else
    {
        const auto color = kind == Kind::Canvas ? theme.cardBackground : kind == Kind::Chrome ? theme.windowBackground : theme.surfaceBackground;
        target->FillRectangle(bounds, host.GetSolidBrush(color));
        if (kind == Kind::Canvas && splitChrome.active)
        {
            const float headerH = splitChrome.headerHeightDip;
            const float splitX  = std::clamp(splitChrome.splitX, bounds.left + 1.f, std::max(bounds.left + 1.f, bounds.right - 1.f));
            const float seamTop = std::min(bounds.bottom, bounds.top + headerH);
            if (! theme.highContrast)
            {
                auto passiveTint = theme.subduedText;
                passiveTint.a    = 0.025f;
                const auto pRect = splitChrome.sourceActive ? D2D1::RectF(bounds.left, seamTop, splitX, bounds.bottom)
                                                            : D2D1::RectF(splitX, seamTop, bounds.right, bounds.bottom);
                if (auto* wash = host.GetSolidBrush(passiveTint))
                    target->FillRectangle(pRect, wash);
            }
            if (auto* headerBrush = host.GetSolidBrush(theme.surfaceBackground))
                target->FillRectangle(D2D1::RectF(bounds.left, bounds.top, bounds.right, seamTop), headerBrush);
            if (auto* divider = host.GetSolidBrush(theme.borderDefault))
            {
                target->DrawLine(D2D1::Point2F(bounds.left, seamTop - 0.5f), D2D1::Point2F(bounds.right, seamTop - 0.5f), divider);
                if (splitChrome.linked)
                {
                    target->DrawLine(D2D1::Point2F(splitX, seamTop), D2D1::Point2F(splitX, bounds.bottom), divider);
                }
                else
                {
                    if (! _dashedStroke)
                    {
                        wil::com_ptr<ID2D1Factory> factory;
                        target->GetFactory(factory.put());
                        if (factory)
                        {
                            const D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
                                D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER, 10.f, D2D1_DASH_STYLE_DASH, 0.f);
                            (void)factory->CreateStrokeStyle(props, nullptr, 0, _dashedStroke.put());
                        }
                    }
                    target->DrawLine(D2D1::Point2F(splitX, seamTop), D2D1::Point2F(splitX, bounds.bottom), divider, 1.f, _dashedStroke.get());
                }
            }
            if (splitChrome.linked && seamTop < bounds.bottom)
            {
                auto seamAccent   = theme.highContrast ? theme.focusStroke : theme.accent;
                seamAccent.a      = theme.highContrast ? 1.0f : 0.55f;
                const float glowB = std::min(bounds.bottom, seamTop + 120.f);
                if (auto* accentBrush = host.GetSolidBrush(seamAccent))
                    target->DrawLine(D2D1::Point2F(splitX, seamTop), D2D1::Point2F(splitX, glowB), accentBrush, 2.f);
            }
            const auto accentRect = splitChrome.sourceActive ? D2D1::RectF(splitX, bounds.top, bounds.right, bounds.top + 2.f)
                                                             : D2D1::RectF(bounds.left, bounds.top, splitX, bounds.top + 2.f);
            if (auto* activeLine = host.GetSolidBrush(theme.highContrast ? theme.focusStroke : theme.accent))
                target->FillRectangle(accentRect, activeLine);
            if (splitChrome.activeChipRect.right > splitChrome.activeChipRect.left)
            {
                if (auto* chipFill = host.GetSolidBrush(theme.selectionFill))
                    target->FillRoundedRectangle(D2D1::RoundedRect(splitChrome.activeChipRect, 9.f, 9.f), chipFill);
            }
        }
        if (auto* line = host.GetSolidBrush(theme.borderDefault))
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

bool Surface::OnMouseDown(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers)
{
    if (DxUi::Panel::OnMouseDown(host, point, rightButton, modifiers))
        return true;
    return kind == Kind::ModalBackdrop;
}

bool Surface::OnMouseUp(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers)
{
    if (DxUi::Panel::OnMouseUp(host, point, rightButton, modifiers))
        return true;
    return kind == Kind::ModalBackdrop;
}

void SubtleToggleButton::Paint(DxUi::ControlHost& host) const
{
    DxUi::Button::Paint(host);
    if (! IsVisible() || ! IsEnabled() || ! GetToggleState().value_or(false) || (! IsHovered() && ! IsPressed()))
        return;

    auto* target = host.GetDeviceContext();
    if (! target)
        return;

    const auto& theme            = host.GetTheme();
    const auto b                 = GetBounds();
    const D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(b.left + 0.5f, b.top + 0.5f, b.right - 0.5f, b.bottom - 0.5f), 6.0f, 6.0f);
    if (theme.highContrast)
    {
        if (auto* border = host.GetSolidBrush(theme.focusStroke))
            target->DrawRoundedRectangle(pill, border, 1.0f);
        return;
    }
    if (auto* wash = host.GetSolidBrush(D2D1::ColorF(theme.text.r, theme.text.g, theme.text.b, IsPressed() ? 0.12f : 0.07f)))
        target->FillRoundedRectangle(pill, wash);
    if (auto* border = host.GetSolidBrush(D2D1::ColorF(theme.accent.r, theme.accent.g, theme.accent.b, IsPressed() ? 0.55f : 0.35f)))
        target->DrawRoundedRectangle(pill, border, 1.0f);
}

void DirtyDot::Paint(DxUi::ControlHost& host) const
{
    if (! IsVisible())
        return;

    auto* target = host.GetDeviceContext();
    if (! target)
        return;

    const auto& theme = host.GetTheme();
    const auto b      = GetBounds();
    const float cx    = (b.left + b.right) * 0.5f;
    const float cy    = (b.top + b.bottom) * 0.5f;
    const float r     = std::max(2.f, std::min(b.right - b.left, b.bottom - b.top) * 0.44f);
    if (auto* brush = host.GetSolidBrush(theme.highContrast ? theme.text : theme.accent))
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), brush);
}

void StatusLanguageComboBox::Paint(DxUi::ControlHost& host) const
{
    if (! IsVisible())
        return;

    auto* target = host.GetDeviceContext();
    if (! target)
        return;

    const auto& theme     = host.GetTheme();
    const auto b          = GetBounds();
    const float widthDip  = b.right - b.left;
    const float heightDip = b.bottom - b.top;
    if (widthDip <= 4.f || heightDip <= 4.f)
        return;

    const bool showChrome = IsHovered() || IsPopupOpen() || (HasFocus() && host.IsKeyboardFocusVisible());
    if (showChrome)
    {
        const D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(b.left + 0.5f, b.top + 1.5f, b.right - 0.5f, b.bottom - 1.5f), 4.f, 4.f);
        if (theme.highContrast)
        {
            if (auto* fill = host.GetSolidBrush(theme.windowBackground))
                target->FillRoundedRectangle(pill, fill);
            if (auto* border = host.GetSolidBrush((HasFocus() && host.IsKeyboardFocusVisible()) ? theme.focusStroke : theme.border))
                target->DrawRoundedRectangle(pill, border, 1.f);
        }
        else
        {
            if (auto* wash = host.GetSolidBrush(D2D1::ColorF(theme.text.r, theme.text.g, theme.text.b, IsPopupOpen() ? 0.10f : 0.06f)))
                target->FillRoundedRectangle(pill, wash);
            const auto& strokeColor = (HasFocus() && host.IsKeyboardFocusVisible()) ? theme.focusStroke : theme.borderDefault;
            if (auto* border = host.GetSolidBrush(strokeColor))
                target->DrawRoundedRectangle(pill, border, 1.f);
        }
    }

    const std::wstring_view displayed = GetDisplayedText();
    const float leftPad               = showChrome ? 8.f : 4.f;
    const float rightPad              = showChrome ? 22.f : 4.f;
    const float textW                 = std::max(8.f, widthDip - leftPad - rightPad);
    const std::uint64_t typoRev       = host.GetTypographyRevision();

    if (! _textLayout || _cachedText != displayed || _cachedWidth != textW || _cachedHeight != heightDip || _cachedTypographyRevision != typoRev)
    {
        _textLayout.reset();
        if (auto* factory = host.GetWriteFactory())
        {
            if (auto* format = host.GetTextFormat(
                    DxUi::FontRole::Small, 12.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, false))
            {
                wil::com_ptr<IDWriteTextLayout> layout;
                if (SUCCEEDED(factory->CreateTextLayout(displayed.data(), static_cast<UINT32>(displayed.size()), format, textW, heightDip, layout.put())) &&
                    layout)
                {
                    if (! _ellipsisSign)
                        (void)factory->CreateEllipsisTrimmingSign(format, _ellipsisSign.put());
                    const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0u, 0u};
                    (void)layout->SetTrimming(&trimming, _ellipsisSign.get());
                    _textLayout               = std::move(layout);
                    _cachedText               = std::wstring(displayed);
                    _cachedWidth              = textW;
                    _cachedHeight             = heightDip;
                    _cachedTypographyRevision = typoRev;
                }
            }
        }
    }

    const D2D1_COLOR_F ink = theme.highContrast ? theme.text : (showChrome ? theme.text : theme.subduedText);
    if (_textLayout)
    {
        if (auto* textBrush = host.GetSolidBrush(ink))
            target->DrawTextLayout(D2D1::Point2F(b.left + leftPad, b.top), _textLayout.get(), textBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    if (showChrome)
    {
        if (auto* iconFormat = host.GetTextFormat(
                DxUi::FontRole::Icon, 10.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, false))
        {
            if (auto* iconBrush = host.GetSolidBrush(ink))
            {
                constexpr wchar_t chevron  = L'\xE70D';
                const D2D1_RECT_F iconRect = D2D1::RectF(std::max(b.left + 4.f, b.right - 20.f), b.top, b.right - 4.f, b.bottom);
                target->DrawTextW(&chevron, 1u, iconFormat, iconRect, iconBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
}

bool Editor::OnMouseDown(DxUi::ControlHost& host, D2D1_POINT_2F point, bool rightButton, UINT modifiers) noexcept
{
    if (IsReadOnly() && ! rightButton && _onBeforePassivePointerActivate)
    {
        const auto viewport    = TryGetTextInputViewportRect();
        const auto info        = GetScrollInfo();
        const bool scrollable  = info && info->contentHeightDip > info->viewportHeightDip;
        const bool onScrollbar = scrollable && point.x >= GetBounds().right - 16.f;
        const bool inViewport  = viewport && point.x >= viewport->left && point.x < viewport->right && point.y >= viewport->top && point.y < viewport->bottom;
        if (! inViewport || onScrollbar)
            return DxUi::RichTextEditor::OnMouseDown(host, point, rightButton, modifiers);

        const auto lifetime = GetControlLifetimeToken(*this);
        std::optional<DxUi::RichText::Selection> clickedSelection;
        if (DxUi::RichTextEditor::OnMouseDown(host, point, rightButton, modifiers) && ! lifetime.expired() && GetHost() == &host)
            clickedSelection = GetSelection();
        if (lifetime.expired() || GetHost() != &host)
            return true;
        host.ReleaseMouseCapture();
        if (lifetime.expired() || GetHost() != &host)
            return true;
        const auto callback = _onBeforePassivePointerActivate;
        if (callback)
            (void)callback(clickedSelection);
        return true;
    }
    return DxUi::RichTextEditor::OnMouseDown(host, point, rightButton, modifiers);
}

HRESULT Editor::PrepareContent(DxUi::ControlHost& host) noexcept
{
    const auto selfLifetime = GetControlLifetimeToken(*this);
    auto* const peer        = _syntaxPeer;
    const auto peerLifetime = _syntaxPeerLifetime;
    auto* const owner       = host.GetRoot();
    if (! peer || peerLifetime.expired() || ! owner || ! IsVisible() || ! IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source ||
        GetHost() != &host || ! peer->IsAttachedTo(host, GetParent()))
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    if (ownerLifetime.expired())
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto source       = peer->Snapshot();
    const auto peerRevision = peer->Revision();
    const auto syntax       = peer->GetMarkdownSyntax();
    if (syntax && syntax->Revision() == peerRevision && source.IsSameRevision(peer->Snapshot()))
    {
        const auto currentSyntax  = GetMarkdownSyntax();
        const bool alreadyAdopted = currentSyntax && Snapshot().IsSameRevision(source) && currentSyntax->Revision() == Revision() &&
                                    currentSyntax->DocumentIdentity() != syntax->DocumentIdentity() && &currentSyntax->Nodes() == &syntax->Nodes();
        if (! alreadyAdopted)
        {
            const auto targetRevision = Revision();
            const HRESULT result      = TryAdoptReadOnlyMarkdownSyntax(source, syntax);
            if (FAILED(result))
                return result;

            // Adoption's final preparation request can re-enter and retire
            // either editor or replace the owning root. Prove all borrowers
            // still belong to this host before the ordinary preparation call.
            if (selfLifetime.expired() || peerLifetime.expired() || ownerLifetime.expired())
                return S_FALSE;
            if (GetHost() != &host || ! peer->IsAttachedTo(host, GetParent()) || host.GetRoot() != owner || ! source.IsSameRevision(peer->Snapshot()) ||
                peer->Revision() != peerRevision)
                return S_FALSE;
            if (result == S_OK && (! IsVisible() || ! IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source || Revision() != targetRevision ||
                                   ! Snapshot().IsSameRevision(source) || GetMarkdownAnalysisProfile() != syntax->Profile()))
                return S_FALSE;
        }
    }
    if (selfLifetime.expired())
        return S_FALSE;
    return DxUi::RichTextEditor::PrepareContent(host);
}

namespace
{
// Product branding only. Shared controls, focus, input and editor presentation
// belong to DxUi. Keep the resource bytes across device loss, never a second host.
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
class BrandMark final : public DxUi::Control
{
    BrandMarkPixels _source;
    wil::com_ptr<ID2D1Bitmap1> _bitmap;
    std::uint64_t _bitmapGeneration{};

public:
    BrandMark()
    {
        EnableVisualContentPreparation();
        // LoadIcon returns a module-owned shared icon. Retain just its pixels;
        // ordinary startup needs no WIC factory/converter for this known asset.
        const auto icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_REDTILDE));
        THROW_LAST_ERROR_IF_NULL(icon);
        _source = LoadBrandMarkPixels(icon);
    }
    void PrepareVisualContent(DxUi::ControlHost& host) noexcept override
    {
        const auto generation = host.GetResourceGeneration();
        if (_bitmap && _bitmapGeneration == generation)
            return;
        _bitmap.reset();
        _bitmapGeneration     = 0;
        auto* target          = host.GetDeviceContext();
        const auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (target &&
            SUCCEEDED(target->CreateBitmap(D2D1::SizeU(_source.width, _source.height), _source.bgra.data(), _source.width * 4, properties, _bitmap.put())))
            _bitmapGeneration = generation;
    }
    void CancelVisualContentPreparation() noexcept override
    {
        _bitmap.reset();
        _bitmapGeneration = 0;
    }
    void Paint(DxUi::ControlHost& host) const override
    {
        auto* target = host.GetDeviceContext();
        if (! target || ! _bitmap || _bitmapGeneration != host.GetResourceGeneration())
            return;
        target->DrawBitmap(_bitmap.get(), GetBounds());
    }
};
#pragma warning(pop)
void Section(DxUi::Label* label)
{
    THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, .88f}));
}
void Icon(DxUi::Button* button, std::wstring glyph)
{
    button->SetVariant(DxUi::ButtonVariant::IconOnly);
    THROW_IF_FAILED(button->SetContentFontSizeDip(18.f));
    button->SetText(std::move(glyph));
}
template <class Fn> SubtleToggleButton* MakeSubtleToggle(DxUi::Panel& parent, unsigned label, std::wstring tooltip, Fn&& onClick)
{
    auto* button = parent.AddChild<SubtleToggleButton>(Resource(label));
    THROW_IF_FAILED(button->SetContentFontSizeDip(14.f));
    button->SetAccessibleName(Resource(label));
    button->SetTooltipText(tooltip.empty() ? Resource(label) : std::move(tooltip));
    button->SetOnClick(std::forward<Fn>(onClick));
    button->SetAppearance(DxUi::ButtonAppearance::Subtle);
    return button;
}
[[nodiscard]] bool ContainsControl(const DxUi::Control* ancestor, const DxUi::Control* needle) noexcept
{
    if (! ancestor || ! needle)
        return false;
    if (ancestor == needle)
        return true;
    const std::size_t count = ancestor->GetLogicalChildCount();
    for (std::size_t i = 0; i < count; ++i)
        if (ContainsControl(ancestor->GetLogicalChild(i), needle))
            return true;
    return false;
}
} // namespace

HRESULT Application::BuildUi()
{
    const DxUi::Failure::Scope constructionScope;
    auto tree    = std::make_unique<Surface>();
    _root        = tree.get();
    _documentBar = _root->AddChild<Surface>();
    _brandMark   = _documentBar->AddChild<BrandMark>();
    _title       = Label(*_documentBar, IDS_UNTITLED, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_title->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _dirtyDot = _documentBar->AddChild<DirtyDot>();
    _dirtyDot->SetFocusable(false);
    _dirtyDot->SetAccessibleName(Resource(IDS_CHANGED));
    _dirtyDot->SetTooltipText(Resource(IDS_CHANGED));
    _dirtyDot->SetVisible(false);
    constexpr std::array<std::pair<unsigned, Command>, 3> viewModes{{
        {IDS_COMPOSE, Command::Compose},
        {IDS_SOURCE, Command::Source},
        {IDS_SPLIT, Command::Split},
    }};
    for (unsigned index = 0; index < viewModes.size(); ++index)
    {
        _modeButtons[index] = Action(*_documentBar, viewModes[index].first, viewModes[index].second);
        _modeButtons[index]->SetAccessibleAutomationId(std::format(L"RedTilde.View{}", index));
        _modeButtons[index]->SetAppearance(DxUi::ButtonAppearance::Tab);
    }
    _fileMenu = Action(*_documentBar, IDS_FILE, Command::FileMenu);
    _fileMenu->SetVariant(DxUi::ButtonVariant::DropDown);
    _fileMenu->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _saveButton = Action(*_documentBar, IDS_SAVE, Command::Save, Resource(IDS_TIP_SAVE));
    _saveButton->SetPrimary(true);
    _exitFocus      = Action(*_documentBar, IDS_EXITFOCUS, Command::Focus);
    _minimizeButton = Action(*_documentBar, IDS_MINIMIZE, Command::Minimize);
    _maximizeButton = Action(*_documentBar, IDS_MAXIMIZE, Command::Maximize);
    _closeButton    = Action(*_documentBar, IDS_CLOSE, Command::Close);
    Icon(_minimizeButton, L"\xE921");
    Icon(_maximizeButton, L"\xE922");
    Icon(_closeButton, L"\xE8BB");
    for (auto* caption : {_minimizeButton, _maximizeButton})
    {
        caption->SetFocusable(false);
        caption->SetAppearance(DxUi::ButtonAppearance::Caption);
        THROW_IF_FAILED(caption->SetContentFontSizeDip(12.f));
    }
    _closeButton->SetFocusable(false);
    _closeButton->SetAppearance(DxUi::ButtonAppearance::CaptionClose);
    THROW_IF_FAILED(_closeButton->SetContentFontSizeDip(12.f));
    _commandBar               = _root->AddChild<Surface>();
    _commandBar->borderBottom = true;
    const std::array<std::pair<unsigned, Command>, 14> buttons{{{IDS_UNDO, Command::Undo},
                                                                {IDS_REDO, Command::Redo},
                                                                {IDS_HEADING, Command::Heading},
                                                                {IDS_BOLD, Command::Bold},
                                                                {IDS_ITALIC, Command::Italic},
                                                                {IDS_CODE, Command::Code},
                                                                {IDS_BULLETS, Command::Bullets},
                                                                {IDS_TASK, Command::Task},
                                                                {IDS_INSERT, Command::Insert},
                                                                {IDS_FIND, Command::Find},
                                                                {IDS_LOCAL, Command::LocalSource},
                                                                {IDS_FOCUS, Command::Focus},
                                                                {IDS_SETTINGS, Command::Settings},
                                                                {IDS_LINK, Command::Link}}};
    for (const auto& [id, command] : buttons)
        _tools.push_back(Action(*_commandBar, id, command));
    for (auto* tool : _tools)
        tool->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _tools[2]->SetVariant(DxUi::ButtonVariant::DropDown);
    _tools[8]->SetVariant(DxUi::ButtonVariant::DropDown);
    const std::array<std::pair<unsigned, const wchar_t*>, 10> glyphs{{{0, L"\xE7A7"},
                                                                      {1, L"\xE7A6"},
                                                                      {3, L"\xE8DD"},
                                                                      {4, L"\xE8DB"},
                                                                      {5, L"\xE943"},
                                                                      {6, L"\xE8FD"},
                                                                      {7, L"\xE73A"},
                                                                      {11, L"\xE740"},
                                                                      {12, L"\xE713"},
                                                                      {13, L"\xE71B"}}};
    for (const auto& [index, glyph] : glyphs)
        Icon(_tools[index], glyph);
    _tools[3]->SetTooltipText(Resource(IDS_TIP_BOLD));
    _tools[4]->SetTooltipText(Resource(IDS_TIP_ITALIC));
    _tools[5]->SetTooltipText(Resource(IDS_TIP_CODE));
    _tools[13]->SetTooltipText(Resource(IDS_TIP_LINK));
    _tools[8]->SetLeadingIcon(L"\xE710");
    _tools[9]->SetText(Resource(IDS_FIND));
    _tools[9]->SetLeadingIcon(L"\xE721");
    _tools[9]->SetShortcutText(Resource(IDS_FIND_SHORTCUT));
    _settingsToggle = _tools[12];
    _filesToggle    = MakeSubtleToggle(*_commandBar, IDS_FILES, Resource(IDS_TIP_FILES), [this] { Execute(Command::Files); });
    Icon(_filesToggle, L"\xE90C");
    _panelToggle = MakeSubtleToggle(*_commandBar, IDS_PANEL, Resource(IDS_TIP_PANEL), [this] { Execute(Command::Outline); });
    Icon(_panelToggle, L"\xE90D");
    _overflowButton = Action(*_commandBar, IDS_MORE, Command::Overflow);
    Icon(_overflowButton, L"\xE712");
    _overflowButton->SetAppearance(DxUi::ButtonAppearance::Subtle);
    for (auto& divider : _commandDividers)
        divider = _commandBar->AddChild<DxUi::Separator>();
    _modeHint = Label(*_root, IDS_BANNER, DxUi::FontRole::Small);
    _modeHint->SetVisible(false);
    _page   = _root->AddChild<Surface>(Surface::Kind::Canvas);
    _editor = _page->AddChild<Editor>();
    _editor->SetAccessibleName(Resource(IDS_DOC));
    _editor->SetAccessibleAutomationId(L"RedTilde.Document");
    if (! AcceptSourceResult(_editor->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_editor->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _editor->SetCodeTokenizer(DxUi::RichText::TokenizeBuiltinCode);
    _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages);
    if (! AcceptSourceResult(_editor->TrySetViewMode(DxUi::RichText::ViewMode::Rich)))
        return E_INVALIDARG;
    _editor->SetOnTextChanged([this] { Changed(); });
    _editor->SetOnSelectionChanged([this] { SelectionChanged(); });
    _editor->SetBitmapPasteHandler([this](std::span<const std::byte> dib, bool v5) { return BeginBitmapPaste(dib, v5); });
    _editor->SetOnContextMenu([this](POINT point, bool keyboard) { Menu(point, false, false, keyboard); });
    _editor->SetOnLinkActivated([this](const DxUi::RichTextLinkActivation& activation) { RunUiAction([&] { RequestOpenLink(activation); }); });
    _editor->SetOnScrollChanged([this](float scrollDip)
    {
        OnSplitPaneScrolled(_editor, scrollDip);
        OnSpellScrollChanged();
    });
    _editor->SetOnBeforePassivePointerActivate([this](std::optional<DxUi::RichText::Selection> selection)
    {
        SwitchSplitActivePane(SplitActivePane::Compose, true, selection);
        return true;
    });
    _mirror = _page->AddChild<Editor>();
    _mirror->SetSyntaxAdoptionPeer(_editor);
    if (! AcceptSourceResult(_mirror->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_mirror->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _mirror->SetCodeTokenizer(DxUi::RichText::TokenizeBuiltinCode);
    _mirror->SetRemoteImagesEnabled(_settings.allowRemoteImages);
    _mirror->SetReadOnly(true);
    _mirror->SetOnScrollChanged([this](float scrollDip) { OnSplitPaneScrolled(_mirror, scrollDip); });
    _mirror->SetOnBeforePassivePointerActivate([this](std::optional<DxUi::RichText::Selection> selection)
    {
        SwitchSplitActivePane(SplitActivePane::Source, true, selection);
        return true;
    });
    _splitComposeTitle = Label(*_page, IDS_COMPOSE, DxUi::FontRole::Small);
    Section(_splitComposeTitle);
    _splitSourceTitle = Label(*_page, IDS_SOURCE, DxUi::FontRole::Small);
    Section(_splitSourceTitle);
    _splitComposeChipBg = _page->AddChild<DxUi::CardPanel>();
    _splitComposeChipBg->SetCornerRadius(9.f);
    _splitSourceChipBg = _page->AddChild<DxUi::CardPanel>();
    _splitSourceChipBg->SetCornerRadius(9.f);
    _splitComposeChip = Label(*_page, IDS_SPLIT_EDITING_CHIP, DxUi::FontRole::Small);
    THROW_IF_FAILED(_splitComposeChip->SetTypographyOverride(DxUi::LabelTypography{11.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _splitComposeChip->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _splitSourceChip = Label(*_page, IDS_READONLYSHORT, DxUi::FontRole::Small);
    THROW_IF_FAILED(_splitSourceChip->SetTypographyOverride(DxUi::LabelTypography{11.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _splitSourceChip->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _splitSwitchHint = Label(*_page, IDS_SPLIT_SWITCH_HINT, DxUi::FontRole::Small);
    THROW_IF_FAILED(_splitSwitchHint->SetTypographyOverride(DxUi::LabelTypography{11.5f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _splitSwitchHint->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _splitScrollSyncButton =
        MakeSubtleToggle(*_page, IDS_SPLIT_SCROLL_SYNC, Resource(IDS_TIP_SPLIT_SCROLL_ON), [this] { Execute(Command::ToggleSplitScrollSync); });
    Icon(_splitScrollSyncButton, L"\xE71B");
    THROW_IF_FAILED(_splitScrollSyncButton->SetContentFontSizeDip(14.f));
    _splitScrollSyncButton->SetToggleState(_settings.splitScrollSync);
    for (DxUi::Control* control : {static_cast<DxUi::Control*>(_splitComposeTitle),
                                   static_cast<DxUi::Control*>(_splitSourceTitle),
                                   static_cast<DxUi::Control*>(_splitComposeChipBg),
                                   static_cast<DxUi::Control*>(_splitSourceChipBg),
                                   static_cast<DxUi::Control*>(_splitComposeChip),
                                   static_cast<DxUi::Control*>(_splitSourceChip),
                                   static_cast<DxUi::Control*>(_splitSwitchHint),
                                   static_cast<DxUi::Control*>(_splitScrollSyncButton)})
        control->SetVisible(false);
    _imageConsent = _page->AddChild<Surface>(Surface::Kind::Floating);
    _imageConsent->SetVisible(false);
    _imageConsent->SetAccessibleAutomationId(L"RedTilde.RemoteImageConsent");
    _imageConsentText = Label(*_imageConsent, IDS_IMAGECONSENTBLOCKED, DxUi::FontRole::Small);
    _imageConsentText->SetMultiline(true);
    _imageConsentAction = Action(*_imageConsent, IDS_IMAGEALLOWHERE, Command::DocumentImages);
    _imageConsentAction->SetAccessibleAutomationId(L"RedTilde.AllowDocumentImages");
    _imageReload = Action(*_imageConsent, IDS_IMAGERELOAD, Command::ReloadImages);
    _imageReload->SetAccessibleAutomationId(L"RedTilde.ReloadImages");
    if (! AcceptSourceResult(_mirror->TrySetViewMode(DxUi::RichText::ViewMode::Source)))
        return E_INVALIDARG;
    _mirror->SetAccessibleName(Resource(IDS_MIRROR));
    _leftRail = _root->AddChild<CollapsedPaneRail>(PaneSide::Left);
    _leftRail->SetAccessibleName(Resource(IDS_RAIL_FILES_NAME));
    _leftRail->ConfigureStrings(Resource(IDS_RAIL_EXPAND_FILES), Resource(IDS_RAIL_NO_MODIFIED));
    _leftRail->SetVisible(false);
    _leftRail->onDotInvoked      = [this](std::uint64_t id) { RunUiAction([&] { OnRailDotActivated(PaneSide::Left, static_cast<std::size_t>(id)); }); };
    _leftRail->onExpandClicked   = [this] { Execute(Command::PinFiles); };
    _leftRail->onHoverReveal     = [this] { RunUiAction([&] { RevealPaneOverlay(PaneSide::Left); }); };
    _leftRail->onAnimationStep   = [this] { ApplySidePaneOverlayBounds(); };
    _leftRail->onCollapseSettled = [this]
    {
        RunUiAction([&]
        {
            ApplySidePaneOverlayBounds();
            if (! _settings.showFiles && _folderFiles.IsActive())
                RefreshFiles();
        });
    };
    _rightRail = _root->AddChild<CollapsedPaneRail>(PaneSide::Right);
    _rightRail->SetAccessibleName(Resource(IDS_RAIL_OUTLINE_NAME));
    _rightRail->ConfigureStrings(Resource(IDS_RAIL_EXPAND_OUTLINE), Resource(IDS_OUTLINEEMPTY));
    _rightRail->SetVisible(false);
    _rightRail->onDotInvoked      = [this](std::uint64_t id) { RunUiAction([&] { OnRailDotActivated(PaneSide::Right, static_cast<std::size_t>(id)); }); };
    _rightRail->onExpandClicked   = [this] { Execute(Command::PinOutline); };
    _rightRail->onHoverReveal     = [this] { RunUiAction([&] { RevealPaneOverlay(PaneSide::Right); }); };
    _rightRail->onAnimationStep   = [this] { ApplySidePaneOverlayBounds(); };
    _rightRail->onCollapseSettled = [this] { ApplySidePaneOverlayBounds(); };
    _sidebar                      = _root->AddChild<Surface>(Surface::Kind::Sidebar);
    _sidebar->borderLeft          = true;
    _outlineTitle                 = Label(*_sidebar, IDS_OUTLINESECTION, DxUi::FontRole::Small);
    Section(_outlineTitle);
    _outlineTree = _sidebar->AddChild<DxUi::Tree>();
    _outlineTree->SetModel(&_outlineModel);
    _outlineTree->SetDelegate(&_outlineModel);
    _outlineTree->SetRowHeightDip(42);
    _outlineTree->SetIndentDip(14);
    _outlineTree->SetAppearance({.drawBackground = false, .drawBorder = false, .showExpanders = false});
    THROW_IF_FAILED(_outlineTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    _outlineTree->SetInvokeOnSingleClick(true);
    _outlineTree->SetAccessibleName(Resource(IDS_OUTLINE));
    _outlineTree->SetEmptyStateText(Resource(IDS_OUTLINEEMPTY));
    _outlineModel.invoked = [this](std::uint64_t id)
    {
        if (! id || id > _documentInfo.headings.size())
            return;
        if (IsPaneOverlayActive(PaneSide::Right))
            DismissHoverOverlays(true);
        const auto offset  = _documentInfo.headings[static_cast<std::size_t>(id - 1)].offset;
        auto* const active = ActiveEditor();
        if (! AcceptSourceResult(active->TrySetSelection({offset, offset})))
            return;
        (void)active->ScrollSourceRangeIntoView(_host, offset, offset, true);
        _host.SetFocusControl(active, MayRequestNativeFocus());
        SelectionChanged();
    };
    _outlineModel.expanded = [this](std::uint64_t id, bool value)
    {
        if (value)
            _session.collapsedHeadings.erase(id);
        else
            _session.collapsedHeadings.insert(id);
        if (_outlinePreviewBaseline && PreviewOutlineMatchesCommitted())
            _outlinePreviewBaseline->collapsedHeadings = _session.collapsedHeadings;
        RefreshOutline();
    };
    _currentBlock = Label(*_sidebar, IDS_CURRENTBLOCK, DxUi::FontRole::Small);
    _listSummary  = Label(*_sidebar, IDS_BLOCKBULLETLIST, DxUi::FontRole::Small);
    THROW_IF_FAILED(_listSummary->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _listSummaryIcon = _sidebar->AddChild<DxUi::Label>(L"\xE8FD");
    _listSummaryIcon->SetFontRole(DxUi::FontRole::Icon);
    _listSummaryIcon->SetAccessibleName(Resource(IDS_BLOCKLIST));
    THROW_IF_FAILED(_currentBlock->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _blockSourceAction = Action(*_sidebar, IDS_LOCAL, Command::LocalSource);
    _blockSourceAction->SetAppearance(DxUi::ButtonAppearance::Outline);
    _equationHelp = Label(*_sidebar, IDS_EQUATIONSECTION, DxUi::FontRole::Small);
    Section(_equationHelp);
    _equation = _sidebar->AddChild<DxUi::RichTextEditor>();
    _equation->SetReadOnly(true);
    if (! AcceptSourceResult(_equation->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_equation->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _equation->SetAccessibleName(Resource(IDS_READONLY));
    ApplyMarkdownSyntax();

    _currentBlockTitle = Label(*_sidebar, IDS_CURRENTSECTION, DxUi::FontRole::Small);
    Section(_currentBlockTitle);
    _outlineCount = Label(*_sidebar, IDS_EMPTY, DxUi::FontRole::Small);
    THROW_IF_FAILED(_outlineCount->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _outlineCount->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _pinOutline = MakeSubtleToggle(*_sidebar, IDS_PIN_OUTLINE, Resource(IDS_PIN_OUTLINE), [this] { Execute(Command::PinOutline); });
    Icon(_pinOutline, L"\xE718");
    THROW_IF_FAILED(_pinOutline->SetContentFontSizeDip(12.f));
    _contextDivider         = _sidebar->AddChild<DxUi::Separator>();
    _equationReadOnly       = Label(*_sidebar, IDS_READONLYSHORT, DxUi::FontRole::Small);
    _filesPane              = _root->AddChild<Surface>(Surface::Kind::Sidebar);
    _filesPane->borderRight = true;
    _filesTitle             = Label(*_filesPane, IDS_FILES, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_filesTitle->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _filesRootIcon = _filesPane->AddChild<DxUi::Label>(L"\xE8B7");
    _filesRootIcon->SetFontRole(DxUi::FontRole::Icon);
    _filesRootIcon->SetAccessibleName(Resource(IDS_FILES));
    _filesSectionTitle = Label(*_filesPane, IDS_FILESSECTION, DxUi::FontRole::Small);
    Section(_filesSectionTitle);
    _filesAddFolder = Action(*_filesPane, IDS_ADD_FOLDER_SHORT, Command::AddFolder, Resource(IDS_ADDFOLDER));
    _filesAddFolder->SetLeadingIcon(L"\xE8F4");
    _filesAddFolder->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _filesAddFolder->SetAccessibleAutomationId(L"RedTilde.FilesAddFolder");
    _filesAddGitHub = Action(*_filesPane, IDS_GITHUB_SHORT, Command::OpenGitHub, Resource(IDS_OPENGITHUB));
    _filesAddGitHub->SetLeadingIcon(L"\xE774");
    _filesAddGitHub->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _filesAddGitHub->SetAccessibleAutomationId(L"RedTilde.FilesAddGitHub");
    _pinFiles = MakeSubtleToggle(*_filesPane, IDS_PIN_FILES, Resource(IDS_PIN_FILES), [this] { Execute(Command::PinFiles); });
    Icon(_pinFiles, L"\xE718");
    THROW_IF_FAILED(_pinFiles->SetContentFontSizeDip(12.f));
    _filesFilter = _filesPane->AddChild<DxUi::TextField>();
    _filesFilter->SetAccessibleName(Resource(IDS_FILTERFILES));
    _filesFilter->SetPlaceholder(Resource(IDS_FILTERSHORT));
    _filesFilter->SetOnTextChanged([this](std::wstring_view) { RefreshFiles(); });
    _filesTree = _filesPane->AddChild<DxUi::Tree>();
    _filesTree->SetModel(&_filesModel);
    _filesTree->SetDelegate(&_filesModel);
    _filesTree->SetAccessibleName(Resource(IDS_FILES));
    _filesTree->SetRowHeightDip(31);
    _filesTree->SetIndentDip(16);
    _filesTree->SetAppearance({.drawBackground = false, .drawBorder = false, .reserveExpanderSpace = true, .neutralDecorations = true});
    THROW_IF_FAILED(_filesTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    _filesTree->SetInvokeOnSingleClick(true);
    _filesTree->SetEmptyStateText(Resource(IDS_FILESUNTITLED));
    _filesEmptyOpenFolder = Action(*_filesPane, IDS_OPENFOLDER, Command::OpenFolder);
    _filesEmptyOpenFolder->SetVisible(false);
    _filesEmptyOpenFolder->SetAccessibleAutomationId(L"RedTilde.FilesEmptyOpenFolder");
    _filesEmptyOpenGitHub = Action(*_filesPane, IDS_OPENGITHUB, Command::OpenGitHub);
    _filesEmptyOpenGitHub->SetVisible(false);
    _filesEmptyOpenGitHub->SetAccessibleAutomationId(L"RedTilde.FilesEmptyOpenGitHub");
    _filesCount = Label(*_filesPane, IDS_FILESUNTITLED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_filesCount->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _filesCount->SetMultiline(true);
    _filesFooterDivider = _filesPane->AddChild<DxUi::Separator>();
    _filesFooterDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    _filesModel.contextMenu = [this](std::uint64_t id, POINT screenPoint) { OnFilesContextMenu(id, screenPoint); };
    _filesModel.invoked     = [this](std::uint64_t id)
    {
        if (id > WorkspaceGroupId && id <= WorkspaceGroupId + RecoveryService::MaximumDocuments)
        {
            const auto key = static_cast<unsigned>(id - WorkspaceGroupId - 1);
            if (key == _session.key)
            {
                _workspaceSelectionPreferred = true;
                if (_filesTree->GetSelectedItemId() != id)
                    _filesTree->SetSelectedItemId(id);
                if (IsPaneOverlayActive(PaneSide::Left))
                {
                    DismissHoverOverlays(true);
                    _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
                }
                return;
            }
            for (std::size_t index = 0; index < _inactiveDocuments.size(); ++index)
                if (_inactiveDocuments[index].metadata.key == key)
                {
                    const bool previous          = _workspaceSelectionPreferred;
                    _workspaceSelectionPreferred = true;
                    if (SwitchWorkspaceDocument(index))
                    {
                        if (IsPaneOverlayActive(PaneSide::Left))
                            DismissHoverOverlays(true);
                    }
                    else
                    {
                        _workspaceSelectionPreferred = previous;
                        if (IsUiAttached())
                            RefreshFiles();
                    }
                    break;
                }
            return;
        }
        const auto bindingIt = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == id; });
        if (bindingIt != _filesRowBindings.end())
        {
            const auto& binding = bindingIt->second;
            if (binding.kind == FilesRowContextBinding::Kind::LocalEntry)
            {
                if (! binding.isDirectory && IsMarkdownFile(binding.localPath) && IsPaneOverlayActive(PaneSide::Left))
                    DismissHoverOverlays(true);
                OpenPath(binding.localPath);
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::LocalRootHeader)
            {
                if (_collapsedLocalRoots.contains(binding.localRoot))
                    _collapsedLocalRoots.erase(binding.localRoot);
                else
                    _collapsedLocalRoots.insert(binding.localRoot);
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::GitHubRepoHeader)
            {
                const std::wstring key = binding.githubOwner + L"/" + binding.githubRepo;
                if (_collapsedGitHubRepos.contains(key))
                {
                    _collapsedGitHubRepos.erase(key);
                    for (auto& connected : _connectedGitHubRepos)
                    {
                        if (connected.ref.owner == binding.githubOwner && connected.ref.repo == binding.githubRepo && ! connected.loaded && ! connected.loading)
                        {
                            connected.loading = true;
                            GitHubRequest req;
                            req.kind      = GitHubRequest::Kind::FetchRepoTree;
                            req.requestId = ++_nextGitHubRequestId;
                            req.owner     = connected.ref.owner;
                            req.repo      = connected.ref.repo;
                            req.branch    = connected.ref.branch;
                            (void)_github.Submit(std::move(req));
                        }
                    }
                }
                else
                {
                    _collapsedGitHubRepos.insert(key);
                }
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::GitHubDir)
            {
                const std::wstring key = binding.githubOwner + L"/" + binding.githubRepo + L":" + binding.githubPath;
                if (_collapsedGitHubDirs.contains(key))
                    _collapsedGitHubDirs.erase(key);
                else
                    _collapsedGitHubDirs.insert(key);
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::GitHubFile)
            {
                if (IsPaneOverlayActive(PaneSide::Left))
                    DismissHoverOverlays(true);
                OpenGitHubDocument(binding.githubOwner, binding.githubRepo, binding.githubBranch, binding.githubPath);
                return;
            }
        }
        if (id && id <= _visibleFilePaths.size())
        {
            const auto path  = _visibleFilePaths[static_cast<std::size_t>(id - 1)];
            bool isDirectory = false;
            if (_folderSnapshot)
            {
                const auto& entries = _folderSnapshot->entries;
                if (const auto item = std::find_if(entries.begin(), entries.end(), [&](const FilesEntry& entry) { return entry.path == path; });
                    item != entries.end())
                    isDirectory = item->directory;
            }
            if (! isDirectory && IsMarkdownFile(path) && IsPaneOverlayActive(PaneSide::Left))
                DismissHoverOverlays(true);
            OpenPath(path);
        }
    };
    _filesModel.expanded = [this](std::uint64_t id, bool expanded)
    {
        if (id == WorkspaceGroupId)
        {
            _workspaceCollapsed = ! expanded;
            if (_workspaceCollapsed)
                _workspaceSelectionPreferred = false;
            RefreshFiles();
            return;
        }
        if (id == GitHubSectionGroupId)
        {
            _githubSectionCollapsed = ! expanded;
            RefreshFiles();
            return;
        }
        const auto bindingIt = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == id; });
        if (bindingIt != _filesRowBindings.end())
        {
            const auto& binding = bindingIt->second;
            if (binding.kind == FilesRowContextBinding::Kind::LocalRootHeader)
            {
                if (expanded)
                    _collapsedLocalRoots.erase(binding.localRoot);
                else
                    _collapsedLocalRoots.insert(binding.localRoot);
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::LocalEntry && binding.isDirectory)
            {
                if (expanded)
                    _collapsedFolders.erase(binding.localPath);
                else
                    _collapsedFolders.insert(binding.localPath);
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::GitHubRepoHeader)
            {
                const std::wstring key = binding.githubOwner + L"/" + binding.githubRepo;
                if (expanded)
                {
                    _collapsedGitHubRepos.erase(key);
                    for (auto& connected : _connectedGitHubRepos)
                    {
                        if (connected.ref.owner == binding.githubOwner && connected.ref.repo == binding.githubRepo && ! connected.loaded && ! connected.loading)
                        {
                            connected.loading = true;
                            GitHubRequest req;
                            req.kind      = GitHubRequest::Kind::FetchRepoTree;
                            req.requestId = ++_nextGitHubRequestId;
                            req.owner     = connected.ref.owner;
                            req.repo      = connected.ref.repo;
                            req.branch    = connected.ref.branch;
                            (void)_github.Submit(std::move(req));
                        }
                    }
                }
                else
                {
                    _collapsedGitHubRepos.insert(key);
                }
                RefreshFiles();
                return;
            }
            if (binding.kind == FilesRowContextBinding::Kind::GitHubDir)
            {
                const std::wstring key = binding.githubOwner + L"/" + binding.githubRepo + L":" + binding.githubPath;
                if (expanded)
                    _collapsedGitHubDirs.erase(key);
                else
                    _collapsedGitHubDirs.insert(key);
                RefreshFiles();
                return;
            }
        }
        if (! id || id > _visibleFilePaths.size())
            return;
        const auto path = _visibleFilePaths[static_cast<std::size_t>(id - 1)];
        if (expanded)
            _collapsedFolders.erase(path);
        else
            _collapsedFolders.insert(path);
        RefreshFiles();
    };
    _filesSplitter = _root->AddChild<DxUi::Splitter>();
    _filesSplitter->SetSubtle(true);
    _filesSplitter->SetAccessibleName(Resource(IDS_FILESWIDTH));
    _filesSplitter->SetMinimumFirstPane(200);
    _filesSplitter->SetOnChange([this](DxUi::SplitterChange change)
    {
        _settings.filesWidth = static_cast<unsigned>(std::clamp(change.position, 200.f, 400.f));
        Layout();
        if (change.phase == DxUi::SplitterChangePhase::Commit)
            SavePreferences();
    });

    _drawer = _root->AddChild<Surface>(Surface::Kind::DrawerRight);
    _drawer->SetVisible(false);
    _settingsTitle = Label(*_drawer, IDS_SETTINGS, DxUi::FontRole::Subtitle);
    _drawerClose   = Action(*_drawer, IDS_CLOSE, Command::CloseDrawer);
    Icon(_drawerClose, L"\xE8BB");
    _drawerClose->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _drawerClose->SetTooltipText(Resource(IDS_CLOSE));
    THROW_IF_FAILED(_drawerClose->SetContentFontSizeDip(12.f));
    _drawerImmediate       = Label(*_drawer, IDS_IMMEDIATE, DxUi::FontRole::Small);
    _settingsHeaderDivider = _drawer->AddChild<DxUi::Separator>();
    _settingsHeaderDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    _settingsScroll  = _drawer->AddChild<DxUi::ScrollPanel>();
    _appearanceTitle = Label(*_settingsScroll, IDS_APPEARANCESECTION, DxUi::FontRole::Small);
    Section(_appearanceTitle);
    constexpr std::array<std::pair<unsigned, Command>, 4> themes{{
        {IDS_SYSTEM, Command::ThemeSystem},
        {IDS_LIGHT, Command::ThemeLight},
        {IDS_DARK, Command::ThemeDark},
        {IDS_PAPER, Command::ThemePaper},
    }};
    for (const auto& [label, command] : themes)
        _themeButtons.push_back(Action(*_settingsScroll, label, command));
    _readingWidth = _settingsScroll->AddChild<DxUi::NumericStepper>();
    _readingWidth->SetLabel(Resource(IDS_READINGWIDTH), 140);
    _readingWidth->SetMinimum(480);
    _readingWidth->SetMaximum(1040);
    _readingWidth->SetStep(40);
    _readingWidth->SetLargeStep(80);
    _readingWidth->SetAccessibleName(Resource(IDS_READINGWIDTH));
    _readingWidth->SetStepButtonNames(Resource(IDS_WIDER), Resource(IDS_NARROWER));
    _readingWidth->SetOnChange([this](DxUi::NumericStepperChange c)
    {
        _settings.readingWidth = static_cast<unsigned>(c.value);
        Layout();
        if (c.phase == DxUi::NumericStepperChangePhase::Commit)
            SavePreferences();
    });
    _textScale = _settingsScroll->AddChild<DxUi::NumericStepper>();
    _textScale->SetLabel(Resource(IDS_TEXTSIZE), 140);
    _textScale->SetMinimum(75);
    _textScale->SetMaximum(200);
    _textScale->SetStep(5);
    _textScale->SetAccessibleName(Resource(IDS_TEXTSIZE));
    _textScale->SetStepButtonNames(Resource(IDS_LARGER), Resource(IDS_SMALLER));
    _textScale->SetOnChange([this](DxUi::NumericStepperChange c)
    {
        _settings.textScale = static_cast<unsigned>(c.value);
        Layout();
        if (c.phase == DxUi::NumericStepperChangePhase::Commit)
            SavePreferences();
    });
    _writingTitle = Label(*_settingsScroll, IDS_WRITINGSECTION, DxUi::FontRole::Small);
    Section(_writingTitle);
    _outlineToggle     = MakeSettingToggle(*_settingsScroll, IDS_VIEWTOOLS, Command::Outline);
    _focusToggle       = MakeSettingToggle(*_settingsScroll, IDS_FOCUS, Command::Focus);
    _dimToggle         = MakeSettingToggle(*_settingsScroll, IDS_DIMBLOCKS, Command::DimBlocks);
    _strikeTasksToggle = MakeSettingToggle(*_settingsScroll, IDS_STRIKECOMPLETEDTASKS, Command::StrikeCompletedTasks, Resource(IDS_STRIKECOMPLETEDTASKSHELP));
    _strikeTasksHelp   = Label(*_settingsScroll, IDS_STRIKECOMPLETEDTASKSHELP, DxUi::FontRole::Small);
    _strikeTasksHelp->SetMultiline(true);
    _keepUnsavedToggle = MakeSettingToggle(*_settingsScroll, IDS_SETTING_KEEP_UNSAVED, Command::ExitUnsavedBehavior, Resource(IDS_SETTING_KEEP_UNSAVED_HELP));
    _keepUnsavedHelp   = Label(*_settingsScroll, IDS_SETTING_KEEP_UNSAVED_HELP, DxUi::FontRole::Small);
    _keepUnsavedHelp->SetMultiline(true);
    _proofingTitle = Label(*_settingsScroll, IDS_PROOFINGSECTION, DxUi::FontRole::Small);
    Section(_proofingTitle);
    _spellingEnabledToggle = MakeSettingToggle(*_settingsScroll, IDS_SPELL_CHECK_TYPING, Command::ToggleSpelling, Resource(IDS_SPELL_CHECK_TYPING_HELP));
    _spellingHelp          = Label(*_settingsScroll, IDS_SPELL_CHECK_TYPING_HELP, DxUi::FontRole::Small);
    _spellingHelp->SetMultiline(true);
    _spellingLanguageLabel = Label(*_settingsScroll, IDS_SPELL_LANGUAGE_LABEL, DxUi::FontRole::Small);
    _spellingLanguageCombo = _settingsScroll->AddChild<DxUi::ComboBox>();
    _spellingLanguageCombo->SetVariant(DxUi::ComboBoxVariant::Modern);
    _spellingLanguageCombo->SetEditable(false);
    _spellingLanguageCombo->SetMaxVisibleItems(8u);
    _spellingLanguageCombo->SetAccessibleName(Resource(IDS_SPELL_LANGUAGE_LABEL));
    _spellingLanguageCombo->SetAccessibleAutomationId(L"RedTilde.SpellingLanguage");
    _spellingLanguageCombo->SetTooltipText(Resource(IDS_SPELL_LANGUAGE_SELECT_TIP));
    _spellingLanguageCombo->SetOnPopupRequested([this]
    {
        RunUiAction([&] { RefreshSpellLanguageCombos(true); });
        return false;
    });
    _spellingLanguageCombo->SetOnSelectionChanged([this](std::size_t index)
    {
        if (_spell.syncingLanguageCombos || ! _spellingLanguageCombo)
            return;
        const auto items = _spellingLanguageCombo->GetItems();
        if (index < items.size())
            ApplySpellLanguageSelection(items[index].value, false);
    });
    _spellingIgnoreUpperToggle  = MakeSettingToggle(*_settingsScroll, IDS_SPELL_IGNORE_UPPER, Command::SpellIgnoreUppercase);
    _spellingIgnoreDigitsToggle = MakeSettingToggle(*_settingsScroll, IDS_SPELL_IGNORE_DIGITS, Command::SpellIgnoreDigits);
    _spellingIgnorePathsToggle  = MakeSettingToggle(*_settingsScroll, IDS_SPELL_IGNORE_PATHS, Command::SpellIgnoreInternetPaths);
    _spellingDictPathLabel      = Label(*_settingsScroll, IDS_SPELL_CUSTOM_DICT, DxUi::FontRole::Small);
    _spellingDictChooseButton   = Action(*_settingsScroll, IDS_SPELL_DICT_CHOOSE, Command::SpellChooseCustomDictionary);
    _spellingDictResetButton    = Action(*_settingsScroll, IDS_SPELL_DICT_RESET, Command::SpellResetCustomDictionary);
    _syntaxTitle                = Label(*_settingsScroll, IDS_SYNTAXSECTION, DxUi::FontRole::Small);
    Section(_syntaxTitle);
    _syntaxIntro = Label(*_settingsScroll, IDS_SYNTAXINTRO, DxUi::FontRole::Small);
    _syntaxIntro->SetMultiline(true);
    struct SyntaxToggleRow
    {
        unsigned label{};
        unsigned help{};
        Command command{};
    };
    constexpr std::array<SyntaxToggleRow, MarkdownSyntaxCount> syntaxRows{{
        {IDS_SYNTAXHIGHLIGHT, IDS_SYNTAXHIGHLIGHTHELP, Command::SyntaxHighlight},
        {IDS_SYNTAXSUPERSCRIPT, IDS_SYNTAXSUPERSCRIPTHELP, Command::SyntaxSuperscript},
        {IDS_SYNTAXSUBSCRIPT, IDS_SYNTAXSUBSCRIPTHELP, Command::SyntaxSubscript},
        {IDS_SYNTAXINSERTED, IDS_SYNTAXINSERTEDHELP, Command::SyntaxInserted},
        {IDS_SYNTAXWIKILINKS, IDS_SYNTAXWIKILINKSHELP, Command::SyntaxWikiLinks},
        {IDS_SYNTAXDEFINITIONS, IDS_SYNTAXDEFINITIONSHELP, Command::SyntaxDefinitionLists},
        {IDS_SYNTAXABBREVIATIONS, IDS_SYNTAXABBREVIATIONSHELP, Command::SyntaxAbbreviations},
    }};
    for (std::size_t index = 0; index < syntaxRows.size(); ++index)
    {
        const auto& row       = syntaxRows[index];
        const auto help       = Resource(row.help);
        _syntaxToggles[index] = MakeSettingToggle(*_settingsScroll, row.label, row.command, help);
        _syntaxHelp[index]    = Label(*_settingsScroll, row.help, DxUi::FontRole::Small);
        _syntaxHelp[index]->SetMultiline(true);
    }
    _privacyTitle = Label(*_settingsScroll, IDS_PRIVACYSECTION, DxUi::FontRole::Small);
    Section(_privacyTitle);
    _remoteImagesToggle = MakeSettingToggle(*_settingsScroll, IDS_REMOTEIMAGES, Command::RemoteImages, Resource(IDS_LOCALIMAGES));
    _drawerHelp         = Label(*_settingsScroll, IDS_LOCALIMAGES, DxUi::FontRole::Small);
    _drawerHelp->SetMultiline(true);
    _localNetworkImagesToggle = MakeSettingToggle(*_settingsScroll, IDS_LOCALNETWORKIMAGES, Command::LocalNetworkImages, Resource(IDS_LOCALNETWORKIMAGESHELP));
    _localNetworkImagesHelp   = Label(*_settingsScroll, IDS_LOCALNETWORKIMAGESHELP, DxUi::FontRole::Small);
    _localNetworkImagesHelp->SetMultiline(true);
    _imageFirstPasteToggle = MakeSettingToggle(*_settingsScroll, IDS_IMAGEFIRSTPASTE, Command::ImageFirstPaste, Resource(IDS_IMAGEFIRSTPASTEHELP));
    _imageFirstPasteHelp   = Label(*_settingsScroll, IDS_IMAGEFIRSTPASTEHELP, DxUi::FontRole::Small);
    _imageFirstPasteHelp->SetMultiline(true);
    _recoveryAction = Action(*_settingsScroll, IDS_RECOVERY, Command::Recovery);
    _accountsTitle  = Label(*_settingsScroll, IDS_ACCOUNTS_SECTION, DxUi::FontRole::Small);
    Section(_accountsTitle);
    _accountsStatusLabel = Label(*_settingsScroll, IDS_GITHUB_NOT_SIGNED_IN, DxUi::FontRole::Small);
    _accountsStatusLabel->SetMultiline(true);
    _accountsDeviceBtn   = Action(*_settingsScroll, IDS_GITHUB_SIGNIN_DEVICE, Command::GitHubSignInDevice);
    _accountsImportGhBtn = Action(*_settingsScroll, IDS_GITHUB_IMPORT_GH, Command::GitHubImportGhCli);
    _accountsTokenInput  = _settingsScroll->AddChild<DxUi::TextField>();
    _accountsTokenInput->SetPlaceholder(Resource(IDS_GITHUB_TOKEN_PLACEHOLDER));
    _accountsTokenInput->SetAccessibleName(Resource(IDS_GITHUB_TOKEN_PLACEHOLDER));
    _accountsTokenInput->SetMasked(true);
    _accountsTokenInput->SetPasswordRevealMode(DxUi::PasswordRevealMode::Peek);
    _accountsTokenInput->SetOnSubmitted([this] { Execute(Command::GitHubUseToken); });
    _accountsUseTokenBtn = Action(*_settingsScroll, IDS_GITHUB_SAVE_TOKEN, Command::GitHubUseToken);
    _accountsSignOutBtn  = Action(*_settingsScroll, IDS_GITHUB_SIGNOUT, Command::GitHubSignOut);
    _aboutTitle          = Label(*_settingsScroll, IDS_ABOUTSECTION, DxUi::FontRole::Small);
    Section(_aboutTitle);
    _aboutVersion = _settingsScroll->AddChild<DxUi::Label>(FormatStringResource(nullptr, IDS_ABOUTVERSION, REDTILDE_PRODUCT_VERSION_WIDE));
    _aboutVersion->SetFontRole(DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_aboutVersion->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _aboutSummary = Label(*_settingsScroll, IDS_ABOUT, DxUi::FontRole::Small);
    _aboutSummary->SetMultiline(true);
    _shortcutsList = Label(*_settingsScroll, IDS_SHORTCUTS, DxUi::FontRole::Small);
    _shortcutsList->SetMultiline(true);
    _findBar = _root->AddChild<Surface>(Surface::Kind::Floating);
    _findBar->SetVisible(false);
    _findText = _findBar->AddChild<DxUi::TextField>();
    _findText->SetAccessibleName(Resource(IDS_FINDHINT));
    _findText->SetPlaceholder(Resource(IDS_FINDHINT));
    _findText->SetOnSubmitted([this] { FindNext(); });
    _findText->SetOnTextChanged([this](std::wstring_view) { RefreshFind(); });
    _findCount = Label(*_findBar, IDS_NOMATCH, DxUi::FontRole::Small);
    Icon(Action(*_findBar, IDS_FINDPREVIOUS, Command::FindPrevious), L"\xE70E");
    Icon(Action(*_findBar, IDS_FINDNEXT, Command::FindNext), L"\xE70D");
    Icon(Action(*_findBar, IDS_CLOSE, Command::CloseDrawer), L"\xE8BB");
    _replaceText = _findBar->AddChild<DxUi::TextField>();
    _replaceText->SetAccessibleName(Resource(IDS_REPLACEHINT));
    _replaceText->SetPlaceholder(Resource(IDS_REPLACEHINT));
    Action(*_findBar, IDS_REPLACE, Command::Replace);
    Action(*_findBar, IDS_REPLACEALL, Command::ReplaceAll);
    _findCaseToggle = Action(*_findBar, IDS_MATCHCASE, Command::FindMatchCase);
    _blockDraft     = _root->AddChild<Surface>(Surface::Kind::Floating);
    _blockDraft->SetVisible(false);
    Label(*_blockDraft, IDS_LOCAL, DxUi::FontRole::BodyStrong);
    Label(*_blockDraft, IDS_BLOCKHELP, DxUi::FontRole::Small);
    _blockDraftText = _blockDraft->AddChild<DxUi::TextField>();
    _blockDraftText->SetFontRole(DxUi::FontRole::Monospace);
    _blockDraftText->SetMultiline(true);
    _blockDraftText->SetAccessibleName(Resource(IDS_LOCAL));
    _blockDraftText->SetOnTextChanged([this](std::wstring_view)
    {
        if (HasBlockDraft())
            CheckpointRecovery();
    });
    _blockDraftText->SetOnPreviewKeyDown([this](DxUi::ControlHost&, UINT key, UINT modifiers)
    {
        if (key == VK_RETURN && (modifiers & MK_CONTROL))
        {
            Execute(Command::ApplyBlock);
            return true;
        }
        return false;
    });
    Action(*_blockDraft, IDS_APPLY, Command::ApplyBlock)->SetPrimary(true);
    Action(*_blockDraft, IDS_CANCEL, Command::CloseDrawer);
    _modalBackdrop = _root->AddChild<Surface>(Surface::Kind::ModalBackdrop);
    _modalBackdrop->SetVisible(false);
    _recoveryPanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _recoveryPanel->SetVisible(false);
    Label(*_recoveryPanel, IDS_RECOVERY, DxUi::FontRole::Subtitle);
    _recoveryDescription = Label(*_recoveryPanel, IDS_EMPTY, DxUi::FontRole::Small);
    _recoveryDescription->SetMultiline(true);
    _recoveryPreview = _recoveryPanel->AddChild<DxUi::TextField>();
    _recoveryPreview->SetMultiline(true);
    _recoveryPreview->SetReadOnly(true);
    _recoveryPreview->SetAccessibleName(Resource(IDS_RECOVERY));
    Action(*_recoveryPanel, IDS_RESTORE, Command::RestoreRecovery)->SetPrimary(true);
    Action(*_recoveryPanel, IDS_DISCARD, Command::DiscardRecovery);
    Action(*_recoveryPanel, IDS_LATER, Command::LaterRecovery);
    _restoreAllRecovery = Action(*_recoveryPanel, IDS_RESTOREALL, Command::RestoreAllRecovery);
    _restoreAllRecovery->SetVisible(false);
    _saveConfirmPanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _saveConfirmPanel->SetVisible(false);
    _saveConfirmPanel->SetAccessibleAutomationId(L"RedTilde.SaveConfirmModal");
    _saveConfirmTitle = Label(*_saveConfirmPanel, IDS_SAVE_CONFIRM_SINGLE_TITLE, DxUi::FontRole::Subtitle);
    _saveConfirmHint  = Label(*_saveConfirmPanel, IDS_SAVE_CONFIRM_HINT, DxUi::FontRole::Small);
    _saveConfirmHint->SetMultiline(true);
    _saveConfirmSelectAll = _saveConfirmPanel->AddChild<DxUi::Checkbox>(Resource(IDS_SELECT_ALL));
    _saveConfirmSelectAll->SetAccessibleAutomationId(L"RedTilde.SaveConfirmSelectAll");
    _saveConfirmSelectAll->SetChecked(true);
    _saveConfirmSelectAll->SetOnToggled([this](bool) { Execute(Command::SaveConfirmSelectAll); });
    _saveConfirmScroll = _saveConfirmPanel->AddChild<DxUi::ScrollPanel>();
    _saveConfirmScroll->SetAccessibleName(Resource(IDS_SAVE_CONFIRM_MULTI_TITLE));
    _saveConfirmPrimary = Action(*_saveConfirmPanel, IDS_SAVE, Command::SaveConfirmAccept);
    _saveConfirmPrimary->SetPrimary(true);
    _saveConfirmPrimary->SetAccessibleAutomationId(L"RedTilde.SaveConfirmPrimary");
    _saveConfirmKeepExit = Action(*_saveConfirmPanel, IDS_KEEP_AND_EXIT, Command::SaveConfirmKeepExit);
    _saveConfirmKeepExit->SetAccessibleAutomationId(L"RedTilde.SaveConfirmKeepExit");
    _saveConfirmDiscard = Action(*_saveConfirmPanel, IDS_DONT_SAVE, Command::SaveConfirmDiscard);
    _saveConfirmDiscard->SetAccessibleAutomationId(L"RedTilde.SaveConfirmDiscard");
    _saveConfirmCancel = Action(*_saveConfirmPanel, IDS_CANCEL, Command::CloseDrawer);
    _saveConfirmCancel->SetAccessibleAutomationId(L"RedTilde.SaveConfirmCancel");

    _renamePanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _renamePanel->SetVisible(false);
    _renamePanel->SetAccessibleAutomationId(L"RedTilde.RenameFileModal");
    _renameTitle    = Label(*_renamePanel, IDS_RENAME_TITLE, DxUi::FontRole::Subtitle);
    _renameLocation = Label(*_renamePanel, IDS_EMPTY, DxUi::FontRole::Small);
    _renameLocation->SetMultiline(true);
    _renameInput = _renamePanel->AddChild<DxUi::TextField>();
    _renameInput->SetAccessibleAutomationId(L"RedTilde.RenameFileInput");
    _renameInput->SetAccessibleName(Resource(IDS_RENAME_TITLE));
    _renameInput->SetOnSubmitted([this] { Execute(Command::RenameConfirm); });
    _renameInput->SetOnTextChanged([this](std::wstring_view text)
    {
        if (! _renameSession || ! _renameError || ! _renameConfirmBtn)
            return;
        WindowsWorkspaceFileOps fallbackOps;
        IWorkspaceFileOps* ops = &fallbackOps;
#if defined(ENABLE_TESTS)
        if (_options.fileOpsForTest)
            ops = _options.fileOpsForTest.get();
#endif
        const auto err = ops->ValidateFileName(text);
        const bool ok  = err == FileNameValidationError::None;
        _renameConfirmBtn->SetEnabled(ok);
        _renameError->SetText(ok ? std::wstring{} : Resource(IDS_RENAME_INVALID));
    });
    _renameError = Label(*_renamePanel, IDS_EMPTY, DxUi::FontRole::Small);
    _renameError->SetMultiline(true);
    _renameConfirmBtn = Action(*_renamePanel, IDS_CTX_RENAME, Command::RenameConfirm);
    _renameConfirmBtn->SetPrimary(true);
    Action(*_renamePanel, IDS_CANCEL, Command::CloseDrawer);

    _fileConflictPanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _fileConflictPanel->SetVisible(false);
    _fileConflictPanel->SetAccessibleAutomationId(L"RedTilde.FileConflictModal");
    _fileConflictTitle = Label(*_fileConflictPanel, IDS_FILE_CONFLICT_TITLE, DxUi::FontRole::Subtitle);
    _fileConflictDesc  = Label(*_fileConflictPanel, IDS_EMPTY, DxUi::FontRole::Small);
    _fileConflictDesc->SetMultiline(true);
    _conflictReplaceBtn  = Action(*_fileConflictPanel, IDS_REPLACE_TARGET, Command::FileConflictReplace);
    _conflictKeepBothBtn = Action(*_fileConflictPanel, IDS_KEEP_BOTH, Command::FileConflictKeepBoth);
    _conflictKeepBothBtn->SetPrimary(true);
    Action(*_fileConflictPanel, IDS_CANCEL, Command::CloseDrawer);

    _openGitHubPanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _openGitHubPanel->SetVisible(false);
    _openGitHubPanel->SetAccessibleAutomationId(L"RedTilde.OpenGitHubModal");
    _openGitHubTitle     = Label(*_openGitHubPanel, IDS_OPENGITHUB, DxUi::FontRole::Subtitle);
    _openGitHubAuthLabel = Label(*_openGitHubPanel, IDS_GITHUB_NOT_SIGNED_IN, DxUi::FontRole::Small);
    _openGitHubAuthLabel->SetMultiline(true);
    _openGitHubDeviceBtn   = Action(*_openGitHubPanel, IDS_GITHUB_SIGNIN_DEVICE, Command::GitHubSignInDevice);
    _openGitHubImportGhBtn = Action(*_openGitHubPanel, IDS_GITHUB_IMPORT_GH, Command::GitHubImportGhCli);
    _openGitHubTokenInput  = _openGitHubPanel->AddChild<DxUi::TextField>();
    _openGitHubTokenInput->SetPlaceholder(Resource(IDS_GITHUB_TOKEN_PLACEHOLDER));
    _openGitHubTokenInput->SetAccessibleName(Resource(IDS_GITHUB_TOKEN_PLACEHOLDER));
    _openGitHubTokenInput->SetMasked(true);
    _openGitHubTokenInput->SetPasswordRevealMode(DxUi::PasswordRevealMode::Peek);
    _openGitHubTokenInput->SetOnSubmitted([this] { Execute(Command::GitHubUseToken); });
    _openGitHubUseTokenBtn = Action(*_openGitHubPanel, IDS_GITHUB_SAVE_TOKEN, Command::GitHubUseToken);
    _openGitHubUrlInput    = _openGitHubPanel->AddChild<DxUi::TextField>();
    _openGitHubUrlInput->SetAccessibleAutomationId(L"RedTilde.OpenGitHubUrlInput");
    _openGitHubUrlInput->SetPlaceholder(Resource(IDS_GITHUB_URL_PLACEHOLDER));
    _openGitHubUrlInput->SetAccessibleName(Resource(IDS_GITHUB_URL_PLACEHOLDER));
    _openGitHubUrlInput->SetOnTextChanged([this](std::wstring_view) { RefreshOpenGitHubSuggestTree(); });
    _openGitHubUrlInput->SetOnSubmitted([this] { Execute(Command::GitHubOpenConfirm); });
    _openGitHubUrlInput->SetOnPreviewKeyDown([this](DxUi::ControlHost& host, UINT key, UINT)
    {
        if (key == VK_DOWN && _githubSuggestTree && ! _githubSuggestMatches.empty())
        {
            host.SetFocusControl(_githubSuggestTree, MayRequestNativeFocus());
            _githubSuggestTree->SetSelectedItemId(1u);
            return true;
        }
        return false;
    });
    _openGitHubBranchInput = _openGitHubPanel->AddChild<DxUi::TextField>();
    _openGitHubBranchInput->SetAccessibleAutomationId(L"RedTilde.OpenGitHubBranchInput");
    _openGitHubBranchInput->SetPlaceholder(Resource(IDS_GITHUB_BRANCH_PLACEHOLDER));
    _openGitHubBranchInput->SetAccessibleName(Resource(IDS_GITHUB_BRANCH_PLACEHOLDER));
    _openGitHubBranchInput->SetOnSubmitted([this] { Execute(Command::GitHubOpenConfirm); });
    _openGitHubSuggestLabel = Label(*_openGitHubPanel, IDS_GITHUB_SUGGESTIONS, DxUi::FontRole::Small);
    Section(_openGitHubSuggestLabel);
    _githubSuggestTree = _openGitHubPanel->AddChild<DxUi::Tree>();
    _githubSuggestTree->SetModel(&_githubSuggestModel);
    _githubSuggestTree->SetDelegate(&_githubSuggestModel);
    _githubSuggestTree->SetAccessibleAutomationId(L"RedTilde.OpenGitHubSuggestTree");
    _githubSuggestTree->SetAccessibleName(Resource(IDS_GITHUB_SUGGESTIONS));
    _githubSuggestTree->SetRowHeightDip(30);
    _githubSuggestTree->SetAppearance({.drawBackground = false, .drawBorder = true, .reserveExpanderSpace = false, .neutralDecorations = true});
    _githubSuggestTree->SetInvokeOnSingleClick(true);
    _githubSuggestModel.invoked = [this](std::uint64_t id)
    {
        if (id && id <= _githubSuggestMatches.size())
        {
            const auto& repo = _githubSuggestMatches[static_cast<std::size_t>(id - 1)];
            _openGitHubUrlInput->SetText(repo.fullName);
            if (_openGitHubBranchInput->GetText().empty())
                _openGitHubBranchInput->SetText(repo.defaultBranch);
            RefreshOpenGitHubSuggestTree();
        }
    };
    _openGitHubErrorLabel = Label(*_openGitHubPanel, IDS_EMPTY, DxUi::FontRole::Small);
    _openGitHubErrorLabel->SetMultiline(true);
    _openGitHubOpenBtn = Action(*_openGitHubPanel, IDS_GITHUB_OPEN_BTN, Command::GitHubOpenConfirm);
    _openGitHubOpenBtn->SetPrimary(true);
    _openGitHubOpenBtn->SetAccessibleAutomationId(L"RedTilde.OpenGitHubConfirm");
    Action(*_openGitHubPanel, IDS_CANCEL, Command::CloseDrawer);
    _focusPill = _root->AddChild<Surface>(Surface::Kind::Floating);
    _stats     = Label(*_focusPill, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_stats->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _positionStatus = Label(*_root, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_positionStatus->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _positionStatus->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _spellStatusCombo = _root->AddChild<StatusLanguageComboBox>();
    _spellStatusCombo->SetVariant(DxUi::ComboBoxVariant::Modern);
    _spellStatusCombo->SetDensity(DxUi::Density::Compact);
    _spellStatusCombo->SetEditable(false);
    _spellStatusCombo->SetMaxVisibleItems(8u);
    _spellStatusCombo->SetAccessibleName(Resource(IDS_SPELL_STATUS_COMBO_NAME));
    _spellStatusCombo->SetAccessibleAutomationId(L"RedTilde.SpellStatus");
    _spellStatusCombo->SetOnPopupRequested([this]
    {
        RunUiAction([&] { RefreshSpellLanguageCombos(true); });
        return false;
    });
    _spellStatusCombo->SetOnSelectionChanged([this](std::size_t index)
    {
        if (_spell.syncingLanguageCombos || ! _spellStatusCombo)
            return;
        const auto items = _spellStatusCombo->GetItems();
        if (index < items.size())
            ApplySpellLanguageSelection(items[index].value, true);
    });
    _feedback = Label(*_root, IDS_EMPTY, DxUi::FontRole::Small);
    for (auto* label : {_stats, _positionStatus, _feedback})
        THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _floating = _root->AddChild<Surface>(Surface::Kind::Floating);
    for (const auto& [id, cmd] : std::array<std::pair<unsigned, Command>, 5>{
             {{IDS_HEADING, Command::Heading}, {IDS_BOLD, Command::Bold}, {IDS_ITALIC, Command::Italic}, {IDS_CODE, Command::Code}, {IDS_LINK, Command::Link}}})
        _floatingActions.push_back(Action(*_floating, id, cmd));
    _floatingActions.front()->SetVariant(DxUi::ButtonVariant::DropDown);
    _selectionHintChip = _floating->AddChild<DxUi::CardPanel>();
    _selectionHintChip->SetCornerRadius(4);
    _selectionHint = Label(*_selectionHintChip, IDS_SELECTIONKEY, DxUi::FontRole::Small);
    THROW_IF_FAILED(_selectionHint->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _selectionHint->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _floating->SetVisible(false);
    _host.SetRoot(std::move(tree));
    _host.SetOnEscape([this] { return DismissSurface(); });
    return S_OK;
}

void Application::Layout()
{
    ++_activity.layouts;
    if (! _root)
        return;
    const auto bounds = _host.GetClientBoundsDip();
    const float width = bounds.right, height = bounds.bottom;
    _root->SetBounds(bounds);
    const bool focus = _settings.focusMode;
    const float bar = focus ? 40.f : 56.f, top = focus ? 40.f : 104.f, bottom = std::max(top + 120, height - 28);
    _documentBar->SetBounds(D2D1::RectF(0, 0, width, bar));
    _commandBar->SetBounds(D2D1::RectF(0, 56, width, 104));
    _commandBar->SetVisible(! focus);
    _brandMark->SetVisible(! focus);
    _brandMark->SetBounds(D2D1::RectF(16, 14, 44, 42));
    const auto name = _session.path.empty() ? Resource(IDS_UNTITLED) : _session.path.filename().native();
    _title->SetText(name);
    const bool compactHeader  = width < 1040;
    const float tabsLeft      = compactHeader ? width - 520 : width / 2 - 104;
    const float titleLeft     = focus ? 20.f : 54.f;
    const float titleLimit    = focus ? std::max(titleLeft + 56.f, width - 316.f) : std::max(titleLeft + 56.f, tabsLeft - 28.f);
    const auto typography     = _host.GetTypographyRevision();
    const float identityWidth = std::max(40.f, titleLimit - titleLeft);
    if (typography && (_identityMeasure.textRevision != _title->GetMeasureRevision() || _identityMeasure.typographyRevision != typography ||
                       _identityMeasure.availableWidth != identityWidth || _identityMeasure.dpi != _host.GetDpi()))
    {
        DxUi::DesiredSize identitySize;
        THROW_IF_FAILED(DxUi::MeasureControl(*_title, {_host}, DxUi::LayoutConstraints::AtMost(identityWidth), identitySize));
        _identityMeasure = {_title->GetMeasureRevision(), _host.GetTypographyRevision(), identityWidth, identitySize.sizeDip.width, _host.GetDpi()};
    }
    const float titleRight = typography ? std::min(titleLimit, titleLeft + _identityMeasure.width) : titleLimit;
    _title->SetBounds(focus ? D2D1::RectF(20.f, 6.f, titleRight, 34.f) : D2D1::RectF(54.f, 16.f, titleRight, 40.f));
    if (_dirtyDot)
    {
        const float dotCenterY = focus ? 20.f : 28.f;
        _dirtyDot->SetBounds(D2D1::RectF(titleRight + 8.f, dotCenterY - 4.f, titleRight + 16.f, dotCenterY + 4.f));
        _dirtyDot->SetVisible(Dirty());
    }
    constexpr std::array<float, 3> tabOffsets{0.f, 92.f, 170.f}, tabWidths{88.f, 74.f, 54.f};
    for (unsigned index = 0; index < 3; ++index)
    {
        _modeButtons[index]->SetVisible(! focus);
        _modeButtons[index]->SetBounds(D2D1::RectF(tabsLeft + tabOffsets[index], 8, tabsLeft + tabOffsets[index] + tabWidths[index], 46));
        _modeButtons[index]->SetToggleState(static_cast<unsigned>(_settings.view) == index);
    }
    _fileMenu->SetVisible(! focus);
    _fileMenu->SetBounds(D2D1::RectF(width - 294, 8, width - 230, 48));
    _saveButton->SetVisible(! focus);
    _saveButton->SetBounds(D2D1::RectF(width - 217, 8, width - 146, 48));
    _exitFocus->SetVisible(focus);
    _exitFocus->SetBounds(D2D1::RectF(width - 288, 4, width - 144, 36));
    const std::array<DxUi::Button*, 3> captions{_minimizeButton, _maximizeButton, _closeButton};
    for (unsigned i = 0; i < 3; ++i)
        captions[i]->SetBounds(D2D1::RectF(width - 138 + static_cast<float>(i) * 46.f, 0, width - 92 + static_cast<float>(i) * 46.f, std::min(36.f, bar)));
    _maximizeButton->SetText(IsZoomed(_window.get()) ? L"\xE923" : L"\xE922");
    float x    = 12;
    auto place = [&](DxUi::Button* button, float w)
    {
        button->SetBounds(D2D1::RectF(x, 62, x + w, 98));
        x += w + 2;
    };
    place(_filesToggle, 36);
    auto divide = [&](unsigned index)
    {
        _commandDividers[index]->SetVisible(true);
        _commandDividers[index]->SetBounds(D2D1::RectF(x + 8, 70, x + 9, 90));
        x += 19;
    };
    divide(0);
    place(_tools[0], 36);
    place(_tools[1], 36);
    divide(1);
    place(_tools[2], 128);
    divide(2);
    const float commandLimit = width - 248;
    _commandDividers[3]->SetVisible(false);
    _commandDividers[4]->SetVisible(false);
    for (const unsigned i : {3u, 4u, 5u, 13u, 6u, 7u, 8u})
    {
        const float w   = i == 8 ? 100.f : 36.f;
        const bool fits = x + w + 44 <= commandLimit;
        _tools[i]->SetVisible(fits);
        if (fits)
            place(_tools[i], w);
        if ((i == 13 || i == 7) && fits)
            divide(i == 13 ? 3 : 4);
    }
    _tools[10]->SetVisible(false);
    const bool overflow = std::ranges::any_of(std::array<unsigned, 7>{3, 4, 5, 13, 6, 7, 8}, [this](unsigned i) { return ! _tools[i]->IsVisible(); });
    _overflowButton->SetVisible(overflow);
    if (overflow)
        place(_overflowButton, 36);
    _commandDividers[5]->SetVisible(false);
    x = width - 248;
    place(_tools[9], 120);
    place(_tools[11], 36);
    place(_panelToggle, 36);
    place(_settingsToggle, 36);
    _settingsToggle->SetToggleState(_drawer->IsVisible());
    _tools[11]->SetToggleState(focus);
    constexpr float OutlineWidthDip         = 240.f;
    constexpr float MinPinnedCanvasWidthDip = 320.f;
    constexpr float MinPinnedSplitCanvasDip = 440.f;
    const float filesWidth                  = static_cast<float>(_settings.filesWidth);
    const bool split                        = _settings.view == EditorView::Split;
    const float minPinnedCanvas             = split ? MinPinnedSplitCanvasDip : MinPinnedCanvasWidthDip;
    const bool leftWantsDock                = ! focus && _settings.showFiles && _settings.filesPinMode != PanePinMode::Unpinned;
    const bool rightWantsDock               = ! focus && _settings.showOutline && _settings.outlinePinMode != PanePinMode::Unpinned;
    const bool leftPinned                   = leftWantsDock && _settings.filesPinMode == PanePinMode::Pinned;
    const bool rightPinned                  = rightWantsDock && _settings.outlinePinMode == PanePinMode::Pinned;
    const bool leftAuto                     = leftWantsDock && _settings.filesPinMode == PanePinMode::Auto;
    const bool rightAuto                    = rightWantsDock && _settings.outlinePinMode == PanePinMode::Auto;
    const float rightThreshold              = static_cast<float>(_settings.readingWidth + 336);
    bool leftDocked                         = false;
    bool rightDocked                        = false;

    if (leftPinned && rightPinned)
    {
        if (width - filesWidth - OutlineWidthDip >= minPinnedCanvas)
        {
            leftDocked  = true;
            rightDocked = true;
        }
        else if (_lastPinnedSide == PaneSide::Right)
        {
            rightDocked = (width - OutlineWidthDip - CollapsedPaneRail::RailWidthDip) >= minPinnedCanvas;
        }
        else
        {
            leftDocked = (width - filesWidth - CollapsedPaneRail::RailWidthDip) >= minPinnedCanvas;
        }
        _leftAutoCollapsed  = ! focus && ! leftDocked;
        _rightAutoCollapsed = ! focus && ! rightDocked;
    }
    else if (leftPinned)
    {
        leftDocked         = (width - filesWidth - CollapsedPaneRail::RailWidthDip) >= minPinnedCanvas;
        _leftAutoCollapsed = ! focus && ! leftDocked;
        const float lSpace = leftDocked ? filesWidth : CollapsedPaneRail::RailWidthDip;
        if (! rightAuto)
            _rightAutoCollapsed = ! focus;
        else if (split || _leftAutoCollapsed)
            _rightAutoCollapsed = true;
        else if (_rightAutoCollapsed)
            _rightAutoCollapsed = (width - lSpace) < rightThreshold + 24.f;
        else
            _rightAutoCollapsed = (width - lSpace) < rightThreshold;
        rightDocked = rightAuto && ! split && ! _rightAutoCollapsed;
    }
    else if (rightPinned)
    {
        rightDocked         = (width - OutlineWidthDip - CollapsedPaneRail::RailWidthDip) >= minPinnedCanvas;
        _rightAutoCollapsed = ! focus && ! rightDocked;
        if (! leftAuto)
            _leftAutoCollapsed = ! focus;
        else
        {
            const float rightReserve = rightDocked ? OutlineWidthDip : CollapsedPaneRail::RailWidthDip;
            const float leftThresh   = static_cast<float>(_settings.readingWidth + _settings.filesWidth + 96 + 6) + rightReserve;
            _leftAutoCollapsed       = _leftAutoCollapsed ? (width < leftThresh + 24.f) : (width < leftThresh);
        }
        leftDocked = leftAuto && ! _leftAutoCollapsed;
    }
    else
    {
        if (! leftAuto)
            _leftAutoCollapsed = ! focus;
        else
        {
            const float leftThresh = static_cast<float>(_settings.readingWidth + _settings.filesWidth + 96 + 6) + CollapsedPaneRail::RailWidthDip;
            _leftAutoCollapsed     = _leftAutoCollapsed ? (width < leftThresh + 24.f) : (width < leftThresh);
        }
        leftDocked         = leftAuto && ! _leftAutoCollapsed;
        const float lSpace = leftDocked ? filesWidth : CollapsedPaneRail::RailWidthDip;
        if (! rightAuto)
            _rightAutoCollapsed = ! focus;
        else if (split || (leftAuto && _leftAutoCollapsed))
            _rightAutoCollapsed = true;
        else if (_rightAutoCollapsed)
            _rightAutoCollapsed = (width - lSpace) < rightThreshold + 24.f;
        else
            _rightAutoCollapsed = (width - lSpace) < rightThreshold;
        rightDocked = rightAuto && ! split && ! _rightAutoCollapsed;
    }

    _filesToggle->SetToggleState(leftDocked);
    _panelToggle->SetToggleState(rightDocked);
    const bool leftCollapsed  = ! focus && ! leftDocked;
    const bool rightCollapsed = ! focus && ! rightDocked;
    const float left          = focus ? 0.f : (leftDocked ? filesWidth : CollapsedPaneRail::RailWidthDip);
    const float right         = focus ? width : (rightDocked ? width - OutlineWidthDip : width - CollapsedPaneRail::RailWidthDip);

    auto syncRail = [&](CollapsedPaneRail* rail, bool collapsed)
    {
        if (! rail)
            return;
        const auto& state = rail->State();
        if (focus)
        {
            rail->SnapToMode(PaneDisplayMode::Hidden, false);
        }
        else if (! collapsed)
        {
            rail->SnapToMode(PaneDisplayMode::Docked, false);
        }
        else
        {
            rail->SetAutoCollapsed(true);
            if (IsInitializing() || state.mode == PaneDisplayMode::Hidden)
                rail->SnapToMode(PaneDisplayMode::CollapsedRail, true);
            else if (state.mode == PaneDisplayMode::Docked && state.phase != PaneAnimationPhase::Collapsing)
                rail->StartCollapse(_host, false);
        }
    };
    syncRail(_leftRail, leftCollapsed);
    syncRail(_rightRail, rightCollapsed);
    if (_leftRail)
    {
        const auto& ls      = _leftRail->State();
        const bool showRail = ! focus && (ls.autoCollapsed || ls.mode == PaneDisplayMode::CollapsedRail || ls.mode == PaneDisplayMode::HoverOverlay ||
                                          ls.phase == PaneAnimationPhase::Collapsing);
        _leftRail->SetVisible(showRail);
        _leftRail->SetBounds(D2D1::RectF(0.f, top, CollapsedPaneRail::RailWidthDip, bottom));
    }
    if (_rightRail)
    {
        const auto& rs      = _rightRail->State();
        const bool showRail = ! focus && (rs.autoCollapsed || rs.mode == PaneDisplayMode::CollapsedRail || rs.mode == PaneDisplayMode::HoverOverlay ||
                                          rs.phase == PaneAnimationPhase::Collapsing);
        _rightRail->SetVisible(showRail);
        _rightRail->SetBounds(D2D1::RectF(width - CollapsedPaneRail::RailWidthDip, top, width, bottom));
    }

    const auto layoutRequest = _activity.layouts;
    ApplySidePaneOverlayBounds();
    if (_activity.layouts != layoutRequest)
        return;

    _page->SetBounds(D2D1::RectF(left, top, right, bottom));
    const float mid           = std::floor((left + right) * 0.5f);
    const float splitHeaderH  = split && ! focus ? 30.f : 0.f;
    const float gutter        = focus ? 32.f : split ? 24.f : 48.f;
    const float consentHeight = right - left < 600.f ? 104.f : 64.f;
    const float consentTop    = top + splitHeaderH + 12.f;
    const float contentTop    = std::max(top + splitHeaderH +
                                             (focus   ? 36.f
                                              : split ? 20.f
                                                      : 60.f),
                                         _imageConsent->IsVisible() ? consentTop + consentHeight + 12.f : top + splitHeaderH);
    _imageConsent->SetBounds(D2D1::RectF(left + 12.f, consentTop, right - 12.f, consentTop + consentHeight));
    const bool narrowConsent = right - left < 600.f;
    _imageConsentText->SetBounds(D2D1::RectF(left + 24.f, consentTop + 8.f, narrowConsent ? right - 24.f : right - 256.f, consentTop + 48.f));
    const float consentButtonsTop = consentTop + (narrowConsent ? 60.f : 14.f);
    _imageConsentAction->SetBounds(
        D2D1::RectF(narrowConsent ? left + 24.f : right - 244.f, consentButtonsTop, narrowConsent ? left + 128.f : right - 140.f, consentButtonsTop + 36.f));
    _imageReload->SetBounds(D2D1::RectF(right - 132.f, consentButtonsTop, right - 24.f, consentButtonsTop + 36.f));
    _editor->SetBounds(D2D1::RectF(left + gutter, contentTop, (split ? mid : right) - gutter, bottom));
    _mirror->SetVisible(split);
    _mirror->SetBounds(D2D1::RectF(mid + gutter, _imageConsent->IsVisible() ? contentTop : top + splitHeaderH + 20.f, right - gutter, bottom));

    const bool showSplitHeader        = split && ! focus;
    const bool sourceActive           = _splitActivePane == SplitActivePane::Source;
    const D2D1_RECT_F composeChipRect = D2D1::RectF(left + 86.f, top + 5.f, left + 160.f, top + 25.f);
    const D2D1_RECT_F sourceChipRect  = D2D1::RectF(mid + 88.f, top + 5.f, mid + 162.f, top + 25.f);
    _page->splitChrome                = Surface::SplitChromeState{30.f,
                                                                  mid,
                                                                  showSplitHeader ? (sourceActive ? sourceChipRect : composeChipRect) : D2D1_RECT_F{},
                                                                  showSplitHeader,
                                                                  sourceActive,
                                                                  _settings.splitScrollSync};
    if (_splitComposeTitle)
    {
        for (DxUi::Control* control : {static_cast<DxUi::Control*>(_splitComposeTitle),
                                       static_cast<DxUi::Control*>(_splitSourceTitle),
                                       static_cast<DxUi::Control*>(_splitComposeChip),
                                       static_cast<DxUi::Control*>(_splitSourceChip),
                                       static_cast<DxUi::Control*>(_splitScrollSyncButton)})
            control->SetVisible(showSplitHeader);
        if (showSplitHeader)
        {
            _splitComposeTitle->SetBounds(D2D1::RectF(left + 16.f, top + 5.f, left + 82.f, top + 25.f));
            _splitSourceTitle->SetBounds(D2D1::RectF(mid + 24.f, top + 5.f, mid + 84.f, top + 25.f));
            _splitComposeChipBg->SetBounds(composeChipRect);
            _splitComposeChip->SetBounds(composeChipRect);
            _splitSourceChipBg->SetBounds(sourceChipRect);
            _splitSourceChip->SetBounds(sourceChipRect);
            _splitScrollSyncButton->SetBounds(D2D1::RectF(mid - 14.f, top + 3.f, mid + 14.f, top + 27.f));
            const bool showHint = (right - left) * 0.5f >= 340.f;
            _splitSwitchHint->SetVisible(showHint);
            if (showHint)
                _splitSwitchHint->SetBounds(sourceActive ? D2D1::RectF(right - 156.f, top + 5.f, right - 16.f, top + 25.f)
                                                         : D2D1::RectF(mid - 160.f, top + 5.f, mid - 20.f, top + 25.f));
        }
        else
        {
            _splitComposeChipBg->SetVisible(false);
            _splitSourceChipBg->SetVisible(false);
            _splitSwitchHint->SetVisible(false);
        }
    }
    UpdateSplitHeaders();
    DxUi::RichTextEditorAppearance appearance;
    appearance.markdownPresentation        = DxUi::RichText::MarkdownPresentationStyle::Subtle;
    appearance.strikeCompletedTasks        = _settings.strikeCompletedTasks;
    appearance.drawBorder                  = false;
    appearance.drawFocusIndicator          = false;
    appearance.useAccentLinkColor          = true;
    appearance.showVerticalScrollbar       = true;
    appearance.paddingDip                  = 0;
    appearance.maximumTextWidthDip         = focus ? 680.f : static_cast<float>(_settings.readingWidth);
    appearance.bodyFontSizeDip             = (focus ? 18.f : split ? 15.f : 16.f) * static_cast<float>(_settings.textScale) / 100.f;
    appearance.codeFontSizeDip             = 13.5f * static_cast<float>(_settings.textScale) / 100.f;
    appearance.lineHeightMultiplier        = _settings.view == EditorView::Source ? 1.75f : 1.65f;
    appearance.headingFontSizeDip          = {34.f * static_cast<float>(_settings.textScale) / 100.f,
                                              23.f * static_cast<float>(_settings.textScale) / 100.f,
                                              20.f * static_cast<float>(_settings.textScale) / 100.f,
                                              18.f * static_cast<float>(_settings.textScale) / 100.f,
                                              16.f * static_cast<float>(_settings.textScale) / 100.f,
                                              16.f * static_cast<float>(_settings.textScale) / 100.f};
    appearance.headingLineHeightMultiplier = {1.2f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f};
    appearance.paragraphSpacingAfterLines  = 18.f / 26.4f;
    appearance.listItemSpacingDip          = 10.f;
    appearance.semanticSpacing             = DxUi::RichText::SemanticSpacing{
        .headingBeforeDip = {0.f, 40.f, 32.f, 24.f, 20.f, 20.f}, .headingAfterDip = {0.f, 14.f, 14.f, 14.f, 14.f, 14.f}, .containerIndentDip = 10.f};
    auto sourceAppearance                   = appearance;
    sourceAppearance.bodyFontSizeDip        = appearance.codeFontSizeDip;
    sourceAppearance.colorizeMarkdownSource = true;
    sourceAppearance.lineHeightMultiplier   = 1.75f;
    // The canonical monospace role and its fallback use the same family.
    // Reading that policy needs no host text format or graphics initialization.
    sourceAppearance.bodyFontFamily = DxUi::Typography::GetSpec(DxUi::FontRole::Monospace).familyName;
    if (! AcceptSourceResult(_editor->TrySetAppearance(_settings.view == EditorView::Source ? sourceAppearance : appearance)))
        return;
    _editor->SetFocusModeEnabled(focus && _settings.dimInactiveBlocks);
    if (! AcceptSourceResult(_mirror->TrySetAppearance(sourceAppearance)))
        return;
    if (! AcceptSourceResult(_equation->TrySetAppearance(appearance)))
        return;
    _filesSplitter->SetBounds(D2D1::RectF(0, top, width, bottom));
    _filesSplitter->SetMinimumSecondPane(std::max(48.f, width - 406));
    _filesSplitter->SetPosition(filesWidth);
    const float drawerLeft = width - 380;
    _drawer->SetBounds(D2D1::RectF(drawerLeft, top, width, bottom));
    const float dx             = drawerLeft + 24;
    const float dRight         = width - 24;
    const float fullW          = std::max(40.f, dRight - dx);
    const float helpLeft       = dx + 8.f;
    const float helpW          = std::max(40.f, dRight - helpLeft);
    const float scrollTop      = top + 74.f;
    const float contentOriginY = scrollTop + 12.f;
    _settingsTitle->SetBounds(D2D1::RectF(dx, top + 14.f, width - 48.f, top + 44.f));
    _drawerClose->SetBounds(D2D1::RectF(width - 40.f, top + 14.f, width - 12.f, top + 42.f));
    _drawerImmediate->SetBounds(D2D1::RectF(dx, top + 44.f, dRight, top + 64.f));
    _settingsHeaderDivider->SetBounds(D2D1::RectF(drawerLeft, top + 72.f, width, top + 73.f));
    _settingsScroll->SetBounds(D2D1::RectF(drawerLeft + 8.f, scrollTop, width - 4.f, bottom - 8.f));
    const auto measureLabelHeight = [&](DxUi::Label* label, float maxWidth, float fallbackHeight) -> float
    {
        if (! label || maxWidth <= 1.f || _host.GetTypographyRevision() == 0u)
            return fallbackHeight;
        DxUi::DesiredSize desired;
        const HRESULT hr = DxUi::MeasureControl(*label, {_host}, DxUi::LayoutConstraints::AtMost(maxWidth), desired);
        if (hr == E_OUTOFMEMORY)
            DxUi::Failure::FailAllocation();
        if (FAILED(hr))
            return fallbackHeight;
        return std::max(18.f, std::ceil(desired.sizeDip.height));
    };

    float y = contentOriginY;
    _appearanceTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 34.f;
    std::array<DxUi::ButtonThemePreview, 4> previews{{{},
                                                      {{D2D1::ColorF(0xF0F1F4), D2D1::ColorF(0xF8F8FA), D2D1::ColorF(0xFFFFFF)}},
                                                      {{D2D1::ColorF(0x17191E), D2D1::ColorF(0x1C1F25), D2D1::ColorF(0x22252B)}},
                                                      {{D2D1::ColorF(0xEDE6D8), D2D1::ColorF(0xF3EDDF), D2D1::ColorF(0xFFFAEF)}}}};
    previews[0] = previews[_systemDark ? 2 : 1];
    for (unsigned i = 0; i < 4; ++i)
    {
        _themeButtons[i]->SetBounds(D2D1::RectF(dx + static_cast<float>(i % 2) * 168.f,
                                                y + static_cast<float>(i / 2) * 80.f,
                                                dx + 156 + static_cast<float>(i % 2) * 168.f,
                                                y + 64.f + static_cast<float>(i / 2) * 80.f));
        _themeButtons[i]->SetThemePreview(previews[i]);
        _themeButtons[i]->SetToggleState(static_cast<unsigned>(_settings.appearance) == i);
    }
    y += 168.f;
    _readingWidth->SetBounds(D2D1::RectF(dx, y, dRight, y + 36.f));
    if (! _readingWidth->IsEditing())
        _readingWidth->SetValue(_settings.readingWidth);
    y += 48.f;
    _textScale->SetBounds(D2D1::RectF(dx, y, dRight, y + 36.f));
    if (! _textScale->IsEditing())
        _textScale->SetValue(_settings.textScale);
    y += 52.f;

    _writingTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 34.f;
    _outlineToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _outlineToggle->SetChecked(_settings.showOutline);
    y += 40.f;
    _focusToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _focusToggle->SetChecked(focus);
    y += 40.f;
    _dimToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _dimToggle->SetChecked(_settings.dimInactiveBlocks);
    y += 40.f;
    _strikeTasksToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _strikeTasksToggle->SetChecked(_settings.strikeCompletedTasks);
    y += 36.f;
    const float strikeHelpH = measureLabelHeight(_strikeTasksHelp, helpW, 38.f);
    _strikeTasksHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + strikeHelpH));
    y += strikeHelpH + 12.f;
    _keepUnsavedToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _keepUnsavedToggle->SetChecked(_settings.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume);
    y += 36.f;
    const float keepUnsavedHelpH = measureLabelHeight(_keepUnsavedHelp, helpW, 40.f);
    _keepUnsavedHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + keepUnsavedHelpH));
    y += keepUnsavedHelpH + 16.f;

    const float proofingTop = y;
    _proofingTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 34.f;
    _spellingEnabledToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    y += 36.f;
    const float spellHelpH = measureLabelHeight(_spellingHelp, helpW, 40.f);
    _spellingHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + spellHelpH));
    y += spellHelpH + 12.f;
    _spellingLanguageLabel->SetBounds(D2D1::RectF(dx, y, dRight, y + 22.f));
    y += 24.f;
    _spellingLanguageCombo->SetBounds(D2D1::RectF(dx, y, dRight, y + 36.f));
    y += 48.f;
    _spellingIgnoreUpperToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    y += 40.f;
    _spellingIgnoreDigitsToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    y += 40.f;
    _spellingIgnorePathsToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    y += 44.f;
    _spellingDictPathLabel->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 30.f;
    _spellingDictChooseButton->SetBounds(D2D1::RectF(dx, y, dx + 156, y + 36.f));
    _spellingDictResetButton->SetBounds(D2D1::RectF(dx + 168, y, dRight, y + 36.f));
    y += 56.f;
    RefreshProofingDrawerControls();

    _syntaxTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 32.f;
    const float syntaxIntroH = measureLabelHeight(_syntaxIntro, fullW, 40.f);
    _syntaxIntro->SetBounds(D2D1::RectF(dx, y, dRight, y + syntaxIntroH));
    y += syntaxIntroH + 12.f;
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
    {
        _syntaxToggles[index]->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
        _syntaxToggles[index]->SetChecked(_settings.markdownSyntax[index]);
        y += 36.f;
        const float rowHelpH = measureLabelHeight(_syntaxHelp[index], helpW, 40.f);
        _syntaxHelp[index]->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + rowHelpH));
        y += rowHelpH + 12.f;
    }
    y += 8.f;

    const float privacyTop = y;
    _privacyTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 34.f;
    _remoteImagesToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _remoteImagesToggle->SetChecked(_settings.allowRemoteImages);
    y += 36.f;
    const float drawerHelpH = measureLabelHeight(_drawerHelp, helpW, 40.f);
    _drawerHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + drawerHelpH));
    y += drawerHelpH + 12.f;
    _localNetworkImagesToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _localNetworkImagesToggle->SetChecked(_settings.allowLocalNetworkImages);
    y += 36.f;
    const float localNetHelpH = measureLabelHeight(_localNetworkImagesHelp, helpW, 56.f);
    _localNetworkImagesHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + localNetHelpH));
    y += localNetHelpH + 12.f;
    _imageFirstPasteToggle->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    _imageFirstPasteToggle->SetChecked(_settings.imageFirstPaste);
    y += 36.f;
    const float pasteHelpH = measureLabelHeight(_imageFirstPasteHelp, helpW, 56.f);
    _imageFirstPasteHelp->SetBounds(D2D1::RectF(helpLeft, y, dRight, y + pasteHelpH));
    y += pasteHelpH + 14.f;
    _recoveryAction->SetBounds(D2D1::RectF(dx, y, dRight, y + 36.f));
    y += 52.f;

    _accountsTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 30.f;
    const float acctStatusH = measureLabelHeight(_accountsStatusLabel, fullW, 24.f);
    _accountsStatusLabel->SetBounds(D2D1::RectF(dx, y, dRight, y + acctStatusH));
    y += acctStatusH + 10.f;
    const float halfBtnW = (fullW - 8.f) * 0.5f;
    _accountsDeviceBtn->SetBounds(D2D1::RectF(dx, y, dx + halfBtnW, y + 34.f));
    _accountsImportGhBtn->SetBounds(D2D1::RectF(dx + halfBtnW + 8.f, y, dRight, y + 34.f));
    y += 42.f;
    _accountsTokenInput->SetBounds(D2D1::RectF(dx, y, dRight - 104.f, y + 34.f));
    _accountsUseTokenBtn->SetBounds(D2D1::RectF(dRight - 96.f, y, dRight, y + 34.f));
    y += 42.f;
    _accountsSignOutBtn->SetBounds(D2D1::RectF(dx, y, dRight, y + 34.f));
    y += 50.f;
    RefreshAccountsDrawerControls();

    const float aboutTop = y;
    _aboutTitle->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 30.f;
    _aboutVersion->SetBounds(D2D1::RectF(dx, y, dRight, y + 24.f));
    y += 28.f;
    const float aboutSummaryH = measureLabelHeight(_aboutSummary, fullW, 42.f);
    _aboutSummary->SetBounds(D2D1::RectF(dx, y, dRight, y + aboutSummaryH));
    y += aboutSummaryH + 10.f;
    const float shortcutsH = measureLabelHeight(_shortcutsList, fullW, 160.f);
    _shortcutsList->SetBounds(D2D1::RectF(dx, y, dRight, y + shortcutsH));
    y += shortcutsH + 24.f;

    _settingsProofingScrollY = std::max(0.f, proofingTop - contentOriginY);
    _settingsPrivacyScrollY  = std::max(0.f, privacyTop - contentOriginY);
    _settingsAboutScrollY    = std::max(0.f, aboutTop - contentOriginY);
    _settingsScroll->SetContentHeight(std::ceil(y - scrollTop));
    const float findAreaLeft  = split && ! focus && _splitActivePane == SplitActivePane::Source ? mid : left;
    const float findAreaRight = split && ! focus && _splitActivePane == SplitActivePane::Compose ? mid : right;
    const float findAvailW    = std::max(300.f, findAreaRight - findAreaLeft);
    const bool wideFind       = findAvailW >= 660.f;
    const float findBarW      = wideFind ? 628.f : 460.f;
    const float findRight     = std::min(width - 16.f, std::max(16.f + findBarW, findAreaRight - 16.f));
    const float fx            = std::max(16.f, findRight - findBarW);
    const float fy            = top + splitHeaderH + 12.f;
    _findBar->SetBounds(D2D1::RectF(fx, fy, findRight, fy + (wideFind ? 104.f : 148.f)));
    _findText->SetBounds(D2D1::RectF(fx + 12, fy + 10, fx + 204, fy + 46));
    _findCaseToggle->SetToggleState(_find.matchCase);
    _findCaseToggle->SetBounds(wideFind ? D2D1::RectF(fx + 212.f, fy + 10.f, fx + 320.f, fy + 46.f)
                                        : D2D1::RectF(fx + 12.f, fy + 104.f, fx + 146.f, fy + 140.f));
    const float findStatus  = wideFind ? 328.f : 212.f;
    const float findButtons = wideFind ? 478.f : 314.f;
    _findCount->SetBounds(D2D1::RectF(fx + findStatus, fy + 14, fx + findStatus + 98.f, fy + 42));
    for (unsigned i = 0; i < 3; ++i)
        _findBar->GetLogicalChild(i + 2)->SetBounds(
            D2D1::RectF(fx + findButtons + static_cast<float>(i) * 44.f, fy + 10, fx + findButtons + 40.f + static_cast<float>(i) * 44.f, fy + 46));
    _replaceText->SetBounds(D2D1::RectF(fx + 12, fy + 58, fx + 236, fy + 94));
    _findBar->GetLogicalChild(6)->SetBounds(D2D1::RectF(fx + 244, fy + 58, fx + 326, fy + 94));
    _findBar->GetLogicalChild(7)->SetBounds(D2D1::RectF(fx + 334, fy + 58, fx + 448, fy + 94));
    const float draftLeft = std::max(left + 16, (left + right - 680) / 2), draftRight = std::min(right - 16, draftLeft + 680);
    float draftTop = top + 72;
    if (_blockDraftSession)
        _blockDraftSession->pendingPlacement = false;
    if (_activeOverlay == OverlayKind::BlockDraftModal && _blockDraftSession)
    {
        const auto& range        = _blockDraftSession->range;
        auto* const activeEditor = ActiveEditor();
        if (const auto rects = activeEditor->TryGetTextInputRangeRects(_host, range.first, range.last); rects && ! rects->empty())
            draftTop = rects->back().bottom + 8;
        else if (const auto caret = activeEditor->TryGetTextInputCaretRect(_host, range.first))
            draftTop = caret->bottom + 8; // Empty/hidden-only source still has a prepared insertion site.
        else
            _blockDraftSession->pendingPlacement = true;
    }
    draftTop = std::clamp(draftTop, top + 8, std::max(top + 8, bottom - 244));
    _blockDraft->SetBounds(D2D1::RectF(draftLeft, draftTop, draftRight, draftTop + 232));
    _blockDraft->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 8, draftRight - 16, draftTop + 36));
    _blockDraft->GetLogicalChild(1)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 40, draftRight - 16, draftTop + 64));
    _blockDraftText->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 72, draftRight - 16, draftTop + 172));
    _blockDraft->GetLogicalChild(3)->SetBounds(D2D1::RectF(draftRight - 244, draftTop + 184, draftRight - 112, draftTop + 220));
    _blockDraft->GetLogicalChild(4)->SetBounds(D2D1::RectF(draftRight - 100, draftTop + 184, draftRight - 16, draftTop + 220));
    _recoveryPanel->SetBounds(D2D1::RectF(draftLeft, top + 8, draftRight, bottom - 8));
    _recoveryPanel->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 20, top + 24, draftRight - 20, top + 60));
    _recoveryDescription->SetBounds(D2D1::RectF(draftLeft + 20, top + 68, draftRight - 20, top + 128));
    const bool multipleRecovery = _restoreAllRecovery->IsVisible();
    _recoveryPreview->SetBounds(D2D1::RectF(draftLeft + 20, top + 136, draftRight - 20, bottom - (multipleRecovery ? 124.f : 80.f)));
    if (multipleRecovery)
    {
        const float buttonWidth = (draftRight - draftLeft - 48.f) / 2.f;
        for (unsigned i = 0; i < 4; ++i)
        {
            const float buttonLeft = draftLeft + 20 + static_cast<float>(i % 2) * (buttonWidth + 8.f);
            const float buttonTop  = bottom - 106 + static_cast<float>(i / 2) * 44.f;
            _recoveryPanel->GetLogicalChild(i + 3)->SetBounds(D2D1::RectF(buttonLeft, buttonTop, buttonLeft + buttonWidth, buttonTop + 34));
        }
    }
    else
        for (unsigned i = 0; i < 3; ++i)
            _recoveryPanel->GetLogicalChild(i + 3)->SetBounds(
                D2D1::RectF(draftLeft + 20 + static_cast<float>(i) * 208.f, bottom - 62, draftLeft + 220 + static_cast<float>(i) * 208.f, bottom - 28));
    LayoutOverlayPanels(left, right, top, bottom, width, height);
    const float spellW     = std::clamp(width * 0.17f, 148.f, 212.f);
    const float statsRight = std::max(240.f, (width - spellW) * 0.5f - 8.f);
    _stats->SetBounds(D2D1::RectF(16.f, height - 26.f, statsRight, height - 2.f));
    if (_spellStatusCombo)
        _spellStatusCombo->SetBounds(D2D1::RectF(statsRight + 8.f, height - 27.f, statsRight + 8.f + spellW, height - 1.f));
    _positionStatus->SetBounds(D2D1::RectF(statsRight + 16.f + spellW, height - 26.f, width - 16.f, height - 2.f));
    _focusPill->SetBounds(focus ? D2D1::RectF(width / 2 - 224, height - 50, width / 2 + 224, height - 10) : D2D1::RectF(0, height - 28, width / 2, height));
    _focusPill->kind = focus ? Surface::Kind::Floating : Surface::Kind::Chrome;
    if (focus)
        _stats->SetBounds(D2D1::RectF(width / 2 - 208, height - 44, width / 2 + 208, height - 16));
    UpdateCounts();
    _title->SetTextColor(focus ? std::optional(_host.GetTheme().subduedText) : std::nullopt);
    _stats->SetVisible(true);
    _positionStatus->SetVisible(! focus);
    if (_spellStatusCombo)
        _spellStatusCombo->SetVisible(! focus);
    _feedback->SetBounds(D2D1::RectF(left + 16, bottom - 22, right - 16, bottom));
    // Reposition only an already visible toolbar: resize must not reopen
    // tools that Escape or another contextual surface deliberately dismissed.
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SelectionChanged();
    _host.Invalidate();
}

void Application::LayoutOverlayPanels(float left, float right, float top, float bottom, float width, float height)
{
    if (_modalBackdrop)
        _modalBackdrop->SetBounds(D2D1::RectF(0.f, 0.f, width, height));

    const float modalAreaW = std::max(380.f, std::min(width - 32.f, right - left + 220.f));
    if (_saveConfirmPanel)
    {
        const float dialogW        = std::clamp(modalAreaW, 380.f, 640.f);
        const std::size_t rowCount = _saveConfirmSession ? _saveConfirmSession->items.size() : 0u;
        const bool multiDoc        = rowCount > 1u;
        const float listH          = std::clamp(static_cast<float>(std::max<std::size_t>(1u, rowCount)) * 48.f, 52.f, 224.f);
        const float dialogH        = std::min(height - 48.f, (multiDoc ? 230.f : 196.f) + listH);
        const float x0             = std::max(16.f, (width - dialogW) * 0.5f);
        const float y0             = std::max(top + 8.f, (top + bottom - dialogH) * 0.5f);
        const float x1             = x0 + dialogW;
        const float y1             = y0 + dialogH;
        _saveConfirmPanel->SetBounds(D2D1::RectF(x0, y0, x1, y1));
        _saveConfirmTitle->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 18.f, x1 - 20.f, y0 + 48.f));
        _saveConfirmHint->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 50.f, x1 - 20.f, y0 + 88.f));
        float listTop = y0 + 92.f;
        if (multiDoc)
        {
            _saveConfirmSelectAll->SetVisible(true);
            _saveConfirmSelectAll->SetBounds(D2D1::RectF(x0 + 20.f, listTop, x1 - 20.f, listTop + 28.f));
            listTop += 34.f;
        }
        else
        {
            _saveConfirmSelectAll->SetVisible(false);
        }
        const float buttonTop  = y1 - 54.f;
        const float listBottom = std::max(listTop + 48.f, buttonTop - 14.f);
        _saveConfirmScroll->SetBounds(D2D1::RectF(x0 + 16.f, listTop, x1 - 16.f, listBottom));
        const float innerW = std::max(280.f, dialogW - 44.f);
        float ry           = listTop + 4.f;
        for (std::size_t i = 0; i < _saveConfirmRowControls.size(); ++i)
        {
            const auto& rowControls = _saveConfirmRowControls[i];
            const bool activeRow    = i < rowCount;
            rowControls.checkbox->SetVisible(activeRow);
            rowControls.locationLabel->SetVisible(activeRow);
            if (! activeRow)
                continue;
            const auto& item             = _saveConfirmSession->items[i];
            const std::wstring nameLabel = item.location.fileName;
            const std::wstring locText = item.location.relativePath.empty()
                                             ? item.location.sectionLabel
                                             : FormatStringResource(nullptr, IDS_SAVE_LOCATION_FORMAT, item.location.sectionLabel, item.location.relativePath);
            rowControls.checkbox->SetText(nameLabel);
            rowControls.checkbox->SetTooltipText(item.location.fullPath);
            rowControls.locationLabel->SetText(locText);
            rowControls.locationLabel->SetTooltipText(item.location.fullPath);
            rowControls.checkbox->SetBounds(D2D1::RectF(x0 + 22.f, ry, x0 + 22.f + innerW, ry + 24.f));
            rowControls.locationLabel->SetBounds(D2D1::RectF(x0 + 48.f, ry + 24.f, x0 + 22.f + innerW, ry + 42.f));
            ry += 48.f;
        }
        _saveConfirmScroll->SetContentHeight(std::max(0.f, ry - listTop));

        const float gap  = 8.f;
        const float btnW = (dialogW - 40.f - gap * 3.f) * 0.25f;
        float bx         = x0 + 20.f;
        for (auto* btn : {_saveConfirmPrimary, _saveConfirmKeepExit, _saveConfirmDiscard, _saveConfirmCancel})
        {
            btn->SetBounds(D2D1::RectF(bx, buttonTop, bx + btnW, buttonTop + 34.f));
            bx += btnW + gap;
        }
    }

    if (_renamePanel)
    {
        const float dialogW = std::clamp(width - 48.f, 360.f, 520.f);
        const float dialogH = 224.f;
        const float x0      = std::max(16.f, (width - dialogW) * 0.5f);
        const float y0      = std::max(top + 12.f, (top + bottom - dialogH) * 0.5f);
        const float x1      = x0 + dialogW;
        const float y1      = y0 + dialogH;
        _renamePanel->SetBounds(D2D1::RectF(x0, y0, x1, y1));
        _renameTitle->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 18.f, x1 - 20.f, y0 + 48.f));
        _renameLocation->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 50.f, x1 - 20.f, y0 + 82.f));
        _renameInput->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 88.f, x1 - 20.f, y0 + 124.f));
        _renameError->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 130.f, x1 - 20.f, y0 + 162.f));
        _renamePanel->GetLogicalChild(4)->SetBounds(D2D1::RectF(x1 - 228.f, y1 - 52.f, x1 - 124.f, y1 - 18.f));
        _renamePanel->GetLogicalChild(5)->SetBounds(D2D1::RectF(x1 - 116.f, y1 - 52.f, x1 - 20.f, y1 - 18.f));
    }

    if (_fileConflictPanel)
    {
        const float dialogW = std::clamp(width - 48.f, 360.f, 500.f);
        const float dialogH = 196.f;
        const float x0      = std::max(16.f, (width - dialogW) * 0.5f);
        const float y0      = std::max(top + 12.f, (top + bottom - dialogH) * 0.5f);
        const float x1      = x0 + dialogW;
        const float y1      = y0 + dialogH;
        _fileConflictPanel->SetBounds(D2D1::RectF(x0, y0, x1, y1));
        _fileConflictTitle->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 18.f, x1 - 20.f, y0 + 48.f));
        _fileConflictDesc->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 54.f, x1 - 20.f, y0 + 120.f));
        const float btnW = (dialogW - 56.f) / 3.f;
        _conflictReplaceBtn->SetBounds(D2D1::RectF(x0 + 20.f, y1 - 52.f, x0 + 20.f + btnW, y1 - 18.f));
        _conflictKeepBothBtn->SetBounds(D2D1::RectF(x0 + 28.f + btnW, y1 - 52.f, x0 + 28.f + btnW * 2.f, y1 - 18.f));
        _fileConflictPanel->GetLogicalChild(4)->SetBounds(D2D1::RectF(x0 + 36.f + btnW * 2.f, y1 - 52.f, x1 - 20.f, y1 - 18.f));
    }

    if (_openGitHubPanel)
    {
        const float dialogW = std::clamp(width - 32.f, 480.f, 760.f);
        const float dialogH = std::clamp(height - 40.f, 440.f, 560.f);
        const float x0      = std::max(16.f, (width - dialogW) * 0.5f);
        const float y0      = std::max(top + 8.f, (top + bottom - dialogH) * 0.5f);
        const float x1      = x0 + dialogW;
        const float y1      = y0 + dialogH;
        const float innerW  = dialogW - 40.f;
        _openGitHubPanel->SetBounds(D2D1::RectF(x0, y0, x1, y1));
        _openGitHubTitle->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 18.f, x1 - 20.f, y0 + 48.f));
        _openGitHubAuthLabel->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 50.f, x1 - 20.f, y0 + 72.f));
        const float tokenAreaW = std::min(300.f, innerW * 0.44f);
        const float authBtnW   = std::max(120.f, (innerW - tokenAreaW - 16.f) * 0.5f);
        _openGitHubDeviceBtn->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 76.f, x0 + 20.f + authBtnW, y0 + 108.f));
        _openGitHubImportGhBtn->SetBounds(D2D1::RectF(x0 + 28.f + authBtnW, y0 + 76.f, x0 + 28.f + authBtnW * 2.f, y0 + 108.f));
        _openGitHubTokenInput->SetBounds(D2D1::RectF(x1 - 20.f - tokenAreaW, y0 + 76.f, x1 - 122.f, y0 + 108.f));
        _openGitHubUseTokenBtn->SetBounds(D2D1::RectF(x1 - 114.f, y0 + 76.f, x1 - 20.f, y0 + 108.f));
        _openGitHubUrlInput->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 118.f, x1 - 244.f, y0 + 154.f));
        _openGitHubBranchInput->SetBounds(D2D1::RectF(x1 - 236.f, y0 + 118.f, x1 - 20.f, y0 + 154.f));
        _openGitHubSuggestLabel->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 162.f, x1 - 20.f, y0 + 184.f));
        _githubSuggestTree->SetBounds(D2D1::RectF(x0 + 20.f, y0 + 186.f, x1 - 20.f, y1 - 86.f));
        _openGitHubErrorLabel->SetBounds(D2D1::RectF(x0 + 20.f, y1 - 80.f, x1 - 240.f, y1 - 18.f));
        _openGitHubOpenBtn->SetBounds(D2D1::RectF(x1 - 228.f, y1 - 52.f, x1 - 122.f, y1 - 18.f));
        _openGitHubPanel->GetLogicalChild(12)->SetBounds(D2D1::RectF(x1 - 114.f, y1 - 52.f, x1 - 20.f, y1 - 18.f));
    }
}

void Application::ApplySidePaneOverlayBounds()
{
    if (! _root || ! _leftRail || ! _rightRail || ! _sidebar || ! _filesPane)
        return;
    const auto bounds      = _host.GetClientBoundsDip();
    const float width      = bounds.right;
    const float height     = bounds.bottom;
    const bool focus       = _settings.focusMode;
    const float top        = focus ? 40.f : 104.f;
    const float bottom     = std::max(top + 120.f, height - 28.f);
    const float filesWidth = static_cast<float>(_settings.filesWidth);
    const auto& leftState  = _leftRail->State();
    const auto& rightState = _rightRail->State();
    const auto layoutReq   = _activity.layouts;

    const bool rightOverlay      = rightState.autoCollapsed || rightState.phase == PaneAnimationPhase::Collapsing;
    const bool showSidebar       = ! focus && (rightState.mode == PaneDisplayMode::Docked || rightState.mode == PaneDisplayMode::HoverOverlay ||
                                               rightState.phase == PaneAnimationPhase::Expanding || rightState.phase == PaneAnimationPhase::Collapsing);
    const bool sidebarWasVisible = _sidebar->IsVisible();
    if (showSidebar && ! sidebarWasVisible && ! PublishCurrentBlockLabelIfVisible(true))
        return;
    if (_activity.layouts != layoutReq)
        return;
    _sidebar->SetVisible(showSidebar);
    if (_activity.layouts != layoutReq)
        return;
    const float rightProgress = (rightState.mode == PaneDisplayMode::Docked && rightState.phase == PaneAnimationPhase::Settled) ? 1.f : rightState.progress;
    const float sx            = width - 240.f * rightProgress;
    _sidebar->kind            = rightOverlay ? Surface::Kind::DrawerRight : Surface::Kind::Sidebar;
    _sidebar->borderLeft      = true;
    _sidebar->SetBounds(D2D1::RectF(sx, top, sx + 240.f, bottom));
    const float side   = sx + 16.f;
    const float sRight = sx + 240.f;
    _outlineTitle->SetBounds(D2D1::RectF(side + 4.f, top + 16.f, sx + 84.f, top + 40.f));
    _outlineCount->SetBounds(D2D1::RectF(sx + 88.f, top + 16.f, sRight - 44.f, top + 40.f));
    const bool rightDockedUi = rightState.mode == PaneDisplayMode::Docked && rightState.phase != PaneAnimationPhase::Collapsing;
    if (_pinOutline)
    {
        _pinOutline->SetBounds(D2D1::RectF(sRight - 40.f, top + 14.f, sRight - 12.f, top + 42.f));
        _pinOutline->SetText(rightDockedUi ? L"\xE840" : L"\xE718");
        _pinOutline->SetToggleState(rightDockedUi);
        const auto pinOutlineText = Resource(rightDockedUi ? IDS_UNPIN_OUTLINE : IDS_PIN_OUTLINE);
        _pinOutline->SetTooltipText(pinOutlineText);
        _pinOutline->SetAccessibleName(pinOutlineText);
    }
    const float contextHeight = _equationContext ? 260.f : 128.f;
    const float outlineHeight = std::max(40.f, static_cast<float>(_outlineModel.rows.size()) * 42.f + 4.f);
    const float outlineBottom = std::min(top + 44.f + outlineHeight, std::max(top + 92.f, bottom - contextHeight));
    _outlineTree->SetBounds(D2D1::RectF(side - 4.f, top + 44.f, sRight - 8.f, outlineBottom));
    _contextDivider->SetBounds(D2D1::RectF(side + 4.f, outlineBottom + 12.f, sRight - 12.f, outlineBottom + 13.f));
    _contextDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    _currentBlockTitle->SetBounds(D2D1::RectF(side + 4.f, outlineBottom + 28.f, sRight - 16.f, outlineBottom + 48.f));
    _currentBlock->SetBounds(D2D1::RectF(side + 4.f, outlineBottom + 52.f, sRight - 16.f, outlineBottom + 76.f));
    if (showSidebar && ! PublishCurrentBlockLabelIfVisible())
        return;
    if (_activity.layouts != layoutReq)
        return;
    _listSummaryIcon->SetBounds(D2D1::RectF(side + 4.f, outlineBottom + 52.f, side + 20.f, outlineBottom + 76.f));
    _listSummary->SetBounds(D2D1::RectF(side + 28.f, outlineBottom + 52.f, sRight - 16.f, outlineBottom + 76.f));
    _blockSourceAction->SetBounds(D2D1::RectF(side + 4.f, outlineBottom + 84.f, sRight - 20.f, outlineBottom + 116.f));
    _equationHelp->SetVisible(_equationContext);
    _equation->SetVisible(_equationContext);
    _equationReadOnly->SetVisible(_equationContext);
    _equationHelp->SetBounds(D2D1::RectF(side, outlineBottom + 132.f, sRight - 100.f, outlineBottom + 156.f));
    _equationReadOnly->SetBounds(D2D1::RectF(sRight - 96.f, outlineBottom + 132.f, sRight - 16.f, outlineBottom + 156.f));
    _equation->SetBounds(D2D1::RectF(side, outlineBottom + 164.f, sRight - 16.f, bottom - 16.f));

    const bool leftOverlay   = leftState.autoCollapsed || leftState.phase == PaneAnimationPhase::Collapsing;
    const bool showFiles     = ! focus && (leftState.mode == PaneDisplayMode::Docked || leftState.mode == PaneDisplayMode::HoverOverlay ||
                                           leftState.phase == PaneAnimationPhase::Expanding || leftState.phase == PaneAnimationPhase::Collapsing);
    const bool showLeftRail  = ! focus && (leftState.autoCollapsed || leftState.mode == PaneDisplayMode::CollapsedRail ||
                                           leftState.mode == PaneDisplayMode::HoverOverlay || leftState.phase == PaneAnimationPhase::Collapsing);
    const bool showRightRail = ! focus && (rightState.autoCollapsed || rightState.mode == PaneDisplayMode::CollapsedRail ||
                                           rightState.mode == PaneDisplayMode::HoverOverlay || rightState.phase == PaneAnimationPhase::Collapsing);
    _leftRail->SetVisible(showLeftRail);
    _rightRail->SetVisible(showRightRail);
    _filesPane->SetVisible(showFiles);
    const float leftProgress = (leftState.mode == PaneDisplayMode::Docked && leftState.phase == PaneAnimationPhase::Settled) ? 1.f : leftState.progress;
    const float fxPane       = -filesWidth * (1.f - leftProgress);
    const float fRight       = fxPane + filesWidth;
    _filesPane->kind         = leftOverlay ? Surface::Kind::DrawerLeft : Surface::Kind::Sidebar;
    _filesPane->borderRight  = true;
    _filesPane->SetBounds(D2D1::RectF(fxPane, top, fRight, bottom));
    _filesSectionTitle->SetBounds(D2D1::RectF(fxPane + 20.f, top + 16.f, std::max(fxPane + 72.f, fRight - 200.f), top + 40.f));
    if (_filesAddFolder)
        _filesAddFolder->SetBounds(D2D1::RectF(fRight - 196.f, top + 14.f, fRight - 122.f, top + 42.f));
    if (_filesAddGitHub)
        _filesAddGitHub->SetBounds(D2D1::RectF(fRight - 118.f, top + 14.f, fRight - 44.f, top + 42.f));
    const bool leftDockedUi = leftState.mode == PaneDisplayMode::Docked && leftState.phase != PaneAnimationPhase::Collapsing;
    if (_pinFiles)
    {
        _pinFiles->SetBounds(D2D1::RectF(fRight - 40.f, top + 14.f, fRight - 12.f, top + 42.f));
        _pinFiles->SetText(leftDockedUi ? L"\xE840" : L"\xE718");
        _pinFiles->SetToggleState(leftDockedUi);
        const auto pinFilesText = Resource(leftDockedUi ? IDS_UNPIN_FILES : IDS_PIN_FILES);
        _pinFiles->SetTooltipText(pinFilesText);
        _pinFiles->SetAccessibleName(pinFilesText);
    }
    _filesRootIcon->SetBounds(D2D1::RectF(fxPane + 20.f, top + 52.f, fxPane + 36.f, top + 78.f));
    _filesTitle->SetBounds(D2D1::RectF(fxPane + 44.f, top + 52.f, fRight - 16.f, top + 78.f));
    _filesFilter->SetBounds(D2D1::RectF(fxPane + 20.f, top + 84.f, fRight - 12.f, top + 114.f));
    const float footerWidth   = filesWidth - 24.f;
    const auto footerRevision = _filesCount->GetMeasureRevision(), typographyRevision = _host.GetTypographyRevision();
    const auto footerDpi = _host.GetDpi();
    // Keep hidden measurement keys stale; the first visible layout measures
    // current text/width/typography/DPI before reserving the footer's bounds.
    if (_filesPane->IsVisible() && (_filesFooterMeasure.widthDip != footerWidth || _filesFooterMeasure.textRevision != footerRevision ||
                                    _filesFooterMeasure.typographyRevision != typographyRevision || _filesFooterMeasure.dpi != footerDpi))
    {
        DxUi::DesiredSize desired;
        const HRESULT measured = DxUi::MeasureControl(*_filesCount, {_host}, DxUi::LayoutConstraints::AtMost(footerWidth), desired);
        if (measured == E_OUTOFMEMORY)
            DxUi::Failure::FailAllocation();
        THROW_IF_FAILED(measured);
        _filesFooterMeasure.widthDip           = footerWidth;
        _filesFooterMeasure.heightDip          = std::max(24.f, std::ceil(desired.sizeDip.height));
        _filesFooterMeasure.textRevision       = footerRevision;
        _filesFooterMeasure.typographyRevision = _host.GetTypographyRevision();
        _filesFooterMeasure.dpi                = footerDpi;
        ++_filesFooterMeasure.measurements;
    }
    const float footerTop = bottom - 8.f - _filesFooterMeasure.heightDip;
    _filesFooterDivider->SetBounds(D2D1::RectF(fxPane, footerTop - 8.f, fRight, footerTop - 7.f));
    _filesTree->SetBounds(D2D1::RectF(fxPane + 4.f, top + 122.f, fRight - 4.f, footerTop - 4.f));
    const bool showEmptyActions =
        showFiles && _settings.filesRoots.empty() && _connectedGitHubRepos.empty() && _filesModel.rows.empty() && (footerTop - (top + 122.f)) > 160.f;
    if (_filesEmptyOpenFolder && _filesEmptyOpenGitHub)
    {
        _filesEmptyOpenFolder->SetVisible(showEmptyActions);
        _filesEmptyOpenGitHub->SetVisible(showEmptyActions);
        if (showEmptyActions)
        {
            const float btnW = std::max(140.f, filesWidth - 48.f);
            const float bx   = fxPane + (filesWidth - btnW) * 0.5f;
            const float by   = std::min(footerTop - 92.f, top + 210.f);
            _filesEmptyOpenFolder->SetBounds(D2D1::RectF(bx, by, bx + btnW, by + 34.f));
            _filesEmptyOpenGitHub->SetBounds(D2D1::RectF(bx, by + 42.f, bx + btnW, by + 76.f));
        }
    }
    _filesCount->SetBounds(D2D1::RectF(fxPane + 12.f, footerTop, fRight - 12.f, bottom - 8.f));
    _filesSplitter->SetVisible(! focus && _settings.showFiles && ! leftState.autoCollapsed && leftState.mode == PaneDisplayMode::Docked &&
                               leftState.phase == PaneAnimationPhase::Settled);
    _host.Invalidate();
}

void Application::UpdateSplitHeaders()
{
    if (! _page || ! _editor || ! _mirror)
        return;
    const bool split        = _settings.view == EditorView::Split;
    const bool sourceActive = split && _splitActivePane == SplitActivePane::Source;
    const auto& theme       = _host.GetTheme();
    _editor->SetAccessibleName(Resource(static_cast<unsigned>(! split ? IDS_DOC : sourceActive ? IDS_SPLIT_COMPOSE_PREVIEW : IDS_SPLIT_COMPOSE_EDITING)));
    _mirror->SetAccessibleName(Resource(static_cast<unsigned>(! split ? IDS_MIRROR : sourceActive ? IDS_SPLIT_SOURCE_EDITING : IDS_SPLIT_SOURCE_MIRROR)));
    if (! _splitComposeTitle)
        return;
    const bool showHeader = split && ! _settings.focusMode;
    _splitComposeTitle->SetTextColor(sourceActive ? std::optional(theme.subduedText) : std::nullopt);
    _splitSourceTitle->SetTextColor(sourceActive ? std::nullopt : std::optional(theme.subduedText));
    _splitComposeChip->SetText(Resource(sourceActive ? IDS_READONLYSHORT : IDS_SPLIT_EDITING_CHIP));
    _splitComposeChip->SetTextColor(sourceActive ? std::optional(theme.subduedText) : std::optional(theme.selectionText));
    _splitSourceChip->SetText(Resource(sourceActive ? IDS_SPLIT_EDITING_CHIP : IDS_READONLYSHORT));
    _splitSourceChip->SetTextColor(sourceActive ? std::optional(theme.selectionText) : std::optional(theme.subduedText));
    _splitComposeChipBg->SetVisible(showHeader && sourceActive);
    _splitSourceChipBg->SetVisible(showHeader && ! sourceActive);
    _splitSwitchHint->SetTextColor(theme.subduedText);
    _splitScrollSyncButton->SetToggleState(_settings.splitScrollSync);
    _splitScrollSyncButton->SetTooltipText(Resource(_settings.splitScrollSync ? IDS_TIP_SPLIT_SCROLL_ON : IDS_TIP_SPLIT_SCROLL_OFF));
    _page->splitChrome.sourceActive = sourceActive;
    _page->splitChrome.linked       = _settings.splitScrollSync;
}

void Application::DismissHoverOverlays(bool animated)
{
    auto dismiss = [&](CollapsedPaneRail* rail)
    {
        if (! rail)
            return;
        const auto& state = rail->State();
        rail->CancelHoverDwell();
        if (state.mode == PaneDisplayMode::HoverOverlay || state.phase == PaneAnimationPhase::Expanding || state.phase == PaneAnimationPhase::LeaveGrace)
        {
            if (animated && ! _host.GetTheme().reducedMotion)
                rail->StartCollapse(_host, true);
            else
                rail->SnapToMode(PaneDisplayMode::CollapsedRail, true);
        }
    };
    dismiss(_leftRail);
    dismiss(_rightRail);
    ApplySidePaneOverlayBounds();
    if (! _settings.showFiles && ! IsPaneOverlayActive(PaneSide::Left) && _folderFiles.IsActive())
        RefreshFiles();
}

bool Application::IsPaneOverlayActive(PaneSide side) const noexcept
{
    const auto* const rail = side == PaneSide::Left ? _leftRail : _rightRail;
    if (! rail)
        return false;
    const auto& state = rail->State();
    return state.phase != PaneAnimationPhase::Collapsing &&
           (state.mode == PaneDisplayMode::HoverOverlay ||
            (state.autoCollapsed && (state.phase == PaneAnimationPhase::Expanding || state.phase == PaneAnimationPhase::LeaveGrace)));
}

void Application::UpdateHoverOverlayPointer(D2D1_POINT_2F clientDip, bool insideWindow)
{
    _lastPointerClientDip = clientDip;
    _pointerInsideClient  = insideWindow;
    if (HasBlockDraft() || HasActiveModalPrompt() || _activeOverlay == OverlayKind::RecoveryModal || _activeOverlay == OverlayKind::SettingsDrawer ||
        _activeOverlay == OverlayKind::BlockDraftModal || _activeOverlay == OverlayKind::SaveConfirmModal || _activeOverlay == OverlayKind::RenameFileModal ||
        _activeOverlay == OverlayKind::FileConflictModal || _activeOverlay == OverlayKind::OpenGitHubModal || _printPreview != nullptr)
    {
        if (_leftRail)
            _leftRail->CancelHoverDwell();
        if (_rightRail)
            _rightRail->CancelHoverDwell();
        return;
    }
    const auto contains = [](const D2D1_RECT_F& rect, D2D1_POINT_2F point) noexcept
    { return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom; };
    auto update = [&](CollapsedPaneRail* rail, Surface* pane)
    {
        if (! rail || ! rail->IsVisible() || ! pane)
            return;
        const bool overlayOpen = rail->State().mode == PaneDisplayMode::HoverOverlay;
        const bool focusInside = overlayOpen && pane->IsVisible() && ContainsControl(pane, _host.GetFocusControl());
        const bool menuOpen    = rail == _leftRail && _filesContextMenuOpen;
        const bool pointerInside =
            insideWindow && (contains(rail->GetBounds(), clientDip) ||
                             (pane->IsVisible() && (rail->State().autoCollapsed || rail->State().phase == PaneAnimationPhase::Collapsing) &&
                              contains(pane->GetBounds(), clientDip)));
        rail->HandlePointerMoveDip(_host, clientDip, pointerInside || focusInside || menuOpen);
    };
    update(_leftRail, _filesPane);
    update(_rightRail, _sidebar);
}

void Application::OnRailDotActivated(PaneSide side, std::size_t itemIndex)
{
    if (HasBlockDraft() || HasActiveModalPrompt() || _files.Busy() || _switchingDocument || IsClosing())
        return;
    DismissHoverOverlays(false);
    if (side == PaneSide::Right)
    {
        if (itemIndex >= _documentInfo.headings.size())
            return;
        const auto offset  = _documentInfo.headings[itemIndex].offset;
        auto* const active = ActiveEditor();
        if (! AcceptSourceResult(active->TrySetSelection({offset, offset})))
            return;
        (void)active->ScrollSourceRangeIntoView(_host, offset, offset, true);
        _host.SetFocusControl(active, MayRequestNativeFocus());
        SelectionChanged();
        return;
    }
    const auto activeDirtyKey = itemIndex;
    if (activeDirtyKey == _session.key)
    {
        _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
        return;
    }
    for (std::size_t i = 0; i < _inactiveDocuments.size(); ++i)
    {
        if (_inactiveDocuments[i].metadata.key == activeDirtyKey)
        {
            const bool previous          = _workspaceSelectionPreferred;
            _workspaceSelectionPreferred = true;
            if (! SwitchWorkspaceDocument(i))
            {
                _workspaceSelectionPreferred = previous;
                if (IsUiAttached())
                    RefreshFiles();
            }
            return;
        }
    }
}

void Application::RevealPaneOverlay(PaneSide side)
{
    auto* const rail = side == PaneSide::Left ? _leftRail : _rightRail;
    auto* const peer = side == PaneSide::Left ? _rightRail : _leftRail;
    if (! rail || ! rail->IsVisible() || rail->State().mode == PaneDisplayMode::Docked || _settings.focusMode)
        return;
    if (peer && (peer->State().mode == PaneDisplayMode::HoverOverlay || peer->State().phase == PaneAnimationPhase::Expanding))
    {
        peer->StartCollapse(_host, true);
        if (side == PaneSide::Right && ! _settings.showFiles && _folderFiles.IsActive())
            RefreshFiles();
    }
    rail->StartExpand(_host, true);
    if (side == PaneSide::Left && ! _settings.showFiles && (! _settings.filesRoots.empty() || ! _connectedGitHubRepos.empty()))
        RefreshFiles(false);
    ApplySidePaneOverlayBounds();
}
} // namespace RedTilde
