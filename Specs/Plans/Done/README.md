# Completed work

Completed plans are history: they record sequencing, rationale, and evidence, never current behavior. A plan is
added here, newest first, in the change that moves it to Done, as a bullet that links its title to its file and
ends with an em dash and its outcome.

- [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md) — completed Gates 1–7 through `RedTilde v1.0.0` (`production`), local 8-build/full-`x64` qualification, sandboxed `Install-RedTilde.ps1` smoke, hosted `Qualify main Release` `#45167960343` (`x64` & `ARM64` `1.0.538` packages, unsigned with `.sha256` sidecars per owner decision A3), `CHANGELOG.md` `1.0.0`, and annotated tag `v1.0.0`.
- [Remaining work and qualification](RemainingWork_2026-11-04.md) — closed `T-S2`, `T-U2`, `T-I1`, `T-P1`, `T-Q2` (`C11`/`C12`/`S6`), and `T-CV1` with source-bound qualification under `Measurements/`, explicit `capabilities.json` `deferred` scope, and `Q15`/`Q17` governance holds (`GovernanceDisposition_2026-11-21`).

- [Side panes and bidirectional split editing](SidePanesAndSplitEditing_2026-11-22.md) — fixed `_panelToggle` hover/selected pill parity (`ButtonVariant::IconOnly` + `SubtleToggleButton`, `IDS_TIP_PANEL`, `Ctrl+Shift+L`), shipped responsive `28`-DIP collapsed edge rails with `24`-DIP hysteresis, `200 ms` `Control::Tick` animations, hover-reveal overlays (`180 ms` dwell / `300 ms` leave grace), modified-file and hierarchical Outline dot navigation + tooltips, bidirectional Split editing (`ExchangeDocumentState` preserving `128`-step unified Undo/Redo, click-to-switch & `F6`), linked/independent Split scrolling (`splitScrollSync` + `Ctrl+Shift+Y`), active-pane Find/Replace without `MatchTable` rebuild, and 5 deterministic Release harness captures (`199/199` native tests + `45/45` captures across `x64 Debug`/`Release`/`ASan Debug`, real ASan detection probe, crash/recovery/performance suites, `validate.ps1`, `format.ps1 -Check`, and all 6 zero-warning builds); physical ARM64 runtime, owner-agreed interactive desktop focus/Narrator UIA pass, quiet-host paired Release performance study, and hosted CI remain separate gates.

- [Production readiness audit and remediation](ProductionReadinessAudit_2026-11-21.md) — remediated all `C1`–`C6`, `H1`–`H9`, `M1`–`M4`, `T1`–`T3`, and `S1`–`S6` audit findings across Print Preview concurrency/drivers/WaitingOverlay, modal file/error reentrancy, 16 MiB/JSON5 settings safety and live reload, OneDrive/FAT32 Files & PDF storage, Protocol-2 failure boundaries, installer/real-location/packaging safety, resource/naming gates, and 8 added regression tests (`188/188` native tests passing across `x64 Debug`/`Release`/`ASan Debug`, six zero-warning builds, verified `x64` production package + post-package validation); physical ARM64 runtime and external/interactive hardware gates remain tracked in companion WIP plans.

- [Explicit state machines and lifetime safety](ExplicitStateMachines_2026-11-10.md) — completed all six slices replacing implicit boolean/optional state machines across `Application`, `AsyncFiles`, `BitmapPasteWriter`, `RecoveryService`, `FilesService`, overlays, Find & Replace, workspace switching, and test-evidence tooling (`RT-SM-01`..`RT-SM-09`), reconciled normative domain contracts, and executed full local qualification; physical ARM64 runtime and owner-authorized interactive focus gates remain separate.

- [API-5 consumer adoption](Api5ConsumerAdoption_2026-11-08.md) — six zero-warning local builds, all three hosted x64 profiles with 465 passing cases and 120 captures, actual symbolic-link saves and ASan detection, plus fifteen local production refusals. ARM64 compilation and broader physical/provider/performance/release gates remain distinct.

- [Document image consent and bounded refresh](DocumentImageConsent_2026-11-08.md) — Q13 implementation, six clean builds, all three x64 nonstorage profiles, reviewed captures and production refusals; real-location and broader adoption gates remain active.

- [Tooling retirement caller audit](ToolingRetirementAudit_2026-11-08.md) — actual V1/V2 callers, sealed V4 identity and passing tooling evidence; retirement/shared-module migration remain active.

## Completed scopes — 7 November 2026

- [Native editing qualification](NativeEditingQualification_2026-11-07.md) — Source Enter/EOLs, native ACP capacity, preview/mirror admission and hostile file-paste names; six local builds, six hosted native profiles with 864 passing cases and 228 captures, and both version 0.1.389 production ZIPs; original local privilege failures remain explicit.

- [Bitmap paste](BitmapPaste_2026-11-07.md) — bounded durable app-owned PNG assets, persisted text/image preference, cancellation and one-step Undo; six local zero-warning builds, six hosted native profiles with 840 passing cases and 228 captures, and both version 0.1.378 production ZIP smoke checks.

- [Provider save fallback](ProviderSaveFallback_2026-11-07.md) — durable recovery before provider-refused replacement fallback, provenance and conflict checks; six builds with the explicit local symbolic-link privilege boundary.

## Completed scopes — 6 November 2026

- [Production probe and packaging boundaries](ProductionProbeBoundaries_2026-11-06.md) — early test-command refusal, ProductionOnly packaging, original write counterexample, six builds and thirteen actual x64 production-package refusals; local symbolic-link and hosted platform gates remain explicit.

- [Find, zoom, window placement and recovery-first startup](RemainingInteractions_2026-11-06.md) — Q9/Q14, confirmed link handoff, independent heap boundaries, six native profile/architecture combinations with 768 passing cases and 216 captures, and verified production ZIPs.

- [Linked-file save safety](LinkedFileSaveSafety_2026-11-06.md) — Q12, durable full-source protection before shared-target mutation, original crash/restart evidence, six builds and actual hosted symbolic-link execution; the later ARM64 ASan heap-fixture failure remains separately owned.

- [Save safety and UI failure boundaries](FileSafetyAndUiBoundaries_2026-11-06.md) — one conflict surface, reliable stamps/full file identity, UTF-8/capacity diagnostics, guarded UI actions, long-checkout fixtures, six builds, 324 cases and 108 hidden captures.

- [Document workspaces, recovery and rendering adoption](WorkspaceRecoveryAndRendering_2026-11-06.md) — retained per-document history/recovery, lossless EOL and storage/settings/Find failures, exact SDK pin, six builds, 273 cases and 108 standard captures.

## Completed scopes — 5 November 2026

- [Selected quote and code insertions](SelectedBlockInsertions_2026-11-05.md) — actual command transformation, exact source/history/focus regressions, preserved counterexample, owned-window activation checks, six builds and three complete x64 profiles.

- [Recovery envelope bounds and deterministic file fixtures](RecoveryEnvelopeBounds_2026-11-05.md) — decoded-capacity refusal, complete quarantine preservation, retained parser/fixture failures, six builds and three complete x64 profiles; broader recovery and native ARM64 acceptance remain in WIP.

- [Close during Save and bounded Editor validation](CloseDuringSave_2026-11-05.md) — durable save settlement, failed-write recovery, safe Open abandonment, lifecycle counterexample, exact bounded case union, six builds and three complete x64 profiles; broader acceptance remains in WIP.

- [Recovery Restore identity](RecoveryRestoreIdentity_2026-11-05.md) — clicked-snapshot binding, missing-copy refusal, retained counterexample and fixture synchronization failure, six builds and three complete native x64 profiles; broader recovery work remains in WIP.

- [Real keyboard and focus qualification](KeyboardAndFocusQualification_2026-11-05.md) — nine named stages including F2, explicit timing agreement, canonical warning/lease and verified foreground/focus/pointer restoration; physical IME/touch/AT/clipboard acceptance stays separate.

## Consolidated records — 4 November 2026

- [Diagram label backgrounds and duplicate Close](DiagramLabelBackgroundsAndClosePrompt_2026-11-04.md) — exact-pin contrast/canvas rendering, original matched captures, prompt reentrancy regression and six-profile scoped qualification; broader correctness and acceptance remain in WIP.

- [Quote inner margins and themed document scrollbars](QuoteMarginsAndScrollbars_2026-11-04.md) — exact-pin x64 Release/Debug/ASan checks, actual hidden captures and scoped UI closeout; broader acceptance stays in WIP.

- [WIP consolidation and complete transfer ledger](WipConsolidation_2026-11-04.md) — one current owner, explicit dispositions and preserved original source identities.

- [DeliveryRedTeam_2026-10-31](DeliveryRedTeam_2026-10-31.md) — superseded planning reference; unfinished scope transferred.

- [DeliveryReview_2026-10-31](DeliveryReview_2026-10-31.md) — superseded planning reference; unfinished scope transferred.

- [DesignRefresh_2026-10-22](DesignRefresh_2026-10-22.md) — completed scoped record; acceptance remains separately owned.

- [DxUiDeliveryAdoption_2026-11-02](DxUiDeliveryAdoption_2026-11-02.md) — superseded planning reference; unfinished scope transferred.

- [NativeBuildPolicyCoherence_2026-10-28](NativeBuildPolicyCoherence_2026-10-28.md) — completed scoped record; acceptance remains separately owned.

- [ProductionReview_2026-10-28](ProductionReview_2026-10-28.md) — superseded planning reference; unfinished scope transferred.

- [RedesignCompletionAndRendering_2026-10-24](RedesignCompletionAndRendering_2026-10-24.md) — superseded planning reference; unfinished scope transferred.

- [RedMonitor_Adoption_2026-10-23](RedMonitor_Adoption_2026-10-23.md) — completed scoped record; acceptance remains separately owned.

- [RedTilde_2026-10-20](RedTilde_2026-10-20.md) — superseded planning reference; unfinished scope transferred.

- [WipFolderQuality_2026-10-31](WipFolderQuality_2026-10-31.md) — completed scoped record; acceptance remains separately owned.

## Earlier completed records


- [Persistent folder navigation, samples and rendering review](FolderNavigationAndSamples_2026-11-03.md) — Open Folder, remembered root, restart/navigation regressions, expanded examples and native code colors; retained rendering gaps and performance/platform gates remain with the active coordinator.

- [Skill package integrity](SkillPackageIntegrity_2026-10-28.md) — bounded structure/UI/reference checks, scoped AGENTS inventory and independent planner/package controls; retained aggregate ARM64 dependency identity failure remains outside this tooling scope.

- [Scoped testing](ScopedTesting_2026-10-21.md) — affected selection, exact success reuse, incremental baselines and qualified native PR coverage.

- [Formatter evidence](FormatterEvidence_2026-10-21.md) — selected formatter bytes participate in exact tooling evidence; qualified in all six native profiles.

Active work is in [WIP](../WIP/README.md).

- [Hosted fixture build-admission version](HostedFixtureVersion_2026-11-04.md) — source-bound counterexample, explicit matching/mismatched version probes and complete tooling validation under a hostile inherited product version; fresh combined hosted main qualification remains in WIP.

- [Test-run evidence retention](TestRunEvidence_2026-10-21.md) — completed workflow implementation; product qualification boundaries remain explicit.

- [MSVC 14.52 and Release optimization](Msvc1452ReleaseOptimization_2026-10-23.md) - local six-profile builds, three Full x64 profiles and reviewed original size/performance evidence; native ARM64 and hosted provisioning remain separate.

- [Hosted MSVC setup followup](MsvcHostedSetup_2026-10-27.md) - explicit GUI completion, exact tool checks, diagnostic retention and passing local tooling; hosted delivery limits remain explicit.

- [PowerShellTooling_2026-10-28](PowerShellTooling_2026-10-28.md) — previously completed historical record.
