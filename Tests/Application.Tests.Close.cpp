#include "Application.Tests.Close.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include "res/Resource.h"
#include <chrono>
#include <format>
#include <stdexcept>
#include <thread>

namespace RedTilde
{
namespace
{
void Require(bool condition, std::string_view message)
{
    if (!condition)
        throw std::runtime_error(std::string(message));
}
void Success(HRESULT result, std::string_view message)
{
    Require(SUCCEEDED(result), message);
}
bool HasLabelPrefix(const DxUi::Control *control, std::wstring_view prefix)
{
    if (!control)
        return false;
    if (const auto *label = dynamic_cast<const DxUi::Label *>(control); label && label->GetText().starts_with(prefix))
        return true;
    for (std::size_t index = 0; index < control->GetLogicalChildCount(); ++index)
        if (HasLabelPrefix(control->GetLogicalChild(index), prefix))
            return true;
    return false;
}
bool HasSaveFailure(const DxUi::Control *control)
{
    const auto format = Resource(IDS_SAVEFAILED);
    return HasLabelPrefix(control, std::wstring_view(format).substr(0, format.find(L'{')));
}
void Pump(Application &app)
{
    app.Prepare(1);
    // Prepare deliberately stops after host retirement. Shutdown notifications
    // still belong to this exact owned HWND; never pump another app's messages.
    if (const auto window = app.Window())
    {
        MSG message{};
        unsigned count{};
        while (count++ < 64 && app.Window() == window && PeekMessageW(&message, window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
void Await(Application &app, const std::function<bool()> &ready, std::string_view message)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do
    {
        Pump(app);
        if (ready())
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);
    Require(false, message);
}
std::vector<std::filesystem::path> Copies(const std::filesystem::path &directory)
{
    std::vector<std::filesystem::path> result;
    for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
        if (entry.path().extension() == L".rtd")
            result.push_back(entry.path());
    return result;
}
void Checkpoint(Application &app)
{
    app.CheckpointRecovery(true);
    Await(app, [&] { return app.RecoveryReady() && app.RecoverySettled(); }, "checkpoint settles before the admitted file command");
    Success(app.RecoveryStatus(), "checkpoint is durable");
}
auto ReleaseOnExit(const std::shared_ptr<AsyncFiles::TestGate> &gate)
{
    Require(gate->entered && gate->proceed && gate->finished, "create deterministic file admission events");
    return wil::scope_exit(
        [gate]
        {
            (void)SetEvent(gate->proceed.get());
            if (WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0)
                (void)WaitForSingleObject(gate->finished.get(), 10000);
        });
}
void Entered(const std::shared_ptr<AsyncFiles::TestGate> &gate)
{
    Require(WaitForSingleObject(gate->entered.get(), 10000) == WAIT_OBJECT_0, "real file worker reaches its admission barrier");
}
void Release(const std::shared_ptr<AsyncFiles::TestGate> &gate)
{
    Require(SetEvent(gate->proceed.get()) != FALSE && WaitForSingleObject(gate->finished.get(), 10000) == WAIT_OBJECT_0,
            "real filesystem operation finishes and publishes after release");
}
void Seed(Application &app, const std::filesystem::path &path)
{
    // Fixture paths are owned here; recovery creates its own directories asynchronously.
    std::filesystem::create_directories(path.parent_path());
    Success(WriteBytesAtomic(path, "Notebook\r\n"), "seed original Markdown bytes");
    StoredDocument document;
    Success(LoadDocument(path, document), "read original identity");
    app.SetDocument(document.text, path, document.bom, document.stamp);
}
void Edit(Application &app, std::wstring text)
{
    const auto end = app.Document().GetText().size();
    app.Document().SetSelection({end, end});
    Require(app.Document().ReplaceSelection(std::move(text)), "admit a real source edit");
}
} // namespace
void RegisterCloseTests(const std::filesystem::path &root, const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test("Editor.OpenErrorsExplainEncodingAndCapacity",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"specific-open-errors";
             const auto input = launch.dataDirectory / L"unsupported.md";
             launch.filePickerForTest = [&](bool saving) { return saving ? std::filesystem::path{} : input; };
             Application app(launch);
             Success(app.Initialize(), "create hidden specific-open-error app");
             Seed(app, launch.dataDirectory / L"retained.md");
             const auto source = app.Document().Snapshot();
             Success(WriteBytesAtomic(input, std::string(65537u, 'x')), "seed known oversized UTF-8 source");
             app.Execute(Command::Open);
             std::size_t units = 65537u, limit = 65536u;
             const auto tooLarge = std::vformat(Resource(IDS_FILETOOLARGECOUNT), std::make_wformat_args(units, limit));
             Await(app, [&] { return HasLabelPrefix(app.Host().GetRoot(), tooLarge); }, "Open reports the actual size and supported limit");
             Require(app.Document().Snapshot().IsSameRevision(source) && !app.Dirty(), "oversized input retains the current clean source");
             Success(WriteBytesAtomic(input, "\xFF\xFEtext"), "seed unsupported UTF-16 source");
             app.Execute(Command::Open);
             Await(app, [&] { return HasLabelPrefix(app.Host().GetRoot(), Resource(IDS_FILENOTUTF8)); }, "Open explains the strict UTF-8 policy");
             Require(app.Document().Snapshot().IsSameRevision(source) && !app.Dirty(), "invalid encoding retains source and saved baseline");
         });
    test("Editor.SaveAsSameTargetStillChecksSavedBaseline",
         [&]
         {
             for (const bool alias : {false, true})
             {
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.dataDirectory = root / (alias ? L"save-as-current-alias" : L"save-as-current-path");
                 const auto target = launch.dataDirectory / L"notebook.md", linked = launch.dataDirectory / L"alias.md";
                 unsigned questions{}, pickers{};
                 launch.filePickerForTest = [&](bool saving)
                 {
                     ++pickers;
                     return saving ? alias ? linked : target : std::filesystem::path{};
                 };
                 launch.saveConflictForTest = [&]
                 {
                     ++questions;
                     return SaveConflictChoice::Cancel;
                 };
                 Application app(launch);
                 Success(app.Initialize(), "create hidden same-target Save As app");
                 Seed(app, target);
                 Edit(app, L"My writing\n");
                 const auto source = app.Document().Snapshot();
                 Success(WriteBytesAtomic(target, "External version\n"), "replace original before same-target Save As");
                 if (alias)
                     Require(CreateHardLinkW(linked.c_str(), target.c_str(), nullptr), "create owned alias of current external target");
                 app.Execute(Command::SaveAs);
                 Await(app, [&] { return questions == 1; }, "Save As checks a current target or alias after the picker");
                 StoredDocument actual;
                 Success(LoadDocument(target, actual), "read target after same-target Save As refusal");
                 Require(pickers == 1 && questions == 1 && actual.text == L"External version\n" && app.Dirty() &&
                             app.Document().Snapshot().IsSameRevision(source) && app.Document().CanUndo(),
                         "picker selection of the current target cannot bypass its saved-baseline conflict protection");
             }
         });
    test("Editor.SaveConflictChoicesAreSingleAndSourceBound",
         [&]
         {
             for (const auto choice :
                  {SaveConflictChoice::Cancel, SaveConflictChoice::Reload, SaveConflictChoice::SaveAs, SaveConflictChoice::Overwrite})
             {
                 Testing::ForegroundOwnershipObserver foreground;
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.dataDirectory = root / (L"conflict-choice-" + std::to_wstring(static_cast<unsigned>(choice)));
                 const auto target = launch.dataDirectory / L"notebook.md", copy = launch.dataDirectory / L"separate-copy.md";
                 Application *active{};
                 unsigned questions{}, pickers{}, dirtyQuestions{};
                 bool duplicateCloseKept{};
                 launch.unsavedPromptForTest = [&]
                 {
                     ++dirtyQuestions;
                     return IDCANCEL;
                 };
                 launch.saveConflictForTest = [&]
                 {
                     ++questions;
                     const auto window = active->Window();
                     (void)SendMessageW(window, WM_CLOSE, 0, 0);
                     active->Execute(Command::Close);
                     duplicateCloseKept = active->Window() == window && active->Host().GetRoot();
                     return choice;
                 };
                 launch.filePickerForTest = [&](bool saving)
                 {
                     ++pickers;
                     return saving ? copy : std::filesystem::path{};
                 };
                 Application app(launch);
                 active = &app;
                 Success(app.Initialize(), "create hidden conflict-choice application");
                 Seed(app, target);
                 Edit(app, L"My retained writing\r\n");
                 const auto source = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 const auto mine = app.Document().GetText();
                 Success(WriteBytesAtomic(target, "External writing\r\n"), "create a real external conflict");
                 app.Execute(Command::Save);
                 Await(
                     app, [&] { return questions == 1 && (choice == SaveConflictChoice::Cancel || !app.Dirty()); },
                     "one conflict choice completes its selected action");
                 Require(duplicateCloseKept && dirtyQuestions == 0 && app.Window() && questions == 1 && !HasSaveFailure(app.Host().GetRoot()),
                         "duplicate Close keeps the existing conflict prompt and Cancel has no second error surface");
                 StoredDocument actual;
                 Success(LoadDocument(target, actual), "read conflict destination");
                 if (choice == SaveConflictChoice::Reload)
                     Require(app.Document().GetText() == L"External writing\r\n" && !app.Dirty() && !app.Document().CanUndo(),
                             "explicit Reload adopts disk source and establishes a clean baseline");
                 else
                 {
                     Require(app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection &&
                                 app.Document().CanUndo(),
                             "Cancel and both save choices retain exact editor source, selection and history");
                     Require(app.Dirty() == (choice == SaveConflictChoice::Cancel), "only a successful selected save clears dirty state");
                 }
                 Require(actual.text == (choice == SaveConflictChoice::Overwrite ? mine : L"External writing\r\n"),
                         "only an explicit Overwrite changes the conflicting destination");
                 Require(pickers == (choice == SaveConflictChoice::SaveAs ? 1u : 0u), "Save As alone invokes the picker once");
                 if (choice == SaveConflictChoice::SaveAs)
                 {
                     StoredDocument separate;
                     Success(LoadDocument(copy, separate), "read separately saved copy");
                     Require(separate.text == mine, "Save As publishes the selected source separately");
                 }
                 if (choice == SaveConflictChoice::SaveAs || choice == SaveConflictChoice::Overwrite)
                 {
                     Edit(app, L"Next paragraph\r\n");
                     const auto next = app.Document().GetText();
                     app.Execute(Command::Save);
                     Await(app, [&] { return !app.Dirty(); }, "the next save accepts the newly published baseline");
                     StoredDocument saved;
                     Success(LoadDocument(choice == SaveConflictChoice::SaveAs ? copy : target, saved), "read subsequent save");
                     Require(saved.text == next && questions == 1, "successful conflict resolution does not poison the next save identity");
                 }
                 Require(foreground.NeverAcquiredForeground(), "all conflict command checks preserve foreground");
             }
         });
    test("Editor.StaleConflictChoicePreservesLaterEdits",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"conflict-stale-choice";
             Application *active{};
             unsigned questions{};
             bool edited{};
             launch.saveConflictForTest = [&]
             {
                 ++questions;
                 const auto end = active->Document().GetText().size();
                 active->Document().SetSelection({end, end});
                 edited = active->Document().ReplaceSelection(L"Later edit\n");
                 return SaveConflictChoice::Overwrite;
             };
             Application app(launch);
             active = &app;
             Success(app.Initialize(), "create hidden stale conflict app");
             const auto target = launch.dataDirectory / L"notebook.md";
             Seed(app, target);
             Edit(app, L"First edit\n");
             Success(WriteBytesAtomic(target, "External source\n"), "create stale-choice conflict");
             app.Execute(Command::Save);
             Await(app, [&] { return questions == 1; }, "collect the conflict choice after its nested edit");
             StoredDocument actual;
             Success(LoadDocument(target, actual), "read source after stale overwrite choice");
             Require(edited && app.Dirty() && app.Document().GetText() == L"Notebook\r\nFirst edit\nLater edit\n" &&
                         actual.text == L"External source\n",
                     "a choice for an older immutable source cannot overwrite or discard newer edits");
             Require(app.Document().Undo() && app.Document().GetText() == L"Notebook\r\nFirst edit\n", "later edit retains its own Undo transaction");
         });
    test("Editor.SavePickerCannotRetargetNewerSource",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"save-picker-stale";
             const auto copy = launch.dataDirectory / L"unadmitted-copy.md";
             Application *active{};
             bool edited{};
             launch.filePickerForTest = [&](bool)
             {
                 const auto end = active->Document().GetText().size();
                 active->Document().SetSelection({end, end});
                 edited = active->Document().ReplaceSelection(L"Later picker edit\n");
                 return copy;
             };
             Application app(launch);
             active = &app;
             Success(app.Initialize(), "create hidden stale picker app");
             Seed(app, launch.dataDirectory / L"notebook.md");
             Edit(app, L"Before picker\n");
             app.Execute(Command::SaveAs);
             Require(edited && app.Dirty() && !std::filesystem::exists(copy) &&
                         app.Document().GetText() == L"Notebook\r\nBefore picker\nLater picker edit\n",
                     "a picker opened for older source never silently saves or retargets a newer source");
         });
    test("Editor.ConflictReloadPreservesEditsMadeDuringRead",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"conflict-reload-later-edit";
             const auto gate = launch.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
             unsigned questions{};
             bool reset{};
             launch.saveConflictForTest = [&]
             {
                 ++questions;
                 reset = ResetEvent(gate->entered.get()) && ResetEvent(gate->proceed.get());
                 return SaveConflictChoice::Reload;
             };
             Application app(launch);
             Success(app.Initialize(), "create hidden delayed conflict reload");
             const auto release = ReleaseOnExit(gate);
             const auto target = launch.dataDirectory / L"notebook.md";
             Seed(app, target);
             Edit(app, L"Before reload\n");
             Success(WriteBytesAtomic(target, "External reload source\n"), "seed conflict before delayed reload");
             app.Execute(Command::Save);
             Entered(gate);
             Release(gate);
             Await(
                 app, [&] { return questions == 1 && WaitForSingleObject(gate->entered.get(), 0) == WAIT_OBJECT_0; },
                 "Reload admits its own blocked reader");
             Require(reset, "pause only the owned Reload reader");
             Edit(app, L"During reload\n");
             const auto source = app.Document().Snapshot();
             Release(gate);
             Await(
                 app, [&] { return HasLabelPrefix(app.Host().GetRoot(), Resource(IDS_FILECONTEXTCHANGED)); },
                 "the actual Reload completion is consumed and refused as stale");
             Require(app.Document().Snapshot().IsSameRevision(source) && app.Dirty() &&
                         app.Document().GetText() == L"Notebook\r\nBefore reload\nDuring reload\n",
                     "a late Reload result cannot replace newer source");
             StoredDocument actual;
             Success(LoadDocument(target, actual), "read after refused stale reload");
             Require(actual.text == L"External reload source\n", "stale reload never writes the original");
         });
    test("Editor.ConflictReloadFailureRetainsSourceAndHistory",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"conflict-reload-invalid";
             const auto target = launch.dataDirectory / L"notebook.md";
             unsigned questions{};
             HRESULT staged = E_UNEXPECTED;
             launch.saveConflictForTest = [&]
             {
                 ++questions;
                 staged = WriteBytesAtomic(target, "\xF0\x28\x8C\x28");
                 return SaveConflictChoice::Reload;
             };
             Application app(launch);
             Success(app.Initialize(), "create hidden failed conflict reload");
             Seed(app, target);
             Edit(app, L"Retained after failed reload\n");
             const auto source = app.Document().Snapshot();
             const auto selection = app.Document().GetSelection();
             Success(WriteBytesAtomic(target, "External source\n"), "create reload conflict");
             app.Execute(Command::Save);
             Await(
                 app, [&] { return HasLabelPrefix(app.Host().GetRoot(), Resource(IDS_FILENOTUTF8)); },
                 "invalid Reload input explains its real encoding failure");
             Require(questions == 1 && SUCCEEDED(staged) && app.Dirty() && app.Document().Snapshot().IsSameRevision(source) &&
                         app.Document().GetSelection() == selection && app.Document().CanUndo(),
                     "failed Reload retains source, selection, history and dirty protection without another conflict question");
             std::string bytes;
             Success(ReadBytes(target, bytes), "read invalid original after failed reload");
             Require(bytes == "\xF0\x28\x8C\x28", "failed Reload never writes or repairs the original");
         });
    test("Recovery.CloseDuringSaveWaitsForDurableSuccess",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"close-save-success";
             const auto gate = launch.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
             Application app(launch);
             Success(app.Initialize(), "create hidden save-close app");
             const auto release = ReleaseOnExit(gate);
             const auto target = launch.dataDirectory / L"notebook.md";
             Seed(app, target);
             Edit(app, L"Saved continuation\r\n");
             const auto saved = app.Document().GetText();
             Checkpoint(app);
             Require(Copies(launch.dataDirectory).size() == 1, "dirty source has a durable recovery copy");
             app.Execute(Command::Save);
             Entered(gate);
             const auto window = app.Window();
             app.Host().SetFocusControl(&app.Document(), false);
             Require(app.Document().HasFocus(), "seed logical editor focus without acquiring desktop focus");
             const auto pendingRevision = app.Document().Snapshot();
             (void)SendMessageW(window, WM_CLOSE, 0, 0);
             app.Execute(Command::Close);
             app.Execute(Command::New);
             (void)SendMessageW(window, WM_CHAR, L'X', 0);
             Require(app.Document().Snapshot().IsSameRevision(pendingRevision), "queued commands and text cannot edit while Close waits");
             (void)SendMessageW(window, WM_SHOWWINDOW, FALSE, 0);
             (void)SendMessageW(window, WM_KILLFOCUS, 0, 0);
             Require(!app.Document().HasFocus(), "waiting Close still delivers hide and focus-loss lifecycle to the native host");
             Pump(app);
             Require(app.Window() == window && app.Host().GetHwnd() == window && !IsWindowVisible(window) && Copies(launch.dataDirectory).size() == 1,
                     "duplicate Close preserves the hidden notification owner and recovery before the save finishes");
             Release(gate);
             Await(app, [&] { return app.Window() == nullptr; }, "Close must finish after the admitted save succeeds without another Close");
             StoredDocument actual;
             Success(LoadDocument(target, actual), "read saved target after asynchronous shutdown");
             Require(actual.text == saved && Copies(launch.dataDirectory).empty(), "successful close saves the admitted bytes before retirement");
             Require(foreground.NeverAcquiredForeground(), "hidden save-close success preserves foreground");
         });
    test("Recovery.CloseDuringSaveFailurePreservesSourceAndRecovery",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"close-save-failure";
             const auto gate = launch.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
             Application app(launch);
             Success(app.Initialize(), "create hidden failed-save-close app");
             const auto release = ReleaseOnExit(gate);
             const auto target = launch.dataDirectory / L"notebook.md";
             Seed(app, target);
             Edit(app, L"Retained edit\r\n");
             const auto revision = app.Document().Snapshot();
             const auto selection = app.Document().GetSelection();
             Checkpoint(app);
             app.Execute(Command::Save);
             Entered(gate);
             wil::unique_hfile denyWrite(CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
             Require(denyWrite.is_valid(), "hold an owned target against replacement");
             (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
             Release(gate);
             Pump(app);
             Await(
                 app, [&] { return app.RecoverySettled() && HasSaveFailure(app.Host().GetRoot()); },
                 "failed save publishes its error and leaves recovery settled");
             Require(app.Window() && app.Host().GetHwnd() == app.Window() && app.Dirty() && app.Document().Snapshot().IsSameRevision(revision) &&
                         app.Document().GetSelection() == selection && app.Document().CanUndo(),
                     "failed save keeps the exact source, selection, history and live host");
             Require(Copies(launch.dataDirectory).size() == 1, "failed close retains durable recovery");
             std::string envelope;
             Success(ReadBytes(Copies(launch.dataDirectory).front(), envelope), "read retained failed-save recovery");
             RecoverySnapshot snapshot;
             Success(ParseRecovery(envelope, snapshot), "validate retained failed-save envelope");
             Require(snapshot.text == app.Document().GetText() && snapshot.selection == selection && snapshot.originalPath == target,
                     "failed save keeps the exact edited source, selection and original identity recoverable");
             denyWrite.reset();
             StoredDocument actual;
             Success(LoadDocument(target, actual), "read original after failed close");
             Require(actual.text == L"Notebook\r\n", "failed save never replaces the original target");
             // A new Save remains available and must not inherit the cancelled Close.
             app.Execute(Command::Save);
             Await(app, [&] { return !app.Dirty(); }, "editing remains usable and an explicit retry can succeed");
             Require(app.Window() && app.Host().GetHwnd(), "a later Save does not silently retry the abandoned Close");
             Require(foreground.NeverAcquiredForeground() && !IsWindowVisible(app.Window()), "headless failure coverage preserves desktop state");
         });
    test("Recovery.CloseDuringSaveRechecksLaterEdits",
         [&]
         {
             unsigned prompts{};
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"close-save-later-edit";
             launch.unsavedPromptForTest = [&]
             {
                 ++prompts;
                 return IDCANCEL;
             };
             const auto gate = launch.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
             Application app(launch);
             Success(app.Initialize(), "create hidden later-edit close app");
             const auto release = ReleaseOnExit(gate);
             const auto target = launch.dataDirectory / L"notebook.md";
             Seed(app, target);
             Edit(app, L"Admitted\r\n");
             const auto admitted = app.Document().GetText();
             app.Execute(Command::Save);
             Entered(gate);
             Edit(app, L"Later edit\r\n");
             const auto revision = app.Document().Snapshot();
             Checkpoint(app);
             (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
             Release(gate);
             Await(app, [&] { return prompts == 1; }, "successful admitted save must protect edits made after admission");
             Require(app.Window() && app.Host().GetHwnd() && app.Dirty() && app.Document().Snapshot().IsSameRevision(revision),
                     "Cancel retains the later source revision and owner");
             StoredDocument actual;
             Success(LoadDocument(target, actual), "read admitted save with later unsaved edits");
             Require(actual.text == admitted && Copies(launch.dataDirectory).size() == 1,
                     "only the admitted text reaches the original and later text stays recoverable");
         });
    auto closeOpen = [&](bool editDuringOpen)
    {
        Testing::ForegroundOwnershipObserver foreground;
        unsigned prompts{};
        int answer = IDCANCEL;
        LaunchOptions launch;
        launch.headless = true;
        launch.dataDirectory = root / (editDuringOpen ? L"close-open-edited" : L"close-open-clean");
        const auto target = launch.dataDirectory / L"other.md";
        launch.filePickerForTest = [&](bool saving)
        {
            Require(!saving, "this fixture opens through the ordinary picker boundary");
            return target;
        };
        launch.unsavedPromptForTest = [&]
        {
            ++prompts;
            return answer;
        };
        const auto gate = launch.fileGateForTest = std::make_shared<AsyncFiles::TestGate>();
        Application app(launch);
        Success(app.Initialize(), "create hidden open-close app");
        const auto release = ReleaseOnExit(gate);
        std::filesystem::create_directories(target.parent_path());
        Success(WriteBytesAtomic(target, "Different notebook\n"), "seed pending Open");
        app.SetDocument(L"Current notebook\n");
        app.Execute(Command::Open);
        Entered(gate);
        if (editDuringOpen)
            Edit(app, L"Retained edit\n");
        const auto revision = app.Document().Snapshot();
        (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
        if (editDuringOpen)
            Require(prompts == 1 && app.Window() && app.Dirty(), "Close abandons Open before asking about the new edits");
        else
            Require(prompts == 0 && app.Host().GetHwnd() == nullptr, "plain Open is abandoned without a busy-close prompt or a filesystem join");
        Release(gate);
        if (editDuringOpen)
        {
            Pump(app);
            Require(app.Document().Snapshot().IsSameRevision(revision) && app.Document().CanUndo(),
                    "abandoned Open cannot publish into the retained source after Cancel");
            answer = IDNO;
            (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
            Require(prompts == 2, "a fresh Close can discard only after another explicit choice");
        }
        Await(app, [&] { return app.Window() == nullptr; }, "Open abandonment settles ordinary asynchronous shutdown");
        Require(foreground.NeverAcquiredForeground(), "hidden Open abandonment preserves desktop foreground");
    };
    test("Recovery.CloseAbandonsPlainOpenWithoutPrompt", [&] { closeOpen(false); });
    test("Recovery.CloseAbandonsOpenAndProtectsLaterEdits", [&] { closeOpen(true); });
}
} // namespace RedTilde
