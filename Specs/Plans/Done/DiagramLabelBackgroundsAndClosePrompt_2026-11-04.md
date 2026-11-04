# Diagram label backgrounds and duplicate Close

Status: DONE — scoped implementation, native qualification and authoritative documentation

- [x] Adopt exact DxUi API-4 implementation `2a7e7c28f95a70193e4a2f68a89001d54b4d2131` for canvas-matching diagram labels and prepared contrast fallback.
- [x] Add horizontal state/message-label examples and deterministic Light/Dark native capture scenarios.
- [x] Preserve original before/after PNGs, sidecars, fixture and evidence bytes; verify all 22 published blobs against their original hashes.
- [x] Verify all eight Review/Save background samples equal the unchanged canvas in the corrected Light/Dark, 96/192-DPI comparisons.
- [x] Implement Q18: keep the existing Save/Discard/Cancel prompt and ignore duplicate native or command Close.
- [x] Register the hidden reentrancy regression: one prompt, exact dirty source/selection/history after Cancel, then a fresh Close and Discard.
- [x] Pass six zero-warning/zero-error builds, all 75 cases in each x64 configuration, all 36 captures per configuration, process/recovery/crash/performance-smoke checks and a real ASan detection probe.
- [x] Update file-safety, design and validation contracts, public usage/examples and change fragments.

Runtime implementation: `2879d4dc3786866227149d0267d06bedcf6a6cfd`. The
[original capture packet](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/README.md) and
[qualification projection](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/qualification.json)
retain the exact code, pin, binary, fixture, run and log identities. The reviewed Release executable has SHA-256
`88DEC3AB0CA64F2E2A1BEFBAA9B900646821501DFD3263C9B886BA4019F268D0`.

| x64 configuration | Complete native run |
| --- | --- |
| Release | `20261104T141932128Z-57212-db1d3317131f413f9e06e15c6fc7de45` |
| Debug | `20261104T142356867Z-57212-4a2ceb19129c43cd818ea646ae537c95` |
| ASan Debug | `20261104T142816378Z-57212-c4115212e47c414e8fcbbe0e7a99deca` |

The final ARM64 ASan cross-build was initially refused by the 1,800 MiB capacity guard. A previously verified
archive was relocated with matching bytes and repaired restore metadata, then only the remaining profile
was resumed. All earlier accepted profiles and native runs were retained. This was admission refusal before
compilation, not a native test failure. No ARM64 runtime or physical-input acceptance is inferred from cross-builds.

Durable behavior belongs to [file safety](../../Core/Core_FileSafety.md),
[design](../../UI/UI_DesignSystem.md) and [validation](../../Testing/Testing_Validation.md).
Remaining shutdown/storage/editing correctness, current-pin native ARM64, physical input/AT, paired performance
and combined hosted-main acceptance retain their owners in the [single active plan](../WIP/RemainingWork_2026-11-04.md).
