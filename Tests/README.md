# Tests

Run `test.ps1 -Configuration Debug -Suite Core`; use `-SkipBuild` for the exact existing artifact.
`-Suite Captures` generates non-activating application renders; `-Suite Crash` runs an isolated child fault.
`-Suite All` includes all three. ASan Core/All must detect an isolated intentional heap-use-after-free.

Native cases are compiled from `Scope.Tests.Core.cpp` into the same executable and run with `--self-test`.
Core also runs a hidden child-process probe that changes preferences, closes immediately and verifies the final state after process exit.
They exercise public controls and app commands without desktop input, clipboard access or foreground activation.
Receipts and failed logs remain under `.build/tests/<run>`. The validation contract lists broader gates.

The S5 harness adds `--performance-probe <json>` and `--resource-probe <json>`, with an explicit `--document`
fixture containing `S5_EDIT_HERE`, `--scenario compose|split`, `--cycles 4..64` and optional `--warp`.
Each edited sample waits for acknowledged native geometry and GPU completion. Resource mode also tests local
images, hidden idle, resize, exact pixels across three simulated device replacements and asynchronous close.
The two Core cases run these protocols on both default and WARP hosts without showing a window.

Core also checks deferred hidden mirrors, exact Split synchronization/undo, offset-only outline navigation,
heading label/level changes, same-length dirty resets and syntax-backed equation inspector selection.

Commit and build the instrumented Release baseline before using `performance-paired.ps1 -FreezeBaseline` with
an `-OutputRoot` in a fresh `.build` directory. Use `-Baseline <frozen-directory> -SameBinaryControl` for A/A,
then omit that switch for the candidate study. The default has 12 independent blocks per Compose/Split scenario;
source, native adapter/driver, compiler, pin, fixture, harness and power identities must match. All raw slots
and failed runs remain under `.build/performance`. The new judge is review-required and cannot close DxUi D4c.

Audit the complete studies independently, including every raw median, receipt hash, control flag and adjusted
probability:

```powershell
.\Tests\Scope.Tests.Performance.Evidence.ps1 -SelfTest
.\Tests\Scope.Tests.Performance.Evidence.ps1 -StudyPath @('.build/performance/aa', '.build/performance/ab') -Output .build/performance/audit.json
```

The typed subset-expansion reference uses a separate numerical implementation. `validate.ps1` runs its four
known-value oracles and parses the test scripts. Auditing is a numerical/integrity check; it does not approve
the policy or compensate for competing activity, unstable controls or unavailable physical/GPU counters.
Resource idle CPU is bracketed separately from the expensive process/thread snapshots. Keep both costs and
unavailable DXGI counters explicit. The reviewed [S5 packet](../Measurements/S5_2026-10-21/Final/README.md)
includes successful audits, six expected rejection cases and the remaining acceptance gates.

For the native-v2 noise investigation, use `performance-observed.ps1 -Baseline <new-frozen-baseline> -OutputRoot
<fresh-.build-directory>`. It records one A/A and A/B study (12 blocks/view, 64 cycles each), total CPU and selected
process activity. The paired judge's family and bands are unchanged. Global thread inventory is confined to resource
or explicitly post-primary diagnostic phases; latency memory observations wait for recovery settlement. Command CPU and processor-boundary arrays are
diagnostics, not new acceptance slots. `Scope.Tests.Performance.Observation.ps1` tests operation/chronological
diagnostics, legacy parsing, malformed v2 samples and truthful CPU activity classification. Native Core verifies
settled memory and diagnostic output; all original source/history/resource assertions remain enabled.

Add `-ControlDiagnostics` on both observed studies (or `--control-diagnostics` on a native probe) to capture
ordered application edit phases, preparation/GPU-wait CPU, frame counts and memory after fixed hidden idle.
These opt-in clocks change observed cost, so use a new common frozen baseline. Keep primary samples unchanged;
the late global inventory never substitutes a more favorable memory result. The independent auditor verifies
phase accounting and every saved/dirty caption call. Core also exercises immediate Unicode caption identity,
same-value write elimination and retry after rejected/reentrant native writes on the owned hidden HWND.

`Scope.Tests.Performance.Diagnostics.ps1` audits the retained small protocol fixture and changes its phase order,
CPU accounting, inventory ordering, caption count and declared mode while updating each native receipt hash.
The reader also rejects a downgrade to the readable seven-phase prototype when the study requires the current
protocol. The independent reader must reject each semantic defect for the expected reason. `validate.ps1` runs these six
regressions; fixture replay is not execution or qualification of a current native binary.
