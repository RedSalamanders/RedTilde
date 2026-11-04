# Redesign completion performance — 2026-10-24

**Status: review-required.** This is a reviewed aggregate projection, not a performance acceptance, rebaseline, or quiet-host qualification. The two current cohorts use the same SDK and candidate binary but answer different baseline questions; their results are kept separate.

The recorded investigation bands are 5% for timing/CPU and 2% for memory. They are review triggers, not permitted degradation limits. For each independent block i, the paired log-ratio is d_i = 0.5 × (ln(B1) + ln(B2) − ln(A1) − ln(A2)); the reported paired estimate is exp(mean(d_i)) − 1. Exact sign-flip inference uses the 12 independent block values, followed by Holm adjustment over the 12 predeclared slots. The displayed baseline/candidate medians are descriptive and are not divided to form that paired estimate. Each cohort used 12 independent ABBA/BAAB blocks per scenario, four serial processes per block (A1, A2, B1, B2), 64 measured cycles per process, and two fixed warmup cycles per process. Both collections used seed 20261024. The same-binary A/A phase is shown separately from the A/B comparison; the Holm family has 12 slots across Compose and Split.

## Binding

- SDK revision: `94bf9801f51a544422ecb8aa6ae86d2e33aadcb2`
- Actual PR base: `f819395d36c55b93e3bf9559d12d0ec401e42b3d`; measured instrumented original-lineage baseline: `b5e15cc0a213fb7f2465ebf3b22a609c4af1d19b`.
- Starting-redesign baseline: `3128855e94a582ab21a5da72163cb8d641ae4f26`.
- Candidate: commit `fa0757d568652560ea05563d09ab3bb6f46298b8`, source identity `13a224b5999c66113b7c452c8b0090472c8e33b0cd25c3b5f57f717ebc778e62`, binary SHA-256 `2C2049D71C0FE9CAF55F6B1273B12E114DA12E804744830D2BE419933202AEC6`.
- Baseline binary SHA-256 values: actual-base lineage `F7C62F185E1F6C6DBC96581FDBBEEAEBEA891BBF818941A02906F2CDD889BA4B`; starting redesign `85BF0188D6ED0A8370D649615F4AF0DA9C8023398EBF54DD382660C010AC04B0`.

## Actual PR-base-lineage cohort

The actual PR base is `f819395d36c55b93e3bf9559d12d0ec401e42b3d`. The measured original arm is the instrumentation-preserving descendant `b5e15cc0a213fb7f2465ebf3b22a609c4af1d19b`; it is not described as a raw build of the PR base. Numerical audit: 7,988 checks; both A/A and A/B comparison artifacts matched independent references; each phase covered 96 native slots across 12 metrics. The audit is numerical/integrity evidence only. Audit artifact SHA-256: `89678DC808332DFFF60D88D17E25A54A41AD70F4E1699614556624FCF02575E2`.

### A/A same-binary control

| Scenario | Metric | Baseline median | Candidate median | Paired change | Holm p | Finding | Control stable | Max repeat drift |
|---|---|---:|---:|---:|---:|---|:---:|---:|
| Compose | commandUs | 243.175 microseconds | 237.725 microseconds | -0.419% (down) | 1 | no-resolved-change | False | 24.897% |
| Compose | ownerCpuCycles | 7268383.25 cycles | 7145281.25 cycles | -0.535% (down) | 1 | no-resolved-change | False | 11.409% |
| Compose | completedEditUs | 16726.5 microseconds | 16722.325 microseconds | -0.061% (down) | 1 | no-resolved-change | True | 0.292% |
| Compose | cleanCompletedUs | 16681.75 microseconds | 16679.8 microseconds | 0.068% (up) | 1 | no-resolved-change | True | 1.121% |
| Compose | privateBytes | 45041664 bytes | 45182976 bytes | 0.229% (up) | 1 | no-resolved-change | False | 5.864% |
| Compose | workingSetBytes | 54292480 bytes | 54513664 bytes | 0.338% (up) | 0.33984375 | no-resolved-change | True | 1.577% |
| Split | commandUs | 252.75 microseconds | 255.825 microseconds | 0.217% (up) | 1 | no-resolved-change | False | 18.476% |
| Split | ownerCpuCycles | 9537189 cycles | 9620122.25 cycles | 0.876% (up) | 1 | no-resolved-change | False | 9.429% |
| Split | completedEditUs | 16724.05 microseconds | 16722.925 microseconds | -0.035% (down) | 1 | no-resolved-change | True | 0.543% |
| Split | cleanCompletedUs | 16701.95 microseconds | 16692.4 microseconds | -0.053% (down) | 1 | no-resolved-change | True | 0.528% |
| Split | privateBytes | 45236224 bytes | 45582336 bytes | 0.61% (up) | 1 | no-resolved-change | False | 3.586% |
| Split | workingSetBytes | 54722560 bytes | 54757376 bytes | 0.152% (up) | 1 | no-resolved-change | True | 1.141% |

Comparison SHA-256: `040A3A14B532E60D29E12F39FB2F3425E5B4C44D28AEE69B0E6B0E7F1FC521E4`. Same-binary control: `True`. Disposition: `review-required`.

### A/B actual-base-lineage versus candidate

| Scenario | Metric | Baseline median | Candidate median | Paired change | Holm p | Finding | Control stable | Max repeat drift |
|---|---|---:|---:|---:|---:|---|:---:|---:|
| Compose | commandUs | 240.6 microseconds | 282.375 microseconds | 16.77% (up) | 0.00585938 | regressed | False | 18.751% |
| Compose | ownerCpuCycles | 7368435.25 cycles | 7486946 cycles | 1.174% (up) | 0.12939453 | no-resolved-change | False | 9.775% |
| Compose | completedEditUs | 16728.775 microseconds | 16719.25 microseconds | -0.073% (down) | 0.54638672 | no-resolved-change | True | 0.247% |
| Compose | cleanCompletedUs | 16674.725 microseconds | 16681.2 microseconds | 0.053% (up) | 0.88476562 | no-resolved-change | True | 0.555% |
| Compose | privateBytes | 44957696 bytes | 50513920 bytes | 12.228% (up) | 0.00585938 | regressed | False | 5.152% |
| Compose | workingSetBytes | 54382592 bytes | 56827904 bytes | 4.628% (up) | 0.00585938 | regressed | True | 1.672% |
| Split | commandUs | 265.875 microseconds | 327.175 microseconds | 20.482% (up) | 0.00585938 | regressed | False | 14.161% |
| Split | ownerCpuCycles | 9607195 cycles | 10737980.25 cycles | 11.625% (up) | 0.00585938 | regressed | False | 7.541% |
| Split | completedEditUs | 16728 microseconds | 16745.525 microseconds | 0.101% (up) | 0.33203125 | no-resolved-change | True | 0.597% |
| Split | cleanCompletedUs | 16689.275 microseconds | 16674.275 microseconds | -0.025% (down) | 0.88476562 | no-resolved-change | True | 0.555% |
| Split | privateBytes | 45271040 bytes | 51163136 bytes | 12.683% (up) | 0.00585938 | regressed | False | 3.966% |
| Split | workingSetBytes | 54777856 bytes | 57296896 bytes | 4.611% (up) | 0.00585938 | regressed | True | 1.878% |

Comparison SHA-256: `172388F06C01A5959E3EF604D4F2C23DCA8A13CBD51E3F4004DC6EDB9D33BCB4`. Same-binary control: `False`. Disposition: `review-required`.

The A/B phase classified seven metrics as regressed: Compose command time, private bytes, and working set; Split command time, owner CPU cycles, private bytes, and working set. Five have unstable within-block repeats, while the two working-set findings have stable controls. This does not identify a cause.

## Starting-redesign cohort

The measured starting arm is `3128855e94a582ab21a5da72163cb8d641ae4f26`. Numerical audit: 8,015 checks; both A/A and A/B comparison artifacts matched independent references; each phase covered 96 native slots across 12 metrics. Audit artifact SHA-256: `4F7F25272EBDB22C4C7EF14E48E380C2959A9F9BC9F1495AC47FE5A08C9C072D`. The A/B phase has no resolved change in any of 12 metrics; six metrics have unstable within-block repeat drift.

### A/A same-binary control

| Scenario | Metric | Baseline median | Candidate median | Paired change | Holm p | Finding | Control stable | Max repeat drift |
|---|---|---:|---:|---:|---:|---|:---:|---:|
| Compose | commandUs | 294.7 microseconds | 296.425 microseconds | 0.578% (up) | 1 | no-resolved-change | False | 17.46% |
| Compose | ownerCpuCycles | 7459851 cycles | 7579860.25 cycles | 0.835% (up) | 1 | no-resolved-change | False | 7.33% |
| Compose | completedEditUs | 16716.65 microseconds | 16708.425 microseconds | -0.047% (down) | 1 | no-resolved-change | True | 0.351% |
| Compose | cleanCompletedUs | 16684.1 microseconds | 16696.775 microseconds | 0.055% (up) | 1 | no-resolved-change | True | 0.487% |
| Compose | privateBytes | 49858560 bytes | 50614272 bytes | 0.395% (up) | 1 | no-resolved-change | False | 4.036% |
| Compose | workingSetBytes | 56770560 bytes | 56674304 bytes | -0.191% (down) | 0.95605469 | no-resolved-change | True | 1.283% |
| Split | commandUs | 315.7 microseconds | 318.45 microseconds | 0.089% (up) | 1 | no-resolved-change | False | 12.139% |
| Split | ownerCpuCycles | 10852251.25 cycles | 10686684.75 cycles | -1.437% (down) | 0.19335938 | no-resolved-change | False | 7.143% |
| Split | completedEditUs | 16736.45 microseconds | 16745.3 microseconds | 0.04% (up) | 1 | no-resolved-change | True | 0.428% |
| Split | cleanCompletedUs | 16703.35 microseconds | 16686.775 microseconds | -0.006% (down) | 1 | no-resolved-change | True | 0.976% |
| Split | privateBytes | 50612224 bytes | 51058688 bytes | 0.461% (up) | 1 | no-resolved-change | False | 3.76% |
| Split | workingSetBytes | 57303040 bytes | 57311232 bytes | 0.068% (up) | 1 | no-resolved-change | True | 1.448% |

Comparison SHA-256: `1E3C8AF18296C64F7894C5381D51BD2ED1D1E144CDEB3B41F3A82A66EB6C50E3`. Same-binary control: `True`. Disposition: `review-required`.

### A/B starting redesign versus candidate

| Scenario | Metric | Baseline median | Candidate median | Paired change | Holm p | Finding | Control stable | Max repeat drift |
|---|---|---:|---:|---:|---:|---|:---:|---:|
| Compose | commandUs | 281.3 microseconds | 282.4 microseconds | -0.525% (down) | 1 | no-resolved-change | False | 8.727% |
| Compose | ownerCpuCycles | 7643780.75 cycles | 7511847.75 cycles | -1.747% (down) | 0.28710938 | no-resolved-change | False | 14.667% |
| Compose | completedEditUs | 16711.075 microseconds | 16719.25 microseconds | 0.008% (up) | 1 | no-resolved-change | True | 0.365% |
| Compose | cleanCompletedUs | 16696.075 microseconds | 16681.25 microseconds | -0.038% (down) | 1 | no-resolved-change | True | 0.634% |
| Compose | privateBytes | 50081792 bytes | 50538496 bytes | 0.616% (up) | 1 | no-resolved-change | False | 3.601% |
| Compose | workingSetBytes | 56686592 bytes | 56791040 bytes | 0.104% (up) | 1 | no-resolved-change | True | 1.485% |
| Split | commandUs | 311.875 microseconds | 318.525 microseconds | 0.901% (up) | 1 | no-resolved-change | False | 12.052% |
| Split | ownerCpuCycles | 10737482.5 cycles | 10817271.75 cycles | 0.073% (up) | 1 | no-resolved-change | False | 11.9% |
| Split | completedEditUs | 16736.3 microseconds | 16745.95 microseconds | -0.01% (down) | 1 | no-resolved-change | True | 0.563% |
| Split | cleanCompletedUs | 16684.75 microseconds | 16686.975 microseconds | -0.02% (down) | 1 | no-resolved-change | True | 0.733% |
| Split | privateBytes | 50739200 bytes | 51083264 bytes | 0.486% (up) | 1 | no-resolved-change | False | 3.511% |
| Split | workingSetBytes | 57280512 bytes | 57374720 bytes | 0.137% (up) | 1 | no-resolved-change | True | 1.87% |

Comparison SHA-256: `06D86225A7A665410DBC266F4A2C740326720A19795343F3A75EB7B5633F27EA`. Same-binary control: `False`. Disposition: `review-required`.

Across all 12 starting-redesign A/B point estimates, the range is −1.747% (Compose owner CPU cycles) to +0.901% (Split command time); no metric is classified as improved or regressed after Holm adjustment.

## Host observation and exclusions

| Cohort | Phase | Samples | Mean system busy | Peak system busy | Largest sampling gap | Complete coverage | Quiet CPU window verified |
|---|---|---:|---:|---:|---:|:---:|:---:|
| actual-pr-base-lineage | aa | 393 | 15.32% | 31.81% | 1.056s / 2.5s | True | False |
| actual-pr-base-lineage | ab | 394 | 15.47% | 34.16% | 1.053s / 2.5s | True | False |
| starting-redesign | aa | 395 | 15.44% | 39.13% | 1.064s / 2.5s | True | False |
| starting-redesign | ab | 393 | 15.46% | 38.15% | 1.05s / 2.5s | True | False |

Sampling coverage met the recorded interval-gap limit, but no quiet CPU window was verified. These observations do not establish an exclusive lease, quiet host, or physical/GPU isolation. Process observations are not exhaustive.

A separate exploratory memory diagnostic is excluded from the primary decision: it reported about 0.2 MiB additional candidate private bytes before first frame, and after rendering one 4,198,400-byte region in the original versus two in the candidate in each of three trials. No allocation stack or ownership attribution was captured, so this does not establish cause.

Projection exclusions: raw study/slot/native receipts, paired-block samples and schedules, logs and observer event streams; executables/symbols/compiler traces/toolchain and dependency manifests/private source manifests; user/worktree/machine/window/process identifiers; allocation addresses and raw memory traces; prior functional qualification receipts/screenshots. The earlier archived cohort remains unchanged and is not pooled with these current runs.

This proposal is an aggregate projection containing all 48 rows (12 metrics in each of two phases for each of two cohorts); it publishes directions and aggregate statistics, not raw per-process or per-block samples. Original private evidence remains authoritative. See [performance-proposal.json](performance-proposal.json) for the structured aggregate projection.




The [execution checklist](../../../Specs/Plans/Done/RedesignCompletionAndRendering_2026-10-24.md) owns current qualification. Later diagnostics and fixes do not rewrite this completed cohort.
