# Recovery envelope bounds and deterministic file fixtures

Status: DONE — scoped RT-08 implementation, regression and documentation closeout

- [x] Retain the unchanged parser's checksum-valid oversized-envelope termination, original input, executable/PDB and receipt.
- [x] Return invalid data for decoded-capacity failures without publishing partial caller output or terminating recovery.
- [x] Cover ASCII, BMP and supplementary characters, complete original quarantine bytes, collisions and the valid full-capacity boundary.
- [x] Preserve the first Release fixture failures and make fixtures create their own directories independently of asynchronous recovery initialization.
- [x] Retain the earlier native ARM64 hosted Open fixture failures; production remains withheld for that failed source.
- [x] Pass six zero-warning/error builds, all 82 cases and 36 captures in each x64 profile, process/recovery/crash checks and real ASan detection.
- [x] Verify native/SDK/source archives and the canonical integration against actual original compiled input bytes.
- [x] Validate canonical specifications, tooling, formatting and the governed single-entry changelog fragment.
- [x] Merge lasting behavior into [file safety](../../Core/Core_FileSafety.md),
  [validation](../../Testing/Testing_Validation.md) and usage documentation; review existing recovery capabilities.
- [x] Verify the repaired source's hosted native x64/ARM64 Release execution and both test-disabled production ZIPs after the combined test gate.

Qualified source: `bc024dd88d061a8530ead80a5c6df7505a88637d`.
SDK: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.
[The evidence packet](../../../Measurements/RecoveryEnvelopeBounds_2026-11-05/README.md) preserves exact
counterexample, fixture-failure, native/profile and original-source identities.

This closes RT-08 and its fixture repair. Session end, continuous-typing staleness, other recovery/storage
behavior, newer-source native/production acceptance, ARM64 Debug/ASan, physical input and paired R10 acceptance remain
in the [single active plan](../WIP/RemainingWork_2026-11-04.md).
