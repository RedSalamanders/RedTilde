# Observed RedTilde design-refresh v5r3 paired study

cohorts.json is a derived, privacy-filtered account of the v5r3 12-block-per-view AA/AB cohort. AA is the same-binary control; AB compares the original UI with the refreshed UI. They remain separate. Each contains 12 metric rows and 288 control drifts; the projection retains all 576 drifts, all 192 native slots (48 Compose and 48 Split per study), and 48 unique pair receipts with four slot references each. It preserves the measured arrays, medians, source/compiled/binary/pin/profile/compiler/fixture identities and origin-relative integrity metadata. It excludes absolute paths, binaries, raw logs, processor placement, and host/process names.

The independent typed audit passed 7988 checks. The AB metric rows are shown as recorded; flags and investigation bands are preserved without pooling or acceptance:

| View | Metric | Baseline median | Candidate median | Paired change | p | adjusted p | Finding |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| Compose | commandUs | 304.2 | 351.8 | 15.312% | 0.000488281 | 0.005859375 | regressed |
| Compose | ownerCpuCycles | 7634151 | 8142184 | 5.806% | 0.001953125 | 0.009765625 | regressed |
| Compose | completedEditUs | 16574.125 | 16563.6 | -0.004% | 0.983398438 | 1 | no-resolved-change |
| Compose | cleanCompletedUs | 16908.25 | 17023.55 | 0.141% | 0.685546875 | 1 | no-resolved-change |
| Compose | privateBytes | 44883968 | 49682432 | 10.428% | 0.000488281 | 0.005859375 | regressed |
| Compose | workingSetBytes | 54200320 | 56166400 | 3.551% | 0.000488281 | 0.005859375 | regressed |
| Split | commandUs | 360.05 | 390.575 | 9.794% | 0.000488281 | 0.005859375 | regressed |
| Split | ownerCpuCycles | 9704452.75 | 11301005 | 16.637% | 0.000488281 | 0.005859375 | regressed |
| Split | completedEditUs | 16589.7 | 16521.8 | -0.333% | 0.057128906 | 0.228515625 | no-resolved-change |
| Split | cleanCompletedUs | 16922.875 | 16943.875 | -0.13% | 0.462890625 | 1 | no-resolved-change |
| Split | privateBytes | 45305856 | 50378752 | 11.333% | 0.000488281 | 0.005859375 | regressed |
| Split | workingSetBytes | 54544384 | 57016320 | 4.603% | 0.000488281 | 0.005859375 | regressed |

The Compose owner CPU result has adjusted p 0.009765625; the other seven flagged AB results have adjusted p 0.005859375. Completed-edit medians are about 16.6 ms in both views and have no resolved change. This is descriptive evidence only: no causal attribution, policy acceptance, or performance-cost acceptance is claimed.

Host observation covered 378 AA and 378 AB intervals with complete coverage. The shared host was not quiet: AA mean/max system busy was 9.098%/29.187%; AB was 9.331%/36.827%. Maximum coverage gaps were 1.033s and 1.031s, respectively.

Original producer records remain private and unchanged. Relative paths, lengths and SHA-256 references support integrity review, but this derived projection is not a replay bundle; raw execution logs and binaries are not included.
