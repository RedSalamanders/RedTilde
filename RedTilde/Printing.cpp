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
#include <array>
#include <atomic>
#include <cmath>
#include <commdlg.h>
#include <cstring>
#include <d3d11.h>
#include <documenttarget.h>
#include <format>
#include <mutex>
#include <objbase.h>
#include <ole2.h>
#include <prntvpt.h>
#include <shlwapi.h>
#include <span>
#include <wil/com.h>
#include <wil/result.h>
#include <wincodec.h>
#include <winspool.h>

namespace RedTilde::Printing
{
namespace
{
constexpr unsigned MaximumPages = static_cast<unsigned>(DxUi::RichText::Document::MaximumUnits) + 1u;
constexpr float MaximumPreviewEdge = 1600.f;
constexpr std::size_t MaximumImageBytes = 64u * 1024u * 1024u;
constexpr HRESULT Cancelled = HRESULT_FROM_WIN32(ERROR_CANCELLED);

HRESULT LoadPrintImports(const char *library) noexcept
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

bool Equal(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() && (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
                                                                                static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}

bool ValidPaper(const Paper &paper) noexcept
{
    const auto &size = paper.size;
    const auto &area = paper.printable;
    return std::isfinite(size.width) && std::isfinite(size.height) && size.width > 0.f && size.height > 0.f && size.width <= 16384.f &&
           size.height <= 16384.f && std::isfinite(area.left) && std::isfinite(area.top) && std::isfinite(area.right) && std::isfinite(area.bottom) &&
           area.left >= 0.f && area.top >= 0.f && area.right <= size.width && area.bottom <= size.height && area.right - area.left >= 1.f &&
           area.bottom - area.top >= 1.f;
}

std::optional<std::wstring> DeviceName(std::span<const wchar_t> storage, WORD offset)
{
    if (offset < sizeof(DEVNAMES) / sizeof(wchar_t) || offset >= storage.size())
        return std::nullopt;
    const auto tail = storage.subspan(offset);
    const auto end = std::ranges::find(tail, L'\0');
    if (end == tail.end())
        return std::nullopt;
    return std::wstring(tail.begin(), end);
}

struct Band
{
    float top{}, bottom{};
};

struct Page
{
    float top{}, height{}, scale = 1.f;
};

void MergeBands(std::vector<Band> &bands)
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
    Pages() = default;
    Pages(const Pages &) = delete;
    Pages &operator=(const Pages &) = delete;
    HRESULT Prepare(const Document &document, const Paper &paper, IWICImagingFactory *wic, const std::atomic_bool &cancel);
    HRESULT Preview(unsigned number, PreviewPage &output);
    HRESULT Record(unsigned number, ID2D1CommandList **output);
    ID2D1DeviceContext &Context() const noexcept
    {
        return *host_.Controls().GetDeviceContext();
    }
    [[nodiscard]] unsigned Count() const noexcept
    {
        return static_cast<unsigned>(pages_.size());
    }

  private:
    HRESULT FinishPreparation(const std::atomic_bool &cancel);
    HRESULT ResolveImages(const Document &document, IWICImagingFactory *wic, const std::atomic_bool &cancel);
    HRESULT Paginate(const std::atomic_bool &cancel);
    std::optional<Band> SourceBand(DxUi::RichText::MarkdownRange source) const;

    DxUi::EmbeddedHost host_;
    DxUi::RichTextEditor *editor_{};
    UINT width_{}, height_{};
    Paper paper_{};
    D2D1_RECT_F content_{};
    std::vector<Page> pages_;
    unsigned previewed_{};
    wil::com_ptr<ID2D1CommandList> previewCommands_;
};

HRESULT Pages::FinishPreparation(const std::atomic_bool &cancel)
{
    for (unsigned step = 0; step < 4096u; ++step)
    {
        if (cancel.load())
            return Cancelled;
        RETURN_IF_FAILED(host_.Prepare(width_, height_, 96.f));
        if (!host_.NeedsPreparation())
            return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
}

HRESULT Pages::ResolveImages(const Document &document, IWICImagingFactory *wic, const std::atomic_bool &cancel)
{
    using namespace DxUi::RichText;
    std::vector<RichImageRequest> requests;
    editor_->TakePendingImageRequests(requests);
    std::size_t retained{};
    for (const auto &request : requests)
    {
        if (cancel.load())
            return Cancelled;
        std::shared_ptr<const RichImage> pixels;
        if (request.SourceKind() == RichImageSourceKind::LocalFile && !request.FilePath().starts_with(L"\\\\"))
        {
            std::string encoded;
            if (SUCCEEDED(ReadImageFileBytes(request.FilePath(), encoded, &cancel)))
                (void)RichImage::Decode(wic, std::as_bytes(std::span(encoded)), pixels, &cancel);
        }
        else
        {
            const auto ready =
                std::ranges::find_if(document.networkImages, [&](const auto &image)
                                     { return image.request.SourceKind() == request.SourceKind() && image.request.Source() == request.Source(); });
            if (ready != document.networkImages.end())
                pixels = ready->pixels;
        }
        if (pixels && pixels->RetainedBytes() > MaximumImageBytes - retained)
            pixels.reset();
        if (pixels)
            retained += pixels->RetainedBytes();
        (void)editor_->CompleteImageRequest(request, std::move(pixels));
    }
    return requests.empty() ? S_OK : FinishPreparation(cancel);
}

HRESULT Pages::Prepare(const Document &document, const Paper &paper, IWICImagingFactory *wic, const std::atomic_bool &cancel)
{
    constexpr float margin = 72.f;
    if (!ValidPaper(paper))
        return E_INVALIDARG;
    paper_ = paper;
    const auto &area = paper.printable;
    const float insetX = std::min(margin, paper.size.width / 4.f), insetY = std::min(margin, paper.size.height / 4.f);
    content_ = {std::max(area.left, insetX), std::max(area.top, insetY), std::min(area.right, paper.size.width - insetX),
                std::min(area.bottom, paper.size.height - insetY)};
    if (content_.right - content_.left < 1.f || content_.bottom - content_.top < 1.f)
        content_ = area;
    width_ = static_cast<UINT>(content_.right - content_.left);
    // The persistent surface is used only to prepare native resources. The
    // document extent and the recorded page lists are independent of its height.
    height_ = static_cast<UINT>(std::clamp(content_.bottom - content_.top, 1.f, 512.f));
    wil::com_ptr<ID3D11Device> device;
    RETURN_IF_FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                                       device.put(), nullptr, nullptr));
    std::shared_ptr<DxUi::GraphicsDevice> graphics;
    RETURN_IF_FAILED(DxUi::GraphicsDevice::Create(device.get(), graphics));
    RETURN_IF_FAILED(host_.Attach(std::move(graphics)));
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
    appearance.paddingDip = 0.f;
    appearance.bodyFontSizeDip = 14.f;
    appearance.codeFontSizeDip = 13.f;
    appearance.autoParagraphDirection = true;
    appearance.strikeCompletedTasks = document.strikeCompletedTasks;
    appearance.markdownPresentation = MarkdownPresentationStyle::Subtle;
    editor->SetAppearance(appearance);
    editor_ = editor.get();
    host_.Controls().SetRoot(std::move(editor));
    RETURN_IF_FAILED(FinishPreparation(cancel));
    RETURN_IF_FAILED(ResolveImages(document, wic, cancel));
    return Paginate(cancel);
}

std::optional<Band> Pages::SourceBand(DxUi::RichText::MarkdownRange source) const
{
    const auto rectangles = editor_->TryGetTextInputRangeRects(host_.Controls(), source.first, source.last);
    if (!rectangles || rectangles->empty())
        return std::nullopt;
    Band band{rectangles->front().top, rectangles->front().bottom};
    for (const auto rectangle : rectangles.value())
    {
        band.top = std::min(band.top, rectangle.top);
        band.bottom = std::max(band.bottom, rectangle.bottom);
    }
    return band;
}

HRESULT Pages::Paginate(const std::atomic_bool &cancel)
{
    using DxUi::RichText::SemanticRole;
    const auto scroll = editor_->GetScrollInfo();
    const auto viewport = editor_->TryGetTextInputViewportRect();
    if (!scroll || !viewport || scroll->offsetDip != 0.f || std::abs(viewport->right - viewport->left - static_cast<float>(width_)) > 0.01f)
        return E_UNEXPECTED;
    const auto rectangles = editor_->TryGetTextInputRangeRects(host_.Controls(), 0, editor_->Snapshot().Length());
    if (!rectangles)
        return E_UNEXPECTED;
    const float pageHeight = content_.bottom - content_.top;
    std::vector<Band> bands;
    bands.reserve(rectangles->size());
    for (const auto rectangle : rectangles.value())
        if (rectangle.bottom > rectangle.top && rectangle.right > rectangle.left)
            bands.push_back({std::max(0.f, rectangle.top - viewport->top), rectangle.bottom - viewport->top});
    MergeBands(bands);

    // The source rectangles give physical text lines. Add whole rows and
    // rendered objects where possible, and attach headings to the next line.
    std::vector<Band> keepTogether;
    if (const auto semantic = editor_->GetAcknowledgedSemanticDocument(host_.Controls()))
        for (const auto &element : semantic->Elements())
        {
            if (cancel.load())
                return Cancelled;
            const auto role = element.role;
            if (role != SemanticRole::Heading && role != SemanticRole::Row && role != SemanticRole::Image && role != SemanticRole::Diagram &&
                role != SemanticRole::Math)
                continue;
            auto bounds = SourceBand(element.source);
            if (!bounds)
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

    float top = 0.f;
    float lastBottom = 0.f;
    for (const auto band : bands)
    {
        if (cancel.load())
            return Cancelled;
        if (band.bottom - top > pageHeight && lastBottom > top)
        {
            const float end = std::min(band.top, top + pageHeight);
            pages_.push_back({top, end - top});
            top = end;
        }
        if (band.bottom - top > pageHeight)
            top = band.top;
        if (band.bottom - band.top > pageHeight)
        {
            const float height = band.bottom - band.top;
            pages_.push_back({band.top, height, pageHeight / height});
            top = band.bottom;
        }
        lastBottom = band.bottom;
        if (pages_.size() >= MaximumPages)
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    if (lastBottom > top || pages_.empty())
        pages_.push_back({top, std::max(1.f, std::min(pageHeight, scroll->contentHeightDip - top))});
    return S_OK;
}

HRESULT Pages::Record(unsigned number, ID2D1CommandList **output)
{
    if (number == previewed_)
    {
        previewCommands_.copy_to(output);
        return S_OK;
    }
    const auto &page = pages_[number - 1u];
    auto &context = Context();
    wil::com_ptr<ID2D1CommandList> commands;
    RETURN_IF_FAILED(context.CreateCommandList(commands.put()));
    wil::com_ptr<ID2D1SolidColorBrush> ink, rule, shade;
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), ink.put()));
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(0.33f, 0.33f, 0.33f), rule.put()));
    RETURN_IF_FAILED(context.CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.95f, 0.95f), shade.put()));
    wil::com_ptr<ID2D1Image> previous;
    context.GetTarget(previous.put());
    const auto restore = wil::scope_exit(
        [&] noexcept
        {
            context.SetTarget(previous.get());
            context.SetTransform(D2D1::Matrix3x2F::Identity());
        });
    context.SetTarget(commands.get());
    context.SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    context.SetTransform(D2D1::Matrix3x2F::Scale(page.scale, page.scale) * D2D1::Matrix3x2F::Translation(content_.left, content_.top));
    context.BeginDraw();
    const DxUi::RichText::ExportDrawing drawing{.text = ink.get(), .decoration = rule.get(), .fill = shade.get(), .quoteFill = shade.get()};
    const bool drawn = editor_->TryDrawPreparedPage(host_.Controls(), {0.f, 0.f, static_cast<float>(width_), page.height}, page.top, drawing);
    RETURN_IF_FAILED(context.EndDraw());
    if (!drawn)
        return E_FAIL;
    context.SetTarget(nullptr);
    RETURN_IF_FAILED(commands->Close());
    *output = commands.detach();
    return S_OK;
}

HRESULT Pages::Preview(unsigned number, PreviewPage &output)
{
    wil::com_ptr<ID2D1CommandList> commands;
    RETURN_IF_FAILED(Record(number, commands.put()));
    auto &context = Context();
    const float scale = std::min(1.5f, MaximumPreviewEdge / std::max(paper_.size.width, paper_.size.height));
    const auto extent = [scale](float size) { return static_cast<UINT>(std::clamp(std::ceil(size * scale), 1.f, MaximumPreviewEdge)); };
    const auto width = extent(paper_.size.width);
    const auto height = extent(paper_.size.height);
    const auto format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    const auto props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, format, 96.f, 96.f);
    wil::com_ptr<ID2D1Bitmap1> target, readback;
    RETURN_IF_FAILED(context.CreateBitmap({width, height}, nullptr, 0, props, target.put()));
    {
        wil::com_ptr<ID2D1Image> previous;
        context.GetTarget(previous.put());
        const auto restore = wil::scope_exit(
            [&] noexcept
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
    output.paper = paper_;
    output.count = Count();
    output.number = number;
    output.width = width;
    output.height = height;
    previewCommands_ = std::move(commands);
    previewed_ = number;
    return S_OK;
}

std::optional<Paper> DefaultPaper()
{
    if (FAILED(LoadPrintImports("WINSPOOL.DRV")))
        return std::nullopt;
    DWORD length{};
    (void)GetDefaultPrinterW(nullptr, &length);
    if (!length || length > 32768u)
        return std::nullopt;
    std::wstring name(length, L'\0');
    if (!GetDefaultPrinterW(name.data(), &length))
        return std::nullopt;
    const wil::unique_hdc dc(CreateICW(L"WINSPOOL", name.c_str(), nullptr, nullptr));
    Paper paper;
    return SUCCEEDED(ReadPaper(dc.get(), paper)) ? std::optional{paper} : std::nullopt;
}

HRESULT Ticket(Selection &selection, IStream **ticket)
{
    RETURN_IF_FAILED(LoadPrintImports("prntvpt.dll"));
    using Provider = wil::unique_any<HPTPROVIDER, decltype(&PTCloseProvider), PTCloseProvider>;
    Provider provider;
    RETURN_IF_FAILED(PTOpenProvider(selection.printer.c_str(), 1, provider.put()));
    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.put()));
    RETURN_IF_FAILED(PTConvertDevModeToPrintTicket(provider.get(), static_cast<ULONG>(selection.deviceMode.size()),
                                                   reinterpret_cast<DEVMODEW *>(selection.deviceMode.data()), kPTJobScope, stream.get()));
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));
    *ticket = stream.detach();
    return S_OK;
}

HRESULT DestinationFormat(Selection &selection, FileFormat &format)
{
    RETURN_IF_FAILED(LoadPrintImports("WINSPOOL.DRV"));
    using Printer = wil::unique_any<HANDLE, decltype(&ClosePrinter), ClosePrinter>;
    Printer printer;
    RETURN_IF_WIN32_BOOL_FALSE(OpenPrinterW(selection.printer.data(), printer.put(), nullptr));
    DWORD needed{};
    (void)GetPrinterW(printer.get(), 2, nullptr, 0, &needed);
    if (needed < sizeof(PRINTER_INFO_2W) || needed > 1024u * 1024u)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    std::vector<BYTE> bytes(needed);
    RETURN_IF_WIN32_BOOL_FALSE(GetPrinterW(printer.get(), 2, bytes.data(), needed, &needed));
    const auto name = reinterpret_cast<const PRINTER_INFO_2W *>(bytes.data())->pDriverName;
    if (name && Equal(name, L"Microsoft Print To PDF"))
        format = FileFormat::Pdf;
    else if (name && (Equal(name, L"Microsoft XPS Document Writer v4") || Equal(name, L"Microsoft XPS Document Writer")))
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

    bool operator==(const OutputIdentity &other) const noexcept
    {
        return id.VolumeSerialNumber == other.id.VolumeSerialNumber && std::ranges::equal(id.FileId.Identifier, other.id.FileId.Identifier) &&
               basic.LastWriteTime.QuadPart == other.basic.LastWriteTime.QuadPart && basic.ChangeTime.QuadPart == other.basic.ChangeTime.QuadPart &&
               standard.EndOfFile.QuadPart == other.standard.EndOfFile.QuadPart;
    }
};

HRESULT IdentifyOutput(const std::filesystem::path &path, std::optional<OutputIdentity> &identity)
{
    wil::unique_handle file(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!file)
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
    FileOutput() = default;
    FileOutput(const FileOutput &) = delete;
    FileOutput &operator=(const FileOutput &) = delete;
    ~FileOutput()
    {
        stream.reset();
        lease_.reset();
        if (!staged_.empty())
            (void)DeleteFileW(staged_.c_str());
    }

    HRESULT Open(const std::filesystem::path &path, const std::filesystem::path &source)
    {
        if (!path.is_absolute())
            return E_INVALIDARG;
        RETURN_IF_FAILED(IdentifyOutput(path, expected_));
        std::error_code error;
        if (!source.empty() && (Equal(path.native(), source.native()) || (expected_ && std::filesystem::equivalent(path, source, error))))
            return E_ACCESSDENIED;
        GUID id{};
        RETURN_IF_FAILED(CoCreateGuid(&id));
        wchar_t leaf[40]{};
        if (!StringFromGUID2(id, leaf, static_cast<int>(std::size(leaf))))
            return E_FAIL;
        auto staged = path.parent_path() / std::format(L".redtilde-print-{}.tmp", leaf);
        lease_.reset(CreateFileW(staged.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        RETURN_LAST_ERROR_IF(!lease_);
        staged_.swap(staged);
        return SHCreateStreamOnFileEx(staged_.c_str(), STGM_READWRITE | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, stream.put());
    }

    HRESULT Publish(const std::filesystem::path &path, const std::atomic_bool &cancel)
    {
        RETURN_IF_FAILED(stream->Commit(STGC_DEFAULT));
        STATSTG facts{};
        RETURN_IF_FAILED(stream->Stat(&facts, STATFLAG_NONAME));
        if (!facts.cbSize.QuadPart)
            return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        stream.reset();
        std::optional<OutputIdentity> current;
        RETURN_IF_FAILED(IdentifyOutput(path, current));
        if (current != expected_)
            return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        if (cancel.load())
            return Cancelled;
        lease_.reset();
        if (expected_)
            RETURN_IF_WIN32_BOOL_FALSE(ReplaceFileW(path.c_str(), staged_.c_str(), nullptr, 0, nullptr, nullptr));
        if (!expected_)
            RETURN_IF_WIN32_BOOL_FALSE(MoveFileExW(staged_.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH));
        staged_.clear();
        return S_OK;
    }

    wil::com_ptr<IStream> stream;

  private:
    std::filesystem::path staged_;
    wil::unique_handle lease_;
    std::optional<OutputIdentity> expected_;
};
} // namespace

bool Paper::operator==(const Paper &other) const noexcept
{
    return size.width == other.size.width && size.height == other.size.height && printable.left == other.printable.left &&
           printable.top == other.printable.top && printable.right == other.printable.right && printable.bottom == other.printable.bottom;
}

bool PageRangesFit(std::span<const PageRange> ranges, unsigned pageCount) noexcept
{
    return std::ranges::all_of(ranges,
                               [pageCount](const auto &range) { return range.first && range.last >= range.first && range.last <= pageCount; });
}

HRESULT ReadPaper(HDC dc, Paper &paper) noexcept
{
    if (!dc)
        return E_HANDLE;
    const int xDpi = GetDeviceCaps(dc, LOGPIXELSX), yDpi = GetDeviceCaps(dc, LOGPIXELSY);
    if (xDpi <= 0 || yDpi <= 0)
        return E_INVALIDARG;
    const float x = 96.f / static_cast<float>(xDpi), y = 96.f / static_cast<float>(yDpi);
    Paper selected;
    selected.size = {static_cast<float>(GetDeviceCaps(dc, PHYSICALWIDTH)) * x, static_cast<float>(GetDeviceCaps(dc, PHYSICALHEIGHT)) * y};
    const float left = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETX)) * x, top = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETY)) * y;
    selected.printable = {std::max(0.f, left), std::max(0.f, top),
                          std::min(selected.size.width, left + static_cast<float>(GetDeviceCaps(dc, HORZRES)) * x),
                          std::min(selected.size.height, top + static_cast<float>(GetDeviceCaps(dc, VERTRES)) * y)};
    if (!ValidPaper(selected))
        return E_INVALIDARG;
    paper = selected;
    return S_OK;
}

HRESULT Dialog::Choose(HWND owner, const DialogOptions &options, DialogResult &result)
{
    // The native property sheet and its printer extensions run in an OLE STA.
    RETURN_IF_FAILED(DxUi::Failure::InvokeNative(&OleInitialize, nullptr));
    const auto uninitialize = wil::scope_exit([]() noexcept { OleUninitialize(); });
    RETURN_IF_FAILED(LoadPrintImports("COMDLG32.dll"));
    std::array<PRINTPAGERANGE, 32> ranges{};
    PRINTDLGEXW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.hDevMode = deviceMode.release();
    dialog.hDevNames = deviceNames.release();
    dialog.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_EXCL_COPIESANDCOLLATE | PD_NOSELECTION | PD_NOWARNING;
    if (options.printToFile)
        dialog.Flags |= PD_PRINTTOFILE;
    if (options.currentPage)
        dialog.Flags |= PD_CURRENTPAGE;
    else if (options.rangeNeedsCorrection)
    {
        // Ask for a replacement range without displaying the old, invalid one.
        dialog.Flags |= PD_PAGENUMS;
    }
    else if (!options.ranges.empty() && options.ranges.size() <= ranges.size() && PageRangesFit(options.ranges, options.pageCount))
    {
        dialog.Flags |= PD_PAGENUMS;
        dialog.nPageRanges = static_cast<DWORD>(options.ranges.size());
        for (std::size_t index = 0; index < options.ranges.size(); ++index)
            ranges[index] = {options.ranges[index].first, options.ranges[index].last};
    }
    dialog.nMinPage = 1u;
    dialog.nMaxPage = options.pageCount;
    dialog.nMaxPageRanges = static_cast<DWORD>(ranges.size());
    dialog.lpPageRanges = ranges.data();
    dialog.nCopies = 1u;
    dialog.nStartPage = START_PAGE_GENERAL;
    const HRESULT shown = DxUi::Failure::InvokeNative(&PrintDlgExW, &dialog);
    deviceMode.reset(dialog.hDevMode);
    deviceNames.reset(dialog.hDevNames);
    const wil::unique_hdc dc(dialog.hDC);
    RETURN_IF_FAILED(shown);
    if (dialog.dwResultAction != PD_RESULT_PRINT && dialog.dwResultAction != PD_RESULT_APPLY)
        return S_FALSE;
    const auto modeBytes = GlobalSize(deviceMode.get());
    const auto namesBytes = GlobalSize(deviceNames.get());
    const wil::unique_hglobal_locked mode(deviceMode.get()), names(deviceNames.get());
    if (!mode || !names || modeBytes < sizeof(DEVMODEW) || namesBytes < sizeof(DEVNAMES) || dialog.nPageRanges > ranges.size())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto *devMode = static_cast<const DEVMODEW *>(mode.get());
    const auto total = static_cast<std::size_t>(devMode->dmSize) + devMode->dmDriverExtra;
    if (devMode->dmSize < sizeof(DEVMODEW) || total > modeBytes)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    const auto *devNames = static_cast<const DEVNAMES *>(names.get());
    const auto storage = std::span(static_cast<const wchar_t *>(names.get()), namesBytes / sizeof(wchar_t));
    auto printer = DeviceName(storage, devNames->wDeviceOffset);
    auto port = DeviceName(storage, devNames->wOutputOffset);
    if (!printer || printer->empty() || !port)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    Selection selected;
    if (dc)
    {
        RETURN_IF_FAILED(ReadPaper(dc.get(), selected.paper));
    }
    else
    {
        // Some drivers return updated settings on Apply without a context.
        const wil::unique_hdc info(CreateICW(L"WINSPOOL", printer->c_str(), nullptr, devMode));
        RETURN_IF_FAILED(ReadPaper(info.get(), selected.paper));
    }
    selected.printer = std::move(printer.value());
    const bool printToFile = (dialog.Flags & PD_PRINTTOFILE) != 0u;
    selected.port = printToFile ? L"FILE:" : std::move(port.value());
    selected.deviceMode.resize(total);
    std::memcpy(selected.deviceMode.data(), mode.get(), total);
    const bool currentPage = (dialog.Flags & PD_CURRENTPAGE) != 0u;
    if (!currentPage && (dialog.Flags & PD_PAGENUMS) != 0u)
    {
        if (!dialog.nPageRanges)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        for (const auto &range : std::span(ranges).first(dialog.nPageRanges))
            selected.ranges.push_back({range.nFromPage, range.nToPage});
    }
    result = {std::move(selected), dialog.dwResultAction == PD_RESULT_PRINT, currentPage, printToFile};
    return S_OK;
}

namespace
{
struct Request
{
    unsigned page = 1u;
    std::optional<Paper> paper;
    std::optional<Selection> selection;
};
} // namespace

struct Operation
{
    Operation() = default;
    Operation(const Operation &) = delete;
    Operation &operator=(const Operation &) = delete;
    std::mutex mutex;
    HWND window{};
    UINT message{};
    wil::unique_event_nothrow requested, fileChosen;
    std::atomic_bool cancel{};
    std::optional<Notice> notice;
    Request request;
    HRESULT fileStatus = Cancelled;
    std::filesystem::path destination;

    bool Post(Notice next) noexcept
    {
        const std::lock_guard lock(mutex);
        if (!window || cancel.load())
            return false;
        notice = std::move(next);
        return PostMessageW(window, message, 0, 0) != FALSE;
    }
};

namespace
{
struct Work
{
    Work() = default;
    Work(const Work &) = delete;
    Work &operator=(const Work &) = delete;
    ProcessWorkLease lease;
    std::shared_ptr<Operation> operation;
    Job job;
};

HRESULT Submit(Work &work, Selection &selection, Pages &pages, IWICImagingFactory *wic, Notice &finished)
{
    auto &operation = *work.operation;
    auto &job = work.job;
    if (operation.cancel.load())
        return Cancelled;
    const auto count = pages.Count();
    const auto &ranges = selection.ranges;
    if (!PageRangesFit(ranges, count))
    {
        finished.invalidRangePageCount = count;
        return E_INVALIDARG;
    }
    const auto forPages = [&](auto &&submitPage) -> HRESULT
    {
        for (unsigned page = 1u; page <= count; ++page)
        {
            if (operation.cancel.load())
                return Cancelled;
            if (!ranges.empty() && std::ranges::none_of(ranges, [page](const auto &range) { return page >= range.first && page <= range.last; }))
                continue;
            wil::com_ptr<ID2D1CommandList> commands;
            RETURN_IF_FAILED(pages.Record(page, commands.put()));
            RETURN_IF_FAILED(submitPage(page, *commands));
        }
        return S_OK;
    };
#if defined(REDTILDE_ENABLE_TESTS)
    if (job.pageForTest)
    {
        RETURN_IF_FAILED(forPages(job.pageForTest));
        return job.finishForTest ? job.finishForTest() : S_OK;
    }
#endif
    std::filesystem::path destination;
    if (Equal(selection.port, L"PORTPROMPT:") || Equal(selection.port, L"FILE:"))
    {
        FileFormat format{};
        RETURN_IF_FAILED(DestinationFormat(selection, format));
        if (!operation.Post({.kind = Notice::Kind::ChooseFile, .format = format}))
            return Cancelled;
        // Close always signals this event. The worker retains only immutable
        // inputs and never shows a dialog itself.
        if (!operation.fileChosen.wait())
            return E_FAIL;
        if (operation.cancel.load())
            return Cancelled;
        const std::lock_guard lock(operation.mutex);
        RETURN_IF_FAILED(operation.fileStatus);
        destination = std::move(operation.destination);
        if (!destination.is_absolute())
            return E_INVALIDARG;
    }
    if (operation.cancel.load())
        return Cancelled;
    wil::com_ptr<IStream> ticket;
    RETURN_IF_FAILED(Ticket(selection, ticket.put()));
    FileOutput output;
    if (!destination.empty())
        RETURN_IF_FAILED(output.Open(destination, job.document.path));
    wil::com_ptr<IPrintDocumentPackageTargetFactory> factory;
    RETURN_IF_FAILED(CoCreateInstance(__uuidof(PrintDocumentPackageTargetFactory), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
    wil::com_ptr<IPrintDocumentPackageTarget> target;
    RETURN_IF_FAILED(factory->CreateDocumentPackageTargetForPrintJob(selection.printer.c_str(), job.document.title.c_str(), output.stream.get(),
                                                                     ticket.get(), target.put()));
    auto cancelOnFailure = wil::scope_exit([&] noexcept { (void)target->Cancel(); });
    wil::com_ptr<ID2D1Device> device;
    pages.Context().GetDevice(device.put());
    wil::com_ptr<ID2D1PrintControl> print;
    RETURN_IF_FAILED(device->CreatePrintControl(wic, target.get(), nullptr, print.put()));
    RETURN_IF_FAILED(forPages([&](unsigned, ID2D1CommandList &commands) { return print->AddPage(&commands, selection.paper.size, nullptr); }));
    if (operation.cancel.load())
        return Cancelled;
    RETURN_IF_FAILED(print->Close());
    cancelOnFailure.release();
    print.reset();
    target.reset();
    if (!destination.empty())
    {
        RETURN_IF_FAILED(output.Publish(destination, operation.cancel));
        finished.savedFile = true;
    }
    return S_OK;
}

HRESULT RunSession(Work &work, Notice &finished)
{
    auto &operation = *work.operation;
    if (operation.cancel.load())
        return Cancelled;
    wil::com_ptr<IWICImagingFactory> wic;
    RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.put())));
    const auto initial = work.job.initialPaper ? work.job.initialPaper : DefaultPaper();
    constexpr Paper letter{{816.f, 1056.f}, {0.f, 0.f, 816.f, 1056.f}};
    auto paper = initial.value_or(letter);
    bool fallback = !initial;
    auto pages = std::make_unique<Pages>();
    RETURN_IF_FAILED(pages->Prepare(work.job.document, paper, wic.get(), operation.cancel));
    Request request;
    for (;;)
    {
        if (operation.cancel.load())
            return Cancelled;
        if (request.selection)
        {
            if (request.selection->paper != paper)
                return E_INVALIDARG;
            return Submit(work, request.selection.value(), *pages, wic.get(), finished);
        }
        if (request.paper)
        {
            if (request.paper.value() != paper)
            {
                pages.reset();
                pages = std::make_unique<Pages>();
                paper = request.paper.value();
                RETURN_IF_FAILED(pages->Prepare(work.job.document, paper, wic.get(), operation.cancel));
            }
            fallback = false;
        }
        Notice notice{.kind = Notice::Kind::Preview, .preview = PreviewPage{.fallbackPaper = fallback}};
        RETURN_IF_FAILED(pages->Preview(std::clamp(request.page, 1u, pages->Count()), notice.preview.value()));
        if (!operation.Post(std::move(notice)))
            return Cancelled;
        if (!operation.requested.wait())
            return E_FAIL;
        const std::lock_guard lock(operation.mutex);
        request = std::exchange(operation.request, {});
    }
}

void CALLBACK RunPrint(PTP_CALLBACK_INSTANCE, void *context) noexcept
{
    ProcessWorkLease lease = std::move(static_cast<Work *>(context)->lease);
    const std::unique_ptr<Work> work(static_cast<Work *>(context));
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
    catch (const std::bad_alloc &)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::filesystem::filesystem_error &error)
    {
        // Invalidated or unavailable output paths fail the job at the worker boundary.
        finished.result = HRESULT_FROM_WIN32(static_cast<DWORD>(error.code().value()));
    }
    catch (const std::exception &)
    {
        DxUi::Failure::FailUnexpected();
    }
    work->job = Job{};
    (void)work->operation->Post(std::move(finished));
}
} // namespace

HRESULT Start(HWND owner, UINT message, Job job, std::shared_ptr<Operation> &operation)
{
    auto staged = std::make_shared<Operation>();
    RETURN_IF_FAILED(staged->requested.create(wil::EventOptions::None));
    RETURN_IF_FAILED(staged->fileChosen.create(wil::EventOptions::ManualReset));
    staged->window = owner;
    staged->message = message;
    auto work = std::make_unique<Work>();
    RETURN_IF_FAILED(ProcessWork::Acquire(work->lease));
    work->operation = staged;
    work->job = std::move(job);
    auto *admitted = work.release();
    if (!TrySubmitThreadpoolCallback(RunPrint, admitted, nullptr))
    {
        const DWORD error = GetLastError();
        work.reset(admitted);
        return HRESULT_FROM_WIN32(error);
    }
    operation = std::move(staged);
    return S_OK;
}

std::optional<Notice> TakeNotice(const std::shared_ptr<Operation> &operation)
{
    if (!operation)
        return std::nullopt;
    const std::lock_guard lock(operation->mutex);
    return std::exchange(operation->notice, std::nullopt);
}

void RequestPreview(Operation &operation, unsigned page, std::optional<Paper> paper) noexcept
{
    const std::lock_guard lock(operation.mutex);
    operation.request = {.page = page, .paper = paper};
    operation.requested.SetEvent();
}

void RequestPrint(Operation &operation, Selection selection) noexcept
{
    const std::lock_guard lock(operation.mutex);
    operation.request = {.selection = std::move(selection)};
    operation.requested.SetEvent();
}

void ChooseFile(const std::shared_ptr<Operation> &operation, HRESULT result, std::filesystem::path destination) noexcept
{
    const std::lock_guard lock(operation->mutex);
    operation->fileStatus = result;
    operation->destination.swap(destination);
    operation->fileChosen.SetEvent();
}

void Stop(std::shared_ptr<Operation> &operation) noexcept
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
