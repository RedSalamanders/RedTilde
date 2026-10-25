#include "Application.h"
#include "Common/CrashHandler.h"
#include "res/Resource.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <DxUi/TextInputServices.h>
#include <algorithm>
#include <chrono>
#include <commdlg.h>
#include <dwmapi.h>
#include <format>
#include <fstream>
#include <new>
#include <psapi.h>
#include <shobjidl.h>
#include <thread>
#include <wil/result.h>
#include <wincodec.h>
#include <windowsx.h>

namespace RedTilde
{
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
} // namespace

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
           L"$$ x = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a} $$\n\n"
           L"## Keep the details\n\n"
           L"| View | Best for |\n| --- | --- |\n| Compose | Staying in your writing |\n| Source | Every Markdown detail |\n| Split | Both at a "
           L"glance |\n\n"
           L"> Simple tools. Thoughtful defaults. Your files stay yours.\n\n"
           L"Images use ordinary Markdown: `![description](images/photo.png)`. Insert a local image from the toolbar or paste a file path.\n";
}

Application::Application(LaunchOptions value) : options(std::move(value)) {}
Application::~Application()
{
    closing = true;
    preferenceWriter.Close();
    recovery.Close();
    files.Close();
    folderFiles.Close();
    images.Shutdown();
    host.Detach();
    window.reset();
}

HRESULT Application::Initialize()
{
    if (options.dataDirectory.empty())
        options.dataDirectory = DefaultDataDirectory();
    if (!options.headless)
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
    window.reset(CreateWindowExW(options.headless ? WS_EX_NOACTIVATE : 0, cls.lpszClassName, Resource(IDS_APP).c_str(), WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top, nullptr, nullptr,
                                 cls.hInstance, this));
    if (!window)
        return HRESULT_FROM_WIN32(GetLastError());
    if (!host.Attach(window.get(), {.forceWarp = options.forceWarp, .batchInitialRootChildren = true}))
        return HRESULT_FROM_WIN32(GetLastError());
    files.Initialize(window.get(), FileReady);
    const MARGINS frame{1, 1, 1, 1};
    (void)DwmExtendFrameIntoClientArea(window.get(), &frame);
    SetWindowPos(window.get(), nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    preferenceWriter.Initialize(options.dataDirectory, window.get(), PreferencesReady);
    RETURN_IF_FAILED(recovery.Initialize(options.dataDirectory, window.get(), RecoveryReadyMessage));
    RETURN_IF_FAILED(images.Initialize(window.get(), ImageReady, 0x5254));
    initialized = true;
    initializing = true;
    // BuildUi is the sole creation site for the label whose localized facts we cache.
    currentBlockPublishedFacts.reset();
    BuildUi();
    ApplyTheme();
    SetDocument(SampleDocument());
    if (!options.document.empty())
    {
        if (options.headless)
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
    return S_OK;
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
        theme.selectionText = D2D1::ColorF(dark ? 0xFFFFFF : 0xA12A34);
        theme.focusStroke = theme.accent;
        theme.windowBackground = D2D1::ColorF(dark ? 0x17191E : settings.appearance == Appearance::Paper ? 0xEDE6D8 : 0xF0F1F4);
        theme.surfaceBackground = D2D1::ColorF(dark ? 0x1C1F25 : settings.appearance == Appearance::Paper ? 0xF3EDDF : 0xF8F8FA);
        theme.cardBackground = theme.inputFill = D2D1::ColorF(dark ? 0x22252B : settings.appearance == Appearance::Paper ? 0xFFFAEF : 0xFFFFFF);
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
        target->SetLinkUnderlinePolicy(underline);
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
        nextInfo.emplace(AnalyzeDocument(*text, documentInfo.lines));
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
        documentInfo = std::move(*nextInfo);
        documentAnalysisSource = source;
        ++activity.documentAnalysisUpdates;
    }
    if (outlineRowsChanged || collapseStateChanged)
        RefreshOutline();
}
void Application::SynchronizeMirror()
{
    if (settings.view != EditorView::Split)
        return;
    const auto source = editor->Snapshot();
    if (!mirroredSource || !mirroredSource->IsSameRevision(source))
    {
        mirror->SetText(CurrentText());
        mirroredSource = source;
        ++activity.mirrorResets;
    }
}
void Application::Status(unsigned id)
{
    if (feedback)
        feedback->SetText(Resource(id));
}
void Application::SetDocument(std::wstring text, std::filesystem::path path, bool hasBom, FileStamp identity)
{
    recovered = false;
    editor->SetText(std::move(text));
    documentPath = std::move(path);
    bom = hasBom;
    stamp = identity;
    savedText = editor->GetText();
    dirtyFact.reset();
    editor->SetDocumentFilePath(documentPath.native());
    SetView(settings.view);
    Changed();
    RefreshFiles(true);
}
void Application::Changed()
{
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Enter);
    ++activity.changes;
    const auto source = editor->Snapshot();
    RefreshDocumentAnalysis(source, true);
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Analysis);
    SynchronizeMirror();
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
    if (SendMessageW(window.get(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(caption.c_str())) == TRUE && request == activity.captionRequests)
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
    bool changedDraft{};
    if (HasBlockDraft() && editor->Snapshot().IsSameRevision(blockDraftSnapshot) && editor->Revision() == blockDraftRevision)
    {
        draft = blockDraftText->GetText();
        const std::wstring_view source = CurrentText();
        if (draft != source.substr(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first))
        {
            if (!DxUi::RichText::IsValidUtf16(draft) ||
                source.size() - (blockDraftRange.last - blockDraftRange.first) + draft.size() > DxUi::RichText::Document::MaximumUnits)
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            changedDraft = true;
            needsRecovery = true;
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
    if (FAILED(recovery.Submit(std::move(snapshot), flush)))
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
        if (!recoveryCandidates.empty() && (!options.headless || options.recoveryRestartProbe || options.scenario == L"recovery"))
            ShowRecovery();
    }
}
void Application::ShowRecovery()
{
    if (recoveryCandidates.empty())
    {
        recoveryPanel->SetVisible(false);
        Status(IDS_NORECOVERY);
        host.SetFocusControl(editor, !options.headless);
        return;
    }
    CloseDrawers();
    const auto &snapshot = recoveryCandidates.front().snapshot;
    const auto path = snapshot.originalPath.empty() ? Resource(IDS_UNTITLED) : snapshot.originalPath.native();
    recoveryDescription->SetText(std::vformat(Resource(IDS_RECOVERYHELP), std::make_wformat_args(path)));
    recoveryPreview->SetText(snapshot.text);
    recoveryPanel->SetVisible(true);
    host.SetFocusControl(recoveryPanel->GetLogicalChild(3), !options.headless);
    Layout();
}
void Application::SetView(EditorView view)
{
    selectionToolsFocus.reset();
    // The same editor/document/history survives all global presentation changes.
    settings.view = view;
    editor->SetViewMode(view == EditorView::Source ? DxUi::RichText::ViewMode::Source : DxUi::RichText::ViewMode::Rich);
    SynchronizeMirror();
    modeHint->SetText(Resource(view == EditorView::Source ? IDS_SOURCEHINT : view == EditorView::Split ? IDS_SPLITHINT : IDS_BANNER));
    floating->SetVisible(false);
    Layout();
    SelectionChanged();
    host.SetFocusControl(editor, false);
    if (!initializing)
        SavePreferences();
}
void Application::SelectionChanged(bool refreshTools)
{
    EditDiagnostics::SelectionInvocation selectionInvocation(editDiagnostics);
    if (closing || !floating)
        return;
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
            SynchronizeMirror();
            mirror->SetSelection(selection);
            (void)mirror->ScrollSourceRangeIntoView(host, selection.Start(), selection.End(), false);
            ++activity.mirrorSelectionUpdates;
        }
        std::wstring expression;
        std::array<bool, 4> formats{};
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::InlineSemantics);
            if (syntax)
                for (const auto &node : syntax->Inlines())
                    if (selection.caret >= node.source.first && selection.caret <= node.source.last)
                    {
                        using Kind = DxUi::RichText::MarkdownInlineKind;
                        if (node.kind == Kind::Math && node.value.size() <= DxUi::RichText::MathMaximumSourceUnits)
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
        }
        bool layoutChanged{};
        {
            EditDiagnostics::SelectionMeasure measure(editDiagnostics, SelectionComponent::ToolCaptionPublication);
            const auto preview = expression.empty() ? std::wstring{} : L"\\[" + expression + L"\\]";
            if (equation->GetText() != preview)
                equation->SetText(preview);
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
            // Model refresh already reconciles and reveals surviving focused rows.
            // Republish only an actual selection or focus transition.
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
            const CurrentBlockLabelFacts blockFacts{blockResourceId, count};
            if (!currentBlockPublishedFacts || currentBlockPublishedFacts.value() != blockFacts)
            {
                const auto blockType = ResourceView(blockResourceId);
                auto text = std::vformat(ResourceView(IDS_BLOCKINFO), std::make_wformat_args(blockType, count));
                const auto request = ++activity.currentBlockLabelConstructions;
                currentBlockPublishedFacts.reset();
                currentBlock->SetText(std::move(text));
                // A nested publication owns its newer facts, just as a nested caption request does.
                if (activity.currentBlockLabelConstructions == request)
                    currentBlockPublishedFacts = blockFacts;
            }
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
        host.SetFocusControl(floatingActions.front(), !options.headless);
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
        editor->SetSourceHighlights({});
    RefreshFind();
    Layout();
    host.SetFocusControl(editor, !options.headless);
    if (!initializing && !closing)
        CheckpointRecovery();
}

void Application::Execute(Command command)
{
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
    MarkdownCommandRequest request;
    bool semantic = true;
    switch (command)
    {
    case Command::FileMenu:
        FileMenu();
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
        host.SetFocusControl(settings.showFiles ? static_cast<DxUi::Control *>(filesTree) : editor, !options.headless);
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
        editor->SetRemoteImagesEnabled(settings.allowRemoteImages);
        images.DiscardCancelled();
        remoteImagesToggle->SetToggleState(settings.allowRemoteImages);
        ServiceImages();
        SavePreferences();
        return;
    case Command::DimBlocks:
        settings.dimInactiveBlocks = !settings.dimInactiveBlocks;
        Layout();
        SavePreferences();
        return;
    case Command::Recovery:
        ShowRecovery();
        return;
    case Command::LaterRecovery:
        recoveryPanel->SetVisible(false);
        host.SetFocusControl(editor, !options.headless);
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
                [this]
                {
                    const auto candidate = recoveryCandidates.front();
                    SetDocument(candidate.snapshot.text, candidate.snapshot.originalPath, candidate.snapshot.bom);
                    recovered = true;
                    editor->SetSelection(candidate.snapshot.selection);
                    Changed();
                    const auto snapshot = RecoverySnapshot{editor->GetText(), documentPath, bom, editor->GetSelection(), ++recoveryGeneration};
                    if (FAILED(recovery.Submit(snapshot, true, candidate.id)))
                    {
                        Status(IDS_RECOVERYFAILED);
                        return;
                    }
                    recoveryCandidates.erase(recoveryCandidates.begin());
                    CloseDrawers();
                    Status(IDS_RECOVERED);
                });
        return;
    case Command::New:
        AskThen(
            [this]
            {
                SetDocument({});
                host.SetFocusControl(editor, !options.headless);
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
        (void)editor->ReplaceSelection(L"\n\n```mermaid\ngraph LR\nA[Idea] --> B[Write]\nB --> C[Share]\n```\n\n");
        semantic = false;
        break;
    case Command::Math:
        (void)editor->ReplaceSelection(L"\n\n$$ x = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a} $$\n\n");
        semantic = false;
        break;
    case Command::Quote:
        (void)editor->ReplaceSelection(L"\n> A thought worth keeping.\n");
        semantic = false;
        break;
    case Command::Fence:
        (void)editor->ReplaceSelection(L"\n\n```text\nYour code here\n```\n\n");
        semantic = false;
        break;
    case Command::LocalSource:
        LocalSource();
        return;
    case Command::Find:
        drawer->SetVisible(false);
        floating->SetVisible(false);
        findBar->SetVisible(true);
        Layout();
        RefreshFind();
        host.SetFocusControl(findText, !options.headless);
        return;
    case Command::FindNext:
        FindNext();
        return;
    case Command::Settings:
        findBar->SetVisible(false);
        RefreshFind();
        floating->SetVisible(false);
        drawer->SetVisible(!drawer->IsVisible());
        Layout();
        host.SetFocusControl(drawer->IsVisible() ? static_cast<DxUi::Control *>(themeButtons[0]) : editor, !options.headless);
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
    host.SetFocusControl(editor, !options.headless);
}

void Application::LocalSource()
{
    if (editor->GetSemanticSourceRegion())
    {
        (void)editor->EndSemanticSourceEdit(host);
        return;
    }
    const auto semantic = editor->GetAcknowledgedSemanticDocument(host);
    const auto selection = editor->GetSelection();
    if (semantic)
    {
        const auto elements = semantic->Elements();
        for (std::size_t index = elements.size(); index > 0; --index)
        {
            const auto &element = elements[index - 1];
            if (selection.caret >= element.source.first && selection.caret <= element.source.last &&
                editor->BeginSemanticSourceEdit(host, semantic, index - 1))
            {
                CloseDrawers();
                return;
            }
        }
    }
    const auto source = editor->GetText();
    std::size_t first = selection.Start(), last = selection.End();
    // Exact math source is a syntax fact; an unrelated preview may still be
    // preparing. Raster acknowledgment is required only for in-document editors.
    if (const auto document = editor->GetSemanticDocument())
        for (const auto &element : document->Elements())
            if (element.role == DxUi::RichText::SemanticRole::Math && selection.caret >= element.source.first &&
                selection.caret <= element.source.last)
            {
                first = element.source.first;
                last = element.source.last;
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
        editor->SetSourceHighlights(std::span(&block, 1));
    }
    Layout();
    host.SetFocusControl(blockDraftText, !options.headless);
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
    const std::wstring value(blockDraftText->GetText());
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

void Application::Menu(POINT point, bool insertOnly, bool headingOnly)
{
    if (options.headless)
        return; // Focus-taking menus are an explicitly interactive gate.
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
    floating->SetVisible(false);
    DxUi::ContextMenuSessionCallbacks callbacks;
    callbacks.focusFirstNavigableItem = true;
    (void)DxUi::ContextMenu::ShowAsync(
        window.get(), point, items, host.GetTheme(),
        [this](std::optional<int> selected)
        {
            if (closing)
                return;
            if (selected)
                Execute(static_cast<Command>(selected.value() - 1));
            host.SetFocusControl(editor, true);
        },
        callbacks);
}

std::filesystem::path Application::PickFile(bool saving, bool image)
{
    if (options.headless)
        return {};
    wil::com_ptr<IFileDialog> dialog;
    HRESULT hr = saving ? CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.put()))
                        : CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.put()));
    if (FAILED(hr))
        return {};
    const COMDLG_FILTERSPEC markdown[]{{L"Markdown", L"*.md;*.markdown"}, {L"Text files", L"*.txt"}, {L"All files", L"*.*"}};
    const COMDLG_FILTERSPEC pictures[]{{L"Images", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"}};
    (void)dialog->SetFileTypes(image ? 1u : 3u, image ? pictures : markdown);
    DWORD flags{};
    (void)dialog->GetOptions(&flags);
    (void)dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR | (saving ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST));
    if (saving)
    {
        (void)dialog->SetDefaultExtension(L"md");
        const auto baseName =
            documentPath.filename().empty() ? std::filesystem::path(Resource(IDS_UNTITLED)).stem().native() : documentPath.stem().native();
        const auto name = recovered              ? std::vformat(Resource(IDS_RECOVEREDNAME), std::make_wformat_args(baseName))
                          : documentPath.empty() ? Resource(IDS_UNTITLED)
                                                 : documentPath.filename().native();
        (void)dialog->SetFileName(name.c_str());
    }
    if (FAILED(dialog->Show(window.get())))
        return {};
    wil::com_ptr<IShellItem> item;
    wil::unique_cotaskmem_string path;
    if (FAILED(dialog->GetResult(item.put())) || FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, path.put())))
        return {};
    return std::filesystem::path(path.get());
}

void Application::AskThen(std::function<void()> continuation)
{
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
    if (options.headless)
        return;
    const int choice =
        MessageBoxW(window.get(), Resource(IDS_SAVEQUESTION).c_str(), Resource(IDS_APP).c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3);
    if (choice == IDNO)
        continuation();
    else if (choice == IDYES)
    {
        afterSave = std::move(continuation);
        Save();
    }
}
void Application::Open()
{
    AskThen(
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
                Status(IDS_LOADING);
        });
}
void Application::Save(bool saveAs, bool force)
{
    saveAs = saveAs || recovered;
    if (files.Busy())
    {
        Status(IDS_BUSY);
        return;
    }
    auto path = documentPath;
    if (saveAs || path.empty())
        path = PickFile(true);
    if (path.empty())
    {
        afterSave = {};
        return;
    }
    const auto expected = (!force && !saveAs && path == documentPath) ? std::optional<FileStamp>(stamp) : std::nullopt;
    const HRESULT hr = files.Submit(true, path, editor->GetText(), bom, expected);
    if (FAILED(hr))
    {
        afterSave = {};
        Status(IDS_SAVEFAILED);
    }
    else
        Status(IDS_SAVING);
}
void Application::FileCompleted()
{
    auto completed = files.Take();
    if (!completed)
        return;
    auto &result = completed.value();
    if (FAILED(result.status))
    {
        if (result.saving && result.status == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && !options.headless)
        {
            const int choice =
                MessageBoxW(window.get(), Resource(IDS_CONFLICT).c_str(), Resource(IDS_APP).c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (choice == IDYES)
            {
                Save(false, true);
                return;
            }
        }
        const auto error = ErrorText(result.status);
        const auto message = std::vformat(Resource(result.saving ? IDS_SAVEFAILED : IDS_OPENFAILED), std::make_wformat_args(error));
        if (!options.headless)
            MessageBoxW(window.get(), message.c_str(), Resource(IDS_ERROR).c_str(), MB_OK | MB_ICONERROR);
        feedback->SetText(message);
        afterSave = {};
        return;
    }
    if (result.saving)
    {
        recovered = false;
        savedText = std::move(result.document.text);
        dirtyFact.reset();
        documentPath = std::move(result.path);
        RefreshFiles(true);
        stamp = result.document.stamp;
        editor->SetDocumentFilePath(documentPath.native());
        Changed();
        Status(IDS_SAVED);
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
        auto apply = [this, staged]
        {
            SetDocument(std::move(staged->document.text), std::move(staged->path), staged->document.bom, staged->document.stamp);
            Status(IDS_SAVED);
        };
        if (openingSnapshot && editor->Snapshot().IsSameRevision(openingSnapshot.value()))
            apply();
        else
            AskThen(std::move(apply));
        openingSnapshot.reset();
    }
}
void Application::ServiceImages()
{
    if (!editor || closing)
        return;
    editor->TakePendingImageRequests(imageRequests);
    images.DiscardCancelled();
    if (!imageRequests.empty())
        (void)images.Submit(imageRequests);
    if (images.HasResults())
    {
        std::array<RichImageLoader::Result, RichImageLoader::MaximumResults> results;
        const auto count = images.Drain(results);
        for (std::size_t index = 0; index < count; ++index)
            (void)editor->CompleteImageRequest(results[index].request, std::move(results[index].image));
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
    try
    {
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
            return app->HandleMessage(hwnd, message, wParam, lParam);
    }
    catch (const std::bad_alloc &)
    {
        std::terminate();
    }
    catch (const std::exception &)
    {
        OutputDebugStringW(L"RedTilde window callback failed; source retained\n");
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
void Application::BeginClose()
{
    // Reaching this point means the user saved or explicitly discarded source.
    // Retire only this session's snapshot and wait asynchronously for its result.
    (void)recovery.Submit(std::nullopt, true);
    recovery.Stop();
    closing = true;
    files.Close();
    folderFiles.Close();
    images.Shutdown();
    host.Detach();
    waitingForPreferences = preferenceWriter.Busy() || !recovery.Finished();
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
        window.reset();
    }
}
void Application::FinishClose()
{
    if (closing && !preferenceWriter.Busy() && recovery.Finished())
    {
        waitingForPreferences = false;
        recovery.Close();
        preferenceWriter.Close();
        window.reset();
    }
}
LRESULT Application::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        if (closing)
            return 0;
        if (blockDraft->IsVisible())
        {
            Status(IDS_BLOCKPENDING);
            host.SetFocusControl(blockDraftText, !options.headless);
            return 0;
        }
        if (files.Busy())
        {
            if (options.headless)
            {
                Status(IDS_BUSY);
                return 0;
            }
            const int choice =
                MessageBoxW(hwnd, Resource(IDS_CLOSEBUSY).c_str(), Resource(IDS_APP).c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (choice == IDYES)
            {
                BeginClose();
            }
            return 0;
        }
        AskThen([this] { BeginClose(); });
        return 0;
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
        folderFiles.Close();
        folderFilesActive = false;
        host.Detach();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    if (message == WM_GETMINMAXINFO)
    {
        auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
        const UINT dpi = GetDpiForWindow(hwnd);
        info->ptMinTrackSize = {MulDiv(800, static_cast<int>(dpi), 96), MulDiv(560, static_cast<int>(dpi), 96)};
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
    if (message == ImageReady)
    {
        if (wParam == 0x5254)
            ServiceImages();
        return 0;
    }
    if (message == WM_SETTINGCHANGE || message == WM_THEMECHANGED)
        ApplyTheme();
    bool handled{};
    const auto result = host.HandleMessage(hwnd, message, wParam, lParam, handled);
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
    for (unsigned index = 0; index < turns; ++index)
    {
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
        if (!refreshed && !host.HasPendingContentPreparation() && editor->GetImageResourceStatistics().pendingSources == 0 && !files.Busy() &&
            recovery.Ready() && (!folderFilesActive || folderSnapshot || FAILED(folderFiles.Status())))
            break;
        // Harness-only bounded event wait; production blocks in GetMessage.
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}
bool Application::Capture(const std::filesystem::path &path)
{
    Prepare();
    DxUi::WindowHostBitmapCapture bitmap;
    if (!host.RenderInitialFrameForShow())
        return false;
    // Hidden capture reads this application's D2D render target. It neither shows
    // the window nor reads pixels from the desktop. The library's diagnostic
    // capture entrypoint deliberately refuses an invisible swap-chain host.
    auto *context = host.GetDeviceContext();
    if (!context)
        return false;
    wil::com_ptr<ID2D1Image> target;
    context->GetTarget(target.put());
    wil::com_ptr<ID2D1Bitmap1> source;
    if (!target || FAILED(target->QueryInterface(IID_PPV_ARGS(source.put()))))
        return false;
    const auto size = source->GetPixelSize();
    const auto readbackProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, source->GetPixelFormat());
    wil::com_ptr<ID2D1Bitmap1> staging;
    if (FAILED(context->CreateBitmap(size, nullptr, 0, &readbackProperties, staging.put())) ||
        FAILED(staging->CopyFromBitmap(nullptr, source.get(), nullptr)))
        return false;
    D2D1_MAPPED_RECT mapped{};
    if (FAILED(staging->Map(D2D1_MAP_OPTIONS_READ, &mapped)))
        return false;
    const auto unmap = wil::scope_exit([&] { (void)staging->Unmap(); });
    bitmap.widthPx = size.width;
    bitmap.heightPx = size.height;
    bitmap.bgraPixels.resize(static_cast<std::size_t>(size.width) * size.height * 4);
    for (UINT row = 0; row < size.height; ++row)
        std::copy_n(mapped.bits + static_cast<std::size_t>(row) * mapped.pitch, size.width * 4,
                    bitmap.bgraPixels.data() + static_cast<std::size_t>(row) * size.width * 4);
    wil::com_ptr<IWICImagingFactory> factory;
    wil::com_ptr<IWICStream> stream;
    wil::com_ptr<IWICBitmapEncoder> encoder;
    wil::com_ptr<IWICBitmapFrameEncode> frame;
    wil::com_ptr<IPropertyBag2> properties;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put()))))
        return false;
    if (FAILED(factory->CreateStream(stream.put())) || FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)))
        return false;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put())) ||
        FAILED(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache)))
        return false;
    if (FAILED(encoder->CreateNewFrame(frame.put(), properties.put())) || FAILED(frame->Initialize(properties.get())) ||
        FAILED(frame->SetSize(bitmap.widthPx, bitmap.heightPx)))
        return false;
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (FAILED(frame->SetPixelFormat(&format)) || !IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA))
        return false;
    if (FAILED(frame->WritePixels(bitmap.heightPx, bitmap.widthPx * 4, static_cast<UINT>(bitmap.bgraPixels.size()), bitmap.bgraPixels.data())))
        return false;
    return SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
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
                command = Command::Open;
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
        return RunDesignInteractiveProbe(*this, options);
    if (!options.performance.empty())
        return RunPerformanceProbe(*this, options);
    if (options.recoveryProbe)
    {
        editor->SetSelection({0, 0});
        (void)editor->ReplaceSelection(L"Recovered after crash: 東京 😀\r\n");
        CheckpointRecovery(true);
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
        if (recoveryCandidates.size() != 1)
            return 7;
        Execute(Command::RestoreRecovery);
        const bool correct = Dirty() && recovered && editor->GetText().starts_with(L"Recovered after crash: 東京 😀\r\n");
        CheckpointRecovery(true);
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
        settings.appearance = options.scenario == L"compose-dark" || options.scenario == L"math-dark" || options.scenario == L"reference-dark"
                                  ? Appearance::Dark
                                  : Appearance::Light;
        ApplyTheme();
        if (options.scenario == L"math" || options.scenario == L"math-dark")
            SetDocument(L"# Mathematics, in the flow\n\nInline $E=mc^2$ shares the text baseline. Exact TeX stays one F2 away.\n\n"
                        L"$$x=\\frac{-b+\\sqrt{b^2-4ac}}{2a}$$\n\n"
                        L"$$A=\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\\quad \\vec{v}=\\begin{pmatrix}x\\\\y\\end{pmatrix}$$\n\n"
                        L"$$\\begin{aligned}f(x)&=x^2+1\\\\f'(x)&=2x\\end{aligned}$$\n\n"
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
    (void)host.PrimeForShow();
    if (identityMeasure.typographyRevision != host.GetTypographyRevision())
    {
        Layout();
        (void)host.PrimeForShow();
    }
    ShowWindow(window.get(), SW_SHOWNORMAL);
    UpdateWindow(window.get());
    host.SetFocusControl(editor, true);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (!RouteShortcut(message) && !host.DispatchKeyboardMessage(message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (waitingForPreferences && !preferenceWriter.Busy() && recovery.Finished())
            (void)SendMessageW(window.get(), PreferencesReady, 0, 0);
        if (!closing && files.Busy())
            FileCompleted();
    }
    return static_cast<int>(message.wParam);
}
} // namespace RedTilde
