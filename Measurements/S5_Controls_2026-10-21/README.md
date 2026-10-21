# S5 remaining-control investigation — 2026-10-21

Status: investigation, synchronous caption repair, local validation and new paired collection complete.
Performance acceptance remains open, together with all hardware, physical input, native ARM64, upstream D4c
and release gates. This packet does not qualify the whole S5 plan or establish non-regression.

## Scope and findings

The previous [follow-up](../S5_Followup_2026-10-21/README.md) remains unchanged. This continuation first retains
its current executable, then runs a fixed sixteen-process hardware/WARP contrast (four per view/backend). It
adds bounded opt-in command phases and repeats that sixteen-process diagnostic with identical instrumentation.
Both renderers vary. Every instrumented transaction renders once; hardware process thread counts are all 111.
Private bytes are unchanged between the primary snapshot and observations after 100/500 ms of hidden idle.
These observations do not support extra render count, different hardware thread counts or short settlement as
universal explanations. WARP thread counts vary (33–37); changing renderer performs different work and is not a
performance rebaseline. The CPU/VirtualAllocation profiler request is denied by host policy (`0xc5585011`);
there is no successful ETW trace or stack-based attribution. Policy is unchanged.

The original combined medians are also compared offline with balanced-operation/cycle estimators across all
four latest retained studies. Numerous controls remain flagged under those alternatives. The original primary
median, twelve-slot family and 5% timing/CPU and 2% memory investigation bands are therefore retained.

Explicit application phases show substantial native-caption cost (hardware diagnostic process medians roughly
32–50 microseconds). Ordinary edits already dirty often request the same title repeatedly. The repair caches
only positively acknowledged native identity text and skips identical writes, retaining synchronous Unicode,
filename and dirty-state updates. A failed or superseded request invalidates that cache. Hidden-window tests
cover typing, burst/undo, unchanged dirty state, Unicode replacement, rejected receiver writes and reentrant
document replacement. The own-window sender checks the explicit
[WM_SETTEXT acknowledgment](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-settext); it does not infer
acceptance from a wrapper return or defer native work outside the timed transaction.

An earlier queued-caption prototype is not adopted: it would introduce a new asynchronous readiness obligation
and could shift caption work outside the existing command boundary. Its source/artifact and passing focused
receipt remain private diagnostic history. The final fix is synchronous. Earlier failed retry receipts remain
retained, and the final failure/supersession regressions must pass before closeout.

## New baseline and declared paired scope

The incremental repair baseline is commit `24ef979b34ce86556a6ea1e1c2ad4dd57fea560d` on
`ej/redtilde-s5-baseline-v4`, frozen before the production repair. It already contains the earlier production
optimizations and identical optional diagnostics; it is not a replacement for the original pre-optimization
baseline. Both sides keep exact DxUi `dd619ef0c5855307eb57758072658f2e3b777fd8` (API 4), MSVC 14.52.37025/v145
and SDK 10.0.26100.0. The common input set additionally binds `RedTilde/EditDiagnostics.h`.

The prescribed smoke is one two-block, four-cycle A/B protocol check. The main collection is one observed A/A
then A/B cohort, each twelve independent ABBA/BAAB blocks per Compose/Split view, 64 timed cycles, two warmups,
seed 20261021 and `-ControlDiagnostics`. Retain every process, control and observer interval without pooling or
repeating to obtain favorable controls. Independently audit all raw slots and malformed diagnostic receipts.

The fixed fixture alternates saved/dirty on every transaction; both sides therefore still need 132 caption
writes per process. It can validate the incremental change, but cannot quantify the saving from eliminating
same-title writes in ordinary dirty typing. That saving is established by deterministic call-count behavior
tests, without claiming a measured typing speedup. Full edited-frame owner CPU includes all synchronous caption
work. Post-primary idle/inventory observations never replace primary memory samples.

## Completed collection and assessment

All **192 main native processes** complete. [Independent numerical/integrity auditing](IndependentAudit.json)
passes **65,139 checks**; [the 16-process smoke](SmokeAudit.json) passes 1,712 checks. Every raw native/pair record
is retained under [A/A](aa/study.json), [A/B](ab/study.json) and [Smoke](Smoke/study.json). The new production
candidate is commit `3de529d7f35ec95de52e1a2b8e66bf746bf01f7e`; its exact measured source and compiler are bound
by [the source manifest](validation/MeasuredSource.json) and study metadata. Later documentation/validator
closeout is checked against that manifest and the same executable bytes, without rewriting historical receipts.

[The observer](host-summary.json) has complete coverage: A/A 256 samples, maximum gap 1.048 seconds, average
CPU 15.660%, peak 41.635%; A/B 255 samples, maximum gap 1.044 seconds, average CPU 13.550%, peak 28.883%.
RedPrism contributes recorded CPU deltas of 288.906/313.063 CPU-seconds. No selected compiler activity appears
in this cohort. This is **not a verified quiet host**; selected names are not exhaustive and GPU activity is
unmeasured. [Raw intervals](activity.jsonl), [collection times](collection.json) and [observer status](observer-status.json)
are retained. Earlier cohorts are not pooled with this one.

The [judge's twelve primary results](PairedSummary.json) all receive `no-resolved-change` under its fixed
band-plus-Holm rule. That is not evidence of equivalence. In particular, Compose completed edited-frame latency
has a **positive 0.065499% paired signal, adjusted p=0.017578125**, despite being below the 5% investigation
band. Descriptive A/B medians are 8.318300/8.322600 ms. Retain that signal for assessment; the band is not an
allowed degradation. Split edited-frame medians are 8.326325/8.321825 ms (paired −0.030952%, adjusted p=0.249023).

Compose command/owner CPU changes are −3.482188%/−2.420733%; Split changes are +0.604834%/+0.705028%.
Their A/B controls flag 12/13 and 12/7 of 24 observations respectively. Compose private/working-set controls flag
2/1 observations; Split private memory flags 5 while working set remains within band. Separate A/A command/CPU
controls also vary. Completed-edit and clean-frame controls stay within band. No primary metric crosses both
the investigation band and adjusted significance threshold, and no performance acceptance is claimed.

[Deterministic activity](ActivityInvariants.json) holds in all 192 processes: 132 changes, recovery submissions,
caption requests and synchronous caption writes; zero chrome layouts; zero Compose mirror resets and 132 Split
mirror resets. Every timed transaction renders once. Every process has 111 late diagnostic threads. Private
bytes and working set are unchanged through 500 ms of hidden idle in every process. Short settlement delays or
different final thread counts therefore do not explain the remaining hardware-cohort memory flags.

[Per-process phases](Diagnostics/PerProcess.json) and [descriptive aggregates](Diagnostics/Aggregates.json)
remain diagnostic. Reconstructed three-frame cycle medians are 24.989125/24.986975 ms in Compose and
24.989200/24.992800 ms in Split; these omit owner work between measured samples and do not replace any primary
metric. Phase medians are not additive. The different signs across paced components, lower Compose owner CPU
and unavailable stack attribution do not establish a cause for the small positive frame signal.

## Correctness, compatibility and remaining gates

[All six builds](validation/BuildMatrix.json) pass. The three recorded x64 `All` profiles pass **81 behavioral
cases** and generate **39 actual harness captures**, with durable preferences, crash/restart recovery, minidump
and real ASan detection checks. [All 13 baseline/candidate captures](validation/CaptureComparison.json) are
byte-identical; the current Release images are retained under `captures/`. ARM64 results are cross-builds only.
[Eight full-fixture lifecycle runs](resources/receipt.json) pass baseline/candidate × Compose/Split × hardware/WARP,
each with three exact replacement pixel comparisons, hidden/clean idle, resize and zero attached hosts after close.

[Five malformed diagnostic receipts](MalformedDiagnostics.json) are rejected after updating their hashes.
[Permanent validator-owned regressions](validation/DiagnosticRegressions.json) reproduce these semantic refusals
and a sixth protocol-downgrade refusal for their expected reasons. A current study cannot omit caption evidence
by changing to the readable legacy prototype. The initially accepted downgrade is retained as a failed validation
attempt, then rejected after repair. [Legacy replay](validation/LegacyReplay.json) still passes 7,729 checks. The rejected
caption retry attempts remain under `validation/RejectedRetry`; they are failures, not qualification receipts.
Temporary diagnostic grouping was corrected before publication; the retained summary asserts all six groups,
positive thread counts and closure over all 192 raw records.

The caption repair eliminates redundant same-title writes in functional typing/burst tests. It is not a proven
typing speedup, and the alternating primary fixture does not omit its native work. Keep the functional repair
reviewable on the feature branch while **deferring performance acceptance**. Resolve the small positive Compose
frame signal with a suitable quiet cohort and calibrated policy before adoption; deferring this repair remains
an option when an already-qualified performance result is required. Do not silently rebaseline or widen bands.
Actual device removal/occlusion, longer-term GPU/kernel lifetime, native ARM64 execution, physical IME/AT,
authorized real-focus testing and upstream/release acceptance stay open.

Disk exhaustion occurred after collection and audit. A verified file-by-file relocation preserved the discarded
prototype on C:. Current executables, frozen baselines and all raw study receipts remain unchanged. Continuation
archives use the C: volume; no independent application or build was terminated.
## Final source and package closeout

[Source coverage](validation/SourceCoverage.json) proves unchanged native inputs, all six executable artifacts
and all seven paired common inputs. [The refreshed builds](validation/CloseoutBuilds.json) bind source identity
`815a0b7d047415fda63a7bb7acd118c960a17f1cb99e7d843c06f10e0570c805`. Only documentation and evidence-validation tools changed after collection. The new evidence
regression test and tightened reader are checked separately; original runtime identities are retained.
[Fresh affected validation](validation/Affected.log) passes all 37 observation checks, the four numerical oracles,
the permanent six semantic-rejection regressions and a fresh [27-case Release Core run](validation/CloseoutReleaseCore.json).
[Environment recheck](validation/EnvironmentRecheck.json) matches measured CPU/OS, power policy and graphics DLL identities.

[Retained raw replay](validation/RetainedAudit.json) repeats all 65,139 checks. [Actual native-v1 replay](validation/NativeV1Replay.json)
passes 6,577 checks in addition to the 7,729-check native-v2 replay. [The extracted portable package](validation/Portable.json)
passes all 27 native cases, with the same Release executable SHA-256
`334A8CBC79DD6F5C6D0BD76CD9FBC62DDEDD715C4FFA4237C07A9208605B4697`.
No unchanged historical runtime receipt is relabeled as fresh execution.

## Complete primary results

All 24 study/metric rows are retained below. Wall-time units are microseconds, CPU values are cycles and memory
values are bytes. Descriptive process medians need not have the quotient shown by paired block log effects.
A/A and A/B each have twelve independent blocks per view, with 24 same-side controls per metric.

| Study | View | Metric | A median | B median | Paired change | Holm p | Flags / 24 | Max drift |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| aa | Compose | commandUs | 168.175 | 167.6 | -2.027037 | % | 1.000000 | 20 | 24.176 | % |
| aa | Compose | ownerCpuCycles | 12001613 | 12221471.25 | -0.525024 | % | 1.000000 | 11 | 15.180 | % |
| aa | Compose | completedEditUs | 8318.25 | 8318.95 | -0.008093 | % | 1.000000 | 0 | 0.193 | % |
| aa | Compose | cleanCompletedUs | 8353.675 | 8351.475 | -0.019173 | % | 1.000000 | 0 | 0.380 | % |
| aa | Compose | privateBytes | 76292096 | 76412928 | 0.540968 | % | 1.000000 | 4 | 3.356 | % |
| aa | Compose | workingSetBytes | 62902272 | 62986240 | 0.303693 | % | 0.457031 | 2 | 3.339 | % |
| aa | Split | commandUs | 226.175 | 231.5 | 2.969601 | % | 1.000000 | 11 | 29.493 | % |
| aa | Split | ownerCpuCycles | 15912911 | 15944350.75 | -0.034454 | % | 1.000000 | 8 | 15.860 | % |
| aa | Split | completedEditUs | 8324.25 | 8320.6 | -0.010001 | % | 1.000000 | 0 | 0.317 | % |
| aa | Split | cleanCompletedUs | 8341.775 | 8341.35 | 0.002903 | % | 1.000000 | 0 | 0.518 | % |
| aa | Split | privateBytes | 76865536 | 76959744 | 0.039180 | % | 1.000000 | 0 | 1.674 | % |
| aa | Split | workingSetBytes | 63287296 | 63137792 | -0.049341 | % | 1.000000 | 0 | 1.467 | % |
| ab | Compose | commandUs | 167.575 | 161.2 | -3.482188 | % | 0.597656 | 12 | 44.145 | % |
| ab | Compose | ownerCpuCycles | 11961590.75 | 11892487.25 | -2.420733 | % | 0.249023 | 13 | 18.145 | % |
| ab | Compose | completedEditUs | 8318.3 | 8322.6 | 0.065499 | % | 0.017578 | 0 | 0.204 | % |
| ab | Compose | cleanCompletedUs | 8352.35 | 8349.425 | -0.072018 | % | 1.000000 | 0 | 0.416 | % |
| ab | Compose | privateBytes | 76400640 | 76552192 | 0.167497 | % | 1.000000 | 2 | 3.119 | % |
| ab | Compose | workingSetBytes | 62892032 | 62871552 | 0.225521 | % | 1.000000 | 1 | 3.807 | % |
| ab | Split | commandUs | 193.5 | 192.375 | 0.604834 | % | 1.000000 | 12 | 40.130 | % |
| ab | Split | ownerCpuCycles | 14385495.5 | 14735937.5 | 0.705028 | % | 1.000000 | 7 | 14.307 | % |
| ab | Split | completedEditUs | 8326.325 | 8321.825 | -0.030952 | % | 0.249023 | 0 | 0.296 | % |
| ab | Split | cleanCompletedUs | 8339.975 | 8348.5 | 0.016322 | % | 1.000000 | 0 | 1.118 | % |
| ab | Split | privateBytes | 76658688 | 76396544 | -0.018589 | % | 1.000000 | 5 | 3.086 | % |
| ab | Split | workingSetBytes | 63059968 | 63313920 | 0.390712 | % | 0.085938 | 0 | 1.779 | % |

The portable receipt retains its original packaging source identity; later validation-only edits leave its executable and packaged assets unchanged.

The workspace volume exhausted its space before final Git staging. [A verified continuation copy](validation/ContinuationCopy.json) preserves all 2,066 working-tree files with the identical final source identity. The final closeout commit is made in that C: copy; runtime/build/paired receipts retain their original Z: provenance, and the original working tree is preserved.

[Git readback](validation/GitReadback.json) preserves all initially staged packet bytes with automatic CRLF conversion enabled. [Its independent replay](validation/GitReadbackAudit.json) passes all 65,139 checks; final metadata is hash-checked again before commit.
