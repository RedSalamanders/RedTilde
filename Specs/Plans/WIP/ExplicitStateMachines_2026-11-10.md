# Explicit state machines and lifetime safety across RedTilde

## Metadata

- **Status**: ACTIVE (Plan only — zero production code changes in this PR)
- **Date**: 2026-11-10
- **Baseline commit**: `36e9cbe87ce46fb132401b0554acf6397cd543ab` (`origin/main`)
- **Authoritative contracts to reconcile on closeout**:
  - [Specs/README.md](../../README.md)
  - [Specs/Core/Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Specs/Core/Core_FileSafety.md](../../Core/Core_FileSafety.md)
  - [Specs/Core/Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md)
  - [Specs/Editor/Editor_Markdown.md](../../Editor/Editor_Markdown.md)
  - [Specs/UI/UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
  - [Specs/Testing/Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [Specs/Testing/Testing_Performance.md](../../Testing/Testing_Performance.md)
  - [Specs/Testing/Testing_RunEvidence.md](../../Testing/Testing_RunEvidence.md)
- **Companion active plan**: [RemainingWork_2026-11-04.md](RemainingWork_2026-11-04.md) (owner qualification and delivery gates remain separate and explicit)

---

## Context and motivation

An organization-wide audit of implicit state machines across the `RedSalamanders` repositories identified **13 implicit state machines** in RedTilde (`11` native C++ machines across `RedTilde/` and `Common/` plus `2` PowerShell build/evidence machines under `Build/`) and **9 concrete state-transition defects** (`RT-SM-01` through `RT-SM-09`) where boolean tuples, split `std::optional` caches, and uncoordinated worker-drain predicates permit impossible or stale states.

Replacing ad-hoc boolean tuples with explicit `enum class` phases, payload-carrying `std::variant` file transactions, unified cache structs, and single transition/invariant helpers elevates all three engineering dimensions without changing RedTilde's external behavior, file formats, settings schema, or pinned `DxUi.lib` ABI:

1. **User Experience (UX)**: Eliminates overlapping modal surfaces (`recoveryPanel` simultaneously visible with `findBar` or `drawer`), prevents unrecoverable folder-pane `failed` latches after transient directory errors, closes stale `openingSnapshot`/`openingApprovals` retention after failed opens, fixes redundant find-match recomputation when reopening the Find bar, and prevents double-submission of recovery candidate discards.
2. **Developer Experience (DX)**: Collapses 7 scattered lifecycle booleans, 10 file-transaction optionals/vectors, and 23 Find & Replace synchronization fields in [Application.h](../../../RedTilde/Application.h) into self-documenting value types whose legal transitions are enforced in one place.
3. **Agent Experience (AX)**: Makes legal phases, admission predicates (`CanAcceptCommand()`, `AreShutdownWorkersSettled()`), and per-document cache boundaries machine-readable from header declarations alone instead of requiring cross-file reconstruction across `3,500+` lines of [Application.cpp](../../../RedTilde/Application.cpp) and [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp).

---

## Non-breaking invariants

Every implementation slice executing this plan MUST preserve all repository rules in [AGENTS.md](../../../AGENTS.md):

1. **Zero external regressions**: Preserve UTF-8 BOM and EOL fidelity, staged atomic saves, symlink target following, multi-hardlink durable recovery copies, external conflict detection (`ERROR_REVISION_MISMATCH`), source ACP selection, undo history, IME composition, keyboard focus, and UI Automation behavior.
2. **Pinned DxUi boundary**: Consume `DxUi.lib` strictly at the exact commit and `apiRevision` in `Dependencies/DxUi.lock.json`. Shared control state machines inside `DxUi` belong to the companion `RedSalamanders/DxUi` plan (`Specs/Plans/WIP/ExplicitStateMachines_2026-11-10.md` in `https://github.com/RedSalamanders/DxUi`).
3. **Exception, WIL, and warning discipline**: Keep `/Wall /permissive- /sdl /utf-8` with zero warnings across all six `(x64, ARM64) x (Debug, Release, ASan Debug)` configurations. WIL owns all Windows and COM handles; no exceptions cross Win32 or COM callbacks; allocation failure at application boundaries remains fatal.
4. **Zero-allocation hot paths and event-blocked idle**: Steady-state rendering, composition, and idle waiting must never busy-poll or introduce unbounded heap churn. Retain paired performance comparisons under [Specs/Testing/Testing_Performance.md](../../Testing/Testing_Performance.md) before claiming non-regression.
5. **Truthful capability and qualification status**: Keep [capabilities.json](../../../capabilities.json) truthful; cross-compilation never substitutes for native ARM64 execution, and headless harness tests never substitute for owner-authorized interactive desktop focus tests.

---

## Verified defect and implicit state machine ledger

### Concrete defects to close (`RT-SM-01` .. `RT-SM-09`)

| ID | Severity | Location | Verified failure mode | Slice |
| --- | --- | --- | --- | --- |
| `RT-SM-01` | High | [Application.cpp](../../../RedTilde/Application.cpp) (`BeginClose` L2829, `FinishClose` L2854 vs `DispatchUiMessage` L3503) | `BeginClose()` and `FinishClose()` define worker settlement as `!preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy()`, whereas `DispatchUiMessage()` omits `!bitmapWriter.Busy()` and synchronously sends `SendMessageW(window.get(), PreferencesReady, 0, 0)` on every dispatched UI message while `bitmapWriter` is still busy during close. | Slice 1 |
| `RT-SM-02` | Medium | [Application.cpp](../../../RedTilde/Application.cpp) (`Initialize` L329–357, `AskThen` L2300, `Save` L2397) | Lifecycle guard clauses across `>25` entry points test inconsistent subsets of `(initialized, initializing, closing, closingAfterSave, sessionEnding, switchingDocument, unsavedPromptOpen)`; `initialized` and `initializing` are simultaneously `true` during `Initialize()`, `Save()` omits `closingAfterSave` and `sessionEnding`, and `AskThen()` omits `closing`, `closingAfterSave`, and `sessionEnding`. | Slice 1 |
| `RT-SM-03` | High | [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) (`OpenPath` L527–529) & [Application.cpp](../../../RedTilde/Application.cpp) (`CompleteFileOperation` L2604–2609, `WM_CLOSE` L2939–2947) | Failed `files.Submit()` in `OpenPath()` clears `openingFromFiles` without resetting `openingSnapshot`; non-startup load failures in `CompleteFileOperation()` leave `openingSnapshot` and `openingApprovals` populated; `WM_CLOSE` open-cancellation resets `openingSnapshot`, `heldOpen`, and `openingFromFiles` without clearing `openingApprovals` or `conflictReload`. | Slice 2 |
| `RT-SM-04` | Medium | [AsyncFiles.h](../../../RedTilde/AsyncFiles.h) (`Worker` L111, `Close` L129–137, `Saving` L150) & [BitmapPaste.h](../../../RedTilde/BitmapPaste.h) (`Stop` L255–275) | `AsyncFiles::Worker` leaves `busy = true` and `saving = true` after disk I/O finishes until `Take()`, making `Saving()` report `true` for already-finished writes and forcing callers (`AbandonStartupRestore` L433, `WM_CLOSE` L2939) to destroy/re-allocate `state` via `Close(); Initialize(...)`. `BitmapPasteWriter::Stop()` synthesizes a fake `leased = true` state under the lock to route unconsumed results through `Discard()`. | Slice 2 |
| `RT-SM-05` | High | [Recovery.cpp](../../../Common/Recovery.cpp) (`Worker` L308–310 vs L383) & [Application.cpp](../../../RedTilde/Application.cpp) (`DiscardRecovery` L1747–1757) | After initial `Scan()`, `RecoveryService::State::Worker` sets `self->settled = self->pending.empty()` without checking `self->discards.empty()`, briefly signaling `settlement` while a queued discard has not executed. On the UI side, `Command::DiscardRecovery` leaves the candidate at `recoveryCandidates.front()` with the panel interactive until `RecoveryReadyMessage` arrives, allowing duplicate `Discard` or `Restore` submissions for the same candidate ID. | Slice 3 |
| `RT-SM-06` | High | [Files.cpp](../../../Common/Files.cpp) (`Fail` L435–452, watch rescan L481 & L524–528) & [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) (`RefreshFiles` L189–195) | When `FilesService::State::Worker` latches `failed = true` and exits, `Application::folderFilesActive` stays `true`; selecting a new valid folder via `SelectFilesRoot` sees `folderFilesActive == true`, skips `folderFiles.Initialize()`, and calls `SetRoot()` on the dead worker forever. In addition, watch-coalesced rescans set `scan = true` before setting `busy = true` on the next loop iteration. | Slice 3 |
| `RT-SM-07` | Medium | [Application.cpp](../../../RedTilde/Application.cpp) (`ExecuteCommand` L1980–1982, L2014–2017, `WM_PAINT` L3084–3093) | `Command::Find` and `Command::Settings` hide `floating` and each other without hiding `recoveryPanel`, allowing two modal/overlay surfaces to remain visible simultaneously. Deferred `pendingSelectionTools` and `pendingBlockPlacement` flags mutate control visibility/layout from inside `HandleMessage(WM_PAINT)`. | Slice 4 |
| `RT-SM-08` | Medium | [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) (`RefreshFind` L541–547, L580–589) | When `findBar` reopens, `RefreshFind()` swaps `closedFindMatches` back into `findMatches` and then resets `closedFindSource` and `closedFindQuery` instead of restoring `findSource`, `findQuery`, and `findCachedMatchCase`. Consequently `!findSource` at L587 is unconditionally `true`, discarding the swapped matches and forcing `CollectSourceFindMatches` on every reopen. | Slice 5 |
| `RT-SM-09` | High | [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) (`SwitchWorkspaceDocument` L417–436) & [Application.cpp](../../../RedTilde/Application.cpp) (`RefreshDocumentAnalysis` L859) | In `SwitchWorkspaceDocument()`, `primary->ExchangeDocumentState(document.state)` mutates the live editor at L417 before checking `if (!current()) return true;` at L424, which on reentrant teardown returns early before swapping `documentPath`, `savedText`, `stamp`, `collapsedHeadings`, `bom`, `recovered`, and `activeDocumentKey`. Separately, `previewCollapsedHeadings` and `previewCommittedOutline` are split optionals that overwrite live `collapsedHeadings` during transient diverged IME previews. | Slice 5 |

### Complete inventory of the 13 implicit state machines

1. **Application lifecycle, window teardown & close-during-save** ([Application.h](../../../RedTilde/Application.h) L264, L343, L381–382, L416–418; [Application.cpp](../../../RedTilde/Application.cpp) L241–254, L329–358, L2803–3049, L3491–3508).
2. **Document open / save / conflict-reload / startup-restore / workspace-approval** ([Application.h](../../../RedTilde/Application.h) L262, L378–380, L416, L420, L425, L429–438; [Application.cpp](../../../RedTilde/Application.cpp) L337–356, L425–442, L1166–1186, L2298–2699; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L503–531).
3. **`AsyncFiles` single-slot worker coordination** ([AsyncFiles.h](../../../RedTilde/AsyncFiles.h) L46–199).
4. **Clipboard bitmap paste (`BitmapPasteWriter` & `pendingBitmapPaste`)** ([BitmapPaste.h](../../../RedTilde/BitmapPaste.h) L40–290; [Application.h](../../../RedTilde/Application.h) L329–339; [Application.cpp](../../../RedTilde/Application.cpp) L2700–2766).
5. **`RecoveryService` debounce/settlement & UI candidate disposition** ([Recovery.cpp](../../../Common/Recovery.cpp) L221–575; [Application.h](../../../RedTilde/Application.h) L260–263, L349–352; [Application.cpp](../../../RedTilde/Application.cpp) L1097–1208, L1740–1828).
6. **Folder enumeration & directory watch (`FilesService` & `DirectoryWatch`)** ([Files.cpp](../../../Common/Files.cpp) L213–672; [Application.h](../../../RedTilde/Application.h) L267–269, L359, L383–384; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L85–355).
7. **Contextual overlays, `blockDraft` vs `SemanticSourceRegion`, and deferred placement** ([Application.h](../../../RedTilde/Application.h) L324, L343–355, L409–415; [Application.cpp](../../../RedTilde/Application.cpp) L1255–1582, L1925–2163, L3084–3093; [Application.Chrome.cpp](../../../RedTilde/Application.Chrome.cpp) L853–906; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L818–855).
8. **Find & Replace match table and 4 UI projections** ([Application.h](../../../RedTilde/Application.h) L299–312, L324; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L50–84, L532–777).
9. **Document analysis & transient IME/preview outline collapse** ([Application.h](../../../RedTilde/Application.h) L390, L402–404; [Application.cpp](../../../RedTilde/Application.cpp) L805–893; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L43–49, L440–446).
10. **Multi-document workspace session swap & reentrant UI/UIA publication caches** ([Application.h](../../../RedTilde/Application.h) L256, L284–285, L321–323, L360–408; [Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp) L393–490; [Application.cpp](../../../RedTilde/Application.cpp) L894–945, L1047–1095, L1398–1451).
11. **Auxiliary worker & diagnostic machines (`RichImageLoader`, `SettingsWriter`, `ExternalLinkState`, `EditDiagnostics`)** ([ImageLoader.h](../../../RedTilde/ImageLoader.h) L183–306; [Settings.cpp](../../../Common/Settings.cpp) L206–302; [Application.cpp](../../../RedTilde/Application.cpp) L180–200, L492–693; [EditDiagnostics.h](../../../RedTilde/EditDiagnostics.h) L41–235).
12. **Scoped-testing planner & receipt baseline progression** ([ScopedTesting.psm1](../../../Build/ScopedTesting.psm1)).
13. **Per-run evidence directory lifecycle (`Active` -> `Completed` / `Failed` / `Interrupted`)** ([TestRunEvidence.psm1](../../../Build/TestRunEvidence.psm1)).

---

## Target architecture (Patterns A–D)

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    Uninitialized --> Initializing : Initialize()
    Initializing --> Active : Initial Layout() complete
    Active --> ClosingAfterSave : WM_CLOSE while AsyncFiles::Phase::Saving
    ClosingAfterSave --> Active : Save failed or canceled (ResumeAfterSaveClose)
    ClosingAfterSave --> DrainingForClose : Save succeeded -> BeginClose()
    Active --> DrainingForClose : WM_CLOSE -> BeginClose()
    Active --> SessionEnding : WM_ENDSESSION
    DrainingForClose --> Destroyed : AreShutdownWorkersSettled() -> FinishClose()
    SessionEnding --> Destroyed : Recovery WaitForSettlement(2000) complete
```

- **Pattern A — Flat `enum class` + transition helper** (zero heap overhead, single source of truth):
  - `Application::LifecyclePhase`: `Uninitialized`, `Initializing`, `Active`, `ClosingAfterSave`, `DrainingForClose`, `SessionEnding`, `Destroyed`, paired with private `CanAcceptCommand()`, `IsUiAttached()`, and `AreShutdownWorkersSettled()` (`!preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy()`).
  - `AsyncFiles::Phase`: `Idle`, `Loading`, `Saving`, `Completed`, `Closed` — `Saving()` returns `phase == Phase::Saving`; `Busy()` returns `phase == Phase::Loading || phase == Phase::Saving || phase == Phase::Completed`; `CancelLoad()` returns an idle-ready state without heap re-allocation.
  - `BitmapPasteWriter::Phase`: `Idle`, `Encoding`, `Ready`, `Leased`, `Retiring`, `Stopped`, with dedicated encode vs retire worker entry points instead of multiplexing `job->discard` through synthesized `leased = true`.
  - `RecoveryService::WorkerPhase` (`Scanning`, `Running`, `Stopping`, `Finished`) + single `UpdateSettlementLocked()` invariant (`ready && pending.empty() && discards.empty() && !writingBatch`), plus per-candidate `Disposition` (`PendingOffer`, `Discarding`, `Restoring`) on the UI side.
  - `FilesService::Phase` (`Idle`, `Scanning`, `Watching`, `CoalescingWatch`, `Failed`, `Closed`), eliminating `Application::folderFilesActive` in favor of querying `folderFiles.Phase()` and re-initializing cleanly from `Phase::Failed` when the user chooses a new root.
  - `ExternalLinkState::Phase` (`Idle`, `ConfirmingPrompt`, `LaunchingSta`, `CompletedPendingDrain`, `Closed`) and `EditDiagnostics::SelectionPhase` (`Disarmed`, `Armed`, `InInvocation`, `InComponent`, `Invalid`).
- **Pattern B — `std::variant` transaction sum types** (data exists if and only if its state is active):
  - `StartupRestoreState`: `Disabled`, `WaitingForRecovery`, `OpeningLastDocument`, `Completed` (holding `startupSource` only while `WaitingForRecovery` or `OpeningLastDocument`).
  - `ActiveFileTransaction`: `std::variant<std::monostate, StartupOpenTx, PickerOpenTx, FilesPaneOpenTx, HeldOpenTx, SaveTx, ConflictReloadTx>` plus an explicit `ModalPromptKind { None, UnsavedChanges, SaveConflict, ExternalLink }` instead of overloading `unsavedPromptOpen`. Resetting `activeFileTx = std::monostate{}` on failure or cancel atomically destroys `openingSnapshot`, `openingApprovals`, `heldOpen`, and `conflictReload`.
- **Pattern C — Exclusive overlay & consolidated feature controllers**:
  - `OverlayKind`: `None`, `FloatingSelectionTools`, `FindBar`, `SettingsDrawer`, `BlockDraftModal`, `RecoveryModal` governed by `SetActiveOverlay(OverlayKind)` so `recoveryPanel` cannot stay visible behind `findBar` or `drawer`, paired with `std::optional<BlockDraftSession>` and post-prepare placement completion outside `WM_PAINT`.
  - `FindController`: single `MatchTable` (`source`, `query`, `matchCase`, `matches`) retained across Find-bar close/reopen for `F3`/`Shift+F3` (deleting duplicate `closedFind*` fields and closing `RT-SM-08`), plus `Projections` (`sectionCounts`, `highlights`, `statusLabel`, `outlineBadges`).
- **Pattern D — Atomic workspace session bundle & `ReentrantPublication<T>`**:
  - Extract `DocumentSessionMetadata` (`documentPath`, `savedText`, `stamp`, `collapsedHeadings`, `activeDocumentKey`, `bom`, `recovered`) shared by `Application` and `WorkspaceDocument`, swapped unconditionally alongside `primary->ExchangeDocumentState(...)` before any reentrant UI callback, plus `DerivedDocumentCaches::Reset()` and `std::optional<OutlinePreviewBaseline>`.
  - Extract `ReentrantPublication<T>` encapsulating the monotonic request-ID invalidation protocol shared by `appliedCaption`, `currentBlockPublishedFacts`, `listSummaryFacts`, and `selectionCaptionPublishedEquationContext`.

---

## Ordered execution slices

Each slice is independently buildable, testable, warning-free, and scoped so bisecting a regression isolates one subsystem.

### Slice 1 — `Application::LifecyclePhase` & unified worker-drain settlement (`RT-SM-01`, `RT-SM-02`)

- **Files**:
  - [RedTilde/Application.h](../../../RedTilde/Application.h)
  - [RedTilde/Application.cpp](../../../RedTilde/Application.cpp)
  - [Tests/Application.Tests.Close.cpp](../../../Tests/Application.Tests.Close.cpp)
  - [Specs/Core/Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Specs/Core/Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md)
- **Changes**:
  1. Introduce `enum class LifecyclePhase : std::uint8_t { Uninitialized, Initializing, Active, ClosingAfterSave, DrainingForClose, SessionEnding, Destroyed }` and `TransitionLifecycle(LifecyclePhase)` on `Application`.
  2. Replace `initialized`, `initializing`, `closing`, `closingAfterSave`, `waitingForPreferences`, and `sessionEnding` with `lifecyclePhase` (retaining `visibleBeforeSaveClose` inside close-after-save state).
  3. Extract `AreShutdownWorkersSettled() const noexcept` checking `!preferenceWriter.Busy() && recovery.Finished() && !bitmapWriter.Busy()` and call it from `BeginClose()`, `FinishClose()`, and `DispatchUiMessage()` (closing `RT-SM-01`).
  4. Replace ad-hoc subsets of lifecycle guards in `Save()`, `AskThen()`, `Open()`, `ExecuteCommand()`, and `HandleMessage()` with `CanAcceptCommand()` / `IsUiAttached()` (closing `RT-SM-02`).
- **Tests**: Add deterministic close-drain regression in [Application.Tests.Close.cpp](../../../Tests/Application.Tests.Close.cpp) where `preferenceWriter` and `recovery` are already settled while `bitmapWriter.Busy()` remains `true`, proving `DispatchUiMessage()` sends zero spurious `PreferencesReady` messages until `BitmapReady` settles.

### Slice 2 — `AsyncFiles::Phase`, `BitmapPasteWriter::Phase`, and `ActiveFileTransaction` (`RT-SM-03`, `RT-SM-04`)

- **Files**:
  - [RedTilde/AsyncFiles.h](../../../RedTilde/AsyncFiles.h)
  - [RedTilde/BitmapPaste.h](../../../RedTilde/BitmapPaste.h)
  - [RedTilde/Application.h](../../../RedTilde/Application.h)
  - [RedTilde/Application.cpp](../../../RedTilde/Application.cpp)
  - [RedTilde/Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp)
  - [Tests/Application.Tests.Files.cpp](../../../Tests/Application.Tests.Files.cpp)
  - [Tests/Application.Tests.Close.cpp](../../../Tests/Application.Tests.Close.cpp)
  - [Specs/Core/Core_FileSafety.md](../../Core/Core_FileSafety.md)
- **Changes**:
  1. Refactor `AsyncFiles::State` to `Phase { Idle, Loading, Saving, Completed, Closed }` so `Worker` transitions `Loading`/`Saving` -> `Completed` on disk completion (`Saving()` is `false` once disk write finishes) and `CancelInFlightLoad()` detaches an in-flight reader cleanly.
  2. Refactor `BitmapPasteWriter::State` to `Phase { Idle, Encoding, Ready, Leased, Retiring, Stopped }` so `Stop()` transitions `Ready` -> `Retiring` directly under the mutex without faking `leased = true`.
  3. Replace loose `startupPending`, `startupOpening`, `startupSource`, `openingSnapshot`, `openingFromFiles`, `openingApprovals`, `heldOpen`, `afterSave`, `conflictReload`, and overloaded `unsavedPromptOpen` with `StartupRestoreState`, `ActiveFileTransaction`, and `ModalPromptKind`.
- **Tests**: Add unit/harness cases in [Application.Tests.Files.cpp](../../../Tests/Application.Tests.Files.cpp) proving failed `OpenPath()` submission, failed async load completion, and `WM_CLOSE` cancellation leave zero retained `openingSnapshot`, `openingApprovals`, or `conflictReload` state.

### Slice 3 — `RecoveryService` settlement invariant & `FilesService` failure recovery (`RT-SM-05`, `RT-SM-06`)

- **Files**:
  - [Common/Recovery.h](../../../Common/Recovery.h)
  - [Common/Recovery.cpp](../../../Common/Recovery.cpp)
  - [Common/Files.h](../../../Common/Files.h)
  - [Common/Files.cpp](../../../Common/Files.cpp)
  - [RedTilde/Application.h](../../../RedTilde/Application.h)
  - [RedTilde/Application.cpp](../../../RedTilde/Application.cpp)
  - [RedTilde/Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp)
  - [Tests/Application.Tests.Files.cpp](../../../Tests/Application.Tests.Files.cpp)
  - [Specs/Core/Core_FileSafety.md](../../Core/Core_FileSafety.md)
- **Changes**:
  1. In `RecoveryService::State`, introduce `WorkerPhase { Scanning, Running, Stopping, Finished }` and `UpdateSettlementLocked()` so `settled` and `settlement` require `ready && pending.empty() && discards.empty() && !writingBatch` at every transition, including post-`Scan()`.
  2. Track `RecoveryCandidate` UI disposition (`Available`, `Discarding`) so `Command::DiscardRecovery` immediately marks/advances the front candidate and ignores duplicate clicks before `RecoveryReadyMessage`.
  3. In `FilesService::State`, introduce `Phase { Idle, Scanning, Watching, CoalescingWatch, Failed, Closed }`, mark `Phase::Scanning` atomically when watch coalescing schedules a rescan, and update `Application::RefreshFiles()` to re-initialize `folderFiles` when `folderFiles.IsFailed()` instead of latching behind `folderFilesActive`.
- **Tests**: Add tests in [Application.Tests.Files.cpp](../../../Tests/Application.Tests.Files.cpp) covering concurrent `Discard()` during initial `Scan()`, duplicate `DiscardRecovery` commands, and recovery of the Files pane when switching from a failed directory root to a valid directory root.

### Slice 4 — Exclusive `OverlayKind` surface controller & post-prepare placement (`RT-SM-07`)

- **Files**:
  - [RedTilde/Application.h](../../../RedTilde/Application.h)
  - [RedTilde/Application.cpp](../../../RedTilde/Application.cpp)
  - [RedTilde/Application.Chrome.cpp](../../../RedTilde/Application.Chrome.cpp)
  - [RedTilde/Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp)
  - [Tests/Application.Tests.DesignRefresh.cpp](../../../Tests/Application.Tests.DesignRefresh.cpp)
  - [Specs/UI/UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
- **Changes**:
  1. Introduce `OverlayKind { None, FloatingSelectionTools, FindBar, SettingsDrawer, BlockDraftModal, RecoveryModal }` and `SetActiveOverlay(OverlayKind)` enforcing mutual exclusion across all five overlay `Surface` controls (fixing `Command::Find` and `Command::Settings` while `recoveryPanel` is visible, while retaining the unrecovered candidate list so the user can reopen or remain dirty-safe).
  2. Group `blockDraftSnapshot`, `blockDraftRange`, `blockDraftRevision`, and `pendingBlockPlacement` into `std::optional<BlockDraftSession>`.
  3. Move deferred `pendingSelectionTools` / `pendingBlockPlacement` geometry resolution out of `HandleMessage(WM_PAINT)` into the pre-render `Prepare()` pass so `WM_PAINT` never mutates control visibility or triggers re-layout.
- **Tests**: Add headless harness assertions in [Application.Tests.DesignRefresh.cpp](../../../Tests/Application.Tests.DesignRefresh.cpp) verifying `Command::Find` and `Command::Settings` dismiss `recoveryPanel` cleanly without overlapping surfaces and preserve focus/Escape restoration.

### Slice 5 — `FindController`, `DocumentSessionMetadata` atomic swap, `OutlinePreviewBaseline`, and `ReentrantPublication<T>` (`RT-SM-08`, `RT-SM-09`)

- **Files**:
  - [RedTilde/Application.h](../../../RedTilde/Application.h)
  - [RedTilde/Application.cpp](../../../RedTilde/Application.cpp)
  - [RedTilde/Application.Navigation.cpp](../../../RedTilde/Application.Navigation.cpp)
  - [RedTilde/ImageLoader.h](../../../RedTilde/ImageLoader.h)
  - [RedTilde/EditDiagnostics.h](../../../RedTilde/EditDiagnostics.h)
  - [Common/Settings.cpp](../../../Common/Settings.cpp)
  - [Tests/Application.Tests.Core.cpp](../../../Tests/Application.Tests.Core.cpp)
  - [Tests/Application.Tests.RemoteImages.cpp](../../../Tests/Application.Tests.RemoteImages.cpp)
  - [Specs/Editor/Editor_Markdown.md](../../Editor/Editor_Markdown.md)
- **Changes**:
  1. Extract `FindController` with a single `MatchTable` shared across open Find bar and closed `F3`/`Shift+F3` navigation, deleting `closedFindMatches`, `closedFindSource`, `closedFindQuery`, and `closedFindCase` and eliminating the reopen `activity.findMatchRebuilds` regression (`RT-SM-08`).
  2. Extract `DocumentSessionMetadata` and `DerivedDocumentCaches` so `SwitchWorkspaceDocument()` swaps document metadata atomically with `primary->ExchangeDocumentState(document.state)` before any `if (!current())` check (`RT-SM-09`).
  3. Replace `previewCollapsedHeadings` + `previewCommittedOutline` with `std::optional<OutlinePreviewBaseline>` without clobbering committed collapse sets.
  4. Extract `ReentrantPublication<T>` for the four counter-guarded UI/UIA string publications, and replace boolean tuples in `ExternalLinkState` and `EditDiagnostics` with explicit `enum class` phases.
- **Tests**: Assert `activity.findMatchRebuilds` does not increment when closing and reopening `findBar` on an unchanged document/query in [Application.Tests.Core.cpp](../../../Tests/Application.Tests.Core.cpp), and test workspace switch atomicity under reentrant UI callbacks.

### Slice 6 — Scoped-testing and run-evidence state clarity (PowerShell tooling)

- **Files**:
  - [Build/ScopedTesting.psm1](../../../Build/ScopedTesting.psm1)
  - [Build/TestRunEvidence.psm1](../../../Build/TestRunEvidence.psm1)
  - [Tests/Tooling.Tests.ScopedTesting.ps1](../../../Tests/Tooling.Tests.ScopedTesting.ps1)
  - [Tests/Tooling.Tests.RunEvidence.ps1](../../../Tests/Tooling.Tests.RunEvidence.ps1)
  - [Specs/Testing/Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [Specs/Testing/Testing_RunEvidence.md](../../Testing/Testing_RunEvidence.md)
- **Changes**: Document and enforce explicit receipt disposition enums (`BaselineAdvanced`, `PartialRunRetained`, `FailedRetained`, `MutatedWorktreeRetained`, `CiPending`) in [ScopedTesting.psm1](../../../Build/ScopedTesting.psm1) and explicit run-record lifecycle transitions (`Active` -> `Completed` | `Failed` | `Interrupted`) in [TestRunEvidence.psm1](../../../Build/TestRunEvidence.psm1) while keeping on-disk JSON schemas backward-compatible.
- **Tests**: Extend [Tooling.Tests.ScopedTesting.ps1](../../../Tests/Tooling.Tests.ScopedTesting.ps1) and [Tooling.Tests.RunEvidence.ps1](../../../Tests/Tooling.Tests.RunEvidence.ps1).

---

## Validation and qualification gates

### Routine validation for every implementation slice

```powershell
pwsh -NoProfile -File .\validate.ps1
pwsh -NoProfile -File .\Tools\Test-Changelog.ps1 -Base origin/main
pwsh -NoProfile -File .\format.ps1 -Check
pwsh -NoProfile -File .\Test-Changes.ps1 -Explain
pwsh -NoProfile -File .\test.ps1
```

### Full local qualification before moving this plan to `Specs/Plans/Done/`

1. **All six configurations build with zero warnings**:
   - `.\build.ps1 -Platform x64 -Configuration Debug`
   - `.\build.ps1 -Platform x64 -Configuration Release`
   - `.\build.ps1 -Platform x64 -Configuration "ASan Debug"`
   - `.\build.ps1 -Platform ARM64 -Configuration Debug`
   - `.\build.ps1 -Platform ARM64 -Configuration Release`
   - `.\build.ps1 -Platform ARM64 -Configuration "ASan Debug"`
2. **All three x64 test configurations + real ASan probe**:
   - `.\test.ps1 -Platform x64 -Configuration Debug -Full`
   - `.\test.ps1 -Platform x64 -Configuration Release -Full`
   - `.\test.ps1 -Platform x64 -Configuration "ASan Debug" -Full`
3. **Paired performance evidence**: Capture before/after completed-frame and allocation pairs on identical fixtures per [Specs/Testing/Testing_Performance.md](../../Testing/Testing_Performance.md) (`FindController` reopen, workspace switch, and contextual overlay transitions must show equal or lower allocations/rebuilds).
4. **Explicitly gated checks (do NOT falsify or auto-run without authorization)**:
   - **Interactive desktop focus/clipboard suites**: Require explicit owner agreement to the desktop time before running.
   - **Native ARM64 runtime execution**: Cross-compilation on x64 proves compilation only; native ARM64 runtime claims remain gated on a physical ARM64 host.

---

## Stop conditions

Stop and ask the repository owner before proceeding if any slice encounters:

1. A behavioral change that would alter saved Markdown bytes, UTF-8 BOM/EOL preservation, recovery envelope JSON format, or `RedTilde.settings.json` schema.
2. Any requirement to modify `DxUi` headers or bump `Dependencies/DxUi.lock.json` inside an application state-machine slice (land upstream `DxUi` changes separately first).
3. Any measured steady-state frame-time, memory, or allocation regression on the performance fixtures in [Application.Tests.Performance.cpp](../../../Tests/Application.Tests.Performance.cpp).

---

## Closeout checklist

When all six slices and required qualification gates are complete:

1. Reconcile durable state-machine invariants into [Core_Architecture.md](../../Core/Core_Architecture.md), [Core_FileSafety.md](../../Core/Core_FileSafety.md), [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md), [Editor_Markdown.md](../../Editor/Editor_Markdown.md), [UI_DesignSystem.md](../../UI/UI_DesignSystem.md), [Testing_Validation.md](../../Testing/Testing_Validation.md), and [Testing_RunEvidence.md](../../Testing/Testing_RunEvidence.md).
2. Review user-facing docs and [capabilities.json](../../../capabilities.json) for accuracy; regenerate any affected deterministic harness captures if visual overlay behavior changed.
3. Move this file from `Specs/Plans/WIP/ExplicitStateMachines_2026-11-10.md` to `Specs/Plans/Done/ExplicitStateMachines_2026-11-10.md`, update [Specs/Plans/WIP/README.md](README.md) and [Specs/Plans/Done/README.md](../Done/README.md), and re-run `.\validate.ps1` and `.\Test-Changes.ps1 -Mode PrePush`.
