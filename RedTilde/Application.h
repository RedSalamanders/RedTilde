#pragma once
#include "AsyncFiles.h"
#include "Common/DocumentInfo.h"
#include "Common/Files.h"
#include "Common/Recovery.h"
#include "Common/Settings.h"
#include "EditDiagnostics.h"
#include "ImageLoader.h"
#include "NavigationModel.h"
#include "UiControls.h"
#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <utility>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
enum class Command
{
    New,
    Open,
    Save,
    SaveAs,
    Compose,
    Source,
    Split,
    Undo,
    Redo,
    Heading,
    Bold,
    Italic,
    Code,
    Strike,
    Bullets,
    Numbered,
    Task,
    Insert,
    Find,
    Settings,
    LocalSource,
    Focus,
    Paragraph,
    H1,
    H2,
    H3,
    Link,
    Image,
    Table,
    Mermaid,
    Math,
    Quote,
    Fence,
    RowAdd,
    RowRemove,
    ColumnAdd,
    ColumnRemove,
    ThemeSystem,
    ThemeLight,
    ThemeDark,
    ThemePaper,
    Outline,
    CloseDrawer,
    FindNext,
    ApplyBlock,
    Recovery,
    RestoreRecovery,
    DiscardRecovery,
    LaterRecovery,
    FileMenu,
    Files,
    Overflow,
    FindPrevious,
    Replace,
    ReplaceAll,
    Minimize,
    Maximize,
    Close,
    DimBlocks,
    RemoteImages,
    // Ordered as MarkdownSyntax; each toggles one optional RichMarkdown syntax.
    SyntaxHighlight,
    SyntaxSuperscript,
    SyntaxSubscript,
    SyntaxInserted,
    SyntaxWikiLinks,
    SyntaxDefinitionLists,
    SyntaxAbbreviations,
    OpenFolder,
    StrikeCompletedTasks
};
struct LaunchOptions
{
    bool headless{}, selfTest{}, crashProbe{}, preferencesProbe{}, recoveryProbe{}, recoveryRestartProbe{};
    bool resourceProbe{}, forceWarp{}, controlDiagnostics{};
    unsigned performanceCycles = 12;
    unsigned captureDpi{}, viewportWidthPx{}, viewportHeightPx{};
    std::wstring scenario = L"compose-light";
    std::filesystem::path document, capture, dataDirectory, benchmark, performance, designInteractive;
};
struct ApplicationActivity
{
    std::uint64_t changes{}, documentAnalysisUpdates{}, layouts{}, mirrorResets{}, recoverySubmissions{}, captionRequests{}, captionCalls{};
    std::uint64_t findMatchRebuilds{}, findHighlightUpdates{}, findSectionCountRebuilds{};
    std::uint64_t selectionPresentationUpdates{}, syntaxPresentationUpdates{}, mirrorSelectionUpdates{}, outlineSelectionUpdates{};
    std::uint64_t currentBlockLabelConstructions{};
    std::uint64_t selectionCaptionConstructions{};
    std::uint64_t filesModelRebuilds{};
    mutable std::uint64_t dirtyTextComparisons{};
    std::uint64_t recoveryTextCopies{};
};
class PerformanceProbe;
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626)
class Application
{
  public:
    explicit Application(LaunchOptions options);
    ~Application();
    [[nodiscard]] HRESULT Initialize();
    int Run();
    void Execute(Command command);
    void SetView(EditorView view);
    void SetDocument(std::wstring text, std::filesystem::path path = {}, bool bom = false, FileStamp stamp = {});
    // Interaction probes settle content first, then capture without pumping
    // ambient pointer-leave messages for their deliberately hidden HWND.
    bool Capture(const std::filesystem::path &path, bool prepareContent = true);
    void Prepare(unsigned turns = 128);
    [[nodiscard]] bool Dirty() const;
    [[nodiscard]] Editor &Document() const
    {
        return *editor;
    }
    [[nodiscard]] DxUi::ControlHost &Host()
    {
        return host;
    }
    [[nodiscard]] const Settings &Preferences() const
    {
        return settings;
    }
    [[nodiscard]] HWND Window() const
    {
        return window.get();
    }
    [[nodiscard]] DxUi::TextField *BlockDraft() const
    {
        return blockDraftText;
    }
    [[nodiscard]] bool HasBlockDraft() const
    {
        return blockDraft && blockDraft->IsVisible();
    }
    [[nodiscard]] std::size_t RecoveryCount() const
    {
        return recoveryCandidates.size();
    }
    [[nodiscard]] bool RecoveryReady() const
    {
        return recovery.Ready();
    }
    [[nodiscard]] bool RecoverySettled() const
    {
        return recovery.Settled();
    }
    [[nodiscard]] HRESULT RecoveryStatus() const
    {
        return recovery.Status();
    }
    [[nodiscard]] std::wstring EquationPreviewText() const
    {
        return equation->GetText();
    }
    [[nodiscard]] bool RequiresSaveAs() const
    {
        return recovered;
    }
    void CheckpointRecovery(bool immediate = false);
    [[nodiscard]] ApplicationActivity Activity() const noexcept
    {
        return activity;
    }
    [[nodiscard]] const DxUi::RichTextEditor &SourceMirror() const noexcept
    {
        return *mirror;
    }

  private:
    friend class PerformanceProbe;
    friend class DesignRefreshProbe;
    friend class DesignInteractiveProbe;
    EditDiagnostics *editDiagnostics{}; // Borrowed only for a synchronous harness transaction.
    ApplicationActivity activity;
    std::optional<std::wstring> appliedCaption;
    LaunchOptions options;
    Settings settings;
    SettingsWriter preferenceWriter;
    RecoveryService recovery;
    std::vector<RecoveryCandidate> recoveryCandidates;
    bool recoveryOffered{}, recovered{};
    std::uint64_t recoveryGeneration{};
    wil::unique_hwnd window;
    NavigationModel outlineModel, filesModel;
    DxUi::ControlHost host;
    FilesService folderFiles;
    std::optional<FilesSnapshot> folderSnapshot;
    std::set<std::filesystem::path> collapsedFolders;
    DxUi::Control *brandMark{};
    DxUi::Tree *outlineTree{}, *filesTree{};
    Surface *filesPane{}, *focusPill{};
    DxUi::Splitter *filesSplitter{};
    DxUi::Label *filesTitle{}, *filesCount{}, *currentBlock{}, *positionStatus{}, *findCount{}, *drawerHelp{}, *currentBlockTitle{},
        *equationReadOnly{}, *writingTitle{}, *syntaxTitle{}, *syntaxIntro{}, *privacyTitle{}, *drawerImmediate{};
    std::array<DxUi::Button *, MarkdownSyntaxCount> syntaxToggles{};
    std::array<DxUi::Label *, MarkdownSyntaxCount> syntaxHelp{};
    std::array<DxUi::Separator *, 6> commandDividers{};
    DxUi::CardPanel *saveStateChip{};
    DxUi::Label *filesSectionTitle{}, *outlineCount{}, *filesRootIcon{}, *listSummaryIcon{};
    DxUi::Button *closeFiles{}, *blockSourceAction{};
    DxUi::Separator *contextDivider{}, *filesFooterDivider{};
    DxUi::Label *listSummary{};
    std::optional<std::pair<unsigned, std::size_t>> listSummaryFacts;
    std::uint64_t listSummaryPublication{};
    // Header measurement waits for the host's first typography preparation; cold layout reserves explicit bounds.
    struct IdentityMeasure
    {
        std::uint64_t textRevision{}, typographyRevision{};
        float availableWidth = -1.f, width{}, dpi{};
    } identityMeasure;
    DxUi::TextField *filesFilter{}, *replaceText{};
    DxUi::Button *filesToggle{}, *panelToggle{}, *settingsToggle{}, *overflowButton{}, *exitFocus{}, *fileMenu{}, *saveButton{}, *minimizeButton{},
        *maximizeButton{}, *closeButton{}, *dimToggle{}, *strikeTasksToggle{}, *remoteImagesToggle{}, *recoveryAction{}, *drawerClose{};
    DxUi::NumericStepper *readingWidth{}, *textScale{};
    DxUi::ScrollPanel *settingsScroll{};
    DxUi::CardPanel *selectionHintChip{};
    DxUi::Label *selectionHint{};
    std::vector<DxUi::RichText::MarkdownRange> findMatches;
    std::optional<DxUi::RichText::SourceSnapshot> findSource;
    std::wstring findQuery;
    std::vector<std::size_t> findSectionCounts;
    std::uint64_t outlineRowsRevision{}, findBadgeRowsRevision{}, findSectionOutlineRevision{}, findSectionAnalysis{};
    std::optional<std::size_t> findHighlightedCurrent, findDisplayedCurrent, findDisplayedTotal;
    std::wstring findDisplayedQuery;
    bool findPresentationCleared = true, findSectionCountsValid{}, findHighlightsNeedRefresh = true, findHasHighlights{};
    bool findBadgesNeedRefresh = true;
    std::vector<std::filesystem::path> visibleFilePaths;
    std::optional<std::size_t> currentHeading;
    struct CurrentBlockLabelFacts
    {
        unsigned resourceId{};
        std::size_t lineCharacterCount{};
        bool operator==(const CurrentBlockLabelFacts &) const noexcept = default;
    };
    std::optional<CurrentBlockLabelFacts> currentBlockPublishedFacts;
    std::optional<CurrentBlockLabelFacts> currentBlockLatestFacts;
    std::optional<bool> selectionCaptionPublishedEquationContext;
    bool updatingFind{}, equationContext{}, pendingSelectionTools{}, pendingBlockPlacement{}, systemDark{};
    AsyncFiles files;
    RichImageLoader images;
    std::vector<DxUi::RichText::RichImageRequest> imageRequests;
    Editor *editor{};
    DxUi::RichTextEditor *mirror{};
    Surface *root{}, *documentBar{}, *commandBar{}, *page{}, *sidebar{}, *floating{}, *drawer{}, *findBar{}, *blockDraft{};
    DxUi::Label *title{}, *saveState{}, *stats{}, *feedback{}, *outlineTitle{}, *equationHelp{}, *modeHint{};
    DxUi::RichTextEditor *equation{};
    DxUi::TextField *findText{};
    DxUi::TextField *blockDraftText{};
    Surface *recoveryPanel{};
    DxUi::TextField *recoveryPreview{};
    DxUi::Label *recoveryDescription{};
    DxUi::RichText::SourceSnapshot blockDraftSnapshot;
    DxUi::RichText::MarkdownRange blockDraftRange;
    std::uint64_t blockDraftRevision{};
    std::vector<DxUi::Button *> tools, floatingActions, themeButtons;
    std::array<DxUi::Button *, 3> modeButtons{};
    DxUi::Button *outlineToggle{}, *focusToggle{};
    std::filesystem::path documentPath, folderRoot;
    bool lastFilesDirty{}, folderFilesActive{};
    HRESULT folderAdmissionStatus = S_OK;
    struct FilesFooterMeasureCache
    {
        float widthDip = -1, heightDip = 52, dpi{};
        std::uint64_t textRevision{}, typographyRevision{}, measurements{};
    } filesFooterMeasure;
    std::set<std::uint64_t> collapsedHeadings;
    std::wstring savedText;
    struct DirtyFact
    {
        DxUi::RichText::SourceSnapshot source;
        bool recovered{}, dirty{};
    };
    // Saved-baseline replacement invalidates this fact without copying savedText.
    mutable std::optional<DirtyFact> dirtyFact;
    mutable std::optional<DxUi::RichText::SourceSnapshot> cachedSource;
    mutable std::wstring cachedText;
    std::optional<DxUi::RichText::SourceSnapshot> mirroredSource, selectionFactsSource;
    std::optional<DxUi::RichText::SourceSnapshot> documentAnalysisSource, committedAnalysisSource;
    std::optional<std::set<std::uint64_t>> previewCollapsedHeadings;
    std::optional<std::vector<std::pair<unsigned, std::wstring>>> previewCommittedOutline;
    std::shared_ptr<const DxUi::RichText::MarkdownSyntax> selectionFactsSyntax;
    DxUi::RichText::Selection selectionFactsSelection;
    EditorView selectionFactsView = EditorView::Compose;
    std::uint64_t selectionFactsAnalysis{};
    struct SelectionToolsFocusRequest
    {
        DxUi::RichText::SourceSnapshot source;
        DxUi::RichText::Selection selection;
        EditorView view;
    };
    std::optional<SelectionToolsFocusRequest> selectionToolsFocus;
    bool bom{}, closing{}, initialized{}, initializing{};
    bool waitingForPreferences{};
    FileStamp stamp;
    DocumentInfo documentInfo;
    DocumentLineIndexStorage documentLineIndexStorage;
    std::function<void()> afterSave;
    std::optional<DxUi::RichText::SourceSnapshot> openingSnapshot;
    static constexpr UINT ImageReady = WM_APP + 401, FileReady = WM_APP + 402, PreferencesReady = WM_APP + 403, RecoveryReadyMessage = WM_APP + 404,
                          FolderReady = WM_APP + 405;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM) noexcept;
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);
    bool RouteShortcut(const MSG &message);
    void BuildUi();
    void PrepareDesignScenario();
    void RefreshOutline();
    [[nodiscard]] bool PreviewOutlineMatchesCommitted() const;
    void RefreshFiles(bool rootChanged = false);
    void EnsureFilesRoot();
    void SelectFilesRoot(const std::filesystem::path &path);
    void OpenFolder();
    void UpdateCurrentFileDirtyBadge(bool dirty);
    void FolderCompleted();
    void OpenPath(const std::filesystem::path &path);
    void RefreshFind();
    void ReplaceFind(bool all);
    void FileMenu(bool overflow = false);
    bool DismissSurface();
    LRESULT FrameHitTest(HWND hwnd, LPARAM point) const;
    void Layout();
    void ApplyTheme();
    void ApplyMarkdownSyntax();
    [[nodiscard]] const std::wstring &CurrentText() const;
    [[nodiscard]] bool SynchronizeMirror();
    void Changed();
    void RefreshDocumentAnalysis(const DxUi::RichText::SourceSnapshot &source, bool committed);
    void UpdateCounts();
    void UpdateCaption(std::wstring caption);
    void SelectionChanged(bool refreshTools = true);
    [[nodiscard]] bool PublishCurrentBlockLabelIfVisible(bool sidebarWillBeVisible = false);
    void FocusSelectionTools();
    void Status(unsigned id);
    void SavePreferences();
    void ServiceImages();
    void FileCompleted();
    void AskThen(std::function<void()> continuation);
    void Open();
    void Save(bool saveAs = false, bool force = false);
    std::filesystem::path PickFile(bool saving, bool image = false, bool folder = false);
    void Menu(POINT point, bool insertOnly = false, bool headingOnly = false);
    void LocalSource();
    void ApplyBlockSource();
    void FindNext(bool previous = false);
    void CloseDrawers();
    void BeginClose();
    void RecoveryCompleted();
    void ShowRecovery();
    void FinishClose();
    DxUi::Button *Action(DxUi::Panel &parent, unsigned label, Command command, std::wstring tooltip = {});
    DxUi::Label *Label(DxUi::Panel &parent, unsigned label, DxUi::FontRole role = DxUi::FontRole::Body);
};
#pragma warning(pop)
[[nodiscard]] std::wstring_view ResourceView(unsigned id);
[[nodiscard]] std::wstring Resource(unsigned id);
[[nodiscard]] std::wstring SampleDocument();
enum class CoreTestScope
{
    All,
    Storage,
    Settings,
    Recovery,
    Editor
};
int RunCoreTests(const std::filesystem::path &root, CoreTestScope scope = CoreTestScope::All, bool listOnly = false);
int RunPerformanceProbe(Application &app, const LaunchOptions &options);
int RunDesignInteractiveProbe(Application &app, const LaunchOptions &options);
} // namespace RedTilde
