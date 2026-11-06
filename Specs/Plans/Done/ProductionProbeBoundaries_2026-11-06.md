# Production probe and packaging boundaries

Status: DONE — implementation and source-bound local/x64-production verification.
Implementation: `526cc8db2d2716d0a86c37c1ce92c992b0e7475b`.
Dependency: API 4 `a1915ffdd94b18fd1817371f1dc074f6a0b547f1`.

- [x] Refuse all eleven test-only production switches before application profile resolution, crash-handler
  installation, probe dispatch or window creation. Missing operands refuse before operand parsing.
- [x] Exclude intentional sanitizer code and crash/sanitizer dispatch from production compilation while
  retaining supported capture/performance diagnostics and ordinary test-enabled Release.
- [x] Make default portable packaging request ProductionOnly and reject test-enabled artifacts before staging.
- [x] Execute all thirteen refusal cases against the actual clean-extracted x64 package, with exit 2 and absent
  data directories; retain the positive capture smoke, package receipt and original binary/ZIP/member hashes.
- [x] Retain the original production preferences-probe counterexample: exit 0 and an actual 285-byte write.
- [x] Complete six warning-free builds, original 141-input/native/SDK/run member verification, all local
  x64 case unions/captures, actual ASan and interrupted-write recovery. Preserve the real Win32 1314 failures.
- [x] Pass full compiler-free validation, 44 scoped-tooling cases and formatting; update build/testing contracts
  and portable usage. Resolve RT-16 against current product-diagnostic ownership without a blanket source ban.

The [packet](../../../Measurements/ProductionProbeBoundaries_2026-11-06/README.md) retains exact identities
and separates the ordinary local cohorts from the additional actual production-only build. Each local x64
cohort still has one symbolic-link privilege failure; it is not relabelled as passed.

Hosted native ARM64/production and the complete current-source test-before-production gate remain with
[T-Q2](../WIP/RemainingWork_2026-11-04.md#t-q2). Cross-builds, this x64 package and prior-source hosted passes
do not close those gates or establish physical input/AT, API-5 migration, performance or product-release acceptance.
