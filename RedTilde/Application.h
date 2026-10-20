#pragma once
#include "AsyncFiles.h"
#include "Common/DocumentInfo.h"
#include "Common/Recovery.h"
#include "Common/Settings.h"
#include "ImageLoader.h"
#include "UiControls.h"
#include <array>
#include <filesystem>
#include <functional>
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
    LaterRecovery
};
struct LaunchOptions
{
    bool headless{}, selfTest{}, crashProbe{}, preferencesProbe{}, recoveryProbe{}, recoveryRestartProbe{};
    std::wstring scenario = L"compose-light";
    std::filesystem::path document, capture, dataDirectory, benchmark;
};
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
    bool Capture(const std::filesystem::path &path);
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

  private:
    LaunchOptions options;
    Settings settings;
    SettingsWriter preferenceWriter;
    RecoveryService recovery;
    std::vector<RecoveryCandidate> recoveryCandidates;
    bool recoveryOffered{}, recovered{};
    std::uint64_t recoveryGeneration{};
    wil::unique_hwnd window;
    DxUi::ControlHost host;
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
    std::vector<DxUi::Button *> topActions, tools, floatingActions, outlineButtons, themeButtons;
    std::array<DxUi::Button *, 3> modeButtons{};
    DxUi::Button *outlineToggle{}, *focusToggle{};
    std::filesystem::path documentPath;
    std::wstring savedText;
    bool bom{}, closing{}, initialized{}, initializing{};
    bool waitingForPreferences{};
    FileStamp stamp;
    DocumentInfo documentInfo;
    std::function<void()> afterSave;
    std::optional<DxUi::RichText::SourceSnapshot> openingSnapshot;
    static constexpr UINT ImageReady = WM_APP + 401, FileReady = WM_APP + 402, PreferencesReady = WM_APP + 403, RecoveryReadyMessage = WM_APP + 404;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM) noexcept;
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);
    void BuildUi();
    void Layout();
    void ApplyTheme();
    void Changed();
    void SelectionChanged();
    void Status(unsigned id);
    void SavePreferences();
    void ServiceImages();
    void FileCompleted();
    void AskThen(std::function<void()> continuation);
    void Open();
    void Save(bool saveAs = false, bool force = false);
    std::filesystem::path PickFile(bool saving, bool image = false);
    void Menu(POINT point, bool insertOnly = false, bool headingOnly = false);
    void LocalSource();
    void ApplyBlockSource();
    void FindNext();
    void CloseDrawers();
    void BeginClose();
    void RecoveryCompleted();
    void ShowRecovery();
    void FinishClose();
    DxUi::Button *Action(DxUi::Panel &parent, unsigned label, Command command, std::wstring tooltip = {});
    DxUi::Label *Label(DxUi::Panel &parent, unsigned label, DxUi::FontRole role = DxUi::FontRole::Body);
};
[[nodiscard]] std::wstring Resource(unsigned id);
[[nodiscard]] std::wstring SampleDocument();
int RunCoreTests(const std::filesystem::path &root);
} // namespace RedTilde
