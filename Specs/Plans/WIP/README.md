# Active work

- **ACTIVE**: [Remaining work and qualification](RemainingWork_2026-11-04.md) — current qualification and delivery plan for RedTilde. Start with its verified checkpoint and ordered tasks; deferred scope and unanswered owner decisions remain explicit.
- **ACTIVE**: [Explicit state machines and lifetime safety](ExplicitStateMachines_2026-11-10.md) — six-slice plan replacing implicit boolean/optional state machines across `Application`, `AsyncFiles`, `BitmapPasteWriter`, `RecoveryService`, `FilesService`, overlays, Find & Replace, workspace switching, and test-evidence tooling (`RT-SM-01`..`RT-SM-09`) without breaking external behavior.

The owner's 7 November delivery scope requires x64 runtime tests and ARM64 compilation in Debug, Release and
ASan Debug. ARM64 runtime is explicitly set aside; the normal production release gate is unchanged.

[Completed and superseded records](../Done/README.md) retain decisions, evidence, failures and detailed appendices.
[The consolidation ledger](../Done/WipConsolidation_2026-11-04.md) accounts for every original unchecked direct-plan item.
Add new in-scope work to the current plan instead of creating another overlapping queue.
