# Production Readiness Audit & Remediation Ledger (2026-11-11 – 2026-11-21)

## Metadata

- **Status**: `DONE`
- **Date**: `2026-11-21` (remediated and closed `2026-11-22`)
- **Audited commit range & state**: `63cd441..abaa0da` (10-day window across `2026-11-11` through `2026-11-21`, including `PrintPreview::WaitingOverlay`) and surrounding subsystems for side effects. Original file/line anchors below record the pre-remediation `abaa0da` baseline.
- **Companion active plans**: [Remaining work and qualification](RemainingWork_2026-11-04.md), [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md)
- **Authoritative contracts preserved / reconciled**:
  - [Specs/README.md](../../README.md)
  - [Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Core_Settings.md](../../Core/Core_Settings.md)
  - [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md)
  - [Core_CrashHandling.md](../../Core/Core_CrashHandling.md)
  - [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  - [Core_PerformanceAndResources.md](../../Core/Core_PerformanceAndResources.md)
  - [Editor_Markdown.md](../../Editor/Editor_Markdown.md)
  - [UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
  - [Build_Toolchain.md](../../Build/Build_Toolchain.md)
  - [Build_UnifiedCI.md](../../Build/Build_UnifiedCI.md)
  - [Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [AGENTS.md](../../../AGENTS.md)
  - [SettingsStore.schema.json](../../../SettingsStore.schema.json)
  - [capabilities.json](../../../capabilities.json)

> [!NOTE]
> Completed historical audit and remediation record under `Specs/Plans/Done/`. Durable behavioral requirements, invariants, and compatibility rules belong in the owning domain contracts under `Specs/<Domain>/`; see [the WIP index](../WIP/README.md) for active work and [capabilities.json](../../../capabilities.json) for current product capability status.

---

## 1. Production-Readiness Verdict & Verified-Clean Baseline

**Verdict (`2026-11-22` audit remediation closure): All audit-identified defects (`C1`–`C6`, `H1`–`H9`, `M1`–`M4`, `T1`–`T3`, `S1`–`S6`) are remediated and locally verified on Windows x64 (`Debug`, `Release`, `ASan Debug`) plus six-profile zero-warning compilation (`x64`/`ARM64` × `Debug`/`Release`/`ASan Debug`) and clean portable x64 packaging.**

Separate external, hardware, and cross-repository gates — physical ARM64 host execution, owner-authorized interactive desktop focus/IME/assistive-technology runs, physical/virtual printer driver testing, live OneDrive/SMB/WSL real-location storage runs, paired Release performance/resource qualification, and upstream `DxUi`/`RedMonitor` qualification — remain explicitly open in [capabilities.json](../../../capabilities.json), [Remaining work and qualification](RemainingWork_2026-11-04.md), and [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md).

### Verified-Clean Baseline (Audited & Re-verified)

- **H-27 `_camelCase` member naming & `.clang-tidy`**: Fixed the `.clang-tidy` `HeaderFilterRegex` (`H9`) so `.build/` and `ThirdParty/` headers are excluded. Live LLVM `clang-tidy.exe` (`22.1.3`) across all `29` first-party `.cpp` translation units reported `0` `readability-identifier-naming`, `0` `bugprone-reserved-identifier`, and `0` third-party header-filter leaks (`11/11` `Common/*.cpp` files exited `0`; the `18` `RedTilde/*.cpp` and `Tests/*.cpp` files reported `0` user diagnostics and hit only one Clang-vs-MSVC AST constraint inside pinned upstream `.build/dependencies/DxUi/source/0c371a6197eb8c6b/include/DxUi/DxUi.h:316` `MenuFlyoutItem::children`, recorded as an upstream `DxUi` follow-up). [Tooling.Tests.NamingStyle.ps1](../../../Tests/Tooling.Tests.NamingStyle.ps1) enforces `_camelCase`, `.clang-tidy`, and identifier hygiene (`91` checks across all `70` first-party C++ files).
- **Core diagnostic / work infrastructure**: [CrashHandler.cpp](../../../Common/CrashHandler.cpp), [DxUiFailureHost.cpp](../../../Common/DxUiFailureHost.cpp), [ProcessWork.cpp](../../../Common/ProcessWork.cpp), and [RemoteImage.cpp](../../../Common/RemoteImage.cpp) were audited and hold their zero-allocation fault-path, original-context dump, bounded teardown, and SSRF/DNS-pinning contracts.
- **Test inventories & resource tables**: [native-case-catalog.json](../../../Tests/native-case-catalog.json) (`188` cases including `8` added regression tests) and [NativeInventory.json](../../../Tests/NativeInventory.json) (`26` native test files) match C++ registrations and disk files 1:1. [Resource.h](../../../res/Resource.h) and [RedTilde.rc](../../../res/RedTilde.rc) (`#pragma code_page(65001)`) pass [Tooling.Tests.ResourceStrings.ps1](../../../Tests/Tooling.Tests.ResourceStrings.ps1) (`2956` checks across `308` base strings, zero orphans, zero missing/duplicate IDs, checked placeholder formatting), and compiled `FileVersionInfo` matches [Version.h](../../../Common/Version.h).
- **Schema field parity**: All `16` persisted settings keys, ranges, defaults, unknown-key preservation, `focusMode` omission, cold recovery, and `10` live-reloaded fields agree across [SettingsStore.schema.json](../../../SettingsStore.schema.json), [Settings.h](../../../Common/Settings.h), [Settings.cpp](../../../Common/Settings.cpp), [Application.cpp](../../../RedTilde/Application.cpp), and [Core_Settings.md](../../Core/Core_Settings.md).

### Summary of Remediated Findings

| Severity | Count | Categories |
| --- | --- | --- |
| **Critical / Blockers (`C1`–`C6`)** | 6 | Print Preview spurious-wake deadlock, modal reentrancy UAF crashes, JSON5 `NaN`/`Infinity` 0-byte settings wipe, 16 MiB vs. 266 KiB settings write budget mismatch, `Install-RedTilde.ps1` wipe/upgrade/`UninstallString` failures, `Test-RealLocations.ps1` unquoted-space write-policy violation |
| **High (`H1`–`H9`)** | 9 | Real printer driver `DEVMODEW`/forms/shutdown issues, PDF/XPS OneDrive & FAT32 failures, Files pane OneDrive rejection, unformatted `{0}` in `IDS_SAVEFAILED`, stuck-hidden `ClosingAfterSave`, `RecoveryService` `Stop()`/race/`.lease` bugs, settings live-reload & startup-comment races, `_host.Attach` returning `S_OK`, `.clang-tidy`/build/validation/packaging/security-guard breakages |
| **Medium (`M1`–`M4`)** | 4 groups | Protocol-2 failure-policy/`noexcept` violations, `WaitingOverlay` clip/freeze/COM bugs, application/windowing edge cases, installer/packaging/tooling defects |
| **Tests, Resources & Tooling (`T1`–`T3`)** | 3 groups | Printing test stack-UAF/tautology/busy-spin bugs, Settings test non-verifying assertions, unused `IDS_*`/accelerator/resource/naming/keyboard-state gaps |
| **Simplifications (`S1`–`S6`)** | 6 | Settings/Storage dedup, unnecessary 16 MiB copies & no-op saves, resource helper/ID cleanup, redundant shutdown recovery submit, `PrintPreview` helpers, dead CI/build/spec cleanup |

---

## 2. Critical / Release-Blocking Bugs (`C1`–`C6`)

- [x] **C1 — Spurious empty-request race permanently deadlocks Print Preview (`Printing.cpp`)**
  - **Locations**: [Printing.cpp](../../../RedTilde/Printing.cpp#L882-L954), [Printing.cpp](../../../RedTilde/Printing.cpp#L1253-L1297), [Printing.cpp](../../../RedTilde/Printing.cpp#L1365-L1422), [Printing.cpp](../../../RedTilde/Printing.cpp#L1464)
  - **Root cause**: `Operation::requested` is an auto-reset event (`wil::EventOptions::None`) and `Operation::request` was a non-optional `Request` whose default-constructed value was `{id = 0, printerEpoch = 1, page = 1}`.
  - **Resolution**: Changed `Operation::request` to `std::optional<Request>`, ignored wakes when `!operation.request`, bound `PrinterConfigProbe` completion by `(printerEpoch, printer)`, preserved in-flight printer probe waiting for same-epoch page requests inside `RunSession`, preserved pending printer/settings on `RequestPreview`, and added `Editor.PrintPreviewRapidSwitchWithPendingProbeDoesNotDeadlock` in [Application.Tests.Printing.cpp](../../../Tests/Application.Tests.Printing.cpp).

- [x] **C2 — Modal reentrancy use-after-free crashes across `PickFile`, `PickPrintOutput`, and error `MessageBoxW`**
  - **Locations**: [Application.h](../../../RedTilde/Application.h#L186-L192), [Application.cpp](../../../RedTilde/Application.cpp#L2235-L2263), [Application.cpp](../../../RedTilde/Application.cpp#L2585-L2639), [Application.cpp](../../../RedTilde/Application.cpp#L2721-L2741), [Application.cpp](../../../RedTilde/Application.cpp#L3024-L3029), [Application.cpp](../../../RedTilde/Application.cpp#L3525-L3528), [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp#L390-L402), [Application.Printing.cpp](../../../RedTilde/Application.Printing.cpp#L247-L267), [Application.Printing.cpp](../../../RedTilde/Application.Printing.cpp#L307-L312)
  - **Resolution**: Extended `ModalPromptKind` and scoped `_modalPrompt` guards across `PickFile`, `PickPrintOutput`, and error `MessageBoxW` dialogs (`CompleteFileOperation`, `ReportPrintFailure`), and re-verified `CanAcceptCommand()` / `IsUiAttached()` after every modal return before dereferencing controls or submitting operations.

- [x] **C3 — JSON5 `NaN`/`Infinity` in preserved unknown settings silently truncates `RedTilde.settings.json` to 0 bytes**
  - **Locations**: [Settings.cpp](../../../Common/Settings.cpp#L32), [Settings.cpp](../../../Common/Settings.cpp#L541-L562), [Settings.cpp](../../../Common/Settings.cpp#L639-L652), [Settings.cpp](../../../Common/Settings.cpp#L777-L778), [Application.cpp](../../../RedTilde/Application.cpp#L392), [Application.cpp](../../../RedTilde/Application.cpp#L4158)
  - **Resolution**: Checked serialization error propagation in `SerializeSettings`/`TrySerializeSettings`/`SaveSettings` so JSON5 `NaN`/`Infinity` in unknown fields never emits 0-byte output to `WriteBytesAtomic` (rejecting non-finite preserved numbers during validation/serialization), removed unconditional startup `_windowPlacementDirty = true` writes, updated [Core_Settings.md](../../Core/Core_Settings.md), and added regression coverage in [Application.Tests.Settings.cpp](../../../Tests/Application.Tests.Settings.cpp).

- [x] **C4 — Settings budget mismatch: 16 MiB read limit vs. 266 KiB atomic write limit**
  - **Locations**: [Settings.cpp](../../../Common/Settings.cpp#L31), [Settings.cpp](../../../Common/Settings.cpp#L778) vs. [Storage.cpp](../../../Common/Storage.cpp#L36), [Storage.cpp](../../../Common/Storage.cpp#L409-L410), [Storage.cpp](../../../Common/Storage.cpp#L514)
  - **Resolution**: Parameterized `Storage.cpp` `ReadBytes`/`WriteBytesAtomic` with explicit `maximumBytes`, passed `MaximumSettingsFileBytes` (`16 MiB`) from `Settings.cpp` while keeping document `MaximumBytes` as default (`S1`), and added `Settings.RejectsNonFiniteJson5AndHonorsLargeSettingsBudget` in [Application.Tests.Settings.cpp](../../../Tests/Application.Tests.Settings.cpp).

- [x] **C5 — `Installer/Install-RedTilde.ps1`: Destructive `-Uninstall` wipe, broken upgrades, and broken Windows 11 `UninstallString`**
  - **Locations**: [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1#L29-L63), [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1#L80), [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1#L104), [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1#L142-L171), [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1#L210-L211)
  - **Resolution**: Required `#Requires -Version 7` (plus Windows PowerShell / `pwsh` absolute-path handling in `UninstallString`), switched `IsPathRooted` to `IsPathFullyQualified`, blocked UNC/device-namespace/ancestor-of-user-data/known-shell/profile paths and source/target overlap, enforced verified install ownership before deleting `$targetDirectory`, changed CWD out before removal, removed registry/shortcuts after file removal succeeds, replaced nested `Copy-Item` upgrades with staged clean replacement, and added full Pester/tooling behavioral tests in [Tooling.Tests.Installer.ps1](../../../Tests/Tooling.Tests.Installer.ps1).

- [x] **C6 — `Tools/Test-RealLocations.ps1`: Unquoted `Start-Process -ArgumentList` breaks spaces in paths and violates fail-closed write policy**
  - **Locations**: [Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1#L466-L471), [Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1#L517-L521)
  - **Resolution**: Replaced both `Start-Process` calls with `Invoke-RedTildeHarness` (`ProcessStartInfo.ArgumentList`), preserving exact quoting for paths with spaces (e.g., `OneDrive - Personal`), and added fixture tests in [Tooling.Tests.RealLocations.ps1](../../../Tests/Tooling.Tests.RealLocations.ps1).

---

## 3. High-Severity Bugs & Production Side Effects (`H1`–`H9`)

- [x] **H1 — Real-world Windows printer driver bugs & shutdown crash on slow spooler submit (`Printing.cpp`)**
  - **Locations**: [Printing.cpp](../../../RedTilde/Printing.cpp#L412-L423), [Printing.cpp](../../../RedTilde/Printing.cpp#L488-L502), [Printing.cpp](../../../RedTilde/Printing.cpp#L528-L576), [Printing.cpp](../../../RedTilde/Printing.cpp#L855-L860), [Printing.cpp](../../../RedTilde/Printing.cpp#L1132-L1189), [Printing.cpp](../../../RedTilde/Printing.cpp#L1315-L1327), [Application.Printing.cpp](../../../RedTilde/Application.Printing.cpp#L228-L230)
  - **Resolution**: Used distinct input/output `DEVMODEW` buffers in `DocumentPropertiesW`, handled `DM_PAPERSIZE` vs `DM_FORMNAME` normalization and `ConfigurePrinter` form display, queried printer-supported forms via `DeviceCapabilitiesW` (`DC_PAPERNAMES` / `DC_PAPERS`), preserved existing valid printer configuration across transient timeout on settings change, guarded `DefaultPrinterName` against `length == 0`, moved blocking spooler submit off the bounded `ProcessWorkLease`, and updated [Editor_Markdown.md](../../Editor/Editor_Markdown.md).

- [x] **H2 — Printing to PDF/XPS fails on OneDrive Files On-Demand and FAT32/exFAT/older SMB (`Printing.cpp`)**
  - **Locations**: [Printing.cpp](../../../RedTilde/Printing.cpp#L670-L676), [Printing.cpp](../../../RedTilde/Printing.cpp#L715-L736) vs. [Storage.cpp](../../../Common/Storage.cpp#L55-L70), [Storage.cpp](../../../Common/Storage.cpp#L185-L196), [Storage.cpp](../../../Common/Storage.cpp#L720-L726)
  - **Resolution**: Allowed non-surrogate reparse placeholders via `FileAttributeTagInfo` + `IsReparseTagNameSurrogate`, added `FileIdInfo` fallback on `ERROR_INVALID_PARAMETER`/`ERROR_NOT_SUPPORTED`/`ERROR_INVALID_FUNCTION`, flushed file handle in `FileOutput::Publish`, and added protected replacement fallback when `ReplaceFileW` returns `ERROR_NOT_SUPPORTED`.

- [x] **H3 — Files pane rejects OneDrive Files On-Demand folders (`Common/Files.cpp`)**
  - **Locations**: [Files.cpp](../../../Common/Files.cpp#L116-L130), [Files.cpp](../../../Common/Files.cpp#L173), [Files.cpp](../../../Common/Files.cpp#L272)
  - **Resolution**: Applied consistent `IsReparseTagNameSurrogate` filtering across `Files.cpp` so surrogate symlinks/junctions remain blocked while cloud file placeholders traverse cleanly, and documented this in [Core_FileSafety.md](../../Core/Core_FileSafety.md).

- [x] **H4 — Unformatted `{0}` displayed on synchronous save submission failure (`Application.cpp`)**
  - **Locations**: [Application.cpp](../../../RedTilde/Application.cpp#L2786-L2791), [RedTilde.rc](../../../res/RedTilde.rc#L94)
  - **Resolution**: Formatted `IDS_SAVEFAILED` with `ErrorText(hr)` before calling `_feedback->SetText(...)` and covered synchronous save refusal in `Editor.SyncSaveRefusalAndCloseConflictUseLocalizedMessages` ([Application.Tests.Close.cpp](../../../Tests/Application.Tests.Close.cpp)).

- [x] **H5 — Permanent stuck-hidden window in `LifecyclePhase::ClosingAfterSave` on `TrySetDocumentFilePath` refusal (`Application.cpp`)**
  - **Locations**: [Application.cpp](../../../RedTilde/Application.cpp#L3066-L3071), [Application.cpp](../../../RedTilde/Application.cpp#L3527), [Application.cpp](../../../RedTilde/Application.cpp#L3548-L3550)
  - **Resolution**: Called `ResumeAfterSaveClose()` whenever post-save completion fails while `IsClosingAfterSave()`.

- [x] **H6 — `RecoveryService::Stop()` during `Scanning`, `candidates.size()` data race, and leaked `.lease` files (`Common/Recovery.cpp`)**
  - **Locations**: [Recovery.cpp](../../../Common/Recovery.cpp#L290-L294), [Recovery.cpp](../../../Common/Recovery.cpp#L336), [Recovery.cpp](../../../Common/Recovery.cpp#L338-L353), [Recovery.cpp](../../../Common/Recovery.cpp#L661), [Recovery.cpp](../../../Common/Recovery.cpp#L682), [Recovery.cpp](../../../Common/Recovery.cpp#L721-L731)
  - **Resolution**: Kept `scanComplete` orthogonal to `workerPhase` in `SettledLocked()`/`Ready()`, synchronized `candidates.size()` under `mutex`, removed created `session-<candidate>.lease` files after corrupt quarantine/read failure, and added `Recovery.StopDuringScanAndOrphanLeaseCleanup` ([Application.Tests.Files.cpp](../../../Tests/Application.Tests.Files.cpp)).

- [x] **H7 — Live settings reload races, unconditional startup comment stripping, and 1 s polling loop (`Application.cpp`, `Common/Settings.cpp`)**
  - **Locations**: [Application.cpp](../../../RedTilde/Application.cpp#L392), [Application.cpp](../../../RedTilde/Application.cpp#L1734-L1755), [Application.cpp](../../../RedTilde/Application.cpp#L4158), [Settings.cpp](../../../Common/Settings.cpp#L777-L778), [Settings.cpp](../../../Common/Settings.cpp#L830-L837), [Settings.cpp](../../../Common/Settings.cpp#L1060-L1101)
  - **Resolution**: Checked `_preferenceWriter.Busy()` during live reload, eliminated unconditional startup placement writes and skipped no-op canonical saves (`SaveSettings`) to preserve user JSON5 comments, retried transient `ERROR_RETRY` reads instead of treating them as malformed, replaced indefinite 1 s `WatchThread` retry polling on access-denied dirs with event-blocked handling, and updated [Core_Settings.md](../../Core/Core_Settings.md) and [Application.Tests.Settings.cpp](../../../Tests/Application.Tests.Settings.cpp).

- [x] **H8 — `_host.Attach` failure can return `S_OK` (`Application.cpp`, `PrintPreview.cpp`)**
  - **Locations**: [Application.cpp](../../../RedTilde/Application.cpp#L394-L395), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L505-L506)
  - **Resolution**: Guarded `GetLastError()` with fallback `E_FAIL` so failed `_host.Attach(...)` never returns `S_OK`.

- [x] **H9 — Build, validation, packaging & security-guard breakages (`.clang-tidy`, `validate.ps1`, `build.ps1`, `Test-RealLocations.ps1`, `package.ps1`)**
  - [x] Anchored `.clang-tidy` `HeaderFilterRegex` to direct `.h` headers under `Common/`, `RedTilde/`, and `Tests/` (including `Tests/Support/`) so `.build/` and `ThirdParty/` headers are excluded cleanly.
  - [x] Updated [Tooling.Tests.RealLocations.ps1](../../../Tests/Tooling.Tests.RealLocations.ps1) to select a current `testsEnabled -eq $true` binary rather than failing on a leftover `-ProductionOnly` Release binary right after `package.ps1`.
  - [x] Scoped `$env:RedTildeBuildNumber` and `$env:RedTildeEnableTests` in [build.ps1](../../../build.ps1) and [Invoke-ProjectCi.ps1](../../../Tools/Invoke-ProjectCi.ps1) using `try/finally` with `[NullString]::Value`, and scoped `RedTildeEnableTests='false'` around production artifact attestation in [package.ps1](../../../package.ps1).
  - [x] Blocked forward-slash `//?/`/`//./` device-namespace bypasses and OneDrive Known Folder Move (`%USERPROFILE%\Desktop`/`Documents`) paths on both raw and normalized paths in [Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1).
  - [x] Staged `Installer/Install-RedTilde.ps1` in [package.ps1](../../../package.ps1), rewrote staged portable `README.md` relative links (`docs/README.md`, `Installer/Install-RedTilde.ps1`, `capabilities.json`), and moved staged README link verification after all package assets/examples/DLLs are copied so `examples/README.md` and every relative link are verified inside `$stage`.

---

## 4. Medium-Severity Bugs, Failure-Policy Violations & `WaitingOverlay` Findings (`M1`–`M4`)

- [x] **M1 — Protocol-2 failure-policy / `noexcept` / bare `std::terminate()` violations**
  - [x] [Main.cpp](../../../RedTilde/Main.cpp): Switched `pending.txt` check to non-throwing `exists(..., error)`, routed OOM/unexpected runtime exceptions through `DxUi::Failure::FailAllocation()`/`FailUnexpected()` when `DxUi::Failure::IsConfigured()`, and separated CLI parsing errors from runtime execution.
  - [x] [Printing.cpp](../../../RedTilde/Printing.cpp): Wrapped `RunPrinterConfigProbe` in `COINIT_MULTITHREADED` + `DxUi::Failure::Scope` and eliminated throwing string copies from `RequestPreview` (`noexcept`).
  - [x] [Application.Chrome.cpp](../../../RedTilde/Application.Chrome.cpp): Replaced `std::terminate()` on `E_OUTOFMEMORY` with `DxUi::Failure::FailAllocation()`.
  - [x] [Settings.cpp](../../../Common/Settings.cpp): Moved pending settings in `SettingsWriter::State::Worker` and added `bad_alloc` -> `FailAllocation()` + `std::exception` -> `FailUnexpected()`.
  - [x] [Recovery.cpp](../../../Common/Recovery.cpp) and [Files.cpp](../../../Common/Files.cpp): Added `catch (const std::exception&)` -> `DxUi::Failure::FailUnexpected()` in workers.
  - [x] [BitmapPaste.h](../../../RedTilde/BitmapPaste.h): Pre-allocated `retireJob` in `Initialize()`, moved DIB copy outside `_state->mutex`, made `Stop() noexcept` allocation-free via out-of-line `~State()`, and wired worker exception handling to `DxUi::Failure`.

- [x] **M2 — `PrintPreview::WaitingOverlay` defects (`abaa0da`)**
  - **Locations**: [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L77), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L184-L251), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L316-L326), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L438-L439), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L616-L617), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L854-L865), [PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp#L1005-L1006)
  - **Resolution**: Added `PushAxisAlignedClip` in `WaitingOverlay::Paint`, ordered `Layout()` before `BeginRendering()`, re-requested animation in `Layout()` when waiting overlay is visible without reduced motion, and replaced throwing `wil::com_ptr` with `wil::com_ptr_nothrow` in `Sheet` and `WaitingOverlay`.

- [x] **M3 — Additional Application & Windowing Defects**
  - [x] Deferred `ClosePrintPreview()` destruction on `WM_NCDESTROY` to `PrintPreview::WindowProc` after `HandleMessage` returns ([PrintPreview.cpp](../../../RedTilde/PrintPreview.cpp), [Application.Printing.cpp](../../../RedTilde/Application.Printing.cpp)).
  - [x] Localized `IDS_CONFLICTHELP` conflict feedback on `ERROR_REVISION_MISMATCH` during `ClosingAfterSave` ([Application.cpp](../../../RedTilde/Application.cpp)) and verified with `Editor.SyncSaveRefusalAndCloseConflictUseLocalizedMessages`.
  - [x] Filtered drop extension before `GetFileAttributesW` in `OpenDroppedFile`, checked `_printPreview` on `WM_DROPFILES`, and released `HDROP` before modal `AskWorkspaceThen` prompts ([Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp), [Application.cpp](../../../RedTilde/Application.cpp)); covered by `Editor.DragDropOpensMarkdownAndRejectsInvalidDrops`.
  - [x] Passed skip flags to `SwitchWorkspaceDocument` in `CloseActiveDocument()` so discarding a dirty slot avoids redundant recovery writes and double `RefreshFiles()` calls ([Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp)).
  - [x] Cleaned up auto-created `%TEMP%\RedTilde-Harness-*` directories on exit while preserving crash artifacts when crash artifacts exist ([Main.cpp](../../../RedTilde/Main.cpp)).
  - [x] Blocked duplicate external link activation during `CompletedPendingDrain` ([Application.cpp](../../../RedTilde/Application.cpp)).
  - [x] Handled CRLF line endings in `ParseLegacySettings` ([Settings.cpp](../../../Common/Settings.cpp)).

- [x] **M4 — Installer, Packaging, Build & CI Tooling Defects**
  - [x] Completed `Applications\RedTilde.exe\shell\open\command`, `SHChangeNotify(SHCNE_ASSOCCHANGED, ...)`, Registry `InstallLocation`-based `-Uninstall`, stale-association cleanup across custom path upgrades, and architecture-aware `Resolve-RedTildeSourceDirectory` in [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1).
  - [x] Added `vcpkgSha256` attestation to `Get-RedTildeSdkBuildInputSnapshot` in [Harness.psm1](../../../Build/Harness.psm1).
  - [x] Fixed UTF-8/`-z` filename handling and `$LASTEXITCODE` checking in `Get-RedTildeSourceIdentity` ([Harness.psm1](../../../Build/Harness.psm1)).
  - [x] Sorted VC redist folders by `[version]` in [package.ps1](../../../package.ps1).
  - [x] Split comma-separated `-Suites` before Local SDK validation in [test.ps1](../../../test.ps1).
  - [x] Added `Test-Path`, `-version '[18.0,19.0)'`, and safer log timestamps in [.github/actions/setup-msvc/Setup.ps1](../../../.github/actions/setup-msvc/Setup.ps1).
  - [x] Invoked `vcpkg-install.ps1` in [Build-LocalSdkCandidate.ps1](../../../Tools/Build-LocalSdkCandidate.ps1).
  - [x] Fixed `Process` handle disposal/`WaitForExit()`, preserved logs on exceptions, checked dangling reparse links, and verified `passed == selected && !catalogOnly` in [Test-RealLocations.ps1](../../../Tools/Test-RealLocations.ps1).
  - [x] Verified clean worktree before checkout, baseline consistency (`vcpkg.json` == `vcpkg-tool.json`), `--vcpkg-root`, `[NullString]::Value` env isolation, and unshallow handling in [vcpkg-install.ps1](../../../vcpkg-install.ps1) and [VcpkgToolCheckout.psm1](../../../Build/VcpkgToolCheckout.psm1).

---

## 5. Test Suite, Resource & Tooling Gate Defects (`T1`–`T3`)

- [x] **T1 — `Tests/Application.Tests.Printing.cpp` stack-UAF, null-deref, non-falsifiability & coverage gaps**
  - **Resolution**: Moved async fixture state into `std::shared_ptr` with `wil::scope_exit` event signaling and `std::atomic_bool` flags, guarded `app._printPreview != nullptr` in `Wait`, pumped preview messages cleanly in waiting overlay tests, replaced spinning loops and added real `PrintPreview` captures + static invariance checks, added `ForegroundOwnershipObserver` + `!IsWindowVisible` checks, isolated `PrintingProbe::Wheel` from live physical desktop modifiers via `KeyboardState::ScopedSynthetic`, and added `Editor.PrintPreviewRapidSwitchWithPendingProbeDoesNotDeadlock`, `Editor.PrintPageRangesAndKeyboardShortcuts`, and `Editor.PrintPreviewEmptyPrintersProbeCapAndRememberImage` to [Application.Tests.Printing.cpp](../../../Tests/Application.Tests.Printing.cpp) and [native-case-catalog.json](../../../Tests/native-case-catalog.json).

- [x] **T2 — `Tests/Application.Tests.Settings.cpp` & installer behavioral test gaps**
  - **Resolution**: Enabled `_options.watchSettingsForTest` initialization in [Application.cpp](../../../RedTilde/Application.cpp), replaced blind sleeps with deterministic watcher events, exercised self-save stamp deduplication outside the coalesce window, verified no-op save JSON5 comment preservation, cleaned up large temporary fixtures, tested JSON5 `NaN`/`Infinity` and `>266 KiB`..`16 MiB` budgets in `Settings.RejectsNonFiniteJson5AndHonorsLargeSettingsBudget` ([Application.Tests.Settings.cpp](../../../Tests/Application.Tests.Settings.cpp)), and added comprehensive Pester/tooling behavioral coverage for [Install-RedTilde.ps1](../../../Installer/Install-RedTilde.ps1) in [Tooling.Tests.Installer.ps1](../../../Tests/Tooling.Tests.Installer.ps1).

- [x] **T3 — Resource strings, naming gates & synthetic keyboard state gaps**
  - **Resolution**: Pruned `17` unused `IDS_*` definitions and added `11` localized `IDS_SHORTCUT_*` strings to [Resource.h](../../../res/Resource.h) and [RedTilde.rc](../../../res/RedTilde.rc), set `VS_FF_PRERELEASE`, aligned `IDS_FINDNEXT` (`"Next · F3"`), added `-?` to `IDS_COMMANDLINEHELP`, used `IDS_ABOUTVERSION` in `WriteCommandVersion`, switched ASCII punctuation to Unicode `…`/`’`, avoided flashing unformatted `{0}` placeholders in [Application.Chrome.cpp](../../../RedTilde/Application.Chrome.cpp), replaced menu `+ offset` with explicit `std::array` resource tables, added `Resource.h -> RC` orphan + strict UTF-8 + unformatted placeholder checks in [Tooling.Tests.ResourceStrings.ps1](../../../Tests/Tooling.Tests.ResourceStrings.ps1), tightened C++14 digit-separator & member/reserved-identifier parsing in [Tooling.Tests.NamingStyle.ps1](../../../Tests/Tooling.Tests.NamingStyle.ps1), and saved/restored Win32 `GetKeyboardState`/`SetKeyboardState` in `KeyboardState::ScopedSynthetic` ([KeyboardState.h](../../../RedTilde/KeyboardState.h)).

---

## 6. Simplification & Deduplication (`S1`–`S6`)

- [x] **S1 — Deduplicate file-reading & identity helpers in `Common/Settings.cpp`**: Consolidated duplicate file-I/O and identity logic onto parameterized `Storage.cpp` `ReadBytes`/`WriteBytesAtomic` helpers.
- [x] **S2 — Eliminate double `16 MiB` copies and no-op writes in `Common/Settings.cpp`**: Removed redundant full-buffer copies before `yyjson_read_opts` and skipped `WriteBytesAtomic` when normalized serialized content matches existing settings.
- [x] **S3 — Prune unused `IDS_*` IDs and explicit shortcut resource tables**: Removed `17` unused resource IDs while preserving the `Common/Resources.h` helper contract tested by [Application.Tests.Core.cpp](../../../Tests/Application.Tests.Core.cpp).
- [x] **S4 — Remove redundant shutdown recovery call in `BeginClose()`**: Relied on `ResetWorkspace()` retirement instead of submitting a duplicate empty snapshot.
- [x] **S5 — Simplify `PrintPreview.cpp` string & waiting-text helpers**: Fixed `{name, name}` copy and extracted `BuildWaitingCopy` for `ShowWaitingOverlay()` / `SyncWaitingText()`.
- [x] **S6 — Prune dead/duplicated CI, MSBuild, tooling, and spec lines**: Cleaned duplicate `Invoke-ProjectCi.ps1`, `Directory.Build.props`, `build.ps1`, and `validate.ps1` branches, included `.github/actions/setup-msvc/Setup.ps1` in PowerShell syntax validation, and reconciled [Core_Settings.md](../../Core/Core_Settings.md), [Core_FileSafety.md](../../Core/Core_FileSafety.md), [Build_Toolchain.md](../../Build/Build_Toolchain.md), and [Testing_Validation.md](../../Testing/Testing_Validation.md).

---

## 7. Sequenced Remediation Waves (Completed)

- **Wave 1 (`C1`–`C6`, `S1`–`S2`)**: Complete.
- **Wave 2 (`H1`–`H9`, `M4`, `S4`, `S6`)**: Complete.
- **Wave 3 (`M1`–`M3`, `T1`–`T3`, `S3`, `S5`) & Wave 4 Spec Reconciliation**: Complete.

---

## 8. Verification Receipt (2026-11-22)

1. **All 6 Native Builds (`0 warning(s), 0 error(s)`)** — Visual Studio 18 Insiders, MSVC toolset `14.52.37025`, Windows SDK `10.0.26100.0`, pinned `DxUi` commit `0c371a6197eb8c6b876b4b2843d75078bc21035c` (API 5):
   - `x64 Debug`, `x64 Release`, `x64 ASan Debug`
   - `ARM64 Debug`, `ARM64 Release`, `ARM64 ASan Debug` (cross-compiled cleanly; ARM64 runtime remains gated on physical ARM64 hardware)
2. **All 3 Complete `x64 -Full` Harness Runs (`test.ps1 -Platform x64 -Full`)**:
   - **`x64 Debug`**: `188/188` behavioral cases passed (`Storage` 32, `Settings` 18, `Recovery` 23, `EditorContent` 56, `EditorWorkspace` 25, `EditorMedia` 28, `EditorPerformance` 3, `EditorRendering` 3); failure-report self-test exit `3`; forced render `125,2 FPS; private memory 80,2 MiB; dirty edit/render 1,83 ms`; `40` actual hidden captures with viewport/DPI provenance; isolated local crash dump and marker verified.
   - **`x64 Release`**: `188/188` behavioral cases passed; failure-report self-test exit `0`; forced render `124,7 FPS; private memory 68,7 MiB; dirty edit/render 0,13 ms`; `40` actual hidden captures; isolated local crash dump and marker verified.
   - **`x64 ASan Debug`**: `188/188` behavioral cases passed; failure-report self-test exit `3`; forced render `125,3 FPS; private memory 357,7 MiB; dirty edit/render 5,12 ms`; `40` actual hidden captures; isolated local crash dump and marker verified; `Real AddressSanitizer heap-use-after-free detection verified`.
3. **Static Analysis & Naming Style**:
   - Live LLVM `clang-tidy.exe` `22.1.3` with `--config-file=.clang-tidy` across all `29` first-party `.cpp` files: `0` `readability-identifier-naming`, `0` `bugprone-reserved-identifier`, and `0` `.build/` or `ThirdParty/` header-filter leaks (`11/11` `Common/*.cpp` exited `0`; the `18` `RedTilde/*.cpp` and `Tests/*.cpp` files hit only one Clang-vs-MSVC AST constraint inside pinned upstream `.build/dependencies/DxUi/source/0c371a6197eb8c6b/include/DxUi/DxUi.h:316` `MenuFlyoutItem::children`, logged as an upstream `DxUi` follow-up).
   - `Tests/Tooling.Tests.NamingStyle.ps1`: `PASS: naming style (91 checks, 70 C++ files)`.
   - `Tests/Tooling.Tests.ResourceStrings.ps1`: `PASS: resource strings (2956 checks, 308 base strings)`.
4. **Portable Production Packaging & Final Gates**:
   - `.\package.ps1 -Platform x64`: built zero-warning `-ProductionOnly` Release binary, staged `Installer/Install-RedTilde.ps1`, validated all staged `README.md` relative links after full asset copy (`examples/README.md`, `Installer/Install-RedTilde.ps1`, `Diagnostics.md`, `docs/README.md`, `capabilities.json`), generated `.build\package\RedTilde-0.1.0-x64-20261122-124356-504.zip` + `.sha256` + receipt, passed clean-extraction smoke (`--help`, `--version`, `--capture`), and verified all `16` test-only entry-point refusals (`exit 2`, no profile directory created).
   - Immediate post-package `.\validate.ps1` passed cleanly, and test-enabled `x64 Release` was rebuilt (`0 warning(s), 0 error(s)`).
   - Final read-only gates: `.\format.ps1 -Check` (`clang-format 22.1.3`), `.\Tools\Test-Changelog.ps1`, and `.\validate.ps1` passed (`exit 0`).
