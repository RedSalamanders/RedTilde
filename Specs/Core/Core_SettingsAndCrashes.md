# Settings, theming and local crash diagnostics

Settings live under `%LOCALAPPDATA%\RedTilde` and use a versioned UTF-8 document. A caller may select an isolated
data directory for tests. Themes are System, Light, Dark and Paper. Respect Windows high contrast before custom
colors. Apply theme, reading width, text scale and focus mode immediately; no restart is required. Reduced motion
follows Windows preferences. Invalid settings fall back per field and retain the rejected file for inspection.
Persist through the same atomic writer as documents. Settings errors must not prevent editing a document.
Preference persistence uses one active application worker and one coalesced latest pending state. Admitted
writes finish after the window closes without joining storage on the UI thread; notifications lose their window binding.

Install local crash handlers before UI/workers. Record a uniquely named `.dmp` with a bounded crash path and a
pending marker; do not allocate while formatting crash metadata. DbgHelp is loaded before the fault. Handle
unhandled SEH, terminate, invalid CRT parameters, pure virtual calls and abort. A reentrant crash must not recurse.
The next ordinary launch identifies the pending report and offers its location. Reports remain local; no upload
or telemetry is implied. Bound retention to ten app-owned dumps during normal startup. Dumps may include document
text; the user must choose any later sharing action. A crash probe runs only in an isolated harness child process.

The crash handler is a last-resort diagnostic, not a recovery save. Recovery snapshots are written during healthy
execution. ASan owns sanitizer diagnostics and must still pass a real detection probe before qualification.
