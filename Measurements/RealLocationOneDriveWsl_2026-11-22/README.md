# Gate 5 Live OneDrive, WSL 2, and SMB Qualification — 22 November 2026

Base commit: `b32eb58db33c72ef9d14fa1b625abaaa832068d4` (plus Gate 5 `Common/Storage.cpp` and harness updates, `compiledSourceIdentity` `698e017a48960c875aca3adea89af526fa2562631cfb1958bdc7353873d252eb`, `x64 Release` binary `12598187A2B51D276111DEABDA60EA7C09B40E4DF0972D25568A7BF99628A0AE`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Execution command: `Tools/Test-RealLocations.ps1` (`-Configuration Release -IncludeNativeSuites -RetainLogs`) against owner-declared `selftest` roots.
Ledger: [onedrive-wsl-qualification.json](onedrive-wsl-qualification.json).
Retained raw logs: `.build/real-location-runs/4079c45521474e1f805c95ff4e0d1622/` (OneDrive), `.build/real-location-runs/c58bb6b2e2aa41caabeabbc9304dc85f/` and `.build/real-location-runs/9ca6932bae6342399f5438d0add7d427/` (two consecutive WSL 2 runs), and `.build/real-location-runs/f77197a930be4cd18feae69b0ec942b6/` (SMB regression verification).

## Summary Results (`gateStatus: gate5-completed`, `unexpectedFailures: []` across all backends)

| Backend & Redacted Root | Status | `--capture` | `Storage` | `Settings` | `Recovery` | Leftover Dirs |
| --- | --- | --- | --- | --- | --- | --- |
| **OneDrive** (`%USERPROFILE%\OneDrive\RedSalamander.Perf\selftest`) | **PASSED** | PASS (`119,669` B PNG, SHA-256 preserved) | **34 / 34** | **19 / 19** | **23 / 23** | **0** |
| **WSL 2 (`9P`)** (`\\wsl.localhost\<distro>\tmp\RedSalamander.Perf\selftest`, 2 consecutive runs) | **PASSED** | PASS (`119,669` B PNG, SHA-256 preserved) | **23 / 34** (`0` unexpected; `11` probed primitives) | **17 / 19** (`0` unexpected; `2` probed watch primitives) | **23 / 23** | **0** |
| **SMB / NAS** (`\\<nas-host>\<selftest-share>\selftest`) | **PASSED** | PASS (`119,669` B PNG, SHA-256 preserved) | **27 / 34** (`0` unexpected; `7` probed primitives) | **19 / 19** | **23 / 23** | **0** |

## Diagnosed & Repaired WSL 2 (`p9rdr.sys`) Findings

1. **Zero legacy volume serial with valid 64-bit inode (`Common/Storage.cpp`):**
   - WSL 2 `9P` rejects `FileIdInfo` (`Win32 50`, `ERROR_NOT_SUPPORTED`) and returns `dwVolumeSerialNumber == 0` with non-zero `nFileIndex` from `GetFileInformationByHandle`.
   - Added `VolumeRootFingerprint` inside `CompleteIdentity` ONLY when `FileIdInfo` returns unsupported (`87`/`50`/`1`), `stamp.volume == 0`, and `(stamp.fileIndex || stamp.fileIndexHigh)` is non-zero, hashing the normalized `GetFinalPathNameByHandleW` volume/share root so `HasIdentity`/`SameObject` stay deterministic across repeated reads and writes.
2. **Single-link atomic POSIX rename when `ReplaceFileW` is unsupported (`Common/Storage.cpp`):**
   - WSL 2 `9P` returns `ERROR_NOT_SUPPORTED` (`Win32 50`) for `ReplaceFileW`, whereas `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` succeeds atomically and preserves single-link atomic replacement semantics across `Settings`, `Recovery`, and `SaveDocument`.
   - Updated `WriteBytesAtomic` to retry `MoveFileExW` on unsupported real `ReplaceFileW` results (after test injection hooks, while keeping non-atomic `SaveProtectedDocument` for hardlinked files or providers where `MoveFileExW` is also unsupported).
3. **Probed provider primitives (`Tools/Test-RealLocations.ps1`):**
   - Extended `Get-RedTildeLocationCapabilities` and `Split-RedTildeRealLocationFailures` (`56` checks in `Tests/Tooling.Tests.RealLocations.ps1`) to record `fileSystemName`, `volumeSerialZero`, `hiddenAttributePreserved`, and overlapped `ReadDirectoryChangesW` support, and updated `Remove-RedTildeDirectoryNoReparse` to recurse cleanly into non-surrogate cloud directories while deleting symlinks/junctions directly.
   - On WSL 2 `9P` (`fileSystemName: "9P"`, `volumeSerialZero: true`, `hiddenAttributePreserved: false`, `readDirectoryChangesSupported: false` with `Win32 1`, `hardlinkCreateSucceeded: false`, `symlinkCreateSupported: false` with `Win32 5`, `junctionCreateSupported: false`), the harness cleanly isolates only those exact probed primitive failures while verifying `0` unexpected failures across `Storage`, `Settings`, and `Recovery`.
