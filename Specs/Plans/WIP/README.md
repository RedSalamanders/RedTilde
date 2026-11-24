# Active work

Files in this directory are non-normative execution or decision records; current requirements stay in the owning
domain spec under `Specs/<Domain>/`. This index is routing only. Every direct plan in this directory is listed
exactly once, as a bullet that opens with its bold status and a colon (`- **ACTIVE**:`), links its title to its
file, and ends with an em dash and its purpose. `ACTIVE` is executable work, `HOLD` paused work, `DECISION`
unresolved product intent, and `RETIRED` a bounded tombstone that names its retirement date, reason, current
owner, and a `git show` recovery command.

- **ACTIVE**: [Dictionary and spell checking](DictionaryAndSpellChecking_2026-11-21.md) — implementation specification for RedTilde dictionary and spell-checking support using the selected pluggable `ISpellProvider` hybrid architecture (Windows native primary, portable `CUSTOM.DIC` custom dictionary, optional deferred Hunspell fallback), phased slices `S1`–`S4`, planned upstream DxUi simplifications (`DX-SPELL-1`–`DX-SPELL-6`), and reference annexes.

- **HOLD**: [RedTilde proofing simplification after DxUi upstream adoption](DxUiProofingSimplification_2026-11-24.md) — consumer cleanup queued behind `DxUi/Specs/Plans/WIP/RichTextProofingSupport_2026-11-24.md` (`DX-SPELL-1`–`DX-SPELL-7`) and an explicit `Dependencies/DxUi.lock.json` pin bump to delete RedTilde's temporary prose-walking, viewport-slicing, hit-test, suppression-reupload, and deferred-dispatch adapters.

Completed `v1.0.0` release plans — [Version 1.0 release readiness](../Done/V1ReleaseReadiness_2026-11-21.md) and [Remaining work and qualification](../Done/RemainingWork_2026-11-04.md) — moved to [Specs/Plans/Done/](../Done/README.md).

The owner's 7 November delivery scope requires x64 runtime tests and ARM64 compilation in Debug, Release and
ASan Debug. ARM64 runtime is explicitly set aside; the normal production release gate is unchanged.

[Completed and superseded records](../Done/README.md) retain decisions, evidence, failures and detailed appendices.
[The consolidation ledger](../Done/WipConsolidation_2026-11-04.md) accounts for every original unchecked direct-plan item.
Add new in-scope work to the current plan instead of creating another overlapping queue.
