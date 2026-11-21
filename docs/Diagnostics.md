# Diagnostics, Crash Reports & RedMonitor

## Version & Build Identification

Check the running build version in **Settings → About & Shortcuts** (`Ctrl+,`) or from a terminal:

```powershell
RedTilde.exe --version
# or: RedTilde.exe -v
```

This prints `RedTilde 0.1.<build> experimental`. Always keep the exact `RedTilde.exe`, matching `RedTilde.pdb`, and pinned dependency identity (`Dependencies/DxUi.lock.json`) alongside any diagnostic report.

## Diagnostics with RedMonitor

RedTilde emits bounded TraceLogging/ETW diagnostics labelled `RedTilde` through the shared header-only `RedMonitor` client SDK (`include/RedMonitor/Client.h`).

Install the production Release viewer independently from its standalone RedMonitor package:

- Run `Install-RedMonitor.ps1` from the extracted RedMonitor ZIP (`CurrentUser` installs under `%LOCALAPPDATA%\Programs\RedMonitor`; `LocalMachine` selects `%ProgramFiles%\RedMonitor`).
- Start RedMonitor separately, or run `RedTilde.exe --redmonitor`. A bare Debug launch may also start the installed Release helper asynchronously; Release builds, `--help`, `--version`, tests, and probes never auto-start it.
- RedTilde continues working normally if no viewer is installed or if ETW session permissions prevent collection (ETW capture may require *Performance Log Users* membership or elevation). An explicit `RedTilde.exe --redmonitor` invocation returns a non-zero exit code when the helper is absent.

Ordinary RedTilde builds compile only the pinned `Client.h` header and verify its local hashes—they never rebuild or package the viewer. Maintainers update the client with `Update-RedMonitorClient.ps1 -Commit FULL_SHA` and commit the reviewed header, MIT notice, and lock together without requiring a sibling checkout. Existing application logs, crash recovery, and preferences keep their own ownership.

## Local Crash Reports & Retention

Unexpected C++ exceptions or fatal SDK/application callback failures record diagnostics locally under the profile's `Crashes` folder (default `%LOCALAPPDATA%\RedTilde\Crashes`, or `<data-dir>\Crashes` when `--data-dir` is passed):

- `RedTilde-<timestamp>-<pid>-<creation>.dmp` — local minidump (`MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules`) retaining the original failing thread and context.
- Matching `.record` sidecar — fixed-buffer failure record (`protocol`, `process`, `thread`, `kind`, `result`, `dump_result`, `file`, `function`, `line`, `exception_code`, `context_ip`/`context_sp`, and stack frames) written without dynamic heap allocation from the fault path.
- `pending.txt` — next-launch marker so the next ordinary interactive launch notifies you of the local crash report path and clears the marker.

Reports may contain document text and **remain strictly local**—nothing is ever uploaded automatically. Expected input, capacity (`65,536` UTF-16 units), and native I/O refusals keep your editable document intact and show their ordinary status without crashing. Healthy debounced `Recovery/` checkpoints remain separate from fatal crash reporting. `ASan Debug` builds retain AddressSanitizer's independent detection diagnostics.

See [Core_CrashHandling.md](../Specs/Core/Core_CrashHandling.md) for the full crash-handling contract and [User Guide](README.md) for recovery instructions.
