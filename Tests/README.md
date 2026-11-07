# Tests

Run `test.ps1 -Configuration Debug -Suite Core`; use `-SkipBuild` for the exact existing artifact.
`-Suite Captures` generates non-activating application renders; `-Suite Crash` runs an isolated child fault.
`-Suite All` includes all three. ASan Core/All must detect an isolated intentional heap-use-after-free.

Native cases are compiled from `Application.Tests.Core.cpp` into the same executable and run with `--self-test`.
Core also runs a hidden child-process probe that changes preferences, closes immediately and verifies the final state after process exit.
They exercise public controls and app commands without desktop input, clipboard access or foreground activation.
The native catalog contains 140 cases: Storage (30), Settings (7), Recovery (19) and Editor (84).
The current hidden scenarios cover Find case/repeat and replacement, text-scale key/wheel messages, window
placement and launch policy, recovery-first last-document startup, confirmed links with an owned fake STA
handoff, and independent real/controlled heap walks. They preserve exact source/history without opening a
browser or taking desktop focus. These checks do not establish real shell association, physical input or AT acceptance.
Linked-file cases check complete recovery before mutation, all hardlink identities, pre-write/partial-write/flush
failures, restart discovery, independent recovery leases, failed admission, late aliases and followed symbolic targets.
RecoveryRestart additionally terminates an owned child during a protected partial hardlink write, then verifies
the released lease and complete admitted recovery in a fresh process. It uses no desktop input or crash-handler writes.
Directed storage checkpoints reproduce edits during staging and post-publication read denial. The same native
regressions check equal-byte/timestamp object replacement, cooperative sharing and Save As aliases. Hidden app
commands cover all four conflict choices, duplicate Close, cancelled/stale choices, failed/stale Reload and
same-target Save As without foreground input or a second conflict error surface.
Timestamp-only touches avoid spurious conflicts; encoding/capacity cases independently verify the actual unit
count, specific localized explanation and unchanged source/BOM on refusal.
Full identity has an independent native `FileIdInfo` oracle, including the complete volume serial and both
halves of the identifier. Recoverable command/menu/Close/completion and supplied-message shortcut failures
retain source/history and pending write results. Menu callbacks keep F2/Link draft focus and reject root
retirement before focusing controls. These supplied messages do not inject desktop input.
Receipts and failed logs remain under `.build/evidence/test-runs/<id>/payload`. The validation contract lists broader gates.

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

## Affected testing

Use `.\Test-Changes.ps1 -Explain` from the repository root to see selected scopes, paths and reasons, then `.\Test-Changes.ps1` to execute. `-Scopes A,B` is an explicit partial selection; `-Mode Full` and `-Mode PrePush` retain full applicable obligations. `-Force` bypasses exact result reuse. The next ordinary edit compares against the last successful default affected run, not every older change on the branch. Read [the validation contract](../Specs/Testing/Testing_Validation.md) for mutation barriers, CI_PENDING and qualification limits.


Generated test output stays under `.build/evidence/test-runs/<id>/payload` on success, failure and interruption.
See the [retention contract](../Specs/Testing/Testing_RunEvidence.md) for verification and qualification boundaries.

## Design refresh interaction and captures

`test.ps1 -Suite Captures` generates 36 actual hidden renders: the normal presentation scenes plus Files,
Find/Replace, Focus and exact equation source, with narrow and image-2 weekly-report 96/144/192-DPI variants. The
reference-dark scene reads `Tests/Fixtures/reference-dark.md`, seeds the same twenty-file Reports tree
and selects the same current section. At 96 DPI its actual viewport is exactly 1440x900 pixels. Every PNG has a scenario
sidecar with requested and actual viewport/DPI, native window DPI, simulation state and invented fixture name.
The receipt binds both files to the exact build, source and DxUi pin. Simulated DPI exercises layout; physical
monitor acceptance remains separate. Files captures retire their owned junction before evidence sealing while
retaining the proved immutable tree model. No capture activates a window or takes a desktop screenshot.

After agreeing with the person at the desktop, run `test-interactive.ps1 -Configuration Release -SkipBuild`.
`-InteractiveAutoStart` is allowed only with explicit prior agreement and displays the canonical 30-second Start
countdown. It builds/reuses the exact pinned DxUi lease executable; the lease owns the session warning,
watchdog and verified focus/foreground/pointer restoration. The child refuses to show or inject input without
that visible owning parent. Nine real keyboard stages cover editor input/undo, Files, Find, Settings, Focus,
selection tools, F2, File menu and asynchronous close. The probe and restoration receipts remain separate;
physical IME, touch and assistive technology still require their independent acceptance.

RedMonitor controls verify the two vendored files entirely offline: exact bytes, missing/tampered inputs, invalid locks and reparse paths. No source checkout or viewer build is required. Actual application --redmonitor-probe tests cover cross-process Unicode/severity/PID/TID; installer closure tests belong to RedMonitor.

`Settings.VersionAndFieldFallback` covers the disabled default, invalid values and disk round trip for completed-task
strike-through. `Editor.DesignRefreshLiveSettingsAndEscape` toggles the real Writing preference across Compose,
Source and Split while preserving selection, source revision, empty undo history, dirty state and desktop focus.
The `task-presentation-light` / `task-presentation-dark` hidden capture scenes apply the preference explicitly and
record its actual value in their scenario sidecars. The corresponding sample is `docs/examples/tasks-and-insets.md`.
