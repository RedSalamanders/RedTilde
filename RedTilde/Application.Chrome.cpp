#include "Application.h"
#include "res/Resource.h"
#include <DxUi/Typography.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <wil/result.h>

namespace RedTilde
{
BrandMarkPixels LoadBrandMarkPixels(HICON icon)
{
    ICONINFO info{};
    const auto readIcon = GetIconInfo(icon, &info);
    wil::unique_hbitmap color(info.hbmColor), mask(info.hbmMask);
    THROW_IF_WIN32_BOOL_FALSE(readIcon);
    THROW_HR_IF_NULL(E_UNEXPECTED, color.get());
    BITMAP native{};
    THROW_LAST_ERROR_IF(GetObjectW(color.get(), sizeof(native), &native) != sizeof(native));
    THROW_HR_IF(E_UNEXPECTED, native.bmWidth <= 0 || native.bmWidth > 256 || native.bmHeight <= 0 || native.bmHeight > 256 || native.bmPlanes != 1 ||
                                  native.bmBitsPixel != 32);

    BrandMarkPixels result{static_cast<UINT>(native.bmWidth), static_cast<UINT>(native.bmHeight), {}};
    result.bgra.resize(static_cast<std::size_t>(result.width) * result.height * 4);
    BITMAPINFO bitmap{};
    bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth = native.bmWidth;
    bitmap.bmiHeader.biHeight = -native.bmHeight;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;
    wil::unique_hdc dc(CreateCompatibleDC(nullptr));
    THROW_LAST_ERROR_IF_NULL(dc);
    THROW_LAST_ERROR_IF(GetDIBits(dc.get(), color.get(), 0, result.height, result.bgra.data(), &bitmap, DIB_RGB_COLORS) != native.bmHeight);
    // GetDIBits returns straight BGRA for this owned alpha-icon asset. Match WIC's
    // round-to-nearest premultiplication exactly, including RGB under zero alpha.
    for (std::size_t offset = 0; offset < result.bgra.size(); offset += 4)
    {
        const unsigned alpha = result.bgra[offset + 3];
        for (unsigned channel = 0; channel < 3; ++channel)
            result.bgra[offset + channel] = static_cast<std::uint8_t>((result.bgra[offset + channel] * alpha + 127u) / 255u);
    }
    return result;
}

HRESULT SourceMirrorEditor::PrepareContent(DxUi::ControlHost &host) noexcept
{
    const auto selfLifetime = GetControlLifetimeToken(*this);
    auto *const primary = primary_;
    const auto primaryLifetime = primaryLifetime_;
    auto *const owner = host.GetRoot();
    if (!primary || primaryLifetime.expired() || !owner || !IsVisible() || !IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source ||
        GetHost() != &host || !primary->IsAttachedTo(host, GetParent()))
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    if (ownerLifetime.expired())
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto source = primary->Snapshot();
    const auto primaryRevision = primary->Revision();
    const auto syntax = primary->GetMarkdownSyntax();
    if (syntax && syntax->Revision() == primaryRevision && source.IsSameRevision(primary->Snapshot()))
    {
        const auto currentSyntax = GetMarkdownSyntax();
        const bool alreadyAdopted = currentSyntax && Snapshot().IsSameRevision(source) && currentSyntax->Revision() == Revision() &&
                                    currentSyntax->DocumentIdentity() != syntax->DocumentIdentity() && &currentSyntax->Nodes() == &syntax->Nodes();
        if (!alreadyAdopted)
        {
            const auto targetRevision = Revision();
            const HRESULT result = TryAdoptReadOnlyMarkdownSyntax(source, syntax);
            if (FAILED(result))
                return result;

            // Adoption's final preparation request can re-enter and retire
            // either editor or replace the owning root. Prove all borrowers
            // still belong to this host before the ordinary preparation call.
            if (selfLifetime.expired() || primaryLifetime.expired() || ownerLifetime.expired())
                return S_FALSE;
            if (GetHost() != &host || !primary->IsAttachedTo(host, GetParent()) || host.GetRoot() != owner ||
                !source.IsSameRevision(primary->Snapshot()) || primary->Revision() != primaryRevision)
                return S_FALSE;
            if (result == S_OK &&
                (!IsVisible() || !IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source || Revision() != targetRevision ||
                 !Snapshot().IsSameRevision(source) || GetMarkdownAnalysisProfile() != syntax->Profile()))
                return S_FALSE;
        }
    }
    if (selfLifetime.expired())
        return S_FALSE;
    return DxUi::RichTextEditor::PrepareContent(host);
}

namespace
{
// Product branding only. Shared controls, focus, input and editor presentation
// belong to DxUi. Keep the resource bytes across device loss, never a second host.
class BrandMark final : public DxUi::Control
{
    BrandMarkPixels source;
    wil::com_ptr<ID2D1Bitmap1> bitmap;
    std::uint64_t bitmapGeneration{};

  public:
    BrandMark()
    {
        EnableVisualContentPreparation();
        // LoadIcon returns a module-owned shared icon. Retain just its pixels;
        // ordinary startup needs no WIC factory/converter for this known asset.
        const auto icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_REDTILDE));
        THROW_LAST_ERROR_IF_NULL(icon);
        source = LoadBrandMarkPixels(icon);
    }
    void PrepareVisualContent(DxUi::ControlHost &host) noexcept override
    {
        const auto generation = host.GetResourceGeneration();
        if (bitmap && bitmapGeneration == generation)
            return;
        bitmap.reset();
        bitmapGeneration = 0;
        auto *target = host.GetDeviceContext();
        const auto properties =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (target &&
            SUCCEEDED(target->CreateBitmap(D2D1::SizeU(source.width, source.height), source.bgra.data(), source.width * 4, properties, bitmap.put())))
            bitmapGeneration = generation;
    }
    void CancelVisualContentPreparation() noexcept override
    {
        bitmap.reset();
        bitmapGeneration = 0;
    }
    void Paint(DxUi::ControlHost &host) const override
    {
        auto *target = host.GetDeviceContext();
        if (!target || !bitmap || bitmapGeneration != host.GetResourceGeneration())
            return;
        target->DrawBitmap(bitmap.get(), GetBounds());
    }
};
void Section(DxUi::Label *label)
{
    THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, .88f}));
}
void Icon(DxUi::Button *button, std::wstring glyph)
{
    button->SetVariant(DxUi::ButtonVariant::IconOnly);
    THROW_IF_FAILED(button->SetContentFontSizeDip(18.f));
    button->SetText(std::move(glyph));
}
} // namespace

void Application::BuildUi()
{
    auto tree = std::make_unique<Surface>();
    root = tree.get();
    documentBar = root->AddChild<Surface>();
    brandMark = documentBar->AddChild<BrandMark>();
    title = Label(*documentBar, IDS_UNTITLED, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(title->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    saveStateChip = documentBar->AddChild<DxUi::CardPanel>();
    saveStateChip->SetCornerRadius(10.f);
    saveState = Label(*saveStateChip, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(saveState->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    saveState->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    for (unsigned index = 0; index < 3; ++index)
    {
        modeButtons[index] = Action(*documentBar, IDS_COMPOSE + index, static_cast<Command>(static_cast<unsigned>(Command::Compose) + index));
        modeButtons[index]->SetAccessibleAutomationId(std::format(L"RedTilde.View{}", index));
        modeButtons[index]->SetAppearance(DxUi::ButtonAppearance::Tab);
    }
    fileMenu = Action(*documentBar, IDS_FILE, Command::FileMenu);
    fileMenu->SetVariant(DxUi::ButtonVariant::DropDown);
    fileMenu->SetAppearance(DxUi::ButtonAppearance::Subtle);
    saveButton = Action(*documentBar, IDS_SAVE, Command::Save, Resource(IDS_TIP_SAVE));
    saveButton->SetPrimary(true);
    exitFocus = Action(*documentBar, IDS_EXITFOCUS, Command::Focus);
    minimizeButton = Action(*documentBar, IDS_MINIMIZE, Command::Minimize);
    maximizeButton = Action(*documentBar, IDS_MAXIMIZE, Command::Maximize);
    closeButton = Action(*documentBar, IDS_CLOSE, Command::Close);
    Icon(minimizeButton, L"\xE921");
    Icon(maximizeButton, L"\xE922");
    Icon(closeButton, L"\xE8BB");
    for (auto *caption : {minimizeButton, maximizeButton, closeButton})
    {
        caption->SetAppearance(DxUi::ButtonAppearance::Subtle);
        THROW_IF_FAILED(caption->SetContentFontSizeDip(12.f));
    }
    commandBar = root->AddChild<Surface>();
    commandBar->borderBottom = true;
    const std::array<std::pair<unsigned, Command>, 14> buttons{{{IDS_UNDO, Command::Undo},
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
                                                                {IDS_SETTINGS, Command::Settings},
                                                                {IDS_LINK, Command::Link}}};
    for (const auto &[id, command] : buttons)
        tools.push_back(Action(*commandBar, id, command));
    for (auto *tool : tools)
        tool->SetAppearance(DxUi::ButtonAppearance::Subtle);
    tools[2]->SetVariant(DxUi::ButtonVariant::DropDown);
    tools[8]->SetVariant(DxUi::ButtonVariant::DropDown);
    const std::array<std::pair<unsigned, const wchar_t *>, 10> glyphs{{{0, L"\xE7A7"},
                                                                       {1, L"\xE7A6"},
                                                                       {3, L"\xE8DD"},
                                                                       {4, L"\xE8DB"},
                                                                       {5, L"\xE943"},
                                                                       {6, L"\xE8FD"},
                                                                       {7, L"\xE73A"},
                                                                       {11, L"\xE740"},
                                                                       {12, L"\xE713"},
                                                                       {13, L"\xE71B"}}};
    for (const auto &[index, glyph] : glyphs)
        Icon(tools[index], glyph);
    tools[3]->SetTooltipText(Resource(IDS_TIP_BOLD));
    tools[4]->SetTooltipText(Resource(IDS_TIP_ITALIC));
    tools[5]->SetTooltipText(Resource(IDS_TIP_CODE));
    tools[13]->SetTooltipText(Resource(IDS_TIP_LINK));
    tools[8]->SetLeadingIcon(L"\xE710");
    tools[9]->SetText(Resource(IDS_FIND));
    tools[9]->SetLeadingIcon(L"\xE721");
    tools[9]->SetShortcutText(Resource(IDS_FIND_SHORTCUT));
    settingsToggle = tools[12];
    filesToggle = Action(*commandBar, IDS_FILES, Command::Files, Resource(IDS_TIP_FILES));
    Icon(filesToggle, L"\xE90C");
    filesToggle->SetAppearance(DxUi::ButtonAppearance::Subtle);
    panelToggle = Action(*commandBar, IDS_PANEL, Command::Outline);
    Icon(panelToggle, L"\xE90D");
    panelToggle->SetVariant(DxUi::ButtonVariant::Disclosure);
    panelToggle->SetAppearance(DxUi::ButtonAppearance::Subtle);
    overflowButton = Action(*commandBar, IDS_MORE, Command::Overflow);
    Icon(overflowButton, L"\xE712");
    overflowButton->SetAppearance(DxUi::ButtonAppearance::Subtle);
    for (auto &divider : commandDividers)
        divider = commandBar->AddChild<DxUi::Separator>();
    modeHint = Label(*root, IDS_BANNER, DxUi::FontRole::Small);
    modeHint->SetVisible(false);
    page = root->AddChild<Surface>(Surface::Kind::Canvas);
    editor = page->AddChild<Editor>();
    editor->SetAccessibleName(Resource(IDS_DOC));
    editor->SetAccessibleAutomationId(L"RedTilde.Document");
    editor->SetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown);
    editor->SetMarkdownRenderingEnabled(true);
    editor->SetRemoteImagesEnabled(settings.allowRemoteImages);
    editor->SetViewMode(DxUi::RichText::ViewMode::Rich);
    editor->SetOnTextChanged([this] { Changed(); });
    editor->SetOnSelectionChanged([this] { SelectionChanged(); });
    editor->SetOnContextMenu([this](POINT point, bool) { Menu(point); });
    mirror = page->AddChild<SourceMirrorEditor>(*editor);
    mirror->SetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown);
    mirror->SetMarkdownRenderingEnabled(true);
    mirror->SetReadOnly(true);
    mirror->SetViewMode(DxUi::RichText::ViewMode::Source);
    mirror->SetAccessibleName(Resource(IDS_MIRROR));
    sidebar = root->AddChild<Surface>(Surface::Kind::Sidebar);
    sidebar->borderLeft = true;
    outlineTitle = Label(*sidebar, IDS_OUTLINESECTION, DxUi::FontRole::Small);
    Section(outlineTitle);
    outlineTree = sidebar->AddChild<DxUi::Tree>();
    outlineTree->SetModel(&outlineModel);
    outlineTree->SetDelegate(&outlineModel);
    outlineTree->SetRowHeightDip(42);
    outlineTree->SetIndentDip(14);
    outlineTree->SetAppearance({.drawBackground = false, .drawBorder = false, .showExpanders = false});
    THROW_IF_FAILED(
        outlineTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    outlineTree->SetInvokeOnSingleClick(true);
    outlineTree->SetAccessibleName(Resource(IDS_OUTLINE));
    outlineTree->SetEmptyStateText(Resource(IDS_OUTLINEEMPTY));
    outlineModel.invoked = [this](std::uint64_t id)
    {
        if (!id || id > documentInfo.headings.size())
            return;
        const auto offset = documentInfo.headings[static_cast<std::size_t>(id - 1)].offset;
        editor->SetSelection({offset, offset});
        (void)editor->ScrollSourceRangeIntoView(host, offset, offset, true);
        host.SetFocusControl(editor, !options.headless);
        SelectionChanged();
    };
    outlineModel.expanded = [this](std::uint64_t id, bool value)
    {
        if (value)
            collapsedHeadings.erase(id);
        else
            collapsedHeadings.insert(id);
        if (previewCollapsedHeadings && PreviewOutlineMatchesCommitted())
            previewCollapsedHeadings = collapsedHeadings;
        RefreshOutline();
    };
    currentBlock = Label(*sidebar, IDS_CURRENTBLOCK, DxUi::FontRole::Small);
    listSummary = Label(*sidebar, IDS_BLOCKBULLETLIST, DxUi::FontRole::Small);
    THROW_IF_FAILED(listSummary->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    listSummaryIcon = sidebar->AddChild<DxUi::Label>(L"\xE8FD");
    listSummaryIcon->SetFontRole(DxUi::FontRole::Icon);
    listSummaryIcon->SetAccessibleName(Resource(IDS_BLOCKLIST));
    THROW_IF_FAILED(currentBlock->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    blockSourceAction = Action(*sidebar, IDS_LOCAL, Command::LocalSource);
    blockSourceAction->SetAppearance(DxUi::ButtonAppearance::Outline);
    equationHelp = Label(*sidebar, IDS_EQUATIONSECTION, DxUi::FontRole::Small);
    Section(equationHelp);
    equation = sidebar->AddChild<DxUi::RichTextEditor>();
    equation->SetReadOnly(true);
    equation->SetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown);
    equation->SetMarkdownRenderingEnabled(true);
    equation->SetAccessibleName(Resource(IDS_READONLY));

    currentBlockTitle = Label(*sidebar, IDS_CURRENTSECTION, DxUi::FontRole::Small);
    Section(currentBlockTitle);
    outlineCount = Label(*sidebar, IDS_OUTLINECOUNT, DxUi::FontRole::Small);
    THROW_IF_FAILED(outlineCount->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    outlineCount->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    contextDivider = sidebar->AddChild<DxUi::Separator>();
    equationReadOnly = Label(*sidebar, IDS_READONLYSHORT, DxUi::FontRole::Small);
    filesPane = root->AddChild<Surface>(Surface::Kind::Sidebar);
    filesPane->borderRight = true;
    filesTitle = Label(*filesPane, IDS_FILES, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(filesTitle->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    filesRootIcon = filesPane->AddChild<DxUi::Label>(L"\xE8B7");
    filesRootIcon->SetFontRole(DxUi::FontRole::Icon);
    filesRootIcon->SetAccessibleName(Resource(IDS_FILES));
    filesSectionTitle = Label(*filesPane, IDS_FILESSECTION, DxUi::FontRole::Small);
    Section(filesSectionTitle);
    closeFiles = Action(*filesPane, IDS_CLOSEFILES, Command::Files);
    Icon(closeFiles, L"\xE8BB");
    closeFiles->SetAppearance(DxUi::ButtonAppearance::Subtle);
    THROW_IF_FAILED(closeFiles->SetContentFontSizeDip(12.f));
    filesFilter = filesPane->AddChild<DxUi::TextField>();
    filesFilter->SetAccessibleName(Resource(IDS_FILTERFILES));
    filesFilter->SetPlaceholder(Resource(IDS_FILTERSHORT));
    filesFilter->SetOnTextChanged([this](std::wstring_view) { RefreshFiles(); });
    filesTree = filesPane->AddChild<DxUi::Tree>();
    filesTree->SetModel(&filesModel);
    filesTree->SetDelegate(&filesModel);
    filesTree->SetAccessibleName(Resource(IDS_FILES));
    filesTree->SetRowHeightDip(31);
    filesTree->SetIndentDip(16);
    filesTree->SetAppearance({.drawBackground = false, .drawBorder = false, .reserveExpanderSpace = true, .neutralDecorations = true});
    THROW_IF_FAILED(filesTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    filesTree->SetInvokeOnSingleClick(true);
    filesTree->SetEmptyStateText(Resource(IDS_FILESUNTITLED));
    filesCount = Label(*filesPane, IDS_FILESUNTITLED, DxUi::FontRole::Small);
    THROW_IF_FAILED(filesCount->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    filesCount->SetMultiline(true);
    filesFooterDivider = filesPane->AddChild<DxUi::Separator>();
    filesFooterDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    filesModel.invoked = [this](std::uint64_t id)
    {
        if (id && id <= visibleFilePaths.size())
            OpenPath(visibleFilePaths[static_cast<std::size_t>(id - 1)]);
    };
    filesModel.expanded = [this](std::uint64_t id, bool expanded)
    {
        if (!id || id > visibleFilePaths.size())
            return;
        const auto path = visibleFilePaths[static_cast<std::size_t>(id - 1)];
        if (expanded)
            collapsedFolders.erase(path);
        else
            collapsedFolders.insert(path);
        RefreshFiles();
    };
    filesSplitter = root->AddChild<DxUi::Splitter>();
    filesSplitter->SetSubtle(true);
    filesSplitter->SetAccessibleName(Resource(IDS_FILESWIDTH));
    filesSplitter->SetMinimumFirstPane(200);
    filesSplitter->SetOnChange(
        [this](DxUi::SplitterChange change)
        {
            settings.filesWidth = static_cast<unsigned>(std::clamp(change.position, 200.f, 400.f));
            Layout();
            if (change.phase == DxUi::SplitterChangePhase::Commit)
                SavePreferences();
        });

    drawer = root->AddChild<Surface>(Surface::Kind::Floating);
    drawer->SetVisible(false);
    settingsScroll = drawer->AddChild<DxUi::ScrollPanel>();
    Label(*settingsScroll, IDS_SETTINGS, DxUi::FontRole::Subtitle);
    Section(Label(*settingsScroll, IDS_APPEARANCESECTION, DxUi::FontRole::Small));
    for (unsigned index = 0; index < 4; ++index)
        themeButtons.push_back(
            Action(*settingsScroll, IDS_SYSTEM + index, static_cast<Command>(static_cast<unsigned>(Command::ThemeSystem) + index)));
    readingWidth = settingsScroll->AddChild<DxUi::NumericStepper>();
    readingWidth->SetLabel(Resource(IDS_READINGWIDTH), 140);
    readingWidth->SetMinimum(480);
    readingWidth->SetMaximum(1040);
    readingWidth->SetStep(40);
    readingWidth->SetLargeStep(80);
    readingWidth->SetAccessibleName(Resource(IDS_READINGWIDTH));
    readingWidth->SetStepButtonNames(Resource(IDS_WIDER), Resource(IDS_NARROWER));
    readingWidth->SetOnChange(
        [this](DxUi::NumericStepperChange c)
        {
            settings.readingWidth = static_cast<unsigned>(c.value);
            Layout();
            if (c.phase == DxUi::NumericStepperChangePhase::Commit)
                SavePreferences();
        });
    textScale = settingsScroll->AddChild<DxUi::NumericStepper>();
    textScale->SetLabel(Resource(IDS_TEXTSIZE), 140);
    textScale->SetMinimum(75);
    textScale->SetMaximum(200);
    textScale->SetStep(5);
    textScale->SetAccessibleName(Resource(IDS_TEXTSIZE));
    textScale->SetStepButtonNames(Resource(IDS_LARGER), Resource(IDS_SMALLER));
    textScale->SetOnChange(
        [this](DxUi::NumericStepperChange c)
        {
            settings.textScale = static_cast<unsigned>(c.value);
            Layout();
            if (c.phase == DxUi::NumericStepperChangePhase::Commit)
                SavePreferences();
        });
    outlineToggle = Action(*settingsScroll, IDS_VIEWTOOLS, Command::Outline);
    focusToggle = Action(*settingsScroll, IDS_FOCUS, Command::Focus);
    dimToggle = Action(*settingsScroll, IDS_DIMBLOCKS, Command::DimBlocks);
    Section(Label(*settingsScroll, IDS_PRIVACYSECTION, DxUi::FontRole::Small));
    drawerHelp = Label(*settingsScroll, IDS_LOCALIMAGES, DxUi::FontRole::Small);
    drawerHelp->SetMultiline(true);
    Action(*settingsScroll, IDS_RECOVERY, Command::Recovery);
    Action(*settingsScroll, IDS_CLOSE, Command::CloseDrawer);
    Label(*settingsScroll, IDS_IMMEDIATE, DxUi::FontRole::Small);
    remoteImagesToggle = Action(*settingsScroll, IDS_REMOTEIMAGES, Command::RemoteImages);
    writingTitle = Label(*settingsScroll, IDS_WRITINGSECTION, DxUi::FontRole::Small);
    Section(writingTitle);
    findBar = root->AddChild<Surface>(Surface::Kind::Floating);
    findBar->SetVisible(false);
    findText = findBar->AddChild<DxUi::TextField>();
    findText->SetAccessibleName(Resource(IDS_FINDHINT));
    findText->SetPlaceholder(Resource(IDS_FINDHINT));
    findText->SetOnSubmitted([this] { FindNext(); });
    findText->SetOnTextChanged([this](std::wstring_view) { RefreshFind(); });
    findCount = Label(*findBar, IDS_NOMATCH, DxUi::FontRole::Small);
    Icon(Action(*findBar, IDS_FINDPREVIOUS, Command::FindPrevious), L"\xE70E");
    Icon(Action(*findBar, IDS_FINDNEXT, Command::FindNext), L"\xE70D");
    Icon(Action(*findBar, IDS_CLOSE, Command::CloseDrawer), L"\xE8BB");
    replaceText = findBar->AddChild<DxUi::TextField>();
    replaceText->SetAccessibleName(Resource(IDS_REPLACEHINT));
    replaceText->SetPlaceholder(Resource(IDS_REPLACEHINT));
    Action(*findBar, IDS_REPLACE, Command::Replace);
    Action(*findBar, IDS_REPLACEALL, Command::ReplaceAll);
    blockDraft = root->AddChild<Surface>(Surface::Kind::Floating);
    blockDraft->SetVisible(false);
    Label(*blockDraft, IDS_LOCAL, DxUi::FontRole::BodyStrong);
    Label(*blockDraft, IDS_BLOCKHELP, DxUi::FontRole::Small);
    blockDraftText = blockDraft->AddChild<DxUi::TextField>();
    blockDraftText->SetFontRole(DxUi::FontRole::Monospace);
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
    Action(*blockDraft, IDS_CANCEL, Command::CloseDrawer);
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
    focusPill = root->AddChild<Surface>(Surface::Kind::Floating);
    stats = Label(*focusPill, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(stats->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    positionStatus = Label(*root, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(positionStatus->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    positionStatus->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    feedback = Label(*root, IDS_EMPTY, DxUi::FontRole::Small);
    for (auto *label : {stats, positionStatus, feedback})
        THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    floating = root->AddChild<Surface>(Surface::Kind::Floating);
    for (const auto &[id, cmd] : std::array<std::pair<unsigned, Command>, 5>{{{IDS_HEADING, Command::Heading},
                                                                              {IDS_BOLD, Command::Bold},
                                                                              {IDS_ITALIC, Command::Italic},
                                                                              {IDS_CODE, Command::Code},
                                                                              {IDS_LINK, Command::Link}}})
        floatingActions.push_back(Action(*floating, id, cmd));
    floatingActions.front()->SetVariant(DxUi::ButtonVariant::DropDown);
    selectionHintChip = floating->AddChild<DxUi::CardPanel>();
    selectionHintChip->SetCornerRadius(4);
    selectionHint = Label(*selectionHintChip, IDS_SELECTIONKEY, DxUi::FontRole::Small);
    THROW_IF_FAILED(selectionHint->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    selectionHint->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    floating->SetVisible(false);
    host.SetRoot(std::move(tree));
    host.SetOnEscape([this] { return DismissSurface(); });
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
    const float bar = focus ? 40.f : 56.f, top = focus ? 40.f : 104.f, bottom = std::max(top + 120, height - 28);
    documentBar->SetBounds(D2D1::RectF(0, 0, width, bar));
    commandBar->SetBounds(D2D1::RectF(0, 56, width, 104));
    commandBar->SetVisible(!focus);
    brandMark->SetVisible(!focus);
    brandMark->SetBounds(D2D1::RectF(16, 14, 44, 42));
    const bool compactHeader = width < 1040;
    const float tabsLeft = compactHeader ? width - 520 : width / 2 - 104;
    const float titleLimit = std::max(56.f, tabsLeft - 120.f);
    const auto typography = host.GetTypographyRevision();
    const float identityWidth = titleLimit - 54.f;
    if (typography && (identityMeasure.textRevision != title->GetMeasureRevision() || identityMeasure.typographyRevision != typography ||
                       identityMeasure.availableWidth != identityWidth || identityMeasure.dpi != host.GetDpi()))
    {
        DxUi::DesiredSize identitySize;
        THROW_IF_FAILED(DxUi::MeasureControl(*title, {host}, DxUi::LayoutConstraints::AtMost(identityWidth), identitySize));
        identityMeasure = {title->GetMeasureRevision(), host.GetTypographyRevision(), identityWidth, identitySize.sizeDip.width, host.GetDpi()};
    }
    const float titleRight = typography ? std::min(titleLimit, 54.f + identityMeasure.width) : titleLimit;
    title->SetBounds(focus ? D2D1::RectF(20, 6, width - 320, 34) : D2D1::RectF(54, 16, titleRight, 40));
    saveStateChip->SetVisible(!focus);
    saveStateChip->SetBounds(D2D1::RectF(titleRight + 10, 18, titleRight + 62, 38));
    saveState->SetBounds(D2D1::RectF(titleRight + 14, 18, titleRight + 58, 38));
    constexpr std::array<float, 3> tabOffsets{0.f, 92.f, 170.f}, tabWidths{88.f, 74.f, 54.f};
    for (unsigned index = 0; index < 3; ++index)
    {
        modeButtons[index]->SetVisible(!focus);
        modeButtons[index]->SetBounds(D2D1::RectF(tabsLeft + tabOffsets[index], 8, tabsLeft + tabOffsets[index] + tabWidths[index], 46));
        modeButtons[index]->SetToggleState(static_cast<unsigned>(settings.view) == index);
    }
    fileMenu->SetVisible(!focus);
    fileMenu->SetBounds(D2D1::RectF(width - 294, 8, width - 230, 48));
    saveButton->SetVisible(!focus);
    saveButton->SetBounds(D2D1::RectF(width - 217, 8, width - 146, 48));
    exitFocus->SetVisible(focus);
    exitFocus->SetBounds(D2D1::RectF(width - 288, 4, width - 144, 36));
    const std::array<DxUi::Button *, 3> captions{minimizeButton, maximizeButton, closeButton};
    for (unsigned i = 0; i < 3; ++i)
        captions[i]->SetBounds(D2D1::RectF(width - 138 + i * 46, 0, width - 92 + i * 46, std::min(36.f, bar)));
    maximizeButton->SetText(IsZoomed(window.get()) ? L"\xE923" : L"\xE922");
    float x = 12;
    auto place = [&](DxUi::Button *button, float w)
    {
        button->SetBounds(D2D1::RectF(x, 62, x + w, 98));
        x += w + 2;
    };
    place(filesToggle, 36);
    auto divide = [&](unsigned index)
    {
        commandDividers[index]->SetVisible(true);
        commandDividers[index]->SetBounds(D2D1::RectF(x + 8, 70, x + 9, 90));
        x += 19;
    };
    divide(0);
    place(tools[0], 36);
    place(tools[1], 36);
    divide(1);
    place(tools[2], 128);
    divide(2);
    const float commandLimit = width - 248;
    commandDividers[3]->SetVisible(false);
    commandDividers[4]->SetVisible(false);
    for (const unsigned i : {3u, 4u, 5u, 13u, 6u, 7u, 8u})
    {
        const float w = i == 8 ? 100.f : 36.f;
        const bool fits = x + w + 44 <= commandLimit;
        tools[i]->SetVisible(fits);
        if (fits)
            place(tools[i], w);
        if ((i == 13 || i == 7) && fits)
            divide(i == 13 ? 3 : 4);
    }
    tools[10]->SetVisible(false);
    const bool overflow = std::ranges::any_of(std::array<unsigned, 7>{3, 4, 5, 13, 6, 7, 8}, [this](unsigned i) { return !tools[i]->IsVisible(); });
    overflowButton->SetVisible(overflow);
    if (overflow)
        place(overflowButton, 36);
    commandDividers[5]->SetVisible(false);
    x = width - 248;
    place(tools[9], 120);
    place(tools[11], 36);
    place(panelToggle, 36);
    place(settingsToggle, 36);
    filesToggle->SetToggleState(settings.showFiles);
    panelToggle->SetToggleState(settings.showOutline);
    settingsToggle->SetToggleState(drawer->IsVisible());
    tools[11]->SetToggleState(focus);
    const bool fileOverlay = settings.showFiles && width < static_cast<float>(settings.readingWidth + settings.filesWidth + 96 + 6);
    const float filesWidth = static_cast<float>(settings.filesWidth);
    const float left = settings.showFiles && !fileOverlay && !focus ? filesWidth : 0;
    const bool showSidebar = settings.showOutline && !focus && !fileOverlay && width - left >= static_cast<float>(settings.readingWidth + 336) &&
                             settings.view != EditorView::Split;
    const float right = showSidebar ? width - 240 : width;
    const auto layoutRequest = activity.layouts;
    const bool sidebarWasVisible = sidebar->IsVisible();
    if (showSidebar && !sidebarWasVisible && !PublishCurrentBlockLabelIfVisible(true))
        return;
    if (activity.layouts != layoutRequest)
        return;
    sidebar->SetVisible(showSidebar);
    if (activity.layouts != layoutRequest)
        return;
    sidebar->SetBounds(D2D1::RectF(width - 240, top, width, bottom));
    const float side = width - 224;
    outlineTitle->SetBounds(D2D1::RectF(side + 4, top + 16, width - 100, top + 40));
    outlineCount->SetBounds(D2D1::RectF(width - 104, top + 16, width - 16, top + 40));
    const float contextHeight = equationContext ? 260.f : 128.f;
    const float outlineHeight = std::max(40.f, static_cast<float>(outlineModel.rows.size()) * 42.f + 4.f);
    const float outlineBottom = std::min(top + 44 + outlineHeight, std::max(top + 92, bottom - contextHeight));
    outlineTree->SetBounds(D2D1::RectF(side - 4, top + 44, width - 8, outlineBottom));
    contextDivider->SetBounds(D2D1::RectF(side + 4, outlineBottom + 12, width - 12, outlineBottom + 13));
    contextDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    currentBlockTitle->SetBounds(D2D1::RectF(side + 4, outlineBottom + 28, width - 16, outlineBottom + 48));
    currentBlock->SetBounds(D2D1::RectF(side + 4, outlineBottom + 52, width - 16, outlineBottom + 76));
    if (showSidebar && !PublishCurrentBlockLabelIfVisible())
        return;
    if (activity.layouts != layoutRequest)
        return;
    listSummaryIcon->SetBounds(D2D1::RectF(side + 4, outlineBottom + 52, side + 20, outlineBottom + 76));
    listSummary->SetBounds(D2D1::RectF(side + 28, outlineBottom + 52, width - 16, outlineBottom + 76));
    blockSourceAction->SetBounds(D2D1::RectF(side + 4, outlineBottom + 84, width - 20, outlineBottom + 116));
    equationHelp->SetVisible(equationContext);
    equation->SetVisible(equationContext);
    equationReadOnly->SetVisible(equationContext);
    equationHelp->SetBounds(D2D1::RectF(side, outlineBottom + 132, width - 100, outlineBottom + 156));
    equationReadOnly->SetBounds(D2D1::RectF(width - 96, outlineBottom + 132, width - 16, outlineBottom + 156));
    equation->SetBounds(D2D1::RectF(side, outlineBottom + 164, width - 16, bottom - 16));
    page->SetBounds(D2D1::RectF(left, top, right, bottom));
    const bool split = settings.view == EditorView::Split;
    const float mid = (left + right) / 2;
    const float gutter = focus ? 32.f : split ? 24.f : 48.f;
    editor->SetBounds(D2D1::RectF(left + gutter, top + (focus ? 36.f : 60.f), (split ? mid : right) - gutter, bottom));
    mirror->SetVisible(split);
    mirror->SetBounds(D2D1::RectF(mid + 24, top + 56, right - 24, bottom - 24));
    DxUi::RichTextEditorAppearance appearance;
    appearance.drawBorder = false;
    appearance.drawFocusIndicator = false;
    appearance.useAccentLinkColor = true;
    appearance.paddingDip = 0;
    appearance.maximumTextWidthDip = focus ? 680.f : static_cast<float>(settings.readingWidth);
    appearance.bodyFontSizeDip = (focus ? 18.f : split ? 15.f : 16.f) * settings.textScale / 100.f;
    appearance.codeFontSizeDip = 13.5f * settings.textScale / 100.f;
    appearance.lineHeightMultiplier = settings.view == EditorView::Source ? 1.75f : 1.65f;
    appearance.headingFontSizeDip = {34.f * settings.textScale / 100.f, 23.f * settings.textScale / 100.f, 20.f * settings.textScale / 100.f,
                                     18.f * settings.textScale / 100.f, 16.f * settings.textScale / 100.f, 16.f * settings.textScale / 100.f};
    appearance.headingLineHeightMultiplier = {1.2f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f};
    appearance.paragraphSpacingAfterLines = 18.f / 26.4f;
    appearance.listItemSpacingDip = 10.f;
    appearance.semanticSpacing = DxUi::RichText::SemanticSpacing{
        .headingBeforeDip = {0.f, 40.f, 32.f, 24.f, 20.f, 20.f}, .headingAfterDip = {0.f, 14.f, 14.f, 14.f, 14.f, 14.f}, .containerIndentDip = 10.f};
    auto sourceAppearance = appearance;
    sourceAppearance.bodyFontSizeDip = appearance.codeFontSizeDip;
    sourceAppearance.colorizeMarkdownSource = true;
    sourceAppearance.lineHeightMultiplier = 1.75f;
    // The canonical monospace role and its fallback use the same family.
    // Reading that policy needs no host text format or graphics initialization.
    sourceAppearance.bodyFontFamily = DxUi::Typography::GetSpec(DxUi::FontRole::Monospace).familyName;
    editor->SetAppearance(settings.view == EditorView::Source ? sourceAppearance : appearance);
    editor->SetFocusModeEnabled(focus && settings.dimInactiveBlocks);
    mirror->SetAppearance(sourceAppearance);
    equation->SetAppearance(appearance);
    filesPane->SetVisible(settings.showFiles && !focus);
    filesPane->borderRight = fileOverlay;
    filesPane->SetBounds(D2D1::RectF(0, top, filesWidth, bottom));
    filesSectionTitle->SetBounds(D2D1::RectF(20, top + 16, filesWidth - 52, top + 40));
    closeFiles->SetBounds(D2D1::RectF(filesWidth - 44, top + 14, filesWidth - 12, top + 46));
    filesRootIcon->SetBounds(D2D1::RectF(20, top + 52, 36, top + 78));
    filesTitle->SetBounds(D2D1::RectF(44, top + 52, filesWidth - 16, top + 78));
    filesFilter->SetBounds(D2D1::RectF(20, top + 84, filesWidth - 12, top + 114));
    const float footerWidth = filesWidth - 24;
    const auto footerRevision = filesCount->GetMeasureRevision(), typographyRevision = host.GetTypographyRevision();
    const auto footerDpi = host.GetDpi();
    // Keep hidden measurement keys stale; the first visible layout measures
    // current text/width/typography/DPI before reserving the footer's bounds.
    if (filesPane->IsVisible() && (filesFooterMeasure.widthDip != footerWidth || filesFooterMeasure.textRevision != footerRevision ||
                                   filesFooterMeasure.typographyRevision != typographyRevision || filesFooterMeasure.dpi != footerDpi))
    {
        DxUi::DesiredSize desired;
        const HRESULT measured = DxUi::MeasureControl(*filesCount, {host}, DxUi::LayoutConstraints::AtMost(footerWidth), desired);
        if (measured == E_OUTOFMEMORY)
            std::terminate();
        THROW_IF_FAILED(measured);
        filesFooterMeasure.widthDip = footerWidth;
        filesFooterMeasure.heightDip = std::max(24.f, std::ceil(desired.sizeDip.height));
        filesFooterMeasure.textRevision = footerRevision;
        // A cold MeasureControl may initialize the host's typography itself.
        filesFooterMeasure.typographyRevision = host.GetTypographyRevision();
        filesFooterMeasure.dpi = footerDpi;
        ++filesFooterMeasure.measurements;
    }
    const float footerTop = bottom - 8 - filesFooterMeasure.heightDip;
    filesFooterDivider->SetBounds(D2D1::RectF(0, footerTop - 8, filesWidth, footerTop - 7));
    filesTree->SetBounds(D2D1::RectF(4, top + 122, filesWidth - 4, footerTop - 4));
    filesCount->SetBounds(D2D1::RectF(12, footerTop, filesWidth - 12, bottom - 8));
    filesSplitter->SetVisible(settings.showFiles && !focus && !fileOverlay);
    filesSplitter->SetBounds(D2D1::RectF(0, top, width, bottom));
    filesSplitter->SetMinimumSecondPane(std::max(48.f, width - 406));
    filesSplitter->SetPosition(filesWidth);
    const float drawerLeft = width - 380;
    drawer->SetBounds(D2D1::RectF(drawerLeft, top, width, bottom));
    settingsScroll->SetBounds(D2D1::RectF(drawerLeft + 16, top + 12, width - 8, bottom - 12));
    settingsScroll->SetContentHeight(782);
    const float dx = drawerLeft + 24;
    settingsScroll->GetLogicalChild(0)->SetBounds(D2D1::RectF(dx, top + 20, width - 24, top + 52));
    settingsScroll->GetLogicalChild(1)->SetBounds(D2D1::RectF(dx, top + 68, width - 24, top + 92));
    std::array<DxUi::ButtonThemePreview, 4> previews{{{},
                                                      {{D2D1::ColorF(0xF0F1F4), D2D1::ColorF(0xF8F8FA), D2D1::ColorF(0xFFFFFF)}},
                                                      {{D2D1::ColorF(0x17191E), D2D1::ColorF(0x1C1F25), D2D1::ColorF(0x22252B)}},
                                                      {{D2D1::ColorF(0xEDE6D8), D2D1::ColorF(0xF3EDDF), D2D1::ColorF(0xFFFAEF)}}}};
    previews[0] = previews[systemDark ? 2 : 1];
    for (unsigned i = 0; i < 4; ++i)
    {
        themeButtons[i]->SetBounds(D2D1::RectF(dx + (i % 2) * 168, top + 104 + (i / 2) * 80, dx + 156 + (i % 2) * 168, top + 168 + (i / 2) * 80));
        themeButtons[i]->SetThemePreview(previews[i]);
        themeButtons[i]->SetToggleState(static_cast<unsigned>(settings.appearance) == i);
    }
    readingWidth->SetBounds(D2D1::RectF(dx, top + 272, width - 24, top + 308));
    if (!readingWidth->IsEditing())
        readingWidth->SetValue(settings.readingWidth);
    textScale->SetBounds(D2D1::RectF(dx, top + 320, width - 24, top + 356));
    if (!textScale->IsEditing())
        textScale->SetValue(settings.textScale);
    writingTitle->SetBounds(D2D1::RectF(dx, top + 372, width - 24, top + 396));
    outlineToggle->SetBounds(D2D1::RectF(dx, top + 408, width - 24, top + 444));
    outlineToggle->SetToggleState(settings.showOutline);
    focusToggle->SetBounds(D2D1::RectF(dx, top + 456, width - 24, top + 492));
    focusToggle->SetToggleState(focus);
    dimToggle->SetBounds(D2D1::RectF(dx, top + 504, width - 24, top + 540));
    dimToggle->SetToggleState(settings.dimInactiveBlocks);
    settingsScroll->GetLogicalChild(11)->SetBounds(D2D1::RectF(dx, top + 556, width - 24, top + 580));
    remoteImagesToggle->SetBounds(D2D1::RectF(dx, top + 592, width - 24, top + 628));
    remoteImagesToggle->SetToggleState(settings.allowRemoteImages);
    drawerHelp->SetBounds(D2D1::RectF(dx, top + 636, width - 24, top + 676));
    settingsScroll->GetLogicalChild(13)->SetBounds(D2D1::RectF(dx, top + 692, width - 24, top + 728));
    settingsScroll->GetLogicalChild(14)->SetBounds(D2D1::RectF(dx + 224, top + 740, width - 24, top + 776));
    settingsScroll->GetLogicalChild(15)->SetBounds(D2D1::RectF(dx, top + 740, width - 160, top + 776));
    const float fx = right - 476, fy = top + 12;
    findBar->SetBounds(D2D1::RectF(fx, fy, right - 16, fy + 104));
    findText->SetBounds(D2D1::RectF(fx + 12, fy + 10, fx + 204, fy + 46));
    findCount->SetBounds(D2D1::RectF(fx + 212, fy + 14, fx + 310, fy + 42));
    for (unsigned i = 0; i < 3; ++i)
        findBar->GetLogicalChild(i + 2)->SetBounds(D2D1::RectF(fx + 314 + i * 44, fy + 10, fx + 354 + i * 44, fy + 46));
    replaceText->SetBounds(D2D1::RectF(fx + 12, fy + 58, fx + 236, fy + 94));
    findBar->GetLogicalChild(6)->SetBounds(D2D1::RectF(fx + 244, fy + 58, fx + 326, fy + 94));
    findBar->GetLogicalChild(7)->SetBounds(D2D1::RectF(fx + 334, fy + 58, fx + 448, fy + 94));
    const float draftLeft = std::max(left + 16, (left + right - 680) / 2), draftRight = std::min(right - 16, draftLeft + 680);
    float draftTop = top + 72;
    pendingBlockPlacement = false;
    if (blockDraft->IsVisible())
    {
        if (const auto rects = editor->TryGetTextInputRangeRects(host, blockDraftRange.first, blockDraftRange.last); rects && !rects->empty())
            draftTop = rects->back().bottom + 8;
        else if (const auto caret = editor->TryGetTextInputCaretRect(host, blockDraftRange.first))
            draftTop = caret->bottom + 8; // Empty/hidden-only source still has a prepared insertion site.
        else
            pendingBlockPlacement = true;
    }
    draftTop = std::clamp(draftTop, top + 8, std::max(top + 8, bottom - 244));
    blockDraft->SetBounds(D2D1::RectF(draftLeft, draftTop, draftRight, draftTop + 232));
    blockDraft->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 8, draftRight - 16, draftTop + 36));
    blockDraft->GetLogicalChild(1)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 40, draftRight - 16, draftTop + 64));
    blockDraftText->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 72, draftRight - 16, draftTop + 172));
    blockDraft->GetLogicalChild(3)->SetBounds(D2D1::RectF(draftRight - 244, draftTop + 184, draftRight - 112, draftTop + 220));
    blockDraft->GetLogicalChild(4)->SetBounds(D2D1::RectF(draftRight - 100, draftTop + 184, draftRight - 16, draftTop + 220));
    recoveryPanel->SetBounds(D2D1::RectF(draftLeft, top + 8, draftRight, bottom - 8));
    recoveryPanel->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 20, top + 24, draftRight - 20, top + 60));
    recoveryDescription->SetBounds(D2D1::RectF(draftLeft + 20, top + 68, draftRight - 20, top + 128));
    recoveryPreview->SetBounds(D2D1::RectF(draftLeft + 20, top + 136, draftRight - 20, bottom - 80));
    for (unsigned i = 0; i < 3; ++i)
        recoveryPanel->GetLogicalChild(i + 3)->SetBounds(D2D1::RectF(draftLeft + 20 + i * 208, bottom - 62, draftLeft + 220 + i * 208, bottom - 28));
    stats->SetBounds(D2D1::RectF(16, height - 26, width / 2, height - 2));
    positionStatus->SetBounds(D2D1::RectF(width / 2, height - 26, width - 16, height - 2));
    focusPill->SetBounds(focus ? D2D1::RectF(width / 2 - 224, height - 50, width / 2 + 224, height - 10)
                               : D2D1::RectF(0, height - 28, width / 2, height));
    focusPill->kind = focus ? Surface::Kind::Floating : Surface::Kind::Chrome;
    if (focus)
        stats->SetBounds(D2D1::RectF(width / 2 - 208, height - 44, width / 2 + 208, height - 16));
    UpdateCounts();
    title->SetTextColor(focus ? std::optional(host.GetTheme().subduedText) : std::nullopt);
    stats->SetVisible(true);
    positionStatus->SetVisible(!focus);
    const auto name = documentPath.empty() ? Resource(IDS_UNTITLED) : documentPath.filename().native();
    title->SetText(focus && Dirty() ? name + L" · " + Resource(IDS_CHANGED) : name);
    feedback->SetBounds(D2D1::RectF(left + 16, bottom - 22, right - 16, bottom));
    // Reposition only an already visible toolbar: resize must not reopen
    // tools that Escape or another contextual surface deliberately dismissed.
    if (floating->IsVisible())
        SelectionChanged();
    host.Invalidate();
}
} // namespace RedTilde
