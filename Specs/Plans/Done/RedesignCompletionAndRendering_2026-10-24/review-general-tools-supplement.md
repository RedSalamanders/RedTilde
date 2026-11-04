# DxUi general Tools tests review supplement

```json
{
  "reviewedCommit": "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
  "sourceCommit": "b06f534b36ffc2c0c9671a142d1e51545480be22",
  "status": "COMPLETE",
  "scope": "All 18 pending Tools/tests rows assigned from review-general-tests-current.md. Full file reads only; no source changes, builds or test runs.",
  "selectedCount": 18,
  "reviewedCount": 18,
  "pendingCount": 0,
  "files": [
    {
      "path": "Tools/tests/Invoke-ToolingTests.ps1",
      "sha256": "657B71F8B5BA586F8FB9DA526BDDE7805D4415855D28943DD53BACBCF870BB15",
      "reviewNotes": {
        "callers": "Top-level tooling-test dispatcher invokes 23 scripts in fixed order, records independent failures, and reports aggregate failure after the loop.",
        "ownershipAndLifetime": "Dispatcher does not own fixture state; each child runs in the same PowerShell process and shares process-level environment/current directory, so per-test cleanup is delegated to its script.",
        "failureAndCancellation": "Per-script exceptions are caught so later suites still run; this can leave global mutations observable to later scripts if a failing script does not restore them.",
        "testCoverage": "Named suites include BuildMatrix, PerformanceComparison, PairedRun, BenchmarkGate, BenchmarkHost, CiGate, CapabilitySkipPolicy, SuiteFailure, InteractiveMode, ConsumerUpdate, and UnicodeGraphemeData. Static aggregator review only; no suites executed."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Invoke-ToolingTests"
      ]
    },
    {
      "path": "Tools/tests/Test-AsanRuntime.ps1",
      "sha256": "C83660650E6A1FBAF482B47B36A603643D991190B4F48EB4CC9318ACADDE12D1",
      "reviewNotes": {
        "callers": "Tooling dispatcher invokes this integration test; it imports the Visual Studio environment helper and builds a synthetic vcxproj through the actual ASan staging target.",
        "ownershipAndLifetime": "Synthetic inputs and staged runtime live under repository .build/ToolTests; the test does not show fixture cleanup, so generated test material persists there.",
        "failureAndCancellation": "Missing runtime DLL is expected to fail staging; after a synthetic runtime is supplied, the staged copy hash must match. Build failures throw immediately.",
        "testCoverage": "Exercises ARM64 target staging and content identity, but this is a static script review; no build/test was run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Test-AsanRuntime"
      ]
    },
    {
      "path": "Tools/tests/Test-BenchmarkGate.ps1",
      "sha256": "5DC494EF0641D65228ABAFC6EFD43A42BAB33E9BA0B281E26063B64A2B16568C",
      "reviewNotes": {
        "callers": "Runs benchmark-gate, receipt/comparator and workflow projection cases against repository scripts plus constructed Git/JSON fixtures.",
        "ownershipAndLifetime": "Fixture helpers create isolated temporary repository roots and receipts; the script uses deterministic seeded schedules and separate base/candidate judge modules. It also reads retained Measurements data as immutable inputs.",
        "failureAndCancellation": "Covers fail-closed invalid evidence, missing controls, mismatched hashes/identity, malformed summaries and workflow wiring. Failure cases distinguish inconclusive evidence from pass/fail; fixtures are removed by shared TestSupport wrappers.",
        "testCoverage": "Fully read 1,721 lines and all case declarations, including exact receipt-byte binding, control drift, exact budgets, Holm/judge policy, retained A/A evidence, workflow events/permissions/timeouts and strict script parsing. No test run performed."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "every measured path is measured",
        "both judges are bound to the exact same retained receipt bytes",
        "the workflow cancels what a push supersedes, bounds its runtime and pins what it runs"
      ]
    },
    {
      "path": "Tools/tests/Test-BenchmarkHost.ps1",
      "sha256": "EE97874BB4A97CCFE1CD151D9F79E1445892458D07272EE73562BE3BE244709A",
      "reviewNotes": {
        "callers": "Imports TestSupport and exercises the hosted benchmark observer through bounded child processes.",
        "ownershipAndLifetime": "Observer child process handle is owned and disposed; async stdout/stderr drains and output directories are fixture-scoped. Original benchmark receipts are asserted unchanged.",
        "failureAndCancellation": "Tests refuse overwrites, failed readiness/stop, exhausted observation budgets and unsupported hosts. Time-bounded child behavior reports completion/exit state.",
        "testCoverage": "Named cases cover continuous observations and retained raw samples, no-overwrite behavior, budget exhaustion, and unsupported-host refusal. No tests run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "hosted study admits continuous observations and retains original native outputs",
        "host observations refuse to overwrite original evidence",
        "observation budget exhaustion retains raw samples and fails admission"
      ]
    },
    {
      "path": "Tools/tests/Test-BuildMatrix.ps1",
      "sha256": "81C1330283C84A25FFEE178792D23A9902CA5ADF10AB294DD62ED2AF1EC81C4C",
      "reviewNotes": {
        "callers": "Exercises the consumer project-matrix validator against temporary project/solution fixtures and a secondary working directory.",
        "ownershipAndLifetime": "Fixture roots are created and removed through TestSupport; project configurations and solution mappings are synthetic.",
        "failureAndCancellation": "Rejects absent/duplicate configurations, silent Debug fallback, missing build mappings and build-0 mappings; validates relative Root resolution independent of caller cwd.",
        "testCoverage": "Named cases verify real configurations, alternate-cwd root resolution and fallback/missing-build rejection. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "every project requires real configuration entries",
        "the consumer validator resolves a relative Root from another working directory",
        "a silent Debug fallback and a missing build are rejected"
      ]
    },
    {
      "path": "Tools/tests/Test-CapabilitySkipPolicy.ps1",
      "sha256": "9EAAAFD4D096CB83958DBDB0A1E2DE6BFD9B98D2288ABA9FDB9CCE2321997EA1",
      "reviewNotes": {
        "callers": "Tests the capability-skip policy validator and parser against source fixture manifests/logs.",
        "ownershipAndLifetime": "Temporary fixtures are scoped through TestSupport; policy JSON is read as test input and no global app process is launched.",
        "failureAndCancellation": "Malformed, duplicate, self-hosted, new/changed and wrong-lane allowances fail closed. Log parsing requires exact suite/test/reason association and skip boundaries.",
        "testCoverage": "Covers x64 Debug and ARM64 ASan lane mismatch, empty/multiple skip boundary cases, and preservation of reporter helper semantics. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Test-CapabilitySkipPolicy"
      ]
    },
    {
      "path": "Tools/tests/Test-CiGate.ps1",
      "sha256": "B0FBE9A7BB2B14DF53ADFFD65190B956BECFDC936453F6876DB134233F1971F4",
      "reviewNotes": {
        "callers": "Invokes the CI-gate script with controlled environment values and fixture status/report files.",
        "ownershipAndLifetime": "Environment changes are restored in finally; synthetic gate outputs live under temporary fixture roots.",
        "failureAndCancellation": "Required check failures, malformed hosted markers, paired/manual requirements and invalid provenance produce explicit refusals; scope/doc-only classifications are checked separately.",
        "testCoverage": "Cases cover complete PR and docs-skip classification, required statuses, native-skip conditions, paired/manual gate requirements and exact baseline/candidate hashes. No tests run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Test-CiGate"
      ]
    },
    {
      "path": "Tools/tests/Test-ConsumerUpdate.ps1",
      "sha256": "F60E8355E619A0954447CB8C3D8B92BFF5BDE5F0C376A56E864AC18928D252DF",
      "reviewNotes": {
        "callers": "Tests the ConsumerUpdate module against synthetic lockfiles and mock request/credential helpers; current script includes the new bounded owned-child case.",
        "ownershipAndLifetime": "Fixture root is removed after each case; the owned credential helper uses its exact Process handle, drains redirected streams and leaves an unrelated sentinel process untouched. Lock bytes/hash are checked before and after.",
        "failureAndCancellation": "New case checks success output only after child completion, nonzero exit refusal, timeout/termination, bounded output handling and no leak; network is mocked and never contacted.",
        "testCoverage": "Reviewed all ten advisory/update scenarios including lock pin preservation and `the owned credential helper is bounded, drained and only returns successful output`. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "the owned credential helper is bounded, drained and only returns successful output"
      ]
    },
    {
      "path": "Tools/tests/Test-InteractiveLease.ps1",
      "sha256": "F4F6121AD5A3E81B71985F66EA0F5873ACD850D71DD7B7C05BE1C94C11201589",
      "reviewNotes": {
        "callers": "Exercises DxUi.InteractiveLease.exe and the PowerShell lease reader/wrapper; interactive UI proof is delegated to native self-test on a private desktop.",
        "ownershipAndLifetime": "The bounded PowerShell harness owns/disposes its direct child Process object; native SelfTest::Run in Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp:532-550 runs ChildProcesses and DescendantProcesses, aggregates failed Check calls, and returns nonzero if any proof fails. DescendantProcesses at lines 686-746 verifies PID plus creation time and waits for exact descendant termination on normal-exit and timeout cases.",
        "failureAndCancellation": "Checks usage refusals before desktop access, failed result writes, hosted authorization, launch failures and direct process timeout. Hosted modes reject absent or altered runner markers.",
        "testCoverage": "The PowerShell case `the self-test passes: children, the session's lease, the confirmation and the warning` at Tools/tests/Test-InteractiveLease.ps1:49-62 requires exit 0, a PASS summary, and zero FAIL lines. Since native SelfTest::Run invokes DescendantProcesses and propagates any failed check to its exit status, the existing behavioral test covers exact descendant completion; no additional source-mirroring assertion is needed. This is source review only; no test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; inherited native descendant-completion coverage confirmed at InteractiveLease.Tests.Runner.cpp:532-550,686-746. No tests executed.",
      "testIds": [
        "the self-test passes: children, the session's lease, the confirmation and the warning",
        "--check reports failed result writes without taking the desktop",
        "the lease ends only the processes it started"
      ]
    },
    {
      "path": "Tools/tests/Test-InteractiveMode.ps1",
      "sha256": "437E6E4F055B2AA7154AA8226A74735A9C3B28D5CA13CED4950761A5D4ACD462",
      "reviewNotes": {
        "callers": "Tests test.ps1 and InteractiveRun.psm1 policy, lease protocol, result reader and hosted ARM preparation using fake commands and synthetic receipts.",
        "ownershipAndLifetime": "Fixture roots and fake scripts are removed via TestSupport; environment markers are saved/restored in finally. No real desktop takeover is initiated by this script.",
        "failureAndCancellation": "Refusal paths assert no build/log/output writes; structured lease results preserve child identity, restoration states and cancellation/timeout; stale results are removed before launch and raw old logs are retained separately.",
        "testCoverage": "Full 946-line review covers suite routing, authorization, timeouts, result protocol, exact child names, restoration presentation, native console behavior, prior-log preservation, hosted wrapper and ARM guard. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "a run without -Suites takes Menu and NativeTextInput, and the fixtures only when they are named",
        "approved automatic starts retain their mode, bounded countdown and distinct result",
        "a refused lease preserves raw prior output without qualifying it"
      ]
    },
    {
      "path": "Tools/tests/Test-PairedRun.ps1",
      "sha256": "BC4D58B3FD5CD71D1F91F72E2CCC0A282F6DBF466A6B5AE96A651C924D94A889",
      "reviewNotes": {
        "callers": "Tests PairedRun and PerformanceComparison module behavior plus performance-paired.ps1 via parsed/extracted functions; current version includes the newer randomized independent-block and A/A cases.",
        "ownershipAndLifetime": "Fixture Git trees are created in scoped roots. Overlay records retain hashes/backups and restore only owned edits; rollback refuses to overwrite later mutations and preserves backups. A/A maps roles to one physical build root while ordinary comparisons keep two.",
        "failureAndCancellation": "Changed/missing overlay bytes, mutated backup, incompatible identities, same tree/commit, incomplete restore and changed benchmark project all fail before measurement; partial timestamp staging is rolled back.",
        "testCoverage": "Named cases cover schedule reproducibility and balance, A/A same artifact, legacy include aliasing, overlay attestation/rollback, source fingerprints (non-ASCII), exact measured base judge bytes and build preflight. No tests run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "independent orders retain reproducible prefixes and balance runs within every block",
        "A/A assigns both randomized labels to one root and one build, while ordinary pairs keep two roots",
        "overlay rollback preserves later edits to created and replaced paths while restoring unaffected files",
        "paired preflight rejects absent or changed compiler, dependency, fixture and environment identities"
      ]
    },
    {
      "path": "Tools/tests/Test-PerformanceComparison.ps1",
      "sha256": "9E86B6336D70B3B8889F5E0FE4ED19E96FC4A6D6FFEB80BE831FDA42F75A43C1",
      "reviewNotes": {
        "callers": "Tests receipt validators, exact/rank judges, pair-set comparator and Compare-Performance.ps1 using generated receipts plus stored Measurements fixtures.",
        "ownershipAndLifetime": "All writable command output is fixture-scoped; stored measurement/comparison artifacts are read-only. Deterministic pseudo-random sequences seed statistical cases.",
        "failureAndCancellation": "Rejects malformed timing diagnostics, nonfinite metrics, hidden work, identity mismatch, missing rounds, mixed binary/fixture provenance and exact-budget increases. JSON status and exit codes are asserted.",
        "testCoverage": "Full 635-line review covers legacy comparison parity, exact sign-flip/Holm inference, rank ties/large counts/reference example, medians/spread/bands, exact budgets, and every stored paired/unpaired artifact. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "the migrated block judge uses identity, independent paired effects and separate exact budgets",
        "the exact rank test counts ties as an enumeration of every rank assignment does",
        "every stored paired comparison is reproduced exactly",
        "every stored unpaired receipt still validates"
      ]
    },
    {
      "path": "Tools/tests/Test-SuiteFailure.ps1",
      "sha256": "5C4159C466D614BCC83DD5A8C411275FB14FEC92FC1644F340CDA0A906859201",
      "reviewNotes": {
        "callers": "Tests SuiteFailure.psm1 parsing and test.ps1 reporting with fixture logs.",
        "ownershipAndLifetime": "Fixture log files are temporary; no child processes or global state are retained.",
        "failureAndCancellation": "Separates watchdog timeout from ordinary failure, handles missing logs and exit codes, and associates skip lines with test/suite context.",
        "testCoverage": "Named cases include timeout line outside tail, non-timeout failure classification, missing log, multiple skips, and test.ps1 reporting integration. No tests run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "a run the watchdog ended is reported with its exit code and the hung test",
        "each capability skip is named after the test it belongs to"
      ]
    },
    {
      "path": "Tools/tests/Test-TestFilter.ps1",
      "sha256": "2A1608B9382A10BE8CA4C7136AF8EF80BE93C597191C428CE8BF1A57CBCF0834",
      "reviewNotes": {
        "callers": "Runs the built DxUi.ControlTests.exe from repository root using named suites and exact --test arguments.",
        "ownershipAndLifetime": "Uses only the built test executable and captures streams; no fixture files are written. Assumes binary already exists and fails if absent.",
        "failureAndCancellation": "Unknown or malformed names exit usage code before test start; fixture suites reject named filters; invocation restores current location in finally.",
        "testCoverage": "Named cases check exact/case-sensitive filtering, partial unknown lists, empty/trailing/delimited names, no direct test calls in suite runners, and all registered Grid tests starting in order. No executable was run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "the filter runs only the named tests, in suite order",
        "an unknown name fails the run instead of passing with nothing run",
        "without the filter a suite runs every test its runner registers"
      ]
    },
    {
      "path": "Tools/tests/Test-TestPort.ps1",
      "sha256": "ABDD1E7D3990A1BA148AE83357A8D5D40AD863E4E4DE3090DA933A28E8477D23",
      "reviewNotes": {
        "callers": "Exercises Test-DxUiTestPort in Validation.psm1 with synthetic manifest, source and disposition policy files.",
        "ownershipAndLifetime": "Each fixture root is owned by Invoke-FixtureCase; all source/policy content is synthetic.",
        "failureAndCancellation": "Missing/deleted current cases, dropped dispositions, unexplained exclusion, missing replacement and duplicate policy all fail closed; explicit source rename and contextual currentFile retain historical origin.",
        "testCoverage": "Ten cases cover retained, deleted, discarded, renamed, excluded and runtime-replaced rows. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "a retained case is accounted for",
        "silently deleting a case fails",
        "a current runtime replacement passes",
        "a duplicate policy fails"
      ]
    },
    {
      "path": "Tools/tests/Test-TestWatchdog.ps1",
      "sha256": "AF65119F893743C4140A4D6A0D8442769F3FFC1F8C30EEA6E1C635C40FCE4DFD",
      "reviewNotes": {
        "callers": "Starts bounded DxUi.ControlTests.exe runs and consumes SuiteFailure reports; test.ps1 invokes this after build.",
        "ownershipAndLifetime": "Owns/disposes its child process; redirects/drains both streams and uses Kill(true) only for the bounded process it started. CPU probe child is always ended in finally.",
        "failureAndCancellation": "Tests watchdog timeout, disabled deadline, progress extension, idle waiting, malformed args and usage refusal under outer bounds so a broken inner watchdog cannot hang the script.",
        "testCoverage": "Named cases cover hung named test/fixture, progress reset, no-watchdog behavior, CPU idleness, 300-second default, durations, malformed values, runtime failure report, test.ps1 timeout summary and Menu-driver cleanup guard. No run performed."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "a test that never returns ends the run at its deadline, named",
        "an armed watchdog waits instead of polling",
        "test.ps1 surfaces the timeout: the exit code and the TIMEOUT line among the printed lines"
      ]
    },
    {
      "path": "Tools/tests/Test-UnicodeGraphemeData.ps1",
      "sha256": "D815C57F1DF99048EAE228338B943E018AABA8E9BAC5EADFF5243F59DAA51EC3",
      "reviewNotes": {
        "callers": "Runs Generate-UnicodeGraphemeData.ps1 in generate/check modes against pinned Unicode inputs and generated headers.",
        "ownershipAndLifetime": "Uses repository-tracked input/output paths; check mode snapshots timestamps and asserts read-only behavior. Missing-input cases use fixture directories.",
        "failureAndCancellation": "Rejects missing/stale generated outputs and altered input/license hashes before rewriting outputs; exact bytes are compared.",
        "testCoverage": "Checks deterministic regeneration of both headers, check-mode timestamp preservation and fail-closed missing/stale/hash cases. No generator was run during this review."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Test-UnicodeGraphemeData"
      ]
    },
    {
      "path": "Tools/tests/TestSupport.psm1",
      "sha256": "7B85C5F706CFD12E2381D831FF10A2D28712C89E2BC670F2019815E3306916EF",
      "reviewNotes": {
        "callers": "Imported by tooling test scripts for assertions, case aggregation, isolated fixtures, JSON and SHA helpers.",
        "ownershipAndLifetime": "Fixture parent is repository .build/tooling-fixtures; Invoke-FixtureCase creates a fresh child and Remove-FixtureRoot cleans it in finally with a containment check.",
        "failureAndCancellation": "Invoke-TestCase accumulates failures instead of stopping the suite; fixture teardown always runs. Cleanup resolves paths and refuses deletion outside the owned fixture parent.",
        "testCoverage": "Reviewed all 99 lines and exported helper surface; no dedicated case here, coverage comes from all calling scripts. No test run."
      },
      "reviewer": "mermaid_executor",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "testIds": [
        "Invoke-TestCase",
        "Invoke-FixtureCase",
        "Remove-FixtureRoot"
      ]
    }
  ]
}
```

