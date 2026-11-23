# Application evidence

- [RedTilde v1.0.0 production release qualification (`x64`/`ARM64` hosted & local builds, tests, packaging, installer smoke), 2026-11-23](ReleaseV100_2026-11-23/README.md) — `1.0.0` (`production`) across all 8 zero-warning local builds (`x64`/`ARM64` × `Debug`/`Release`/`ASan Debug`/`ProductionOnly`), Full `x64` `Release`/`Debug`/`ASan Debug` passes (`203/203` catalog cases, real ASan `heapUseAfterFreeDetected: true`, `FailureReportRouting`, `RecoveryCrashRestart`, `LinkedSaveCrashRestart`), transparent note on earlier `20261123T085815666Z` `RecoveryService` teardown timeout, local `RedTilde-1.0.0-x64-20261123-101619-065.zip` + sandboxed `Install-RedTilde.ps1` verification, and hosted `Qualify main Release` `#45167960343` (`1b8174b8`, `version: 1.0.538`, `7/7` jobs green: `x64` & `ARM64` regression `203/203`, `RedTilde-1.0.538-x64-20261123-120618-798.zip` SHA-256 `1DF31F25...`, `RedTilde-1.0.538-ARM64-20261123-040919-734.zip` SHA-256 `0E3DE44A...`, `16/16` exit-`2` refusals, unsigned with `.sha256` sidecars), with earlier failed `#45148102890` (`versionBase` mismatch fixed in PR #163/`#164`) preserved transparently.
- [Gate 6 hardware GPU, DWM quiescence & observed A/A control qualification (`0` unrequested idle renders, `12/12` `no-resolved-change`), 2026-11-23](PerformanceAndGpuQualification_2026-11-23/README.md) — `5a6d85d` `x64 Release` (`0C201BBA...`, `NVIDIA GeForce RTX 5080` Compose/Split + `WARP` Compose) `--resource-probe --control-diagnostics` passes (`0` idle renders/CPU ticks, 3x bit-identical device-loss recovery, `closed.nativeHosts == 0`), visible-window DWM minimize/restore quiescence (`minimizedCpuDeltaMs: 0`, `cleanCloseExitCode: 0`), 6-block observed A/A control + phase/selection attribution (`12/12` `no-resolved-change`, `controlsStable: true` for `workingSetBytes`, `quietCpuWindowVerified: false` on shared host), and transparent record of a non-reproducing `2026-11-22` 29-pixel ClearType `&` edge observation (`10/10` reruns + serial Gate 6 run passed).
- [Gate 4 interactive desktop focus, live UIA tree & clipboard/media qualification (`9/9` lease stages + `62` UIA elements), 2026-11-22](InteractiveInputQualification_2026-11-22/README.md) — owner-agreed Default-desktop `test-interactive.ps1` (`x64 Release` `29CC8C38...`, `134` real `SendInput` events, `Foreground: unchanged`, `Focus: unchanged`, `Cursor: restored` in `1.3 s`; prior exit-25 cursor-shift run preserved separately), live `UIAutomationClient` tree/pattern probe (`62` descendants, `56` named controls, `9` patterns, `Source`/`Compose` & `Settings` activation, clean `CloseMainWindow` exit `0`), and `EditorMedia`/`EditorBehavior`/`Settings`/`Storage` clipboard/media passes (physical East Asian IME / Narrator / Touch deferred post-1.0).
- [Gate 3 real Windows print driver & PDF qualification (`0` leftover `.tmp` files), 2026-11-22](PrintDriverQualification_2026-11-22/README.md) — live spooling against `Microsoft Print To PDF` (`10.0.26100.4484`) across `markdown-showcase.md` (Letter Portrait `3/3` pages `1,395,041` B, A4 Landscape range `1, 2-3, 2` `3` pages `1,389,291` B), `rendering-fallback-cases.md` (`Current page = 2`, `1` page `1,060,063` B), `local-images.md` (`2/2` pages with PNG/JPG/BMP/GIF/TIF `6` image XObjects `1,152,279` B), atomic sentinel overwrite (`ReplaceFileW`), cancelled `ChooseFile` sentinel preservation, and open-source overwrite refusal (`E_ACCESSDENIED`) — all verified independently via WinRT `Windows.Data.Pdf.PdfDocument` + FlateDecode inspection + rendered bitmap ink checks; plus live `ConfigurePrinter`, `PTConvertDevModeToPrintTicket`, Portrait/Landscape/A4/Duplex/Copies/Collation, and preview checks without spooling physical paper on `Brother HL-L2445DW Printer` (`Microsoft IPP Class Driver`), `OneNote (Desktop)`, and `OneNote (Desktop) - Protected`.
- [Gate 5 live OneDrive, WSL 2, and SMB qualification (0 unexpected failures), 2026-11-22](RealLocationOneDriveWsl_2026-11-22/README.md) — fail-closed live qualification across all three real-location backends (`unexpectedFailures: []`, `0` leftover `redtilde-run-*` folders): OneDrive (`Storage` `34/34`, `Settings` `19/19`, `Recovery` `23/23`), two consecutive WSL 2 `9P` runs (`Recovery` `23/23`, `Settings` `17/19` + `2` probed watch primitives, `Storage` `23/34` + `11` probed `9P` primitives after `VolumeRootFingerprint` and `MoveFileExW` atomic fallback), and SMB (`Settings` `19/19`, `Recovery` `23/23`, `Storage` `27/34` + `7` probed primitives), each with `119,669`-byte PNG showcase `--capture` PASS.
- [Live SMB real-location re-qualification (0 unexpected failures), 2026-11-22](RealLocationSmb_2026-11-22/README.md) — fail-closed live SMB2 execution on `path.smb.primary` with `RefreshPublishedTimestamp` (`Common/Storage.cpp`) and probed primitive separation (`Tools/Test-RealLocations.ps1`): `status: PASSED`, `--capture` (`118,565` PNG), `Settings` (`19/19`), `Recovery` (`23/23`), `Storage` (`27/34`, `0` unexpected failures; `7` classified `unsupportedProviderPrimitives`), and `0` leftover `redtilde-run-*` folders (OneDrive and WSL still await owner-declared `selftest` roots).
- [Gate 1 hosted Release & Gate 2 native ARM64 qualification, 2026-11-21](Gate1AndArm64Qualification_2026-11-21/README.md) — hosted `Qualify main Release` (`x64`/`ARM64` `0.1.519` packages + `178/178` cases, `40` captures) plus hosted native `ARM64` `Debug` (`178/178`) and `ASan Debug` (`178/178` + real ARM64 ASan `heapUseAfterFreeDetected: true`) with zero warnings/errors.
- [Hardware GPU resource lifecycle, observed A/A control & attribution, 2026-11-21](PerformanceAndGpuQualification_2026-11-21/README.md) — `--resource-probe` (`NVIDIA GeForce RTX 5080` Compose/Split and `WARP` Compose: `0` unrequested idle renders, `0` idle OS CPU ticks, 3x identical-pixel device-loss recovery), 6-block observed A/A studies (`12/12` `no-resolved-change`), phase/selection attribution, and Debug allocation-attribution review (quiet-host A/B and physical DWM occlusion/DPI remain open under Gate 6).
- [Live SMB real-location qualification & root-cause diagnosis, 2026-11-21](RealLocationSmb_2026-11-21/README.md) — fail-closed live SMB execution on `path.smb.primary` with clean cleanup (`0` leftover dirs), `--capture` PASS (`118,136` PNG), `Settings` (`15/15`), `Recovery` (`22/22`), and `Storage` (`16/32` with 4 proven Win32/SMB2 root causes queued for Wave 2 `Common/Storage.cpp` fixes); OneDrive and WSL backends await owner-declared paths.
- [Governance, tooling & production-review reconciliation, 2026-11-21](GovernanceDisposition_2026-11-21/README.md) — post-merge fold PR #147 completion, reconciled `PR-01`..`PR-46`/`S1`..`S16` dispositions, rejected-cleanup preservation (`retention-20261028`), `Q15` (`4,772` tooling assertions) / `Q17` explicit holds, and Gate 3 printing readiness summary.
- [Linked-file saves and independent cosmetic settlement, 2026-11-06](LinkedSaveAndCosmeticSettlement_2026-11-06/README.md) — six zero-warning builds; 116 passing cases and one actual symbolic-link fixture capability failure per x64 profile, real interrupted-write recovery, consumer capture counterexample and retained original attempts. Hosted symbolic-link execution remains required.

- [Diagram spacing, text insets and task presentation](DiagramSpacing_2026-11-03/README.md) — exact-pin geometry,
  raster, source/history, settings and hidden-capture evidence, with broader qualification kept separate.

- [Mermaid sequence syntax expansion, 2026-11-03](MermaidSyntax_2026-11-03/README.md) — separately preserved fixed baseline, actor/note/numbering and plain-arrow examples, exact-pin consumer source/history coverage and hidden theme/DPI captures.

- [Shared Mermaid and Markdown display corrections, 2026-11-03](MermaidDisplay_2026-11-03/README.md) — canonical library defects, an old-pin fail-before app regression, corrected eight-family support claims and exact-pin consumer evidence; broader qualification remains separate.

- [Folder navigation, samples and rendering review, 2026-11-03](FolderNavigation_2026-11-03/README.md) — independent persisted folder root, restart and code-color pixel regressions, nine hidden sample renders, retained environment refusals and a bounded comparison with Markdown Viewer. Broader platform, interaction and performance gates stay explicit.

- [Current-source performance comparison, 2026-10-28](RedesignCompletion_2026-10-28/README.md) — four separately retained actual-PR-base/original-UI A/A and A/B comparisons, all 48 metric rows and same-side control vectors, with public source, binary, profile and input-manifest hashes. Independent audit: 16,891 checks; actual-base A/B has zero resolved regressions, while original-UI A/B retains four (two stable working-set and two command findings with unstable controls). Quiet CPU is unverified and the base policy is unqualified; no performance acceptance is claimed. ICF size scout showed no binary-size change and was rejected.
- [Latest completed native qualification, 2026-10-27](RedesignCompletion_2026-10-27/p28-native-9cf8b221/README.md) — producer `9cf8b221`: six builds, three Full x64 profiles with 66 cases and 23 capture pairs each, real ASan detection and nine authorized keyboard stages. ARM64 is cross-build only. Later candidates retain separate source identities and qualification gates.
- [Latest complete current-policy comparison, 2026-10-27](RedesignCompletion_2026-10-27/p28-policy-9cf8b221/README.md) — 576 processes and 72 rows. Original-UI Compose/Split working-set flags are +2.0457%/+2.4615%; Compose private bytes are +2.0952% with unstable controls. Compose command time remains unresolved. Starting-redesign and actual PR18-base comparisons have zero regressions; 36/72 controls are unstable. No performance acceptance is claimed.
- [Rejected startup icon decoder, 2026-10-27](RedesignCompletion_2026-10-27/icon-scout-4855c6f2/README.md) — all 32 same-pin scout reports, 24 result rows and frozen actual-base/candidate identities. Canonical replay passes 1,993 checks and a further root projection review passes 850. No repeatable material working-set saving is visible; retain canonical extraction and both original-UI working-set findings.
- [Resident census and branding hypotheses, 2026-10-27](RedesignCompletion_2026-10-27/p28-resident-census-4a635562/README.md) — helper14 original/current census plus separate helper12 branding composite. The two query scopes, amended-controller history, 32 KiB residual and incomplete per-page coverage remain explicit. These diagnostics establish no allocation owner, saving or acceptance.

- [Merged SDK renderer/Menu qualification, 2026-10-27](RedesignCompletion_2026-10-27/sdk-7020121d/README.md) — exact mirror of the reviewed SDK packet: six native profiles, 505 manifest data streams, 124 Menu cases per profile and both actual ASan detectors. All three paired scenario sets have zero regression rows, but 69/72 controls are unstable and the base policy is unqualified. SDK PRs #113–#116 merge as `6382b306`; the consumer remains pinned to its reachable renderer98 ancestor.
- [Earlier diagnostic follow-up, 2026-10-27](RedesignCompletion_2026-10-27/diagnostics-eed2abe9/README.md) — eight resident-memory processes, 297 snapshots and 160,882 region rows; separate V2/V3 selection instrumentation and a rejected formatter prototype. The microbenchmark's small benefit does not justify adoption. These diagnostics do not remove either primary flag or establish allocation ownership or performance acceptance.

- [Earlier eed2abe9 native qualification, 2026-10-27](RedesignCompletion_2026-10-27/qualification-eed2abe9/README.md) — six builds, three x64 Full runs with 65 cases and 23 capture pairs each, real ASan detection and an independent 21-check artifact/case audit. ARM64 remains cross-build only; hosted RedTilde ARM64 is open.
- [Earlier eed2abe9 keyboard qualification, 2026-10-27](RedesignCompletion_2026-10-27/input-eed2abe9/README.md) — nine stages, 168 real keyboard events and recorded initial focus; foreground/focus/pointer restoration is unchanged. IME/assistive-technology and performance acceptance remain separate.
- [Editor persistence observer evidence, 2026-10-27](RedesignCompletion_2026-10-27/editor-persistence-eed2abe9/README.md) — retained 798 Debug failure, controlled sharing discriminator and source-bound eed requalification. The historical cause remains unattributed; the observer waits for writer idle/success before one read, with no production storage-policy change.
- [Earlier eed2abe9 three-reference performance comparison, 2026-10-27](RedesignCompletion_2026-10-27/performance-eed2abe9/README.md) — 576 processes, 72 rows, 25,120 canonical audit checks and 13,322 separate raw replay checks. Two original-UI flags remain: Compose command +8.64% (unstable controls) and Split working set +2.027% (stable controls). Actual PR18-base A/B has zero regressions; 36/72 control rows are unstable and no quiet CPU window is verified. No performance acceptance is claimed.
- [Previous a29c9f84 native checkpoint, 2026-10-26](RedesignCompletion_2026-10-26/qualification-a29c9f84/README.md) — three x64 profiles pass 65 cases/23 captures, x64 ASan detects the expected fault, and ARM64 is cross-build only. SDK Debug Menu attempts and the merged PR112 hosted checks are documented for that checkpoint. Its [pending metadata continuation](RedesignCompletion_2026-10-26/qualification-a29c9f84/metadata-continuation-pending.json) is explicitly incomplete.
- [Earlier source-ink renderer diagnostic, 2026-10-27](RedesignCompletion_2026-10-27/source-ink-descriptive-1fb2fbbc/README.md) — separate 16-process precursor, 12 unresolved metric rows, and 4,263,527 independent checks. Descriptive only; it did not establish renderer adoption or performance acceptance. Later exact-source qualification and the complete eed2abe9 comparison remain separate.
- [Previous a29c9f84 three-reference comparison, 2026-10-26](RedesignCompletion_2026-10-26/performance-a29c9f84/README.md) — historical 576-process cohort with one original-UI Split CPU finding (+11.5179672254146%) and 36/72 unstable controls. Keep it separate from eed2abe9 and the earlier 09f cohort; no performance acceptance is claimed.
- [Fresh 45c2895 native checkpoint](RedesignCompletion_2026-10-26/qualification-30fb0c2/final-45c2895/README.md) — six fresh builds at app 45c2895 / SDK 4077f57d; three x64 Full profiles pass 65 cases and 23 captures each, x64 ASan detects the intentional use-after-free, and ARM64 evidence is cross-build only. Separate keyboard run: nine stages, 168 injected events. Performance flags, SDK policy acceptance, cursor lease and hosted ARM64 remain open. Packet manifest SHA-256: `05F60838E546D4069E762C0D8C722E01D36C4D6FC001A3FBE6B8C7905B6D7867`.

- [30fb0c2 native qualification](RedesignCompletion_2026-10-26/qualification-30fb0c2/README.md) — six local build profiles, three x64 Full profiles (65 cases and 23 captures each), true ASan detection and separate 9-stage/168-event keyboard run. ARM64 remains cross-build only; hosted run 40320880695 started zero steps under the payment/spending restriction. Performance and policy gates remain open. Packet manifest SHA-256: `5920C7EE69B941A8680AAD9F34ACC561A74E70F19C26D91A433F4AB0B7D53FD7`.
- [30fb0c2 editor-preparation attempts](RedesignCompletion_2026-10-26/editor-attempts-30fb0c2/README.md) — four current-only traces and 16 matched earlier frame reports retain all attempts. This is work-progression/frame timing evidence without stage CPU attribution or performance acceptance. SDK policy acceptance, cursor lease and all three original-UI performance flags remain open. Packet manifest SHA-256: `55F8459818FE820C75887CBB81BFAD81F2E2BE03A5FDDA3133F61B130870B1F3`.

- [Fresh final 09f performance collection, 2026-10-26](RedesignCompletion_2026-10-26/performance-09f22877/README.md) — 576 processes across three separate lineages, with independent raw replay and 25,202 audit checks. Three original-UI flags remain; 36/72 controls are unstable, all quiet CPU windows are unverified, and the actual PR #16-base comparison has no resolved finding.

Reviewed RedTilde measurements and actual application captures belong here. Generated build/test logs and
intermediate images stay under `.build`. Do not use DxUi or another Red application's receipts as product
qualification. Retained packets record source identity, dependency, binary hash, profile, scenario and limitations.

- [Earlier completion evidence, 2026-10-26](RedesignCompletion_2026-10-26/README.md) — exact SDK header-extension
  support, six-profile native matrix, nine-stage keyboard run and fresh 576-process three-reference collection.
  Four original-UI flags remain, 39/72 controls are unstable, and hosted RedTilde ARM64 stays open.
  Its separate four-process heap diagnostic archives exact stack joins and request-path evidence with explicit
  observer, decoder, module-lifetime and phase-alignment limits.
  A fixed 16-process mirror feature-removal diagnostic retains exact raw reports and negative behavior checks;
  its descriptive CPU difference guides the optional syntax-sharing implementation, not acceptance.
- [Final local 9025be4 matrix and performance study, 2026-10-25](RedesignCompletion_2026-10-25/README.md) — six local
  builds and three noninteractive x64 Full profiles on SDK `2fed8246`, plus 576 reports across separate original-UI,
  starting-redesign and actual PR13-base lineages. Five original-UI A/B regressions remain; actual PR13-base A/B
  has no resolved change and does not close the original-UI gate. Hosted CI, focus, native ARM64 and performance
  acceptance remain separate.
- [9025be4 interactive keyboard input](RedesignCompletion_2026-10-25/input-9025be4/README.md) — nine keyboard stages
  pass with 168 injected events under the authorized interactive lease; broader focus and physical IME/AT coverage
  remain separate.
- [Earlier keyboard qualification at da8aafb](RedesignCompletion_2026-10-24/keyboard-da8aafb/README.md) — all nine
  stages pass at app `da8aafb` / SDK `baff43d8` through the authorized canonical countdown, with 168 real events
  and verified desktop restoration. The earlier setup failure and pre-launch prompt timeout remain separate.

- [Fresh SDK actual-base comparison, 2026-10-25](RedesignCompletion_2026-10-24/sdk-594d67fb/README.md) — exact
  mirror of the library's 78-row/144-receipt packet. Zero flags coexist with 69/72 unstable controls, observer
  cadence breaches and an unqualified policy; this is library evidence with an inconclusive verdict.

- [Final production-code redesign comparison, 2026-10-25](RedesignCompletion_2026-10-24/performance-138a648/README.md)
  — 384 processes, all 48 primary/control rows and 16,003 independent checks at app `138a648` / SDK `baff43d8`.
  Five original-UI flags remain; the starting-redesign comparison has zero flags and four improvements.
  Common-SDK baseline overlays, observed host load, controls and performance advice remain explicit.

- [Final command-phase diagnostic, 2026-10-25](RedesignCompletion_2026-10-24/command-phases-138a648/README.md)
  — a separate 16-process, two-block study retains all nine phases and 36,864 raw phase values. Descriptive
  Selection deltas are not causal attribution, an achievable saving or performance acceptance.

- [Final local redesign matrix, 2026-10-25](RedesignCompletion_2026-10-24/qualification-fa492ab/README.md) — six
  builds and three x64 55-case/23-capture runs at app `fa492ab` / SDK `baff43d8`. Hosted ARM64 remains open.

- [SDK baff43d8 paired evidence, 2026-10-25](RedesignCompletion_2026-10-24/sdk-baff43d8/README.md) — exact mirror
  of the canonical library packet: 78 rows, zero regression findings, unstable controls and unqualified policy.

- [Earlier frozen redesign comparison, 2026-10-25](RedesignCompletion_2026-10-24/performance-fd24623/README.md) — all 48 A/A and A/B metric rows at app `fd24623` / SDK `cfe875fd`, with 384 native runs and 16,003 independent checks. Five original-UI regression flags remain; the actual starting-redesign comparison has zero regressions and three improvements. Controls, observed host load and verified audit-file hashes remain explicit; final qualification is pending. Later source fixes are not measured by this packet.

- [Resource and stage diagnostics, 2026-10-25](RedesignCompletion_2026-10-24/resource-and-stage-fd24623/README.md) — 24 hardware/WARP resource processes and 16 separate stage processes at the same frozen source. Every cost/memory cell and stage median is retained. Results are descriptive, initial preparation excludes initialization, and no allocation attribution or cost acceptance is claimed.

- [Earlier redesign completion comparison, 2026-10-24](RedesignCompletion_2026-10-24/performance-fa0757d/README.md) — retained historical 48-row projection with seven original-base regressions. It is not pooled with the later collection.

- [Design refresh integration, 2026-10-24](DesignRefresh_2026-10-24/README.md) — historical optimization and publication checkpoint, retained separately from the later complete comparison and its exact source bindings.

- [Image-2 visual fidelity, 2026-10-23](VisualFidelity_2026-10-23/README.md) — matched hidden report captures, source-bound six-profile build evidence and three x64 53-case/23-capture runs for the recorded SDK revision. Later dependency pins require their own qualification.

- [Design refresh, 2026-10-22](DesignRefresh_2026-10-22/README.md) — historical v5r3 six builds, three x64 Full 52-case/20-capture runs and the nine-stage keyboard slice. Its eight-regression cohort and earlier failures remain preserved separately from the latest comparison.

- [S5 local execution and final assessment, 2026-10-21](S5_2026-10-21/Final/README.md) — six current-toolchain
  builds, 25 cases per x64 profile, 192 paired processes, independent numerical/integrity audit, eight
  default/WARP lifecycle runs and exact capture comparison. No directional change resolved; noisy controls,
  competing activity, policy acceptance and longer-term GPU/physical/platform qualification remain open.

- [S5 noise follow-through, 2026-10-21](S5_Noise_2026-10-21/README.md) — revised native instrumentation,
  fixed-scope observed A/A and A/B studies and the assessment of remaining control/host uncertainty.

- [Earlier S5 continuation checkpoint](S5_2026-10-21/README.md) — historical handoff, original invalid/corrected
  statistical results, old-toolchain frozen baseline and independent private backup. Superseded by the final assessment.

- [Creation and initial validation, 2026-10-20](Creation_2026-10-20/README.md) — six builds, 14 behavioral cases
  in all three x64 profiles, preference shutdown probes, sanitizer detection, local crash reporting,
  portable extraction and actual application captures.

- [Native math, broader Mermaid and recovery, 2026-10-20](MathMermaidRecovery_2026-10-20/README.md) — six fresh
  application and library builds, 20 cases in each x64 profile, 57 noninteractive library suite passes,
  crash/restart and discard failure/retry, actual math/diagram/recovery captures, portable extraction and
  paired Release diagnostics with explicit performance limitations.

Prior pre-1.0 packets retain their original experimental/baseline context and separate qualification limits.
