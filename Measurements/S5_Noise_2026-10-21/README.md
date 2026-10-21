# S5 noise follow-through — 2026-10-21

Status: native-v2 investigation, collection and local validation complete; final source-bound matrix is recorded below.
The application stays experimental. Quiet-host qualification, hardware, upstream D4c and acceptance of the
app's new judge remain separate open gates.

## Outcome

The replacement completes all 192 processes with continuous observer coverage: 211/213 samples, maximum gaps
1.054/1.176 seconds. It is **not a quiet-host study**. System CPU averages 36.37% for A/A and 62.55% for A/B,
peaking at 94.13%/100%, with compiler, linker, MSBuild, CMake and Ninja activity recorded. This follows the user's
direction to proceed directly on the shared computer. No additional timing round was selected to obtain passing
controls. The fixed primary family and investigation bands remain unchanged.

Replacement A/B resolves no primary directional change. Compose complete-frame medians are 8.3227/8.3234 ms;
Split medians are 8.3212/8.3212 ms. Several command/CPU/private-memory controls, Compose working set and one
Split clean-frame control remain unstable. Absence of a resolved difference is not equivalence or acceptance.
The first attempt reports a Compose command improvement (−7.85%, adjusted p=0.005859), with unstable controls
and incomplete activity coverage; it is retained without a causal or acceptance claim. Attempts are not pooled.

[The independent audit](s5-noise-independent-all.json) passes **15,454 checks** across all four studies and
384 processes. [Activity invariants](ActivityInvariants.json) hold in every process: 132 changes, baseline
132 layouts/mirror resets, candidate zero layouts, zero Compose mirror resets and 132 Split mirror resets.
[Three integrity-consistent malformed receipts](AdversarialNativeV2.json) are rejected after updating their hashes:
unsettled memory, non-integer processor IDs and missing command CPU samples. Historical native-v1 bytes remain intact.

[Diagnostics](DiagnosticsSummary.json) observe different start/end processors in 17,952/24,576 first-attempt
samples (73.05%) and 19,156/24,576 replacement samples (77.95%), across all 32 logical processor IDs. This proves
observed boundary movement, not its cause or all intervening migrations. Command CPU medians and chronological/
operation summaries remain diagnostics. They do not establish that migration alone caused the control noise.

Cheap latency snapshot wall costs range from 6.8–930.2 microseconds in the first attempt and 7.6–171.1 microseconds
in the replacement. The removed global enumeration cost is an instrumentation change, not an application speedup.
Memory observations are settled but private-byte variation still exceeds its band, so asynchronous settlement and
enumeration were not sufficient explanations for every flag.

All eight refreshed [full-fixture resource runs](resources/receipt.json) pass default/WARP × Compose/Split ×
baseline/candidate checks, including three simulated replacements with exact pixels and zero attached hosts
after close. Full thread inventory remains confined to these resource phases. These short shared-host runs do
not qualify actual device removal, physical occlusion or longer-term GPU/kernel lifetime.

## Investigation and fixed collection scope

The previous [native-v1 packet](../S5_2026-10-21/Final/README.md) retains every noisy control and competing-activity
observation. Its 96 A/A processes all end with 111 threads. The median ratio of the first four versus last sixteen
command samples is 0.9843, and the corresponding complete owner-cycle ratio is 0.9889. Some individual processes
have early spikes, but there is no consistent startup direction; neither a thread-count difference nor universally
insufficient warmup is established. No samples or old rounds are discarded.

The harness performs global thread enumeration around latency collection, often consuming 100–170 ms of process
CPU and perturbing the observation environment. Recovery readiness also differs from worker settlement: ordinary
undo requests immediate cleanup while the worker can still finish asynchronously. These are instrumentation
confounders to isolate, not proven causes of every old flag.

Native-v2 keeps the original two warmup cycles and primary twelve-slot family. It removes global thread inventory
from latency snapshots, reports its unavailable count as null, and waits for successful recovery settlement outside
the timed transactions before observing memory. Resource phases retain full thread inventory and its cost.
Command-only CPU and boundary processor arrays expose additional diagnostics without changing the judge. Normal
recovery remains active during edits. Production callbacks, power policy, process/thread priorities and affinity
are unchanged.

One new frozen baseline retains pre-optimization app callbacks and the identical revised instrumentation/pin.
The prescribed collection is **one A/A then one A/B**, each 12 blocks per Compose/Split view, 64 timed cycles and
two fixed warmups, seed 20261021, with literal schedules and every raw slot retained. Increasing cycles improves
within-process observation support; it is not independent replication beyond the twelve blocks. No repeat is
selected to obtain favorable controls, and no change is claimed relative to the different native-v1 protocol.

The user directed proceeding directly on the shared computer rather than coordinating a quiet lease. Total CPU,
selected process deltas and observation limitations accompany both studies. Actual activity, control stability,
quiet CPU observations and policy acceptance are distinct outcomes. A strong machine does not establish quiet.

## Validation and remaining work

The revised Release Core preflight passes all 25 cases, including the updated native-protocol assertions.
The observation-tool tests cover operation/chronological medians, boundary changes including processor zero,
legacy parsing, malformed/unsettled v2 data, activity on a lightly loaded powerful host, excessive or missing CPU
observations and partial multi-group coverage. The independent auditor still replays all 6,577 checks in the old
packet. An early empty-array property-access defect was corrected before native collection; validation now passes.

The matching baseline is branch `ej/redtilde-s5-baseline-v3` at `c2ed7ff`, derived from `1ccbdb7` with the identical
new common instrumentation. Native implementation is committed in `26c8918`; collector/auditor repairs are bound
by each candidate source identity and final validation manifest. [Each study](Attempt2/ab/study.json) records exact
source/compiler/dependency/binary/fixture/environment identities and the literal schedule. The replacement uses
the same native executables as the first attempt; only collector/auditor code and documentation changed.
Exact DxUi stays `dd619ef0c5855307eb57758072658f2e3b777fd8`, MSVC 14.52.37025/v145, SDK 10.0.26100.0.
No library source, power policy, affinity, priority or production callback changed in this continuation.

The first native-v2 attempt completed all 192 processes. Its observer failed on a null-valued expression during
rapid compiler/CMake process churn; the original getter stack was not retained. Read volatile process properties
once, classify missing/inaccessible values as unavailable, and retain structured observer errors in future runs.
Tests reproduce disappearing/second-read-null properties. The audit's added processor loop also overwrote its
block-name variable; using a separate loop key repairs the grouping. All 7,729 independent checks now pass over
the first attempt. UTC/local/JSON coverage tests exposed and fixed a separate date-comparison defect before reuse.
Twenty-four observation checks pass, including complete/missing observer coverage and date representations.

The first attempt remains immutable and is not accepted as a quiet-host study. One replacement uses the same
native binaries, fixture, family, seed and fixed collection scope after collector/auditor repair. The replacement
is justified by missing activity coverage, not by the control flags or the first attempt's command finding.

## Validation and open gates

All six final builds and all 25 cases in each x64 profile pass, with 39 captures, 13 byte-identical baseline/candidate comparisons, real ASan detection, recovery/crash probes and portable extraction (25 cases). Source-bound receipts are retained under `validation/`. The owning
matrix requires all six builds, 25 cases in each x64 profile, real sanitizer detection, captures, crash/restart,
portable extraction, validators/format and replay of both native protocol versions. ARM64 builds establish no
native runtime claim. Documentation/gallery review finds no visual change; actual capture hashes are checked.

Keep S5 acceptance and the whole plan open. A suitable quiet host or a directly authorized exclusive window is
still needed for quiet-host qualification; raw activity here cannot be renamed quiet because the machine is
powerful. Do not infer production non-regression from the fixed completed-frame medians or accept the first
attempt's favorable command result. The new judge's acceptance, upstream D4c, native ARM64 execution, physical
IME/touch/AT/focus, actual device loss/occlusion, GPU lifetime and release handoff remain unverified.

## All primary results

Values are medians of process medians: microseconds for timing, cycles for owner CPU and bytes for memory. Change is the paired geometric effect. Each flag count is out of 24 same-side controls; investigation bands are not degradation allowances.

### Attempt1 / AA

| View / metric | A | B | Change | Holm p | Flags | Max drift | Finding |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Compose / commandUs | 151.82 | 146.47 | -1.97% | 1.000000 | 14/24 | 37.14% | no-resolved-change |
| Compose / ownerCpuCycles | 11,181,305.25 | 11,009,409.00 | -0.53% | 1.000000 | 14/24 | 26.79% | no-resolved-change |
| Compose / completedEditUs | 8,324.97 | 8,325.52 | 0.00% | 1.000000 | 0/24 | 0.18% | no-resolved-change |
| Compose / cleanCompletedUs | 8,340.10 | 8,339.90 | 0.00% | 1.000000 | 0/24 | 0.59% | no-resolved-change |
| Compose / privateBytes | 76,079,104.00 | 76,011,520.00 | -0.18% | 1.000000 | 3/24 | 3.92% | no-resolved-change |
| Compose / workingSetBytes | 62,889,984.00 | 62,806,016.00 | -0.16% | 1.000000 | 0/24 | 1.35% | no-resolved-change |
| Split / commandUs | 185.38 | 187.95 | +0.42% | 1.000000 | 15/24 | 45.16% | no-resolved-change |
| Split / ownerCpuCycles | 14,893,560.00 | 14,658,253.25 | -0.37% | 1.000000 | 10/24 | 22.14% | no-resolved-change |
| Split / completedEditUs | 8,327.83 | 8,326.62 | -0.03% | 1.000000 | 0/24 | 0.22% | no-resolved-change |
| Split / cleanCompletedUs | 8,335.07 | 8,344.27 | +0.11% | 0.076172 | 0/24 | 0.31% | no-resolved-change |
| Split / privateBytes | 76,445,696.00 | 76,300,288.00 | -0.16% | 1.000000 | 2/24 | 2.97% | no-resolved-change |
| Split / workingSetBytes | 63,254,528.00 | 63,172,608.00 | -0.06% | 1.000000 | 0/24 | 1.57% | no-resolved-change |

### Attempt1 / AB

| View / metric | A | B | Change | Holm p | Flags | Max drift | Finding |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Compose / commandUs | 171.22 | 159.93 | -7.85% | 0.005859 | 17/24 | 41.76% | improved |
| Compose / ownerCpuCycles | 12,292,181.00 | 12,324,370.25 | -2.13% | 1.000000 | 12/24 | 41.08% | no-resolved-change |
| Compose / completedEditUs | 8,325.83 | 8,327.02 | 0.00% | 1.000000 | 0/24 | 0.24% | no-resolved-change |
| Compose / cleanCompletedUs | 8,339.35 | 8,336.92 | -0.06% | 0.687500 | 0/24 | 0.40% | no-resolved-change |
| Compose / privateBytes | 76,335,104.00 | 76,390,400.00 | -0.07% | 1.000000 | 3/24 | 3.18% | no-resolved-change |
| Compose / workingSetBytes | 62,896,128.00 | 63,039,488.00 | +0.06% | 1.000000 | 0/24 | 1.67% | no-resolved-change |
| Split / commandUs | 261.52 | 223.55 | -4.66% | 0.708008 | 16/24 | 40.50% | no-resolved-change |
| Split / ownerCpuCycles | 17,761,583.75 | 17,470,480.25 | -0.74% | 1.000000 | 15/24 | 28.71% | no-resolved-change |
| Split / completedEditUs | 8,321.48 | 8,322.20 | 0.00% | 1.000000 | 0/24 | 0.22% | no-resolved-change |
| Split / cleanCompletedUs | 8,344.42 | 8,344.17 | +0.03% | 1.000000 | 0/24 | 0.46% | no-resolved-change |
| Split / privateBytes | 76,351,488.00 | 76,666,880.00 | +0.07% | 1.000000 | 3/24 | 4.13% | no-resolved-change |
| Split / workingSetBytes | 63,229,952.00 | 63,178,752.00 | -0.09% | 1.000000 | 1/24 | 2.43% | no-resolved-change |

### Attempt2 / AA

| View / metric | A | B | Change | Holm p | Flags | Max drift | Finding |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Compose / commandUs | 188.57 | 190.32 | +1.15% | 1.000000 | 6/24 | 21.49% | no-resolved-change |
| Compose / ownerCpuCycles | 13,026,864.00 | 13,168,178.00 | +0.20% | 1.000000 | 4/24 | 14.24% | no-resolved-change |
| Compose / completedEditUs | 8,324.55 | 8,322.73 | -0.01% | 1.000000 | 0/24 | 0.23% | no-resolved-change |
| Compose / cleanCompletedUs | 8,342.48 | 8,345.40 | -0.01% | 1.000000 | 0/24 | 0.43% | no-resolved-change |
| Compose / privateBytes | 76,183,552.00 | 76,683,264.00 | +0.27% | 1.000000 | 5/24 | 2.79% | no-resolved-change |
| Compose / workingSetBytes | 62,832,640.00 | 62,849,024.00 | -0.03% | 1.000000 | 0/24 | 1.60% | no-resolved-change |
| Split / commandUs | 226.60 | 240.43 | +5.69% | 0.023438 | 15/24 | 40.19% | regressed |
| Split / ownerCpuCycles | 16,649,740.50 | 16,924,859.25 | +1.16% | 1.000000 | 9/24 | 41.82% | no-resolved-change |
| Split / completedEditUs | 8,321.42 | 8,324.80 | +0.04% | 1.000000 | 0/24 | 0.20% | no-resolved-change |
| Split / cleanCompletedUs | 8,338.35 | 8,341.80 | +0.05% | 1.000000 | 0/24 | 0.55% | no-resolved-change |
| Split / privateBytes | 76,769,280.00 | 76,883,968.00 | +0.25% | 1.000000 | 4/24 | 3.30% | no-resolved-change |
| Split / workingSetBytes | 63,244,288.00 | 63,354,880.00 | +0.09% | 1.000000 | 2/24 | 2.21% | no-resolved-change |

### Attempt2 / AB

| View / metric | A | B | Change | Holm p | Flags | Max drift | Finding |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Compose / commandUs | 222.52 | 220.88 | -2.19% | 1.000000 | 16/24 | 74.34% | no-resolved-change |
| Compose / ownerCpuCycles | 14,805,361.00 | 15,651,958.25 | +2.48% | 1.000000 | 13/24 | 35.85% | no-resolved-change |
| Compose / completedEditUs | 8,322.67 | 8,323.42 | +0.02% | 1.000000 | 0/24 | 0.30% | no-resolved-change |
| Compose / cleanCompletedUs | 8,342.48 | 8,335.95 | -0.15% | 0.076172 | 0/24 | 1.04% | no-resolved-change |
| Compose / privateBytes | 75,704,320.00 | 76,001,280.00 | +0.30% | 1.000000 | 5/24 | 3.93% | no-resolved-change |
| Compose / workingSetBytes | 62,660,608.00 | 62,746,624.00 | +0.22% | 1.000000 | 2/24 | 3.32% | no-resolved-change |
| Split / commandUs | 285.62 | 284.90 | +0.96% | 1.000000 | 17/24 | 48.36% | no-resolved-change |
| Split / ownerCpuCycles | 18,989,741.25 | 19,040,169.25 | +2.25% | 1.000000 | 14/24 | 19.31% | no-resolved-change |
| Split / completedEditUs | 8,321.15 | 8,321.23 | -0.01% | 1.000000 | 0/24 | 0.71% | no-resolved-change |
| Split / cleanCompletedUs | 8,342.85 | 8,342.95 | -1.28% | 1.000000 | 1/24 | 35.31% | no-resolved-change |
| Split / privateBytes | 76,056,576.00 | 76,437,504.00 | +0.40% | 1.000000 | 3/24 | 3.12% | no-resolved-change |
| Split / workingSetBytes | 63,027,200.00 | 63,125,504.00 | -0.07% | 1.000000 | 0/24 | 2.00% | no-resolved-change |
