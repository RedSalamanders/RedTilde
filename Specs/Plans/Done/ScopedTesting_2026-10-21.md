# Scoped testing

Status: DONE; implementation, native qualification and documentation completed on 2026-10-21.
Owner: [validation contract](../../Testing/Testing_Validation.md)

## Implementation resumed 2026-10-21

- [x] Re-inventory current default-branch sources and live GitHub workflows; preserve unrelated primary/continuation checkouts.
- [x] Migrate contextual active native names and inventories; preserve historical evidence and legacy source.
- [x] Implement explainable affected selection and exact local result/build attestation.
- [x] Add source/index baselines from successful default affected runs; explicit partial runs and failures do not advance them.
- [x] Retain complete explicit Full and pre-push coverage with CI_PENDING distinct from passed evidence.
- [x] Define one independent tooling job and six exact native PR profiles; no duplicate identical main-push matrix.
- [x] Complete focused tooling/format verification and native/CI qualification; retain every failure.
- [x] Review the final diff, publish the qualified PR and move this implementation plan to Done. Merge is gated on the final candidate checks.

The current private-repository GitHub account rejects branch-ruleset access with its plan/visibility restriction. Do not change visibility or claim protected checks are enforced. Explicitly verify all named candidate checks before merge.

Selection, exact evidence, source/index mutation, runtime closure, Unicode rename discovery, native ownership and clean/dirty/concurrent CI delegation have executable fixtures. Native filenames and group IDs do not rewrite legacy golden expectations. Foreground/clipboard desktop operation is not used locally by this tooling work; hosted native runners own their desktop. Resource measurements are not rebaselined and no production performance claim is made.

## Qualification record

- Local validate.ps1 and format.ps1 -Check passed; all 23 scoped regressions passed, including rejection of repeated expected CI profiles.
- Initial candidate 6b413ae passed all eight checks in [run 39623188961](https://github.com/RedSalamanders/RedTilde/actions/runs/39623188961), including all six actual native profiles, all 27 partitioned cases, and real ASan detection on x64 and ARM64.
- Final implementation candidate e2b4812 passed all eight checks in [run 39625942405](https://github.com/RedSalamanders/RedTilde/actions/runs/39625942405). All six distinct native jobs contain an actual successful Full native qualification step. The uniqueness guard and all 23 scoped regression cases passed.

This documentation-only closeout retains that original source/profile qualification; it does not claim new native, resource, physical input or performance execution. S5/S6 and retained baseline studies remain separate. The native harness source identity changed through naming/dispatch updates; no frozen performance evidence is rewritten or silently requalified. Merge of [PR #4](https://github.com/RedSalamanders/RedTilde/pull/4) remains gated on the final prose candidate's tooling and retained-proof checks.
