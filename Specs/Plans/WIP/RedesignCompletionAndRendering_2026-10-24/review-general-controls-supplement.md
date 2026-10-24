# DxUi general controls test review supplement

```json
{
  "reviewedCommit": "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
  "sourceCommit": "b06f534b36ffc2c0c9671a142d1e51545480be22",
  "status": "IN_PROGRESS",
  "scope": "Read-only supplement for pending Tests/Controls and Tests/ConsumerModules rows selected from review-general-tests-current.md at initialization. Each row will be marked reviewed only after a full source read and test/caller trace; no builds or tests are run.",
  "assignedFileCount": 34,
  "fullReadCount": 31,
  "pendingCount": 3,
  "files": [
    {
      "path": "Tests/Controls/Controls.Tests.DxUiTestHelpers.h",
      "sha256": "7b6c7349a11dcc9b48ac5a3f129beb784025fd048de9ccefead49162eb200c6a",
      "lineCount": 2683,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestGridMultilineDecomposedAccentsPaintLikePrecomposed",
        "TestNativeTransitionStartsOnTheDispatcherTimelineAfterAStall",
        "TestApplicationTextServiceLifecycle",
        "TestHostFocusReplacementLeavesControlAlone"
      ],
      "reviewNotes": {
        "callers": "Shared include for Controls test translation units; AttachedHostWindow routes WndProc through WindowHost::HandleMessage, then optional application callback. DispatchQueuedMessageForTest is used by host and animation tests; foreground helpers back native-TSF activation tests; RequireFocusReplacementLeavesControlAlone is used by Controls and native text-input regression tests.",
        "ownershipAndLifetime": "AttachedHostWindow owns HWND via wil::unique_hwnd and WindowHost, explicitly detaches before resetting HWND, and stores callbacks capturing test state only while fixture lives. COM/WIC helpers use wil::com_ptr and scope_exit for VARIANT/BSTR/SAFEARRAY/IMM context cleanup. Grid models are borrowed by grids and fixture member order ensures model outlives window-owned tree. Test artifact helper creates a PID-specific directory under repo .build and is used for animation/windowhost logs and gallery captures; consult the DxUi runner sandbox/output contract before treating it as authorized test scratch.",
        "failureAndCancellation": "Message pumps cap at 4096 messages and optional duration; bounded preparation loops cap at 1024. Wait helpers use absolute deadlines and report wait errors. Foreground retry caps attempts at five; repeated observed thieves are capability skips, activation refusal fails. Potential policy issue: GetDxUiTestArtifactDirectory writes under .build without an ownership marker in this helper; assess against the selected test-runner sandbox contract, since this review did not execute it.",
        "testCoverage": "Helper coverage is indirect: named grid multiline, native dispatcher-stall, application TSF lifecycle and focus-replacement tests consume it. The shared fixture also provides test filtering/watchdog, bitmap compare, text/grid/tree models and input/foreground instrumentation; no helper-only test proves every branch. No execution/build performed."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.GridMultilineRendering.h",
      "sha256": "f0a363167255c0a1315ea5514d0a15a2e7fc5e815042126fd1d63be4ca791e6b",
      "lineCount": 881,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "Included by DxUi.Tests.Rendering.cpp after the multiline fixtures and registered in its rendering suite. MultilineBed owns the model before AttachedHostWindow so the host-owned Grid can borrow it; grid replacement is via SetRoot.",
        "ownershipAndLifetime": "Captures use host test utilities and DirectWrite layouts in local wil::com_ptr; model text stays exact (including decomposed accents) and is not normalized. Fresh-grid comparisons preserve model and settings while replacing only the Grid/root. Device loss tests retain DWrite layouts while rebuilding D2D resources.",
        "failureAndCancellation": "Tests bound the 100k-value prefix and cache ceiling and assert clipping, ellipsis, and layout-reuse behavior across RTL, DPI/theme/font/density, eviction, device loss and host movement. Long loops are deterministic but substantial; no cancellation path is exercised. Independent font/platform raster variability is controlled by same-process twins, while DirectWrite-specific shaping remains platform-dependent.",
        "testCoverage": "Eleven registered rendering cases cover cluster-safe prefixes, bidi marker direction, viewport clipping, cache prefix identity/caps, retained layout invalidation, device loss and movement. Base rendering cases live in DxUi.Tests.Rendering.cpp. No runtime or pixel verdict is claimed here."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextInputServices.h",
      "sha256": "669e47ab4f96154432d6e0decf8ef06cf7502daf6fc81c3d6a6eab964d256492",
      "lineCount": 1208,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "Included and registered by DxUi.Tests.NativeTextInput.cpp; the tests exercise TextInputServices, application TextInputClient and the shared COM text store created by CreateClientTextStore. TSF-focused cases initialize STA and use AttachedHostWindow; lower-level store cases use fake clients/sinks and no renderer HWND.",
        "ownershipAndLifetime": "Fake clients and sinks are declared before stores where held COM references must release first. Application TextInputServices borrows the attached HWND; clients use shared_ptr snapshots. Clipboard/TSF resources are local and services are explicitly cleared/detached. Reentrancy fixtures intentionally replace, clear, or retire clients inside Read/Apply/Cancel/TSF callbacks and check the successor stays active.",
        "failureAndCancellation": "Coverage includes stale revision/focus, masked/read-only access, invalid/oversized snapshots and UTF-16 clipboard data, deferred lock posting failure and retry, callback disconnection, final store release, composition cancellation/rollback, and balanced transaction notifications. Potential test-guideline issue: TextServicesReentrantFocusSink::OnSetFocus catches std::exception (which includes std::bad_alloc) and maps to E_FAIL despite repo rule to keep bad_alloc fatal; this is test-fixture code and should be dispositioned by owner.",
        "testCoverage": "Fifteen named cases cover COM composition/edit transactions, reentrancy, lock coalescing, geometry, clipboard bounds, focus retirement and disconnect. The foreground-preservation case may capability-skip only on observed foreground theft; it fails activation refusal. No tests/builds were run in this review."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextStoreComposition.h",
      "sha256": "ec60870948d48465544419c9a0fc155d39073e53b7a5889018e6247f3c51fc28",
      "lineCount": 168,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestNativeTextStoreCompositionRangeUsesStagedTextAfterInsert",
        "TestNativeTextStoreSequentialCompositionsInOneLockHaveSeparateUndoUnits",
        "TestNativeTextStoreStaleSameLockCompositionCannotReplaceExternalText",
        "TestRichTextNativeTsfCompositionAndDisconnection"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/DxUi.ControlTests.vcxproj",
      "sha256": "03a35fd7d38aecfd2e9d2a8e005116f04fbbc99009f1523120eaf1e62490142c",
      "lineCount": 11,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "Project build wiring: explicit ClCompile entries for test suites; no test execution evidence"
      ],
      "reviewNotes": {
        "callers": "The project compiles the Controls test translation units and references src/DxUi.vcxproj; the runner registration lives in DxUi.Tests.Runner.cpp, so inclusion here alone does not prove registration.",
        "ownershipAndLifetime": "The project owns no runtime objects; MSBuild includes each listed translation unit in the test executable. Explicit per-file includes make adding a new test implementation require project registration.",
        "failureAndCancellation": "Configuration defines Debug/Release and ASan Debug for x64/ARM64; the file sets ENABLE_TESTS and Console subsystem. No project-level test timeout/cancellation policy is encoded here.",
        "testCoverage": "Source review only: verifies explicit source inclusion, DxUi reference, and ENABLE_TESTS. No build or test run was performed; runtime coverage must be established from named runner registrations and receipts."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Animation.cpp",
      "sha256": "19d080bf3aaa806516a3d75254d1a1339a0b1e60ad3b653ab5758b5ec477dc59",
      "lineCount": 1654,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestFrameRuntimeClockIsMonotonic",
        "TestAnimationDispatcherActiveSubscribersReceiveMonotonicHighResolutionTicks",
        "TestSliderTouchHaloStaysInsideItsPaintBoundsAndBehindOverlays",
        "TestGridScrollbarFeedbackFollowsHoverAndDragState",
        "TestPageHostConnectedOverlayAnimationEmitsCompositionGateMetrics",
        "TestPageHostTransitionBegunWhileHiddenResumesWhenPainted"
      ],
      "reviewNotes": {
        "callers": "RunAnimationTests registers this translation unit in DxUi.Tests.Runner.cpp. Cases compose FrameRuntime, AnimationDispatcher, controls, PageHost, AttachedHostWindow and Support performance capture; the embedded clock family is included from Controls.Tests.AnimationClock.h.",
        "ownershipAndLifetime": "WindowHost/root trees and controls are scope-owned; test models outlive borrowed Grid references. AttachedHostWindow owns and detaches native HWND. AnimationDispatcher callbacks use stack context only while subscription scope is active; metric sink is stopped by ScopedAnimationPerfJsonl.",
        "failureAndCancellation": "Tests cover reduced-motion snapping, settled tick cancellation, pointer capture loss, hidden PageHost no-subscription/resume, device-independent visual clipping, dispatcher monotonicity/hitch, and a live native stall. Dispatcher test globally calls Shutdown before/after cases, so serialized suite execution is assumed. Timing/foreground and pixel-capture paths are not run in this review.",
        "testCoverage": "Named tests cover frame clock arithmetic, dispatcher cadence/metrics, button/slider/caret/grid/page transitions, overlays, reduced motion and embedded epochs. Static reading found no definite product defect. The native dispatcher stall and connected-overlay metric cases rely on wall-clock callbacks and actual capture; no build/runtime result is claimed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.ComboBox.cpp",
      "sha256": "3eca13d18e5ada0b5e85e9344ef2135f37ff55f2360ff7b09c4bdd7d5bdfa234",
      "lineCount": 1433,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestAttachedComboBoxSentMouseHoverAndOutsideDismiss",
        "TestComboBoxTouchRowsKeepGeometryAndSelectionCoherent",
        "TestComboBoxLocalizedNoMatchesRefreshesOpenPopup",
        "TestComboBoxInputLeavesAComboBoxTheFocusCallbackDestroyed",
        "TestEditableComboBoxImportReportsWhenTextChangedDestroysComboBox",
        "TestComboBoxTextChangeSnapshotCleanupRetiresRootBeforeCallerContinues"
      ],
      "reviewNotes": {
        "callers": "RunComboBoxTests is dispatched by DxUi.Tests.Runner.cpp. Suite covers ComboBox popup layout/input, ScrollPanel overlay transforms, TextInput state, color-font rendering, theme styling, callbacks and reentrant root replacement.",
        "ownershipAndLifetime": "ComboBox/Grid items and model references are owned by local vectors/control; attached hosts own their HWND and root; tests check callback snapshots remain alive if controls are destroyed or callables replaced. One helper constructs a shared_ptr with explicit new/delete for a cleanup sentinel to force root cleanup on captured-callback release.",
        "failureAndCancellation": "Named tests cover popup light-dismiss to nonfocusable target, click-through/overlay hit blocking, scroll viewport coordinate transforms, clamped page navigation, scrollbar drag, touch min row height invalid values, reentrant callback/destruction, import survival result and popup close on focus loss. Native color-font test depends on installed DirectWrite color-font support and asserts pixels; no build/run executed.",
        "testCoverage": "Thirty-eight cases include rendering/pixel probes, hover and click, constraints, scrolling, selection and focus, ownership/reentrancy, localization and callback state. Same-host captures avoid stored bitmap oracles. Review found no confirmed defect; source review is not runtime qualification."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Controls.cpp",
      "sha256": "7a8f975777eb867125a2567fd1402b049f59cdd9edba832ab00c5c6f555edd56",
      "lineCount": 3525,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunControlTests registers focused container, callback, grid, input and layout cases and calls the intrinsic/form suites. Local fixtures exercise Panel, ScrollPanel, PageHost, TabControl and host dispatch directly; the shared test runner drives these registrations.",
        "ownershipAndLifetime": "Panel/Tab extraction tests move owning unique_ptr children, preserve metadata/selection, and probe reentrant focus/capture callbacks that can transfer or destroy owners. ScrollPanel detach probes cover hover, capture, nested descendants, pages and tabs. Traversal and PageHost fixtures deliberately replace roots/pages during callbacks and verify stale results are rejected; shared state is test-owned to survive retired controls.",
        "failureAndCancellation": "Coverage exercises capture cancellation and rollback on extracted live sliders, silent capture release, callback-driven root replacement, exception-to-HRESULT behavior, measured-layout output preservation on invalid input/overflow, and clean recovery capture after page/scroll paint retirement. Paint capture uses attached host windows; other cases are deterministic direct dispatch. No runtime result is claimed.",
        "testCoverage": "The 3525-line file registers broad regressions for ownership transfer, reentrant focus/capture, empty slots, accessibility invoke, keyboard/mouse semantics, menu/tab geometry and callbacks, selection, grid checkboxes, toggles, tag picker, and animation-adjacent painting. Meaningful remaining gap: no fault-injection allocator coverage for container/tab mutations; no build/test executed by this review."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.EditorControls.cpp",
      "sha256": "3f25ff036a07fab47e56747087493a246a3bf8f956c78876d2e037932a841382",
      "lineCount": 2012,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunEditorControlTests registers Splitter, NumericStepper, ColorPicker, reparenting and catalog cases, then dispatches shared document/editor tests. Local helpers send direct control input to WindowHost; movement cases compare a reparented control against a newly configured oracle.",
        "ownershipAndLifetime": "Tests use stack WindowHost/PageHost and unique_ptr control trees. Reparent cases transfer owning child pointers with TakeChild; callback replacement cases deliberately destroy the active root during Commit. TestEditorControlPressesLeaveAControlTheFocusCallbackDestroyed exercises focus-callback teardown for Splitter and ColorPicker. The panel empty-slot case wraps ClearChildren in SEH because access violation is not a C++ exception.",
        "failureAndCancellation": "The suite covers Splitter capture/Escape rollback, NumericStepper Escape/unparseable-edit rollback and callback root replacement, ColorPicker capture/Escape/Cancel rollback, host-change gradient release, density/flow propagation, and stale-slot clearing. Several tests call Paint without a device context; host-dependent capture paths use AttachedHostWindow and CaptureWindow. No cancellation fault injection or runtime result is claimed.",
        "testCoverage": "Registered cases include numeric parsing/rounding and locale digits, drag and keyboard input, callbacks that replace roots or retain mutable state, picker hex and typed components, accessibility names and caption width bounds, moved control geometry and cached resources, inheritance announcements and null child slots. Evidence is source-only; no build/test executed by this review."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Gallery.cpp",
      "sha256": "bf347dc6504c79219af9f8de0a7d0490273b417f1c890d9c71c1ed774acdabd0",
      "lineCount": 2101,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "RunGalleryGenerator",
        "RunGalleryGeneratorPerTheme",
        "RunButtonContrastAuditGenerator"
      ],
      "reviewNotes": {
        "callers": "Generator entrypoints are invoked by the test executable/tooling-selected gallery and contrast-audit modes, not DXUI_RUN_TEST registrations. RunGalleryGenerator builds five deterministic theme scenes; menu/combo bitmap capture is driven from a dedicated owner HWND plus a joined background popup-window observer.",
        "ownershipAndLifetime": "GalleryScene owns the control tree and models via unique_ptr; model pointers are borrowed by Tree/Grid and remain alive through capture. GalleryBitmapPreview owns capture bytes and caches device-specific D2D bitmap/device via wil::com_ptr, replacing cache when host device changes. Popup capture uses stack AttachedHostWindow, a driver thread joined before local teardown, and scope_exit to dismiss owned popup chain.",
        "failureAndCancellation": "Popup waits have finite deadlines and bounded polling; capture validates visible/nontransparent pixels before composition. Thread driver records failure text and is joined before Require. Output paths are made absolute, existing output is removed, then PNG write/existence checked; this is a generator side effect that depends on caller-provided output directory. No build or runtime qualification performed.",
        "testCoverage": "Gallery content spans control variants, themes, RTL/localized forms, rich text/math/diagram previews, popup snapshots, and button-contrast calculations. Source tests verify scene expectations and output checks, but no dedicated unit registration or assertion independently validates capture-thread timeout cleanup or pixel provenance; no execution result claimed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.InteractiveLease.cpp",
      "sha256": "6e7813a0370e751561a9eeec1b1a4d4b30c8a893a4d35e0b3f61e81edf406417",
      "lineCount": 1314,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunInteractiveLeaseTests is registered in DxUi.Tests.Runner; this source exercises shared DesktopLease/RunLease/CheckInteractiveDesktop from Tests/Support and the non-activating native backend.",
        "ownershipAndLifetime": "FakeDesktop models windows and callbacks with stack fixtures. Win32 backend test owns only an offscreen WS_EX_NOACTIVATE test HWND with wil::unique_hwnd and never shows it. FakeServices records RAII-like acquire/release/awake/warning order; test uses no real foreground interaction.",
        "failureAndCancellation": "Named cases cover declined/timed-out/interrupted requests, failed children/start/watchdog, busy lease, lost desktop during countdown, failed foreground/focus/cursor restoration, stale capture and release on every path. Loops are finite per lease constants and fake Wait never sleeps. Real Win32 probe is read-only.",
        "testCoverage": "Suite registers 44 named cases across lease restoration, availability, confirmation/autostart, process results, serialization/parser and countdown. Implementation resides in Support.Tests.InteractiveLease.h and external tool runner supplies private desktop for its own self-test; no test run performed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Menu.cpp",
      "sha256": "1c919be05e8a211aaa1d158349e0fff31c5376f213b2129801282ad61d8b0576",
      "lineCount": 10820,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunMenuTests dispatches RunNewControlCapturingMenuTests and RunMenuDescriptionTests; RunMenuDescriptionTests registers accessibility, modal and async menu, UIA, scroll, focus, DPI, layout, rendering, and lifetime cases. Menu callbacks are invoked by popup message handling and posted payload dispatch.",
        "ownershipAndLifetime": "Fixtures own AttachedHostWindow, popup UIA COM providers, test payloads, and foreground witness threads with WIL/jthread scopes. Tests verify retained providers disconnect after popup teardown, payload registry saturation/release and owner drain, queued invoke cancellation on WM_NCDESTROY, layout/accessibility resource return while providers remain held, and process-exit suppression of callbacks. No defect confirmed from the complete source read.",
        "failureAndCancellation": "Coverage exercises payload-post saturation HRESULT and recovery, failed optional description prep with recovery, Escape dismissal before queued UIA action dispatch, submenu close without ending parent session, foreign-thread dismissal, device-loss redraw, and foreground loss/retry. Interactive/focus/cursor tests use the foreground-held helpers or explicitly skip without the desktop; code review only, no test execution.",
        "testCoverage": "Meaningful exact cases include TestDescribedMenuAccessibilityInvokeReportsPayloadSaturationAndRecovers, TestDescribedMenuPointerAndCancelledQueuedInvoke, TestPlainMenuAccessibilityScrollsFocusedRow, TestMenuSurvivesAWindowClosingItFromSetCursor, TestNativeMenuBarNestedPopupCanDestroyHostSafely, and TestLargeMenuPaintsOnlyVisibleRowsWithCachedOffsets. Additional registrations cover exact layout geometry, 50/128/200-row behavior, both visual themes, fractional DPI, and process exit; no runtime or pass claim."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.MultilineText.cpp",
      "sha256": "e15f6c59d0664ff636e2b4d5bd7ce9631bd1eba75d20c3ea3c752c4b4e9daaac",
      "lineCount": 3492,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunMultilineTextTests is invoked by the Controls test runner. Cases instantiate TextField/ExposedTextField directly through WindowHost or ClipboardHostWindow and call OnKeyDown, OnChar, mouse handlers, wheel handlers, and TextInputState import/export.",
        "ownershipAndLifetime": "TextField and root ownership are stack/Panel scoped; ClipboardHostWindow owns the native host/clipboard interaction. Direct tests keep source text snapshots by value. Clipboard-sensitive cases use RetryClipboardSensitiveAction where contention is expected. No confirmed defect in test source; no production callers were traced because this row is test-only.",
        "failureAndCancellation": "Assertions cover failed/no-op clipboard actions preserving sentinel clipboard, LF normalization from CRLF paste, selection replacement/caret collapse, undo/redo and redo invalidation after a new edit, word/visual-line navigation, wheel delta accumulation, and dialog Return/Escape/Tab routing. Clipboard retries are bounded helper-mediated; no explicit cancellation path.",
        "testCoverage": "Exact registered cases include TestMultilineTextFieldUndoRedoRestoresPartialSelectionReplacement, TestWrappedMultilineTextFieldCtrlXCutsPartialSelection, TestMultilineTextFieldAccumulatesPartialWheelDelta, TestWrappedMultilineTextFieldPageKeysUseVisualLines, and TestMultilineTextFieldMixedDialogFlowKeepsReturnEscapeAndTabRoutingCorrect. Coverage distinguishes logical newline versus wrapped visual-line behavior and full versus partial ranges. No tests were run."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.NewControls.cpp",
      "sha256": "c637b6f36dee1695ae284765b3c81e897e72274e6bc695ca87909fba980a540f",
      "lineCount": 3027,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunNewControlTests registers button, checkbox/radio, progress/page, slider, toolbar/menu bar, tab, status strip/tag picker, and host backdrop cases. The separate RunNewControlCapturingMenuTests registers the two native oversized-menu modal fixtures through RunMenuTests.",
        "ownershipAndLifetime": "Tests mostly use stack-owned controls or Panel-owned children under WindowHost/AttachedHostWindow. Callback-reentrancy cases replace the root, reorder/remove tab siblings, or retain captured state; composite-parts test asserts embedded controls cannot be detached. Three test fixtures use shared_ptr<int>(new int, custom deleter) to trigger captured-state cleanup and host root retirement; this is test-only and works through the custom deleter, but it is a localized raw new/delete pattern inconsistent with DxUi AGENTS resource guidance only if that guidance is interpreted as applying to ordinary test allocations. No production defect confirmed.",
        "failureAndCancellation": "Tests cover callback-triggered root replacement and sibling mutation, selection/callback identity resolution, capture release before callbacks, slider draft cancellation on capture loss, invalid/nonfinite range rejection, progress tick resets/hidden/reduced-motion cases, and oversized-menu dismiss cleanup. The two capturing-menu fixtures use an auxiliary std::thread with bounded popup readiness and scope-exit Escape; parent runner holds the explicit interactive lease. No tests were run.",
        "testCoverage": "Exact cases include TestTabControlRemoveResolvesRequestedPageAfterFocusCallbackReordersSiblings, TestTabControlSelectionSnapshotCleanupCanRetireRootBeforeMouseDownTail, TestProgressBarIndeterminateTickSurvivesClockReset, TestSliderOffCenterGrabAndCancel, TestTagPickerSelectCallbackCanRetireRootAndReadSelectionAndCapture, and TestContextMenuPopupScrollsOversizedContent. Around 100 registered cases span callback mutability/lifetimes, stable tab indices, RTL, boundaries, capture and rendering-missing-context paths. No runtime qualification is claimed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.ReadOnly.cpp",
      "sha256": "d41645dbff91678b11031d60f7901e2a1aecd5bae5cbdabb12785f1fac6e229d",
      "lineCount": 1697,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "RunReadOnlyTests registers unit and native-host coverage for read-only multiline and wrapped multiline TextField. Direct WindowHost cases exercise TextField handlers; AttachedHostWindow cases route WM_COPY/CUT/PASTE/CLEAR/CHAR/UNDO and verify the native host HWND is the text-input target with no bridge child.",
        "ownershipAndLifetime": "Tests own host, Panel tree, and ExposedTextField through automatic/Panel lifetime. Clipboard-sensitive test bodies use RetryClipboardSensitiveAction, while non-clipboard navigation/undo paths avoid clipboard state. Attached fixtures are no-activation and use SendMessage for deterministic native message routing. No specific defect confirmed from source; no runtime claim.",
        "failureAndCancellation": "Covers copy allowed with selection while cut/paste/clear/character/undo/redo mutation is rejected, empty selection preserving clipboard, text and caret, direct/wrapped navigation still functioning, and native state/visible selection staying aligned after Ctrl+Arrow. Retry handles clipboard contention only; no cancellation branch.",
        "testCoverage": "Meaningful exact cases include TestAttachedReadOnlyMultilineNativeHostSuppressesMutatingMessages, TestReadOnlyMultilineNativeHostCopyShortcutsPreserveSelectionAndText, TestAttachedReadOnlyMultilineNativeHostUndoRedoLeaveTextAndSelectionUnchanged, TestReadOnlyWrappedMultilineTextFieldSuppressesMutationAndKeepsNavigation, and TestAttachedReadOnlyWrappedMultilineNativeHostCtrlArrowKeepsCaretAligned. The suite pairs logical-newline and wrapped visual-line behavior and tests both selected and collapsed-caret paths. No tests were run."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Rendering.cpp",
      "sha256": "14cacc34b997268a61073866c1be75ad97795530b9b1d7b961ef5155ad501461",
      "lineCount": 2832,
      "reviewStatus": "pending",
      "disposition": "pending-full-read",
      "testIds": [],
      "reviewNotes": {
        "callers": "Pending full-file read and test/caller trace.",
        "ownershipAndLifetime": "Pending full-file read.",
        "failureAndCancellation": "Pending full-file read.",
        "testCoverage": "Pending exact case identification; no tests executed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Runner.cpp",
      "sha256": "dee55732f3452b9d6adf999f98a11089d027301ccafe238d5214df24f7baaedb",
      "lineCount": 615,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestWatchdogSelfTestBlocksForever",
        "RunWatchdogSelfTestFixture",
        "RunWatchdogSelfTestProgress",
        "RunTooltipTests",
        "RunAnimationTests",
        "RunInteractiveLeaseTests"
      ],
      "reviewNotes": {
        "callers": "wmain is the Controls executable entrypoint and dispatches suite functions declared in this file. RunDxUiTestFilter records exact test-name matches in shared helper; Tools invoke CLI switches for targeted runs, watchdog probes and artifact output.",
        "ownershipAndLifetime": "Clipboard test adapter is stack-owned and globally installed only inside wmain, with scope_exit clearing the borrowed global. Perf capture, activation guard, foreground thief, watchdog timeout and fixture deadline have scoped owners. The runner calls dispatcher Shutdown after each suite.",
        "failureAndCancellation": "CLI parsing validates identifiers/numbers and rejects unknown switches, incompatible no-activate/focus combinations and watchdog switches with suite/test filters. Watchdog deadlines wrap each named case or fixture. Test failure exits are routed away from dialogs. Fixture suite MenuExitLifetime intentionally returns process exit through CRT path.",
        "testCoverage": "Runner explicitly dispatches suite registrations including Controls, InteractiveLease, Tooltip, Animation, Rendering and NativeTextInput, and rejects unmatched test names so name-filtered runs cannot pass empty. Source review only; no executable was run. No issue confirmed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Theme.cpp",
      "sha256": "d6160e78a88f4d29307aa289fa8a08836dd9d4df2559739a708078573b35a2cb",
      "lineCount": 2994,
      "reviewStatus": "pending",
      "disposition": "pending-full-read",
      "testIds": [],
      "reviewNotes": {
        "callers": "Pending full-file read and test/caller trace.",
        "ownershipAndLifetime": "Pending full-file read.",
        "failureAndCancellation": "Pending full-file read.",
        "testCoverage": "Pending exact case identification; no tests executed."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Tooltip.cpp",
      "sha256": "c9a258a29508a27eac35bb6606870f114f48ae3a8377cc8a0fe4aa6091d42ed4",
      "lineCount": 681,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestTooltipInspectionScrollsWithoutChangingFocusOrValue",
        "TestTooltipInspectionOutsideDismissalConsumesWholeGesture",
        "TestTooltipInspectionCaptureCallbackWinsReentrantOpen",
        "TestTooltipLayerHideDelayExpiresAfterTimerTicks",
        "TestTooltipDeadlinesUseCurrentDispatcherClockAfterIdleHostTick",
        "TestPassiveSupplementalTooltipUsesHostHitTestingDelayLifetimeAndClickThrough"
      ],
      "reviewNotes": {
        "callers": "RunTooltipTests is called by DxUi.Tests.Runner.cpp. Tooltip cases drive WindowHost::SetTooltip/InspectTooltip, input message routing, Grid/Tree hover, and AnimationDispatcher ticks; real attached windows use no-activate setup.",
        "ownershipAndLifetime": "AttachedHostWindow owns native windows; controls live in Panel-owned trees and test models outlive borrowed references. ReentrantTooltipCaptureControl intentionally opens a newer inspection from OnCaptureLost and verifies latest callback state wins. DispatcherTicks unsubscribes on scope exit.",
        "failureAndCancellation": "Tests cover exact hide/show deadlines, movement cancellation, superseded reentrant inspection, click-through dismissal consuming the releasing gesture, Escape and pointer-drag scroll, stale target replacement, tiny viewport clamping and same-cell tracking. Message dispatch uses bounded 10-second deadline; no build/runtime validation performed.",
        "testCoverage": "Eighteen named cases cover placement, wrapping, clipping, persistent inspection navigation, focus/value retention, reentrancy, stale dispatcher clocks, hover tracking and passive target lifetime. Source review found no definite defect."
      }
    },
    {
      "path": "Tests/Controls/DxUi.Tests.Tree.cpp",
      "sha256": "6e37b5f8c7137217908b44f4e371616421219584fbe2ded21b1e5aae680d38c8",
      "lineCount": 3582,
      "reviewStatus": "pending",
      "disposition": "pending-full-read",
      "testIds": [],
      "reviewNotes": {
        "callers": "Pending full-file read and test/caller trace.",
        "ownershipAndLifetime": "Pending full-file read.",
        "failureAndCancellation": "Pending full-file read.",
        "testCoverage": "Pending exact case identification; no tests executed."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.FormAccessibility.h",
      "sha256": "bea70d975efbf0bce2f76d999c34b6b78d176b46809af8061068cea507f14d01",
      "lineCount": 12,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestFormAccessibilityWindowLabelLifetimeAndReplacement",
        "TestFormAccessibilityEmbeddedLabelLifetimeAndReplacement",
        "TestFormAccessibilityWindowPublicationStopsAfterDestructiveCallback",
        "TestFormAccessibilityEmbeddedPublicationStopsAfterEditorRetirement"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.Forms.h",
      "sha256": "800b49c94d28a7b9dc806a9773341b25494d7753d596a2c9cbaf8c7980f9c08c",
      "lineCount": 3,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "RunFormLayoutTests",
        "TestFormsConstraintsBudgetsAndCustomFallbackFailBeforeGeometry",
        "TestFormsMeasurementCallbacksRetireOrSupersedeSafely",
        "TestFormsBoundsCallbacksKeepNewerLayoutsAndRejectForeignMutations",
        "TestFormsEmbeddedReflowPreservesCompositionSelectionAndFocus"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.GridUiaPattern.h",
      "sha256": "e36c23ba67b23a5fcb757200360a87cdd455055a5e81b04b559b16e3517882fd",
      "lineCount": 19,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestGridUiaPatternSupportsBoundedOffscreenGetItem",
        "TestGridUiaPatternStopsWhenCellReadReplacesItsControl",
        "TestGridUiaPatternContainsModelExceptionsAtTheProviderBoundary",
        "TestGridUiaPatternWindowAnswersWhenTheFirstPublicationFails"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.HostControlRegressions.h",
      "sha256": "5fa384938d16484a7be87ac92b871265489324cfb8476dc049764dad59842d53",
      "lineCount": 16,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestPageHostWindowUiaTraversesInvokesAndRetiresPageChildren",
        "TestPageHostEmbeddedUiaTraversesInvokesAndRetiresPageChildren",
        "TestEmbeddedTransitionsUseApplicationClockIncludingZero",
        "TestEmbeddedGraphicsRetirementPreservesApplicationBindings"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.IntrinsicLayout.h",
      "sha256": "168ac50dd72d8878c27d8d424f3c99b2708598a93e2253363788eb442246a71b",
      "lineCount": 3,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "RunIntrinsicLayoutTests",
        "TestIntrinsicMeasurementContainsFailuresAndPreservesOutputs",
        "TestFlexMeasureCallbacksCannotPublishStaleOwnershipOrGeometry",
        "TestEmbeddedVisualCancellationCanReenterDetach",
        "TestIntrinsicLayoutFailureKeepsPaintingThePreviousGeometry"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.MenuPopupReadiness.cpp",
      "sha256": "4ab8ad203c162808716480093e241899c111bb1e78e6aa790f8f0cfadfd5c027",
      "lineCount": 45,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
        "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
      ],
      "reviewNotes": {
        "callers": "Implements the readiness helper regression called by the Controls runner; it creates synthetic owned STATIC windows on a private desktop to exercise the helper.",
        "testIds": [
          "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
          "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
        ],
        "failureAndCancellation": "It proves hidden initialization HWNDs are rejected even with matching owner/class, accepts the same popup after show, and rejects it again after hide. Setup failures surface through Require; no test was run here.",
        "ownershipAndLifetime": "The fixture creates an isolated desktop and thread-owned HWNDs using wil::unique_hdesk/unique_hwnd; the test never switches the input desktop or takes foreground focus.",
        "testCoverage": "Exact case: TestMenuPopupDiscoveryRejectsHiddenInitializationWindows; the related late-popup failure cleanup case is TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.MenuPopupReadiness.h",
      "sha256": "86413cc4ad57cb5e363e93ba449a2fdd170bce38d3f732339bc13dd994a09260",
      "lineCount": 160,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
        "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
      ],
      "reviewNotes": {
        "callers": "The readiness and dismissal helpers are consumed by menu tests and setup drivers; the declared regression is implemented in Controls.Tests.MenuPopupReadiness.cpp.",
        "testIds": [
          "TestMenuPopupDiscoveryRejectsHiddenInitializationWindows",
          "TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu"
        ],
        "failureAndCancellation": "WaitForOwnedContextMenuPopupWindow has a finite timeout; DismissOwnedContextMenuPopupOnce uses SendMessageTimeout and waits briefly for destruction. The failure guard keeps dismissing late-created menus for a bounded eight seconds when a driver fails.",
        "ownershipAndLifetime": "Discovery requires a same-process, visible popup whose GW_OWNER matches the requested owner. The helpers close only discovered owned popups and bound repeated dismissal attempts; DismissMenusIfDriverFails ties cleanup to the driver's failure string.",
        "testCoverage": "Direct case TestMenuPopupDiscoveryRejectsHiddenInitializationWindows validates hidden-to-visible readiness on a private desktop. TestMenuDriverThatFailsBeforeItsPopupComesUpStillClosesTheMenu exercises late popup cleanup. No tests were executed."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.NativeDevicePolicy.h",
      "sha256": "9f39b7b78d703eedde6ed348a4521c730e3709dab5566fdd8244e3895b9061fb",
      "lineCount": 76,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss"
      ],
      "reviewNotes": {
        "callers": "The test runner invokes TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss, defined inline in this header; it runs on a worker initialized for COM and constructs WARP-backed host fixtures.",
        "testIds": [
          "TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss"
        ],
        "failureAndCancellation": "The test captures worker exceptions in exception_ptr and rethrows after join. It checks conflicting attach policy is refused without losing attachment accounting, then verifies device-loss recovery retains WARP and releases the final attachment.",
        "ownershipAndLifetime": "The test owns a GraphicsFixture, shared GraphicsDevice pool, two AttachedHostWindow instances, and a worker thread. WIL scope_exit balances CoUninitialize, and both hosts detach before the worker exits.",
        "testCoverage": "Exact case: TestWindowHostExplicitWarpPolicyRejectsConflictsAndSurvivesDeviceLoss. This reviewer read the case but did not execute it."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextElementMovement.cpp",
      "sha256": "ec5045b6b4033482ed957fc8c9d0a3cfe59ac2df373c45cadbe18a5d4d31b247",
      "lineCount": 200,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextElementMovement.h",
      "sha256": "3bd8b34daa5f3fe3a02556e69a85a46f0ee6767664b5a313397d640f02b93c76",
      "lineCount": 2,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "RunTextElementMovementTests",
        "TestUnicode17BulkCharacterMovementOracle",
        "TestUnicode17CollapsedDeletionCorpus",
        "TestTextFieldInteriorAcpDeletion",
        "TestComboBoxInteriorAcpDeletion"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextElements.cpp",
      "sha256": "7878220931232e57fd05ea7b119fc45cb357622e021a144b23a833c024de9d06",
      "lineCount": 68,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "TestUnicode17DefaultGraphemeCorpus",
        "TestTextElementBoundsMalformedAndLongContext"
      ],
      "reviewNotes": {
        "callers": "Implements RunTextElementTests() and is registered in the Controls runner; exercises the shared text-element boundary APIs against UnicodeGraphemeCorpus.",
        "testIds": [
          "TestUnicode17DefaultGraphemeCorpus",
          "TestTextElementBoundsMalformedAndLongContext"
        ],
        "failureAndCancellation": "Checks every UTF-16 offset for next/previous movement, nearest-boundary caret snapping, full 766-case corpus count, malformed inputs, long extender context and forward progress. No cancellation path applies.",
        "ownershipAndLifetime": "All corpus strings and expected boundaries are borrowed per-case data; malformed and embedded-NUL cases use explicit-length std::wstring values.",
        "testCoverage": "Exact cases: TestUnicode17DefaultGraphemeCorpus and TestTextElementBoundsMalformedAndLongContext. No tests were executed."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.TextElements.h",
      "sha256": "f8b224508bb27808d648bf58c158424c6252fc68c38f222bcb41d3ed0e49304a",
      "lineCount": 2,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
      "testIds": [
        "RunTextElementTests",
        "TestUnicode17DefaultGraphemeCorpus",
        "TestTextElementBoundsMalformedAndLongContext"
      ],
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
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.AnimationClock.cpp",
      "sha256": "401851eb15ddaef3469bd72e8cda656b24874033404a3fa6a0a7b5d518b2ba94",
      "lineCount": 501,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "Tests instantiate controls under EmbeddedHost or AttachedHostWindow and drive AdvanceAnimation or dispatcher ticks. ClockCase covers small, 32-bit rollover-adjacent, trillion-ms, near-int64, and uint64-max epochs; only focused native stall case uses real Sleep.",
        "ownershipAndLifetime": "GraphicsFixture, animation pool, host, root/control, and model owners are stack/unique_ptr scoped. Tests detach/re-attach explicitly, and native window helper detaches host before releasing HWND. No raw retained control beyond owning tree lifetime in reviewed tests.",
        "failureAndCancellation": "Assertions check transitions start at zero despite arbitrary epoch, settle without idle perpetual ticks, ignore hidden/zero-size/backward time, and continue across attachment without jumping. Native stall test uses dispatcher hitch clamp and observes a subsequent slider key transition; no cancellation-specific assertion.",
        "testCoverage": "Ten registered declarations provide deterministic embedded epoch coverage plus native dispatcher source and stall recovery. The stall test is wall-clock/dispatcher dependent and can be timing-sensitive; no build or runtime result is claimed."
      }
    },
    {
      "path": "Tests/Controls/Controls.Tests.AnimationClock.h",
      "sha256": "cffd8969ed58499c312f3ebb0805aaa9ff753f51bf8c58251c586c994756e12e",
      "lineCount": 12,
      "reviewStatus": "reviewed",
      "disposition": "source-read-and-reviewed-no-runtime-qualification",
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
      "reviewNotes": {
        "callers": "Declares ten animation-clock tests implemented by Controls.Tests.AnimationClock.cpp; DxUi.Tests.Runner.cpp invokes the suite. EmbeddedHost and WindowHost exercise the shared dispatcher timeline.",
        "ownershipAndLifetime": "Declarations carry no ownership; implementations create local GraphicsFixture, animation pool, EmbeddedHost, AttachedHostWindow, and tree/control models. Lifetimes are scope-bound; attached host helper detaches before HWND destruction.",
        "failureAndCancellation": "Tests assert skipped/hidden/zero-sized time does not advance embedded animation and backward time is ignored. Native dispatcher stall case sleeps a bounded multiple of hitch clamp; no explicit cancellation path is applicable.",
        "testCoverage": "Names cover hover, slider, caret, tree, grid glyph, tooltip, paused clock, reattachment monotonicity, native clock source, and post-stall transition. Actual execution not performed."
      }
    }
  ]
}
```
