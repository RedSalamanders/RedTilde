# RedTilde RedMonitor adoption

Status: ACTIVE qualification. The canonical detailed execution plan is
[RedMonitor extraction and adoption](https://github.com/RedSalamanders/RedMonitor/blob/main/Specs/Plans/WIP/RedMonitorExtraction_2026-10-21.md).
The current contract is [RedMonitor integration](../../Core/Core_RedMonitorIntegration.md).

- [x] Inspect current main and preserve diagnostics, crash and provider ownership.
- [x] Implement the bounded labelled native adapter and explicit installed-helper command.
- [x] Replace managed viewer builds/staging with one vendored client header, MIT notice and immutable lock.
- [x] Add local-only verification and explicit two-file update tooling.
- [x] Update contracts, usage and application-only packaging with the client MIT notice.
- [x] Run offline client controls and repository validators/format checks on the revised boundary.
- [x] Build all six profiles and run required x64 Debug/Release/ASan suites, including actual ASan detection.
- [x] Verify the actual app ETW Unicode/severity/PID/TID probe, missing-helper behavior and clean app-only package.
After final source validation, retain reviewed receipts under Measurements, fold changes, commit and push; verify the remote branch equals local HEAD.
- [ ] Close native ARM64 runtime, physical input/accessibility, controlled performance and release gates.

Earlier 2026-10-23 managed-viewer checkpoints remain historical evidence in Measurements and .build.
They do not qualify this revised dependency boundary. Tests never require an installed helper.
No ordinary build invokes the dependency updater. The viewer owns its separate Release packaging.
The final source matrix and app-only package replace the earlier managed-viewer qualification. The pinned public header commit is 9ebea6577fe9a5ced7647283c918f51f67d666fd. All three actual consumer ETW probes preserve Unicode/severity/PID/TID/time, and absent installed-helper handling returns 1 without application state. Clean package extraction verifies the client MIT notice and Settings with system-only PATH. Reviewed receipt paths/hashes live under Measurements/RedMonitor_Adoption_2026-10-23.

## Main integration - 2026-10-27

The approved integration combines current main with the retained MSVC 14.52 work and the standalone header-only RedMonitor client. Qualification of this combined source is recorded separately from earlier branch receipts. Historical passes and remaining native, interactive, performance and release gates are retained; the merge does not close them automatically.
