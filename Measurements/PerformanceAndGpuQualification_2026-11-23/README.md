# Hardware GPU, DWM Quiescence & Observed A/A Control Qualification — 23 November 2026

Source baseline: `5a6d85d6af03fd07e17e119e2df01a912f40047c` (source identity `685155b119431234f70280bc6778660636cd23fa3b21f68551fa2de20c29feaa`, `x64 Release` binary `0C201BBAB000993B3E0F79E2096F228EB4F0BC4742374D318C46E6FEC9AE52D0`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Ledgers: [resource-summary.json](resource-summary.json) · [observed-summary.json](observed-summary.json) · [dwm-occlusion-probe.json](dwm-occlusion-probe.json). Prior baseline packet: [PerformanceAndGpuQualification_2026-11-21](../PerformanceAndGpuQualification_2026-11-21/README.md) (`ea111980`, including [debug-allocation-review.md](../PerformanceAndGpuQualification_2026-11-21/debug-allocation-review.md)).

## 1. Real Hardware GPU vs WARP Resource Lifecycle (`--resource-probe --control-diagnostics`)

Executed serially (`144 DPI`, `2160x1350`, `16` cycles) on `docs/examples/s5-performance.md` across Hardware (`NVIDIA GeForce RTX 5080`, `vendorId: 4318`, `deviceId: 11266`, `Compose` & `Split`) and `WARP` (`Microsoft Basic Render Driver`, `Compose`):

- **Clean hidden idle (`clean-hidden-idle`, `511.1 ms` HW Compose / `643.6 ms` HW Split / `541.3 ms` WARP Compose):** **0 unrequested renders** (`renders` stayed `64` / `67` / `69`), **0 idle OS CPU ticks** (`idleProcessCpu100ns = 0`).
- **Hidden notification idle (`hidden-notification-idle` after `WM_SHOWWINDOW(FALSE)`, `508.4 ms` / `491.6 ms` / `492.9 ms`):** **0 unrequested renders**, **0 idle OS CPU ticks**.
- **Forced resume & resize:** `resumed-forced-frame` (`renders`: `65` / `68` / `70`) and `resized-forced-frame` (`rootChildFlushes = 7` in all three runs) completed cleanly.
- **3x device-loss recovery (`DebugSimulateDeviceLoss()`, generations `4 -> 6 -> 8`):** Bit-identical native PNG captures preserved across all 3 cycles in all three probes (`exactPixelsPreserved = true`).
- **Teardown & bounded heap walk:** `closed.nativeHosts == 0` in all three probes; `ownedHeapDiagnostics.complete == true` (`3,878,260` B busy HW Compose, `3,876,302` B HW Split, `4,322,277` B WARP Compose).
- **Non-reproducing earlier `hw-split` observation (2026-11-22):** An earlier `2026-11-22` `hw-split` run on the same commit stopped during its first `recreated-forced-frame` check due to a 29-pixel ClearType subpixel edge difference (`maxChannelDelta = 15`) confined to the two `&` glyphs inside `\begin{bmatrix}a&b\\c&d\end{bmatrix}` (`bbox=(1276,1025)-(1338,1033)`), where `resource-after-0.png` matched later runs bit-for-bit. Ten consecutive diagnostic reruns (`WARP` split, `HW` split 4-cycle, `HW` split 16-cycle, 3 repeats, and 5 path-length-matched repeats) plus this complete serial `hw-split` probe all passed with `0` differing pixels across all 3 device-loss cycles; no source defect was present.

## 2. Visible-Window DWM Minimize/Restore Quiescence Probe

Measured against a visible `RedTilde.MainWindow` instance (`dwm-occlusion-probe.json`):

- Minimized via `SW_MINIMIZE` (`minimizedIsIconic = true`) for `2000 ms`: **`minimizedCpuDeltaMs = 0`**, working set unchanged (`76,427,264` B -> `76,427,264` B), private bytes unchanged (`80,068,608` B -> `80,068,608` B), GDI handles unchanged (`45 -> 45`), USER handles unchanged (`55 -> 55`).
- Restored cleanly (`SW_RESTORE`, `restoredVisibleAndNonIconic = true`) and closed via `WM_CLOSE` (`cleanCloseExitCode = 0`).

## 3. Observed Same-Binary A/A Control & Command/Selection Attribution

Executed 6 `ABBA`/`BAAB` blocks × 16 cycles × 2 views (`24` processes per view, `48` total Hardware processes) with `-ControlDiagnostics` and `Build/PerformanceObservation.psm1`:

- **Statistical A/A result:** All `12` primary metrics (`Compose` & `Split` × `commandUs`, `ownerCpuCycles`, `completedEditUs`, `cleanCompletedUs`, `privateBytes`, `workingSetBytes`) reported `finding = no-resolved-change` (`primaryFindingCountRegressed = 0`, `primaryFindingCountImproved = 0`, `adjustedP = 1.0`).
- **Control stability & host observation (truthful limits):** `observer.coverageComplete = true` (`187` samples, `maxGapSeconds = 1.65 <= 2.5 s`). `workingSetBytes` achieved `controlsStable = true` in both views (`67.86 MiB` Compose / `68.97 MiB` Split); ambient developer-host background load was present during the shared-host run (`meanSystemBusyPercent = 65.73%`, `maximumSystemBusyPercent = 92.37%`, so `quietCpuWindowVerified = false` and CPU/latency control drift reflected shared-host load).
- **`app-edit-phases-v2` medians (24 processes/view):**
  - **Compose (`commandUs = 241.22 us`, `completedEditUs = 8318.97 us`, `cleanCompletedUs = 8318.05 us`):** `chrome` (`59.8 us`), `caption` (`56.3 us`), `identity` (`33.08 us`), `selection` (`20.65 us`), `analysis` (`14.23 us`), `enter` (`11.57 us`), `recovery` (`6.95 us`), `exit` (`1.73 us`), `mirror` (`0.4 us`). Top `app-selection-components-v1` items: `block-labels` (`3.92 us`), `position-status` (`3.52 us`), `tool-caption-publication` (`1.42 us`).
  - **Split (`commandUs = 212.8 us`, `completedEditUs = 8323.58 us`, `cleanCompletedUs = 8321.22 us`):** `caption` (`52.58 us`), `chrome` (`49.4 us`), `identity` (`31.38 us`), `selection` (`18.67 us`), `analysis` (`12.35 us`), `enter` (`10.85 us`), `recovery` (`6.57 us`), `mirror` (`2.25 us`), `exit` (`1.58 us`). Top `app-selection-components-v1` items: `position-status` (`3.88 us`), `mirror` (`1.33 us`), `tool-caption-publication` (`1.27 us`).

## Scope Disposition

Quiet-host (`quietCpuWindowVerified == true`) paired A/B candidate studies on future hot-path or API-6 (`H-01`/`H-02`/`H-05`/`H-14`) changes and live multi-monitor DPI-switch hardware verification remain ongoing post-1.0 rules under `Performance` & `Core_PerformanceAndResources` contracts.
