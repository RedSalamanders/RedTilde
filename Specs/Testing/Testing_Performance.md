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

Opt-in `--control-diagnostics` / `-ControlDiagnostics` records bounded synchronous edit subphases in native-v2
receipts. `app-edit-phases-v2` separates native entry, document analysis, mirror, identity labels, native caption,
remaining chrome, selection, recovery and native exit. The initial seven-phase v1 prototype remains readable.
An explicit current diagnostic study requires v2; reject a downgrade even if its edited receipt hashes match.
Phase wall/CPU arrays must cover every timed transaction in order, stay within command cost, and reconstruct
full owner CPU with the preparation/GPU-wait diagnostics. Retain render and preparation counts. Arm the borrowed
observer only for a synchronous harness transaction, clear it on every exit, and perform no diagnostic clock
reads during ordinary editing. The optional instrumentation changes observed cost: freeze and measure a matching
common baseline before a production repair; never compare it directly with uninstrumented acceptance results.

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
