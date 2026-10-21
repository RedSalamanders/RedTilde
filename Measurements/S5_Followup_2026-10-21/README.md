# S5 user-requested follow-up — 2026-10-21

Status: the requested fixed-scope collection and independent audit pass. Quiet-host and performance acceptance
remain open, together with hardware and upstream qualification. The application remains experimental.

The user reported that CPU activity was quiet and directed starting immediately. One additional A/A then A/B
cohort was therefore started at **11:24:24 CEST** and completed at **11:31:16 CEST** (09:24:24–09:31:16 UTC).
This is a separate, human-requested observation after a reported host-condition change. It does not replace,
pool with or select favorable results from the [earlier attempts](../S5_Noise_2026-10-21/README.md).

## Outcome and limits

All **192 native processes** complete, with 12 independent ABBA/BAAB blocks per Compose/Split view in each
study, 64 timed cycles, two fixed warmups and seed 20261021. The primary twelve-slot family, 5% timing/CPU and
2% memory investigation bands, source, binaries, fixture and judge are unchanged. No run was dropped or added
after inspecting its outcome. [The independent reference](IndependentAudit.json) passes **7,729 checks**.

The [host trace](host-summary.json) has complete coverage, but **does not establish quiet CPU**:

| Study | CPU samples | Mean busy CPU | Peak busy CPU | Maximum sample gap | Selected external CPU |
| --- | ---: | ---: | ---: | ---: | ---: |
| A/A | 199 | 35.323% | 50.805% | 1.047 s | 1,413.359 CPU-seconds |
| A/B | 202 | 30.169% | 100.000% | 1.257 s | 893.859 CPU-seconds |

Compiler, linker, MSBuild and RedPrism CPU deltas are recorded. No process observation is inaccessible.
Selected names are not exhaustive, start/exit work can escape deltas, and GPU activity is unmeasured. Complete
coverage means the sampler covered the windows; it does not mean the host was quiet. The collector ends without
error. [Collection scope/times](collection.json), [observer status](observer-status.json) and the original
[activity intervals](activity.jsonl) are retained.

The judge resolves command-wall improvements of **−8.573% in Compose** (Holm p=0.017578125) and **−5.631% in
Split** (p=0.021484375). Their A/B same-side controls flag **19/24** and **7/24** observations, with maximum
drifts of 94.344% and 14.768%; the separate A/A command controls are also unstable. These numerical findings
do **not** establish accepted speedups or non-regression. Owner-CPU and private-memory controls remain unstable,
and A/B working-set controls flag both views. No metric resolves degradation. A non-significant result is not
evidence of equivalence. The judge remains review-required and cannot accept upstream DxUi D4c.

Complete edited-frame medians remain near 8.32 ms: Compose A/B 8.323175/8.323500 ms, Split
8.321750/8.317150 ms. Completed-edit and clean-frame controls are within their bands in both studies. Their
stability does not close the noisy command/CPU/memory slots or establish physical presentation latency.

[Activity checks](ActivityInvariants.json) pass in every process: 132 changes and recovery submissions;
baseline 132 layouts/mirror resets; candidate zero layouts, zero Compose mirror resets and 132 Split mirror
resets. [Diagnostics](DiagnosticsSummary.json) retain 19,377/24,576 differing processor-boundary samples
(78.845%, all 32 IDs) and cheap snapshot costs of 8.1–185.7 microseconds. Boundary observations do not count all
migrations or establish scheduling as the cause of the control noise. [Per-process operation/chronological
diagnostics](noise.json) preserve every sample without changing the primary analysis.

## Provenance and validation

Measured candidate commit: `2c8f29e5e6ea2213f4311f63678d37b37b0942a3`.
Measured source identity: `dc8e5470237f08d9842d585b11685a260781bc000ed3604d938258b3a21a76ff`.
Frozen baseline commit: `c2ed7ff82568bfe655b9542e60439dcca43a848a` (`ej/redtilde-s5-baseline-v3`).
Exact DxUi pin: `dd619ef0c5855307eb57758072658f2e3b777fd8`, API 4. Both binaries use MSVC 14.52.37025,
v145, SDK 10.0.26100.0 and the identical six common harness inputs. Actual hardware observation is the NVIDIA
GeForce RTX 5080, 2040×1350, 144 DPI, Windows 10.0.26300, 32 logical processors, Balanced power policy.
Affinity, priority and power policy were not changed. The hidden harness never takes desktop focus.

[Study metadata](ab/study.json) records complete compiler/dependency/environment identities and schedules;
[measured identities](validation/MeasuredIdentity.json) verify all **101 prior source inputs** and **687 prior
evidence files** unchanged. Candidate executable SHA-256 is
`F326E6E08307C9F6BC87B6311E1760C98A75A7EFA44BB123A369CAC8471DBA5E`; baseline is
`D314AAE3B854916274D63867899EF41FC3C383A0B744867F44C1C41BA5D0BA7D`.

The [previous six-build/three-profile matrix](../S5_Noise_2026-10-21/validation/matrix.json), 75 application
cases, 39 captures, sanitizer detection and eight full-fixture default/WARP lifecycle results remain their own
exact-source receipts. This follow-up changes documentation/evidence only. The measured Release source and
binary match that prior All receipt; no new full-matrix execution is claimed. A refreshed source-bound Release
receipt and affected validation accompany the documentation closeout, with unchanged executable verification.
[Affected checks](validation/Affected.log) pass validate/format, all 24 observation behaviors and the four
independent numerical oracles. [Retained-byte replay](validation/RetainedAudit.json) again passes 7,729 checks.
The [closeout receipt](validation/Closeout.json) records the four documentation-only changes, fresh Release
source identity and unchanged executable; [the build receipt](validation/CloseoutReleaseBuild.json) binds it.
The final source identity is `9236fca80489a7e66356cdbf182ef0b7c8b05e880df4a3d5d142e89dd01737e7`.
[Git index readback](validation/GitRoundtrip.json) with automatic CRLF conversion enabled preserves all 263
initial packet files and the manifest exactly; [the checkout audit](validation/GitReadbackAudit.json) passes
7,729 checks again. Added closeout metadata is hash-checked after the final index export as well.

Quiet-host evidence, stable command/CPU/memory controls, acceptance of the new judge, longer-term GPU/kernel
lifetime, actual device removal/physical occlusion, native ARM64 execution, physical IME/AT, authorized focus
testing and upstream/release acceptance remain open. S5, S6 and the creation plan stay in WIP. Do not infer a
quiet environment from the collection directory's historical `quiet` label; use the observed classification.

## Reproduction and complete primary results

On the measured source, run the following once with a fresh private destination. Audit all raw slots; assess
actual activity and every control flag separately. Do not rerun until favorable or silently change the bands.

```powershell
.\performance-observed.ps1 -Baseline .build/s5-20261021/baseline-v3-msvc1452 `
    -OutputRoot .build/s5-20261021/observed-v3-quiet-1124 -Blocks 12 -Cycles 64 -Seed 20261021
.\Tests\Scope.Tests.Performance.Evidence.ps1 -StudyPath @(
    '.build/s5-20261021/observed-v3-quiet-1124/aa',
    '.build/s5-20261021/observed-v3-quiet-1124/ab') -Output .build/s5-quiet-followup-independent.json
```

The following [machine-readable summary](PairedSummary.json) preserves all 24 study/metric assessments.
Wall-time units are microseconds; CPU values are cycles; memory values are bytes. A/B medians are descriptive
medians of process medians; the paired change comes from block log-ratios, so it need not equal their quotient.
Flags count the 24 same-side control observations per metric, without dropping any block.

| Study | View | Metric | A median | B median | Paired change | Adjusted p | Flagged controls | Max drift | Finding |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| aa | Compose | commandUs | 218.2 | 218.325 | 1.3998 | % | 1.000000 | 12 | / | 24 | 30.492 | % | no-resolved-change |
| aa | Compose | ownerCpuCycles | 14115851.75 | 13776888.25 | -0.4475 | % | 1.000000 | 13 | / | 24 | 9.207 | % | no-resolved-change |
| aa | Compose | completedEditUs | 8325.525 | 8323.225 | -0.0074 | % | 1.000000 | 0 | / | 24 | 0.445 | % | no-resolved-change |
| aa | Compose | cleanCompletedUs | 8343.775 | 8344.15 | 0.0056 | % | 1.000000 | 0 | / | 24 | 0.618 | % | no-resolved-change |
| aa | Compose | privateBytes | 76079104 | 76566528 | 0.4052 | % | 1.000000 | 7 | / | 24 | 4.371 | % | no-resolved-change |
| aa | Compose | workingSetBytes | 62832640 | 63105024 | 0.1679 | % | 1.000000 | 0 | / | 24 | 1.807 | % | no-resolved-change |
| aa | Split | commandUs | 274.375 | 275.6 | 2.0224 | % | 1.000000 | 9 | / | 24 | 33.351 | % | no-resolved-change |
| aa | Split | ownerCpuCycles | 17883369 | 18085627.25 | 1.1408 | % | 1.000000 | 3 | / | 24 | 16.024 | % | no-resolved-change |
| aa | Split | completedEditUs | 8322.475 | 8323.65 | 0.0061 | % | 1.000000 | 0 | / | 24 | 0.197 | % | no-resolved-change |
| aa | Split | cleanCompletedUs | 8348.85 | 8342.85 | -0.0400 | % | 1.000000 | 0 | / | 24 | 0.466 | % | no-resolved-change |
| aa | Split | privateBytes | 76316672 | 76453888 | 0.0338 | % | 1.000000 | 2 | / | 24 | 2.958 | % | no-resolved-change |
| aa | Split | workingSetBytes | 63137792 | 63059968 | -0.0359 | % | 1.000000 | 0 | / | 24 | 1.851 | % | no-resolved-change |
| ab | Compose | commandUs | 218.775 | 204.8 | -8.5734 | % | 0.017578 | 19 | / | 24 | 94.344 | % | improved |
| ab | Compose | ownerCpuCycles | 13708755.5 | 13715943.25 | 0.1773 | % | 1.000000 | 14 | / | 24 | 28.112 | % | no-resolved-change |
| ab | Compose | completedEditUs | 8323.175 | 8323.5 | -0.0154 | % | 1.000000 | 0 | / | 24 | 0.344 | % | no-resolved-change |
| ab | Compose | cleanCompletedUs | 8351.075 | 8349.125 | 0.0251 | % | 1.000000 | 0 | / | 24 | 0.646 | % | no-resolved-change |
| ab | Compose | privateBytes | 76124160 | 76625920 | 0.4396 | % | 1.000000 | 6 | / | 24 | 2.526 | % | no-resolved-change |
| ab | Compose | workingSetBytes | 62754816 | 62867456 | 0.1774 | % | 1.000000 | 1 | / | 24 | 3.003 | % | no-resolved-change |
| ab | Split | commandUs | 262.55 | 253.525 | -5.6308 | % | 0.021484 | 7 | / | 24 | 14.768 | % | improved |
| ab | Split | ownerCpuCycles | 16998035.25 | 16831828 | -0.4912 | % | 1.000000 | 7 | / | 24 | 8.007 | % | no-resolved-change |
| ab | Split | completedEditUs | 8321.75 | 8317.15 | -0.0344 | % | 1.000000 | 0 | / | 24 | 0.226 | % | no-resolved-change |
| ab | Split | cleanCompletedUs | 8348.95 | 8346.075 | -0.0194 | % | 1.000000 | 0 | / | 24 | 0.617 | % | no-resolved-change |
| ab | Split | privateBytes | 76800000 | 76595200 | -0.2288 | % | 1.000000 | 4 | / | 24 | 3.703 | % | no-resolved-change |
| ab | Split | workingSetBytes | 63356928 | 63340544 | -0.0738 | % | 1.000000 | 2 | / | 24 | 2.452 | % | no-resolved-change |
