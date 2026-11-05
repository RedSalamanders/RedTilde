# Settings, theming and local crash diagnostics

Settings live under `%LOCALAPPDATA%\RedTilde` and use a versioned UTF-8 document. A caller may select an isolated
data directory for tests. Themes are System, Light, Dark and Paper. Respect Windows high contrast before custom
colors. Persist appearance, view, outline visibility, Files-pane visibility, selected root and width, reading width, text scale,
inactive-block dimming, completed-task strike-through, remote-image opt-in and optional Markdown syntax. The
`strikeCompletedTasks` field accepts only 0 or 1; missing or invalid values keep the disabled default. It is live
presentation policy shared by Compose and Split. Defaults are System appearance, Compose view, outline visible, Files pane closed,
Files width 280 DIP, reading width 760 DIP, text scale 100%, inactive-block dimming enabled, remote images disabled and every optional Markdown syntax enabled. Files widths are
limited to 200–400 DIP, reading width to 480–1040 DIP, and text scale to 75–200%; invalid fields fall back
independently without discarding valid neighboring settings.
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
Focus mode is session-local and is not persisted. Apply persisted
appearance, reading width, text scale and dimming immediately; no restart is required. Reduced motion follows
Windows preferences. The version-1 `remoteImages=0|1` field defaults independently to disabled on missing or invalid
input. Seven optional Markdown syntax fields — `syntaxHighlight`, `syntaxSuperscript`, `syntaxSubscript`,
`syntaxInserted`, `syntaxWikiLinks`, `syntaxDefinitionLists` and `syntaxAbbreviations`, each `0|1` — default
independently to enabled on missing or invalid input and apply immediately. They are application-owned names;
never persist library extension flag values. The live privacy command retires remote requests/pixels immediately; local image identity and completion
remain valid. Invalid settings fall back per field and retain the rejected file for inspection. Persist
through the same atomic writer as documents. Settings errors must not prevent editing a document. Preference
persistence uses one active application worker and one coalesced latest pending state. Admitted writes finish after
the window closes without joining storage on the UI thread; notifications lose their window binding.

After accepting Close, stop producers and detach the editor host. If an admitted preferences write or recovery
retirement is still pending, hide the application window and retain its notification HWND until the preferences
writer is idle and recovery has finished. Recheck both states after dispatched messages as well as completion
notifications; either service may finish first. Window destruction requires both finished states; service closure
clears its notification binding. Do not join storage on the UI thread or retire the HWND after only one service completes.
`WM_QUERYENDSESSION` immediately permits session termination and requests checkpoints for every dirty workspace
document without a modal save question or shutdown-blocking reason. At final `WM_ENDSESSION(TRUE)`, editing
commands stop, all workspace checkpoints are requested again and recovery may wait for at most 2,000 ms. A
failure/timeout is recorded; it cannot turn the bounded allowance into an indefinite join. `WM_ENDSESSION(FALSE)`
keeps the window, histories, pending snapshots and editing available. Ordinary Close retains its asynchronous
worker-settlement contract. An ordinary visible launch registers Windows application restart for update/logoff
coordination with crash/hang restart disabled; crash recovery remains the next ordinary launch's local preview.
Synchronous native window and accessibility notifications during document-change publication may re-enter
accepted Close; resumed updates must stop before using controls detached by that Close. This includes the
Files badge publication before selection synchronization and recovery checkpointing.

Install local crash handlers before UI/workers. Record a uniquely named `.dmp` with a bounded crash path and a
pending marker; do not allocate while formatting crash metadata. DbgHelp is loaded before the fault. Handle
unhandled SEH, terminate, invalid CRT parameters, pure virtual calls and abort. A reentrant crash must not recurse.
The next ordinary launch identifies the pending report and offers its location. Reports remain local; no upload
or telemetry is implied. Bound retention to ten app-owned dumps during normal startup. Dumps may include document
text; the user must choose any later sharing action. A crash probe runs only in an isolated harness child process.

The crash handler is a last-resort diagnostic, not a recovery save. Recovery snapshots are written during healthy
execution. ASan owns sanitizer diagnostics and must still pass a real detection probe before qualification.
