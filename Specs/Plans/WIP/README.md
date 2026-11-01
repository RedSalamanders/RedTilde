# Active plans

Source `45651052e5868312a1908aabd4a93665abab97c5` (31 October 2026). Product code
is unchanged since `146a964`. Read
[the WIP folder quality review](WipFolderQuality_2026-10-31.md) and execute its
sequence. The entries below are one register. They are not eight queues.

## Execute

- [Delivery review](DeliveryReview_2026-10-31.md) — 13 unchecked product defects. Re-read on 31 October; the anchors still match the source. Start with RT-04, and write the failing test first.
- [Delivery red team](DeliveryRedTeam_2026-10-31.md) — Phase 1–6 sequence, the extra packaging and probe findings, and the approved Q3/Q4/Q8 requirements. RT-18 is defensive hardening without a demonstrated failure. Remediation has not started.

## Owner decisions and current CI precedence

- [Production-readiness review](ProductionReview_2026-10-28.md) — Product decisions remain unimplemented. Q3's five-second recovery cap, Q4's replacement of the busy-close dialog, and Q8's OneDrive/SMB/WSL requirements remain approved; later review drops do not revoke them. The current [unified CI contract](../../Build/Build_UnifiedCI.md) supersedes conflicting CI and test-matrix parts of Q1/Q5/Q16. Q18–Q22 remain open. The `push` trigger is already in `ci.yml`.

## Registers, not the next edit

- [RedTilde creation](RedTilde_2026-10-20.md) — S5, S6, and the long-horizon C-register. The 21 October pin, checkout paths, and "every item remains unchecked" line are stale. Text scale and the bounded math and diagram slices have landed.
- [Design refresh](DesignRefresh_2026-10-22.md) — R1–R9 and V1–V4 are implemented. The 29 October memory-cost acceptance stands. Command findings, quiet-host, policy, and hosted ARM64 remain. The cited producer `e0908338` / DxUi `086ad82a` is not the current pin.
- [Redesign completion and rendering](RedesignCompletionAndRendering_2026-10-24.md) — qualification diary and the 24 October review ledgers. The decision to stop memory-repair experiments stands. Unchecked boxes that restart allocation attribution, or that still say the consumer is on SDK086, are superseded.

## Closeout candidates

Shared ARM64, physical input, performance, and release gates stay on the delivery sequence. These two plans are not the active product task.

- [Native build policy](NativeBuildPolicyCoherence_2026-10-28.md) — the policy is merged. One closeout checkbox remains.
- [RedMonitor adoption](RedMonitor_Adoption_2026-10-23.md) — the adapter checklist is done. The open line repeats the shared gates.

Do not add another review-only plan ahead of Phase 1.
