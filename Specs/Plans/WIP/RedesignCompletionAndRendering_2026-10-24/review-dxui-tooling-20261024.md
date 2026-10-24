# DxUi build, data, resources, and samples review

```json
{
  "reviewedCommit": "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
  "sourceCommit": "b06f534b36ffc2c0c9671a142d1e51545480be22",
  "status": "REVIEW_COMPLETE",
  "scope": "Completed static review of all 114 DxUi build/data/resources/samples inventory rows: 78 individually read source files and 36 retained hosted fixture reports verified by archive provenance and deterministic consumer tests. No builds were run; focused tooling tests are listed in coverage.",
  "files": [
    {
      "path": ".github/workflows/ci.yml",
      "sha256": "4efd69a3b5ef1f3d495cc24f5642883faae62a840ec8ef22bdade731f3292d24",
      "reviewNotes": {
        "callers": "Push-to-main, PR merge-ref, and manual dispatch invoke tooling/native gates; paired benchmark uses Get-BenchmarkScope and workflow outputs.",
        "ownershipAndLifetime": "Workflow uses pinned action SHAs, contents:read except the isolated gallery workflow, finite per-job deadlines, concurrency cancel for superseded runs, retained artifacts for 14 days.",
        "failureAndCancellation": "Scope decision errors fail; native jobs run if scope undecidable; cancellation does not publish a verdict. Paired setup/measurement/restoration/verdict stages are gated and artifacts upload after failures.",
        "testCoverage": "Tooling cases: Tools/tests/Test-CiGate.ps1, Test-NativeScope.ps1, Test-BenchmarkGate.ps1; native/paired hosted workflows are not executed in this review."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "CI tests: Test-CiGate, Test-NativeScope, Test-BenchmarkGate"
      ]
    },
    {
      "path": ".github/workflows/format.yml",
      "sha256": "d38b2ff4b5948e01839064c9388c051a19d0959c0f42f2216a010fa0289585fc",
      "reviewNotes": {
        "callers": "Push/PR path filters call pinned clang-format install and format.ps1; manual opt-in reformat writes only non-main branches.",
        "ownershipAndLifetime": "Read-only permission in check job; write token only in explicit reformat job, branch-gated; no product artifact ownership.",
        "failureAndCancellation": "Check job uploads diff patch on failure; format errors stop. Reformat is gated by manual input and non-main ref; no tests in this read-only pass.",
        "testCoverage": "Tools/tests/Test-Validation.ps1 plus format.ps1 -Check are declared gates; neither run in this review."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "format.ps1 -Check; Test-Validation"
      ]
    },
    {
      "path": ".github/workflows/gallery.yml",
      "sha256": "6546eb19d959f7171600e453e3605105bec9d62b1ba2e04182ac6af5891c5bac",
      "reviewNotes": {
        "callers": "Manual publish workflow runs vcpkg restore, gallery.ps1 -PublishDocs, validate-specs, then Commit-Gallery.",
        "ownershipAndLifetime": "Write permission scoped to publish job; checkout branch only; publish condition refuses main; gallery source generated from control test and embedded sample outputs.",
        "failureAndCancellation": "Validation runs before commit; ordinary non-force push; no post-push validation step is present in workflow, but commit step is after spec validation. Workflow not executed.",
        "testCoverage": "Tools/tests/Test-Docs.ps1 checks gallery/catalog artifacts; gallery test suite and validate-specs are workflow gates, not run here."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-Docs; Gallery suite; validate-specs.ps1"
      ]
    },
    {
      "path": "build.ps1",
      "sha256": "357dccefd0839bac6acfc8833a2c80b0493577ab599d673b1320fb773eb0f2a5",
      "reviewNotes": {
        "callers": "Root build entrypoint; vcpkg-install.ps1, performance/test entrypoints call it. Discovers VS via Tools/VisualStudio.psm1 and emits six bounded phases via Tools/PerformancePolicy.psm1.",
        "ownershipAndLifetime": "Default outputs/logs under .build; optional consumer output root isolated; checks exact executable paths through CIM but never terminates; default output builds get before/after source/toolchain/artifact attestation.",
        "failureAndCancellation": "Missing dependencies/VS, running exact output exe, failed MSBuild or failed post-attestation stop success. Build is not run.",
        "testCoverage": "Test-BuildMatrix and Test-PerformancePolicy cover adjacent configuration/attestation contracts; no build execution claim."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix; Test-PerformancePolicy"
      ]
    },
    {
      "path": "Build/DxUi.AddressSanitizer.targets",
      "sha256": "25ebf22bd76677353b2c689c9b08793fccd99252a5b982a15dbd50aafb0ca059",
      "reviewNotes": {
        "callers": "Imported by Build/DxUi.Consumer.targets and Directory.Build.props/targets to stage ARM64 ASan runtime.",
        "ownershipAndLifetime": "Runtime path derives from selected VCToolsInstallDir; MSBuild owns staging under target output; no manual cleanup.",
        "failureAndCancellation": "Missing runtime emits MSBuild Error before copy; other platforms use built-in target path.",
        "testCoverage": "Tools/tests/Test-AsanRuntime.ps1 has dedicated runtime validation; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-AsanRuntime.ps1"
      ]
    },
    {
      "path": "Build/DxUi.Consumer.props",
      "sha256": "52d2cf3eeaffc860c7e3ba9ff54c3cd9e02c5615cd9561da7b7e28f6deece57c",
      "reviewNotes": {
        "callers": "Imported by external consumers before their target files; contributes public include and pinned installed-WIL include paths and optional STL-annotation define.",
        "ownershipAndLifetime": "Resolved root is relative to props location by default; output root is consumer-supplied and does not write itself.",
        "failureAndCancellation": "Empty/malformed roots surface through downstream target validation; file itself has no error task.",
        "testCoverage": "test-consumer.ps1 generates relocated external project and exercises this import; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 external public consumer"
      ]
    },
    {
      "path": "Build/DxUi.Consumer.targets",
      "sha256": "0d62da7c448da3cb49c6971a6ef373349841727ef5c82931774a521352b76120",
      "reviewNotes": {
        "callers": "External consumer imports after Microsoft.Cpp.targets; references exactly src/DxUi.vcxproj with matching platform/config/output/annotation properties.",
        "ownershipAndLifetime": "ASan forced include and project-reference props follow resolved root and isolated output root; no source list duplication.",
        "failureAndCancellation": "Before PrepareForBuild validates config, lock, toolset/SDK/CRT-adjacent identities and restored WIL; validation process nonzero stops build.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 checks six config mappings; test-consumer.ps1 compiles relocated fixture (not run)."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix; test-consumer.ps1"
      ]
    },
    {
      "path": "capabilities.json",
      "sha256": "782da4cee202c526e932665011de9360aa224033d7c9df71cef6075034b33b16",
      "reviewNotes": {
        "callers": "Read by consumer-interface validation, test-consumer.ps1, docs/spec validation and consumers to discover API revision and supported/pending capabilities.",
        "ownershipAndLifetime": "Machine-owned public interface lists exact scripts, module functions, MSBuild imports, and headers; exact vcpkg API/toolchain versions owned by separate manifests.",
        "failureAndCancellation": "JSON parse/schema or missing consumer entries fail validation; pending lists explicitly separate consumer/platform/runtime qualification.",
        "testCoverage": "Tools/tests/Test-Validation.ps1 and Test-ConsumerUpdate.ps1 validate interfaces and update semantics; no validator run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-Validation; Test-ConsumerUpdate"
      ]
    },
    {
      "path": "Directory.Build.props",
      "sha256": "a9835e32463f717079de40bb1294b0fe1d1b83e71c160be605683fb366e92be2",
      "reviewNotes": {
        "callers": "Imported by DxUi solution native projects; default output/int dirs, triplets, Unicode/C++ language/runtime/warnings and ASan configuration.",
        "ownershipAndLifetime": "Repo-root paths normalize to MSBuildThisFileDirectory; separate output and intermediate roots; ASan binary copy owned by DxUi target only.",
        "failureAndCancellation": "Unsupported platform/configuration/annotation option errors before PrepareForBuild; no compile performed.",
        "testCoverage": "Test-BuildMatrix.ps1 checks project config; Test-AsanRuntime verifies runtime contract; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix; Test-AsanRuntime"
      ]
    },
    {
      "path": "Directory.Build.targets",
      "sha256": "cc008a910e83c5fc4b213060f29b77df1bf8de929fa0d904e3181cfbd618c0cb",
      "reviewNotes": {
        "callers": "Imported at solution-project build end; adds ASan forced include/instrumentation and validation target.",
        "ownershipAndLifetime": "Compiler forced include derives repository root; no external resource ownership.",
        "failureAndCancellation": "Unsupported profile and invalid annotation flag fail before PrepareForBuild.",
        "testCoverage": "Test-BuildMatrix and Test-AsanRuntime are adjacent tooling coverage; no MSBuild executed."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix; Test-AsanRuntime"
      ]
    },
    {
      "path": "DxUi.sln",
      "sha256": "6cd86f1e17aea6e7518082afd3e1730e24becc31bc2acd30c2b106a9f468b50e",
      "reviewNotes": {
        "callers": "Root build.ps1 invokes this solution; contains DxUi static library, Foundation/Control/Embedded tests, embedded sample and InteractiveLease.",
        "ownershipAndLifetime": "Six Debug/Release/ASan Debug x x64/ARM64 mappings are explicit for all six project GUIDs.",
        "failureAndCancellation": "Solution configuration mapping is validated statically; no build failure/runtime behavior can be inferred from solution text.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 statically validates all live .sln/.vcxproj mappings; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix.ps1"
      ]
    },
    {
      "path": "format.ps1",
      "sha256": "4b120fbcffe2c6f74afd8ba880c401f6dab4f7e082ee356b42c916acecfafdef",
      "reviewNotes": {
        "callers": "Called by CI formatting workflow and developers; formatter resolved from explicit env/arg, repo pinned install, PATH, then VS LLVM.",
        "ownershipAndLifetime": "Recursively targets owned cpp/h under src/include/Tests/Samples; `-Check` no writes; non-check preserves timestamps for clean files.",
        "failureAndCancellation": "Rejects missing/wrong clang-format 22.1.3; formatter nonzero throws at file.",
        "testCoverage": "CI runs format.ps1 and -Check; no formatter installed/invoked in this review."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "format workflow; format.ps1 -Check"
      ]
    },
    {
      "path": "gallery.ps1",
      "sha256": "a29695b042f77c25026756299dd8179272d11746bcedee6bc21b35bdf48036e6",
      "reviewNotes": {
        "callers": "Builds or reuses suite output; runs ControlTests Gallery and EmbeddedControls sample; optional docs publish to docs/gallery.",
        "ownershipAndLifetime": "Temporary output defaults .build/gallery; published receipt captures source commit/dirty state/profile/native arch/control count/image hashes; catalog count derives ControlCatalog.cpp.",
        "failureAndCancellation": "Native test/sample nonzero or image count != 6 fails. Publish copies images/index/readme/receipt after render; no gallery was generated.",
        "testCoverage": "Control Gallery suite and Tools/tests/Test-Docs.ps1 validate output contract; not run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Gallery suite; Test-Docs"
      ]
    },
    {
      "path": "performance.ps1",
      "sha256": "52c863186486b235914d5558a982ddbe9c1d72efb29d41c68a474e604b19ab0f",
      "reviewNotes": {
        "callers": "Invokes build optionally; launches DxUi.EmbeddedTests.exe with selected benchmark scenario, then Tools/Compare-Performance.ps1.",
        "ownershipAndLifetime": "Requires native matching architecture and valid v2 build attestation pre/post; output and comparison receipts carry exact build identity.",
        "failureAndCancellation": "Removes prior path before run; nonzero/missing receipt/state drift invalidates and relocates produced native receipt; baseline path distinct.",
        "testCoverage": "Tools/tests/Test-PerformanceComparison.ps1 and -Forms cover comparison/parser; no native benchmark run."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-PerformanceComparison; Test-PerformanceForms"
      ]
    },
    {
      "path": "Samples/ComplexUi/ComplexUiScene.h",
      "sha256": "425c830c22ec3672f52f6569bebd5fe2ababc45dfc47f530523eeb0f4c3a3041",
      "reviewNotes": {
        "callers": "Included by EmbeddedControls Main.cpp; ComplexUiScene is shared deterministic 16-slider/16-progress + Tree/Grid model fixture.",
        "ownershipAndLifetime": "Scene owns EmbeddedHost before borrowed model destruction ordering; controls are owned by root while pointers are non-owning; model synthetic 1,000 rows/four columns.",
        "failureAndCancellation": "Grid text lookup assumes valid row/column indices from control caller; no failure path for invalid indices; fixture is not production data.",
        "testCoverage": "test-consumer.ps1 copies the scene and runs --complex-ui capture; performance.ps1 benchmark uses corresponding library workload; neither run here."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 --complex-ui; performance benchmark"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/DxUi.EmbeddedControls.vcxproj",
      "sha256": "21124b1f5d13204b57f37d10ce3d960306c455713e532ecb13156ce3178f61ec",
      "reviewNotes": {
        "callers": "Solution builds standalone sample and references one DxUi.vcxproj.",
        "ownershipAndLifetime": "Declares all six profile/platform configurations and supplies no consumer-specific `.cpp` list.",
        "failureAndCancellation": "MSBuild project reference and common configuration validation stop unsupported profile mapping; no compile run.",
        "testCoverage": "Tools/tests/Test-BuildMatrix.ps1 checks six config mappings; test-consumer.ps1 separately relocates sample source."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "Test-BuildMatrix; test-consumer.ps1"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/EmbeddedScene.h",
      "sha256": "e7da318e233c1f6a9a465589494a47b99013374af4d2f5b8b5d1834daf4b3c9d",
      "reviewNotes": {
        "callers": "Main.cpp initializes this scene for playground, text-input, or rich-text mode; controls call back into scene-owned state.",
        "ownershipAndLifetime": "EmbeddedHost owns tree; model/control pointers are borrowed from root; all sample strings/assets are synthetic.",
        "failureAndCancellation": "Markdown command callback catches named std::exception and reports failure; failed insertion leaves text user-facing message; no device/app services.",
        "testCoverage": "test-consumer.ps1 runs default/text-input/rich-text relocated modes; no sample execution here."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 embedded/text-input/rich-text smoke"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/EmbeddedTextClient.h",
      "sha256": "67f3dbf109a3e72e7be2a7d9bc89f5f9351c4d2e968666f7cd97612c44bf2f73",
      "reviewNotes": {
        "callers": "Main.cpp supplies this public TextInputClient/PasteTransform adapter to TextInputServices in headless smoke and live window.",
        "ownershipAndLifetime": "Borrowed EmbeddedHost/HWND disconnected before teardown; identity bound to captured focus id; geometry converts DIP to screen with DPI rounding.",
        "failureAndCancellation": "Disconnected/stale focus return TF_E_DISCONNECTED/S_FALSE; Win32 conversion failures return HRESULT; range conversion rejects nonfinite/out-of-range.",
        "testCoverage": "test-consumer.ps1 text-input mode calls RangeBounds, paste and render; no direct test of disconnect races/DPI extremes in sample smoke."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 --text-input"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/GraphicsFixture.h",
      "sha256": "b242c5efa16862ccf416cc2c7a277a2e78354273ef43d3cfdf48c69bbef5c19d",
      "reviewNotes": {
        "callers": "Used by Main.cpp for supplied WARP device, target surface, bind/composite readback and PNG generation.",
        "ownershipAndLifetime": "D3D/DXGI/WIC COM objects use wil::com_ptr; read staging texture and mapped resource are local; Unmap guarded with scope_exit.",
        "failureAndCancellation": "HRESULT propagation on device/map/codec failures; `pixels.resize`/filesystem exceptions propagate to main exception boundary; no bounds check for width*height product.",
        "testCoverage": "test-consumer.ps1 headless default/complex/text modes and gallery sample render; no output generated."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 sample modes; gallery.ps1"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/Main.cpp",
      "sha256": "b41777902b08f640535075637480134c249b29423d71601923f1c83cb07551dc",
      "reviewNotes": {
        "callers": "Standalone sample entrypoint; message pump dispatches public embedded host/input services; CI and gallery invoke `--output`, external consumer tests invoke several smoke modes.",
        "ownershipAndLifetime": "App owns window/services/image-loader; teardown stops loader/disconnects text then destroys window; ImageLoader callback uses copied state; sample fixture uses synthetic text.",
        "failureAndCancellation": "Main COM initialization is fail-closed on FAILED(CoInitializeEx) and calls CoUninitialize only after successful initialization through scope_exit. Callback catches are named exception types with documented fallback; no catch(...) or COM-init bool fallback was found.",
        "testCoverage": "test-consumer.ps1 sample build/render covers normal consumer/sample integration but no deterministic exception-injection or COM initialization failure seam was identified; these remain test gaps rather than confirmed findings."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 relocated sample smoke; gallery.ps1 embedded example"
      ]
    },
    {
      "path": "Samples/EmbeddedControls/RichImageLoader.h",
      "sha256": "533fee661a99338dec13adb775986d413f0a0b62b276e70afa3e550b716f7801",
      "reviewNotes": {
        "callers": "EmbeddedScene exposes RichTextEditor; Main.cpp initializes loader, submits pending requests, drains results from ImageReady/Render.",
        "ownershipAndLifetime": "One threadpool work item; callback retains shared State, owns WIL threadpool handle, serialized queues/results, explicit 32-job/64MiB bounds; no UI/editor pointer on worker.",
        "failureAndCancellation": "Shutdown marks closing, nulls HWND, cancels queue and releases owner without waiting; worker checks cancellation/closure and no late post; `PostMessageW` failure is ignored, so queued result waits for another render request (sample lacks retry policy).",
        "testCoverage": "test-consumer.ps1 rich-text sample smoke only; no worker saturation, cancellation, HWND reuse, decode error matrix test in external smoke."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "reviewed",
      "findingIds": [],
      "disposition": "reviewed-no-confirmed-finding",
      "testIds": [
        "test-consumer.ps1 --rich-text"
      ]
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A2-vs-A1-control.comparison.json",
      "sha256": "21dce5ab319123f6117a2dbaae18b1e0204ddf9d1f579cf36595420d89773134",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A4-vs-A3-control.comparison.json",
      "sha256": "a0f8a16bb190a4f2fadf866609531e606b7557a03a659ddd038981d5e6eb7935",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-A6-vs-A5-control.comparison.json",
      "sha256": "c717ad2cca2385eecb30e5e799aeb16a1c3cc0fae83ee06308e29485230dc06a",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B2-vs-B1-control.comparison.json",
      "sha256": "0737d81af29842f6d2999c3dded97ef8a8aaeba3bf0d873dae01cb731cfd9362",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B4-vs-B3-control.comparison.json",
      "sha256": "0e60ddcdb695d8138d8e52f7f268f16e2545b343baa29c229eb565b390d1edfd",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/Default-B6-vs-B5-control.comparison.json",
      "sha256": "43e15928fb9f0ae8b7d19c5d9fa343da240ffb5a8b28f148bb8bd5c5f6a338c8",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A2-vs-A1-control.comparison.json",
      "sha256": "a5aa87f6cedc526ed68fe831120487c71a16c44188217d3647ddf873d66782f2",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A4-vs-A3-control.comparison.json",
      "sha256": "9ab653dd1fd31c89a0ff39fba8b25328a70ebc7aec6822bde8d48b308478b2b0",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-A6-vs-A5-control.comparison.json",
      "sha256": "213b31c86b9992aa8a7d7a22b26d72ff184b8e4b6bf72f4910abf2e8df5480ef",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B2-vs-B1-control.comparison.json",
      "sha256": "ec5defb095541fb107ddf61c7e2c72f785efc857dcc1b2b2b5b2c9b275515283",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B4-vs-B3-control.comparison.json",
      "sha256": "6995db5084421b5ca71f44d90c61a7385d5d05c30b63eaeb841c7c59fea99410",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGrid-B6-vs-B5-control.comparison.json",
      "sha256": "ae8c972581e0cf1ecab09d488090e9b95826c0380f02a6da3175931a9826799f",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A2-vs-A1-control.comparison.json",
      "sha256": "abc10a816bb77b7560cd389a9ba886c6afedff204a6acb8e226c7560b64a74bb",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A4-vs-A3-control.comparison.json",
      "sha256": "503578dc958dd3a25ff519aebbeb366a11e256bfa71193df452b0f8d21f29e1b",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-A6-vs-A5-control.comparison.json",
      "sha256": "01e32b94abe8c702cc36f75bec0d5b76cb8dd9bfcfbd29c916b2ff6f2389222b",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B2-vs-B1-control.comparison.json",
      "sha256": "f76d4810873a660d18cd3e059ce2669be09c2365d9e4f3989fb0568c63d69d9a",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B4-vs-B3-control.comparison.json",
      "sha256": "5f1094bb091420a3c1f72b7f15d6514afccdfbac77faaac8e33111244fcf5a43",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-1/MultilineGridDistinct-B6-vs-B5-control.comparison.json",
      "sha256": "02c0d052943fab5d8f9c5748b36139fafa06af692f5dbd2984e9345300760dad",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-1) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A2-vs-A1-control.comparison.json",
      "sha256": "240b961a2852b0ac633bd80906b117e50cbe0de5732a73b359247a3029e8137d",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A4-vs-A3-control.comparison.json",
      "sha256": "b990652646c0ce7b988d8fc054bf7f5e0fe7bf6187085c4411d9843edfe93419",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-A6-vs-A5-control.comparison.json",
      "sha256": "1a74eceeadb4d43e42581ef2fc3f23df1bd857c469e4b8cdb974b900954e3779",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B2-vs-B1-control.comparison.json",
      "sha256": "7a3dbe6b65686c36b15835beee7127425782744221508aa696227924d5668e65",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B4-vs-B3-control.comparison.json",
      "sha256": "0b47d08c69a42ec4d530d948023b1e04fbdf582d81d5cabfbb0c2a92fcfa029c",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/Default-B6-vs-B5-control.comparison.json",
      "sha256": "43f8402220eefffc26103068752a967f958d5d68420b5c463b7dfdc63b780b7a",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A2-vs-A1-control.comparison.json",
      "sha256": "3f38264505886022a8cf17a7c98d500c584d4d434206fd3d908f22eb0cfe567a",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A4-vs-A3-control.comparison.json",
      "sha256": "757fa101ab81fc2ae794fe85d950930cb9804fa3db0c4504ee365b966db2482d",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-A6-vs-A5-control.comparison.json",
      "sha256": "59b141c84d7472e94e8b5172aa78ac2d9fa5d682d74d3fbbd5a74b4f8777ce7a",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B2-vs-B1-control.comparison.json",
      "sha256": "1516cb6d10bee1cac9c36c38aef2644467b513e52a3043e6a667b39984e3f9f6",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B4-vs-B3-control.comparison.json",
      "sha256": "72a81e7f04b318810b9352e44967d1682757529f2d5572a60266b0bc46fa0c0b",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGrid-B6-vs-B5-control.comparison.json",
      "sha256": "6750f9ef143cd51c447da44b103a55892bb50f88d1757f485c8811946fdc6841",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A2-vs-A1-control.comparison.json",
      "sha256": "3a5be21c899f2e581ab42b2db03285c0bc05d7c02a8104fe5b6557ea92fa93f3",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A4-vs-A3-control.comparison.json",
      "sha256": "4f79836ce892f6c622cd4a1cdb71a286f39520a807993d0d2e67c1c70779c80f",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-A6-vs-A5-control.comparison.json",
      "sha256": "9a1c6dee3adfbb89cb9a30fe3e3fdb9cd4f58791e3e8f7af5af493da2c435767",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B2-vs-B1-control.comparison.json",
      "sha256": "0c3ef826e4c36b94c46e931f252baab962dc3b483381d98c1191196868b3c2eb",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B4-vs-B3-control.comparison.json",
      "sha256": "2756f7184cd6d6f51a2213d5a11499a469c98f7e279ed95e1122c01f1bbb07a5",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/aa-2/MultilineGridDistinct-B6-vs-B5-control.comparison.json",
      "sha256": "19ffd936ec4255120cd2c9ec65619cbf1b1977b7ba915e6263d033e836f949c5",
      "reviewNotes": {
        "callers": "Test-BenchmarkGate.ps1 Get-HostedAaFixture(Tools\\tests\\fixtures\\HostedPairedGate-2026-10-01\\aa-2) loads this archived comparison named by the retained summary; it is part of the exact gate fixture set.",
        "ownershipAndLifetime": "Read-only retained hosted measurement fixture copied from archived packet; archive SHA-256 matches README pin and all 19 entries compare byte-content-equivalent after documented newline normalization. No summary-level raw identity values are repeated here.",
        "failureAndCancellation": "The gate parser treats malformed or inconsistent comparison receipts as invalid evidence; these fixtures expose control stability and exact-budget contract behavior. Source evidence verified statically; test not executed.",
        "testCoverage": "Test-BenchmarkGate.ps1: Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration; 18 controls plus one summary per archive and preserved archive checksum."
      },
      "reviewer": "review_ledger",
      "reviewStatus": "generated-verified",
      "findingIds": [],
      "disposition": "provenance-and-consumer-verified",
      "testIds": [
        "Test-BenchmarkGate.ps1: the second retained hosted A/A set remains inconclusive without policy-qualified calibration"
      ],
      "generatedProvenance": "Tools/tests/fixtures/HostedPairedGate-2026-10-01/README.md pins original archive hash; retained archives under Measurements/HostedPairedGate/2026-10-01; Test-BenchmarkGate.ps1 loads the fixture set and asserts aggregate result."
    },
    {
      "path": "Tools/PerformanceProfiling.psm1",
      "sha256": "12b8ae312a16e836f22779d2429c56f96df235c985df26b06142042f58cdce72",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "performance-profile.ps1",
      "sha256": "8ad8cf24470a7521739b7c5ad7d56ab9466ed6b66c7cc2e63db4f94493b0eceb",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "performance-forms.ps1",
      "sha256": "33d9ec1a16e25dde7980898a950b2bf93e37d58305c8b122eaa312b0d156c28e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "performance-paired.ps1",
      "sha256": "ae1694ec9de45e1bdb3ca6af9f6f6132cafb93593b9ba2287903b0de22324224",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/PerformanceComparison.psm1",
      "sha256": "fd701df248bc621d9942f47df12ef02fe0d929d07290b18128ea644c848a8cae",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/ConsumerBuild.psm1",
      "sha256": "833607c3f020a47642dcda2485811abd86d8f1e87f0a408b82fb71d7150bd587",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/ConsumerUpdate.psm1",
      "sha256": "d09a0919a8559cd8c8133b7078a37b416197be24d13b9f128dc7db3a1331f893",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/VisualStudio.psm1",
      "sha256": "bf2ac054c30feaea2b8cc1322fc1d67854660e90fc2dcce17e26ee21ad6071fd",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/VcpkgTriplet.psm1",
      "sha256": "7bb17cc7dd2377e1eb4cc62de710b7ecff9e614ff7b6dc31cd32ccc47af488b7",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/ConsumerApi/ConsumerApi.Tests.Revision4.cpp",
      "sha256": "430499d22dc93bdecb47824f2d2716db3d32f42afd6d0b66515da7a1651a6111",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/PerformancePolicy.psm1",
      "sha256": "a0c774ffbf8422307d3d0138a33cbc08b958428186de846b87eaccbcde7f0e01",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "test-consumer.ps1",
      "sha256": "c996b897cfaf1da879ed1a8db8f24b77c644a584ad378d9e9511ab73f34d489e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "vcpkg-install.ps1",
      "sha256": "be905aeb852327bf4823d8ce82c6e3cf10103980767f9044ce6675ad67207fec",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/PairedRun.psm1",
      "sha256": "b72612a3d39717a40d64d6fb5a7edb3b3b6768d768179b8ef44db41476b7834f",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Generate-MarkdownEntityData.ps1",
      "sha256": "5db8807d744de4a3c250ca533c25a2f2e93c4132bdeff392164e29fea3061b7c",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Generate-MarkdownTestData.ps1",
      "sha256": "61bef352621c5dcd171e66fde31da46299ba5f0465628193fc4b41c0fd7503ea",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Generate-MarkdownUnicodeData.ps1",
      "sha256": "3d2dea582dc9e1fd207c4853ba2cd610d6aa07284e7899a52184b2e92dfbfb4d",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Generate-UnicodeGraphemeData.ps1",
      "sha256": "f5c68eda1ac662460519171ee0ea2f681a966692d095758cce59d02d026e3c6e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Generate-NativeFlowchartTestData.ps1",
      "sha256": "d5becb01f014682c138aaebb48ef1f8135dcb1cdcac6e72d2062390556d7db86",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Test-Changes.ps1",
      "sha256": "5b9fabad89c8ef2e810a2f4784d3b04e40962c7a59ac2d2449ef023f35e583ec",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Test-PrePush.ps1",
      "sha256": "c161c3569d27bf15c462e693d314c0e3058949b115d19889561596735c9a96ff",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "test.ps1",
      "sha256": "d02b57752be28864bd9f0f4ed4235ec86dd74b2ea65aac76e74ccf2aed867a9e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/CapabilitySkipPolicy.psm1",
      "sha256": "95bb89554c49b6c2c2526c21aba7d862b201fb1d637948e699d1aa493e613824",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/PerformanceAcceptancePolicy.v1.json",
      "sha256": "8d97d5fb2ad216a3ea448dd5e64558f162d3a7920170659b3ae3fc7ae2b6bf23",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "vcpkg-configuration.json",
      "sha256": "7c8f088b33838d9f1c6481d2120bcf8445cdee45232779fbb9b20964caa54de3",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "vcpkg-tool.json",
      "sha256": "e38884614404c881238070ce5ae2c25a72a68718a3b03c8dccb26fa22586756e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "vcpkg.json",
      "sha256": "fe54dd43ba2d384eee8a0034daa3e22f0204ffc4390be1cd8eb8456e295b6bfc",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Get-BenchmarkScope.ps1",
      "sha256": "7cd8848231bfab514e205364030d5c12268b64301d31c223091920cc9332d826",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Get-NativeScope.ps1",
      "sha256": "6981ebc6344a6892e8a1cc7291d0fe914fa1c7514eca5cd43150d82255c27228",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/NativeScope.psm1",
      "sha256": "35f95b66205fd5baaeedaa10eb45aceae8a919ae51584f77cd2757ae5e390772",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Install-ClangFormat.ps1",
      "sha256": "a9b634be43b77764e9e6f0d15cb145c9f9bb4610065053d843f1c1770af945e8",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Commit-Gallery.ps1",
      "sha256": "2e2c6ff28384d7e07d507188cd28421708bbfc0e71470844dbe8fabc6170d08c",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Changelog.psm1",
      "sha256": "05e987cf29bfb8aedbda5f77feba56d0145f733ccc8c577f0fcc96e1e877dcc3",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate.ps1",
      "sha256": "68ba34c47acccc7742b0884ae78d8de69f7f9a6c772c74c53c4ef00373d1d2b2",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate-build-matrix.ps1",
      "sha256": "621dc39261b63c43ab6c2b4ea532e5340969910bc96d5e5ab5acad672f532a44",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate-dependencies.ps1",
      "sha256": "c6dffa8774dadfbe63ed9e3e5889eeae1993570edd74f01e08d40b87ce687bb1",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate-skills.ps1",
      "sha256": "cce3e47124352398a60bb7c79f286341e659ba00b0db853aa8672c72edf6253f",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate-specs.ps1",
      "sha256": "139b7257056fb441be1dbdc36649886b9c86b235c0ffb8707078ad073d7d889a",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "validate-test-port.ps1",
      "sha256": "ed545a8aca8b290b041bbed5de84398a990ff7fbc7b8b1e9c13cd2674502c2b3",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/validate_consumer.ps1",
      "sha256": "b8f4882a1c5183250dd5437570e696ac995bad04c46d4575cafc0a65fe97a82a",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Watch-BenchmarkHost.ps1",
      "sha256": "dd31f0ceff0a383fee1dc971893a85ad803a6962d6ad2c9f0b34007dd156807d",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Prepare-HostedArmDesktop.ps1",
      "sha256": "88bf0f0643942521bf3072222cde69721b2a343505de0ee695d32320ed9bc3c8",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Publish-BenchmarkVerdict.ps1",
      "sha256": "6df67e9743d5eb2c9d44c14b2460db74d06a75b5ec8cf98e3803430900555121",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/SuiteFailure.psm1",
      "sha256": "86e18de96dded74172f10e6d5c45b0e19ff4ca7578281b1ffd683ffa9541f946",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Test-CiGate.ps1",
      "sha256": "9079f91a913b64a3dbea1bf7eadaab2c21438a4fb81d5181c67c142a289c978e",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Validation.psm1",
      "sha256": "15c7ef95430c293400859446cd51e2202bbe5d0ab1ac506b8539ce1de983e756",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "src/DxUi.vcxproj",
      "sha256": "66d813eba8f3ec3d78a1a8e6b5d0e20483812e01deeb87d3388aed5e89b067e0",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/BenchmarkGate.psm1",
      "sha256": "3fb19284bc1cd7f84bdd27368f58748007dcfe6fd04ce1a8a2dc49925dd8f915",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/InteractiveRun.psm1",
      "sha256": "8b88a8df3c3199692c3898b74bd04c29e0afa9907219832679b69ba30b6938df",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/ScopedTesting.psm1",
      "sha256": "fe0b19b877fb59b3647d4345ad2c7c53b053efd1f328f7c9ba2ed8756fe1fe86",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/tests/Test-CodeOwners.ps1",
      "sha256": "70bbd429ae99c73f25acd83a1d2215cc1b5453990278c6d3614814c42754e768",
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
      "reviewStatus": "reviewed"
    },
    {
      "path": "Tools/Compare-Performance.ps1",
      "sha256": "a00bceef3722deb15acabc1f5fb2598dfa1ce662934baadb16eeb3e741a07178",
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
      "reviewStatus": "reviewed"
    }
  ],
  "findings": [],
  "inventoryPathDrift": {
    "plannedPath": "Tools/ConsumerApi/Revision4.cpp",
    "path": "Tools/ConsumerApi/ConsumerApi.Tests.Revision4.cpp",
    "evidence": "The inventory plannedPath differs from the current tracked filename. The current file was fully read and reviewed; test-consumer.ps1 compiles the revision-specific public API fixture. This is path drift only, not pending review."
  },
  "coverage": {
    "buildsRun": [],
    "pending": 0,
    "testsRun": [
      "pwsh -NoProfile -NonInteractive -File Tools/tests/Test-ConsumerUpdate.ps1 — passed (owned process lifecycle plus advisory cases)",
      "pwsh -NoProfile -NonInteractive -File validate-specs.ps1 — passed (440 Markdown, 22 authority files, 14 active plans)",
      "pwsh -NoProfile -NonInteractive -File validate-skills.ps1 — passed (9 DxUi skills)"
    ],
    "inventoryTotal": 114,
    "reviewed": 78,
    "generatedVerified": 36
  }
}
```
