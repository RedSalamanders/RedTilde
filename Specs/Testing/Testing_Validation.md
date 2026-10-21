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
Optional command diagnostics require complete ordered phases, bounded subphase wall/CPU, reconstructed full
owner CPU and actual rendered-frame counters. Refuse missing/reordered/negative samples, early global inventory
and a missing late thread count. Replay older native-v1/native-v2 receipts without optional diagnostics unchanged.
The hidden native Core probe exercises both views with diagnostics enabled and checks idle quiescence after
the primary memory observation. New instrumentation requires a common baseline before the production repair.
Caption regressions cover Unicode names, immediate clean/dirty transitions, same-value dirty edits, source/history
restoration after a burst, failed Win32 writes and reentrant identity replacement on the app-owned hidden window.
Validator-owned evidence regressions replay the retained small fixture and reject six integrity-consistent
semantic corruptions for their expected reasons; they never substitute fixture replay for native execution.
Declared current diagnostics cannot downgrade to the readable legacy prototype to omit caption evidence.

Keep compiler-change failures and new-toolchain builds distinct from earlier successful receipts. A later
documentation/tooling edit requires a fresh source-bound build receipt even if the native binary stays identical.
Retain the original runtime receipt/source identity when closing documentation or validation-tool changes. Reuse
its native result only after proving identical compiled inputs, native harness, dependency, compiler, binary,
profile, environment and scope; run the changed validation tools separately. Never rewrite a historical receipt
as new execution. A changed native artifact requires the affected native execution again.

The app's `--self-test` and `--capture <path> --scenario <name>` never activate a window, read/write the system
clipboard or synthesize desktop input. Drive the same app/control commands as the UI. Native rendering captures
come from the public host's diagnostic bitmap hook and WIC; record the exact binary/dependency/scenario identity.
No desktop screenshot or Computer Use capture establishes app correctness.

Final gates: validator/format, x64 Debug/Release/ASan tests, real sanitizer detection, all ARM64 builds, native
ARM64 execution for runtime claims, paired Release resource/latency evidence, WARP/device-loss/hidden/dirty
render checks, physical IME/AT, and time-authorized interactive keyboard/focus restoration. Separate application
results from upstream library evidence. Keep failed receipts; a focused pass does not close a broader gate.

## Scoped iteration and exact evidence

`Test-Changes.ps1 -Explain` is the read-only planning entrypoint; `Test-Changes.ps1` runs affected scopes. The first run compares committed work since the local merge base with `origin/main` plus independent staged, working, rename-side, deletion and untracked paths. No ref is fetched. Unknown executable/build/shared inputs widen coverage. `-Scopes A,B` selects explicit scopes; unknown or incompatible selectors fail before execution.

A successful default affected run retains a source-byte and Git-index baseline for that checkout, platform, configuration and environment. The next edit selects only changes since that baseline. Staging or committing identical tested working bytes does not rerun tests. A genuinely different staged blob is retained. Failed, interrupted, corrupt, mismatched, explicit partial or CI-pending runs never advance this baseline. An explicit `-BaseRef` bypasses the incremental baseline. Unexplained artifact changes widen coverage. No selection is `NOT_EVALUATED`, never a new repository pass.

Only exact successful reusable scope results may be reused: source/dependency/build policy, binary/DLL/PDB and required deployed inputs, physical checkout, toolchain/environment, profile and normalized scenario arguments must match. Missing runtime inputs, JSON-only profiles, failures and mutation invalidate evidence. Receipts publish atomically only after successful unchanged execution. `-Force` bypasses result reuse; explicit Full/PrePush do not narrow obligations through the incremental baseline. IO/clipboard, stress, resource/benchmark observations, captures and process-fault workflows remain fresh when selected. Omission by an affected plan is not reusable qualification for omitted scopes.

`test.ps1 -Full -SkipTooling` runs the complete native obligation for its profile; `validate.ps1` runs independent tooling and scope regressions once. `format.ps1 -Check` is an independently reusable Format scope. `-Mode Full` selects the complete applicable local obligation. `-Mode PrePush` accounts for matching local work and forthcoming enabled CI. Deferral requires clean committed bytes, a matching reviewed workflow digest, matching profile/scenarios and stable HEAD/worktree through the API lookup. Disabled/missing/unreadable CI, changed runner contracts and direct default-branch pushes keep obligations local. Deferral is always `CI_PENDING`, never a pass or merge approval.

The PR workflow runs independent tooling once and full native tests in x64/ARM64 Debug, Release and ASan Debug. ARM64 execution is native on its hosted ARM64 runner; cross-builds alone never qualify runtime behavior. Every named PR check must succeed on the candidate before merge. The current private-repository account cannot enforce protected-branch checks; the merge procedure must verify the checks explicitly. Visibility is unchanged. There is no automatic main-push rerun of an identical validated merge tree. A changed integration tree, source, runner, profile, environment investigation or unresolved failure requires its corresponding new qualification. Manual dispatch remains available.

Physical focus/IME/AT, release/package acceptance, hardware-specific qualification and paired resource/performance studies retain their owning gates. Hidden/scoped/native CI passes never close those gates. Native test basenames use contextual `Scope.Tests.Something.h/.cpp`; inventories cover active files only. Immutable historical receipts, archived source and legacy references are not renamed or rewritten.

RedTilde partitions its 27 reviewed native cases into Storage (5), Settings (2), Recovery (5) and Editor (15). Native listing and execution must equal Tests/native-case-catalog.json, reject unknown/duplicate/non-self-test scope arguments, and never count a catalog listing as execution. A Full run executes each partition once in an isolated process/data root and checks the complete union; the legacy --self-test default still names All. Editor observations remain non-cacheable because they include native resource/render measurements. Preferences, recovery-restart, captures, process-crash, performance smoke and real ASan detection are separately selectable obligations. The paired S5/S6 studies retain their unchanged source/fixture/judge and remain separate from these functional scopes.

CI's native-scope job binds compiled inputs, test/build/workflow code, the version stamp and hosted image descriptor. A documentation-only continuation may retain the original native qualification only from an exact matching producer with all six actual successful Full native steps; skipped, failed, cancelled, incomplete or unavailable evidence cannot authorize it. The original source/profile/binary/environment evidence remains the observation; inherited rows explicitly claim no new execution, resource measurement or clipboard/environment result. Manual dispatch always executes fresh native qualification. Each native checkout verifies the descriptor before building. This also permits a documentation/Done closeout without repeating an identical already-qualified native matrix.
