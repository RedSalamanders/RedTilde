# Document image consent and bounded refresh

Status: DONE for Q13 implementation and local native/production qualification.

The [source-bound packet](../../../Measurements/Api5ConsumerAdoption_2026-11-08/README.md)
records implementation `9feb787e`, SDK `21fae0dc` (API 5), six zero-warning
builds, all 375 x64 nonstorage case executions, 120 captures, actual ASan and
fifteen actual x64 production-entry refusals. Every original source/native/SDK/run
member verifies. ARM64 is compilation-only. The local Storage privilege failure
remains unwaived; real provider, physical-input and wider adoption gates remain
with the [parent](../WIP/RemainingWork_2026-11-04.md).

The editor and architecture contracts own per-document/local-network permission,
direct anonymous transport, connection-time address checks, bounded source-owned
refresh and truthful refusal reasons. The validation contract owns independent
child inputs, the exact case union and retained first failures. Native captures
use invented content and preserve source/build/DPI provenance.
