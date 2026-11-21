# Governance, Tooling & Production-Review Reconciliation — 21 November 2026

Source state: `ea111980a9aa3f9db0a8b49880b751c437e853fa` (`RedTilde` `main` after post-merge changelog fold PR #147).
Purpose: Reconcile open `T-G1` administrative items in [RemainingWork_2026-11-04.md](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md), historical [ProductionReview_2026-10-28.md](../../Specs/Plans/Done/ProductionReview_2026-10-28.md) items, [reduced historical retention](../../Measurements/RedesignCompletion_2026-10-28/retention-20261028.md), [Q15 tooling retirement](../../Specs/Plans/Done/ToolingRetirementAudit_2026-11-08.md), and Gate 3 virtual-PDF printing coverage.

## 1. Post-Merge `Changes/` Cadence (`T-G1`)

- All three pending fragments from PRs #145 and #146 were folded cleanly via `Tools/Submit-ChangelogFold.ps1` into PR #147 (`ea111980`), leaving `Changes/` with only `Changes/README.md`.

## 2. Reconciled Historical Production Review Dispositions (`PR-01`..`PR-46`, `S1`..`S16`)

- **Closed in `main` (`<= ea111980`):**
  - **File safety, recovery & exceptions:** `PR-01`..`PR-14`, `PR-17`..`PR-21`, `PR-24`, `PR-26`, `PR-27`..`PR-32`, `PR-34`..`PR-38`, `PR-42`..`PR-46`. Production-only Release builds omit `REDTILDE_ENABLE_TESTS` (verified in `Directory.Build.props` / `production-*.json` `testsEnabled: false`, 16/16 test switches exiting `2`).
  - **UI & polishing:** `PR-15` IME/draft modal command guards, `PR-22` CommonMark fence rules (fixing ```` ```code``` ```` false fences; remaining 128-heading bound / Setext parity stays explicitly documented/deferred), `PR-23` localized `STRINGTABLE` resources (`Tooling.Tests.ResourceStrings.ps1`), `PR-39`–`PR-41` (`CloseDocument` `Ctrl+W`, `WM_DROPFILES`, `Minimize` 0×0 early return, Find case/F3/Shift+Enter).
- **Intentionally retained / deferred (not hidden regressions):**
  - `S4` / `Q10`: Label-cache removal remains gated on paired non-regression evidence under Gate 6.
  - `S7`: Opt-in `--control-diagnostics` compiles zero timing reads when disabled; left available for paired studies.
  - `S10`: `Measurements/` packets stay sanitized in git per repository policy; raw dumps/recordings stay in `.build/` or owner-authorized `.Perf` storage.
  - `S12`: 13 reviewed `#pragma warning(disable : 4625 4626 5026 5027)` scopes retain their `2027-01-31` review date cleanly under `/Wall`.
  - `S13` / `PR-25`: Image request deduplication and API-6 adoption (`H-01`/`H-02`/`H-05`/`H-14`) remain queued for RedHome D20 / Gate 7.

## 3. Rejected Worktree/Cache Cleanups & `Q15` / `Q17` Dispositions

- **Rejected cleanups ([retention-20261028.md](../../Measurements/RedesignCompletion_2026-10-28/retention-20261028.md)):** The four clean task worktrees and three object-directory cleanups that hit `blocked by policy` on 28 October remain **un-touched** as recorded; no later task may treat an earlier refusal as permission to delete third-party/owner caches or non-`.build` worktrees.
- **`Q15` shared-tooling migration & V1/V2 validator retirement:**
  - Static performance validation still passes `4,772` assertions cleanly (`Tests/Scope.Tests.Performance.Tooling.ps1`).
  - As established in [ToolingRetirementAudit_2026-11-08.md](../../Specs/Plans/Done/ToolingRetirementAudit_2026-11-08.md), retiring `Assert-RedTildeReviewedHeaderDeltaProfileV1` / `V2` and migrating copied CI/harness modules requires modifying the pinned shared `.github` CI engine or cross-repository consumers — explicitly outside Wave 1 (`Cross-Repo Follow-Up` excluded). Retained intentionally until coordinated across sibling repos.
- **`Q17` privacy/history rewrite:** Remains on **explicit owner hold**. History audit (`Measurements/HistorySnapshotAudit_2026-11-08/README.md`) is complete (524 commits / 255 refs scanned with Gitleaks; zero unresolved secrets), and no history rewrite or force-push runs without explicit owner execution.

## 4. Gate 3 Printing Status Check (`T-U2`)

- Local temporary driver directories (`.build/printing-20261118` and `.build/print-preview-qualification-*`) are ephemeral `.build` working outputs already cycled out after prior local runs.
- Deterministic native printing coverage (`Editor.PrintingPreservesRevisionAndHonorsPageRanges`, `Editor.PrintingFailureAndShutdownRetireJobs`, `Editor.PrintingDiagramFollowsCanvasAndPaper`, `Editor.PrintingCloseOverridesPendingPrintAction`) passes across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` profiles (local `x64` profiles and hosted `de873ee` `x64 Release` + `ARM64 Release`/`Debug`/`ASan Debug` runs `#44928555514`, `#44930332182`, and `#44930335298`).
- **Open for Gate 3 (requires owner-agreed desktop time & physical hardware):** Physical sheet output on `Brother HL-L2445DW Printer`, secondary virtual driver (`OneNote (Desktop)` or XPS) + live OS `Print` / `Save Print Output As` PDF file inspection & modal focus restoration receipt archived permanently under `Measurements/`.
