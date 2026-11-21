#include "Printing.h"
#include "Common/ProcessWork.h"
#include "Common/Storage.h"
// delayimp.h requires the Windows declarations first.
// clang-format off
#include <windows.h>
#include <delayimp.h>
// clang-format on
#include <DxUi/Embedded.h>
#include <DxUi/Failure.h>
#include <DxUi/RichSemanticDocument.h>
#include <DxUi/RichTextExport.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <d3d11.h>
#include <documenttarget.h>
#include <format>
#include <limits>
#include <mutex>
#include <objbase.h>
#include <prntvpt.h>
#include <shlwapi.h>
#include <span>
#include <wil/com.h>
#include <wil/result.h>
#include <wincodec.h>
#include <winspool.h>

namespace RedTilde::Printing
{
bool SamePrintName(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() &&
           (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}

bool IsFilePort(std::wstring_view port) noexcept
{
    return SamePrintName(port, L"PORTPROMPT:") || SamePrintName(port, L"FILE:");
}

namespace
{
constexpr unsigned MaximumPages         = static_cast<unsigned>(DxUi::RichText::Document::MaximumUnits) + 1u;
constexpr std::size_t MaximumPageRanges = 32u;
constexpr float MaximumPreviewEdge      = 1600.f;
constexpr std::size_t MaximumImageBytes = 64u * 1024u * 1024u;
constexpr HRESULT Cancelled             = HRESULT_FROM_WIN32(ERROR_CANCELLED);

HRESULT LoadPrintImports(const char* library) noexcept
{
    // Keep print-only DLLs out of ordinary editor startup. Convert the two
    // delay-loader failures to checked job errors at first use.
    __try
    {
        return __HrLoadAllImportsForDll(library);
    }
    __except ((GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND) ||
               GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH)
    {
        return HRESULT_FROM_WIN32(GetExceptionCode() & 0xffffu);
    }
}

bool ValidPaper(const Paper& paper) noexcept
{
    const auto& size = paper.size;
    const auto& area = paper.printable;
    return std::isfinite(size.width) && std::isfinite(size.height) && size.width > 0.f && size.height > 0.f && size.width <= 16384.f &&
           size.height <= 16384.f && std::isfinite(area.left) && std::isfinite(area.top) && std::isfinite(area.right) && std::isfinite(area.bottom) &&
           area.left >= 0.f && area.top >= 0.f && area.right <= size.width && area.bottom <= size.height && area.right - area.left >= 1.f &&
           area.bottom - area.top >= 1.f;
}

struct Band
{
    float top{}, bottom{};
};

struct Page
{
    float top{}, height{}, scale = 1.f;
};

void MergeBands(std::vector<Band>& bands)
{
    std::ranges::sort(bands, {}, &Band::top);
    std::size_t count{};
    for (const auto band : bands)
    {
        if (count && band.top < bands[count - 1].bottom)
            bands[count - 1].bottom = std::max(bands[count - 1].bottom, band.bottom);
        else
            bands[count++] = band;
    }
    bands.resize(count);
}

class Pages
{
public:
    Pages()                        = default;
    Pages(const Pages&)            = delete;
    Pages& operator=(const Pages&) = delete;
    HRESULT Prepare(const Document& document, const Paper& paper, IWICImagingFactory* wic, const std::atomic_bool& cancel);
    HRESULT Preview(unsigned number, PreviewPage& output);
    HRESULT Record(unsigned number, ID2D1CommandList** output);
    ID2D1DeviceContext& Context() const noexcept
    {
        return *_host.Controls().GetDeviceContext();
    }
    [[nodiscard]] unsigned Count() const noexcept
    {
        return static_cast<unsigned>(_pages.size());
    }

private:
    HRESULT FinishPreparation(const std::atomic_bool& cancel);
    HRESULT ResolveImages(const Document& document, IWICImagingFactory* wic, const std::atomic_bool& cancel);
    HRESULT Paginate(const std::atomic_bool& cancel);
    std::optional<Band> SourceBand(DxUi::RichText::MarkdownRange source) const;

    DxUi::EmbeddedHost _host;
    DxUi::RichTextEditor* _editor{};
    UINT _width{}, _height{};
    Paper _paper{};
    D2D1_RECT_F _content{};
    std::vector<Page> _pages;
    unsigned _previewed{};
    wil::com_ptr<ID2D1CommandList> _previewCommands;
};

HRESULT Pages::FinishPreparation(const std::atomic_bool& cancel)
{
    for (unsigned step = 0; step < 4096u; ++step)
    {
        if (cancel.load())
            return Cancelled;
        RETURN_IF_FAILED(_host.Prepare(_width, _height, 96.f));
        if (! _host.NeedsPreparation())
            return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
}

HRESULT Pages::ResolveImages(const Document& document, IWICImagingFactory* wic, const std::atomic_bool& cancel)
{
    using namespace DxUi::RichText;
    std::vector<RichImageRequest> requests;
    _editor->TakePendingImageRequests(requests);
    std::size_t retained{};
    for (const auto& request : requests)
    {
        if (cancel.load())
            return Cancelled;
        std::shared_ptr<const RichImage> pixels;
        if (request.SourceKind() == RichImageSourceKind::LocalFile && ! request.FilePath().starts_with(L"\\\\"))
        {
            std::string encoded;
            if (SUCCEEDED(ReadImageFileBytes(request.FilePath(), encoded, &cancel)))
                (void)RichImage::Decode(wic, std::as_bytes(std::span(encoded)), pixels, &cancel);
        }
        else
        {
            const auto ready = std::ranges::find_if(document.networkImages, [&](const auto& image) {
                return image.request.SourceKind() == request.SourceKind() && image.request.Source() == request.Source();
            });
            if (ready != document.networkImages.end())
                pixels = ready->pixels;
        }
        if (pixels && pixels->RetainedBytes() > MaximumImageBytes - retained)
            pixels.reset();
        if (pixels)
            retained += pixels->RetainedBytes();
        (void)_editor->CompleteImageRequest(request, std::move(pixels));
    }
    return requests.empty() ? S_OK : FinishPreparation(cancel);
}

HRESULT Pages::Prepare(const Document& document, const Paper& paper, IWICImagingFactory* wic, const std::atomic_bool& cancel)
{
    constexpr float margin = 72.f;
    if (! ValidPaper(paper))
        return E_INVALIDARG;
    _paper             = paper;
    const auto& area   = paper.printable;
    const float insetX = std::min(margin, paper.size.width / 4.f), insetY = std::min(margin, paper.size.height / 4.f);
    _content = {std::max(area.left, insetX),
                std::max(area.top, insetY),
                std::min(area.right, paper.size.width - insetX),
                std::min(area.bottom, paper.size.height - insetY)};
    if (_content.right - _content.left < 1.f || _content.bottom - _content.top < 1.f)
        _content = area;
    _width = static_cast<UINT>(_content.right - _content.left);
    // The persistent surface is used only to prepare native resources. The
    // document extent and the recorded page lists are independent of its height.
    _height = static_cast<UINT>(std::clamp(_content.bottom - _content.top, 1.f, 512.f));
    wil::com_ptr<ID3D11Device> device;
    RETURN_IF_FAILED(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr));
    std::shared_ptr<DxUi::GraphicsDevice> graphics;
    RETURN_IF_FAILED(DxUi::GraphicsDevice::Create(device.get(), graphics));
    RETURN_IF_FAILED(_host.Attach(std::move(graphics)));
    using namespace DxUi::RichText;
    auto editor = std::make_unique<DxUi::RichTextEditor>(document.source.Text());
    if (editor->TrySetMarkdownAnalysisProfile(MarkdownProfile::RichMarkdown) != SourceResult::Success ||
        editor->TrySetMarkdownAnalysisExtensions(document.extensions) != SourceResult::Success ||
        editor->TrySetMarkdownRenderingEnabled(true) != SourceResult::Success || editor->TrySetViewMode(ViewMode::Rich) != SourceResult::Success ||
        editor->TrySetDocumentFilePath(document.path.native()) != SourceResult::Success)
        return E_INVALIDARG;
    editor->SetReadOnly(true);
    editor->SetRemoteImagesEnabled(document.allowRemoteImages);
    DxUi::RichTextEditorAppearance appearance;
    appearance.drawBorder = appearance.drawFocusIndicator = false;
    appearance.paddingDip                                 = 0.f;
    appearance.bodyFontSizeDip                            = 14.f;
    appearance.codeFontSizeDip                            = 13.f;
    appearance.autoParagraphDirection                     = true;
    appearance.strikeCompletedTasks                       = document.strikeCompletedTasks;
    appearance.markdownPresentation                       = MarkdownPresentationStyle::Subtle;
    editor->SetAppearance(appearance);
    _editor = editor.get();
    _host.Controls().SetRoot(std::move(editor));
    RETURN_IF_FAILED(FinishPreparation(cancel));
    RETURN_IF_FAILED(ResolveImages(document, wic, cancel));
    return Paginate(cancel);
}

std::optional<Band> Pages::SourceBand(DxUi::RichText::MarkdownRange source) const
{
    const auto rectangles = _editor->TryGetTextInputRangeRects(_host.Controls(), source.first, source.last);
    if (! rectangles || rectangles->empty())
        return std::nullopt;
    Band band{rectangles->front().top, rectangles->front().bottom};
    for (const auto rectangle : rectangles.value())
    {
        band.top    = std::min(band.top, rectangle.top);
        band.bottom = std::max(band.bottom, rectangle.bottom);
    }
    return band;
}

HRESULT Pages::Paginate(const std::atomic_bool& cancel)
{
    using DxUi::RichText::SemanticRole;
    const auto scroll   = _editor->GetScrollInfo();
    const auto viewport = _editor->TryGetTextInputViewportRect();
    if (! scroll || ! viewport || scroll->offsetDip != 0.f || std::abs(viewport->right - viewport->left - static_cast<float>(_width)) > 0.01f)
        return E_UNEXPECTED;
    const auto rectangles = _editor->TryGetTextInputRangeRects(_host.Controls(), 0, _editor->Snapshot().Length());
    if (! rectangles)
        return E_UNEXPECTED;
    const float pageHeight = _content.bottom - _content.top;
    std::vector<Band> bands;
    bands.reserve(rectangles->size());
    for (const auto rectangle : rectangles.value())
        if (rectangle.bottom > rectangle.top && rectangle.right > rectangle.left)
            bands.push_back({std::max(0.f, rectangle.top - viewport->top), rectangle.bottom - viewport->top});
    MergeBands(bands);

    // The source rectangles give physical text lines. Add whole rows and
    // rendered objects where possible, and attach headings to the next line.
    std::vector<Band> keepTogether;
    if (const auto semantic = _editor->GetAcknowledgedSemanticDocument(_host.Controls()))
        for (const auto& element : semantic->Elements())
        {
            if (cancel.load())
                return Cancelled;
            const auto role = element.role;
            if (role != SemanticRole::Heading && role != SemanticRole::Row && role != SemanticRole::Image && role != SemanticRole::Diagram &&
                role != SemanticRole::Math)
                continue;
            auto bounds = SourceBand(element.source);
            if (! bounds)
                continue;
            bounds->top = std::max(0.f, bounds->top - viewport->top);
            bounds->bottom -= viewport->top;
            if (role == SemanticRole::Heading)
            {
                const auto next = std::ranges::lower_bound(bands, bounds->bottom, {}, &Band::top);
                if (next != bands.end() && next->bottom - bounds->top <= pageHeight)
                    bounds->bottom = next->bottom;
            }
            if (role != SemanticRole::Row || bounds->bottom - bounds->top <= pageHeight)
                keepTogether.push_back(bounds.value());
        }
    bands.insert(bands.end(), keepTogether.begin(), keepTogether.end());
    MergeBands(bands);

    float top        = 0.f;
    float lastBottom = 0.f;
    for (const auto band : bands)
    {
        if (cancel.load())
            return Cancelled;
        if (band.bottom - top > pageHeight && lastBottom > top)
        {
            const float end = std::min(band.top, top + pageHeight);
            _pages.push_back({top, end - top});
            top = end;
        }
        if (band.bottom - top > pageHeight)
            top = band.top;
        if (band.bottom - band.top > pageHeight)
        {
            const float height = band.bottom - band.top;
            _pages.push_back({band.top, height, pageHeight / height});
            top = band.bottom;
        }
        lastBottom = band.bottom;
        if (_pages.size() >= MaximumPages)
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    if (lastBottom > top || _pages.empty())
        _pages.push_back({top, std::max(1.f, std::min(pageHeight, scroll->contentHeightDip - top))});
    return S_OK;
}

HRESULT Pages::Record(unsigned number, ID2D1CommandList** output)
{
    if (number == _previewed)
    {
        _previewCommands.copy_to(output);
        return S_OK;
    }
    const auto& page = _pages[number - 1u];
    auto& context    = Context();
    wil::com_ptr<ID2D1CommandList> commands;
    RETURN_IF_FAILED(context.CreateCommandList(commands.put()));
    wil::com_ptr<ID2D1SolidColorBrush> ink, rule, paperFill, shade;
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), ink.put()));
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(0.33f, 0.33f, 0.33f), rule.put()));
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), paperFill.put()));
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.95f, 0.95f), shade.put()));
    wil::com_ptr<ID2D1Image> previous;
    context.GetTarget(previous.put());
    const auto restore = wil::scope_exit([&] noexcept
    {
        context.SetTarget(previous.get());
        context.SetTransform(D2D1::Matrix3x2F::Identity());
    });
    context.SetTarget(commands.get());
    context.SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    context.SetTransform(D2D1::Matrix3x2F::Scale(page.scale, page.scale) * D2D1::Matrix3x2F::Translation(_content.left, _content.top));
    context.BeginDraw();
    // Diagram shapes can cover a large area, such as a subgraph around the entire graph.
    // Their backing follows the paper instead of the shaded code and quote surface.
    const DxUi::RichText::ExportDrawing drawing{
        .text = ink.get(), .decoration = rule.get(), .fill = paperFill.get(), .quoteFill = shade.get(), .underlineLinks = false};
    const bool drawn = _editor->TryDrawPreparedPage(_host.Controls(), {0.f, 0.f, static_cast<float>(_width), page.height}, page.top, drawing);
    RETURN_IF_FAILED(context.EndDraw());
    if (! drawn)
        return E_FAIL;
    context.SetTarget(nullptr);
    RETURN_IF_FAILED(commands->Close());
    *output = commands.detach();
    return S_OK;
}

HRESULT Pages::Preview(unsigned number, PreviewPage& output)
{
    wil::com_ptr<ID2D1CommandList> commands;
    RETURN_IF_FAILED(Record(number, commands.put()));
    auto& context     = Context();
    const float scale = std::min(1.5f, MaximumPreviewEdge / std::max(_paper.size.width, _paper.size.height));
    const auto extent = [scale](float size) { return static_cast<UINT>(std::clamp(std::ceil(size * scale), 1.f, MaximumPreviewEdge)); };
    const auto width  = extent(_paper.size.width);
    const auto height = extent(_paper.size.height);
    const auto format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    const auto props  = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, format, 96.f, 96.f);
    wil::com_ptr<ID2D1Bitmap1> target, readback;
    RETURN_IF_FAILED(context.CreateBitmap({width, height}, nullptr, 0, props, target.put()));
    {
        wil::com_ptr<ID2D1Image> previous;
        context.GetTarget(previous.put());
        const auto restore = wil::scope_exit([&] noexcept
        {
            context.SetTarget(previous.get());
            context.SetTransform(D2D1::Matrix3x2F::Identity());
        });
        context.SetTarget(target.get());
        context.SetTransform(D2D1::Matrix3x2F::Scale(scale, scale));
        context.BeginDraw();
        context.Clear(D2D1::ColorF(D2D1::ColorF::White));
        context.DrawImage(commands.get());
        RETURN_IF_FAILED(context.EndDraw());
    }
    const auto cpuProps = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, format, 96.f, 96.f);
    RETURN_IF_FAILED(context.CreateBitmap({width, height}, nullptr, 0, cpuProps, readback.put()));
    RETURN_IF_FAILED(readback->CopyFromBitmap(nullptr, target.get(), nullptr));
    const auto rowBytes = width * 4u;
    output.bgra.resize(static_cast<std::size_t>(rowBytes) * height);
    D2D1_MAPPED_RECT mapped{};
    RETURN_IF_FAILED(readback->Map(D2D1_MAP_OPTIONS_READ, &mapped));
    const auto unmap = wil::scope_exit([&] noexcept { (void)readback->Unmap(); });
    for (std::size_t y = 0; y < height; ++y)
        std::memcpy(output.bgra.data() + y * rowBytes, mapped.bits + y * mapped.pitch, rowBytes);
    output.paper     = _paper;
    output.count     = Count();
    output.number    = number;
    output.width     = width;
    output.height    = height;
    _previewCommands = std::move(commands);
    _previewed       = number;
    return S_OK;
}

std::wstring DefaultPrinterName()
{
    DWORD length{};
    (void)GetDefaultPrinterW(nullptr, &length);
    if (! length || length > 32768u)
        return {};
    std::wstring name(length, L'\0');
    if (! GetDefaultPrinterW(name.data(), &length))
        return {};
    name.resize(length - 1u);
    return name;
}

Destinations ListDestinations(std::wstring_view preferred)
{
    Destinations result;
    if (FAILED(LoadPrintImports("WINSPOOL.DRV")))
        return result;
    constexpr DWORD flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    DWORD bytes{}, count{};
    (void)EnumPrintersW(flags, nullptr, 4u, nullptr, 0u, &bytes, &count);
    for (unsigned attempt = 0; attempt < 2u && bytes >= sizeof(PRINTER_INFO_4W) && bytes <= 4u * 1024u * 1024u; ++attempt)
    {
        std::vector<BYTE> buffer(bytes);
        if (! EnumPrintersW(flags, nullptr, 4u, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes, &count))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                continue;
            break;
        }
        if (count > buffer.size() / sizeof(PRINTER_INFO_4W))
            break;
        const auto printers = std::span(reinterpret_cast<const PRINTER_INFO_4W*>(buffer.data()), count);
        for (const auto& printer : printers)
            if (printer.pPrinterName && printer.pPrinterName[0] != L'\0')
                result.names.emplace_back(printer.pPrinterName);
        break;
    }
    const auto defaultPrinter = DefaultPrinterName();
    if (! defaultPrinter.empty() && std::ranges::none_of(result.names, [&](const auto& name) { return SamePrintName(name, defaultPrinter); }))
        result.names.push_back(defaultPrinter);
    std::ranges::sort(result.names, [](const std::wstring& left, const std::wstring& right) {
        return CompareStringOrdinal(left.c_str(), -1, right.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    });
    result.names.erase(std::unique(result.names.begin(), result.names.end(), SamePrintName), result.names.end());
    const auto select = [&](std::wstring_view name)
    { return std::ranges::find_if(result.names, [&](const auto& candidate) { return SamePrintName(candidate, name); }); };
    auto chosen = select(preferred);
    if (chosen == result.names.end())
        chosen = select(defaultPrinter);
    if (chosen != result.names.end())
        result.selected = *chosen;
    else if (! result.names.empty())
        result.selected = result.names.front();
    return result;
}

using Printer                      = wil::unique_any<HANDLE, decltype(&ClosePrinter), ClosePrinter>;
constexpr DWORD MaximumDriverBytes = 1024u * 1024u;

HRESULT ReadPrinterInfo(HANDLE printer, std::vector<BYTE>& bytes)
{
    DWORD needed{};
    if (! GetPrinterW(printer, 2, nullptr, 0, &needed))
    {
        const auto error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
            return HRESULT_FROM_WIN32(error ? error : ERROR_INVALID_DATA);
    }
    if (needed < sizeof(PRINTER_INFO_2W) || needed > MaximumDriverBytes)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    bytes.resize(needed);
    RETURN_IF_WIN32_BOOL_FALSE(GetPrinterW(printer, 2, bytes.data(), needed, &needed));
    return S_OK;
}

std::vector<std::wstring> ListForms(HANDLE printer)
{
    DWORD bytes{}, count{};
    (void)EnumFormsW(printer, 1u, nullptr, 0u, &bytes, &count);
    if (bytes < sizeof(FORM_INFO_1W) || bytes > MaximumDriverBytes)
        return {};
    std::vector<BYTE> storage(bytes);
    if (! EnumFormsW(printer, 1u, storage.data(), bytes, &bytes, &count) || count > storage.size() / sizeof(FORM_INFO_1W))
        return {};
    std::vector<std::wstring> names;
    for (const auto& form : std::span(reinterpret_cast<const FORM_INFO_1W*>(storage.data()), count))
        if (form.pName && form.pName[0] != L'\0' && wcsnlen_s(form.pName, CCHFORMNAME) < CCHFORMNAME)
            names.emplace_back(form.pName);
    return names;
}

DEVMODEW* CheckedDeviceMode(std::span<std::byte> bytes) noexcept
{
    if (bytes.size() < sizeof(DEVMODEW))
        return nullptr;
    auto* const mode = reinterpret_cast<DEVMODEW*>(bytes.data());
    return mode->dmSize >= sizeof(DEVMODEW) && static_cast<size_t>(mode->dmSize) + mode->dmDriverExtra <= bytes.size() ? mode : nullptr;
}

PrinterSettings SettingsFrom(const DEVMODEW& mode)
{
    PrinterSettings settings;
    if (mode.dmFields & DM_FORMNAME)
        settings.form.assign(mode.dmFormName, std::ranges::find(mode.dmFormName, L'\0'));
    if (mode.dmFields & DM_ORIENTATION)
        settings.orientation = mode.dmOrientation == DMORIENT_LANDSCAPE ? Orientation::Landscape : Orientation::Portrait;
    if (mode.dmFields & DM_DUPLEX)
        settings.duplex = mode.dmDuplex == DMDUP_VERTICAL ? Duplex::LongEdge : mode.dmDuplex == DMDUP_HORIZONTAL ? Duplex::ShortEdge : Duplex::OneSided;
    if (mode.dmFields & DM_COLLATE)
        settings.collate = mode.dmCollate == DMCOLLATE_TRUE;
    if (mode.dmFields & DM_COPIES)
        settings.copies = static_cast<unsigned>(std::max<short>(1, mode.dmCopies));
    return settings;
}

HRESULT MergeDeviceMode(HANDLE printer, Selection& selection, const PrinterSettings& requested)
{
    auto* const mode = CheckedDeviceMode(selection.deviceMode);
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), ! mode);
    const auto existing = SettingsFrom(*mode);
    if (requested.form == existing.form && requested.orientation == existing.orientation && requested.duplex == existing.duplex &&
        requested.collate == existing.collate && requested.copies == existing.copies)
        return S_OK;
    if (! requested.form.empty() && ! SamePrintName(requested.form, existing.form))
    {
        RETURN_HR_IF(E_INVALIDARG, requested.form.size() >= CCHFORMNAME);
        mode->dmFields = (mode->dmFields & ~(DM_PAPERSIZE | DM_PAPERWIDTH | DM_PAPERLENGTH)) | DM_FORMNAME;
        std::ranges::fill(mode->dmFormName, L'\0');
        std::ranges::copy(requested.form, mode->dmFormName);
    }
    if (requested.orientation)
    {
        mode->dmFields |= DM_ORIENTATION;
        mode->dmOrientation = requested.orientation.value() == Orientation::Landscape ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
    }
    if (requested.duplex)
    {
        const auto duplex = requested.duplex.value();
        mode->dmFields |= DM_DUPLEX;
        mode->dmDuplex = duplex == Duplex::LongEdge ? DMDUP_VERTICAL : duplex == Duplex::ShortEdge ? DMDUP_HORIZONTAL : DMDUP_SIMPLEX;
    }
    if (requested.collate)
    {
        mode->dmFields |= DM_COLLATE;
        mode->dmCollate = requested.collate.value() ? DMCOLLATE_TRUE : DMCOLLATE_FALSE;
    }
    if (requested.copies != existing.copies)
    {
        RETURN_HR_IF(E_INVALIDARG, requested.copies < 1u || requested.copies > static_cast<unsigned>((std::numeric_limits<short>::max)()));
        mode->dmFields |= DM_COPIES;
        mode->dmCopies = static_cast<short>(requested.copies);
    }
    const LONG needed = DocumentPropertiesW(nullptr, printer, selection.printer.data(), nullptr, nullptr, 0);
    if (needed < static_cast<LONG>(sizeof(DEVMODEW)) || static_cast<size_t>(needed) > selection.deviceMode.size() ||
        DocumentPropertiesW(nullptr, printer, selection.printer.data(), mode, mode, DM_IN_BUFFER | DM_OUT_BUFFER) != IDOK ||
        ! CheckedDeviceMode(selection.deviceMode))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto applied = SettingsFrom(*mode);
    if ((! requested.form.empty() && ! SamePrintName(applied.form, requested.form)) ||
        (requested.orientation && applied.orientation != requested.orientation) || (requested.duplex && applied.duplex != requested.duplex) ||
        (requested.collate && applied.collate != requested.collate) || applied.copies != requested.copies)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    return S_OK;
}

HRESULT PaperForSelection(Selection& selection, Paper& paper) noexcept
{
    const auto* mode = CheckedDeviceMode(selection.deviceMode);
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), ! mode);
    const wil::unique_hdc dc(CreateICW(L"WINSPOOL", selection.printer.c_str(), nullptr, mode));
    return ReadPaper(dc.get(), paper);
}

HRESULT ApplyJobOptions(Selection& selection)
{
    auto* const mode = CheckedDeviceMode(selection.deviceMode);
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), ! mode);
    auto desired = SettingsFrom(*mode);
    if (selection.copies.value_or(desired.copies) == desired.copies && (! selection.collate || selection.collate == desired.collate))
        return S_OK;
    desired.copies = selection.copies.value_or(desired.copies);
    if (selection.collate)
        desired.collate = selection.collate;
    Printer printer;
    RETURN_IF_WIN32_BOOL_FALSE(OpenPrinterW(selection.printer.data(), printer.put(), nullptr));
    RETURN_IF_FAILED(MergeDeviceMode(printer.get(), selection, desired));
    Paper paper;
    RETURN_IF_FAILED(PaperForSelection(selection, paper));
    if (paper != selection.paper)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    return S_OK;
}

HRESULT Ticket(Selection& selection, IStream** ticket)
{
    RETURN_IF_FAILED(LoadPrintImports("prntvpt.dll"));
    using Provider = wil::unique_any<HPTPROVIDER, decltype(&PTCloseProvider), PTCloseProvider>;
    Provider provider;
    RETURN_IF_FAILED(PTOpenProvider(selection.printer.c_str(), 1, provider.put()));
    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.put()));
    RETURN_IF_FAILED(PTConvertDevModeToPrintTicket(
        provider.get(), static_cast<ULONG>(selection.deviceMode.size()), reinterpret_cast<DEVMODEW*>(selection.deviceMode.data()), kPTJobScope, stream.get()));
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));
    *ticket = stream.detach();
    return S_OK;
}

HRESULT DestinationFormat(Selection& selection, FileFormat& format)
{
    RETURN_IF_FAILED(LoadPrintImports("WINSPOOL.DRV"));
    Printer printer;
    RETURN_IF_WIN32_BOOL_FALSE(OpenPrinterW(selection.printer.data(), printer.put(), nullptr));
    std::vector<BYTE> bytes;
    RETURN_IF_FAILED(ReadPrinterInfo(printer.get(), bytes));
    const auto name = reinterpret_cast<const PRINTER_INFO_2W*>(bytes.data())->pDriverName;
    if (name && SamePrintName(name, L"Microsoft Print To PDF"))
        format = FileFormat::Pdf;
    else if (name && (SamePrintName(name, L"Microsoft XPS Document Writer v4") || SamePrintName(name, L"Microsoft XPS Document Writer")))
        format = FileFormat::OpenXps;
    else
        format = FileFormat::DriverDefined;
    return S_OK;
}

struct OutputIdentity
{
    FILE_ID_INFO id{};
    FILE_BASIC_INFO basic{};
    FILE_STANDARD_INFO standard{};

    bool operator==(const OutputIdentity& other) const noexcept
    {
        return id.VolumeSerialNumber == other.id.VolumeSerialNumber && std::ranges::equal(id.FileId.Identifier, other.id.FileId.Identifier) &&
               basic.LastWriteTime.QuadPart == other.basic.LastWriteTime.QuadPart && basic.ChangeTime.QuadPart == other.basic.ChangeTime.QuadPart &&
               standard.EndOfFile.QuadPart == other.standard.EndOfFile.QuadPart;
    }
};

HRESULT IdentifyOutput(const std::filesystem::path& path, std::optional<OutputIdentity>& identity)
{
    wil::unique_handle file(CreateFileW(path.c_str(),
                                        FILE_READ_ATTRIBUTES,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr,
                                        OPEN_EXISTING,
                                        FILE_FLAG_OPEN_REPARSE_POINT,
                                        nullptr));
    if (! file)
    {
        const auto error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
            return HRESULT_FROM_WIN32(error);
        identity.reset();
        return S_OK;
    }
    OutputIdentity facts;
    RETURN_IF_WIN32_BOOL_FALSE(GetFileInformationByHandleEx(file.get(), FileBasicInfo, &facts.basic, sizeof(facts.basic)));
    if ((facts.basic.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0u)
        return E_ACCESSDENIED;
    RETURN_IF_WIN32_BOOL_FALSE(GetFileInformationByHandleEx(file.get(), FileIdInfo, &facts.id, sizeof(facts.id)));
    RETURN_IF_WIN32_BOOL_FALSE(GetFileInformationByHandleEx(file.get(), FileStandardInfo, &facts.standard, sizeof(facts.standard)));
    identity = facts;
    return S_OK;
}

// Hold the new sibling against replacement until the driver closes its stream.
// Publish the chosen filename only after successful spooling and stream commit.
class FileOutput
{
public:
    FileOutput()                             = default;
    FileOutput(const FileOutput&)            = delete;
    FileOutput& operator=(const FileOutput&) = delete;
    ~FileOutput()
    {
        stream.reset();
        _lease.reset();
        if (! _staged.empty())
            (void)DeleteFileW(_staged.c_str());
    }

    HRESULT Open(const std::filesystem::path& path, const std::filesystem::path& source)
    {
        if (! path.is_absolute())
            return E_INVALIDARG;
        RETURN_IF_FAILED(IdentifyOutput(path, _expected));
        std::error_code error;
        if (! source.empty() && (SamePrintName(path.native(), source.native()) || (_expected && std::filesystem::equivalent(path, source, error))))
            return E_ACCESSDENIED;
        GUID id{};
        RETURN_IF_FAILED(CoCreateGuid(&id));
        wchar_t leaf[40]{};
        if (! StringFromGUID2(id, leaf, static_cast<int>(std::size(leaf))))
            return E_FAIL;
        auto staged = path.parent_path() / std::format(L".redtilde-print-{}.tmp", leaf);
        _lease.reset(CreateFileW(staged.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        RETURN_LAST_ERROR_IF(! _lease);
        _staged.swap(staged);
        return SHCreateStreamOnFileEx(_staged.c_str(), STGM_READWRITE | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, stream.put());
    }

    HRESULT Publish(const std::filesystem::path& path, const std::atomic_bool& cancel)
    {
        RETURN_IF_FAILED(stream->Commit(STGC_DEFAULT));
        STATSTG facts{};
        RETURN_IF_FAILED(stream->Stat(&facts, STATFLAG_NONAME));
        if (! facts.cbSize.QuadPart)
            return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        stream.reset();
        std::optional<OutputIdentity> current;
        RETURN_IF_FAILED(IdentifyOutput(path, current));
        if (current != _expected)
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        if (cancel.load())
            return Cancelled;
        _lease.reset();
        if (_expected)
            RETURN_IF_WIN32_BOOL_FALSE(ReplaceFileW(path.c_str(), _staged.c_str(), nullptr, 0, nullptr, nullptr));
        if (! _expected)
            RETURN_IF_WIN32_BOOL_FALSE(MoveFileExW(_staged.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH));
        _staged.clear();
        return S_OK;
    }

    wil::com_ptr<IStream> stream;

private:
    std::filesystem::path _staged;
    wil::unique_handle _lease;
    std::optional<OutputIdentity> _expected;
};
} // namespace

bool Paper::operator==(const Paper& other) const noexcept
{
    return size.width == other.size.width && size.height == other.size.height && printable.left == other.printable.left &&
           printable.top == other.printable.top && printable.right == other.printable.right && printable.bottom == other.printable.bottom;
}

namespace
{
std::wstring_view TrimPageInput(std::wstring_view text) noexcept
{
    constexpr std::wstring_view space = L" \t\r\n";
    const auto first                  = text.find_first_not_of(space);
    return first == text.npos ? std::wstring_view{} : text.substr(first, text.find_last_not_of(space) - first + 1u);
}
} // namespace

std::optional<unsigned> ParsePageNumber(std::wstring_view text, unsigned pageCount) noexcept
{
    if (text.size() > 512u)
        return std::nullopt;
    unsigned page{};
    for (const auto character : TrimPageInput(text))
    {
        if (character < L'0' || character > L'9')
            return std::nullopt;
        const auto digit = static_cast<unsigned>(character - L'0');
        if (page > pageCount / 10u || (page == pageCount / 10u && digit > pageCount % 10u))
            return std::nullopt;
        page = page * 10u + digit;
    }
    return page ? std::optional{page} : std::nullopt;
}

std::optional<std::vector<PageRange>> ParsePageRanges(std::wstring_view text, unsigned pageCount)
{
    if (text.size() > 512u)
        return std::nullopt;
    std::vector<PageRange> ranges;
    if (TrimPageInput(text).empty())
        return ranges;
    for (;;)
    {
        const auto separator = text.find(L',');
        const auto span      = text.substr(0, separator);
        const auto dash      = span.find_first_of(L"-–");
        const auto first     = ParsePageNumber(span.substr(0, dash), pageCount);
        const auto last      = dash == span.npos ? first : ParsePageNumber(span.substr(dash + 1u), pageCount);
        if (! first || ! last || first.value() > last.value() || ranges.size() == MaximumPageRanges)
            return std::nullopt;
        ranges.push_back({first.value(), last.value()});
        if (separator == text.npos)
            return ranges;
        text.remove_prefix(separator + 1u);
    }
}

bool PageRangesFit(std::span<const PageRange> ranges, unsigned pageCount) noexcept
{
    return std::ranges::all_of(ranges, [pageCount](const auto& range) { return range.first && range.last >= range.first && range.last <= pageCount; });
}

HRESULT ReadPaper(HDC dc, Paper& paper) noexcept
{
    if (! dc)
        return E_HANDLE;
    const int xDpi = GetDeviceCaps(dc, LOGPIXELSX), yDpi = GetDeviceCaps(dc, LOGPIXELSY);
    if (xDpi <= 0 || yDpi <= 0)
        return E_INVALIDARG;
    const float x = 96.f / static_cast<float>(xDpi), y = 96.f / static_cast<float>(yDpi);
    Paper selected;
    selected.size    = {static_cast<float>(GetDeviceCaps(dc, PHYSICALWIDTH)) * x, static_cast<float>(GetDeviceCaps(dc, PHYSICALHEIGHT)) * y};
    const float left = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETX)) * x, top = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETY)) * y;
    selected.printable = {std::max(0.f, left),
                          std::max(0.f, top),
                          std::min(selected.size.width, left + static_cast<float>(GetDeviceCaps(dc, HORZRES)) * x),
                          std::min(selected.size.height, top + static_cast<float>(GetDeviceCaps(dc, VERTRES)) * y)};
    if (! ValidPaper(selected))
        return E_INVALIDARG;
    paper = selected;
    return S_OK;
}

HRESULT ConfigurePrinter(const std::wstring& name, const std::optional<PrinterSettings>& requested, PrinterConfiguration& result)
{
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND), name.empty());
    RETURN_IF_FAILED(LoadPrintImports("WINSPOOL.DRV"));
    PrinterConfiguration configured;
    auto& selection   = configured.selection;
    selection.printer = name;
    Printer printer;
    RETURN_IF_WIN32_BOOL_FALSE(OpenPrinterW(selection.printer.data(), printer.put(), nullptr));
    std::vector<BYTE> info;
    RETURN_IF_FAILED(ReadPrinterInfo(printer.get(), info));
    const auto port = reinterpret_cast<const PRINTER_INFO_2W*>(info.data())->pPortName;
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), ! port || port[0] == L'\0');
    selection.port    = port;
    const LONG needed = DocumentPropertiesW(nullptr, printer.get(), selection.printer.data(), nullptr, nullptr, 0);
    if (needed < static_cast<LONG>(sizeof(DEVMODEW)) || needed > static_cast<LONG>(MaximumDriverBytes))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    selection.deviceMode.resize(static_cast<size_t>(needed));
    auto* const mode = reinterpret_cast<DEVMODEW*>(selection.deviceMode.data());
    if (DocumentPropertiesW(nullptr, printer.get(), selection.printer.data(), mode, nullptr, DM_OUT_BUFFER) != IDOK ||
        ! CheckedDeviceMode(selection.deviceMode))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (requested)
        RETURN_IF_FAILED(MergeDeviceMode(printer.get(), selection, requested.value()));
    RETURN_IF_FAILED(PaperForSelection(selection, selection.paper));
    configured.settings = SettingsFrom(*mode);
    if (! configured.settings.form.empty())
    {
        configured.forms = ListForms(printer.get());
        if (std::ranges::none_of(configured.forms, [&](const auto& form) { return SamePrintName(form, configured.settings.form); }))
            configured.forms.insert(configured.forms.begin(), configured.settings.form);
    }
    if (mode->dmFields & DM_COPIES)
    {
        const auto copies        = DeviceCapabilitiesW(name.c_str(), port, DC_COPIES, nullptr, mode);
        configured.maximumCopies = static_cast<unsigned>(std::clamp(copies, 1, static_cast<int>((std::numeric_limits<short>::max)())));
    }
    configured.settings.copies = std::min(configured.settings.copies, configured.maximumCopies);
    if (configured.settings.duplex && DeviceCapabilitiesW(name.c_str(), port, DC_DUPLEX, nullptr, mode) != 1)
        configured.settings.duplex.reset();
    if (configured.settings.collate && DeviceCapabilitiesW(name.c_str(), port, DC_COLLATE, nullptr, mode) != 1)
        configured.settings.collate.reset();
    result = std::move(configured);
    return S_OK;
}

namespace
{
constexpr DWORD DefaultPrinterConfigurationTimeoutMs = 1500u;
constexpr std::size_t MaximumSpoolerProbes           = 8u;
std::atomic_size_t activeSpoolerProbes{0u};
static_assert(std::is_trivially_destructible_v<std::atomic_size_t>);

struct Request
{
    std::uint64_t id{};
    std::uint64_t printerEpoch = 1u;
    unsigned page              = 1u;
    std::optional<std::wstring> printer;
    std::optional<PrinterSettings> settings;
    std::optional<Selection> selection;
};

enum class PostResult
{
    Posted,
    Superseded,
    Cancelled
};
} // namespace

struct Operation
{
    Operation()                            = default;
    Operation(const Operation&)            = delete;
    Operation& operator=(const Operation&) = delete;
    std::mutex mutex;
    HWND window{};
    UINT message{};
    wil::unique_event_nothrow requested, fileChosen;
    std::atomic_bool cancel{};
    std::uint64_t activeRequestId    = 1u;
    std::uint64_t activePrinterEpoch = 1u;
    std::optional<Notice> notice;
    std::optional<Notice> pendingConfig;
    Request request;
    HRESULT fileStatus = Cancelled;
    std::filesystem::path destination;

    PostResult Post(Notice next) noexcept
    {
        const std::lock_guard lock(mutex);
        if (! window || cancel.load())
            return PostResult::Cancelled;
        if (next.kind == Notice::Kind::Preview)
        {
            if (next.printerEpoch != activePrinterEpoch)
                return PostResult::Superseded;
            if (next.requestId != activeRequestId)
            {
                if (next.stage == Stage::ConfigurePrinter)
                {
                    pendingConfig = Notice{.kind           = Notice::Kind::Preview,
                                           .result         = next.result,
                                           .stage          = next.stage,
                                           .printerEpoch   = next.printerEpoch,
                                           .printerPending = next.printerPending,
                                           .printer        = std::move(next.printer),
                                           .configuration  = std::move(next.configuration)};
                }
                return PostResult::Superseded;
            }
            if (pendingConfig && pendingConfig->printerEpoch == activePrinterEpoch && next.stage != Stage::ConfigurePrinter)
            {
                next.stage          = pendingConfig->stage;
                next.result         = pendingConfig->result;
                next.printer        = std::move(pendingConfig->printer);
                next.printerPending = pendingConfig->printerPending;
                next.configuration  = std::move(pendingConfig->configuration);
                pendingConfig.reset();
            }
            else if (next.stage == Stage::ConfigurePrinter)
            {
                pendingConfig.reset();
            }
        }
        if (notice && notice->destinations && ! next.destinations)
            next.destinations = std::move(notice->destinations);
        notice = std::move(next);
        return PostMessageW(window, message, 0, 0) != FALSE ? PostResult::Posted : PostResult::Cancelled;
    }
};

namespace
{
struct Work
{
    Work()                       = default;
    Work(const Work&)            = delete;
    Work& operator=(const Work&) = delete;
    ProcessWorkLease lease;
    std::shared_ptr<Operation> operation;
    Job job;
};

struct PrinterConfigProbe final
{
    PrinterConfigProbe()                                     = default;
    ~PrinterConfigProbe()                                    = default;
    PrinterConfigProbe(const PrinterConfigProbe&)            = delete;
    PrinterConfigProbe& operator=(const PrinterConfigProbe&) = delete;
    PrinterConfigProbe(PrinterConfigProbe&&)                 = delete;
    PrinterConfigProbe& operator=(PrinterConfigProbe&&)      = delete;

    std::wstring printer;
    std::optional<PrinterSettings> requested;
    bool retryWithoutSettings{};
    std::atomic_bool abandoned{};
    wil::unique_event_nothrow done;
    HRESULT result = E_FAIL;
    std::optional<PrinterConfiguration> configuration;
#if defined(REDTILDE_ENABLE_TESTS)
    bool useTestPrinter{};
    std::function<HRESULT(const std::wstring&, const std::optional<PrinterSettings>&, PrinterConfiguration&)> printerForTest;
    ProcessWorkLease testLease;
#endif
};

DWORD WINAPI RunPrinterConfigProbe(void* context) noexcept
{
    const std::unique_ptr<std::shared_ptr<PrinterConfigProbe>> holder(static_cast<std::shared_ptr<PrinterConfigProbe>*>(context));
    const auto probe = *holder;
#if defined(REDTILDE_ENABLE_TESTS)
    // Release any test callback lease only after clearing the test callback and signaling completion.
    ProcessWorkLease testLease = std::move(probe->testLease);
#endif
    const auto cleanup = wil::scope_exit([&]() noexcept
    {
        activeSpoolerProbes.fetch_sub(1u, std::memory_order_acq_rel);
        (void)probe->done.SetEvent();
    });
    try
    {
        PrinterConfiguration configured;
#if defined(REDTILDE_ENABLE_TESTS)
        const auto releaseCallback = wil::scope_exit([&]() noexcept { probe->printerForTest = {}; });
        if (probe->useTestPrinter)
        {
            HRESULT hr = probe->printerForTest ? probe->printerForTest(probe->printer, probe->requested, configured) : E_INVALIDARG;
            if (FAILED(hr) && probe->retryWithoutSettings && probe->requested && ! probe->abandoned.load(std::memory_order_acquire))
                hr = probe->printerForTest ? probe->printerForTest(probe->printer, std::nullopt, configured) : E_INVALIDARG;
            probe->result = hr;
            if (SUCCEEDED(hr))
                probe->configuration = std::move(configured);
            return 0u;
        }
#endif
        HRESULT hr = ConfigurePrinter(probe->printer, probe->requested, configured);
        if (FAILED(hr) && probe->retryWithoutSettings && probe->requested && ! probe->abandoned.load(std::memory_order_acquire))
            hr = ConfigurePrinter(probe->printer, std::nullopt, configured);
        probe->result = hr;
        if (SUCCEEDED(hr))
            probe->configuration = std::move(configured);
    }
    catch (const std::bad_alloc&)
    {
        std::terminate();
    }
    catch (const std::exception&)
    {
        std::terminate();
    }
    return 0u;
}

HRESULT StartPrinterConfigProbe([[maybe_unused]] const Work& work,
                                bool retryWithoutSettings,
                                std::wstring printer,
                                std::optional<PrinterSettings> requested,
                                std::shared_ptr<PrinterConfigProbe>& outProbe)
{
    outProbe.reset();
#if defined(REDTILDE_ENABLE_TESTS)
    const bool useTestPrinter = work.job.destinationsForTest.has_value();
    if (! useTestPrinter)
        RETURN_IF_FAILED(LoadPrintImports("WINSPOOL.DRV"));
#else
    RETURN_IF_FAILED(LoadPrintImports("WINSPOOL.DRV"));
#endif
    auto probe = std::make_shared<PrinterConfigProbe>();
    RETURN_IF_FAILED(probe->done.create(wil::EventOptions::ManualReset));
    probe->printer              = std::move(printer);
    probe->requested            = std::move(requested);
    probe->retryWithoutSettings = retryWithoutSettings;
#if defined(REDTILDE_ENABLE_TESTS)
    probe->useTestPrinter = useTestPrinter;
    if (useTestPrinter)
    {
        probe->printerForTest = work.job.printerForTest;
        RETURN_IF_FAILED(ProcessWork::Acquire(probe->testLease));
    }
#endif
    std::size_t current = activeSpoolerProbes.load(std::memory_order_acquire);
    for (;;)
    {
        if (current >= MaximumSpoolerProbes)
            return HRESULT_FROM_WIN32(ERROR_BUSY);
        if (activeSpoolerProbes.compare_exchange_weak(current, current + 1u, std::memory_order_acq_rel))
            break;
    }
    auto holder = std::make_unique<std::shared_ptr<PrinterConfigProbe>>(probe);
    const wil::unique_handle thread(CreateThread(nullptr, 0u, RunPrinterConfigProbe, holder.get(), 0u, nullptr));
    if (! thread)
    {
        const DWORD error = GetLastError();
        activeSpoolerProbes.fetch_sub(1u, std::memory_order_acq_rel);
        return HRESULT_FROM_WIN32(error);
    }
    (void)holder.release();
    outProbe = std::move(probe);
    return S_OK;
}

HRESULT Submit(Work& work, Selection& selection, Pages& pages, IWICImagingFactory* wic, Notice& finished)
{
    auto& operation = *work.operation;
    auto& job       = work.job;
    if (operation.cancel.load())
        return Cancelled;
    const auto count   = pages.Count();
    const auto& ranges = selection.ranges;
    if (! PageRangesFit(ranges, count))
    {
        finished.invalidRangePageCount = count;
        return E_INVALIDARG;
    }
    const auto forPages = [&](auto&& submitPage) -> HRESULT
    {
        for (unsigned page = 1u; page <= count; ++page)
        {
            if (operation.cancel.load())
                return Cancelled;
            if (! ranges.empty() && std::ranges::none_of(ranges, [page](const auto& range) { return page >= range.first && page <= range.last; }))
                continue;
            finished.stage      = Stage::Page;
            finished.failedPage = page;
            wil::com_ptr<ID2D1CommandList> commands;
            RETURN_IF_FAILED(pages.Record(page, commands.put()));
            RETURN_IF_FAILED(submitPage(page, *commands));
        }
        return S_OK;
    };
#if defined(REDTILDE_ENABLE_TESTS)
    if (job.pageForTest)
    {
        if (job.submissionForTest)
            job.submissionForTest(selection);
        RETURN_IF_FAILED(forPages(job.pageForTest));
        finished.stage = Stage::FinishJob;
        return job.finishForTest ? job.finishForTest() : S_OK;
    }
#endif
    finished.stage = Stage::ConfigurePrinter;
    RETURN_IF_FAILED(ApplyJobOptions(selection));
    std::filesystem::path destination;
    if (IsFilePort(selection.port))
    {
        finished.stage = Stage::ChooseFile;
        FileFormat format{};
        RETURN_IF_FAILED(DestinationFormat(selection, format));
        if (operation.Post({.kind = Notice::Kind::ChooseFile, .format = format}) != PostResult::Posted)
            return Cancelled;
        // Close always signals this event. The worker retains only immutable
        // inputs and never shows a dialog itself.
        if (! operation.fileChosen.wait())
            return E_FAIL;
        if (operation.cancel.load())
            return Cancelled;
        const std::lock_guard lock(operation.mutex);
        RETURN_IF_FAILED(operation.fileStatus);
        destination = std::move(operation.destination);
        if (! destination.is_absolute())
            return E_INVALIDARG;
    }
    if (operation.cancel.load())
        return Cancelled;
    finished.stage = Stage::PrepareJob;
    wil::com_ptr<IStream> ticket;
    RETURN_IF_FAILED(Ticket(selection, ticket.put()));
    FileOutput output;
    if (! destination.empty())
    {
        finished.stage = Stage::SaveFile;
        RETURN_IF_FAILED(output.Open(destination, job.document.path));
        finished.stage = Stage::PrepareJob;
    }
    wil::com_ptr<IPrintDocumentPackageTargetFactory> factory;
    RETURN_IF_FAILED(CoCreateInstance(__uuidof(PrintDocumentPackageTargetFactory), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
    wil::com_ptr<IPrintDocumentPackageTarget> target;
    RETURN_IF_FAILED(factory->CreateDocumentPackageTargetForPrintJob(
        selection.printer.c_str(), job.document.title.c_str(), output.stream.get(), ticket.get(), target.put()));
    auto cancelOnFailure = wil::scope_exit([&] noexcept { (void)target->Cancel(); });
    wil::com_ptr<ID2D1Device> device;
    pages.Context().GetDevice(device.put());
    wil::com_ptr<ID2D1PrintControl> print;
    RETURN_IF_FAILED(device->CreatePrintControl(wic, target.get(), nullptr, print.put()));
    RETURN_IF_FAILED(forPages([&](unsigned, ID2D1CommandList& commands) { return print->AddPage(&commands, selection.paper.size, nullptr); }));
    if (operation.cancel.load())
        return Cancelled;
    finished.stage = Stage::FinishJob;
    RETURN_IF_FAILED(print->Close());
    cancelOnFailure.release();
    print.reset();
    target.reset();
    if (! destination.empty())
    {
        finished.stage = Stage::SaveFile;
        RETURN_IF_FAILED(output.Publish(destination, operation.cancel));
        finished.savedFile = true;
    }
    return S_OK;
}

HRESULT RunSession(Work& work, Notice& finished)
{
    auto& operation = *work.operation;
    if (operation.cancel.load())
        return Cancelled;
    wil::com_ptr<IWICImagingFactory> wic;
    RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.put())));
    auto& job = work.job;
    const DWORD timeoutMs =
#if defined(REDTILDE_ENABLE_TESTS)
        job.printerTimeoutMsForTest.value_or(DefaultPrinterConfigurationTimeoutMs);
#else
        DefaultPrinterConfigurationTimeoutMs;
#endif
    auto destinations =
#if defined(REDTILDE_ENABLE_TESTS)
        job.destinationsForTest ? std::move(job.destinationsForTest.value()) :
#endif
                                ListDestinations(job.initialPrinter);
    const auto selectedPrinter = destinations.selected;
    if (operation.Post({.kind = Notice::Kind::Destinations, .printer = selectedPrinter, .destinations = std::move(destinations)}) == PostResult::Cancelled)
        return Cancelled;

    constexpr Paper letter{{816.f, 1056.f}, {0.f, 0.f, 816.f, 1056.f}};
    auto paper = letter;
    std::wstring configuredPrinter;
    std::unique_ptr<Pages> pages;
    Request request{.id           = 1u,
                    .printerEpoch = 1u,
                    .printer      = selectedPrinter,
                    .settings     = SamePrintName(job.initialPrinter, selectedPrinter) ? std::move(job.initialSettings) : std::nullopt};
    bool initialRequest            = true;
    std::uint64_t currentRequestId = 1u;
    std::uint64_t currentEpoch     = 1u;
    unsigned currentPage           = 1u;
    std::wstring activePrinter     = selectedPrinter;
    std::shared_ptr<PrinterConfigProbe> pendingProbe;
    const auto abandonPending = wil::scope_exit([&]() noexcept
    {
        if (pendingProbe)
            pendingProbe->abandoned.store(true, std::memory_order_release);
    });

    for (;;)
    {
        if (operation.cancel.load())
            return Cancelled;
        if (request.selection)
        {
            if (pendingProbe)
            {
                pendingProbe->abandoned.store(true, std::memory_order_release);
                pendingProbe.reset();
            }
            finished.stage   = Stage::PrepareJob;
            finished.printer = request.selection.value().printer;
            if (! pages || configuredPrinter.empty() || ! SamePrintName(configuredPrinter, request.selection->printer) || request.selection->paper != paper)
                return E_INVALIDARG;
            return Submit(work, request.selection.value(), *pages, wic.get(), finished);
        }

        currentRequestId = request.id;
        currentEpoch     = request.printerEpoch;
        currentPage      = request.page;
        Notice notice{.kind = Notice::Kind::Preview, .requestId = currentRequestId, .printerEpoch = currentEpoch};
        auto desired = paper;
        if (request.printer)
        {
            if (pendingProbe)
            {
                pendingProbe->abandoned.store(true, std::memory_order_release);
                pendingProbe.reset();
            }
            activePrinter    = request.printer.value();
            const bool retry = std::exchange(initialRequest, false) && request.settings.has_value();
            if (! activePrinter.empty())
            {
                notice.stage   = Stage::ConfigurePrinter;
                notice.printer = activePrinter;
                std::shared_ptr<PrinterConfigProbe> probe;
                const HRESULT started = StartPrinterConfigProbe(work, retry, activePrinter, std::move(request.settings), probe);
                if (FAILED(started))
                {
                    notice.result = started;
                    if (! SamePrintName(configuredPrinter, activePrinter))
                    {
                        configuredPrinter.clear();
                        desired = letter;
                    }
                }
                else
                {
                    const HANDLE waitHandles[2]{operation.requested.get(), probe->done.get()};
                    const DWORD waited = WaitForMultipleObjects(2u, waitHandles, FALSE, timeoutMs);
                    if (waited == WAIT_OBJECT_0)
                    {
                        if (operation.cancel.load())
                        {
                            probe->abandoned.store(true, std::memory_order_release);
                            return Cancelled;
                        }
                        probe->abandoned.store(true, std::memory_order_release);
                        const std::lock_guard lock(operation.mutex);
                        request = std::exchange(operation.request, {});
                        continue;
                    }
                    if (waited == WAIT_OBJECT_0 + 1u)
                    {
                        if (operation.cancel.load())
                            return Cancelled;
                        notice.result = probe->result;
                        if (SUCCEEDED(probe->result) && probe->configuration)
                        {
                            configuredPrinter    = probe->configuration->selection.printer;
                            desired              = probe->configuration->selection.paper;
                            notice.configuration = std::move(probe->configuration);
                        }
                        else if (! SamePrintName(configuredPrinter, activePrinter))
                        {
                            configuredPrinter.clear();
                            desired = letter;
                        }
                    }
                    else if (waited == WAIT_TIMEOUT)
                    {
                        if (operation.cancel.load())
                        {
                            probe->abandoned.store(true, std::memory_order_release);
                            return Cancelled;
                        }
                        pendingProbe          = std::move(probe);
                        notice.printerPending = true;
                        notice.result         = S_OK;
                        configuredPrinter.clear();
                        desired = letter;
                    }
                    else
                    {
                        probe->abandoned.store(true, std::memory_order_release);
                        return HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
            else
            {
                configuredPrinter.clear();
                desired = letter;
            }
        }
        else
        {
            initialRequest        = false;
            notice.printer        = activePrinter;
            notice.printerPending = (pendingProbe != nullptr);
        }

        if (! pages || desired != paper)
        {
            pages.reset();
            pages = std::make_unique<Pages>();
            paper = desired;
            RETURN_IF_FAILED(pages->Prepare(job.document, paper, wic.get(), operation.cancel));
        }
        notice.preview.emplace();
        notice.preview->fallbackPaper = configuredPrinter.empty();
        RETURN_IF_FAILED(pages->Preview(std::clamp(currentPage, 1u, pages->Count()), notice.preview.value()));
        if (operation.Post(std::move(notice)) == PostResult::Cancelled)
            return Cancelled;

        for (;;)
        {
            if (operation.cancel.load())
                return Cancelled;
            if (! pendingProbe)
            {
                if (! operation.requested.wait())
                    return E_FAIL;
                if (operation.cancel.load())
                    return Cancelled;
                const std::lock_guard lock(operation.mutex);
                request = std::exchange(operation.request, {});
                break;
            }
            const HANDLE waitHandles[2]{operation.requested.get(), pendingProbe->done.get()};
            const DWORD waited = WaitForMultipleObjects(2u, waitHandles, FALSE, INFINITE);
            if (waited == WAIT_OBJECT_0)
            {
                if (operation.cancel.load())
                    return Cancelled;
                const std::lock_guard lock(operation.mutex);
                request = std::exchange(operation.request, {});
                break;
            }
            if (waited == WAIT_OBJECT_0 + 1u)
            {
                if (operation.cancel.load())
                    return Cancelled;
                const auto completed = std::move(pendingProbe);
                Notice resolved{.kind           = Notice::Kind::Preview,
                                .result         = completed->result,
                                .stage          = Stage::ConfigurePrinter,
                                .requestId      = currentRequestId,
                                .printerEpoch   = currentEpoch,
                                .printerPending = false,
                                .printer        = completed->printer};
                auto resolvedDesired = paper;
                if (SUCCEEDED(completed->result) && completed->configuration)
                {
                    configuredPrinter      = completed->configuration->selection.printer;
                    resolvedDesired        = completed->configuration->selection.paper;
                    resolved.configuration = std::move(completed->configuration);
                }
                else if (! SamePrintName(configuredPrinter, completed->printer))
                {
                    configuredPrinter.clear();
                    resolvedDesired = letter;
                }
                if (! pages || resolvedDesired != paper)
                {
                    pages.reset();
                    pages = std::make_unique<Pages>();
                    paper = resolvedDesired;
                    RETURN_IF_FAILED(pages->Prepare(job.document, paper, wic.get(), operation.cancel));
                }
                resolved.preview.emplace();
                resolved.preview->fallbackPaper = configuredPrinter.empty();
                RETURN_IF_FAILED(pages->Preview(std::clamp(currentPage, 1u, pages->Count()), resolved.preview.value()));
                if (operation.Post(std::move(resolved)) == PostResult::Cancelled)
                    return Cancelled;
                continue;
            }
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
}

void CALLBACK RunPrint(PTP_CALLBACK_INSTANCE, void* context) noexcept
{
    ProcessWorkLease lease = std::move(static_cast<Work*>(context)->lease);
    const std::unique_ptr<Work> work(static_cast<Work*>(context));
    Notice finished{.kind = Notice::Kind::Finished};
    try
    {
        finished.result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(finished.result))
        {
            const wil::unique_couninitialize_call apartment;
            const DxUi::Failure::Scope failureScope;
            finished.result = RunSession(*work, finished);
        }
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        // Invalidated or unavailable output paths fail the job at the worker boundary.
        finished.result = HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
    }
    catch (const std::exception&)
    {
        DxUi::Failure::FailUnexpected();
    }
    work->job = Job{};
    (void)work->operation->Post(std::move(finished));
}
} // namespace

HRESULT Start(HWND owner, UINT message, Job job, std::shared_ptr<Operation>& operation)
{
    auto staged = std::make_shared<Operation>();
    RETURN_IF_FAILED(staged->requested.create(wil::EventOptions::None));
    RETURN_IF_FAILED(staged->fileChosen.create(wil::EventOptions::ManualReset));
    staged->window  = owner;
    staged->message = message;
    auto work       = std::make_unique<Work>();
    RETURN_IF_FAILED(ProcessWork::Acquire(work->lease));
    work->operation = staged;
    work->job       = std::move(job);
    auto* admitted  = work.release();
    if (! TrySubmitThreadpoolCallback(RunPrint, admitted, nullptr))
    {
        const DWORD error = GetLastError();
        work.reset(admitted);
        return HRESULT_FROM_WIN32(error);
    }
    operation = std::move(staged);
    return S_OK;
}

std::optional<Notice> TakeNotice(const std::shared_ptr<Operation>& operation)
{
    if (! operation)
        return std::nullopt;
    const std::lock_guard lock(operation->mutex);
    return std::exchange(operation->notice, std::nullopt);
}

void RequestPreview(Operation& operation, unsigned page) noexcept
{
    const std::lock_guard lock(operation.mutex);
    ++operation.activeRequestId;
    if (operation.notice && operation.notice->kind == Notice::Kind::Preview)
    {
        std::optional<Notice> preservedDestinations;
        if (operation.notice->stage == Stage::ConfigurePrinter && operation.notice->printerEpoch == operation.activePrinterEpoch)
        {
            operation.pendingConfig = Notice{.kind           = Notice::Kind::Preview,
                                             .result         = operation.notice->result,
                                             .stage          = operation.notice->stage,
                                             .printerEpoch   = operation.notice->printerEpoch,
                                             .printerPending = operation.notice->printerPending,
                                             .printer        = std::move(operation.notice->printer),
                                             .configuration  = std::move(operation.notice->configuration)};
        }
        if (operation.notice->destinations)
        {
            preservedDestinations = Notice{.kind         = Notice::Kind::Destinations,
                                           .printer      = operation.pendingConfig ? operation.pendingConfig->printer : std::wstring{},
                                           .destinations = std::move(operation.notice->destinations)};
        }
        operation.notice = std::move(preservedDestinations);
    }
    operation.request = {.id = operation.activeRequestId, .printerEpoch = operation.activePrinterEpoch, .page = page};
    operation.requested.SetEvent();
}

void RequestPrinter(Operation& operation, unsigned page, std::wstring printer, std::optional<PrinterSettings> settings) noexcept
{
    const std::lock_guard lock(operation.mutex);
    ++operation.activeRequestId;
    ++operation.activePrinterEpoch;
    operation.pendingConfig.reset();
    if (operation.notice && operation.notice->kind == Notice::Kind::Preview)
    {
        if (operation.notice->destinations)
            operation.notice = Notice{.kind = Notice::Kind::Destinations, .destinations = std::move(operation.notice->destinations)};
        else
            operation.notice.reset();
    }
    operation.request = {.id           = operation.activeRequestId,
                         .printerEpoch = operation.activePrinterEpoch,
                         .page         = page,
                         .printer      = std::move(printer),
                         .settings     = std::move(settings)};
    operation.requested.SetEvent();
}

void RequestPrint(Operation& operation, Selection selection) noexcept
{
    const std::lock_guard lock(operation.mutex);
    ++operation.activeRequestId;
    operation.pendingConfig.reset();
    if (operation.notice && operation.notice->kind == Notice::Kind::Preview)
        operation.notice.reset();
    operation.request = {.id = operation.activeRequestId, .printerEpoch = operation.activePrinterEpoch, .selection = std::move(selection)};
    operation.requested.SetEvent();
}

void ChooseFile(const std::shared_ptr<Operation>& operation, HRESULT result, std::filesystem::path destination) noexcept
{
    const std::lock_guard lock(operation->mutex);
    operation->fileStatus = result;
    operation->destination.swap(destination);
    operation->fileChosen.SetEvent();
}

void Stop(std::shared_ptr<Operation>& operation) noexcept
{
    if (const auto job = std::exchange(operation, nullptr))
    {
        const std::lock_guard lock(job->mutex);
        job->window = nullptr;
        job->cancel.store(true);
        job->requested.SetEvent();
        job->fileChosen.SetEvent();
    }
}
} // namespace RedTilde::Printing
