# DxUi general tests review (in progress)

```json
{
    "reviewedCommit":  "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
    "sourceCommit":  "b06f534b36ffc2c0c9671a142d1e51545480be22",
    "status":  "IN_PROGRESS",
    "scope":  "Selected general-test rows. This report records full reads with concrete file notes; outsourced pending Tools/tests, Tests/Controls, and Tests/ConsumerModules rows remain pending until their owning supplements are merged.",
    "selectedCount":  112,
    "reviewedCount":  60,
    "pendingCount":  52,
    "files":  [
                  {
                      "path":  "Tests/capability-skip-allowlist.json",
                      "sha256":  "c11b73a49ef858a76e6335db4e18e2abb1b086374c6bdb8fa89853d70e0be008",
                      "reviewNotes":  {
                                          "callers":  "No direct reference found in the reviewed test/tooling text searches; the file is the native capability-skip policy input.",
                                          "ownershipAndLifetime":  "A 39-byte tracked JSON policy file containing version 1 and an empty allowances array; no generated/runtime ownership.",
                                          "failureAndCancellation":  "Empty allowlist means no capabilities are waived. No cancellation path applies to static policy input.",
                                          "testCoverage":  "The file itself has no case identifier and currently declares no exceptions; direct executable-consumer coverage is a gap to verify in the build/test manifest."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Fixture/policy input reviewed; direct test ID absent or caller review remains pending as noted.",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/ConsumerModules/ConsumerModules.Tests.Driver.cpp",
                      "sha256":  "4ba696fd015f1727b121c3cb5e584f2b17d877265a7911f3293a983726f25a06",
                      "reviewNotes":  {
                                          "callers":  "Built as the standalone consumer-module driver; wmain loads NativeModuleA.dll and NativeModuleB.dll and invokes exported DxUiModuleProbe plus the executable probe.",
                                          "ownershipAndLifetime":  "WIL unique_hmodule owns both DLL handles; each two-round UI jthread is joined before module unload, and each thread initializes STA COM.",
                                          "failureAndCancellation":  "LoadLibrary/GetProcAddress failures have distinct exit codes; probe failures aggregate and return 12. The fixture prints a count and never unloads modules while their UI thread is live.",
                                          "testCoverage":  "wmain executes EXE and both module probes twice across two generations, then joins before RAII unload. It is a focused DLL lifetime consumer fixture, not evidence of the broader native suite."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
                      "testIds":  [
                                      "wmain",
                                      "DxUiModuleProbe"
                                  ]
                  },
                  {
                      "path":  "Tests/ConsumerModules/ConsumerModules.Tests.NativeModule.cpp",
                      "sha256":  "e3998097b59b15649f801beb8c2fef8b53453149e284731d6ed638520348332a",
                      "reviewNotes":  {
                                          "callers":  "ConsumerModules.Tests.Driver.cpp resolves the exported DxUiModuleProbe from the executable and both consumer DLLs.",
                                          "ownershipAndLifetime":  "The probe creates a no-activate owned HWND, attaches/detaches ControlHost with scope guard, owns a local control tree and dismisses asynchronous context menu before callback storage is destroyed.",
                                          "failureAndCancellation":  "Returns distinct probe failures for window/attach/menu/popup/callback path; exported noexcept boundary terminates on bad_alloc and converts named std::exception to failure code after diagnostic.",
                                          "testCoverage":  "RunModuleProbe checks animation tick, dispatcher HWND/module ownership, context-menu procedure ownership and invocation/closure; exported DxUiModuleProbe reruns that behavior behind an exception-safe ABI boundary. Driver repeats it twice per generation."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
                      "testIds":  [
                                      "RunModuleProbe",
                                      "DxUiModuleProbe"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.ClipboardTransport.h",
                      "sha256":  "8ace8716cc63062dafbacdaa63fdb5479bf34634874f7c8202385db9c0cc3dcf",
                      "reviewNotes":  {
                                          "callers":  "Included by DxUi.Tests.NativeTextInput.cpp and its native input test registration.",
                                          "ownershipAndLifetime":  "ClipboardHostWindow and ClipboardTransportFixture own the native clipboard fixture; local strings/models own test payloads; embedded TextInputServices is explicitly detached after its oversized-payload case.",
                                          "failureAndCancellation":  "Injects GlobalAlloc failure, busy clipboard, EmptyClipboard failure, publication failure and embedded NUL; verifies cut leaves source text and prior clipboard contents intact until successful publication.",
                                          "testCoverage":  "Registered tests cover large Unicode round trips at 65,535/65,536/65,537/100,000 units, native Grid/TextField copy/paste/cut, failure preservation, oversized native clipboard rejection at embedded text-service boundary, and a benchmark case with allocation-count assertion plus timing output."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
                      "testIds":  [
                                      "TestNativeClipboardTransportBenchmark",
                                      "TestNativeClipboardLargeSelections",
                                      "TestNativeClipboardFailuresPreserveText",
                                      "TestEmbeddedClipboardRejectsLargeNativePayload"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.DxUiFocusEventClient.h",
                      "sha256":  "402e777dba824f6129313bd2337c765fec33ecf040b8fad93d86819c29b8fcfd",
                      "reviewNotes":  {
                                          "callers":  "Included by DxUi.Tests.Menu.cpp and DxUi.Tests.WindowHost.cpp; observed consumers include TestWindowHostTabRaisesAutomationFocusChanges and TestWindowHostTreeArrowAnnouncesTheFocusedItem.",
                                          "ownershipAndLifetime":  "FocusNameRecorder is WRL/FTM COM with mutex-protected event names; client owns events, recorder and last-declared jthread so thread joins before state destruction. Focus callback reads cached name/process only.",
                                          "failureAndCancellation":  "UIA connection/transaction timeouts bound provider calls and setup/request waits; setup stage and HRESULT are logged; focus event names are filtered to the current process. Destructor pumps while awaiting thread exit, but a failure after its deadline would still encounter jthread join semantics.",
                                          "testCoverage":  "Observed tests cover native host tab-focus announcement and tree focused-item announcement; other direct callsites are in Menu/WindowHost suites. No independent stress test of UIA client teardown under a permanently blocked COM provider."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
                      "testIds":  [
                                      "TestWindowHostTabRaisesAutomationFocusChanges",
                                      "TestWindowHostTreeArrowAnnouncesTheFocusedItem"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.DxUiTestHelpers.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.DxUiTestMovedControls.h",
                      "sha256":  "7ed7e2f8f227de220fdc97559b5d5ed9a616093ddfa2e4fdd89a7a71950ed6b2",
                      "reviewNotes":  {
                                          "callers":  "Used by DxUi.Tests.EditorControls.cpp move/fresh comparisons for ColorPicker, NumericStepper, TabControl, MenuBar and TextField.",
                                          "ownershipAndLifetime":  "Each scenario owns two AttachedHostWindow hosts and control trees; helper extracts a child, transfers unique_ptr to second host, compares its geometry and BGRA pixels against a freshly configured control. Host/device lifetime remains local to scenario.",
                                          "failureAndCancellation":  "Device-loss scenario hides the first host and requires second Direct2D device identity to differ; every scenario requires moved control to paint and geometry/pixels equal the fresh peer, reporting pixel/rectangle differences.",
                                          "testCoverage":  "Actual callers: TestColorPickerMovedBetweenHostsMatchesAFreshOne, TestNumericStepperMovedBetweenHostsMatchesAFreshOne, TestTabControlMovedBetweenHostsMatchesAFreshOne, TestMenuBarMovedBetweenHostsMatchesAFreshOne, TestTextFieldMovedBetweenHostsMatchesAFreshOne. The helper extracts via moving GetChildren()[index] directly instead of Panel::TakeChild; this is a test-fixture caveat because it bypasses the public extraction callbacks/inheritance contract."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Test-fixture extraction caveat: helper bypasses Panel::TakeChild; route to test owner for review.",
                      "testIds":  [
                                      "TestColorPickerMovedBetweenHostsMatchesAFreshOne",
                                      "TestNumericStepperMovedBetweenHostsMatchesAFreshOne",
                                      "TestTabControlMovedBetweenHostsMatchesAFreshOne",
                                      "TestMenuBarMovedBetweenHostsMatchesAFreshOne",
                                      "TestTextFieldMovedBetweenHostsMatchesAFreshOne"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.GridMultilineFixtures.h",
                      "sha256":  "615f3d851e354d68d66085a6168e81cd860e63e60929cfc239ab65230efdabba",
                      "reviewNotes":  {
                                          "callers":  "Included by DxUi.Tests.Grid.cpp, DxUi.Tests.Rendering.cpp and Controls.Tests.GridMultilineRendering.h; consumers use TextTableModel, CaptureWindow, ConfigureHostPlace, PixelRect and RequireIdentical.",
                                          "ownershipAndLifetime":  "TextTableModel owns its nested cell strings and width vector; captures and derived difference records are values. WIL/host ownership is delegated to caller fixtures.",
                                          "failureAndCancellation":  "TextTableModel uses bounds-checked vector access and exposes stable row lookup; pixel utilities treat out-of-capture coordinates as transparent zero and RequireIdentical first checks capture dimensions.",
                                          "testCoverage":  "This header is fixture/utility only and registers no test ID; caller test registration is in Grid and Rendering suites (full rendering caller review is pending). Tests relying on these helpers assert multiline text geometry/pixel output."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Fixture/policy input reviewed; direct test ID absent or caller review remains pending as noted.",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.GridMultilineRendering.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.LocalizedLayoutFixture.h",
                      "sha256":  "f14e4e4dafd54df43b5207290df160c2117d94d04dba4b9f0df99d75cdf285ad",
                      "reviewNotes":  {
                                          "callers":  "Used by DxUi.Tests.Controls.cpp TestLocalizedActionLayout.",
                                          "ownershipAndLifetime":  "Fixture owns DirectWrite factory/format/layout COM pointers through WIL and uses fixed local arrays for scenarios/rectangles; arrange strategy is injected by caller.",
                                          "failureAndCancellation":  "Every DirectWrite creation, wrapping setting and metrics call is required to succeed; final assertion verifies all measured French action rectangles fit, preserve minimum sizes/order and do not overlap.",
                                          "testCoverage":  "TestLocalizedActionLayout executes 24 width/scale combinations, 2,000 repeats per workload across five timed rounds, then validates all layout geometry. Timing is diagnostic output; no threshold qualifies performance."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found during source-backed test-fixture review; limitations are stated in notes.",
                      "testIds":  [
                                      "TestLocalizedActionLayout"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextInputServices.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextStoreComposition.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.ControlTests.vcxproj",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Animation.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.ComboBox.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Controls.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.EditorControls.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Gallery.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Grid.cpp",
                      "sha256":  "5b5e1c00c853d073f343132d9c5534a3359c1317afea2c62a8f9fb003d105f6f",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "RunGridTests",
                                      "DxUi.Tests.Grid suite"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.InteractiveLease.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Menu.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.MenuResources.h",
                      "sha256":  "8c025ee902b6caf5b6af1dfbde2ce7997ff916346351fd4fa82fb9b7eab33c48",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "RunMenuResourceTests",
                                      "RunMenuResourceScalingTests",
                                      "RunMenuTextLayoutResourceTests"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.MultilineText.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.NewControls.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.ReadOnly.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Rendering.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Runner.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.TextField.cpp",
                      "sha256":  "f8fea659ccb3ad32ae1a5f9e6cc7db8a0b9975d85d2e0a1f3499f822246c7723",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "RunTextFieldTests",
                                      "DxUi.Tests.TextField suite"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Theme.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Tooltip.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/DxUi.Tests.Tree.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.FormAccessibility.cpp",
                      "sha256":  "968a1bd376182b4a669fee0d4e34c89895bd4843849adcb48b67891c49cf9a4a",
                      "reviewNotes":  {
                                          "callers":  "Public test entrypoints are declared in Controls.Tests.FormAccessibility.h and called by control test registration; embedded variants use the application EmbeddedUiaTest::Bridge.",
                                          "ownershipAndLifetime":  "FormFixture owns window/view/bridge and root tree; COM providers use wil::com_ptr_nothrow, embedded cases run on a dedicated STA jthread with activation blocker, and event hooks/subscriptions are cleared via scope guards/destructors.",
                                          "failureAndCancellation":  "Validates malformed validation states/text bounds are rejected without replacing last good UIA snapshot; weak external label lifetime/replacement/detach retires providers; destructive synchronous notification reentrancy stops later events; detached retained editor actions return UIA_E_ELEMENTNOTAVAILABLE.",
                                          "testCoverage":  "Window and embedded cases assert labeled semantic editor metadata, required/valid/help/status, reflow-stable identity and draft/selection, label hide/detach/replacement expiry, event idempotence, and root/editor/retained-editor/hidden-field destructive callback ordering. Embedded queue-time assertions allow at most the already attempted event; not a proof for arbitrary asynchronous timing."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.FormAccessibility.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.Forms.cpp",
                      "sha256":  "603e19ac3d8f3eaf83b28a5a1aecf88413f97ac3e3919c801092bef84e5071ab",
                      "reviewNotes":  {
                                          "callers":  "RunFormLayoutTests is registered by the control runner; public FormPanel/FormField, ControlHost and EmbeddedHost APIs are exercised.",
                                          "ownershipAndLifetime":  "Unique_ptr control trees and GraphicsFixture/GraphicsDevice RAII own resources. Embedded input fixtures stay on-thread; callbacks use external state and lifetime tokens for retirement.",
                                          "failureAndCancellation":  "Tests invalid policies/dimensions, shaping budgets, arithmetic overflow, nonconvergent minima, failed measurement, and reentrant detach/clear/reparent/focus/bounds changes; prior or newer-authored geometry must survive.",
                                          "testCoverage":  "Localized DPI/density/RTL/columns, intrinsic minima, fractional widths, editor/part ownership, reentrant layout, embedded composition/selection/focus/cancel, native preparation. Runtime execution not claimed."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
                      "testIds":  [
                                      "TestFormsMeasureIsPureAndSharedLabelCacheIsStable",
                                      "TestFormsLocalizedGeometryMatrix",
                                      "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
                                      "TestFormsFractionalTranslationFailsBeforeAnyChildGeometry",
                                      "TestFormsIntrinsicEditorMinimaDriveNarrowContentAndCache",
                                      "TestFormsMeasurementCallbacksRetireOrSupersedeSafely",
                                      "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
                                      "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus",
                                      "TestFormsNativePreparationAndFactoryCoverage"
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.Forms.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.GridUiaPattern.cpp",
                      "sha256":  "2cea15a1752dde483401d54fd586cab9ef42a11a0b628cf7b48eac4a5bb99e14",
                      "reviewNotes":  {
                                          "callers":  "Grid UIA test entrypoints are registered by the control runner; they acquire WindowHost UIA root and call COM Grid/GridItem/Table/ScrollItem providers, including a foreign UIA client apartment.",
                                          "ownershipAndLifetime":  "Grid model, AttachedHostWindow, providers, events and worker threads are fixture-owned; COM pointers/SafeArrays use WIL/scope guards. Foreign client is bounded while owner pumps the hidden nonactivating window; cleanup releases gate and joins.",
                                          "failureAndCancellation":  "Injects row shrink, same-address reassignment, replacement/notification callbacks during reads, standard exceptions and bad_alloc at COM boundary, stale-row deletion and first-publication failure. Stale providers return UIA_E_ELEMENTNOTAVAILABLE; exception outputs remain null.",
                                          "testCoverage":  "Covers offscreen GetItem bounds/read budget, foreign-client access, cache eviction, navigation/runtime IDs, model revision races/retry, error recovery, deleted/unreadable rows, display-order headers, WM_GETOBJECT, scroll-into-view and focus of scrolled-out columns. No stress/fuzzing claim."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.GridUiaPattern.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.HostControlRegressions.cpp",
                      "sha256":  "f3b2765a59ff7f3bd153d389a8a106e10130c9da5c713d5793bcf5edb2498227",
                      "reviewNotes":  {
                                          "callers":  "Public Test* entrypoints are declared in Controls.Tests.HostControlRegressions.h and registered in the control test runner; UIA cases use the shared UiaTest client and embedded bridge.",
                                          "ownershipAndLifetime":  "WARP GraphicsFixture and GraphicsDevice pools are local RAII fixtures; embedded UIA runs on dedicated STA with activation blocker; WindowHost and AttachedHostWindow own their control trees/windows. Callback captures remain alive through synchronous message dispatch.",
                                          "failureAndCancellation":  "Exercises current logical PageHost membership and retired providers, hidden/disabled focus traversal, scroll transform input, multi-button drag/capture, scrollbar repeated paging, keyboard-context viewport clamping, app key fallback filtering, extracted RadioButton independence, and borrowed graphics bindings after ReplaceDevice/Detach.",
                                          "testCoverage":  "Focused regression tests cover page-tree UIA and invoke lifetime in both hosts, tab traversal, animation clock epochs including zero, caret invalidation only on phase changes, scroll context/menu anchors, drag/capture and focus preservation, fallback key filtering, radio extraction, and application graphics-state preservation; no asynchronous race/stress coverage asserted here."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.HostControlRegressions.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.IntrinsicLayout.cpp",
                      "sha256":  "811a81402812b094fb7f3d62121e2cbca0b0be15ee4d73c6e993e41dbb6c585d",
                      "reviewNotes":  {
                                          "callers":  "RunIntrinsicLayoutTests is registered by the control runner; fixtures use public controls, FlexPanel/FormPanel, WindowHost/EmbeddedHost and visual-preparation hooks.",
                                          "ownershipAndLifetime":  "Control trees and GraphicsFixture/GraphicsDevice resources use RAII; STA threads isolate WARP host fixtures, activation blockers prevent focus, and callback state outlives hosts. Thread completion has a 30-second bound before join.",
                                          "failureAndCancellation":  "Validates atomic outputs for invalid constraints, named/allocation exceptions, bounded shaping, explicit fallback, aggregate/arithmetic overflow, nonconvergent minima, reentrant retirement/reparent/focus/layout, and visual-cancellation/device-replacement/hide-show wakes.",
                                          "testCoverage":  "34 registered cases cover text/editor metrics, typography/rendering parity and cache, device recreation, callback cancellation, flex wrapping/RTL/overflow/rounding/ownership, native/embedded preparation, draft/composition/focus and stale-press cancellation. WARP parity checks do not establish a performance speedup."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this test file during source-backed review; coverage limits are stated in notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.IntrinsicLayout.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.MenuPopupReadiness.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.MenuPopupReadiness.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.NativeDevicePolicy.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextElementMovement.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextElementMovement.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextElements.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.TextElements.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.UnicodeGraphemeCorpus.h",
                      "sha256":  "55ed4eff16a837bdc22e4390ddc67ec3e29fe6e22d50c8a61ab3915f3b1c52f4",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "Unicode grapheme corpus consumers",
                                      "Test-UnicodeGraphemeData.ps1"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/DxUi.EmbeddedTests.vcxproj",
                      "sha256":  "fbbcba65d43ab27cb63dd762223355d2b335c314922846fdc3f6ba0e9e1f4927",
                      "reviewNotes":  {
                                          "callers":  "Standalone Embedded test executable referenced by solution/Tools native suite planning; one translation unit Embedded.Tests.Embedded.cpp; references src/DxUi.vcxproj.",
                                          "ownershipAndLifetime":  "Build input only; Debug/Release/ASan x64 and ARM64 configurations select v145 and Windows SDK 10.0.26100.0. /bigobj applies to the large harness translation unit.",
                                          "failureAndCancellation":  "Build fails through MSBuild project reference/compiler/linker errors; no runtime cancellation or postbuild copies declared.",
                                          "testCoverage":  "Embedded.Tests.Embedded.cpp registers function and retained-control scenarios; coverage is the standalone Embedded suite across declared platforms/configurations."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No project wiring defect observed.",
                      "testIds":  [
                                      "Embedded suite"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.BenchmarkMain.h",
                      "sha256":  "ec05a1004c8968625d2b6a4d8312d9fc6da4877aa308d94e57f6bc52b3d482b2",
                      "reviewNotes":  {
                                          "callers":  "Included by Embedded.Tests.Embedded.cpp as wmain; routes benchmark flags to ComplexUiBenchmark::Run and default invocation to RunFunctionalTests.",
                                          "ownershipAndLifetime":  "Entry point initializes STA COM and uses WIL scope_exit for CoUninitialize; benchmark retention mode emits 60 separate output cycles.",
                                          "failureAndCancellation":  "Unknown flags fall through to functional tests rather than returning usage failure; only fixed argv shapes select benchmark modes.",
                                          "testCoverage":  "CLI branches cover baseline, multiline distinct/retention/heap/paced variants and functional test dispatch; this header is an entrypoint, not per-scenario assertion source."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete issue found; unknown-argument fallthrough is a minor command-surface ambiguity.",
                      "testIds":  [
                                      "--benchmark",
                                      "--benchmark-multiline-grid",
                                      "--benchmark-retention",
                                      "--benchmark-multiline-grid-heap-paced",
                                      "RunFunctionalTests"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.ComplexUiBenchmark.h",
                      "sha256":  "4126fdce2b80e8294a1ed880f963ec67d25546a1b321f6a1fd0c282b64c967f9",
                      "reviewNotes":  {
                                          "callers":  "BenchmarkMain dispatches each CLI workload here; uses Samples/ComplexUi scene, GraphicsFixture WARP device, shared heap diagnostic and allocation counters.",
                                          "ownershipAndLifetime":  "Function-local scene/device/staging resources use RAII; output stream owns JSON report. Timed clean/dirty loops count same-thread C++ allocations and sample private/working-set memory; optional retention mode repeats 6000 frames and samples every 200.",
                                          "failureAndCancellation":  "HRESULT/check failures abort benchmark; report file state is checked at open and after close. Fixed process heap diagnostics are opt-in and explicitly reject partial heap accounting.",
                                          "testCoverage":  "Scenarios assert no C++ allocations in clean/composition, dirty allocation ceilings, preparation deltas, hidden-view zero work, surface reuse, and retained resource samples; not a product perf comparison by itself."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No cause attribution from this harness alone; capture validates its own workload invariants and emits raw samples.",
                      "testIds":  [
                                      "--benchmark",
                                      "--benchmark-multiline-grid",
                                      "--benchmark-retention",
                                      "--benchmark-multiline-grid-heap",
                                      "--benchmark-multiline-grid-heap-paced"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.Embedded.cpp",
                      "sha256":  "e2aec932acecf38c55f8cff046fc779f4dcfefbb4afc476bc9f33ef9fcf717d0",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "RunFunctionalTests",
                                      "Embedded functional suite"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.EmbeddedAccessibility.h",
                      "sha256":  "d3fb68121e9aadc3cd899825949d4540336596f75a905ceb423f493415b661fe",
                      "reviewNotes":  {
                                          "callers":  "Included by Embedded.Tests.Embedded.cpp; top-level TestEmbeddedAccessibility invokes disclosure, offscreen Grid GetItem, multi-select Tree, throwing model, and core embedded UIA scenarios.",
                                          "ownershipAndLifetime":  "EmbeddedScene owns view/device-bound controls; models and delegates outlive borrowed pointers; site root is cleared before detach. Foreign-client std::thread is joined; COM initialized/uninitialized in worker. Two returned SAFEARRAYs are manually SafeArrayDestroy-ed.",
                                          "failureAndCancellation":  "Covers invalid placement/value, foreign-thread rejection, COM-marshaled action pumping, stale providers after hide/detach/root replacement, same-path control replacement, and throwing model getter recovery. Assertion failures use Check/Hr.",
                                          "testCoverage":  "TestEmbeddedDisclosureAccessibility, TestEmbeddedGridOffscreenGetItem, TestEmbeddedTreeMultiSelect, TestEmbeddedAccessibilityContainsAThrowingModelGetter, and TestEmbeddedAccessibility are called by embedded functional suite."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [
                                         "F-EMBED-ACCESSIBILITY-SAFEARRAY-01"
                                     ],
                      "disposition":  "Actionable cleanup finding: returned SAFEARRAY values at selection readback and text rectangles use manual SafeArrayDestroy despite neighboring WIL SAFEARRAY RAII pattern; replace with unique_any in focused cleanup.",
                      "testIds":  [
                                      "TestEmbeddedDisclosureAccessibility",
                                      "TestEmbeddedGridOffscreenGetItem",
                                      "TestEmbeddedTreeMultiSelect",
                                      "TestEmbeddedAccessibilityContainsAThrowingModelGetter",
                                      "TestEmbeddedAccessibility"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.EmbeddedUiaBridge.h",
                      "sha256":  "183aff6d375d32f848c9cb9d5960790c6800e08fdce44b6f40c11cbd5171865c",
                      "reviewNotes":  {
                                          "callers":  "Bridge is consumed by Embedded.Tests.EmbeddedUia.h and Embedded.Tests.PreparedTreeEvents.h to adapt EmbeddedHost providers into a real application WM_GETOBJECT tree.",
                                          "ownershipAndLifetime":  "Bridge owns HWND and WRL application provider with WIL/wil::com_ptr; ApplicationSite borrows ApplicationElement with bridge lifetime. Destructor disconnects view/site, clears root, disconnects UIA provider, then destroys HWND.",
                                          "failureAndCancellation":  "Attach validates root/attachment; event pumping is bounded at 4096 messages per call. Provider uses COM threading and fixture clients pump owner STA; stale/detached provider behavior is asserted by consumers.",
                                          "testCoverage":  "TestEmbeddedUiaEventHarness and prepared-tree event fixtures exercise application-parent/fragment-root bridge and marshaled UIA calls; UiaTestClient owns subscription teardown before bridge destruction."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; ownership ordering is explicit and consumer harness exercises host bridge.",
                      "testIds":  [
                                      "TestEmbeddedUiaEventHarness",
                                      "PreparedTreeEvents scenarios"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.GridSelectionBenchmark.h",
                      "sha256":  "692201c512d8285ccf29f50a7b7041fa38aad9e371b46d1d2ae766cbead69a26",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp routes --benchmark-grid-selection \u003creport\u003e [parts] to GridSelectionBenchmark::Run; uses public GridSelectionModel/Grid APIs and fixture-provided allocation counters.",
                                          "ownershipAndLifetime":  "Benchmark owns synthetic row model, embedded view, WARP completion texture, query vectors and output stream; view precedes borrowed model destruction. Allocation hooks count thread-local plain C++ requests and documented live bytes, excluding aligned/COM/driver storage.",
                                          "failureAndCancellation":  "Invalid part names fail before measurement; fixed nonzero row sizes avoid empty-vector indexing; HRESULT/check failures abort. Report is checked after close; no retry/cancel path.",
                                          "testCoverage":  "Opt-in paint, selection-cost, membership, retention, mutator, preserve and default-scene diagnostics exercise several sizes, stable-id shapes and select/clear/preserve lifecycles; not implicitly run as functional suite."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete issue found; diagnostic reports carefully disclose metric boundaries and does not establish product-level performance by itself.",
                      "testIds":  [
                                      "--benchmark-grid-selection",
                                      "paint",
                                      "selectionCost",
                                      "membership",
                                      "retention",
                                      "mutators",
                                      "preserve",
                                      "complexUiScene"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.InteractionLayout.h",
                      "sha256":  "114a90815c9fd3650e247be5e838ac1c252f5f844e6e4e9d885bc99c37d35969",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp invokes TestInteractionLayoutWithoutPaint(gpu); scenarios use EmbeddedHost input callback to probe interaction layout after layout changes.",
                                          "ownershipAndLifetime":  "Each test owns EmbeddedScene, controls, and host; model stays alive for borrowed view references. Input callbacks may mutate the tree, and test-owned callbacks remain scoped to synchronous input delivery.",
                                          "failureAndCancellation":  "Stale layout, wrong extent/DPI/device, failed paint, hidden/zero-size view, wrong-thread call, and callback-driven tree mutation fail closed or return retry; cancellation restores slider state.",
                                          "testCoverage":  "TestInteractionLayoutWithoutPaint asserts interaction acknowledgment does not publish UIA/text geometry; cases exercise relocated clicks, hide, slider cancel, splitter capture, reentrant tree mutation, and invalidation guards."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; test guards input-only layout publication and reentrant mutation.",
                      "testIds":  [
                                      "TestInteractionLayoutWithoutPaint"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.LocalizedLayout.h",
                      "sha256":  "6c21102d15b0826d7776b101c5aab2d7d95dfdb491145d501badec4ee2228922",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp invokes localized short-viewport, invalidation/selected-detail, stacked-body clipping, checkbox captions, RTL checkbox and RTL tab title cases.",
                                          "ownershipAndLifetime":  "Each scenario owns EmbeddedScene/control tree; borrowed ComplexUiModel remains in scope longer than scene/view. Captures use fixture-owned GPU resources and local vectors.",
                                          "failureAndCancellation":  "Hr/Check fail test on preparation, geometry, paint, UIA action, focus, or callback mismatch; each view detaches before borrowed model leaves scope.",
                                          "testCoverage":  "Tests cover French wrapped/long labels, viewport/DPI/width reflow, selection/focus, hit testing, virtualization, clean composition allocation counts, clipping/UIA hit targets, and RTL raster placement."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; tests exercise actual retained drawing and hit geometry across DPI/viewport variants.",
                      "testIds":  [
                                      "TestLocalizedShortViewport",
                                      "TestLocalizedInvalidationAndSelectedDetail",
                                      "TestLocalizedStackedBodyClipping",
                                      "TestLocalizedCheckboxCaption",
                                      "TestRightToLeftCheckboxCaptionHugsIndicator",
                                      "TestRightToLeftTabTitleStartsAtTheRight"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.ComplexUiBoundaries.h",
                      "sha256":  "f217c8c6864556b2f6a07c2fb08fef81942a6275a8722d045041e4bfdc031480",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp invokes TestDevicePolicies and dispatches optional `--complex-ui-boundaries` diagnostics; uses ComplexUiProfiling counters and the synthetic WARP scene.",
                                          "ownershipAndLifetime":  "Eight Window probes are allocated once outside timing; fixed per-frame arrays retain QPC samples; D3D staging texture uses WIL COM ownership. Output JSON is written after timed probes.",
                                          "failureAndCancellation":  "Invalid raster mode is rejected before device creation; HRESULT/check failures abort. Complete flag captures thread/process CPU query availability, and output labels diagnostic-only/non-GPU timing scope.",
                                          "testCoverage":  "TestDevicePolicies checks modes, actual device flags, WARP output pixels; diagnostic run checks clean preparation/composition invariants, allocation-free probes, hidden surface release, and writes window samples."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No acceptance claim or concrete defect; standalone diagnostic explicitly disclaims scheduler/GPU attribution.",
                      "testIds":  [
                                      "TestDevicePolicies",
                                      "--complex-ui-boundaries",
                                      "boundary clean probes"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.ComplexUiProfiling.h",
                      "sha256":  "beee3e4c80fb9a6fc85c2d65f699405a82cd71838cdf7c07a5c6e0d3b285f61c",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp routes --benchmark-complex-ui-profile \u003cout\u003e Default|MultilineGrid Off|On to this diagnostic; profiling wrapper records stage and process/thread observations around the synthetic workload.",
                                          "ownershipAndLifetime":  "Fixed-size stage/thread arrays and bounded counters are owned by the profiling context; sampling uses Win32 process/thread/heap queries and emits labeled diagnostic samples. It is opt-in and does not alter product code paths.",
                                          "failureAndCancellation":  "Unavailable CPU/thread/process counters are represented as unavailable; invalid invocation is rejected by the CLI router. Profile output is diagnostic and bounded by wrapper timeout; no cancellation/acceptance contract.",
                                          "testCoverage":  "The embedded runner dispatches the profile command and workload variants; profile output is diagnostic only, not a paired acceptance receipt or proof of an allocation cause."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; keep diagnostic and acceptance evidence separate.",
                      "testIds":  [
                                      "--benchmark-complex-ui-profile",
                                      "Default profile scenario",
                                      "MultilineGrid profile scenario"
                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.FormLayoutBenchmark.h",
                      "sha256":  "78af68623bad96ff47cf8a02839d9b787ce9f954de3f0db3ed6f5c95906594f2",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.Embedded.cpp routes --benchmark-form-layout \u003creport\u003e [seed] [blocks] to FormLayoutBenchmark::Run; compares public FormPanel layout with a manual leaf-layout baseline using public APIs.",
                                          "ownershipAndLifetime":  "Fixture owns EmbeddedHost, root/control hierarchy, measurement caches and completion texture; borrowed model state and control lifetime tokens are scoped to the fixture. Output and graphics resources use RAII; per-frame allocation counters are switched off by scope_exit.",
                                          "failureAndCancellation":  "Seed/block parsing is bounded and profile parity is checked before timing; hidden/pending composition, device-generation transitions, output stream completion, and target pixel completion are checked. No catch/retry; mismatch aborts diagnostic.",
                                          "testCoverage":  "Pre-timing tests compare hierarchy, semantics, metadata, geometry and populated pixels across LTR/RTL, DPI/density, validation and width states; timing/lifecycle tracks warmup, unchanged, width, validation, hide/show, device replacement, root retirement and detach."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; diagnostic labels timing/allocation scopes and explicitly excludes physical removed-device qualification.",
                      "testIds":  [
                                      "--benchmark-form-layout",
                                      "CheckParity",
                                      "CheckPreflightGuards",
                                      "Lifecycle"
                                  ]
                  },
                  {
                      "path":  "Tests/Foundation/DxUi.FoundationTests.vcxproj",
                      "sha256":  "060c899500e304f2e5bee1514be8e66ace3f62a8d8c67d19c039be2e7890f3f7",
                      "reviewNotes":  {
                                          "callers":  "Foundation test project is included in the solution/native suite; compiles Foundation.Tests.Foundation.cpp and Foundation.Tests.PublicHelpers.cpp and references src/DxUi.vcxproj.",
                                          "ownershipAndLifetime":  "Build metadata only; declares Debug/Release/ASan for x64 and ARM64 with v145 and Windows SDK 10.0.26100.",
                                          "failureAndCancellation":  "Compiler/link failures fail the test target; configuration list does not add a separate runtime timeout or cancellation mechanism.",
                                          "testCoverage":  "Builds the standalone Foundation runner for each declared profile; behavioral assertions reside in the two listed translation units."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No project wiring defect observed.",
                      "testIds":  [
                                      "Foundation test executable"
                                  ]
                  },
                  {
                      "path":  "Tests/Foundation/Foundation.Tests.Foundation.cpp",
                      "sha256":  "2fb3abda3ff1305177b0e3262690910cc2e5528f4012933b99c729d27923656d",
                      "reviewNotes":  {
                                          "callers":  "Native suite launches Foundation test executable; main routes hidden CRT/ASAN child probes then runs public-helper, frame clock, motion policy, frame stage, and metrics assertions.",
                                          "ownershipAndLifetime":  "Runner owns process-level CRT report routing and test invocation; hidden child mode is used to verify ASAN reporting without crashing the main suite.",
                                          "failureAndCancellation":  "Check aggregates assertion failures into nonzero exit; hidden ASAN child failure is isolated and parent validates expected signal/diagnostic.",
                                          "testCoverage":  "TestPublicHelpers and frame/motion metric checks run in the normal entrypoint; hidden ASAN child test exercises sanitizer boundary."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; exact assertion IDs for inline policy checks are not separately registered.",
                      "testIds":  [
                                      "TestPublicHelpers",
                                      "frame clock policy assertions",
                                      "motion policy assertions",
                                      "frame stage and metrics assertions",
                                      "hidden ASAN child probe"
                                  ]
                  },
                  {
                      "path":  "Tests/Foundation/Foundation.Tests.PublicHelpers.cpp",
                      "sha256":  "e5812bda1c73aaa0afc2bca0277af3871e1fdf48bfea9aa524e4da95432cc82d",
                      "reviewNotes":  {
                                          "callers":  "Foundation.Tests.Foundation.cpp calls TestPublicHelpers; tests compile/link public helper imports and use isolated DirectWrite factories for font cache scenarios.",
                                          "ownershipAndLifetime":  "Test-only menu host is noncopyable; DirectWrite factories and font handles use COM/WIL ownership. Internal cache access is limited to simulating a negative cache result.",
                                          "failureAndCancellation":  "Assertions fail the public helper aggregate; factory creation and font lookup failures are reported, and cache invalidation test checks results after factory isolation.",
                                          "testCoverage":  "TestPublicHelpers covers public consumer helper APIs and font availability cache invalidation across isolated factories."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; internal cache hook remains confined to test code.",
                      "testIds":  [
                                      "TestPublicHelpers",
                                      "font availability cache invalidation"
                                  ]
                  },
                  {
                      "path":  "Tests/InteractiveLease/DxUi.InteractiveLease.vcxproj",
                      "sha256":  "971abec0fab1092c3243a294df676e1049c70c92d5a8739895399a47e56ced7e",
                      "reviewNotes":  {
                                          "callers":  "InteractiveLease native runner is built by the solution and tooling suite; includes runner plus confirmation/banner fixtures and references DxUi without adding another native link dependency.",
                                          "ownershipAndLifetime":  "Project wiring only; native executable owns its helper process and UI fixtures.",
                                          "failureAndCancellation":  "Build errors fail project; runtime admission/timeout behavior is exercised by runner and scripts, not MSBuild.",
                                          "testCoverage":  "Build target compiles InteractiveLease.Tests.Runner.cpp and referenced fixture headers for declared platform/configuration profiles."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No project wiring defect observed.",
                      "testIds":  [
                                      "InteractiveLease native executable"
                                  ]
                  },
                  {
                      "path":  "Tests/InteractiveLease/InteractiveLease.Tests.ChildProcess.h",
                      "sha256":  "9d8c6d908b8f671ba43a546c453adfb1cef14b01a395ff8bee8132e1796206c4",
                      "reviewNotes":  {
                                          "callers":  "RunChildProcess is called by Win32LeaseServices::RunChild and SelfTest::ChildProcesses.",
                                          "ownershipAndLifetime":  "Owns handles, suspended child and optional kill-on-close job; direct-process cleanup is scoped.",
                                          "failureAndCancellation":  "LEASE-JOB-001: job setup failure closes the job but still resumes an uncontained child; interactive lease descendant containment is lost.",
                                          "testCoverage":  "SelfTest covers success, timeout, interrupt, missing image and log failure; injected job-stage and descendant lifecycle coverage are absent."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [
                                         "LEASE-JOB-001"
                                     ],
                      "disposition":  "confirmed-contract-gap",
                      "testIds":  [
                                      "SelfTest::ChildProcesses",
                                      "DxUi.InteractiveLease.exe --self-test"
                                  ]
                  },
                  {
                      "path":  "Tests/InteractiveLease/InteractiveLease.Tests.Confirmation.h",
                      "sha256":  "f0b8570fea64a5feafd27eaedc668b5c3866ba8121fabdd03126f04bef89ce2a",
                      "reviewNotes":  {
                                          "callers":  "InteractiveLease.Tests.Runner.cpp PrivateDesktop() calls the confirmation flow with scripted and native TaskDialog scenarios; lease orchestration supplies the confirmation service.",
                                          "ownershipAndLifetime":  "TaskDialog callback state is stack/context-owned for synchronous dialog lifetime; callback tracks cancellation, interruption, countdown, and scripted decision without retaining caller references.",
                                          "failureAndCancellation":  "Default decision is Cancel unless approved auto-start applies; silent mode avoids sound; dialog unavailability is reported as a skipped hosted scenario. Footer formatting catches bad_alloc locally to preserve dialog fallback.",
                                          "testCoverage":  "PrivateDesktop runner exercises Start/Cancel, timeout, interruption, auto-start, close, and clicked Start outcomes; hosted suite is gated on private desktop availability."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; hosted tests correctly disclose private desktop availability as a gate.",
                      "testIds":  [
                                      "InteractiveLease.Tests.Runner.cpp::PrivateDesktop confirmation scenarios"
                                  ]
                  },
                  {
                      "path":  "Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp",
                      "sha256":  "ea5693c4680fae44210ca288585290ad887f394546155cd308f47edd9ea32165",
                      "reviewNotes":  {
                                          "callers":  "Executable wmain routes --check, --run, --run-hosted and --self-test; Run delegates to Support.Tests.InteractiveLease::RunLease and Win32LeaseServices delegates to ChildProcess/Confirmation/WarningBanner helpers. SelfTest::Run invokes ChildProcesses, DescendantProcesses, hosted warning input, between-child recovery, session mutex, and private desktop scenarios.",
                                          "ownershipAndLifetime":  "Win32LeaseServices owns mutex, desktop probe/backend, banner and process-local interrupt/finished events; lease orchestration releases session mutex and restores the desktop before result serialization. SelfTest owns a PID-named temp directory and child-process fixtures; descendant cleanup verifies PID plus creation time before any failure cleanup.",
                                          "failureAndCancellation":  "CLI rejects unknown/missing arguments and gates hosted mode on exact environment markers before desktop inspection or result writes. Child tests cover job create/configure/assign refusal before Resume, success, timeout, interruption, missing executable, log-open failure, and contained descendant completion; dialog/warning tests may skip when private desktop unavailable. Console close waits at most 4.5s for lease restoration.",
                                          "testCoverage":  "SelfTest::ChildProcesses and ::DescendantProcesses assert nonzero/success, timeout/interruption, denied containment setup, child output, and both normal-exit and timeout descendant termination; ::PrivateDesktop covers confirmation outcomes and warning patch restoration, with explicit skip when desktop creation is unavailable."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No defect confirmed from source review; private desktop scenarios are environment-gated and must remain disclosed separately from headless self-test coverage.",
                      "testIds":  [
                                      "SelfTest::ChildProcesses",
                                      "SelfTest::DescendantProcesses",
                                      "SelfTest::HostedWarningInput",
                                      "SelfTest::WarningForegroundBetweenChildren",
                                      "SelfTest::SessionLease",
                                      "SelfTest::PrivateDesktop"
                                  ]
                  },
                  {
                      "path":  "Tests/InteractiveLease/InteractiveLease.Tests.WarningBanner.h",
                      "sha256":  "18323d806f669703cea8c0ff3fde377cd326549cc465e75a2db983b43587119e",
                      "reviewNotes":  {
                                          "callers":  "InteractiveLease.Tests.Runner.cpp PrivateDesktop() composes banner with activation patch and validates click-through and hosted synthetic input behavior.",
                                          "ownershipAndLifetime":  "Banner owns HWND, font, and region through WIL; hosted patch records original window styles/cursor state and restores on teardown; test callbacks are synchronous and scoped.",
                                          "failureAndCancellation":  "Banner creation and patch validation fail closed; click is sent down then up with retry/message pumping, and restoration paths run on cancellation/failure. Redraw allocation failure is explicitly caught for retry.",
                                          "testCoverage":  "PrivateDesktop scenarios exercise banner visibility, click-through, hosted patch ownership, cursor movement, input release and restoration; test may skip where private desktop unavailable."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; restoration and fail-closed handling are observable in the hosted integration harness.",
                      "testIds":  [
                                      "InteractiveLease.Tests.Runner.cpp::PrivateDesktop warning banner and hosted patch cases"
                                  ]
                  },
                  {
                      "path":  "Tests/native-test-files.json",
                      "sha256":  "fec8f0df8bf41bdea237b1c58803eaab5c9dc96fa952c23c11ae50c6c5971ca4",
                      "reviewNotes":  {
                                          "callers":  "Tools/ScopedTesting.psm1 Get-NativeSourceManifest reads this JSON and compares it with live native test inputs; Tests/README.md documents registration.",
                                          "ownershipAndLifetime":  "Tracked static input manifest; no runtime allocation or mutable object lifetime.",
                                          "failureAndCancellation":  "Missing/extra/duplicate declarations are rejected by manifest validation; policy has no cancellation state.",
                                          "testCoverage":  "Test-NativeScope.ps1 exercises manifest discovery and declaration validation; the manifest is configuration, not a test case."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed manifest and direct consumer. No concrete omission confirmed from the current manifest read.",
                      "testIds":  [
                                      "Test-NativeScope.ps1"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.ClipboardFixture.h",
                      "sha256":  "10184b263a4c988d37d17192017004c3b452e00778c01320962ccbacf2804e54",
                      "reviewNotes":  {
                                          "callers":  "Fixture installs clipboard test hooks and is consumed by native test cases; full caller list not yet verified.",
                                          "ownershipAndLifetime":  "Fixture owns simulated clipboard state and uses WIL global-memory handles; hook selection is thread_local. Underlying DxUi test hook pointers are shared process state, so concurrent use would require serialization.",
                                          "failureAndCancellation":  "Injected OS-operation failures flow through clipboard operations; RAII releases global memory at fixture teardown.",
                                          "testCoverage":  "Focused clipboard caller inventory remains to verify; no test IDs asserted in this header."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Fixture read complete; caller/test linkage remains a stated coverage gap.",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.CountingWriteFactory.h",
                      "sha256":  "a81960fd1d83e77515c1ad2e4893a3cbca220295ef123f9c4902ab402a1e2eef",
                      "reviewNotes":  {
                                          "callers":  "Support.Tests.PreparedFlowchart.h includes the wrapper; PreparedFlowchartTests::BudgetAndLifecycle and borrowed-target scenarios pass it through to PreparedFlowchart. RichTextLayout and Embedded suites invoke BudgetAndLifecycle.",
                                          "ownershipAndLifetime":  "WRL RuntimeClass implements IDWriteFactory and owns the wrapped factory through wil::com_ptr. It only forwards factory methods and counts CreateTextFormat/CreateTextLayout calls.",
                                          "failureAndCancellation":  "Injectable format HRESULT and failLayoutAt return failures through the actual SDK call surface, nulling output interfaces when applicable; no internal exception/cancel path.",
                                          "testCoverage":  "Controls.Tests.RichTextLayout.cpp and Embedded.Tests.Embedded.cpp exercise actual intercepted SDK calls and injected format/layout failures through PreparedFlowchart."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed; useful test seam with actual consumer coverage.",
                      "testIds":  [
                                      "PreparedFlowchartTests::BudgetAndLifecycle",
                                      "RichTextLayout",
                                      "Embedded"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.Footnotes.h",
                      "sha256":  "f131f2b60c54d8ce1c2166e023242925bf71e16c22ad550347188d592b044450",
                      "reviewNotes":  {
                                          "callers":  "Included and consumed by Controls.Tests.RichMarkdown.cpp, Controls.Tests.RichSemanticDocument.cpp, Controls.Tests.RichTextEditor.cpp, and Embedded.Tests.RichTextResources.h.",
                                          "ownershipAndLifetime":  "Inline fixtures describe immutable expected cases; parser completion advances with bounded work, and ExerciseNavigation/ExerciseSourceEdits accept editor/host callbacks without owning app state.",
                                          "failureAndCancellation":  "Cases cover missing/duplicate/cyclic/unreachable footnotes, Unicode normalization boundaries, parser containers, stale syntax after edits, read-only/disabled navigation, and undo/redo; checks fail through caller-provided assertion function.",
                                          "testCoverage":  "RichMarkdown parser tests, RichSemanticDocument source ownership/fallback tests, RichTextEditor source/navigation tests, and embedded source/navigation tests consume the shared corpus."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed shared semantic corpus and caller coverage; no defect confirmed.",
                      "testIds":  [
                                      "TestLosslessMarkdownFootnoteBlockGrammarAndProfileIsolation",
                                      "TestLosslessMarkdownFootnoteResolutionAndReachableNumbering",
                                      "TestLosslessMarkdownFootnoteBindingRevisionAndCancellation",
                                      "TestRichTextEditorNativeFootnoteResolutionAndSourceEdits",
                                      "TestEmbeddedFootnoteResolutionAndSourceEdits"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.DesktopLease.h",
                      "sha256":  "2da04048ea0b49ef97d6b0f4696423e30c99073dba5fda30dd5ec3ce18689a8c",
                      "reviewNotes":  {
                                          "callers":  "Support.Tests.InteractiveLease.h composes the lease; InteractiveLease runner calls CheckInteractiveDesktop for admission and teardown; Controls.Tests.InteractiveLease exercises fake-backend restoration and desktop availability.",
                                          "ownershipAndLifetime":  "DesktopLease stores a borrowed DesktopBackend reference and snapshot; restore has bounded retries, records exit state separately, and restores foreground then focus then physical pointer. Win32 backend uses WIL scoped ownership for process, desktop and input-attachment cleanup.",
                                          "failureAndCancellation":  "Window disappearance is reported Gone; inability to restore is Failed and focus is NotAttempted when foreground recovery fails. Focus query errors are distinguished from successful null focus. Admission rejects inaccessible/noninteractive desktops and unreadable pointer.",
                                          "testCoverage":  "Controls.Tests.InteractiveLease covers no-op, stale minimized focus, every null-focus path, failed readback, retries, gone windows, focus/cursor failure and admission reason ordering. Hosted InteractiveLease runner exercises confirmation/cancel/child completion integration."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No defect confirmed; fake-backend coverage is broad, with hosted interactive restoration separately gated.",
                      "testIds":  [
                                      "TestDesktopLeaseLeavesAnUntouchedDesktopAlone",
                                      "TestDesktopLeaseDoesNotCaptureStaleFocusFromMinimizedForeground",
                                      "TestDesktopLeaseCaptureDistinguishesEveryNullFocusPath",
                                      "TestDesktopLeaseRejectsFailedFocusReadback",
                                      "TestDesktopLeaseReportsAForegroundItCannotRestore",
                                      "TestInteractiveDesktopCheckNamesEachReasonToRefuse",
                                      "TestInteractiveDesktopCheckStopsAtTheFirstReason"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.FailureReports.h",
                      "sha256":  "a69833453838901bf2fe5061daf5cdf6e607afc80649a1cd351c7788ef5d6f9a",
                      "reviewNotes":  {
                                          "callers":  "Foundation.Tests.Foundation.cpp, Embedded.Tests.Embedded.cpp, InteractiveLease.Tests.Runner.cpp, and DxUi.Tests.Runner.cpp call RouteAwayFromDialogs during startup.",
                                          "ownershipAndLifetime":  "Process-global CRT report hook is installed once per test process; no owned heap/window resource.",
                                          "failureAndCancellation":  "Debug CRT assertions are routed away from modal UI; abort/report behavior still terminates failing test rather than hanging for user input.",
                                          "testCoverage":  "Startup registration is verified in each listed runner; no separate hook-specific assertion found."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No defect confirmed; process-global hook has runner-startup coverage, with no dedicated behavior test.",
                      "testIds":  [
                                      "Foundation.Tests.Foundation.cpp startup",
                                      "Embedded.Tests.Embedded.cpp startup",
                                      "InteractiveLease.Tests.Runner.cpp startup",
                                      "DxUi.Tests.Runner.cpp startup"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.ForegroundThief.h",
                      "sha256":  "a540fedce6feeb844e1c315e3196e6288759b9005bb35c4085662cc0908e6edf",
                      "reviewNotes":  {
                                          "callers":  "DxUi.Tests.Runner.cpp constructs optional ForegroundThief for --foreground-thief / --foreground-thief=\u003crange\u003e; menus and NativeTextInput tests observe foreground loss through window helpers.",
                                          "ownershipAndLifetime":  "A std::jthread owns its worker; destructor stop request and join precede destruction. The worker owns a tiny popup HWND through WIL RAII.",
                                          "failureAndCancellation":  "Window class registration/creation failures terminate the adversarial helper path; stop token and short message wait bound shutdown latency.",
                                          "testCoverage":  "Only explicit adversarial runner mode enables this. Normal no-activation suites avoid invoking it; runner parses range and collects outcomes."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed adversarial fixture and caller. It intentionally exercises focus theft only in opt-in scenarios.",
                      "testIds":  [
                                      "--foreground-thief runner option",
                                      "DxUi.Tests.Menu foreground-loss scenarios",
                                      "DxUi.Tests.NativeTextInput foreground-loss scenarios"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.HeapDiagnostic.h",
                      "sha256":  "c9c5e000fd2a5ab888ef730959d8e88fb166177a8667f077d1ae3e1d8d6afa64",
                      "reviewNotes":  {
                                          "callers":  "Embedded.Tests.ComplexUiBenchmark.h and Embedded.Tests.ComplexUiProfiling.h emit diagnostics; DxUi.Tests.MenuResources.h uses it for opt-in menu resource heap reports.",
                                          "ownershipAndLifetime":  "Uses fixed-size stack arrays for bounded heap/sample enumeration; WIL scope exit unlocks each heap if HeapLock succeeds.",
                                          "failureAndCancellation":  "GetProcessHeaps/HeapWalk/HeapUnlock failures are reported through supplied Check callback; exceeding fixed heap capacity fails closed instead of writing beyond storage.",
                                          "testCoverage":  "Benchmark/profiling paths and resource diagnostics call the helper; this is opt-in evidence, not runtime product behavior."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No defect confirmed; bounded diagnostic helper has explicit failure reporting.",
                      "testIds":  [
                                      "ComplexUiBenchmark profiling cases",
                                      "ComplexUiProfiling capture",
                                      "MenuResources heap diagnostic mode"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.InteractiveLease.h",
                      "sha256":  "9d7e0658830db619ad9edb3ab3a34d404de3ef5e0b2c47a228f126a9947bf3e4",
                      "reviewNotes":  {
                                          "callers":  "InteractiveLease.Tests.Runner.cpp implements LeaseServices and uses RunLease/ParsePlan/FormatLeaseResult; Tools/InteractiveRun.psm1 consumes its result contract and tests compare exit-code mappings.",
                                          "ownershipAndLifetime":  "RunLease borrows LeaseServices; it takes the session mutex only after desktop admission, uses scope guards to release the mutex, restore focus/pointer, hide warning, and undo keep-awake across all exits. Child outcomes are retained as value records.",
                                          "failureAndCancellation":  "Input parser rejects malformed/empty plans; declined, timed-out, interrupted, unavailable, launch-failed, child-failed, and restoration-failed outcomes map to explicit states/codes. Child cancellation is delegated to RunChild; no hidden retry.",
                                          "testCoverage":  "Controls.Tests.InteractiveLease covers event ordering and all service failure paths; InteractiveLease runner integration covers result serialization and native child process execution; Test-InteractiveMode checks PowerShell/native exit-code agreement."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed state machine and runner contract; no source-backed defect confirmed.",
                      "testIds":  [
                                      "TestLeaseConfirmationAndWarningOrdering",
                                      "TestLeaseRestoresAfterDeclinedConfirmation",
                                      "TestLeaseRestoresAfterChildFailure",
                                      "TestLeaseCaptureReasonSurvivesConfirmationAndChildren",
                                      "Test-InteractiveMode.ps1"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.PerformanceCapture.h",
                      "sha256":  "52f23d55215ed49afa57183bd9e49f425348ebef31d0c4bd387e6017ad7b9c4a",
                      "reviewNotes":  {
                                          "callers":  "DxUi.Tests.Runner.cpp installs the sink for optional --performance-jsonl diagnostic output and scope-exit clears it; Animation and WindowHost performance fixtures also start and stop capture around scenarios.",
                                          "ownershipAndLifetime":  "One inline std::ofstream is the process-local sink; Start truncates/creates target and installs global diagnostics/frame callbacks; Stop clears both callbacks then closes the stream.",
                                          "failureAndCancellation":  "Start throws on directory/file creation failure. Emit is noexcept and catches std::exception silently; ordinary output stream failbit is not checked per record. This affects optional ControlTests diagnostic output only.",
                                          "testCoverage":  "Runner and Animation/WindowHost perf paths consume capture; no focused test asserts serialization failure or dropped-write handling. Paired Embedded benchmark acceptance records are a separate path and do not depend on this sink."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Advisory: optional diagnostic JSONL may silently lose records after a write failure; no acceptance-evidence impact established.",
                      "testIds":  [
                                      "--performance-jsonl",
                                      "Animation performance capture",
                                      "WindowHost performance capture"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.TestWatchdog.h",
                      "sha256":  "a6fbc903c4f81febfa215ba65527f355a2e34b739937f502ee3e640a92c66892",
                      "reviewNotes":  {
                                          "callers":  "Controls.Tests.DxUiTestHelpers.h arms named cases and renews on progress; DxUi.Tests.Runner.cpp arms fixture-wide work and exposes hidden self-test mode; InteractiveLease runner also configures watchdog.",
                                          "ownershipAndLifetime":  "One intentionally process-lifetime leaked singleton protects against hangs during process exit; a single detached watchdog thread waits on a condition variable, with all armed state guarded by mutex and generation counter.",
                                          "failureAndCancellation":  "Timeout writes a bounded stderr line then terminates the owned test process with exit 124; nested arming exits 1. ScopedTestDeadline disarms through RAII.",
                                          "testCoverage":  "Test-TestWatchdog.ps1 drives hidden blocking test cases with short deadlines and validates timeout naming/exit; named regular tests are wrapped through runner helper."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No defect confirmed; detached worker is intentionally process-lifetime in test executables and teardown terminates the process.",
                      "testIds":  [
                                      "Test-TestWatchdog.ps1",
                                      "TestWatchdogSelfTestBlocksForever",
                                      "ScopedTestDeadline wrapper"
                                  ]
                  },
                  {
                      "path":  "Tests/Support/Support.Tests.TestWindowActivationGuard.h",
                      "sha256":  "28f8764f37a06db502f3f02e49fd87426d4fad00bdbb89b7e97374ca5cd442ac",
                      "reviewNotes":  {
                                          "callers":  "DxUi.Tests.Runner.cpp installs blocker for noninteractive suites; FormAccessibility, HostControlRegressions, IntrinsicLayout, RichTextAccessibility, and WindowHost fixtures use or assert the blocker.",
                                          "ownershipAndLifetime":  "WIL unique_hhook owns CBT hook; thread-local active pointer routes same-thread hook callbacks. Callers install/remove on their UI thread; singleton-per-thread guard rejects nesting.",
                                          "failureAndCancellation":  "Start returns false when already active or hook installation fails; destructor calls Stop. Hook protects newly created top-level windows and blocks activation/focus transfers while counting interceptions.",
                                          "testCoverage":  "Runner-wide protected mode and focused HostControlRegressions/WindowHost assertions cover protected window style and blocked activation. Cross-thread Stop is not exercised and would leave owner-thread TLS pointer uncleared; same-thread use is the current caller contract."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No reachable failure confirmed. Document/guard same-thread Stop precondition if helper becomes cross-thread reusable.",
                      "testIds":  [
                                      "DxUi.Tests.Runner no-activation setup",
                                      "HostControlRegressions protected window cases",
                                      "WindowHost activation blocker cases"
                                  ]
                  },
                  {
                      "path":  "Tests/test-scopes.json",
                      "sha256":  "4f6d27688ec59e3c65c8a71d63127de02de91607f35d5f528e9ec1dfe13b1067",
                      "reviewNotes":  {
                                          "callers":  "Tools/ScopedTesting.psm1 consumes standalone scopes; Test-NativeScope.ps1 validates PR contract metadata; Test-InteractiveMode.ps1 and Test-ScopedTesting.ps1 use isolated fixtures.",
                                          "ownershipAndLifetime":  "Tracked declarative scope, rule, interactive-obligation, PR coverage, and contract-file metadata; no runtime ownership.",
                                          "failureAndCancellation":  "Schema/semantic validation failures stop scoped test planning; invalid scope and coverage entries do not silently become runnable obligations.",
                                          "testCoverage":  "Test-NativeScope.ps1, Test-InteractiveMode.ps1, and Test-ScopedTesting.ps1 provide consumer-specific metadata validation."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "Reviewed policy data and listed direct tooling consumers; no specific invalid entry found.",
                      "testIds":  [
                                      "Test-NativeScope.ps1",
                                      "Test-InteractiveMode.ps1",
                                      "Test-ScopedTesting.ps1"
                                  ]
                  },
                  {
                      "path":  "Tools/tests/Invoke-ToolingTests.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-AsanRuntime.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-BenchmarkGate.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-BenchmarkHost.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-BuildMatrix.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-CapabilitySkipPolicy.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-CiGate.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-ConsumerUpdate.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-Docs.ps1",
                      "sha256":  "e667ac28e2f80f529dba64702fd57323f9fc51ca70d6aeb7cdbb1ae4299f88b9",
                      "reviewNotes":  {
                                          "callers":  "Registered tooling test Test-Docs; imports Validation.psm1 and Changelog.psm1 and directly invokes Test-DxUiDocs/Measurements/DesignSystem/Specs/Changelog plus Commit-Gallery and Fold-Changelog.",
                                          "ownershipAndLifetime":  "Synthetic content is under Invoke-FixtureCase roots; the gallery publication fixture makes a temporary bare origin and worktree, publishes only its fixture branch, and explicitly removes the base fixture on normal completion. Changelog workflow fixture is repository-local under .build/changelog-fixtures.",
                                          "failureAndCancellation":  "Validators reject missing catalog usage, changed image bytes, raw docs measurements and unexplained/wrong-owner receipts; specs exclude nested Git checkouts while still validating ordinary docs. Gallery commit refuses force push and NoPush mode leaves origin unchanged; malformed changelog prevents partial fold.",
                                          "testCoverage":  "Tests cover docs/catalog/gallery hash integrity, fenced code parsing, measurement ownership, design system catalog/preview consistency, spec link walk/nested checkout exclusion, changelog fragment shape/fold/line endings/atomic refusal, gallery-only commit/push/no-op/non-force/no-push, completed-plan index, and CI/manual workflow publication guards."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-InteractiveLease.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-InteractiveMode.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-NativeScope.ps1",
                      "sha256":  "0589fa66913e047b5de0d948dec304b95f9548540d3a77d82538ba8607206057",
                      "reviewNotes":  {
                                          "callers":  "Test-NativeScope.ps1 is itself registered in the tooling test suite; it invokes the NativeScope module and fixture scripts against synthetic Git repositories.",
                                          "ownershipAndLifetime":  "Git fixtures are created with repository-local test support and removed under fixture ownership; Invoke-ScopeScript snapshots and restores GITHUB environment variables. Git subprocesses are scoped to each fixture with -C.",
                                          "failureAndCancellation":  "Checks unavailable/malformed commit pairs and uncommitted repositories fail closed; workflow aggregate prerequisites, time limits, and required test ordering are falsified to ensure they cannot silently pass.",
                                          "testCoverage":  "Fixture and direct cases cover docs-only vs source/build/tool/test/document paths, unknown/unavailable paths, actual tracked-path classification, merge-base PR metadata and malformed inputs, CI trigger/workflow producer dependencies, six native budgets/configuration/timeouts, consumer limits, ASan annotation, sequential suite ordering and no duplicate Full build."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-PairedRun.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-PerformanceComparison.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-PerformanceForms.ps1",
                      "sha256":  "8f12c7da8d4533eeb2b0a82a1d35e63ad79c6433d66f27aff065ea46ac5ef1eb",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  "Test-PerformanceForms.ps1"
                  },
                  {
                      "path":  "Tools/tests/Test-PerformancePolicy.ps1",
                      "sha256":  "ce22139a1a93e10901b75b76b8bd51c27db3afe315aa56163cd7ab8e725cf6df",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  "Test-PerformancePolicy.ps1"
                  },
                  {
                      "path":  "Tools/tests/Test-PerformanceProfiling.ps1",
                      "sha256":  "c7f2ed7265f100b2d516abc1b8401d9a17d1c30d83623e280641eea3a4580882",
                      "reviewNotes":  {
                                          "callers":  "Called by the tooling dispatcher; additionally targets the diagnostic benchmark/report APIs and retains its own child-process fixtures in repository-local test support.",
                                          "ownershipAndLifetime":  "Synthetic report fixtures isolate mutable files; owned child tests retain explicit PID/process-handle lifecycle and bounded redirected-output draining; instrumentation and capture artifacts remain fixture-owned.",
                                          "failureAndCancellation":  "Checks schema rejection, bounded diagnostic instrumentation/PNG capture, exact raw-report preservation, direct child timeout, and inherited-pipe drain timeout. A descendant can outlive the direct child by design in this generic helper test and is explicitly terminated by the test using recorded process identity; this is not an interactive lease finding.",
                                          "testCoverage":  "Test-PerformanceProfiling.ps1 contains synthetic schema/capture cases and real owned-child success/failure/timeout/pipeline cases. Read only; not run in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  "Test-PerformanceProfiling.ps1"
                  },
                  {
                      "path":  "Tools/tests/Test-RunEvidence.ps1",
                      "sha256":  "3e4cdfe9a3f6a8952386a12e016e79a08eb91786a804778ea18483c7cc6409e2",
                      "reviewNotes":  {
                                          "callers":  "Runs under the governed tooling test entrypoint; imports TestSupport.psm1 and tests evidence/run receipt operations through repository tooling functions.",
                                          "ownershipAndLifetime":  "Fixture roots are created through New-FixtureRoot and isolated below the fixture scratch root; success evidence is checked immutable and interrupted partial evidence is intentionally retained for recovery. The script also captures/restores process environment around scoped execution.",
                                          "failureAndCancellation":  "Rejects path escapes, drive-root and relative roots, writing handles, linked ancestors/payloads, identity substitution, invalid run owners, and stale metadata. Abrupt child exit is treated as interrupted evidence without publishing a completion marker; no production process is targeted.",
                                          "testCoverage":  "Invoke-FixtureCase cases cover concurrent unique evidence directories, success immutability and prior receipt/ADS preservation, invalid roots/open payloads/reparse links, rename/replacement identity, owner rejection, hard-link read-only behavior, child Environment.Exit(17) interruption and later-run preservation, exclusive native-output capture, and stale provider metadata vs ordered observed inventory."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-ScopedTesting.ps1",
                      "sha256":  "84af30b91e6cae7974f513121320de122a67c0e61d623d624fd561e9c308adaf",
                      "reviewNotes":  {
                                          "callers":  "Tooling dispatcher invokes this PowerShell test; it imports Tools/ScopedTesting.psm1 and the public Test-Changes.ps1 runner in a repository-local fixture.",
                                          "ownershipAndLifetime":  "Creates a unique fixture below .build/ToolTests, a nested isolated Git repository and dependency/artifact files; snapshots PATH and instrumentation environment and restores them in finally; cleanup verifies the resolved fixture remains below the owned root.",
                                          "failureAndCancellation":  "Exercises invalid paths/rules, stale/corrupt receipts, source/library/runtime/dependency mutation, concurrent PR state changes, capability skips and post-run mutation. Nested fake public-runner subprocesses have no explicit WaitForExit timeout; fixture commands are deterministic local PowerShell only.",
                                          "testCoverage":  "Named cases cover scalar/batched globs, interactive obligations, all six profiles, clean/staged/unstaged/deleted/renamed paths, identity closure, platform-specific vcpkg evidence, PR delegation, and cached runner failure. Read only; not executed."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  [
                                      "Test-ScopedTesting.ps1: scope and interactive obligations",
                                      "Test-ScopedTesting.ps1: source/artifact/dependency identities and stale receipt rejection",
                                      "Test-ScopedTesting.ps1: PR delegation contract and runner lifecycle"
                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-SuiteFailure.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-TestFilter.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-TestPort.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-TestWatchdog.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-UnicodeGraphemeData.ps1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tools/tests/Test-Validation.ps1",
                      "sha256":  "629c3ae4201d298903bb86f5fd02dd830a8a7c20a1a88aa42bd8efa58194b141",
                      "reviewNotes":  {
                                          "callers":  "Fully read; concrete registration/call site identified in source or consumer.",
                                          "ownershipAndLifetime":  "Fully read; source-level owners and borrowed/scoped lifetimes recorded.",
                                          "failureAndCancellation":  "Fully read; failure/cancel handling recorded; no runtime claim from static review.",
                                          "testCoverage":  "Actual test IDs listed; tests were not executed in this review."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "reviewed-no-confirmed-finding",
                      "testIds":  "Test-Validation.ps1"
                  },
                  {
                      "path":  "Tools/tests/Test-VcpkgTriplet.ps1",
                      "sha256":  "26c947b5d024ed41a0522cd4de34f4b242a5f29d22f540f91ce31473af61d6ba",
                      "reviewNotes":  {
                                          "callers":  "Invoked by the tooling suite; imports VisualStudio.psm1 and VcpkgTriplet.psm1 and parses the public vcpkg-install.ps1/build.ps1 consumer surface.",
                                          "ownershipAndLifetime":  "Synthetic Visual Studio/vcpkg fixture files use Invoke-FixtureCase-managed roots; no actual checkout, vcpkg restore, or network is needed. CMake and Windows PowerShell 5.1 are optional executable probes and skip when absent.",
                                          "failureAndCancellation":  "Missing/malformed version files and absent stock triplets fail with repair/path context before clone/bootstrap/run; overlay generation rejects unsupported patch pins/empty install paths and preserves stock text.",
                                          "testCoverage":  "Direct and fixture cases cover version parsing/normalization/BOM/line endings/length, CMake escaping and pin readback, stable no-op timestamps, changed installation/toolset/stock triplet rewrites, missing triplet no-write, unchanged installer interface/early discovery/order, shared discovery callers, ASCII and optional PowerShell 5.1 parsing."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No confirmed defect found in this file during source-backed test review; limitations/gaps are stated in review notes.",
                      "testIds":  [
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
                                  ]
                  },
                  {
                      "path":  "Tools/tests/TestSupport.psm1",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.AnimationClock.cpp",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Controls/Controls.Tests.AnimationClock.h",
                      "sha256":  null,
                      "reviewNotes":  {
                                          "callers":  "Pending full read.",
                                          "ownershipAndLifetime":  "Pending full read.",
                                          "failureAndCancellation":  "Pending full read.",
                                          "testCoverage":  "Pending full read."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "pending",
                      "findingIds":  [

                                     ],
                      "disposition":  "pending-full-read",
                      "testIds":  [

                                  ]
                  },
                  {
                      "path":  "Tests/Embedded/Embedded.Tests.BenchmarkInstrument.h",
                      "sha256":  "d4cce9201fb0fa080e26b0cad623a64198e00b4d3a28bb9190aafb6b864d8153",
                      "reviewNotes":  {
                                          "callers":  "Included from exactly one translation unit, Embedded.Tests.Embedded.cpp; benchmarks access thread-local plain/allocation counters and opt-in allocation failure injection.",
                                          "ownershipAndLifetime":  "Defines global replacement new/delete operators; thread-local counters avoid cross-thread interference and liveBytes measures plain forms only by documented scope. Aligned allocations are counted but excluded from liveBytes.",
                                          "failureAndCancellation":  "Injected allocation failure throws std::bad_alloc at chosen count; allocation primitives throw on malloc/aligned allocation failure. This is a test executable-wide instrumentation shim.",
                                          "testCoverage":  "GridSelection and ComplexUi benchmark paths consume counts; native allocation-failure sweeps exercise failAllocationAfter; compile/link inclusion exactly once is documented to avoid duplicate operator definitions."
                                      },
                      "reviewer":  "review_ledger",
                      "reviewStatus":  "reviewed",
                      "findingIds":  [

                                     ],
                      "disposition":  "No concrete defect found; documented live-byte metric excludes aligned forms, so reports must retain that boundary.",
                      "testIds":  [
                                      "Embedded allocation-count scenarios",
                                      "Embedded allocation-failure sweeps"
                                  ]
                  }
              ]
}
```
