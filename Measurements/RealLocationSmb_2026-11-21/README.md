# Live SMB Real-Location Qualification & Root-Cause Diagnosis — 21 November 2026

Source baseline: `ea111980a9aa3f9db0a8b49880b751c437e853fa` (`x64 Release` binary `D27923E4F2406B654A26944808002DDC7DBB092968DCC0A9DEE2DAAAA14BAD09`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Execution target: manifest-declared `path.smb.primary` (`smb-share`) via `Tools/Test-RealLocations.ps1 -IncludeKind smb-share -Configuration Release -IncludeNativeSuites -RetainLogs`.
Ledger: [smb-qualification.json](smb-qualification.json).
Raw retained suite reports and logs: `.build/real-location-runs/634e189f237948398089759b7fa028f7/`.

## Summary Results

| Scope | Outcome | Details |
| --- | --- | --- |
| UTF-8 BOM Showcase Capture (`--capture`) | **PASSED** | `118,136`-byte PNG + scenario sidecar verified; document SHA-256 `1BA77DC71F2CC8E75F7C6826B2300AAE8D673A3D18C6D5E076D79E5CB4C749A8` and BOM preserved |
| `Settings` (`15` cases) | **15 / 15 PASSED** | JSON5 parse/merge, lossless migration, corrupt `.invalid-*` backup, atomic save, and coalesced `SettingsWatcher` live reload all passed on the live SMB share |
| `Recovery` (`22` cases) | **22 / 22 PASSED** | Debounce, five-second continuous-typing bound, exclusive lease, 512-entry scan, close-during-save, session-end, and quarantine all passed on the live SMB share |
| `Storage` (`32` cases) | **16 PASSED / 16 FAILED** | Exposed concrete Win32/SMB2 behavior differences and `Common/Storage.cpp` SMB bugs (classified below) |
| SMB Per-Run Directory Cleanup | **VERIFIED CLEAN** | Zero `redtilde-run-*` directories left on the SMB root after execution and probe |

## Root-Cause Classification of the 16 `Storage` Failures

A direct Win32/SMB2 diagnostic probe against a fresh per-run directory on the same SMB target established four distinct root causes:

1. **Product SMB bug — pre-`MoveFileExW` timestamp becomes stale after SMB server rename (`ftLastWriteTime` delta `+503,718` 100-ns ticks ≈ `50.37 ms`)**
   - `Common/Storage.cpp` (`WriteBytesAtomic`) samples `writtenStamp` from `metadata` on `.tmp` *before* calling `MoveFileExW(..., MOVEFILE_WRITE_THROUGH)` / `ReplaceFileW`.
   - On this SMB share, `MoveFileExW` returns `TRUE`, keeping `dwVolumeSerialNumber` and `nFileIndex` identical, but the SMB server updates `ftLastWriteTime` after handle close/rename (`134397566204590000` -> `134397566205093718`).
   - Any test asserting exact `loaded.stamp == saved` (`Utf8.BomUnicodeMixedEolRoundTrip`, `Storage.PublishedSaveDoesNotDependOnContentReadback`, `Storage.AtomicSaveKeepsLongLeafWithinPathBudget`, `Storage.SharedWriterReadAndSaveAsBaseline`, and the three `Storage.ProviderFallback*` fixtures that seed via `SaveDocument`) therefore fails `FileStamp::operator==`.

2. **Product SMB bug — `ReplaceFileW` fails with `ERROR_PATH_NOT_FOUND` (`3`) on both `0` and `REPLACEFILE_IGNORE_MERGE_ERRORS` flags**
   - The diagnostic probe recorded `ReplaceFileW(..., 0) == FALSE (GetLastError() = 3)` and `ReplaceFileW(..., REPLACEFILE_IGNORE_MERGE_ERRORS) == FALSE (GetLastError() = 3)`.
   - `Common/Storage.cpp` only enters the recovery-protected in-place fallback for `ERROR_NOT_SUPPORTED` (`50`), `ERROR_INVALID_FUNCTION` (`1`), or `ERROR_CALL_NOT_IMPLEMENTED` (`120`). On an existing file on this SMB server, `ReplaceFileW` therefore returns Win32 error `3` instead of either publishing via fallback or succeeding.

3. **SMB server hardlink reporting & fixture creation limits (`nNumberOfLinks == 1`, Win32 `5` symlink, unsupported remote NTFS junction)**
   - `CreateHardLinkW` returned `TRUE`, yet subsequent `GetFileInformationByHandle` queries on the share still reported `nNumberOfLinks = 1` (preventing `Storage.cpp` from entering `SaveProtectedDocument`, causing all 5 `Storage.Hardlink*` cases to fail).
   - Remote client-side NTFS junction creation (`Files.ReparseLeavesNeverTraverse`) and relative symlink creation (`Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget`, `Win32 5`) are unsupported by this SMB share configuration.
   - `GetFileInformationByHandleEx(FileIdInfo)` returned `FALSE` with `ERROR_INVALID_PARAMETER` (`87`). `Storage.cpp` (`CompleteIdentity`) already handles error `87` gracefully by falling back to `BY_HANDLE_FILE_INFORMATION`; only `Storage.StampMatchesFullNativeFileIdentity` fails because that test unconditionally asserts `FileIdInfo` succeeds.

4. **Intentional local-profile guard in `Common/BitmapAssets.cpp` (`DRIVE_REMOTE = 4`)**
   - `GetDriveTypeW` returns `4` (`DRIVE_REMOTE`) for UNC roots. `BitmapAssets.cpp` intentionally restricts `%LOCALAPPDATA%\RedTilde\Assets` clipboard PNG creation to `DRIVE_FIXED` (`3`). Failing `Storage.BitmapAssetsEncodeAndRetireOwnedFiles` under `--data-dir <UNC>` confirms that guard rather than a defect; this test should run with a local `Assets` data root even when documents reside on SMB.

## Gate Disposition

- **Gate 5 (`T-S2`, `T-I1`) remains OPEN.**
- Required Wave 2 follow-up:
  1. Fix `Common/Storage.cpp` SMB publication (`ReplaceFileW` `ERROR_PATH_NOT_FOUND` handling / fallback admission + refresh or tolerance of post-rename `ftLastWriteTime` when content hash and file identity match) with targeted tests.
  2. Separate test-fixture SMB capability checks (`FileIdInfo`, junction, symlink, `nNumberOfLinks > 1`) from core UTF-8/atomic/fallback SMB tests so genuine SMB storage regressions fail hard without conflating unsupported server primitives.
  3. Qualify dedicated manifest-declared OneDrive and WSL `selftest` roots once authorized by the owner.
