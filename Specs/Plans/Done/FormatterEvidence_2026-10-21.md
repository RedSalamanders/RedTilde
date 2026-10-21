# Formatter evidence identity

Status: DONE. Owner: [validation contract](../../Testing/Testing_Validation.md).

- [x] Include the actual selected formatter path and binary hash in tooling evidence.
- [x] Add an independent check against the resolved formatter bytes; retain selection behavior and native scenarios.
- [x] Complete local validation and exact candidate CI checks; move the implementation record to Done. Final prose checks gate merge.

The scoped testing implementation remains completed. This follow-up closes an environment-identity omission found while qualifying RedPrism on a host with another formatter version. Source, existing measurement packets, the DxUi pin and native behavior are unchanged. Shared runner input changes still require the enabled PR qualification; no forthcoming CI is counted as passed.

Local validation passed all 24 scoped regressions, existing performance/evidence fixtures and formatting. Implementation candidate f15698b passed all eight checks in [run 39640301316](https://github.com/RedSalamanders/RedTilde/actions/runs/39640301316), including six distinct actual successful Full native qualification executions. Final documentation-only qualification may retain this evidence only for matching native inputs and runner image; a changed image requires fresh qualification. [PR #7](https://github.com/RedSalamanders/RedTilde/pull/7) records the final exact candidate checks.
