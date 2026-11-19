#pragma once
#include "Printing.h"
#include "UiControls.h"

namespace RedTilde
{
class Application;

class PrintPreview final
{
  public:
    explicit PrintPreview(Application &app);
    ~PrintPreview();
    PrintPreview(const PrintPreview &) = delete;
    PrintPreview &operator=(const PrintPreview &) = delete;

    [[nodiscard]] HRESULT Initialize(std::wstring_view title);
    void Show();
    [[nodiscard]] HWND Window() const noexcept;
    [[nodiscard]] DxUi::ControlHost &Host() noexcept;
    [[nodiscard]] const Printing::PreviewPage &Page() const noexcept;
    [[nodiscard]] std::optional<std::vector<Printing::PageRange>> SelectedRanges() const;
    [[nodiscard]] bool CanPrint() const noexcept;
    [[nodiscard]] bool DispatchKeyboardMessage(const MSG &message);
    void SetTheme(const DxUi::ThemePalette &theme) noexcept;
    void BeginRendering();
    void Present(Printing::PreviewPage page, std::wstring status);
    void SetStatus(std::wstring status);
    void ReportInvalidRange(bool focus);

  private:
    class Sheet;
    enum class Action : WPARAM
    {
        Print,
        Close
    };
    static constexpr UINT PreviewAction = WM_APP + 1u;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PostAction(Action action) noexcept;
    void Layout();

    Application &app_;
    HWND owner_{};
    wil::unique_hwnd window_;
    DxUi::ControlHost host_;
    Surface *root_{};
    DxUi::ScrollPanel *scroll_{};
    Sheet *sheet_{};
    DxUi::Label *title_{}, *document_{}, *pageLabel_{}, *total_{}, *rangeLabel_{}, *status_{};
    DxUi::TextField *page_{}, *ranges_{};
    DxUi::Button *previous_{}, *next_{}, *zoom_{}, *current_{}, *close_{}, *print_{};
    bool ownerDisabled_{}, fitWidth_{}, actionPending_{};
};
} // namespace RedTilde
