#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/MonitorDiagnostics.h"
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
std::wstring Resource(unsigned id)
{
    const wchar_t *value{};
    const int length = LoadStringW(GetModuleHandleW(nullptr), id, reinterpret_cast<LPWSTR>(&value), 0);
    return length > 0 ? std::wstring(value, static_cast<std::size_t>(length)) : std::wstring{};
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
    if (!host.Attach(window.get(), {.forceWarp = options.forceWarp}))
        return HRESULT_FROM_WIN32(GetLastError());
    files.Initialize(window.get(), FileReady);
    preferenceWriter.Initialize(options.dataDirectory, window.get(), PreferencesReady);
    RETURN_IF_FAILED(recovery.Initialize(options.dataDirectory, window.get(), RecoveryReadyMessage));
    RETURN_IF_FAILED(images.Initialize(window.get(), ImageReady, 0x5254));
    initialized = true;
    initializing = true;
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
    return result;
}
DxUi::Button *Application::Action(DxUi::Panel &parent, unsigned label, Command command, std::wstring tooltip)
{
    auto *button = parent.AddChild<DxUi::Button>(Resource(label));
    button->SetAccessibleName(Resource(label));
    button->SetTooltipText(tooltip.empty() ? Resource(label) : std::move(tooltip));
    button->SetOnClick([this, command] { Execute(command); });
    return button;
}

void Application::BuildUi()
{
    auto tree = std::make_unique<Surface>();
    root = tree.get();
    documentBar = root->AddChild<Surface>();
    auto *brand = Label(*documentBar, IDS_APP, DxUi::FontRole::Subtitle);
    brand->SetText(L"~  " + Resource(IDS_APP));
    brand->SetTextColor(D2D1::ColorF(0xC83C46));
    brand->SetBounds(D2D1::RectF(24, 12, 180, 48));
    title = Label(*documentBar, IDS_UNTITLED, DxUi::FontRole::BodyStrong);
    saveState = Label(*documentBar, IDS_SAVED, DxUi::FontRole::Small);
    topActions.push_back(Action(*documentBar, IDS_NEW, Command::New, L"New · Ctrl+N"));
    topActions.push_back(Action(*documentBar, IDS_OPEN, Command::Open, L"Open · Ctrl+O"));
    topActions.push_back(Action(*documentBar, IDS_SAVE, Command::Save, L"Save · Ctrl+S"));
    topActions.back()->SetPrimary(true);
    topActions.push_back(Action(*documentBar, IDS_SAVEAS, Command::SaveAs, L"Save as · Ctrl+Shift+S"));
    commandBar = root->AddChild<Surface>();
    const std::array<std::pair<unsigned, Command>, 13> buttons{{{IDS_UNDO, Command::Undo},
                                                                {IDS_REDO, Command::Redo},
                                                                {IDS_HEADING, Command::Heading},
                                                                {IDS_BOLD, Command::Bold},
                                                                {IDS_ITALIC, Command::Italic},
                                                                {IDS_CODE, Command::Code},
                                                                {IDS_BULLETS, Command::Bullets},
                                                                {IDS_TASK, Command::Task},
                                                                {IDS_INSERT, Command::Insert},
                                                                {IDS_FIND, Command::Find},
                                                                {IDS_LOCAL, Command::LocalSource},
                                                                {IDS_FOCUS, Command::Focus},
                                                                {IDS_SETTINGS, Command::Settings}}};
    for (const auto &[id, command] : buttons)
        tools.push_back(Action(*commandBar, id, command));
    for (unsigned index = 0; index < 3; ++index)
        modeButtons[index] = Action(*root, IDS_COMPOSE + index, static_cast<Command>(static_cast<unsigned>(Command::Compose) + index));
    modeHint = Label(*root, IDS_BANNER, DxUi::FontRole::Small);
    page = root->AddChild<Surface>(Surface::Kind::Canvas);
    editor = page->AddChild<Editor>();
    editor->SetAccessibleName(Resource(IDS_DOC));
    editor->SetAccessibleAutomationId(L"RedTilde.Document");
    editor->SetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown);
    editor->SetMarkdownRenderingEnabled(true);
    editor->SetViewMode(DxUi::RichText::ViewMode::Rich);
    editor->SetOnTextChanged([this] { Changed(); });
    editor->changedSelection = [this] { SelectionChanged(); };
    editor->SetOnContextMenu([this](POINT point, bool) { Menu(point); });
    mirror = root->AddChild<DxUi::RichTextEditor>();
    mirror->SetReadOnly(true);
    mirror->SetViewMode(DxUi::RichText::ViewMode::Source);
    mirror->SetAccessibleName(Resource(IDS_MIRROR));
    mirror->SetVisible(false);
    sidebar = root->AddChild<Surface>(Surface::Kind::Sidebar);
    outlineTitle = Label(*sidebar, IDS_OUTLINE, DxUi::FontRole::BodyStrong);
    for (unsigned index = 0; index < 12; ++index)
    {
        auto *button = sidebar->AddChild<DxUi::Button>();
        button->SetOnClick(
            [this, index]
            {
                if (index >= documentInfo.headings.size())
                    return;
                const auto offset = documentInfo.headings[index].offset;
                editor->SetSelection({offset, offset});
                (void)editor->ScrollSourceRangeIntoView(host, offset, offset, true);
                host.SetFocusControl(editor, !options.headless);
                SelectionChanged();
            });
        outlineButtons.push_back(button);
    }
    equationHelp = Label(*sidebar, IDS_PREVIEW, DxUi::FontRole::BodyStrong);
    equation = sidebar->AddChild<DxUi::RichTextEditor>();
    equation->SetReadOnly(true);
    equation->SetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown);
    equation->SetMarkdownRenderingEnabled(true);
    equation->SetAccessibleName(Resource(IDS_PREVIEW));
    equation->SetText(L"$x = \\frac{-b + \\sqrt{b^2 - 4ac}}{2a}$");
    auto *mathHelp = Label(*sidebar, IDS_MATHLIMIT, DxUi::FontRole::Small);
    mathHelp->SetMultiline(true);
    mathHelp->SetBounds(D2D1::RectF(0, 0, 0, 0)); // arranged below with the panel's other labels
    drawer = root->AddChild<Surface>(Surface::Kind::Floating);
    drawer->SetVisible(false);
    Label(*drawer, IDS_SETTINGS, DxUi::FontRole::Subtitle);
    const std::array<unsigned, 4> themeIds{IDS_SYSTEM, IDS_LIGHT, IDS_DARK, IDS_PAPER};
    for (unsigned index = 0; index < 4; ++index)
        themeButtons.push_back(Action(*drawer, themeIds[index], static_cast<Command>(static_cast<unsigned>(Command::ThemeSystem) + index)));
    outlineToggle = Action(*drawer, IDS_VIEWTOOLS, Command::Outline);
    focusToggle = Action(*drawer, IDS_FOCUS, Command::Focus);
    Action(*drawer, IDS_RECOVERY, Command::Recovery);
    Action(*drawer, IDS_CLOSE, Command::CloseDrawer);
    findBar = root->AddChild<Surface>(Surface::Kind::Floating);
    findBar->SetVisible(false);
    findText = findBar->AddChild<DxUi::TextField>();
    findText->SetAccessibleName(Resource(IDS_FINDHINT));
    findText->SetOnSubmitted([this] { FindNext(); });
    Action(*findBar, IDS_FINDNEXT, Command::FindNext);
    Action(*findBar, IDS_CLOSE, Command::CloseDrawer);
    blockDraft = root->AddChild<Surface>(Surface::Kind::Floating);
    blockDraft->SetVisible(false);
    Label(*blockDraft, IDS_LOCAL, DxUi::FontRole::Subtitle);
    Label(*blockDraft, IDS_BLOCKHELP, DxUi::FontRole::Small);
    blockDraftText = blockDraft->AddChild<DxUi::TextField>();
    blockDraftText->SetMultiline(true);
    blockDraftText->SetAccessibleName(Resource(IDS_LOCAL));
    blockDraftText->SetOnTextChanged(
        [this](std::wstring_view)
        {
            if (HasBlockDraft())
                CheckpointRecovery();
        });
    blockDraftText->SetOnPreviewKeyDown(
        [this](DxUi::ControlHost &, UINT key, UINT modifiers)
        {
            if (key == VK_RETURN && (modifiers & MK_CONTROL))
            {
                Execute(Command::ApplyBlock);
                return true;
            }
            return false;
        });
    Action(*blockDraft, IDS_APPLY, Command::ApplyBlock)->SetPrimary(true);
    Action(*blockDraft, IDS_CLOSE, Command::CloseDrawer);
    recoveryPanel = root->AddChild<Surface>(Surface::Kind::Floating);
    recoveryPanel->SetVisible(false);
    Label(*recoveryPanel, IDS_RECOVERY, DxUi::FontRole::Subtitle);
    recoveryDescription = Label(*recoveryPanel, IDS_RECOVERYHELP, DxUi::FontRole::Small);
    recoveryDescription->SetMultiline(true);
    recoveryPreview = recoveryPanel->AddChild<DxUi::TextField>();
    recoveryPreview->SetMultiline(true);
    recoveryPreview->SetReadOnly(true);
    recoveryPreview->SetAccessibleName(Resource(IDS_RECOVERY));
    Action(*recoveryPanel, IDS_RESTORE, Command::RestoreRecovery)->SetPrimary(true);
    Action(*recoveryPanel, IDS_DISCARD, Command::DiscardRecovery);
    Action(*recoveryPanel, IDS_LATER, Command::LaterRecovery);
    stats = Label(*root, IDS_SAVED, DxUi::FontRole::Small);
    feedback = Label(*root, IDS_FOOTERHINT, DxUi::FontRole::Small);
    floating = root->AddChild<Surface>(Surface::Kind::Floating);
    for (const auto &[id, command] : std::array<std::pair<unsigned, Command>, 5>{{{IDS_BOLD, Command::Bold},
                                                                                  {IDS_ITALIC, Command::Italic},
                                                                                  {IDS_CODE, Command::Code},
                                                                                  {IDS_LINK, Command::Link},
                                                                                  {IDS_LOCAL, Command::LocalSource}}})
        floatingActions.push_back(Action(*floating, id, command));
    floating->SetVisible(false);
    host.SetRoot(std::move(tree));
    host.SetOnEscape(
        [this]
        {
            if (drawer->IsVisible() || findBar->IsVisible() || floating->IsVisible() || blockDraft->IsVisible() || recoveryPanel->IsVisible())
            {
                CloseDrawers();
                return true;
            }
            if (editor->GetSemanticSourceRegion())
                return editor->EndSemanticSourceEdit(host);
            return false;
        });
}

void Application::Layout()
{
    ++activity.layouts;
    if (!root)
        return;
    const auto bounds = host.GetClientBoundsDip();
    const float width = bounds.right, height = bounds.bottom;
    root->SetBounds(bounds);
    const bool focus = settings.focusMode;
    documentBar->SetVisible(!focus);
    commandBar->SetVisible(!focus);
    documentBar->SetBounds(D2D1::RectF(0, 0, width, 62));
    title->SetBounds(D2D1::RectF(196, 10, std::max(300.f, width - 430), 32));
    saveState->SetBounds(D2D1::RectF(196, 32, std::max(300.f, width - 430), 52));
    const std::array<float, 4> topWidths{54, 60, 68, 86};
    float x = width - 312;
    for (std::size_t index = 0; index < topActions.size(); ++index)
    {
        topActions[index]->SetBounds(D2D1::RectF(x, 14, x + topWidths[index], 48));
        x += topWidths[index] + 8;
    }
    commandBar->SetBounds(D2D1::RectF(0, 62, width, 110));
    const bool compact = width < 1120;
    const std::array<float, 13> toolWidths{52, 52, 92, 50, 50, 50, 64, 48, 62, 48, 126, 88, 72};
    const std::array<float, 13> compactWidths{46, 46, 84, 44, 44, 44, 56, 44, 56, 44, 44, 44, 66};
    x = 20;
    for (std::size_t index = 0; index < tools.size(); ++index)
    {
        const float itemWidth = compact ? compactWidths[index] : toolWidths[index];
        tools[index]->SetText(index == 10 && compact ? L"F2"
                              : index == 11 && compact
                                  ? L"F11"
                                  : Resource(std::array<unsigned, 13>{IDS_UNDO, IDS_REDO, IDS_HEADING, IDS_BOLD, IDS_ITALIC, IDS_CODE, IDS_BULLETS,
                                                                      IDS_TASK, IDS_INSERT, IDS_FIND, IDS_LOCAL, IDS_FOCUS, IDS_SETTINGS}[index]));
        tools[index]->SetBounds(D2D1::RectF(x, 70, x + itemWidth, 102));
        x += itemWidth + 5;
    }
    const float viewY = focus ? 12.f : 124.f;
    for (unsigned index = 0; index < 3; ++index)
    {
        modeButtons[index]->SetBounds(D2D1::RectF(24 + index * 91.f, viewY, 109 + index * 91.f, viewY + 32));
        modeButtons[index]->SetPrimary(static_cast<unsigned>(settings.view) == index);
    }
    modeHint->SetBounds(D2D1::RectF(316, viewY, width - 24, viewY + 32));
    const float top = viewY + 48, bottom = height - 66;
    const bool showSidebar = settings.showOutline && width >= 1120 && !focus && settings.view != EditorView::Split;
    sidebar->SetVisible(showSidebar);
    const float right = showSidebar ? width - 278 : width - 24;
    const bool split = settings.view == EditorView::Split;
    const float left = split ? 24.f : std::max(24.f, (right - 24 - 800) / 2 + 24);
    const float pageRight = split ? (width - 36) / 2 : std::min(right, left + 800);
    page->SetBounds(D2D1::RectF(left, top, pageRight, bottom));
    const float padding = split ? 20.f : 32.f;
    editor->SetBounds(D2D1::RectF(left + padding, top + 20, pageRight - padding, bottom - 16));
    mirror->SetVisible(split);
    mirror->SetBounds(D2D1::RectF(pageRight + 16, top, width - 24, bottom));
    sidebar->SetBounds(D2D1::RectF(width - 262, top, width - 16, bottom));
    outlineTitle->SetBounds(D2D1::RectF(width - 246, top + 14, width - 30, top + 44));
    float outlineY = top + 54;
    for (std::size_t index = 0; index < outlineButtons.size(); ++index)
    {
        auto *button = outlineButtons[index];
        const bool visible = index < documentInfo.headings.size();
        button->SetVisible(visible);
        if (visible)
        {
            const auto &heading = documentInfo.headings[index];
            button->SetText(heading.text);
            button->SetAccessibleName(heading.text);
            button->SetBounds(D2D1::RectF(width - 246 + std::min(heading.level - 1, 3u) * 10, outlineY, width - 30, outlineY + 34));
            outlineY += 42;
        }
    }
    const float equationY = std::min(bottom - 230, std::max(top + 280, outlineY + 32));
    equationHelp->SetBounds(D2D1::RectF(width - 246, equationY, width - 30, equationY + 28));
    equation->SetBounds(D2D1::RectF(width - 246, equationY + 38, width - 30, equationY + 148));
    if (auto *help = sidebar->GetLogicalChild(sidebar->GetLogicalChildCount() - 1))
        help->SetBounds(D2D1::RectF(width - 246, equationY + 150, width - 30, equationY + 208));
    const float drawerLeft = std::max(24.f, width - 372);
    drawer->SetBounds(D2D1::RectF(drawerLeft, 118, width - 24, 478));
    drawer->GetLogicalChild(0)->SetBounds(D2D1::RectF(drawerLeft + 20, 134, width - 44, 170));
    for (unsigned index = 0; index < 4; ++index)
    {
        themeButtons[index]->SetBounds(D2D1::RectF(drawerLeft + 20 + (index % 2) * 154.f, 188.f + (index / 2) * 44.f,
                                                   drawerLeft + 162 + (index % 2) * 154.f, 222.f + (index / 2) * 44.f));
        themeButtons[index]->SetPrimary(static_cast<unsigned>(settings.appearance) == index);
    }
    outlineToggle->SetBounds(D2D1::RectF(drawerLeft + 20, 286, width - 44, 320));
    outlineToggle->SetPrimary(settings.showOutline);
    focusToggle->SetBounds(D2D1::RectF(drawerLeft + 20, 330, width - 44, 364));
    drawer->GetLogicalChild(drawer->GetLogicalChildCount() - 2)->SetBounds(D2D1::RectF(drawerLeft + 20, 380, width - 44, 414));
    drawer->GetLogicalChild(drawer->GetLogicalChildCount() - 1)->SetBounds(D2D1::RectF(drawerLeft + 20, 428, width - 44, 462));
    findBar->SetBounds(D2D1::RectF(std::max(24.f, width - 550), viewY + 40, width - 24, viewY + 96));
    findText->SetBounds(D2D1::RectF(std::max(36.f, width - 538), viewY + 50, width - 182, viewY + 84));
    findBar->GetLogicalChild(1)->SetBounds(D2D1::RectF(width - 172, viewY + 50, width - 108, viewY + 84));
    findBar->GetLogicalChild(2)->SetBounds(D2D1::RectF(width - 100, viewY + 50, width - 36, viewY + 84));
    const float draftLeft = std::max(24.f, (width - 680) / 2), draftRight = std::min(width - 24, draftLeft + 680);
    blockDraft->SetBounds(D2D1::RectF(draftLeft, top + 24, draftRight, bottom - 24));
    blockDraft->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 20, top + 36, draftRight - 20, top + 72));
    blockDraft->GetLogicalChild(1)->SetBounds(D2D1::RectF(draftLeft + 20, top + 78, draftRight - 20, top + 108));
    blockDraftText->SetBounds(D2D1::RectF(draftLeft + 20, top + 120, draftRight - 20, bottom - 92));
    blockDraft->GetLogicalChild(3)->SetBounds(D2D1::RectF(draftRight - 244, bottom - 74, draftRight - 112, bottom - 40));
    blockDraft->GetLogicalChild(4)->SetBounds(D2D1::RectF(draftRight - 100, bottom - 74, draftRight - 20, bottom - 40));
    recoveryPanel->SetBounds(D2D1::RectF(draftLeft, top + 8, draftRight, bottom - 8));
    recoveryPanel->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 20, top + 24, draftRight - 20, top + 60));
    recoveryDescription->SetBounds(D2D1::RectF(draftLeft + 20, top + 68, draftRight - 20, top + 128));
    recoveryPreview->SetBounds(D2D1::RectF(draftLeft + 20, top + 136, draftRight - 20, bottom - 80));
    for (unsigned index = 0; index < 3; ++index)
        recoveryPanel->GetLogicalChild(index + 3)->SetBounds(
            D2D1::RectF(draftLeft + 20 + index * 208.f, bottom - 62, draftLeft + 220 + index * 208.f, bottom - 28));
    stats->SetBounds(D2D1::RectF(24, height - 52, width / 2, height - 24));
    feedback->SetBounds(D2D1::RectF(width / 2, height - 52, width - 24, height - 24));
    host.Invalidate();
}

void Application::ApplyTheme()
{
    bool dark = settings.appearance == Appearance::Dark;
    if (settings.appearance == Appearance::System)
    {
        DWORD value = 1, size = sizeof(value);
        (void)RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme",
                           RRF_RT_REG_DWORD, nullptr, &value, &size);
        dark = value == 0;
    }
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
        theme.selectionFill = theme.accent = systemColor(COLOR_HIGHLIGHT);
    }
    else
    {
        theme.accent = D2D1::ColorF(dark ? 0xFF7778 : 0xC83C46);
        theme.accentHover = D2D1::ColorF(dark ? 0xFF999A : 0xB6303C);
        theme.accentPressed = D2D1::ColorF(dark ? 0xFFABAC : 0xA12A34);
        theme.selectionFill = D2D1::ColorF(dark ? 0xA63B46 : 0xF2CFD3);
        theme.selectionText = theme.text;
        theme.focusStroke = theme.accent;
        theme.windowBackground = D2D1::ColorF(dark ? 0x17191E : settings.appearance == Appearance::Paper ? 0xEDE6D8 : 0xF0F1F4);
        theme.surfaceBackground = D2D1::ColorF(dark ? 0x1C1F25 : settings.appearance == Appearance::Paper ? 0xF3EDDF : 0xF8F8FA);
        theme.cardBackground = theme.inputFill = D2D1::ColorF(dark ? 0x22252B : settings.appearance == Appearance::Paper ? 0xFFFAEF : 0xFFFFFF);
        theme.buttonBorder = theme.borderDefault;
    }
    BOOL animation = TRUE;
    (void)SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animation, 0);
    theme.reducedMotion = !animation || options.headless;
    host.SetTheme(theme);
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
    return editor && (recovered || editor->Snapshot().Length() != savedText.size() || CurrentText() != savedText);
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
    editor->SetDocumentFilePath(documentPath.native());
    SetView(settings.view);
    Changed();
}
void Application::Changed()
{
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Enter);
    ++activity.changes;
    const auto &text = CurrentText();
    auto info = AnalyzeDocument(text);
    const bool outlineChanged =
        info.headings.size() != documentInfo.headings.size() ||
        !std::equal(info.headings.begin(), info.headings.end(), documentInfo.headings.begin(),
                    [](const Heading &left, const Heading &right) { return left.level == right.level && left.text == right.text; });
    // Offsets always follow the current source, even when the outline's labels
    // and indentation can keep their existing bounds.
    documentInfo = std::move(info);
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Analysis);
    SynchronizeMirror();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Mirror);
    const auto name = documentPath.empty() ? Resource(IDS_UNTITLED) : documentPath.filename().native();
    title->SetText(name);
    const bool dirty = Dirty();
    saveState->SetText(Resource(dirty ? IDS_CHANGED : IDS_SAVED));
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Identity);
    UpdateCaption(std::format(L"{}{} — RedTilde", dirty ? L"• " : L"", name));
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Caption);
    const auto encodingSuffix = bom ? L" BOM" : L"";
    stats->SetText(std::vformat(Resource(IDS_STATS), std::make_wformat_args(documentInfo.words, documentInfo.characters, encodingSuffix)));
    tools[0]->SetEnabled(editor->CanUndo());
    tools[1]->SetEnabled(editor->CanRedo());
    floating->SetVisible(false);
    if (outlineChanged)
        Layout();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Chrome);
    SelectionChanged();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Selection);
    if (!initializing)
        CheckpointRecovery();
    if (editDiagnostics)
        editDiagnostics->Mark(EditPhase::Recovery);
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
    auto text = CurrentText();
    auto selection = editor->GetSelection();
    bool needsRecovery = Dirty();
    if (HasBlockDraft() && editor->Snapshot().IsSameRevision(blockDraftSnapshot) && editor->Revision() == blockDraftRevision)
    {
        const std::wstring draft(blockDraftText->GetText());
        if (draft != text.substr(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first))
        {
            if (!DxUi::RichText::IsValidUtf16(draft) ||
                text.size() - (blockDraftRange.last - blockDraftRange.first) + draft.size() > DxUi::RichText::Document::MaximumUnits)
            {
                Status(IDS_RECOVERYFAILED);
                return;
            }
            // Recovery is a previewable copy of the proposed edit. The live
            // authoritative source/history remain untouched until Apply.
            text.replace(blockDraftRange.first, blockDraftRange.last - blockDraftRange.first, draft);
            selection = {blockDraftRange.first, blockDraftRange.first + draft.size()};
            needsRecovery = true;
        }
    }
    if (needsRecovery)
        snapshot = RecoverySnapshot{std::move(text), documentPath, bom, selection, ++recoveryGeneration};
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
void Application::SelectionChanged()
{
    const auto selection = editor->GetSelection();
    if (settings.view == EditorView::Split)
    {
        mirror->SetSelection(selection);
        (void)mirror->ScrollSourceRangeIntoView(host, selection.Start(), selection.End(), false);
    }
    std::wstring expression;
    if (const auto syntax = editor->GetMarkdownSyntax())
    {
        for (const auto &node : syntax->Inlines())
            if (node.kind == DxUi::RichText::MarkdownInlineKind::Math && selection.caret >= node.source.first && selection.caret <= node.source.last)
            {
                expression = node.value.size() <= DxUi::RichText::MathMaximumSourceUnits ? node.value : std::wstring{};
                break;
            }
    }
    else
        expression = EquationAt(CurrentText(), selection.caret);
    const auto preview = expression.empty() ? std::wstring{} : L"\\[" + expression + L"\\]";
    if (equation->GetText() != preview)
        equation->SetText(preview);
    if (selection.Start() == selection.End() || settings.view == EditorView::Source || drawer->IsVisible() || findBar->IsVisible() ||
        blockDraft->IsVisible())
    {
        floating->SetVisible(false);
        return;
    }
    const auto rectangles = editor->TryGetTextInputRangeRects(host, selection.Start(), selection.End());
    if (!rectangles || rectangles->empty())
        return;
    const auto first = rectangles->front();
    const float left = std::clamp(first.left, 24.f, std::max(24.f, host.GetClientBoundsDip().right - 460));
    const float top = std::max(editor->GetBounds().top + 4, first.top - 46);
    floating->SetBounds(D2D1::RectF(left, top, left + 436, top + 40));
    const std::array<float, 5> widths{58, 58, 58, 58, 164};
    float x = left + 8;
    for (std::size_t index = 0; index < floatingActions.size(); ++index)
    {
        floatingActions[index]->SetBounds(D2D1::RectF(x, top + 4, x + widths[index], top + 36));
        x += widths[index] + 4;
    }
    floating->SetVisible(true);
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
    drawer->SetVisible(false);
    findBar->SetVisible(false);
    floating->SetVisible(false);
    blockDraft->SetVisible(false);
    recoveryPanel->SetVisible(false);
    editor->SetReadOnly(false);
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
    if (blockDraft->IsVisible() && command != Command::ApplyBlock && command != Command::CloseDrawer)
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    MarkdownCommandRequest request;
    bool semantic = true;
    switch (command)
    {
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
        Menu(host.DipPointToScreenPoint({command == Command::Heading ? 140.f : 650.f, 108.f}), command == Command::Insert,
             command == Command::Heading);
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
        host.SetFocusControl(findText, !options.headless);
        return;
    case Command::FindNext:
        FindNext();
        return;
    case Command::Settings:
        findBar->SetVisible(false);
        floating->SetVisible(false);
        drawer->SetVisible(!drawer->IsVisible());
        Layout();
        host.SetFocusControl(drawer->IsVisible() ? static_cast<DxUi::Control *>(themeButtons[0]) : editor, !options.headless);
        return;
    case Command::Focus:
        settings.focusMode = !settings.focusMode;
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
    if (semantic)
        for (const auto &element : semantic->Elements())
            if (element.role == DxUi::RichText::SemanticRole::Math && selection.caret >= element.source.first &&
                selection.caret <= element.source.last)
            {
                first = element.source.first;
                last = element.source.last;
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
    floating->SetVisible(false);
    blockDraft->SetVisible(true);
    editor->SetReadOnly(true);
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
        MonitorDiagnostics::Write(RedMonitor::Severity::Error,
                                  result.saving ? L"save failed; source and previous target retained" : L"open failed; current document retained");
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
        Status(result.saving ? IDS_SAVEFAILED : IDS_OPENFAILED);
        afterSave = {};
        return;
    }
    if (result.saving)
    {
        recovered = false;
        savedText = std::move(result.document.text);
        documentPath = std::move(result.path);
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
void Application::FindNext()
{
    const std::wstring needle(findText->GetText());
    if (needle.empty())
        return;
    const auto text = editor->GetText();
    auto match = text.find(needle, editor->GetSelection().End());
    if (match == text.npos)
        match = text.find(needle);
    if (match == text.npos)
    {
        Status(IDS_NOMATCH);
        return;
    }
    editor->SetSelection({match, match + needle.size()});
    (void)editor->ScrollSourceRangeIntoView(host, match, match + needle.size(), false);
    Status(IDS_FOUND);
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
    if (message == WM_DESTROY)
    {
        if (!options.headless)
            PostQuitMessage(0);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
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
    if (message == WM_SIZE || message == WM_DPICHANGED)
        Layout();
    if (message == WM_PAINT)
        ServiceImages();
    return handled ? result : DefWindowProcW(hwnd, message, wParam, lParam);
}

void Application::Prepare(unsigned turns)
{
    for (unsigned index = 0; index < turns; ++index)
    {
        (void)host.RenderInitialFrameForShow();
        ServiceImages();
        MSG message{};
        unsigned processed{};
        while (processed++ < 64 && PeekMessageW(&message, window.get(), 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (files.Busy())
            FileCompleted(); // also drains a result if its post met a full queue
        RecoveryCompleted();
        if (!host.HasPendingContentPreparation() && editor->GetImageResourceStatistics().pendingSources == 0 && !files.Busy() && recovery.Ready())
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

int Application::Run()
{
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
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return 0;
    }
    if (options.headless)
    {
        settings.appearance = options.scenario == L"compose-dark" || options.scenario == L"math-dark" ? Appearance::Dark : Appearance::Light;
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
        return options.capture.empty() || Capture(options.capture) ? 0 : 3;
    }
    (void)host.PrimeForShow();
    ShowWindow(window.get(), SW_SHOWNORMAL);
    UpdateWindow(window.get());
    host.SetFocusControl(editor, true);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        bool shortcut{};
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            (message.hwnd == window.get() || IsChild(window.get(), message.hwnd)))
        {
            const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0, shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            std::optional<Command> command;
            if (ctrl)
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
            if (message.wParam == VK_F2)
                command = Command::LocalSource;
            if (message.wParam == VK_F11)
                command = Command::Focus;
            if (message.wParam == VK_F10 && (GetKeyState(VK_MENU) & 0x8000))
            {
                SelectionChanged();
                if (floating->IsVisible())
                    host.SetFocusControl(floatingActions[0], true);
                shortcut = true;
            }
            if (command)
            {
                Execute(command.value());
                shortcut = true;
            }
        }
        if (!shortcut)
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
