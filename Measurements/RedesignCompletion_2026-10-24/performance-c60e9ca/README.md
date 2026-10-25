# RedTilde paired performance closeout

This projection preserves all 72 metric rows across three cohorts and AA/AB. It is an evidence summary, not policy acceptance.

The [full numerical projection](summary.json) and [72-row CSV](metric-rows.csv) retain the published values.
The lead independently rehashes 576 native reports and recomputes all 72 rows, 864 block effects, 1,728 control
drifts, exact sign-flip probabilities, Holm adjustments and classifications. All 1,080 CSV cells match the JSON.
The initial private CSV export contained empty dictionary rows; it is retained separately and is not this packet.
The [local build/runtime packet](../local-qualification-33481d3-c60e9ca/README.md) records its independent gates.

Candidate source identity: `d2a54e46a73202fa4de13438131830ec2480bdada6db965e1554ee9ffd0b4d68`; DxUi pin: `1714f8b0c0fa628bcef56de76078f85c869fc50c`.

Quiet-host qualification: **not established**. Observer and host activity are descriptive. The docs-only c60 profile continuity reuses six byte-identical 334 artifacts; it is not a new c60 native runtime claim.

Lineage keeps each source anchor separate from the measured baseline or overlay. The explicit PR12 dependency adoption is shown as an additional cohort.

## Cohort and control summary

| Cohort | Phase | Control | Improved | Regressed | No resolved change | Stable metric rows | Unstable metric rows | Repeat drifts above band / 288 | Max repeat drift | Observer samples | Mean / max busy CPU | Quiet window |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| original-ui | aa | same binary | 0 | 0 | 12 | 6 / 12 | 6 / 12 | 63 / 288 | 35.864% | 407 | 13.25% / 36.72% | no |
| original-ui | ab | A/B | 0 | 5 | 7 | 6 / 12 | 6 / 12 | 62 / 288 | 33.082% | 405 | 12.9% / 96.5% | no |
| starting-redesign | aa | same binary | 0 | 0 | 12 | 5 / 12 | 7 / 12 | 67 / 288 | 80.053% | 422 | 23.25% / 100% | no |
| starting-redesign | ab | A/B | 4 | 0 | 8 | 6 / 12 | 6 / 12 | 52 / 288 | 21.63% | 405 | 10.48% / 34.04% | no |
| actual-pr12-base | aa | same binary | 0 | 0 | 12 | 6 / 12 | 6 / 12 | 59 / 288 | 49.176% | 407 | 11.99% / 39.37% | no |
| actual-pr12-base | ab | A/B | 0 | 0 | 12 | 6 / 12 | 6 / 12 | 62 / 288 | 40.129% | 410 | 13.95% / 95.2% | no |

Repeat drift values above the investigation band are diagnostic counts, not a policy threshold or acceptance result. Host CPU and observer coverage are descriptive; no quiet-host window was qualified.

## All 72 metric rows

| Cohort | Phase | Scenario | Metric | Baseline median | Candidate median | Change | Holm p | Band | Class | Same binary | Stable controls |
|---|---|---|---|---:|---:|---:|---:|---:|---|---|---|
| original-ui | aa | Compose | commandUs | 240.9 | 250.725 | 3.2749% | 1 | 5% | no-resolved-change | True | False |
| original-ui | aa | Compose | ownerCpuCycles | 8321373.75 | 8304856.25 | -0.0728% | 1 | 5% | no-resolved-change | True | False |
| original-ui | aa | Compose | completedEditUs | 16700.4 | 16709.35 | -0.0592% | 1 | 5% | no-resolved-change | True | True |
| original-ui | aa | Compose | cleanCompletedUs | 16827.275 | 16782.275 | -0.0796% | 1 | 5% | no-resolved-change | True | True |
| original-ui | aa | Compose | privateBytes | 44681216 | 44929024 | 0.5687% | 1 | 2% | no-resolved-change | True | False |
| original-ui | aa | Compose | workingSetBytes | 57159680 | 57407488 | 0.2226% | 1 | 2% | no-resolved-change | True | True |
| original-ui | aa | Split | commandUs | 259.7 | 265.9 | 2.9519% | 1 | 5% | no-resolved-change | True | False |
| original-ui | aa | Split | ownerCpuCycles | 10468122.25 | 10498260.5 | 0.626% | 1 | 5% | no-resolved-change | True | False |
| original-ui | aa | Split | completedEditUs | 16717.125 | 16723.425 | 0.0579% | 1 | 5% | no-resolved-change | True | True |
| original-ui | aa | Split | cleanCompletedUs | 16771.425 | 16800.35 | 0.0599% | 1 | 5% | no-resolved-change | True | True |
| original-ui | aa | Split | privateBytes | 45824000 | 45623296 | -0.05% | 1 | 2% | no-resolved-change | True | False |
| original-ui | aa | Split | workingSetBytes | 57579520 | 57616384 | 0.2676% | 0.9375 | 2% | no-resolved-change | True | True |
| original-ui | ab | Compose | commandUs | 254.55 | 282.725 | 13.1801% | 0.0087890625 | 5% | regressed | False | False |
| original-ui | ab | Compose | ownerCpuCycles | 8521423 | 8832501.5 | 4.8089% | 0.0087890625 | 5% | no-resolved-change | False | False |
| original-ui | ab | Compose | completedEditUs | 16708.9 | 16671.95 | -0.2076% | 0.01708984375 | 5% | no-resolved-change | False | True |
| original-ui | ab | Compose | cleanCompletedUs | 16817.475 | 16814.125 | -0.0218% | 1 | 5% | no-resolved-change | False | True |
| original-ui | ab | Compose | privateBytes | 44761088 | 45893632 | 1.944% | 0.017578125 | 2% | no-resolved-change | False | False |
| original-ui | ab | Compose | workingSetBytes | 57274368 | 58304512 | 1.8204% | 0.005859375 | 2% | no-resolved-change | False | True |
| original-ui | ab | Split | commandUs | 258.225 | 294.25 | 13.0683% | 0.0087890625 | 5% | regressed | False | False |
| original-ui | ab | Split | ownerCpuCycles | 10708540.5 | 11711928.25 | 11.2411% | 0.005859375 | 5% | regressed | False | False |
| original-ui | ab | Split | completedEditUs | 16734.025 | 16742.475 | 0.1344% | 0.20068359375 | 5% | no-resolved-change | False | True |
| original-ui | ab | Split | cleanCompletedUs | 16794.125 | 16788.75 | -0.0378% | 1 | 5% | no-resolved-change | False | True |
| original-ui | ab | Split | privateBytes | 45502464 | 46411776 | 2.2564% | 0.0087890625 | 2% | regressed | False | False |
| original-ui | ab | Split | workingSetBytes | 57636864 | 58828800 | 2.1758% | 0.005859375 | 2% | regressed | False | True |
| starting-redesign | aa | Compose | commandUs | 265.075 | 271 | 1.9254% | 1 | 5% | no-resolved-change | True | False |
| starting-redesign | aa | Compose | ownerCpuCycles | 8301313 | 8355157.75 | 0.9939% | 1 | 5% | no-resolved-change | True | False |
| starting-redesign | aa | Compose | completedEditUs | 16724.225 | 16683.9 | -0.0774% | 1 | 5% | no-resolved-change | True | True |
| starting-redesign | aa | Compose | cleanCompletedUs | 16719.6 | 16753 | -0.4473% | 1 | 5% | no-resolved-change | True | False |
| starting-redesign | aa | Compose | privateBytes | 50212864 | 50266112 | 0.2318% | 1 | 2% | no-resolved-change | True | False |
| starting-redesign | aa | Compose | workingSetBytes | 59615232 | 59670528 | 0.0209% | 1 | 2% | no-resolved-change | True | True |
| starting-redesign | aa | Split | commandUs | 284.9 | 289.925 | 1.13% | 1 | 5% | no-resolved-change | True | False |
| starting-redesign | aa | Split | ownerCpuCycles | 11812328 | 11736430.25 | -0.0242% | 1 | 5% | no-resolved-change | True | False |
| starting-redesign | aa | Split | completedEditUs | 16765.575 | 16758.9 | -0.0124% | 1 | 5% | no-resolved-change | True | True |
| starting-redesign | aa | Split | cleanCompletedUs | 16771.625 | 16794.3 | 0.0643% | 1 | 5% | no-resolved-change | True | True |
| starting-redesign | aa | Split | privateBytes | 51101696 | 50620416 | -0.7724% | 0.193359375 | 2% | no-resolved-change | True | False |
| starting-redesign | aa | Split | workingSetBytes | 60418048 | 60317696 | -0.1827% | 1 | 2% | no-resolved-change | True | True |
| starting-redesign | ab | Compose | commandUs | 289.725 | 293.425 | 2.3048% | 1 | 5% | no-resolved-change | False | False |
| starting-redesign | ab | Compose | ownerCpuCycles | 8976959.75 | 9032898.25 | -0.2852% | 1 | 5% | no-resolved-change | False | False |
| starting-redesign | ab | Compose | completedEditUs | 16682 | 16660.3 | -0.0225% | 1 | 5% | no-resolved-change | False | True |
| starting-redesign | ab | Compose | cleanCompletedUs | 16877.4 | 16806.05 | -0.2023% | 1 | 5% | no-resolved-change | False | True |
| starting-redesign | ab | Compose | privateBytes | 50208768 | 46180352 | -8.299% | 0.005859375 | 2% | improved | False | False |
| starting-redesign | ab | Compose | workingSetBytes | 59750400 | 58345472 | -2.3469% | 0.005859375 | 2% | improved | False | True |
| starting-redesign | ab | Split | commandUs | 287.55 | 287.125 | 1.0665% | 1 | 5% | no-resolved-change | False | False |
| starting-redesign | ab | Split | ownerCpuCycles | 12168414.5 | 12105489.25 | -1.4286% | 0.9375 | 5% | no-resolved-change | False | False |
| starting-redesign | ab | Split | completedEditUs | 16735.75 | 16741.725 | 0.0143% | 1 | 5% | no-resolved-change | False | True |
| starting-redesign | ab | Split | cleanCompletedUs | 16805.675 | 16793.05 | 0.0543% | 1 | 5% | no-resolved-change | False | True |
| starting-redesign | ab | Split | privateBytes | 50919424 | 46520320 | -9.043% | 0.005859375 | 2% | improved | False | False |
| starting-redesign | ab | Split | workingSetBytes | 60393472 | 58841088 | -2.458% | 0.005859375 | 2% | improved | False | True |
| actual-pr12-base | aa | Compose | commandUs | 266.9 | 284.95 | 2.1886% | 1 | 5% | no-resolved-change | True | False |
| actual-pr12-base | aa | Compose | ownerCpuCycles | 8868926.5 | 8946969.25 | 0.4437% | 1 | 5% | no-resolved-change | True | False |
| actual-pr12-base | aa | Compose | completedEditUs | 16689.325 | 16696.05 | -0.0125% | 1 | 5% | no-resolved-change | True | True |
| actual-pr12-base | aa | Compose | cleanCompletedUs | 16801.625 | 16793.95 | -0.0798% | 1 | 5% | no-resolved-change | True | True |
| actual-pr12-base | aa | Compose | privateBytes | 46237696 | 46051328 | -0.159% | 1 | 2% | no-resolved-change | True | False |
| actual-pr12-base | aa | Compose | workingSetBytes | 58204160 | 58234880 | -0.0137% | 1 | 2% | no-resolved-change | True | True |
| actual-pr12-base | aa | Split | commandUs | 293.125 | 291.725 | -0.1847% | 1 | 5% | no-resolved-change | True | False |
| actual-pr12-base | aa | Split | ownerCpuCycles | 12065021.5 | 11940371.25 | -0.2016% | 1 | 5% | no-resolved-change | True | False |
| actual-pr12-base | aa | Split | completedEditUs | 16721.925 | 16723 | 0.0138% | 1 | 5% | no-resolved-change | True | True |
| actual-pr12-base | aa | Split | cleanCompletedUs | 16799.675 | 16790.65 | -0.0763% | 1 | 5% | no-resolved-change | True | True |
| actual-pr12-base | aa | Split | privateBytes | 46075904 | 46327808 | 0.2001% | 1 | 2% | no-resolved-change | True | False |
| actual-pr12-base | aa | Split | workingSetBytes | 58853376 | 58812416 | 0.0172% | 1 | 2% | no-resolved-change | True | True |
| actual-pr12-base | ab | Compose | commandUs | 266.95 | 274.375 | 1.65% | 1 | 5% | no-resolved-change | False | False |
| actual-pr12-base | ab | Compose | ownerCpuCycles | 8651564.5 | 8604657.25 | -1.0273% | 1 | 5% | no-resolved-change | False | False |
| actual-pr12-base | ab | Compose | completedEditUs | 16695.7 | 16692.6 | -0.0404% | 1 | 5% | no-resolved-change | False | True |
| actual-pr12-base | ab | Compose | cleanCompletedUs | 16760.675 | 16789.6 | 0.2315% | 0.404296875 | 5% | no-resolved-change | False | True |
| actual-pr12-base | ab | Compose | privateBytes | 46108672 | 46006272 | -0.0203% | 1 | 2% | no-resolved-change | False | False |
| actual-pr12-base | ab | Compose | workingSetBytes | 58312704 | 58306560 | -0.0314% | 1 | 2% | no-resolved-change | False | True |
| actual-pr12-base | ab | Split | commandUs | 289.95 | 294.275 | 2.046% | 1 | 5% | no-resolved-change | False | False |
| actual-pr12-base | ab | Split | ownerCpuCycles | 11974951 | 12022007.5 | 2.0866% | 1 | 5% | no-resolved-change | False | False |
| actual-pr12-base | ab | Split | completedEditUs | 16754.1 | 16765.925 | 0.0181% | 1 | 5% | no-resolved-change | False | True |
| actual-pr12-base | ab | Split | cleanCompletedUs | 16774.125 | 16795.625 | 0.0385% | 1 | 5% | no-resolved-change | False | True |
| actual-pr12-base | ab | Split | privateBytes | 46174208 | 46516224 | 0.6242% | 1 | 2% | no-resolved-change | False | False |
| actual-pr12-base | ab | Split | workingSetBytes | 58861568 | 58851328 | -0.0129% | 1 | 2% | no-resolved-change | False | True |

All studies contain 96 hash-bound native slots; each metric preserves 24 same-side drift observations, and the private audit contains 144 block-pair records with 864 replayed effects. Refer to JSON for per-row drift values and evidence digests. No absolute local paths, host process names, or raw receipt filenames are included in this public summary.

## Historical Original UI flags

| Cohort | Phase | Scenario | Metric | Prior change | Prior classification |
|---|---|---|---|---:|---|
| original-ui | ab | Compose | commandUs | 12.3639% | regressed |
| original-ui | ab | Split | commandUs | 18.7469% | regressed |
| original-ui | ab | Split | ownerCpuCycles | 13.7564% | regressed |
| original-ui | ab | Split | privateBytes | 2.1428% | regressed |
| original-ui | ab | Split | workingSetBytes | 2.1449% | regressed |
