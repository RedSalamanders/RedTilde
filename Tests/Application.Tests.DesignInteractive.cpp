#include "Application.Tests.DesignInteractive.h"
#include "RedTilde/Application.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <string>
#include <tlhelp32.h>
#include <vector>

namespace RedTilde
{
namespace
{
void Require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
std::string JsonString(std::string_view value)
{
    std::string result = "\"";
    for (const char unit : value)
    {
        const auto c = static_cast<unsigned char>(unit);
        if (c == '\\' || c == '"')
        {
            result += '\\';
            result += static_cast<char>(c);
        }
        else if (c < 0x20)
            result += std::format("\\u{:04x}", c);
        else
            result += static_cast<char>(c);
    }
    result += '"';
    return result;
}
DWORD ParentProcess() noexcept
{
    wil::unique_handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot || snapshot.get() == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32W process{sizeof(process)};
    if (Process32FirstW(snapshot.get(), &process))
        do
        {
            if (process.th32ProcessID == GetCurrentProcessId())
                return process.th32ParentProcessID;
        } while (Process32NextW(snapshot.get(), &process));
    return 0;
}
bool CanonicalLeaseOwnsParent() noexcept
{
    // Reuse the canonical executable/protocol, not a consumer-local warning or
    // restoration implementation. A manual child launch must remain harmless.
    const DWORD parent = ParentProcess();
    const HWND warning = FindWindowW(L"DxUi.InteractiveLease.Warning", nullptr);
    DWORD warningProcess{};
    if (!parent || !warning || !IsWindowVisible(warning) || !GetWindowThreadProcessId(warning, &warningProcess) || warningProcess != parent)
        return false;
    wil::unique_handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parent));
    std::array<wchar_t, 32768> name{};
    DWORD length = static_cast<DWORD>(name.size());
    if (!process || !QueryFullProcessImageNameW(process.get(), 0, name.data(), &length))
        return false;
    const std::wstring_view path(name.data(), length);
    const auto separator = path.find_last_of(L"\\/");
    const auto executable = separator == path.npos ? path : path.substr(separator + 1);
    constexpr std::wstring_view expected = L"DxUi.InteractiveLease.exe";
    if (CompareStringOrdinal(executable.data(), static_cast<int>(executable.size()), expected.data(), static_cast<int>(expected.size()), TRUE) !=
        CSTR_EQUAL)
        return false;
    wil::unique_handle mutex(OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, L"Local\\DxUi.InteractiveTestRun.v1"));
    if (!mutex)
        return false;
    const DWORD wait = WaitForSingleObject(mutex.get(), 0);
    if (wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED)
    {
        // Ownership was unexpectedly available; release that acquired mutex and
        // refuse before showing or activating any consumer window.
        const auto release = wil::scope_exit([&] { (void)ReleaseMutex(mutex.get()); });
        return false;
    }
    return wait == WAIT_TIMEOUT;
}
} // namespace

// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4626 5027)
class DesignInteractiveProbe final
{
    struct Outcome
    {
        std::string name;
        bool passed{};
        std::string error;
    };
    Application &app;
    const LaunchOptions &options;
    std::vector<Outcome> outcomes;
    std::array<bool, 256> attemptedKeys{};
    std::wstring baseline;
    bool leaseGuarded{}, shuttingDown{};
    std::uint64_t injectedEvents{};
    [[nodiscard]] bool OwnedForeground() const noexcept
    {
        DWORD process{};
        const HWND foreground = GetForegroundWindow();
        return foreground && GetWindowThreadProcessId(foreground, &process) && process == GetCurrentProcessId();
    }
    [[nodiscard]] bool NativeFocusInside() const noexcept
    {
        const HWND focus = GetFocus();
        return OwnedForeground() && focus && (focus == app.Window() || IsChild(app.Window(), focus));
    }
    [[nodiscard]] bool EditorFocused() const noexcept
    {
        DxUi::NativeTextInputState native;
        const auto selection = app.Document().GetSelection();
        return app.Host().GetFocusControl() == &app.Document() && NativeFocusInside() && app.Host().HasActiveNativeTextInputSession() &&
               app.Host().TryReadNativeTextInputState(&app.Document(), native) && native.text == app.Document().GetText() &&
               native.caretIndex == selection.caret && native.selectionAnchorIndex.value_or(native.caretIndex) == selection.anchor &&
               native.multiline && !native.masked && native.readOnly == app.Document().IsReadOnly();
    }
    void Pump()
    {
        MSG message{};
        unsigned processed{};
        while (processed++ < 128 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                Require(shuttingDown, "The interactive window quit before the close case");
                break;
            }
            if (!app.RouteShortcut(message) && !app.Host().DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (app.closing)
        {
            app.FinishClose();
            return;
        }
        if (app.Host().GetHwnd())
        {
            (void)app.Host().RenderInitialFrameForShow();
            app.ServiceImages();
            app.FolderCompleted();
            app.RecoveryCompleted();
            if (app.files.Busy())
                app.FileCompleted();
        }
    }
    template <class Predicate> void Await(Predicate &&complete, const char *message, DWORD timeout = 5000)
    {
        const auto deadline = GetTickCount64() + timeout;
        do
        {
            Pump();
            Require(shuttingDown || !app.closing, "The owned application began closing before its close stage");
            if (complete())
                return;
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        } while (GetTickCount64() < deadline);
        Require(complete(), message);
    }
    void Send(std::span<INPUT> events)
    {
        Require(leaseGuarded && OwnedForeground(), "Input refused because the owned test process does not hold foreground");
        const UINT count = static_cast<UINT>(events.size());
        const UINT sent = SendInput(count, events.data(), sizeof(INPUT));
        injectedEvents += sent;
        Require(sent == count, "Windows refused part of the owned keyboard input sequence");
    }
    static INPUT KeyEvent(WORD key, bool up = false) noexcept
    {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key;
        input.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
        if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN || key == VK_HOME || key == VK_END || key == VK_DELETE)
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        return input;
    }
    void Key(WORD key, std::initializer_list<WORD> modifiers = {})
    {
        std::array<INPUT, 10> events{};
        std::size_t count{};
        Require(modifiers.size() <= 4, "The bounded chord has too many modifiers");
        for (const auto modifier : modifiers)
        {
            events[count++] = KeyEvent(modifier);
            attemptedKeys[modifier] = true;
        }
        attemptedKeys[key] = true;
        events[count++] = KeyEvent(key);
        events[count++] = KeyEvent(key, true);
        for (auto modifier = modifiers.end(); modifier != modifiers.begin();)
            events[count++] = KeyEvent(*--modifier, true);
        Send(std::span(events).first(count));
    }
    void Type(std::wstring_view text)
    {
        for (const wchar_t character : text)
        {
            std::array<INPUT, 2> events{};
            for (auto &input : events)
            {
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = character;
                input.ki.dwFlags = KEYEVENTF_UNICODE;
            }
            events[1].ki.dwFlags |= KEYEVENTF_KEYUP;
            Send(events);
        }
    }
    void ReleaseKeys() noexcept
    {
        std::array<INPUT, 256> events{};
        unsigned count{};
        for (unsigned key = 0; key < attemptedKeys.size(); ++key)
            if (attemptedKeys[key] && (GetAsyncKeyState(static_cast<int>(key)) & 0x8000))
                events[count++] = KeyEvent(static_cast<WORD>(key), true);
        // Cleanup releases only keys this child pressed, even if foreground was
        // lost. It never inserts text or presses a key into another application.
        if (count)
            (void)SendInput(count, events.data(), sizeof(INPUT));
    }
    template <class Test> void Case(std::string name, Test &&test)
    {
        outcomes.push_back({std::move(name), false, {}});
        test();
        outcomes.back().passed = true;
    }
    void Setup()
    {
        Require(!options.headless && !options.designInteractive.empty() && CanonicalLeaseOwnsParent(),
                "The probe requires its visible canonical DxUi desktop-lease parent");
        leaseGuarded = true;
        for (const int key : {VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN})
            Require((GetAsyncKeyState(key) & 0x8000) == 0, "The approved desktop hand-over still has a held modifier");
        const auto directory = std::filesystem::absolute(options.dataDirectory) / L"DesignInteractiveFixture/Writing";
        std::filesystem::create_directories(directory / L"Drafts");
        Require(SUCCEEDED(WriteBytesAtomic(directory / L"Drafts/Chapter2.md", "# A next chapter\n")) &&
                    SUCCEEDED(WriteBytesAtomic(directory / L"Outline.md", "# An invented outline\n")),
                "Cannot seed the owned interaction fixture");
        baseline = L"# Ideas in progress\n\nA careful sentence and **strong words**.\n\n## Next steps\n\n"
                   L"Read the notes and make a plan. Keep useful notes nearby.\n\nInline $a_i^2$ stays exact.\n";
        const auto path = directory / L"Ideas.md";
        FileStamp identity;
        Require(SUCCEEDED(SaveDocument(path, baseline, false, nullptr, identity)), "Cannot stage the owned interaction document");
        app.SetDocument(baseline, path, false, identity);
        app.SetView(EditorView::Compose);
        (void)app.Host().PrimeForShow();
        ShowWindow(app.Window(), SW_SHOWNORMAL);
        UpdateWindow(app.Window());
        (void)SetForegroundWindow(app.Window());
        app.Host().SetFocusControl(&app.Document(), true);
        Await([&] { return EditorFocused(); }, "Windows refused the foreground grant from the canonical lease");
    }
    void Execute()
    {
        Case("NativeEditor.KeyboardTextAndUndo",
             [&]
             {
                 Key('2', {VK_CONTROL});
                 Await([&] { return app.Preferences().view == EditorView::Source && EditorFocused(); }, "Ctrl+2 did not route to Source");
                 const auto at = baseline.find(L"A careful");
                 app.Document().SetSelection({at, at});
                 app.Host().SetFocusControl(&app.Document(), true);
                 Type(L"x");
                 Await([&] { return app.Document().GetText() == baseline.substr(0, at) + L"x" + baseline.substr(at); },
                       "Real keyboard text did not reach the authoritative source");
                 Key('Z', {VK_CONTROL});
                 Await([&] { return app.Document().GetText() == baseline && !app.Dirty(); }, "Ctrl+Z did not undo the real keyboard edit");
                 Key('1', {VK_CONTROL});
                 Await([&] { return app.Preferences().view == EditorView::Compose; }, "Ctrl+1 did not route back to Compose");
             });
        Case("Files.ShortcutFocusAndEscape",
             [&]
             {
                 const auto at = baseline.find(L"A careful");
                 app.Document().SetSelection({at, at + 5});
                 app.SelectionChanged();
                 const auto selection = app.Document().GetSelection();
                 Key('E', {VK_CONTROL, VK_SHIFT});
                 Await(
                     [&]
                     { return app.settings.showFiles && app.folderSnapshot && app.Host().GetFocusControl() == app.filesTree && NativeFocusInside(); },
                     "Ctrl+Shift+E did not open and natively focus the Files tree");
                 Key(VK_ESCAPE);
                 Await([&] { return !app.settings.showFiles && EditorFocused(); }, "Escape did not close Files and restore native editor focus");
                 Require(app.Document().GetText() == baseline && app.Document().GetSelection() == selection,
                         "Files routing changed source or the preserved source selection");
             });
        Case("Find.TypedQueryTraversalAndEscape",
             [&]
             {
                 Key('F', {VK_CONTROL});
                 Await([&] { return app.findBar->IsVisible() && app.Host().GetFocusControl() == app.findText && NativeFocusInside(); },
                       "Ctrl+F did not focus the real Find field");
                 Key('A', {VK_CONTROL});
                 Type(L"notes");
                 Await([&] { return app.findText->GetText() == L"notes" && app.findMatches.size() == 2; },
                       "Native Find input did not update exact matches");
                 Key(VK_F3);
                 Await([&] { return app.Document().GetSelection().Start() == baseline.find(L"notes"); }, "F3 did not select the first source match");
                 Key(VK_F3);
                 Await([&] { return app.Document().GetSelection().Start() == baseline.rfind(L"notes"); },
                       "F3 did not advance to the next source match");
                 Key(VK_F3, {VK_SHIFT});
                 Await([&] { return app.Document().GetSelection().Start() == baseline.find(L"notes"); },
                       "Shift+F3 did not select the previous source match");
                 const auto selection = app.Document().GetSelection();
                 Key(VK_ESCAPE);
                 Await([&] { return !app.findBar->IsVisible() && EditorFocused(); }, "Escape did not restore editor focus from Find");
                 Require(app.Document().GetText() == baseline && app.Document().GetSelection() == selection,
                         "Find changed source or lost its selection on close");
             });
        Case("Settings.KeyboardDrawerAndEscape",
             [&]
             {
                 Key(VK_OEM_COMMA, {VK_CONTROL});
                 Await([&] { return app.drawer->IsVisible() && NativeFocusInside(); }, "Ctrl+comma did not open Settings with native focus");
                 const auto *first = app.Host().GetFocusControl();
                 Key(VK_TAB);
                 Await([&] { return app.Host().GetFocusControl() != first; }, "Tab did not navigate actual Settings controls");
                 Key(VK_RETURN);
                 Await([&] { return app.settings.appearance == Appearance::Light; }, "Enter did not invoke the next real theme tile");
                 Key(VK_ESCAPE);
                 Await([&] { return !app.drawer->IsVisible() && EditorFocused(); }, "Escape did not restore native editor focus from Settings");
                 Require(app.Document().GetText() == baseline && !app.Dirty(), "Live Settings changed document source/history");
             });
        Case("Focus.KeyboardToggleAndEscape",
             [&]
             {
                 Key(VK_F11);
                 Await([&] { return app.settings.focusMode && app.exitFocus->IsVisible(); }, "F11 did not enter Focus mode");
                 Key(VK_ESCAPE);
                 Await([&] { return !app.settings.focusMode && EditorFocused(); }, "Escape did not leave Focus mode with native editor routing");
                 Require(app.Document().GetText() == baseline && !app.Dirty(), "Focus mode changed source/history");
             });
        Case("SelectionToolbar.AltF10TabEnterEscape",
             [&]
             {
                 Key('2', {VK_CONTROL});
                 Await([&] { return app.settings.view == EditorView::Source; }, "Source preparation did not preserve real shortcut routing");
                 const auto at = baseline.find(L"A careful");
                 app.Document().SetSelection({at, at});
                 app.Host().SetFocusControl(&app.Document(), true);
                 for (unsigned index = 0; index < 5; ++index)
                 {
                     Key(VK_RIGHT, {VK_SHIFT});
                     Await([&] { return app.Document().GetSelection().End() == at + index + 1; }, "Shift+Right did not extend the source selection");
                 }
                 const auto selection = app.Document().GetSelection();
                 Key('1', {VK_CONTROL});
                 Await([&] { return app.settings.view == EditorView::Compose && app.Document().GetSelection() == selection; },
                       "Compose lost the actual keyboard selection");
                 Key(VK_F10, {VK_MENU});
                 Await([&] { return app.floating->IsVisible() && app.Host().GetFocusControl() == app.floatingActions[0] && NativeFocusInside(); },
                       "Alt+F10 did not focus selected-text tools");
                 Key(VK_TAB);
                 Await([&] { return app.Host().GetFocusControl() == app.floatingActions[1]; }, "Tab did not navigate the actual floating toolbar");
                 Key(VK_RETURN);
                 auto formatted = baseline;
                 formatted.replace(at, 5, L"**A car**");
                 Await([&] { return app.Document().GetText() == formatted && app.Dirty() && EditorFocused(); },
                       "Enter did not invoke formatting on the retained source selection");
                 Key('Z', {VK_CONTROL});
                 Await([&] { return app.Document().GetText() == baseline && !app.Dirty(); },
                       "One undo did not restore the actual formatting transaction");
                 Key(VK_F10, {VK_MENU});
                 Await([&] { return app.floating->IsVisible() && app.Host().GetFocusControl() == app.floatingActions[0] && NativeFocusInside(); },
                       "Selected tools could not reopen with first-action focus after undo");
                 const auto retained = app.Document().GetSelection();
                 Key(VK_ESCAPE);
                 Await([&] { return !app.floating->IsVisible() && EditorFocused(); },
                       "Escape did not close selected tools and restore native editor focus");
                 Require(app.Document().GetSelection() == retained && app.Document().GetText() == baseline,
                         "Toolbar Escape discarded the preserved source selection");
             });
        Case("Equation.F2ApplyUndoAndCancel",
             [&]
             {
                 const auto caret = baseline.find(L"a_i^2");
                 app.Document().SetSelection({caret, caret});
                 app.SelectionChanged();
                 Key(VK_F2);
                 Await([&] { return app.HasBlockDraft() && app.BlockDraft()->GetText() == L"$a_i^2$" && NativeFocusInside(); },
                       "F2 did not focus the guarded exact-delimiter equation draft");
                 Key('A', {VK_CONTROL});
                 Type(L"$a_i^3$");
                 Await([&] { return app.BlockDraft()->GetText() == L"$a_i^3$"; }, "Native equation-source typing did not reach the draft");
                 Key(VK_RETURN, {VK_CONTROL});
                 auto edited = baseline;
                 edited.replace(edited.find(L"$a_i^2$"), 7, L"$a_i^3$");
                 Await([&] { return !app.HasBlockDraft() && app.Document().GetText() == edited && EditorFocused(); },
                       "Ctrl+Enter did not apply one exact source transaction and restore native editor focus");
                 Key('Z', {VK_CONTROL});
                 Await([&] { return app.Document().GetText() == baseline && !app.Dirty(); }, "One undo did not restore the equation source");
                 app.Document().SetSelection({caret, caret});
                 app.SelectionChanged();
                 const auto retainedSource = app.Document().Snapshot();
                 const auto retainedRevision = app.Document().Revision();
                 const auto retainedSelection = app.Document().GetSelection();
                 const bool retainedUndo = app.Document().CanUndo(), retainedRedo = app.Document().CanRedo();
                 Key(VK_F2);
                 Await([&] { return app.HasBlockDraft(); }, "F2 could not reopen the equation draft");
                 Key('A', {VK_CONTROL});
                 Type(L"$a_i^4$");
                 Await([&] { return app.BlockDraft()->GetText() == L"$a_i^4$"; }, "Cancelled equation draft did not receive native text");
                 Key(VK_ESCAPE);
                 Await([&] { return !app.HasBlockDraft() && EditorFocused(); }, "Escape did not cancel the equation draft and restore native focus");
                 Require(app.Document().GetText() == baseline && !app.Dirty() && app.Document().Snapshot().IsSameRevision(retainedSource) &&
                             app.Document().Revision() == retainedRevision && app.Document().GetSelection() == retainedSelection &&
                             app.Document().CanUndo() == retainedUndo && app.Document().CanRedo() == retainedRedo,
                         "Cancelled equation source mutated authoritative source, revision, selection or undo/redo history");
             });
        Case("FileMenu.KeyboardOpenAndDismiss",
             [&]
             {
                 const auto selection = app.Document().GetSelection();
                 app.Host().SetFocusControl(app.fileMenu, true);
                 Key(VK_RETURN);
                 Await(
                     [&]
                     {
                         DWORD process{};
                         const HWND capture = GetCapture();
                         return capture && capture != app.Window() && GetWindowThreadProcessId(capture, &process) && process == GetCurrentProcessId();
                     },
                     "Enter did not open the real app-owned File menu");
                 Key(VK_ESCAPE);
                 Await([&] { return GetCapture() == nullptr && EditorFocused(); },
                       "Escape did not dismiss the native File menu and restore native focus");
                 Require(app.Document().GetText() == baseline && app.Document().GetSelection() == selection,
                         "Native menu dismissal changed source/selection");
             });
        Case("Window.AltF4AsyncClose",
             [&]
             {
                 Require(!app.Dirty() && EditorFocused(), "Close setup must preserve the clean owned document");
                 shuttingDown = true;
                 Key(VK_F4, {VK_MENU});
                 Await([&] { return app.Window() == nullptr; }, "Alt+F4 did not complete asynchronous app-owned shutdown", 10000);
                 Require(app.closing && app.Host().GetHwnd() == nullptr && !app.folderFiles.TakeResult(),
                         "Shutdown retained a live UI binding or directory publication");
             });
    }
    int WriteResult()
    {
        const bool passed = outcomes.size() == 9 && std::ranges::all_of(outcomes, [](const auto &outcome) { return outcome.passed; });
        std::string json = std::format("{{\n  \"schemaVersion\": 1,\n  \"probe\": \"design-interaction\",\n  \"leaseGuarded\": {},\n"
                                       "  \"realKeyboardInput\": {},\n  \"injectedEvents\": {},\n  \"clipboardTouched\": false,\n"
                                       "  \"physicalImeOrAtQualified\": false,\n  \"tests\": [\n",
                                       leaseGuarded ? "true" : "false", injectedEvents ? "true" : "false", injectedEvents);
        for (std::size_t index = 0; index < outcomes.size(); ++index)
        {
            const auto &outcome = outcomes[index];
            json += std::format("    {{\"name\": {}, \"passed\": {}, \"error\": {}}}{}\n", JsonString(outcome.name),
                                outcome.passed ? "true" : "false", JsonString(outcome.error), index + 1 == outcomes.size() ? "" : ",");
        }
        json += std::format("  ],\n  \"passed\": {},\n  \"restoration\": \"Verified separately by the canonical parent desktop lease\"\n}}\n",
                            passed ? "true" : "false");
        return SUCCEEDED(WriteBytesAtomic(options.designInteractive, json)) ? passed ? 0 : 1 : 3;
    }

  public:
    DesignInteractiveProbe(Application &application, const LaunchOptions &launch) : app(application), options(launch) {}
    int Run()
    {
        const auto release = wil::scope_exit([&] { ReleaseKeys(); });
        try
        {
            Setup();
            Execute();
        }
        catch (const std::bad_alloc &)
        {
            std::terminate(); // Application-test boundary: allocation failure is fatal.
        }
        catch (const std::exception &error)
        {
            // Harness boundary: retain the exact failed stage, stop all further
            // input and retire only this child's app before the parent restores.
            if (outcomes.empty() || outcomes.back().passed)
                outcomes.push_back({"Lease.Setup", false, error.what()});
            else
                outcomes.back().error = error.what();
            if (!app.closing && app.Host().GetRoot())
            {
                const auto text = app.Document().GetText();
                std::string units;
                for (const wchar_t unit : text)
                    units += std::format("{:04x} ", static_cast<unsigned>(unit));
                const auto selection = app.Document().GetSelection();
                outcomes.back().error += std::format("; sourceUtf16=[{}]; selection={}/{}; dirty={}; editorFocused={}", units, selection.anchor,
                                                     selection.caret, app.Dirty(), EditorFocused());
            }
            OutputDebugStringW(L"RedTilde design interaction failed; see the retained probe JSON.\n");
            ReleaseKeys();
            if (leaseGuarded && app.Window())
            {
                shuttingDown = true;
                try
                {
                    if (!app.closing)
                        app.BeginClose();
                    Await([&] { return app.Window() == nullptr; }, "Failure cleanup did not retire its owned HWND", 10000);
                }
                catch (const std::bad_alloc &)
                {
                    std::terminate(); // Cleanup must retain fatal allocation behavior.
                }
                catch (const std::exception &cleanup)
                {
                    // Keep the original failure and its cleanup outcome; the
                    // canonical parent job/watchdog still owns final retirement.
                    outcomes.back().error += std::string("; cleanup: ") + cleanup.what();
                }
            }
        }
        return WriteResult();
    }
};
#pragma warning(pop)

int RunDesignInteractiveProbe(Application &app, const LaunchOptions &options)
{
    DesignInteractiveProbe probe(app, options);
    return probe.Run();
}
} // namespace RedTilde
