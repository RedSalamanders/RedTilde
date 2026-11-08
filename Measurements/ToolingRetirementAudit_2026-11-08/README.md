# Reviewed-header tooling retirement audit

The [source-bound audit](qualification.json) at `9feb787e` finds that the V1/V2
validators are reachable. The exported profile loader dispatches both versions,
the supported paired-study command calls that loader, and the independent
evidence auditor plus tooling fixtures still read the two historical profiles.
The current sealed profile is V4; selecting it does not make those replay paths
dead code.

The complete validator run passes, including 4,772 performance reference/audit
assertions and the V4 positive/negative controls. The ledger retains the nine
actual input hashes, current profile bytes and original full-static output hashes.
This is a caller audit, not a fresh performance comparison or policy acceptance.

Q15 retirement must first migrate the historical replay callers and immutable
profile inputs. Shared-module migration needs an exact shared owner/pin and its
consumer coverage before local copies or separately owned branches can be
removed. No validator, fixture, module or branch was deleted by this audit. Those
implementation steps remain with the [single active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
