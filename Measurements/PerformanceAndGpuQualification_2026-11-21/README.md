# Hardware GPU Resource Lifecycle, Observed A/A Control & Attribution — 21 November 2026

Source baseline: `ea111980a9aa3f9db0a8b49880b751c437e853fa` (`x64 Release` binary `D27923E4F2406B654A26944808002DDC7DBB092968DCC0A9DEE2DAAAA14BAD09`, DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Ledgers: [hardware-gpu-resource.json](hardware-gpu-resource.json) · [aa-control-summary.json](aa-control-summary.json) · [debug-allocation-review.md](debug-allocation-review.md).

## 1. Real Hardware GPU vs WARP Resource Lifecycle (`--resource-probe --control-diagnostics`)

Executed serially (`Start-Process -Wait`) on `docs/examples/s5-performance.md` at `144 DPI` (`2160x1350`, `16` cycles):

- **Hardware (`NVIDIA GeForce RTX 5080`, `vendorId: 4318`, `deviceId: 11266`) — `Compose` and `Split`:**
  - `clean-after-dirty` -> `clean-hidden-idle` (`516.9 ms` / `487.5 ms`): **0 unrequested renders** (`renders` stayed `64`), **0 idle OS CPU ticks** (`idleProcessCpu100ns = 0`).
  - `hidden-notification-idle` after `WM_SHOWWINDOW(FALSE)` (`513.1 ms` / `515.0 ms`): **0 unrequested renders** (`renders = 64`), **0 idle OS CPU ticks**.
  - `resumed-forced-frame` (`renders = 65`) and `resized-forced-frame` (`rootChildFlushes`: `7` Compose, `6` Split) succeeded.
  - **Device-loss recovery:** 3 simulated cycles (`DebugSimulateDeviceLoss()`, resource generations `4 -> 6 -> 8`) produced **bit-identical** PNG captures before and after recreation.
  - **Teardown:** `closed.nativeHosts == 0` in both views; `ownedHeapDiagnostics.complete == true` (`3,834,510` bytes busy in Compose, `3,831,576` bytes in Split).
- **WARP (`Microsoft Basic Render Driver`) — `Compose`:**
  - Same zero-render idle quiescence (`renders = 64`, `idleProcessCpu100ns = 0`), 3x bit-identical device-loss recovery, `closed.nativeHosts == 0`, `ownedHeapDiagnostics.complete == true`.
  - Contrast: WARP `privateBytes` (`~34.8 MiB` in A/A, `~55.8 MiB` after image fixture) is ~43 MiB lower than Hardware (`~77.5 MiB` A/A, `~123.6 MiB` after image fixture) while `workingSetBytes` (`~66.4 MiB` vs `~64.6 MiB`) and CRT/default heap busy payload (`~3.8–4.5 MiB`) remain comparable — attributing the hardware private-byte baseline primarily to D3D11/NVIDIA WDDM runtime allocations.

## 2. Observed Same-Binary A/A Control & Current Command/WS Attribution

Executed 6 independent `ABBA`/`BAAB` blocks × 16 cycles × 2 views (24 processes per view per renderer) with `-ControlDiagnostics` and `Build/PerformanceObservation.psm1`:

- **Statistical A/A result:** All `12` primary metric rows had `finding = no-resolved-change` (`adjP >= 0.375`).
- **Control stability & host observation:** In Hardware `aa-hw`, `completedEditUs` (`~8.32 ms`), `cleanCompletedUs` (`~8.32 ms`), and `workingSetBytes` (`64.56 MiB` Compose / `65.57 MiB` Split) had `controlsStable = true`; `commandUs` (`122.0 us` Compose / `113.0 us` Split), `ownerCpuCycles`, and `privateBytes` had shared-host variance (`controlsStable = false`). Observer coverage was `complete` (`maxGap = 1.04 s <= 2.5 s`), and `quietCpuWindowVerified = false` (max busy CPU `18.60%` HW / `17.55%` WARP with active external processes).
- **Current `app-edit-phases-v2` attribution (Hardware A/A medians):**
  - **Compose (`commandUs = 122.0 us`):** `caption` (`39.3 us`), `identity` (`20.7 us`), `selection` (`19.1 us`), `analysis` (`12.5 us`), `enter` (`9.15 us`), `chrome` (`7.0 us`), `recovery` (`6.1 us`), `exit` (`1.4 us`), `mirror` (`0.5 us`).
  - **Split (`commandUs = 113.0 us`):** `caption` (`37.8 us`), `identity` (`19.1 us`), `selection` (`17.8 us`), `analysis` (`11.9 us`), `enter` (`8.7 us`), `chrome` (`7.0 us`), `recovery` (`5.5 us`), `mirror` (`1.6 us`), `exit` (`1.4 us`).
  - **Top `app-selection-components-v1` items:** `block-labels` (`3.4 us` Compose / `0.7 us` Split) and `position-status` (`3.2 us` / `3.3 us`).

## Remaining Open Gate 6 Items

1. Quiet-host (`quietCpuWindowVerified == true`) paired A/B candidate study whenever a future candidate changes hot paths or adopts API 6.
2. Physical DWM occlusion and live monitor DPI change verification (explicitly separate from hidden `WM_SHOWWINDOW` and simulated `--capture-dpi`).
