#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "Common/TextUtilities.h"
#include "KeyboardState.h"
#include "PrintPreview.h"
#include "res/Resource.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <DxUi/TextInputServices.h>
#include <algorithm>
#include <dwmapi.h>
#include <format>
#include <mutex>
#include <new>
#include <shellapi.h>
#include <type_traits>
#include <wil/result.h>
#include <windowsx.h>

namespace RedTilde
{
static_assert(std::is_nothrow_swappable_v<DocumentInfo>, "analysis facts must publish without throwing");
namespace
{
class OfflineCaptureGitHubTransport final : public IGitHubTransport
{
public:
    HRESULT SendRequest(const GitHubHttpRequest&, const std::atomic_bool*, HANDLE, GitHubHttpResponse& outResponse) noexcept override
    {
        outResponse                 = {};
        outResponse.transportStatus = HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED);
        return outResponse.transportStatus;
    }
};
// App-local display categories over the canonical immutable tree, not a parser.
unsigned CurrentBlockResourceAt(const DxUi::RichText::MarkdownSyntax& syntax,
                                std::span<const std::uint32_t> topLevelBlocks,
                                std::size_t caret,
                                std::optional<std::pair<unsigned, std::size_t>>* listSummary = nullptr) noexcept
{
    using namespace DxUi::RichText;
    const auto source = syntax.Source();
    const auto& nodes = syntax.Nodes();
    unsigned resource = IDS_BLOCKPARAGRAPH;
    if (nodes.empty() || source.empty() || topLevelBlocks.empty())
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
    // Published child/sibling links are acyclic and source-ordered. Jump to the
    // containing top-level sibling via binary search, then descend its path.
    const auto it = std::upper_bound(topLevelBlocks.begin(), topLevelBlocks.end(), position, [&](std::size_t pos, std::uint32_t idx) noexcept {
        return pos < nodes[idx].block.source.first;
    });
    if (it == topLevelBlocks.begin())
        return resource;
    std::size_t index = *std::prev(it);
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
} // namespace

bool Application::IsMarkdownCommandCapacityExceeded(const Editor& editor,
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
#if defined(ENABLE_TESTS)
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
#if defined(ENABLE_TESTS)
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
    _spellService.Stop();
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
    _multiFolderFiles.Close();
    _images.Shutdown();
    _imageFiles.Close();
    _host.Detach();
    _window.reset();
}

HRESULT Application::Initialize()
{
    if (_options.dataDirectory.empty())
        _options.dataDirectory = DefaultDataDirectory();
    if (_startupRestore.enabled || ! _options.headless
#if defined(ENABLE_TESTS)
        || _options.preferencesProbe || _options.watchSettingsForTest
#endif
    )
    {
        SettingsLoadResult loaded = LoadSettingsCold(_options.dataDirectory);
        _settings                 = std::move(loaded.settings);
        if (loaded.status == SettingsLoadStatus::Loaded || loaded.status == SettingsLoadStatus::Missing || loaded.status == SettingsLoadStatus::Migrated ||
            loaded.status == SettingsLoadStatus::Invalid)
        {
            _settingsFileStamp = loaded.stamp;
        }
        if (loaded.status == SettingsLoadStatus::Invalid || loaded.status == SettingsLoadStatus::Unreadable ||
            loaded.status == SettingsLoadStatus::Incompatible)
            MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"RedTilde could not use the settings file on startup. Defaults applied.");
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
        const auto priorPlacement = _settings.windowPlacement;
        const bool maximized      = priorPlacement && priorPlacement.value().maximized;
        const WindowPlacementSettings resolved{bounds, actualDpi, maximized};
        _settings.windowPlacement = resolved;
        _windowPlacementDirty = ! priorPlacement || ! EqualRect(&priorPlacement->normalBounds, &resolved.normalBounds) || priorPlacement->dpi != resolved.dpi ||
                                priorPlacement->maximized != resolved.maximized;
    }
    if (! _host.Attach(_window.get(), {.forceWarp = _options.forceWarp, .batchInitialRootChildren = true}))
    {
        const DWORD error = GetLastError();
        return error ? HRESULT_FROM_WIN32(error) : E_FAIL;
    }
    _files.Initialize(_window.get(), FileReady);
#if defined(ENABLE_TESTS)
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
    const bool captureMode = ! _options.capture.empty();
    const std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)> ghImporter =
        captureMode ? std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)>(
                          [](std::wstring& outToken, const std::atomic_bool*, HANDLE) noexcept
    {
        ScrubWideString(outToken);
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    })
#if defined(ENABLE_TESTS)
                    : (_options.ghCliImporterForTest ? std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)>(
                                                           [importer = _options.ghCliImporterForTest](std::wstring& outToken, const std::atomic_bool*, HANDLE)
    {
        ScrubWideString(outToken);
        auto token = importer();
        if (! token || token->empty())
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        outToken.assign(*token);
        ScrubWideString(*token);
        return S_OK;
    })
                                                     : std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)>{});
#else
                    : std::function<HRESULT(std::wstring&, const std::atomic_bool*, HANDLE)>{};
#endif
#if defined(ENABLE_TESTS)
    const std::string deviceClientId =
        _options.githubDeviceClientIdForTest.empty() ? std::string(DefaultGitHubDeviceClientId) : _options.githubDeviceClientIdForTest;
    _shellReveal = _options.shellRevealForTest ? _options.shellRevealForTest : std::make_shared<WindowsShellReveal>();
    _credentials = _options.credentialStoreForTest ? _options.credentialStoreForTest
                                                   : (captureMode ? std::static_pointer_cast<ICredentialStore>(std::make_shared<MemoryCredentialStore>())
                                                                  : std::static_pointer_cast<ICredentialStore>(std::make_shared<WindowsCredentialStore>()));
    RETURN_IF_FAILED(_fileOps.Initialize(
        _window.get(), FileOpReady, _shellReveal, _options.fileOpsForTest ? _options.fileOpsForTest : std::make_shared<WindowsWorkspaceFileOps>()));
    RETURN_IF_FAILED(_github.Initialize(_window.get(),
                                        GitHubReady,
                                        _options.githubTransportForTest
                                            ? _options.githubTransportForTest
                                            : (captureMode ? std::static_pointer_cast<IGitHubTransport>(std::make_shared<OfflineCaptureGitHubTransport>())
                                                           : std::static_pointer_cast<IGitHubTransport>(std::make_shared<WinHttpGitHubTransport>())),
                                        _credentials,
                                        deviceClientId,
                                        ghImporter));
#else
    _shellReveal = std::make_shared<WindowsShellReveal>();
    _credentials = captureMode ? std::static_pointer_cast<ICredentialStore>(std::make_shared<MemoryCredentialStore>())
                               : std::static_pointer_cast<ICredentialStore>(std::make_shared<WindowsCredentialStore>());
    RETURN_IF_FAILED(_fileOps.Initialize(_window.get(), FileOpReady, _shellReveal, std::make_shared<WindowsWorkspaceFileOps>()));
    RETURN_IF_FAILED(_github.Initialize(_window.get(),
                                        GitHubReady,
                                        captureMode ? std::static_pointer_cast<IGitHubTransport>(std::make_shared<OfflineCaptureGitHubTransport>())
                                                    : std::static_pointer_cast<IGitHubTransport>(std::make_shared<WinHttpGitHubTransport>()),
                                        _credentials,
                                        std::string(DefaultGitHubDeviceClientId),
                                        ghImporter));
#endif
    _redSalamanderInstalledPath = _shellReveal->FindRedSalamander(_settings.redSalamanderPath);
    for (const auto& repoRef : _settings.githubRepos)
    {
        GitHubConnectedRepoState state;
        state.ref = repoRef;
        _connectedGitHubRepos.push_back(std::move(state));
        _collapsedGitHubRepos.insert(repoRef.owner + L"/" + repoRef.repo);
    }
    _host.SetClipboardPreference(_settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
    TransitionLifecycle(LifecyclePhase::Initializing);
    // BuildUi is the sole creation site for the label whose localized facts we cache.
    _currentBlockLatestFacts.reset();
    _currentBlockPublishedFacts.Invalidate();
    RETURN_IF_FAILED(BuildUi());
    InitializeSpelling();
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
    const bool shouldWatchSettings = (! _options.headless
#if defined(ENABLE_TESTS)
                                      && ! _options.preferencesProbe
#endif
                                      )
#if defined(ENABLE_TESTS)
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
    constexpr std::size_t MaximumExternalLinkUnits = 2048u;
    if (destination.empty() || destination.size() > MaximumExternalLinkUnits || ! DxUi::RichText::IsValidUtf16(destination))
        return false;
    for (std::size_t i = 0; i < destination.size();)
    {
        const wchar_t unit = destination[i];
        if (unit <= L' ' || unit == 0x7f || unit == L'"')
            return false;
        std::uint32_t cp = static_cast<std::uint32_t>(unit);
        if (unit >= 0xD800u && unit <= 0xDBFFu && i + 1u < destination.size())
        {
            const wchar_t low = destination[i + 1u];
            cp                = 0x10000u + ((static_cast<std::uint32_t>(unit - 0xD800u) << 10) | static_cast<std::uint32_t>(low - 0xDC00u));
            i += 2u;
        }
        else
        {
            ++i;
        }
        if (IsUnsafeDisplayCodePoint(cp))
            return false;
    }
    if (StartsWithAsciiOrdinalIgnoreCase(destination, L"mailto:"))
    {
        const std::size_t recipientEnd = destination.find_first_of(L"?#", 7u);
        const auto recipient           = recipientEnd == std::wstring_view::npos ? destination.substr(7u) : destination.substr(7u, recipientEnd - 7u);
        return ! recipient.empty() && destination.find(L'\\', 7u) == std::wstring_view::npos;
    }
    const std::size_t authority = StartsWithAsciiOrdinalIgnoreCase(destination, L"https://")  ? 8u
                                  : StartsWithAsciiOrdinalIgnoreCase(destination, L"http://") ? 7u
                                                                                              : 0u;
    if (authority == 0u || destination.size() <= authority)
        return false;
    const std::size_t hostEnd = destination.find_first_of(L"/?#", authority);
    if (hostEnd == authority)
        return false;
    const auto authoritySpan = hostEnd == std::wstring_view::npos ? destination.substr(authority) : destination.substr(authority, hostEnd - authority);
    return authoritySpan.find(L'\\') == std::wstring_view::npos;
}

std::optional<std::size_t> Application::CurrentLinkElement(const std::shared_ptr<const DxUi::RichText::SemanticDocument>& document) const
{
    auto* const activeEditor = ActiveEditor();
    if (! document || ! activeEditor || ! activeEditor->Snapshot().MatchesText(document->Syntax()->Source()))
        return std::nullopt;
    const auto selection = activeEditor->GetSelection();
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
    if (! CanAcceptCommand() || IsSessionEnding() || HasActiveModalUi() || activation.wiki || ! SupportedExternalLink(activation.destination))
    {
        Status(IDS_LINKUNAVAILABLE);
        return;
    }
    auto* const primary = ActiveEditor();
    const auto semantic = primary->GetSemanticDocument();
    const auto source   = primary->Snapshot();
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
        if (_externalLink->phase == ExternalLinkState::Phase::LaunchingSta || _externalLink->phase == ExternalLinkState::Phase::CompletedPendingDrain)
        {
            // No UI callback or status publication while holding a worker lock.
            return;
        }
    }
    std::wstring destination(activation.destination); // Own every byte before the modal/reentrant boundary.
    const auto selection = primary->GetSelection();
    const auto key       = _session.key;
    auto* const owner    = _root;
    const auto lifetime  = GetControlLifetimeToken(*primary);
    bool approved{};
    _modalPrompt      = ModalPromptKind::ExternalLink;
    const auto unwind = wil::scope_exit([&] { _modalPrompt = ModalPromptKind::None; });
#if defined(ENABLE_TESTS)
    const auto testLaunch = _options.launchLinkForTest;
#endif
    if (_options.headless)
    {
#if defined(ENABLE_TESTS)
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
        const auto message = FormatStringResource(nullptr, IDS_OPENLINKCONFIRM, SanitizeUiText(destination, 512u));
        approved =
            DxUi::Failure::InvokeNative(
                &MessageBoxW, _window.get(), message.c_str(), Resource(IDS_OPENLINK).c_str(), static_cast<UINT>(MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)) ==
            IDYES;
    }
    if (! approved)
        return;
    if (IsClosing() || lifetime.expired() || _host.GetRoot() != owner || ActiveEditor() != primary)
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
#if defined(ENABLE_TESTS)
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
        if (_externalLink->phase != ExternalLinkState::Phase::Idle || ! _externalLink->window)
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
#if defined(ENABLE_TESTS)
    const auto settled = wil::scope_exit([&]
    {
        if (job->gate)
            (void)SetEvent(job->gate->finished.get());
    });
#endif
    HRESULT result = E_FAIL;
    try
    {
#if defined(ENABLE_TESTS)
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
#if defined(ENABLE_TESTS)
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
DxUi::Toggle* Application::MakeSettingToggle(DxUi::Panel& parent, unsigned label, Command command, std::wstring_view tooltip)
{
    auto* toggle = parent.AddChild<DxUi::Toggle>(Resource(label));
    toggle->SetAccessibleName(Resource(label));
    if (! tooltip.empty())
    {
        const std::wstring tipText(tooltip);
        toggle->SetTooltipText(tipText);
        toggle->SetAccessibleHelpText(tipText);
    }
    toggle->SetOnToggled([this, command](bool) { Execute(command); });
    return toggle;
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
    const bool highContrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0 ||
                              (_options.headless && (_options.scenario == L"high-contrast" || _options.scenario == L"spelling-high-contrast"));
    if (highContrast)
    {
        const auto systemColor = [](int index)
        {
            const auto value = GetSysColor(index);
            return D2D1::ColorF(GetRValue(value) / 255.f, GetGValue(value) / 255.f, GetBValue(value) / 255.f);
        };
        theme.highContrast     = true;
        theme.windowBackground = theme.surfaceBackground = theme.cardBackground = theme.overlayBackground = systemColor(COLOR_WINDOW);
        theme.text = theme.subduedText = systemColor(COLOR_WINDOWTEXT);
        theme.disabledText             = systemColor(COLOR_GRAYTEXT);
        theme.selectionFill = theme.selectionInactiveFill = systemColor(COLOR_HIGHLIGHT);
        theme.accent = theme.accentHover = theme.accentPressed = systemColor(COLOR_HOTLIGHT);
        theme.selectionText                                    = systemColor(COLOR_HIGHLIGHTTEXT);
        theme.focusStroke = theme.focusStrokeOuter = systemColor(COLOR_HOTLIGHT);
        theme.focusStrokeInner                     = systemColor(COLOR_WINDOW);
        theme.inputFill = theme.buttonFill = theme.headerBackground = systemColor(COLOR_BTNFACE);
        theme.buttonHotFill = theme.headerHovered = systemColor(COLOR_HIGHLIGHT);
        theme.buttonPressedFill = theme.headerPressed = systemColor(COLOR_HIGHLIGHT);
        theme.overlayBorder = theme.inputBorder = theme.buttonBorder = theme.border = theme.borderDefault = theme.borderStrong = theme.gridLine =
            systemColor(COLOR_WINDOWTEXT);
        theme.smokeOverlay      = D2D1::ColorF(0.f, 0.f, 0.f, 0.65f);
        theme.tooltipBackground = systemColor(COLOR_INFOBK);
        theme.tooltipText       = systemColor(COLOR_INFOTEXT);
        theme.scrollbarTrack    = systemColor(COLOR_WINDOW);
        theme.scrollbarThumb    = systemColor(COLOR_BTNSHADOW);
        theme.scrollbarThumbHot = systemColor(COLOR_HOTLIGHT);
        theme.primaryButtonFill = systemColor(COLOR_HIGHLIGHT);
        theme.primaryButtonText = systemColor(COLOR_HIGHLIGHTTEXT);
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
        theme.overlayBackground                           = theme.surfaceBackground;
        theme.cardBackground = theme.inputFill = D2D1::ColorF(dark ? 0x22252Bu : _settings.appearance == Appearance::Paper ? 0xFFFAEFu : 0xFFFFFFu);
        theme.buttonBorder                     = theme.borderDefault;
        theme.primaryButtonFill                = D2D1::ColorF(0xC83C46);
        theme.primaryButtonText                = D2D1::ColorF(D2D1::ColorF::White);
    }
    BOOL animation = TRUE;
    (void)SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animation, 0);
    theme.reducedMotion = ! animation || _options.headless;
    _host.SetTheme(theme);
    const std::optional<D2D1_COLOR_F> mutedHelpColor = highContrast ? std::nullopt : std::optional(theme.subduedText);
    for (auto* helpLabel : {_strikeTasksHelp,
                            _drawerHelp,
                            _localNetworkImagesHelp,
                            _imageFirstPasteHelp,
                            _spellingHelp,
                            _syntaxIntro,
                            _drawerImmediate,
                            _spellingLanguageLabel,
                            _spellingDictPathLabel})
        if (helpLabel)
            helpLabel->SetTextColor(mutedHelpColor);
    for (auto* syntaxHelp : _syntaxHelp)
        if (syntaxHelp)
            syntaxHelp->SetTextColor(mutedHelpColor);
    if (_printPreview)
        _printPreview->SetTheme(theme);
    const auto underline = highContrast ? DxUi::RichTextLinkUnderlinePolicy::Always : DxUi::RichTextLinkUnderlinePolicy::HoverOrCaret;
    for (auto* target : {static_cast<DxUi::RichTextEditor*>(_editor), static_cast<DxUi::RichTextEditor*>(_mirror), _equation})
        if (! AcceptSourceResult(target->TrySetLinkUnderlinePolicy(underline)))
            return;
    const BOOL nativeDark = dark;
    (void)DwmSetWindowAttribute(_window.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &nativeDark, sizeof(nativeDark));
    Layout();
}

const std::wstring& Application::CurrentText() const
{
    const auto source = ActiveEditor()->Snapshot();
    if (! _derivedCaches.cachedSource || ! _derivedCaches.cachedSource->IsSameRevision(source))
    {
        _derivedCaches.cachedText   = source.Text();
        _derivedCaches.cachedSource = source;
    }
    return _derivedCaches.cachedText;
}
bool Application::Dirty() const
{
    auto* const activeEditor = ActiveEditor();
    if (! activeEditor)
        return false;
    const auto source = activeEditor->Snapshot();
    if (_derivedCaches.dirtyFact && _derivedCaches.dirtyFact->recovered == _session.recovered &&
        _derivedCaches.dirtyFact->resumedUnsaved == _session.resumedUnsaved && _derivedCaches.dirtyFact->source.IsSameRevision(source))
        return _derivedCaches.dirtyFact->dirty;
    bool dirty = _session.recovered || _session.resumedUnsaved || source.Length() != _session.savedText.size();
    if (! dirty)
    {
        ++_activity.dirtyTextComparisons;
        dirty = CurrentText() != _session.savedText;
    }
    _derivedCaches.dirtyFact = DerivedDocumentCaches::DirtyFact{source, _session.recovered, _session.resumedUnsaved, dirty};
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
        ! committed && newSourceRevision && _derivedCaches.committedAnalysisSource && _derivedCaches.committedAnalysisSource->MatchesText(*text);
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
std::wstring Application::ActiveDocumentDisplayName() const
{
    if (_session.origin.kind == DocumentOriginInfo::Kind::GitHub && ! _session.origin.relativePath.empty())
    {
        const std::wstring_view rel(_session.origin.relativePath);
        const auto slash = rel.find_last_of(L'/');
        const auto leaf  = slash == std::wstring_view::npos ? rel : rel.substr(slash + 1u);
        if (! leaf.empty())
            return SanitizeUiText(leaf);
    }
    return _session.path.empty() ? Resource(IDS_UNTITLED) : SanitizeUiText(_session.path.filename().native());
}
void Application::ClearStatus()
{
    if (_feedback && ! _feedback->GetText().empty())
        _feedback->SetText(L"");
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
    if (! EnsurePrimaryEditorAuthoritative())
        return false;
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
    _session.recovered      = false;
    _session.resumedUnsaved = false;
    _session.origin         = {};
    _session.spellLanguageOverride.reset();
    _session.path = std::move(path);
    _customDictionary.ClearIgnoreOnceForWorkspace(_session.key);
    _spell.appliedKey.reset();
    _spell.issues.clear();
    _spell.suppressedTypingWord.reset();
    _spell.typingSuppressionActive = false;
    EnsureFilesRoot();
    _session.bom       = hasBom;
    _session.stamp     = identity;
    _session.savedText = _editor->GetText();
    _derivedCaches.dirtyFact.reset();
    _imageWatchAcknowledgedSemantic.reset();
    _imageWatchWasRichVisible = false;
    SetView(_settings.view);
    Changed();
    ScheduleSpellCheck(true);
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
    if (! IsInitializing())
        ClearStatus();
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Enter);
    ++_activity.changes;
    auto* const activeEditor = ActiveEditor();
    const auto source        = activeEditor->Snapshot();
    RefreshDocumentAnalysis(source, true);
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Analysis);
    if (! SynchronizeMirror())
        return;
    if (_editDiagnostics)
        _editDiagnostics->Mark(EditPhase::Mirror);
    const auto name                  = ActiveDocumentDisplayName();
    const bool dirty                 = Dirty();
    const bool identityLayoutChanged = _title->GetText() != name;
    _title->SetText(name);
    if (_dirtyDot)
    {
        const bool dotChanged = _dirtyDot->IsVisible() != dirty;
        _dirtyDot->SetVisible(dirty);
        if (dotChanged && ! identityLayoutChanged)
            _host.Invalidate();
    }
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
    _tools[0]->SetEnabled(activeEditor->CanUndo());
    _tools[1]->SetEnabled(activeEditor->CanRedo());
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    if (dirty != _lastFilesDirty)
    {
        if (_settings.showFiles)
        {
            UpdateCurrentFileDirtyBadge(dirty);
            // A synchronous accessibility publication can dispatch Close and detach the host.
            if (IsClosing())
                return;
        }
        else
        {
            _lastFilesDirty = dirty;
            RefreshLeftRailDots();
        }
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
    OnSpellEditorChanged();
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

void Application::SetView(EditorView view)
{
    if (! IsUiAttached())
        return;
    if (view != EditorView::Split && ! EnsurePrimaryEditorAuthoritative())
        return;
    const bool viewChanged            = _settings.view != view;
    const std::uint64_t transitionGen = ++_viewTransitionGeneration;
    auto* const primary               = ActiveEditor();
    auto* const target                = MirrorEditor();
    auto* const owner                 = _root;
    const auto primaryLifetime        = GetControlLifetimeToken(*primary);
    const auto targetLifetime         = GetControlLifetimeToken(*target);
    const auto ownerLifetime          = GetControlLifetimeToken(*owner);
    const auto current                = [&]
    {
        return ! IsClosing() && ! primaryLifetime.expired() && ! targetLifetime.expired() && ! ownerLifetime.expired() && ActiveEditor() == primary &&
               MirrorEditor() == target && _root == owner && _host.GetRoot() == owner && _viewTransitionGeneration == transitionGen;
    };
    _selectionToolsFocus.reset();
    // The same editor/document/history survives all global presentation changes.
    // Commit _settings.view only after the underlying editor accepts the mode.
    if (primary == _editor &&
        ! AcceptSourceResult(primary->TrySetViewMode(view == EditorView::Source ? DxUi::RichText::ViewMode::Source : DxUi::RichText::ViewMode::Rich)))
        return;
    if (! current())
        return;
    _settings.view = view;
    if (viewChanged)
    {
        _imageWatchAcknowledgedSemantic.reset();
        _imageWatchWasRichVisible = false;
    }
    // Source supersession aborts source-bound work, but this view transition
    // uses the latest source. Retired owners and a newer view request still win.
    (void)SynchronizeMirror();
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
    ScheduleSpellCheck(true);
    _host.SetFocusControl(primary, false);
    if (! current())
        return;
    if (! IsInitializing() && viewChanged)
        SavePreferences();
}
void Application::EnsureSyntaxLookupCache(const std::shared_ptr<const DxUi::RichText::MarkdownSyntax>& syntax)
{
    auto& cache = _derivedCaches.syntaxCache;
    if (cache.syntax == syntax)
        return;
    cache.syntax = syntax;
    cache.topLevelBlocks.clear();
    cache.fencedMathBlocks.clear();
    if (! syntax)
        return;
    const auto& nodes = syntax->Nodes();
    if (nodes.empty())
        return;
    for (std::size_t idx = nodes[0].firstChild; idx != DxUi::RichText::MarkdownNoNode; idx = nodes[idx].nextSibling)
        cache.topLevelBlocks.push_back(static_cast<std::uint32_t>(idx));
    for (std::size_t idx = 1; idx < nodes.size(); ++idx)
    {
        const auto& block = nodes[idx].block;
        if (block.kind == DxUi::RichText::MarkdownBlockKind::FencedCode && (block.fenceMarker == L'$' || block.fenceMarker == L'[') && block.closed)
            cache.fencedMathBlocks.push_back(static_cast<std::uint32_t>(idx));
    }
    std::sort(cache.fencedMathBlocks.begin(),
              cache.fencedMathBlocks.end(),
              [&](std::uint32_t a, std::uint32_t b) noexcept
    {
        const auto leftStart  = nodes[a].block.source.first;
        const auto rightStart = nodes[b].block.source.first;
        return leftStart < rightStart;
    });
}
void Application::SelectionChanged(bool refreshTools)
{
    EditDiagnostics::SelectionInvocation selectionInvocation(_editDiagnostics);
    if (IsClosing() || _switchingDocument || ! _floating)
        return;
    auto* const activeEditor = ActiveEditor();
    if (_pendingBitmapPaste && (_pendingBitmapPaste->lifetime.expired() || _pendingBitmapPaste->documentKey != _session.key ||
                                _pendingBitmapPaste->view != _settings.view || _pendingBitmapPaste->documentPath != _session.path ||
                                _pendingBitmapPaste->selection != activeEditor->GetSelection() || activeEditor->IsSelectionGestureActive() ||
                                activeEditor->Snapshot().ChangedRangeFrom(_pendingBitmapPaste->source).kind != DxUi::RichText::SourceChangeKind::Unchanged))
        _pendingBitmapPaste.reset();
    if (activeEditor->IsSelectionGestureActive())
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
        return activeEditor->GetSelection();
    }();
    const auto source = [&]
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionSnapshot);
        return activeEditor->Snapshot();
    }();
    const auto sourceRevision = activeEditor->Revision();
    OnSpellSelectionChanged();
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::DocumentAnalysisRefresh);
        RefreshDocumentAnalysis(source, false);
    }
    const auto syntax = [&]
    {
        EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::SelectionSnapshot);
        return activeEditor->GetMarkdownSyntax();
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
            auto* const primary        = activeEditor;
            auto* const target         = MirrorEditor();
            auto* const owner          = _root;
            const auto primaryLifetime = GetControlLifetimeToken(*primary);
            const auto targetLifetime  = GetControlLifetimeToken(*target);
            const auto ownerLifetime   = GetControlLifetimeToken(*owner);
            const auto current         = [&]
            {
                if (IsClosing() || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || ActiveEditor() != primary ||
                    MirrorEditor() != target || _root != owner || _host.GetRoot() != owner || _settings.view != EditorView::Split)
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
            if (_settings.splitScrollSync)
            {
                _splitScrollSyncInProgress = true;
                const auto resetSync       = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
                (void)target->ScrollSourceRangeIntoView(_host, selection.Start(), selection.End(), false);
            }
            ++_activity.mirrorSelectionUpdates;
        }
        std::wstring expression;
        std::array<bool, 4> formats{};
        EnsureSyntaxLookupCache(syntax);
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::InlineSemantics);
            // In-place math source previews what is being typed; only the region itself is read.
            if (const auto region = activeEditor->GetSemanticSourceRegion(); region && region->source.first <= region->source.last &&
                                                                             region->source.last <= source.Length() &&
                                                                             selection.caret >= region->source.first && selection.caret <= region->source.last)
                expression = MathRegionExpression(*region, source.Slice(region->source.first, region->source.last));
            if (syntax && ! _derivedCaches.syntaxCache.topLevelBlocks.empty())
            {
                const auto& nodes               = syntax->Nodes();
                const auto& inlines             = syntax->Inlines();
                const auto& topLevel            = _derivedCaches.syntaxCache.topLevelBlocks;
                const auto collectInlineMatches = [&](auto&& self, std::size_t inlineIdx) -> void
                {
                    while (inlineIdx != DxUi::RichText::MarkdownNoNode)
                    {
                        const auto& inlineNode = inlines[inlineIdx];
                        if (selection.caret < inlineNode.source.first)
                            break;
                        if (selection.caret <= inlineNode.source.last)
                        {
                            using Kind = DxUi::RichText::MarkdownInlineKind;
                            if (inlineNode.kind == Kind::Math && expression.empty() && inlineNode.value.size() <= DxUi::RichText::MathMaximumSourceUnits)
                                expression = inlineNode.value;
                            if (inlineNode.kind == Kind::Strong)
                                formats[0] = true;
                            if (inlineNode.kind == Kind::Emphasis)
                                formats[1] = true;
                            if (inlineNode.kind == Kind::Code)
                                formats[2] = true;
                            if (inlineNode.kind == Kind::Link)
                                formats[3] = true;
                            if (inlineNode.firstChild != DxUi::RichText::MarkdownNoNode)
                                self(self, inlineNode.firstChild);
                        }
                        inlineIdx = inlineNode.nextSibling;
                    }
                };
                const auto scanBlockSubtree = [&](auto&& self, std::size_t blockIdx) -> void
                {
                    while (blockIdx != DxUi::RichText::MarkdownNoNode)
                    {
                        const auto& subNode = nodes[blockIdx];
                        if (selection.caret < subNode.block.source.first)
                            break;
                        if (selection.caret <= subNode.block.source.last)
                        {
                            if (subNode.firstInline != DxUi::RichText::MarkdownNoNode)
                                collectInlineMatches(collectInlineMatches, subNode.firstInline);
                            if (subNode.firstChild != DxUi::RichText::MarkdownNoNode)
                                self(self, subNode.firstChild);
                        }
                        blockIdx = subNode.nextSibling;
                    }
                };
                auto it = std::upper_bound(topLevel.begin(), topLevel.end(), selection.caret, [&](std::size_t pos, std::uint32_t idx) noexcept {
                    return pos < nodes[idx].block.source.first;
                });
                if (it != topLevel.begin())
                {
                    --it;
                    while (it != topLevel.begin())
                    {
                        const auto prevIdx = *std::prev(it);
                        if (nodes[prevIdx].block.source.last < selection.caret)
                            break;
                        --it;
                    }
                    for (; it != topLevel.end(); ++it)
                    {
                        const auto& topNode = nodes[*it];
                        if (selection.caret < topNode.block.source.first)
                            break;
                        if (selection.caret <= topNode.block.source.last)
                        {
                            if (topNode.firstInline != DxUi::RichText::MarkdownNoNode)
                                collectInlineMatches(collectInlineMatches, topNode.firstInline);
                            if (topNode.firstChild != DxUi::RichText::MarkdownNoNode)
                                scanBlockSubtree(scanBlockSubtree, topNode.firstChild);
                        }
                    }
                }
            }
            // Own-line $$ and \[ \] display math is a source-owned block, not an inline node.
            if (syntax && expression.empty() && ! _derivedCaches.syntaxCache.fencedMathBlocks.empty())
            {
                const auto& nodes      = syntax->Nodes();
                const auto& mathBlocks = _derivedCaches.syntaxCache.fencedMathBlocks;
                auto it = std::upper_bound(mathBlocks.begin(), mathBlocks.end(), selection.caret, [&](std::size_t pos, std::uint32_t idx) noexcept {
                    return pos < nodes[idx].block.source.first;
                });
                while (it != mathBlocks.begin())
                {
                    --it;
                    const auto& mathNode = nodes[*it];
                    if (selection.caret >= mathNode.block.source.first && selection.caret <= WithoutLineEnd(syntax->Source(), mathNode.block.source))
                    {
                        auto body = syntax->Slice(mathNode.block.content);
                        while (! body.empty() && (body.back() == L'\n' || body.back() == L'\r'))
                            body.remove_suffix(1);
                        if (body.size() <= DxUi::RichText::MathMaximumSourceUnits)
                            expression = body;
                        break;
                    }
                    if (mathNode.block.source.last < selection.caret)
                        break;
                }
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
            const bool sourceViewActive    = activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source;
            _tools[2]->SetEnabled(! sourceViewActive);
            _tools[6]->SetEnabled(! sourceViewActive);
            _tools[7]->SetEnabled(! sourceViewActive);
            for (unsigned i = 0; i < 4; ++i)
            {
                auto* button = _tools[i == 3 ? 12 : i + 3];
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
            const auto previousHeading = _currentHeading;
            _currentHeading.reset();
            const auto headingIt = std::upper_bound(_documentInfo.headings.begin(),
                                                    _documentInfo.headings.end(),
                                                    selection.caret,
                                                    [](std::size_t caretPos, const Heading& heading) noexcept { return caretPos < heading.offset; });
            if (headingIt != _documentInfo.headings.begin())
                _currentHeading = static_cast<std::size_t>(std::distance(_documentInfo.headings.begin(), headingIt) - 1);
            const auto headingId = _currentHeading ? std::optional<std::uint64_t>(_currentHeading.value() + 1) : std::nullopt;
            // Model refresh already reconciles surviving focused rows and keeps the viewport.
            // Republish only an actual selection or focus transition, which reveals it.
            if (_outlineTree->GetSelectedItemId() != headingId || _outlineTree->GetFocusedItemId() != headingId)
            {
                ++_activity.outlineSelectionUpdates;
                _outlineTree->SetSelectedItemId(headingId);
            }
            if (_currentHeading != previousHeading)
                RefreshRightRailDots();
        }
        DocumentPosition position;
        std::optional<std::pair<unsigned, std::size_t>> listFacts;
        unsigned blockResourceId{};
        std::size_t count{};
        {
            EditDiagnostics::SelectionMeasure measure(_editDiagnostics, SelectionComponent::BlockLookup);
            position = DocumentPositionAt(_documentInfo, selection.caret);
            count    = position.lineCharacters;
            blockResourceId =
                syntax ? CurrentBlockResourceAt(*syntax, _derivedCaches.syntaxCache.topLevelBlocks, selection.caret, &listFacts) : IDS_BLOCKPARAGRAPH;
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
    if (selection.Start() == selection.End() || activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source ||
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
    const auto rectangles = activeEditor->TryGetTextInputRangeRects(_host, selection.Start(), selection.End());
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
    const auto first        = rectangles->front();
    const auto editorBounds = activeEditor->GetBounds();
    const float minLeft     = _settings.view == EditorView::Split ? editorBounds.left : 16.f;
    const float maxRight    = (_settings.view == EditorView::Split ? editorBounds.right + 16.f : _host.GetClientBoundsDip().right) - 16.f;
    const float left        = std::clamp(first.left, minLeft, std::max(minLeft, maxRight - 436.f));
    const float minimumTop  = editorBounds.top + 4.f;
    const float top         = std::clamp(first.top - 52, minimumTop, std::max(minimumTop, _page->GetBounds().bottom - 52));
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
    auto* const activeEditor = ActiveEditor();
    _selectionToolsFocus     = SelectionToolsFocusRequest{activeEditor->Snapshot(), activeEditor->GetSelection(), _settings.view};
    SelectionChanged();
}

void Application::SavePreferences()
{
    const bool allowSave = ! _options.headless
#if defined(ENABLE_TESTS)
                           || _options.preferencesProbe || _options.watchSettingsForTest
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
    if (_preferenceWriter.Busy())
    {
        _settingsReloadDeferred = true;
        return;
    }
    if (_window)
        (void)KillTimer(_window.get(), SettingsRetryTimerId);
    Settings reloaded;
    FileStamp stamp{};
    const HRESULT loadResult = LoadSettingsLive(_options.dataDirectory, reloaded, &stamp);
    if (FAILED(loadResult))
    {
        if (loadResult == HRESULT_FROM_WIN32(ERROR_RETRY))
        {
            if (_settingsReloadRetries < 3u && _window)
            {
                ++_settingsReloadRetries;
                (void)SetTimer(_window.get(), SettingsRetryTimerId, 150u, nullptr);
            }
            return;
        }
        _settingsReloadRetries = 0u;
        if (loadResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || loadResult == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
            return;
        OutputDebugStringW(L"RedTilde: ignoring malformed live settings edit on disk\n");
        MonitorDiagnostics::Write(RedMonitor::Severity::Warning, L"Ignoring malformed live settings edit on disk");
        return;
    }
    _settingsReloadRetries  = 0u;
    _settingsReloadDeferred = false;

    if (stamp == _settingsFileStamp)
        return;
    if (const auto writtenStamp = _preferenceWriter.LastWrittenStamp(); writtenStamp && *writtenStamp == stamp)
    {
        _settingsFileStamp = stamp;
        return;
    }
    _settingsFileStamp = stamp;

    const bool appearanceChanged   = _settings.appearance != reloaded.appearance;
    const bool clipboardChanged    = _settings.imageFirstPaste != reloaded.imageFirstPaste;
    const bool localNetChanged     = _settings.allowLocalNetworkImages != reloaded.allowLocalNetworkImages;
    const bool remoteChanged       = _settings.allowRemoteImages != reloaded.allowRemoteImages;
    const bool syntaxChanged       = _settings.markdownSyntax != reloaded.markdownSyntax;
    const bool splitSyncChanged    = _settings.splitScrollSync != reloaded.splitScrollSync;
    const bool pinModesChanged     = _settings.filesPinMode != reloaded.filesPinMode || _settings.outlinePinMode != reloaded.outlinePinMode;
    const bool spellingChanged     = _settings.spelling != reloaded.spelling;
    const bool dictPathChanged     = _settings.spelling.customDictionaryPath != reloaded.spelling.customDictionaryPath;
    const bool exitBehaviorChanged = _settings.exitUnsavedBehavior != reloaded.exitUnsavedBehavior;
    const bool layoutNeeded        = appearanceChanged || _settings.readingWidth != reloaded.readingWidth || _settings.textScale != reloaded.textScale ||
                                     _settings.showOutline != reloaded.showOutline || _settings.dimInactiveBlocks != reloaded.dimInactiveBlocks ||
                                     _settings.strikeCompletedTasks != reloaded.strikeCompletedTasks || syntaxChanged || remoteChanged || localNetChanged ||
                                     splitSyncChanged || pinModesChanged;

    _settings.appearance              = reloaded.appearance;
    _settings.readingWidth            = reloaded.readingWidth;
    _settings.textScale               = reloaded.textScale;
    _settings.showOutline             = reloaded.showOutline;
    _settings.dimInactiveBlocks       = reloaded.dimInactiveBlocks;
    _settings.allowRemoteImages       = reloaded.allowRemoteImages;
    _settings.allowLocalNetworkImages = reloaded.allowLocalNetworkImages;
    _settings.imageFirstPaste         = reloaded.imageFirstPaste;
    _settings.strikeCompletedTasks    = reloaded.strikeCompletedTasks;
    _settings.exitUnsavedBehavior     = reloaded.exitUnsavedBehavior;
    _settings.markdownSyntax          = reloaded.markdownSyntax;
    _settings.splitScrollSync         = reloaded.splitScrollSync;
    _settings.filesPinMode            = reloaded.filesPinMode;
    _settings.outlinePinMode          = reloaded.outlinePinMode;
    _settings.spelling                = reloaded.spelling;

    if (exitBehaviorChanged && _keepUnsavedToggle)
        _keepUnsavedToggle->SetChecked(_settings.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume);
    if (clipboardChanged)
    {
        _host.SetClipboardPreference(_settings.imageFirstPaste ? DxUi::TextClipboardPreference::ImageFirst : DxUi::TextClipboardPreference::TextFirst);
        if (_imageFirstPasteToggle)
            _imageFirstPasteToggle->SetChecked(_settings.imageFirstPaste);
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
            _remoteImagesToggle->SetChecked(_settings.allowRemoteImages);
        if (_localNetworkImagesToggle)
            _localNetworkImagesToggle->SetChecked(_settings.allowLocalNetworkImages);
        ServiceImages();
    }
    if (syntaxChanged && _editor && _mirror && _equation)
        ApplyMarkdownSyntax();
    if (spellingChanged)
    {
        ++_spell.languageConfigVersion;
        if (dictPathChanged)
            (void)_customDictionary.Load(ActiveCustomDictionaryPath());
        _spell.activeLanguages = ResolveEffectiveSpellLanguages(std::nullopt);
        RefreshProofingDrawerControls();
        RefreshSpellStatusChip();
        ScheduleSpellCheck(true);
    }
    if (appearanceChanged)
        ApplyTheme();
    else if (layoutNeeded)
        Layout();
}
void Application::SetActiveOverlay(OverlayKind next)
{
    const OverlayKind previous = _activeOverlay;
    const bool wasFindBar      = previous == OverlayKind::FindBar;
    const bool wasBlockDraft   = previous == OverlayKind::BlockDraftModal;
    const bool isModal         = IsModalOverlayKind(next);
    if (next == OverlayKind::FindBar || next == OverlayKind::SettingsDrawer || next == OverlayKind::BlockDraftModal || isModal)
        DismissHoverOverlays(false);
    if (next != OverlayKind::FloatingSelectionTools)
    {
        _pendingSelectionTools = false;
        _selectionToolsFocus.reset();
    }
    if (wasBlockDraft && next != OverlayKind::BlockDraftModal)
    {
        _blockDraftSession.reset();
        auto* const activeEditor = ActiveEditor();
        activeEditor->SetReadOnly(false);
        (void)AcceptSourceResult(activeEditor->TrySetSourceHighlights({}));
    }
    if (previous == OverlayKind::SaveConfirmModal && next != OverlayKind::SaveConfirmModal &&
        (! _saveConfirmSession || ! _saveConfirmSession->sequentialSaveActive))
        _saveConfirmSession.reset();
    if (previous == OverlayKind::RenameFileModal && next != OverlayKind::RenameFileModal)
        _renameSession.reset();
    if (previous == OverlayKind::FileConflictModal && next != OverlayKind::FileConflictModal)
    {
        if (_fileConflictSession)
            _pendingPasteQueue.clear();
        _fileConflictSession.reset();
    }
    if ((previous == OverlayKind::OpenGitHubModal && next != OverlayKind::OpenGitHubModal) ||
        (previous == OverlayKind::SettingsDrawer && next != OverlayKind::SettingsDrawer))
    {
        ClearPendingDeviceFlow();
        ClearGitHubTokenInputs();
        RefreshGitHubAuthControls();
    }
    if (_modalBackdrop)
        _modalBackdrop->SetVisible(isModal);
    _floating->SetVisible(next == OverlayKind::FloatingSelectionTools);
    _findBar->SetVisible(next == OverlayKind::FindBar);
    _drawer->SetVisible(next == OverlayKind::SettingsDrawer);
    _blockDraft->SetVisible(next == OverlayKind::BlockDraftModal);
    _recoveryPanel->SetVisible(next == OverlayKind::RecoveryModal);
    if (_saveConfirmPanel)
        _saveConfirmPanel->SetVisible(next == OverlayKind::SaveConfirmModal);
    if (_renamePanel)
        _renamePanel->SetVisible(next == OverlayKind::RenameFileModal);
    if (_fileConflictPanel)
        _fileConflictPanel->SetVisible(next == OverlayKind::FileConflictModal);
    if (_openGitHubPanel)
        _openGitHubPanel->SetVisible(next == OverlayKind::OpenGitHubModal);
    _activeOverlay = next;
    if (wasFindBar && next != OverlayKind::FindBar)
        RefreshFind();
    if (next == OverlayKind::BlockDraftModal && ! wasBlockDraft)
    {
        ProjectVisibleSpellDecorations();
    }
    else if (wasBlockDraft && next != OverlayKind::BlockDraftModal && _settings.spelling.enabled)
    {
        if (_spell.checkPending || ! _spell.appliedKey)
            ScheduleSpellCheck(true);
        else
            ProjectVisibleSpellDecorations();
    }
    if (next == OverlayKind::None && _window && _recovery.Ready() && (! _sessionResumeApplied || ! _recoveryOffered) && ! HasActiveModalUi() &&
        ! _files.Busy() && ! _switchingDocument && ! std::holds_alternative<HeldOpenTx>(_activeFileTx) && ! IsClosing() && ! IsSessionEnding())
        (void)PostMessageW(_window.get(), RecoveryReadyMessage, 0, 0);
}

DxUi::Panel* Application::ActiveModalPanel() const noexcept
{
    switch (_activeOverlay)
    {
        case OverlayKind::RecoveryModal: return _recoveryPanel;
        case OverlayKind::SaveConfirmModal: return _saveConfirmPanel;
        case OverlayKind::RenameFileModal: return _renamePanel;
        case OverlayKind::FileConflictModal: return _fileConflictPanel;
        case OverlayKind::OpenGitHubModal: return _openGitHubPanel;
        case OverlayKind::BlockDraftModal: return HasBlockDraft() ? _blockDraft : nullptr;
        case OverlayKind::None:
        case OverlayKind::FloatingSelectionTools:
        case OverlayKind::FindBar:
        case OverlayKind::SettingsDrawer:
        default: return nullptr;
    }
}

bool Application::HandleModalTabKey()
{
    auto* const modalPanel = ActiveModalPanel();
    if (! modalPanel || ! modalPanel->IsVisible())
        return false;
    const bool ctrl  = KeyboardState::Held(VK_CONTROL);
    const bool alt   = KeyboardState::Held(VK_MENU);
    const bool shift = KeyboardState::Held(VK_SHIFT);
    if (ctrl || alt)
        return true;
    auto* const focused = _host.GetFocusControl();
    if (focused && _host.GetNativeTextCompositionActive(focused).value_or(false))
        return false;
    if (modalPanel == _openGitHubPanel && focused == _openGitHubTokenInput && _openGitHubTokenInput && _openGitHubTokenInput->IsVisible() &&
        _openGitHubTokenInput->IsEnabled() && _openGitHubTokenInput->OnKeyDown(_host, VK_TAB, shift ? MK_SHIFT : 0u))
        return true;

    std::vector<DxUi::Control*> focusables;
    const auto collect = [&](auto&& self, DxUi::Control* control) -> void
    {
        if (! control || ! control->IsVisible() || ! control->IsEnabled())
            return;
        if (control != modalPanel && control->IsFocusable())
            focusables.push_back(control);
        const std::size_t childCount = control->GetLogicalChildCount();
        for (std::size_t i = 0; i < childCount; ++i)
            self(self, control->GetLogicalChild(i));
    };
    collect(collect, modalPanel);
    if (focusables.empty())
        return true;

    const auto it = std::ranges::find(focusables, focused);
    DxUi::Control* target{};
    if (it == focusables.end())
        target = shift ? focusables.back() : focusables.front();
    else
    {
        const auto idx = static_cast<std::size_t>(std::distance(focusables.begin(), it));
        target         = focusables[shift ? ((idx + focusables.size() - 1u) % focusables.size()) : ((idx + 1u) % focusables.size())];
    }
    _host.SetFocusControl(target, MayRequestNativeFocus());
    return true;
}

void Application::CloseDrawers()
{
    const bool hadBlockDraft = HasBlockDraft();
    _saveConfirmSession.reset();
    _renameSession.reset();
    _fileConflictSession.reset();
    _pendingPasteQueue.clear();
    ClearPendingDeviceFlow();
    ClearGitHubTokenInputs();
    SetActiveOverlay(OverlayKind::None);
    Layout();
    _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
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
        AbortSaveConfirmBatch();
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
#if defined(ENABLE_TESTS)
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
        case Command::AddFolder: AddFolder(); return;
        case Command::OpenGitHub: ShowOpenGitHubModal(); return;
        case Command::Overflow: FileMenu(true); return;
        case Command::Files:
        case Command::PinFiles:
        {
            if (_activeOverlay == OverlayKind::FloatingSelectionTools)
                SetActiveOverlay(OverlayKind::None);
            else
            {
                _pendingSelectionTools = false;
                _selectionToolsFocus.reset();
            }
            const bool docked = _leftRail && _leftRail->State().mode == PaneDisplayMode::Docked && _leftRail->State().phase != PaneAnimationPhase::Collapsing;
            if (docked)
            {
                _settings.showFiles    = false;
                _settings.filesPinMode = PanePinMode::Unpinned;
            }
            else
            {
                _settings.showFiles    = true;
                _settings.filesPinMode = PanePinMode::Pinned;
                _lastPinnedSide        = PaneSide::Left;
            }
            DismissHoverOverlays(false);
            RefreshFiles(true);
            Layout();
            SavePreferences();
            _host.SetFocusControl(! docked && _filesPane->IsVisible() ? static_cast<DxUi::Control*>(_filesTree) : ActiveEditor(), MayRequestNativeFocus());
            return;
        }
        case Command::Outline:
        case Command::PinOutline:
        {
            const bool docked =
                _rightRail && _rightRail->State().mode == PaneDisplayMode::Docked && _rightRail->State().phase != PaneAnimationPhase::Collapsing;
            DismissHoverOverlays(false);
            if (docked)
            {
                _settings.showOutline    = false;
                _settings.outlinePinMode = PanePinMode::Unpinned;
            }
            else
            {
                _settings.showOutline    = true;
                _settings.outlinePinMode = PanePinMode::Pinned;
                _lastPinnedSide          = PaneSide::Right;
            }
            Layout();
            SavePreferences();
            _host.SetFocusControl(! docked && _sidebar && _sidebar->IsVisible() && _outlineTree ? static_cast<DxUi::Control*>(_outlineTree) : ActiveEditor(),
                                  MayRequestNativeFocus());
            return;
        }
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
            auto* const activeEditor = ActiveEditor();
            const auto document      = activeEditor->GetSemanticDocument();
            const auto index         = CurrentLinkElement(document);
            if (! index)
            {
                Status(IDS_LINKUNAVAILABLE);
                return;
            }
            const auto& link = document->Elements()[index.value()];
            if (link.destination.starts_with(L'#'))
            {
                if (! activeEditor->NavigateSemanticElement(_host, document, index.value()))
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
            _remoteImagesToggle->SetChecked(_settings.allowRemoteImages);
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
            _localNetworkImagesToggle->SetChecked(_settings.allowLocalNetworkImages);
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
            _imageFirstPasteToggle->SetChecked(_settings.imageFirstPaste);
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
            _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
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
                    const RecoveryCandidate adopted{found->id, found->snapshot};
                    if (! AdoptRecoveryCandidate(adopted, false, true))
                        return;
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
            const auto* activeEd    = ActiveEditor();
            const std::size_t reuse = _session.path.empty() && ! Dirty() && ! activeEd->CanUndo() && ! activeEd->CanRedo() ? 1u : 0u;
            if (_inactiveDocuments.size() + 1 + candidates.size() - reuse > RecoveryService::MaximumDocuments)
            {
                Status(IDS_WORKSPACEFULL);
                return;
            }
            bool anyAdopted{};
            bool anyFailed{};
            for (const auto& candidate : candidates)
            {
                const auto found = std::ranges::find_if(_recoveryCandidates, [&](const auto& current) { return current.id == candidate.id; });
                if (found == _recoveryCandidates.end() || found->disposition != RecoveryDisposition::Available)
                    continue; // A completion retired or started discarding this exact copy; never substitute another.
                const RecoveryCandidate adopted{found->id, found->snapshot};
                if (! AdoptRecoveryCandidate(adopted, false, true))
                {
                    anyFailed = true;
                    break;
                }
                anyAdopted = true;
                std::erase_if(_recoveryCandidates, [&](const auto& current) { return current.id == candidate.id; });
            }
            if (! anyAdopted)
                return;
            if (! _inactiveDocuments.empty())
                _workspaceSelectionPreferred = true;
            RefreshFiles();
            if (anyFailed)
            {
                ShowRecovery();
                Status(IDS_RECOVERYFAILED);
                return;
            }
            CloseDrawers();
            Status(IDS_RECOVERED);
            return;
        }
        case Command::New:
            AskWorkspaceThen([this]
            {
                ResetWorkspace(true);
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
        case Command::SwitchSplitPane:
            SwitchSplitActivePane(_splitActivePane == SplitActivePane::Compose ? SplitActivePane::Source : SplitActivePane::Compose, true);
            return;
        case Command::ToggleSplitScrollSync:
            _settings.splitScrollSync = ! _settings.splitScrollSync;
            Layout();
            if (_settings.splitScrollSync && _settings.view == EditorView::Split)
            {
                if (const auto info = ActiveEditor()->GetScrollInfo())
                    ApplySplitScrollSync(ActiveEditor(), info->offsetDip);
            }
            SavePreferences();
            return;
        case Command::Undo:
            (void)ActiveEditor()->Undo();
            semantic = false;
            break;
        case Command::Redo:
            (void)ActiveEditor()->Redo();
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
        {
            auto* const activeEditor = ActiveEditor();
            if (activeEditor->GetViewMode() == DxUi::RichText::ViewMode::Source)
            {
                const std::wstring_view marker = command == Command::Bold     ? L"**"
                                                 : command == Command::Italic ? L"*"
                                                 : command == Command::Strike ? L"~~"
                                                                              : L"`";
                if (! activeEditor->ToggleFormatting(marker))
                {
                    if (activeEditor->Snapshot().Length() + marker.size() * 2u > Document::MaximumUnits)
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
        }
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
            const auto selection = ActiveEditor()->GetSelection();
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
            const auto path          = PickFile(false, true);
            auto* const activeEditor = ActiveEditor();
            if (path.empty() || ! CanAcceptCommand() || ! IsUiAttached() || ! activeEditor)
                return;
            DxUi::TextClipboardContent content;
            content.filePaths.push_back(path.native());
            std::wstring insertion;
            if (activeEditor->TryPrepareClipboardPaste(content, insertion))
            {
                const auto selection     = activeEditor->GetSelection();
                const auto currentLength = activeEditor->Snapshot().Length();
                const auto replaced      = selection.End() - selection.Start();
                const auto insertedUnits = insertion.size();
                if (! activeEditor->ReplaceSelection(std::move(insertion)))
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
            auto* const activeEditor = ActiveEditor();
            const auto snippet =
                NormalizeInsertedLineEndings(command == Command::Mermaid ? L"\n\n```mermaid\ngraph LR\nA[Idea] --> B[Write]\nB --> C[Share]\n```\n\n"
                                                                         : L"\n\n$$\nx = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}\n$$\n\n",
                                             CurrentText());
            const auto selection     = activeEditor->GetSelection();
            const auto currentLength = activeEditor->Snapshot().Length();
            const auto replaced      = selection.End() - selection.Start();
            const auto insertedUnits = snippet.size();
            if (! activeEditor->ReplaceSelection(snippet))
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
            auto* const activeEditor = ActiveEditor();
            const auto selection     = activeEditor->GetSelection();
            const bool quote         = command == Command::Quote;
            const auto snippet =
                NormalizeInsertedLineEndings(quote ? L"\n> A thought worth keeping.\n" : L"\n\n```text\nYour code here\n```\n\n", CurrentText());
            const bool changed = selection.Start() != selection.End()
                                     ? activeEditor->WrapSelectionInMarkdownBlock(quote ? MarkdownBlockKind::BlockQuote : MarkdownBlockKind::FencedCode,
                                                                                  quote ? std::wstring_view{} : std::wstring_view(L"text"))
                                     : activeEditor->ReplaceSelection(snippet);
            if (changed)
                Status(IDS_FORMATTED);
            else
            {
                const auto currentLength    = activeEditor->Snapshot().Length();
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
            _host.SetFocusControl(opening ? static_cast<DxUi::Control*>(_themeButtons[0]) : ActiveEditor(), MayRequestNativeFocus());
            return;
        }
        case Command::Focus:
            _settings.focusMode = ! _settings.focusMode;
            CloseDrawers();
            Layout();
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
        case Command::ToggleSpelling:
        case Command::SpellIgnoreUppercase:
        case Command::SpellIgnoreDigits:
        case Command::SpellIgnoreInternetPaths:
        case Command::SpellCycleLanguage:
        case Command::SpellChooseCustomDictionary:
        case Command::SpellResetCustomDictionary:
        case Command::OpenProofingSettings: ExecuteSpellingCommand(command); return;
        case Command::ExitUnsavedBehavior:
            _settings.exitUnsavedBehavior =
                _settings.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume ? ExitUnsavedBehavior::Prompt : ExitUnsavedBehavior::KeepAndResume;
            if (_keepUnsavedToggle)
                _keepUnsavedToggle->SetChecked(_settings.exitUnsavedBehavior == ExitUnsavedBehavior::KeepAndResume);
            SavePreferences();
            return;
        case Command::SaveConfirmSelectAll:
        {
            if (! _saveConfirmSession || _syncingSaveConfirmChecks)
                return;
            const bool checked        = _saveConfirmSelectAll && _saveConfirmSelectAll->IsChecked();
            _syncingSaveConfirmChecks = true;
            for (std::size_t i = 0; i < _saveConfirmSession->items.size() && i < _saveConfirmRowControls.size(); ++i)
            {
                _saveConfirmSession->items[i].selected = checked;
                _saveConfirmRowControls[i].checkbox->SetChecked(checked);
            }
            _syncingSaveConfirmChecks = false;
            const bool anyChecked     = std::ranges::any_of(_saveConfirmSession->items, &SaveConfirmItem::selected);
            if (_saveConfirmPrimary)
                _saveConfirmPrimary->SetEnabled(anyChecked || _saveConfirmSession->items.size() <= 1u);
            return;
        }
        case Command::SaveConfirmAccept:
        {
            if (! _saveConfirmSession)
                return;
            if (_saveConfirmSession->items.size() <= 1u)
            {
                _saveConfirmSession->items[0].selected = true;
                for (std::size_t i = 0; i < _saveConfirmRowControls.size(); ++i)
                    _saveConfirmRowControls[i].checkbox->SetChecked(true);
            }
            const bool anyChecked = std::ranges::any_of(_saveConfirmSession->items, &SaveConfirmItem::selected);
            if (! anyChecked)
            {
                auto cont = std::move(_saveConfirmSession->continuation);
                _saveConfirmSession.reset();
                SetActiveOverlay(OverlayKind::None);
                Layout();
                if (cont)
                    cont();
                return;
            }
            _saveConfirmSession->sequentialSaveActive = true;
            SetActiveOverlay(OverlayKind::None);
            Layout();
            AdvanceSaveConfirmSequentialSave();
            return;
        }
        case Command::SaveConfirmKeepExit:
        {
            _saveConfirmSession.reset();
            SetActiveOverlay(OverlayKind::None);
            Layout();
            BeginClose(true);
            return;
        }
        case Command::SaveConfirmDiscard:
        {
            if (! _saveConfirmSession)
                return;
            auto cont = std::move(_saveConfirmSession->continuation);
            _saveConfirmSession.reset();
            SetActiveOverlay(OverlayKind::None);
            Layout();
            if (cont)
                cont();
            return;
        }
        case Command::RenameConfirm: CommitRenameFileModal(); return;
        case Command::FileConflictReplace: CommitFileConflictResolution(true); return;
        case Command::FileConflictKeepBoth: CommitFileConflictResolution(false); return;
        case Command::GitHubOpenConfirm: CommitOpenGitHubModal(); return;
        case Command::GitHubSignInDevice:
        {
            if (! _github.DeviceFlowConfigured())
            {
                Status(IDS_GITHUB_ERR_DEVICE_UNAVAILABLE);
                if (_openGitHubErrorLabel && _activeOverlay == OverlayKind::OpenGitHubModal)
                    _openGitHubErrorLabel->SetText(Resource(IDS_GITHUB_ERR_DEVICE_UNAVAILABLE));
                return;
            }
            GitHubRequest req;
            req.kind      = GitHubRequest::Kind::StartDeviceFlow;
            req.requestId = ++_nextGitHubRequestId;
            req.flowId    = ++_nextGitHubFlowId;
            if (FAILED(_github.Submit(std::move(req))))
            {
                Status(IDS_BUSY);
                return;
            }
            ClearPendingDeviceFlow();
            _activeDeviceFlowId = req.flowId;
            RefreshGitHubAuthControls();
            if (_activeOverlay == OverlayKind::SettingsDrawer || _activeOverlay == OverlayKind::OpenGitHubModal)
                Layout();
            return;
        }
        case Command::GitHubImportGhCli:
        {
            if (_githubImportInProgress || ! _options.capture.empty())
                return;
            GitHubRequest req;
            req.kind      = GitHubRequest::Kind::ImportGhCli;
            req.requestId = ++_nextGitHubRequestId;
            if (FAILED(_github.Submit(std::move(req))))
            {
                Status(IDS_BUSY);
                return;
            }
            _githubImportInProgress = true;
            if (_openGitHubErrorLabel)
                _openGitHubErrorLabel->SetText(L"");
            RefreshGitHubAuthControls();
            Status(IDS_LOADING);
            return;
        }
        case Command::GitHubUseToken:
        {
            DxUi::TextField* sourceInput = nullptr;
            if (_activeOverlay == OverlayKind::OpenGitHubModal)
                sourceInput = _openGitHubTokenInput;
            else if (_activeOverlay == OverlayKind::SettingsDrawer)
                sourceInput = _accountsTokenInput;
            if (! sourceInput || sourceInput->GetText().empty())
                return;

            GitHubRequest req;
            req.kind      = GitHubRequest::Kind::SignInWithPat;
            req.requestId = ++_nextGitHubRequestId;
            req.tokenOrDeviceCode.Assign(sourceInput->GetText());
            sourceInput->SetText(L"");
            if (FAILED(_github.Submit(std::move(req))))
                Status(IDS_BUSY);
            return;
        }
        case Command::GitHubSignOut:
        {
            ClearPendingDeviceFlow();
            ClearGitHubTokenInputs();
            GitHubRequest req;
            req.kind      = GitHubRequest::Kind::SignOut;
            req.requestId = ++_nextGitHubRequestId;
            if (FAILED(_github.Submit(std::move(req))))
                Status(IDS_BUSY);
            return;
        }
        case Command::FilesCtxOpen:
        case Command::FilesCtxRevealExplorer:
        case Command::FilesCtxRevealRedSalamander:
        case Command::FilesCtxCopyFullPath:
        case Command::FilesCtxCopyRelPath:
        case Command::FilesCtxRename:
        case Command::FilesCtxCut:
        case Command::FilesCtxCopy:
        case Command::FilesCtxPaste:
        case Command::FilesCtxRemoveRoot:
        case Command::FilesCtxRemoveRepo: ExecuteFilesContextAction(command); return;
    }
    auto* const activeEditor = ActiveEditor();
    if (semantic)
    {
        const auto result = activeEditor->ExecuteMarkdownCommand(request);
        if (result == MarkdownCommandStatus::Unavailable)
        {
            if (IsMarkdownCommandCapacityExceeded(*activeEditor, request, activeEditor->GetSelection()))
                ReportCapacityLimit();
            else
                Status(IDS_NOTAVAILABLE);
        }
        else
            Status(IDS_FORMATTED);
    }
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    _host.SetFocusControl(activeEditor, MayRequestNativeFocus());
}

void Application::LocalSource()
{
    auto* const activeEditor = ActiveEditor();
    if (activeEditor->GetSemanticSourceRegion())
    {
        if (activeEditor->EndSemanticSourceEdit(_host))
            SelectionChanged();
        return;
    }
    const auto semantic  = activeEditor->GetAcknowledgedSemanticDocument(_host);
    const auto selection = activeEditor->GetSelection();
    const auto source    = activeEditor->GetText();
    if (semantic)
    {
        const auto elements = semantic->Elements();
        for (std::size_t index = elements.size(); index > 0; --index)
        {
            const auto& element = elements[index - 1];
            // Code, diagrams and math edit their exact source in place. A display equation
            // ends at its closing delimiter, so the following line is not inside it.
            const auto elementLast = element.role == DxUi::RichText::SemanticRole::Math ? WithoutLineEnd(source, element.source) : element.source.last;
            if (selection.caret >= element.source.first && selection.caret <= elementLast && activeEditor->BeginSemanticSourceEdit(_host, semantic, index - 1))
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
    if (const auto document = activeEditor->GetSemanticDocument())
        for (const auto& element : document->Elements())
            if (element.role == DxUi::RichText::SemanticRole::Math && selection.caret >= element.source.first &&
                selection.caret <= WithoutLineEnd(source, element.source))
            {
                first = element.source.first;
                last  = WithoutLineEnd(source, element.source);
                break;
            }
    if (first == last)
        if (const auto syntax = activeEditor->GetMarkdownSyntax())
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
    _blockDraftSession = BlockDraftSession{activeEditor->Snapshot(), activeEditor->Revision(), {first, last}, false};
    _blockDraftText->SetText(source.substr(first, last - first));
    if (_blockDraftTitle)
    {
        const auto title = Resource(_equationContext ? IDS_BLOCKEQUATION : IDS_LOCAL);
        _blockDraftTitle->SetAccessibleName(title);
        _blockDraftTitle->SetText(title);
    }
    SetActiveOverlay(OverlayKind::BlockDraftModal);
    activeEditor->SetReadOnly(true);
    if (last > first)
    {
        const DxUi::RichTextSourceHighlight block{first, last - first, true};
        (void)AcceptSourceResult(activeEditor->TrySetSourceHighlights(std::span(&block, 1)));
    }
    Layout();
    _host.SetFocusControl(_blockDraftText, MayRequestNativeFocus());
    CheckpointRecovery();
}

void Application::ApplyBlockSource()
{
    if (! HasBlockDraft())
        return;
    auto* const activeEditor = ActiveEditor();
    const auto& draftSession = _blockDraftSession.value();
    if (! activeEditor->Snapshot().IsSameRevision(draftSession.snapshot) || activeEditor->Revision() != draftSession.revision)
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
    activeEditor->SetReadOnly(false);
    if (! activeEditor->ApplySourceEdits(draftSession.snapshot, draftSession.revision, std::span(&edit, 1), {end, end}))
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
    auto add = [&](unsigned label, Command command, unsigned shortcut = 0u)
    {
        DxUi::MenuFlyoutItem item;
        item.text            = Resource(label);
        item.commandId       = static_cast<int>(command) + 1;
        item.acceleratorText = shortcut ? Resource(shortcut) : std::wstring{};
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
            add(IDS_BOLD, Command::Bold, IDS_SHORTCUT_BOLD);
            add(IDS_ITALIC, Command::Italic, IDS_SHORTCUT_ITALIC);
            add(IDS_STRIKE, Command::Strike);
            add(IDS_CODE, Command::Code, IDS_SHORTCUT_CODE);
            add(IDS_NUMBERED, Command::Numbered);
            add(IDS_LOCAL, Command::LocalSource, IDS_SHORTCUT_LOCAL);
            if (CurrentLinkElement(ActiveEditor()->GetSemanticDocument()))
                add(IDS_OPENLINK, Command::OpenLink);
            add(IDS_ROWADD, Command::RowAdd);
            add(IDS_ROWREMOVE, Command::RowRemove);
            add(IDS_COLADD, Command::ColumnAdd);
            add(IDS_COLREMOVE, Command::ColumnRemove);
        }
    }
    if (! headingOnly)
    {
        add(IDS_LINK, Command::Link, IDS_SHORTCUT_LINK);
        add(IDS_IMAGE, Command::Image);
        add(IDS_TABLE, Command::Table);
        add(IDS_MERMAID, Command::Mermaid);
        add(IDS_MATH, Command::Math);
        add(IDS_QUOTE, Command::Quote);
        add(IDS_FENCE, Command::Fence);
    }
    return items;
}

void Application::Menu(POINT point, bool insertOnly, bool headingOnly, bool keyboardInvocation)
{
    if (_options.headless || HasActiveModalUi())
        return; // Focus-taking menus are an explicitly interactive gate.
    auto items = ContextMenuItems(insertOnly, headingOnly);
    if (! insertOnly && ! headingOnly)
        PopulateSpellContextMenu(point, keyboardInvocation, items);
    auto* const primary  = ActiveEditor();
    const auto source    = primary->Snapshot();
    const auto revision  = primary->Revision();
    const auto selection = primary->GetSelection();
    const auto key       = _session.key;
    const auto lifetime  = GetControlLifetimeToken(*primary);
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SetActiveOverlay(OverlayKind::None);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(_window.get(),
                                       point,
                                       items,
                                       _host.GetTheme(),
                                       [this, primary, source, revision, selection, key, lifetime](std::optional<int> selected) noexcept
    {
        RunUiAction([&]
        {
            if (selected)
            {
                if (HandleSpellMenuSelection(*selected, key, source, revision, selection))
                {
                    OnMenuClosed(std::nullopt);
                    return;
                }
            }
            if (selected == static_cast<int>(Command::OpenLink) + 1)
            {
                if (! IsUiAttached() || lifetime.expired() || ActiveEditor() != primary)
                    return;
                if (_session.key != key || ! primary->Snapshot().IsSameRevision(source) || primary->GetSelection() != selection)
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

void Application::OnMenuClosed(std::optional<int> selected, DxUi::Control* restoreFocusControl) noexcept
{
    RunUiAction([&]
    {
        if (IsClosing() || IsSessionEnding() || ! _root || _host.GetRoot() != _root)
            return;
        if (selected)
            Execute(static_cast<Command>(selected.value() - 1));
        // Commands may retire the root or open a draft/drawer that owns focus.
        if (CanAcceptCommand() && ! _printPreview && (_activeOverlay == OverlayKind::None || _activeOverlay == OverlayKind::FloatingSelectionTools))
            _host.SetFocusControl(restoreFocusControl ? restoreFocusControl : ActiveEditor(), MayRequestNativeFocus());
    });
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
void Application::BeginClose(bool preserveSessionResume)
{
    Printing::Stop(_printOperation);
    ClosePrintPreview();
    if (_window)
    {
        (void)KillTimer(_window.get(), SettingsRetryTimerId);
        (void)KillTimer(_window.get(), SpellDebounceTimerId);
        (void)KillTimer(_window.get(), GitHubDevicePollTimerId);
    }
    _spellService.Stop();
    _fileOps.Stop();
    ClearPendingDeviceFlow();
    ClearGitHubTokenInputs();
    _github.Stop();
    _saveConfirmSession.reset();
    _renameSession.reset();
    _fileConflictSession.reset();
    _activeFileTx = std::monostate{};
    _pendingPasteQueue.clear();
    _deferredFilesContextMenu.reset();
    _filesContextMenuOpen = false;
    AbandonStartupRestore();
    RememberActiveDocument();
    RecordWindowPlacement(true);
    CloseExternalLinks();
    if (preserveSessionResume)
    {
        CheckpointWorkspaceRecovery(RecoverySnapshotKind::SessionResume);
        ResetWorkspace(false, true);
    }
    else
    {
        ResetWorkspace(true, true);
    }
    _recovery.Stop();
    TransitionLifecycle(LifecyclePhase::DrainingForClose);
    _pendingBitmapPaste.reset();
    _bitmapWriter.Stop();
    _settingsWatcher.Close();
    _files.Close();
    _folderFiles.Close();
    _multiFolderFiles.Close();
    _images.Shutdown();
    _imageFiles.Close();
    _host.Detach();
    if (! AreShutdownWorkersSettled())
    {
        // Keep only a hidden notification window until admitted preferences/recovery
        // are durable. The UI never joins storage, and process exit cannot kill the
        // worker between its final write and atomic replacement.
        ShowWindow(_window.get(), SW_HIDE);
    }
    else
    {
        _recovery.Close();
        _preferenceWriter.Close();
        _fileOps.Close();
        _github.Close();
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
        _fileOps.Close();
        _github.Close();
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
            CheckpointWorkspaceRecovery(RecoverySnapshotKind::SessionResume);
        }
        return TRUE; // Honor shutdown immediately; never create a shutdown blocker.
    }
    if (message == WM_ENDSESSION)
    {
        if (wParam != FALSE)
        {
            _sessionEndCommitted = true;
            if (_window)
            {
                (void)KillTimer(_window.get(), SettingsRetryTimerId);
                (void)KillTimer(_window.get(), SpellDebounceTimerId);
                (void)KillTimer(_window.get(), GitHubDevicePollTimerId);
            }
            _spellService.Stop();
            _fileOps.Stop();
            ClearPendingDeviceFlow();
            ClearGitHubTokenInputs();
            _github.Stop();
            _pendingPasteQueue.clear();
            _deferredFilesContextMenu.reset();
            _filesContextMenuOpen = false;
            if (_lifecyclePhase == LifecyclePhase::Active || _lifecyclePhase == LifecyclePhase::Initializing ||
                _lifecyclePhase == LifecyclePhase::ClosingAfterSave)
            {
                _phaseBeforeSessionEnd = _lifecyclePhase;
                TransitionLifecycle(LifecyclePhase::SessionEnding);
            }
            Printing::Stop(_printOperation);
            ClosePrintPreview();
            _pendingBitmapPaste.reset();
            _bitmapWriter.Stop();
            _settingsWatcher.Close();
            if (! IsClosing())
                CheckpointWorkspaceRecovery(RecoverySnapshotKind::SessionResume);
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
                TransitionLifecycle(_phaseBeforeSessionEnd);
        }
        return 0;
    }
    if (message == WM_CLOSE)
    {
        if (IsClosing() || IsClosingAfterSave() || IsSessionEnding() || HasActiveModalPrompt() || IsModalOverlay() ||
            (_saveConfirmSession && _saveConfirmSession->sequentialSaveActive) || _switchingDocument)
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
        AskWorkspaceThen([this] { BeginClose(false); }, {}, true);
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
        if (_window)
        {
            (void)KillTimer(_window.get(), SettingsRetryTimerId);
            (void)KillTimer(_window.get(), SpellDebounceTimerId);
            (void)KillTimer(_window.get(), GitHubDevicePollTimerId);
        }
        _spellService.Stop();
        _fileOps.Close();
        ClearPendingDeviceFlow();
        _github.Close();
        Printing::Stop(_printOperation);
        ClosePrintPreview();
        _bitmapWriter.Stop();
        _bitmapWriter.Detach();
        _settingsWatcher.Close();
        _folderFiles.Close();
        _multiFolderFiles.Close();
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
        else if (! IsClosing() && ! IsSessionEnding())
        {
            if (FAILED(_preferenceWriter.Status()))
                Status(IDS_SETTINGSFAILED);
            if (_settingsReloadDeferred && ! _preferenceWriter.Busy() && ! IsClosingAfterSave())
            {
                _settingsReloadDeferred = false;
                ReloadSettingsFromDisk();
            }
        }
        return 0;
    }
    if (message == RecoveryReadyMessage)
    {
        RecoveryCompleted();
        return 0;
    }
    if (message == SpellReady)
    {
        SpellCompleted();
        return 0;
    }
    if (message == FileOpReady)
    {
        FileOpCompleted();
        return 0;
    }
    if (message == GitHubReady)
    {
        GitHubCompleted();
        return 0;
    }
    if (IsClosing())
        return DefWindowProcW(hwnd, message, wParam, lParam);
    if (message == SpellMenuRequested)
    {
        HandlePostedSpellLeftClickMenu();
        return 0;
    }
    if (message == FilesContextMenuRequested)
    {
        const auto pending = std::move(_deferredFilesContextMenu);
        _deferredFilesContextMenu.reset();
        if (pending)
            ShowFilesContextMenuNow(pending->target, pending->screenPoint, pending->keyboardInvocation);
        return 0;
    }
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
    if (message == WM_TIMER && wParam == SettingsRetryTimerId)
    {
        (void)KillTimer(hwnd, SettingsRetryTimerId);
        if (! IsClosing() && ! IsClosingAfterSave() && ! IsSessionEnding())
            ReloadSettingsFromDisk();
        return 0;
    }
    if (message == WM_TIMER && wParam == SpellDebounceTimerId)
    {
        (void)KillTimer(hwnd, SpellDebounceTimerId);
        if (! IsClosing() && ! IsClosingAfterSave() && ! IsSessionEnding())
            OnSpellDebounceTimer();
        return 0;
    }
    if (message == WM_TIMER && wParam == GitHubDevicePollTimerId)
    {
        (void)KillTimer(hwnd, GitHubDevicePollTimerId);
        if (! IsClosing() && ! IsClosingAfterSave() && ! IsSessionEnding() && _pendingDeviceFlow)
        {
            if (GetTickCount64() >= _pendingDeviceFlow->deadlineTickMs)
            {
                ClearPendingDeviceFlow();
                if (_openGitHubErrorLabel && _activeOverlay == OverlayKind::OpenGitHubModal)
                    _openGitHubErrorLabel->SetText(Resource(IDS_GITHUB_ERR_DEVICE_EXPIRED));
                Status(IDS_GITHUB_ERR_DEVICE_EXPIRED);
                RefreshGitHubAuthControls();
                if (_activeOverlay == OverlayKind::SettingsDrawer || _activeOverlay == OverlayKind::OpenGitHubModal)
                    Layout();
                return 0;
            }
            GitHubRequest poll;
            poll.kind      = GitHubRequest::Kind::PollDeviceFlow;
            poll.requestId = ++_nextGitHubRequestId;
            poll.flowId    = _pendingDeviceFlow->flowId;
            poll.tokenOrDeviceCode.Assign(_pendingDeviceFlow->info.deviceCode);
            (void)_github.Submit(std::move(poll));
        }
        return 0;
    }
    if (message == SettingsChanged)
    {
        _settingsWatcher.Acknowledge();
        _settingsReloadRetries = 0u;
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
        const auto drop = reinterpret_cast<HDROP>(wParam);
        auto finish     = wil::scope_exit([&] { DragFinish(drop); });
        if (! CanAcceptCommand() || HasActiveModalUi() || _files.Busy() || _printPreview)
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
        finish.release();
        DragFinish(drop);
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
        if (! HasActiveModalUi() && (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0 && ! KeyboardState::Held(VK_MENU))
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
    if (message == WM_KEYDOWN && wParam == VK_TAB && HandleModalTabKey())
        return 0;
    if (message == WM_MOUSEMOVE)
    {
        const float scale = 96.f / _host.GetDpi();
        UpdateHoverOverlayPointer({static_cast<float>(GET_X_LPARAM(lParam)) * scale, static_cast<float>(GET_Y_LPARAM(lParam)) * scale}, true);
    }
    else if (message == WM_MOUSELEAVE)
    {
        UpdateHoverOverlayPointer({-1.f, -1.f}, false);
    }
    else if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
    {
        const float scale             = 96.f / _host.GetDpi();
        const D2D1_POINT_2F clientDip = {static_cast<float>(GET_X_LPARAM(lParam)) * scale, static_cast<float>(GET_Y_LPARAM(lParam)) * scale};
        const auto inRect             = [](const DxUi::Control* c, D2D1_POINT_2F pt) noexcept
        {
            if (! c || ! c->IsVisible())
                return false;
            const auto r = c->GetBounds();
            return pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom;
        };
        if ((IsPaneOverlayActive(PaneSide::Left) && ! inRect(_filesPane, clientDip) && ! inRect(_leftRail, clientDip)) ||
            (IsPaneOverlayActive(PaneSide::Right) && ! inRect(_sidebar, clientDip) && ! inRect(_rightRail, clientDip)))
            DismissHoverOverlays(true);
        if (message == WM_LBUTTONDOWN)
            OnEditorLeftButtonDown(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}, wParam);
    }
    else if (message == WM_LBUTTONDBLCLK)
    {
        OnEditorLeftButtonDoubleClick();
    }
    bool handled{};
    const auto result = _host.HandleMessage(hwnd, message, wParam, lParam, handled);
    if (! IsClosing())
        ReconcileSpellImeExit();
    if (message == WM_LBUTTONUP)
        OnEditorLeftButtonUp(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
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
        const auto semantic     = _editor ? _editor->GetSemanticDocument() : nullptr;
        const auto acknowledged = (_editor && semantic) ? _editor->GetAcknowledgedSemanticDocument(_host) : nullptr;
        const bool richVisible  = _editor && _editor->GetViewMode() == DxUi::RichText::ViewMode::Rich && _editor->IsVisible() && semantic != nullptr;
        if (_imageFiles.HasResult() || _images.HasActiveOrResults() || _imageWatchDocumentPath != _session.path || _imageConsentPath != _session.path ||
            semantic != _imageConsentSource || (semantic && semantic != _imageWatchSemantic) || acknowledged != _imageWatchAcknowledgedSemantic ||
            richVisible != _imageWatchWasRichVisible)
            ServiceImages();
        // Published syntax or deferred placement can arrive after a committed source/selection event.
        // Observe immutable identity and defer overlay/layout reconciliation out of WM_PAINT.
        const bool placementPending =
            ! IsClosing() && (_pendingSelectionTools || (_blockDraftSession && _blockDraftSession->pendingPlacement) ||
                              ActiveEditor()->GetMarkdownSyntax() != _derivedCaches.selectionFactsSyntax || _pendingComposeScrollRestore.has_value() ||
                              _pendingSourceScrollRestore.has_value() || _pendingSplitScroll.has_value() || _spell.immediatePending);
        if (placementPending && ! _postPreparePosted)
            _postPreparePosted = PostMessageW(hwnd, PostPreparePlacement, 0, 0) != FALSE;
    }
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}

bool Application::ReconcilePostPreparePlacement()
{
    if (IsClosing() || ! _host.GetRoot())
        return false;
    bool scrolled{};
    if (_pendingComposeScrollRestore && _editor && _editor->IsVisible())
    {
        const float offset = _pendingComposeScrollRestore.value();
        _pendingComposeScrollRestore.reset();
        _splitScrollSyncInProgress = true;
        const auto reset           = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
        scrolled                   = _editor->SetScrollOffsetDip(offset) || scrolled;
    }
    if (_pendingSourceScrollRestore && _mirror && _mirror->IsVisible())
    {
        const float offset = _pendingSourceScrollRestore.value();
        _pendingSourceScrollRestore.reset();
        _splitScrollSyncInProgress = true;
        const auto reset           = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
        scrolled                   = _mirror->SetScrollOffsetDip(offset) || scrolled;
    }
    if (_pendingSplitScroll)
    {
        const auto pending = std::exchange(_pendingSplitScroll, std::nullopt);
        if (_settings.view == EditorView::Split && _settings.splitScrollSync)
        {
            ApplySplitScrollSync(pending->source, pending->scrollDip, false);
            scrolled = true;
        }
    }
    const bool refreshFacts     = _pendingSelectionTools || ActiveEditor()->GetMarkdownSyntax() != _derivedCaches.selectionFactsSyntax;
    const bool pendingPlacement = _blockDraftSession && _blockDraftSession->pendingPlacement;
    if (refreshFacts)
        SelectionChanged(_pendingSelectionTools || _activeOverlay == OverlayKind::FloatingSelectionTools);
    if (pendingPlacement)
        Layout();
    bool spellDispatched{};
    if (_spell.immediatePending)
    {
        DispatchSpellCheckNow();
        spellDispatched = ! _spell.immediatePending;
    }
    return refreshFacts || pendingPlacement || scrolled || spellDispatched;
}

bool Application::RouteShortcut(const MSG& message)
{
    bool shortcut{};
    if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) && (message.hwnd == _window.get() || IsChild(_window.get(), message.hwnd)))
    {
        const bool ctrl = KeyboardState::Held(VK_CONTROL), shift = KeyboardState::Held(VK_SHIFT), alt = KeyboardState::Held(VK_MENU);
        if (message.message == WM_KEYDOWN && message.wParam == VK_TAB && HandleModalTabKey())
            return true;
        if (HasActiveModalUi())
        {
            if (message.wParam == VK_F4 && alt && ! ctrl)
                return true;
            return false;
        }
        std::optional<Command> command;
        if (ctrl && ! alt)
        {
            switch (message.wParam)
            {
                case 'N': command = Command::New; break;
                case 'O': command = shift ? Command::OpenFolder : Command::Open; break;
                case 'G':
                    if (shift)
                        command = Command::OpenGitHub;
                    break;
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
                case 'L':
                    if (shift)
                        command = Command::Outline;
                    break;
                case 'Y':
                    if (shift)
                        command = Command::ToggleSplitScrollSync;
                    break;
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
        if (ctrl && ! alt && ! shift && _filesTree && _host.GetFocusControl() == _filesTree &&
            (message.wParam == 'X' || message.wParam == 'C' || message.wParam == 'V'))
        {
            const auto selectedId = _filesTree->GetSelectedItemId();
            if (selectedId)
            {
                const auto it = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == selectedId.value(); });
                if (it != _filesRowBindings.end())
                {
                    _pendingContextTarget = it->second;
                    if (message.wParam == 'X')
                        command = Command::FilesCtxCut;
                    else if (message.wParam == 'C')
                        command = Command::FilesCtxCopy;
                    else
                        command = Command::FilesCtxPaste;
                }
            }
        }
        if (((message.wParam == VK_F10 && shift && ! ctrl && ! alt) || (message.wParam == VK_APPS && ! ctrl && ! shift && ! alt)) && _filesTree &&
            _host.GetFocusControl() == _filesTree)
        {
            if (const auto selectedId = _filesTree->GetSelectedItemId())
            {
                const auto it = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == selectedId.value(); });
                if (it != _filesRowBindings.end())
                {
                    const auto bounds = _filesTree->GetBounds();
                    ShowFilesContextMenuNow(it->second, _host.DipPointToScreenPoint({(bounds.left + bounds.right) * 0.5f, bounds.top + 32.f}), true);
                }
                return true;
            }
        }
        if (message.wParam == VK_F3 && ! ctrl && ! alt)
            command = shift ? Command::FindPrevious : Command::FindNext;
        if (message.wParam == VK_F2 && ! ctrl && ! shift && ! alt)
        {
            if (_filesTree && _host.GetFocusControl() == _filesTree)
            {
                if (const auto selectedId = _filesTree->GetSelectedItemId())
                {
                    const auto it = std::ranges::find_if(_filesRowBindings, [&](const auto& pair) { return pair.first == selectedId.value(); });
                    if (it != _filesRowBindings.end() &&
                        (it->second.kind == FilesRowContextBinding::Kind::LocalEntry ||
                         (it->second.kind == FilesRowContextBinding::Kind::WorkspaceDoc && it->second.githubOwner.empty() && ! it->second.localPath.empty())))
                    {
                        _pendingContextTarget = it->second;
                        Execute(Command::FilesCtxRename);
                    }
                }
                return true;
            }
            command = Command::LocalSource;
        }
        if (message.wParam == VK_F6 && ! ctrl && ! shift && ! alt)
            command = Command::SwitchSplitPane;
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
        if (! IsClosing())
            ReconcileSpellImeExit();
        if (_lifecyclePhase == LifecyclePhase::DrainingForClose && AreShutdownWorkersSettled())
        {
#if defined(ENABLE_TESTS)
            ++_preferencesReadySentDuringClose;
#endif
            (void)SendMessageW(_window.get(), PreferencesReady, 0, 0);
        }
        if (! IsClosing() && _files.Busy())
            FileCompleted();
    });
}

void Application::RefreshLeftRailDots()
{
    if (! _leftRail)
        return;
    struct DirtyEntry
    {
        unsigned key{};
        std::filesystem::path path;
        DocumentOriginInfo origin;
        bool active{};
    };
    std::vector<DirtyEntry> dirtyEntries;
    std::vector<std::wstring> baseNames;
    auto resolveBase = [&](const std::filesystem::path& path, const DocumentOriginInfo& origin)
    {
        if (origin.kind == DocumentOriginInfo::Kind::GitHub && ! origin.relativePath.empty())
            return SanitizeUiText(std::filesystem::path(origin.relativePath).filename().native());
        return path.empty() ? Resource(IDS_UNTITLED) : SanitizeUiText(path.filename().native());
    };
    baseNames.push_back(resolveBase(_session.path, _session.origin));
    if (Dirty())
        dirtyEntries.push_back({_session.key, _session.path, _session.origin, true});
    for (const auto& doc : _inactiveDocuments)
    {
        baseNames.push_back(resolveBase(doc.metadata.path, doc.metadata.origin));
        if (doc.dirty)
            dirtyEntries.push_back({doc.metadata.key, doc.metadata.path, doc.metadata.origin, false});
    }
    std::ranges::sort(dirtyEntries, {}, &DirtyEntry::key);

    const std::wstring editedWord = Resource(IDS_FILESEDITED);
    std::vector<RailDotItem> items;
    items.reserve(dirtyEntries.size());
    for (const auto& entry : dirtyEntries)
    {
        const std::wstring base = resolveBase(entry.path, entry.origin);
        const auto duplicates   = std::ranges::count(baseNames, base);
        std::wstring label      = base;
        if (duplicates > 1)
        {
            if (entry.origin.kind == DocumentOriginInfo::Kind::GitHub && ! entry.origin.owner.empty() && ! entry.origin.repo.empty())
            {
                label = SanitizeUiText(entry.origin.owner + L"/" + entry.origin.repo) + L"/" + base;
            }
            else if (! entry.path.empty())
            {
                const auto parent = SanitizeUiText(entry.path.parent_path().filename().native());
                if (! parent.empty())
                    label = parent + L"/" + base;
            }
        }
        RailDotItem item;
        item.id         = entry.key;
        item.level      = 1u;
        item.active     = entry.active;
        item.name       = label;
        item.itemStatus = editedWord;
        item.tooltip    = FormatStringResource(nullptr, IDS_RAIL_FILE_TOOLTIP, label, editedWord);
        items.push_back(std::move(item));
    }
    _leftRail->SetItems(std::move(items), true);
}

void Application::RefreshRightRailDots()
{
    if (! _rightRail)
        return;
    const bool findActive = _activeOverlay == OverlayKind::FindBar;
    std::vector<RailDotItem> items;
    items.reserve(_documentInfo.headings.size());
    for (std::size_t i = 0; i < _documentInfo.headings.size(); ++i)
    {
        const auto& heading = _documentInfo.headings[i];
        std::wstring cleaned;
        cleaned.reserve(std::min<std::size_t>(heading.text.size(), 80u));
        bool lastSpace = false;
        for (const wchar_t ch : heading.text)
        {
            if (ch == L'\r' || ch == L'\n' || ch == L'\t' || ch == L' ')
            {
                if (! cleaned.empty() && ! lastSpace)
                {
                    cleaned.push_back(L' ');
                    lastSpace = true;
                }
            }
            else
            {
                cleaned.push_back(ch);
                lastSpace = false;
            }
            if (cleaned.size() >= 82u)
                break;
        }
        while (! cleaned.empty() && cleaned.back() == L' ')
            cleaned.pop_back();
        if (cleaned.size() > 80u)
        {
            cleaned.resize(79u);
            if (! cleaned.empty() && IS_HIGH_SURROGATE(cleaned.back()))
                cleaned.pop_back();
            cleaned.push_back(L'\x2026');
        }
        cleaned = SanitizeUiText(cleaned, 80u);
        RailDotItem item;
        item.id             = i;
        item.level          = heading.level;
        item.active         = _currentHeading && _currentHeading.value() == i;
        item.findMatchCount = (findActive && i < _find.projections.sectionCounts.size()) ? _find.projections.sectionCounts[i] : 0u;
        item.name           = std::move(cleaned);
        item.tooltip        = item.findMatchCount > 0u ? FormatStringResource(nullptr, IDS_RAIL_HEADING_MATCHES, item.name, item.findMatchCount)
                                                       : FormatStringResource(nullptr, IDS_RAIL_HEADING_TOOLTIP, item.name, item.level);
        items.push_back(std::move(item));
    }
    _rightRail->SetItems(std::move(items), true);
}
} // namespace RedTilde
