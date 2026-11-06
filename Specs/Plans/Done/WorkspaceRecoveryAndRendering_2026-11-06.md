# Document workspaces, recovery and rendering adoption

Status: DONE — retained-file behavior, lossless failure paths and scoped exact-pin qualification.
Implementation: `fe9b7d15b522630ed54fc8891516b24137a4d5fc`.
DxUi: `8dc328dec0a80cc0e36fa08e5364265ebd25d37e`, API 4.

- [x] Retain independent source/selection/undo and dirty identity while switching Files-pane documents without prompts.
- [x] Protect all dirty documents for New, explicit Open File/Open Folder and Close, with atomic capacity, draft and callback guards.
- [x] Checkpoint/recover every modified document independently, including a two-file crash/relaunch and Restore All.
- [x] Bound continuous-typing recovery admission, checkpoint session end and retain data through cancelled/confirmed shutdown paths.
- [x] Preserve document EOLs for inserted templates, F2 Apply and draft recovery while retaining exact outside source and history.
- [x] Hold Open completion behind a source draft; preserve bad settings headers and clean up failed staging files after closing handles.
- [x] Unwind the Find replacement guard after failure; verify a later replacement and Undo.
- [x] Explicitly refuse default network-file image transport before I/O for every SDK network-file form.
- [x] Adopt the supplied Mermaid/math corrections, add the public examples and preserve 16 native theme/DPI captures plus original comparisons.
- [x] Complete six zero-warning builds, all 91 cases and 36 captures per x64 profile, real ASan detection and source/app/SDK preservation.
- [x] Update authoritative contracts, capabilities and usage documentation.

[Source-bound evidence](../../../Measurements/WorkspaceRecoveryAndRendering_2026-11-06/README.md) retains
original counterexamples, intermediate fixture/warning failures and exact identities. No OS logoff, native ARM64
runtime, physical input or paired-performance acceptance is inferred from these hidden local runs. Remaining
work stays with the [single current plan](../WIP/RemainingWork_2026-11-04.md).
