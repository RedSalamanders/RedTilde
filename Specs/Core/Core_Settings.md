# Settings, theming, live reload, and close worker settlement

Status: current normative contract

Local crash diagnostics, DxUi protocol-2 reporting, dump retention, crash probes, and process shutdown drain live in [the crash-handling contract](Core_CrashHandling.md). The machine-readable schema lives in [SettingsStore.schema.json](../../SettingsStore.schema.json) and is staged beside `RedTilde.exe` in build outputs and packages.

## Files, JSON5 format, and schema lifecycle

Settings live under `%LOCALAPPDATA%\RedTilde` in release builds as `RedTilde.settings.json` and in `_DEBUG` builds as `RedTilde-debug.settings.json` (`SettingsFileName` / `SettingsPath` in `Common/Settings.h`). Harness and test callers select an isolated `--data-dir`. Informational CLI commands (`--version`, `-v`, `--help`, `-h`, `/?`, `-?`) print output and exit before resolving `%LOCALAPPDATA%\RedTilde`, creating directories or templates, or installing crash handlers.

Files use strict finite UTF-8 JSON5 (`(YYJSON_READ_JSON5 & ~YYJSON_READ_ALLOW_INF_AND_NAN) | YYJSON_READ_ALLOW_BOM`), supporting `//` and `/* */` comments, trailing commas, unquoted object keys, single-quoted strings, and an optional UTF-8 BOM (while rejecting `NaN` and `Infinity` literals that standard JSON serialization cannot represent), bounded to a hard `16 MiB` (`16 * 1024 * 1024` bytes) maximum read and atomic write budget. The root value must be a JSON object with an integral number property `schema >= 1` (`CurrentSchemaVersion = 1`; integer numbers and zero-fraction floats such as `10.0` are accepted, while missing, zero, negative, fractional, or non-numeric `schema` values are rejected with `ERROR_BAD_FORMAT`).

Unknown root properties (such as `$schema` or future extensions), unknown nested properties under `windowPlacement` and `markdownSyntax`, and newer integral `schema` versions (`schema > 1`) remain readable and are preserved across `SaveSettings` via document-tree merging onto the existing valid JSON5 file on disk.

## Cold load, migration, corruption recovery, and field fallback

On ordinary interactive startup (and probes explicitly opting into cold recovery), `LoadSettingsCold` runs once:

- **Valid JSON5 present**: parses valid properties, falls back per field for invalid individual values, preserves `FileStamp`, and returns `SettingsLoadStatus::Loaded`.
- **JSON5 absent + legacy `RedTilde.settings` (`RedTilde.Settings/1` with LF or CRLF line endings) present**: performs a lossless one-time migration (`ParseLegacySettings`) and atomically writes `RedTilde.settings.json` (or `RedTilde-debug.settings.json`) while leaving the legacy file intact on disk (`SettingsLoadStatus::Migrated`). Subsequent launches load the JSON5 file directly, and `SaveSettings` skips no-op writes whose normalized serialized document matches the existing file bytes on disk so user comments are preserved.
- **Both absent**: writes a clean defaults template (`schema: 1`) atomically and returns `SettingsLoadStatus::Missing`.
- **Malformed JSON5 or invalid `schema`**: moves the corrupt file atomically in the same directory to `<settings-path>.invalid-<yyyyMMdd-HHmmss-fff>` preserving its exact bytes, writes a clean defaults template, and returns `SettingsLoadStatus::Invalid`.
- **Unreadable or oversized (`> 16 MiB`)**: leaves the disk file untouched, creates no `.invalid-*` backup, returns defaults in memory, and returns `SettingsLoadStatus::Unreadable`.

Both `LoadSettingsLive` and `SaveSettings` validate any existing file first without creating `.invalid-*` backups or overwriting malformed or oversized files (`ERROR_BAD_FORMAT` / `ERROR_FILE_TOO_LARGE`).

### Persisted fields and defaults

Themes are `system`, `light`, `dark`, and `paper` (`Appearance`); Windows High Contrast always overrides custom colors before theme palette selection. Invalid individual fields fall back to their own default without discarding valid neighboring settings:

- `appearance`: `"system"` | `"light"` | `"dark"` | `"paper"` (default `"system"`).
- `view`: `"compose"` | `"source"` | `"split"` (default `"compose"`).
- `focusMode`: session-local only; ignored on load and omitted on save so restart never hides commands (`false`).
- `showOutline`: boolean (default `true`); `showFiles`: boolean (default `false`).
- `filesRoot` / `lastDocument`: strict UTF-16-valid absolute Windows paths, `<= 32,767` UTF-16 code units, containing no control characters (`< 0x20`). Spaces, Unicode, backslashes, and equals signs round-trip verbatim; invalid paths fall back independently to empty and are omitted from serialization.
- `filesWidth`: integer `200..400` DIPs (default `280`); `readingWidth`: integer `480..1040` DIPs (default `760`); `textScale`: integer `75..200` percent (default `100`). Negative, fractional, non-numeric, out-of-range, and overflowing values fall back independently.
- `dimInactiveBlocks`: boolean (default `true`).
- `allowRemoteImages`: boolean (default `false`) — persistent “Always allow remote images” consent.
- `allowLocalNetworkImages`: boolean (default `false`) — independent private/loopback/UNC opt-in that never grants remote image consent on its own. Per-document image grants remain workspace-only and are never persisted.
- `imageFirstPaste`: boolean (default `false`) — clipboard paste prefers bitmap images over Unicode text only when enabled.
- `strikeCompletedTasks`: boolean (default `false`) — live presentation setting shared by Compose and Split.
- `windowPlacement`: `{ left, top, right, bottom, dpi, maximized }` tuple in physical screen coordinates (`+/-1,048,576`, extent `<= 32,768` pixels, non-empty `left < right && top < bottom`). `dpi` (`48..768`) defaults to `96` when missing; malformed `dpi` or bounds reject the tuple. Invalid `maximized` defaults independently to `false`. Never serialize partial, iconic, or maximized bounds as normal placement. Restore selects the nearest monitor, scales size for target DPI, and clamps to work area (`STARTUPINFO` show state and headless capture geometry still take precedence).
- `markdownSyntax`: object of seven application-owned booleans (`highlight`, `superscript`, `subscript`, `inserted`, `wikiLinks`, `definitionLists`, `abbreviations`), each defaulting independently to `true`.

## Live settings watcher and non-writing reload

In interactive non-harness runs (and test fixtures opting into `LaunchOptions::watchSettingsForTest`), `SettingsWatcher` starts after initialization completes:

- Uses an event-driven OVERLAPPED `ReadDirectoryChangesW` loop paired with a manual-reset `_stopEvent` on a single `ProcessWorkLease`-held worker thread — zero busy polling timers. Permanent directory-open refusals stop the watcher immediately and bounded consecutive transient failures stop retrying instead of polling every second.
- Filters notifications case-insensitively to the active settings filename, coalesces rapid edit bursts over a `250 ms` window, and posts at most one unacknowledged `Application::SettingsChanged` message to the UI window (`PostChanged` / `Acknowledge`).
- `ReloadSettingsFromDisk()` defers via `_settingsReloadDeferred` if `_preferenceWriter.Busy()` (re-checking on `PreferencesReady`), calls read-only `LoadSettingsLive`, verifies the `FileStamp` differs from `_settingsFileStamp` and `_preferenceWriter.LastWrittenStamp()` so RedTilde’s own asynchronous `SettingsWriter` saves never cause reload loops, applies live presentation fields (`appearance`, `readingWidth`, `textScale`, `showOutline`, `dimInactiveBlocks`, `allowRemoteImages`, `allowLocalNetworkImages`, `imageFirstPaste`, `strikeCompletedTasks`, `markdownSyntax`) immediately, and **never** calls `SavePreferences()` or writes to disk.
- If a live disk edit is temporarily unreadable (`ERROR_SHARING_VIOLATION` / `ERROR_FILE_NOT_FOUND` / `ERROR_RETRY` with bounded timer retry) or malformed JSON5 (`ERROR_BAD_FORMAT` / `ERROR_FILE_TOO_LARGE`), `ReloadSettingsFromDisk()` leaves last-good in-memory settings and the user’s disk file completely untouched, logs a diagnostic warning (`OutputDebugStringW` + `MonitorDiagnostics::Write`), and resumes reloading on the next valid edit.
- `SettingsWatcher::Close()` signals `_stopEvent`, cancels pending overlapped I/O via `CancelIoEx`, and joins cleanly in `BeginClose()`, `WM_ENDSESSION(TRUE)`, `WM_NCDESTROY`, and `~Application()`.

## Startup precedence, asynchronous writes, and Close worker settlement

The independent `filesRoot` and `lastDocument` preferences never couple implicitly to each other or override recovery snapshots or explicit CLI paths:

- An ordinary launch without an explicit file starts blank, runs the initial recovery scan first, and opens `lastDocument` asynchronously only when no recovery copies apply. Early edits, New, Open, Open Folder, Files-row activation, or Close supersede startup restoration cleanly. Unavailable `lastDocument` paths show status on a clean Untitled document without modal errors.
- Persistence uses `SettingsWriter` (`WorkerPhase`: `Idle`, `Active`, `Closed`) with one coalesced latest pending `Settings` value and atomic same-directory replacement carrying the read `FileStamp`. Settings errors never block document editing. Focus mode (`Command::Focus`) stays session-local and never writes settings.

After accepting Close (`Application::LifecyclePhase::DrainingForClose`), stop producers (including `_settingsWatcher.Close()`), detach the editor host, and hide rather than destroying the notification HWND if `!AreShutdownWorkersSettled()` (`!_preferenceWriter.Busy() && _recovery.Finished() && !_bitmapWriter.Busy()`; leased/retiring bitmap assets remain busy until committed or declined). `DispatchUiMessage()` and completion messages recheck `AreShutdownWorkersSettled()` without spurious `PreferencesReady` churn and never join storage on the UI thread.

`WM_QUERYENDSESSION` permits shutdown immediately and checkpoints dirty documents without modal prompts. `WM_ENDSESSION(TRUE)` stops editing commands, closes `_settingsWatcher`, retires pending bitmap paste without waiting, re-requests workspace checkpoints, and bounds recovery settlement to `2,000 ms`. `WM_ENDSESSION(FALSE)` leaves the window and editing active.

Expected source/codec/configuration failures use checked APIs leaving state untouched; capacity remains `65,536` UTF-16 units and unexpected allocation failure remains fatal (`Core_CrashHandling.md`).
