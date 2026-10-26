# Final three-reference performance collection at 0f68857b

This fixed collection retains **576 native processes**, with A/A controls and A/B comparisons kept separate
for original UI `c1579053`, starting redesign `4ce958a8`, and the actual PR #15 base `45493e63`. The collector is
`0f68857b`; its native production source is `fc6c62d`, at exact DxUi `47a9d259`. Documentation receipt refreshes
preserved every qualified native artifact byte. Each study uses 12 blocks per view, 64 cycles, seed 20261026,
the same 693-unit fixture, and diagnostics OFF. The declared sequence was retained without retries or pooling.

| Original-UI A/B finding | Paired change | Holm adjusted p | Same-side controls stable |
| --- | ---: | ---: | --- |
| Compose command time | +9.5339% | 0.005859375 | No |
| Split owner-thread cycles | +12.1749% | 0.005859375 | No |
| Split private bytes | +2.0231% | 0.015625 | No |
| Split working set | +2.3906% | 0.005859375 | Yes |

All four findings remain **OPEN**. Split command time is unresolved in this collection; that is not proof of
a causal gain over another collection. Starting-redesign A/B has four improved memory rows and no regressions;
actual PR #15-base A/B has 12 unresolved rows and no regressions. None of those findings accepts the original-UI
costs. Completed-edit and clean-completion latency have no resolved changes. Do not pool source lineages.

Across all six studies, 39 of 72 metric controls are unstable. All six observer envelopes have complete cadence
coverage, but no quiet CPU window is verified. This limits attribution and acceptance even where a point estimate
is unresolved. Keep the original 5% timing/CPU and 2% memory investigation bands and 12-row Holm families.

The original and starting harness overlays are `6b97e3a0` and `2aec8783`, both on SDK47a9. The actual-base overlay
is `b69bb2a0`: its application production files and SDK2fed pin remain unchanged. All nine common inputs match.
Only its A/B study uses the separately reviewed, fixed two-header extension profile; A/A executes its own frozen
baseline on both sides. The ordinary same-pin/default admission guards are retained. See the supporting
[header-extension packet](../header-extension-47a9/README.md) for the exact pair and probe limits.

[summary.json](summary.json) records all 72 rows, six source lineages, header/input digests and observer envelopes.
[metric-rows.csv](metric-rows.csv) contains the same 936 data cells. The [raw report index](raw-report-index.json)
binds every retained native report by cohort, phase, view, block, slot and SHA-256. The canonical independent
typed auditor passes 25,177 checks across all six studies. A separate lead replay recomputes raw process medians,
864 log block effects, exact sign-flip p-values, Holm decisions and 1,728 control drifts, rehashes all 576 native
reports, and round-trips the CSV; its [receipt](root-replay.json) binds these published bytes. Another read-only
review independently matches all report hashes and projected comparison/CSV cells. These are integrity and
numerical checks, not policy self-approval or a general repository acceptance result.

The raw native reports, native EXE/PDB freezes, common-harness receipts, full observer streams and all prior
attempts remain in the governed local evidence tree. This public projection excludes local paths and desktop
identifiers. [Native qualification](../qualification-fc6c62d/README.md),
[keyboard qualification](../input-0f68857b/README.md), hosted RedTilde ARM64, physical IME/AT and performance
acceptance are separate gates. The parent plan remains in WIP.
