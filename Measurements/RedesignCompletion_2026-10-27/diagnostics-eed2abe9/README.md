# RedTilde diagnostic evidence

This reviewed packet records the separate private diagnostic studies. It is not an application qualification or acceptance packet.

## Resident-image diagnostic

The fixed study completed eight 64-cycle processes in Compose A-B-B-A then Split B-A-A-B order. The exact reference and candidate commits, SDK pins, source identities, executable and PDB hashes, shared 58-header closure, driver hashes, and replay hashes are in `evidence-binding.json` and `memory-summary.json`. The independent replay verified all eight receipt inventories and per-row arithmetic: 297 snapshots and 160,882 committed-region rows at 4,096 bytes per page. It also checked the source guard, helper/observer parent identity, target creation window, expected frozen target path and image hash, and the 64-cycle diagnostics-off report for each slot.

The summaries report valid-page medians for unknown, MEM_IMAGE, MEM_PRIVATE, MEM_MAPPED, and the `[main-image]` subset. Path-only, path-unavailable, and mapping-cap lookup statuses remain separate from the observer's mapping-cache-completeness flag. The tail samples are near-terminal with unknown phase; they are not primary-phase snapshots. These values show working-set membership at query time and do not identify allocation or heap owners, physical exclusivity, or cause.

A separate first attempt stopped at a module-scope source-identity lookup before starting any child or observer; it produced no application measurement. The completed collection is separate and passed all eight native slots. The stock PowerShell summary step later stopped with an index error before producing its own summary; the independent Python replay passed. That postprocessor error is not a native application failure.

## Formatter diagnostic

The standalone formatter experiment timed 8.4 million calls across 168 rows and matched checksums for all 56 case/sample groups. Its isolated MSBuild operation succeeded with zero warnings and errors, and 21 protected production artifacts remained unchanged. The script-generated receipt failed a strict compiler-path postcondition because one tracking record omits the executable path; the successful build log independently records the pinned compiler. The bounded prototype showed about 30.2–48.5 ns lower medians in six supported cases and no measurable difference in the fallback case. This is a standalone microbenchmark; the prototype was not adopted and no application gain is established.

## Selection profiles

The separately bound v2 and v3 component profiles each contain four fixed Compose/Split runs with 64 cycles and 128 selection transactions per process. They record 16 and 18 instrumented components respectively. Their executable and source identities differ, while both bind the same source revision and SDK pin. These are instrumentation-only timing profiles, not an application comparison or gain claim.

## Production boundary

The unchanged compiled application and runtime producer remain `eed2abe9885c57f44ab192155b341582782020f9` on SDK `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`. Its two primary acceptance flags remain open. The formatter prototype is not adopted. The selection-instrumentation records above do not change the application binary or dependency pin.

## Packet integrity

`manifest.json` seals each packet file by byte count and SHA-256. The manifest intentionally does not hash itself. This packet includes no process identifiers, raw resident tables, raw mapped paths, file IDs, absolute paths, machine names, or unrelated module basenames.
