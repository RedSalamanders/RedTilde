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
        const auto color = kind == Kind::Canvas ? theme.cardBackground : kind == Kind::Floating ? theme.overlayBackground : theme.surfaceBackground;
        const auto bounds = GetBounds();
        target->FillRectangle(bounds, host.GetSolidBrush(color));
        if (kind == Kind::Floating)
            target->DrawRoundedRectangle(D2D1::RoundedRect(bounds, 8, 8), host.GetSolidBrush(theme.borderDefault));
        DxUi::Panel::Paint(host);
    }
    Kind kind;
};

class Editor final : public DxUi::RichTextEditor
{
  public:
    using DxUi::RichTextEditor::RichTextEditor;
    std::function<void()> changedSelection;
    void Paint(DxUi::ControlHost &host) const override
    {
        DxUi::RichTextEditor::Paint(host);
        // Keep a visible focus ring for keyboard navigation. Pointer-led writing
        // uses the caret, so the page need not look like a form input.
        if (!host.IsKeyboardFocusVisible() && !host.GetTheme().highContrast)
            if (auto *target = host.GetDeviceContext())
                target->DrawRoundedRectangle(D2D1::RoundedRect(GetBounds(), 4, 4), host.GetSolidBrush(host.GetTheme().cardBackground), 2.0f);
    }
    bool OnMouseUp(DxUi::ControlHost &host, D2D1_POINT_2F point, bool right, UINT modifiers) noexcept override
    {
        const bool result = DxUi::RichTextEditor::OnMouseUp(host, point, right, modifiers);
        Notify();
        return result;
    }
    bool OnKeyDown(DxUi::ControlHost &host, UINT key, UINT modifiers) noexcept override
    {
        const bool result = DxUi::RichTextEditor::OnKeyDown(host, key, modifiers);
        Notify();
        return result;
    }

  private:
    void Notify() noexcept
    {
        // App callbacks do not destroy their sender. Allocation failure is fatal;
        // unexpected application errors end this callback without unwinding Win32.
        try
        {
            if (changedSelection)
                changedSelection();
        }
        catch (const std::bad_alloc &)
        {
            std::terminate();
        }
        catch (const std::exception &)
        {
            OutputDebugStringW(L"RedTilde selection callback failed\n");
        }
    }
};

} // namespace RedTilde
