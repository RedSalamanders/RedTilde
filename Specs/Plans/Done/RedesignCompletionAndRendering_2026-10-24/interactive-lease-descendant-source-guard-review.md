# InteractiveLease descendant cleanup source-guard review

Review date: 2026-10-24. Scope: read-only review of the changed source guard and its native behavioral-test linkage. No tracked files were edited; no native executable or child process was launched.

The guard change is narrowly scoped to the one `TerminateProcess(descendant.get(), ...)` in `Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp` inside `SelfTest::DescendantProcesses`. It requires the call to remain between that fixture's method start and the next `FakeHostedInputOperations` declaration, requires `GetProcessTimes` plus the failed-identity `continue` and mismatched-creation-time `continue` to precede it, and still requires exactly one such exception. Every other matched `TerminateProcess` call in the six reviewed source files must use `process.hProcess`; process discovery APIs and broad foreground grants remain prohibited. The guard also checks the caller-level contract: the self-test is launched with `--self-test`, its process must exit zero, and it must report one PASS summary with no FAIL rows. In the runner, `SelfTestRun` invokes `DescendantProcesses()` before returning the self-test result (runner lines 541-546), so that invocation propagates the behavioral assertion failures.

A private AST probe extracted and executed only the changed `Invoke-TestCase` script block against copied fixture files under `.build/audit-20261024/interactive-lease-source-guard-d4dbebbc0c8c4dbead7266d3e790dbd6`. Results: baseline passed; an unrelated `TerminateProcess(otherProcess, ...)` mutation was rejected; removing the creation-time mismatch barrier was rejected. This checks source-contract sensitivity only; it is not the native self-test or suite. Probe copies remain in that private directory.

Disposition: no concrete guard regression found. One bounded limitation: the source assertion recognizes the required identity-barrier expressions textually rather than proving Windows process ownership; runtime confidence continues to come from the exact-PID/creation-time native self-test and the official run. This is not an atomic process-tree join guarantee; it only verifies completion of the retained PID/creation-verified descendant handles in the fixture.

Current file identities:
- `Tools/tests/Test-InteractiveLease.ps1`: SHA-256 `F529C07D1E2AA17F5649B05C827B6DEC759C760BCF104C6E924CDD9740DBE52E`
- `Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp`: SHA-256 `EA5693C4680FAE44210CA288585290AD887F394546155CD308F47EDD9EA32165`
