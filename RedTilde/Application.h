#pragma once
#include "AsyncFiles.h"
#include "BitmapPaste.h"
#include "Common/DocumentInfo.h"
#include "Common/Files.h"
#include "Common/Recovery.h"
#include "Common/Settings.h"
#include "EditDiagnostics.h"
#include "ImageLoader.h"
#include "NavigationModel.h"
#include "UiControls.h"
#include <array>
#include <exception>
#include <filesystem>
#include <functional>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
class ApplicationRuntime;
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
    FindMatchCase,
    Replace,
    ReplaceAll,
    Minimize,
    Maximize,
    Close,
    DimBlocks,
    RemoteImages,
    LocalNetworkImages,
    DocumentImages,
    ReloadImages,
    // Ordered as MarkdownSyntax; each toggles one optional RichMarkdown syntax.
    SyntaxHighlight,
    SyntaxSuperscript,
    SyntaxSubscript,
    SyntaxInserted,
    SyntaxWikiLinks,
    SyntaxDefinitionLists,
    SyntaxAbbreviations,
    OpenFolder,
    StrikeCompletedTasks,
    RestoreAllRecovery,
    ZoomIn,
    ZoomOut,
    ZoomReset,
    OpenLink,
    ImageFirstPaste
};
enum class SaveConflictChoice
{
    Cancel,
    Reload,
    SaveAs,
    Overwrite
};
struct LaunchOptions
{
    int showCommand = SW_SHOWNORMAL;
    bool explicitShowCommand{};
    bool headless{}, selfTest{}, crashProbe{}, preferencesProbe{}, recoveryProbe{}, recoveryRestartProbe{};
    bool resourceProbe{}, forceWarp{}, controlDiagnostics{};
    unsigned performanceCycles = 12;
    unsigned captureDpi{}, viewportWidthPx{}, viewportHeightPx{};
    std::wstring scenario = L"compose-light";
    std::filesystem::path document, capture, dataDirectory, benchmark, performance, designInteractive;
#if defined(REDTILDE_ENABLE_TESTS)
    bool startupRestoreForTest{}; // Exercise ordinary startup policy in a hidden owner; no new production switch.
    // Hidden harness replacement for the ordinary unsaved-change question.
    std::function<int()> unsavedPromptForTest;
    std::function<SaveConflictChoice()> saveConflictForTest;
    // Directed recoverable failures at the real UI entry points; never compiled into production.
    std::function<HRESULT(Command)> commandAdmissionForTest;
    std::function<HRESULT()> fileCompletionAdmissionForTest;
    std::function<std::filesystem::path(bool)> filePickerForTest;
    std::shared_ptr<AsyncFiles::TestGate> fileGateForTest;
    std::function<bool(std::wstring_view)> confirmLinkForTest;
    std::function<HRESULT(std::wstring_view)> launchLinkForTest;
    std::shared_ptr<AsyncFiles::TestGate> linkGateForTest;
#endif
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
    std::uint64_t workspaceSwitches{}, workspaceRetainedPeak = 1;
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
    void Execute(Command command) noexcept;
    void SetView(EditorView view);
    bool SetDocument(std::wstring text, std::filesystem::path path = {}, bool bom = false, FileStamp stamp = {}, bool rememberForStartup = true);
    // Interaction probes settle content first, then capture without pumping
    // ambient pointer-leave messages for their deliberately hidden HWND.
    bool Capture(const std::filesystem::path &path, bool prepareContent = true);
    // Bounded harness owner turns, including independent editor/mirror cosmetics.
    // This is not the production event loop or an input-readiness prerequisite.
    void Prepare(unsigned turns = 128);
    [[nodiscard]] bool Dirty() const;
    [[nodiscard]] bool IsSessionEnding() const noexcept
    {
        return sessionEnding;
    }
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
#if defined(REDTILDE_ENABLE_TESTS)
    friend int RunFailureHostProbe(std::wstring_view, const std::filesystem::path &, ApplicationRuntime &);
#endif
    template <class Action> void RunUiAction(Action &&action) noexcept
    {
        // Expected refusal uses explicit status/results. Unexpected C++ failures at this ABI/callback
        // boundary use the process-owned original-context reporter and cannot resume a partial action.
        try
        {
            const DxUi::Failure::Scope actionScope;
            std::forward<Action>(action)();
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
    Application(LaunchOptions value, const DxUi::Failure::Scope &constructionScope);
    void ReportUiFailure(std::string_view cause) noexcept;
    [[nodiscard]] bool AcceptSourceResult(DxUi::RichText::SourceResult result);
    void ExecuteCommand(Command command, unsigned zoomSteps = 1);
    void DispatchUiMessage(const MSG &message) noexcept;
    void OnMenuClosed(std::optional<int> selected) noexcept;
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
        *maximizeButton{}, *closeButton{}, *dimToggle{}, *strikeTasksToggle{}, *remoteImagesToggle{}, *localNetworkImagesToggle{}, *recoveryAction{},
        *drawerClose{};
    DxUi::NumericStepper *readingWidth{}, *textScale{};
    DxUi::ScrollPanel *settingsScroll{};
    DxUi::CardPanel *selectionHintChip{};
    DxUi::Label *selectionHint{};
    std::vector<DxUi::RichText::MarkdownRange> findMatches;
    std::optional<DxUi::RichText::SourceSnapshot> findSource;
    std::wstring findQuery;
    std::vector<DxUi::RichText::MarkdownRange> closedFindMatches;
    std::optional<DxUi::RichText::SourceSnapshot> closedFindSource;
    std::wstring closedFindQuery;
    bool findMatchCase{}, findCachedMatchCase{}, closedFindCase{};
    int zoomWheelRemainder{};
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
    FileChangeService imageFiles;
    std::vector<std::filesystem::path> watchedImagePaths;
    std::shared_ptr<const DxUi::RichText::SemanticDocument> imageWatchSemantic;
    std::filesystem::path imageWatchDocumentPath;
    bool imageBindingTransition{};
    std::uint64_t imageFileRefreshes{}, imageWatchPathBuilds{};
    DxUi::Button *imageFirstPasteToggle{};
    BitmapPasteWriter bitmapWriter;
    struct PendingBitmapPaste
    {
        std::uint64_t request{};
        unsigned documentKey{};
        DxUi::RichText::SourceSnapshot source;
        DxUi::RichText::Selection selection;
        EditorView view{};
        std::filesystem::path documentPath;
        std::weak_ptr<int> lifetime;
    };
    std::optional<PendingBitmapPaste> pendingBitmapPaste;
    std::vector<DxUi::RichText::RichImageRequest> imageRequests;
    Editor *editor{};
    DxUi::RichTextEditor *mirror{};
    Surface *root{}, *documentBar{}, *commandBar{}, *page{}, *sidebar{}, *floating{}, *drawer{}, *findBar{}, *blockDraft{};
    DxUi::Label *title{}, *saveState{}, *stats{}, *feedback{}, *outlineTitle{}, *equationHelp{}, *modeHint{};
    DxUi::RichTextEditor *equation{};
    DxUi::TextField *findText{};
    DxUi::Button *findCaseToggle{};
    DxUi::TextField *blockDraftText{};
    Surface *recoveryPanel{};
    DxUi::TextField *recoveryPreview{};
    DxUi::Label *recoveryDescription{};
    DxUi::Button *restoreAllRecovery{};
    DxUi::RichText::SourceSnapshot blockDraftSnapshot;
    DxUi::RichText::MarkdownRange blockDraftRange;
    std::uint64_t blockDraftRevision{};
    std::vector<DxUi::Button *> tools, floatingActions, themeButtons;
    std::array<DxUi::Button *, 3> modeButtons{};
    DxUi::Button *outlineToggle{}, *focusToggle{};
    std::filesystem::path documentPath, folderRoot;
    Surface *imageConsent{};
    DxUi::Label *imageConsentText{};
    DxUi::Button *imageConsentAction{}, *imageReload{};
    std::shared_ptr<const DxUi::RichText::SemanticDocument> imageConsentSource;
    std::filesystem::path imageConsentPath;
    bool documentRemoteImagesAllowed{}, imageConsentHasNetwork{};
    HRESULT imageFailureStatus = S_OK;
    struct WorkspaceDocument
    {
        DxUi::RichText::Document state;
        std::filesystem::path path;
        std::wstring savedText;
        FileStamp stamp;
        std::set<std::uint64_t> collapsedHeadings;
        unsigned recoveryKey{};
        bool bom{}, recovered{}, dirty{}, remoteImagesAllowed{};
    };
    struct WorkspaceApproval
    {
        unsigned key{};
        DxUi::RichText::SourceSnapshot source;
    };
    std::vector<WorkspaceDocument> inactiveDocuments;
    static constexpr std::uint64_t WorkspaceGroupId = std::uint64_t{1} << 63;
    bool workspaceCollapsed{};
    std::vector<WorkspaceApproval> workspaceApprovals, openingApprovals;
    unsigned activeDocumentKey{};
    bool switchingDocument{}, openingFromFiles{};
    bool sessionEnding{};
    HRESULT sessionEndRecoveryStatus = S_OK;
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
    bool bom{}, closing{}, initialized{}, initializing{}, unsavedPromptOpen{}, reportingUiFailure{};
    bool closingAfterSave{}, visibleBeforeSaveClose{};
    bool waitingForPreferences{};
    bool windowPlacementDirty{};
    bool startupPolicy{}, startupPending{}, startupOpening{};
    struct ExternalLinkState;
    struct ExternalLinkJob;
    std::shared_ptr<ExternalLinkState> externalLink;
    bool linkPromptOpen{};
    std::optional<DxUi::RichText::SourceSnapshot> startupSource;
    FileStamp stamp;
    DocumentInfo documentInfo;
    DocumentLineIndexStorage documentLineIndexStorage;
    std::function<void()> afterSave;
    std::optional<DxUi::RichText::SourceSnapshot> openingSnapshot;
    std::optional<AsyncFiles::Result> heldOpen;
    struct ConflictReload
    {
        unsigned key{};
        DxUi::RichText::SourceSnapshot source;
        std::filesystem::path path;
    };
    std::optional<ConflictReload> conflictReload;
    static constexpr UINT ImageReady = WM_APP + 401, FileReady = WM_APP + 402, PreferencesReady = WM_APP + 403, RecoveryReadyMessage = WM_APP + 404,
                          FolderReady = WM_APP + 405, ExternalLinkReady = WM_APP + 406, BitmapReady = WM_APP + 407, ImageFileChanged = WM_APP + 408;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM) noexcept;
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);
    bool RouteShortcut(const MSG &message);
    [[nodiscard]] HRESULT BuildUi();
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
    [[nodiscard]] bool SwitchWorkspaceDocument(std::size_t index);
    [[nodiscard]] bool StartWorkspaceDocument();
    void ResetWorkspace();
    void CheckpointWorkspaceRecovery();
    void AskWorkspaceThen(std::function<void()> continuation, std::vector<WorkspaceApproval> approved = {});
    void RefreshFind();
    void ReplaceFind(bool all);
    void FileMenu(bool overflow = false);
    bool DismissSurface();
    LRESULT FrameHitTest(HWND hwnd, LPARAM point) const;
    void Layout();
    [[nodiscard]] static RECT ClampWindowBounds(RECT requested, RECT workArea, unsigned savedDpi, unsigned currentDpi) noexcept;
    [[nodiscard]] int InitialShowCommand() const noexcept;
    [[nodiscard]] bool MayRequestNativeFocus() const noexcept;
    void RecordWindowPlacement(bool persist);
    void AbandonStartupRestore();
    void RememberActiveDocument();
    [[nodiscard]] static bool SupportedExternalLink(std::wstring_view destination) noexcept;
    [[nodiscard]] std::optional<std::size_t> CurrentLinkElement(const std::shared_ptr<const DxUi::RichText::SemanticDocument> &document) const;
    void RequestOpenLink(const DxUi::RichTextLinkActivation &activation);
    void ExternalLinkCompleted();
    void CloseExternalLinks() noexcept;
    static DWORD WINAPI ExternalLinkWorker(void *context) noexcept;
    [[nodiscard]] std::vector<DxUi::MenuFlyoutItem> ContextMenuItems(bool insertOnly, bool headingOnly);
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
    void RefreshImageConsent();
    std::optional<std::wstring> BeginBitmapPaste(std::span<const std::byte> dib, bool v5);
    void ServiceBitmapPaste();
    void FileCompleted() noexcept;
    void CompleteFileOperation();
    void AskThen(std::function<void()> continuation);
    void Open();
    void Save(bool saveAs = false, bool force = false, std::optional<std::filesystem::path> selectedPath = {});
    void ResolveSaveConflict(const AsyncFiles::Result &result);
    std::filesystem::path PickFile(bool saving, bool image = false, bool folder = false);
    void Menu(POINT point, bool insertOnly = false, bool headingOnly = false);
    void LocalSource();
    void ApplyBlockSource();
    void FindNext(bool previous = false);
    void CloseDrawers();
    void BeginClose();
    void ResumeAfterSaveClose();
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
    Editor,
    EditorBehavior,
    EditorPerformance,
    EditorRendering
};
int RunCoreTests(const std::filesystem::path &root, CoreTestScope scope = CoreTestScope::All, bool listOnly = false);
#if defined(REDTILDE_ENABLE_TESTS)
int RunLinkedSaveInterruptionProbe(const std::filesystem::path &root, bool restart);
#endif
int RunPerformanceProbe(Application &app, const LaunchOptions &options);
int RunDesignInteractiveProbe(Application &app, const LaunchOptions &options);
} // namespace RedTilde
