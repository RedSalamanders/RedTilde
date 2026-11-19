#include "PrintPreview.h"
#include "Application.h"
#include "res/Resource.h"
#include <algorithm>
#include <dwmapi.h>
#include <format>
#include <wil/result.h>

namespace RedTilde
{
// A single bounded page raster. The UI recreates its own bitmap after device loss.
class PrintPreview::Sheet final : public DxUi::Control
{
  public:
    Sheet()
    {
        EnableVisualContentPreparation();
    }
    Sheet(const Sheet &) = delete;
    Sheet(Sheet &&) = delete;
    Sheet &operator=(const Sheet &) = delete;
    Sheet &operator=(Sheet &&) = delete;
    void SetPage(Printing::PreviewPage page)
    {
        page_ = std::move(page);
        bitmap_.reset();
        RequestVisualContentPreparation();
    }
    [[nodiscard]] const Printing::PreviewPage &Page() const noexcept
    {
        return page_;
    }
    void Paint(DxUi::ControlHost &host) const override
    {
        auto *target = host.GetDeviceContext();
        if (!target || !bitmap_ || generation_ != host.GetResourceGeneration())
            return;
        const auto bounds = GetBounds();
        target->DrawBitmap(bitmap_.get(), bounds, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (auto *rule = host.GetSolidBrush(host.GetTheme().borderDefault))
            target->DrawRectangle(bounds, rule);
    }

  private:
    void PrepareVisualContent(DxUi::ControlHost &host) noexcept override
    {
        const auto generation = host.GetResourceGeneration();
        if (page_.bgra.empty() || (bitmap_ && generation_ == generation))
            return;
        bitmap_.reset();
        const auto props =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (auto *target = host.GetDeviceContext();
            target && SUCCEEDED(target->CreateBitmap({page_.width, page_.height}, page_.bgra.data(), page_.width * 4u, props, bitmap_.put())))
            generation_ = generation;
    }
    void CancelVisualContentPreparation() noexcept override
    {
        bitmap_.reset();
    }

    Printing::PreviewPage page_;
    wil::com_ptr<ID2D1Bitmap1> bitmap_;
    std::uint64_t generation_{};
};

PrintPreview::PrintPreview(Application &app) : app_(app) {}

PrintPreview::~PrintPreview()
{
    const bool wasActive = window_ && GetActiveWindow() == window_.get();
    if (ownerDisabled_)
    {
        if (IsWindow(owner_))
            EnableWindow(owner_, TRUE);
        if (wasActive && !app_.closing && !app_.sessionEnding && GetForegroundWindow() == window_.get() && IsWindowVisible(owner_))
            (void)SetActiveWindow(owner_);
    }
    host_.Detach();
    root_ = nullptr;
    window_.reset();
}

HRESULT PrintPreview::Initialize(std::wstring_view title)
{
    owner_ = app_.Window();
    const auto instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW cls{sizeof(cls)};
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.lpszClassName = L"RedTilde.PrintPreview";
    if (!RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());

    const auto dpi = GetDpiForWindow(owner_);
    const auto scaled = [dpi](int dips) { return MulDiv(dips, static_cast<int>(dpi), 96); };
    RECT parent{};
    RETURN_IF_WIN32_BOOL_FALSE(GetWindowRect(owner_, &parent));
    const auto monitor = MonitorFromWindow(owner_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO work{sizeof(work)};
    RETURN_IF_WIN32_BOOL_FALSE(GetMonitorInfoW(monitor, &work));
    const int width = scaled(880), height = scaled(840);
    const LONG left = parent.left + (parent.right - parent.left - width) / 2, top = parent.top + (parent.bottom - parent.top - height) / 2;
    const auto frame = Application::ClampWindowBounds({left, top, left + width, top + height}, work.rcWork, dpi, dpi);
    const DWORD exStyle = app_.options.headless ? WS_EX_NOACTIVATE : 0;
    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX;
    window_.reset(CreateWindowExW(exStyle, cls.lpszClassName, Resource(IDS_PRINTPREVIEW).c_str(), style, frame.left, frame.top,
                                  frame.right - frame.left, frame.bottom - frame.top, owner_, nullptr, instance, this));
    RETURN_LAST_ERROR_IF_NULL(window_.get());
    if (!host_.Attach(window_.get(), {.forceWarp = app_.options.forceWarp}))
        return HRESULT_FROM_WIN32(GetLastError());
    SetTheme(app_.host.GetTheme());

    auto tree = std::make_unique<Surface>();
    root_ = tree.get();
    title_ = root_->AddChild<DxUi::Label>(Resource(IDS_PRINTPREVIEW));
    title_->SetFontRole(DxUi::FontRole::Title);
    document_ = root_->AddChild<DxUi::Label>(std::wstring(title));
    document_->SetFontRole(DxUi::FontRole::Small);
    document_->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    page_ = root_->AddChild<DxUi::NumericStepper>();
    page_->SetLabel(Resource(IDS_PRINTPREVIEWPAGE), 38.f);
    page_->Field().SetAccessibleName(Resource(IDS_PRINTPREVIEWPAGE));
    page_->SetStepButtonNames(Resource(IDS_PRINTNEXT), Resource(IDS_PRINTPREVIOUS));
    page_->SetMinimum(1);
    page_->SetMaximum(1);
    page_->SetEnabled(false);
    page_->SetOnChange(
        [this](const DxUi::NumericStepperChange &change)
        {
            if (change.phase == DxUi::NumericStepperChangePhase::Commit)
                app_.RunUiAction([&] { app_.PreviewPrintPage(static_cast<unsigned>(change.value)); });
        });
    total_ = root_->AddChild<DxUi::Label>();
    zoom_ = root_->AddChild<DxUi::Button>(Resource(IDS_PRINTFITWIDTH));
    zoom_->SetAccessibleName(Resource(IDS_PRINTFITWIDTH));
    zoom_->SetEnabled(false);
    zoom_->SetOnClick(
        [this]
        {
            app_.RunUiAction(
                [&]
                {
                    fitWidth_ = !fitWidth_;
                    auto label = Resource(fitWidth_ ? IDS_PRINTFITPAGE : IDS_PRINTFITWIDTH);
                    zoom_->SetText(label);
                    zoom_->SetAccessibleName(std::move(label));
                    scroll_->ScrollToTop();
                    Layout();
                });
        });
    close_ = root_->AddChild<DxUi::Button>(Resource(IDS_CLOSE));
    close_->SetAccessibleName(Resource(IDS_CLOSE));
    close_->SetOnClick([this] { PostAction(Action::Close); });
    print_ = root_->AddChild<DxUi::Button>(Resource(IDS_PRINT));
    print_->SetAccessibleName(Resource(IDS_PRINT));
    print_->SetOnClick([this] { PostAction(Action::Print); });
    print_->SetPrimary(true);
    status_ = root_->AddChild<DxUi::Label>();
    status_->SetMultiline(true);
    status_->SetFontRole(DxUi::FontRole::Small);
    scroll_ = root_->AddChild<DxUi::ScrollPanel>();
    sheet_ = scroll_->AddChild<Sheet>();
    host_.SetRoot(std::move(tree));
    host_.SetCancelButton(close_);
    host_.SetDefaultButton(print_);
    BeginRendering();
    Layout();
    return S_OK;
}

void PrintPreview::Show()
{
    host_.SetFocusControl(close_, false);
    if (app_.options.headless)
        return;
    ownerDisabled_ = IsWindowEnabled(owner_) != FALSE;
    if (ownerDisabled_)
        EnableWindow(owner_, FALSE);
    (void)host_.PrimeForShow();
    ShowWindow(window_.get(), SW_SHOWNORMAL);
}

HWND PrintPreview::Window() const noexcept
{
    return window_.get();
}

DxUi::ControlHost &PrintPreview::Host() noexcept
{
    return host_;
}

const Printing::PreviewPage &PrintPreview::Page() const noexcept
{
    return sheet_->Page();
}

bool PrintPreview::CanPrint() const noexcept
{
    return window_ && print_->IsEnabled();
}

void PrintPreview::SetTheme(const DxUi::ThemePalette &theme) noexcept
{
    host_.SetTheme(theme);
    const BOOL dark = theme.dark && !theme.highContrast;
    (void)DwmSetWindowAttribute(window_.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void PrintPreview::BeginRendering()
{
    print_->SetEnabled(false);
    page_->SetEnabled(false);
    sheet_->SetVisible(false);
    zoom_->SetEnabled(false);
    SetStatus(Resource(IDS_PRINTPREVIEWPREPARING));
}

void PrintPreview::Present(Printing::PreviewPage page, std::wstring status)
{
    const bool first = Page().count == 0u;
    const auto count = page.count;
    total_->SetText(std::vformat(Resource(IDS_PRINTPREVIEWTOTAL), std::make_wformat_args(count)));
    page_->SetMaximum(count);
    page_->SetValue(page.number);
    sheet_->SetVisible(true);
    sheet_->SetPage(std::move(page));
    zoom_->SetEnabled(true);
    page_->SetEnabled(true);
    print_->SetEnabled(true);
    scroll_->ScrollToTop();
    SetStatus(std::move(status));
    Layout();
    if (first && host_.GetFocusControl() == close_)
        host_.SetFocusControl(print_, false);
}

void PrintPreview::SetStatus(std::wstring status)
{
    status_->SetText(std::move(status));
}

void PrintPreview::Layout()
{
    const auto client = host_.GetClientBoundsDip();
    const auto width = client.right, height = client.bottom;
    root_->SetBounds(client);
    const bool compact = width < 626.f;
    const float padding = compact ? 12.f : 24.f;
    const float toolbar = compact ? 98.f : 54.f;
    const float canvasTop = toolbar + 46.f;
    const float right = width - padding;
    title_->SetBounds({padding, 12.f, compact ? right : width * .54f, 44.f});
    document_->SetVisible(!compact);
    document_->SetBounds({width * .55f, 12.f, right, 44.f});
    page_->SetBounds({padding, 54.f, padding + 166.f, 88.f});
    total_->SetBounds({padding + 176.f, 54.f, padding + 260.f, 88.f});
    zoom_->SetBounds({right - 302.f, toolbar, right - 194.f, toolbar + 34.f});
    close_->SetBounds({right - 184.f, toolbar, right - 102.f, toolbar + 34.f});
    print_->SetBounds({right - 92.f, toolbar, right, toolbar + 34.f});
    const float bottom = std::max(canvasTop + 1.f, height - 62.f);
    scroll_->SetBounds({0.f, canvasTop, width, bottom});
    status_->SetBounds({padding, bottom + 6.f, right, height - 8.f});
    const auto paper = Page().paper.size;
    constexpr float gutter = 20.f;
    const float roomWidth = width - 2.f * gutter - scroll_->GetScrollbarThickness();
    const float viewportHeight = bottom - canvasTop;
    if (!Page().count || roomWidth <= 0.f || viewportHeight <= 2.f * gutter)
        return;
    const float scaleWidth = roomWidth / paper.width;
    const float scale = fitWidth_ ? scaleWidth : std::min(scaleWidth, (viewportHeight - 2.f * gutter) / paper.height);
    const float sheetWidth = paper.width * scale, sheetHeight = paper.height * scale;
    const float left = (width - sheetWidth) * .5f;
    const float top = canvasTop + std::max(gutter, (viewportHeight - sheetHeight) * .5f);
    sheet_->SetBounds({left, top, left + sheetWidth, top + sheetHeight});
    scroll_->SetContentHeight(top - canvasTop + sheetHeight + gutter);
}

void PrintPreview::PostAction(Action action) noexcept
{
    if (!actionPending_)
        actionPending_ = PostMessageW(window_.get(), PreviewAction, static_cast<WPARAM>(action), 0) != FALSE;
}

bool PrintPreview::DispatchKeyboardMessage(const MSG &message)
{
    if (!message.hwnd || (message.hwnd != window_.get() && message.hwnd != host_.GetTextInputHwnd()))
        return false;
    if (message.message == WM_KEYDOWN)
    {
        const bool control = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0, alt = GetKeyState(VK_MENU) < 0;
        if (control && !shift && !alt && message.wParam == 'P')
        {
            if (!(message.lParam & (static_cast<LPARAM>(1) << 30)))
                PostAction(Action::Print);
            return true;
        }
        if (!control && !shift && !alt && (message.wParam == VK_PRIOR || message.wParam == VK_NEXT) && host_.GetFocusControl() != &page_->Field())
        {
            const bool forward = message.wParam == VK_NEXT;
            if (fitWidth_)
            {
                const auto viewport = scroll_->GetViewportRect();
                scroll_->SetScrollOffset(scroll_->GetScrollOffset() + (forward ? 1.f : -1.f) * (viewport.bottom - viewport.top));
            }
            else if (CanPrint())
                app_.PreviewPrintPage(forward ? std::min(Page().number + 1u, Page().count) : std::max(Page().number, 2u) - 1u);
            return true;
        }
    }
    return host_.DispatchKeyboardMessage(message);
}

LRESULT CALLBACK PrintPreview::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_NCCREATE)
    {
        const auto *creation = reinterpret_cast<const CREATESTRUCTW *>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
    }
    auto *preview = reinterpret_cast<PrintPreview *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (preview && preview->root_)
    {
        LRESULT result{};
        preview->app_.RunUiAction([&] { result = preview->HandleMessage(hwnd, message, wParam, lParam); });
        return result;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PrintPreview::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == PreviewAction)
    {
        actionPending_ = false;
        if (wParam == static_cast<WPARAM>(Action::Print))
            app_.ChoosePrintDestination();
        else if (wParam == static_cast<WPARAM>(Action::Close))
            app_.DismissPrintPreview();
        return 0;
    }
    if (message == WM_CLOSE)
    {
        PostAction(Action::Close);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        host_.Detach();
        root_ = nullptr;
        (void)window_.release();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        if (app_.printPreview.get() == this)
        {
            Printing::Stop(app_.printOperation);
            app_.ClosePrintPreview();
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_GETMINMAXINFO)
    {
        const auto dpi = GetDpiForWindow(hwnd);
        MONITORINFO monitor{sizeof(monitor)};
        if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
        {
            auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
            info->ptMinTrackSize = {std::min(static_cast<LONG>(MulDiv(660, static_cast<int>(dpi), 96)), monitor.rcWork.right - monitor.rcWork.left),
                                    std::min(static_cast<LONG>(MulDiv(480, static_cast<int>(dpi), 96)), monitor.rcWork.bottom - monitor.rcWork.top)};
        }
        return 0;
    }
    bool handled{};
    const auto result = host_.HandleMessage(hwnd, message, wParam, lParam, handled);
    if (!IsIconic(hwnd) && (message == WM_SIZE || message == WM_DPICHANGED))
        Layout();
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}
} // namespace RedTilde
