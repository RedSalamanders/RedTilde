#include "Application.Tests.DesignRefresh.h"
#include "RedTilde/Application.h"
#include "RedTilde/KeyboardState.h"
#include "Support/Support.Tests.BitmapAssets.h"
#include "Support/Support.Tests.Foreground.h"
#include "Support/Support.Tests.Images.h"
#include "Support/Support.Tests.TextServices.h"
#include "res/Resource.h"
#include <UIAutomation.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <objbase.h>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <wil/com.h>
#include <wil/resource.h>
#include <wincodec.h>
#include <windowsx.h>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view message)
{
    if (! condition)
        throw std::runtime_error(std::string(message));
}
void Success(HRESULT result, std::string_view message)
{
    Require(SUCCEEDED(result), message);
}
bool Near(float left, float right) noexcept
{
    return std::abs(left - right) <= 1.1f;
}
bool Inside(D2D1_RECT_F inner, D2D1_RECT_F outer) noexcept
{
    return inner.left >= outer.left - 0.1f && inner.top >= outer.top - 0.1f && inner.right <= outer.right + 0.1f && inner.bottom <= outer.bottom + 0.1f &&
           inner.right > inner.left && inner.bottom > inner.top;
}
bool Overlap(D2D1_RECT_F left, D2D1_RECT_F right) noexcept
{
    return std::min(left.right, right.right) > std::max(left.left, right.left) + 0.1f &&
           std::min(left.bottom, right.bottom) > std::max(left.top, right.top) + 0.1f;
}
void Color(D2D1_COLOR_F actual, unsigned rgb, std::string_view message)
{
    Require(std::abs(actual.r - static_cast<float>((rgb >> 16) & 255) / 255.f) < 0.001f &&
                std::abs(actual.g - static_cast<float>((rgb >> 8) & 255) / 255.f) < 0.001f &&
                std::abs(actual.b - static_cast<float>(rgb & 255) / 255.f) < 0.001f && actual.a == 1,
            message);
}
void CheckBrandIconPixels(HICON icon, unsigned requestedWidth, unsigned requestedHeight, bool expectNativeFrame)
{
    Require(icon != nullptr, "use the compiled RedTilde icon resource");

    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Require(SUCCEEDED(initialized) || initialized == RPC_E_CHANGED_MODE, "use the test's existing COM apartment for icon decoding");
    const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
    wil::com_ptr<IWICImagingFactory> factory;
    Success(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())),
            "create a WIC decoder for the icon resource");
    wil::com_ptr<IWICBitmap> bitmap;
    Success(factory->CreateBitmapFromHICON(icon, bitmap.put()), "decode the requested icon frame");
    UINT width{}, height{};
    Success(bitmap->GetSize(&width, &height), "read decoded icon geometry");
    Require(width == requestedWidth && height == requestedHeight, "icon resource decodes to the requested geometry");

    wil::com_ptr<IWICFormatConverter> converter;
    Success(factory->CreateFormatConverter(converter.put()), "create an alpha-preserving icon pixel view");
    Success(converter->Initialize(bitmap.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "convert icon pixels to explicit BGRA alpha");
    std::vector<BYTE> pixels(static_cast<std::size_t>(requestedWidth) * requestedHeight * 4);
    Success(converter->CopyPixels(nullptr, requestedWidth * 4, static_cast<UINT>(pixels.size()), pixels.data()), "read decoded icon alpha pixels");
    std::size_t transparent{}, opaque{}, ink{};
    UINT left = requestedWidth, top = requestedHeight, right{}, bottom{};
    for (UINT y = 0; y < requestedHeight; ++y)
    {
        for (UINT x = 0; x < requestedWidth; ++x)
        {
            const auto alpha = pixels[(static_cast<std::size_t>(y) * requestedWidth + x) * 4 + 3];
            transparent += alpha == 0;
            opaque += alpha == 255;
            if (alpha != 0)
            {
                ++ink;
                left   = std::min(left, x);
                top    = std::min(top, y);
                right  = std::max(right, x + 1);
                bottom = std::max(bottom, y + 1);
            }
        }
    }
    Require(
        transparent > 0 && ink > 0 && (! expectNativeFrame || opaque > 0),
        std::format(
            "icon {}x{} alpha: native={}, transparent={}, opaque={}, ink={}", requestedWidth, requestedHeight, expectNativeFrame, transparent, opaque, ink));
    if (requestedWidth <= 32 && requestedHeight <= 32)
    {
        // Preserve the existing small-icon legibility oracle. Larger frames have
        // less antialias coverage, so their ink area is not the same percentage.
        const auto pixelCount = static_cast<std::size_t>(requestedWidth) * requestedHeight;
        Require(ink > pixelCount / 4 && opaque < pixelCount && right - left >= requestedWidth * 3 / 5 && bottom - top >= requestedHeight * 2 / 5,
                std::format("icon {}x{} silhouette: transparent={}, opaque={}, ink={}, width={}, height={}",
                            requestedWidth,
                            requestedHeight,
                            transparent,
                            opaque,
                            ink,
                            right - left,
                            bottom - top));
    }

    wil::com_ptr<IWICFormatConverter> premultiplied;
    Success(factory->CreateFormatConverter(premultiplied.put()), "create an independent premultiplied icon reference");
    Success(premultiplied->Initialize(bitmap.get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "prepare the independent WIC premultiplied reference for the native brand resource");
    Success(premultiplied->CopyPixels(nullptr, requestedWidth * 4, static_cast<UINT>(pixels.size()), pixels.data()),
            "read the independent premultiplied icon reference");
    const auto canonicalPixels = LoadBrandMarkPixels(icon);
    Require(canonicalPixels.width == width && canonicalPixels.height == height && canonicalPixels.bgra == pixels,
            "canonical HICON extraction matches every WIC reference byte, including partial and zero alpha");
}
void CheckBrandIconResource(HINSTANCE module, unsigned requestedSize, bool expectNativeFrame = true)
{
    wil::unique_hicon icon{static_cast<HICON>(
        LoadImageW(module, MAKEINTRESOURCEW(IDI_REDTILDE), IMAGE_ICON, static_cast<int>(requestedSize), static_cast<int>(requestedSize), LR_DEFAULTCOLOR))};
    CheckBrandIconPixels(icon.get(), requestedSize, requestedSize, expectNativeFrame);
}
std::vector<BYTE> ReadBrandMarkRaster(const std::filesystem::path& path, float dpi)
{
    wil::com_ptr<IWICImagingFactory> factory;
    Success(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())),
            "create a WIC decoder for the hidden application capture");
    wil::com_ptr<IWICBitmapDecoder> decoder;
    Success(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.put()),
            "open the hidden application capture");
    wil::com_ptr<IWICBitmapFrameDecode> frame;
    Success(decoder->GetFrame(0, frame.put()), "read the hidden application capture frame");
    wil::com_ptr<IWICFormatConverter> converter;
    Success(factory->CreateFormatConverter(converter.put()), "create the capture pixel view");
    Success(converter->Initialize(frame.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "convert the hidden capture to BGRA pixels");
    UINT width{}, height{};
    Success(converter->GetSize(&width, &height), "read the hidden capture dimensions");
    std::vector<BYTE> pixels(static_cast<std::size_t>(width) * height * 4);
    Success(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data()), "read hidden capture pixels");
    const auto pixelScale = dpi / 96.f;
    const auto left = static_cast<UINT>(12.f * pixelScale), top = static_cast<UINT>(10.f * pixelScale);
    const auto right = std::min(width, static_cast<UINT>(48.f * pixelScale)), bottom = std::min(height, static_cast<UINT>(46.f * pixelScale));
    Require(right > left && bottom > top, "brand-mark capture crop fits the hidden viewport");
    const auto backgroundOffset = (static_cast<std::size_t>(2u) * width + 2u) * 4;
    std::size_t visible{};
    std::vector<BYTE> region;
    region.reserve(static_cast<std::size_t>(right - left) * (bottom - top) * 4);
    for (UINT y = top; y < bottom; ++y)
        for (UINT x = left; x < right; ++x)
        {
            const auto offset = (static_cast<std::size_t>(y) * width + x) * 4;
            region.insert(region.end(), pixels.begin() + static_cast<std::ptrdiff_t>(offset), pixels.begin() + static_cast<std::ptrdiff_t>(offset + 4));
            visible += ! std::equal(pixels.begin() + static_cast<std::ptrdiff_t>(offset),
                                    pixels.begin() + static_cast<std::ptrdiff_t>(offset + 3),
                                    pixels.begin() + static_cast<std::ptrdiff_t>(backgroundOffset));
        }
    Require(visible > 16, "the brand-mark region contains rendered icon pixels");
    return region;
}
std::vector<BYTE> ReadControlRaster(const std::filesystem::path& path, D2D1_RECT_F bounds, float dpi)
{
    // Control oracles compare only their region; the brand-mark reader has a fixed-region/alpha contract.
    wil::com_ptr<IWICImagingFactory> factory;
    Success(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())),
            "create a WIC decoder for the hidden caption-button capture");
    wil::com_ptr<IWICBitmapDecoder> decoder;
    Success(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.put()),
            "open the hidden caption-button capture");
    wil::com_ptr<IWICBitmapFrameDecode> frame;
    Success(decoder->GetFrame(0, frame.put()), "read the caption-button capture frame");
    wil::com_ptr<IWICFormatConverter> converter;
    Success(factory->CreateFormatConverter(converter.put()), "create the caption-button pixel view");
    Success(converter->Initialize(frame.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "convert caption pixels to BGRA");
    UINT width{}, height{};
    Success(converter->GetSize(&width, &height), "read caption capture dimensions");
    Require(width > 0 && height > 0 && width <= 4096 && height <= 4096 && width <= std::numeric_limits<UINT>::max() / 4,
            "caption capture dimensions stay within the bounded hidden viewport");
    const float scale = dpi / 96.f;
    const auto left   = static_cast<UINT>(std::max(0.f, std::floor(bounds.left * scale)));
    const auto top    = static_cast<UINT>(std::max(0.f, std::floor(bounds.top * scale)));
    const auto right  = std::min(width, static_cast<UINT>(std::ceil(bounds.right * scale)));
    const auto bottom = std::min(height, static_cast<UINT>(std::ceil(bounds.bottom * scale)));
    Require(right > left && bottom > top, "caption capture crop fits the hidden viewport");
    const auto pixelBytes = static_cast<std::size_t>(width) * height * 4;
    Require(pixelBytes <= std::numeric_limits<UINT>::max(), "caption capture byte count fits the WIC copy contract");
    std::vector<BYTE> pixels(pixelBytes);
    Success(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data()), "read caption capture pixels");
    std::vector<BYTE> region;
    region.reserve(static_cast<std::size_t>(right - left) * (bottom - top) * 4);
    for (UINT y = top; y < bottom; ++y)
        for (UINT x = left; x < right; ++x)
        {
            const auto offset = (static_cast<std::size_t>(y) * width + x) * 4;
            region.insert(region.end(), pixels.begin() + static_cast<std::ptrdiff_t>(offset), pixels.begin() + static_cast<std::ptrdiff_t>(offset + 4));
        }
    return region;
}
void WriteFixture(const std::filesystem::path& path, std::string_view contents = "# Invented notes\n")
{
    std::filesystem::create_directories(path.parent_path());
    Success(WriteBytesAtomic(path, contents), "seed design integration fixture");
}
void Await(Application& app,
           const std::function<bool()>& complete,
           std::chrono::milliseconds timeout              = std::chrono::seconds(10),
           const std::function<std::string()>& diagnostic = {})
{
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (! complete() && std::chrono::steady_clock::now() < end)
    {
        app.Prepare(32);
        // Prepare stops after host retirement; only this owned notification
        // window may still need to receive asynchronous shutdown completions.
        if (const auto window = app.Window())
        {
            MSG message{};
            unsigned processed{};
            while (processed++ < 64 && app.Window() == window && PeekMessageW(&message, window, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    Require(complete(), diagnostic ? diagnostic() : "hidden application did not settle the expected bounded worker result");
}
} // namespace

// This probe observes the application's real command/model boundaries. It does
// not replace production callbacks or synthesize desktop focus, input or menus.
class DesignRefreshProbe final
{
    static LaunchOptions Options(const std::filesystem::path& root, std::wstring_view name, std::wstring scenario = L"compose-light")
    {
        LaunchOptions options;
        options.headless      = true;
        options.dataDirectory = root / name;
        options.scenario      = std::move(scenario);
        return options;
    }
    static void Resize(Application& app, unsigned width, unsigned height, UINT dpi = 0, bool prepare = true)
    {
        if (! dpi)
            dpi = GetDpiForWindow(app.Window());
        RECT rect{};
        Require(GetWindowRect(app.Window(), &rect) != FALSE, "read hidden frame rectangle");
        rect.right  = rect.left + MulDiv(static_cast<int>(width), static_cast<int>(dpi), 96);
        rect.bottom = rect.top + MulDiv(static_cast<int>(height), static_cast<int>(dpi), 96);
        // This is the same owner-window message/host path as an OS DPI change.
        // It exercises layout at 100/150/200%; it is not physical-monitor evidence.
        (void)SendMessageW(app.Window(), WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&rect));
        Require(SetWindowPos(app.Window(),
                             nullptr,
                             rect.left,
                             rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                "size the bounded hidden viewport independently of physical monitor tracking limits");
        app.Layout();
        if (prepare)
            app.Prepare(128);
        const auto actual = app.Host().GetClientBoundsDip();
        Require(Near(actual.right, static_cast<float>(width)) && Near(actual.bottom, static_cast<float>(height)),
                "hidden layout scenarios use their actual requested DIP viewport");
        Require(Near(app.Host().GetDpi(), static_cast<float>(dpi)), "hidden host adopts the requested DPI");
    }
    static void CheckChrome(Application& app)
    {
        const auto client = app.Host().GetClientBoundsDip();
        for (auto* bar : {app._documentBar, app._commandBar})
        {
            if (! bar->IsVisible())
                continue;
            Require(Inside(bar->GetBounds(), client), "chrome region remains inside the client area");
            std::vector<DxUi::Control*> visible;
            for (std::size_t index = 0; index < bar->GetLogicalChildCount(); ++index)
            {
                auto* control = bar->GetLogicalChild(index);
                if (! control->IsVisible())
                    continue;
                Require(Inside(control->GetBounds(), bar->GetBounds()), "every visible chrome item has positive, in-row bounds");
                visible.push_back(control);
            }
            for (std::size_t first = 0; first < visible.size(); ++first)
                for (std::size_t second = first + 1; second < visible.size(); ++second)
                    Require(! Overlap(visible[first]->GetBounds(), visible[second]->GetBounds()), "visible chrome items never overlap");
        }
        Require(Inside(app._editor->GetBounds(), app._page->GetBounds()), "editor fits the writing surface");
        if (app._mirror->IsVisible())
        {
            Require(Inside(app._mirror->GetBounds(), app._page->GetBounds()), "read-only Split mirror fits the writing surface");
            Require(! Overlap(app._editor->GetBounds(), app._mirror->GetBounds()), "editable and derived Split text regions never overlap");
        }
    }
    static LRESULT Hit(Application& app, D2D1_POINT_2F point)
    {
        const auto screen = app.Host().DipPointToScreenPoint(point);
        return app.FrameHitTest(app.Window(), MAKELPARAM(static_cast<SHORT>(screen.x), static_cast<SHORT>(screen.y)));
    }
    static LRESULT HitControl(Application& app, const DxUi::Control* control)
    {
        const auto bounds = control->GetBounds();
        return Hit(app, {(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2});
    }
    static bool AccessibilityContainsName(Application& app, std::wstring_view expected)
    {
        wil::com_ptr_nothrow<IRawElementProviderFragmentRoot> root;
        root.attach(DxUi::CreateWindowHostAccessibilityProvider(app.Host().GetHwnd()));
        Require(root != nullptr, "the application exposes its current UI Automation root");
        wil::com_ptr_nothrow<IRawElementProviderFragment> rootFragment;
        Success(root.query_to(rootFragment.put()), "the UI Automation root supports fragment navigation");
        std::size_t visited{};
        const auto visit = [&](auto&& self, IRawElementProviderFragment* current, std::size_t depth) -> bool
        {
            if (! current || depth > 64u || visited >= 4096u)
                return false;
            ++visited;
            wil::com_ptr_nothrow<IRawElementProviderSimple> simple;
            Success(current->QueryInterface(IID_PPV_ARGS(simple.put())), "query a UI Automation fragment's simple provider");
            wil::unique_variant value;
            Success(simple->GetPropertyValue(UIA_NamePropertyId, &value), "read a UI Automation fragment's accessible Name");
            if (value.vt == VT_BSTR && std::wstring_view(value.bstrVal ? value.bstrVal : L"") == expected)
                return true;

            wil::com_ptr_nothrow<IRawElementProviderFragment> child;
            Success(current->Navigate(NavigateDirection_FirstChild, child.put()), "navigate to the first UI Automation child");
            while (child && visited < 4096u)
            {
                if (self(self, child.get(), depth + 1u))
                    return true;
                wil::com_ptr_nothrow<IRawElementProviderFragment> sibling;
                Success(child->Navigate(NavigateDirection_NextSibling, sibling.put()), "navigate to the next UI Automation sibling");
                child = std::move(sibling);
            }
            return false;
        };
        return visit(visit, rootFragment.get(), 0u);
    }

public:
    static void Register(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
    {
        test("Editor.LocalImageChangesReloadWithoutEditing",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto fixture = root / L"image-auto-refresh";
            const auto image   = fixture / L"Illustrations/cover.bmp";
            std::filesystem::create_directories(image.parent_path());
            const auto write = [&](bool singlePixel)
            {
                const auto encoded = Testing::ImageBitmapFixture(singlePixel);
                Require(SUCCEEDED(WriteBytesAtomic(image, std::string_view(reinterpret_cast<const char*>(encoded.data()), encoded.size()))),
                        "write the owned image with independently known decoded dimensions");
            };
            write(false);
            Application app(Options(root, L"image-auto-refresh/settings"));
            Require(SUCCEEDED(app.Initialize()) &&
                        app.SetDocument(L"# Illustration notebook\n\n![Invented cover](Illustrations/cover.bmp)\n", fixture / L"Notebook.md"),
                    "bind an invented notebook and local image in the hidden production application");
            Await(app,
                  [&]
            {
                const auto stats = app.Document().GetImageResourceStatistics();
                return app._imageFileRefreshes >= 1u && stats.readySources == 1u && stats.decodedBytes == 8u;
            });
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto saved     = app._session.savedText;
            const auto refreshes = app._imageFileRefreshes;
            write(true);
            Await(app,
                  [&]
            {
                const auto stats = app.Document().GetImageResourceStatistics();
                return app._imageFileRefreshes > refreshes && stats.readySources == 1u && stats.decodedBytes == 4u;
            },
                  std::chrono::seconds(10),
                  [&]
            {
                const auto stats = app.Document().GetImageResourceStatistics();
                std::size_t jobs{}, results{};
                bool active{};
                if (const auto state = app._images._state)
                {
                    const std::lock_guard lock(state->mutex);
                    jobs    = state->jobCount;
                    results = state->resultCount;
                    active  = state->phase == RichImageLoader::State::WorkerPhase::Active;
                }
                return std::format("changed image did not settle: refreshes={}/{} watches={} pathBuilds={} "
                                   "ready={} pending={} failed={} bytes={} status={:08X} jobs={} results={} active={}",
                                   app._imageFileRefreshes,
                                   refreshes,
                                   app._watchedImagePaths.size(),
                                   app._imageWatchPathBuilds,
                                   stats.readySources,
                                   stats.pendingSources,
                                   stats.failedSources,
                                   stats.decodedBytes,
                                   static_cast<unsigned>(app._imageFailureStatus),
                                   jobs,
                                   results,
                                   active);
            });
            Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app._session.savedText == saved &&
                        ! app.Dirty() && ! app.Document().CanUndo() && ! app.Document().CanRedo() && foreground.NeverAcquiredForeground(),
                    "a real image file change replaces decoded pixels without source, history, saved-state or foreground changes");
            Require(app._watchedImagePaths.size() == 1u, "a refreshed ready image retains one canonical watch path after decode job completion");
            Require(app.Capture(fixture / L"refreshed-local-image.png"), "capture actual refreshed pixels with invented fixture content");
            const auto pathBuilds    = app._imageWatchPathBuilds;
            const auto editRefreshes = app._imageFileRefreshes;
            const auto end           = app.Document().GetText().size();
            app.Document().SetSelection({end, end});
            Require(app.Document().ReplaceSelection(L"\nAnother invented observation.\n"), "type ordinary prose after the image");
            app.Prepare(128);
            Require(app._imageWatchPathBuilds == pathBuilds && app._imageFileRefreshes == editRefreshes,
                    "ordinary source edits reuse unchanged image watch paths rather than rebuilding paths or rearming I/O");
            const auto sourceWithRemote = app.Document().GetText() + L"\n![Blocked illustration](HtTpS://images.example.test/invented.bmp)\n";
            Require(app.SetDocument(sourceWithRemote, fixture / L"Notebook.md"), "add a blocked remote destination beside the retained local image");
            Await(app,
                  [&]
            {
                const auto semantic = app.Document().GetSemanticDocument();
                const auto stats    = app.Document().GetImageResourceStatistics();
                return semantic && semantic->Syntax()->Source() == sourceWithRemote &&
                       std::ranges::count_if(semantic->Elements(), [](const auto& element) { return element.role == DxUi::RichText::SemanticRole::Image; }) ==
                           2 &&
                       stats.requestedSources == 1u && stats.readySources == 1u && stats.pendingSources == 0u;
            });
            Require(app._imageWatchPathBuilds == pathBuilds && app._imageFileRefreshes == editRefreshes && app._watchedImagePaths.size() == 1u &&
                        app._watchedImagePaths.front() == image,
                    "a remote-only semantic change preserves the admitted local watch without rearming I/O or reloading images");
            Require(SUCCEEDED(WriteBytesAtomic(fixture / L"image-refresh-metrics.json",
                                               std::format("{{\"automaticRefreshes\":{},\"observedFiles\":1,\"decodedBytesBefore\":8,"
                                                           "\"decodedBytesAfter\":4,\"sourceUnchanged\":true}}\n",
                                                           app._imageFileRefreshes))),
                    "retain deterministic refresh-work counters with the native run");
            const auto replacement      = fixture / L"Illustrations/replacement.bmp";
            const auto replacementBytes = Testing::ImageBitmapFixture(false);
            Success(WriteBytesAtomic(replacement, std::string_view(reinterpret_cast<const char*>(replacementBytes.data()), replacementBytes.size())),
                    "seed the independently sized replacement image");
            Require(app.SetDocument(L"![Replacement cover](Illustrations/replacement.bmp)\n", fixture / L"Notebook.md"),
                    "retarget the image without changing the document base");
            Await(app,
                  [&]
            {
                const auto stats = app.Document().GetImageResourceStatistics();
                return stats.readySources == 1u && stats.decodedBytes == 8u && app._watchedImagePaths.size() == 1u &&
                       app._watchedImagePaths.front() == replacement;
            });
            const auto movedFixture = fixture / L"Moved";
            const auto movedImage   = movedFixture / L"Illustrations/replacement.bmp";
            std::filesystem::create_directories(movedImage.parent_path());
            const auto movedBytes = Testing::ImageBitmapFixture(true);
            Success(WriteBytesAtomic(movedImage, std::string_view(reinterpret_cast<const char*>(movedBytes.data()), movedBytes.size())),
                    "seed a different image at the same relative destination under a new document base");
            Require(app.SetDocument(L"![Replacement cover](Illustrations/replacement.bmp)\n", movedFixture / L"Notebook.md"),
                    "move the document base while retaining the same literal image destination");
            Await(app,
                  [&]
            {
                const auto stats = app.Document().GetImageResourceStatistics();
                return stats.readySources == 1u && stats.decodedBytes == 4u && app._watchedImagePaths.size() == 1u &&
                       app._watchedImagePaths.front() == movedImage;
            });
            Require(app.SetDocument(L"# Text-only notebook\n\nAnother invented observation.\n", movedFixture / L"Notebook.md"),
                    "replace the image document with an image-free source");
            Await(app, [&] { return app.Document().GetImageResourceStatistics().imageElements == 0u; });
            Require(app._watchedImagePaths.empty(), "an image-free replacement retires every old ready image watch path");
            const auto retiredRefreshes = app._imageFileRefreshes;
            write(false);
            Success(WriteBytesAtomic(movedImage, std::string_view(reinterpret_cast<const char*>(replacementBytes.data()), replacementBytes.size())),
                    "change the retired file under the current document base");
            const auto quietUntil = GetTickCount64() + 400u;
            while (GetTickCount64() < quietUntil)
            {
                app.Prepare(4);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            Require(app._imageFileRefreshes == retiredRefreshes, "the retired file binding cannot request another editor reload");

            {
                RichImageLoader staLoader;
                Success(staLoader.Initialize(app.Window(), WM_APP + 0x350u, 0x737461u), "initialize RichImageLoader for STA worker probe");
                const auto staImage = fixture / L"Illustrations/sta-cover.bmp";
                const auto staBytes = Testing::ImageBitmapFixture(true);
                Success(WriteBytesAtomic(staImage, std::string_view(reinterpret_cast<const char*>(staBytes.data()), staBytes.size())),
                        "seed local image for STA RichImageLoader probe");
                app.SetDocument(L"![STA](Illustrations/sta-cover.bmp)\n", fixture / L"Notebook.md");
                std::vector<DxUi::RichText::RichImageRequest> staRequests;
                for (unsigned turn = 0u; turn < 256u && staRequests.empty(); ++turn)
                {
                    (void)app.Host().RenderInitialFrameForShow();
                    app.Document().TakePendingImageRequests(staRequests);
                }
                Require(staRequests.size() == 1u, "capture RichImageRequest for STA worker probe");
                {
                    const auto state = staLoader._state;
                    const std::lock_guard lock(state->mutex);
                    state->jobs[0u]         = staRequests.front();
                    state->jobCount         = 1u;
                    state->callbackLifetime = state;
                    Success(ProcessWork::Acquire(state->callbackLease), "acquire ProcessWorkLease for STA RichImageLoader probe");
                    state->phase = RichImageLoader::State::WorkerPhase::Active;
                }
                HRESULT threadComInit = E_FAIL;
                HRESULT threadComKeep = E_FAIL;
                std::jthread staThread([&]()
                {
                    threadComInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                    const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(threadComInit));
                    RichImageLoader::State::Worker(nullptr, staLoader._state.get(), nullptr);
                    threadComKeep = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                    if (SUCCEEDED(threadComKeep))
                        CoUninitialize();
                });
                staThread.join();
                std::vector<RichImageLoader::Result> staResults;
                staLoader.Drain(staResults);
                Require(SUCCEEDED(threadComInit) && threadComKeep == S_FALSE && staResults.size() == 1u && SUCCEEDED(staResults.front().status) &&
                            staResults.front().image != nullptr,
                        "RichImageLoader::State::Worker accepts RPC_E_CHANGED_MODE on STA threads without uninitializing caller COM");
                staLoader.Shutdown();
            }
        });
        test("Editor.ClipboardBitmapAssetHandler",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"clipboard-image-handler"));
            Success(app.Initialize(), "create hidden clipboard-image app");
            app.SetDocument(L"A selected observation.\r\n");
            Require(app.Host().GetClipboardPreference() == DxUi::TextClipboardPreference::TextFirst,
                    "ordinary clipboard paste retains the Unicode-first default");
            Require(app.Document().HasBitmapPasteHandler(), "the app owns a bitmap asset paste handler");
            Require(foreground.NeverAcquiredForeground(), "clipboard image admission never takes desktop focus");
        });
        test("Editor.ClipboardBitmapPasteCommitUndoAndBusy",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto options = Options(root, L"clipboard-image-commit");
            Application app(options);
            Success(app.Initialize(), "create hidden bitmap transaction app");
            const std::wstring original = L"A selected observation.\r\n";
            app.SetDocument(original);
            app.Prepare(128);
            app.Host().SetFocusControl(&app.Document());
            app.Document().SetSelection({2, 10});
            const auto selection = app.Document().GetSelection();
            app.Execute(Command::ImageFirstPaste);
            Require(app.Preferences().imageFirstPaste && app.Host().GetClipboardPreference() == DxUi::TextClipboardPreference::ImageFirst &&
                        app.Document().GetText() == original && app.Document().GetSelection() == selection,
                    "live image-first setting preserves source and selection");
            auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
            Require(gate->entered && gate->proceed, "create owned bitmap worker gate");
            app._bitmapWriter.SetTestGate(gate);
            const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            const auto dib     = Testing::ClipboardBitmapFixture(true);
            Require(! app.BeginBitmapPaste(dib, true), "the synchronous handler defers file I/O and source insertion");
            Await(app, [&] { return WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; });
            const auto request = app._pendingBitmapPaste->request;
            (void)app.BeginBitmapPaste(dib, true);
            Require(app._pendingBitmapPaste->request == request && app.Document().GetText() == original,
                    "one busy paste cannot replace the admitted source request");
            (void)SetEvent(gate->proceed.get());
            Await(app, [&] { return ! app._pendingBitmapPaste && ! app._bitmapWriter.Busy() && ! app._bitmapWriter.HasResult(); });
            const auto pasted = app.Document().GetText();
            Require(pasted != original && pasted.starts_with(L"A ![Pasted image](") && pasted.ends_with(L"observation.\r\n") &&
                        Testing::ClipboardAssetCount(options.dataDirectory) == 1,
                    "one durable asset produces one source image reference while retaining outside CRLF content");
            Await(app,
                  [&]
            {
                const auto images = app.Document().GetImageResourceStatistics();
                return images.readySources != 0 || images.failedSources != 0;
            });
            const auto images = app.Document().GetImageResourceStatistics();
            Require(images.readySources == 1 && images.failedSources == 0, "the native document resolves and decodes the durable pasted PNG reference");
            Require(app.Document().Undo() && app.Document().GetText() == original && app.Document().GetSelection() == selection,
                    "one Undo restores the entire original bitmap-paste selection and source");
            Require(Testing::ClipboardAssetCount(options.dataDirectory) == 1 && app.Document().Redo() && app.Document().GetText() == pasted,
                    "asset retention supports exact Redo");
            app.Execute(Command::ImageFirstPaste);
            Require(! app.Preferences().imageFirstPaste && app.Host().GetClipboardPreference() == DxUi::TextClipboardPreference::TextFirst,
                    "the app option restores Unicode-first policy without changing pasted source");
            {
                const auto staDir     = options.dataDirectory / L"sta-worker";
                const auto rootDrive  = staDir.root_name().native();
                const bool fixedDrive = rootDrive.size() == 2u && rootDrive[1] == L':' && GetDriveTypeW(staDir.root_path().c_str()) == DRIVE_FIXED;
                if (fixedDrive)
                {
                    BitmapPasteWriter staWriter;
                    Success(staWriter.Initialize(staDir, app.Window(), WM_APP + 0x351u), "initialize BitmapPasteWriter for STA worker probe");
                    auto job            = std::make_unique<BitmapPasteWriter::Job>();
                    job->state          = staWriter._state;
                    job->dib            = Testing::ClipboardBitmapFixture(true);
                    job->v5             = true;
                    job->result.request = 1u;
                    {
                        const std::scoped_lock lock(staWriter._state->mutex);
                        staWriter._state->serial = 1u;
                        staWriter._state->phase  = BitmapPasteWriter::Phase::Encoding;
                    }
                    Success(ProcessWork::Acquire(job->lease), "acquire ProcessWorkLease for STA BitmapPasteWriter probe");
                    HRESULT threadComInit = E_FAIL;
                    HRESULT threadComKeep = E_FAIL;
                    std::jthread staThread([&]()
                    {
                        threadComInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                        const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(threadComInit));
                        BitmapPasteWriter::Worker(nullptr, job.release());
                        threadComKeep = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                        if (SUCCEEDED(threadComKeep))
                            CoUninitialize();
                    });
                    staThread.join();
                    BitmapPasteWriter::Result staResult;
                    Require(SUCCEEDED(threadComInit) && threadComKeep == S_FALSE && staWriter.Take(staResult) && SUCCEEDED(staResult.status) &&
                                ! staResult.asset.path.empty(),
                            "BitmapPasteWriter::Worker accepts RPC_E_CHANGED_MODE on STA threads without uninitializing caller COM");
                    Success(staWriter.Discard(std::move(staResult)), "retire STA BitmapPasteWriter probe asset");
                    Await(app, [&] { return ! staWriter.Busy(); });
                    staWriter.Stop();
                }
            }
            Require(foreground.NeverAcquiredForeground(), "bitmap paste never acquires the desktop or clipboard");
        });
        test("Editor.ClipboardBitmapPasteDeclinesSupersededSource",
             [&]
        {
            const auto options = Options(root, L"clipboard-image-stale");
            Application app(options);
            Success(app.Initialize(), "create stale bitmap transaction app");
            const std::wstring original = L"An unchanged paragraph.\n";
            app.SetDocument(original);
            app.Prepare(128);
            app.Host().SetFocusControl(&app.Document());
            auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
            app._bitmapWriter.SetTestGate(gate);
            const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            const auto dib     = Testing::ClipboardBitmapFixture();
            (void)app.BeginBitmapPaste(dib, false);
            Await(app, [&] { return WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; });
            Require(app.Document().ReplaceSelection(L"new") && app.Document().Undo(), "change then restore source while the bitmap is pending");
            Require(! app._pendingBitmapPaste, "a source edit permanently retires the old paste even after Undo");
            (void)SetEvent(gate->proceed.get());
            Await(app, [&] { return ! app._bitmapWriter.Busy() && ! app._bitmapWriter.HasResult(); });
            Require(app.Document().GetText() == original && app.Document().CanRedo() && Testing::ClipboardAssetCount(options.dataDirectory) == 0,
                    "a stale worker result preserves newer history and removes only its unreferenced asset");
        });
        test("Editor.ClipboardBitmapPasteFailureAndReadOnly",
             [&]
        {
            const auto options = Options(root, L"clipboard-image-refusal");
            Application app(options);
            Success(app.Initialize(), "create bitmap refusal app");
            const std::wstring original = L"Preserved source.\n";
            app.SetDocument(original);
            app.Prepare(128);
            app.Host().SetFocusControl(&app.Document());
            (void)app.BeginBitmapPaste(std::vector<std::byte>(40u), false);
            Await(app, [&] { return ! app._pendingBitmapPaste && ! app._bitmapWriter.Busy() && ! app._bitmapWriter.HasResult(); });
            Require(app.Document().GetText() == original && ! app.Document().CanUndo() && Testing::ClipboardAssetCount(options.dataDirectory) == 0,
                    "malformed bitmap failure retains exact source and history and publishes no file");
            auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
            app._bitmapWriter.SetTestGate(gate);
            const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            (void)app.BeginBitmapPaste(Testing::ClipboardBitmapFixture(), false);
            Await(app, [&] { return WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; });
            app.Document().SetReadOnly(true);
            (void)SetEvent(gate->proceed.get());
            Await(app, [&] { return ! app._pendingBitmapPaste && ! app._bitmapWriter.Busy() && ! app._bitmapWriter.HasResult(); });
            Require(app.Document().GetText() == original && Testing::ClipboardAssetCount(options.dataDirectory) == 0,
                    "a read-only transition refuses the late paste and retires the owned asset");
        });
        test("Editor.ClipboardBitmapPasteCloseDrainsOwnedWorker",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"clipboard-image-close"));
            Success(app.Initialize(), "create bitmap close app");
            app.SetDocument(L"A clean document.\n");
            app.Prepare(128);
            app.Host().SetFocusControl(&app.Document());
            auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
            app._bitmapWriter.SetTestGate(gate);
            const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            (void)app.BeginBitmapPaste(Testing::ClipboardBitmapFixture(), false);
            Await(app, [&] { return WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; });
            app.Execute(Command::Close);
            Require(app.IsClosing() && app.Window() && app._bitmapWriter.Busy() && ! app._pendingBitmapPaste,
                    "accepted Close retains only its notification window until the owned bitmap worker settles");
            (void)SetEvent(gate->proceed.get());
            Await(app, [&] { return ! app.Window(); });
            Require(! app._bitmapWriter.Busy() && foreground.NeverAcquiredForeground(),
                    "cancelled bitmap work drains without a UI-thread join or desktop takeover");
        });
        test("Editor.ClipboardBitmapPasteSessionEndRetiresInsertion",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto options = Options(root, L"clipboard-image-session");
            Application app(options);
            Success(app.Initialize(), "create bitmap session-end app");
            const std::wstring original = L"A session observation.\n";
            app.SetDocument(original);
            app.Prepare(128);
            app.Host().SetFocusControl(&app.Document());
            auto gate = std::make_shared<BitmapPasteWriter::TestGate>();
            app._bitmapWriter.SetTestGate(gate);
            const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            (void)app.BeginBitmapPaste(Testing::ClipboardBitmapFixture(), false);
            Await(app, [&] { return WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; });
            Require(SendMessageW(app.Window(), WM_QUERYENDSESSION, 0, 0) == TRUE, "session query permits shutdown while bitmap encoding is pending");
            (void)SendMessageW(app.Window(), WM_ENDSESSION, FALSE, 0);
            Require(! app.IsSessionEnding() && app._pendingBitmapPaste.has_value(), "cancelled system shutdown retains the admitted bitmap operation");
            (void)SendMessageW(app.Window(), WM_ENDSESSION, TRUE, 0);
            Require(app.IsSessionEnding() && ! app._pendingBitmapPaste && app._bitmapWriter.Busy(),
                    "final session end retires insertion without waiting for the held codec worker");
            (void)app.BeginBitmapPaste(Testing::ClipboardBitmapFixture(), false);
            Require(! app._pendingBitmapPaste, "final session end refuses another bitmap admission");
            (void)SetEvent(gate->proceed.get());
            Await(app, [&] { return ! app._bitmapWriter.Busy() && ! app._bitmapWriter.HasResult(); });
            Require(app.Document().GetText() == original && Testing::ClipboardAssetCount(options.dataDirectory) == 0 && foreground.NeverAcquiredForeground(),
                    "late bitmap work cannot alter checkpointed source or acquire the desktop");
        });
        test("Editor.Api5CheckedAdmissionPreservesWorkspaceAndHistory",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"api5-checked-admission"));
            Success(app.Initialize(), "initialize hidden API-5 admission app");
            Require(app.SetDocument(L"Saved observation\n"), "admit baseline document");
            Require(app.Document().ReplaceSelection(L"Unsaved "), "retain an unsaved source/history before refusal");
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto saved     = app._session.savedText;
            const auto path      = app._session.path;
            const auto key       = app._session.key;
            for (const auto& candidate : {std::wstring(DxUi::RichText::Document::MaximumUnits + 1u, L'x'), std::wstring(1u, L'\xD800')})
            {
                Require(! app.SetDocument(candidate), "invalid or oversized source returns an explicit refusal");
                Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app._session.savedText == saved &&
                            app._session.path == path && app._session.key == key && app.Dirty() && app.Document().CanUndo(),
                        "checked source refusal preserves source, selection, saved baseline, dirty workspace and undo history");
                Require(app._feedback->GetText() == Resource(candidate.size() > DxUi::RichText::Document::MaximumUnits ? IDS_REPLACEFAILED : IDS_ACTIONFAILED),
                        "source capacity and invalid-source refusals use the intended status");
            }
            Require(! app.SetDocument(L"Replacement", L"relative.md") && app.Document().Snapshot().IsSameRevision(source) && app._session.savedText == saved &&
                        app._session.path == path && app.Dirty(),
                    "invalid document metadata refuses before resetting editable source or its saved baseline");
            Require(app.SetDocument(std::wstring(DxUi::RichText::Document::MaximumUnits - 2u, L'x')), "admit near-capacity source");
            const auto end = app.Document().Snapshot().Length();
            Require(app.Document().TrySetSelection({end, end}) == DxUi::RichText::SourceResult::Success && app.Document().ReplaceSelection(L"y") &&
                        app.Document().ReplaceSelection(L"z") && app.Document().Undo(),
                    "seed independent dirty undo and redo history near capacity");
            Require(app.Document().TrySetSelection({0u, app.Document().Snapshot().Length()}) == DxUi::RichText::SourceResult::Success,
                    "select exact near-capacity source");
            const auto nearCapacity = app.Document().Snapshot();
            const auto selected     = app.Document().GetSelection();
            for (const auto command : {Command::Quote, Command::Fence})
            {
                app.Execute(command);
                Require(app.Document().Snapshot().IsSameRevision(nearCapacity) && app.Document().GetSelection() == selected && app.Document().CanUndo() &&
                            app.Document().CanRedo() && app.Dirty(),
                        "quote/code capacity refusal preserves the selected source and both history directions");
            }
            Require(foreground.NeverAcquiredForeground(), "API-5 admission coverage keeps the desktop untouched");
        });
        test("Editor.RecoverableCommandsAndMenusRetainSourceAndDraftFocus",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            auto options = Options(root, L"guarded-command-menu");
            bool failCommand{};
            options.commandAdmissionForTest = [&](Command command) { return failCommand && command == Command::Fence ? E_BOUNDS : S_OK; };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden guarded-command app");
            app.SetDocument(L"Notebook\nA useful observation.\n");
            app.Document().SetSelection({0, 8});
            Require(app.Document().ReplaceSelection(L"Revised notebook"), "seed dirty source before command failure");
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            failCommand          = true;
            app.Execute(Command::Fence);
            app.OnMenuClosed(static_cast<int>(Command::Fence) + 1);
            Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app.Dirty() && app.Document().CanUndo() &&
                        app._feedback->GetText() == Resource(IDS_ACTIONFAILED),
                    "direct and real menu-callback entry points contain recoverable failures and retain source/history");
            failCommand = false;
            for (const auto command : {Command::LocalSource, Command::Link})
            {
                app.Document().SetSelection({0, 7});
                app.OnMenuClosed(static_cast<int>(command) + 1);
                Require(app.HasBlockDraft() && app.Host().GetFocusControl() == app._blockDraftText && app.Document().Snapshot().IsSameRevision(source),
                        "shared menu close keeps focus in the draft opened by F2 or Link");
                app.Execute(Command::CloseDrawer);
            }
            app._options.unsavedPromptForTest = [] { return IDNO; };
            app.OnMenuClosed(static_cast<int>(Command::Close) + 1);
            Require(app.IsClosing() && app.Host().GetRoot() == nullptr, "menu close revalidates a retired root before attempting focus");
            Require(foreground.NeverAcquiredForeground(), "menu callback replay never opens a desktop menu or acquires focus");
        });
        test("Editor.FileCompletionFailureRetainsPendingWriteAndCancelsClose",
             [&]
        {
            for (const bool close : {false, true})
            {
                Testing::ForegroundOwnershipObserver foreground;
                auto options         = Options(root, close ? L"completion-failure-close" : L"completion-failure-open");
                const auto directory = options.dataDirectory;
                const auto gate = options.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
                bool failCompletion{};
                options.fileCompletionAdmissionForTest = [&] { return std::exchange(failCompletion, false) ? E_FAIL : S_OK; };
                Application app(std::move(options));
                Success(app.Initialize(), "initialize hidden completion-boundary app");
                const auto release = wil::scope_exit([&]
                {
                    (void)SetEvent(gate->proceed.get());
                    if (WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0)
                        (void)WaitForSingleObject(gate->finished.get(), 10000);
                });
                const auto path    = directory / L"Notebook.md";
                WriteFixture(path, "Notebook\n");
                StoredDocument loaded;
                Success(LoadDocument(path, loaded), "load completion baseline");
                app.SetDocument(loaded.text, path, loaded.bom, loaded.stamp);
                app.Document().SetSelection({loaded.text.size(), loaded.text.size()});
                Require(app.Document().ReplaceSelection(L"Admitted writing\n"), "seed admitted source");
                const auto source = app.Document().Snapshot();
                bool staleContinuation{};
                app._activeFileTx = Application::SaveTx{[&] { staleContinuation = true; }};
                app.Execute(Command::Save);
                Require(WaitForSingleObject(gate->entered.get(), 10000) == WAIT_OBJECT_0, "real save reaches admission barrier");
                if (close)
                    (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
                Require(SetEvent(gate->proceed.get()) && WaitForSingleObject(gate->finished.get(), 10000) == WAIT_OBJECT_0,
                        "the real write settles before completion failure");
                failCompletion = true;
                app.FileCompleted();
                Require(app.Window() && ! app.IsClosing() && ! app.IsClosingAfterSave() && app.Dirty() && app._files.Busy() &&
                            app.Document().Snapshot().IsSameRevision(source) && app._feedback->GetText() == Resource(IDS_ACTIONFAILED),
                        "a completion exception retains the pending result, dirty source and window while cancelling pending Close");
                app.FileCompleted();
                Require(! app._files.Busy() && ! app.Dirty() && app.Window() && ! staleContinuation && app.Document().Snapshot().IsSameRevision(source),
                        "later completion adopts the known write once without replaying an old Close/Open continuation");
                StoredDocument actual;
                Success(LoadDocument(path, actual), "read completed guarded save");
                Require(actual.text == source.Text() && actual.stamp == app._session.stamp && foreground.NeverAcquiredForeground(),
                        "the successful saved baseline remains exact and hidden after a recoverable completion failure");
            }
        });
        test("Editor.CloseCancellationNeverFallsThroughToDefaultDestroy",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            auto options                 = Options(root, L"close-exception-boundary");
            options.unsavedPromptForTest = [] { return IDCANCEL; };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden Close-boundary app");
            app.SetDocument(L"Notebook\n");
            Require(app.Document().ReplaceSelection(L"Unsaved writing\n"), "seed dirty Close source");
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto window    = app.Window();
            Require(SendMessageW(window, WM_CLOSE, 0, 0) == 0 && IsWindow(window) && app.Window() == window && ! app.IsClosing() && app.Dirty() &&
                        app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app.Document().CanUndo(),
                    "a cancelled Close stays handled and never invokes DefWindowProc destruction");
            app._options.unsavedPromptForTest = [] { return IDNO; };
            (void)SendMessageW(window, WM_CLOSE, 0, 0);
            Require(app.IsClosing() && app.Host().GetRoot() == nullptr && foreground.NeverAcquiredForeground(),
                    "a later explicit discard/Close remains available without desktop takeover");
        });
        test("Editor.WorkspaceSwitchRetainsEveryHistoryAndRecovery",
             [&]
        {
            for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
            {
                const auto suffix    = std::to_wstring(static_cast<unsigned>(view));
                const auto directory = root / (L"workspace-files-" + suffix);
                const auto first = directory / L"Alpha.md", second = directory / L"Beta.markdown";
                WriteFixture(first, "\xef\xbb\xbf# Alpha\r\nA useful notebook.\r\n");
                WriteFixture(second, "# Beta\nA second notebook.\n");
                unsigned questions{};
                auto options                 = Options(root, L"workspace-ui-" + suffix);
                options.unsavedPromptForTest = [&]
                {
                    ++questions;
                    return IDCANCEL;
                };
                const auto data = options.dataDirectory;
                Testing::ForegroundOwnershipObserver foreground;
                Application app(std::move(options));
                Success(app.Initialize(), "initialize the hidden document workspace");
                StoredDocument loaded;
                Success(LoadDocument(first, loaded), "read Alpha's original source and encoding");
                app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
                app.Execute(Command::DocumentImages);
                Require(app.Document().RemoteImagesEnabled() && ! app.Preferences().allowRemoteImages, "Alpha owns an explicit document-only image grant");
                app.SetView(view);
                app.Execute(Command::Files);
                Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->markdownCount == 2; });
                app.Document().SetSelection({7, 2});
                Require(app.Document().ReplaceSelection(L"Alpha edited"), "edit Alpha's selected word");
                app.Document().SetSelection({7, 2});
                Require(app.Document().ReplaceSelection(L"Beta") && app.Document().Undo(), "retain an independent Alpha Redo branch");
                const auto alphaSource    = app.Document().Snapshot();
                const auto alphaSelection = app.Document().GetSelection();
                const std::wstring alpha  = L"# Alpha edited\r\nA useful notebook.\r\n";
                Require(app.Document().GetText() == alpha && app.Dirty() && app.Document().CanUndo() && app.Document().CanRedo(),
                        "Alpha has the independently specified source and both history branches");
                app.OpenPath(second);
                Await(app, [&] { return app._session.path == second && ! app._files.Busy(); });
                Require(questions == 0 && app._inactiveDocuments.size() == 1 && app.Document().GetText() == L"# Beta\nA second notebook.\n" &&
                            ! app.Document().CanUndo() && app._folderRoot == directory,
                        "a Files-pane switch keeps the dirty document without asking or changing the root");
                Require(! app.Document().RemoteImagesEnabled(), "Beta cannot inherit Alpha's image grant");
                app.Document().SetSelection({6, 2});
                Require(app.Document().ReplaceSelection(L"Beta revised"), "edit Beta independently");
                app.Document().SetSelection({6, 2});
                const auto betaSource    = app.Document().Snapshot();
                const auto betaSelection = app.Document().GetSelection();
                const std::wstring beta  = L"# Beta revised\nA second notebook.\n";
                WriteFixture(first, "# External revision\r\n");
                app.OpenPath(first);
                Require(! app._files.Busy() && questions == 0 && app._session.path == first && app._session.bom && app._session.stamp == loaded.stamp &&
                            app.Document().Snapshot().IsSameRevision(alphaSource) && app.Document().GetSelection() == alphaSelection,
                        "cached activation retains exact source, BOM, saved identity and oriented selection despite an external file change");
                Require(app.Document().RemoteImagesEnabled(), "cached Alpha activation restores its own image grant");
                Require(app.Document().Undo() && app.Document().GetText() == loaded.text && ! app.Dirty() && app.Document().Redo() &&
                            app.Document().GetText() == alpha && app.Dirty() && app.Document().Redo() &&
                            app.Document().GetText() == L"# Beta edited\r\nA useful notebook.\r\n" && app.Document().Undo() &&
                            app.Document().GetText() == alpha,
                        "Alpha's independent Undo/Redo and saved-content identity survive repeated switches");
                app.OpenPath(second);
                Require(app._session.path == second && app.Document().Snapshot().IsSameRevision(betaSource) && app.Document().GetSelection() == betaSelection &&
                            app.Document().Undo() && app.Document().GetText() == L"# Beta\nA second notebook.\n" && ! app.Dirty() && app.Document().Redo() &&
                            app.Document().GetText() == beta && app.Dirty(),
                        "Beta's history never contains an Alpha edit");
                Require(! app.Document().RemoteImagesEnabled(), "cached Beta activation retains its independent blocked policy");
                Require(app.SourceMirror().Snapshot().Text() == beta || view != EditorView::Split,
                        "the visible Split mirror derives from the newly active source");
                app.CheckpointWorkspaceRecovery();
                Await(app, [&] { return app._recovery.Settled(); });
                Success(app._recovery.Status(), "every dirty workspace document is durable");
                RecoveryService observer;
                Success(observer.Initialize(data), "scan while both workspace documents retain active leases");
                Await(app, [&] { return observer.Ready(); });
                Require(observer.TakeCandidates().empty(), "another process cannot offer an active workspace's snapshots");
                observer.Stop();
                Await(app, [&] { return observer.Finished(); });
                app._recovery.Stop();
                Await(app, [&] { return app._recovery.Finished(); });
                RecoveryService restarted;
                Success(restarted.Initialize(data), "scan the ended workspace on restart");
                Await(app, [&] { return restarted.Ready(); });
                const auto copies = restarted.TakeCandidates();
                Require(copies.size() == 2 &&
                            std::ranges::any_of(copies,
                                                [&](const auto& copy)
                { return copy.snapshot.originalPath == first && copy.snapshot.text == alpha && copy.snapshot.bom; }) &&
                            std::ranges::any_of(copies,
                                                [&](const auto& copy)
                { return copy.snapshot.originalPath == second && copy.snapshot.text == beta && ! copy.snapshot.bom; }),
                        "restart discovers every modified source with its own original path and encoding");
                restarted.Stop();
                Await(app, [&] { return restarted.Finished(); });
                Require(questions == 0 && app.Activity().workspaceSwitches == 3 && app.Activity().workspaceRetainedPeak == 2 &&
                            foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()),
                        "all workspace switching and recovery checks remain hidden without save questions or desktop activation");
            }
        });
        test("Editor.WorkspaceOpenDocumentsOrderAndSelection",
             [&]
        {
            const auto directory = root / L"workspace-open-documents-order";
            const auto first = directory / L"Alpha.md", second = directory / L"Beta.md", third = directory / L"Gamma.md";
            WriteFixture(first, "# Alpha\nFirst note.\n");
            WriteFixture(second, "# Beta\nSecond note.\n");
            WriteFixture(third, "# Gamma\nThird note.\n");
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"workspace-open-documents-ui"));
            Success(app.Initialize(), "initialize hidden workspace order and selection app");
            Resize(app, 1360, 900);
            StoredDocument loaded;
            Success(LoadDocument(first, loaded), "read Alpha fixture");
            app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
            app.Execute(Command::Files);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->markdownCount == 3; });
            app.OpenPath(second);
            Await(app, [&] { return app._session.path == second && ! app._files.Busy(); });
            app.OpenPath(third);
            Await(app, [&] { return app._session.path == third && ! app._files.Busy(); });
            app.Prepare(256);

            const auto checkWorkspaceOrder = [&](std::initializer_list<std::pair<unsigned, std::wstring_view>> expectedRows)
            {
                Require(! app._filesModel.rows.empty() && app._filesModel.rows[0].id == Application::WorkspaceGroupId &&
                            app._filesModel.rows[0].text == Resource(IDS_OPENDOCUMENTS) && app._filesModel.rows.size() >= 1u + expectedRows.size(),
                        "Modified documents group appears in the Files tree");
                std::size_t idx = 1u;
                for (const auto& [docKey, docName] : expectedRows)
                {
                    Require(app._filesModel.rows[idx].id == Application::WorkspaceGroupId + docKey + 1u && app._filesModel.rows[idx].text == docName &&
                                app._filesModel.rows[idx].badgeIsDot,
                            "Modified documents keeps modified entries in stable workspace opening order");
                    ++idx;
                }
            };
            const auto folderRowId = [&](std::wstring_view name)
            {
                const auto row = std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [&](const auto& item) {
                    return item.id < Application::WorkspaceGroupId && item.text == name;
                });
                Require(row != app._filesModel.rows.end(), "find expected file row in the folder listing");
                return row->id;
            };
            const auto clickVisibleRow = [&](std::size_t visibleIndex)
            {
                const auto hit = app._filesTree->GetVisibleItemHitRect(visibleIndex);
                Require(hit.has_value(), "target Files row has visible hit geometry");
                const D2D1_POINT_2F point{(hit.value().left + hit.value().right) * 0.5f, (hit.value().top + hit.value().bottom) * 0.5f};
                Require(app._filesTree->OnMouseDown(app.Host(), point, false, 0), "press Files row");
                (void)app._filesTree->OnMouseUp(app.Host(), point, false, 0);
            };

            Require(std::ranges::none_of(app._filesModel.rows, [](const auto& item) { return item.id >= Application::WorkspaceGroupId; }),
                    "clean retained documents do not show the Modified documents section");
            Require(app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(folderRowId(L"Gamma.md")),
                    "opening from the folder listing selects that folder row");

            // Modify Alpha and Gamma while keeping Beta clean: Modified documents must show only Alpha and Gamma.
            clickVisibleRow(static_cast<std::size_t>(std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& item) {
                return item.id < Application::WorkspaceGroupId && item.text == L"Alpha.md";
            }) - app._filesModel.rows.begin()));
            Require(app._session.path == first && app.Document().ReplaceSelection(L" "), "dirty Alpha");
            clickVisibleRow(static_cast<std::size_t>(std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& item) {
                return item.id < Application::WorkspaceGroupId && item.text == L"Gamma.md";
            }) - app._filesModel.rows.begin()));
            Require(app._session.path == third && app.Document().ReplaceSelection(L" "), "dirty Gamma while Beta stays clean");
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {2u, L"Gamma.md"}});
            Require(std::ranges::none_of(app._filesModel.rows, [](const auto& item) { return item.id == Application::WorkspaceGroupId + 2u; }),
                    "unmodified Beta stays absent from Modified documents");

            // Now dirty Beta too so all three modified documents appear in stable workspace opening order.
            clickVisibleRow(static_cast<std::size_t>(std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& item) {
                return item.id < Application::WorkspaceGroupId && item.text == L"Beta.md";
            }) - app._filesModel.rows.begin()));
            Require(app._session.path == second && app.Document().ReplaceSelection(L" "), "dirty Beta");
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});

            clickVisibleRow(1u);
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._session.path == first && app._session.key == 0u &&
                        app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(Application::WorkspaceGroupId + 1),
                    "clicking Alpha in Modified documents activates Alpha without reordering or jumping selection to the folder row");

            clickVisibleRow(2u);
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._session.path == second && app._session.key == 1u &&
                        app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(Application::WorkspaceGroupId + 2),
                    "clicking Beta in Modified documents keeps stable row order and selects Beta's workspace row");

            const auto revision = app._folderSnapshot->revision;
            WriteFixture(directory / L"Delta.md", "# Delta\nFourth note.\n");
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->revision > revision && app._folderSnapshot->markdownCount == 4; });
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(Application::WorkspaceGroupId + 2),
                    "directory-watch refreshes preserve the Modified documents selection");

            app._filesModel.OnTreeToggleExpanded(Application::WorkspaceGroupId, false);
            Require(app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(folderRowId(L"Beta.md")),
                    "collapsing Modified documents falls back to the active file's visible folder row");
            app._filesModel.OnTreeToggleExpanded(Application::WorkspaceGroupId, true);
            app.Prepare(128);
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(folderRowId(L"Beta.md")),
                    "re-expanding Modified documents does not jump selection away from the folder row");

            clickVisibleRow(2u);
            Require(app._session.path == second && app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(Application::WorkspaceGroupId + 2),
                    "clicking the already-active Modified documents row selects it");
            app.RefreshFiles();
            Require(app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(Application::WorkspaceGroupId + 2),
                    "subsequent refreshes retain the explicitly selected Modified documents row");

            const auto gammaFolderIndex = static_cast<std::size_t>(std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& item) {
                return item.id < Application::WorkspaceGroupId && item.text == L"Gamma.md";
            }) - app._filesModel.rows.begin());
            clickVisibleRow(gammaFolderIndex);
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._session.path == third && app._session.key == 2u &&
                        app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(folderRowId(L"Gamma.md")),
                    "clicking Gamma in the folder listing selects the folder row while keeping Modified documents ordered");

            app.Document().SetSelection({0, 0});
            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft(), "open a guarded local draft to test refused workspace switching");
            clickVisibleRow(1u);
            checkWorkspaceOrder({{0u, L"Alpha.md"}, {1u, L"Beta.md"}, {2u, L"Gamma.md"}});
            Require(app._session.path == third && app._session.key == 2u &&
                        app._filesTree->GetSelectedItemId() == std::optional<std::uint64_t>(folderRowId(L"Gamma.md")),
                    "a refused workspace switch keeps the active document and restores its tree selection");
            app.Execute(Command::CloseDrawer);
            Require(! app.HasBlockDraft() && foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()),
                    "closing the local draft leaves the hidden workspace intact without desktop takeover");
        });
        test("Recovery.WorkspaceExplicitActionsProtectEveryDirtyDocument",
             [&]
        {
            const auto directory = root / L"workspace-actions";
            const auto first = directory / L"Alpha.md", second = directory / L"Beta.md";
            WriteFixture(first, "# Alpha\n");
            WriteFixture(second, "# Beta\n");
            unsigned questions{};
            bool cancelSecond            = true;
            auto options                 = Options(root, L"workspace-action-ui");
            options.unsavedPromptForTest = [&] { return ++questions == 2 && cancelSecond ? IDCANCEL : IDNO; };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize the hidden explicit-action workspace");
            StoredDocument loaded;
            Success(LoadDocument(first, loaded), "read the first explicit-action fixture");
            app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
            app.Document().SetSelection({7, 7});
            Require(app.Document().ReplaceSelection(L" changed"), "modify Alpha before switching");
            app.OpenPath(second);
            Await(app, [&] { return app._session.path == second && ! app._files.Busy(); });
            app.Document().SetSelection({6, 6});
            Require(app.Document().ReplaceSelection(L" changed"), "modify Beta before an explicit action");
            Require(questions == 0, "ordinary workspace navigation never opens the save question");
            app.Execute(Command::New);
            Require(questions == 2 && app._inactiveDocuments.size() == 1 && app.Dirty() && ! app.IsClosing(),
                    "Cancel on a later document cancels New without discarding either modified source");
            const auto retained = [&]
            {
                std::vector<std::wstring> source{app.Document().GetText()};
                for (const auto& document : app._inactiveDocuments)
                    source.push_back(document.state.Text());
                std::ranges::sort(source);
                return source;
            }();
            Require(retained == std::vector<std::wstring>{L"# Alpha changed\n", L"# Beta changed\n"}, "both exact sources remain retained after cancellation");
            cancelSecond = false;
            for (const auto command : {Command::Open, Command::OpenFolder})
            {
                questions = 0;
                app.Execute(command); // The owned headless picker cancels after the save decisions.
                Require(questions == 2 && app._inactiveDocuments.size() == 1 && app.Dirty() && ! app._files.Busy(),
                        "explicit Open File/Folder asks about every dirty document and picker cancellation retains the workspace");
            }
            questions         = 0;
            const auto window = app.Window();
            (void)SendMessageW(window, WM_CLOSE, 0, 0);
            Require(questions == 2 && app.IsClosing(), "Close asks about both dirty documents before accepting Discard");
            Await(app, [&] { return ! app.Window(); });
            StoredDocument original;
            Success(LoadDocument(first, original), "read Alpha after explicit discard");
            Require(original.text == L"# Alpha\n", "discard never writes Alpha's original file");
            Success(LoadDocument(second, original), "read Beta after explicit discard");
            Require(original.text == L"# Beta\n", "discard never writes Beta's original file");
        });
        test("Recovery.RestoreAllKeepsIndependentCopiesAndNavigation",
             [&]
        {
            auto options        = Options(root, L"restore-all-workspace", L"recovery");
            const auto data     = options.dataDirectory;
            const auto original = data / L"Documents/Notes.md";
            WriteFixture(original, "# Original\r\n");
            const std::array<RecoverySnapshot, 3> expected{{{L"First copy 東京\r\n", original, true, {15, 0}, 1},
                                                            {L"Second copy 😀\n", original, false, {14, 0}, 2},
                                                            {L"Untitled notes\n", {}, false, {0, 14}, 3}}};
            for (std::size_t index = 0; index < expected.size(); ++index)
                WriteFixture(data / L"Recovery" / std::format(L"session-{:032x}.rtd", index + 1), SerializeRecovery(expected[index]));
            Testing::ForegroundOwnershipObserver foreground;
            Application app(std::move(options));
            Success(app.Initialize(), "initialize three inactive recovery copies");
            Await(app, [&] { return app._recovery.Ready() && app.RecoveryCount() == 3; });
            app.Execute(Command::Recovery);
            Resize(app, 800, 560);
            Require(app._restoreAllRecovery->IsVisible() && app._recoveryPanel->IsVisible(),
                    "multiple copies expose the explicit Restore all action beside the read-only preview");
            for (std::size_t index = 3; index < 7; ++index)
            {
                const auto* button = app._recoveryPanel->GetLogicalChild(index);
                Require(Inside(button->GetBounds(), app._recoveryPanel->GetBounds()) && ! Overlap(button->GetBounds(), app._recoveryPreview->GetBounds()),
                        "all recovery actions fit the narrow panel without covering the preview");
                for (std::size_t previous = 3; previous < index; ++previous)
                    Require(! Overlap(button->GetBounds(), app._recoveryPanel->GetLogicalChild(previous)->GetBounds()),
                            "recovery actions never overlap each other");
            }
            app.Execute(Command::RestoreAllRecovery);
            Require(app._inactiveDocuments.size() == 2 && app.RecoveryCount() == 0 && app.Dirty() && app.RequiresSaveAs(),
                    "Restore all retains every copy in a separate dirty document");
            std::vector<unsigned> keys{app._session.key};
            for (const auto& document : app._inactiveDocuments)
            {
                Require(document.dirty && document.metadata.recovered, "every inactive restored copy requires Save As");
                keys.push_back(document.metadata.key);
            }
            std::vector<std::wstring> found;
            for (const auto key : keys)
            {
                app._filesModel.OnTreeItemInvoked(Application::WorkspaceGroupId + key + 1);
                const auto source       = app.Document().GetText();
                const auto expectedCopy = std::ranges::find_if(expected, [&](const auto& value) { return value.text == source; });
                Require(expectedCopy != expected.end() && app.Document().GetSelection() == expectedCopy->selection &&
                            app._session.path == expectedCopy->originalPath && app._session.bom == expectedCopy->bom && app.Dirty() && app.RequiresSaveAs() &&
                            ! app.Document().CanUndo(),
                        "workspace ids distinguish identical original paths and untitled copies with exact source, encoding and selection");
                found.push_back(source);
            }
            std::ranges::sort(found);
            Require(std::ranges::adjacent_find(found) == found.end() && found.size() == 3,
                    "all three independently specified sources are reachable without aliasing duplicate paths");
            app.CheckpointWorkspaceRecovery();
            Await(app, [&] { return app._recovery.Settled(); });
            Success(app._recovery.Status(), "all replacement copies become durable before their originals retire");
            StoredDocument unchanged;
            Success(LoadDocument(original, unchanged), "read original after restoring all copies");
            Require(unchanged.text == L"# Original\r\n" && foreground.NeverAcquiredForeground(),
                    "restore and navigation neither overwrite the original nor acquire foreground");
        });
        test("Recovery.SessionEndCheckpointsEveryDocumentAndCancellationKeepsEditing",
             [&]
        {
            unsigned questions{};
            auto options                 = Options(root, L"workspace-session-end");
            const auto data              = options.dataDirectory;
            options.unsavedPromptForTest = [&]
            {
                ++questions;
                return IDCANCEL;
            };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden session-end workspace");
            app.SetDocument(L"Alpha\r\n", data / L"Alpha.md", true);
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Edited ") && app.StartWorkspaceDocument(), "retain Alpha before editing Beta");
            app.SetDocument(L"Beta\n", data / L"Beta.md");
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Edited "), "modify Beta independently before shutdown");
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto window    = app.Window();
            const auto flags     = static_cast<LPARAM>(ENDSESSION_LOGOFF | ENDSESSION_CLOSEAPP);
            Require(SendMessageW(window, WM_QUERYENDSESSION, 0, flags) == TRUE && ! app.IsSessionEnding() && questions == 0,
                    "the session query permits shutdown without opening a save question or destroying the editor");
            (void)SendMessageW(window, WM_ENDSESSION, FALSE, flags);
            Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app.Document().CanUndo() &&
                        ! app.IsSessionEnding(),
                    "cancelled shutdown keeps exact source, selection and history live");
            Require(app.Document().ReplaceSelection(L"Later "), "editing remains available after a cancelled session end");
            (void)SendMessageW(window, WM_ENDSESSION, TRUE, flags);
            Require(app.IsSessionEnding() && app._sessionEndRecoveryStatus == S_OK && app._recovery.Settled() && questions == 0 && app.Window() == window &&
                        ! app.IsClosing(),
                    "final shutdown settles both recovery copies within its bounded allowance without destructive Close");
            const auto final = app.Document().Snapshot();
            app.Execute(Command::New);
            Require(app.Document().Snapshot().IsSameRevision(final), "queued commands cannot change the final shutdown checkpoint");
            std::vector<RecoverySnapshot> copies;
            for (const auto& entry : std::filesystem::directory_iterator(data / L"Recovery"))
                if (entry.path().extension() == L".rtd")
                {
                    std::string bytes;
                    Success(ReadBytes(entry.path(), bytes), "read a final session checkpoint");
                    RecoverySnapshot copy;
                    Success(ParseRecovery(bytes, copy), "validate a final session checkpoint");
                    copies.push_back(std::move(copy));
                }
            Require(copies.size() == 2 && std::ranges::any_of(copies, [](const auto& copy) { return copy.text == L"Edited Alpha\r\n" && copy.bom; }) &&
                        std::ranges::any_of(copies, [&](const auto& copy) { return copy.text == app.Document().GetText() && ! copy.bom; }),
                    "final session-end recovery contains every dirty document including edits after the cancelled query");
        });
        test("Editor.WorkspaceCapacityRefusesWithoutEvictingEdits",
             [&]
        {
            auto options = Options(root, L"workspace-capacity");
            Application app(std::move(options));
            Success(app.Initialize(), "initialize bounded document workspace");
            const auto directory = root / L"workspace-capacity-files";
            for (unsigned index = 0; index < RecoveryService::MaximumDocuments; ++index)
            {
                if (index)
                    Require(app.StartWorkspaceDocument(), "admit each document within the declared workspace capacity");
                app.SetDocument(std::format(L"Document {}\r\n", index), directory / std::format(L"Notes-{}.md", index));
                app.Document().SetSelection({0, 0});
                Require(app.Document().ReplaceSelection(L"Edited "), "retain an independent edit in every workspace slot");
            }
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto key       = app._session.key;
            Require(app._inactiveDocuments.size() == 63 && ! app.StartWorkspaceDocument() && app.Document().Snapshot().IsSameRevision(source) &&
                        app.Document().GetSelection() == selection && app._session.key == key && app._inactiveDocuments.size() == 63,
                    "capacity refusal leaves all documents, the active source and selection untouched");
            const auto extra = directory / L"Extra.md";
            WriteFixture(extra, "# Extra file\n");
            app.OpenPath(extra);
            Await(app, [&] { return ! app._files.Busy(); });
            Require(app.Document().Snapshot().IsSameRevision(source) && app._session.key == key && app._inactiveDocuments.size() == 63,
                    "a completed Files read cannot evict a dirty document when the workspace is full");
            for (const auto& document : app._inactiveDocuments)
                Require(document.dirty && document.state.CanUndo() && document.state.Text().starts_with(L"Edited Document "),
                        "every retained history and exact edited prefix survives both refusals");
            Require(app.Document().CanUndo() && app._feedback->GetText() == Resource(IDS_WORKSPACEFULL) && app.Activity().workspaceSwitches == 63 &&
                        app.Activity().workspaceRetainedPeak == 64,
                    "the capacity explanation remains visible and the active Undo history remains available");
        });
        test("Editor.OpenCompletionWaitsForLocalDraft",
             [&]
        {
            auto options      = Options(root, L"open-draft-completion");
            const auto target = root / L"open-draft-file/Next.md";
            WriteFixture(target, "# Next file\n");
            options.filePickerForTest = [&](bool saving) { return saving ? std::filesystem::path{} : target; };
            const auto gate = options.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
            Application app(std::move(options));
            Success(app.Initialize(), "initialize local draft versus asynchronous Open");
            const auto release        = wil::scope_exit([&]
            {
                (void)SetEvent(gate->proceed.get());
                if (WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0)
                    (void)WaitForSingleObject(gate->finished.get(), 10000);
            });
            const std::wstring source = L"# Local draft\r\n\r\nA note waiting for the reader.\r\n";
            app.SetDocument(source);
            app.Prepare(256);
            app.Execute(Command::Open);
            Require(WaitForSingleObject(gate->entered.get(), 10000) == WAIT_OBJECT_0, "the actual reader reaches its owned admission gate");
            const auto note = source.find(L"note");
            app.Document().SetSelection({note, note + 4});
            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft(), "open a pending ordinary-text draft while the read is in flight");
            app.BlockDraft()->SetTextAndNotify(L"A revised note");
            const auto entry = app.Document().Snapshot();
            Require(SetEvent(gate->proceed.get()) && WaitForSingleObject(gate->finished.get(), 10000) == WAIT_OBJECT_0,
                    "publish the actual reader completion after the draft exists");
            Await(app, [&] { return std::holds_alternative<Application::HeldOpenTx>(app._activeFileTx); });
            Require(app.HasBlockDraft() && app.Document().Snapshot().IsSameRevision(entry) && app.BlockDraft()->GetText() == L"A revised note" &&
                        app._session.path.empty(),
                    "Open completion holds its staged source without replacing or hiding the current draft");
            app.Execute(Command::CloseDrawer);
            Await(app, [&] { return app._session.path == target && std::holds_alternative<std::monostate>(app._activeFileTx); });
            Require(! app.HasBlockDraft() && app.Document().GetText() == L"# Next file\n" && ! app.Dirty(),
                    "cancelling the local draft resumes the exact staged Open once");
        });
        test("Editor.SplitMirrorFallbackRecoversAfterHiddenEdits",
             [&]
        {
            Application app(Options(root, L"mirror-fallback"));
            Success(app.Initialize(), "initialize hidden mirror fallback fixture");
            const std::wstring source = L"# Mirror\r\n\r\nBody 世界 😀\r\n";
            app.SetDocument(source);
            app.SetView(EditorView::Split);
            app.Prepare(256);
            app._mirror->SetViewMode(DxUi::RichText::ViewMode::Rich);
            app._editor->SetSelection({source.size(), source.size()});
            Require(app._editor->ReplaceSelection(L"fallback"), "edit with incompatible mirror profile");
            const auto fallback = app._editor->Snapshot();
            Require(app._mirror->GetText() == app._editor->GetText() && ! app._mirror->Snapshot().IsSameRevision(fallback) &&
                        app._mirror->GetSelection() == app._editor->GetSelection() && app._editor->CanUndo(),
                    "ordinary exact-text fallback preserves the primary source, ACPs and history");
            app.Prepare(256);
            const auto primaryFallbackSyntax = app._editor->GetMarkdownSyntax();
            const auto richMirrorSyntax      = app._mirror->GetMarkdownSyntax();
            Require(primaryFallbackSyntax && richMirrorSyntax && app._mirror->GetViewMode() == DxUi::RichText::ViewMode::Rich &&
                        primaryFallbackSyntax->Profile() == richMirrorSyntax->Profile() && &primaryFallbackSyntax->Nodes() != &richMirrorSyntax->Nodes(),
                    "a non-Source mirror view keeps its independent parser fallback");
            app.SetView(EditorView::Compose);
            app._mirror->SetViewMode(DxUi::RichText::ViewMode::Source);
            const auto hidden = app._mirror->Snapshot();
            const auto resets = app.Activity().mirrorResets;
            Require(app._editor->ReplaceSelection(L" hidden"), "edit the authoritative hidden source");
            Require(app._mirror->Snapshot().IsSameRevision(hidden) && app.Activity().mirrorResets == resets,
                    "hidden Compose defers mirror work after a fallback");
            app.SetView(EditorView::Split);
            Require(app._mirror->Snapshot().IsSameRevision(app._editor->Snapshot()) && app._mirror->GetSelection() == app._editor->GetSelection() &&
                        app.Activity().mirrorResets == resets + 1u,
                    "visible Split recovers snapshot sharing after hidden edits");
            app.Prepare(256);
            const auto primaryRecoveredSyntax = app._editor->GetMarkdownSyntax();
            const auto mirrorRecoveredSyntax  = app._mirror->GetMarkdownSyntax();
            Require(primaryRecoveredSyntax && mirrorRecoveredSyntax && primaryRecoveredSyntax->Revision() == app._editor->Revision() &&
                        mirrorRecoveredSyntax->Revision() == app._mirror->Revision() &&
                        primaryRecoveredSyntax->DocumentIdentity() != mirrorRecoveredSyntax->DocumentIdentity() &&
                        &primaryRecoveredSyntax->Nodes() == &mirrorRecoveredSyntax->Nodes(),
                    "the visible mirror adopts the latest canonical parse after hidden revisions");
            Require(app._editor->Undo() && app._editor->Undo() && app._editor->GetText() == source && ! app.Dirty() &&
                        app._mirror->Snapshot().IsSameRevision(app._editor->Snapshot()),
                    "both edits undo to the exact saved CRLF source without mirror history");
        });
        test("Editor.SplitMirrorNestedEditKeepsNewestPublication",
             [&]
        {
            Application app(Options(root, L"mirror-nested"));
            Success(app.Initialize(), "initialize nested mirror fixture");
            const std::wstring source = L"# Mirror\n\nBody\n";
            app.SetDocument(source);
            app.SetView(EditorView::Split);
            app.Prepare(256);
            app._editor->SetSelection({source.size(), source.size()});
            bool entered = false, edited = false;
            app._mirror->SetOnSelectionChanged([&]
            {
                if (entered)
                    return;
                entered        = true;
                const auto end = app._editor->Snapshot().Length();
                app._editor->SetSelection({end, end});
                edited = app._editor->ReplaceSelection(L" nested");
            });
            Require(app._editor->ReplaceSelection(L"outer"), "commit outer mirror-triggering edit");
            Require(entered && edited && app._editor->GetText() == source + L"outer nested" &&
                        app._mirror->Snapshot().IsSameRevision(app._editor->Snapshot()) && app._mirror->GetSelection() == app._editor->GetSelection() &&
                        app._derivedCaches.mirroredSource && app._derivedCaches.mirroredSource->IsSameRevision(app._editor->Snapshot()) &&
                        app._derivedCaches.selectionFactsSource && app._derivedCaches.selectionFactsSource->IsSameRevision(app._editor->Snapshot()) &&
                        app._derivedCaches.selectionFactsSelection == app._editor->GetSelection(),
                    "nested primary edit supersedes every outer source/selection cache publication");
            const auto before = app.Activity().selectionPresentationUpdates;
            app.SelectionChanged();
            Require(app.Activity().selectionPresentationUpdates == before, "the outer continuation never overwrites the newest selection facts");
            app._mirror->SetOnSelectionChanged({});
            Require(app._editor->Undo() && app._editor->GetText() == source + L"outer" && app._editor->Undo() && app._editor->GetText() == source &&
                        ! app.Dirty(),
                    "nested and outer edits keep separate authoritative undo records");
        });
        test("Editor.SplitMirrorCallbackCanRetireRoot",
             [&]
        {
            Application app(Options(root, L"mirror-retirement"));
            Success(app.Initialize(), "initialize mirror root-retirement fixture");
            app.SetDocument(L"# Mirror\n\nBody\n");
            app.Prepare(256);
            auto* const owner   = app._root;
            const auto lifetime = GetControlLifetimeToken(*owner);
            const auto resets   = app.Activity().mirrorResets;
            bool retired        = false;
            app._mirror->SetOnSelectionChanged([&]
            {
                retired = true;
                app.Host().SetRoot(std::make_unique<Surface>());
            });
            app.SetView(EditorView::Split);
            Require(retired && lifetime.expired() && ! app.IsClosing() && app.Host().GetRoot() != owner && app.Activity().mirrorResets == resets,
                    "owner retirement without Close aborts mirror cache publication and all caller continuation");
        });
        test("Editor.SplitViewTransitionCompletesAfterNestedEdit",
             [&]
        {
            const auto run = [&](std::wstring_view name, EditorView startingView)
            {
                auto options             = Options(root, name);
                options.preferencesProbe = true;
                const auto dataDirectory = options.dataDirectory;
                Application app(std::move(options));
                Success(app.Initialize(), "initialize nested view-transition fixture");
                const std::wstring source = L"# View transition\r\n\r\nBody 世界 😀\r\n";
                app.SetDocument(source);
                if (startingView == EditorView::Source)
                    app.SetView(EditorView::Source);
                app.Prepare(256);

                app.Document().SetSelection({source.size(), source.size()});
                Require(app.Document().ReplaceSelection(L" already dirty"), "make the source dirty before changing to Split");
                const auto beforeNestedEdit = source + L" already dirty";
                bool entered{}, nestedEdit{};
                app._mirror->SetOnSelectionChanged([&]
                {
                    if (entered)
                        return;
                    entered        = true;
                    const auto end = app.Document().Snapshot().Length();
                    app.Document().SetSelection({end, end});
                    nestedEdit = app.Document().ReplaceSelection(L" nested");
                });

                app.SetView(EditorView::Split);
                app._mirror->SetOnSelectionChanged({});
                const auto expected = beforeNestedEdit + L" nested";
                Require(entered && nestedEdit && app.Document().GetText() == expected &&
                            app.Document().GetSelection() == DxUi::RichText::Selection{expected.size(), expected.size()} && app.Document().CanUndo() &&
                            app.Dirty(),
                        "the nested edit remains authoritative and retains its undo history");
                Require(app.Preferences().view == EditorView::Split && app.Document().GetViewMode() == DxUi::RichText::ViewMode::Rich &&
                            app._mirror->IsVisible() && app.Host().GetFocusControl() == &app.Document(),
                        "the outer Split request completes visibility and logical focus after source supersession");
                Require(app._mirror->GetText() == expected && app._mirror->Snapshot().IsSameRevision(app.Document().Snapshot()) &&
                            app._mirror->GetSelection() == app.Document().GetSelection(),
                        "the visible mirror publishes the latest nested source and selection");
                CheckChrome(app);

                // A polling read can deny the worker's atomic replacement. Observe its
                // completion and result before opening the saved file.
                Await(app, [&] { return ! app._preferenceWriter.Busy(); }, std::chrono::seconds(2));
                const auto writeStatus = app._preferenceWriter.Status();
                Require(SUCCEEDED(writeStatus),
                        std::format("the completed Split preference write succeeds (HRESULT=0x{:08X})", static_cast<unsigned long>(writeStatus)));
                Settings persisted;
                Success(LoadSettings(dataDirectory, persisted), "read completed Split preferences");
                Require(persisted.view == EditorView::Split, "the completed Split transition reaches persisted preferences");
                Require(app.Document().Undo() && app.Document().GetText() == beforeNestedEdit && app.Document().Undo() && app.Document().GetText() == source &&
                            ! app.Dirty(),
                        "nested and pre-transition edits remain separately undoable to the original CRLF source");
            };

            run(L"compose-to-split-nested-edit", EditorView::Compose);
            run(L"source-to-split-nested-edit", EditorView::Source);
        });
        test("Editor.NestedViewRequestSupersedesOuterSplit",
             [&]
        {
            auto options             = Options(root, L"nested-source-view-wins");
            options.preferencesProbe = true;
            const auto dataDirectory = options.dataDirectory;
            Application app(std::move(options));
            Success(app.Initialize(), "initialize nested view-request fixture");
            const std::wstring source = L"# Nested view\n\nKeep this source unchanged.\n";
            app.SetDocument(source);
            app.Prepare(256);

            bool entered{}, nestedViewCompleted{};
            app._mirror->SetOnSelectionChanged([&]
            {
                if (entered)
                    return;
                entered = true;
                app.SetView(EditorView::Source);
                nestedViewCompleted = app.Preferences().view == EditorView::Source && app.Document().GetViewMode() == DxUi::RichText::ViewMode::Source &&
                                      ! app._mirror->IsVisible();
            });
            app.SetView(EditorView::Split);
            app._mirror->SetOnSelectionChanged({});

            Require(entered && nestedViewCompleted && app.Document().GetText() == source && ! app.Dirty() && app.Preferences().view == EditorView::Source &&
                        app.Document().GetViewMode() == DxUi::RichText::ViewMode::Source && ! app._mirror->IsVisible() &&
                        app.Host().GetFocusControl() == &app.Document(),
                    "a nested Source request remains the final view after the outer Split callback resumes");
            CheckChrome(app);

            Await(app, [&] { return ! app._preferenceWriter.Busy(); }, std::chrono::seconds(2));
            const auto writeStatus = app._preferenceWriter.Status();
            Require(SUCCEEDED(writeStatus),
                    std::format("the nested Source preference write succeeds (HRESULT=0x{:08X})", static_cast<unsigned long>(writeStatus)));
            Settings persisted;
            Success(LoadSettings(dataDirectory, persisted), "read completed nested Source preferences");
            Require(persisted.view == EditorView::Source, "the latest nested Source request reaches persisted preferences");
        });
        test("Window.Image2QuietChromeAndCompactInspector",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"image2-visual"));
            Success(app.Initialize(), "create the hidden image-2 comparison app");
            app.SetDocument(L"# Weekly report\n\n15 October 2026\n\n## What changed since last report\n\n- First item\n- Second item\n\n## In "
                            L"progress\n\nNext.\n");
            app.Execute(Command::Files);
            app.Prepare(256);
            const auto listCaret = app.Document().GetText().find(L"Second item");
            app.Document().SetSelection({listCaret, listCaret});
            app.SelectionChanged();
            const auto listKind         = Resource(IDS_BLOCKBULLETLIST);
            const std::size_t listItems = 2;
            Require(app._listSummary->IsVisible() && ! app._currentBlock->IsVisible() &&
                        app._listSummary->GetText() == std::vformat(Resource(IDS_BLOCKITEMINFO), std::make_wformat_args(listKind, listItems)),
                    "the inspector shows the actual enclosing list count from the canonical syntax tree");
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            for (const UINT dpi : {96u, 144u, 192u})
            {
                Resize(app, 1440, 900, dpi);
                CheckChrome(app);
                for (const auto* mode : app._modeButtons)
                    Require(mode->GetAppearance() == DxUi::ButtonAppearance::Tab, "view commands use the shared underlined-tab appearance");
                Require(app._filesToggle->GetAppearance() == DxUi::ButtonAppearance::Subtle &&
                            app._tools[9]->GetShortcutText() == Resource(IDS_FIND_SHORTCUT) &&
                            app._blockSourceAction->GetAppearance() == DxUi::ButtonAppearance::Outline,
                        "formatting, Find hints and inspector actions adopt shared quiet controls");
                Require(! app.Document().GetAppearance().drawBorder && ! app.Document().GetAppearance().drawFocusIndicator &&
                            ! app._outlineTree->GetAppearance().drawBackground && ! app._outlineTree->GetAppearance().drawBorder &&
                            ! app._outlineTree->GetAppearance().showExpanders && ! app._filesTree->GetAppearance().drawBorder,
                        "document and navigation panes have no nested input frames");
                const auto identity = app._title->GetBounds(), dot = app._dirtyDot->GetBounds();
                Require(! app._dirtyDot->IsVisible() && Near((identity.top + identity.bottom) / 2, (dot.top + dot.bottom) / 2) && dot.left > identity.right,
                        "clean title hides the dirty dot while keeping its header baseline aligned right of the filename");
                Require(app._blockSourceAction->GetBounds().bottom < 500 && ! app._overflowButton->IsVisible() && app._pinFiles->IsVisible(),
                        "short outlines keep block tools nearby and a full-width toolbar needs no overflow button");
                Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection,
                        "visual layout and DPI changes preserve the authoritative document and selection");
            }
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "image-2 verification preserves the desktop");
        });
        test("Editor.HiddenCurrentBlockPublishesLatestOnReveal",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"hidden-current-block", L"narrow"));
            Success(app.Initialize(), "create the hidden current-block publication app");
            Resize(app, 820, 560, 96, false);
            const auto hiddenPriorText              = std::wstring(app._currentBlock->GetText());
            const auto initiallyHiddenConstructions = app.Activity().currentBlockLabelConstructions;
            const std::wstring source               = L"First paragraph.\n\n- List alpha\n- List beta\n\nSecond paragraph is longer.\n";
            app.SetDocument(source);
            app.Prepare(256);
            const auto first  = source.find(L"First paragraph.");
            const auto list   = source.find(L"List alpha");
            const auto second = source.find(L"Second paragraph");
            Require(first != std::wstring::npos && list != std::wstring::npos && second != std::wstring::npos,
                    "the synthetic source contains distinct paragraph and list contexts");
            const auto setCaret = [&](std::size_t caret)
            {
                app.Document().SetSelection({caret, caret});
                app.SelectionChanged(false);
            };
            const auto expectedParagraph = [&]
            {
                const auto position = DocumentPositionAt(app._documentInfo, app.Document().GetSelection().caret);
                const auto format   = Resource(IDS_BLOCKINFO);
                const auto kind     = Resource(IDS_BLOCKPARAGRAPH);
                return std::vformat(format, std::make_wformat_args(kind, position.lineCharacters));
            };
            const auto title = std::wstring(app._currentBlockTitle->GetText());
            setCaret(first + 3);
            const auto firstFacts = expectedParagraph();
            Require(! app._sidebar->IsVisible() && app._currentBlock->IsVisible() && app._currentBlock->GetText() == hiddenPriorText &&
                        app.Activity().currentBlockLabelConstructions == initiallyHiddenConstructions && ! AccessibilityContainsName(app, firstFacts),
                    "the 820-DIP initial layout retains paragraph facts without exposing a hidden label to UI Automation");

            Resize(app, 1440, 900, 96, false);
            Require(app._sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == initiallyHiddenConstructions + 1 &&
                        app._currentBlock->GetText() == firstFacts && AccessibilityContainsName(app, firstFacts),
                    "the first sidebar reveal publishes the latest localized paragraph before the first render/provider read");

            const auto entrySource    = app.Document().Snapshot();
            const auto entrySelection = app.Document().GetSelection();
            app._settings.focusMode   = true;
            app.Layout();
            Require(! app._sidebar->IsVisible(), "Focus mode hides the sidebar before repeated source edits");
            const auto hiddenConstructions = app.Activity().currentBlockLabelConstructions;
            const auto entryUndo           = app.Document().CanUndo();
            for (unsigned cycle = 0; cycle < 64; ++cycle)
            {
                Require(app.Document().ReplaceSelection(L"x"), "admit a hidden paragraph character");
                Require(app.Document().Undo(), "undo the hidden paragraph character immediately");
                Require(app.Document().Snapshot().IsSameRevision(entrySource) && app.Document().GetSelection() == entrySelection &&
                            app.Document().CanUndo() == entryUndo && app.Document().CanRedo(),
                        "hidden edit/undo cycles restore the exact source, caret and undo admission");
                Require(app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                        "hidden edit/undo cycles do not format or publish current-block text");
            }

            app.Prepare(256);
            Require(app.Document().Snapshot().IsSameRevision(entrySource) && app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                    "settling the restored syntax while hidden preserves source and still defers label publication");
            setCaret(list + 5);
            const auto listKind         = Resource(IDS_BLOCKBULLETLIST);
            const std::size_t listItems = 2;
            const auto listText         = std::vformat(Resource(IDS_BLOCKITEMINFO), std::make_wformat_args(listKind, listItems));
            Require(! app._currentBlock->IsVisible() && app._listSummary->IsVisible() && app._listSummary->GetText() == listText &&
                        app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                    "hidden list context still updates its immediate local visibility and item summary without constructing the hidden block label");
            setCaret(second + 3);
            const auto latestFacts = expectedParagraph();
            Require(latestFacts != firstFacts && app._currentBlock->IsVisible() && ! app._sidebar->IsVisible() && app._currentBlock->GetText() == firstFacts &&
                        ! AccessibilityContainsName(app, latestFacts) && app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                    "a later paragraph replaces hidden pending list facts without publishing early");
            app._settings.focusMode = false;
            app.Layout();
            Require(app._sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == hiddenConstructions + 1 &&
                        app._currentBlock->GetText() == latestFacts && AccessibilityContainsName(app, latestFacts),
                    "revealing Focus publishes the newest paragraph rather than stale list or prior paragraph facts");
            Require(app._currentBlockTitle->GetText() == title, "deferred label publication preserves the static current-block title resource");

            const auto visibleListConstructions = app.Activity().currentBlockLabelConstructions;
            setCaret(list + 5);
            Require(! app._currentBlock->IsVisible() && app._listSummary->IsVisible() && app._listSummary->GetText() == listText &&
                        app.Activity().currentBlockLabelConstructions == visibleListConstructions,
                    "visible sidebar list selection keeps the list summary immediate and does not publish a locally hidden block label");
            const auto beforeVisibleParagraph = app.Activity().currentBlockLabelConstructions;
            setCaret(first + 3);
            auto visibleFacts = expectedParagraph();
            Require(app._currentBlock->IsVisible() && app._currentBlock->GetText() == visibleFacts &&
                        app.Activity().currentBlockLabelConstructions == beforeVisibleParagraph + 1,
                    "returning from list context publishes the current paragraph immediately");

            const auto editSource    = app.Document().Snapshot();
            const auto editSelection = app.Document().GetSelection();
            Require(app.Document().ReplaceSelection(L"z"), "edit a visible current-block paragraph");
            visibleFacts = expectedParagraph();
            Require(app._currentBlock->GetText() == visibleFacts, "a visible paragraph edit updates its label synchronously");
            Require(app.Document().Undo(), "undo the visible paragraph edit");
            visibleFacts = expectedParagraph();
            Require(app._currentBlock->GetText() == visibleFacts && app.Document().Snapshot().IsSameRevision(editSource) &&
                        app.Document().GetSelection() == editSelection && app.Document().CanRedo(),
                    "visible undo immediately restores the exact label, source, caret and redo admission");
            Require(app.Document().Redo(), "redo the visible paragraph edit");
            visibleFacts = expectedParagraph();
            Require(app._currentBlock->GetText() == visibleFacts, "visible redo immediately republishes its paragraph facts");
            Require(app.Document().Undo(), "restore the source after the visible redo oracle");
            visibleFacts = expectedParagraph();
            Require(app._currentBlock->GetText() == visibleFacts && app.Document().Snapshot().IsSameRevision(editSource) &&
                        app.Document().GetSelection() == editSelection,
                    "the complete visible edit/undo/redo sequence restores the original revision and caret");

            bool selectFirst{};
            const auto exerciseHiddenReveal = [&](const std::function<void()>& hide, const std::function<void()>& reveal, std::string_view cause)
            {
                hide();
                Require(! app._sidebar->IsVisible(), cause);
                const auto before = app.Activity().currentBlockLabelConstructions;
                setCaret((selectFirst ? first : second) + 3);
                selectFirst = ! selectFirst;
                Require(app.Activity().currentBlockLabelConstructions == before && app._currentBlock->IsVisible(),
                        "each hidden visibility cause defers a changed paragraph's label construction");
                const auto pending = expectedParagraph();
                reveal();
                Require(app._sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == before + 1 && app._currentBlock->GetText() == pending &&
                            AccessibilityContainsName(app, pending),
                        "each sidebar reveal publishes current text to the label and UI Automation");
            };
            exerciseHiddenReveal(
                [&]
            {
                app._settings.showOutline = false;
                app.Layout();
            },
                [&]
            {
                app._settings.showOutline = true;
                app.Layout();
            },
                "outline-off hides the sidebar");
            exerciseHiddenReveal([&] { app.SetView(EditorView::Split); }, [&] { app.SetView(EditorView::Compose); }, "Split view hides the sidebar");
            exerciseHiddenReveal(
                [&] { Resize(app, 800, 560, 96, false); }, [&] { Resize(app, 1440, 900, 96, false); }, "the narrow viewport hides the sidebar");
            exerciseHiddenReveal(
                [&]
            {
                app._settings.showFiles      = true;
                app._settings.filesPinMode   = PanePinMode::Auto;
                app._settings.outlinePinMode = PanePinMode::Auto;
                Resize(app, 1024, 900, 96, false);
                Require(app._leftRail->IsVisible() && app._rightRail->IsVisible() && ! app._filesPane->IsVisible() && ! app._sidebar->IsVisible() &&
                            ! app._filesSplitter->IsVisible(),
                        "the narrow viewport collapses both panes to edge rails without showing the splitter");
                app.RevealPaneOverlay(PaneSide::Left);
                Require(app._filesPane->IsVisible() && app._filesPane->borderRight && ! app._filesSplitter->IsVisible() && ! app._sidebar->IsVisible(),
                        "the file-overlay fixture is active rather than a normal left Files pane");
            },
                [&]
            {
                app._settings.showFiles    = false;
                app._settings.filesPinMode = PanePinMode::Unpinned;
                Resize(app, 1440, 900, 96, false);
            },
                "the narrow Files overlay hides the sidebar");
            Require(app.Document().Snapshot().IsSameRevision(entrySource) && app._currentBlockTitle->GetText() == title &&
                        foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()),
                    "all reveal paths preserve the document and static title without activating the hidden app");
        });
        test("Window.DesignRefreshMergedFrameHitTests",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"design-frame"));
            Success(app.Initialize(), "create hidden merged-frame app");
            Resize(app, 1440, 900);
            Require(HitControl(app, app._closeButton) == HTCLOSE && HitControl(app, app._maximizeButton) == HTMAXBUTTON &&
                        HitControl(app, app._minimizeButton) == HTMINBUTTON,
                    "caption controls report native close/maximize/minimize targets, including the snap-flyout target");
            Require(HitControl(app, app._saveButton) == HTCLIENT && HitControl(app, app._fileMenu) == HTCLIENT,
                    "Save and File remain client controls rather than draggable caption space");
            for (auto* mode : app._modeButtons)
                Require(HitControl(app, mode) == HTCLIENT && ! mode->GetAccessibleName().empty() && ! mode->GetAccessibleAutomationId().empty(),
                        "each visible named view selector remains a client target");
            Require(Hit(app, {28, 28}) == HTCAPTION && HitControl(app, app._editor) == HTCLIENT,
                    "document identity is draggable and the document remains ordinary client space");
            RECT frame{};
            Require(GetWindowRect(app.Window(), &frame) != FALSE, "read merged frame for border probes");
            const auto rawHit = [&](int x, int y) { return app.FrameHitTest(app.Window(), MAKELPARAM(static_cast<SHORT>(x), static_cast<SHORT>(y))); };
            Require(rawHit(frame.left + 1, frame.top + 1) == HTTOPLEFT && rawHit(frame.right - 1, frame.top + 1) == HTTOPRIGHT &&
                        rawHit(frame.left + 1, frame.bottom - 1) == HTBOTTOMLEFT && rawHit(frame.right - 1, frame.bottom - 1) == HTBOTTOMRIGHT,
                    "all four native resize corners remain reachable");
            Require(rawHit(frame.left + 1, (frame.top + frame.bottom) / 2) == HTLEFT && rawHit(frame.right - 1, (frame.top + frame.bottom) / 2) == HTRIGHT,
                    "native side resizing survives removal of the duplicate title bar");
            MINMAXINFO minimum{};
            (void)SendMessageW(app.Window(), WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&minimum));
            const auto dpi = GetDpiForWindow(app.Window());
            Require(minimum.ptMinTrackSize.x == MulDiv(800, static_cast<int>(dpi), 96) && minimum.ptMinTrackSize.y == MulDiv(560, static_cast<int>(dpi), 96),
                    "the native minimum tracking dimensions follow DPI");
            app.Execute(Command::Focus);
            Require(HitControl(app, app._exitFocus) == HTCLIENT && HitControl(app, app._maximizeButton) == HTMAXBUTTON,
                    "Focus mode retains Exit focus and native caption targets");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "frame probing never shows or activates its window");
        });
        test("Window.DesignRefreshCaptionButtonPointerLifecycle",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"caption-pointer-lifecycle"));
            Success(app.Initialize(), "create hidden caption-button lifecycle app");
            const auto source         = app.Document().Snapshot();
            const auto selection      = app.Document().GetSelection();
            auto* const keyboardFocus = app.Host().GetFocusControl();
            const bool canUndo = app.Document().CanUndo(), canRedo = app.Document().CanRedo();
            unsigned minimizeClicks{}, maximizeClicks{}, closeClicks{};
            app._minimizeButton->SetOnClick([&] { ++minimizeClicks; });
            app._maximizeButton->SetOnClick([&] { ++maximizeClicks; });
            app._closeButton->SetOnClick([&] { ++closeClicks; });
            Require(app._minimizeButton->GetAppearance() == DxUi::ButtonAppearance::Caption &&
                        app._maximizeButton->GetAppearance() == DxUi::ButtonAppearance::Caption &&
                        app._closeButton->GetAppearance() == DxUi::ButtonAppearance::CaptionClose,
                    "caption controls use the shared native-caption neutral and close treatments");
            Require(! app._minimizeButton->IsFocusable() && ! app._maximizeButton->IsFocusable() && ! app._closeButton->IsFocusable(),
                    "caption actions do not steal editor keyboard focus");
            const auto screenPoint = [&](D2D1_POINT_2F dip)
            {
                const auto point = app.Host().DipPointToScreenPoint(dip);
                return MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y));
            };
            const auto clientPoint = [&](D2D1_POINT_2F dip)
            {
                POINT point{GET_X_LPARAM(screenPoint(dip)), GET_Y_LPARAM(screenPoint(dip))};
                Require(ScreenToClient(app.Window(), &point) != FALSE, "convert caption release to client coordinates");
                return MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y));
            };
            const auto center = [](const DxUi::Control* control)
            {
                const auto bounds = control->GetBounds();
                return D2D1::Point2F((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
            };
            const auto captionDown = [&](DxUi::Button* button, LRESULT hit)
            {
                const auto point  = center(button);
                const auto packed = screenPoint(point);
                (void)SendMessageW(app.Window(), WM_NCMOUSEMOVE, static_cast<WPARAM>(hit), packed);
                (void)SendMessageW(app.Window(), WM_NCLBUTTONDOWN, static_cast<WPARAM>(hit), packed);
            };
            for (const UINT dpi : {96u, 144u, 192u})
            {
                minimizeClicks = maximizeClicks = closeClicks = 0;
                Resize(app, 1440, 900, dpi);
                const auto idlePath = root / std::format(L"caption-close-idle-{}.png", dpi);
                // Resize has settled content. Snapshot each synthetic input
                // step before pumping real-pointer leave for the hidden HWND.
                Require(app.Capture(idlePath, false), "capture idle caption close control without showing the window");
                const auto idlePixels  = ReadControlRaster(idlePath, app._closeButton->GetBounds(), app.Host().GetDpi());
                const auto closeHit    = HitControl(app, app._closeButton);
                const auto closeCenter = center(app._closeButton);
                (void)SendMessageW(app.Window(), WM_NCMOUSEMOVE, static_cast<WPARAM>(closeHit), screenPoint(closeCenter));
                Require(app._closeButton->IsHovered() && ! app._maximizeButton->IsHovered() && ! app._minimizeButton->IsHovered(),
                        std::format("nonclient hover targets only Close at {} DPI", dpi));
                const auto hoverPath = root / std::format(L"caption-close-hover-{}.png", dpi);
                Require(app.Capture(hoverPath, false), "capture hovered caption close control without showing the window");
                Require(app._closeButton->IsHovered(), "capturing a prepared interaction frame preserves its synthetic hover state");
                Require(idlePixels != ReadControlRaster(hoverPath, app._closeButton->GetBounds(), app.Host().GetDpi()),
                        std::format("close hover changes its rendered pixels at {} DPI", dpi));
                (void)SendMessageW(app.Window(), WM_NCMOUSELEAVE, 0, 0);
                Require(! app._closeButton->IsHovered() && ! app._maximizeButton->IsHovered() && ! app._minimizeButton->IsHovered(),
                        std::format("nonclient leave clears caption hover at {} DPI", dpi));

                captionDown(app._closeButton, HTCLOSE);
                Require(GetCapture() == app.Window(), "pressed Close owns the normal DxUi capture");
#if DXUI_ENABLE_DIAGNOSTICS
                Require(app._closeButton->DebugIsPressed(), "caption down enters the shared button pressed state");
#endif
                const auto pressedPath = root / std::format(L"caption-close-pressed-{}.png", dpi);
                Require(app.Capture(pressedPath, false), "capture pressed caption close control without showing the window");
                Require(idlePixels != ReadControlRaster(pressedPath, app._closeButton->GetBounds(), app.Host().GetDpi()),
                        std::format("pressed Close changes its rendered pixels at {} DPI", dpi));
                (void)SendMessageW(app.Window(), WM_CANCELMODE, 0, 0);
                Require(GetCapture() != app.Window() && closeClicks == 0, "cancel clears caption press without invoking Close");
#if DXUI_ENABLE_DIAGNOSTICS
                Require(! app._closeButton->DebugIsPressed(), "cancel resets the shared button pressed state");
#endif
                captionDown(app._closeButton, HTCLOSE);
                (void)SendMessageW(app.Window(), WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(500, 200));
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, MAKELPARAM(500, 200));
                Require(closeClicks == 0 && GetCapture() != app.Window(), "release outside a caption control cancels invocation");

                captionDown(app._minimizeButton, HTMINBUTTON);
                Require(GetCapture() == app.Window(), "a caption press transfers to the normal DxUi client capture");
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(center(app._minimizeButton)));
                Require(minimizeClicks == 1, "Minimize invokes exactly once through client release");
                captionDown(app._maximizeButton, HTMAXBUTTON);
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(center(app._maximizeButton)));
                Require(maximizeClicks == 1, "Maximize invokes exactly once through client release");
                // A native double-click replaces the second down message;
                // each completed caption-button click still invokes once.
                captionDown(app._maximizeButton, HTMAXBUTTON);
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(center(app._maximizeButton)));
                (void)SendMessageW(app.Window(), WM_NCLBUTTONDBLCLK, HTMAXBUTTON, screenPoint(center(app._maximizeButton)));
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(center(app._maximizeButton)));
                Require(maximizeClicks == 3, "the complete down/up/double-click/up sequence invokes once per completed click");
                captionDown(app._closeButton, HTCLOSE);
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(closeCenter));
                Require(closeClicks == 1, "Close invokes exactly once through client release");
                captionDown(app._closeButton, HTCLOSE);
                (void)SendMessageW(app.Window(), WM_NCLBUTTONUP, HTCLOSE, screenPoint(closeCenter));
                Require(closeClicks == 2, "guarded nonclient release fallback invokes one repeated Close action");
                const auto closePacked = screenPoint(closeCenter);
                (void)SendMessageW(app.Window(), WM_NCLBUTTONDBLCLK, HTCLOSE, closePacked);
                (void)SendMessageW(app.Window(), WM_LBUTTONUP, 0, clientPoint(closeCenter));
                Require(closeClicks == 3 && ! app.IsClosing(), "caption double-click follows the client lifecycle without native duplicate close");
                (void)SendMessageW(app.Window(), WM_NCMOUSELEAVE, 0, 0);
            }
            Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app.Document().CanUndo() == canUndo &&
                        app.Document().CanRedo() == canRedo && app.Host().GetFocusControl() == keyboardFocus,
                    "caption pointer handling preserves exact source, selection, undo history and keyboard focus");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()),
                    "caption pointer and raster coverage never shows or activates the hidden test window");
        });
        test("Window.DesignRefreshResponsiveLayoutAndDpi",
             [&]
        {
            Testing::ForegroundOwnershipObserver foregroundOwnership;
            const auto foreground = GetForegroundWindow();
            DWORD initialProcessAtEntry{};
            const bool initialOwnerAtEntry = GetWindowThreadProcessId(foreground, &initialProcessAtEntry) != 0;
            {
                Application app(Options(root, L"design-layout"));
                Success(app.Initialize(), "create hidden responsive-layout app");
                app.Execute(Command::Files);
                app._settings.filesPinMode   = PanePinMode::Auto;
                app._settings.outlinePinMode = PanePinMode::Auto;
                const auto source            = app.Document().Snapshot();
                const auto selected          = app.Document().GetSelection();
                const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
                for (const UINT dpi : {96u, 144u, 192u})
                    for (const unsigned width : {800u, 1120u, 1136u, 1142u, 1200u, 1300u, 1440u})
                    {
                        Resize(app, width, width == 800 ? 560 : 900, dpi);
                        CheckChrome(app);
                        const auto actual = app.Host().GetClientBoundsDip();
                        if (actual.right < 1170)
                        {
                            Require(! app._sidebar->IsVisible() && ! app._filesSplitter->IsVisible() && ! app._filesPane->IsVisible() &&
                                        app._leftRail->IsVisible() && app._rightRail->IsVisible() &&
                                        Near(app._page->GetBounds().left, CollapsedPaneRail::RailWidthDip),
                                    std::format("Collapsed rails width={} dpi={} files={} reading={} sidebar={} splitter={} pane={} left={}",
                                                actual.right,
                                                dpi,
                                                app._settings.filesWidth,
                                                app._settings.readingWidth,
                                                app._sidebar->IsVisible(),
                                                app._filesSplitter->IsVisible(),
                                                app._filesPane->IsVisible(),
                                                app._page->GetBounds().left)
                                        .c_str());
                            if (width == 800)
                                Require(app._overflowButton->IsVisible(), "the minimum-width command row moves hidden groups into overflow");
                        }
                        if (actual.right >= 1400)
                            Require(app._sidebar->IsVisible() && app._filesPane->IsVisible() && app._filesSplitter->IsVisible() &&
                                        ! app._leftRail->IsVisible() && ! app._rightRail->IsVisible() &&
                                        ! Overlap(app._filesPane->GetBounds(), app._page->GetBounds()) &&
                                        ! Overlap(app._sidebar->GetBounds(), app._page->GetBounds()),
                                    "wide layout shows both panes beside the canvas without overlap");
                        if (actual.right >= 1194 && actual.right <= 1375)
                            Require(! app._sidebar->IsVisible() && app._rightRail->IsVisible() && app._filesSplitter->IsVisible(),
                                    "the right panel collapses before the Files pane becomes an edge rail");
                        if (app._filesSplitter->IsVisible())
                            Require(app._editor->GetBounds().right - app._editor->GetBounds().left >=
                                            static_cast<float>(app.Preferences().readingWidth) - 0.1f &&
                                        Near(app.Document().GetAppearance().maximumTextWidthDip, static_cast<float>(app.Preferences().readingWidth)),
                                    "inline Files includes its splitter and preserves the requested document measure before overlaying");
                        app.SetView(EditorView::Split);
                        CheckChrome(app);
                        Require(app.SourceMirror().IsReadOnly() && ! app._sidebar->IsVisible(), "Split retains a derived read-only mirror and available width");
                        app.SetView(EditorView::Compose);
                        Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selected &&
                                    app.Document().CanUndo() == undo && app.Document().CanRedo() == redo,
                                "responsive pane transitions preserve source, selection and both history directions");
                    }
                const auto observedForeground = GetForegroundWindow();
                const bool testWindowVisible  = IsWindowVisible(app.Window()) != FALSE;
                if (! foregroundOwnership.NeverAcquiredForeground() || testWindowVisible)
                {
                    DWORD initialProcess{}, observedProcess{};
                    const bool initialOwnerAvailable  = GetWindowThreadProcessId(foreground, &initialProcess) != 0;
                    const bool observedOwnerAvailable = GetWindowThreadProcessId(observedForeground, &observedProcess) != 0;
                    Require(false,
                            std::format("DPI/layout scenarios preserve the desktop: foregroundChanged={} testWindowVisible={} "
                                        "observedIsTestWindow={} initialOwnerAvailable={} observedOwnerAvailable={} "
                                        "initialProcessIsTest={} observedProcessIsTest={} initialWindowStillExists={} "
                                        "initialForegroundWasNull={} observedForegroundWasNull={} initialOwnerAtEntry={} "
                                        "initialProcessAtEntryIsTest={}",
                                        observedForeground != foreground,
                                        testWindowVisible,
                                        observedForeground == app.Window(),
                                        initialOwnerAvailable,
                                        observedOwnerAvailable,
                                        initialProcess == GetCurrentProcessId(),
                                        observedProcess == GetCurrentProcessId(),
                                        IsWindow(foreground) != FALSE,
                                        foreground == nullptr,
                                        observedForeground == nullptr,
                                        initialOwnerAtEntry,
                                        initialProcessAtEntry == GetCurrentProcessId())
                                .c_str());
                }
            }
            Require(foregroundOwnership.NeverAcquiredForeground(), "responsive-layout teardown never activates the owned window");
        });
        test("Editor.DesignRefreshFocusViewsAndToggleStates",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"design-focus"));
            Success(app.Initialize(), "create hidden focus/view app");
            app.SetDocument(L"# Durable source\n\nThe writing stays exact.\n");
            app.Document().SetSelection({18, 18});
            Require(app.Document().ReplaceSelection(L"Careful "), "admit an undoable edit before presentation changes");
            const auto source    = app.Document().GetText();
            const auto revision  = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            app.Execute(Command::Files);
            for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
            {
                app.SetView(view);
                for (std::size_t index = 0; index < app._modeButtons.size(); ++index)
                    Require(app._modeButtons[index]->GetToggleState() == std::optional<bool>(index == static_cast<std::size_t>(view)),
                            "exactly one named segmented view reports pressed state");
                Require(! app._mirror->GetAppearance().bodyFontFamily.empty() && Near(app._mirror->GetAppearance().bodyFontSizeDip, 13.5f) &&
                            Near(app._mirror->GetAppearance().lineHeightMultiplier, 1.75f),
                        "the derived Source mirror uses the host monospace family and explicit source measure");
                if (view == EditorView::Source)
                    Require(app.Document().GetAppearance().bodyFontFamily == app._mirror->GetAppearance().bodyFontFamily &&
                                Near(app.Document().GetAppearance().bodyFontSizeDip, 13.5f) && Near(app.Document().GetAppearance().lineHeightMultiplier, 1.75f),
                            "the authoritative Source view uses the same monospace presentation as its derived mirror");
                if (view == EditorView::Split)
                {
                    const auto mirrored = app._mirror->Snapshot();
                    Require(! app._mirror->ReplaceSelection(L"Forbidden mirror edit") && app._mirror->Snapshot().IsSameRevision(mirrored) &&
                                app.Document().Snapshot().IsSameRevision(revision),
                            "the derived Split view rejects edits without changing either source or history");
                }
                app.Execute(Command::Focus);
                Require(app.Preferences().focusMode && ! app._commandBar->IsVisible() && ! app._sidebar->IsVisible() && ! app._filesPane->IsVisible() &&
                            ! app._positionStatus->IsVisible() && app._exitFocus->IsVisible(),
                        "Focus mode removes competing chrome while keeping its explicit exit");
                const auto edited             = Resource(IDS_CHANGED);
                const std::size_t editedWords = 7;
                Require(app._focusPill->IsVisible() && app._stats->IsVisible() && Inside(app._stats->GetBounds(), app._focusPill->GetBounds()) &&
                            app._stats->GetText() == std::vformat(Resource(IDS_FOCUSCOUNTS), std::make_wformat_args(editedWords, edited)),
                        "the focused status pill keeps the independent word count and Edited state without the full status row");
                Require(app.Document().IsFocusModeEnabled() && Near(app.Document().GetAppearance().maximumTextWidthDip, 680) &&
                            Near(app.Document().GetAppearance().bodyFontSizeDip, view == EditorView::Source ? 13.5f : 18.f),
                        "Focus mode uses the focused measure/type and shared editor dimming behavior");
                Require(app.DismissSurface() && ! app.Preferences().focusMode, "Escape exits Focus mode through the same dismiss path");
                Require(app.Document().GetText() == source && app.Document().Snapshot().IsSameRevision(revision) &&
                            app.Document().GetSelection() == selection && app.Document().CanUndo(),
                        "global view/focus changes preserve authoritative source, selection, revision and undo history");
            }
            Require(app.Document().Undo() && ! app.Dirty(), "one undo still returns to the saved source after every presentation switch");
            app.Execute(Command::Focus);
            const auto saved             = Resource(IDS_SAVED);
            const std::size_t savedWords = 6;
            Require(app._stats->GetText() == std::vformat(Resource(IDS_FOCUSCOUNTS), std::make_wformat_args(savedWords, saved)),
                    "undoing to the saved revision updates the focused pill to Saved and the original word count");
            Require(app.DismissSurface(), "leave the focused status-pill check through Escape routing");
            Require(foreground.NeverAcquiredForeground(), "hidden focus transitions change logical routing only");
        });
        test("Outline.DesignRefreshHierarchyCurrentAndFindCounts",
             [&]
        {
            {
                Testing::ForegroundOwnershipObserver foregroundOwnership;
                const auto foreground = GetForegroundWindow();
                DWORD initialForegroundProcess{};
                const bool initialForegroundOwnerAvailable = GetWindowThreadProcessId(foreground, &initialForegroundProcess) != 0;
                {
                    Application publicationApp(Options(root, L"design-outline-publication"));
                    Success(publicationApp.Initialize(), "create hidden outline-publication app");
                    std::wstring publicationSource = L"Preamble.\n\n# Root\n\nOpening.\n\n";
                    for (unsigned index = 0; index < 20; ++index)
                        publicationSource += L"## Section " + std::to_wstring(index + 1) + L"\n\nBody.\n\n";
                    publicationSource += L"# Last\n\nTail.\n";
                    publicationApp.SetDocument(publicationSource);
                    Resize(publicationApp, 1440, 560, 96);
                    Require(publicationApp._sidebar->IsVisible() && publicationApp._outlineModel.rows.size() == 22 &&
                                ! publicationApp._outlineTree->GetSelectedItemId() && ! publicationApp._outlineTree->GetFocusedItemId(),
                            "preamble has no current heading in either canonical tree identity");
                    const auto opening = publicationSource.find(L"Opening");
                    const std::optional<std::uint64_t> rootHeading{1}, lastHeading{22};
                    const auto entering = publicationApp.Activity().outlineSelectionUpdates;
                    publicationApp.Document().SetSelection({opening, opening});
                    Require(publicationApp.Activity().outlineSelectionUpdates == entering + 1 &&
                                publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == rootHeading,
                            "entering a section synchronously publishes both selected and focused heading identities");
                    const auto stable    = publicationApp.Activity().outlineSelectionUpdates;
                    const bool priorUndo = publicationApp.Document().CanUndo();
                    for (unsigned edit = 0; edit < 8; ++edit)
                    {
                        const DxUi::RichText::Selection caret{opening + edit % 2, opening + edit % 2};
                        publicationApp.Document().SetSelection(caret);
                        Require(publicationApp.Document().ReplaceSelection(L"x") && publicationApp.Document().Undo() &&
                                    publicationApp.Document().GetText() == publicationSource && publicationApp.Document().GetSelection() == caret &&
                                    publicationApp.Document().CanUndo() == priorUndo && publicationApp.Document().CanRedo() && ! publicationApp.Dirty(),
                                "prose edit and one undo preserve exact source, caret and history within the current section");
                        publicationApp.Prepare(64);
                    }
                    Require(publicationApp.Activity().outlineSelectionUpdates == stable && publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == rootHeading,
                            "caret, edit, undo and syntax refreshes never republish an unchanged current section");
                    publicationApp._outlineTree->SetFocusedItemId(2);
                    Require(publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == std::optional<std::uint64_t>{2},
                            "canonical tree focus can differ from its selected item");
                    publicationApp.Document().SetSelection({opening + 3, opening + 3});
                    Require(publicationApp.Activity().outlineSelectionUpdates == stable + 1 &&
                                publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == rootHeading,
                            "a changed focused identity is immediately reconciled even when selection already matches");
                    const auto tail = publicationSource.find(L"Tail");
                    publicationApp.Document().SetSelection({tail, tail});
                    Require(publicationApp.Activity().outlineSelectionUpdates == stable + 2 &&
                                publicationApp._outlineTree->GetSelectedItemId() == lastHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == lastHeading && publicationApp._outlineTree->GetFirstVisibleItemIndex() > 0,
                            "an actual section transition immediately selects, focuses and reveals the distant heading");
                    const auto rebuilt = publicationApp.Activity().outlineSelectionUpdates;
                    publicationApp._outlineModel.OnTreeToggleExpanded(1, false);
                    Require(publicationApp._outlineModel.rows.size() == 2 && publicationApp._outlineTree->GetFirstVisibleItemIndex() == 0,
                            "collapsing earlier descendants clamps the retained distant heading into the short model");
                    publicationApp._outlineModel.OnTreeToggleExpanded(1, true);
                    // The pinned Tree keeps the user's viewport across a model refresh; it no longer scrolls to unchanged focus.
                    Require(publicationApp.Activity().outlineSelectionUpdates == rebuilt && publicationApp._outlineModel.rows.size() == 22 &&
                                publicationApp._outlineTree->GetSelectedItemId() == lastHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == lastHeading && publicationApp._outlineTree->GetFirstVisibleItemIndex() == 0,
                            "canonical model refresh keeps the surviving current heading and the viewport without redundant app publication");
                    publicationApp.SetView(EditorView::Split);
                    publicationApp.Document().SetSelection({opening, opening});
                    const auto hiddenUpdates = publicationApp.Activity().outlineSelectionUpdates;
                    Require(! publicationApp._sidebar->IsVisible() && hiddenUpdates == rebuilt + 1 &&
                                publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == rootHeading,
                            "hidden Split presentation still receives actual current-section transitions immediately");
                    publicationApp.SetView(EditorView::Compose);
                    Require(publicationApp._sidebar->IsVisible() && publicationApp.Activity().outlineSelectionUpdates == hiddenUpdates &&
                                publicationApp._outlineTree->GetSelectedItemId() == rootHeading &&
                                publicationApp._outlineTree->GetFocusedItemId() == rootHeading && publicationApp.Document().GetText() == publicationSource &&
                                publicationApp.Document().CanUndo() == priorUndo && publicationApp.Document().CanRedo(),
                            "revealing the panel preserves its already current identity and the exact source/history");
                    publicationApp.Document().SetSelection({0, 0});
                    const auto cleared = publicationApp.Activity().outlineSelectionUpdates;
                    publicationApp.Document().SetSelection({1, 1});
                    Require(cleared == hiddenUpdates + 1 && publicationApp.Activity().outlineSelectionUpdates == cleared &&
                                ! publicationApp._outlineTree->GetSelectedItemId() && ! publicationApp._outlineTree->GetFocusedItemId(),
                            "leaving all sections clears both identities once and repeated preamble carets remain inert");
                }
                const auto observedForeground = GetForegroundWindow();
                if (! foregroundOwnership.NeverAcquiredForeground())
                {
                    DWORD observedProcess{};
                    const bool observedOwnerAvailable = GetWindowThreadProcessId(observedForeground, &observedProcess) != 0;
                    Require(false,
                            std::format("outline publication probes never acquire desktop focus: initialForegroundWasNull={} "
                                        "observedForegroundWasNull={} initialOwnerAtEntry={} observedOwnerAvailable={} "
                                        "initialProcessAtEntryIsTest={} observedProcessIsTest={} initialWindowStillExists={}",
                                        foreground == nullptr,
                                        observedForeground == nullptr,
                                        initialForegroundOwnerAvailable,
                                        observedOwnerAvailable,
                                        initialForegroundProcess == GetCurrentProcessId(),
                                        observedProcess == GetCurrentProcessId(),
                                        IsWindow(foreground) != FALSE));
                }
            }
            Application app(Options(root, L"design-outline"));
            Success(app.Initialize(), "create hidden outline-tree app");
            const std::wstring source = L"# Root\n\nOpening Needle.\n\n## Child\n\nNeedle.\n\n### Leaf\n\nDetail.\n\n# Next\n\nTail.\n\n#   \n";
            app.SetDocument(source);
            app.Prepare(256);
            app.Document().SetSelection({source.size(), source.size()});
            const auto analysesBeforeEdit = app.Activity().documentAnalysisUpdates;
            Require(app.Document().ReplaceSelection(L"history sentinel") && app.Activity().documentAnalysisUpdates == analysesBeforeEdit + 1 &&
                        app.Document().Undo() && app.Document().CanRedo(),
                    "outline presentation starts with an observable retained redo entry");
            Require(app._outlineModel.rows.size() == 4, "empty headings never generate blank outline rows");
            const auto& rows = app._outlineModel.rows;
            Require(rows[0].text == L"Root" && rows[0].depth == 0 && rows[1].text == L"Child" && rows[1].depth == 1 && rows[1].parentId == rows[0].id &&
                        rows[2].depth == 2 && rows[2].parentId == rows[1].id && rows[3].depth == 0,
                    "outline rows retain the actual heading hierarchy and stable parent identities");
            const auto rootId = rows[0].id;
            app._outlineModel.OnTreeToggleExpanded(rootId, false);
            Require(app._outlineModel.rows.size() == 2 && app._outlineModel.rows[0].text == L"Root" && ! app._outlineModel.rows[0].expanded &&
                        app._outlineModel.rows[1].text == L"Next",
                    "collapsing a parent hides only its descendants through the actual tree delegate");
            app._outlineModel.OnTreeToggleExpanded(rootId, true);
            Require(app._outlineModel.rows.size() == 4 && app._outlineModel.rows[0].expanded,
                    "expanding restores the nested hierarchy without rebuilding document source");
            const auto sectionCaret = source.find(L"Detail");
            app.Document().SetSelection({sectionCaret, sectionCaret});
            Require(app._outlineTree->GetSelectedItemId() == std::optional<std::uint64_t>(rows[2].id),
                    "the canonical selection callback exposes its current heading without consumer key/mouse hooks");
            const auto next = rows[3].id;
            app._outlineModel.OnTreeItemInvoked(next);
            Require(app.Document().GetSelection().caret == source.find(L"# Next") && app.Host().GetFocusControl() == &app.Document(),
                    "outline invocation navigates the source offset and returns logical editor routing");
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"Needle");
            app.Prepare(256);
            Require(app._outlineModel.rows[0].badgeText == L"2" && app._outlineModel.rows[1].badgeText == L"1",
                    "open Find shows real section match counts including nested headings");
            const auto queryWork = app.Activity();
            for (const auto caret : {source.find(L"Opening"), source.find(L"Detail"), source.find(L"Tail")})
            {
                app.Document().SetSelection({caret, caret});
                app.SelectionChanged();
            }
            const auto quietCaretWork = app.Activity();
            Require(quietCaretWork.findMatchRebuilds == queryWork.findMatchRebuilds &&
                        quietCaretWork.findSectionCountRebuilds == queryWork.findSectionCountRebuilds &&
                        quietCaretWork.findHighlightUpdates == queryWork.findHighlightUpdates,
                    "caret-only updates with Find open reuse source matches, section counts and unchanged highlights");
            const auto highlightBase = quietCaretWork.findHighlightUpdates;
            const auto firstMatch    = source.find(L"Needle");
            app.Document().SetSelection({firstMatch, firstMatch + 6});
            app.SelectionChanged();
            Require(app.Activity().findHighlightUpdates == highlightBase + 1, "changing the active current match refreshes the bounded highlight vector once");
            app.Document().SetSelection({source.find(L"Opening"), source.find(L"Opening")});
            app.SelectionChanged();
            const auto inactiveHighlightUpdates = app.Activity().findHighlightUpdates;
            for (const auto caret : {source.find(L"Detail"), source.find(L"Tail")})
            {
                app.Document().SetSelection({caret, caret});
                app.SelectionChanged();
            }
            Require(app.Activity().findHighlightUpdates == inactiveHighlightUpdates,
                    "moving among non-match carets does not reapply unchanged current-match highlights");
            const auto retained          = app.Document().Snapshot();
            const auto retainedSelection = app.Document().GetSelection();
            const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
            const auto unchanged = [&]
            {
                Require(app.Document().Snapshot().IsSameRevision(retained) && app.Document().GetText() == source &&
                            app.Document().GetSelection() == retainedSelection && app.Document().CanUndo() == undo && app.Document().CanRedo() == redo,
                        "outline collapse and Find presentation preserve exact source, selection and both history directions");
            };
            const auto sectionBuildsBeforeCollapse = app.Activity().findSectionCountRebuilds;
            app._outlineModel.OnTreeToggleExpanded(rootId, false);
            Require(app.Activity().findSectionCountRebuilds == sectionBuildsBeforeCollapse + 1,
                    "section totals are rebuilt for the changed outline identity, not for caret movement");
            Require(app._outlineModel.rows.size() == 2 && ! app._outlineModel.rows[0].expanded && app._outlineModel.rows[0].badgeText == L"2" &&
                        app._outlineModel.rows[1].text == L"Next",
                    "collapsing during Find hides descendants while the parent still counts every nested match");
            unchanged();
            app.Execute(Command::CloseDrawer);
            Require(app._outlineModel.rows.size() == 2 && ! app._outlineModel.rows[0].expanded &&
                        std::ranges::all_of(app._outlineModel.rows, [](const auto& row) { return row.badgeText.empty(); }),
                    "closing Find clears transient counts while preserving outline collapse");
            const auto closedWork = app.Activity();
            for (const auto caret : {source.find(L"Opening"), source.find(L"Detail"), source.find(L"Tail")})
            {
                app.Document().SetSelection({caret, caret});
                app.SelectionChanged();
                app.RefreshFind();
            }
            app.Document().SetSelection(retainedSelection);
            app.SelectionChanged();
            Require(app.Activity().findMatchRebuilds == closedWork.findMatchRebuilds &&
                        app.Activity().findSectionCountRebuilds == closedWork.findSectionCountRebuilds &&
                        app.Activity().findHighlightUpdates == closedWork.findHighlightUpdates,
                    "closed Find clears its presentation once and remains inert across caret and syntax refreshes");
            unchanged();
            app.Execute(Command::Find);
            Require(app._find.table.matches.size() == 2 && app._outlineModel.rows.size() == 2 && ! app._outlineModel.rows[0].expanded &&
                        app._outlineModel.rows[0].badgeText == L"2",
                    "reopening Find restores inclusive counts without overriding a collapsed parent");
            unchanged();
            app._outlineModel.OnTreeToggleExpanded(rootId, true);
            Require(app._outlineModel.rows.size() == 4 && app._outlineModel.rows[0].expanded && app._outlineModel.rows[0].badgeText == L"2" &&
                        app._outlineModel.rows[1].badgeText == L"1",
                    "expanding during Find restores descendants and their own section counts");
            unchanged();
            const auto replacementSource = L"# Fresh\n\nNeedle once.\n\n## Nested\n\nNeedle twice.\n";
            app.SetDocument(replacementSource);
            Require(app._find.table.matches.size() == 2 && app._find.table.source && app._find.table.source->IsSameRevision(app.Document().Snapshot()) &&
                        app._outlineModel.rows.size() == 2 && app._outlineModel.rows[0].badgeText == L"2" && app._outlineModel.rows[1].badgeText == L"1",
                    "document replacement rebuilds current source matches and section counts without stale cached badges");
            app.Execute(Command::CloseDrawer);
            const auto closedSourceWork       = app.Activity();
            const auto sourceBeforeClosedEdit = app.Document().Snapshot();
            app.Document().SetSelection({sourceBeforeClosedEdit.Length(), sourceBeforeClosedEdit.Length()});
            Require(app.Document().ReplaceSelection(L"Needle after refresh\n"), "closed Find source edit commits normally");
            app.Prepare(512);
            Require(! app._find.IsPresentationActive() && ! app._find.projections.hasHighlights &&
                        std::ranges::all_of(app._outlineModel.rows, [](const auto& row) { return row.badgeText.empty(); }) &&
                        app.Activity().findMatchRebuilds == closedSourceWork.findMatchRebuilds &&
                        app.Activity().findSectionCountRebuilds == closedSourceWork.findSectionCountRebuilds &&
                        app.Activity().findHighlightUpdates == closedSourceWork.findHighlightUpdates,
                    "closed Find stays inert through a source edit and completed syntax preparation");
            app.Execute(Command::Find);
            Require(app._find.table.matches.size() == 3 && app._find.table.source && app._find.table.source->IsSameRevision(app.Document().Snapshot()) &&
                        app._outlineModel.rows[0].badgeText == L"3" && app._outlineModel.rows[1].badgeText == L"2",
                    "reopening after closed source and syntax changes rebuilds current full matches and section totals");
            app.Host().SetFocusControl(&app.Document(), false);
            const auto committedPreviewSource    = app.Document().GetText();
            const auto committedPreviewSelection = app.Document().GetSelection();
            const auto previewChanges            = app.Activity().changes;
            const auto previewUndo = app.Document().CanUndo(), previewRedo = app.Document().CanRedo();
            DxUi::TextInputState entry;
            entry.text       = committedPreviewSource;
            entry.multiline  = true;
            entry.caretIndex = committedPreviewSelection.caret;
            if (committedPreviewSelection.anchor != committedPreviewSelection.caret)
                entry.selectionAnchorIndex = committedPreviewSelection.anchor;
            auto preview           = entry;
            const auto firstNeedle = committedPreviewSource.find(L"Needle");
            preview.text.replace(firstNeedle, 6, L"Noodle");
            const auto previewRebuilds        = app.Activity().findMatchRebuilds;
            const auto previewSectionRebuilds = app.Activity().findSectionCountRebuilds;
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false) && app._find.table.matches.size() == 2 &&
                        app.Document().GetSelection() == committedPreviewSelection && app._find.table.source &&
                        app._find.table.source->IsSameRevision(app.Document().Snapshot()) && app._outlineModel.rows[0].badgeText == L"2" &&
                        app._outlineModel.rows[1].badgeText == L"2" && app.Activity().findMatchRebuilds == previewRebuilds + 1 &&
                        app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 1,
                    "same-ACP non-notifying source preview rebuilds the exact current Find source and ranges");
            auto secondPreview = preview;
            secondPreview.text.replace(firstNeedle, 6, L"Nooble");
            const auto firstPreviewSource = app.Document().Snapshot();
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), secondPreview, false) && app._find.table.matches.size() == 2 &&
                        app.Document().GetSelection() == committedPreviewSelection && ! app.Document().Snapshot().IsSameRevision(firstPreviewSource) &&
                        app._find.table.source && app._find.table.source->IsSameRevision(app.Document().Snapshot()) &&
                        app.Activity().findMatchRebuilds == previewRebuilds + 2 && app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 2,
                    "successive same-ACP preview revisions do not reuse stale Find ranges");
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), entry, false) && app.Document().GetText() == committedPreviewSource &&
                        app.Document().GetSelection() == committedPreviewSelection && app._find.table.matches.size() == 3 && app._find.table.source &&
                        app._find.table.source->IsSameRevision(app.Document().Snapshot()) && app._outlineModel.rows[0].badgeText == L"3" &&
                        app._outlineModel.rows[1].badgeText == L"2" && app.Activity().findMatchRebuilds == previewRebuilds + 3 &&
                        app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 3,
                    "text-service cancellation restores Find matches and section totals from the entry source revision");
            Require(app.Activity().changes == previewChanges && app.Document().CanUndo() == previewUndo && app.Document().CanRedo() == previewRedo,
                    "transient Find preview and cancellation create no committed activity or history");
            const auto pendingSyntax = app.Document().GetMarkdownSyntax();
            const auto beforeSyntax  = app.Activity();
            app.Prepare(512);
            Require(app.Document().GetMarkdownSyntax() && app.Document().GetMarkdownSyntax() != pendingSyntax &&
                        app.Activity().syntaxPresentationUpdates > beforeSyntax.syntaxPresentationUpdates &&
                        app.Activity().documentAnalysisUpdates == beforeSyntax.documentAnalysisUpdates &&
                        app.Activity().findMatchRebuilds == beforeSyntax.findMatchRebuilds &&
                        app.Activity().findSectionCountRebuilds == beforeSyntax.findSectionCountRebuilds &&
                        app.Activity().findHighlightUpdates == beforeSyntax.findHighlightUpdates && app.Document().GetText() == committedPreviewSource &&
                        app.Document().GetSelection() == committedPreviewSelection && app.Document().CanUndo() == previewUndo &&
                        app.Document().CanRedo() == previewRedo,
                    "actual syntax publication reuses current open Find presentation without changing source, selection or history");

            Application previewApp(Options(root, L"design-outline-source-preview"));
            Success(previewApp.Initialize(), "create source-consistent outline preview app");
            const std::wstring previewSource = L"# Root\n\nNeedle root.\n\n## Child\n\nNeedle child.\n";
            previewApp.SetDocument(previewSource);
            previewApp.Prepare(256);
            previewApp.Execute(Command::Find);
            previewApp._findText->SetTextAndNotify(L"Needle");
            previewApp.Prepare(256);
            previewApp._outlineModel.OnTreeToggleExpanded(1, false);
            Require(previewApp._outlineModel.rows.size() == 1 && previewApp._outlineModel.rows.front().text == L"Root" &&
                        ! previewApp._outlineModel.rows.front().expanded,
                    "source-preview fixture begins with a deliberately collapsed committed section");
            const auto previewCaret = previewSource.find(L"Needle root") + 4;
            previewApp.Document().SetSelection({previewCaret, previewCaret});
            const auto caretAnalysis = previewApp.Activity().documentAnalysisUpdates;
            previewApp.Document().SetSelection({previewCaret + 1, previewCaret + 1});
            previewApp.SelectionChanged();
            Require(previewApp.Activity().documentAnalysisUpdates == caretAnalysis, "caret-only selection refresh reuses source-derived document analysis");
            previewApp.Document().SetSelection({previewCaret, previewCaret});
            const auto committedSnapshot  = previewApp.Document().Snapshot();
            const auto committedSelection = previewApp.Document().GetSelection();
            const auto committedChanges   = previewApp.Activity().changes;
            const auto committedRecovery  = previewApp.Activity().recoverySubmissions;
            const auto committedUndo = previewApp.Document().CanUndo(), committedRedo = previewApp.Document().CanRedo();
            const auto analysisBaseline = previewApp.Activity().documentAnalysisUpdates;
            DxUi::TextInputState headingPreviewEntry;
            headingPreviewEntry.text       = previewSource;
            headingPreviewEntry.multiline  = true;
            headingPreviewEntry.caretIndex = previewCaret;
            auto firstPreview              = headingPreviewEntry;
            firstPreview.text.replace(0, 6, L"Plain "); // Same ACP, but the first heading marker is no longer present.
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), firstPreview, false) &&
                        previewApp.Document().GetText() == firstPreview.text && previewApp.Document().GetSelection() == committedSelection &&
                        previewApp._documentInfo.headings.size() == 1 && previewApp._documentInfo.headings.front().text == L"Child" &&
                        previewApp._documentInfo.headings.front().offset == firstPreview.text.find(L"## Child") &&
                        previewApp._documentInfo.lineIndex == AnalyzeDocument(firstPreview.text).lineIndex && previewApp._outlineModel.rows.size() == 1 &&
                        previewApp._outlineModel.rows.front().text == L"Child" && previewApp._outlineModel.rows.front().badgeText == L"1" &&
                        previewApp.Activity().documentAnalysisUpdates == analysisBaseline + 1,
                    "same-ACP heading-marker preview refreshes current outline identity, offsets and Find section counts");
            auto shiftedPreview     = firstPreview;
            const auto childHeading = shiftedPreview.text.find(L"## Child");
            shiftedPreview.text.insert(childHeading, L"\n"); // Add a source boundary before the child section's match.
            const auto secondPreviewAnalysis = previewApp.Activity().documentAnalysisUpdates;
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), shiftedPreview, false) &&
                        previewApp._documentInfo.headings.size() == 1 && previewApp._documentInfo.headings.front().offset == childHeading + 1 &&
                        previewApp._documentInfo.lineIndex == AnalyzeDocument(shiftedPreview.text).lineIndex && previewApp._outlineModel.rows.size() == 1 &&
                        previewApp._outlineModel.rows.front().text == L"Child" && previewApp._outlineModel.rows.front().badgeText == L"1" &&
                        previewApp.Activity().documentAnalysisUpdates == secondPreviewAnalysis + 1,
                    "successive preview analysis follows a shifted heading boundary while retaining current match totals");
            const auto cancelAnalysis = previewApp.Activity().documentAnalysisUpdates;
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), headingPreviewEntry, false) &&
                        previewApp.Document().GetText() == committedSnapshot.Text() && previewApp.Document().GetSelection() == committedSelection &&
                        previewApp._documentInfo.headings.size() == 2 && previewApp._documentInfo.headings[0].text == L"Root" &&
                        previewApp._documentInfo.headings[0].offset == 0 && previewApp._documentInfo.headings[1].text == L"Child" &&
                        previewApp._documentInfo.headings[1].offset == previewSource.find(L"## Child") &&
                        previewApp._documentInfo.lineIndex == AnalyzeDocument(previewSource).lineIndex && previewApp._outlineModel.rows.size() == 1 &&
                        previewApp._outlineModel.rows.front().text == L"Root" && ! previewApp._outlineModel.rows.front().expanded &&
                        previewApp._outlineModel.rows.front().badgeText == L"2" && previewApp._find.table.matches.size() == 2 &&
                        previewApp._find.table.source && previewApp._find.table.source->IsSameRevision(previewApp.Document().Snapshot()) &&
                        previewApp.Activity().documentAnalysisUpdates == cancelAnalysis + 1,
                    "cancellation restores entry headings, offsets, collapsed outline and Find badges from its exact source revision");
            Require(previewApp.Activity().changes == committedChanges && previewApp.Activity().recoverySubmissions == committedRecovery &&
                        previewApp.Document().CanUndo() == committedUndo && previewApp.Document().CanRedo() == committedRedo,
                    "transient heading previews and cancellation preserve committed activity, recovery and undo/redo history");
            const auto unchangedAnalysis = previewApp.Activity().documentAnalysisUpdates;
            previewApp.Changed();
            Require(previewApp.Activity().documentAnalysisUpdates == unchangedAnalysis && previewApp._outlineModel.rows.size() == 1 &&
                        ! previewApp._outlineModel.rows.front().expanded && previewApp.Document().GetText() == previewSource &&
                        previewApp.Document().GetSelection() == committedSelection && previewApp.Document().CanUndo() == committedUndo &&
                        previewApp.Document().CanRedo() == committedRedo,
                    "same-revision committed analysis retains the collapsed outline and exact source/history");
            auto committedHeadingPreview = headingPreviewEntry;
            committedHeadingPreview.text.replace(2, 4, L"Tree");
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), committedHeadingPreview, false) &&
                        previewApp._documentInfo.headings[0].text == L"Tree" && previewApp._outlineModel.rows.size() == 2,
                    "a changed heading preview temporarily exposes its current outline");
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), headingPreviewEntry, false) &&
                        previewApp._outlineModel.rows.size() == 1 && ! previewApp._outlineModel.rows.front().expanded,
                    "restore-entry reinstates the committed collapse before the result transaction");
            Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), committedHeadingPreview, true) &&
                        previewApp._documentInfo.headings[0].text == L"Tree" && previewApp._outlineModel.rows.size() == 2 &&
                        previewApp.Document().GetSelection() == committedSelection && previewApp.Document().Undo() &&
                        previewApp.Document().GetText() == previewSource && ! previewApp.Document().CanUndo() &&
                        previewApp._documentInfo.headings[0].text == L"Root" && previewApp.Document().Redo() &&
                        previewApp.Document().GetText() == committedHeadingPreview.text && previewApp._documentInfo.headings[0].text == L"Tree" &&
                        previewApp._documentInfo.lineIndex == AnalyzeDocument(committedHeadingPreview.text).lineIndex,
                    "preview result commit clears changed collapse identity and remains exactly one source undo/redo transaction");
            previewApp.SetDocument(std::wstring(2048, L'\n'));
            const auto manyLineCapacity = previewApp._documentInfo.lineIndex.capacity();
            previewApp.SetDocument(L"Z\r\n");
            Require(previewApp._documentInfo.lineIndex == std::vector<DocumentLine>{{0, 1}, {3, 0}} && previewApp._documentInfo.lineIndex.capacity() <= 64 &&
                        previewApp._documentInfo.lineIndex.capacity() < manyLineCapacity && previewApp._documentLineIndexStorage.Stats().spareCapacity <= 64 &&
                        previewApp.Document().GetText() == L"Z\r\n" && ! previewApp.Document().CanUndo() && ! previewApp.Document().CanRedo(),
                    "small document replacement releases the predecessor's large line index and retains exact clean history");
        });
        test("Editor.DocumentAnalysisReusesBoundedLineIndexStorage",
             [&]
        {
            Application app(Options(root, L"design-line-index-reuse"));
            Success(app.Initialize(), "create hidden line-index storage app");
            const std::wstring source = L"α cat 😀 cat\r\ncat";
            app.SetDocument(source);
            app.Prepare(128);
            const auto warmAt = source.find(L"cat");
            app.Document().SetSelection({warmAt, warmAt + 3});
            Require(app.Document().ReplaceSelection(L"dog") && app.Document().Undo() && app.Document().GetText() == source,
                    "one fixture edit/undo warms the application storage path");
            app.SetDocument(source);
            app.Prepare(128);
            Require(! app.Document().CanUndo() && ! app.Document().CanRedo(), "the storage fixture starts with clean source history");
            const auto initialSnapshot        = app.Document().Snapshot();
            const auto initialStats           = app._documentLineIndexStorage.Stats();
            const auto initialAnalysisUpdates = app.Activity().documentAnalysisUpdates;
            const auto matchesAnalysisFacts   = [&](std::wstring_view text)
            {
                const auto expected = AnalyzeDocument(text);
                return app._documentInfo.words == expected.words && app._documentInfo.characters == expected.characters &&
                       app._documentInfo.lines == expected.lines && app._documentInfo.newlineKind == expected.newlineKind &&
                       app._documentInfo.lineIndex == expected.lineIndex && app._documentInfo.headings.size() == expected.headings.size();
            };
            app.Document().SetSelection({0, 0});
            app.SelectionChanged();
            Require(app.Document().Snapshot().IsSameRevision(initialSnapshot) && app._derivedCaches.documentAnalysisSource &&
                        app._derivedCaches.documentAnalysisSource->IsSameRevision(initialSnapshot) && matchesAnalysisFacts(source) &&
                        app.Activity().documentAnalysisUpdates == initialAnalysisUpdates && ! app.Document().CanUndo() && ! app.Document().CanRedo(),
                    "caret-only refresh preserves the published source facts and clean history");
            for (unsigned cycle = 0; cycle < 4; ++cycle)
            {
                const auto editAt = source.find(L"cat");
                app.Document().SetSelection({editAt, editAt + 3});
                auto editedSource = source;
                editedSource.replace(editAt, 3, L"dog");
                Require(app.Document().ReplaceSelection(L"dog") && app.Document().GetText() == editedSource && matchesAnalysisFacts(editedSource) &&
                            app._derivedCaches.documentAnalysisSource && app._derivedCaches.documentAnalysisSource->IsSameRevision(app.Document().Snapshot()),
                        "each same-line-count edit publishes exact facts for its source revision");
                Require(app.Document().Undo() && app.Document().GetText() == source && matchesAnalysisFacts(source) &&
                            app._derivedCaches.documentAnalysisSource && app._derivedCaches.documentAnalysisSource->IsSameRevision(app.Document().Snapshot()) &&
                            ! app.Document().CanUndo() && app.Document().CanRedo(),
                        "undo restores exact published line facts and source text");
            }
            const auto reusedStats = app._documentLineIndexStorage.Stats();
            Require(reusedStats.reuseCount >= initialStats.reuseCount + 8 && reusedStats.capacityGrowthCount == initialStats.capacityGrowthCount &&
                        reusedStats.capacityNormalizationCount == initialStats.capacityNormalizationCount &&
                        app.Activity().documentAnalysisUpdates == initialAnalysisUpdates + 8 && app.Document().GetText() == source &&
                        matchesAnalysisFacts(source) && ! app.Document().CanUndo() && app.Document().CanRedo(),
                    "application edit/undo transactions reuse warmed bounded storage without changing exact source facts");
        });
        test("Editor.ExplicitLinksConfirmAndDispatchWithoutEditing",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            struct Observation
            {
                Observation()                              = default;
                Observation(const Observation&)            = delete;
                Observation& operator=(const Observation&) = delete;
                Observation(Observation&&)                 = delete;
                Observation& operator=(Observation&&)      = delete;
                std::mutex mutex;
                std::wstring destination;
                unsigned launches{};
                DWORD thread{};
                bool sta{};
                HRESULT nextResult = S_OK;
            };
            const auto observed = std::make_shared<Observation>();
            auto options        = Options(root, L"explicit-link-dispatch");
            bool approve{};
            unsigned confirmations{};
            Application* owner{};
            options.confirmLinkForTest = [&](std::wstring_view destination)
            {
                ++confirmations;
                Require(destination == L"https://example.invalid/a?x=1&y=2", "confirmation owns the independently specified resolved URL");
                if (approve)
                    owner->Execute(Command::OpenLink); // Duplicate intent must keep the existing confirmation.
                return approve;
            };
            options.launchLinkForTest = [observed](std::wstring_view destination) -> HRESULT
            {
                APTTYPE type{};
                APTTYPEQUALIFIER qualifier{};
                const HRESULT apartment = CoGetApartmentType(&type, &qualifier);
                std::lock_guard lock(observed->mutex);
                observed->destination = destination;
                ++observed->launches;
                observed->thread = GetCurrentThreadId();
                observed->sta    = SUCCEEDED(apartment) && (type == APTTYPE_STA || type == APTTYPE_MAINSTA);
                return observed->nextResult;
            };
            auto gate = std::make_shared<AsyncFiles::TestGate>();
            (void)SetEvent(gate->proceed.get());
            options.linkGateForTest = gate;
            Application app(std::move(options));
            owner = &app;
            Success(app.Initialize(), "create hidden explicit-link owner");
            const std::wstring source = L"[Notebook](https://example.invalid/a?x=1&amp;y=2)\r\n";
            app.SetDocument(source);
            app.Document().SetSelection({source.size(), source.size()});
            Require(app.Document().ReplaceSelection(L"!") && app.Document().Undo(), "retain redo before external navigation");
            app.Document().SetSelection({9u, 1u});
            app.Prepare(512);
            const auto snapshot = app.Document().Snapshot();
            const auto selected = app.Document().GetSelection();
            const auto items    = app.ContextMenuItems(false, false);
            Require(std::ranges::any_of(items,
                                        [](const auto& item)
            { return item.commandId == static_cast<int>(Command::OpenLink) + 1 && item.text == Resource(IDS_OPENLINK); }),
                    "the real source-context menu exposes its localized explicit Open link command");
            app.Execute(Command::OpenLink);
            Require(confirmations == 1u && ! app._externalLink && app.Document().Snapshot().IsSameRevision(snapshot),
                    "declining confirmation starts no worker and retains exact source");
            approve             = true;
            const auto semantic = app.Document().GetSemanticDocument();
            const auto link     = app.CurrentLinkElement(semantic);
            Require(link && app.Document().HasLinkActivationHandler() && app.Document().NavigateSemanticElement(app.Host(), semantic, link.value()),
                    "the public activation route used by Ctrl-click delegates to the installed application confirmation handler");
            Require(WaitForSingleObject(gate->finished.get(), 3000) == WAIT_OBJECT_0, "the owned STA dispatch completes its fake handoff");
            app.Prepare(64);
            {
                std::lock_guard lock(observed->mutex);
                Require(confirmations == 2u && observed->launches == 1u && observed->destination == L"https://example.invalid/a?x=1&y=2" &&
                            observed->thread != GetCurrentThreadId() && observed->sta,
                        "confirmed activation hands exact owned URL bytes to a fresh STA outside the UI thread");
                observed->nextResult = E_ACCESSDENIED;
            }
            Require(app._feedback->GetText() == Resource(IDS_LINKOPENED) && app.Document().Snapshot().IsSameRevision(snapshot) &&
                        app.Document().GetSelection() == selected && ! app.Document().CanUndo() && app.Document().CanRedo() && ! app.Dirty(),
                    "successful navigation changes neither source, directional selection, history nor saved state");
            auto failedGate = std::make_shared<AsyncFiles::TestGate>();
            (void)SetEvent(failedGate->proceed.get());
            app._options.linkGateForTest = failedGate;
            app.Execute(Command::OpenLink);
            Require(WaitForSingleObject(failedGate->finished.get(), 3000) == WAIT_OBJECT_0, "the second handoff publishes its actual failed result");
            app.Prepare(64);
            Require(app._feedback->GetText() == Resource(IDS_LINKFAILED) && app.Document().Snapshot().IsSameRevision(snapshot) &&
                        app.Document().GetSelection() == selected && app.Document().CanRedo(),
                    "a failed external handoff reports status while retaining exact source and histories");
            for (const auto destination : {L"https://example.invalid/notes", L"HTTP://example.invalid", L"mailto:writer@example.invalid"})
                Require(Application::SupportedExternalLink(destination), "the explicit scheme policy accepts its supported destinations");
            for (const auto destination : {L"https://",
                                           L"https://?query",
                                           L"file:///C:/tool.exe",
                                           L"shell:AppsFolder",
                                           L"javascript:alert(1)",
                                           L"data:text/plain,writing",
                                           L"relative.md",
                                           L"https://example.invalid/a b",
                                           L"https://example.invalid/\"a"})
                Require(! Application::SupportedExternalLink(destination), "unsupported schemes and malformed shell arguments remain inert");
            constexpr wchar_t embeddedNull[] = L"https://example.invalid/a\0b";
            Require(! Application::SupportedExternalLink(std::wstring_view(embeddedNull, std::size(embeddedNull) - 1u)),
                    "embedded NUL cannot shorten the confirmed external destination");
            Require(foreground.NeverAcquiredForeground(), "link policy and fake shell handoffs never acquire desktop foreground");
        });
        test("Editor.LinkConfirmationRejectsSupersededAndClosedOwners",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            for (const unsigned mode : {0u, 1u, 2u})
            {
                auto options = Options(root, std::format(L"link-confirmation-context-{}", mode));
                Application* owner{};
                auto launches             = std::make_shared<std::atomic<unsigned>>(0u);
                options.launchLinkForTest = [launches](std::wstring_view) -> HRESULT
                {
                    ++*launches;
                    return S_OK;
                };
                options.confirmLinkForTest = [&](std::wstring_view)
                {
                    if (mode == 0u)
                        owner->SetDocument(L"Newer writing");
                    else if (mode == 1u)
                        owner->Document().SetSelection({0u, 0u});
                    else
                        owner->BeginClose();
                    return true;
                };
                Application app(std::move(options));
                owner = &app;
                Success(app.Initialize(), "create hidden reentrant confirmation owner");
                app.SetDocument(L"[Notes](https://example.invalid/notes)\r\n");
                app.Document().SetSelection({1u, 1u});
                app.Prepare(512);
                app.Execute(Command::OpenLink);
                Require(launches->load() == 0u && ! app._externalLink, "a superseded or closed confirmation never admits a shell dispatch");
                if (mode == 0u)
                    Require(app.Document().GetText() == L"Newer writing", "confirmation reentrancy retains the newer source");
                else if (mode == 1u)
                    Require(app.Document().GetSelection() == DxUi::RichText::Selection{0u, 0u}, "confirmation reentrancy retains the newer selection");
                else
                    Require(app.Host().GetHwnd() == nullptr, "a confirmation may close its owner without touching detached controls");
            }
            Require(foreground.NeverAcquiredForeground(), "reentrant confirmation checks retain desktop foreground");
        });
        test("Editor.PendingLinkDispatchRetiresAfterApplicationClose",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            auto gate                  = std::make_shared<AsyncFiles::TestGate>();
            const auto release         = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
            auto launches              = std::make_shared<std::atomic<unsigned>>(0u);
            auto options               = Options(root, L"link-dispatch-close");
            options.confirmLinkForTest = [](std::wstring_view) { return true; };
            options.launchLinkForTest  = [launches](std::wstring_view) -> HRESULT
            {
                ++*launches;
                return S_OK;
            };
            options.linkGateForTest = gate;
            auto app                = std::make_unique<Application>(std::move(options));
            Success(app->Initialize(), "create hidden delayed link owner");
            app->SetDocument(L"[Notes](https://example.invalid/notes)\r\n");
            app->Document().SetSelection({1u, 1u});
            app->Prepare(512);
            app->Execute(Command::OpenLink);
            Require(WaitForSingleObject(gate->entered.get(), 3000) == WAIT_OBJECT_0 && launches->load() == 0u,
                    "link command returns while its owned worker is blocked before handoff");
            app->BeginClose();
            app.reset();
            Require(SetEvent(gate->proceed.get()) != FALSE && WaitForSingleObject(gate->finished.get(), 3000) == WAIT_OBJECT_0 && launches->load() == 1u,
                    "the value-owned dispatch retires after application destruction without an owner callback or UI-thread join");
            Require(foreground.NeverAcquiredForeground(), "pending-link teardown never takes desktop foreground");
        });
        test("Editor.StartupLastDocumentAndRecoveryPrecedence",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto wait = [](Application& app, const auto& ready)
            {
                for (unsigned turn = 0; turn < 3000; ++turn)
                {
                    app.Prepare(32);
                    if (ready())
                        return;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                throw std::runtime_error("Startup fixture did not settle");
            };
            const auto launch = [&](std::wstring_view name)
            {
                auto options                  = Options(root, name);
                options.startupRestoreForTest = true;
                options.preferencesProbe      = true;
                return options;
            };
            {
                auto options         = launch(L"startup-last-document");
                const auto directory = options.dataDirectory;
                const auto first = directory / L"Notebook.md", second = directory / L"Second.md";
                const auto pinned = directory / L"Pinned folder";
                std::filesystem::create_directories(pinned);
                Success(WriteBytesAtomic(first, "# Remembered\r\n"), "seed the actual remembered document");
                Success(WriteBytesAtomic(second, "# Second\n"), "seed a distinct workspace document");
                Settings saved;
                saved.lastDocument = first;
                saved.filesRoot    = pinned;
                Success(SaveSettings(directory, saved), "seed independent last-document and Files-root preferences");
                Application app(std::move(options));
                Success(app.Initialize(), "initialize hidden ordinary-startup policy");
                Require(app.Document().GetText().empty() && app._startupRestore.IsWaitingForRecovery(),
                        "ordinary startup begins with blank source while recovery scans");
                wait(app, [&] { return ! app._startupRestore.IsWaitingForRecovery() && ! app._startupRestore.IsOpeningLastDocument() && ! app._files.Busy(); });
                Require(app.Document().GetText() == L"# Remembered\r\n" && app._session.path == first && ! app.Dirty() && ! app.Document().CanUndo() &&
                            app._settings.filesRoot == pinned,
                        "the worker reopens exact saved source after a completed empty recovery scan without moving the pinned folder");
                app.OpenPath(second);
                wait(app, [&] { return ! app._files.Busy() && app._session.path == second; });
                Require(app._settings.lastDocument == second && app.SwitchWorkspaceDocument(0u) && app._settings.lastDocument == first,
                        "successful file opening and retained-workspace switching remember the active normal document");
                app.Execute(Command::New);
                Require(app.Document().GetText().empty() && app._settings.lastDocument.empty(),
                        "New remembers a blank untitled startup instead of the previous file");
                Require(app.Document().ReplaceSelection(L"Saved new writing\r\n"), "edit the new untitled document before Save As");
                const auto target = directory / L"Saved draft.md";
                app.Save(true, {}, target);
                wait(app, [&] { return ! app._files.Busy() && app._session.path == target; });
                wait(app, [&] { return ! app._preferenceWriter.Busy(); });
                Settings durable;
                Success(LoadSettings(directory, durable), "read the real coalesced preference publication");
                Require(durable.lastDocument == target && ! app.Dirty(), "a successful Save As durably remembers its normal document");
            }
            {
                auto options = launch(L"startup-missing-document");
                Settings saved;
                saved.lastDocument = options.dataDirectory / L"Unavailable.md";
                Success(SaveSettings(options.dataDirectory, saved), "seed a remembered path that does not exist");
                Application app(std::move(options));
                Success(app.Initialize(), "initialize missing-document startup");
                wait(app, [&] { return ! app._startupRestore.IsWaitingForRecovery() && ! app._startupRestore.IsOpeningLastDocument() && ! app._files.Busy(); });
                Require(app.Document().GetText().empty() && app._session.path.empty() && ! app.Dirty() && ! app.Document().CanUndo() &&
                            ! app._feedback->GetText().empty(),
                        "an unavailable remembered document leaves a blank clean untitled document with a useful status");
            }
            {
                auto options         = launch(L"startup-recovery-precedence");
                const auto directory = options.dataDirectory;
                const auto original  = directory / L"Notebook.md";
                std::filesystem::create_directories(directory / L"Recovery");
                Success(WriteBytesAtomic(original, "# Original on disk\n"), "seed the existing normal document behind recovery");
                Settings saved;
                saved.lastDocument = original;
                Success(SaveSettings(directory, saved), "seed a normal startup path alongside pending recovery");
                const RecoverySnapshot copy{L"# Unsaved recovered writing\r\n", original, false, {2u, 5u}, 17u};
                Success(WriteBytesAtomic(directory / L"Recovery" / L"session-00000000000000000000000000000001.rtd", SerializeRecovery(copy)),
                        "seed an inactive real recovery envelope");
                auto gate               = std::make_shared<AsyncFiles::TestGate>();
                options.fileGateForTest = gate;
                const auto release      = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
                Application app(std::move(options));
                Success(app.Initialize(), "initialize recovery-first startup");
                wait(app, [&] { return app._recovery.Ready() && ! app._startupRestore.IsWaitingForRecovery(); });
                Require(app._recoveryPanel->IsVisible() && app._recoveryCandidates.size() == 1u && app.Document().GetText().empty() && ! app._files.Busy() &&
                            ! app._startupRestore.IsOpeningLastDocument() && WaitForSingleObject(gate->entered.get(), 0) == WAIT_TIMEOUT,
                        "a pending recovery copy takes precedence before any automatic file-read admission");
                app.Execute(Command::RestoreRecovery);
                Require(app.Document().GetText() == copy.text && app._session.recovered && app.Dirty(),
                        "the chosen recovery source remains an unsaved copy requiring Save As");
                std::string unchanged;
                Success(ReadBytes(original, unchanged), "read the original after restoring the recovery copy");
                Require(unchanged == "# Original on disk\n", "startup and recovery never rewrite the original document");
            }
            Require(foreground.NeverAcquiredForeground(), "all startup, worker and preference checks retain desktop foreground");
        });
        test("Editor.StartupRestoreNeverReplacesNewerIntent",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            for (const unsigned mode : {0u, 1u, 2u})
            {
                auto options                  = Options(root, std::format(L"startup-newer-intent-{}", mode));
                options.startupRestoreForTest = true;
                options.preferencesProbe      = true;
                const auto original           = options.dataDirectory / L"Remembered.md";
                std::filesystem::create_directories(options.dataDirectory);
                Success(WriteBytesAtomic(original, "Old remembered writing"), "seed the actual delayed startup document");
                Settings saved;
                saved.lastDocument = original;
                Success(SaveSettings(options.dataDirectory, saved), "seed the delayed startup preference");
                auto gate               = std::make_shared<AsyncFiles::TestGate>();
                options.fileGateForTest = gate;
                unsigned prompts{};
                options.unsavedPromptForTest = [&]
                {
                    ++prompts;
                    return IDCANCEL;
                };
                const auto release = wil::scope_exit([&] { (void)SetEvent(gate->proceed.get()); });
                Application app(std::move(options));
                Success(app.Initialize(), "initialize an owned hidden startup race");
                if (mode == 0u)
                    Require(app.Document().ReplaceSelection(L"New writing before scan"), "type before recovery-ready publication");
                for (unsigned turn = 0; turn < 3000 && app._startupRestore.IsWaitingForRecovery(); ++turn)
                {
                    app.Prepare(32);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                Require(! app._startupRestore.IsWaitingForRecovery(), "initial recovery admission settles for the startup race");
                if (mode == 0u)
                {
                    Require(! app._startupRestore.IsOpeningLastDocument() && ! app._files.Busy() && app.Document().GetText() == L"New writing before scan" &&
                                app.Dirty(),
                            "early user writing prevents the automatic read from starting");
                    continue;
                }
                Require(WaitForSingleObject(gate->entered.get(), 3000) == WAIT_OBJECT_0 && app._startupRestore.IsOpeningLastDocument(),
                        "the real startup worker reaches its owned I/O barrier");
                if (mode == 1u)
                    Require(app.Document().ReplaceSelection(L"New writing while opening"), "type after the automatic read was admitted");
                else
                    app.Execute(Command::New);
                Require(SetEvent(gate->proceed.get()) != FALSE, "release only the owned startup I/O barrier");
                Require(WaitForSingleObject(gate->finished.get(), 3000) == WAIT_OBJECT_0, "the actual delayed file read completes");
                for (unsigned turn = 0; turn < 3000 && (app._startupRestore.IsOpeningLastDocument() || app._files.Busy()); ++turn)
                {
                    app.Prepare(32);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                app.Prepare(64);
                Require(! app._startupRestore.IsOpeningLastDocument() && ! app._files.Busy() && app._session.path.empty() && prompts == 0u,
                        "a superseded automatic read neither changes document identity nor asks a save question");
                if (mode == 1u)
                    Require(app.Document().GetText() == L"New writing while opening" && app.Dirty() && app.Document().CanUndo(),
                            "late user writing and its undo transaction survive automatic-open completion");
                else
                    Require(app.Document().GetText().empty() && ! app.Dirty() && ! app.Document().CanUndo() && app._settings.lastDocument.empty(),
                            "an explicit New abandons the old read even when its blank source revision could otherwise match");
            }
            Require(foreground.NeverAcquiredForeground(), "startup race checks never activate an application window");
        });
        test("Window.PlacementAndLaunchPolicyPreserveHiddenSource",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const RECT negativeWork{-1880, 40, 0, 1080};
            const RECT expectedNegative{-1880, 40, -680, 840};
            auto actual = Application::ClampWindowBounds({-2300, -90, -1100, 710}, negativeWork, 96u, 96u);
            Require(EqualRect(&actual, &expectedNegative),
                    "restoration uses independently specified screen coordinates including left/top taskbar work-area offsets");
            const RECT expectedDpi{100, 100, 1600, 1000};
            actual = Application::ClampWindowBounds({100, 100, 1100, 700}, {0, 0, 2560, 1440}, 96u, 144u);
            Require(EqualRect(&actual, &expectedDpi), "a target DPI change scales the normal size while retaining its bounded screen origin");
            const RECT smallWork{0, 0, 640, 480};
            actual = Application::ClampWindowBounds({-30000, 40000, 30000, 60000}, smallWork, 96u, 192u);
            Require(EqualRect(&actual, &smallWork), "the first/restored window and its minimum size fit a work area smaller than the usual minimum");
            const RECT safeWork{50, 30, 1050, 730};
            actual = Application::ClampWindowBounds(
                {std::numeric_limits<LONG>::min(), std::numeric_limits<LONG>::min(), std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max()},
                safeWork,
                0u,
                std::numeric_limits<unsigned>::max());
            Require(EqualRect(&actual, &safeWork), "wide intermediates and fallback DPI prevent overflow for hostile geometry");
            const RECT invalidWork{0, 0, 1, 1};
            actual = Application::ClampWindowBounds({0, 0, 1000, 800}, {5, 5, 4, 4}, 96u, 96u);
            Require(EqualRect(&actual, &invalidWork), "an invalid work-area query has a bounded positive fallback");
            Application app(Options(root, L"window-placement-policy"));
            Success(app.Initialize(), "create hidden placement policy owner");
            app.SetDocument(L"Exact notebook\r\n");
            const auto source             = app.Document().Snapshot();
            app._settings.windowPlacement = WindowPlacementSettings{{120, 80, 1120, 780}, 96u, true};
            Require(app.InitialShowCommand() == SW_SHOWMAXIMIZED, "ordinary startup restores a saved maximized state");
            app._settings.windowPlacement.value().maximized = false;
            Require(app.InitialShowCommand() == SW_SHOWNORMAL, "ordinary startup retains a saved normal state");
            app._options.explicitShowCommand = true;
            for (const int show : {SW_HIDE, SW_SHOWNOACTIVATE, SW_SHOWMINIMIZED, SW_SHOWMINNOACTIVE, SW_SHOWNORMAL, SW_SHOWMAXIMIZED})
            {
                app._options.showCommand = show;
                Require(app.InitialShowCommand() == show, "an explicit launcher show state takes precedence over the saved state");
            }
            const auto savedPlacement = app._settings.windowPlacement.value();
            app.RecordWindowPlacement(true);
            Require(EqualRect(&app._settings.windowPlacement.value().normalBounds, &savedPlacement.normalBounds) &&
                        app.Document().Snapshot().IsSameRevision(source) && ! app.Document().CanUndo() && ! app.Dirty() && ! IsWindowVisible(app.Window()) &&
                        foreground.NeverAcquiredForeground(),
                    "headless placement tests preserve the explicit fixture, source/history and desktop foreground without showing a window");
        });
        test("Editor.TextScaleShortcutsAndWheelPreserveHistory",
             [&]
        {
            Application app(Options(root, L"text-scale-input"));
            Success(app.Initialize(), "create hidden text-scale input fixture");
            const std::wstring sourceText = L"# Heading\r\n\r\nExact source \U0001f600\r\n";
            Require(sourceText.size() == 30u, "the independent zoom fixture has its declared UTF-16 source length");
            app.SetDocument(sourceText);
            app.Document().SetSelection({app.Document().Snapshot().Length(), app.Document().Snapshot().Length()});
            Require(app.Document().ReplaceSelection(L"!") && app.Document().Undo(), "retain redo for presentation-only zoom assertions");
            KeyboardState::ScopedSynthetic syntheticKeyboard;
            std::array<BYTE, 256> savedKeyboard{};
            Require(GetKeyboardState(savedKeyboard.data()) != FALSE, "snapshot the calling thread's modifier state for zoom");
            const auto restoreKeyboard = wil::scope_exit([&] noexcept
            {
                if (! SetKeyboardState(savedKeyboard.data()))
                    std::terminate();
            });
            std::array<BYTE, 256> requestedKeyboard{};
            const auto modifiers = [&](bool ctrl, bool alt = false, bool shift = false)
            {
                requestedKeyboard.fill(0);
                requestedKeyboard[VK_CONTROL] = requestedKeyboard[VK_LCONTROL] = ctrl ? 0x80 : 0;
                requestedKeyboard[VK_MENU] = requestedKeyboard[VK_RMENU] = alt ? 0x80 : 0;
                requestedKeyboard[VK_SHIFT] = requestedKeyboard[VK_LSHIFT] = shift ? 0x80 : 0;
            };
            MSG routed{};
            routed.hwnd    = app.Window();
            routed.message = WM_KEYDOWN;
            const auto key = [&](UINT value)
            {
                // Earlier commands can pump messages and replace the thread's keyboard snapshot.
                KeyboardState::Write(requestedKeyboard);
                routed.wParam = value;
                return app.RouteShortcut(routed);
            };
            const auto wheel = [&](short delta)
            {
                KeyboardState::Write(requestedKeyboard);
                (void)SendMessageW(app.Window(), WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, static_cast<WORD>(delta)), 0);
            };
            for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
            {
                app.SetView(view);
                app.Document().SetSelection({28u, 13u});
                app.Prepare(512);
                const auto source = app.Document().Snapshot();
                const auto focus  = app.Host().GetFocusControl();
                modifiers(true);
                Require(key(VK_OEM_PLUS) && app._settings.textScale == 105u && key(VK_OEM_MINUS) && app._settings.textScale == 100u,
                        "Ctrl plus/equal and minus adjust document scale by five percentage points");
                Require(key(VK_ADD) && app._settings.textScale == 105u && key(VK_SUBTRACT) && app._settings.textScale == 100u,
                        "numeric-keypad plus and minus retain the same scale policy");
                modifiers(true, false, true);
                Require(key(VK_OEM_PLUS) && app._settings.textScale == 105u, "the shifted plus key retains ordinary Ctrl-plus zoom");
                modifiers(true, true);
                Require(! key(VK_OEM_MINUS) && app._settings.textScale == 105u, "AltGr input is not stolen as a zoom shortcut");
                modifiers(true);
                Require(key('0') && app._settings.textScale == 100u, "Ctrl zero restores the default document scale");
                wheel(60);
                Require(app._settings.textScale == 100u, "a partial high-resolution wheel notch waits without changing scale");
                wheel(60);
                Require(app._settings.textScale == 105u, "two partial notches combine into exactly one text-size step");
                wheel(-240);
                Require(app._settings.textScale == 95u, "a multi-notch wheel message applies every admitted step in one action");
                wheel(-32768);
                Require(app._settings.textScale == 75u, "negative wheel input clamps at the existing minimum scale");
                wheel(32767);
                Require(app._settings.textScale == 200u, "positive wheel input clamps at the existing maximum scale");
                const auto layouts = app.Activity().layouts;
                wheel(120);
                Require(app._settings.textScale == 200u && app.Activity().layouts == layouts,
                        "an already clamped zoom performs no redundant layout or preference work");
                Require(ParseSettings(SerializeSettings(app._settings)).textScale == 200u,
                        "keyboard and wheel changes use the existing persistent scale preference");
                Require(key('0') && app._settings.textScale == 100u && app.Document().Snapshot().IsSameRevision(source) &&
                            app.Document().GetSelection() == DxUi::RichText::Selection{28u, 13u} && ! app.Document().CanUndo() && app.Document().CanRedo() &&
                            ! app.Dirty() && app.Host().GetFocusControl() == focus && ! IsWindowVisible(app.Window()),
                        "all zoom paths preserve exact source, directional selection, history, saved state and logical focus in every view");
                // End one gesture before testing the next editor view.
                (void)SendMessageW(app.Window(), WM_MOUSEWHEEL, MAKEWPARAM(0, 0), 0);
            }
        });
        test("Editor.FindCaseAndClosedRepeatPreserveSource",
             [&]
        {
            Application app(Options(root, L"find-case-repeat"));
            Success(app.Initialize(), "create hidden case and closed-repeat Find fixture");
            const std::wstring source = L"Cat cat CAT\r\n\u00c9cole \u00e9cole \U0001f600 cat";
            Require(source.size() == 31u, "the independent UTF-16 Find fixture has its declared source length");
            app.SetDocument(source);
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"cat");
            const std::array<std::size_t, 4> expected{0u, 4u, 8u, 28u};
            Require(! app._find.matchCase && app._find.table.matches.size() == expected.size(), "Find starts case-insensitive over exact counted source");
            for (std::size_t index = 0; index < expected.size(); ++index)
                Require(app._find.table.matches[index].first == expected[index] && app._find.table.matches[index].last == expected[index] + 3u,
                        "case-insensitive Find retains independently specified source ACP ranges across CRLF and a surrogate pair");
            app.Document().SetSelection({0u, 0u});
            app.Execute(Command::FindNext);
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{4u, 7u}, "visible Find reaches the second match");
            app.Execute(Command::CloseDrawer);
            const auto snapshot = app.Document().Snapshot();
            const auto work     = app.Activity();
            Require(! app._findBar->IsVisible() && ! app._find.IsPresentationActive() && ! app._find.projections.hasHighlights,
                    "closing Find retires its visible match presentation");
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{8u, 11u}, "closed-bar repeat advances to the retained third match");
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{28u, 31u},
                    "closed-bar repeat uses exact UTF-16 offsets after the supplementary character");
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{0u, 3u}, "closed-bar repeat wraps forward");
            app.Execute(Command::FindPrevious);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{28u, 31u} && ! app._findBar->IsVisible() && ! app._find.IsPresentationActive() &&
                        ! app._find.projections.hasHighlights && app.Activity().findMatchRebuilds == work.findMatchRebuilds &&
                        app.Document().Snapshot().IsSameRevision(snapshot) && ! app.Document().CanUndo(),
                    "closed forward/reverse repeats reuse warm matches without source/history edits or reopening Find");
            app.Execute(Command::FindMatchCase);
            app.Execute(Command::FindNext);
            Require(app._find.matchCase && app.Document().GetSelection() == DxUi::RichText::Selection{4u, 7u},
                    "Match case lazily refreshes the closed cache and skips differently cased source");
            const auto caseWork = app.Activity().findMatchRebuilds;
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection() == DxUi::RichText::Selection{28u, 31u} && app.Activity().findMatchRebuilds == caseWork,
                    "the refreshed closed case-sensitive cache remains reusable");
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"\u00e9cole");
            Require(app._find.table.matches.size() == 1u && app._find.table.matches[0].first == 19u,
                    "Match case distinguishes the independently specified accented capital");
            app.Execute(Command::FindMatchCase);
            Require(app._find.table.matches.size() == 2u && app._find.table.matches[0].first == 13u && app._find.table.matches[1].first == 19u,
                    "ordinal case-insensitive Find handles non-ASCII text without rewriting its source");
            Require(app._findCaseToggle->GetAccessibleName() == Resource(IDS_MATCHCASE), "the Find preference has its localized accessible command name");
        });
        test("Editor.SingleReplaceSelectsFollowingMatch",
             [&]
        {
            Application app(Options(root, L"find-single-replace"));
            Success(app.Initialize(), "create hidden one-action replacement fixture");
            app.SetDocument(L"cat cat cat");
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"cat");
            app._replaceText->SetTextAndNotify(L"dog");
            app.Document().SetSelection({0u, 0u});
            app.Execute(Command::FindNext);
            app.Execute(Command::Replace);
            Require(app.Document().GetText() == L"dog cat cat" && app.Document().GetSelection() == DxUi::RichText::Selection{4u, 7u},
                    "one Replace changes the selected match and selects the next match");
            app.Execute(Command::Replace);
            Require(app.Document().GetText() == L"dog dog cat" && app.Document().GetSelection() == DxUi::RichText::Selection{8u, 11u},
                    "the following Replace needs one action rather than a separate navigation action");
            Require(app.Document().Undo() && app.Document().GetText() == L"dog cat cat" && app.Document().Undo() &&
                        app.Document().GetText() == L"cat cat cat" && ! app.Dirty(),
                    "each replacement remains one independent source-history transaction");
        });
        test("Editor.DesignRefreshFindReplaceUndoAndCapacity",
             [&]
        {
            Application app(Options(root, L"design-find"));
            Success(app.Initialize(), "create hidden Find/Replace app");
            const std::wstring source = L"α cat 😀 cat\r\ncat";
            app.SetDocument(source);
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"cat");
            Require(app._find.table.matches.size() == 3, "Find uses exact source ranges across Unicode and mixed line boundaries");
            const std::array<std::size_t, 3> expected{source.find(L"cat"), source.find(L"cat", source.find(L"cat") + 3), source.rfind(L"cat")};
            app.Document().SetSelection({0, 0});
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection().Start() == expected[0], "Find next selects the first exact source match");
            app.Execute(Command::FindNext);
            Require(app.Document().GetSelection().Start() == expected[1], "Find next advances by the selected source range");
            app.Execute(Command::FindPrevious);
            Require(app.Document().GetSelection().Start() == expected[0], "Find previous selects the preceding match");
            app.Execute(Command::FindPrevious);
            Require(app.Document().GetSelection().Start() == expected[2], "Find previous wraps to the final match");
            app._replaceText->SetTextAndNotify(L"犬");
            app.Execute(Command::ReplaceAll);
            Require(app.Document().GetText() == L"α 犬 😀 犬\r\n犬" && app.Dirty(), "Replace all changes only all exact source matches");
            Require(app.Document().Undo() && app.Document().GetText() == source && ! app.Dirty(),
                    "one undo restores every replacement and the original Unicode/EOLs");
            app.SetDocument(L"aaa");
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"a");
            app._replaceText->SetTextAndNotify(std::wstring(24000, L'z'));
            const auto retained = app.Document().Snapshot();
            const auto selected = app.Document().GetSelection();
            app.Execute(Command::ReplaceAll);
            Require(app.Document().GetText() == L"aaa" && app.Document().Snapshot().IsSameRevision(retained) && app.Document().GetSelection() == selected &&
                        ! app.Document().CanUndo(),
                    "an oversized replacement set is rejected before any source/history/selection mutation");
            const auto selection = app.Document().GetSelection();
            Require(app.DismissSurface() && ! app._findBar->IsVisible() && app.Host().GetFocusControl() == &app.Document() &&
                        app.Document().GetSelection() == selection,
                    "Escape closes docked Find and restores logical editor routing with the source selection");
            app.SetDocument(std::wstring(5000, L'a'));
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"a");
            app.Document().SetSelection({4999, 5000});
            app.SelectionChanged();
            Require(app._find.table.matches.size() == 5000 && app._find.table.source && app._find.table.source->IsSameRevision(app.Document().Snapshot()) &&
                        app.Host().RenderInitialFrameForShow() && app.Document().GetSelection().Start() == 4999,
                    "a current match beyond the bounded highlight prefix stays selected and renderable with cached exact source ranges");
            app.SetDocument(L"cat cat");
            app._findText->SetTextAndNotify(L"cat");
            app._replaceText->SetText(L"dog");
            app.Document().SetReadOnly(true);
            const auto failedSource = app.Document().Snapshot();
            app.ReplaceFind(true);
            Require(! app._find.updating && app.Document().Snapshot().IsSameRevision(failedSource) && ! app.Document().CanUndo(),
                    "failed source-edit admission releases Find suppression without changing source or history");
            app.Document().SetReadOnly(false);
            app._replaceText->SetTextAndNotify(L"dog");
            app.ReplaceFind(true);
            Require(app.Document().GetText() == L"dog dog" && ! app._find.updating && app._find.table.matches.empty() && app.Document().Undo(),
                    "Find and Replace remain usable after the rejected transaction");
        });
        test("Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster",
             [&]
        {
            Application app(Options(root, L"design-brand-mark"));
            Success(app.Initialize(), "create hidden brand-mark raster app");
            app.SetDocument(L"Prepared content remains independent of the retained brand resource.\n");
            app.Prepare(512);
            const auto beforePath = root / L"brand-mark-before-device-loss.png";
            Require(app.Capture(beforePath), "capture the prepared brand mark before device replacement");
            const auto before = ReadBrandMarkRaster(beforePath, app.Host().GetDpi());
            app.Host().DebugSimulateDeviceLoss();
            app.Prepare(512);
            const auto afterPath = root / L"brand-mark-after-device-loss.png";
            Require(app.Capture(afterPath), "capture the re-prepared brand mark after device replacement");
            const auto after = ReadBrandMarkRaster(afterPath, app.Host().GetDpi());
            Require(before == after, "device recreation preserves the exact hidden brand-mark raster");
            app.Prepare(512);
            const auto repeatedPath = root / L"brand-mark-repeated-render.png";
            Require(app.Capture(repeatedPath), "capture the warm brand mark after a repeated render");
            const auto repeated = ReadBrandMarkRaster(repeatedPath, app.Host().GetDpi());
            Require(after == repeated, "warm repeated rendering preserves the exact prepared brand-mark raster");
        });
        test("Editor.DesignRefreshEquationContextAndDraftGuards",
             [&]
        {
            Application app(Options(root, L"design-equation"));
            Success(app.Initialize(), "create hidden contextual-equation app");
            Resize(app, 1024, 900, 96);
            const std::wstring source = L"# Exact equation\n\nCode `$fake$` and $a_i^2$ with a tail.\n";
            app.SetDocument(source);
            app.Prepare(512);
            auto at = source.find(L"fake");
            app.Document().SetSelection({at, at});
            app.SelectionChanged();
            Require(! app._equationContext && ! app._equation->IsVisible() && ! app._equationHelp->IsVisible(),
                    "ordinary/code context reserves no empty equation inspector");
            at = source.find(L"a_i^2");
            app.Document().SetSelection({at, at});
            app.SelectionChanged();
            Require(app._equationContext && app._equation->IsVisible() && app._equation->IsReadOnly() && app._equationHelp->IsVisible(),
                    "only current math exposes the explicitly read-only contextual equation rendering");
            app.Prepare(512);
            Require(! app._sidebar->IsVisible() && ! app._equation->GetAcknowledgedSemanticDocument(app.Host()),
                    "narrow layout defers the hidden equation inspector instead of requiring wide-only resources");
            Resize(app, 1440, 900, 96);
            app.Prepare(512);
            const auto inspected = app._equation->GetAcknowledgedSemanticDocument(app.Host());
            Require(inspected && std::ranges::count(inspected->Elements(), DxUi::RichText::SemanticRole::Math, &DxUi::RichText::SemanticElement::role) == 1,
                    "the inspector renders its preview as one native equation rather than literal delimiters");
            for (const unsigned index : {3u, 4u, 5u, 12u})
                Require(! app._tools[index]->IsEnabled(), "inline formatting commands are disabled inside equations");
            const auto envelope  = source.find(L"$a_i^2$");
            const bool entryUndo = app.Document().CanUndo(), entryRedo = app.Document().CanRedo();
            app.Execute(Command::LocalSource);
            const auto region = app.Document().GetSemanticSourceRegion();
            Require(region && region->source.first == envelope && region->source.last == envelope + 7 && ! app.HasBlockDraft() &&
                        ! app.Document().IsReadOnly() && app.Document().GetText() == source,
                    std::format("F2 shows the exact inline equation envelope in place: region={} [{},{}] expected [{},{}] draft={}",
                                region.has_value(),
                                region ? region->source.first : 0u,
                                region ? region->source.last : 0u,
                                envelope,
                                envelope + 7,
                                app.HasBlockDraft())
                        .c_str());
            Require(app.DismissSurface() && ! app.Document().GetSemanticSourceRegion() && app.Document().GetText() == source && ! app.Dirty() &&
                        app.Document().CanUndo() == entryUndo && app.Document().CanRedo() == entryRedo,
                    "Escape leaves unedited in-place equation source without touching text or history");
            app.Prepare(512);
            app.Document().SetSelection({at, at});
            app.Execute(Command::LocalSource);
            const auto digit = source.find(L"2$");
            app.Document().SetSelection({digit, digit + 1});
            const std::wstring edited = L"# Exact equation\n\nCode `$fake$` and $a_i^3$ with a tail.\n";
            Require(app.Document().GetSemanticSourceRegion() && app._equationContext && app._equation->GetText() == L"\\[\na_i^2\n\\]",
                    "in-place equation source keeps the rendered inspector preview");
            Require(app.Document().GetSemanticSourceRegion() && app.Document().ReplaceSelection(L"3") && app.Document().GetText() == edited && app.Dirty(),
                    "typing inside the in-place equation edits the authoritative source directly");
            Require(app._equationContext && app._equation->GetText() == L"\\[\na_i^3\n\\]" && ! app._tools[3]->IsEnabled(),
                    "the inspector previews the TeX being typed and inline formatting stays disabled");
            app.Execute(Command::LocalSource);
            Require(! app.Document().GetSemanticSourceRegion() && app.Document().GetText() == edited && app.Document().Undo() &&
                        app.Document().GetText() == source,
                    "F2 returns to the rendered equation, keeps the edit and one undo restores the exact source");

            // The guarded local draft remains the F2 surface for ordinary text without a native region.
            app.Prepare(512);
            const auto tail = source.find(L"tail");
            app.Document().SetSelection({tail, tail + 4});
            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft() && app.BlockDraft()->GetText() == L"tail" && app.Document().IsReadOnly() && ! app.Document().GetSemanticSourceRegion(),
                    "F2 on ordinary selected text attaches a guarded exact-source draft");
            Require(app.BlockDraft()->GetFontRole() == DxUi::FontRole::Monospace, "exact source uses canonical TextField monospace metrics and input geometry");
            app.Prepare(512);
            const auto draftHighlights = app.Document().GetSourceHighlights();
            Require(app._blockDraftSession.has_value(), "open draft retains an explicit session");
            const auto& draftSession = app._blockDraftSession.value();
            Require(draftHighlights.size() == 1 && draftHighlights.front().start == draftSession.range.first &&
                        draftHighlights.front().length == draftSession.range.last - draftSession.range.first && draftHighlights.front().current,
                    "the open draft retains its exact source highlight across completed presentation work");
            const auto anchor = app.Document().TryGetTextInputRangeRects(app.Host(), draftSession.range.first, draftSession.range.last);
            Require(anchor && ! anchor->empty() && ! draftSession.pendingPlacement && Near(app._blockDraft->GetBounds().top, anchor->back().bottom + 8),
                    std::format("prepared F2 source must attach below native geometry: anchor={}, pending={}, top={}, expected={}, bottom={}",
                                anchor && ! anchor->empty(),
                                draftSession.pendingPlacement,
                                app._blockDraft->GetBounds().top,
                                anchor && ! anchor->empty() ? anchor->back().bottom + 8 : -1.f,
                                app._page->GetBounds().bottom));
            app.BlockDraft()->SetTextAndNotify(L"end");
            app.Execute(Command::Find);
            Require(app.HasBlockDraft() && ! app._findBar->IsVisible() && app.Document().GetText() == source,
                    "another presentation command cannot discard or mutate a pending draft");
            app.Execute(Command::CloseDrawer);
            Require(! app.HasBlockDraft() && ! app.Document().IsReadOnly() && app.Document().GetSourceHighlights().empty() &&
                        app.Document().GetText() == source && ! app.Dirty(),
                    "Cancel preserves exact source/history and restores editing");
            app.Document().SetSelection({tail, tail + 4});
            app.Execute(Command::LocalSource);
            app.BlockDraft()->SetTextAndNotify(L"end");
            app.Execute(Command::ApplyBlock);
            Require(app.Document().GetSourceHighlights().empty() &&
                        app.Document().GetText() == L"# Exact equation\n\nCode `$fake$` and $a_i^2$ with a end.\n" && app.Document().Undo() &&
                        app.Document().GetText() == source,
                    "Apply is a single undoable exact-source replacement");
            app.SetDocument(L"");
            app.Prepare(512);
            app.Execute(Command::LocalSource);
            app.Prepare(512);
            const auto emptyCaret = app.Document().TryGetTextInputCaretRect(app.Host(), 0);
            Require(app.HasBlockDraft() && app.BlockDraft()->GetText().empty() && emptyCaret && ! app._blockDraftSession.value().pendingPlacement &&
                        Near(app._blockDraft->GetBounds().top, emptyCaret->bottom + 8),
                    "an empty document anchors F2 at its prepared insertion site without an endless placement retry");
            app.BlockDraft()->SetTextAndNotify(L"First words.");
            app.Execute(Command::ApplyBlock);
            Require(app.Document().GetText() == L"First words." && app.Document().Undo() && app.Document().GetText().empty(),
                    "the empty-source draft creates one undoable authoritative transaction");

            const std::wstring linkedEquation = L"[Parent](https://example.test \"Parent title\") and $a_i^2$.\n";
            app.SetDocument(linkedEquation);
            app.Prepare(512);
            const auto equationAt = linkedEquation.find(L"a_i^2");
            app.Document().SetSelection({equationAt, equationAt});
            const auto linkedSource    = app.Document().Snapshot();
            const auto linkedSelection = app.Document().GetSelection();
            const bool linkedUndo = app.Document().CanUndo(), linkedRedo = app.Document().CanRedo();
            app.Execute(Command::Source);
            app.Prepare(512);
            Require(app.Preferences().view == EditorView::Source && app.Document().GetViewMode() == DxUi::RichText::ViewMode::Source &&
                        app.Document().Snapshot().IsSameRevision(linkedSource) && app.Document().GetSelection() == linkedSelection,
                    "Source view keeps the exact linked equation revision and selection");
            app.Execute(Command::Split);
            app.Prepare(512);
            Require(app.Preferences().view == EditorView::Split && app.SourceMirror().GetText() == linkedEquation &&
                        app.Document().Snapshot().IsSameRevision(linkedSource) && app.Document().GetSelection() == linkedSelection,
                    "Split mirrors linked math without creating another editable source");
            app.Execute(Command::Compose);
            app.Prepare(512);
            auto linkedSemantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
            Require(static_cast<bool>(linkedSemantic), "linked Compose semantic publication exists");
            const auto preservedLink =
                std::ranges::find_if(linkedSemantic->Elements(), [](const auto& element) { return element.role == DxUi::RichText::SemanticRole::Link; });
            Require(app.Preferences().view == EditorView::Compose && app.Document().GetViewMode() == DxUi::RichText::ViewMode::Rich &&
                        app.Document().Snapshot().IsSameRevision(linkedSource) && app.Document().GetSelection() == linkedSelection &&
                        app.Document().CanUndo() == linkedUndo && app.Document().CanRedo() == linkedRedo && preservedLink != linkedSemantic->Elements().end() &&
                        preservedLink->title == L"Parent title",
                    "Compose restores native math while preserving parent link title and both history directions");
            app.Execute(Command::LocalSource);
            Require(app.Document().GetSemanticSourceRegion() && ! app.HasBlockDraft(), "F2 opens the exact linked equation source in place");
            Require(app.DismissSurface() && ! app.Document().GetSemanticSourceRegion() && app.Document().GetText() == linkedEquation && ! app.Dirty() &&
                        app.Document().CanUndo() == linkedUndo && app.Document().CanRedo() == linkedRedo,
                    "leaving unedited linked equation source keeps its text and history untouched");
            app.Prepare(512);
            app.Document().SetSelection({equationAt, equationAt});
            app.Execute(Command::LocalSource);
            const auto linkedDigit = linkedEquation.find(L"2$");
            app.Document().SetSelection({linkedDigit, linkedDigit + 1});
            Require(app.Document().GetSemanticSourceRegion() && app.Document().ReplaceSelection(L"3"), "edit the linked equation in place");
            app.Execute(Command::LocalSource);
            const std::wstring editedLinkedEquation = L"[Parent](https://example.test \"Parent title\") and $a_i^3$.\n";
            Require(! app.Document().GetSemanticSourceRegion() && app.Document().GetText() == editedLinkedEquation && app.Document().Undo() &&
                        app.Document().GetText() == linkedEquation && app.Document().Redo() && app.Document().GetText() == editedLinkedEquation,
                    "an in-place linked equation edit is one undoable change with exact surrounding source");
            app.Prepare(512);
            linkedSemantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
            Require(static_cast<bool>(linkedSemantic), "edited linked semantic publication exists");
            const auto retainedLink =
                std::ranges::find_if(linkedSemantic->Elements(), [](const auto& element) { return element.role == DxUi::RichText::SemanticRole::Link; });
            Require(retainedLink != linkedSemantic->Elements().end() && retainedLink->title == L"Parent title",
                    "apply/undo/redo preserves the neighboring parent link title");
        });
        test("Editor.PersistentFolderNavigationAndRestart",
             [&]
        {
            const auto library = root / L"folder-library";
            const auto first = library / L"overview.md", nested = library / L"Notes/meeting.markdown";
            const auto external = root / L"folder-outside/other.md";
            WriteFixture(first, "# Overview\n");
            WriteFixture(nested, "# Meeting\n");
            WriteFixture(external, "# Other\n");
            auto launch             = Options(root, L"folder-restart-settings");
            launch.preferencesProbe = true; // Isolated persisted preferences with a hidden, non-activating window.
            {
                Application app(launch);
                Success(app.Initialize(), "initialize the persistent-folder app");
                app.SetDocument(L"# Overview\n", first);
                Require(app.Preferences().filesRoot == library, "the first file establishes its parent as the persistent root");
                app.SelectFilesRoot(library);
                Await(app, [&] { return app._folderSnapshot && ! app._folderFiles.Busy(); });
                Require(app._folderRoot == library && app._folderSnapshot->markdownCount == 2,
                        "Open Folder publishes Markdown from the chosen root and nested folders");
                const auto generation = app._folderSnapshot->generation;
                app.OpenPath(nested);
                Await(app, [&] { return app._session.path == nested; });
                Require(app._folderRoot == library && app._folderSnapshot->generation == generation &&
                            std::ranges::any_of(app._filesModel.rows, [](const auto& row) { return row.text == L"meeting.markdown"; }),
                        "a nested file retains the same scan generation and expands its ancestors");
                app.Document().SetSelection({0, 0});
                Require(app.Document().ReplaceSelection(L"Draft "), "create an unsaved edit before selecting a folder");
                const auto source    = app.Document().Snapshot();
                const auto selection = app.Document().GetSelection();
                app.SelectFilesRoot(external.parent_path());
                Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == external.parent_path(); });
                Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection && app.Dirty(),
                        "selecting a folder preserves an unsaved document, its source revision and selection");
                Require(app.Document().Undo() && ! app.Dirty(), "folder selection preserves the original undo history");
                app.SelectFilesRoot(library);
                Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == library; });
                app.OpenPath(external);
                Await(app, [&] { return app._session.path == external; });
                Require(app.Preferences().filesRoot == library && app._folderSnapshot->markdownCount == 2,
                        "opening an external file retains every Markdown entry in the chosen library");
                app.OpenPath(root / L"missing-folder/missing.md");
                Await(app, [&] { return ! app._files.Busy(); });
                Require(app._session.path == external && app.Preferences().filesRoot == library, "a failed Open preserves the document and selected root");
                app.Execute(Command::New);
                Require(app._session.path.empty() && app.Preferences().filesRoot == library && app._folderSnapshot,
                        "New retains the chosen folder while creating an untitled document");
                app.Execute(Command::Files);
                Require(app._folderFiles.CurrentPhase() == FilesPhase::Closed && app.Preferences().filesRoot == library,
                        "hiding Files stops its worker and retains the selected root");
                app.Execute(Command::Files);
                Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == library && ! app._folderFiles.Busy(); });
                const auto settled = app.Activity();
                app.Prepare(32);
                Require(app.Activity().filesModelRebuilds == settled.filesModelRebuilds, "an idle persistent tree does not rebuild or introduce polling work");
                Await(app, [&] { return ! app._preferenceWriter.Busy(); });
                Success(app._preferenceWriter.Status(), "the chosen root and visibility reach durable preferences");
            }
            {
                Application restarted(launch);
                Success(restarted.Initialize(), "restart using the same isolated settings directory");
                Await(restarted, [&] { return restarted._folderSnapshot && ! restarted._folderFiles.Busy(); });
                Require(restarted.Preferences().showFiles && restarted._folderRoot == library && restarted._folderSnapshot->markdownCount == 2 &&
                            restarted._session.path.empty() && ! restarted.Dirty(),
                        "startup restores the library independently of the last document without creating unsaved edits");
                restarted.SelectFilesRoot(root / L"removed-library");
                Await(restarted, [&] { return restarted._folderSnapshot && FAILED(restarted._folderSnapshot->status); });
                Require(restarted.Document().ReplaceSelection(L"Still editable "), "an unavailable remembered folder never disables editing");
                const auto retained = restarted.Document().Snapshot();
                restarted.SelectFilesRoot(library);
                Await(restarted, [&] { return restarted._folderSnapshot && restarted._folderSnapshot->root == library; });
                Require(restarted.Document().Snapshot().IsSameRevision(retained), "Open Folder recovers an unavailable root without losing edits");
                Await(restarted, [&] { return ! restarted._preferenceWriter.Busy(); });
                Success(restarted._preferenceWriter.Status(), "the replacement folder is durable before ending the fixture");
            }
        });
        test("Editor.NativeCodeHighlightingPreservesSource",
             [&]
        {
            Application app(Options(root, L"code-highlighting"));
            Success(app.Initialize(), "create hidden code rendering app");
            app.SetDocument(L"# Code example\n\n```cpp\n// A small example\nconst int answer = 42;\n```\n");
            app.Prepare(256);
            const auto source    = app.Document().Snapshot();
            const auto selection = app.Document().GetSelection();
            const auto colored = root / L"code-colored.png", plain = root / L"code-plain.png";
            Require(app.Capture(colored), "capture default language-aware code ink");
            app.Document().SetCodeTokenizer({});
            app.Prepare(256);
            Require(app.Capture(plain), "capture ordinary code ink for an independent pixel comparison");
            std::string coloredBytes, plainBytes;
            Success(ReadBytes(colored, coloredBytes), "read highlighted rendering");
            Success(ReadBytes(plain, plainBytes), "read ordinary rendering");
            Require(coloredBytes != plainBytes && app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection &&
                        ! app.Document().CanUndo() && ! app.Dirty(),
                    "the application's default tokenizer changes actual code pixels while preserving source, selection and history");
        });
        test("Editor.NativeMermaidProfilesAndDisplayRegressions",
             [&]
        {
            Application app(Options(root, L"mermaid-profiles"));
            Success(app.Initialize(), "create hidden native Mermaid consumer");
            constexpr std::array<std::wstring_view, 18> bodies{
                L"flowchart LR\nDraft[Draft] --> Saved[Saved]\n",
                L"stateDiagram-v2\n[*] --> Draft\nDraft --> Saved : save\nSaved --> [*]\n",
                L"classDiagram\nDocument <|-- Markdown\nMarkdown : +save()\n",
                L"sequenceDiagram\nparticipant Writer\nparticipant Notebook\nWriter->>Notebook: Save draft\nNotebook-->>Writer: Saved\n",
                L"pie showData title Notebook review\n\"Reviewed\" : 60\n\"Draft\" : 40\n",
                L"erDiagram\nNOTEBOOK ||--o{ NOTE : contains\nNOTE ||--o{ REVISION : retains\n",
                L"gantt\ndateFormat YYYY-MM-DD\ntitle Notebook review\nsection Writing\nDraft : 2026-11-03, 2d\nReview : 2026-11-05, 1d\n",
                L"mindmap\n  root((Notebook))\n    Drafts\n      Ideas\n    Research\n",
                L"flowchart LR\nA[<b>Warning: </b>] -->|<i>continue </i>| B[Continue]\n",
                L"erDiagram\nCUSTOMER ||--o{ ORDER : places\nORDER ||--|{ LINE-ITEM : contains\n"
                L"CUSTOMER }|..|{ DELIVERY-ADDRESS : uses\n",
                L"classDiagram\nNotebook \"1\" --> \"*\" Note\nNotebook \"1\" --> \"*\" Attachment\n",
                L"sequenceDiagram\nactor Writer as Author\nparticipant App as Editor\nautonumber\n"
                L"Note left of Writer: Draft\nWriter->App: Save\nNote over App,Writer: Shared\nApp-->Writer: Saved\n"
                L"Note right of App: Stored\n",
                L"sequenceDiagram\nNote over Writer: One participant\nNote over Writer,App: Two participants\n",
                L"sequenceDiagram\nautonumber\nWriter->>App: Save\nApp-->>Writer: Saved\n",
                L"sequenceDiagram\nWriter->App: Save\nApp-->Writer: Saved\nWriter-)Writer: Continue\n",
                L"flowchart\nDraft --> Saved\n",
                L"flowchart TB\nNotebook -->|Retain the exact Markdown source| Source\nNotebook -->|Prepare a readable preview| Compose\n",
                L"flowchart TD\nDraft -->|review| Summary\nNotes -->|check| Preview\nDraft -->|preview| Preview\nNotes -->|summarize| Summary\n"};
            for (const auto appearance : {Appearance::Light, Appearance::Dark})
            {
                app._settings.appearance = appearance;
                app.ApplyTheme();
                for (std::size_t index = 0; index < bodies.size(); ++index)
                {
                    const auto source = L"# Native diagram\n\n```mermaid\n" + std::wstring(bodies[index]) + L"```\n\nNeighboring prose stays rendered.\n";
                    app.SetView(EditorView::Compose);
                    app.SetDocument(source);
                    app.Prepare(1024);
                    const auto snapshot = app.Document().Snapshot();
                    const auto at       = source.find(L"```mermaid\n") + 11;
                    app.Document().SetSelection({at, at});
                    const auto selection     = app.Document().GetSelection();
                    const auto requireNative = [&]
                    {
                        const auto semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
                        Require(semantic && app.Document().GetMarkdownRenderingStatus() == DxUi::MarkdownRenderingStatus::Semantic,
                                std::format("Mermaid case {} acknowledges native geometry without a whole-document fallback", index));
                        const auto diagram =
                            std::ranges::find(semantic->Elements(), DxUi::RichText::SemanticRole::Diagram, &DxUi::RichText::SemanticElement::role);
                        Require(
                            diagram != semantic->Elements().end() && diagram->display.last == diagram->display.first + 1 && semantic->Diagnostics().empty() &&
                                std::ranges::count(semantic->Elements(), DxUi::RichText::SemanticRole::Heading, &DxUi::RichText::SemanticElement::role) == 1,
                            std::format("Mermaid case {} is one native object with rich neighbors, not a literal code fallback", index));
                        if (index == 11u)
                            Require(diagram->name.find(L"Author") != std::wstring::npos && diagram->name.find(L"Editor") != std::wstring::npos &&
                                        diagram->name.find(L"Draft") < diagram->name.find(L"1. Save") &&
                                        diagram->name.find(L"1. Save") < diagram->name.find(L"Shared") &&
                                        diagram->name.find(L"Shared") < diagram->name.find(L"2. Saved") &&
                                        diagram->name.find(L"2. Saved") < diagram->name.find(L"Stored"),
                                    "consumer accessibility retains actor aliases, notes and derived message order");
                    };
                    requireNative();
                    for (const auto view : {EditorView::Source, EditorView::Split, EditorView::Compose})
                    {
                        app.SetView(view);
                        app.Prepare(1024);
                        Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection &&
                                    ! app.Document().CanUndo() && ! app.Document().CanRedo() && ! app.Dirty(),
                                "native diagram view switches preserve exact source, ACP selection and clean history");
                        if (view == EditorView::Split)
                            Require(app.SourceMirror().GetText() == source, "Split derives its mirror from the exact diagram source");
                    }
                    requireNative();
                    app.Execute(Command::LocalSource);
                    Require(app.Document().GetSemanticSourceRegion().has_value() && ! app.HasBlockDraft(),
                            "F2 enters the library-owned exact diagram source region");
                    app.Execute(Command::LocalSource);
                    app.Prepare(1024);
                    Require(! app.Document().GetSemanticSourceRegion() && app.Document().Snapshot().IsSameRevision(snapshot) &&
                                app.Document().GetSelection() == selection && ! app.Document().CanUndo() && ! app.Document().CanRedo() && ! app.Dirty(),
                            "leaving unedited diagram source preserves its revision, selection and history");
                    requireNative();
                    if (index == 11u)
                    {
                        app.Execute(Command::LocalSource);
                        const auto noteAt = source.find(L"Shared");
                        app.Document().SetSelection({noteAt, noteAt + 6u});
                        Require(app.Document().ReplaceSelection(L"Reviewed"), "edit one note through the authoritative F2 source region");
                        app.Execute(Command::LocalSource);
                        app.Prepare(1024);
                        auto edited = source;
                        edited.replace(noteAt, 6u, L"Reviewed");
                        Require(app.Document().GetText() == edited && app.Document().CanUndo(), "note edits preserve every other authored byte");
                        Require(app.Document().Undo(), "undo the F2 note edit");
                        app.Prepare(1024);
                        Require(app.Document().GetText() == source && ! app.Document().CanUndo() && app.Document().CanRedo(),
                                "undo restores exact extended Mermaid source without a view-switch history entry");
                        requireNative();
                        Require(app.Document().Redo() && app.Document().GetText() == edited && app.Document().Undo(),
                                "redo and undo retain the same source-backed note transaction");
                        app.Prepare(1024);
                        requireNative();
                    }
                }
            }
            const auto fixture               = root / L"native-profile.md";
            const std::wstring captureSource = L"# Native capture\n\n```mermaid\nflowchart LR\nA[<b>Warning: </b>] --> B[Continue]\n```\n";
            WriteFixture(fixture, DxUi::RichText::EncodeUtf8(captureSource));
            for (const bool dark : {false, true})
            {
                auto captureOptions     = Options(root, dark ? L"native-capture-dark" : L"native-capture-light");
                captureOptions.scenario = dark ? L"sample-document-dark" : L"sample-document";
                captureOptions.document = fixture;
                captureOptions.capture  = root / (dark ? L"native-capture-dark.png" : L"native-capture-light.png");
                auto sidecar            = captureOptions.capture;
                sidecar += L".scenario.json";
                Application captureApp(std::move(captureOptions));
                Success(captureApp.Initialize(), "create hidden custom-document capture");
                Require(captureApp.Run() == 0 && captureApp.Document().GetText() == captureSource &&
                            captureApp.Document().GetMarkdownRenderingStatus() == DxUi::MarkdownRenderingStatus::Semantic,
                        "custom capture scenarios keep the supplied diagram and rich neighboring heading");
                Color(captureApp.Host().GetTheme().cardBackground,
                      dark ? 0x22252Bu : 0xFFFFFFu,
                      "the custom capture's actual native palette follows its explicit Light/Dark scenario");
                std::string metadata;
                Success(ReadBytes(sidecar, metadata), "read the custom capture's actual theme provenance");
                Require(metadata.find(dark ? "\"renderedAppearance\": \"dark\"" : "\"renderedAppearance\": \"light\"") != std::string::npos &&
                            metadata.find("\"fixtureBasename\": \"native-profile.md\"") != std::string::npos,
                        "the sidecar names the actual rendered appearance and invented input basename");
            }
        });
        test("Editor.DesignRefreshFilesRerootFilterAndDirtyOpen",
             [&]
        {
            const auto directory = root / L"design-files-fixture";
            const auto first = directory / L"Drafts/current.md", other = directory / L"Published/other.markdown";
            WriteFixture(first, "# Current\n");
            WriteFixture(directory / L"Drafts/nested/child.MD");
            WriteFixture(directory / L"Drafts/nested/second.md");
            WriteFixture(directory / L"Drafts/empty/ignored.txt");
            WriteFixture(other, "# Published\n");
            Application app(Options(root, L"design-files-ui"));
            Success(app.Initialize(), "create hidden Files integration app");
            Resize(app, 1440, 900, 96, false);
            StoredDocument loaded;
            Success(LoadDocument(first, loaded), "read original Files fixture");
            app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
            const auto selection = app.Document().GetSelection();
            app.Execute(Command::Files);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == first.parent_path(); });
            Require(app._folderRoot == first.parent_path() && app._folderSnapshot->markdownCount == 3 && app.Host().GetFocusControl() == app._filesTree &&
                        app._filesToggle->GetToggleState() == std::optional<bool>(true),
                    "opening Files publishes the document folder and routes logical focus to its real tree");
            const auto folder = std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"nested"; });
            Require(folder != app._filesModel.rows.end() && ! folder->expanded && folder->badgeText == L"2",
                    "other folders start collapsed with Markdown counts");
            const auto id = folder->id;
            app._filesModel.OnTreeToggleExpanded(id, true);
            Require(std::ranges::any_of(app._filesModel.rows, [](const auto& row) { return row.text == L"child.MD"; }),
                    "shared-tree expansion exposes application-owned child rows");
            const auto retainedFilesSource    = app.Document().Snapshot();
            const auto notificationGeneration = app._folderSnapshot->generation;
            auto notificationRevision         = app._folderSnapshot->revision;
            const auto arrived                = first.parent_path() / L"arrived";
            const auto arrivedDocument        = arrived / L"new.md";
            WriteFixture(arrivedDocument);
            Await(app,
                  [&]
            {
                return app._folderSnapshot && app._folderSnapshot->revision > notificationRevision &&
                       std::ranges::any_of(app._folderSnapshot->entries, [&](const auto& entry) { return entry.path == arrivedDocument; });
            });
            const auto arrivedRow =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"arrived"; });
            const auto nestedRow =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"nested"; });
            Require(arrivedRow != app._filesModel.rows.end() && ! arrivedRow->expanded && arrivedRow->badgeText == L"1" &&
                        nestedRow != app._filesModel.rows.end() && nestedRow->expanded &&
                        std::ranges::none_of(app._filesModel.rows, [](const auto& row) { return row.text == L"new.md"; }),
                    "an actual notification starts a newly discovered folder collapsed and preserves existing user expansion");
            const auto currentRow =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"current.md"; });
            Require(app._folderRoot == first.parent_path() && app._folderSnapshot->generation == notificationGeneration &&
                        currentRow != app._filesModel.rows.end() && app._filesTree->GetSelectedItemId() == std::optional(currentRow->id),
                    "notification refresh keeps the document's containing root expanded and its current-file row visible and selected");
            const auto arrivedId = arrivedRow->id, nestedId = nestedRow->id;
            app._filesModel.OnTreeToggleExpanded(arrivedId, true);
            app._filesModel.OnTreeToggleExpanded(nestedId, false);
            notificationRevision = app._folderSnapshot->revision;
            WriteFixture(first.parent_path() / L"another.md");
            Await(app,
                  [&]
            {
                return app._folderSnapshot && app._folderSnapshot->revision > notificationRevision &&
                       std::ranges::any_of(app._folderSnapshot->entries, [&](const auto& entry) { return entry.path == first.parent_path() / L"another.md"; });
            });
            Require(std::ranges::any_of(app._filesModel.rows, [](const auto& row) { return row.text == L"new.md"; }) &&
                        std::ranges::none_of(app._filesModel.rows, [](const auto& row) { return row.text == L"child.MD"; }),
                    "later notifications preserve both explicit expansion and explicit collapse choices");
            const auto removedRow =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"arrived"; });
            Require(removedRow != app._filesModel.rows.end(), "the folder to retire remains in the current Files model");
            app._filesModel.OnTreeToggleExpanded(removedRow->id, false);
            notificationRevision = app._folderSnapshot->revision;
            Require(std::filesystem::remove(arrivedDocument) && std::filesystem::remove(arrived), "remove only the owned empty folder fixture");
            Await(app,
                  [&]
            {
                return app._folderSnapshot && app._folderSnapshot->revision > notificationRevision &&
                       std::ranges::none_of(app._folderSnapshot->entries, [&](const auto& entry) { return entry.path == arrived; });
            });
            Require(! app._collapsedFolders.contains(arrived), "removed directories retire their retained collapse-state keys");
            notificationRevision = app._folderSnapshot->revision;
            WriteFixture(arrivedDocument);
            Await(app,
                  [&]
            {
                return app._folderSnapshot && app._folderSnapshot->revision > notificationRevision &&
                       std::ranges::any_of(app._folderSnapshot->entries, [&](const auto& entry) { return entry.path == arrivedDocument; });
            });
            Require(std::ranges::any_of(app._filesModel.rows, [](const auto& row) { return row.text == L"arrived" && ! row.expanded; }) &&
                        app.Document().Snapshot().IsSameRevision(retainedFilesSource) && app.Document().GetSelection() == selection &&
                        ! app.Document().CanUndo() && ! app.Document().CanRedo(),
                    "a recreated directory starts collapsed without notification edits to source, selection or history");
            app._filesFilter->SetTextAndNotify(L"CHILD");
            Require(app._filesModel.rows.size() == 2 && app._filesModel.rows[0].text == L"nested" && app._filesModel.rows[1].text == L"child.MD",
                    "the live name filter keeps only case-insensitive matching files and ancestors");
            Require(app.DismissSurface() && app._filesFilter->GetText().empty() && app.Preferences().showFiles,
                    "first Escape clears the file filter without closing the pane");
            app.Document().SetSelection({0, 0});
            Require(app.Document().ReplaceSelection(L"Uncommitted "), "make the current document dirty before a tree open");
            const auto retained = app.Document().GetText();
            app.OpenPath(other);
            Await(app, [&] { return app._session.path == other && app._folderSnapshot && app._folderSnapshot->root == first.parent_path(); });
            Require(app._inactiveDocuments.size() == 1 && app._inactiveDocuments.front().state.Text() == retained && app._inactiveDocuments.front().dirty &&
                        app._inactiveDocuments.front().state.CanUndo(),
                    "a tree open retains the previous dirty source and history without a save question");
            Require(app.Document().GetText() == L"# Published\n" && app._folderRoot == first.parent_path() &&
                        std::ranges::any_of(app._filesModel.rows, [](const auto& row) { return row.id == Application::WorkspaceGroupId + 1u; }) &&
                        app._filesTree->GetSelectedItemId() == std::nullopt,
                    "opening a clean file outside the browser root retains only modified documents in the workspace group without selecting clean rows");
            const auto savedAs = directory / L"Archive/renamed.md";
            std::filesystem::create_directories(savedAs.parent_path());
            Success(app._files.Submit(true, savedAs, app.Document().GetText(), false), "admit the save-as writer stage without a native file picker");
            Await(app, [&] { return app._session.path == savedAs && app._folderSnapshot && app._folderSnapshot->root == first.parent_path(); });
            Require(app._folderRoot == first.parent_path() && ! app.Dirty(), "successful Save As retains the browser root and preserves the saved baseline");
            app.Document().SetSelection(selection);
            app.CloseDrawers();
            Require(app.DismissSurface() && ! app.Preferences().showFiles && app.Host().GetFocusControl() == &app.Document() &&
                        app.Document().GetSelection() == selection,
                    "closing Files restores the editor's selection and logical routing");
        });
        test("Editor.FilesDirtyBadgeUpdatesOneVisibleRow",
             [&]
        {
            const auto directory = root / L"files-dirty-badge-fixture";
            const auto document  = directory / L"current.md";
            WriteFixture(document, "# Current document\n");
            for (unsigned index = 0; index < 96; ++index)
                WriteFixture(directory / std::format(L"note-{:03}.md", index));
            WriteFixture(directory / L"archive/retained.md");

            Application app(Options(root, L"files-dirty-badge"));
            Success(app.Initialize(), "create the hidden large Files-pane app");
            Resize(app, 1440, 900, 96, false);
            StoredDocument loaded;
            Success(LoadDocument(document, loaded), "read the current document fixture");
            app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
            app.Execute(Command::Files);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == directory && ! app._folderFiles.Busy(); });
            Require(app._filesModel.rows.size() == 98, "the synthetic Files model contains a nontrivial set of visible rows");
            const auto collapsed =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"archive"; });
            Require(collapsed != app._filesModel.rows.end() && ! collapsed->expanded && collapsed->badgeText == L"1",
                    "the fixture includes a collapsed directory with an independent count badge");

            const auto current =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"current.md"; });
            Require(current != app._filesModel.rows.end() && current->badgeText.empty() && ! current->badgeIsDot,
                    "the clean current document starts without the Edited dot");
            const auto currentId  = current->id;
            const auto selectedId = app._filesModel.rows.back().id;
            app._filesTree->SetSelectedItemId(selectedId);
            app._filesTree->SetFocusedItemId(selectedId);
            const auto treeBounds = app._filesTree->GetBounds();
            const auto wheelPoint = D2D1::Point2F(treeBounds.left + 20.f, treeBounds.top + 20.f);
            for (unsigned step = 0; step < 6; ++step)
                Require(app._filesTree->OnMouseWheel(app.Host(), wheelPoint, static_cast<float>(WHEEL_DELTA), 0),
                        "scroll the tree away from its focused last row");
            const auto firstVisible = app._filesTree->GetFirstVisibleItemIndex();
            Require(firstVisible > 0 && firstVisible < app._filesModel.rows.size() - 1 && app._filesTree->GetSelectedItemId() == std::optional(selectedId) &&
                        app._filesTree->GetFocusedItemId() == std::optional(selectedId),
                    "the fixture has an offscreen focused row and a separately observable scroll position");

            struct TreeItemProperties
            {
                std::wstring name, itemStatus;
            };
            const auto readStringProperty = [](IRawElementProviderSimple* provider, PROPERTYID property, std::string_view message)
            {
                wil::unique_variant value;
                Success(provider->GetPropertyValue(property, &value), message);
                if (value.vt == VT_EMPTY || value.vt == VT_NULL)
                    return std::wstring{};
                Require(value.vt == VT_BSTR, message);
                return std::wstring(value.bstrVal ? value.bstrVal : L"");
            };
            app.Prepare(256);
            wil::com_ptr_nothrow<IRawElementProviderFragmentRoot> accessibilityRoot;
            accessibilityRoot.attach(DxUi::CreateWindowHostAccessibilityProvider(app.Host().GetHwnd()));
            Require(accessibilityRoot != nullptr, "the attached application window exposes its canonical UI Automation root");
            wil::com_ptr_nothrow<IRawElementProviderFragment> rootFragment;
            Success(accessibilityRoot.query_to(rootFragment.put()), "the canonical UI Automation root supports fragment navigation");
            const auto findTreeItem = [&](std::wstring_view expectedName)
            {
                std::optional<TreeItemProperties> result;
                std::size_t visited{};
                const auto visit = [&](auto&& self, IRawElementProviderFragment* current, std::size_t depth) -> bool
                {
                    if (! current || depth > 64u || visited >= 4096u)
                        return false;
                    ++visited;
                    wil::com_ptr_nothrow<IRawElementProviderSimple> simple;
                    Success(current->QueryInterface(IID_PPV_ARGS(simple.put())), "query the UI Automation fragment's simple provider");
                    wil::unique_variant controlType;
                    Success(simple->GetPropertyValue(UIA_ControlTypePropertyId, &controlType), "read the current UI Automation fragment's control type");
                    Require(controlType.vt == VT_I4, "UI Automation control type uses its documented integer value");
                    if (controlType.lVal == UIA_TreeItemControlTypeId)
                    {
                        const auto name = readStringProperty(simple.get(), UIA_NamePropertyId, "read a Files TreeItem name");
                        if (name == expectedName)
                        {
                            result = TreeItemProperties{name, readStringProperty(simple.get(), UIA_ItemStatusPropertyId, "read a Files TreeItem status")};
                            return true;
                        }
                    }
                    wil::com_ptr_nothrow<IRawElementProviderFragment> child;
                    Success(current->Navigate(NavigateDirection_FirstChild, child.put()), "navigate to the first UI Automation child");
                    while (child && visited < 4096u)
                    {
                        if (self(self, child.get(), depth + 1u))
                            return true;
                        wil::com_ptr_nothrow<IRawElementProviderFragment> sibling;
                        Success(child->Navigate(NavigateDirection_NextSibling, sibling.put()), "navigate to the next UI Automation sibling");
                        child = std::move(sibling);
                    }
                    return false;
                };
                Require(visit(visit, rootFragment.get(), 0u), "find the named Files TreeItem in the bounded provider tree");
                return result.value();
            };
            const auto cleanProvider          = findTreeItem(L"current.md");
            const auto initialArchiveProvider = findTreeItem(L"archive");
            Require(cleanProvider.name == L"current.md" && cleanProvider.itemStatus.empty() && initialArchiveProvider.name == L"archive" &&
                        initialArchiveProvider.itemStatus == L"1",
                    "the real UI Automation providers expose a clean file and the collapsed archive's count badge");

            const auto rowSnapshot      = app._filesModel.rows;
            const auto folderGeneration = app._folderSnapshot->generation;
            const auto folderRevision   = app._folderSnapshot->revision;
            const auto folderStatus     = app._folderSnapshot->status;
            const auto watchStatus      = app._folderSnapshot->watchStatus;
            const auto footer           = std::wstring(app._filesCount->GetText());
            Require(app.Document().ReplaceSelection(L"x"), "make the current document dirty");
            Require(app._filesModel.rows.size() == rowSnapshot.size() + 2u && app._filesModel.rows[0].id == Application::WorkspaceGroupId &&
                        app._filesModel.rows[1].id == Application::WorkspaceGroupId + app._session.key + 1u && app._filesModel.rows[1].text == L"current.md" &&
                        app._filesModel.rows[1].badgeIsDot,
                    "making the active document dirty inserts the Modified documents group cleanly");
            const auto edited = std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [&](const auto& row) { return row.id == currentId; });
            Require(edited != app._filesModel.rows.end() && edited->badgeText == Resource(IDS_FILESEDITED) && edited->badgeIsDot,
                    "the current file receives the localized accessible dot status");
            app.Prepare(256);
            const auto editedProvider       = findTreeItem(L"current.md");
            const auto dirtyArchiveProvider = findTreeItem(L"archive");
            Require(editedProvider.name == L"current.md" && editedProvider.itemStatus == Resource(IDS_FILESEDITED) && dirtyArchiveProvider.name == L"archive" &&
                        dirtyArchiveProvider.itemStatus == initialArchiveProvider.itemStatus,
                    "the live UI Automation providers expose the localized Edited status on current.md while preserving archive count");
            Require(app._filesCount->GetText() == footer && app._folderSnapshot->generation == folderGeneration &&
                        app._folderSnapshot->revision == folderRevision && app._folderSnapshot->status == folderStatus &&
                        app._folderSnapshot->watchStatus == watchStatus,
                    "a dirty transition preserves footer and watcher snapshot");

            Require(app.Document().Undo() && ! app.Dirty(), "undo restores the exact clean document state");
            Require(app._filesModel.rows.size() == rowSnapshot.size() &&
                        std::ranges::none_of(app._filesModel.rows, [](const auto& row) { return row.id >= Application::WorkspaceGroupId; }),
                    "undo removes the Modified documents group when no files remain dirty");
            app.Prepare(256);
            const auto restoredProvider        = findTreeItem(L"current.md");
            const auto restoredArchiveProvider = findTreeItem(L"archive");
            Require(restoredProvider.name == L"current.md" && restoredProvider.itemStatus.empty() && restoredArchiveProvider.name == L"archive" &&
                        restoredArchiveProvider.itemStatus == initialArchiveProvider.itemStatus,
                    "undo clears only the current file's live UI Automation status and preserves the neighboring count");
            const auto restored = std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [&](const auto& row) { return row.id == currentId; });
            Require(restored != app._filesModel.rows.end() && restored->badgeText.empty() && ! restored->badgeIsDot,
                    "the current file dot clears when its source becomes clean");

            app._filesFilter->SetTextAndNotify(L"note-");
            Require(std::ranges::none_of(app._filesModel.rows, [](const auto& row) { return row.text == L"current.md"; }),
                    "the filter hides the current file row without changing the snapshot");
            Require(app.Document().ReplaceSelection(L"y"), "dirty the current document while its row is filtered out");
            Require(std::ranges::none_of(app._filesModel.rows, [](const auto& row) { return row.text == L"current.md"; }),
                    "a filtered-out current file stays hidden under the active filter even when dirty");
            Require(app.Document().Undo() && ! app.Dirty(), "filtered current document returns to its clean state");
            app._filesFilter->SetTextAndNotify(L"");
            Require(app.Document().ReplaceSelection(L"z"), "dirty the document before replacing it with a different root");
            const auto nextDirectory = root / L"files-dirty-badge-next-root";
            const auto nextDocument  = nextDirectory / L"renamed.md";
            WriteFixture(nextDocument, "# Renamed document\n");
            StoredDocument next;
            Success(LoadDocument(nextDocument, next), "read the replacement root fixture");
            app.SelectFilesRoot(nextDirectory);
            app.SetDocument(next.text, nextDocument, next.bom, next.stamp);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == nextDirectory && ! app._folderFiles.Busy(); });
            const auto renamed =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"renamed.md"; });
            Require(app._folderRoot == nextDirectory && ! app.Dirty() && ! app._lastFilesDirty && renamed != app._filesModel.rows.end() &&
                        renamed->badgeText.empty() && ! renamed->badgeIsDot,
                    "a dirty-to-clean document replacement uses the full root refresh and publishes the new clean row");

            Application untitled(Options(root, L"files-dirty-no-snapshot"));
            Success(untitled.Initialize(), "create the hidden untitled Files-pane app");
            Resize(untitled, 1440, 900, 96, false);
            untitled.Execute(Command::Files);
            Require(! untitled._folderSnapshot && untitled._filesModel.rows.empty() && untitled._folderFiles.CurrentPhase() == FilesPhase::Closed,
                    "the untitled Files pane has no folder snapshot or worker");
            const auto emptyFooter = std::wstring(untitled._filesCount->GetText());
            Require(untitled.Document().ReplaceSelection(L"x") && untitled.Dirty() && untitled._lastFilesDirty,
                    "dirty bookkeeping updates while the Files pane has no snapshot");
            Require(untitled._filesModel.rows.size() == 2u && untitled._filesModel.rows[0].id == Application::WorkspaceGroupId &&
                        untitled._filesModel.rows[1].id == Application::WorkspaceGroupId + untitled._session.key + 1u &&
                        untitled._filesModel.rows[1].badgeIsDot && untitled._filesCount->GetText() == emptyFooter &&
                        untitled._folderFiles.CurrentPhase() == FilesPhase::Closed,
                    "an empty-root dirty transition shows only the dirty untitled document under Modified documents and keeps the localized footer intact");
            Require(untitled.Document().Undo() && ! untitled.Dirty() && ! untitled._lastFilesDirty && untitled._filesModel.rows.empty() &&
                        untitled._folderFiles.CurrentPhase() == FilesPhase::Closed,
                    "undo clears Modified documents and restores the empty untitled row model");
        });
        test("Editor.DesignRefreshFilesAdmissionErrorsAndRootActions",
             [&]
        {
            const auto directory = root / L"design-files-errors-fixture";
            const auto document  = directory / L"Draft/current.md";
            WriteFixture(document, "# Current\n");
            WriteFixture(directory / L"Draft/folder.md/inside.md");
            Application app(Options(root, L"design-files-errors"));
            Success(app.Initialize(), "create hidden Files error/root-action app");
            Resize(app, 1440, 900, 96, false);
            app.SetDocument(L"Unsaved writing");
            app.Execute(Command::Files);
            Require(app._filesCount->GetText() == Resource(IDS_FILESUNTITLED) && app._folderFiles.CurrentPhase() == FilesPhase::Closed,
                    "opening Files for untitled writing needs neither a root watcher nor a misleading empty scan");
            Require(app._filesFooterMeasure.measurements == 1 && app._filesFooterMeasure.typographyRevision == app.Host().GetTypographyRevision(),
                    "the first cold Files open measures once and retains the typography epoch initialized by that measurement");
            app.Layout();
            Require(app._filesFooterMeasure.measurements == 1, "the cold visible footer measurement is reused on the next layout");
            StoredDocument loaded;
            Success(LoadDocument(document, loaded), "load first saved root fixture");
            app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == document.parent_path(); });
            Require(app._folderFiles.IsActive() && app._filesTitle->GetText() == L"Draft", "a first saved document starts and labels the Files worker");
            const auto folder =
                std::find_if(app._filesModel.rows.begin(), app._filesModel.rows.end(), [](const auto& row) { return row.text == L"folder.md"; });
            Require(folder != app._filesModel.rows.end(), "the populated Markdown-suffixed folder is visible");
            const auto retained = app.Document().Snapshot();
            app._filesModel.OnTreeItemInvoked(folder->id);
            Require(! app._files.Busy() && app._session.path == document && app.Document().Snapshot().IsSameRevision(retained),
                    "invoking a folder whose name ends in .md never submits a file-open command");
            const auto missing = directory / L"unavailable/current.md";
            app.SelectFilesRoot(missing.parent_path());
            app.SetDocument(L"Still editable\n", missing);
            Await(app, [&] { return app._folderSnapshot && app._folderSnapshot->root == missing.parent_path(); });
            Require(FAILED(app._folderSnapshot->status) && app._folderSnapshot->errorCount > 0 && app._filesCount->GetText() != Resource(IDS_FILESLOADING) &&
                        app.Document().GetText() == L"Still editable\n",
                    "an unavailable root settles as an explicit pane error while the document remains editable");
            // Force an admission refusal at the application/service boundary,
            // without allocation injection, thread hooks or native desktop UI.
            app._folderFiles.Close();
            app._folderFiles.FailNextInitializeForTest(E_UNEXPECTED);
            app._session.path       = directory / L"next/current.md";
            app._settings.filesRoot = app._session.path.parent_path();
            app.RefreshFiles(true);
            Require(app._filesCount->GetText() == Resource(IDS_FILESUNAVAILABLE),
                    "a rejected scan admission retains its error instead of waiting forever for an unsubmitted result");
            Require(app.Document().ReplaceSelection(L"Available "), "Files service failure never disables document editing");
            // A synthetic root model tests the label without scanning a real
            // drive root or creating any path outside this harness fixture.
            app._folderFiles.Close();
            app._session.path         = L"R:\\current.md";
            app._folderRoot           = L"R:\\";
            app._settings.filesRoot   = app._folderRoot;
            app._folderSnapshot       = FilesSnapshot{};
            app._folderSnapshot->root = app._folderRoot;
            app.RefreshFiles();
            Require(app._filesTitle->GetText() == L"R:\\" && app._folderFiles.CurrentPhase() == FilesPhase::Closed,
                    "a volume root keeps a readable label even when filename() is empty, without acquiring another volume");
            auto& snapshot             = app._folderSnapshot.value();
            snapshot.markdownCount     = 7;
            snapshot.entryLimitReached = true;
            app.RefreshFiles();
            Require(app._filesCount->GetText() == L"7 Markdown files · 5000 examined-entry traversal limit",
                    "the entry footer distinguishes returned Markdown files from the canonical examined-entry ceiling");
            snapshot.entryLimitReached = false;
            snapshot.depthLimitReached = true;
            app.RefreshFiles();
            Require(app._filesCount->GetText() == L"7 Markdown files · depth 8 traversal limit", "a depth-only footer reports its own applied depth ceiling");
            snapshot.entryLimitReached = true;
            app.RefreshFiles();
            Require(app._filesCount->GetText() == L"7 Markdown files · 5000 examined-entry traversal limit · depth 8 traversal limit",
                    "simultaneous limits retain both explanations");
            snapshot.appliedLimits = {2, 3};
            app.RefreshFiles();
            Require(app._filesCount->GetText() == L"7 Markdown files · 3 examined-entry traversal limit · depth 2 traversal limit",
                    "custom admitted limits are reported from the snapshot rather than duplicated defaults");
            snapshot.status      = E_ACCESSDENIED;
            snapshot.errorCount  = 1;
            snapshot.watchStatus = E_FAIL;
            app.RefreshFiles();
            Require(app._filesCount->GetText() == L"7 Markdown files · some folders unavailable · change notifications unavailable · 3 "
                                                  L"examined-entry traversal limit · depth 2 traversal limit",
                    "access and watch failures never hide either simultaneous traversal limit");
            const std::wstring completeFooter(app._filesCount->GetText());
            app._settings.filesWidth   = 200;
            app._settings.filesPinMode = PanePinMode::Auto;
            Resize(app, 800, 560, 96);
            Require(app._leftRail->IsVisible() && ! app._filesPane->IsVisible(),
                    "narrow viewport collapses Files to rail while deferring hidden footer measurement");
            app.RevealPaneOverlay(PaneSide::Left);
            DxUi::DesiredSize desired;
            Success(DxUi::MeasureControl(*app._filesCount, {app.Host()}, DxUi::LayoutConstraints::AtMost(176.f), desired),
                    "independently measure the complete wrapped footer through canonical DxUi");
            const auto footer = app._filesCount->GetBounds();
            Require(footer.bottom - footer.top >= desired.sizeDip.height && footer.bottom - footer.top > 52 && Inside(footer, app._filesPane->GetBounds()) &&
                        Inside(app._filesTree->GetBounds(), app._filesPane->GetBounds()) && ! Overlap(footer, app._filesTree->GetBounds()) &&
                        ! Overlap(footer, app._filesFilter->GetBounds()) && ! Overlap(app._filesTree->GetBounds(), app._filesFilter->GetBounds()),
                    "the minimum pane reserves every wrapped footer fact without overlapping the tree or filter");
            const auto measures = app._filesFooterMeasure.measurements;
            app.Layout();
            app.Layout();
            Require(app._filesFooterMeasure.measurements == measures,
                    "unchanged layout reuses its footer measurement instead of creating another native text layout");
            app._settings.filesWidth = 220;
            app.Layout();
            Require(app._filesFooterMeasure.measurements == measures + 1, "a changed footer width performs one new canonical measurement");
            snapshot.entryLimitReached = snapshot.depthLimitReached = false;
            snapshot.status = snapshot.watchStatus = S_OK;
            snapshot.errorCount                    = 0;
            app.RefreshFiles();
            Require(Near(app._filesCount->GetBounds().bottom - app._filesCount->GetBounds().top, 24),
                    "the ordinary short-count footer uses the compact image-2 minimum");
            const auto retainedSource    = app.Document().Snapshot();
            const auto retainedRevision  = app.Document().Revision();
            const auto retainedSelection = app.Document().GetSelection();
            const bool retainedUndo = app.Document().CanUndo(), retainedRedo = app.Document().CanRedo(), retainedDirty = app.Dirty();
            app._settings.showFiles    = false;
            app._settings.filesPinMode = PanePinMode::Unpinned;
            app.DismissHoverOverlays(true);
            app.Layout();
            const auto hiddenMeasures      = app._filesFooterMeasure.measurements;
            const auto retainedFooterWidth = app._filesFooterMeasure.widthDip, retainedFooterDpi = app._filesFooterMeasure.dpi;
            const auto retainedFooterRevision = app._filesFooterMeasure.textRevision;
            for (const UINT dpi : {96u, 144u, 192u})
            {
                app._settings.filesWidth = dpi == 144 ? 220u : 200u;
                app._filesCount->SetText(dpi == 144 ? Resource(IDS_FILESUNTITLED) : completeFooter);
                Resize(app, 800, 560, dpi, false);
                app._settings.appearance = dpi == 144 ? Appearance::Dark : Appearance::Light;
                app.ApplyTheme();
                Require(! app._filesPane->IsVisible() && app._filesFooterMeasure.measurements == hiddenMeasures &&
                            app._filesFooterMeasure.widthDip == retainedFooterWidth && app._filesFooterMeasure.dpi == retainedFooterDpi &&
                            app._filesFooterMeasure.textRevision == retainedFooterRevision,
                        "closed Files preserves its stale measurement across real owner resize, DPI, text and theme changes");
            }
            app._settings.showFiles    = true;
            app._settings.filesPinMode = PanePinMode::Auto;
            app.Layout();
            Require(app._leftRail->IsVisible() && ! app._filesPane->IsVisible(),
                    "re-enabling Files at narrow 800x560 enters CollapsedRail before explicit hover expansion");
            app.RevealPaneOverlay(PaneSide::Left);
            Require(app._filesPane->IsVisible() && app._filesFooterMeasure.measurements == hiddenMeasures + 1 &&
                        app._filesFooterMeasure.textRevision == app._filesCount->GetMeasureRevision() &&
                        app._filesFooterMeasure.typographyRevision == app.Host().GetTypographyRevision() && Near(app._filesFooterMeasure.widthDip, 176) &&
                        Near(app._filesFooterMeasure.dpi, 192),
                    "reopening Files measures the current text, narrow width, typography and DPI exactly once");
            DxUi::DesiredSize reopenedDesired;
            Success(DxUi::MeasureControl(*app._filesCount, {app.Host()}, DxUi::LayoutConstraints::AtMost(176.f), reopenedDesired),
                    "independently measure the complete footer after hidden changes");
            const auto reopenedFooter = app._filesCount->GetBounds();
            Require(app._filesCount->GetText() == completeFooter && reopenedFooter.bottom - reopenedFooter.top >= reopenedDesired.sizeDip.height &&
                        reopenedFooter.bottom - reopenedFooter.top > 52 && Inside(reopenedFooter, app._filesPane->GetBounds()) &&
                        ! Overlap(reopenedFooter, app._filesTree->GetBounds()) && ! Overlap(reopenedFooter, app._filesFilter->GetBounds()),
                    "the first reopened layout reserves every wrapped fact without clipping or overlapping Files content");
            app.Layout();
            Require(app._filesFooterMeasure.measurements == hiddenMeasures + 1, "an unchanged visible layout reuses the measurement acquired on reopening");
            app._settings.focusMode = true;
            app.Layout();
            const std::size_t emptyFiles = 0;
            app._filesCount->SetText(std::vformat(Resource(IDS_FILESCOUNT), std::make_wformat_args(emptyFiles)));
            app._settings.filesWidth = 220;
            app.Layout();
            Require(! app._filesPane->IsVisible() && app._filesFooterMeasure.measurements == hiddenMeasures + 1,
                    "Focus mode also defers measurements for a requested but logically hidden Files pane");
            app._settings.focusMode = false;
            app.Layout();
            Require(app._leftRail->IsVisible() && ! app._filesPane->IsVisible() && app._filesFooterMeasure.measurements == hiddenMeasures + 1,
                    "leaving Focus at narrow 800x560 keeps Files collapsed in rail until hover-expanded");
            app.RevealPaneOverlay(PaneSide::Left);
            Require(app._filesPane->IsVisible() && app._filesFooterMeasure.measurements == hiddenMeasures + 2 &&
                        Near(app._filesCount->GetBounds().bottom - app._filesCount->GetBounds().top, 24),
                    "leaving Focus measures a stale short footer once and restores its minimum height");
            Require(app.Document().Snapshot().IsSameRevision(retainedSource) && app.Document().Revision() == retainedRevision &&
                        app.Document().GetSelection() == retainedSelection && app.Document().CanUndo() == retainedUndo &&
                        app.Document().CanRedo() == retainedRedo && app.Dirty() == retainedDirty,
                    "deferred footer measurement preserves exact source, selection, history and save state");
        });
        test("Editor.DesignRefreshTransientSurfacesAndClose",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto document = root / L"design-close-fixture/current.md";
            WriteFixture(document, "# Retained\n\nFind this text.\n");
            Application app(Options(root, L"design-close"));
            Success(app.Initialize(), "create hidden surface/close app");
            StoredDocument loaded;
            Success(LoadDocument(document, loaded), "load close fixture");
            app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
            app.Execute(Command::Files);
            Await(app, [&] { return app._folderSnapshot.has_value(); });
            app.Execute(Command::Settings);
            Require(app._drawer->IsVisible() && ! app._findBar->IsVisible(), "Settings owns one visible contextual surface");
            app.Execute(Command::Find);
            app._findText->SetTextAndNotify(L"Find");
            Require(app._findBar->IsVisible() && ! app._drawer->IsVisible() && app._find.table.matches.size() == 1,
                    "Find replaces Settings and computes its own visible query state");
            app.Execute(Command::Settings);
            Require(app._drawer->IsVisible() && ! app._findBar->IsVisible() && ! app._find.IsPresentationActive() && ! app._find.projections.hasHighlights &&
                        std::ranges::all_of(app._outlineModel.rows, [](const auto& row) { return row.badgeText.empty(); }),
                    "hiding Find through Settings clears source highlights and transient outline counts");
            app.Execute(Command::CloseDrawer);
            app.Execute(Command::Find);
            const auto caret = app.Document().GetText().find(L"Find");
            app.Document().SetSelection({caret, caret});
            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft() && ! app._findBar->IsVisible() && ! app._find.IsPresentationActive() && ! app._find.projections.hasHighlights,
                    "F2 also clears Find presentation when switching to an exact local-source draft");
            app.Execute(Command::CloseDrawer);
            Require(! app.Dirty(), "surface commands never create document edits");
            app.Document().SetSelection({caret, caret + 4});
            app.Prepare(256);
            Require(app._floating->IsVisible(), "programmatic selection opens tools through the shared callback");
            Require(app._selectionHint->GetTypographyOverride().has_value() && Near(app._selectionHint->GetTypographyOverride().value().fontSizeDip, 11) &&
                        Inside(app._selectionHint->GetBounds(), app._selectionHintChip->GetBounds()),
                    "the keyboard hint uses an 11-DIP label inside the shared bordered chip");
            Resize(app, 800, 560);
            Require(app._floating->IsVisible() && Inside(app._floating->GetBounds(), app._page->GetBounds()),
                    "visible selected-text tools remain inside the writing viewport after resize");
            app.Execute(Command::CloseDrawer);
            // Exercise a newly observed syntax identity after explicit
            // dismissal. Derived context must refresh without reopening it.
            app._derivedCaches.selectionFactsSyntax.reset();
            app.Prepare(256);
            Require(! app._floating->IsVisible() && ! app._pendingSelectionTools, "late syntax reconciliation preserves explicit selected-tools dismissal");
            const auto generation = app._folderFiles.Generation();
            Require(generation > 0, "the close scenario owns a real active directory watcher");
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(app.IsClosing() && app.Host().GetHwnd() == nullptr && app._folderFiles.Generation() == 0,
                    "clean Close detaches the native host and invalidates directory publication immediately");
            const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            // Once the host has destroyed its controls, pump only the owned
            // shutdown notification HWND. Never dereference a retired editor.
            while (app.Window() && std::chrono::steady_clock::now() < end)
            {
                MSG message{};
                unsigned processed{};
                while (processed++ < 64 && PeekMessageW(&message, app.Window(), 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                    if (! app.Window())
                        break;
                }
                app.FinishClose();
                if (app.Window())
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            Require(app.Window() == nullptr && ! app._folderFiles.TakeResult(), "asynchronous retirement finishes without a late UI result or UI-thread join");
            Require(foreground.NeverAcquiredForeground(), "watch/surface shutdown never steals desktop focus");
        });
        test("Editor.DesignRefreshLiveSettingsAndEscape",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"design-settings"));
            Success(app.Initialize(), "create hidden live-settings app");
            app.SetDocument(L"# Settings preserve writing\n\nA retained source.\n\n- [x] Completed **work**\n- [ ] Open work\n");
            const auto revision = app.Document().Snapshot();
            Resize(app, 800, 560);
            app.Execute(Command::Settings);
            Require(app._drawer->IsVisible() && app._settingsToggle->GetToggleState() == std::optional<bool>(true),
                    "Settings opens as a pressed-state nonmodal drawer");
            Require(app._readingWidth->RequestValue(app.Host(), 920) && app._textScale->RequestValue(app.Host(), 125),
                    "real shared stepper commits update appearance preferences");
            Require(app.Preferences().readingWidth == 920 && app.Preferences().textScale == 125 &&
                        Near(app.Document().GetAppearance().maximumTextWidthDip, 920) && Near(app.Document().GetAppearance().bodyFontSizeDip, 20),
                    "reading measure and text scale apply immediately to the authoritative editor");
            app.Execute(Command::ThemePaper);
            Require(app.Preferences().appearance == Appearance::Paper && app._themeButtons[3]->GetToggleState() == std::optional<bool>(true),
                    "theme tiles expose the current live appearance through pressed state");
            app.Execute(Command::DimBlocks);
            Require(! app.Preferences().dimInactiveBlocks && ! app._dimToggle->IsChecked(),
                    "focus dimming is a live persisted preference with a real toggle state");
            const auto selection = app.Document().GetSelection();
            Require(! app.Preferences().strikeCompletedTasks && ! app.Document().GetAppearance().strikeCompletedTasks,
                    "completed-task strike-through starts disabled");
            app.Execute(Command::StrikeCompletedTasks);
            app.SetView(EditorView::Split);
            app.Prepare(256);
            Require(app.Preferences().strikeCompletedTasks && app.Document().GetAppearance().strikeCompletedTasks &&
                        app._mirror->GetAppearance().strikeCompletedTasks && app._strikeTasksToggle->IsChecked(),
                    "the real preference applies to Compose and the Split mirror");
            app.SetView(EditorView::Source);
            app.SetView(EditorView::Compose);
            app.Execute(Command::StrikeCompletedTasks);
            app.Prepare(256);
            const auto retainedSelection = app.Document().GetSelection();
            Require(! app.Document().GetAppearance().strikeCompletedTasks && ! app._mirror->GetAppearance().strikeCompletedTasks &&
                        app.Document().Snapshot().IsSameRevision(revision) && selection.anchor == retainedSelection.anchor &&
                        selection.caret == retainedSelection.caret && ! app.Dirty(),
                    "completion presentation round-trips views without changing source, selection, revision or dirty state");
            Require(! app.Document().CanUndo(), "presentation commands leave the empty undo history unchanged");
            Require(! app.Preferences().allowRemoteImages && ! app.Document().RemoteImagesEnabled() && ! app._remoteImagesToggle->IsChecked(),
                    "the privacy control starts off and reports the actual editor image policy");
            app.Execute(Command::RemoteImages);
            Require(app.Preferences().allowRemoteImages && app.Document().RemoteImagesEnabled() && app._remoteImagesToggle->IsChecked(),
                    "the live privacy control applies a usable opt-in rather than a decorative preference");
            app.Execute(Command::RemoteImages);
            for (const auto* help : {app._strikeTasksHelp, app._localNetworkImagesHelp, app._imageFirstPasteHelp, app._spellingHelp})
            {
                const auto bounds = help->GetBounds();
                DxUi::DesiredSize desired;
                Success(DxUi::MeasureControl(*help, {app.Host()}, DxUi::LayoutConstraints::AtMost(bounds.right - bounds.left), desired),
                        "measure a settings explanation beneath its toggle");
                Require(desired.sizeDip.height <= bounds.bottom - bounds.top + 0.5f, "settings explanation fits its measured bounds beneath its toggle");
            }
            app._settingsScroll->SetScrollOffset(400);
            Require(app._settingsScroll->GetScrollOffset() > 0 &&
                        app._settingsScroll->GetContentHeight() > app._settingsScroll->GetViewportRect().bottom - app._settingsScroll->GetViewportRect().top,
                    "the minimum-window drawer can scroll its complete settings content");
            Require(app.DismissSurface() && ! app._drawer->IsVisible() && app.Host().GetFocusControl() == &app.Document() &&
                        app.Document().Snapshot().IsSameRevision(revision) && ! app.Dirty(),
                    "Escape returns logical editor routing without creating source/history edits");
            Require(foreground.NeverAcquiredForeground(), "live settings never take desktop focus in hidden tests");
        });
        test("Editor.MarkdownSyntaxOptionsReanalyzeLive",
             [&]
        {
            using DxUi::RichText::MarkdownExtensions;
            using DxUi::RichText::MarkdownInlineKind;
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"syntax-options"));
            Success(app.Initialize(), "create hidden syntax-options app");
            const std::wstring text = L"Mark ==this== and see [[Home|home]].\n";
            app.SetDocument(text);
            const auto revision = app.Document().Snapshot();
            Resize(app, 800, 560);
            app.Execute(Command::Settings);
            app.Prepare(256);
            const auto inlineKind = [&](MarkdownInlineKind kind)
            {
                const auto syntax = app.Document().GetMarkdownSyntax();
                return syntax && std::ranges::any_of(syntax->Inlines(), [&](const auto& node) { return node.kind == kind; });
            };
            const auto wikiLink = [&]
            {
                const auto syntax = app.Document().GetMarkdownSyntax();
                return syntax && std::ranges::any_of(syntax->Inlines(), [](const auto& node) { return node.wiki; });
            };
            const auto views = [&](MarkdownExtensions expected)
            {
                return app.Document().GetMarkdownAnalysisExtensions() == expected && app._mirror->GetMarkdownAnalysisExtensions() == expected &&
                       app._equation->GetMarkdownAnalysisExtensions() == expected;
            };
            // GFM (31) plus every optional RichMarkdown bit through abbreviations.
            Require(views(MarkdownExtensions::RichOptionalMask) && inlineKind(MarkdownInlineKind::Highlight) && wikiLink() &&
                        std::ranges::all_of(app._syntaxToggles, [](auto* toggle) { return toggle->IsChecked(); }),
                    "every optional syntax starts enabled on the editor, mirror and inspector and is analysed");
            for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
            {
                const auto bounds = app._syntaxHelp[index]->GetBounds();
                DxUi::DesiredSize desired;
                Success(DxUi::MeasureControl(*app._syntaxHelp[index], {app.Host()}, DxUi::LayoutConstraints::AtMost(bounds.right - bounds.left), desired),
                        "measure a syntax explanation at its drawer width");
                Require(desired.sizeDip.height <= bounds.bottom - bounds.top + 0.5f && ! app._syntaxToggles[index]->GetAccessibleHelpText().empty(),
                        std::format("syntax explanation {} fits its reserved lines and is the toggle's help text", index).c_str());
            }
            app.Execute(Command::SyntaxHighlight);
            app.Prepare(256);
            Require(! app.Preferences().SyntaxEnabled(MarkdownSyntax::Highlight) && ! app._syntaxToggles[0]->IsChecked() &&
                        views(static_cast<MarkdownExtensions>(4095u & ~32u)) && ! inlineKind(MarkdownInlineKind::Highlight) && wikiLink(),
                    "turning Highlight off reanalyses all three views and leaves other syntax enabled");
            app.Execute(Command::SyntaxWikiLinks);
            app.Prepare(256);
            Require(! app.Preferences().SyntaxEnabled(MarkdownSyntax::WikiLinks) && views(static_cast<MarkdownExtensions>(4095u & ~32u & ~512u)) &&
                        ! wikiLink(),
                    "turning wiki links off keeps [[target]] as literal text");
            app.Execute(Command::SyntaxHighlight);
            app.Execute(Command::SyntaxWikiLinks);
            app.Prepare(256);
            Require(views(MarkdownExtensions::RichOptionalMask) && inlineKind(MarkdownInlineKind::Highlight) && wikiLink() &&
                        app.Document().GetText() == text && app.Document().Snapshot().IsSameRevision(revision) && ! app.Dirty(),
                    "syntax options change presentation only, never source, revision or save state");
            Require(foreground.NeverAcquiredForeground(), "syntax options never take desktop focus in hidden tests");
        });
        test("Rendering.DocumentThemedScrollbars",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"document-scrollbars"));
            Success(app.Initialize(), "create hidden scrollbar app");
            std::wstring text = L"> A useful notebook keeps the observation beside its explanation.\n>\n"
                                L"> Keep one thought per paragraph, and leave room for the next question.\n\n";
            for (unsigned line = 0; line < 80; ++line)
                text += L"A notebook paragraph leaves enough room for the next observation.\n\n";
            app.SetDocument(text);
            app.Document().SetSelection({text.size(), text.size()});
            Require(app.Document().ReplaceSelection(L"!") && app.Document().Undo(), "scrollbar fixture retains redo history");
            const DxUi::RichText::Selection selection{4, 4};
            app.Document().SetSelection(selection);
            const auto source   = app.Document().Snapshot();
            const auto revision = app.Document().Revision();
            for (const UINT dpi : {96u, 144u, 192u})
                for (const auto theme : {Appearance::Light, Appearance::Dark, Appearance::Paper})
                    for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
                    {
                        Resize(app, 1440, 900, dpi, false);
                        app._settings.appearance = theme;
                        app.Execute(view == EditorView::Compose ? Command::Compose : view == EditorView::Source ? Command::Source : Command::Split);
                        app.ApplyTheme();
                        app.Layout();
                        app.Prepare(512);
                        Require(app.Document().GetAppearance().showVerticalScrollbar && app.SourceMirror().GetAppearance().showVerticalScrollbar,
                                "Compose, Source and the Split mirror opt into the native scrollbar lane");
                        for (auto* editor : std::array<DxUi::RichTextEditor*, 2>{app._editor, app._mirror})
                        {
                            if (! editor->IsVisible())
                                continue;
                            const auto info = editor->GetScrollInfo();
                            Require(info && info->viewportHeightDip > 0 && info->contentHeightDip > info->viewportHeightDip,
                                    std::format("long document overflows at DPI {}, theme {}, view {}, mirror {} (ready {}, viewport {}, content {})",
                                                dpi,
                                                static_cast<int>(theme),
                                                static_cast<int>(view),
                                                editor == app._mirror,
                                                info.has_value(),
                                                info ? info->viewportHeightDip : 0,
                                                info ? info->contentHeightDip : 0));
                            (void)editor->SetScrollOffsetDip(0);
                            const auto bounds = editor->GetBounds();
                            const auto lane   = D2D1::RectF(bounds.right - 8, bounds.top + 2, bounds.right, bounds.bottom - 2);
                            const auto image =
                                root /
                                std::format(
                                    L"scrollbar-{}-{}-{}-{}.png", dpi, static_cast<int>(theme), static_cast<int>(view), editor == &app.Document() ? 0 : 1);
                            Require(app.Capture(image), "capture a hidden overflow scrollbar");
                            const auto ordinary   = ReadControlRaster(image, lane, app.Host().GetDpi());
                            const auto palette    = app.Host().GetTheme();
                            auto marked           = palette;
                            marked.scrollbarThumb = D2D1::ColorF(.1f, .8f, .35f, 1);
                            app.Host().SetTheme(marked);
                            Require(app.Capture(image), "capture the same native scrollbar with a distinct theme thumb");
                            Require(ordinary != ReadControlRaster(image, lane, app.Host().GetDpi()),
                                    "overflow thumb pixels respond to the host theme in the scrollbar lane");
                            app.Host().SetTheme(palette);
                            app.Prepare(512);
                            const auto page = D2D1::Point2F(bounds.right - 4, bounds.bottom - 12);
                            Require(editor->OnMouseDown(app.Host(), page, false, 0) && editor->GetScrollInfo()->offsetDip > 0,
                                    "clicking below the thumb pages the document");
                            (void)editor->OnMouseUp(app.Host(), page, false, 0);
                            (void)editor->SetScrollOffsetDip(0);
                            const auto thumb = D2D1::Point2F(bounds.right - 4, bounds.top + 6);
                            Require(editor->OnMouseDown(app.Host(), thumb, false, 0) && app.Host().GetCapturedControl() == editor,
                                    "primary thumb press owns native control capture");
                            const auto dragged = D2D1::Point2F(thumb.x, thumb.y + 60);
                            (void)editor->OnMouseMove(app.Host(), dragged, 0);
                            Require(editor->GetScrollInfo()->offsetDip > 0 && editor->OnMouseUp(app.Host(), dragged, false, 0) &&
                                        app.Host().GetCapturedControl() == nullptr,
                                    "thumb drag scrolls and release retires capture");
                        }
                        Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().Revision() == revision && app.Document().GetText() == text &&
                                    app.Document().GetSelection() == selection && ! app.Document().CanUndo() && app.Document().CanRedo(),
                                "scrolling, theme, DPI and view changes preserve source ACP and redo history");
                    }
            app.SetDocument(L"A short notebook.\n");
            app.Execute(Command::Compose);
            app.Layout();
            app.Prepare(512);
            const auto shortInfo = app.Document().GetScrollInfo();
            Require(shortInfo && shortInfo->contentHeightDip <= shortInfo->viewportHeightDip, "a fitting document has no scroll extent");
            const auto bounds = app.Document().GetBounds();
            const auto lane   = D2D1::RectF(bounds.right - 8, bounds.top + 2, bounds.right, bounds.bottom - 2);
            const auto image  = root / L"scrollbar-fitting.png";
            Require(app.Capture(image), "capture the fitting document");
            const auto fitting    = ReadControlRaster(image, lane, app.Host().GetDpi());
            auto marked           = app.Host().GetTheme();
            marked.scrollbarThumb = D2D1::ColorF(.1f, .8f, .35f, 1);
            app.Host().SetTheme(marked);
            Require(app.Capture(image) && fitting == ReadControlRaster(image, lane, app.Host().GetDpi()), "a fitting document paints no phantom thumb");
            Require(! IsWindowVisible(app.Window()) && foreground.NeverAcquiredForeground(),
                    "scrollbar regression never shows a window or takes desktop focus");
        });
        test("Rendering.DesignRefreshTypographyAndThemeRoles",
             [&]
        {
            {
                Testing::ForegroundOwnershipObserver foreground;
                Application lazy(Options(root, L"design-lazy-layout"));
                Success(lazy.Initialize(), "create hidden app before its first explicit preparation");
                Require(lazy.Host().GetTypographyRevision() == 0 && lazy.Host().GetDeviceContext() == nullptr && lazy._filesFooterMeasure.measurements == 0,
                        "hidden initialization leaves this host's text formats, graphics context and closed footer unprepared");
                const std::wstring text = L"# Quiet writing\n\nKeep these source words.\n";
                lazy.SetDocument(text);
                lazy.Document().SetSelection({text.size(), text.size()});
                Require(lazy.Document().ReplaceSelection(L"!") && lazy.Document().Undo(), "unprepared layout fixture has a real retained redo transaction");
                const auto source    = lazy.Document().Snapshot();
                const auto revision  = lazy.Document().Revision();
                const auto selection = lazy.Document().GetSelection();
                for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
                {
                    lazy.SetView(view);
                    Resize(lazy, view == EditorView::Source ? 800u : 1440u, 900, view == EditorView::Split ? 192u : 96u, false);
                    lazy._settings.appearance = view == EditorView::Source ? Appearance::Paper : Appearance::Light;
                    lazy.ApplyTheme();
                    lazy.Layout();
                    Require(lazy.Host().GetTypographyRevision() == 0 && lazy.Host().GetDeviceContext() == nullptr &&
                                lazy._filesFooterMeasure.measurements == 0 && ! lazy.SourceMirror().GetAppearance().bodyFontFamily.empty(),
                            "source-family policy, hidden footer, view, DPI and theme layout acquire no eager host text or graphics resources");
                    if (view == EditorView::Source)
                        Require(lazy.Document().GetAppearance().bodyFontFamily == lazy.SourceMirror().GetAppearance().bodyFontFamily,
                                "Source and the derived mirror receive the same nonempty monospace family before preparation");
                    Require(lazy.Document().Snapshot().IsSameRevision(source) && lazy.Document().Revision() == revision &&
                                lazy.Document().GetSelection() == selection && lazy.Document().GetText() == text && ! lazy.Document().CanUndo() &&
                                lazy.Document().CanRedo() && ! lazy.Dirty(),
                            "unprepared view and layout changes preserve source ACP, revision, selection and redo history");
                }
                lazy.Prepare(256);
                auto* monospace = lazy.Host().GetTextFormat(DxUi::FontRole::Monospace);
                Require(monospace && lazy.Host().GetTypographyRevision() != 0 && lazy.Host().GetDeviceContext() != nullptr,
                        "the first real preparation initializes the ordinary shared host resources");
                std::wstring resolvedFamily(static_cast<std::size_t>(monospace->GetFontFamilyNameLength()) + 1, L'\0');
                Success(monospace->GetFontFamilyName(resolvedFamily.data(), static_cast<UINT32>(resolvedFamily.size())),
                        "read the native resolved monospace family after preparation");
                resolvedFamily.pop_back();
                Require(lazy.SourceMirror().GetAppearance().bodyFontFamily == resolvedFamily && lazy.SourceMirror().GetText() == text &&
                            lazy.Document().Snapshot().IsSameRevision(source) && lazy.Document().Revision() == revision &&
                            lazy.Document().GetSelection() == selection && ! lazy.Document().CanUndo() && lazy.Document().CanRedo() && ! lazy.Dirty(),
                        "deferred canonical family selection matches native typography and leaves the derived source/history exact");
                Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(lazy.Window()),
                        "lazy-resource checks never show or activate the owned test window");
            }
            Application app(Options(root, L"design-render"));
            Success(app.Initialize(), "create hidden design appearance app");
            const std::wstring documentText = L"# Quiet writing\n\nA [link](https://example.invalid) stays local.\n\n"
                                              L"- First item\n- Second item\n\nParagraph one\n\nParagraph two";
            app.SetDocument(documentText);
            app.Prepare(256);
            app.Document().SetSelection({documentText.size(), documentText.size()});
            Require(app.Document().ReplaceSelection(L"!") && app.Document().Undo(), "typography fixture retains a real redo transaction");
            const auto paragraphOffset = documentText.find(L"Paragraph one");
            const DxUi::RichText::Selection retainedSelection{paragraphOffset + 3u, paragraphOffset + 3u};
            app.Document().SetSelection(retainedSelection);
            const auto source         = app.Document().Snapshot();
            const auto sourceRevision = app.Document().Revision();
            const auto checkSpacing   = [&]
            {
                app.Prepare(256);
                const auto firstItem       = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"First item"));
                const auto secondItem      = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"Second item"));
                const auto firstParagraph  = app.Document().TryGetTextInputCaretRect(app.Host(), paragraphOffset);
                const auto secondParagraph = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"Paragraph two"));
                const auto& type           = app.Document().GetAppearance();
                const float bodyLine       = type.bodyFontSizeDip * type.lineHeightMultiplier;
                // Native hit-test caret extents can be shorter than the
                // configured line advance. Equal-font top insets cancel;
                // compare complete row advances, without clipped UIA bounds.
                Require(type.markdownPresentation == DxUi::RichText::MarkdownPresentationStyle::Subtle && type.paragraphSpacingAfterLines == 18.f / 26.4f &&
                            type.listItemSpacingDip == 10.f && firstItem && secondItem && firstParagraph && secondParagraph &&
                            firstItem->bottom > firstItem->top && firstParagraph->bottom > firstParagraph->top &&
                            Near(secondItem->top - firstItem->top - bodyLine, 10) && Near(secondParagraph->top - firstParagraph->top, 2 * bodyLine),
                        "native rich geometry uses ten-DIP item gaps and credits one real blank line toward its body-line paragraph margin");
                Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().Revision() == sourceRevision &&
                            app.Document().GetSelection() == retainedSelection && app.Document().GetText() == documentText && ! app.Document().CanUndo() &&
                            app.Document().CanRedo(),
                        "shared typography preparation preserves exact source, caret, revision and redo history");
            };
            checkSpacing();
            Require(Near(app._saveButton->GetContentFontSizeDip().value_or(0), 14) && Near(app._modeButtons[0]->GetContentFontSizeDip().value_or(0), 14) &&
                        Near(app._tools[2]->GetContentFontSizeDip().value_or(0), 14) && Near(app._tools[3]->GetContentFontSizeDip().value_or(0), 18) &&
                        Near(app._minimizeButton->GetContentFontSizeDip().value_or(0), 12),
                    "chrome uses shared 14-DIP labels, 18-DIP command glyphs and compact 12-DIP native caption glyphs");
            app.Execute(Command::ThemeLight);
            if (! app.Host().GetTheme().highContrast)
            {
                Color(app.Host().GetTheme().windowBackground, 0xF0F1F4, "Light chrome matches the design role");
                Color(app.Host().GetTheme().surfaceBackground, 0xF8F8FA, "Light panes match the design role");
                Color(app.Host().GetTheme().cardBackground, 0xFFFFFF, "Light page matches the design role");
                Color(app.Host().GetTheme().accent, 0xC83C46, "Light accent matches the accessible link role");
                Color(app.Host().GetTheme().selectionFill, 0xF2CFD3, "Light active selection uses the product tint");
                Color(app.Host().GetTheme().selectionInactiveFill, 0xF2CFD3, "Light inactive current rows retain the product tint");
                Color(app.Host().GetTheme().selectionText, 0xA12A34, "Light selected labels retain contrast on the product tint");
                Require(app.Document().GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret,
                        "Light rich links use hover/caret decoration through the shared editor policy");
            }
            app.Execute(Command::ThemePaper);
            if (! app.Host().GetTheme().highContrast)
                Color(app.Host().GetTheme().cardBackground, 0xFFFAEF, "Paper page matches the design role");
            app.Execute(Command::ThemeDark);
            if (! app.Host().GetTheme().highContrast)
            {
                Color(app.Host().GetTheme().windowBackground, 0x17191E, "Dark chrome matches the design role");
                Color(app.Host().GetTheme().surfaceBackground, 0x1C1F25, "Dark panes match the design role");
                Color(app.Host().GetTheme().cardBackground, 0x22252B, "Dark page matches the design role");
                Color(app.Host().GetTheme().selectionInactiveFill, 0xA63B46, "Dark inactive current rows retain the product tint");
                Require(app.Host().GetTheme().primaryButtonFill.has_value(), "the primary button has an explicit accessible fill");
                Color(app.Host().GetTheme().primaryButtonFill.value(), 0xC83C46, "the primary button retains contrast with its white label");
            }
            app._systemDark = false;
            app.Layout();
            const auto systemPreviewOnLight = app._themeButtons[0]->GetThemePreview();
            const auto lightPreview         = app._themeButtons[1]->GetThemePreview();
            const auto darkPreview          = app._themeButtons[2]->GetThemePreview();
            Require(systemPreviewOnLight.has_value() && lightPreview.has_value() && darkPreview.has_value() &&
                        systemPreviewOnLight.value() == lightPreview.value() && systemPreviewOnLight.value() != darkPreview.value(),
                    "System appearance preview uses the cached light system preference while explicit Dark is selected");
            app.Execute(Command::ThemeLight);
            app._systemDark = true;
            app.Layout();
            const auto systemPreviewOnDark = app._themeButtons[0]->GetThemePreview();
            const auto updatedDarkPreview  = app._themeButtons[2]->GetThemePreview();
            const auto updatedLightPreview = app._themeButtons[1]->GetThemePreview();
            Require(systemPreviewOnDark.has_value() && updatedDarkPreview.has_value() && updatedLightPreview.has_value() &&
                        systemPreviewOnDark.value() == updatedDarkPreview.value() && systemPreviewOnDark.value() != updatedLightPreview.value(),
                    "System appearance preview uses the cached dark system preference while explicit Light is selected");
            Testing::ForegroundOwnershipObserver captureForeground;
            std::string firstSettingsCapture;
            for (const bool initialSystemDark : {false, true})
            {
                auto captureOptions    = Options(root, initialSystemDark ? L"settings-capture-from-dark" : L"settings-capture-from-light", L"settings");
                captureOptions.capture = captureOptions.dataDirectory / L"settings.png";
                Application captureApp(captureOptions);
                Success(captureApp.Initialize(), "create a hidden Settings capture fixture");
                captureApp.Execute(Command::ThemeLight);
                captureApp._systemDark = initialSystemDark;
                captureApp.PrepareDesignScenario();
                const auto capturedSystemPreview = captureApp._themeButtons[0]->GetThemePreview();
                const auto capturedLightPreview  = captureApp._themeButtons[1]->GetThemePreview();
                Require(! captureApp._systemDark && capturedSystemPreview.has_value() && capturedLightPreview.has_value() &&
                            capturedSystemPreview.value() == capturedLightPreview.value(),
                        "the Settings fixture resolves both cached starting preferences to declared System Light");
                std::string captureMetadata;
                auto captureSidecar = captureOptions.capture;
                captureSidecar += L".scenario.json";
                Success(ReadBytes(captureSidecar, captureMetadata), "read the actual Settings fixture provenance");
                Require(captureMetadata.find("\"effectiveSystemPreference\": \"light\"") != std::string::npos &&
                            captureMetadata.find("\"systemPreferenceSource\": \"synthetic-fixture\"") != std::string::npos,
                        "Settings provenance declares the effective synthetic System preference");
                Require(captureApp.Document().ReplaceSelection(L"!") && captureApp.Document().Undo() && captureApp.Document().CanRedo(),
                        "the seeded Settings document retains a real redo transaction before capture");
                const auto captureSource    = captureApp.Document().Snapshot();
                const auto captureRevision  = captureApp.Document().Revision();
                const auto captureSelection = captureApp.Document().GetSelection();
                const bool captureUndo      = captureApp.Document().CanUndo();
                const auto captureFocus     = captureApp.Host().GetFocusControl();
                captureApp.Layout();
                captureApp.Prepare(512);
                Require(captureApp.Capture(captureOptions.capture), "capture the actual hidden Settings render");
                Require(captureApp.Document().Snapshot().IsSameRevision(captureSource) && captureApp.Document().Revision() == captureRevision &&
                            captureApp.Document().GetSelection() == captureSelection && captureApp.Document().CanUndo() == captureUndo &&
                            captureApp.Document().CanRedo() && captureApp.Host().GetFocusControl() == captureFocus &&
                            captureForeground.NeverAcquiredForeground() && ! IsWindowVisible(captureApp.Window()),
                        "Settings fixture rendering preserves source, selection, both history directions and hidden logical focus");
                std::string settingsCapture;
                Success(ReadBytes(captureOptions.capture, settingsCapture), "read the actual Settings PNG");
                Require(! settingsCapture.empty() && (firstSettingsCapture.empty() || settingsCapture == firstSettingsCapture),
                        "Settings capture bytes are identical for initially Light and Dark cached preferences");
                firstSettingsCapture = std::move(settingsCapture);
            }
            app.Execute(Command::ThemeDark);
            Require(app.Document().GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret &&
                        app._mirror->GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret,
                    "Dark matches the reference with hover/caret link underlining in Compose and Split");
            const auto& appearance = app.Document().GetAppearance();
            Require(! appearance.drawBorder && appearance.useAccentLinkColor && Near(appearance.paddingDip, 0) && Near(appearance.maximumTextWidthDip, 760) &&
                        Near(appearance.bodyFontSizeDip, 16) && Near(appearance.codeFontSizeDip, 13.5f),
                    "the page uses shared rich-editor appearance with no nested editor frame and the intended typography");
            Require(app.Host().RenderInitialFrameForShow() && app.Document().Snapshot().IsSameRevision(source),
                    "theme/type presentation renders through the native host without changing source history");
            Require(app._outlineTitle->GetTypographyOverride().has_value() && Near(app._outlineTitle->GetTypographyOverride().value().fontSizeDip, 11) &&
                        Near(app._outlineTitle->GetTypographyOverride().value().characterSpacingDip, .88f) &&
                        app._outlineTree->GetTypographyOverride().has_value() && Near(app._outlineTree->GetTypographyOverride().value().fontSizeDip, 13) &&
                        app._themeButtons[3]->GetThemePreview().has_value(),
                    "sections, hierarchical rows and theme previews use shared typography/presentation APIs");
            app.SetView(EditorView::Source);
            Require(Near(app.Document().GetAppearance().bodyFontSizeDip, 13.5f) && Near(app.Document().GetAppearance().lineHeightMultiplier, 1.75f) &&
                        ! app.Document().GetAppearance().bodyFontFamily.empty() && app.Document().GetAppearance().colorizeMarkdownSource,
                    "Source retains monospace 13.5/1.75 syntax ink instead of inheriting rich prose typography");
            app.SetView(EditorView::Split);
            app.Prepare(256);
            checkSpacing();
            Require(Near(app.SourceMirror().GetAppearance().bodyFontSizeDip, 13.5f) && app.SourceMirror().GetAppearance().colorizeMarkdownSource &&
                        app.Document().Snapshot().IsSameRevision(source) && app.SourceMirror().GetMarkdownSyntax() &&
                        ! app.SourceMirror().DebugGetState().sourceInkPreparationPending && app.SourceMirror().DebugGetState().sourceInkRangeCount > 0,
                    "the read-only Split mirror uses the source appearance without changing document history");
            Application contrast(Options(root, L"design-high-contrast", L"high-contrast"));
            Success(contrast.Initialize(), "create deterministic hidden high-contrast app");
            Require(contrast.Host().GetTheme().highContrast, "the harness high-contrast scenario uses the shared accessibility palette");
            const auto background = GetSysColor(COLOR_WINDOW);
            const unsigned rgb =
                (static_cast<unsigned>(GetRValue(background)) << 16) | (static_cast<unsigned>(GetGValue(background)) << 8) | GetBValue(background);
            Color(contrast.Host().GetTheme().cardBackground, rgb, "high contrast uses the system window surface");
            const auto hyperlink = GetSysColor(COLOR_HOTLIGHT);
            const unsigned linkRgb =
                (static_cast<unsigned>(GetRValue(hyperlink)) << 16) | (static_cast<unsigned>(GetGValue(hyperlink)) << 8) | GetBValue(hyperlink);
            Color(contrast.Host().GetTheme().accent, linkRgb, "high-contrast rich links use the system hyperlink ink");
            Require(contrast.Document().GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::Always, "high contrast always retains an underline");
            const auto module = GetModuleHandleW(nullptr);
            for (const unsigned size : {16u, 24u, 32u, 48u, 64u, 128u, 256u})
                CheckBrandIconResource(module, size);
            for (const unsigned size : {20u, 40u, 96u})
                CheckBrandIconResource(module, size, false);
            const auto defaultIcon   = LoadIconW(module, MAKEINTRESOURCEW(IDI_REDTILDE)); // Shared class icon: borrowed, never destroyed here.
            const auto defaultWidth  = static_cast<unsigned>(GetSystemMetrics(SM_CXICON));
            const auto defaultHeight = static_cast<unsigned>(GetSystemMetrics(SM_CYICON));
            constexpr std::array nativeIconSizes{16u, 24u, 32u, 48u, 64u, 128u, 256u};
            const bool defaultNative = defaultWidth == defaultHeight && std::ranges::find(nativeIconSizes, defaultWidth) != nativeIconSizes.end();
            CheckBrandIconPixels(defaultIcon, defaultWidth, defaultHeight, defaultNative);
            Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().Revision() == sourceRevision &&
                        app.Document().GetSelection() == retainedSelection && ! app.Document().CanUndo() && app.Document().CanRedo(),
                    "theme, view, native spacing and icon checks leave exact source, selection and history unchanged");
        });
        test("Editor.DesignRefreshDeferredSelectionToolsFocus",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"design-deferred-tools"));
            Success(app.Initialize(), "create hidden deferred-tools app");
            app.SetDocument(L"# Prepared tools\n\nSelect these careful words.\n");
            app.Prepare(512);
            const auto source = app.Document().Snapshot();
            const auto at     = app.Document().GetText().find(L"careful");
            app.Document().SetSelection({at, at + 7});
            auto bounds               = app.Document().GetBounds();
            bounds.right              = bounds.left + 600;
            const auto retireGeometry = [&]
            {
                bounds.right -= 1;
                app.Document().SetBounds(bounds);
                app.Host().SetFocusControl(&app.Document(), false);
            };
            retireGeometry();
            app.FocusSelectionTools();
            Require(app._selectionToolsFocus && app._pendingSelectionTools, "Alt+F10 intent survives temporarily unavailable native geometry");
            app.Prepare(512);
            Require(app._floating->IsVisible() && ! app._selectionToolsFocus && ! app._pendingSelectionTools &&
                        app.Host().GetFocusControl() == app._floatingActions.front(),
                    "the prepared selection fulfills keyboard focus on the first toolbar action");
            app.CloseDrawers();
            retireGeometry();
            app.FocusSelectionTools();
            Require(app._selectionToolsFocus && app.DismissSurface(), "Escape owns a deferred toolbar request before it becomes visible");
            app.Prepare(512);
            Require(! app._selectionToolsFocus && ! app._floating->IsVisible() && app.Host().GetFocusControl() == &app.Document(),
                    "Escape cancels deferred first-action focus and prevents a late toolbar reopening");
            retireGeometry();
            app.FocusSelectionTools();
            Require(app._selectionToolsFocus && app._pendingSelectionTools, "a changed native measure defers focus before selection cancellation");
            app.Document().SetSelection({at + 1, at + 7});
            app.Prepare(512);
            Require(! app._selectionToolsFocus && app.Host().GetFocusControl() == &app.Document(),
                    "a changed selection cannot fulfill an older toolbar focus request");
            app.CloseDrawers();
            retireGeometry();
            app.FocusSelectionTools();
            Require(app._selectionToolsFocus && app._pendingSelectionTools, "a changed native measure defers focus before revision cancellation");
            const auto retainedSelection = app.Document().GetSelection();
            const DxUi::RichText::SourceEdit append{{source.Length(), source.Length()}, L"A "};
            Require(app.Document().ApplySourceEdits(source, app.Document().Revision(), std::span(&append, 1), retainedSelection) &&
                        app.Document().GetSelection() == retainedSelection,
                    "admit a new source revision while preserving the deferred request's selection");
            app.Prepare(512);
            Require(! app._selectionToolsFocus && app.Host().GetFocusControl() == &app.Document() && app.Document().Undo() &&
                        app.Document().GetText() == source.Text(),
                    "a changed source revision cancels deferred focus while keeping one exact undo transaction");
            app.Document().SetSelection({at, at + 7});
            app.Prepare(512);
            app.Host().SetFocusControl(app._floatingActions[1], false);
            MSG activation{};
            activation.hwnd    = app.Window();
            activation.message = WM_KEYDOWN;
            activation.wParam  = VK_RETURN;
            activation.lParam  = 1 | static_cast<LPARAM>(MapVirtualKeyW(VK_RETURN, MAPVK_VK_TO_VSC) << 16);
            activation.time    = GetTickCount();
            Require(app.Host().DispatchKeyboardMessage(activation), "dispatch toolbar activation through the canonical pre-translation hook");
            auto formatted = source.Text();
            formatted.replace(at, 7, L"**careful**");
            Require(app.Document().GetText() == formatted && app.Host().GetFocusControl() == &app.Document(),
                    "the toolbar action applies Bold and returns logical editor focus on keydown");
            // The canonical pump owns character translation before a
            // callback changes focus. A command must create no editor char.
            MSG character{};
            Require(! PeekMessageW(&character, app.Window(), WM_CHAR, WM_CHAR, PM_NOREMOVE),
                    "command activation generates no translated character for the refocused editor");
            (void)SendMessageW(app.Window(), WM_KEYUP, VK_RETURN, activation.lParam);
            Require(app.Document().GetText() == formatted && app.Document().Undo() && app.Document().GetText() == source.Text(),
                    "the translated activation character cannot enter the refocused editor or create a second undo transaction");
            app.Document().SetSelection({source.Length(), source.Length()});
            app.Prepare(512);
            activation.wParam = VK_SPACE;
            activation.lParam = 1 | static_cast<LPARAM>(MapVirtualKeyW(VK_SPACE, MAPVK_VK_TO_VSC) << 16);
            Require(app.Host().DispatchKeyboardMessage(activation) && PeekMessageW(&character, app.Window(), WM_CHAR, WM_CHAR, PM_REMOVE) &&
                        character.wParam == L' ',
                    "ordinary same-owner editor Space retains one native translated character");
            (void)DispatchMessageW(&character);
            Require(app.Document().GetText() == source.Text() + L" " && app.Document().Undo() && app.Document().GetText() == source.Text(),
                    "ordinary editor typing remains one exact undo transaction through the canonical pump");
            {
                KeyboardState::ScopedSynthetic syntheticKeyboard;
                std::array<BYTE, 256> savedKeyboard{};
                Require(GetKeyboardState(savedKeyboard.data()) != FALSE, "snapshot only this test thread's keyboard state");
                const auto restoreKeyboard = wil::scope_exit([&] noexcept
                {
                    if (! SetKeyboardState(savedKeyboard.data()))
                        std::terminate();
                });
                const auto modifiers       = [&](bool ctrl, bool alt, bool shift)
                {
                    std::array<BYTE, 256> keys{};
                    keys[VK_CONTROL] = keys[VK_LCONTROL] = ctrl ? 0x80 : 0;
                    keys[VK_MENU] = keys[VK_RMENU] = alt ? 0x80 : 0;
                    keys[VK_SHIFT] = keys[VK_LSHIFT] = shift ? 0x80 : 0;
                    KeyboardState::Write(keys);
                };
                MSG routed{};
                routed.hwnd         = app.Window();
                routed.message      = WM_KEYDOWN;
                const auto retained = app.Document().Snapshot();
                const auto selected = app.Document().GetSelection();
                const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
                const auto focus       = app.Host().GetFocusControl();
                const auto passThrough = [&](UINT key, bool ctrl, bool alt, bool shift)
                {
                    modifiers(ctrl, alt, shift);
                    routed.wParam = key;
                    Require(! app.RouteShortcut(routed), "unassigned modified keys pass through application shortcut routing");
                    Require(app.Document().Snapshot().IsSameRevision(retained) && app.Document().GetSelection() == selected &&
                                app.Document().CanUndo() == undo && app.Document().CanRedo() == redo && app.Host().GetFocusControl() == focus,
                            "AltGr and modified function keys preserve source, selection, history and focus");
                };
                passThrough('E', true, true, false);
                passThrough('K', true, true, false);
                for (const auto key : {VK_F2, VK_F11})
                {
                    passThrough(static_cast<UINT>(key), true, false, false);
                    passThrough(static_cast<UINT>(key), false, true, false);
                    passThrough(static_cast<UINT>(key), false, false, true);
                }
                passThrough(VK_F3, true, false, false);
                passThrough(VK_F3, false, true, false);
                passThrough(VK_F10, true, true, false);
                passThrough(VK_F10, false, true, true);
                modifiers(false, false, false);
                routed.wParam = VK_F3;
                Require(app.RouteShortcut(routed), "unmodified F3 retains Find-next routing");
                modifiers(false, false, true);
                Require(app.RouteShortcut(routed), "Shift+F3 retains Find-previous routing");
                modifiers(false, false, false);
                app.Document().SetSelection({at, at + 7});
                app.Prepare(512);
                modifiers(true, false, false);
                routed.wParam = 'E';
                auto code     = source.Text();
                code.replace(at, 7, L"`careful`");
                Require(app.RouteShortcut(routed) && app.Document().GetText() == code,
                        "ordinary Ctrl+E still applies the inline-code command to the exact source selection");
                modifiers(false, false, false);
                Require(app.Document().Undo() && app.Document().GetText() == source.Text() && app.Host().GetFocusControl() == &app.Document(),
                        "one undo restores the Ctrl+E source transaction and editor routing");
                app.Document().SetSelection({at, at + 7});
                app.Prepare(512);
                modifiers(false, true, false);
                routed.wParam = VK_F10;
                Require(app.RouteShortcut(routed), "Alt+F10 retains explicit selection-toolbar routing");
                app.Prepare(512);
                Require(app.Host().GetFocusControl() == app._floatingActions.front(), "Alt+F10 focuses the prepared first selection action");
                modifiers(false, false, false);
                app.CloseDrawers();
                Require(app.Document().ReplaceSelection(L"Retained writing"), "seed dirty shortcut-boundary source");
                const auto protectedSource           = app.Document().Snapshot();
                const auto protectedSelection        = app.Document().GetSelection();
                app._options.commandAdmissionForTest = [](Command command) { return command == Command::LocalSource ? E_BOUNDS : S_OK; };
                routed.wParam                        = VK_F2;
                app.DispatchUiMessage(routed);
                Require(app.Document().Snapshot().IsSameRevision(protectedSource) && app.Document().GetSelection() == protectedSelection && app.Dirty() &&
                            app.Document().CanUndo() && app._feedback->GetText() == Resource(IDS_ACTIONFAILED),
                        "the real message-pump shortcut boundary contains a recoverable failure and retains dirty source");
                app._options.commandAdmissionForTest = {};
            }
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "deferred logical-focus tests never take the desktop");
        });
        test("Window.DesignRefreshPersistentStatusFacts",
             [&]
        {
            Application app(Options(root, L"design-status"));
            Success(app.Initialize(), "create hidden persistent-status app");
            Resize(app, 1440, 900, 96);
            {
                const auto loader = app._images._state;
                Require(static_cast<bool>(loader), "the hidden app owns its image-loader state");
                {
                    std::lock_guard lock(loader->mutex);
                    Require(loader->phase == RichImageLoader::State::WorkerPhase::Idle && ! loader->jobCount && ! loader->current.Id(),
                            "the image-free fixture has no active or queued decoder");
                    Require(ResetEvent(loader->networkAbort.get()) != FALSE, "clear only the idle fixture's owned abort event");
                }
                for (unsigned index = 0; index < 8; ++index)
                    app._images.DiscardCancelled();
                Require(WaitForSingleObject(loader->networkAbort.get(), 0) == WAIT_TIMEOUT,
                        "pruning empty image requests leaves the actual owned network-abort event unsignalled");
            }
            Require(app._positionStatus->GetTypographyOverride().has_value() && app._stats->GetTypographyOverride().has_value() &&
                        Near(app._positionStatus->GetTypographyOverride().value().fontSizeDip, 12) &&
                        Near(app._stats->GetTypographyOverride().value().fontSizeDip, 12),
                    "persistent document facts use the specified 12-DIP status size");
            Require(Resource(IDS_BLOCKINFO).find(L'\x00B7') != std::wstring::npos && Resource(IDS_BLOCKINFO).find(L'\x00C2') == std::wstring::npos,
                    "resource compiler decodes the UTF-8 middle dot without mojibake");
            app.SetDocument(L"# Heading\r\n\r\nWords here.\nMixed tail.\r", {}, true);
            app.Prepare(256);
            const auto at = app.Document().GetText().find(L"Words");
            app.Document().SetSelection({at, at + 5});
            Require(app._positionStatus->GetText().find(Resource(IDS_EOLMIXED)) != std::wstring_view::npos,
                    "mixed CRLF/LF/CR source is reported truthfully in persistent status");
            const std::wstring counts(app._stats->GetText()), position(app._positionStatus->GetText());
            app.Status(IDS_FORMATTED);
            Require(app._stats->GetText() == counts && app._positionStatus->GetText() == position && app._feedback->GetText() == Resource(IDS_FORMATTED),
                    "transient feedback remains in the document footer instead of overwriting persistent document facts");
            app.Document().SetSelection({2, 2});
            app.SelectionChanged();
            Require(app._currentBlock->GetText().find(Resource(IDS_BLOCKHEADING)) != std::wstring_view::npos,
                    "Current block identifies the actual Markdown heading context in addition to its size");

            const auto expectBlock = [&](unsigned id, std::size_t characters, std::string_view message)
            {
                const auto kind = Resource(id);
                Require(app._currentBlock->GetText() == std::vformat(Resource(IDS_BLOCKINFO), std::make_wformat_args(kind, characters)), message);
            };
            app.SetDocument(L"Words here.\n\nMore prose.\n");
            app.Host().SetFocusControl(&app.Document(), false);
            app.Document().SetSelection({3, 3});
            app.Prepare(256);
            expectBlock(IDS_BLOCKPARAGRAPH, 11, "the paragraph inspector counts physical source characters independently of its syntax");
            const auto entrySource    = app.Document().Snapshot();
            const auto entrySelection = app.Document().GetSelection();
            const auto entryUndo = app.Document().CanUndo(), entryRedo = app.Document().CanRedo();
            const auto settledConstructions = app.Activity().currentBlockLabelConstructions;
            for (const auto view : {EditorView::Source, EditorView::Split, EditorView::Compose})
            {
                app.SetView(view);
                app.Prepare(256);
                expectBlock(IDS_BLOCKPARAGRAPH, 11, "equal paragraph facts remain exact across global views");
            }
            app.Execute(Command::Focus);
            app.Prepare(256);
            app.Execute(Command::Focus);
            app.Prepare(256);
            app.SelectionChanged(false);
            Require(app.Activity().currentBlockLabelConstructions == settledConstructions && app.Document().Snapshot().IsSameRevision(entrySource) &&
                        app.Document().GetSelection() == entrySelection && app.Document().CanUndo() == entryUndo && app.Document().CanRedo() == entryRedo &&
                        app.Host().GetFocusControl() == &app.Document(),
                    "equal View/Focus/syntax facts construct no localized label and preserve source, history and logical focus");
            Require(app.Document().ReplaceSelection(L"x"), "insert one source character in the current paragraph");
            expectBlock(IDS_BLOCKPARAGRAPH, 12, "a physical-line count change publishes immediately before preparation");
            Require(app.Activity().currentBlockLabelConstructions == settledConstructions + 1,
                    "a changed line count constructs the localized label exactly once");
            const auto syntaxUpdates = app.Activity().syntaxPresentationUpdates;
            app.Prepare(256);
            Require(app.Activity().currentBlockLabelConstructions == settledConstructions + 1 && app.Activity().syntaxPresentationUpdates > syntaxUpdates &&
                        app.Document().GetMarkdownSyntax() && app.Document().GetText() == L"Worxds here.\n\nMore prose.\n" &&
                        app.Document().GetSelection() == DxUi::RichText::Selection{4, 4},
                    "the new immutable syntax does not reconstruct equal paragraph facts or change the admitted edit");
            Require(app.Document().Undo(), "undo the single admitted source edit");
            expectBlock(IDS_BLOCKPARAGRAPH, 11, "undo restores the actual physical-line count immediately");
            app.Prepare(256);
            Require(app.Activity().currentBlockLabelConstructions == settledConstructions + 2 && app.Document().GetText() == entrySource.Text() &&
                        app.Document().GetSelection() == entrySelection && app.Document().CanUndo() == entryUndo && app.Document().CanRedo(),
                    "undo constructs only the restored count and preserves the one source transaction");

            app.SetDocument(L"plain\n");
            app.Document().SetSelection({2, 2});
            app.Host().SetFocusControl(&app.Document(), false);
            app.Prepare(256);
            expectBlock(IDS_BLOCKPARAGRAPH, 5, "the preview entry is a five-character paragraph");
            const auto paragraphCaptionConstructions = app.Activity().selectionCaptionConstructions;
            Require(app._tools[2]->GetText() == Resource(IDS_PARAGRAPH) && app._tools[2]->GetAccessibleName() == Resource(IDS_HEADING),
                    "the visible toolbar caption follows paragraph context while its command name stays Heading");
            const auto previewChanges = app.Activity().changes;
            const auto previewUndo = app.Document().CanUndo(), previewRedo = app.Document().CanRedo();
            const auto paragraphConstructions = app.Activity().currentBlockLabelConstructions;
            DxUi::TextInputState entry;
            entry.text       = L"plain\n";
            entry.multiline  = true;
            entry.caretIndex = 2;
            auto preview     = entry;
            preview.text     = L"# one\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                    "import a same-ACP, equal-length heading preview without a committed edit");
            app.Prepare(256);
            expectBlock(IDS_BLOCKHEADING, 5, "a resource-kind change at the same ACP and line length updates the inspector");
            Require(app.Activity().currentBlockLabelConstructions == paragraphConstructions + 1,
                    "ready heading syntax constructs its new localized kind exactly once");
            preview.text = L"**x**\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                    "import a same-length strong-text preview at the same caret");
            app.Prepare(256);
            expectBlock(IDS_BLOCKPARAGRAPH, 5, "strong inline source remains a five-character paragraph");
            Require(app._tools[3]->GetToggleState() == std::optional<bool>{true} && app._tools[5]->GetToggleState() == std::optional<bool>{false},
                    "returning to paragraph facts still publishes the exact strong formatting state");
            const auto strongConstructions = app.Activity().currentBlockLabelConstructions;
            preview.text                   = L"`one`\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                    "import different inline syntax with unchanged paragraph label facts");
            app.Prepare(256);
            expectBlock(IDS_BLOCKPARAGRAPH, 5, "inline code retains the exact same paragraph label");
            Require(app.Activity().currentBlockLabelConstructions == strongConstructions && app._tools[3]->GetToggleState() == std::optional<bool>{false} &&
                        app._tools[5]->GetToggleState() == std::optional<bool>{true},
                    "equal label facts skip construction while unrelated formatting toggles still change");
            preview.text = L"$x+y$\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                    "import an equal-length mathematical context at the same ACP");
            app.Prepare(512);
            expectBlock(IDS_MATH, 5, "the mathematical context overrides the paragraph's localized kind");
            Require(app._equationContext && app._equation->GetText() == L"\\[\nx+y\n\\]" && ! app._tools[3]->IsEnabled() &&
                        app._tools[2]->GetText() == Resource(IDS_MATH) && app._tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                        app.Activity().selectionCaptionConstructions == paragraphCaptionConstructions + 1,
                    "equation source/action state and localized visible caption change once while the command name stays Heading");
            const auto firstMathCaptionConstructions = app.Activity().selectionCaptionConstructions;
            preview.text                             = L"  $x$\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                    "shift the mathematical source range without changing caret ACP or physical-line length");
            Require(! app.Document().GetMarkdownSyntax() && ! app._equationContext && app._equation->GetText().empty() &&
                        app._tools[2]->GetText() == Resource(IDS_PARAGRAPH) &&
                        app.Activity().selectionCaptionConstructions == firstMathCaptionConstructions + 1,
                    "pending replacement syntax clears stale math and publishes the paragraph caption once");
            app.Prepare(512);
            expectBlock(IDS_MATH, 5, "a shifted same-ACP math range preserves the actual kind and count");
            Require(app._equationContext && app._equation->GetText() == L"\\[\nx\n\\]" && app._tools[2]->GetText() == Resource(IDS_MATH) &&
                        app._tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                        app.Activity().selectionCaptionConstructions == firstMathCaptionConstructions + 2,
                    "ready replacement syntax publishes its exact equation and restores the math caption once");
            const auto settledMathCaptionConstructions = app.Activity().selectionCaptionConstructions;
            app.SelectionChanged(false);
            app.Prepare(32);
            Require(app.Activity().selectionCaptionConstructions == settledMathCaptionConstructions && app._equationContext &&
                        app._equation->GetText() == L"\\[\nx\n\\]" && app._tools[2]->GetAccessibleName() == Resource(IDS_HEADING),
                    "repeating current ready math facts reuses the caption and preserves the exact equation and command name");
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), entry, false),
                    "cancel all transient previews back to the entry source");
            app.Prepare(256);
            expectBlock(IDS_BLOCKPARAGRAPH, 5, "preview cancellation restores the exact paragraph facts");
            Require(! app._equationContext && app._equation->GetText().empty() && app._tools[2]->GetText() == Resource(IDS_PARAGRAPH) &&
                        app._tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                        app.Activity().selectionCaptionConstructions == settledMathCaptionConstructions + 1 && app._tools[3]->IsEnabled() &&
                        app._tools[3]->GetToggleState() == std::optional<bool>{false} && app._tools[5]->GetToggleState() == std::optional<bool>{false} &&
                        app.Document().GetText() == entry.text && app.Document().GetSelection() == DxUi::RichText::Selection{2, 2} &&
                        app.Activity().changes == previewChanges && app.Document().CanUndo() == previewUndo && app.Document().CanRedo() == previewRedo &&
                        app.Host().GetFocusControl() == &app.Document(),
                    "preview/cancel refresh every dependent fact without committing source/history or changing logical focus");

            const auto checkBlockPositions =
                [&](std::wstring_view text, std::initializer_list<std::tuple<std::size_t, unsigned, std::size_t>> positions, bool outOfOrderLinks = false)
            {
                app.SetDocument(std::wstring(text));
                app.Document().SetSelection({text.size(), text.size()});
                Require(app.Document().ReplaceSelection(L" ") && app.Document().Undo(), "give each block-kind fixture a real retained redo transaction");
                app.Prepare(512);
                const auto syntax = app.Document().GetMarkdownSyntax();
                Require(syntax && syntax->Source() == text && app.Document().CanRedo(),
                        "classify only the current ready canonical publication with retained history");
                if (outOfOrderLinks)
                {
                    bool backwardLink{};
                    const auto& nodes = syntax->Nodes();
                    for (std::size_t index = 0; index < nodes.size(); ++index)
                        backwardLink = backwardLink || (nodes[index].nextSibling != DxUi::RichText::MarkdownNoNode && nodes[index].nextSibling < index);
                    Require(backwardLink, "reference extraction actually creates source-order links that differ from vector append order");
                }
                std::optional<std::pair<unsigned, std::size_t>> lastVisibleFacts;
                const auto firstPosition        = *positions.begin();
                const auto firstFacts           = std::pair{std::get<1>(firstPosition), std::get<2>(firstPosition)};
                const auto initialConstructions = app.Activity().currentBlockLabelConstructions;
                app.Document().SetSelection({std::get<0>(firstPosition), std::get<0>(firstPosition)});
                app.SelectionChanged(false);
                const auto initialLatestFacts = app._currentBlockLatestFacts;
                Require(initialLatestFacts && initialLatestFacts.value().resourceId == firstFacts.first &&
                            initialLatestFacts.value().lineCharacterCount == firstFacts.second,
                        "the first independently specified source position matches retained canonical facts");
                if (app._sidebar->IsVisible() && app._currentBlock->IsVisible())
                {
                    expectBlock(firstFacts.first, firstFacts.second, "the first source-position oracle is published when its label is visible");
                    lastVisibleFacts = firstFacts;
                }
                else
                {
                    Require(app.Activity().currentBlockLabelConstructions == initialConstructions,
                            "classifying the first hidden source position does not construct its current-block label");
                    if (! app._currentBlock->IsVisible())
                        Require(app._listSummary->IsVisible() && ! app._listSummary->GetText().empty(),
                                "the first hidden list position retains its visible nonempty list summary");
                }
                const auto snapshot = app.Document().Snapshot();
                const auto changes  = app.Activity().changes;
                const auto canUndo = app.Document().CanUndo(), canRedo = app.Document().CanRedo();
                for (const auto [caret, id, characters] : positions)
                {
                    const auto constructions = app.Activity().currentBlockLabelConstructions;
                    app.Document().SetSelection({caret, caret});
                    app.SelectionChanged(false);
                    const auto facts       = std::pair{id, characters};
                    const auto latestFacts = app._currentBlockLatestFacts;
                    Require(latestFacts && latestFacts.value().resourceId == id && latestFacts.value().lineCharacterCount == characters,
                            "the independently specified source position matches the retained canonical block kind and line count");
                    const bool labelPublished = app._sidebar->IsVisible() && app._currentBlock->IsVisible();
                    if (labelPublished)
                    {
                        expectBlock(id, characters, "the visible label matches the literal source-position kind and physical-line count");
                        const auto expectedConstructions = lastVisibleFacts && lastVisibleFacts.value() == facts ? 0u : 1u;
                        Require(app.Activity().currentBlockLabelConstructions == constructions + expectedConstructions,
                                "visible nested-kind/count transitions publish once relative to the last visible literal facts");
                        lastVisibleFacts = facts;
                    }
                    else
                    {
                        Require(app.Activity().currentBlockLabelConstructions == constructions,
                                "hidden current-block labels do not construct text for a newly classified source position");
                        if (! app._currentBlock->IsVisible())
                            Require(app._listSummary->IsVisible() && ! app._listSummary->GetText().empty(),
                                    "a locally hidden list block retains its visible nonempty list summary");
                    }
                    Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == DxUi::RichText::Selection{caret, caret} &&
                                app.Document().CanUndo() == canUndo && app.Document().CanRedo() == canRedo && app.Activity().changes == changes &&
                                app.Host().GetFocusControl() == &app.Document(),
                            "block lookup never rewrites source, ACP selection, committed history or logical focus");
                }
                Require(app.Document().Redo() && app.Document().GetText() == std::wstring(text) + L" " && app.Document().Undo() &&
                            app.Document().GetText() == text,
                        "the exact source transaction remains redoable and undoable after every classification query");
            };
            checkBlockPositions(L"> - [x] task\n>   - child\n\nOutside\n",
                                {{0, IDS_BLOCKLIST, 12},
                                 {2, IDS_BLOCKLIST, 12},
                                 {4, IDS_BLOCKLIST, 12},
                                 {13, IDS_BLOCKLIST, 11},
                                 {19, IDS_BLOCKLIST, 11},
                                 {26, IDS_BLOCKPARAGRAPH, 7},
                                 {34, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"> # Head\n> words\n",
                                {{0, IDS_BLOCKHEADING, 8},
                                 {2, IDS_BLOCKHEADING, 8},
                                 {8, IDS_BLOCKHEADING, 8},
                                 {9, IDS_BLOCKQUOTE, 7},
                                 {11, IDS_BLOCKQUOTE, 7},
                                 {17, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"> ```\n> code\n> ```\n",
                                {{0, IDS_BLOCKCODE, 5},
                                 {5, IDS_BLOCKCODE, 5},
                                 {6, IDS_BLOCKCODE, 6},
                                 {8, IDS_BLOCKCODE, 6},
                                 {13, IDS_BLOCKCODE, 5},
                                 {19, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"> Title\n> =====\n", {{2, IDS_BLOCKHEADING, 7}, {10, IDS_BLOCKHEADING, 7}, {16, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L">     code\n", {{0, IDS_BLOCKCODE, 10}, {6, IDS_BLOCKCODE, 10}, {11, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"> - # Head\n", {{0, IDS_BLOCKHEADING, 10}, {6, IDS_BLOCKHEADING, 10}, {11, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"| A | B |\n| - | - |\n| x | y |\n",
                                {{0, IDS_BLOCKTABLE, 9},
                                 {2, IDS_BLOCKTABLE, 9},
                                 {10, IDS_BLOCKTABLE, 9},
                                 {12, IDS_BLOCKTABLE, 9},
                                 {20, IDS_BLOCKTABLE, 9},
                                 {22, IDS_BLOCKTABLE, 9},
                                 {26, IDS_BLOCKTABLE, 9},
                                 {29, IDS_BLOCKTABLE, 9},
                                 {30, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(
                L"> | A | B |\n> | - | - |\n> | x | y |\n",
                {{0, IDS_BLOCKTABLE, 11}, {4, IDS_BLOCKTABLE, 11}, {12, IDS_BLOCKTABLE, 11}, {28, IDS_BLOCKTABLE, 11}, {36, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"***", {{0, IDS_BLOCKDIVIDER, 3}, {3, IDS_BLOCKDIVIDER, 3}});
            checkBlockPositions(L"<div>raw</div>", {{0, IDS_BLOCKHTML, 14}, {5, IDS_BLOCKHTML, 14}, {14, IDS_BLOCKHTML, 14}});
            checkBlockPositions(
                L"[ref]: /target\n",
                {{0, IDS_BLOCKLINKDEFINITION, 14}, {7, IDS_BLOCKLINKDEFINITION, 14}, {14, IDS_BLOCKLINKDEFINITION, 14}, {15, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"---\ntitle: draft\n---\n",
                                {{0, IDS_BLOCKMETADATA, 3}, {4, IDS_BLOCKMETADATA, 12}, {17, IDS_BLOCKMETADATA, 3}, {21, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"[^note]: source\n\nUse [^note].\n",
                                {{0, IDS_BLOCKFOOTNOTE, 15}, {9, IDS_BLOCKFOOTNOTE, 15}, {17, IDS_BLOCKPARAGRAPH, 12}, {30, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"> [ref]: /target\n> words\n",
                                {{0, IDS_BLOCKLINKDEFINITION, 16}, {2, IDS_BLOCKLINKDEFINITION, 16}, {19, IDS_BLOCKQUOTE, 7}, {25, IDS_BLOCKPARAGRAPH, 0}},
                                true);
            checkBlockPositions(L"", {{0, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"# End", {{0, IDS_BLOCKHEADING, 5}, {5, IDS_BLOCKHEADING, 5}});
            checkBlockPositions(L"# End\n", {{5, IDS_BLOCKHEADING, 5}, {6, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"# End\r\n", {{5, IDS_BLOCKHEADING, 5}, {7, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"# End\r", {{5, IDS_BLOCKHEADING, 5}, {6, IDS_BLOCKPARAGRAPH, 0}});
            checkBlockPositions(L"```\nx\n```", {{4, IDS_BLOCKCODE, 1}, {9, IDS_BLOCKCODE, 3}});

            app.SetDocument(L"> # Head\n");
            app.Document().SetSelection({4, 4});
            Require(! app.Document().GetMarkdownSyntax(), "the replaced source genuinely awaits its new canonical syntax");
            expectBlock(IDS_BLOCKPARAGRAPH, 8, "pending syntax exposes no stale heading or quote category");
            const auto pendingConstructions = app.Activity().currentBlockLabelConstructions;
            app.Prepare(512);
            expectBlock(IDS_BLOCKHEADING, 8, "ready nested syntax replaces the pending paragraph category");
            Require(app.Activity().currentBlockLabelConstructions == pendingConstructions + 1 && app.Document().GetText() == L"> # Head\n" &&
                        app.Document().GetSelection() == DxUi::RichText::Selection{4, 4},
                    "syntax readiness constructs the new nested category once without moving the source selection");

            app.SetDocument(L"> plain\n");
            app.Document().SetSelection({4, 4});
            app.Prepare(512);
            expectBlock(IDS_BLOCKQUOTE, 7, "ordinary quoted prose retains its recognized quote ancestor");
            const auto nestedChanges = app.Activity().changes;
            const auto nestedUndo = app.Document().CanUndo(), nestedRedo = app.Document().CanRedo();
            DxUi::TextInputState nestedEntry;
            nestedEntry.text                         = L"> plain\n";
            nestedEntry.multiline                    = true;
            nestedEntry.caretIndex                   = 4;
            auto nestedPreview                       = nestedEntry;
            nestedPreview.text                       = L"> - one\n";
            const auto beforeNestedListConstructions = app.Activity().currentBlockLabelConstructions;
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), nestedPreview, false),
                    "preview a nested list item at the same ACP and physical-line length");
            expectBlock(IDS_BLOCKPARAGRAPH, 7, "the pending nested-list preview publishes its visible paragraph facts");
            Require(app.Activity().currentBlockLabelConstructions == beforeNestedListConstructions + 1,
                    "invalidating the ready quote publishes the different visible pending kind once");
            const auto pendingNestedListConstructions = app.Activity().currentBlockLabelConstructions;
            app.Prepare(512);
            Require(app._currentBlockLatestFacts && app._currentBlockLatestFacts.value().resourceId == IDS_BLOCKLIST &&
                        app._currentBlockLatestFacts.value().lineCharacterCount == 7,
                    "ready nested-list syntax retains its literal source kind and physical-line count");
            Require(! app._currentBlock->IsVisible() && app._listSummary->IsVisible() && ! app._listSummary->GetText().empty(),
                    "the visible nested-list summary replaces its hidden current-block label");
            Require(app.Activity().currentBlockLabelConstructions == pendingNestedListConstructions,
                    "ready nested-list syntax does not construct the now-hidden current-block label");
            const auto nestedConstructions = app.Activity().currentBlockLabelConstructions;
            nestedPreview.text             = L"> # one\n";
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), nestedPreview, false),
                    "replace the nested preview with an equal-length heading at the same caret");
            app.Prepare(512);
            expectBlock(IDS_BLOCKHEADING, 7, "nested heading syntax invalidates the list-item publication cache");
            Require(app.Activity().currentBlockLabelConstructions > nestedConstructions,
                    "different ready nested kinds with equal counts construct a fresh localized label");
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), nestedEntry, false),
                    "cancel the nested syntax preview back to the entry quote");
            app.Prepare(512);
            expectBlock(IDS_BLOCKQUOTE, 7, "nested preview cancellation restores the quote category");
            Require(app.Document().GetText() == nestedEntry.text && app.Document().GetSelection() == DxUi::RichText::Selection{4, 4} &&
                        app.Activity().changes == nestedChanges && app.Document().CanUndo() == nestedUndo && app.Document().CanRedo() == nestedRedo,
                    "nested-kind derivation and cache invalidation preserve exact noncommitting preview history");

            app.SetDocument(L"```text\ncode\n```\n");
            app.Document().SetSelection({8, 8});
            app.Prepare(512);
            expectBlock(IDS_BLOCKCODE, 4, "the prepared fenced source begins as a code block");
            const auto regionSource    = app.Document().Snapshot();
            const auto regionSelection = app.Document().GetSelection();
            const auto regionUndo = app.Document().CanUndo(), regionRedo = app.Document().CanRedo();
            app.Execute(Command::LocalSource);
            Require(app.Document().GetSemanticSourceRegion().has_value() && ! app.HasBlockDraft(),
                    "F2 enters the canonical in-document source region rather than an app-local parser or draft");
            app.Prepare(512);
            expectBlock(IDS_BLOCKSOURCE, 4, "the reachable SourceEditor node has its localized source-block category");
            app.Execute(Command::LocalSource);
            app.Prepare(512);
            expectBlock(IDS_BLOCKCODE, 4, "leaving the source region restores the actual fenced-code category");
            Require(! app.Document().GetSemanticSourceRegion() && app.Document().Snapshot().IsSameRevision(regionSource) &&
                        app.Document().GetSelection() == regionSelection && app.Document().CanUndo() == regionUndo && app.Document().CanRedo() == regionRedo &&
                        app.Host().GetFocusControl() == &app.Document(),
                    "source-editor entry/exit changes presentation only and preserves exact source, selection, history and focus");
        });
        test("Editor.OverlaysMutuallyExcludeRecoveryAndDeferPaintPlacement",
             [&]
        {
            auto options = Options(root, L"overlays-recovery-paint");
            WriteFixture(options.dataDirectory / L"Recovery" / std::format(L"session-{:032x}.rtd", 1),
                         SerializeRecovery(RecoverySnapshot{L"Recovered draft text\n", {}, false, {0, 0}, 1}));
            Testing::ForegroundOwnershipObserver foreground;
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden overlay-exclusivity fixture");
            Await(app, [&] { return app._recovery.Ready() && app.RecoveryCount() == 1; });

            app.Execute(Command::Recovery);
            Require(app._recoveryPanel->IsVisible() && app._activeOverlay == Application::OverlayKind::RecoveryModal && ! app._findBar->IsVisible() &&
                        ! app._drawer->IsVisible() && ! app._floating->IsVisible() && ! app._blockDraft->IsVisible(),
                    "Command::Recovery activates RecoveryModal exclusively");

            app.Execute(Command::Find);
            Require(app._findBar->IsVisible() && ! app._recoveryPanel->IsVisible() && app._activeOverlay == Application::OverlayKind::FindBar &&
                        app.RecoveryCount() == 1,
                    "Command::Find replaces RecoveryModal while retaining recovery candidates");

            app.Execute(Command::Recovery);
            Require(app._recoveryPanel->IsVisible() && ! app._findBar->IsVisible() && app._activeOverlay == Application::OverlayKind::RecoveryModal,
                    "reopening RecoveryModal dismisses FindBar");

            app.Execute(Command::Settings);
            Require(app._drawer->IsVisible() && ! app._recoveryPanel->IsVisible() && app._activeOverlay == Application::OverlayKind::SettingsDrawer &&
                        app.RecoveryCount() == 1,
                    "Command::Settings replaces RecoveryModal while retaining recovery candidates");

            app.Execute(Command::Recovery);
            Require(app._recoveryPanel->IsVisible() && ! app._drawer->IsVisible() && app._activeOverlay == Application::OverlayKind::RecoveryModal,
                    "reopening RecoveryModal dismisses SettingsDrawer");

            Require(app.DismissSurface() && app._activeOverlay == Application::OverlayKind::None && ! app._recoveryPanel->IsVisible() &&
                        app.Host().GetFocusControl() == &app.Document(),
                    "DismissSurface closes RecoveryModal and restores focus to the editor");

            app.SetDocument(L"# Paint deferral\n\nSelect careful words.\n");
            app.Prepare(512);
            const auto at = app.Document().GetText().find(L"careful");
            app.Document().SetSelection({at, at + 7});
            app.Prepare(512);
            app.CloseDrawers();
            Require(app._activeOverlay == Application::OverlayKind::None && ! app._floating->IsVisible() && ! app._pendingSelectionTools,
                    "CloseDrawers clears selection tools before WM_PAINT deferral check");

            app._pendingSelectionTools    = true;
            const auto layoutsBeforePaint = app.Activity().layouts;
            (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
            Require(app._postPreparePosted && app._pendingSelectionTools && ! app._floating->IsVisible() &&
                        app._activeOverlay == Application::OverlayKind::None && app.Activity().layouts == layoutsBeforePaint,
                    "WM_PAINT posts PostPreparePlacement without mutating overlay visibility or layout synchronously");

            const auto pumpWindowMessages = [&]
            {
                MSG message{};
                unsigned processed{};
                while (processed++ < 64 && PeekMessageW(&message, app.Window(), 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            };
            pumpWindowMessages();
            Require(! app._postPreparePosted && ! app._pendingSelectionTools && app._floating->IsVisible() &&
                        app._activeOverlay == Application::OverlayKind::FloatingSelectionTools,
                    "pumping PostPreparePlacement clears the flag and reconciles floating selection tools");

            app.Execute(Command::LocalSource);
            app.Prepare(512);
            Require(app.HasBlockDraft() && app._blockDraftSession.has_value() && ! app._blockDraftSession.value().pendingPlacement,
                    "LocalSource settles initial block draft placement");
            app._blockDraftSession.value().pendingPlacement = true;
            const auto layoutsBeforeDraftPaint              = app.Activity().layouts;
            (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
            Require(app._postPreparePosted && app._blockDraftSession.value().pendingPlacement && app.Activity().layouts == layoutsBeforeDraftPaint,
                    "WM_PAINT defers pending block draft layout to PostPreparePlacement");
            pumpWindowMessages();
            Require(! app._postPreparePosted && app._blockDraftSession.has_value() && ! app._blockDraftSession.value().pendingPlacement &&
                        app.Activity().layouts > layoutsBeforeDraftPaint,
                    "PostPreparePlacement completes deferred block draft placement");
            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()),
                    "overlay exclusivity and paint deferral checks preserve desktop ownership");
        });
        test("Editor.WorkspaceSwitchAtomicallySwapsMetadataUnderReentrantClose",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            const auto directory = root / L"workspace-reentrant-swap";
            const auto first     = directory / L"Alpha.md";
            const auto second    = directory / L"Beta.md";
            WriteFixture(first, "# Alpha\n\n## Alpha child\n\nBody.\n");
            WriteFixture(second, "# Beta\n\n## Beta child\n\nBody.\n");

            Application app(Options(root, L"workspace-reentrant-swap-ui"));
            Success(app.Initialize(), "initialize hidden workspace reentrant-swap app");
            StoredDocument loadedAlpha;
            Success(LoadDocument(first, loadedAlpha), "read Alpha fixture");
            app.SetDocument(loadedAlpha.text, first, loadedAlpha.bom, loadedAlpha.stamp);
            app.Prepare(128);

            app._outlineModel.OnTreeToggleExpanded(1, false);
            Require(app._session.collapsedHeadings == std::set<std::uint64_t>{1}, "Alpha begins with its first outline heading collapsed");

            DxUi::TextInputState entry;
            entry.text       = loadedAlpha.text;
            entry.multiline  = true;
            entry.caretIndex = loadedAlpha.text.size();
            auto transient   = entry;
            transient.text.replace(0, 7, L"Plain  ");
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), transient, false), "admit transient IME outline preview");
            Require(app._outlinePreviewBaseline.has_value() && app._outlinePreviewBaseline->collapsedHeadings == std::set<std::uint64_t>{1} &&
                        app._session.collapsedHeadings.empty(),
                    "transient outline preview preserves committed collapsed headings in outlinePreviewBaseline");
            Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), entry, false), "cancel transient IME outline preview");
            Require(! app._outlinePreviewBaseline.has_value() && app._session.collapsedHeadings == std::set<std::uint64_t>{1},
                    "cancelling transient IME preview restores committed session.collapsedHeadings");

            app.Execute(Command::DocumentImages);
            Require(app._session.remoteImagesAllowed && ! app.Preferences().allowRemoteImages, "Alpha holds its explicit document-level remote image grant");
            const auto alphaEnd = app.Document().Snapshot().Length();
            app.Document().SetSelection({alphaEnd, alphaEnd});
            Require(app.Document().ReplaceSelection(L"Alpha edited\n"), "edit Alpha before opening Beta");
            const auto alphaText      = app.Document().GetText();
            const auto alphaSavedText = app._session.savedText;
            const auto alphaKey       = app._session.key;
            const auto alphaCollapsed = app._session.collapsedHeadings;

            app.OpenPath(second);
            Await(app, [&] { return app._session.path == second && ! app._files.Busy(); });
            const auto betaEnd = app.Document().Snapshot().Length();
            app.Document().SetSelection({betaEnd, betaEnd});
            Require(app.Document().ReplaceSelection(L"Beta edited\n"), "edit Beta before switching back under reentrant close");
            const auto betaText      = app.Document().GetText();
            const auto betaSavedText = app._session.savedText;
            const auto betaKey       = app._session.key;
            const auto betaCollapsed = app._session.collapsedHeadings;
            Require(! app._session.remoteImagesAllowed && app._inactiveDocuments.size() == 1 && app._inactiveDocuments[0].metadata.path == first &&
                        app._inactiveDocuments[0].metadata.key == alphaKey && app._inactiveDocuments[0].metadata.savedText == alphaSavedText &&
                        app._inactiveDocuments[0].metadata.collapsedHeadings == alphaCollapsed && app._inactiveDocuments[0].metadata.remoteImagesAllowed &&
                        app._inactiveDocuments[0].state.Text() == alphaText,
                    "inactive workspace slot holds Alpha metadata and state prior to reentrant switch");

            bool retired{};
            auto* const oldRoot        = app._root;
            const auto oldRootLifetime = GetControlLifetimeToken(*oldRoot);
            app._editor->SetOnSelectionChanged([&]
            {
                if (retired)
                    return;
                retired = true;
                app.Host().SetRoot(std::make_unique<Surface>());
            });

            Require(app.SwitchWorkspaceDocument(0), "workspace switch succeeds after committing exchange before reentrant retirement");
            Require(retired && oldRootLifetime.expired() && app.Host().GetRoot() != oldRoot,
                    "reentrant callback retired the owner root during ExchangeDocumentState");
            Require(app._session.path == first && app._session.key == alphaKey && app._session.savedText == alphaSavedText &&
                        app._session.collapsedHeadings == alphaCollapsed && app._session.remoteImagesAllowed,
                    "active session metadata swaps atomically with Document state despite reentrant owner retirement");
            Require(app._inactiveDocuments.size() == 1 && app._inactiveDocuments[0].metadata.path == second &&
                        app._inactiveDocuments[0].metadata.key == betaKey && app._inactiveDocuments[0].metadata.savedText == betaSavedText &&
                        app._inactiveDocuments[0].metadata.collapsedHeadings == betaCollapsed && ! app._inactiveDocuments[0].metadata.remoteImagesAllowed &&
                        app._inactiveDocuments[0].dirty && app._inactiveDocuments[0].state.Text() == betaText,
                    "inactive workspace slot retains synchronized Beta metadata and state");
            Require(! app._outlinePreviewBaseline.has_value() && ! app._derivedCaches.documentAnalysisSource.has_value() &&
                        foreground.NeverAcquiredForeground(),
                    "derived caches and preview baseline reset cleanly without taking desktop foreground");
        });
        test("Editor.CapacityLimitReportsLocalizedErrorAcrossCommands",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"capacity-limit-commands"));
            Success(app.Initialize(), "initialize hidden capacity-limit app");
            const auto expectedLimit = FormatStringResource(nullptr, IDS_FILETOOLARGELIMIT, DxUi::RichText::Document::MaximumUnits);

            app.SetView(EditorView::Source);
            Require(app.SetDocument(std::wstring(DxUi::RichText::Document::MaximumUnits - 1u, L'a')), "seed near-capacity document for Source formatting");
            app.Document().SetSelection({0u, 2u});
            const auto formatSnapshot = app.Document().Snapshot();
            app._feedback->SetText(L"");
            app.Execute(Command::Bold);
            Require(app.Document().Snapshot().IsSameRevision(formatSnapshot) && ! app.Dirty() && app._feedback->GetText() == expectedLimit,
                    "Source ToggleFormatting capacity refusal preserves document and reports localized limit");

            Require(app.SetDocument(std::wstring(DxUi::RichText::Document::MaximumUnits - 10u, L'b')), "seed near-capacity document for template insertion");
            for (const auto command : {Command::Mermaid, Command::Math})
            {
                app.Document().SetSelection({0u, 0u});
                const auto insertSnapshot = app.Document().Snapshot();
                app._feedback->SetText(L"");
                app.Execute(command);
                Require(app.Document().Snapshot().IsSameRevision(insertSnapshot) && ! app.Dirty() && app._feedback->GetText() == expectedLimit,
                        "Mermaid and Math capacity refusal preserves document and reports localized limit");
            }

            app.Document().SetSelection({0u, 4u});
            const auto draftSnapshot = app.Document().Snapshot();
            app.Execute(Command::LocalSource);
            Require(app.HasBlockDraft(), "open block source draft near capacity");
            app.BlockDraft()->SetText(std::wstring(20u, L'c'));
            app._feedback->SetText(L"");
            app.Execute(Command::ApplyBlock);
            Require(app.HasBlockDraft() && app.Document().Snapshot().IsSameRevision(draftSnapshot) && ! app.Dirty() &&
                        app._feedback->GetText() == expectedLimit,
                    "ApplyBlockSource capacity refusal retains draft, preserves source and reports localized limit");
            app.Execute(Command::CloseDrawer);

            app.SetView(EditorView::Compose);
            Require(app.SetDocument(std::wstring(DxUi::RichText::Document::MaximumUnits, L'd')), "seed exact-capacity document for semantic Markdown command");
            app.Prepare();
            for (const auto command : {Command::H1, Command::Bullets})
            {
                app.Document().SetSelection({0u, 2u});
                const auto semanticSnapshot = app.Document().Snapshot();
                app._feedback->SetText(L"");
                app.Execute(command);
                Require(app.Document().Snapshot().IsSameRevision(semanticSnapshot) && ! app.Dirty() && app._feedback->GetText() == expectedLimit &&
                            foreground.NeverAcquiredForeground(),
                        "ExecuteMarkdownCommand capacity refusal preserves source and reports localized limit");
            }
        });
        test("Editor.CloseDocumentProtectsDirtyAndSwitchesWorkspace",
             [&]
        {
            const auto directory = root / L"close-document-workspace";
            const auto first = directory / L"Alpha.md", second = directory / L"Beta.md", third = directory / L"Gamma.md";
            WriteFixture(first, "# Alpha\n");
            WriteFixture(second, "# Beta\n");
            WriteFixture(third, "# Gamma\n");
            Testing::ForegroundOwnershipObserver foreground;
            unsigned prompts{};
            int promptReply              = IDCANCEL;
            auto options                 = Options(root, L"close-document-ui");
            options.unsavedPromptForTest = [&]
            {
                ++prompts;
                return promptReply;
            };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden close-document workspace");
            StoredDocument loaded;
            Success(LoadDocument(first, loaded), "read Alpha fixture");
            app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
            app.OpenPath(second);
            Await(app, [&] { return app._session.path == second && ! app._files.Busy(); });
            app.OpenPath(third);
            Await(app, [&] { return app._session.path == third && ! app._files.Busy(); });
            Require(app._inactiveDocuments.size() == 2 && app._session.path == third, "three documents open in workspace");

            const auto betaIndex =
                static_cast<std::size_t>(std::find_if(app._inactiveDocuments.begin(), app._inactiveDocuments.end(), [&](const auto& document) {
                return document.metadata.path == second;
            }) - app._inactiveDocuments.begin());
            Require(betaIndex < app._inactiveDocuments.size() && app.SwitchWorkspaceDocument(betaIndex), "switch to Beta");
            app.Document().SetSelection({7u, 7u});
            Require(app.Document().ReplaceSelection(L"Edited\n"), "make Beta dirty");

            app.Execute(Command::CloseDocument);
            Require(prompts == 1 && app._session.path == second && app.Dirty() && app._inactiveDocuments.size() == 2,
                    "Cancel on CloseDocument keeps the active dirty document and all workspace slots");

            promptReply = IDNO;
            app.Execute(Command::CloseDocument);
            Require(prompts == 2 && app._session.path == third && ! app.Dirty() && app._inactiveDocuments.size() == 1 &&
                        app._inactiveDocuments[0].metadata.path == first,
                    "discarding on CloseDocument closes Beta and switches to adjacent Gamma");
            StoredDocument diskBeta;
            Success(LoadDocument(second, diskBeta), "read Beta on disk after discard");
            Require(diskBeta.text == L"# Beta\n", "discarded CloseDocument never overwrites Beta on disk");

            {
                KeyboardState::ScopedSynthetic keyboard;
                std::array<BYTE, 256> keys{};
                keys[VK_CONTROL] = 0x80;
                KeyboardState::Write(keys);
                const MSG shortcut{app.Window(), WM_KEYDOWN, 'W', 0};
                Require(app.RouteShortcut(shortcut), "Ctrl+W routes Command::CloseDocument");
            }
            Require(prompts == 2 && app._session.path == first && app._inactiveDocuments.empty() && ! app.Dirty(),
                    "closing clean Gamma via Ctrl+W switches to final remaining Alpha");

            app.Document().SetSelection({8u, 8u});
            Require(app.Document().ReplaceSelection(L"Final edit\n"), "make Alpha dirty");
            app.Execute(Command::CloseDocument);
            Require(prompts == 3 && app._session.path.empty() && app.Document().GetText().empty() && ! app.Dirty() && app._inactiveDocuments.empty() &&
                        foreground.NeverAcquiredForeground(),
                    "closing the last dirty document protects edits and resets to a clean untitled document");
            StoredDocument diskAlpha;
            Success(LoadDocument(first, diskAlpha), "read Alpha on disk after final close");
            Require(diskAlpha.text == L"# Alpha\n", "final CloseDocument discard never overwrites Alpha on disk");
        });
        test("Editor.DragDropOpensMarkdownAndRejectsInvalidDrops",
             [&]
        {
            const auto directory = root / L"drag-drop-files";
            const auto current = directory / L"Current.md", dropped = directory / L"Dropped.md", notes = directory / L"Notes.txt",
                       picture = directory / L"Picture.png", missing = directory / L"Missing.md", folder = directory / L"Folder";
            WriteFixture(current, "# Current\n");
            WriteFixture(dropped, "# Dropped\nValid markdown body.\n");
            WriteFixture(notes, "Plain UTF-8 notes.\n");
            WriteFixture(picture, "not-actually-markdown");
            std::filesystem::create_directories(folder);

            Testing::ForegroundOwnershipObserver foreground;
            unsigned prompts{};
            int promptReply              = IDCANCEL;
            auto options                 = Options(root, L"drag-drop-ui");
            options.unsavedPromptForTest = [&]
            {
                ++prompts;
                return promptReply;
            };
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden drag-drop app");
            StoredDocument loaded;
            Success(LoadDocument(current, loaded), "read Current fixture");
            app.SetDocument(loaded.text, current, loaded.bom, loaded.stamp);
            app.Document().SetSelection({10u, 10u});
            Require(app.Document().ReplaceSelection(L"Unsaved draft\n"), "make Current dirty");
            const auto dirtySnapshot = app.Document().Snapshot();

            for (const auto& invalid : {folder, picture, missing})
            {
                app._feedback->SetText(L"");
                Require(! app.OpenDroppedFile(invalid) && prompts == 0 && app._session.path == current &&
                            app.Document().Snapshot().IsSameRevision(dirtySnapshot) && app.Dirty() && app._feedback->GetText() == Resource(IDS_DROPUNSUPPORTED),
                        "invalid directory, binary extension or missing drop refuses without prompting or mutating document");
            }

            Require(app.OpenDroppedFile(dropped) && prompts == 1 && app._session.path == current && app.Document().Snapshot().IsSameRevision(dirtySnapshot) &&
                        app.Dirty() && ! app._files.Busy(),
                    "valid drop checks unsaved changes and Cancel preserves dirty document");

            promptReply = IDNO;
            Require(app.OpenDroppedFile(dropped) && prompts == 2, "discarding dirty document admits valid Markdown drop");
            Await(app, [&] { return app._session.path == dropped && ! app._files.Busy(); });
            Require(app.Document().GetText() == L"# Dropped\nValid markdown body.\n" && ! app.Dirty(), "dropped Markdown file loads cleanly");

            Require(app.OpenDroppedFile(notes), "valid UTF-8 .txt drop is admitted");
            Await(app, [&] { return app._session.path == notes && ! app._files.Busy(); });
            Require(app.Document().GetText() == L"Plain UTF-8 notes.\n" && ! app.Dirty() && foreground.NeverAcquiredForeground(),
                    "dropped .txt file loads cleanly without desktop activation");
        });
        test("Editor.FocusModeDoesNotRewriteSettings",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            auto options             = Options(root, L"focus-no-settings-rewrite");
            options.preferencesProbe = true;
            const auto settingsPath  = SettingsPath(options.dataDirectory);
            Application app(std::move(options));
            Success(app.Initialize(), "initialize hidden focus-settings app");
            app.Execute(Command::ThemeDark);
            Await(app, [&] { return ! app._preferenceWriter.Busy(); });
            Require(SUCCEEDED(app._preferenceWriter.Status()), "initial settings write succeeds");

            std::string beforeBytes;
            FileStamp beforeStamp;
            Success(ReadBytes(settingsPath, beforeBytes, &beforeStamp), "read baseline RedTilde.settings");

            app.Execute(Command::Focus);
            Require(app.Preferences().focusMode && ! app._preferenceWriter.Busy(), "entering Focus mode does not queue SavePreferences");
            app.Prepare(64);
            app.Execute(Command::Focus);
            Require(! app.Preferences().focusMode && ! app._preferenceWriter.Busy(), "exiting Focus mode does not queue SavePreferences");
            app.Prepare(64);

            std::string afterBytes;
            FileStamp afterStamp;
            Success(ReadBytes(settingsPath, afterBytes, &afterStamp), "read RedTilde.settings after Focus toggles");
            Require(beforeBytes == afterBytes && beforeStamp == afterStamp && foreground.NeverAcquiredForeground(),
                    "Focus mode toggles never rewrite RedTilde.settings on disk");
        });

        test("Editor.M7ThemeContrastDeviceLossAndLinkSanitization",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            // P0-17: High Contrast populates complete GetSysColor palette and guards subduedText chrome.
            {
                auto hcOptions     = Options(root, L"hc-m7");
                hcOptions.scenario = L"high-contrast";
                Application hcApp(hcOptions);
                Success(hcApp.Initialize(), "initialize high-contrast M7 fixture");
                Resize(hcApp, 1280, 800);
                const auto& theme = hcApp.Host().GetTheme();
                Require(theme.highContrast, "high-contrast scenario sets theme.highContrast");
                Require(theme.overlayBackground.r == theme.windowBackground.r && theme.overlayBackground.g == theme.windowBackground.g &&
                            theme.overlayBackground.b == theme.windowBackground.b,
                        "HC overlayBackground tracks COLOR_WINDOW");
                Require(theme.primaryButtonFill.has_value() &&
                            (theme.primaryButtonFill.value().r != D2D1::ColorF(0xC83C46).r || theme.primaryButtonFill.value().g != D2D1::ColorF(0xC83C46).g),
                        "HC primaryButtonFill does not hardcode non-HC crimson");
                Require(theme.subduedText.r == theme.text.r && theme.subduedText.g == theme.text.g && theme.subduedText.b == theme.text.b,
                        "HC subduedText matches window text for full contrast");
                hcApp.SetView(EditorView::Split);
                hcApp.Layout();
                Require(hcApp._splitSwitchHint != nullptr, "split chrome labels update cleanly in high contrast");
            }

            // P0-17 Paper overlayBackground + P1-04 device loss dashed stroke + P1-20 scroll restore target + P2-06 links.
            {
                Application app(Options(root, L"m7-theme-deviceloss-links"));
                Success(app.Initialize(), "initialize M7 theme/device-loss/link fixture");
                Resize(app, 1280, 800);
                app._settings.appearance = Appearance::Paper;
                app.ApplyTheme();
                Color(app.Host().GetTheme().overlayBackground, 0xF3EDDF, "Paper overlayBackground uses Paper surfaceBackground");

                // P1-04: Device loss advances generation and recreates split seam _dashedStroke cleanly.
                app._settings.splitScrollSync = false;
                app.SetDocument(L"# Split Seam\n\nParagraph content.\n");
                app.SetView(EditorView::Split);
                app.Prepare(128);
                const auto genBefore = app.Host().GetResourceGeneration();
                app.Host().DebugSimulateDeviceLoss();
                app.Prepare(128);
                Require(app.Host().GetResourceGeneration() > genBefore && app.Capture(root / L"m7-deviceloss-split.png"),
                        "split view survives device loss and repaints dashed seam cleanly");
                std::error_code ec;
                std::filesystem::remove(root / L"m7-deviceloss-split.png", ec);

                // P1-20 & PERF-08: SetView skips SavePreferences when unchanged; SynchronizeMirror sets _pendingSourceScrollRestore for _mirror.
                app.SetView(EditorView::Source);
                const auto beforeSavePrefs = app._settings.view;
                Require(beforeSavePrefs == EditorView::Source && ! app._preferenceWriter.Busy(), "unchanged/settled view check");
                app.SetView(EditorView::Source); // no-op
                Require(! app._preferenceWriter.Busy(), "SetView to already active view skips redundant SavePreferences");

                app.SetView(EditorView::Split);
                app._settings.splitScrollSync = false;
                app.Document().SetText(L"# Sync\n\nLine 1\n\nLine 2\n");
                app._pendingSourceScrollRestore.reset();
                app._pendingComposeScrollRestore.reset();
                Require(app.SynchronizeMirror(), "SynchronizeMirror succeeds in Split view");
                Require(! app._pendingComposeScrollRestore.has_value(), "mirror sync never writes _pendingComposeScrollRestore when target is _mirror");

                // P2-06: Link validation and UI string sanitization.
                Require(Application::SupportedExternalLink(L"https://example.com/path?x=1#y"), "accepts clean HTTPS link");
                Require(Application::SupportedExternalLink(L"mailto:user@example.com"), "accepts clean mailto link");
                Require(! Application::SupportedExternalLink(L"https://evil.example\\share/doc"), "rejects backslash in http authority");
                Require(! Application::SupportedExternalLink(L"mailto:user\\domain@example.com"), "rejects backslash in mailto destination");
                Require(! Application::SupportedExternalLink(L"https://example.com/\x202E"), "rejects BiDi override U+202E");
                Require(! Application::SupportedExternalLink(L"https://example.com/\x200B"), "rejects zero-width U+200B");
                Require(! Application::SupportedExternalLink(L"https://example.com/\x0007"), "rejects C0 control");
                Require(! Application::SupportedExternalLink(std::wstring(2049u, L'a')), "rejects URL > 2048 units");
                Require(! Application::SupportedExternalLink(L"https://example.com/\xD800"), "rejects unpaired UTF-16 surrogate");
                Require(! Application::SupportedExternalLink(L"https:///empty-host"), "rejects empty HTTP authority");
                Require(! Application::SupportedExternalLink(L"https://example.com/\x061C"), "rejects Arabic Letter Mark U+061C");
                Require(! Application::SupportedExternalLink(L"mailto:?subject=x"), "rejects mailto link with empty recipient before query");
                Require(! Application::SupportedExternalLink(L"mailto:#frag"), "rejects mailto link with empty recipient before fragment");
                Require(! Application::SupportedExternalLink(L"mailto:"), "rejects bare mailto scheme without recipient");
                Require(Application::SupportedExternalLink(L"mailto:user@example.com?subject=hi"), "accepts mailto link with recipient and query");

                Require(IsUnsafeDisplayCodePoint(0x061Cu), "IsUnsafeDisplayCodePoint rejects Arabic Letter Mark U+061C");
                const auto sanitized = SanitizeUiText(L"Title\x202E\x0007\x200B\x061CName", 16u);
                Require(sanitized == L"TitleName", "SanitizeUiText removes BiDi/ALM/C0/zero-width characters cleanly");
                Require(SanitizeUiText(L"A\U0001F642B", 2u) == L"A",
                        "SanitizeUiText stops truncation before an overflowing safe surrogate pair instead of stitching trailing chars");
            }
            Require(foreground.NeverAcquiredForeground(), "M7 theme/device-loss/link tests stay hidden");
        });

        test("Editor.M7SelectionLookupAndIdleLayoutGates",
             [&]
        {
            Testing::ForegroundOwnershipObserver foreground;
            Application app(Options(root, L"m7-perf-selection-layout"));
            Success(app.Initialize(), "initialize M7 perf selection/layout fixture");
            Resize(app, 1280, 800);

            // PERF-08: SyntaxLookupCache + binary-search headings/inlines/math + idle WM_PAINT ServiceImages bypass.
            std::wstring doc;
            for (int i = 0; i < 60; ++i)
                doc += std::format(L"# Heading {}\n\nParagraph with **bold** *italic* `code` [link](https://example.com).\n\n", i);
            doc += L"$$\nE = mc^2\n$$\n\n- item 1\n- item 2\n";
            app.SetDocument(doc);
            app.Prepare(256);

            Require(app._derivedCaches.syntaxCache.syntax != nullptr && app._derivedCaches.syntaxCache.topLevelBlocks.size() >= 100u &&
                        ! app._derivedCaches.syntaxCache.fencedMathBlocks.empty(),
                    "SyntaxLookupCache populates top-level and fenced math block indices once per syntax revision");

            const auto boldPos = doc.find(L"bold") + 1;
            app.Document().SetSelection({boldPos, boldPos});
            app.SelectionChanged();
            Require(app._tools[3]->GetToggleState().value_or(false) && ! app._equationContext,
                    "caret inside **bold** detects Strong format via O(log B) tree walk");

            const auto mathPos = doc.find(L"mc^2");
            app.Document().SetSelection({mathPos, mathPos});
            app.SelectionChanged();
            Require(app._equationContext && app.EquationPreviewText().find(L"mc^2") != std::wstring::npos,
                    "caret inside fenced $$ block finds math via binary search on fencedMathBlocks");

            const auto headingPos = doc.find(L"# Heading 45") + 3;
            app.Document().SetSelection({headingPos, headingPos});
            app.SelectionChanged();
            Require(app._currentHeading.has_value() && app._currentHeading.value() == 45u,
                    "heading selection matches exact outline heading via std::upper_bound");

            // Idle WM_PAINT skips image watch mutex work when no images active/pending and view unchanged.
            (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
            Require(app._imageWatchWasRichVisible == true && ! app._images.HasActiveOrResults() && ! app._imageFiles.HasResult(),
                    "image watch settles rich-visible flag and idle gates report false");
            (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);

            // PERF-02: Pin label caching + hidden drawer layout skip keeps theme previews fresh.
            app._settings.showOutline = true;
            app.Layout();
            Require(app._lastPinOutlineDocked == std::optional<bool>{true}, "pin cache records docked state");
            app._settings.appearance = Appearance::Dark;
            app.ApplyTheme();
            app.Layout();
            Require(! app._drawer->IsVisible() && app._themeButtons[0]->GetThemePreview().has_value(),
                    "hidden drawer skips heavy layout while preserving live theme swatch preview");

            // Live-edit WM_PAINT image loading + consent without calling Prepare(), plus Source view switch and empty-doc settlement.
            {
                const auto imgDir = root / L"m7-live-wm-paint";
                std::filesystem::create_directories(imgDir);
                const auto bmpBytes = Testing::ImageBitmapFixture();
                Success(WriteBytesAtomic(imgDir / L"live.bmp", std::string_view(reinterpret_cast<const char*>(bmpBytes.data()), bmpBytes.size())),
                        "seed live.bmp");
                const std::wstring baseDoc = L"# Base\n\nText.\n";
                const auto docPath         = imgDir / L"doc.md";
                Success(WriteBytesAtomic(docPath, "# Base\n\nText.\n"), "seed doc.md");
                Require(app.SetDocument(baseDoc, docPath), "open base doc for live WM_PAINT image test");
                app.Prepare(256);
                (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                Require(app._imageWatchWasRichVisible && ! app._imageConsent->IsVisible() && app._watchedImagePaths.empty(),
                        "base doc settles before live image edit");

                const auto insertAt = app.Document().GetText().size();
                app.Document().SetSelection({insertAt, insertAt});
                Require(app.Document().ReplaceSelection(L"\n![Live local](live.bmp)\n![Live remote](https://example.com/a.png)\n"),
                        "insert live local and remote images without Prepare()");
                const auto done = [&]
                {
                    return app._imageConsent->IsVisible() && app.Document().GetImageResourceStatistics().readySources == 1u &&
                           app._watchedImagePaths.size() == 1u && app._imageWatchAcknowledgedSemantic != nullptr;
                };
                const auto deadline = GetTickCount64() + 5000ull;
                while (! done() && GetTickCount64() < deadline)
                {
                    MSG msg{};
                    while (PeekMessageW(&msg, app.Window(), 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&msg);
                        DispatchMessageW(&msg);
                    }
                    (void)app.Host().RenderInitialFrameForShow();
                    (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                    if (done())
                        break;
                    (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5u, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                }
                Require(done(), "live WM_PAINT loop loads local image, updates watch path, and shows remote consent without Prepare()");

                app.SetView(EditorView::Source);
                (void)app.Host().RenderInitialFrameForShow();
                (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                Require(app.SetDocument(L"# Clean\n", imgDir / L"clean.md"), "open clean doc in Source mode");
                (void)app.Host().RenderInitialFrameForShow();
                (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                Require(! app._imageConsent->IsVisible() && app._imageConsentPath == (imgDir / L"clean.md") &&
                            app._imageWatchDocumentPath == (imgDir / L"clean.md") && app._watchedImagePaths.empty() && ! app._imageWatchWasRichVisible &&
                            app._imageWatchAcknowledgedSemantic == nullptr,
                        "Source view switch and clean doc reset image consent and watched paths via WM_PAINT");

                Require(app.SetDocument(L"", imgDir / L"empty.md"), "open empty doc in Source mode");
                (void)app.Host().RenderInitialFrameForShow();
                (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                (void)app.Host().RenderInitialFrameForShow();
                (void)SendMessageW(app.Window(), WM_PAINT, 0, 0);
                Require(! app._imageWatchWasRichVisible && app._imageWatchAcknowledgedSemantic == nullptr &&
                            app._imageWatchDocumentPath == (imgDir / L"empty.md"),
                        "empty doc settles cleanly without image-watch churn");
            }

            Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "M7 perf gate test stays hidden");
        });
    }
};
void RegisterDesignRefreshTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    DesignRefreshProbe::Register(root, test);
    test("Editor.CapturesSettleIndependentSourceCosmetics",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        LaunchOptions launch;
        launch.headless         = true;
        launch.dataDirectory    = root / L"source-cosmetic-capture";
        launch.viewportWidthPx  = 1280;
        launch.viewportHeightPx = 960;
        Application app(launch);
        Success(app.Initialize(), "create hidden source-cosmetic capture fixture");
        std::wstring source;
        for (unsigned row = 0; row < 16384; ++row)
            source += L" \n";
        app.SetDocument(source);
        app.SetView(EditorView::Source);
        auto appearance                  = app.Document().GetAppearance();
        appearance.sourceLineNumbers     = true;
        appearance.sourceWhitespaceMarks = true;
        app.Document().SetAppearance(appearance);
        const auto snapshot = app.Document().Snapshot();
        Require(app.Capture(launch.dataDirectory / L"prepared-guides.png"), "capture prepared source through the owned harness");
        const auto state = app.Document().DebugGetState();
        Require(! app.Host().HasPendingContentPreparation() && ! state.markdownPreparationPending && ! state.sourceInkPreparationPending &&
                    ! state.codeInkPreparationPending && ! state.sourceGuidesPreparationPending,
                "a completed documentation capture includes independent source ink and guides after content becomes ready");
        Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetText() == source && ! app.Document().CanUndo(),
                "settling capture cosmetics never edits source or creates history");
        Require(foreground.NeverAcquiredForeground() && ! IsWindowVisible(app.Window()), "source-cosmetic capture preserves desktop ownership");
    });
}
} // namespace RedTilde
