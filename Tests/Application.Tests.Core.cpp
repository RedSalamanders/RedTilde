#include "RedTilde/Application.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace RedTilde
{
using DxUi::RichText::ParseMath;
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
std::wstring WindowCaption(HWND window)
{
    std::array<wchar_t, 1024> text{};
    const int length = GetWindowTextW(window, text.data(), static_cast<int>(text.size()));
    Require(length > 0, "read native Unicode caption");
    return std::wstring(text.data(), static_cast<std::size_t>(length));
}
} // namespace
int RunCoreTests(const std::filesystem::path &root, CoreTestScope scope, bool listOnly)
{
    std::filesystem::create_directories(root);
    std::ofstream report(root / L"tests.json", std::ios::binary);
    report << "{\n  \"tests\": [\n";
    unsigned passed{}, failed{}, selected{}, registered{};
    auto test = [&](std::string_view name, const std::function<void()> &run)
    {
        ++registered;
        const auto prefix = name.substr(0, name.find('.'));
        CoreTestScope owner{};
        if (prefix == "Utf8" || prefix == "Storage" || prefix == "Files")
            owner = CoreTestScope::Storage;
        else if (prefix == "Settings")
            owner = CoreTestScope::Settings;
        else if (prefix == "Recovery")
            owner = CoreTestScope::Recovery;
        else if (prefix == "Editor" || prefix == "Window" || prefix == "Outline" || prefix == "Math" || prefix == "Performance" ||
                 prefix == "Rendering")
            owner = CoreTestScope::Editor;
        else
            throw std::invalid_argument("Unmapped native test prefix");
        if (scope != CoreTestScope::All && owner != scope)
            return;
        if (selected++)
            report << ",\n";
        if (listOnly)
        {
            report << "    {\"name\":\"" << name << "\",\"executed\":false}";
            return;
        }
        report << "    {\"name\":\"" << name << "\",\"passed\":";
        try
        {
            run();
            ++passed;
            report << "true}";
        }
        catch (const std::bad_alloc &)
        {
            std::terminate();
        }
        catch (const std::exception &error)
        {
            ++failed;
            report << "false,\"error\":\"";
            for (const char c : std::string_view(error.what()))
                report << (c == '"' ? '\'' : c == '\\' ? '/' : c == '\n' ? ' ' : c);
            report << "\"}";
        }
        report.flush();
    };
    test("Utf8.BomUnicodeMixedEolRoundTrip",
         [&]
         {
             const auto path = root / L"Unicode-東京.md";
             const std::wstring source = L"# Café 東京 😀\r\n\n**β**\rnext\n";
             FileStamp saved;
             Success(SaveDocument(path, source, true, nullptr, saved), "save UTF-8 BOM");
             StoredDocument loaded;
             Success(LoadDocument(path, loaded), "load UTF-8 BOM");
             Require(loaded.text == source && loaded.bom && loaded.stamp == saved, "source, BOM and stamp preserved");
             std::string bytes;
             Success(ReadBytes(path, bytes), "read saved bytes");
             Require(bytes.starts_with("\xEF\xBB\xBF"), "BOM bytes preserved");
         });
    test("Utf8.InvalidInputDoesNotReplaceOutput",
         [&]
         {
             const auto path = root / L"invalid.md";
             Success(WriteBytesAtomic(path, "\xF0\x28\x8C\x28"), "create malformed input");
             StoredDocument loaded{L"retained", true, {}};
             Require(FAILED(LoadDocument(path, loaded)) && loaded.text == L"retained" && loaded.bom, "invalid UTF-8 leaves output unchanged");
             Success(WriteBytesAtomic(path, std::string("A\0B", 3)), "create NUL input");
             Require(FAILED(LoadDocument(path, loaded)) && loaded.text == L"retained", "NUL leaves output unchanged");
         });
    test("Storage.AtomicConflictAndFailedSave",
         [&]
         {
             const auto path = root / L"conflict.md";
             FileStamp saved;
             Success(SaveDocument(path, L"original", false, nullptr, saved), "create original");
             Success(WriteBytesAtomic(path, "external"), "simulate external editor");
             FileStamp unused;
             Require(SaveDocument(path, L"mine", false, &saved, unused) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "external conflict rejected");
             StoredDocument loaded;
             Success(LoadDocument(path, loaded), "read after rejected save");
             Require(loaded.text == L"external", "external contents retained");
             Require(FAILED(SaveDocument(root / L"missing" / L"file.md", L"mine", false, nullptr, unused)), "missing-parent failure surfaced");
             for (const auto &entry : std::filesystem::directory_iterator(root))
                 Require(entry.path().extension() != L".tmp", "no owned temporary leaked");
         });
    test("Storage.CapacityRejectedWithoutTruncation",
         [&]
         {
             const auto path = root / L"oversized.md";
             Success(WriteBytesAtomic(path, std::string(65537, 'x')), "write oversized input");
             StoredDocument loaded{L"keep", false, {}};
             Require(LoadDocument(path, loaded) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) && loaded.text == L"keep", "oversized file rejected");
             FileStamp stamp;
             Require(FAILED(SaveDocument(path, std::wstring(65537, L'x'), false, nullptr, stamp)), "oversized save rejected");
         });
    test("Settings.VersionAndFieldFallback",
         [&]
         {
             Settings value;
             value.appearance = Appearance::Paper;
             value.view = EditorView::Split;
             value.showOutline = false;
             const auto round = ParseSettings(SerializeSettings(value));
             Require(round.appearance == value.appearance && round.view == value.view && !round.showOutline, "settings round trip");
             const auto invalid = ParseSettings("RedTilde.Settings/1\ntheme=2\nview=99\nscale=900\noutline=0\n");
             Require(invalid.appearance == Appearance::Dark && invalid.view == EditorView::Compose && !invalid.showOutline,
                     "field fallback isolated");
             Require(ParseSettings("RedTilde.Settings/9\ntheme=2\n").appearance == Appearance::System, "unknown schema defaults");
             Success(SaveSettings(root, value), "save preferences");
             Settings loaded;
             Success(LoadSettings(root, loaded), "reload preferences");
             Require(loaded.appearance == Appearance::Paper, "persisted preferences");
         });
    test("Outline.FencesAndUnicode",
         []
         {
             const auto info = AnalyzeDocument(L"# 東京\r\n```md\n# hidden\n```\n## Second ##\n~~~\n### hidden too\n~~~\n");
             Require(info.headings.size() == 2 && info.headings[0].text == L"東京" && info.headings[1].text == L"Second", "only real ATX headings");
             Require(info.headings[1].level == 2, "heading level retained");
         });
    test("Settings.AsyncLatestValueSurvivesWriterClose",
         [&]
         {
             const auto directory = root / L"async-preferences";
             SettingsWriter writer;
             writer.Initialize(directory);
             Settings value;
             for (unsigned index = 0; index < 50; ++index)
             {
                 value.appearance = static_cast<Appearance>(index % 3);
                 Success(writer.Submit(value), "submit bounded preference update");
             }
             value.appearance = Appearance::Paper;
             value.view = EditorView::Split;
             value.showOutline = false;
             Success(writer.Submit(value), "submit final preference state");
             writer.Close();
             bool saved{};
             for (unsigned turn = 0; turn < 400 && !saved; ++turn)
             {
                 Settings loaded;
                 if (SUCCEEDED(LoadSettings(directory, loaded)))
                     saved = loaded.appearance == Appearance::Paper && loaded.view == EditorView::Split && !loaded.showOutline;
                 if (!saved)
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
             }
             Require(saved, "latest admitted preferences persist after UI binding closes");
         });
    test("Math.NativeSubsetAndRejection",
         []
         {
             const auto expression = ParseMath(L"x = \\frac{-b+\\sqrt{b^2-4ac}}{2a}");
             Require(static_cast<bool>(expression), "quadratic formula parses");
             Require(static_cast<bool>(ParseMath(L"\\sum_{i=0}^{n} i + \\alpha \\leq \\infty")), "scripts and symbols parse");
             Require(!ParseMath(L"\\include{secrets}") && !ParseMath(L"\\frac{a}") && !ParseMath(L"x}"), "unsupported/malformed input rejected");
             Require(!ParseMath(std::wstring(4097, L'x')), "capacity bounded");
             Require(!ParseMath(std::wstring(30, L'{') + L"x" + std::wstring(30, L'}')), "recursion bounded");
             const std::wstring markdown = L"a $x^2$ b $$ y=\\frac{a}{b} $$";
             Require(EquationAt(markdown, 4) == L"x^2" && EquationAt(markdown, 20) == L" y=\\frac{a}{b} ", "caret equation extraction");
         });
    test("Recovery.StrictEnvelopeAndUnicode",
         []
         {
             RecoverySnapshot value{L"# 東京 😀\r\n$α$\n", L"Z:\\writing\\東京.md", true, {2, 5}, 42};
             const auto bytes = SerializeRecovery(value);
             RecoverySnapshot parsed;
             Success(ParseRecovery(bytes, parsed), "parse recovery envelope");
             Require(parsed.text == value.text && parsed.originalPath == value.originalPath && parsed.bom && parsed.generation == 42 &&
                         parsed.selection.anchor == 2 && parsed.selection.caret == 5,
                     "source, encoding, location and selection round trip");
             auto corrupt = bytes;
             corrupt.back() ^= 1;
             Require(FAILED(ParseRecovery(corrupt, parsed)) && parsed.text == value.text, "checksum failure preserves output");
             auto corruptMetadata = bytes;
             const auto bomAt = corruptMetadata.find(" 1 2 5 ");
             Require(bomAt != corruptMetadata.npos, "test metadata locates the independent encoding field");
             corruptMetadata[bomAt + 1] = '0';
             Require(FAILED(ParseRecovery(corruptMetadata, parsed)), "checksum also protects BOM and selection metadata");
             Require(FAILED(ParseRecovery(bytes.substr(0, bytes.size() - 1), parsed)), "truncated write rejected");
             Require(FAILED(ParseRecovery("RedTilde.Recovery/9\n", parsed)), "future schema rejected");
             Require(FAILED(ParseRecovery("RedTilde.Recovery/1\n99999999999999999999999 0 0 0 0 0 0\n", parsed)), "overflow rejected");
             const RecoverySnapshot maximum{std::wstring(65536, L'界'), {}, false, {65536, 65536}, 100};
             Success(ParseRecovery(SerializeRecovery(maximum), parsed), "maximum Unicode document fits the recovery envelope");
             Require(parsed.text == maximum.text, "full-capacity recovery does not truncate source");
         });
    test("Recovery.DebounceLeaseCoalescingAndRetirement",
         [&]
         {
             const auto directory = root / L"recovery-service";
             auto wait = [](const std::function<bool()> &predicate)
             {
                 for (unsigned turn = 0; turn < 1000; ++turn)
                 {
                     if (predicate())
                         return;
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 Require(false, "recovery worker timed out");
             };
             RecoveryService writer;
             Success(writer.Initialize(directory), "initialize worker");
             wait([&] { return writer.Ready(); });
             for (unsigned index = 0; index < 50; ++index)
                 Success(writer.Submit(RecoverySnapshot{L"draft " + std::to_wstring(index), {}, false, {0, 0}, index}), "coalesce rapid edit");
             Success(writer.Submit(RecoverySnapshot{L"latest 😀\r\n", L"Z:\\unchanged.md", true, {0, 6}, 51}, true), "flush final state");
             wait([&] { return writer.Settled(); });
             Success(writer.Status(), "durable checkpoint");
             std::filesystem::path checkpoint;
             for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                 if (entry.path().extension() == L".rtd")
                     checkpoint = entry.path();
             Require(!checkpoint.empty(), "checkpoint exists");
             wil::unique_handle locked(CreateFileW(checkpoint.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
             Require(static_cast<bool>(locked), "hold recovery replacement lock");
             Success(writer.Submit(RecoverySnapshot{L"blocked replacement", {}, false, {0, 0}, 52}, true), "admit failing checkpoint");
             wait([&] { return writer.Settled(); });
             Require(FAILED(writer.Status()), "checkpoint replacement failure is visible");
             std::string preserved;
             Success(ReadBytes(checkpoint, preserved), "read retained recovery after failed replacement");
             RecoverySnapshot retained;
             Success(ParseRecovery(preserved, retained), "old checkpoint remains complete");
             Require(retained.text == L"latest 😀\r\n", "failed write cannot destroy the previous recoverable source");
             locked.reset();
             Success(writer.Submit(retained, true), "retry checkpoint after releasing lock");
             wait([&] { return writer.Settled(); });
             Success(writer.Status(), "successful checkpoint clears prior failure");
             RecoveryService concurrent;
             Success(concurrent.Initialize(directory), "second live process identity");
             wait([&] { return concurrent.Ready(); });
             Require(concurrent.TakeCandidates().empty(), "active leased snapshot is never offered");
             concurrent.Stop();
             wait([&] { return concurrent.Finished(); });
             writer.Stop();
             wait([&] { return writer.Finished(); });
             RecoveryService restart;
             Success(restart.Initialize(directory), "restart scan");
             wait([&] { return restart.Ready(); });
             const auto candidates = restart.TakeCandidates();
             Require(candidates.size() == 1 && candidates[0].snapshot.text == L"latest 😀\r\n" && candidates[0].snapshot.bom,
                     "restart discovers exact latest source");
             Success(restart.Submit(candidates[0].snapshot, true, candidates[0].id), "adopt without overwriting original");
             wait([&] { return restart.Settled(); });
             Success(restart.Submit(std::nullopt, true), "matching clean save retires recovery");
             restart.Stop();
             wait([&] { return restart.Finished(); });
             for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                 Require(entry.path().extension() != L".rtd", "clean session and adopted snapshot retired");
             Require(FAILED(restart.Discard(L"..\\outside")), "foreign paths cannot be discarded");
         });
    test("Recovery.CorruptAndFutureDataQuarantined",
         [&]
         {
             const auto directory = root / L"recovery-corrupt";
             std::filesystem::create_directories(directory / L"Recovery");
             const auto path = directory / L"Recovery" / L"session-0123456789abcdef0123456789abcdef.rtd";
             Success(WriteBytesAtomic(path, "RedTilde.Recovery/9\nkeep these bytes"), "write future snapshot");
             RecoveryService scanner;
             Success(scanner.Initialize(directory), "scan unknown schema");
             for (unsigned turn = 0; turn < 1000 && !scanner.Ready(); ++turn)
                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
             Require(scanner.Ready() && scanner.TakeCandidates().empty(), "unknown schema never restores");
             std::string bytes;
             Success(ReadBytes(path.native() + L".quarantine", bytes), "quarantined bytes readable");
             Require(bytes == "RedTilde.Recovery/9\nkeep these bytes", "unreadable snapshot is retained intact");
             scanner.Stop();
         });
    test("Editor.ViewSwitchKeepsSourceSelectionHistory",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create hidden app");
             const auto foreground = GetForegroundWindow();
             app.SetDocument(L"# Hello\n\nUnicode 😀 **words**\n");
             app.Document().SetSelection({9, 16});
             Require(app.Document().ReplaceSelection(L"世界"), "edit source");
             const auto source = app.Document().GetText();
             const auto selection = app.Document().GetSelection();
             const auto snapshot = app.Document().Snapshot();
             app.SetView(EditorView::Source);
             app.SetView(EditorView::Split);
             app.SetView(EditorView::Compose);
             Require(app.Document().GetText() == source && app.Document().GetSelection() == selection, "view switch source/selection unchanged");
             Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().CanUndo(), "same source revision and history");
             Require(app.Dirty(), "edited source dirty");
             Require(app.Document().Undo() && !app.Dirty(), "undo to saved source clears dirty");
             Require(GetForegroundWindow() == foreground, "hidden app never acquires foreground");
         });
    test("Editor.HiddenMirrorSynchronizesWithoutEditingSource",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create deferred-mirror app");
             const std::wstring source = L"# Mirror\n\nBody 東京 😀.\n";
             app.SetDocument(source);
             app.Prepare();
             const auto hidden = app.SourceMirror().Snapshot();
             const auto before = app.Activity();
             const auto body = source.find(L"Body");
             app.Document().SetSelection({body, body});
             Require(app.Document().ReplaceSelection(L"More "), "edit Compose prose");
             Require(app.SourceMirror().Snapshot().IsSameRevision(hidden), "hidden mirror retains its prior revision");
             Require(app.Activity().mirrorResets == before.mirrorResets && app.Activity().layouts == before.layouts,
                     "ordinary Compose edits do not rebuild the mirror or chrome");
             Require(app.Document().Undo() && !app.Dirty(), "undo restores saved source");
             Require(app.Document().Redo(), "redo the edit before switching views");
             const auto current = app.Document().Snapshot();
             const auto selection = app.Document().GetSelection();
             app.SetView(EditorView::Source);
             Require(app.SourceMirror().Snapshot().IsSameRevision(hidden), "Source also leaves the hidden mirror deferred");
             app.SetView(EditorView::Split);
             Require(app.SourceMirror().GetText() == app.Document().GetText() && app.SourceMirror().GetSelection() == selection,
                     "entering Split synchronizes exact source and selection");
             Require(app.Document().Snapshot().IsSameRevision(current) && app.Document().CanUndo(),
                     "synchronization preserves the editable source revision and history");
             const auto mirrored = app.SourceMirror().Snapshot();
             app.SetView(EditorView::Split);
             Require(app.SourceMirror().Snapshot().IsSameRevision(mirrored), "reselecting Split preserves the current mirror revision");
             const auto split = app.Activity();
             Require(app.Document().ReplaceSelection(L"Split "), "edit Split prose");
             Require(app.SourceMirror().GetText() == app.Document().GetText() && app.Activity().mirrorResets == split.mirrorResets + 1,
                     "each visible Split revision updates its mirror");
             Require(app.Activity().layouts == split.layouts, "Split prose does not relayout chrome");
             Require(app.Document().Undo() && app.Document().Undo(), "undo both edits");
             Require(app.SourceMirror().GetText() == source && !app.Dirty(), "Split undo restores the exact saved source in both panes");
         });
    test("Editor.OutlineTracksShiftedOffsetsAndChangedLabels",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create outline app");
             const std::wstring source = L"# First\n\nBody.\n\n## Navigation target\n\nTail.\n";
             app.SetDocument(source);
             app.Prepare();
             std::function<DxUi::Button *(DxUi::Control *, std::wstring_view)> find = [&](DxUi::Control *control,
                                                                                          std::wstring_view name) -> DxUi::Button *
             {
                 if (auto *button = dynamic_cast<DxUi::Button *>(control); button && button->GetText() == name)
                     return button;
                 for (std::size_t index = 0; index < control->GetLogicalChildCount(); ++index)
                     if (auto *button = find(control->GetLogicalChild(index), name))
                         return button;
                 return nullptr;
             };
             auto *target = find(app.Host().GetRoot(), L"Navigation target");
             Require(target != nullptr, "outline target is present");
             const auto before = app.Activity();
             const auto body = source.find(L"Body");
             app.Document().SetSelection({body, body});
             Require(app.Document().ReplaceSelection(L"Expanded "), "shift a later heading with prose");
             Require(app.Activity().layouts == before.layouts, "offset-only changes preserve chrome layout");
             Require(target->Invoke(app.Host(), false), "invoke outline without taking desktop focus");
             const auto heading = app.Document().GetText().find(L"## Navigation target");
             Require(app.Document().GetSelection() == DxUi::RichText::Selection{heading, heading}, "outline navigation follows the new offset");
             const auto label = heading + 3;
             app.Document().SetSelection({label, label + std::wstring_view(L"Navigation target").size()});
             Require(app.Document().ReplaceSelection(L"Renamed target"), "rename the heading");
             Require(target->GetText() == L"Renamed target" && app.Activity().layouts == before.layouts + 1,
                     "changed heading text updates the visible label and layout");
             const auto indented = target->GetBounds().left;
             app.Document().SetSelection({heading, heading + 1});
             Require(app.Document().ReplaceSelection(L""), "change heading level");
             Require(target->GetBounds().left < indented, "changed heading level updates outline indentation");
             Require(app.Document().Undo() && app.Document().Undo() && app.Document().Undo() && !app.Dirty(),
                     "outline edits retain exact multi-step undo");
             app.Document().SetSelection({source.size(), source.size()});
             Require(app.Document().ReplaceSelection(L"\n### Added heading\n"), "add another heading");
             auto *added = find(app.Host().GetRoot(), L"Added heading");
             Require(added && added->IsVisible(), "added heading becomes an outline action");
             Require(app.Document().Undo() && !added->IsVisible() && !app.Dirty(),
                     "removing the heading hides its action and preserves saved source");
         });
    test("Window.CaptionSkipsIdenticalWritesAndPreservesImmediateIdentity",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"caption-cache";
             Application app(launch);
             Success(app.Initialize(), "create caption app");
             app.SetDocument(L"seed", root / L"caption-東京.md");
             app.Prepare();
             Require(WindowCaption(app.Window()) == L"caption-東京.md — RedTilde", "initial caption follows the Unicode filename");
             auto before = app.Activity();
             app.Document().SetSelection({0, 0});
             Require(app.Document().ReplaceSelection(L"A") && app.Document().ReplaceSelection(L"B") && app.Document().Undo() && app.Document().Undo(),
                     "burst edits and undo complete");
             Require(app.Activity().captionCalls == before.captionCalls + 2 && app.Activity().captionRequests == before.captionRequests + 4 &&
                         WindowCaption(app.Window()) == L"caption-東京.md — RedTilde",
                     "only dirty-state transitions write the native caption, synchronously");
             app.Prepare();
             Require(app.Activity().captionCalls == before.captionCalls + 2 && WindowCaption(app.Window()) == L"caption-東京.md — RedTilde" &&
                         !app.Dirty() && app.Document().GetText() == L"seed" && !app.Document().CanUndo(),
                     "returning to saved source preserves caption, clean state and history without deferred writes");
             before = app.Activity();
             Require(app.Document().ReplaceSelection(L"A") && app.Document().ReplaceSelection(L"B"), "dirty burst admitted");
             app.Prepare();
             Require(app.Activity().captionCalls == before.captionCalls + 1 && WindowCaption(app.Window()) == L"• caption-東京.md — RedTilde" &&
                         app.Dirty(),
                     "dirty burst applies one immediate native identity");
             before = app.Activity();
             Require(app.Document().ReplaceSelection(L"C"), "further dirty edit admitted");
             app.Prepare();
             Require(app.Activity().captionCalls == before.captionCalls, "an unchanged dirty caption requires no redundant native write");
             app.SetDocument(L"replacement", root / L"old-name.md");
             Require(app.Document().ReplaceSelection(L"x"), "old document edit admitted");
             app.SetDocument(L"latest", root / L"renamed-😀.md");
             app.Prepare();
             Require(WindowCaption(app.Window()) == L"renamed-😀.md — RedTilde" && !app.Dirty() && app.Document().GetText() == L"latest",
                     "native identity follows the newest Unicode filename and clean state");
         });
    test("Window.CaptionRetriesFailedAndSupersededNativeWrites",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"caption-retry";
             Application app(launch);
             Success(app.Initialize(), "create caption retry app");
             app.SetDocument(L"seed", root / L"caption.md");
             app.Prepare();
             struct Interception
             {
                 Application *app;
                 WNDPROC previous{};
                 std::filesystem::path nestedPath;
                 bool reject{}, reenter{}, failed{};
                 LRESULT rejection = FALSE;
             } context{&app, {}, root / L"nested-東京.md"};
             constexpr auto property = L"RedTilde.CaptionTest";
             Require(SetPropW(app.Window(), property, &context) != FALSE, "bind owned hidden-window interception");
             const auto restore = wil::scope_exit(
                 [&]
                 {
                     if (context.previous)
                         (void)SetWindowLongPtrW(app.Window(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(context.previous));
                     (void)RemovePropW(app.Window(), property);
                 });
             WNDPROC intercept = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept -> LRESULT
             {
                 auto &state = *static_cast<Interception *>(GetPropW(hwnd, L"RedTilde.CaptionTest"));
                 if (message == WM_SETTEXT && std::exchange(state.reject, false))
                     return state.rejection;
                 const bool reenter = message == WM_SETTEXT && std::exchange(state.reenter, false);
                 const auto result = CallWindowProcW(state.previous, hwnd, message, wParam, lParam);
                 if (reenter)
                 {
                     try
                     {
                         state.app->SetDocument(L"nested", state.nestedPath);
                     }
                     catch (const std::bad_alloc &)
                     {
                         std::terminate();
                     }
                     catch (const std::exception &)
                     {
                         state.failed = true;
                     }
                 }
                 return result;
             };
             context.previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(app.Window(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(intercept)));
             Require(context.previous != nullptr, "intercept only the app-owned hidden window");
             context.reject = true;
             app.Document().SetSelection({0, 0});
             Require(app.Document().ReplaceSelection(L"A") && WindowCaption(app.Window()) == L"caption.md — RedTilde",
                     "failed native write leaves the old actual caption");
             auto before = app.Activity();
             Require(app.Document().ReplaceSelection(L"B"), "retry edit is admitted");
             Require(app.Activity().captionCalls == before.captionCalls + 1, "retry edit invokes one native caption write");
             Require(WindowCaption(app.Window()) == L"• caption.md — RedTilde", "retry applies the expected dirty caption");
             context.reject = true;
             context.rejection = -1;
             app.SetDocument(L"negative", root / L"negative.md");
             before = app.Activity();
             app.SetDocument(L"retry", root / L"negative.md");
             Require(app.Activity().captionCalls == before.captionCalls + 1 && WindowCaption(app.Window()) == L"negative.md — RedTilde",
                     "a negative receiver result also remains retryable");
             context.reenter = true;
             app.SetDocument(L"outer", root / L"outer.md");
             Require(!context.failed && WindowCaption(app.Window()) == L"nested-東京.md — RedTilde" && app.Document().GetText() == L"nested",
                     "a native notification may synchronously replace the document identity");
             before = app.Activity();
             app.SetDocument(L"fresh", root / L"outer.md");
             Require(app.Activity().captionCalls == before.captionCalls + 1 && WindowCaption(app.Window()) == L"outer.md — RedTilde",
                     "superseded native write cannot cache the stale outer identity as applied");
         });
    test("Editor.DirtyCacheTracksDirectResetAndSameLengthUndo",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create dirty-cache app");
             app.SetDocument(L"saved");
             Require(!app.Dirty(), "saved source starts clean");
             app.Document().SetText(L"other");
             Require(app.Dirty(), "same-length direct reset invalidates cached source");
             app.Document().SetText(L"saved");
             Require(!app.Dirty(), "direct reset to saved bytes is clean");
             app.Document().SetText(L"");
             Require(app.Dirty(), "length mismatch is dirty");
             app.Document().SetText(L"saved");
             app.Document().SetSelection({4, 5});
             Require(app.Document().ReplaceSelection(L"s") && app.Dirty(), "same-length transaction is dirty");
             Require(app.Document().Undo() && !app.Dirty(), "same-length undo to exact saved bytes is clean");
         });
    test("Editor.SemanticFormattingUndoAndTheme",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create formatting app");
             app.SetDocument(L"Hello world\n");
             app.Prepare();
             app.Document().SetSelection({0, 5});
             app.Execute(Command::Bold);
             Require(app.Document().GetText() == L"**Hello** world\n", "semantic bold preserves outside source");
             app.Execute(Command::Undo);
             Require(app.Document().GetText() == L"Hello world\n" && !app.Dirty(), "exact semantic undo");
             app.Execute(Command::ThemeDark);
             Require(app.Host().GetTheme().dark, "dark applies live");
             app.Execute(Command::ThemeLight);
             Require(!app.Host().GetTheme().dark, "light applies live");
             Require(app.Capture(root / L"test-compose.png"), "actual app capture");
         });
    test("Editor.LocalDiagramSourceAndLiteralFallback",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create local-source app");
             const std::wstring source = L"# Diagram\n\n```mermaid\ngraph LR\nA[Start] --> B[Done]\n```\n\n```mermaid\npie\ntitle Unsupported\n```\n";
             app.SetDocument(source);
             app.Prepare(256);
             const auto semantic = app.Document().GetSemanticDocument();
             Require(static_cast<bool>(semantic), "semantic publication exists");
             bool diagram{};
             for (const auto &element : semantic->Elements())
                 if (element.role == DxUi::RichText::SemanticRole::Diagram)
                     diagram = true;
             Require(diagram, "supported graph becomes native diagram");
             app.Document().SetSelection({30, 30});
             app.Execute(Command::LocalSource);
             app.Prepare();
             Require(app.Document().GetSemanticSourceRegion().has_value(), "local diagram source region entered");
             Require(app.Document().GetText() == source && !app.Dirty(), "local presentation preserves exact source/history");
             Require(app.Document().EndSemanticSourceEdit(app.Host()), "local source exits");
             Require(app.Document().GetText().find(L"pie\ntitle Unsupported") != std::wstring::npos, "unsupported family exact source retained");
         });
    test("Editor.LocalOrdinarySourceCommitCancelAndStale",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create local draft app");
             const std::wstring source = L"# Heading\n\nA **careful** paragraph.\n\nUntouched tail.\n";
             app.SetDocument(source);
             app.Prepare();
             const auto begin = source.find(L"A **");
             app.Document().SetSelection({begin, begin + 24});
             app.Execute(Command::LocalSource);
             Require(app.HasBlockDraft() && app.Document().IsReadOnly(), "ordinary source draft preserves rich document around it");
             app.BlockDraft()->SetText(L"Changed *locally*.");
             (void)SendMessageW(app.Window(), WM_CLOSE, 0, 0);
             Require(IsWindow(app.Window()) && app.HasBlockDraft(), "close protects an unapplied source draft");
             Require(app.Document().GetText() == source && !app.Dirty(), "draft does not mutate source before Apply");
             app.Execute(Command::ApplyBlock);
             Require(!app.HasBlockDraft() && !app.Document().IsReadOnly() && app.Dirty(), "Apply closes draft and changes source");
             Require(app.Document().GetText() == source.substr(0, begin) + L"Changed *locally*." + source.substr(begin + 24),
                     "only exact selected interval changes");
             Require(app.Document().Undo() && app.Document().GetText() == source, "one transaction restores exact source");
             app.Prepare();
             app.Document().SetSelection({begin, begin + 24});
             app.Execute(Command::LocalSource);
             app.BlockDraft()->SetText(L"Cancelled");
             app.Execute(Command::CloseDrawer);
             Require(app.Document().GetText() == source && !app.Dirty(), "cancel leaves source and dirty state unchanged");
             app.Prepare();
             app.Document().SetSelection({begin, begin + 24});
             app.Execute(Command::LocalSource);
             app.BlockDraft()->SetText(L"Stale replacement");
             app.Document().SetText(L"A different revision");
             app.Execute(Command::ApplyBlock);
             Require(app.Document().GetText() == L"A different revision" && app.HasBlockDraft(), "stale draft cannot replace a newer revision");
             app.Execute(Command::CloseDrawer);
         });
    test("Editor.DeviceLossAndLocalImagePixels",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create resource app");
             app.SetDocument(L"# Before device loss\n\nA retained document.\n");
             app.Prepare();
             const auto source = app.Document().GetText();
             app.Host().DebugSimulateDeviceLoss();
             app.Prepare();
             Require(app.Capture(root / L"after-device-loss.png") && app.Document().GetText() == source,
                     "device loss recreates pixels and retains source");
             app.SetDocument(L"# Image\n\n![Actual app capture](test-compose.png)\n", root / L"image-document.md");
             app.Prepare(512);
             const auto stats = app.Document().GetImageResourceStatistics();
             Require(stats.readySources == 1 && stats.decodedBytes > 0, "relative local image decoded through worker and admitted");
             Require(app.Capture(root / L"test-image.png"), "capture displays prepared local image");
         });
    test("Files.AsyncSaveRetainsEditsMadeDuringWrite",
         [&]
         {
             const auto path = root / L"async-save.md";
             FileStamp original;
             Success(SaveDocument(path, L"before", false, nullptr, original), "create async destination");
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create async save app");
             app.SetDocument(L"before", path, false, original);
             app.Document().SetSelection({6, 6});
             Require(app.Document().ReplaceSelection(L" saved"), "edit before Save");
             app.Execute(Command::Save);
             app.Document().SetSelection({12, 12});
             Require(app.Document().ReplaceSelection(L" later"), "edit during write");
             app.Prepare(512);
             StoredDocument loaded;
             Success(LoadDocument(path, loaded), "read asynchronously saved file");
             Require(loaded.text == L"before saved", "Save writes the admitted snapshot");
             Require(app.Document().GetText() == L"before saved later" && app.Dirty(), "later edits remain in editor and dirty");
         });
    test("Editor.InDocumentMathLocalSourceAndBroaderMermaid",
         [&]
         {
             using namespace DxUi::RichText;
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"math-product";
             Application app(launch);
             Success(app.Initialize(), "create native math app");
             const std::wstring source = L"# Mathematics\r\n\r\nInline $a_i^2$ stays in the line.\r\n\r\n"
                                         L"$$\\left(\\frac{1}{2}\\right)+\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}$$\r\n";
             app.SetDocument(source);
             app.Prepare(1024);
             auto semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(semantic && std::ranges::count(semantic->Elements(), SemanticRole::Math, &SemanticElement::role) == 2,
                     "both inline and display equations are native document objects");
             const auto first = source.find(L"$a_i^2$");
             app.Document().SetSelection({first + 2, first + 2});
             app.Execute(Command::LocalSource);
             Require(app.HasBlockDraft() && app.BlockDraft()->GetText() == L"$a_i^2$",
                     "F2 targets exact math delimiters rather than the whole paragraph");
             app.BlockDraft()->SetText(L"$a_i^3$");
             app.Execute(Command::ApplyBlock);
             Require(app.Document().GetText().find(L"$a_i^3$") != std::wstring::npos && app.Document().Undo() && app.Document().GetText() == source,
                     "local equation edit and undo preserve the rest and all original EOLs");
             app.Prepare(1024);
             app.Host().DebugSimulateDeviceLoss();
             app.Prepare(1024);
             Require(app.Document().GetAcknowledgedSemanticDocument(app.Host()) && app.Capture(root / L"math-device-loss.png"),
                     "math survives native device replacement");
             for (const auto body : {L"stateDiagram-v2\ndirection LR\n[*] --> Draft\nDraft --> Saved : Save\n",
                                     L"classDiagram\ndirection LR\nDocument <|-- Markdown\nMarkdown : +save()\n"})
             {
                 const auto diagram = L"```mermaid\n" + std::wstring(body) + L"```\n";
                 app.SetDocument(diagram);
                 app.Prepare(1024);
                 semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
                 Require(semantic && std::ranges::count(semantic->Elements(), SemanticRole::Diagram, &SemanticElement::role) == 1 &&
                             app.Document().GetText() == diagram,
                         "state/class graph renders without modifying source");
                 app.Document().SetSelection({20, 20});
                 app.Execute(Command::LocalSource);
                 Require(app.Document().GetSemanticSourceRegion().has_value(), "broader Mermaid uses native exact-source editing");
                 app.Execute(Command::LocalSource);
             }
             app.SetDocument(L"Unsupported $\\input{private}$ stays exact.\n");
             app.Prepare(1024);
             semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(semantic && !semantic->Diagnostics().empty() && semantic->Map().Display().find(L"$\\input{private}$") != std::wstring::npos,
                     "unsupported equation remains literal with diagnostics");
             const std::wstring inspector = L"Code `$fake$` and $a_i^2$ and \\(b^3\\).\n";
             app.SetDocument(inspector);
             app.Prepare(1024);
             const auto code = inspector.find(L"fake");
             app.Document().SetSelection({code, code});
             app.Document().changedSelection();
             Require(app.EquationPreviewText().empty(), "current syntax keeps inline code out of the equation inspector");
             for (const auto &notation : {std::wstring(L"a_i^2"), std::wstring(L"b^3")})
             {
                 const auto math = inspector.find(notation);
                 app.Document().SetSelection({math, math});
                 app.Document().changedSelection();
                 Require(app.EquationPreviewText() == L"\\[" + notation + L"\\]", "current syntax drives either math delimiter family");
             }
             const auto oversized = L"$" + std::wstring(65534u, L'x') + L"$";
             app.SetDocument(oversized);
             app.Prepare(1024);
             app.Document().SetSelection({65000, 65000});
             (void)app.Document().OnKeyDown(app.Host(), VK_LEFT, 0);
             app.Document().changedSelection(); // The same handler, without swallowing an unexpected callback exception.
             Require(app.Document().GetText() == oversized && app.EquationPreviewText().empty(),
                     "maximum-capacity oversized math stays source-exact and cannot overflow the inspector");
         });
    test("Recovery.LocalDraftCancelPreviewRestoreProtectsOriginal",
         [&]
         {
             const auto directory = root / L"recovery-product";
             std::filesystem::create_directories(directory);
             const auto original = directory / L"original.md";
             FileStamp identity;
             Success(SaveDocument(original, L"Original writing", true, nullptr, identity), "save original file");
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = directory;
             auto app = std::make_unique<Application>(launch);
             Success(app->Initialize(), "create recoverable draft app");
             app->SetDocument(L"Original writing", original, true, identity);
             app->Prepare();
             app->Document().SetSelection({0, 8});
             app->Execute(Command::LocalSource);
             app->BlockDraft()->SetText(L"Unapplied 東京 😀");
             app->CheckpointRecovery(true);
             auto settle = [&]
             {
                 for (unsigned turn = 0; turn < 1000 && !app->RecoverySettled(); ++turn)
                 {
                     app->Prepare();
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 Require(app->RecoverySettled(), "draft recovery settled");
             };
             settle();
             Require(app->Document().GetText() == L"Original writing" && !app->Dirty(), "checkpoint does not apply a local draft to live source");
             app->Execute(Command::CloseDrawer);
             settle();
             for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                 Require(entry.path().extension() != L".rtd", "cancelling a clean draft retires its proposed recovery");
             app->Execute(Command::LocalSource);
             app->BlockDraft()->SetText(L"Unapplied 東京 😀");
             app->CheckpointRecovery(true);
             settle();
             app.reset();
             // Wait for the stopped owner's lease to close, never join the UI or
             // steal a live session. A process crash closes the same lease in OS.
             for (unsigned turn = 0; turn < 1000; ++turn)
             {
                 bool leased{};
                 for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                     if (entry.path().extension() == L".lease")
                     {
                         wil::unique_handle probe(CreateFileW(entry.path().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr));
                         leased = leased || !probe;
                     }
                 if (!leased)
                     break;
                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
             }
             app = std::make_unique<Application>(launch);
             Success(app->Initialize(), "restart app with prior draft");
             app->Prepare(1024);
             Require(app->RecoveryCount() == 1, "one prior session available for review");
             app->Execute(Command::Recovery);
             app->Execute(Command::LaterRecovery);
             Require(app->RecoveryCount() == 1 && !app->Dirty(), "keeping for later preserves snapshot and current document");
             app->Execute(Command::RestoreRecovery);
             Require(app->Document().GetText() == L"Unapplied 東京 😀 writing" && app->Dirty() && app->RequiresSaveAs(),
                     "explicit restore produces a dirty copy including the unapplied draft");
             Require(app->Host().GetFocusControl() == &app->Document(), "restore returns logical keyboard routing to the editor");
             StoredDocument unchanged;
             Success(LoadDocument(original, unchanged), "read original after recovery");
             Require(unchanged.text == L"Original writing" && unchanged.bom, "recovery never overwrites original Markdown");
         });
    test("Recovery.DiscardFailureRetainsCandidateAndRetryClearsError",
         [&]
         {
             const auto directory = root / L"recovery-discard";
             std::filesystem::create_directories(directory / L"Recovery");
             const auto snapshot = directory / L"Recovery" / L"session-fedcba9876543210fedcba9876543210.rtd";
             Success(WriteBytesAtomic(snapshot, SerializeRecovery({L"Retained writing 東京", {}, false, {0, 0}, 1})), "seed inactive recovery");
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = directory;
             Application app(launch);
             Success(app.Initialize(), "open recovery review");
             app.Prepare(1024);
             Require(app.RecoveryCount() == 1, "inactive candidate available");
             const auto current = app.Document().GetText();
             app.Execute(Command::Recovery);
             wil::unique_handle locked(CreateFileW(snapshot.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
             Require(static_cast<bool>(locked), "prevent candidate deletion");
             auto settle = [&]
             {
                 for (unsigned turn = 0; turn < 1000 && !app.RecoverySettled(); ++turn)
                 {
                     app.Prepare();
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 app.Prepare();
                 Require(app.RecoverySettled(), "discard worker settled");
             };
             app.Execute(Command::DiscardRecovery);
             settle();
             Require(app.RecoveryCount() == 1 && FAILED(app.RecoveryStatus()) && std::filesystem::exists(snapshot),
                     "failed discard retains the reviewable candidate and reports failure");
             locked.reset();
             app.Execute(Command::DiscardRecovery);
             settle();
             Require(app.RecoveryCount() == 0 && SUCCEEDED(app.RecoveryStatus()) && !std::filesystem::exists(snapshot),
                     "successful retry removes the candidate and clears the operation error");
             Require(app.Document().GetText() == current && !app.Dirty(), "discard never replaces the active document");
             Require(app.Host().GetFocusControl() == &app.Document(), "discarding the final snapshot returns logical focus to the editor");
         });
    test("Performance.NativeCompletedFramesPreserveSourceAndHistory",
         [&]
         {
             for (const auto *view : {L"compose", L"split"})
             {
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.scenario = view;
                 launch.performanceCycles = 4;
                 launch.controlDiagnostics = true;
                 launch.dataDirectory = root / (L"completed-" + std::wstring(view));
                 launch.performance = launch.dataDirectory / L"native.json";
                 Application app(launch);
                 Success(app.Initialize(), "create completed-frame app");
                 const std::wstring source = L"# Native frames\n\nS5_EDIT_HERE 東京 😀 and $a_i^2$.\n\n$$\\frac{1}{2}$$\n";
                 app.SetDocument(source);
                 Require(RunPerformanceProbe(app, launch) == 0, "native completed-frame protocol succeeds");
                 Require(app.Document().GetText() == source && !app.Dirty() && !app.Document().CanUndo(),
                         "completed frame cycles restore clean source and undo history");
                 Require(app.Document().GetAcknowledgedSemanticDocument(app.Host()) != nullptr,
                         "completed frame has acknowledged current native semantic geometry");
                 Require(app.RecoverySettled() && SUCCEEDED(app.RecoveryStatus()), "memory sampling waits for successful recovery settlement");
                 std::string probeReport;
                 Success(ReadBytes(launch.performance, probeReport), "read completed-frame diagnostics");
                 Require(probeReport.find("\"schemaVersion\":2") != probeReport.npos && probeReport.find("\"threads\":null") != probeReport.npos &&
                             probeReport.find("\"commandOwnerCpuCycles\":[") != probeReport.npos &&
                             probeReport.find("\"ownerProcessorStart\":[") != probeReport.npos &&
                             probeReport.find("\"recoverySettledAtMemory\":true") != probeReport.npos,
                         "latency protocol separates cheap settled memory, command CPU and processor-boundary observations");
                 Require(probeReport.find("\"protocol\":\"app-edit-phases-v2\"") != probeReport.npos &&
                             probeReport.find("\"name\":\"chrome\"") != probeReport.npos &&
                             probeReport.find("\"postMemory\":[") != probeReport.npos && probeReport.find("\"renderCounts\":[") != probeReport.npos,
                         "bounded command phases and post-primary quiescence observations are retained");
             }
         });
    test("Rendering.NativeDefaultAndWarpResourceLifecycle",
         [&]
         {
             for (const bool warp : {false, true})
             {
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.forceWarp = warp;
                 launch.resourceProbe = true;
                 launch.performanceCycles = 4;
                 launch.dataDirectory = root / (warp ? L"resource-warp" : L"resource-default");
                 launch.performance = launch.dataDirectory / L"native.json";
                 Application app(launch);
                 Success(app.Initialize(), "create native resource app");
                 app.SetDocument(L"# Resource lifecycle\n\nS5_EDIT_HERE and $a_i^2$.\n\n$$\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}$$\n\n"
                                 L"```mermaid\nstateDiagram-v2\n[*] --> Draft\nDraft --> Saved : Save\n```\n");
                 Require(RunPerformanceProbe(app, launch) == 0, "actual native resource, image, resize and device-loss protocol succeeds");
                 Require(app.Window() == nullptr && app.Host().GetHwnd() == nullptr, "resource protocol retires its window and native host");
             }
         });
    report << "\n  ],\n  \"passed\": " << passed << ", \"failed\": " << failed << ", \"selected\": " << selected << ", \"registered\": " << registered
           << ", \"catalogOnly\": " << (listOnly ? "true" : "false") << "\n}\n";
    return failed || selected == 0 ? 1 : 0;
}
} // namespace RedTilde
