# Gate 1 Hosted Release & Gate 2 Native ARM64 Qualification — 21 November 2026

Source commit: `de873eebdfddc970be295180abfc4c505250b74b` (`main`, Gate 1 convergence merge; followed immediately by changelog fold commit `ea111980a9aa3f9db0a8b49880b751c437e853fa` with no C++ or build-input changes).  
Dependency pin: `DxUi` API 5 (`0c371a6197eb8c6b876b4b2843d75078bc21035c`), MSVC `14.52.37025`, Windows SDK `10.0.26100.0`.  
Ledger: [hosted-release-and-arm64.json](hosted-release-and-arm64.json).

## Hosted Runs Qualified

| Workflow / Run | Profile | Host | Build | Catalog Cases | Captures | Crash / Special Probes |
| --- | --- | --- | --- | --- | --- | --- |
| [Qualify main Release #44928555514](https://github.com/RedSalamanders/RedTilde/actions/runs/44928555514) | `x64 Release` | `X64` (`10.0.26100.0`) | `0 Warning(s)`, `0 Error(s)` | `178/178` (`Storage` 32, `Settings` 15, `Recovery` 22, `Editor` 109) | `40` | `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget` passed; `FailureReportRouting` passed; 6 dumps (`-531475201`) |
| [Qualify main Release #44928555514](https://github.com/RedSalamanders/RedTilde/actions/runs/44928555514) | `ARM64 Release` | `Arm64` (`10.0.26200.0`) | `0 Warning(s)`, `0 Error(s)` | `178/178` | `40` | `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget` passed; `FailureReportRouting` passed; 6 dumps (`-531475201`) |
| [Manual extended qualification #44930332182](https://github.com/RedSalamanders/RedTilde/actions/runs/44930332182) | `ARM64 Debug` | `Arm64` (`10.0.26200.0`) | `0 Warning(s)`, `0 Error(s)` | `178/178` | `40` | `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget` passed; `FailureReportRouting` (`exitCode 3`, `152` stderr bytes) passed; 6 dumps |
| [Manual extended qualification #44930335298](https://github.com/RedSalamanders/RedTilde/actions/runs/44930335298) | `ARM64 ASan Debug` | `Arm64` (`10.0.26200.0`) | `0 Warning(s)`, `0 Error(s)` | `178/178` | `40` | `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget` passed; `FailureReportRouting` passed; real ASan `heapUseAfterFreeDetected: true` (`exitCode 1`); 5 dumps |

## Production Release ZIP Packages & Smoke Verification (`#44928555514`, version `0.1.519`, build `519`)

- **`x64` (`RedTilde-0.1.519-x64-20261121-173310-061.zip`, `3,671,770` bytes):**
  - Archive SHA-256: `0411498ff197a084fdd454fe6be3050b44b1469632bc5106e6f87b1e0f4fd895`
  - Packaged binary SHA-256: `1E40AB24FC4AAE80A205E4D505454ADF8BA783C958BD04656CB49F7A9F4648FE`
  - `testsEnabled: false`, `runtimeRedistributableBundled: true`, `smokePassed: true`, `16/16` test-only entry points refused with exit code `2` and absent data directory.
- **`ARM64` (`RedTilde-0.1.519-ARM64-20261121-093426-031.zip`, `4,154,583` bytes):**
  - Archive SHA-256: `0f42da229c95634ca338d7fa29541cac9be8822f86d906fbefc8d24cfe8317e3`
  - Packaged binary SHA-256: `5FADA6675CF4CAB697F52582EB521CD6BBC41B8D3C34E41ABC721C3F69555DA7`
  - `testsEnabled: false`, `runtimeRedistributableBundled: true`, `smokePassed: true`, `16/16` test-only entry points refused with exit code `2` and absent data directory.

## Gate Outcome

- **Gate 1 (`CV-6-05`, `H-10`, `H-15`, `H-26`, `H-27`)**: **COMPLETED** (hosted Release qualification passed; changelog fold landed in PR #147).
- **Gate 2 (Physical ARM64 Runtime Qualification, `T-Q2` ARM64 parity)**: **COMPLETED** on hosted `windows-11-vs2026-arm` (`Arm64`) across `Release`, `Debug`, `ASan Debug`, and portable `ARM64` production package smoke (`v1.0.0` release cut remains under Gate 7).
