# Application evidence

Reviewed RedTilde measurements and actual application captures belong here. Generated build/test logs and
intermediate images stay under `.build`. Do not use DxUi or another Red application's receipts as product
qualification. Retained packets record source identity, dependency, binary hash, profile, scenario and limitations.

- [S5 continuation checkpoint, 2026-10-21](S5_2026-10-21/README.md) — tested edit-work reduction, completed-frame
  harness, retained invalid/corrected statistical results, v2 frozen baseline, six builds and 25 cases per x64
  profile. Full revised paired/resource studies and qualification remain open; independent private backup included.

- [Creation and initial validation, 2026-10-20](Creation_2026-10-20/README.md) — six builds, 14 behavioral cases
  in all three x64 profiles, preference shutdown probes, sanitizer detection, local crash reporting,
  portable extraction and actual application captures.

- [Native math, broader Mermaid and recovery, 2026-10-20](MathMermaidRecovery_2026-10-20/README.md) — six fresh
  application and library builds, 20 cases in each x64 profile, 57 noninteractive library suite passes,
  crash/restart and discard failure/retry, actual math/diagram/recovery captures, portable extraction and
  paired Release diagnostics with explicit performance limitations.

Upstream PR #89 remains experimental; native ARM64, physical input/AT, production paired performance and release
handoff are separate gates.
