# Live SMB Real-Location Re-Qualification — 22 November 2026

Base commit: `4a255bcd5705c01346c9a728b6b6625ace9118c4` (plus Wave 2 `Common/Storage.cpp` & harness updates, `compiledSourceIdentity` `6a0a60b2339bd1f7832899793b38ce3171c3da38d40bfa483b3e87103039201a`, `x64 Release` binary `6728583A367DE10D6D0962F74DDB823F492D8E73C62CE87FA50D123403AC3378`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Execution target: manifest-declared `path.smb.primary` (`smb-share`, `\\<nas-host>\<selftest-share>\selftest`) via `Tools/Test-RealLocations.ps1 -ManifestPath 'Z:\RedSalamander.Perf\config\machine-resources.json' -IncludeKind smb-share -Configuration Release -IncludeNativeSuites -RetainLogs` (verified across two consecutive passing runs: `8c2367e01a074b388411d740745ddf6d` and `70abe690f78a434db367a309c6ca2e4b`).
Ledger: [smb-qualification.json](smb-qualification.json).
Raw retained suite reports and logs: `.build/real-location-runs/70abe690f78a434db367a309c6ca2e4b/`.

## Summary Results (`status: PASSED`, `unexpectedFailures: []`)

| Scope | Outcome | Details |
| --- | --- | --- |
| UTF-8 BOM Showcase Capture (`--capture`) | **PASSED** | `118,565`-byte PNG + scenario sidecar verified; document SHA-256 `1BA77DC71F2CC8E75F7C6826B2300AAE8D673A3D18C6D5E076D79E5CB4C749A8` and BOM preserved |
| `Settings` (`19` cases) | **19 / 19 PASSED** | JSON5 parse/merge, lossless migration, `.invalid-*` backup, coalesced live reload burst, and `Settings.SplitScrollSyncPersistenceAndLiveReload` all passed live on SMB2 |
| `Recovery` (`23` cases) | **23 / 23 PASSED** | Debounce, five-second typing cap, exclusive leases, 512-entry scan, close-during-save, session-end, and quarantine all passed live on SMB2 |
| `Storage` (`34` cases) | **27 / 34 PASSED · 0 `unexpectedFailures`** | Every UTF-8 round-trip, atomic save, `ProviderFallback*`, `SharedWriterReadAndSaveAsBaseline`, `StampMatchesFullNativeFileIdentity`, `PublishedSaveDoesNotDependOnContentReadback`, `PublishedTimestampRefreshPreservesIdentityAndGuardsReplacement`, `ImageFileReadAndWatchOnCurrentRoot`, and `BitmapAssetsEncodeAndRetireOwnedFiles` (`DRIVE_REMOTE` refusal guard) passed live on SMB2; the `7` remaining failures are exclusively probed server-unsupported primitives (`unsupportedProviderPrimitives`) |
| SMB Capability Probe | **VERIFIED** | `driveType: 4`, `fileIdInfoSupported: false (87)`, `hardlinkCreateSucceeded: true`, `hardlinkLinkCountReported: false` (`nNumberOfLinks == 1`), `hardlinkSharesFileIndex: true`, `symlinkCreateSupported: false (5)`, `junctionCreateSupported: false` |
| SMB Per-Run Directory Cleanup | **VERIFIED CLEAN** | Zero `redtilde-run-*` directories left on `\\<nas-host>\<selftest-share>\selftest` after execution |

## Implemented Repairs Covered by This Run

1. **Best-effort post-publication timestamp refresh & protected-write timestamp pin (`Common/Storage.cpp`):**
   - Added `RefreshPublishedTimestamp` inside `WriteNewBytesDurable`, `WriteBytesAtomic`, and `SaveProtectedDocument`: re-opens published path for metadata (`0` access), adopts updated `ftLastWriteTime` ONLY when handle identity (`SameObject`) and byte size match, never opens content, never fails a completed write, and never adopts a foreign replacement or external size change.
   - `SaveProtectedDocument` also pins `SetFileTime` + `FlushFileBuffers` before closing the write handle and closes `metadata` before `RefreshPublishedTimestamp`.
2. **Pre-publication handle release (`Common/Storage.cpp`):**
   - `SaveDocument` closes its initial target `metadata` probe handle before invoking `WriteBytesAtomic` (`ReplaceFileW`) or `SaveProtectedDocument`, eliminating an open SMB2 fid on the replaced target while preserving the `link` lease for symlinks.
3. **Probed capability separation (`Tools/Test-RealLocations.ps1`):**
   - `Get-RedTildeLocationCapabilities` probes `FileIdInfo`, `nNumberOfLinks`, hardlink index sharing, symlinks, and junctions inside `<runRoot>\cap-probe` (cleaned up immediately).
   - `Split-RedTildeRealLocationFailures` separates only exact primitive-dependent cases when their probed capability is `false`, ensuring any core UTF-8, atomic save, timestamp-refresh, or fallback failure remains an unexpected failure (`NATIVE_SUITE_FAILED`).
   - `Tests/Support/Support.Tests.StorageSafety.h` verifies legacy `BY_HANDLE_FILE_INFORMATION` identity ONLY when `FileIdInfo` explicitly returns `ERROR_INVALID_PARAMETER`, `ERROR_NOT_SUPPORTED`, or `ERROR_INVALID_FUNCTION`, plus adds `Storage.PublishedTimestampRefreshPreservesIdentityAndGuardsReplacement` and `Storage.ImageFileReadAndWatchOnCurrentRoot`.
   - `Tests/Application.Tests.Core.cpp` explicitly verifies the intentional `DRIVE_REMOTE` `ERROR_ACCESS_DENIED` guard in `Storage.BitmapAssetsEncodeAndRetireOwnedFiles`.

## Remaining Gate 5 Scope

- **SMB/NAS (`path.smb.primary`):** Live qualification **PASSED** (`0` unexpected failures).
- **OneDrive Personal/Business & WSL 2:** Still **OPEN**, awaiting explicit owner-declared dedicated `selftest` roots (fail-closed policy forbids writing outside explicitly declared `selftest` roots).
