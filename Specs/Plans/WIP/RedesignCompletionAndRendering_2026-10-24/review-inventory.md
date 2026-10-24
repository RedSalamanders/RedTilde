# Code review inventory - 2026-10-24

Machine-readable review data for the parent plan. All rows start pending; source hashes do not establish review completion. Keep this fenced JSON under the documentation tree so planning records are validated as documentation. The parent plan defines the required review evidence and completion check.

```json
{
  "schemaVersion": 1,
  "date": "2026-10-24",
  "status": "SOURCE_REVIEW_COMPLETE_RUNTIME_AND_PERFORMANCE_QUALIFICATION_OPEN",
  "description": "Execution inventory reconciled with b06 file renames and new first-party inputs; reviewed rows require four substantive review notes and tests or explicit gaps. Pending hashes remain planning/admission identities, not completed reviews. RedTilde report merged; DxUi review in progress.",
  "plannedAt": {
    "RedTilde": "c7456854c8f964bb9ea067295e6435f99d426515",
    "DxUi": "ceffe76a429d15039ceff1f7dfae7e1d7126323e"
  },
  "scope": "Tracked first-party C/C++ headers and sources, scripts, build/workflow inputs, JSON data/manifests and native resources; plus the exact DxUi consumer lock. Generated code stays listed for generator/provenance review. Vendored source/corpora, skills, documentation, assets and ignored products are not a line-by-line audit target; their ownership/provenance and boundary behavior still require review.",
  "entryCount": 482,
  "files": [
    {
      "repository": "RedTilde",
      "path": ".github/workflows/ci.yml",
      "sha256": "b9d237b55efd7e0f29a685e9fa7b8e82daabf0795eab90bae52fa79122f50a56",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "CI covers six exact native profiles and tooling once with no duplicate main-push run",
        "retained CI proof requires all six actual successful native executions"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "PR-to-main and manual workflow invoke native-scope, tooling and six profile jobs; Get-CiNativeScope provides descriptor/proof; upload steps retain input and profile logs.",
        "ownershipAndLifetime": "Read-only GitHub permissions, actions pinned by SHA, same-ref concurrency cancels superseded runs; artifacts retained 14 days.",
        "failureAndCancellation": "Input mismatch fails native validation; retained proof is emitted only after all six native executions succeed. Cancellation is not success evidence.",
        "testCoverage": "Scoped tests validate workflow digest/profile/coverage/proof contracts; hosted CI was not run."
      },
      "plannedSha256": "b9d237b55efd7e0f29a685e9fa7b8e82daabf0795eab90bae52fa79122f50a56"
    },
    {
      "repository": "RedTilde",
      "path": "build.ps1",
      "sha256": "ab218b05f15a512ee599ab42f40c55f3e8fa3783a57a1d7b1382b2a731c48a28",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "public runner expands empty and explicit tooling argument arrays exactly",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Used by package.ps1, test.ps1, Test-Changes.ps1 and test-interactive.ps1; restores pinned DxUi unless SkipRestore.",
        "ownershipAndLifetime": "Profile outputs/logs are under repo .build; refuses if exact profile exe is running; source identity is checked before successful receipt.",
        "failureAndCancellation": "MSBuild/restore/source mutation/dependency identity failures stop receipt publication.",
        "testCoverage": "Mock public-runner fixtures cover argument expansion and affected baseline decisions; no actual build was run, leaving MSBuild execution unqualified."
      },
      "plannedSha256": "ab218b05f15a512ee599ab42f40c55f3e8fa3783a57a1d7b1382b2a731c48a28"
    },
    {
      "repository": "RedTilde",
      "path": "Build/Harness.psm1",
      "sha256": "e53552e8cf7002f52a09cdca5d593757365919c646c76c5031804cbb267f454e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "native identity changes with source/binary/runtime/profile/environment",
        "missing required runtime inputs and JSON-only profiles fail before identity creation",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "build.ps1 records source identity; test.ps1 and paired performance runner validate artifacts and launch owned processes.",
        "ownershipAndLifetime": "Owns child Process, redirected pipes, timeout and descendant cleanup; artifact identity hashes exe/runtime closure.",
        "failureAndCancellation": "Timeout kills only owned process tree and preserves logs; stale source, receipt, binary or runtime closure refuses reuse.",
        "testCoverage": "Scoped fixture tests cover source/binary/runtime/profile/environment identities and missing runtime inputs; no product child launched."
      },
      "plannedSha256": "e53552e8cf7002f52a09cdca5d593757365919c646c76c5031804cbb267f454e"
    },
    {
      "repository": "RedTilde",
      "path": "Build/Performance.psm1",
      "sha256": "a5850d16712cc8d8c7ae494eb3366be791805fc933e3bd9dc15cc4aab49396cb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1: math, schedule and compiler-profile fixtures",
        "Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest: independent comparator reference"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "paired and observed performance entrypoints consume validation/math; evidence auditor has an independent comparator.",
        "ownershipAndLifetime": "Pure calculations over deserialized samples; compiler-profile reader binds project TU list to raw compiler tlog.",
        "failureAndCancellation": "Rejects malformed schemas, invalid/incomplete samples, inconsistent phase accounting and unsupported/mixed compiler options; comparisons remain review-required.",
        "testCoverage": "Tooling validators cover medians, schedules, exact sign-flip/Holm results, invalid metrics and compiler-profile boundaries; no native study ran."
      },
      "plannedSha256": "a5850d16712cc8d8c7ae494eb3366be791805fc933e3bd9dc15cc4aab49396cb"
    },
    {
      "repository": "RedTilde",
      "path": "Build/PerformanceObservation.psm1",
      "sha256": "30a4715b2a29da67c00af77a3ae43c9af5ddaea59b1a10b176b2acb2e8b828ea",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1: observation and coverage contracts"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "performance-observed.ps1 launches activity observer and analyzes process/noise/coverage.",
        "ownershipAndLifetime": "Observer runs in an owned background job and writes JSONL under fresh repo .build output; process deltas keyed by PID/start time.",
        "failureAndCancellation": "Controller records observer status, stops its own job, and refuses incomplete time coverage; unavailable process metrics are never zeroed.",
        "testCoverage": "Observation tests cover CPU thresholds, unavailable processes, identity deltas, schema and interval coverage; orchestration not run."
      },
      "plannedSha256": "30a4715b2a29da67c00af77a3ae43c9af5ddaea59b1a10b176b2acb2e8b828ea"
    },
    {
      "repository": "RedTilde",
      "path": "Build/RedTilde.DxUi.props",
      "sha256": "5955c0931e04fbe59d09692244ff319e5f0cfeb586ce8dfed3a7156a4caf99cf",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Six build configurations and native inventory"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "RedTilde vcxproj imports resolved dependency properties and canonical DxUi consumer props.",
        "ownershipAndLifetime": "Imports only generated resolved properties; pre-build target requires restored DxUi root.",
        "failureAndCancellation": "Missing restore root fails before compile with direct instruction.",
        "testCoverage": "Six project configs are validated; no MSBuild property evaluation/build ran."
      },
      "plannedSha256": "5955c0931e04fbe59d09692244ff319e5f0cfeb586ce8dfed3a7156a4caf99cf"
    },
    {
      "repository": "RedTilde",
      "path": "Build/ScopedTesting.psm1",
      "sha256": "50d0f479281f4ead2a72beb6152f92ee8a34730dbe37973374e309d2ad0bc9de",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "active native naming and inventory are contextual and complete",
        "committed/staged/working/deleted/rename sides/Unicode/untracked are independently discovered",
        "native identity changes with source/binary/runtime/profile/environment",
        "iteration state is source/index/profile/environment bound and rejects corruption",
        "live project/include consumers follow added dependencies and terminate include cycles",
        "only a clean committed candidate can delegate matching enabled CI, always as pending",
        "retained CI proof requires all six actual successful native executions",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Test-Changes uses scope/source/index/environment/artifact/receipt/CI/iteration functions; test.ps1 and Get-CiNativeScope reuse manifest/runtime/CI helpers.",
        "ownershipAndLifetime": "Git child args are structured; identities hash exact source/index/environment/profile/artifacts; receipts stage then rename; project include expansion cycle-guards.",
        "failureAndCancellation": "Invalid selectors/paths, missing base/history and stale evidence fail or conservatively widen; GitHub lookup uncertainty keeps work local; mutation blocks iteration advancement.",
        "testCoverage": "Tooling suite exercises path/scope mapping, Unicode/index changes, identity, receipts, CI delegation races, include closure and mutated runs. No live API calls."
      },
      "plannedSha256": "50d0f479281f4ead2a72beb6152f92ee8a34730dbe37973374e309d2ad0bc9de"
    },
    {
      "repository": "RedTilde",
      "path": "Build/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "38 evidence preservation checks in Tests/Tooling.Tests.RunEvidence.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "test.ps1 and test-interactive.ps1 create, complete and publish owned test-run evidence.",
        "ownershipAndLifetime": "Pins local non-reparse path identities, unique run lock and owner PID/start time; writes beneath repo .build/evidence; preserves old compatibility receipts under payload.",
        "failureAndCancellation": "Refuses root/UNC/relative/reparse outputs, open/writable payloads, identity swaps, wrong owner, active artifact consumers and invalid names; closes locks in completion finally.",
        "testCoverage": "Run-evidence test validates concurrent isolation, byte/ACL/ADS preservation, path/link/owner refusal, long paths and publication. No product artifacts touched."
      },
      "plannedSha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63"
    },
    {
      "repository": "RedTilde",
      "path": "Build/Toolchain.psm1",
      "sha256": "4638ad95d1f48106ed09a7654196462b2aa97c144fb5b44c4946342cf41d55f4",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "tooling identity includes the selected formatter bytes",
        "format.ps1 -Check"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "build/test-interactive resolve VS 2026 MSBuild; format and identity discovery resolve clang-format.",
        "ownershipAndLifetime": "Resolves installation paths and checks executable path within selected output profile; no persistent child state.",
        "failureAndCancellation": "Missing compiler tools fail explicitly; in-use app blocks overwrite.",
        "testCoverage": "Formatter bytes affect scoped identity; current format check passed. No full MSBuild discovery matrix."
      },
      "plannedSha256": "4638ad95d1f48106ed09a7654196462b2aa97c144fb5b44c4946342cf41d55f4"
    },
    {
      "repository": "RedTilde",
      "path": "capabilities.json",
      "sha256": "7c479509e93ce756e5666eef77aa191376e5d511ce2ccb939afc9ef282194a98",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Exact dependency and capability status"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "validate.ps1 and package metadata consumer.",
        "ownershipAndLifetime": "Static versioned implemented/pending capability declaration.",
        "failureAndCancellation": "Malformed JSON or dropped explicit pending gates fail validator.",
        "testCoverage": "Experimental/pending contract validation passed; not a runtime behavior test."
      },
      "plannedSha256": "7c479509e93ce756e5666eef77aa191376e5d511ce2ccb939afc9ef282194a98"
    },
    {
      "repository": "RedTilde",
      "path": "Common/CrashHandler.cpp",
      "sha256": "8f585fb51670551318ebb196880bfb627d2c748720425458f037073671fbe98b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestNoMarkerMakesNoOffer",
        "TestMarkerWithActivePluginOffersAndApplies",
        "TestBuildDumpPathShape",
        "TestMarkerRoundTrip"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\Main.cpp startup registers CrashHandler; shutdown unregisters it. Cross-repository CrashHandlingTests test seams are in D:\\RedSalamander\\Tests\\CrashHandlingTests\\CrashHandling.Tests.Runner.cpp.",
        "ownershipAndLifetime": "Registration is process-lifetime state; teardown path inspected for paired unregister.",
        "failureAndCancellation": "Failure paths report dump/handler errors; no recovery guarantee claimed.",
        "testCoverage": "No direct application-test assertion found in reviewed scope; crash-specific coverage remains gap."
      },
      "plannedSha256": "8f585fb51670551318ebb196880bfb627d2c748720425458f037073671fbe98b"
    },
    {
      "repository": "RedTilde",
      "path": "Common/CrashHandler.h",
      "sha256": "ef82c354e4432ca8bfaae01fe5f4a1917f95b03c2f9c9b2eb653ae42c17738ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestNoMarkerMakesNoOffer",
        "TestMarkerWithActivePluginOffersAndApplies",
        "TestBuildDumpPathShape",
        "TestMarkerRoundTrip"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\Main.cpp startup calls CrashHandler registration; cross-repository test seams are consumed by CrashHandling.Tests.Runner.cpp.",
        "ownershipAndLifetime": "Interface exposes registration lifetime; implementation is in CrashHandler.cpp.",
        "failureAndCancellation": "Caller must handle initialization result; no cancellation semantics.",
        "testCoverage": "No direct test coverage identified in assigned app tests."
      },
      "plannedSha256": "ef82c354e4432ca8bfaae01fe5f4a1917f95b03c2f9c9b2eb653ae42c17738ad"
    },
    {
      "repository": "RedTilde",
      "path": "Common/DocumentInfo.cpp",
      "sha256": "77def0df6de5b3d22c31535666b2ca51dd538a3b7c886be311f76bc31e1b0a00",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ],
      "findingIds": [],
      "disposition": "simplification-candidate",
      "reviewNotes": {
        "callers": "DocumentPositionAt is consumed by source inspector/status mapping. Repository source search found EquationAt only at its declaration/definition and former helper-only test; current app inspector uses syntax-backed extraction, so it was dead.",
        "ownershipAndLifetime": "DocumentInfo and positions are value-oriented; EquationAt implementation removed with no retained state or ownership.",
        "failureAndCancellation": "DocumentPositionAt remains bounded/noexcept; removed helper had no cancellation or external resources.",
        "testCoverage": "Parser boundary and syntax-backed inspector integration now covered by Math.NativeSubsetAndRejection and Editor.InDocumentMathLocalSourceAndBroaderMermaid. C++ tests not built/run."
      },
      "plannedSha256": "9e7eeece032dadee3f79a4044ff148dad4d0434f9e6e60f3422af766bc1949ff"
    },
    {
      "repository": "RedTilde",
      "path": "Common/DocumentInfo.h",
      "sha256": "40244f3098fb742e4db783ef40e48652aadbb6c8adec0c39cfebffc0a258ea2a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ],
      "findingIds": [],
      "disposition": "simplification-candidate",
      "reviewNotes": {
        "callers": "DocumentPositionAt declaration serves source inspector/status; EquationAt had no production caller and its declaration was removed.",
        "ownershipAndLifetime": "Value declarations; borrowed views only, no retained memory.",
        "failureAndCancellation": "DocumentPositionAt states noexcept/clamping contract; no async cancellation.",
        "testCoverage": "Core parser boundary and syntax-backed inspector cover current behavior; EquationAt was test-only and removed. Native tests not compiled/run."
      },
      "plannedSha256": "0414aea3bdbc67f49b97109178dcb6e42a6d9b51460a62766b4bfc59bc005b8b"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Files.cpp",
      "sha256": "08e78096ea35aed558c42d4318af0f829f661ffef29ae037c0e8a0d40ad3a76e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Shared file helpers are used by FilesService and recovery/settings paths.",
        "ownershipAndLifetime": "Filesystem handles and buffers use scoped ownership; no persistent worker ownership here.",
        "failureAndCancellation": "Error-code/HRESULT paths propagate failure; callers own cancellation policy.",
        "testCoverage": "Files and recovery app tests cover relevant usage, not all OS error permutations."
      },
      "plannedSha256": "08e78096ea35aed558c42d4318af0f829f661ffef29ae037c0e8a0d40ad3a76e"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Files.h",
      "sha256": "2d3f9bf94e0faaeb1c0b7ed47b369dfffa12e6d6ffb434a5b1517662c6c49603",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares shared file helpers used by application services.",
        "ownershipAndLifetime": "Contracts are value/borrowed API; implementation owns transient OS resources.",
        "failureAndCancellation": "Failure contract is declared for callers; cancellation is caller-specific.",
        "testCoverage": "Coverage is in Files/Recovery tests; declaration has no independent test."
      },
      "plannedSha256": "2d3f9bf94e0faaeb1c0b7ed47b369dfffa12e6d6ffb434a5b1517662c6c49603"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Recovery.cpp",
      "sha256": "3f7e3b5ecd9619cb30d99e39307e2063782da8328a435b9c1c2be7426c19cc63",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Recovery service call sites originate in Application lifecycle/save paths.",
        "ownershipAndLifetime": "Lease and worker state bound service lifetime; shutdown requests cancellation and joins via completion protocol.",
        "failureAndCancellation": "Versioned/checksummed snapshots, quarantine and retry cover corruption; queue-full post failure remains investigation only because dispatch wake/completion checks exist.",
        "testCoverage": "Recovery tests cover lock replacement, retry, restart/restore/discard and dirty Save As; no confirmed shutdown hang."
      },
      "plannedSha256": "3f7e3b5ecd9619cb30d99e39307e2063782da8328a435b9c1c2be7426c19cc63"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Recovery.h",
      "sha256": "a2791dbb672905631bb2b1cc54b9913017fac1a11410ad8b385ce6ae0e7cedd9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares recovery worker and request/result contracts called by Application.",
        "ownershipAndLifetime": "Service owns worker/lease; request/result data crosses asynchronously.",
        "failureAndCancellation": "Cancellation/close state is explicit; post failure must be interpreted with app completion polling.",
        "testCoverage": "Application tests cover recovery lifecycle; stress around every post-failure interleaving remains gap."
      },
      "plannedSha256": "a2791dbb672905631bb2b1cc54b9913017fac1a11410ad8b385ce6ae0e7cedd9"
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImage.cpp",
      "sha256": "a1c0ad49201146c61e4625dbc12a890b13dd17a915371cd08a1b9f9986ec914e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "ImageLoader uses bounded remote-image request implementation.",
        "ownershipAndLifetime": "WinHTTP/session/request resources are scoped; no ambient auth/cookie/referrer and redirects disabled.",
        "failureAndCancellation": "Deadline, size bound, cancellation, HTTP errors handled; no unbounded response path observed.",
        "testCoverage": "Remote image tests use loopback and cover auth/redirect/limit/deadline/cancel/revoke/shutdown."
      },
      "plannedSha256": "a1c0ad49201146c61e4625dbc12a890b13dd17a915371cd08a1b9f9986ec914e"
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImage.h",
      "sha256": "ea55a04cb361bab5dae7da0c190f700543de2e98ab2bb217f6a64cc58ec0d0e5",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares remote image request contract consumed by ImageLoader.",
        "ownershipAndLifetime": "Caller supplies request context; result ownership is explicit.",
        "failureAndCancellation": "Cancellation and failure status are part of request/result path.",
        "testCoverage": "Loopback remote-image tests cover major contract; no external network test expected."
      },
      "plannedSha256": "ea55a04cb361bab5dae7da0c190f700543de2e98ab2bb217f6a64cc58ec0d0e5"
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImageImports.h",
      "sha256": "9544bf865901ef66981a755e66c7efe36e152aff2bfd26b3537ad1c2a464fcc4",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Import declarations support remote-image implementation only.",
        "ownershipAndLifetime": "No owning state; imported function surface.",
        "failureAndCancellation": "Import failure is surfaced by implementation; no separate recovery behavior.",
        "testCoverage": "No direct test; remote image tests validate externally visible behavior."
      },
      "plannedSha256": "9544bf865901ef66981a755e66c7efe36e152aff2bfd26b3537ad1c2a464fcc4"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Settings.cpp",
      "sha256": "1f4134148d810cee761d797f722e0826565eb3ef6b416139ab3c85a654ae61ae",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "SettingsWriter called by Application persistence/close flow.",
        "ownershipAndLifetime": "Worker retains latest pending settings snapshot; close signals async completion.",
        "failureAndCancellation": "Fieldwise parser fallback; failed post is not by itself a demonstrated hang.",
        "testCoverage": "Settings/app tests cover persistence and lifecycle; post-failure interleaving remains investigation."
      },
      "plannedSha256": "1f4134148d810cee761d797f722e0826565eb3ef6b416139ab3c85a654ae61ae"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Settings.h",
      "sha256": "73310426bff33169ca187be4e4c5cb24b1df9c2826b8f35e3ac6e2d179c70be3",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares settings snapshot/writer API consumed by Application.",
        "ownershipAndLifetime": "Snapshot ownership copied to writer; worker lifetime managed by writer.",
        "failureAndCancellation": "Failure surfaced via status/completion; caller must observe close completion.",
        "testCoverage": "Settings and application tests cover relevant persistence; no stand-alone exhaustive writer stress noted."
      },
      "plannedSha256": "73310426bff33169ca187be4e4c5cb24b1df9c2826b8f35e3ac6e2d179c70be3"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Storage.cpp",
      "sha256": "db4d62d8503aad67cfb73712dec9284f7ba97b986d0bbc1dc445c98871cef341",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [
        "STORAGE-CONCURRENT-WRITER-COVERAGE"
      ],
      "disposition": "coverage-gap",
      "reviewNotes": {
        "callers": "Atomic byte writes support settings/recovery persistence.",
        "ownershipAndLifetime": "Transient file handles use scoped ownership; replacement is atomic at file operation boundary.",
        "failureAndCancellation": "Optimistic stamp checked before ReplaceFile/MoveFile; not CAS against simultaneous noncooperating writers.",
        "testCoverage": "Tests cover change-before-save, not true concurrent-writer race (specific coverage gap)."
      },
      "plannedSha256": "db4d62d8503aad67cfb73712dec9284f7ba97b986d0bbc1dc445c98871cef341"
    },
    {
      "repository": "RedTilde",
      "path": "Common/Storage.h",
      "sha256": "bbc228765adf14031b90f54f6f2d66f34f915a2264e7c05094e5aa90e7297e04",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [
        "STORAGE-CONCURRENT-WRITER-COVERAGE"
      ],
      "disposition": "coverage-gap",
      "reviewNotes": {
        "callers": "Declares storage read/write and atomic replacement API used by settings/recovery.",
        "ownershipAndLifetime": "Callers own data buffers; storage owns transient handles.",
        "failureAndCancellation": "Result reports conflict/failure; simultaneous external writers are outside guaranteed CAS contract.",
        "testCoverage": "Storage behavior is exercised by settings/recovery tests; concurrent external race is not covered."
      },
      "plannedSha256": "bbc228765adf14031b90f54f6f2d66f34f915a2264e7c05094e5aa90e7297e04"
    },
    {
      "repository": "RedTilde",
      "path": "Dependencies/DxUi.lock.json",
      "sha256": "781d418d147fbb0abb7dd88c76b735b8d5f5b8d634f6b6d2b44523834c3334b1",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Exact dependency and capability status"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "restore-dxui validates repository, exact commit, API revision and target; tests/package record pin.",
        "ownershipAndLifetime": "Pin resolves source/dependency output under repo .build.",
        "failureAndCancellation": "Malformed pin refuses; clone/checkout/consumer failures stop before resolved props.",
        "testCoverage": "JSON pin/capability validator passed; no network restore executed."
      },
      "plannedSha256": "74f687de9d4c5ab7a7601035da3ae61a13194e81f6fd4dc226362f98dc995d70",
      "leadDeltaReview": "Reviewed immutable SDK adoption at API revision 4: 94bf9801f51a544422ecb8aa6ae86d2e33aadcb2, with no added package dependency. Clean canonical source pin and x64 restore validated; final app profiles remain pending."
    },
    {
      "repository": "RedTilde",
      "path": "Directory.Build.props",
      "sha256": "fc4a983cc286eb8b459766f22f2638aacd1486cce53d417aa893a89331c0cc07",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Six build configurations and native inventory"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "MSBuild applies output paths, language standard, warning policy, Unicode, SDL/conformance and ASan config to native projects.",
        "ownershipAndLifetime": "All intermediate/output dirs are profile/project-local under .build; no shared external output declaration.",
        "failureAndCancellation": "Warnings-as-errors and compiler gates fail build; no fallback to different language mode.",
        "testCoverage": "Project config presence validated, but effective property evaluation/build not run."
      },
      "plannedSha256": "fc4a983cc286eb8b459766f22f2638aacd1486cce53d417aa893a89331c0cc07"
    },
    {
      "repository": "RedTilde",
      "path": "format.ps1",
      "sha256": "3eda7cff89ade8359987235247d30b69a64c0b438991f98b10aa0178378b3724",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "format.ps1 -Check"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "CI tooling job and developer check/reformat entrypoint; formatter selected by Toolchain module.",
        "ownershipAndLifetime": "Enumerates C++ files under Common, RedTilde and Tests; Check is read-only, default writes formatter output.",
        "failureAndCancellation": "Formatter discovery/error fails closed; no external output.",
        "testCoverage": "Read-only formatting check passed after changed C++ was formatted."
      },
      "plannedSha256": "3eda7cff89ade8359987235247d30b69a64c0b438991f98b10aa0178378b3724"
    },
    {
      "repository": "RedTilde",
      "path": "package.ps1",
      "sha256": "725505f3d323ea244eb73356277e0b566099196bb9b0324692502ad2159cf281",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Portable packaging entrypoint; optionally builds Release then stages app/docs/assets/dependencies/licences.",
        "ownershipAndLifetime": "Stage, zip and receipt remain beneath .build/package; stage is timestamped; runtime redistributable explicitly not bundled.",
        "failureAndCancellation": "Refuses stale source/binary receipt; copy/archive failures stop; does not verify clean extraction itself.",
        "testCoverage": "No package execution/smoke test in current tooling suite or this run; archive extraction/licensing payload remains gap."
      },
      "plannedSha256": "725505f3d323ea244eb73356277e0b566099196bb9b0324692502ad2159cf281"
    },
    {
      "repository": "RedTilde",
      "path": "performance-observed.ps1",
      "sha256": "2fc4a67b9a07bf78d47d11c16e1ba05940c717f0133152ed1e0857368fcd5dcd",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1",
        "Tests/Scope.Tests.Performance.Tooling.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Developer diagnostic wrapper starts observer around paired A/A and A/B studies.",
        "ownershipAndLifetime": "Requires fresh output below repo .build; retains logs, activity, observer status, summaries and native receipt noise.",
        "failureAndCancellation": "Observer/child failure or incomplete coverage stops acceptance but retains attempt evidence; only owned job is stopped.",
        "testCoverage": "Data validators cover observations and paired evidence shapes, not live orchestration or real study."
      },
      "plannedSha256": "2fc4a67b9a07bf78d47d11c16e1ba05940c717f0133152ed1e0857368fcd5dcd"
    },
    {
      "repository": "RedTilde",
      "path": "performance-paired.ps1",
      "sha256": "ec17f60be95d513c6db4314ebe78201fc680da795563c835eb1241517640777b",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1",
        "Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "performance-observed wrapper or explicit user invokes freeze baseline/measure study.",
        "ownershipAndLifetime": "Validates Release receipt, exact fixture/compiler/environment and executable per slot; each child launched by Harness into fresh .build output.",
        "failureAndCancellation": "Refuses stale identity, inappropriate same-binary pairing, renderer/schema drift or failed child; failed attempts retained and never accepted.",
        "testCoverage": "Tooling/evidence fixtures cover inference and provenance rules; no baseline freeze or native paired measurements run."
      },
      "plannedSha256": "ec17f60be95d513c6db4314ebe78201fc680da795563c835eb1241517640777b"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde.sln",
      "sha256": "6e39173e22e39a02b705ddffebb3366c7c045131c57dfcfab89ebba0dd4d0da2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Optional VS solution entrypoint for the single RedTilde project and x64/ARM64 Debug/Release/ASan configs.",
        "ownershipAndLifetime": "Explicit solution-to-project mapping for six profile pairs.",
        "failureAndCancellation": "No custom hooks; build failures owned by MSBuild.",
        "testCoverage": "No solution parse/build was run; validator checks project config existence, not sln mapping."
      },
      "plannedSha256": "6e39173e22e39a02b705ddffebb3366c7c045131c57dfcfab89ebba0dd4d0da2"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.CaptureScenarios.cpp",
      "sha256": "acff958a17b2288b203cb9a0fa97b2dfeb9149d50ee1a3c0d36255a326e7f929",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Harness capture scenario calls lifecycle and UI transitions under deterministic hidden setup.",
        "ownershipAndLifetime": "Scenario-owned resources scoped to capture run; no desktop activation path intended.",
        "failureAndCancellation": "Scenario failure aborts capture and propagates diagnostics; cancellation bounded by harness.",
        "testCoverage": "Inspected capture flows; not run. Add/retain source identity and scenario provenance with captures."
      },
      "plannedSha256": "acff958a17b2288b203cb9a0fa97b2dfeb9149d50ee1a3c0d36255a326e7f929"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.Chrome.cpp",
      "sha256": "120f54db0480f599508e3919028b2c76645987c007c0d0694e3a0660caf34770",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Application::BuildChrome creates BrandMark; DxUi visual preparation invokes its public hook.",
        "ownershipAndLifetime": "Retains WIC source over device replacement; D2D bitmap stamped with host resource generation and cleared on preparation cancellation; Paint draws only current-generation bitmap.",
        "failureAndCancellation": "Creation failure leaves no drawable bitmap; generation guard suppresses stale resource. No device COM query/create/retry in Paint.",
        "testCoverage": "New registered hidden case compares raster before/after device loss and repeated warm render. Not built/run; this is a rendering-contract correction, not evidence of 4 MiB cause/reduction."
      },
      "plannedSha256": "fcbc97ceb109675cada7090c1c0fb556cb2c9aff62d1fd33f1cc8243b54e7f6e"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.cpp",
      "sha256": "15e84820a8ab6a9cda325ecfe7877dc154312cf4b4b7941d444ba35ff8f46b1e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Orchestrates services, save, find/replace and 13 SetFocusControl call sites, each on a UI transition; no exact redundant app-command call proven.",
        "ownershipAndLifetime": "Owns service lifetimes; BeginClose retires recovery/stops services/detaches host; saves recheck baseline.",
        "failureAndCancellation": "Stale async results filtered; close completion polled after messages; queue-full post failure not established as hang.",
        "testCoverage": "Core/files/find tests cover app paths; performance tests measure process/UI metrics but no allocation-stack causality."
      },
      "plannedSha256": "15e84820a8ab6a9cda325ecfe7877dc154312cf4b4b7941d444ba35ff8f46b1e"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.h",
      "sha256": "efc666d69f8a0b0b0b68ae39b40269fd33e3ecb40eb11ce5597025dd8cbb6bff",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares application state consumed by Main and scenario/tests.",
        "ownershipAndLifetime": "Owns top-level controls/services; explicit teardown contract in Application implementation.",
        "failureAndCancellation": "Lifecycle state gates late results and close; no standalone cancellation behavior.",
        "testCoverage": "Core and design tests exercise public application state; no direct header-only tests."
      },
      "plannedSha256": "efc666d69f8a0b0b0b68ae39b40269fd33e3ecb40eb11ce5597025dd8cbb6bff"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.Navigation.cpp",
      "sha256": "ac2765c5d422960afae07d73e1f4c816fa1253fd1ebc33e884333fac679c6886",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Navigation transitions call focus at two sites after route changes; no exact duplicate invocation established.",
        "ownershipAndLifetime": "Navigation model and async result state guarded by generation/revision.",
        "failureAndCancellation": "Stale navigation results discarded; cancellation on new root/close.",
        "testCoverage": "Files and interactive navigation tests cover behavior; no perf call count assertion."
      },
      "plannedSha256": "ac2765c5d422960afae07d73e1f4c816fa1253fd1ebc33e884333fac679c6886"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/AsyncFiles.h",
      "sha256": "8eb3cda6b6e6362d7ebeda811e0a628c461b739f83b56e3110215a35a3bf85e1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "FilesService invoked from Application root/navigation commands.",
        "ownershipAndLifetime": "Single worker coalesces latest root; bounded result state and stop token limit lifetime.",
        "failureAndCancellation": "Generation/revision reject stale results; close stops without blocking UI join.",
        "testCoverage": "Files tests cover filtering/sort/limits/cancel/reparse/latest-root/coalescing/stale/close."
      },
      "plannedSha256": "8eb3cda6b6e6362d7ebeda811e0a628c461b739f83b56e3110215a35a3bf85e1"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/EditDiagnostics.h",
      "sha256": "b789d0a0dbd16e5936582719915cf873f63e672f0bda009d4df02ca7b5cfab48",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Editor diagnostics consumed by document inspector paths.",
        "ownershipAndLifetime": "Diagnostic records are value-owned and bounded by edit state.",
        "failureAndCancellation": "Invalid/stale ranges are filtered at consumers; no async worker ownership.",
        "testCoverage": "Core tests cover extraction/inspection paths; no dedicated lifetime stress identified."
      },
      "plannedSha256": "b789d0a0dbd16e5936582719915cf873f63e672f0bda009d4df02ca7b5cfab48"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/ImageLoader.h",
      "sha256": "a3c800d467186f8599b7357ac49ce59d5dba206d09d90b15b3b2d0d15873ec78",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Application image flow invokes ImageLoader, which calls Common remote image transport.",
        "ownershipAndLifetime": "Worker uses MTA; decoded image/result state scoped to loader and host lifetime.",
        "failureAndCancellation": "Cancellation plus generation checks discard stale work; shutdown tested.",
        "testCoverage": "Remote image tests cover loopback transport, revoke and shutdown; no allocation attribution."
      },
      "plannedSha256": "a3c800d467186f8599b7357ac49ce59d5dba206d09d90b15b3b2d0d15873ec78"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Main.cpp",
      "sha256": "7918862fd087de59c563b21c9be196d6c1f4a620c2f66468a1914d72d0f070af",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Entrypoint constructs/initializes Application and routes messages.",
        "ownershipAndLifetime": "Main owns application/UI lifetime through message loop.",
        "failureAndCancellation": "Initialization/dispatch errors unwind; close completion checked by loop.",
        "testCoverage": "Harness launches app scenarios; no standalone entrypoint test."
      },
      "plannedSha256": "7918862fd087de59c563b21c9be196d6c1f4a620c2f66468a1914d72d0f070af"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/NavigationModel.h",
      "sha256": "72a77ffc90965572ff011fd187e6ff831e87c73070b20a9b22d83adfed14f613",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Model is mutated by Application navigation and FilesService results.",
        "ownershipAndLifetime": "Snapshot values copied across worker/UI boundary; revisions guard identity.",
        "failureAndCancellation": "Cancel/revision invalidates obsolete navigation requests.",
        "testCoverage": "Navigation/file tests cover stale-root and latest-request behavior."
      },
      "plannedSha256": "72a77ffc90965572ff011fd187e6ff831e87c73070b20a9b22d83adfed14f613"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/RedTilde.vcxproj",
      "sha256": "99240363fa0b926aabaf6b13035a9cced00d55dbb2bafed7ec8c3d5446ea7ded",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Project compiles app, test integration and performance configurations.",
        "ownershipAndLifetime": "Build owns generated outputs; project does not own runtime data.",
        "failureAndCancellation": "Build failures are surfaced by MSBuild; profile configurations include x64/ARM64 Debug/Release/ASan.",
        "testCoverage": "No execution coverage itself; verify configurations via parent build workflow."
      },
      "plannedSha256": "99240363fa0b926aabaf6b13035a9cced00d55dbb2bafed7ec8c3d5446ea7ded"
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/UiControls.h",
      "sha256": "de0e8d6bdf69a6f0bb88c2b2d53beffd87a43b2ed03aba45d7372ece05bff780",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Defines inspector/find controls used by Application.",
        "ownershipAndLifetime": "UI control tree owns controls; callbacks route to Application.",
        "failureAndCancellation": "Find snapshot/query cache is bounded; highlight cap is 4096 and active range remains visible.",
        "testCoverage": "Core and interactive tests cover selection/find/replace; focus-taking suite not run."
      },
      "plannedSha256": "de0e8d6bdf69a6f0bb88c2b2d53beffd87a43b2ed03aba45d7372ece05bff780"
    },
    {
      "repository": "RedTilde",
      "path": "res/RedTilde.manifest",
      "sha256": "a2ab46534b26273903bc0dc9df374d9cd352e9e54b70534f901c1ee08904e484",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Embedded app activation context defines asInvoker, PerMonitorV2, longPathAware and supported Windows identity.",
        "ownershipAndLifetime": "Static executable resource; no runtime-owned object.",
        "failureAndCancellation": "Malformed resource fails resource/build activation; no custom retry.",
        "testCoverage": "No dedicated manifest runtime/parser assertion identified; build/DPI native tests pending."
      },
      "plannedSha256": "a2ab46534b26273903bc0dc9df374d9cd352e9e54b70534f901c1ee08904e484"
    },
    {
      "repository": "RedTilde",
      "path": "res/RedTilde.rc",
      "sha256": "71df47846061f1dd880d6a0bc290c668ab943e314d2069e92f45076832603d86",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Editor.DesignRefreshLiveSettingsAndEscape",
        "Editor.DesignRefreshTransientSurfacesAndClose"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Compiled app string/version/icon resource consumed by Application/Chrome.",
        "ownershipAndLifetime": "Static strings, indexed format substitutions and icon binary resource.",
        "failureAndCancellation": "Unknown/missing IDs or RC syntax errors fail resource compilation; lookup fallback elsewhere.",
        "testCoverage": "UI app cases use some strings but no exhaustive RC-ID/placeholder parity test; no native build."
      },
      "plannedSha256": "71df47846061f1dd880d6a0bc290c668ab943e314d2069e92f45076832603d86"
    },
    {
      "repository": "RedTilde",
      "path": "res/Resource.h",
      "sha256": "d3723f905d3f0d8c70def4725dec5f0f858b50871767cf90e0502268ade3b475",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Included by RedTilde.rc and app C++ as compile-time resource IDs.",
        "ownershipAndLifetime": "Integer constants; no runtime object lifetime.",
        "failureAndCancellation": "ID/string mismatch fails compile or displays wrong resource; no local recovery.",
        "testCoverage": "No exhaustive header/RC parity test identified; no resource compilation run."
      },
      "plannedSha256": "d3723f905d3f0d8c70def4725dec5f0f858b50871767cf90e0502268ade3b475"
    },
    {
      "repository": "RedTilde",
      "path": "restore-dxui.ps1",
      "sha256": "20122314d7c72b24ab8ddccd72796e8e41ef30f0e4d51725959334a0b4751b02",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Exact dependency and capability status",
        "PowerShell syntax"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "build.ps1 invokes restore once per platform unless SkipRestore; validates exact lock and canonical consumer.",
        "ownershipAndLifetime": "Pinned source/resolved props/platform dependency products remain under repo .build/dependencies/DxUi.",
        "failureAndCancellation": "Clone/checkout/consumer/vcpkg failure stops before properties are treated as restored; partial source retained for diagnosis.",
        "testCoverage": "Lock validation and PowerShell parser passed; network restore/build not run."
      },
      "plannedSha256": "20122314d7c72b24ab8ddccd72796e8e41ef30f0e4d51725959334a0b4751b02"
    },
    {
      "repository": "RedTilde",
      "path": "Test-Changes.ps1",
      "sha256": "ece5628054570552d3ef363ce27c1a172ea383726e197b5eeb7adea66cb7c1ce",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "public default records a successful affected baseline and then runs nothing unchanged",
        "a later edit selects only the newly affected scope, not earlier branch changes",
        "explicit identical success is reused and partial selection never advances the default baseline",
        "failures and post-run mutations never advance the affected baseline or retain reusable success",
        "public runner expands empty and explicit tooling argument arrays exactly",
        "public final runner rejects pending fragments before build or test discovery"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Developer Affected/Full/PrePush public scoped runner; changelog fixture checks PrePush ordering.",
        "ownershipAndLifetime": "Reads exact source/index/environment/artifact identities; successful iteration state under .build/reports; child suites delegated.",
        "failureAndCancellation": "Stale SkipBuild refuses; failures/mutations prevent receipts/baseline advancement; PrePush identifies pending CI separately.",
        "testCoverage": "Scoped fixtures cover baseline/reuse/narrowing/partial selection/failure/mutation/argument binding; changelog suite verifies gate before discovery."
      },
      "plannedSha256": "ece5628054570552d3ef363ce27c1a172ea383726e197b5eeb7adea66cb7c1ce"
    },
    {
      "repository": "RedTilde",
      "path": "test-interactive.ps1",
      "sha256": "6ffb4c8a623796e814072463186b719ab86910052dd6480edbe14443c7d54c60",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Window.AltF4AsyncClose",
        "Equation.F2ApplyUndoAndCancel",
        "test.ps1 unleased Lease.Setup refusal"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Explicit time-authorized interactive qualification; builds and validates exact pinned lease then runs required keyboard/focus child.",
        "ownershipAndLifetime": "Lease owns warning, session desktop lock, child execution/restoration; evidence and receipt are run-scoped; app/lease identities rechecked.",
        "failureAndCancellation": "Requires available desktop and no implicit auto-start; failures produce failed evidence; only opt-in switch starts countdown.",
        "testCoverage": "Required interactive IDs are checked in code but runner was not invoked; no focus-taking suite ran."
      },
      "plannedSha256": "6ffb4c8a623796e814072463186b719ab86910052dd6480edbe14443c7d54c60"
    },
    {
      "repository": "RedTilde",
      "path": "test.ps1",
      "sha256": "f11dc5abdea3bc90965443e1cd34927373dfe79f9fe763ca70e5c77d79972686",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "PreferencesProcessExit",
        "RecoveryCrashRestart",
        "Crash",
        "PerformanceBaseline",
        "Capture",
        "AddressSanitizer",
        "native catalog/partition validation"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Runtime runner called from CI/Test-Changes; supports native scopes, capture, crash, recovery restart, settings exit and perf.",
        "ownershipAndLifetime": "Receipt-verified executable, isolated evidence run, unique child data dirs and final receipt even on failure.",
        "failureAndCancellation": "Rejects stale artifact, invalid suite/profile/catalog, child exit/data errors; deliberate recovery crash is separated from restart process; failed evidence retained.",
        "testCoverage": "Runner compares list-tests catalog and behavioral outputs; named process probes include PreferencesProcessExit and RecoveryCrashRestart. No native suite/build run here."
      },
      "plannedSha256": "f11dc5abdea3bc90965443e1cd34927373dfe79f9fe763ca70e5c77d79972686"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Core.cpp",
      "sha256": "bd9c68c5165837348ff81b3228998035238ab1c564be50e123b07971eedebf62",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Registers Math.NativeSubsetAndRejection and Editor.InDocumentMathLocalSourceAndBroaderMermaid only in native self-test runner.",
        "ownershipAndLifetime": "Local sources/apps and immutable semantic publications; no new persistent state.",
        "failureAndCancellation": "Checks over-limit parser rejection, unsupported exact-source fallback, neighboring links/syntax and unchanged authoritative source.",
        "testCoverage": "4096 boundary now uses one coalesced text node; typed classes/styles/operator limits/matrix capacity covered; whitespace slash math and true parent-link-enclosed rejected math assert exact source and Bold semantic display run. Not built/run."
      },
      "plannedSha256": "8e6ae80baa9edc6b4154a4623b195075167ec72f3d97ac5eef2ac0066de2e3b8"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignInteractive.cpp",
      "sha256": "d66ca92c1469035a6365bb2f08c617d7e1ece01da88ef56354ca2e10f617e269",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.DesignInteractive.cpp"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Interactive refresh tests invoke application focus/UI pathways.",
        "ownershipAndLifetime": "Owned harness window and lease bound test lifetime.",
        "failureAndCancellation": "Test setup guards activation; not run absent focus agreement.",
        "testCoverage": "Covers design interactions; focused/manual behavior remains unexecuted here."
      },
      "plannedSha256": "d66ca92c1469035a6365bb2f08c617d7e1ece01da88ef56354ca2e10f617e269"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignInteractive.h",
      "sha256": "41c4c541952ff245b4e4cc5e6145a5e5df1bc7cf2caba3baa6e134b456ce86e6",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.DesignInteractive.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares interactive test entry points wired into test registry.",
        "ownershipAndLifetime": "Test fixture state is owned by suite.",
        "failureAndCancellation": "Activation contract delegated to implementation/harness.",
        "testCoverage": "No separate coverage beyond corresponding cpp."
      },
      "plannedSha256": "41c4c541952ff245b4e4cc5e6145a5e5df1bc7cf2caba3baa6e134b456ce86e6"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignRefresh.cpp",
      "sha256": "ea4fb87c93ff5c2662653711505b7441b4d85f004556f16dc6c46c5b0ec75110",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster",
        "Editor.DesignRefreshEquationContextAndDraftGuards"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Registers Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster and Editor.DesignRefreshEquationContextAndDraftGuards.",
        "ownershipAndLifetime": "Hidden app, WIC decoder, pixel arrays and capture paths local to test-owned fixture root.",
        "failureAndCancellation": "Raster must remain exact after resource reset/rebuild and warm re-render; draft cancellation preserves history; apply is undoable.",
        "testCoverage": "Adds warm repeated capture equality plus Source/Split/Compose and F2 cancel/apply/undo/redo parent-title checks. Case not built/run."
      },
      "plannedSha256": "f6b00df14da0d41ddc4d0d2a4aa5fb1a17f8795da1e3ce7efa71aa40f9b5989e"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignRefresh.h",
      "sha256": "bfa91d004cc818f23aec07609b23a985245c16fd4afebcaa4c7dd4a4bb2251c7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.DesignRefresh.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares design refresh tests referenced by test registration.",
        "ownershipAndLifetime": "No runtime resource ownership.",
        "failureAndCancellation": "No independent cancellation path.",
        "testCoverage": "Coverage in corresponding cpp; header itself is declaration only."
      },
      "plannedSha256": "bfa91d004cc818f23aec07609b23a985245c16fd4afebcaa4c7dd4a4bb2251c7"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Files.cpp",
      "sha256": "ce324aa55e067c0006d6e027ec9663458c191fd69ad40d9a9ce2881c1a73c150",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.Files.cpp"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Tests FilesService/app navigation/root and result delivery.",
        "ownershipAndLifetime": "Fixture owns temp sandbox and service lifetime.",
        "failureAndCancellation": "Cases cover cancellation, stale generation, coalescing and close.",
        "testCoverage": "Broad FilesService lifecycle and filtering coverage; not run."
      },
      "plannedSha256": "ce324aa55e067c0006d6e027ec9663458c191fd69ad40d9a9ce2881c1a73c150"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Files.h",
      "sha256": "3df3de8a09e0bfe3e747050306d2066837a400f8d0bfe29a55ba192402953382",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.Files.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares Files tests registered by app test harness.",
        "ownershipAndLifetime": "No owned runtime state.",
        "failureAndCancellation": "Failure/cancellation exercised by implementation tests.",
        "testCoverage": "Coverage in corresponding cpp."
      },
      "plannedSha256": "3df3de8a09e0bfe3e747050306d2066837a400f8d0bfe29a55ba192402953382"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Performance.cpp",
      "sha256": "ac7f6912cce121af62752a2e606ece5c40bd37a9bebcd81eb299a9782fcbb164",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.Performance.cpp"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Performance scenario collects private bytes/working set/resources/frame timing/CPU by scenario.",
        "ownershipAndLifetime": "Metrics and scenario artifacts owned by harness run.",
        "failureAndCancellation": "Scenario has bounded cycles and reports missing/invalid metrics; no heap stack trace in this implementation.",
        "testCoverage": "Perf instrumentation exists; seven historical regressions remain open and current diagnostic lacks allocation stack."
      },
      "plannedSha256": "ac7f6912cce121af62752a2e606ece5c40bd37a9bebcd81eb299a9782fcbb164"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.RemoteImages.cpp",
      "sha256": "0f524f78a4ae959cf3681475c39530baa7ffc2f81d96fb0295fac55e9c35c012",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.RemoteImages.cpp"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Tests Application image loader and remote image requests.",
        "ownershipAndLifetime": "Loopback server and fixture resources are test-owned.",
        "failureAndCancellation": "Covers auth/redirect/size/deadline/cancel/revoke/shutdown.",
        "testCoverage": "Focused transport and app lifecycle tests; not run."
      },
      "plannedSha256": "0f524f78a4ae959cf3681475c39530baa7ffc2f81d96fb0295fac55e9c35c012"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.RemoteImages.h",
      "sha256": "76516bd27adcd39961edec246d45d4ef0f66000912fcc75920a84159fbb84f27",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Application.Tests.RemoteImages.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Declares remote image cases wired to test harness.",
        "ownershipAndLifetime": "No owned resource itself.",
        "failureAndCancellation": "No independent failure handling.",
        "testCoverage": "Coverage in cpp."
      },
      "plannedSha256": "76516bd27adcd39961edec246d45d4ef0f66000912fcc75920a84159fbb84f27"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/native-case-catalog.json",
      "sha256": "c166b6eb606de95c54ad508c0c25f4817e11b5464fe54b6e934fc18eb6168e7e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "test.ps1 compares catalog against app list-tests for every native scope.",
        "ownershipAndLifetime": "Static case/scope record only.",
        "failureAndCancellation": "Catalog/list mismatch fails before behavioral suite is counted.",
        "testCoverage": "BrandMark device-loss raster case added to Editor scope; validator passes but native listing is not executed until build."
      },
      "plannedSha256": "942e80483caf9d91d3a8600ec5874971b5b47f70948c77efbd04bde0aa2304be"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/NativeInventory.json",
      "sha256": "8f1060c529717296214efc7879299065c56909c0cd76fc87802841da223c1c9b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/NativeInventory.json"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Inventory registers source/test ownership for native suite.",
        "ownershipAndLifetime": "Declarative inventory.",
        "failureAndCancellation": "Stale entries can hide tests from tooling.",
        "testCoverage": "Inventory validation belongs to test tooling; not run."
      },
      "plannedSha256": "8f1060c529717296214efc7879299065c56909c0cd76fc87802841da223c1c9b"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Diagnostics.ps1",
      "sha256": "3386f7a85904c7a00e50b5f784598987698c87546955ff7411cfaf8d5877079c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Diagnostics.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Performance scope diagnostics consume run evidence and source metadata.",
        "ownershipAndLifetime": "PowerShell temp/artifacts constrained by test sandbox policy.",
        "failureAndCancellation": "Rejects malformed/untrusted diagnostic records; actual script not executed.",
        "testCoverage": "Tooling tests cover scope contract; diagnostic stack capture absent."
      },
      "plannedSha256": "3386f7a85904c7a00e50b5f784598987698c87546955ff7411cfaf8d5877079c"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Evidence.ps1",
      "sha256": "d4b38a749c6824ff1ce344e472f077c7b4f3094676fb8c1313f46287dad950c2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Evidence.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Validates evidence bundle identity and run comparisons.",
        "ownershipAndLifetime": "Evidence paths/records are read-only inputs to validator.",
        "failureAndCancellation": "Rejects incomplete/mismatched cohorts.",
        "testCoverage": "ABBA/BAAB and identity validation; no current full same-SDK study."
      },
      "plannedSha256": "d4b38a749c6824ff1ce344e472f077c7b4f3094676fb8c1313f46287dad950c2"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Observation.ps1",
      "sha256": "e65621638f44e3e8aa2e9f346afe9f893939fa2881b5f9d530121fc5e38a6e2b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Observation validation checks metric records and run consistency.",
        "ownershipAndLifetime": "No application-owned state.",
        "failureAndCancellation": "Outlier/missing data handled as invalid/inconclusive rather than success.",
        "testCoverage": "Perf evidence tests cover parsing/consistency; not run."
      },
      "plannedSha256": "e65621638f44e3e8aa2e9f346afe9f893939fa2881b5f9d530121fc5e38a6e2b"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Tooling.ps1",
      "sha256": "da854c344a56ffc05fa62db2925309508c04e59d9e44c67482cdd1ba4ce1756a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Validates performance tooling wiring and policy surfaces.",
        "ownershipAndLifetime": "Tooling controls own run directories.",
        "failureAndCancellation": "Failure paths protect against invalid paths/configuration; global profile behavior out of scope.",
        "testCoverage": "Tooling tests exercise inventory/argument contracts; not run."
      },
      "plannedSha256": "da854c344a56ffc05fa62db2925309508c04e59d9e44c67482cdd1ba4ce1756a"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Support/Support.Tests.FilesFixtures.h",
      "sha256": "f2306c9fee4e7bd54a8f66c06e471723e5a479b6bb8bf178207bec22abb425bd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Support/Support.Tests.FilesFixtures.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "File test fixture helpers are consumed by Files tests.",
        "ownershipAndLifetime": "Fixtures own sandboxed paths/resources and scoped cleanup.",
        "failureAndCancellation": "Fixture setup failures fail closed; cleanup respects sandbox ownership.",
        "testCoverage": "Coverage inherited by Files tests; individual helper contracts not separately enumerated."
      },
      "plannedSha256": "f2306c9fee4e7bd54a8f66c06e471723e5a479b6bb8bf178207bec22abb425bd"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Support/Support.Tests.TextServices.h",
      "sha256": "81a6866c450d1a0c5035f68c9f232c3dbb538cfa1ea8f00812fb166efdf37ccb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Support/Support.Tests.TextServices.h"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Text service fixtures support editor/document tests.",
        "ownershipAndLifetime": "Text buffers/COM service objects are scoped to fixture lifetime.",
        "failureAndCancellation": "Failure paths release fixture-owned services; async text callbacks require host teardown.",
        "testCoverage": "Core/remote text coverage uses helpers; no dedicated helper test."
      },
      "plannedSha256": "81a6866c450d1a0c5035f68c9f232c3dbb538cfa1ea8f00812fb166efdf37ccb"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/test-scopes.json",
      "sha256": "5e02ac35d2f4671331471e364497b1e0f6b9700d5ab205eb51e9bf01eaa70b29",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/test-scopes.json"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Maps scope names to test cases used by affected iteration.",
        "ownershipAndLifetime": "Declarative metadata.",
        "failureAndCancellation": "Incorrect mapping can omit execution; no runtime cancellation.",
        "testCoverage": "Validate mapping with tooling test; not run."
      },
      "plannedSha256": "5e02ac35d2f4671331471e364497b1e0f6b9700d5ab205eb51e9bf01eaa70b29"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Tooling.Tests.RunEvidence.ps1",
      "sha256": "04e6aa403a22a8921b7c5e8548fa92d12533508d8a5098cfdecedfda9268a2ef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Tooling.Tests.RunEvidence.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Tests run evidence generation/validation callers.",
        "ownershipAndLifetime": "Temporary evidence governed by sandbox policy.",
        "failureAndCancellation": "Covers malformed/missing identity and failure handling.",
        "testCoverage": "Run-evidence tooling tests reviewed, not executed."
      },
      "plannedSha256": "04e6aa403a22a8921b7c5e8548fa92d12533508d8a5098cfdecedfda9268a2ef"
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Tooling.Tests.ScopedTesting.ps1",
      "sha256": "507db3426d59f1a7095454a62676c45d7d031a2458e6e950d1e1ea7e2a32e80d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tests/Tooling.Tests.ScopedTesting.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Tests scoped testing and affected-set resolution.",
        "ownershipAndLifetime": "Harness-owned state and temp remain within test sandbox.",
        "failureAndCancellation": "Fail-closed on invalid/empty scope evidence.",
        "testCoverage": "Scope-tooling coverage reviewed, not executed."
      },
      "plannedSha256": "507db3426d59f1a7095454a62676c45d7d031a2458e6e950d1e1ea7e2a32e80d"
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "single-fragment final gate is read-only and requires folding",
        "preview writes nothing",
        "newest first, stable topic order, preserve released history and README",
        "preserve BOM, CRLF and Unicode bytes",
        "retry after publication cleans fragments without duplicate entries",
        "overlapping fold fails without touching inputs",
        "Unreleased heading at EOF is supported"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Developer and PrePush/final handoff wrapper; imports shared module.",
        "ownershipAndLifetime": "Exclusive repo-local lock; temp staged beside changelog; publishes changelog before deleting re-read fragments.",
        "failureAndCancellation": "Check/WhatIf read-only; malformed/reparse/race/lock failures refuse; retry is deduped.",
        "testCoverage": "Synthetic workflow cases passed for preview, single fragment, ordering/history, BOM/CRLF, retry, invalid inputs, lock and EOF."
      },
      "plannedSha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba"
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Get-CiNativeScope.ps1",
      "sha256": "52d66cfd12a2f68f745f69f57c3f3c6f4ed235b2b82656cc8a639bb6ad7a6db1",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "CI covers six exact native profiles and tooling once with no duplicate main-push run",
        "only a clean committed candidate can delegate matching enabled CI, always as pending",
        "disabled/unavailable CI, stale workflow and mismatched profiles keep obligations local",
        "working and clean-commit races during coverage lookup refuse delegation",
        "direct default-branch pushes have no forthcoming PR coverage",
        "retained CI proof requires all six actual successful native executions"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "CI scope job creates/verifies native descriptor; optional gh API/artifact reads prior PR proof.",
        "ownershipAndLifetime": "Descriptor and downloaded evidence under repo .build/ci-scope; each external download uses run-specific folder.",
        "failureAndCancellation": "Uncertain, stale, missing or failed latest native evidence selects fresh matrix; verification compares exact input key and build number.",
        "testCoverage": "Fixture tests cover workflow digest, profile mapping, delegation refusal/proof and races; no live GitHub run/API called."
      },
      "plannedSha256": "52d66cfd12a2f68f745f69f57c3f3c6f4ed235b2b82656cc8a639bb6ad7a6db1"
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "22 behavioral checks in Tools/Test-ChangelogWorkflow.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fold-Changelog and Test-Changelog import module; Test-ChangelogWorkflow drives synthetic fixture repos.",
        "ownershipAndLifetime": "Strict UTF-8 reads; preserves original newline/BOM; exclusive fold lease and temp staging.",
        "failureAndCancellation": "Rejects reparse paths/malformed fragment/layout/content mutation; publishes before deleting; retry deduplicates.",
        "testCoverage": "22 behavioral changelog fixture checks passed under validate.ps1."
      },
      "plannedSha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d"
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "editing permits one valid dated Unicode change",
        "invalid fragment rejection cases without mutation",
        "single-fragment final gate is read-only and requires folding",
        "relative roots follow PowerShell location without touching the process directory"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "validate.ps1/developers validate edits; strict folded mode used by final handoff.",
        "ownershipAndLifetime": "Read-only wrapper; default root is command repo; relative roots resolve in PowerShell provider.",
        "failureAndCancellation": "Invalid input/pending strict-gate fragments throw before mutation.",
        "testCoverage": "Synthetic workflow validates valid/invalid fragments, read-only final check and relative-root behavior; standalone command passed."
      },
      "plannedSha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18"
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "editing permits one valid dated Unicode change",
        "invalid fragment rejection cases",
        "single-fragment final gate is read-only and requires folding",
        "preview writes nothing",
        "relative roots follow PowerShell location without touching the process directory",
        "newest first, stable topic order, preserve released history and README",
        "preserve BOM, CRLF and Unicode bytes",
        "retry after publication cleans fragments without duplicate entries",
        "missing README, invalid headings, unexpected files and directories fail",
        "overlapping fold fails without touching inputs",
        "Unreleased heading at EOF is supported",
        "public final runner rejects pending fragments before build or test discovery",
        "a timed-out fixture child is terminated before fixture cleanup"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "validate.ps1 runs it with existing repo .build scratch; standalone cross-project regression suite copies wrapper/module into owned fixtures.",
        "ownershipAndLifetime": "Creates GUID direct children and deletes only verified exact children; timeout kills only child whose PID/start identity it recorded.",
        "failureAndCancellation": "Cleanup path/name validated; child output drains with bound; all failures reported at end.",
        "testCoverage": "Latest run reports 22 passed, zero failed; fixtures include format, path, encoding, retry, lock and gate ordering."
      },
      "plannedSha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94"
    },
    {
      "repository": "RedTilde",
      "path": "validate.ps1",
      "sha256": "c3cfa6959f7389f89ce452f4d62c9dabfb022dc22499c8d31febb9b2219ebb58",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Changes workflow",
        "Specifications and links",
        "Skills and scoped agent guidance",
        "Exact dependency and capability status",
        "Six build configurations and native inventory",
        "Run evidence retention",
        "Scope contracts and public runner",
        "PowerShell syntax",
        "Performance protocol"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "CI tooling and test.ps1 Tooling suite invoke it; developers run directly.",
        "ownershipAndLifetime": "Orchestrates metadata/script validators and fixture suites; fixtures are beneath repo .build and owned by each suite.",
        "failureAndCancellation": "Runs checks independently, collects failures, and terminates if any fail; does not build or run product native suites.",
        "testCoverage": "Latest direct run passed Changes workflow (22), specs/links, skills, pin/capability, six configurations/catalog, run evidence (38), scoped tooling (25), PowerShell syntax and performance validators."
      },
      "plannedSha256": "c3cfa6959f7389f89ce452f4d62c9dabfb022dc22499c8d31febb9b2219ebb58"
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/ci.yml",
      "sha256": "4efd69a3b5ef1f3d495cc24f5642883faae62a840ec8ef22bdade731f3292d24",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "CI tests: Test-CiGate, Test-NativeScope, Test-BenchmarkGate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Push-to-main, PR merge-ref, and manual dispatch invoke tooling/native gates; paired benchmark uses Get-BenchmarkScope and workflow outputs.",
        "ownershipAndLifetime": "Workflow uses pinned action SHAs, contents:read except the isolated gallery workflow, finite per-job deadlines, concurrency cancel for superseded runs, retained artifacts for 14 days.",
        "failureAndCancellation": "Scope decision errors fail; native jobs run if scope undecidable; cancellation does not publish a verdict. Paired setup/measurement/restoration/verdict stages are gated and artifacts upload after failures.",
        "testCoverage": "Tooling cases: Tools/tests/Test-CiGate.ps1, Test-NativeScope.ps1, Test-BenchmarkGate.ps1; native/paired hosted workflows are not executed in this review."
      },
      "plannedSha256": "79e071d8bf16708f8e075ae9141502c2645b8b4cf8c75b8334a917adc96855af",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/format.yml",
      "sha256": "d38b2ff4b5948e01839064c9388c051a19d0959c0f42f2216a010fa0289585fc",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "format.ps1 -Check; Test-Validation"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Push/PR path filters call pinned clang-format install and format.ps1; manual opt-in reformat writes only non-main branches.",
        "ownershipAndLifetime": "Read-only permission in check job; write token only in explicit reformat job, branch-gated; no product artifact ownership.",
        "failureAndCancellation": "Check job uploads diff patch on failure; format errors stop. Reformat is gated by manual input and non-main ref; no tests in this read-only pass.",
        "testCoverage": "Tools/tests/Test-Validation.ps1 plus format.ps1 -Check are declared gates; neither run in this review."
      },
      "plannedSha256": "1cdaa9cb299e5985abae3e641e45f503f4b0451213c1a217768b437af43c81d1",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/gallery.yml",
      "sha256": "6546eb19d959f7171600e453e3605105bec9d62b1ba2e04182ac6af5891c5bac",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Docs; Gallery suite; validate-specs.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Manual publish workflow runs vcpkg restore, gallery.ps1 -PublishDocs, validate-specs, then Commit-Gallery.",
        "ownershipAndLifetime": "Write permission scoped to publish job; checkout branch only; publish condition refuses main; gallery source generated from control test and embedded sample outputs.",
        "failureAndCancellation": "Validation runs before commit; ordinary non-force push; no post-push validation step is present in workflow, but commit step is after spec validation. Workflow not executed.",
        "testCoverage": "Tools/tests/Test-Docs.ps1 checks gallery/catalog artifacts; gallery test suite and validate-specs are workflow gates, not run here."
      },
      "plannedSha256": "6546eb19d959f7171600e453e3605105bec9d62b1ba2e04182ac6af5891c5bac",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "build.ps1",
      "sha256": "357dccefd0839bac6acfc8833a2c80b0493577ab599d673b1320fb773eb0f2a5",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix; Test-PerformancePolicy"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Root build entrypoint; vcpkg-install.ps1, performance/test entrypoints call it. Discovers VS via Tools/VisualStudio.psm1 and emits six bounded phases via Tools/PerformancePolicy.psm1.",
        "ownershipAndLifetime": "Default outputs/logs under .build; optional consumer output root isolated; checks exact executable paths through CIM but never terminates; default output builds get before/after source/toolchain/artifact attestation.",
        "failureAndCancellation": "Missing dependencies/VS, running exact output exe, failed MSBuild or failed post-attestation stop success. Build is not run.",
        "testCoverage": "Test-BuildMatrix and Test-PerformancePolicy cover adjacent configuration/attestation contracts; no build execution claim."
      },
      "plannedSha256": "357dccefd0839bac6acfc8833a2c80b0493577ab599d673b1320fb773eb0f2a5",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.AddressSanitizer.targets",
      "sha256": "25ebf22bd76677353b2c689c9b08793fccd99252a5b982a15dbd50aafb0ca059",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-AsanRuntime.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Imported by Build/DxUi.Consumer.targets and Directory.Build.props/targets to stage ARM64 ASan runtime.",
        "ownershipAndLifetime": "Runtime path derives from selected VCToolsInstallDir; MSBuild owns staging under target output; no manual cleanup.",
        "failureAndCancellation": "Missing runtime emits MSBuild Error before copy; other platforms use built-in target path.",
        "testCoverage": "Tools/tests/Test-AsanRuntime.ps1 has dedicated runtime validation; not run."
      },
      "plannedSha256": "25ebf22bd76677353b2c689c9b08793fccd99252a5b982a15dbd50aafb0ca059",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.Consumer.props",
      "sha256": "52d2cf3eeaffc860c7e3ba9ff54c3cd9e02c5615cd9561da7b7e28f6deece57c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 external public consumer"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Imported by external consumers before their target files; contributes public include and pinned installed-WIL include paths and optional STL-annotation define.",
        "ownershipAndLifetime": "Resolved root is relative to props location by default; output root is consumer-supplied and does not write itself.",
        "failureAndCancellation": "Empty/malformed roots surface through downstream target validation; file itself has no error task.",
        "testCoverage": "test-consumer.ps1 generates relocated external project and exercises this import; not run."
      },
      "plannedSha256": "52d2cf3eeaffc860c7e3ba9ff54c3cd9e02c5615cd9561da7b7e28f6deece57c",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.Consumer.targets",
      "sha256": "0d62da7c448da3cb49c6971a6ef373349841727ef5c82931774a521352b76120",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix; test-consumer.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "External consumer imports after Microsoft.Cpp.targets; references exactly src/DxUi.vcxproj with matching platform/config/output/annotation properties.",
        "ownershipAndLifetime": "ASan forced include and project-reference props follow resolved root and isolated output root; no source list duplication.",
        "failureAndCancellation": "Before PrepareForBuild validates config, lock, toolset/SDK/CRT-adjacent identities and restored WIL; validation process nonzero stops build.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 checks six config mappings; test-consumer.ps1 compiles relocated fixture (not run)."
      },
      "plannedSha256": "0d62da7c448da3cb49c6971a6ef373349841727ef5c82931774a521352b76120",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "capabilities.json",
      "sha256": "782da4cee202c526e932665011de9360aa224033d7c9df71cef6075034b33b16",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Validation; Test-ConsumerUpdate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Read by consumer-interface validation, test-consumer.ps1, docs/spec validation and consumers to discover API revision and supported/pending capabilities.",
        "ownershipAndLifetime": "Machine-owned public interface lists exact scripts, module functions, MSBuild imports, and headers; exact vcpkg API/toolchain versions owned by separate manifests.",
        "failureAndCancellation": "JSON parse/schema or missing consumer entries fail validation; pending lists explicitly separate consumer/platform/runtime qualification.",
        "testCoverage": "Tools/tests/Test-Validation.ps1 and Test-ConsumerUpdate.ps1 validate interfaces and update semantics; no validator run."
      },
      "plannedSha256": "782da4cee202c526e932665011de9360aa224033d7c9df71cef6075034b33b16",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Directory.Build.props",
      "sha256": "a9835e32463f717079de40bb1294b0fe1d1b83e71c160be605683fb366e92be2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix; Test-AsanRuntime"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Imported by DxUi solution native projects; default output/int dirs, triplets, Unicode/C++ language/runtime/warnings and ASan configuration.",
        "ownershipAndLifetime": "Repo-root paths normalize to MSBuildThisFileDirectory; separate output and intermediate roots; ASan binary copy owned by DxUi target only.",
        "failureAndCancellation": "Unsupported platform/configuration/annotation option errors before PrepareForBuild; no compile performed.",
        "testCoverage": "Test-BuildMatrix.ps1 checks project config; Test-AsanRuntime verifies runtime contract; not run."
      },
      "plannedSha256": "a9835e32463f717079de40bb1294b0fe1d1b83e71c160be605683fb366e92be2",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Directory.Build.targets",
      "sha256": "cc008a910e83c5fc4b213060f29b77df1bf8de929fa0d904e3181cfbd618c0cb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix; Test-AsanRuntime"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Imported at solution-project build end; adds ASan forced include/instrumentation and validation target.",
        "ownershipAndLifetime": "Compiler forced include derives repository root; no external resource ownership.",
        "failureAndCancellation": "Unsupported profile and invalid annotation flag fail before PrepareForBuild.",
        "testCoverage": "Test-BuildMatrix and Test-AsanRuntime are adjacent tooling coverage; no MSBuild executed."
      },
      "plannedSha256": "cc008a910e83c5fc4b213060f29b77df1bf8de929fa0d904e3181cfbd618c0cb",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "DxUi.sln",
      "sha256": "6cd86f1e17aea6e7518082afd3e1730e24becc31bc2acd30c2b106a9f468b50e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Root build.ps1 invokes this solution; contains DxUi static library, Foundation/Control/Embedded tests, embedded sample and InteractiveLease.",
        "ownershipAndLifetime": "Six Debug/Release/ASan Debug x x64/ARM64 mappings are explicit for all six project GUIDs.",
        "failureAndCancellation": "Solution configuration mapping is validated statically; no build failure/runtime behavior can be inferred from solution text.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 statically validates all live .sln/.vcxproj mappings; not run."
      },
      "plannedSha256": "6cd86f1e17aea6e7518082afd3e1730e24becc31bc2acd30c2b106a9f468b50e",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "format.ps1",
      "sha256": "4b120fbcffe2c6f74afd8ba880c401f6dab4f7e082ee356b42c916acecfafdef",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "format workflow; format.ps1 -Check"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Called by CI formatting workflow and developers; formatter resolved from explicit env/arg, repo pinned install, PATH, then VS LLVM.",
        "ownershipAndLifetime": "Recursively targets owned cpp/h under src/include/Tests/Samples; `-Check` no writes; non-check preserves timestamps for clean files.",
        "failureAndCancellation": "Rejects missing/wrong clang-format 22.1.3; formatter nonzero throws at file.",
        "testCoverage": "CI runs format.ps1 and -Check; no formatter installed/invoked in this review."
      },
      "plannedSha256": "4b120fbcffe2c6f74afd8ba880c401f6dab4f7e082ee356b42c916acecfafdef",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "gallery.ps1",
      "sha256": "a29695b042f77c25026756299dd8179272d11746bcedee6bc21b35bdf48036e6",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Gallery suite; Test-Docs"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Builds or reuses suite output; runs ControlTests Gallery and EmbeddedControls sample; optional docs publish to docs/gallery.",
        "ownershipAndLifetime": "Temporary output defaults .build/gallery; published receipt captures source commit/dirty state/profile/native arch/control count/image hashes; catalog count derives ControlCatalog.cpp.",
        "failureAndCancellation": "Native test/sample nonzero or image count != 6 fails. Publish copies images/index/readme/receipt after render; no gallery was generated.",
        "testCoverage": "Control Gallery suite and Tools/tests/Test-Docs.ps1 validate output contract; not run."
      },
      "plannedSha256": "a29695b042f77c25026756299dd8179272d11746bcedee6bc21b35bdf48036e6",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/AccessibilityTextUnits.h",
      "sha256": "051b4b92289854cd9a404f24e8f34403763c3bd519e60472ee4a2cf59c249f1c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "051b4b92289854cd9a404f24e8f34403763c3bd519e60472ee4a2cf59c249f1c",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Configuration.h",
      "sha256": "bd193165742a2204e5a0ed71cef2d1663a8345579173d5c948e6f0853947441f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "bd193165742a2204e5a0ed71cef2d1663a8345579173d5c948e6f0853947441f",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/ControlCatalog.h",
      "sha256": "8ae54ea18460382b1fc6b2c819f4ca109863bacb552ef1a5717281fff6c28e97",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "8ae54ea18460382b1fc6b2c819f4ca109863bacb552ef1a5717281fff6c28e97",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Diagnostics.h",
      "sha256": "1539bdc27c5f2d07a49f84bcd51f406e51c567e4e4482545101ce6e92093a51c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "1539bdc27c5f2d07a49f84bcd51f406e51c567e4e4482545101ce6e92093a51c",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/DxUi.h",
      "sha256": "346892bee7804141d3ab97f03a60bedc55329935a4eb8150a0c06b1f6c74bb8e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestWindowHostDetachDeactivatesSecureTextInputBeforeDestroyingRoot",
        "TestWindowHostProcessExitDetachAbandonsRetainedControlObserversBeforeNativeTeardown",
        "TestWindowHostKeyboardMessageRejectsRetiredForeignAndFailedOwners",
        "TestNativeTextInputBackendImeCompositionPayloadPreviewsAndCancelRestoresBase",
        "TestNativeTextInputBackendUndoRedoAndRedoClear",
        "TestWindowHostAccessibilityRegistrationFailureRollsBackAndAllowsRetry"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full public header read (6,090 lines). Reviewed ControlHost API, focus/input/IME dispatch declarations, posted-work diagnostics, rendering/presentation lifecycle and embedded/animation hooks against current host and test surfaces; this header declares contracts only, with implementation in DxUi.WindowHost.cpp and DxUi.NativeTextInput.cpp.",
        "ownershipAndLifetime": "The header documents _root-owned control tree, non-owning interaction observers, weak lifetimes for retained text-input/tooltip targets, posted text-store cookie, COM resource ownership, detach and process-exit observer abandonment. The declarations were checked against these stated boundaries; no source-confirmed defect found.",
        "failureAndCancellation": "Reviewed declarations for detach, capture/focus pruning, prepared-content cancellation, text-store deferred-work invalidation, IME composition cancellation and rendering/resource recovery. This is a source review only; no build or runtime behavior was tested.",
        "testCoverage": "Relevant named coverage in current tests includes TestWindowHostDetachDeactivatesSecureTextInputBeforeDestroyingRoot, TestWindowHostProcessExitDetachAbandonsRetainedControlObserversBeforeNativeTeardown, TestWindowHostKeyboardMessageRejectsRetiredForeignAndFailedOwners, TestNativeTextInputBackendImeCompositionPayloadPreviewsAndCancelRestoresBase, TestNativeTextInputBackendUndoRedoAndRedoClear and TestWindowHostAccessibilityRegistrationFailureRollsBackAndAllowsRetry. No tests were executed by this reviewer."
      },
      "plannedSha256": "4369858bb69ebdf89a13b20c37b32011e5c1b94b8156d2c6ce885912260ea369",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Embedded.h",
      "sha256": "00b49ce34e0eae0ea22e82aacc408606082cff47b164e1d8c2d2a54079a53947",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "6ceacf9dacb6dc75a4d7e4dbd3f97df18086a31d9501324c68d520791af09c5c",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/EmbeddedAccessibility.h",
      "sha256": "ed19e79e7ff1c98c3e524aeacf91c4687e847f97e2f50225a9b09c5cf2de2ebf",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "ed19e79e7ff1c98c3e524aeacf91c4687e847f97e2f50225a9b09c5cf2de2ebf",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/FocusRestore.h",
      "sha256": "34dc2c6b4213f845819b5a4da8feb6232c79388efb23e6853677821ccb927853",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "34dc2c6b4213f845819b5a4da8feb6232c79388efb23e6853677821ccb927853",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/FrameRuntime.h",
      "sha256": "7e00d607d6ec6c8e5d34cbfd30e7f01f384aa73cd3025b2b7581fa6df45c8aac",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "7e00d607d6ec6c8e5d34cbfd30e7f01f384aa73cd3025b2b7581fa6df45c8aac",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/MarkdownSyntax.h",
      "sha256": "9417156cf96e3d7dd80b77dba33cd9e334dc4adfcd6851b8bdd28cb4b06da4e8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks",
        "Support.Tests.MarkdownInlines",
        "Support.Tests.MarkdownCapacity"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "MarkdownParser is the only constructor/publication entry and SemanticBuilder consumes immutable syntax.",
        "ownershipAndLifetime": "Syntax owns source, revision, dialect, lexical/block/inline trees, footnotes and diagnostics; all indices are revision-local.",
        "failureAndCancellation": "Begin contract preserves prior request on failure; Advance discards staged request on failure; caps and scalar/source coordinates explicit.",
        "testCoverage": "Support.Tests.MarkdownBlocks, MarkdownInlines, MarkdownCapacity and corpus tests cover parsing; exact checks are attributed on their test rows."
      },
      "plannedSha256": "9417156cf96e3d7dd80b77dba33cd9e334dc4adfcd6851b8bdd28cb4b06da4e8",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/NativeMenuInterop.h",
      "sha256": "e4b12127af6dcdb4c158b75fee5800cb94ae0a2a0e5e6805b16b85f109da28c4",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "e4b12127af6dcdb4c158b75fee5800cb94ae0a2a0e5e6805b16b85f109da28c4",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/PointerInput.h",
      "sha256": "c16de418c74f1f50e0d9568736b9914a7b947716844706b299eeacdeec4b928d",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "c16de418c74f1f50e0d9568736b9914a7b947716844706b299eeacdeec4b928d",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/PreparedTreeAccessibility.h",
      "sha256": "ccec7113bfe2aa93e51d4beb89e2572f7c15c2990a653d68f87d759ee7664b89",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.PreparedTreeAccessibilityFixture",
        "Embedded.PreparedTreeAccessibilityRowsFixture"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Tree UIA host adapters consume IPreparedTreeAccessibilityRows; accessibility snapshots retain shared source owner.",
        "ownershipAndLifetime": "Borrowed text view lifetime is supplied by retained immutable source owner; interface is data-only/no COM/UI ownership.",
        "failureAndCancellation": "Count/GetItem/FindItem are noexcept bounded reads; documentation requires epoch replacement when row semantics change.",
        "testCoverage": "Prepared-tree accessibility embedded fixtures cover snapshot lifetime/epoch; no standalone interface-only test."
      },
      "plannedSha256": "d8f29f824a20fd48adf3e97345988275f4777eeeb73c6773b784a0da32ec1a0a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichDocument.h",
      "sha256": "567884f5dbc1caa7895736f073b10b9c1ec15f1c5c6f5eac227a17d50fb2e359",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument",
        "Embedded.RichDocumentStorageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Document is source/model base for parser, semantic transactions and editor; snapshots feed background read-only parsing.",
        "ownershipAndLifetime": "Immutable snapshots share piece-tree revision state; document owns selection and bounded undo/redo history.",
        "failureAndCancellation": "UTF-16 scalar/CRLF boundaries and 64 Ki-unit/history caps guard replacement; transient composition changes retain/restore history semantics.",
        "testCoverage": "Controls.Tests.RichDocument and rich-document storage/failure fixtures cover pieces/history; no run here."
      },
      "plannedSha256": "567884f5dbc1caa7895736f073b10b9c1ec15f1c5c6f5eac227a17d50fb2e359",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichImage.h",
      "sha256": "2641ebf4c5d2d053f87de96a3d2584bb9e9a0f274a5b705ac113a297a5bec4cc",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for RichImage.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for RichImage.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "2641ebf4c5d2d053f87de96a3d2584bb9e9a0f274a5b705ac113a297a5bec4cc",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMarkdown.h",
      "sha256": "de050951881f6de69c9e3c21a933720495ea6b6134eeadb21381476f06412ae1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown.RunRichMarkdownTests"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Legacy projection path backs v0 layout while semantic layout is opt-in; ToggleMarker edits Document source.",
        "ownershipAndLifetime": "Projection owns source/display/maps/runs; immutable block projection payloads share line outputs across preparations.",
        "failureAndCancellation": "PreparedProjection stages changed blocks/indexes before swap; API explicitly disclaims CommonMark conformance.",
        "testCoverage": "Controls.Tests.RichMarkdown exercises projection, incremental reuse and mapping; no run here."
      },
      "plannedSha256": "de050951881f6de69c9e3c21a933720495ea6b6134eeadb21381476f06412ae1",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMath.h",
      "sha256": "7ca8efde3a6e3d255770c507dcdf649f9e5ce2fd58ba32cca45297dafb60a5e0",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Public math interface exposes typed model/result and bounded options.",
        "ownershipAndLifetime": "Result ownership is value-based; caller retains source independently.",
        "failureAndCancellation": "Failure status distinguishes unsupported/error from ready output.",
        "testCoverage": "Support.Tests.MathCorpus exercises public contract cases. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "be4838496d5c14ba6f85613677d1331663e81d7c4f2d9c40ee27a0f00eeb4e08",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemantic.h",
      "sha256": "f5f24b5634463f39f564bc02986fdeeda2bbbbb3e69f64e2a937cf18186e29a9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "RichSemanticDocument uses SourceDisplayMap; semantic commands and editor map display selections back to source.",
        "ownershipAndLifetime": "Immutable map owns source/display spans and affinity indexes; SourceEdit replacement views are synchronous borrowed values.",
        "failureAndCancellation": "Build validates partition, ownership and UTF-16; ApplySourceEdits validates revision/ranges and stages before Document::Replace.",
        "testCoverage": "Support.Tests.RichSemanticDocument and command/source-change fixtures cover mapping and edits; no run here."
      },
      "plannedSha256": "f5f24b5634463f39f564bc02986fdeeda2bbbbb3e69f64e2a937cf18186e29a9",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemanticDocument.h",
      "sha256": "382e2e063531b4b86a624a6d8208c65b589799a44e6eb782a6480e86f5b705d1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.SemanticDiagrams",
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "SemanticBuilder consumes MarkdownSyntax and publishes immutable SemanticDocument consumed by geometry, resources and UIA.",
        "ownershipAndLifetime": "Document owns syntax, map, elements/paragraphs/navigation, style runs and value-only extension model; no SDK/host resource.",
        "failureAndCancellation": "Begin/Advance staging retains previous publication on failure; source editing is only a revision-local eligibility hint guarded by owner at action time.",
        "testCoverage": "Support.Tests.RichSemanticDocument, semantic diagrams, image/math and accessibility fixtures; no run here."
      },
      "plannedSha256": "9619b4f38b662c02e68ebaaea6eafb63f0d3dc52aa6c37fb25fed95d0ff31a89",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextCommands.h",
      "sha256": "1770a9b2b492e85ab9558c8adae4e1e4e333b05592cc4bd2f21efdeb3c446620",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Editor routes MarkdownCommand requests to BuildMarkdownCommand/Deletion and ApplyMarkdownCommand.",
        "ownershipAndLifetime": "Request views are borrowed only while planner builds owned immutable edits/source snapshot and expected revision.",
        "failureAndCancellation": "Rejected plan leaves Document intact; Apply validates expected source/revision before one transaction.",
        "testCoverage": "Support.Tests.RichSemanticEditor structural command source/selection/history cases; no run here."
      },
      "plannedSha256": "1770a9b2b492e85ab9558c8adae4e1e4e333b05592cc4bd2f21efdeb3c446620",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextLayout.h",
      "sha256": "8a23fbec041a2d2b0e90dcca19ff77eaff1725697d67868f20683b92f32163f8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout and PreparedSemanticGeometry use PreparedLayout for text geometry and draw; facade handles v0/semantic selection.",
        "ownershipAndLifetime": "Caller supplies borrowed DirectWrite factory/targets/brushes; prepared state owns layouts/maps but no HWND/device/thread.",
        "failureAndCancellation": "Validation and bounded work APIs document failed preparation preserves prior pair; draw borrows transient brushes and restores shared layout state.",
        "testCoverage": "Controls.Tests.RichTextLayout and RichSemanticGeometry cover mapping/layout/geometry; no run here."
      },
      "plannedSha256": "8a23fbec041a2d2b0e90dcca19ff77eaff1725697d67868f20683b92f32163f8",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextPresentation.h",
      "sha256": "d96715b8b40f75bbd31e4d4a00b505cd8e6fac92db12da65b925ef2b948652e0",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry.Admission"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "SemanticSpacing is consumed by semantic geometry/layout options.",
        "ownershipAndLifetime": "Value-only fixed-size arrays/scalar; no resources or lifetime dependencies.",
        "failureAndCancellation": "IsValid rejects non-finite/out-of-policy spacing before geometry preparation.",
        "testCoverage": "Semantic geometry admission tests cover invalid/valid spacing; no dedicated struct test found."
      },
      "plannedSha256": "d96715b8b40f75bbd31e4d4a00b505cd8e6fac92db12da65b925ef2b948652e0",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/TextInputServices.h",
      "sha256": "4c50590f3c523900bfa5d2fb1df737fd79871e67b3d9d03674f79bf8b0a60a1f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "4c50590f3c523900bfa5d2fb1df737fd79871e67b3d9d03674f79bf8b0a60a1f",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/ThemeColors.h",
      "sha256": "b8f43321fb894a39bd16ec10314bbde753eb3658d44baa9b91062cc74c44e95c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "b8f43321fb894a39bd16ec10314bbde753eb3658d44baa9b91062cc74c44e95c",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Typography.h",
      "sha256": "ed7d2f50731fd6756646125c6a7ee04ddc62baa93944959bc902588d4884a4a7",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full header read. MeasureSingleLineTextWidthPx and MeasureWrappedTextHeightPx resolve the shared factory, cached format, DPI, and measurement helpers internally; repository-wide rg found no production call sites beyond these definitions, so external-consumer use remains possible.",
        "ownershipAndLifetime": "Factory/formats and both static caches retain COM ownership through wil::com_ptr; cache lookup/publication is serialized by GetMeasurementCacheMutex, and invalidation is per factory or global. No concrete lifetime defect found.",
        "failureAndCancellation": "Public measurement helpers return zero metrics for missing factory/format, invalid size bounds, DirectWrite layout failure, or metrics failure; no cancellation path exists in this helper.",
        "testCoverage": "No direct tests found for MeasureSingleLineTextWidthPx/MeasureWrappedTextHeightPx. Adjacent coverage includes TestLabelTypographyCharacterSpacingUsesRequestedTotal and TestButtonPreparedIconTextLayoutMatchesDrawTextW; neither proves these exported measurement helpers. No tests executed."
      },
      "plannedSha256": "ed7d2f50731fd6756646125c6a7ee04ddc62baa93944959bc902588d4884a4a7",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "performance-forms.ps1",
      "sha256": "33d9ec1a16e25dde7980898a950b2bf93e37d58305c8b122eaa312b0d156c28e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformanceForms.ps1: public form-layout report schema and runner behavior"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "failureAndCancellation": "Requires matching native architecture and attested executable; rejects numeric/schema/timing/parity inconsistencies and post-run mutation; malformed outputs are retained as invalid evidence.",
        "testCoverage": "Test-PerformanceForms.ps1 exercises public runner schema and invalid evidence cases with executable/build shims; no native rendering, benchmark, or acceptance verdict.",
        "ownershipAndLifetime": "Binds form-specific input hashes, validates fixed profiles, schedules, timing and lifecycle arrays; archives stale reports and publishes parsed immutable native bytes/raw companion after identity checks.",
        "callers": "Developer opt-in Manual/Form layout diagnostic; imports PerformancePolicy and launches build.ps1 unless SkipBuild, then invokes DxUi.EmbeddedTests.exe benchmark-form-layout."
      },
      "plannedSha256": "0201a5910f36d8b69d93965f3601de6037f4b1943d13c44a21514fa0b104e979",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "performance-paired.ps1",
      "sha256": "ae1694ec9de45e1bdb3ca6af9f6f6132cafb93593b9ba2287903b0de22324224",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PairedRun.ps1: overlay immutability, SkipBuild/attestation, matched preflight, and restoration cases"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-PairedRun.ps1 covers selection, source distinction, overlay stability, compiled-input reuse, matched full-profile preflight and controls; no native paired run here.",
        "callers": "Top-level paired driver uses PairedRun selection/overlay and PerformancePolicy identity/attestation, then measures both trees with common harness.",
        "failureAndCancellation": "Refuses identical sources, mismatched full identity, missing/stale attestation, changed compiled overlay under SkipBuild, invalid comparison, and successful study whose restoration fails.",
        "ownershipAndLifetime": "Owns generated worktrees/report directory; named-tree source overlays and project timestamps are backed up and restored in finally; exact receipts and judge provenance retained."
      },
      "plannedSha256": "bd29873180db31b9509a4804c1bf1f5ddeb1939c487f66529d6e7451cfe1c4f0",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "performance-profile.ps1",
      "sha256": "8ad8cf24470a7521739b7c5ad7d56ab9466ed6b66c7cc2e63db4f94493b0eceb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformanceProfiling.ps1: public profiling entrypoint and mode conflicts"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "failureAndCancellation": "Rejects non-native architecture, invalid report/output location, incompatible heap/timer/raster combinations, and build exit failure before measurement.",
        "testCoverage": "Test-PerformanceProfiling.ps1 asserts mode conflicts and output boundaries using scripted fixtures; native workload/visual result not run.",
        "ownershipAndLifetime": "Normalizes exact parameter modes, defaults output under .build/reports, archives prior outputs as stale before replacing; no native child ownership itself.",
        "callers": "Developer opt-in CLI; delegates to PerformanceProfiling.psm1, optionally build.ps1."
      },
      "plannedSha256": "8ad8cf24470a7521739b7c5ad7d56ab9466ed6b66c7cc2e63db4f94493b0eceb",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "performance.ps1",
      "sha256": "52c863186486b235914d5558a982ddbe9c1d72efb29d41c68a474e604b19ab0f",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-PerformanceComparison; Test-PerformanceForms"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Invokes build optionally; launches DxUi.EmbeddedTests.exe with selected benchmark scenario, then Tools/Compare-Performance.ps1.",
        "ownershipAndLifetime": "Requires native matching architecture and valid v2 build attestation pre/post; output and comparison receipts carry exact build identity.",
        "failureAndCancellation": "Removes prior path before run; nonzero/missing receipt/state drift invalidates and relocates produced native receipt; baseline path distinct.",
        "testCoverage": "Tools/tests/Test-PerformanceComparison.ps1 and -Forms cover comparison/parser; no native benchmark run."
      },
      "plannedSha256": "52c863186486b235914d5558a982ddbe9c1d72efb29d41c68a474e604b19ab0f",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/ComplexUi/ComplexUiScene.h",
      "sha256": "425c830c22ec3672f52f6569bebd5fe2ababc45dfc47f530523eeb0f4c3a3041",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 --complex-ui; performance benchmark"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Included by EmbeddedControls Main.cpp; ComplexUiScene is shared deterministic 16-slider/16-progress + Tree/Grid model fixture.",
        "ownershipAndLifetime": "Scene owns EmbeddedHost before borrowed model destruction ordering; controls are owned by root while pointers are non-owning; model synthetic 1,000 rows/four columns.",
        "failureAndCancellation": "Grid text lookup assumes valid row/column indices from control caller; no failure path for invalid indices; fixture is not production data.",
        "testCoverage": "test-consumer.ps1 copies the scene and runs --complex-ui capture; performance.ps1 benchmark uses corresponding library workload; neither run here."
      },
      "plannedSha256": "425c830c22ec3672f52f6569bebd5fe2ababc45dfc47f530523eeb0f4c3a3041",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/DxUi.EmbeddedControls.vcxproj",
      "sha256": "21124b1f5d13204b57f37d10ce3d960306c455713e532ecb13156ce3178f61ec",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix; test-consumer.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Solution builds standalone sample and references one DxUi.vcxproj.",
        "ownershipAndLifetime": "Declares all six profile/platform configurations and supplies no consumer-specific `.cpp` list.",
        "failureAndCancellation": "MSBuild project reference and common configuration validation stop unsupported profile mapping; no compile run.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 checks six config mappings; test-consumer.ps1 separately relocates sample source."
      },
      "plannedSha256": "21124b1f5d13204b57f37d10ce3d960306c455713e532ecb13156ce3178f61ec",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/EmbeddedScene.h",
      "sha256": "e7da318e233c1f6a9a465589494a47b99013374af4d2f5b8b5d1834daf4b3c9d",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 embedded/text-input/rich-text smoke"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Main.cpp initializes this scene for playground, text-input, or rich-text mode; controls call back into scene-owned state.",
        "ownershipAndLifetime": "EmbeddedHost owns tree; model/control pointers are borrowed from root; all sample strings/assets are synthetic.",
        "failureAndCancellation": "Markdown command callback catches named std::exception and reports failure; failed insertion leaves text user-facing message; no device/app services.",
        "testCoverage": "test-consumer.ps1 runs default/text-input/rich-text relocated modes; no sample execution here."
      },
      "plannedSha256": "e7da318e233c1f6a9a465589494a47b99013374af4d2f5b8b5d1834daf4b3c9d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/EmbeddedTextClient.h",
      "sha256": "67f3dbf109a3e72e7be2a7d9bc89f5f9351c4d2e968666f7cd97612c44bf2f73",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 --text-input"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Main.cpp supplies this public TextInputClient/PasteTransform adapter to TextInputServices in headless smoke and live window.",
        "ownershipAndLifetime": "Borrowed EmbeddedHost/HWND disconnected before teardown; identity bound to captured focus id; geometry converts DIP to screen with DPI rounding.",
        "failureAndCancellation": "Disconnected/stale focus return TF_E_DISCONNECTED/S_FALSE; Win32 conversion failures return HRESULT; range conversion rejects nonfinite/out-of-range.",
        "testCoverage": "test-consumer.ps1 text-input mode calls RangeBounds, paste and render; no direct test of disconnect races/DPI extremes in sample smoke."
      },
      "plannedSha256": "67f3dbf109a3e72e7be2a7d9bc89f5f9351c4d2e968666f7cd97612c44bf2f73",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/GraphicsFixture.h",
      "sha256": "b242c5efa16862ccf416cc2c7a277a2e78354273ef43d3cfdf48c69bbef5c19d",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 sample modes; gallery.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Used by Main.cpp for supplied WARP device, target surface, bind/composite readback and PNG generation.",
        "ownershipAndLifetime": "D3D/DXGI/WIC COM objects use wil::com_ptr; read staging texture and mapped resource are local; Unmap guarded with scope_exit.",
        "failureAndCancellation": "HRESULT propagation on device/map/codec failures; `pixels.resize`/filesystem exceptions propagate to main exception boundary; no bounds check for width*height product.",
        "testCoverage": "test-consumer.ps1 headless default/complex/text modes and gallery sample render; no output generated."
      },
      "plannedSha256": "b242c5efa16862ccf416cc2c7a277a2e78354273ef43d3cfdf48c69bbef5c19d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/Main.cpp",
      "sha256": "b41777902b08f640535075637480134c249b29423d71601923f1c83cb07551dc",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 relocated sample smoke; gallery.ps1 embedded example"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Standalone sample entrypoint; message pump dispatches public embedded host/input services; CI and gallery invoke `--output`, external consumer tests invoke several smoke modes.",
        "ownershipAndLifetime": "App owns window/services/image-loader; teardown stops loader/disconnects text then destroys window; ImageLoader callback uses copied state; sample fixture uses synthetic text.",
        "failureAndCancellation": "Main COM initialization is fail-closed on FAILED(CoInitializeEx) and calls CoUninitialize only after successful initialization through scope_exit. Callback catches are named exception types with documented fallback; no catch(...) or COM-init bool fallback was found.",
        "testCoverage": "test-consumer.ps1 sample build/render covers normal consumer/sample integration but no deterministic exception-injection or COM initialization failure seam was identified; these remain test gaps rather than confirmed findings."
      },
      "plannedSha256": "1dce92700688ca4efd6c7744b56a821ba0ebb608d7f9855599239b09cbca84d4",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/RichImageLoader.h",
      "sha256": "533fee661a99338dec13adb775986d413f0a0b62b276e70afa3e550b716f7801",
      "category": "samples",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 --rich-text"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "EmbeddedScene exposes RichTextEditor; Main.cpp initializes loader, submits pending requests, drains results from ImageReady/Render.",
        "ownershipAndLifetime": "One threadpool work item; callback retains shared State, owns WIL threadpool handle, serialized queues/results, explicit 32-job/64MiB bounds; no UI/editor pointer on worker.",
        "failureAndCancellation": "Shutdown marks closing, nulls HWND, cancels queue and releases owner without waiting; worker checks cancellation/closure and no late post; `PostMessageW` failure is ignored, so queued result waits for another render request (sample lacks retry policy).",
        "testCoverage": "test-consumer.ps1 rich-text sample smoke only; no worker saturation, cancellation, HWND reuse, decode error matrix test in external smoke."
      },
      "plannedSha256": "533fee661a99338dec13adb775986d413f0a0b62b276e70afa3e550b716f7801",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/ControlCatalog.cpp",
      "sha256": "d6fb32eb76cff6fcd1befff2303fa0f237654a5c7cd3e0cfd11e80249c7fb29a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "Controls.Tests.Forms",
        "DxUi.Tests.EditorControls"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; no additional confirmed defect",
      "reviewNotes": {
        "callers": "Read all 94 lines. Public callers are the Forms test factory and editor-control catalog tests; each enum kind maps to one static descriptor and one CreateControl switch arm.",
        "ownershipAndLifetime": "Factory returns an owning unique_ptr and constructs directly into the caller output; no retained raw ownership. No defect found.",
        "failureAndCancellation": "CreateControl catches std::bad_alloc at the HRESULT boundary and returns E_OUTOFMEMORY; unknown enum values return E_INVALIDARG. No failure injection performed.",
        "testCoverage": "Existing catalog coverage: Controls.Tests.Forms checks catalog size; DxUi.Tests.EditorControls checks descriptor enumeration and creation. No tests executed."
      },
      "plannedSha256": "d6fb32eb76cff6fcd1befff2303fa0f237654a5c7cd3e0cfd11e80249c7fb29a",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Accessibility.cpp",
      "sha256": "15b50c2aa53eb6151454019afe2b015eaf1cd5853c0e05bd296b0743db47ee0f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "dc265921d1518f8a2fae3d81cfbac2388f58059b9780747332cf62768dc48e3d",
      "reviewEvidence": "review-host-controls-current.md",
      "leadDeltaReview": "Lead reviewed current delta: interested focus refresh reuses existing FindWindowHostControlPath with live pointer/visibility/semantic validation and DFS fallback. Wide-tree 128-mutation test reduces resolution visits 66048 to 1792 and checks stale-path/hidden focus. Final-source full profiles remain open."
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.AccessibilityTextUnits.h",
      "sha256": "27f77b8fb0a2c02c1f447a32d6d5a9fbd85d7cb9ab9c312a19b46781867531d7",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "27f77b8fb0a2c02c1f447a32d6d5a9fbd85d7cb9ab9c312a19b46781867531d7",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.ComboBox.cpp",
      "sha256": "ed9e23a81ee5c88d8302359589ced3c6ffdfb9590c0146e23228968a5f0fbb12",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestComboBoxPopupInsideScrolledPanelUsesViewportCoordinates",
        "TestComboBoxPopupFlipsAboveWhenBelowSpaceIsInsufficient",
        "TestComboBoxPopupScrollbarPagesLongLists",
        "TestComboBoxTextChangedCallbackRetainsMutableStateAcrossNotifications",
        "TestComboBoxSelectionStopsAfterSelectionChangedDestroysComboBox"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; no additional confirmed defect",
      "reviewNotes": {
        "callers": "Read the complete 3,114-line implementation and traced ComboBox composition into TagPicker plus direct ControlHost text-input, popup, selection, and accessibility call paths.",
        "ownershipAndLifetime": "Reviewed popup/selection/text callbacks, reentry and weak-lifetime checks, capture routing, backdrop bitmap/device cache ownership, and one-line layout invalidation. No confirmed lifetime defect in this file.",
        "failureAndCancellation": "DirectWrite/D2D creation failures return local empty/fallback geometry; popup backdrop failures degrade to no backdrop. Clipboard imports normalize control characters; this is source review, not runtime validation.",
        "testCoverage": "Relevant existing cases include TestComboBoxPopupInsideScrolledPanelUsesViewportCoordinates, TestComboBoxPopupFlipsAboveWhenBelowSpaceIsInsufficient, TestComboBoxPopupScrollbarPagesLongLists, TestComboBoxTextChangedCallbackRetainsMutableStateAcrossNotifications, and TestComboBoxSelectionStopsAfterSelectionChangedDestroysComboBox. No tests were executed."
      },
      "plannedSha256": "aff827d2485537f824232fc917b65b3e831b65bae595b0fc796c3cac1bf0b11b",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Controls.cpp",
      "sha256": "debb6a7397f3f7bd81989d91f5bcfe40f063800387eb0777b04942b763fb24f7",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestAccessibilityProviderExposesGridCellToggleAndRangePatterns",
        "TestAccessibilityRangeValueChangesReachSubscribedClient"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; confirmed ProgressBar RangeValue bound-change publication gap; no runtime claim",
      "reviewNotes": {
        "callers": "Reviewed all 11,788 lines plus the Control::RequestInvalidate caller/implementation in DxUi.cpp. ProgressBar SetMinimum/SetMaximum call only RequestInvalidate, which reaches ControlHost::Invalidate but not accessibility publication; SetValue explicitly refreshes the WindowHost snapshot.",
        "ownershipAndLifetime": "Reviewed callback and weak-lifetime paths across container, selection, capture, and tooltip code. No confirmed ownership/lifetime defect in this file; ProgressBar bound setters retain the control and publish through the attached host when present.",
        "failureAndCancellation": "Range setters reject non-finite inputs, then mutate the bound; refresh gap can leave the immutable accessibility snapshot and subscribed RangeValue properties stale until an unrelated refresh. This is source-backed; no test/build executed here.",
        "testCoverage": "Existing TestAccessibilityProviderExposesGridCellToggleAndRangePatterns covers progress-cell RangeValue; TestAccessibilityRangeValueChangesReachSubscribedClient covers ProgressBar value events but its minimum/maximum checks target Slider. No current ProgressBar bound-change regression; patch artifacts under .build/audit-20261024 cover actual-client events and provider getters."
      },
      "plannedSha256": "6ba631060b799191acbdcc47819ff70bc857f39d9b6f93033a89611a8f96e121",
      "reviewEvidence": "review-host-controls-current.md",
      "leadDeltaReview": "Lead reviewed current delta: ProgressBar min/max setters publish accessible changes by the same host refresh path as SetValue. Real subscribed-client regression fails before and passes after; Embedded preparation contract unchanged."
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.cpp",
      "sha256": "90fdfef55ac24fbed1146fdf543ca32c772bd86daecf5e7fdb7099054e67d36a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "Controls.Tests.FormAccessibility"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; no additional confirmed defect",
      "reviewNotes": {
        "callers": "Read all 1,408 lines. Traced Control state setters into ControlHost invalidation/accessibility publication and the form-property implementation into its provider/test helpers.",
        "ownershipAndLifetime": "Reviewed weak-lifetime guards around callbacks and virtual notifications, parent/host propagation, focus synchronization, content-preparation teardown, and form-label weak ownership. No confirmed defect.",
        "failureAndCancellation": "Backdrop capture checks invalid extents, byte-count overflow, and local GDI/D2D acquisition failures with RAII and fallbacks. SetAccessibleFormProperties returns E_OUTOFMEMORY before replacing prior metadata. No runtime failure injection performed.",
        "testCoverage": "Existing form-property ownership/lifetime cases include Tests/Controls/Controls.Tests.FormAccessibility.cpp (e.g. label detach and expired weak label). No tests were executed."
      },
      "plannedSha256": "207017d799dd6a5ca8b51727bb2a741f785d07614f48417da730d0317c858635",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.EditorControls.cpp",
      "sha256": "6cd46dd280760ee487b4f9d95c776adcd3ba01ba092005718b50ba843392a2db",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "44ef22a3bf667fa608e86aa838f212747575f327c0963a82c652b0fa6dd77e00",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.FocusRestore.h",
      "sha256": "ba672d41cdd7c3f289cd0cda7873d00f9782c642de19eaa9b48cca803f68249f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "ba672d41cdd7c3f289cd0cda7873d00f9782c642de19eaa9b48cca803f68249f",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Forms.cpp",
      "sha256": "bd6ce7027bf35f8163a83e3ae8292c723a612ba94db786aa5482534d5ffb409c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
        "TestFormsFractionalTranslationFailsBeforeAnyChildGeometry",
        "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
        "TestFormsMaximumInventoryRetainsCacheAndFiniteGeometry",
        "TestFormsNestedIntrinsicEditorIsAnAcceptedDescendant",
        "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; no additional confirmed defect",
      "reviewNotes": {
        "callers": "Read all 988 lines and traced FormField/FormPanel measure, intrinsic preparation, arrangement, accessibility synchronization, editor detach, and nested-editor paths.",
        "ownershipAndLifetime": "Reviewed retained part/editor pointers with lifetime tokens, callback-driven child retirement/replacement, detach cleanup, and staged bounds commit. No confirmed defect.",
        "failureAndCancellation": "Layout validates finite extents, caps fields at 256, stages before geometry mutation, aborts superseded passes, and catches named bad_alloc at HRESULT boundaries. No runtime failure injection performed.",
        "testCoverage": "Current coverage in Controls.Tests.Forms includes localized geometry, measurement budgets, exact fractional translation, custom-editor callbacks, maximum inventory, nested editors, and embedded reflow. No tests were executed."
      },
      "plannedSha256": "f9fa953e13b31aae558e45e96d3aa6a6f196a372334e859d48018cd79d5ffba2",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Grid.cpp",
      "sha256": "64881d61efff692bca3f70624c36d4fc5e9ace98318294e0afebda7d32c9a657",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestGridSetModelStopsAfterAGroupGetterReplacesItsRoot",
        "TestGridKeyboardCheckboxQueryStopsAfterGetterRetiresTheGrid",
        "TestGridSelectionOfALargeListFollowsGesturesAndDataChanges",
        "TestGridVisibleWorkMetricsStayBoundedForLargeDatasets",
        "TestGridHeaderDragReordersColumnsWithoutTriggeringSort",
        "TestGridRightToLeftMirrorsColumnGeometryAndHitTesting",
        "TestGridMultilineTooltipFollowsPaintedLines",
        "TestGridTextLayoutTableStopsAtItsCeilingEvictsTheLeastRecentlyUsedAndHalves"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full source read (7,859 lines). Traced Grid public selection/layout/query entrypoints into model and delegate callbacks, Paint/Tick, pointer and keyboard handlers, retained text-layout preparation, scrollbar/group navigation, and the corresponding test file.",
        "ownershipAndLifetime": "Grid borrows IGridModel and IGridDelegate; BorrowedControlModelGuard checks control lifetime, binding, and model revision around virtual calls. Retained DirectWrite layouts/formats and bounded text tables are released on host move, hide, and paint-generation eviction; callback paths revalidate before continuing. No source-confirmed defect found in this pass.",
        "failureAndCancellation": "Reviewed named failure handling at noexcept/callback boundaries, staged row-selection buffers, model-query guards, click/gesture revalidation, and layout-cache construction/abandonment. Allocation/model exceptions are contained where required; no build or runtime failure injection was performed.",
        "testCoverage": "Focused current cases include TestGridSetModelStopsAfterAGroupGetterReplacesItsRoot, TestGridKeyboardCheckboxQueryStopsAfterGetterRetiresTheGrid, TestGridSelectionOfALargeListFollowsGesturesAndDataChanges, TestGridVisibleWorkMetricsStayBoundedForLargeDatasets, TestGridHeaderDragReordersColumnsWithoutTriggeringSort, TestGridRightToLeftMirrorsColumnGeometryAndHitTesting, TestGridMultilineTooltipFollowsPaintedLines and TestGridTextLayoutTableStopsAtItsCeilingEvictsTheLeastRecentlyUsedAndHalves. No tests were executed by this reviewer."
      },
      "plannedSha256": "9ac239a10f086afaa86e1998f2aec1c4c0201de463e66a4994cdd091ed8f9b72",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.h",
      "sha256": "f6f32ba08edcd3d0f97b5ae5cda7dab74bb81c894a593cf1e5f11b5083429089",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "f6f32ba08edcd3d0f97b5ae5cda7dab74bb81c894a593cf1e5f11b5083429089",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Internal.h",
      "sha256": "d328ca55e70a315b2b3e8614da91eefc5b158ad6970f0b22d132e9294798f7b7",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestGridSetModelStopsAfterAGroupGetterReplacesItsRoot",
        "TestGridSetModelDetectsSamePointerReentrantReplacement",
        "TestGridTextLayoutTableStopsAtItsCeilingEvictsTheLeastRecentlyUsedAndHalves",
        "TestTreePointerRechecksTheModelAfterFocusCallbacks",
        "TestAccessibilityInterestedMutationsBoundFocusedPathResolution"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read. BorrowedControlModelGuard and TryControlCallback are used by Grid/Tree model query and callback flows; ControlModelQueryAccess is the shared bridge for revision, binding, host, and geometry identity. Retained text-layout helpers serve Grid multiline cell caches; focus and UIA declarations are implemented by WindowHost/Accessibility paths.",
        "ownershipAndLifetime": "Model guards store a weak lifetime token and validate owner/model/binding/host/geometry revisions before accepting values. Layout-table entries retain text layouts with their keys and release unused layouts at paint end. SAFEARARRAY uses a unique_ptr deleter.",
        "failureAndCancellation": "Model callback exceptions are translated to a false result and callers revalidate state; replacement callback allocation failure preserves the previous subscription. Text-layout growth/resize failures are allowed to keep the previous bounded table. No confirmed rollback defect found; no runtime testing by this reviewer.",
        "testCoverage": "Shared contracts have coverage in TestGridSetModelStopsAfterAGroupGetterReplacesItsRoot, TestGridSetModelDetectsSamePointerReentrantReplacement, TestGridTextLayoutTableStopsAtItsCeilingEvictsTheLeastRecentlyUsedAndHalves, TestTreePointerRechecksTheModelAfterFocusCallbacks, and TestAccessibilityInterestedMutationsBoundFocusedPathResolution. No tests executed."
      },
      "plannedSha256": "4b75cc434c199b80dc299f2b64f94d1736abbd2f66904f0b06263b5d550bbda4",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Layout.cpp",
      "sha256": "cb31f8c5f7446213d896d8fe78cc5a132304eba3bbe6477a0226ddc05ca5f148",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs",
        "TestFlexCacheInvalidationAndCompositeBounds",
        "TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry",
        "TestFlexBoundsCallbacksCannotPublishRetiredOrSupersededPasses",
        "TestFlexNestedMeasureCannotReplaceArrangementSnapshots"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full source read. Control::Measure guards intrinsic measurement and preserves prior outputs on failure; FlexPanel::BuildLayout stages layout snapshots before ApplyLayout publishes geometry.",
        "ownershipAndLifetime": "Borrowed model/control callbacks are guarded by lifetime tokens and current-owner checks; composite layout snapshots are published only after the pass remains current.",
        "failureAndCancellation": "Measurement/layout callback failures return failure without replacing committed outputs or prior geometry; reentrant ownership changes are checked between callback boundaries.",
        "testCoverage": "Focused cases include TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs, TestFlexCacheInvalidationAndCompositeBounds, TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry, TestFlexBoundsCallbacksCannotPublishRetiredOrSupersededPasses, and TestFlexNestedMeasureCannotReplaceArrangementSnapshots. No tests executed."
      },
      "plannedSha256": "5a59546af1d55ca6ce9838cb3558c90183b38004380448ce95fb85f28f56c893",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownBlocks.h",
      "sha256": "3c9c206a2acd05358cc281a26e75dbc5ce2d3b3fc743192d72b79ac8fcc6c5ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Block dispatch recognizes bounded block starts and advances source intervals monotonically.",
        "ownershipAndLifetime": "Nested block parsing uses explicit stacks and capped node/depth counts.",
        "failureAndCancellation": "Unsupported constructs preserve source slices and diagnostics rather than publishing partial semantic output.",
        "testCoverage": "Support.Tests.MarkdownBlocks covers fenced blocks, lists, tables and corpus extraction. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "3c9c206a2acd05358cc281a26e75dbc5ce2d3b3fc743192d72b79ac8fcc6c5ad",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownCharacterReference.h",
      "sha256": "4fb5f5eac34311d605f9916f2889f13f26a5dae00b02799b7813eae1c240f265",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownCharacterReference.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownCharacterReference.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "4fb5f5eac34311d605f9916f2889f13f26a5dae00b02799b7813eae1c240f265",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownEntityData.h",
      "sha256": "fe27d6f6fd7bdb89dd252729524aa835a0ea798087e409a81bcaa4e255ecaf33",
      "category": "production",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Generated entity table is immutable lookup data used by character-reference decoding.",
        "ownershipAndLifetime": "Lookup returns spans into static storage without per-call allocation or ownership transfer.",
        "failureAndCancellation": "Parser validates complete names and scalar values before publication.",
        "testCoverage": "Support.Tests.MarkdownBlocks includes named/numeric references and invalid forms. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "fe27d6f6fd7bdb89dd252729524aa835a0ea798087e409a81bcaa4e255ecaf33",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownInlines.h",
      "sha256": "62cce330ebfdb9e65bbd337ca5a521854c8b73b4111911669a194492151c0487",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Inline parser tracks source spans for emphasis/code/link/image constructs.",
        "ownershipAndLifetime": "Inline nodes own decoded text and preserve source correlation for untouched syntax.",
        "failureAndCancellation": "Depth, source and node limits fail closed with exact source retention.",
        "testCoverage": "Support.Tests.MarkdownInlines covers nested delimiters, entities, URLs and malformed syntax. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "4d6fe2886398af9b6021769544bbb0c3633910f8b9ac92a563622187d96f9c67",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownLabels.h",
      "sha256": "13031872578ddb54e53efe212a337baefeb195074f23b6e783b45735c6f2682c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownLabels.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownLabels.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "13031872578ddb54e53efe212a337baefeb195074f23b6e783b45735c6f2682c",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSourceCorrelation.h",
      "sha256": "a7af9b55d38cb7691fb4ce2469b0d2a3c8b00d4f7c8a667b21f4c1ab3c3f4b1c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownSourceCorrelation.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownSourceCorrelation.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "a7af9b55d38cb7691fb4ce2469b0d2a3c8b00d4f7c8a667b21f4c1ab3c3f4b1c",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSyntax.cpp",
      "sha256": "735e314ec52cd8b2d44c55c704254c387f822a2bb259c57ac845fb0a5074414a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Parse entry validates options and stages the request before replacing published syntax.",
        "ownershipAndLifetime": "Incremental Advance resumes bounded parsing and retains the prior committed tree on failure.",
        "failureAndCancellation": "Revision and source intervals propagate across block and inline parsing.",
        "testCoverage": "Support.Tests.MarkdownBlocks and MarkdownCapacity cover parser output and budgets. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "735e314ec52cd8b2d44c55c704254c387f822a2bb259c57ac845fb0a5074414a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownTableCells.h",
      "sha256": "f548d2132cf17c931d8a894fc9a6354d39e6ff17cd9291bec8ea8c8d929812da",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Cell splitting respects escaped pipes and normalizes syntax padding.",
        "ownershipAndLifetime": "Cells are returned through bounded spans without allocating.",
        "failureAndCancellation": "Invalid row forms fail as a whole instead of publishing incomplete cells.",
        "testCoverage": "Support.Tests.MarkdownBlocks includes empty-cell and table-row cases. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "f548d2132cf17c931d8a894fc9a6354d39e6ff17cd9291bec8ea8c8d929812da",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownUnicodeData.h",
      "sha256": "1e49ee2858300241526b6a257b44aa0c45d90b6ed35c43c900403aa449bca262",
      "category": "production",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Generated Unicode property data is static and queried through bounded searches.",
        "ownershipAndLifetime": "No ownership or dynamic initialization is introduced by lookups.",
        "failureAndCancellation": "Callers validate scalar ranges before querying grapheme properties.",
        "testCoverage": "Support.Tests.MarkdownInlines and RichTextLayout exercise Unicode behavior. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "1e49ee2858300241526b6a257b44aa0c45d90b6ed35c43c900403aa449bca262",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Menu.cpp",
      "sha256": "9f731027bb7bb884dab21c69267ca59a1bf00d3db0730833959882282844fec1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestMenuDebugStateProbeBoundsWedgedWindowThread",
        "TestMenuKeyboardNavigationSkipsInfoRows",
        "TestMenuModalLoopPeeksOnlyLivePopupsWhenAPeekClosesASubmenu",
        "TestMenuHoveringSiblingClosesOpenSubmenuAfterDelay",
        "TestMenuPopupRelayoutsOnDpiChanged",
        "TestContextMenuShowAsyncKeepsOwnerPaintableWhileOpen",
        "TestMenuNativeFocusSelectsNoRowAndRestoresTheChosenOne",
        "TestMenuPointerCursorIsArrowOverPopupsAndTheWindowsOwnOutside"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full source read (6,853 lines). Traced ContextMenu::Show/ShowAsync, CreateMenuPopupWindow, modal and async dispatch loops, popup WndProc, pointer/keyboard routing, submenu timers, DPI reflow, UIA invoke/focus posts, rendering and debug probes.",
        "ownershipAndLifetime": "MenuController owns popup chain and root item copies; each MenuPopup owns its copied rows, ControlHost, layouts, backdrop snapshot, and device-bound resources. DestroyMenuPopupWindow marks controller-initiated teardown around synchronous DestroyWindow reentry, then detaches the host. Posted UIA/debug requests use the bounded payload registry drained on WM_NCDESTROY; cross-thread debug dispatch is shared state with bounded waits and abandonment. No source-confirmed defect found in this pass.",
        "failureAndCancellation": "Reviewed popup-creation/reflow failure paths, failed description preparation fallback/rejection, async finalization idempotence, process-exit shutdown, capture/focus restoration, timer cancellation and posted-payload drain. No build or runtime failure injection was performed.",
        "testCoverage": "Current menu coverage includes TestMenuDebugStateProbeBoundsWedgedWindowThread, TestMenuKeyboardNavigationSkipsInfoRows, TestMenuModalLoopPeeksOnlyLivePopupsWhenAPeekClosesASubmenu, TestMenuHoveringSiblingClosesOpenSubmenuAfterDelay, TestMenuPopupRelayoutsOnDpiChanged, TestContextMenuShowAsyncKeepsOwnerPaintableWhileOpen, TestMenuNativeFocusSelectsNoRowAndRestoresTheChosenOne and TestMenuPointerCursorIsArrowOverPopupsAndTheWindowsOwnOutside. No tests were executed by this reviewer."
      },
      "plannedSha256": "dc274149227c2a842429ea49e337f93b8421864e0a4abbad92e5ea9077699324",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.cpp",
      "sha256": "dfe55c6745aeb5a7924f09a392dd2af994d12f624d961e329fa89a9ff2cbd433",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "CheckOriginalCorpus parses per-example outcomes; CheckFamilyGrammar covers family isolation and diagnostics.",
        "ownershipAndLifetime": "Parser preserves node/edge source intervals and publishes only a complete model.",
        "failureAndCancellation": "CheckFanout covers stable Cartesian expansion and atomic caps.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      },
      "plannedSha256": "d425140cf2409ecfb1f82c4950840b2faaa7e3dfb692ca3ff1828188df5cea90",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.h",
      "sha256": "2358fe2921aada2cdd0f6af0c11bc8044b86c32d14faf6584e42bce77c6f74af",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "Model carries family, direction, shape and typed relation.",
        "ownershipAndLifetime": "Diagnostic records outcome, issue category and source interval.",
        "failureAndCancellation": "Model vectors are bounded and parser publication is all-or-nothing.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      },
      "plannedSha256": "8cdf3e1471050b088490273d7357eda242c794456083d3b72f9912c9d8bf62a8",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.cpp",
      "sha256": "d0955bda79392340748bf7abeb22d989c1ac9db141b8a3e38597ff5d166aca66",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Corpus lays out every admitted original graph.",
        "ownershipAndLifetime": "Geometry checks route, ports and extents.",
        "failureAndCancellation": "Fanout checks finite paths and endpoint ports.",
        "testCoverage": "Stadium sizing and ports use capsule geometry; global lane width remains shared. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedSha256": "2ff4ecadf2ad418ebe044ea5d8146a0e6c9f6aea6a1f7d23a3ce9ff72d76c974",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.h",
      "sha256": "1ee37f869cf3935677b26a09638cf7347cf1f5b80740ae51fc5a69638c92575b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Layout accepts bounded parsed graphs.",
        "ownershipAndLifetime": "Plan stores bounded node and edge placements.",
        "failureAndCancellation": "Shape and relation types survive through placed geometry.",
        "testCoverage": "Maximum extents and work steps are explicit. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedSha256": "6ca5af87c500447bb6c4fc65b848f5d20cee889a2dcbc067b22b83e9aa642387",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeMath.cpp",
      "sha256": "eae4ea21cb0c020f60d641f46d08478f342cef0a34bfd4f081de9ce1059cd6a9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Native math parser is reached through semantic native extension dispatch and prepared-math construction.",
        "ownershipAndLifetime": "Model owns parsed TeX source/token data and prepared output; no raw COM ownership.",
        "failureAndCancellation": "Unsupported syntax and explicit caps produce failure while the caller retains exact fence source.",
        "testCoverage": "Support.Tests.MathCorpus contains normalized parser expectations and adversarial limits. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "0d80b6b671de3461643c92fb62f95f83580084ece4ad25d2f8371c4c0b2ffa27",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeTextInput.cpp",
      "sha256": "7b25db8c288b1d0e8450613f791d3f2e6a6861a3c261268e77b2ef6ff228141f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Related registered cases identified: TestNativeImeResultCallbacksPreserveNewerTextAndComposition; TestNativeImeCancellationCallbacksPreserveNewerState; TestNativeImeCommitOwnsOneUndoUnitAndLateResultPreservesApplicationText. No test execution."
      },
      "plannedSha256": "ea0cdd57636994fb8359783c8e79b07c342c65febef39c2f5c08f28c055be608",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PointerInput.cpp",
      "sha256": "2520cf55044fc8da14071054011d252bc9a83a0397797226ec23afeed7f69c03",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "2520cf55044fc8da14071054011d252bc9a83a0397797226ec23afeed7f69c03",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PointerInput.h",
      "sha256": "d9cc5f1ed8c7cc8e61fe9bed609ebffb2c8d9bb6e7d8539da4577a3807f82824",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "d9cc5f1ed8c7cc8e61fe9bed609ebffb2c8d9bb6e7d8539da4577a3807f82824",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.cpp",
      "sha256": "dad35e17dd95b0c29f3f2abbcfaa928e1acd42f868ee7e08a257e1d14bc1e26d",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Relations exercises typed inheritance paint.",
        "ownershipAndLifetime": "StadiumRaster checks capsule rendering against a nonrectangular oracle.",
        "failureAndCancellation": "Labels exercises measured text geometry.",
        "testCoverage": "Incremental preparation publishes after layout and paths are complete. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedSha256": "399741ef95f6c9911be6a9afd39627304c3e0f1628cce2b6684987594570f0a4",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.h",
      "sha256": "b9469918cdb2068d4cdb65f6d4ed0308eea258456360b2b072732d508d6f44fb",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Prepared renderer validates model limits and resource epoch.",
        "ownershipAndLifetime": "Geometry/readiness are exposed as immutable prepared state.",
        "failureAndCancellation": "Drawing keeps relation arrow and node shape paths distinct.",
        "testCoverage": "Rendering uses native paths without runtime dependency loading. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedSha256": "b9469918cdb2068d4cdb65f6d4ed0308eea258456360b2b072732d508d6f44fb",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedMath.h",
      "sha256": "20035b7ce1c0bb7da3ecaeb3261177d3cb2d949c68e399dffcabf4b118d06084",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "PreparedMath stores immutable layout/raster data and resource epoch.",
        "ownershipAndLifetime": "Preparation is bounded and publishes complete output only after measurement/render succeeds.",
        "failureAndCancellation": "Resource recreation invalidates stale prepared output without losing source fallback.",
        "testCoverage": "Support.Tests.MathCorpus and Embedded.Tests.RichTextResources exercise preparation/rendering. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "61ceb37a016d2f709f5371412d03016f34c951f4b9ba5bee6f80992488d19329",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibility.h",
      "sha256": "5ec593951306aab44bfd529af2e3e3928965354f64af271e0c3b019662235aea",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility semantic provider properties/navigation/actions"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "UIA providers query guarded snapshots and dispatch mutations to the window thread; embedded providers enforce their own thread contract.",
        "ownershipAndLifetime": "Provider retains target ref, path/identity and weak document identity; every call revalidates snapshot, binding, semantic revision and element bounds.",
        "failureAndCancellation": "Stale/wrong-role records return UIA unavailable/unsupported; Toggle/Invoke translate bad_alloc and std::exception at noexcept COM boundaries.",
        "testCoverage": "Controls.Tests.RichTextAccessibility semantic-provider fixtures cover properties, navigation, actions and embedded/client providers; not run here."
      },
      "plannedSha256": "5ec593951306aab44bfd529af2e3e3928965354f64af271e0c3b019662235aea",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibilityEpoch.h",
      "sha256": "65b95f8d202c883d755aadd018f81467912740a0f89213635aaabc4364d620e6",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility stale-provider lifecycle"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument and host accessibility snapshots use the epoch to invalidate stale providers.",
        "ownershipAndLifetime": "Data-only object owns one atomic revision, no UI, HWND, host or SDK reference.",
        "failureAndCancellation": "Revision invalidates provider identity without dereferencing the editor from foreign UIA calls.",
        "testCoverage": "Controls.Tests.RichTextAccessibility lifecycle fixtures; no standalone epoch wraparound case found; not run here."
      },
      "plannedSha256": "65b95f8d202c883d755aadd018f81467912740a0f89213635aaabc4364d620e6",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocument.cpp",
      "sha256": "c016485231a4bc39f53415159fb8a3a8e6e80488738c1910ffdbd88c88e9b271",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.RichDocument.cpp in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.RichDocument.cpp to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "c016485231a4bc39f53415159fb8a3a8e6e80488738c1910ffdbd88c88e9b271",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocumentLayout.cpp",
      "sha256": "f03cd7aade94d6a8d88725dad7158f14335ff65e6ff83dea22069dd87f58c5d4",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.RichDocumentLayout.cpp in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.RichDocumentLayout.cpp to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedSha256": "f03cd7aade94d6a8d88725dad7158f14335ff65e6ff83dea22069dd87f58c5d4",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichEditorLayout.h",
      "sha256": "fe83b32163b16431015a56ba7849b280f67c07291271fbc6ad3cb33ca4c96937",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout.RunRichTextLayoutTests",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichTextEditor begins/advances a facade for legacy v0 or prepared semantic geometry.",
        "ownershipAndLifetime": "Facade switches after target prep completes; it cancels other staging path and publishes generation changes at completion.",
        "failureAndCancellation": "Queries dispatch to active layout; Clear retires both. Caller owns exception recovery.",
        "testCoverage": "Controls.Tests.RichTextLayout and semantic geometry fixtures cover projection/layout handoff; not run here."
      },
      "plannedSha256": "fe83b32163b16431015a56ba7849b280f67c07291271fbc6ad3cb33ca4c96937",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.cpp",
      "sha256": "84047cf4d3f8765f03d7564561405082d751bd4b7387a8653adfc4fdec60244d",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage.Files",
        "Support.Tests.RichImage.Core",
        "Embedded.Tests.RichImageFailures"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImage LoadFile/Decode/CreatePbgra are invoked by image service from RichImageResources requests.",
        "ownershipAndLifetime": "Decode owns bytes while lazy WIC sources live, validates format/size and stages pixels before assigning output.",
        "failureAndCancellation": "Cancellation polled through file/WIC/pixel loops; HRESULT failures preserve output; exceptions translate at noexcept APIs.",
        "testCoverage": "Support.Tests.RichImage.Files/Core and embedded image failure/capture fixtures cover decode/orientation/failures; not run here."
      },
      "plannedSha256": "84047cf4d3f8765f03d7564561405082d751bd4b7387a8653adfc4fdec60244d",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.h",
      "sha256": "34d27813d652032e61f48accb25719fbef6369943513d8a9bd07a4fb2274ebcd",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage",
        "Support.Tests.RichImageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImageResources and image service share request state and diagnostic file-call seam.",
        "ownershipAndLifetime": "Requests own path, identity, kind and atomic cancellation; test injection is diagnostic-build only.",
        "failureAndCancellation": "Cancellation token is read-only to workers; request state follows shared ownership.",
        "testCoverage": "Support.Tests.RichImage and Support.Tests.RichImageResources cover request/file lifetimes; not run here."
      },
      "plannedSha256": "34d27813d652032e61f48accb25719fbef6369943513d8a9bd07a4fb2274ebcd",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImagePaths.h",
      "sha256": "864e905b5674f26b6d3da143117312eadb0524a278a1a25b67832f7b0e5bf5e1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImagePaths.Paths",
        "Support.Tests.RichImagePaths.FileReferences",
        "Support.Tests.RichImagePaths.Retained"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImageResources resolves destinations against document base; paste uses BuildMarkdownFileReference.",
        "ownershipAndLifetime": "Lexical parser is bounded and owns normalized text/component spans; no filesystem lookup or network access.",
        "failureAndCancellation": "Local resolver rejects ambiguous roots/scheme encodings and invalid UTF-16; remote URI handling is lexical.",
        "testCoverage": "Support.Tests.RichImagePaths.Paths/FileReferences/Retained plus codec path fixtures; not run here."
      },
      "plannedSha256": "864e905b5674f26b6d3da143117312eadb0524a278a1a25b67832f7b0e5bf5e1",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImageResources.h",
      "sha256": "a658daf91c20acff83cf3c44ca89762fb1467b408384c438321b46adfaa5e490",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "RichTextEditor routes prepared semantic image elements to owner-thread RichImageResources; application service owns asynchronous worker jobs.",
        "ownershipAndLifetime": "Resource entries hold shared image/request state; cancellation is atomic and callbacks/workers are explicitly caller-owned.",
        "failureAndCancellation": "Begin/Advance stage bounded source and placement sets; Complete only admits dispatched current requests and enforces decoded-byte caps.",
        "testCoverage": "Support.Tests.RichImageResources and Controls.Tests.RichTextInput cover deduplication, path policy, cancellation, queue overflow and failed fallback. Test attribution only; this agent did not run it."
      },
      "plannedSha256": "a658daf91c20acff83cf3c44ca89762fb1467b408384c438321b46adfaa5e490",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichMarkdownProjection.cpp",
      "sha256": "15df5d3c3ecdd6faf431cd2a54fede054a6fe643cc09c9c090cc8a63b0900a97",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown.RunRichMarkdownTests",
        "Controls.Tests.RichDocumentLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout consumes projection blocks as v0 compatibility; RichEditorLayout chooses v0 when semantic prep absent.",
        "ownershipAndLifetime": "PreparedProjection stages changed physical-line blocks, reuses immutable neighbors and publishes complete rebuilt mappings.",
        "failureAndCancellation": "Mapping is range-checked; coverage mismatch throws before state swap.",
        "testCoverage": "Controls.Tests.RichMarkdown and rich-document layout fixtures; not run here."
      },
      "plannedSha256": "15df5d3c3ecdd6faf431cd2a54fede054a6fe643cc09c9c090cc8a63b0900a97",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemantic.cpp",
      "sha256": "b90e450504c76b5d12808b46d796419ee4dd747817b42e70d3d946a75a5cd209",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument builds SourceDisplayMap; ApplySourceEdits applies source-owned changes.",
        "ownershipAndLifetime": "Map owns source/display/spans and affinity anchors; primary source ownership is checked for overlap.",
        "failureAndCancellation": "Invalid ranges/affinity/spans/UTF-16/transactions fail before publication; edits use Document::Replace history.",
        "testCoverage": "Support.Tests.RichSemanticDocument/source-change fixtures cover maps and transactions; not run here."
      },
      "plannedSha256": "b90e450504c76b5d12808b46d796419ee4dd747817b42e70d3d946a75a5cd209",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticDocument.cpp",
      "sha256": "583ca51586e3e84d7508d43fb157f262efcf95f83a5c7d50e3a359914f1631b3",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Document builder consumes immutable RichSemantic input and publishes staged document state.",
        "ownershipAndLifetime": "Source identity/revision and object sidecars are owned through document storage.",
        "failureAndCancellation": "Native preparation errors propagate as HRESULT/status so fallback retains source.",
        "testCoverage": "Support.Tests.RichSemanticDocument and SemanticSourceBlock cover projection/failure paths. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "da13ec1895dd296f658c96d6dbf467e80b4ccb6f983d49451c4c39006fedaf0c",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.cpp",
      "sha256": "0d161cdc926b0d2ab6c2f0eae52c7d1645f57fa01aea53562f93b5ecde1e7530",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Geometry preparation traverses semantic nodes and native prepared object references.",
        "ownershipAndLifetime": "Plans keep source intervals, bounds and object handles scoped to the document revision.",
        "failureAndCancellation": "Invalid geometry and allocation failures leave publication staged; caller owns fallback policy.",
        "testCoverage": "Support.Tests.RichSemanticGeometry and PreparedFlowchart cover layout and failure oracles. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "7843829edfa306f1947c3eb8aeecc4c27de65877295d3046351f1e22e1457998",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.h",
      "sha256": "63ffba706d7b833cd7dd5874dbc5ac5bbc7935c595c842e481073b6869df120b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Geometry API separates immutable plan data from preparation progress.",
        "ownershipAndLifetime": "Placed objects and source ranges are revision-local.",
        "failureAndCancellation": "Budgets and HRESULT-bearing preparation states keep failures explicit.",
        "testCoverage": "Support.Tests.RichSemanticGeometry and SemanticSourceBlock exercise consuming contracts. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "5ed3b1fe7b6ed24ee1e588519f53c2196eec9f62f42ebd7572439ae6e0ec4757",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticHtml.h",
      "sha256": "2c8417f26eca42b5cc0eac43d89848eaccdc3b6ae37f3f756892ad4936f1c80e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticSourceBlock",
        "Support.Tests.RichSemanticDocument HTML fixtures"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument calls NativeHtmlPlan while normalizing inline and block leaves.",
        "ownershipAndLifetime": "Tag vectors, nesting stacks, source offsets and title/destination values stage per leaf; admission is bounded.",
        "failureAndCancellation": "Unsupported/crossed/unclosed tags or overlapping anchors remain literal; no general HTML recovery/execution.",
        "testCoverage": "Support.Tests.SemanticSourceBlock and semantic-document HTML fixtures; not run here."
      },
      "plannedSha256": "2c8417f26eca42b5cc0eac43d89848eaccdc3b6ae37f3f756892ad4936f1c80e",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticValues.h",
      "sha256": "3031f1706c7c011ccf049794586cc94efe05f666dd1aed33726d2619102a6861",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument entity and code ownership"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument resolves inline nodes through SemanticValue before emitting display/source spans.",
        "ownershipAndLifetime": "Units own source ranges and generated/transformed flags; entities decode only as complete lexical tokens.",
        "failureAndCancellation": "Mismatch throws instead of guessing source coordinates; output remains staged.",
        "testCoverage": "Support.Tests.RichSemanticDocument and inline/model corpus entity/code fixtures; not run here."
      },
      "plannedSha256": "3031f1706c7c011ccf049794586cc94efe05f666dd1aed33726d2619102a6861",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSourceInk.h",
      "sha256": "829529fc34f2c1e4a565a398b85ec3b7154caa2585c170066af329e9cf0a9343",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines; embedded source rendering case name pending"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichTextEditor advances RichSourceInk for source coloring and passes completed ranges to PreparedLayout.",
        "ownershipAndLifetime": "256-step turn budget advances node/inline scans, merge order and event publication; incomplete ranges hidden.",
        "failureAndCancellation": "Overflow gives ordinary source; exceptions clear partial state and propagate.",
        "testCoverage": "Markdown inline and embedded rendering fixtures cover coloring/progress; exact focused case names need test-file review."
      },
      "plannedSha256": "829529fc34f2c1e4a565a398b85ec3b7154caa2585c170066af329e9cf0a9343",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextCommands.cpp",
      "sha256": "6c26a98f0b5187969d3e5a0ae74f0cbde2ba349868a9e3bc5467aec3fc3bb227",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Command dispatch resolves selections and source units before structural mutation.",
        "ownershipAndLifetime": "Undo transaction and source snapshots are scoped to the owning editor/document.",
        "failureAndCancellation": "Unsupported partial structural cases reject before publication; allocation boundary maps named failures.",
        "testCoverage": "Support.Tests.RichSemanticEditor and Controls.Tests.RichSemantic cover source/selection/undo expectations. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "1378a6d337601e32bcbe3ce2c5367340d2be01c9ed7964ef1e63db7ee1c28bc7",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextEditor.cpp",
      "sha256": "8aeb1af2fd4a675acdde047085f0b4a049e7f2ae819efdc04035f8262f8156c2",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Editor commands route into RichTextCommands and then publish document/history changes.",
        "ownershipAndLifetime": "Editor state owns selection, undo stack and document revision; callbacks stay UI-thread scoped.",
        "failureAndCancellation": "Failed transactions preserve prior selection/source and do not emit success notifications.",
        "testCoverage": "Controls.Tests.RichTextEditor and RichSemanticEditor cover key paths. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "3dc84b9428521e04b37e55ccc29a3333bfb47a3830ac9bd9fc43a670352e4508",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextLayout.cpp",
      "sha256": "886be4434a3b72dc8c3338fb6be08e6a8589bc76261bb1534a55fb72f0ffac51",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "PreparedLayout serves PreparedDocumentLayout and PreparedSemanticGeometry for DirectWrite geometry, painting and hit tests.",
        "ownershipAndLifetime": "Layout stages COM layout, inline objects, clusters, lines and metrics; source/display mappings remain public coordinates.",
        "failureAndCancellation": "Input/range checks precede publication; transient shared-layout drawing state restored by scope_exit.",
        "testCoverage": "Controls.Tests.RichTextLayout and semantic geometry/raster fixtures; not run here."
      },
      "plannedSha256": "886be4434a3b72dc8c3338fb6be08e6a8589bc76261bb1534a55fb72f0ffac51",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Scrollbar.cpp",
      "sha256": "f70bd75d85454b5dc307ef7388550aa4f19b7a8db11503a38c1db45998d49776",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "f70bd75d85454b5dc307ef7388550aa4f19b7a8db11503a38c1db45998d49776",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.SemanticExtensions.h",
      "sha256": "1c44749aece263be944fb61519c8b1f9fe2928b19bf272d149a11402bee2b1e8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticDiagrams"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Extension dispatch is family-scoped and returns typed native object variants.",
        "ownershipAndLifetime": "Prepared objects own derived model/layout and are immutable after publication.",
        "failureAndCancellation": "Unknown or invalid fences fail closed so document builder can retain exact source.",
        "testCoverage": "Support.Tests.SemanticDiagrams, Support.Tests.NativeFlowchartCorpus and Support.Tests.MathCorpus. Test coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "4b620f5bee8add648004b00824f878ccd884685a2fbf18f8b2c5ca31ee0a7ce6",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.SingleLineTextEditing.cpp",
      "sha256": "6e24615e3149d6487d5ab6f215310dbd60c39dda4a339e89eaa3c5de02bab03b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestTextFieldClickPlacesCaretNearPointer",
        "TestTextFieldCtrlBackspaceDeletesPreviousWord",
        "TestTextFieldEmojiZwJBackspaceDeletesWholeTextElement",
        "TestTextFieldEmojiShiftArrowSelectionExpandsByWholeTextElement",
        "TestTextFieldLongSelectionPaintStaysInsideTextViewport",
        "TestTextFieldBidiSelectionPaintUsesDirectWriteRangeBounds"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read. TextField and editable ComboBox route layout measurement, drawing, caret hit-testing, and selection rectangles through these helpers; cache clearing is also shared with secure-text handling.",
        "ownershipAndLifetime": "DirectWrite layouts are retained with wil::com_ptr. SingleLineTextLayoutCache owns a copied exact string and resets text format identity and dimensions on invalidation; secureCacheText wipes cached text before clearing/replacing.",
        "failureAndCancellation": "Layout creation, SetMaxWidth/SetMaxHeight, hit-testing, and selection-range failure return empty/fallback geometry; no cancellation token is used. Grapheme/word stepping bounds caret positions to text and UTF-16 codepoint helpers.",
        "testCoverage": "Relevant cases include TestTextFieldClickPlacesCaretNearPointer, TestTextFieldCtrlBackspaceDeletesPreviousWord, TestTextFieldEmojiZwJBackspaceDeletesWholeTextElement, TestTextFieldEmojiShiftArrowSelectionExpandsByWholeTextElement, TestTextFieldLongSelectionPaintStaysInsideTextViewport, and TestTextFieldBidiSelectionPaintUsesDirectWriteRangeBounds. No tests executed."
      },
      "plannedSha256": "6e24615e3149d6487d5ab6f215310dbd60c39dda4a339e89eaa3c5de02bab03b",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElements.cpp",
      "sha256": "870cb07abc38b13c94b8c0982df664936d282a0c6c23b801ca719c0c8ee629cc",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "870cb07abc38b13c94b8c0982df664936d282a0c6c23b801ca719c0c8ee629cc",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElements.h",
      "sha256": "ca82bec5f26ef487f1c3e344ebf75ec9f0b6ae2c2d238bdd9a30cc4a90802153",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "ca82bec5f26ef487f1c3e344ebf75ec9f0b6ae2c2d238bdd9a30cc4a90802153",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElementsData.h",
      "sha256": "634788ce7e2fae26aea98501c6ad932a65dac6b9474bf8aa599434d51ca5af70",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Unicode scalar classification is consumed by RichText grapheme navigation/deletion helpers, which binary-search the generated ranges.",
        "ownershipAndLifetime": "Generated immutable constexpr data has pinned Unicode 17.0.0 version and generator/license provenance; no runtime ownership.",
        "failureAndCancellation": "Not applicable: static generated table; malformed UTF-16 handling is in the consumer helpers.",
        "testCoverage": "Controls.Tests.RichTextLayout and Controls.Tests.RichDocumentLayout exercise grapheme boundaries, combining sequences and supplementary scalars; no tests run here."
      },
      "plannedSha256": "634788ce7e2fae26aea98501c6ad932a65dac6b9474bf8aa599434d51ca5af70",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextInput.cpp",
      "sha256": "d2af894eae36a916ce726fa54d729a60f0597cdc4eef57db03e7fadf8b5d1a24",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Related registered cases identified: TestNativeTextInputTextStoreEditsALongDocument; TestNativeTextInputBackendImeWindowsUpdateAfterMultilineScroll. No combined multiline TSF scroll-plus-undo case found; no test execution."
      },
      "plannedSha256": "884ff05dfc89aa1893e40488f9d9bd218adef5653eba55328dcd5191ac1a3913",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextStoreACP.cpp",
      "sha256": "b072969d6ff48846b78a163846b3b00d96386b404f07ea83b4ad59a20abc9c22",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Related registered cases identified: TestNativeTextStoreSequentialCompositionsInOneLockHaveSeparateUndoUnits; TestNativeTextInputTextStoreEditsALongDocument. No focused post-mutation rollback case found; no test execution."
      },
      "plannedSha256": "5e9afcc4c26bbe451051545cac74289edf51b89ecd44e424ca1340ea3b14c251",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Theme.cpp",
      "sha256": "16a15e496c13c21db79d68d3b99c6a7d89c669e517ed096796a1cc1a906e26b1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestChooseContrastingTextColorUsesMeasuredContrast",
        "TestThemeColorsPaletteFallsBackForUnsuppliedAlertColors",
        "TestThemeColorsPaletteKeepsDistinctReadableAlertTones",
        "TestPrimaryButtonContrastPreservesSystemColorPairs",
        "TestThemeColorsAccentPressedUsesBaseThemePolarity"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full source read. ThemePalette construction flows through MakeDefaultThemePalette and MakeThemePalette; control style consumers call focused Resolve*VisualStyle functions for buttons, toggles, fields, grid/tree surfaces, badges, alerts, and progress.",
        "ownershipAndLifetime": "Pure value transformations over ThemePalette and D2D1_COLOR_F; no retained resources or callback ownership in this file.",
        "failureAndCancellation": "All entry points are noexcept value transforms; there is no cancellation behavior. Alpha/channel interpolation is clamped where the helpers define unit intervals.",
        "testCoverage": "Direct color/theme coverage includes TestChooseContrastingTextColorUsesMeasuredContrast, TestThemeColorsPaletteFallsBackForUnsuppliedAlertColors, TestThemeColorsPaletteKeepsDistinctReadableAlertTones, TestPrimaryButtonContrastPreservesSystemColorPairs, and TestThemeColorsAccentPressedUsesBaseThemePolarity. No tests executed."
      },
      "plannedSha256": "8dee2d2373282a11cfc629fd758d7cc82c76245a379d7f92b8c3008af45fcaa5",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Tree.cpp",
      "sha256": "a56cd0a896a1df858d317a59359cdba73cdb0c0635d6527049c7c5595df5e223",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestTreePointerRechecksTheModelAfterFocusCallbacks",
        "TestTreeSelectionCallbacksFireOncePerChangeAndSeeTheFinishedSelection",
        "TestTreeSelectAllStopsWhenSelectionCallbackReplacesItsDelegate",
        "TestTreeSelectionSetCallbackMayDestroyTheTree",
        "TestTreeSingleClickCaptureTransferMayRetireTheTree",
        "TestTreeGesturesContinueWhenACallbackChangesAnotherControl"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full source read. Tree input, paint, selection, expansion, context-menu, reorder, typeahead, tooltip, and accessibility paths query ITreeModel and notify ITreeDelegate; GetItemLayoutMetrics is shared by painting, hit-testing, tooltip, and exposed item geometry.",
        "ownershipAndLifetime": "ITreeModel is non-owning; TreeModelQueryGuard checks weak control lifetime plus model/binding/host/geometry revisions around borrowed reads. UIA/delegate calls are followed by lifetime and binding revalidation where retained state is used. Animation/cache data is owned by Tree.",
        "failureAndCancellation": "Getter/delegate boundaries generally contain standard exceptions and reject stale model state; selection-set edits stage collection before committing, and capture/reorder state is cleared on cancellation/model replacement.",
        "testCoverage": "Focused cases include TestTreePointerRechecksTheModelAfterFocusCallbacks, TestTreeSelectionCallbacksFireOncePerChangeAndSeeTheFinishedSelection, TestTreeSelectAllStopsWhenSelectionCallbackReplacesItsDelegate, TestTreeSelectionSetCallbackMayDestroyTheTree, TestTreeSingleClickCaptureTransferMayRetireTheTree, and TestTreeGesturesContinueWhenACallbackChangesAnotherControl. No tests executed."
      },
      "plannedSha256": "7ed830492ebeffa836ecf98644d112005461366d7ca41bf740f39f4ef2d6a26c",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Typeahead.cpp",
      "sha256": "05ce67c7c18b65c32a8388e49ebc35791233b3ce2a027763e39317b42cfed23e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "05ce67c7c18b65c32a8388e49ebc35791233b3ce2a027763e39317b42cfed23e",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Typography.h",
      "sha256": "cd25c87f67134154a930a9cc3bfa112a40f87569e7ab9770f79d7d3538ff92ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "cd25c87f67134154a930a9cc3bfa112a40f87569e7ab9770f79d7d3538ff92ad",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Win32Hooks.h",
      "sha256": "c5bea4da02db45f90c70203ed1c88de9fed8656ecb8c9873e7169e9cc4218c23",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "c5bea4da02db45f90c70203ed1c88de9fed8656ecb8c9873e7169e9cc4218c23",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.WindowHost.cpp",
      "sha256": "3100948f5cf940ce39903fc5f1b548de4ec87ae2439c5add83004820de9d2aec",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "ed1f004aabd559c69ce2d3665363ff74a3d86bf659d6b3be71f3667c1dde9880",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUiNativeMenuInterop.h",
      "sha256": "c12d6b4c041c73911f8b3be92841d4b28cf48620b4bf83ec65ca5534e4b68646",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "c12d6b4c041c73911f8b3be92841d4b28cf48620b4bf83ec65ca5534e4b68646",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/NativeMenuInterop.cpp",
      "sha256": "26a4a629bae4ace5a162304e4e9a14d45dc9d5fb1455a0d603a20edcc54feb46",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "e1807475db64c3bdf6f4fba97acf5c0d73055a0abe759fb104bcd7be54841d58",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextClipboard.cpp",
      "sha256": "305d0c1a5113c4c48d5743564cc2b7334f848701dbf4eeada7cc9b9a6c56cae1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "305d0c1a5113c4c48d5743564cc2b7334f848701dbf4eeada7cc9b9a6c56cae1",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextClipboard.h",
      "sha256": "82df11645b24522d7c1bd56a51ffa99fe670d049ccaa9222033babbce918681e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "82df11645b24522d7c1bd56a51ffa99fe670d049ccaa9222033babbce918681e",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextInputServices.cpp",
      "sha256": "e18a7f28a58baead8305926e8426addc72921d8dd5163049ad10b5e920d156a5",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [
        "TestApplicationTextStoreTransactions",
        "TestApplicationTextStoreFailuresAndGeometry",
        "TestApplicationTextServiceRetirementReadReentrancyPreservesSameClientSuccessor",
        "TestApplicationTextServiceRetirementPreservesForeignFocusAndReentrantWinner",
        "TestApplicationCompositionCancelAfterFocusReplacement"
      ],
      "findingIds": null,
      "disposition": "reviewed-source; no additional confirmed defect",
      "reviewNotes": {
        "callers": "Read all 997 lines. Traced ITextStoreACP callbacks through ClientTextStoreTarget and TextInputServices attach/dispatch/clipboard/cancel APIs, including TextInputClient read/apply boundaries.",
        "ownershipAndLifetime": "Reviewed same-thread checks, shared client retention across callbacks, operation/focus revisions, dispatch-cookie invalidation, TSF composition ownership, and staged attach rollback. No confirmed use-after-retirement or focus-restoration defect.",
        "failureAndCancellation": "Edits are staged in `_snapshot` through one TSF lock and applied once by Flush using the captured revision; failed edits discard staged dirty state. Disconnect cancels only a still-owned matching focus session. Partial apply remains unproven; no fault injection/runtime test performed.",
        "testCoverage": "Relevant current cases include TestApplicationTextStoreTransactions, TestApplicationTextStoreFailuresAndGeometry, TestApplicationTextServiceRetirementReadReentrancyPreservesSameClientSuccessor, TestApplicationTextServiceRetirementPreservesForeignFocusAndReentrantWinner, and TestApplicationCompositionCancelAfterFocusReplacement. Tests not run."
      },
      "plannedSha256": "01bcf48761ba51b66cc2920c6d4a88f32a20a3e463cf0220a810dbdebb74ee2f",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextStoreTarget.h",
      "sha256": "2238bfa5192eb1317980b8b8a48634b1a7c246495e9602a61361e35ac4f4cd77",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "6a9a7eb6028f13f70e32c6bf7fb3cae926c7c84dc44387fd5a5a840a31492540",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TsfFocus.h",
      "sha256": "fc42f41bf5253d708bab5cbab1cd533c54b66fb70aca72dfdb3388ce01b8da8a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "fc42f41bf5253d708bab5cbab1cd533c54b66fb70aca72dfdb3388ce01b8da8a",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/DxUi.vcxproj",
      "sha256": "66d813eba8f3ec3d78a1a8e6b5d0e20483812e01deeb87d3388aed5e89b067e0",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix",
        "native builds represented by test-consumer.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 checks project and solution mapping; external consumer script tests compile/link against public library. No build run in this review.",
        "failureAndCancellation": "Build matrix validator checks project configuration uniqueness and coverage; missing shader/source compile inputs fail in MSBuild.",
        "callers": "MSBuild static-library project included by DxUi.sln; source set contains controls, rendering, foundation and embedded shader compilation.",
        "ownershipAndLifetime": "Static library; compile items and generated shader outputs are MSBuild-owned; explicit six Debug/Release/ASan Debug by x64/ARM64 configurations."
      },
      "plannedSha256": "66d813eba8f3ec3d78a1a8e6b5d0e20483812e01deeb87d3388aed5e89b067e0",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Foundation/FrameRuntime.cpp",
      "sha256": "e4d747f471cde94d1bb368a99ef3a4f496daa4365b6831b04b84215a56d80be3",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "e4d747f471cde94d1bb368a99ef3a4f496daa4365b6831b04b84215a56d80be3",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Rendering/Embedded.cpp",
      "sha256": "4f4413ccb5b79e28929bad568cb50a573463497ebd2389eee8718c50bcb629e9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "e08cbd13c9d0313c7251e08a81fb2e9dc1294a7e54001beb46f6f216b5914a20",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/AnimationDispatcher.h",
      "sha256": "ee967b0a098f4dd114c21e264f504e5a4908116fdc33e18f164d8e3aa4df463b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "ee967b0a098f4dd114c21e264f504e5a4908116fdc33e18f164d8e3aa4df463b",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/Diagnostics.h",
      "sha256": "baceffa050b9f7feba93879cf1e949f24e44ebccb42bd4d9b2b8efce8c813702",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "baceffa050b9f7feba93879cf1e949f24e44ebccb42bd4d9b2b8efce8c813702",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/LiveResourceCount.h",
      "sha256": "d9073bc9720e38988dd4a034bd40f050df8fb9c30275c57583cad42e0f274e9e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "d9073bc9720e38988dd4a034bd40f050df8fb9c30275c57583cad42e0f274e9e",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/PostedPayload.h",
      "sha256": "0867b27479500e43800ad42e675ed503b565f0244e54bffd3ada7331138baa5f",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "04a87d28d23f40d7330383e1c5bb0dbcfbb1b5775145bdd63624092bf8ef4950",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/RequireAddressSanitizer.h",
      "sha256": "afe0989ccbe34c9e2ffc8c3c6b9394924586e605c7ab371182d6affec5e29f32",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "full-file-read-wrapper-or-contract-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read; these declarations/wrappers resolve to their public canonical header or documented support contract.",
        "ownershipAndLifetime": "Full file read; no owned runtime state or nontrivial lifetime transition is defined in this file.",
        "failureAndCancellation": "Full file read; no runtime failure behavior was exercised.",
        "testCoverage": "Direct runtime test case not established for this declaration/wrapper; no tests were executed."
      },
      "plannedSha256": "afe0989ccbe34c9e2ffc8c3c6b9394924586e605c7ab371182d6affec5e29f32",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/Utilities.h",
      "sha256": "b83184424f0cc02bb79bff1c99a8368dcb9e8635b8188ebe1d5285a1fdafd789",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "b83184424f0cc02bb79bff1c99a8368dcb9e8635b8188ebe1d5285a1fdafd789",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "src/Support/WindowMessages.h",
      "sha256": "df2b86d51b959bf26b424b069ebaf56c37ff5b6d8354a4857443b1efebcb47fc",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "No direct case name is asserted from this file-only review; focused test linkage remains a gap. No tests were executed."
      },
      "plannedSha256": "df2b86d51b959bf26b424b069ebaf56c37ff5b6d8354a4857443b1efebcb47fc",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Test-Changes.ps1",
      "sha256": "5b9fabad89c8ef2e810a2f4784d3b04e40962c7a59ac2d2449ef023f35e583ec",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ScopedTesting.ps1 and focused native/tooling scopes from Tools/tests/Invoke-ToolingTests.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Affected/Full/PrePush orchestration consumes ScopedTesting manifest, build/test entrypoints and exact receipts; CI PrePush delegates only verified matching obligations.",
        "testCoverage": "Test-ScopedTesting covers changed-path and identity/delegation contracts; runtime and CI execution not performed here.",
        "ownershipAndLifetime": "Attests input/artifact identity before selected run, uses profile-local native receipts plus independent tooling receipts; instrumentation never writes qualification receipts.",
        "failureAndCancellation": "No-work is NOT_EVALUATED; unknown scopes widen; dirty input after run blocks receipt; skip build needs exact receipt; no ARM64 runtime cross-host; skips remain nonreusable."
      },
      "plannedSha256": "5b9fabad89c8ef2e810a2f4784d3b04e40962c7a59ac2d2449ef023f35e583ec",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "test-consumer.ps1",
      "sha256": "c996b897cfaf1da879ed1a8db8f24b77c644a584ad378d9e9511ab73f34d489e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Script itself: relocated exact-pin external consumer build, public-header first-include TUs, render modes, native module dispatch, negative pin/build identity checks"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Embeds ten negative pin/build identity cases plus external sample and two plugin-like DLL checks; not run here.",
        "ownershipAndLifetime": "Creates unique relocation fixture, clones local without hardlinks, isolates vcpkg/output and consumer files under selected FixtureRoot, writes report beneath .build/reports.",
        "callers": "Manual/CI consumer qualification invokes pinned checkout as standalone dependency.",
        "failureAndCancellation": "Refuses dirty source or wrong host architecture, fails on restore/compile/render/module/pin mismatch, restores test mutations in finally."
      },
      "plannedSha256": "e5b7102b2c6662e30fb9d6fb1125508a10bfddb4c53051f5d35c4ec35e8ddaec",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Test-PrePush.ps1",
      "sha256": "c161c3569d27bf15c462e693d314c0e3058949b115d19889561596735c9a96ff",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ScopedTesting.ps1: profile matrix/delegation contracts"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Runs Test-Changes PrePush sequentially across six architecture/configuration profiles in pwsh subprocesses and aggregates exit statuses.",
        "testCoverage": "No direct dedicated Test-PrePush script unit test found; six-profile matrix is declared in source and related scoped contracts are tested. No profiles run here.",
        "ownershipAndLifetime": "Each child owns profile-scoped build/test lifecycle; redirects both output streams asynchronously before waiting; parent process disposed per profile.",
        "failureAndCancellation": "Any nonzero profile fails accounting; success explicitly says pending CI and interactive obligations are not passed."
      },
      "plannedSha256": "c161c3569d27bf15c462e693d314c0e3058949b115d19889561596735c9a96ff",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "test.ps1",
      "sha256": "d02b57752be28864bd9f0f4ed4235ec86dd74b2ea65aac76e74ccf2aed867a9e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-TestFilter.ps1, Test-TestWatchdog.ps1, Test-InteractiveLease.ps1, and suite-level native case catalogs"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Canonical native suite runner; defaults ordinary runs through Test-Changes, Full/explicit suite path handles native and focus leases, performance receipt and test evidence.",
        "testCoverage": "Filter/watchdog and lease selftests plus native suites cover runner contracts; no test-suite execution by this review.",
        "ownershipAndLifetime": "Starts exact TestRunEvidence context; runs tooling before build unless skipped, requires current build attestation under SkipBuild, and owns all suite logs/receipts.",
        "failureAndCancellation": "Rejects unsupported architecture/suite/interactive mode; watchdog/timeouts and native exits feed SuiteFailure; foreground suite skips fail; evidence completion always in finally."
      },
      "plannedSha256": "9cd0a4fc6d0179424f5461774916ee183d935de320bd3f996ce537556bcf587d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/capability-skip-allowlist.json",
      "sha256": "c11b73a49ef858a76e6335db4e18e2abb1b086374c6bdb8fa89853d70e0be008",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "Fixture/policy input reviewed; direct test ID absent or caller review remains pending as noted.",
      "reviewNotes": {
        "callers": "No direct reference found in the reviewed test/tooling text searches; the file is the native capability-skip policy input.",
        "ownershipAndLifetime": "A 39-byte tracked JSON policy file containing version 1 and an empty allowances array; no generated/runtime ownership.",
        "failureAndCancellation": "Empty allowlist means no capabilities are waived. No cancellation path applies to static policy input.",
        "testCoverage": "The file itself has no case identifier and currently declares no exceptions; direct executable-consumer coverage is a gap to verify in the build/test manifest."
      },
      "plannedSha256": "c11b73a49ef858a76e6335db4e18e2abb1b086374c6bdb8fa89853d70e0be008",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/ConsumerModules/ConsumerModules.Tests.Driver.cpp",
      "sha256": "4ba696fd015f1727b121c3cb5e584f2b17d877265a7911f3293a983726f25a06",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "wmain",
        "DxUiModuleProbe"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
      "reviewNotes": {
        "callers": "Built as the standalone consumer-module driver; wmain loads NativeModuleA.dll and NativeModuleB.dll and invokes exported DxUiModuleProbe plus the executable probe.",
        "ownershipAndLifetime": "WIL unique_hmodule owns both DLL handles; each two-round UI jthread is joined before module unload, and each thread initializes STA COM.",
        "failureAndCancellation": "LoadLibrary/GetProcAddress failures have distinct exit codes; probe failures aggregate and return 12. The fixture prints a count and never unloads modules while their UI thread is live.",
        "testCoverage": "wmain executes EXE and both module probes twice across two generations, then joins before RAII unload. It is a focused DLL lifetime consumer fixture, not evidence of the broader native suite."
      },
      "plannedSha256": "4ba696fd015f1727b121c3cb5e584f2b17d877265a7911f3293a983726f25a06",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/ConsumerModules/ConsumerModules.Tests.NativeModule.cpp",
      "sha256": "e3998097b59b15649f801beb8c2fef8b53453149e284731d6ed638520348332a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunModuleProbe",
        "DxUiModuleProbe"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
      "reviewNotes": {
        "callers": "ConsumerModules.Tests.Driver.cpp resolves the exported DxUiModuleProbe from the executable and both consumer DLLs.",
        "ownershipAndLifetime": "The probe creates a no-activate owned HWND, attaches/detaches ControlHost with scope guard, owns a local control tree and dismisses asynchronous context menu before callback storage is destroyed.",
        "failureAndCancellation": "Returns distinct probe failures for window/attach/menu/popup/callback path; exported noexcept boundary terminates on bad_alloc and converts named std::exception to failure code after diagnostic.",
        "testCoverage": "RunModuleProbe checks animation tick, dispatcher HWND/module ownership, context-menu procedure ownership and invocation/closure; exported DxUiModuleProbe reruns that behavior behind an exception-safe ABI boundary. Driver repeats it twice per generation."
      },
      "plannedSha256": "e3998097b59b15649f801beb8c2fef8b53453149e284731d6ed638520348332a",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.ClipboardTransport.h",
      "sha256": "8ace8716cc63062dafbacdaa63fdb5479bf34634874f7c8202385db9c0cc3dcf",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestNativeClipboardTransportBenchmark",
        "TestNativeClipboardLargeSelections",
        "TestNativeClipboardFailuresPreserveText",
        "TestEmbeddedClipboardRejectsLargeNativePayload"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
      "reviewNotes": {
        "callers": "Included by DxUi.Tests.NativeTextInput.cpp and its native input test registration.",
        "ownershipAndLifetime": "ClipboardHostWindow and ClipboardTransportFixture own the native clipboard fixture; local strings/models own test payloads; embedded TextInputServices is explicitly detached after its oversized-payload case.",
        "failureAndCancellation": "Injects GlobalAlloc failure, busy clipboard, EmptyClipboard failure, publication failure and embedded NUL; verifies cut leaves source text and prior clipboard contents intact until successful publication.",
        "testCoverage": "Registered tests cover large Unicode round trips at 65,535/65,536/65,537/100,000 units, native Grid/TextField copy/paste/cut, failure preservation, oversized native clipboard rejection at embedded text-service boundary, and a benchmark case with allocation-count assertion plus timing output."
      },
      "plannedSha256": "8f0bf0da68516dae156b1a16c89641ec7722310764ff7f255a88be865856ec14",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiFocusEventClient.h",
      "sha256": "402e777dba824f6129313bd2337c765fec33ecf040b8fad93d86819c29b8fcfd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestWindowHostTabRaisesAutomationFocusChanges",
        "TestWindowHostTreeArrowAnnouncesTheFocusedItem"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
      "reviewNotes": {
        "callers": "Included by DxUi.Tests.Menu.cpp and DxUi.Tests.WindowHost.cpp; observed consumers include TestWindowHostTabRaisesAutomationFocusChanges and TestWindowHostTreeArrowAnnouncesTheFocusedItem.",
        "ownershipAndLifetime": "FocusNameRecorder is WRL/FTM COM with mutex-protected event names; client owns events, recorder and last-declared jthread so thread joins before state destruction. Focus callback reads cached name/process only.",
        "failureAndCancellation": "UIA connection/transaction timeouts bound provider calls and setup/request waits; setup stage and HRESULT are logged; focus event names are filtered to the current process. Destructor pumps while awaiting thread exit, but a failure after its deadline would still encounter jthread join semantics.",
        "testCoverage": "Observed tests cover native host tab-focus announcement and tree focused-item announcement; other direct callsites are in Menu/WindowHost suites. No independent stress test of UIA client teardown under a permanently blocked COM provider."
      },
      "plannedSha256": "402e777dba824f6129313bd2337c765fec33ecf040b8fad93d86819c29b8fcfd",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiTestHelpers.h",
      "sha256": "7b6c7349a11dcc9b48ac5a3f129beb784025fd048de9ccefead49162eb200c6a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestGridMultilineDecomposedAccentsPaintLikePrecomposed",
        "TestNativeTransitionStartsOnTheDispatcherTimelineAfterAStall",
        "TestApplicationTextServiceLifecycle",
        "TestHostFocusReplacementLeavesControlAlone"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Shared include for Controls test translation units; AttachedHostWindow routes WndProc through WindowHost::HandleMessage, then optional application callback. DispatchQueuedMessageForTest is used by host and animation tests; foreground helpers back native-TSF activation tests; RequireFocusReplacementLeavesControlAlone is used by Controls and native text-input regression tests.",
        "ownershipAndLifetime": "AttachedHostWindow owns HWND via wil::unique_hwnd and WindowHost, explicitly detaches before resetting HWND, and stores callbacks capturing test state only while fixture lives. COM/WIC helpers use wil::com_ptr and scope_exit for VARIANT/BSTR/SAFEARRAY/IMM context cleanup. Grid models are borrowed by grids and fixture member order ensures model outlives window-owned tree. Test artifact helper creates a PID-specific directory under repo .build and is used for animation/windowhost logs and gallery captures; consult the DxUi runner sandbox/output contract before treating it as authorized test scratch.",
        "failureAndCancellation": "Message pumps cap at 4096 messages and optional duration; bounded preparation loops cap at 1024. Wait helpers use absolute deadlines and report wait errors. Foreground retry caps attempts at five; repeated observed thieves are capability skips, activation refusal fails. Potential policy issue: GetDxUiTestArtifactDirectory writes under .build without an ownership marker in this helper; assess against the selected test-runner sandbox contract, since this review did not execute it.",
        "testCoverage": "Helper coverage is indirect: named grid multiline, native dispatcher-stall, application TSF lifecycle and focus-replacement tests consume it. The shared fixture also provides test filtering/watchdog, bitmap compare, text/grid/tree models and input/foreground instrumentation; no helper-only test proves every branch. No execution/build performed."
      },
      "plannedSha256": "7b6c7349a11dcc9b48ac5a3f129beb784025fd048de9ccefead49162eb200c6a",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiTestMovedControls.h",
      "sha256": "7ed7e2f8f227de220fdc97559b5d5ed9a616093ddfa2e4fdd89a7a71950ed6b2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestColorPickerMovedBetweenHostsMatchesAFreshOne",
        "TestNumericStepperMovedBetweenHostsMatchesAFreshOne",
        "TestTabControlMovedBetweenHostsMatchesAFreshOne",
        "TestMenuBarMovedBetweenHostsMatchesAFreshOne",
        "TestTextFieldMovedBetweenHostsMatchesAFreshOne"
      ],
      "findingIds": [],
      "disposition": "Test-fixture extraction caveat: helper bypasses Panel::TakeChild; route to test owner for review.",
      "reviewNotes": {
        "callers": "Used by DxUi.Tests.EditorControls.cpp move/fresh comparisons for ColorPicker, NumericStepper, TabControl, MenuBar and TextField.",
        "ownershipAndLifetime": "Each scenario owns two AttachedHostWindow hosts and control trees; helper extracts a child, transfers unique_ptr to second host, compares its geometry and BGRA pixels against a freshly configured control. Host/device lifetime remains local to scenario.",
        "failureAndCancellation": "Device-loss scenario hides the first host and requires second Direct2D device identity to differ; every scenario requires moved control to paint and geometry/pixels equal the fresh peer, reporting pixel/rectangle differences.",
        "testCoverage": "Actual callers: TestColorPickerMovedBetweenHostsMatchesAFreshOne, TestNumericStepperMovedBetweenHostsMatchesAFreshOne, TestTabControlMovedBetweenHostsMatchesAFreshOne, TestMenuBarMovedBetweenHostsMatchesAFreshOne, TestTextFieldMovedBetweenHostsMatchesAFreshOne. The helper extracts via moving GetChildren()[index] directly instead of Panel::TakeChild; this is a test-fixture caveat because it bypasses the public extraction callbacks/inheritance contract."
      },
      "plannedSha256": "7ed7e2f8f227de220fdc97559b5d5ed9a616093ddfa2e4fdd89a7a71950ed6b2",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridMultilineFixtures.h",
      "sha256": "615f3d851e354d68d66085a6168e81cd860e63e60929cfc239ab65230efdabba",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "Fixture/policy input reviewed; direct test ID absent or caller review remains pending as noted.",
      "reviewNotes": {
        "callers": "Included by DxUi.Tests.Grid.cpp, DxUi.Tests.Rendering.cpp and Controls.Tests.GridMultilineRendering.h; consumers use TextTableModel, CaptureWindow, ConfigureHostPlace, PixelRect and RequireIdentical.",
        "ownershipAndLifetime": "TextTableModel owns its nested cell strings and width vector; captures and derived difference records are values. WIL/host ownership is delegated to caller fixtures.",
        "failureAndCancellation": "TextTableModel uses bounds-checked vector access and exposes stable row lookup; pixel utilities treat out-of-capture coordinates as transparent zero and RequireIdentical first checks capture dimensions.",
        "testCoverage": "This header is fixture/utility only and registers no test ID; caller test registration is in Grid and Rendering suites (full rendering caller review is pending). Tests relying on these helpers assert multiline text geometry/pixel output."
      },
      "plannedSha256": "615f3d851e354d68d66085a6168e81cd860e63e60929cfc239ab65230efdabba",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridMultilineRendering.h",
      "sha256": "f0a363167255c0a1315ea5514d0a15a2e7fc5e815042126fd1d63be4ca791e6b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestGridMultilineDecomposedAccentsPaintLikePrecomposed",
        "TestGridMultilineShapedPrefixCutInsideAClusterPaintsLikeItsShortTwin",
        "TestGridMultilineRightToLeftFlowKeepsMarkerSideAndClipping",
        "TestGridMultilineCellNarrowerThanItsWordKeepsInkInsideAndOffersTheValue",
        "TestGridMultilineCellCutByTheViewportPaintsAShiftedCropOfItsWholeSelf",
        "TestGridMultilineRepaintsAtANewDpiLikeAFreshGrid",
        "TestGridMultilineRepaintsAfterThemeFontAndDensityChangesLikeAFreshAttach",
        "TestGridMultilinePrefixSharingNeverCrossesTextThatLaysOutDifferently",
        "TestGridMultilineLayoutTablesStayAtALoweredCeilingAndPaintRightAfterEviction",
        "TestGridMultilineTrimmedCellsPaintTheSameAfterDeviceLoss",
        "TestGridMultilineMovedBetweenHostsMatchesAFreshOne"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Included by DxUi.Tests.Rendering.cpp after the multiline fixtures and registered in its rendering suite. MultilineBed owns the model before AttachedHostWindow so the host-owned Grid can borrow it; grid replacement is via SetRoot.",
        "ownershipAndLifetime": "Captures use host test utilities and DirectWrite layouts in local wil::com_ptr; model text stays exact (including decomposed accents) and is not normalized. Fresh-grid comparisons preserve model and settings while replacing only the Grid/root. Device loss tests retain DWrite layouts while rebuilding D2D resources.",
        "failureAndCancellation": "Tests bound the 100k-value prefix and cache ceiling and assert clipping, ellipsis, and layout-reuse behavior across RTL, DPI/theme/font/density, eviction, device loss and host movement. Long loops are deterministic but substantial; no cancellation path is exercised. Independent font/platform raster variability is controlled by same-process twins, while DirectWrite-specific shaping remains platform-dependent.",
        "testCoverage": "Eleven registered rendering cases cover cluster-safe prefixes, bidi marker direction, viewport clipping, cache prefix identity/caps, retained layout invalidation, device loss and movement. Base rendering cases live in DxUi.Tests.Rendering.cpp. No runtime or pixel verdict is claimed here."
      },
      "plannedSha256": "f0a363167255c0a1315ea5514d0a15a2e7fc5e815042126fd1d63be4ca791e6b",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.LocalizedLayoutFixture.h",
      "sha256": "f14e4e4dafd54df43b5207290df160c2117d94d04dba4b9f0df99d75cdf285ad",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestLocalizedActionLayout"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
      "reviewNotes": {
        "callers": "Used by DxUi.Tests.Controls.cpp TestLocalizedActionLayout.",
        "ownershipAndLifetime": "Fixture owns DirectWrite factory/format/layout COM pointers through WIL and uses fixed local arrays for scenarios/rectangles; arrange strategy is injected by caller.",
        "failureAndCancellation": "Every DirectWrite creation, wrapping setting and metrics call is required to succeed; final assertion verifies all measured French action rectangles fit, preserve minimum sizes/order and do not overlap.",
        "testCoverage": "TestLocalizedActionLayout executes 24 width/scale combinations, 2,000 repeats per workload across five timed rounds, then validates all layout geometry. Timing is diagnostic output; no threshold qualifies performance."
      },
      "plannedSha256": "f14e4e4dafd54df43b5207290df160c2117d94d04dba4b9f0df99d75cdf285ad",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextInputServices.h",
      "sha256": "669e47ab4f96154432d6e0decf8ef06cf7502daf6fc81c3d6a6eab964d256492",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestApplicationTextStoreTransactions",
        "TestApplicationTextStoreFailuresAndGeometry",
        "TestApplicationTextStoreDefersExternalNotificationUntilLockUnwinds",
        "TestApplicationTextServiceLifecycle",
        "TestApplicationTextServiceReadReentrancyPreservesNewClient",
        "TestApplicationTextServiceCancelReentrancyPreservesNewClient",
        "TestApplicationTextServiceSameClientReattachmentSurvivesReadCallbacks",
        "TestApplicationTextServiceRetirementReadReentrancyPreservesSameClientSuccessor",
        "TestApplicationTextServiceDetachRejectsReentrantReattach",
        "TestApplicationTextServiceRetirementPreservesForeignFocusAndReentrantWinner",
        "TestApplicationClipboardCommands",
        "TestBoundedClipboardDecode",
        "TestApplicationTextStoreDeferredLocks",
        "TestApplicationCompositionCancelAfterFocusReplacement",
        "TestApplicationTextStoreInsertionAndLayout"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Included and registered by DxUi.Tests.NativeTextInput.cpp; the tests exercise TextInputServices, application TextInputClient and the shared COM text store created by CreateClientTextStore. TSF-focused cases initialize STA and use AttachedHostWindow; lower-level store cases use fake clients/sinks and no renderer HWND.",
        "ownershipAndLifetime": "Fake clients and sinks are declared before stores where held COM references must release first. Application TextInputServices borrows the attached HWND; clients use shared_ptr snapshots. Clipboard/TSF resources are local and services are explicitly cleared/detached. Reentrancy fixtures intentionally replace, clear, or retire clients inside Read/Apply/Cancel/TSF callbacks and check the successor stays active.",
        "failureAndCancellation": "Coverage includes stale revision/focus, masked/read-only access, invalid/oversized snapshots and UTF-16 clipboard data, deferred lock posting failure and retry, callback disconnection, final store release, composition cancellation/rollback, and balanced transaction notifications. Potential test-guideline issue: TextServicesReentrantFocusSink::OnSetFocus catches std::exception (which includes std::bad_alloc) and maps to E_FAIL despite repo rule to keep bad_alloc fatal; this is test-fixture code and should be dispositioned by owner.",
        "testCoverage": "Fifteen named cases cover COM composition/edit transactions, reentrancy, lock coalescing, geometry, clipboard bounds, focus retirement and disconnect. The foreground-preservation case may capability-skip only on observed foreground theft; it fails activation refusal. No tests/builds were run in this review."
      },
      "plannedSha256": "10c0f41450d32d5a574377c2725f3e107e477b4ece9531c61b106a0f8ab8729b",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextStoreComposition.h",
      "sha256": "ec60870948d48465544419c9a0fc155d39073e53b7a5889018e6247f3c51fc28",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestNativeTextStoreCompositionRangeUsesStagedTextAfterInsert",
        "TestNativeTextStoreSequentialCompositionsInOneLockHaveSeparateUndoUnits",
        "TestNativeTextStoreStaleSameLockCompositionCannotReplaceExternalText",
        "TestRichTextNativeTsfCompositionAndDisconnection"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Included by Controls.Tests.TextInputServices.h and DxUi.Tests.NativeTextInput.cpp; the two small COM doubles feed the real native text store's composition-query and composition-notification tests.",
        "testIds": [
          "TestNativeTextStoreCompositionRangeUsesStagedTextAfterInsert",
          "TestNativeTextStoreSequentialCompositionsInOneLockHaveSeparateUndoUnits",
          "TestNativeTextStoreStaleSameLockCompositionCannotReplaceExternalText",
          "TestRichTextNativeTsfCompositionAndDisconnection"
        ],
        "failureAndCancellation": "Only GetExtent/SetExtent and composition GetRange/owner are supported; unsupported COM methods fail explicitly with E_NOTIMPL. The fixtures do not model asynchronous cancellation or production ownership.",
        "ownershipAndLifetime": "ClientTextRange and ClientComposition are stack test doubles with explicit COM reference counters; composition GetRange returns its embedded range with AddRef. Their methods are intentionally minimal and return E_NOTIMPL for unused ITfRange members.",
        "testCoverage": "Named consumers include TestNativeTextStoreCompositionRangeUsesStagedTextAfterInsert, TestNativeTextStoreSequentialCompositionsInOneLockHaveSeparateUndoUnits, TestNativeTextStoreStaleSameLockCompositionCannotReplaceExternalText and TestRichTextNativeTsfCompositionAndDisconnection. Tests were not run."
      },
      "plannedSha256": "ec60870948d48465544419c9a0fc155d39073e53b7a5889018e6247f3c51fc28",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.ControlTests.vcxproj",
      "sha256": "03a35fd7d38aecfd2e9d2a8e005116f04fbbc99009f1523120eaf1e62490142c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Project build wiring: explicit ClCompile entries for test suites; no test execution evidence"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "The project compiles the Controls test translation units and references src/DxUi.vcxproj; the runner registration lives in DxUi.Tests.Runner.cpp, so inclusion here alone does not prove registration.",
        "ownershipAndLifetime": "The project owns no runtime objects; MSBuild includes each listed translation unit in the test executable. Explicit per-file includes make adding a new test implementation require project registration.",
        "failureAndCancellation": "Configuration defines Debug/Release and ASan Debug for x64/ARM64; the file sets ENABLE_TESTS and Console subsystem. No project-level test timeout/cancellation policy is encoded here.",
        "testCoverage": "Source review only: verifies explicit source inclusion, DxUi reference, and ENABLE_TESTS. No build or test run was performed; runtime coverage must be established from named runner registrations and receipts."
      },
      "plannedSha256": "ce63772309f12c04624f19a71cc11883864fc66f69b51f21780f7117f57e77f6",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Accessibility.cpp",
      "sha256": "aebf4c478c6609b8f0675dbcec0cf44558e3b6c43e2c962efe5ee2620e66bbf9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "fce48b89bb0236cd8842f37d50523ab177be013f7a189205acb43262fb37bab8",
      "reviewEvidence": "review-host-controls-current.md",
      "leadDeltaReview": "Lead fully reviewed added wide-tree path-resolution test and extended real ProgressBar client/getter test. Red/green diagnostics retained; the broad current-source Accessibility run is separate."
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Animation.cpp",
      "sha256": "19d080bf3aaa806516a3d75254d1a1339a0b1e60ad3b653ab5758b5ec477dc59",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestFrameRuntimeClockIsMonotonic",
        "TestAnimationDispatcherActiveSubscribersReceiveMonotonicHighResolutionTicks",
        "TestSliderTouchHaloStaysInsideItsPaintBoundsAndBehindOverlays",
        "TestGridScrollbarFeedbackFollowsHoverAndDragState",
        "TestPageHostConnectedOverlayAnimationEmitsCompositionGateMetrics",
        "TestPageHostTransitionBegunWhileHiddenResumesWhenPainted"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunAnimationTests registers this translation unit in DxUi.Tests.Runner.cpp. Cases compose FrameRuntime, AnimationDispatcher, controls, PageHost, AttachedHostWindow and Support performance capture; the embedded clock family is included from Controls.Tests.AnimationClock.h.",
        "ownershipAndLifetime": "WindowHost/root trees and controls are scope-owned; test models outlive borrowed Grid references. AttachedHostWindow owns and detaches native HWND. AnimationDispatcher callbacks use stack context only while subscription scope is active; metric sink is stopped by ScopedAnimationPerfJsonl.",
        "failureAndCancellation": "Tests cover reduced-motion snapping, settled tick cancellation, pointer capture loss, hidden PageHost no-subscription/resume, device-independent visual clipping, dispatcher monotonicity/hitch, and a live native stall. Dispatcher test globally calls Shutdown before/after cases, so serialized suite execution is assumed. Timing/foreground and pixel-capture paths are not run in this review.",
        "testCoverage": "Named tests cover frame clock arithmetic, dispatcher cadence/metrics, button/slider/caret/grid/page transitions, overlays, reduced motion and embedded epochs. Static reading found no definite product defect. The native dispatcher stall and connected-overlay metric cases rely on wall-clock callbacks and actual capture; no build/runtime result is claimed."
      },
      "plannedSha256": "22445bfa531ec29a232d8e2e5a480b6f11f7ec3df048736cd565c8941613ab57",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.ComboBox.cpp",
      "sha256": "3eca13d18e5ada0b5e85e9344ef2135f37ff55f2360ff7b09c4bdd7d5bdfa234",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestAttachedComboBoxSentMouseHoverAndOutsideDismiss",
        "TestComboBoxTouchRowsKeepGeometryAndSelectionCoherent",
        "TestComboBoxLocalizedNoMatchesRefreshesOpenPopup",
        "TestComboBoxInputLeavesAComboBoxTheFocusCallbackDestroyed",
        "TestEditableComboBoxImportReportsWhenTextChangedDestroysComboBox",
        "TestComboBoxTextChangeSnapshotCleanupRetiresRootBeforeCallerContinues"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunComboBoxTests is dispatched by DxUi.Tests.Runner.cpp. Suite covers ComboBox popup layout/input, ScrollPanel overlay transforms, TextInput state, color-font rendering, theme styling, callbacks and reentrant root replacement.",
        "ownershipAndLifetime": "ComboBox/Grid items and model references are owned by local vectors/control; attached hosts own their HWND and root; tests check callback snapshots remain alive if controls are destroyed or callables replaced. One helper constructs a shared_ptr with explicit new/delete for a cleanup sentinel to force root cleanup on captured-callback release.",
        "failureAndCancellation": "Named tests cover popup light-dismiss to nonfocusable target, click-through/overlay hit blocking, scroll viewport coordinate transforms, clamped page navigation, scrollbar drag, touch min row height invalid values, reentrant callback/destruction, import survival result and popup close on focus loss. Native color-font test depends on installed DirectWrite color-font support and asserts pixels; no build/run executed.",
        "testCoverage": "Thirty-eight cases include rendering/pixel probes, hover and click, constraints, scrolling, selection and focus, ownership/reentrancy, localization and callback state. Same-host captures avoid stored bitmap oracles. Review found no confirmed defect; source review is not runtime qualification."
      },
      "plannedSha256": "3eca13d18e5ada0b5e85e9344ef2135f37ff55f2360ff7b09c4bdd7d5bdfa234",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Controls.cpp",
      "sha256": "7a8f975777eb867125a2567fd1402b049f59cdd9edba832ab00c5c6f555edd56",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestPanelTakeChildPreservesInheritanceAndReparentsSafely",
        "TestChildAndTabExtractionPreservesReentrantHostTransfer",
        "TestChildAndTabExtractionCancelLiveSliderDrafts",
        "TestScrollPanelEndsTheGestureOfANestedRowItsPanelTakes",
        "TestTabControlLayoutRevalidatesPagesAfterCallbacks",
        "TestPanelTraversalCallbacksCanRetireOwnerSafely",
        "TestPageHostOutgoingPaintCanReplaceItsPageWithoutLeakingDrawingState",
        "TestScrollPanelQueriesAndPaintStopAfterRootRetirement",
        "TestStackPanelLayoutStopsAfterChildCallbacksRetireItsTree",
        "TestControlsThatFocusThemselvesLeaveAControlTheFocusCallbackDestroyed"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunControlTests registers focused container, callback, grid, input and layout cases and calls the intrinsic/form suites. Local fixtures exercise Panel, ScrollPanel, PageHost, TabControl and host dispatch directly; the shared test runner drives these registrations.",
        "ownershipAndLifetime": "Panel/Tab extraction tests move owning unique_ptr children, preserve metadata/selection, and probe reentrant focus/capture callbacks that can transfer or destroy owners. ScrollPanel detach probes cover hover, capture, nested descendants, pages and tabs. Traversal and PageHost fixtures deliberately replace roots/pages during callbacks and verify stale results are rejected; shared state is test-owned to survive retired controls.",
        "failureAndCancellation": "Coverage exercises capture cancellation and rollback on extracted live sliders, silent capture release, callback-driven root replacement, exception-to-HRESULT behavior, measured-layout output preservation on invalid input/overflow, and clean recovery capture after page/scroll paint retirement. Paint capture uses attached host windows; other cases are deterministic direct dispatch. No runtime result is claimed.",
        "testCoverage": "The 3525-line file registers broad regressions for ownership transfer, reentrant focus/capture, empty slots, accessibility invoke, keyboard/mouse semantics, menu/tab geometry and callbacks, selection, grid checkboxes, toggles, tag picker, and animation-adjacent painting. Meaningful remaining gap: no fault-injection allocator coverage for container/tab mutations; no build/test executed by this review."
      },
      "plannedSha256": "e03c0a6e9374f7734f0360b119ad92ac9b4d03343b73191e5df019d6bb894631",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.EditorControls.cpp",
      "sha256": "3f25ff036a07fab47e56747087493a246a3bf8f956c78876d2e037932a841382",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestSplitterDragPreviewsCommitsAndCancels",
        "TestSplitterChangeCallbackCanReplaceRootSafely",
        "TestNumericStepperTypingPreviewsEnterCommitsEscapeCancels",
        "TestNumericStepperEditsEndWithCommitOrCancel",
        "TestColorPickerCancelPaths",
        "TestColorPickerCommitCallbackCanReplaceRootSafely",
        "TestColorPickerReleasesItsGradientsWhenItsHostChanges",
        "TestMovingAControlAnnouncesWhatItNowInheritsOnce",
        "TestPanelSkipsTheSlotOfAChildMovedOutThroughGetChildren",
        "TestEditorControlPressesLeaveAControlTheFocusCallbackDestroyed"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunEditorControlTests registers Splitter, NumericStepper, ColorPicker, reparenting and catalog cases, then dispatches shared document/editor tests. Local helpers send direct control input to WindowHost; movement cases compare a reparented control against a newly configured oracle.",
        "ownershipAndLifetime": "Tests use stack WindowHost/PageHost and unique_ptr control trees. Reparent cases transfer owning child pointers with TakeChild; callback replacement cases deliberately destroy the active root during Commit. TestEditorControlPressesLeaveAControlTheFocusCallbackDestroyed exercises focus-callback teardown for Splitter and ColorPicker. The panel empty-slot case wraps ClearChildren in SEH because access violation is not a C++ exception.",
        "failureAndCancellation": "The suite covers Splitter capture/Escape rollback, NumericStepper Escape/unparseable-edit rollback and callback root replacement, ColorPicker capture/Escape/Cancel rollback, host-change gradient release, density/flow propagation, and stale-slot clearing. Several tests call Paint without a device context; host-dependent capture paths use AttachedHostWindow and CaptureWindow. No cancellation fault injection or runtime result is claimed.",
        "testCoverage": "Registered cases include numeric parsing/rounding and locale digits, drag and keyboard input, callbacks that replace roots or retain mutable state, picker hex and typed components, accessibility names and caption width bounds, moved control geometry and cached resources, inheritance announcements and null child slots. Evidence is source-only; no build/test executed by this review."
      },
      "plannedSha256": "9651ce1190c5e9f2d30d146bf7707506bfbff4e696c7f99e7a7b41d038b29ca6",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Gallery.cpp",
      "sha256": "bf347dc6504c79219af9f8de0a7d0490273b417f1c890d9c71c1ed774acdabd0",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunGalleryGenerator",
        "RunGalleryGeneratorPerTheme",
        "RunButtonContrastAuditGenerator"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Generator entrypoints are invoked by the test executable/tooling-selected gallery and contrast-audit modes, not DXUI_RUN_TEST registrations. RunGalleryGenerator builds five deterministic theme scenes; menu/combo bitmap capture is driven from a dedicated owner HWND plus a joined background popup-window observer.",
        "ownershipAndLifetime": "GalleryScene owns the control tree and models via unique_ptr; model pointers are borrowed by Tree/Grid and remain alive through capture. GalleryBitmapPreview owns capture bytes and caches device-specific D2D bitmap/device via wil::com_ptr, replacing cache when host device changes. Popup capture uses stack AttachedHostWindow, a driver thread joined before local teardown, and scope_exit to dismiss owned popup chain.",
        "failureAndCancellation": "Popup waits have finite deadlines and bounded polling; capture validates visible/nontransparent pixels before composition. Thread driver records failure text and is joined before Require. Output paths are made absolute, existing output is removed, then PNG write/existence checked; this is a generator side effect that depends on caller-provided output directory. No build or runtime qualification performed.",
        "testCoverage": "Gallery content spans control variants, themes, RTL/localized forms, rich text/math/diagram previews, popup snapshots, and button-contrast calculations. Source tests verify scene expectations and output checks, but no dedicated unit registration or assertion independently validates capture-thread timeout cleanup or pixel provenance; no execution result claimed."
      },
      "plannedSha256": "16577e6990042b855b94ba6e710a3a7c92c7772a532b9fbf68313ab941aaa56a",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Grid.cpp",
      "sha256": "5b5e1c00c853d073f343132d9c5534a3359c1317afea2c62a8f9fb003d105f6f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunGridTests",
        "DxUi.Tests.Grid suite"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "392831acb286886672f70fc565c4ff5bcf23a11879d7abbe84076d8255607f2e",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.InteractiveLease.cpp",
      "sha256": "6e7813a0370e751561a9eeec1b1a4d4b30c8a893a4d35e0b3f61e81edf406417",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestDesktopLeaseCaptureDistinguishesEveryNullFocusPath",
        "TestDesktopLeaseDoesNotTrustACallThatReportsSuccess",
        "TestWin32BackendReadsTheDesktopWithoutChangingIt",
        "TestApprovedAutomaticLeaseRefusesADesktopLostDuringTheCountdown",
        "TestLeaseReleasesTheSessionOnEveryPath",
        "TestLeaseResultNamesEveryFact",
        "TestLeasePlanFileParsing",
        "TestConfirmationCountdownEndsInACancellation"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunInteractiveLeaseTests is registered in DxUi.Tests.Runner; this source exercises shared DesktopLease/RunLease/CheckInteractiveDesktop from Tests/Support and the non-activating native backend.",
        "ownershipAndLifetime": "FakeDesktop models windows and callbacks with stack fixtures. Win32 backend test owns only an offscreen WS_EX_NOACTIVATE test HWND with wil::unique_hwnd and never shows it. FakeServices records RAII-like acquire/release/awake/warning order; test uses no real foreground interaction.",
        "failureAndCancellation": "Named cases cover declined/timed-out/interrupted requests, failed children/start/watchdog, busy lease, lost desktop during countdown, failed foreground/focus/cursor restoration, stale capture and release on every path. Loops are finite per lease constants and fake Wait never sleeps. Real Win32 probe is read-only.",
        "testCoverage": "Suite registers 44 named cases across lease restoration, availability, confirmation/autostart, process results, serialization/parser and countdown. Implementation resides in Support.Tests.InteractiveLease.h and external tool runner supplies private desktop for its own self-test; no test run performed."
      },
      "plannedSha256": "6e7813a0370e751561a9eeec1b1a4d4b30c8a893a4d35e0b3f61e81edf406417",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Menu.cpp",
      "sha256": "1c919be05e8a211aaa1d158349e0fff31c5376f213b2129801282ad61d8b0576",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestDescribedMenuAccessibilityInvokesAndDisconnects",
        "TestDescribedMenuAccessibilityInvokeReportsPayloadSaturationAndRecovers",
        "TestDescribedMenuPointerAndCancelledQueuedInvoke",
        "TestDescribedMenuSubmenuKeepsSessionAndIdentity",
        "TestDescribedMenuScrollRepublishesAccessibleGeometry",
        "TestPlainMenuAccessibilityInvokesAndDisconnects",
        "TestPlainMenuAccessibilityScrollsFocusedRow",
        "TestMenuNativeFocusSelectsNoRowAndRestoresTheChosenOne",
        "TestMenuSurvivesAWindowClosingItFromSetCursor",
        "TestMenuForegroundWitnessReacquiresFocusAfterPreparedSetup",
        "TestDescribedMenuReleasesItsMemoryWhenItCloses",
        "TestDescribedRowsHoldOneLayoutEach",
        "TestDescribedMenuDrawsItsDescriptionAgainAfterDeviceLoss",
        "TestLargeMenuPaintsOnlyVisibleRowsWithCachedOffsets",
        "TestModalMenuDismissesOnSentCancellationAndDeactivation",
        "TestMenuLoopEndsWhenAnotherThreadDismissesIt",
        "TestNativeMenuBarNestedPopupCanDestroyHostSafely",
        "TestMenuExitLifetime"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunMenuTests dispatches RunNewControlCapturingMenuTests and RunMenuDescriptionTests; RunMenuDescriptionTests registers accessibility, modal and async menu, UIA, scroll, focus, DPI, layout, rendering, and lifetime cases. Menu callbacks are invoked by popup message handling and posted payload dispatch.",
        "ownershipAndLifetime": "Fixtures own AttachedHostWindow, popup UIA COM providers, test payloads, and foreground witness threads with WIL/jthread scopes. Tests verify retained providers disconnect after popup teardown, payload registry saturation/release and owner drain, queued invoke cancellation on WM_NCDESTROY, layout/accessibility resource return while providers remain held, and process-exit suppression of callbacks. No defect confirmed from the complete source read.",
        "failureAndCancellation": "Coverage exercises payload-post saturation HRESULT and recovery, failed optional description prep with recovery, Escape dismissal before queued UIA action dispatch, submenu close without ending parent session, foreign-thread dismissal, device-loss redraw, and foreground loss/retry. Interactive/focus/cursor tests use the foreground-held helpers or explicitly skip without the desktop; code review only, no test execution.",
        "testCoverage": "Meaningful exact cases include TestDescribedMenuAccessibilityInvokeReportsPayloadSaturationAndRecovers, TestDescribedMenuPointerAndCancelledQueuedInvoke, TestPlainMenuAccessibilityScrollsFocusedRow, TestMenuSurvivesAWindowClosingItFromSetCursor, TestNativeMenuBarNestedPopupCanDestroyHostSafely, and TestLargeMenuPaintsOnlyVisibleRowsWithCachedOffsets. Additional registrations cover exact layout geometry, 50/128/200-row behavior, both visual themes, fractional DPI, and process exit; no runtime or pass claim."
      },
      "plannedSha256": "d79f1e0391190a70a1749a8d84cb47871871ac1be30e17a49c44b2fd1f8f4d14",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.MenuResources.h",
      "sha256": "8c025ee902b6caf5b6af1dfbde2ce7997ff916346351fd4fa82fb9b7eab33c48",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunMenuResourceTests",
        "RunMenuResourceScalingTests",
        "RunMenuTextLayoutResourceTests"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "8c025ee902b6caf5b6af1dfbde2ce7997ff916346351fd4fa82fb9b7eab33c48",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.MultilineText.cpp",
      "sha256": "e15f6c59d0664ff636e2b4d5bd7ce9631bd1eba75d20c3ea3c752c4b4e9daaac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestMultilineTextFieldBackspaceDeleteRemovesSelectionAcrossLogicalNewline",
        "TestMultilineTextFieldCtrlXCutsSelectionAcrossLogicalNewline",
        "TestWrappedMultilineTextFieldCtrlVReplacesPartialSelection",
        "TestMultilineTextFieldUndoRedoRestoresPartialSelectionReplacement",
        "TestMultilineTextFieldRedoClearsAfterNewEdit",
        "TestMultilineTextFieldMouseClickPlacesCaretByPointAndTypesAtCaret",
        "TestMultilineTextFieldCtrlShiftArrowExtendsSelectionByWord",
        "TestWrappedMultilineTextFieldPageKeysUseVisualLines",
        "TestMultilineTextFieldAccumulatesPartialWheelDelta",
        "TestWrappedMultilineTextFieldReturnReplacesSelectionWithNewline",
        "TestMultilineTextFieldMixedDialogFlowKeepsReturnEscapeAndTabRoutingCorrect"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunMultilineTextTests is invoked by the Controls test runner. Cases instantiate TextField/ExposedTextField directly through WindowHost or ClipboardHostWindow and call OnKeyDown, OnChar, mouse handlers, wheel handlers, and TextInputState import/export.",
        "ownershipAndLifetime": "TextField and root ownership are stack/Panel scoped; ClipboardHostWindow owns the native host/clipboard interaction. Direct tests keep source text snapshots by value. Clipboard-sensitive cases use RetryClipboardSensitiveAction where contention is expected. No confirmed defect in test source; no production callers were traced because this row is test-only.",
        "failureAndCancellation": "Assertions cover failed/no-op clipboard actions preserving sentinel clipboard, LF normalization from CRLF paste, selection replacement/caret collapse, undo/redo and redo invalidation after a new edit, word/visual-line navigation, wheel delta accumulation, and dialog Return/Escape/Tab routing. Clipboard retries are bounded helper-mediated; no explicit cancellation path.",
        "testCoverage": "Exact registered cases include TestMultilineTextFieldUndoRedoRestoresPartialSelectionReplacement, TestWrappedMultilineTextFieldCtrlXCutsPartialSelection, TestMultilineTextFieldAccumulatesPartialWheelDelta, TestWrappedMultilineTextFieldPageKeysUseVisualLines, and TestMultilineTextFieldMixedDialogFlowKeepsReturnEscapeAndTabRoutingCorrect. Coverage distinguishes logical newline versus wrapped visual-line behavior and full versus partial ranges. No tests were run."
      },
      "plannedSha256": "e15f6c59d0664ff636e2b4d5bd7ce9631bd1eba75d20c3ea3c752c4b4e9daaac",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.NativeTextInput.cpp",
      "sha256": "b8627471d45871793df0438a962bdd0caf59fe51ab2f35eba9658af1af0f8d6f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "09b3a8ba36655a60a7108d1ef462d69eecb61dae8cf181ed17830bc22b548aef",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.NewControls.cpp",
      "sha256": "c637b6f36dee1695ae284765b3c81e897e72274e6bc695ca87909fba980a540f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestButtonMouseClickReleasesHostCaptureBeforeCallback",
        "TestRadioButtonSelectionChangedCanReplaceRootSafely",
        "TestProgressBarIndeterminateTickSurvivesClockReset",
        "TestSliderOffCenterGrabAndCancel",
        "TestTabControlHiddenTabsKeepStableIndicesAndLeaveTheHeader",
        "TestTabControlRemoveResolvesRequestedPageAfterFocusCallbackReordersSiblings",
        "TestTabControlCloseReportsResolvedIndexAfterCloseCallbackRemovesSibling",
        "TestTagPickerSelectCallbackCanRetireRootAndReadSelectionAndCapture",
        "TestTabControlSelectionSnapshotCleanupCanRetireRootBeforeMouseDownTail",
        "TestMenuBarHoverMoveSnapshotCleanupCanRetireRootSafely",
        "TestCompositeControlsKeepTheirOwnParts",
        "TestContextMenuPopupHonorsSessionMaxRootHeight"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunNewControlTests registers button, checkbox/radio, progress/page, slider, toolbar/menu bar, tab, status strip/tag picker, and host backdrop cases. The separate RunNewControlCapturingMenuTests registers the two native oversized-menu modal fixtures through RunMenuTests.",
        "ownershipAndLifetime": "Tests mostly use stack-owned controls or Panel-owned children under WindowHost/AttachedHostWindow. Callback-reentrancy cases replace the root, reorder/remove tab siblings, or retain captured state; composite-parts test asserts embedded controls cannot be detached. Three test fixtures use shared_ptr<int>(new int, custom deleter) to trigger captured-state cleanup and host root retirement; this is test-only and works through the custom deleter, but it is a localized raw new/delete pattern inconsistent with DxUi AGENTS resource guidance only if that guidance is interpreted as applying to ordinary test allocations. No production defect confirmed.",
        "failureAndCancellation": "Tests cover callback-triggered root replacement and sibling mutation, selection/callback identity resolution, capture release before callbacks, slider draft cancellation on capture loss, invalid/nonfinite range rejection, progress tick resets/hidden/reduced-motion cases, and oversized-menu dismiss cleanup. The two capturing-menu fixtures use an auxiliary std::thread with bounded popup readiness and scope-exit Escape; parent runner holds the explicit interactive lease. No tests were run.",
        "testCoverage": "Exact cases include TestTabControlRemoveResolvesRequestedPageAfterFocusCallbackReordersSiblings, TestTabControlSelectionSnapshotCleanupCanRetireRootBeforeMouseDownTail, TestProgressBarIndeterminateTickSurvivesClockReset, TestSliderOffCenterGrabAndCancel, TestTagPickerSelectCallbackCanRetireRootAndReadSelectionAndCapture, and TestContextMenuPopupScrollsOversizedContent. Around 100 registered cases span callback mutability/lifetimes, stable tab indices, RTL, boundaries, capture and rendering-missing-context paths. No runtime qualification is claimed."
      },
      "plannedSha256": "e5aede42c305cc03622c3889ee82d1df488ad45316fc4b6e674a2593c23c4703",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.ReadOnly.cpp",
      "sha256": "d41645dbff91678b11031d60f7901e2a1aecd5bae5cbdabb12785f1fac6e229d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestReadOnlyMultilineTextFieldSuppressesMutationAndKeepsNavigation",
        "TestAttachedReadOnlyMultilineNativeHostSuppressesMutatingMessages",
        "TestReadOnlyMultilineNativeHostCopyShortcutsPreserveSelectionAndText",
        "TestReadOnlyMultilineTextFieldCopyCutShortcutsWithoutSelectionLeaveClipboardAndTextUnchanged",
        "TestAttachedReadOnlyMultilineNativeHostUndoRedoLeaveTextAndSelectionUnchanged",
        "TestAttachedReadOnlyMultilineNativeHostCutAndClearWithoutSelectionLeaveClipboardAndTextUnchanged",
        "TestReadOnlyMultilineTextFieldCtrlBackspaceDeleteWithoutSelectionLeaveTextUnchanged",
        "TestReadOnlyMultilineTextFieldCtrlArrowMovesByWordBoundary",
        "TestReadOnlyWrappedMultilineTextFieldSuppressesMutationAndKeepsNavigation",
        "TestAttachedReadOnlyWrappedMultilineNativeHostSuppressesMutatingMessages",
        "TestAttachedReadOnlyWrappedMultilineNativeHostUndoRedoLeaveTextAndSelectionUnchanged",
        "TestReadOnlyWrappedMultilineTextFieldCtrlArrowMovesByWordBoundary",
        "TestAttachedReadOnlyWrappedMultilineNativeHostCtrlArrowKeepsCaretAligned"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunReadOnlyTests registers unit and native-host coverage for read-only multiline and wrapped multiline TextField. Direct WindowHost cases exercise TextField handlers; AttachedHostWindow cases route WM_COPY/CUT/PASTE/CLEAR/CHAR/UNDO and verify the native host HWND is the text-input target with no bridge child.",
        "ownershipAndLifetime": "Tests own host, Panel tree, and ExposedTextField through automatic/Panel lifetime. Clipboard-sensitive test bodies use RetryClipboardSensitiveAction, while non-clipboard navigation/undo paths avoid clipboard state. Attached fixtures are no-activation and use SendMessage for deterministic native message routing. No specific defect confirmed from source; no runtime claim.",
        "failureAndCancellation": "Covers copy allowed with selection while cut/paste/clear/character/undo/redo mutation is rejected, empty selection preserving clipboard, text and caret, direct/wrapped navigation still functioning, and native state/visible selection staying aligned after Ctrl+Arrow. Retry handles clipboard contention only; no cancellation branch.",
        "testCoverage": "Meaningful exact cases include TestAttachedReadOnlyMultilineNativeHostSuppressesMutatingMessages, TestReadOnlyMultilineNativeHostCopyShortcutsPreserveSelectionAndText, TestAttachedReadOnlyMultilineNativeHostUndoRedoLeaveTextAndSelectionUnchanged, TestReadOnlyWrappedMultilineTextFieldSuppressesMutationAndKeepsNavigation, and TestAttachedReadOnlyWrappedMultilineNativeHostCtrlArrowKeepsCaretAligned. The suite pairs logical-newline and wrapped visual-line behavior and tests both selected and collapsed-caret paths. No tests were run."
      },
      "plannedSha256": "d41645dbff91678b11031d60f7901e2a1aecd5bae5cbdabb12785f1fac6e229d",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Rendering.cpp",
      "sha256": "14cacc34b997268a61073866c1be75ad97795530b9b1d7b961ef5155ad501461",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "TestGridMultilineLayoutsSurviveRepaintForEveryDistinctVisibleCell",
        "TestGridReleasesItsLayoutsWhenItStopsPainting",
        "TestGridMultilineOversizedValueShapesOnlyItsVisiblePrefix",
        "TestThroughputGraphHueChurnPerformanceScenario",
        "TestAttachedLargeGridLongRunScrollingStaysBoundedWithoutResizeChurn",
        "TestAttachedHostFirstPartialPaintPresentsFullAfterCreateAndResize",
        "TestAttachedHostRecoversAfterSimulatedDeviceLoss"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect from the full test-file review. One performance-specific graph scenario is environment-gated by DXUI_GRAPH_PERF=1; ordinary graph rendering and pixel coverage are separately registered. No tests were run as part of this review.",
      "reviewNotes": {
        "callers": "Tests/Controls/DxUi.Tests.Runner.cpp declares and invokes RunRenderingTests; this file registers the rendering group through DXUI_RUN_TEST. It also includes Controls.Tests.GridMultilineRendering.h, whose tests run in the same group.",
        "ownershipAndLifetime": "The attached-host cases use AttachedHostWindow; the relevant helper destructor calls _host.Detach() before host-window teardown. Grid tests cover cache release when hidden, detached, remodeled, moved between hosts, or no longer visible, as well as device-loss recovery.",
        "failureAndCancellation": "The test group exercises bounded shaping of oversized text, clipping, device loss, and first partial presentation after create/resize. No cancellation behavior is applicable in this rendering test file; device loss is simulated and verified through host recovery cases.",
        "testCoverage": "Coverage spans cache identity/invalidation, multiline clamping and ellipsis, RTL/emoji/grapheme/surrogate handling, theme/DPI changes, transient surfaces and high contrast, graph bands, large attached-grid visible-work bounds, same-size redraw without resize, resize flush, and device loss. The DXUI_GRAPH_PERF-gated hue-churn measurement is optional; functional graph pixel/band tests remain independent. This is source review only; none of these tests were executed here."
      },
      "plannedSha256": "475f4313f36544c2f363b0e39cf08001b4ab945a37e2ba9eeabbe4533cba68f1",
      "reviewEvidence": "review-general-controls-final-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Runner.cpp",
      "sha256": "dee55732f3452b9d6adf999f98a11089d027301ccafe238d5214df24f7baaedb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestWatchdogSelfTestBlocksForever",
        "RunWatchdogSelfTestFixture",
        "RunWatchdogSelfTestProgress",
        "RunTooltipTests",
        "RunAnimationTests",
        "RunInteractiveLeaseTests"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "wmain is the Controls executable entrypoint and dispatches suite functions declared in this file. RunDxUiTestFilter records exact test-name matches in shared helper; Tools invoke CLI switches for targeted runs, watchdog probes and artifact output.",
        "ownershipAndLifetime": "Clipboard test adapter is stack-owned and globally installed only inside wmain, with scope_exit clearing the borrowed global. Perf capture, activation guard, foreground thief, watchdog timeout and fixture deadline have scoped owners. The runner calls dispatcher Shutdown after each suite.",
        "failureAndCancellation": "CLI parsing validates identifiers/numbers and rejects unknown switches, incompatible no-activate/focus combinations and watchdog switches with suite/test filters. Watchdog deadlines wrap each named case or fixture. Test failure exits are routed away from dialogs. Fixture suite MenuExitLifetime intentionally returns process exit through CRT path.",
        "testCoverage": "Runner explicitly dispatches suite registrations including Controls, InteractiveLease, Tooltip, Animation, Rendering and NativeTextInput, and rejects unmatched test names so name-filtered runs cannot pass empty. Source review only; no executable was run. No issue confirmed."
      },
      "plannedSha256": "dee55732f3452b9d6adf999f98a11089d027301ccafe238d5214df24f7baaedb",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.TextField.cpp",
      "sha256": "f8fea659ccb3ad32ae1a5f9e6cc7db8a0b9975d85d2e0a1f3499f822246c7723",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunTextFieldTests",
        "DxUi.Tests.TextField suite"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "e4826402bbc874c08ef57bb54fdbb05b04b074f17e209ff310078fd1e68f350b",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Theme.cpp",
      "sha256": "d6160e78a88f4d29307aa289fa8a08836dd9d4df2559739a708078573b35a2cb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "TestChooseContrastingTextColorUsesMeasuredContrast",
        "TestInactiveSelectionTextUsesThePaintedGroundAndKeepsHighContrastPairs",
        "TestThemeColorsPaletteKeepsDistinctReadableAlertTones",
        "TestThemeColorsPaletteFallsBackForUnsuppliedAlertColors",
        "TestPrimaryButtonContrastPreservesSystemColorPairs",
        "TestComboBoxVariantsExposeDistinctChromeContracts",
        "TestGridCellChromeFollowsViewerDerivedResolvedCellVisuals",
        "TestLabelTypographyOverrideValidatesAtomically"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect from the full test-file review. The suite contains both formula-level palette assertions and outcome-level checks for contrast, alpha compositing, high-contrast system pairs, distinct alerts, invalid-input preservation, and selected grid-cell colors. No tests were run as part of this review.",
      "reviewNotes": {
        "callers": "Tests/Controls/DxUi.Tests.Runner.cpp declares RunThemeTests and dispatches the Theme suite to it at line 514. This file registers 96 named DXUI_RUN_TEST cases in RunThemeTests.",
        "ownershipAndLifetime": "Most cases exercise pure theme/palette/style resolvers. A smaller set constructs stack-owned Label, Button, ComboBox, WindowHost, Grid, and model fixtures; the grid cases keep models alive through their use and inspect the debug visual-state projection before locals destruct.",
        "failureAndCancellation": "The tests use finite local inputs and synchronous assertions; there is no cancellation or asynchronous work in this file. Invalid label typography and invalid button preview values are checked for atomic preservation of the previous accepted value.",
        "testCoverage": "The file covers color conversion/hash/blending, contrast and alpha composition, light/dark/high-contrast palettes, alert fallback/derivation, disabled/focused/hovered/pressed/toggled control styles, Grid/Tree/ComboBox chrome, labels, preview validation, and ProgressBar/Toolbar style resolvers. Some tests assert exact resolver formulas, while independent properties such as measured WCAG ratio >=4.5, opaque high-contrast colors, preserved custom colors, and distinct alert tones add result-level checks. This is source review only; none of these tests were executed here."
      },
      "plannedSha256": "d6160e78a88f4d29307aa289fa8a08836dd9d4df2559739a708078573b35a2cb",
      "reviewEvidence": "review-general-controls-final-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Tooltip.cpp",
      "sha256": "c9a258a29508a27eac35bb6606870f114f48ae3a8377cc8a0fe4aa6091d42ed4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestTooltipInspectionScrollsWithoutChangingFocusOrValue",
        "TestTooltipInspectionOutsideDismissalConsumesWholeGesture",
        "TestTooltipInspectionCaptureCallbackWinsReentrantOpen",
        "TestTooltipLayerHideDelayExpiresAfterTimerTicks",
        "TestTooltipDeadlinesUseCurrentDispatcherClockAfterIdleHostTick",
        "TestPassiveSupplementalTooltipUsesHostHitTestingDelayLifetimeAndClickThrough"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "RunTooltipTests is called by DxUi.Tests.Runner.cpp. Tooltip cases drive WindowHost::SetTooltip/InspectTooltip, input message routing, Grid/Tree hover, and AnimationDispatcher ticks; real attached windows use no-activate setup.",
        "ownershipAndLifetime": "AttachedHostWindow owns native windows; controls live in Panel-owned trees and test models outlive borrowed references. ReentrantTooltipCaptureControl intentionally opens a newer inspection from OnCaptureLost and verifies latest callback state wins. DispatcherTicks unsubscribes on scope exit.",
        "failureAndCancellation": "Tests cover exact hide/show deadlines, movement cancellation, superseded reentrant inspection, click-through dismissal consuming the releasing gesture, Escape and pointer-drag scroll, stale target replacement, tiny viewport clamping and same-cell tracking. Message dispatch uses bounded 10-second deadline; no build/runtime validation performed.",
        "testCoverage": "Eighteen named cases cover placement, wrapping, clipping, persistent inspection navigation, focus/value retention, reentrancy, stale dispatcher clocks, hover tracking and passive target lifetime. Source review found no definite defect."
      },
      "plannedSha256": "c9a258a29508a27eac35bb6606870f114f48ae3a8377cc8a0fe4aa6091d42ed4",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Tree.cpp",
      "sha256": "6e37b5f8c7137217908b44f4e371616421219584fbe2ded21b1e5aae680d38c8",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "TestTreeSelectionAndReconciliationStopAfterModelGetterReplacesModel",
        "TestTreeSelectionDelegateReplacingModelStopsOuterNotifications",
        "TestTreeRowsStraddlingTheViewportEdgesPaintOnlyInsideIt",
        "TestTreeSingleClickCaptureTransferMayRetireTheTree",
        "TestTreeCtrlAndShiftClickThroughTheHostMessages",
        "TestTreeSelectAllStopsWhenSelectionCallbackReplacesItsDelegate",
        "TestTreeSelectionSetCallbackMayDestroyTheTree",
        "TestTreeMultiSelectionSurvivesModelChangesExpandCollapseAndScrolling",
        "TestTreeExpansionAnimatesWhenTheDelegatePublishes"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect from the full test-file review. Tree tests register through the standard suite runner and have named coverage for model/delegate reentrancy, object retirement, selection reconciliation, pointer/keyboard behavior, clipping, capture, expansion, and rendering. No tests were run as part of this review.",
      "reviewNotes": {
        "callers": "Tests/Controls/DxUi.Tests.Runner.cpp declares RunTreeTests and dispatches the Tree suite at line 559. RunTreeTests registers the test cases in this file, including the reentrancy, capture, selection, geometry and expansion cases listed above.",
        "ownershipAndLifetime": "The fixtures keep mutable models and delegates alive alongside the Tree that borrows them. Tests deliberately replace a WindowHost root or model/delegate during callbacks and use lifetime tokens or post-callback state assertions to detect stale access. TestTreeSingleClickCaptureTransferMayRetireTheTree uses a scope guard that releases capture only if one of its two owned test HWNDs still owns it.",
        "failureAndCancellation": "Coverage checks injected model-getter exceptions preserve committed selection, model replacement aborts stale outer operations, callback-driven root/delegate retirement stops later notifications, and row drags cancel through Escape, capture loss, removed rows, or incompatible selection gestures. No asynchronous cancellation path is exercised by this file.",
        "testCoverage": "The suite exercises single/multi selection and visible-order ranges, callbacks and UIA-style requests, keyboard typeahead/navigation, expand/collapse, scrolling/wheel accumulation, scrollbar hover/drag, RTL expander hit targets, tooltip clipping, row/adornment layout, drag-reorder subtree constraints, and model changes. It also has bitmap checks for clipping rows at viewport boundaries and selected-row fill pixels. This is source review only; none of these tests were executed here."
      },
      "plannedSha256": "27da2d3bde8c8d226f998baca35b918fd58b50e8c48e5861836e6e3749376bf8",
      "reviewEvidence": "review-general-controls-final-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.WindowHost.cpp",
      "sha256": "88406d96395137ea565a5a0bc38abd626fd96a0bc9a2e16aa1c1a4bc661bafed",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "44dc726ecf7a6b22e40b3dcd7a854c3693f3f67acebe9342793c03edc2597d2e",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.FormAccessibility.cpp",
      "sha256": "968a1bd376182b4a669fee0d4e34c89895bd4843849adcb48b67891c49cf9a4a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestFormAccessibilityWindowPropertiesAndStableEditor",
        "TestFormAccessibilityEmbeddedPropertiesAndStableEditor",
        "TestFormAccessibilityWindowLabelLifetimeAndReplacement",
        "TestFormAccessibilityEmbeddedLabelLifetimeAndReplacement",
        "TestFormAccessibilityWindowPresentationEventsAreIdempotent",
        "TestFormAccessibilityEmbeddedPresentationEventsAreIdempotent",
        "TestFormAccessibilityWindowPublicationStopsAfterDestructiveCallback",
        "TestFormAccessibilityEmbeddedPublicationStopsAfterDestructiveCallback",
        "TestFormAccessibilityWindowPublicationStopsAfterEditorRetirement",
        "TestFormAccessibilityEmbeddedPublicationStopsAfterEditorRetirement"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Public test entrypoints are declared in Controls.Tests.FormAccessibility.h and called by control test registration; embedded variants use the application EmbeddedUiaTest::Bridge.",
        "ownershipAndLifetime": "FormFixture owns window/view/bridge and root tree; COM providers use wil::com_ptr_nothrow, embedded cases run on a dedicated STA jthread with activation blocker, and event hooks/subscriptions are cleared via scope guards/destructors.",
        "failureAndCancellation": "Validates malformed validation states/text bounds are rejected without replacing last good UIA snapshot; weak external label lifetime/replacement/detach retires providers; destructive synchronous notification reentrancy stops later events; detached retained editor actions return UIA_E_ELEMENTNOTAVAILABLE.",
        "testCoverage": "Window and embedded cases assert labeled semantic editor metadata, required/valid/help/status, reflow-stable identity and draft/selection, label hide/detach/replacement expiry, event idempotence, and root/editor/retained-editor/hidden-field destructive callback ordering. Embedded queue-time assertions allow at most the already attempted event; not a proof for arbitrary asynchronous timing."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.FormAccessibility.cpp",
      "plannedSha256": "1d9a05932d3600edd22c0afb5accb946b541d6a7d2ffa67c279258ec5e3e592e",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.FormAccessibility.h",
      "sha256": "bea70d975efbf0bce2f76d999c34b6b78d176b46809af8061068cea507f14d01",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestFormAccessibilityWindowLabelLifetimeAndReplacement",
        "TestFormAccessibilityEmbeddedLabelLifetimeAndReplacement",
        "TestFormAccessibilityWindowPublicationStopsAfterDestructiveCallback",
        "TestFormAccessibilityEmbeddedPublicationStopsAfterEditorRetirement"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares ten native and embedded Form accessibility cases registered by the Controls test runner; bodies live in the form-accessibility test suite.",
        "testIds": [
          "TestFormAccessibilityWindowLabelLifetimeAndReplacement",
          "TestFormAccessibilityEmbeddedLabelLifetimeAndReplacement",
          "TestFormAccessibilityWindowPublicationStopsAfterDestructiveCallback",
          "TestFormAccessibilityEmbeddedPublicationStopsAfterEditorRetirement"
        ],
        "failureAndCancellation": "The case set contains destructive callback and editor-retirement publication stops plus idempotent presentation events; no failure logic is implemented in this header.",
        "ownershipAndLifetime": "Declaration-only file; case names specifically describe stable editor and label lifetime across replacement and retirement.",
        "testCoverage": "Representative named cases are TestFormAccessibilityWindowLabelLifetimeAndReplacement, TestFormAccessibilityEmbeddedLabelLifetimeAndReplacement, TestFormAccessibilityWindowPublicationStopsAfterDestructiveCallback and TestFormAccessibilityEmbeddedPublicationStopsAfterEditorRetirement. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.FormAccessibility.h",
      "plannedSha256": "bea70d975efbf0bce2f76d999c34b6b78d176b46809af8061068cea507f14d01",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.Forms.cpp",
      "sha256": "603e19ac3d8f3eaf83b28a5a1aecf88413f97ac3e3919c801092bef84e5071ab",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestFormsMeasureIsPureAndSharedLabelCacheIsStable",
        "TestFormsLocalizedGeometryMatrix",
        "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
        "TestFormsFractionalTranslationFailsBeforeAnyChildGeometry",
        "TestFormsIntrinsicEditorMinimaDriveNarrowContentAndCache",
        "TestFormsMeasurementCallbacksRetireOrSupersedeSafely",
        "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
        "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus",
        "TestFormsNativePreparationAndFactoryCoverage"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
      "reviewNotes": {
        "callers": "RunFormLayoutTests is registered by the control runner; public FormPanel/FormField, ControlHost and EmbeddedHost APIs are exercised.",
        "ownershipAndLifetime": "Unique_ptr control trees and GraphicsFixture/GraphicsDevice RAII own resources. Embedded input fixtures stay on-thread; callbacks use external state and lifetime tokens for retirement.",
        "failureAndCancellation": "Tests invalid policies/dimensions, shaping budgets, arithmetic overflow, nonconvergent minima, failed measurement, and reentrant detach/clear/reparent/focus/bounds changes; prior or newer-authored geometry must survive.",
        "testCoverage": "Localized DPI/density/RTL/columns, intrinsic minima, fractional widths, editor/part ownership, reentrant layout, embedded composition/selection/focus/cancel, native preparation. Runtime execution not claimed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.Forms.cpp",
      "plannedSha256": "09b92f152bc515ca99daec0b87ddc3fb12c2cf60ce062bab73b948b4794cc854",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.Forms.h",
      "sha256": "800b49c94d28a7b9dc806a9773341b25494d7753d596a2c9cbaf8c7980f9c08c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunFormLayoutTests",
        "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
        "TestFormsMeasurementCallbacksRetireOrSupersedeSafely",
        "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
        "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares RunFormLayoutTests(), the Forms suite entry called by the Controls test runner; implementation and test registration live in Controls.Tests.Forms.cpp.",
        "testIds": [
          "RunFormLayoutTests",
          "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
          "TestFormsMeasurementCallbacksRetireOrSupersedeSafely",
          "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
          "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus"
        ],
        "failureAndCancellation": "No behavior is implemented here. Runtime fixture failures, cancellation, and COM/resource lifetime are covered by the implementation suite, not this declaration.",
        "ownershipAndLifetime": "This declaration-only header owns no state and adds no runtime dependency; include placement is within the Controls test project.",
        "testCoverage": "The entry routes to cases including TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry, TestFormsMeasurementCallbacksRetireOrSupersedeSafely, TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations and TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.Forms.h",
      "plannedSha256": "800b49c94d28a7b9dc806a9773341b25494d7753d596a2c9cbaf8c7980f9c08c",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridUiaPattern.cpp",
      "sha256": "2cea15a1752dde483401d54fd586cab9ef42a11a0b628cf7b48eac4a5bb99e14",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestGridUiaPatternSupportsBoundedOffscreenGetItem",
        "TestGridUiaPatternStopsWhenCellReadReplacesItsControl",
        "TestGridUiaPatternStopsWhenCellReadShrinksItsModel",
        "TestGridUiaPatternRetriesSupersededSameAddressCapture",
        "TestGridUiaPatternStopsWhenCellReadNotifiesSameModel",
        "TestGridUiaPatternPreservesAssignmentDuringCachedCellCreation",
        "TestGridUiaPatternPreservesAssignmentDuringRowNavigation",
        "TestGridUiaPatternContainsModelExceptionsAtTheProviderBoundary",
        "TestGridUiaPatternFollowsTheColumnDisplayOrder",
        "TestGridUiaPatternScrollsAnOffscreenItemIntoView",
        "TestGridUiaPatternFocusesACellOfAScrolledOutColumn"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
      "reviewNotes": {
        "callers": "Grid UIA test entrypoints are registered by the control runner; they acquire WindowHost UIA root and call COM Grid/GridItem/Table/ScrollItem providers, including a foreign UIA client apartment.",
        "ownershipAndLifetime": "Grid model, AttachedHostWindow, providers, events and worker threads are fixture-owned; COM pointers/SafeArrays use WIL/scope guards. Foreign client is bounded while owner pumps the hidden nonactivating window; cleanup releases gate and joins.",
        "failureAndCancellation": "Injects row shrink, same-address reassignment, replacement/notification callbacks during reads, standard exceptions and bad_alloc at COM boundary, stale-row deletion and first-publication failure. Stale providers return UIA_E_ELEMENTNOTAVAILABLE; exception outputs remain null.",
        "testCoverage": "Covers offscreen GetItem bounds/read budget, foreign-client access, cache eviction, navigation/runtime IDs, model revision races/retry, error recovery, deleted/unreadable rows, display-order headers, WM_GETOBJECT, scroll-into-view and focus of scrolled-out columns. No stress/fuzzing claim."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.GridUiaPattern.cpp",
      "plannedSha256": "5c7fa0233ed32055015acb53bae47fdbfcd261b84fdcbaee1758bf89821f02ed",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridUiaPattern.h",
      "sha256": "e36c23ba67b23a5fcb757200360a87cdd455055a5e81b04b559b16e3517882fd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestGridUiaPatternSupportsBoundedOffscreenGetItem",
        "TestGridUiaPatternStopsWhenCellReadReplacesItsControl",
        "TestGridUiaPatternContainsModelExceptionsAtTheProviderBoundary",
        "TestGridUiaPatternWindowAnswersWhenTheFirstPublicationFails"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares the Grid UIA pattern cases registered by the Controls runner; implementations are in the Grid UIA pattern suite.",
        "testIds": [
          "TestGridUiaPatternSupportsBoundedOffscreenGetItem",
          "TestGridUiaPatternStopsWhenCellReadReplacesItsControl",
          "TestGridUiaPatternContainsModelExceptionsAtTheProviderBoundary",
          "TestGridUiaPatternWindowAnswersWhenTheFirstPublicationFails"
        ],
        "failureAndCancellation": "The declarations include callback replacement, row shrink, model exception, superseded capture and failed publication scenarios; this header itself has no executable failure boundary.",
        "ownershipAndLifetime": "This declaration-only header owns no provider or model state; the declared cases target Grid provider snapshots and model/control replacement.",
        "testCoverage": "Exact named cases include TestGridUiaPatternSupportsBoundedOffscreenGetItem, TestGridUiaPatternStopsWhenCellReadReplacesItsControl, TestGridUiaPatternContainsModelExceptionsAtTheProviderBoundary and TestGridUiaPatternWindowAnswersWhenTheFirstPublicationFails. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.GridUiaPattern.h",
      "plannedSha256": "2875f14647575d6423df34cd1449d6710220d33df0292726e6c85e28460e72df",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.HostControlRegressions.cpp",
      "sha256": "f3b2765a59ff7f3bd153d389a8a106e10130c9da5c713d5793bcf5edb2498227",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestPageHostWindowUiaTraversesInvokesAndRetiresPageChildren",
        "TestPageHostEmbeddedUiaTraversesInvokesAndRetiresPageChildren",
        "TestPageHostTabTraversalUsesCurrentLogicalChildren",
        "TestEmbeddedTransitionsUseApplicationClockIncludingZero",
        "TestNativeCaretTicksInvalidateOnlyWhenPixelsChange",
        "TestScrollPanelForwardsSecondaryPressInContentCoordinates",
        "TestScrollPanelModelContextMenusUseScreenCoordinates",
        "TestASecondButtonDuringADragLeavesTheDragToItsControl",
        "TestScrollPanelScrollbarDoubleClickPagesAgain",
        "TestScrollPanelKeepsTheFocusAChildTakesOnAnUnhandledPress",
        "TestKeyboardContextAnchorsStayInsideTheScrollViewport",
        "TestNativeUnhandledKeysReachApplicationFallback",
        "TestExtractedRadioDoesNotUseItsFormerGroup",
        "TestEmbeddedGraphicsRetirementPreservesApplicationBindings"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Public Test* entrypoints are declared in Controls.Tests.HostControlRegressions.h and registered in the control test runner; UIA cases use the shared UiaTest client and embedded bridge.",
        "ownershipAndLifetime": "WARP GraphicsFixture and GraphicsDevice pools are local RAII fixtures; embedded UIA runs on dedicated STA with activation blocker; WindowHost and AttachedHostWindow own their control trees/windows. Callback captures remain alive through synchronous message dispatch.",
        "failureAndCancellation": "Exercises current logical PageHost membership and retired providers, hidden/disabled focus traversal, scroll transform input, multi-button drag/capture, scrollbar repeated paging, keyboard-context viewport clamping, app key fallback filtering, extracted RadioButton independence, and borrowed graphics bindings after ReplaceDevice/Detach.",
        "testCoverage": "Focused regression tests cover page-tree UIA and invoke lifetime in both hosts, tab traversal, animation clock epochs including zero, caret invalidation only on phase changes, scroll context/menu anchors, drag/capture and focus preservation, fallback key filtering, radio extraction, and application graphics-state preservation; no asynchronous race/stress coverage asserted here."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.HostControlRegressions.cpp",
      "plannedSha256": "a44c1ae8965f50bb3689da2923e06a80c1683c4819de142cc8410ac4428902e3",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.HostControlRegressions.h",
      "sha256": "5fa384938d16484a7be87ac92b871265489324cfb8476dc049764dad59842d53",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestPageHostWindowUiaTraversesInvokesAndRetiresPageChildren",
        "TestPageHostEmbeddedUiaTraversesInvokesAndRetiresPageChildren",
        "TestEmbeddedTransitionsUseApplicationClockIncludingZero",
        "TestEmbeddedGraphicsRetirementPreservesApplicationBindings"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares focused Host/Control regression cases consumed by the Controls runner; implementations are in the host-control regression suite.",
        "testIds": [
          "TestPageHostWindowUiaTraversesInvokesAndRetiresPageChildren",
          "TestPageHostEmbeddedUiaTraversesInvokesAndRetiresPageChildren",
          "TestEmbeddedTransitionsUseApplicationClockIncludingZero",
          "TestEmbeddedGraphicsRetirementPreservesApplicationBindings"
        ],
        "failureAndCancellation": "The declaration list includes destructive publication callbacks and graphics retirement/rebinding scenarios, but contains no runtime handling itself.",
        "ownershipAndLifetime": "The header only declares functions. Its cases cover page/embedded accessibility children, focus, application clock, embedded graphics bindings, and pointer-coordinate forwarding.",
        "testCoverage": "Exact names include TestPageHostWindowUiaTraversesInvokesAndRetiresPageChildren, TestPageHostEmbeddedUiaTraversesInvokesAndRetiresPageChildren, TestEmbeddedTransitionsUseApplicationClockIncludingZero and TestEmbeddedGraphicsRetirementPreservesApplicationBindings. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.HostControlRegressions.h",
      "plannedSha256": "71cda21acdefbca35c3d69bd93238b44a8a989f221a53950698e547740741240",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.IntrinsicLayout.cpp",
      "sha256": "811a81402812b094fb7f3d62121e2cbca0b0be15ee4d73c6e993e41dbb6c585d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs",
        "TestIntrinsicLeavesShapeUnicodeAndKeepEditorsStable",
        "TestButtonContentFontSizeNativeGlyphPaintCacheAndGeneration",
        "TestButtonPreparedIconTextLayoutMatchesDrawTextW",
        "TestEmbeddedVisualPreparationRejectsReplacedDeviceAndRecovers",
        "TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry",
        "TestFlexBoundsCallbacksCannotPublishRetiredOrSupersededPasses",
        "TestFlexRepresentedEndpointsPreserveMinimaAndOrder",
        "TestIntrinsicPreparationIsSharedByNativeAndEmbeddedHosts",
        "TestIntrinsicReflowPreservesTextDraftAndCancelsMovedPress",
        "TestTextFieldFontRolePreservesEmbeddedCompositionAndUpdatesGeometry"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
      "reviewNotes": {
        "callers": "RunIntrinsicLayoutTests is registered by the control runner; fixtures use public controls, FlexPanel/FormPanel, WindowHost/EmbeddedHost and visual-preparation hooks.",
        "ownershipAndLifetime": "Control trees and GraphicsFixture/GraphicsDevice resources use RAII; STA threads isolate WARP host fixtures, activation blockers prevent focus, and callback state outlives hosts. Thread completion has a 30-second bound before join.",
        "failureAndCancellation": "Validates atomic outputs for invalid constraints, named/allocation exceptions, bounded shaping, explicit fallback, aggregate/arithmetic overflow, nonconvergent minima, reentrant retirement/reparent/focus/layout, and visual-cancellation/device-replacement/hide-show wakes.",
        "testCoverage": "34 registered cases cover text/editor metrics, typography/rendering parity and cache, device recreation, callback cancellation, flex wrapping/RTL/overflow/rounding/ownership, native/embedded preparation, draft/composition/focus and stale-press cancellation. WARP parity checks do not establish a performance speedup."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.IntrinsicLayout.cpp",
      "plannedSha256": "71680a8a743b4d20081839c375f604621e59ee9b160b932f2ca5535727f8f0f7",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.IntrinsicLayout.h",
      "sha256": "168ac50dd72d8878c27d8d424f3c99b2708598a93e2253363788eb442246a71b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunIntrinsicLayoutTests",
        "TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs",
        "TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry",
        "TestEmbeddedVisualCancellationCanReenterDetach",
        "TestIntrinsicLayoutFailureKeepsPaintingThePreviousGeometry"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares RunIntrinsicLayoutTests(), which the Controls test runner invokes; the production assertions reside in Controls.Tests.IntrinsicLayout.cpp.",
        "testIds": [
          "RunIntrinsicLayoutTests",
          "TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs",
          "TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry",
          "TestEmbeddedVisualCancellationCanReenterDetach",
          "TestIntrinsicLayoutFailureKeepsPaintingThePreviousGeometry"
        ],
        "failureAndCancellation": "No failure path is implemented by this header. The included suite contains explicit callback-retirement, failure-containment, preparation-cancel, and previous-geometry retention coverage.",
        "ownershipAndLifetime": "Declaration-only forwarding header; no runtime state or ownership is defined.",
        "testCoverage": "Representative implementation cases are TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs, TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry, TestEmbeddedVisualCancellationCanReenterDetach and TestIntrinsicLayoutFailureKeepsPaintingThePreviousGeometry. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.IntrinsicLayout.h",
      "plannedSha256": "168ac50dd72d8878c27d8d424f3c99b2708598a93e2253363788eb442246a71b",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.MarkdownCorpus.h",
      "sha256": "d617f056df524bfc6eb76bbe990c7c83a5b594143e730c5d39f940f27861fc02",
      "category": "tests",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Generated static fixture reviewed via format, provenance, consumer/oracle integration and complete case manifest. The Markdown header has an explicit entry-by-entry manual interpretation gap noted in testCoverage; no confirmed runtime defect.",
      "reviewNotes": {
        "callers": "This is generated data only; Controls.Tests.RichMarkdown and Controls.Tests.RichSemanticDocument iterate the CommonMark/GFM source, block expectations and entity table. Generator inputs are SHA-pinned; its full corpus counts are 652 CommonMark, 672 GFM and 2,125 entity values.",
        "ownershipAndLifetime": "The header consists of constexpr string views/records with no runtime ownership. The generator emits UTF-16 escapes to preserve exact source units.",
        "failureAndCancellation": "Source-input integrity is pinned and the generator rejects incomplete corpus counts. Entry-level output semantics are checked by the downstream corpus test suite; I did not execute it.",
        "testCoverage": "Controls.Tests.RichMarkdown (RunRichMarkdownTests, corpus and entity cases), Controls.Tests.RichSemanticDocument; Tools/Generate-MarkdownTestData.ps1 and Test-MarkdownData.ps1 own deterministic generation/check. Review limitation: I checked section boundaries, consumers, source pins/count invariants and generated representation; I did not manually interpret all 2,125 raw entries."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.MarkdownCorpus.h",
      "plannedSha256": "d617f056df524bfc6eb76bbe990c7c83a5b594143e730c5d39f940f27861fc02",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.MenuPopupReadiness.cpp",
      "sha256": "4ab8ad203c162808716480093e241899c111bb1e78e6aa790f8f0cfadfd5c027",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
        "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Implements the readiness helper regression called by the Controls runner; it creates synthetic owned STATIC windows on a private desktop to exercise the helper.",
        "testIds": [
          "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
          "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
        ],
        "failureAndCancellation": "It proves hidden initialization HWNDs are rejected even with matching owner/class, accepts the same popup after show, and rejects it again after hide. Setup failures surface through Require; no test was run here.",
        "ownershipAndLifetime": "The fixture creates an isolated desktop and thread-owned HWNDs using wil::unique_hdesk/unique_hwnd; the test never switches the input desktop or takes foreground focus.",
        "testCoverage": "Exact case: TestMenuPopupDiscoveryRejectsHiddenInitializationWindows; the related late-popup failure cleanup case is TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.MenuPopupReadiness.cpp",
      "plannedSha256": "fa4604b87da37f426f93c3c03b4f2a2e9f5e0f90d7e2449231c1ba753c2e7d59",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.MenuPopupReadiness.h",
      "sha256": "86413cc4ad57cb5e363e93ba449a2fdd170bce38d3f732339bc13dd994a09260",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
        "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "The readiness and dismissal helpers are consumed by menu tests and setup drivers; the declared regression is implemented in Controls.Tests.MenuPopupReadiness.cpp.",
        "testIds": [
          "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
          "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
        ],
        "failureAndCancellation": "WaitForOwnedContextMenuPopupWindow has a finite timeout; DismissOwnedContextMenuPopupOnce uses SendMessageTimeout and waits briefly for destruction. The failure guard keeps dismissing late-created menus for a bounded eight seconds when a driver fails.",
        "ownershipAndLifetime": "Discovery requires a same-process, visible popup whose GW_OWNER matches the requested owner. The helpers close only discovered owned popups and bound repeated dismissal attempts; DismissMenusIfDriverFails ties cleanup to the driver's failure string.",
        "testCoverage": "Direct case TestMenuPopupDiscoveryRejectsHiddenInitializationWindows validates hidden-to-visible readiness on a private desktop. TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu exercises late popup cleanup. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.MenuPopupReadiness.h",
      "plannedSha256": "86413cc4ad57cb5e363e93ba449a2fdd170bce38d3f732339bc13dd994a09260",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.NativeDevicePolicy.h",
      "sha256": "9f39b7b78d703eedde6ed348a4521c730e3709dab5566fdd8244e3895b9061fb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "The test runner invokes TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss, defined inline in this header; it runs on a worker initialized for COM and constructs WARP-backed host fixtures.",
        "testIds": [
          "TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss"
        ],
        "failureAndCancellation": "The test captures worker exceptions in exception_ptr and rethrows after join. It checks conflicting attach policy is refused without losing attachment accounting, then verifies device-loss recovery retains WARP and releases the final attachment.",
        "ownershipAndLifetime": "The test owns a GraphicsFixture, shared GraphicsDevice pool, two AttachedHostWindow instances, and a worker thread. WIL scope_exit balances CoUninitialize, and both hosts detach before the worker exits.",
        "testCoverage": "Exact case: TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss. This reviewer read the case but did not execute it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.NativeDevicePolicy.h",
      "plannedSha256": "9f39b7b78d703eedde6ed348a4521c730e3709dab5566fdd8244e3895b9061fb",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.PreparedTreeAccessibilityEvents.h",
      "sha256": "dabba04d347eb9799999a5f45d70a6227c76550597f4626189cab2eca105e52f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The harness runs a real UIA client on an MTA jthread and exercises callbacks through the native ControlHost provider.",
        "ownershipAndLifetime": "COM/event objects use WIL smart pointers and events; callback entry uses mutex-protected admission counts and teardown unsubscribes then drains accepted callbacks.",
        "failureAndCancellation": "Timeout gates bound reentrant notifications and teardown; root replacement and burst replacement assert retained source generations retire.",
        "testCoverage": "TestPreparedTreeSourceReplacementNotifiesReentrantNativeClient, TestPreparedTreeSourceBurstRetiresIntermediateSourcesDuringNativeCallback, TestPreparedTreeInvalidCountFallsBackToCurrentModel, TestPreparedTreeCaptureCanRetireItsControl."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.PreparedTreeAccessibilityEvents.h",
      "plannedSha256": "dabba04d347eb9799999a5f45d70a6227c76550597f4626189cab2eca105e52f",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocument.cpp",
      "sha256": "427e475aeb8d6deca7436fbe29acab885d78713e019cc6f15519acb0dedc5b44",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite exercises Document and SourceSnapshot value APIs plus source replacement helper.",
        "ownershipAndLifetime": "Snapshots are held across edits/resets and concurrency test captures immutable values.",
        "failureAndCancellation": "Rejections use named standard exceptions and assert unchanged source/revision/history.",
        "testCoverage": "Tests cover Unicode/CRLF bounds, UTF-8, randomized edit oracles, history, persistent split/merge, maximum fragmentation and borrowed input. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocument.cpp",
      "plannedSha256": "f28fe9a52e9a11570c8ffe3487f879a7b9e8e41e0c4a38d68ce3cd84f32c1bbf",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocument.h",
      "sha256": "06bc81affee80252523e07198cf1c4b1ae4b7a9d84c6e9b5b8d13957bce7075a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichDocument.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocument.h",
      "plannedSha256": "06bc81affee80252523e07198cf1c4b1ae4b7a9d84c6e9b5b8d13957bce7075a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocumentLayout.cpp",
      "sha256": "6773e332e2db89769de31e777d453938674451b5e6365dc224e0066255d7038f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout is compared against flat PreparedLayout and raw DirectWrite observations; wrapper callers also compare actual ControlHost rendering.",
        "ownershipAndLifetime": "WIL COM wrappers own DWrite/D2D interfaces, while test-created spans and vectors own probe/raster buffers for call duration.",
        "failureAndCancellation": "Independent geometry, cancellation, resource-loss, range/cap and atomic-publication cases cover the retained chunk pipeline.",
        "testCoverage": "TestRichDocumentLayoutWholeGeometryOracle, TestRichDocumentLayoutWarpRasterOracle (108 exact WARP image pairs), TestRichDocumentLayoutCooperativeBudgetsAndGeometry, TestRichDocumentLayoutCoalescingCancellationAndFailure."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocumentLayout.cpp",
      "plannedSha256": "b175d0780ae6d487403aa7cf9ffcf42f7573e41234ce23b3679d9747e915003a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocumentLayout.h",
      "sha256": "2a2518e1d6f1486081d92b9c7c65585e9ae35b012f2291d25fb6de1488024aee",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "RunRichDocumentLayoutTests is registered by the Controls test runner and implemented in the paired .cpp file.",
        "ownershipAndLifetime": "Declaration-only header; no owned state.",
        "failureAndCancellation": "No independent failure path at the declaration boundary.",
        "testCoverage": "Exact test names and coverage are recorded on Controls.Tests.RichDocumentLayout.cpp."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocumentLayout.h",
      "plannedSha256": "2a2518e1d6f1486081d92b9c7c65585e9ae35b012f2291d25fb6de1488024aee",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichMarkdown.cpp",
      "sha256": "71e5ebdb70750020ba821e96a687763e72c3d2d19edbdd0f497403b31b1d78aa",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite routes to independent block/inline/capacity/HTML/footnote tests and native flowchart fixtures.",
        "ownershipAndLifetime": "Published syntax and parser snapshots are retained immutably across cancellation, revision and profile changes.",
        "failureAndCancellation": "Every parsing scenario checks exact partition/source bounds and unsupported inputs remain literal or receive bounded diagnostics.",
        "testCoverage": "Tests cover full CommonMark/GFM corpora, semantic expectations, extension identity, nesting budgets, native flowchart model/capacity and footnotes. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichMarkdown.cpp",
      "plannedSha256": "9e5ce69e0126010bf7f33738c31330e0b7844f17f14293e7986f9ae62a62b304",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichMarkdown.h",
      "sha256": "c57c2df2a3dbae9068f1c75989ededb0417727d6354dc93df361682f9ef147bd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichMarkdown.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichMarkdown.h",
      "plannedSha256": "c57c2df2a3dbae9068f1c75989ededb0417727d6354dc93df361682f9ef147bd",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichProjectionBlocks.cpp",
      "sha256": "6a5448a7d3a1eb940e39ed94f5a38a7c6ccbc8e8e3f6876752141de31ea0be3b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichProjectionBlocks"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "PreparedProjection is compared against full Project output across source and rich modes.",
        "ownershipAndLifetime": "Held immutable blocks and clones are checked after edits, clear, moves, and sibling revisions.",
        "failureAndCancellation": "Invalid projections/ranges reject and preserve prior publication; block overflow rebuilds or falls back coherently.",
        "testCoverage": "Coverage includes complete ACP mapping, source ranges/styles, line joins, Unicode edits, capacity and fixed descriptive trials. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichProjectionBlocks.cpp",
      "plannedSha256": "d665404ab5469f1ff7d839debe2fc946aa58611b44b70a8356031b9323b086b4",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichProjectionBlocks.h",
      "sha256": "aff2d33a58e4ee190f725d0a45aa74f9c191757f91cf839e211db7d7ab1a0fa3",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichProjectionBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichProjectionBlocks.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichProjectionBlocks.h",
      "plannedSha256": "aff2d33a58e4ee190f725d0a45aa74f9c191757f91cf839e211db7d7ab1a0fa3",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemantic.cpp",
      "sha256": "8f499fa3c17c34dce130a1d81807d9aa9ec09a0c9709873aa7b9285e3d7ce256",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Native semantic entrypoints invoke focused parser, layout and editor command suites.",
        "ownershipAndLifetime": "Test helpers own local prepared objects and release them at case boundaries.",
        "failureAndCancellation": "Failure paths assert stable source/output expectations rather than silently skipping.",
        "testCoverage": "Relevant suites include native structural commands, family grammar and independent layout goldens. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemantic.cpp",
      "plannedSha256": "3b28a5fc696c861e226b6a0291f638d23881aa8b806eaefcbef8c35bc9cc7b68",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemantic.h",
      "sha256": "20b62e004bb09d02a3a055f235922dced1c14e519236dcca0ce7370b9fb05d3e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemantic.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemantic.h",
      "plannedSha256": "20b62e004bb09d02a3a055f235922dced1c14e519236dcca0ce7370b9fb05d3e",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticDocument.cpp",
      "sha256": "7829b897c99b27b26dd455f1ff9fe1c63c7c4fc7cfbac06d4de6cb1cdf7b2e34",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite calls parser, semantic builder, native math/diagram prepare paths and semantic fixtures.",
        "ownershipAndLifetime": "Model expectations are value-based; parser publications and retained documents remain independently held.",
        "failureAndCancellation": "Rejected syntax checks exact source fallback and owned diagnostics; native test helper budgets are bounded where explicit.",
        "testCoverage": "Coverage includes math parser/layout, Mermaid grammar, semantic corpus, front matter/footnotes, source ownership, raster and source-region cases. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticDocument.cpp",
      "plannedSha256": "f0edce5c4f2cfb2a7c6481e1d809ef76028c52f5ede32b23a8c62363b78001bc",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticDocument.h",
      "sha256": "0132c82758ed7a5ba3753936e08cc62c693e73e4d4bd77ccba2af5a675ea6b72",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemanticDocument.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticDocument.h",
      "plannedSha256": "0132c82758ed7a5ba3753936e08cc62c693e73e4d4bd77ccba2af5a675ea6b72",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticGeometry.cpp",
      "sha256": "0ecfd9f38105257c102a876be4fe008aeedacbceaa079fe5eb8c9b018dcd883b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite composes semantic geometry/editor fixtures through native and embedded host helpers.",
        "ownershipAndLifetime": "Host/editor fixtures are scoped around each scenario and prepared objects are released with their owners.",
        "failureAndCancellation": "Fallback ownership test rejects arbitrary/partial source intervals while retaining the prior publication; rendering-admission failure has explicit suite coverage.",
        "testCoverage": "Named tests cover geometry, carets, links/HTML, task markers, footnotes, Mermaid objects and failure containment. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticGeometry.cpp",
      "plannedSha256": "feb3d8abd25e9968183b3bb8311814557810bc499eb11bef7fed16e2cadee002",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticGeometry.h",
      "sha256": "e9aadf9681b4914d8777c9a95e22247e3fc684aae2f27742b100cfbaf241f635",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemanticGeometry.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticGeometry.h",
      "plannedSha256": "e9aadf9681b4914d8777c9a95e22247e3fc684aae2f27742b100cfbaf241f635",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSourceChanges.cpp",
      "sha256": "75573cb9d41cf6dc30349b7a835ee8b4c6dc3ea79bf3dd44236770f0032e3a2a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "SourceSnapshot::ChangedRangeFrom is exercised after Document edits, undo/redo, transient previews and branch copies.",
        "ownershipAndLifetime": "Snapshots and expected flat strings are value-owned through concurrent reads.",
        "failureAndCancellation": "Every interval is checked for bounded scalar/CRLF edges and exact prefix/replacement/suffix reconstruction.",
        "testCoverage": "Tests cover recent-window tracking, expiry to full replacement, reset, random Unicode, moved/default snapshots. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSourceChanges.cpp",
      "plannedSha256": "f710c841ac5d5be9288c646d5a7af5baf1458861fd5a5d261281d6efc6373b51",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSourceChanges.h",
      "sha256": "dd68969d800689b9e06ac859dade916d76eebe393ef1a58e851671767830217f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSourceChanges.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSourceChanges.h",
      "plannedSha256": "dd68969d800689b9e06ac859dade916d76eebe393ef1a58e851671767830217f",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextAccessibility.cpp",
      "sha256": "5afdd72e02c955a910af4e6b930f7fb1d90eafb3840b532db4e030bf4e189e36",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Window and embedded providers share scenario helpers with explicit backend parameter.",
        "ownershipAndLifetime": "UIA provider references are reacquired around document revisions and range operations.",
        "failureAndCancellation": "Failure cases exercise native/literal Math fallback without changing source range ownership.",
        "testCoverage": "Coverage includes provider type/name/help text and exact source ranges; parent combined-14 reports EditorControls/Embedded pass. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextAccessibility.cpp",
      "plannedSha256": "02d65dcdab6de60f85571263e181a1ee26f7ba67027e5a20d4dcde4b0f48d5f2",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextAccessibility.h",
      "sha256": "51e75ae7a64a18386718b4a69c56fac5e8713b299a2cb92d34e4b9c8aad82c1d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextAccessibility.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextAccessibility.h",
      "plannedSha256": "51e75ae7a64a18386718b4a69c56fac5e8713b299a2cb92d34e4b9c8aad82c1d",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextEditor.cpp",
      "sha256": "09937127179152c141dec628daab70ad3413190de618c5af924bb4066d71525f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Tests exercise RichTextEditor through ControlHost/native messages, text input service imports, clipboard services and Markdown semantic preparation.",
        "ownershipAndLifetime": "Control root owns editors; COM/resource/request state uses WIL/shared ownership, and callbacks deliberately cover reentrancy, replacement, root retirement and host rebinding.",
        "failureAndCancellation": "Coverage includes source/selection/history atomicity, transient TSF preview/commit, bounded cooperative preparation, stale completion, fallback, cancellation and remote-image opt-in.",
        "testCoverage": "TestRichTextEditorCommittedTextThenSelectionCallbacks, TestRichTextEditorNativeCooperativePreparation, TestRichTextEditorNativeFlowchartRenderingAndFallbacks, TestRichTextEditorRemoteImagePolicy; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextEditor.cpp",
      "plannedSha256": "faefe84ac44d668a652e27cf578bcba4d9db283ea058cc12bdab8da805d6b62a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextEditor.h",
      "sha256": "f7eba1c7d65e589e81a146d1c8312f7e1548c0e695835fa019d0d511cf70dd64",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextEditor.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextEditor.h",
      "plannedSha256": "f7eba1c7d65e589e81a146d1c8312f7e1548c0e695835fa019d0d511cf70dd64",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextInput.cpp",
      "sha256": "9291a288f02b17bd9c5d5c8105a45eaf16f55405b724a3908932af5a20a3ab60",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextInput"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Exercises production TextInputServices, EmbeddedHost, RichTextEditor and application RichImageLoader paths.",
        "ownershipAndLifetime": "WIL handles, scope guards, service/client objects and request values are scoped through completion/teardown.",
        "failureAndCancellation": "Stale revision, read-only, active composition, cancellation, queue overflow, rebinding and disconnected-provider paths assert fail-closed behavior.",
        "testCoverage": "Suite includes clipboard/image source transactions, IMM preview/result, mapped geometry, composition, and image loader lifecycle. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextInput.cpp",
      "plannedSha256": "b4d791fc2ae231e171da656923dde6a6503a79c5014bb1a7549e2e7eadf36d56",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextInput.h",
      "sha256": "16c8bb007e1af52583f3203e3fe3b02830e77ae41c849c42fbc427d6c9924bb9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextInput"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextInput.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextInput.h",
      "plannedSha256": "16c8bb007e1af52583f3203e3fe3b02830e77ae41c849c42fbc427d6c9924bb9",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextLayout.cpp",
      "sha256": "c4387b284e2cc2efea7c68b2227cf34ce11bd20694639ddee5f6372e980169df",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite drives PreparedLayout, Unicode navigation, source-ink preparation and prepared flowchart raster helpers.",
        "ownershipAndLifetime": "DirectWrite/Direct2D objects use WIL owners; prepared copies are tested against shared mutable drawing effects.",
        "failureAndCancellation": "Malformed projections/options reject while prior geometry remains coherent; capacity paths keep source ink unpublished.",
        "testCoverage": "Tests cover DPI-independent geometry, Unicode grapheme stops, typography, link/source ink, borrowed targets and flowchart raster/budget. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextLayout.cpp",
      "plannedSha256": "56efd06b5f4e2a39752dc8fea46f92098529a813eed6097eb40706d6f40ed029",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextLayout.h",
      "sha256": "193d05a45396851b88b45a0b77124d7af53130d3f02641901e11a2046f0b42ce",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextLayout.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextLayout.h",
      "plannedSha256": "193d05a45396851b88b45a0b77124d7af53130d3f02641901e11a2046f0b42ce",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextElementMovement.cpp",
      "sha256": "ec5045b6b4033482ed957fc8c9d0a3cfe59ac2df373c45cadbe18a5d4d31b247",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestUnicode17BulkCharacterMovementOracle",
        "TestTextElementBulkMovementFixedTrials",
        "TestTextElementBulkMovementMaximumSourceAndMalformedBounds",
        "TestTextFieldInteriorAcpDeletion",
        "TestMultilineFieldInteriorAcpDeletion",
        "TestComboBoxInteriorAcpDeletion",
        "TestUnicode17CollapsedDeletionCorpus",
        "TestTextElementSingleBackwardFixedTrials"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Implements RunTextElementMovementTests() via the Controls test runner; uses the independent UnicodeGraphemeCorpus and production movement/deletion helpers plus test TextField/ComboBox inputs.",
        "testIds": [
          "TestUnicode17BulkCharacterMovementOracle",
          "TestTextElementBulkMovementFixedTrials",
          "TestTextElementBulkMovementMaximumSourceAndMalformedBounds",
          "TestTextFieldInteriorAcpDeletion",
          "TestMultilineFieldInteriorAcpDeletion",
          "TestComboBoxInteriorAcpDeletion",
          "TestUnicode17CollapsedDeletionCorpus",
          "TestTextElementSingleBackwardFixedTrials"
        ],
        "failureAndCancellation": "The suite checks extreme signed counts and maximum source bounds, malformed legacy code units, interior ACP deletion, and rollback through undo. Timing observations are printed but do not assert a platform threshold.",
        "ownershipAndLifetime": "Corpus vectors and input fixtures are local to each case. The file's performance loops report timings but do not retain global state; TextField/ComboBox cases check undo restores original text/caret.",
        "testCoverage": "Exact cases: TestUnicode17BulkCharacterMovementOracle, TestTextElementBulkMovementFixedTrials, TestTextElementBulkMovementMaximumSourceAndMalformedBounds, TestTextFieldInteriorAcpDeletion, TestMultilineFieldInteriorAcpDeletion, TestComboBoxInteriorAcpDeletion, TestUnicode17CollapsedDeletionCorpus and TestTextElementSingleBackwardFixedTrials. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.TextElementMovement.cpp",
      "plannedSha256": "08042ed9f3a5d08b40d491cf165b4f9eb30c57abadfc9c3c5f39c237f59f8c6f",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextElementMovement.h",
      "sha256": "3bd8b34daa5f3fe3a02556e69a85a46f0ee6767664b5a313397d640f02b93c76",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunTextElementMovementTests",
        "TestUnicode17BulkCharacterMovementOracle",
        "TestUnicode17CollapsedDeletionCorpus",
        "TestTextFieldInteriorAcpDeletion",
        "TestComboBoxInteriorAcpDeletion"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares RunTextElementMovementTests(), called by the Controls test runner and implemented in Controls.Tests.TextElementMovement.cpp.",
        "testIds": [
          "RunTextElementMovementTests",
          "TestUnicode17BulkCharacterMovementOracle",
          "TestUnicode17CollapsedDeletionCorpus",
          "TestTextFieldInteriorAcpDeletion",
          "TestComboBoxInteriorAcpDeletion"
        ],
        "failureAndCancellation": "No runtime behavior in this header; implementation checks bounded progress, signed count saturation, malformed UTF-16, and reversible editing.",
        "ownershipAndLifetime": "Declaration-only test entrypoint; no state is owned.",
        "testCoverage": "Implementation cases include TestUnicode17BulkCharacterMovementOracle, TestUnicode17CollapsedDeletionCorpus, TestTextFieldInteriorAcpDeletion and TestComboBoxInteriorAcpDeletion. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.TextElementMovement.h",
      "plannedSha256": "3bd8b34daa5f3fe3a02556e69a85a46f0ee6767664b5a313397d640f02b93c76",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextElements.cpp",
      "sha256": "7878220931232e57fd05ea7b119fc45cb357622e021a144b23a833c024de9d06",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestUnicode17DefaultGraphemeCorpus",
        "TestTextElementBoundsMalformedAndLongContext"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Implements RunTextElementTests() and is registered in the Controls runner; exercises the shared text-element boundary APIs against UnicodeGraphemeCorpus.",
        "testIds": [
          "TestUnicode17DefaultGraphemeCorpus",
          "TestTextElementBoundsMalformedAndLongContext"
        ],
        "failureAndCancellation": "Checks every UTF-16 offset for next/previous movement, nearest-boundary caret snapping, full 766-case corpus count, malformed inputs, long extender context and forward progress. No cancellation path applies.",
        "ownershipAndLifetime": "All corpus strings and expected boundaries are borrowed per-case data; malformed and embedded-NUL cases use explicit-length std::wstring values.",
        "testCoverage": "Exact cases: TestUnicode17DefaultGraphemeCorpus and TestTextElementBoundsMalformedAndLongContext. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.TextElements.cpp",
      "plannedSha256": "3569f184f605ac0e3b4231a50be91e43640caf844e45df0e2e3f6232eba5593d",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextElements.h",
      "sha256": "f8b224508bb27808d648bf58c158424c6252fc68c38f222bcb41d3ed0e49304a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunTextElementTests",
        "TestUnicode17DefaultGraphemeCorpus",
        "TestTextElementBoundsMalformedAndLongContext"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares RunTextElementTests(), invoked by the Controls test runner and defined in Controls.Tests.TextElements.cpp.",
        "testIds": [
          "RunTextElementTests",
          "TestUnicode17DefaultGraphemeCorpus",
          "TestTextElementBoundsMalformedAndLongContext"
        ],
        "failureAndCancellation": "No runtime failure or cancellation behavior is implemented here; the suite checks malformed and out-of-range legacy text inputs make bounded progress.",
        "ownershipAndLifetime": "Declaration-only test entrypoint with no owned state.",
        "testCoverage": "Implementation cases are TestUnicode17DefaultGraphemeCorpus and TestTextElementBoundsMalformedAndLongContext. No tests were executed."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.TextElements.h",
      "plannedSha256": "f8b224508bb27808d648bf58c158424c6252fc68c38f222bcb41d3ed0e49304a",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.UnicodeGraphemeCorpus.h",
      "sha256": "55ed4eff16a837bdc22e4390ddc67ec3e29fe6e22d50c8a61ab3915f3b1c52f4",
      "category": "tests",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Unicode grapheme corpus consumers",
        "Test-UnicodeGraphemeData.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.UnicodeGraphemeCorpus.h",
      "plannedSha256": "55ed4eff16a837bdc22e4390ddc67ec3e29fe6e22d50c8a61ab3915f3b1c52f4",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/DxUi.EmbeddedTests.vcxproj",
      "sha256": "fbbcba65d43ab27cb63dd762223355d2b335c314922846fdc3f6ba0e9e1f4927",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Embedded suite"
      ],
      "findingIds": [],
      "disposition": "No project wiring defect observed.",
      "reviewNotes": {
        "callers": "Standalone Embedded test executable referenced by solution/Tools native suite planning; one translation unit Embedded.Tests.Embedded.cpp; references src/DxUi.vcxproj.",
        "ownershipAndLifetime": "Build input only; Debug/Release/ASan x64 and ARM64 configurations select v145 and Windows SDK 10.0.26100.0. /bigobj applies to the large harness translation unit.",
        "failureAndCancellation": "Build fails through MSBuild project reference/compiler/linker errors; no runtime cancellation or postbuild copies declared.",
        "testCoverage": "Embedded.Tests.Embedded.cpp registers function and retained-control scenarios; coverage is the standalone Embedded suite across declared platforms/configurations."
      },
      "plannedSha256": "fbbcba65d43ab27cb63dd762223355d2b335c314922846fdc3f6ba0e9e1f4927",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.BenchmarkMain.h",
      "sha256": "ec05a1004c8968625d2b6a4d8312d9fc6da4877aa308d94e57f6bc52b3d482b2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--benchmark",
        "--benchmark-multiline-grid",
        "--benchmark-retention",
        "--benchmark-multiline-grid-heap-paced",
        "RunFunctionalTests"
      ],
      "findingIds": [],
      "disposition": "No concrete issue found; unknown-argument fallthrough is a minor command-surface ambiguity.",
      "reviewNotes": {
        "callers": "Included by Embedded.Tests.Embedded.cpp as wmain; routes benchmark flags to ComplexUiBenchmark::Run and default invocation to RunFunctionalTests.",
        "ownershipAndLifetime": "Entry point initializes STA COM and uses WIL scope_exit for CoUninitialize; benchmark retention mode emits 60 separate output cycles.",
        "failureAndCancellation": "Unknown flags fall through to functional tests rather than returning usage failure; only fixed argv shapes select benchmark modes.",
        "testCoverage": "CLI branches cover baseline, multiline distinct/retention/heap/paced variants and functional test dispatch; this header is an entrypoint, not per-scenario assertion source."
      },
      "plannedSha256": "ec05a1004c8968625d2b6a4d8312d9fc6da4877aa308d94e57f6bc52b3d482b2",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.ComplexUiBenchmark.h",
      "sha256": "4126fdce2b80e8294a1ed880f963ec67d25546a1b321f6a1fd0c282b64c967f9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--benchmark",
        "--benchmark-multiline-grid",
        "--benchmark-retention",
        "--benchmark-multiline-grid-heap",
        "--benchmark-multiline-grid-heap-paced"
      ],
      "findingIds": [],
      "disposition": "No cause attribution from this harness alone; capture validates its own workload invariants and emits raw samples.",
      "reviewNotes": {
        "callers": "BenchmarkMain dispatches each CLI workload here; uses Samples/ComplexUi scene, GraphicsFixture WARP device, shared heap diagnostic and allocation counters.",
        "ownershipAndLifetime": "Function-local scene/device/staging resources use RAII; output stream owns JSON report. Timed clean/dirty loops count same-thread C++ allocations and sample private/working-set memory; optional retention mode repeats 6000 frames and samples every 200.",
        "failureAndCancellation": "HRESULT/check failures abort benchmark; report file state is checked at open and after close. Fixed process heap diagnostics are opt-in and explicitly reject partial heap accounting.",
        "testCoverage": "Scenarios assert no C++ allocations in clean/composition, dirty allocation ceilings, preparation deltas, hidden-view zero work, surface reuse, and retained resource samples; not a product perf comparison by itself."
      },
      "plannedSha256": "4126fdce2b80e8294a1ed880f963ec67d25546a1b321f6a1fd0c282b64c967f9",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.Embedded.cpp",
      "sha256": "e2aec932acecf38c55f8cff046fc779f4dcfefbb4afc476bc9f33ef9fcf717d0",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "RunFunctionalTests",
        "Embedded functional suite"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "9ad95b9ea93afa1ba7b9ac33aa4a6a847358b5b3de3e5bed0d407f860b6a7596",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedAccessibility.h",
      "sha256": "d3fb68121e9aadc3cd899825949d4540336596f75a905ceb423f493415b661fe",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestEmbeddedDisclosureAccessibility",
        "TestEmbeddedGridOffscreenGetItem",
        "TestEmbeddedTreeMultiSelect",
        "TestEmbeddedAccessibilityContainsAThrowingModelGetter",
        "TestEmbeddedAccessibility"
      ],
      "findingIds": [
        "F-EMBED-ACCESSIBILITY-SAFEARRAY-01"
      ],
      "disposition": "Actionable cleanup finding: returned SAFEARRAY values at selection readback and text rectangles use manual SafeArrayDestroy despite neighboring WIL SAFEARRAY RAII pattern; replace with unique_any in focused cleanup.",
      "reviewNotes": {
        "callers": "Included by Embedded.Tests.Embedded.cpp; top-level TestEmbeddedAccessibility invokes disclosure, offscreen Grid GetItem, multi-select Tree, throwing model, and core embedded UIA scenarios.",
        "ownershipAndLifetime": "EmbeddedScene owns view/device-bound controls; models and delegates outlive borrowed pointers; site root is cleared before detach. Foreign-client std::thread is joined; COM initialized/uninitialized in worker. Two returned SAFEARRAYs are manually SafeArrayDestroy-ed.",
        "failureAndCancellation": "Covers invalid placement/value, foreign-thread rejection, COM-marshaled action pumping, stale providers after hide/detach/root replacement, same-path control replacement, and throwing model getter recovery. Assertion failures use Check/Hr.",
        "testCoverage": "TestEmbeddedDisclosureAccessibility, TestEmbeddedGridOffscreenGetItem, TestEmbeddedTreeMultiSelect, TestEmbeddedAccessibilityContainsAThrowingModelGetter, and TestEmbeddedAccessibility are called by embedded functional suite."
      },
      "plannedSha256": "6fb50a12ca673c6101169ce33f0d14c830bc3e48d299e9ccb28eb3a5d9479b75",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedTextInput.h",
      "sha256": "2ffb25aaa65eb0273b51a26b1c0e61e895401703ef56a0d696349fb628a8694b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "2ffb25aaa65eb0273b51a26b1c0e61e895401703ef56a0d696349fb628a8694b",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedUia.h",
      "sha256": "e1f71b8741f449da36f3861a04eb28aafcc8fd7f599d9922d7dc819b707595dc",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "1462308f3cd2452ac77be0c8243c5533d13cb653ac227fca446b65cbc7f6732d",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedUiaBridge.h",
      "sha256": "183aff6d375d32f848c9cb9d5960790c6800e08fdce44b6f40c11cbd5171865c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestEmbeddedUiaEventHarness",
        "PreparedTreeEvents scenarios"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; ownership ordering is explicit and consumer harness exercises host bridge.",
      "reviewNotes": {
        "callers": "Bridge is consumed by Embedded.Tests.EmbeddedUia.h and Embedded.Tests.PreparedTreeEvents.h to adapt EmbeddedHost providers into a real application WM_GETOBJECT tree.",
        "ownershipAndLifetime": "Bridge owns HWND and WRL application provider with WIL/wil::com_ptr; ApplicationSite borrows ApplicationElement with bridge lifetime. Destructor disconnects view/site, clears root, disconnects UIA provider, then destroys HWND.",
        "failureAndCancellation": "Attach validates root/attachment; event pumping is bounded at 4096 messages per call. Provider uses COM threading and fixture clients pump owner STA; stale/detached provider behavior is asserted by consumers.",
        "testCoverage": "TestEmbeddedUiaEventHarness and prepared-tree event fixtures exercise application-parent/fragment-root bridge and marshaled UIA calls; UiaTestClient owns subscription teardown before bridge destruction."
      },
      "plannedSha256": "183aff6d375d32f848c9cb9d5960790c6800e08fdce44b6f40c11cbd5171865c",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.GridSelectionBenchmark.h",
      "sha256": "692201c512d8285ccf29f50a7b7041fa38aad9e371b46d1d2ae766cbead69a26",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--benchmark-grid-selection",
        "paint",
        "selectionCost",
        "membership",
        "retention",
        "mutators",
        "preserve",
        "complexUiScene"
      ],
      "findingIds": [],
      "disposition": "No concrete issue found; diagnostic reports carefully disclose metric boundaries and does not establish product-level performance by itself.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp routes --benchmark-grid-selection <report> [parts] to GridSelectionBenchmark::Run; uses public GridSelectionModel/Grid APIs and fixture-provided allocation counters.",
        "ownershipAndLifetime": "Benchmark owns synthetic row model, embedded view, WARP completion texture, query vectors and output stream; view precedes borrowed model destruction. Allocation hooks count thread-local plain C++ requests and documented live bytes, excluding aligned/COM/driver storage.",
        "failureAndCancellation": "Invalid part names fail before measurement; fixed nonzero row sizes avoid empty-vector indexing; HRESULT/check failures abort. Report is checked after close; no retry/cancel path.",
        "testCoverage": "Opt-in paint, selection-cost, membership, retention, mutator, preserve and default-scene diagnostics exercise several sizes, stable-id shapes and select/clear/preserve lifecycles; not implicitly run as functional suite."
      },
      "plannedSha256": "692201c512d8285ccf29f50a7b7041fa38aad9e371b46d1d2ae766cbead69a26",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.InteractionLayout.h",
      "sha256": "114a90815c9fd3650e247be5e838ac1c252f5f844e6e4e9d885bc99c37d35969",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestInteractionLayoutWithoutPaint"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; test guards input-only layout publication and reentrant mutation.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp invokes TestInteractionLayoutWithoutPaint(gpu); scenarios use EmbeddedHost input callback to probe interaction layout after layout changes.",
        "ownershipAndLifetime": "Each test owns EmbeddedScene, controls, and host; model stays alive for borrowed view references. Input callbacks may mutate the tree, and test-owned callbacks remain scoped to synchronous input delivery.",
        "failureAndCancellation": "Stale layout, wrong extent/DPI/device, failed paint, hidden/zero-size view, wrong-thread call, and callback-driven tree mutation fail closed or return retry; cancellation restores slider state.",
        "testCoverage": "TestInteractionLayoutWithoutPaint asserts interaction acknowledgment does not publish UIA/text geometry; cases exercise relocated clicks, hide, slider cancel, splitter capture, reentrant tree mutation, and invalidation guards."
      },
      "plannedSha256": "114a90815c9fd3650e247be5e838ac1c252f5f844e6e4e9d885bc99c37d35969",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.LocalizedLayout.h",
      "sha256": "6c21102d15b0826d7776b101c5aab2d7d95dfdb491145d501badec4ee2228922",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestLocalizedShortViewport",
        "TestLocalizedInvalidationAndSelectedDetail",
        "TestLocalizedStackedBodyClipping",
        "TestLocalizedCheckboxCaption",
        "TestRightToLeftCheckboxCaptionHugsIndicator",
        "TestRightToLeftTabTitleStartsAtTheRight"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; tests exercise actual retained drawing and hit geometry across DPI/viewport variants.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp invokes localized short-viewport, invalidation/selected-detail, stacked-body clipping, checkbox captions, RTL checkbox and RTL tab title cases.",
        "ownershipAndLifetime": "Each scenario owns EmbeddedScene/control tree; borrowed ComplexUiModel remains in scope longer than scene/view. Captures use fixture-owned GPU resources and local vectors.",
        "failureAndCancellation": "Hr/Check fail test on preparation, geometry, paint, UIA action, focus, or callback mismatch; each view detaches before borrowed model leaves scope.",
        "testCoverage": "Tests cover French wrapped/long labels, viewport/DPI/width reflow, selection/focus, hit testing, virtualization, clean composition allocation counts, clipping/UIA hit targets, and RTL raster placement."
      },
      "plannedSha256": "6c21102d15b0826d7776b101c5aab2d7d95dfdb491145d501badec4ee2228922",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityBenchmark.h",
      "sha256": "ad9d87bd9677c7d6b41dae0f21a7eedd1406ab17c5e4d552298f4fac08d9c4d0",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityBenchmark"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "The benchmark prepares identical source and controls before timing snapshot updates.",
        "ownershipAndLifetime": "Providers and fixture model stay alive through the measured and post-measurement accessibility checks.",
        "failureAndCancellation": "Detach explicitly tests stale provider behavior after measured phases.",
        "testCoverage": "Outputs descriptive count/allocation/timing samples without imposing unstable timing thresholds. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "ad9d87bd9677c7d6b41dae0f21a7eedd1406ab17c5e4d552298f4fac08d9c4d0",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityFixture.h",
      "sha256": "0494cf3af7ed90d4b52816e2983c565f952ac2a6c063de7c53db596579f7a8e7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityFixture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Synthetic 10,000-row model defines stable literal IDs and labels independently.",
        "ownershipAndLifetime": "Model owns row names and exposes only views during provider access.",
        "failureAndCancellation": "Out-of-range and missing IDs return empty results; counters are owner-thread scoped.",
        "testCoverage": "Consumed by large prepared-row fixture and semantic tests. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "0494cf3af7ed90d4b52816e2983c565f952ac2a6c063de7c53db596579f7a8e7",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityRowsFixture.h",
      "sha256": "016746cfa75fcdd3cbf1513ba7b0536caf47ac10cf70445df2490992102d8715",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityRowsFixture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Prepared row adapter copies fixture names before publication and exposes immutable views.",
        "ownershipAndLifetime": "Worker construction joins before prepared rows are exposed to controls/providers.",
        "failureAndCancellation": "Feature-guarded adapter avoids requiring unsupported public API in baseline builds.",
        "testCoverage": "Consumed by large tree accessibility benchmark. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "016746cfa75fcdd3cbf1513ba7b0536caf47ac10cf70445df2490992102d8715",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilitySemantics.h",
      "sha256": "78ec16b0f2f641f03ac0d3129d2beb7d0cbb3fb99894ecb74342dd01204f9289",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The embedded accessibility provider consumes immutable prepared Tree row snapshots attached to EmbeddedHost.",
        "ownershipAndLifetime": "Providers retain snapshot owners until root replacement/detach; test uses shared witness counters to verify snapshot lifetime exactly.",
        "failureAndCancellation": "Same-count same-id content replacement updates retained provider names, then replacing root/detaching returns ELEMENTNOTAVAILABLE safely.",
        "testCoverage": "TestPreparedTreeAccessibilitySemantics; comment explicitly excludes reentrant-client, large-count and G1 claims."
      },
      "plannedSha256": "78ec16b0f2f641f03ac0d3129d2beb7d0cbb3fb99894ecb74342dd01204f9289",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeEvents.h",
      "sha256": "e9066ca1dbeebe69d8bcc81467af2948302bcbbe46d8939074322dc9c47063ff",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "EmbeddedHost UIA bridge publishes property and structure events from prepared snapshots to real MTA client callbacks.",
        "ownershipAndLifetime": "Observer callback admission is mutex-protected and teardown closes admission, unsubscribes, drains accepted callbacks, then releases retained UIA elements and labels.",
        "failureAndCancellation": "Gated notification bursts assert intermediate snapshots release, latest property is coalesced, labels remain valid only while alive, and root replacement invalidates elements.",
        "testCoverage": "TestPreparedTreeSourceReplacementNotifiesReentrantEmbeddedClient, TestPreparedTreeSourceBurstRetiresIntermediateSourcesDuringEmbeddedCallback, TestPreparedTreePropertyBurstCoalescesWithoutRetainingSources, TestPreparedTreeLabeledByBurstCoalescesWithoutRetainingSources."
      },
      "plannedSha256": "e9066ca1dbeebe69d8bcc81467af2948302bcbbe46d8939074322dc9c47063ff",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeRootLifetime.h",
      "sha256": "8dff9321137b6c61536ad7b03f5e000f2f41a6c43c8cc1c568d61f9db69332f4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeRootLifetime"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Root-lifetime test replaces source while retaining root and old providers.",
        "ownershipAndLifetime": "Borrowed fixture root is cleared before teardown; COM ownership uses WIL holders.",
        "failureAndCancellation": "Checks detach invalidates retained provider and superseded source is released.",
        "testCoverage": "TestPreparedTreeRootDoesNotRetainSupersededSource. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "8dff9321137b6c61536ad7b03f5e000f2f41a6c43c8cc1c568d61f9db69332f4",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.ComplexUiBoundaries.h",
      "sha256": "f217c8c6864556b2f6a07c2fb08fef81942a6275a8722d045041e4bfdc031480",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestDevicePolicies",
        "--complex-ui-boundaries",
        "boundary clean probes"
      ],
      "findingIds": [],
      "disposition": "No acceptance claim or concrete defect; standalone diagnostic explicitly disclaims scheduler/GPU attribution.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp invokes TestDevicePolicies and dispatches optional `--complex-ui-boundaries` diagnostics; uses ComplexUiProfiling counters and the synthetic WARP scene.",
        "ownershipAndLifetime": "Eight Window probes are allocated once outside timing; fixed per-frame arrays retain QPC samples; D3D staging texture uses WIL COM ownership. Output JSON is written after timed probes.",
        "failureAndCancellation": "Invalid raster mode is rejected before device creation; HRESULT/check failures abort. Complete flag captures thread/process CPU query availability, and output labels diagnostic-only/non-GPU timing scope.",
        "testCoverage": "TestDevicePolicies checks modes, actual device flags, WARP output pixels; diagnostic run checks clean preparation/composition invariants, allocation-free probes, hidden surface release, and writes window samples."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.ComplexUiBoundaries.h",
      "plannedSha256": "31b2e90e8a0da446cf570eef896b199e50fc7996a09c5c4144ea6b0b402c5aae",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.ComplexUiProfiling.h",
      "sha256": "beee3e4c80fb9a6fc85c2d65f699405a82cd71838cdf7c07a5c6e0d3b285f61c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--benchmark-complex-ui-profile",
        "Default profile scenario",
        "MultilineGrid profile scenario"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; keep diagnostic and acceptance evidence separate.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp routes --benchmark-complex-ui-profile <out> Default|MultilineGrid Off|On to this diagnostic; profiling wrapper records stage and process/thread observations around the synthetic workload.",
        "ownershipAndLifetime": "Fixed-size stage/thread arrays and bounded counters are owned by the profiling context; sampling uses Win32 process/thread/heap queries and emits labeled diagnostic samples. It is opt-in and does not alter product code paths.",
        "failureAndCancellation": "Unavailable CPU/thread/process counters are represented as unavailable; invalid invocation is rejected by the CLI router. Profile output is diagnostic and bounded by wrapper timeout; no cancellation/acceptance contract.",
        "testCoverage": "The embedded runner dispatches the profile command and workload variants; profile output is diagnostic only, not a paired acceptance receipt or proof of an allocation cause."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.ComplexUiProfiling.h",
      "plannedSha256": "beee3e4c80fb9a6fc85c2d65f699405a82cd71838cdf7c07a5c6e0d3b285f61c",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.FormLayoutBenchmark.h",
      "sha256": "78af68623bad96ff47cf8a02839d9b787ce9f954de3f0db3ed6f5c95906594f2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--benchmark-form-layout",
        "CheckParity",
        "CheckPreflightGuards",
        "Lifecycle"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; diagnostic labels timing/allocation scopes and explicitly excludes physical removed-device qualification.",
      "reviewNotes": {
        "callers": "Embedded.Tests.Embedded.cpp routes --benchmark-form-layout <report> [seed] [blocks] to FormLayoutBenchmark::Run; compares public FormPanel layout with a manual leaf-layout baseline using public APIs.",
        "ownershipAndLifetime": "Fixture owns EmbeddedHost, root/control hierarchy, measurement caches and completion texture; borrowed model state and control lifetime tokens are scoped to the fixture. Output and graphics resources use RAII; per-frame allocation counters are switched off by scope_exit.",
        "failureAndCancellation": "Seed/block parsing is bounded and profile parity is checked before timing; hidden/pending composition, device-generation transitions, output stream completion, and target pixel completion are checked. No catch/retry; mismatch aborts diagnostic.",
        "testCoverage": "Pre-timing tests compare hierarchy, semantics, metadata, geometry and populated pixels across LTR/RTL, DPI/density, validation and width states; timing/lifecycle tracks warmup, unchanged, width, validation, hide/show, device replacement, root retirement and detach."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.FormLayoutBenchmark.h",
      "plannedSha256": "78af68623bad96ff47cf8a02839d9b787ce9f954de3f0db3ed6f5c95906594f2",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichDocumentStorageResources.h",
      "sha256": "acc5e0701101b22711ced325c2b26cc862d7d56dab1b6ba53a1f924b9230f686",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Allocation/retained-byte instrumentation wraps Document source replacement, undo/redo, transient metadata and source-change snapshots.",
        "ownershipAndLifetime": "The fixtures compare tracked live bytes and explicitly verify release to zero after scoped document/history destruction.",
        "failureAndCancellation": "Injected bad_alloc points assert source, selection, revision and full undo/redo history remain coherent; transient metadata and equality queries have bounded/no allocation checks.",
        "testCoverage": "TestRichDocumentFragmentedHistoryStorageBudget, TestRichRecordedWholeTextStorageBudget, TestRichDocumentAllocationFailureAtomicity, TestRichSourceChangeRolloverFailureAtomicity."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichDocumentStorageResources.h",
      "plannedSha256": "acc5e0701101b22711ced325c2b26cc862d7d56dab1b6ba53a1f924b9230f686",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichGeometryResources.h",
      "sha256": "e47e11ce969bca72996df2c3d94eb69f079eb33ee2d8c39cb3ef801fcc83df5e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The fixture drives PreparedLayout and PreparedDocumentLayout via supplied DirectWrite/WARP host APIs.",
        "ownershipAndLifetime": "Document snapshots and immutable projection values are owned; WIL owns COM factories; counters are scoped and restored.",
        "failureAndCancellation": "Injected allocation points preserve prior source/projection/geometry, retries succeed, max retained bytes and retirement are bounded; composition checks no allocations.",
        "testCoverage": "TestRichPreparedLayoutClearAllocationBudget, TestRichPreparedLayoutValueFailureAtomicity, TestRichDocumentGeometryFailureAtomicity, TestRichDocumentGeometryMaximumStorageBudget, TestRichDocumentGeometryReplacementPeakBudget."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichGeometryResources.h",
      "plannedSha256": "e47e11ce969bca72996df2c3d94eb69f079eb33ee2d8c39cb3ef801fcc83df5e",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichImageCapture.h",
      "sha256": "5fc76d8d2f5dbe6cde884f4af54ca7ac66d3b61aaffa1d3d7a0a097e3b19100a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.RichImageCapture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Harness capture uses deterministic invented paths and generated pixel fixtures.",
        "ownershipAndLifetime": "Image data is owned by RichImage and request objects remain scoped through completion.",
        "failureAndCancellation": "Missing-file path is explicitly retained as fallback and source/history are asserted unchanged.",
        "testCoverage": "CaptureRichImageSample exercises the actual embedded image sample. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichImageCapture.h",
      "plannedSha256": "5fc76d8d2f5dbe6cde884f4af54ca7ac66d3b61aaffa1d3d7a0a097e3b19100a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichImageFailures.h",
      "sha256": "0d1802fcdd7e32e8af39f124b6c369bded4518a15ee4369a6aa2eef7b856bf84",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "RichImageResources and prepared semantic geometry are exercised through bounded staged image admission and owner completion APIs.",
        "ownershipAndLifetime": "Caller-held request tokens and immutable pixel buffers are value/shared-owned; resource clear/control teardown cancels tokens.",
        "failureAndCancellation": "Allocation/decode/driver failures retain alternatives, preserve prior publication, honor limits and do not create idle retry loops.",
        "testCoverage": "Image resource allocation-failure and raster/driver failure cases in Embedded.Tests.RichImageFailures; reviewed source only, no test run."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichImageFailures.h",
      "plannedSha256": "0055810050e1fdf338fdac59062f812b0e1b56c557563cbff1b856cd430aec3c",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichProjectionResources.h",
      "sha256": "e3406c1d831e61d630326a0e4d434ecaa702cb4155e18081ca8469db3ddbf034",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Projection snapshot slicing and PreparedProjection consume immutable Document snapshots and source-change metadata.",
        "ownershipAndLifetime": "Expected and actual projections own source, maps, units and runs; counters are restored through scoped guards.",
        "failureAndCancellation": "Allocation sweeps cover slicing, preparation and flat projection while preserving prior published values; mapping queries and retirement are checked allocation-free.",
        "testCoverage": "TestRichProjectionSnapshotAllocationFailureAtomicity, TestRichPreparedProjectionFailureAtomicity, TestRichFlatProjectionFailureAtomicity."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichProjectionResources.h",
      "plannedSha256": "e3406c1d831e61d630326a0e4d434ecaa702cb4155e18081ca8469db3ddbf034",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticDocumentResources.h",
      "sha256": "174d82358b617ffc2e06ea344ccbf7486094cb49ac5c66f4e37a5329ee9c38e5",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "SemanticBuilder consumes published MarkdownSyntax and publishes immutable SemanticDocument values.",
        "ownershipAndLifetime": "Old/current syntax and semantic values are shared owners; failure fixtures retain old publication and inspect tracked live bytes.",
        "failureAndCancellation": "Allocation sweeps cover complete build and Begin admission; explicit retry, unchanged request coalescing and max-source retained-byte cap are asserted.",
        "testCoverage": "TestRichSemanticDocumentFailureAndStorageBudgets."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticDocumentResources.h",
      "plannedSha256": "e49adaa270b97ca4c20ab6aee31199b2a77265d282c895e58998f696606f350b",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticGeometryResources.h",
      "sha256": "14663ee4bdf2be80cbf4649cf051e32d5a360464634c37cfc90b879d278433de",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "PreparedSemanticGeometry is prepared from SemanticDocument plus layout options, while editor text-service queries consume its immutable geometry.",
        "ownershipAndLifetime": "Shared semantic/document inputs and WIL DirectWrite factory are retained during preparation; resource graph is explicitly cleared after allocation tracking.",
        "failureAndCancellation": "Query allocation failures return unavailable without source/history/geometry mutation; Begin allocation sweeps preserve prior publication and allow retry.",
        "testCoverage": "TestRichSemanticEditorQueryFailureBudgets, TestRichSemanticGeometryFailureBudgets (including native spacing and footnote modes)."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticGeometryResources.h",
      "plannedSha256": "b501f5d0ee57d73a30520263ae190c9578d679083ef94b2ecab064fc39c6a83f",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticResources.h",
      "sha256": "ec2b7a3004301404fce848f78300a8a82785d84ab43b344de8892542e81c0af2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Exercises semantic coordinate maps, Markdown parser/builder, semantic geometry and flowchart native preparation, plus real native/embedded UIA dispatch.",
        "ownershipAndLifetime": "Expected source/map values are separately constructed; immutable semantic documents and prepared resources retain shared ownership; UIA clients subscribe/unsubscribe and callback gates protect lifetime.",
        "failureAndCancellation": "Fault sweeps cover parser/builder/geometry/flowchart allocation failures, retry and pending preservation; UIA collection and owner-dispatch failures return HRESULTs; max map bytes and scalar no-allocation queries checked.",
        "testCoverage": "TestRichSemanticNativeDispatchFailureAtomicity, TestRichSemanticAccessibilityFailureAtomicity, TestRichMarkdownCommandHostFailureAtomicity, TestRichMarkdownCommandFailureAtomicity, TestRichSemanticFailureAndStorageBudgets."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticResources.h",
      "plannedSha256": "906ff556c1313bb54c2671121bcf341b44fb5afad706609fcfc18f317d1a6b1d",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichTextResources.h",
      "sha256": "b0396960cb3125aa7c13342c6fb532ca0dc97ea83e6635bf715aa110375b78a7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Broad supplied-device fixture integrates Markdown/parser, flowchart, semantic, rich-image, grapheme, text-input and retained editor behavior through EmbeddedHost.",
        "ownershipAndLifetime": "GraphicsDevice pools are shared owners, EmbeddedHost owns editor/control state, immutable syntax/semantic/image values retain explicit owners, and UIA callbacks use scoped detach/drain paths.",
        "failureAndCancellation": "Covers staged preparation failure/cancellation, source fallback and retention, semantic allocation sweeps, images, source/history, identity, pending input, no-activation shared views, Unicode capacity, UIA metadata limits and retry.",
        "testCoverage": "TestNativeFlowchartAllocationFailureAtomicity, TestRichMarkdownParserFailureAndStorageBudgets, TestEmbeddedFlowchartRenderingAndFallbacks, TestRichTextCooperativeSchedulingAndTextInput, TestRetainedRichTextUiaRangeAllocationBudget, TestUnicodeGraphemeForwardWalkAllocationBudget. Test code read only; no execution by this agent."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichTextResources.h",
      "plannedSha256": "78c6e3c4d533971fd6e367ad4ae2521ffa498531f0bdd33c97a51b6820ceb005",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/DxUi.FoundationTests.vcxproj",
      "sha256": "060c899500e304f2e5bee1514be8e66ace3f62a8d8c67d19c039be2e7890f3f7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Foundation test executable"
      ],
      "findingIds": [],
      "disposition": "No project wiring defect observed.",
      "reviewNotes": {
        "callers": "Foundation test project is included in the solution/native suite; compiles Foundation.Tests.Foundation.cpp and Foundation.Tests.PublicHelpers.cpp and references src/DxUi.vcxproj.",
        "ownershipAndLifetime": "Build metadata only; declares Debug/Release/ASan for x64 and ARM64 with v145 and Windows SDK 10.0.26100.",
        "failureAndCancellation": "Compiler/link failures fail the test target; configuration list does not add a separate runtime timeout or cancellation mechanism.",
        "testCoverage": "Builds the standalone Foundation runner for each declared profile; behavioral assertions reside in the two listed translation units."
      },
      "plannedSha256": "060c899500e304f2e5bee1514be8e66ace3f62a8d8c67d19c039be2e7890f3f7",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/Foundation.Tests.Foundation.cpp",
      "sha256": "2fb3abda3ff1305177b0e3262690910cc2e5528f4012933b99c729d27923656d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestPublicHelpers",
        "frame clock policy assertions",
        "motion policy assertions",
        "frame stage and metrics assertions",
        "hidden ASAN child probe"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; exact assertion IDs for inline policy checks are not separately registered.",
      "reviewNotes": {
        "callers": "Native suite launches Foundation test executable; main routes hidden CRT/ASAN child probes then runs public-helper, frame clock, motion policy, frame stage, and metrics assertions.",
        "ownershipAndLifetime": "Runner owns process-level CRT report routing and test invocation; hidden child mode is used to verify ASAN reporting without crashing the main suite.",
        "failureAndCancellation": "Check aggregates assertion failures into nonzero exit; hidden ASAN child failure is isolated and parent validates expected signal/diagnostic.",
        "testCoverage": "TestPublicHelpers and frame/motion metric checks run in the normal entrypoint; hidden ASAN child test exercises sanitizer boundary."
      },
      "plannedSha256": "2fb3abda3ff1305177b0e3262690910cc2e5528f4012933b99c729d27923656d",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/Foundation.Tests.PublicHelpers.cpp",
      "sha256": "e5812bda1c73aaa0afc2bca0277af3871e1fdf48bfea9aa524e4da95432cc82d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestPublicHelpers",
        "font availability cache invalidation"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; internal cache hook remains confined to test code.",
      "reviewNotes": {
        "callers": "Foundation.Tests.Foundation.cpp calls TestPublicHelpers; tests compile/link public helper imports and use isolated DirectWrite factories for font cache scenarios.",
        "ownershipAndLifetime": "Test-only menu host is noncopyable; DirectWrite factories and font handles use COM/WIL ownership. Internal cache access is limited to simulating a negative cache result.",
        "failureAndCancellation": "Assertions fail the public helper aggregate; factory creation and font lookup failures are reported, and cache invalidation test checks results after factory isolation.",
        "testCoverage": "TestPublicHelpers covers public consumer helper APIs and font availability cache invalidation across isolated factories."
      },
      "plannedSha256": "e5812bda1c73aaa0afc2bca0277af3871e1fdf48bfea9aa524e4da95432cc82d",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/DxUi.InteractiveLease.vcxproj",
      "sha256": "971abec0fab1092c3243a294df676e1049c70c92d5a8739895399a47e56ced7e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "InteractiveLease native executable"
      ],
      "findingIds": [],
      "disposition": "No project wiring defect observed.",
      "reviewNotes": {
        "callers": "InteractiveLease native runner is built by the solution and tooling suite; includes runner plus confirmation/banner fixtures and references DxUi without adding another native link dependency.",
        "ownershipAndLifetime": "Project wiring only; native executable owns its helper process and UI fixtures.",
        "failureAndCancellation": "Build errors fail project; runtime admission/timeout behavior is exercised by runner and scripts, not MSBuild.",
        "testCoverage": "Build target compiles InteractiveLease.Tests.Runner.cpp and referenced fixture headers for declared platform/configuration profiles."
      },
      "plannedSha256": "971abec0fab1092c3243a294df676e1049c70c92d5a8739895399a47e56ced7e",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.ChildProcess.h",
      "sha256": "bc9984220afefd501e35186bccab7f6f9c08897ccdaad9586b24461f14ce544c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "root",
      "testIds": [
        "SelfTest::ChildProcesses",
        "SelfTest::DescendantProcesses",
        "Tools/tests/Test-InteractiveLease.ps1"
      ],
      "findingIds": [
        "LEASE-JOB-ADMISSION",
        "LEASE-MEMBER-COMPLETION"
      ],
      "disposition": "reviewed-fixed-diagnostics-pass-final-profiles-pending",
      "reviewNotes": {
        "callers": "InteractiveLease Runner invokes RunChildProcess for each leased suite; its native self-test substitutes only owned job admission operations. EndContainedChildJob is private to this complete-job supervisor, distinct from direct-child-only tooling.",
        "ownershipAndLifetime": "WIL owns process/thread/job/log/stdio and each retained member handle. Assignment precedes Resume; capture verifies IsProcessInJob before retaining handles. Explicit teardown is attempted once; failure guard still requests job termination, and kill-on-close remains active. PID alone never authorizes termination.",
        "failureAndCancellation": "Create/configure/assign errors refuse the suspended child. One 5s deadline bounds member capture/waits; capture, wait and allocation errors fail verification. Timeout/interruption request job termination before desktop restoration. Membership races preclude an atomic whole-tree completion claim; exact retained handles are waited, accounting zero is only a drain indication.",
        "testCoverage": "Injected denied job stages fail before repair (combined-17); descendant normal-exit completion fails before retained-handle wait (combined-18); combined-19 native lease self-test passes 98/98 after repair. Later guard-only cleanup delta is fully source-reviewed; official Release build passes and final runtime profiles will rerun the self-test. Direct-child timeout/interruption, private desktop warning and confirmation cases are also registered."
      },
      "plannedSha256": "dec6f57e16ca012163fc0796d38d188b37eb141bbeb9c17ff6fa50511db9e167",
      "reviewEvidence": "Lead full current-file review and retained combined-17/18/19 diagnostics; final source guard delta reviewed."
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.Confirmation.h",
      "sha256": "f0b8570fea64a5feafd27eaedc668b5c3866ba8121fabdd03126f04bef89ce2a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "InteractiveLease.Tests.Runner.cpp::PrivateDesktop confirmation scenarios"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; hosted tests correctly disclose private desktop availability as a gate.",
      "reviewNotes": {
        "callers": "InteractiveLease.Tests.Runner.cpp PrivateDesktop() calls the confirmation flow with scripted and native TaskDialog scenarios; lease orchestration supplies the confirmation service.",
        "ownershipAndLifetime": "TaskDialog callback state is stack/context-owned for synchronous dialog lifetime; callback tracks cancellation, interruption, countdown, and scripted decision without retaining caller references.",
        "failureAndCancellation": "Default decision is Cancel unless approved auto-start applies; silent mode avoids sound; dialog unavailability is reported as a skipped hosted scenario. Footer formatting catches bad_alloc locally to preserve dialog fallback.",
        "testCoverage": "PrivateDesktop runner exercises Start/Cancel, timeout, interruption, auto-start, close, and clicked Start outcomes; hosted suite is gated on private desktop availability."
      },
      "plannedSha256": "f0b8570fea64a5feafd27eaedc668b5c3866ba8121fabdd03126f04bef89ce2a",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp",
      "sha256": "ea5693c4680fae44210ca288585290ad887f394546155cd308f47edd9ea32165",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "SelfTest::ChildProcesses",
        "SelfTest::DescendantProcesses",
        "SelfTest::HostedWarningInput",
        "SelfTest::WarningForegroundBetweenChildren",
        "SelfTest::SessionLease",
        "SelfTest::PrivateDesktop"
      ],
      "findingIds": [],
      "disposition": "No defect confirmed from source review; private desktop scenarios are environment-gated and must remain disclosed separately from headless self-test coverage.",
      "reviewNotes": {
        "callers": "Executable wmain routes --check, --run, --run-hosted and --self-test; Run delegates to Support.Tests.InteractiveLease::RunLease and Win32LeaseServices delegates to ChildProcess/Confirmation/WarningBanner helpers. SelfTest::Run invokes ChildProcesses, DescendantProcesses, hosted warning input, between-child recovery, session mutex, and private desktop scenarios.",
        "ownershipAndLifetime": "Win32LeaseServices owns mutex, desktop probe/backend, banner and process-local interrupt/finished events; lease orchestration releases session mutex and restores the desktop before result serialization. SelfTest owns a PID-named temp directory and child-process fixtures; descendant cleanup verifies PID plus creation time before any failure cleanup.",
        "failureAndCancellation": "CLI rejects unknown/missing arguments and gates hosted mode on exact environment markers before desktop inspection or result writes. Child tests cover job create/configure/assign refusal before Resume, success, timeout, interruption, missing executable, log-open failure, and contained descendant completion; dialog/warning tests may skip when private desktop unavailable. Console close waits at most 4.5s for lease restoration.",
        "testCoverage": "SelfTest::ChildProcesses and ::DescendantProcesses assert nonzero/success, timeout/interruption, denied containment setup, child output, and both normal-exit and timeout descendant termination; ::PrivateDesktop covers confirmation outcomes and warning patch restoration, with explicit skip when desktop creation is unavailable."
      },
      "plannedSha256": "d9716dbb3324f8aa86a1f497a3aa275595554dc8386558a1dd2a61822f540477",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.WarningBanner.h",
      "sha256": "18323d806f669703cea8c0ff3fde377cd326549cc465e75a2db983b43587119e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "InteractiveLease.Tests.Runner.cpp::PrivateDesktop warning banner and hosted patch cases"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; restoration and fail-closed handling are observable in the hosted integration harness.",
      "reviewNotes": {
        "callers": "InteractiveLease.Tests.Runner.cpp PrivateDesktop() composes banner with activation patch and validates click-through and hosted synthetic input behavior.",
        "ownershipAndLifetime": "Banner owns HWND, font, and region through WIL; hosted patch records original window styles/cursor state and restores on teardown; test callbacks are synchronous and scoped.",
        "failureAndCancellation": "Banner creation and patch validation fail closed; click is sent down then up with retry/message pumping, and restoration paths run on cancellation/failure. Redraw allocation failure is explicitly caught for retry.",
        "testCoverage": "PrivateDesktop scenarios exercise banner visibility, click-through, hosted patch ownership, cursor movement, input release and restoration; test may skip where private desktop unavailable."
      },
      "plannedSha256": "18323d806f669703cea8c0ff3fde377cd326549cc465e75a2db983b43587119e",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/native-test-files.json",
      "sha256": "fec8f0df8bf41bdea237b1c58803eaab5c9dc96fa952c23c11ae50c6c5971ca4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-NativeScope.ps1"
      ],
      "findingIds": [],
      "disposition": "Reviewed manifest and direct consumer. No concrete omission confirmed from the current manifest read.",
      "reviewNotes": {
        "callers": "Tools/ScopedTesting.psm1 Get-NativeSourceManifest reads this JSON and compares it with live native test inputs; Tests/README.md documents registration.",
        "ownershipAndLifetime": "Tracked static input manifest; no runtime allocation or mutable object lifetime.",
        "failureAndCancellation": "Missing/extra/duplicate declarations are rejected by manifest validation; policy has no cancellation state.",
        "testCoverage": "Test-NativeScope.ps1 exercises manifest discovery and declaration validation; the manifest is configuration, not a test case."
      },
      "plannedSha256": "01c3aca460d9325d324b4d7cfd8b0381294cd8dbd462f9338da729c64b269804",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.ClipboardFixture.h",
      "sha256": "10184b263a4c988d37d17192017004c3b452e00778c01320962ccbacf2804e54",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [],
      "findingIds": [],
      "disposition": "Fixture read complete; caller/test linkage remains a stated coverage gap.",
      "reviewNotes": {
        "callers": "Fixture installs clipboard test hooks and is consumed by native test cases; full caller list not yet verified.",
        "ownershipAndLifetime": "Fixture owns simulated clipboard state and uses WIL global-memory handles; hook selection is thread_local. Underlying DxUi test hook pointers are shared process state, so concurrent use would require serialization.",
        "failureAndCancellation": "Injected OS-operation failures flow through clipboard operations; RAII releases global memory at fixture teardown.",
        "testCoverage": "Focused clipboard caller inventory remains to verify; no test IDs asserted in this header."
      },
      "plannedPath": "Tests/Support/Scope.Tests.ClipboardFixture.h",
      "plannedSha256": "10184b263a4c988d37d17192017004c3b452e00778c01320962ccbacf2804e54",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.CountingWriteFactory.h",
      "sha256": "a81960fd1d83e77515c1ad2e4893a3cbca220295ef123f9c4902ab402a1e2eef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "PreparedFlowchartTests::BudgetAndLifecycle",
        "RichTextLayout",
        "Embedded"
      ],
      "findingIds": [],
      "disposition": "Reviewed; useful test seam with actual consumer coverage.",
      "reviewNotes": {
        "callers": "Support.Tests.PreparedFlowchart.h includes the wrapper; PreparedFlowchartTests::BudgetAndLifecycle and borrowed-target scenarios pass it through to PreparedFlowchart. RichTextLayout and Embedded suites invoke BudgetAndLifecycle.",
        "ownershipAndLifetime": "WRL RuntimeClass implements IDWriteFactory and owns the wrapped factory through wil::com_ptr. It only forwards factory methods and counts CreateTextFormat/CreateTextLayout calls.",
        "failureAndCancellation": "Injectable format HRESULT and failLayoutAt return failures through the actual SDK call surface, nulling output interfaces when applicable; no internal exception/cancel path.",
        "testCoverage": "Controls.Tests.RichTextLayout.cpp and Embedded.Tests.Embedded.cpp exercise actual intercepted SDK calls and injected format/layout failures through PreparedFlowchart."
      },
      "plannedPath": "Tests/Support/Scope.Tests.CountingWriteFactory.h",
      "plannedSha256": "a81960fd1d83e77515c1ad2e4893a3cbca220295ef123f9c4902ab402a1e2eef",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.FlowchartLayout.h",
      "sha256": "df668ea737758094caa33856784e07275552847db9e1d3c8a53f84d55914f38a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Corpus covers admitted source-reference graphs.",
        "ownershipAndLifetime": "Geometry pins rank, routes and typed relations.",
        "failureAndCancellation": "Fanout checks finite geometry, endpoint ports and resource bounds.",
        "testCoverage": "Invariants checks extents and endpoint boundary constraints. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedPath": "Tests/Support/Scope.Tests.FlowchartLayout.h",
      "plannedSha256": "a4df647191d043110e9215dff2c4c95f927198990a76f466f309aa39f63aa853",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.Footnotes.h",
      "sha256": "f131f2b60c54d8ce1c2166e023242925bf71e16c22ad550347188d592b044450",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestLosslessMarkdownFootnoteBlockGrammarAndProfileIsolation",
        "TestLosslessMarkdownFootnoteResolutionAndReachableNumbering",
        "TestLosslessMarkdownFootnoteBindingRevisionAndCancellation",
        "TestRichTextEditorNativeFootnoteResolutionAndSourceEdits",
        "TestEmbeddedFootnoteResolutionAndSourceEdits"
      ],
      "findingIds": [],
      "disposition": "Reviewed shared semantic corpus and caller coverage; no defect confirmed.",
      "reviewNotes": {
        "callers": "Included and consumed by Controls.Tests.RichMarkdown.cpp, Controls.Tests.RichSemanticDocument.cpp, Controls.Tests.RichTextEditor.cpp, and Embedded.Tests.RichTextResources.h.",
        "ownershipAndLifetime": "Inline fixtures describe immutable expected cases; parser completion advances with bounded work, and ExerciseNavigation/ExerciseSourceEdits accept editor/host callbacks without owning app state.",
        "failureAndCancellation": "Cases cover missing/duplicate/cyclic/unreachable footnotes, Unicode normalization boundaries, parser containers, stale syntax after edits, read-only/disabled navigation, and undo/redo; checks fail through caller-provided assertion function.",
        "testCoverage": "RichMarkdown parser tests, RichSemanticDocument source ownership/fallback tests, RichTextEditor source/navigation tests, and embedded source/navigation tests consume the shared corpus."
      },
      "plannedPath": "Tests/Support/Scope.Tests.Footnotes.h",
      "plannedSha256": "f131f2b60c54d8ce1c2166e023242925bf71e16c22ad550347188d592b044450",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownBlocks.h",
      "sha256": "b48e039f996317a7db4fbb45951cd39ea3d917c53385a713533ce564f684c727",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "CheckOriginalCorpus checks all 105 original outcomes, with case indices on mismatches.",
        "ownershipAndLifetime": "CheckFamilyGrammar uses independently authored normalized model and diagnostic expectations.",
        "failureAndCancellation": "CheckFanout checks stable Cartesian order, source ranges, caps and original cases 80/81.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownBlocks.h",
      "plannedSha256": "a8b91d491b5a8609ae1ddb3206780c1cbbbc7a70ee37d34eda456840c9838bff",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownCapacity.h",
      "sha256": "5ce29010056b90d2a55c69440244017a3afd59dc3aa0adedf56b00bd4506c88e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownCapacity.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownCapacity.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownCapacity.h",
      "plannedSha256": "5ce29010056b90d2a55c69440244017a3afd59dc3aa0adedf56b00bd4506c88e",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownHtml.h",
      "sha256": "a4d40be77823bbc132c571247cd85b49bbca369281f10dae4d7b102349149bac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownHtml.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownHtml.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownHtml.h",
      "plannedSha256": "a4d40be77823bbc132c571247cd85b49bbca369281f10dae4d7b102349149bac",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownInlines.h",
      "sha256": "9299d4177a333d3be31666b0ee12121dd0abbc27c483ce5b5ee16b91a3dce7db",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Shared fixture scenarios are consumed by Markdown parser/model tests and editor-level profile tests.",
        "ownershipAndLifetime": "Static expected strings and source snippets are constexpr views owned by the test binary.",
        "failureAndCancellation": "Cases compare independently authored inline syntax/output for profiles; no mutation or asynchronous state in this fixture.",
        "testCoverage": "Support.Tests.MarkdownInlines; Controls.Tests.RichMarkdown and Controls.Tests.RichTextEditor reference these scenarios."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownInlines.h",
      "plannedSha256": "b415afb134e3141f33f82efbc6d8514f22d9a95a82b8a99f76c78976eb59793d",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownSourceCorrelation.h",
      "sha256": "3fa85960dae726967a49772bbfe06f9e883647c9b2839619d8b3165eed324cef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownSourceCorrelation.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownSourceCorrelation.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownSourceCorrelation.h",
      "plannedSha256": "3fa85960dae726967a49772bbfe06f9e883647c9b2839619d8b3165eed324cef",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.NativeFlowchartCorpus.h",
      "sha256": "bfe2b2fb4f37cc72e4e4dd94c853ecdd3b533f573c013ed30f01a5d8ae12e9c3",
      "category": "tests",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Generated static fixture reviewed via format, provenance, consumer/oracle integration and complete case manifest. The Markdown header has an explicit entry-by-entry manual interpretation gap noted in testCoverage; no confirmed runtime defect.",
      "reviewNotes": {
        "callers": "Generated pinned Mermaid 11.4.1 fixture data feeds Support.Tests.MarkdownBlocks, Support.Tests.FlowchartLayout, Controls.Tests.RichMarkdown, and Embedded.Tests.RichTextResources.",
        "ownershipAndLifetime": "Fixture entries are static constexpr UTF-16 views and expected outcome/count/direction records; source documentation bytes remain separately pinned and generator-owned.",
        "failureAndCancellation": "Decoded a concise manifest of all 105 numbered original cases and enumerated 20 family rows: 16 prepared original cases, 89 unsupported original cases, 1 prepared state family case, and unsupported non-flowchart families with typed reasons. Fan-out originals 80/81 expect four edges. No tests executed by this agent.",
        "testCoverage": "Support.Tests.MarkdownBlocks (TestNativeFlowchartPinnedIndependentCorpus), Support.Tests.FlowchartLayout, Controls.Tests.RichMarkdown, Embedded.Tests.RichTextResources. Parent-run evidence read from .build/regression-20261024-red/combined-14/results.json and state-description-after.log: targeted original-corpus test, flowchart-allocation sweep, full EditorControls, and Embedded all report exitCode 0."
      },
      "plannedPath": "Tests/Support/Scope.Tests.NativeFlowchartCorpus.h",
      "plannedSha256": "01256cee713abb65697535c9601dfbc1a3eed3efd2c057bb27e61f28cdce2374",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PreparedFlowchart.h",
      "sha256": "2f95dbf492fe773a9859a1df7a379a3f2eeeea17590a763117031f712c750e85",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Coalescing covers same-instance intrinsic option reuse.",
        "ownershipAndLifetime": "Relations checks hollow inheritance and distinct association raster output.",
        "failureAndCancellation": "StadiumRaster checks curved capsule output.",
        "testCoverage": "Labels checks native measured label geometry, bidi and multiline behavior. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedPath": "Tests/Support/Scope.Tests.PreparedFlowchart.h",
      "plannedSha256": "ef41d64f31e6444476be45f9917feefa529bf6395ee98986ece4c998b157a095",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImage.h",
      "sha256": "1535e9a96e6a57675f9f3119305ae6f7e98385bde75f09b6106676973df3b171",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Image parser/model tests use independent expected source and dimensions.",
        "ownershipAndLifetime": "Fixture buffers own decoded image bytes for the duration of the case.",
        "failureAndCancellation": "Unsupported and malformed image input assert rejection without partial model.",
        "testCoverage": "Support.Tests.RichImage covers metadata and source normalization. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImage.h",
      "plannedSha256": "1535e9a96e6a57675f9f3119305ae6f7e98385bde75f09b6106676973df3b171",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImagePaths.h",
      "sha256": "ba1db2f67609d4fdce5d1a6b0ee79d49138eb661faa3b8beaeb42611ffc70634",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImagePaths"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Path cases independently cover local paths, URI forms and traversal policy.",
        "ownershipAndLifetime": "Resolver results are value-owned paths with bounded lengths.",
        "failureAndCancellation": "Invalid schemes, encodings and traversal cases are rejected.",
        "testCoverage": "Support.Tests.RichImagePaths covers path admission and canonicalization. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImagePaths.h",
      "plannedSha256": "ba1db2f67609d4fdce5d1a6b0ee79d49138eb661faa3b8beaeb42611ffc70634",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImageResources.h",
      "sha256": "6620340033708ffa3ea0ad56cf2cd36fef1f8c5562ebc417fed015bbdc3e1107",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Fixture constructs independent SemanticDocument values and exercises RichImageResources plus RichTextEditor image request APIs.",
        "ownershipAndLifetime": "Decoded RichImage payloads are immutable shared owners; request tokens are copied/moved to explicitly test cancellation/retirement.",
        "failureAndCancellation": "Covers admission ceilings, path deduplication, shared decoded-byte accounting, current/stale completions, source/base/mode/visibility changes, no retry loop and actual raster pixels.",
        "testCoverage": "RichImageResourceTests.Admission, ReadyPicture, Pixel, PatternPixels, Retained; called by Controls.Tests.RichTextEditor and embedded image tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImageResources.h",
      "plannedSha256": "165ce889c454ae5d8ff081f6cd8d4e6f691f16f63de0afd31abc0e3b3f52a745",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemantic.h",
      "sha256": "1fc507d8b00161724e5d94381a3c68bd0c78dbbb9fe68fdadf498941f86c2556",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Parser contract helpers create independent expected node/diagnostic values.",
        "ownershipAndLifetime": "Semantic input owns source ranges and model values; no pointer-lifetime shortcut is used.",
        "failureAndCancellation": "Unsupported branches assert retained source and diagnostic outcome.",
        "testCoverage": "Structural command model cases include selection/source/history expectations. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemantic.h",
      "plannedSha256": "462522bf82712089022591c08021ac887fbedd62b88562804d52940117983d75",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticDocument.h",
      "sha256": "aebe9e4b7b1fab8636f84a7842e2ddcfa658627798ffcb4956768251a9ef643c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.RichSemanticDocument.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.RichSemanticDocument.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticDocument.h",
      "plannedSha256": "1dca03e06e841512315efeb0e4b2fac2e317873a66575f0fa5837f1a3d44588a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticEditor.h",
      "sha256": "73545f3be04226082577e40b0f7886745141b8fc8b76fb280aaa57e37b5f2c0b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Editor command fixtures exercise full source-to-selection mutation paths.",
        "ownershipAndLifetime": "Test owners hold snapshots/history through undo and redo comparisons.",
        "failureAndCancellation": "Failure cases assert source and selection preservation after rejected commands.",
        "testCoverage": "Cases cover tables, lists, headings, quote/list Enter and inline-to-code conversions. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticEditor.h",
      "plannedSha256": "5074b6c728ecff4f004147f00154764605a192db8bf0fe8c657c9f2b2c614544",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticGeometry.h",
      "sha256": "2e3b7ce22ec8fba909e81713ea386cfa1eb6087d2e7cab6b1cbf3e0b1a5e8992",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Geometry expectations are independently normalized and finite.",
        "ownershipAndLifetime": "Fixtures own layout inputs and destroy prepared resources at case completion.",
        "failureAndCancellation": "Invalid geometry and resource limits check failure without partial publication.",
        "testCoverage": "Cases cover relation kind, endpoints, extents and routing bounds. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticGeometry.h",
      "plannedSha256": "ecf78951cbb5d7de36e582c11baa93ee87b321cc43b6a0871e19b12d8e6a3b08",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichTextStateReplacement.h",
      "sha256": "ff74dde6002acb577b05d4b28f56657e81b7a81bf8b935fd1fde9bd32c0eda1a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "State replacement fixture compares source before and after a scoped replacement.",
        "ownershipAndLifetime": "Selection and document revisions are captured as values.",
        "failureAndCancellation": "Rejected replacements preserve original content and history.",
        "testCoverage": "Tests target state replacement behavior independently of rendering. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichTextStateReplacement.h",
      "plannedSha256": "ff74dde6002acb577b05d4b28f56657e81b7a81bf8b935fd1fde9bd32c0eda1a",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SemanticDiagrams.h",
      "sha256": "14f5bd5e0d6b39c9f5b38d860b5b23b09c009dfccab4229ec849c1e27ec0df2c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticDiagrams"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Fixture scenarios distinguish accepted semantic fences from exact-source fallback.",
        "ownershipAndLifetime": "Normalized expectations include family, names and model relations.",
        "failureAndCancellation": "Fallback assertions check retained source and diagnostic classification.",
        "testCoverage": "Tests are consumed by semantic diagram and embedded resource suites. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.SemanticDiagrams.h",
      "plannedSha256": "e195d32e3a6d5ee51282a152dfcfb9d11f1fa499ca1924ae8be47dce0a3d5402",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SemanticSourceBlock.h",
      "sha256": "cd2bb9f219d0fdbb222b6c2fc1288d759201e746d44d2a37c737a856f69bf492",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticSourceBlock"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Fence integration tests distinguish ready native objects from literal source fallback.",
        "ownershipAndLifetime": "Fixture asserts exact prefixes, payload and suffix across the source range.",
        "failureAndCancellation": "Failure expectations preserve source and verify diagnostic/retained state.",
        "testCoverage": "Semantic source block and fallback tests cover Markdown embedding boundaries. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.SemanticSourceBlock.h",
      "plannedSha256": "b4aed4a08089232eab4b19afa512bb59e0459a88f1b5cbacee5c55ead51095dc",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.DesktopLease.h",
      "sha256": "2da04048ea0b49ef97d6b0f4696423e30c99073dba5fda30dd5ec3ce18689a8c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestDesktopLeaseLeavesAnUntouchedDesktopAlone",
        "TestDesktopLeaseDoesNotCaptureStaleFocusFromMinimizedForeground",
        "TestDesktopLeaseCaptureDistinguishesEveryNullFocusPath",
        "TestDesktopLeaseRejectsFailedFocusReadback",
        "TestDesktopLeaseReportsAForegroundItCannotRestore",
        "TestInteractiveDesktopCheckNamesEachReasonToRefuse",
        "TestInteractiveDesktopCheckStopsAtTheFirstReason"
      ],
      "findingIds": [],
      "disposition": "No defect confirmed; fake-backend coverage is broad, with hosted interactive restoration separately gated.",
      "reviewNotes": {
        "callers": "Support.Tests.InteractiveLease.h composes the lease; InteractiveLease runner calls CheckInteractiveDesktop for admission and teardown; Controls.Tests.InteractiveLease exercises fake-backend restoration and desktop availability.",
        "ownershipAndLifetime": "DesktopLease stores a borrowed DesktopBackend reference and snapshot; restore has bounded retries, records exit state separately, and restores foreground then focus then physical pointer. Win32 backend uses WIL scoped ownership for process, desktop and input-attachment cleanup.",
        "failureAndCancellation": "Window disappearance is reported Gone; inability to restore is Failed and focus is NotAttempted when foreground recovery fails. Focus query errors are distinguished from successful null focus. Admission rejects inaccessible/noninteractive desktops and unreadable pointer.",
        "testCoverage": "Controls.Tests.InteractiveLease covers no-op, stale minimized focus, every null-focus path, failed readback, retries, gone windows, focus/cursor failure and admission reason ordering. Hosted InteractiveLease runner exercises confirmation/cancel/child completion integration."
      },
      "plannedSha256": "2da04048ea0b49ef97d6b0f4696423e30c99073dba5fda30dd5ec3ce18689a8c",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.FailureReports.h",
      "sha256": "a69833453838901bf2fe5061daf5cdf6e607afc80649a1cd351c7788ef5d6f9a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Foundation.Tests.Foundation.cpp startup",
        "Embedded.Tests.Embedded.cpp startup",
        "InteractiveLease.Tests.Runner.cpp startup",
        "DxUi.Tests.Runner.cpp startup"
      ],
      "findingIds": [],
      "disposition": "No defect confirmed; process-global hook has runner-startup coverage, with no dedicated behavior test.",
      "reviewNotes": {
        "callers": "Foundation.Tests.Foundation.cpp, Embedded.Tests.Embedded.cpp, InteractiveLease.Tests.Runner.cpp, and DxUi.Tests.Runner.cpp call RouteAwayFromDialogs during startup.",
        "ownershipAndLifetime": "Process-global CRT report hook is installed once per test process; no owned heap/window resource.",
        "failureAndCancellation": "Debug CRT assertions are routed away from modal UI; abort/report behavior still terminates failing test rather than hanging for user input.",
        "testCoverage": "Startup registration is verified in each listed runner; no separate hook-specific assertion found."
      },
      "plannedSha256": "a69833453838901bf2fe5061daf5cdf6e607afc80649a1cd351c7788ef5d6f9a",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.ForegroundThief.h",
      "sha256": "a540fedce6feeb844e1c315e3196e6288759b9005bb35c4085662cc0908e6edf",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--foreground-thief runner option",
        "DxUi.Tests.Menu foreground-loss scenarios",
        "DxUi.Tests.NativeTextInput foreground-loss scenarios"
      ],
      "findingIds": [],
      "disposition": "Reviewed adversarial fixture and caller. It intentionally exercises focus theft only in opt-in scenarios.",
      "reviewNotes": {
        "callers": "DxUi.Tests.Runner.cpp constructs optional ForegroundThief for --foreground-thief / --foreground-thief=<range>; menus and NativeTextInput tests observe foreground loss through window helpers.",
        "ownershipAndLifetime": "A std::jthread owns its worker; destructor stop request and join precede destruction. The worker owns a tiny popup HWND through WIL RAII.",
        "failureAndCancellation": "Window class registration/creation failures terminate the adversarial helper path; stop token and short message wait bound shutdown latency.",
        "testCoverage": "Only explicit adversarial runner mode enables this. Normal no-activation suites avoid invoking it; runner parses range and collects outcomes."
      },
      "plannedSha256": "a540fedce6feeb844e1c315e3196e6288759b9005bb35c4085662cc0908e6edf",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.HeapDiagnostic.h",
      "sha256": "c9c5e000fd2a5ab888ef730959d8e88fb166177a8667f077d1ae3e1d8d6afa64",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "ComplexUiBenchmark profiling cases",
        "ComplexUiProfiling capture",
        "MenuResources heap diagnostic mode"
      ],
      "findingIds": [],
      "disposition": "No defect confirmed; bounded diagnostic helper has explicit failure reporting.",
      "reviewNotes": {
        "callers": "Embedded.Tests.ComplexUiBenchmark.h and Embedded.Tests.ComplexUiProfiling.h emit diagnostics; DxUi.Tests.MenuResources.h uses it for opt-in menu resource heap reports.",
        "ownershipAndLifetime": "Uses fixed-size stack arrays for bounded heap/sample enumeration; WIL scope exit unlocks each heap if HeapLock succeeds.",
        "failureAndCancellation": "GetProcessHeaps/HeapWalk/HeapUnlock failures are reported through supplied Check callback; exceeding fixed heap capacity fails closed instead of writing beyond storage.",
        "testCoverage": "Benchmark/profiling paths and resource diagnostics call the helper; this is opt-in evidence, not runtime product behavior."
      },
      "plannedSha256": "c9c5e000fd2a5ab888ef730959d8e88fb166177a8667f077d1ae3e1d8d6afa64",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.InteractiveLease.h",
      "sha256": "9d7e0658830db619ad9edb3ab3a34d404de3ef5e0b2c47a228f126a9947bf3e4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestLeaseConfirmationAndWarningOrdering",
        "TestLeaseRestoresAfterDeclinedConfirmation",
        "TestLeaseRestoresAfterChildFailure",
        "TestLeaseCaptureReasonSurvivesConfirmationAndChildren",
        "Test-InteractiveMode.ps1"
      ],
      "findingIds": [],
      "disposition": "Reviewed state machine and runner contract; no source-backed defect confirmed.",
      "reviewNotes": {
        "callers": "InteractiveLease.Tests.Runner.cpp implements LeaseServices and uses RunLease/ParsePlan/FormatLeaseResult; Tools/InteractiveRun.psm1 consumes its result contract and tests compare exit-code mappings.",
        "ownershipAndLifetime": "RunLease borrows LeaseServices; it takes the session mutex only after desktop admission, uses scope guards to release the mutex, restore focus/pointer, hide warning, and undo keep-awake across all exits. Child outcomes are retained as value records.",
        "failureAndCancellation": "Input parser rejects malformed/empty plans; declined, timed-out, interrupted, unavailable, launch-failed, child-failed, and restoration-failed outcomes map to explicit states/codes. Child cancellation is delegated to RunChild; no hidden retry.",
        "testCoverage": "Controls.Tests.InteractiveLease covers event ordering and all service failure paths; InteractiveLease runner integration covers result serialization and native child process execution; Test-InteractiveMode checks PowerShell/native exit-code agreement."
      },
      "plannedSha256": "9d7e0658830db619ad9edb3ab3a34d404de3ef5e0b2c47a228f126a9947bf3e4",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PerformanceCapture.h",
      "sha256": "52f23d55215ed49afa57183bd9e49f425348ebef31d0c4bd387e6017ad7b9c4a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "--performance-jsonl",
        "Animation performance capture",
        "WindowHost performance capture"
      ],
      "findingIds": [],
      "disposition": "Advisory: optional diagnostic JSONL may silently lose records after a write failure; no acceptance-evidence impact established.",
      "reviewNotes": {
        "callers": "DxUi.Tests.Runner.cpp installs the sink for optional --performance-jsonl diagnostic output and scope-exit clears it; Animation and WindowHost performance fixtures also start and stop capture around scenarios.",
        "ownershipAndLifetime": "One inline std::ofstream is the process-local sink; Start truncates/creates target and installs global diagnostics/frame callbacks; Stop clears both callbacks then closes the stream.",
        "failureAndCancellation": "Start throws on directory/file creation failure. Emit is noexcept and catches std::exception silently; ordinary output stream failbit is not checked per record. This affects optional ControlTests diagnostic output only.",
        "testCoverage": "Runner and Animation/WindowHost perf paths consume capture; no focused test asserts serialization failure or dropped-write handling. Paired Embedded benchmark acceptance records are a separate path and do not depend on this sink."
      },
      "plannedSha256": "52f23d55215ed49afa57183bd9e49f425348ebef31d0c4bd387e6017ad7b9c4a",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PreparedTreeSemanticFixture.h",
      "sha256": "09704ea7fcbe7944fcf4e084d4c0678ba5e46e9284c099af50391e2cbeae61dd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Fixture constructs prepared semantic tree with explicit node ownership.",
        "ownershipAndLifetime": "Resource and document lifetime are scoped to test fixture teardown.",
        "failureAndCancellation": "Failure setup is isolated so partial preparation is observed safely.",
        "testCoverage": "Consumed by embedded semantic-tree accessibility event tests. Coverage attribution only; this agent did not execute tests."
      },
      "plannedSha256": "09704ea7fcbe7944fcf4e084d4c0678ba5e46e9284c099af50391e2cbeae61dd",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SelectionEventInterruption.h",
      "sha256": "09a019cf73dba7315becec0a765fd46ca6f18881d08be5fe413d0693e80c642e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "09a019cf73dba7315becec0a765fd46ca6f18881d08be5fe413d0693e80c642e",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.TestWatchdog.h",
      "sha256": "a6fbc903c4f81febfa215ba65527f355a2e34b739937f502ee3e640a92c66892",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-TestWatchdog.ps1",
        "TestWatchdogSelfTestBlocksForever",
        "ScopedTestDeadline wrapper"
      ],
      "findingIds": [],
      "disposition": "No defect confirmed; detached worker is intentionally process-lifetime in test executables and teardown terminates the process.",
      "reviewNotes": {
        "callers": "Controls.Tests.DxUiTestHelpers.h arms named cases and renews on progress; DxUi.Tests.Runner.cpp arms fixture-wide work and exposes hidden self-test mode; InteractiveLease runner also configures watchdog.",
        "ownershipAndLifetime": "One intentionally process-lifetime leaked singleton protects against hangs during process exit; a single detached watchdog thread waits on a condition variable, with all armed state guarded by mutex and generation counter.",
        "failureAndCancellation": "Timeout writes a bounded stderr line then terminates the owned test process with exit 124; nested arming exits 1. ScopedTestDeadline disarms through RAII.",
        "testCoverage": "Test-TestWatchdog.ps1 drives hidden blocking test cases with short deadlines and validates timeout naming/exit; named regular tests are wrapped through runner helper."
      },
      "plannedSha256": "a6fbc903c4f81febfa215ba65527f355a2e34b739937f502ee3e640a92c66892",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.TestWindowActivationGuard.h",
      "sha256": "28f8764f37a06db502f3f02e49fd87426d4fad00bdbb89b7e97374ca5cd442ac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "DxUi.Tests.Runner no-activation setup",
        "HostControlRegressions protected window cases",
        "WindowHost activation blocker cases"
      ],
      "findingIds": [],
      "disposition": "No reachable failure confirmed. Document/guard same-thread Stop precondition if helper becomes cross-thread reusable.",
      "reviewNotes": {
        "callers": "DxUi.Tests.Runner.cpp installs blocker for noninteractive suites; FormAccessibility, HostControlRegressions, IntrinsicLayout, RichTextAccessibility, and WindowHost fixtures use or assert the blocker.",
        "ownershipAndLifetime": "WIL unique_hhook owns CBT hook; thread-local active pointer routes same-thread hook callbacks. Callers install/remove on their UI thread; singleton-per-thread guard rejects nesting.",
        "failureAndCancellation": "Start returns false when already active or hook installation fails; destructor calls Stop. Hook protects newly created top-level windows and blocks activation/focus transfers while counting interceptions.",
        "testCoverage": "Runner-wide protected mode and focused HostControlRegressions/WindowHost assertions cover protected window style and blocked activation. Cross-thread Stop is not exercised and would leave owner-thread TLS pointer uncleared; same-thread use is the current caller contract."
      },
      "plannedSha256": "28f8764f37a06db502f3f02e49fd87426d4fad00bdbb89b7e97374ca5cd442ac",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.UiaTestClient.h",
      "sha256": "3e2f2716f4ae8ea7726ac696e8ebf4f8494769c7bbcfbe64b3846346c600d040",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "structural_executor",
      "testIds": [],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Full file read for this review slice; caller/entry paths were traced where invoked by the reviewed host, input, UIA, or embedded paths. No repository-wide caller inventory is claimed.",
        "ownershipAndLifetime": "Reviewed ownership/lifetime boundaries in the available source context; no confirmed defect recorded for this path in this pass.",
        "failureAndCancellation": "Reviewed failure/cancellation transitions in the available source context. Runtime behavior remains unverified because no build or tests were run by this reviewer.",
        "testCoverage": "Full file read; test case IDs are not enumerated here. No tests were executed by this reviewer."
      },
      "plannedSha256": "6f3a5400d5abe5185ec38eaacc00e53c241e0152f070793d1041d64d88874d7b",
      "reviewEvidence": "review-host-controls-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/test-scopes.json",
      "sha256": "4f6d27688ec59e3c65c8a71d63127de02de91607f35d5f528e9ec1dfe13b1067",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-NativeScope.ps1",
        "Test-InteractiveMode.ps1",
        "Test-ScopedTesting.ps1"
      ],
      "findingIds": [],
      "disposition": "Reviewed policy data and listed direct tooling consumers; no specific invalid entry found.",
      "reviewNotes": {
        "callers": "Tools/ScopedTesting.psm1 consumes standalone scopes; Test-NativeScope.ps1 validates PR contract metadata; Test-InteractiveMode.ps1 and Test-ScopedTesting.ps1 use isolated fixtures.",
        "ownershipAndLifetime": "Tracked declarative scope, rule, interactive-obligation, PR coverage, and contract-file metadata; no runtime ownership.",
        "failureAndCancellation": "Schema/semantic validation failures stop scoped test planning; invalid scope and coverage entries do not silently become runnable obligations.",
        "testCoverage": "Test-NativeScope.ps1, Test-InteractiveMode.ps1, and Test-ScopedTesting.ps1 provide consumer-specific metadata validation."
      },
      "plannedSha256": "a35b8619f45aa872b026a5dc978d5d6dff54ebc0ff2f2e9fadadca6413bf8c65",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/BenchmarkGate.psm1",
      "sha256": "3fb19284bc1cd7f84bdd27368f58748007dcfe6fd04ce1a8a2dc49925dd8f915",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-BenchmarkGate.ps1 exercises scope, pair resolution, controls, policy trust, receipt replay, verdict, annotations, artifact cardinality and CI workflow surface. This review ran no tests.",
        "failureAndCancellation": "Missing/malformed/mutated evidence, schedule mismatch, untrusted policy/judge or changed raw receipts fail closed as invalid/inconclusive; pass never claims no change.",
        "callers": "Imported by Publish-BenchmarkVerdict and gate tooling tests; computes changed-path scope, replays retained receipts/control comparisons and produces CI verdict.",
        "ownershipAndLifetime": "Reads retained summaries/receipt bytes and approved source identities; loads only trusted base/candidate judge versions after hash and policy checks; no benchmark child ownership."
      },
      "plannedSha256": "cbc568f05ead14e964b9b8d7556deb5d1d27c2f255eff73a2c6c17c553ee87ab",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/CapabilitySkipPolicy.psm1",
      "sha256": "95bb89554c49b6c2c2526c21aba7d862b201fb1d637948e699d1aa493e613824",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-CapabilitySkipPolicy.ps1: expected capability skip rules"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "test.ps1 uses skip parser and CI expected/unexpected skip classification.",
        "testCoverage": "Focused test validates match/allowlist and unexpected skip behavior; no CI run here.",
        "ownershipAndLifetime": "Pure parser/policy functions over suite logs and declared environment capability manifest.",
        "failureAndCancellation": "Malformed or unrecognized skip lines remain unexpected; CI fails undeclared skip while local receipts preserve partial status."
      },
      "plannedSha256": "95bb89554c49b6c2c2526c21aba7d862b201fb1d637948e699d1aa493e613824",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Changelog.psm1",
      "sha256": "05e987cf29bfb8aedbda5f77feba56d0145f733ccc8c577f0fcc96e1e877dcc3",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ChangelogWorkflow.ps1: compatibility import/export surface"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Workflow tests invoke public scripts/module consumers; no shim-specific test beyond exported behavior.",
        "callers": "Compatibility module forwards functions to shared Documentation/Changelog implementation for legacy validator/benchmark callers.",
        "ownershipAndLifetime": "No independent state or file ownership; local module import/export only.",
        "failureAndCancellation": "Imported implementation performs validation and locking; shim exposes exactly four public functions."
      },
      "plannedSha256": "05e987cf29bfb8aedbda5f77feba56d0145f733ccc8c577f0fcc96e1e877dcc3",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Commit-Gallery.ps1",
      "sha256": "2e2c6ff28384d7e07d507188cd28421708bbfc0e71470844dbe8fabc6170d08c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-Docs.ps1: gallery publication scope/commit workflow source contracts"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-Docs.ps1 checks only gallery is staged and publication workflow uses script; no commit/push performed.",
        "callers": "Manual gallery workflow commits regenerated docs/gallery via this public script, with optional local NoPush preview.",
        "ownershipAndLifetime": "Stages only gallery path, uses bot-local git identity in Actions, ordinary push only; generation.json alone is excluded from commit decision.",
        "failureAndCancellation": "Requires named non-main branch; staging/commit/push failures stop; never force-pushes."
      },
      "plannedSha256": "2e2c6ff28384d7e07d507188cd28421708bbfc0e71470844dbe8fabc6170d08c",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Compare-Performance.ps1",
      "sha256": "a00bceef3722deb15acabc1f5fb2598dfa1ce662934baadb16eeb3e741a07178",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-PerformanceComparison",
        "Test-BenchmarkGate",
        "Test-PairedRun"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "failureAndCancellation": "Sets terminating PowerShell behavior and propagates Invoke-PerformanceComparison exit code; invalid inputs are handled by underlying comparison entrypoint.",
        "testCoverage": "Tools/tests/Test-PerformanceComparison.ps1 validates wrapper-generated comparisons and exit codes; Test-BenchmarkGate/PairedRun include it in harness identity/overlay inputs.",
        "callers": "Legacy/local paired driver performance-paired.ps1 invokes this wrapper for each comparison; PerformanceComparison test harness invokes it as subprocess for output/exit behavior.",
        "ownershipAndLifetime": "Thin public script adapter; imports PerformanceComparison.psm1 and passes only caller-supplied receipt paths and output path."
      },
      "plannedSha256": "a00bceef3722deb15acabc1f5fb2598dfa1ce662934baadb16eeb3e741a07178",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerApi/ConsumerApi.Tests.Revision4.cpp",
      "sha256": "430499d22dc93bdecb47824f2d2716db3d32f42afd6d0b66515da7a1651a6111",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "test-consumer.ps1 compiles the revision-specific public API fixture"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "test-consumer.ps1 covers it when run; no compilation performed here.",
        "callers": "External consumer script copies/compiles this fixture; static assertions and concrete model/delegate instantiations detect public API drift.",
        "failureAndCancellation": "Compile fails on signature or pure-virtual contract drift.",
        "ownershipAndLifetime": "Compile-time-only client uses public headers and owns no runtime state."
      },
      "plannedPath": "Tools/ConsumerApi/Revision4.cpp",
      "plannedSha256": "f31a504d79394f3cf717e2702b7e59b1745744a0b1abb4f49bdb02b8bb579361",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerBuild.psm1",
      "sha256": "833607c3f020a47642dcda2485811abd86d8f1e87f0a408b82fb71d7150bd587",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-BuildMatrix.ps1: consumer build identity fixtures"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-BuildMatrix.ps1 source fixtures; actual consumer build not run here.",
        "callers": "Consumer build root uses Get-DxUiConsumerBuildIdentity before compiling pinned DxUi.",
        "failureAndCancellation": "Rejects invalid source/API pin, missing properties, unsupported compiler host, or absent compiler/SDK files.",
        "ownershipAndLifetime": "Read-only MSBuild property query and SHA-256 identity over commit/API/toolchain/SDK; no restore/build/check-out mutation."
      },
      "plannedSha256": "833607c3f020a47642dcda2485811abd86d8f1e87f0a408b82fb71d7150bd587",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerUpdate.psm1",
      "sha256": "d09a0919a8559cd8c8133b7078a37b416197be24d13b9f128dc7db3a1331f893",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ConsumerUpdate.ps1: advisory status cases; owned child success/nonzero/timeout, unrelated-child survival, timeout PID/start identity exit, unchanged lock pin"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-ConsumerUpdate.ps1 now starts real local PowerShell children for success, nonzero and timeout, verifies the timed-out PID/start identity is gone while an unrelated owned sentinel stays alive, and checks the exact lock hash remains unchanged. Test ran successfully after change.",
        "callers": "Show-DxUiUpdateNotice is called once by ConsumerBuild from consumer restore/build entrypoints; if no GH_TOKEN/GITHUB_TOKEN, it invokes the owned gh auth-token child before its bounded read-only API requests.",
        "failureAndCancellation": "Timeout returns no token after kill/exit confirmation; start/read/wait errors are swallowed only at advisory boundary without echoing credential transport exceptions; nonzero exit or incomplete pipe drain cannot yield a token.",
        "ownershipAndLifetime": "Private Invoke-DxUiOwnedProcess owns one Process handle; concurrently drains stdout/stderr, never logs either stream, uses bounded process and drain waits, kills only the direct process it started, confirms its exit within one second and disposes the handle."
      },
      "plannedSha256": "6a1398bbf9a8700bf9058ca46d20f0eada631252026e86e23c252917f6266ff5",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ChangelogWorkflow.ps1: validation, lock, idempotence, encoding, retry and final-runner gate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-ChangelogWorkflow.ps1 checks exact fold order/encoding, locking, retry, relative root, pending final gate and process cleanup.",
        "callers": "Test-Changes PrePush and final handoff call -Check; developer invokes fold command to merge pending Changes fragments.",
        "ownershipAndLifetime": "Validates first, uses Documentation Changelog module and exclusive checkout-local lock, publishes folded changelog before deleting fragments; WhatIf/Check avoid writes.",
        "failureAndCancellation": "Rejects malformed/reparse path inputs and pending fragments at check; interrupted cleanup can retry without duplication; I/O errors terminate."
      },
      "plannedSha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownEntityData.ps1",
      "sha256": "5db8807d744de4a3c250ca533c25a2f2e93c4132bdeff392164e29fea3061b7c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-MarkdownData.ps1: per-generator offline reproducibility, read-only check, stale-output and all-input/license hash-pin cases"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Generated corpus/table consumed by compiled native parser/tests; regeneration and verification are invoked by focused tooling tests.",
        "ownershipAndLifetime": "Produces sorted bounded semicolon entity table from pinned WHATWG JSON; checks 2,125 count, name grammar, scalar values and characters agree. Reads pinned source/license, writes one generated header only outside Check; no parser implementation/data imported.",
        "testCoverage": "Test-MarkdownData.ps1 independently compares generated bytes to committed header, reruns deterministically, verifies Check does not mutate, and tamper/missing pin refusal.",
        "failureAndCancellation": "Rejects any source/license hash mismatch, invalid name/count/scalar/expansion or stale output in Check mode."
      },
      "plannedSha256": "5db8807d744de4a3c250ca533c25a2f2e93c4132bdeff392164e29fea3061b7c",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownTestData.ps1",
      "sha256": "61bef352621c5dcd171e66fde31da46299ba5f0465628193fc4b41c0fd7503ea",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-MarkdownData.ps1: offline reproducibility, read-only check, stale output and full pin refusal per generator"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Generated corpus/table consumed by compiled native parser/tests; regeneration and verification are invoked by focused tooling tests.",
        "ownershipAndLifetime": "Builds corpus preservation vectors from pinned CommonMark/GFM/WHATWG inputs; preserves licenses/provenance and explicitly labels vectors non-conformance. Offline deterministic header generator; emits only expected vector arrays from fully pinned inputs.",
        "testCoverage": "Test-MarkdownData.ps1 verifies exact committed generated bytes, deterministic repeat, read-only Check and changed/missing input refusal.",
        "failureAndCancellation": "Rejects corpus count/state mismatch, malformed entity/source hashes and stale output under Check."
      },
      "plannedSha256": "4e332e0227d6bbfef0fbe221a85b77f96a252a03d4ee79ca3fdefcc2aa6ea819",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownUnicodeData.ps1",
      "sha256": "3d2dea582dc9e1fd207c4853ba2cd610d6aa07284e7899a52184b2e92dfbfb4d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-MarkdownData.ps1: generator reproducibility, Check no-write and pinned source/license rejection"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Generated corpus/table consumed by compiled native parser/tests; regeneration and verification are invoked by focused tooling tests.",
        "ownershipAndLifetime": "Emits Unicode 17 punctuation/symbol/space/alphanumeric ranges and full-fold mappings for Markdown parser behavior. Offline generator hashes three pinned data/license sources and writes one table header; shared license provenance retained.",
        "testCoverage": "Test-MarkdownData.ps1 tests committed-byte parity/repeat, read-only Check and altered/missing pins.",
        "failureAndCancellation": "Rejects changed pins, invalid UnicodeData range pairing/unterminated interval, or stale generated output."
      },
      "plannedSha256": "3d2dea582dc9e1fd207c4853ba2cd610d6aa07284e7899a52184b2e92dfbfb4d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-NativeFlowchartTestData.ps1",
      "sha256": "d5becb01f014682c138aaebb48ef1f8135dcb1cdcac6e72d2062390556d7db86",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-MarkdownData.ps1: offline reproducibility, Check read-only, output stale/missing, all data/license/source pins"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Generated corpus/table consumed by compiled native parser/tests; regeneration and verification are invoked by focused tooling tests.",
        "ownershipAndLifetime": "Generates original Mermaid 11.4.1 vectors plus independent DxUi family/subset expectations. Offline, all source docs and expectation/matrix JSON are hashed, collection references verified; generated arrays explicitly distinguish reference examples from reviewed subset expectations.",
        "testCoverage": "Test-MarkdownData.ps1 compares committed bytes and checks repeat/check/pin tampering; native parser execution is covered elsewhere, not by generation.",
        "failureAndCancellation": "Rejects missing/changed references, unexpected 105/24/22 counts, unsupported expectation shape/outcome, or stale output."
      },
      "plannedSha256": "a56b9089219f81cd1b3c234a3a0a0c4cbb355819ecc3323a1b8523444fd90300",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-UnicodeGraphemeData.ps1",
      "sha256": "f5c68eda1ac662460519171ee0ea2f681a966692d095758cce59d02d026e3c6e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-UnicodeGraphemeData.ps1: deterministic outputs, non-mutating Check, missing/stale outputs and data/license pin refusal"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Generated corpus/table consumed by compiled native parser/tests; regeneration and verification are invoked by focused tooling tests.",
        "ownershipAndLifetime": "Generates merged Unicode 17 property ranges and all 766 GraphemeBreakTest vectors. Pinned offline generation writes only two caller-selected outputs; source/license hashes and expected vector count guard provenance.",
        "testCoverage": "Test-UnicodeGraphemeData.ps1 validates exact committed bytes, deterministic repeat, check timestamps and mutation/missing input refusal.",
        "failureAndCancellation": "Rejects overlapping/bad ranges, non-scalar test vectors, malformed boundaries/count, changed/missing input or stale Check output."
      },
      "plannedSha256": "b95dc08c5150e0a698605bb033362da1660f6ed224480808f2af18888502d2c4",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Get-BenchmarkScope.ps1",
      "sha256": "7cd8848231bfab514e205364030d5c12268b64301d31c223091920cc9332d826",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-BenchmarkGate.ps1: path rules are exhaustively checked against actual receipt inputs and workflow pins exact pair commits"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-BenchmarkGate.ps1 covers benchmark inclusion/exclusion and exact PR commit identity; no hosted workflow run.",
        "callers": "Workflow wrapper resolves PR first-parent/candidate pair, obtains changed paths, delegates scope classification to BenchmarkGate and exports outputs/summary.",
        "ownershipAndLifetime": "Read-only Git comparison and GitHub output/summary append in job environment; no repository mutation.",
        "failureAndCancellation": "Missing base or unreadable/mismatched expected commit fails job rather than deciding scope optimistically."
      },
      "plannedSha256": "e13d9f5d1f85f15321b6c10913dc334ed9ba0a02a69367b6ec27e7574374db97",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Get-NativeScope.ps1",
      "sha256": "6981ebc6344a6892e8a1cc7291d0fe914fa1c7514eca5cd43150d82255c27228",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-NativeScope.ps1: documentation-only/native-required path classes, unknown paths and exact commit pairing"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-NativeScope.ps1 checks documentation and source coverage, unknown paths, workflow event pair validation, budgets, early build gate; no hosted run.",
        "callers": "Workflow wrapper runs all non-PR events natively; PR change decision delegates to NativeScope over read paths and emits workflow summary/output.",
        "ownershipAndLifetime": "Read-only Git changed-path enumeration, normalizes de-duplicates paths; workflow output is scoped to current job.",
        "failureAndCancellation": "Unknown paths or empty/unreadable changes require native jobs; named base/candidate SHA mismatch fails closed."
      },
      "plannedSha256": "01dfcb03f5d9211031e6ff17ddf73f02346f79c87e0a77f6cd0edd20fa0ed9ce",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Install-ClangFormat.ps1",
      "sha256": "a9b634be43b77764e9e6f0d15cb145c9f9bb4610065053d843f1c1770af945e8",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "No dedicated installer test found; format.ps1 -Check and CI format job are downstream consumers"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "No dedicated adversarial archive/installer test found in Tools/tests; format check/CI uses resulting exact version. This is explicit coverage gap.",
        "callers": "Downloads pinned 22.1.3 wheel from PyPI index or accepts WheelPath; extracts one executable under .build/format for format.ps1.",
        "ownershipAndLifetime": "Uses exact wheel SHA-256, verifies executable version, extracts only named archive entry, deletes only downloaded wheel in finally; preserves preexisting valid install.",
        "failureAndCancellation": "Rejects ambiguous/missing pinned index file, hash mismatch, missing archive member, or wrong resulting binary version."
      },
      "plannedSha256": "a9b634be43b77764e9e6f0d15cb145c9f9bb4610065053d843f1c1770af945e8",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/InteractiveRun.psm1",
      "sha256": "8b88a8df3c3199692c3898b74bd04c29e0afa9907219832679b69ba30b6938df",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-InteractiveMode",
        "Test-TestWatchdog"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-InteractiveMode.ps1 covers suite selection/refusal, hosted runner gating, plan quoting, result integrity/restoration, Ctrl+C seam and ARM preparation predicates; native desktop behavior is covered by hosted workflow/native lease rather than this pure module test.",
        "failureAndCancellation": "Receipt parser records schema/duplicate/malformed fields and child outcomes; completed success requires restoration, authorization and exact expected child sequence.",
        "callers": "test.ps1 uses suite selection/refusal, plan/result parsing and local or verified hosted lease wrapper; Prepare-HostedArmDesktop consumes runner predicate.",
        "ownershipAndLifetime": "Owns lease plan/result files in caller work directory and process handle; native lease remains attached to console so Ctrl+C reaches its restore path."
      },
      "plannedSha256": "4beb87a7f5cd05e72c37cc4beaceaae39bd6f21caf9db4bcbc39235f9fa9fb13",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ChangelogWorkflow.ps1: dated fragments, validation, atomic fold, lock contention, encoding, retry, path safety"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-ChangelogWorkflow.ps1 exercises these source-backed behaviors, including lock and interrupted publication.",
        "callers": "Fold-Changelog/Test-Changelog and validator use shared changelog behavior.",
        "ownershipAndLifetime": "Reads fragments; fold serializes with exclusive lock, atomically publishes output and then removes only folded fragments; preserves encoding/newlines; reparse points rejected in ancestors.",
        "failureAndCancellation": "Validation catches malformed root/heading/fragments; lock contention aborts before mutation; retry avoids duplicate entries, while failed cleanup leaves input to retry."
      },
      "plannedSha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/NativeScope.psm1",
      "sha256": "35f95b66205fd5baaeedaa10eb45aceae8a919ae51584f77cd2757ae5e390772",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-NativeScope.ps1: all documentation rules and unknown-path conservative classification"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-NativeScope.ps1 tests all tracked source/test/sample/build/tool and document categories plus workflow consistency.",
        "callers": "Get-NativeScope.ps1 and CI workflow use this module to select native PR jobs.",
        "ownershipAndLifetime": "Pure documented-path rules and normalized ordinal unique path scan.",
        "failureAndCancellation": "A path not recognized as documentation remains native; no changes/no readable paths is conservative native."
      },
      "plannedSha256": "35f95b66205fd5baaeedaa10eb45aceae8a919ae51584f77cd2757ae5e390772",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/PairedRun.psm1",
      "sha256": "b72612a3d39717a40d64d6fb5a7edb3b3b6768d768179b8ef44db41476b7834f",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PairedRun.ps1: side selection, source distinction, overlay before/after measurement, SkipBuild compiled-input gate, timestamp restore and isolated judge loading"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "failureAndCancellation": "Restore-HarnessOverlay now compares current overlay-created and replaced path bytes against installedSha256 before any removal/restoration; later edits are preserved and aggregated as restoration failures.",
        "testCoverage": "Tools/tests/Test-PairedRun.ps1 fixture “overlay rollback preserves later edits to created and replaced paths while restoring unaffected files” verifies both edit cases are retained and reported; regular create/replace restoration and corruption cases remain covered.",
        "ownershipAndLifetime": "Tracks created/replaced paths and created parent directories; backs up replaced originals and checks backup hashes; restores named trees in finally.",
        "callers": "performance-paired.ps1 creates per-side overlays, checks bytes before/after each measurement, and restores them on exit."
      },
      "plannedSha256": "358b7013b4a0a238ef3d3e2f757a49baa86bdb2b2fe604c84d5fda0a76dcfed5",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceAcceptancePolicy.v1.json",
      "sha256": "a80d2bce198e4eec3405024c2959bd13058605921ee9e0b935a5d3350e77e8d2",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformancePolicy.ps1: policy authority from measured base; Tests/Test-BenchmarkGate.ps1: qualified-policy gate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Named policy tests reject candidate bootstrap and trust only independently qualified base policy; no qualification claimed.",
        "ownershipAndLifetime": "Static proposed policy; identity hashes are explicit review targets, not authority; accepted policy must be independently qualified in measured baseline Git blob.",
        "callers": "Defines candidate migration policy only; qualified=false, status policy-review-required, exact versioned judge/assignment hashes, alpha/family/budgets and full identity keys.",
        "failureAndCancellation": "Unqualified status prevents this candidate copy from self-authorizing a pass."
      },
      "plannedSha256": "bac3f8a8e1b0f31fd109041029439ad2fd35531c24fa4dde33a49d3ff34241a7",
      "reviewEvidence": "review-dxui-tooling-20261024.md",
      "leadDeltaReview": "Lead reviewed current delta: assignment source target updated after the reviewed rollback repair; qualified remains false, policy-review-required, judgment digest, random assignment and metric bands unchanged. Test-PerformancePolicy: 39/39 passed; this does not approve performance."
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceComparison.psm1",
      "sha256": "fd701df248bc621d9942f47df12ef02fe0d929d07290b18128ea644c848a8cae",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformanceComparison.ps1: receipt identity, exact rank/sign-flip/Holm and paired-set reference cases"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-PerformanceComparison.ps1 exercises independent exact statistical references, malformed receipts, deterministic serialization; no hosted/native study run.",
        "callers": "Compare-Performance.ps1 and performance-paired.ps1 use comparator/judge APIs; paired path loads baseline committed judge blob for same-byte comparison.",
        "failureAndCancellation": "Rejects malformed or mismatched receipts and unsupported allocation/count; reports advice-required or invalid-evidence, never rebaselines.",
        "ownershipAndLifetime": "Receipt and statistical logic plus bounded local Git subprocess for committed judge/provenance; rank-distribution memoization is module-local."
      },
      "plannedSha256": "d080952fbec4d35b8d2a26ccaf27612611876a1f3c015f360780b3065032b378",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformancePolicy.psm1",
      "sha256": "a0c774ffbf8422307d3d0138a33cbc08b958428186de846b87eaccbcde7f0e01",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformancePolicy.ps1: trusted-base policy, receipt identity, compiler/linker closure, native project/source topology, junction refusal, failed-build receipt revocation"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-PerformancePolicy.ps1 contains extensive fixture coverage listed by named cases; no build or benchmark run.",
        "ownershipAndLifetime": "Captures complete build/environment/tool/dependency identity and builds receipt only after matching before/after state; revokes old receipts when build begins/fails.",
        "callers": "Build and performance entrypoints use policy identity/attestation APIs before and after compile or measurement.",
        "failureAndCancellation": "Requires trusted qualified policy from measured baseline; refuses unknown/missing metadata, compiler/tool/profile/source/dependency/artifact mismatch, redirected paths and build failure."
      },
      "plannedSha256": "b1a4d8546be93601ea7f7042a6e4eee0d948d35091a6f85256e425369924ae4a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceProfiling.psm1",
      "sha256": "12b8ae312a16e836f22779d2429c56f96df235c985df26b06142042f58cdce72",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-PerformanceProfiling.ps1: exact profiling schemas, CPU/thread/heap completeness, boundary timers and WARP policy; stale build/output boundary; timeout/drain/owned-process identity"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "failureAndCancellation": "On timeout kills only the owned original process handle, records termination/drain failures, rejects malformed/unavailable telemetry masquerading as complete, and publishes only after before/after measurement/build identity matches.",
        "testCoverage": "Test-PerformanceProfiling.ps1 includes malformed schema/PNG, integer boundaries, declared partial telemetry, fixed WARP/timer policy, stale build/output refusal, ownership identity and bounded timeout/drain fixtures. No native profile run here.",
        "ownershipAndLifetime": "Runs a redirected, no-window child with fixed 120 s timeout, bounded drain/pipe/capture/report sizes; validates raw immutable report and PNG structure; writes reports beneath .build/reports when output is in-repo.",
        "callers": "performance-profile.ps1 invokes Invoke-ProfilingMeasurement; performance-forms.ps1 independently shares PerformancePolicy state and uses the same attested embedded test executable."
      },
      "plannedSha256": "2a0a1c9edca392040616929155214634accf46e572633466dc7e6aaa288347ed",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Prepare-HostedArmDesktop.ps1",
      "sha256": "88bf0f0643942521bf3072222cde69721b2a343505de0ee695d32320ed9bc3c8",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-InteractiveMode"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-InteractiveMode.ps1 tests hosted ARM admission and guarded behavior through source/seam harness; no live hosted desktop validation performed in this review.",
        "failureAndCancellation": "Refuses non-hosted/non-ARM environment; checks foreground has not changed before minimize and fails closed if window refuses minimize.",
        "callers": "Invoked in CI only after verified hosted ARM foreground lease; narrows action to exact observed WWAHost/CoreWindow and separate UserOOBE window.",
        "ownershipAndLifetime": "Process and HWND references are observational, no process termination; current user policy values may be set on disposable hosted runner only."
      },
      "plannedSha256": "9cb229629883f3e2959aa855fa47f81bbc6ace6f4a0bf5ca5d2dab9021a2a7ac",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Publish-BenchmarkVerdict.ps1",
      "sha256": "6df67e9743d5eb2c9d44c14b2460db74d06a75b5ec8cf98e3803430900555121",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-BenchmarkGate.ps1 covers pass/inconclusive/invalid/manual/PR gating, paired-root cardinality and SHA mismatch.",
        "failureAndCancellation": "Missing/unreadable/invalid evidence yields no verdict and exit 1; gate vs manual result behavior explicit.",
        "callers": "CI verdict step consumes paired summary and same-binary controls; emits annotation, summary, artifacts and outputs; PR gate fails degraded/inconclusive.",
        "ownershipAndLifetime": "Reads summary evidence; writes verdict files in chosen output directory and workflow summary/output; candidate judged as data, not executed."
      },
      "plannedSha256": "56243977a0ef64badd234a599079b229689630f4186ca6c6f203667e660130fe",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/ScopedTesting.psm1",
      "sha256": "fe0b19b877fb59b3647d4345ad2c7c53b053efd1f328f7c9ba2ed8756fe1fe86",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-ScopedTesting",
        "Test-NativeScope",
        "Test-BuildMatrix"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-ScopedTesting.ps1 covers changed path matching, identity invalidation, dependency/artifact changes, PR contract and receipt behavior; Test-NativeScope and Test-BuildMatrix cover related scope/build-map helpers.",
        "failureAndCancellation": "Unknown scopes, unsafe paths, missing installed dependency metadata or absent artifacts fail closed; stale/corrupt receipts miss cache; PR proof must match clean branch/workflow/head/base and is double-checked after query.",
        "callers": "Imported by test.ps1, build.ps1, test-consumer.ps1 and paired driver; derives changed-path plan, obligations, source/dependency/artifact/environment/tool identities and PR coverage.",
        "ownershipAndLifetime": "Git child process disposed; receipts are per scope/profile; source and dependencies hashed, opt-in values hashed; PR external gh queries only decide reusable delegated coverage."
      },
      "plannedSha256": "fd728facd01e19100b7b01234a4808c52e003628076ab0cda7ab42e09ab43a5e",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/SuiteFailure.psm1",
      "sha256": "86e18de96dded74172f10e6d5c45b0e19ff4ca7578281b1ffd683ffa9541f946",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-SuiteFailure",
        "Test-TestWatchdog"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-SuiteFailure.ps1 and Tools/tests/Test-TestWatchdog.ps1 cover timeout naming, ordinary exits, absent log, and skip attribution.",
        "failureAndCancellation": "Only exit code 124 plus TIMEOUT line claims named timeout; absent logs return empty tail; skip parser associates skip with active suite/test.",
        "callers": "test.ps1 calls Get-SuiteFailureReport on suite failure and Write-SuiteSkips for suite log summaries.",
        "ownershipAndLifetime": "Reads logs only; returns summary/timeout/tail and skip strings without retaining mutable state."
      },
      "plannedSha256": "86e18de96dded74172f10e6d5c45b0e19ff4ca7578281b1ffd683ffa9541f946",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-ChangelogWorkflow.ps1: fragment validation and pending/folded gate cases"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-ChangelogWorkflow.ps1 exercises accepted/rejected fragment shapes, reparse safety and require-folded state.",
        "callers": "Read-only operator and validator entrypoint for Changes fragments/CHANGELOG.md.",
        "ownershipAndLifetime": "Reads resolved root, validates changelog/fragments and prints pending count; no persistent writes.",
        "failureAndCancellation": "Malformed heading/fragment/path or RequireFolded pending fragment is terminating validation error."
      },
      "plannedSha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "changelog workflow self-contained fixture cases",
        "Tools/tests/Test-Validation.ps1"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Script defines 18-ish changelog Cases and the public final-runner refusal/timeout negative controls; runner invokes it through test tooling. No execution during this review.",
        "failureAndCancellation": "Invalid fragment/fold/lock cases refuse without mutation; child timeout cleanup kills only the owned fixture process tree and verifies exit.",
        "callers": "Standalone script imports shared Changelog module and invokes fold/validate public entrypoints against synthetic checkouts.",
        "ownershipAndLifetime": "Requires caller-owned existing scratch parent; each GUID child is checked direct descendant, child processes are bounded and owned; finally removes exact uniquely named child only."
      },
      "plannedSha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-CiGate.ps1",
      "sha256": "9079f91a913b64a3dbea1bf7eadaab2c21438a4fb81d5181c67c142a289c978e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-CiGate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-CiGate.ps1 has table-driven success/failure combinations for PR/manual and paired verdict requirements.",
        "failureAndCancellation": "Rejects absent/unknown job outcomes, unsafe skips, wrong PR commit pair, and incorrect verdict statuses.",
        "callers": "CI final required-check job passes all workflow job outputs and expected paired commit IDs.",
        "ownershipAndLifetime": "Pure input validation, no external state."
      },
      "plannedSha256": "e75713cb5e95977783fc0f013689ce85ed8a2e31867906b9d063db84980117aa",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-RunEvidence",
        "test.ps1 evidence lifecycle"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-RunEvidence.ps1 covers concurrent/duplicate close, publication, locked/reparse payload, ownership, interruption, native artifact capture and metadata preservation; no runner test executed here.",
        "failureAndCancellation": "Rejects root/UNC/relative paths, reparse points, replaced anchors, concurrent writes and incomplete artifact preservation; interrupted run leaves retained evidence; never deletes payload.",
        "callers": "test.ps1 creates run evidence before suite work and completes/publishes receipt after; standalone helper provides archived payload and compatibility reports.",
        "ownershipAndLifetime": "Pins repository/evidence ancestors and file identity, owns run lock and optional shared artifact lock; all handles disposed in completion/failure; evidence payload remains preserved."
      },
      "plannedSha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Invoke-ToolingTests.ps1",
      "sha256": "657b71f8b5ba586f8fb9da526bdde7805d4415855d28943dd53bacbcf870bb15",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Invoke-ToolingTests"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Top-level tooling-test dispatcher invokes 23 scripts in fixed order, records independent failures, and reports aggregate failure after the loop.",
        "ownershipAndLifetime": "Dispatcher does not own fixture state; each child runs in the same PowerShell process and shares process-level environment/current directory, so per-test cleanup is delegated to its script.",
        "failureAndCancellation": "Per-script exceptions are caught so later suites still run; this can leave global mutations observable to later scripts if a failing script does not restore them.",
        "testCoverage": "Named suites include BuildMatrix, PerformanceComparison, PairedRun, BenchmarkGate, BenchmarkHost, CiGate, CapabilitySkipPolicy, SuiteFailure, InteractiveMode, ConsumerUpdate, and UnicodeGraphemeData. Static aggregator review only; no suites executed."
      },
      "plannedSha256": "eeb2e0570a4c7d82bc0280a797fa8a9426c1ae51cca2135e3527e53292296551",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-AsanRuntime.ps1",
      "sha256": "c83660650e6a1fbaf482b47b36a603643d991190b4f48eb4cc9318acadde12d1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-AsanRuntime"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tooling dispatcher invokes this integration test; it imports the Visual Studio environment helper and builds a synthetic vcxproj through the actual ASan staging target.",
        "ownershipAndLifetime": "Synthetic inputs and staged runtime live under repository .build/ToolTests; the test does not show fixture cleanup, so generated test material persists there.",
        "failureAndCancellation": "Missing runtime DLL is expected to fail staging; after a synthetic runtime is supplied, the staged copy hash must match. Build failures throw immediately.",
        "testCoverage": "Exercises ARM64 target staging and content identity, but this is a static script review; no build/test was run."
      },
      "plannedSha256": "c83660650e6a1fbaf482b47b36a603643d991190b4f48eb4cc9318acadde12d1",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BenchmarkGate.ps1",
      "sha256": "5dc494ef0641d65228abafc6efd43a42bab33e9ba0b281e26063b64a2b16568c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "every measured path is measured",
        "both judges are bound to the exact same retained receipt bytes",
        "the workflow cancels what a push supersedes, bounds its runtime and pins what it runs"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Runs benchmark-gate, receipt/comparator and workflow projection cases against repository scripts plus constructed Git/JSON fixtures.",
        "ownershipAndLifetime": "Fixture helpers create isolated temporary repository roots and receipts; the script uses deterministic seeded schedules and separate base/candidate judge modules. It also reads retained Measurements data as immutable inputs.",
        "failureAndCancellation": "Covers fail-closed invalid evidence, missing controls, mismatched hashes/identity, malformed summaries and workflow wiring. Failure cases distinguish inconclusive evidence from pass/fail; fixtures are removed by shared TestSupport wrappers.",
        "testCoverage": "Fully read 1,721 lines and all case declarations, including exact receipt-byte binding, control drift, exact budgets, Holm/judge policy, retained A/A evidence, workflow events/permissions/timeouts and strict script parsing. No test run performed."
      },
      "plannedSha256": "dced1b09968771116c85ec82016693500fae447a99e1a38b009af4cac31290f8",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BenchmarkHost.ps1",
      "sha256": "ee97874bb4a97ccfe1cd151d9f79e1445892458d07272ee73562be3be244709a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "hosted study admits continuous observations and retains original native outputs",
        "host observations refuse to overwrite original evidence",
        "observation budget exhaustion retains raw samples and fails admission"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Imports TestSupport and exercises the hosted benchmark observer through bounded child processes.",
        "ownershipAndLifetime": "Observer child process handle is owned and disposed; async stdout/stderr drains and output directories are fixture-scoped. Original benchmark receipts are asserted unchanged.",
        "failureAndCancellation": "Tests refuse overwrites, failed readiness/stop, exhausted observation budgets and unsupported hosts. Time-bounded child behavior reports completion/exit state.",
        "testCoverage": "Named cases cover continuous observations and retained raw samples, no-overwrite behavior, budget exhaustion, and unsupported-host refusal. No tests run."
      },
      "plannedSha256": "ee97874bb4a97ccfe1cd151d9f79e1445892458d07272ee73562be3be244709a",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BuildMatrix.ps1",
      "sha256": "81c1330283c84a25ffee178792d23a9902ca5adf10ab294dd62ed2af1ec81c4c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "every project requires real configuration entries",
        "the consumer validator resolves a relative Root from another working directory",
        "a silent Debug fallback and a missing build are rejected"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Exercises the consumer project-matrix validator against temporary project/solution fixtures and a secondary working directory.",
        "ownershipAndLifetime": "Fixture roots are created and removed through TestSupport; project configurations and solution mappings are synthetic.",
        "failureAndCancellation": "Rejects absent/duplicate configurations, silent Debug fallback, missing build mappings and build-0 mappings; validates relative Root resolution independent of caller cwd.",
        "testCoverage": "Named cases verify real configurations, alternate-cwd root resolution and fallback/missing-build rejection. No test run."
      },
      "plannedSha256": "81c1330283c84a25ffee178792d23a9902ca5adf10ab294dd62ed2af1ec81c4c",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-CapabilitySkipPolicy.ps1",
      "sha256": "9eaaafd4d096cb83958dbdb0a1e2de6bfd9b98d2288aba9fdb9cce2321997ea1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-CapabilitySkipPolicy"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests the capability-skip policy validator and parser against source fixture manifests/logs.",
        "ownershipAndLifetime": "Temporary fixtures are scoped through TestSupport; policy JSON is read as test input and no global app process is launched.",
        "failureAndCancellation": "Malformed, duplicate, self-hosted, new/changed and wrong-lane allowances fail closed. Log parsing requires exact suite/test/reason association and skip boundaries.",
        "testCoverage": "Covers x64 Debug and ARM64 ASan lane mismatch, empty/multiple skip boundary cases, and preservation of reporter helper semantics. No test run."
      },
      "plannedSha256": "9eaaafd4d096cb83958dbdb0a1e2de6bfd9b98d2288aba9fdb9cce2321997ea1",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-CiGate.ps1",
      "sha256": "b0fbe9a7bb2b14df53adffd65190b956becfdc936453f6876db134233f1971f4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-CiGate"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Invokes the CI-gate script with controlled environment values and fixture status/report files.",
        "ownershipAndLifetime": "Environment changes are restored in finally; synthetic gate outputs live under temporary fixture roots.",
        "failureAndCancellation": "Required check failures, malformed hosted markers, paired/manual requirements and invalid provenance produce explicit refusals; scope/doc-only classifications are checked separately.",
        "testCoverage": "Cases cover complete PR and docs-skip classification, required statuses, native-skip conditions, paired/manual gate requirements and exact baseline/candidate hashes. No tests run."
      },
      "plannedSha256": "ade184704a87c9605aa02575e0845732b4269b73adb8da6a3b11e6bb070add86",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-ConsumerUpdate.ps1",
      "sha256": "f60e8355e619a0954447cb8c3d8b92bff5bde5f0c376a56e864ac18928d252df",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "the owned credential helper is bounded, drained and only returns successful output"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests the ConsumerUpdate module against synthetic lockfiles and mock request/credential helpers; current script includes the new bounded owned-child case.",
        "ownershipAndLifetime": "Fixture root is removed after each case; the owned credential helper uses its exact Process handle, drains redirected streams and leaves an unrelated sentinel process untouched. Lock bytes/hash are checked before and after.",
        "failureAndCancellation": "New case checks success output only after child completion, nonzero exit refusal, timeout/termination, bounded output handling and no leak; network is mocked and never contacted.",
        "testCoverage": "Reviewed all ten advisory/update scenarios including lock pin preservation and `the owned credential helper is bounded, drained and only returns successful output`. No test run."
      },
      "plannedSha256": "4760c3dd60c46a531d4af3242b439f7e0cb1c8c60858221e2cfd63412b6b6281",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-Docs.ps1",
      "sha256": "e667ac28e2f80f529dba64702fd57323f9fc51ca70d6aeb7cdbb1ae4299f88b9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "complete docs pass",
        "missing control usage fails",
        "changed gallery bytes fail",
        "lambdas in fenced code are not links",
        "raw measurements cannot return to docs",
        "measurements require library ownership and explanation",
        "complete design system passes",
        "new control requires preview",
        "preview requires card marker",
        "removed control leaves no stale component",
        "complete specification tree passes",
        "nested git checkouts are not part of tree",
        "nested checkout measurement receipts are excluded",
        "changelog fragment must be one dated bullet",
        "fold orders fragments and removes them",
        "malformed or empty fold changes nothing",
        "merge retains changelog line endings",
        "gallery regeneration commits and pushes only gallery files",
        "receipt-only or identical rewrite is a no-op",
        "README or index changes publish without sheet changes",
        "gallery publish refuses force push",
        "NoPush commits locally only",
        "each completed plan has exactly one Done link",
        "native Release CI regenerates gallery before upload",
        "gallery workflow is manual-only with safeguards"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Registered tooling test Test-Docs; imports Validation.psm1 and Changelog.psm1 and directly invokes Test-DxUiDocs/Measurements/DesignSystem/Specs/Changelog plus Commit-Gallery and Fold-Changelog.",
        "ownershipAndLifetime": "Synthetic content is under Invoke-FixtureCase roots; the gallery publication fixture makes a temporary bare origin and worktree, publishes only its fixture branch, and explicitly removes the base fixture on normal completion. Changelog workflow fixture is repository-local under .build/changelog-fixtures.",
        "failureAndCancellation": "Validators reject missing catalog usage, changed image bytes, raw docs measurements and unexplained/wrong-owner receipts; specs exclude nested Git checkouts while still validating ordinary docs. Gallery commit refuses force push and NoPush mode leaves origin unchanged; malformed changelog prevents partial fold.",
        "testCoverage": "Tests cover docs/catalog/gallery hash integrity, fenced code parsing, measurement ownership, design system catalog/preview consistency, spec link walk/nested checkout exclusion, changelog fragment shape/fold/line endings/atomic refusal, gallery-only commit/push/no-op/non-force/no-push, completed-plan index, and CI/manual workflow publication guards."
      },
      "plannedSha256": "e667ac28e2f80f529dba64702fd57323f9fc51ca70d6aeb7cdbb1ae4299f88b9",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-InteractiveLease.ps1",
      "sha256": "f529c07d1e2aa17f5649b05c827b6dec759c760bcf104c6e924cdd9740dbe52e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "the self-test passes: children, the session's lease, the confirmation and the warning",
        "--check reports failed result writes without taking the desktop",
        "the lease ends only the processes it started"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; inherited native descendant-completion coverage confirmed at InteractiveLease.Tests.Runner.cpp:532-550,686-746. No tests executed.",
      "reviewNotes": {
        "callers": "Exercises DxUi.InteractiveLease.exe and the PowerShell lease reader/wrapper; interactive UI proof is delegated to native self-test on a private desktop.",
        "ownershipAndLifetime": "The bounded PowerShell harness owns/disposes its direct child Process object; native SelfTest::Run in Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp:532-550 runs ChildProcesses and DescendantProcesses, aggregates failed Check calls, and returns nonzero if any proof fails. DescendantProcesses at lines 686-746 verifies PID plus creation time and waits for exact descendant termination on normal-exit and timeout cases.",
        "failureAndCancellation": "Checks usage refusals before desktop access, failed result writes, hosted authorization, launch failures and direct process timeout. Hosted modes reject absent or altered runner markers.",
        "testCoverage": "The PowerShell case `the self-test passes: children, the session's lease, the confirmation and the warning` at Tools/tests/Test-InteractiveLease.ps1:49-62 requires exit 0, a PASS summary, and zero FAIL lines. Since native SelfTest::Run invokes DescendantProcesses and propagates any failed check to its exit status, the existing behavioral test covers exact descendant completion; no additional source-mirroring assertion is needed. This is source review only; no test run."
      },
      "plannedSha256": "f4f6121ad5a3e81b71985f66ea0f5873acd850d71dd7b7c05be1c94c11201589",
      "reviewEvidence": "review-general-tools-supplement.md",
      "leadDeltaReview": "Native self-test passed but the first Full run exposed an obsolete source guard. Lead reviewed the narrow fixture/creation-time allowance and all retained bans. Focused 7/7 passes; independent AST fixture passes current source and rejects unrelated handles and removal of the identity barrier. Runtime/whole-profile results remain separate."
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-InteractiveMode.ps1",
      "sha256": "437e6e4f055b2aa7154aa8226a74735a9c3b28d5ca13ced4950761a5d4acd462",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "a run without -Suites takes Menu and NativeTextInput, and the fixtures only when they are named",
        "approved automatic starts retain their mode, bounded countdown and distinct result",
        "a refused lease preserves raw prior output without qualifying it"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests test.ps1 and InteractiveRun.psm1 policy, lease protocol, result reader and hosted ARM preparation using fake commands and synthetic receipts.",
        "ownershipAndLifetime": "Fixture roots and fake scripts are removed via TestSupport; environment markers are saved/restored in finally. No real desktop takeover is initiated by this script.",
        "failureAndCancellation": "Refusal paths assert no build/log/output writes; structured lease results preserve child identity, restoration states and cancellation/timeout; stale results are removed before launch and raw old logs are retained separately.",
        "testCoverage": "Full 946-line review covers suite routing, authorization, timeouts, result protocol, exact child names, restoration presentation, native console behavior, prior-log preservation, hosted wrapper and ARM guard. No test run."
      },
      "plannedSha256": "cc2d8a520ceb7a436a1a652352c8477fdcef6f05c4c7004d68afa54931ae0122",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-MarkdownData.ps1",
      "sha256": "8a65257dbb780d6849c3657a571e05b032acfc1ab116d3936fb7729d3cee48fe",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-MarkdownData.ps1"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; regeneration and stale/missing-pin coverage is explicit; no tooling defect confirmed.",
      "reviewNotes": {
        "callers": "Offline generator workflow invokes this script as a focused fixture test.",
        "ownershipAndLifetime": "Temporary inputs and outputs are created under the fixture root supplied by Invoke-FixtureCase.",
        "failureAndCancellation": "Test asserts missing/stale pins fail before output and preserves changed artifacts for diagnosis.",
        "testCoverage": "Covers deterministic output for four generated datasets, immutable check mode, and all declared data/license pins. This agent did not execute it."
      },
      "plannedSha256": "31fc079ffc669d665aea9537db714b7e8412c0197d7da4871889fb3b0cd8e035",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-NativeScope.ps1",
      "sha256": "0589fa66913e047b5de0d948dec304b95f9548540d3a77d82538ba8607206057",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "docs-only paths classify separately and measurements remain native",
        "tracked production/test/build/tool/doc paths match scope classification",
        "fixture PR merge refs classify changed paths and malformed or missing pairs fail",
        "scope refuses uncommitted worktree and unavailable repository inputs",
        "CI and format workflows gate on native scope with independent producers",
        "six native profiles declare bounded per-stage budgets",
        "consumer and ASan matrix annotations are explicit",
        "falsified short timeouts and truncated historical budgets fail",
        "required native stages run once in order with no failure bypass",
        "Complete-TestRun NativeScope"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Test-NativeScope.ps1 is itself registered in the tooling test suite; it invokes the NativeScope module and fixture scripts against synthetic Git repositories.",
        "ownershipAndLifetime": "Git fixtures are created with repository-local test support and removed under fixture ownership; Invoke-ScopeScript snapshots and restores GITHUB environment variables. Git subprocesses are scoped to each fixture with -C.",
        "failureAndCancellation": "Checks unavailable/malformed commit pairs and uncommitted repositories fail closed; workflow aggregate prerequisites, time limits, and required test ordering are falsified to ensure they cannot silently pass.",
        "testCoverage": "Fixture and direct cases cover docs-only vs source/build/tool/test/document paths, unknown/unavailable paths, actual tracked-path classification, merge-base PR metadata and malformed inputs, CI trigger/workflow producer dependencies, six native budgets/configuration/timeouts, consumer limits, ASan annotation, sequential suite ordering and no duplicate Full build."
      },
      "plannedSha256": "3f49f43fa4f1c4d453624188755f3e501e8936656f39e8a29505fc4905b29f82",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PairedRun.ps1",
      "sha256": "bc4d58b3fd5cd71d1f91f72e2ccc0a282f6dbf466a6b5ae96a651c924d94a889",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "independent orders retain reproducible prefixes and balance runs within every block",
        "A/A assigns both randomized labels to one root and one build, while ordinary pairs keep two roots",
        "overlay rollback preserves later edits to created and replaced paths while restoring unaffected files",
        "paired preflight rejects absent or changed compiler, dependency, fixture and environment identities"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests PairedRun and PerformanceComparison module behavior plus performance-paired.ps1 via parsed/extracted functions; current version includes the newer randomized independent-block and A/A cases.",
        "ownershipAndLifetime": "Fixture Git trees are created in scoped roots. Overlay records retain hashes/backups and restore only owned edits; rollback refuses to overwrite later mutations and preserves backups. A/A maps roles to one physical build root while ordinary comparisons keep two.",
        "failureAndCancellation": "Changed/missing overlay bytes, mutated backup, incompatible identities, same tree/commit, incomplete restore and changed benchmark project all fail before measurement; partial timestamp staging is rolled back.",
        "testCoverage": "Named cases cover schedule reproducibility and balance, A/A same artifact, legacy include aliasing, overlay attestation/rollback, source fingerprints (non-ASCII), exact measured base judge bytes and build preflight. No tests run."
      },
      "plannedSha256": "e8a6acd3e5446fdb15fc35285caa0f610f5f1e65f08d2088cea6332b8fe31487",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceComparison.ps1",
      "sha256": "9e86b6336d70b3b8889f5e0fe4ed19e96fc4a6d6ffeb80be831fda42f75a43c1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "the migrated block judge uses identity, independent paired effects and separate exact budgets",
        "the exact rank test counts ties as an enumeration of every rank assignment does",
        "every stored paired comparison is reproduced exactly",
        "every stored unpaired receipt still validates"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests receipt validators, exact/rank judges, pair-set comparator and Compare-Performance.ps1 using generated receipts plus stored Measurements fixtures.",
        "ownershipAndLifetime": "All writable command output is fixture-scoped; stored measurement/comparison artifacts are read-only. Deterministic pseudo-random sequences seed statistical cases.",
        "failureAndCancellation": "Rejects malformed timing diagnostics, nonfinite metrics, hidden work, identity mismatch, missing rounds, mixed binary/fixture provenance and exact-budget increases. JSON status and exit codes are asserted.",
        "testCoverage": "Full 635-line review covers legacy comparison parity, exact sign-flip/Holm inference, rank ties/large counts/reference example, medians/spread/bands, exact budgets, and every stored paired/unpaired artifact. No test run."
      },
      "plannedSha256": "4c0e76280ab378b9bc031da114138cb28f64ec23074615c9c8474c14c01001f4",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceForms.ps1",
      "sha256": "8f12c7da8d4533eeb2b0a82a1d35e63ad79c6433d66f27aff065ea46ac5ef1eb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": "Test-PerformanceForms.ps1",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "03c6d6bd14e81b83fe5f1c52238ef1ae0a21c7ce5225605c1cec81f035280e7e",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformancePolicy.ps1",
      "sha256": "ce22139a1a93e10901b75b76b8bd51c27db3afe315aa56163cd7ab8e725cf6df",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": "Test-PerformancePolicy.ps1",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "9ec18c5e43d920fd016a3476a73b5e4eb61429d09b8a3dd8bef827ed44f2c0c2",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceProfiling.ps1",
      "sha256": "c7f2ed7265f100b2d516abc1b8401d9a17d1c30d83623e280641eea3a4580882",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": "Test-PerformanceProfiling.ps1",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Called by the tooling dispatcher; additionally targets the diagnostic benchmark/report APIs and retains its own child-process fixtures in repository-local test support.",
        "ownershipAndLifetime": "Synthetic report fixtures isolate mutable files; owned child tests retain explicit PID/process-handle lifecycle and bounded redirected-output draining; instrumentation and capture artifacts remain fixture-owned.",
        "failureAndCancellation": "Checks schema rejection, bounded diagnostic instrumentation/PNG capture, exact raw-report preservation, direct child timeout, and inherited-pipe drain timeout. A descendant can outlive the direct child by design in this generic helper test and is explicitly terminated by the test using recorded process identity; this is not an interactive lease finding.",
        "testCoverage": "Test-PerformanceProfiling.ps1 contains synthetic schema/capture cases and real owned-child success/failure/timeout/pipeline cases. Read only; not run in this review."
      },
      "plannedSha256": "59e1cc732d6c3a7f29f6ae0bacd68b7e48023d986061b2bc6db1fdd76c517c36",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-RunEvidence.ps1",
      "sha256": "3e4cdfe9a3f6a8952386a12e016e79a08eb91786a804778ea18483c7cc6409e2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "concurrent evidence directories are unique",
        "successful evidence is immutable",
        "previous receipt and alternate stream survive rejected run",
        "path escape and unsupported root shapes are rejected",
        "writing payload and linked ancestors/payloads are rejected",
        "directory identity is checked across rename and replacement",
        "run owner is validated",
        "shared read-only hard link is accepted",
        "abrupt child exit retains partial evidence without completion marker",
        "later run preserves interrupted evidence",
        "native output capture has an exclusive lease and failed output remains",
        "stale provider metadata is checked against actual inventory"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Runs under the governed tooling test entrypoint; imports TestSupport.psm1 and tests evidence/run receipt operations through repository tooling functions.",
        "ownershipAndLifetime": "Fixture roots are created through New-FixtureRoot and isolated below the fixture scratch root; success evidence is checked immutable and interrupted partial evidence is intentionally retained for recovery. The script also captures/restores process environment around scoped execution.",
        "failureAndCancellation": "Rejects path escapes, drive-root and relative roots, writing handles, linked ancestors/payloads, identity substitution, invalid run owners, and stale metadata. Abrupt child exit is treated as interrupted evidence without publishing a completion marker; no production process is targeted.",
        "testCoverage": "Invoke-FixtureCase cases cover concurrent unique evidence directories, success immutability and prior receipt/ADS preservation, invalid roots/open payloads/reparse links, rename/replacement identity, owner rejection, hard-link read-only behavior, child Environment.Exit(17) interruption and later-run preservation, exclusive native-output capture, and stale provider metadata vs ordered observed inventory."
      },
      "plannedSha256": "3e4cdfe9a3f6a8952386a12e016e79a08eb91786a804778ea18483c7cc6409e2",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-ScopedTesting.ps1",
      "sha256": "84af30b91e6cae7974f513121320de122a67c0e61d623d624fd561e9c308adaf",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-ScopedTesting.ps1: scope and interactive obligations",
        "Test-ScopedTesting.ps1: source/artifact/dependency identities and stale receipt rejection",
        "Test-ScopedTesting.ps1: PR delegation contract and runner lifecycle"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Tooling dispatcher invokes this PowerShell test; it imports Tools/ScopedTesting.psm1 and the public Test-Changes.ps1 runner in a repository-local fixture.",
        "ownershipAndLifetime": "Creates a unique fixture below .build/ToolTests, a nested isolated Git repository and dependency/artifact files; snapshots PATH and instrumentation environment and restores them in finally; cleanup verifies the resolved fixture remains below the owned root.",
        "failureAndCancellation": "Exercises invalid paths/rules, stale/corrupt receipts, source/library/runtime/dependency mutation, concurrent PR state changes, capability skips and post-run mutation. Nested fake public-runner subprocesses have no explicit WaitForExit timeout; fixture commands are deterministic local PowerShell only.",
        "testCoverage": "Named cases cover scalar/batched globs, interactive obligations, all six profiles, clean/staged/unstaged/deleted/renamed paths, identity closure, platform-specific vcpkg evidence, PR delegation, and cached runner failure. Read only; not executed."
      },
      "plannedSha256": "6930ecce087d0cf9e52b1f1bdb4c7435655f5dba2d6a7da065b30d8c1bffc4f9",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-SuiteFailure.ps1",
      "sha256": "5c4159c466d614bcc83dd5a8c411275fb14fec92fc1644f340cda0a906859201",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "a run the watchdog ended is reported with its exit code and the hung test",
        "each capability skip is named after the test it belongs to"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Tests SuiteFailure.psm1 parsing and test.ps1 reporting with fixture logs.",
        "ownershipAndLifetime": "Fixture log files are temporary; no child processes or global state are retained.",
        "failureAndCancellation": "Separates watchdog timeout from ordinary failure, handles missing logs and exit codes, and associates skip lines with test/suite context.",
        "testCoverage": "Named cases include timeout line outside tail, non-timeout failure classification, missing log, multiple skips, and test.ps1 reporting integration. No tests run."
      },
      "plannedSha256": "5c4159c466d614bcc83dd5a8c411275fb14fec92fc1644f340cda0a906859201",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestFilter.ps1",
      "sha256": "2a1608b9382a10be8ca4c7136af8ef80be93c597191c428ce8bf1a57cbcf0834",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "the filter runs only the named tests, in suite order",
        "an unknown name fails the run instead of passing with nothing run",
        "without the filter a suite runs every test its runner registers"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Runs the built DxUi.ControlTests.exe from repository root using named suites and exact --test arguments.",
        "ownershipAndLifetime": "Uses only the built test executable and captures streams; no fixture files are written. Assumes binary already exists and fails if absent.",
        "failureAndCancellation": "Unknown or malformed names exit usage code before test start; fixture suites reject named filters; invocation restores current location in finally.",
        "testCoverage": "Named cases check exact/case-sensitive filtering, partial unknown lists, empty/trailing/delimited names, no direct test calls in suite runners, and all registered Grid tests starting in order. No executable was run."
      },
      "plannedSha256": "2a1608b9382a10be8ca4c7136af8ef80be93c597191c428ce8bf1a57cbcf0834",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestPort.ps1",
      "sha256": "abdd1e7d3990a1ba148ae83357a8d5d40ad863e4e4de3090da933a28e8477d23",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "a retained case is accounted for",
        "silently deleting a case fails",
        "a current runtime replacement passes",
        "a duplicate policy fails"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Exercises Test-DxUiTestPort in Validation.psm1 with synthetic manifest, source and disposition policy files.",
        "ownershipAndLifetime": "Each fixture root is owned by Invoke-FixtureCase; all source/policy content is synthetic.",
        "failureAndCancellation": "Missing/deleted current cases, dropped dispositions, unexplained exclusion, missing replacement and duplicate policy all fail closed; explicit source rename and contextual currentFile retain historical origin.",
        "testCoverage": "Ten cases cover retained, deleted, discarded, renamed, excluded and runtime-replaced rows. No test run."
      },
      "plannedSha256": "abdd1e7d3990a1ba148ae83357a8d5d40ad863e4e4de3090da933a28e8477d23",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestWatchdog.ps1",
      "sha256": "af65119f893743c4140a4d6a0d8442769f3ffc1f8c30eea6e1c635c40fce4dfd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "a test that never returns ends the run at its deadline, named",
        "an armed watchdog waits instead of polling",
        "test.ps1 surfaces the timeout: the exit code and the TIMEOUT line among the printed lines"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Starts bounded DxUi.ControlTests.exe runs and consumes SuiteFailure reports; test.ps1 invokes this after build.",
        "ownershipAndLifetime": "Owns/disposes its child process; redirects/drains both streams and uses Kill(true) only for the bounded process it started. CPU probe child is always ended in finally.",
        "failureAndCancellation": "Tests watchdog timeout, disabled deadline, progress extension, idle waiting, malformed args and usage refusal under outer bounds so a broken inner watchdog cannot hang the script.",
        "testCoverage": "Named cases cover hung named test/fixture, progress reset, no-watchdog behavior, CPU idleness, 300-second default, durations, malformed values, runtime failure report, test.ps1 timeout summary and Menu-driver cleanup guard. No run performed."
      },
      "plannedSha256": "af65119f893743c4140a4d6a0d8442769f3ffc1f8c30eea6e1c635c40fce4dfd",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-UnicodeGraphemeData.ps1",
      "sha256": "d815c57f1df99048eae228338b943e018aaba8e9bac5eadff5243f59daa51ec3",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-UnicodeGraphemeData"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Runs Generate-UnicodeGraphemeData.ps1 in generate/check modes against pinned Unicode inputs and generated headers.",
        "ownershipAndLifetime": "Uses repository-tracked input/output paths; check mode snapshots timestamps and asserts read-only behavior. Missing-input cases use fixture directories.",
        "failureAndCancellation": "Rejects missing/stale generated outputs and altered input/license hashes before rewriting outputs; exact bytes are compared.",
        "testCoverage": "Checks deterministic regeneration of both headers, check-mode timestamp preservation and fail-closed missing/stale/hash cases. No generator was run during this review."
      },
      "plannedSha256": "5316963582c72f6a20121d30102aa93a5d1d6935c4dd94138fe1ce8421a8832b",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-Validation.ps1",
      "sha256": "629c3ae4201d298903bb86f5fd02dd830a8a7c20a1a88aa42bd8efa58194b141",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": "Test-Validation.ps1",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "callers": "Fully read; concrete registration/call site identified in source or consumer.",
        "ownershipAndLifetime": "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
        "failureAndCancellation": "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
        "testCoverage": "Actual test IDs listed; tests were not executed in this review."
      },
      "plannedSha256": "629c3ae4201d298903bb86f5fd02dd830a8a7c20a1a88aa42bd8efa58194b141",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-VcpkgTriplet.ps1",
      "sha256": "26c947b5d024ed41a0522cd4de34f4b242a5f29d22f540f91ce31473af61d6ba",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "default toolset version parsing handles BOM and line endings",
        "missing version file fails with repair guidance",
        "malformed and oversized version file is rejected",
        "overlay preserves stock text and appends exact pins",
        "empty and no-final-newline stock triplets form valid overlays",
        "paths are escaped for CMake round-trip",
        "CMake reads stock settings and both pins",
        "overlay changes only when effective inputs change",
        "missing stock triplet writes nothing",
        "vcpkg-install keeps interface and fails before cloning",
        "build and install consumers share VS discovery",
        "installer modules parse under PowerShell 5.1"
      ],
      "findingIds": [],
      "disposition": "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
      "reviewNotes": {
        "callers": "Invoked by the tooling suite; imports VisualStudio.psm1 and VcpkgTriplet.psm1 and parses the public vcpkg-install.ps1/build.ps1 consumer surface.",
        "ownershipAndLifetime": "Synthetic Visual Studio/vcpkg fixture files use Invoke-FixtureCase-managed roots; no actual checkout, vcpkg restore, or network is needed. CMake and Windows PowerShell 5.1 are optional executable probes and skip when absent.",
        "failureAndCancellation": "Missing/malformed version files and absent stock triplets fail with repair/path context before clone/bootstrap/run; overlay generation rejects unsupported patch pins/empty install paths and preserves stock text.",
        "testCoverage": "Direct and fixture cases cover version parsing/normalization/BOM/line endings/length, CMake escaping and pin readback, stable no-op timestamps, changed installation/toolset/stock triplet rewrites, missing triplet no-write, unchanged installer interface/early discovery/order, shared discovery callers, ASCII and optional PowerShell 5.1 parsing."
      },
      "plannedSha256": "26c947b5d024ed41a0522cd4de34f4b242a5f29d22f540f91ce31473af61d6ba",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/TestSupport.psm1",
      "sha256": "7b85c5f706cfd12e2381d831ff10a2d28712c89e2bc670f2019815e3306916ef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Invoke-TestCase",
        "Invoke-FixtureCase",
        "Remove-FixtureRoot"
      ],
      "findingIds": [],
      "disposition": "Full source-backed read completed; no tests executed. Limitations and any identified coverage gaps are stated in reviewNotes.",
      "reviewNotes": {
        "callers": "Imported by tooling test scripts for assertions, case aggregation, isolated fixtures, JSON and SHA helpers.",
        "ownershipAndLifetime": "Fixture parent is repository .build/tooling-fixtures; Invoke-FixtureCase creates a fresh child and Remove-FixtureRoot cleans it in finally with a containment check.",
        "failureAndCancellation": "Invoke-TestCase accumulates failures instead of stopping the suite; fixture teardown always runs. Cleanup resolves paths and refuses deletion outside the owned fixture parent.",
        "testCoverage": "Reviewed all 99 lines and exported helper surface; no dedicated case here, coverage comes from all calling scripts. No test run."
      },
      "plannedSha256": "7b85c5f706cfd12e2381d831ff10a2d28712c89e2bc670f2019815e3306916ef",
      "reviewEvidence": "review-general-tools-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/validate_consumer.ps1",
      "sha256": "b8f4882a1c5183250dd5437570e696ac995bad04c46d4575cafc0a65fe97a82a",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-ConsumerUpdate"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-ConsumerUpdate.ps1 covers consumer pin/API/checkouts, including tamper cases; direct script failure boundary coverage is in that harness.",
        "failureAndCancellation": "Any mismatch or git failure throws before consumer build proceeds.",
        "callers": "Called by Tools/ConsumerBuild.psm1 after checkout; verifies canonical repository, exact commit, clean checkout, API revision and sole DxUi target.",
        "ownershipAndLifetime": "Reads lock, git state and capabilities; creates no external state."
      },
      "plannedSha256": "b8f4882a1c5183250dd5437570e696ac995bad04c46d4575cafc0a65fe97a82a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Validation.psm1",
      "sha256": "15c7ef95430c293400859446cd51e2202bbe5d0ab1ac506b8539ce1de983e756",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Validation",
        "Test-Docs",
        "Test-TestPort",
        "Test-BuildMatrix"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-Validation.ps1 covers dependency/skills/shared validation, Test-Docs exercises spec walk, Test-TestPort inherited mapping and Test-BuildMatrix configuration mapping. No dedicated cancellation/process-launch failure test identified for every validator.",
        "failureAndCancellation": "Invalid/missing JSON, XML, path, provenance, pin, inherited-test or config data is surfaced as explicit validation failure; orchestration continues later steps while recording each failure.",
        "callers": "Imported by validate.ps1 and all validation wrapper scripts; functions are consumed by Docs, Dependency, Skill, TestPort and six-configuration build-matrix entrypoints.",
        "ownershipAndLifetime": "Stateless validation helpers normalize/read project inputs and return findings; wrappers create independent PowerShell processes through Invoke-DxUiValidation to prevent Force imports tearing down caller module state."
      },
      "plannedSha256": "15c7ef95430c293400859446cd51e2202bbe5d0ab1ac506b8539ce1de983e756",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/VcpkgTriplet.psm1",
      "sha256": "7bb17cc7dd2377e1eb4cc62de710b7ecff9e614ff7b6dc31cd32ccc47af488b7",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1: overlay bytes, escaping, line endings and stable timestamp"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tests stock-text preservation, quotes, CRLF/LF, no-op timestamp, toolset/path/source changes; no vcpkg restore.",
        "callers": "vcpkg-install.ps1 renders overlay from stock triplet plus VS/toolset pins.",
        "failureAndCancellation": "Missing stock triplet and malformed version are rejected; identical bytes preserve timestamp.",
        "ownershipAndLifetime": "Pure text builder and write-if-bytes-differ into caller output; ABI-triggering rewrite occurs only on change."
      },
      "plannedSha256": "7bb17cc7dd2377e1eb4cc62de710b7ecff9e614ff7b6dc31cd32ccc47af488b7",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/VisualStudio.psm1",
      "sha256": "bf2ac054c30feaea2b8cc1322fc1d67854660e90fc2dcce17e26ee21ad6071fd",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1 and Test-BuildMatrix.ps1: VS installation/default-toolset fixtures"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Mocked missing/malformed/BOM/version fixtures; actual installed toolchain not invoked.",
        "callers": "build.ps1, test-consumer.ps1, vcpkg-install.ps1 and Test-AsanRuntime.ps1 share discovery/default toolset.",
        "failureAndCancellation": "Explicitly fails on missing vswhere/VS/default version file or malformed toolset before restore.",
        "ownershipAndLifetime": "Read-only vswhere and default version-file discovery, no install/write."
      },
      "plannedSha256": "bf2ac054c30feaea2b8cc1322fc1d67854660e90fc2dcce17e26ee21ad6071fd",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/Watch-BenchmarkHost.ps1",
      "sha256": "dd31f0ceff0a383fee1dc971893a85ad803a6962d6ad2c9f0b34007dd156807d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkHost"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-BenchmarkHost.ps1 covers stop, completion, sampling, preexisting artifact refusal and failure limit; no independent test of collection overhead quantification.",
        "failureAndCancellation": "Win32 read failures/errors propagate; max sample exhaustion produces failed completion; stop file terminates cleanly; process access/exited cases are labeled.",
        "callers": "CI paired benchmark starts it as a background job, then requests stop via sidecar; observer records host/process snapshots without judging benchmark samples.",
        "ownershipAndLifetime": "Owns StreamWriter, disposes every Process snapshot; creates output and ready/completion sidecars, refuses preexisting paths."
      },
      "plannedSha256": "dd31f0ceff0a383fee1dc971893a85ad803a6962d6ad2c9f0b34007dd156807d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate-build-matrix.ps1",
      "sha256": "621dc39261b63c43ab6c2b4ea532e5340969910bc96d5e5ab5acad672f532a44",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BuildMatrix"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 exercises six-configuration project and solution mapping; no standalone wrapper process contract assertion located.",
        "failureAndCancellation": "Module throws findings through Complete-DxUiValidation. XML/solution read failures fail closed.",
        "callers": "Called by validate.ps1 through Get-DxUiValidationSteps and imports the shared Validation module; no private state.",
        "ownershipAndLifetime": "Only resolves repository root and delegates to Test-DxUiBuildMatrix; no owned resources."
      },
      "plannedSha256": "621dc39261b63c43ab6c2b4ea532e5340969910bc96d5e5ab5acad672f532a44",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate-dependencies.ps1",
      "sha256": "c6dffa8774dadfbe63ed9e3e5889eeae1993570edd74f01e08d40b87ce687bb1",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Validation"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-Validation.ps1 directly exercises Test-DxUiDependencies; no explicit subprocess test for this wrapper.",
        "failureAndCancellation": "Validation result failures throw; malformed/missing inputs fail via shared reader.",
        "callers": "Called by validate.ps1; imports Validation and delegates source provenance/pin checks at repo root.",
        "ownershipAndLifetime": "No state beyond module invocation."
      },
      "plannedSha256": "c6dffa8774dadfbe63ed9e3e5889eeae1993570edd74f01e08d40b87ce687bb1",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate-skills.ps1",
      "sha256": "cce3e47124352398a60bb7c79f286341e659ba00b0db853aa8672c72edf6253f",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Validation"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-Validation.ps1 covers Test-DxUiSkills with valid/invalid metadata; wrapper process behavior not independently asserted.",
        "failureAndCancellation": "Validation errors throw; malformed skill metadata is reported by module.",
        "callers": "Called by validate.ps1; delegates repository skill frontmatter checks.",
        "ownershipAndLifetime": "No retained state."
      },
      "plannedSha256": "cce3e47124352398a60bb7c79f286341e659ba00b0db853aa8672c72edf6253f",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate-specs.ps1",
      "sha256": "139b7257056fb441be1dbdc36649886b9c86b235c0ffb8707078ad073d7d889a",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-Docs"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-Docs.ps1 exercises Test-DxUiSpecs on temporary trees and invalid references; wrapper itself not process-tested.",
        "failureAndCancellation": "Shared validation reports missing/invalid references as failures.",
        "callers": "Called by validate.ps1; delegates specification authority/index/reference validation.",
        "ownershipAndLifetime": "No retained state."
      },
      "plannedSha256": "139b7257056fb441be1dbdc36649886b9c86b235c0ffb8707078ad073d7d889a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate-test-port.ps1",
      "sha256": "ed545a8aca8b290b041bbed5de84398a990ff7fbc7b8b1e9c13cd2674502c2b3",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-TestPort"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tools/tests/Test-TestPort.ps1 exercises Test-DxUiTestPort; wrapper itself not separately process-tested.",
        "failureAndCancellation": "Shared validator throws if port ledger has invalid/missing disposition or replacement.",
        "callers": "Called by validate.ps1; delegates inherited test disposition audit.",
        "ownershipAndLifetime": "No owned resources."
      },
      "plannedSha256": "ed545a8aca8b290b041bbed5de84398a990ff7fbc7b8b1e9c13cd2674502c2b3",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "validate.ps1",
      "sha256": "68ba34c47acccc7742b0884ae78d8de69f7f9a6c772c74c53c4ef00373d1d2b2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-Validation.ps1 and CI validation job"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Test-Validation.ps1 verifies ordered validator list and continue-after-failure aggregation; no validation suite run here.",
        "callers": "Top-level validator invokes all five independent validators and tooling tests via Validation module and reports each result.",
        "ownershipAndLifetime": "Read-only checks; collects all results before final fail rather than leaving early after a failed validator.",
        "failureAndCancellation": "Any failed validator raises at end; formatter check remains separate by contract."
      },
      "plannedSha256": "68ba34c47acccc7742b0884ae78d8de69f7f9a6c772c74c53c4ef00373d1d2b2",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-configuration.json",
      "sha256": "7c8f088b33838d9f1c6481d2120bcf8445cdee45232779fbb9b20964caa54de3",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1 and external consumer restore path"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Configuration consumed by actual restore contracts, but no restore run here.",
        "ownershipAndLifetime": "Static config consumed by vcpkg install under repository .build or consumer-owned output root.",
        "callers": "vcpkg manifest configuration disables overlay ports; dependency selection remains the pinned baseline and WIL manifest.",
        "failureAndCancellation": "No added port override path; invalid dependency resolution fails in vcpkg restore."
      },
      "plannedSha256": "7c8f088b33838d9f1c6481d2120bcf8445cdee45232779fbb9b20964caa54de3",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-install.ps1",
      "sha256": "be905aeb852327bf4823d8ce82c6e3cf10103980767f9044ce6675ad67207fec",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1: installer interface, fail-before-clone, shared VS discovery, overlay path and PS5.1 ASCII/parse contracts"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Focused Test-VcpkgTriplet source/fixture tests; real restore not run here.",
        "ownershipAndLifetime": "All checkout/download/build/package/install roots live below caller build root; private per-platform install roots; immutable commit pin; overlay generated from pinned stock triplet.",
        "callers": "build.ps1, test-consumer.ps1 and paired runner invoke vcpkg restore; VisualStudio and VcpkgTriplet modules centralize selection and generated triplet.",
        "failureAndCancellation": "Rejects malformed lock, missing git/manifest/version file, wrong remote, dirty checkout, clone/fetch/checkout/bootstrap/install failures; toolchain validated before network clone."
      },
      "plannedSha256": "be905aeb852327bf4823d8ce82c6e3cf10103980767f9044ce6675ad67207fec",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-tool.json",
      "sha256": "e38884614404c881238070ce5ae2c25a72a68718a3b03c8dccb26fa22586756e",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1: vcpkg installer source/pin checks"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Tooling fixtures cover installer source/ordering contracts; no network restore run.",
        "ownershipAndLifetime": "Static lock for managed .build/vcpkg-tool clone; not a generated product artifact.",
        "callers": "Pins canonical microsoft/vcpkg repository at an exact full commit; consumed by vcpkg-install.ps1.",
        "failureAndCancellation": "Installer checks full 40-hex commit, remote identity, clean checkout and exact HEAD before bootstrap/install."
      },
      "plannedSha256": "e38884614404c881238070ce5ae2c25a72a68718a3b03c8dccb26fa22586756e",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "vcpkg.json",
      "sha256": "fe54dd43ba2d384eee8a0034daa3e22f0204ffc4390be1cd8eb8456e295b6bfc",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Tools/tests/Test-VcpkgTriplet.ps1 and test-consumer.ps1: manifest restore/copy contract"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Consumer/vcpkg test sources cover manifest root and isolated output, but not dependency restore in this review.",
        "ownershipAndLifetime": "vcpkg manifest consumed by per-platform or consumer-owned isolated installs.",
        "callers": "Names DxUi manifest, exact builtin baseline matching vcpkg-tool commit, one WIL dependency with minimum version.",
        "failureAndCancellation": "Pinned baseline prevents floating package recipes; restore fails on unavailable/incompatible package."
      },
      "plannedSha256": "fe54dd43ba2d384eee8a0034daa3e22f0204ffc4390be1cd8eb8456e295b6bfc",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.AnimationClock.cpp",
      "sha256": "401851eb15ddaef3469bd72e8cda656b24874033404a3fa6a0a7b5d518b2ba94",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestEmbeddedButtonHoverTransitionAtAnyEpoch",
        "TestEmbeddedSliderVisualTransitionsAtAnyEpoch",
        "TestEmbeddedCaretRestartsVisibleAndBlinksAtAnyEpoch",
        "TestEmbeddedTreeExpansionAnimatesAndCompletesAtAnyEpoch",
        "TestEmbeddedGridSortGlyphSettlesAtAnyEpoch",
        "TestEmbeddedFirstTooltipHonorsItsDelayAtAnyEpoch",
        "TestEmbeddedAnimationClockSkipsPausesInTicking",
        "TestEmbeddedAnimationClockContinuesAcrossAttachments",
        "TestNativeAnimationClockIsTheDispatcherTimeline",
        "TestNativeTransitionStartsOnTheDispatcherTimelineAfterAStall"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Tests instantiate controls under EmbeddedHost or AttachedHostWindow and drive AdvanceAnimation or dispatcher ticks. ClockCase covers small, 32-bit rollover-adjacent, trillion-ms, near-int64, and uint64-max epochs; only focused native stall case uses real Sleep.",
        "ownershipAndLifetime": "GraphicsFixture, animation pool, host, root/control, and model owners are stack/unique_ptr scoped. Tests detach/re-attach explicitly, and native window helper detaches host before releasing HWND. No raw retained control beyond owning tree lifetime in reviewed tests.",
        "failureAndCancellation": "Assertions check transitions start at zero despite arbitrary epoch, settle without idle perpetual ticks, ignore hidden/zero-size/backward time, and continue across attachment without jumping. Native stall test uses dispatcher hitch clamp and observes a subsequent slider key transition; no cancellation-specific assertion.",
        "testCoverage": "Ten registered declarations provide deterministic embedded epoch coverage plus native dispatcher source and stall recovery. The stall test is wall-clock/dispatcher dependent and can be timing-sensitive; no build or runtime result is claimed."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "401851eb15ddaef3469bd72e8cda656b24874033404a3fa6a0a7b5d518b2ba94",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.AnimationClock.h",
      "sha256": "cffd8969ed58499c312f3ebb0805aaa9ff753f51bf8c58251c586c994756e12e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "TestEmbeddedButtonHoverTransitionAtAnyEpoch",
        "TestEmbeddedSliderVisualTransitionsAtAnyEpoch",
        "TestEmbeddedCaretRestartsVisibleAndBlinksAtAnyEpoch",
        "TestEmbeddedTreeExpansionAnimatesAndCompletesAtAnyEpoch",
        "TestEmbeddedGridSortGlyphSettlesAtAnyEpoch",
        "TestEmbeddedFirstTooltipHonorsItsDelayAtAnyEpoch",
        "TestEmbeddedAnimationClockSkipsPausesInTicking",
        "TestEmbeddedAnimationClockContinuesAcrossAttachments",
        "TestNativeAnimationClockIsTheDispatcherTimeline",
        "TestNativeTransitionStartsOnTheDispatcherTimelineAfterAStall"
      ],
      "findingIds": null,
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "reviewNotes": {
        "callers": "Declares ten animation-clock tests implemented by Controls.Tests.AnimationClock.cpp; DxUi.Tests.Runner.cpp invokes the suite. EmbeddedHost and WindowHost exercise the shared dispatcher timeline.",
        "ownershipAndLifetime": "Declarations carry no ownership; implementations create local GraphicsFixture, animation pool, EmbeddedHost, AttachedHostWindow, and tree/control models. Lifetimes are scope-bound; attached host helper detaches before HWND destruction.",
        "failureAndCancellation": "Tests assert skipped/hidden/zero-sized time does not advance embedded animation and backward time is ignored. Native dispatcher stall case sleeps a bounded multiple of hitch clamp; no explicit cancellation path is applicable.",
        "testCoverage": "Names cover hover, slider, caret, tree, grid glyph, tooltip, paused clock, reattachment monotonicity, native clock source, and post-stall transition. Actual execution not performed."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "cffd8969ed58499c312f3ebb0805aaa9ff753f51bf8c58251c586c994756e12e",
      "reviewEvidence": "review-general-controls-supplement.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.BenchmarkInstrument.h",
      "sha256": "d4cce9201fb0fa080e26b0cad623a64198e00b4d3a28bb9190aafb6b864d8153",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Embedded allocation-count scenarios",
        "Embedded allocation-failure sweeps"
      ],
      "findingIds": [],
      "disposition": "No concrete defect found; documented live-byte metric excludes aligned forms, so reports must retain that boundary.",
      "reviewNotes": {
        "callers": "Included from exactly one translation unit, Embedded.Tests.Embedded.cpp; benchmarks access thread-local plain/allocation counters and opt-in allocation failure injection.",
        "ownershipAndLifetime": "Defines global replacement new/delete operators; thread-local counters avoid cross-thread interference and liveBytes measures plain forms only by documented scope. Aligned allocations are counted but excluded from liveBytes.",
        "failureAndCancellation": "Injected allocation failure throws std::bad_alloc at chosen count; allocation primitives throw on malloc/aligned allocation failure. This is a test executable-wide instrumentation shim.",
        "testCoverage": "GridSelection and ComplexUi benchmark paths consume counts; native allocation-failure sweeps exercise failAllocationAfter; compile/link inclusion exactly once is documented to avoid duplicate operator definitions."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "d4cce9201fb0fa080e26b0cad623a64198e00b4d3a28bb9190aafb6b864d8153",
      "reviewEvidence": "review-general-tests-current.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-CodeOwners.ps1",
      "sha256": "70bbd429ae99c73f25acd83a1d2215cc1b5453990278c6d3614814c42754e768",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-CodeOwners"
      ],
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "reviewNotes": {
        "testCoverage": "Four actual cases: matcher semantics, single canonical location, valid owners/tracked paths and protected paths owner coverage.",
        "failureAndCancellation": "Unsupported patterns, untracked patterns, unreviewed owners and protected unowned paths fail test.",
        "callers": "Listed in Invoke-ToolingTests.ps1; reads .github/CODEOWNERS and tracked path list to assert repository owner rules.",
        "ownershipAndLifetime": "Pure fixture/read-only logic; tracked paths queried through UTF-8 git subprocess; process disposed."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "70bbd429ae99c73f25acd83a1d2215cc1b5453990278c6d3614814c42754e768",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A2-vs-A1-control.comparison.json",
      "sha256": "21dce5ab319123f6117a2dbaae18b1e0204ddf9d1f579cf36595420d89773134",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "21dce5ab319123f6117a2dbaae18b1e0204ddf9d1f579cf36595420d89773134",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A4-vs-A3-control.comparison.json",
      "sha256": "a0f8a16bb190a4f2fadf866609531e606b7557a03a659ddd038981d5e6eb7935",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "a0f8a16bb190a4f2fadf866609531e606b7557a03a659ddd038981d5e6eb7935",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A6-vs-A5-control.comparison.json",
      "sha256": "c717ad2cca2385eecb30e5e799aeb16a1c3cc0fae83ee06308e29485230dc06a",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "c717ad2cca2385eecb30e5e799aeb16a1c3cc0fae83ee06308e29485230dc06a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B2-vs-B1-control.comparison.json",
      "sha256": "0737d81af29842f6d2999c3dded97ef8a8aaeba3bf0d873dae01cb731cfd9362",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "0737d81af29842f6d2999c3dded97ef8a8aaeba3bf0d873dae01cb731cfd9362",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B4-vs-B3-control.comparison.json",
      "sha256": "0e60ddcdb695d8138d8e52f7f268f16e2545b343baa29c229eb565b390d1edfd",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "0e60ddcdb695d8138d8e52f7f268f16e2545b343baa29c229eb565b390d1edfd",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B6-vs-B5-control.comparison.json",
      "sha256": "43e15928fb9f0ae8b7d19c5d9fa343da240ffb5a8b28f148bb8bd5c5f6a338c8",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "43e15928fb9f0ae8b7d19c5d9fa343da240ffb5a8b28f148bb8bd5c5f6a338c8",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A2-vs-A1-control.comparison.json",
      "sha256": "a5aa87f6cedc526ed68fe831120487c71a16c44188217d3647ddf873d66782f2",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "a5aa87f6cedc526ed68fe831120487c71a16c44188217d3647ddf873d66782f2",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A4-vs-A3-control.comparison.json",
      "sha256": "9ab653dd1fd31c89a0ff39fba8b25328a70ebc7aec6822bde8d48b308478b2b0",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "9ab653dd1fd31c89a0ff39fba8b25328a70ebc7aec6822bde8d48b308478b2b0",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A6-vs-A5-control.comparison.json",
      "sha256": "213b31c86b9992aa8a7d7a22b26d72ff184b8e4b6bf72f4910abf2e8df5480ef",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "213b31c86b9992aa8a7d7a22b26d72ff184b8e4b6bf72f4910abf2e8df5480ef",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B2-vs-B1-control.comparison.json",
      "sha256": "ec5defb095541fb107ddf61c7e2c72f785efc857dcc1b2b2b5b2c9b275515283",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "ec5defb095541fb107ddf61c7e2c72f785efc857dcc1b2b2b5b2c9b275515283",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B4-vs-B3-control.comparison.json",
      "sha256": "6995db5084421b5ca71f44d90c61a7385d5d05c30b63eaeb841c7c59fea99410",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "6995db5084421b5ca71f44d90c61a7385d5d05c30b63eaeb841c7c59fea99410",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B6-vs-B5-control.comparison.json",
      "sha256": "ae8c972581e0cf1ecab09d488090e9b95826c0380f02a6da3175931a9826799f",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "ae8c972581e0cf1ecab09d488090e9b95826c0380f02a6da3175931a9826799f",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A2-vs-A1-control.comparison.json",
      "sha256": "abc10a816bb77b7560cd389a9ba886c6afedff204a6acb8e226c7560b64a74bb",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "abc10a816bb77b7560cd389a9ba886c6afedff204a6acb8e226c7560b64a74bb",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A4-vs-A3-control.comparison.json",
      "sha256": "503578dc958dd3a25ff519aebbeb366a11e256bfa71193df452b0f8d21f29e1b",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "503578dc958dd3a25ff519aebbeb366a11e256bfa71193df452b0f8d21f29e1b",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A6-vs-A5-control.comparison.json",
      "sha256": "01e32b94abe8c702cc36f75bec0d5b76cb8dd9bfcfbd29c916b2ff6f2389222b",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "01e32b94abe8c702cc36f75bec0d5b76cb8dd9bfcfbd29c916b2ff6f2389222b",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B2-vs-B1-control.comparison.json",
      "sha256": "f76d4810873a660d18cd3e059ce2669be09c2365d9e4f3989fb0568c63d69d9a",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "f76d4810873a660d18cd3e059ce2669be09c2365d9e4f3989fb0568c63d69d9a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B4-vs-B3-control.comparison.json",
      "sha256": "5f1094bb091420a3c1f72b7f15d6514afccdfbac77faaac8e33111244fcf5a43",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "5f1094bb091420a3c1f72b7f15d6514afccdfbac77faaac8e33111244fcf5a43",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B6-vs-B5-control.comparison.json",
      "sha256": "02c0d052943fab5d8f9c5748b36139fafa06af692f5dbd2984e9345300760dad",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "02c0d052943fab5d8f9c5748b36139fafa06af692f5dbd2984e9345300760dad",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A2-vs-A1-control.comparison.json",
      "sha256": "240b961a2852b0ac633bd80906b117e50cbe0de5732a73b359247a3029e8137d",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "240b961a2852b0ac633bd80906b117e50cbe0de5732a73b359247a3029e8137d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A4-vs-A3-control.comparison.json",
      "sha256": "b990652646c0ce7b988d8fc054bf7f5e0fe7bf6187085c4411d9843edfe93419",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "b990652646c0ce7b988d8fc054bf7f5e0fe7bf6187085c4411d9843edfe93419",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A6-vs-A5-control.comparison.json",
      "sha256": "1a74eceeadb4d43e42581ef2fc3f23df1bd857c469e4b8cdb974b900954e3779",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "1a74eceeadb4d43e42581ef2fc3f23df1bd857c469e4b8cdb974b900954e3779",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B2-vs-B1-control.comparison.json",
      "sha256": "7a3dbe6b65686c36b15835beee7127425782744221508aa696227924d5668e65",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "7a3dbe6b65686c36b15835beee7127425782744221508aa696227924d5668e65",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B4-vs-B3-control.comparison.json",
      "sha256": "0b47d08c69a42ec4d530d948023b1e04fbdf582d81d5cabfbb0c2a92fcfa029c",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "0b47d08c69a42ec4d530d948023b1e04fbdf582d81d5cabfbb0c2a92fcfa029c",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B6-vs-B5-control.comparison.json",
      "sha256": "43f8402220eefffc26103068752a967f958d5d68420b5c463b7dfdc63b780b7a",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "43f8402220eefffc26103068752a967f958d5d68420b5c463b7dfdc63b780b7a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A2-vs-A1-control.comparison.json",
      "sha256": "3f38264505886022a8cf17a7c98d500c584d4d434206fd3d908f22eb0cfe567a",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "3f38264505886022a8cf17a7c98d500c584d4d434206fd3d908f22eb0cfe567a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A4-vs-A3-control.comparison.json",
      "sha256": "757fa101ab81fc2ae794fe85d950930cb9804fa3db0c4504ee365b966db2482d",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "757fa101ab81fc2ae794fe85d950930cb9804fa3db0c4504ee365b966db2482d",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A6-vs-A5-control.comparison.json",
      "sha256": "59b141c84d7472e94e8b5172aa78ac2d9fa5d682d74d3fbbd5a74b4f8777ce7a",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "59b141c84d7472e94e8b5172aa78ac2d9fa5d682d74d3fbbd5a74b4f8777ce7a",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B2-vs-B1-control.comparison.json",
      "sha256": "1516cb6d10bee1cac9c36c38aef2644467b513e52a3043e6a667b39984e3f9f6",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "1516cb6d10bee1cac9c36c38aef2644467b513e52a3043e6a667b39984e3f9f6",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B4-vs-B3-control.comparison.json",
      "sha256": "72a81e7f04b318810b9352e44967d1682757529f2d5572a60266b0bc46fa0c0b",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "72a81e7f04b318810b9352e44967d1682757529f2d5572a60266b0bc46fa0c0b",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B6-vs-B5-control.comparison.json",
      "sha256": "6750f9ef143cd51c447da44b103a55892bb50f88d1757f485c8811946fdc6841",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "6750f9ef143cd51c447da44b103a55892bb50f88d1757f485c8811946fdc6841",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A2-vs-A1-control.comparison.json",
      "sha256": "3a5be21c899f2e581ab42b2db03285c0bc05d7c02a8104fe5b6557ea92fa93f3",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "3a5be21c899f2e581ab42b2db03285c0bc05d7c02a8104fe5b6557ea92fa93f3",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A4-vs-A3-control.comparison.json",
      "sha256": "4f79836ce892f6c622cd4a1cdb71a286f39520a807993d0d2e67c1c70779c80f",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "4f79836ce892f6c622cd4a1cdb71a286f39520a807993d0d2e67c1c70779c80f",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A6-vs-A5-control.comparison.json",
      "sha256": "9a1c6dee3adfbb89cb9a30fe3e3fdb9cd4f58791e3e8f7af5af493da2c435767",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "9a1c6dee3adfbb89cb9a30fe3e3fdb9cd4f58791e3e8f7af5af493da2c435767",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B2-vs-B1-control.comparison.json",
      "sha256": "0c3ef826e4c36b94c46e931f252baab962dc3b483381d98c1191196868b3c2eb",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "0c3ef826e4c36b94c46e931f252baab962dc3b483381d98c1191196868b3c2eb",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B4-vs-B3-control.comparison.json",
      "sha256": "2756f7184cd6d6f51a2213d5a11499a469c98f7e279ed95e1122c01f1bbb07a5",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "2756f7184cd6d6f51a2213d5a11499a469c98f7e279ed95e1122c01f1bbb07a5",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B6-vs-B5-control.comparison.json",
      "sha256": "19ffd936ec4255120cd2c9ec65619cbf1b1977b7ba915e6263d033e836f949c5",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "generated-verified",
      "reviewer": "review_ledger",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "19ffd936ec4255120cd2c9ec65619cbf1b1977b7ba915e6263d033e836f949c5",
      "reviewEvidence": "review-dxui-tooling-20261024.md"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MathCorpus.h",
      "sha256": "3e0be1acecdf04598563465c72144478f6bca29f04146caa64cbcc2998b4df90",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Corpus cases define source, normalized expected math model and rejection reason independently.",
        "ownershipAndLifetime": "Expected node vectors are fixture-owned and compared after parsing, avoiding parser output as its own oracle.",
        "failureAndCancellation": "Cap and malformed cases retain exact source on rejection.",
        "testCoverage": "Support.Tests.MathCorpus and NativeMath test entrypoints. Coverage attribution only; this agent did not execute tests."
      },
      "addedAt": "execution-2026-10-24",
      "plannedSha256": "3e0be1acecdf04598563465c72144478f6bca29f04146caa64cbcc2998b4df90",
      "reviewEvidence": "review-dxui-rich-20261024.md"
    }
  ],
  "reviewStatusValues": [
    "pending",
    "reviewed",
    "generated-verified"
  ],
  "executionAt": {
    "RedTilde": "d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5",
    "DxUi": "94bf9801f51a544422ecb8aa6ae86d2e33aadcb2"
  }
}
```
