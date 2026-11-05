# Recovery Restore identity

Status: DONE — scoped implementation, regression, build/runtime and domain-documentation closeout

- [x] Reproduce RT-04/PR-06 against unchanged Restore logic: discard the clicked snapshot during the unsaved
  prompt while another copy remains; observe the wrong-copy assertion with six other recovery cases passing.
- [x] Capture the selected candidate and its immutable snapshot before the prompt; revalidate its exact id
  after the prompt or save, decline a missing copy with a localized explanation, and retire only that id.
- [x] Cover both a remaining candidate and an empty list, exact current revision/selection/history retention,
  a fresh explicit Restore of the remaining copy and unchanged desktop foreground.
- [x] Preserve the initial Debug synchronization failure. Check ready/settled/candidate-count facts after
  hidden UI pumping, including the drawer refresh's new checkpoint; keep the original timeout and assertions.
- [x] Pass six zero-warning/zero-error builds, all 76 cases and 36 captures in each x64 configuration,
  process/recovery/crash/performance-smoke checks and the real ASan detection probe.
- [x] Preserve each native executable/PDB/receipt closure in a member-verified archive; retain the five
  completed profiles and resume only the final ARM64 ASan build on 5 November.
- [x] Merge lasting behavior into [file safety](../../Core/Core_FileSafety.md), the
  [validation contract](../../Testing/Testing_Validation.md) and [usage documentation](../../../docs/README.md).
- [x] Retain and verify hosted main `83e764a0` / run 42000363611: native x64/ARM64 Release each pass
  76 cases, 36 captures and assertion probes; both production ZIPs pass after the combined gate with tests
  disabled and successful smoke. [Original receipt/archive identities](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/hosted-qualification.json)

Implementation and stabilized fixture: `31d6f59e20237dff220b0dc93723f95193e0fe81`.
SDK pin: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.
The [evidence packet](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/README.md) records exact source,
native run, failure, archive and receipt identities. The canonical integrated source has the same compiled
identity as this frozen qualification checkpoint.

This closes RT-04/PR-06. Other recovery/shutdown/storage work, native ARM64 runtime for the new app candidate,
physical input/AT, paired performance and release acceptance remain in the
[single active plan](../WIP/RemainingWork_2026-11-04.md).
