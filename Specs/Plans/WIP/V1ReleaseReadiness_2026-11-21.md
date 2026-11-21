# RedTilde Version 1.0 Release Readiness Plan

## Metadata

- **Status**: `ACTIVE`
- **Date**: 2026-11-21
- **Baseline commit**: `b7c31920b8bb9f1c0952d6b6f152978328137058` (`main` after PR #145)
- **Companion active plan**: [RemainingWork_2026-11-04.md](RemainingWork_2026-11-04.md) (retains broader governance/tooling items including `Q17`, `Q15`, rejected-cleanup dispositions, and routine post-merge changelog folds alongside the open engineering gates)
- **Archived predecessor**: [ExplicitStateMachines_2026-11-10.md](../Done/ExplicitStateMachines_2026-11-10.md) (`DONE`)
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

### Gate 3 — Physical Printer & Virtual Driver Qualification (`T-U2`) (**OPEN**)

- **Closes `RemainingWork` rows:** `T-U2` step 6 (**Publish the qualified behavior**).
- **Host printers observed on development machine (2026-11-21):**
  - Physical hardware IPP device: `Brother HL-L2445DW Printer` (`Microsoft IPP Class Driver`)
  - Virtual PDF driver: `Microsoft Print to PDF` (`PORTPROMPT:`)
  - Virtual document printer: `OneNote (Desktop)` / `Microsoft Virtual Print Class Driver`
  - Optional: enable `Microsoft XPS Document Writer` if multi-driver XPS validation is required.
- **Execution:**
  1. Print multi-page showcase Markdown containing headings, Unicode/CJK/emoji, inline & display LaTeX equations, native Mermaid diagrams, tables, and local images to `Microsoft Print to PDF` (inspect generated PDF pagination, page ranges `1, 3-4`, `Current page`, Portrait & Landscape, Letter & A4, copies/collation, cancellation mid-job).
  2. Run physical sheet verification on `Brother HL-L2445DW Printer` (duplex one-sided / long-edge / short-edge, physical paper fidelity).
  3. Verify second virtual destination (`OneNote (Desktop)` or XPS) and agree on desktop timing to verify live OS `Print` / `Save Print Output As` modal focus restoration.
  4. Register any new deterministic printing regression cases in [Tests/NativeInventory.json](../../../Tests/NativeInventory.json) and [Tests/native-case-catalog.json](../../../Tests/native-case-catalog.json).
- **Verification & exit criteria:**
  - Source-, OS-, driver-, and paper-bound evidence archived under [Measurements/](../../../Measurements/README.md); update [Editor_Markdown.md](../../Editor/Editor_Markdown.md), [UI_DesignSystem.md](../../UI/UI_DesignSystem.md), and [capabilities.json](../../../capabilities.json).

---

### Gate 4 — Physical Input, Assistive Technology & Agreed Interactive Focus (`T-Q2`) (**OPEN**)

- **Closes `RemainingWork` rows:** `T-Q2` clipboard, physical IME, UIA/AT, touch, and interactive focus qualification.
- **Entry criteria:** Explicit timing agreement with the person at the desktop (`Silence never authorizes focus-taking tests`).
- **Execution:**
  1. Run [test-interactive.ps1](../../../test-interactive.ps1) (canonical warning banner, desktop lease, owned-window target only, verified foreground/focus/pointer restoration on pass or abort).
  2. Execute supervised manual/structured scenarios for:
     - **Clipboard:** mixed Unicode text + bitmap paste transitions under both text-first and image-first preferences, plus RichText/Source copy-paste fidelity.
     - **Physical IME composition:** East Asian IME (Japanese/Chinese) and dead-key sequences across Compose, Source, Split, Find bar, and `F2` local-source boxes (ensuring no lost preedit text or outline flicker during composition).
     - **UI Automation & Screen Reader:** Narrator/Inspect walk of caption bar, command row, selection floating toolbar (`Alt+F10`), Files tree (including `Open documents`), Outline badges, Find bar, Recovery modal, and Settings (`About & Shortcuts`).
     - **Touch / Pen (if hardware present):** touch scroll/tap target responsiveness and focus preservation.
- **Verification & exit criteria:**
  - Archive source-bound interactive, IME, clipboard, and AT verification packet under [Measurements/](../../../Measurements/README.md).

---

### Gate 5 — Real Storage & Network Share Qualification (`T-S2`, `T-I1`) (**OPEN — PARTIAL: LIVE SMB EXECUTED WITH OPEN `Common/Storage.cpp` DEFECTS**)

- **Closes `RemainingWork` rows:** `T-S2` (real OneDrive/cloud, SMB/NAS, WSL qualification), `T-I1` (Q8/Q13 real-location acceptance).
- **Host storage resources observed on 2026-11-21 (inventory only — never direct write targets):**
  - **OneDrive Consumer:** `C:\Users\eric\OneDrive` (`$env:OneDriveConsumer`, present locally; awaits owner-declared `selftest` root)
  - **OneDrive Business:** `C:\Users\eric\OneDrive - QlikTech Inc` (`$env:OneDriveCommercial`, present locally; awaits owner-declared `selftest` root)
  - **WSL 2:** `Ubuntu` distro installed (Version 2; UNC root `\\wsl.localhost\Ubuntu\...`; awaits owner-declared `selftest` root)
  - **Dedicated SMB / NAS Self-Test Share:** `\\rubygloom\RedSalamander-SelfTest\selftest` declared in `Z:\RedSalamander.Perf\config\machine-resources.json` under id `path.smb.primary` (separate from media share `\\rubygloom\Multimedia`; **reachable and executed on `ea111980`**)
- **Safety & isolation design (Allowed Write Locations Only — Fail Closed):**
  1. **Implemented & executed locally:** [Tools/Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1) (opt-in; NEVER invoked automatically by default headless runs or hosted CI; now extended with `-IncludeNativeSuites`, `-NativeScopes`, `-RetainLogs`, `Start-Process -Wait` timeouts, and read-only-safe per-run cleanup) and its compiler-free gate [Tests/Tooling.Tests.RealLocations.ps1](../../../Tests/Tooling.Tests.RealLocations.ps1) (`41` checks in [validate.ps1](../../../validate.ps1)) enforce the fail-closed contract:
     - Live OneDrive, SMB, and WSL tests ONLY run against explicitly manifest-declared dedicated `selftest` roots (`redtilde.real-locations.v1` or compatible `red-salamander.test-machine-resources.v1` entries, each required to contain a `selftest` path segment and never pointing to drive roots, `%USERPROFILE%`, Desktop, Documents, undeclared `C:\Users\...\OneDrive`, or `\\rubygloom\Multimedia`). Fail closed if a manifest or target root is missing, relative, >1024 chars, contains NUL/newlines or `.`/`..` segments, is a shallow 2-segment UNC root, lacks a `selftest` segment, or is/traverses a surrogate symlink/junction.
     - Perform explicit reachability preflight per target and operate strictly inside a per-run `redtilde-run-<guid>` subfolder with exit cleanup in `finally` that never follows reparse points.
  2. **Live SMB qualification executed (`ea111980`, [RealLocationSmb_2026-11-21](../../../Measurements/RealLocationSmb_2026-11-21/README.md)):**
     - **Passed:** UTF-8 BOM `--capture` (`118,136`-byte PNG, unchanged document SHA-256 `1BA77DC7...`), `Settings` (`15/15`), `Recovery` (`22/22`), and per-run directory cleanup (`0` leftover `redtilde-run-*` directories).
     - **Failed (`Storage` `16/32` passed, `16` failed) — 4 proven Win32/SMB2 root causes diagnosed:**
       1. *Product SMB bug:* `Common/Storage.cpp` samples `writtenStamp` before `MoveFileExW(..., MOVEFILE_WRITE_THROUGH)` / `ReplaceFileW`, whereas the SMB server updates `ftLastWriteTime` post-rename (`+503,718` 100-ns ticks ≈ `50.37 ms`) while keeping volume serial and file index identical.
       2. *Product SMB bug:* `ReplaceFileW(..., 0)` and `ReplaceFileW(..., REPLACEFILE_IGNORE_MERGE_ERRORS)` both fail with `ERROR_PATH_NOT_FOUND` (`3`) on an existing SMB target, which `Common/Storage.cpp` does not route to the protected fallback.
       3. *SMB server capability & fixture limits:* `CreateHardLinkW` returns `TRUE` while `GetFileInformationByHandle` still reports `nNumberOfLinks == 1`; `GetFileInformationByHandleEx(FileIdInfo)` returns error `87` (`Storage.cpp` already falls back, but `Storage.StampMatchesFullNativeFileIdentity` asserts `FileIdInfo`); remote junction/symlink fixture creation (`Win32 5`) is denied by the share.
       4. *Intentional local-profile boundary:* `Common/BitmapAssets.cpp` intentionally rejects `DRIVE_REMOTE` (`4`) for `--data-dir` clipboard PNG staging (`Assets` belong under `%LOCALAPPDATA%` on `DRIVE_FIXED`).
  3. **Remaining Wave 2 work to close Gate 5 (`T-S2`, `T-I1`):**
     - Implement and qualify the Wave 2 `Common/Storage.cpp` SMB fixes (refresh/handle post-rename `ftLastWriteTime` when identity/bytes match; route `ReplaceFileW` `ERROR_PATH_NOT_FOUND` fallback safely; keep local `DRIVE_FIXED` data root in SMB `BitmapAssets` tests and separate SMB server capability probes from core save regressions) and re-qualify `path.smb.primary` to `0` unexpected failures.
     - Obtain owner-declared dedicated `selftest` roots for OneDrive Personal/Business (`CfApi` non-name-surrogate cloud placeholders) and WSL 2 `Ubuntu` and archive clean live receipts across all three backends, plus update [capabilities.json](../../../capabilities.json) and [Core_FileSafety.md](../../Core/Core_FileSafety.md).

---

### Gate 6 — Paired Quiet-Host Performance & Real Hardware Graphics (`T-P1`) (**OPEN — PARTIAL: HARDWARE GPU PROBE, OBSERVED A/A CONTROL & DESIGN REVIEW RECORDED**)

- **Closes `RemainingWork` rows:** `T-P1` (attribution, Debug allocation-attribution design review, matched Release studies, long-lived GPU/hardware graphics).
- **Executed & verified evidence ([PerformanceAndGpuQualification_2026-11-21](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/README.md)):**
  1. **Hardware GPU vs WARP resource lifecycle (`--resource-probe --control-diagnostics`, `ea111980` `x64 Release`):** `NVIDIA GeForce RTX 5080` (`Compose` and `Split`, `144 DPI`, `2160x1350`, `16` cycles) and `WARP` (`Compose`) both pass `0` unrequested idle renders, `0` idle OS CPU ticks (`idleProcessCpu100ns = 0`), `3x` bit-identical device-loss recovery (`generations: [4, 6, 8]`), `closed.nativeHosts == 0`, and `ownedHeapDiagnostics.complete == true` (`~3.83 MiB` CRT/default busy payload vs `~43 MiB` GPU/WDDM driver delta on hardware).
  2. **Observed A/A control & command/WS attribution:** `6` `ABBA`/`BAAB` blocks × `16` cycles × `2` views (`12/12` primary metrics `no-resolved-change`, `coverageComplete: true`, `maxGap <= 1.04 s`); recorded `app-edit-phases-v2` and `app-selection-components-v1` medians (`Compose commandUs = 122.0 us`: `caption 39.3 us`, `identity 20.7 us`, `selection 19.1 us`, `analysis 12.5 us`; `Split commandUs = 113.0 us`).
  3. **Debug allocation-attribution design review ([debug-allocation-review.md](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/debug-allocation-review.md)):** Verified bounded `Application.Tests.HeapDiagnostic.h`, private ETW StackWalk joining/overflow rules, and preservation of the 29 Oct memory disposition.
- **Remaining open to close Gate 6 (`T-P1`):**
  1. Quiet-host (`quietCpuWindowVerified == true`) paired A/B candidate studies whenever hot paths or API 6 (`H-01`/`H-02`/`H-05`/`H-14`) change code.
  2. Physical DWM occlusion and live monitor DPI switch verification (explicitly separate from hidden `WM_SHOWWINDOW`, simulated `--capture-dpi`, and `DebugSimulateDeviceLoss()`).

---

### Gate 7 — Upstream `DxUi` Sign-Off, v1.0.0 Cut, Signing & Release Handoff (`Q21`, `C11`/`C12`/`S6`) (**OPEN**)

- **Closes `RemainingWork` rows:** `T-Q2` (`C11`/`C12`/`S6` release decisions), `Q21` release cut.
- **Entry criteria:** Gates 1–6 closed (or explicit owner waiver recorded in [RemainingWork_2026-11-04.md](RemainingWork_2026-11-04.md) and [capabilities.json](../../../capabilities.json)).
- **Execution:**
  1. **Upstream dependency alignment:** Confirm upstream `DxUi` qualification sign-off and decide final shipped `DxUi` pin (API 5 or API 6 per RedHome D20) and pinned `RedMonitor` `Client.h` hash.
  2. **Version 1.0.0 transition:**
     - Flip `REDTILDE_VERSION_MAJOR 1`, `REDTILDE_VERSION_MINOR 0`, and empty `REDTILDE_VERSION_SUFFIX` / `REDTILDE_VERSION_SUFFIX_WIDE` in [Common/Version.h](../../../Common/Version.h).
     - Transition [capabilities.json](../../../capabilities.json) and [validate.ps1](../../../validate.ps1) from `experimental` to `production` truthfully with remaining non-v1.0 deferrals cleanly listed under `deferred`.
     - Update [docs/README.md](../../../docs/README.md), [docs/Portable.md](../../../docs/Portable.md), [README.md](../../../README.md), and regenerate Settings / showcase screenshots if visible version strings changed.
  3. **Final release build, signing & changelog release:**
     - Pass hosted CI main qualification across all 6 profiles plus production x64 & ARM64 packages (`package.ps1`).
     - Apply Authenticode code signing to release executables and installer scripts (if distributed publicly) and regenerate `.sha256` sidecars / verification receipts.
     - Fold remaining `Changes/` fragments via post-merge fold PR and generate the `1.0.0` section using [Tools/Release-Changelog.ps1](../../../Tools/Release-Changelog.ps1) (never hand-editing).
     - Verify end-to-end clean-machine install, Start Menu launch, optional `.md`/`.markdown` HKCU association, and clean uninstall (preserving `%LOCALAPPDATA%\RedTilde`) via [Installer/Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1).
- **Verification & exit criteria:**
  - Signed v1.0.0 release ZIPs (x64 & ARM64), final receipts archived in `Measurements/`, git tag / GitHub Release created, and `RemainingWork_2026-11-04.md` + `V1ReleaseReadiness_2026-11-21.md` moved to `Specs/Plans/Done/`.

---

## Mapping to Remaining Items in `RemainingWork_2026-11-04.md`

| Section in `RemainingWork_2026-11-04.md` | Remaining Unchecked Items | Covered Here By |
| --- | --- | --- |
| `T-S2` (1 item) | Wave 2 SMB `Common/Storage.cpp` fixes & re-run, plus OneDrive/cloud and WSL `selftest` qualification | **Gate 5** (`PARTIAL` — SMB executed) |
| `T-U2` (1 item) | Step 6: publish qualified physical/PDF/driver printing behavior & receipts | **Gate 3** |
| `T-I1` (1 item) | Q8/Q13 real-location acceptance | **Gate 5** (`PARTIAL` — SMB executed) |
| `T-P1` (3 remaining open items; Debug alloc design review checked) | Quiet-host A/B matched Release studies, command/WS optimization follow-ups, physical DWM occlusion/DPI switch | **Gate 6** (`PARTIAL` — GPU probe, A/A & design review done) |
| `T-Q2` (2 items: physical IME/AT/touch + `C11`/`C12`/`S6` release cut; ARM64 runtime completed) | Clipboard/IME/AT/touch + `C11`/`C12`/`S6` release cut | **Gate 2** (**COMPLETED**), **Gate 4**, **Gate 7** |
| `T-CV1` (0 remaining items; `H-10`, `H-15`, `H-26`, `H-27`, `CV-6-05` completed locally and qualified in hosted Release & ARM64) | JSON5 settings, `SettingsWatcher` live reload, JSON settings filenames/`Core_Settings.md`, `_camelCase` + `.clang-tidy`, 155-line `.clang-format` (API-6 `H-01`/`H-02`/`H-05`/`H-14` wait for RedHome D20 / Gate 7) | **Gate 1** (**COMPLETED**) |
| `T-G1` (2 open items: `Q15` shared-module migration, `Q17` coordinated privacy/history audit; dispositions & PR #147 fold completed) | `Q17` coordinated privacy/history audit, `Q15` historical replay migration | Tracked in [RemainingWork_2026-11-04.md](RemainingWork_2026-11-04.md) and [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md) |
