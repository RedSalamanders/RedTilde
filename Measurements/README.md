# Application evidence

Reviewed RedTilde measurements and actual application captures belong here. Generated build/test logs and
intermediate images stay under `.build`. Do not use DxUi or another Red application's receipts as product
qualification. Retained packets record source identity, dependency, binary hash, profile, scenario and limitations.

- [Design refresh, 2026-10-22](DesignRefresh_2026-10-22/README.md) — current v5r3 six builds, three x64 Full 52-case/20-capture runs and the nine-stage keyboard slice pass. Its completed fixed AA/AB cohort retains eight AB regressions and shared-host limitations; performance optimization/advice and R10 stay open. Historical failures/cohorts and parent product/platform holds remain distinct.

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
