#include "Application.Tests.Settings.h"
#include "Common/Settings.h"
#include "Common/Storage.h"
#include "RedTilde/Application.h"
#include "Support/Support.Tests.Foreground.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <wil/resource.h>

namespace RedTilde
{
class SettingsProbe final
{
public:
    [[nodiscard]] static const Settings& CurrentSettings(const Application& app) noexcept
    {
        return app._settings;
    }

    [[nodiscard]] static bool WatcherActive(const Application& app) noexcept
    {
        return app._settingsWatcher.Active();
    }

    [[nodiscard]] static bool PreferencesBusy(const Application& app) noexcept
    {
        return app._preferenceWriter.Busy();
    }
};

namespace
{
void Require(bool condition, std::string_view message)
{
    if (! condition)
        throw std::runtime_error(std::string(message));
}

void Success(HRESULT result, std::string_view message)
{
    if (FAILED(result))
        throw std::runtime_error(std::string(message));
}

bool PumpMessagesUntil(unsigned timeoutMs, const std::function<bool()>& condition)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;)
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (condition())
            return true;
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
            return condition();
        const auto remainingMs = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count());
        (void)MsgWaitForMultipleObjects(0, nullptr, FALSE, std::min<DWORD>(remainingMs, 25u), QS_ALLINPUT);
    }
}
} // namespace

void RegisterSettingsTests(const std::filesystem::path& root, const std::function<void(std::string_view, const std::function<void()>&)>& test)
{
    test("Settings.Json5SyntaxUnknownFieldsAndSchemaMerge",
         [&]
    {
        constexpr std::string_view json5 = "\xEF\xBB\xBF"
                                           "{\n"
                                           "  // JSON5 line comment\n"
                                           "  schema: 9.0,\n"
                                           "  /* block comment */\n"
                                           "  appearance: 'paper',\n"
                                           "  readingWidth: 840,\n"
                                           "  customExtension: { enabled: true, tags: ['a', 'b'], },\n"
                                           "  windowPlacement: { left: 100, top: 80, right: 1100, bottom: 780, futureDock: 'right', },\n"
                                           "  markdownSyntax: { wikiLinks: false, futureToggle: true, },\n"
                                           "}\n";
        Settings parsed;
        std::uint64_t schemaVersion{};
        Success(TryParseSettings(json5, parsed, &schemaVersion), "JSON5 with BOM, comments, trailing commas, and integral schema 9.0 must parse");
        Require(schemaVersion == 9u && parsed.appearance == Appearance::Paper && parsed.readingWidth == 840u &&
                    ! parsed.SyntaxEnabled(MarkdownSyntax::WikiLinks) && parsed.windowPlacement.has_value() && parsed.windowPlacement->normalBounds.left == 100,
                "parsed JSON5 fields match expected settings values");

        for (const auto* invalidSchema : {R"({"appearance": "dark"})",
                                          R"({"schema": 0, "appearance": "dark"})",
                                          R"({"schema": -2, "appearance": "dark"})",
                                          R"({"schema": 1.5, "appearance": "dark"})",
                                          R"({"schema": "1", "appearance": "dark"})",
                                          R"([1, 2, 3])",
                                          R"({unterminated: )"})
        {
            Settings scratch;
            scratch.appearance = Appearance::Dark;
            Require(TryParseSettings(invalidSchema, scratch) == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) && scratch.appearance == Appearance::Dark,
                    "invalid or non-integral schema or non-object JSON must be rejected without mutating caller state");
        }

        const auto directory = root / L"settings-merge-save";
        std::filesystem::create_directories(directory);
        Success(WriteBytesAtomic(SettingsPath(directory), json5), "seed JSON5 file with unknown keys and newer schema");

        Settings updated  = parsed;
        updated.textScale = 140u;
        updated.view      = EditorView::Split;
        Success(SaveSettings(directory, updated), "merge-save updates onto existing valid JSON5 settings");

        std::string savedBytes;
        Success(ReadBytes(SettingsPath(directory), savedBytes), "read merged JSON back from disk");
        Require(savedBytes.find("\"schema\": 9") != std::string::npos && savedBytes.find("\"customExtension\"") != std::string::npos &&
                    savedBytes.find("\"futureDock\"") != std::string::npos && savedBytes.find("\"futureToggle\"") != std::string::npos &&
                    savedBytes.find("\"textScale\": 140") != std::string::npos && savedBytes.find("\"view\": \"split\"") != std::string::npos,
                "merge-save preserves newer schema and unknown root/nested members while applying updated fields");
    });

    test("Settings.ColdLoadMigrationCorruptBackupAndSizeCap",
         [&]
    {
        Require(SettingsFileName(false) == L"RedTilde.settings.json" && SettingsFileName(true) == L"RedTilde-debug.settings.json",
                "Release and Debug builds resolve distinct settings filenames");

        const auto missingDir    = root / L"cold-missing";
        const auto missingResult = LoadSettingsCold(missingDir, false);
        Require(missingResult.status == SettingsLoadStatus::Missing && missingResult.templateWritten &&
                    std::filesystem::exists(SettingsPath(missingDir, false)) && missingResult.settings.appearance == Appearance::System,
                "cold load in an empty directory writes the default template cleanly");

        const auto migrateDir = root / L"cold-migrate";
        std::filesystem::create_directories(migrateDir);
        constexpr std::string_view legacyText = "RedTilde.Settings/1\n"
                                                "theme=3\n"
                                                "view=2\n"
                                                "scale=135\n"
                                                "readingWidth=880\n"
                                                "syntaxWikiLinks=0\n"
                                                "lastDocument=C:\\Notes\\Migrated Note.md\n";
        Success(WriteBytesAtomic(migrateDir / L"RedTilde.settings", legacyText), "seed legacy settings for one-time migration");
        const auto migrated = LoadSettingsCold(migrateDir, false);
        Require(migrated.status == SettingsLoadStatus::Migrated && migrated.templateWritten && migrated.settings.appearance == Appearance::Paper &&
                    migrated.settings.view == EditorView::Split && migrated.settings.textScale == 135u && migrated.settings.readingWidth == 880u &&
                    ! migrated.settings.SyntaxEnabled(MarkdownSyntax::WikiLinks) && migrated.settings.lastDocument == L"C:\\Notes\\Migrated Note.md" &&
                    std::filesystem::exists(SettingsPath(migrateDir, false)) && std::filesystem::exists(migrateDir / L"RedTilde.settings"),
                "cold load performs one-time lossless migration from legacy text format to JSON");

        const auto corruptDir = root / L"cold-corrupt";
        std::filesystem::create_directories(corruptDir);
        constexpr std::string_view corruptBytes = "{ \"schema\": 1, broken_json ";
        Success(WriteBytesAtomic(SettingsPath(corruptDir, true), corruptBytes), "seed malformed JSON5 file");
        const auto corruptResult = LoadSettingsCold(corruptDir, true);
        Require(corruptResult.status == SettingsLoadStatus::Invalid && corruptResult.templateWritten && ! corruptResult.backupPath.empty() &&
                    std::filesystem::exists(corruptResult.backupPath) && std::filesystem::exists(SettingsPath(corruptDir, true)),
                "cold load backs up corrupted settings and writes a fresh defaults template");
        Require(corruptResult.backupPath.filename().wstring().find(L"RedTilde-debug.settings.json.invalid-") == 0u,
                "corrupted backup uses documented .invalid-<timestamp> suffix");
        std::string backedUpBytes;
        Success(ReadBytes(corruptResult.backupPath, backedUpBytes), "read corrupted backup bytes");
        Require(backedUpBytes == corruptBytes, "corrupted backup preserves exact malformed bytes");

        const auto incompatibleDirectory = root / L"cold-incompatible";
        std::filesystem::create_directories(incompatibleDirectory);
        const auto incompatiblePath                  = SettingsPath(incompatibleDirectory, false);
        constexpr std::string_view incompatibleBytes = "{schema: 0, appearance: 'dark'}\n";
        Success(WriteBytesAtomic(incompatiblePath, incompatibleBytes), "seed readable settings with an unsupported schema");
        const auto incompatible = LoadSettingsCold(incompatibleDirectory, false);
        std::string preservedBytes;
        Success(ReadBytes(incompatiblePath, preservedBytes), "read the preserved incompatible settings");
        Require(incompatible.status == SettingsLoadStatus::Incompatible && incompatible.settings.appearance == Appearance::System &&
                    ! incompatible.templateWritten && incompatible.backupPath.empty() && preservedBytes == incompatibleBytes,
                "cold fallback preserves readable incompatible settings at their original path");

        const auto racingDirectory = root / L"cold-competing-edit";
        const auto racingPath      = SettingsPath(racingDirectory, false);
        Success(WriteBytesAtomic(racingPath, corruptBytes), "seed malformed settings for the guarded recovery race");
        struct Editor
        {
            std::filesystem::path path;
            std::string_view document;
            bool replaced{};
        };
        Editor editor{racingPath, R"json({"schema":1,"appearance":"paper"})json"};
        {
            const auto stopHook = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
            SetAtomicWriteHookForTest(
                [](AtomicWriteCheckpoint step, const std::filesystem::path& target, void* context) noexcept
            {
                auto& edit = *static_cast<Editor*>(context);
                if (step != AtomicWriteCheckpoint::Staged || edit.path != target)
                    return;
                const wil::unique_hfile file{
                    CreateFileW(target.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
                DWORD written{};
                const auto size = static_cast<DWORD>(edit.document.size());
                edit.replaced   = file && WriteFile(file.get(), edit.document.data(), size, &written, nullptr) && written == size;
            },
                &editor);
            const auto raced = LoadSettingsCold(racingDirectory, false);
            Require(editor.replaced && raced.status == SettingsLoadStatus::Invalid && ! raced.templateWritten && ! raced.backupPath.empty(),
                    "cold recovery detects the edit after staging its defaults");
            Success(ReadBytes(raced.backupPath, backedUpBytes), "read the exact diagnosed version after the race");
            Require(backedUpBytes == corruptBytes, "the recovery backup keeps the diagnosed bytes");
        }
        Success(ReadBytes(racingPath, preservedBytes), "read the competing settings after recovery");
        Require(preservedBytes == editor.document, "guarded cold recovery leaves the competing edit in place");

        const auto oversizedDir = root / L"cold-oversized";
        std::filesystem::create_directories(oversizedDir);
        const auto oversizedPath = SettingsPath(oversizedDir, false);
        const auto removeLarge   = wil::scope_exit([&]
        {
            std::error_code cleanupError;
            std::filesystem::remove(oversizedPath, cleanupError);
        });
        {
            std::ofstream stream(oversizedPath, std::ios::binary | std::ios::trunc);
            Require(static_cast<bool>(stream), "create oversized settings fixture");
            const std::string chunk(1024u * 1024u, ' ');
            for (int index = 0; index < 17; ++index)
                stream.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
            Require(static_cast<bool>(stream), "write >16 MiB settings fixture");
        }
        const auto oversizedCold = LoadSettingsCold(oversizedDir, false);
        Require(oversizedCold.status == SettingsLoadStatus::Unreadable && ! oversizedCold.templateWritten && oversizedCold.backupPath.empty() &&
                    std::filesystem::file_size(oversizedPath) > 16u * 1024u * 1024u,
                "unreadable >16 MiB settings keep defaults in memory without overwriting or renaming disk file");
        Settings liveScratch;
        Require(LoadSettingsLive(oversizedDir, liveScratch, nullptr, false) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE),
                "live load enforces the 16 MiB budget");
        Require(SaveSettings(oversizedDir, liveScratch, false) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE),
                "save refuses to overwrite an oversized settings file");
    });

    test("Settings.LiveReloadAndMalformedRecovery",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto directory = root / L"live-reload-app";
        Settings initial;
        initial.appearance   = Appearance::System;
        initial.readingWidth = 760u;
        initial.textScale    = 100u;
        Success(SaveSettings(directory, initial), "seed valid settings before starting hidden Application");

        LaunchOptions options;
        options.headless             = true;
        options.watchSettingsForTest = true;
        options.dataDirectory        = directory;

        Application app(options);
        Success(app.Initialize(), "initialize headless Application with SettingsWatcher active");
        Require(SettingsProbe::WatcherActive(app), "SettingsWatcher starts when watchSettingsForTest is enabled");

        // Own save via SettingsWriter must persist to disk and suppress its own watcher stamp past coalesce window.
        app.Execute(Command::ThemeDark);
        Require(PumpMessagesUntil(3000u, [&] { return ! SettingsProbe::PreferencesBusy(app); }), "own preference save completes asynchronously");
        std::string selfSavedBytes;
        Success(ReadBytes(SettingsPath(directory), selfSavedBytes), "read settings after self-save");
        Require(selfSavedBytes.find("\"appearance\": \"dark\"") != std::string::npos, "self-save persists Dark appearance on disk");
        (void)PumpMessagesUntil(450u, [] { return false; });
        Require(SettingsProbe::CurrentSettings(app).appearance == Appearance::Dark, "own preference state remains Dark after self-save watcher settles");

        // External valid JSON5 edit with comments and unknown keys applies live fields without rewriting disk.
        constexpr std::string_view externalValid = "{\n"
                                                   "  // External user edit\n"
                                                   "  \"schema\": 1,\n"
                                                   "  \"appearance\": \"paper\",\n"
                                                   "  \"readingWidth\": 920,\n"
                                                   "  \"textScale\": 130,\n"
                                                   "  \"showOutline\": false,\n"
                                                   "  \"dimInactiveBlocks\": false,\n"
                                                   "  \"allowRemoteImages\": true,\n"
                                                   "  \"allowLocalNetworkImages\": true,\n"
                                                   "  \"imageFirstPaste\": true,\n"
                                                   "  \"strikeCompletedTasks\": true,\n"
                                                   "  \"markdownSyntax\": { \"wikiLinks\": false, },\n"
                                                   "  \"editorNote\": \"do-not-clobber\",\n"
                                                   "}\n";
        Success(WriteBytesAtomic(SettingsPath(directory), externalValid), "write external JSON5 edit");
        Require(PumpMessagesUntil(4000u,
                                  [&]
        {
            const auto& current = SettingsProbe::CurrentSettings(app);
            return current.appearance == Appearance::Paper && current.readingWidth == 920u && current.textScale == 130u && ! current.showOutline &&
                   ! current.dimInactiveBlocks && current.allowRemoteImages && current.allowLocalNetworkImages && current.imageFirstPaste &&
                   current.strikeCompletedTasks && ! current.SyntaxEnabled(MarkdownSyntax::WikiLinks);
        }),
                "live reload applies every live setting from disk");

        std::string diskAfterReload;
        Success(ReadBytes(SettingsPath(directory), diskAfterReload), "read settings file after live reload");
        Require(diskAfterReload == externalValid, "live reload never writes to or reformats the watched settings file");

        // Malformed live edit preserves last-good settings and leaves malformed disk bytes untouched.
        constexpr std::string_view malformedLive = "{ \"schema\": 1, \"appearance\": \"light\", broken ";
        Success(WriteBytesAtomic(SettingsPath(directory), malformedLive), "write malformed JSON5 live edit");
        (void)PumpMessagesUntil(450u,
                                [&]
        {
            const auto& current = SettingsProbe::CurrentSettings(app);
            return current.appearance != Appearance::Paper || current.readingWidth != 920u || current.textScale != 130u;
        });
        {
            const auto& current = SettingsProbe::CurrentSettings(app);
            Require(current.appearance == Appearance::Paper && current.readingWidth == 920u && current.textScale == 130u,
                    "malformed live edit preserves last-good live settings");
        }
        std::string malformedOnDisk;
        Success(ReadBytes(SettingsPath(directory), malformedOnDisk), "verify malformed live edit bytes remain untouched");
        Require(malformedOnDisk == malformedLive, "live reload does not clobber or back up malformed user edits on disk");
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            Require(entry.path().filename().wstring().find(L".invalid-") == std::wstring::npos, "malformed live edit creates no .invalid-* backup");
        }

        // Fixing the malformed edit resumes live reload cleanly.
        constexpr std::string_view repairedLive = R"({"schema": 1, "appearance": "light", "readingWidth": 640})";
        Success(WriteBytesAtomic(SettingsPath(directory), repairedLive), "repair settings file on disk");
        Require(PumpMessagesUntil(4000u,
                                  [&]
        {
            const auto& current = SettingsProbe::CurrentSettings(app);
            return current.appearance == Appearance::Light && current.readingWidth == 640u;
        }),
                "valid live edit after malformed edit applies cleanly");
        Require(foreground.NeverAcquiredForeground(), "live settings reload checks preserve desktop foreground");
    });

    test("Settings.WatcherRapidBurstCoalescing",
         [&]
    {
        Testing::ForegroundOwnershipObserver foreground;
        const auto directory = root / L"watcher-burst";
        Settings initial;
        Success(SaveSettings(directory, initial), "seed initial burst settings");

        LaunchOptions options;
        options.headless             = true;
        options.watchSettingsForTest = true;
        options.dataDirectory        = directory;

        Application app(options);
        Success(app.Initialize(), "initialize Application for rapid burst watcher test");

        for (unsigned step = 1; step <= 10; ++step)
        {
            Settings next;
            next.readingWidth = 520u + step * 40u; // Final step == 920
            next.appearance   = step == 10u ? Appearance::Dark : Appearance::Light;
            Success(WriteBytesAtomic(SettingsPath(directory), SerializeSettings(next)), "write rapid burst update");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        unsigned observedTransitions{};
        unsigned lastWidth        = initial.readingWidth;
        Appearance lastAppearance = initial.appearance;
        Require(PumpMessagesUntil(4000u,
                                  [&]
        {
            const auto& current = SettingsProbe::CurrentSettings(app);
            if (current.readingWidth != lastWidth || current.appearance != lastAppearance)
            {
                ++observedTransitions;
                lastWidth      = current.readingWidth;
                lastAppearance = current.appearance;
            }
            return current.appearance == Appearance::Dark && current.readingWidth == 920u;
        }),
                "coalesced SettingsWatcher converges cleanly to the final burst state");
        Require(observedTransitions < 10u && foreground.NeverAcquiredForeground(),
                "rapid burst coalesces intermediate writes cleanly without acquiring foreground");
    });

    test("Settings.RejectsJson5NanAndInfinityWithoutTruncatingFile",
         [&]
    {
        for (const auto* invalidLiteral : {R"({"schema": 1, "extra": NaN})", R"({"schema": 1, "extra": Infinity})", R"({"schema": 1, "extra": -Infinity})"})
        {
            Settings parsed;
            parsed.appearance = Appearance::Light;
            Require(TryParseSettings(invalidLiteral, parsed) == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) && parsed.appearance == Appearance::Light,
                    "JSON5 NaN and Infinity literals are rejected cleanly without mutating caller settings");
        }

        const auto directory = root / L"nan-no-truncate";
        std::filesystem::create_directories(directory);
        constexpr std::string_view seededWithNan = "{\n  \"schema\": 1,\n  \"appearance\": \"dark\",\n  \"extra\": NaN\n}\n";
        Success(WriteBytesAtomic(SettingsPath(directory), seededWithNan), "seed settings containing unsupported NaN literal");

        Settings toSave;
        toSave.appearance    = Appearance::Paper;
        const HRESULT saveHr = SaveSettings(directory, toSave);
        std::string diskAfter;
        Success(ReadBytes(SettingsPath(directory), diskAfter), "disk file remains readable after SaveSettings");
        Require(! diskAfter.empty() && (FAILED(saveHr) || diskAfter.find("\"appearance\": \"paper\"") != std::string::npos),
                "SaveSettings never truncates existing settings file to 0 bytes on non-finite preserved input");
    });

    test("Settings.SavesAndLoadsBetweenDocumentCapAndSixteenMiB",
         [&]
    {
        const auto directory = root / L"settings-512k";
        std::filesystem::create_directories(directory);
        const auto cleanup  = wil::scope_exit([&]
        {
            std::error_code ec;
            std::filesystem::remove_all(directory, ec);
        });
        std::string payload = "{\n  \"schema\": 1,\n  \"appearance\": \"dark\",\n  \"readingWidth\": 840,\n  \"blob\": \"";
        payload.append(512u * 1024u, 'x');
        payload += "\"\n}\n";
        const auto settingsPath = SettingsPath(directory, false);
        Success(WriteBytesAtomic(settingsPath, payload, nullptr, nullptr, true, nullptr, false, nullptr, 16u * 1024u * 1024u),
                "write >266 KiB valid settings fixture");

        const auto cold = LoadSettingsCold(directory, false);
        Require(cold.status == SettingsLoadStatus::Loaded && cold.settings.appearance == Appearance::Dark && cold.settings.readingWidth == 840u,
                "cold load reads valid settings above the 266 KiB document cap");

        Settings updated  = cold.settings;
        updated.textScale = 125u;
        Success(SaveSettings(directory, updated, false), "SaveSettings updates >266 KiB settings cleanly within the 16 MiB budget");

        Settings live;
        Success(LoadSettingsLive(directory, live, nullptr, false), "live load reads merged >266 KiB settings cleanly");
        Require(live.textScale == 125u && live.readingWidth == 840u && std::filesystem::file_size(settingsPath) > 500u * 1024u,
                "updated >266 KiB settings preserve large unknown fields cleanly");
    });

    test("Settings.LegacyCrLfMigrationAndNoOpSavePreservesFile",
         [&]
    {
        const auto migrateDir = root / L"cold-migrate-crlf";
        std::filesystem::create_directories(migrateDir);
        constexpr std::string_view crlfLegacy = "RedTilde.Settings/1\r\n"
                                                "theme=2\r\n"
                                                "view=1\r\n"
                                                "scale=115\r\n"
                                                "readingWidth=720\r\n"
                                                "lastDocument=C:\\Notes\\CRLF Note.md\r\n";
        Success(WriteBytesAtomic(migrateDir / L"RedTilde.settings", crlfLegacy), "seed CRLF legacy settings file");
        const auto migrated = LoadSettingsCold(migrateDir, false);
        Require(migrated.status == SettingsLoadStatus::Migrated && migrated.settings.appearance == Appearance::Dark &&
                    migrated.settings.view == EditorView::Source && migrated.settings.textScale == 115u && migrated.settings.readingWidth == 720u &&
                    migrated.settings.lastDocument == L"C:\\Notes\\CRLF Note.md",
                "CRLF legacy settings migrate without trailing \\r corruption");

        const auto noopDir = root / L"noop-save-comments";
        std::filesystem::create_directories(noopDir);
        constexpr std::string_view commented = "{\n"
                                               "  // User comment kept on no-op save\n"
                                               "  \"schema\": 1,\n"
                                               "  \"appearance\": \"dark\",\n"
                                               "  \"readingWidth\": 760\n"
                                               "}\n";
        Success(WriteBytesAtomic(SettingsPath(noopDir, false), commented), "seed commented JSON5 settings");
        const auto loaded = LoadSettingsCold(noopDir, false);
        Require(loaded.status == SettingsLoadStatus::Loaded, "load commented JSON5 settings");
        Success(SaveSettings(noopDir, loaded.settings, false), "no-op SaveSettings succeeds cleanly");
        std::string diskAfter;
        Success(ReadBytes(SettingsPath(noopDir, false), diskAfter), "read commented settings after no-op SaveSettings");
        Require(diskAfter == commented, "no-op SaveSettings skips rewrite and preserves existing user JSON5 comments");
    });
}
} // namespace RedTilde
