# Hosted fixture build-admission version

Status: DONE — isolated tooling implementation, regression and authoritative documentation

- [x] Preserve the original main x64 qualification failure and reproduce its exact admission refusal under inherited product version 137.
- [x] Give every synthetic artifact-admission probe an explicit version matching its version-zero mock executable.
- [x] Retain the hostile inherited version for mock builds and add an explicit mismatched-version admission refusal.
- [x] Keep source, package-header, compiler, SDK-source and dependency-redirection negative controls under a matching version.
- [x] Pass all 41 scoped-tooling cases and complete validation under inherited version 137; pass formatting and the change-fragment check.
- [x] Merge the durable fixture requirement into [the validation contract](../../Testing/Testing_Validation.md).

The [evidence packet](../../../Measurements/HostedFixtureVersion_2026-11-04/README.md) retains the source
and raw-log identities. No native application source, runtime build-admission rule, dependency pin or product
version policy changes. This closes the fixture implementation, not the combined hosted main gate: a fresh
successful x64/ARM64 qualification is still owned by T-Q1 in the [active plan](../WIP/RemainingWork_2026-11-04.md).
