# Provider save fallback verification

Implementation: `33ea112bb7fed87e7a1578f1df1c2c4457349697`; exact API-4 SDK `a1915ffd`.

[Local qualification](qualification.json) records six zero-warning builds and 141 original
compiled inputs, with every source/native/SDK/run archive member independently verified.
Each x64 profile passes 131 of 132 cases and retains the actual symbolic-link fixture
failure (Win32 1314), 36 hidden captures, interrupted-write restart and actual ASan detection.
ARM64 entries in that local packet are cross-builds. The original local Win32 1314
failure is retained separately from the successful hosted runs below.

At main `3bf8803637f73e272550495818a5ae10335a1268`,
[Debug and ASan](hosted-debug-asan.json) and [Release](hosted-release.json)
each pass on native x64 and ARM64: 132 cases and 36 hidden captures per profile,
792 case executions and 216 captures across all six profiles. All four provider
cases, actual symbolic-link saves, interrupted-write recovery and the ASan detection
probes pass. Every original payload member and downloaded artifact wrapper is byte-verified.

Both `0.1.365` production packages pass after the combined Release test gate.
Their original smoke receipts record all thirteen test/probe refusals per package
with exit 2 and absent data directories, positive smoke, package/executable hashes
and actual PE architecture. These are qualification artifacts; this record does not
publish a GitHub release.

The [original comparison](counterfactual.json) passes one of four provider cases before
the fallback and all four afterward. Actual local Win32 file handles verify full identity,
BOM/EOLs, durable complete-source protection, failed writes/flushes, restart recovery and
conflict revalidation. Only a provider-unsupported result from the replacement operation
admits fallback; the same error from staging or flushing does not.

These directed provider errors do not establish support on a real OneDrive, SMB/NAS or
WSL location. Those remain in the current plan, together with physical-input, API-5 migration
and performance-policy acceptance. Initial private compile and catalog failures are retained.
