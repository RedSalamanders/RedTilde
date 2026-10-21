# Scoped testing

Status: ACTIVE; user authorized implementation on 2026-10-21.
Owner: [validation contract](../../Testing/Testing_Validation.md)

## Implementation resumed 2026-10-21

- [x] Re-inventory current default-branch sources and live GitHub workflows; preserve unrelated primary/continuation checkouts.
- [x] Migrate contextual active native names and inventories; preserve historical evidence and legacy source.
- [x] Implement explainable affected selection and exact local result/build attestation.
- [x] Add source/index baselines from successful default affected runs; explicit partial runs and failures do not advance them.
- [x] Retain complete explicit Full and pre-push coverage with CI_PENDING distinct from passed evidence.
- [x] Define one independent tooling job and six exact native PR profiles; no duplicate identical main-push matrix.
- [ ] Complete focused tooling/format verification and native/CI qualification; retain every failure.
- [ ] Review the final diff, publish/merge the PR after its exact candidate checks pass, and move this plan to Done.

The current private-repository GitHub account rejects branch-ruleset access with its plan/visibility restriction. Do not change visibility or claim protected checks are enforced. Explicitly verify all named candidate checks before merge.

Selection, exact evidence, source/index mutation, runtime closure, Unicode rename discovery, native ownership and clean/dirty/concurrent CI delegation have executable fixtures. Native filenames and group IDs do not rewrite legacy golden expectations. Foreground/clipboard desktop operation is not used locally by this tooling work; hosted native runners own their desktop. Resource measurements are not rebaselined and no production performance claim is made.

## Qualification record

- Local validate.ps1 and format.ps1 -Check passed; all 23 scoped regressions passed, including rejection of repeated expected CI profiles.
- Initial candidate 6b413ae passed all eight checks in [run 39623188961](https://github.com/RedSalamanders/RedTilde/actions/runs/39623188961), including all six actual native profiles, all 27 partitioned cases, and real ASan detection on x64 and ARM64.
- The final evidence uniqueness guard changes shared runner inputs and remains subject to the final candidate CI matrix. This plan stays active until that qualification and merge review complete.
