# Save safety and UI failure boundaries

Implementation: `7a5ecc49d40d9ef007329fb9d21f2b1c69fbb771`.
Exact dependency: DxUi API 4 at `0ffaa1a6ae43f074f60e419eaa5becc5bc3fe0b3`.
The [verified qualification](qualification.json) binds six zero-warning builds, 108 cases and 36 captures
in each x64 profile, the real ASan detector, canonical app/SDK/run archives and 141 original compiled inputs.
The compiled source identity is `6f21f442c5fd75be423a71db23d11af22aa6a42be551c5fe8062d76b3a4f8dde`.
Local ARM64 entries are cross-builds. The additive
[hosted Release packet](hosted-qualification.json) verifies main `a25f1835` in
[run 42165374140](https://github.com/RedSalamanders/RedTilde/actions/runs/42165374140):
all seven jobs pass, with all 108 cases, 36 captures and the assertion probe on
both native x64 and ARM64. Both test-disabled production ZIPs (`0.1.310`) pass
clean-extraction smoke. All thirteen original GitHub artifact wrappers match their
reported SHA256 digests; canonical receipts and inner production ZIP bytes are
independently verified. ARM64 Debug/ASan and physical qualification stay separate.

Save uses one conflict surface with Reload, Save As, Overwrite and Cancel. Its asynchronous continuations
revalidate the approved workspace, immutable source, baseline, BOM and recovered-copy facts. Failed or stale
Reload preserves edits and history; other documents and the root remain retained. Save As aliases cannot
bypass same-target conflict checks. Encoding/capacity errors identify invalid UTF-8, unsupported encodings and
the known decoded size.

Storage rechecks the expected target after staging, immediately before publication. The stamp includes the
complete 64-bit native volume identity and 128-bit file identity where supported; a timestamp-only touch with
the same object/content is harmless. Staged bytes and final metadata supply a successful stamp without a
fallible content readback after publication. Same-handle reads reject observed instability without changing
the caller's result. This is a best-effort cross-process check, not an atomic compare-and-swap or a guarantee
against uncooperative writers. Cloud/share/WSL fallback acceptance and distinct link-write policies remain open.

Shortcut, menu, window-message and save-completion boundaries preserve editable state and report localized
action failures. Successful saved path/text/stamp facts are published together before fallible view refresh.
Expected exceptions cannot escape the native UI boundary; allocation failure retains its fatal contract.
The common-controls v6 manifest dependency is explicit for the native conflict dialog.

The [retained originals](retained-attempts.json) separate three independent storage counterexamples, the
`0xC0000138` startup failure of the first zero-warning app build, and the independent full-file-identity oracle
on NTFS/ReFS. The original zero-warning startup build is not described as a native test pass. These isolated
probes remain separate from the final whole-application matrix.

Long-checkout fixtures pass twelve MSVC setup cases and 38 NTFS/evidence checks on actual NTFS. The first full
validation correctly rejected an old ARM64 dependency record still naming `8dc328de`; normal restore refreshed
it to `0ffaa1a6`, and independent compiler/SDK identity and all lease/archive policy controls pass afterward.
Original failure/identity records remain retained. Timing/FPS/memory smoke observations are unpaired and do not
establish R10 or shared performance-policy acceptance.

Durable contracts live in [file safety](../../Specs/Core/Core_FileSafety.md),
[architecture](../../Specs/Core/Core_Architecture.md), [validation](../../Specs/Testing/Testing_Validation.md),
[run evidence](../../Specs/Testing/Testing_RunEvidence.md) and [usage](../../docs/README.md).
The [single active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md) retains real locations/links,
managed images, remaining UI work, native/physical qualification, API migration and release decisions.

The previous workspace/rendering implementation has an independently verified
[hosted Release packet](../WorkspaceRecoveryAndRendering_2026-11-06/hosted-qualification.json).
It retains the earlier source independently of the new save-safety hosted packet.
