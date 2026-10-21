# Formatter evidence identity

Status: ACTIVE. Owner: [validation contract](../../Testing/Testing_Validation.md).

- [x] Include the actual selected formatter path and binary hash in tooling evidence.
- [x] Add an independent check against the resolved formatter bytes; retain selection behavior and native scenarios.
- [ ] Complete local validation and exact candidate CI checks, then move this bounded follow-up to Done and merge.

The scoped testing implementation remains completed. This follow-up closes an environment-identity omission found while qualifying RedPrism on a host with another formatter version. Source, existing measurement packets, the DxUi pin and native behavior are unchanged. Shared runner input changes still require the enabled PR qualification; no forthcoming CI is counted as passed.
