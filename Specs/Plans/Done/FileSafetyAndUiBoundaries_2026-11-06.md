# Save safety and UI failure boundaries

Status: DONE — implementation, scoped local profiles and hosted native Release qualification.
Implementation: `7a5ecc49d40d9ef007329fb9d21f2b1c69fbb771`.
Dependency: DxUi API 4 at `0ffaa1a6ae43f074f60e419eaa5becc5bc3fe0b3`.

- [x] Implement the single Reload/Save As/Overwrite/Cancel surface and guarded asynchronous continuations.
- [x] Recheck staged-save targets; preserve complete native file identity and reliable successful stamps.
- [x] Reject observed unstable reads, protect Save As aliases and expose specific UTF-8/capacity errors.
- [x] Guard shortcut/menu/message/save-completion failures and publish saved facts before fallible refresh.
- [x] Declare the native common-controls dependency; retain the original startup failure and counterexamples.
- [x] Repair long-checkout fixtures while retaining dedicated long-path and actual NTFS coverage.
- [x] Complete six zero-warning builds, 324 native case executions, 108 hidden captures and real ASan detection.
- [x] Verify original app/SDK/source/run archives and update normative specifications, catalog and usage docs.
- [x] Verify hosted main `a25f1835`: all 108 cases and 36 captures on native x64/ARM64 Release,
  both production ZIP smoke checks, all seven jobs and thirteen original artifact digests.

The [source-bound packet](../../../Measurements/FileSafetyAndUiBoundaries_2026-11-06/README.md) records 141
actual compiled inputs and independent original failures. This closes RT-07/Q6/Q7/C6 and the RT-12/PR-07/35
UI boundary scope. C6 is a documented best-effort race contract, not an atomic filesystem compare-and-swap.
Q8 cloud/share/WSL fallbacks, Q12 link-write policies, native ARM64 Debug/ASan, physical input and performance
acceptance remain with the [single active plan](../WIP/RemainingWork_2026-11-04.md).
