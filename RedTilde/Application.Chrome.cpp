#include "Application.h"
#include "Common/Version.h"
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
    THROW_HR_IF(E_UNEXPECTED,
                native.bmWidth <= 0 || native.bmWidth > 256 || native.bmHeight <= 0 || native.bmHeight > 256 || native.bmPlanes != 1 ||
                    native.bmBitsPixel != 32);

    BrandMarkPixels result{static_cast<UINT>(native.bmWidth), static_cast<UINT>(native.bmHeight), {}};
    result.bgra.resize(static_cast<std::size_t>(result.width) * result.height * 4);
    BITMAPINFO bitmap{};
    bitmap.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth       = native.bmWidth;
    bitmap.bmiHeader.biHeight      = -native.bmHeight;
    bitmap.bmiHeader.biPlanes      = 1;
    bitmap.bmiHeader.biBitCount    = 32;
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

HRESULT SourceMirrorEditor::PrepareContent(DxUi::ControlHost& host) noexcept
{
    const auto selfLifetime    = GetControlLifetimeToken(*this);
    auto* const primary        = _primary;
    const auto primaryLifetime = _primaryLifetime;
    auto* const owner          = host.GetRoot();
    if (! primary || primaryLifetime.expired() || ! owner || ! IsVisible() || ! IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source ||
        GetHost() != &host || ! primary->IsAttachedTo(host, GetParent()))
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto ownerLifetime = GetControlLifetimeToken(*owner);
    if (ownerLifetime.expired())
        return DxUi::RichTextEditor::PrepareContent(host);

    const auto source          = primary->Snapshot();
    const auto primaryRevision = primary->Revision();
    const auto syntax          = primary->GetMarkdownSyntax();
    if (syntax && syntax->Revision() == primaryRevision && source.IsSameRevision(primary->Snapshot()))
    {
        const auto currentSyntax  = GetMarkdownSyntax();
        const bool alreadyAdopted = currentSyntax && Snapshot().IsSameRevision(source) && currentSyntax->Revision() == Revision() &&
                                    currentSyntax->DocumentIdentity() != syntax->DocumentIdentity() && &currentSyntax->Nodes() == &syntax->Nodes();
        if (! alreadyAdopted)
        {
            const auto targetRevision = Revision();
            const HRESULT result      = TryAdoptReadOnlyMarkdownSyntax(source, syntax);
            if (FAILED(result))
                return result;

            // Adoption's final preparation request can re-enter and retire
            // either editor or replace the owning root. Prove all borrowers
            // still belong to this host before the ordinary preparation call.
            if (selfLifetime.expired() || primaryLifetime.expired() || ownerLifetime.expired())
                return S_FALSE;
            if (GetHost() != &host || ! primary->IsAttachedTo(host, GetParent()) || host.GetRoot() != owner || ! source.IsSameRevision(primary->Snapshot()) ||
                primary->Revision() != primaryRevision)
                return S_FALSE;
            if (result == S_OK && (! IsVisible() || ! IsReadOnly() || GetViewMode() != DxUi::RichText::ViewMode::Source || Revision() != targetRevision ||
                                   ! Snapshot().IsSameRevision(source) || GetMarkdownAnalysisProfile() != syntax->Profile()))
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
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
class BrandMark final : public DxUi::Control
{
    BrandMarkPixels _source;
    wil::com_ptr<ID2D1Bitmap1> _bitmap;
    std::uint64_t _bitmapGeneration{};

public:
    BrandMark()
    {
        EnableVisualContentPreparation();
        // LoadIcon returns a module-owned shared icon. Retain just its pixels;
        // ordinary startup needs no WIC factory/converter for this known asset.
        const auto icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_REDTILDE));
        THROW_LAST_ERROR_IF_NULL(icon);
        _source = LoadBrandMarkPixels(icon);
    }
    void PrepareVisualContent(DxUi::ControlHost& host) noexcept override
    {
        const auto generation = host.GetResourceGeneration();
        if (_bitmap && _bitmapGeneration == generation)
            return;
        _bitmap.reset();
        _bitmapGeneration     = 0;
        auto* target          = host.GetDeviceContext();
        const auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (target &&
            SUCCEEDED(target->CreateBitmap(D2D1::SizeU(_source.width, _source.height), _source.bgra.data(), _source.width * 4, properties, _bitmap.put())))
            _bitmapGeneration = generation;
    }
    void CancelVisualContentPreparation() noexcept override
    {
        _bitmap.reset();
        _bitmapGeneration = 0;
    }
    void Paint(DxUi::ControlHost& host) const override
    {
        auto* target = host.GetDeviceContext();
        if (! target || ! _bitmap || _bitmapGeneration != host.GetResourceGeneration())
            return;
        target->DrawBitmap(_bitmap.get(), GetBounds());
    }
};
#pragma warning(pop)
void Section(DxUi::Label* label)
{
    THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, .88f}));
}
void Icon(DxUi::Button* button, std::wstring glyph)
{
    button->SetVariant(DxUi::ButtonVariant::IconOnly);
    THROW_IF_FAILED(button->SetContentFontSizeDip(18.f));
    button->SetText(std::move(glyph));
}
} // namespace

HRESULT Application::BuildUi()
{
    const DxUi::Failure::Scope constructionScope;
    auto tree    = std::make_unique<Surface>();
    _root        = tree.get();
    _documentBar = _root->AddChild<Surface>();
    _brandMark   = _documentBar->AddChild<BrandMark>();
    _title       = Label(*_documentBar, IDS_UNTITLED, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_title->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _saveStateChip = _documentBar->AddChild<DxUi::CardPanel>();
    _saveStateChip->SetCornerRadius(10.f);
    _saveState = Label(*_saveStateChip, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_saveState->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _saveState->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    for (unsigned index = 0; index < 3; ++index)
    {
        _modeButtons[index] = Action(*_documentBar, IDS_COMPOSE + index, static_cast<Command>(static_cast<unsigned>(Command::Compose) + index));
        _modeButtons[index]->SetAccessibleAutomationId(std::format(L"RedTilde.View{}", index));
        _modeButtons[index]->SetAppearance(DxUi::ButtonAppearance::Tab);
    }
    _fileMenu = Action(*_documentBar, IDS_FILE, Command::FileMenu);
    _fileMenu->SetVariant(DxUi::ButtonVariant::DropDown);
    _fileMenu->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _saveButton = Action(*_documentBar, IDS_SAVE, Command::Save, Resource(IDS_TIP_SAVE));
    _saveButton->SetPrimary(true);
    _exitFocus      = Action(*_documentBar, IDS_EXITFOCUS, Command::Focus);
    _minimizeButton = Action(*_documentBar, IDS_MINIMIZE, Command::Minimize);
    _maximizeButton = Action(*_documentBar, IDS_MAXIMIZE, Command::Maximize);
    _closeButton    = Action(*_documentBar, IDS_CLOSE, Command::Close);
    Icon(_minimizeButton, L"\xE921");
    Icon(_maximizeButton, L"\xE922");
    Icon(_closeButton, L"\xE8BB");
    for (auto* caption : {_minimizeButton, _maximizeButton})
    {
        caption->SetFocusable(false);
        caption->SetAppearance(DxUi::ButtonAppearance::Caption);
        THROW_IF_FAILED(caption->SetContentFontSizeDip(12.f));
    }
    _closeButton->SetFocusable(false);
    _closeButton->SetAppearance(DxUi::ButtonAppearance::CaptionClose);
    THROW_IF_FAILED(_closeButton->SetContentFontSizeDip(12.f));
    _commandBar               = _root->AddChild<Surface>();
    _commandBar->borderBottom = true;
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
    for (const auto& [id, command] : buttons)
        _tools.push_back(Action(*_commandBar, id, command));
    for (auto* tool : _tools)
        tool->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _tools[2]->SetVariant(DxUi::ButtonVariant::DropDown);
    _tools[8]->SetVariant(DxUi::ButtonVariant::DropDown);
    const std::array<std::pair<unsigned, const wchar_t*>, 10> glyphs{{{0, L"\xE7A7"},
                                                                      {1, L"\xE7A6"},
                                                                      {3, L"\xE8DD"},
                                                                      {4, L"\xE8DB"},
                                                                      {5, L"\xE943"},
                                                                      {6, L"\xE8FD"},
                                                                      {7, L"\xE73A"},
                                                                      {11, L"\xE740"},
                                                                      {12, L"\xE713"},
                                                                      {13, L"\xE71B"}}};
    for (const auto& [index, glyph] : glyphs)
        Icon(_tools[index], glyph);
    _tools[3]->SetTooltipText(Resource(IDS_TIP_BOLD));
    _tools[4]->SetTooltipText(Resource(IDS_TIP_ITALIC));
    _tools[5]->SetTooltipText(Resource(IDS_TIP_CODE));
    _tools[13]->SetTooltipText(Resource(IDS_TIP_LINK));
    _tools[8]->SetLeadingIcon(L"\xE710");
    _tools[9]->SetText(Resource(IDS_FIND));
    _tools[9]->SetLeadingIcon(L"\xE721");
    _tools[9]->SetShortcutText(Resource(IDS_FIND_SHORTCUT));
    _settingsToggle = _tools[12];
    _filesToggle    = Action(*_commandBar, IDS_FILES, Command::Files, Resource(IDS_TIP_FILES));
    Icon(_filesToggle, L"\xE90C");
    _filesToggle->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _panelToggle = Action(*_commandBar, IDS_PANEL, Command::Outline);
    Icon(_panelToggle, L"\xE90D");
    _panelToggle->SetVariant(DxUi::ButtonVariant::Disclosure);
    _panelToggle->SetAppearance(DxUi::ButtonAppearance::Subtle);
    _overflowButton = Action(*_commandBar, IDS_MORE, Command::Overflow);
    Icon(_overflowButton, L"\xE712");
    _overflowButton->SetAppearance(DxUi::ButtonAppearance::Subtle);
    for (auto& divider : _commandDividers)
        divider = _commandBar->AddChild<DxUi::Separator>();
    _modeHint = Label(*_root, IDS_BANNER, DxUi::FontRole::Small);
    _modeHint->SetVisible(false);
    _page   = _root->AddChild<Surface>(Surface::Kind::Canvas);
    _editor = _page->AddChild<Editor>();
    _editor->SetAccessibleName(Resource(IDS_DOC));
    _editor->SetAccessibleAutomationId(L"RedTilde.Document");
    if (! AcceptSourceResult(_editor->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_editor->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _editor->SetCodeTokenizer(DxUi::RichText::TokenizeBuiltinCode);
    _editor->SetRemoteImagesEnabled(_settings.allowRemoteImages);
    if (! AcceptSourceResult(_editor->TrySetViewMode(DxUi::RichText::ViewMode::Rich)))
        return E_INVALIDARG;
    _editor->SetOnTextChanged([this] { Changed(); });
    _editor->SetOnSelectionChanged([this] { SelectionChanged(); });
    _editor->SetBitmapPasteHandler([this](std::span<const std::byte> dib, bool v5) { return BeginBitmapPaste(dib, v5); });
    _editor->SetOnContextMenu([this](POINT point, bool) { Menu(point); });
    _editor->SetOnLinkActivated([this](const DxUi::RichTextLinkActivation& activation) { RunUiAction([&] { RequestOpenLink(activation); }); });
    _mirror = _page->AddChild<SourceMirrorEditor>(*_editor);
    if (! AcceptSourceResult(_mirror->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_mirror->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _mirror->SetReadOnly(true);
    _imageConsent = _page->AddChild<Surface>(Surface::Kind::Floating);
    _imageConsent->SetVisible(false);
    _imageConsent->SetAccessibleAutomationId(L"RedTilde.RemoteImageConsent");
    _imageConsentText = Label(*_imageConsent, IDS_IMAGECONSENTBLOCKED, DxUi::FontRole::Small);
    _imageConsentText->SetMultiline(true);
    _imageConsentAction = Action(*_imageConsent, IDS_IMAGEALLOWHERE, Command::DocumentImages);
    _imageConsentAction->SetAccessibleAutomationId(L"RedTilde.AllowDocumentImages");
    _imageReload = Action(*_imageConsent, IDS_IMAGERELOAD, Command::ReloadImages);
    _imageReload->SetAccessibleAutomationId(L"RedTilde.ReloadImages");
    if (! AcceptSourceResult(_mirror->TrySetViewMode(DxUi::RichText::ViewMode::Source)))
        return E_INVALIDARG;
    _mirror->SetAccessibleName(Resource(IDS_MIRROR));
    _sidebar             = _root->AddChild<Surface>(Surface::Kind::Sidebar);
    _sidebar->borderLeft = true;
    _outlineTitle        = Label(*_sidebar, IDS_OUTLINESECTION, DxUi::FontRole::Small);
    Section(_outlineTitle);
    _outlineTree = _sidebar->AddChild<DxUi::Tree>();
    _outlineTree->SetModel(&_outlineModel);
    _outlineTree->SetDelegate(&_outlineModel);
    _outlineTree->SetRowHeightDip(42);
    _outlineTree->SetIndentDip(14);
    _outlineTree->SetAppearance({.drawBackground = false, .drawBorder = false, .showExpanders = false});
    THROW_IF_FAILED(_outlineTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    _outlineTree->SetInvokeOnSingleClick(true);
    _outlineTree->SetAccessibleName(Resource(IDS_OUTLINE));
    _outlineTree->SetEmptyStateText(Resource(IDS_OUTLINEEMPTY));
    _outlineModel.invoked = [this](std::uint64_t id)
    {
        if (! id || id > _documentInfo.headings.size())
            return;
        const auto offset = _documentInfo.headings[static_cast<std::size_t>(id - 1)].offset;
        if (! AcceptSourceResult(_editor->TrySetSelection({offset, offset})))
            return;
        (void)_editor->ScrollSourceRangeIntoView(_host, offset, offset, true);
        _host.SetFocusControl(_editor, MayRequestNativeFocus());
        SelectionChanged();
    };
    _outlineModel.expanded = [this](std::uint64_t id, bool value)
    {
        if (value)
            _session.collapsedHeadings.erase(id);
        else
            _session.collapsedHeadings.insert(id);
        if (_outlinePreviewBaseline && PreviewOutlineMatchesCommitted())
            _outlinePreviewBaseline->collapsedHeadings = _session.collapsedHeadings;
        RefreshOutline();
    };
    _currentBlock = Label(*_sidebar, IDS_CURRENTBLOCK, DxUi::FontRole::Small);
    _listSummary  = Label(*_sidebar, IDS_BLOCKBULLETLIST, DxUi::FontRole::Small);
    THROW_IF_FAILED(_listSummary->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _listSummaryIcon = _sidebar->AddChild<DxUi::Label>(L"\xE8FD");
    _listSummaryIcon->SetFontRole(DxUi::FontRole::Icon);
    _listSummaryIcon->SetAccessibleName(Resource(IDS_BLOCKLIST));
    THROW_IF_FAILED(_currentBlock->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _blockSourceAction = Action(*_sidebar, IDS_LOCAL, Command::LocalSource);
    _blockSourceAction->SetAppearance(DxUi::ButtonAppearance::Outline);
    _equationHelp = Label(*_sidebar, IDS_EQUATIONSECTION, DxUi::FontRole::Small);
    Section(_equationHelp);
    _equation = _sidebar->AddChild<DxUi::RichTextEditor>();
    _equation->SetReadOnly(true);
    if (! AcceptSourceResult(_equation->TrySetMarkdownAnalysisProfile(DxUi::RichText::MarkdownProfile::RichMarkdown)) ||
        ! AcceptSourceResult(_equation->TrySetMarkdownRenderingEnabled(true)))
        return E_INVALIDARG;
    _equation->SetAccessibleName(Resource(IDS_READONLY));
    ApplyMarkdownSyntax();

    _currentBlockTitle = Label(*_sidebar, IDS_CURRENTSECTION, DxUi::FontRole::Small);
    Section(_currentBlockTitle);
    _outlineCount = Label(*_sidebar, IDS_OUTLINECOUNT, DxUi::FontRole::Small);
    THROW_IF_FAILED(_outlineCount->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _outlineCount->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _contextDivider         = _sidebar->AddChild<DxUi::Separator>();
    _equationReadOnly       = Label(*_sidebar, IDS_READONLYSHORT, DxUi::FontRole::Small);
    _filesPane              = _root->AddChild<Surface>(Surface::Kind::Sidebar);
    _filesPane->borderRight = true;
    _filesTitle             = Label(*_filesPane, IDS_FILES, DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_filesTitle->SetTypographyOverride(DxUi::LabelTypography{13.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _filesRootIcon = _filesPane->AddChild<DxUi::Label>(L"\xE8B7");
    _filesRootIcon->SetFontRole(DxUi::FontRole::Icon);
    _filesRootIcon->SetAccessibleName(Resource(IDS_FILES));
    _filesSectionTitle = Label(*_filesPane, IDS_FILESSECTION, DxUi::FontRole::Small);
    Section(_filesSectionTitle);
    _closeFiles = Action(*_filesPane, IDS_CLOSEFILES, Command::Files);
    Icon(_closeFiles, L"\xE8BB");
    _closeFiles->SetAppearance(DxUi::ButtonAppearance::Subtle);
    THROW_IF_FAILED(_closeFiles->SetContentFontSizeDip(12.f));
    _filesFilter = _filesPane->AddChild<DxUi::TextField>();
    _filesFilter->SetAccessibleName(Resource(IDS_FILTERFILES));
    _filesFilter->SetPlaceholder(Resource(IDS_FILTERSHORT));
    _filesFilter->SetOnTextChanged([this](std::wstring_view) { RefreshFiles(); });
    _filesTree = _filesPane->AddChild<DxUi::Tree>();
    _filesTree->SetModel(&_filesModel);
    _filesTree->SetDelegate(&_filesModel);
    _filesTree->SetAccessibleName(Resource(IDS_FILES));
    _filesTree->SetRowHeightDip(31);
    _filesTree->SetIndentDip(16);
    _filesTree->SetAppearance({.drawBackground = false, .drawBorder = false, .reserveExpanderSpace = true, .neutralDecorations = true});
    THROW_IF_FAILED(_filesTree->SetTypographyOverride(DxUi::Tree::TypographyOverride{13.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_SEMI_BOLD}));
    _filesTree->SetInvokeOnSingleClick(true);
    _filesTree->SetEmptyStateText(Resource(IDS_FILESUNTITLED));
    _filesCount = Label(*_filesPane, IDS_FILESUNTITLED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_filesCount->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _filesCount->SetMultiline(true);
    _filesFooterDivider = _filesPane->AddChild<DxUi::Separator>();
    _filesFooterDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    _filesModel.invoked = [this](std::uint64_t id)
    {
        if (id > WorkspaceGroupId && id <= WorkspaceGroupId + RecoveryService::MaximumDocuments)
        {
            const auto key = static_cast<unsigned>(id - WorkspaceGroupId - 1);
            if (key == _session.key)
            {
                _workspaceSelectionPreferred = true;
                if (_filesTree->GetSelectedItemId() != id)
                    _filesTree->SetSelectedItemId(id);
                return;
            }
            for (std::size_t index = 0; index < _inactiveDocuments.size(); ++index)
                if (_inactiveDocuments[index].metadata.key == key)
                {
                    const bool previous          = _workspaceSelectionPreferred;
                    _workspaceSelectionPreferred = true;
                    if (! SwitchWorkspaceDocument(index))
                    {
                        _workspaceSelectionPreferred = previous;
                        if (IsUiAttached())
                            RefreshFiles();
                    }
                    break;
                }
            return;
        }
        if (id && id <= _visibleFilePaths.size())
            OpenPath(_visibleFilePaths[static_cast<std::size_t>(id - 1)]);
    };
    _filesModel.expanded = [this](std::uint64_t id, bool expanded)
    {
        if (id == WorkspaceGroupId)
        {
            _workspaceCollapsed = ! expanded;
            if (_workspaceCollapsed)
                _workspaceSelectionPreferred = false;
            RefreshFiles();
            return;
        }
        if (! id || id > _visibleFilePaths.size())
            return;
        const auto path = _visibleFilePaths[static_cast<std::size_t>(id - 1)];
        if (expanded)
            _collapsedFolders.erase(path);
        else
            _collapsedFolders.insert(path);
        RefreshFiles();
    };
    _filesSplitter = _root->AddChild<DxUi::Splitter>();
    _filesSplitter->SetSubtle(true);
    _filesSplitter->SetAccessibleName(Resource(IDS_FILESWIDTH));
    _filesSplitter->SetMinimumFirstPane(200);
    _filesSplitter->SetOnChange([this](DxUi::SplitterChange change)
    {
        _settings.filesWidth = static_cast<unsigned>(std::clamp(change.position, 200.f, 400.f));
        Layout();
        if (change.phase == DxUi::SplitterChangePhase::Commit)
            SavePreferences();
    });

    _drawer = _root->AddChild<Surface>(Surface::Kind::Floating);
    _drawer->SetVisible(false);
    _settingsScroll = _drawer->AddChild<DxUi::ScrollPanel>();
    Label(*_settingsScroll, IDS_SETTINGS, DxUi::FontRole::Subtitle);
    Section(Label(*_settingsScroll, IDS_APPEARANCESECTION, DxUi::FontRole::Small));
    for (unsigned index = 0; index < 4; ++index)
        _themeButtons.push_back(Action(*_settingsScroll, IDS_SYSTEM + index, static_cast<Command>(static_cast<unsigned>(Command::ThemeSystem) + index)));
    _readingWidth = _settingsScroll->AddChild<DxUi::NumericStepper>();
    _readingWidth->SetLabel(Resource(IDS_READINGWIDTH), 140);
    _readingWidth->SetMinimum(480);
    _readingWidth->SetMaximum(1040);
    _readingWidth->SetStep(40);
    _readingWidth->SetLargeStep(80);
    _readingWidth->SetAccessibleName(Resource(IDS_READINGWIDTH));
    _readingWidth->SetStepButtonNames(Resource(IDS_WIDER), Resource(IDS_NARROWER));
    _readingWidth->SetOnChange([this](DxUi::NumericStepperChange c)
    {
        _settings.readingWidth = static_cast<unsigned>(c.value);
        Layout();
        if (c.phase == DxUi::NumericStepperChangePhase::Commit)
            SavePreferences();
    });
    _textScale = _settingsScroll->AddChild<DxUi::NumericStepper>();
    _textScale->SetLabel(Resource(IDS_TEXTSIZE), 140);
    _textScale->SetMinimum(75);
    _textScale->SetMaximum(200);
    _textScale->SetStep(5);
    _textScale->SetAccessibleName(Resource(IDS_TEXTSIZE));
    _textScale->SetStepButtonNames(Resource(IDS_LARGER), Resource(IDS_SMALLER));
    _textScale->SetOnChange([this](DxUi::NumericStepperChange c)
    {
        _settings.textScale = static_cast<unsigned>(c.value);
        Layout();
        if (c.phase == DxUi::NumericStepperChangePhase::Commit)
            SavePreferences();
    });
    _outlineToggle     = Action(*_settingsScroll, IDS_VIEWTOOLS, Command::Outline);
    _focusToggle       = Action(*_settingsScroll, IDS_FOCUS, Command::Focus);
    _dimToggle         = Action(*_settingsScroll, IDS_DIMBLOCKS, Command::DimBlocks);
    _strikeTasksToggle = Action(*_settingsScroll, IDS_STRIKECOMPLETEDTASKS, Command::StrikeCompletedTasks, Resource(IDS_STRIKECOMPLETEDTASKSHELP));
    _strikeTasksToggle->SetAccessibleHelpText(Resource(IDS_STRIKECOMPLETEDTASKSHELP));
    _syntaxTitle = Label(*_settingsScroll, IDS_SYNTAXSECTION, DxUi::FontRole::Small);
    Section(_syntaxTitle);
    _syntaxIntro = Label(*_settingsScroll, IDS_SYNTAXINTRO, DxUi::FontRole::Small);
    _syntaxIntro->SetMultiline(true);
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
    {
        const auto offset = static_cast<unsigned>(index);
        const auto help   = Resource(IDS_SYNTAXHIGHLIGHTHELP + offset);
        _syntaxToggles[index] =
            Action(*_settingsScroll, IDS_SYNTAXHIGHLIGHT + offset, static_cast<Command>(static_cast<unsigned>(Command::SyntaxHighlight) + offset), help);
        _syntaxToggles[index]->SetAccessibleHelpText(help);
        _syntaxHelp[index] = Label(*_settingsScroll, IDS_SYNTAXHIGHLIGHTHELP + offset, DxUi::FontRole::Small);
        _syntaxHelp[index]->SetMultiline(true);
    }
    _privacyTitle = Label(*_settingsScroll, IDS_PRIVACYSECTION, DxUi::FontRole::Small);
    Section(_privacyTitle);
    _drawerHelp = Label(*_settingsScroll, IDS_LOCALIMAGES, DxUi::FontRole::Small);
    _drawerHelp->SetMultiline(true);
    _recoveryAction           = Action(*_settingsScroll, IDS_RECOVERY, Command::Recovery);
    _drawerClose              = Action(*_settingsScroll, IDS_CLOSE, Command::CloseDrawer);
    _drawerImmediate          = Label(*_settingsScroll, IDS_IMMEDIATE, DxUi::FontRole::Small);
    _remoteImagesToggle       = Action(*_settingsScroll, IDS_REMOTEIMAGES, Command::RemoteImages);
    _localNetworkImagesToggle = Action(*_settingsScroll, IDS_LOCALNETWORKIMAGES, Command::LocalNetworkImages, Resource(IDS_LOCALNETWORKIMAGESHELP));
    _localNetworkImagesToggle->SetAccessibleHelpText(Resource(IDS_LOCALNETWORKIMAGESHELP));
    _imageFirstPasteToggle = Action(*_settingsScroll, IDS_IMAGEFIRSTPASTE, Command::ImageFirstPaste, Resource(IDS_IMAGEFIRSTPASTEHELP));
    _imageFirstPasteToggle->SetAccessibleHelpText(Resource(IDS_IMAGEFIRSTPASTEHELP));
    _writingTitle = Label(*_settingsScroll, IDS_WRITINGSECTION, DxUi::FontRole::Small);
    Section(_writingTitle);
    _aboutTitle = Label(*_settingsScroll, IDS_ABOUTSECTION, DxUi::FontRole::Small);
    Section(_aboutTitle);
    _aboutVersion = _settingsScroll->AddChild<DxUi::Label>(FormatStringResource(nullptr, IDS_ABOUTVERSION, REDTILDE_PRODUCT_VERSION_WIDE));
    _aboutVersion->SetFontRole(DxUi::FontRole::BodyStrong);
    THROW_IF_FAILED(_aboutVersion->SetTypographyOverride(DxUi::LabelTypography{14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, 0}));
    _aboutSummary = Label(*_settingsScroll, IDS_ABOUT, DxUi::FontRole::Small);
    _aboutSummary->SetMultiline(true);
    _shortcutsList = Label(*_settingsScroll, IDS_SHORTCUTS, DxUi::FontRole::Small);
    _shortcutsList->SetMultiline(true);
    _findBar = _root->AddChild<Surface>(Surface::Kind::Floating);
    _findBar->SetVisible(false);
    _findText = _findBar->AddChild<DxUi::TextField>();
    _findText->SetAccessibleName(Resource(IDS_FINDHINT));
    _findText->SetPlaceholder(Resource(IDS_FINDHINT));
    _findText->SetOnSubmitted([this] { FindNext(); });
    _findText->SetOnTextChanged([this](std::wstring_view) { RefreshFind(); });
    _findCount = Label(*_findBar, IDS_NOMATCH, DxUi::FontRole::Small);
    Icon(Action(*_findBar, IDS_FINDPREVIOUS, Command::FindPrevious), L"\xE70E");
    Icon(Action(*_findBar, IDS_FINDNEXT, Command::FindNext), L"\xE70D");
    Icon(Action(*_findBar, IDS_CLOSE, Command::CloseDrawer), L"\xE8BB");
    _replaceText = _findBar->AddChild<DxUi::TextField>();
    _replaceText->SetAccessibleName(Resource(IDS_REPLACEHINT));
    _replaceText->SetPlaceholder(Resource(IDS_REPLACEHINT));
    Action(*_findBar, IDS_REPLACE, Command::Replace);
    Action(*_findBar, IDS_REPLACEALL, Command::ReplaceAll);
    _findCaseToggle = Action(*_findBar, IDS_MATCHCASE, Command::FindMatchCase);
    _blockDraft     = _root->AddChild<Surface>(Surface::Kind::Floating);
    _blockDraft->SetVisible(false);
    Label(*_blockDraft, IDS_LOCAL, DxUi::FontRole::BodyStrong);
    Label(*_blockDraft, IDS_BLOCKHELP, DxUi::FontRole::Small);
    _blockDraftText = _blockDraft->AddChild<DxUi::TextField>();
    _blockDraftText->SetFontRole(DxUi::FontRole::Monospace);
    _blockDraftText->SetMultiline(true);
    _blockDraftText->SetAccessibleName(Resource(IDS_LOCAL));
    _blockDraftText->SetOnTextChanged([this](std::wstring_view)
    {
        if (HasBlockDraft())
            CheckpointRecovery();
    });
    _blockDraftText->SetOnPreviewKeyDown([this](DxUi::ControlHost&, UINT key, UINT modifiers)
    {
        if (key == VK_RETURN && (modifiers & MK_CONTROL))
        {
            Execute(Command::ApplyBlock);
            return true;
        }
        return false;
    });
    Action(*_blockDraft, IDS_APPLY, Command::ApplyBlock)->SetPrimary(true);
    Action(*_blockDraft, IDS_CANCEL, Command::CloseDrawer);
    _recoveryPanel = _root->AddChild<Surface>(Surface::Kind::Floating);
    _recoveryPanel->SetVisible(false);
    Label(*_recoveryPanel, IDS_RECOVERY, DxUi::FontRole::Subtitle);
    _recoveryDescription = Label(*_recoveryPanel, IDS_RECOVERYHELP, DxUi::FontRole::Small);
    _recoveryDescription->SetMultiline(true);
    _recoveryPreview = _recoveryPanel->AddChild<DxUi::TextField>();
    _recoveryPreview->SetMultiline(true);
    _recoveryPreview->SetReadOnly(true);
    _recoveryPreview->SetAccessibleName(Resource(IDS_RECOVERY));
    Action(*_recoveryPanel, IDS_RESTORE, Command::RestoreRecovery)->SetPrimary(true);
    Action(*_recoveryPanel, IDS_DISCARD, Command::DiscardRecovery);
    Action(*_recoveryPanel, IDS_LATER, Command::LaterRecovery);
    _restoreAllRecovery = Action(*_recoveryPanel, IDS_RESTOREALL, Command::RestoreAllRecovery);
    _restoreAllRecovery->SetVisible(false);
    _focusPill = _root->AddChild<Surface>(Surface::Kind::Floating);
    _stats     = Label(*_focusPill, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_stats->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _positionStatus = Label(*_root, IDS_SAVED, DxUi::FontRole::Small);
    THROW_IF_FAILED(_positionStatus->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _positionStatus->SetAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _feedback = Label(*_root, IDS_EMPTY, DxUi::FontRole::Small);
    for (auto* label : {_stats, _positionStatus, _feedback})
        THROW_IF_FAILED(label->SetTypographyOverride(DxUi::LabelTypography{12.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _floating = _root->AddChild<Surface>(Surface::Kind::Floating);
    for (const auto& [id, cmd] : std::array<std::pair<unsigned, Command>, 5>{
             {{IDS_HEADING, Command::Heading}, {IDS_BOLD, Command::Bold}, {IDS_ITALIC, Command::Italic}, {IDS_CODE, Command::Code}, {IDS_LINK, Command::Link}}})
        _floatingActions.push_back(Action(*_floating, id, cmd));
    _floatingActions.front()->SetVariant(DxUi::ButtonVariant::DropDown);
    _selectionHintChip = _floating->AddChild<DxUi::CardPanel>();
    _selectionHintChip->SetCornerRadius(4);
    _selectionHint = Label(*_selectionHintChip, IDS_SELECTIONKEY, DxUi::FontRole::Small);
    THROW_IF_FAILED(_selectionHint->SetTypographyOverride(DxUi::LabelTypography{11.f, DWRITE_FONT_WEIGHT_NORMAL, 0}));
    _selectionHint->SetAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _floating->SetVisible(false);
    _host.SetRoot(std::move(tree));
    _host.SetOnEscape([this] { return DismissSurface(); });
    return S_OK;
}

void Application::Layout()
{
    ++_activity.layouts;
    if (! _root)
        return;
    const auto bounds = _host.GetClientBoundsDip();
    const float width = bounds.right, height = bounds.bottom;
    _root->SetBounds(bounds);
    const bool focus = _settings.focusMode;
    const float bar = focus ? 40.f : 56.f, top = focus ? 40.f : 104.f, bottom = std::max(top + 120, height - 28);
    _documentBar->SetBounds(D2D1::RectF(0, 0, width, bar));
    _commandBar->SetBounds(D2D1::RectF(0, 56, width, 104));
    _commandBar->SetVisible(! focus);
    _brandMark->SetVisible(! focus);
    _brandMark->SetBounds(D2D1::RectF(16, 14, 44, 42));
    const bool compactHeader  = width < 1040;
    const float tabsLeft      = compactHeader ? width - 520 : width / 2 - 104;
    const float titleLimit    = std::max(56.f, tabsLeft - 120.f);
    const auto typography     = _host.GetTypographyRevision();
    const float identityWidth = titleLimit - 54.f;
    if (typography && (_identityMeasure.textRevision != _title->GetMeasureRevision() || _identityMeasure.typographyRevision != typography ||
                       _identityMeasure.availableWidth != identityWidth || _identityMeasure.dpi != _host.GetDpi()))
    {
        DxUi::DesiredSize identitySize;
        THROW_IF_FAILED(DxUi::MeasureControl(*_title, {_host}, DxUi::LayoutConstraints::AtMost(identityWidth), identitySize));
        _identityMeasure = {_title->GetMeasureRevision(), _host.GetTypographyRevision(), identityWidth, identitySize.sizeDip.width, _host.GetDpi()};
    }
    const float titleRight = typography ? std::min(titleLimit, 54.f + _identityMeasure.width) : titleLimit;
    _title->SetBounds(focus ? D2D1::RectF(20, 6, width - 320, 34) : D2D1::RectF(54, 16, titleRight, 40));
    _saveStateChip->SetVisible(! focus);
    _saveStateChip->SetBounds(D2D1::RectF(titleRight + 10, 18, titleRight + 62, 38));
    _saveState->SetBounds(D2D1::RectF(titleRight + 14, 18, titleRight + 58, 38));
    constexpr std::array<float, 3> tabOffsets{0.f, 92.f, 170.f}, tabWidths{88.f, 74.f, 54.f};
    for (unsigned index = 0; index < 3; ++index)
    {
        _modeButtons[index]->SetVisible(! focus);
        _modeButtons[index]->SetBounds(D2D1::RectF(tabsLeft + tabOffsets[index], 8, tabsLeft + tabOffsets[index] + tabWidths[index], 46));
        _modeButtons[index]->SetToggleState(static_cast<unsigned>(_settings.view) == index);
    }
    _fileMenu->SetVisible(! focus);
    _fileMenu->SetBounds(D2D1::RectF(width - 294, 8, width - 230, 48));
    _saveButton->SetVisible(! focus);
    _saveButton->SetBounds(D2D1::RectF(width - 217, 8, width - 146, 48));
    _exitFocus->SetVisible(focus);
    _exitFocus->SetBounds(D2D1::RectF(width - 288, 4, width - 144, 36));
    const std::array<DxUi::Button*, 3> captions{_minimizeButton, _maximizeButton, _closeButton};
    for (unsigned i = 0; i < 3; ++i)
        captions[i]->SetBounds(D2D1::RectF(width - 138 + static_cast<float>(i) * 46.f, 0, width - 92 + static_cast<float>(i) * 46.f, std::min(36.f, bar)));
    _maximizeButton->SetText(IsZoomed(_window.get()) ? L"\xE923" : L"\xE922");
    float x    = 12;
    auto place = [&](DxUi::Button* button, float w)
    {
        button->SetBounds(D2D1::RectF(x, 62, x + w, 98));
        x += w + 2;
    };
    place(_filesToggle, 36);
    auto divide = [&](unsigned index)
    {
        _commandDividers[index]->SetVisible(true);
        _commandDividers[index]->SetBounds(D2D1::RectF(x + 8, 70, x + 9, 90));
        x += 19;
    };
    divide(0);
    place(_tools[0], 36);
    place(_tools[1], 36);
    divide(1);
    place(_tools[2], 128);
    divide(2);
    const float commandLimit = width - 248;
    _commandDividers[3]->SetVisible(false);
    _commandDividers[4]->SetVisible(false);
    for (const unsigned i : {3u, 4u, 5u, 13u, 6u, 7u, 8u})
    {
        const float w   = i == 8 ? 100.f : 36.f;
        const bool fits = x + w + 44 <= commandLimit;
        _tools[i]->SetVisible(fits);
        if (fits)
            place(_tools[i], w);
        if ((i == 13 || i == 7) && fits)
            divide(i == 13 ? 3 : 4);
    }
    _tools[10]->SetVisible(false);
    const bool overflow = std::ranges::any_of(std::array<unsigned, 7>{3, 4, 5, 13, 6, 7, 8}, [this](unsigned i) { return ! _tools[i]->IsVisible(); });
    _overflowButton->SetVisible(overflow);
    if (overflow)
        place(_overflowButton, 36);
    _commandDividers[5]->SetVisible(false);
    x = width - 248;
    place(_tools[9], 120);
    place(_tools[11], 36);
    place(_panelToggle, 36);
    place(_settingsToggle, 36);
    _filesToggle->SetToggleState(_settings.showFiles);
    _panelToggle->SetToggleState(_settings.showOutline);
    _settingsToggle->SetToggleState(_drawer->IsVisible());
    _tools[11]->SetToggleState(focus);
    const bool fileOverlay       = _settings.showFiles && width < static_cast<float>(_settings.readingWidth + _settings.filesWidth + 96 + 6);
    const float filesWidth       = static_cast<float>(_settings.filesWidth);
    const float left             = _settings.showFiles && ! fileOverlay && ! focus ? filesWidth : 0;
    const bool showSidebar       = _settings.showOutline && ! focus && ! fileOverlay && width - left >= static_cast<float>(_settings.readingWidth + 336) &&
                                   _settings.view != EditorView::Split;
    const float right            = showSidebar ? width - 240 : width;
    const auto layoutRequest     = _activity.layouts;
    const bool sidebarWasVisible = _sidebar->IsVisible();
    if (showSidebar && ! sidebarWasVisible && ! PublishCurrentBlockLabelIfVisible(true))
        return;
    if (_activity.layouts != layoutRequest)
        return;
    _sidebar->SetVisible(showSidebar);
    if (_activity.layouts != layoutRequest)
        return;
    _sidebar->SetBounds(D2D1::RectF(width - 240, top, width, bottom));
    const float side = width - 224;
    _outlineTitle->SetBounds(D2D1::RectF(side + 4, top + 16, width - 100, top + 40));
    _outlineCount->SetBounds(D2D1::RectF(width - 104, top + 16, width - 16, top + 40));
    const float contextHeight = _equationContext ? 260.f : 128.f;
    const float outlineHeight = std::max(40.f, static_cast<float>(_outlineModel.rows.size()) * 42.f + 4.f);
    const float outlineBottom = std::min(top + 44 + outlineHeight, std::max(top + 92, bottom - contextHeight));
    _outlineTree->SetBounds(D2D1::RectF(side - 4, top + 44, width - 8, outlineBottom));
    _contextDivider->SetBounds(D2D1::RectF(side + 4, outlineBottom + 12, width - 12, outlineBottom + 13));
    _contextDivider->SetOrientation(DxUi::SeparatorOrientation::Horizontal);
    _currentBlockTitle->SetBounds(D2D1::RectF(side + 4, outlineBottom + 28, width - 16, outlineBottom + 48));
    _currentBlock->SetBounds(D2D1::RectF(side + 4, outlineBottom + 52, width - 16, outlineBottom + 76));
    if (showSidebar && ! PublishCurrentBlockLabelIfVisible())
        return;
    if (_activity.layouts != layoutRequest)
        return;
    _listSummaryIcon->SetBounds(D2D1::RectF(side + 4, outlineBottom + 52, side + 20, outlineBottom + 76));
    _listSummary->SetBounds(D2D1::RectF(side + 28, outlineBottom + 52, width - 16, outlineBottom + 76));
    _blockSourceAction->SetBounds(D2D1::RectF(side + 4, outlineBottom + 84, width - 20, outlineBottom + 116));
    _equationHelp->SetVisible(_equationContext);
    _equation->SetVisible(_equationContext);
    _equationReadOnly->SetVisible(_equationContext);
    _equationHelp->SetBounds(D2D1::RectF(side, outlineBottom + 132, width - 100, outlineBottom + 156));
    _equationReadOnly->SetBounds(D2D1::RectF(width - 96, outlineBottom + 132, width - 16, outlineBottom + 156));
    _equation->SetBounds(D2D1::RectF(side, outlineBottom + 164, width - 16, bottom - 16));
    _page->SetBounds(D2D1::RectF(left, top, right, bottom));
    const bool split          = _settings.view == EditorView::Split;
    const float mid           = (left + right) / 2;
    const float gutter        = focus ? 32.f : split ? 24.f : 48.f;
    const float consentHeight = right - left < 600.f ? 104.f : 64.f;
    const float consentTop    = top + 12.f;
    const float contentTop    = std::max(top + (focus ? 36.f : 60.f), _imageConsent->IsVisible() ? consentTop + consentHeight + 12.f : top);
    _imageConsent->SetBounds(D2D1::RectF(left + 12.f, consentTop, right - 12.f, consentTop + consentHeight));
    const bool narrowConsent = right - left < 600.f;
    _imageConsentText->SetBounds(D2D1::RectF(left + 24.f, consentTop + 8.f, narrowConsent ? right - 24.f : right - 256.f, consentTop + 48.f));
    const float consentButtonsTop = consentTop + (narrowConsent ? 60.f : 14.f);
    _imageConsentAction->SetBounds(
        D2D1::RectF(narrowConsent ? left + 24.f : right - 244.f, consentButtonsTop, narrowConsent ? left + 128.f : right - 140.f, consentButtonsTop + 36.f));
    _imageReload->SetBounds(D2D1::RectF(right - 132.f, consentButtonsTop, right - 24.f, consentButtonsTop + 36.f));
    _editor->SetBounds(D2D1::RectF(left + gutter, contentTop, (split ? mid : right) - gutter, bottom));
    _mirror->SetVisible(split);
    _mirror->SetBounds(D2D1::RectF(mid + 24, _imageConsent->IsVisible() ? contentTop : top + 56, right - 24, bottom - 24));
    DxUi::RichTextEditorAppearance appearance;
    appearance.markdownPresentation        = DxUi::RichText::MarkdownPresentationStyle::Subtle;
    appearance.strikeCompletedTasks        = _settings.strikeCompletedTasks;
    appearance.drawBorder                  = false;
    appearance.drawFocusIndicator          = false;
    appearance.useAccentLinkColor          = true;
    appearance.showVerticalScrollbar       = true;
    appearance.paddingDip                  = 0;
    appearance.maximumTextWidthDip         = focus ? 680.f : static_cast<float>(_settings.readingWidth);
    appearance.bodyFontSizeDip             = (focus ? 18.f : split ? 15.f : 16.f) * static_cast<float>(_settings.textScale) / 100.f;
    appearance.codeFontSizeDip             = 13.5f * static_cast<float>(_settings.textScale) / 100.f;
    appearance.lineHeightMultiplier        = _settings.view == EditorView::Source ? 1.75f : 1.65f;
    appearance.headingFontSizeDip          = {34.f * static_cast<float>(_settings.textScale) / 100.f,
                                              23.f * static_cast<float>(_settings.textScale) / 100.f,
                                              20.f * static_cast<float>(_settings.textScale) / 100.f,
                                              18.f * static_cast<float>(_settings.textScale) / 100.f,
                                              16.f * static_cast<float>(_settings.textScale) / 100.f,
                                              16.f * static_cast<float>(_settings.textScale) / 100.f};
    appearance.headingLineHeightMultiplier = {1.2f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f};
    appearance.paragraphSpacingAfterLines  = 18.f / 26.4f;
    appearance.listItemSpacingDip          = 10.f;
    appearance.semanticSpacing             = DxUi::RichText::SemanticSpacing{
        .headingBeforeDip = {0.f, 40.f, 32.f, 24.f, 20.f, 20.f}, .headingAfterDip = {0.f, 14.f, 14.f, 14.f, 14.f, 14.f}, .containerIndentDip = 10.f};
    auto sourceAppearance                   = appearance;
    sourceAppearance.bodyFontSizeDip        = appearance.codeFontSizeDip;
    sourceAppearance.colorizeMarkdownSource = true;
    sourceAppearance.lineHeightMultiplier   = 1.75f;
    // The canonical monospace role and its fallback use the same family.
    // Reading that policy needs no host text format or graphics initialization.
    sourceAppearance.bodyFontFamily = DxUi::Typography::GetSpec(DxUi::FontRole::Monospace).familyName;
    if (! AcceptSourceResult(_editor->TrySetAppearance(_settings.view == EditorView::Source ? sourceAppearance : appearance)))
        return;
    _editor->SetFocusModeEnabled(focus && _settings.dimInactiveBlocks);
    if (! AcceptSourceResult(_mirror->TrySetAppearance(sourceAppearance)))
        return;
    if (! AcceptSourceResult(_equation->TrySetAppearance(appearance)))
        return;
    _filesPane->SetVisible(_settings.showFiles && ! focus);
    _filesPane->borderRight = fileOverlay;
    _filesPane->SetBounds(D2D1::RectF(0, top, filesWidth, bottom));
    _filesSectionTitle->SetBounds(D2D1::RectF(20, top + 16, filesWidth - 52, top + 40));
    _closeFiles->SetBounds(D2D1::RectF(filesWidth - 44, top + 14, filesWidth - 12, top + 46));
    _filesRootIcon->SetBounds(D2D1::RectF(20, top + 52, 36, top + 78));
    _filesTitle->SetBounds(D2D1::RectF(44, top + 52, filesWidth - 16, top + 78));
    _filesFilter->SetBounds(D2D1::RectF(20, top + 84, filesWidth - 12, top + 114));
    const float footerWidth   = filesWidth - 24;
    const auto footerRevision = _filesCount->GetMeasureRevision(), typographyRevision = _host.GetTypographyRevision();
    const auto footerDpi = _host.GetDpi();
    // Keep hidden measurement keys stale; the first visible layout measures
    // current text/width/typography/DPI before reserving the footer's bounds.
    if (_filesPane->IsVisible() && (_filesFooterMeasure.widthDip != footerWidth || _filesFooterMeasure.textRevision != footerRevision ||
                                    _filesFooterMeasure.typographyRevision != typographyRevision || _filesFooterMeasure.dpi != footerDpi))
    {
        DxUi::DesiredSize desired;
        const HRESULT measured = DxUi::MeasureControl(*_filesCount, {_host}, DxUi::LayoutConstraints::AtMost(footerWidth), desired);
        if (measured == E_OUTOFMEMORY)
            std::terminate();
        THROW_IF_FAILED(measured);
        _filesFooterMeasure.widthDip     = footerWidth;
        _filesFooterMeasure.heightDip    = std::max(24.f, std::ceil(desired.sizeDip.height));
        _filesFooterMeasure.textRevision = footerRevision;
        // A cold MeasureControl may initialize the host's typography itself.
        _filesFooterMeasure.typographyRevision = _host.GetTypographyRevision();
        _filesFooterMeasure.dpi                = footerDpi;
        ++_filesFooterMeasure.measurements;
    }
    const float footerTop = bottom - 8 - _filesFooterMeasure.heightDip;
    _filesFooterDivider->SetBounds(D2D1::RectF(0, footerTop - 8, filesWidth, footerTop - 7));
    _filesTree->SetBounds(D2D1::RectF(4, top + 122, filesWidth - 4, footerTop - 4));
    _filesCount->SetBounds(D2D1::RectF(12, footerTop, filesWidth - 12, bottom - 8));
    _filesSplitter->SetVisible(_settings.showFiles && ! focus && ! fileOverlay);
    _filesSplitter->SetBounds(D2D1::RectF(0, top, width, bottom));
    _filesSplitter->SetMinimumSecondPane(std::max(48.f, width - 406));
    _filesSplitter->SetPosition(filesWidth);
    const float drawerLeft = width - 380;
    _drawer->SetBounds(D2D1::RectF(drawerLeft, top, width, bottom));
    _settingsScroll->SetBounds(D2D1::RectF(drawerLeft + 16, top + 12, width - 8, bottom - 12));
    _settingsScroll->SetContentHeight(1934);
    const float dx = drawerLeft + 24;
    _settingsScroll->GetLogicalChild(0)->SetBounds(D2D1::RectF(dx, top + 20, width - 24, top + 52));
    _settingsScroll->GetLogicalChild(1)->SetBounds(D2D1::RectF(dx, top + 68, width - 24, top + 92));
    std::array<DxUi::ButtonThemePreview, 4> previews{{{},
                                                      {{D2D1::ColorF(0xF0F1F4), D2D1::ColorF(0xF8F8FA), D2D1::ColorF(0xFFFFFF)}},
                                                      {{D2D1::ColorF(0x17191E), D2D1::ColorF(0x1C1F25), D2D1::ColorF(0x22252B)}},
                                                      {{D2D1::ColorF(0xEDE6D8), D2D1::ColorF(0xF3EDDF), D2D1::ColorF(0xFFFAEF)}}}};
    previews[0] = previews[_systemDark ? 2 : 1];
    for (unsigned i = 0; i < 4; ++i)
    {
        _themeButtons[i]->SetBounds(D2D1::RectF(dx + static_cast<float>(i % 2) * 168.f,
                                                top + 104 + static_cast<float>(i / 2) * 80.f,
                                                dx + 156 + static_cast<float>(i % 2) * 168.f,
                                                top + 168 + static_cast<float>(i / 2) * 80.f));
        _themeButtons[i]->SetThemePreview(previews[i]);
        _themeButtons[i]->SetToggleState(static_cast<unsigned>(_settings.appearance) == i);
    }
    _readingWidth->SetBounds(D2D1::RectF(dx, top + 272, width - 24, top + 308));
    if (! _readingWidth->IsEditing())
        _readingWidth->SetValue(_settings.readingWidth);
    _textScale->SetBounds(D2D1::RectF(dx, top + 320, width - 24, top + 356));
    if (! _textScale->IsEditing())
        _textScale->SetValue(_settings.textScale);
    _writingTitle->SetBounds(D2D1::RectF(dx, top + 372, width - 24, top + 396));
    _outlineToggle->SetBounds(D2D1::RectF(dx, top + 408, width - 24, top + 444));
    _outlineToggle->SetToggleState(_settings.showOutline);
    _focusToggle->SetBounds(D2D1::RectF(dx, top + 456, width - 24, top + 492));
    _focusToggle->SetToggleState(focus);
    _dimToggle->SetBounds(D2D1::RectF(dx, top + 504, width - 24, top + 540));
    _dimToggle->SetToggleState(_settings.dimInactiveBlocks);
    _strikeTasksToggle->SetBounds(D2D1::RectF(dx, top + 556, width - 24, top + 592));
    _strikeTasksToggle->SetToggleState(_settings.strikeCompletedTasks);
    _syntaxTitle->SetBounds(D2D1::RectF(dx, top + 608, width - 24, top + 632));
    _syntaxIntro->SetBounds(D2D1::RectF(dx, top + 640, width - 24, top + 680));
    // Each option reserves two lines of explanation beneath its toggle.
    for (std::size_t index = 0; index < MarkdownSyntaxCount; ++index)
    {
        const float row = top + 692 + static_cast<float>(index) * 92.f;
        _syntaxToggles[index]->SetBounds(D2D1::RectF(dx, row, width - 24, row + 36));
        _syntaxToggles[index]->SetToggleState(_settings.markdownSyntax[index]);
        _syntaxHelp[index]->SetBounds(D2D1::RectF(dx, row + 40, width - 24, row + 80));
    }
    const float privacy = top + 1340;
    _privacyTitle->SetBounds(D2D1::RectF(dx, privacy, width - 24, privacy + 24));
    _remoteImagesToggle->SetBounds(D2D1::RectF(dx, privacy + 36, width - 24, privacy + 72));
    _remoteImagesToggle->SetToggleState(_settings.allowRemoteImages);
    _localNetworkImagesToggle->SetBounds(D2D1::RectF(dx, privacy + 80, width - 24, privacy + 116));
    _localNetworkImagesToggle->SetToggleState(_settings.allowLocalNetworkImages);
    _drawerHelp->SetBounds(D2D1::RectF(dx, privacy + 124, width - 24, privacy + 164));
    _imageFirstPasteToggle->SetBounds(D2D1::RectF(dx, privacy + 176, width - 24, privacy + 212));
    _imageFirstPasteToggle->SetToggleState(_settings.imageFirstPaste);
    _recoveryAction->SetBounds(D2D1::RectF(dx, privacy + 228, width - 24, privacy + 264));
    _drawerClose->SetBounds(D2D1::RectF(dx + 224, privacy + 276, width - 24, privacy + 312));
    _drawerImmediate->SetBounds(D2D1::RectF(dx, privacy + 276, width - 160, privacy + 312));
    const float about = privacy + 336;
    _aboutTitle->SetBounds(D2D1::RectF(dx, about, width - 24, about + 24));
    _aboutVersion->SetBounds(D2D1::RectF(dx, about + 30, width - 24, about + 54));
    _aboutSummary->SetBounds(D2D1::RectF(dx, about + 58, width - 24, about + 98));
    _shortcutsList->SetBounds(D2D1::RectF(dx, about + 106, width - 24, about + 250));
    const bool wideFind = right - left >= 660.f;
    const float fx = std::max(16.f, right - (wideFind ? 644.f : 476.f)), fy = top + 12;
    _findBar->SetBounds(D2D1::RectF(fx, fy, right - 16, fy + (wideFind ? 104.f : 148.f)));
    _findText->SetBounds(D2D1::RectF(fx + 12, fy + 10, fx + 204, fy + 46));
    _findCaseToggle->SetToggleState(_find.matchCase);
    _findCaseToggle->SetBounds(wideFind ? D2D1::RectF(fx + 212.f, fy + 10.f, fx + 320.f, fy + 46.f)
                                        : D2D1::RectF(fx + 12.f, fy + 104.f, fx + 146.f, fy + 140.f));
    const float findStatus  = wideFind ? 328.f : 212.f;
    const float findButtons = wideFind ? 478.f : 314.f;
    _findCount->SetBounds(D2D1::RectF(fx + findStatus, fy + 14, fx + findStatus + 98.f, fy + 42));
    for (unsigned i = 0; i < 3; ++i)
        _findBar->GetLogicalChild(i + 2)->SetBounds(
            D2D1::RectF(fx + findButtons + static_cast<float>(i) * 44.f, fy + 10, fx + findButtons + 40.f + static_cast<float>(i) * 44.f, fy + 46));
    _replaceText->SetBounds(D2D1::RectF(fx + 12, fy + 58, fx + 236, fy + 94));
    _findBar->GetLogicalChild(6)->SetBounds(D2D1::RectF(fx + 244, fy + 58, fx + 326, fy + 94));
    _findBar->GetLogicalChild(7)->SetBounds(D2D1::RectF(fx + 334, fy + 58, fx + 448, fy + 94));
    const float draftLeft = std::max(left + 16, (left + right - 680) / 2), draftRight = std::min(right - 16, draftLeft + 680);
    float draftTop = top + 72;
    if (_blockDraftSession)
        _blockDraftSession->pendingPlacement = false;
    if (_activeOverlay == OverlayKind::BlockDraftModal && _blockDraftSession)
    {
        const auto& range = _blockDraftSession->range;
        if (const auto rects = _editor->TryGetTextInputRangeRects(_host, range.first, range.last); rects && ! rects->empty())
            draftTop = rects->back().bottom + 8;
        else if (const auto caret = _editor->TryGetTextInputCaretRect(_host, range.first))
            draftTop = caret->bottom + 8; // Empty/hidden-only source still has a prepared insertion site.
        else
            _blockDraftSession->pendingPlacement = true;
    }
    draftTop = std::clamp(draftTop, top + 8, std::max(top + 8, bottom - 244));
    _blockDraft->SetBounds(D2D1::RectF(draftLeft, draftTop, draftRight, draftTop + 232));
    _blockDraft->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 8, draftRight - 16, draftTop + 36));
    _blockDraft->GetLogicalChild(1)->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 40, draftRight - 16, draftTop + 64));
    _blockDraftText->SetBounds(D2D1::RectF(draftLeft + 16, draftTop + 72, draftRight - 16, draftTop + 172));
    _blockDraft->GetLogicalChild(3)->SetBounds(D2D1::RectF(draftRight - 244, draftTop + 184, draftRight - 112, draftTop + 220));
    _blockDraft->GetLogicalChild(4)->SetBounds(D2D1::RectF(draftRight - 100, draftTop + 184, draftRight - 16, draftTop + 220));
    _recoveryPanel->SetBounds(D2D1::RectF(draftLeft, top + 8, draftRight, bottom - 8));
    _recoveryPanel->GetLogicalChild(0)->SetBounds(D2D1::RectF(draftLeft + 20, top + 24, draftRight - 20, top + 60));
    _recoveryDescription->SetBounds(D2D1::RectF(draftLeft + 20, top + 68, draftRight - 20, top + 128));
    const bool multipleRecovery = _restoreAllRecovery->IsVisible();
    _recoveryPreview->SetBounds(D2D1::RectF(draftLeft + 20, top + 136, draftRight - 20, bottom - (multipleRecovery ? 124.f : 80.f)));
    if (multipleRecovery)
    {
        const float buttonWidth = (draftRight - draftLeft - 48.f) / 2.f;
        for (unsigned i = 0; i < 4; ++i)
        {
            const float buttonLeft = draftLeft + 20 + static_cast<float>(i % 2) * (buttonWidth + 8.f);
            const float buttonTop  = bottom - 106 + static_cast<float>(i / 2) * 44.f;
            _recoveryPanel->GetLogicalChild(i + 3)->SetBounds(D2D1::RectF(buttonLeft, buttonTop, buttonLeft + buttonWidth, buttonTop + 34));
        }
    }
    else
        for (unsigned i = 0; i < 3; ++i)
            _recoveryPanel->GetLogicalChild(i + 3)->SetBounds(
                D2D1::RectF(draftLeft + 20 + static_cast<float>(i) * 208.f, bottom - 62, draftLeft + 220 + static_cast<float>(i) * 208.f, bottom - 28));
    _stats->SetBounds(D2D1::RectF(16, height - 26, width / 2, height - 2));
    _positionStatus->SetBounds(D2D1::RectF(width / 2, height - 26, width - 16, height - 2));
    _focusPill->SetBounds(focus ? D2D1::RectF(width / 2 - 224, height - 50, width / 2 + 224, height - 10) : D2D1::RectF(0, height - 28, width / 2, height));
    _focusPill->kind = focus ? Surface::Kind::Floating : Surface::Kind::Chrome;
    if (focus)
        _stats->SetBounds(D2D1::RectF(width / 2 - 208, height - 44, width / 2 + 208, height - 16));
    UpdateCounts();
    _title->SetTextColor(focus ? std::optional(_host.GetTheme().subduedText) : std::nullopt);
    _stats->SetVisible(true);
    _positionStatus->SetVisible(! focus);
    const auto name = _session.path.empty() ? Resource(IDS_UNTITLED) : _session.path.filename().native();
    _title->SetText(focus && Dirty() ? name + L" · " + Resource(IDS_CHANGED) : name);
    _feedback->SetBounds(D2D1::RectF(left + 16, bottom - 22, right - 16, bottom));
    // Reposition only an already visible toolbar: resize must not reopen
    // tools that Escape or another contextual surface deliberately dismissed.
    if (_activeOverlay == OverlayKind::FloatingSelectionTools)
        SelectionChanged();
    _host.Invalidate();
}
} // namespace RedTilde
