# RedTilde eed2abe9 — complete three-reference performance comparison

This packet records the declared 576-process sequence at application `eed2abe9885c57f44ab192155b341582782020f9`, pinned to DxUi `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`. Collection and both numerical/integrity audits passed. **Performance acceptance remains OPEN.**

The original-UI A/B comparison flags Compose command time **+8.64%** with unstable controls and Split working set **+2.027%** with stable controls. Split owner CPU is **-0.575%**, unresolved with unstable controls; this cohort does not reproduce the older +11.5179% CPU finding. Separate historical cohorts stay unchanged and are not pooled with this one.

| Reference | A/A flags | A/B flags | A/B improvements | Unstable controls, A/A and A/B |
|---|---:|---:|---:|---:|
| Original UI | 0 | 2 | 0 | 6/12 and 6/12 |
| Starting redesign | 0 | 0 | 5 | 6/12 and 6/12 |
| Actual PR18 base `07b232a7` | 0 | 0 | 1 | 6/12 and 6/12 |

The starting-redesign A/B rows show four lower memory measurements and lower Split owner CPU. The actual-base A/B Split owner CPU change is **-9.441%** with unstable controls. These are the frozen judge's findings, not causal attribution or accepted savings. All 72 rows, including every unresolved result, are in `metric-rows.csv` and `summary.json`.

The profile is 12 independent ABBA/BAAB blocks per view, 64 cycles, seed `20261027`, diagnostics **OFF**. Timing/CPU investigation bands remain 5%; memory remains 2%. There were no sequence rerolls, exclusions, cross-cohort pooling or threshold changes. Each A/A runs its reference executable on both sides. Each A/B uses its separately identified SDK4077 reference and the SDK98 candidate; common compiler/header admission is recorded without relabeling different runtime identities.

The canonical typed C# subset-enumeration audit passes **25,120 checks** over six studies. The separate Python raw replay passes **13,322 checks**, rehashes all 576 reports, reconstructs 864 log block effects, 1,728 within-side control drifts and 72 Holm-adjusted rows, and checks 936 CSV cells. The public index contains 576 unique raw report hashes. `root-replay.json` binds the replay script, summary, CSV and raw index; `evidence-index.json` and `packet-hashes.json` bind the published projection.

All six host-observer windows have complete cadence coverage. None establishes a quiet CPU window. Across the six studies, **36/72 control rows are unstable**. Public observer data omit process identities and retain counts and scalar activity only. Control instability does not remove either original-UI flag.

The measured candidate is bound to its complete executed Release build receipt and frozen EXE; reference manifests were declared before the sequence. The candidate freeze-manifest digest is explicitly a **postcollection attestation of a freeze output**, not a predeclared measurement input. The primary candidate receipts independently bind the executed source, pin, artifact and nine common inputs.

Raw native receipts, observer streams, compiler receipts, the declaration and both replay programs are retained under `.build/performance-final-eed2abe9-three-reference` and `.build/audit-20261027/source-ink-adoption-eed2abe9`. Those private payloads include environment details and are not copied into this public projection. The separate diagnostics-enabled command/memory study is outside this acceptance family.

Three x64 Full runs, ARM64 cross-builds, keyboard input and SDK Menu results have separately bound receipts. They do not qualify performance, physical presentation, broader IME/assistive-technology coverage or hosted RedTilde ARM64. The latest hosted RedTilde run starts no native work under the account payment/spending restriction; that ARM64 gate stays OPEN.
