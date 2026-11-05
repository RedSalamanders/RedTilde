# Recovery Restore identity

Implementation and stabilized fixture: `31d6f59e20237dff220b0dc93723f95193e0fe81`.
Dependency: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.

Hosted main `83e764a0` completes [run 42000363611](https://github.com/RedSalamanders/RedTilde/actions/runs/42000363611):
both native x64/ARM64 Release roles pass 76 cases, 36 captures and their assertion probes. Both production
roles start after the combined gate, disable test mode and pass clean-extraction smoke. Original native/
production receipts and actual ZIP bytes are retained and rehashed in the
[safe hosted projection](hosted-qualification.json). This accepts that Restore candidate's Release scope;
the later close-during-save source and physical/performance gates remain separate.

Restore previously read the first recovery entry in its continuation after the unsaved-change question or
an admitted save. Recovery completions could remove that entry while the question pumped messages. The
continuation could restore another snapshot or dereference an empty list.

The command now captures the clicked id and immutable snapshot before the question. It checks that exact id
before changing the editor. If the copy is gone, it displays a localized explanation, preserves the current
document and leaves every other candidate available. A fresh explicit Restore can choose another copy.
Successful retirement also names only the captured id. Source, selection, history, BOM and original-location
policy remain owned by the existing recovery workflow.

## Independent failure and repair

The hidden `Recovery.RestoreKeepsClickedIdentityAcrossUnsavedPrompt` case drives ordinary application commands
and real recovery worker completion through a test-only unsaved-question callback. With unchanged Restore code
at `a6a34ccb0fdb060b9f295e5a9a59adf0ac5a0893`, six recovery cases pass and the new case fails its wrong-copy
assertion. That original run is `20261104T161140153Z-59104-b834dc60494e48be99a6480a9c963a5a`.
Its executable, PDB, raw logs and failed canonical receipt remain retained.

The initial fixed Debug run at `924d1ce6` exposes a fixture synchronization flaw: the final UI pump after
waiting can publish retirement, refresh the drawer and submit another checkpoint. That failed run,
`20261104T162528723Z-27684-27345ffadd444dabb195f97a153fbd09`, remains separate. The stabilized fixture checks
ready, settled and expected candidate count after pumping, verifies successful worker status, and explicitly
checks that the drawer-refresh checkpoint was exercised. Its timeout and Restore assertions are unchanged.

Both another remaining snapshot and an empty list are covered. The current dirty revision, selection, undo
history and recovered state must remain exact; the remaining copy stays on disk and a fresh Restore can
explicitly select it. The hidden window preserves desktop foreground throughout.

## Qualification and retained evidence

[The qualification projection](qualification.json) binds six builds with zero warnings/errors, all 76 native
cases and 36 PNG/sidecar captures in each x64 Release, Debug and ASan Debug profile: 228 case executions and
108 captures. Separate process, recovery-restart, crash and performance-smoke obligations pass, as does the
real isolated ASan heap-use-after-free detection probe.

The interrupted matrix retained its five completed profiles. ARM64 Debug's completed bytes were verified and
preserved; only the final ARM64 ASan build resumed on 5 November. All six native archives were rehashed and
their executable/receipt members independently checked before this projection was written. Original logs,
receipts, binaries/symbols and preservation inventories remain private. The canonical integrated source has
compiled identity `5e137713537ca826ed99f57a7a82253eeb597eaab84bb669688cd20b5665584a`, matching the frozen candidate.

ARM64 coverage here is cross-build coverage. Hidden functional checks and ordinary unpaired FPS/memory
observations do not establish native ARM64 runtime, physical keyboard/IME/AT/touch, paired performance or
product-wide release acceptance. Those gates and other remaining correctness work stay with the
[active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).

## Additional real keyboard/focus qualification

After a fresh Release build under the current compiler context, the user explicitly agreed to run the canonical
desktop harness with its visible 30-second automatic Start countdown. The exact pinned lease was prepared
without changing SDK library/PDB bytes. All nine real-input stages pass, including F2 in-place equation editing,
undo/exit, the File menu and asynchronous clean close. The parent independently verifies unchanged original
foreground/focus and restored pointer; the child reports no clipboard access.

[The keyboard projection](keyboard-qualification.json) binds the app/lease binaries, original parent/child
receipt hashes, nine outcomes and restoration facts. Original foreground identities and fixture paths remain
private. This closes the named real-keyboard/F2 gates; it supplies no physical IME, touch, assistive-technology,
clipboard, other-platform or paired-performance acceptance.
