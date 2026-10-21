# Validation and evidence

Use `validate.ps1` for links/spec indexes, skill metadata, dependencies and build inventory; aggregate all failures.
Use `format.ps1 -Check` for C++. Scoped iteration uses `Test-Changes.ps1 -Explain` then the affected default.
PrePush accounts for existing enabled CI; pending CI is never recorded as a pass. Do not rerun an identical full
matrix without a changed source, pin, binary, profile or unresolved result.

Native test filenames use `Scope.Tests.Behavior.cpp/.h`. Cover strict Unicode and EOL/BOM round trips, invalid
input/capacity rejection, atomic write failure, external conflict, dirty-state/undo, view history/selection,
semantic formatting, local source, search/outline, image decoding/cancellation, Mermaid literal fallback,
equation parsing/unsupported input, settings fallback/live application and isolated crash reports. Native math
tests require both inline/display objects, matrix/delimiter geometry, local source/undo and device replacement.
State/class diagrams must become native objects with exact local source access. Recovery checks strict Unicode
envelopes, corruption/future versions, live-session leases, coalescing, cancellation, dirty Save As restoration,
unapplied drafts, original-file preservation and a real process crash followed by restart.

App-derived work has behavioral coverage: hidden mirrors retain their revision through Compose/Source edits;
entering Split and subsequent Split edits synchronize source/selection without resetting editable history.
Prose edits do not relayout chrome, but shifted outline targets still navigate correctly and changed heading
labels/levels update their controls. Dirty state must survive direct resets and same-length edit/undo. Inspector
tests distinguish code from current math syntax and retain the oversized-source guard. Native completed-frame
and default/WARP resource probes run through the same executable as Core, with their paired study protocol in
[Testing_Performance.md](Testing_Performance.md).

PowerShell evidence tooling has independent numerical oracles under `Tests/` and is syntax-checked with the
other scripts. Retain the full v2 study audit and adversarial rejection receipts alongside the raw studies.
Native-v2 diagnostics also require cheap settled-memory snapshots, complete command CPU/processor arrays and
observer coverage. Observation tests cover process disappearance and single reads of volatile properties, CPU
classification and UTC/local/JSON date representations. An incomplete trace is retained as a failed collection;
its native measurements can be audited independently without qualifying host quiet or pooling attempts.
Keep compiler-change failures and new-toolchain builds distinct from earlier successful receipts. A later
documentation/tooling edit requires a fresh source-bound build receipt even if the native binary stays identical.

The app's `--self-test` and `--capture <path> --scenario <name>` never activate a window, read/write the system
clipboard or synthesize desktop input. Drive the same app/control commands as the UI. Native rendering captures
come from the public host's diagnostic bitmap hook and WIC; record the exact binary/dependency/scenario identity.
No desktop screenshot or Computer Use capture establishes app correctness.

Final gates: validator/format, x64 Debug/Release/ASan tests, real sanitizer detection, all ARM64 builds, native
ARM64 execution for runtime claims, paired Release resource/latency evidence, WARP/device-loss/hidden/dirty
render checks, physical IME/AT, and time-authorized interactive keyboard/focus restoration. Separate application
results from upstream library evidence. Keep failed receipts; a focused pass does not close a broader gate.
