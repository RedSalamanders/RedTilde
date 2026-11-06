# Production launch and portable packaging boundaries

Implementation: `526cc8db2d2716d0a86c37c1ce92c992b0e7475b`.
Dependency: API 4 `a1915ffdd94b18fd1817371f1dc074f6a0b547f1`.

[Qualification](qualification.json) verifies all 141 original compiled inputs and every retained
application/SDK/native-run archive member. All six builds have zero warnings and errors. Each x64 Release,
Debug and ASan cohort passes 127 cases and all 36 captures; the actual symbolic-link fixture fails with Win32
1314 on this machine. Those local cohorts remain incomplete. Actual ASan detection and interrupted-write
crash/restart checks pass. ARM64 entries are cross-builds.

The additional actual x64 ProductionOnly build is warning-free. Its clean-extracted portable ZIP renders the
positive hidden capture and refuses all thirteen test-only cases with exit 2, leaving each initially absent
data directory absent. The cases cover self-test/list/scope, interactive test, deliberate crash, recovery
crash/restart, preferences, sanitizer, linked-save crash/restart and two missing-operand cases. Original ZIP,
executable, production-native and package-evidence archive identities are retained.

The original published `0.1.342` production ZIP provides the counterexample: `--preferences-probe` exits 0
and creates a 285-byte preferences file in its owned fixture directory. The same intended refusal now succeeds
in the actual candidate package. No real user profile or foreground input is used.

All compiler-free validators and formatting pass, including 44 scoped-tooling cases. Two public-command
regressions verify that default packaging requests ProductionOnly and that default/SkipBuild paths explicitly
reject a test-enabled artifact before creating staging. Ordinary test-enabled Release remains available for
regression execution. Supported hidden capture/performance diagnostics remain product capabilities; the
older blanket demand to remove every `Tests/` source from every Release role is superseded by that ownership.

Durable requirements are in [the build/CI contract](../../Specs/Build/Build_UnifiedCI.md) and
[validation](../../Specs/Testing/Testing_Validation.md); [portable usage](../../docs/Portable.md) explains the
command behavior. The [scoped Done record](../../Specs/Plans/Done/ProductionProbeBoundaries_2026-11-06.md)
closes implementation and these local/x64-production checks. Native ARM64 runtime/production, physical
input/AT, application API-5 migration and performance stay in the
[single active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
