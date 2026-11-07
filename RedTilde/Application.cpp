#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "res/Resource.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <DxUi/TextInputServices.h>
#include <algorithm>
#include <chrono>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <format>
#include <fstream>
#include <mutex>
#include <new>
#include <psapi.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <thread>
#include <type_traits>
#include <wil/result.h>
#include <wincodec.h>
#include <windowsx.h>
#include <winhttp.h>

namespace RedTilde
{
static_assert(std::is_nothrow_swappable_v<DocumentInfo>, "analysis facts must publish without throwing");
namespace
{
// App-local display categories over the canonical immutable tree, not a parser.
unsigned CurrentBlockResourceAt(const DxUi::RichText::MarkdownSyntax &syntax, std::size_t caret,
                                std::optional<std::pair<unsigned, std::size_t>> *listSummary = nullptr) noexcept
{
    using namespace DxUi::RichText;
    const auto source = syntax.Source();
    const auto &nodes = syntax.Nodes();
    unsigned resource = IDS_BLOCKPARAGRAPH;
    if (nodes.empty() || source.empty())
        return resource;
    auto position = std::min(caret, source.size());
    if (position == source.size())
    {
        // A final unterminated line retains its block at the insertion ACP.
        // A real trailing terminator instead exposes the new blank writing line.
        if (source.back() == L'\r' || source.back() == L'\n')
            return resource;
        --position;
    }
    // Published child/sibling links are acyclic and source-ordered. Visit only
    // preceding siblings and the containing path; vector append order is not source order.
    auto index = nodes[0].firstChild;
    while (index != MarkdownNoNode)
    {
        const auto &node = nodes[index];
        if (position < node.block.source.first)
            break;
        if (position >= node.block.source.last)
        {
            index = node.nextSibling;
            continue;
        }
        switch (node.block.kind)
        {
        case MarkdownBlockKind::AtxHeading:
        case MarkdownBlockKind::SetextHeading:
            resource = IDS_BLOCKHEADING;
            break;
        case MarkdownBlockKind::FencedCode:
        case MarkdownBlockKind::IndentedCode:
            resource = IDS_BLOCKCODE;
            break;
        case MarkdownBlockKind::List:
            resource = IDS_BLOCKLISTCONTAINER;
            if (listSummary)
            {
                std::size_t count{};
                unsigned kind = IDS_BLOCKBULLETLIST;
                for (auto child = node.firstChild; child != MarkdownNoNode; child = nodes[child].nextSibling)
                    if (nodes[child].block.kind == MarkdownBlockKind::ListItem)
                    {
                        const auto marker = nodes[child].block.info.first;
                        if (marker < source.size() && source[marker] >= L'0' && source[marker] <= L'9')
                            kind = IDS_BLOCKNUMBEREDLIST;
                        ++count;
                    }
                *listSummary = std::pair{kind, count};
            }
            break;
        case MarkdownBlockKind::ListItem:
            resource = IDS_BLOCKLIST;
            break;
        case MarkdownBlockKind::Table:
        case MarkdownBlockKind::TableRow:
        case MarkdownBlockKind::TableCell:
            resource = IDS_BLOCKTABLE;
            break;
        case MarkdownBlockKind::BlockQuote:
            resource = IDS_BLOCKQUOTE;
            break;
        case MarkdownBlockKind::ThematicBreak:
            resource = IDS_BLOCKDIVIDER;
            break;
        case MarkdownBlockKind::HtmlBlock:
            resource = IDS_BLOCKHTML;
            break;
        case MarkdownBlockKind::ReferenceDefinition:
            resource = IDS_BLOCKLINKDEFINITION;
            break;
        case MarkdownBlockKind::FrontMatter:
            resource = IDS_BLOCKMETADATA;
            break;
        case MarkdownBlockKind::FootnoteDefinition:
            resource = IDS_BLOCKFOOTNOTE;
            break;
        case MarkdownBlockKind::SourceEditor:
            resource = IDS_BLOCKSOURCE;
            break;
        case MarkdownBlockKind::Document:
        case MarkdownBlockKind::Paragraph:
        case MarkdownBlockKind::BlankLine:
            // Ordinary leaf prose retains a recognized list/quote ancestor.
            break;
        }
        index = node.firstChild;
    }
    return resource;
}
// A display math block owns its closing line terminator. Caret tests and drafts end
// at the closing delimiter, so the following line is not inside the equation.
std::size_t WithoutLineEnd(std::wstring_view source, DxUi::RichText::MarkdownRange range) noexcept
{
    auto last = std::min(range.last, source.size());
    while (last > range.first && (source[last - 1] == L'\n' || source[last - 1] == L'\r'))
        --last;
    return last;
}
// The live TeX inside an in-place math source region, without its delimiters. Other
// regions, or an envelope whose delimiters are being edited, yield no expression.
std::wstring MathRegionExpression(const DxUi::RichText::MarkdownSourceRegion &region, std::wstring_view envelope)
{
    using DxUi::RichText::MarkdownSourceRegionKind;
    const auto trim = [](std::wstring_view text)
    {
        while (!text.empty() && (text.front() == L'\n' || text.front() == L'\r'))
            text.remove_prefix(1);
        while (!text.empty() && (text.back() == L'\n' || text.back() == L'\r'))
            text.remove_suffix(1);
        return text;
    };
    envelope = trim(envelope);
    const auto inner = [&](std::wstring_view open, std::wstring_view close)
    {
        return envelope.size() > open.size() + close.size() && envelope.starts_with(open) && envelope.ends_with(close)
                   ? trim(envelope.substr(open.size(), envelope.size() - open.size() - close.size()))
                   : std::wstring_view{};
    };
    std::wstring_view expression;
    if (region.kind == MarkdownSourceRegionKind::Inline && region.inlineKind == DxUi::RichText::MarkdownInlineSourceKind::Math)
        expression = envelope.starts_with(L"\\(") ? inner(L"\\(", L"\\)") : inner(L"$", L"$");
    else if (region.kind == MarkdownSourceRegionKind::Block)
        expression = envelope.starts_with(L"$$") ? inner(L"$$", L"$$") : envelope.starts_with(L"\\[") ? inner(L"\\[", L"\\]") : std::wstring_view{};
    return expression.size() <= DxUi::RichText::MathMaximumSourceUnits ? std::wstring(expression) : std::wstring{};
}
// GFM stays on; each optional RichMarkdown family follows its own preference.
DxUi::RichText::MarkdownExtensions MarkdownExtensionsFor(const Settings &settings) noexcept
{
    using DxUi::RichText::MarkdownExtensions;
    constexpr std::array<MarkdownExtensions, MarkdownSyntaxCount> flags{
        MarkdownExtensions::Highlight, MarkdownExtensions::Superscript,     MarkdownExtensions::Subscript,    MarkdownExtensions::Inserted,
        MarkdownExtensions::WikiLinks, MarkdownExtensions::DefinitionLists, MarkdownExtensions::Abbreviations};
    auto extensions = MarkdownExtensions::All;
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        if (settings.markdownSyntax[index])
            extensions = extensions | flags[index];
    return extensions;
}
} // namespace

struct Application::ExternalLinkState
{
    explicit ExternalLinkState(HWND owner) : window(owner) {}
    ExternalLinkState(const ExternalLinkState &) = delete;
    ExternalLinkState &operator=(const ExternalLinkState &) = delete;
    ExternalLinkState(ExternalLinkState &&) = delete;
    ExternalLinkState &operator=(ExternalLinkState &&) = delete;
    std::mutex mutex;
    HWND window{};
    bool busy{};
    std::optional<HRESULT> completed;
};
struct Application::ExternalLinkJob
{
    ExternalLinkJob() = default;
    ExternalLinkJob(const ExternalLinkJob &) = delete;
    ExternalLinkJob &operator=(const ExternalLinkJob &) = delete;
    ExternalLinkJob(ExternalLinkJob &&) = delete;
    ExternalLinkJob &operator=(ExternalLinkJob &&) = delete;
    ProcessWorkLease lease;
    std::shared_ptr<ExternalLinkState> state;
    std::wstring destination;
#if defined(REDTILDE_ENABLE_TESTS)
    std::function<HRESULT(std::wstring_view)> launchForTest;
    std::shared_ptr<AsyncFiles::TestGate> gate;
#endif
};

std::wstring_view ResourceView(unsigned id)
{
    const wchar_t *value{};
    const int length = LoadStringW(GetModuleHandleW(nullptr), id, reinterpret_cast<LPWSTR>(&value), 0);
    // The executable owns this immutable resource storage for the process lifetime.
    // Resolve each request so the calling thread's resource language still applies.
    return length > 0 ? std::wstring_view(value, static_cast<std::size_t>(length)) : std::wstring_view{};
}
std::wstring Resource(unsigned id)
{
    return std::wstring(ResourceView(id));
}

std::wstring SampleDocument()
{
    return L"# A little room for big ideas\n\n"
           L"Good writing starts with a clear space. **RedTilde** keeps your words in flow, with the precision of Markdown just a shortcut away.\n\n"
           L"## Make it yours\n\n"
           L"Select a few words to reveal your text tools. Switch to **Source** whenever you want to see exactly what is on the page. Your selection "
           L"and undo history stay with you.\n\n"
           L"- [x] A beautiful place to write\n- [x] Markdown you can trust\n- [ ] Your next great idea\n\n"
           L"## Show the thinking\n\n"
           L"```mermaid\ngraph LR\nA[An idea] --> B[Find the words]\nB --> C[Make it clear]\n```\n\n"
           L"Mathematics belongs in your document. Inline $E=mc^2$ and display equations stay source-exact:\n\n"
           L"$$\nx = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}\n$$\n\n"
           L"## Keep the details\n\n"
           L"| View | Best for |\n| --- | --- |\n| Compose | Staying in your writing |\n| Source | Every Markdown detail |\n| Split | Both at a "
           L"glance |\n\n"
           L"> Simple tools. Thoughtful defaults. Your files stay yours.\n\n"
           L"Images use ordinary Markdown: `![description](images/photo.png)`. Insert a local image from the toolbar or paste a file path.\n";
}

Application::Application(LaunchOptions value) : Application(std::move(value), DxUi::Failure::Scope{}) {}
Application::Application(LaunchOptions value, const DxUi::Failure::Scope &) : options(std::move(value))
{
    startupPolicy = !options.headless;
#if defined(REDTILDE_ENABLE_TESTS)
    startupPolicy = startupPolicy || options.startupRestoreForTest;
#endif
}
Application::~Application()
{
    closing = true;
    bitmapWriter.Stop();
    bitmapWriter.Detach();
    CloseExternalLinks();
    preferenceWriter.Close();
    recovery.Close();
    files.Close();
    folderFiles.Close();
    images.Shutdown();
    imageFiles.Close();
    host.Detach();
    window.reset();
}

HRESULT Application::Initialize()
{
    if (options.dataDirectory.empty())
        options.dataDirectory = DefaultDataDirectory();
    if (startupPolicy || options.preferencesProbe)
        (void)LoadSettings(options.dataDirectory, settings);
    WNDCLASSEXW cls{sizeof(cls)};
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon = LoadIconW(cls.hInstance, MAKEINTRESOURCEW(IDI_REDTILDE));
    cls.hIconSm = cls.hIcon;
    cls.lpszClassName = L"RedTilde.MainWindow";
    if (!RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    const UINT dpi = GetDpiForSystem();
    RECT bounds{0, 0, MulDiv(options.scenario == L"narrow" ? 820 : 1360, static_cast<int>(dpi), 96), MulDiv(900, static_cast<int>(dpi), 96)};
    (void)AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    RECT requested = bounds;
    unsigned savedDpi = dpi;
    RECT workArea{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (!options.headless)
    {
        if (settings.windowPlacement)
        {
            requested = settings.windowPlacement.value().normalBounds;
            savedDpi = settings.windowPlacement.value().dpi;
        }
        MONITORINFO monitor{sizeof(monitor)};
        const HMONITOR target =
            settings.windowPlacement ? MonitorFromRect(&requested, MONITOR_DEFAULTTONEAREST) : MonitorFromPoint(POINT{}, MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(target, &monitor))
            workArea = monitor.rcWork;
        if (!settings.windowPlacement)
        {
            const LONG width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
            requested = {workArea.left + (workArea.right - workArea.left - width) / 2, workArea.top + (workArea.bottom - workArea.top - height) / 2,
                         0, 0};
            requested.right = requested.left + width;
            requested.bottom = requested.top + height;
        }
        bounds = ClampWindowBounds(requested, workArea, savedDpi, dpi);
    }
    window.reset(CreateWindowExW(options.headless ? WS_EX_NOACTIVATE : 0, cls.lpszClassName, Resource(IDS_APP).c_str(), WS_OVERLAPPEDWINDOW,
                                 options.headless ? CW_USEDEFAULT : bounds.left, options.headless ? CW_USEDEFAULT : bounds.top,
                                 bounds.right - bounds.left, bounds.bottom - bounds.top, nullptr, nullptr, cls.hInstance, this));
    if (!window)
        return HRESULT_FROM_WIN32(GetLastError());
    if (!options.headless)
    {
        const unsigned actualDpi = GetDpiForWindow(window.get());
        bounds = ClampWindowBounds(requested, workArea, savedDpi, actualDpi);
        if (!SetWindowPos(window.get(), nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
                          SWP_NOZORDER | SWP_NOACTIVATE))
            return HRESULT_FROM_WIN32(GetLastError());
        const bool maximized = settings.windowPlacement && settings.windowPlacement.value().maximized;
        settings.windowPlacement = WindowPlacementSettings{bounds, actualDpi, maximized};
        windowPlacementDirty = true;
    }
    if (!host.Attach(window.get(), {.forceWarp = options.forceWarp, .batchInitialRootChildren = true}))
        return HRESULT_FROM_WIN32(GetLastError());
    files.Initialize(window.get(), FileReady);
#if defined(REDTILDE_ENABLE_TESTS)
    files.SetTestGate(options.fileGateForTest);
#endif
    const MARGINS frame{1, 1, 1, 1};
    (void)DwmExtendFrameIntoClientArea(window.get(), &frame);
    SetWindowPos(window.get(), nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    preferenceWriter.Initialize(options.dataDirectory, window.get(), PreferencesReady);
    RETURN_IF_FAILED(recovery.Initialize(options.dataDirectory, window.get(), RecoveryReadyMessage));
    RETURN_IF_FAILED(images.Initialize(window.get(), ImageReady, 0x5254));
    RETURN_IF_FAILED(imageFiles.Initialize(window.get(), ImageFileChanged, 0x5254));
    images.SetAllowLocalNetwork(settings.allowLocalNetworkImages);
    RETURN_IF_FAILED(bitmapWriter.Initialize(options.dataDirectory, window.get(), BitmapReady));
    host.SetClipboardPreference(settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
    initialized = true;
    initializing = true;
    // BuildUi is the sole creation site for the label whose localized facts we cache.
    currentBlockLatestFacts.reset();
    currentBlockPublishedFacts.reset();
    RETURN_IF_FAILED(BuildUi());
    ApplyTheme();
    SetDocument(startupPolicy ? std::wstring{} : SampleDocument());
    if (startupPolicy && options.document.empty())
    {
        startupSource = editor->Snapshot();
        startupPending = true;
    }
    if (!options.document.empty())
    {
        if (options.headless && !startupPolicy)
        {
            StoredDocument loaded;
            RETURN_IF_FAILED(LoadDocument(options.document, loaded));
            SetDocument(std::move(loaded.text), options.document, loaded.bom, loaded.stamp);
        }
        else
        {
            openingSnapshot = editor->Snapshot();
            RETURN_IF_FAILED(files.Submit(false, options.document));
            Status(IDS_LOADING);
        }
    }
    initializing = false;
    Layout();
    if (!options.headless && FAILED(RegisterApplicationRestart(L"", RESTART_NO_CRASH | RESTART_NO_HANG)))
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Windows restart registration unavailable; ordinary recovery remains enabled");
    return S_OK;
}

RECT Application::ClampWindowBounds(RECT requested, RECT workArea, unsigned savedDpi, unsigned currentDpi) noexcept
{
    // Application policy uses screen coordinates throughout. Never feed
    // WINDOWPLACEMENT's taskbar-relative workspace coordinates to SetWindowPos.
    if (workArea.right <= workArea.left || workArea.bottom <= workArea.top)
        return {0, 0, 1, 1};
    if (savedDpi < 48u || savedDpi > 768u)
        savedDpi = 96;
    if (currentDpi < 48u || currentDpi > 768u)
        currentDpi = 96;
    const auto workWidth = static_cast<long long>(workArea.right) - workArea.left;
    const auto workHeight = static_cast<long long>(workArea.bottom) - workArea.top;
    const auto scaled = [&](long long size) { return (std::clamp(size, 1ll, 32768ll) * currentDpi + savedDpi / 2u) / savedDpi; };
    const auto minimumWidth = std::min(workWidth, static_cast<long long>(800u) * currentDpi / 96u);
    const auto minimumHeight = std::min(workHeight, static_cast<long long>(560u) * currentDpi / 96u);
    const auto width = std::clamp(scaled(static_cast<long long>(requested.right) - requested.left), minimumWidth, workWidth);
    const auto height = std::clamp(scaled(static_cast<long long>(requested.bottom) - requested.top), minimumHeight, workHeight);
    const auto left =
        std::clamp(static_cast<long long>(requested.left), static_cast<long long>(workArea.left), static_cast<long long>(workArea.right) - width);
    const auto top =
        std::clamp(static_cast<long long>(requested.top), static_cast<long long>(workArea.top), static_cast<long long>(workArea.bottom) - height);
    return {static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(left + width), static_cast<LONG>(top + height)};
}

int Application::InitialShowCommand() const noexcept
{
    if (options.explicitShowCommand)
        return options.showCommand;
    return settings.windowPlacement && settings.windowPlacement.value().maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
}

bool Application::MayRequestNativeFocus() const noexcept
{
    return !options.headless && window && IsWindowVisible(window.get()) && !IsIconic(window.get()) && GetForegroundWindow() == window.get();
}

void Application::RecordWindowPlacement(bool persist)
{
    if (!initialized || initializing || options.headless || closing || !window || IsIconic(window.get()))
        return;
    auto placement = settings.windowPlacement.value_or(WindowPlacementSettings{});
    placement.maximized = IsZoomed(window.get()) != FALSE;
    if (!placement.maximized)
    {
        if (!GetWindowRect(window.get(), &placement.normalBounds))
            return;
        placement.dpi = GetDpiForWindow(window.get());
    }
    if (!ValidWindowPlacement(placement))
        return;
    const auto previous = settings.windowPlacement;
    if (!previous || !EqualRect(&previous.value().normalBounds, &placement.normalBounds) || previous.value().dpi != placement.dpi ||
        previous.value().maximized != placement.maximized)
    {
        settings.windowPlacement = placement;
        windowPlacementDirty = true;
    }
    if (persist && windowPlacementDirty)
        SavePreferences();
}

void Application::AbandonStartupRestore()
{
    startupPending = false;
    startupSource.reset();
    if (!std::exchange(startupOpening, false))
        return;
    // Only this one automatic read is abandoned. Its worker retains its old
    // closed notification state; no filesystem wait or desktop interaction occurs.
    files.Close();
    files.Initialize(window.get(), FileReady);
#if defined(REDTILDE_ENABLE_TESTS)
    files.SetTestGate(options.fileGateForTest);
#endif
    heldOpen.reset();
    openingSnapshot.reset();
    openingFromFiles = false;
    openingApprovals.clear();
}

void Application::RememberActiveDocument()
{
    if (initializing || closing)
        return;
    const auto path = recovered ? std::filesystem::path{} : documentPath;
    if (settings.lastDocument != path)
    {
        settings.lastDocument = path;
        SavePreferences();
    }
}

bool Application::SupportedExternalLink(std::wstring_view destination) noexcept
{
    // Application-owned shell policy, not another Markdown/URL parser. DxUi
    // supplies the resolved destination; executable/file/custom schemes stay inert.
    if (destination.empty() || destination.size() > DxUi::RichText::Document::MaximumUnits ||
        std::ranges::any_of(destination, [](wchar_t unit) { return unit <= L' ' || unit == 0x7f || unit == L'"'; }))
        return false;
    const auto starts = [&](std::wstring_view prefix)
    {
        return destination.size() >= prefix.size() &&
               std::ranges::all_of(destination.substr(0, prefix.size()), [](wchar_t unit) { return unit < 0x80; }) &&
               CompareStringOrdinal(destination.data(), static_cast<int>(prefix.size()), prefix.data(), static_cast<int>(prefix.size()), TRUE) ==
                   CSTR_EQUAL;
    };
    if (starts(L"mailto:"))
        return destination.size() > 7u;
    const std::size_t authority = starts(L"https://") ? 8u : starts(L"http://") ? 7u : 0u;
    return authority != 0u && destination.size() > authority && destination.find_first_of(L"/?#", authority) != authority;
}

std::optional<std::size_t> Application::CurrentLinkElement(const std::shared_ptr<const DxUi::RichText::SemanticDocument> &document) const
{
    if (!document || !editor->Snapshot().MatchesText(document->Syntax()->Source()))
        return std::nullopt;
    const auto selection = editor->GetSelection();
    const auto elements = document->Elements();
    for (std::size_t index = 0; index < elements.size(); ++index)
    {
        const auto &element = elements[index];
        if (element.role == DxUi::RichText::SemanticRole::Link && !element.wiki && selection.Start() >= element.source.first &&
            selection.Start() < element.source.last && selection.End() <= element.source.last)
            return index;
    }
    return std::nullopt;
}

void Application::RequestOpenLink(const DxUi::RichTextLinkActivation &activation)
{
    if (closing || !root || host.GetRoot() != root)
        return;
    if (closingAfterSave || sessionEnding || switchingDocument || linkPromptOpen || HasBlockDraft() || activation.wiki ||
        !SupportedExternalLink(activation.destination))
    {
        Status(IDS_LINKUNAVAILABLE);
        return;
    }
    const auto semantic = editor->GetSemanticDocument();
    const auto source = editor->Snapshot();
    if (!semantic || !source.MatchesText(semantic->Syntax()->Source()) ||
        std::ranges::none_of(semantic->Elements(),
                             [&](const auto &element)
                             {
                                 return element.role == DxUi::RichText::SemanticRole::Link && element.source == activation.source &&
                                        element.destination == activation.destination && !element.wiki;
                             }))
    {
        Status(IDS_LINKCHANGED);
        return;
    }
    if (externalLink)
    {
        std::lock_guard lock(externalLink->mutex);
        if (externalLink->busy)
        {
            // No UI callback or status publication while holding a worker lock.
            return;
        }
    }
    std::wstring destination(activation.destination); // Own every byte before the modal/reentrant boundary.
    const auto selection = editor->GetSelection();
    const auto key = activeDocumentKey;
    auto *const owner = root;
    auto *const primary = editor;
    const auto lifetime = GetControlLifetimeToken(*primary);
    bool approved{};
    linkPromptOpen = true;
    const auto unwind = wil::scope_exit([&] { linkPromptOpen = false; });
#if defined(REDTILDE_ENABLE_TESTS)
    const auto testLaunch = options.launchLinkForTest;
#endif
    if (options.headless)
    {
#if defined(REDTILDE_ENABLE_TESTS)
        const auto confirm = options.confirmLinkForTest;
        if (!confirm || !testLaunch)
            return;
        approved = confirm(destination);
#else
        return;
#endif
    }
    else
    {
        const auto message = std::vformat(Resource(IDS_OPENLINKCONFIRM), std::make_wformat_args(destination));
        approved = DxUi::Failure::InvokeNative(&MessageBoxW, window.get(), message.c_str(), Resource(IDS_OPENLINK).c_str(),
                                               static_cast<UINT>(MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)) == IDYES;
    }
    if (!approved)
        return;
    if (closing || lifetime.expired() || host.GetRoot() != owner || editor != primary)
        return;
    if (sessionEnding || activeDocumentKey != key || !primary->Snapshot().IsSameRevision(source) || primary->GetSelection() != selection)
    {
        Status(IDS_LINKCHANGED);
        return;
    }
    if (!externalLink)
        externalLink = std::make_shared<ExternalLinkState>(window.get());
    auto job = std::make_unique<ExternalLinkJob>();
    job->state = externalLink;
    job->destination = std::move(destination);
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless)
    {
        job->launchForTest = testLaunch;
        job->gate = options.linkGateForTest;
    }
#endif
    HRESULT admitted = S_OK;
    admitted = ProcessWork::Acquire(job->lease);
    if (FAILED(admitted))
    {
        Status(IDS_LINKFAILED);
        return;
    }
    {
        std::lock_guard lock(externalLink->mutex);
        if (externalLink->busy || !externalLink->window)
            admitted = HRESULT_FROM_WIN32(ERROR_BUSY);
        else
        {
            externalLink->busy = true;
            externalLink->completed.reset();
            // A fresh short-lived STA is required by shell extensions. Unlike
            // the MTA codec/file workers it cannot borrow an arbitrary pool apartment.
            // The EXE owns only value/shared-state data; Close invalidates posting
            // without waiting for another application's shell handler.
            wil::unique_handle thread(CreateThread(nullptr, 0, ExternalLinkWorker, job.get(), 0, nullptr));
            if (!thread)
            {
                const DWORD error = GetLastError();
                admitted = HRESULT_FROM_WIN32(error ? error : ERROR_NOT_ENOUGH_MEMORY);
                externalLink->busy = false;
            }
            else
                (void)job.release();
        }
    }
    Status(FAILED(admitted) ? IDS_LINKFAILED : IDS_LINKOPENING);
}

DWORD WINAPI Application::ExternalLinkWorker(void *context) noexcept
{
    ProcessWorkLease lease = std::move(static_cast<ExternalLinkJob *>(context)->lease);
    const std::unique_ptr<ExternalLinkJob> job(static_cast<ExternalLinkJob *>(context));
#if defined(REDTILDE_ENABLE_TESTS)
    const auto settled = wil::scope_exit(
        [&]
        {
            if (job->gate)
                (void)SetEvent(job->gate->finished.get());
        });
#endif
    HRESULT result = E_FAIL;
    try
    {
#if defined(REDTILDE_ENABLE_TESTS)
        bool admitted = true;
        if (job->gate)
        {
            (void)SetEvent(job->gate->entered.get());
            admitted = WaitForSingleObject(job->gate->proceed.get(), 30000) == WAIT_OBJECT_0;
        }
        if (!admitted)
            result = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        else
#endif
        {
            result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(result))
            {
                const wil::unique_couninitialize_call apartment;
                const DxUi::Failure::Scope failureScope;
#if defined(REDTILDE_ENABLE_TESTS)
                if (job->launchForTest)
                    result = job->launchForTest(job->destination);
                else
#endif
                {
                    SHELLEXECUTEINFOW launch{sizeof(launch)};
                    launch.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
                    launch.lpVerb = L"open";
                    launch.lpFile = job->destination.c_str();
                    launch.nShow = SW_SHOWNORMAL;
                    // No process handle is requested or retained.
                    if (DxUi::Failure::InvokeNative(&ShellExecuteExW, &launch))
                        result = S_OK;
                    else
                    {
                        const DWORD error = GetLastError();
                        result = HRESULT_FROM_WIN32(error ? error : ERROR_OPEN_FAILED);
                    }
                }
            }
        }
    }
    catch (const std::bad_alloc &)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::exception &)
    {
        // Unexpected failures cannot unwind through the native thread entry or resume a partial callback.
        DxUi::Failure::FailUnexpected();
    }
    std::lock_guard lock(job->state->mutex);
    job->state->busy = false;
    job->state->completed = result;
    if (job->state->window)
        (void)PostMessageW(job->state->window, ExternalLinkReady, 0, 0);
    return 0;
}

void Application::ExternalLinkCompleted()
{
    if (!externalLink)
        return;
    std::optional<HRESULT> completed;
    {
        std::lock_guard lock(externalLink->mutex);
        completed = std::exchange(externalLink->completed, std::nullopt);
    }
    if (completed && !closing && root && host.GetRoot() == root)
        Status(SUCCEEDED(completed.value()) ? IDS_LINKOPENED : IDS_LINKFAILED);
}

void Application::CloseExternalLinks() noexcept
{
    if (externalLink)
    {
        std::lock_guard lock(externalLink->mutex);
        externalLink->window = nullptr;
        externalLink->completed.reset();
    }
}

DxUi::Label *Application::Label(DxUi::Panel &parent, unsigned label, DxUi::FontRole role)
{
    auto *result = parent.AddChild<DxUi::Label>(Resource(label));
    result->SetFontRole(role);
    if (role == DxUi::FontRole::Small)
        THROW_IF_FAILED(result->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    else if (role == DxUi::FontRole::Body)
        THROW_IF_FAILED(result->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    return result;
}
DxUi::Button *Application::Action(DxUi::Panel &parent, unsigned label, Command command, std::wstring tooltip)
{
    auto *button = parent.AddChild<DxUi::Button>(Resource(label));
    THROW_IF_FAILED(button->SetContentFontSizeDip(14.f));
    button->SetAccessibleName(Resource(label));
    button->SetTooltipText(tooltip.empty() ? Resource(label) : std::move(tooltip));
    button->SetOnClick([this, command] { Execute(command); });
    return button;
}

void Application::ApplyMarkdownSyntax()
{
    // Matching masks keep the read-only mirror eligible for parsed-syntax adoption.
    const auto extensions = MarkdownExtensionsFor(settings);
    if (!AcceptSourceResult(editor->TrySetMarkdownAnalysisExtensions(extensions)))
        return;
    if (!AcceptSourceResult(mirror->TrySetMarkdownAnalysisExtensions(extensions)))
        return;
    (void)AcceptSourceResult(equation->TrySetMarkdownAnalysisExtensions(extensions));
}

void Application::ApplyTheme()
{
    // Cache the system preference independently of the selected appearance so the
    // System preview remains truthful without registry reads during layout.
    DWORD value = 1, size = sizeof(value);
    const auto preference = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                                         L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size);
    systemDark = preference == ERROR_SUCCESS && value == 0;
    const bool dark = settings.appearance == Appearance::Dark || (settings.appearance == Appearance::System && systemDark);
    auto theme = DxUi::MakeDefaultThemePalette(dark);
    HIGHCONTRASTW contrast{sizeof(contrast)};
    (void)SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    const bool highContrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0 || (options.headless && options.scenario == L"high-contrast");
    if (highContrast)
    {
        const auto systemColor = [](int index)
        {
            const auto value = GetSysColor(index);
            return D2D1::ColorF(GetRValue(value) / 255.f, GetGValue(value) / 255.f, GetBValue(value) / 255.f);
        };
        theme.highContrast = true;
        theme.windowBackground = theme.surfaceBackground = theme.cardBackground = systemColor(COLOR_WINDOW);
        theme.text = systemColor(COLOR_WINDOWTEXT);
        theme.selectionFill = theme.selectionInactiveFill = systemColor(COLOR_HIGHLIGHT);
        theme.accent = systemColor(COLOR_HOTLIGHT);
        theme.selectionText = systemColor(COLOR_HIGHLIGHTTEXT);
        theme.focusStroke = systemColor(COLOR_HOTLIGHT);
    }
    else
    {
        theme.accent = D2D1::ColorF(dark ? 0xFF7778 : 0xC83C46);
        theme.accentHover = D2D1::ColorF(dark ? 0xFF999A : 0xB6303C);
        theme.accentPressed = D2D1::ColorF(dark ? 0xFFABAC : 0xA12A34);
        theme.selectionFill = theme.selectionInactiveFill = D2D1::ColorF(dark ? 0xA63B46 : 0xF2CFD3);
        theme.selectionText = D2D1::ColorF(dark ? 0xFFFFFFu : 0xA12A34);
        theme.focusStroke = theme.accent;
        theme.windowBackground = D2D1::ColorF(dark ? 0x17191Eu : settings.appearance == Appearance::Paper ? 0xEDE6D8u : 0xF0F1F4u);
        theme.surfaceBackground = D2D1::ColorF(dark ? 0x1C1F25u : settings.appearance == Appearance::Paper ? 0xF3EDDFu : 0xF8F8FAu);
        theme.cardBackground = theme.inputFill = D2D1::ColorF(dark ? 0x22252Bu : settings.appearance == Appearance::Paper ? 0xFFFAEFu : 0xFFFFFFu);
        theme.buttonBorder = theme.borderDefault;
    }
    BOOL animation = TRUE;
    (void)SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animation, 0);
    theme.reducedMotion = !animation || options.headless;
    theme.primaryButtonFill = D2D1::ColorF(0xC83C46);
    theme.primaryButtonText = D2D1::ColorF(D2D1::ColorF::White);
    host.SetTheme(theme);
    const auto underline = highContrast ? DxUi::RichTextLinkUnderlinePolicy::Always : DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret;
    for (auto *target : {static_cast<DxUi::RichTextEditor *>(editor), mirror, equation})
        if (!AcceptSourceResult(target->TrySetLinkUnderlinePolicy(underline)))
            return;
    const BOOL nativeDark = dark;
    (void)DwmSetWindowAttribute(window.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &nativeDark, sizeof(nativeDark));
    Layout();
}

const std::wstring &Application::CurrentText() const
{
    const auto source = editor->Snapshot();
    if (!cachedSource || !cachedSource->IsSameRevision(source))
    {
        cachedText = source.Text();
        cachedSource = source;
    }
    return cachedText;
}
bool Application::Dirty() const
{
    if (!editor)
        return false;
    const auto source = editor->Snapshot();
    if (dirtyFact && dirtyFact->recovered == recovered && dirtyFact->source.IsSameRevision(source))
        return dirtyFact->dirty;
    bool dirty = recovered || source.Length() != savedText.size();
    if (!dirty)
    {
        ++activity.dirtyTextComparisons;
        dirty = CurrentText() != savedText;
    }
    dirtyFact = DirtyFact{source, recovered, dirty};
    return dirty;
}
void Application::RefreshDocumentAnalysis(const DxUi::RichText::SourceSnapshot &source, bool committed)
{
    const auto sameOutline = [](const std::vector<Heading> &left, const std::vector<Heading> &right)
    {
        return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(),
                                                         [](const Heading &a, const Heading &b) { return a.level == b.level && a.text == b.text; });
    };
    const auto matchesCommittedOutline = [](const std::vector<Heading> &headings, const std::vector<std::pair<unsigned, std::wstring>> &identity)
    {
        return headings.size() == identity.size() &&
               std::equal(headings.begin(), headings.end(), identity.begin(),
                          [](const Heading &heading, const auto &item) { return heading.level == item.first && heading.text == item.second; });
    };

    const bool newSourceRevision = !documentAnalysisSource || !documentAnalysisSource->IsSameRevision(source);
    if (!newSourceRevision && !committed)
        return;

    const std::wstring *text{};
    if (newSourceRevision)
        text = &CurrentText();
    const bool sourceMatchesCommitted = !committed && newSourceRevision && committedAnalysisSource && *text == committedAnalysisSource->Text();
    const bool returningToCommitted = !committed && newSourceRevision && sourceMatchesCommitted;
    const bool transientSource = !committed && newSourceRevision && committedAnalysisSource && !sourceMatchesCommitted;
    if (transientSource && !previewCollapsedHeadings)
    {
        previewCollapsedHeadings = collapsedHeadings;
        previewCommittedOutline.emplace();
        previewCommittedOutline->reserve(documentInfo.headings.size());
        for (const auto &heading : documentInfo.headings)
            previewCommittedOutline->emplace_back(heading.level, heading.text);
    }

    std::optional<DocumentInfo> nextInfo;
    const DocumentInfo *analysis = &documentInfo;
    bool outlineRowsChanged{};
    if (newSourceRevision)
    {
        nextInfo.emplace(documentLineIndexStorage.Analyze(*text, documentInfo.lines));
        outlineRowsChanged = !sameOutline(nextInfo->headings, documentInfo.headings);
        analysis = &*nextInfo;
    }

    bool collapseStateChanged{};
    if ((committed || returningToCommitted) && previewCollapsedHeadings && collapsedHeadings != *previewCollapsedHeadings)
    {
        collapsedHeadings = *previewCollapsedHeadings;
        collapseStateChanged = true;
    }
    if (transientSource && previewCollapsedHeadings && previewCommittedOutline && outlineRowsChanged)
    {
        const auto desired =
            matchesCommittedOutline(analysis->headings, *previewCommittedOutline) ? *previewCollapsedHeadings : std::set<std::uint64_t>{};
        if (collapsedHeadings != desired)
        {
            collapsedHeadings = desired;
            collapseStateChanged = true;
        }
    }
    if (committed)
    {
        const bool sameAsPrior =
            previewCommittedOutline ? matchesCommittedOutline(analysis->headings, *previewCommittedOutline) : !outlineRowsChanged;
        if (!sameAsPrior && !collapsedHeadings.empty())
        {
            collapsedHeadings.clear();
            collapseStateChanged = true;
        }
        committedAnalysisSource = source;
        previewCollapsedHeadings.reset();
        previewCommittedOutline.reset();
    }
    else if (returningToCommitted)
    {
        previewCollapsedHeadings.reset();
        previewCommittedOutline.reset();
    }

    if (newSourceRevision)
    {
        using std::swap;
        swap(documentInfo, *nextInfo);
        documentLineIndexStorage.Retire(std::move(nextInfo->lineIndex), documentInfo.lines);
        documentAnalysisSource = source;
        ++activity.documentAnalysisUpdates;
    }
    if (outlineRowsChanged || collapseStateChanged)
        RefreshOutline();
}
bool Application::SynchronizeMirror()
{
    if (closing)
        return false;
    if (settings.view != EditorView::Split)
        return true;
    if (!root || host.GetRoot() != root)
        return false;
    auto *const primary = editor;
    auto *const target = mirror;
    auto *const owner = root;
    const auto source = primary->Snapshot();
    if (mirroredSource && mirroredSource->IsSameRevision(source))
        return true;
    const auto selection = primary->GetSelection();
    const auto revision = primary->Revision();
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto targetLifetime = GetControlLifetimeToken(*target);
    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    const auto current = [&]
    {
        if (closing || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || editor != primary || mirror != target ||
            root != owner || host.GetRoot() != owner || settings.view != EditorView::Split)
            return false;
        // These app-owned controls are never reparented independently of the
        // root; its host ownership and all three weak identities were checked.
        return primary->Snapshot().IsSameRevision(source) && primary->Revision() == revision && primary->GetSelection() == selection;
    };
    if (target->SynchronizeReadOnlySourceSnapshot(source, selection))
    {
        if (!current() || !target->Snapshot().IsSameRevision(source) || target->GetSelection() != selection)
            return false;
    }
    else
    {
        if (!current())
            return false;
        // Retain exact SetText fallback for an incompatible mirror profile.
        // Own this spelling across callbacks instead of borrowing a mutable app cache.
        const auto text = source.Text();
        if (!AcceptSourceResult(target->TrySetText(text)))
            return false;
        if (!current() || target->GetText() != text)
            return false;
    }
    // A nested synchronization may already have published this same source.
    if (!mirroredSource || !mirroredSource->IsSameRevision(source))
    {
        mirroredSource = source;
        ++activity.mirrorResets;
    }
    return true;
}
void Application::Status(unsigned id)
{
    if (feedback)
        feedback->SetText(Resource(id));
}
bool Application::AcceptSourceResult(DxUi::RichText::SourceResult result)
{
    if (result == DxUi::RichText::SourceResult::Success)
        return true;
    Status(result == DxUi::RichText::SourceResult::CapacityExceeded ? IDS_REPLACEFAILED : IDS_ACTIONFAILED);
    return false;
}
bool Application::SetDocument(std::wstring text, std::filesystem::path path, bool hasBom, FileStamp identity, bool rememberForStartup)
{
    const bool previousImageTransition = imageBindingTransition;
    imageBindingTransition = true;
    const auto restoreImageTransition = wil::scope_exit([&] { imageBindingTransition = previousImageTransition; });
    // A checked refusal preserves the entire current document, saved baseline, recovery and pending insertion.
    if (!DxUi::RichText::IsValidSourceUtf16(text))
        return AcceptSourceResult(DxUi::RichText::SourceResult::InvalidSource);
    if (text.size() > DxUi::RichText::Document::MaximumUnits)
        return AcceptSourceResult(DxUi::RichText::SourceResult::CapacityExceeded);
    const auto lifetime = GetControlLifetimeToken(*editor);
    // A replacement document cannot inherit a one-document grant through synchronous text callbacks.
    const bool previousDocumentPermission = documentRemoteImagesAllowed;
    const auto previousPermissionSource = editor->Snapshot();
    bool permissionCommitted = false;
    const auto restorePermission = wil::scope_exit(
        [&]() noexcept
        {
            if (!permissionCommitted && !lifetime.expired() && !closing && documentRemoteImagesAllowed == previousDocumentPermission &&
                editor->Snapshot().IsSameRevision(previousPermissionSource))
                editor->SetRemoteImagesEnabled(settings.allowRemoteImages || previousDocumentPermission);
        });
    editor->SetRemoteImagesEnabled(settings.allowRemoteImages);
    if (lifetime.expired() || closing)
        return false;
    if (!AcceptSourceResult(editor->TrySetDocumentFilePath(path.native())) || lifetime.expired() || closing)
        return false;
    if (!AcceptSourceResult(editor->TrySetText(text)))
        return false;
    if (lifetime.expired() || closing)
        return false;
    documentRemoteImagesAllowed = false;
    permissionCommitted = true;
    pendingBitmapPaste.reset();
    if (!initializing)
        AbandonStartupRestore();
    recovered = false;
    documentPath = std::move(path);
    EnsureFilesRoot();
    bom = hasBom;
    stamp = identity;
    savedText = editor->GetText();
    dirtyFact.reset();
    SetView(settings.view);
    Changed();
    RefreshFiles(true);
    if (rememberForStartup)
        RememberActiveDocument();
    return true;
}
void Application::Changed()
{
    pendingBitmapPaste.reset();
    if (switchingDocument)
        return;
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Enter);
    ++activity.changes;
    const auto source = editor->Snapshot();
    RefreshDocumentAnalysis(source, true);
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Analysis);
    if (!SynchronizeMirror())
        return;
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Mirror);
    const auto name = documentPath.empty() ? Resource(IDS_UNTITLED) : documentPath.filename().native();
    const bool dirty = Dirty();
    const auto displayedName = settings.focusMode && dirty ? name + L" · " + Resource(IDS_CHANGED) : name;
    const bool identityLayoutChanged = title->GetText() != displayedName;
    title->SetText(displayedName);
    saveState->SetText(Resource(dirty ? IDS_CHANGED : IDS_SAVED));
    if (identityLayoutChanged)
        Layout();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Identity);
    UpdateCaption(std::format(L"{}{} — RedTilde", dirty ? L"• " : L"", name));
    // Sending WM_SETTEXT to our own HWND is synchronous and can re-enter Close.
    // BeginClose detaches the host, so do not touch cached controls after it returns.
    if (closing)
        return;
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Caption);
    UpdateCounts();
    tools[0]->SetEnabled(editor->CanUndo());
    tools[1]->SetEnabled(editor->CanRedo());
    floating->SetVisible(false);
    if (settings.showFiles && dirty != lastFilesDirty)
    {
        UpdateCurrentFileDirtyBadge(dirty);
        // A synchronous accessibility publication can dispatch Close and detach the host.
        if (closing)
            return;
    }
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Chrome);
    if (editDiagnostics)
    {
        auto selectionArm = editDiagnostics->ArmSelection();
        SelectionChanged();
    }
    else
        SelectionChanged();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Selection);
    if (!initializing)
        CheckpointRecovery();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Recovery);
}

void Application::UpdateCounts()
{
    if (settings.focusMode)
    {
        const auto state = ResourceView(Dirty() ? IDS_CHANGED : IDS_SAVED);
        stats->SetText(std::vformat(ResourceView(IDS_FOCUSCOUNTS), std::make_wformat_args(documentInfo.words, state)));
    }
    else
    {
        const auto readingMinutes = std::max(std::size_t{1}, (documentInfo.words + 199) / 200);
        stats->SetText(
            std::vformat(ResourceView(IDS_STATUSCOUNTS), std::make_wformat_args(documentInfo.words, documentInfo.characters, readingMinutes)));
    }
}

bool Application::PublishCurrentBlockLabelIfVisible(bool sidebarWillBeVisible)
{
    if (closing)
        return false;
    if (!root || !sidebar || !currentBlock)
        return false;
    if ((!sidebar->IsVisible() && !sidebarWillBeVisible) || !currentBlock->IsVisible() || !currentBlockLatestFacts ||
        currentBlockPublishedFacts == currentBlockLatestFacts)
        return true;

    const auto facts = currentBlockLatestFacts.value();
    const auto blockType = ResourceView(facts.resourceId);
    auto text = std::vformat(ResourceView(IDS_BLOCKINFO), std::make_wformat_args(blockType, facts.lineCharacterCount));
    const auto request = ++activity.currentBlockLabelConstructions;
    currentBlockPublishedFacts.reset();
    auto *const publishedLabel = currentBlock;
    auto *const publishedRoot = root;
    const auto labelLifetime = GetControlLifetimeToken(*publishedLabel);
    const auto rootLifetime = GetControlLifetimeToken(*publishedRoot);
    publishedLabel->SetText(std::move(text));
    // SetText can reenter and retire either control or install newer requested facts.
    if (closing || labelLifetime.expired() || rootLifetime.expired() || currentBlock != publishedLabel || root != publishedRoot ||
        host.GetRoot() != publishedRoot)
        return false;
    // A nested callback can update the requested facts without publishing while hidden.
    if (activity.currentBlockLabelConstructions == request && currentBlockLatestFacts == facts)
        currentBlockPublishedFacts = facts;
    return true;
}

void Application::UpdateCaption(std::wstring caption)
{
    ++activity.captionRequests;
    if (appliedCaption && *appliedCaption == caption)
        return;
    // A native write remains synchronous and part of this command's measured
    // work. Repeated edits while already dirty need no identical Win32 write.
    // Invalidate before entering Win32 so reentrant requests cannot see stale
    // cached state; failed or superseded writes must remain retryable.
    appliedCaption.reset();
    const auto request = activity.captionRequests;
    ++activity.captionCalls;
    // This is our own HWND. Use WM_SETTEXT's explicit acceptance result: the
    // SetWindowText wrapper may report success for a receiver's zero result.
    if (DxUi::Failure::InvokeNative(&SendMessageW, window.get(), static_cast<UINT>(WM_SETTEXT), WPARAM{},
                                    reinterpret_cast<LPARAM>(caption.c_str())) == TRUE &&
        request == activity.captionRequests)
        appliedCaption = std::move(caption);
    else
        appliedCaption.reset();
}

void Application::CheckpointRecovery(bool immediate)
{
    std::optional<RecoverySnapshot> snapshot;
    auto selection = editor->GetSelection();
    bool needsRecovery = Dirty();
    std::wstring_view draft;
    std::wstring normalizedDraft;
    bool changedDraft{};
    if (HasBlockDraft() && editor->Snapshot().IsSameRevision(blockDraftSnapshot) && editor->Revision() == blockDraftRevision)
    {
        draft = blockDraftText->GetText();
        const std::wstring_view source = CurrentText();
        if (draft != source.substr(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first))
        {
            normalizedDraft = NormalizeInsertedLineEndings(draft, source);
            draft = normalizedDraft;
            if (!DxUi::RichText::IsValidUtf16(draft) ||
                source.size() - (blockDraftRange.last - blockDraftRange.first) + draft.size() > DxUi::RichText::Document::MaximumUnits)
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            changedDraft = draft != source.substr(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first);
            needsRecovery = needsRecovery || changedDraft;
        }
    }
    if (needsRecovery)
    {
        auto text = CurrentText();
        ++activity.recoveryTextCopies;
        if (changedDraft)
        {
            // Recovery is a previewable copy of the proposed edit. The live
            // authoritative source/history remain untouched until Apply.
            text.replace(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first, draft);
            selection = {blockDraftRange.first, blockDraftRange.first + draft.size()};
        }
        snapshot = RecoverySnapshot{std::move(text), documentPath, bom, selection, ++recoveryGeneration};
    }
    const bool flush = immediate || !snapshot.has_value();
    ++activity.recoverySubmissions;
    if (FAILED(recovery.SubmitDocument(activeDocumentKey, std::move(snapshot), flush)))
        Status(IDS_RECOVERYFAILED);
}
void Application::RecoveryCompleted()
{
    if (closing)
    {
        FinishClose();
        return;
    }
    if (!feedback)
        return;
    auto candidates = recovery.TakeCandidates();
    for (auto &candidate : candidates)
        recoveryCandidates.push_back(std::move(candidate));
    bool retiredCandidate{};
    for (const auto &id : recovery.TakeRetired())
        retiredCandidate = std::erase_if(recoveryCandidates, [&](const auto &candidate) { return candidate.id == id; }) != 0 || retiredCandidate;
    if (retiredCandidate && recoveryPanel->IsVisible())
        ShowRecovery();
    if (FAILED(recovery.Status()))
        Status(IDS_RECOVERYFAILED);
    if (!recoveryOffered && recovery.Ready())
    {
        recoveryOffered = true;
        if (!recoveryCandidates.empty() && (startupPolicy || options.recoveryRestartProbe || options.scenario == L"recovery"))
            ShowRecovery();
    }
    if (startupPending && recovery.Ready())
    {
        startupPending = false;
        if (SUCCEEDED(recovery.Status()) && recoveryCandidates.empty() && startupSource && editor->Snapshot().IsSameRevision(startupSource.value()) &&
            !settings.lastDocument.empty() && documentPath.empty() && !files.Busy() && !HasBlockDraft() && !sessionEnding)
        {
            openingSnapshot = editor->Snapshot();
            const HRESULT result = files.Submit(false, settings.lastDocument);
            startupOpening = SUCCEEDED(result);
            if (startupOpening)
                Status(IDS_LOADING);
            else
            {
                startupSource.reset();
                openingSnapshot.reset();
                Status(IDS_BUSY);
            }
        }
        else
            startupSource.reset();
    }
}
void Application::ShowRecovery()
{
    if (recoveryCandidates.empty())
    {
        recoveryPanel->SetVisible(false);
        Status(IDS_NORECOVERY);
        host.SetFocusControl(editor, MayRequestNativeFocus());
        return;
    }
    CloseDrawers();
    const auto &snapshot = recoveryCandidates.front().snapshot;
    const auto path = snapshot.originalPath.empty() ? Resource(IDS_UNTITLED) : snapshot.originalPath.native();
    const auto count = recoveryCandidates.size();
    restoreAllRecovery->SetVisible(count > 1);
    recoveryDescription->SetText(count > 1 ? std::vformat(Resource(IDS_RECOVERYMULTIPLEHELP), std::make_wformat_args(count, path))
                                           : std::vformat(Resource(IDS_RECOVERYHELP), std::make_wformat_args(path)));
    recoveryPreview->SetText(snapshot.text);
    recoveryPanel->SetVisible(true);
    host.SetFocusControl(recoveryPanel->GetLogicalChild(3), MayRequestNativeFocus());
    Layout();
}
void Application::SetView(EditorView view)
{
    if (closing || !root || host.GetRoot() != root)
        return;
    auto *const primary = editor;
    auto *const target = mirror;
    auto *const owner = root;
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto targetLifetime = GetControlLifetimeToken(*target);
    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    const auto current = [&]
    {
        return !closing && !primaryLifetime.expired() && !targetLifetime.expired() && !ownerLifetime.expired() && editor == primary &&
               mirror == target && root == owner && host.GetRoot() == owner && settings.view == view;
    };
    selectionToolsFocus.reset();
    // The same editor/document/history survives all global presentation changes.
    settings.view = view;
    if (!AcceptSourceResult(primary->TrySetViewMode(view == EditorView::Source ? DxUi::RichText::ViewMode::Source : DxUi::RichText::ViewMode::Rich)))
        return;
    if (!current())
        return;
    // Source supersession aborts source-bound work, but this view transition
    // uses the latest source. Retired owners and a newer view request still win.
    (void)SynchronizeMirror();
    if (!current())
        return;
    modeHint->SetText(Resource(static_cast<unsigned>(view == EditorView::Source  ? IDS_SOURCEHINT
                                                     : view == EditorView::Split ? IDS_SPLITHINT
                                                                                 : IDS_BANNER)));
    if (!current())
        return;
    floating->SetVisible(false);
    if (!current())
        return;
    Layout();
    if (!current())
        return;
    SelectionChanged();
    if (!current())
        return;
    host.SetFocusControl(primary, false);
    if (!current())
        return;
    if (!initializing)
        SavePreferences();
}
void Application::SelectionChanged(bool refreshTools)
{
    EditDiagnostics::SelectionInvocation selectionInvocation(editDiagnostics);
    if (closing || switchingDocument || !floating)
        return;
    if (pendingBitmapPaste && (pendingBitmapPaste->lifetime.expired() || pendingBitmapPaste->documentKey != activeDocumentKey ||
                               pendingBitmapPaste->view != settings.view || pendingBitmapPaste->documentPath != documentPath ||
                               pendingBitmapPaste->selection != editor->GetSelection() || editor->IsSelectionGestureActive() ||
                               editor->Snapshot().ChangedRangeFrom(pendingBitmapPaste->source).kind != DxUi::RichText::SourceChangeKind::Unchanged))
        pendingBitmapPaste.reset();
    if (editor->IsSelectionGestureActive())
    {
        // Shared editor owns every selection route and the final release event.
        // Keep pointer movement O(1); derived facts and placement update on release.
        pendingSelectionTools = false;
        selectionToolsFocus.reset();
        floating->SetVisible(false);
        return;
    }
    const auto selection = [&]
    {
        EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::SelectionSnapshot);
        return editor->GetSelection();
    }();
    const auto source = [&]
    {
        EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::SelectionSnapshot);
        return editor->Snapshot();
    }();
    const auto sourceRevision = editor->Revision();
    {
        EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::DocumentAnalysisRefresh);
        RefreshDocumentAnalysis(source, false);
    }
    const auto syntax = [&]
    {
        EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::SelectionSnapshot);
        return editor->GetMarkdownSyntax();
    }();
    bool selectionChanged{};
    {
        EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::SelectionFactsCache);
        if (selectionToolsFocus && (!selectionToolsFocus->source.IsSameRevision(source) || selectionToolsFocus->selection != selection ||
                                    selectionToolsFocus->view != settings.view))
            selectionToolsFocus.reset();
        selectionChanged = !selectionFactsSource || !selectionFactsSource->IsSameRevision(source) || selectionFactsSelection != selection ||
                           selectionFactsView != settings.view || selectionFactsAnalysis != activity.changes;
    }
    if (selectionChanged || selectionFactsSyntax != syntax)
    {
        if (selectionChanged)
            ++activity.selectionPresentationUpdates;
        else
            ++activity.syntaxPresentationUpdates;
        if (selectionChanged && settings.view == EditorView::Split)
        {
            // Text-service previews and cancellation notify selection without
            // a committed edit. Match source identity before using their ACPs;
            // equal-length previews can also change Unicode/CRLF boundaries.
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::Mirror);
            if (closing || !root || host.GetRoot() != root)
                return;
            auto *const primary = editor;
            auto *const target = mirror;
            auto *const owner = root;
            const auto primaryLifetime = GetControlLifetimeToken(*primary);
            const auto targetLifetime = GetControlLifetimeToken(*target);
            const auto ownerLifetime = GetControlLifetimeToken(*owner);
            const auto current = [&]
            {
                if (closing || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || editor != primary ||
                    mirror != target || root != owner || host.GetRoot() != owner || settings.view != EditorView::Split)
                    return false;
                return primary->Snapshot().IsSameRevision(source) && primary->Revision() == sourceRevision && primary->GetSelection() == selection;
            };
            if (!SynchronizeMirror() || !current())
                return;
            // Read-only Source has no rich visual affinity. Adoption already
            // reset this exact selection; retain the canonical setter for a
            // real selection change or an incompatible-profile fallback.
            if (target->GetSelection() != selection || !target->IsReadOnly() || target->GetViewMode() != DxUi::RichText::ViewMode::Source)
            {
                if (!AcceptSourceResult(target->TrySetSelection(selection)))
                    return;
            }
            if (!current())
                return;
            (void)target->ScrollSourceRangeIntoView(host, selection.Start(), selection.End(), false);
            ++activity.mirrorSelectionUpdates;
        }
        std::wstring expression;
        std::array<bool, 4> formats{};
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::InlineSemantics);
            // In-place math source previews what is being typed; only the region itself is read.
            if (const auto region = editor->GetSemanticSourceRegion();
                region && region->source.first <= region->source.last && region->source.last <= source.Length() &&
                selection.caret >= region->source.first && selection.caret <= region->source.last)
                expression = MathRegionExpression(*region, source.Slice(region->source.first, region->source.last));
            if (syntax)
                for (const auto &node : syntax->Inlines())
                    if (selection.caret >= node.source.first && selection.caret <= node.source.last)
                    {
                        using Kind = DxUi::RichText::MarkdownInlineKind;
                        if (node.kind == Kind::Math && expression.empty() && node.value.size() <= DxUi::RichText::MathMaximumSourceUnits)
                            expression = node.value;
                        if (node.kind == Kind::Strong)
                            formats[0] = true;
                        if (node.kind == Kind::Emphasis)
                            formats[1] = true;
                        if (node.kind == Kind::Code)
                            formats[2] = true;
                        if (node.kind == Kind::Link)
                            formats[3] = true;
                    }
            // Own-line $$ and \[ \] display math is a source-owned block, not an inline node.
            if (syntax && expression.empty())
                for (const auto &node : syntax->Nodes())
                    if (node.block.kind == DxUi::RichText::MarkdownBlockKind::FencedCode &&
                        (node.block.fenceMarker == L'$' || node.block.fenceMarker == L'[') && node.block.closed &&
                        selection.caret >= node.block.source.first && selection.caret <= WithoutLineEnd(syntax->Source(), node.block.source))
                    {
                        auto body = syntax->Slice(node.block.content);
                        while (!body.empty() && (body.back() == L'\n' || body.back() == L'\r'))
                            body.remove_suffix(1);
                        if (body.size() <= DxUi::RichText::MathMaximumSourceUnits)
                            expression = body;
                        break;
                    }
        }
        bool layoutChanged{};
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::ToolCaptionPublication);
            // The pinned grammar recognizes display delimiters only on their own lines.
            const auto preview = expression.empty() ? std::wstring{} : L"\\[\n" + expression + L"\n\\]";
            if (equation->GetText() != preview)
                if (!AcceptSourceResult(equation->TrySetText(preview)))
                    return;
            const bool nextEquationContext = !expression.empty();
            layoutChanged = equationContext != nextEquationContext;
            equationContext = nextEquationContext;
            for (unsigned i = 0; i < 4; ++i)
            {
                auto *button = tools[i == 3 ? 13 : i + 3];
                button->SetEnabled(!equationContext);
                button->SetToggleState(formats[i]);
            }
            if (!selectionCaptionPublishedEquationContext || selectionCaptionPublishedEquationContext.value() != nextEquationContext)
            {
                const auto request = ++activity.selectionCaptionConstructions;
                selectionCaptionPublishedEquationContext.reset();
                tools[2]->SetText(Resource(nextEquationContext ? IDS_MATH : IDS_PARAGRAPH));
                if (activity.selectionCaptionConstructions == request)
                    selectionCaptionPublishedEquationContext = nextEquationContext;
            }
        }
        if (selectionChanged)
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::Outline);
            currentHeading.reset();
            for (std::size_t i = 0; i < documentInfo.headings.size(); ++i)
                if (documentInfo.headings[i].offset <= selection.caret)
                    currentHeading = i;
            const auto headingId = currentHeading ? std::optional<std::uint64_t>(currentHeading.value() + 1) : std::nullopt;
            // Model refresh already reconciles surviving focused rows and keeps the viewport.
            // Republish only an actual selection or focus transition, which reveals it.
            if (outlineTree->GetSelectedItemId() != headingId || outlineTree->GetFocusedItemId() != headingId)
            {
                ++activity.outlineSelectionUpdates;
                outlineTree->SetSelectedItemId(headingId);
            }
        }
        DocumentPosition position;
        std::optional<std::pair<unsigned, std::size_t>> listFacts;
        unsigned blockResourceId{};
        std::size_t count{};
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::BlockLookup);
            position = DocumentPositionAt(documentInfo, selection.caret);
            count = position.lineCharacters;
            blockResourceId = syntax ? CurrentBlockResourceAt(*syntax, selection.caret, &listFacts) : IDS_BLOCKPARAGRAPH;
            if (equationContext)
                blockResourceId = IDS_MATH;
        }
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::BlockLabels);
            currentBlockLatestFacts = CurrentBlockLabelFacts{blockResourceId, count};
            const bool showListSummary = listFacts && (blockResourceId == IDS_BLOCKLIST || blockResourceId == IDS_BLOCKLISTCONTAINER);
            listSummary->SetVisible(showListSummary);
            listSummaryIcon->SetVisible(showListSummary);
            currentBlock->SetVisible(!showListSummary);
            if (showListSummary && listSummaryFacts != listFacts)
            {
                const auto kind = ResourceView(listFacts.value().first);
                const auto items = listFacts.value().second;
                const auto request = ++listSummaryPublication;
                listSummaryFacts.reset();
                listSummary->SetText(std::vformat(ResourceView(IDS_BLOCKITEMINFO), std::make_wformat_args(kind, items)));
                if (listSummaryPublication == request)
                    listSummaryFacts = listFacts;
            }
            if (!PublishCurrentBlockLabelIfVisible())
                return;
        }
        if (selectionChanged)
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::PositionStatus);
            const auto selected = selection.End() - selection.Start();
            const auto encoding = bom ? L" BOM" : L"";
            unsigned eolId = IDS_EOLNONE;
            switch (documentInfo.newlineKind)
            {
            case DocumentNewlineKind::CrLf:
                eolId = IDS_EOLCRLF;
                break;
            case DocumentNewlineKind::Lf:
                eolId = IDS_EOLLF;
                break;
            case DocumentNewlineKind::Cr:
                eolId = IDS_EOLCR;
                break;
            case DocumentNewlineKind::Mixed:
                eolId = IDS_EOLMIXED;
                break;
            case DocumentNewlineKind::None:
                break;
            }
            const auto eol = ResourceView(eolId);
            positionStatus->SetText(
                std::vformat(ResourceView(IDS_STATUSPOSITION), std::make_wformat_args(selected, position.line, position.column, encoding, eol)));
        }
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::Find);
            RefreshFind();
        }
        EditDiagnostics::SelectionMeasure routingMeasure(editDiagnostics, SelectionComponent::Routing);
        selectionFactsSyntax = syntax;
        selectionFactsSource = source;
        selectionFactsSelection = selection;
        selectionFactsView = settings.view;
        selectionFactsAnalysis = activity.changes;
        if (layoutChanged)
            Layout();
    }
    // Syntax publication refreshes facts, but must not reopen tools dismissed
    // by Escape or an explicit navigation command.
    if (!refreshTools)
        return;
    if (selection.Start() == selection.End() || settings.view == EditorView::Source || drawer->IsVisible() || findBar->IsVisible() ||
        blockDraft->IsVisible() || settings.focusMode || equationContext)
    {
        pendingSelectionTools = false;
        selectionToolsFocus.reset();
        floating->SetVisible(false);
        return;
    }
    const auto rectangles = editor->TryGetTextInputRangeRects(host, selection.Start(), selection.End());
    if (!rectangles)
    {
        pendingSelectionTools = true;
        return;
    }
    pendingSelectionTools = false;
    if (rectangles->empty())
    {
        // A prepared hidden-only source range has no painted anchor. It is not
        // pending work and must not create a permanent invalidation loop.
        selectionToolsFocus.reset();
        floating->SetVisible(false);
        return;
    }
    const auto first = rectangles->front();
    const float viewport = host.GetClientBoundsDip().right;
    const float left = std::clamp(first.left, 16.f, std::max(16.f, viewport - 452));
    const float minimumTop = editor->GetBounds().top + 4;
    const float top = std::clamp(first.top - 52, minimumTop, std::max(minimumTop, page->GetBounds().bottom - 52));
    floating->SetBounds(D2D1::RectF(left, top, left + 436, top + 44));
    const std::array<float, 5> widths{112, 56, 56, 56, 56};
    float x = left + 8;
    for (std::size_t i = 0; i < floatingActions.size(); ++i)
    {
        floatingActions[i]->SetBounds(D2D1::RectF(x, top + 6, x + widths[i], top + 38));
        x += widths[i] + 4;
    }
    selectionHintChip->SetBounds(D2D1::RectF(left + 372, top + 10, left + 428, top + 34));
    selectionHint->SetBounds(D2D1::RectF(left + 376, top + 12, left + 424, top + 32));
    floating->SetVisible(true);
    if (selectionToolsFocus)
    {
        selectionToolsFocus.reset();
        host.SetFocusControl(floatingActions.front(), MayRequestNativeFocus());
    }
}

void Application::FocusSelectionTools()
{
    selectionToolsFocus = SelectionToolsFocusRequest{editor->Snapshot(), editor->GetSelection(), settings.view};
    SelectionChanged();
}

void Application::SavePreferences()
{
    if ((options.headless && !options.preferencesProbe) || initializing)
        return;
    const HRESULT result = preferenceWriter.Submit(settings);
    if (FAILED(result))
        Status(IDS_SETTINGSFAILED);
    else
        windowPlacementDirty = false;
}
void Application::CloseDrawers()
{
    const bool hadBlockDraft = blockDraft->IsVisible();
    pendingSelectionTools = false;
    pendingBlockPlacement = false;
    selectionToolsFocus.reset();
    drawer->SetVisible(false);
    findBar->SetVisible(false);
    floating->SetVisible(false);
    blockDraft->SetVisible(false);
    recoveryPanel->SetVisible(false);
    editor->SetReadOnly(false);
    if (hadBlockDraft)
        (void)AcceptSourceResult(editor->TrySetSourceHighlights({}));
    RefreshFind();
    Layout();
    host.SetFocusControl(editor, MayRequestNativeFocus());
    if (!initializing && !closing)
        CheckpointRecovery();
    if (hadBlockDraft && heldOpen)
        (void)PostMessageW(window.get(), FileReady, 0, 0);
}

void Application::ReportUiFailure(std::string_view cause) noexcept
{
    MonitorDiagnostics::WriteUtf8(RedMonitor::Severity::Error, cause);
    if (reportingUiFailure)
        return;
    reportingUiFailure = true;
    const auto restore = wil::scope_exit([&] { reportingUiFailure = false; });
    afterSave = {};
    try
    {
        if (closingAfterSave)
            ResumeAfterSaveClose();
        if (!closing && root && host.GetRoot() == root)
            Status(IDS_ACTIONFAILED);
    }
    catch (const std::bad_alloc &)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::exception &)
    {
        DxUi::Failure::FailUnexpected();
    }
}
void Application::Execute(Command command) noexcept
{
    RunUiAction([&] { ExecuteCommand(command); });
}
void Application::ExecuteCommand(Command command, unsigned zoomSteps)
{
    if (command == Command::New || command == Command::Open || command == Command::OpenFolder || command == Command::Close ||
        command == Command::LocalSource || command == Command::RestoreRecovery || command == Command::RestoreAllRecovery)
        pendingBitmapPaste.reset();
    if (closing || closingAfterSave || sessionEnding || switchingDocument)
        return;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.commandAdmissionForTest)
    {
        const HRESULT admitted = options.commandAdmissionForTest(command);
        if (FAILED(admitted))
        {
            ReportUiFailure("checked command admission refusal");
            return;
        }
    }
#endif
    using namespace DxUi::RichText;
    // Presentation/structural changes cannot interrupt native IME composition.
    DxUi::NativeTextInputState native;
    if (host.TryReadNativeTextInputState(host.GetFocusControl(), native) && native.compositionStartIndex)
        return;
    selectionToolsFocus.reset();
    if (blockDraft->IsVisible() && command != Command::ApplyBlock && command != Command::CloseDrawer)
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (command == Command::New || command == Command::Open || command == Command::OpenFolder || command == Command::Close ||
        command == Command::RestoreRecovery || command == Command::RestoreAllRecovery)
        AbandonStartupRestore();
    MarkdownCommandRequest request;
    bool semantic = true;
    switch (command)
    {
    case Command::FileMenu:
        FileMenu();
        return;
    case Command::OpenFolder:
        OpenFolder();
        return;
    case Command::Overflow:
        FileMenu(true);
        return;
    case Command::Files:
        pendingSelectionTools = false;
        floating->SetVisible(false);
        settings.showFiles = !settings.showFiles;
        RefreshFiles(true);
        Layout();
        SavePreferences();
        host.SetFocusControl(settings.showFiles ? static_cast<DxUi::Control *>(filesTree) : editor, MayRequestNativeFocus());
        return;
    case Command::Minimize:
        if (!options.headless)
            ShowWindow(window.get(), SW_MINIMIZE);
        return;
    case Command::Maximize:
        if (!options.headless)
            ShowWindow(window.get(), IsZoomed(window.get()) ? SW_RESTORE : SW_MAXIMIZE);
        return;
    case Command::Close:
        SendMessageW(window.get(), WM_CLOSE, 0, 0);
        return;
    case Command::OpenLink:
    {
        const auto document = editor->GetSemanticDocument();
        const auto index = CurrentLinkElement(document);
        if (!index)
        {
            Status(IDS_LINKUNAVAILABLE);
            return;
        }
        const auto &link = document->Elements()[index.value()];
        if (link.destination.starts_with(L'#'))
        {
            if (!editor->NavigateSemanticElement(host, document, index.value()))
                Status(IDS_NOTAVAILABLE);
        }
        else
            RequestOpenLink({link.source, link.destination, link.title, link.name, link.wiki});
        return;
    }
    case Command::FindPrevious:
        FindNext(true);
        return;
    case Command::Replace:
        ReplaceFind(false);
        return;
    case Command::ReplaceAll:
        ReplaceFind(true);
        return;
    case Command::RemoteImages:
        settings.allowRemoteImages = !settings.allowRemoteImages;
        editor->SetRemoteImagesEnabled(settings.allowRemoteImages || documentRemoteImagesAllowed);
        images.DiscardCancelled();
        remoteImagesToggle->SetToggleState(settings.allowRemoteImages);
        ServiceImages();
        SavePreferences();
        return;
    case Command::LocalNetworkImages:
        settings.allowLocalNetworkImages = !settings.allowLocalNetworkImages;
        images.SetAllowLocalNetwork(settings.allowLocalNetworkImages);
        // Retire existing request identities before applying the new transport policy.
        editor->SetRemoteImagesEnabled(false);
        editor->SetRemoteImagesEnabled(settings.allowRemoteImages || documentRemoteImagesAllowed);
        images.DiscardCancelled();
        localNetworkImagesToggle->SetToggleState(settings.allowLocalNetworkImages);
        ServiceImages();
        SavePreferences();
        return;
    case Command::DocumentImages:
        if (settings.allowRemoteImages)
            return;
        documentRemoteImagesAllowed = !documentRemoteImagesAllowed;
        editor->SetRemoteImagesEnabled(documentRemoteImagesAllowed);
        images.DiscardCancelled();
        imageFailureStatus = S_OK;
        ServiceImages();
        return;
    case Command::ReloadImages:
        imageFailureStatus = S_OK;
        imageWatchReloadPending = true;
        editor->ReloadImages();
        images.DiscardCancelled();
        ServiceImages();
        return;
    case Command::ImageFirstPaste:
        settings.imageFirstPaste = !settings.imageFirstPaste;
        host.SetClipboardPreference(settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
        imageFirstPasteToggle->SetToggleState(settings.imageFirstPaste);
        SavePreferences();
        return;
    case Command::DimBlocks:
        settings.dimInactiveBlocks = !settings.dimInactiveBlocks;
        Layout();
        SavePreferences();
        return;
    case Command::StrikeCompletedTasks:
        settings.strikeCompletedTasks = !settings.strikeCompletedTasks;
        Layout();
        SavePreferences();
        return;
    case Command::SyntaxHighlight:
    case Command::SyntaxSuperscript:
    case Command::SyntaxSubscript:
    case Command::SyntaxInserted:
    case Command::SyntaxWikiLinks:
    case Command::SyntaxDefinitionLists:
    case Command::SyntaxAbbreviations:
    {
        // Reanalysis keeps source, selection and history; only presentation changes.
        auto &enabled = settings.markdownSyntax[static_cast<std::size_t>(command) - static_cast<std::size_t>(Command::SyntaxHighlight)];
        enabled = !enabled;
        ApplyMarkdownSyntax();
        Layout();
        SavePreferences();
        return;
    }
    case Command::Recovery:
        ShowRecovery();
        return;
    case Command::LaterRecovery:
        recoveryPanel->SetVisible(false);
        host.SetFocusControl(editor, MayRequestNativeFocus());
        return;
    case Command::DiscardRecovery:
        if (!recoveryCandidates.empty())
        {
            if (FAILED(recovery.Discard(recoveryCandidates.front().id)))
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            Status(IDS_RECOVERYBUSY);
        }
        return;
    case Command::RestoreRecovery:
        if (!recoveryCandidates.empty())
            AskThen(
                [this, candidate = recoveryCandidates.front()]
                {
                    // A prompt or admitted save can pump recovery completions.
                    // Only the exact copy clicked before that boundary may restore.
                    if (std::none_of(recoveryCandidates.begin(), recoveryCandidates.end(),
                                     [&candidate](const auto &current) { return current.id == candidate.id; }))
                    {
                        Status(IDS_RECOVERYUNAVAILABLE);
                        return;
                    }
                    if (!StartWorkspaceDocument() || closing || host.GetRoot() != root)
                        return;
                    SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom, {}, false);
                    recovered = true;
                    (void)AcceptSourceResult(editor->TrySetSelection(candidate.snapshot.selection));
                    Changed();
                    const auto snapshot = RecoverySnapshot{editor->GetText(), documentPath, bom, editor->GetSelection(), ++recoveryGeneration};
                    if (FAILED(recovery.SubmitDocument(activeDocumentKey, snapshot, true, candidate.id)))
                    {
                        Status(IDS_RECOVERYFAILED);
                        return;
                    }
                    std::erase_if(recoveryCandidates, [&candidate](const auto &current) { return current.id == candidate.id; });
                    CloseDrawers();
                    Status(IDS_RECOVERED);
                });
        return;
    case Command::RestoreAllRecovery:
    {
        if (files.Busy() || HasBlockDraft() || unsavedPromptOpen || recoveryCandidates.empty())
        {
            Status(IDS_BUSY);
            return;
        }
        const auto candidates = recoveryCandidates;
        const std::size_t reuse = documentPath.empty() && !Dirty() && !editor->CanUndo() && !editor->CanRedo() ? 1u : 0u;
        if (inactiveDocuments.size() + 1 + candidates.size() - reuse > RecoveryService::MaximumDocuments)
        {
            Status(IDS_WORKSPACEFULL);
            return;
        }
        for (const auto &candidate : candidates)
        {
            if (std::ranges::none_of(recoveryCandidates, [&](const auto &current) { return current.id == candidate.id; }))
                continue; // A completion retired this exact copy; never substitute another.
            if (!StartWorkspaceDocument() || closing || host.GetRoot() != root)
                return;
            SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom, {}, false);
            if (closing || host.GetRoot() != root)
                return;
            recovered = true;
            (void)AcceptSourceResult(editor->TrySetSelection(candidate.snapshot.selection));
            Changed();
            if (closing || host.GetRoot() != root)
                return;
            const auto snapshot = RecoverySnapshot{editor->GetText(), documentPath, bom, editor->GetSelection(), ++recoveryGeneration};
            if (FAILED(recovery.SubmitDocument(activeDocumentKey, snapshot, true, candidate.id)))
            {
                Status(IDS_RECOVERYFAILED);
                return; // Keep the original candidate until its replacement can be admitted.
            }
            std::erase_if(recoveryCandidates, [&](const auto &current) { return current.id == candidate.id; });
        }
        RefreshFiles();
        CloseDrawers();
        Status(IDS_RECOVERED);
        return;
    }
    case Command::New:
        AskWorkspaceThen(
            [this]
            {
                ResetWorkspace();
                SetDocument({});
                host.SetFocusControl(editor, MayRequestNativeFocus());
            });
        return;
    case Command::Open:
        Open();
        return;
    case Command::Save:
        Save();
        return;
    case Command::SaveAs:
        Save(true);
        return;
    case Command::Compose:
        SetView(EditorView::Compose);
        return;
    case Command::Source:
        SetView(EditorView::Source);
        return;
    case Command::Split:
        SetView(EditorView::Split);
        return;
    case Command::Undo:
        (void)editor->Undo();
        semantic = false;
        break;
    case Command::Redo:
        (void)editor->Redo();
        semantic = false;
        break;
    case Command::Heading:
    case Command::Insert:
    {
        const auto bounds = tools[command == Command::Heading ? 2 : 8]->GetBounds();
        Menu(host.DipPointToScreenPoint({bounds.left, bounds.bottom}), command == Command::Insert, command == Command::Heading);
    }
        return;
    case Command::Bold:
    case Command::Italic:
    case Command::Strike:
    case Command::Code:
        if (settings.view == EditorView::Source)
        {
            (void)editor->ToggleFormatting(command == Command::Bold     ? L"**"
                                           : command == Command::Italic ? L"*"
                                           : command == Command::Strike ? L"~~"
                                                                        : L"`");
            semantic = false;
        }
        else
        {
            request.command = MarkdownCommand::ToggleStyle;
            request.style = command == Command::Bold     ? MarkdownTextStyle::Strong
                            : command == Command::Italic ? MarkdownTextStyle::Emphasis
                            : command == Command::Strike ? MarkdownTextStyle::Strikethrough
                                                         : MarkdownTextStyle::Code;
        }
        break;
    case Command::Bullets:
        request.command = MarkdownCommand::SetList;
        break;
    case Command::Numbered:
        request.command = MarkdownCommand::SetList;
        request.listKind = MarkdownListKind::Ordered;
        break;
    case Command::Task:
        request.command = MarkdownCommand::SetList;
        request.listKind = MarkdownListKind::Task;
        break;
    case Command::Paragraph:
    case Command::H1:
    case Command::H2:
    case Command::H3:
        request.command = MarkdownCommand::SetHeading;
        request.headingLevel = static_cast<unsigned>(command) - static_cast<unsigned>(Command::Paragraph);
        break;
    case Command::Table:
        request.command = MarkdownCommand::InsertTable;
        break;
    case Command::RowAdd:
        request.command = MarkdownCommand::InsertTableRow;
        break;
    case Command::RowRemove:
        request.command = MarkdownCommand::DeleteTableRow;
        break;
    case Command::ColumnAdd:
        request.command = MarkdownCommand::InsertTableColumn;
        break;
    case Command::ColumnRemove:
        request.command = MarkdownCommand::DeleteTableColumn;
        break;
    case Command::Link:
    {
        const auto selection = editor->GetSelection();
        LocalSource();
        if (blockDraft->IsVisible())
        {
            blockDraftRange = {selection.Start(), selection.End()};
            const auto label = blockDraftSnapshot.Slice(selection.Start(), selection.End());
            blockDraftText->SetText(L"[" + (label.empty() ? L"link text" : label) + L"](https://)");
        }
        return;
    }
    case Command::Image:
    {
        const auto path = PickFile(false, true);
        if (path.empty())
            return;
        DxUi::TextClipboardContent content;
        content.filePaths.push_back(path.native());
        std::wstring insertion;
        if (editor->TryPrepareClipboardPaste(content, insertion))
            (void)editor->ReplaceSelection(std::move(insertion));
        else
            Status(IDS_NOTAVAILABLE);
        semantic = false;
        break;
    }
    case Command::Mermaid:
        (void)editor->ReplaceSelection(
            NormalizeInsertedLineEndings(L"\n\n```mermaid\ngraph LR\nA[Idea] --> B[Write]\nB --> C[Share]\n```\n\n", CurrentText()));
        semantic = false;
        break;
    case Command::Math:
        // Display delimiters must own their lines; a one-line $$...$$ stays literal at this pin.
        (void)editor->ReplaceSelection(NormalizeInsertedLineEndings(L"\n\n$$\nx = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}\n$$\n\n", CurrentText()));
        semantic = false;
        break;
    case Command::Quote:
    case Command::Fence:
    {
        const auto selection = editor->GetSelection();
        const bool quote = command == Command::Quote;
        const bool changed = selection.Start() != selection.End()
                                 ? editor->WrapSelectionInMarkdownBlock(quote ? MarkdownBlockKind::BlockQuote : MarkdownBlockKind::FencedCode,
                                                                        quote ? std::wstring_view{} : std::wstring_view(L"text"))
                                 : editor->ReplaceSelection(NormalizeInsertedLineEndings(
                                       quote ? L"\n> A thought worth keeping.\n" : L"\n\n```text\nYour code here\n```\n\n", CurrentText()));
        Status(changed ? IDS_FORMATTED : IDS_NOTAVAILABLE);
        semantic = false;
        break;
    }
    case Command::LocalSource:
        LocalSource();
        return;
    case Command::Find:
        drawer->SetVisible(false);
        floating->SetVisible(false);
        findBar->SetVisible(true);
        Layout();
        RefreshFind();
        host.SetFocusControl(findText, MayRequestNativeFocus());
        return;
    case Command::FindNext:
        FindNext();
        return;
    case Command::FindMatchCase:
        findMatchCase = !findMatchCase;
        findSource.reset();
        findCaseToggle->SetToggleState(findMatchCase);
        RefreshFind();
        if (findBar->IsVisible())
            host.SetFocusControl(findText, MayRequestNativeFocus());
        return;
    case Command::ZoomIn:
    case Command::ZoomOut:
    case Command::ZoomReset:
    {
        const int delta = static_cast<int>(std::min(zoomSteps, 25u)) * (command == Command::ZoomIn ? 5 : -5);
        const auto scale =
            command == Command::ZoomReset ? 100u : static_cast<unsigned>(std::clamp(static_cast<int>(settings.textScale) + delta, 75, 200));
        if (settings.textScale != scale)
        {
            settings.textScale = scale;
            Layout();
            SavePreferences();
        }
        return;
    }
    case Command::Settings:
        findBar->SetVisible(false);
        RefreshFind();
        floating->SetVisible(false);
        drawer->SetVisible(!drawer->IsVisible());
        Layout();
        host.SetFocusControl(drawer->IsVisible() ? static_cast<DxUi::Control *>(themeButtons[0]) : editor, MayRequestNativeFocus());
        return;
    case Command::Focus:
        settings.focusMode = !settings.focusMode;
        CloseDrawers();
        Layout();
        SavePreferences();
        return;
    case Command::Outline:
        settings.showOutline = !settings.showOutline;
        Layout();
        SavePreferences();
        return;
    case Command::ThemeSystem:
    case Command::ThemeLight:
    case Command::ThemeDark:
    case Command::ThemePaper:
        settings.appearance = static_cast<Appearance>(static_cast<unsigned>(command) - static_cast<unsigned>(Command::ThemeSystem));
        ApplyTheme();
        SavePreferences();
        return;
    case Command::CloseDrawer:
        CloseDrawers();
        return;
    case Command::ApplyBlock:
        ApplyBlockSource();
        return;
    }
    if (semantic)
    {
        const auto result = editor->ExecuteMarkdownCommand(request);
        Status(result == MarkdownCommandStatus::Unavailable ? IDS_NOTAVAILABLE : IDS_FORMATTED);
    }
    floating->SetVisible(false);
    host.SetFocusControl(editor, MayRequestNativeFocus());
}

void Application::LocalSource()
{
    if (editor->GetSemanticSourceRegion())
    {
        if (editor->EndSemanticSourceEdit(host))
            SelectionChanged();
        return;
    }
    const auto semantic = editor->GetAcknowledgedSemanticDocument(host);
    const auto selection = editor->GetSelection();
    const auto source = editor->GetText();
    if (semantic)
    {
        const auto elements = semantic->Elements();
        for (std::size_t index = elements.size(); index > 0; --index)
        {
            const auto &element = elements[index - 1];
            // Code, diagrams and math edit their exact source in place. A display equation
            // ends at its closing delimiter, so the following line is not inside it.
            const auto elementLast =
                element.role == DxUi::RichText::SemanticRole::Math ? WithoutLineEnd(source, element.source) : element.source.last;
            if (selection.caret >= element.source.first && selection.caret <= elementLast &&
                editor->BeginSemanticSourceEdit(host, semantic, index - 1))
            {
                CloseDrawers();
                // The block caption and live equation preview follow the region.
                SelectionChanged();
                return;
            }
        }
    }
    std::size_t first = selection.Start(), last = selection.End();
    // When the native region refuses (pending preparation), math falls back to a guarded
    // draft of its exact envelope. Exact math source is a syntax fact; an unrelated preview
    // may still be preparing. Raster acknowledgment is required only for in-document editors.
    if (const auto document = editor->GetSemanticDocument())
        for (const auto &element : document->Elements())
            if (element.role == DxUi::RichText::SemanticRole::Math && selection.caret >= element.source.first &&
                selection.caret <= WithoutLineEnd(source, element.source))
            {
                first = element.source.first;
                last = WithoutLineEnd(source, element.source);
                break;
            }
    if (first == last)
        if (const auto syntax = editor->GetMarkdownSyntax())
            for (const auto &node : syntax->Inlines())
                if (node.kind == DxUi::RichText::MarkdownInlineKind::Math && selection.caret >= node.source.first &&
                    selection.caret <= node.source.last)
                {
                    first = node.source.first;
                    last = node.source.last;
                    break;
                }
    if (first == last)
    {
        first = first ? source.find_last_of(L"\r\n", first - 1) : source.npos;
        first = first == source.npos ? 0 : first + 1;
        last = source.find_first_of(L"\r\n", selection.caret);
        last = last == source.npos ? source.size() : last;
    }
    blockDraftSnapshot = editor->Snapshot();
    blockDraftRevision = editor->Revision();
    blockDraftRange = {first, last};
    blockDraftText->SetText(source.substr(first, last - first));
    drawer->SetVisible(false);
    findBar->SetVisible(false);
    RefreshFind();
    floating->SetVisible(false);
    blockDraft->GetLogicalChild(0)->SetAccessibleName(Resource(equationContext ? IDS_BLOCKEQUATION : IDS_LOCAL));
    static_cast<DxUi::Label *>(blockDraft->GetLogicalChild(0))->SetText(Resource(equationContext ? IDS_BLOCKEQUATION : IDS_LOCAL));
    blockDraft->SetVisible(true);
    editor->SetReadOnly(true);
    if (last > first)
    {
        const DxUi::RichTextSourceHighlight block{first, last - first, true};
        (void)AcceptSourceResult(editor->TrySetSourceHighlights(std::span(&block, 1)));
    }
    Layout();
    host.SetFocusControl(blockDraftText, MayRequestNativeFocus());
    CheckpointRecovery();
}

void Application::ApplyBlockSource()
{
    if (!blockDraft->IsVisible())
        return;
    if (!editor->Snapshot().IsSameRevision(blockDraftSnapshot) || editor->Revision() != blockDraftRevision)
    {
        Status(IDS_NOTAVAILABLE);
        return;
    }
    const auto draft = blockDraftText->GetText();
    const auto original = blockDraftSnapshot.Slice(blockDraftRange.first, blockDraftRange.last);
    const std::wstring value = draft == original ? std::wstring(draft) : NormalizeInsertedLineEndings(draft, CurrentText());
    if (!DxUi::RichText::IsValidUtf16(value) ||
        blockDraftSnapshot.Length() - (blockDraftRange.last - blockDraftRange.first) + value.size() > DxUi::RichText::Document::MaximumUnits)
    {
        Status(IDS_NOTAVAILABLE);
        return;
    }
    const DxUi::RichText::SourceEdit edit{blockDraftRange, value};
    const auto end = blockDraftRange.first + value.size();
    editor->SetReadOnly(false);
    (void)editor->ApplySourceEdits(blockDraftSnapshot, blockDraftRevision, std::span(&edit, 1), {end, end});
    CloseDrawers();
    Status(IDS_FORMATTED);
}

std::vector<DxUi::MenuFlyoutItem> Application::ContextMenuItems(bool insertOnly, bool headingOnly)
{
    std::vector<DxUi::MenuFlyoutItem> items;
    auto add = [&](unsigned label, Command command, std::wstring accelerator = {})
    {
        DxUi::MenuFlyoutItem item;
        item.text = Resource(label);
        item.commandId = static_cast<int>(command) + 1;
        item.acceleratorText = std::move(accelerator);
        items.push_back(std::move(item));
    };
    if (!insertOnly)
    {
        add(IDS_PARAGRAPH, Command::Paragraph);
        add(IDS_H1, Command::H1);
        add(IDS_H2, Command::H2);
        add(IDS_H3, Command::H3);
        if (!headingOnly)
        {
            add(IDS_BOLD, Command::Bold, L"Ctrl+B");
            add(IDS_ITALIC, Command::Italic, L"Ctrl+I");
            add(IDS_STRIKE, Command::Strike);
            add(IDS_CODE, Command::Code);
            add(IDS_NUMBERED, Command::Numbered);
            add(IDS_LOCAL, Command::LocalSource, L"F2");
            if (CurrentLinkElement(editor->GetSemanticDocument()))
                add(IDS_OPENLINK, Command::OpenLink);
            add(IDS_ROWADD, Command::RowAdd);
            add(IDS_ROWREMOVE, Command::RowRemove);
            add(IDS_COLADD, Command::ColumnAdd);
            add(IDS_COLREMOVE, Command::ColumnRemove);
        }
    }
    if (!headingOnly)
    {
        add(IDS_LINK, Command::Link);
        add(IDS_IMAGE, Command::Image);
        add(IDS_TABLE, Command::Table);
        add(IDS_MERMAID, Command::Mermaid);
        add(IDS_MATH, Command::Math);
        add(IDS_QUOTE, Command::Quote);
        add(IDS_FENCE, Command::Fence);
    }
    return items;
}

void Application::Menu(POINT point, bool insertOnly, bool headingOnly)
{
    if (options.headless)
        return; // Focus-taking menus are an explicitly interactive gate.
    auto items = ContextMenuItems(insertOnly, headingOnly);
    const auto source = editor->Snapshot();
    const auto selection = editor->GetSelection();
    const auto key = activeDocumentKey;
    const auto lifetime = GetControlLifetimeToken(*editor);
    floating->SetVisible(false);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(
        window.get(), point, items, host.GetTheme(),
        [this, source, selection, key, lifetime](std::optional<int> selected) noexcept
        {
            RunUiAction(
                [&]
                {
                    if (selected == static_cast<int>(Command::OpenLink) + 1)
                    {
                        if (closing || lifetime.expired() || !root || host.GetRoot() != root)
                            return;
                        if (activeDocumentKey != key || !editor->Snapshot().IsSameRevision(source) || editor->GetSelection() != selection)
                        {
                            Status(IDS_LINKCHANGED);
                            return;
                        }
                    }
                    OnMenuClosed(selected);
                });
        },
        callbacks);
}

std::filesystem::path Application::PickFile(bool saving, bool image, bool folder)
{
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless && options.filePickerForTest && !image && !folder)
        return options.filePickerForTest(saving);
#endif
    if (options.headless)
        return {};
    wil::com_ptr<IFileDialog> dialog;
    const CLSID classId = saving ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hr = DxUi::Failure::InvokeNative(&CoCreateInstance, classId, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER), __uuidof(IFileDialog),
                                             reinterpret_cast<void **>(dialog.put()));
    if (FAILED(hr))
        return {};
    const auto markdownLabel = Resource(IDS_FILTERMARKDOWN), utf8Label = Resource(IDS_FILTERUTF8), allLabel = Resource(IDS_FILTERALL),
               imageLabel = Resource(IDS_FILTERIMAGES);
    const COMDLG_FILTERSPEC markdown[]{{markdownLabel.c_str(), L"*.md;*.markdown"}, {utf8Label.c_str(), L"*.txt"}, {allLabel.c_str(), L"*.*"}};
    const COMDLG_FILTERSPEC pictures[]{{imageLabel.c_str(), L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"}};
    if (!folder)
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileTypes, image ? 1u : 3u, image ? pictures : markdown);
    else
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetTitle, Resource(IDS_OPENFOLDER).c_str());
    DWORD flags{};
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::GetOptions, &flags);
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetOptions,
                             flags | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR |
                                 (folder   ? FOS_PICKFOLDERS | FOS_PATHMUSTEXIST
                                  : saving ? FOS_OVERWRITEPROMPT
                                           : FOS_FILEMUSTEXIST));
    if (!settings.filesRoot.empty())
    {
        wil::com_ptr<IShellItem> initial;
        if (SUCCEEDED(DxUi::Failure::InvokeNative(&SHCreateItemFromParsingName, settings.filesRoot.c_str(), nullptr, __uuidof(IShellItem),
                                                  reinterpret_cast<void **>(initial.put()))))
            (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultFolder, initial.get());
    }
    if (saving)
    {
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultExtension, L"md");
        const auto baseName =
            documentPath.filename().empty() ? std::filesystem::path(Resource(IDS_UNTITLED)).stem().native() : documentPath.stem().native();
        const auto name = recovered              ? std::vformat(Resource(IDS_RECOVEREDNAME), std::make_wformat_args(baseName))
                          : documentPath.empty() ? Resource(IDS_UNTITLED)
                                                 : documentPath.filename().native();
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileName, name.c_str());
    }
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::Show, window.get())))
        return {};
    wil::com_ptr<IShellItem> item;
    wil::unique_cotaskmem_string path;
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::GetResult, item.put())) ||
        FAILED(InvokeNativeMethod(item.get(), &IShellItem::GetDisplayName, SIGDN_FILESYSPATH, path.put())))
        return {};
    return std::filesystem::path(path.get());
}

void Application::AskThen(std::function<void()> continuation)
{
    if (unsavedPromptOpen)
        return;
    if (blockDraft->IsVisible())
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    if (!Dirty())
    {
        continuation();
        return;
    }
    if (options.headless
#if defined(REDTILDE_ENABLE_TESTS)
        && !options.unsavedPromptForTest
#endif
    )
        return;
    int choice = IDCANCEL;
    {
        // A modal loop can dispatch another Close. The original question alone
        // owns this continuation until its answer has been collected.
        unsavedPromptOpen = true;
        const auto restore = wil::scope_exit([&] { unsavedPromptOpen = false; });
#if defined(REDTILDE_ENABLE_TESTS)
        if (options.headless)
            choice = options.unsavedPromptForTest();
        else
#endif
            choice = DxUi::Failure::InvokeNative(&MessageBoxW, window.get(), Resource(IDS_SAVEQUESTION).c_str(), Resource(IDS_APP).c_str(),
                                                 static_cast<UINT>(MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3));
    }
    if (choice == IDNO)
        continuation();
    else if (choice == IDYES)
    {
        afterSave = std::move(continuation);
        Save();
    }
}
void Application::AskWorkspaceThen(std::function<void()> continuation, std::vector<WorkspaceApproval> approved)
{
    if (closing || closingAfterSave || unsavedPromptOpen || switchingDocument || sessionEnding || !root || host.GetRoot() != root)
        return;
    const auto accepted = [&](unsigned key, const DxUi::RichText::SourceSnapshot &source)
    { return std::ranges::any_of(approved, [&](const auto &value) { return value.key == key && value.source.IsSameRevision(source); }); };
    if (!Dirty() || accepted(activeDocumentKey, editor->Snapshot()))
    {
        const auto pending = std::find_if(inactiveDocuments.begin(), inactiveDocuments.end(), [&](const auto &document)
                                          { return document.dirty && !accepted(document.recoveryKey, document.state.Snapshot()); });
        if (pending == inactiveDocuments.end())
        {
            workspaceApprovals = std::move(approved);
            continuation();
            return;
        }
        if (!SwitchWorkspaceDocument(static_cast<std::size_t>(pending - inactiveDocuments.begin())))
            return;
        if (closing || !root || host.GetRoot() != root)
            return;
    }
    const auto promptingSource = editor->Snapshot();
    const auto promptingKey = activeDocumentKey;
    AskThen(
        [this, promptingSource, promptingKey, continuation = std::move(continuation), approved = std::move(approved)]() mutable
        {
            approved.push_back({promptingKey, promptingSource});
            AskWorkspaceThen(std::move(continuation), std::move(approved));
        });
}
void Application::Open()
{
    AskWorkspaceThen(
        [this]
        {
            const auto path = PickFile(false);
            if (path.empty())
                return;
            openingSnapshot = editor->Snapshot();
            const HRESULT hr = files.Submit(false, path);
            if (FAILED(hr))
                Status(IDS_BUSY);
            else
            {
                openingFromFiles = false;
                openingApprovals = workspaceApprovals;
                Status(IDS_LOADING);
            }
        });
}
void Application::Save(bool saveAs, bool force, std::optional<std::filesystem::path> selectedPath)
{
    if (closing || !root || host.GetRoot() != root)
        return;
    saveAs = saveAs || recovered;
    if (files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    const auto source = editor->Snapshot();
    const auto key = activeDocumentKey;
    const auto originalPath = documentPath;
    const auto originalStamp = stamp;
    const bool originalBom = bom, originalRecovered = recovered;
    auto path = selectedPath ? selectedPath.value() : documentPath;
    if (!selectedPath && (saveAs || path.empty()))
        path = PickFile(true);
    if (path.empty())
    {
        afterSave = {};
        return;
    }
    if (closing || !root || host.GetRoot() != root)
    {
        afterSave = {};
        return;
    }
    if (key != activeDocumentKey || documentPath != originalPath || stamp != originalStamp || bom != originalBom || recovered != originalRecovered ||
        !editor->Snapshot().IsSameRevision(source))
    {
        afterSave = {};
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    const auto expected = (!force && !documentPath.empty()) ? std::optional<FileStamp>(stamp) : std::nullopt;
    const bool sameIdentityOnly = saveAs && path.lexically_normal() != documentPath.lexically_normal();
    const SaveRecoveryContext protection{options.dataDirectory, editor->GetSelection(), ++recoveryGeneration};
    const HRESULT hr = files.Submit(true, path, editor->GetText(), bom, expected, sameIdentityOnly, false, documentPath, protection);
    if (FAILED(hr))
    {
        afterSave = {};
        Status(IDS_SAVEFAILED);
    }
    else
        Status(IDS_SAVING);
}
void Application::ResolveSaveConflict(const AsyncFiles::Result &result)
{
    if (closing || !root || host.GetRoot() != root)
    {
        afterSave = {};
        return;
    }
    const auto source = editor->Snapshot();
    const auto key = activeDocumentKey;
    const auto path = documentPath;
    const auto identity = stamp;
    const bool originalBom = bom, originalRecovered = recovered;
    Status(IDS_CONFLICT);
    SaveConflictChoice choice = SaveConflictChoice::Cancel;
    {
        unsavedPromptOpen = true;
        const auto restore = wil::scope_exit([&] { unsavedPromptOpen = false; });
#if defined(REDTILDE_ENABLE_TESTS)
        if (options.headless && options.saveConflictForTest)
            choice = options.saveConflictForTest();
        else
#endif
            if (!options.headless)
        {
            const auto dialogTitle = Resource(IDS_APP), instruction = Resource(IDS_CONFLICT), content = Resource(IDS_CONFLICTHELP);
            const auto reload = Resource(IDS_RELOAD), saveAs = Resource(IDS_SAVEAS), overwrite = Resource(IDS_OVERWRITE),
                       cancel = Resource(IDS_CANCEL);
            const TASKDIALOG_BUTTON buttons[]{
                {IDS_RELOAD, reload.c_str()}, {IDS_SAVEAS, saveAs.c_str()}, {IDS_OVERWRITE, overwrite.c_str()}, {IDCANCEL, cancel.c_str()}};
            TASKDIALOGCONFIG config{};
            config.cbSize = sizeof(config);
            config.hwndParent = window.get();
            config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
            config.pszWindowTitle = dialogTitle.c_str();
            config.pszMainIcon = TD_WARNING_ICON;
            config.pszMainInstruction = instruction.c_str();
            config.pszContent = content.c_str();
            config.cButtons = static_cast<UINT>(std::size(buttons));
            config.pButtons = buttons;
            config.nDefaultButton = IDCANCEL;
            int pressed = IDCANCEL;
            if (SUCCEEDED(DxUi::Failure::InvokeNative(&TaskDialogIndirect, &config, &pressed, nullptr, nullptr)))
            {
                if (pressed == IDS_RELOAD)
                    choice = SaveConflictChoice::Reload;
                else if (pressed == IDS_SAVEAS)
                    choice = SaveConflictChoice::SaveAs;
                else if (pressed == IDS_OVERWRITE)
                    choice = SaveConflictChoice::Overwrite;
            }
        }
    }
    if (closing || !root || host.GetRoot() != root)
    {
        afterSave = {};
        return;
    }
    if (key != activeDocumentKey || path != documentPath || stamp != identity || originalBom != bom || originalRecovered != recovered ||
        !editor->Snapshot().IsSameRevision(source))
    {
        afterSave = {};
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    if (choice == SaveConflictChoice::Overwrite)
        Save(false, true, result.path);
    else if (choice == SaveConflictChoice::SaveAs)
        Save(true);
    else
    {
        afterSave = {};
        if (choice == SaveConflictChoice::Reload)
        {
            conflictReload = ConflictReload{key, source, path};
            if (SUCCEEDED(files.Submit(false, result.path, {}, false, {}, false, true)))
                Status(IDS_LOADING);
            else
            {
                conflictReload.reset();
                Status(IDS_BUSY);
            }
        }
    }
}
void Application::OnMenuClosed(std::optional<int> selected) noexcept
{
    RunUiAction(
        [&]
        {
            if (closing || !root || host.GetRoot() != root)
                return;
            if (selected)
                Execute(static_cast<Command>(selected.value() - 1));
            // Commands may retire the root or open a draft/drawer that owns focus.
            if (!closing && root && host.GetRoot() == root && !HasBlockDraft() && !drawer->IsVisible() && !findBar->IsVisible() &&
                !recoveryPanel->IsVisible())
                host.SetFocusControl(editor, MayRequestNativeFocus());
        });
}
void Application::FileCompleted() noexcept
{
    RunUiAction([&] { CompleteFileOperation(); });
}
void Application::CompleteFileOperation()
{
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.fileCompletionAdmissionForTest)
    {
        const HRESULT admitted = options.fileCompletionAdmissionForTest();
        if (FAILED(admitted))
        {
            ReportUiFailure("checked file-completion admission refusal");
            return;
        }
    }
#endif
    auto completed = heldOpen && !HasBlockDraft() ? std::exchange(heldOpen, std::nullopt) : files.Take();
    if (!completed)
        return;
    if (!completed->saving && HasBlockDraft())
    {
        heldOpen = std::move(completed);
        Status(IDS_BLOCKPENDING);
        return;
    }
    auto &result = completed.value();
    const bool automaticStartup = !result.saving && std::exchange(startupOpening, false);
    if (automaticStartup)
    {
        const auto admitted = std::exchange(startupSource, std::nullopt);
        if (!admitted || !editor->Snapshot().IsSameRevision(admitted.value()) || !recoveryCandidates.empty() || sessionEnding)
        {
            openingSnapshot.reset();
            openingFromFiles = false;
            Status(IDS_FILECONTEXTCHANGED);
            return;
        }
    }
    if (FAILED(result.status))
    {
        if (result.reloading)
            conflictReload.reset();
        const bool failedClose = closingAfterSave;
        if (failedClose)
            ResumeAfterSaveClose();
        MonitorDiagnostics::Write(RedMonitor::Severity::Error,
                                  result.saving ? L"save failed; source and previous target retained" : L"open failed; current document retained");
        if (result.saving && result.status == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && !failedClose)
        {
            ResolveSaveConflict(result);
            return;
        }
        std::wstring message;
        if (result.status == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION))
            message = Resource(result.saving ? IDS_SOURCEENCODING : IDS_FILENOTUTF8);
        else if (result.status == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE))
        {
            constexpr auto limit = DxUi::RichText::Document::MaximumUnits;
            message = result.rejectedUnits ? std::vformat(Resource(IDS_FILETOOLARGECOUNT), std::make_wformat_args(result.rejectedUnits, limit))
                                           : std::vformat(Resource(IDS_FILETOOLARGELIMIT), std::make_wformat_args(limit));
        }
        else
        {
            const auto error = ErrorText(result.status);
            message = std::vformat(Resource(result.saving ? IDS_SAVEFAILED : IDS_OPENFAILED), std::make_wformat_args(error));
        }
        if (!options.headless && !failedClose && !automaticStartup)
            (void)DxUi::Failure::InvokeNative(&MessageBoxW, window.get(), message.c_str(), Resource(IDS_ERROR).c_str(),
                                              static_cast<UINT>(MB_OK | MB_ICONERROR));
        feedback->SetText(message);
        if (!result.saving)
        {
            openingFromFiles = false;
            if (automaticStartup)
                openingSnapshot.reset();
        }
        afterSave = {};
        return;
    }
    if (result.reloading)
    {
        const auto request = std::exchange(conflictReload, std::nullopt);
        if (closing || !root || host.GetRoot() != root)
            return;
        if (!request || activeDocumentKey != request->key || documentPath != request->path || !editor->Snapshot().IsSameRevision(request->source))
        {
            Status(IDS_FILECONTEXTCHANGED);
            return;
        }
        if (SetDocument(std::move(result.document.text), std::move(result.path), result.document.bom, result.document.stamp))
            Status(IDS_SAVED);
        return;
    }
    if (result.saving)
    {
        recovered = false;
        savedText = std::move(result.document.text);
        dirtyFact.reset();
        documentPath = std::move(result.path);
        stamp = result.document.stamp;
        EnsureFilesRoot();
        RefreshFiles(true);
        if (!AcceptSourceResult(editor->TrySetDocumentFilePath(documentPath.native())))
            return;
        RememberActiveDocument();
        Changed();
        Status(IDS_SAVED);
        if (closingAfterSave)
        {
            if (Dirty())
            {
                // Save owns only its admitted immutable source. Closing cannot
                // discard edits typed after that snapshot was submitted.
                ResumeAfterSaveClose();
                AskWorkspaceThen([this] { BeginClose(); });
            }
            else
            {
                if (std::ranges::any_of(inactiveDocuments, [](const auto &document) { return document.dirty; }))
                {
                    ResumeAfterSaveClose();
                    AskWorkspaceThen([this] { BeginClose(); });
                }
                else
                {
                    closingAfterSave = false;
                    visibleBeforeSaveClose = false;
                    BeginClose();
                }
            }
            return;
        }
        if (afterSave)
        {
            auto continuation = std::move(afterSave);
            afterSave = {};
            AskThen(std::move(continuation)); // edits typed during a save remain protected
        }
    }
    else
    {
        // User may have typed while a slow open ran. Recheck dirty state before
        // replacing the source; this is a new consent decision, not a stale one.
        auto staged = std::make_shared<AsyncFiles::Result>(std::move(result));
        const bool fromFiles = std::exchange(openingFromFiles, false);
        const auto entrySource = std::exchange(openingSnapshot, std::nullopt);
        auto approvals = std::exchange(openingApprovals, {});
        auto apply = [this, staged]
        {
            if (SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp))
            {
                ResetWorkspace();
                RefreshFiles();
                Status(IDS_SAVED);
            }
        };
        if (fromFiles)
        {
            const auto previousCount = inactiveDocuments.size();
            if (StartWorkspaceDocument() && !closing && host.GetRoot() == root)
            {
                if (SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp))
                    Status(IDS_SAVED);
                else if (inactiveDocuments.size() > previousCount && SwitchWorkspaceDocument(previousCount))
                    inactiveDocuments.pop_back(); // Retire only the new empty slot; the prior file/history is active again.
            }
        }
        else if (entrySource && editor->Snapshot().IsSameRevision(entrySource.value()))
            apply();
        else
            AskWorkspaceThen(std::move(apply), std::move(approvals));
    }
}
std::optional<std::wstring> Application::BeginBitmapPaste(std::span<const std::byte> dib, bool v5)
{
    if (closing || sessionEnding || closingAfterSave || switchingDocument || unsavedPromptOpen || !editor || editor->IsReadOnly() ||
        settings.view == EditorView::Source || host.GetFocusControl() != editor || editor->IsSelectionGestureActive() ||
        (blockDraft && blockDraft->IsVisible()))
        return std::nullopt;
    PendingBitmapPaste pending;
    pending.documentKey = activeDocumentKey;
    pending.source = editor->Snapshot();
    pending.selection = editor->GetSelection();
    pending.view = settings.view;
    pending.documentPath = documentPath;
    pending.lifetime = GetControlLifetimeToken(*editor);
    const auto submitted = bitmapWriter.Submit(dib, v5, pending.request);
    if (FAILED(submitted))
    {
        Status(submitted == HRESULT_FROM_WIN32(ERROR_BUSY) ? IDS_IMAGEPASTEBUSY : IDS_IMAGEPASTEFAILED);
        return std::nullopt;
    }
    pendingBitmapPaste = std::move(pending);
    Status(IDS_IMAGEPASTING);
    // The synchronous SDK callback owns no I/O. This application completes its
    // separately guarded source transaction after the durable worker result.
    return std::nullopt;
}
void Application::ServiceBitmapPaste()
{
    BitmapPasteWriter::Result result;
    if (!bitmapWriter.Take(result))
        return;
    auto pending = std::exchange(pendingBitmapPaste, std::nullopt);
    bool keep{};
    const auto settle = wil::scope_exit(
        [&]
        {
            if (keep)
                bitmapWriter.Commit(result.request);
            else if (FAILED(bitmapWriter.Discard(std::move(result))))
                OutputDebugStringW(L"RedTilde: a declined clipboard asset was retained after a cleanup scheduling error.\n");
        });
    if (closing || sessionEnding)
        return;
    if (FAILED(result.status))
    {
        Status(IDS_IMAGEPASTEFAILED);
        return;
    }
    if (!pending || unsavedPromptOpen || pending->request != result.request || pending->lifetime.expired() ||
        pending->documentKey != activeDocumentKey || pending->view != settings.view || pending->documentPath != documentPath || !editor ||
        editor->IsReadOnly() || host.GetFocusControl() != editor || editor->IsSelectionGestureActive() ||
        editor->GetSelection() != pending->selection ||
        editor->Snapshot().ChangedRangeFrom(pending->source).kind != DxUi::RichText::SourceChangeKind::Unchanged ||
        (blockDraft && blockDraft->IsVisible()))
    {
        Status(IDS_IMAGEPASTECANCELLED);
        return;
    }
    const auto lifetime = pending->lifetime;
    const auto alt = Resource(IDS_PASTEDIMAGE);
    DxUi::RichText::MarkdownCommandRequest request;
    request.command = DxUi::RichText::MarkdownCommand::InsertImage;
    request.text = alt;
    request.destination = result.asset.uri;
    keep = editor->ExecuteMarkdownCommand(request) == DxUi::RichText::MarkdownCommandStatus::Changed;
    if (!closing && !lifetime.expired())
        Status(keep ? IDS_IMAGEPASTED : IDS_IMAGEPASTEFAILED);
}
void Application::RefreshImageConsent()
{
    if (!editor || !imageConsent || closing)
        return;
    auto *const owner = root;
    if (!owner || host.GetRoot() != owner)
        return;
    const auto lifetime = GetControlLifetimeToken(*owner);
    const auto semantic = editor->GetSemanticDocument();
    const auto current = [&]() noexcept
    {
        return !lifetime.expired() && !closing && root == owner && host.GetRoot() == owner && editor && editor->GetSemanticDocument() == semantic &&
               imageConsentSource == semantic;
    };
    if (semantic != imageConsentSource || imageConsentPath != documentPath)
    {
        imageConsentSource = semantic;
        imageConsentPath = documentPath;
        imageConsentHasNetwork = false;
        imageFailureStatus = S_OK;
        if (semantic)
            for (const auto &element : semantic->Elements())
                if (element.role == DxUi::RichText::SemanticRole::Image)
                {
                    const auto &value = element.destination;
                    const auto scheme = [&](std::wstring_view prefix)
                    {
                        return value.size() >= prefix.size() && CompareStringOrdinal(value.data(), static_cast<int>(prefix.size()), prefix.data(),
                                                                                     static_cast<int>(prefix.size()), TRUE) == CSTR_EQUAL;
                    };
                    // This only selects explanatory chrome; the SDK and transport independently authorize requests.
                    if (scheme(L"http://") || scheme(L"https://") || (scheme(L"file://") && value.size() > 7u && value[7u] != L'/') ||
                        value.starts_with(L"//") || value.starts_with(L"\\\\") || documentPath.native().starts_with(L"\\\\"))
                    {
                        imageConsentHasNetwork = true;
                        break;
                    }
                }
    }
    const bool enabled = settings.allowRemoteImages || documentRemoteImagesAllowed;
    unsigned text = static_cast<unsigned>(!enabled                     ? IDS_IMAGECONSENTBLOCKED
                                          : settings.allowRemoteImages ? IDS_IMAGECONSENTSETTINGS
                                                                       : IDS_IMAGECONSENTDOCUMENT);
    if (FAILED(imageFailureStatus))
        text = static_cast<unsigned>(imageFailureStatus == E_ACCESSDENIED                             ? IDS_IMAGEPOLICYFAILED
                                     : imageFailureStatus == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) ? IDS_IMAGETOOLARGE
                                     : imageFailureStatus == HRESULT_FROM_WIN32(ERROR_TIMEOUT) ||
                                             imageFailureStatus == HRESULT_FROM_WIN32(ERROR_WINHTTP_TIMEOUT)
                                         ? IDS_IMAGETIMEOUT
                                         : IDS_IMAGELOADFAILED);
    if (imageConsentText->GetText() != ResourceView(text))
        imageConsentText->SetText(Resource(text));
    if (!current())
        return;
    const unsigned action = documentRemoteImagesAllowed ? IDS_IMAGEBLOCKHERE : IDS_IMAGEALLOWHERE;
    if (imageConsentAction->GetText() != ResourceView(action))
        imageConsentAction->SetText(Resource(action));
    if (!current())
        return;
    imageConsentAction->SetVisible(imageConsentHasNetwork && !settings.allowRemoteImages);
    if (!current())
        return;
    imageReload->SetEnabled(true);
    if (!current())
        return;
    const bool visible = imageConsentHasNetwork || FAILED(imageFailureStatus);
    if (imageConsent->IsVisible() != visible)
    {
        imageConsent->SetVisible(visible);
        if (current())
            Layout();
    }
}

void Application::ServiceImages()
{
    if (!editor || closing || imageBindingTransition || switchingDocument)
        return;
    auto *const primary = editor;
    const auto lifetime = GetControlLifetimeToken(*primary);
    const auto current = [&]() noexcept { return !lifetime.expired() && !closing && editor == primary; };
    const auto source = primary->Snapshot();
    const bool watchBindingChanged = !source.IsSameRevision(imageWatchSource) || imageWatchDocumentPath != documentPath;
    if (watchBindingChanged)
    {
        imageWatchSource = source;
        imageWatchDocumentPath = documentPath;
        imageWatchReloadPending = false;
    }
    if (const auto notice = imageFiles.TakeResult())
    {
        if (FAILED(notice.value().status))
            imageFailureStatus = notice.value().status;
        else
        {
            imageWatchReloadPending = true;
            ++imageFileRefreshes;
            primary->ReloadImages();
            if (!current())
                return;
        }
    }
    RefreshImageConsent();
    if (!current())
        return;
    editor->TakePendingImageRequests(imageRequests);
    images.SetDocumentContext(documentPath.native());
    const auto previousWatchCount = watchedImageRequests.size();
    std::erase_if(watchedImageRequests, [](const auto &request) { return request.Cancelled(); });
    bool watchChanged = watchBindingChanged || previousWatchCount != watchedImageRequests.size();
    for (const auto &request : imageRequests)
        if (request.SourceKind() == DxUi::RichText::RichImageSourceKind::LocalFile &&
            std::ranges::find(watchedImageRequests, request) == watchedImageRequests.end() && watchedImageRequests.size() < 32u)
        {
            watchedImageRequests.push_back(request);
            watchChanged = true;
        }
    if (watchChanged && (!imageWatchReloadPending || !imageRequests.empty()))
    {
        std::vector<std::filesystem::path> paths;
        paths.reserve(watchedImageRequests.size());
        for (const auto &request : watchedImageRequests)
            paths.emplace_back(request.FilePath());
        const auto status = imageFiles.SetFiles(paths);
        imageWatchReloadPending = false;
        if (FAILED(status))
        {
            (void)imageFiles.SetFiles({}); // Retire the old binding when automatic watching cannot be admitted.
            imageFailureStatus = status;
        }
    }
    images.DiscardCancelled();
    if (!imageRequests.empty())
        (void)images.Submit(imageRequests);
    if (images.HasResults())
    {
        std::array<RichImageLoader::Result, RichImageLoader::MaximumResults> results;
        const auto count = images.Drain(results);
        for (std::size_t index = 0; index < count; ++index)
        {
            const bool accepted = primary->CompleteImageRequest(results[index].request, std::move(results[index].image));
            if (!current())
                return;
            if (accepted && FAILED(results[index].status) && results[index].status != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                imageFailureStatus = results[index].status;
        }
        RefreshImageConsent();
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_NCCREATE)
    {
        auto *app = static_cast<Application *>(reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    auto *app = reinterpret_cast<Application *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app && message == WM_NCCALCSIZE && wParam)
    {
        auto *size = reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam);
        if (IsZoomed(hwnd))
        {
            const UINT dpi = GetDpiForWindow(hwnd);
            const int edge = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
            InflateRect(&size->rgrc[0], -edge, -edge);
        }
        return 0;
    }
    if (app && app->initialized)
    {
        LRESULT result{};
        app->RunUiAction([&] { result = app->HandleMessage(hwnd, message, wParam, lParam); });
        return result; // A failed Close stays handled instead of reaching DefWindowProc.
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
void Application::BeginClose()
{
    // Reaching this point means the user saved or explicitly discarded source.
    // Retire only this session's snapshot and wait asynchronously for its result.
    AbandonStartupRestore();
    RememberActiveDocument();
    RecordWindowPlacement(true);
    CloseExternalLinks();
    ResetWorkspace();
    (void)recovery.SubmitDocument(activeDocumentKey, std::nullopt, true);
    recovery.Stop();
    closing = true;
    pendingBitmapPaste.reset();
    bitmapWriter.Stop();
    files.Close();
    folderFiles.Close();
    images.Shutdown();
    imageFiles.Close();
    host.Detach();
    waitingForPreferences = preferenceWriter.Busy() || !recovery.Finished() || bitmapWriter.Busy();
    if (waitingForPreferences)
    {
        // Keep only a hidden notification window until admitted preferences are
        // durable. The UI never joins storage, and process exit cannot kill the
        // worker between its final write and atomic replacement.
        ShowWindow(window.get(), SW_HIDE);
    }
    else
    {
        preferenceWriter.Close();
        bitmapWriter.Detach();
        window.reset();
    }
}
void Application::ResumeAfterSaveClose()
{
    closingAfterSave = false;
    // Restore the existing placement without interrupting whoever has focus
    // now. Headless harness windows must remain hidden on every failure path.
    if (std::exchange(visibleBeforeSaveClose, false) && !options.headless)
        ShowWindow(window.get(), SW_SHOWNA);
}
void Application::FinishClose()
{
    if (closing && !preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy())
    {
        waitingForPreferences = false;
        recovery.Close();
        preferenceWriter.Close();
        bitmapWriter.Detach();
        window.reset();
    }
}
LRESULT Application::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == BitmapReady)
    {
        if (wParam == BitmapPasteWriter::Cookie)
        {
            ServiceBitmapPaste();
            FinishClose();
        }
        return 0;
    }
    if (message == ExternalLinkReady)
    {
        ExternalLinkCompleted();
        return 0;
    }
    if (message == WM_QUERYENDSESSION)
    {
        if (!closing)
        {
            AbandonStartupRestore();
            RecordWindowPlacement(true);
            CheckpointWorkspaceRecovery();
        }
        return TRUE; // Honor shutdown immediately; never create a shutdown blocker.
    }
    if (message == WM_ENDSESSION)
    {
        sessionEnding = wParam != FALSE;
        if (sessionEnding)
        {
            pendingBitmapPaste.reset();
            bitmapWriter.Stop();
            if (!closing)
                CheckpointWorkspaceRecovery();
            // Windows may terminate us when this message returns. Only this
            // final system boundary waits; recovery never needs a UI callback.
            sessionEndRecoveryStatus = recovery.WaitForSettlement(2000);
            if (FAILED(sessionEndRecoveryStatus))
                MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Session-end recovery did not settle within its bounded allowance");
            // End the ordinary process loop without the clean-close path that
            // retires dirty copies. A hidden fixture owns only its test window,
            // never the parent harness's message loop (as for WM_DESTROY).
            if (!options.headless)
                PostQuitMessage(0);
        }
        return 0;
    }
    if (message == WM_CLOSE)
    {
        if (closing || closingAfterSave || unsavedPromptOpen || switchingDocument)
            return 0;
        if (blockDraft->IsVisible())
        {
            Status(IDS_BLOCKPENDING);
            host.SetFocusControl(blockDraftText, MayRequestNativeFocus());
            return 0;
        }
        AbandonStartupRestore();
        if (files.Busy())
        {
            if (files.Saving())
            {
                // Keep the host and notification HWND alive until the admitted
                // write/replace reports success. Recovery remains available if
                // storage fails or the process ends while the save is pending.
                CheckpointRecovery(true);
                afterSave = {};
                closingAfterSave = true;
                visibleBeforeSaveClose = IsWindowVisible(window.get()) != FALSE;
                ShowWindow(window.get(), SW_HIDE);
                return 0;
            }
            // An Open has no write to settle. Invalidate that worker's state
            // before the ordinary dirty question; Cancel must also refuse its
            // late result, and Save must be free to admit a new operation.
            files.Close();
            files.Initialize(window.get(), FileReady);
#if defined(REDTILDE_ENABLE_TESTS)
            files.SetTestGate(options.fileGateForTest);
#endif
            openingSnapshot.reset();
        }
        heldOpen.reset();
        openingFromFiles = false;
        AskWorkspaceThen([this] { BeginClose(); });
        return 0;
    }
    if ((message == WM_NCLBUTTONDOWN || message == WM_NCLBUTTONDBLCLK) && (wParam == HTCLOSE || wParam == HTMAXBUTTON || wParam == HTMINBUTTON) &&
        FrameHitTest(hwnd, lParam) == static_cast<LRESULT>(wParam))
    {
        // Caption controls are rendered in the client area, but Windows routes
        // their hit-tested presses through the nonclient message family. Give
        // the real DxUi control the ordinary client lifecycle so it owns capture
        // and can cancel activation when the pointer is released elsewhere.
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (ScreenToClient(hwnd, &point))
        {
            const LPARAM clientPoint = MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y));
            bool handled{};
            const UINT clientMessage = message == WM_NCLBUTTONDBLCLK ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN;
            const auto result = host.HandleMessage(hwnd, clientMessage, MK_LBUTTON, clientPoint, handled);
            if (handled)
                return result;
        }
    }
    if (message == WM_NCLBUTTONUP && GetCapture() == hwnd)
    {
        // Normally SetCapture retargets release to WM_LBUTTONUP. Keep a guarded
        // fallback for hosts that still receive the release as nonclient input.
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (ScreenToClient(hwnd, &point))
        {
            bool handled{};
            const auto result =
                host.HandleMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y)), handled);
            if (handled)
                return result;
        }
    }
    if (message == WM_NCHITTEST)
        return FrameHitTest(hwnd, lParam);
    if (message == WM_NCACTIVATE)
        return DefWindowProcW(hwnd, message, wParam, -1);
    if (message == WM_DESTROY)
    {
        if (!options.headless)
            PostQuitMessage(0);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        bitmapWriter.Stop();
        bitmapWriter.Detach();
        folderFiles.Close();
        imageFiles.Close();
        folderFilesActive = false;
        host.Detach();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_GETMINMAXINFO)
    {
        auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
        const UINT dpi = GetDpiForWindow(hwnd);
        MONITORINFO monitor{sizeof(monitor)};
        const bool available = GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE;
        info->ptMinTrackSize = {
            available ? std::min(static_cast<LONG>(MulDiv(800, static_cast<int>(dpi), 96)), monitor.rcWork.right - monitor.rcWork.left)
                      : MulDiv(800, static_cast<int>(dpi), 96),
            available ? std::min(static_cast<LONG>(MulDiv(560, static_cast<int>(dpi), 96)), monitor.rcWork.bottom - monitor.rcWork.top)
                      : MulDiv(560, static_cast<int>(dpi), 96)};
        return 0;
    }
    if (message == PreferencesReady)
    {
        if (waitingForPreferences)
        {
            FinishClose();
        }
        else if (!closing && FAILED(preferenceWriter.Status()))
            Status(IDS_SETTINGSFAILED);
        return 0;
    }
    if (message == RecoveryReadyMessage)
    {
        RecoveryCompleted();
        return 0;
    }
    if (closing)
        return DefWindowProcW(hwnd, message, wParam, lParam);
    if (message == FolderReady)
    {
        FolderCompleted();
        return 0;
    }
    if (message == FileReady)
    {
        FileCompleted();
        return 0;
    }
    if (closingAfterSave)
    {
        const bool nativeLifecycle = message == WM_SHOWWINDOW || message == WM_KILLFOCUS || message == WM_ACTIVATE || message == WM_ACTIVATEAPP ||
                                     message == WM_CANCELMODE || message == WM_CAPTURECHANGED || message == WM_SIZE || message == WM_DPICHANGED;
        if (!nativeLifecycle)
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == ImageReady || message == ImageFileChanged)
    {
        if (wParam == 0x5254)
            ServiceImages();
        return 0;
    }
    if (message == WM_SETTINGCHANGE || message == WM_THEMECHANGED)
        ApplyTheme();
    if (message == WM_MOUSEWHEEL)
    {
        if ((GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0 && (GetKeyState(VK_MENU) & 0x8000) == 0)
        {
            const int accumulated = zoomWheelRemainder + GET_WHEEL_DELTA_WPARAM(wParam);
            const int steps = accumulated / WHEEL_DELTA;
            zoomWheelRemainder = accumulated % WHEEL_DELTA;
            if (steps != 0)
                RunUiAction([&]
                            { ExecuteCommand(steps > 0 ? Command::ZoomIn : Command::ZoomOut, static_cast<unsigned>(steps > 0 ? steps : -steps)); });
            return 0;
        }
        zoomWheelRemainder = 0;
    }
    bool handled{};
    const auto result = host.HandleMessage(hwnd, message, wParam, lParam, handled);
    if (message == WM_MOVE || message == WM_SIZE || message == WM_EXITSIZEMOVE || message == WM_DPICHANGED)
    {
        const bool wasMaximized = settings.windowPlacement && settings.windowPlacement.value().maximized;
        const bool commitPlacement =
            message == WM_EXITSIZEMOVE || message == WM_DPICHANGED || (!IsIconic(hwnd) && wasMaximized != (IsZoomed(hwnd) != FALSE));
        RecordWindowPlacement(commitPlacement);
    }
    if (message == WM_SIZE || message == WM_DPICHANGED ||
        ((message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) && blockDraft && blockDraft->IsVisible()))
        Layout();
    if (message == WM_PAINT)
    {
        ServiceImages();
        // Published syntax can arrive after a committed source/selection event.
        // Observe its immutable identity once; never parse in the paint path.
        if (!closing && (pendingSelectionTools || editor->GetMarkdownSyntax() != selectionFactsSyntax))
            SelectionChanged(pendingSelectionTools || floating->IsVisible());
        if (!closing && pendingBlockPlacement)
            Layout();
    }
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}

void Application::Prepare(unsigned turns)
{
    if (closing || !host.GetRoot())
        return;
    const auto cosmeticsPending = [](const DxUi::RichTextEditor *target)
    {
        if (!target || !target->IsVisible())
            return false;
        const auto state = target->DebugGetState();
        return state.markdownPreparationPending || state.sourceInkPreparationPending || state.codeInkPreparationPending ||
               state.sourceGuidesPreparationPending;
    };
    for (unsigned index = 0; index < turns; ++index)
    {
        if (bitmapWriter.HasResult())
            ServiceBitmapPaste();
        if (closing || !host.GetRoot())
            return;
        (void)host.RenderInitialFrameForShow();
        const bool identityRelayout = identityMeasure.typographyRevision != host.GetTypographyRevision();
        if (identityRelayout)
            Layout();
        ServiceImages();
        MSG message{};
        unsigned processed{};
        while (processed++ < 64 && PeekMessageW(&message, window.get(), 0, 0, PM_REMOVE))
        {
            if (!host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (closing || !host.GetRoot())
            return;
        if (files.Busy())
            FileCompleted(); // also drains a result if its post met a full queue
        RecoveryCompleted();
        FolderCompleted();
        if (closing || !host.GetRoot())
            return;
        // A render may publish new immutable syntax. Reconcile dependent chrome
        // before declaring quiescence, then paint it on the next turn. Hidden
        // hosts deliberately do not receive invalidation-driven WM_PAINT.
        const bool refreshFacts = pendingSelectionTools || editor->GetMarkdownSyntax() != selectionFactsSyntax;
        const bool refreshed = refreshFacts || pendingBlockPlacement || identityRelayout;
        if (refreshFacts)
            SelectionChanged(pendingSelectionTools || floating->IsVisible());
        if (pendingBlockPlacement)
            Layout();
        // This bounded harness drain also settles optional pixels for captures.
        // Production input/frame readiness remains the host's earlier content
        // boundary; its event loop never calls this synchronous convenience loop.
        if (!refreshed && !host.HasPendingContentPreparation() && !cosmeticsPending(editor) && !cosmeticsPending(mirror) &&
            editor->GetImageResourceStatistics().pendingSources == 0 && !files.Busy() && recovery.Ready() &&
            (!folderFilesActive || folderSnapshot || FAILED(folderFiles.Status())))
            break;
        // Harness-only bounded event wait; production blocks in GetMessage.
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}
bool Application::Capture(const std::filesystem::path &path, bool prepareContent)
{
    if (prepareContent)
        Prepare();
    DxUi::Button *captureHover{};
    if (options.headless && path == options.capture && (options.scenario == L"caption-close-hover" || options.scenario == L"caption-maximize-hover"))
    {
        const bool close = options.scenario == L"caption-close-hover";
        captureHover = close ? closeButton : maximizeButton;
        const auto bounds = captureHover->GetBounds();
        const auto point = host.DipPointToScreenPoint({(bounds.left + bounds.right) * .5f, (bounds.top + bounds.bottom) * .5f});
        bool handled{};
        // Preparation pumps queued leave messages for this hidden window.
        // Apply owned synthetic hover after that pump, immediately before the
        // frame readback, without moving the pointer or taking native input.
        (void)host.HandleMessage(window.get(), WM_NCMOUSEMOVE, close ? HTCLOSE : HTMAXBUTTON,
                                 MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y)), handled);
        if (handled || !captureHover->IsHovered())
            return false;
    }
    DxUi::WindowHostBitmapCapture bitmap;
    // Stage the completed frame before Present rotates the swap-chain buffers.
    // The canonical hidden capture path never shows or activates this HWND.
    if (!host.DebugCaptureBitmap(bitmap, true))
        return false;
    if (captureHover && !captureHover->IsHovered())
        return false;
    wil::com_ptr<IWICImagingFactory> factory;
    wil::com_ptr<IWICStream> stream;
    wil::com_ptr<IWICBitmapEncoder> encoder;
    wil::com_ptr<IWICBitmapFrameEncode> frame;
    wil::com_ptr<IPropertyBag2> properties;
    if (FAILED(DxUi::Failure::InvokeNative(&CoCreateInstance, CLSID_WICImagingFactory, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER),
                                           __uuidof(IWICImagingFactory), reinterpret_cast<void **>(factory.put()))))
        return false;
    if (FAILED(InvokeNativeMethod(factory.get(), &IWICImagingFactory::CreateStream, stream.put())) ||
        FAILED(InvokeNativeMethod(stream.get(), &IWICStream::InitializeFromFilename, path.c_str(), static_cast<DWORD>(GENERIC_WRITE))))
        return false;
    if (FAILED(InvokeNativeMethod(factory.get(), &IWICImagingFactory::CreateEncoder, GUID_ContainerFormatPng, nullptr, encoder.put())) ||
        FAILED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::Initialize, stream.get(), WICBitmapEncoderNoCache)))
        return false;
    if (FAILED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::CreateNewFrame, frame.put(), properties.put())) ||
        FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::Initialize, properties.get())) ||
        FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::SetSize, bitmap.widthPx, bitmap.heightPx)))
        return false;
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::SetPixelFormat, &format)) ||
        !IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA))
        return false;
    if (FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::WritePixels, bitmap.heightPx, bitmap.widthPx * 4u,
                                  static_cast<UINT>(bitmap.bgraPixels.size()), bitmap.bgraPixels.data())))
        return false;
    return SUCCEEDED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::Commit)) &&
           SUCCEEDED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::Commit));
}

bool Application::RouteShortcut(const MSG &message)
{
    bool shortcut{};
    if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) && (message.hwnd == window.get() || IsChild(window.get(), message.hwnd)))
    {
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0, shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                   alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        std::optional<Command> command;
        if (ctrl && !alt)
        {
            switch (message.wParam)
            {
            case 'N':
                command = Command::New;
                break;
            case 'O':
                command = shift ? Command::OpenFolder : Command::Open;
                break;
            case 'S':
                command = shift ? Command::SaveAs : Command::Save;
                break;
            case 'E':
                command = shift ? Command::Files : Command::Code;
                break;
            case 'K':
                command = Command::Link;
                break;
            case 'F':
                command = Command::Find;
                break;
            case '1':
                command = Command::Compose;
                break;
            case '2':
                command = Command::Source;
                break;
            case '3':
                command = Command::Split;
                break;
            case VK_OEM_COMMA:
                command = Command::Settings;
                break;
            case VK_OEM_PLUS:
            case VK_ADD:
                command = Command::ZoomIn;
                break;
            case VK_OEM_MINUS:
            case VK_SUBTRACT:
                command = Command::ZoomOut;
                break;
            case '0':
            case VK_NUMPAD0:
                command = Command::ZoomReset;
                break;
            }
        }
        if (message.wParam == VK_F3 && !ctrl && !alt)
            command = shift ? Command::FindPrevious : Command::FindNext;
        if (message.wParam == VK_F2 && !ctrl && !shift && !alt)
            command = Command::LocalSource;
        if (message.wParam == VK_F11 && !ctrl && !shift && !alt)
            command = Command::Focus;
        if (message.wParam == VK_F10 && alt && !ctrl && !shift)
        {
            FocusSelectionTools();
            shortcut = true;
        }
        if (command)
        {
            Execute(command.value());
            shortcut = true;
        }
    }
    return shortcut;
}

int Application::Run()
{
    if (!options.designInteractive.empty())
    {
#if defined(REDTILDE_ENABLE_TESTS)
        return RunDesignInteractiveProbe(*this, options);
#else
        return 2;
#endif
    }
    if (!options.performance.empty())
        return RunPerformanceProbe(*this, options);
    if (options.recoveryProbe)
    {
        // Isolated harness fixtures: exercise the real Files worker, independent
        // histories and checkpoint leases before the intentional child crash.
        const auto directory = options.dataDirectory / L"Workspace";
        std::filesystem::create_directories(directory);
        const auto alpha = directory / L"Alpha.md", beta = directory / L"Beta.md";
        if (FAILED(WriteBytesAtomic(alpha, "\xef\xbb\xbf# Alpha\r\n")) || FAILED(WriteBytesAtomic(beta, "# Beta\n")))
            return 6;
        StoredDocument loaded;
        if (FAILED(LoadDocument(alpha, loaded)))
            return 6;
        SetDocument(loaded.text, alpha, loaded.bom, loaded.stamp);
        editor->SetSelection({0, 0});
        (void)editor->ReplaceSelection(L"Recovered after crash: 東京 😀\r\n");
        OpenPath(beta);
        for (unsigned turn = 0; turn < 1000 && documentPath != beta; ++turn)
        {
            Prepare(32);
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        }
        if (documentPath != beta || inactiveDocuments.size() != 1)
            return 6;
        editor->SetSelection({0, 0});
        (void)editor->ReplaceSelection(L"A second unsaved copy: 世界\n");
        CheckpointWorkspaceRecovery();
        for (unsigned turn = 0; turn < 1000 && !recovery.Settled(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        if (!recovery.Settled() || FAILED(recovery.Status()))
            return 6;
        CrashProbe();
    }
    if (options.recoveryRestartProbe)
    {
        Prepare();
        RecoveryCompleted();
        if (recoveryCandidates.size() != 2)
            return 7;
        Execute(Command::RestoreAllRecovery);
        const auto expected = [&](const std::filesystem::path &path, std::wstring_view text, bool hasBom)
        {
            return path.filename() == (hasBom ? L"Alpha.md" : L"Beta.md") &&
                   text == (hasBom ? L"Recovered after crash: 東京 😀\r\n# Alpha\r\n" : L"A second unsaved copy: 世界\n# Beta\n");
        };
        bool correct = Dirty() && recovered && inactiveDocuments.size() == 1 && expected(documentPath, editor->GetText(), bom);
        for (const auto &document : inactiveDocuments)
            correct = correct && document.dirty && document.recovered && expected(document.path, document.state.Text(), document.bom);
        StoredDocument original;
        correct = correct && SUCCEEDED(LoadDocument(options.dataDirectory / L"Workspace/Alpha.md", original)) && original.text == L"# Alpha\r\n" &&
                  original.bom;
        correct = correct && SUCCEEDED(LoadDocument(options.dataDirectory / L"Workspace/Beta.md", original)) && original.text == L"# Beta\n" &&
                  !original.bom;
        CheckpointWorkspaceRecovery();
        recovery.Stop();
        for (unsigned turn = 0; turn < 1000 && !recovery.Finished(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        return correct && recovery.Finished() && SUCCEEDED(recovery.Status()) ? 0 : 8;
    }
    if (options.preferencesProbe)
    {
        Execute(Command::ThemeLight);
        Execute(Command::ThemeDark);
        Execute(Command::ThemePaper);
        (void)SendMessageW(window.get(), WM_CLOSE, 0, 0);
        if (window)
            (void)SendMessageW(window.get(), WM_CLOSE, 0, 0);
        MSG message{};
        while (window && GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            if (!host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        return 0;
    }
    if (options.headless)
    {
        settings.appearance = options.scenario == L"sample-document-dark" || options.scenario == L"task-presentation-dark" ||
                                      options.scenario == L"compose-dark" || options.scenario == L"math-dark" ||
                                      options.scenario == L"quote-scroll-dark" || options.scenario == L"diagram-label-dark" ||
                                      options.scenario == L"reference-dark" || options.scenario == L"markdown-style" ||
                                      options.scenario == L"caption-close-hover" || options.scenario == L"caption-maximize-hover"
                                  ? Appearance::Dark
                                  : Appearance::Light;
        ApplyTheme();
        if (options.scenario == L"math" || options.scenario == L"math-dark")
            SetDocument(L"# Mathematics, in the flow\n\nInline $E=mc^2$ shares the text baseline. Exact TeX stays one F2 away.\n\n"
                        L"$$\nx=\\frac{-b+\\sqrt{b^2-4ac}}{2a}\n$$\n\n"
                        L"$$\nA=\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\\quad \\vec{v}=\\begin{pmatrix}x\\\\y\\end{pmatrix}\n$$\n\n"
                        L"$$\n\\begin{aligned}f(x)&=x^2+1\\\\f'(x)&=2x\\end{aligned}\n$$\n\n"
                        L"Unsupported commands keep their exact source: $\\input{private}$.\n");
        if (options.scenario == L"mermaid")
            SetDocument(L"# Ideas, connected\n\n## A state diagram\n\n```mermaid\nstateDiagram-v2\ndirection LR\n[*] --> Draft\nDraft --> Saved : "
                        L"Save\n```\n\n"
                        L"## A class diagram\n\n```mermaid\nclassDiagram\ndirection LR\nDocument <|-- Markdown\nMarkdown : +save()\n```\n");
        if (options.scenario == L"recovery")
        {
            recoveryCandidates.push_back({L"0123456789abcdef0123456789abcdef",
                                          {L"# Your writing is here\r\n\r\nThe unsaved paragraph, including 東京 and 😀, is ready to restore.\r\n",
                                           L"C:\\Notes\\My next idea.md",
                                           true,
                                           {0, 0},
                                           12}});
            ShowRecovery();
        }
        if (options.scenario == L"source")
            SetView(EditorView::Source);
        if (options.scenario == L"split")
            SetView(EditorView::Split);
        if (options.scenario == L"settings")
            Execute(Command::Settings);
        Prepare();
        if (!options.benchmark.empty())
        {
            using Clock = std::chrono::steady_clock;
            constexpr unsigned frames = 60;
            const auto start = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
                (void)host.RenderInitialFrameForShow();
            const double cleanSeconds = std::chrono::duration<double>(Clock::now() - start).count();
            const auto dirtyStart = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
            {
                editor->SetSelection({0, 0});
                (void)editor->ReplaceSelection(L"x");
                (void)host.RenderInitialFrameForShow();
                (void)editor->Undo();
                (void)host.RenderInitialFrameForShow();
            }
            const double dirtySeconds = std::chrono::duration<double>(Clock::now() - dirtyStart).count();
            PROCESS_MEMORY_COUNTERS_EX memory{};
            (void)GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory), sizeof(memory));
            std::ofstream report(options.benchmark, std::ios::binary);
            report << "{\n  \"scenario\":\"sample-richtext-native-hidden\",\n  \"cleanFrames\":" << frames
                   << ",\n  \"cleanRenderFps\":" << frames / cleanSeconds << ",\n  \"dirtyFrames\":" << frames * 2
                   << ",\n  \"dirtyEditAndRenderMs\":" << dirtySeconds * 1000 / (frames * 2) << ",\n  \"workingSetBytes\":" << memory.WorkingSetSize
                   << ",\n  \"privateBytes\":" << memory.PrivateUsage << ",\n  \"dpi\":" << host.GetDpi()
                   << ",\n  \"widthPx\":" << host.DebugGetWidthPx() << ",\n  \"heightPx\":" << host.DebugGetHeightPx()
                   << ",\n  \"qualification\":\"initial application baseline only; forced native renders, not presentation FPS or a paired "
                      "non-regression claim\"\n}\n";
            if (!report)
                return 4;
        }
        if (options.scenario == L"context-selection")
        {
            const auto text = editor->GetText();
            const auto begin = text.find(L"Good writing");
            if (begin != text.npos)
            {
                editor->SetSelection({begin, begin + 12});
                SelectionChanged();
            }
        }
        if (options.scenario == L"local-source")
        {
            const auto begin = editor->GetText().find(L"Good writing");
            if (begin != std::wstring::npos)
            {
                editor->SetSelection({begin, begin + 12});
                Execute(Command::LocalSource);
            }
        }
        if (options.scenario == L"math" || options.scenario == L"math-dark")
        {
            const auto begin = editor->GetText().find(L"\\frac");
            if (begin != std::wstring::npos)
            {
                editor->SetSelection({begin, begin});
                SelectionChanged();
            }
        }
        PrepareDesignScenario();
        return options.capture.empty() || Capture(options.capture) ? 0 : 3;
    }
    if (startupPending)
        RecoveryCompleted(); // Also settle a scan that completed before the ordinary message loop started.
    (void)host.PrimeForShow();
    if (identityMeasure.typographyRevision != host.GetTypographyRevision())
    {
        Layout();
        (void)host.PrimeForShow();
    }
    ShowWindow(window.get(), InitialShowCommand());
    UpdateWindow(window.get());
    RecordWindowPlacement(true);
    host.SetFocusControl(editor, MayRequestNativeFocus());
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        DispatchUiMessage(message);
    }
    return static_cast<int>(message.wParam);
}
void Application::DispatchUiMessage(const MSG &message) noexcept
{
    RunUiAction(
        [&]
        {
            if (closingAfterSave)
                (void)DxUi::Failure::InvokeNative(&DispatchMessageW, &message);
            else if (!RouteShortcut(message) && !host.DispatchKeyboardMessage(message))
            {
                (void)DxUi::Failure::InvokeNative(&TranslateMessage, &message);
                (void)DxUi::Failure::InvokeNative(&DispatchMessageW, &message);
            }
            if (waitingForPreferences && !preferenceWriter.Busy() && recovery.Finished())
                (void)SendMessageW(window.get(), PreferencesReady, 0, 0);
            if (!closing && files.Busy())
                FileCompleted();
        });
}
} // namespace RedTilde
