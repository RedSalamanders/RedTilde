#include "RedTilde/Application.h"
#include "../res/Resource.h"
#include "Application.Tests.DesignRefresh.h"
#include "Application.Tests.Files.h"
#include "Application.Tests.HeapDiagnostic.h"
#include "Application.Tests.RemoteImages.h"
#include "Support/Support.Tests.TextServices.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

namespace RedTilde
{
using DxUi::RichText::ParseMath;
namespace
{
using Testing::TextServiceImportProbe;

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
// Inspect the compact native report's selected members without adding a product JSON dependency.
std::string_view JsonObjectAt(std::string_view source, std::size_t open)
{
    Require(open < source.size() && source[open] == '{', "resource probe JSON object starts");
    unsigned depth{};
    bool quoted{}, escaped{};
    for (std::size_t index = open; index < source.size(); ++index)
    {
        const char value = source[index];
        if (quoted)
        {
            if (escaped)
                escaped = false;
            else if (value == '\\')
                escaped = true;
            else if (value == '"')
                quoted = false;
            continue;
        }
        if (value == '"')
            quoted = true;
        else if (value == '{')
            ++depth;
        else if (value == '}' && --depth == 0)
            return source.substr(open, index - open + 1);
    }
    throw std::runtime_error("resource probe JSON object is terminated");
}
std::string_view JsonObjectAfter(std::string_view source, std::string_view marker)
{
    const auto markerAt = source.find(marker);
    Require(markerAt != source.npos, "resource probe JSON object marker is present");
    const auto open = source.find_first_not_of(" \t\r\n", markerAt + marker.size());
    return JsonObjectAt(source, open);
}
std::string_view JsonObjectContaining(std::string_view source, std::size_t markerAt)
{
    Require(markerAt < source.size(), "resource probe JSON object member is present");
    std::array<std::size_t, 32> objects{};
    std::size_t depth{};
    bool quoted{}, escaped{};
    for (std::size_t index = 0; index < markerAt; ++index)
    {
        const char value = source[index];
        if (quoted)
        {
            if (escaped)
                escaped = false;
            else if (value == '\\')
                escaped = true;
            else if (value == '"')
                quoted = false;
            continue;
        }
        if (value == '"')
            quoted = true;
        else if (value == '{')
        {
            Require(depth < objects.size(), "resource probe JSON nesting is bounded");
            objects[depth++] = index;
        }
        else if (value == '}')
        {
            Require(depth > 0, "resource probe JSON braces are balanced");
            --depth;
        }
    }
    Require(depth > 0, "resource probe member is inside an object");
    return JsonObjectAt(source, objects[depth - 1]);
}
std::string_view JsonNumberToken(std::string_view object, std::string_view field)
{
    const auto marker = std::format("\"{}\":", field);
    const auto at = object.find(marker);
    Require(at != object.npos, "resource probe numeric field is present");
    auto value = object.substr(at + marker.size());
    const auto end = value.find_first_of(",}");
    Require(end != value.npos, "resource probe numeric member is delimited");
    value = value.substr(0, end);
    const auto begin = value.find_first_not_of(" \t\r\n");
    Require(begin != value.npos, "resource probe numeric member is nonempty");
    value.remove_prefix(begin);
    return value.substr(0, value.find_last_not_of(" \t\r\n") + 1);
}
std::uint64_t JsonUnsigned(std::string_view object, std::string_view field)
{
    const auto token = JsonNumberToken(object, field);
    std::uint64_t result{};
    const auto [parsed, error] = std::from_chars(token.data(), token.data() + token.size(), result);
    Require(error == std::errc{} && parsed == token.data() + token.size(), "resource probe unsigned field is valid");
    return result;
}
double JsonPositiveFinite(std::string_view object, std::string_view field)
{
    const auto token = JsonNumberToken(object, field);
    double result{};
    const auto [parsed, error] = std::from_chars(token.data(), token.data() + token.size(), result);
    Require(!token.empty() && error == std::errc{} && parsed == token.data() + token.size() && std::isfinite(result) && result > 0,
            "resource probe timing field is positive and finite");
    return result;
}
void RequireCompletedResourceCost(std::string_view cost, std::string_view description)
{
    (void)JsonPositiveFinite(cost, "completedWallUs");
    (void)JsonUnsigned(cost, "processCpu100ns");
    (void)JsonUnsigned(cost, "ownerCpuCycles");
    const auto flushes = JsonUnsigned(cost, "rootChildFlushes");
    Require(flushes > 0 && flushes <= 8, description);
}
void RequireResourcePhase(std::string_view report, std::string_view phase, bool completed)
{
    const auto marker = std::format("\"phase\":\"{}\"", phase);
    const auto entry = JsonObjectContaining(report, report.find(marker));
    if (completed)
        RequireCompletedResourceCost(JsonObjectAfter(entry, "\"preparation\":"), "reset frame flush count is in the bounded root-child range");
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
    RegisterFilesTests(root, test);
    RegisterDesignRefreshTests(root, test);
    RegisterRemoteImageTests(root, test);
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
             const auto exactSourceLimit = L"\\text{" + std::wstring(DxUi::RichText::MathMaximumSourceUnits - 7u, L'x') + L"}";
             Require(exactSourceLimit.size() == DxUi::RichText::MathMaximumSourceUnits && static_cast<bool>(ParseMath(exactSourceLimit)),
                     "the exact source-unit limit is admitted");
             Require(!ParseMath(std::wstring(4097, L'x')), "capacity bounded");
             Require(!ParseMath(std::wstring(30, L'{') + L"x" + std::wstring(30, L'}')), "recursion bounded");
             const auto text = ParseMath(L"\\text{two words}");
             Require(text && text.root.children.size() == 1 && text.root.children.front().kind == DxUi::RichText::MathKind::Text &&
                         text.root.children.front().text == L"two words" && !text.root.children.front().italic,
                     "text mode retains spaces and upright text semantics");
             const auto limits = ParseMath(L"\\sum\\limits_{i=0}^{n} i + y = z");
             Require(limits && !limits.root.children.empty() && limits.root.children.front().kind == DxUi::RichText::MathKind::Scripts &&
                         !limits.root.children.front().children.empty(),
                     "large operator scripts retain a typed base");
             const auto &sum = limits.root.children.front().children.front();
             Require(sum.atomClass == DxUi::RichText::MathAtomClass::Operator && sum.largeOperator && sum.displayLimits &&
                         sum.limits == DxUi::RichText::MathLimits::AboveBelow,
                     "explicit limits classify and place a large operator as an operator");
             const auto noLimits = ParseMath(L"\\sum\\nolimits_{i=0}^{n}");
             Require(noLimits && noLimits.root.children.front().children.front().limits == DxUi::RichText::MathLimits::Side,
                     "nolimits remains a typed side-script placement");
             const auto displayStyle = ParseMath(L"\\displaystyle x");
             const auto scriptStyle = ParseMath(L"\\scriptstyle x");
             Require(displayStyle && scriptStyle && displayStyle.root.children.front().kind == DxUi::RichText::MathKind::Style &&
                         displayStyle.root.children.front().text == L"displaystyle" &&
                         scriptStyle.root.children.front().kind == DxUi::RichText::MathKind::Style &&
                         scriptStyle.root.children.front().text == L"scriptstyle",
                     "display and script styles remain explicit typed nodes");
             const auto operators = ParseMath(L"x + y = z");
             const auto hasClass = [&](std::wstring_view value, DxUi::RichText::MathAtomClass atomClass)
             {
                 return std::ranges::any_of(operators.root.children,
                                            [&](const auto &node) { return node.text == value && node.atomClass == atomClass; });
             };
             Require(operators && hasClass(L"+", DxUi::RichText::MathAtomClass::Binary) && hasClass(L"=", DxUi::RichText::MathAtomClass::Relation),
                     "binary and relation atoms keep distinct spacing classes");
             const auto matrixSource = [](unsigned rows)
             {
                 std::wstring value = L"\\begin{matrix}";
                 for (unsigned row = 0; row < rows; ++row)
                 {
                     if (row)
                         value += L"\\\\";
                     for (unsigned column = 0; column < 16; ++column)
                     {
                         if (column)
                             value += L'&';
                         value += L'x';
                     }
                 }
                 value += L"\\end{matrix}";
                 return value;
             };
             Require(static_cast<bool>(ParseMath(matrixSource(8))) && !ParseMath(matrixSource(9)),
                     "the 128-cell matrix budget admits its boundary and rejects excess cells");
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
             const auto committed = app.Activity();
             const auto committedSource = app.Document().Snapshot();
             const auto committedSelection = app.Document().GetSelection();
             app.Prepare();
             const auto prepared = app.Activity();
             Require(prepared.syntaxPresentationUpdates > committed.syntaxPresentationUpdates &&
                         prepared.selectionPresentationUpdates == committed.selectionPresentationUpdates &&
                         prepared.mirrorSelectionUpdates == committed.mirrorSelectionUpdates,
                     "new syntax refreshes context without repeating unchanged mirror selection or position facts");
             Require(app.Document().Snapshot().IsSameRevision(committedSource) && app.Document().GetSelection() == committedSelection &&
                         app.SourceMirror().GetSelection() == committedSelection && app.SourceMirror().GetText() == app.Document().GetText() &&
                         app.Document().CanUndo(),
                     "syntax publication preserves exact editable and mirror source, ACP selection and history");
             Require(app.Document().Undo() && app.Document().Undo(), "undo both edits");
             Require(app.SourceMirror().GetText() == source && !app.Dirty(), "Split undo restores the exact saved source in both panes");

             struct ImeMirrorFixture
             {
                 std::wstring_view name;
                 std::wstring source;
                 DxUi::RichText::Selection selection;
                 std::wstring composition;
                 std::size_t cursorInComposition;
             };
             const std::array<ImeMirrorFixture, 3> fixtures{{
                 {L"ime-preview-growth", L"abc", {3, 3}, L"X\U0001F600\r\nZ", 6},
                 {L"ime-preview-emoji-replacement", L"\U0001F600z", {0, 2}, L"ab", 1},
                 {L"ime-preview-crlf-replacement", L"\r\nz", {0, 2}, L"ab", 1},
             }};
             for (const auto &fixture : fixtures)
             {
                 const HWND foregroundBefore = GetForegroundWindow();
                 HWND appWindow{};
                 {
                     LaunchOptions imeLaunch;
                     imeLaunch.headless = true;
                     imeLaunch.dataDirectory = root / std::wstring(fixture.name);
                     Application imeApp(imeLaunch);
                     Success(imeApp.Initialize(), "initialize hidden text-service mirror app");
                     appWindow = imeApp.Window();
                     Require(appWindow && !IsWindowVisible(appWindow), "text-service mirror window remains hidden");
                     imeApp.SetDocument(fixture.source);
                     imeApp.SetView(EditorView::Split);
                     imeApp.Prepare();
                     Require(imeApp.SourceMirror().GetText() == fixture.source, "Split begins with the exact fixture source");

                     // Seed one redo record without changing the fixture's source bytes.
                     auto seededText = fixture.source;
                     const auto lastUnit = seededText.size() - 1;
                     imeApp.Document().SetSelection({lastUnit, seededText.size()});
                     Require(imeApp.Document().ReplaceSelection(L"q"), "seed one reversible source edit");
                     seededText.back() = L'q';
                     Require(imeApp.Document().Undo() && imeApp.Document().GetText() == fixture.source && imeApp.Document().CanRedo(),
                             "restore fixture source while retaining a redo record");
                     imeApp.Document().SetSelection(fixture.selection);
                     Require(imeApp.SourceMirror().GetText() == fixture.source && imeApp.SourceMirror().GetSelection() == fixture.selection,
                             "seeded history leaves the hidden mirror at the fixture source and ACPs");

                     const auto historyCanUndo = imeApp.Document().CanUndo();
                     const auto historyCanRedo = imeApp.Document().CanRedo();
                     const auto beforePreview = imeApp.Activity();
                     auto previewText = fixture.source;
                     previewText.replace(fixture.selection.Start(), fixture.selection.End() - fixture.selection.Start(), fixture.composition);
                     const DxUi::RichText::Selection previewSelection{fixture.selection.Start() + fixture.cursorInComposition,
                                                                      fixture.selection.Start() + fixture.cursorInComposition};
                     DxUi::TextInputState entry;
                     entry.text = fixture.source;
                     entry.multiline = true;
                     entry.caretIndex = fixture.selection.caret;
                     if (fixture.selection.anchor != fixture.selection.caret)
                         entry.selectionAnchorIndex = fixture.selection.anchor;
                     auto preview = entry;
                     preview.text = previewText;
                     preview.caretIndex = previewSelection.caret;
                     preview.selectionAnchorIndex.reset();
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), preview, false),
                             "canonical text-service preview is accepted on the attached editor");
                     Require(imeApp.Document().GetText() == previewText, "text-service preview changes the live source view");
                     Require(imeApp.SourceMirror().GetText() == previewText, "Split mirror synchronizes the live IME preview text");
                     Require(imeApp.Document().GetSelection() == previewSelection && imeApp.SourceMirror().GetSelection() == previewSelection,
                             "Split mirror and source expose the same preview ACP");
                     Require(imeApp.Activity().changes == beforePreview.changes &&
                                 imeApp.Activity().recoverySubmissions == beforePreview.recoverySubmissions,
                             "silent IME preview does not publish a document edit or recovery snapshot");
                     Require(imeApp.Document().CanUndo() == historyCanUndo && imeApp.Document().CanRedo() == historyCanRedo,
                             "silent IME preview does not alter seeded undo or redo history");

                     auto nextPreview = preview;
                     nextPreview.text[fixture.selection.Start()] = L'Y';
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), nextPreview, false),
                             "successive same-ACP text-service candidate is accepted");
                     Require(imeApp.Document().GetText() == nextPreview.text && imeApp.SourceMirror().GetText() == nextPreview.text &&
                                 imeApp.Document().GetSelection() == previewSelection && imeApp.SourceMirror().GetSelection() == previewSelection,
                             "successive text-service candidate synchronizes its source even with unchanged ACPs");
                     Require(imeApp.Activity().changes == beforePreview.changes &&
                                 imeApp.Activity().recoverySubmissions == beforePreview.recoverySubmissions &&
                                 imeApp.Document().CanUndo() == historyCanUndo && imeApp.Document().CanRedo() == historyCanRedo,
                             "successive same-ACP preview preserves committed activity and history");

                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), entry, false),
                             "canonical text-service cancellation restores the entry state");
                     Require(imeApp.Document().GetText() == fixture.source && imeApp.Document().GetSelection() == fixture.selection,
                             "IME cancellation restores the exact original source and selection");
                     Require(imeApp.SourceMirror().GetText() == fixture.source && imeApp.SourceMirror().GetSelection() == fixture.selection,
                             "Split mirror follows IME cancellation text and ACPs");
                     Require(imeApp.Activity().changes == beforePreview.changes &&
                                 imeApp.Activity().recoverySubmissions == beforePreview.recoverySubmissions &&
                                 imeApp.Document().CanUndo() == historyCanUndo && imeApp.Document().CanRedo() == historyCanRedo,
                             "IME cancellation preserves activity, recovery count and seeded history");
                     Require(imeApp.Document().Redo() && imeApp.Document().GetText() == seededText, "redo remains usable after IME cancellation");
                     Require(imeApp.Document().Undo() && imeApp.Document().GetText() == fixture.source,
                             "undo restores the fixture source after the preserved redo");

                     // Re-run the same composition as a result commit. It must be one source transaction.
                     imeApp.Document().SetSelection(fixture.selection);
                     const auto beforeCommit = imeApp.Activity();
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), preview, false),
                             "canonical preview before commit is accepted");
                     Require(imeApp.Document().GetText() == previewText && imeApp.SourceMirror().GetText() == previewText &&
                                 imeApp.Activity().changes == beforeCommit.changes &&
                                 imeApp.Activity().recoverySubmissions == beforeCommit.recoverySubmissions,
                             "preview before result remains synchronized without committing source or recovery");
                     // The native/TSF host restores its entry text before the
                     // notifying result import, so history records one edit.
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), entry, false),
                             "canonical commit protocol restores entry without notification");
                     auto result = preview;
                     result.caretIndex = fixture.selection.Start() + fixture.composition.size();
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), result, true), "canonical result commit is accepted");
                     Require(imeApp.Document().GetText() == previewText && imeApp.SourceMirror().GetText() == previewText,
                             "committed IME result synchronizes source and Split mirror");
                     Require(imeApp.SourceMirror().GetSelection() == imeApp.Document().GetSelection() &&
                                 imeApp.Document().GetSelection().caret == fixture.selection.Start() + fixture.composition.size(),
                             "committed IME result leaves both panes at the result's exact ending ACP");
                     Require(imeApp.Activity().changes == beforeCommit.changes + 1 &&
                                 imeApp.Activity().recoverySubmissions == beforeCommit.recoverySubmissions + 1,
                             "IME result is exactly one notifying edit and recovery submission");
                     Require(imeApp.Document().CanUndo() && !imeApp.Document().CanRedo(),
                             "IME result creates one undo record and replaces the redo branch");
                     Require(imeApp.Document().Undo() && imeApp.Document().GetText() == fixture.source &&
                                 imeApp.SourceMirror().GetText() == fixture.source,
                             "one undo restores committed IME source in both panes");
                     Require(imeApp.Document().Redo() && imeApp.Document().GetText() == previewText && imeApp.SourceMirror().GetText() == previewText,
                             "redo restores committed IME source in both panes");
                     Require(!IsWindowVisible(appWindow) && GetForegroundWindow() == foregroundBefore,
                             "hidden IME regression never shows or activates its app window");
                 }
                 Require(GetForegroundWindow() == foregroundBefore, "IME regression restores the original foreground window");
             }
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
             std::function<DxUi::Tree *(DxUi::Control *)> findTree = [&](DxUi::Control *control) -> DxUi::Tree *
             {
                 if (auto *tree = dynamic_cast<DxUi::Tree *>(control); tree && tree->GetAccessibleName() == L"Document outline")
                     return tree;
                 for (std::size_t i = 0; i < control->GetLogicalChildCount(); ++i)
                     if (auto *tree = findTree(control->GetLogicalChild(i)))
                         return tree;
                 return nullptr;
             };
             auto *target = findTree(app.Host().GetRoot());
             Require(target && target->GetModel(), "semantic outline tree is present");
             auto row = [&](std::wstring_view name) -> std::optional<std::size_t>
             {
                 for (std::size_t i = 0; i < target->GetModel()->GetVisibleItemCount(); ++i)
                 {
                     DxUi::TreeItemData item;
                     target->GetModel()->GetVisibleItem(i, item);
                     if (item.text == name)
                         return i;
                 }
                 return {};
             };
             Require(row(L"Navigation target").has_value(), "outline target is present");
             const auto before = app.Activity();
             const auto body = source.find(L"Body");
             app.Document().SetSelection({body, body});
             Require(app.Document().ReplaceSelection(L"Expanded "), "shift a later heading with prose");
             Require(app.Activity().layouts == before.layouts, "offset-only changes preserve chrome layout");
             Require(target->RequestSelectVisibleItem(row(L"Navigation target").value()) && target->OnKeyDown(app.Host(), VK_RETURN, 0),
                     "invoke outline without taking desktop focus");
             const auto heading = app.Document().GetText().find(L"## Navigation target");
             Require(app.Document().GetSelection() == DxUi::RichText::Selection{heading, heading}, "outline navigation follows the new offset");
             const auto label = heading + 3;
             app.Document().SetSelection({label, label + std::wstring_view(L"Navigation target").size()});
             Require(app.Document().ReplaceSelection(L"Renamed target"), "rename the heading");
             Require(row(L"Renamed target").has_value() && !row(L"Navigation target").has_value() && app.Activity().layouts == before.layouts,
                     "changed heading text refreshes visible rows without recomputing window geometry");
             Require(target->RequestSelectVisibleItem(row(L"Renamed target").value()) && target->OnKeyDown(app.Host(), VK_RETURN, 0) &&
                         app.Document().GetSelection() == DxUi::RichText::Selection{heading, heading},
                     "the renamed outline action retains the exact current source offset");
             DxUi::TreeItemData renamed;
             target->GetModel()->GetVisibleItem(row(L"Renamed target").value(), renamed);
             const auto indented = renamed.depth;
             app.Document().SetSelection({heading, heading + 1});
             Require(app.Document().ReplaceSelection(L""), "change heading level");
             target->GetModel()->GetVisibleItem(row(L"Renamed target").value(), renamed);
             Require(renamed.depth < indented, "changed heading level updates outline indentation");
             Require(app.Document().Undo() && app.Document().Undo() && app.Document().Undo() && !app.Dirty(),
                     "outline edits retain exact multi-step undo");
             Require(app.Document().GetText() == source && row(L"Navigation target").has_value() &&
                         target->RequestSelectVisibleItem(row(L"Navigation target").value()) && target->OnKeyDown(app.Host(), VK_RETURN, 0) &&
                         app.Document().GetSelection() ==
                             DxUi::RichText::Selection{source.find(L"## Navigation target"), source.find(L"## Navigation target")},
                     "undo restores the exact heading label and navigation offset after committed analysis");
             Require(app.Document().Redo() && app.Document().Redo() && app.Document().Redo() && row(L"Renamed target").has_value(),
                     "redo republishes the offset, label and level changes through the same analysis path");
             const auto replayedHeading = app.Document().GetText().find(L"# Renamed target");
             target->GetModel()->GetVisibleItem(row(L"Renamed target").value(), renamed);
             Require(renamed.depth < indented && target->RequestSelectVisibleItem(row(L"Renamed target").value()) &&
                         target->OnKeyDown(app.Host(), VK_RETURN, 0) &&
                         app.Document().GetSelection() == DxUi::RichText::Selection{replayedHeading, replayedHeading} && app.Document().Undo() &&
                         app.Document().Undo() && app.Document().Undo() && app.Document().GetText() == source && !app.Dirty(),
                     "replayed heading facts and their subsequent undo remain exact without rebuilding fixed regions");
             app.Document().SetSelection({source.size(), source.size()});
             Require(app.Document().ReplaceSelection(L"\n### Added heading\n"), "add another heading");
             Require(row(L"Added heading").has_value(), "added heading becomes an outline row");
             Require(app.Document().Undo() && !row(L"Added heading").has_value() && !app.Dirty(),
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
                 std::filesystem::path nestedPath, nestedClosePath;
                 HWND window{};
                 bool reject{}, reenter{}, closeAfterCaption{}, failed{}, cleanBeforeClose{}, closeDispatched{}, hostDetachedDuringCallback{};
                 LRESULT rejection = FALSE;
             } context{&app, {}, root / L"nested-東京.md"};
             context.window = app.Window();
             context.nestedClosePath = root / L"nested-close.md";
             constexpr auto property = L"RedTilde.CaptionTest";
             Require(SetPropW(app.Window(), property, &context) != FALSE, "bind owned hidden-window interception");
             const auto restore = wil::scope_exit(
                 [&]
                 {
                     if (IsWindow(context.window))
                     {
                         if (context.previous)
                             (void)SetWindowLongPtrW(context.window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(context.previous));
                         (void)RemovePropW(context.window, property);
                     }
                 });
             WNDPROC intercept = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept -> LRESULT
             {
                 auto &state = *static_cast<Interception *>(GetPropW(hwnd, L"RedTilde.CaptionTest"));
                 if (message == WM_SETTEXT && std::exchange(state.reject, false))
                     return state.rejection;
                 const bool reenter = message == WM_SETTEXT && std::exchange(state.reenter, false);
                 const bool closeAfterCaption = message == WM_SETTEXT && std::exchange(state.closeAfterCaption, false);
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
                 if (closeAfterCaption)
                 {
                     try
                     {
                         state.app->SetDocument(L"saved before nested close", state.nestedClosePath);
                         state.cleanBeforeClose = !state.app->Dirty();
                         (void)SendMessageW(hwnd, WM_CLOSE, 0, 0);
                         state.closeDispatched = true;
                         state.hostDetachedDuringCallback = state.app->Host().GetHwnd() == nullptr && state.app->Host().GetRoot() == nullptr;
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
             const HWND hiddenWindow = app.Window();
             const HWND foreground = GetForegroundWindow();
             context.closeAfterCaption = true;
             app.Document().SetSelection({0, 0});
             Require(app.Document().ReplaceSelection(L"trigger"), "caption-triggered nested-close edit is admitted");
             Require(!context.failed && context.cleanBeforeClose && context.closeDispatched && context.hostDetachedDuringCallback,
                     "the owned WM_SETTEXT callback marks source clean and synchronously detaches the host on accepted Close");
             Require(app.Host().GetHwnd() == nullptr && app.Host().GetRoot() == nullptr,
                     "nested Close retires the controls before WM_SETTEXT resumes");
             Require((app.Window() == nullptr && !IsWindow(hiddenWindow)) ||
                         (app.Window() == hiddenWindow && IsWindow(hiddenWindow) && !IsWindowVisible(hiddenWindow)),
                     "accepted Close destroys the owned window or retains it hidden while admitted persistence finishes");
             Require(GetForegroundWindow() == foreground, "hidden native-close regression preserves foreground focus");
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
             auto requireCached = [&](bool dirty)
             {
                 Require(app.Dirty() == dirty, "exact content determines dirty state");
                 const auto comparisons = app.Activity().dirtyTextComparisons;
                 Require(app.Dirty() == dirty && app.Dirty() == dirty && app.Activity().dirtyTextComparisons == comparisons,
                         "repeated dirty queries reuse one immutable-source fact");
             };
             requireCached(false);
             auto before = app.Activity();
             app.CheckpointRecovery();
             app.CheckpointRecovery();
             Require(app.Activity().recoverySubmissions == before.recoverySubmissions + 2 &&
                         app.Activity().recoveryTextCopies == before.recoveryTextCopies &&
                         app.Activity().dirtyTextComparisons == before.dirtyTextComparisons,
                     "clean checkpoints still submit retirement without copying or comparing source again");
             app.Document().SetText(L"other");
             Require(app.Dirty(), "same-length direct reset invalidates cached source");
             Require(app.Activity().dirtyTextComparisons == before.dirtyTextComparisons + 1,
                     "same-length direct reset performs a new exact comparison");
             requireCached(true);
             app.Document().SetText(L"saved");
             Require(!app.Dirty(), "direct reset to saved bytes is clean");
             requireCached(false);
             before = app.Activity();
             app.Document().SetText(L"");
             Require(app.Dirty(), "length mismatch is dirty");
             Require(app.Activity().dirtyTextComparisons == before.dirtyTextComparisons, "length mismatch retains the comparison short circuit");
             requireCached(true);
             app.Document().SetText(L"saved");
             app.Document().SetSelection({4, 5});
             Require(app.Document().ReplaceSelection(L"s") && app.Dirty(), "same-length transaction is dirty");
             Require(app.Document().Undo() && !app.Dirty(), "same-length undo to exact saved bytes is clean");
             requireCached(false);
             before = app.Activity();
             app.CheckpointRecovery();
             Require(app.Activity().recoveryTextCopies == before.recoveryTextCopies, "clean undo checkpoint does not copy text before retirement");
             Require(app.Document().Redo() && app.Dirty(), "same-length redo remains dirty");
             requireCached(true);
             Require(app.Document().Undo() && !app.Dirty(), "second undo restores saved content");
             app.Document().SetSelection({5, 5});
             const auto canUndo = app.Document().CanUndo(), canRedo = app.Document().CanRedo();
             DxUi::TextInputState entry;
             entry.text = L"saved";
             entry.multiline = true;
             entry.caretIndex = 5;
             auto preview = entry;
             preview.text = L"saver";
             before = app.Activity();
             Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), preview, false) && app.Dirty(),
                     "silent same-length same-ACP preview invalidates the dirty fact");
             requireCached(true);
             Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), entry, false) && !app.Dirty(),
                     "silent cancellation restores exact saved content");
             requireCached(false);
             Require(app.Document().GetText() == entry.text && app.Document().GetSelection() == DxUi::RichText::Selection{5, 5} &&
                         app.Document().CanUndo() == canUndo && app.Document().CanRedo() == canRedo && app.Activity().changes == before.changes &&
                         app.Activity().recoverySubmissions == before.recoverySubmissions,
                     "dirty queries preserve preview/cancel source, selection, history and committed activity");
             app.SetDocument(L"other");
             requireCached(false);
             app.Document().SetText(L"saved");
             requireCached(true);
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
             app.Execute(Command::Save);
             Require(app.Dirty(), "in-flight save retains the previous dirty baseline");
             const auto admitted = app.Document().Snapshot();
             app.Prepare(512);
             Success(LoadDocument(path, loaded), "read second asynchronously saved file");
             Require(loaded.text == L"before saved later" && app.Document().Snapshot().IsSameRevision(admitted) && !app.Dirty(),
                     "save completion invalidates dirty facts even when live source did not change");
             const auto comparisons = app.Activity().dirtyTextComparisons;
             Require(!app.Dirty() && app.Activity().dirtyTextComparisons == comparisons, "the new saved baseline reuses its exact clean fact");
             Require(app.Document().Undo() && app.Dirty(), "undo away from the newly saved baseline is dirty");
             Require(app.Document().Redo() && !app.Dirty(), "redo to the newly saved baseline is clean");
             app.Document().SetSelection({0, 1});
             Require(app.Document().ReplaceSelection(L"B") && app.Dirty(), "admit a same-length save candidate");
             app.Execute(Command::Save);
             app.Document().SetSelection({0, 1});
             Require(app.Document().ReplaceSelection(L"b") && !app.Dirty(), "same-length in-flight edit can return to the previous saved bytes");
             app.Prepare(512);
             Success(LoadDocument(path, loaded), "read same-length asynchronously saved file");
             Require(loaded.text == L"Before saved later" && app.Document().GetText() == L"before saved later" && app.Dirty(),
                     "save completion invalidates a cached clean fact against the old baseline");
             Require(app.Document().Undo() && !app.Dirty(), "undo to the admitted save bytes is clean");
             Require(app.Document().Redo() && app.Dirty(), "redo to the in-flight edit is dirty");
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
             Require(app.EquationPreviewText().empty(), "current syntax keeps inline code out of the equation inspector");
             for (const auto &notation : {std::wstring(L"a_i^2"), std::wstring(L"b^3")})
             {
                 const auto math = inspector.find(notation);
                 app.Document().SetSelection({math, math});
                 Require(app.EquationPreviewText() == L"\\[" + notation + L"\\]", "current syntax drives either math delimiter family");
             }
             const std::wstring spacedSlash = L"**kept** \\( \\text{two words} + x \\) and [parent](https://example.test \"Parent title\")\n";
             app.SetDocument(spacedSlash);
             app.Prepare(1024);
             semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(semantic && std::ranges::count(semantic->Elements(), SemanticRole::Math, &SemanticElement::role) == 1 &&
                         semantic->Diagnostics().empty() &&
                         std::ranges::any_of(semantic->Syntax()->Inlines(), [](const auto &node) { return node.kind == MarkdownInlineKind::Strong; }),
                     "whitespace-delimited slash math and neighboring rich syntax share one source publication");
             const auto parentLink =
                 std::ranges::find_if(semantic->Elements(), [](const auto &element) { return element.role == SemanticRole::Link; });
             Require(parentLink != semantic->Elements().end() && parentLink->title == L"Parent title",
                     "math processing preserves the exact title on its neighboring parent link");
             const auto spacedAt = spacedSlash.find(L"two words");
             app.Document().SetSelection({spacedAt, spacedAt});
             Require(app.EquationPreviewText().find(L"\\text{two words}") != std::wstring::npos && app.Document().GetText() == spacedSlash,
                     "the syntax-backed inspector recognizes whitespace-tolerant slash delimiters without rewriting source");
             const std::wstring rejectedWithNeighbors =
                 L"**rich neighbor** before [parent $\\include{private}$](https://example.test \"Parent title\") after.\n";
             app.SetDocument(rejectedWithNeighbors);
             app.Prepare(1024);
             semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(static_cast<bool>(semantic), "rejected-math semantic publication exists");
             const auto rejected = std::ranges::find_if(semantic->Diagnostics(), [](const auto &diagnostic)
                                                        { return diagnostic.kind == SemanticDiagnosticKind::UnsupportedMath; });
             const auto rejectedLink =
                 std::ranges::find_if(semantic->Elements(), [](const auto &element) { return element.role == SemanticRole::Link; });
             const auto richNeighborDisplay = semantic->Map().Display().find(L"rich neighbor");
             Require(
                 semantic && rejected != semantic->Diagnostics().end() &&
                     rejectedWithNeighbors.substr(rejected->source.first, rejected->source.last - rejected->source.first) ==
                         L"$\\include{private}$" &&
                     rejectedLink != semantic->Elements().end() && rejectedLink->title == L"Parent title" &&
                     std::ranges::any_of(semantic->Syntax()->Inlines(), [](const auto &node) { return node.kind == MarkdownInlineKind::Strong; }) &&
                     richNeighborDisplay != std::wstring::npos &&
                     std::ranges::any_of(semantic->Runs(),
                                         [richNeighborDisplay](const auto &run)
                                         {
                                             return (run.flags & DxUi::RichText::Bold) != 0 && run.start <= richNeighborDisplay &&
                                                    richNeighborDisplay + std::wstring_view(L"rich neighbor").size() <= run.start + run.length;
                                         }) &&
                     semantic->Map().Display().find(L"\\include{private}") != std::wstring::npos && app.Document().GetText() == rejectedWithNeighbors,
                 "a rejected native equation preserves its exact local source while rich neighboring nodes and link title remain active");
             const std::wstring diagramNeighbors =
                 L"[Parent](https://example.test \"Parent title\")\n\n```mermaid\nclassDiagram\nBase <|-- Derived\n```\n\n"
                 L"```mermaid\nflowchart LR\nStart([Begin]) --> Finish([Done])\n```\n";
             app.SetDocument(diagramNeighbors);
             app.Prepare(1024);
             semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(static_cast<bool>(semantic), "typed diagram semantic publication exists");
             std::vector<std::wstring_view> diagramAlternatives;
             for (const auto &element : semantic->Elements())
                 if (element.role == SemanticRole::Diagram)
                     diagramAlternatives.push_back(element.name);
             const auto diagramLink =
                 std::ranges::find_if(semantic->Elements(), [](const auto &element) { return element.role == SemanticRole::Link; });
             Require(semantic && diagramAlternatives.size() == 2 && semantic->Diagnostics().empty() &&
                         diagramAlternatives[0].find(L"Derived inherits Base") != std::wstring_view::npos &&
                         diagramAlternatives[1].find(L"Begin") != std::wstring_view::npos &&
                         diagramAlternatives[1].find(L"Done") != std::wstring_view::npos && diagramLink != semantic->Elements().end() &&
                         diagramLink->title == L"Parent title" && app.Document().GetText() == diagramNeighbors,
                     "typed class inheritance and stadium flowchart nodes retain their native alternatives and parent link title");
             const auto oversized = L"$" + std::wstring(65534u, L'x') + L"$";
             app.SetDocument(oversized);
             app.Prepare(1024);
             app.Document().SetSelection({65000, 65000});
             (void)app.Document().OnKeyDown(app.Host(), VK_LEFT, 0);
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
             auto before = app->Activity();
             app->Execute(Command::LocalSource);
             Require(app->HasBlockDraft() && app->Activity().recoveryTextCopies == before.recoveryTextCopies,
                     "opening an unchanged clean draft submits retirement without copying source");
             const auto selection = app->Document().GetSelection();
             const auto canUndo = app->Document().CanUndo(), canRedo = app->Document().CanRedo();
             app->BlockDraft()->SetText(L"Unapplied 東京 😀");
             before = app->Activity();
             app->CheckpointRecovery(true);
             Require(app->Activity().recoveryTextCopies == before.recoveryTextCopies + 1,
                     "a changed guarded draft creates one previewable source copy");
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
             Require(app->Document().GetSelection() == selection && app->Document().CanUndo() == canUndo && app->Document().CanRedo() == canRedo,
                     "checkpoint preview preserves authoritative selection and undo/redo");
             unsigned checkpoints{};
             for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                 if (entry.path().extension() == L".rtd")
                 {
                     std::string bytes;
                     RecoverySnapshot proposed;
                     Success(ReadBytes(entry.path(), bytes), "read proposed draft checkpoint");
                     Success(ParseRecovery(bytes, proposed), "validate proposed draft checkpoint");
                     Require(proposed.text == L"Unapplied 東京 😀 writing" && proposed.originalPath == original && proposed.bom &&
                                 proposed.selection == DxUi::RichText::Selection{0, std::wstring_view(L"Unapplied 東京 😀").size()},
                             "recovery retains exact proposed source, selection, path and encoding");
                     ++checkpoints;
                 }
             Require(checkpoints == 1, "one live-session draft checkpoint exists");
             before = app->Activity();
             app->Execute(Command::CloseDrawer);
             Require(app->Activity().recoverySubmissions == before.recoverySubmissions + 1 &&
                         app->Activity().recoveryTextCopies == before.recoveryTextCopies,
                     "cancelling a clean draft submits immediate retirement without copying text");
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
             const auto comparisons = app->Activity().dirtyTextComparisons;
             Require(app->Dirty() && app->Dirty() && app->Activity().dirtyTextComparisons == comparisons,
                     "restored state overrides the clean baseline without repeated comparisons");
             Require(app->Host().GetFocusControl() == &app->Document(), "restore returns logical keyboard routing to the editor");
             StoredDocument unchanged;
             Success(LoadDocument(original, unchanged), "read original after recovery");
             Require(unchanged.text == L"Original writing" && unchanged.bom, "recovery never overwrites original Markdown");
             app->SetDocument(L"Unapplied 東京 😀 writing");
             Require(!app->Dirty() && !app->RequiresSaveAs(), "explicit document replacement clears the recovered-state dirty fact");
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
    test("Editor.LocalizedResourceViewsRetainStorageAndFormatting",
         []
         {
             const auto blockFormat = ResourceView(IDS_BLOCKINFO);
             const auto positionFormat = ResourceView(IDS_STATUSPOSITION);
             const std::wstring retainedBlockFormat(blockFormat), retainedPositionFormat(positionFormat);
             Require(!blockFormat.empty() && !positionFormat.empty() && ResourceView(0xFFFFu).empty() && Resource(0xFFFFu).empty(),
                     "known localized templates are present and missing resources stay empty");
             // A view must not borrow a temporary owning string or a buffer reused by the next lookup.
             for (unsigned index = 0; index < 64; ++index)
             {
                 const auto other = Resource(index % 2 ? IDS_BLOCKPARAGRAPH : IDS_STATUSCOUNTS);
                 Require(!other.empty(), "subsequent resource lookups remain valid");
             }
             Require(blockFormat == retainedBlockFormat && positionFormat == retainedPositionFormat,
                     "borrowed templates survive subsequent resource allocations");
             const auto blockType = ResourceView(IDS_BLOCKPARAGRAPH);
             const auto eol = ResourceView(IDS_EOLCRLF);
             const std::size_t characters = 42, selected = 3, line = 7, column = 11;
             const auto encoding = L" BOM";
             Require(std::vformat(blockFormat, std::make_wformat_args(blockType, characters)) ==
                             std::vformat(retainedBlockFormat, std::make_wformat_args(blockType, characters)) &&
                         std::vformat(positionFormat, std::make_wformat_args(selected, line, column, encoding, eol)) ==
                             std::vformat(retainedPositionFormat, std::make_wformat_args(selected, line, column, encoding, eol)),
                     "borrowed localized templates preserve positional formatting and owned output");
             Require(std::vformat(L"{1}: {0}", std::make_wformat_args(blockType, characters)) == L"42: " + std::wstring(blockType),
                     "borrowed resource arguments permit translated placeholder reordering");
         });
    test("Performance.OwnedHeapDiagnosticDistinguishesBusyFreeAndPartialCoverage",
         [&]
         {
             wil::unique_hheap heap(HeapCreate(0, 0, 0));
             Require(static_cast<bool>(heap), "create an isolated test-owned heap");
             const std::array<HANDLE, 1> handles{heap.get()};
             const auto sample = [&](std::size_t entryLimit = Test::MaximumDiagnosticEntriesPerHeap)
             {
                 std::ostringstream report;
                 const bool complete = Test::WriteOwnedHeapDiagnostic(report, handles, entryLimit);
                 return std::pair{complete, report.str()};
             };
             const auto before = sample();
             Require(before.first && JsonUnsigned(before.second, "actualWalkedHeapCount") == 1 &&
                         JsonUnsigned(before.second, "completeHeapCount") == 1,
                     "the explicit owned heap completes without claiming process-wide coverage");
             const auto beforeBusy = JsonUnsigned(JsonObjectAfter(before.second, "\"totals\":"), "busyPayloadBytes");
             constexpr std::size_t allocationBytes = 64 * 1024;
             bool allocationReleased{};
             {
                 auto *allocation = HeapAlloc(heap.get(), 0, allocationBytes);
                 Require(allocation != nullptr, "allocate a known payload in the owned heap");
                 const auto freeAllocation = wil::scope_exit([&] { allocationReleased = HeapFree(heap.get(), 0, allocation) != FALSE; });
                 const auto busy = sample();
                 Require(busy.first && JsonUnsigned(JsonObjectAfter(busy.second, "\"totals\":"), "busyPayloadBytes") >= beforeBusy + allocationBytes,
                         "heap payload reports the live owned allocation without equating it to process private bytes");
                 const auto partial = sample(1);
                 Require(!partial.first && partial.second.find("\"budgetExceeded\":true") != partial.second.npos &&
                             JsonUnsigned(partial.second, "entryLimit") == 1,
                         "a bounded partial walk retains its explicit truncation instead of claiming complete coverage");
                 const auto row = JsonObjectContaining(busy.second, busy.second.find("\"index\":0"));
                 const auto exact = sample(static_cast<std::size_t>(JsonUnsigned(row, "walkedEntries")));
                 Require(exact.first, "an exact entry-limit boundary recognizes end-of-heap after the final allowed entry");
             }
             const auto freed = sample();
             // Sanitizer quarantine and heap managers may retain physical blocks after a successful logical free.
             Require(allocationReleased && freed.first,
                     "release succeeds and physical heap state remains measurable without requiring purge or working-set trimming");
             const std::array<HANDLE, 2> duplicate{heap.get(), heap.get()};
             std::ostringstream duplicateReport;
             Require(!Test::WriteOwnedHeapDiagnostic(duplicateReport, duplicate) && JsonUnsigned(duplicateReport.str(), "attemptedHeapCount") == 1 &&
                         JsonUnsigned(duplicateReport.str(), "actualWalkedHeapCount") == 1,
                     "duplicate handles cannot double-count the same physical heap");
             std::array<HANDLE, Test::MaximumDiagnosticHeaps + 1> absent{};
             std::ostringstream absentReport;
             Require(
                 !Test::WriteOwnedHeapDiagnostic(absentReport, absent) && JsonUnsigned(absentReport.str(), "requestedHeapCount") == absent.size() &&
                     JsonUnsigned(absentReport.str(), "boundedHeapCount") == Test::MaximumDiagnosticHeaps &&
                     JsonUnsigned(absentReport.str(), "attemptedHeapCount") == 0 && JsonUnsigned(absentReport.str(), "actualWalkedHeapCount") == 0,
                 "invalid and over-limit inputs distinguish requested, bounded and actual heap walks");
         });
    test("Performance.SelectionDiagnosticRejectsReentryAndUnwinding",
         [&]
         {
             EditDiagnostics diagnostic;
             {
                 auto arm = diagnostic.ArmSelection();
                 auto invocation = diagnostic.EnterSelection();
                 {
                     auto outer = diagnostic.MeasureSelection(SelectionComponent::Find);
                     auto nested = diagnostic.MeasureSelection(SelectionComponent::Outline);
                 }
             }
             Require(!diagnostic.selection.complete, "overlapping selection components cannot produce an attributed timing record");
             EditDiagnostics unwinding;
             try
             {
                 auto arm = unwinding.ArmSelection();
                 auto invocation = unwinding.EnterSelection();
                 throw std::runtime_error("expected diagnostic unwind");
             }
             catch (const std::runtime_error &)
             {
                 // This deliberate local exception verifies diagnostic invalidation; it is not an application failure.
             }
             Require(!unwinding.selection.complete, "an unwound selection is reported incomplete instead of inventing phase durations");
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
                 Require(probeReport.find("\"protocol\":\"app-selection-components-v1\"") != probeReport.npos &&
                             probeReport.find("\"parentReconciliation\":") != probeReport.npos &&
                             probeReport.find("\"appBuffersAfterPrimary\":") != probeReport.npos &&
                             probeReport.find("\"heapDiagnosticCoverage\":\"explicit-default-and-crt-only\"") != probeReport.npos,
                         "nested selection attribution and after-primary storage have distinct bounded diagnostic records");
                 const auto hostCounters = JsonObjectAfter(probeReport, "\"hostCacheCounters\":");
                 Require(JsonUnsigned(hostCounters, "configuredTextFormats") == app.Host().DebugGetConfiguredTextFormatCount() &&
                             JsonUnsigned(hostCounters, "cachedBrushes") == app.Host().DebugGetBrushCacheSize() &&
                             JsonUnsigned(hostCounters, "rootLogicalChildren") == app.Host().GetRoot()->GetLogicalChildCount(),
                         "post-primary cache counts describe the owned host without claiming allocation ownership");
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
                 std::string probeReport;
                 Success(ReadBytes(launch.performance, probeReport), "read native resource lifecycle diagnostics");
                 const bool reportsWarp = probeReport.find("\"renderer\":\"WARP\"") != probeReport.npos;
                 const bool reportsHardware = probeReport.find("\"renderer\":\"Hardware\"") != probeReport.npos;
                 Require(reportsWarp || reportsHardware, "resource probe reports a recognized adapter mode");
                 Require(!warp || reportsWarp, "resource probe honors the explicit WARP adapter request");
                 const auto firstPreparation = JsonObjectAfter(probeReport, "\"firstCompletedPreparation\":");
                 RequireCompletedResourceCost(firstPreparation, "initial reset frame flush count is in the bounded root-child range");
                 RequireResourcePhase(probeReport, "clean-after-dirty", false);
                 RequireResourcePhase(probeReport, "clean-hidden-idle", false);
                 RequireResourcePhase(probeReport, "hidden-notification-idle", false);
                 RequireResourcePhase(probeReport, "resumed-forced-frame", false);
                 RequireResourcePhase(probeReport, "resized-forced-frame", true);
                 std::size_t recreated{};
                 constexpr std::string_view recreatedMarker = "\"phase\":\"recreated-forced-frame\"";
                 for (auto at = probeReport.find(recreatedMarker); at != probeReport.npos;
                      at = probeReport.find(recreatedMarker, at + recreatedMarker.size()))
                 {
                     const auto entry = JsonObjectContaining(probeReport, at);
                     RequireCompletedResourceCost(JsonObjectAfter(entry, "\"preparation\":"),
                                                  "device-loss reset frame flush count is in the bounded root-child range");
                     ++recreated;
                 }
                 Require(recreated == 3, "resource probe records all three device-loss recovery preparations");
                 RequireResourcePhase(probeReport, "closed", false);
                 Require(probeReport.find("\"completed\":true") != probeReport.npos, "resource probe report is complete");
                 Require(app.Window() == nullptr && app.Host().GetHwnd() == nullptr, "resource protocol retires its window and native host");
             }
         });
    report << "\n  ],\n  \"passed\": " << passed << ", \"failed\": " << failed << ", \"selected\": " << selected << ", \"registered\": " << registered
           << ", \"catalogOnly\": " << (listOnly ? "true" : "false") << "\n}\n";
    return failed || selected == 0 ? 1 : 0;
}
} // namespace RedTilde
