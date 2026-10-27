# RedTilde production review ledger

Read-only source review at source commit `c7456854c8f964bb9ea067295e6435f99d426515` (docs-only coordinator commit `d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5`). The ledger records 78 completed file reads with SHA-256 and per-file caller, ownership/lifetime, failure/cancellation, and test-coverage notes. No tests or builds were run.

```json
{
  "reviewedCommit": "d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5",
  "sourceCommit": "c7456854c8f964bb9ea067295e6435f99d426515",
  "scope": "W RedTilde working tree based on d53b4c6; 78 first-party inventory paths were read/reviewed, including 30 workflow/build/tooling/resource entries in this supplement. Hashes reflect current working-tree bytes. Repository tooling validators passed; no product build/native tests.",
  "files": [
    {
      "path": ".github/workflows/ci.yml",
      "sha256": "b9d237b55efd7e0f29a685e9fa7b8e82daabf0795eab90bae52fa79122f50a56",
      "reviewNotes": {
        "callers": "PR-to-main and manual workflow invoke native-scope, tooling and six profile jobs; Get-CiNativeScope provides descriptor/proof; upload steps retain input and profile logs.",
        "ownershipAndLifetime": "Read-only GitHub permissions, actions pinned by SHA, same-ref concurrency cancels superseded runs; artifacts retained 14 days.",
        "failureAndCancellation": "Input mismatch fails native validation; retained proof is emitted only after all six native executions succeed. Cancellation is not success evidence.",
        "testCoverage": "Scoped tests validate workflow digest/profile/coverage/proof contracts; hosted CI was not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "CI covers six exact native profiles and tooling once with no duplicate main-push run",
        "retained CI proof requires all six actual successful native executions"
      ]
    },
    {
      "path": "build.ps1",
      "sha256": "ab218b05f15a512ee599ab42f40c55f3e8fa3783a57a1d7b1382b2a731c48a28",
      "reviewNotes": {
        "callers": "Used by package.ps1, test.ps1, Test-Changes.ps1 and test-interactive.ps1; restores pinned DxUi unless SkipRestore.",
        "ownershipAndLifetime": "Profile outputs/logs are under repo .build; refuses if exact profile exe is running; source identity is checked before successful receipt.",
        "failureAndCancellation": "MSBuild/restore/source mutation/dependency identity failures stop receipt publication.",
        "testCoverage": "Mock public-runner fixtures cover argument expansion and affected baseline decisions; no actual build was run, leaving MSBuild execution unqualified."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "public runner expands empty and explicit tooling argument arrays exactly",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ]
    },
    {
      "path": "Build/Harness.psm1",
      "sha256": "e53552e8cf7002f52a09cdca5d593757365919c646c76c5031804cbb267f454e",
      "reviewNotes": {
        "callers": "build.ps1 records source identity; test.ps1 and paired performance runner validate artifacts and launch owned processes.",
        "ownershipAndLifetime": "Owns child Process, redirected pipes, timeout and descendant cleanup; artifact identity hashes exe/runtime closure.",
        "failureAndCancellation": "Timeout kills only owned process tree and preserves logs; stale source, receipt, binary or runtime closure refuses reuse.",
        "testCoverage": "Scoped fixture tests cover source/binary/runtime/profile/environment identities and missing runtime inputs; no product child launched."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "native identity changes with source/binary/runtime/profile/environment",
        "missing required runtime inputs and JSON-only profiles fail before identity creation",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ]
    },
    {
      "path": "Build/Performance.psm1",
      "sha256": "a5850d16712cc8d8c7ae494eb3366be791805fc933e3bd9dc15cc4aab49396cb",
      "reviewNotes": {
        "callers": "paired and observed performance entrypoints consume validation/math; evidence auditor has an independent comparator.",
        "ownershipAndLifetime": "Pure calculations over deserialized samples; compiler-profile reader binds project TU list to raw compiler tlog.",
        "failureAndCancellation": "Rejects malformed schemas, invalid/incomplete samples, inconsistent phase accounting and unsupported/mixed compiler options; comparisons remain review-required.",
        "testCoverage": "Tooling validators cover medians, schedules, exact sign-flip/Holm results, invalid metrics and compiler-profile boundaries; no native study ran."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1: math, schedule and compiler-profile fixtures",
        "Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest: independent comparator reference"
      ]
    },
    {
      "path": "Build/PerformanceObservation.psm1",
      "sha256": "30a4715b2a29da67c00af77a3ae43c9af5ddaea59b1a10b176b2acb2e8b828ea",
      "reviewNotes": {
        "callers": "performance-observed.ps1 launches activity observer and analyzes process/noise/coverage.",
        "ownershipAndLifetime": "Observer runs in an owned background job and writes JSONL under fresh repo .build output; process deltas keyed by PID/start time.",
        "failureAndCancellation": "Controller records observer status, stops its own job, and refuses incomplete time coverage; unavailable process metrics are never zeroed.",
        "testCoverage": "Observation tests cover CPU thresholds, unavailable processes, identity deltas, schema and interval coverage; orchestration not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1: observation and coverage contracts"
      ]
    },
    {
      "path": "Build/RedTilde.DxUi.props",
      "sha256": "5955c0931e04fbe59d09692244ff319e5f0cfeb586ce8dfed3a7156a4caf99cf",
      "reviewNotes": {
        "callers": "RedTilde vcxproj imports resolved dependency properties and canonical DxUi consumer props.",
        "ownershipAndLifetime": "Imports only generated resolved properties; pre-build target requires restored DxUi root.",
        "failureAndCancellation": "Missing restore root fails before compile with direct instruction.",
        "testCoverage": "Six project configs are validated; no MSBuild property evaluation/build ran."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Six build configurations and native inventory"
      ]
    },
    {
      "path": "Build/ScopedTesting.psm1",
      "sha256": "50d0f479281f4ead2a72beb6152f92ee8a34730dbe37973374e309d2ad0bc9de",
      "reviewNotes": {
        "callers": "Test-Changes uses scope/source/index/environment/artifact/receipt/CI/iteration functions; test.ps1 and Get-CiNativeScope reuse manifest/runtime/CI helpers.",
        "ownershipAndLifetime": "Git child args are structured; identities hash exact source/index/environment/profile/artifacts; receipts stage then rename; project include expansion cycle-guards.",
        "failureAndCancellation": "Invalid selectors/paths, missing base/history and stale evidence fail or conservatively widen; GitHub lookup uncertainty keeps work local; mutation blocks iteration advancement.",
        "testCoverage": "Tooling suite exercises path/scope mapping, Unicode/index changes, identity, receipts, CI delegation races, include closure and mutated runs. No live API calls."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "active native naming and inventory are contextual and complete",
        "committed/staged/working/deleted/rename sides/Unicode/untracked are independently discovered",
        "native identity changes with source/binary/runtime/profile/environment",
        "iteration state is source/index/profile/environment bound and rejects corruption",
        "live project/include consumers follow added dependencies and terminate include cycles",
        "only a clean committed candidate can delegate matching enabled CI, always as pending",
        "retained CI proof requires all six actual successful native executions",
        "failures and post-run mutations never advance the affected baseline or retain reusable success"
      ]
    },
    {
      "path": "Build/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "reviewNotes": {
        "callers": "test.ps1 and test-interactive.ps1 create, complete and publish owned test-run evidence.",
        "ownershipAndLifetime": "Pins local non-reparse path identities, unique run lock and owner PID/start time; writes beneath repo .build/evidence; preserves old compatibility receipts under payload.",
        "failureAndCancellation": "Refuses root/UNC/relative/reparse outputs, open/writable payloads, identity swaps, wrong owner, active artifact consumers and invalid names; closes locks in completion finally.",
        "testCoverage": "Run-evidence test validates concurrent isolation, byte/ACL/ADS preservation, path/link/owner refusal, long paths and publication. No product artifacts touched."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "38 evidence preservation checks in Tests/Tooling.Tests.RunEvidence.ps1"
      ]
    },
    {
      "path": "Build/Toolchain.psm1",
      "sha256": "4638ad95d1f48106ed09a7654196462b2aa97c144fb5b44c4946342cf41d55f4",
      "reviewNotes": {
        "callers": "build/test-interactive resolve VS 2026 MSBuild; format and identity discovery resolve clang-format.",
        "ownershipAndLifetime": "Resolves installation paths and checks executable path within selected output profile; no persistent child state.",
        "failureAndCancellation": "Missing compiler tools fail explicitly; in-use app blocks overwrite.",
        "testCoverage": "Formatter bytes affect scoped identity; current format check passed. No full MSBuild discovery matrix."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "tooling identity includes the selected formatter bytes",
        "format.ps1 -Check"
      ]
    },
    {
      "path": "capabilities.json",
      "sha256": "7c479509e93ce756e5666eef77aa191376e5d511ce2ccb939afc9ef282194a98",
      "reviewNotes": {
        "callers": "validate.ps1 and package metadata consumer.",
        "ownershipAndLifetime": "Static versioned implemented/pending capability declaration.",
        "failureAndCancellation": "Malformed JSON or dropped explicit pending gates fail validator.",
        "testCoverage": "Experimental/pending contract validation passed; not a runtime behavior test."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Exact dependency and capability status"
      ]
    },
    {
      "path": "Common/CrashHandler.cpp",
      "sha256": "8f585fb51670551318ebb196880bfb627d2c748720425458f037073671fbe98b",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\Main.cpp startup registers CrashHandler; shutdown unregisters it. Cross-repository CrashHandlingTests test seams are in D:\\RedSalamander\\Tests\\CrashHandlingTests\\CrashHandling.Tests.Runner.cpp.",
        "ownershipAndLifetime": "Registration is process-lifetime state; teardown path inspected for paired unregister.",
        "failureAndCancellation": "Failure paths report dump/handler errors; no recovery guarantee claimed.",
        "testCoverage": "No direct application-test assertion found in reviewed scope; crash-specific coverage remains gap."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "TestNoMarkerMakesNoOffer",
        "TestMarkerWithActivePluginOffersAndApplies",
        "TestBuildDumpPathShape",
        "TestMarkerRoundTrip"
      ]
    },
    {
      "path": "Common/CrashHandler.h",
      "sha256": "ef82c354e4432ca8bfaae01fe5f4a1917f95b03c2f9c9b2eb653ae42c17738ad",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\Main.cpp startup calls CrashHandler registration; cross-repository test seams are consumed by CrashHandling.Tests.Runner.cpp.",
        "ownershipAndLifetime": "Interface exposes registration lifetime; implementation is in CrashHandler.cpp.",
        "failureAndCancellation": "Caller must handle initialization result; no cancellation semantics.",
        "testCoverage": "No direct test coverage identified in assigned app tests."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "TestNoMarkerMakesNoOffer",
        "TestMarkerWithActivePluginOffersAndApplies",
        "TestBuildDumpPathShape",
        "TestMarkerRoundTrip"
      ]
    },
    {
      "path": "Common/DocumentInfo.cpp",
      "sha256": "77def0df6de5b3d22c31535666b2ca51dd538a3b7c886be311f76bc31e1b0a00",
      "reviewNotes": {
        "callers": "DocumentPositionAt is consumed by source inspector/status mapping. Repository source search found EquationAt only at its declaration/definition and former helper-only test; current app inspector uses syntax-backed extraction, so it was dead.",
        "ownershipAndLifetime": "DocumentInfo and positions are value-oriented; EquationAt implementation removed with no retained state or ownership.",
        "failureAndCancellation": "DocumentPositionAt remains bounded/noexcept; removed helper had no cancellation or external resources.",
        "testCoverage": "Parser boundary and syntax-backed inspector integration now covered by Math.NativeSubsetAndRejection and Editor.InDocumentMathLocalSourceAndBroaderMermaid. C++ tests not built/run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "simplification-candidate",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ]
    },
    {
      "path": "Common/DocumentInfo.h",
      "sha256": "40244f3098fb742e4db783ef40e48652aadbb6c8adec0c39cfebffc0a258ea2a",
      "reviewNotes": {
        "callers": "DocumentPositionAt declaration serves source inspector/status; EquationAt had no production caller and its declaration was removed.",
        "ownershipAndLifetime": "Value declarations; borrowed views only, no retained memory.",
        "failureAndCancellation": "DocumentPositionAt states noexcept/clamping contract; no async cancellation.",
        "testCoverage": "Core parser boundary and syntax-backed inspector cover current behavior; EquationAt was test-only and removed. Native tests not compiled/run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "simplification-candidate",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ]
    },
    {
      "path": "Common/Files.cpp",
      "sha256": "08e78096ea35aed558c42d4318af0f829f661ffef29ae037c0e8a0d40ad3a76e",
      "reviewNotes": {
        "callers": "Shared file helpers are used by FilesService and recovery/settings paths.",
        "ownershipAndLifetime": "Filesystem handles and buffers use scoped ownership; no persistent worker ownership here.",
        "failureAndCancellation": "Error-code/HRESULT paths propagate failure; callers own cancellation policy.",
        "testCoverage": "Files and recovery app tests cover relevant usage, not all OS error permutations."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Files.h",
      "sha256": "2d3f9bf94e0faaeb1c0b7ed47b369dfffa12e6d6ffb434a5b1517662c6c49603",
      "reviewNotes": {
        "callers": "Declares shared file helpers used by application services.",
        "ownershipAndLifetime": "Contracts are value/borrowed API; implementation owns transient OS resources.",
        "failureAndCancellation": "Failure contract is declared for callers; cancellation is caller-specific.",
        "testCoverage": "Coverage is in Files/Recovery tests; declaration has no independent test."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Recovery.cpp",
      "sha256": "3f7e3b5ecd9619cb30d99e39307e2063782da8328a435b9c1c2be7426c19cc63",
      "reviewNotes": {
        "callers": "Recovery service call sites originate in Application lifecycle/save paths.",
        "ownershipAndLifetime": "Lease and worker state bound service lifetime; shutdown requests cancellation and joins via completion protocol.",
        "failureAndCancellation": "Versioned/checksummed snapshots, quarantine and retry cover corruption; queue-full post failure remains investigation only because dispatch wake/completion checks exist.",
        "testCoverage": "Recovery tests cover lock replacement, retry, restart/restore/discard and dirty Save As; no confirmed shutdown hang."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Recovery.h",
      "sha256": "a2791dbb672905631bb2b1cc54b9913017fac1a11410ad8b385ce6ae0e7cedd9",
      "reviewNotes": {
        "callers": "Declares recovery worker and request/result contracts called by Application.",
        "ownershipAndLifetime": "Service owns worker/lease; request/result data crosses asynchronously.",
        "failureAndCancellation": "Cancellation/close state is explicit; post failure must be interpreted with app completion polling.",
        "testCoverage": "Application tests cover recovery lifecycle; stress around every post-failure interleaving remains gap."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/RemoteImage.cpp",
      "sha256": "a1c0ad49201146c61e4625dbc12a890b13dd17a915371cd08a1b9f9986ec914e",
      "reviewNotes": {
        "callers": "ImageLoader uses bounded remote-image request implementation.",
        "ownershipAndLifetime": "WinHTTP/session/request resources are scoped; no ambient auth/cookie/referrer and redirects disabled.",
        "failureAndCancellation": "Deadline, size bound, cancellation, HTTP errors handled; no unbounded response path observed.",
        "testCoverage": "Remote image tests use loopback and cover auth/redirect/limit/deadline/cancel/revoke/shutdown."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/RemoteImage.h",
      "sha256": "ea55a04cb361bab5dae7da0c190f700543de2e98ab2bb217f6a64cc58ec0d0e5",
      "reviewNotes": {
        "callers": "Declares remote image request contract consumed by ImageLoader.",
        "ownershipAndLifetime": "Caller supplies request context; result ownership is explicit.",
        "failureAndCancellation": "Cancellation and failure status are part of request/result path.",
        "testCoverage": "Loopback remote-image tests cover major contract; no external network test expected."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/RemoteImageImports.h",
      "sha256": "9544bf865901ef66981a755e66c7efe36e152aff2bfd26b3537ad1c2a464fcc4",
      "reviewNotes": {
        "callers": "Import declarations support remote-image implementation only.",
        "ownershipAndLifetime": "No owning state; imported function surface.",
        "failureAndCancellation": "Import failure is surfaced by implementation; no separate recovery behavior.",
        "testCoverage": "No direct test; remote image tests validate externally visible behavior."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Settings.cpp",
      "sha256": "1f4134148d810cee761d797f722e0826565eb3ef6b416139ab3c85a654ae61ae",
      "reviewNotes": {
        "callers": "SettingsWriter called by Application persistence/close flow.",
        "ownershipAndLifetime": "Worker retains latest pending settings snapshot; close signals async completion.",
        "failureAndCancellation": "Fieldwise parser fallback; failed post is not by itself a demonstrated hang.",
        "testCoverage": "Settings/app tests cover persistence and lifecycle; post-failure interleaving remains investigation."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Settings.h",
      "sha256": "73310426bff33169ca187be4e4c5cb24b1df9c2826b8f35e3ac6e2d179c70be3",
      "reviewNotes": {
        "callers": "Declares settings snapshot/writer API consumed by Application.",
        "ownershipAndLifetime": "Snapshot ownership copied to writer; worker lifetime managed by writer.",
        "failureAndCancellation": "Failure surfaced via status/completion; caller must observe close completion.",
        "testCoverage": "Settings and application tests cover relevant persistence; no stand-alone exhaustive writer stress noted."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "Common/Storage.cpp",
      "sha256": "db4d62d8503aad67cfb73712dec9284f7ba97b986d0bbc1dc445c98871cef341",
      "reviewNotes": {
        "callers": "Atomic byte writes support settings/recovery persistence.",
        "ownershipAndLifetime": "Transient file handles use scoped ownership; replacement is atomic at file operation boundary.",
        "failureAndCancellation": "Optimistic stamp checked before ReplaceFile/MoveFile; not CAS against simultaneous noncooperating writers.",
        "testCoverage": "Tests cover change-before-save, not true concurrent-writer race (specific coverage gap)."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [
        "STORAGE-CONCURRENT-WRITER-COVERAGE"
      ],
      "disposition": "coverage-gap",
      "testIds": []
    },
    {
      "path": "Common/Storage.h",
      "sha256": "bbc228765adf14031b90f54f6f2d66f34f915a2264e7c05094e5aa90e7297e04",
      "reviewNotes": {
        "callers": "Declares storage read/write and atomic replacement API used by settings/recovery.",
        "ownershipAndLifetime": "Callers own data buffers; storage owns transient handles.",
        "failureAndCancellation": "Result reports conflict/failure; simultaneous external writers are outside guaranteed CAS contract.",
        "testCoverage": "Storage behavior is exercised by settings/recovery tests; concurrent external race is not covered."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [
        "STORAGE-CONCURRENT-WRITER-COVERAGE"
      ],
      "disposition": "coverage-gap",
      "testIds": []
    },
    {
      "path": "Dependencies/DxUi.lock.json",
      "sha256": "74f687de9d4c5ab7a7601035da3ae61a13194e81f6fd4dc226362f98dc995d70",
      "reviewNotes": {
        "callers": "restore-dxui validates repository, exact commit, API revision and target; tests/package record pin.",
        "ownershipAndLifetime": "Pin resolves source/dependency output under repo .build.",
        "failureAndCancellation": "Malformed pin refuses; clone/checkout/consumer failures stop before resolved props.",
        "testCoverage": "JSON pin/capability validator passed; no network restore executed."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Exact dependency and capability status"
      ]
    },
    {
      "path": "Directory.Build.props",
      "sha256": "fc4a983cc286eb8b459766f22f2638aacd1486cce53d417aa893a89331c0cc07",
      "reviewNotes": {
        "callers": "MSBuild applies output paths, language standard, warning policy, Unicode, SDL/conformance and ASan config to native projects.",
        "ownershipAndLifetime": "All intermediate/output dirs are profile/project-local under .build; no shared external output declaration.",
        "failureAndCancellation": "Warnings-as-errors and compiler gates fail build; no fallback to different language mode.",
        "testCoverage": "Project config presence validated, but effective property evaluation/build not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Six build configurations and native inventory"
      ]
    },
    {
      "path": "format.ps1",
      "sha256": "3eda7cff89ade8359987235247d30b69a64c0b438991f98b10aa0178378b3724",
      "reviewNotes": {
        "callers": "CI tooling job and developer check/reformat entrypoint; formatter selected by Toolchain module.",
        "ownershipAndLifetime": "Enumerates C++ files under Common, RedTilde and Tests; Check is read-only, default writes formatter output.",
        "failureAndCancellation": "Formatter discovery/error fails closed; no external output.",
        "testCoverage": "Read-only formatting check passed after changed C++ was formatted."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "format.ps1 -Check"
      ]
    },
    {
      "path": "package.ps1",
      "sha256": "725505f3d323ea244eb73356277e0b566099196bb9b0324692502ad2159cf281",
      "reviewNotes": {
        "callers": "Portable packaging entrypoint; optionally builds Release then stages app/docs/assets/dependencies/licences.",
        "ownershipAndLifetime": "Stage, zip and receipt remain beneath .build/package; stage is timestamped; runtime redistributable explicitly not bundled.",
        "failureAndCancellation": "Refuses stale source/binary receipt; copy/archive failures stop; does not verify clean extraction itself.",
        "testCoverage": "No package execution/smoke test in current tooling suite or this run; archive extraction/licensing payload remains gap."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "performance-observed.ps1",
      "sha256": "2fc4a67b9a07bf78d47d11c16e1ba05940c717f0133152ed1e0857368fcd5dcd",
      "reviewNotes": {
        "callers": "Developer diagnostic wrapper starts observer around paired A/A and A/B studies.",
        "ownershipAndLifetime": "Requires fresh output below repo .build; retains logs, activity, observer status, summaries and native receipt noise.",
        "failureAndCancellation": "Observer/child failure or incomplete coverage stops acceptance but retains attempt evidence; only owned job is stopped.",
        "testCoverage": "Data validators cover observations and paired evidence shapes, not live orchestration or real study."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1",
        "Tests/Scope.Tests.Performance.Tooling.ps1"
      ]
    },
    {
      "path": "performance-paired.ps1",
      "sha256": "ec17f60be95d513c6db4314ebe78201fc680da795563c835eb1241517640777b",
      "reviewNotes": {
        "callers": "performance-observed wrapper or explicit user invokes freeze baseline/measure study.",
        "ownershipAndLifetime": "Validates Release receipt, exact fixture/compiler/environment and executable per slot; each child launched by Harness into fresh .build output.",
        "failureAndCancellation": "Refuses stale identity, inappropriate same-binary pairing, renderer/schema drift or failed child; failed attempts retained and never accepted.",
        "testCoverage": "Tooling/evidence fixtures cover inference and provenance rules; no baseline freeze or native paired measurements run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1",
        "Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest"
      ]
    },
    {
      "path": "RedTilde.sln",
      "sha256": "6e39173e22e39a02b705ddffebb3366c7c045131c57dfcfab89ebba0dd4d0da2",
      "reviewNotes": {
        "callers": "Optional VS solution entrypoint for the single RedTilde project and x64/ARM64 Debug/Release/ASan configs.",
        "ownershipAndLifetime": "Explicit solution-to-project mapping for six profile pairs.",
        "failureAndCancellation": "No custom hooks; build failures owned by MSBuild.",
        "testCoverage": "No solution parse/build was run; validator checks project config existence, not sln mapping."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/Application.CaptureScenarios.cpp",
      "sha256": "acff958a17b2288b203cb9a0fa97b2dfeb9149d50ee1a3c0d36255a326e7f929",
      "reviewNotes": {
        "callers": "Harness capture scenario calls lifecycle and UI transitions under deterministic hidden setup.",
        "ownershipAndLifetime": "Scenario-owned resources scoped to capture run; no desktop activation path intended.",
        "failureAndCancellation": "Scenario failure aborts capture and propagates diagnostics; cancellation bounded by harness.",
        "testCoverage": "Inspected capture flows; not run. Add/retain source identity and scenario provenance with captures."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/Application.Chrome.cpp",
      "sha256": "120f54db0480f599508e3919028b2c76645987c007c0d0694e3a0660caf34770",
      "reviewNotes": {
        "callers": "Application::BuildChrome creates BrandMark; DxUi visual preparation invokes its public hook.",
        "ownershipAndLifetime": "Retains WIC source over device replacement; D2D bitmap stamped with host resource generation and cleared on preparation cancellation; Paint draws only current-generation bitmap.",
        "failureAndCancellation": "Creation failure leaves no drawable bitmap; generation guard suppresses stale resource. No device COM query/create/retry in Paint.",
        "testCoverage": "New registered hidden case compares raster before/after device loss and repeated warm render. Not built/run; this is a rendering-contract correction, not evidence of 4 MiB cause/reduction."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster"
      ]
    },
    {
      "path": "RedTilde/Application.cpp",
      "sha256": "15e84820a8ab6a9cda325ecfe7877dc154312cf4b4b7941d444ba35ff8f46b1e",
      "reviewNotes": {
        "callers": "Orchestrates services, save, find/replace and 13 SetFocusControl call sites, each on a UI transition; no exact redundant app-command call proven.",
        "ownershipAndLifetime": "Owns service lifetimes; BeginClose retires recovery/stops services/detaches host; saves recheck baseline.",
        "failureAndCancellation": "Stale async results filtered; close completion polled after messages; queue-full post failure not established as hang.",
        "testCoverage": "Core/files/find tests cover app paths; performance tests measure process/UI metrics but no allocation-stack causality."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/Application.h",
      "sha256": "efc666d69f8a0b0b0b68ae39b40269fd33e3ecb40eb11ce5597025dd8cbb6bff",
      "reviewNotes": {
        "callers": "Declares application state consumed by Main and scenario/tests.",
        "ownershipAndLifetime": "Owns top-level controls/services; explicit teardown contract in Application implementation.",
        "failureAndCancellation": "Lifecycle state gates late results and close; no standalone cancellation behavior.",
        "testCoverage": "Core and design tests exercise public application state; no direct header-only tests."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/Application.Navigation.cpp",
      "sha256": "ac2765c5d422960afae07d73e1f4c816fa1253fd1ebc33e884333fac679c6886",
      "reviewNotes": {
        "callers": "Navigation transitions call focus at two sites after route changes; no exact duplicate invocation established.",
        "ownershipAndLifetime": "Navigation model and async result state guarded by generation/revision.",
        "failureAndCancellation": "Stale navigation results discarded; cancellation on new root/close.",
        "testCoverage": "Files and interactive navigation tests cover behavior; no perf call count assertion."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/AsyncFiles.h",
      "sha256": "8eb3cda6b6e6362d7ebeda811e0a628c461b739f83b56e3110215a35a3bf85e1",
      "reviewNotes": {
        "callers": "FilesService invoked from Application root/navigation commands.",
        "ownershipAndLifetime": "Single worker coalesces latest root; bounded result state and stop token limit lifetime.",
        "failureAndCancellation": "Generation/revision reject stale results; close stops without blocking UI join.",
        "testCoverage": "Files tests cover filtering/sort/limits/cancel/reparse/latest-root/coalescing/stale/close."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/EditDiagnostics.h",
      "sha256": "b789d0a0dbd16e5936582719915cf873f63e672f0bda009d4df02ca7b5cfab48",
      "reviewNotes": {
        "callers": "Editor diagnostics consumed by document inspector paths.",
        "ownershipAndLifetime": "Diagnostic records are value-owned and bounded by edit state.",
        "failureAndCancellation": "Invalid/stale ranges are filtered at consumers; no async worker ownership.",
        "testCoverage": "Core tests cover extraction/inspection paths; no dedicated lifetime stress identified."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/ImageLoader.h",
      "sha256": "a3c800d467186f8599b7357ac49ce59d5dba206d09d90b15b3b2d0d15873ec78",
      "reviewNotes": {
        "callers": "Application image flow invokes ImageLoader, which calls Common remote image transport.",
        "ownershipAndLifetime": "Worker uses MTA; decoded image/result state scoped to loader and host lifetime.",
        "failureAndCancellation": "Cancellation plus generation checks discard stale work; shutdown tested.",
        "testCoverage": "Remote image tests cover loopback transport, revoke and shutdown; no allocation attribution."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/Main.cpp",
      "sha256": "7918862fd087de59c563b21c9be196d6c1f4a620c2f66468a1914d72d0f070af",
      "reviewNotes": {
        "callers": "Entrypoint constructs/initializes Application and routes messages.",
        "ownershipAndLifetime": "Main owns application/UI lifetime through message loop.",
        "failureAndCancellation": "Initialization/dispatch errors unwind; close completion checked by loop.",
        "testCoverage": "Harness launches app scenarios; no standalone entrypoint test."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/NavigationModel.h",
      "sha256": "72a77ffc90965572ff011fd187e6ff831e87c73070b20a9b22d83adfed14f613",
      "reviewNotes": {
        "callers": "Model is mutated by Application navigation and FilesService results.",
        "ownershipAndLifetime": "Snapshot values copied across worker/UI boundary; revisions guard identity.",
        "failureAndCancellation": "Cancel/revision invalidates obsolete navigation requests.",
        "testCoverage": "Navigation/file tests cover stale-root and latest-request behavior."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/RedTilde.vcxproj",
      "sha256": "99240363fa0b926aabaf6b13035a9cced00d55dbb2bafed7ec8c3d5446ea7ded",
      "reviewNotes": {
        "callers": "Project compiles app, test integration and performance configurations.",
        "ownershipAndLifetime": "Build owns generated outputs; project does not own runtime data.",
        "failureAndCancellation": "Build failures are surfaced by MSBuild; profile configurations include x64/ARM64 Debug/Release/ASan.",
        "testCoverage": "No execution coverage itself; verify configurations via parent build workflow."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "RedTilde/UiControls.h",
      "sha256": "de0e8d6bdf69a6f0bb88c2b2d53beffd87a43b2ed03aba45d7372ece05bff780",
      "reviewNotes": {
        "callers": "Defines inspector/find controls used by Application.",
        "ownershipAndLifetime": "UI control tree owns controls; callbacks route to Application.",
        "failureAndCancellation": "Find snapshot/query cache is bounded; highlight cap is 4096 and active range remains visible.",
        "testCoverage": "Core and interactive tests cover selection/find/replace; focus-taking suite not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "res/RedTilde.manifest",
      "sha256": "a2ab46534b26273903bc0dc9df374d9cd352e9e54b70534f901c1ee08904e484",
      "reviewNotes": {
        "callers": "Embedded app activation context defines asInvoker, PerMonitorV2, longPathAware and supported Windows identity.",
        "ownershipAndLifetime": "Static executable resource; no runtime-owned object.",
        "failureAndCancellation": "Malformed resource fails resource/build activation; no custom retry.",
        "testCoverage": "No dedicated manifest runtime/parser assertion identified; build/DPI native tests pending."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "res/RedTilde.rc",
      "sha256": "71df47846061f1dd880d6a0bc290c668ab943e314d2069e92f45076832603d86",
      "reviewNotes": {
        "callers": "Compiled app string/version/icon resource consumed by Application/Chrome.",
        "ownershipAndLifetime": "Static strings, indexed format substitutions and icon binary resource.",
        "failureAndCancellation": "Unknown/missing IDs or RC syntax errors fail resource compilation; lookup fallback elsewhere.",
        "testCoverage": "UI app cases use some strings but no exhaustive RC-ID/placeholder parity test; no native build."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Editor.DesignRefreshLiveSettingsAndEscape",
        "Editor.DesignRefreshTransientSurfacesAndClose"
      ]
    },
    {
      "path": "res/Resource.h",
      "sha256": "d3723f905d3f0d8c70def4725dec5f0f858b50871767cf90e0502268ade3b475",
      "reviewNotes": {
        "callers": "Included by RedTilde.rc and app C++ as compile-time resource IDs.",
        "ownershipAndLifetime": "Integer constants; no runtime object lifetime.",
        "failureAndCancellation": "ID/string mismatch fails compile or displays wrong resource; no local recovery.",
        "testCoverage": "No exhaustive header/RC parity test identified; no resource compilation run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
    },
    {
      "path": "restore-dxui.ps1",
      "sha256": "20122314d7c72b24ab8ddccd72796e8e41ef30f0e4d51725959334a0b4751b02",
      "reviewNotes": {
        "callers": "build.ps1 invokes restore once per platform unless SkipRestore; validates exact lock and canonical consumer.",
        "ownershipAndLifetime": "Pinned source/resolved props/platform dependency products remain under repo .build/dependencies/DxUi.",
        "failureAndCancellation": "Clone/checkout/consumer/vcpkg failure stops before properties are treated as restored; partial source retained for diagnosis.",
        "testCoverage": "Lock validation and PowerShell parser passed; network restore/build not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Exact dependency and capability status",
        "PowerShell syntax"
      ]
    },
    {
      "path": "Test-Changes.ps1",
      "sha256": "ece5628054570552d3ef363ce27c1a172ea383726e197b5eeb7adea66cb7c1ce",
      "reviewNotes": {
        "callers": "Developer Affected/Full/PrePush public scoped runner; changelog fixture checks PrePush ordering.",
        "ownershipAndLifetime": "Reads exact source/index/environment/artifact identities; successful iteration state under .build/reports; child suites delegated.",
        "failureAndCancellation": "Stale SkipBuild refuses; failures/mutations prevent receipts/baseline advancement; PrePush identifies pending CI separately.",
        "testCoverage": "Scoped fixtures cover baseline/reuse/narrowing/partial selection/failure/mutation/argument binding; changelog suite verifies gate before discovery."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "public default records a successful affected baseline and then runs nothing unchanged",
        "a later edit selects only the newly affected scope, not earlier branch changes",
        "explicit identical success is reused and partial selection never advances the default baseline",
        "failures and post-run mutations never advance the affected baseline or retain reusable success",
        "public runner expands empty and explicit tooling argument arrays exactly",
        "public final runner rejects pending fragments before build or test discovery"
      ]
    },
    {
      "path": "test-interactive.ps1",
      "sha256": "6ffb4c8a623796e814072463186b719ab86910052dd6480edbe14443c7d54c60",
      "reviewNotes": {
        "callers": "Explicit time-authorized interactive qualification; builds and validates exact pinned lease then runs required keyboard/focus child.",
        "ownershipAndLifetime": "Lease owns warning, session desktop lock, child execution/restoration; evidence and receipt are run-scoped; app/lease identities rechecked.",
        "failureAndCancellation": "Requires available desktop and no implicit auto-start; failures produce failed evidence; only opt-in switch starts countdown.",
        "testCoverage": "Required interactive IDs are checked in code but runner was not invoked; no focus-taking suite ran."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Window.AltF4AsyncClose",
        "Equation.F2ApplyUndoAndCancel",
        "test.ps1 unleased Lease.Setup refusal"
      ]
    },
    {
      "path": "test.ps1",
      "sha256": "f11dc5abdea3bc90965443e1cd34927373dfe79f9fe763ca70e5c77d79972686",
      "reviewNotes": {
        "callers": "Runtime runner called from CI/Test-Changes; supports native scopes, capture, crash, recovery restart, settings exit and perf.",
        "ownershipAndLifetime": "Receipt-verified executable, isolated evidence run, unique child data dirs and final receipt even on failure.",
        "failureAndCancellation": "Rejects stale artifact, invalid suite/profile/catalog, child exit/data errors; deliberate recovery crash is separated from restart process; failed evidence retained.",
        "testCoverage": "Runner compares list-tests catalog and behavioral outputs; named process probes include PreferencesProcessExit and RecoveryCrashRestart. No native suite/build run here."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "PreferencesProcessExit",
        "RecoveryCrashRestart",
        "Crash",
        "PerformanceBaseline",
        "Capture",
        "AddressSanitizer",
        "native catalog/partition validation"
      ]
    },
    {
      "path": "Tests/Application.Tests.Core.cpp",
      "sha256": "bd9c68c5165837348ff81b3228998035238ab1c564be50e123b07971eedebf62",
      "reviewNotes": {
        "callers": "Registers Math.NativeSubsetAndRejection and Editor.InDocumentMathLocalSourceAndBroaderMermaid only in native self-test runner.",
        "ownershipAndLifetime": "Local sources/apps and immutable semantic publications; no new persistent state.",
        "failureAndCancellation": "Checks over-limit parser rejection, unsupported exact-source fallback, neighboring links/syntax and unchanged authoritative source.",
        "testCoverage": "4096 boundary now uses one coalesced text node; typed classes/styles/operator limits/matrix capacity covered; whitespace slash math and true parent-link-enclosed rejected math assert exact source and Bold semantic display run. Not built/run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Math.NativeSubsetAndRejection",
        "Editor.InDocumentMathLocalSourceAndBroaderMermaid"
      ]
    },
    {
      "path": "Tests/Application.Tests.DesignInteractive.cpp",
      "sha256": "d66ca92c1469035a6365bb2f08c617d7e1ece01da88ef56354ca2e10f617e269",
      "reviewNotes": {
        "callers": "Interactive refresh tests invoke application focus/UI pathways.",
        "ownershipAndLifetime": "Owned harness window and lease bound test lifetime.",
        "failureAndCancellation": "Test setup guards activation; not run absent focus agreement.",
        "testCoverage": "Covers design interactions; focused/manual behavior remains unexecuted here."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.DesignInteractive.cpp"
      ]
    },
    {
      "path": "Tests/Application.Tests.DesignInteractive.h",
      "sha256": "41c4c541952ff245b4e4cc5e6145a5e5df1bc7cf2caba3baa6e134b456ce86e6",
      "reviewNotes": {
        "callers": "Declares interactive test entry points wired into test registry.",
        "ownershipAndLifetime": "Test fixture state is owned by suite.",
        "failureAndCancellation": "Activation contract delegated to implementation/harness.",
        "testCoverage": "No separate coverage beyond corresponding cpp."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.DesignInteractive.h"
      ]
    },
    {
      "path": "Tests/Application.Tests.DesignRefresh.cpp",
      "sha256": "ea4fb87c93ff5c2662653711505b7441b4d85f004556f16dc6c46c5b0ec75110",
      "reviewNotes": {
        "callers": "Registers Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster and Editor.DesignRefreshEquationContextAndDraftGuards.",
        "ownershipAndLifetime": "Hidden app, WIC decoder, pixel arrays and capture paths local to test-owned fixture root.",
        "failureAndCancellation": "Raster must remain exact after resource reset/rebuild and warm re-render; draft cancellation preserves history; apply is undoable.",
        "testCoverage": "Adds warm repeated capture equality plus Source/Split/Compose and F2 cancel/apply/undo/redo parent-title checks. Case not built/run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster",
        "Editor.DesignRefreshEquationContextAndDraftGuards"
      ]
    },
    {
      "path": "Tests/Application.Tests.DesignRefresh.h",
      "sha256": "bfa91d004cc818f23aec07609b23a985245c16fd4afebcaa4c7dd4a4bb2251c7",
      "reviewNotes": {
        "callers": "Declares design refresh tests referenced by test registration.",
        "ownershipAndLifetime": "No runtime resource ownership.",
        "failureAndCancellation": "No independent cancellation path.",
        "testCoverage": "Coverage in corresponding cpp; header itself is declaration only."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.DesignRefresh.h"
      ]
    },
    {
      "path": "Tests/Application.Tests.Files.cpp",
      "sha256": "ce324aa55e067c0006d6e027ec9663458c191fd69ad40d9a9ce2881c1a73c150",
      "reviewNotes": {
        "callers": "Tests FilesService/app navigation/root and result delivery.",
        "ownershipAndLifetime": "Fixture owns temp sandbox and service lifetime.",
        "failureAndCancellation": "Cases cover cancellation, stale generation, coalescing and close.",
        "testCoverage": "Broad FilesService lifecycle and filtering coverage; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.Files.cpp"
      ]
    },
    {
      "path": "Tests/Application.Tests.Files.h",
      "sha256": "3df3de8a09e0bfe3e747050306d2066837a400f8d0bfe29a55ba192402953382",
      "reviewNotes": {
        "callers": "Declares Files tests registered by app test harness.",
        "ownershipAndLifetime": "No owned runtime state.",
        "failureAndCancellation": "Failure/cancellation exercised by implementation tests.",
        "testCoverage": "Coverage in corresponding cpp."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.Files.h"
      ]
    },
    {
      "path": "Tests/Application.Tests.Performance.cpp",
      "sha256": "ac7f6912cce121af62752a2e606ece5c40bd37a9bebcd81eb299a9782fcbb164",
      "reviewNotes": {
        "callers": "Performance scenario collects private bytes/working set/resources/frame timing/CPU by scenario.",
        "ownershipAndLifetime": "Metrics and scenario artifacts owned by harness run.",
        "failureAndCancellation": "Scenario has bounded cycles and reports missing/invalid metrics; no heap stack trace in this implementation.",
        "testCoverage": "Perf instrumentation exists; seven historical regressions remain open and current diagnostic lacks allocation stack."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.Performance.cpp"
      ]
    },
    {
      "path": "Tests/Application.Tests.RemoteImages.cpp",
      "sha256": "0f524f78a4ae959cf3681475c39530baa7ffc2f81d96fb0295fac55e9c35c012",
      "reviewNotes": {
        "callers": "Tests Application image loader and remote image requests.",
        "ownershipAndLifetime": "Loopback server and fixture resources are test-owned.",
        "failureAndCancellation": "Covers auth/redirect/size/deadline/cancel/revoke/shutdown.",
        "testCoverage": "Focused transport and app lifecycle tests; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.RemoteImages.cpp"
      ]
    },
    {
      "path": "Tests/Application.Tests.RemoteImages.h",
      "sha256": "76516bd27adcd39961edec246d45d4ef0f66000912fcc75920a84159fbb84f27",
      "reviewNotes": {
        "callers": "Declares remote image cases wired to test harness.",
        "ownershipAndLifetime": "No owned resource itself.",
        "failureAndCancellation": "No independent failure handling.",
        "testCoverage": "Coverage in cpp."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.RemoteImages.h"
      ]
    },
    {
      "path": "Tests/native-case-catalog.json",
      "sha256": "c166b6eb606de95c54ad508c0c25f4817e11b5464fe54b6e934fc18eb6168e7e",
      "reviewNotes": {
        "callers": "test.ps1 compares catalog against app list-tests for every native scope.",
        "ownershipAndLifetime": "Static case/scope record only.",
        "failureAndCancellation": "Catalog/list mismatch fails before behavioral suite is counted.",
        "testCoverage": "BrandMark device-loss raster case added to Editor scope; validator passes but native listing is not executed until build."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Editor.DesignRefreshBrandMarkDeviceLossRetainsRaster"
      ]
    },
    {
      "path": "Tests/NativeInventory.json",
      "sha256": "8f1060c529717296214efc7879299065c56909c0cd76fc87802841da223c1c9b",
      "reviewNotes": {
        "callers": "Inventory registers source/test ownership for native suite.",
        "ownershipAndLifetime": "Declarative inventory.",
        "failureAndCancellation": "Stale entries can hide tests from tooling.",
        "testCoverage": "Inventory validation belongs to test tooling; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/NativeInventory.json"
      ]
    },
    {
      "path": "Tests/Scope.Tests.Performance.Diagnostics.ps1",
      "sha256": "3386f7a85904c7a00e50b5f784598987698c87546955ff7411cfaf8d5877079c",
      "reviewNotes": {
        "callers": "Performance scope diagnostics consume run evidence and source metadata.",
        "ownershipAndLifetime": "PowerShell temp/artifacts constrained by test sandbox policy.",
        "failureAndCancellation": "Rejects malformed/untrusted diagnostic records; actual script not executed.",
        "testCoverage": "Tooling tests cover scope contract; diagnostic stack capture absent."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Diagnostics.ps1"
      ]
    },
    {
      "path": "Tests/Scope.Tests.Performance.Evidence.ps1",
      "sha256": "d4b38a749c6824ff1ce344e472f077c7b4f3094676fb8c1313f46287dad950c2",
      "reviewNotes": {
        "callers": "Validates evidence bundle identity and run comparisons.",
        "ownershipAndLifetime": "Evidence paths/records are read-only inputs to validator.",
        "failureAndCancellation": "Rejects incomplete/mismatched cohorts.",
        "testCoverage": "ABBA/BAAB and identity validation; no current full same-SDK study."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Evidence.ps1"
      ]
    },
    {
      "path": "Tests/Scope.Tests.Performance.Observation.ps1",
      "sha256": "e65621638f44e3e8aa2e9f346afe9f893939fa2881b5f9d530121fc5e38a6e2b",
      "reviewNotes": {
        "callers": "Observation validation checks metric records and run consistency.",
        "ownershipAndLifetime": "No application-owned state.",
        "failureAndCancellation": "Outlier/missing data handled as invalid/inconclusive rather than success.",
        "testCoverage": "Perf evidence tests cover parsing/consistency; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Observation.ps1"
      ]
    },
    {
      "path": "Tests/Scope.Tests.Performance.Tooling.ps1",
      "sha256": "da854c344a56ffc05fa62db2925309508c04e59d9e44c67482cdd1ba4ce1756a",
      "reviewNotes": {
        "callers": "Validates performance tooling wiring and policy surfaces.",
        "ownershipAndLifetime": "Tooling controls own run directories.",
        "failureAndCancellation": "Failure paths protect against invalid paths/configuration; global profile behavior out of scope.",
        "testCoverage": "Tooling tests exercise inventory/argument contracts; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Scope.Tests.Performance.Tooling.ps1"
      ]
    },
    {
      "path": "Tests/Support/Support.Tests.FilesFixtures.h",
      "sha256": "f2306c9fee4e7bd54a8f66c06e471723e5a479b6bb8bf178207bec22abb425bd",
      "reviewNotes": {
        "callers": "File test fixture helpers are consumed by Files tests.",
        "ownershipAndLifetime": "Fixtures own sandboxed paths/resources and scoped cleanup.",
        "failureAndCancellation": "Fixture setup failures fail closed; cleanup respects sandbox ownership.",
        "testCoverage": "Coverage inherited by Files tests; individual helper contracts not separately enumerated."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Support/Support.Tests.FilesFixtures.h"
      ]
    },
    {
      "path": "Tests/Support/Support.Tests.TextServices.h",
      "sha256": "81a6866c450d1a0c5035f68c9f232c3dbb538cfa1ea8f00812fb166efdf37ccb",
      "reviewNotes": {
        "callers": "Text service fixtures support editor/document tests.",
        "ownershipAndLifetime": "Text buffers/COM service objects are scoped to fixture lifetime.",
        "failureAndCancellation": "Failure paths release fixture-owned services; async text callbacks require host teardown.",
        "testCoverage": "Core/remote text coverage uses helpers; no dedicated helper test."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Support/Support.Tests.TextServices.h"
      ]
    },
    {
      "path": "Tests/test-scopes.json",
      "sha256": "5e02ac35d2f4671331471e364497b1e0f6b9700d5ab205eb51e9bf01eaa70b29",
      "reviewNotes": {
        "callers": "Maps scope names to test cases used by affected iteration.",
        "ownershipAndLifetime": "Declarative metadata.",
        "failureAndCancellation": "Incorrect mapping can omit execution; no runtime cancellation.",
        "testCoverage": "Validate mapping with tooling test; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/test-scopes.json"
      ]
    },
    {
      "path": "Tests/Tooling.Tests.RunEvidence.ps1",
      "sha256": "04e6aa403a22a8921b7c5e8548fa92d12533508d8a5098cfdecedfda9268a2ef",
      "reviewNotes": {
        "callers": "Tests run evidence generation/validation callers.",
        "ownershipAndLifetime": "Temporary evidence governed by sandbox policy.",
        "failureAndCancellation": "Covers malformed/missing identity and failure handling.",
        "testCoverage": "Run-evidence tooling tests reviewed, not executed."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Tooling.Tests.RunEvidence.ps1"
      ]
    },
    {
      "path": "Tests/Tooling.Tests.ScopedTesting.ps1",
      "sha256": "507db3426d59f1a7095454a62676c45d7d031a2458e6e950d1e1ea7e2a32e80d",
      "reviewNotes": {
        "callers": "Tests scoped testing and affected-set resolution.",
        "ownershipAndLifetime": "Harness-owned state and temp remain within test sandbox.",
        "failureAndCancellation": "Fail-closed on invalid/empty scope evidence.",
        "testCoverage": "Scope-tooling coverage reviewed, not executed."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Tooling.Tests.ScopedTesting.ps1"
      ]
    },
    {
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "reviewNotes": {
        "callers": "Developer and PrePush/final handoff wrapper; imports shared module.",
        "ownershipAndLifetime": "Exclusive repo-local lock; temp staged beside changelog; publishes changelog before deleting re-read fragments.",
        "failureAndCancellation": "Check/WhatIf read-only; malformed/reparse/race/lock failures refuse; retry is deduped.",
        "testCoverage": "Synthetic workflow cases passed for preview, single fragment, ordering/history, BOM/CRLF, retry, invalid inputs, lock and EOF."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "single-fragment final gate is read-only and requires folding",
        "preview writes nothing",
        "newest first, stable topic order, preserve released history and README",
        "preserve BOM, CRLF and Unicode bytes",
        "retry after publication cleans fragments without duplicate entries",
        "overlapping fold fails without touching inputs",
        "Unreleased heading at EOF is supported"
      ]
    },
    {
      "path": "Tools/Get-CiNativeScope.ps1",
      "sha256": "52d66cfd12a2f68f745f69f57c3f3c6f4ed235b2b82656cc8a639bb6ad7a6db1",
      "reviewNotes": {
        "callers": "CI scope job creates/verifies native descriptor; optional gh API/artifact reads prior PR proof.",
        "ownershipAndLifetime": "Descriptor and downloaded evidence under repo .build/ci-scope; each external download uses run-specific folder.",
        "failureAndCancellation": "Uncertain, stale, missing or failed latest native evidence selects fresh matrix; verification compares exact input key and build number.",
        "testCoverage": "Fixture tests cover workflow digest, profile mapping, delegation refusal/proof and races; no live GitHub run/API called."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "CI covers six exact native profiles and tooling once with no duplicate main-push run",
        "only a clean committed candidate can delegate matching enabled CI, always as pending",
        "disabled/unavailable CI, stale workflow and mismatched profiles keep obligations local",
        "working and clean-commit races during coverage lookup refuse delegation",
        "direct default-branch pushes have no forthcoming PR coverage",
        "retained CI proof requires all six actual successful native executions"
      ]
    },
    {
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "reviewNotes": {
        "callers": "Fold-Changelog and Test-Changelog import module; Test-ChangelogWorkflow drives synthetic fixture repos.",
        "ownershipAndLifetime": "Strict UTF-8 reads; preserves original newline/BOM; exclusive fold lease and temp staging.",
        "failureAndCancellation": "Rejects reparse paths/malformed fragment/layout/content mutation; publishes before deleting; retry deduplicates.",
        "testCoverage": "22 behavioral changelog fixture checks passed under validate.ps1."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "22 behavioral checks in Tools/Test-ChangelogWorkflow.ps1"
      ]
    },
    {
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "reviewNotes": {
        "callers": "validate.ps1/developers validate edits; strict folded mode used by final handoff.",
        "ownershipAndLifetime": "Read-only wrapper; default root is command repo; relative roots resolve in PowerShell provider.",
        "failureAndCancellation": "Invalid input/pending strict-gate fragments throw before mutation.",
        "testCoverage": "Synthetic workflow validates valid/invalid fragments, read-only final check and relative-root behavior; standalone command passed."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "editing permits one valid dated Unicode change",
        "invalid fragment rejection cases without mutation",
        "single-fragment final gate is read-only and requires folding",
        "relative roots follow PowerShell location without touching the process directory"
      ]
    },
    {
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "reviewNotes": {
        "callers": "validate.ps1 runs it with existing repo .build scratch; standalone cross-project regression suite copies wrapper/module into owned fixtures.",
        "ownershipAndLifetime": "Creates GUID direct children and deletes only verified exact children; timeout kills only child whose PID/start identity it recorded.",
        "failureAndCancellation": "Cleanup path/name validated; child output drains with bound; all failures reported at end.",
        "testCoverage": "Latest run reports 22 passed, zero failed; fixtures include format, path, encoding, retry, lock and gate ordering."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
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
      ]
    },
    {
      "path": "validate.ps1",
      "sha256": "c3cfa6959f7389f89ce452f4d62c9dabfb022dc22499c8d31febb9b2219ebb58",
      "reviewNotes": {
        "callers": "CI tooling and test.ps1 Tooling suite invoke it; developers run directly.",
        "ownershipAndLifetime": "Orchestrates metadata/script validators and fixture suites; fixtures are beneath repo .build and owned by each suite.",
        "failureAndCancellation": "Runs checks independently, collects failures, and terminates if any fail; does not build or run product native suites.",
        "testCoverage": "Latest direct run passed Changes workflow (22), specs/links, skills, pin/capability, six configurations/catalog, run evidence (38), scoped tooling (25), PowerShell syntax and performance validators."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
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
      ]
    }
  ],
  "focusReview": {
    "currentDxUiSourceCommit": "b06f534b36ffc2c0c9671a142d1e51545480be22",
    "reviewedFiles": [
      "src/Controls/DxUi.WindowHost.cpp",
      "src/Controls/DxUi.Controls.cpp",
      "src/Controls/DxUi.TextStoreACP.cpp",
      "src/Controls/TextStoreTarget.h",
      "include/DxUi/DxUi.h",
      "Tests/Controls/DxUi.Tests.WindowHost.cpp",
      "Tests/Controls/Controls.Tests.HostControlRegressions.cpp",
      "Tests/Controls/Controls.Tests.TextStoreComposition.h"
    ],
    "finding": "No provably redundant Application SetFocusControl invocation identified. Calls are transition-driven; same-target call refreshes text/UIA state. Recursive membership/interactivity checks and revalidations span callback/mutation boundaries and preserve reentrancy/tree correctness. Instrument request count, tree walks, tree size and duration before proposing measured simplification.",
    "equationAt": "Declaration/definition plus one in-scope Core test caller; current inspector uses syntax-backed extraction; no Common/RedTilde production caller. Treat as test-only/dead-helper simplification candidate, not runtime defect.",
    "tests": "Focus/reentrancy/tree membership/UIA/TextStore tests inspected, not run. No persistent cache/state skip recommended without mutation-generation correctness and measurement."
  },
  "perfReview": {
    "historical": "Seven open performance regressions; historical private memory +10.83%/+11.51% in cf1393a2 SDK cohort. No allocation stack trace, same-SDK full baseline-vs-candidate study, or causal focus evidence.",
    "artifacts": [
      "D:\\RedTilde\\Measurements\\DesignRefresh_2026-10-24\\README.md",
      "D:\\RedTilde\\.build\\design-refresh\\performance-investigation-2f-v3\\bounded-diagnostic-predeclaration.json",
      "D:\\RedTilde\\.build\\design-refresh\\performance-investigation-2f-v3\\bounded-diagnostic-audit.json",
      "D:\\RedTilde\\.build\\design-refresh\\performance-investigation-2f-v3\\post-repair-predeclaration.json",
      "D:\\RedTilde\\.build\\design-refresh\\performance-investigation-e62\\pre-repair-summary.json"
    ],
    "allocationTrace": "No ETL/heap/snapshot/memory/dump file found in the focused investigation directories. bounded-diagnostic.log was not opened. WPR heap tracing denied 0x80070005 per parent; no active trace/configured image reported.",
    "conclusion": "4 MiB retention cause remains unknown. No EquationAt or focus-update allocation attribution. Keep regressions open pending same-SDK paired study and allocation stack evidence."
  },
  "limitations": "Repository tooling validators passed; no product build/native app tests were run, so C++ test compilation and raster assertions remain pending the final SDK pin. Focus suite was not run. Hashes identify reviewed source, not runtime proof. F08 queue-full/post failure remains investigation only: no confirmed hang and no injected PostMessage failure test. Seven private-memory regressions remain open; no allocation cause or BrandMark memory reduction is established.",
  "filesReviewed": 78,
  "diagnosticFollowup": {
    "result": "No allocation stack obtained; do not attribute retained 4 MiB.",
    "probe": "Owned-child DbgEng helper compiled under existing .build diagnostic directory. CreateProcess2Wide/CreateProcessWide/CreateProcessAndAttach2Wide paths did not yield a usable initial event. Suspended owned-child plus DbgEng AttachProcess reached attach event, but IDebugControl4::AddBreakpoint2 returned 0x8000ffff; ExecuteWide breakpoint setup returned 0x80040205.",
    "untracedRun": "One untraced 4-cycle hidden Compose run emitted native-debug.json; qualification is hidden native completed frames only, not a paired memory result.",
    "wpr": "Prior WPR heap attempt denied with 0x80070005; no allocation trace.",
    "artifacts": [
      {
        "sha256": "34A00737C6881C62FB45E3A20E5418B1E86370494501B916984528E6106AFF60",
        "name": "RedTilde.Rt24Memory.exe"
      },
      {
        "sha256": "87DAF3B357BE356FC725A48337D2DA9D0E035F46B2D22C71C5E34D0B5921E38B",
        "name": "RedTilde.pdb"
      },
      {
        "sha256": "6F2B34A43AFBDA8F3D18A04489E051DA7DC7BD3912EA42775AC164BC4C9556DE",
        "name": "DbgEngOwnedProbe.cpp"
      },
      {
        "sha256": "62160C704F6EBE8F1207970D5884F19B0D84C26CE5C0E0A6837173A3A6F3B594",
        "name": "DbgEngOwnedProbe.exe"
      },
      {
        "sha256": "70EA07F057DEA830655442C37200D47D8F6683F1B751AD95D950945C000D0E33",
        "name": "native-debug.json"
      },
      {
        "sha256": "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855",
        "name": "allocation-stacks.txt"
      },
      {
        "sha256": "3DBB6D01D3240783F0954FDCA544B651278205CFF92CE8DCEAF72E1ECF05BAB5",
        "name": "debugger-status.txt"
      }
    ],
    "remainingProcess": "No process remained at the exact copied exe path after probe.",
    "skipBuildAttestationReview": {
      "scope": "Read-only current-state comparison; no gate or build invoked",
      "currentReceiptMatchesCurrentIdentity": true,
      "currentReceiptMatchesMeasurementIdentity": true,
      "currentBuildInputMatchesMeasurement": true,
      "currentArtifactMatchesMeasurement": true,
      "currentIncrementalReceiptPresent": true,
      "currentIncrementalReceiptState": "PASSED",
      "logRecordsSkipBuildGateFailureWithoutExceptionText": true,
      "conclusion": "Refusal is not reproducible from current source, build inputs, toolchain, output artifacts, or receipt; all checked identities match the existing measurement and successful receipt. The historical gate failure provides no exception detail. Cause remains undetermined; a transient mismatch during the earlier tooling-to-gate interval is possible but unproven."
    }
  }
}
```
