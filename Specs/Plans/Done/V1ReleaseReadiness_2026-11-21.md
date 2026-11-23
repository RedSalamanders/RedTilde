# RedTilde Version 1.0 Release Readiness Plan

## Metadata

- **Status**: `DONE` (closed `2026-11-23` — `RedTilde 1.0.0` production release qualified and shipped)
- **Date**: `2026-11-21` · **Closed**: `2026-11-23`
- **Baseline commit**: `b7c31920b8bb9f1c0952d6b6f152978328137058` (`main` after PR #145) · **Release baseline**: `1b8174b8d28b6177ee81c79df2f5b3066c0dc959` (after PR #164)
- **Companion plan**: [RemainingWork_2026-11-04.md](RemainingWork_2026-11-04.md) (`DONE`; post-1.0 governance holds `Q15`/`Q17` recorded in [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md))
- **Post-1.0 active feature plan**: [DictionaryAndSpellChecking_2026-11-21.md](../WIP/DictionaryAndSpellChecking_2026-11-21.md) (`ACTIVE`, targeted for `v1.1`)
- **Archived predecessor**: [ExplicitStateMachines_2026-11-10.md](ExplicitStateMachines_2026-11-10.md) (`DONE`)
- **Authoritative contracts**:
  - [Specs/README.md](../../README.md)
  - [Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  - [Core_Settings.md](../../Core/Core_Settings.md)
  - [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md)
  - [Core_CrashHandling.md](../../Core/Core_CrashHandling.md)
  - [Editor_Markdown.md](../../Editor/Editor_Markdown.md)
  - [UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
  - [Build_Toolchain.md](../../Build/Build_Toolchain.md)
  - [Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [Testing_Performance.md](../../Testing/Testing_Performance.md)
  - [capabilities.json](../../../capabilities.json)

---

## Purpose

All local in-repo state-machine, CLI, UX, installer, packaging, documentation, and x64 full-matrix readiness work is landed on `main` (`b7c31920`). This plan sequences the **seven remaining release gates (Gates 1–7)** required to advance RedTilde from `0.1` (`experimental`) to a signed, qualified `1.0.0` production release.

Source merge alone is never product release acceptance. Every gate below closes only when source-bound execution evidence is recorded under [Measurements/](../../../Measurements/README.md), domain contracts reflect the verified behavior, and [capabilities.json](../../../capabilities.json) stays truthful.

---

## Non-Breaking Rules

1. One focused PR per gate slice (mechanical reformatting never shares a PR with functional behavior changes).
2. Consume `DxUi` strictly through the exact commit and `apiRevision` in [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json) unless executing the upstream `DxUi` pin transition in Gate 7.
3. Preserve UTF-8 BOM/EOL fidelity, atomic saves, symlink following, multi-hardlink durable recovery copies, resource-string `{0}` placeholders, WIL RAII, zero `/Wall` warnings across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` builds, and `Changes/` fragments.
4. Real-location, interactive-desktop, physical-printer, and quiet-host performance tests never run silently or unsupervised in default headless CI.

---

## Gate Breakdown (Points 1–7)

### Gate 1 — Settings & Hygiene Convergence (`CV-6-05`, `H-10`, `H-15`, `H-26`, `H-27`) — **COMPLETED**

- **Status:** **COMPLETED** (merged to `main` in `de873ee`, folded in PR #147 `ea111980`, and qualified across local `x64` profiles plus hosted `Qualify main Release` `#44928555514` and hosted native `ARM64` runs `#44930332182`/`#44930335298`; see [Gate 1 & ARM64 qualification](../../../Measurements/Gate1AndArm64Qualification_2026-11-21/README.md)).
- **Closes `RemainingWork` rows:** `T-CV1` (`H-10`, `H-15`, `H-26`, `H-27`), `CV-6-05`.
- **Implemented slices:**
  1. **Slice 1A — Mechanical Formatter Convergence (`CV-6-05`):**
     - Adopted RedHome's 155-line [.clang-format](../../../.clang-format) style and `Install-CiFormatter` inside [format.ps1](../../../format.ps1), formatting C/C++ files with zero behavioral changes.
  2. **Slice 1B — `_camelCase` Member Naming & `.clang-tidy` (`H-27`):**
     - Added repository-root [.clang-tidy](../../../.clang-tidy) (`bugprone-reserved-identifier`, `readability-identifier-naming` requiring `_camelCase` private/protected non-static members), converted private/protected fields across `Common/`, `RedTilde/`, and `Tests/`, and added compiler-free [Tests/Tooling.Tests.NamingStyle.ps1](../../../Tests/Tooling.Tests.NamingStyle.ps1) to [validate.ps1](../../../validate.ps1).
  3. **Slice 1C — `yyjson` Settings Store, Schema & Live Reload (`H-10`, `H-15`, `H-26`):**
     - Added [vcpkg.json](../../../vcpkg.json), pinned [vcpkg-tool.json](../../../vcpkg-tool.json) (`aeb162b7`), [Build/vcpkg-triplets/](../../../Build/vcpkg-triplets/), [Build/VcpkgToolset.psm1](../../../Build/VcpkgToolset.psm1), and [vcpkg-install.ps1](../../../vcpkg-install.ps1) strictly inside `.build/` (`--binarysource=clear`, `VCPKG_DISABLE_METRICS=1`, `VCPKG_DOWNLOADS`), with `vcpkgSha256` receipt binding in [Build/Harness.psm1](../../../Build/Harness.psm1) and `Dependencies/yyjson-LICENSE.txt` staging in [package.ps1](../../../package.ps1).
     - Migrated `Common/Settings.{h,cpp}` to UTF-8 JSON5 (`YYJSON_READ_JSON5 | YYJSON_READ_ALLOW_BOM`, `16 MiB` read budget), lossless one-time migration from legacy `RedTilde.Settings/1`, `.invalid-*` backup on cold parse failure, document-tree merge preserving unknown keys and newer integral `schema` versions, and emitted [SettingsStore.schema.json](../../../SettingsStore.schema.json) alongside `RedTilde.exe`.
     - Renamed active settings files and contract to `RedTilde.settings.json`, `RedTilde-debug.settings.json`, and [Specs/Core/Core_Settings.md](../../Core/Core_Settings.md) (`H-26`).
     - Added event-driven coalesced `SettingsWatcher` (`OVERLAPPED` `ReadDirectoryChangesW` + manual-reset `_stopEvent` + `ProcessWorkLease`, `250 ms` coalescing, non-writing live reload via `LoadSettingsLive` and `FileStamp` loop prevention) (`H-15`), plus [Tests/Application.Tests.Settings.{h,cpp}](../../../Tests/Application.Tests.Settings.cpp).
- **Verified local evidence:**
  - All six `(x64, ARM64) × (Debug, Release, ASan Debug)` builds compile with zero warnings (`passed: true`, source identity `95c5bd10`).
  - Full x64 non-interactive native test runs pass in all three profiles (`178` catalog cases: `Storage 32`, `Settings 15`, `Recovery 22`, `Editor 109`, plus `Performance`, `Preferences`, `RecoveryRestart`, `Sanitizer`, `FailureReportRouting`, and `InteractiveProbeRefusal`):
    - `x64 Debug`: `.build/evidence/test-runs/20261121T150532426Z-67508-1e25903bd93340169cdc830f2f2282b0` (`outcome: passed`)
    - `x64 Release`: `.build/evidence/test-runs/20261121T150913464Z-74740-e9eca9c010d148159d4283941060c9dc` (`outcome: passed`)
    - `x64 ASan Debug`: `.build/evidence/test-runs/20261121T151254349Z-67996-a999f12ffea54ab9b67b49024bb697dd` (`outcome: passed`)
  - `.\format.ps1 -Check`, `.\validate.ps1`, and `.\Tools\Test-Changelog.ps1 -Base origin/main` pass after Phase 5 reconciliation.

---

### Gate 2 — Physical ARM64 Runtime Qualification (**COMPLETED**)

- **Status:** **COMPLETED** on hosted `windows-11-vs2026-arm` (`Arm64`, `10.0.26200.0`, commit `de873ee`, DxUi `0c371a6197eb8c6b876b4b2843d75078bc21035c`); `v1.0.0` release cut remains under Gate 7.
- **Closes `RemainingWork` rows:** `T-Q2` (ARM64 runtime parity), hosted ARM64 qualification notes.
- **Executed & verified evidence ([Gate 1 & ARM64 qualification](../../../Measurements/Gate1AndArm64Qualification_2026-11-21/README.md)):**
  1. `ARM64 Release` + portable production ZIP smoke (`Qualify main Release` [#44928555514](https://github.com/RedSalamanders/RedTilde/actions/runs/44928555514)): `0 Warning(s)`/`0 Error(s)`, `178/178` catalog cases (including `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget`), `40` captures, `FailureReportRouting`, 6 dumps (`-531475201`), plus `RedTilde-0.1.519-ARM64-20261121-093426-031.zip` (`smokePassed: true`, `16/16` exit-`2` refusals).
  2. `ARM64 Debug Full` (`Manual extended qualification` [#44930332182](https://github.com/RedSalamanders/RedTilde/actions/runs/44930332182)): `0 Warning(s)`/`0 Error(s)`, `178/178` catalog cases, `40` captures, `FailureReportRouting` (`exitCode 3`), 6 dumps.
  3. `ARM64 ASan Debug Full` (`Manual extended qualification` [#44930335298](https://github.com/RedSalamanders/RedTilde/actions/runs/44930335298)): `0 Warning(s)`/`0 Error(s)`, `178/178` catalog cases, `40` captures, `FailureReportRouting`, real ARM64 ASan detection (`heapUseAfterFreeDetected: true`, `exitCode 1`), 5 dumps.

---

### Gate 3 — Physical Printer & Virtual Driver Qualification (`T-U2`) — **COMPLETED**

- **Status:** **COMPLETED** (`2026-11-22`, source commit `fc33843f`, Windows `10.0.26300.9974`, `x64 Release` binary `B4AD60EB15D62C62B171821F81AC504A680ADEB930BABD913E0AF5537B0A20E1`, DxUi `0c371a6197eb8c6b876b4b2843d75078bc21035c`; see [PrintDriverQualification_2026-11-22](../../../Measurements/PrintDriverQualification_2026-11-22/README.md)).
- **Closes `RemainingWork` rows:** `T-U2` step 6 (**Publish the qualified behavior**).
- **Verified live destination results (`leftoverPrintTemps = 0`):**
  1. **`Microsoft Print to PDF` (`Microsoft Print To PDF` `10.0.26100.4484`, `PORTPROMPT:`):** Generated and independently verified via `Windows.Data.Pdf.PdfDocument` + FlateDecode inspection + rendered bitmap ink checks across `docs/examples/markdown-showcase.md` (Letter Portrait `3/3` pages `1,395,041` B; A4 Landscape disjoint/overlapping range `1, 2-3, 2` -> `3` pages `1,389,291` B), `docs/examples/rendering-fallback-cases.md` (`Current page = 2` -> `1` page `1,060,063` B), `docs/examples/local-images.md` (`2/2` pages with all 5 formats & `6` image XObjects `1,152,279` B), pre-existing sentinel PDF overwrite via `ReplaceFileW` (`1` page `1,056,017` B), cancelled `ChooseFile` sentinel preservation (`32` B unchanged, `ERROR_CANCELLED`), and open-source overwrite refusal (`E_ACCESSDENIED`, source SHA-256 unchanged).
  2. **Physical & secondary virtual drivers (zero paper spooled per explicit owner scope A2):** Verified `ConfigurePrinter`, `DEVMODEW` → `PrintTicket` XML conversion (`S_OK`), Portrait/Landscape/A4 transitions, duplex (`OneSided`/`LongEdge`/`ShortEdge` & `999` copies/collation on `Brother HL-L2445DW Printer`), and preview bitmap generation (`previewFallbackPaper = false`, `previewHasInk = true`) across `Brother HL-L2445DW Printer` (`Microsoft IPP Class Driver`), `OneNote (Desktop)` (`Send to Microsoft OneNote 16 Driver`), and `OneNote (Desktop) - Protected` (`Microsoft Virtual Print Class Driver`); physical toner/paper sheet inspection remains explicitly scoped as `"physical-printer-paper-sheet-inspection"`.
  3. **Deterministic inventory & contracts:** All 9 headless printing cases remain registered in [Tests/NativeInventory.json](../../../Tests/NativeInventory.json) and [Tests/native-case-catalog.json](../../../Tests/native-case-catalog.json); updated [Editor_Markdown.md](../../Editor/Editor_Markdown.md), [UI_DesignSystem.md](../../UI/UI_DesignSystem.md), and [capabilities.json](../../../capabilities.json) (`real-windows-pdf-and-driver-print-qualification` promoted to `supported`).

---

### Gate 4 — Physical Input, Assistive Technology & Agreed Interactive Focus (`T-Q2`) — **COMPLETED (v1.0.0 Scope)**

- **Status:** **COMPLETED (v1.0.0 scope)** (`2026-11-22`, `a85cdf1` `x64 Release` binary `29CC8C389BE4B4E1A6379E07C35F167944279FE0EAC0F6E83DA7FC9E3745FB9F`, pinned DxUi `0c371a6197eb8c6b876b4b2843d75078bc21035c`; see [InteractiveInputQualification_2026-11-22](../../../Measurements/InteractiveInputQualification_2026-11-22/README.md)); physical East Asian IME, Narrator screen-reader walkthrough, and physical Touch/Pen hardware checks explicitly deferred post-1.0 per owner decision.
- **Closes `RemainingWork` rows:** `T-Q2` authorized interactive focus, live UIA inspection, and deterministic clipboard/media qualification (`"authorized-interactive-menu-and-focus-qualification"` promoted to `supported`).
- **Verified results:**
  1. **Real-desktop interactive lease (`test-interactive.ps1 -Platform x64 -Configuration Release -SkipBuild -InteractiveAutoStart`):** Run `20261122T220455008Z-19144-a535ec0dc2374e57959a650a49179eaf` passed all `9/9` real keyboard/focus checks (`134` `SendInput` events, `1.3 s`, `Exit: 0`, `Foreground: unchanged`, `Focus: unchanged`, `Cursor: restored`; earlier exit-`25` user cursor-shift attempt `cd8d77d3` retained transparently).
  2. **Live `UIAutomationClient` tree & pattern probe (`uia-probe-receipt.json`):** `62` descendants (`56` named controls, `9` patterns incl. `TextPattern`, `TogglePattern`, `InvokePattern`, `TablePattern`), verified `Source`/`Compose` view activation and `Settings` drawer toggle, clean `CloseMainWindow()` exit (`0`).
  3. **Clipboard & bitmap-paste qualification (`29CC8C38...`):** `--self-test` scopes `EditorMedia`, `EditorBehavior`, `Settings`, and `Storage` all exited `0` (covering `Editor.ClipboardBitmapPaste*`, `Settings.ImageFirstPasteRoundTripAndFallback`, `Storage.BitmapAssetsEncodeAndRetireOwnedFiles`, `Editor.FilePasteAdmissionRejectsHostileNames`).

---

### Gate 5 — Real Storage & Network Share Qualification (`T-S2`, `T-I1`) — **COMPLETED**

- **Status:** **COMPLETED** (`2026-11-22`, closing SMB `path.smb.primary` plus owner-authorized OneDrive and WSL 2 `selftest` roots with `0` unexpected failures and `0` leftover directories; see [RealLocationOneDriveWsl_2026-11-22](../../../Measurements/RealLocationOneDriveWsl_2026-11-22/README.md) and [RealLocationSmb_2026-11-22](../../../Measurements/RealLocationSmb_2026-11-22/README.md)).
- **Closes `RemainingWork` rows:** `T-S2` (real OneDrive/cloud, SMB/NAS, WSL 2 qualification), `T-I1` (Q8/Q13 real-location acceptance).
- **Declared storage locations & observed results (`12598187A2B51D27`, compiled source `698e017a`):**
  - **OneDrive (`path.onedrive.selftest`, `%USERPROFILE%\OneDrive\RedSalamander.Perf\selftest`, `NTFS`):** `status: PASSED` (`redtilde-run-4079c45521474e1f805c95ff4e0d1622`) — `--capture` (`119,669` PNG, SHA-256 preserved), `Storage` **`34/34`**, `Settings` **`19/19`**, `Recovery` **`23/23`**, `0` leftover directories.
  - **WSL 2 (`path.wsl.selftest`, `\\wsl.localhost\<distro>\tmp\RedSalamander.Perf\selftest`, `9P` / `p9rdr.sys`):** Two consecutive runs (`redtilde-run-c58bb6b2e2aa41caabeabbc9304dc85f` and `redtilde-run-9ca6932bae6342399f5438d0add7d427`) **`status: PASSED`** — `--capture` (`119,669` PNG, SHA-256 preserved), `Recovery` **`23/23`**, `Settings` **`17/19`** (`0` unexpected; `2` probed watch primitives), `Storage` **`23/34`** (`0` unexpected; `11` probed `9P` primitives), `0` leftover directories.
  - **Dedicated SMB / NAS (`path.smb.primary`, `\\<nas-host>\<selftest-share>\selftest`, `NTFS`):** Re-qualified (`redtilde-run-f77197a930be4cd18feae69b0ec942b6`) **`status: PASSED`** — `--capture` (`119,669` PNG, SHA-256 preserved), `Settings` **`19/19`**, `Recovery` **`23/23`**, `Storage` **`27/34`** (`0` unexpected; `7` probed primitives), `0` leftover directories.
- **Implemented repairs & fail-closed contract:**
  1. [Tools/Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1) + [Tests/Tooling.Tests.RealLocations.ps1](../../../Tests/Tooling.Tests.RealLocations.ps1) (`56` checks in [validate.ps1](../../../validate.ps1)): fail-closed validation requiring explicit `selftest` segments, extended `Get-RedTildeLocationCapabilities` (`fileSystemName`, `volumeSerialZero`, `hiddenAttributePreserved`, `readDirectoryChangesSupported`), exact `Split-RedTildeRealLocationFailures` classification, and non-surrogate `Remove-RedTildeDirectoryNoReparse` cleanup.
  2. SMB2 + WSL 2 `9P` repairs ([Common/Storage.cpp](../../../Common/Storage.cpp), [Specs/Core/Core_FileSafety.md](../../Core/Core_FileSafety.md)): post-publication metadata-only `RefreshPublishedTimestamp`, `SetFileTime`+`FlushFileBuffers` pinning in `SaveProtectedDocument`, pre-publication metadata handle release in `SaveDocument`, normalized volume/share `VolumeRootFingerprint` in `CompleteIdentity` when `FileIdInfo` is unsupported and `dwVolumeSerialNumber == 0`, and single-link `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` fallback when real `ReplaceFileW` returns unsupported.
  3. [capabilities.json](../../../capabilities.json): promoted `"native-cloud-smb-nas-and-wsl-location-qualification"` to `supported`.

---

### Gate 6 — Paired Quiet-Host Performance & Real Hardware Graphics (`T-P1`) — **COMPLETED (v1.0.0 Scope)**

- **Status:** **COMPLETED (v1.0.0 scope)** (`2026-11-23`, `5a6d85d` `x64 Release` binary `0C201BBAB000993B3E0F79E2096F228EB4F0BC4742374D318C46E6FEC9AE52D0`, source identity `685155b119431234f70280bc6778660636cd23fa3b21f68551fa2de20c29feaa`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`; see [PerformanceAndGpuQualification_2026-11-23](../../../Measurements/PerformanceAndGpuQualification_2026-11-23/README.md) and baseline [PerformanceAndGpuQualification_2026-11-21](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/README.md)).
- **Closes `RemainingWork` rows:** `T-P1` (`"paired-release-performance-and-warp-resource-qualification"` promoted to `supported` in [capabilities.json](../../../capabilities.json)).
- **Executed & verified evidence:**
  1. **Hardware GPU vs WARP resource lifecycle (`--resource-probe --control-diagnostics`, `5a6d85d`):** `NVIDIA GeForce RTX 5080` (`Compose` and `Split`) and `WARP` (`Compose`) at `144 DPI` (`2160x1350`, `16` cycles) all passed with `0` unrequested idle renders, `0` idle OS CPU ticks (`idleProcessCpu100ns = 0`), `resized-forced-frame` (`rootChildFlushes = 7`), `3x` bit-identical device-loss recovery (`generations: [4, 6, 8]`), `closed.nativeHosts == 0`, and `ownedHeapDiagnostics.complete == true` (`~3.88 MiB` HW / `~4.32 MiB` WARP busy CRT payload). A non-reproducing `2026-11-22` 29-pixel ClearType edge observation on `\begin{bmatrix}a&b\\c&d\end{bmatrix}` did not recur across `10/10` reruns or the serial Gate 6 run and is documented transparently in [PerformanceAndGpuQualification_2026-11-23](../../../Measurements/PerformanceAndGpuQualification_2026-11-23/README.md).
  2. **Visible-window DWM minimize/restore quiescence (`dwm-occlusion-probe.json`):** Minimizing `RedTilde.MainWindow` via `SW_MINIMIZE` for `2000 ms` recorded `minimizedCpuDeltaMs = 0`, unchanged working set/private bytes (`76,427,264` / `80,068,608` B), unchanged GDI/USER handles (`45` / `55`), `restoredVisibleAndNonIconic = true`, and `cleanCloseExitCode = 0`.
  3. **Observed A/A control & command/selection attribution (`observed-summary.json`):** `6` `ABBA`/`BAAB` blocks × `16` cycles × `2` views (`24` Hardware processes/view; `12/12` primary metrics `no-resolved-change`, `adjustedP = 1.0`, `controlsStable = true` on `workingSetBytes`, `observer.coverageComplete = true` with `maxGapSeconds = 1.65 <= 2.5 s`, shared-host ambient load `quietCpuWindowVerified = false`); recorded `app-edit-phases-v2` and `app-selection-components-v1` medians (`Compose commandUs = 241.22 us`, `Split commandUs = 212.8 us`).
  4. **Debug allocation-attribution design review ([debug-allocation-review.md](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/debug-allocation-review.md)):** Verified bounded `Application.Tests.HeapDiagnostic.h`, private ETW StackWalk joining/overflow rules, and preservation of the 29 Oct memory disposition.
- **Ongoing post-1.0 rules (not blocking `v1.0.0`):**
  1. Quiet-host (`quietCpuWindowVerified == true`) paired A/B candidate studies whenever future code changes touch hot paths or adopt API 6 (`H-01`/`H-02`/`H-05`/`H-14`).
  2. Live multi-monitor DPI-switch hardware verification on a multi-DPI desktop.

---

### Gate 7 — Upstream `DxUi` Sign-Off, v1.0.0 Cut, Signing Disposition & Release Handoff (`Q21`, `C11`/`C12`/`S6`) — **COMPLETED (`v1.0.0`, `2026-11-23`)**

- **Status:** **COMPLETED (`2026-11-23`)** — see [ReleaseV100_2026-11-23](../../../Measurements/ReleaseV100_2026-11-23/README.md), [local-qualification.json](../../../Measurements/ReleaseV100_2026-11-23/local-qualification.json), [installer-smoke.json](../../../Measurements/ReleaseV100_2026-11-23/installer-smoke.json), and [hosted-qualification.json](../../../Measurements/ReleaseV100_2026-11-23/hosted-qualification.json).
- **Closes `RemainingWork` rows:** `T-Q2` (`C11`/`C12`/`S6` release decisions), `Q21` release cut.
- **Owner scope decisions (Gates 3, 4 & 7 — `A1`–`A4`):**
  - `A1` / `A2`: Completed Gate 4 (`test-interactive.ps1` + live UIA/clipboard) and Gate 3 (`Microsoft Print to PDF` + WinRT verification + live printer config/DEVMODE/PrintTicket/preview without physical sheet consumption), with physical IME/Narrator/Touch and physical printer sheet inspection deferred post-1.0 in [capabilities.json](../../../capabilities.json).
  - `A3`: Shipped `v1.0.0` portable ZIPs (`x64` & `ARM64`) unsigned with verified `.zip.sha256` sidecars (`public-ca-authenticode-code-signing` listed under `deferred` in [capabilities.json](../../../capabilities.json)).
  - `A4`: Released `CHANGELOG.md` `1.0.0`, qualified hosted `x64` & `ARM64` `Qualify main Release`, archived receipts, moved both plans to `Specs/Plans/Done/`, and pushed annotated git tag `v1.0.0`.
- **Executed & verified evidence:**
  1. **Dependency pin alignment:** Shipped on pinned `DxUi` API 5 (`0c371a6197eb8c6b876b4b2843d75078bc21035c`) and pinned `RedMonitor` client header (`9ebea6577fe9a5ced7647283c918f51f67d666fd`); API 6 adoption (`H-01`/`H-02`/`H-05`/`H-14`) deferred post-1.0.
  2. **Version `1.0.0` transition (`PR #161`, `PR #163` `1f906912`, `PR #164` `1b8174b8`):**
     - Updated [Common/Version.h](../../../Common/Version.h) (`1.0`), `res/RedTilde.manifest`, `res/RedTilde.rc` (`IDS_ABOUT`), [.ci/project.json](../../../.ci/project.json) (`versionBase: "1.0"`), [vcpkg.json](../../../vcpkg.json) (`1.0.0`), [capabilities.json](../../../capabilities.json) (`status: "production"`, `version: "1.0.0"`, `pending: []`), [validate.ps1](../../../validate.ps1), `README.md`, `docs/`, and regenerated [`docs/screenshots/settings.png`](../../../docs/screenshots/settings.png).
     - All 8 local builds (`x64`/`ARM64` × `Release`/`Debug`/`ASan Debug`/`Release -ProductionOnly`) compiled with `0 Warning(s)`/`0 Error(s)`, Full `x64` `Release`/`Debug`/`ASan Debug` passed (`203/203` catalog cases each + `FailureReportRouting` + real ASan detection, with an earlier `20261123T085815666Z` `RecoveryService` teardown timeout transparently retained), and local `package.ps1` + sandboxed `Installer/Install-RedTilde.ps1` smoke passed.
  3. **Hosted `Qualify main Release` (`x64` & `ARM64` — Run [`#45167960343`](https://github.com/RedSalamanders/RedTilde/actions/runs/45167960343) on `1b8174b8`):**
     - All `7/7` jobs succeeded (`buildNumber: 538`, `version: 1.0.538`), with `203/203` regression cases on both `x64` and `ARM64` and `16/16` exit-`2` refusals on both `RedTilde-1.0.538-x64-20261123-120618-798.zip` (`1DF31F25FCFFBEBD094C3D8CA86EE6A7C53EDD859EB6CB261D9738D591DFA01F`) and `RedTilde-1.0.538-ARM64-20261123-040919-734.zip` (`0E3DE44A53317FDAD240D14236D289F2CEAF43D86BB83898A6F887B296E37C5C`). Earlier failed [`#45148102890`](https://github.com/RedSalamanders/RedTilde/actions/runs/45148102890) (`versionBase: "0.1"` mismatch on `c8ac8dd2`) is preserved transparently.
  4. **Changelog release & plan closeout:** Generated `## 1.0.0 - 2026-11-23` via [Tools/Release-Changelog.ps1](../../../Tools/Release-Changelog.ps1), moved `V1ReleaseReadiness_2026-11-21.md` and `RemainingWork_2026-11-04.md` to `Specs/Plans/Done/`, and tagged `v1.0.0`.

---

## Mapping to Remaining Items in `RemainingWork_2026-11-04.md`

| Section in `RemainingWork_2026-11-04.md` | Items & Disposition | Covered By |
| --- | --- | --- |
| `T-S2` | Live OneDrive, WSL 2, and SMB `selftest` qualification completed (`0` unexpected failures, `0` leftover dirs) | **Gate 5** (**COMPLETED**) |
| `T-U2` | Published live `Microsoft Print to PDF` & Windows driver/PrintTicket qualification receipts (physical sheet inspection deferred post-1.0) | **Gate 3** (**COMPLETED**) |
| `T-I1` | Q8/Q13 real-location acceptance completed across OneDrive, WSL 2, and SMB | **Gate 5** (**COMPLETED**) |
| `T-P1` | DWM minimize/restore quiescence (`0 ms` CPU), `NVIDIA GeForce RTX 5080` + `WARP` probes, 6-block observed A/A + attribution, Debug alloc design review (quiet-host A/B & multi-monitor DPI rules retained post-1.0) | **Gate 6** (**COMPLETED**) |
| `T-Q2` | ARM64 runtime (`Gate 2`), interactive focus/UIA/clipboard (`Gate 4`), and `C11`/`C12`/`S6` `v1.0.0` release cut (`Gate 7`) completed (physical IME/Narrator/Touch & Authenticode deferred in `capabilities.json`) | **Gates 2, 4, 7** (**COMPLETED**) |
| `T-CV1` | JSON5 settings, `SettingsWatcher` live reload, `_camelCase` + `.clang-tidy`, 155-line `.clang-format` completed (API-6 adoption waits for upstream `DxUi` sign-off / post-1.0) | **Gate 1** (**COMPLETED**) |
| `T-G1` | `PR-01`..`PR-46`/`S1`..`S16` & fold PRs completed; `Q15` shared-module migration & `Q17` coordinated privacy/history audit explicitly held outside v1.0.0 scope | **Transferred / held post-1.0** in [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md) |
