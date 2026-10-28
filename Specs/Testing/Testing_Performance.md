# Completed-frame performance and resource validation

Status: S5 validation contract. A collected study is not automatically a qualified pass.

Keep the original forced-render results unchanged. They submit renders while edited semantic work can remain
pending; they are neither presentation FPS nor completed-frame latency. Comparisons that add native math or
diagram support also change the work. Measure an optimization against the same implemented features and pin.

The application harness must separately record the source transaction/callback, cooperative preparation and
submission, and GPU completion wait. A dirty sample starts from a fully prepared source, inserts into the fixed
prose edit site, waits for current syntax/semantic/native geometry and the GPU, and restores exact source by undo
before another sample. Refuse stale/pending preparation, failed rendering, changed source/history, an unavailable
completion fence or a deadline expiry. Keep raw sample arrays and app activity counters. Clean renders also wait
for GPU completion. These are hidden native completed frames, not screen presentation or physical input latency.

Use one common harness/fixture for baseline and candidate and retain the baseline before optimizing application
behavior. Compose and Split are distinct scenarios. Match Release/x64, compiler/options, dependency, fixture bytes,
dimensions/DPI, renderer, GPU/driver/WARP, OS/CPU and power policy. Verify source/build/executable/harness identities
before and after each process. Processes run serially. An independently drawn ABBA or BAAB order balances each
block; retain the seed and literal schedule. Twelve blocks provide 24 processes per side and scenario. Run a
separate same-executable A/A control with the same protocol; do not count within-block repeats as independent.

Report paired log-ratios and exact sign-flip inference with Holm correction across the predeclared timing and
memory slots. Timing and memory investigation bands are 5% and 2%, respectively, consistent with DxUi; they are
noise investigation bands, not permitted degradation. Keep same-side control drift, every flagged metric and all
raw rounds. Deterministic activity/resource bounds must hold in every process and cannot be averaged away.
The 12 primary slots are command wall time, complete edit/frame latency, clean complete-frame latency, owner-thread
CPU cycles, private bytes and working set in each view. Preparation/submission and GPU-wait medians/arrays remain
phase diagnostics: presentation pacing can redistribute their durations without increasing complete latency or
CPU work. Do not label a compensating wait increase as additional executed work. CPU cycles are not converted to
nanoseconds; retain power/CPU identity and A/A controls. This family is fixed before the locked paired study.
An uncalibrated/new judge or missing provenance leaves statistical acceptance open. Neither an A/A study nor a
candidate's own policy can qualify the upstream DxUi D4c gate. Confirmed degradation requires measured optimization,
scope/deferral options and developer advice before acceptance; never silently rebaseline.

Resource probes use the actual native app tree and preserve source/history/selection across default/WARP rendering,
dirty/undo cycles, hidden message-driven idle, resize and simulated device replacement. Check current native
geometry, image/cache bounds, generation changes and exact post-replacement pixels. Snapshot process private bytes,
working set, handles, GDI/USER objects, host caches and available DXGI usage at each named phase. A missing metric
is unavailable, never zero. Hidden/clean idle must request no additional preparation or rendering; production
workers continue blocking on events. Teardown probes retain before/after handle/resource observations.

A separately declared virtual-memory census is a diagnostic, not a substitute for those process counters.
`MEM_PRIVATE`-type committed ranges are not process-private bytes, mapped image size is not resident working set,
and an additional font mapping does not identify a causal allocation owner. Preserve unresolved mapping identities
and empty/exit snapshots explicitly. Suspension-based sampling must report bracket coverage and its missing costs;
intrusive stack walks and sparse instruction pointers cannot be presented as CPU shares or causal attribution.
Extended `QueryWorkingSetEx` page observations can include allocations outside the traditional process working set,
including AWE and large pages; summing its valid-page observations does not establish `WorkingSetSize` equivalence.
Use separately bounded `QueryWorkingSet` membership when that distinction matters, retaining each API's coverage,
limits, failures and observation brackets. Keep an unreconciled difference explicit; never subtract it from the
primary process counter or attribute it to an application allocation. See Microsoft's
[extended-query contract](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-queryworkingsetex)
and [working-set definition](https://learn.microsoft.com/en-us/windows/win32/memory/working-set).

A state-aligned diagnostic checkpoint follows the exact primary snapshot and precedes idle, report serialization
and other diagnostic inventory. Bracket nonce/event/file setup separately with process counters, retain known
handle/storage costs, and prohibit message pumping while the observer owns the checkpoint. Such a snapshot
includes diagnostic setup and is not untouched production-process memory. A resource-retention counterfactual
must bind the same executable and unchanged UI/source behavior to explicit process conditions, prove the intended
resource path was exercised or excluded, and report CPU-versus-memory costs. Cumulative build/paint counters
are not live-resource counts. Preserve all fixed slots and any refused setup attempt; diagnostic differences
do not replace primary acceptance outcomes. Public projections use safe mapping labels without raw paths,
account SIDs, volume/file identifiers or allocation addresses; private evidence retains the original bindings.

An owned-process heap trace must retain its executable, collection declaration, raw stream digests, loss and
schema counters, observer query brackets and versioned decoder. StackWalk association uses the original-event
timestamp/process/thread in its payload; a later header timestamp or an unknown header process is not that
identity. Validate the entire bounded stack payload against its schema instead of truncating at named fields.
Report unmatched, duplicate, ambiguous and unknown records. A redundant ReAlloc-summary interpretation must
state its exact observed allocation/free sequence, preserve the original allocation's recorded stack and byte counts,
retain its reconciliation count and expose unproven thread/allocator continuity. Consistent captured records
do not prove complete process allocation coverage. Module point observations are not load/unload lifetimes,
and phase-unmatched snapshots, including startup/teardown, cannot substitute for settled native memory phases.

Resource-only native reports also retain `firstCompletedPreparation` and a `preparation` object on resize and
device-recreation phases. Each brackets the operation, cooperative native preparation and a GPU completion fence,
then records completed wall time, owner cycles, process CPU and intermediate root-child Flush count. Snapshot
collection and post-replacement capture comparisons occur after that interval. Initial preparation includes GPU
fence setup; recreation includes retiring and rebuilding that fence. These observations measure hidden completed
work, not startup-to-visible or physical presentation latency. They stay outside the fixed twelve primary slots;
baseline and candidate require the identical revised common harness before comparison.

Bracket each idle interval with cheap process-time and owner-cycle reads. Resource snapshots enumerate system
threads and can consume significant CPU; record their collection wall/CPU cost separately. Cumulative CPU
differences between resource snapshots include collection work and must never be labeled idle CPU. The v2
judge preserves fractional Holm probabilities; numerical tests include independently counted mixed-sign tails
and the exact adjusted value. Retain v1 raw studies and invalid judge outputs as investigation history.

Audit collected v2 evidence with `Tests/Scope.Tests.Performance.Evidence.ps1`. Its typed reference calculates
sign-flip tails by subset expansion and Holm adjustment independently of the production PowerShell judge.
Check the retained schedule, unique native receipts/hashes, per-side binary/source identities, common harness,
compiler/dependency/environment metadata, raw sample counts/medians, the fixed family, every control drift and
directional finding. `validate.ps1` runs its known-value oracles. A numerical match does not approve the policy
or recreate missing binary/environment provenance. Altered results, bands, native bytes, reused slots, compiler
profiles and the invalid legacy judge must be rejected.

Record competing build/test activity and the limits of any sampler. A user-directed run may proceed on a shared
host, but it must retain that overlap and leave affected acceptance open. Do not discard flagged blocks, extend
the family after seeing results or repeat until a desired result appears. Report the collected observation and
name the outstanding qualification instead. A toolchain change requires a new retained baseline with the same
common instrumentation and current compiler; an old compiler's executable cannot be paired with a new one.

Compiler profile schema 2 compares the complete common Release invocation after removing reviewed source-file
arguments and replacing only the exact repository prefix in reviewed include/object/PDB path operands with
`<ROOT>`. Macro values and external paths stay byte-exact. Translation-unit inventories, raw command bytes and
their hashes remain recorded separately, so a code change or isolated worktree is not mistaken for a
toolchain change. Every project translation unit must have exactly one command and the same normalized flags;
mixed, missing, duplicate or forced-include commands refuse comparison. All optimization, diagnostic, macro,
sanitizer, include and output options remain in the profile. External paths stay exact. The separate dependency
identity still requires identical compiler/linker/MSBuild binaries, SDK headers/libraries, runtime, architecture
and exact DxUi pin. Both sides use this common instrumented protocol before a new baseline is frozen.
The evidence auditor recomputes the normalized profile hash, checks the unique homogeneous source inventory
and verifies each retained raw command file. Historical schema-less receipts remain readable as historical
evidence; new paired collection requires a freshly frozen schema-2 compiler profile.

The evidence auditor also requires exact common-input key sets, cardinality and hashes to match across baseline,
candidate and comparison. Historical schema-less smoke receipts may use only the exact seven-file legacy set, and
only when the study has no dependency-comparison/profile marker or raw compiler receipt and both compiler profiles
remain in the exact schema-less historical shape. The historical eight-file set remains readable for prior typed
studies; new studies use the nine-file set including `Build/ReviewedHeaderDeltaProfile.json`. Reviewed-header mode
requires that ninth input and binds its digest to the reviewed profile SHA-256. A marked/current study cannot
downgrade to the seven-file compatibility exception.

The native-v2 fixture (schema 2) keeps two fixed warmup cycles, all timed edit/undo samples and the same twelve
primary slots. It records command-only owner CPU cycles and owner processor IDs at transaction/frame boundaries
as diagnostics. Boundary equality does not rule out migrations that return to the same processor. Never infer
clock frequency or scheduling causality from these observations alone. Report insert/undo medians, chronological
halves and median absolute deviation separately without replacing the predeclared combined primary median.

Latency snapshots use only cheap process counters, report thread count as unavailable (`null`), and wait for
successful recovery settlement before the initial/final memory observations. These waits are outside timed
transactions; normal recovery remains enabled throughout the timed edits. Global thread inventory stays in
resource-only phases with its collection cost. Both sides require the identical revised harness and a newly
frozen baseline; native-v1 observations remain immutable and are not directly compared with native-v2 results.

`performance-observed.ps1` collects one serial A/A then A/B study, twelve blocks per view and 64 timed cycles by
default. Record this scope before running, retain every process and do not repeat until controls pass. Its observer
starts before either study and records system CPU and selected process deltas. A quiet CPU observation requires at
least two valid intervals, maximum total busy CPU at most 5%, no selected external CPU and no inaccessible or
partial processor-group observations. This is an observation, not an exclusive lease or GPU/physical qualification.
The complete study window must also have no observation gap above 2.5 seconds (including its ends). Missing
coverage cannot establish quiet. Tolerate disappearing process properties as unavailable observations; read
volatile values once and retain observer errors. Normalize UTC/local/JSON date representations before checking
coverage. A failed collector permits a documented replacement collection after repair, with every failed attempt
retained and no pooling or selection based on its statistical findings.
User-directed shared-host execution proceeds with activity recorded; a powerful host is not evidence of quiet.
Do not change power policy, affinity, priorities, the primary family or investigation bands to force acceptance.
A later collection explicitly requested after a reported host-condition change is a separate fixed-scope cohort.
Record that trigger before collection, preserve the earlier studies and assess actual activity anew; do not pool,
replace unfavorable controls or promote a human report of quiet to a verified quiet observation.

When one candidate is compared against multiple references, report each cohort and its A/A and A/B phase separately.
Bind the baseline source/build identity, measured overlay, dependency adoption mode, candidate source/build identity,
compiler profile, common harness inputs and raw receipts independently; keep a source anchor distinct from the measured
overlay commit. Do not substitute a later merged base for an original reference or combine cohorts into a single
acceptance result. Summaries retain every primary metric row, same-side control status and drift observations for each
cohort. A curated public projection may expose aggregate evidence hashes and synthetic workload identity, but must
exclude local absolute paths, machine/process names and raw receipt filenames; the private audit retains the full
receipt/path bindings. Unstable controls, missing quiet coverage or an unqualified upstream policy keep acceptance
open even when another cohort has improvements or no resolved regressions.

Opt-in `--control-diagnostics` / `-ControlDiagnostics` records bounded synchronous edit subphases in native-v2 receipts.

The same option may retain the existing DxUi frame metric sink's total/update/render/present wall durations. This sink is
borrowed on the UI thread only inside each edit or clean transaction and is cleared on every exit; the hidden
probe owns the sole registration in that process. Its callback allocates nothing and never reenters controls.
Keep at most 64 complete four-value attempt records per transaction and match the count to the probe's explicit
render requests. The sole owned hidden host's render-count delta must be positive and no greater than that count.
Update preparation may return before its render counter advances; retain those attempts too. Raw zero stage
values on early returns may represent unassigned timings and must not be treated as measured zero-cost work.
Reject missing, reordered, partial or overflowing sequences, mismatched request/host counts
and stage sums beyond total plus the declared 3-microsecond rounding allowance. Record vector capacities and
reserved payload bytes: diagnostic memory observations include that storage, while allocator metadata is not
accounted for. Frame durations are wall-time diagnostics, not owner-CPU attribution or completed GPU latency.
They never replace any primary outcome. A study declaring `dxui-frame-metrics-v1` requires every edit and clean
record and its storage metadata; historical receipts without this optional diagnostic remain readable.

`app-edit-phases-v2` separates native entry, document analysis, mirror, identity labels, native caption,
remaining chrome, selection, recovery and native exit. The initial seven-phase v1 prototype remains readable.
An explicit current diagnostic study requires v2; reject a downgrade even if its edited receipt hashes match.
Phase wall/CPU arrays must cover every timed transaction in order, stay within command cost, and reconstruct
full owner CPU with the preparation/GPU-wait diagnostics. Retain render and preparation counts. Arm the borrowed
observer only for a synchronous harness transaction, clear it on every exit, and perform no diagnostic clock
reads during ordinary editing. The optional instrumentation changes observed cost: freeze and measure a matching
common baseline before a production repair; never compare it directly with uninstrumented acceptance results.

Current opt-in reports also retain `app-selection-components-v1`: twelve disjoint selection components, ordered
per-transaction raw wall/cycle/call records and their enclosing selection interval. Arm it only around the
synchronous `Changed` selection publication, reject reentry/unwinding or failed clock reads, and mark unexecuted
components unavailable rather than supplying invented durations. Component sums must fit both their whole
invocation and the parent selection phase; the retained remainder must reconstruct that parent. Historical
reports without this optional object remain readable; a current study that declares it must retain it.

Opt-in native diagnostics may also retain the separate `app-editor-preparation-v1` object. For each measured
edit transaction, record every `Ready` call's `RenderInitialFrameForShow` attempt, capped at the shared 64-attempt
transaction bound. Each record binds its zero-based attempt index to the same ordered `dxui-frame-metrics-v1`
attempt and captures the primary and mirror `RichTextEditor::DebugGetState()` immediately before and after that
call, plus the host render-count endpoints. The collector reserves attempt and transaction storage before measured
edits, reports its reserved bytes, and rejects missing/extra attempts, index or render-count reconciliation errors,
and decreasing cumulative preparation counters. The recorded debug-state fields are raw snapshots; a zero work
field is not a timing or proof that a stage was skipped, and these snapshots do not attribute CPU among editor
stages or host traversal. The evidence auditor independently checks each retained attempt against its frame-metric
record, including transaction counts, ordering, counters and reserved-storage arithmetic. Historical reports
without this optional object remain readable. When disabled, this diagnostic performs no editor debug queries
and allocates no diagnostic storage.

A common diagnostic harness does not guarantee equal probe counts in different implementations. Report active
component-call counts when comparing outer phases: each timed scope adds its own clock/cycle reads, and the
enclosing interval/remainder includes that overhead. A phase delta cannot be attributed wholly to product work
when scope counts differ. Diagnostics-off acceptance executes no diagnostic clock/cycle reads; do not infer
zero instruction overhead from that gate or subtract a guessed per-probe cost.

After the primary memory observations, opt-in app storage records report known buffer sizes/capacities and
snapshot lengths without flattening source. Missing members in a historical implementation remain unavailable.
After all primary, idle and resource observations, an optional heap walk may inspect only the explicitly supplied,
long-lived default/CRT heaps; deduplicate handles and report incomplete coverage, per-heap errors and capture-only
cost separately from serialization. The app test-layer helper caps storage at 32 handles and each walk at one
million entries. Its aggregate busy/free/committed bytes are not process-private bytes, working set or allocation
ownership. No purge, trimming, affinity, priority or power-policy change is permitted. Include this helper in the
matched harness input closure before freezing a new baseline; retain the prior seven-input freezes as history.
Post-primary host counters may record configured text formats, brushes and direct logical root children. These
are object counts rather than retained bytes or a complete control-tree inventory. Sampling follows the heap
observation, so the counters cannot change that observation or any primary metric.
Heap-diagnostic tests distinguish logical allocation requests from native heap storage. Sanitizer interception
may redirect or quarantine the physical allocation; retain an independent range-membership observation and
require native-payload growth only for the ordinary native allocator. Keep the complete-walk, budget, duplicate,
invalid-handle and exact-boundary assertions active in every profile.

Diagnostic memory is sampled after the primary snapshot, then after 100 and 500 ms of event-driven hidden idle.
No additional render/preparation is allowed during those waits. Full global thread inventory runs only in the
last diagnostic observation, with its cost retained after all primary measurements. These observations test
quiescence and thread-count hypotheses; do not select a more favorable memory sample or replace the original
primary memory metric/band. Hardware/WARP contrasts perform different work and are diagnostic, not a rebaseline.
Retain a failed CPU-profiler policy request as unavailable evidence; do not change system profiling policy.

Hidden diagnostics do not establish physical visibility/occlusion, actual device removal, kernel/GPU lifetime,
real-focus caret/IME/AT behavior or native ARM64 execution. Preserve those gates separately. A cross-build, library
fixture or screenshot alone cannot close product resource acceptance. Reviewed application receipts belong in
`Measurements/`; all generated processes, executables, logs and private data stay in `.build`.

### Design refresh comparison viewport

The common native performance harness normalizes the owned hidden client area to exactly 1440 by 900 DIP at
the recorded native window DPI before warmup, without activation. It adjusts only outer window extents until
the actual client pixels match. Baseline native-caption and candidate merged-caption frames therefore measure
the same viewport. Freeze a new common instrumented baseline with this harness; older runs are not pooled.
The existing strict fixture, pin, compiler, renderer, native geometry and harness identity checks remain enabled.
For a dependency adoption, first compare original application chrome and refreshed chrome on the same new exact
DxUi pin; upstream library changes retain their independent baseline and qualification.
An additional actual-PR-base adoption study may explicitly select `-DependencyAdoption`. The default still refuses
different dependencies and compiler profiles. Adoption permits only the exact dependency commit/fingerprint pair to
change: API revision, platform, runtime, compiler, SDK, flags and every other dependency field must match. Retain and
independently replay separate public-header inventories for both the clean pinned source checkout and the actual
platform/triplet package include directory selected by the dependency fingerprint. Reject missing/changed bytes in
either inventory. When the compiler profile differs, permit only the exact SDK source include and fingerprinted
package include operands to vary; require one of each on both sides, retain the raw compiler command receipts, and
independently verify those raw commands reproduce the recorded profiles. Every other compiler option, macro, include
operand, quote and flag spelling remains exact. Historical schema-one source-only header evidence remains readable for
same-pin studies only; it cannot qualify dependency adoption.
This mode measures the complete follow-up, including the SDK adoption; it never replaces the original-UI comparison
on one common pin, changes a metric/band, qualifies the upstream policy or accepts a new ABI.

`-ReviewedHeaderDelta` is a separate, opt-in comparison for one fixed, reviewed SDK pair. It is not a general
allowance for API-revision-compatible header changes. The fixed profile binds exact old/new SDK commits and
fingerprints, the complete 58-row inventory (25 DxUi source headers and 33 vcpkg package headers), the two reviewed
SDK source header hash pairs, every unchanged inventory row, exact compiler include-command evidence and a
source-hashed isolated ABI probe. Only the exact reviewed additive declarations may differ; every other header,
dependency, compiler, runtime, SDK, target, flag and unknown field remains exact. The existing same-pin and
`-DependencyAdoption` paths continue requiring identical public-header inventories. The fixed profile is a common
harness input. The independent evidence auditor verifies its copied bytes and separately replays paired-study
compiler receipts; those receipts are not replaced by the ABI probe. The probe confirms equal tested object layouts and
method availability under its recorded isolated compile context; PCH is disabled and it links only the probe object,
so full application compile/link equivalence and DxUi library integration are outside its evidence. All 33 vcpkg
package rows and the other 23 DxUi source rows remain identical. A different SDK/header delta requires a separately
reviewed profile, refreshed common baselines and new positive/negative oracles. This mode does not change timing
bands, workloads, the metric family or acceptance
policy. Its isolated ABI probe does not establish complete application ABI/link compatibility or performance qualification.

The parsed-snapshot v2 profile is a separately reviewed source-coordinated extension for exact SDK `47a9d259`
to `4077f57d`. It permits exactly `DxUi.h`, `MarkdownSyntax.h` and `RichDocument.h` source-header changes,
with every other source/package row, compiler operand and dependency field governed by the same strict guards.
Its isolated source probe must verify all three exact new method signatures, the existing synchronization
signature on both sides, a 16-byte MarkdownSyntax increase with unchanged alignment, and unchanged measured
MarkdownParser, RichTextEditor, SourceSnapshot and Document layouts. These are named-type source/layout checks,
not binary interoperability. Consumers rebuild against matching pinned headers/library and require their own
native editing, rendering and input qualification. The original v1 validator and byte-exact fixture remain for
historical two-header studies; v2 needs its own actual compiler/header receipts and positive/negative oracles
before any collection. Neither profile changes workload, metric family, bands or acceptance policy.


The V4 reviewed-header profile covers only the exact SDK pair `98c795b7` to `086ad82a` at API revision 4. It records all 25 source-header and 33 package-header rows, and permits changes in exactly `DxUi.h`, `RichTextLayout.h` and `RichTextPresentation.h`. The probe records four named layouts (`ThemePalette`, `ProgressBar`, `RichTextEditorAppearance`, and `LayoutOptions`), checks the legacy button enum/default behavior, links the existing capture API on both libraries, and links the candidate-only methods against the matching candidate library. The baseline has no Markdown presentation default to test; that result is explicitly not applicable. `ProgressBar` remains the same size because existing padding accommodates the added state. These are observations for the isolated probe and do not establish ABI interchangeability or safe mixing of headers and libraries; each consumer must rebuild against its pinned matching pair. The profile also binds both real application compiler contexts and their captured link command evidence. It permits no differences beyond the listed header rows and the two exact SDK include operands. The sealed profile bytes and independent auditor must be regenerated/reviewed whenever the exact pair or evidence changes.

The complete V4 dependency audit compares JSON identities by exact, case-sensitive field sets and strict value
types, independent of property order. Missing, additional, renamed, null or differently typed fields fail closed;
only the sealed dependency commit/fingerprint pair may differ. The portable self-test exercises this full
dependency path, including rehashed header/package mutations, rather than validating only the isolated probe.
Earlier V1/V2 profile behavior remains unchanged.

`performance-observed.ps1` records `observer-status.json` before it requests observer shutdown. Its `state` is
therefore a pre-stop snapshot: `Running` does not describe the terminal collection result. Read collection pass,
observer errors, sampling coverage and quiet-window fields separately, retaining all literal observations.
The [retained v2 source probe](../../Measurements/RedesignCompletion_2026-10-26/parsed-syntax-header-probe/README.md)
records its exact SDK pair, isolated application contexts, measured layouts and raw receipt digests. Those
isolated contexts do not identify the actual PR base; each paired study still binds its own production lineage.

### 2026-10-25 local source-bound evidence

The [9025be4 local paired study](../../Measurements/RedesignCompletion_2026-10-25/performance-9025be4/README.md)
uses 576 hash-verified native reports, 72 metric rows, 864 paired block effects and 1,728 same-side drift values.
It keeps original UI, starting redesign and actual PR13-base as separate cohorts, and keeps A/A controls separate
from A/B candidate comparisons. Original-UI A/B retains five resolved regressions: Compose command time +7.4456%,
private bytes +2.1001%; Split command time +9.3709%, owner CPU cycles +12.0103% and working set +2.2311%.
Starting-redesign A/B has four resolved memory improvements and no regressions. Actual PR13-base A/B has no
resolved changes; this explicit dependency-adoption result does not resolve the original-UI regression gate.

The matching [local native matrix](../../Measurements/RedesignCompletion_2026-10-25/qualification-9025be4/README.md)
passes six local builds and three noninteractive x64 Full runs; ARM64 is cross-build only. Three hidden-focus
checks failed in the first Release attempt while an SDK foreground lease overlapped. A later serial Full run with
the same executable and unchanged source/assertions passed all 39 Editor cases. The overlap does not establish
an application activation event or root cause because no per-case event trace was captured. All six observer
windows report complete sample coverage but no verified quiet CPU window. These observations do not establish
causality, performance acceptance, hosted CI, real-focus qualification or native ARM64 execution.
