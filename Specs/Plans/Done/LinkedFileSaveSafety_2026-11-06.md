# Linked-file save safety

Status: DONE — Q12 linked-file implementation and source-specific local/hosted verification.
Implementation: `020cb0b9480e4560d61d4b2b181a3793cb4a7ef9`.
Dependency: API 4 `ba94f4eb0cd8bbdeecb0b96d5e21fb325fe2d9c9`.

- [x] Preserve the original save-success counterexample that split hardlink identity and left the other alias unchanged.
- [x] Guard in-place shared-target writes with complete identity/expected-byte checks and an independent durable
  full-source recovery record; retain protection and the old saved baseline on failed/partial writes.
- [x] Preserve hardlink aliases and follow symbolic targets while retaining the symbolic entry.
- [x] Verify pre-write, partial-write, flush, recovery admission/retirement and late-alias boundaries independently.
- [x] Terminate an owned child during an actual partial write and verify alias identity, partial target bytes,
  complete recovery source and released lease in a fresh process.
- [x] Retain all six zero-warning builds, three local x64 runtime cohorts and actual ASan detection. Each local
  cohort retains its symbolic-link fixture failure (Win32 1314); those incomplete cohorts are not relabelled.
- [x] Verify actual symbolic-link cases in hosted native x64/ARM64 Release and Debug, and both ASan Storage
  partitions. Release/Debug and x64 ASan each pass all 117 cases and 36 captures. ARM64 ASan's later heap-fixture
  failure remains failed and separately owned; it does not erase its completed 25-case Storage partition.
- [x] Publish original wrapper, canonical member and production-package hashes, plus the failed ASan disposition.
- [x] Keep durable behavior in file-safety, build and validation contracts and usage documentation.

The [packet](../../../Measurements/LinkedSaveAndCosmeticSettlement_2026-11-06/README.md) retains exact identities.
Release source `4e85948b` and extended source `48f1f447` contain this implementation. This closeout does not
qualify later interaction/clipboard candidates, real cloud/SMB/WSL locations, physical input/AT or paired
performance. Those and the repaired heap fixture's new ARM64 ASan execution remain in the
[single active plan](../WIP/RemainingWork_2026-11-04.md).
