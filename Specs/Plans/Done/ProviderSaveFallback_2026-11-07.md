# Provider save fallback — completed implementation scope

Status: complete for the recovery-protected provider-refusal backend, local qualification
and exact-source hosted native/production qualification.

An unsupported destination replacement may reuse the guarded in-place writer only after
independent durable complete-source recovery. The writer rechecks identity/content under
its cooperative handle, retains protection and the previous saved baseline after failure,
and changes the saved stamp only on success. Other unsupported statuses do not authorize it.

The normative contract is [file safety](../../Core/Core_FileSafety.md). The
[qualification packet](../../../Measurements/ProviderSaveFallback_2026-11-07/README.md)
retains four independent provider cases, original before/after images, six zero-warning
builds, 131 passing cases plus the actual symlink-privilege failure per x64 profile,
restart protection, hidden captures and actual ASan detection at `33ea112b`.

Main `3bf88036` passes all six hosted native profiles: 132 cases and 36 captures
per profile, including all four provider cases and actual symbolic-link saves.
Original payloads, ASan detection and interrupted-write recovery are independently
verified. Both test-disabled `0.1.365` packages pass their original smoke checks and
thirteen actual refusals each after the combined Release test gate. See the
[Debug/ASan packet](../../../Measurements/ProviderSaveFallback_2026-11-07/hosted-debug-asan.json)
and [Release/production packet](../../../Measurements/ProviderSaveFallback_2026-11-07/hosted-release.json).

Real provider/share/WSL locations, physical-input acceptance, API-5 adoption and
performance-policy acceptance remain in the [current plan](../WIP/RemainingWork_2026-11-04.md).
This scoped completion does not accept them.
