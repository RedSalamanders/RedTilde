# Validation and evidence

Storage failure regressions direct write and flush refusal through a thread-local fault under
`REDTILDE_ENABLE_TESTS`; production builds contain neither the setter nor the injection branches. Assert
the actual owned staging handle closes before deletion, original bytes/stamp survive, and no `.tmp` remains.
Rejected settings tests cover synchronous/asynchronous persistence and real hidden view changes. Draft and
template tests compare exact LF/CRLF/CR/mixed source, normalized recovery previews, unchanged-draft identity
and one-step history. Find uses invalid precommit UTF-16 admission to prove suppression unwinds and retry works.

Hidden no-activation scenarios observe foreground events filtered to the test process, including brief
activation followed by restoration or window destruction. Subscribe before creating the application,
retain the observation throughout the scenario, and check both queued events and current ownership.
Teardown-specific coverage checks again after destruction of the owned application.
Do not require an unrelated user window to remain foreground: the person may switch or close their own
windows during a hidden run. Keep window-visibility checks and all logical-focus assertions. Observer
negative controls replay its callback privately; they never activate a window or emit a false desktop event.

The hidden `markdown-style` capture scenario uses invented content matching the
heading/flow/equation/table/quote example. The Captures suite runs it at 96, 144
and 192 DPI and preserves each image's actual viewport/DPI provenance. Verify
visible heading rules, neutral horizontal table lines, content-sized columns and
centered display objects; these screenshots supplement geometry/source-affinity
tests and do not establish native-monitor or platform acceptance.
The `caption-close-hover` and `caption-maximize-hover` variants use the same
invented source at all three DPIs and synthesize hover only on the hidden host.
They preserve ordinary scenario/build/DPI provenance without moving the pointer,
taking focus, or opening the native Snap flyout.
Synthetic hover is applied after the final preparation message pump and retained
through readback. At each DPI, resting, close-hover and maximize-hover PNG hashes
must differ, so valid provenance alone cannot qualify a missing visual state.
Interaction probes settle content before staging synthetic input and capture the prepared frame without pumping ambient pointer-leave messages for their hidden HWND. Standalone captures retain their final preparation step.
The bounded application preparation helper drains independent Markdown analysis, source/code ink, line numbers
and whitespace marks for both visible editor surfaces before declaring capture quiescence. Content readiness
alone does not establish completed cosmetic publication. This harness rule never makes cosmetic work a
production input/frame prerequisite; visible production windows continue through their event-driven host wakes.
The large source-guide capture regression preserves exact source/history and proves no desktop activation.
The shared host stages hidden captures before swap-chain presentation; a consumer
must not read the D2D swap-chain target after Present and assume it is the current frame.
Release-policy evaluation uses the pinned SDK's current defaults plus explicit
consumer bindings and compiler/linker negative controls. Its v2 lease-policy
receipt records `sdkDefaultProfiles`; the historical v1 omitted-argument regression
remains historical evidence rather than a requirement to reproduce a repaired SDK bug.

Use `validate.ps1` for links/spec indexes, skill metadata, dependencies and build inventory; aggregate all failures.
The [skill package contract](Testing_SkillPackages.md) defines structural, package-reference and scoped-guidance
checks separately from model decisions. Its negative fixtures run without native builds or desktop access.
Use `format.ps1 -Check` for C++. Scoped iteration uses `Test-Changes.ps1 -Explain` then the affected default.
PrePush accounts for existing enabled CI; pending CI is never recorded as a pass. Do not rerun an identical full
matrix without a changed source, pin, binary, profile or unresolved result.

Mock build/admission fixtures use an explicit version matching their synthetic executable, independently of
the inherited hosted product version. Every positive and negative admission probe receives that same explicit
version in an isolated child process. A separate negative probe requests a different version and must be refused.
Keep source-byte, installed-header, compiler-context and resolved-dependency refusals under the matching
version so an unrelated inherited-version mismatch cannot make those negative controls pass vacuously.
Fixture tests never change the application's real build-admission or version policy.

The reviewed Core catalog keeps Storage, Settings, Recovery and Editor ownership. The complete Editor list
is checked before execution. The public runner executes its behavior, Performance-prefix and Rendering-prefix
cases in three separate hidden child processes (`EditorBehavior`, `EditorPerformance`, `EditorRendering`),
each with the unchanged 90-second watchdog. Every child must execute exactly its declared cases, and their
unique union must equal the full Editor catalog before recording a successful Editor result. Direct `Editor`
and `All` selection retain their complete case sets. Empty/duplicate groups, omissions, duplicates, catalog-only
execution and failed/timeout children cannot qualify a group. This separates expensive independent scenarios
from the behavioral batch; it changes no case assertion or performance acceptance band. Preserve earlier
whole-Editor timeouts with their original source, binary and raw reports.
When original working input bytes differ from governed checkout text conversion, retain originals and
their per-file map/archive hash. A Git commit alone does not recreate those working bytes. Label later
Git-derived checkouts as reconstructed and verify actual identity before reuse; never rewrite historical
receipts or guess line endings to satisfy a source hash.

Native test filenames use `Scope.Tests.Behavior.cpp/.h`. Cover strict Unicode and EOL/BOM round trips, invalid
input/capacity rejection, atomic write failure, external conflict, dirty-state/undo, view history/selection,
semantic formatting, local source, search/outline, image decoding/cancellation, Mermaid literal fallback,
equation parsing/unsupported input, settings fallback/live application and isolated crash reports. Native math
tests require both inline/display objects, matrix/delimiter geometry, local source/undo and device replacement.
File-lifecycle regressions hold the real worker at an event barrier before I/O, then execute the ordinary Close
command in a hidden window. Cover save success, actual replacement failure, retained recovery and original bytes,
duplicate Close, later edits, a usable retry, Open abandonment and stale-result refusal after Cancel. Test-only
barriers and picker substitution are excluded from production; release them on every fixture exit and retain
source-bound failing receipts before changing runtime behavior. Pump only the owned HWND, including its shutdown
notifications after host retirement, without acquiring desktop focus or clipboard.
State/class diagrams must become native objects with exact local source access. Recovery checks strict Unicode
envelopes, corruption/future versions, live-session leases, coalescing, cancellation, dirty Save As restoration,
unapplied drafts, original-file preservation and a real process crash followed by restart.
The crash/restart probe edits two actual Markdown files through ordinary Files activation, checkpoints both,
crashes the isolated child, then restores both exact sources as independent dirty Save As copies. It also proves
both original files unchanged. Native workspace cases cover Compose/Source/Split histories and oriented selection,
external file changes while inactive, all-document prompt cancellation, duplicate original paths, untitled copies,
64-document refusal, active leases, independently retired checkpoints, continuous typing's five-second admission
bound, cancelled/final session termination and holding a completed Open behind a local draft. These hidden cases
do not acquire desktop focus or claim OS restart, physical IME/AT or real-location qualification.

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

Preference-persistence tests await the SettingsWriter's bounded idle boundary and require its successful status
before opening the settings file once. Polling LoadSettings while the writer is active can hold a read handle
that blocks atomic replacement. The nested-view regressions retain their two-second wait budget, exact persisted
view and source/history assertions; do not retry a failed write or change production file-sharing policy to make
the observer pass. Retain the writer HRESULT on failure. A controlled held-reader reproduction establishes that
interference mechanism; it does not attribute an earlier failure whose worker result was not recorded.

Keep compiler-change failures and new-toolchain builds distinct from earlier successful receipts. A later
documentation/tooling edit requires a fresh source-bound build receipt even if the native binary stays identical.
Retain the original runtime receipt/source identity when closing documentation or validation-tool changes. Reuse
its native result only after proving identical compiled inputs, native harness, dependency, compiler, binary,
profile, environment and scope; run the changed validation tools separately. Never rewrite a historical receipt
as new execution. A changed native artifact requires the affected native execution again.

Current build receipts are schema 2 and bind the full source identity, byte-hashed compiled first-party inputs,
exact clean DxUi lock commit and source tree, source/package public-header inventories, resolved consumer-properties
bytes and evaluated dependency paths, selected compiler context, and complete output artifact closure. Before touching
the receipt or restoring dependencies, `build.ps1` rejects reparse points in the output, intermediate and
dependency-restore ancestry. It then revokes the prior successful receipt; every failure leaves the profile ineligible
for `-SkipBuild`. Unknown or mismatched build inputs require a verified
app-profile clean before compilation; documentation-only changes may reuse identical compiled outputs while updating
the full source binding. Tests, packaging and performance runs must use the shared artifact admission check. Old
receipts remain historical evidence only and cannot admit a current executable.

The app's `--self-test` and `--capture <path> --scenario <name>` never activate a window, read/write the system
clipboard or synthesize desktop input. Drive the same app/control commands as the UI. Native rendering captures
come from the public host's diagnostic bitmap hook and WIC; record the exact binary/dependency/scenario identity.
No desktop screenshot or Computer Use capture establishes app correctness.
Custom invented documents use `--document <path> --scenario sample-document` for Light or
`sample-document-dark` for Dark. These scenarios preserve the loaded document rather than substituting a
built-in fixture. Saved preferences do not choose a hidden capture's theme. Sidecars record the rendered
appearance separately from the ambient System preference and bind the document's fixture basename.
`Tests/Fixtures/reference-dark.md` is a runtime input to the reference-dark DPI captures;
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

RedTilde partitions its 128 registered native cases into Storage (25), Settings (6), Recovery (19) and Editor (78). Registration scanning includes reviewed Core `.cpp` and `.h` files from the native inventory. Native listing and execution must equal Tests/native-case-catalog.json, reject unknown/duplicate/non-self-test scope arguments, and never count a catalog listing as execution. A Full run executes each partition once in an isolated process/data root and checks the complete union; the legacy --self-test default still names All. Editor observations remain non-cacheable because they include native resource/render measurements. Preferences, recovery-restart, captures, process-crash, performance smoke and real ASan detection are separately selectable obligations. The paired S5/S6 studies retain their unchanged source/fixture/judge and remain separate from these functional scopes.

RecoveryRestart also runs test-only `--linked-save-crash-probe` and `--linked-save-restart-probe` children in one
explicit absolute harness directory. The first exits with exactly 79 after an admitted hardlink write has changed
only part of the shared target, while its writer and independent recovery lease are still live. The second proves
the OS released that lease and the ordinary scanner offers the complete admitted source/BOM/path/selection/generation.
Both aliases must contain the same actual partial bytes. A durable checkpoint marker distinguishes this controlled
interruption from a setup failure; no destructor or fault-handler recovery write may supply the missing content.
The commands are absent from test-disabled production builds and never create a window or acquire desktop input.

File-seeding fixtures create their owned parent directories explicitly before writing. Application initialization only admits asynchronous recovery setup; tests must not depend on its worker having created a fixture directory before the first storage call.

`Recovery.DuplicateCloseKeepsUnsavedPrompt` uses an owned hidden window and a test-only prompt callback.
The callback sends a second window Close and command Close while the original choice is pending. Assert one
prompt, no queued continuation, unchanged dirty source/history after Cancel, and a fresh prompt whose Discard
can finish closing. The callback does not run in production builds and does not replace physical dialog coverage.

`Recovery.RestoreKeepsClickedIdentityAcrossUnsavedPrompt` removes the clicked candidate through the normal
Discard command and worker completion while the hidden unsaved question is open. Cover both another candidate
remaining and an empty list: Restore must preserve the exact dirty revision, selection and undo history,
leave the other copy available, and allow a fresh explicit Restore of that copy. Keep the original unchanged-code
wrong-copy failure separate from successful fixed runs. The hidden scenario preserves foreground focus.
Evaluate the recovery wait's terminal facts after pumping hidden UI: ready, settled and the expected candidate
count must agree, with successful worker status. Publishing retirement can refresh the drawer and submit a
new checkpoint. Do not pump again after accepting the wait and then assert its earlier settled observation.

The hidden responsive-layout/DPI case retains the exact initial foreground and hidden-window assertions. A failure records which predicate changed, whether the observed foreground is the test window or another process-owned window, and whether the original window still exists. These observations do not attribute a historical foreground change or permit activation; preserve the failed run separately from later qualification.

Hidden startup coverage enables ordinary startup policy only through a test-build-only LaunchOptions field,
retains WS_EX_NOACTIVATE, and uses actual recovery scanning, file workers and coalesced preference writes.
Owned I/O barriers distinguish edits before the scan, edits during read and an explicit New that abandons the
old notification state. Catalog listing and syntax-only compilation never establish runtime startup acceptance.
External-link cases use explicit confirmation and handoff seams with owned gates. They exercise real fresh-STA
COM initialization, the shared activation route, source/selection/history preservation, failure publication and
application destruction before a delayed handoff retires. They never launch a real browser/mail app or contact
the network. Native shell association and physical interaction acceptance remain with their release/input gates.

The Files dirty-transition regression uses a nontrivial invented folder model, a separately selected/focused row
and a nonzero scroll position. Dirty/undo must change only the visible current-file badge without rebuilding the
model or altering selection, focus, scroll, footer or watcher snapshot. The canonical native TreeItem providers
must expose stable filename Name and empty/Edited/empty ItemStatus across dirty/undo, while a neighboring folder's
count status stays unchanged. SDK subscribed-client tests separately cover status event delivery. Filtered and
absent snapshots update bookkeeping; root changes retain full refresh. Context-caption coverage checks first synchronization, math entry,
same-context syntax changes and cancellation, with transition-bounded construction and the command's stable
accessible Name. These registered obligations require fresh execution for the source/pin under qualification.

CI's native-scope job binds compiled inputs, test/build/workflow code, the version stamp and hosted image descriptor. A documentation-only continuation may retain the original native qualification only from an exact matching producer with all six actual successful Full native steps; skipped, failed, cancelled, incomplete or unavailable evidence cannot authorize it. The original source/profile/binary/environment evidence remains the observation; inherited rows explicitly claim no new execution, resource measurement or clipboard/environment result. Manual dispatch always executes fresh native qualification. Each native checkout verifies the descriptor before building. This also permits a documentation/Done closeout without repeating an identical already-qualified native matrix.

Reusable tooling/formatting evidence also binds the actual selected formatter path and binary hash. Installing a different formatter invalidates that evidence even when source, PATH spelling and Visual Studio metadata are unchanged. This does not change formatter selection or native behavior.


## Test-run retention

[Generated test-run evidence](Testing_RunEvidence.md) owns completed, failed and interrupted output storage and verification.

## Design refresh scenarios

The hidden capture obligation contains 36 PNG/sidecar pairs: 21 named scenes including Files, Find,
Focus, equation-source editing, quote/scrollbar and diagram-label Light/Dark scenes, plus narrow,
reference-document, Markdown-style and caption-hover 96/144/192-DPI variants. Each sidecar records requested,
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
Before that direct MSBuild invocation, it forwards the receipt-pinned compiler, SDK, host architecture and
configuration-specific whole-program policy through the shared build-argument helper described in
[the build contract](../Build/Build_Toolchain.md). Helper project references must not overwrite the consumer's
policy-fingerprinted SDK archive with different defaults. Evaluated lease/archive profile regressions run without
compilation, activation or input and do not replace the real authorized keyboard obligation.
The child verifies its visible direct-parent warning and held session mutex before showing or injecting; each
input sequence also checks process-owned foreground. `-InteractiveAutoStart` requires prior explicit agreement
and records the visible 30-second countdown. The nine real keyboard stages exercise the same shortcut routing
as the ordinary message loop, preserve exact source/history and check native focus, including close. Failed
children, absent stages or unverified restoration cannot pass. This does not qualify physical IME/touch/AT.

An observed helper-induced archive-policy mismatch requires retention of the actual archive/command evidence and
a controlled stop before primary performance collection. An unchanged, already linked static app executable
does not prove the current archive still matches its fingerprinted policy. Preserve all prior runtime identities,
failures and uncollected study declarations; restore the canonical archive and obtain the required new-source
qualification before a newly declared continuation. Never retag prior runs or issue a primary permit to bypass it.

Tooling and Format receipt identity includes PowerShell, the selected formatter and the actual toolchain/runtime inputs. It does not discover or launch Python or inspect PyYAML: neither is a dependency of these scopes. Unrelated Python installation changes must not add a dependency to their receipt contract.

## Mocked build version isolation

Build-input guard fixtures that write a text stand-in for the executable explicitly select build number zero.
They exercise a nonzero inherited product build number in the child environment and verify the fixture receipt
retains zero. Hosted qualification version inputs must not leak into this stand-in. The production executable's
frozen-version validation remains enabled; this fixture does not establish native PE version coverage.

Wide-layout assertions explicitly size and verify their owned hidden viewport through the existing Resize helper.
CreateWindow with default placement can be constrained by a small hosted desktop; a requested initial size is not
proof of the actual viewport. Contextual-equation coverage starts at 1024 DIP, verifies deferred hidden preview,
then reveals and validates native math at 1440 DIP. Persistent sidebar-label checks use a declared 1440-DIP viewport.
No fixture changes the desktop mode, foreground window or production responsive policy.
