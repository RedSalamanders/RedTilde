# Settings, theming and local crash diagnostics

Settings live under `%LOCALAPPDATA%\RedTilde` and use a versioned UTF-8 document. A caller may select an isolated
data directory for tests. Themes are System, Light, Dark and Paper. Respect Windows high contrast before custom
colors. Persist appearance, view, outline visibility, Files-pane visibility and width, reading width, text scale,
inactive-block dimming and remote-image opt-in. Defaults are System appearance, Compose view, outline visible, Files pane closed,
Files width 280 DIP, reading width 760 DIP, text scale 100%, inactive-block dimming enabled, and remote images disabled. Files widths are
limited to 200–400 DIP, reading width to 480–1040 DIP, and text scale to 75–200%; invalid fields fall back
independently without discarding valid neighboring settings.
Focus mode is session-local and is not persisted. Apply persisted
appearance, reading width, text scale and dimming immediately; no restart is required. Reduced motion follows
Windows preferences. The version-1 `remoteImages=0|1` field defaults independently to disabled on missing or invalid
input. The live privacy command retires remote requests/pixels immediately; local image identity and completion
remain valid. Invalid settings fall back per field and retain the rejected file for inspection. Persist
through the same atomic writer as documents. Settings errors must not prevent editing a document. Preference
persistence uses one active application worker and one coalesced latest pending state. Admitted writes finish after
the window closes without joining storage on the UI thread; notifications lose their window binding.

Install local crash handlers before UI/workers. Record a uniquely named `.dmp` with a bounded crash path and a
pending marker; do not allocate while formatting crash metadata. DbgHelp is loaded before the fault. Handle
unhandled SEH, terminate, invalid CRT parameters, pure virtual calls and abort. A reentrant crash must not recurse.
The next ordinary launch identifies the pending report and offers its location. Reports remain local; no upload
or telemetry is implied. Bound retention to ten app-owned dumps during normal startup. Dumps may include document
text; the user must choose any later sharing action. A crash probe runs only in an isolated harness child process.

The crash handler is a last-resort diagnostic, not a recovery save. Recovery snapshots are written during healthy
execution. ASan owns sanitizer diagnostics and must still pass a real detection probe before qualification.
