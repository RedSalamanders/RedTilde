# Close during Save and bounded Editor validation

Status: DONE — scoped RT-02/PR-05 implementation, regression, build/runtime and domain-documentation closeout

- [x] Reproduce four old busy-close contract failures with real worker admission barriers.
- [x] Retain save/result/recovery while hidden; reconcile the saved baseline, protect later edits and ignore duplicate Close.
- [x] Cancel failed Close, restore placement with the error, retain original bytes/source/history/recovery and permit explicit retry.
- [x] Abandon Open before protecting later edits and refuse late publication after Cancel.
- [x] Preserve the focus-loss counterexample and retain native lifecycle while refusing queued edits.
- [x] Retain both whole-Editor timeouts; use bounded children with unchanged assertions/deadlines and mandatory complete case union.
- [x] Pass six zero-warning/error builds, all 81 cases and 36 captures per x64 profile, process/recovery/crash checks and real ASan detection.
- [x] Recheck five preserved profiles and resume only final ARM64 ASan; retain app/SDK/source closures and raw failures.
- [x] Pass canonical tooling/format validation and all 42 independent scoped-runner fixtures.
- [x] Merge lasting behavior into [file safety](../../Core/Core_FileSafety.md), [design](../../UI/UI_DesignSystem.md),
  [validation](../../Testing/Testing_Validation.md), usage docs and the capability manifest.

Qualified source: `15ac77cf68521570c21d1a161445a55305cdeee9`.
SDK: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.
[The evidence packet](../../../Measurements/CloseDuringSave_2026-11-05/README.md) records exact source,
counterexample/timeout, profile-archive and canonical receipt identities.

This closes RT-02/PR-05 and the bounded execution repair needed for qualification. Native ARM64 runtime,
physical/foreground input, paired performance, other recovery/storage work and release acceptance remain
in the [single active plan](../WIP/RemainingWork_2026-11-04.md).
