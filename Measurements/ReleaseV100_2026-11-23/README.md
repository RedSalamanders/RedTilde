# RedTilde v1.0.0 Production Release Qualification — 23 November 2026

Ledgers: [local-qualification.json](local-qualification.json) · [installer-smoke.json](installer-smoke.json) · [hosted-qualification.json](hosted-qualification.json).

## Scope & Decisions

- **Version & status:** `RedTilde 1.0.0` (`production`), pinned `DxUi` API 5 (`0c371a6197eb8c6b876b4b2843d75078bc21035c`), pinned `RedMonitor` client header (`9ebea6577fe9a5ced7647283c918f51f67d666fd`).
- **Signing disposition (explicit owner decision A3):** Shipped `v1.0.0` artifacts unsigned with verified `.zip.sha256` sidecars (`public-ca-authenticode-code-signing` listed under `deferred` in [`capabilities.json`](../../capabilities.json)).
- **Deferred post-1.0 items:** Recorded in [`capabilities.json`](../../capabilities.json) (`pending: []`, `deferred`: `upstream-performance-and-product-qualification`, `documents-over-65536-utf16-units`, `conditional-cross-process-atomic-replacement`, `physical-ime-touch-and-assistive-technology`, `physical-printer-paper-sheet-inspection`, `public-ca-authenticode-code-signing`, `full-latex-and-mermaid-language-compatibility`, `wiki-target-navigation-and-embed-resolution`).

## Local Zero-Warning Build & Full `x64` Matrix

All profiles share compiled source identity `720905a6e7b1c1d69f60f02c59702f7a15419fe6f8e2cb10fc4569c29488f5c8` and compiled with `0 Warning(s)` and `0 Error(s)`:

- **Builds verified:** `x64` and `ARM64` × (`Release`, `Debug`, `ASan Debug`, and `Release -ProductionOnly`).
- **Full `x64` non-interactive test matrices (`203/203` catalog cases each + Preferences, RecoveryRestart, Performance, Captures, FailureReportRouting, and ASan probe):**
  - `x64 Release` (`20261123T090525611Z-61572-62e381761f0c47aa95e3db6d25068bfa`, binary `19F60FF0...`): `passed = true`, `FailureReportRouting` exit `0`.
  - `x64 Debug` (`20261123T090825904Z-78484-7a5680bdd31d4eb1b1cbf84d5642df70`, binary `82906602...`): `passed = true`, `FailureReportRouting` exit `3`.
  - `x64 ASan Debug` (`20261123T091122646Z-57996-fec48ff1423448fd9db701f08c94b35b`, binary `817F7944...`): `passed = true`, `FailureReportRouting` exit `3`, real `AddressSanitizer` `heapUseAfterFreeDetected = true`.
- **Transparent retention of earlier `EditorContent` teardown timeout:** An earlier `x64 Release` test run (`20261123T085815666Z-64900-c053e3d91837468eb1dcd2ebc9b24c15`) passed `60/60` `EditorContent` cases and then exited `-532391851` in `ApplicationRuntime::~ApplicationRuntime()` (`Common/DxUiFailureHost.cpp:146`, `ERROR_TIMEOUT` `0x800705b4`). Minidump + PDB inspection (`RedTilde.pdb`) showed sole active worker thread `73352` in `RedTilde::RecoveryService::State::Worker` (`Common/Recovery.cpp:540`, `Remove(... .lease)` -> `ntdll!ZwClose+0x14`) after exiting its loop while concurrent processes were running on the host; `3/3` isolated `EditorContent` reruns and the complete `x64 Release`, `Debug`, and `ASan Debug` suites passed cleanly.

## Packaging, CLI, Screenshots & Sandboxed Installer Smoke

- **Portable production package (`package.ps1 -Platform x64`):** `RedTilde-1.0.0-x64-20261123-101619-065.zip` (archive SHA-256 `AA669267B696B4BC7109A80518A35A3FACEE5EA4507F1F28C66594215742A0B5`, production binary `A97C7E251BD96F46DF2A03712B5DB3FA16F5EAE564140004E04F6613BC968D62`, `status = production`, `version = 1.0.0`, `testsEnabled = false`, `smokePassed = true`, `16/16` test-only entry-point refusals with exit code `2` and absent data directories).
- **Updated screenshot & CLI verification:** Regenerated [`docs/screenshots/settings.png`](../../docs/screenshots/settings.png) (`96` DPI), visually verified `settings-about` displays `RedTilde 1.0.0` and updated `IDS_ABOUT`, and confirmed `RedTilde.exe --version` outputs `RedTilde 1.0.0`.
- **Sandboxed [`Installer/Install-RedTilde.ps1`](../../Installer/Install-RedTilde.ps1) end-to-end smoke ([installer-smoke.json](installer-smoke.json)):** Extracted `RedTilde-1.0.0-x64-20261123-101619-065.zip` into an isolated `.build/installer-v100-smoke` sandbox, installed with `-RegisterFileAssociation`, verified Start Menu `.lnk`, `RedTilde.exe --version` (`RedTilde 1.0.0`), hidden `--capture` (`118,711` B PNG), Uninstall/`DisplayVersion = 1.0.0`, `RedTilde.Markdown` ProgId, `.md`/`.markdown` `OpenWithProgids`, and `Applications\RedTilde.exe` entries; then verified `-Uninstall` cleanly removed the install directory, shortcut, and registry keys while preserving user settings data untouched.

## Hosted `Qualify main Release` (`x64` & `ARM64` — Run `#45167960343`)

- **Qualification run ([`45167960343`](https://github.com/RedSalamanders/RedTilde/actions/runs/45167960343), [hosted-qualification.json](hosted-qualification.json)):** Ran on `main` commit `1b8174b8d28b6177ee81c79df2f5b3066c0dc959` (`buildNumber: 538`, `version: 1.0.538`, `nativeDigest: 9fca4e632beb7fa71de37d09954a7563abc866f84d4b6c320aaa1ed42da2d57e`) with all `7/7` jobs succeeding (`admission`, `regression (x64)`, `regression (ARM64)`, `test-gate`, `production (x64)`, `production (ARM64)`, `complete`):
  - **`x64` Regression (`windows-2025-vs2026`):** `203/203` cases (`0` failed, `0` skipped, `assertionProbePassed = true`), binary `RedTilde.exe` SHA-256 `bd3226bf9f5269cda39f051221d215a8629d232e2658512949f4aeae628b0de5`, evidence ID `20261123T115835905Z-2080-aa603a234b0b4f2cb88bbdc1c38cb774`.
  - **`ARM64` Regression (`windows-11-vs2026-arm`):** `203/203` cases (`0` failed, `0` skipped, `assertionProbePassed = true`), binary `RedTilde.exe` SHA-256 `5062d3e5b74328b739cd0f3b0ea82fe8eb312f101adb10408a44c009608e601d`, evidence ID `20261123T115515975Z-6680-6e3467e4a343475baf0865988fef862b`.
  - **`x64` Production ZIP (`production-x64`):** `RedTilde-1.0.538-x64-20261123-120618-798.zip` (`5,773,643` B, archive SHA-256 `1DF31F25FCFFBEBD094C3D8CA86EE6A7C53EDD859EB6CB261D9738D591DFA01F`, binary SHA-256 `E434C8CCBA67B43E8F358897170767ED64B1355C25724E201D85BBBA814F7758`, `status = production`, `smokePassed = true`, `16/16` exit-`2` refusals).
  - **`ARM64` Production ZIP (`production-ARM64`):** `RedTilde-1.0.538-ARM64-20261123-040919-734.zip` (`6,254,049` B, archive SHA-256 `0E3DE44A53317FDAD240D14236D289F2CEAF43D86BB83898A6F887B296E37C5C`, binary SHA-256 `8DB48BCFCE42CBCC624B67F41E151645DB666BD82D2EA66CD05F0F3837035C34`, `status = production`, `smokePassed = true`, `16/16` exit-`2` refusals).
- **Transparent retention of earlier failed hosted run ([`45148102890`](https://github.com/RedSalamanders/RedTilde/actions/runs/45148102890)):** On `c8ac8dd2c2a56266fa4519607f391198c7dddbe2`, `regression (x64)` and `regression (ARM64)` passed, then `production (x64)` and `production (ARM64)` failed in `Write-CiProductionReceipt` (`Production executable and frozen batch version differ.`) because `.ci/project.json` still carried `"versionBase": "0.1"`. PR `#163` (`1f906912`) aligned `.ci/project.json` (`1.0`), `vcpkg.json` (`1.0.0`), and `validate.ps1`, and fold PR `#164` (`1b8174b8`) left `Changes/` clear before run `#45167960343` succeeded.
