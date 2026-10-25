# Redesign completion evidence

These packets retain separate observations; they do not establish performance acceptance or close the WIP plan.
Each packet names its exact source, dependency and executable. Later edits require their own qualification.

- [Final production-code local matrix](qualification-fa492ab/README.md): app `fa492ab` at SDK `baff43d8` passes
  all six builds and all 55 ordinary cases in each x64 profile, with real ASan detection and 23 captures per
  profile. ARM64 is cross-build only; hosted execution, current input and performance acceptance remain open.
- [SDK paired comparison at baff43d8](sdk-baff43d8/README.md): all 78 metric rows against the actual library
  PR base, with zero regression findings, 71/72 unstable controls and unqualified policy. The canonical packet
  lives in DxUi; this exact mirror preserves the measured source and distinct test-executable boundary.
- [Fresh frozen comparison](performance-fd24623/README.md): 384 native processes, all 48 metric rows and 16,003
  independent audit checks. Five original-UI regression flags remain; the actual starting-redesign cohort has no
  regression findings and three improvements. Loaded-host observations and control instability remain explicit.
- [Resource and stage diagnostics](resource-and-stage-fd24623/README.md): 24 hardware/WARP resource processes and
  a separate 16-process stage study. Initial preparation excludes application initialization. Mixed results are
  descriptive; neither study attributes the retained allocation or accepts a cost.
- [Earlier comparison](performance-fa0757d/README.md): preserved historical projection with seven original-base
  regression flags. It is not pooled with the fresh collection.

The new projections bind their numerical audit files by SHA-256: 7,988 checks for the original-lineage comparison, 8,015 for the starting-redesign comparison, the corrected resource-cost audit, and 5,589 checks for the stage audit. The packet readmes and structured JSON identify each digest.
The frozen functional matrix and input receipts remain source-bound in the coordinator's qualification checkpoint.
Final-source regressions, build/runtime/platform validation and performance dispositions remain open. RedTilde's
hosted native ARM64 gate remains open at the owner's request after GitHub refused to start the workflow.
- [SDK paired studies at cfe875fd](sdk-cfe875fd/README.md): complete local same-build AA and hosted actual-base summaries, all 156 metric rows, independent numeric audits and control limits (AA controls 24/24 unstable per scenario; hosted 23/24, 23/24, 22/24); AA does not qualify a noise floor; review-required and unqualified.
