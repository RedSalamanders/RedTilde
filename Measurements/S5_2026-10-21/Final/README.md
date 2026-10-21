# RedTilde S5 local execution and assessment — 2026-10-21

Local implementation, collection and assessment are complete. **S5 qualification and S6 remain open.** The
application remains experimental. This packet does not approve DxUi's upstream D4c policy or release readiness.

## Outcome and limits

The redundant app work is removed with behavioral coverage: ordinary prose edit/undo no longer relayouts chrome,
Compose avoids resetting its hidden source mirror, and Split still follows every real source change. Exact
dirty-state, outline offsets/labels, history and current-syntax equation selection remain tested. In every measured
process the native harness completed all 36 warmup/timed edit/undo changes and restored the original source.
Baseline activity was 36 layouts and 36 mirror resets. Candidate activity was zero layouts, zero Compose mirror
resets and 36 Split mirror resets. These counters establish the removed work; they do not prove latency improvement.

Two new 12-block studies each completed 96 serial processes: A/A control and A/B candidate, Compose and Split,
16 timed edit/undo cycles plus two warmup cycles per process. **No primary metric resolves a directional change**
after exact sign-flip inference and Holm correction across the fixed twelve slots. Completed edited-frame medians
remain about 8.31 ms. Command/owner CPU and some memory controls exceed their investigation bands; A/B also has
one Split clean-frame control flag. No block was discarded, no baseline was silently changed, and no repeat was
selected to obtain a desired answer. Neither absence of significance nor the stable frame medians proves equivalence.

The user directed proceeding without waiting. The selected-process observer recorded competing compiler/MSBuild
CPU throughout the study window, 07:38:08–07:41:44 UTC. A peer also reported an already-running approved test
preflight. Its test pass does not remove the interference. The observer is not an exclusive-machine lease or an
ETW/GPU trace. The judge remains review-required; **performance acceptance is inconclusive**.

Eight default/WARP lifecycle runs completed with bounded fixture caches, no unrequested idle rendering/preparation,
resize, three simulated device replacements, exact replacement pixels and zero attached native hosts after close.
Thirteen baseline/candidate application captures are byte-identical. These short hidden probes do not establish
long-term kernel/GPU lifetime, actual driver removal or physical visibility/occlusion.

## Provenance

- App branch: `ej/redtilde-s5`; production optimization `1b10e7e49eb2c1327eb2924b1867f662bb0d7fb1`, corrected native
  harness/runtime code `2c3efa1bbede19cf882b0b1dc5d5030b92efec94`, checkpoint parent `b8a06474de0d7d81498cf21544972fb5aaa7f5b7`.
- Baseline v2: `1ccbdb7ba37621eb8c55e08258a9387463ea6da7`. It preserves the original app callbacks and the same corrected
  instrumentation, features and dependency. Its rebuilt executable is frozen privately under
  `.build/s5-20261021/baseline-v2-msvc1452`; old MSVC 14.51 frozen executables remain untouched.
- Baseline binary SHA-256: `BFBAFA9213C8A0AB74E70F844C606A0AECEA5464ACA7166229B0EA59DF18BDFD`.
- Candidate binary SHA-256: `336F4A9EC14FB95D3EA734A4885A0DC998634FCA8AB3D1802E17AC7ACAC0C00B`.
- Exact DxUi: `dd619ef0c5855307eb57758072658f2e3b777fd8`, API 4. Library evidence head/PR is separate from the consumer pin.
- Compiler: MSVC `14.52.37025`, v145, Windows SDK `10.0.26100.0`, x64 Release `/O2 /MD /W4 /WX /sdl /permissive- /std:c++latest`.
  The installed compiler changed from the checkpoint's 14.51.36231. Stale dependency-compiler guards refused the
  first attempts; [retained logs](logs) precede a fresh official restore, baseline and six-profile app build.
- Machine SINON, AMD64 Family 26 Model 68 Stepping 0, 32 logical processors; Windows 10.0.26300.0, Balanced power.
  Hardware adapter NVIDIA GeForce RTX 5080; WARP Microsoft Basic Render Driver. Native driver version, DLL hashes,
  adapter IDs, DPI 144 and 2040×1350 dimensions are retained in the study/native receipts.
- Common fixture: 652 UTF-16 units, `docs/examples/s5-performance.md`, SHA-256
  `746E1833A4C85ACEC6F59A0BBFF458FB068C9347C9E30B16CFF9DFB32BB11811`.
  Both sides matched all six harness input hashes, dependency fingerprint, effective compiler profile and environment.
  Source and binary guards ran before/after every measured process. Seed `20261021` and literal ABBA/BAAB schedules are retained.

The native compiler/resource inputs are recorded in [NativeInputs.json](NativeInputs.json). Measurement-time
source identities are in each study, not inferred from a later documentation commit. Final build/test receipts
bind the final documentation/tooling source separately. The reviewed data below is immutable observation data;
generated executables, object files, dumps and private recovery directories remain outside this repository's tracked evidence.

## Independent verification

[IndependentAudit-v2-msvc1452.json](IndependentAudit-v2-msvc1452.json) records **6,577 passing checks** over all
192 native slots. `Tests/Scope.Tests.Performance.Evidence.ps1` uses typed C# median, subset expansion and Holm
arithmetic, independent of the production PowerShell bitmask enumeration. Four known-value oracles include the
mixed-sign exact tail `158/4096` and fractional Holm value `0.462890625`. The audit checks scope, identities,
schedule, unique receipts, native hashes/counts, raw medians, all twelve effects/probabilities, every same-side
control and every directional finding. It verifies retained numerical/metadata consistency; it cannot recreate
missing binary provenance or approve the judge's policy.

Packet attributes preserve raw receipt bytes across Git checkouts. The retained
`validation/GitRoundtrip.json` also verifies both studies after an index checkout with automatic CRLF enabled;
line-ending conversion must not invalidate a native receipt hash.

[Six adversarial cases](IndependentAudit-Negative-Final-v2-msvc1452.json) reject an altered adjusted probability,
changed investigation band, legacy v1 judge, compiler mismatch, modified native bytes and reused native slot
for their expected reasons. Corruption was applied only to private copies. The original raw slots and old invalid
v1 outputs remain untouched. `validate.ps1` runs the numerical oracles and parses PowerShell tests.

Replay the reviewed numerical audit from the repository root:

```powershell
.\Tests\Scope.Tests.Performance.Evidence.ps1 -StudyPath @('Measurements/S5_2026-10-21/Final/aa-v2-msvc1452', 'Measurements/S5_2026-10-21/Final/ab-v2-msvc1452')
```

## Paired primary results

A/B values are medians of process medians (bytes for memory, cycles for owner CPU, microseconds for timing). Change is the paired geometric effect, so it need not equal the ratio of the displayed medians. Each control count is out of 24 same-side differences. All findings are no-resolved-change; bands are investigation thresholds, not acceptance tolerances.

### AA

| View / metric | A | B | Paired change | Holm p | Flagged controls | Max drift |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Compose / commandUs | 217.62 | 192.72 | -6.03% | 1.000000 | 20/24 | 69.26% |
| Compose / ownerCpuCycles | 14,196,219.00 | 14,157,484.25 | -2.59% | 1.000000 | 20/24 | 39.41% |
| Compose / completedEditUs | 8,314.08 | 8,316.02 | +0.03% | 1.000000 | 0/24 | 0.54% |
| Compose / cleanCompletedUs | 8,363.28 | 8,354.65 | -0.04% | 1.000000 | 0/24 | 0.89% |
| Compose / privateBytes | 74,633,216.00 | 75,157,504.00 | +0.33% | 0.626953 | 6/24 | 3.58% |
| Compose / workingSetBytes | 61,433,856.00 | 61,431,808.00 | +0.18% | 1.000000 | 3/24 | 3.52% |
| Split / commandUs | 286.18 | 272.05 | -3.87% | 1.000000 | 20/24 | 114.45% |
| Split / ownerCpuCycles | 18,419,332.75 | 17,920,924.00 | +0.42% | 1.000000 | 12/24 | 39.79% |
| Split / completedEditUs | 8,320.23 | 8,322.15 | +0.04% | 1.000000 | 0/24 | 0.96% |
| Split / cleanCompletedUs | 8,348.17 | 8,351.83 | -0.03% | 1.000000 | 0/24 | 0.87% |
| Split / privateBytes | 75,911,168.00 | 76,068,864.00 | +0.03% | 1.000000 | 2/24 | 2.66% |
| Split / workingSetBytes | 62,619,648.00 | 62,523,392.00 | -0.06% | 1.000000 | 0/24 | 1.83% |

### AB

| View / metric | A | B | Paired change | Holm p | Flagged controls | Max drift |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Compose / commandUs | 217.35 | 221.07 | +3.66% | 1.000000 | 18/24 | 44.86% |
| Compose / ownerCpuCycles | 13,320,759.25 | 13,671,198.00 | +3.27% | 0.287109 | 9/24 | 16.73% |
| Compose / completedEditUs | 8,310.92 | 8,314.48 | +0.05% | 1.000000 | 0/24 | 0.59% |
| Compose / cleanCompletedUs | 8,363.77 | 8,357.05 | -0.05% | 1.000000 | 0/24 | 0.88% |
| Compose / privateBytes | 75,393,024.00 | 74,934,272.00 | -0.25% | 1.000000 | 4/24 | 2.37% |
| Compose / workingSetBytes | 61,593,600.00 | 61,652,992.00 | +0.02% | 1.000000 | 0/24 | 2.00% |
| Split / commandUs | 267.70 | 258.80 | -2.70% | 1.000000 | 13/24 | 85.31% |
| Split / ownerCpuCycles | 16,880,221.00 | 16,732,361.75 | -0.53% | 1.000000 | 6/24 | 9.56% |
| Split / completedEditUs | 8,317.55 | 8,319.15 | -0.07% | 1.000000 | 0/24 | 1.67% |
| Split / cleanCompletedUs | 8,350.85 | 8,350.75 | +0.38% | 1.000000 | 1/24 | 11.05% |
| Split / privateBytes | 76,029,952.00 | 76,120,064.00 | +0.08% | 1.000000 | 2/24 | 2.86% |
| Split / workingSetBytes | 62,478,336.00 | 62,509,056.00 | +0.04% | 1.000000 | 0/24 | 1.90% |

The original slot arrays, literal schedules, receipt hashes and all control ratios are in aa-v2-msvc1452/ and ab-v2-msvc1452/. Preparation/submission and GPU-wait are retained diagnostics, not additional post-selected primary slots. Their split is affected by pacing; no extra executed work is inferred from compensating GPU wait.

## Resource assessment

[ResourceSummary-v2-msvc1452.json](ResourceSummary-v2-msvc1452.json) and [raw resource receipts/PNGs](resources)
retain all eight baseline/candidate × default/WARP × Compose/Split runs. These are lifecycle observations under
concurrent compilation, not latency-study samples. Each run has sixteen dirty cycles, nine resource phases and
three increasing replacement generations. Image decode is exactly 11,016,000 bytes, no pending decode remains,
and the fixture uses at most twelve brushes and five configured formats. Every post-replacement PNG matches
its pre-replacement reference exactly.

All sixteen idle brackets last 490.4–525.7 ms. Measured process CPU is zero in fifteen and one 15.625 ms quantum
in the remaining WARP/Split candidate bracket; owner cycles range from 950,681 to 1,486,319. No additional render
or preparation is requested. Resource-snapshot collection itself costs 76.4–157.2 ms wall and 78.125–171.875 ms
process CPU. Cumulative process-time subtraction across snapshots would mislabel that work as idle CPU.

All close phases report zero attached native hosts. Closed process handles are 321 for the hardware runs and
284–288 for WARP. Private memory between the first and third replacement changes by −4,092 to +4,592 KiB across
the eight runs; all values, handles, USER/GDI, generations, decoded bytes and available DXGI observations are retained.
This is not a zero-growth or long-term leak claim. Missing DXGI usage is `null` plus its HRESULT, never a zero
allocation. Driver/process caches and kernel lifetime require a longer independently qualified study.

## Build, behavioral and artifact validation

The refreshed toolchain builds x64 and ARM64 Debug, Release and ASan Debug. All three x64 profiles pass all
25 behavioral cases, preferences after immediate process exit, real recovery crash/restart with Save As protection,
13 actual non-activating captures per profile and isolated local minidump/marker checks. ASan detects the intentional
heap-use-after-free; an ordinary crash is not credited. All ARM64 results are cross-builds, with no runtime claim.
Final source-bound receipts and validator/format/scoped coverage accounting are retained under `validation/`.

[CaptureComparison-msvc1452.json](CaptureComparison-msvc1452.json) binds all thirteen baseline/candidate PNG
hashes. Native math and state/class diagram captures were also visually reviewed. There is no visual change to
publish to the design system. The portable package is experimental; extraction runs its own 25 cases using the
same executable, with host-installed runtime dependencies. Archive/extraction identities belong in `validation/`.

## Open gates and handoff

- Accept/calibrate the new measurement policy; collect suitable isolated evidence to resolve unstable command,
  CPU, memory and the flagged clean-frame control. This packet establishes no accepted regression or equivalence.
- Qualify longer-term GPU/kernel lifetime, physical occlusion and actual device removal. The eight short hidden
  lifecycle runs and three simulated replacements per run establish their exact scenarios only.
- Native ARM64 execution, physical IME/touch/assistive technology, time-authorized app focus/menu interaction,
  upstream PR performance acceptance, final release pin/installer/file associations and conditional cross-process
  save hardening remain open. No real-focus RedTilde test was authorized or run.

The single creation plan stays indexed under WIP. No library source or consumer pin changed during this resumed
assessment. Canonical DxUi work remains in its owned worktree and draft PRs #90/#91; the original dirty sibling
checkout was preserved. RedTilde has no remote, so final commits and refreshed external checkpoints preserve the work locally.
