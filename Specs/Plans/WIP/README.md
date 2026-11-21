# Active work

Files in this directory are non-normative execution or decision records; current requirements stay in the owning
domain spec under `Specs/<Domain>/`. This index is routing only. Every direct plan in this directory is listed
exactly once, as a bullet that opens with its bold status and a colon (`- **ACTIVE**:`), links its title to its
file, and ends with an em dash and its purpose. `ACTIVE` is executable work, `HOLD` paused work, `DECISION`
unresolved product intent, and `RETIRED` a bounded tombstone that names its retirement date, reason, current
owner, and a `git show` recovery command.

- **ACTIVE**: [Remaining work and qualification](RemainingWork_2026-11-04.md) — current qualification and delivery plan for RedTilde. T-U2 covers print preview, page selection, and output to physical printers, PDF and other installed Windows destinations.

- **ACTIVE**: [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md) — sequenced execution plan covering the seven remaining release gates (settings convergence, ARM64 runtime, physical printing, IME/AT/interactive focus, live OneDrive/SMB/WSL storage, quiet-host performance/GPU, and v1.0.0 release cut).

The owner's 7 November delivery scope requires x64 runtime tests and ARM64 compilation in Debug, Release and
ASan Debug. ARM64 runtime is explicitly set aside; the normal production release gate is unchanged.

[Completed and superseded records](../Done/README.md) retain decisions, evidence, failures and detailed appendices.
[The consolidation ledger](../Done/WipConsolidation_2026-11-04.md) accounts for every original unchecked direct-plan item.
Add new in-scope work to the current plan instead of creating another overlapping queue.
