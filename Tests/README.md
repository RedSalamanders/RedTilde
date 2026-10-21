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

Commit and build the instrumented Release baseline before using `performance-paired.ps1 -FreezeBaseline` with
an `-OutputRoot` in a fresh `.build` directory. Use `-Baseline <frozen-directory> -SameBinaryControl` for A/A,
then omit that switch for the candidate study. The default has 12 independent blocks per Compose/Split scenario;
source, native adapter/driver, compiler, pin, fixture, harness and power identities must match. All raw slots
and failed runs remain under `.build/performance`. The new judge is review-required and cannot close DxUi D4c.
