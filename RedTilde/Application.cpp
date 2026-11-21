#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "KeyboardState.h"
#include "PrintPreview.h"
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
#include <iterator>
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
enum class ImageReferenceCategory
{
    PossibleFile,
    Network,
    Other
};
// Shared explanatory-chrome/watch-invalidation policy over semantic values.
// This performs no URI admission, path resolution, permission grant or I/O;
// only SDK-admitted LocalFile requests can supply actual watched paths.
ImageReferenceCategory ImageReferenceCategoryFor(const DxUi::RichText::SemanticElement& element) noexcept
{
    if (element.wiki)
        return ImageReferenceCategory::Other;
    const auto& value = element.destination;
    const auto prefix = [&](std::wstring_view text)
    {
        return value.size() >= text.size() &&
               CompareStringOrdinal(value.data(), static_cast<int>(text.size()), text.data(), static_cast<int>(text.size()), TRUE) == CSTR_EQUAL;
    };
    if (prefix(L"http://") || prefix(L"https://") || (prefix(L"file://") && value.size() > 7u && value[7u] != L'/') || value.starts_with(L"//") ||
        value.starts_with(L"\\\\"))
        return ImageReferenceCategory::Network;
    return prefix(L"data:") ? ImageReferenceCategory::Other : ImageReferenceCategory::PossibleFile;
}
// App-local display categories over the canonical immutable tree, not a parser.
unsigned CurrentBlockResourceAt(const DxUi::RichText::MarkdownSyntax& syntax,
                                std::size_t caret,
                                std::optional<std::pair<unsigned, std::size_t>>* listSummary = nullptr) noexcept
{
    using namespace DxUi::RichText;
    const auto source = syntax.Source();
    const auto& nodes = syntax.Nodes();
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
        const auto& node = nodes[index];
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
            case MarkdownBlockKind::SetextHeading: resource = IDS_BLOCKHEADING; break;
            case MarkdownBlockKind::FencedCode:
            case MarkdownBlockKind::IndentedCode: resource = IDS_BLOCKCODE; break;
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
            case MarkdownBlockKind::ListItem: resource = IDS_BLOCKLIST; break;
            case MarkdownBlockKind::Table:
            case MarkdownBlockKind::TableRow:
            case MarkdownBlockKind::TableCell: resource = IDS_BLOCKTABLE; break;
            case MarkdownBlockKind::BlockQuote: resource = IDS_BLOCKQUOTE; break;
            case MarkdownBlockKind::ThematicBreak: resource = IDS_BLOCKDIVIDER; break;
            case MarkdownBlockKind::HtmlBlock: resource = IDS_BLOCKHTML; break;
            case MarkdownBlockKind::ReferenceDefinition: resource = IDS_BLOCKLINKDEFINITION; break;
            case MarkdownBlockKind::FrontMatter: resource = IDS_BLOCKMETADATA; break;
            case MarkdownBlockKind::FootnoteDefinition: resource = IDS_BLOCKFOOTNOTE; break;
            case MarkdownBlockKind::SourceEditor: resource = IDS_BLOCKSOURCE; break;
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
std::wstring MathRegionExpression(const DxUi::RichText::MarkdownSourceRegion& region, std::wstring_view envelope)
{
    using DxUi::RichText::MarkdownSourceRegionKind;
    const auto trim = [](std::wstring_view text)
    {
        while (! text.empty() && (text.front() == L'\n' || text.front() == L'\r'))
            text.remove_prefix(1);
        while (! text.empty() && (text.back() == L'\n' || text.back() == L'\r'))
            text.remove_suffix(1);
        return text;
    };
    envelope         = trim(envelope);
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
DxUi::RichText::MarkdownExtensions MarkdownExtensionsFor(const Settings& settings) noexcept
{
    using DxUi::RichText::MarkdownExtensions;
    constexpr std::array<MarkdownExtensions, MarkdownSyntaxCount> flags{MarkdownExtensions::Highlight,
                                                                        MarkdownExtensions::Superscript,
                                                                        MarkdownExtensions::Subscript,
                                                                        MarkdownExtensions::Inserted,
                                                                        MarkdownExtensions::WikiLinks,
                                                                        MarkdownExtensions::DefinitionLists,
                                                                        MarkdownExtensions::Abbreviations};
    auto extensions = MarkdownExtensions::All;
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
        if (settings.markdownSyntax[index])
            extensions = extensions | flags[index];
    return extensions;
}
bool IsMarkdownCommandCapacityExceeded(const DxUi::RichTextEditor& editor,
                                       const DxUi::RichText::MarkdownCommandRequest& request,
                                       DxUi::RichText::Selection selection)
{
    using namespace DxUi::RichText;
    std::optional<MarkdownCommandPlan> plan;
    const auto syntax     = editor.GetMarkdownSyntax();
    const auto snapshot   = editor.Snapshot();
    const auto planResult = syntax ? TryBuildMarkdownCommand(snapshot, editor.Revision(), syntax, selection, request, plan) : SourceResult::InvalidSource;
    if (planResult == SourceResult::CapacityExceeded)
        return true;
    if (syntax && request.command == MarkdownCommand::SetHeading && request.headingLevel > 0u && ! CanSetHeading(*syntax, selection, request.headingLevel) &&
        CanSetHeading(*syntax, selection, 0u) && snapshot.Length() + request.headingLevel + 1u > Document::MaximumUnits)
        return true;
    if (! plan || ! plan->IsApplicable())
        return false;
    std::size_t nextLength = snapshot.Length();
    for (const auto& edit : plan->Edits())
    {
        const auto removed = edit.source.last - edit.source.first;
        if (edit.replacement.size() > Document::MaximumUnits - (nextLength - removed))
            return true;
        nextLength = nextLength - removed + edit.replacement.size();
    }
    return false;
}
} // namespace

struct Application::ExternalLinkState
{
    enum class Phase : std::uint8_t
    {
        Idle,
        LaunchingSta,
        CompletedPendingDrain,
        Closed
    };
    explicit ExternalLinkState(HWND owner) : window(owner)
    {
    }
    ExternalLinkState(const ExternalLinkState&)            = delete;
    ExternalLinkState& operator=(const ExternalLinkState&) = delete;
    ExternalLinkState(ExternalLinkState&&)                 = delete;
    ExternalLinkState& operator=(ExternalLinkState&&)      = delete;
    std::mutex mutex;
    HWND window{};
    Phase phase{Phase::Idle};
    HRESULT completedResult{S_OK};
};
struct Application::ExternalLinkJob
{
    ExternalLinkJob()                                  = default;
    ExternalLinkJob(const ExternalLinkJob&)            = delete;
    ExternalLinkJob& operator=(const ExternalLinkJob&) = delete;
    ExternalLinkJob(ExternalLinkJob&&)                 = delete;
    ExternalLinkJob& operator=(ExternalLinkJob&&)      = delete;
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
    return LoadStringResourceView(nullptr, id);
}
std::wstring Resource(unsigned id)
{
    return LoadStringResource(nullptr, id);
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

Application::Application(LaunchOptions value) : Application(std::move(value), DxUi::Failure::Scope{})
{
}
Application::Application(LaunchOptions value, const DxUi::Failure::Scope&) : _options(std::move(value))
{
    _startupRestore.enabled = ! _options.headless;
#if defined(REDTILDE_ENABLE_TESTS)
    _startupRestore.enabled = _startupRestore.enabled || _options.startupRestoreForTest;
#endif
}
void Application::TransitionLifecycle(LifecyclePhase next) noexcept
{
    _lifecyclePhase = next;
}
Application::~Application()
{
    TransitionLifecycle(LifecyclePhase::Destroyed);
    Printing::Stop(_printOperation);
    ClosePrintPreview();
    _bitmapWriter.Stop();
    _bitmapWriter.Detach();
    CloseExternalLinks();
    _settingsWatcher.Close();
    _preferenceWriter.Close();
    _recovery.Close();
    _files.Close();
    _folderFiles.Close();
    _images.Shutdown();
    _imageFiles.Close();
    _host.Detach();
    _window.reset();
}

HRESULT Application::Initialize()
{
    if (_options.dataDirectory.empty())
        _options.dataDirectory = DefaultDataDirectory();
    if (_startupRestore.enabled || _options.preferencesProbe || ! _options.headless)
    {
        SettingsLoadResult loaded = LoadSettingsCold(_options.dataDirectory);
        _settings                 = std::move(loaded.settings);
        if (loaded.status == SettingsLoadStatus::Loaded || loaded.status == SettingsLoadStatus::Missing || loaded.status == SettingsLoadStatus::Migrated ||
            loaded.status == SettingsLoadStatus::Invalid)
        {
            _settingsFileStamp = loaded.stamp;
        }
        if (loaded.status == SettingsLoadStatus::Invalid || loaded.status == SettingsLoadStatus::Unreadable)
            MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Settings file on startup could not be parsed cleanly; defaults applied");
    }
    WNDCLASSEXW cls{sizeof(cls)};
    cls.lpfnWndProc   = WindowProc;
    cls.hInstance     = GetModuleHandleW(nullptr);
    cls.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon         = LoadIconW(cls.hInstance, MAKEINTRESOURCEW(IDI_REDTILDE));
    cls.hIconSm       = cls.hIcon;
    cls.lpszClassName = L"RedTilde.MainWindow";
    if (! RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    const UINT dpi = GetDpiForSystem();
    RECT bounds{0, 0, MulDiv(_options.scenario == L"narrow" ? 820 : 1360, static_cast<int>(dpi), 96), MulDiv(900, static_cast<int>(dpi), 96)};
    (void)AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    RECT requested    = bounds;
    unsigned savedDpi = dpi;
    RECT workArea{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (! _options.headless)
    {
        if (_settings.windowPlacement)
        {
            requested = _settings.windowPlacement.value().normalBounds;
            savedDpi  = _settings.windowPlacement.value().dpi;
        }
        MONITORINFO monitor{sizeof(monitor)};
        const HMONITOR target =
            _settings.windowPlacement ? MonitorFromRect(&requested, MONITOR_DEFAULTTONEAREST) : MonitorFromPoint(POINT{}, MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(target, &monitor))
            workArea = monitor.rcWork;
        if (! _settings.windowPlacement)
        {
            const LONG width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
            requested = {workArea.left + (workArea.right - workArea.left - width) / 2, workArea.top + (workArea.bottom - workArea.top - height) / 2, 0, 0};
            requested.right  = requested.left + width;
            requested.bottom = requested.top + height;
        }
        bounds = ClampWindowBounds(requested, workArea, savedDpi, dpi);
    }
    _window.reset(CreateWindowExW(_options.headless ? WS_EX_NOACTIVATE : 0,
                                  cls.lpszClassName,
                                  Resource(IDS_APP).c_str(),
                                  WS_OVERLAPPEDWINDOW,
                                  _options.headless ? CW_USEDEFAULT : bounds.left,
                                  _options.headless ? CW_USEDEFAULT : bounds.top,
                                  bounds.right - bounds.left,
                                  bounds.bottom - bounds.top,
                                  nullptr,
                                  nullptr,
                                  cls.hInstance,
                                  this));
    if (! _window)
        return HRESULT_FROM_WIN32(GetLastError());
    DragAcceptFiles(_window.get(), TRUE);
    if (! _options.headless)
    {
        const unsigned actualDpi = GetDpiForWindow(_window.get());
        bounds                   = ClampWindowBounds(requested, workArea, savedDpi, actualDpi);
        if (! SetWindowPos(
                _window.get(), nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE))
            return HRESULT_FROM_WIN32(GetLastError());
        const bool maximized      = _settings.windowPlacement && _settings.windowPlacement.value().maximized;
        _settings.windowPlacement = WindowPlacementSettings{bounds, actualDpi, maximized};
        _windowPlacementDirty     = true;
    }
    if (! _host.Attach(_window.get(), {.forceWarp = _options.forceWarp, .batchInitialRootChildren = true}))
        return HRESULT_FROM_WIN32(GetLastError());
    _files.Initialize(_window.get(), FileReady);
#if defined(REDTILDE_ENABLE_TESTS)
    _files.SetTestGate(_options.fileGateForTest);
#endif
    const MARGINS frame{1, 1, 1, 1};
    (void)DwmExtendFrameIntoClientArea(_window.get(), &frame);
    SetWindowPos(_window.get(), nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    _preferenceWriter.Initialize(_options.dataDirectory, _window.get(), PreferencesReady);
    RETURN_IF_FAILED(_recovery.Initialize(_options.dataDirectory, _window.get(), RecoveryReadyMessage));
    RETURN_IF_FAILED(_images.Initialize(_window.get(), ImageReady, 0x5254));
    RETURN_IF_FAILED(_imageFiles.Initialize(_window.get(), ImageFileChanged, 0x5254));
    _images.SetAllowLocalNetwork(_settings.allowLocalNetworkImages);
    RETURN_IF_FAILED(_bitmapWriter.Initialize(_options.dataDirectory, _window.get(), BitmapReady));
    _host.SetClipboardPreference(_settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
    TransitionLifecycle(LifecyclePhase::Initializing);
    // BuildUi is the sole creation site for the label whose localized facts we cache.
    _currentBlockLatestFacts.reset();
    _currentBlockPublishedFacts.Invalidate();
    RETURN_IF_FAILED(BuildUi());
    ApplyTheme();
    SetDocument(_startupRestore.enabled ? std::wstring{} : SampleDocument());
    if (_startupRestore.enabled && _options.document.empty())
    {
        _startupRestore.source = _editor->Snapshot();
        _startupRestore.phase  = StartupRestoreState::Phase::WaitingForRecovery;
    }
    if (! _options.document.empty())
    {
        if (_options.headless && ! _startupRestore.enabled)
        {
            StoredDocument loaded;
            RETURN_IF_FAILED(LoadDocument(_options.document, loaded));
            SetDocument(std::move(loaded.text), _options.document, loaded.bom, loaded.stamp);
        }
        else
        {
            auto entrySource = _editor->Snapshot();
            RETURN_IF_FAILED(_files.Submit(false, _options.document));
            _activeFileTx = StartupOpenTx{std::move(entrySource)};
            Status(IDS_LOADING);
        }
    }
    TransitionLifecycle(LifecyclePhase::Active);
    Layout();
    const bool shouldWatchSettings = (! _options.headless && ! _options.preferencesProbe)
#if defined(REDTILDE_ENABLE_TESTS)
                                     || _options.watchSettingsForTest
#endif
        ;
    if (shouldWatchSettings)
    {
        const HRESULT watchResult = _settingsWatcher.Start(_options.dataDirectory, std::wstring(SettingsFileName()), _window.get(), SettingsChanged);
        if (FAILED(watchResult))
            MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Settings watcher could not start; live settings reload disabled");
    }
    if (! _options.headless && FAILED(RegisterApplicationRestart(L"", RESTART_NO_CRASH | RESTART_NO_HANG)))
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
    const auto workWidth     = static_cast<long long>(workArea.right) - workArea.left;
    const auto workHeight    = static_cast<long long>(workArea.bottom) - workArea.top;
    const auto scaled        = [&](long long size) { return (std::clamp(size, 1ll, 32768ll) * currentDpi + savedDpi / 2u) / savedDpi; };
    const auto minimumWidth  = std::min(workWidth, static_cast<long long>(800u) * currentDpi / 96u);
    const auto minimumHeight = std::min(workHeight, static_cast<long long>(560u) * currentDpi / 96u);
    const auto width         = std::clamp(scaled(static_cast<long long>(requested.right) - requested.left), minimumWidth, workWidth);
    const auto height        = std::clamp(scaled(static_cast<long long>(requested.bottom) - requested.top), minimumHeight, workHeight);
    const auto left = std::clamp(static_cast<long long>(requested.left), static_cast<long long>(workArea.left), static_cast<long long>(workArea.right) - width);
    const auto top  = std::clamp(static_cast<long long>(requested.top), static_cast<long long>(workArea.top), static_cast<long long>(workArea.bottom) - height);
    return {static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(left + width), static_cast<LONG>(top + height)};
}

int Application::InitialShowCommand() const noexcept
{
    if (_options.explicitShowCommand)
        return _options.showCommand;
    return _settings.windowPlacement && _settings.windowPlacement.value().maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
}

bool Application::MayRequestNativeFocus() const noexcept
{
    return ! _options.headless && _window && IsWindowVisible(_window.get()) && ! IsIconic(_window.get()) && GetForegroundWindow() == _window.get();
}

void Application::RecordWindowPlacement(bool persist)
{
    if (_lifecyclePhase != LifecyclePhase::Active || _options.headless || ! _window || IsIconic(_window.get()))
        return;
    auto placement      = _settings.windowPlacement.value_or(WindowPlacementSettings{});
    placement.maximized = IsZoomed(_window.get()) != FALSE;
    if (! placement.maximized)
    {
        if (! GetWindowRect(_window.get(), &placement.normalBounds))
            return;
        placement.dpi = GetDpiForWindow(_window.get());
    }
    if (! ValidWindowPlacement(placement))
        return;
    const auto previous = _settings.windowPlacement;
    if (! previous || ! EqualRect(&previous.value().normalBounds, &placement.normalBounds) || previous.value().dpi != placement.dpi ||
        previous.value().maximized != placement.maximized)
    {
        _settings.windowPlacement = placement;
        _windowPlacementDirty     = true;
    }
    if (persist && _windowPlacementDirty)
        SavePreferences();
}

void Application::AbandonStartupRestore()
{
    const bool wasOpening = _startupRestore.IsOpeningLastDocument();
    if (_startupRestore.phase != StartupRestoreState::Phase::Disabled)
        _startupRestore.phase = StartupRestoreState::Phase::Completed;
    _startupRestore.source.reset();
    if (! wasOpening)
        return;
    // Only this one automatic read is abandoned. Its worker retains its old
    // invalidated generation; no filesystem wait or desktop interaction occurs.
    _files.CancelInFlightLoad();
    if (std::holds_alternative<StartupOpenTx>(_activeFileTx) || std::holds_alternative<HeldOpenTx>(_activeFileTx))
        _activeFileTx = std::monostate{};
}

void Application::RememberActiveDocument()
{
    if (_lifecyclePhase != LifecyclePhase::Active && _lifecyclePhase != LifecyclePhase::ClosingAfterSave)
        return;
    const auto path = _session.recovered ? std::filesystem::path{} : _session.path;
    if (_settings.lastDocument != path)
    {
        _settings.lastDocument = path;
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
        return destination.size() >= prefix.size() && std::ranges::all_of(destination.substr(0, prefix.size()), [](wchar_t unit) { return unit < 0x80; }) &&
               CompareStringOrdinal(destination.data(), static_cast<int>(prefix.size()), prefix.data(), static_cast<int>(prefix.size()), TRUE) == CSTR_EQUAL;
    };
    if (starts(L"mailto:"))
        return destination.size() > 7u;
    const std::size_t authority = starts(L"https://") ? 8u : starts(L"http://") ? 7u : 0u;
    return authority != 0u && destination.size() > authority && destination.find_first_of(L"/?#", authority) != authority;
}

std::optional<std::size_t> Application::CurrentLinkElement(const std::shared_ptr<const DxUi::RichText::SemanticDocument>& document) const
{
    if (! document || ! _editor->Snapshot().MatchesText(document->Syntax()->Source()))
        return std::nullopt;
    const auto selection = _editor->GetSelection();
    const auto elements  = document->Elements();
    for (std::size_t index = 0; index < elements.size(); ++index)
    {
        const auto& element = elements[index];
        if (element.role == DxUi::RichText::SemanticRole::Link && ! element.wiki && selection.Start() >= element.source.first &&
            selection.Start() < element.source.last && selection.End() <= element.source.last)
            return index;
    }
    return std::nullopt;
}

void Application::RequestOpenLink(const DxUi::RichTextLinkActivation& activation)
{
    if (! IsUiAttached())
        return;
    if (! CanAcceptCommand() || IsSessionEnding() || HasActiveModalPrompt() || HasBlockDraft() || activation.wiki ||
        ! SupportedExternalLink(activation.destination))
    {
        Status(IDS_LINKUNAVAILABLE);
        return;
    }
    const auto semantic = _editor->GetSemanticDocument();
    const auto source   = _editor->Snapshot();
    if (! semantic || ! source.MatchesText(semantic->Syntax()->Source()) ||
        std::ranges::none_of(semantic->Elements(),
                             [&](const auto& element)
    {
        return element.role == DxUi::RichText::SemanticRole::Link && element.source == activation.source && element.destination == activation.destination &&
               ! element.wiki;
    }))
    {
        Status(IDS_LINKCHANGED);
        return;
    }
    if (_externalLink)
    {
        std::lock_guard lock(_externalLink->mutex);
        if (_externalLink->phase == ExternalLinkState::Phase::LaunchingSta)
        {
            // No UI callback or status publication while holding a worker lock.
            return;
        }
    }
    std::wstring destination(activation.destination); // Own every byte before the modal/reentrant boundary.
    const auto selection = _editor->GetSelection();
    const auto key       = _session.key;
    auto* const owner    = _root;
    auto* const primary  = _editor;
    const auto lifetime  = GetControlLifetimeToken(*primary);
    bool approved{};
    _modalPrompt      = ModalPromptKind::ExternalLink;
    const auto unwind = wil::scope_exit([&] { _modalPrompt = ModalPromptKind::None; });
#if defined(REDTILDE_ENABLE_TESTS)
    const auto testLaunch = _options.launchLinkForTest;
#endif
    if (_options.headless)
    {
#if defined(REDTILDE_ENABLE_TESTS)
        const auto confirm = _options.confirmLinkForTest;
        if (! confirm || ! testLaunch)
            return;
        approved = confirm(destination);
#else
        return;
#endif
    }
    else
    {
        const auto message = FormatStringResource(nullptr, IDS_OPENLINKCONFIRM, destination);
        approved =
            DxUi::Failure::InvokeNative(
                &MessageBoxW, _window.get(), message.c_str(), Resource(IDS_OPENLINK).c_str(), static_cast<UINT>(MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)) ==
            IDYES;
    }
    if (! approved)
        return;
    if (IsClosing() || lifetime.expired() || _host.GetRoot() != owner || _editor != primary)
        return;
    if (IsSessionEnding() || _session.key != key || ! primary->Snapshot().IsSameRevision(source) || primary->GetSelection() != selection)
    {
        Status(IDS_LINKCHANGED);
        return;
    }
    if (! _externalLink)
        _externalLink = std::make_shared<ExternalLinkState>(_window.get());
    auto job         = std::make_unique<ExternalLinkJob>();
    job->state       = _externalLink;
    job->destination = std::move(destination);
#if defined(REDTILDE_ENABLE_TESTS)
    if (_options.headless)
    {
        job->launchForTest = testLaunch;
        job->gate          = _options.linkGateForTest;
    }
#endif
    HRESULT admitted = S_OK;
    admitted         = ProcessWork::Acquire(job->lease);
    if (FAILED(admitted))
    {
        Status(IDS_LINKFAILED);
        return;
    }
    {
        std::lock_guard lock(_externalLink->mutex);
        if (_externalLink->phase == ExternalLinkState::Phase::LaunchingSta || _externalLink->phase == ExternalLinkState::Phase::Closed ||
            ! _externalLink->window)
            admitted = HRESULT_FROM_WIN32(ERROR_BUSY);
        else
        {
            _externalLink->phase           = ExternalLinkState::Phase::LaunchingSta;
            _externalLink->completedResult = S_OK;
            // A fresh short-lived STA is required by shell extensions. Unlike
            // the MTA codec/file workers it cannot borrow an arbitrary pool apartment.
            // The EXE owns only value/shared-state data; Close invalidates posting
            // without waiting for another application's shell handler.
            wil::unique_handle thread(CreateThread(nullptr, 0, ExternalLinkWorker, job.get(), 0, nullptr));
            if (! thread)
            {
                const DWORD error    = GetLastError();
                admitted             = HRESULT_FROM_WIN32(error ? error : ERROR_NOT_ENOUGH_MEMORY);
                _externalLink->phase = ExternalLinkState::Phase::Idle;
            }
            else
                (void)job.release();
        }
    }
    Status(FAILED(admitted) ? IDS_LINKFAILED : IDS_LINKOPENING);
}

DWORD WINAPI Application::ExternalLinkWorker(void* context) noexcept
{
    ProcessWorkLease lease = std::move(static_cast<ExternalLinkJob*>(context)->lease);
    const std::unique_ptr<ExternalLinkJob> job(static_cast<ExternalLinkJob*>(context));
#if defined(REDTILDE_ENABLE_TESTS)
    const auto settled = wil::scope_exit([&]
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
        if (! admitted)
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
                    launch.fMask  = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
                    launch.lpVerb = L"open";
                    launch.lpFile = job->destination.c_str();
                    launch.nShow  = SW_SHOWNORMAL;
                    // No process handle is requested or retained.
                    if (DxUi::Failure::InvokeNative(&ShellExecuteExW, &launch))
                        result = S_OK;
                    else
                    {
                        const DWORD error = GetLastError();
                        result            = HRESULT_FROM_WIN32(error ? error : ERROR_OPEN_FAILED);
                    }
                }
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::exception&)
    {
        // Unexpected failures cannot unwind through the native thread entry or resume a partial callback.
        DxUi::Failure::FailUnexpected();
    }
    std::lock_guard lock(job->state->mutex);
    if (job->state->phase != ExternalLinkState::Phase::Closed && job->state->window)
    {
        job->state->phase           = ExternalLinkState::Phase::CompletedPendingDrain;
        job->state->completedResult = result;
        (void)PostMessageW(job->state->window, ExternalLinkReady, 0, 0);
    }
    return 0;
}

void Application::ExternalLinkCompleted()
{
    if (! _externalLink)
        return;
    std::optional<HRESULT> completed;
    {
        std::lock_guard lock(_externalLink->mutex);
        if (_externalLink->phase == ExternalLinkState::Phase::CompletedPendingDrain)
        {
            completed            = _externalLink->completedResult;
            _externalLink->phase = ExternalLinkState::Phase::Idle;
        }
    }
    if (completed && IsUiAttached())
        Status(SUCCEEDED(completed.value()) ? IDS_LINKOPENED : IDS_LINKFAILED);
}

void Application::CloseExternalLinks() noexcept
{
    if (_externalLink)
    {
        std::lock_guard lock(_externalLink->mutex);
        _externalLink->window = nullptr;
        _externalLink->phase  = ExternalLinkState::Phase::Closed;
    }
}

DxUi::Label* Application::Label(DxUi::Panel& parent, unsigned label, DxUi::FontRole role)
{
    auto* result = parent.AddChild<DxUi::Label>(Resource(label));
    result->SetFontRole(role);
    if (role == DxUi::FontRole::Small)
        THROW_IF_FAILED(result->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    else if (role == DxUi::FontRole::Body)
        THROW_IF_FAILED(result->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    return result;
}
DxUi::Button* Application::Action(DxUi::Panel& parent, unsigned label, Command command, std::wstring tooltip)
{
    auto* button = parent.AddChild<DxUi::Button>(Resource(label));
    THROW_IF_FAILED(button->SetContentFontSizeDip(14.f));
    button->SetAccessibleName(Resource(label));
    button->SetTooltipText(tooltip.empty() ? Resource(label) : std::move(tooltip));
    button->SetOnClick([this, command] { Execute(command); });
    return button;
}

void Application::ApplyMarkdownSyntax()
{
    // Matching masks keep the read-only mirror eligible for parsed-syntax adoption.
    const auto extensions = MarkdownExtensionsFor(_settings);
    if (! AcceptSourceResult(_editor->TrySetMarkdownAnalysisExtensions(extensions)))
        return;
    if (! AcceptSourceResult(_mirror->TrySetMarkdownAnalysisExtensions(extensions)))
        return;
    (void)AcceptSourceResult(_equation->TrySetMarkdownAnalysisExtensions(extensions));
}

void Application::ApplyTheme()
{
    // Cache the system preference independently of the selected appearance so the
    // System preview remains truthful without registry reads during layout.
    DWORD value = 1, size = sizeof(value);
    const auto preference = RegGetValueW(HKEY_CURRENT_USER,
                                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                                         L"AppsUseLightTheme",
                                         RRF_RT_REG_DWORD,
                                         nullptr,
                                         &value,
                                         &size);
    _systemDark           = preference == ERROR_SUCCESS && value == 0;
    const bool dark       = _settings.appearance == Appearance::Dark || (_settings.appearance == Appearance::System && _systemDark);
    auto theme            = DxUi::MakeDefaultThemePalette(dark);
    HIGHCONTRASTW contrast{sizeof(contrast)};
    (void)SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    const bool highContrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0 || (_options.headless && _options.scenario == L"high-contrast");
    if (highContrast)
    {
        const auto systemColor = [](int index)
        {
            const auto value = GetSysColor(index);
            return D2D1::ColorF(GetRValue(value) / 255.f, GetGValue(value) / 255.f, GetBValue(value) / 255.f);
        };
        theme.highContrast     = true;
        theme.windowBackground = theme.surfaceBackground = theme.cardBackground = systemColor(COLOR_WINDOW);
        theme.text                                                              = systemColor(COLOR_WINDOWTEXT);
        theme.selectionFill = theme.selectionInactiveFill = systemColor(COLOR_HIGHLIGHT);
        theme.accent                                      = systemColor(COLOR_HOTLIGHT);
        theme.selectionText                               = systemColor(COLOR_HIGHLIGHTTEXT);
        theme.focusStroke                                 = systemColor(COLOR_HOTLIGHT);
    }
    else
    {
        theme.accent        = D2D1::ColorF(dark ? 0xFF7778 : 0xC83C46);
        theme.accentHover   = D2D1::ColorF(dark ? 0xFF999A : 0xB6303C);
        theme.accentPressed = D2D1::ColorF(dark ? 0xFFABAC : 0xA12A34);
        theme.selectionFill = theme.selectionInactiveFill = D2D1::ColorF(dark ? 0xA63B46 : 0xF2CFD3);
        theme.selectionText                               = D2D1::ColorF(dark ? 0xFFFFFFu : 0xA12A34);
        theme.focusStroke                                 = theme.accent;
        theme.windowBackground                            = D2D1::ColorF(dark ? 0x17191Eu : _settings.appearance == Appearance::Paper ? 0xEDE6D8u : 0xF0F1F4u);
        theme.surfaceBackground                           = D2D1::ColorF(dark ? 0x1C1F25u : _settings.appearance == Appearance::Paper ? 0xF3EDDFu : 0xF8F8FAu);
        theme.cardBackground = theme.inputFill = D2D1::ColorF(dark ? 0x22252Bu : _settings.appearance == Appearance::Paper ? 0xFFFAEFu : 0xFFFFFFu);
        theme.buttonBorder                     = theme.borderDefault;
    }
    BOOL animation = TRUE;
    (void)SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animation, 0);
    theme.reducedMotion     = ! animation || _options.headless;
    theme.primaryButtonFill = D2D1::ColorF(0xC83C46);
    theme.primaryButtonText = D2D1::ColorF(D2D1::ColorF::White);
    _host.SetTheme(theme);
    if (_printPreview)
        _printPreview->SetTheme(theme);
    const auto underline = highContrast ? DxUi::RichTextLinkUnderlinePolicy::Always : DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret;
    for (auto* target : {static_cast<DxUi::RichTextEditor*>(_editor), _mirror, _equation})
        if (! AcceptSourceResult(target->TrySetLinkUnderlinePolicy(underline)))
            return;
    const BOOL nativeDark = dark;
    (void)DwmSetWindowAttribute(_window.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &nativeDark, sizeof(nativeDark));
    Layout();
}

const std::wstring& Application::CurrentText() const
{
    const auto source = _editor->Snapshot();
    if (! _derivedCaches.cachedSource || ! _derivedCaches.cachedSource->IsSameRevision(source))
    {
        _derivedCaches.cachedText   = source.Text();
        _derivedCaches.cachedSource = source;
    }
    return _derivedCaches.cachedText;
}
bool Application::Dirty() const
{
    if (! _editor)
        return false;
    const auto source = _editor->Snapshot();
    if (_derivedCaches.dirtyFact && _derivedCaches.dirtyFact->recovered == _session.recovered && _derivedCaches.dirtyFact->source.IsSameRevision(source))
        return _derivedCaches.dirtyFact->dirty;
    bool dirty = _session.recovered || source.Length() != _session.savedText.size();
    if (! dirty)
    {
        ++_activity.dirtyTextComparisons;
        dirty = CurrentText() != _session.savedText;
    }
    _derivedCaches.dirtyFact = DerivedDocumentCaches::DirtyFact{source, _session.recovered, dirty};
    return dirty;
}
void Application::RefreshDocumentAnalysis(const DxUi::RichText::SourceSnapshot& source, bool committed)
{
    const auto sameOutline = [](const std::vector<Heading>& left, const std::vector<Heading>& right)
    {
        return left.size() == right.size() &&
               std::equal(left.begin(), left.end(), right.begin(), [](const Heading& a, const Heading& b) { return a.level == b.level && a.text == b.text; });
    };
    const auto matchesCommittedOutline = [](const std::vector<Heading>& headings, const std::vector<std::pair<unsigned, std::wstring>>& identity)
    {
        return headings.size() == identity.size() &&
               std::equal(headings.begin(), headings.end(), identity.begin(), [](const Heading& heading, const auto& item) {
            return heading.level == item.first && heading.text == item.second;
        });
    };

    const bool newSourceRevision = ! _derivedCaches.documentAnalysisSource || ! _derivedCaches.documentAnalysisSource->IsSameRevision(source);
    if (! newSourceRevision && ! committed)
        return;

    const std::wstring* text{};
    if (newSourceRevision)
        text = &CurrentText();
    const bool sourceMatchesCommitted =
        ! committed && newSourceRevision && _derivedCaches.committedAnalysisSource && *text == _derivedCaches.committedAnalysisSource->Text();
    const bool returningToCommitted = ! committed && newSourceRevision && sourceMatchesCommitted;
    const bool transientSource      = ! committed && newSourceRevision && _derivedCaches.committedAnalysisSource && ! sourceMatchesCommitted;
    if (transientSource && ! _outlinePreviewBaseline)
    {
        auto& baseline             = _outlinePreviewBaseline.emplace();
        baseline.collapsedHeadings = _session.collapsedHeadings;
        baseline.committedOutline.reserve(_documentInfo.headings.size());
        for (const auto& heading : _documentInfo.headings)
            baseline.committedOutline.emplace_back(heading.level, heading.text);
    }

    std::optional<DocumentInfo> nextInfo;
    const DocumentInfo* analysis = &_documentInfo;
    bool outlineRowsChanged{};
    if (newSourceRevision)
    {
        nextInfo.emplace(_documentLineIndexStorage.Analyze(*text, _documentInfo.lines));
        outlineRowsChanged = ! sameOutline(nextInfo->headings, _documentInfo.headings);
        analysis           = &*nextInfo;
    }

    bool collapseStateChanged{};
    if ((committed || returningToCommitted) && _outlinePreviewBaseline && _session.collapsedHeadings != _outlinePreviewBaseline->collapsedHeadings)
    {
        _session.collapsedHeadings = _outlinePreviewBaseline->collapsedHeadings;
        collapseStateChanged       = true;
    }
    if (transientSource && _outlinePreviewBaseline && outlineRowsChanged)
    {
        const auto desired = matchesCommittedOutline(analysis->headings, _outlinePreviewBaseline->committedOutline) ? _outlinePreviewBaseline->collapsedHeadings
                                                                                                                    : std::set<std::uint64_t>{};
        if (_session.collapsedHeadings != desired)
        {
            _session.collapsedHeadings = desired;
            collapseStateChanged       = true;
        }
    }
    if (committed)
    {
        const bool sameAsPrior =
            _outlinePreviewBaseline ? matchesCommittedOutline(analysis->headings, _outlinePreviewBaseline->committedOutline) : ! outlineRowsChanged;
        if (! sameAsPrior && ! _session.collapsedHeadings.empty())
        {
            _session.collapsedHeadings.clear();
            collapseStateChanged = true;
        }
        _derivedCaches.committedAnalysisSource = source;
        _outlinePreviewBaseline.reset();
    }
    else if (returningToCommitted)
        _outlinePreviewBaseline.reset();

    if (newSourceRevision)
    {
        using std::swap;
        swap(_documentInfo, *nextInfo);
        _documentLineIndexStorage.Retire(std::move(nextInfo->lineIndex), _documentInfo.lines);
        _derivedCaches.documentAnalysisSource = source;
        ++_activity.documentAnalysisUpdates;
    }
    if (outlineRowsChanged || collapseStateChanged)
        RefreshOutline();
}
bool Application::SynchronizeMirror()
{
    if (IsClosing())
        return false;
    if (_settings.view != EditorView::Split)
        return true;
    if (! _root || _host.GetRoot() != _root)
        return false;
    auto* const primary = _editor;
    auto* const target  = _mirror;
    auto* const owner   = _root;
    const auto source   = primary->Snapshot();
    if (_derivedCaches.mirroredSource && _derivedCaches.mirroredSource->IsSameRevision(source))
        return true;
    const auto selection       = primary->GetSelection();
    const auto revision        = primary->Revision();
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto targetLifetime  = GetControlLifetimeToken(*target);
    const auto ownerLifetime   = GetControlLifetimeToken(*owner);
    const auto current         = [&]
    {
        if (IsClosing() || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || _editor != primary || _mirror != target ||
            _root != owner || _host.GetRoot() != owner || _settings.view != EditorView::Split)
            return false;
        // These app-owned controls are never reparented independently of the
        // root; its host ownership and all three weak identities were checked.
        return primary->Snapshot().IsSameRevision(source) && primary->Revision() == revision && primary->GetSelection() == selection;
    };
    if (target->SynchronizeReadOnlySourceSnapshot(source, selection))
    {
        if (! current() || ! target->Snapshot().IsSameRevision(source) || target->GetSelection() != selection)
            return false;
    }
    else
    {
        if (! current())
            return false;
        // Retain exact SetText fallback for an incompatible mirror profile.
        // Own this spelling across callbacks instead of borrowing a mutable app cache.
        const auto text = source.Text();
        if (! AcceptSourceResult(target->TrySetText(text)))
            return false;
        if (! current() || target->GetText() != text)
            return false;
    }
    // A nested synchronization may already have published this same source.
    if (! _derivedCaches.mirroredSource || ! _derivedCaches.mirroredSource->IsSameRevision(source))
    {
        _derivedCaches.mirroredSource = source;
        ++_activity.mirrorResets;
    }
    return true;
}
void Application::Status(unsigned id)
{
    if (_feedback)
        _feedback->SetText(Resource(id));
}
void Application::ReportCapacityLimit()
{
    if (_feedback)
        _feedback->SetText(FormatStringResource(nullptr, IDS_FILETOOLARGELIMIT, DxUi::RichText::Document::MaximumUnits));
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
    const bool previousImageTransition = _imageBindingTransition;
    _imageBindingTransition            = true;
    const auto restoreImageTransition  = wil::scope_exit([&] { _imageBindingTransition = previousImageTransition; });
    // A checked refusal preserves the entire current document, saved baseline, recovery and pending insertion.
    if (! DxUi::RichText::IsValidSourceUtf16(text))
        return AcceptSourceResult(DxUi::RichText::SourceResult::InvalidSource);
    if (text.size() > DxUi::RichText::Document::MaximumUnits)
        return AcceptSourceResult(DxUi::RichText::SourceResult::CapacityExceeded);
    const auto lifetime = GetControlLifetimeToken(*_editor);
    // A replacement document cannot inherit a one-document grant through synchronous text callbacks.
    const bool previousDocumentPermission = _session.remoteImagesAllowed;
    const auto previousPermissionSource   = _editor->Snapshot();
    bool permissionCommitted              = false;
    const auto restorePermission          = wil::scope_exit([&]() noexcept
    {
        if (! permissionCommitted && ! lifetime.expired() && ! IsClosing() && _session.remoteImagesAllowed == previousDocumentPermission &&
            _editor->Snapshot().IsSameRevision(previousPermissionSource))
            _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages || previousDocumentPermission);
    });
    _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages);
    if (lifetime.expired() || IsClosing())
        return false;
    if (! AcceptSourceResult(_editor->TrySetDocumentFilePath(path.native())) || lifetime.expired() || IsClosing())
        return false;
    if (! AcceptSourceResult(_editor->TrySetText(text)))
        return false;
    if (lifetime.expired() || IsClosing())
        return false;
    _session.remoteImagesAllowed = false;
    _printNetworkImages.clear();
    permissionCommitted = true;
    _pendingBitmapPaste.reset();
    if (! IsInitializing())
        AbandonStartupRestore();
    _session.recovered = false;
    _session.path      = std::move(path);
    EnsureFilesRoot();
    _session.bom       = hasBom;
    _session.stamp     = identity;
    _session.savedText = _editor->GetText();
    _derivedCaches.dirtyFact.reset();
    SetView(_settings.view);
    Changed();
    RefreshFiles(true);
    if (rememberForStartup)
        RememberActiveDocument();
    return true;
}
void Application::Changed()
{
    _pendingBitmapPaste.reset();
    if (_switchingDocument)
        return;
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Enter);
    ++_activity.changes;
    const auto source = _editor->Snapshot();
    RefreshDocumentAnalysis(source, true);
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Analysis);
    if (! SynchronizeMirror())
        return;
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Mirror);
    const auto name                  = _session.path.empty() ? Resource(IDS_UNTITLED) : _session.path.filename().native();
    const bool dirty                 = Dirty();
    const auto displayedName         = _settings.focusMode && dirty ? name + L" · " + Resource(IDS_CHANGED) : name;
    const bool identityLayoutChanged = _title->GetText() != displayedName;
    _title->SetText(displayedName);
    _saveState->SetText(Resource(dirty ? IDS_CHANGED : IDS_SAVED));
    if (identityLayoutChanged)
        Layout();
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Identity);
    UpdateCaption(FormatStringResource(nullptr, IDS_WINDOWTITLE, dirty ? L"• " : L"", name));
    // Sending WM_SETTEXT to our own HWND is synchronous and can re-enter Close.
    // BeginClose detaches the host, so do not touch cached controls after it returns.
    if (IsClosing())
        return;
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Caption);
    UpdateCounts();
    _tools[0]->SetEnabled(_editor->CanUndo());
    _tools[1]->SetEnabled(_editor->CanRedo());
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    if (_settings.showFiles && dirty != _lastFilesDirty)
    {
        UpdateCurrentFileDirtyBadge(dirty);
        // A synchronous accessibility publication can dispatch Close and detach the host.
        if (IsClosing())
            return;
    }
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Chrome);
    if (_editDiagnostics)
    {
        auto selectionArm = _editDiagnostics->ArmSelection();
        SelectionChanged();
    }
    else
        SelectionChanged();
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Selection);
    if (! IsInitializing())
        CheckpointRecovery();
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Recovery);
}

void Application::UpdateCounts()
{
    if (_settings.focusMode)
    {
        const auto state = ResourceView(Dirty() ? IDS_CHANGED : IDS_SAVED);
        _stats->SetText(FormatStringResource(nullptr, IDS_FOCUSCOUNTS, _documentInfo.words, state));
    }
    else
    {
        const auto readingMinutes = std::max(std::size_t{1}, (_documentInfo.words + 199) / 200);
        _stats->SetText(FormatStringResource(nullptr, IDS_STATUSCOUNTS, _documentInfo.words, _documentInfo.characters, readingMinutes));
    }
}

bool Application::PublishCurrentBlockLabelIfVisible(bool sidebarWillBeVisible)
{
    if (IsClosing())
        return false;
    if (! _root || ! _sidebar || ! _currentBlock)
        return false;
    if ((! _sidebar->IsVisible() && ! sidebarWillBeVisible) || ! _currentBlock->IsVisible() || ! _currentBlockLatestFacts ||
        _currentBlockPublishedFacts.Matches(_currentBlockLatestFacts.value()))
        return true;

    const auto facts     = _currentBlockLatestFacts.value();
    const auto blockType = ResourceView(facts.resourceId);
    auto text            = FormatStringResource(nullptr, IDS_BLOCKINFO, blockType, facts.lineCharacterCount);
    ++_activity.currentBlockLabelConstructions;
    const auto token           = _currentBlockPublishedFacts.BeginPublish();
    auto* const publishedLabel = _currentBlock;
    auto* const publishedRoot  = _root;
    const auto labelLifetime   = GetControlLifetimeToken(*publishedLabel);
    const auto rootLifetime    = GetControlLifetimeToken(*publishedRoot);
    publishedLabel->SetText(std::move(text));
    // SetText can reenter and retire either control or install newer requested facts.
    if (IsClosing() || labelLifetime.expired() || rootLifetime.expired() || _currentBlock != publishedLabel || _root != publishedRoot ||
        _host.GetRoot() != publishedRoot)
        return false;
    // A nested callback can update the requested facts without publishing while hidden.
    if (_currentBlockLatestFacts == facts)
        _currentBlockPublishedFacts.CommitPublish(token, facts);
    return true;
}

void Application::UpdateCaption(std::wstring caption)
{
    ++_activity.captionRequests;
    if (_appliedCaption.Matches(caption))
        return;
    // A native write remains synchronous and part of this command's measured
    // work. Repeated edits while already dirty need no identical Win32 write.
    // Invalidate before entering Win32 so reentrant requests cannot see stale
    // cached state; failed or superseded writes must remain retryable.
    const auto token = _appliedCaption.BeginPublish();
    ++_activity.captionCalls;
    // This is our own HWND. Use WM_SETTEXT's explicit acceptance result: the
    // SetWindowText wrapper may report success for a receiver's zero result.
    if (DxUi::Failure::InvokeNative(&SendMessageW, _window.get(), static_cast<UINT>(WM_SETTEXT), WPARAM{}, reinterpret_cast<LPARAM>(caption.c_str())) == TRUE)
        _appliedCaption.CommitPublish(token, std::move(caption));
}

void Application::CheckpointRecovery(bool immediate)
{
    std::optional<RecoverySnapshot> snapshot;
    auto selection     = _editor->GetSelection();
    bool needsRecovery = Dirty();
    std::wstring_view draft;
    std::wstring normalizedDraft;
    bool changedDraft{};
    if (HasBlockDraft() && _editor->Snapshot().IsSameRevision(_blockDraftSession.value().snapshot) &&
        _editor->Revision() == _blockDraftSession.value().revision)
    {
        const auto& range              = _blockDraftSession.value().range;
        draft                          = _blockDraftText->GetText();
        const std::wstring_view source = CurrentText();
        if (draft != source.substr(range.first, range.last - range.first))
        {
            normalizedDraft = NormalizeInsertedLineEndings(draft, source);
            draft           = normalizedDraft;
            if (! DxUi::RichText::IsValidUtf16(draft) || source.size() - (range.last - range.first) + draft.size() > DxUi::RichText::Document::MaximumUnits)
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            changedDraft  = draft != source.substr(range.first, range.last - range.first);
            needsRecovery = needsRecovery || changedDraft;
        }
    }
    if (needsRecovery)
    {
        auto text = CurrentText();
        ++_activity.recoveryTextCopies;
        if (changedDraft)
        {
            const auto& range = _blockDraftSession.value().range;
            // Recovery is a previewable copy of the proposed edit. The live
            // authoritative source/history remain untouched until Apply.
            text.replace(range.first, range.last - range.first, draft);
            selection = {range.first, range.first + draft.size()};
        }
        snapshot = RecoverySnapshot{std::move(text), _session.path, _session.bom, selection, ++_recoveryGeneration};
    }
    const bool flush = immediate || ! snapshot.has_value();
    ++_activity.recoverySubmissions;
    if (FAILED(_recovery.SubmitDocument(_session.key, std::move(snapshot), flush)))
        Status(IDS_RECOVERYFAILED);
}
void Application::RecoveryCompleted()
{
    if (IsClosing())
    {
        FinishClose();
        return;
    }
    if (! _feedback)
        return;
    auto candidates = _recovery.TakeCandidates();
    for (auto& candidate : candidates)
        _recoveryCandidates.push_back({std::move(candidate.id), std::move(candidate.snapshot), RecoveryDisposition::Available});
    bool retiredCandidate{};
    for (const auto& id : _recovery.TakeRetired())
        retiredCandidate = std::erase_if(_recoveryCandidates, [&](const auto& candidate) { return candidate.id == id; }) != 0 || retiredCandidate;
    for (const auto& id : _recovery.TakeFailedDiscards())
        for (auto& candidate : _recoveryCandidates)
            if (candidate.id == id && candidate.disposition == RecoveryDisposition::Discarding)
                candidate.disposition = RecoveryDisposition::Available;
    if (_recovery.Settled() && FAILED(_recovery.Status()))
        for (auto& candidate : _recoveryCandidates)
            if (candidate.disposition == RecoveryDisposition::Discarding)
                candidate.disposition = RecoveryDisposition::Available;
    if (retiredCandidate && _activeOverlay == OverlayKind::RecoveryModal)
        ShowRecovery();
    if (FAILED(_recovery.Status()))
        Status(IDS_RECOVERYFAILED);
    if (! _recoveryOffered && _recovery.Ready())
    {
        _recoveryOffered = true;
        if (! _recoveryCandidates.empty() && (_startupRestore.enabled || _options.recoveryRestartProbe || _options.scenario == L"recovery"))
            ShowRecovery();
    }
    if (_startupRestore.IsWaitingForRecovery() && _recovery.Ready())
    {
        if (SUCCEEDED(_recovery.Status()) && _recoveryCandidates.empty() && _startupRestore.source &&
            _editor->Snapshot().IsSameRevision(_startupRestore.source.value()) && ! _settings.lastDocument.empty() && _session.path.empty() &&
            ! _files.Busy() && ! HasBlockDraft() && ! IsSessionEnding())
        {
            auto entrySource     = _editor->Snapshot();
            const HRESULT result = _files.Submit(false, _settings.lastDocument);
            if (SUCCEEDED(result))
            {
                _startupRestore.phase = StartupRestoreState::Phase::OpeningLastDocument;
                _activeFileTx         = StartupOpenTx{std::move(entrySource)};
                Status(IDS_LOADING);
            }
            else
            {
                _startupRestore.phase = StartupRestoreState::Phase::Completed;
                _startupRestore.source.reset();
                Status(IDS_BUSY);
            }
        }
        else
        {
            _startupRestore.phase = StartupRestoreState::Phase::Completed;
            _startupRestore.source.reset();
        }
    }
}
void Application::ShowRecovery()
{
    if (_recoveryCandidates.empty())
    {
        if (_activeOverlay == OverlayKind::RecoveryModal)
        {
            SetActiveOverlay(OverlayKind::None);
            Layout();
        }
        Status(IDS_NORECOVERY);
        _host.SetFocusControl(_editor, MayRequestNativeFocus());
        return;
    }
    const auto& snapshot = _recoveryCandidates.front().snapshot;
    const auto path      = snapshot.originalPath.empty() ? Resource(IDS_UNTITLED) : snapshot.originalPath.native();
    const auto count     = _recoveryCandidates.size();
    _restoreAllRecovery->SetVisible(count > 1);
    _recoveryDescription->SetText(count > 1 ? FormatStringResource(nullptr, IDS_RECOVERYMULTIPLEHELP, count, path)
                                            : FormatStringResource(nullptr, IDS_RECOVERYHELP, path));
    _recoveryPreview->SetText(snapshot.text);
    SetActiveOverlay(OverlayKind::RecoveryModal);
    _host.SetFocusControl(_recoveryPanel->GetLogicalChild(3), MayRequestNativeFocus());
    Layout();
    if (! IsInitializing() && ! IsClosing())
        CheckpointRecovery();
}
void Application::SetView(EditorView view)
{
    if (! IsUiAttached())
        return;
    auto* const primary        = _editor;
    auto* const target         = _mirror;
    auto* const owner          = _root;
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto targetLifetime  = GetControlLifetimeToken(*target);
    const auto ownerLifetime   = GetControlLifetimeToken(*owner);
    const auto current         = [&]
    {
        return ! IsClosing() && ! primaryLifetime.expired() && ! targetLifetime.expired() && ! ownerLifetime.expired() && _editor == primary &&
               _mirror == target && _root == owner && _host.GetRoot() == owner && _settings.view == view;
    };
    _selectionToolsFocus.reset();
    // The same editor/document/history survives all global presentation changes.
    _settings.view = view;
    if (! AcceptSourceResult(primary->TrySetViewMode(view == EditorView::Source ? DxUi::RichText::ViewMode::Source : DxUi::RichText::ViewMode::Rich)))
        return;
    if (! current())
        return;
    // Source supersession aborts source-bound work, but this view transition
    // uses the latest source. Retired owners and a newer view request still win.
    (void)SynchronizeMirror();
    if (! current())
        return;
    _modeHint->SetText(Resource(static_cast<unsigned>(view == EditorView::Source ? IDS_SOURCEHINT : view == EditorView::Split ? IDS_SPLITHINT : IDS_BANNER)));
    if (! current())
        return;
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    if (! current())
        return;
    Layout();
    if (! current())
        return;
    SelectionChanged();
    if (! current())
        return;
    _host.SetFocusControl(primary, false);
    if (! current())
        return;
    if (! IsInitializing())
        SavePreferences();
}
void Application::SelectionChanged(bool refreshTools)
{
    EditDiagnostics::SelectionInvocation selectionInvocation(_editDiagnostics);
    if (IsClosing() || _switchingDocument || ! _floating)
        return;
    if (_pendingBitmapPaste && (_pendingBitmapPaste->lifetime.expired() || _pendingBitmapPaste->documentKey != _session.key ||
                                _pendingBitmapPaste->view != _settings.view || _pendingBitmapPaste->documentPath != _session.path ||
                                _pendingBitmapPaste->selection != _editor->GetSelection() || _editor->IsSelectionGestureActive() ||
                                _editor->Snapshot().ChangedRangeFrom(_pendingBitmapPaste->source).kind != DxUi::RichText::SourceChangeKind::Unchanged))
        _pendingBitmapPaste.reset();
    if (_editor->IsSelectionGestureActive())
    {
        // Shared editor owns every selection route and the final release event.
        // Keep pointer movement O(1); derived facts and placement update on release.
        if (_activeOverlay == OverlayKind::FloatingSelectionTools)
            SetActiveOverlay(OverlayKind::None);
        else
        {
            _pendingSelectionTools = false;
            _selectionToolsFocus.reset();
        }
        return;
    }
    const auto selection = [&]
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionSnapshot);
        return _editor->GetSelection();
    }();
    const auto source = [&]
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionSnapshot);
        return _editor->Snapshot();
    }();
    const auto sourceRevision = _editor->Revision();
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::DocumentAnalysisRefresh);
        RefreshDocumentAnalysis(source, false);
    }
    const auto syntax = [&]
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionSnapshot);
        return _editor->GetMarkdownSyntax();
    }();
    bool selectionChanged{};
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionFactsCache);
        if (_selectionToolsFocus && (! _selectionToolsFocus->source.IsSameRevision(source) || _selectionToolsFocus->selection != selection ||
                                     _selectionToolsFocus->view != _settings.view))
            _selectionToolsFocus.reset();
        selectionChanged = ! _derivedCaches.selectionFactsSource || ! _derivedCaches.selectionFactsSource->IsSameRevision(source) ||
                           _derivedCaches.selectionFactsSelection != selection || _derivedCaches.selectionFactsView != _settings.view ||
                           _derivedCaches.selectionFactsAnalysis != _activity.changes;
    }
    if (selectionChanged || _derivedCaches.selectionFactsSyntax != syntax)
    {
        if (selectionChanged)
            ++_activity.selectionPresentationUpdates;
        else
            ++_activity.syntaxPresentationUpdates;
        if (selectionChanged && _settings.view == EditorView::Split)
        {
            // Text-service previews and cancellation notify selection without
            // a committed edit. Match source identity before using their ACPs;
            // equal-length previews can also change Unicode/CRLF boundaries.
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::Mirror);
            if (! IsUiAttached())
                return;
            auto* const primary        = _editor;
            auto* const target         = _mirror;
            auto* const owner          = _root;
            const auto primaryLifetime = GetControlLifetimeToken(*primary);
            const auto targetLifetime  = GetControlLifetimeToken(*target);
            const auto ownerLifetime   = GetControlLifetimeToken(*owner);
            const auto current         = [&]
            {
                if (IsClosing() || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || _editor != primary ||
                    _mirror != target || _root != owner || _host.GetRoot() != owner || _settings.view != EditorView::Split)
                    return false;
                return primary->Snapshot().IsSameRevision(source) && primary->Revision() == sourceRevision && primary->GetSelection() == selection;
            };
            if (! SynchronizeMirror() || ! current())
                return;
            // Read-only Source has no rich visual affinity. Adoption already
            // reset this exact selection; retain the canonical setter for a
            // real selection change or an incompatible-profile fallback.
            if (target->GetSelection() != selection || ! target->IsReadOnly() || target->GetViewMode() != DxUi::RichText::ViewMode::Source)
            {
                if (! AcceptSourceResult(target->TrySetSelection(selection)))
                    return;
            }
            if (! current())
                return;
            (void)target->ScrollSourceRangeIntoView(_host, selection.Start(), selection.End(), false);
            ++_activity.mirrorSelectionUpdates;
        }
        std::wstring expression;
        std::array<bool, 4> formats{};
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::InlineSemantics);
            // In-place math source previews what is being typed; only the region itself is read.
            if (const auto region = _editor->GetSemanticSourceRegion(); region && region->source.first <= region->source.last &&
                                                                        region->source.last <= source.Length() && selection.caret >= region->source.first &&
                                                                        selection.caret <= region->source.last)
                expression = MathRegionExpression(*region, source.Slice(region->source.first, region->source.last));
            if (syntax)
                for (const auto& node : syntax->Inlines())
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
                for (const auto& node : syntax->Nodes())
                    if (node.block.kind == DxUi::RichText::MarkdownBlockKind::FencedCode &&
                        (node.block.fenceMarker == L'$' || node.block.fenceMarker == L'[') && node.block.closed && selection.caret >= node.block.source.first &&
                        selection.caret <= WithoutLineEnd(syntax->Source(), node.block.source))
                    {
                        auto body = syntax->Slice(node.block.content);
                        while (! body.empty() && (body.back() == L'\n' || body.back() == L'\r'))
                            body.remove_suffix(1);
                        if (body.size() <= DxUi::RichText::MathMaximumSourceUnits)
                            expression = body;
                        break;
                    }
        }
        bool layoutChanged{};
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::ToolCaptionPublication);
            // The pinned grammar recognizes display delimiters only on their own lines.
            const auto preview = expression.empty() ? std::wstring{} : L"\\[\n" + expression + L"\n\\]";
            if (_equation->GetText() != preview)
                if (! AcceptSourceResult(_equation->TrySetText(preview)))
                    return;
            const bool nextEquationContext = ! expression.empty();
            layoutChanged                  = _equationContext != nextEquationContext;
            _equationContext               = nextEquationContext;
            for (unsigned i = 0; i < 4; ++i)
            {
                auto* button = _tools[i == 3 ? 13 : i + 3];
                button->SetEnabled(! _equationContext);
                button->SetToggleState(formats[i]);
            }
            if (! _selectionCaptionPublishedEquationContext.Matches(nextEquationContext))
            {
                ++_activity.selectionCaptionConstructions;
                const auto token = _selectionCaptionPublishedEquationContext.BeginPublish();
                _tools[2]->SetText(Resource(nextEquationContext ? IDS_MATH : IDS_PARAGRAPH));
                _selectionCaptionPublishedEquationContext.CommitPublish(token, nextEquationContext);
            }
        }
        if (selectionChanged)
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::Outline);
            _currentHeading.reset();
            for (std::size_t i = 0; i < _documentInfo.headings.size(); ++i)
                if (_documentInfo.headings[i].offset <= selection.caret)
                    _currentHeading = i;
            const auto headingId = _currentHeading ? std::optional<std::uint64_t>(_currentHeading.value() + 1) : std::nullopt;
            // Model refresh already reconciles surviving focused rows and keeps the viewport.
            // Republish only an actual selection or focus transition, which reveals it.
            if (_outlineTree->GetSelectedItemId() != headingId || _outlineTree->GetFocusedItemId() != headingId)
            {
                ++_activity.outlineSelectionUpdates;
                _outlineTree->SetSelectedItemId(headingId);
            }
        }
        DocumentPosition position;
        std::optional<std::pair<unsigned, std::size_t>> listFacts;
        unsigned blockResourceId{};
        std::size_t count{};
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::BlockLookup);
            position        = DocumentPositionAt(_documentInfo, selection.caret);
            count           = position.lineCharacters;
            blockResourceId = syntax ? CurrentBlockResourceAt(*syntax, selection.caret, &listFacts) : IDS_BLOCKPARAGRAPH;
            if (_equationContext)
                blockResourceId = IDS_MATH;
        }
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::BlockLabels);
            _currentBlockLatestFacts   = CurrentBlockLabelFacts{blockResourceId, count};
            const bool showListSummary = listFacts && (blockResourceId == IDS_BLOCKLIST || blockResourceId == IDS_BLOCKLISTCONTAINER);
            _listSummary->SetVisible(showListSummary);
            _listSummaryIcon->SetVisible(showListSummary);
            _currentBlock->SetVisible(! showListSummary);
            if (showListSummary && ! _listSummaryFacts.Matches(listFacts.value()))
            {
                const auto kind  = ResourceView(listFacts.value().first);
                const auto items = listFacts.value().second;
                const auto token = _listSummaryFacts.BeginPublish();
                _listSummary->SetText(FormatStringResource(nullptr, IDS_BLOCKITEMINFO, kind, items));
                _listSummaryFacts.CommitPublish(token, listFacts.value());
            }
            if (! PublishCurrentBlockLabelIfVisible())
                return;
        }
        if (selectionChanged)
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::PositionStatus);
            const auto selected = selection.End() - selection.Start();
            const auto encoding = _session.bom ? L" BOM" : L"";
            unsigned eolId      = IDS_EOLNONE;
            switch (_documentInfo.newlineKind)
            {
                case DocumentNewlineKind::CrLf: eolId = IDS_EOLCRLF; break;
                case DocumentNewlineKind::Lf: eolId = IDS_EOLLF; break;
                case DocumentNewlineKind::Cr: eolId = IDS_EOLCR; break;
                case DocumentNewlineKind::Mixed: eolId = IDS_EOLMIXED; break;
                case DocumentNewlineKind::None: break;
            }
            const auto eol = ResourceView(eolId);
            _positionStatus->SetText(FormatStringResource(nullptr, IDS_STATUSPOSITION, selected, position.line, position.column, encoding, eol));
        }
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::Find);
            RefreshFind();
        }
        EditDiagnostics::SelectionMeasure routingMeasure(_editDiagnostics, SelectionComponent::Routing);
        _derivedCaches.selectionFactsSyntax    = syntax;
        _derivedCaches.selectionFactsSource    = source;
        _derivedCaches.selectionFactsSelection = selection;
        _derivedCaches.selectionFactsView      = _settings.view;
        _derivedCaches.selectionFactsAnalysis  = _activity.changes;
        if (layoutChanged)
            Layout();
    }
    // Syntax publication refreshes facts, but must not reopen tools dismissed
    // by Escape or an explicit navigation command.
    if (! refreshTools)
        return;
    if (selection.Start() == selection.End() || _settings.view == EditorView::Source ||
        (_activeOverlay != OverlayKind::None && _activeOverlay != OverlayKind::FloatingSelectionTools) || _settings.focusMode || _equationContext)
    {
        if (_activeOverlay == OverlayKind::FloatingSelectionTools)
            SetActiveOverlay(OverlayKind::None);
        else
        {
            _pendingSelectionTools = false;
            _selectionToolsFocus.reset();
        }
        return;
    }
    const auto rectangles = _editor->TryGetTextInputRangeRects(_host, selection.Start(), selection.End());
    if (! rectangles)
    {
        _pendingSelectionTools = true;
        return;
    }
    _pendingSelectionTools = false;
    if (rectangles->empty())
    {
        // A prepared hidden-only source range has no painted anchor. It is not
        // pending work and must not create a permanent invalidation loop.
        SetActiveOverlay(OverlayKind::None);
        return;
    }
    const auto first       = rectangles->front();
    const float viewport   = _host.GetClientBoundsDip().right;
    const float left       = std::clamp(first.left, 16.f, std::max(16.f, viewport - 452));
    const float minimumTop = _editor->GetBounds().top + 4;
    const float top        = std::clamp(first.top - 52, minimumTop, std::max(minimumTop, _page->GetBounds().bottom - 52));
    _floating->SetBounds(D2D1::RectF(left, top, left + 436, top + 44));
    const std::array<float, 5> widths{112, 56, 56, 56, 56};
    float x = left + 8;
    for (std::size_t i = 0; i < _floatingActions.size(); ++i)
    {
        _floatingActions[i]->SetBounds(D2D1::RectF(x, top + 6, x + widths[i], top + 38));
        x += widths[i] + 4;
    }
    _selectionHintChip->SetBounds(D2D1::RectF(left + 372, top + 10, left + 428, top + 34));
    _selectionHint->SetBounds(D2D1::RectF(left + 376, top + 12, left + 424, top + 32));
    SetActiveOverlay(OverlayKind::FloatingSelectionTools);
    if (_selectionToolsFocus)
    {
        _selectionToolsFocus.reset();
        _host.SetFocusControl(_floatingActions.front(), MayRequestNativeFocus());
    }
}

void Application::FocusSelectionTools()
{
    _selectionToolsFocus = SelectionToolsFocusRequest{_editor->Snapshot(), _editor->GetSelection(), _settings.view};
    SelectionChanged();
}

void Application::SavePreferences()
{
    const bool allowSave = ! _options.headless || _options.preferencesProbe
#if defined(REDTILDE_ENABLE_TESTS)
                           || _options.watchSettingsForTest
#endif
        ;
    if (! allowSave || IsInitializing())
        return;
    const HRESULT result = _preferenceWriter.Submit(_settings);
    if (FAILED(result))
        Status(IDS_SETTINGSFAILED);
    else
        _windowPlacementDirty = false;
}

void Application::ReloadSettingsFromDisk()
{
    Settings reloaded;
    FileStamp stamp{};
    const HRESULT loadResult = LoadSettingsLive(_options.dataDirectory, reloaded, &stamp);
    if (FAILED(loadResult))
    {
        if (loadResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || loadResult == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
            return;
        OutputDebugStringW(L"RedTilde: ignoring malformed live settings edit on disk\n");
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Ignoring malformed live settings edit on disk");
        return;
    }

    if (stamp == _settingsFileStamp)
        return;
    if (const auto writtenStamp = _preferenceWriter.LastWrittenStamp(); writtenStamp && *writtenStamp == stamp)
    {
        _settingsFileStamp = stamp;
        return;
    }
    _settingsFileStamp = stamp;

    const bool appearanceChanged = _settings.appearance != reloaded.appearance;
    const bool clipboardChanged  = _settings.imageFirstPaste != reloaded.imageFirstPaste;
    const bool localNetChanged   = _settings.allowLocalNetworkImages != reloaded.allowLocalNetworkImages;
    const bool remoteChanged     = _settings.allowRemoteImages != reloaded.allowRemoteImages;
    const bool syntaxChanged     = _settings.markdownSyntax != reloaded.markdownSyntax;
    const bool layoutNeeded      = appearanceChanged || _settings.readingWidth != reloaded.readingWidth || _settings.textScale != reloaded.textScale ||
                                   _settings.showOutline != reloaded.showOutline || _settings.dimInactiveBlocks != reloaded.dimInactiveBlocks ||
                                   _settings.strikeCompletedTasks != reloaded.strikeCompletedTasks || syntaxChanged || remoteChanged || localNetChanged;

    _settings.appearance              = reloaded.appearance;
    _settings.readingWidth            = reloaded.readingWidth;
    _settings.textScale               = reloaded.textScale;
    _settings.showOutline             = reloaded.showOutline;
    _settings.dimInactiveBlocks       = reloaded.dimInactiveBlocks;
    _settings.allowRemoteImages       = reloaded.allowRemoteImages;
    _settings.allowLocalNetworkImages = reloaded.allowLocalNetworkImages;
    _settings.imageFirstPaste         = reloaded.imageFirstPaste;
    _settings.strikeCompletedTasks    = reloaded.strikeCompletedTasks;
    _settings.markdownSyntax          = reloaded.markdownSyntax;

    if (clipboardChanged)
    {
        _host.SetClipboardPreference(_settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
        if (_imageFirstPasteToggle)
            _imageFirstPasteToggle->SetToggleState(_settings.imageFirstPaste);
    }
    if (localNetChanged || remoteChanged)
    {
        _printNetworkImages.clear();
        _images.SetAllowLocalNetwork(_settings.allowLocalNetworkImages);
        if (localNetChanged && _editor)
            _editor->SetRemoteImagesEnabled(false);
        if (_editor)
            _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
        _images.DiscardCancelled();
        if (_remoteImagesToggle)
            _remoteImagesToggle->SetToggleState(_settings.allowRemoteImages);
        if (_localNetworkImagesToggle)
            _localNetworkImagesToggle->SetToggleState(_settings.allowLocalNetworkImages);
        ServiceImages();
    }
    if (syntaxChanged && _editor && _mirror && _equation)
        ApplyMarkdownSyntax();
    if (appearanceChanged)
        ApplyTheme();
    else if (layoutNeeded)
        Layout();
}
void Application::SetActiveOverlay(OverlayKind next)
{
    const bool wasFindBar    = _activeOverlay == OverlayKind::FindBar;
    const bool wasBlockDraft = _activeOverlay == OverlayKind::BlockDraftModal;
    if (next != OverlayKind::FloatingSelectionTools)
    {
        _pendingSelectionTools = false;
        _selectionToolsFocus.reset();
    }
    if (wasBlockDraft && next != OverlayKind::BlockDraftModal)
    {
        _blockDraftSession.reset();
        _editor->SetReadOnly(false);
        (void)AcceptSourceResult(_editor->TrySetSourceHighlights({}));
    }
    _floating->SetVisible(next == OverlayKind::FloatingSelectionTools);
    _findBar->SetVisible(next == OverlayKind::FindBar);
    _drawer->SetVisible(next == OverlayKind::SettingsDrawer);
    _blockDraft->SetVisible(next == OverlayKind::BlockDraftModal);
    _recoveryPanel->SetVisible(next == OverlayKind::RecoveryModal);
    _activeOverlay = next;
    if (wasFindBar && next != OverlayKind::FindBar)
        RefreshFind();
}

void Application::CloseDrawers()
{
    const bool hadBlockDraft = HasBlockDraft();
    SetActiveOverlay(OverlayKind::None);
    Layout();
    _host.SetFocusControl(_editor, MayRequestNativeFocus());
    if (! IsInitializing() && ! IsClosing())
        CheckpointRecovery();
    if (hadBlockDraft && std::holds_alternative<HeldOpenTx>(_activeFileTx))
        (void)PostMessageW(_window.get(), FileReady, 0, 0);
}

void Application::ReportUiFailure(std::string_view cause) noexcept
{
    MonitorDiagnostics::WriteUtf8(RedMonitor::Severity::Error, cause);
    if (_reportingUiFailure)
        return;
    _reportingUiFailure = true;
    const auto restore  = wil::scope_exit([&] { _reportingUiFailure = false; });
    if (std::holds_alternative<SaveTx>(_activeFileTx))
        _activeFileTx = std::monostate{};
    try
    {
        if (IsClosingAfterSave())
            ResumeAfterSaveClose();
        if (IsUiAttached())
            Status(IDS_ACTIONFAILED);
    }
    catch (const std::bad_alloc&)
    {
        DxUi::Failure::FailAllocation();
    }
    catch (const std::exception&)
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
    if (! CanAcceptCommand())
        return;
    if (command == Command::New || command == Command::Open || command == Command::OpenFolder || command == Command::Close ||
        command == Command::CloseDocument || command == Command::LocalSource || command == Command::RestoreRecovery || command == Command::RestoreAllRecovery)
        _pendingBitmapPaste.reset();
#if defined(REDTILDE_ENABLE_TESTS)
    if (_options.commandAdmissionForTest)
    {
        const HRESULT admitted = _options.commandAdmissionForTest(command);
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
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return;
    _selectionToolsFocus.reset();
    if (HasBlockDraft() && command != Command::ApplyBlock && command != Command::CloseDrawer)
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (command == Command::New || command == Command::Open || command == Command::OpenFolder || command == Command::Close ||
        command == Command::CloseDocument || command == Command::RestoreRecovery || command == Command::RestoreAllRecovery)
        AbandonStartupRestore();
    MarkdownCommandRequest request;
    bool semantic = true;
    switch (command)
    {
        case Command::FileMenu: FileMenu(); return;
        case Command::OpenFolder: OpenFolder(); return;
        case Command::Overflow: FileMenu(true); return;
        case Command::Files:
            if (_activeOverlay == OverlayKind::FloatingSelectionTools)
                SetActiveOverlay(OverlayKind::None);
            else
            {
                _pendingSelectionTools = false;
                _selectionToolsFocus.reset();
            }
            _settings.showFiles = ! _settings.showFiles;
            RefreshFiles(true);
            Layout();
            SavePreferences();
            _host.SetFocusControl(_settings.showFiles ? static_cast<DxUi::Control*>(_filesTree) : _editor, MayRequestNativeFocus());
            return;
        case Command::Minimize:
            if (! _options.headless)
                ShowWindow(_window.get(), SW_MINIMIZE);
            return;
        case Command::Maximize:
            if (! _options.headless)
                ShowWindow(_window.get(), IsZoomed(_window.get()) ? SW_RESTORE : SW_MAXIMIZE);
            return;
        case Command::Close: SendMessageW(_window.get(), WM_CLOSE, 0, 0); return;
        case Command::OpenLink:
        {
            const auto document = _editor->GetSemanticDocument();
            const auto index    = CurrentLinkElement(document);
            if (! index)
            {
                Status(IDS_LINKUNAVAILABLE);
                return;
            }
            const auto& link = document->Elements()[index.value()];
            if (link.destination.starts_with(L'#'))
            {
                if (! _editor->NavigateSemanticElement(_host, document, index.value()))
                    Status(IDS_NOTAVAILABLE);
            }
            else
                RequestOpenLink({link.source, link.destination, link.title, link.name, link.wiki});
            return;
        }
        case Command::FindPrevious: FindNext(true); return;
        case Command::Replace: ReplaceFind(false); return;
        case Command::ReplaceAll: ReplaceFind(true); return;
        case Command::RemoteImages:
            _printNetworkImages.clear();
            _settings.allowRemoteImages = ! _settings.allowRemoteImages;
            _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
            _images.DiscardCancelled();
            _remoteImagesToggle->SetToggleState(_settings.allowRemoteImages);
            ServiceImages();
            SavePreferences();
            return;
        case Command::LocalNetworkImages:
            _printNetworkImages.clear();
            _settings.allowLocalNetworkImages = ! _settings.allowLocalNetworkImages;
            _images.SetAllowLocalNetwork(_settings.allowLocalNetworkImages);
            // Retire existing request identities before applying the new transport policy.
            _editor->SetRemoteImagesEnabled(false);
            _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
            _images.DiscardCancelled();
            _localNetworkImagesToggle->SetToggleState(_settings.allowLocalNetworkImages);
            ServiceImages();
            SavePreferences();
            return;
        case Command::DocumentImages:
            if (_settings.allowRemoteImages)
                return;
            _printNetworkImages.clear();
            _session.remoteImagesAllowed = ! _session.remoteImagesAllowed;
            _editor->SetRemoteImagesEnabled(_session.remoteImagesAllowed);
            _images.DiscardCancelled();
            _imageFailureStatus = S_OK;
            ServiceImages();
            return;
        case Command::ReloadImages:
            _printNetworkImages.clear();
            _imageFailureStatus = S_OK;
            _editor->ReloadImages();
            _images.DiscardCancelled();
            ServiceImages();
            return;
        case Command::ImageFirstPaste:
            _settings.imageFirstPaste = ! _settings.imageFirstPaste;
            _host.SetClipboardPreference(_settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
            _imageFirstPasteToggle->SetToggleState(_settings.imageFirstPaste);
            SavePreferences();
            return;
        case Command::DimBlocks:
            _settings.dimInactiveBlocks = ! _settings.dimInactiveBlocks;
            Layout();
            SavePreferences();
            return;
        case Command::StrikeCompletedTasks:
            _settings.strikeCompletedTasks = ! _settings.strikeCompletedTasks;
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
            auto& enabled = _settings.markdownSyntax[static_cast<std::size_t>(command) - static_cast<std::size_t>(Command::SyntaxHighlight)];
            enabled       = ! enabled;
            ApplyMarkdownSyntax();
            Layout();
            SavePreferences();
            return;
        }
        case Command::Recovery: ShowRecovery(); return;
        case Command::LaterRecovery:
            SetActiveOverlay(OverlayKind::None);
            Layout();
            _host.SetFocusControl(_editor, MayRequestNativeFocus());
            return;
        case Command::DiscardRecovery:
            if (! _recoveryCandidates.empty())
            {
                auto& front = _recoveryCandidates.front();
                if (front.disposition == RecoveryDisposition::Discarding)
                {
                    Status(IDS_RECOVERYBUSY);
                    return;
                }
                front.disposition = RecoveryDisposition::Discarding;
                if (FAILED(_recovery.Discard(front.id)))
                {
                    front.disposition = RecoveryDisposition::Available;
                    Status(IDS_RECOVERYFAILED);
                    return;
                }
                Status(IDS_RECOVERYBUSY);
            }
            return;
        case Command::RestoreRecovery:
            if (! _recoveryCandidates.empty())
            {
                if (_recoveryCandidates.front().disposition == RecoveryDisposition::Discarding)
                {
                    Status(IDS_RECOVERYBUSY);
                    return;
                }
                AskThen([this, candidate = _recoveryCandidates.front()]
                {
                    // A prompt or admitted save can pump recovery completions.
                    // Only the exact copy clicked before that boundary may restore.
                    const auto found = std::ranges::find_if(_recoveryCandidates, [&candidate](const auto& current) { return current.id == candidate.id; });
                    if (found == _recoveryCandidates.end())
                    {
                        Status(IDS_RECOVERYUNAVAILABLE);
                        return;
                    }
                    if (found->disposition != RecoveryDisposition::Available)
                    {
                        Status(IDS_RECOVERYBUSY);
                        return;
                    }
                    if (! StartWorkspaceDocument() || ! IsUiAttached())
                        return;
                    if (! _inactiveDocuments.empty())
                        _workspaceSelectionPreferred = true;
                    SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom, {}, false);
                    _session.recovered = true;
                    (void)AcceptSourceResult(_editor->TrySetSelection(candidate.snapshot.selection));
                    Changed();
                    const auto snapshot = RecoverySnapshot{_editor->GetText(), _session.path, _session.bom, _editor->GetSelection(), ++_recoveryGeneration};
                    if (FAILED(_recovery.SubmitDocument(_session.key, snapshot, true, candidate.id)))
                    {
                        Status(IDS_RECOVERYFAILED);
                        return;
                    }
                    std::erase_if(_recoveryCandidates, [&candidate](const auto& current) { return current.id == candidate.id; });
                    CloseDrawers();
                    Status(IDS_RECOVERED);
                });
            }
            return;
        case Command::RestoreAllRecovery:
        {
            if (_files.Busy() || HasBlockDraft() || HasActiveModalPrompt() || _recoveryCandidates.empty())
            {
                Status(IDS_BUSY);
                return;
            }
            if (std::ranges::any_of(_recoveryCandidates, [](const auto& candidate) { return candidate.disposition == RecoveryDisposition::Discarding; }))
            {
                Status(IDS_RECOVERYBUSY);
                return;
            }
            const auto candidates   = _recoveryCandidates;
            const std::size_t reuse = _session.path.empty() && ! Dirty() && ! _editor->CanUndo() && ! _editor->CanRedo() ? 1u : 0u;
            if (_inactiveDocuments.size() + 1 + candidates.size() - reuse > RecoveryService::MaximumDocuments)
            {
                Status(IDS_WORKSPACEFULL);
                return;
            }
            for (const auto& candidate : candidates)
            {
                const auto found = std::ranges::find_if(_recoveryCandidates, [&](const auto& current) { return current.id == candidate.id; });
                if (found == _recoveryCandidates.end() || found->disposition != RecoveryDisposition::Available)
                    continue; // A completion retired or started discarding this exact copy; never substitute another.
                if (! StartWorkspaceDocument() || ! IsUiAttached())
                    return;
                SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom, {}, false);
                if (! IsUiAttached())
                    return;
                _session.recovered = true;
                (void)AcceptSourceResult(_editor->TrySetSelection(candidate.snapshot.selection));
                Changed();
                if (! IsUiAttached())
                    return;
                const auto snapshot = RecoverySnapshot{_editor->GetText(), _session.path, _session.bom, _editor->GetSelection(), ++_recoveryGeneration};
                if (FAILED(_recovery.SubmitDocument(_session.key, snapshot, true, candidate.id)))
                {
                    Status(IDS_RECOVERYFAILED);
                    return; // Keep the original candidate until its replacement can be admitted.
                }
                std::erase_if(_recoveryCandidates, [&](const auto& current) { return current.id == candidate.id; });
            }
            if (! _inactiveDocuments.empty())
                _workspaceSelectionPreferred = true;
            RefreshFiles();
            CloseDrawers();
            Status(IDS_RECOVERED);
            return;
        }
        case Command::New:
            AskWorkspaceThen([this]
            {
                ResetWorkspace();
                SetDocument({});
                _host.SetFocusControl(_editor, MayRequestNativeFocus());
            });
            return;
        case Command::Open: Open(); return;
        case Command::Save: Save(); return;
        case Command::SaveAs: Save(true); return;
        case Command::CloseDocument: CloseActiveDocument(); return;
        case Command::Print: Print(); return;
        case Command::Compose: SetView(EditorView::Compose); return;
        case Command::Source: SetView(EditorView::Source); return;
        case Command::Split: SetView(EditorView::Split); return;
        case Command::Undo:
            (void)_editor->Undo();
            semantic = false;
            break;
        case Command::Redo:
            (void)_editor->Redo();
            semantic = false;
            break;
        case Command::Heading:
        case Command::Insert:
        {
            const auto bounds = _tools[command == Command::Heading ? 2 : 8]->GetBounds();
            Menu(_host.DipPointToScreenPoint({bounds.left, bounds.bottom}), command == Command::Insert, command == Command::Heading);
        }
            return;
        case Command::Bold:
        case Command::Italic:
        case Command::Strike:
        case Command::Code:
            if (_settings.view == EditorView::Source)
            {
                const std::wstring_view marker = command == Command::Bold     ? L"**"
                                                 : command == Command::Italic ? L"*"
                                                 : command == Command::Strike ? L"~~"
                                                                              : L"`";
                if (! _editor->ToggleFormatting(marker))
                {
                    if (_editor->Snapshot().Length() + marker.size() * 2u > Document::MaximumUnits)
                        ReportCapacityLimit();
                    else
                        Status(IDS_NOTAVAILABLE);
                }
                semantic = false;
            }
            else
            {
                request.command = MarkdownCommand::ToggleStyle;
                request.style   = command == Command::Bold     ? MarkdownTextStyle::Strong
                                  : command == Command::Italic ? MarkdownTextStyle::Emphasis
                                  : command == Command::Strike ? MarkdownTextStyle::Strikethrough
                                                               : MarkdownTextStyle::Code;
            }
            break;
        case Command::Bullets: request.command = MarkdownCommand::SetList; break;
        case Command::Numbered:
            request.command  = MarkdownCommand::SetList;
            request.listKind = MarkdownListKind::Ordered;
            break;
        case Command::Task:
            request.command  = MarkdownCommand::SetList;
            request.listKind = MarkdownListKind::Task;
            break;
        case Command::Paragraph:
        case Command::H1:
        case Command::H2:
        case Command::H3:
            request.command      = MarkdownCommand::SetHeading;
            request.headingLevel = static_cast<unsigned>(command) - static_cast<unsigned>(Command::Paragraph);
            break;
        case Command::Table: request.command = MarkdownCommand::InsertTable; break;
        case Command::RowAdd: request.command = MarkdownCommand::InsertTableRow; break;
        case Command::RowRemove: request.command = MarkdownCommand::DeleteTableRow; break;
        case Command::ColumnAdd: request.command = MarkdownCommand::InsertTableColumn; break;
        case Command::ColumnRemove: request.command = MarkdownCommand::DeleteTableColumn; break;
        case Command::Link:
        {
            const auto selection = _editor->GetSelection();
            LocalSource();
            if (HasBlockDraft())
            {
                auto& draft      = _blockDraftSession.value();
                draft.range      = {selection.Start(), selection.End()};
                const auto label = draft.snapshot.Slice(selection.Start(), selection.End());
                _blockDraftText->SetText(L"[" + (label.empty() ? Resource(IDS_DEFAULTLINKTEXT) : std::wstring(label)) + L"](https://)");
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
            if (_editor->TryPrepareClipboardPaste(content, insertion))
            {
                const auto selection     = _editor->GetSelection();
                const auto currentLength = _editor->Snapshot().Length();
                const auto replaced      = selection.End() - selection.Start();
                const auto insertedUnits = insertion.size();
                if (! _editor->ReplaceSelection(std::move(insertion)))
                {
                    if (currentLength - replaced + insertedUnits > Document::MaximumUnits)
                        ReportCapacityLimit();
                    else
                        Status(IDS_NOTAVAILABLE);
                }
                else
                    Status(IDS_FORMATTED);
            }
            else
                Status(IDS_NOTAVAILABLE);
            semantic = false;
            break;
        }
        case Command::Mermaid:
        case Command::Math:
        {
            // Display delimiters must own their lines; a one-line $$...$$ stays literal at this pin.
            const auto snippet =
                NormalizeInsertedLineEndings(command == Command::Mermaid ? L"\n\n```mermaid\ngraph LR\nA[Idea] --> B[Write]\nB --> C[Share]\n```\n\n"
                                                                         : L"\n\n$$\nx = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}\n$$\n\n",
                                             CurrentText());
            const auto selection     = _editor->GetSelection();
            const auto currentLength = _editor->Snapshot().Length();
            const auto replaced      = selection.End() - selection.Start();
            const auto insertedUnits = snippet.size();
            if (! _editor->ReplaceSelection(snippet))
            {
                if (currentLength - replaced + insertedUnits > Document::MaximumUnits)
                    ReportCapacityLimit();
                else
                    Status(IDS_NOTAVAILABLE);
            }
            else
                Status(IDS_FORMATTED);
            semantic = false;
            break;
        }
        case Command::Quote:
        case Command::Fence:
        {
            const auto selection = _editor->GetSelection();
            const bool quote     = command == Command::Quote;
            const auto snippet =
                NormalizeInsertedLineEndings(quote ? L"\n> A thought worth keeping.\n" : L"\n\n```text\nYour code here\n```\n\n", CurrentText());
            const bool changed = selection.Start() != selection.End()
                                     ? _editor->WrapSelectionInMarkdownBlock(quote ? MarkdownBlockKind::BlockQuote : MarkdownBlockKind::FencedCode,
                                                                             quote ? std::wstring_view{} : std::wstring_view(L"text"))
                                     : _editor->ReplaceSelection(snippet);
            if (changed)
                Status(IDS_FORMATTED);
            else
            {
                const auto currentLength    = _editor->Snapshot().Length();
                const auto replaced         = selection.End() - selection.Start();
                const bool capacityExceeded = selection.Start() != selection.End() ? currentLength + (quote ? 2u : 12u) > Document::MaximumUnits
                                                                                   : currentLength - replaced + snippet.size() > Document::MaximumUnits;
                if (capacityExceeded)
                    ReportCapacityLimit();
                else
                    Status(IDS_NOTAVAILABLE);
            }
            semantic = false;
            break;
        }
        case Command::LocalSource: LocalSource(); return;
        case Command::Find:
            SetActiveOverlay(OverlayKind::FindBar);
            Layout();
            RefreshFind();
            _host.SetFocusControl(_findText, MayRequestNativeFocus());
            return;
        case Command::FindNext: FindNext(); return;
        case Command::FindMatchCase:
            _find.matchCase = ! _find.matchCase;
            _find.InvalidateMatches();
            _findCaseToggle->SetToggleState(_find.matchCase);
            RefreshFind();
            if (_activeOverlay == OverlayKind::FindBar)
                _host.SetFocusControl(_findText, MayRequestNativeFocus());
            return;
        case Command::ZoomIn:
        case Command::ZoomOut:
        case Command::ZoomReset:
        {
            const int delta  = static_cast<int>(std::min(zoomSteps, 25u)) * (command == Command::ZoomIn ? 5 : -5);
            const auto scale = command == Command::ZoomReset ? 100u : static_cast<unsigned>(std::clamp(static_cast<int>(_settings.textScale) + delta, 75, 200));
            if (_settings.textScale != scale)
            {
                _settings.textScale = scale;
                Layout();
                SavePreferences();
            }
            return;
        }
        case Command::Settings:
        {
            const bool opening = _activeOverlay != OverlayKind::SettingsDrawer;
            SetActiveOverlay(opening ? OverlayKind::SettingsDrawer : OverlayKind::None);
            Layout();
            _host.SetFocusControl(opening ? static_cast<DxUi::Control*>(_themeButtons[0]) : _editor, MayRequestNativeFocus());
            return;
        }
        case Command::Focus:
            _settings.focusMode = ! _settings.focusMode;
            CloseDrawers();
            Layout();
            return;
        case Command::Outline:
            _settings.showOutline = ! _settings.showOutline;
            Layout();
            SavePreferences();
            return;
        case Command::ThemeSystem:
        case Command::ThemeLight:
        case Command::ThemeDark:
        case Command::ThemePaper:
            _settings.appearance = static_cast<Appearance>(static_cast<unsigned>(command) - static_cast<unsigned>(Command::ThemeSystem));
            ApplyTheme();
            SavePreferences();
            return;
        case Command::CloseDrawer: CloseDrawers(); return;
        case Command::ApplyBlock: ApplyBlockSource(); return;
    }
    if (semantic)
    {
        const auto result = _editor->ExecuteMarkdownCommand(request);
        if (result == MarkdownCommandStatus::Unavailable)
        {
            if (IsMarkdownCommandCapacityExceeded(*_editor, request, _editor->GetSelection()))
                ReportCapacityLimit();
            else
                Status(IDS_NOTAVAILABLE);
        }
        else
            Status(IDS_FORMATTED);
    }
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    _host.SetFocusControl(_editor, MayRequestNativeFocus());
}

void Application::LocalSource()
{
    if (_editor->GetSemanticSourceRegion())
    {
        if (_editor->EndSemanticSourceEdit(_host))
            SelectionChanged();
        return;
    }
    const auto semantic  = _editor->GetAcknowledgedSemanticDocument(_host);
    const auto selection = _editor->GetSelection();
    const auto source    = _editor->GetText();
    if (semantic)
    {
        const auto elements = semantic->Elements();
        for (std::size_t index = elements.size(); index > 0; --index)
        {
            const auto& element = elements[index - 1];
            // Code, diagrams and math edit their exact source in place. A display equation
            // ends at its closing delimiter, so the following line is not inside it.
            const auto elementLast = element.role == DxUi::RichText::SemanticRole::Math ? WithoutLineEnd(source, element.source) : element.source.last;
            if (selection.caret >= element.source.first && selection.caret <= elementLast && _editor->BeginSemanticSourceEdit(_host, semantic, index - 1))
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
    if (const auto document = _editor->GetSemanticDocument())
        for (const auto& element : document->Elements())
            if (element.role == DxUi::RichText::SemanticRole::Math && selection.caret >= element.source.first &&
                selection.caret <= WithoutLineEnd(source, element.source))
            {
                first = element.source.first;
                last  = WithoutLineEnd(source, element.source);
                break;
            }
    if (first == last)
        if (const auto syntax = _editor->GetMarkdownSyntax())
            for (const auto& node : syntax->Inlines())
                if (node.kind == DxUi::RichText::MarkdownInlineKind::Math && selection.caret >= node.source.first && selection.caret <= node.source.last)
                {
                    first = node.source.first;
                    last  = node.source.last;
                    break;
                }
    if (first == last)
    {
        first = first ? source.find_last_of(L"\r\n", first - 1) : source.npos;
        first = first == source.npos ? 0 : first + 1;
        last  = source.find_first_of(L"\r\n", selection.caret);
        last  = last == source.npos ? source.size() : last;
    }
    _blockDraftSession = BlockDraftSession{_editor->Snapshot(), _editor->Revision(), {first, last}, false};
    _blockDraftText->SetText(source.substr(first, last - first));
    _blockDraft->GetLogicalChild(0)->SetAccessibleName(Resource(_equationContext ? IDS_BLOCKEQUATION : IDS_LOCAL));
    static_cast<DxUi::Label*>(_blockDraft->GetLogicalChild(0))->SetText(Resource(_equationContext ? IDS_BLOCKEQUATION : IDS_LOCAL));
    SetActiveOverlay(OverlayKind::BlockDraftModal);
    _editor->SetReadOnly(true);
    if (last > first)
    {
        const DxUi::RichTextSourceHighlight block{first, last - first, true};
        (void)AcceptSourceResult(_editor->TrySetSourceHighlights(std::span(&block, 1)));
    }
    Layout();
    _host.SetFocusControl(_blockDraftText, MayRequestNativeFocus());
    CheckpointRecovery();
}

void Application::ApplyBlockSource()
{
    if (! HasBlockDraft())
        return;
    const auto& draftSession = _blockDraftSession.value();
    if (! _editor->Snapshot().IsSameRevision(draftSession.snapshot) || _editor->Revision() != draftSession.revision)
    {
        Status(IDS_NOTAVAILABLE);
        return;
    }
    const auto draft         = _blockDraftText->GetText();
    const auto original      = draftSession.snapshot.Slice(draftSession.range.first, draftSession.range.last);
    const std::wstring value = draft == original ? std::wstring(draft) : NormalizeInsertedLineEndings(draft, CurrentText());
    if (! DxUi::RichText::IsValidUtf16(value))
    {
        Status(IDS_NOTAVAILABLE);
        return;
    }
    if (draftSession.snapshot.Length() - (draftSession.range.last - draftSession.range.first) + value.size() > DxUi::RichText::Document::MaximumUnits)
    {
        ReportCapacityLimit();
        return;
    }
    const DxUi::RichText::SourceEdit edit{draftSession.range, value};
    const auto end = draftSession.range.first + value.size();
    _editor->SetReadOnly(false);
    if (! _editor->ApplySourceEdits(draftSession.snapshot, draftSession.revision, std::span(&edit, 1), {end, end}))
    {
        CloseDrawers();
        Status(IDS_NOTAVAILABLE);
        return;
    }
    CloseDrawers();
    Status(IDS_FORMATTED);
}

std::vector<DxUi::MenuFlyoutItem> Application::ContextMenuItems(bool insertOnly, bool headingOnly)
{
    std::vector<DxUi::MenuFlyoutItem> items;
    auto add = [&](unsigned label, Command command, std::wstring accelerator = {})
    {
        DxUi::MenuFlyoutItem item;
        item.text            = Resource(label);
        item.commandId       = static_cast<int>(command) + 1;
        item.acceleratorText = std::move(accelerator);
        items.push_back(std::move(item));
    };
    if (! insertOnly)
    {
        add(IDS_PARAGRAPH, Command::Paragraph);
        add(IDS_H1, Command::H1);
        add(IDS_H2, Command::H2);
        add(IDS_H3, Command::H3);
        if (! headingOnly)
        {
            add(IDS_BOLD, Command::Bold, L"Ctrl+B");
            add(IDS_ITALIC, Command::Italic, L"Ctrl+I");
            add(IDS_STRIKE, Command::Strike);
            add(IDS_CODE, Command::Code);
            add(IDS_NUMBERED, Command::Numbered);
            add(IDS_LOCAL, Command::LocalSource, L"F2");
            if (CurrentLinkElement(_editor->GetSemanticDocument()))
                add(IDS_OPENLINK, Command::OpenLink);
            add(IDS_ROWADD, Command::RowAdd);
            add(IDS_ROWREMOVE, Command::RowRemove);
            add(IDS_COLADD, Command::ColumnAdd);
            add(IDS_COLREMOVE, Command::ColumnRemove);
        }
    }
    if (! headingOnly)
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
    if (_options.headless)
        return; // Focus-taking menus are an explicitly interactive gate.
    auto items           = ContextMenuItems(insertOnly, headingOnly);
    const auto source    = _editor->Snapshot();
    const auto selection = _editor->GetSelection();
    const auto key       = _session.key;
    const auto lifetime  = GetControlLifetimeToken(*_editor);
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(_window.get(),
                                       point,
                                       items,
                                       _host.GetTheme(),
                                       [this, source, selection, key, lifetime](std::optional<int> selected) noexcept
    {
        RunUiAction([&]
        {
            if (selected == static_cast<int>(Command::OpenLink) + 1)
            {
                if (! IsUiAttached() || lifetime.expired())
                    return;
                if (_session.key != key || ! _editor->Snapshot().IsSameRevision(source) || _editor->GetSelection() != selection)
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
    if (_options.headless && _options.filePickerForTest && ! image && ! folder)
        return _options.filePickerForTest(saving);
#endif
    if (_options.headless)
        return {};
    wil::com_ptr<IFileDialog> dialog;
    const CLSID classId = saving ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hr          = DxUi::Failure::InvokeNative(
        &CoCreateInstance, classId, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER), __uuidof(IFileDialog), reinterpret_cast<void**>(dialog.put()));
    if (FAILED(hr))
        return {};
    const auto markdownLabel = Resource(IDS_FILTERMARKDOWN), utf8Label = Resource(IDS_FILTERUTF8), allLabel = Resource(IDS_FILTERALL),
               imageLabel = Resource(IDS_FILTERIMAGES);
    const COMDLG_FILTERSPEC markdown[]{{markdownLabel.c_str(), L"*.md;*.markdown"}, {utf8Label.c_str(), L"*.txt"}, {allLabel.c_str(), L"*.*"}};
    const COMDLG_FILTERSPEC pictures[]{{imageLabel.c_str(), L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"}};
    if (! folder)
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileTypes, image ? 1u : 3u, image ? pictures : markdown);
    else
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetTitle, Resource(IDS_OPENFOLDER).c_str());
    DWORD flags{};
    (void)InvokeNativeMethod(dialog.get(), &IFileDialog::GetOptions, &flags);
    (void)InvokeNativeMethod(dialog.get(),
                             &IFileDialog::SetOptions,
                             flags | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR |
                                 (folder   ? FOS_PICKFOLDERS | FOS_PATHMUSTEXIST
                                  : saving ? FOS_OVERWRITEPROMPT
                                           : FOS_FILEMUSTEXIST));
    if (! _settings.filesRoot.empty())
    {
        wil::com_ptr<IShellItem> initial;
        if (SUCCEEDED(DxUi::Failure::InvokeNative(
                &SHCreateItemFromParsingName, _settings.filesRoot.c_str(), nullptr, __uuidof(IShellItem), reinterpret_cast<void**>(initial.put()))))
            (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultFolder, initial.get());
    }
    if (saving)
    {
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetDefaultExtension, L"md");
        const auto baseName = _session.path.filename().empty() ? std::filesystem::path(Resource(IDS_UNTITLED)).stem().native() : _session.path.stem().native();
        const auto name     = _session.recovered      ? FormatStringResource(nullptr, IDS_RECOVEREDNAME, baseName)
                              : _session.path.empty() ? Resource(IDS_UNTITLED)
                                                      : _session.path.filename().native();
        (void)InvokeNativeMethod(dialog.get(), &IFileDialog::SetFileName, name.c_str());
    }
    if (FAILED(InvokeNativeMethod(dialog.get(), &IFileDialog::Show, _window.get())))
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
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return;
    if (HasBlockDraft())
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    if (_files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    if (! Dirty())
    {
        continuation();
        return;
    }
    if (_options.headless
#if defined(REDTILDE_ENABLE_TESTS)
        && ! _options.unsavedPromptForTest
#endif
    )
        return;
    int choice = IDCANCEL;
    {
        // A modal loop can dispatch another Close. The original question alone
        // owns this continuation until its answer has been collected.
        _modalPrompt       = ModalPromptKind::UnsavedChanges;
        const auto restore = wil::scope_exit([&] { _modalPrompt = ModalPromptKind::None; });
#if defined(REDTILDE_ENABLE_TESTS)
        if (_options.headless)
            choice = _options.unsavedPromptForTest();
        else
#endif
            choice = DxUi::Failure::InvokeNative(&MessageBoxW,
                                                 _window.get(),
                                                 Resource(IDS_SAVEQUESTION).c_str(),
                                                 Resource(IDS_APP).c_str(),
                                                 static_cast<UINT>(MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3));
    }
    if (choice == IDNO)
        continuation();
    else if (choice == IDYES)
    {
        _activeFileTx = SaveTx{std::move(continuation)};
        Save();
    }
}
void Application::AskWorkspaceThen(std::function<void()> continuation, std::vector<WorkspaceApproval> approved)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
        return;
    const auto accepted = [&](unsigned key, const DxUi::RichText::SourceSnapshot& source)
    { return std::ranges::any_of(approved, [&](const auto& value) { return value.key == key && value.source.IsSameRevision(source); }); };
    if (! Dirty() || accepted(_session.key, _editor->Snapshot()))
    {
        const auto pending = std::find_if(_inactiveDocuments.begin(), _inactiveDocuments.end(), [&](const auto& document) {
            return document.dirty && ! accepted(document.metadata.key, document.state.Snapshot());
        });
        if (pending == _inactiveDocuments.end())
        {
            _workspaceApprovals = std::move(approved);
            continuation();
            return;
        }
        if (! SwitchWorkspaceDocument(static_cast<std::size_t>(pending - _inactiveDocuments.begin())))
            return;
        if (! IsUiAttached())
            return;
    }
    const auto promptingSource = _editor->Snapshot();
    const auto promptingKey    = _session.key;
    AskThen([this, promptingSource, promptingKey, continuation = std::move(continuation), approved = std::move(approved)]() mutable
    {
        approved.push_back({promptingKey, promptingSource});
        AskWorkspaceThen(std::move(continuation), std::move(approved));
    });
}
void Application::Open()
{
    AskWorkspaceThen([this]
    {
        const auto path = PickFile(false);
        if (path.empty())
            return;
        auto entrySource = _editor->Snapshot();
        const HRESULT hr = _files.Submit(false, path);
        if (FAILED(hr))
        {
            _activeFileTx = std::monostate{};
            Status(IDS_BUSY);
        }
        else
        {
            _activeFileTx = PickerOpenTx{std::move(entrySource), _workspaceApprovals};
            Status(IDS_LOADING);
        }
    });
}
void Application::Save(bool saveAs, bool force, std::optional<std::filesystem::path> selectedPath)
{
    if (! CanAcceptCommand() || HasActiveModalPrompt())
    {
        if (std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = std::monostate{};
        return;
    }
    saveAs = saveAs || _session.recovered;
    if (_files.Busy())
    {
        if (std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = std::monostate{};
        Status(IDS_BUSY);
        return;
    }
    const auto source        = _editor->Snapshot();
    const auto key           = _session.key;
    const auto originalPath  = _session.path;
    const auto originalStamp = _session.stamp;
    const bool originalBom = _session.bom, originalRecovered = _session.recovered;
    auto path = selectedPath ? selectedPath.value() : _session.path;
    if (! selectedPath && (saveAs || path.empty()))
        path = PickFile(true);
    if (path.empty())
    {
        _activeFileTx = std::monostate{};
        return;
    }
    if (! CanAcceptCommand())
    {
        _activeFileTx = std::monostate{};
        return;
    }
    if (key != _session.key || _session.path != originalPath || _session.stamp != originalStamp || _session.bom != originalBom ||
        _session.recovered != originalRecovered || ! _editor->Snapshot().IsSameRevision(source))
    {
        _activeFileTx = std::monostate{};
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    const auto expected         = (! force && ! _session.path.empty()) ? std::optional<FileStamp>(_session.stamp) : std::nullopt;
    const bool sameIdentityOnly = saveAs && path.lexically_normal() != _session.path.lexically_normal();
    const SaveRecoveryContext protection{_options.dataDirectory, _editor->GetSelection(), ++_recoveryGeneration};
    const HRESULT hr = _files.Submit(true, path, _editor->GetText(), _session.bom, expected, sameIdentityOnly, false, _session.path, protection);
    if (FAILED(hr))
    {
        _activeFileTx = std::monostate{};
        Status(IDS_SAVEFAILED);
    }
    else
    {
        if (! std::holds_alternative<SaveTx>(_activeFileTx))
            _activeFileTx = SaveTx{};
        Status(IDS_SAVING);
    }
}
void Application::ResolveSaveConflict(const AsyncFiles::Result& result)
{
    if (! IsUiAttached())
    {
        _activeFileTx = std::monostate{};
        return;
    }
    const auto source      = _editor->Snapshot();
    const auto key         = _session.key;
    const auto path        = _session.path;
    const auto identity    = _session.stamp;
    const bool originalBom = _session.bom, originalRecovered = _session.recovered;
    Status(IDS_CONFLICT);
    SaveConflictChoice choice = SaveConflictChoice::Cancel;
    {
        const auto previousPrompt = std::exchange(_modalPrompt, ModalPromptKind::SaveConflict);
        const auto restore        = wil::scope_exit([&] { _modalPrompt = previousPrompt; });
#if defined(REDTILDE_ENABLE_TESTS)
        if (_options.headless && _options.saveConflictForTest)
            choice = _options.saveConflictForTest();
        else
#endif
            if (! _options.headless)
        {
            const auto dialogTitle = Resource(IDS_APP), instruction = Resource(IDS_CONFLICT), content = Resource(IDS_CONFLICTHELP);
            const auto reload = Resource(IDS_RELOAD), saveAs = Resource(IDS_SAVEAS), overwrite = Resource(IDS_OVERWRITE), cancel = Resource(IDS_CANCEL);
            const TASKDIALOG_BUTTON buttons[]{
                {IDS_RELOAD, reload.c_str()}, {IDS_SAVEAS, saveAs.c_str()}, {IDS_OVERWRITE, overwrite.c_str()}, {IDCANCEL, cancel.c_str()}};
            TASKDIALOGCONFIG config{};
            config.cbSize             = sizeof(config);
            config.hwndParent         = _window.get();
            config.dwFlags            = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
            config.pszWindowTitle     = dialogTitle.c_str();
            config.pszMainIcon        = TD_WARNING_ICON;
            config.pszMainInstruction = instruction.c_str();
            config.pszContent         = content.c_str();
            config.cButtons           = static_cast<UINT>(std::size(buttons));
            config.pButtons           = buttons;
            config.nDefaultButton     = IDCANCEL;
            int pressed               = IDCANCEL;
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
    if (! IsUiAttached())
    {
        _activeFileTx = std::monostate{};
        return;
    }
    if (key != _session.key || path != _session.path || _session.stamp != identity || originalBom != _session.bom || originalRecovered != _session.recovered ||
        ! _editor->Snapshot().IsSameRevision(source))
    {
        _activeFileTx = std::monostate{};
        Status(IDS_FILECONTEXTCHANGED);
        return;
    }
    if (choice == SaveConflictChoice::Overwrite)
        Save(false, true, result.path);
    else if (choice == SaveConflictChoice::SaveAs)
        Save(true);
    else
    {
        _activeFileTx = std::monostate{};
        if (choice == SaveConflictChoice::Reload)
        {
            if (SUCCEEDED(_files.Submit(false, result.path, {}, false, {}, false, true)))
            {
                _activeFileTx = ConflictReloadTx{key, source, path};
                Status(IDS_LOADING);
            }
            else
                Status(IDS_BUSY);
        }
    }
}
void Application::OnMenuClosed(std::optional<int> selected) noexcept
{
    RunUiAction([&]
    {
        if (IsClosing() || IsSessionEnding() || ! _root || _host.GetRoot() != _root)
            return;
        if (selected)
            Execute(static_cast<Command>(selected.value() - 1));
        // Commands may retire the root or open a draft/drawer that owns focus.
        if (CanAcceptCommand() && ! _printPreview && (_activeOverlay == OverlayKind::None || _activeOverlay == OverlayKind::FloatingSelectionTools))
            _host.SetFocusControl(_editor, MayRequestNativeFocus());
    });
}
void Application::FileCompleted() noexcept
{
    RunUiAction([&] { CompleteFileOperation(); });
}
void Application::CompleteFileOperation()
{
#if defined(REDTILDE_ENABLE_TESTS)
    if (_options.fileCompletionAdmissionForTest)
    {
        const HRESULT admitted = _options.fileCompletionAdmissionForTest();
        if (FAILED(admitted))
        {
            ReportUiFailure("checked file-completion admission refusal");
            return;
        }
    }
#endif
    std::optional<AsyncFiles::Result> completed;
    bool fromFiles{};
    std::optional<DxUi::RichText::SourceSnapshot> entrySource;
    std::vector<WorkspaceApproval> approvals;
    bool previousWorkspaceSelectionPreferred{};
    std::optional<ConflictReloadTx> reloadTx;
    if (auto* held = std::get_if<HeldOpenTx>(&_activeFileTx))
    {
        if (HasBlockDraft())
        {
            Status(IDS_BLOCKPENDING);
            return;
        }
        completed                           = std::move(held->result);
        fromFiles                           = held->fromFiles;
        entrySource                         = std::move(held->entrySource);
        approvals                           = std::move(held->approvals);
        previousWorkspaceSelectionPreferred = held->previousWorkspaceSelectionPreferred;
        reloadTx                            = std::move(held->conflictReload);
        _activeFileTx                       = std::monostate{};
    }
    else
    {
        completed = _files.Take();
        if (! completed)
            return;
        if (! completed.value().saving && HasBlockDraft())
        {
            HeldOpenTx stagedHeld;
            stagedHeld.result = std::move(completed.value());
            if (auto* picker = std::get_if<PickerOpenTx>(&_activeFileTx))
            {
                stagedHeld.entrySource = std::move(picker->entrySource);
                stagedHeld.approvals   = std::move(picker->approvals);
            }
            else if (auto* filesTx = std::get_if<FilesPaneOpenTx>(&_activeFileTx))
            {
                stagedHeld.fromFiles                           = true;
                stagedHeld.entrySource                         = std::move(filesTx->entrySource);
                stagedHeld.previousWorkspaceSelectionPreferred = filesTx->previousWorkspaceSelectionPreferred;
            }
            else if (auto* startup = std::get_if<StartupOpenTx>(&_activeFileTx))
            {
                stagedHeld.entrySource = std::move(startup->entrySource);
            }
            else if (auto* reload = std::get_if<ConflictReloadTx>(&_activeFileTx))
            {
                stagedHeld.conflictReload = std::move(*reload);
            }
            _activeFileTx = std::move(stagedHeld);
            Status(IDS_BLOCKPENDING);
            return;
        }
        if (auto* picker = std::get_if<PickerOpenTx>(&_activeFileTx))
        {
            entrySource   = std::move(picker->entrySource);
            approvals     = std::move(picker->approvals);
            _activeFileTx = std::monostate{};
        }
        else if (auto* filesTx = std::get_if<FilesPaneOpenTx>(&_activeFileTx))
        {
            fromFiles                           = true;
            entrySource                         = std::move(filesTx->entrySource);
            previousWorkspaceSelectionPreferred = filesTx->previousWorkspaceSelectionPreferred;
            _activeFileTx                       = std::monostate{};
        }
        else if (auto* startup = std::get_if<StartupOpenTx>(&_activeFileTx))
        {
            entrySource   = std::move(startup->entrySource);
            _activeFileTx = std::monostate{};
        }
    }
    auto& result                = completed.value();
    const bool automaticStartup = ! result.saving && ! result.reloading && _startupRestore.IsOpeningLastDocument();
    if (automaticStartup)
    {
        _startupRestore.phase = StartupRestoreState::Phase::Completed;
        const auto admitted   = std::exchange(_startupRestore.source, std::nullopt);
        if (! admitted || ! _editor->Snapshot().IsSameRevision(admitted.value()) || ! _recoveryCandidates.empty() || IsSessionEnding())
        {
            _activeFileTx = std::monostate{};
            Status(IDS_FILECONTEXTCHANGED);
            return;
        }
    }
    if (FAILED(result.status))
    {
        if (result.reloading)
            _activeFileTx = std::monostate{};
        const bool failedClose = IsClosingAfterSave();
        if (failedClose)
            ResumeAfterSaveClose();
        MonitorDiagnostics::Write(RedMonitor::Severity::Error,
                                  result.saving ? L"save failed; source and previous target retained" : L"open failed; current document retained");
        if (result.saving && result.status == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && ! failedClose)
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
            message              = result.rejectedUnits ? FormatStringResource(nullptr, IDS_FILETOOLARGECOUNT, result.rejectedUnits, limit)
                                                        : FormatStringResource(nullptr, IDS_FILETOOLARGELIMIT, limit);
        }
        else
        {
            const auto error = ErrorText(result.status);
            message          = FormatStringResource(nullptr, result.saving ? IDS_SAVEFAILED : IDS_OPENFAILED, error);
        }
        if (! _options.headless && ! failedClose && ! automaticStartup)
            (void)DxUi::Failure::InvokeNative(
                &MessageBoxW, _window.get(), message.c_str(), Resource(IDS_ERROR).c_str(), static_cast<UINT>(MB_OK | MB_ICONERROR));
        _feedback->SetText(message);
        _activeFileTx = std::monostate{};
        return;
    }
    if (result.reloading)
    {
        std::optional<ConflictReloadTx> request = std::move(reloadTx);
        if (! request)
        {
            if (auto* reload = std::get_if<ConflictReloadTx>(&_activeFileTx))
                request = std::move(*reload);
        }
        _activeFileTx = std::monostate{};
        if (! IsUiAttached())
            return;
        if (! request || _session.key != request->key || _session.path != request->path || ! _editor->Snapshot().IsSameRevision(request->source))
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
        std::function<void()> continuation;
        if (auto* saveTx = std::get_if<SaveTx>(&_activeFileTx))
            continuation = std::move(saveTx->afterSave);
        _activeFileTx      = std::monostate{};
        _session.recovered = false;
        _session.savedText = std::move(result.document.text);
        _derivedCaches.dirtyFact.reset();
        if (_session.path != result.path)
            _printNetworkImages.clear();
        _session.path  = std::move(result.path);
        _session.stamp = result.document.stamp;
        EnsureFilesRoot();
        RefreshFiles(true);
        if (! AcceptSourceResult(_editor->TrySetDocumentFilePath(_session.path.native())))
            return;
        RememberActiveDocument();
        Changed();
        Status(IDS_SAVED);
        if (IsClosingAfterSave())
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
                if (std::ranges::any_of(_inactiveDocuments, [](const auto& document) { return document.dirty; }))
                {
                    ResumeAfterSaveClose();
                    AskWorkspaceThen([this] { BeginClose(); });
                }
                else
                {
                    TransitionLifecycle(LifecyclePhase::Active);
                    _visibleBeforeSaveClose = false;
                    BeginClose();
                }
            }
            return;
        }
        if (continuation)
            AskThen(std::move(continuation)); // edits typed during a save remain protected
    }
    else
    {
        // User may have typed while a slow open ran. Recheck dirty state before
        // replacing the source; this is a new consent decision, not a stale one.
        auto staged = std::make_shared<AsyncFiles::Result>(std::move(result));
        auto apply  = [this, staged]
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
            const auto previousCount = _inactiveDocuments.size();
            if (StartWorkspaceDocument() && IsUiAttached())
            {
                _workspaceSelectionPreferred = false;
                if (SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp))
                    Status(IDS_SAVED);
                else
                {
                    _workspaceSelectionPreferred = previousWorkspaceSelectionPreferred;
                    if (_inactiveDocuments.size() > previousCount && SwitchWorkspaceDocument(previousCount))
                    {
                        _inactiveDocuments.pop_back(); // Retire only the new empty slot; the prior file/history is active again.
                        if (IsUiAttached())
                            RefreshFiles();
                    }
                }
            }
            else if (IsUiAttached())
                RefreshFiles();
        }
        else if (entrySource && _editor->Snapshot().IsSameRevision(entrySource.value()))
            apply();
        else
            AskWorkspaceThen(std::move(apply), std::move(approvals));
    }
}
std::optional<std::wstring> Application::BeginBitmapPaste(std::span<const std::byte> dib, bool v5)
{
    if (! CanAcceptCommand() || IsSessionEnding() || HasActiveModalPrompt() || ! _editor || _editor->IsReadOnly() || _settings.view == EditorView::Source ||
        _host.GetFocusControl() != _editor || _editor->IsSelectionGestureActive() || HasBlockDraft())
        return std::nullopt;
    PendingBitmapPaste pending;
    pending.documentKey  = _session.key;
    pending.source       = _editor->Snapshot();
    pending.selection    = _editor->GetSelection();
    pending.view         = _settings.view;
    pending.documentPath = _session.path;
    pending.lifetime     = GetControlLifetimeToken(*_editor);
    const auto submitted = _bitmapWriter.Submit(dib, v5, pending.request);
    if (FAILED(submitted))
    {
        Status(submitted == HRESULT_FROM_WIN32(ERROR_BUSY) ? IDS_IMAGEPASTEBUSY : IDS_IMAGEPASTEFAILED);
        return std::nullopt;
    }
    _pendingBitmapPaste = std::move(pending);
    Status(IDS_IMAGEPASTING);
    // The synchronous SDK callback owns no I/O. This application completes its
    // separately guarded source transaction after the durable worker result.
    return std::nullopt;
}
void Application::ServiceBitmapPaste()
{
    BitmapPasteWriter::Result result;
    if (! _bitmapWriter.Take(result))
        return;
    auto pending = std::exchange(_pendingBitmapPaste, std::nullopt);
    bool keep{};
    const auto settle = wil::scope_exit([&]
    {
        if (keep)
            _bitmapWriter.Commit(result.request);
        else if (FAILED(_bitmapWriter.Discard(std::move(result))))
            OutputDebugStringW(L"RedTilde: a declined clipboard asset was retained after a cleanup scheduling error.\n");
    });
    if (IsClosing() || IsSessionEnding())
        return;
    if (FAILED(result.status))
    {
        Status(IDS_IMAGEPASTEFAILED);
        return;
    }
    if (! pending || HasActiveModalPrompt() || pending->request != result.request || pending->lifetime.expired() || pending->documentKey != _session.key ||
        pending->view != _settings.view || pending->documentPath != _session.path || ! _editor || _editor->IsReadOnly() || _host.GetFocusControl() != _editor ||
        _editor->IsSelectionGestureActive() || _editor->GetSelection() != pending->selection ||
        _editor->Snapshot().ChangedRangeFrom(pending->source).kind != DxUi::RichText::SourceChangeKind::Unchanged || HasBlockDraft())
    {
        Status(IDS_IMAGEPASTECANCELLED);
        return;
    }
    const auto lifetime = pending->lifetime;
    const auto alt      = Resource(IDS_PASTEDIMAGE);
    DxUi::RichText::MarkdownCommandRequest request;
    request.command     = DxUi::RichText::MarkdownCommand::InsertImage;
    request.text        = alt;
    request.destination = result.asset.uri;
    keep                = _editor->ExecuteMarkdownCommand(request) == DxUi::RichText::MarkdownCommandStatus::Changed;
    if (! IsClosing() && ! lifetime.expired())
    {
        if (keep)
            Status(IDS_IMAGEPASTED);
        else if (IsMarkdownCommandCapacityExceeded(*_editor, request, pending->selection))
            ReportCapacityLimit();
        else
            Status(IDS_IMAGEPASTEFAILED);
    }
}
void Application::RefreshImageConsent()
{
    if (! _editor || ! _imageConsent || IsClosing())
        return;
    auto* const owner = _root;
    if (! owner || _host.GetRoot() != owner)
        return;
    const auto lifetime = GetControlLifetimeToken(*owner);
    const auto semantic = _editor->GetSemanticDocument();
    const auto current  = [&]() noexcept
    {
        return ! lifetime.expired() && ! IsClosing() && _root == owner && _host.GetRoot() == owner && _editor && _editor->GetSemanticDocument() == semantic &&
               _imageConsentSource == semantic;
    };
    if (semantic != _imageConsentSource || _imageConsentPath != _session.path)
    {
        _imageConsentSource     = semantic;
        _imageConsentPath       = _session.path;
        _imageConsentHasNetwork = false;
        _imageFailureStatus     = S_OK;
        if (semantic)
            for (const auto& element : semantic->Elements())
                if (element.role == DxUi::RichText::SemanticRole::Image)
                {
                    if (ImageReferenceCategoryFor(element) == ImageReferenceCategory::Network || _session.path.native().starts_with(L"\\\\"))
                    {
                        _imageConsentHasNetwork = true;
                        break;
                    }
                }
    }
    const bool enabled = _settings.allowRemoteImages || _session.remoteImagesAllowed;
    unsigned text      = static_cast<unsigned>(! enabled                     ? IDS_IMAGECONSENTBLOCKED
                                               : _settings.allowRemoteImages ? IDS_IMAGECONSENTSETTINGS
                                                                             : IDS_IMAGECONSENTDOCUMENT);
    if (FAILED(_imageFailureStatus))
        text =
            static_cast<unsigned>(_imageFailureStatus == E_ACCESSDENIED                             ? IDS_IMAGEPOLICYFAILED
                                  : _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) ? IDS_IMAGETOOLARGE
                                  : _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_TIMEOUT) || _imageFailureStatus == HRESULT_FROM_WIN32(ERROR_WINHTTP_TIMEOUT)
                                      ? IDS_IMAGETIMEOUT
                                      : IDS_IMAGELOADFAILED);
    if (_imageConsentText->GetText() != ResourceView(text))
        _imageConsentText->SetText(Resource(text));
    if (! current())
        return;
    const unsigned action = _session.remoteImagesAllowed ? IDS_IMAGEBLOCKHERE : IDS_IMAGEALLOWHERE;
    if (_imageConsentAction->GetText() != ResourceView(action))
        _imageConsentAction->SetText(Resource(action));
    if (! current())
        return;
    _imageConsentAction->SetVisible(_imageConsentHasNetwork && ! _settings.allowRemoteImages);
    if (! current())
        return;
    _imageReload->SetEnabled(true);
    if (! current())
        return;
    const bool visible = _imageConsentHasNetwork || FAILED(_imageFailureStatus);
    if (_imageConsent->IsVisible() != visible)
    {
        _imageConsent->SetVisible(visible);
        if (current())
            Layout();
    }
}

void Application::ServiceImages()
{
    if (! _editor || IsClosing() || _imageBindingTransition || _switchingDocument)
        return;
    auto* const primary      = _editor;
    const auto lifetime      = GetControlLifetimeToken(*primary);
    const auto current       = [&]() noexcept { return ! lifetime.expired() && ! IsClosing() && _editor == primary; };
    const auto semantic      = primary->GetSemanticDocument();
    bool watchBindingChanged = _imageWatchDocumentPath != _session.path;
    if (semantic && semantic != _imageWatchSemantic)
    {
        // Decode cancellation ends a job, including successful completion. It
        // does not end the document's ownership of the displayed file. Compare
        // exact possible-file destinations only on immutable publication;
        // non-file image edits must not rearm local I/O and refetch HTTP images.
        const auto before = _imageWatchSemantic ? _imageWatchSemantic->Elements() : std::span<const DxUi::RichText::SemanticElement>{};
        const auto after  = semantic->Elements();
        const auto image  = [](const auto& element)
        { return element.role == DxUi::RichText::SemanticRole::Image && ImageReferenceCategoryFor(element) == ImageReferenceCategory::PossibleFile; };
        auto previous = std::ranges::find_if(before, image);
        auto next     = std::ranges::find_if(after, image);
        while (previous != before.end() && next != after.end() && previous->destination == next->destination && previous->wiki == next->wiki)
        {
            previous = std::find_if(std::next(previous), before.end(), image);
            next     = std::find_if(std::next(next), after.end(), image);
        }
        watchBindingChanged = watchBindingChanged || previous != before.end() || next != after.end();
        _imageWatchSemantic = semantic;
    }
    if (watchBindingChanged)
    {
        _imageWatchDocumentPath = _session.path;
        const bool hadPaths     = ! _watchedImagePaths.empty();
        _watchedImagePaths.clear();
        (void)_imageFiles.SetFiles({});
        if (hadPaths)
        {
            // A removed/retargeted image or new document base retires the old
            // binding. Obtain fresh canonical paths through SDK admission.
            primary->ReloadImages();
            if (! current())
                return;
        }
    }
    if (const auto notice = _imageFiles.TakeResult())
    {
        if (FAILED(notice.value().status))
            _imageFailureStatus = notice.value().status;
        else
        {
            ++_imageFileRefreshes;
            primary->ReloadImages();
            if (! current())
                return;
        }
    }
    RefreshImageConsent();
    if (! current())
        return;
    _editor->TakePendingImageRequests(_imageRequests);
    _images.SetDocumentContext(_session.path.native());
    bool watchChanged{};
    for (const auto& request : _imageRequests)
        if (request.SourceKind() == DxUi::RichText::RichImageSourceKind::LocalFile && ! request.Cancelled() &&
            std::ranges::find(_watchedImagePaths, request.FilePath(), [](const auto& path) -> const std::wstring& { return path.native(); }) ==
                _watchedImagePaths.end() &&
            _watchedImagePaths.size() < 32u)
        {
            _watchedImagePaths.emplace_back(request.FilePath());
            watchChanged = true;
        }
    if (watchChanged)
    {
        ++_imageWatchPathBuilds;
        const auto status = _imageFiles.SetFiles(_watchedImagePaths);
        if (FAILED(status))
        {
            (void)_imageFiles.SetFiles({}); // Retire the old binding when automatic watching cannot be admitted.
            _imageFailureStatus = status;
        }
    }
    _images.DiscardCancelled();
    if (! _imageRequests.empty())
        (void)_images.Submit(_imageRequests);
    if (_images.HasResults())
    {
        std::array<RichImageLoader::Result, RichImageLoader::MaximumResults> results;
        const auto count = _images.Drain(results);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto& result  = results[index];
            const bool accepted = primary->CompleteImageRequest(result.request, result.image);
            if (! current())
                return;
            if (accepted)
                RememberPrintImage(result.request, result.image);
            if (accepted && FAILED(result.status) && result.status != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                _imageFailureStatus = result.status;
        }
        RefreshImageConsent();
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_NCCREATE)
    {
        auto* app = static_cast<Application*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app && message == WM_NCCALCSIZE && wParam)
    {
        auto* size = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
        if (IsZoomed(hwnd))
        {
            const UINT dpi = GetDpiForWindow(hwnd);
            const int edge = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
            InflateRect(&size->rgrc[0], -edge, -edge);
        }
        return 0;
    }
    if (app && app->_lifecyclePhase != LifecyclePhase::Uninitialized)
    {
        LRESULT result{};
        app->RunUiAction([&] { result = app->HandleMessage(hwnd, message, wParam, lParam); });
        return result; // A failed Close stays handled instead of reaching DefWindowProc.
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
void Application::BeginClose()
{
    Printing::Stop(_printOperation);
    ClosePrintPreview();
    // Reaching this point means the user saved or explicitly discarded source.
    // Retire only this session's snapshot and wait asynchronously for its result.
    AbandonStartupRestore();
    RememberActiveDocument();
    RecordWindowPlacement(true);
    CloseExternalLinks();
    ResetWorkspace();
    (void)_recovery.SubmitDocument(_session.key, std::nullopt, true);
    _recovery.Stop();
    TransitionLifecycle(LifecyclePhase::DrainingForClose);
    _pendingBitmapPaste.reset();
    _bitmapWriter.Stop();
    _settingsWatcher.Close();
    _files.Close();
    _folderFiles.Close();
    _images.Shutdown();
    _imageFiles.Close();
    _host.Detach();
    if (! AreShutdownWorkersSettled())
    {
        // Keep only a hidden notification window until admitted preferences are
        // durable. The UI never joins storage, and process exit cannot kill the
        // worker between its final write and atomic replacement.
        ShowWindow(_window.get(), SW_HIDE);
    }
    else
    {
        _recovery.Close();
        _preferenceWriter.Close();
        _bitmapWriter.Detach();
        TransitionLifecycle(LifecyclePhase::Destroyed);
        _window.reset();
    }
}
void Application::ResumeAfterSaveClose()
{
    if (IsClosingAfterSave())
        TransitionLifecycle(LifecyclePhase::Active);
    // Restore the existing placement without interrupting whoever has focus
    // now. Headless harness windows must remain hidden on every failure path.
    if (std::exchange(_visibleBeforeSaveClose, false) && ! _options.headless)
        ShowWindow(_window.get(), SW_SHOWNA);
}
void Application::FinishClose()
{
    if (_lifecyclePhase == LifecyclePhase::DrainingForClose && AreShutdownWorkersSettled())
    {
        _recovery.Close();
        _preferenceWriter.Close();
        _bitmapWriter.Detach();
        TransitionLifecycle(LifecyclePhase::Destroyed);
        _window.reset();
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
        if (! IsClosing())
        {
            AbandonStartupRestore();
            RecordWindowPlacement(true);
            CheckpointWorkspaceRecovery();
        }
        return TRUE; // Honor shutdown immediately; never create a shutdown blocker.
    }
    if (message == WM_ENDSESSION)
    {
        if (wParam != FALSE)
        {
            _sessionEndCommitted = true;
            if (_lifecyclePhase == LifecyclePhase::Active || _lifecyclePhase == LifecyclePhase::Initializing ||
                _lifecyclePhase == LifecyclePhase::ClosingAfterSave)
                TransitionLifecycle(LifecyclePhase::SessionEnding);
            Printing::Stop(_printOperation);
            ClosePrintPreview();
            _pendingBitmapPaste.reset();
            _bitmapWriter.Stop();
            _settingsWatcher.Close();
            if (! IsClosing())
                CheckpointWorkspaceRecovery();
            // Windows may terminate us when this message returns. Only this
            // final system boundary waits; recovery never needs a UI callback.
            _sessionEndRecoveryStatus = _recovery.WaitForSettlement(2000);
            if (FAILED(_sessionEndRecoveryStatus))
                MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Session-end recovery did not settle within its bounded allowance");
            // End the ordinary process loop without the clean-close path that
            // retires dirty copies. A hidden fixture owns only its test window,
            // never the parent harness's message loop (as for WM_DESTROY).
            if (! _options.headless)
                PostQuitMessage(0);
        }
        else
        {
            _sessionEndCommitted = false;
            if (_lifecyclePhase == LifecyclePhase::SessionEnding)
                TransitionLifecycle(LifecyclePhase::Active);
        }
        return 0;
    }
    if (message == WM_CLOSE)
    {
        if (IsClosing() || IsClosingAfterSave() || IsSessionEnding() || HasActiveModalPrompt() || _switchingDocument)
            return 0;
        if (_printPreview)
        {
            DismissPrintPreview();
            return 0;
        }
        if (HasBlockDraft())
        {
            Status(IDS_BLOCKPENDING);
            _host.SetFocusControl(_blockDraftText, MayRequestNativeFocus());
            return 0;
        }
        AbandonStartupRestore();
        if (_files.HasPendingSave())
        {
            // Keep the host and notification HWND alive until the admitted
            // write/replace reports success. Recovery remains available if
            // storage fails or the process ends while the save is pending.
            CheckpointRecovery(true);
            _activeFileTx = std::monostate{};
            TransitionLifecycle(LifecyclePhase::ClosingAfterSave);
            _visibleBeforeSaveClose = IsWindowVisible(_window.get()) != FALSE;
            ShowWindow(_window.get(), SW_HIDE);
            return 0;
        }
        if (_files.Busy())
        {
            // An Open or Reload has no write to settle. Invalidate that worker's
            // generation before the ordinary dirty question; Cancel must also
            // refuse its late result, and Save must be free to admit a new operation.
            _files.CancelInFlightLoad();
        }
        _activeFileTx = std::monostate{};
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
            const auto result        = _host.HandleMessage(hwnd, clientMessage, MK_LBUTTON, clientPoint, handled);
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
            const auto result = _host.HandleMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y)), handled);
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
        if (! _options.headless)
            PostQuitMessage(0);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        Printing::Stop(_printOperation);
        ClosePrintPreview();
        _bitmapWriter.Stop();
        _bitmapWriter.Detach();
        _settingsWatcher.Close();
        _folderFiles.Close();
        _imageFiles.Close();
        _host.Detach();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_GETMINMAXINFO)
    {
        auto* info     = reinterpret_cast<MINMAXINFO*>(lParam);
        const UINT dpi = GetDpiForWindow(hwnd);
        MONITORINFO monitor{sizeof(monitor)};
        const bool available = GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE;
        info->ptMinTrackSize = {available ? std::min(static_cast<LONG>(MulDiv(800, static_cast<int>(dpi), 96)), monitor.rcWork.right - monitor.rcWork.left)
                                          : MulDiv(800, static_cast<int>(dpi), 96),
                                available ? std::min(static_cast<LONG>(MulDiv(560, static_cast<int>(dpi), 96)), monitor.rcWork.bottom - monitor.rcWork.top)
                                          : MulDiv(560, static_cast<int>(dpi), 96)};
        return 0;
    }
    if (message == PreferencesReady)
    {
        if (SUCCEEDED(_preferenceWriter.Status()))
        {
            if (const auto writtenStamp = _preferenceWriter.LastWrittenStamp())
                _settingsFileStamp = *writtenStamp;
        }
        if (_lifecyclePhase == LifecyclePhase::DrainingForClose)
        {
            FinishClose();
        }
        else if (! IsClosing() && ! IsSessionEnding() && FAILED(_preferenceWriter.Status()))
            Status(IDS_SETTINGSFAILED);
        return 0;
    }
    if (message == RecoveryReadyMessage)
    {
        RecoveryCompleted();
        return 0;
    }
    if (IsClosing())
        return DefWindowProcW(hwnd, message, wParam, lParam);
    if (message == PrintReady)
    {
        PrintCompleted();
        return 0;
    }
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
    if (message == SettingsChanged)
    {
        _settingsWatcher.Acknowledge();
        if (! IsClosing() && ! IsClosingAfterSave() && ! IsSessionEnding())
            ReloadSettingsFromDisk();
        return 0;
    }
    if (IsClosingAfterSave())
    {
        const bool nativeLifecycle = message == WM_SHOWWINDOW || message == WM_KILLFOCUS || message == WM_ACTIVATE || message == WM_ACTIVATEAPP ||
                                     message == WM_CANCELMODE || message == WM_CAPTURECHANGED || message == WM_SIZE || message == WM_DPICHANGED;
        if (! nativeLifecycle)
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_DROPFILES)
    {
        const auto drop   = reinterpret_cast<HDROP>(wParam);
        const auto finish = wil::scope_exit([&] { DragFinish(drop); });
        if (! CanAcceptCommand() || HasActiveModalPrompt() || HasBlockDraft() || _files.Busy())
            return 0;
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFFu, nullptr, 0);
        if (count == 0u)
            return 0;
        if (count > 1u)
        {
            Status(IDS_DROPMULTIPLE);
            return 0;
        }
        const UINT length = DragQueryFileW(drop, 0u, nullptr, 0);
        if (length == 0u || length >= 32767u)
        {
            Status(IDS_DROPUNSUPPORTED);
            return 0;
        }
        std::wstring buffer(static_cast<std::size_t>(length) + 1u, L'\0');
        const UINT copied = DragQueryFileW(drop, 0u, buffer.data(), static_cast<UINT>(buffer.size()));
        if (copied == 0u || copied > length)
        {
            Status(IDS_DROPUNSUPPORTED);
            return 0;
        }
        buffer.resize(copied);
        (void)OpenDroppedFile(std::filesystem::path(std::move(buffer)));
        return 0;
    }
    if (message == ImageReady || message == ImageFileChanged)
    {
        if (wParam == 0x5254)
            ServiceImages();
        return 0;
    }
    if (message == PostPreparePlacement)
    {
        _postPreparePosted = false;
        (void)ReconcilePostPreparePlacement();
        return 0;
    }
    if (message == WM_SETTINGCHANGE || message == WM_THEMECHANGED)
        ApplyTheme();
    if (message == WM_MOUSEWHEEL)
    {
        if ((GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0 && ! KeyboardState::Held(VK_MENU))
        {
            const int accumulated = _zoomWheelRemainder + GET_WHEEL_DELTA_WPARAM(wParam);
            const int steps       = accumulated / WHEEL_DELTA;
            _zoomWheelRemainder   = accumulated % WHEEL_DELTA;
            if (steps != 0)
                RunUiAction([&] { ExecuteCommand(steps > 0 ? Command::ZoomIn : Command::ZoomOut, static_cast<unsigned>(steps > 0 ? steps : -steps)); });
            return 0;
        }
        _zoomWheelRemainder = 0;
    }
    bool handled{};
    const auto result = _host.HandleMessage(hwnd, message, wParam, lParam, handled);
    if (message == WM_MOVE || message == WM_SIZE || message == WM_EXITSIZEMOVE || message == WM_DPICHANGED)
    {
        const bool wasMaximized    = _settings.windowPlacement && _settings.windowPlacement.value().maximized;
        const bool commitPlacement = message == WM_EXITSIZEMOVE || message == WM_DPICHANGED || (! IsIconic(hwnd) && wasMaximized != (IsZoomed(hwnd) != FALSE));
        RecordWindowPlacement(commitPlacement);
    }
    if (message == WM_SIZE || message == WM_DPICHANGED || ((message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) && HasBlockDraft()))
        Layout();
    if (message == WM_PAINT)
    {
        ServiceImages();
        // Published syntax or deferred placement can arrive after a committed source/selection event.
        // Observe immutable identity and defer overlay/layout reconciliation out of WM_PAINT.
        const bool placementPending = ! IsClosing() && (_pendingSelectionTools || (_blockDraftSession && _blockDraftSession->pendingPlacement) ||
                                                        _editor->GetMarkdownSyntax() != _derivedCaches.selectionFactsSyntax);
        if (placementPending && ! _postPreparePosted)
            _postPreparePosted = PostMessageW(hwnd, PostPreparePlacement, 0, 0) != FALSE;
    }
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}

bool Application::ReconcilePostPreparePlacement()
{
    if (IsClosing() || ! _host.GetRoot())
        return false;
    const bool refreshFacts     = _pendingSelectionTools || _editor->GetMarkdownSyntax() != _derivedCaches.selectionFactsSyntax;
    const bool pendingPlacement = _blockDraftSession && _blockDraftSession->pendingPlacement;
    if (refreshFacts)
        SelectionChanged(_pendingSelectionTools || _activeOverlay == OverlayKind::FloatingSelectionTools);
    if (pendingPlacement)
        Layout();
    return refreshFacts || pendingPlacement;
}

void Application::Prepare(unsigned turns)
{
    if (IsClosing() || ! _host.GetRoot())
        return;
    const auto cosmeticsPending = [](const DxUi::RichTextEditor* target)
    {
        if (! target || ! target->IsVisible())
            return false;
        const auto state = target->DebugGetState();
        return state.markdownPreparationPending || state.sourceInkPreparationPending || state.codeInkPreparationPending || state.sourceGuidesPreparationPending;
    };
    for (unsigned index = 0; index < turns; ++index)
    {
        if (_bitmapWriter.HasResult())
            ServiceBitmapPaste();
        if (IsClosing() || ! _host.GetRoot())
            return;
        (void)_host.RenderInitialFrameForShow();
        const bool identityRelayout = _identityMeasure.typographyRevision != _host.GetTypographyRevision();
        if (identityRelayout)
            Layout();
        ServiceImages();
        MSG message{};
        unsigned processed{};
        while (processed++ < 64 && PeekMessageW(&message, _window.get(), 0, 0, PM_REMOVE))
        {
            if (! _host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (IsClosing() || ! _host.GetRoot())
            return;
        if (_files.Busy())
            FileCompleted(); // also drains a result if its post met a full queue
        RecoveryCompleted();
        FolderCompleted();
        if (IsClosing() || ! _host.GetRoot())
            return;
        // A render may publish new immutable syntax. Reconcile dependent chrome
        // before declaring quiescence, then paint it on the next turn. Hidden
        // hosts deliberately do not receive invalidation-driven WM_PAINT.
        const bool refreshed = ReconcilePostPreparePlacement() || identityRelayout;
        // This bounded harness drain also settles optional pixels for captures.
        // Production input/frame readiness remains the host's earlier content
        // boundary; its event loop never calls this synchronous convenience loop.
        if (! refreshed && ! _host.HasPendingContentPreparation() && ! cosmeticsPending(_editor) && ! cosmeticsPending(_mirror) &&
            _editor->GetImageResourceStatistics().pendingSources == 0 && ! _files.Busy() && _recovery.Ready() &&
            (! _folderFiles.IsActive() || _folderSnapshot || FAILED(_folderFiles.Status()) || FAILED(_folderAdmissionStatus)))
            break;
        // Harness-only bounded event wait; production blocks in GetMessage.
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}
bool Application::Capture(const std::filesystem::path& path, bool prepareContent)
{
    if (prepareContent && ! _printPreview)
        Prepare();
    DxUi::Button* captureHover{};
    if (_options.headless && path == _options.capture && (_options.scenario == L"caption-close-hover" || _options.scenario == L"caption-maximize-hover"))
    {
        const bool close  = _options.scenario == L"caption-close-hover";
        captureHover      = close ? _closeButton : _maximizeButton;
        const auto bounds = captureHover->GetBounds();
        const auto point  = _host.DipPointToScreenPoint({(bounds.left + bounds.right) * .5f, (bounds.top + bounds.bottom) * .5f});
        bool handled{};
        // Preparation pumps queued leave messages for this hidden window.
        // Apply owned synthetic hover after that pump, immediately before the
        // frame readback, without moving the pointer or taking native input.
        (void)_host.HandleMessage(
            _window.get(), WM_NCMOUSEMOVE, close ? HTCLOSE : HTMAXBUTTON, MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y)), handled);
        if (handled || ! captureHover->IsHovered())
            return false;
    }
    DxUi::WindowHostBitmapCapture bitmap;
    // Stage the completed frame before Present rotates the swap-chain buffers.
    // The canonical hidden capture path never shows or activates this HWND.
    auto& captureHost = _printPreview ? _printPreview->Host() : _host;
    if (! captureHost.DebugCaptureBitmap(bitmap, true))
        return false;
    if (captureHover && ! captureHover->IsHovered())
        return false;
    wil::com_ptr<IWICImagingFactory> factory;
    wil::com_ptr<IWICStream> stream;
    wil::com_ptr<IWICBitmapEncoder> encoder;
    wil::com_ptr<IWICBitmapFrameEncode> frame;
    wil::com_ptr<IPropertyBag2> properties;
    if (FAILED(DxUi::Failure::InvokeNative(&CoCreateInstance,
                                           CLSID_WICImagingFactory,
                                           nullptr,
                                           static_cast<DWORD>(CLSCTX_INPROC_SERVER),
                                           __uuidof(IWICImagingFactory),
                                           reinterpret_cast<void**>(factory.put()))))
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
    if (FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::SetPixelFormat, &format)) || ! IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA))
        return false;
    if (FAILED(InvokeNativeMethod(frame.get(),
                                  &IWICBitmapFrameEncode::WritePixels,
                                  bitmap.heightPx,
                                  bitmap.widthPx * 4u,
                                  static_cast<UINT>(bitmap.bgraPixels.size()),
                                  bitmap.bgraPixels.data())))
        return false;
    return SUCCEEDED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::Commit)) &&
           SUCCEEDED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::Commit));
}

bool Application::RouteShortcut(const MSG& message)
{
    bool shortcut{};
    if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) && (message.hwnd == _window.get() || IsChild(_window.get(), message.hwnd)))
    {
        const bool ctrl = KeyboardState::Held(VK_CONTROL), shift = KeyboardState::Held(VK_SHIFT), alt = KeyboardState::Held(VK_MENU);
        std::optional<Command> command;
        if (ctrl && ! alt)
        {
            switch (message.wParam)
            {
                case 'N': command = Command::New; break;
                case 'O': command = shift ? Command::OpenFolder : Command::Open; break;
                case 'S': command = shift ? Command::SaveAs : Command::Save; break;
                case 'W':
                    if (! shift)
                        command = Command::CloseDocument;
                    break;
                case 'P':
                    if (! shift)
                    {
                        // A held shortcut must not open another preview or advance into printing.
                        if ((message.lParam & (static_cast<LPARAM>(1) << 30)) != 0)
                            return true;
                        command = Command::Print;
                    }
                    break;
                case 'E': command = shift ? Command::Files : Command::Code; break;
                case 'K': command = Command::Link; break;
                case 'F': command = Command::Find; break;
                case '1': command = Command::Compose; break;
                case '2': command = Command::Source; break;
                case '3': command = Command::Split; break;
                case VK_OEM_COMMA: command = Command::Settings; break;
                case VK_OEM_PLUS:
                case VK_ADD: command = Command::ZoomIn; break;
                case VK_OEM_MINUS:
                case VK_SUBTRACT: command = Command::ZoomOut; break;
                case '0':
                case VK_NUMPAD0: command = Command::ZoomReset; break;
            }
        }
        if (message.wParam == VK_F3 && ! ctrl && ! alt)
            command = shift ? Command::FindPrevious : Command::FindNext;
        if (message.wParam == VK_F2 && ! ctrl && ! shift && ! alt)
            command = Command::LocalSource;
        if (message.wParam == VK_F11 && ! ctrl && ! shift && ! alt)
            command = Command::Focus;
        if (message.wParam == VK_F10 && alt && ! ctrl && ! shift)
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
    if (! _options.designInteractive.empty())
    {
#if defined(REDTILDE_ENABLE_TESTS)
        return RunDesignInteractiveProbe(*this, _options);
#else
        return 2;
#endif
    }
    if (! _options.performance.empty())
        return RunPerformanceProbe(*this, _options);
    if (_options.recoveryProbe)
    {
        // Isolated harness fixtures: exercise the real Files worker, independent
        // histories and checkpoint leases before the intentional child crash.
        const auto directory = _options.dataDirectory / L"Workspace";
        std::filesystem::create_directories(directory);
        const auto alpha = directory / L"Alpha.md", beta = directory / L"Beta.md";
        if (FAILED(WriteBytesAtomic(alpha, "\xef\xbb\xbf# Alpha\r\n")) || FAILED(WriteBytesAtomic(beta, "# Beta\n")))
            return 6;
        StoredDocument loaded;
        if (FAILED(LoadDocument(alpha, loaded)))
            return 6;
        SetDocument(loaded.text, alpha, loaded.bom, loaded.stamp);
        _editor->SetSelection({0, 0});
        (void)_editor->ReplaceSelection(L"Recovered after crash: 東京 😀\r\n");
        OpenPath(beta);
        for (unsigned turn = 0; turn < 1000 && _session.path != beta; ++turn)
        {
            Prepare(32);
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        }
        if (_session.path != beta || _inactiveDocuments.size() != 1)
            return 6;
        _editor->SetSelection({0, 0});
        (void)_editor->ReplaceSelection(L"A second unsaved copy: 世界\n");
        CheckpointWorkspaceRecovery();
        for (unsigned turn = 0; turn < 1000 && ! _recovery.Settled(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        if (! _recovery.Settled() || FAILED(_recovery.Status()))
            return 6;
        CrashProbe();
    }
    if (_options.recoveryRestartProbe)
    {
        Prepare();
        RecoveryCompleted();
        if (_recoveryCandidates.size() != 2)
            return 7;
        Execute(Command::RestoreAllRecovery);
        const auto expected = [&](const std::filesystem::path& path, std::wstring_view text, bool hasBom)
        {
            return path.filename() == (hasBom ? L"Alpha.md" : L"Beta.md") &&
                   text == (hasBom ? L"Recovered after crash: 東京 😀\r\n# Alpha\r\n" : L"A second unsaved copy: 世界\n# Beta\n");
        };
        bool correct = Dirty() && _session.recovered && _inactiveDocuments.size() == 1 && expected(_session.path, _editor->GetText(), _session.bom);
        for (const auto& document : _inactiveDocuments)
            correct =
                correct && document.dirty && document.metadata.recovered && expected(document.metadata.path, document.state.Text(), document.metadata.bom);
        StoredDocument original;
        correct =
            correct && SUCCEEDED(LoadDocument(_options.dataDirectory / L"Workspace/Alpha.md", original)) && original.text == L"# Alpha\r\n" && original.bom;
        correct = correct && SUCCEEDED(LoadDocument(_options.dataDirectory / L"Workspace/Beta.md", original)) && original.text == L"# Beta\n" && ! original.bom;
        CheckpointWorkspaceRecovery();
        _recovery.Stop();
        for (unsigned turn = 0; turn < 1000 && ! _recovery.Finished(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        return correct && _recovery.Finished() && SUCCEEDED(_recovery.Status()) ? 0 : 8;
    }
    if (_options.preferencesProbe)
    {
        Execute(Command::ThemeLight);
        Execute(Command::ThemeDark);
        Execute(Command::ThemePaper);
        (void)SendMessageW(_window.get(), WM_CLOSE, 0, 0);
        if (_window)
            (void)SendMessageW(_window.get(), WM_CLOSE, 0, 0);
        MSG message{};
        while (_window && GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            if (! _host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        return 0;
    }
    if (_options.headless)
    {
        _settings.appearance = _options.scenario == L"sample-document-dark" || _options.scenario == L"task-presentation-dark" ||
                                       _options.scenario == L"compose-dark" || _options.scenario == L"split-dark" || _options.scenario == L"math-dark" ||
                                       _options.scenario == L"quote-scroll-dark" || _options.scenario == L"diagram-label-dark" ||
                                       _options.scenario == L"reference-dark" || _options.scenario == L"markdown-style" ||
                                       _options.scenario == L"caption-close-hover" || _options.scenario == L"caption-maximize-hover"
                                   ? Appearance::Dark
                               : _options.scenario == L"compose-paper" ? Appearance::Paper
                                                                       : Appearance::Light;
        ApplyTheme();
        if (_options.scenario == L"math" || _options.scenario == L"math-dark")
            SetDocument(L"# Mathematics, in the flow\n\nInline $E=mc^2$ shares the text baseline. Exact TeX stays one F2 away.\n\n"
                        L"$$\nx=\\frac{-b+\\sqrt{b^2-4ac}}{2a}\n$$\n\n"
                        L"$$\nA=\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\\quad \\vec{v}=\\begin{pmatrix}x\\\\y\\end{pmatrix}\n$$\n\n"
                        L"$$\n\\begin{aligned}f(x)&=x^2+1\\\\f'(x)&=2x\\end{aligned}\n$$\n\n"
                        L"Unsupported commands keep their exact source: $\\input{private}$.\n");
        if (_options.scenario == L"mermaid")
            SetDocument(L"# Diagrams, in the flow\n\n## Flowchart\n\n```mermaid\nflowchart LR\nDraft[Draft] --> Review[Review]\n"
                        L"Review --> Saved[Saved]\n```\n\n"
                        L"## Sequence diagram\n\n```mermaid\nsequenceDiagram\nparticipant Writer\nparticipant Notebook\n"
                        L"Writer->>Notebook: Save draft\nNotebook-->>Writer: Saved\n```\n\n"
                        L"## State diagram\n\n```mermaid\nstateDiagram-v2\ndirection LR\n[*] --> Draft\nDraft --> Reviewed : review\n"
                        L"Reviewed --> [*]\n```\n");
        if (_options.scenario == L"recovery")
        {
            _recoveryCandidates.push_back({L"0123456789abcdef0123456789abcdef",
                                           {L"# Your writing is here\r\n\r\nThe unsaved paragraph, including 東京 and 😀, is ready to restore.\r\n",
                                            L"C:\\Notes\\My next idea.md",
                                            true,
                                            {0, 0},
                                            12}});
            ShowRecovery();
        }
        if (_options.scenario == L"source")
            SetView(EditorView::Source);
        if (_options.scenario == L"split" || _options.scenario == L"split-dark")
            SetView(EditorView::Split);
        if (_options.scenario == L"settings")
            Execute(Command::Settings);
        Prepare();
        if (! _options.benchmark.empty())
        {
            using Clock               = std::chrono::steady_clock;
            constexpr unsigned frames = 60;
            const auto start          = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
                (void)_host.RenderInitialFrameForShow();
            const double cleanSeconds = std::chrono::duration<double>(Clock::now() - start).count();
            const auto dirtyStart     = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
            {
                _editor->SetSelection({0, 0});
                (void)_editor->ReplaceSelection(L"x");
                (void)_host.RenderInitialFrameForShow();
                (void)_editor->Undo();
                (void)_host.RenderInitialFrameForShow();
            }
            const double dirtySeconds = std::chrono::duration<double>(Clock::now() - dirtyStart).count();
            PROCESS_MEMORY_COUNTERS_EX memory{};
            (void)GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory));
            std::ofstream report(_options.benchmark, std::ios::binary);
            report << "{\n  \"scenario\":\"sample-richtext-native-hidden\",\n  \"cleanFrames\":" << frames
                   << ",\n  \"cleanRenderFps\":" << frames / cleanSeconds << ",\n  \"dirtyFrames\":" << frames * 2
                   << ",\n  \"dirtyEditAndRenderMs\":" << dirtySeconds * 1000 / (frames * 2) << ",\n  \"workingSetBytes\":" << memory.WorkingSetSize
                   << ",\n  \"privateBytes\":" << memory.PrivateUsage << ",\n  \"dpi\":" << _host.GetDpi() << ",\n  \"widthPx\":" << _host.DebugGetWidthPx()
                   << ",\n  \"heightPx\":" << _host.DebugGetHeightPx()
                   << ",\n  \"qualification\":\"initial application baseline only; forced native renders, not presentation FPS or a paired "
                      "non-regression claim\"\n}\n";
            if (! report)
                return 4;
        }
        if (_options.scenario == L"context-selection")
        {
            const auto text  = _editor->GetText();
            const auto begin = text.find(L"Good writing");
            if (begin != text.npos)
            {
                _editor->SetSelection({begin, begin + 12});
                SelectionChanged();
            }
        }
        if (_options.scenario == L"local-source")
        {
            const auto begin = _editor->GetText().find(L"Good writing");
            if (begin != std::wstring::npos)
            {
                _editor->SetSelection({begin, begin + 12});
                Execute(Command::LocalSource);
            }
        }
        if (_options.scenario == L"math" || _options.scenario == L"math-dark")
        {
            const auto begin = _editor->GetText().find(L"\\frac");
            if (begin != std::wstring::npos)
            {
                _editor->SetSelection({begin, begin});
                SelectionChanged();
            }
        }
        PrepareDesignScenario();
        return _options.capture.empty() || Capture(_options.capture) ? 0 : 3;
    }
    if (_startupRestore.IsWaitingForRecovery())
        RecoveryCompleted(); // Also settle a scan that completed before the ordinary message loop started.
    (void)_host.PrimeForShow();
    if (_identityMeasure.typographyRevision != _host.GetTypographyRevision())
    {
        Layout();
        (void)_host.PrimeForShow();
    }
    ShowWindow(_window.get(), InitialShowCommand());
    UpdateWindow(_window.get());
    RecordWindowPlacement(true);
    _host.SetFocusControl(_editor, MayRequestNativeFocus());
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        DispatchUiMessage(message);
    }
    return static_cast<int>(message.wParam);
}
void Application::DispatchUiMessage(const MSG& message) noexcept
{
    RunUiAction([&]
    {
        if (IsClosingAfterSave())
            (void)DxUi::Failure::InvokeNative(&DispatchMessageW, &message);
        else if (! (_printPreview && _printPreview->DispatchKeyboardMessage(message)) && ! RouteShortcut(message) && ! _host.DispatchKeyboardMessage(message))
        {
            (void)DxUi::Failure::InvokeNative(&TranslateMessage, &message);
            (void)DxUi::Failure::InvokeNative(&DispatchMessageW, &message);
        }
        if (_lifecyclePhase == LifecyclePhase::DrainingForClose && AreShutdownWorkersSettled())
        {
#if defined(REDTILDE_ENABLE_TESTS)
            ++_preferencesReadySentDuringClose;
#endif
            (void)SendMessageW(_window.get(), PreferencesReady, 0, 0);
        }
        if (! IsClosing() && _files.Busy())
            FileCompleted();
    });
}
} // namespace RedTilde
