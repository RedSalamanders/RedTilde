#pragma once
#include "AsyncFiles.h"
#include "BitmapPaste.h"
#include "Common/DocumentInfo.h"
#include "Common/Files.h"
#include "Common/Recovery.h"
#include "Common/Resources.h"
#include "Common/Settings.h"
#include "EditDiagnostics.h"
#include "ImageLoader.h"
#include "NavigationModel.h"
#include "Printing.h"
#include "UiControls.h"
#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <variant>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
class ApplicationRuntime;
class PrintPreview;
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
    ImageFirstPaste,
    Print,
    CloseDocument
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
    std::optional<Printing::Destinations> printDestinationsForTest;
    std::function<HRESULT(const std::wstring &, const std::optional<Printing::PrinterSettings> &, Printing::PrinterConfiguration &)>
        printPrinterForTest;
    std::function<HRESULT(unsigned, ID2D1CommandList &)> printPageForTest;
    std::function<HRESULT()> printFinishForTest;
    std::function<void(const Printing::Selection &)> printSubmissionForTest;
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
    enum class LifecyclePhase : std::uint8_t
    {
        Uninitialized,
        Initializing,
        Active,
        ClosingAfterSave,
        DrainingForClose,
        SessionEnding,
        Destroyed
    };
    enum class ModalPromptKind : std::uint8_t
    {
        None,
        UnsavedChanges,
        SaveConflict,
        ExternalLink
    };
    enum class OverlayKind : std::uint8_t
    {
        None,
        FloatingSelectionTools,
        FindBar,
        SettingsDrawer,
        BlockDraftModal,
        RecoveryModal
    };
    struct BlockDraftSession
    {
        DxUi::RichText::SourceSnapshot snapshot;
        std::uint64_t revision{};
        DxUi::RichText::MarkdownRange range;
        bool pendingPlacement{};
    };
    struct StartupRestoreState
    {
        enum class Phase : std::uint8_t
        {
            Disabled,
            WaitingForRecovery,
            OpeningLastDocument,
            Completed
        };
        bool enabled{};
        Phase phase{Phase::Disabled};
        std::optional<DxUi::RichText::SourceSnapshot> source;
        [[nodiscard]] bool IsWaitingForRecovery() const noexcept
        {
            return phase == Phase::WaitingForRecovery;
        }
        [[nodiscard]] bool IsOpeningLastDocument() const noexcept
        {
            return phase == Phase::OpeningLastDocument;
        }
    };
    struct WorkspaceApproval
    {
        unsigned key{};
        DxUi::RichText::SourceSnapshot source;
    };
    struct StartupOpenTx
    {
        DxUi::RichText::SourceSnapshot entrySource;
    };
    struct PickerOpenTx
    {
        DxUi::RichText::SourceSnapshot entrySource;
        std::vector<WorkspaceApproval> approvals;
    };
    struct FilesPaneOpenTx
    {
        DxUi::RichText::SourceSnapshot entrySource;
        bool previousWorkspaceSelectionPreferred{};
    };
    struct ConflictReloadTx
    {
        unsigned key{};
        DxUi::RichText::SourceSnapshot source;
        std::filesystem::path path;
    };
    struct HeldOpenTx
    {
        AsyncFiles::Result result;
        bool fromFiles{};
        std::optional<DxUi::RichText::SourceSnapshot> entrySource;
        std::vector<WorkspaceApproval> approvals;
        bool previousWorkspaceSelectionPreferred{};
        std::optional<ConflictReloadTx> conflictReload;
    };
    struct SaveTx
    {
        std::function<void()> afterSave;
    };
    using ActiveFileTransaction = std::variant<std::monostate, StartupOpenTx, PickerOpenTx, FilesPaneOpenTx, HeldOpenTx, SaveTx, ConflictReloadTx>;
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
    [[nodiscard]] bool IsInitializing() const noexcept
    {
        return lifecyclePhase == LifecyclePhase::Initializing;
    }
    [[nodiscard]] bool IsClosingAfterSave() const noexcept
    {
        return lifecyclePhase == LifecyclePhase::ClosingAfterSave;
    }
    [[nodiscard]] bool IsClosing() const noexcept
    {
        return lifecyclePhase == LifecyclePhase::DrainingForClose || lifecyclePhase == LifecyclePhase::Destroyed;
    }
    [[nodiscard]] bool IsSessionEnding() const noexcept
    {
        return lifecyclePhase == LifecyclePhase::SessionEnding || sessionEndCommitted;
    }
    [[nodiscard]] bool IsUiAttached() const noexcept
    {
        return lifecyclePhase != LifecyclePhase::Uninitialized && !IsClosing() && root && host.GetRoot() == root;
    }
    [[nodiscard]] bool CanAcceptCommand() const noexcept
    {
        return lifecyclePhase == LifecyclePhase::Active && !switchingDocument && root && host.GetRoot() == root;
    }
    [[nodiscard]] bool AreShutdownWorkersSettled() const noexcept
    {
        return !preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy();
    }
    [[nodiscard]] bool HasActiveModalPrompt() const noexcept
    {
        return modalPrompt != ModalPromptKind::None;
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
        return activeOverlay == OverlayKind::BlockDraftModal && blockDraftSession.has_value() && blockDraft && blockDraft->IsVisible();
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
        return session.recovered;
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
    friend class CloseProbe;
    friend class DesignInteractiveProbe;
    friend class PrintPreview;
#if defined(REDTILDE_ENABLE_TESTS)
    friend int RunFailureHostProbe(std::wstring_view, const std::filesystem::path &, ApplicationRuntime &);
    friend class PrintingProbe;
    friend class FilesProbe;
    friend class RecoveryProbe;
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
    void TransitionLifecycle(LifecyclePhase next) noexcept;
    void ReportUiFailure(std::string_view cause) noexcept;
    [[nodiscard]] bool AcceptSourceResult(DxUi::RichText::SourceResult result);
    void ExecuteCommand(Command command, unsigned zoomSteps = 1);
    void DispatchUiMessage(const MSG &message) noexcept;
    void OnMenuClosed(std::optional<int> selected) noexcept;
    EditDiagnostics *editDiagnostics{}; // Borrowed only for a synchronous harness transaction.
    ApplicationActivity activity;
    template <typename T> class ReentrantPublication
    {
      public:
        [[nodiscard]] bool Matches(const T &value) const noexcept
        {
            return published_.has_value() && published_.value() == value;
        }
        [[nodiscard]] std::uint64_t BeginPublish() noexcept
        {
            published_.reset();
            return ++generation_;
        }
        void CommitPublish(std::uint64_t token, T value)
        {
            if (generation_ == token)
                published_ = std::move(value);
        }
        void Invalidate() noexcept
        {
            ++generation_;
            published_.reset();
        }
        [[nodiscard]] const std::optional<T> &Published() const noexcept
        {
            return published_;
        }

      private:
        std::optional<T> published_;
        std::uint64_t generation_{};
    };
    ReentrantPublication<std::wstring> appliedCaption;
    LaunchOptions options;
    Settings settings;
    SettingsWriter preferenceWriter;
    enum class RecoveryDisposition : std::uint8_t
    {
        Available,
        Discarding
    };
    struct OfferedRecoveryCandidate
    {
        std::wstring id;
        RecoverySnapshot snapshot;
        RecoveryDisposition disposition{RecoveryDisposition::Available};
    };
    RecoveryService recovery;
    std::vector<OfferedRecoveryCandidate> recoveryCandidates;
    bool recoveryOffered{};
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
        *equationReadOnly{}, *writingTitle{}, *syntaxTitle{}, *syntaxIntro{}, *privacyTitle{}, *drawerImmediate{}, *aboutTitle{}, *aboutVersion{},
        *aboutSummary{}, *shortcutsList{};
    std::array<DxUi::Button *, MarkdownSyntaxCount> syntaxToggles{};
    std::array<DxUi::Label *, MarkdownSyntaxCount> syntaxHelp{};
    std::array<DxUi::Separator *, 6> commandDividers{};
    DxUi::CardPanel *saveStateChip{};
    DxUi::Label *filesSectionTitle{}, *outlineCount{}, *filesRootIcon{}, *listSummaryIcon{};
    DxUi::Button *closeFiles{}, *blockSourceAction{};
    DxUi::Separator *contextDivider{}, *filesFooterDivider{};
    DxUi::Label *listSummary{};
    ReentrantPublication<std::pair<unsigned, std::size_t>> listSummaryFacts;
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
    struct FindController
    {
        struct MatchTable
        {
            std::optional<DxUi::RichText::SourceSnapshot> source;
            std::wstring query;
            bool matchCase{};
            std::vector<DxUi::RichText::MarkdownRange> matches;
        };
        struct Projections
        {
            std::vector<std::size_t> sectionCounts;
            std::uint64_t badgeRowsRevision{}, sectionOutlineRevision{}, sectionAnalysis{};
            std::optional<std::size_t> highlightedCurrent, displayedCurrent, displayedTotal;
            std::wstring displayedQuery;
            bool presentationCleared{true}, sectionCountsValid{}, highlightsNeedRefresh{true}, hasHighlights{}, badgesNeedRefresh{true};
        };
        MatchTable table;
        Projections projections;
        bool matchCase{}, updating{};

        [[nodiscard]] bool IsPresentationActive() const noexcept
        {
            return !projections.presentationCleared;
        }
        void InvalidateMatches() noexcept
        {
            table.source.reset();
        }
    };
    FindController find;
    int zoomWheelRemainder{};
    std::vector<std::filesystem::path> visibleFilePaths;
    std::optional<std::size_t> currentHeading;
    struct CurrentBlockLabelFacts
    {
        unsigned resourceId{};
        std::size_t lineCharacterCount{};
        bool operator==(const CurrentBlockLabelFacts &) const noexcept = default;
    };
    ReentrantPublication<CurrentBlockLabelFacts> currentBlockPublishedFacts;
    std::optional<CurrentBlockLabelFacts> currentBlockLatestFacts;
    ReentrantPublication<bool> selectionCaptionPublishedEquationContext;
    bool equationContext{}, pendingSelectionTools{}, postPreparePosted{}, systemDark{};
    AsyncFiles files;
    RichImageLoader images;
    std::shared_ptr<Printing::Operation> printOperation;
    std::unique_ptr<PrintPreview> printPreview;
    std::optional<Printing::PrinterConfiguration> printConfiguration;
    std::optional<Printing::PrinterSettings> printSettings;
    std::wstring printPrinter;
    DxUi::Control *printInvoker{};
    std::weak_ptr<int> printInvokerLifetime;
    std::wstring printTitle;
    std::vector<Printing::Image> printNetworkImages;
    void Print();
    void PreviewPrintPage(unsigned number);
    void PreviewPrintDestination(std::optional<Printing::PrinterSettings> requested = std::nullopt);
    void SubmitPrintPreview();
    void ClosePrintPreview() noexcept;
    void DismissPrintPreview() noexcept;
    void PrintCompleted();
    void ReportPrintFailure(HRESULT result, Printing::Stage stage, std::wstring_view printer = {}, unsigned pageNumber = 0u);
    void RememberPrintImage(const DxUi::RichText::RichImageRequest &request, std::shared_ptr<const DxUi::RichText::RichImage> pixels);
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
    OverlayKind activeOverlay{OverlayKind::None};
    std::optional<BlockDraftSession> blockDraftSession;
    std::vector<DxUi::Button *> tools, floatingActions, themeButtons;
    std::array<DxUi::Button *, 3> modeButtons{};
    DxUi::Button *outlineToggle{}, *focusToggle{};
    std::filesystem::path folderRoot;
    Surface *imageConsent{};
    DxUi::Label *imageConsentText{};
    DxUi::Button *imageConsentAction{}, *imageReload{};
    std::shared_ptr<const DxUi::RichText::SemanticDocument> imageConsentSource;
    std::filesystem::path imageConsentPath;
    bool imageConsentHasNetwork{};
    HRESULT imageFailureStatus = S_OK;
    struct DocumentSessionMetadata
    {
        std::filesystem::path path;
        std::wstring savedText;
        FileStamp stamp;
        std::set<std::uint64_t> collapsedHeadings;
        unsigned key{};
        bool bom{};
        bool recovered{};
        bool remoteImagesAllowed{};
    };
    DocumentSessionMetadata session;
    struct WorkspaceDocument
    {
        DxUi::RichText::Document state;
        DocumentSessionMetadata metadata;
        bool dirty{};
    };
    std::vector<WorkspaceDocument> inactiveDocuments;
    static constexpr std::uint64_t WorkspaceGroupId = std::uint64_t{1} << 63;
    bool workspaceCollapsed{}, workspaceSelectionPreferred{};
    std::vector<WorkspaceApproval> workspaceApprovals;
    bool switchingDocument{};
    HRESULT sessionEndRecoveryStatus = S_OK;
    bool lastFilesDirty{};
    HRESULT folderAdmissionStatus = S_OK;
    struct FilesFooterMeasureCache
    {
        float widthDip = -1, heightDip = 52, dpi{};
        std::uint64_t textRevision{}, typographyRevision{}, measurements{};
    } filesFooterMeasure;
    struct DerivedDocumentCaches
    {
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
        std::shared_ptr<const DxUi::RichText::MarkdownSyntax> selectionFactsSyntax;
        DxUi::RichText::Selection selectionFactsSelection;
        EditorView selectionFactsView = EditorView::Compose;
        std::uint64_t selectionFactsAnalysis{};
        std::uint64_t outlineRowsRevision{};

        void Reset() noexcept
        {
            dirtyFact.reset();
            cachedSource.reset();
            cachedText.clear();
            mirroredSource.reset();
            selectionFactsSource.reset();
            documentAnalysisSource.reset();
            committedAnalysisSource.reset();
            selectionFactsSyntax.reset();
            selectionFactsSelection = {};
            selectionFactsView = EditorView::Compose;
            selectionFactsAnalysis = 0;
            outlineRowsRevision = 0;
        }
    };
    DerivedDocumentCaches derivedCaches;
    struct OutlinePreviewBaseline
    {
        std::set<std::uint64_t> collapsedHeadings;
        std::vector<std::pair<unsigned, std::wstring>> committedOutline;
    };
    std::optional<OutlinePreviewBaseline> outlinePreviewBaseline;
    struct SelectionToolsFocusRequest
    {
        DxUi::RichText::SourceSnapshot source;
        DxUi::RichText::Selection selection;
        EditorView view;
    };
    std::optional<SelectionToolsFocusRequest> selectionToolsFocus;
    LifecyclePhase lifecyclePhase{LifecyclePhase::Uninitialized};
    bool sessionEndCommitted{};
    bool reportingUiFailure{};
    ModalPromptKind modalPrompt{ModalPromptKind::None};
    bool visibleBeforeSaveClose{};
#if defined(REDTILDE_ENABLE_TESTS)
    std::uint64_t preferencesReadySentDuringClose{};
#endif
    bool windowPlacementDirty{};
    StartupRestoreState startupRestore;
    struct ExternalLinkState;
    struct ExternalLinkJob;
    std::shared_ptr<ExternalLinkState> externalLink;
    DocumentInfo documentInfo;
    DocumentLineIndexStorage documentLineIndexStorage;
    ActiveFileTransaction activeFileTx;
    static constexpr UINT ImageReady = WM_APP + 401, FileReady = WM_APP + 402, PreferencesReady = WM_APP + 403, RecoveryReadyMessage = WM_APP + 404,
                          FolderReady = WM_APP + 405, ExternalLinkReady = WM_APP + 406, BitmapReady = WM_APP + 407, ImageFileChanged = WM_APP + 408,
                          PrintReady = WM_APP + 409, PostPreparePlacement = WM_APP + 410;
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
    [[nodiscard]] bool OpenDroppedFile(const std::filesystem::path &path);
    void CloseActiveDocument();
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
    void ReportCapacityLimit();
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
    void SetActiveOverlay(OverlayKind next);
    [[nodiscard]] bool ReconcilePostPreparePlacement();
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
    EditorRendering,
    EditorContent,
    EditorWorkspace,
    EditorMedia
};
int RunCoreTests(const std::filesystem::path &root, CoreTestScope scope = CoreTestScope::All, bool listOnly = false);
#if defined(REDTILDE_ENABLE_TESTS)
int RunLinkedSaveInterruptionProbe(const std::filesystem::path &root, bool restart);
#endif
int RunPerformanceProbe(Application &app, const LaunchOptions &options);
int RunDesignInteractiveProbe(Application &app, const LaunchOptions &options);
} // namespace RedTilde
