#include "RedTilde/Application.h"
#include "Application.Tests.Close.h"
#include "Application.Tests.DesignRefresh.h"
#include "Application.Tests.FailureHost.h"
#include "Application.Tests.Files.h"
#include "Application.Tests.HeapDiagnostic.h"
#include "Application.Tests.RemoteImages.h"
#include "Support/Support.Tests.BitmapAssets.h"
#include "Support/Support.Tests.Foreground.h"
#include "Support/Support.Tests.StorageSafety.h"
#include "Support/Support.Tests.TextServices.h"
#include "res/Resource.h"
#include <DxUi/RichMath.h>
#include <DxUi/RichSemanticDocument.h>
#include <DxUi/TextInputServices.h>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
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
int RunLinkedSaveInterruptionProbe(const std::filesystem::path &root, bool restart)
{
    const auto target = root / L"notebook.md", alias = root / L"same-notebook.md";
    auto marker = root / L"partial-write-admitted.txt";
    const std::wstring admitted = L"Complete admitted source 世界\r\nretained after abrupt process exit\r\n";
    if (!restart)
    {
        std::filesystem::create_directories(root);
        FileStamp original, saved;
        Success(SaveDocument(target, L"Original file bytes", false, nullptr, original), "seed interruption target");
        Require(CreateHardLinkW(alias.c_str(), target.c_str(), nullptr), "seed interruption alias");
        const SaveRecoveryContext recovery{root, {9, 2}, 121};
        SetAtomicWriteFailureForTest(AtomicWriteFailure::InPlacePartialWrite);
        SetAtomicWriteHookForTest(
            [](AtomicWriteCheckpoint phase, const std::filesystem::path &, void *context) noexcept
            {
                if (phase != AtomicWriteCheckpoint::InPlacePartialWritten)
                    return;
                const auto &path = *static_cast<const std::filesystem::path *>(context);
                if (FAILED(WriteBytesAtomic(path, "durable recovery preceded this partial target write")))
                    return;
                // Owned test child only: terminate while the writer and recovery
                // lease are still live. No destructor or crash-path recovery write.
                if (!TerminateProcess(GetCurrentProcess(), 79))
                    std::terminate();
            },
            &marker);
        (void)SaveDocument(target, admitted, true, &original, saved, false, nullptr, &recovery);
        return 6; // The exact intentional exit must occur inside the write.
    }
    std::string marked, primary, shared;
    FileStamp first, second;
    Success(ReadBytes(marker, marked), "child reached its protected partial write");
    Require(marked == "durable recovery preceded this partial target write", "exact interruption marker");
    Success(ReadBytes(target, primary, &first), "read partial target after process exit");
    Success(ReadBytes(alias, shared, &second), "read partial alias after process exit");
    Require(primary == shared && first == second && primary != "Original file bytes" &&
                primary != "\xEF\xBB\xBF" + DxUi::RichText::EncodeUtf8(admitted),
            "process really stopped with shared partial bytes, not before admission or after complete success");
    RecoveryService recovery;
    Success(recovery.Initialize(root), "restart linked-save recovery");
    Success(recovery.WaitForSettlement(5000), "restart scan settles after OS releases the crashed process lease");
    Require(recovery.Ready(), "restart scan is complete");
    const auto candidates = recovery.TakeCandidates();
    Require(candidates.size() == 1 && candidates[0].snapshot.text == admitted && candidates[0].snapshot.originalPath == target &&
                candidates[0].snapshot.bom && candidates[0].snapshot.selection == DxUi::RichText::Selection{9, 2} &&
                candidates[0].snapshot.generation == 121,
            "restart offers complete immutable admitted source, BOM, location, selection and generation");
    Success(WriteBytesAtomic(root / L"linked-save-restart.json", "{\"recoveryComplete\":true,\"partialSharedTarget\":true,\"releasedLease\":true}\n"),
            "publish independently verified restart outcome");
    return 0;
}
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
        const bool editorPartition = scope == CoreTestScope::EditorBehavior || scope == CoreTestScope::EditorPerformance ||
                                     scope == CoreTestScope::EditorRendering || scope == CoreTestScope::EditorContent ||
                                     scope == CoreTestScope::EditorWorkspace || scope == CoreTestScope::EditorMedia;
        if (editorPartition)
        {
            if (owner != CoreTestScope::Editor || (scope == CoreTestScope::EditorPerformance && prefix != "Performance") ||
                (scope == CoreTestScope::EditorRendering && prefix != "Rendering") ||
                (scope == CoreTestScope::EditorBehavior && (prefix == "Performance" || prefix == "Rendering")))
                return;
            if (scope == CoreTestScope::EditorContent || scope == CoreTestScope::EditorWorkspace || scope == CoreTestScope::EditorMedia)
            {
                if (prefix == "Performance" || prefix == "Rendering")
                    return;
                const bool media = std::ranges::any_of(
                    std::array<std::string_view, 8>{"Image", "Bitmap", "Mermaid", "Diagram", "Math", "Equation", "CodeHighlighting", "Http"},
                    [&](auto token) { return name.find(token) != name.npos; });
                const bool workspace = !media && (prefix == "Window" || prefix == "Outline" || name.starts_with("Editor.DesignRefresh") ||
                                                  name.starts_with("Editor.Files") || name.starts_with("Editor.PersistentFolder") ||
                                                  name.starts_with("Editor.Workspace") || name.starts_with("Editor.Startup"));
                const auto partition = media ? CoreTestScope::EditorMedia : workspace ? CoreTestScope::EditorWorkspace : CoreTestScope::EditorContent;
                if (scope != partition)
                    return;
            }
        }
        else if (scope != CoreTestScope::All && owner != scope)
            return;
        if (selected++)
            report << ",\n";
        if (listOnly)
        {
            report << "    {\"name\":\"" << name << "\",\"executed\":false}";
            return;
        }
        report << "    {\"name\":\"" << name << "\",\"passed\":";
        // Preserve the active case even if the enclosing owned child times out.
        report.flush();
        const auto started = std::chrono::steady_clock::now();
        try
        {
            run();
            ++passed;
            report << "true";
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
            report << "\"";
        }
        report << ",\"elapsedMs\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count()
               << "}";
        report.flush();
    };
    RegisterFilesTests(root, test);
    RegisterCloseTests(root, test);
    RegisterDesignRefreshTests(root, test);
    RegisterRemoteImageTests(root, test);
    RegisterFailureHostTests(root, test);
    Testing::RegisterStorageSafetyTests(root, test);
    test("Utf8.CI_ASSERTION_PROBE",
         []
         {
             // Verify the same always-on Require used by the regression cases in Release.
             try
             {
                 Require(false, "CI_ASSERTION_PROBE");
             }
             catch (const std::runtime_error &error)
             {
                 if (std::string_view(error.what()) == "CI_ASSERTION_PROBE")
                 {
                     return;
                 }
                 throw;
             }
             throw std::runtime_error("CI_ASSERTION_PROBE_DISABLED");
         });
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
             for (const auto failure : {AtomicWriteFailure::Write, AtomicWriteFailure::Flush})
             {
                 SetAtomicWriteFailureForTest(failure);
                 const auto resetFailure = wil::scope_exit([] { SetAtomicWriteFailureForTest(AtomicWriteFailure::None); });
                 Require(WriteBytesAtomic(path, "replacement") == HRESULT_FROM_WIN32(ERROR_WRITE_FAULT),
                         "directed write and flush faults preserve their real operation failure");
                 StoredDocument retained;
                 Success(LoadDocument(path, retained), "read target after staged failure");
                 Require(retained.text == L"external" && retained.stamp == loaded.stamp,
                         "failed staging leaves the original bytes and identity intact");
                 for (const auto &entry : std::filesystem::directory_iterator(root))
                     Require(entry.path().extension() != L".tmp",
                             "both injected failure phases close and delete their owned share-none staging file");
             }
             for (const auto &entry : std::filesystem::directory_iterator(root))
                 Require(entry.path().extension() != L".tmp", "no owned temporary leaked");
         });
    test("Storage.AtomicSaveKeepsLongLeafWithinPathBudget",
         [&]
         {
             const auto directory = std::filesystem::absolute(root / L"atomic-temp-budget");
             std::filesystem::create_directories(directory);
             const auto prefix = directory.native().size() + 1u;
             Require(prefix + 4u < 240u, "owned fixture leaves room for a valid near-limit target");
             const auto path = directory / (std::wstring(240u - prefix - 3u, L'n') + L".md");
             Require(path.native().size() == 240u, "exercise the full target path budget");
             FileStamp first;
             Success(SaveDocument(path, L"first", false, nullptr, first), "publish a near-limit target");
             FileStamp second;
             Success(SaveDocument(path, L"second", false, &first, second), "atomically replace a near-limit target");
             StoredDocument loaded;
             Success(LoadDocument(path, loaded), "read the near-limit target");
             Require(loaded.text == L"second" && loaded.stamp == second, "replacement bytes and stamp are preserved");
             std::size_t entries{};
             for (const auto &entry : std::filesystem::directory_iterator(directory))
             {
                 Require(entry.path() == path, "no staging temporary leaked beside the target");
                 ++entries;
             }
             Require(entries == 1u, "only the published target remains");
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
    test("Storage.BitmapAssetsEncodeAndRetireOwnedFiles",
         [&]
         {
             const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
             Require(SUCCEEDED(initialized) || initialized == RPC_E_CHANGED_MODE, "use the existing COM apartment for bitmap fixtures");
             const wil::unique_call<decltype(&CoUninitialize), CoUninitialize> apartment(SUCCEEDED(initialized));
             const auto directory = root / L"clipboard-assets";
             for (const bool v5 : {false, true})
             {
                 BitmapAsset asset;
                 const auto dib = Testing::ClipboardBitmapFixture(v5);
                 Success(CreateBitmapAsset(directory, dib, v5, asset), "encode a real DIB to a durable PNG");
                 Require(asset.width == 2 && asset.height == 2 && asset.path.extension() == L".png" && asset.uri.starts_with(L"file:"),
                         "clipboard assets carry their actual dimensions and an absolute file URI");
                 const std::array<unsigned char, 16> expected{0,   0, 255, 255, 0,   255, 0,   static_cast<unsigned char>(v5 ? 128 : 255),
                                                              255, 0, 0,   255, 255, 255, 255, static_cast<unsigned char>(v5 ? 64 : 255)};
                 Require(Testing::ReadClipboardPngPixels(asset.path) == expected,
                         "independent PNG decoding preserves orientation, colors and DIBV5 alpha");
                 std::string png;
                 FileStamp observed;
                 Success(ReadBytes(asset.path, png, &observed), "read complete generated PNG and native stamp");
                 Require(observed == asset.stamp && png.starts_with("\x89PNG\r\n\x1a\n"), "the durable stamp describes the actual closed PNG file");
                 Success(RetireBitmapAsset(asset), "retire an unreferenced owned asset by full identity and content");
                 Require(!std::filesystem::exists(asset.path), "verified asset cleanup removed its own file");
             }
             const auto owned = directory / L"created.bin";
             FileStamp original;
             Success(WriteNewBytesDurable(owned, "safe", original, MaximumClipboardPngBytes), "create an independent owned binary");
             FileStamp untouched = original;
             Require(FAILED(WriteNewBytesDurable(owned, "evil", untouched, MaximumClipboardPngBytes)) && untouched == original,
                     "binary publication never replaces an existing entry or advances a refused stamp");
             {
                 wil::unique_handle file(
                     CreateFileW(owned.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
                 Require(bool(file), "open the independent same-identity edit fixture");
                 DWORD written{};
                 Require(WriteFile(file.get(), "edit", 4, &written, nullptr) && written == 4 && FlushFileBuffers(file.get()),
                         "change bytes without replacing the file");
                 const FILETIME time{static_cast<DWORD>(original.modified), static_cast<DWORD>(original.modified >> 32u)};
                 Require(SetFileTime(file.get(), nullptr, nullptr, &time) != FALSE, "restore the original metadata timestamp independently");
             }
             Require(RemoveCreatedBytesIfUnchanged(owned, original, MaximumClipboardPngBytes) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH),
                     "content validation preserves a same-id same-size same-time modification");
             std::string bytes;
             Success(ReadBytes(owned, bytes), "read preserved independently modified binary");
             Require(bytes == "edit", "cleanup retained modified bytes");
             const auto large = directory / L"bounded-binary.bin";
             FileStamp largeStamp;
             Success(WriteNewBytesDurable(large, std::string(300000u, 'x'), largeStamp, MaximumClipboardPngBytes),
                     "create a bounded asset larger than Markdown input");
             bytes = "unchanged";
             Require(ReadBytes(large, bytes) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) && bytes == "unchanged",
                     "asset support never relaxes the Markdown input limit");
             Success(RemoveCreatedBytesIfUnchanged(large, largeStamp, MaximumClipboardPngBytes), "retire the separately bounded large binary");
             BitmapAsset refused;
             Require(FAILED(CreateBitmapAsset(directory, std::vector<std::byte>(40u), false, refused)) && refused.path.empty(),
                     "malformed bitmap input publishes no asset");
         });
    test("Settings.LocalNetworkImagePermission",
         [&]
         {
             for (const auto *text :
                  {"RedTilde.Settings/1\n", "RedTilde.Settings/1\nlocalNetworkImages=2\n", "RedTilde.Settings/1\nlocalNetworkImages=invalid\n"})
                 Require(!ParseSettings(text).allowLocalNetworkImages, "missing and invalid local-network permission stays off");
             Settings settings;
             settings.allowLocalNetworkImages = true;
             const auto round = ParseSettings(SerializeSettings(settings));
             Require(round.allowLocalNetworkImages && !round.allowRemoteImages, "LAN permission round trips independently of image consent");
             const auto directory = root / L"local-network-image-settings";
             Success(SaveSettings(directory, settings), "persist explicit local-network image preference");
             Settings loaded;
             Success(LoadSettings(directory, loaded), "restore explicit local-network image preference");
             Require(loaded.allowLocalNetworkImages && !loaded.allowRemoteImages,
                     "the opt-in survives restart without granting internet image consent");
         });
    test("Settings.ImageFirstPasteRoundTripAndFallback",
         [&]
         {
             Require(!ParseSettings("RedTilde.Settings/1\n").imageFirstPaste &&
                         !ParseSettings("RedTilde.Settings/1\nimageFirstPaste=2\n").imageFirstPaste &&
                         !ParseSettings("RedTilde.Settings/1\nimageFirstPaste=invalid\n").imageFirstPaste,
                     "missing and invalid clipboard preferences retain Unicode-first behavior");
             Settings settings;
             settings.imageFirstPaste = true;
             Require(ParseSettings(SerializeSettings(settings)).imageFirstPaste, "image-first preference round trips independently");
             const auto directory = root / L"clipboard-settings";
             Success(SaveSettings(directory, settings), "persist image-first preference");
             Settings loaded;
             Success(LoadSettings(directory, loaded), "restore image-first preference");
             Require(loaded.imageFirstPaste, "image-first preference survives restart");
         });
    test("Settings.VersionAndFieldFallback",
         [&]
         {
             Settings value;
             value.appearance = Appearance::Paper;
             value.view = EditorView::Split;
             value.showOutline = false;
             value.strikeCompletedTasks = true;
             const auto round = ParseSettings(SerializeSettings(value));
             Require(round.appearance == value.appearance && round.view == value.view && !round.showOutline, "settings round trip");
             Require(round.strikeCompletedTasks && !ParseSettings("RedTilde.Settings/1\nstrikeCompletedTasks=2\n").strikeCompletedTasks &&
                         !ParseSettings("RedTilde.Settings/1\n").strikeCompletedTasks,
                     "completed-task preference defaults, validation and round trip");
             const auto invalid = ParseSettings("RedTilde.Settings/1\ntheme=2\nview=99\nscale=900\noutline=0\n");
             Require(invalid.appearance == Appearance::Dark && invalid.view == EditorView::Compose && !invalid.showOutline,
                     "field fallback isolated");
             Require(ParseSettings("RedTilde.Settings/9\ntheme=2\n").appearance == Appearance::System, "unknown schema defaults");
             Success(SaveSettings(root, value), "save preferences");
             Settings loaded;
             Success(LoadSettings(root, loaded), "reload preferences");
             Require(loaded.appearance == Appearance::Paper && loaded.strikeCompletedTasks, "persisted preferences");
             const auto rejected = root / L"rejected-settings";
             std::filesystem::create_directories(rejected);
             constexpr std::string_view original = "RedTilde.Settings/9\nview=2\nunknown=keep these bytes\n";
             Success(WriteBytesAtomic(rejected / L"RedTilde.settings", original), "seed rejected settings bytes");
             Require(LoadSettings(rejected, loaded) == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) && loaded.appearance == Appearance::Paper,
                     "a rejected header is reported without replacing caller settings");
             loaded.view = EditorView::Source;
             Require(SaveSettings(rejected, loaded) == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT),
                     "default or changed view cannot overwrite a rejected header");
             SettingsWriter writer;
             writer.Initialize(rejected);
             Success(writer.Submit(loaded), "admit an asynchronous preference update");
             for (unsigned turn = 0; turn < 1000 && writer.Busy(); ++turn)
                 std::this_thread::sleep_for(std::chrono::milliseconds(1));
             Require(!writer.Busy() && writer.Status() == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT),
                     "the asynchronous writer reports the same header refusal");
             writer.Close();
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = rejected;
             {
                 Application app(launch);
                 Success(app.Initialize(), "start with defaults beside the retained rejected header");
                 app.SetView(EditorView::Source);
                 app.SetView(EditorView::Split);
             }
             std::string preserved;
             Success(ReadBytes(rejected / L"RedTilde.settings", preserved), "read rejected settings after real view switches");
             Require(preserved == original, "rejected settings remain byte-exact after synchronous, asynchronous and application view changes");
         });
    test("Settings.LastDocumentPathRoundTripAndFallback",
         []
         {
             Settings value;
             value.filesRoot = L"C:\\Notebooks\\Pinned root";
             value.lastDocument = L"D:\\Writing\\R\u00e9sum\u00e9 = \u6771\u4eac \U0001f600.md";
             const auto round = ParseSettings(SerializeSettings(value));
             Require(round.lastDocument == value.lastDocument && round.filesRoot == value.filesRoot,
                     "the last document and the independent Files root preserve Unicode, spaces and equals signs");
             for (const auto invalid : {"relative.md", "C:\\notes\rmalformed.md", "C:\\bad\xFF.md"})
             {
                 const auto parsed = ParseSettings(std::string("RedTilde.Settings/1\nview=2\nlastDocument=") + invalid + "\nscale=125\n");
                 Require(parsed.lastDocument.empty() && parsed.view == EditorView::Split && parsed.textScale == 125u,
                         "invalid last-document paths fall back independently without replacing valid neighboring preferences");
             }
             value.lastDocument = L"C:\\bad\nview=2";
             Require(SerializeSettings(value).find("lastDocument=") == std::string::npos &&
                         ParseSettings("RedTilde.Settings/1\n").lastDocument.empty(),
                     "invalid paths cannot inject preferences and older settings default to no remembered document");
         });
    test("Settings.WindowPlacementRejectsPartialAndUnsafeValues",
         [&]
         {
             Settings value;
             value.appearance = Appearance::Dark;
             value.windowPlacement = WindowPlacementSettings{{-1760, 90, -560, 890}, 144u, true};
             const auto serialized = SerializeSettings(value);
             const auto round = ParseSettings(serialized);
             Require(round.windowPlacement && EqualRect(&round.windowPlacement.value().normalBounds, &value.windowPlacement.value().normalBounds) &&
                         round.windowPlacement.value().dpi == 144u && round.windowPlacement.value().maximized && round.appearance == Appearance::Dark,
                     "negative screen coordinates, saved DPI and maximized state round trip independently of appearance");
             Require(serialized.find("windowLeft=-1760\n") != std::string::npos && serialized.find("windowDpi=144\n") != std::string::npos,
                     "placement uses counted signed screen coordinates and explicit DPI in the existing versioned settings");
             constexpr std::string_view prefix = "RedTilde.Settings/1\ntheme=2\nscale=125\n";
             for (const auto invalid :
                  {"windowLeft=10\nwindowTop=20\nwindowRight=1210\n", "windowLeft=no\nwindowTop=20\nwindowRight=1210\nwindowBottom=820\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=10\nwindowBottom=820\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=-10\nwindowBottom=820\n",
                   "windowLeft=999999999999\nwindowTop=20\nwindowRight=1210\nwindowBottom=820\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=1048577\nwindowBottom=820\n",
                   "windowLeft=-2147483648\nwindowTop=20\nwindowRight=2147483647\nwindowBottom=820\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=40000\nwindowBottom=820\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=1210\nwindowBottom=820\nwindowDpi=0\n",
                   "windowLeft=10\nwindowTop=20\nwindowRight=1210\nwindowBottom=820\nwindowDpi=broken\n"})
             {
                 const auto rejected = ParseSettings(std::string(prefix) + invalid);
                 Require(!rejected.windowPlacement && rejected.appearance == Appearance::Dark && rejected.textScale == 125u,
                         "an incomplete, malformed or unsafe placement is rejected as a tuple without discarding neighboring preferences");
             }
             const auto old =
                 ParseSettings("RedTilde.Settings/1\nwindowLeft=10\nwindowTop=20\nwindowRight=1210\nwindowBottom=820\nwindowMaximized=2\n");
             Require(old.windowPlacement && old.windowPlacement.value().dpi == 96u && !old.windowPlacement.value().maximized,
                     "missing DPI defaults to 96 and an invalid maximized flag defaults independently to normal");
             value.windowPlacement.value().normalBounds.right = value.windowPlacement.value().normalBounds.left;
             Require(SerializeSettings(value).find("windowLeft=") == std::string::npos,
                     "invalid in-memory geometry never publishes a partial placement record");
             value.windowPlacement = round.windowPlacement;
             const auto directory = root / L"window-placement-settings";
             Success(SaveSettings(directory, value), "persist actual signed placement through the ordinary atomic writer");
             Settings loaded;
             Success(LoadSettings(directory, loaded), "reload actual signed placement");
             Require(loaded.windowPlacement && EqualRect(&loaded.windowPlacement.value().normalBounds, &round.windowPlacement.value().normalBounds) &&
                         loaded.windowPlacement.value().dpi == 144u && loaded.windowPlacement.value().maximized,
                     "the durable settings file preserves normal screen bounds and the separate maximized state");
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
             const auto nested = [](unsigned depth) { return std::wstring(depth, L'{') + L"x" + std::wstring(depth, L'}'); };
             Require(static_cast<bool>(ParseMath(nested(DxUi::RichText::MathMaximumGroupDepth))) &&
                         !ParseMath(nested(DxUi::RichText::MathMaximumGroupDepth + 1u)),
                     "recursion is bounded exactly at the pinned group depth");
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
             // 32 columns keep rows and columns below their 64 limits, so only the cell budget decides.
             constexpr unsigned matrixColumns = 32;
             const auto matrixSource = [](unsigned rows)
             {
                 std::wstring value = L"\\begin{matrix}";
                 for (unsigned row = 0; row < rows; ++row)
                 {
                     if (row)
                         value += L"\\\\";
                     for (unsigned column = 0; column < matrixColumns; ++column)
                     {
                         if (column)
                             value += L'&';
                         value += L'x';
                     }
                 }
                 value += L"\\end{matrix}";
                 return value;
             };
             constexpr auto budgetRows = static_cast<unsigned>(DxUi::RichText::MathMaximumMatrixCells / matrixColumns);
             static_assert(budgetRows * matrixColumns == DxUi::RichText::MathMaximumMatrixCells && budgetRows < DxUi::RichText::MathMaximumRows);
             Require(static_cast<bool>(ParseMath(matrixSource(budgetRows))) && !ParseMath(matrixSource(budgetRows + 1u)),
                     "the pinned matrix cell budget admits its boundary and rejects excess cells");
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
             for (const auto &invalid :
                  {RecoverySnapshot{std::wstring(DxUi::RichText::Document::MaximumUnits + 1u, L'x'), {}, false, {}, 1u},
                   RecoverySnapshot{std::wstring(1u, static_cast<wchar_t>(0xd800)), {}, false, {}, 1u},
                   RecoverySnapshot{L"\U0001F642", {}, false, {1u, 1u}, 1u}, RecoverySnapshot{L"Retained writing", L"relative.md", false, {}, 1u}})
             {
                 std::string retained = "unchanged serialization output";
                 Require(TrySerializeRecovery(invalid, retained) == E_INVALIDARG && retained == "unchanged serialization output",
                         "checked recovery serialization rejects invalid source/path/selection before changing its output");
             }
         });
    test("Recovery.OversizedValidEnvelopeRejectsAndPreservesQuarantine",
         [&]
         {
             constexpr auto maximum = DxUi::RichText::Document::MaximumUnits;
             const auto directory = root / L"oversized-recovery";
             const auto recoveryDirectory = directory / L"Recovery";
             std::filesystem::create_directories(recoveryDirectory);
             auto envelope = [](std::string_view text, std::uint64_t generation)
             {
                 // Independent encoded-length/checksum construction deliberately
                 // bypasses the serializer's already-correct source-unit guard.
                 const auto metadata = std::format("0 {} 0 0 0 {}", text.size(), generation);
                 const auto payload = std::string(text);
                 return std::string("RedTilde.Recovery/1\n") + metadata + std::format(" {}\n", ContentHash(metadata + "\n" + payload)) + payload;
             };
             std::vector<std::string> payloads{std::string(maximum + 1, 'a')};
             std::string bmp, supplementary;
             for (std::size_t index = 0; index <= maximum; ++index)
                 bmp += "界";
             for (std::size_t index = 0; index <= maximum / 2; ++index)
                 supplementary += "😀";
             payloads.push_back(std::move(bmp));
             payloads.push_back(std::move(supplementary));
             std::vector<std::pair<std::filesystem::path, std::string>> invalid;
             const RecoverySnapshot original{L"Retain this notebook", L"Z:\\writing\\notebook.md", true, {2, 7}, 52};
             for (std::size_t index = 0; index < payloads.size(); ++index)
             {
                 invalid.emplace_back(recoveryDirectory / std::format(L"session-{:032x}.rtd", index + 1), envelope(payloads[index], index + 1));
                 Success(WriteBytesAtomic(invalid.back().first, invalid.back().second), "retain the original checksum-valid oversized input");
                 auto parsed = original;
                 {
                     // This owned headless process records a noexcept escape as
                     // exit 77 without a CRT/WER dialog. Restore on every normal
                     // path; production termination handling is untouched.
                     const auto previous = std::set_terminate([]() noexcept { ExitProcess(77); });
// The CRT's borrowed terminate_handler type omits noexcept. Restoring it only
// installs the original pointer; this call never invokes that handler.
// Owner: RedTilde test maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 5039)
                     const auto restore = wil::scope_exit([previous] { (void)std::set_terminate(previous); });
#pragma warning(pop)
                     Require(ParseRecovery(invalid.back().second, parsed) == HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                             "decoded source beyond the editor capacity is an ordinary invalid envelope");
                 }
                 Require(parsed.text == original.text && parsed.originalPath == original.originalPath && parsed.bom == original.bom &&
                             parsed.selection == original.selection && parsed.generation == original.generation,
                         "oversized input leaves every prior output field unchanged");
             }
             const auto collision = recoveryDirectory / L"session-00000000000000000000000000000004.rtd";
             auto preserved = collision;
             preserved += L".quarantine";
             const std::string priorQuarantine = "Earlier quarantine must remain byte-for-byte intact.\n";
             Success(WriteBytesAtomic(collision, invalid.front().second), "seed an invalid envelope beside an existing quarantine");
             Success(WriteBytesAtomic(preserved, priorQuarantine), "seed the earlier quarantine");
             const RecoverySnapshot boundary{std::wstring(maximum, L'界'), {}, false, {maximum, maximum}, 99};
             const auto valid = recoveryDirectory / L"session-00000000000000000000000000000005.rtd";
             Success(WriteBytesAtomic(valid, SerializeRecovery(boundary)), "seed a valid full-capacity recovery copy");
             RecoveryService service;
             Success(service.Initialize(directory), "start the real recovery scanner");
             for (unsigned turn = 0; turn < 1000 && !service.Ready(); ++turn)
                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
             Require(service.Ready() && SUCCEEDED(service.Status()), "oversized envelopes cannot terminate the recovery worker");
             const auto candidates = service.TakeCandidates();
             Require(candidates.size() == 1 && candidates.front().snapshot.text == boundary.text &&
                         candidates.front().snapshot.selection == boundary.selection && candidates.front().snapshot.generation == boundary.generation,
                     "the valid maximum-sized copy remains offered without truncation");
             for (const auto &[path, bytes] : invalid)
             {
                 auto quarantine = path;
                 quarantine += L".quarantine";
                 std::string actual;
                 Success(ReadBytes(quarantine, actual), "read quarantined oversized original");
                 Require(actual == bytes && !std::filesystem::exists(path), "quarantine preserves every original envelope byte");
             }
             std::string actual;
             Success(ReadBytes(collision, actual), "read the unrenamed original after a quarantine-name collision");
             Require(actual == invalid.front().second, "failed quarantine movement retains the recoverable original");
             Success(ReadBytes(preserved, actual), "read the earlier quarantine after the collision");
             Require(actual == priorQuarantine, "quarantine never overwrites earlier evidence");
             service.Close();
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
    test("Recovery.IndependentDocumentCheckpointsAndLeases",
         [&]
         {
             const auto directory = root / L"recovery-documents";
             const auto wait = [](const std::function<bool()> &predicate)
             {
                 for (unsigned turn = 0; turn < 1200; ++turn)
                 {
                     if (predicate())
                         return;
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 Require(false, "document recovery worker timed out");
             };
             RecoveryService writer;
             Success(writer.Initialize(directory), "initialize shared document worker");
             wait([&] { return writer.Ready(); });
             for (unsigned document = 0; document < 3; ++document)
                 Success(
                     writer.SubmitDocument(
                         document, RecoverySnapshot{L"draft " + std::to_wstring(document), L"Z:\\Notebook\\same.md", document == 1, {0, 3}, 1}, true),
                     "independent documents may share a source location");
             wait([&] { return writer.Settled(); });
             Success(writer.Status(), "every admitted document is durable");
             RecoveryService observer;
             Success(observer.Initialize(directory), "scan while all document leases are active");
             wait([&] { return observer.Ready(); });
             Require(observer.TakeCandidates().empty(), "no active document is offered by another process");
             observer.Stop();
             wait([&] { return observer.Finished(); });
             Require(FAILED(writer.SubmitDocument(RecoveryService::MaximumDocuments, RecoverySnapshot{}, true)),
                     "out-of-range document identity is refused");
             Success(writer.SubmitDocument(1, std::nullopt, true), "retire only the matching saved document");
             Success(writer.SubmitDocument(2, RecoverySnapshot{L"latest second 😀\r\n", {}, true, {4, 0}, 2}, true),
                     "update a different document without erasing the first");
             writer.Stop();
             wait([&] { return writer.Finished(); });
             Success(writer.Status(), "stop flushes every pending document");
             RecoveryService restart;
             Success(restart.Initialize(directory), "restart scans every inactive document");
             wait([&] { return restart.Ready(); });
             const auto candidates = restart.TakeCandidates();
             Require(candidates.size() == 2 && std::ranges::any_of(candidates, [](const auto &value) { return value.snapshot.text == L"draft 0"; }) &&
                         std::ranges::any_of(candidates,
                                             [](const auto &value)
                                             {
                                                 return value.snapshot.text == L"latest second 😀\r\n" && value.snapshot.bom &&
                                                        value.snapshot.selection == DxUi::RichText::Selection{4, 0};
                                             }),
                     "restart retains exact source, encoding and selection for both unsaved documents");
             restart.Stop();
             wait([&] { return restart.Finished(); });
         });
    test("Recovery.ContinuousTypingCannotStarveCheckpoint",
         [&]
         {
             const auto directory = root / L"recovery-continuous";
             RecoveryService writer;
             Success(writer.Initialize(directory), "initialize continuous-edit worker");
             for (unsigned turn = 0; turn < 1000 && !writer.Ready(); ++turn)
                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
             Require(writer.Ready(), "continuous-edit worker is ready");
             const auto start = GetTickCount64();
             unsigned generation{};
             bool observed{};
             while (GetTickCount64() - start < 6200)
             {
                 ++generation;
                 Success(writer.Submit(RecoverySnapshot{L"continuous " + std::to_wstring(generation), {}, false, {0, 0}, generation}),
                         "continuous edit remains admitted without an immediate flush");
                 for (const auto &entry : std::filesystem::directory_iterator(directory / L"Recovery"))
                     if (entry.path().extension() == L".rtd")
                     {
                         std::string bytes;
                         RecoverySnapshot checkpoint;
                         Success(ReadBytes(entry.path(), bytes), "read complete checkpoint while typing continues");
                         Success(ParseRecovery(bytes, checkpoint), "a continuous-edit checkpoint remains valid");
                         Require(checkpoint.generation > 1 && checkpoint.generation <= generation,
                                 "checkpoint contains a recently admitted immutable generation");
                         observed = true;
                     }
                 std::this_thread::sleep_for(std::chrono::milliseconds(40));
             }
             Require(observed, "a five-second deadline checkpoints without waiting for quiet typing");
             writer.Stop();
             for (unsigned turn = 0; turn < 1000 && !writer.Finished(); ++turn)
                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
             Require(writer.Finished(), "shutdown bypasses quiet-time debounce");
             Success(writer.Status(), "final continuous edit is durable");
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
    test("Editor.SourceEnterPreservesLineEndings",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             std::array<BYTE, 256> savedKeyboard{};
             Require(GetKeyboardState(savedKeyboard.data()) != FALSE, "snapshot the calling thread's Enter modifiers");
             const auto restoreKeyboard = wil::scope_exit(
                 [&] noexcept
                 {
                     if (!SetKeyboardState(savedKeyboard.data()))
                         std::terminate();
                 });
             for (const std::wstring_view eol : {L"\n", L"\r\n", L"\r"})
                 for (const bool shift : {false, true})
                 {
                     LaunchOptions options;
                     options.headless = true;
                     options.dataDirectory = root;
                     Application app(options);
                     Success(app.Initialize(), "initialize hidden Source Enter app");
                     const std::wstring original = L"first" + std::wstring(eol) + L"last";
                     app.SetDocument(original);
                     app.SetView(EditorView::Source);
                     const auto end = original.size();
                     app.Document().SetSelection({end, end});
                     app.Host().SetFocusControl(&app.Document(), false);
                     std::array<BYTE, 256> keys{};
                     keys[VK_SHIFT] = keys[VK_LSHIFT] = shift ? 0x80 : 0;
                     Require(SetKeyboardState(keys.data()) != FALSE, "set only the owned thread's Enter modifiers");
                     const auto before = app.Activity();
                     SendMessageW(app.Window(), WM_KEYDOWN, VK_RETURN, 0);
                     SendMessageW(app.Window(), WM_CHAR, L'\r', 0);
                     const std::wstring expected = original + std::wstring(eol);
                     Require(app.Document().GetText() == expected &&
                                 app.Document().GetSelection() == DxUi::RichText::Selection{expected.size(), expected.size()},
                             "native Source Enter and Shift+Enter preserve LF, CRLF and CR with exact ending ACP");
                     Require(app.Activity().changes == before.changes + 1 && app.Activity().recoverySubmissions == before.recoverySubmissions + 1,
                             "each native Source Enter publishes exactly one edit and recovery submission");
                     Require(app.Document().Undo() && app.Document().GetText() == original &&
                                 app.Document().GetSelection() == DxUi::RichText::Selection{end, end} && !app.Dirty(),
                             "one undo restores exact saved source, caret and clean state");
                     Require(app.Document().Redo() && app.Document().GetText() == expected, "Source Enter redo preserves the same terminator");
                     Require(!IsWindowVisible(app.Window()) && foreground.NeverAcquiredForeground(), "Source Enter never shows or activates its app");
                 }
             Require(foreground.NeverAcquiredForeground(), "Source Enter teardown never acquires foreground");
         });
    test("Editor.NativeTextStoreCapacityAdmission",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             wil::com_ptr<ITextStoreACP> store;
             {
                 // A hidden HWND intentionally has no live native input session.
                 // Exercise the same consumer Editor in the SDK's headless native host;
                 // the independent application case below covers preview/mirror/recovery.
                 std::size_t notices{};
                 DxUi::ControlHost host;
                 const std::wstring original(65530u, L'x');
                 auto owned = std::make_unique<Editor>(original);
                 auto &editor = *owned;
                 host.SetRoot(std::move(owned));
                 editor.SetSelection({65530u, 65530u});
                 editor.SetOnTextChanged([&] { ++notices; });
                 host.SetFocusControl(&editor, false);
                 store.attach(host.DebugCreateNativeTextInputTextStoreForTest());
                 Require(store != nullptr, "the adopted pin creates the actual native ACP store");
                 LONG first{}, last{};
                 Require(store->QueryInsert(65530, 65530, 10u, &first, &last) == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW),
                         "native store refuses 65540 UTF16 units before staging");
                 Require(editor.GetText() == original && !editor.CanUndo(), "query refusal preserves exact source and history");
                 auto sink = Testing::NativeTextStoreSink::Create();
                 Require(sink != nullptr, "allocate owned native-store sink");
                 Success(store->AdviseSink(__uuidof(ITextStoreACPSink), sink.get(), TS_AS_TEXT_CHANGE | TS_AS_SEL_CHANGE),
                         "advise consumer native-store sink");
                 const auto unadvise = wil::scope_exit([&] { static_cast<void>(store->UnadviseSink(sink.get())); });
                 struct WriteAttempt
                 {
                     ITextStoreACP *store;
                     std::wstring_view overflow, fitting;
                     HRESULT refused{E_UNEXPECTED}, accepted{E_UNEXPECTED};
                 } attempt{store.get(), L"0123456789", L"abc"};
                 sink->context = &attempt;
                 sink->action = [](void *context) noexcept
                 {
                     auto &value = *static_cast<WriteAttempt *>(context);
                     TS_TEXTCHANGE change{};
                     LONG begin{}, end{};
                     value.refused = value.store->InsertTextAtSelection(0u, value.overflow.data(), static_cast<ULONG>(value.overflow.size()), &begin,
                                                                        &end, &change);
                     value.accepted = value.store->InsertTextAtSelection(0u, value.fitting.data(), static_cast<ULONG>(value.fitting.size()), &begin,
                                                                         &end, &change);
                     return value.accepted;
                 };
                 const auto before = notices;
                 HRESULT session{E_UNEXPECTED};
                 Success(store->RequestLock(TS_LF_READWRITE | TS_LF_SYNC, &session), "request native capacity write lock");
                 Success(session, "native in-range edit remains committable after refusal");
                 Require(attempt.refused == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) && attempt.accepted == S_OK &&
                             editor.GetText() == original + L"abc",
                         "overflow preserves the last in-range state in the consumer editor");
                 attempt.overflow = L"1234";
                 attempt.fitting = L"def";
                 Success(store->RequestLock(TS_LF_READWRITE | TS_LF_SYNC, &session), "request exact-capacity native write lock");
                 Success(session, "exact remaining capacity commits");
                 sink->action = nullptr;
                 Require(attempt.refused == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) && attempt.accepted == S_OK &&
                             editor.GetText() == original + L"abcdef" && editor.GetSelection() == DxUi::RichText::Selection{65536u, 65536u},
                         "native store admits exactly 65536 units and preserves current source and ACPs");
                 Require(notices == before + 2, "only the two accepted native writes notify the consumer editor");
                 Require(editor.Undo() && editor.GetText() == original + L"abc" && editor.Undo() && editor.GetText() == original && !editor.CanUndo(),
                         "each native write is one independently undoable edit");
                 Require(host.GetHwnd() == nullptr && foreground.NeverAcquiredForeground(),
                         "native-store capacity check uses no HWND or foreground input");
             }
             LONG first{}, last{};
             Require(FAILED(store->QueryInsert(0, 0, 1u, &first, &last)),
                     "retained native store disconnects after its consumer editor host is destroyed");
             Require(foreground.NeverAcquiredForeground(), "native-store teardown never acquires foreground");
         });
    test("Editor.TextServiceCapacityPreservesPreviewAndMirror",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             {
                 LaunchOptions options;
                 options.headless = true;
                 options.dataDirectory = root;
                 Application app(options);
                 Success(app.Initialize(), "initialize hidden capacity preview app");
                 const std::wstring original(65530u, L'x');
                 app.SetDocument(original);
                 app.SetView(EditorView::Split);
                 app.Document().SetSelection({65530u, 65530u});
                 DxUi::TextInputState entry;
                 entry.text = original;
                 entry.multiline = true;
                 entry.caretIndex = 65530u;
                 auto preview = entry;
                 preview.text += L"\u4eee\U0001F600x";
                 preview.caretIndex = 65534u;
                 const auto before = app.Activity();
                 Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), preview, false),
                         "last in-range application preview is accepted");
                 const auto snapshot = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 auto overflow = entry;
                 overflow.text += L"0123456789";
                 overflow.caretIndex = 65540u;
                 Require(!(app.Document().*TextServiceImportProbe::Import)(app.Host(), overflow, false),
                         "oversized application preview declines atomically");
                 Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection &&
                             app.Document().GetText() == preview.text && app.SourceMirror().GetText() == preview.text &&
                             app.SourceMirror().GetSelection() == selection && !app.Document().CanUndo() && !app.Document().CanRedo(),
                         "overflow keeps the last in-range Unicode preview, exact ACPs, history and mirror");
                 Require(app.Activity().changes == before.changes && app.Activity().recoverySubmissions == before.recoverySubmissions,
                         "preview and refusal do not publish committed edits or recovery");
                 auto exact = entry;
                 exact.text += L"\u4eee\U0001F600xyz";
                 exact.caretIndex = 65536u;
                 Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), exact, false) && app.Document().GetText() == exact.text &&
                             app.SourceMirror().GetText() == exact.text,
                         "exact-capacity application preview and mirror remain usable after refusal");
                 Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), entry, false),
                         "restore entry source through the existing commit protocol");
                 Require((app.Document().*TextServiceImportProbe::Import)(app.Host(), exact, true), "commit the exact-capacity result once");
                 Require(app.Document().GetText() == exact.text && app.SourceMirror().GetText() == exact.text &&
                             app.Document().GetSelection() == DxUi::RichText::Selection{65536u, 65536u} &&
                             app.Activity().changes == before.changes + 1 && app.Activity().recoverySubmissions == before.recoverySubmissions + 1,
                         "one exact-capacity result synchronizes source, ACPs, mirror and recovery");
                 Require(app.Document().Undo() && app.Document().GetText() == original && app.SourceMirror().GetText() == original && !app.Dirty() &&
                             app.Document().Redo() && app.Document().GetText() == exact.text && app.SourceMirror().GetText() == exact.text,
                         "one undo and redo preserve the complete exact-capacity result");
                 Require(!IsWindowVisible(app.Window()) && foreground.NeverAcquiredForeground(),
                         "application capacity previews stay hidden without native focus");
             }
             Require(foreground.NeverAcquiredForeground(), "capacity preview teardown never acquires foreground");
         });
    test("Editor.FilePasteAdmissionRejectsHostileNames",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             LaunchOptions options;
             options.headless = true;
             options.dataDirectory = root;
             Application app(options);
             Success(app.Initialize(), "initialize hidden file-paste admission app");
             for (const auto view : {EditorView::Compose, EditorView::Source, EditorView::Split})
             {
                 const std::wstring original = L"Safe observation.\r\n";
                 app.SetDocument(original);
                 app.SetView(view);
                 app.Document().SetSelection({5u, 5u});
                 const auto snapshot = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 const auto before = app.Activity();
                 for (const auto &path :
                      {std::wstring(L"C:\\SyntheticNotes\\draft.png\"\r\n![intruder](https://assets.example.invalid/pixel.png)\r\n\""),
                       std::wstring(L"C:\\SyntheticNotes\\quote\".png"), std::wstring(L"C:\\SyntheticNotes\\line\nbreak.png")})
                 {
                     DxUi::TextClipboardContent content;
                     content.filePaths.push_back(path);
                     std::wstring replacement = L"unchanged";
                     const bool rejected = !app.Document().TryPrepareClipboardPaste(content, replacement);
                     Require(rejected && replacement == L"unchanged", "hostile file paste declines before producing any Markdown replacement");
                     Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == selection &&
                                 !app.Document().CanUndo() && !app.Document().CanRedo() && app.Activity().changes == before.changes &&
                                 app.Activity().recoverySubmissions == before.recoverySubmissions,
                             "declined file paste preserves source, selection, history and recovery in every view");
                 }
                 DxUi::TextClipboardContent ordinary;
                 ordinary.filePaths.push_back(L"C:\\SyntheticNotes\\safe.png");
                 std::wstring replacement;
                 Require(app.Document().TryPrepareClipboardPaste(ordinary, replacement) && !replacement.empty(),
                         "the same app still prepares ordinary file paste without changing source");
                 Require(app.Document().GetText() == original && app.Document().Snapshot().IsSameRevision(snapshot),
                         "file preparation performs no source edit");
             }
             Require(!IsWindowVisible(app.Window()) && foreground.NeverAcquiredForeground(),
                     "file-paste preparation never uses desktop input or clipboard");
         });
    test("Editor.ViewSwitchKeepsSourceSelectionHistory",
         [&]
         {
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root;
             Application app(launch);
             Success(app.Initialize(), "create hidden app");
             Testing::ForegroundOwnershipObserver foreground;
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
             Require(foreground.NeverAcquiredForeground(), "hidden app never acquires foreground");
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
             Require(app.SourceMirror().Snapshot().IsSameRevision(current), "visible Split shares the immutable primary source tree");
             Require(app.Document().Snapshot().IsSameRevision(current) && app.Document().CanUndo(),
                     "synchronization preserves the editable source revision and history");
             const auto mirrored = app.SourceMirror().Snapshot();
             app.SetView(EditorView::Split);
             Require(app.SourceMirror().Snapshot().IsSameRevision(mirrored), "reselecting Split preserves the current mirror revision");
             const auto split = app.Activity();
             Require(app.Document().ReplaceSelection(L"Split "), "edit Split prose");
             Require(app.SourceMirror().GetText() == app.Document().GetText() && app.Activity().mirrorResets == split.mirrorResets + 1,
                     "each visible Split revision updates its mirror");
             Require(app.SourceMirror().Snapshot().IsSameRevision(app.Document().Snapshot()), "visible edits retain exact shared source identity");
             Require(app.Activity().layouts == split.layouts, "Split prose does not relayout chrome");
             const auto committed = app.Activity();
             const auto committedSource = app.Document().Snapshot();
             const auto committedSelection = app.Document().GetSelection();
             app.Prepare();
             const auto prepared = app.Activity();
             const auto primarySyntax = app.Document().GetMarkdownSyntax();
             const auto mirrorSyntax = app.SourceMirror().GetMarkdownSyntax();
             Require(primarySyntax && mirrorSyntax && primarySyntax != mirrorSyntax && primarySyntax->Revision() == app.Document().Revision() &&
                         mirrorSyntax->Revision() == app.SourceMirror().Revision() &&
                         primarySyntax->DocumentIdentity() != mirrorSyntax->DocumentIdentity() && &primarySyntax->Nodes() == &mirrorSyntax->Nodes() &&
                         &primarySyntax->Inlines() == &mirrorSyntax->Inlines(),
                     "the read-only mirror shares complete parsed storage under its own revision and document identity");
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
                 Testing::ForegroundOwnershipObserver foregroundBefore;
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
                     // A changed transient preview declines undo/redo; cancellation below restores the seeded history.
                     Require(historyCanRedo && !imeApp.Document().CanUndo() && !imeApp.Document().CanRedo(),
                             "undo and redo decline while a changed IME preview is active");

                     auto nextPreview = preview;
                     nextPreview.text[fixture.selection.Start()] = L'Y';
                     Require((imeApp.Document().*TextServiceImportProbe::Import)(imeApp.Host(), nextPreview, false),
                             "successive same-ACP text-service candidate is accepted");
                     Require(imeApp.Document().GetText() == nextPreview.text && imeApp.SourceMirror().GetText() == nextPreview.text &&
                                 imeApp.Document().GetSelection() == previewSelection && imeApp.SourceMirror().GetSelection() == previewSelection,
                             "successive text-service candidate synchronizes its source even with unchanged ACPs");
                     Require(imeApp.Activity().changes == beforePreview.changes &&
                                 imeApp.Activity().recoverySubmissions == beforePreview.recoverySubmissions && !imeApp.Document().CanUndo() &&
                                 !imeApp.Document().CanRedo(),
                             "successive same-ACP preview preserves committed activity and keeps history declined");

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
                     Require(!IsWindowVisible(appWindow) && foregroundBefore.NeverAcquiredForeground(),
                             "hidden IME regression never shows or activates its app window");
                 }
                 Require(foregroundBefore.NeverAcquiredForeground(), "hidden IME regression never acquires desktop foreground");
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
    test("Recovery.DuplicateCloseKeepsUnsavedPrompt",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             Application *owner{};
             unsigned prompts{};
             int response = IDCANCEL;
             bool inside{}, ignored{};
             LaunchOptions launch;
             launch.headless = true;
             launch.dataDirectory = root / L"duplicate-close-prompt";
             launch.unsavedPromptForTest = [&]
             {
                 ++prompts;
                 if (inside || response != IDCANCEL)
                     return response;
                 inside = true;
                 const auto window = owner->Window();
                 (void)SendMessageW(window, WM_CLOSE, 0, 0);
                 owner->Execute(Command::Close);
                 ignored = prompts == 1 && owner->Window() == window && IsWindow(window);
                 inside = false;
                 return response;
             };
             Application app(std::move(launch));
             owner = &app;
             Success(app.Initialize(), "create hidden duplicate-close app");
             app.SetDocument(L"Notebook review");
             app.Document().SetSelection({15, 15});
             Require(app.Document().ReplaceSelection(L"!"), "admit dirty source for the unsaved prompt");
             const auto source = app.Document().Snapshot();
             const auto selection = app.Document().GetSelection();
             const auto window = app.Window();
             (void)SendMessageW(window, WM_CLOSE, 0, 0);
             Require(prompts == 1 && ignored && app.Window() == window && IsWindow(window),
                     "nested native and command Close retain the original prompt and owner");
             Require(app.Dirty() && app.Document().Snapshot().IsSameRevision(source) && app.Document().GetSelection() == selection &&
                         app.Document().CanUndo(),
                     "Cancel preserves the exact dirty revision, selection and undo history");
             response = IDNO;
             (void)SendMessageW(window, WM_CLOSE, 0, 0);
             Require(prompts == 2 && app.Host().GetHwnd() == nullptr,
                     "a fresh Close after Cancel asks again and honors the original Discard continuation");
             Require(foreground.NeverAcquiredForeground(), "the hidden prompt regression preserves desktop foreground");
         });
    test("Recovery.RestoreKeepsClickedIdentityAcrossUnsavedPrompt",
         [&]
         {
             Testing::ForegroundOwnershipObserver foreground;
             // Try the nonempty remainder first: the old continuation restores
             // the wrong copy without needing to dereference an empty vector.
             for (const unsigned survivors : {1u, 0u})
             {
                 const auto directory = root / std::format(L"restore-clicked-{}", survivors);
                 std::filesystem::create_directories(directory / L"Recovery");
                 std::vector<std::filesystem::path> candidates;
                 std::vector<RecoverySnapshot> snapshots;
                 for (unsigned index = 0; index <= survivors; ++index)
                 {
                     candidates.push_back(directory / L"Recovery" / std::format(L"session-{:032x}.rtd", index + 1));
                     snapshots.push_back({std::format(L"Notebook copy {} — 東京\r\n", index),
                                          directory / std::format(L"notebook-{}.md", index),
                                          index != 0,
                                          {2, 5},
                                          index + 1});
                     Success(WriteBytesAtomic(candidates.back(), SerializeRecovery(snapshots.back())), "seed inactive recovery copy");
                 }
                 Application *owner{};
                 unsigned prompts{};
                 std::function<void(std::size_t, std::string_view)> settle;
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.dataDirectory = directory;
                 launch.unsavedPromptForTest = [&]
                 {
                     if (++prompts == 1)
                     {
                         const auto checkpoints = owner->Activity().recoverySubmissions;
                         owner->Execute(Command::DiscardRecovery);
                         settle(survivors, "discard inside the unsaved prompt");
                         Require(owner->RecoveryCount() == survivors, "the clicked candidate retires while the unsaved prompt is open");
                         if (survivors)
                             Require(owner->Activity().recoverySubmissions > checkpoints,
                                     "drawer refresh checkpoints the live dirty document before the wait accepts settlement");
                     }
                     return IDNO;
                 };
                 Application app(std::move(launch));
                 owner = &app;
                 settle = [&](std::size_t expected, std::string_view phase)
                 {
                     for (unsigned turn = 0; turn < 1000; ++turn)
                     {
                         // Publishing candidate retirement refreshes the drawer,
                         // which can checkpoint again. Check terminal facts after
                         // pumping UI, with no later pump invalidating the wait.
                         app.Prepare();
                         if (app.RecoveryReady() && app.RecoverySettled() && app.RecoveryCount() == expected)
                         {
                             Require(SUCCEEDED(app.RecoveryStatus()), "the recovery worker settles successfully");
                             return;
                         }
                         std::this_thread::sleep_for(std::chrono::milliseconds(5));
                     }
                     throw std::runtime_error(std::format("Recovery did not settle after {}: ready={}, settled={}, candidates={} (expected {}), "
                                                          "status=0x{:08X}, submissions={}",
                                                          phase, app.RecoveryReady(), app.RecoverySettled(), app.RecoveryCount(), expected,
                                                          static_cast<std::uint32_t>(app.RecoveryStatus()), app.Activity().recoverySubmissions));
                 };
                 Success(app.Initialize(), "create hidden Restore identity app");
                 settle(survivors + 1, "initial candidate scan");
                 Require(app.RecoveryCount() == survivors + 1, "all inactive candidates are offered");
                 app.SetDocument(L"Current notebook");
                 app.Document().SetSelection({16, 16});
                 Require(app.Document().ReplaceSelection(L" continued"), "make the current notebook dirty");
                 const auto current = app.Document().Snapshot();
                 const auto selection = app.Document().GetSelection();
                 app.Execute(Command::Recovery);
                 app.Execute(Command::RestoreRecovery);
                 Require(prompts == 1 && app.RecoveryCount() == survivors && !app.RequiresSaveAs(),
                         "a vanished clicked candidate cannot restore or retire another copy");
                 Require(app.Dirty() && app.Document().Snapshot().IsSameRevision(current) && app.Document().GetSelection() == selection &&
                             app.Document().CanUndo(),
                         "stale Restore preserves the exact current revision, selection and undo history");
                 if (survivors)
                 {
                     const auto remaining = std::filesystem::exists(candidates[0]) ? 0u : 1u;
                     Require(std::filesystem::exists(candidates[remaining]), "the unchosen recovery copy remains on disk");
                     app.Execute(Command::RestoreRecovery);
                     Require(prompts == 2 && app.RecoveryCount() == 0 && app.RequiresSaveAs() && app.Dirty() &&
                                 app.Document().GetText() == snapshots[remaining].text &&
                                 app.Document().GetSelection() == snapshots[remaining].selection,
                             "a fresh Restore can explicitly choose the remaining exact copy");
                     settle(0, "fresh Restore durability");
                 }
                 Require(foreground.NeverAcquiredForeground(), "hidden Restore identity coverage preserves desktop foreground");
             }
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
// The saved app-owned WNDPROC follows the Win32 no-exception contract; SDK WNDPROC omits noexcept.
// Owner: RedTilde maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 5039)
                             (void)SetWindowLongPtrW(context.window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(context.previous));
#pragma warning(pop)
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
// The saved app-owned WNDPROC follows the Win32 no-exception contract; SDK WNDPROC omits noexcept.
// Owner: RedTilde maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 5039)
                 const auto result = CallWindowProcW(state.previous, hwnd, message, wParam, lParam);
#pragma warning(pop)
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
// The saved app-owned WNDPROC follows the Win32 no-exception contract; SDK WNDPROC omits noexcept.
// Owner: RedTilde maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 5039)
             context.previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(app.Window(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(intercept)));
#pragma warning(pop)
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
             Testing::ForegroundOwnershipObserver foreground;
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
             Require(foreground.NeverAcquiredForeground(), "hidden native-close regression preserves foreground focus");
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
    test("Editor.SelectedBlockInsertionsRetainSourceAndHistory",
         [&]
         {
             struct Fixture
             {
                 std::wstring_view source, selected, quote, code;
             };
             constexpr std::array fixtures{Fixture{L"Before selected after", L"selected", L"Before \r\n\r\n> selected\r\n\r\n after",
                                                   L"Before \r\n\r\n```text\r\nselected\r\n```\r\n\r\n after"},
                                           Fixture{L"Title\r\n\r\nα 😀\r\n第二行\r\n\r\nTail\r\n", L"α 😀\r\n第二行",
                                                   L"Title\r\n\r\n> α 😀\r\n> 第二行\r\n\r\nTail\r\n",
                                                   L"Title\r\n\r\n```text\r\nα 😀\r\n第二行\r\n```\r\n\r\nTail\r\n"}};
             {
                 Testing::ForegroundOwnershipObserver negativeControl;
                 Require(negativeControl.NeverAcquiredForeground(), "hidden observer starts without activation");
                 Testing::ForegroundOwnershipObserverProbe::ReplayOwnedEvent(negativeControl);
                 Require(!negativeControl.NeverAcquiredForeground(), "an owned foreground event is a failing no-activation observation");
             }
             Testing::ForegroundOwnershipObserver foreground;
             for (const auto view : {Command::Compose, Command::Source, Command::Split})
             {
                 LaunchOptions launch;
                 launch.headless = true;
                 launch.dataDirectory = root / L"selected-blocks";
                 Application app(launch);
                 Success(app.Initialize(), "create hidden selected-block app");
                 app.Execute(view);
                 for (const auto &fixture : fixtures)
                     for (const auto command : {Command::Quote, Command::Fence})
                     {
                         app.SetDocument(std::wstring(fixture.source));
                         app.Prepare(128);
                         const auto first = fixture.source.find(fixture.selected);
                         Require(first != fixture.source.npos, "fixture selects real source");
                         const DxUi::RichText::Selection before{first + fixture.selected.size(), first};
                         app.Document().SetSelection(before);
                         const auto snapshot = app.Document().Snapshot();
                         app.Execute(command);
                         Require(app.Document().GetText() == (command == Command::Quote ? fixture.quote : fixture.code) && app.Dirty(),
                                 "Insert transforms exact selected content and retains all outside source");
                         const auto after = app.Document().GetSelection();
                         Require(after.anchor > after.caret && app.Host().GetFocusControl() == &app.Document(),
                                 "Insert retains selection direction and logical editor focus");
                         const auto transformed = app.Document().Snapshot();
                         app.Execute(Command::Undo);
                         Require(app.Document().Snapshot().IsSameRevision(snapshot) && app.Document().GetSelection() == before && !app.Dirty() &&
                                     !app.Document().CanUndo(),
                                 "one application Undo restores exact source, direction and clean saved state");
                         app.Execute(Command::Redo);
                         Require(app.Document().Snapshot().IsSameRevision(transformed) && app.Document().GetSelection() == after && app.Dirty(),
                                 "one application Redo restores the whole selection transformation");
                     }
                 app.SetDocument({});
                 app.Execute(Command::Quote);
                 Require(app.Document().GetText() == L"\r\n> A thought worth keeping.\r\n",
                         "empty-selection Quote uses the default CRLF document terminator");
                 app.SetDocument({});
                 app.Execute(Command::Fence);
                 Require(app.Document().GetText() == L"\r\n\r\n```text\r\nYour code here\r\n```\r\n\r\n",
                         "empty-selection Code uses the default CRLF document terminator");
                 for (const auto &[source, eol] : std::array<std::pair<std::wstring_view, std::wstring_view>, 5>{{{L"Intro\nTail\n", L"\n"},
                                                                                                                  {L"Intro\r\nTail\r\n", L"\r\n"},
                                                                                                                  {L"Intro\rTail\r", L"\r"},
                                                                                                                  {L"Intro\r\nTail\n", L"\r\n"},
                                                                                                                  {L"Intro", L"\r\n"}}})
                     for (const auto command : {Command::Quote, Command::Fence, Command::Mermaid, Command::Math})
                     {
                         app.SetDocument(std::wstring(source));
                         app.Document().SetSelection({source.size(), source.size()});
                         const auto original = app.Document().Snapshot();
                         std::wstring inserted;
                         if (command == Command::Quote)
                             inserted = std::format(L"{0}> A thought worth keeping.{0}", eol);
                         else if (command == Command::Fence)
                             inserted = std::format(L"{0}{0}```text{0}Your code here{0}```{0}{0}", eol);
                         else if (command == Command::Mermaid)
                             inserted = std::format(L"{0}{0}```mermaid{0}graph LR{0}A[Idea] --> B[Write]{0}B --> C[Share]{0}```{0}{0}", eol);
                         else
                             inserted = std::format(L"{0}{0}$${0}x = \\frac{{-b + \\sqrt{{b^2 - 4ac}}}}{{2a}}{0}$${0}{0}", eol);
                         app.Execute(command);
                         Require(app.Document().GetText() == std::wstring(source) + inserted && app.Dirty(),
                                 "all four real Insert templates use the first document terminator while preserving mixed outside source");
                         Require(app.Document().Undo() && app.Document().Snapshot().IsSameRevision(original) && !app.Document().CanUndo() &&
                                     !app.Dirty(),
                                 "each normalized template is one exact-source undoable edit in every view");
                     }
                 app.SetDocument(L"Protected words");
                 app.Document().SetSelection({9u, 0u});
                 app.Document().SetReadOnly(true);
                 const auto protectedSource = app.Document().Snapshot();
                 for (const auto command : {Command::Quote, Command::Fence})
                 {
                     app.Execute(command);
                     Require(app.Document().Snapshot().IsSameRevision(protectedSource) &&
                                 app.Document().GetSelection() == DxUi::RichText::Selection{9u, 0u} && !app.Dirty() && !app.Document().CanUndo(),
                             "a refused selected Insert retains protected writing instead of using a sample template");
                 }
                 app.Document().SetReadOnly(false);
                 app.SetDocument(std::wstring(DxUi::RichText::Document::MaximumUnits, L'x'));
                 app.Document().SetSelection({4u, 1u});
                 const auto fullSource = app.Document().Snapshot();
                 for (const auto command : {Command::Quote, Command::Fence})
                 {
                     app.Execute(command);
                     Require(app.Document().Snapshot().IsSameRevision(fullSource) &&
                                 app.Document().GetSelection() == DxUi::RichText::Selection{4u, 1u} && !app.Dirty() && !app.Document().CanUndo(),
                             "capacity refusal never replaces selected writing with the shorter sample template");
                 }
             }
             Require(foreground.NeverAcquiredForeground(), "headless block insertion never acquires desktop foreground");
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
             struct EolDraft
             {
                 std::wstring_view source, selected, eol;
             };
             constexpr EolDraft drafts[] = {{L"First\r\nSecond\r\n\r\nTail\n", L"First\r\nSecond", L"\r\n"},
                                            {L"First\nSecond\n\nTail\r\n", L"First\nSecond", L"\n"},
                                            {L"First\rSecond\r\rTail\n", L"First\rSecond", L"\r"},
                                            {L"First\nSecond\r\n\r\nTail\r", L"First\nSecond", L"\n"},
                                            {L"First Second", L"First Second", L"\r\n"}};
             unsigned index{};
             for (const auto &draft : drafts)
             {
                 auto isolated = launch;
                 isolated.dataDirectory = root / L"draft-eol" / std::to_wstring(index++);
                 Application target(isolated);
                 Success(target.Initialize(), "create an isolated draft line-ending app");
                 target.SetDocument(std::wstring(draft.source), {}, true);
                 target.Prepare();
                 const DxUi::RichText::Selection before{0, draft.selected.size()};
                 target.Document().SetSelection(before);
                 const auto snapshot = target.Document().Snapshot();
                 target.Execute(Command::LocalSource);
                 Require(target.HasBlockDraft(), "ordinary multiline selection opens a guarded draft");
                 target.Execute(Command::ApplyBlock);
                 Require(target.Document().Snapshot().IsSameRevision(snapshot) && !target.Dirty() && !target.Document().CanUndo(),
                         "unchanged F2 Apply preserves exact mixed line endings and history");
                 target.Prepare();
                 target.Document().SetSelection(before);
                 target.Execute(Command::LocalSource);
                 target.BlockDraft()->SetText(L"Changed\nNext");
                 target.CheckpointRecovery(true);
                 for (unsigned turn = 0; turn < 1000 && !target.RecoverySettled(); ++turn)
                 {
                     target.Prepare();
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 Require(target.RecoverySettled() && SUCCEEDED(target.RecoveryStatus()), "normalized draft recovery settles successfully");
                 const auto replacement = std::wstring(L"Changed") + std::wstring(draft.eol) + L"Next";
                 const auto expected = replacement + std::wstring(draft.source.substr(draft.selected.size()));
                 unsigned copies{};
                 for (const auto &entry : std::filesystem::directory_iterator(isolated.dataDirectory / L"Recovery"))
                     if (entry.path().extension() == L".rtd")
                     {
                         std::string bytes;
                         RecoverySnapshot proposed;
                         Success(ReadBytes(entry.path(), bytes), "read normalized draft checkpoint");
                         Success(ParseRecovery(bytes, proposed), "parse normalized draft checkpoint");
                         Require(proposed.text == expected && proposed.bom && proposed.selection == DxUi::RichText::Selection{0, replacement.size()},
                                 "recovery previews the same normalized insertion and unchanged outside source as Apply");
                         ++copies;
                     }
                 Require(copies == 1 && target.Document().Snapshot().IsSameRevision(snapshot), "draft checkpoint never commits live source");
                 target.Execute(Command::ApplyBlock);
                 Require(target.Document().GetText() == expected && target.Document().Undo() &&
                             target.Document().Snapshot().IsSameRevision(snapshot) && target.Document().GetSelection() == before &&
                             !target.Document().CanUndo(),
                         "F2 Apply uses document endings and one Undo restores exact source and selection");
             }
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
             app.SetDocument(L"# Image\n\n![Actual app capture](after-device-loss.png)\n", root / L"image-document.md");
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
                                         L"$$\r\n\\left(\\frac{1}{2}\\right)+\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\r\n$$\r\n";
             app.SetDocument(source);
             app.Prepare(1024);
             auto semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(semantic && std::ranges::count(semantic->Elements(), SemanticRole::Math, &SemanticElement::role) == 2,
                     "both inline and own-line display equations are native document objects");
             app.SetDocument(L"$$x$$\r\n");
             app.Prepare(1024);
             semantic = app.Document().GetAcknowledgedSemanticDocument(app.Host());
             Require(semantic && std::ranges::count(semantic->Elements(), SemanticRole::Math, &SemanticElement::role) == 0,
                     "the pinned grammar keeps one-line double-dollar text literal");
             app.SetDocument(source);
             app.Prepare(1024);
             const auto first = source.find(L"$a_i^2$");
             app.Document().SetSelection({first + 2, first + 2});
             app.Execute(Command::LocalSource);
             auto region = app.Document().GetSemanticSourceRegion();
             Require(region && region->source.first == first && region->source.last == first + 7 && !app.HasBlockDraft(),
                     "F2 shows exact inline math delimiters in place rather than the whole paragraph");
             app.Document().SetSelection({first + 5, first + 6});
             Require(app.Document().ReplaceSelection(L"3"), "edit the inline equation in place");
             app.Execute(Command::LocalSource);
             Require(!app.Document().GetSemanticSourceRegion() && app.Document().GetText().find(L"$a_i^3$") != std::wstring::npos &&
                         app.Document().Undo() && app.Document().GetText() == source,
                     "in-place equation edit and undo preserve the rest and all original EOLs");
             app.Prepare(1024);
             const auto displayCaret = source.find(L"\\left");
             app.Document().SetSelection({displayCaret, displayCaret});
             app.Execute(Command::LocalSource);
             region = app.Document().GetSemanticSourceRegion();
             const auto displayFirst = source.find(L"$$"), displayLast = source.rfind(L"$$") + 2;
             Require(region && region->source.first == displayFirst && region->source.last >= displayLast && region->source.last <= source.size() &&
                         !app.HasBlockDraft(),
                     std::format("F2 shows the complete own-line display envelope in place: region={} [{},{}] expected [{},{}+]", region.has_value(),
                                 region ? region->source.first : 0u, region ? region->source.last : 0u, displayFirst, displayLast)
                         .c_str());
             Require(app.EquationPreviewText() == L"\\[\n\\left(\\frac{1}{2}\\right)+\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\n\\]",
                     "in-place display source previews its TeX body in the inspector");
             app.Execute(Command::LocalSource);
             Require(!app.Document().GetSemanticSourceRegion() && app.Document().GetText() == source,
                     "leaving unedited display source keeps the document unchanged");
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
                 Require(app.EquationPreviewText() == L"\\[\n" + notation + L"\n\\]", "current syntax drives either math delimiter family");
             }
             const std::wstring display = L"Before.\r\n\r\n$$\r\nx^2\r\n$$\r\n\r\n\\[\r\ny_1\r\n\\]\r\n";
             app.SetDocument(display);
             app.Prepare(1024);
             for (const auto &notation : {std::wstring(L"x^2"), std::wstring(L"y_1")})
             {
                 const auto math = display.find(notation);
                 app.Document().SetSelection({math, math});
                 Require(app.EquationPreviewText() == L"\\[\n" + notation + L"\n\\]",
                         "own-line $$ and \\[ \\] display blocks drive the inspector without their line terminators");
             }
             const auto afterDisplay = display.find(L"$$\r\n\r\n") + 4;
             app.Document().SetSelection({afterDisplay, afterDisplay});
             Require(app.EquationPreviewText().empty(), "the line after a closing display delimiter is outside the equation");
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
             Success(WriteBytesAtomic(root / L"owned-heap-before.json", before.second), "retain the initial owned-heap report before assertions");
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
                 std::fill_n(static_cast<std::byte *>(allocation), allocationBytes, std::byte{0x5A});
                 const auto busy = sample();
                 Success(WriteBytesAtomic(root / L"owned-heap-busy.json", busy.second), "retain the allocated owned-heap report before assertions");
                 struct NativeMembership
                 {
                     bool complete{}, represented{};
                     std::size_t entries{};
                 };
                 // Independent membership oracle, rather than a second implementation of byte accounting.
                 const auto nativeOwnership = [&]
                 {
                     if (!HeapLock(heap.get()))
                         return NativeMembership{};
                     const auto unlock = wil::scope_exit([&] { (void)HeapUnlock(heap.get()); });
                     PROCESS_HEAP_ENTRY entry{};
                     bool represented{};
                     const auto address = reinterpret_cast<std::uintptr_t>(allocation);
                     for (std::size_t index = 0; index <= Test::MaximumDiagnosticEntriesPerHeap; ++index)
                     {
                         if (!HeapWalk(heap.get(), &entry))
                             return NativeMembership{GetLastError() == ERROR_NO_MORE_ITEMS, represented, index};
                         if (index == Test::MaximumDiagnosticEntriesPerHeap)
                             return NativeMembership{false, represented, index + 1u};
                         const auto first = reinterpret_cast<std::uintptr_t>(entry.lpData);
                         if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) && entry.cbData >= allocationBytes && address >= first &&
                             address - first <= entry.cbData - allocationBytes)
                             represented = true;
                     }
                     return NativeMembership{false, represented, 0u};
                 }();
                 const auto busyBytes = JsonUnsigned(JsonObjectAfter(busy.second, "\"totals\":"), "busyPayloadBytes");
#if defined(__SANITIZE_ADDRESS__)
                 // ASan may redirect HeapAlloc and quarantine physical storage outside the supplied native heap.
                 constexpr bool sanitizerAllocator = true;
#else
                 constexpr bool sanitizerAllocator = false;
#endif
                 Require(busy.first && nativeOwnership.complete, "both the owned-heap capture and independent membership walk complete");
                 if constexpr (!sanitizerAllocator)
                     Require(nativeOwnership.represented && busyBytes >= beforeBusy + allocationBytes,
                             "native heap payload includes the independently observed live allocation");
                 Success(
                     WriteBytesAtomic(root / L"owned-heap-allocation-observation.json",
                                      std::format("{{\"sanitizerAllocator\":{},\"logicalAllocationBytes\":{},\"nativeHeapRepresentsAllocation\":{},"
                                                  "\"nativeWalkedEntries\":{},\"busyPayloadBefore\":{},\"busyPayloadAfter\":{}}}",
                                                  sanitizerAllocator, allocationBytes, nativeOwnership.represented, nativeOwnership.entries,
                                                  beforeBusy, busyBytes)),
                     "retain physical versus logical heap observation without claiming process-wide coverage");
                 const auto partial = sample(1);
                 const bool mustTruncate = nativeOwnership.entries > 1u;
                 Success(WriteBytesAtomic(root / L"owned-heap-native-boundaries.json",
                                          std::format("{{\"independentNativeEntries\":{},\"completeWalk\":{},\"oneEntryBudget\":{}}}",
                                                      nativeOwnership.entries, busy.second, partial.second)),
                         "retain complete and bounded native reports before their assertions");
                 Require(partial.first == !mustTruncate && (partial.second.find("\"budgetExceeded\":true") != partial.second.npos) == mustTruncate &&
                             JsonUnsigned(partial.second, "entryLimit") == 1,
                         "the bounded native walk agrees with the independently observed entry count, including empty sanitizer heaps");
                 const auto exact = sample(std::max<std::size_t>(1u, nativeOwnership.entries));
                 Require(exact.first, "an exact entry-limit boundary recognizes end-of-heap after the final allowed entry");
             }
             const auto freed = sample();
             Success(WriteBytesAtomic(root / L"owned-heap-freed.json", freed.second), "retain the freed owned-heap report before assertions");
             // Sanitizer quarantine and heap managers may retain physical blocks after a successful logical free.
             Require(allocationReleased && freed.first,
                     "release succeeds and physical heap state remains measurable without requiring purge or working-set trimming");
             struct ScriptedCalls
             {
                 HANDLE expected{};
                 std::span<const PROCESS_HEAP_ENTRY> entries;
                 std::size_t cursor{}, reads{}, locks{}, unlocks{};
                 DWORD error{}, terminalError = ERROR_NO_MORE_ITEMS;
                 bool allowLock = true, allowUnlock = true;
                 BOOL Lock(HANDLE supplied) noexcept
                 {
                     ++locks;
                     cursor = 0;
                     error = ERROR_SUCCESS;
                     if (supplied != expected || !allowLock)
                     {
                         error = ERROR_ACCESS_DENIED;
                         return FALSE;
                     }
                     return TRUE;
                 }
                 BOOL Walk(HANDLE supplied, PROCESS_HEAP_ENTRY &entry) noexcept
                 {
                     ++reads;
                     if (supplied != expected)
                     {
                         error = ERROR_INVALID_HANDLE;
                         return FALSE;
                     }
                     if (cursor == entries.size())
                     {
                         error = terminalError;
                         return FALSE;
                     }
                     entry = entries[cursor++];
                     error = ERROR_SUCCESS;
                     return TRUE;
                 }
                 BOOL Unlock(HANDLE supplied) noexcept
                 {
                     ++unlocks;
                     if (supplied != expected || !allowUnlock)
                     {
                         error = ERROR_ACCESS_DENIED;
                         return FALSE;
                     }
                     return TRUE;
                 }
                 DWORD Error() const noexcept
                 {
                     return error;
                 }
             };
             struct Replay
             {
                 bool complete{};
                 std::string json;
                 std::size_t reads{}, locks{}, unlocks{};
             };
             std::array<PROCESS_HEAP_ENTRY, 2> scripted{};
             scripted[0].wFlags = PROCESS_HEAP_ENTRY_BUSY;
             scripted[0].cbData = 17u;
             scripted[0].cbOverhead = 3u;
             scripted[1].cbData = 29u;
             scripted[1].cbOverhead = 5u;
             const auto replay = [&](std::span<const PROCESS_HEAP_ENTRY> entries, std::size_t limit, bool lock = true,
                                     DWORD terminal = ERROR_NO_MORE_ITEMS, bool unlock = true)
             {
                 ScriptedCalls calls;
                 calls.expected = heap.get();
                 calls.entries = entries;
                 calls.allowLock = lock;
                 calls.terminalError = terminal;
                 calls.allowUnlock = unlock;
                 std::ostringstream report;
                 const bool complete = Test::WriteOwnedHeapDiagnostic(report, handles, limit, calls);
                 return Replay{complete, report.str(), calls.reads, calls.locks, calls.unlocks};
             };
             const auto empty = replay({}, 1u);
             const auto one = replay(std::span(scripted).first(1u), 1u);
             const auto truncated = replay(scripted, 1u);
             const auto exact = replay(scripted, 2u);
             const auto invalid = replay(scripted, 0u);
             const auto deniedLock = replay(scripted, 2u, false);
             const auto deniedWalk = replay({}, 1u, true, ERROR_ACCESS_DENIED);
             const auto deniedUnlock = replay(scripted, 2u, true, ERROR_NO_MORE_ITEMS, false);
             const auto retainCalls = [](const Replay &value)
             {
                 return std::format("{{\"report\":{},\"readCalls\":{},\"lockCalls\":{},\"unlockCalls\":{}}}", value.json, value.reads, value.locks,
                                    value.unlocks);
             };
             Success(WriteBytesAtomic(root / L"owned-heap-controlled-boundaries.json",
                                      std::format("{{\"empty\":{},\"oneEntry\":{},\"truncated\":{},\"exact\":{},\"invalidBudget\":{},"
                                                  "\"deniedLock\":{},\"deniedWalk\":{},\"deniedUnlock\":{}}}",
                                                  retainCalls(empty), retainCalls(one), retainCalls(truncated), retainCalls(exact),
                                                  retainCalls(invalid), retainCalls(deniedLock), retainCalls(deniedWalk), retainCalls(deniedUnlock))),
                     "retain all controlled boundary reports before checking the diagnostic");
             Require(empty.complete && empty.reads == 1u && empty.locks == 1u && empty.unlocks == 1u &&
                         JsonUnsigned(empty.json, "walkedEntries") == 0u,
                     "an independently empty provider completes within a positive budget using one end query");
             Require(one.complete && one.reads == 2u && one.locks == 1u && one.unlocks == 1u && JsonUnsigned(one.json, "walkedEntries") == 1u &&
                         one.json.find("\"budgetExceeded\":true") == one.json.npos,
                     "an independently one-entry provider completes at the exact boundary instead of inventing truncation");
             Require(!truncated.complete && truncated.reads == 2u && truncated.locks == 1u && truncated.unlocks == 1u &&
                         JsonUnsigned(truncated.json, "walkedEntries") == 1u &&
                         truncated.json.find("\"budgetExceeded\":true") != truncated.json.npos &&
                         JsonUnsigned(JsonObjectAfter(truncated.json, "\"totals\":"), "busyPayloadBytes") == 17u &&
                         JsonUnsigned(JsonObjectAfter(truncated.json, "\"totals\":"), "freePayloadBytes") == 0u,
                     "the independent two-entry provider always proves truncation and never accounts its lookahead entry");
             Require(exact.complete && exact.reads == 3u && exact.locks == 1u && exact.unlocks == 1u &&
                         JsonUnsigned(exact.json, "walkedEntries") == 2u &&
                         JsonUnsigned(JsonObjectAfter(exact.json, "\"totals\":"), "busyPayloadBytes") == 17u &&
                         JsonUnsigned(JsonObjectAfter(exact.json, "\"totals\":"), "freePayloadBytes") == 29u &&
                         JsonUnsigned(JsonObjectAfter(exact.json, "\"totals\":"), "entryOverheadBytes") == 8u,
                     "the independent exact-boundary provider accounts both entries and performs one bounded completion query");
             Require(!invalid.complete && invalid.reads == 0u && invalid.locks == 0u && invalid.unlocks == 0u &&
                         JsonUnsigned(invalid.json, "attemptedHeapCount") == 0u,
                     "an invalid zero budget refuses before acquiring or walking the provider");
             Require(!deniedLock.complete && deniedLock.reads == 0u && deniedLock.locks == 1u && deniedLock.unlocks == 0u &&
                         JsonUnsigned(deniedLock.json, "actualWalkedHeapCount") == 0u,
                     "lock refusal is incomplete coverage and never walks or unlocks an unowned provider");
             Require(!deniedWalk.complete && deniedWalk.reads == 1u && deniedWalk.unlocks == 1u &&
                         JsonUnsigned(JsonObjectContaining(deniedWalk.json, deniedWalk.json.find("\"index\":0")), "error") == ERROR_ACCESS_DENIED,
                     "a provider read failure cannot be relabelled as end-of-heap");
             Require(!deniedUnlock.complete && deniedUnlock.reads == 3u && deniedUnlock.unlocks == 1u &&
                         JsonUnsigned(deniedUnlock.json, "completeHeapCount") == 0u,
                     "unlock failure retains an incomplete result even after an otherwise complete walk");
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
                 const auto frameDiagnostics = JsonObjectAfter(probeReport, "\"frameDiagnostics\":");
                 const auto editTransactions = frameDiagnostics.find("\"editTransactions\":[");
                 const auto cleanTransactions = frameDiagnostics.find("\"cleanTransactions\":[");
                 Require(frameDiagnostics.find("\"protocol\":\"dxui-frame-metrics-v1\"") != frameDiagnostics.npos &&
                             frameDiagnostics.find("\"wallDurationOnly\":true") != frameDiagnostics.npos &&
                             editTransactions != frameDiagnostics.npos && cleanTransactions != frameDiagnostics.npos &&
                             frameDiagnostics.find("\"complete\":true,\"attemptCount\":", editTransactions) != frameDiagnostics.npos &&
                             frameDiagnostics.find("\"complete\":true,\"attemptCount\":", cleanTransactions) != frameDiagnostics.npos,
                         "opt-in DxUi frame timing retains complete edit and clean frame sequences as wall durations");
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
