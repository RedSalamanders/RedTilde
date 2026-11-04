# RedTilde S5 continuation checkpoint — 2026-10-21

Latest follow-through: [native-v2 noise investigation](../S5_Noise_2026-10-21/README.md), including both retained
attempts, collector repair, full activity coverage and the still-open quiet-host/hardware/upstream gates.

**Historical 08:42 checkpoint.** The [final local execution assessment](Final/README.md) supersedes the pending
commands and old-toolchain identities below. The resumed work completed the corrected studies and full-fixture
resource checks with a freshly rebuilt MSVC 14.52.37025 baseline. Qualification gates remain open as recorded there.

The user requested saving everything for another session. **S5 and S6 remain open.** All already-running
workers finished successfully; no build, test, timing study or activity observer remains active at this checkpoint.
This is a handoff, not a completed performance/resource qualification packet.

## Start here next session

Continue the indexed plan at `Specs/Plans/Done/RedTilde_2026-10-20.md`. Read this file before launching work.
Use `Z:\src\RedTilde` on branch `ej/redtilde-s5`; do not start again from the creation commit. The tested code
commit is **`2c3efa1bbede19cf882b0b1dc5d5030b92efec94`**. Later checkpoint commits add documentation/evidence.
The application has no remote configured; preserve its local commits and the external checkpoint below.

1. Keep the current production optimization. Finish the **v2** paired study and full-fixture resource validation.
2. Before timing, finish all builds/tests and check actual compiler/native-worker termination. Never overlap
   timings with compilation or functional suites. No time-authorized interactive test has been agreed.
3. Warm-build current Release after this documentation checkpoint: the app hashes tracked documentation too,
   so the existing v2 receipts intentionally bind the earlier tested source rather than this new handoff text.
4. Verify the six common harness input hashes against `baseline-v2/baseline.json`. Git checkout can convert
   LF to CRLF; identical source semantics are insufficient for this byte check. Normalize only mismatching
   common inputs to the frozen bytes and verify their hashes; do not relax the guard.
5. Run separate serial 12-block A/A and A/B studies with the corrected baseline. Inspect every control flag,
   raw round and unavailable resource counter. The app judge remains review-required; it cannot qualify DxUi D4c.
6. Repeat full-fixture default/WARP resources in Compose/Split using both frozen baseline and candidate. Use
   the new bracketed idle fields, not differences of cumulative snapshot CPU. Then assess remaining gates,
   update code/tests/specs/docs together and retain the original failed/invalid receipts.

```powershell
Set-Location Z:\src\RedTilde
.\build.ps1 -Configuration Release -SkipRestore
.\Test-Changes.ps1 -Explain
.\performance-paired.ps1 -Baseline .build/s5-20261021/baseline-v2 -SameBinaryControl -Blocks 12 -Cycles 16 -OutputRoot .build/s5-20261021/aa-v2
.\performance-paired.ps1 -Baseline .build/s5-20261021/baseline-v2 -Blocks 12 -Cycles 16 -OutputRoot .build/s5-20261021/ab-v2
```

Those output directories must be fresh. **Neither v2 full study has run.** Keep source/harness/fixture and
binary bytes stable during the studies. Existing source-bound receipts refuse stale `-SkipBuild` reuse.

## Repositories and exact identities

| Item | Identity |
| --- | --- |
| App branch | `ej/redtilde-s5` at `Z:\src\RedTilde` |
| Original instrumented baseline | `f91b0994b27b5c0358491ff2a1517b1d2988935c` |
| Production optimization | `1b10e7e49eb2c1327eb2924b1867f662bb0d7fb1` |
| Corrected harness + optimization | `2c3efa1bbede19cf882b0b1dc5d5030b92efec94` |
| Matched baseline v2 branch | `ej/redtilde-s5-baseline-v2`, commit `1ccbdb7ba37621eb8c55e08258a9387463ea6da7` |
| Current exact DxUi pin | `dd619ef0c5855307eb57758072658f2e3b777fd8`, API 4 |
| Canonical owned DxUi worktree | `C:\Users\eric\.codex\worktrees\redtilde-math-mermaid\DxUi` |
| DxUi branch | `ej/redtilde-s5-host-policy` |
| Pushed library evidence head | `7d50cadc6fc73fd242dcbe69627bc5f61f8bbbf9` (runtime pin remains `dd619ef0`) |
| Library PR | [Draft #91](https://github.com/RedSalamanders/DxUi/pull/91), stacked on [draft #90](https://github.com/RedSalamanders/DxUi/pull/90) |

The original `Z:\src\DxUi` remains an unrelated dirty checkout. Do not reset, edit, clean or merge it.
`Z:\src\DxUi-worktrees\rich-text-core-d2` belongs to the PR #89 profiling chat. PR #89 was independently
rewritten to `335c5157a592aba381b8779a216ba7deda282895`; our exact feature/policy pins deliberately retain their
older ancestry. Do not implicitly rebase or adopt that rewrite. Edit shared library code only in the owned
canonical worktree, never in RedTilde's restored dependency source.

## Implemented production changes

`Application::CurrentText` caches one materialized string against immutable source identity. Dirty checks still
compare exact saved source, including direct resets, same-length edits and undo. Recovery uses that source and
retains the guarded proposed-copy behavior for unapplied local drafts.

The hidden read-only mirror is deferred in Compose/Source, synchronized on entering Split, and refreshed for
each actual visible Split source revision. Ordinary edits refresh outline offsets/counts without chrome layout;
heading label/level/count changes still lay out the outline. Current syntax drives the equation inspector before
the pending-analysis fallback. New tests cover exact view/history/selection, shifted navigation targets,
heading rename/level/add/remove, direct-reset dirty state and code/math delimiter distinctions.

The common native harness waits for current acknowledged semantic geometry, required image/mirror readiness
and an actual DXGI completion event. Raw arrays separate command, preparation/submission, GPU wait, complete
frame, clean frame and owner CPU cycles. Resource probes exercise local image decode, clean/hidden idle,
resize, three simulated device replacements with exact PNG equality and asynchronous native-host retirement.

The canonical DxUi change adds `AttachOptions::forceWarp`, preserving a shared owner-thread pool's policy,
inheriting it for default peers and refusing conflicts with `ERROR_INVALID_STATE`. Its native test checks
actual software adapters, idempotence, peers, replacement and final attachment accounting.

## Important investigation results and mistakes retained

The historical 0.275→0.376 ms forced-render difference did not wait for each edited semantic publication and
does not establish completed frame latency or a causal regression. Full-fixture completed edit/frame samples
are approximately 8.3 ms, similar to clean completed frames. These are hidden native frames, not physical FPS.
For 36 edit/undo changes, the original callback does 36 layouts and mirror resets. The optimized callback does
zero chrome layouts in both views and zero mirror resets in Compose; Split retains all 36 visible updates.

The first full v1 A/A and A/B studies each completed 12 independent ABBA/BAAB blocks per view (96 processes per
study), with the same pin/compiler/fixture/adapter. No compiler/native test activity was observed by the retained
one-second observer. Completed-frame same-side controls were stable; several command/CPU/memory controls were
noisy. Preserve all drift flags and raw slots. This observation is not an exclusive machine lease.

**The original v1 adjusted-p outputs are invalid.** PowerShell selected an integer overload for
`[math]::Min(1, ...)`, rounding fractional Holm values to zero and falsely calling one command result improved.
The fix uses `1.0`. An independent mixed-sign binomial-tail oracle asserts raw `158/4096` and adjusted
`0.462890625`. [V1-CorrectedComparisons.json](V1-CorrectedComparisons.json) reanalyzes the unchanged raw blocks;
all 12 slots in both studies have no resolved change after proper correction. Controls and policy still limit
acceptance. Do not overwrite the original v1 JSON or silently replace its judge result.

Resource snapshot collection enumerates all system threads. Its CPU cost was being included in the difference
between adjacent cumulative snapshots, creating misleading idle figures. The v2 harness brackets idle directly
and separately records `snapshotCollectionUs` / `snapshotCollectionCpu100ns`. The Release Core default fixture
records about 94–141 ms collection CPU, versus 15.625 ms and 0 ms in its two approximately 500 ms idle windows.
Owner cycles are recorded separately; process-time granularity and background GPU workers remain relevant.
Do not infer an idle busy loop from the old cumulative differences. Full-fixture v2 assessment is still pending.

## Validation actually completed

At tested app commit `2c3efa1`, source identity
`f1519a127ae851b5fc4a902d640a2fd8aec3d971f55312e232cc2cfb7645519e`:

- All six x64/ARM64 Debug, Release and ASan builds pass. ARM64 results are cross-builds.
- All 25 behavioral cases pass in each x64 profile; all three `-Suite All` runs also generate 13 actual hidden
  captures, check crash/marker behavior and process preferences/recovery, and ASan detects real heap use after free.
- Validators and format checks pass, including the corrected numerical oracle. The v2 full native resource
  protocol runs inside Core on small default/WARP scenes; full-fixture v2 paired/resource runs remain pending.
- Exact current receipts are retained under [V2](V2/Builds/x64-Release.json) and in the private archive.

Original v2 test directories:

- `.build/tests/20261021-083237-285-x64-Release`
- `.build/tests/20261021-083236-513-x64-Debug`
- `.build/tests/20261021-083237-195-x64-ASan-Debug`

Earlier optimized v1 validation is also retained: all six builds; 25 cases per x64 profile; eight full-fixture
baseline/candidate default/WARP resource processes; zero closed native hosts; and three exact post-loss pixel
comparisons per process. All 13 Release captures are byte-identical to the original instrumented baseline.
The v1 extracted portable package passes 25 cases. These older results do not qualify the revised v2 artifact.

The canonical library runtime pin passes 19 default noninteractive suites per x64 profile: 57 passes with zero
skips, real ASan detection, all six builds, aggregate validation/tooling and formatting. Its reviewed evidence is
`Measurements/RichText/2026-10-21-native-host-policy` in the canonical worktree. Its complex-UI before/after
reports are unpaired diagnostics, collected while other work was active; no non-regression claim is made.
Initial WIL compilation failure and safe running-process build refusal remain retained beside successful retries.

## Frozen artifacts and backup

Primary private evidence: `Z:\src\RedTilde\.build\s5-20261021`.
Independent backup: **`Z:\src\RedTilde-Checkpoints\S5_2026-10-21`**. Do not clean either until handoff is verified.
[ArchiveManifest.json](ArchiveManifest.json) records archive paths, sizes and hashes. The archive includes all
original raw studies, observers, full-fixture resource reports/pixels, old portable smoke, original and revised
frozen baselines, original candidate, corrected current Release executable/build/compiler identity, and logs.
A separate ZIP contains the complete three current v2 test runs, including captures and failed-probe diagnostics.
The independent backup also contains an all-ref RedTilde Git bundle and an incremental DxUi S5 bundle. The
library bundle requires base `1be53bae1b520903c07491e8d8b547d2b60e4f6d`, available from the existing canonical
repository/feature branch. Both bundles are verified; their final hashes and refs are in the external manifest.

Use `.build/s5-20261021/baseline-v2` for the next study, not `baseline` or `candidate-v1`:

- Baseline v2 executable SHA: `9C89B8BCDBFE888E18FDC993F2307A105184F3AC26F4B34DD3626C1892C188AF`.
- Current v2 executable SHA: `F2E8F18DCF55BF761F918AE15F2ED58F7E502E11FED75A94F4968CB551A71033`.
- Baseline source identity: `86e445426d7757cbafae8560cb8202440ba59704e89601253fb87791ab323348`.
- Shared compiler profile SHA: `58D144BE5EDBD1EAF2C57718B875443BE49B252A2AB9E4D2AD1259AF5646EEBC`.

The frozen compiler flags contain the original repository path. A relocated checkout, changed toolchain,
dependency, OS/driver runtime or power identity must be investigated rather than bypassing comparison guards.
The common v2 patch is retained as `.build/s5-20261021/common-v2.patch`; both baseline and candidate already
contain it. No additional cherry-pick is needed on `ej/redtilde-s5`.

## Gates to keep open

Complete and assess the v2 A/A/A/B and full-fixture resource studies, including all noisy controls. Independent
review/calibration of the new app judge, upstream D4c, kernel/GPU lifetime, physical occlusion/device removal,
native ARM64 execution, physical IME/AT, agreed-time interactive menu/focus behavior, external-write hardening,
release pin/installer/file associations and full language/large-document compatibility remain separate gates.
Do not move the overall plan to Done or treat pending/different-source PR CI as a pass.

The human-authorized PR #89 profiling quiet window was released at 08:12:31 Europe/Paris on 2026-10-21.
There is no outstanding hold, no authorization for future interactive tests and no recurring automation.
