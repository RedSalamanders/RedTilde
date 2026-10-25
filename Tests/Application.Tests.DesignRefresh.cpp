#include "Application.Tests.DesignRefresh.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.TextServices.h"
#include "res/Resource.h"
#include <UIAutomation.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <format>
#include <initializer_list>
#include <mutex>
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
    if (!condition)
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
    return inner.left >= outer.left - 0.1f && inner.top >= outer.top - 0.1f && inner.right <= outer.right + 0.1f &&
           inner.bottom <= outer.bottom + 0.1f && inner.right > inner.left && inner.bottom > inner.top;
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
void CheckBrandIconResource(HINSTANCE module, unsigned requestedSize)
{
    wil::unique_hicon icon{static_cast<HICON>(LoadImageW(module, MAKEINTRESOURCEW(IDI_REDTILDE), IMAGE_ICON, static_cast<int>(requestedSize),
                                                         static_cast<int>(requestedSize), LR_DEFAULTCOLOR))};
    Require(icon != nullptr, "load the compiled RedTilde icon resource at the requested size");

    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Require(SUCCEEDED(initialized) || initialized == RPC_E_CHANGED_MODE, "use the test's existing COM apartment for icon decoding");
    const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
    wil::com_ptr<IWICImagingFactory> factory;
    Success(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())),
            "create a WIC decoder for the icon resource");
    wil::com_ptr<IWICBitmap> bitmap;
    Success(factory->CreateBitmapFromHICON(icon.get(), bitmap.put()), "decode the requested icon frame");
    UINT width{}, height{};
    Success(bitmap->GetSize(&width, &height), "read decoded icon geometry");
    Require(width == requestedSize && height == requestedSize, "icon resource decodes to the requested square geometry");

    wil::com_ptr<IWICFormatConverter> converter;
    Success(factory->CreateFormatConverter(converter.put()), "create an alpha-preserving icon pixel view");
    Success(converter->Initialize(bitmap.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "convert icon pixels to explicit BGRA alpha");
    std::vector<BYTE> pixels(static_cast<std::size_t>(requestedSize) * requestedSize * 4);
    Success(converter->CopyPixels(nullptr, requestedSize * 4, static_cast<UINT>(pixels.size()), pixels.data()), "read decoded icon alpha pixels");
    std::size_t transparent{}, opaque{}, ink{};
    UINT left = requestedSize, top = requestedSize, right{}, bottom{};
    for (UINT y = 0; y < requestedSize; ++y)
    {
        for (UINT x = 0; x < requestedSize; ++x)
        {
            const auto alpha = pixels[(static_cast<std::size_t>(y) * requestedSize + x) * 4 + 3];
            transparent += alpha == 0;
            opaque += alpha == 255;
            if (alpha != 0)
            {
                ++ink;
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x + 1);
                bottom = std::max(bottom, y + 1);
            }
        }
    }
    Require(transparent > 0 && opaque > 0, "every icon size retains transparent background and opaque ink");
    if (requestedSize <= 32)
    {
        // Preserve the existing small-icon legibility oracle. Larger frames have
        // less antialias coverage, so their ink area is not the same percentage.
        const auto pixelsPerDimension = static_cast<std::size_t>(requestedSize);
        Require(ink > pixelsPerDimension * pixelsPerDimension / 4 && opaque < pixelsPerDimension * pixelsPerDimension &&
                    right - left >= requestedSize * 3 / 5 && bottom - top >= requestedSize * 2 / 5,
                std::format("icon {} silhouette: transparent={}, opaque={}, ink={}, width={}, height={}", requestedSize, transparent, opaque, ink,
                            right - left, bottom - top));
    }

    wil::com_ptr<IWICFormatConverter> premultiplied;
    Success(factory->CreateFormatConverter(premultiplied.put()), "create an independent premultiplied icon reference");
    Success(premultiplied->Initialize(bitmap.get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom),
            "prepare the independent WIC premultiplied reference for the native brand resource");
    Success(premultiplied->CopyPixels(nullptr, requestedSize * 4, static_cast<UINT>(pixels.size()), pixels.data()),
            "read the independent premultiplied icon reference");
    const auto nativePixels = LoadBrandMarkPixels(icon.get());
    Require(nativePixels.width == width && nativePixels.height == height && nativePixels.bgra == pixels,
            "bounded native brand extraction matches every WIC reference byte, including partial and zero alpha");
}
std::vector<BYTE> ReadBrandMarkRaster(const std::filesystem::path &path, float dpi)
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
            region.insert(region.end(), pixels.begin() + static_cast<std::ptrdiff_t>(offset),
                          pixels.begin() + static_cast<std::ptrdiff_t>(offset + 4));
            visible += !std::equal(pixels.begin() + static_cast<std::ptrdiff_t>(offset), pixels.begin() + static_cast<std::ptrdiff_t>(offset + 3),
                                   pixels.begin() + static_cast<std::ptrdiff_t>(backgroundOffset));
        }
    Require(visible > 16, "the brand-mark region contains rendered icon pixels");
    return region;
}
void WriteFixture(const std::filesystem::path &path, std::string_view contents = "# Invented notes\n")
{
    std::filesystem::create_directories(path.parent_path());
    Success(WriteBytesAtomic(path, contents), "seed design integration fixture");
}
void Await(Application &app, const std::function<bool()> &complete)
{
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!complete() && std::chrono::steady_clock::now() < end)
    {
        app.Prepare(32);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    Require(complete(), "hidden application did not settle the expected bounded worker result");
}
} // namespace

// This probe observes the application's real command/model boundaries. It does
// not replace production callbacks or synthesize desktop focus, input or menus.
class DesignRefreshProbe final
{
    static LaunchOptions Options(const std::filesystem::path &root, std::wstring_view name, std::wstring scenario = L"compose-light")
    {
        LaunchOptions options;
        options.headless = true;
        options.dataDirectory = root / name;
        options.scenario = std::move(scenario);
        return options;
    }
    static void Resize(Application &app, unsigned width, unsigned height, UINT dpi = 0, bool prepare = true)
    {
        if (!dpi)
            dpi = GetDpiForWindow(app.Window());
        RECT rect{};
        Require(GetWindowRect(app.Window(), &rect) != FALSE, "read hidden frame rectangle");
        rect.right = rect.left + MulDiv(static_cast<int>(width), static_cast<int>(dpi), 96);
        rect.bottom = rect.top + MulDiv(static_cast<int>(height), static_cast<int>(dpi), 96);
        // This is the same owner-window message/host path as an OS DPI change.
        // It exercises layout at 100/150/200%; it is not physical-monitor evidence.
        (void)SendMessageW(app.Window(), WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&rect));
        Require(SetWindowPos(app.Window(), nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
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
    static void CheckChrome(Application &app)
    {
        const auto client = app.Host().GetClientBoundsDip();
        for (auto *bar : {app.documentBar, app.commandBar})
        {
            if (!bar->IsVisible())
                continue;
            Require(Inside(bar->GetBounds(), client), "chrome region remains inside the client area");
            std::vector<DxUi::Control *> visible;
            for (std::size_t index = 0; index < bar->GetLogicalChildCount(); ++index)
            {
                auto *control = bar->GetLogicalChild(index);
                if (!control->IsVisible())
                    continue;
                Require(Inside(control->GetBounds(), bar->GetBounds()), "every visible chrome item has positive, in-row bounds");
                visible.push_back(control);
            }
            for (std::size_t first = 0; first < visible.size(); ++first)
                for (std::size_t second = first + 1; second < visible.size(); ++second)
                    Require(!Overlap(visible[first]->GetBounds(), visible[second]->GetBounds()), "visible chrome items never overlap");
        }
        Require(Inside(app.editor->GetBounds(), app.page->GetBounds()), "editor fits the writing surface");
        if (app.mirror->IsVisible())
        {
            Require(Inside(app.mirror->GetBounds(), app.page->GetBounds()), "read-only Split mirror fits the writing surface");
            Require(!Overlap(app.editor->GetBounds(), app.mirror->GetBounds()), "editable and derived Split text regions never overlap");
        }
    }
    static LRESULT Hit(Application &app, D2D1_POINT_2F point)
    {
        const auto screen = app.Host().DipPointToScreenPoint(point);
        return app.FrameHitTest(app.Window(), MAKELPARAM(static_cast<SHORT>(screen.x), static_cast<SHORT>(screen.y)));
    }
    static LRESULT HitControl(Application &app, const DxUi::Control *control)
    {
        const auto bounds = control->GetBounds();
        return Hit(app, {(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2});
    }
    static bool AccessibilityContainsName(Application &app, std::wstring_view expected)
    {
        wil::com_ptr_nothrow<IRawElementProviderFragmentRoot> root;
        root.attach(DxUi::CreateWindowHostAccessibilityProvider(app.Host().GetHwnd()));
        Require(root != nullptr, "the application exposes its current UI Automation root");
        wil::com_ptr_nothrow<IRawElementProviderFragment> rootFragment;
        Success(root.query_to(rootFragment.put()), "the UI Automation root supports fragment navigation");
        std::size_t visited{};
        const auto visit = [&](auto &&self, IRawElementProviderFragment *current, std::size_t depth) -> bool
        {
            if (!current || depth > 64u || visited >= 4096u)
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
    static void Register(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
    {
        test("Window.Image2QuietChromeAndCompactInspector",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"image2-visual"));
                 Success(app.Initialize(), "create the hidden image-2 comparison app");
                 app.SetDocument(L"# Weekly report\n\n15 October 2026\n\n## What changed since last report\n\n- First item\n- Second item\n\n## In "
                                 L"progress\n\nNext.\n");
                 app.Execute(Command::Files);
                 app.Prepare(256);
                 const auto listCaret = app.Document().GetText().find(L"Second item");
                 app.Document().SetSelection({listCaret, listCaret});
                 app.SelectionChanged();
                 const auto listKind = Resource(IDS_BLOCKBULLETLIST);
                 const std::size_t listItems = 2;
                 Require(app.listSummary->IsVisible() && !app.currentBlock->IsVisible() &&
                             app.listSummary->GetText() == std::vformat(Resource(IDS_BLOCKITEMINFO), std::make_wformat_args(listKind, listItems)),
                         "the inspector shows the actual enclosing list count from the canonical syntax tree");
                 const auto source = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 for (const UINT dpi : {96u, 144u, 192u})
                 {
                     Resize(app, 1440, 900, dpi);
                     CheckChrome(app);
                     for (const auto *mode : app.modeButtons)
                         Require(mode->GetAppearance() == DxUi::ButtonAppearance::Tab, "view commands use the shared underlined-tab appearance");
                     Require(app.filesToggle->GetAppearance() == DxUi::ButtonAppearance::Subtle &&
                                 app.tools[9]->GetShortcutText() == Resource(IDS_FIND_SHORTCUT) &&
                                 app.blockSourceAction->GetAppearance() == DxUi::ButtonAppearance::Outline,
                             "formatting, Find hints and inspector actions adopt shared quiet controls");
                     Require(!app.Document().GetAppearance().drawBorder && !app.Document().GetAppearance().drawFocusIndicator &&
                                 !app.outlineTree->GetAppearance().drawBackground && !app.outlineTree->GetAppearance().drawBorder &&
                                 !app.outlineTree->GetAppearance().showExpanders && !app.filesTree->GetAppearance().drawBorder,
                             "document and navigation panes have no nested input frames");
                     const auto identity = app.title->GetBounds(), state = app.saveStateChip->GetBounds();
                     Require(Near((identity.top + identity.bottom) / 2, (state.top + state.bottom) / 2) && state.left > identity.right,
                             "filename and save-state chip share the same header baseline");
                     Require(app.blockSourceAction->GetBounds().bottom < 500 && !app.overflowButton->IsVisible() && app.closeFiles->IsVisible(),
                             "short outlines keep block tools nearby and a full-width toolbar needs no overflow button");
                     Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection,
                             "visual layout and DPI changes preserve the authoritative document and selection");
                 }
                 Require(GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()), "image-2 verification preserves the desktop");
             });
        test("Editor.HiddenCurrentBlockPublishesLatestOnReveal",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"hidden-current-block", L"narrow"));
                 Success(app.Initialize(), "create the hidden current-block publication app");
                 Resize(app, 820, 560, 96, false);
                 const auto hiddenPriorText = std::wstring(app.currentBlock->GetText());
                 const auto initiallyHiddenConstructions = app.Activity().currentBlockLabelConstructions;
                 const std::wstring source = L"First paragraph.\n\n- List alpha\n- List beta\n\nSecond paragraph is longer.\n";
                 app.SetDocument(source);
                 app.Prepare(256);
                 const auto first = source.find(L"First paragraph.");
                 const auto list = source.find(L"List alpha");
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
                     const auto position = DocumentPositionAt(app.documentInfo, app.Document().GetSelection().caret);
                     const auto format = Resource(IDS_BLOCKINFO);
                     const auto kind = Resource(IDS_BLOCKPARAGRAPH);
                     return std::vformat(format, std::make_wformat_args(kind, position.lineCharacters));
                 };
                 const auto title = std::wstring(app.currentBlockTitle->GetText());
                 setCaret(first + 3);
                 const auto firstFacts = expectedParagraph();
                 Require(!app.sidebar->IsVisible() && app.currentBlock->IsVisible() && app.currentBlock->GetText() == hiddenPriorText &&
                             app.Activity().currentBlockLabelConstructions == initiallyHiddenConstructions &&
                             !AccessibilityContainsName(app, firstFacts),
                         "the 820-DIP initial layout retains paragraph facts without exposing a hidden label to UI Automation");

                 Resize(app, 1440, 900, 96, false);
                 Require(app.sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == initiallyHiddenConstructions + 1 &&
                             app.currentBlock->GetText() == firstFacts && AccessibilityContainsName(app, firstFacts),
                         "the first sidebar reveal publishes the latest localized paragraph before the first render/provider read");

                 const auto entrySource = app.Document().Snapshot();
                 const auto entrySelection = app.Document().GetSelection();
                 app.settings.focusMode = true;
                 app.Layout();
                 Require(!app.sidebar->IsVisible(), "Focus mode hides the sidebar before repeated source edits");
                 const auto hiddenConstructions = app.Activity().currentBlockLabelConstructions;
                 const auto entryUndo = app.Document().CanUndo();
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
                 Require(app.Document().Snapshot().IsSameRevision(entrySource) &&
                             app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                         "settling the restored syntax while hidden preserves source and still defers label publication");
                 setCaret(list + 5);
                 const auto listKind = Resource(IDS_BLOCKBULLETLIST);
                 const std::size_t listItems = 2;
                 const auto listText = std::vformat(Resource(IDS_BLOCKITEMINFO), std::make_wformat_args(listKind, listItems));
                 Require(
                     !app.currentBlock->IsVisible() && app.listSummary->IsVisible() && app.listSummary->GetText() == listText &&
                         app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                     "hidden list context still updates its immediate local visibility and item summary without constructing the hidden block label");
                 setCaret(second + 3);
                 const auto latestFacts = expectedParagraph();
                 Require(latestFacts != firstFacts && app.currentBlock->IsVisible() && !app.sidebar->IsVisible() &&
                             app.currentBlock->GetText() == firstFacts && !AccessibilityContainsName(app, latestFacts) &&
                             app.Activity().currentBlockLabelConstructions == hiddenConstructions,
                         "a later paragraph replaces hidden pending list facts without publishing early");
                 app.settings.focusMode = false;
                 app.Layout();
                 Require(app.sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == hiddenConstructions + 1 &&
                             app.currentBlock->GetText() == latestFacts && AccessibilityContainsName(app, latestFacts),
                         "revealing Focus publishes the newest paragraph rather than stale list or prior paragraph facts");
                 Require(app.currentBlockTitle->GetText() == title, "deferred label publication preserves the static current-block title resource");

                 const auto visibleListConstructions = app.Activity().currentBlockLabelConstructions;
                 setCaret(list + 5);
                 Require(!app.currentBlock->IsVisible() && app.listSummary->IsVisible() && app.listSummary->GetText() == listText &&
                             app.Activity().currentBlockLabelConstructions == visibleListConstructions,
                         "visible sidebar list selection keeps the list summary immediate and does not publish a locally hidden block label");
                 const auto beforeVisibleParagraph = app.Activity().currentBlockLabelConstructions;
                 setCaret(first + 3);
                 auto visibleFacts = expectedParagraph();
                 Require(app.currentBlock->IsVisible() && app.currentBlock->GetText() == visibleFacts &&
                             app.Activity().currentBlockLabelConstructions == beforeVisibleParagraph + 1,
                         "returning from list context publishes the current paragraph immediately");

                 const auto editSource = app.Document().Snapshot();
                 const auto editSelection = app.Document().GetSelection();
                 Require(app.Document().ReplaceSelection(L"z"), "edit a visible current-block paragraph");
                 visibleFacts = expectedParagraph();
                 Require(app.currentBlock->GetText() == visibleFacts, "a visible paragraph edit updates its label synchronously");
                 Require(app.Document().Undo(), "undo the visible paragraph edit");
                 visibleFacts = expectedParagraph();
                 Require(app.currentBlock->GetText() == visibleFacts && app.Document().Snapshot().IsSameRevision(editSource) &&
                             app.Document().GetSelection() == editSelection && app.Document().CanRedo(),
                         "visible undo immediately restores the exact label, source, caret and redo admission");
                 Require(app.Document().Redo(), "redo the visible paragraph edit");
                 visibleFacts = expectedParagraph();
                 Require(app.currentBlock->GetText() == visibleFacts, "visible redo immediately republishes its paragraph facts");
                 Require(app.Document().Undo(), "restore the source after the visible redo oracle");
                 visibleFacts = expectedParagraph();
                 Require(app.currentBlock->GetText() == visibleFacts && app.Document().Snapshot().IsSameRevision(editSource) &&
                             app.Document().GetSelection() == editSelection,
                         "the complete visible edit/undo/redo sequence restores the original revision and caret");

                 bool selectFirst{};
                 const auto exerciseHiddenReveal = [&](const std::function<void()> &hide, const std::function<void()> &reveal, std::string_view cause)
                 {
                     hide();
                     Require(!app.sidebar->IsVisible(), cause);
                     const auto before = app.Activity().currentBlockLabelConstructions;
                     setCaret((selectFirst ? first : second) + 3);
                     selectFirst = !selectFirst;
                     Require(app.Activity().currentBlockLabelConstructions == before && app.currentBlock->IsVisible(),
                             "each hidden visibility cause defers a changed paragraph's label construction");
                     const auto pending = expectedParagraph();
                     reveal();
                     Require(app.sidebar->IsVisible() && app.Activity().currentBlockLabelConstructions == before + 1 &&
                                 app.currentBlock->GetText() == pending && AccessibilityContainsName(app, pending),
                             "each sidebar reveal publishes current text to the label and UI Automation");
                 };
                 exerciseHiddenReveal(
                     [&]
                     {
                         app.settings.showOutline = false;
                         app.Layout();
                     },
                     [&]
                     {
                         app.settings.showOutline = true;
                         app.Layout();
                     },
                     "outline-off hides the sidebar");
                 exerciseHiddenReveal([&] { app.SetView(EditorView::Split); }, [&] { app.SetView(EditorView::Compose); },
                                      "Split view hides the sidebar");
                 exerciseHiddenReveal([&] { Resize(app, 800, 560, 96, false); }, [&] { Resize(app, 1440, 900, 96, false); },
                                      "the narrow viewport hides the sidebar");
                 exerciseHiddenReveal(
                     [&]
                     {
                         app.settings.showFiles = true;
                         Resize(app, 1024, 900, 96, false);
                         Require(app.filesPane->IsVisible() && app.filesPane->borderRight && !app.filesSplitter->IsVisible(),
                                 "the file-overlay fixture is active rather than a normal left Files pane");
                     },
                     [&]
                     {
                         app.settings.showFiles = false;
                         Resize(app, 1440, 900, 96, false);
                     },
                     "the narrow Files overlay hides the sidebar");
                 Require(app.Document().Snapshot().IsSameRevision(entrySource) && app.currentBlockTitle->GetText() == title &&
                             GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()),
                         "all reveal paths preserve the document and static title without activating the hidden app");
             });
        test("Window.DesignRefreshMergedFrameHitTests",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"design-frame"));
                 Success(app.Initialize(), "create hidden merged-frame app");
                 Resize(app, 1440, 900);
                 Require(HitControl(app, app.closeButton) == HTCLOSE && HitControl(app, app.maximizeButton) == HTMAXBUTTON &&
                             HitControl(app, app.minimizeButton) == HTMINBUTTON,
                         "caption controls report native close/maximize/minimize targets, including the snap-flyout target");
                 Require(HitControl(app, app.saveButton) == HTCLIENT && HitControl(app, app.fileMenu) == HTCLIENT,
                         "Save and File remain client controls rather than draggable caption space");
                 for (auto *mode : app.modeButtons)
                     Require(HitControl(app, mode) == HTCLIENT && !mode->GetAccessibleName().empty() && !mode->GetAccessibleAutomationId().empty(),
                             "each visible named view selector remains a client target");
                 Require(Hit(app, {28, 28}) == HTCAPTION && HitControl(app, app.editor) == HTCLIENT,
                         "document identity is draggable and the document remains ordinary client space");
                 RECT frame{};
                 Require(GetWindowRect(app.Window(), &frame) != FALSE, "read merged frame for border probes");
                 const auto rawHit = [&](int x, int y)
                 { return app.FrameHitTest(app.Window(), MAKELPARAM(static_cast<SHORT>(x), static_cast<SHORT>(y))); };
                 Require(rawHit(frame.left + 1, frame.top + 1) == HTTOPLEFT && rawHit(frame.right - 1, frame.top + 1) == HTTOPRIGHT &&
                             rawHit(frame.left + 1, frame.bottom - 1) == HTBOTTOMLEFT && rawHit(frame.right - 1, frame.bottom - 1) == HTBOTTOMRIGHT,
                         "all four native resize corners remain reachable");
                 Require(rawHit(frame.left + 1, (frame.top + frame.bottom) / 2) == HTLEFT &&
                             rawHit(frame.right - 1, (frame.top + frame.bottom) / 2) == HTRIGHT,
                         "native side resizing survives removal of the duplicate title bar");
                 MINMAXINFO minimum{};
                 (void)SendMessageW(app.Window(), WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&minimum));
                 const auto dpi = GetDpiForWindow(app.Window());
                 Require(minimum.ptMinTrackSize.x == MulDiv(800, static_cast<int>(dpi), 96) &&
                             minimum.ptMinTrackSize.y == MulDiv(560, static_cast<int>(dpi), 96),
                         "the native minimum tracking dimensions follow DPI");
                 app.Execute(Command::Focus);
                 Require(HitControl(app, app.exitFocus) == HTCLIENT && HitControl(app, app.maximizeButton) == HTMAXBUTTON,
                         "Focus mode retains Exit focus and native caption targets");
                 Require(GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()), "frame probing never shows or activates its window");
             });
        test("Window.DesignRefreshResponsiveLayoutAndDpi",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"design-layout"));
                 Success(app.Initialize(), "create hidden responsive-layout app");
                 app.Execute(Command::Files);
                 const auto source = app.Document().Snapshot();
                 const auto selected = app.Document().GetSelection();
                 const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
                 for (const UINT dpi : {96u, 144u, 192u})
                     for (const unsigned width : {800u, 1120u, 1136u, 1142u, 1200u, 1300u, 1440u})
                     {
                         Resize(app, width, width == 800 ? 560 : 900, dpi);
                         CheckChrome(app);
                         const auto actual = app.Host().GetClientBoundsDip();
                         if (actual.right < 1142)
                         {
                             Require(!app.sidebar->IsVisible() && !app.filesSplitter->IsVisible() && app.filesPane->IsVisible() &&
                                         Near(app.page->GetBounds().left, 0),
                                     std::format("Files overlay width={} dpi={} files={} reading={} sidebar={} splitter={} pane={} left={}",
                                                 actual.right, dpi, app.settings.filesWidth, app.settings.readingWidth, app.sidebar->IsVisible(),
                                                 app.filesSplitter->IsVisible(), app.filesPane->IsVisible(), app.page->GetBounds().left)
                                         .c_str());
                             if (width == 800)
                                 Require(app.overflowButton->IsVisible(), "the minimum-width command row moves hidden groups into overflow");
                         }
                         if (actual.right >= 1382)
                             Require(app.sidebar->IsVisible() && app.filesPane->IsVisible() && app.filesSplitter->IsVisible() &&
                                         !Overlap(app.filesPane->GetBounds(), app.page->GetBounds()) &&
                                         !Overlap(app.sidebar->GetBounds(), app.page->GetBounds()),
                                     "wide layout shows both panes beside the canvas without overlap");
                         if (actual.right >= 1142 && actual.right <= 1300)
                             Require(!app.sidebar->IsVisible() && app.filesSplitter->IsVisible(),
                                     "the right panel collapses before the Files pane becomes an overlay");
                         if (app.filesSplitter->IsVisible())
                             Require(app.editor->GetBounds().right - app.editor->GetBounds().left >= app.Preferences().readingWidth - 0.1f &&
                                         Near(app.Document().GetAppearance().maximumTextWidthDip, static_cast<float>(app.Preferences().readingWidth)),
                                     "inline Files includes its splitter and preserves the requested document measure before overlaying");
                         app.SetView(EditorView::Split);
                         CheckChrome(app);
                         Require(app.SourceMirror().IsReadOnly() && !app.sidebar->IsVisible(),
                                 "Split retains a derived read-only mirror and available width");
                         app.SetView(EditorView::Compose);
                         Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selected &&
                                     app.Document().CanUndo() == undo && app.Document().CanRedo() == redo,
                                 "responsive pane transitions preserve source, selection and both history directions");
                     }
                 Require(GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()), "DPI/layout scenarios preserve the desktop");
             });
        test("Editor.DesignRefreshFocusViewsAndToggleStates",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"design-focus"));
                 Success(app.Initialize(), "create hidden focus/view app");
                 app.SetDocument(L"# Durable source\n\nThe writing stays exact.\n");
                 app.Document().SetSelection({18, 18});
                 Require(app.Document().ReplaceSelection(L"Careful "), "admit an undoable edit before presentation changes");
                 const auto source = app.Document().GetText();
                 const auto revision = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 app.Execute(Command::Files);
                 for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
                 {
                     app.SetView(view);
                     for (std::size_t index = 0; index < app.modeButtons.size(); ++index)
                         Require(app.modeButtons[index]->GetToggleState() == std::optional<bool>(index == static_cast<std::size_t>(view)),
                                 "exactly one named segmented view reports pressed state");
                     Require(!app.mirror->GetAppearance().bodyFontFamily.empty() && Near(app.mirror->GetAppearance().bodyFontSizeDip, 13.5f) &&
                                 Near(app.mirror->GetAppearance().lineHeightMultiplier, 1.75f),
                             "the derived Source mirror uses the host monospace family and explicit source measure");
                     if (view == EditorView::Source)
                         Require(app.Document().GetAppearance().bodyFontFamily == app.mirror->GetAppearance().bodyFontFamily &&
                                     Near(app.Document().GetAppearance().bodyFontSizeDip, 13.5f) &&
                                     Near(app.Document().GetAppearance().lineHeightMultiplier, 1.75f),
                                 "the authoritative Source view uses the same monospace presentation as its derived mirror");
                     if (view == EditorView::Split)
                     {
                         const auto mirrored = app.mirror->Snapshot();
                         Require(!app.mirror->ReplaceSelection(L"Forbidden mirror edit") && app.mirror->Snapshot().IsSameRevision(mirrored) &&
                                     app.Document().Snapshot().IsSameRevision(revision),
                                 "the derived Split view rejects edits without changing either source or history");
                     }
                     app.Execute(Command::Focus);
                     Require(app.Preferences().focusMode && !app.commandBar->IsVisible() && !app.sidebar->IsVisible() &&
                                 !app.filesPane->IsVisible() && !app.positionStatus->IsVisible() && app.exitFocus->IsVisible(),
                             "Focus mode removes competing chrome while keeping its explicit exit");
                     const auto edited = Resource(IDS_CHANGED);
                     const std::size_t editedWords = 7;
                     Require(app.focusPill->IsVisible() && app.stats->IsVisible() && Inside(app.stats->GetBounds(), app.focusPill->GetBounds()) &&
                                 app.stats->GetText() == std::vformat(Resource(IDS_FOCUSCOUNTS), std::make_wformat_args(editedWords, edited)),
                             "the focused status pill keeps the independent word count and Edited state without the full status row");
                     Require(app.Document().IsFocusModeEnabled() && Near(app.Document().GetAppearance().maximumTextWidthDip, 680) &&
                                 Near(app.Document().GetAppearance().bodyFontSizeDip, view == EditorView::Source ? 13.5f : 18.f),
                             "Focus mode uses the focused measure/type and shared editor dimming behavior");
                     Require(app.DismissSurface() && !app.Preferences().focusMode, "Escape exits Focus mode through the same dismiss path");
                     Require(app.Document().GetText() == source && app.Document().Snapshot().IsSameRevision(revision) &&
                                 app.Document().GetSelection() == selection && app.Document().CanUndo(),
                             "global view/focus changes preserve authoritative source, selection, revision and undo history");
                 }
                 Require(app.Document().Undo() && !app.Dirty(), "one undo still returns to the saved source after every presentation switch");
                 app.Execute(Command::Focus);
                 const auto saved = Resource(IDS_SAVED);
                 const std::size_t savedWords = 6;
                 Require(app.stats->GetText() == std::vformat(Resource(IDS_FOCUSCOUNTS), std::make_wformat_args(savedWords, saved)),
                         "undoing to the saved revision updates the focused pill to Saved and the original word count");
                 Require(app.DismissSurface(), "leave the focused status-pill check through Escape routing");
                 Require(GetForegroundWindow() == foreground, "hidden focus transitions change logical routing only");
             });
        test("Outline.DesignRefreshHierarchyCurrentAndFindCounts",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 {
                     Application publicationApp(Options(root, L"design-outline-publication"));
                     Success(publicationApp.Initialize(), "create hidden outline-publication app");
                     std::wstring publicationSource = L"Preamble.\n\n# Root\n\nOpening.\n\n";
                     for (unsigned index = 0; index < 20; ++index)
                         publicationSource += L"## Section " + std::to_wstring(index + 1) + L"\n\nBody.\n\n";
                     publicationSource += L"# Last\n\nTail.\n";
                     publicationApp.SetDocument(publicationSource);
                     Resize(publicationApp, 1440, 560, 96);
                     Require(publicationApp.sidebar->IsVisible() && publicationApp.outlineModel.rows.size() == 22 &&
                                 !publicationApp.outlineTree->GetSelectedItemId() && !publicationApp.outlineTree->GetFocusedItemId(),
                             "preamble has no current heading in either canonical tree identity");
                     const auto opening = publicationSource.find(L"Opening");
                     const std::optional<std::uint64_t> rootHeading{1}, lastHeading{22};
                     const auto entering = publicationApp.Activity().outlineSelectionUpdates;
                     publicationApp.Document().SetSelection({opening, opening});
                     Require(publicationApp.Activity().outlineSelectionUpdates == entering + 1 &&
                                 publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == rootHeading,
                             "entering a section synchronously publishes both selected and focused heading identities");
                     const auto stable = publicationApp.Activity().outlineSelectionUpdates;
                     const bool priorUndo = publicationApp.Document().CanUndo();
                     for (unsigned edit = 0; edit < 8; ++edit)
                     {
                         const DxUi::RichText::Selection caret{opening + edit % 2, opening + edit % 2};
                         publicationApp.Document().SetSelection(caret);
                         Require(publicationApp.Document().ReplaceSelection(L"x") && publicationApp.Document().Undo() &&
                                     publicationApp.Document().GetText() == publicationSource && publicationApp.Document().GetSelection() == caret &&
                                     publicationApp.Document().CanUndo() == priorUndo && publicationApp.Document().CanRedo() &&
                                     !publicationApp.Dirty(),
                                 "prose edit and one undo preserve exact source, caret and history within the current section");
                         publicationApp.Prepare(64);
                     }
                     Require(publicationApp.Activity().outlineSelectionUpdates == stable &&
                                 publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == rootHeading,
                             "caret, edit, undo and syntax refreshes never republish an unchanged current section");
                     publicationApp.outlineTree->SetFocusedItemId(2);
                     Require(publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == std::optional<std::uint64_t>{2},
                             "canonical tree focus can differ from its selected item");
                     publicationApp.Document().SetSelection({opening + 3, opening + 3});
                     Require(publicationApp.Activity().outlineSelectionUpdates == stable + 1 &&
                                 publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == rootHeading,
                             "a changed focused identity is immediately reconciled even when selection already matches");
                     const auto tail = publicationSource.find(L"Tail");
                     publicationApp.Document().SetSelection({tail, tail});
                     Require(publicationApp.Activity().outlineSelectionUpdates == stable + 2 &&
                                 publicationApp.outlineTree->GetSelectedItemId() == lastHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == lastHeading &&
                                 publicationApp.outlineTree->GetFirstVisibleItemIndex() > 0,
                             "an actual section transition immediately selects, focuses and reveals the distant heading");
                     const auto rebuilt = publicationApp.Activity().outlineSelectionUpdates;
                     publicationApp.outlineModel.OnTreeToggleExpanded(1, false);
                     Require(publicationApp.outlineModel.rows.size() == 2 && publicationApp.outlineTree->GetFirstVisibleItemIndex() == 0,
                             "collapsing earlier descendants clamps the retained distant heading into the short model");
                     publicationApp.outlineModel.OnTreeToggleExpanded(1, true);
                     const auto revealed = publicationApp.outlineTree->GetVisibleItemHitRect(21);
                     Require(publicationApp.Activity().outlineSelectionUpdates == rebuilt && publicationApp.outlineModel.rows.size() == 22 &&
                                 publicationApp.outlineTree->GetSelectedItemId() == lastHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == lastHeading &&
                                 publicationApp.outlineTree->GetFirstVisibleItemIndex() > 0 && revealed &&
                                 Inside(revealed.value(), publicationApp.outlineTree->GetBounds()),
                             "canonical model refresh reveals the surviving current heading without redundant app publication");
                     publicationApp.SetView(EditorView::Split);
                     publicationApp.Document().SetSelection({opening, opening});
                     const auto hiddenUpdates = publicationApp.Activity().outlineSelectionUpdates;
                     Require(!publicationApp.sidebar->IsVisible() && hiddenUpdates == rebuilt + 1 &&
                                 publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == rootHeading,
                             "hidden Split presentation still receives actual current-section transitions immediately");
                     publicationApp.SetView(EditorView::Compose);
                     Require(publicationApp.sidebar->IsVisible() && publicationApp.Activity().outlineSelectionUpdates == hiddenUpdates &&
                                 publicationApp.outlineTree->GetSelectedItemId() == rootHeading &&
                                 publicationApp.outlineTree->GetFocusedItemId() == rootHeading &&
                                 publicationApp.Document().GetText() == publicationSource && publicationApp.Document().CanUndo() == priorUndo &&
                                 publicationApp.Document().CanRedo(),
                             "revealing the panel preserves its already current identity and the exact source/history");
                     publicationApp.Document().SetSelection({0, 0});
                     const auto cleared = publicationApp.Activity().outlineSelectionUpdates;
                     publicationApp.Document().SetSelection({1, 1});
                     Require(cleared == hiddenUpdates + 1 && publicationApp.Activity().outlineSelectionUpdates == cleared &&
                                 !publicationApp.outlineTree->GetSelectedItemId() && !publicationApp.outlineTree->GetFocusedItemId(),
                             "leaving all sections clears both identities once and repeated preamble carets remain inert");
                 }
                 Require(GetForegroundWindow() == foreground, "outline publication probes never acquire desktop focus");
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
                 Require(app.outlineModel.rows.size() == 4, "empty headings never generate blank outline rows");
                 const auto &rows = app.outlineModel.rows;
                 Require(rows[0].text == L"Root" && rows[0].depth == 0 && rows[1].text == L"Child" && rows[1].depth == 1 &&
                             rows[1].parentId == rows[0].id && rows[2].depth == 2 && rows[2].parentId == rows[1].id && rows[3].depth == 0,
                         "outline rows retain the actual heading hierarchy and stable parent identities");
                 const auto rootId = rows[0].id;
                 app.outlineModel.OnTreeToggleExpanded(rootId, false);
                 Require(app.outlineModel.rows.size() == 2 && app.outlineModel.rows[0].text == L"Root" && !app.outlineModel.rows[0].expanded &&
                             app.outlineModel.rows[1].text == L"Next",
                         "collapsing a parent hides only its descendants through the actual tree delegate");
                 app.outlineModel.OnTreeToggleExpanded(rootId, true);
                 Require(app.outlineModel.rows.size() == 4 && app.outlineModel.rows[0].expanded,
                         "expanding restores the nested hierarchy without rebuilding document source");
                 const auto sectionCaret = source.find(L"Detail");
                 app.Document().SetSelection({sectionCaret, sectionCaret});
                 Require(app.outlineTree->GetSelectedItemId() == std::optional<std::uint64_t>(rows[2].id),
                         "the canonical selection callback exposes its current heading without consumer key/mouse hooks");
                 const auto next = rows[3].id;
                 app.outlineModel.OnTreeItemInvoked(next);
                 Require(app.Document().GetSelection().caret == source.find(L"# Next") && app.Host().GetFocusControl() == &app.Document(),
                         "outline invocation navigates the source offset and returns logical editor routing");
                 app.Execute(Command::Find);
                 app.findText->SetTextAndNotify(L"Needle");
                 app.Prepare(256);
                 Require(app.outlineModel.rows[0].badgeText == L"2" && app.outlineModel.rows[1].badgeText == L"1",
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
                 const auto firstMatch = source.find(L"Needle");
                 app.Document().SetSelection({firstMatch, firstMatch + 6});
                 app.SelectionChanged();
                 Require(app.Activity().findHighlightUpdates == highlightBase + 1,
                         "changing the active current match refreshes the bounded highlight vector once");
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
                 const auto retained = app.Document().Snapshot();
                 const auto retainedSelection = app.Document().GetSelection();
                 const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
                 const auto unchanged = [&]
                 {
                     Require(app.Document().Snapshot().IsSameRevision(retained) && app.Document().GetText() == source &&
                                 app.Document().GetSelection() == retainedSelection && app.Document().CanUndo() == undo &&
                                 app.Document().CanRedo() == redo,
                             "outline collapse and Find presentation preserve exact source, selection and both history directions");
                 };
                 const auto sectionBuildsBeforeCollapse = app.Activity().findSectionCountRebuilds;
                 app.outlineModel.OnTreeToggleExpanded(rootId, false);
                 Require(app.Activity().findSectionCountRebuilds == sectionBuildsBeforeCollapse + 1,
                         "section totals are rebuilt for the changed outline identity, not for caret movement");
                 Require(app.outlineModel.rows.size() == 2 && !app.outlineModel.rows[0].expanded && app.outlineModel.rows[0].badgeText == L"2" &&
                             app.outlineModel.rows[1].text == L"Next",
                         "collapsing during Find hides descendants while the parent still counts every nested match");
                 unchanged();
                 app.Execute(Command::CloseDrawer);
                 Require(app.outlineModel.rows.size() == 2 && !app.outlineModel.rows[0].expanded &&
                             std::ranges::all_of(app.outlineModel.rows, [](const auto &row) { return row.badgeText.empty(); }),
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
                 Require(app.findMatches.size() == 2 && app.outlineModel.rows.size() == 2 && !app.outlineModel.rows[0].expanded &&
                             app.outlineModel.rows[0].badgeText == L"2",
                         "reopening Find restores inclusive counts without overriding a collapsed parent");
                 unchanged();
                 app.outlineModel.OnTreeToggleExpanded(rootId, true);
                 Require(app.outlineModel.rows.size() == 4 && app.outlineModel.rows[0].expanded && app.outlineModel.rows[0].badgeText == L"2" &&
                             app.outlineModel.rows[1].badgeText == L"1",
                         "expanding during Find restores descendants and their own section counts");
                 unchanged();
                 const auto replacementSource = L"# Fresh\n\nNeedle once.\n\n## Nested\n\nNeedle twice.\n";
                 app.SetDocument(replacementSource);
                 Require(app.findMatches.size() == 2 && app.findSource && app.findSource->IsSameRevision(app.Document().Snapshot()) &&
                             app.outlineModel.rows.size() == 2 && app.outlineModel.rows[0].badgeText == L"2" &&
                             app.outlineModel.rows[1].badgeText == L"1",
                         "document replacement rebuilds current source matches and section counts without stale cached badges");
                 app.Execute(Command::CloseDrawer);
                 const auto closedSourceWork = app.Activity();
                 const auto sourceBeforeClosedEdit = app.Document().Snapshot();
                 app.Document().SetSelection({sourceBeforeClosedEdit.Length(), sourceBeforeClosedEdit.Length()});
                 Require(app.Document().ReplaceSelection(L"Needle after refresh\n"), "closed Find source edit commits normally");
                 app.Prepare(512);
                 Require(!app.findSource && app.findMatches.empty() &&
                             std::ranges::all_of(app.outlineModel.rows, [](const auto &row) { return row.badgeText.empty(); }) &&
                             app.Activity().findMatchRebuilds == closedSourceWork.findMatchRebuilds &&
                             app.Activity().findSectionCountRebuilds == closedSourceWork.findSectionCountRebuilds &&
                             app.Activity().findHighlightUpdates == closedSourceWork.findHighlightUpdates,
                         "closed Find stays inert through a source edit and completed syntax preparation");
                 app.Execute(Command::Find);
                 Require(app.findMatches.size() == 3 && app.findSource && app.findSource->IsSameRevision(app.Document().Snapshot()) &&
                             app.outlineModel.rows[0].badgeText == L"3" && app.outlineModel.rows[1].badgeText == L"2",
                         "reopening after closed source and syntax changes rebuilds current full matches and section totals");
                 app.Host().SetFocusControl(&app.Document(), false);
                 const auto committedPreviewSource = app.Document().GetText();
                 const auto committedPreviewSelection = app.Document().GetSelection();
                 const auto previewChanges = app.Activity().changes;
                 const auto previewUndo = app.Document().CanUndo(), previewRedo = app.Document().CanRedo();
                 DxUi::TextInputState entry;
                 entry.text = committedPreviewSource;
                 entry.multiline = true;
                 entry.caretIndex = committedPreviewSelection.caret;
                 if (committedPreviewSelection.anchor != committedPreviewSelection.caret)
                     entry.selectionAnchorIndex = committedPreviewSelection.anchor;
                 auto preview = entry;
                 const auto firstNeedle = committedPreviewSource.find(L"Needle");
                 preview.text.replace(firstNeedle, 6, L"Noodle");
                 const auto previewRebuilds = app.Activity().findMatchRebuilds;
                 const auto previewSectionRebuilds = app.Activity().findSectionCountRebuilds;
                 Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false) && app.findMatches.size() == 2 &&
                             app.Document().GetSelection() == committedPreviewSelection && app.findSource &&
                             app.findSource->IsSameRevision(app.Document().Snapshot()) && app.outlineModel.rows[0].badgeText == L"2" &&
                             app.outlineModel.rows[1].badgeText == L"2" && app.Activity().findMatchRebuilds == previewRebuilds + 1 &&
                             app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 1,
                         "same-ACP non-notifying source preview rebuilds the exact current Find source and ranges");
                 auto secondPreview = preview;
                 secondPreview.text.replace(firstNeedle, 6, L"Nooble");
                 const auto firstPreviewSource = app.Document().Snapshot();
                 Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), secondPreview, false) && app.findMatches.size() == 2 &&
                             app.Document().GetSelection() == committedPreviewSelection &&
                             !app.Document().Snapshot().IsSameRevision(firstPreviewSource) && app.findSource &&
                             app.findSource->IsSameRevision(app.Document().Snapshot()) && app.Activity().findMatchRebuilds == previewRebuilds + 2 &&
                             app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 2,
                         "successive same-ACP preview revisions do not reuse stale Find ranges");
                 Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), entry, false) &&
                             app.Document().GetText() == committedPreviewSource && app.Document().GetSelection() == committedPreviewSelection &&
                             app.findMatches.size() == 3 && app.findSource && app.findSource->IsSameRevision(app.Document().Snapshot()) &&
                             app.outlineModel.rows[0].badgeText == L"3" && app.outlineModel.rows[1].badgeText == L"2" &&
                             app.Activity().findMatchRebuilds == previewRebuilds + 3 &&
                             app.Activity().findSectionCountRebuilds == previewSectionRebuilds + 3,
                         "text-service cancellation restores Find matches and section totals from the entry source revision");
                 Require(app.Activity().changes == previewChanges && app.Document().CanUndo() == previewUndo &&
                             app.Document().CanRedo() == previewRedo,
                         "transient Find preview and cancellation create no committed activity or history");
                 const auto pendingSyntax = app.Document().GetMarkdownSyntax();
                 const auto beforeSyntax = app.Activity();
                 app.Prepare(512);
                 Require(app.Document().GetMarkdownSyntax() && app.Document().GetMarkdownSyntax() != pendingSyntax &&
                             app.Activity().syntaxPresentationUpdates > beforeSyntax.syntaxPresentationUpdates &&
                             app.Activity().documentAnalysisUpdates == beforeSyntax.documentAnalysisUpdates &&
                             app.Activity().findMatchRebuilds == beforeSyntax.findMatchRebuilds &&
                             app.Activity().findSectionCountRebuilds == beforeSyntax.findSectionCountRebuilds &&
                             app.Activity().findHighlightUpdates == beforeSyntax.findHighlightUpdates &&
                             app.Document().GetText() == committedPreviewSource && app.Document().GetSelection() == committedPreviewSelection &&
                             app.Document().CanUndo() == previewUndo && app.Document().CanRedo() == previewRedo,
                         "actual syntax publication reuses current open Find presentation without changing source, selection or history");

                 Application previewApp(Options(root, L"design-outline-source-preview"));
                 Success(previewApp.Initialize(), "create source-consistent outline preview app");
                 const std::wstring previewSource = L"# Root\n\nNeedle root.\n\n## Child\n\nNeedle child.\n";
                 previewApp.SetDocument(previewSource);
                 previewApp.Prepare(256);
                 previewApp.Execute(Command::Find);
                 previewApp.findText->SetTextAndNotify(L"Needle");
                 previewApp.Prepare(256);
                 previewApp.outlineModel.OnTreeToggleExpanded(1, false);
                 Require(previewApp.outlineModel.rows.size() == 1 && previewApp.outlineModel.rows.front().text == L"Root" &&
                             !previewApp.outlineModel.rows.front().expanded,
                         "source-preview fixture begins with a deliberately collapsed committed section");
                 const auto previewCaret = previewSource.find(L"Needle root") + 4;
                 previewApp.Document().SetSelection({previewCaret, previewCaret});
                 const auto caretAnalysis = previewApp.Activity().documentAnalysisUpdates;
                 previewApp.Document().SetSelection({previewCaret + 1, previewCaret + 1});
                 previewApp.SelectionChanged();
                 Require(previewApp.Activity().documentAnalysisUpdates == caretAnalysis,
                         "caret-only selection refresh reuses source-derived document analysis");
                 previewApp.Document().SetSelection({previewCaret, previewCaret});
                 const auto committedSnapshot = previewApp.Document().Snapshot();
                 const auto committedSelection = previewApp.Document().GetSelection();
                 const auto committedChanges = previewApp.Activity().changes;
                 const auto committedRecovery = previewApp.Activity().recoverySubmissions;
                 const auto committedUndo = previewApp.Document().CanUndo(), committedRedo = previewApp.Document().CanRedo();
                 const auto analysisBaseline = previewApp.Activity().documentAnalysisUpdates;
                 DxUi::TextInputState headingPreviewEntry;
                 headingPreviewEntry.text = previewSource;
                 headingPreviewEntry.multiline = true;
                 headingPreviewEntry.caretIndex = previewCaret;
                 auto firstPreview = headingPreviewEntry;
                 firstPreview.text.replace(0, 6, L"Plain "); // Same ACP, but the first heading marker is no longer present.
                 Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), firstPreview, false) &&
                             previewApp.Document().GetText() == firstPreview.text && previewApp.Document().GetSelection() == committedSelection &&
                             previewApp.documentInfo.headings.size() == 1 && previewApp.documentInfo.headings.front().text == L"Child" &&
                             previewApp.documentInfo.headings.front().offset == firstPreview.text.find(L"## Child") &&
                             previewApp.outlineModel.rows.size() == 1 && previewApp.outlineModel.rows.front().text == L"Child" &&
                             previewApp.outlineModel.rows.front().badgeText == L"1" &&
                             previewApp.Activity().documentAnalysisUpdates == analysisBaseline + 1,
                         "same-ACP heading-marker preview refreshes current outline identity, offsets and Find section counts");
                 auto shiftedPreview = firstPreview;
                 const auto childHeading = shiftedPreview.text.find(L"## Child");
                 shiftedPreview.text.insert(childHeading, L"\n"); // Add a source boundary before the child section's match.
                 const auto secondPreviewAnalysis = previewApp.Activity().documentAnalysisUpdates;
                 Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), shiftedPreview, false) &&
                             previewApp.documentInfo.headings.size() == 1 && previewApp.documentInfo.headings.front().offset == childHeading + 1 &&
                             previewApp.outlineModel.rows.size() == 1 && previewApp.outlineModel.rows.front().text == L"Child" &&
                             previewApp.outlineModel.rows.front().badgeText == L"1" &&
                             previewApp.Activity().documentAnalysisUpdates == secondPreviewAnalysis + 1,
                         "successive preview analysis follows a shifted heading boundary while retaining current match totals");
                 const auto cancelAnalysis = previewApp.Activity().documentAnalysisUpdates;
                 Require(
                     (previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), headingPreviewEntry, false) &&
                         previewApp.Document().GetText() == committedSnapshot.Text() && previewApp.Document().GetSelection() == committedSelection &&
                         previewApp.documentInfo.headings.size() == 2 && previewApp.documentInfo.headings[0].text == L"Root" &&
                         previewApp.documentInfo.headings[0].offset == 0 && previewApp.documentInfo.headings[1].text == L"Child" &&
                         previewApp.documentInfo.headings[1].offset == previewSource.find(L"## Child") && previewApp.outlineModel.rows.size() == 1 &&
                         previewApp.outlineModel.rows.front().text == L"Root" && !previewApp.outlineModel.rows.front().expanded &&
                         previewApp.outlineModel.rows.front().badgeText == L"2" && previewApp.findMatches.size() == 2 && previewApp.findSource &&
                         previewApp.findSource->IsSameRevision(previewApp.Document().Snapshot()) &&
                         previewApp.Activity().documentAnalysisUpdates == cancelAnalysis + 1,
                     "cancellation restores entry headings, offsets, collapsed outline and Find badges from its exact source revision");
                 Require(previewApp.Activity().changes == committedChanges && previewApp.Activity().recoverySubmissions == committedRecovery &&
                             previewApp.Document().CanUndo() == committedUndo && previewApp.Document().CanRedo() == committedRedo,
                         "transient heading previews and cancellation preserve committed activity, recovery and undo/redo history");
                 const auto unchangedAnalysis = previewApp.Activity().documentAnalysisUpdates;
                 previewApp.Changed();
                 Require(previewApp.Activity().documentAnalysisUpdates == unchangedAnalysis && previewApp.outlineModel.rows.size() == 1 &&
                             !previewApp.outlineModel.rows.front().expanded && previewApp.Document().GetText() == previewSource &&
                             previewApp.Document().GetSelection() == committedSelection && previewApp.Document().CanUndo() == committedUndo &&
                             previewApp.Document().CanRedo() == committedRedo,
                         "same-revision committed analysis retains the collapsed outline and exact source/history");
                 auto committedHeadingPreview = headingPreviewEntry;
                 committedHeadingPreview.text.replace(2, 4, L"Tree");
                 Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), committedHeadingPreview, false) &&
                             previewApp.documentInfo.headings[0].text == L"Tree" && previewApp.outlineModel.rows.size() == 2,
                         "a changed heading preview temporarily exposes its current outline");
                 Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), headingPreviewEntry, false) &&
                             previewApp.outlineModel.rows.size() == 1 && !previewApp.outlineModel.rows.front().expanded,
                         "restore-entry reinstates the committed collapse before the result transaction");
                 Require((previewApp.Document().*Testing::TextServiceImportProbe::Import)(previewApp.Host(), committedHeadingPreview, true) &&
                             previewApp.documentInfo.headings[0].text == L"Tree" && previewApp.outlineModel.rows.size() == 2 &&
                             previewApp.Document().GetSelection() == committedSelection && previewApp.Document().Undo() &&
                             previewApp.Document().GetText() == previewSource && !previewApp.Document().CanUndo() &&
                             previewApp.documentInfo.headings[0].text == L"Root" && previewApp.Document().Redo() &&
                             previewApp.Document().GetText() == committedHeadingPreview.text && previewApp.documentInfo.headings[0].text == L"Tree",
                         "preview result commit clears changed collapse identity and remains exactly one source undo/redo transaction");
                 previewApp.SetDocument(std::wstring(2048, L'\n'));
                 const auto manyLineCapacity = previewApp.documentInfo.lineIndex.capacity();
                 previewApp.SetDocument(L"Z\r\n");
                 Require(previewApp.documentInfo.lineIndex == std::vector<DocumentLine>{{0, 1}, {3, 0}} &&
                             previewApp.documentInfo.lineIndex.capacity() <= 4 && previewApp.documentInfo.lineIndex.capacity() < manyLineCapacity &&
                             previewApp.Document().GetText() == L"Z\r\n" && !previewApp.Document().CanUndo() && !previewApp.Document().CanRedo(),
                         "small document replacement releases the predecessor's large line index and retains exact clean history");
             });
        test("Editor.DesignRefreshFindReplaceUndoAndCapacity",
             [&]
             {
                 Application app(Options(root, L"design-find"));
                 Success(app.Initialize(), "create hidden Find/Replace app");
                 const std::wstring source = L"α cat 😀 cat\r\ncat";
                 app.SetDocument(source);
                 app.Execute(Command::Find);
                 app.findText->SetTextAndNotify(L"cat");
                 Require(app.findMatches.size() == 3, "Find uses exact source ranges across Unicode and mixed line boundaries");
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
                 app.replaceText->SetTextAndNotify(L"犬");
                 app.Execute(Command::ReplaceAll);
                 Require(app.Document().GetText() == L"α 犬 😀 犬\r\n犬" && app.Dirty(), "Replace all changes only all exact source matches");
                 Require(app.Document().Undo() && app.Document().GetText() == source && !app.Dirty(),
                         "one undo restores every replacement and the original Unicode/EOLs");
                 app.SetDocument(L"aaa");
                 app.Execute(Command::Find);
                 app.findText->SetTextAndNotify(L"a");
                 app.replaceText->SetTextAndNotify(std::wstring(24000, L'z'));
                 const auto retained = app.Document().Snapshot();
                 const auto selected = app.Document().GetSelection();
                 app.Execute(Command::ReplaceAll);
                 Require(app.Document().GetText() == L"aaa" && app.Document().Snapshot().IsSameRevision(retained) &&
                             app.Document().GetSelection() == selected && !app.Document().CanUndo(),
                         "an oversized replacement set is rejected before any source/history/selection mutation");
                 const auto selection = app.Document().GetSelection();
                 Require(app.DismissSurface() && !app.findBar->IsVisible() && app.Host().GetFocusControl() == &app.Document() &&
                             app.Document().GetSelection() == selection,
                         "Escape closes docked Find and restores logical editor routing with the source selection");
                 app.SetDocument(std::wstring(5000, L'a'));
                 app.Execute(Command::Find);
                 app.findText->SetTextAndNotify(L"a");
                 app.Document().SetSelection({4999, 5000});
                 app.SelectionChanged();
                 Require(app.findMatches.size() == 5000 && app.findSource && app.findSource->IsSameRevision(app.Document().Snapshot()) &&
                             app.Host().RenderInitialFrameForShow() && app.Document().GetSelection().Start() == 4999,
                         "a current match beyond the bounded highlight prefix stays selected and renderable with cached exact source ranges");
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
                 const std::wstring source = L"# Exact equation\n\nCode `$fake$` and $a_i^2$ with a tail.\n";
                 app.SetDocument(source);
                 app.Prepare(512);
                 auto at = source.find(L"fake");
                 app.Document().SetSelection({at, at});
                 app.SelectionChanged();
                 Require(!app.equationContext && !app.equation->IsVisible() && !app.equationHelp->IsVisible(),
                         "ordinary/code context reserves no empty equation inspector");
                 at = source.find(L"a_i^2");
                 app.Document().SetSelection({at, at});
                 app.SelectionChanged();
                 Require(app.equationContext && app.equation->IsVisible() && app.equation->IsReadOnly() && app.equationHelp->IsVisible(),
                         "only current math exposes the explicitly read-only contextual equation rendering");
                 for (const unsigned index : {3u, 4u, 5u, 13u})
                     Require(!app.tools[index]->IsEnabled(), "inline formatting commands are disabled inside equations");
                 app.Execute(Command::LocalSource);
                 Require(app.HasBlockDraft() && app.BlockDraft()->GetText() == L"$a_i^2$" && app.Document().IsReadOnly(),
                         "F2 attaches an exact-delimiter guarded equation draft");
                 Require(app.BlockDraft()->GetFontRole() == DxUi::FontRole::Monospace,
                         "exact TeX source uses canonical TextField monospace metrics and input geometry");
                 app.Prepare(512);
                 const auto draftHighlights = app.Document().GetSourceHighlights();
                 Require(draftHighlights.size() == 1 && draftHighlights.front().start == app.blockDraftRange.first &&
                             draftHighlights.front().length == app.blockDraftRange.last - app.blockDraftRange.first &&
                             draftHighlights.front().current,
                         "the open equation draft retains its exact source highlight across completed presentation work");
                 const auto anchor = app.Document().TryGetTextInputRangeRects(app.Host(), app.blockDraftRange.first, app.blockDraftRange.last);
                 Require(anchor && !anchor->empty() && !app.pendingBlockPlacement && Near(app.blockDraft->GetBounds().top, anchor->back().bottom + 8),
                         std::format("prepared F2 source must attach below native geometry: anchor={}, pending={}, top={}, expected={}, bottom={}",
                                     anchor && !anchor->empty(), app.pendingBlockPlacement, app.blockDraft->GetBounds().top,
                                     anchor && !anchor->empty() ? anchor->back().bottom + 8 : -1.f, app.page->GetBounds().bottom));
                 app.BlockDraft()->SetTextAndNotify(L"$a_i^3$");
                 app.Execute(Command::Find);
                 Require(app.HasBlockDraft() && !app.findBar->IsVisible() && app.Document().GetText() == source,
                         "another presentation command cannot discard or mutate a pending equation draft");
                 app.Execute(Command::CloseDrawer);
                 Require(!app.HasBlockDraft() && !app.Document().IsReadOnly() && app.Document().GetSourceHighlights().empty() &&
                             app.Document().GetText() == source && !app.Dirty(),
                         "Cancel preserves exact source/history and restores editing");
                 app.Document().SetSelection({at, at});
                 app.Execute(Command::LocalSource);
                 app.BlockDraft()->SetTextAndNotify(L"$a_i^3$");
                 app.Execute(Command::ApplyBlock);
                 Require(app.Document().GetSourceHighlights().empty() &&
                             app.Document().GetText() == L"# Exact equation\n\nCode `$fake$` and $a_i^3$ with a tail.\n" && app.Document().Undo() &&
                             app.Document().GetText() == source,
                         "Apply is a single undoable exact-source equation replacement");
                 app.SetDocument(L"");
                 app.Prepare(512);
                 app.Execute(Command::LocalSource);
                 app.Prepare(512);
                 const auto emptyCaret = app.Document().TryGetTextInputCaretRect(app.Host(), 0);
                 Require(app.HasBlockDraft() && app.BlockDraft()->GetText().empty() && emptyCaret && !app.pendingBlockPlacement &&
                             Near(app.blockDraft->GetBounds().top, emptyCaret->bottom + 8),
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
                 const auto linkedSource = app.Document().Snapshot();
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
                 const auto preservedLink = std::ranges::find_if(linkedSemantic->Elements(), [](const auto &element)
                                                                 { return element.role == DxUi::RichText::SemanticRole::Link; });
                 Require(app.Preferences().view == EditorView::Compose && app.Document().GetViewMode() == DxUi::RichText::ViewMode::Rich &&
                             app.Document().Snapshot().IsSameRevision(linkedSource) && app.Document().GetSelection() == linkedSelection &&
                             app.Document().CanUndo() == linkedUndo && app.Document().CanRedo() == linkedRedo &&
                             preservedLink != linkedSemantic->Elements().end() && preservedLink->title == L"Parent title",
                         "Compose restores native math while preserving parent link title and both history directions");
                 app.Execute(Command::LocalSource);
                 Require(app.HasBlockDraft() && app.BlockDraft()->GetText() == L"$a_i^2$", "F2 opens the exact linked equation envelope");
                 app.BlockDraft()->SetTextAndNotify(L"$a_i^3$");
                 app.Execute(Command::CloseDrawer);
                 Require(!app.HasBlockDraft() && app.Document().GetText() == linkedEquation && !app.Dirty() &&
                             app.Document().CanUndo() == linkedUndo && app.Document().CanRedo() == linkedRedo,
                         "canceling linked equation source leaves its text and history untouched");
                 app.Execute(Command::LocalSource);
                 app.BlockDraft()->SetTextAndNotify(L"$a_i^3$");
                 app.Execute(Command::ApplyBlock);
                 const std::wstring editedLinkedEquation = L"[Parent](https://example.test \"Parent title\") and $a_i^3$.\n";
                 Require(app.Document().GetText() == editedLinkedEquation && app.Document().Undo() && app.Document().GetText() == linkedEquation &&
                             app.Document().Redo() && app.Document().GetText() == editedLinkedEquation,
                         "F2 apply is one undoable linked equation edit with exact surrounding source");
                 app.Prepare(512);
                 linkedSemantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
                 Require(static_cast<bool>(linkedSemantic), "edited linked semantic publication exists");
                 const auto retainedLink = std::ranges::find_if(linkedSemantic->Elements(), [](const auto &element)
                                                                { return element.role == DxUi::RichText::SemanticRole::Link; });
                 Require(retainedLink != linkedSemantic->Elements().end() && retainedLink->title == L"Parent title",
                         "apply/undo/redo preserves the neighboring parent link title");
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
                 StoredDocument loaded;
                 Success(LoadDocument(first, loaded), "read original Files fixture");
                 app.SetDocument(loaded.text, first, loaded.bom, loaded.stamp);
                 const auto selection = app.Document().GetSelection();
                 app.Execute(Command::Files);
                 Await(app, [&] { return app.folderSnapshot && app.folderSnapshot->root == first.parent_path(); });
                 Require(app.folderRoot == first.parent_path() && app.folderSnapshot->markdownCount == 3 &&
                             app.Host().GetFocusControl() == app.filesTree && app.filesToggle->GetToggleState() == std::optional<bool>(true),
                         "opening Files publishes the document folder and routes logical focus to its real tree");
                 const auto folder =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"nested"; });
                 Require(folder != app.filesModel.rows.end() && !folder->expanded && folder->badgeText == L"2",
                         "other folders start collapsed with Markdown counts");
                 const auto id = folder->id;
                 app.filesModel.OnTreeToggleExpanded(id, true);
                 Require(std::ranges::any_of(app.filesModel.rows, [](const auto &row) { return row.text == L"child.MD"; }),
                         "shared-tree expansion exposes application-owned child rows");
                 const auto retainedFilesSource = app.Document().Snapshot();
                 const auto notificationGeneration = app.folderSnapshot->generation;
                 auto notificationRevision = app.folderSnapshot->revision;
                 const auto arrived = first.parent_path() / L"arrived";
                 const auto arrivedDocument = arrived / L"new.md";
                 WriteFixture(arrivedDocument);
                 Await(app,
                       [&]
                       {
                           return app.folderSnapshot && app.folderSnapshot->revision > notificationRevision &&
                                  std::ranges::any_of(app.folderSnapshot->entries, [&](const auto &entry) { return entry.path == arrivedDocument; });
                       });
                 const auto arrivedRow =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"arrived"; });
                 const auto nestedRow =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"nested"; });
                 Require(arrivedRow != app.filesModel.rows.end() && !arrivedRow->expanded && arrivedRow->badgeText == L"1" &&
                             nestedRow != app.filesModel.rows.end() && nestedRow->expanded &&
                             std::ranges::none_of(app.filesModel.rows, [](const auto &row) { return row.text == L"new.md"; }),
                         "an actual notification starts a newly discovered folder collapsed and preserves existing user expansion");
                 const auto currentRow =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"current.md"; });
                 Require(app.folderRoot == first.parent_path() && app.folderSnapshot->generation == notificationGeneration &&
                             currentRow != app.filesModel.rows.end() && app.filesTree->GetSelectedItemId() == std::optional(currentRow->id),
                         "notification refresh keeps the document's containing root expanded and its current-file row visible and selected");
                 const auto arrivedId = arrivedRow->id, nestedId = nestedRow->id;
                 app.filesModel.OnTreeToggleExpanded(arrivedId, true);
                 app.filesModel.OnTreeToggleExpanded(nestedId, false);
                 notificationRevision = app.folderSnapshot->revision;
                 WriteFixture(first.parent_path() / L"another.md");
                 Await(app,
                       [&]
                       {
                           return app.folderSnapshot && app.folderSnapshot->revision > notificationRevision &&
                                  std::ranges::any_of(app.folderSnapshot->entries,
                                                      [&](const auto &entry) { return entry.path == first.parent_path() / L"another.md"; });
                       });
                 Require(std::ranges::any_of(app.filesModel.rows, [](const auto &row) { return row.text == L"new.md"; }) &&
                             std::ranges::none_of(app.filesModel.rows, [](const auto &row) { return row.text == L"child.MD"; }),
                         "later notifications preserve both explicit expansion and explicit collapse choices");
                 const auto removedRow =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"arrived"; });
                 Require(removedRow != app.filesModel.rows.end(), "the folder to retire remains in the current Files model");
                 app.filesModel.OnTreeToggleExpanded(removedRow->id, false);
                 notificationRevision = app.folderSnapshot->revision;
                 Require(std::filesystem::remove(arrivedDocument) && std::filesystem::remove(arrived), "remove only the owned empty folder fixture");
                 Await(app,
                       [&]
                       {
                           return app.folderSnapshot && app.folderSnapshot->revision > notificationRevision &&
                                  std::ranges::none_of(app.folderSnapshot->entries, [&](const auto &entry) { return entry.path == arrived; });
                       });
                 Require(!app.collapsedFolders.contains(arrived), "removed directories retire their retained collapse-state keys");
                 notificationRevision = app.folderSnapshot->revision;
                 WriteFixture(arrivedDocument);
                 Await(app,
                       [&]
                       {
                           return app.folderSnapshot && app.folderSnapshot->revision > notificationRevision &&
                                  std::ranges::any_of(app.folderSnapshot->entries, [&](const auto &entry) { return entry.path == arrivedDocument; });
                       });
                 Require(std::ranges::any_of(app.filesModel.rows, [](const auto &row) { return row.text == L"arrived" && !row.expanded; }) &&
                             app.Document().Snapshot().IsSameRevision(retainedFilesSource) && app.Document().GetSelection() == selection &&
                             !app.Document().CanUndo() && !app.Document().CanRedo(),
                         "a recreated directory starts collapsed without notification edits to source, selection or history");
                 app.filesFilter->SetTextAndNotify(L"CHILD");
                 Require(app.filesModel.rows.size() == 2 && app.filesModel.rows[0].text == L"nested" && app.filesModel.rows[1].text == L"child.MD",
                         "the live name filter keeps only case-insensitive matching files and ancestors");
                 Require(app.DismissSurface() && app.filesFilter->GetText().empty() && app.Preferences().showFiles,
                         "first Escape clears the file filter without closing the pane");
                 app.Document().SetSelection({0, 0});
                 Require(app.Document().ReplaceSelection(L"Uncommitted "), "make the current document dirty before a tree open");
                 const auto retained = app.Document().GetText();
                 app.OpenPath(other);
                 app.Prepare(128);
                 Require(app.documentPath == first && app.Document().GetText() == retained && !app.files.Busy(),
                         "headless refusal of dirty-document consent prevents tree-open data loss");
                 Require(app.Document().Undo() && !app.Dirty(), "return to the clean source before an admitted open");
                 app.OpenPath(other);
                 Await(app, [&] { return app.documentPath == other && app.folderSnapshot && app.folderSnapshot->root == other.parent_path(); });
                 Require(app.Document().GetText() == L"# Published\n" && app.folderRoot == other.parent_path(),
                         "the normal async Open completion re-roots Files only after successful load");
                 const auto savedAs = directory / L"Archive/renamed.md";
                 std::filesystem::create_directories(savedAs.parent_path());
                 Success(app.files.Submit(true, savedAs, app.Document().GetText(), false),
                         "admit the save-as writer stage without a native file picker");
                 Await(app, [&] { return app.documentPath == savedAs && app.folderSnapshot && app.folderSnapshot->root == savedAs.parent_path(); });
                 Require(app.folderRoot == savedAs.parent_path() && !app.Dirty(),
                         "successful Save As completion re-roots Files and preserves the saved baseline");
                 app.Document().SetSelection(selection);
                 app.CloseDrawers();
                 Require(app.DismissSurface() && !app.Preferences().showFiles && app.Host().GetFocusControl() == &app.Document() &&
                             app.Document().GetSelection() == selection,
                         "closing Files restores the editor's selection and logical routing");
             });
        test("Editor.FilesDirtyBadgeUpdatesOneVisibleRow",
             [&]
             {
                 const auto directory = root / L"files-dirty-badge-fixture";
                 const auto document = directory / L"current.md";
                 WriteFixture(document, "# Current document\n");
                 for (unsigned index = 0; index < 96; ++index)
                     WriteFixture(directory / std::format(L"note-{:03}.md", index));
                 WriteFixture(directory / L"archive/retained.md");

                 Application app(Options(root, L"files-dirty-badge"));
                 Success(app.Initialize(), "create the hidden large Files-pane app");
                 StoredDocument loaded;
                 Success(LoadDocument(document, loaded), "read the current document fixture");
                 app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
                 app.Execute(Command::Files);
                 Await(app, [&] { return app.folderSnapshot && app.folderSnapshot->root == directory && !app.folderFiles.Busy(); });
                 Require(app.filesModel.rows.size() == 98, "the synthetic Files model contains a nontrivial set of visible rows");
                 const auto collapsed =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"archive"; });
                 Require(collapsed != app.filesModel.rows.end() && !collapsed->expanded && collapsed->badgeText == L"1",
                         "the fixture includes a collapsed directory with an independent count badge");

                 const auto current =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"current.md"; });
                 Require(current != app.filesModel.rows.end() && current->badgeText.empty() && !current->badgeIsDot,
                         "the clean current document starts without the Edited dot");
                 const auto currentId = current->id;
                 const auto selectedId = app.filesModel.rows.back().id;
                 app.filesTree->SetSelectedItemId(selectedId);
                 app.filesTree->SetFocusedItemId(selectedId);
                 const auto treeBounds = app.filesTree->GetBounds();
                 const auto wheelPoint = D2D1::Point2F(treeBounds.left + 20.f, treeBounds.top + 20.f);
                 for (unsigned step = 0; step < 6; ++step)
                     Require(app.filesTree->OnMouseWheel(app.Host(), wheelPoint, static_cast<float>(WHEEL_DELTA), 0),
                             "scroll the tree away from its focused last row");
                 const auto firstVisible = app.filesTree->GetFirstVisibleItemIndex();
                 Require(firstVisible > 0 && firstVisible < app.filesModel.rows.size() - 1 &&
                             app.filesTree->GetSelectedItemId() == std::optional(selectedId) &&
                             app.filesTree->GetFocusedItemId() == std::optional(selectedId),
                         "the fixture has an offscreen focused row and a separately observable scroll position");

                 struct TreeItemProperties
                 {
                     std::wstring name, itemStatus;
                 };
                 const auto readStringProperty = [](IRawElementProviderSimple *provider, PROPERTYID property, std::string_view message)
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
                     const auto visit = [&](auto &&self, IRawElementProviderFragment *current, std::size_t depth) -> bool
                     {
                         if (!current || depth > 64u || visited >= 4096u)
                             return false;
                         ++visited;
                         wil::com_ptr_nothrow<IRawElementProviderSimple> simple;
                         Success(current->QueryInterface(IID_PPV_ARGS(simple.put())), "query the UI Automation fragment's simple provider");
                         wil::unique_variant controlType;
                         Success(simple->GetPropertyValue(UIA_ControlTypePropertyId, &controlType),
                                 "read the current UI Automation fragment's control type");
                         Require(controlType.vt == VT_I4, "UI Automation control type uses its documented integer value");
                         if (controlType.lVal == UIA_TreeItemControlTypeId)
                         {
                             const auto name = readStringProperty(simple.get(), UIA_NamePropertyId, "read a Files TreeItem name");
                             if (name == expectedName)
                             {
                                 result = TreeItemProperties{
                                     name, readStringProperty(simple.get(), UIA_ItemStatusPropertyId, "read a Files TreeItem status")};
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
                 const auto cleanProvider = findTreeItem(L"current.md");
                 const auto initialArchiveProvider = findTreeItem(L"archive");
                 Require(cleanProvider.name == L"current.md" && cleanProvider.itemStatus.empty() && initialArchiveProvider.name == L"archive" &&
                             initialArchiveProvider.itemStatus == L"1",
                         "the real UI Automation providers expose a clean file and the collapsed archive's count badge");

                 const auto rowSnapshot = app.filesModel.rows;
                 const auto folderGeneration = app.folderSnapshot->generation;
                 const auto folderRevision = app.folderSnapshot->revision;
                 const auto folderStatus = app.folderSnapshot->status;
                 const auto watchStatus = app.folderSnapshot->watchStatus;
                 const auto footer = std::wstring(app.filesCount->GetText());
                 const auto rebuilds = app.Activity().filesModelRebuilds;
                 Require(app.Document().ReplaceSelection(L"x"), "make the current document dirty");
                 Require(app.Activity().filesModelRebuilds == rebuilds && app.filesModel.rows.size() == rowSnapshot.size(),
                         "dirty state changes update the existing row model without rebuilding it");
                 for (std::size_t index = 0; index < rowSnapshot.size(); ++index)
                 {
                     const auto &before = rowSnapshot[index];
                     const auto &after = app.filesModel.rows[index];
                     const bool isCurrent = before.id == currentId;
                     Require(before.id == after.id && before.parentId == after.parentId && before.text == after.text &&
                                 before.iconText == after.iconText && before.tooltipText == after.tooltipText && before.depth == after.depth &&
                                 before.hasChildren == after.hasChildren && before.expanded == after.expanded &&
                                 before.badgeTone == after.badgeTone &&
                                 (isCurrent || (before.badgeText == after.badgeText && before.badgeIsDot == after.badgeIsDot)),
                             "only the current file row presentation changes on the dirty transition");
                 }
                 const auto edited =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [&](const auto &row) { return row.id == currentId; });
                 Require(edited != app.filesModel.rows.end() && edited->badgeText == Resource(IDS_FILESEDITED) && edited->badgeIsDot,
                         "the current file receives the localized accessible dot status");
                 app.Prepare(256);
                 const auto editedProvider = findTreeItem(L"current.md");
                 const auto dirtyArchiveProvider = findTreeItem(L"archive");
                 Require(editedProvider.name == L"current.md" && editedProvider.itemStatus == Resource(IDS_FILESEDITED) &&
                             dirtyArchiveProvider.name == L"archive" && dirtyArchiveProvider.itemStatus == initialArchiveProvider.itemStatus,
                         "the live UI Automation providers expose the localized Edited status only on the current file");
                 Require(app.filesTree->GetSelectedItemId() == std::optional(selectedId) &&
                             app.filesTree->GetFocusedItemId() == std::optional(selectedId) &&
                             app.filesTree->GetFirstVisibleItemIndex() == firstVisible && app.filesCount->GetText() == footer &&
                             app.folderSnapshot->generation == folderGeneration && app.folderSnapshot->revision == folderRevision &&
                             app.folderSnapshot->status == folderStatus && app.folderSnapshot->watchStatus == watchStatus,
                         "a dirty badge update preserves tree selection, focus, scroll, footer and watcher snapshot");

                 Require(app.Document().Undo() && !app.Dirty(), "undo restores the exact clean document state");
                 Require(app.Activity().filesModelRebuilds == rebuilds && app.filesTree->GetSelectedItemId() == std::optional(selectedId) &&
                             app.filesTree->GetFocusedItemId() == std::optional(selectedId) &&
                             app.filesTree->GetFirstVisibleItemIndex() == firstVisible,
                         "undo removes the dot without rebuilding or moving the user's tree state");
                 app.Prepare(256);
                 const auto restoredProvider = findTreeItem(L"current.md");
                 const auto restoredArchiveProvider = findTreeItem(L"archive");
                 Require(restoredProvider.name == L"current.md" && restoredProvider.itemStatus.empty() &&
                             restoredArchiveProvider.name == L"archive" && restoredArchiveProvider.itemStatus == initialArchiveProvider.itemStatus,
                         "undo clears only the current file's live UI Automation status and preserves the neighboring count");
                 const auto restored =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [&](const auto &row) { return row.id == currentId; });
                 Require(restored != app.filesModel.rows.end() && restored->badgeText.empty() && !restored->badgeIsDot,
                         "the current file dot clears when its source becomes clean");

                 app.filesFilter->SetTextAndNotify(L"note-");
                 Require(std::ranges::none_of(app.filesModel.rows, [](const auto &row) { return row.text == L"current.md"; }),
                         "the filter hides the current file row without changing the snapshot");
                 const auto filteredRebuilds = app.Activity().filesModelRebuilds;
                 Require(app.Document().ReplaceSelection(L"y"), "dirty the current document while its row is filtered out");
                 Require(app.Activity().filesModelRebuilds == filteredRebuilds &&
                             std::ranges::none_of(app.filesModel.rows, [](const auto &row) { return row.text == L"current.md"; }),
                         "a filtered-out current file updates bookkeeping without rebuilding visible rows");
                 Require(app.Document().Undo() && !app.Dirty(), "filtered current document returns to its clean state");
                 app.filesFilter->SetTextAndNotify(L"");
                 Require(app.Document().ReplaceSelection(L"z"), "dirty the document before replacing it with a different root");
                 const auto nextDirectory = root / L"files-dirty-badge-next-root";
                 const auto nextDocument = nextDirectory / L"renamed.md";
                 WriteFixture(nextDocument, "# Renamed document\n");
                 StoredDocument next;
                 Success(LoadDocument(nextDocument, next), "read the replacement root fixture");
                 app.SetDocument(next.text, nextDocument, next.bom, next.stamp);
                 Await(app, [&] { return app.folderSnapshot && app.folderSnapshot->root == nextDirectory && !app.folderFiles.Busy(); });
                 const auto renamed =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"renamed.md"; });
                 Require(app.folderRoot == nextDirectory && !app.Dirty() && !app.lastFilesDirty && renamed != app.filesModel.rows.end() &&
                             renamed->badgeText.empty() && !renamed->badgeIsDot,
                         "a dirty-to-clean document replacement uses the full root refresh and publishes the new clean row");

                 Application untitled(Options(root, L"files-dirty-no-snapshot"));
                 Success(untitled.Initialize(), "create the hidden untitled Files-pane app");
                 untitled.Execute(Command::Files);
                 Require(!untitled.folderSnapshot && untitled.filesModel.rows.empty() && !untitled.folderFilesActive,
                         "the untitled Files pane has no folder snapshot or worker");
                 const auto emptyFooter = std::wstring(untitled.filesCount->GetText());
                 const auto emptyRebuilds = untitled.Activity().filesModelRebuilds;
                 Require(untitled.Document().ReplaceSelection(L"x") && untitled.Dirty() && untitled.lastFilesDirty,
                         "dirty bookkeeping updates while the Files pane has no snapshot");
                 Require(untitled.Activity().filesModelRebuilds == emptyRebuilds && untitled.filesModel.rows.empty() &&
                             untitled.filesCount->GetText() == emptyFooter,
                         "an empty-root dirty transition leaves the empty model and localized footer intact");
                 Require(untitled.Document().Undo() && !untitled.Dirty() && !untitled.lastFilesDirty &&
                             untitled.Activity().filesModelRebuilds == emptyRebuilds,
                         "undo synchronizes clean bookkeeping without a synthetic folder refresh");
             });
        test("Editor.DesignRefreshFilesAdmissionErrorsAndRootActions",
             [&]
             {
                 const auto directory = root / L"design-files-errors-fixture";
                 const auto document = directory / L"Draft/current.md";
                 WriteFixture(document, "# Current\n");
                 WriteFixture(directory / L"Draft/folder.md/inside.md");
                 Application app(Options(root, L"design-files-errors"));
                 Success(app.Initialize(), "create hidden Files error/root-action app");
                 app.SetDocument(L"Unsaved writing");
                 app.Execute(Command::Files);
                 Require(app.filesCount->GetText() == Resource(IDS_FILESUNTITLED) && !app.folderFilesActive,
                         "opening Files for untitled writing needs neither a root watcher nor a misleading empty scan");
                 Require(app.filesFooterMeasure.measurements == 1 && app.filesFooterMeasure.typographyRevision == app.Host().GetTypographyRevision(),
                         "the first cold Files open measures once and retains the typography epoch initialized by that measurement");
                 app.Layout();
                 Require(app.filesFooterMeasure.measurements == 1, "the cold visible footer measurement is reused on the next layout");
                 StoredDocument loaded;
                 Success(LoadDocument(document, loaded), "load first saved root fixture");
                 app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
                 Await(app, [&] { return app.folderSnapshot && app.folderSnapshot->root == document.parent_path(); });
                 Require(app.folderFilesActive && app.filesTitle->GetText() == L"Draft", "a first saved document starts and labels the Files worker");
                 const auto folder =
                     std::find_if(app.filesModel.rows.begin(), app.filesModel.rows.end(), [](const auto &row) { return row.text == L"folder.md"; });
                 Require(folder != app.filesModel.rows.end(), "the populated Markdown-suffixed folder is visible");
                 const auto retained = app.Document().Snapshot();
                 app.filesModel.OnTreeItemInvoked(folder->id);
                 Require(!app.files.Busy() && app.documentPath == document && app.Document().Snapshot().IsSameRevision(retained),
                         "invoking a folder whose name ends in .md never submits a file-open command");
                 const auto missing = directory / L"unavailable/current.md";
                 app.SetDocument(L"Still editable\n", missing);
                 Await(app, [&] { return app.folderSnapshot && app.folderSnapshot->root == missing.parent_path(); });
                 Require(FAILED(app.folderSnapshot->status) && app.folderSnapshot->errorCount > 0 &&
                             app.filesCount->GetText() != Resource(IDS_FILESLOADING) && app.Document().GetText() == L"Still editable\n",
                         "an unavailable root settles as an explicit pane error while the document remains editable");
                 // Force an admission refusal at the application/service boundary,
                 // without allocation injection, thread hooks or native desktop UI.
                 app.folderFiles.Close();
                 app.documentPath = directory / L"next/current.md";
                 app.RefreshFiles(true);
                 Require(app.filesCount->GetText() == Resource(IDS_FILESUNAVAILABLE),
                         "a rejected scan admission retains its error instead of waiting forever for an unsubmitted result");
                 Require(app.Document().ReplaceSelection(L"Available "), "Files service failure never disables document editing");
                 // A synthetic root model tests the label without scanning a real
                 // drive root or creating any path outside this harness fixture.
                 app.folderFilesActive = false;
                 app.documentPath = L"R:\\current.md";
                 app.folderRoot = L"R:\\";
                 app.folderSnapshot = FilesSnapshot{};
                 app.folderSnapshot->root = app.folderRoot;
                 app.RefreshFiles();
                 Require(app.filesTitle->GetText() == L"R:\\" && !app.folderFilesActive,
                         "a volume root keeps a readable label even when filename() is empty, without acquiring another volume");
                 auto &snapshot = app.folderSnapshot.value();
                 snapshot.markdownCount = 7;
                 snapshot.entryLimitReached = true;
                 app.RefreshFiles();
                 Require(app.filesCount->GetText() == L"7 Markdown files · 5000 examined-entry traversal limit",
                         "the entry footer distinguishes returned Markdown files from the canonical examined-entry ceiling");
                 snapshot.entryLimitReached = false;
                 snapshot.depthLimitReached = true;
                 app.RefreshFiles();
                 Require(app.filesCount->GetText() == L"7 Markdown files · depth 8 traversal limit",
                         "a depth-only footer reports its own applied depth ceiling");
                 snapshot.entryLimitReached = true;
                 app.RefreshFiles();
                 Require(app.filesCount->GetText() == L"7 Markdown files · 5000 examined-entry traversal limit · depth 8 traversal limit",
                         "simultaneous limits retain both explanations");
                 snapshot.appliedLimits = {2, 3};
                 app.RefreshFiles();
                 Require(app.filesCount->GetText() == L"7 Markdown files · 3 examined-entry traversal limit · depth 2 traversal limit",
                         "custom admitted limits are reported from the snapshot rather than duplicated defaults");
                 snapshot.status = E_ACCESSDENIED;
                 snapshot.errorCount = 1;
                 snapshot.watchStatus = E_FAIL;
                 app.RefreshFiles();
                 Require(app.filesCount->GetText() == L"7 Markdown files · some folders unavailable · change notifications unavailable · 3 "
                                                      L"examined-entry traversal limit · depth 2 traversal limit",
                         "access and watch failures never hide either simultaneous traversal limit");
                 const std::wstring completeFooter(app.filesCount->GetText());
                 app.settings.filesWidth = 200;
                 Resize(app, 800, 560, 96);
                 DxUi::DesiredSize desired;
                 Success(DxUi::MeasureControl(*app.filesCount, {app.Host()}, DxUi::LayoutConstraints::AtMost(176.f), desired),
                         "independently measure the complete wrapped footer through canonical DxUi");
                 const auto footer = app.filesCount->GetBounds();
                 Require(footer.bottom - footer.top >= desired.sizeDip.height && footer.bottom - footer.top > 52 &&
                             Inside(footer, app.filesPane->GetBounds()) && Inside(app.filesTree->GetBounds(), app.filesPane->GetBounds()) &&
                             !Overlap(footer, app.filesTree->GetBounds()) && !Overlap(footer, app.filesFilter->GetBounds()) &&
                             !Overlap(app.filesTree->GetBounds(), app.filesFilter->GetBounds()),
                         "the minimum pane reserves every wrapped footer fact without overlapping the tree or filter");
                 const auto measures = app.filesFooterMeasure.measurements;
                 app.Layout();
                 app.Layout();
                 Require(app.filesFooterMeasure.measurements == measures,
                         "unchanged layout reuses its footer measurement instead of creating another native text layout");
                 app.settings.filesWidth = 220;
                 app.Layout();
                 Require(app.filesFooterMeasure.measurements == measures + 1, "a changed footer width performs one new canonical measurement");
                 snapshot.entryLimitReached = snapshot.depthLimitReached = false;
                 snapshot.status = snapshot.watchStatus = S_OK;
                 snapshot.errorCount = 0;
                 app.RefreshFiles();
                 Require(Near(app.filesCount->GetBounds().bottom - app.filesCount->GetBounds().top, 24),
                         "the ordinary short-count footer uses the compact image-2 minimum");
                 const auto retainedSource = app.Document().Snapshot();
                 const auto retainedRevision = app.Document().Revision();
                 const auto retainedSelection = app.Document().GetSelection();
                 const bool retainedUndo = app.Document().CanUndo(), retainedRedo = app.Document().CanRedo(), retainedDirty = app.Dirty();
                 app.settings.showFiles = false;
                 app.Layout();
                 const auto hiddenMeasures = app.filesFooterMeasure.measurements;
                 const auto retainedFooterWidth = app.filesFooterMeasure.widthDip, retainedFooterDpi = app.filesFooterMeasure.dpi;
                 const auto retainedFooterRevision = app.filesFooterMeasure.textRevision;
                 for (const UINT dpi : {96u, 144u, 192u})
                 {
                     app.settings.filesWidth = dpi == 144 ? 220u : 200u;
                     app.filesCount->SetText(dpi == 144 ? Resource(IDS_FILESUNTITLED) : completeFooter);
                     Resize(app, 800, 560, dpi, false);
                     app.settings.appearance = dpi == 144 ? Appearance::Dark : Appearance::Light;
                     app.ApplyTheme();
                     Require(!app.filesPane->IsVisible() && app.filesFooterMeasure.measurements == hiddenMeasures &&
                                 app.filesFooterMeasure.widthDip == retainedFooterWidth && app.filesFooterMeasure.dpi == retainedFooterDpi &&
                                 app.filesFooterMeasure.textRevision == retainedFooterRevision,
                             "closed Files preserves its stale measurement across real owner resize, DPI, text and theme changes");
                 }
                 app.settings.showFiles = true;
                 app.Layout();
                 Require(app.filesPane->IsVisible() && app.filesFooterMeasure.measurements == hiddenMeasures + 1 &&
                             app.filesFooterMeasure.textRevision == app.filesCount->GetMeasureRevision() &&
                             app.filesFooterMeasure.typographyRevision == app.Host().GetTypographyRevision() &&
                             Near(app.filesFooterMeasure.widthDip, 176) && Near(app.filesFooterMeasure.dpi, 192),
                         "reopening Files measures the current text, narrow width, typography and DPI exactly once");
                 DxUi::DesiredSize reopenedDesired;
                 Success(DxUi::MeasureControl(*app.filesCount, {app.Host()}, DxUi::LayoutConstraints::AtMost(176.f), reopenedDesired),
                         "independently measure the complete footer after hidden changes");
                 const auto reopenedFooter = app.filesCount->GetBounds();
                 Require(app.filesCount->GetText() == completeFooter &&
                             reopenedFooter.bottom - reopenedFooter.top >= reopenedDesired.sizeDip.height &&
                             reopenedFooter.bottom - reopenedFooter.top > 52 && Inside(reopenedFooter, app.filesPane->GetBounds()) &&
                             !Overlap(reopenedFooter, app.filesTree->GetBounds()) && !Overlap(reopenedFooter, app.filesFilter->GetBounds()),
                         "the first reopened layout reserves every wrapped fact without clipping or overlapping Files content");
                 app.Layout();
                 Require(app.filesFooterMeasure.measurements == hiddenMeasures + 1,
                         "an unchanged visible layout reuses the measurement acquired on reopening");
                 app.settings.focusMode = true;
                 app.Layout();
                 const std::size_t emptyFiles = 0;
                 app.filesCount->SetText(std::vformat(Resource(IDS_FILESCOUNT), std::make_wformat_args(emptyFiles)));
                 app.settings.filesWidth = 220;
                 app.Layout();
                 Require(!app.filesPane->IsVisible() && app.filesFooterMeasure.measurements == hiddenMeasures + 1,
                         "Focus mode also defers measurements for a requested but logically hidden Files pane");
                 app.settings.focusMode = false;
                 app.Layout();
                 Require(app.filesPane->IsVisible() && app.filesFooterMeasure.measurements == hiddenMeasures + 2 &&
                             Near(app.filesCount->GetBounds().bottom - app.filesCount->GetBounds().top, 24),
                         "leaving Focus measures a stale short footer once and restores its minimum height");
                 Require(app.Document().Snapshot().IsSameRevision(retainedSource) && app.Document().Revision() == retainedRevision &&
                             app.Document().GetSelection() == retainedSelection && app.Document().CanUndo() == retainedUndo &&
                             app.Document().CanRedo() == retainedRedo && app.Dirty() == retainedDirty,
                         "deferred footer measurement preserves exact source, selection, history and save state");
             });
        test("Editor.DesignRefreshTransientSurfacesAndClose",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 const auto document = root / L"design-close-fixture/current.md";
                 WriteFixture(document, "# Retained\n\nFind this text.\n");
                 Application app(Options(root, L"design-close"));
                 Success(app.Initialize(), "create hidden surface/close app");
                 StoredDocument loaded;
                 Success(LoadDocument(document, loaded), "load close fixture");
                 app.SetDocument(loaded.text, document, loaded.bom, loaded.stamp);
                 app.Execute(Command::Files);
                 Await(app, [&] { return app.folderSnapshot.has_value(); });
                 app.Execute(Command::Settings);
                 Require(app.drawer->IsVisible() && !app.findBar->IsVisible(), "Settings owns one visible contextual surface");
                 app.Execute(Command::Find);
                 app.findText->SetTextAndNotify(L"Find");
                 Require(app.findBar->IsVisible() && !app.drawer->IsVisible() && app.findMatches.size() == 1,
                         "Find replaces Settings and computes its own visible query state");
                 app.Execute(Command::Settings);
                 Require(app.drawer->IsVisible() && !app.findBar->IsVisible() && app.findMatches.empty() &&
                             std::ranges::all_of(app.outlineModel.rows, [](const auto &row) { return row.badgeText.empty(); }),
                         "hiding Find through Settings clears source highlights and transient outline counts");
                 app.Execute(Command::CloseDrawer);
                 app.Execute(Command::Find);
                 const auto caret = app.Document().GetText().find(L"Find");
                 app.Document().SetSelection({caret, caret});
                 app.Execute(Command::LocalSource);
                 Require(app.HasBlockDraft() && !app.findBar->IsVisible() && app.findMatches.empty(),
                         "F2 also clears Find presentation when switching to an exact local-source draft");
                 app.Execute(Command::CloseDrawer);
                 Require(!app.Dirty(), "surface commands never create document edits");
                 app.Document().SetSelection({caret, caret + 4});
                 app.Prepare(256);
                 Require(app.floating->IsVisible(), "programmatic selection opens tools through the shared callback");
                 Require(app.selectionHint->GetTypographyOverride().has_value() &&
                             Near(app.selectionHint->GetTypographyOverride().value().fontSizeDip, 11) &&
                             Inside(app.selectionHint->GetBounds(), app.selectionHintChip->GetBounds()),
                         "the keyboard hint uses an 11-DIP label inside the shared bordered chip");
                 Resize(app, 800, 560);
                 Require(app.floating->IsVisible() && Inside(app.floating->GetBounds(), app.page->GetBounds()),
                         "visible selected-text tools remain inside the writing viewport after resize");
                 app.Execute(Command::CloseDrawer);
                 // Exercise a newly observed syntax identity after explicit
                 // dismissal. Derived context must refresh without reopening it.
                 app.selectionFactsSyntax.reset();
                 app.Prepare(256);
                 Require(!app.floating->IsVisible() && !app.pendingSelectionTools,
                         "late syntax reconciliation preserves explicit selected-tools dismissal");
                 const auto generation = app.folderFiles.Generation();
                 Require(generation > 0, "the close scenario owns a real active directory watcher");
                 (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
                 Require(app.closing && app.Host().GetHwnd() == nullptr && app.folderFiles.Generation() == 0,
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
                         if (!app.Window())
                             break;
                     }
                     app.FinishClose();
                     if (app.Window())
                         (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                 }
                 Require(app.Window() == nullptr && !app.folderFiles.TakeResult(),
                         "asynchronous retirement finishes without a late UI result or UI-thread join");
                 Require(GetForegroundWindow() == foreground, "watch/surface shutdown never steals desktop focus");
             });
        test("Editor.DesignRefreshLiveSettingsAndEscape",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"design-settings"));
                 Success(app.Initialize(), "create hidden live-settings app");
                 app.SetDocument(L"# Settings preserve writing\n\nA retained source.\n");
                 const auto revision = app.Document().Snapshot();
                 Resize(app, 800, 560);
                 app.Execute(Command::Settings);
                 Require(app.drawer->IsVisible() && app.settingsToggle->GetToggleState() == std::optional<bool>(true),
                         "Settings opens as a pressed-state nonmodal drawer");
                 Require(app.readingWidth->RequestValue(app.Host(), 920) && app.textScale->RequestValue(app.Host(), 125),
                         "real shared stepper commits update appearance preferences");
                 Require(app.Preferences().readingWidth == 920 && app.Preferences().textScale == 125 &&
                             Near(app.Document().GetAppearance().maximumTextWidthDip, 920) &&
                             Near(app.Document().GetAppearance().bodyFontSizeDip, 20),
                         "reading measure and text scale apply immediately to the authoritative editor");
                 app.Execute(Command::ThemePaper);
                 Require(app.Preferences().appearance == Appearance::Paper && app.themeButtons[3]->GetToggleState() == std::optional<bool>(true),
                         "theme tiles expose the current live appearance through pressed state");
                 app.Execute(Command::DimBlocks);
                 Require(!app.Preferences().dimInactiveBlocks && app.dimToggle->GetToggleState() == std::optional<bool>(false),
                         "focus dimming is a live persisted preference with a real toggle state");
                 Require(!app.Preferences().allowRemoteImages && !app.Document().RemoteImagesEnabled() &&
                             app.remoteImagesToggle->GetToggleState() == std::optional<bool>(false),
                         "the privacy control starts off and reports the actual editor image policy");
                 app.Execute(Command::RemoteImages);
                 Require(app.Preferences().allowRemoteImages && app.Document().RemoteImagesEnabled() &&
                             app.remoteImagesToggle->GetToggleState() == std::optional<bool>(true),
                         "the live privacy control applies a usable opt-in rather than a decorative preference");
                 app.Execute(Command::RemoteImages);
                 app.settingsScroll->SetScrollOffset(400);
                 Require(app.settingsScroll->GetScrollOffset() > 0 &&
                             app.settingsScroll->GetContentHeight() >
                                 app.settingsScroll->GetViewportRect().bottom - app.settingsScroll->GetViewportRect().top,
                         "the minimum-window drawer can scroll its complete settings content");
                 Require(app.DismissSurface() && !app.drawer->IsVisible() && app.Host().GetFocusControl() == &app.Document() &&
                             app.Document().Snapshot().IsSameRevision(revision) && !app.Dirty(),
                         "Escape returns logical editor routing without creating source/history edits");
                 Require(GetForegroundWindow() == foreground, "live settings never take desktop focus in hidden tests");
             });
        test("Rendering.DesignRefreshTypographyAndThemeRoles",
             [&]
             {
                 {
                     const auto foreground = GetForegroundWindow();
                     Application lazy(Options(root, L"design-lazy-layout"));
                     Success(lazy.Initialize(), "create hidden app before its first explicit preparation");
                     Require(lazy.Host().GetTypographyRevision() == 0 && lazy.Host().GetDeviceContext() == nullptr &&
                                 lazy.filesFooterMeasure.measurements == 0,
                             "hidden initialization leaves this host's text formats, graphics context and closed footer unprepared");
                     const std::wstring text = L"# Quiet writing\n\nKeep these source words.\n";
                     lazy.SetDocument(text);
                     lazy.Document().SetSelection({text.size(), text.size()});
                     Require(lazy.Document().ReplaceSelection(L"!") && lazy.Document().Undo(),
                             "unprepared layout fixture has a real retained redo transaction");
                     const auto source = lazy.Document().Snapshot();
                     const auto revision = lazy.Document().Revision();
                     const auto selection = lazy.Document().GetSelection();
                     for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
                     {
                         lazy.SetView(view);
                         Resize(lazy, view == EditorView::Source ? 800u : 1440u, 900, view == EditorView::Split ? 192u : 96u, false);
                         lazy.settings.appearance = view == EditorView::Source ? Appearance::Paper : Appearance::Light;
                         lazy.ApplyTheme();
                         lazy.Layout();
                         Require(lazy.Host().GetTypographyRevision() == 0 && lazy.Host().GetDeviceContext() == nullptr &&
                                     lazy.filesFooterMeasure.measurements == 0 && !lazy.SourceMirror().GetAppearance().bodyFontFamily.empty(),
                                 "source-family policy, hidden footer, view, DPI and theme layout acquire no eager host text or graphics resources");
                         if (view == EditorView::Source)
                             Require(lazy.Document().GetAppearance().bodyFontFamily == lazy.SourceMirror().GetAppearance().bodyFontFamily,
                                     "Source and the derived mirror receive the same nonempty monospace family before preparation");
                         Require(lazy.Document().Snapshot().IsSameRevision(source) && lazy.Document().Revision() == revision &&
                                     lazy.Document().GetSelection() == selection && lazy.Document().GetText() == text && !lazy.Document().CanUndo() &&
                                     lazy.Document().CanRedo() && !lazy.Dirty(),
                                 "unprepared view and layout changes preserve source ACP, revision, selection and redo history");
                     }
                     lazy.Prepare(256);
                     auto *monospace = lazy.Host().GetTextFormat(DxUi::FontRole::Monospace);
                     Require(monospace && lazy.Host().GetTypographyRevision() != 0 && lazy.Host().GetDeviceContext() != nullptr,
                             "the first real preparation initializes the ordinary shared host resources");
                     std::wstring resolvedFamily(static_cast<std::size_t>(monospace->GetFontFamilyNameLength()) + 1, L'\0');
                     Success(monospace->GetFontFamilyName(resolvedFamily.data(), static_cast<UINT32>(resolvedFamily.size())),
                             "read the native resolved monospace family after preparation");
                     resolvedFamily.pop_back();
                     Require(lazy.SourceMirror().GetAppearance().bodyFontFamily == resolvedFamily && lazy.SourceMirror().GetText() == text &&
                                 lazy.Document().Snapshot().IsSameRevision(source) && lazy.Document().Revision() == revision &&
                                 lazy.Document().GetSelection() == selection && !lazy.Document().CanUndo() && lazy.Document().CanRedo() &&
                                 !lazy.Dirty(),
                             "deferred canonical family selection matches native typography and leaves the derived source/history exact");
                     Require(GetForegroundWindow() == foreground && !IsWindowVisible(lazy.Window()),
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
                 const auto source = app.Document().Snapshot();
                 const auto sourceRevision = app.Document().Revision();
                 const auto checkSpacing = [&]
                 {
                     app.Prepare(256);
                     const auto firstItem = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"First item"));
                     const auto secondItem = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"Second item"));
                     const auto firstParagraph = app.Document().TryGetTextInputCaretRect(app.Host(), paragraphOffset);
                     const auto secondParagraph = app.Document().TryGetTextInputCaretRect(app.Host(), documentText.find(L"Paragraph two"));
                     const auto &type = app.Document().GetAppearance();
                     const float bodyLine = type.bodyFontSizeDip * type.lineHeightMultiplier;
                     // Native hit-test caret extents can be shorter than the
                     // configured line advance. Equal-font top insets cancel;
                     // compare complete row advances, without clipped UIA bounds.
                     Require(type.paragraphSpacingAfterLines == 18.f / 26.4f && type.listItemSpacingDip == 10.f && firstItem && secondItem &&
                                 firstParagraph && secondParagraph && firstItem->bottom > firstItem->top &&
                                 firstParagraph->bottom > firstParagraph->top && Near(secondItem->top - firstItem->top - bodyLine, 10) &&
                                 Near(secondParagraph->top - firstParagraph->top, 2 * bodyLine),
                             "native rich geometry uses ten-DIP item gaps and credits one real blank line toward its body-line paragraph margin");
                     Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().Revision() == sourceRevision &&
                                 app.Document().GetSelection() == retainedSelection && app.Document().GetText() == documentText &&
                                 !app.Document().CanUndo() && app.Document().CanRedo(),
                             "shared typography preparation preserves exact source, caret, revision and redo history");
                 };
                 checkSpacing();
                 Require(Near(app.saveButton->GetContentFontSizeDip().value_or(0), 14) &&
                             Near(app.modeButtons[0]->GetContentFontSizeDip().value_or(0), 14) &&
                             Near(app.tools[2]->GetContentFontSizeDip().value_or(0), 14) &&
                             Near(app.tools[3]->GetContentFontSizeDip().value_or(0), 18) &&
                             Near(app.minimizeButton->GetContentFontSizeDip().value_or(0), 12),
                         "chrome uses shared 14-DIP labels, 18-DIP command glyphs and compact 12-DIP native caption glyphs");
                 app.Execute(Command::ThemeLight);
                 if (!app.Host().GetTheme().highContrast)
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
                 if (!app.Host().GetTheme().highContrast)
                     Color(app.Host().GetTheme().cardBackground, 0xFFFAEF, "Paper page matches the design role");
                 app.Execute(Command::ThemeDark);
                 if (!app.Host().GetTheme().highContrast)
                 {
                     Color(app.Host().GetTheme().windowBackground, 0x17191E, "Dark chrome matches the design role");
                     Color(app.Host().GetTheme().surfaceBackground, 0x1C1F25, "Dark panes match the design role");
                     Color(app.Host().GetTheme().cardBackground, 0x22252B, "Dark page matches the design role");
                     Color(app.Host().GetTheme().selectionInactiveFill, 0xA63B46, "Dark inactive current rows retain the product tint");
                     Require(app.Host().GetTheme().primaryButtonFill.has_value(), "the primary button has an explicit accessible fill");
                     Color(app.Host().GetTheme().primaryButtonFill.value(), 0xC83C46, "the primary button retains contrast with its white label");
                 }
                 app.systemDark = false;
                 app.Layout();
                 const auto systemPreviewOnLight = app.themeButtons[0]->GetThemePreview();
                 const auto lightPreview = app.themeButtons[1]->GetThemePreview();
                 const auto darkPreview = app.themeButtons[2]->GetThemePreview();
                 Require(systemPreviewOnLight.has_value() && lightPreview.has_value() && darkPreview.has_value() &&
                             systemPreviewOnLight.value() == lightPreview.value() && systemPreviewOnLight.value() != darkPreview.value(),
                         "System appearance preview uses the cached light system preference while explicit Dark is selected");
                 app.Execute(Command::ThemeLight);
                 app.systemDark = true;
                 app.Layout();
                 const auto systemPreviewOnDark = app.themeButtons[0]->GetThemePreview();
                 const auto updatedDarkPreview = app.themeButtons[2]->GetThemePreview();
                 const auto updatedLightPreview = app.themeButtons[1]->GetThemePreview();
                 Require(systemPreviewOnDark.has_value() && updatedDarkPreview.has_value() && updatedLightPreview.has_value() &&
                             systemPreviewOnDark.value() == updatedDarkPreview.value() && systemPreviewOnDark.value() != updatedLightPreview.value(),
                         "System appearance preview uses the cached dark system preference while explicit Light is selected");
                 const auto captureForeground = GetForegroundWindow();
                 std::string firstSettingsCapture;
                 for (const bool initialSystemDark : {false, true})
                 {
                     auto captureOptions =
                         Options(root, initialSystemDark ? L"settings-capture-from-dark" : L"settings-capture-from-light", L"settings");
                     captureOptions.capture = captureOptions.dataDirectory / L"settings.png";
                     Application captureApp(captureOptions);
                     Success(captureApp.Initialize(), "create a hidden Settings capture fixture");
                     captureApp.Execute(Command::ThemeLight);
                     captureApp.systemDark = initialSystemDark;
                     captureApp.PrepareDesignScenario();
                     const auto capturedSystemPreview = captureApp.themeButtons[0]->GetThemePreview();
                     const auto capturedLightPreview = captureApp.themeButtons[1]->GetThemePreview();
                     Require(!captureApp.systemDark && capturedSystemPreview.has_value() && capturedLightPreview.has_value() &&
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
                     const auto captureSource = captureApp.Document().Snapshot();
                     const auto captureRevision = captureApp.Document().Revision();
                     const auto captureSelection = captureApp.Document().GetSelection();
                     const bool captureUndo = captureApp.Document().CanUndo();
                     const auto captureFocus = captureApp.Host().GetFocusControl();
                     captureApp.Layout();
                     captureApp.Prepare(512);
                     Require(captureApp.Capture(captureOptions.capture), "capture the actual hidden Settings render");
                     Require(captureApp.Document().Snapshot().IsSameRevision(captureSource) && captureApp.Document().Revision() == captureRevision &&
                                 captureApp.Document().GetSelection() == captureSelection && captureApp.Document().CanUndo() == captureUndo &&
                                 captureApp.Document().CanRedo() && captureApp.Host().GetFocusControl() == captureFocus &&
                                 GetForegroundWindow() == captureForeground && !IsWindowVisible(captureApp.Window()),
                             "Settings fixture rendering preserves source, selection, both history directions and hidden logical focus");
                     std::string settingsCapture;
                     Success(ReadBytes(captureOptions.capture, settingsCapture), "read the actual Settings PNG");
                     Require(!settingsCapture.empty() && (firstSettingsCapture.empty() || settingsCapture == firstSettingsCapture),
                             "Settings capture bytes are identical for initially Light and Dark cached preferences");
                     firstSettingsCapture = std::move(settingsCapture);
                 }
                 app.Execute(Command::ThemeDark);
                 Require(app.Document().GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret &&
                             app.mirror->GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret,
                         "Dark matches the reference with hover/caret link underlining in Compose and Split");
                 const auto &appearance = app.Document().GetAppearance();
                 Require(!appearance.drawBorder && appearance.useAccentLinkColor && Near(appearance.paddingDip, 0) &&
                             Near(appearance.maximumTextWidthDip, 760) && Near(appearance.bodyFontSizeDip, 16) &&
                             Near(appearance.codeFontSizeDip, 13.5f),
                         "the page uses shared rich-editor appearance with no nested editor frame and the intended typography");
                 Require(app.Host().RenderInitialFrameForShow() && app.Document().Snapshot().IsSameRevision(source),
                         "theme/type presentation renders through the native host without changing source history");
                 Require(app.outlineTitle->GetTypographyOverride().has_value() &&
                             Near(app.outlineTitle->GetTypographyOverride().value().fontSizeDip, 11) &&
                             Near(app.outlineTitle->GetTypographyOverride().value().characterSpacingDip, .88f) &&
                             app.outlineTree->GetTypographyOverride().has_value() &&
                             Near(app.outlineTree->GetTypographyOverride().value().fontSizeDip, 13) &&
                             app.themeButtons[3]->GetThemePreview().has_value(),
                         "sections, hierarchical rows and theme previews use shared typography/presentation APIs");
                 app.SetView(EditorView::Source);
                 Require(Near(app.Document().GetAppearance().bodyFontSizeDip, 13.5f) &&
                             Near(app.Document().GetAppearance().lineHeightMultiplier, 1.75f) &&
                             !app.Document().GetAppearance().bodyFontFamily.empty() && app.Document().GetAppearance().colorizeMarkdownSource,
                         "Source retains monospace 13.5/1.75 syntax ink instead of inheriting rich prose typography");
                 app.SetView(EditorView::Split);
                 app.Prepare(256);
                 checkSpacing();
                 Require(Near(app.SourceMirror().GetAppearance().bodyFontSizeDip, 13.5f) &&
                             app.SourceMirror().GetAppearance().colorizeMarkdownSource && app.Document().Snapshot().IsSameRevision(source) &&
                             app.SourceMirror().GetMarkdownSyntax() && !app.SourceMirror().DebugGetState().sourceInkPreparationPending &&
                             app.SourceMirror().DebugGetState().sourceInkRangeCount > 0,
                         "the read-only Split mirror uses the source appearance without changing document history");
                 Application contrast(Options(root, L"design-high-contrast", L"high-contrast"));
                 Success(contrast.Initialize(), "create deterministic hidden high-contrast app");
                 Require(contrast.Host().GetTheme().highContrast, "the harness high-contrast scenario uses the shared accessibility palette");
                 const auto background = GetSysColor(COLOR_WINDOW);
                 const unsigned rgb = (static_cast<unsigned>(GetRValue(background)) << 16) | (static_cast<unsigned>(GetGValue(background)) << 8) |
                                      GetBValue(background);
                 Color(contrast.Host().GetTheme().cardBackground, rgb, "high contrast uses the system window surface");
                 const auto hyperlink = GetSysColor(COLOR_HOTLIGHT);
                 const unsigned linkRgb =
                     (static_cast<unsigned>(GetRValue(hyperlink)) << 16) | (static_cast<unsigned>(GetGValue(hyperlink)) << 8) | GetBValue(hyperlink);
                 Color(contrast.Host().GetTheme().accent, linkRgb, "high-contrast rich links use the system hyperlink ink");
                 Require(contrast.Document().GetLinkUnderlinePolicy() == DxUi::RichTextLinkUnderlinePolicy::Always,
                         "high contrast always retains an underline");
                 const auto module = GetModuleHandleW(nullptr);
                 for (const unsigned size : {16u, 24u, 32u, 48u, 64u, 128u, 256u})
                     CheckBrandIconResource(module, size);
                 Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().Revision() == sourceRevision &&
                             app.Document().GetSelection() == retainedSelection && !app.Document().CanUndo() && app.Document().CanRedo(),
                         "theme, view, native spacing and icon checks leave exact source, selection and history unchanged");
             });
        test("Editor.DesignRefreshDeferredSelectionToolsFocus",
             [&]
             {
                 const auto foreground = GetForegroundWindow();
                 Application app(Options(root, L"design-deferred-tools"));
                 Success(app.Initialize(), "create hidden deferred-tools app");
                 app.SetDocument(L"# Prepared tools\n\nSelect these careful words.\n");
                 app.Prepare(512);
                 const auto source = app.Document().Snapshot();
                 const auto at = app.Document().GetText().find(L"careful");
                 app.Document().SetSelection({at, at + 7});
                 auto bounds = app.Document().GetBounds();
                 bounds.right = bounds.left + 600;
                 const auto retireGeometry = [&]
                 {
                     bounds.right -= 1;
                     app.Document().SetBounds(bounds);
                     app.Host().SetFocusControl(&app.Document(), false);
                 };
                 retireGeometry();
                 app.FocusSelectionTools();
                 Require(app.selectionToolsFocus && app.pendingSelectionTools, "Alt+F10 intent survives temporarily unavailable native geometry");
                 app.Prepare(512);
                 Require(app.floating->IsVisible() && !app.selectionToolsFocus && !app.pendingSelectionTools &&
                             app.Host().GetFocusControl() == app.floatingActions.front(),
                         "the prepared selection fulfills keyboard focus on the first toolbar action");
                 app.CloseDrawers();
                 retireGeometry();
                 app.FocusSelectionTools();
                 Require(app.selectionToolsFocus && app.DismissSurface(), "Escape owns a deferred toolbar request before it becomes visible");
                 app.Prepare(512);
                 Require(!app.selectionToolsFocus && !app.floating->IsVisible() && app.Host().GetFocusControl() == &app.Document(),
                         "Escape cancels deferred first-action focus and prevents a late toolbar reopening");
                 retireGeometry();
                 app.FocusSelectionTools();
                 Require(app.selectionToolsFocus && app.pendingSelectionTools, "a changed native measure defers focus before selection cancellation");
                 app.Document().SetSelection({at + 1, at + 7});
                 app.Prepare(512);
                 Require(!app.selectionToolsFocus && app.Host().GetFocusControl() == &app.Document(),
                         "a changed selection cannot fulfill an older toolbar focus request");
                 app.CloseDrawers();
                 retireGeometry();
                 app.FocusSelectionTools();
                 Require(app.selectionToolsFocus && app.pendingSelectionTools, "a changed native measure defers focus before revision cancellation");
                 const auto retainedSelection = app.Document().GetSelection();
                 const DxUi::RichText::SourceEdit append{{source.Length(), source.Length()}, L"A "};
                 Require(app.Document().ApplySourceEdits(source, app.Document().Revision(), std::span(&append, 1), retainedSelection) &&
                             app.Document().GetSelection() == retainedSelection,
                         "admit a new source revision while preserving the deferred request's selection");
                 app.Prepare(512);
                 Require(!app.selectionToolsFocus && app.Host().GetFocusControl() == &app.Document() && app.Document().Undo() &&
                             app.Document().GetText() == source.Text(),
                         "a changed source revision cancels deferred focus while keeping one exact undo transaction");
                 app.Document().SetSelection({at, at + 7});
                 app.Prepare(512);
                 app.Host().SetFocusControl(app.floatingActions[1], false);
                 MSG activation{};
                 activation.hwnd = app.Window();
                 activation.message = WM_KEYDOWN;
                 activation.wParam = VK_RETURN;
                 activation.lParam = 1 | static_cast<LPARAM>(MapVirtualKeyW(VK_RETURN, MAPVK_VK_TO_VSC) << 16);
                 activation.time = GetTickCount();
                 Require(app.Host().DispatchKeyboardMessage(activation), "dispatch toolbar activation through the canonical pre-translation hook");
                 auto formatted = source.Text();
                 formatted.replace(at, 7, L"**careful**");
                 Require(app.Document().GetText() == formatted && app.Host().GetFocusControl() == &app.Document(),
                         "the toolbar action applies Bold and returns logical editor focus on keydown");
                 // The canonical pump owns character translation before a
                 // callback changes focus. A command must create no editor char.
                 MSG character{};
                 Require(!PeekMessageW(&character, app.Window(), WM_CHAR, WM_CHAR, PM_NOREMOVE),
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
                     std::array<BYTE, 256> savedKeyboard{};
                     Require(GetKeyboardState(savedKeyboard.data()) != FALSE, "snapshot only this test thread's keyboard state");
                     const auto restoreKeyboard = wil::scope_exit(
                         [&] noexcept
                         {
                             if (!SetKeyboardState(savedKeyboard.data()))
                                 std::terminate();
                         });
                     const auto modifiers = [&](bool ctrl, bool alt, bool shift)
                     {
                         std::array<BYTE, 256> keys{};
                         keys[VK_CONTROL] = keys[VK_LCONTROL] = ctrl ? 0x80 : 0;
                         keys[VK_MENU] = keys[VK_RMENU] = alt ? 0x80 : 0;
                         keys[VK_SHIFT] = keys[VK_LSHIFT] = shift ? 0x80 : 0;
                         Require(SetKeyboardState(keys.data()) != FALSE, "set isolated calling-thread modifier state without injecting input");
                     };
                     MSG routed{};
                     routed.hwnd = app.Window();
                     routed.message = WM_KEYDOWN;
                     const auto retained = app.Document().Snapshot();
                     const auto selected = app.Document().GetSelection();
                     const bool undo = app.Document().CanUndo(), redo = app.Document().CanRedo();
                     const auto focus = app.Host().GetFocusControl();
                     const auto passThrough = [&](UINT key, bool ctrl, bool alt, bool shift)
                     {
                         modifiers(ctrl, alt, shift);
                         routed.wParam = key;
                         Require(!app.RouteShortcut(routed), "unassigned modified keys pass through application shortcut routing");
                         Require(app.Document().Snapshot().IsSameRevision(retained) && app.Document().GetSelection() == selected &&
                                     app.Document().CanUndo() == undo && app.Document().CanRedo() == redo && app.Host().GetFocusControl() == focus,
                                 "AltGr and modified function keys preserve source, selection, history and focus");
                     };
                     passThrough('E', true, true, false);
                     passThrough('K', true, true, false);
                     for (const auto key : {VK_F2, VK_F11})
                     {
                         passThrough(key, true, false, false);
                         passThrough(key, false, true, false);
                         passThrough(key, false, false, true);
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
                     auto code = source.Text();
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
                     Require(app.Host().GetFocusControl() == app.floatingActions.front(), "Alt+F10 focuses the prepared first selection action");
                     modifiers(false, false, false);
                     app.CloseDrawers();
                 }
                 Require(GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()),
                         "deferred logical-focus tests never take the desktop");
             });
        test(
            "Window.DesignRefreshPersistentStatusFacts",
            [&]
            {
                Application app(Options(root, L"design-status"));
                Success(app.Initialize(), "create hidden persistent-status app");
                {
                    const auto loader = app.images.state_;
                    Require(static_cast<bool>(loader), "the hidden app owns its image-loader state");
                    {
                        std::lock_guard lock(loader->mutex);
                        Require(!loader->active && !loader->jobCount && !loader->current.Id(),
                                "the image-free fixture has no active or queued decoder");
                        Require(ResetEvent(loader->networkAbort.get()) != FALSE, "clear only the idle fixture's owned abort event");
                    }
                    for (unsigned index = 0; index < 8; ++index)
                        app.images.DiscardCancelled();
                    Require(WaitForSingleObject(loader->networkAbort.get(), 0) == WAIT_TIMEOUT,
                            "pruning empty image requests leaves the actual owned network-abort event unsignalled");
                }
                Require(app.positionStatus->GetTypographyOverride().has_value() && app.stats->GetTypographyOverride().has_value() &&
                            Near(app.positionStatus->GetTypographyOverride().value().fontSizeDip, 12) &&
                            Near(app.stats->GetTypographyOverride().value().fontSizeDip, 12),
                        "persistent document facts use the specified 12-DIP status size");
                Require(Resource(IDS_BLOCKINFO).find(L'\x00B7') != std::wstring::npos &&
                            Resource(IDS_BLOCKINFO).find(L'\x00C2') == std::wstring::npos,
                        "resource compiler decodes the UTF-8 middle dot without mojibake");
                app.SetDocument(L"# Heading\r\n\r\nWords here.\nMixed tail.\r", {}, true);
                app.Prepare(256);
                const auto at = app.Document().GetText().find(L"Words");
                app.Document().SetSelection({at, at + 5});
                Require(app.positionStatus->GetText().find(Resource(IDS_EOLMIXED)) != std::wstring_view::npos,
                        "mixed CRLF/LF/CR source is reported truthfully in persistent status");
                const std::wstring counts(app.stats->GetText()), position(app.positionStatus->GetText());
                app.Status(IDS_FORMATTED);
                Require(app.stats->GetText() == counts && app.positionStatus->GetText() == position &&
                            app.feedback->GetText() == Resource(IDS_FORMATTED),
                        "transient feedback remains in the document footer instead of overwriting persistent document facts");
                app.Document().SetSelection({2, 2});
                app.SelectionChanged();
                Require(app.currentBlock->GetText().find(Resource(IDS_BLOCKHEADING)) != std::wstring_view::npos,
                        "Current block identifies the actual Markdown heading context in addition to its size");

                const auto expectBlock = [&](unsigned id, std::size_t characters, std::string_view message)
                {
                    const auto kind = Resource(id);
                    Require(app.currentBlock->GetText() == std::vformat(Resource(IDS_BLOCKINFO), std::make_wformat_args(kind, characters)), message);
                };
                app.SetDocument(L"Words here.\n\nMore prose.\n");
                app.Host().SetFocusControl(&app.Document(), false);
                app.Document().SetSelection({3, 3});
                app.Prepare(256);
                expectBlock(IDS_BLOCKPARAGRAPH, 11, "the paragraph inspector counts physical source characters independently of its syntax");
                const auto entrySource = app.Document().Snapshot();
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
                Require(app.Activity().currentBlockLabelConstructions == settledConstructions &&
                            app.Document().Snapshot().IsSameRevision(entrySource) && app.Document().GetSelection() == entrySelection &&
                            app.Document().CanUndo() == entryUndo && app.Document().CanRedo() == entryRedo &&
                            app.Host().GetFocusControl() == &app.Document(),
                        "equal View/Focus/syntax facts construct no localized label and preserve source, history and logical focus");
                Require(app.Document().ReplaceSelection(L"x"), "insert one source character in the current paragraph");
                expectBlock(IDS_BLOCKPARAGRAPH, 12, "a physical-line count change publishes immediately before preparation");
                Require(app.Activity().currentBlockLabelConstructions == settledConstructions + 1,
                        "a changed line count constructs the localized label exactly once");
                const auto syntaxUpdates = app.Activity().syntaxPresentationUpdates;
                app.Prepare(256);
                Require(app.Activity().currentBlockLabelConstructions == settledConstructions + 1 &&
                            app.Activity().syntaxPresentationUpdates > syntaxUpdates && app.Document().GetMarkdownSyntax() &&
                            app.Document().GetText() == L"Worxds here.\n\nMore prose.\n" &&
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
                Require(app.tools[2]->GetText() == Resource(IDS_PARAGRAPH) && app.tools[2]->GetAccessibleName() == Resource(IDS_HEADING),
                        "the visible toolbar caption follows paragraph context while its command name stays Heading");
                const auto previewChanges = app.Activity().changes;
                const auto previewUndo = app.Document().CanUndo(), previewRedo = app.Document().CanRedo();
                const auto paragraphConstructions = app.Activity().currentBlockLabelConstructions;
                DxUi::TextInputState entry;
                entry.text = L"plain\n";
                entry.multiline = true;
                entry.caretIndex = 2;
                auto preview = entry;
                preview.text = L"# one\n";
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
                Require(app.tools[3]->GetToggleState() == std::optional<bool>{true} && app.tools[5]->GetToggleState() == std::optional<bool>{false},
                        "returning to paragraph facts still publishes the exact strong formatting state");
                const auto strongConstructions = app.Activity().currentBlockLabelConstructions;
                preview.text = L"`one`\n";
                Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                        "import different inline syntax with unchanged paragraph label facts");
                app.Prepare(256);
                expectBlock(IDS_BLOCKPARAGRAPH, 5, "inline code retains the exact same paragraph label");
                Require(app.Activity().currentBlockLabelConstructions == strongConstructions &&
                            app.tools[3]->GetToggleState() == std::optional<bool>{false} &&
                            app.tools[5]->GetToggleState() == std::optional<bool>{true},
                        "equal label facts skip construction while unrelated formatting toggles still change");
                preview.text = L"$x+y$\n";
                Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                        "import an equal-length mathematical context at the same ACP");
                app.Prepare(512);
                expectBlock(IDS_MATH, 5, "the mathematical context overrides the paragraph's localized kind");
                Require(app.equationContext && app.equation->GetText() == L"\\[x+y\\]" && !app.tools[3]->IsEnabled() &&
                            app.tools[2]->GetText() == Resource(IDS_MATH) && app.tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                            app.Activity().selectionCaptionConstructions == paragraphCaptionConstructions + 1,
                        "equation source/action state and localized visible caption change once while the command name stays Heading");
                const auto firstMathCaptionConstructions = app.Activity().selectionCaptionConstructions;
                preview.text = L"  $x$\n";
                Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), preview, false),
                        "shift the mathematical source range without changing caret ACP or physical-line length");
                Require(!app.Document().GetMarkdownSyntax() && !app.equationContext && app.equation->GetText().empty() &&
                            app.tools[2]->GetText() == Resource(IDS_PARAGRAPH) &&
                            app.Activity().selectionCaptionConstructions == firstMathCaptionConstructions + 1,
                        "pending replacement syntax clears stale math and publishes the paragraph caption once");
                app.Prepare(512);
                expectBlock(IDS_MATH, 5, "a shifted same-ACP math range preserves the actual kind and count");
                Require(app.equationContext && app.equation->GetText() == L"\\[x\\]" && app.tools[2]->GetText() == Resource(IDS_MATH) &&
                            app.tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                            app.Activity().selectionCaptionConstructions == firstMathCaptionConstructions + 2,
                        "ready replacement syntax publishes its exact equation and restores the math caption once");
                const auto settledMathCaptionConstructions = app.Activity().selectionCaptionConstructions;
                app.SelectionChanged(false);
                app.Prepare(32);
                Require(app.Activity().selectionCaptionConstructions == settledMathCaptionConstructions && app.equationContext &&
                            app.equation->GetText() == L"\\[x\\]" && app.tools[2]->GetAccessibleName() == Resource(IDS_HEADING),
                        "repeating current ready math facts reuses the caption and preserves the exact equation and command name");
                Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), entry, false),
                        "cancel all transient previews back to the entry source");
                app.Prepare(256);
                expectBlock(IDS_BLOCKPARAGRAPH, 5, "preview cancellation restores the exact paragraph facts");
                Require(!app.equationContext && app.equation->GetText().empty() && app.tools[2]->GetText() == Resource(IDS_PARAGRAPH) &&
                            app.tools[2]->GetAccessibleName() == Resource(IDS_HEADING) &&
                            app.Activity().selectionCaptionConstructions == settledMathCaptionConstructions + 1 && app.tools[3]->IsEnabled() &&
                            app.tools[3]->GetToggleState() == std::optional<bool>{false} &&
                            app.tools[5]->GetToggleState() == std::optional<bool>{false} && app.Document().GetText() == entry.text &&
                            app.Document().GetSelection() == DxUi::RichText::Selection{2, 2} && app.Activity().changes == previewChanges &&
                            app.Document().CanUndo() == previewUndo && app.Document().CanRedo() == previewRedo &&
                            app.Host().GetFocusControl() == &app.Document(),
                        "preview/cancel refresh every dependent fact without committing source/history or changing logical focus");

                const auto checkBlockPositions = [&](std::wstring_view text,
                                                     std::initializer_list<std::tuple<std::size_t, unsigned, std::size_t>> positions,
                                                     bool outOfOrderLinks = false)
                {
                    app.SetDocument(std::wstring(text));
                    app.Document().SetSelection({text.size(), text.size()});
                    Require(app.Document().ReplaceSelection(L" ") && app.Document().Undo(),
                            "give each block-kind fixture a real retained redo transaction");
                    app.Prepare(512);
                    const auto syntax = app.Document().GetMarkdownSyntax();
                    Require(syntax && syntax->Source() == text && app.Document().CanRedo(),
                            "classify only the current ready canonical publication with retained history");
                    if (outOfOrderLinks)
                    {
                        bool backwardLink{};
                        const auto &nodes = syntax->Nodes();
                        for (std::size_t index = 0; index < nodes.size(); ++index)
                            backwardLink =
                                backwardLink || (nodes[index].nextSibling != DxUi::RichText::MarkdownNoNode && nodes[index].nextSibling < index);
                        Require(backwardLink, "reference extraction actually creates source-order links that differ from vector append order");
                    }
                    std::optional<std::pair<unsigned, std::size_t>> lastVisibleFacts;
                    const auto firstPosition = *positions.begin();
                    const auto firstFacts = std::pair{std::get<1>(firstPosition), std::get<2>(firstPosition)};
                    const auto initialConstructions = app.Activity().currentBlockLabelConstructions;
                    app.Document().SetSelection({std::get<0>(firstPosition), std::get<0>(firstPosition)});
                    app.SelectionChanged(false);
                    const auto initialLatestFacts = app.currentBlockLatestFacts;
                    Require(initialLatestFacts && initialLatestFacts.value().resourceId == firstFacts.first &&
                                initialLatestFacts.value().lineCharacterCount == firstFacts.second,
                            "the first independently specified source position matches retained canonical facts");
                    if (app.sidebar->IsVisible() && app.currentBlock->IsVisible())
                    {
                        expectBlock(firstFacts.first, firstFacts.second, "the first source-position oracle is published when its label is visible");
                        lastVisibleFacts = firstFacts;
                    }
                    else
                    {
                        Require(app.Activity().currentBlockLabelConstructions == initialConstructions,
                                "classifying the first hidden source position does not construct its current-block label");
                        if (!app.currentBlock->IsVisible())
                            Require(app.listSummary->IsVisible() && !app.listSummary->GetText().empty(),
                                    "the first hidden list position retains its visible nonempty list summary");
                    }
                    const auto snapshot = app.Document().Snapshot();
                    const auto changes = app.Activity().changes;
                    const auto canUndo = app.Document().CanUndo(), canRedo = app.Document().CanRedo();
                    for (const auto [caret, id, characters] : positions)
                    {
                        const auto constructions = app.Activity().currentBlockLabelConstructions;
                        app.Document().SetSelection({caret, caret});
                        app.SelectionChanged(false);
                        const auto facts = std::pair{id, characters};
                        const auto latestFacts = app.currentBlockLatestFacts;
                        Require(latestFacts && latestFacts.value().resourceId == id && latestFacts.value().lineCharacterCount == characters,
                                "the independently specified source position matches the retained canonical block kind and line count");
                        const bool labelPublished = app.sidebar->IsVisible() && app.currentBlock->IsVisible();
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
                            if (!app.currentBlock->IsVisible())
                                Require(app.listSummary->IsVisible() && !app.listSummary->GetText().empty(),
                                        "a locally hidden list block retains its visible nonempty list summary");
                        }
                        Require(app.Document().Snapshot().IsSameRevision(snapshot) &&
                                    app.Document().GetSelection() == DxUi::RichText::Selection{caret, caret} && app.Document().CanUndo() == canUndo &&
                                    app.Document().CanRedo() == canRedo && app.Activity().changes == changes &&
                                    app.Host().GetFocusControl() == &app.Document(),
                                "block lookup never rewrites source, ACP selection, committed history or logical focus");
                    }
                    Require(app.Document().Redo() && app.Document().GetText() == std::wstring(text) + L" " && app.Document().Undo() &&
                                app.Document().GetText() == text,
                            "the exact source transaction remains redoable and undoable after every classification query");
                };
                checkBlockPositions(L"> - [x] task\n>   - child\n\nOutside\n", {{0, IDS_BLOCKLIST, 12},
                                                                                {2, IDS_BLOCKLIST, 12},
                                                                                {4, IDS_BLOCKLIST, 12},
                                                                                {13, IDS_BLOCKLIST, 11},
                                                                                {19, IDS_BLOCKLIST, 11},
                                                                                {26, IDS_BLOCKPARAGRAPH, 7},
                                                                                {34, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"> # Head\n> words\n", {{0, IDS_BLOCKHEADING, 8},
                                                             {2, IDS_BLOCKHEADING, 8},
                                                             {8, IDS_BLOCKHEADING, 8},
                                                             {9, IDS_BLOCKQUOTE, 7},
                                                             {11, IDS_BLOCKQUOTE, 7},
                                                             {17, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"> ```\n> code\n> ```\n", {{0, IDS_BLOCKCODE, 5},
                                                                {5, IDS_BLOCKCODE, 5},
                                                                {6, IDS_BLOCKCODE, 6},
                                                                {8, IDS_BLOCKCODE, 6},
                                                                {13, IDS_BLOCKCODE, 5},
                                                                {19, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"> Title\n> =====\n", {{2, IDS_BLOCKHEADING, 7}, {10, IDS_BLOCKHEADING, 7}, {16, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L">     code\n", {{0, IDS_BLOCKCODE, 10}, {6, IDS_BLOCKCODE, 10}, {11, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"> - # Head\n", {{0, IDS_BLOCKHEADING, 10}, {6, IDS_BLOCKHEADING, 10}, {11, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"| A | B |\n| - | - |\n| x | y |\n", {{0, IDS_BLOCKTABLE, 9},
                                                                           {2, IDS_BLOCKTABLE, 9},
                                                                           {10, IDS_BLOCKTABLE, 9},
                                                                           {12, IDS_BLOCKTABLE, 9},
                                                                           {20, IDS_BLOCKTABLE, 9},
                                                                           {22, IDS_BLOCKTABLE, 9},
                                                                           {26, IDS_BLOCKTABLE, 9},
                                                                           {29, IDS_BLOCKTABLE, 9},
                                                                           {30, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"> | A | B |\n> | - | - |\n> | x | y |\n", {{0, IDS_BLOCKTABLE, 11},
                                                                                 {4, IDS_BLOCKTABLE, 11},
                                                                                 {12, IDS_BLOCKTABLE, 11},
                                                                                 {28, IDS_BLOCKTABLE, 11},
                                                                                 {36, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"***", {{0, IDS_BLOCKDIVIDER, 3}, {3, IDS_BLOCKDIVIDER, 3}});
                checkBlockPositions(L"<div>raw</div>", {{0, IDS_BLOCKHTML, 14}, {5, IDS_BLOCKHTML, 14}, {14, IDS_BLOCKHTML, 14}});
                checkBlockPositions(L"[ref]: /target\n", {{0, IDS_BLOCKLINKDEFINITION, 14},
                                                          {7, IDS_BLOCKLINKDEFINITION, 14},
                                                          {14, IDS_BLOCKLINKDEFINITION, 14},
                                                          {15, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"---\ntitle: draft\n---\n",
                                    {{0, IDS_BLOCKMETADATA, 3}, {4, IDS_BLOCKMETADATA, 12}, {17, IDS_BLOCKMETADATA, 3}, {21, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(
                    L"[^note]: source\n\nUse [^note].\n",
                    {{0, IDS_BLOCKFOOTNOTE, 15}, {9, IDS_BLOCKFOOTNOTE, 15}, {17, IDS_BLOCKPARAGRAPH, 12}, {30, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(
                    L"> [ref]: /target\n> words\n",
                    {{0, IDS_BLOCKLINKDEFINITION, 16}, {2, IDS_BLOCKLINKDEFINITION, 16}, {19, IDS_BLOCKQUOTE, 7}, {25, IDS_BLOCKPARAGRAPH, 0}}, true);
                checkBlockPositions(L"", {{0, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"# End", {{0, IDS_BLOCKHEADING, 5}, {5, IDS_BLOCKHEADING, 5}});
                checkBlockPositions(L"# End\n", {{5, IDS_BLOCKHEADING, 5}, {6, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"# End\r\n", {{5, IDS_BLOCKHEADING, 5}, {7, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"# End\r", {{5, IDS_BLOCKHEADING, 5}, {6, IDS_BLOCKPARAGRAPH, 0}});
                checkBlockPositions(L"```\nx\n```", {{4, IDS_BLOCKCODE, 1}, {9, IDS_BLOCKCODE, 3}});

                app.SetDocument(L"> # Head\n");
                app.Document().SetSelection({4, 4});
                Require(!app.Document().GetMarkdownSyntax(), "the replaced source genuinely awaits its new canonical syntax");
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
                nestedEntry.text = L"> plain\n";
                nestedEntry.multiline = true;
                nestedEntry.caretIndex = 4;
                auto nestedPreview = nestedEntry;
                nestedPreview.text = L"> - one\n";
                const auto beforeNestedListConstructions = app.Activity().currentBlockLabelConstructions;
                Require((app.Document().*Testing::TextServiceImportProbe::Import)(app.Host(), nestedPreview, false),
                        "preview a nested list item at the same ACP and physical-line length");
                expectBlock(IDS_BLOCKPARAGRAPH, 7, "the pending nested-list preview publishes its visible paragraph facts");
                Require(app.Activity().currentBlockLabelConstructions == beforeNestedListConstructions + 1,
                        "invalidating the ready quote publishes the different visible pending kind once");
                const auto pendingNestedListConstructions = app.Activity().currentBlockLabelConstructions;
                app.Prepare(512);
                Require(app.currentBlockLatestFacts && app.currentBlockLatestFacts.value().resourceId == IDS_BLOCKLIST &&
                            app.currentBlockLatestFacts.value().lineCharacterCount == 7,
                        "ready nested-list syntax retains its literal source kind and physical-line count");
                Require(!app.currentBlock->IsVisible() && app.listSummary->IsVisible() && !app.listSummary->GetText().empty(),
                        "the visible nested-list summary replaces its hidden current-block label");
                Require(app.Activity().currentBlockLabelConstructions == pendingNestedListConstructions,
                        "ready nested-list syntax does not construct the now-hidden current-block label");
                const auto nestedConstructions = app.Activity().currentBlockLabelConstructions;
                nestedPreview.text = L"> # one\n";
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
                            app.Activity().changes == nestedChanges && app.Document().CanUndo() == nestedUndo &&
                            app.Document().CanRedo() == nestedRedo,
                        "nested-kind derivation and cache invalidation preserve exact noncommitting preview history");

                app.SetDocument(L"```text\ncode\n```\n");
                app.Document().SetSelection({8, 8});
                app.Prepare(512);
                expectBlock(IDS_BLOCKCODE, 4, "the prepared fenced source begins as a code block");
                const auto regionSource = app.Document().Snapshot();
                const auto regionSelection = app.Document().GetSelection();
                const auto regionUndo = app.Document().CanUndo(), regionRedo = app.Document().CanRedo();
                app.Execute(Command::LocalSource);
                Require(app.Document().GetSemanticSourceRegion().has_value() && !app.HasBlockDraft(),
                        "F2 enters the canonical in-document source region rather than an app-local parser or draft");
                app.Prepare(512);
                expectBlock(IDS_BLOCKSOURCE, 4, "the reachable SourceEditor node has its localized source-block category");
                app.Execute(Command::LocalSource);
                app.Prepare(512);
                expectBlock(IDS_BLOCKCODE, 4, "leaving the source region restores the actual fenced-code category");
                Require(!app.Document().GetSemanticSourceRegion() && app.Document().Snapshot().IsSameRevision(regionSource) &&
                            app.Document().GetSelection() == regionSelection && app.Document().CanUndo() == regionUndo &&
                            app.Document().CanRedo() == regionRedo && app.Host().GetFocusControl() == &app.Document(),
                        "source-editor entry/exit changes presentation only and preserves exact source, selection, history and focus");
            });
    }
};
void RegisterDesignRefreshTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    DesignRefreshProbe::Register(root, test);
}
} // namespace RedTilde
