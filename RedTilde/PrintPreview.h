#pragma once
#include "Printing.h"
#include "UiControls.h"

namespace RedTilde
{
class Application;

class PrintPreview final
{
public:
    explicit PrintPreview(Application& app);
    ~PrintPreview();
    PrintPreview(const PrintPreview&)            = delete;
    PrintPreview& operator=(const PrintPreview&) = delete;

    [[nodiscard]] HRESULT Initialize(std::wstring_view title);
    void Show();
    [[nodiscard]] HWND Window() const noexcept;
    [[nodiscard]] DxUi::ControlHost& Host() noexcept;
    [[nodiscard]] const Printing::PreviewPage& Page() const noexcept;
    [[nodiscard]] std::wstring_view Printer() const noexcept;
    [[nodiscard]] std::optional<std::vector<Printing::PageRange>> SelectedRanges() const;
    [[nodiscard]] std::optional<unsigned> SelectedCopies(unsigned maximum);
    [[nodiscard]] bool Collate() const noexcept;
    [[nodiscard]] bool PrintToFile() const noexcept;
    [[nodiscard]] bool IsReady() const noexcept;
    [[nodiscard]] bool CanPrint() const noexcept;
    [[nodiscard]] bool DispatchKeyboardMessage(const MSG& message);
    void SetTheme(const DxUi::ThemePalette& theme) noexcept;
    void SetDestinations(Printing::Destinations destinations);
    void SetConfiguration(const Printing::PrinterConfiguration* configuration, bool keepJobValues);
    void BeginRendering(std::wstring_view status = {});
    void Present(Printing::PreviewPage page, std::wstring status);
    void SetStatus(std::wstring status);
    void ReportInvalidRange(bool focus);

private:
    class Sheet;
    class Viewport;
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
    void MoveThroughPages(int steps);
    void UpdatePaperSettings();

    Application& _app;
    HWND _owner{};
    wil::unique_hwnd _window;
    DxUi::ControlHost _host;
    Surface *_root{}, *_header{}, *_sidebar{};
    DxUi::ScrollPanel* _settings{};
    Viewport* _scroll{};
    Sheet* _sheet{};
    DxUi::Label *_title{}, *_document{}, *_pageLabel{}, *_total{}, *_printerLabel{}, *_rangeLabel{}, *_status{};
    DxUi::Label *_paperLabel{}, *_orientationLabel{}, *_duplexLabel{}, *_copiesLabel{};
    DxUi::ComboBox *_printers{}, *_papers{}, *_orientation{}, *_duplex{};
    DxUi::TextField *_page{}, *_ranges{}, *_copies{};
    DxUi::Checkbox *_collate{}, *_file{};
    DxUi::Button *_previous{}, *_next{}, *_zoom{}, *_current{}, *_close{}, *_print{};
    bool _ownerDisabled{}, _fitWidth{}, _scrollToPageEnd{}, _rendering{};
    std::optional<Action> _pendingAction;
};
} // namespace RedTilde
