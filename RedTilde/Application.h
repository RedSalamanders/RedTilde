#pragma once
#include "AsyncFiles.h"
#include "BitmapPaste.h"
#include "Common/CredentialStore.h"
#include "Common/CustomDictionary.h"
#include "Common/DocumentInfo.h"
#include "Common/FileOps.h"
#include "Common/Files.h"
#include "Common/GitHubClient.h"
#include "Common/ProseExtractor.h"
#include "Common/Recovery.h"
#include "Common/Resources.h"
#include "Common/Settings.h"
#include "Common/SpellProvider.h"
#include "EditDiagnostics.h"
#include "GitHubService.h"
#include "ImageLoader.h"
#include "NavigationModel.h"
#include "Printing.h"
#include "SidePaneRail.h"
#include "SpellService.h"
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
    CloseDocument,
    SwitchSplitPane,
    ToggleSplitScrollSync,
    PinFiles,
    PinOutline,
    ToggleSpelling,
    SpellIgnoreUppercase,
    SpellIgnoreDigits,
    SpellIgnoreInternetPaths,
    SpellCycleLanguage,
    SpellChooseCustomDictionary,
    SpellResetCustomDictionary,
    OpenProofingSettings,
    AddFolder,
    OpenGitHub,
    ExitUnsavedBehavior,
    SaveConfirmAccept,
    SaveConfirmKeepExit,
    SaveConfirmDiscard,
    SaveConfirmSelectAll,
    RenameConfirm,
    FileConflictReplace,
    FileConflictKeepBoth,
    GitHubOpenConfirm,
    GitHubSignInDevice,
    GitHubUseToken,
    GitHubImportGhCli,
    GitHubSignOut,
    FilesCtxOpen,
    FilesCtxRevealExplorer,
    FilesCtxRevealRedSalamander,
    FilesCtxCopyFullPath,
    FilesCtxCopyRelPath,
    FilesCtxRename,
    FilesCtxCut,
    FilesCtxCopy,
    FilesCtxPaste,
    FilesCtxRemoveRoot,
    FilesCtxRemoveRepo
};

enum class SplitActivePane : std::uint8_t
{
    Compose,
    Source
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
    bool watchSettingsForTest{};  // Exercise live settings watcher against an isolated test directory.
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
    std::optional<DWORD> printPrinterTimeoutMsForTest;
    std::optional<UINT> printWaitingDelayMsForTest;
    std::optional<Printing::Destinations> printDestinationsForTest;
    std::function<HRESULT(const std::wstring&, const std::optional<Printing::PrinterSettings>&, Printing::PrinterConfiguration&)> printPrinterForTest;
    std::function<HRESULT(unsigned, ID2D1CommandList&)> printPageForTest;
    std::function<HRESULT()> printFinishForTest;
    std::function<void(const Printing::Selection&)> printSubmissionForTest;
    std::shared_ptr<ISpellProvider> spellProviderForTest;
    bool forceDxUiSaveDialogForTest{};
    std::function<HRESULT(std::wstring_view)> clipboardWriterForTest;
    std::function<std::optional<FileClipboardPayload>()> clipboardDropReaderForTest;
    std::function<HRESULT(const FileClipboardPayload&)> clipboardDropWriterForTest;
    std::shared_ptr<IShellReveal> shellRevealForTest;
    std::shared_ptr<IWorkspaceFileOps> fileOpsForTest;
    std::shared_ptr<IGitHubTransport> githubTransportForTest;
    std::shared_ptr<ICredentialStore> credentialStoreForTest;
    std::function<std::optional<std::wstring>()> ghCliImporterForTest;
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
    std::uint64_t splitPaneSwitches{}, splitScrollSyncs{};
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
        ExternalLink,
        FileDialog,
        ErrorMessage
    };
    enum class OverlayKind : std::uint8_t
    {
        None,
        FloatingSelectionTools,
        FindBar,
        SettingsDrawer,
        BlockDraftModal,
        RecoveryModal,
        SaveConfirmModal,
        RenameFileModal,
        FileConflictModal,
        OpenGitHubModal
    };
    struct SaveConfirmItem
    {
        unsigned workspaceKey{};
        bool isActive{};
        bool selected{true};
        WorkspaceDisplayLocation location;
    };
    struct SaveConfirmSession
    {
        std::vector<SaveConfirmItem> items;
        std::function<void()> continuation;
        bool closingApplication{};
        std::size_t pendingSaveIndex{};
        bool sequentialSaveActive{};
    };
    struct RenameFileSession
    {
        std::filesystem::path targetPath;
        bool isDirectory{};
    };
    struct FileConflictSession
    {
        std::vector<std::filesystem::path> sources;
        std::filesystem::path targetDirectory;
        std::filesystem::path firstConflictName;
        bool move{};
    };
    struct SaveConfirmRowControls
    {
        DxUi::Checkbox* checkbox{};
        DxUi::Label* locationLabel{};
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
    bool Capture(const std::filesystem::path& path, bool prepareContent = true);
    // Bounded harness owner turns, including independent editor/mirror cosmetics.
    // This is not the production event loop or an input-readiness prerequisite.
    void Prepare(unsigned turns = 128);
    [[nodiscard]] bool Dirty() const;
    [[nodiscard]] bool IsInitializing() const noexcept
    {
        return _lifecyclePhase == LifecyclePhase::Initializing;
    }
    [[nodiscard]] bool IsClosingAfterSave() const noexcept
    {
        return _lifecyclePhase == LifecyclePhase::ClosingAfterSave;
    }
    [[nodiscard]] bool IsClosing() const noexcept
    {
        return _lifecyclePhase == LifecyclePhase::DrainingForClose || _lifecyclePhase == LifecyclePhase::Destroyed;
    }
    [[nodiscard]] bool IsSessionEnding() const noexcept
    {
        return _lifecyclePhase == LifecyclePhase::SessionEnding || _sessionEndCommitted;
    }
    [[nodiscard]] bool IsUiAttached() const noexcept
    {
        return _lifecyclePhase != LifecyclePhase::Uninitialized && ! IsClosing() && _root && _host.GetRoot() == _root;
    }
    [[nodiscard]] bool CanAcceptCommand() const noexcept
    {
        return _lifecyclePhase == LifecyclePhase::Active && ! _switchingDocument && _root && _host.GetRoot() == _root;
    }
    [[nodiscard]] bool AreShutdownWorkersSettled() const noexcept
    {
        return ! _preferenceWriter.Busy() && _recovery.Finished() && ! _bitmapWriter.Busy() && _spellService.Finished() && ! _fileOps.Busy() &&
               _github.Finished();
    }
    [[nodiscard]] bool HasActiveModalPrompt() const noexcept
    {
        return _modalPrompt != ModalPromptKind::None;
    }
    [[nodiscard]] Editor& Document() const
    {
        return *ActiveEditor();
    }
    [[nodiscard]] DxUi::ControlHost& Host()
    {
        return _host;
    }
    [[nodiscard]] const Settings& Preferences() const
    {
        return _settings;
    }
    [[nodiscard]] HWND Window() const
    {
        return _window.get();
    }
    [[nodiscard]] DxUi::TextField* BlockDraft() const
    {
        return _blockDraftText;
    }
    [[nodiscard]] bool HasBlockDraft() const
    {
        return _activeOverlay == OverlayKind::BlockDraftModal && _blockDraftSession.has_value() && _blockDraft && _blockDraft->IsVisible();
    }
    [[nodiscard]] std::size_t RecoveryCount() const
    {
        return _recoveryCandidates.size();
    }
    [[nodiscard]] bool RecoveryReady() const
    {
        return _recovery.Ready();
    }
    [[nodiscard]] bool RecoverySettled() const
    {
        return _recovery.Settled();
    }
    [[nodiscard]] HRESULT RecoveryStatus() const
    {
        return _recovery.Status();
    }
    [[nodiscard]] std::wstring EquationPreviewText() const
    {
        return _equation->GetText();
    }
    [[nodiscard]] bool RequiresSaveAs() const
    {
        return _session.recovered || _session.origin.kind == DocumentOriginInfo::Kind::GitHub;
    }
    void CheckpointRecovery(bool immediate = false, RecoverySnapshotKind kind = RecoverySnapshotKind::CrashRecovery);
    [[nodiscard]] ApplicationActivity Activity() const noexcept
    {
        return _activity;
    }
    [[nodiscard]] const Editor& SourceMirror() const noexcept
    {
        return *MirrorEditor();
    }
    [[nodiscard]] SplitActivePane ActiveSplitPane() const noexcept
    {
        return _splitActivePane;
    }
    [[nodiscard]] bool IsSplitScrollSyncEnabled() const noexcept
    {
        return _settings.splitScrollSync;
    }

private:
    friend class PerformanceProbe;
    friend class DesignRefreshProbe;
    friend class CloseProbe;
    friend class DesignInteractiveProbe;
    friend class PrintPreview;
#if defined(REDTILDE_ENABLE_TESTS)
    friend int RunFailureHostProbe(std::wstring_view, const std::filesystem::path&, ApplicationRuntime&);
    friend class PrintingProbe;
    friend class FilesProbe;
    friend class RecoveryProbe;
    friend class SettingsProbe;
    friend class PanesProbe;
    friend class SplitEditingProbe;
    friend class SpellingProbe;
    friend class WorkspaceDialogsProbe;
    friend class MultiFolderFilesProbe;
    friend class GitHubUiProbe;
#endif
    template <class Action> void RunUiAction(Action&& action) noexcept
    {
        // Expected refusal uses explicit status/results. Unexpected C++ failures at this ABI/callback
        // boundary use the process-owned original-context reporter and cannot resume a partial action.
        try
        {
            const DxUi::Failure::Scope actionScope;
            std::forward<Action>(action)();
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
    Application(LaunchOptions value, const DxUi::Failure::Scope& constructionScope);
    void TransitionLifecycle(LifecyclePhase next) noexcept;
    void ReportUiFailure(std::string_view cause) noexcept;
    [[nodiscard]] bool AcceptSourceResult(DxUi::RichText::SourceResult result);
    void ExecuteCommand(Command command, unsigned zoomSteps = 1);
    void DispatchUiMessage(const MSG& message) noexcept;
    void OnMenuClosed(std::optional<int> selected) noexcept;
    EditDiagnostics* _editDiagnostics{}; // Borrowed only for a synchronous harness transaction.
    ApplicationActivity _activity;
    template <typename T> class ReentrantPublication
    {
    public:
        [[nodiscard]] bool Matches(const T& value) const noexcept
        {
            return _published.has_value() && _published.value() == value;
        }
        [[nodiscard]] std::uint64_t BeginPublish() noexcept
        {
            _published.reset();
            return ++_generation;
        }
        void CommitPublish(std::uint64_t token, T value)
        {
            if (_generation == token)
                _published = std::move(value);
        }
        void Invalidate() noexcept
        {
            ++_generation;
            _published.reset();
        }
        [[nodiscard]] const std::optional<T>& Published() const noexcept
        {
            return _published;
        }

    private:
        std::optional<T> _published;
        std::uint64_t _generation{};
    };
    ReentrantPublication<std::wstring> _appliedCaption;
    LaunchOptions _options;
    Settings _settings;
    SettingsWriter _preferenceWriter;
    SettingsWatcher _settingsWatcher;
    std::optional<FileStamp> _settingsFileStamp;
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
    RecoveryService _recovery;
    std::vector<OfferedRecoveryCandidate> _recoveryCandidates;
    bool _recoveryOffered{};
    std::uint64_t _recoveryGeneration{};
    wil::unique_hwnd _window;
    NavigationModel _outlineModel, _filesModel;
    DxUi::ControlHost _host;
    FilesService _folderFiles;
    MultiFolderFilesService _multiFolderFiles;
    std::optional<FilesSnapshot> _folderSnapshot;
    std::vector<FilesSnapshot> _folderSnapshots;
    std::set<std::filesystem::path> _collapsedFolders;
    std::set<std::filesystem::path> _collapsedLocalRoots;
    std::set<std::wstring> _collapsedGitHubRepos;
    std::set<std::wstring> _collapsedGitHubDirs;
    std::set<std::wstring> _cutFilePaths;
    FileOpsService _fileOps;
    GitHubService _github;
    std::shared_ptr<ICredentialStore> _credentials;
    std::shared_ptr<IShellReveal> _shellReveal;
    std::optional<std::filesystem::path> _redSalamanderInstalledPath;
    struct GitHubConnectedRepoState
    {
        GitHubRepoRef ref;
        std::wstring resolvedBranch;
        std::vector<GitHubTreeEntry> entries;
        bool loaded{};
        bool loading{};
        HRESULT lastError{S_OK};
        std::wstring errorMessage;
        bool operator==(const GitHubConnectedRepoState&) const noexcept = default;
    };
    struct PendingGitHubDocumentOpen
    {
        std::uint64_t requestId{};
        std::wstring owner;
        std::wstring repo;
        std::wstring branch;
        std::wstring repoPath;
        bool keepExistingPreferred{};
    };
    struct FilesRowContextBinding
    {
        enum class Kind : std::uint8_t
        {
            None,
            WorkspaceDoc,
            LocalRootHeader,
            LocalEntry,
            GitHubSectionHeader,
            GitHubRepoHeader,
            GitHubDir,
            GitHubFile
        };
        Kind kind{Kind::None};
        unsigned workspaceKey{};
        std::filesystem::path localRoot;
        std::filesystem::path localPath;
        bool isDirectory{};
        std::wstring githubOwner;
        std::wstring githubRepo;
        std::wstring githubBranch;
        std::wstring githubPath;
    };
    std::vector<GitHubConnectedRepoState> _connectedGitHubRepos;
    std::vector<GitHubRepoSummary> _githubUserRepoCache;
    std::wstring _githubSignedInLogin;
    std::wstring _githubDeviceCodePromptText;
    std::vector<PendingGitHubDocumentOpen> _pendingGitHubOpens;
    struct PendingFilesContextMenu
    {
        std::uint64_t id{};
        POINT screenPoint{};
        bool keyboardInvocation{};
    };
    std::optional<PendingFilesContextMenu> _deferredFilesContextMenu;
    bool _filesContextMenuOpen{};
    std::vector<std::pair<std::uint64_t, FilesRowContextBinding>> _filesRowBindings;
    FilesRowContextBinding _pendingContextTarget;
    std::vector<GitHubRepoSummary> _githubSuggestMatches;
    DxUi::Control* _brandMark{};
    DxUi::Tree *_outlineTree{}, *_filesTree{}, *_githubSuggestTree{};
    NavigationModel _githubSuggestModel;
    Surface *_filesPane{}, *_focusPill{}, *_modalBackdrop{};
    DxUi::Splitter* _filesSplitter{};
    DxUi::Label *_filesTitle{}, *_filesCount{}, *_currentBlock{}, *_positionStatus{}, *_findCount{}, *_drawerHelp{}, *_currentBlockTitle{},
        *_equationReadOnly{}, *_settingsTitle{}, *_appearanceTitle{}, *_writingTitle{}, *_syntaxTitle{}, *_syntaxIntro{}, *_privacyTitle{}, *_drawerImmediate{},
        *_aboutTitle{}, *_aboutVersion{}, *_aboutSummary{}, *_shortcutsList{}, *_proofingTitle{}, *_spellingHelp{}, *_spellingDictPathLabel{},
        *_strikeTasksHelp{}, *_localNetworkImagesHelp{}, *_imageFirstPasteHelp{}, *_spellingLanguageLabel{}, *_keepUnsavedHelp{}, *_accountsTitle{},
        *_accountsStatusLabel{};
    DxUi::ComboBox *_spellStatusCombo{}, *_spellingLanguageCombo{};
    DxUi::Toggle *_outlineToggle{}, *_focusToggle{}, *_dimToggle{}, *_strikeTasksToggle{}, *_keepUnsavedToggle{}, *_remoteImagesToggle{},
        *_localNetworkImagesToggle{}, *_imageFirstPasteToggle{}, *_spellingEnabledToggle{}, *_spellingIgnoreUpperToggle{}, *_spellingIgnoreDigitsToggle{},
        *_spellingIgnorePathsToggle{};
    DxUi::Button *_spellingDictChooseButton{}, *_spellingDictResetButton{}, *_filesAddFolder{}, *_filesAddGitHub{}, *_filesEmptyOpenFolder{},
        *_filesEmptyOpenGitHub{}, *_accountsDeviceBtn{}, *_accountsImportGhBtn{}, *_accountsUseTokenBtn{}, *_accountsSignOutBtn{};
    DxUi::TextField* _accountsTokenInput{};
    SpellService _spellService;
    CustomDictionary _customDictionary;
    struct SpellIssue
    {
        DxUi::RichText::MarkdownRange displayRange;
        std::vector<DxUi::RichText::MarkdownRange> sourceFragments;
        std::wstring word;
        std::wstring normalizedKey;
        std::uint8_t primaryLanguageIndex{};
    };
    struct PendingSpellMenu
    {
        unsigned workspaceKey{};
        DxUi::RichText::SourceSnapshot source;
        std::uint64_t revision{};
        DxUi::RichText::Selection selection;
        SpellIssue issue;
        std::vector<std::wstring> suggestions;
        std::vector<std::wstring> languages;
        std::weak_ptr<int> lifetime;
    };
    struct PendingLeftClickRequest
    {
        unsigned workspaceKey{};
        std::uint64_t revision{};
        std::size_t issueIndex{};
        POINT screenPt{};
    };
    struct SpellControllerState
    {
        std::vector<SpellIssue> issues;
        std::optional<SpellRequestKey> appliedKey;
        std::optional<DxUi::RichText::MarkdownRange> suppressedTypingWord;
        std::uint64_t languageConfigVersion = 1u;
        std::vector<std::wstring> installedLanguages;
        std::vector<std::wstring> activeLanguages{L"en-US"};
        ProviderAvailability effectiveAvailability = ProviderAvailability::Ready;
        ProviderDiagnostic primaryDiagnostic;
        bool typingSuppressionActive{};
        bool checkPending{};
        bool immediatePending{};
        bool syncingLanguageCombos{};
        POINT leftDownClientPt{};
        std::uint64_t leftDownRevision{};
        std::optional<std::size_t> leftDownHitOffset;
        std::optional<D2D1_RECT_F> leftDownIssueRect;
        bool leftDownArmed{};
        std::optional<PendingSpellMenu> pendingMenu;
        std::optional<PendingLeftClickRequest> pendingLeftClick;
        std::optional<std::wstring> cachedUserLocale;
        bool statusInitialized{};
    } _spell;
    std::array<DxUi::Toggle*, MarkdownSyntaxCount> _syntaxToggles{};
    std::array<DxUi::Label*, MarkdownSyntaxCount> _syntaxHelp{};
    std::array<DxUi::Separator*, 6> _commandDividers{};
    DirtyDot* _dirtyDot{};
    DxUi::Label *_filesSectionTitle{}, *_outlineCount{}, *_filesRootIcon{}, *_listSummaryIcon{};
    DxUi::Button* _blockSourceAction{};
    SubtleToggleButton *_pinFiles{}, *_pinOutline{};
    DxUi::Separator *_contextDivider{}, *_filesFooterDivider{}, *_settingsHeaderDivider{};
    DxUi::Label* _listSummary{};
    ReentrantPublication<std::pair<unsigned, std::size_t>> _listSummaryFacts;
    // Header measurement waits for the host's first typography preparation; cold layout reserves explicit bounds.
    struct IdentityMeasure
    {
        std::uint64_t textRevision{}, typographyRevision{};
        float availableWidth = -1.f, width{}, dpi{};
    } _identityMeasure;
    DxUi::TextField *_filesFilter{}, *_replaceText{};
    DxUi::Button *_filesToggle{}, *_panelToggle{}, *_settingsToggle{}, *_overflowButton{}, *_exitFocus{}, *_fileMenu{}, *_saveButton{}, *_minimizeButton{},
        *_maximizeButton{}, *_closeButton{}, *_recoveryAction{}, *_drawerClose{};
    DxUi::NumericStepper *_readingWidth{}, *_textScale{};
    DxUi::ScrollPanel* _settingsScroll{};
    float _settingsPrivacyScrollY{1340.f};
    float _settingsProofingScrollY{1620.f};
    float _settingsAboutScrollY{1660.f};
    DxUi::CardPanel* _selectionHintChip{};
    DxUi::Label* _selectionHint{};
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
            return ! projections.presentationCleared;
        }
        void InvalidateMatches() noexcept
        {
            table.source.reset();
        }
    };
    FindController _find;
    int _zoomWheelRemainder{};
    std::vector<std::filesystem::path> _visibleFilePaths;
    std::optional<std::size_t> _currentHeading;
    struct CurrentBlockLabelFacts
    {
        unsigned resourceId{};
        std::size_t lineCharacterCount{};
        bool operator==(const CurrentBlockLabelFacts&) const noexcept = default;
    };
    ReentrantPublication<CurrentBlockLabelFacts> _currentBlockPublishedFacts;
    std::optional<CurrentBlockLabelFacts> _currentBlockLatestFacts;
    ReentrantPublication<bool> _selectionCaptionPublishedEquationContext;
    bool _equationContext{}, _pendingSelectionTools{}, _postPreparePosted{}, _systemDark{};
    AsyncFiles _files;
    RichImageLoader _images;
    std::shared_ptr<Printing::Operation> _printOperation;
    std::unique_ptr<PrintPreview> _printPreview;
    std::optional<Printing::PrinterConfiguration> _printConfiguration;
    std::optional<Printing::PrinterSettings> _printSettings;
    std::wstring _printPrinter;
    DxUi::Control* _printInvoker{};
    std::weak_ptr<int> _printInvokerLifetime;
    std::wstring _printTitle;
    std::vector<Printing::Image> _printNetworkImages;
    void Print();
    void PreviewPrintPage(unsigned number);
    void PreviewPrintDestination(std::optional<Printing::PrinterSettings> requested = std::nullopt);
    void SubmitPrintPreview();
    void ClosePrintPreview() noexcept;
    void DismissPrintPreview() noexcept;
    void PrintCompleted();
    void ReportPrintFailure(HRESULT result, Printing::Stage stage, std::wstring_view printer = {}, unsigned pageNumber = 0u);
    void RememberPrintImage(const DxUi::RichText::RichImageRequest& request, std::shared_ptr<const DxUi::RichText::RichImage> pixels);
    FileChangeService _imageFiles;
    std::vector<std::filesystem::path> _watchedImagePaths;
    std::shared_ptr<const DxUi::RichText::SemanticDocument> _imageWatchSemantic;
    std::filesystem::path _imageWatchDocumentPath;
    bool _imageBindingTransition{};
    std::uint64_t _imageFileRefreshes{}, _imageWatchPathBuilds{};
    BitmapPasteWriter _bitmapWriter;
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
    std::optional<PendingBitmapPaste> _pendingBitmapPaste;
    std::vector<DxUi::RichText::RichImageRequest> _imageRequests;
    Editor* _editor{};
    Editor* _mirror{};
    CollapsedPaneRail* _leftRail{};
    CollapsedPaneRail* _rightRail{};
    Surface *_root{}, *_documentBar{}, *_commandBar{}, *_page{}, *_sidebar{}, *_floating{}, *_drawer{}, *_findBar{}, *_blockDraft{};
    DxUi::CardPanel *_splitComposeChipBg{}, *_splitSourceChipBg{};
    DxUi::Label *_title{}, *_stats{}, *_feedback{}, *_outlineTitle{}, *_equationHelp{}, *_modeHint{};
    DxUi::Label *_splitComposeTitle{}, *_splitSourceTitle{}, *_splitComposeChip{}, *_splitSourceChip{}, *_splitSwitchHint{};
    DxUi::Button* _splitScrollSyncButton{};
    PaneSide _lastPinnedSide{PaneSide::Left};
    SplitActivePane _splitActivePane = SplitActivePane::Compose;
    bool _leftAutoCollapsed          = false;
    bool _rightAutoCollapsed         = false;
    bool _splitScrollSyncInProgress  = false;
    struct PendingSplitScroll
    {
        Editor* source{};
        float scrollDip{};
    };
    std::optional<PendingSplitScroll> _pendingSplitScroll;
    std::optional<float> _pendingComposeScrollRestore;
    std::optional<float> _pendingSourceScrollRestore;
    D2D1_POINT_2F _lastPointerClientDip{-1.0f, -1.0f};
    bool _pointerInsideClient = false;
    DxUi::RichTextEditor* _equation{};
    DxUi::TextField* _findText{};
    DxUi::Button* _findCaseToggle{};
    DxUi::TextField* _blockDraftText{};
    Surface* _recoveryPanel{};
    DxUi::TextField* _recoveryPreview{};
    DxUi::Label* _recoveryDescription{};
    DxUi::Button* _restoreAllRecovery{};
    Surface *_saveConfirmPanel{}, *_renamePanel{}, *_fileConflictPanel{}, *_openGitHubPanel{};
    DxUi::Label *_saveConfirmTitle{}, *_saveConfirmHint{}, *_renameTitle{}, *_renameLocation{}, *_renameError{}, *_fileConflictTitle{}, *_fileConflictDesc{},
        *_openGitHubTitle{}, *_openGitHubAuthLabel{}, *_openGitHubSuggestLabel{}, *_openGitHubErrorLabel{};
    DxUi::ScrollPanel* _saveConfirmScroll{};
    DxUi::Checkbox* _saveConfirmSelectAll{};
    DxUi::Button *_saveConfirmPrimary{}, *_saveConfirmKeepExit{}, *_saveConfirmDiscard{}, *_saveConfirmCancel{}, *_renameConfirmBtn{}, *_conflictReplaceBtn{},
        *_conflictKeepBothBtn{}, *_openGitHubOpenBtn{}, *_openGitHubDeviceBtn{}, *_openGitHubImportGhBtn{}, *_openGitHubUseTokenBtn{};
    DxUi::TextField *_renameInput{}, *_openGitHubUrlInput{}, *_openGitHubBranchInput{}, *_openGitHubTokenInput{};
    std::vector<SaveConfirmRowControls> _saveConfirmRowControls;
    std::optional<SaveConfirmSession> _saveConfirmSession;
    std::optional<RenameFileSession> _renameSession;
    std::optional<FileConflictSession> _fileConflictSession;
    bool _syncingSaveConfirmChecks{};
    OverlayKind _activeOverlay{OverlayKind::None};
    std::optional<BlockDraftSession> _blockDraftSession;
    std::vector<DxUi::Button*> _tools, _floatingActions, _themeButtons;
    std::array<DxUi::Button*, 3> _modeButtons{};
    std::filesystem::path _folderRoot;
    Surface* _imageConsent{};
    DxUi::Label* _imageConsentText{};
    DxUi::Button *_imageConsentAction{}, *_imageReload{};
    std::shared_ptr<const DxUi::RichText::SemanticDocument> _imageConsentSource;
    std::filesystem::path _imageConsentPath;
    bool _imageConsentHasNetwork{};
    HRESULT _imageFailureStatus = S_OK;
    struct DocumentSessionMetadata
    {
        std::filesystem::path path;
        std::wstring savedText;
        FileStamp stamp;
        std::set<std::uint64_t> collapsedHeadings;
        std::optional<std::wstring> spellLanguageOverride;
        DocumentOriginInfo origin;
        unsigned key{};
        bool bom{};
        bool recovered{};
        bool resumedUnsaved{};
        bool remoteImagesAllowed{};
    };
    DocumentSessionMetadata _session;
    struct WorkspaceDocument
    {
        DxUi::RichText::Document state;
        DocumentSessionMetadata metadata;
        bool dirty{};
    };
    std::vector<WorkspaceDocument> _inactiveDocuments;
    static constexpr std::uint64_t WorkspaceGroupId     = std::uint64_t{1} << 63;
    static constexpr std::uint64_t LocalRootGroupBaseId = std::uint64_t{1} << 60;
    static constexpr std::uint64_t GitHubSectionGroupId = std::uint64_t{1} << 59;
    static constexpr std::uint64_t GitHubRowBaseId      = std::uint64_t{1} << 58;
    bool _workspaceCollapsed{}, _workspaceSelectionPreferred{}, _githubSectionCollapsed{}, _sessionResumeApplied{};
    std::vector<WorkspaceApproval> _workspaceApprovals;
    bool _switchingDocument{};
    HRESULT _sessionEndRecoveryStatus = S_OK;
    bool _lastFilesDirty{};
    HRESULT _folderAdmissionStatus = S_OK;
    struct FilesFooterMeasureCache
    {
        float widthDip = -1, heightDip = 52, dpi{};
        std::uint64_t textRevision{}, typographyRevision{}, measurements{};
    } _filesFooterMeasure;
    struct DerivedDocumentCaches
    {
        struct DirtyFact
        {
            DxUi::RichText::SourceSnapshot source;
            bool recovered{}, resumedUnsaved{}, dirty{};
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
            selectionFactsView      = EditorView::Compose;
            selectionFactsAnalysis  = 0;
            outlineRowsRevision     = 0;
        }
    };
    DerivedDocumentCaches _derivedCaches;
    struct OutlinePreviewBaseline
    {
        std::set<std::uint64_t> collapsedHeadings;
        std::vector<std::pair<unsigned, std::wstring>> committedOutline;
    };
    std::optional<OutlinePreviewBaseline> _outlinePreviewBaseline;
    struct SelectionToolsFocusRequest
    {
        DxUi::RichText::SourceSnapshot source;
        DxUi::RichText::Selection selection;
        EditorView view;
    };
    std::optional<SelectionToolsFocusRequest> _selectionToolsFocus;
    LifecyclePhase _lifecyclePhase{LifecyclePhase::Uninitialized};
    bool _sessionEndCommitted{};
    bool _reportingUiFailure{};
    ModalPromptKind _modalPrompt{ModalPromptKind::None};
    bool _visibleBeforeSaveClose{};
#if defined(REDTILDE_ENABLE_TESTS)
    std::uint64_t _preferencesReadySentDuringClose{};
#endif
    bool _windowPlacementDirty{};
    bool _settingsReloadDeferred{};
    unsigned _settingsReloadRetries{};
    StartupRestoreState _startupRestore;
    struct ExternalLinkState;
    struct ExternalLinkJob;
    std::shared_ptr<ExternalLinkState> _externalLink;
    DocumentInfo _documentInfo;
    DocumentLineIndexStorage _documentLineIndexStorage;
    ActiveFileTransaction _activeFileTx;
    static constexpr UINT_PTR SettingsRetryTimerId    = 0x52545352u;
    static constexpr UINT_PTR SpellDebounceTimerId    = 0x52545353u;
    static constexpr UINT_PTR GitHubDevicePollTimerId = 0x52545354u;
    std::uint64_t _nextFileOpRequestId{1u};
    std::uint64_t _nextGitHubRequestId{1u};
    std::uint64_t _filesMultiFolderGeneration{};
    std::optional<GitHubDeviceCodeInfo> _pendingDeviceFlow;
    static constexpr UINT ImageReady = WM_APP + 401, FileReady = WM_APP + 402, PreferencesReady = WM_APP + 403, RecoveryReadyMessage = WM_APP + 404,
                          FolderReady = WM_APP + 405, ExternalLinkReady = WM_APP + 406, BitmapReady = WM_APP + 407, ImageFileChanged = WM_APP + 408,
                          PrintReady = WM_APP + 409, PostPreparePlacement = WM_APP + 410, SettingsChanged = WM_APP + 411, SpellReady = WM_APP + 412,
                          SpellMenuRequested = WM_APP + 413, FileOpReady = WM_APP + 414, GitHubReady = WM_APP + 415, FilesContextMenuRequested = WM_APP + 416;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM) noexcept;
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);
    bool RouteShortcut(const MSG& message);
    [[nodiscard]] HRESULT BuildUi();
    void PrepareDesignScenario();
    void RefreshOutline();
    [[nodiscard]] bool PreviewOutlineMatchesCommitted() const;
    void RefreshFiles(bool rootChanged = false);
    void EnsureFilesRoot();
    void SelectFilesRoot(const std::filesystem::path& path);
    void AddFilesRoot(const std::filesystem::path& path);
    void RemoveFilesRoot(const std::filesystem::path& path);
    void OpenFolder();
    void AddFolder();
    void UpdateCurrentFileDirtyBadge(bool dirty);
    void FolderCompleted();
    void OnFilesContextMenu(std::uint64_t id, POINT screenPoint);
    void ShowFilesContextMenuNow(std::uint64_t id, POINT screenPoint, bool keyboardInvocation);
    void ExecuteFilesContextAction(Command command);
    void OpenPath(const std::filesystem::path& path);
    [[nodiscard]] bool OpenDroppedFile(const std::filesystem::path& path);
    void CloseActiveDocument();
    [[nodiscard]] bool SwitchWorkspaceDocument(std::size_t index, bool savePreviousRecovery = true, bool refreshFiles = true);
    [[nodiscard]] bool StartWorkspaceDocument();
    void ResetWorkspace(bool retireRecovery = true);
    void CheckpointWorkspaceRecovery(RecoverySnapshotKind kind = RecoverySnapshotKind::CrashRecovery);
    void AskWorkspaceThen(std::function<void()> continuation, std::vector<WorkspaceApproval> approved = {}, bool closingApplication = false);
    void ShowSaveConfirmModal(std::function<void()> continuation, bool closingApplication);
    void AdvanceSaveConfirmSequentialSave();
    void ShowRenameFileModal(const std::filesystem::path& path, bool isDirectory);
    void CommitRenameFileModal();
    void ShowFileConflictModal(std::vector<std::filesystem::path> sources,
                               std::filesystem::path targetDirectory,
                               bool move,
                               std::filesystem::path conflictName);
    void CommitFileConflictResolution(bool replaceExisting);
    void BeginCopyOrMoveToDirectory(std::vector<std::filesystem::path> sources,
                                    const std::filesystem::path& targetDirectory,
                                    bool move,
                                    bool forceOverwrite = false,
                                    bool forceNumbered  = false);
    void FileOpCompleted();
    void ShowOpenGitHubModal();
    void RefreshOpenGitHubSuggestTree();
    void RefreshGitHubAuthControls();
    void CommitOpenGitHubModal();
    void RemoveConnectedGitHubRepo(std::wstring_view owner, std::wstring_view repo);
    void OpenGitHubDocument(std::wstring_view owner, std::wstring_view repo, std::wstring_view branch, std::wstring_view path);
    void GitHubCompleted();
    void RefreshAccountsDrawerControls();
    void RebaseWorkspacePathsAfterFileOp(const std::filesystem::path& sourcePath, const std::filesystem::path& destPath, bool isDirectory, bool move);
    [[nodiscard]] std::filesystem::path ActivePasteTargetDirectory() const;
    [[nodiscard]] bool IsFilesRowTargetLocalEntry(std::uint64_t id) const;
    void LayoutOverlayPanels(float left, float right, float top, float bottom, float width, float height);
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
    [[nodiscard]] std::optional<std::size_t> CurrentLinkElement(const std::shared_ptr<const DxUi::RichText::SemanticDocument>& document) const;
    void RequestOpenLink(const DxUi::RichTextLinkActivation& activation);
    void ExternalLinkCompleted();
    void CloseExternalLinks() noexcept;
    static DWORD WINAPI ExternalLinkWorker(void* context) noexcept;
    [[nodiscard]] std::vector<DxUi::MenuFlyoutItem> ContextMenuItems(bool insertOnly, bool headingOnly);
    void ApplyTheme();
    void ApplyMarkdownSyntax();
    [[nodiscard]] const std::wstring& CurrentText() const;
    [[nodiscard]] bool SynchronizeMirror();
    void Changed();
    void RefreshDocumentAnalysis(const DxUi::RichText::SourceSnapshot& source, bool committed);
    void UpdateCounts();
    void UpdateCaption(std::wstring caption);
    void SelectionChanged(bool refreshTools = true);
    [[nodiscard]] bool PublishCurrentBlockLabelIfVisible(bool sidebarWillBeVisible = false);
    void FocusSelectionTools();
    void ClearStatus();
    void Status(unsigned id);
    void ReportCapacityLimit();
    void SavePreferences();
    void ReloadSettingsFromDisk();
    void ServiceImages();
    void RefreshImageConsent();
    std::optional<std::wstring> BeginBitmapPaste(std::span<const std::byte> dib, bool v5);
    void ServiceBitmapPaste();
    void FileCompleted() noexcept;
    void CompleteFileOperation();
    void AskThen(std::function<void()> continuation);
    void Open();
    void Save(bool saveAs = false, bool force = false, std::optional<std::filesystem::path> selectedPath = {});
    void ResolveSaveConflict(const AsyncFiles::Result& result);
    std::filesystem::path PickFile(bool saving, bool image = false, bool folder = false);
    void Menu(POINT point, bool insertOnly = false, bool headingOnly = false, bool keyboardInvocation = false);
    void InitializeSpelling();
    [[nodiscard]] std::filesystem::path ActiveCustomDictionaryPath() const;
    [[nodiscard]] std::vector<std::wstring> ResolveEffectiveSpellLanguages(const std::optional<std::wstring>& frontMatterHint) const;
    void ScheduleSpellCheck(bool immediate = false);
    void DispatchSpellCheckNow();
    void OnSpellDebounceTimer();
    void SpellCompleted() noexcept;
    void ProcessSpellResult();
    void OnSpellEditorChanged();
    void OnSpellSelectionChanged();
    void OnSpellScrollChanged();
    void ProjectVisibleSpellDecorations();
    void RefreshSpellStatusChip();
    void RefreshProofingDrawerControls();
    void RefreshSpellLanguageCombos(bool queryInstalled = false);
    void ApplySpellLanguageSelection(std::wstring_view value, bool fromStatusBar);
    void UpdateSpellSuppressionFromCaret();
    [[nodiscard]] std::optional<std::size_t> FindSpellIssueAtSourceOffset(std::size_t sourceOffset) const noexcept;
    [[nodiscard]] std::vector<DxUi::MenuFlyoutItem> BuildSpellActionMenuItems(std::size_t issueIndex, DxUi::RichText::Selection selection);
    void PopulateSpellContextMenu(POINT screenPoint, bool keyboardInvocation, std::vector<DxUi::MenuFlyoutItem>& items);
    [[nodiscard]] bool HandleSpellMenuSelection(
        int selectedId, unsigned workspaceKey, const DxUi::RichText::SourceSnapshot& source, std::uint64_t revision, DxUi::RichText::Selection selection);
    void OnEditorLeftButtonDown(POINT clientPt, WPARAM modifiers);
    void OnEditorLeftButtonDoubleClick() noexcept;
    void OnEditorLeftButtonUp(POINT clientPt);
    void HandlePostedSpellLeftClickMenu();
    void ShowSpellLeftClickMenu(std::size_t issueIndex, POINT screenPoint);
    void ExecuteSpellingCommand(Command command);
    [[nodiscard]] std::filesystem::path PickDictionaryFile();
    void LocalSource();
    void ApplyBlockSource();
    void FindNext(bool previous = false);
    void SetActiveOverlay(OverlayKind next);
    [[nodiscard]] bool ReconcilePostPreparePlacement();
    void CloseDrawers();
    [[nodiscard]] Editor* ActiveEditor() const noexcept;
    [[nodiscard]] Editor* MirrorEditor() const noexcept;
    void BindEditorCallbacks();
    void SwitchSplitActivePane(SplitActivePane target, bool moveFocus = false, std::optional<DxUi::RichText::Selection> overrideSelection = std::nullopt);
    void EnsurePrimaryEditorAuthoritative();
    void OnSplitPaneScrolled(Editor* sourcePane, float scrollDip);
    void ApplySplitScrollSync(Editor* sourcePane, float scrollDip, bool allowDefer = true);
    void UpdateSplitHeaders();
    void ApplySidePaneOverlayBounds();
    void DismissHoverOverlays(bool animated = true);
    [[nodiscard]] bool IsPaneOverlayActive(PaneSide side) const noexcept;
    void UpdateHoverOverlayPointer(D2D1_POINT_2F clientDip, bool insideWindow);
    void RefreshLeftRailDots();
    void RefreshRightRailDots();
    void OnRailDotActivated(PaneSide side, size_t itemIndex);
    void RevealPaneOverlay(PaneSide side);
    void BeginClose(bool preserveSessionResume = false);
    void ResumeAfterSaveClose();
    void RecoveryCompleted();
    void ShowRecovery();
    void FinishClose();
    DxUi::Button* Action(DxUi::Panel& parent, unsigned label, Command command, std::wstring tooltip = {});
    DxUi::Toggle* MakeSettingToggle(DxUi::Panel& parent, unsigned label, Command command, std::wstring_view tooltip = {});
    DxUi::Label* Label(DxUi::Panel& parent, unsigned label, DxUi::FontRole role = DxUi::FontRole::Body);
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
int RunCoreTests(const std::filesystem::path& root, CoreTestScope scope = CoreTestScope::All, bool listOnly = false);
#if defined(REDTILDE_ENABLE_TESTS)
int RunLinkedSaveInterruptionProbe(const std::filesystem::path& root, bool restart);
#endif
int RunPerformanceProbe(Application& app, const LaunchOptions& options);
int RunDesignInteractiveProbe(Application& app, const LaunchOptions& options);
} // namespace RedTilde
