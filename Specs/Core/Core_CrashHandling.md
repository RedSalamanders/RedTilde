# Local crash handling and process failure boundaries

Status: current normative contract

Settings, theming, live reload, preferences persistence, and Close worker settlement live in [the settings contract](Core_Settings.md). Process ownership and application lifecycle boundaries are summarized in [architecture](Core_Architecture.md).

## Local crash handler and minidump retention

Install local crash handlers before creating UI or starting workers (`InstallCrashHandler` in `Common/CrashHandler.cpp`). Record a uniquely named `.dmp` with a bounded crash path and a `pending.txt` marker; do not allocate while formatting crash metadata. DbgHelp must be loaded before a fault occurs.

Handle unhandled SEH exceptions, `std::terminate`, invalid CRT parameters (`_set_invalid_parameter_handler`), pure virtual calls (`_set_purecall_handler`), and `SIGABRT`. A reentrant crash must not recurse: serialize DbgHelp use and wait on the active dump event instead of writing concurrently.

The next ordinary launch identifies the pending report (`pending.txt`) and offers its location (`IDS_CRASHREPORT`). Reports remain local; no upload or telemetry is implied. Bound retention to ten app-owned `RedTilde-*.dmp` dumps during normal startup, removing each retired dump's paired `.record` sidecar. Dumps may include document text; the user must choose any later sharing action.

The crash handler is a last-resort diagnostic, not a recovery save. Recovery snapshots are written during healthy execution. AddressSanitizer owns sanitizer diagnostics and must still pass a real detection probe before qualification.

## DxUi API-5 protocol-2 reporter

DxUi API 5 uses an explicit process-owned protocol-2 reporter (`DxUiFailureHost`). Prepare the bounded crash path, handles, and loaded DbgHelp before SDK use; start its dedicated writer thread before installing `DxUi::Failure::ProcessHooks`.

The app-owned reporter preserves the protocol record's natural `CONTEXT` alignment, places that record first, and explicitly accounts for its scalar tail; a `static_assert` size check forbids implicit member or tail padding on every compile target. Keep compiler warning policy enabled (`/Wall`) rather than suppressing alignment diagnostics.

Copy the fixed `DxUi::Failure::Record` without allocation and keep the faulting thread held while the writer attempts its dump. The dump exception stream and fixed `.record` sidecar retain the original exception, machine context, thread, source site, and frames; the writer's thread and context must never replace them. Record the actual native dump `HRESULT`; report completion has a 15-second bound. Reports use a process-creation identity plus PID/time. A missing or unwritable configured destination remains a checked diagnostic failure; never silently substitute another directory or upload.

## Process-owner lifetime and shutdown drain

Keep the reporter (`DxUiFailureHost`), module registration, and COM owner alive until all admitted callbacks (`ProcessWork`) and retained UIA peers retire. Worker admission is counted before submission, bounded to 256 callbacks, and closes before ordinary process drain. A refused submission releases its lease without publishing a job. A callback releases its lease after owned SDK objects and scopes; a WinHTTP transfer retains its lease until the final native `HANDLE_CLOSING` notification.

Ordinary drain pumps owner/COM messages, preserves `WM_QUIT`, then disconnects process-wide UIA providers (`UiaDisconnectAllProviders`) and retries failure-host `BUSY` for a cooperative five seconds. A component must not perform that process-wide operation. Timeout is a failed shutdown with fatal diagnostics (`DxUi::Failure::FailContract`), never permission to unload live owners.

Final `WM_ENDSESSION(TRUE)` keeps the existing 2,000-ms recovery-only allowance ([Core_Settings.md](Core_Settings.md)); its heap `ApplicationRuntime` process owner stays alive until OS teardown (`KeepForProcessTeardown`) so a held codec callback adds no ordinary drain/join deadline.

## Checked APIs and allocation failures

Expected source/codec/configuration rejection uses checked APIs: invalid or oversized `SetDocument` input and invalid file metadata leave source, selection, saved facts, recovery, and histories intact. Capacity remains 65,536 UTF-16 units and produces the localized source-unchanged limit status. Quote/Code Block and Find refusal preserve the selection and both history directions. Recovery parsing/serialization validates source, path, and selection before writing or publishing; refused serialization preserves its output buffer.

Unexpected C++ allocation failure (`std::bad_alloc`) remains fatal, including at application worker and resource-formatting boundaries.

## Isolated crash, sanitizer, and failure-report probes

Crash and sanitizer probes run only in isolated harness child processes under `REDTILDE_ENABLE_TESTS`:

- `--crash-probe`: triggers an unhandled non-continuable exception through the installed crash handler to prove one local minidump (`MDMP`) and `pending.txt` marker are written.
- `--recovery-crash-probe`: stages two dirty workspace documents, waits for settled recovery checkpoints, and triggers `CrashProbe()` so `--recovery-restart-probe` can prove restart restoration.
- `--linked-save-crash-probe`: interrupts a multi-hardlink save at its controlled partial-write checkpoint (exit 79) with its writer and recovery lease still live so `--linked-save-restart-probe` can prove recovery preservation.
- `--failure-host-probe <kind>`: exercises protocol-2 failure-host recording and shutdown drain boundaries in an isolated child process.
- `--asan-probe`: exercises an intentional isolated heap-use-after-free read in `ASan Debug` builds so `test.ps1` verifies real AddressSanitizer instrumentation.
- `--failure-report-self-test`: verifies test-process failure routing (`RedTildeTestFailureReports::RouteAwayFromDialogs()` and `RedTildeTestFailureReports::RunSelfTest()` before `InstallCrashHandler`), ending with stderr diagnostic text and exit code `3` without modal dialogs in Debug / ASan Debug, and exiting `0` in test-enabled Release.

In test-disabled production builds (`!defined(REDTILDE_ENABLE_TESTS)`), every test and probe switch (including `--failure-report-self-test`) refuses immediately with exit code `2` before resolving profile paths, installing the crash handler, or creating a private data directory. Standalone informational flags (`--version`, `-v`, `--help`, `-h`, `/?`, `-?`) likewise exit without creating profile or `Crashes/` directories. See [validation and evidence](../Testing/Testing_Validation.md).
