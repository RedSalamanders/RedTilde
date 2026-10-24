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
Attached hidden text-service import/app-callback integration covers growing previews, equal-length scalar and
CRLF boundary replacement, cancellation retaining seeded undo/redo, and one result commit followed by undo/redo.
It uses the actual editor and installed application callbacks; it does not qualify OS IME message delivery or
physical composition. Hidden native-message attempts that the host correctly refuses remain failed fixtures.
Prose edits do not relayout chrome, but shifted outline targets still navigate correctly and changed heading
labels/levels update their controls. Dirty state must survive direct resets and same-length edit/undo. Inspector
tests distinguish code from current math syntax and retain the oversized-source guard. Native completed-frame
and default/WARP resource probes run through the same executable as Core, with their paired study protocol in
[Testing_Performance.md](Testing_Performance.md).

PowerShell evidence tooling has independent numerical oracles under `Tests/` and is syntax-checked with the
other scripts. Retain the full v2 study audit and adversarial rejection receipts alongside the raw studies.
Independent scope commands pass their manifest `arguments` as command-line tokens to the current PowerShell
engine running the target script with `-File`. Empty arrays invoke strict no-parameter scripts, and named
switches bind as declared. The scoped-testing fixture exercises both cases through the public
`Test-Changes.ps1` runner. Each temporary Git repository sets `core.longpaths=true` locally
before staging; a tracked and committed fixture path exceeds the Windows path limit without
changing global Git configuration.
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
`docs/examples/weekly-report-2026-10-15.md` is a runtime input to the reference-dark DPI captures;
its edits select `Captures` in addition to normal Tooling/Format validation, rather than prose-only coverage.

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

The first HTTP case directly supplies synthetic Windows/MSVC ABI records to the internal remote-image import
filter, without raising exceptions or changing DLLs. Independent encoded expectations cover module/procedure
fallbacks, captured access-denied errors, zero/missing/malformed error metadata, and unchanged output plus
continued search for unrelated access-violation/C++ exceptions. Early URI/limit/cancel/abort checks preserve
caller bytes and existing network-module identities before the owned loopback request exercises real first use.
These record oracles establish error mapping, not recovery from an actually missing Windows system DLL.

Physical focus/IME/AT, release/package acceptance, hardware-specific qualification and paired resource/performance studies retain their owning gates. Hidden/scoped/native CI passes never close those gates. Native test basenames use contextual `Scope.Tests.Something.h/.cpp`; inventories cover active files only. Immutable historical receipts, archived source and legacy references are not renamed or rewritten.

RedTilde partitions its 53 reviewed native cases into Storage (10), Settings (4), Recovery (5) and Editor (34). Native listing and execution must equal Tests/native-case-catalog.json, reject unknown/duplicate/non-self-test scope arguments, and never count a catalog listing as execution. A Full run executes each partition once in an isolated process/data root and checks the complete union; the legacy --self-test default still names All. Editor observations remain non-cacheable because they include native resource/render measurements. Preferences, recovery-restart, captures, process-crash, performance smoke and real ASan detection are separately selectable obligations. The paired S5/S6 studies retain their unchanged source/fixture/judge and remain separate from these functional scopes.

CI's native-scope job binds compiled inputs, test/build/workflow code, the version stamp and hosted image descriptor. A documentation-only continuation may retain the original native qualification only from an exact matching producer with all six actual successful Full native steps; skipped, failed, cancelled, incomplete or unavailable evidence cannot authorize it. The original source/profile/binary/environment evidence remains the observation; inherited rows explicitly claim no new execution, resource measurement or clipboard/environment result. Manual dispatch always executes fresh native qualification. Each native checkout verifies the descriptor before building. This also permits a documentation/Done closeout without repeating an identical already-qualified native matrix.

Reusable tooling/formatting evidence also binds the actual selected formatter path and binary hash. Installing a different formatter invalidates that evidence even when source, PATH spelling and Visual Studio metadata are unchanged. This does not change formatter selection or native behavior.


## Test-run retention

[Generated test-run evidence](Testing_RunEvidence.md) owns completed, failed and interrupted output storage and verification.

## Design refresh scenarios

The hidden capture obligation now contains 20 PNG/sidecar pairs: 17 named scenes including Files, Find,
Focus and equation-source editing, plus narrow 96/144/192-DPI layout variants. Each sidecar records requested,
native and host DPI, actual pixels/DIP, simulation state and invented fixture basename; the runner verifies PNG
IHDR dimensions and binds both hashes, toolchain, binary, source and pin in the receipt. Captures never activate
or publish environment-derived identifiers. The Files fixture freezes its proved model and retires only its
owned junction leaf before sealing evidence; the target and synthetic ordinary files remain in the run.
The Settings capture fixes its System preview to synthetic Light, independently of the ambient Windows
preference, before final layout/render. Its sidecar declares `effectiveSystemPreference=light` and
`systemPreferenceSource=synthetic-fixture`; other captures identify the Windows-preference-or-Light-fallback
origin. The existing theme regression starts the Settings fixture from both cached preference values,
requires identical actual PNG bytes and explicit provenance, and retains source/selection/history and
hidden-window focus assertions. Historical ambient captures remain unchanged observations with that limitation;
their missing preference provenance cannot establish a registry change or read failure.

Hidden preparation reconciles newly published immutable syntax with derived chrome before declaring the
scene settled, and paints that chrome on a following native frame. Device-replacement checks compare exact
pixels from the same settled scene and normalized client viewport; stale context labels are a failure.
Syntax-only reconciliation must preserve an explicit dismissal of the selected-text toolbar.
Spacing assertions measure full semantic frames or equal-font caret-top advances against the configured
line advance. DirectWrite caret hit-test height may be shorter than the line box; subtracting its bottom
cannot establish a paragraph or list margin. Viewport-clipped element bounds require explicit full-visibility
checks. Keep exact ACP, selection and history checks alongside the geometry oracle.
Hidden toolbar regressions use the canonical pretranslation hook with a native activation message and inspect
the real message queue across the callback's editor-focus transition. Command activation must generate no
character, preserve exact formatted source and remain one undo; a same-owner editor Space must still produce
its ordinary translated character. Real keyboard coverage must exercise the same pump. A direct keydown-only
control invocation does not establish translation ownership.

Hidden composition regressions reuse `Tests/Support/Support.Tests.TextServices.h` to call the canonical protected
text-service import on the actual editor through its base member pointer. They construct no derived editor,
replace no callback and acquire no visible IME session. Preview, successive same-ACP revision, cancellation and
result tests retain exact source/selection plus both history directions and committed-change/recovery counts.

`test-interactive.ps1` is an explicit, time-authorized obligation outside the ordinary hidden/Full suites.
It compiles the canonical exact-pinned DxUi InteractiveLease target into the consumer's fingerprint output and
uses that library's runner/protocol for consent, warning, session ownership, watchdog and verified restoration.
The child verifies its visible direct-parent warning and held session mutex before showing or injecting; each
input sequence also checks process-owned foreground. `-InteractiveAutoStart` requires prior explicit agreement
and records the visible 30-second countdown. The nine real keyboard stages exercise the same shortcut routing
as the ordinary message loop, preserve exact source/history and check native focus, including close. Failed
children, absent stages or unverified restoration cannot pass. This does not qualify physical IME/touch/AT.
