# RedTilde production review ledger

Read-only source review at source commit `c7456854c8f964bb9ea067295e6435f99d426515` (docs-only coordinator commit `d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5`). The ledger records 48 completed file reads with SHA-256 and per-file caller, ownership/lifetime, failure/cancellation, and test-coverage notes. No tests or builds were run.

```json
{
  "reviewedCommit": "d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5",
  "sourceCommit": "c7456854c8f964bb9ea067295e6435f99d426515",
  "scope": "Assigned Common/, RedTilde/ production sources and application test/support/tooling files; all 48 files read, hashed, and reviewed. No tests/builds run.",
  "files": [
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
      "sha256": "9e7eeece032dadee3f79a4044ff148dad4d0434f9e6e60f3422af766bc1949ff",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\UiControls.h inspector uses syntax-backed extraction; repository-wide rg across Common/, RedTilde/, Tests/ found no production EquationAt call.",
        "ownershipAndLifetime": "Value-oriented metadata; no long-lived ownership found.",
        "failureAndCancellation": "Malformed/absent metadata uses fallback handling; parsing limits not separately stress-tested.",
        "testCoverage": "Core app tests exercise document extraction; inspector currently uses syntax-backed extraction."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [
        "EQAT-TEST-ONLY"
      ],
      "disposition": "simplification-candidate",
      "testIds": [
        "Math.NativeSubsetAndRejection"
      ]
    },
    {
      "path": "Common/DocumentInfo.h",
      "sha256": "0414aea3bdbc67f49b97109178dcb6e42a6d9b51460a62766b4bfc59bc005b8b",
      "reviewNotes": {
        "callers": "D:\\RedTilde\\RedTilde\\UiControls.h inspector uses syntax-backed extraction; EquationAt has only the in-scope test in Tests/Application.Tests.Core.cpp.",
        "ownershipAndLifetime": "Value types; callers own returned values.",
        "failureAndCancellation": "Failure is represented by optional/result conventions in declaration.",
        "testCoverage": "Core tests cover extraction contract; EquationAt only has a test caller, no production caller."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [
        "EQAT-TEST-ONLY"
      ],
      "disposition": "simplification-candidate",
      "testIds": [
        "Math.NativeSubsetAndRejection"
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
      "sha256": "9544bf865901ef66981a755e5e66c7efe36e152aff2bfd26b3537ad1c2a464fcc",
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
      "sha256": "fcbc97ceb109675cada709c1c0fb556cb2c9aff62d1fd33f1cc8243b54e7f6e",
      "reviewNotes": {
        "callers": "Chrome command/menu paths call SetFocusControl during navigation transitions; no proven redundant invocation.",
        "ownershipAndLifetime": "Application owns chrome controls and routes focus through host; close path requests focus once.",
        "failureAndCancellation": "Focus transitions validate tree membership; callback mutation can invalidate candidate.",
        "testCoverage": "Interactive coverage is focus-taking and was not run; existing tests exercise focus transitions."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": []
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
      "path": "Tests/Application.Tests.Core.cpp",
      "sha256": "8e6ae80baa9edc6b4154a4623b195075167ec72f3d97ac5eef2ac0066de2e3b8",
      "reviewNotes": {
        "callers": "Exercises Application core and is the sole in-scope EquationAt caller; no production EquationAt caller exists in Common/RedTilde/Tests.",
        "ownershipAndLifetime": "Test fixture/application scope owns created controls and teardown.",
        "failureAndCancellation": "Cases assert stale/cancel and lifecycle outcomes; suite not run in this review.",
        "testCoverage": "Core coverage includes document extraction; EquationAt is test-only/dead-helper simplification candidate, not runtime defect."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [
        "EQAT-TEST-ONLY"
      ],
      "disposition": "simplification-candidate",
      "testIds": [
        "Math.NativeSubsetAndRejection"
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
      "sha256": "f6b00df14da0d41ddc4d0d2a4aa5fb1a17f8795da1e3ce7efa71aa40f9b5989e",
      "reviewNotes": {
        "callers": "Refresh tests call deterministic capture/render scenarios and inspect app behavior.",
        "ownershipAndLifetime": "Harness controls fixture/app lifecycle; test data is synthetic.",
        "failureAndCancellation": "Failures report scenario mismatch; no cancellation claim beyond harness.",
        "testCoverage": "Read entire file including previously skipped middle segment; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/Application.Tests.DesignRefresh.cpp"
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
      "sha256": "942e80483caf9d91d3a8600ec5874971b5b47f70948c77efbd04bde0aa2304be",
      "reviewNotes": {
        "callers": "Catalog maps native test cases to affected scopes/callers.",
        "ownershipAndLifetime": "Declarative metadata, no runtime ownership.",
        "failureAndCancellation": "Missing mapping can omit affected tests; validate against inventory on regeneration.",
        "testCoverage": "Not executable tests; catalogue consistency is a coverage risk."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Tests/native-case-catalog.json"
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
  "limitations": [
    "This is scoped review evidence, not full repository review or qualification.",
    "No tests/builds executed; focus-taking suite not run.",
    "Per-file hashes identify reviewed source versions, not proof of runtime behavior.",
    "Potential storage race coverage gap is documented, not a validated defect.",
    "Recovery/settings post-failure hang remains investigation only."
  ],
  "filesReviewed": 48,
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
