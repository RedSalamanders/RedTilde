#include "PrintPreview.h"
#include "Application.h"
#include "KeyboardState.h"
#include "res/Resource.h"
#include <algorithm>
#include <cmath>
#include <dwmapi.h>
#include <format>
#include <utility>
#include <wil/result.h>

namespace RedTilde
{
namespace
{
void SelectNamedItem(DxUi::ComboBox& combo, std::wstring_view name)
{
    const auto items    = combo.GetItems();
    const auto selected = std::ranges::find_if(items, [name](const auto& item) { return Printing::SamePrintName(item.value, name); });
    combo.SetSelectedIndex(selected != items.end() ? std::optional{static_cast<std::size_t>(selected - items.begin())} : std::nullopt);
}
} // namespace

// A single bounded page raster. The UI recreates its own bitmap after device loss.
class PrintPreview::Sheet final : public DxUi::Control
{
public:
    Sheet()
    {
        EnableVisualContentPreparation();
    }
    Sheet(const Sheet&)            = delete;
    Sheet(Sheet&&)                 = delete;
    Sheet& operator=(const Sheet&) = delete;
    Sheet& operator=(Sheet&&)      = delete;
    void SetPage(Printing::PreviewPage page)
    {
        _page = std::move(page);
        _bitmap.reset();
        RequestVisualContentPreparation();
    }
    [[nodiscard]] const Printing::PreviewPage& Page() const noexcept
    {
        return _page;
    }
    void Paint(DxUi::ControlHost& host) const override
    {
        auto* target = host.GetDeviceContext();
        if (! target || ! _bitmap || _generation != host.GetResourceGeneration())
            return;
        const auto bounds = GetBounds();
        target->DrawBitmap(_bitmap.get(), bounds, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (auto* rule = host.GetSolidBrush(host.GetTheme().borderDefault))
            target->DrawRectangle(bounds, rule);
    }

private:
    void PrepareVisualContent(DxUi::ControlHost& host) noexcept override
    {
        const auto generation = host.GetResourceGeneration();
        if (_page.bgra.empty() || (_bitmap && _generation == generation))
            return;
        _bitmap.reset();
        const auto props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (auto* target = host.GetDeviceContext();
            target && SUCCEEDED(target->CreateBitmap({_page.width, _page.height}, _page.bgra.data(), _page.width * 4u, props, _bitmap.put())))
            _generation = generation;
    }
    void CancelVisualContentPreparation() noexcept override
    {
        _bitmap.reset();
    }

    Printing::PreviewPage _page;
    wil::com_ptr<ID2D1Bitmap1> _bitmap;
    std::uint64_t _generation{};
};

class PrintPreview::Viewport final : public DxUi::ScrollPanel
{
public:
    explicit Viewport(PrintPreview& preview) : _preview(preview)
    {
    }
    Viewport(const Viewport&)            = delete;
    Viewport& operator=(const Viewport&) = delete;

    bool OnMouseWheel(DxUi::ControlHost& host, D2D1_POINT_2F point, float delta, UINT modifiers) override
    {
        if (delta == 0.f || (modifiers & (MK_SHIFT | MK_CONTROL)))
            return false;
        if (! _preview.IsReady())
            return true;
        const auto before = GetScrollOffset();
        if (_preview._fitWidth && ScrollPanel::OnMouseWheel(host, point, delta, modifiers) && before != GetScrollOffset())
        {
            _wheelRemainder = 0.f;
            return true;
        }
        if (_wheelRemainder * delta < 0.f)
            _wheelRemainder = 0.f;
        _wheelRemainder += delta;
        const auto steps = static_cast<int>(_wheelRemainder / static_cast<float>(WHEEL_DELTA));
        _wheelRemainder  = std::fmod(_wheelRemainder, static_cast<float>(WHEEL_DELTA));
        _preview.MoveThroughPages(-steps);
        return true;
    }

    void ResetWheel() noexcept
    {
        _wheelRemainder = 0.f;
    }

private:
    PrintPreview& _preview;
    float _wheelRemainder{};
};

PrintPreview::PrintPreview(Application& app) : _app(app)
{
}

PrintPreview::~PrintPreview()
{
    const bool wasActive = _window && GetActiveWindow() == _window.get();
    if (_ownerDisabled)
    {
        if (IsWindow(_owner))
            EnableWindow(_owner, TRUE);
        if (wasActive && ! _app.IsClosing() && ! _app.IsSessionEnding() && GetForegroundWindow() == _window.get() && IsWindowVisible(_owner))
            (void)SetActiveWindow(_owner);
    }
    _host.Detach();
    _root = nullptr;
    _window.reset();
}

HRESULT PrintPreview::Initialize(std::wstring_view title)
{
    _owner              = _app.Window();
    const auto instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW cls{sizeof(cls)};
    cls.lpfnWndProc   = WindowProc;
    cls.hInstance     = instance;
    cls.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon         = LoadIconW(instance, MAKEINTRESOURCEW(IDI_REDTILDE));
    cls.hIconSm       = cls.hIcon;
    cls.lpszClassName = L"RedTilde.PrintPreview";
    if (! RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());

    const auto dpi    = GetDpiForWindow(_owner);
    const auto scaled = [dpi](int dips) { return MulDiv(dips, static_cast<int>(dpi), 96); };
    RECT parent{};
    RETURN_IF_WIN32_BOOL_FALSE(GetWindowRect(_owner, &parent));
    const auto monitor = MonitorFromWindow(_owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO work{sizeof(work)};
    RETURN_IF_WIN32_BOOL_FALSE(GetMonitorInfoW(monitor, &work));
    const int width = scaled(1040), height = scaled(840);
    const LONG left = parent.left + (parent.right - parent.left - width) / 2, top = parent.top + (parent.bottom - parent.top - height) / 2;
    const auto frame      = Application::ClampWindowBounds({left, top, left + width, top + height}, work.rcWork, dpi, dpi);
    const DWORD exStyle   = _app._options.headless ? WS_EX_NOACTIVATE : 0;
    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX;
    _window.reset(CreateWindowExW(exStyle,
                                  cls.lpszClassName,
                                  Resource(IDS_PRINTPREVIEW).c_str(),
                                  style,
                                  frame.left,
                                  frame.top,
                                  frame.right - frame.left,
                                  frame.bottom - frame.top,
                                  _owner,
                                  nullptr,
                                  instance,
                                  this));
    RETURN_LAST_ERROR_IF_NULL(_window.get());
    if (! _host.Attach(_window.get(), {.forceWarp = _app._options.forceWarp}))
        return HRESULT_FROM_WIN32(GetLastError());
    SetTheme(_app._host.GetTheme());

    auto tree             = std::make_unique<Surface>();
    _root                 = tree.get();
    _sidebar              = _root->AddChild<Surface>(Surface::Kind::Sidebar);
    _sidebar->borderRight = true;
    _header               = _root->AddChild<Surface>();
    _header->borderBottom = true;
    _title                = _root->AddChild<DxUi::Label>(Resource(IDS_PRINTPREVIEW));
    _title->SetFontRole(DxUi::FontRole::Title);
    _document = _root->AddChild<DxUi::Label>(std::wstring(title));
    _document->SetFontRole(DxUi::FontRole::Small);
    _document->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _previous = _root->AddChild<DxUi::Button>(Resource(IDS_PRINTLEFTARROW));
    _previous->SetAccessibleName(Resource(IDS_PRINTPREVIOUS));
    _previous->SetOnClick([this] { _app.RunUiAction([&] { _app.PreviewPrintPage(Page().number - 1u); }); });
    _pageLabel = _root->AddChild<DxUi::Label>(Resource(IDS_PRINTPREVIEWPAGE));
    _page      = _root->AddChild<DxUi::TextField>();
    _page->SetAccessibleName(Resource(IDS_PRINTPREVIEWPAGE));
    _page->SetOnSubmitted([this]
    {
        _app.RunUiAction([&]
        {
            const auto count = Page().count;
            if (const auto number = Printing::ParsePageNumber(_page->GetText(), count))
            {
                _page->SetText(std::to_wstring(number.value()));
                _app.PreviewPrintPage(number.value());
            }
            else
            {
                SetStatus(FormatStringResource(nullptr, IDS_PRINTPAGENUMBER, count));
                _page->SetSelectionRange(0, _page->GetText().size());
            }
        });
    });
    _total = _root->AddChild<DxUi::Label>();
    _next  = _root->AddChild<DxUi::Button>(Resource(IDS_PRINTRIGHTARROW));
    _next->SetAccessibleName(Resource(IDS_PRINTNEXT));
    _next->SetOnClick([this] { _app.RunUiAction([&] { _app.PreviewPrintPage(Page().number + 1u); }); });
    _zoom = _root->AddChild<DxUi::Button>(Resource(IDS_PRINTFITWIDTH));
    _zoom->SetAccessibleName(Resource(IDS_PRINTFITWIDTH));
    _zoom->SetEnabled(false);
    _zoom->SetOnClick([this]
    {
        _app.RunUiAction([&]
        {
            _fitWidth  = ! _fitWidth;
            auto label = Resource(_fitWidth ? IDS_PRINTFITPAGE : IDS_PRINTFITWIDTH);
            _zoom->SetText(label);
            _zoom->SetAccessibleName(std::move(label));
            _scroll->ScrollToTop();
            _scroll->ResetWheel();
            Layout();
        });
    });
    _settings = _root->AddChild<DxUi::ScrollPanel>();
    _settings->SetAccessibleName(Resource(IDS_PRINTSETTINGS));
    const auto addChoice = [&](unsigned id, DxUi::Label*& label, DxUi::ComboBox*& choice)
    {
        label  = _settings->AddChild<DxUi::Label>(Resource(id));
        choice = _settings->AddChild<DxUi::ComboBox>();
        choice->SetAccessibleName(Resource(id));
    };
    addChoice(IDS_PRINTDESTINATION, _printerLabel, _printers);
    _printers->SetPlaceholder(Resource(IDS_PRINTPREVIEWPREPARING));
    _printers->SetOnSelectionChanged([this](std::size_t) { _app.RunUiAction([&] { _app.PreviewPrintDestination(); }); });
    _rangeLabel = _settings->AddChild<DxUi::Label>(Resource(IDS_PRINTPAGES));
    _ranges     = _settings->AddChild<DxUi::TextField>();
    _ranges->SetAccessibleName(Resource(IDS_PRINTPAGES));
    _ranges->SetPlaceholder(Resource(IDS_PRINTPAGESHINT));
    _ranges->SetClearButtonEnabled(true);
    _ranges->SetOnSubmitted([this] { PostAction(Action::Print); });
    _current = _settings->AddChild<DxUi::Button>(Resource(IDS_PRINTCURRENTPAGE));
    _current->SetAccessibleName(Resource(IDS_PRINTCURRENTPAGE));
    _current->SetOnClick([this] { _app.RunUiAction([&] { _ranges->SetText(std::to_wstring(Page().number)); }); });
    addChoice(IDS_PRINTPAPER, _paperLabel, _papers);
    addChoice(IDS_PRINTORIENTATION, _orientationLabel, _orientation);
    _orientation->SetItems({{{}, Resource(IDS_PRINTPORTRAIT)}, {{}, Resource(IDS_PRINTLANDSCAPE)}});
    addChoice(IDS_PRINTDUPLEX, _duplexLabel, _duplex);
    _duplex->SetItems({{{}, Resource(IDS_PRINTONESIDED)}, {{}, Resource(IDS_PRINTDUPLEXLONG)}, {{}, Resource(IDS_PRINTDUPLEXSHORT)}});
    for (auto* choice : {_papers, _orientation, _duplex})
        choice->SetOnSelectionChanged([this](std::size_t) { _app.RunUiAction([&] { UpdatePaperSettings(); }); });
    _copiesLabel = _settings->AddChild<DxUi::Label>(Resource(IDS_PRINTCOPIES));
    _copies      = _settings->AddChild<DxUi::TextField>();
    _copies->SetAccessibleName(Resource(IDS_PRINTCOPIES));
    _copies->SetOnSubmitted([this] { PostAction(Action::Print); });
    _collate = _settings->AddChild<DxUi::Checkbox>(Resource(IDS_PRINTCOLLATE));
    _collate->SetAccessibleName(Resource(IDS_PRINTCOLLATE));
    _file = _settings->AddChild<DxUi::Checkbox>(Resource(IDS_PRINTTOFILE));
    _file->SetAccessibleName(Resource(IDS_PRINTTOFILE));
    SetConfiguration(nullptr, false);
    _close = _root->AddChild<DxUi::Button>(Resource(IDS_CLOSE));
    _close->SetAccessibleName(Resource(IDS_CLOSE));
    _close->SetOnClick([this] { PostAction(Action::Close); });
    _print = _root->AddChild<DxUi::Button>(Resource(IDS_PRINTSUBMIT));
    _print->SetAccessibleName(Resource(IDS_PRINTSUBMIT));
    _print->SetOnClick([this] { PostAction(Action::Print); });
    _print->SetPrimary(true);
    _status = _root->AddChild<DxUi::Label>();
    _status->SetMultiline(true);
    _status->SetFontRole(DxUi::FontRole::Small);
    _scroll = _root->AddChild<Viewport>(*this);
    _scroll->SetAccessibleName(Resource(IDS_PRINTPREVIEW));
    _sheet = _scroll->AddChild<Sheet>();
    _host.SetRoot(std::move(tree));
    _host.SetCancelButton(_close);
    _host.SetDefaultButton(_print);
    BeginRendering();
    Layout();
    return S_OK;
}

void PrintPreview::Show()
{
    _host.SetFocusControl(_close, false);
    if (_app._options.headless)
        return;
    _ownerDisabled = IsWindowEnabled(_owner) != FALSE;
    if (_ownerDisabled)
        EnableWindow(_owner, FALSE);
    (void)_host.PrimeForShow();
    ShowWindow(_window.get(), SW_SHOWNORMAL);
}

HWND PrintPreview::Window() const noexcept
{
    return _window.get();
}

DxUi::ControlHost& PrintPreview::Host() noexcept
{
    return _host;
}

const Printing::PreviewPage& PrintPreview::Page() const noexcept
{
    return _sheet->Page();
}

std::wstring_view PrintPreview::Printer() const noexcept
{
    return _printers->GetSelectedValue();
}

std::optional<std::vector<Printing::PageRange>> PrintPreview::SelectedRanges() const
{
    return Printing::ParsePageRanges(_ranges->GetText(), Page().count);
}

std::optional<unsigned> PrintPreview::SelectedCopies(unsigned maximum)
{
    const auto count = _copies->IsVisible() ? Printing::ParsePageNumber(_copies->GetText(), maximum) : std::optional{1u};
    if (! count)
    {
        SetStatus(FormatStringResource(nullptr, IDS_PRINTCOPIESRANGE, maximum));
        _host.SetFocusControl(_copies, ! _app._options.headless && GetForegroundWindow() == _window.get());
        _copies->SetSelectionRange(0, _copies->GetText().size());
    }
    return count;
}

bool PrintPreview::Collate() const noexcept
{
    return _collate->IsVisible() && _collate->IsChecked();
}

bool PrintPreview::PrintToFile() const noexcept
{
    return _file->IsVisible() && _file->IsChecked();
}

bool PrintPreview::IsReady() const noexcept
{
    return _window && ! _rendering && Page().count != 0u;
}

bool PrintPreview::CanPrint() const noexcept
{
    return _window && _print->IsEnabled();
}

void PrintPreview::SetTheme(const DxUi::ThemePalette& theme) noexcept
{
    _host.SetTheme(theme);
    const BOOL dark = theme.dark && ! theme.highContrast;
    (void)DwmSetWindowAttribute(_window.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void PrintPreview::SetDestinations(Printing::Destinations destinations)
{
    std::vector<DxUi::ComboBox::Item> items;
    items.reserve(destinations.names.size());
    for (auto& name : destinations.names)
        items.push_back({name, std::move(name)});
    _printers->SetItems(std::move(items));
    _printers->SetPlaceholder(Resource(IDS_PRINTNOPRINTERSFOUND));
    SelectNamedItem(*_printers, destinations.selected);
    _printers->SetEnabled(! _printers->GetItems().empty());
}

void PrintPreview::SetConfiguration(const Printing::PrinterConfiguration* configuration, bool keepJobValues)
{
    const auto setRow = [](DxUi::Control* label, DxUi::Control* input, bool visible)
    {
        label->SetVisible(visible);
        input->SetVisible(visible);
    };
    const bool showPaper       = configuration && ! configuration->forms.empty();
    const bool showOrientation = configuration && configuration->settings.orientation.has_value();
    const bool showDuplex      = configuration && configuration->settings.duplex.has_value();
    const bool showCopies      = configuration && configuration->maximumCopies > 1u;
    const bool showCollate     = showCopies && configuration->settings.collate.has_value();
    const bool showFile        = configuration && ! Printing::IsFilePort(configuration->selection.port);
    if (showPaper)
    {
        std::vector<DxUi::ComboBox::Item> items;
        items.reserve(configuration->forms.size());
        for (const auto& form : configuration->forms)
            items.push_back({form, form});
        _papers->SetItems(std::move(items));
        SelectNamedItem(*_papers, configuration->settings.form);
    }
    if (showOrientation)
        _orientation->SetSelectedIndex(static_cast<std::size_t>(configuration->settings.orientation.value()));
    if (showDuplex)
        _duplex->SetSelectedIndex(static_cast<std::size_t>(configuration->settings.duplex.value()));
    if (! keepJobValues)
    {
        _copies->SetText(std::to_wstring(configuration ? configuration->settings.copies : 1u));
        _collate->SetChecked(showCollate && configuration->settings.collate.value());
        _file->SetChecked(false);
        _settings->ScrollToTop();
    }
    setRow(_paperLabel, _papers, showPaper);
    setRow(_orientationLabel, _orientation, showOrientation);
    setRow(_duplexLabel, _duplex, showDuplex);
    setRow(_copiesLabel, _copies, showCopies);
    _collate->SetVisible(showCollate);
    _file->SetVisible(showFile);
    if (_scroll)
        Layout();
}

void PrintPreview::UpdatePaperSettings()
{
    if (! _app._printConfiguration || ! IsReady())
        return;
    auto selected = _app._printConfiguration->settings;
    if (_papers->IsVisible())
        selected.form = _papers->GetSelectedValue();
    if (_orientation->IsVisible())
        selected.orientation = static_cast<Printing::Orientation>(_orientation->GetSelectedIndex().value());
    if (_duplex->IsVisible())
        selected.duplex = static_cast<Printing::Duplex>(_duplex->GetSelectedIndex().value());
    _app.PreviewPrintDestination(std::move(selected));
}

void PrintPreview::BeginRendering(std::wstring_view status)
{
    _rendering = true;
    _print->SetEnabled(false);
    _page->SetEnabled(false);
    _previous->SetEnabled(false);
    _next->SetEnabled(false);
    _ranges->SetEnabled(false);
    _printers->SetEnabled(! _printers->GetItems().empty());
    _papers->SetEnabled(false);
    _orientation->SetEnabled(false);
    _duplex->SetEnabled(false);
    _copies->SetEnabled(false);
    _collate->SetEnabled(false);
    _file->SetEnabled(false);
    _current->SetEnabled(false);
    _sheet->SetVisible(false);
    _zoom->SetEnabled(false);
    SetStatus(status.empty() ? Resource(IDS_PRINTPREVIEWPREPARING) : std::wstring(status));
}

void PrintPreview::Present(Printing::PreviewPage page, std::wstring status)
{
    const bool first  = Page().count == 0u;
    _rendering        = false;
    const auto count  = page.count;
    const auto number = page.number;
    _total->SetText(FormatStringResource(nullptr, IDS_PRINTPREVIEWTOTAL, count));
    _page->SetText(std::to_wstring(number));
    _sheet->SetVisible(true);
    _sheet->SetPage(std::move(page));
    _zoom->SetEnabled(true);
    _page->SetEnabled(true);
    _previous->SetEnabled(number > 1u);
    _next->SetEnabled(number < count);
    _ranges->SetEnabled(true);
    _printers->SetEnabled(! _printers->GetItems().empty());
    _papers->SetEnabled(_papers->IsVisible());
    _orientation->SetEnabled(_orientation->IsVisible());
    _duplex->SetEnabled(_duplex->IsVisible());
    _copies->SetEnabled(_copies->IsVisible());
    _collate->SetEnabled(_collate->IsVisible());
    _file->SetEnabled(_file->IsVisible());
    _current->SetEnabled(true);
    _print->SetEnabled(_app._printConfiguration.has_value());
    _scroll->ScrollToTop();
    _scroll->ResetWheel();
    SetStatus(std::move(status));
    if (_status->GetText().empty() && ! SelectedRanges())
        ReportInvalidRange(false);
    Layout();
    if (std::exchange(_scrollToPageEnd, false))
        _scroll->SetScrollOffset(_scroll->GetContentHeight());
    if (first && CanPrint() && _host.GetFocusControl() == _close)
        _host.SetFocusControl(_print, false);
}

void PrintPreview::MoveThroughPages(int steps)
{
    if (! IsReady() || steps == 0)
        return;
    const auto number = static_cast<int>(Page().number);
    const auto next   = std::clamp(number + steps, 1, static_cast<int>(Page().count));
    if (next != number)
    {
        _scrollToPageEnd = _fitWidth && steps < 0;
        _app.PreviewPrintPage(static_cast<unsigned>(next));
    }
}

void PrintPreview::SetStatus(std::wstring status)
{
    _status->SetText(std::move(status));
}

void PrintPreview::ReportInvalidRange(bool focus)
{
    const auto count = Page().count;
    SetStatus(FormatStringResource(nullptr, IDS_PRINTRANGE, count));
    if (focus)
    {
        _host.SetFocusControl(_ranges, ! _app._options.headless && GetForegroundWindow() == _window.get());
        _ranges->SetSelectionRange(0, _ranges->GetText().size());
    }
}

void PrintPreview::Layout()
{
    const auto client = _host.GetClientBoundsDip();
    const auto width = client.right, height = client.bottom;
    _root->SetBounds(client);
    if (width <= 0.f || height <= 0.f)
        return;
    constexpr float headerBottom = 56.f;
    const float panelWidth       = std::min(width * .45f, std::clamp(width * .30f, 214.f, 308.f));
    const float padding          = width < 720.f ? 14.f : 22.f;
    _header->SetBounds({0.f, 0.f, width, headerBottom});
    _sidebar->SetBounds({0.f, headerBottom, panelWidth, height});
    _title->SetBounds({padding, 10.f, width * .52f, 44.f});
    _document->SetBounds({width * .53f, 10.f, width - padding, 44.f});

    const float panelRight     = panelWidth - padding;
    const float footer         = std::max(294.f, height - 60.f);
    const float settingsBottom = footer - 72.f;
    _settings->SetBounds({0.f, headerBottom, panelWidth, settingsBottom});
    const float inputRight = panelRight - _settings->GetScrollbarThickness();
    float y                = headerBottom + 12.f;
    const auto field       = [&](DxUi::Label* label, DxUi::Control* input)
    {
        if (! input->IsVisible())
            return;
        label->SetBounds({padding, y, inputRight, y + 22.f});
        input->SetBounds({padding, y + 24.f, inputRight, y + 60.f});
        y += 69.f;
    };
    field(_printerLabel, _printers);
    field(_rangeLabel, _ranges);
    _current->SetBounds({padding, y - 3.f, inputRight, y + 29.f});
    y += 40.f;
    field(_paperLabel, _papers);
    field(_orientationLabel, _orientation);
    field(_duplexLabel, _duplex);
    field(_copiesLabel, _copies);
    for (auto* check : {_collate, _file})
    {
        if (! check->IsVisible())
            continue;
        check->SetBounds({padding, y, inputRight, y + 32.f});
        y += 36.f;
    }
    _settings->SetContentHeight(y - headerBottom);
    const float mid = panelWidth * .5f;
    _status->SetBounds({padding, settingsBottom + 4.f, panelRight, footer - 4.f});
    _close->SetBounds({padding, footer, mid - 4.f, footer + 42.f});
    _print->SetBounds({mid + 4.f, footer, panelRight, footer + 42.f});

    const float nav            = panelWidth + padding;
    const float right          = width - padding;
    const bool narrowToolbar   = right - nav < 332.f;
    constexpr float toolbarTop = 66.f, toolbarBottom = 100.f;
    _previous->SetBounds({nav, toolbarTop, nav + 34.f, toolbarBottom});
    _pageLabel->SetBounds({nav + 42.f, toolbarTop, nav + 78.f, toolbarBottom});
    _page->SetBounds({nav + 81.f, toolbarTop, nav + 129.f, toolbarBottom});
    _total->SetBounds({nav + 136.f, toolbarTop, nav + 194.f, toolbarBottom});
    _next->SetBounds({nav + 198.f, toolbarTop, nav + 232.f, toolbarBottom});
    const float zoomTop = narrowToolbar ? toolbarBottom + 8.f : toolbarTop;
    _zoom->SetBounds({right - 96.f, zoomTop, right, zoomTop + 34.f});
    const float canvasTop = zoomTop + 42.f;
    const float bottom    = std::max(canvasTop + 1.f, height);
    _scroll->SetBounds({panelWidth, canvasTop, width, bottom});
    const auto paper           = Page().paper.size;
    constexpr float gutter     = 16.f;
    const float scrollbar      = _fitWidth ? _scroll->GetScrollbarThickness() : 0.f;
    const float roomWidth      = width - panelWidth - 2.f * gutter - scrollbar;
    const float viewportHeight = bottom - canvasTop;
    if (! Page().count || roomWidth <= 0.f || viewportHeight <= 2.f * gutter)
        return;
    const float scaleWidth = roomWidth / paper.width;
    const float scale      = _fitWidth ? scaleWidth : std::min(scaleWidth, (viewportHeight - 2.f * gutter) / paper.height);
    const float sheetWidth = paper.width * scale, sheetHeight = paper.height * scale;
    const float left = (panelWidth + width - sheetWidth - scrollbar) * .5f;
    const float top  = canvasTop + std::max(gutter, (viewportHeight - sheetHeight) * .5f);
    _sheet->SetBounds({left, top, left + sheetWidth, top + sheetHeight});
    _scroll->SetContentHeight(top - canvasTop + sheetHeight + gutter);
}

void PrintPreview::PostAction(Action action) noexcept
{
    if (_pendingAction == Action::Close)
        return;
    const bool wasPending = _pendingAction.has_value();
    _pendingAction        = action; // Close overrides pending Print cleanly
    if (! wasPending && ! PostMessageW(_window.get(), PreviewAction, static_cast<WPARAM>(action), 0))
        _pendingAction.reset();
}

bool PrintPreview::DispatchKeyboardMessage(const MSG& message)
{
    if (! message.hwnd || (message.hwnd != _window.get() && message.hwnd != _host.GetTextInputHwnd()))
        return false;
    if (message.message == WM_KEYDOWN)
    {
        const bool control = KeyboardState::Held(VK_CONTROL), shift = KeyboardState::Held(VK_SHIFT), alt = KeyboardState::Held(VK_MENU);
        if (control && ! shift && ! alt && message.wParam == 'P')
        {
            if (! (message.lParam & (static_cast<LPARAM>(1) << 30)))
                PostAction(Action::Print);
            return true;
        }
        const auto* focus  = _host.GetFocusControl();
        const auto* choice = dynamic_cast<const DxUi::ComboBox*>(focus);
        if (! control && ! shift && ! alt && (message.wParam == VK_PRIOR || message.wParam == VK_NEXT) && focus != _page && focus != _ranges &&
            focus != _copies && (! choice || ! choice->IsPopupOpen()))
        {
            const bool forward = message.wParam == VK_NEXT;
            if (_fitWidth)
            {
                const auto viewport = _scroll->GetViewportRect();
                const auto before   = _scroll->GetScrollOffset();
                _scroll->SetScrollOffset(before + (forward ? 1.f : -1.f) * (viewport.bottom - viewport.top));
                if (before != _scroll->GetScrollOffset())
                    return true;
            }
            MoveThroughPages(forward ? 1 : -1);
            return true;
        }
    }
    return _host.DispatchKeyboardMessage(message);
}

LRESULT CALLBACK PrintPreview::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_NCCREATE)
    {
        const auto* creation = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
    }
    auto* preview = reinterpret_cast<PrintPreview*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (preview && preview->_root)
    {
        LRESULT result{};
        preview->_app.RunUiAction([&] { result = preview->HandleMessage(hwnd, message, wParam, lParam); });
        return result;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PrintPreview::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == PreviewAction)
    {
        const auto action = std::exchange(_pendingAction, std::nullopt);
        if (action.has_value())
        {
            if (action.value() == Action::Print)
                _app.SubmitPrintPreview();
            else if (action.value() == Action::Close)
                _app.DismissPrintPreview();
        }
        return 0;
    }
    if (message == WM_CLOSE)
    {
        PostAction(Action::Close);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        _host.Detach();
        _root = nullptr;
        (void)_window.release();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        if (_app._printPreview.get() == this)
        {
            Printing::Stop(_app._printOperation);
            _app.ClosePrintPreview();
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_GETMINMAXINFO)
    {
        const auto dpi = GetDpiForWindow(hwnd);
        MONITORINFO monitor{sizeof(monitor)};
        if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
        {
            auto* info           = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = {std::min(static_cast<LONG>(MulDiv(660, static_cast<int>(dpi), 96)), monitor.rcWork.right - monitor.rcWork.left),
                                    std::min(static_cast<LONG>(MulDiv(480, static_cast<int>(dpi), 96)), monitor.rcWork.bottom - monitor.rcWork.top)};
        }
        return 0;
    }
    bool handled{};
    const auto result = _host.HandleMessage(hwnd, message, wParam, lParam, handled);
    if (! IsIconic(hwnd) && (message == WM_SIZE || message == WM_DPICHANGED))
        Layout();
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}
} // namespace RedTilde
