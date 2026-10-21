---
name: test-harness
description: Write RedTilde behavioral tests, deterministic hidden app captures and source-bound validation receipts.
---

Read [the validation contract](../../../Specs/Testing/Testing_Validation.md). Use contextual
`Scope.Tests.Behavior.cpp/.h` names. Drive public controls and the same application commands through a hidden,
non-activating window. Do not acquire clipboard/foreground/cursor in ordinary tests. Capture the app render,
never the desktop. Receipts include source/pin/toolchain/binary/scenario; preserve failures and distinguish
focused tests, builds and qualification. Measure paired Release latency/memory on a fixed fixture. Native
ARM64, sanitizer detection, physical AT/IME and authorized focus tests remain independent gates.

For S5, follow [the performance contract](../../../Specs/Testing/Testing_Performance.md) and use
`performance-paired.ps1`. Freeze and measure the instrumented baseline before optimizing. Keep submission/GPU
wait diagnostics alongside complete latency and owner CPU cycles; retain every A/A and A/B slot and refusal.
