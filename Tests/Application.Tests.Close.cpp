#include "Application.Tests.Close.h"
#include "RedTilde/Application.h"
#include "res/Resource.h"
#include <chrono>
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
bool HasSaveFailure(const DxUi::Control *control)
{
    if (!control)
        return false;
    const auto format = Resource(IDS_SAVEFAILED);
    const auto prefix = format.substr(0, format.find(L'{'));
    if (const auto *label = dynamic_cast<const DxUi::Label *>(control); label && label->GetText().starts_with(prefix))
        return true;
    for (std::size_t index = 0; index < control->GetLogicalChildCount(); ++index)
        if (HasSaveFailure(control->GetLogicalChild(index)))
            return true;
    return false;
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
    test("Recovery.CloseDuringSaveWaitsForDurableSuccess",
         [&]
         {
             const auto foreground = GetForegroundWindow();
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
             Require(GetForegroundWindow() == foreground, "hidden save-close success preserves foreground");
         });
    test("Recovery.CloseDuringSaveFailurePreservesSourceAndRecovery",
         [&]
         {
             const auto foreground = GetForegroundWindow();
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
             Require(GetForegroundWindow() == foreground && !IsWindowVisible(app.Window()), "headless failure coverage preserves desktop state");
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
        const auto foreground = GetForegroundWindow();
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
        Require(GetForegroundWindow() == foreground, "hidden Open abandonment preserves desktop foreground");
    };
    test("Recovery.CloseAbandonsPlainOpenWithoutPrompt", [&] { closeOpen(false); });
    test("Recovery.CloseAbandonsOpenAndProtectsLaterEdits", [&] { closeOpen(true); });
}
} // namespace RedTilde
