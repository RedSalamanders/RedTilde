# RedTilde P28 — current-policy comparison at 9cf8b221

The declared **576-process** sequence and both independent numerical/integrity reviews completed at app `9cf8b2215fceb38344f02d09430b2ad2870e8110`, pinned to DxUi `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`. **Performance acceptance remains OPEN.**

The original-UI comparison flags three memory rows: Compose working set **+2.0457%**, Split working set **+2.4615%**, and Compose private bytes **+2.0952%**. The two working-set rows have stable controls; the private-byte row has unstable controls and remains flagged. Compose command time is **+2.2192%**, unresolved with unstable controls. The earlier cohort's two flags are historical; no causal optimization gain is inferred by comparing separate cohorts.

| Reference | A/A flags | A/B flags | A/B improvements | Unstable rows in A/A and A/B |
|---|---:|---:|---:|---:|
| Original UI | 0 | 3 | 0 | 6/12 and 6/12 |
| Starting redesign | 0 | 0 | 5 | 6/12 and 6/12 |
| Actual implementation PR18 base | 0 | 0 | 0 | 6/12 and 6/12 |

All 72 rows, including every unresolved and improved row, appear in `summary.json` and `metric-rows.csv`. No metric, slot, flag or failed earlier attempt was removed. The three reference roles preserve their historical anchors separately from their current-policy build overlays. Actual PR18 base `07b232a7` and continuation main `5b545e2` are distinct roles.

The profile remains twelve independent ABBA/BAAB blocks per view, 64 cycles, seed `20261027`, diagnostics **OFF**, a fixed twelve-slot family, 5% timing/CPU and 2% memory investigation bands. These bands are not permitted degradation. Each A/A uses one reference binary on both sides; each A/B uses that SDK4077 reference and the separately bound SDK98 candidate. The current app runtime sources/headers equal the continuation base; this is not an A/B product-optimization effect against that unchanged base. Equal current compilation policy, Main/Monitor support, nine common harness inputs and the complete public-header closure are independently checked.

The canonical typed C# audit passes **25,138 checks**. The root Python replay passes **13,322 checks**, rehashes all 576 reports, reconstructs 864 block effects, 1,728 control observations and 72 Holm-adjusted rows, and round-trips 936 CSV cells. A separate reviewer repeats that work and full admission in **13,940 checks**. The first root replay's stale 25,120 exact-count refusal is preserved: twelve additional translation-unit and six release-policy field checks explain the new count under the unchanged auditor source.

All six observer windows have complete cadence coverage, but none verifies a quiet CPU window; **36/72 controls are unstable**. Public activity data contain scalar counts only. Private raw streams retain observed external activity and unavailable observations. Instability does not erase a finding, and successful collection does not qualify the upstream performance policy.

The freeze-manifest digest was bound before primary collection. The candidate independently matches its complete executed build receipt, binary and common inputs. Rebuilt historical references include the reviewed common compilation policy and support inputs; the original historical binaries remain preserved and are not relabeled as current-policy builds. No compiler-version or resident-memory benefit is inferred from executable size.

The immutable raw studies, declarations, compiler receipts and observer streams remain under `.build/performance-p28-policy-repaired-three-reference`; qualification and independent replay files remain under `.build/audit-20261027/performance-resume`. This public packet omits host paths and process identities and supplies their bound hashes. Diagnostic runs, including the later eight-slot instrumented memory investigation, are excluded from this primary family.

Native qualification is separately bound to producer9cf8b221: all six builds, three x64 Full runs, real ASan detection and authorized keyboard checks. ARM64 cross-builds do not establish runtime execution. Hosted RedTilde ARM64 remains **OPEN** because the latest hosted run starts no tests under the account billing restriction. Physical input, quiet-host and upstream performance-policy holds remain separate.
