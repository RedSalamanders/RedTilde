# RedTilde performance and resource contract

Status: current normative contract
Last reviewed: 2026-11-19

## Mandate

Performance and low resource consumption are primary RedTilde requirements, not optional cleanup. Every design,
implementation, review, and validation change minimizes CPU time, memory, allocations, copies, synchronization, GPU
submissions, wake-ups, and loaded resources while preserving correctness, security, visual quality, and required
behavior.

When several correct designs exist, use the one with the lowest steady-state resource cost unless a measured result
or a materially simpler and safer implementation justifies another choice. An intentional regression is measured in
the affected configuration and justified in the owning contract or an active WIP plan.

"Zero CPU while idle" means RedTilde owns no polling loop or periodic wake-up when no frame, input, I/O completion,
or state change is pending. Operating-system scheduling noise is outside this contract.

## Where the budgets live

Each bound is owned by the contract of the behavior it limits; this file does not restate the numbers.

| Budget or rule | Owner |
| --- | --- |
| Bounded reads and directory enumeration, the debounced recovery write with no autosave or polling timer, notification coalescing, and the single cached snapshot | [Architecture](Core_Architecture.md) |
| Document and image read limits and the retained-parent walk bound | [File safety](Core_FileSafety.md) |
| Deterministic activity and resource bounds, hidden and clean idle, and the native resource lifecycle | [Completed-frame performance](../Testing/Testing_Performance.md) |

## Measurement

Completed-frame measurement, the paired study, its investigation bands, and the resource probes are owned by
[completed-frame performance and resource validation](../Testing/Testing_Performance.md). Receipts and archived
packets follow the [run-evidence contract](../Testing/Testing_RunEvidence.md). An unpaired measurement is diagnostic
only.
