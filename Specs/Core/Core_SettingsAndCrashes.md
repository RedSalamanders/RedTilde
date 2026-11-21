# Settings, theming, and close worker settlement

Local crash diagnostics, DxUi protocol-2 reporting, dump retention, crash probes, and process shutdown drain live in [the crash-handling contract](Core_CrashHandling.md).

Settings live under `%LOCALAPPDATA%\RedTilde` and use a versioned UTF-8 document. A caller may select an isolated
data directory for tests. Themes are System, Light, Dark and Paper. Respect Windows high contrast before custom
colors. Persist appearance, view, outline visibility, Files-pane visibility, selected root and width, reading width, text scale,
inactive-block dimming, completed-task strike-through, remote-image opt-in, local-network image opt-in, image-first paste and optional Markdown syntax. The
`strikeCompletedTasks` field accepts only 0 or 1; missing or invalid values keep the disabled default. It is live
presentation policy shared by Compose and Split. Defaults are System appearance, Compose view, outline visible, Files pane closed,
Files width 280 DIP, reading width 760 DIP, text scale 100%, inactive-block dimming enabled, remote images disabled and every optional Markdown syntax enabled. Files widths are
limited to 200–400 DIP, reading width to 480–1040 DIP, and text scale to 75–200%; invalid fields fall back
independently without discarding valid neighboring settings.

`remoteImages=0|1` is the persistent “Always allow remote images” choice. `localNetworkImages=0|1` is an
independent opt-in, disabled when missing or invalid; it grants no image consent itself. Per-document grants
belong only to the live retained workspace and are absent from settings/recovery. Restart requires renewed
document consent unless the global choice applies. Neither permission changes nor image reload mark text dirty.

Persist normal window bounds in physical screen coordinates (`windowLeft`, `windowTop`, `windowRight`,
`windowBottom`), the source `windowDpi` and a separate `windowMaximized=0|1`. The four coordinates form one
complete tuple; malformed, reversed, zero-size, overflowed or oversized records are ignored. Coordinates are
bounded to +/-1,048,576 and each extent to 32,768 pixels. Missing DPI defaults to 96; a present DPI outside
48–768 or a malformed DPI rejects the placement. An invalid maximized flag defaults independently to normal.
Never serialize a partial/invalid placement, minimized/iconic coordinates or maximized screen bounds as normal bounds.
On restore choose an attached monitor nearest the saved screen rectangle, scale the saved size for its actual
window DPI, and clamp both position and size to its work area. On first run center and fit the default size.
The usual 800x560 DIP minimum must also fit a smaller work area. Use screen coordinates throughout; native
WINDOWPLACEMENT workspace coordinates must not be passed as screen coordinates. See the
[Windows coordinate contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-windowplacement).
Moving/resizing updates the in-memory record; submit through the ordinary coalescing settings worker on
end-of-move/resize, a maximized/normal transition, DPI change, accepted Close or session query. A successful
preference admission carries the latest placement; do no synchronous storage work in a window message.
Ordinary startup restores the saved maximized state. An explicit launch show state from STARTUPINFO/WinMain
takes precedence. Never force native editor focus after a hidden, minimized or nonactivating launch. Headless
captures retain their requested deterministic geometry and do not restore or persist native desktop placement.
A rejected or unknown settings header returns `ERROR_BAD_FORMAT` and leaves both caller settings and the
original file untouched. Synchronous and asynchronous preference writers validate the existing header
before publication; changing views must not overwrite rejected settings with defaults. Preserve the file
until it is explicitly repaired or removed. A valid existing settings file uses its read stamp for the
ordinary guarded atomic replacement; missing files may be created.
The version-1 `filesRoot` field holds a strict UTF-8 absolute Windows path, bounded to 32,767 UTF-16 units and
excluding control characters. Spaces, Unicode, backslashes and equals signs round trip without escaping or
normalizing document content. Missing/invalid roots fall back independently to empty. Serialization omits invalid
roots so a folder cannot introduce another line-based preference. Restoring a root does no synchronous directory
enumeration: the ordinary bounded Files worker reports availability. Root persistence is independent of document
and recovery state; hiding Files retains it, and only explicit folder selection replaces an established root.

The independent version-1 `lastDocument` field uses the same strict, bounded absolute UTF-8 path validation as
`filesRoot`. Remember the active normal file after a successful Open/Save As and workspace switch; New or an
untitled/recovered active document on accepted Close clears it. A recovery restore does not publish its original
path as a newly saved startup document. The persistent folder root never follows this preference implicitly.
An ordinary launch without an explicit file starts blank and waits for the initial recovery scan. Pending
recovery copies take precedence; a failed recovery scan leaves editing available without starting an uncertain
automatic read. With no candidates, reopen the remembered path through the ordinary file worker, using the
exact admitted blank source identity. Do no synchronous existence/file read on the UI thread. An unavailable
path leaves a clean blank Untitled document with status, without a modal error or sample substitution.
Early/late source edits supersede startup restoration without an unsaved-change prompt. Explicit New, Open,
Open Folder, a Files-row activation or Close abandons that startup read by closing its old notification state;
the worker owns and retires its values without a UI wait. An explicit file argument takes precedence over the
remembered path. Hidden deterministic captures continue using their explicit sample/document fixtures.
Source, selection and history are never recovered from this preference; crash recovery retains that ownership.
Focus mode is session-local and is not persisted: `Command::Focus` toggles `settings.focusMode`, closes overlays, and
relayouts without calling `SavePreferences()` or rewriting `RedTilde.settings` on disk. Informational CLI commands
(`--version`, `-v`, `--help`, `-h`, `/?`, `-?`) print usage or `RedTilde <REDTILDE_PRODUCT_VERSION>` and exit before
resolving profile directories, creating `%LOCALAPPDATA%\RedTilde` or `Crashes/`, or installing crash handlers. Apply persisted
appearance, reading width, text scale and dimming immediately; no restart is required. Reduced motion follows
Windows preferences. The independent version-1 `imageFirstPaste=0|1` field defaults to disabled on missing or
invalid input. Its live Settings toggle updates the editor host's clipboard preference and queues ordinary
preference persistence without changing source, selection or history. Unicode-first remains the default.
The version-1 `remoteImages=0|1` field defaults independently to disabled on missing or invalid
input. Seven optional Markdown syntax fields — `syntaxHighlight`, `syntaxSuperscript`, `syntaxSubscript`,
`syntaxInserted`, `syntaxWikiLinks`, `syntaxDefinitionLists` and `syntaxAbbreviations`, each `0|1` — default
independently to enabled on missing or invalid input and apply immediately. They are application-owned names;
never persist library extension flag values. The live privacy command retires remote requests/pixels immediately; local image identity and completion
remain valid. Invalid settings fall back per field and retain the rejected file for inspection. Persist
through the same atomic writer as documents. Settings errors must not prevent editing a document. Persistence
uses `SettingsWriter::State::WorkerPhase` (`Idle`, `Active`, `Closed`) and one coalesced latest pending state. Admitted writes finish after
the window closes without joining storage on the UI thread; notifications lose their window binding.

After accepting Close (`Application::LifecyclePhase::DrainingForClose`), stop producers and detach the editor host.
If `AreShutdownWorkersSettled()` is false (an admitted preferences write, recovery retirement, or bitmap-asset
operation is still pending), hide the application window and retain its notification HWND until all three services
are idle (`!preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy()`). A leased bitmap result
(`BitmapPasteWriter::Phase::Leased` or `Retiring`) is still busy until the owner commits or declines it.
`DispatchUiMessage()` rechecks `AreShutdownWorkersSettled()` after dispatched messages as well as completion
notifications without sending spurious `PreferencesReady` churn; services may finish in any order. Window destruction
requires all three finished states; service closure clears its notification binding. Do not join storage on the UI
thread or retire the HWND after only one service completes.
`WM_QUERYENDSESSION` immediately permits session termination and requests checkpoints for every dirty workspace
document without a modal save question or shutdown-blocking reason. At final `WM_ENDSESSION(TRUE)`, editing
commands stop, all workspace checkpoints are requested again and recovery may wait for at most 2,000 ms. A
pending bitmap insertion is retired and its worker is stopped without an additional UI-thread wait; a
cancelled session end retains the admitted operation. Bitmap completion cannot change checkpointed source. A
failure/timeout is recorded; it cannot turn the bounded allowance into an indefinite join. `WM_ENDSESSION(FALSE)`
keeps the window, histories, pending snapshots and editing available. Ordinary Close retains its asynchronous
worker-settlement contract. An ordinary visible launch registers Windows application restart for update/logoff
coordination with crash/hang restart disabled; crash recovery remains the next ordinary launch's local preview.
Synchronous native window and accessibility notifications during document-change publication may re-enter
accepted Close; resumed updates must stop before using controls detached by that Close. This includes the
Files badge publication before selection synchronization and recovery checkpointing.

Expected source/codec/configuration rejection uses checked APIs. Invalid or oversized `SetDocument` input and
invalid file metadata leave source, selection, saved facts, recovery and histories intact. Capacity remains
65,536 UTF-16 units and produces the existing localized source-unchanged limit status. Quote/Code Block and
Find refusal preserve the selection and both history directions. Recovery parsing/serialization validates
source, path and selection before writing or publishing; refused serialization preserves its output buffer.
Unexpected C++ allocation failure remains fatal; see [the crash-handling contract](Core_CrashHandling.md).
