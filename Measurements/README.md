# Application evidence

Reviewed RedTilde measurements and actual application captures belong here. Generated build/test logs and
intermediate images stay under `.build`. Do not use DxUi or another Red application's receipts as product
qualification. Retained packets record source identity, dependency, binary hash, profile, scenario and limitations.

- [Fresh redesign comparison, 2026-10-25](RedesignCompletion_2026-10-24/performance-fd24623/README.md) — all 48 A/A and A/B metric rows at app `fd24623` / SDK `cfe875fd`, with 384 native runs and 16,003 independent checks. Five original-UI regression flags remain; the actual starting-redesign comparison has zero regressions and three improvements. Controls, observed host load and verified audit-file hashes remain explicit; final qualification is pending. Later source fixes are not measured by this packet.

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

The rich-text dependency remains experimental after PR #89's merge; native ARM64, physical input/AT, production paired performance and release
handoff are separate gates.
