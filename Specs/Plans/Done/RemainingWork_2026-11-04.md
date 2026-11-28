# RedTilde remaining delivery and qualification

Status: DONE
Updated: 23 November 2026
Owner: RedTilde maintainers; shared controls stay with DxUi.

This plan closed with RedTilde `v1.0.0` (`23 November 2026`). Historical superseded queues remain in the
[consolidation record](WipConsolidation_2026-11-04.md). Post-1.0 feature work proceeds in
[Dictionary and spell checking](../WIP/DictionaryAndSpellChecking_2026-11-21.md); durable engineering rules stay
in domain contracts under `Specs/Core`, `Specs/Editor`, `Specs/UI`, `Specs/Build` and `Specs/Testing`.

The [Windows printing workstream](#t-u2) covers print preview, page selection and installed destinations.

The owner's **7 November 2026** delivery scope requires x64 Debug, Release and ASan Debug runtime tests and
ARM64 compilation in all three profiles. ARM64 runtime is explicitly set aside for this delivery. Compilation
does not establish runtime acceptance or change the normal two-architecture production release gate.

## Verified starting point

Reviewed main: `1cd59c08528cc1e20720a45ef833bb9e69c315e4` ([PR44](https://github.com/RedSalamanders/RedTilde/pull/44)).
The starting consumer lock was DxUi API 4 at `703a96c788a79c1e7b28097226a2ca0a735f11bc`, retained on
upstream main through [DxUi PR163](https://github.com/RedSalamanders/DxUi/pull/163). The current quote-margin
adoption uses published API-4 commit [`8c6c2818`](https://github.com/RedSalamanders/DxUi/commit/8c6c2818d084e6124fb2c08eddc00bdba7a9d2aa),
including the sequence-label Debug allocation repair;
its x64 Release/Debug/ASan consumer checks pass in the [scoped closeout](QuoteMarginsAndScrollbars_2026-11-04.md).
Other qualification stays with T-Q1/T-Q2. Do not restore an older
SDK086/SDK702 development pin merely because a historical checklist names it.

The current diagram-label adoption advances API 4 explicitly to `2a7e7c28` in
[PR47](https://github.com/RedSalamanders/RedTilde/pull/47). Its
[scoped closeout](DiagramLabelBackgroundsAndClosePrompt_2026-11-04.md) records six builds, all 75 cases
and 36 captures in each x64 profile, the real ASan probe and the accepted duplicate-Close rule. Current-pin
native ARM64 Release also passes in the source-bound hosted run below. ARM64 Debug/ASan,
physical/interactive and performance acceptance remain separately owned.

The recovery Restore follow-up is implemented at `31d6f59e`, retaining that same API-4 pin. Its
[scoped closeout](RecoveryRestoreIdentity_2026-11-05.md) records six zero-warning builds, all 76 native
cases and 36 captures in each x64 profile, real ASan detection, the original wrong-copy counterexample and
the corrected hidden-wait synchronization. Hosted main `83e764a0` now passes native x64/ARM64 Release,
each with all 76 cases and 36 captures, plus both test-disabled production ZIPs and clean-extraction smoke.
[Verified original receipts and archive identities](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/hosted-qualification.json)
remain separate from the earlier 75-case `1c600d60` run and the later close-during-save candidate.

Close during Save is qualified at `15ac77cf`, still API 4 / `2a7e7c28`. Its
[scoped closeout](CloseDuringSave_2026-11-05.md) records six zero-warning builds, all 81 cases and
36 captures per x64 profile, real ASan detection, retained contract/lifecycle counterexamples and both original
whole-Editor timeouts. Current Editor execution preserves the full case union and each child's 90-second
watchdog. This candidate's native ARM64, physical, performance and release gates remain open.

RT-08 is qualified at `bc024dd8`, with the same pin: six zero-warning builds, all 82 cases and 36 captures
per x64 profile, real ASan detection and exact native/SDK/source closures. The
[scoped closeout](RecoveryEnvelopeBounds_2026-11-05.md) retains the parser termination and fixture
failures. Earlier Close main `94b9d431` passed hosted x64 but failed two native ARM64 Open fixture writes;
production was withheld. The repaired RT-08 source `3e1362d0` subsequently passed both native Release roles
and both production roles in [run 42024995782](https://github.com/RedSalamanders/RedTilde/actions/runs/42024995782);
[its exact hosted packet](../../../Measurements/RecoveryEnvelopeBounds_2026-11-05/hosted-qualification.json)
keeps that 82-case source distinct from later candidates.

| Completed scope | Evidence and limit |
| --- | --- |
| Persistent folder root, Open Folder/Open File, restart restoration and expanded public examples | [Folder/sample closeout](FolderNavigationAndSamples_2026-11-03.md); [packet](../../../Measurements/FolderNavigation_2026-11-03/README.md). |
| Native Mermaid display corrections and bounded sequence syntax expansion | Published exact pin above; [syntax packet](../../../Measurements/MermaidSyntax_2026-11-03/README.md). Full Mermaid parity is deferred. |
| Measured arrow spacing, table/code insets, accent task markers and persistent optional completed-task strike-through | [Comparison and original evidence](../../../Measurements/DiagramSpacing_2026-11-03/README.md), [publication receipt](../../../Measurements/DiagramSpacing_2026-11-03/publication.json). |
| Original PR44 x64 Release implementation checks and source publication | Zero-warning build, 73 native cases, complete noninteractive pre-push, 32 standard captures, and 30 original focused Light/Dark/DPI captures. PR44 checks passed. These are source-bound historical results; current source qualification is recorded below. |
| Design refresh, common build policy and independent RedMonitor client boundary | Scoped implementation is archived below. Their current-candidate performance, physical-input, platform and release gates remain open here. |

The approved narrow memory-cost disposition remains narrow. No functional pass, screenshot, PR merge or
ordinary CI success accepts R10, a measurement policy, quiet-host behavior or another candidate's performance.


Current workspace/rendering implementation: `fe9b7d15`, exact API-4 DxUi `8dc328de`. The [scoped closeout](WorkspaceRecoveryAndRendering_2026-11-06.md) records six zero-warning builds, all 91 cases and 36 captures in each x64 profile, real ASan detection, independent file/draft/settings failure tests and sixteen actual rendering captures. This closes the listed workspace, recovery admission, EOL, RT-06/09/14 and rendering adoption scopes; the remaining conflict/location, image-consent, performance and native/physical gates stay below.

Hosted main `75c0675b` now passes all seven Release jobs for that workspace/rendering scope: native x64/ARM64
each pass 91 cases, 36 captures and assertion/crash checks, followed by both production ZIP smoke tests.
[Verified original receipts, all thirteen wrapper digests and both package hashes](../../../Measurements/WorkspaceRecoveryAndRendering_2026-11-06/hosted-qualification.json)
close that source's Release gate.

The save-safety/UI boundary follow-up is locally qualified at `7a5ecc49`, exact API-4 DxUi `0ffaa1a6`:
six zero-warning builds, all 108 cases and 36 captures in each x64 profile, real ASan detection and 141 actual
compiled inputs. Its [scoped closeout](FileSafetyAndUiBoundaries_2026-11-06.md) retains independent
counterexamples and the first startup failure. Q8/Q12 and the other location, UI and qualification work remain open.

That exact save-safety source now passes all seven hosted Release jobs at main `a25f1835` in
[run 42165374140](https://github.com/RedSalamanders/RedTilde/actions/runs/42165374140): all 108 cases,
36 captures and the assertion probe on native x64/ARM64, plus both test-disabled production ZIP smoke checks.
The [verified original artifact packet](../../../Measurements/FileSafetyAndUiBoundaries_2026-11-06/hosted-qualification.json)
closes this source's hosted Release gate. ARM64 Debug/ASan, physical, performance and later SDK adoption remain separate.

## Execution order

T-U1's prepared diagram-label correction and T-S1's accepted duplicate-Close behavior have their scoped
closeout, following quote padding and themed scrollbars. Continue the remaining T-S1 work, followed by T-S2.
T-Q1 is the first **qualification** repair and can be
investigated independently. Do not start another redesign, repin the SDK or rerun unchanged failed studies
instead of resolving these owners. Every implementation slice updates its domain contract and has a
meaningful failing-then-passing regression where applicable.

<a id="t-s1"></a>
### T-S1 — recovery, shutdown and exception safety

- [x] Bind Restore to the exact snapshot clicked before entering a dirty-document prompt (RT-04/PR-06).
  Revalidate its id after the prompt/save; a missing copy leaves the current document and other candidates
  intact. [Code, counterexample, six builds and all three native x64 profiles](RecoveryRestoreIdentity_2026-11-05.md)
- [x] Complete Q4 / RT-02 / PR-05: hide while an admitted save settles, retain recovery until success,
  reshow with failure, protect later edits and abandon Open. Real I/O-barrier regressions, six builds and
  three complete x64 profiles pass in the [scoped closeout](CloseDuringSave_2026-11-05.md).
- [x] Implement immediate recovery submission on session end, bounded settlement and restart presentation
  (RT-01/PR-03, Q2); keep the 750 ms quiet debounce and enforce the approved five-second staleness cap
  under continuous typing (RT-10/PR-04, Q3).
- [x] Quarantine oversized checksum-valid recovery envelopes and retain complete original contents and
  earlier quarantine files on collision (RT-08). Independent counterexample, three Unicode classes, valid
  boundary, six builds and three complete x64 profiles pass in the [scoped closeout](RecoveryEnvelopeBounds_2026-11-05.md).
- [x] Implement and qualify Q18: keep the existing Save/Discard/Cancel prompt and ignore duplicate native
  or command Close (PR-34). The hidden regression covers Cancel and a fresh Discard in all three x64 profiles.
  [Scoped closeout](DiagramLabelBackgroundsAndClosePrompt_2026-11-04.md)
- [x] Unwind `updatingFind` after a rejected replacement and verify a subsequent valid replacement/Undo (RT-14); [scoped closeout](WorkspaceRecoveryAndRendering_2026-11-06.md).
- [x] Complete shortcut, menu, native-message and save-completion exception boundaries (RT-12, PR-07/35),
  including preserved pending completion and coherent saved facts. [Scoped closeout](FileSafetyAndUiBoundaries_2026-11-06.md).

Owner: `RedTilde/Application.cpp`, `Application.Navigation.cpp`, `Common/Recovery.cpp`, `RedTilde/AsyncFiles.h`.
Acceptance: deterministic Restore+Discard, close-during-save success/failure, continuous typing, logoff,
malformed envelope, second close and size-limit tests. See the retained
[Phase 1 detail](DeliveryRedTeam_2026-10-31.md#phase-1-acceptance).

<a id="t-s2"></a>
### T-S2 — lossless files and storage semantics

- [x] Preserve the document terminator for F2 Apply and insertion commands (RT-03 product half), hold Open
  completion while a local-source draft is open (RT-05), and keep a bad settings header on disk (RT-09).
- [x] Implement the approved single conflict surface: Reload, Save As, Overwrite, Cancel; distinguish failed
  readback from a real content conflict and avoid poisoning the next save stamp (RT-07, Q6).
  [Scoped closeout](FileSafetyAndUiBoundaries_2026-11-06.md).
- [x] Close staging handles before failure cleanup (RT-06); independent write/flush failure tests preserve bytes/stamps and leave no temporary file. [scoped closeout](WorkspaceRecoveryAndRendering_2026-11-06.md).
- [x] Document the achievable best-effort cross-process race contract (C6), recheck after staging and protect
  sharing/Save As aliases with complete native file identities and reliable successful stamps.
  [Scoped closeout](FileSafetyAndUiBoundaries_2026-11-06.md).
- [x] Implement the provider-unsupported replacement fallback (PR-09/10/31, Q8) through the durable
  recovery-protected writer. [Scoped closeout](ProviderSaveFallback_2026-11-07.md): four independent
  provenance/admission/failure/conflict cases, six zero-warning builds, 131 passing cases plus the actual
  symbolic-link privilege failure per x64 profile, hidden captures, restart recovery and real ASan detection.
  Exact main `3bf88036` additionally passes all six hosted native profiles (132 cases and 36 captures each),
  including all four provider cases and actual symbolic-link saves, followed by both production packages.
- [x] Qualify the replacement fallback, read sharing and non-name-surrogate cloud tags on real
  OneDrive/cloud and WSL locations before support claims (completed across all three declared backends on `2026-11-22` with `0` unexpected failures and `0` leftover `redtilde-run-*` directories: OneDrive `%USERPROFILE%\OneDrive\RedSalamander.Perf\selftest` passed `34/34` `Storage`, `19/19` `Settings`, `23/23` `Recovery`; two consecutive WSL 2 `9P` runs passed `23/23` `Recovery`, `17/19` `Settings`, `23/34` `Storage` plus `13` probed `9P` primitives after `VolumeRootFingerprint` and `MoveFileExW` atomic rename fallback; SMB/NAS `path.smb.primary` passed `19/19` `Settings`, `23/23` `Recovery`, `27/34` `Storage` plus `7` probed primitives: [RealLocationOneDriveWsl_2026-11-22](../../../Measurements/RealLocationOneDriveWsl_2026-11-22/README.md); prior SMB-only packet: [RealLocationSmb_2026-11-22](../../../Measurements/RealLocationSmb_2026-11-22/README.md); initial `ea111980` root-cause packet: [RealLocationSmb_2026-11-21](../../../Measurements/RealLocationSmb_2026-11-21/README.md)).
- [x] Preserve symlink/hard-link identities under the approved distinct write policies (Q12).
  Implementation `020cb0b9` preserves hardlink aliases with independent durable recovery and separately follows
  symbolic targets. [Local evidence](../../../Measurements/LinkedSaveAndCosmeticSettlement_2026-11-06/README.md)
  records six zero-warning builds, 116 passing cases per x64 profile and real mid-write crash/restart recovery.
  The local symlink fixture's Win32 1314 remains a failure. Actual hosted symbolic-link cases now pass on
  native x64/ARM64 Release and Debug and both ASan Storage partitions. The
  [scoped closeout](LinkedFileSaveSafety_2026-11-06.md) binds original receipts and package hashes.
  ARM64 ASan's later heap-fixture failure remains with T-Q1; no complete ASan cohort is claimed for it.
- [x] Expose specific unsupported-encoding/capacity errors under the current 65,536 UTF-16-unit limit (Q7).
  [Scoped closeout](FileSafetyAndUiBoundaries_2026-11-06.md).

Owner: `Common/Storage.cpp`, `Common/Settings.cpp`, application save/open paths and
[file safety](../../Core/Core_FileSafety.md). Real-share/WSL evidence is required before support claims.
RT-18 remains optional defensive hardening, not a newly confirmed defect.

<a id="t-u1"></a>
### T-U1 — remaining editing and product interaction

- [x] Complete the 5 November selected-block request: Insert Quote and Code Block transform the selected
  content through shared DxUi commands, preserve unselected text and original line endings, and commit one
  undoable edit in Compose, Source and Split. Retain the existing empty-selection templates and test actual
  application command dispatch; rejection never replaces selected text with a sample. Six builds, all 83
  cases and 36 captures per x64 profile pass in the [scoped closeout](SelectedBlockInsertions_2026-11-05.md).
- [x] Verify the new `graph BT`, class-inheritance LR, accent and gathered-matrix corrections against an
  exact qualified DxUi pin with native application captures; the prior scoped rendering fixes remain historical.
- [x] Keep each modified file while switching within the left-pane root without prompting. Prompt when
  explicitly opening a new file/folder or closing, and persist/recover every modified document after a crash
  and relaunch. Preserve per-document source/selection/history and saved identity through switching.
- [x] Adopt the shared contrast-aware diagram-label background correction and verify actual Light/Dark,
  96/192-DPI native captures at exact API-4 pin `2a7e7c28` without changing Mermaid source or history.
  [Original comparisons and exact qualification](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/README.md)
- [x] Adopt the shared quote/callout inner-margin correction and enable the native themed document scrollbar
  in Compose, Source and Split; cover overflow, paging/dragging, themes, DPI, narrow layout and source/history
  preservation with hidden native tests and captures. [Scoped closeout](QuoteMarginsAndScrollbars_2026-11-04.md)
  records the exact pin, all three x64 profiles, ASan detection and actual hidden captures.
- [x] Qualify Source Enter, native ACP capacity and quoted file-paste software through exact API-4
  `a1915ffd` (DX-3/DX-4 and the RT-03 shared half). The
  [local closeout](NativeEditingQualification_2026-11-07.md) records actual hidden-app EOL/preview/mirror
  paths, the public native store on the app-owned headless component, all six clean builds, 143 passing cases
  and 38 captures per x64 profile, and the actual local symlink privilege failure. No competing editor logic,
  OS clipboard or physical IME acceptance is implied. The subsequent hosted receipts verify all six native
  profiles with 144 passing cases and 38 captures each, including the actual symbolic-link case; see T-Q1.
- [x] Complete real-keyboard `Equation.F2InPlaceEditUndoAndExit` through the existing restoring desktop lease.
  All nine keyboard/focus stages pass at `31d6f59e` after explicit timing agreement and the visible 30-second
  countdown. Foreground/focus remain unchanged and the pointer is restored.
  [Named checks and exact receipt identities](KeyboardAndFocusQualification_2026-11-05.md)
- [x] Implement remaining Q9/Q14 window placement, work-area clamping, launch show state, Find case/repeat,
  confirmed explicit link opening, text-scale shortcuts and last-document/session startup policy.
  Last-folder restoration is already implemented; last-document/session startup is a separate requirement.
  [Scoped closeout](RemainingInteractions_2026-11-06.md): `2709f933` has six zero-warning local builds
  and all six hosted native Release/Debug/ASan combinations, each passing 128 cases and 36 captures.
  Actual symbolic-link saves, the repaired ARM64 ASan heap fixture and both production ZIPs pass. The local
  privilege failures and all original rejected cohorts remain retained. Physical input/AT and real browser
  association stay with T-Q2 rather than being inferred from injected-launcher tests.
- [x] Reconcile the current first-party review inventory and retained app/control findings (`RT-SM-01`..`RT-SM-09`
  closed in [ExplicitStateMachines_2026-11-10.md](ExplicitStateMachines_2026-11-10.md); `CloseDocument` `Ctrl+W`,
  `WM_DROPFILES`, `Open documents` selection, `Q20` capacity-limit reporting, `Command::Focus` session-local behavior,
  Settings About & Shortcuts, and `Specs/`/`docs/` reconciliation completed; owner-authorized interactive desktop focus
  and physical IME/AT/input gates remain open under `T-Q2`).

The approved Files root policy Q11 is completed by the folder-navigation slice; it is not a new task.
Details and original counterexamples remain in the
[delivery review](DeliveryReview_2026-10-31.md) and
[review inventory](RedesignCompletionAndRendering_2026-10-24/review-inventory.md).

<a id="t-u2"></a>
### T-U2 — Print the active Markdown document through Windows

RedTilde previews formatted pages. The preview owns destination, page selection and driver-supported job
options. Its Print action submits the reviewed pages. Physical sheets, another virtual driver and interactive
desktop focus remain open qualification cases.
The source pin adopts the qualified DxUi fix that draws native graphs directly on the editor canvas or paper.
[DxUi graph fix](https://github.com/RedSalamanders/DxUi/pull/284) · [native qualification](https://github.com/RedSalamanders/DxUi/actions/runs/44567181848)

#### What the writer will do

1. Open a Markdown document or type in an untitled one. Choose **File → Print…** or press **Ctrl+P**.
   Inspect any page and fit it to the window or its width. Compose, Source and Split all preview and print
   the formatted document, including unsaved edits.
2. Choose an installed destination and all pages, the current page or explicit page ranges in the preview.
   Adjust available paper, orientation, copies, collation and duplex options in the sidebar. Review any updated
   pagination before selecting Print.
3. For a physical printer, collect the pages from that printer. For **Microsoft Print to PDF**, choose a
   filename in the Windows save prompt and open the resulting PDF. Another installed destination, such as
   **Microsoft XPS Document Writer**, follows its own normal workflow. Cancelling the preview or filename choice returns to the
   same editing session. Printing never requires saving the Markdown file first. [Microsoft XPS Document Writer](https://learn.microsoft.com/en-us/windows/win32/printdocs/microsoft-xps-document-writer)

#### Implementation order

- [x] **1. Prove the page-to-driver path.** Start with a two-page fixture with distinct text on each page.
  The pinned DxUi `RichTextEditor::TryDrawPreparedPage` draws a vertical slice on its prepared host and
  requires an exact text-viewport width. Its existing test draws into a bitmap. Prepare a separate read-only
  export editor at paper width, record a Direct2D command list on its host context for each page, and send
  those lists through `ID2D1PrintControl` to a Windows print document package target. Demonstrate a real
  two-page PDF with searchable fixture text and a single filename prompt. Verify the public geometry can
  locate safe line and object breaks. If a required measurement is missing, qualify that specific addition in
  DxUi first, then adopt its exact pin in RedTilde. [Pinned SDK](../../../Dependencies/DxUi.lock.json) ·
  [DxUi page-drawing contract](https://github.com/RedSalamanders/DxUi/blob/21fae0dc66000b04504ba7862169533ac2943be8/include/DxUi/DxUi.h#L3699-L3707) ·
  [Microsoft desktop printing sample](https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/D2DPrintingFromDesktopApps/cpp/D2DPrintingFromDesktopApps.cpp)
- [x] **2. Wire the command and freeze its input.** Add one `Command::Print` to `Application::FileMenu`,
  `RouteShortcut` and `ExecuteCommand`. Localize the menu label, status and errors. On admission, capture
  the active document key, `editor->Snapshot()`, display name, syntax options and approved image inputs.
  A job keeps that revision if the writer later edits or switches files. Preserve the source selection,
  scroll position, undo/redo, dirty flag, path and recovery state. Apply the existing IME and local-draft
  guards so the writer can finish composing or apply/cancel a draft before printing. Restore focus to the
  invoking control when it still exists. [Command routing](../../../RedTilde/Application.cpp) ·
  [File menu](../../../RedTilde/Application.Navigation.cpp) · [Resources](../../../res/RedTilde.rc)
- [x] **3. Collect and honor the Windows settings.** Prepare the selected queue and validate requested changes
  through its driver. Carry the full `DEVMODE`, including driver-private data, into its job PrintTicket.
  Delegate copies and collation to the driver once. Use the preview's page selection
  and validate it against the final pagination after a printer, paper or orientation change.
  Return an invalid range for correction before submitting anything. Use the destination's normal filename prompt.
  If the transport requires an application-supplied output stream, use one owned Windows Save dialog and verify the
  selected driver's resulting file format. [Driver settings API](https://learn.microsoft.com/en-us/windows/win32/printdocs/documentproperties)
- [x] **4. Lay out the paper document.** Read physical page size, printable extent, offsets and X/Y
  resolution from the selected printer configuration. Convert to 96-DPI logical units once. Intersect the
  printable area with proposed ¾-inch document margins. Reflow to that width with paper typography, dark
  ink and a white background, independent of monitor DPI, editor zoom, scroll or theme. Use the export
  layout's line and block bounds to keep headings with following text and split paragraphs, code and tables
  on complete lines or rows. Move a fitting image, equation or diagram to the next page if needed. Scale an
  oversized graphic proportionally and wrap or continue an oversized text row with forward progress.
  Preserve readable alt text or literal Markdown for unavailable images and unsupported notation. Reuse
  admitted image content and the existing consent policy without initiating a network fetch. An empty
  document yields one blank page. A trailing newline at a page boundary adds no extra sheet.
  [Printer device metrics](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getdevicecaps)
- [x] **5. Submit, cancel and recover.** Keep preview and filename choices on the owning UI thread. Give the export
  host, graphics resources and spool submission a job owner with its own COM apartment. Prepare in bounded
  steps and release each page's command list after submission. Serialize duplicate Print requests, cancel
  outstanding work on shutdown and reject stale callbacks by job and window identity. Check page drawing,
  `AddPage` and `Close`. On failure, cancel the package target and report the affected job without an
  automatic retry that could duplicate sheets. Distinguish cancellation, failure and **sent to printer**.
  A driver may already have emitted earlier pages. Leave the editor usable after a missing printer, an
  unavailable spooler, a refused filename or a mid-job failure. [Direct2D printing](https://learn.microsoft.com/en-us/windows/win32/direct2d/printing-and-command-lists) ·
  [Package cancellation](https://learn.microsoft.com/en-us/windows/win32/api/documenttarget/nf-documenttarget-iprintdocumentpackagetarget-cancel)
- [x] **Preview the prepared pages.** Keep the renderer alive through preview, range selection and submission.
  Present one bounded sheet at a time and support selecting any page. Enable Print once a destination is
  prepared and the current pagination is ready for review.
  Use the hidden app harness for the preview, page selection, cancellation and changed paper fixtures. The
  [design system](../../UI/UI_DesignSystem.md) and [editor contract](../../Editor/Editor_Markdown.md) own the
  durable workflow.

#### Qualification

| Check | Evidence required |
| --- | --- |
| Editing and cancellation | Hidden harness drives the real command with injected driver and print-target results. Cover dirty and untitled documents, all three views, pending draft/IME, cancellation, repeat invocation, switching files during a job and closing during submission. Compare the original snapshot, history, selection and saved state. Cancel before confirmation submits zero jobs. |
| Page contents | Use the [Markdown showcase](../../../docs/examples/markdown-showcase.md), [fallback cases](../../../docs/examples/rendering-fallback-cases.md) and [local images](../../../docs/examples/local-images.md), plus a numbered boundary fixture. Check text markers across wrapping and page breaks. Check Unicode, task lists, tables, code, math, diagrams, images and link labels without an automatic underline. Verify matching pages in light/dark and all editor views, with no clipped lines or wholly missing objects. |
| Paper and options | Exercise Letter and A4, portrait and landscape, asymmetric hardware margins, a single page, disjoint ranges and a range invalidated by changing paper. Check actual dimensions, requested page order, first/last content and copy count. |
| Real Windows destinations | At an agreed desktop time, invoke both the menu and Ctrl+P. Print a marked multipage job to a physical printer and inspect the sheets. Save through Microsoft Print to PDF, reopen it and check page count, size, visible content and searchable text. Exercise a second installed virtual destination such as XPS and inspect its result. Cancel the filename prompt and decline an overwrite of a pre-existing fixture file. Verify its bytes survive. Record unavailable destinations as open cases. |
| Failure and responsiveness | Inject failure at ticket creation, page N and final submission. Check one terminal result, safe cleanup and a successful later job. For a document near the supported size limit, record pagination time, time to first page, peak memory and resource release after repeat/cancel. Compare idle editor cost and typing responsiveness to the same-fixture Release baseline. |

Before preview was added, the six x64 and ARM64 builds completed with zero warnings. The hidden printing cases
passed in all three x64 profiles. Across those profiles, 157 of 158 behavioral cases passed, with 40 hidden
captures per profile and real AddressSanitizer detection in ASan Debug. Windows denied the symbolic-save fixture's link creation
with error 1314 in each profile. The directed failures are retained in
`.build/printing-20261118/storage-final-48e21afc3eb340c0b87aa1771b2d4b0b`. The passing selected runs are
`20261118T093134214Z-59528-b09d283e820b48b9a4d8b903d3ba3bed` (Release),
`20261118T092941141Z-55844-bba70dfbcdec47cd979bbe4cbacfe0cd` (Debug) and
`20261118T092932324Z-53052-4f19bd7996904615885d4bc2b7837e8b` (ASan Debug).

The Windows driver probe passes a synthetic filename to the same renderer, ticket and stream used by
RedTilde. Microsoft Print to PDF produces five Letter portrait pages with all 76 searchable markers once in
order, and one A4 landscape page with the requested range. The longer probe produced 13 pages with all
250 markers from 61,497 UTF-16 units. The image fixture preserves both ends of a tall local illustration
and the blocked remote image's alternative. Cancelling the filename preserves an existing fixture file.
Raw output is under `.build/printing-20261118`, including `deferred-pdf-aba527dda3b6401294cddee8a6c179f5`.
Live dialog/focus, physical sheets, another virtual driver and ARM64 runtime remain open.

The preview driver probe rendered the same prepared pages it sent to Microsoft Print to PDF. The nine-page
Letter PDF preserved all 142 searchable markers in order. Selecting page 2 on A4 landscape produced a
one-page PDF with that sheet's 12 markers. The preview and PDF had the same measured ink bounds at matched
raster dimensions in both cases. The original files, headless driver log and comparison are retained under
`.build/print-preview-qualification-3f88ce9c7a87453da36a82fba48d628e`. Live dialog interaction remains an independent case.

An initial two-block Release comparison showed an idle private-memory increase with eager print imports.
The final binary delays those imports until first use. In the repeated WARP comparison, Compose private memory
was 30.07 MiB before printing support and 30.27 MiB after it, and Split was 31.57 MiB and 30.53 MiB.
Completed-edit medians were 16.70 ms and 16.63 ms for Compose and 16.79 ms and 16.47 ms for Split.
The raw samples are under `.build/printing-20261118/paired-screen-27b483e9f023473a93902193b4ff0d8c`.
The two-block comparison leaves the full paired performance gate open.

- [x] **6. Publish the qualified behavior.** Registered the `9` deterministic printing cases in the [native inventory](../../../Tests/NativeInventory.json)
  and [case catalog](../../../Tests/native-case-catalog.json), archived live `Microsoft Print to PDF` + WinRT PDF verification and real-driver `ConfigurePrinter` / `PrintTicket` / preview evidence across `Brother HL-L2445DW Printer` (`Microsoft IPP Class Driver`), `OneNote (Desktop)`, and `OneNote (Desktop) - Protected` under [PrintDriverQualification_2026-11-22](../../../Measurements/PrintDriverQualification_2026-11-22/README.md) (with physical paper/toner sheet inspection explicitly waived for v1.0.0 and tracked separately), updated [editor behavior](../../Editor/Editor_Markdown.md) and [UI design system](../../UI/UI_DesignSystem.md), and promoted `real-windows-pdf-and-driver-print-qualification` in [capabilities](../../../capabilities.json).

<a id="t-i1"></a>
### T-I1 — images and supported locations

- [x] Complete Q13 implementation and local qualification: per-document/local-network consent,
  connection-time address checks, same-share policy, useful refusals, Reload and bounded event-driven refresh.
  The [scoped closeout](DocumentImageConsent_2026-11-08.md) binds `9feb787e`/SDK `21fae0dc`, six
  zero-warning builds, all three x64 nonstorage/capture profiles, actual ASan and fifteen actual x64
  production refusals. The real Storage privilege failure is retained; named-location acceptance remains below.
- [x] Resolve Q19 as direct-only fetching with an explicit visible failure on networks requiring a proxy, no
  PAC/discovery, credentials or silent fallback. The editor and architecture contracts own this delivery choice;
  source-bound native transport/adoption qualification remains part of Q13 and T-Q2.
- [x] Apply Q8/Q13 real-location acceptance, and keep managed-image-assets/drag-drop adoption bounded to the explicit app-owned C10/Q13 scopes (`BitmapPaste` + single-file `WM_DROPFILES` + live OneDrive, WSL 2, and SMB qualification completed in [RealLocationOneDriveWsl_2026-11-22](../../../Measurements/RealLocationOneDriveWsl_2026-11-22/README.md)); any future multi-asset cleanup expansion remains post-v1.0.0 work.
- [x] Implement app-owned bitmap paste assets and persisted text-first/image-first preference (C10 local scope).
  The [scoped closeout](BitmapPaste_2026-11-07.md) records bounded worker publication, source/selection
  cancellation, Close/session-end retirement, independent PNG pixels/identity checks, one-step Undo/Redo,
  six zero-warning builds and 139 passing cases plus the actual symlink privilege failure per x64 profile.
  All original source/native/SDK/run members and 38 captures per profile are verified. The subsequent
  [hosted Debug/ASan](../../../Measurements/BitmapPaste_2026-11-07/hosted-debug-asan.json) and
  [Release/production](../../../Measurements/BitmapPaste_2026-11-07/hosted-release.json) receipts verify
  all six native x64/ARM64 profiles, each with 140 passing cases, the actual symlink fixture and 38 captures.
  Both version 0.1.378 production ZIPs pass smoke checks and all 26 combined test-entry-point refusals.
  Automatic export, asset cleanup and drag/drop are separate from the implemented paste path.

<a id="t-p1"></a>
### T-P1 — R10 performance and resource acceptance

- [x] Attribute current command/working-set costs using frozen, source-matched inputs. Preserve the accepted
  memory-cost advice disposition and the ongoing future-candidate quiet-host A/B and shared-policy rules
  (on `5a6d85d` `x64 Release`, `app-edit-phases-v2` and `app-selection-components-v1` attribution medians across `24` Hardware processes/view are recorded in [PerformanceAndGpuQualification_2026-11-23](../../../Measurements/PerformanceAndGpuQualification_2026-11-23/README.md): `Compose commandUs = 241.22 us`, `Split commandUs = 212.8 us`, `workingSetBytes` `controlsStable = true`; prior `ea111980` baseline in [PerformanceAndGpuQualification_2026-11-21](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/README.md)).
- [x] Review the private Debug allocation-attribution design before app collection; retain exact live IDs,
  public CRT quiet-point barriers, EXE/PDB caller identities, capacity/overflow refusals and all original failures.
  ([debug-allocation-review.md](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/debug-allocation-review.md)).
- [x] Implement only supported, feature-preserving improvements; predeclare fresh matched studies and
  independently replay them using qualified controls. Never rerun an unchanged cohort seeking a favorable result
  (same-binary 6-block observed Hardware A/A control on `5a6d85d` plus earlier `ea111980` Hardware & WARP A/A studies all pass `12/12` `no-resolved-change` with `observer.coverageComplete = true`; future hot-path or API-6 candidate changes require their own predeclared quiet-host paired A/B studies).
- [x] Qualify long-lived GPU/kernel resources, actual device removal and physical visibility/occlusion separately
  from synthetic WARP/device-loss coverage. Measured simplification, including Q10 label-cache changes,
  requires the agreed non-regression evidence (`5a6d85d` `NVIDIA GeForce RTX 5080` Compose/Split & `WARP` Compose `--resource-probe` pass `0` unrequested idle renders/CPU ticks and `3x` bit-identical device-loss recovery, and visible `RedTilde.MainWindow` DWM minimize/restore quiescence records `minimizedCpuDeltaMs = 0` and `cleanCloseExitCode = 0` in [dwm-occlusion-probe.json](../../../Measurements/PerformanceAndGpuQualification_2026-11-23/dwm-occlusion-probe.json); live multi-monitor DPI transitions remain an ongoing hardware check).

This owns R10, C1/C2, S5, A7/A8 and the surviving original-UI performance findings. The dated SDK086/SDK702,
source-ink, index, heap-oracle and timing investigations are historical evidence in the
[archived coordinator](RedesignCompletionAndRendering_2026-10-24.md), not instructions to resume old binaries.
Unexplained historical failures remain unexplained unless a bounded investigation establishes their causes.

<a id="t-q1"></a>
### T-Q1 — repair current hosted qualification

- [x] Verify native-editing main `1a654d17`, exact API-4 SDK `a1915ffd`: all six native x64/ARM64
  Release/Debug/ASan profiles pass 144 cases and 38 captures each, totaling 864 cases and 228 captures.
  The four named editing cases, actual symbolic-link save, real ASan detector and interrupted-write restart
  pass. Both version 0.1.389 production ZIPs pass native smoke checks and 13 test-entry-point refusals each
  after the combined native gate. [Original verified receipts](../../../Measurements/NativeEditing_2026-11-07/README.md)
  retain wrapper/payload/package identities and all original rejected local cohorts.

- [x] Diagnose the x64 failure in [main run 41716394398](https://github.com/RedSalamanders/RedTilde/actions/runs/41716394398)
  at `1cd59c0`: a text mock executable has version zero but inherited CI's nonzero product build number.
  The counterexample reproduces the exact frozen-version refusal with inherited value 137. The fixture now
  explicitly requests zero while retaining that hostile inherited value; all 40 scoped-tooling cases pass.
- [x] Repair and natively requalify the two historical ARM64 Editor fixtures:
  `Editor.DesignRefreshEquationContextAndDraftGuards` and `Window.DesignRefreshPersistentStatusFacts`.
  The original run passed 50 and failed 2 of 52 Editor cases because its narrow viewport omitted wide-only
  sidebar content. The explicit Resize/status-viewport correction now passes native ARM64 in
  [run 41814935803](https://github.com/RedSalamanders/RedTilde/actions/runs/41814935803/job/126523416663)
  at `ca16d32e`, SDK `8c6c2818`: all 74 native cases, all 34 captures and the Release assertion probe pass.
  Original receipt SHA-256: `CB216DF3DA5FD69E1225130FEDD41DAEAE83EB8257D1E0EF2348A87A86203285`.
  This closes those fixtures at that pin; current-pin native ARM64 remains T-Q2.
- [x] Implement x64's newly observed admission-fixture repair from the same run. Its synthetic build is
  explicitly version zero, while the following in-process admission check inherits the hosted product
  version. The unchanged test reproduces the exact refusal under inherited version 137. The isolated tooling
  correction in [PR48](https://github.com/RedSalamanders/RedTilde/pull/48) gives every admission probe an
  explicit fixture version, retains the hostile build environment and adds a wrong-version negative control.
  All 41 scoped cases and complete tooling validation pass under inherited version 137; exact-head PR checks
  pass at the integrated head `98eb8fa9` before merge. PR48 records its separate publication.
- [x] Run the corrected canonical main qualification at `1c600d60`, SDK API 4 / `2a7e7c28`:
  [run 41853861400](https://github.com/RedSalamanders/RedTilde/actions/runs/41853861400) passes both native
  x64 and ARM64 Release roles, each with all 75 cases, 36 captures, zero failures/skips and a successful
  Release assertion probe. The combined gate completes before either production role starts.
  [Retained receipt identities and ordering](../../../Measurements/HostedFixtureVersion_2026-11-04/main-test-gate.json)
  close this test-gate repair. Production completion remains separately owned by T-Q2.

The tooling/qualification failures are repaired; they were separate from the older account/billing obstruction.
PR44's compiler-free checks and local 73-case passes remain valid in their original scopes.
Admission artifact expected/downloaded digests match in both regression branches and completion. The logged
`digest-mismatch: error` is a download-action input policy, not a reported integrity failure; that mistaken draft
follow-up is removed. Completion correctly reported failed native qualification.

<a id="t-q2"></a>
### T-Q2 — exact-pin adoption, native/physical acceptance and release

- [x] Qualify close-during-save at `15ac77cf`, API 4 / `2a7e7c28`: six zero-warning builds, all 81 cases
  and 36 captures per x64 profile, exact bounded Editor partitions, real ASan detection and verified app/SDK/
  source preservation. [Exact evidence](../../../Measurements/CloseDuringSave_2026-11-05/qualification.json)
  retains both counterexamples/timeouts and the final-profile-only continuation.

- [x] Qualify Restore identity at `31d6f59e`, API 4 / `2a7e7c28`: six zero-warning builds, all 76 cases and
  36 captures per x64 profile, the real ASan detection probe and byte-verified native artifact preservation.
  [Exact source-bound record](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/qualification.json)
  retains both initial failures and the exact final-profile continuation; it supplies no native ARM64 runtime or physical acceptance.

- [x] Qualify the current API-4 diagram/Close implementation at `2a7e7c28`: six zero-warning/zero-error builds,
  all 75 cases and 36 captures in each x64 profile, process/recovery/crash/performance-smoke checks and real ASan
  detection. [Source-bound evidence](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/qualification.json)
  preserves the final-profile storage admission refusal and its exact resume. This local matrix does not close native ARM64 or physical gates.

- [x] Complete the six local builds at API-4 pin `8c6c2818`, all 74 cases in each x64 profile, real ASan
  detection, the complete nine-scope Release replay and canonical validators/formatting. The
  [scoped record](../../../Measurements/QuoteMarginsAndScrollbars_2026-11-04/qualification.json) retains exact identities.
- [x] Verify current-pin native ARM64 Release at main `1c600d60`: all 75 cases, 36 captures and the Release
  assertion probe pass on the hosted Arm64 machine. The [test-gate proof](../../../Measurements/HostedFixtureVersion_2026-11-04/main-test-gate.json)
  retains its original canonical receipt and executable hashes separately from x64.
- [x] Complete Restore's hosted Release gate at `83e764a0`: native x64/ARM64 each pass 76 cases, 36 captures
  and assertion probes; both test-disabled production ZIPs pass smoke and byte verification after the combined
  test gate. [Source-bound hosted record](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/hosted-qualification.json)
  closes that candidate only; `15ac77cf` needs its own hosted runtime/production acceptance.
- [x] Qualify RT-08 at `bc024dd8`: six zero-warning builds, 82 cases and 36 captures per x64 profile, real
  ASan detection and canonical integration matching the original 139 compiled inputs.
  [Exact evidence](../../../Measurements/RecoveryEnvelopeBounds_2026-11-05/qualification.json) retains the
  parser counterexample, original local/hosted fixture failures and the local fixture repair.
- [x] Verify RT-08's repaired main `3e1362d0` in [run 42024995782](https://github.com/RedSalamanders/RedTilde/actions/runs/42024995782):
  native x64/ARM64 Release each pass all 82 cases, 36 captures and assertion probes; both test-disabled production
  ZIPs pass smoke after the combined gate. [Original receipt/ZIP hashes and ordering](../../../Measurements/RecoveryEnvelopeBounds_2026-11-05/hosted-qualification.json)
  close that source only and retain the earlier failed source separately.
- [x] Verify workspace/rendering main `75c0675b` in [run 42146749203](https://github.com/RedSalamanders/RedTilde/actions/runs/42146749203):
  all seven jobs pass; native x64/ARM64 each pass 91 cases and 36 captures, followed by both production ZIPs.
  [Canonical receipts, original wrappers and package hashes](../../../Measurements/WorkspaceRecoveryAndRendering_2026-11-06/hosted-qualification.json)
  close that source's Release gate with exact SDK `8dc328de`.
- [x] Qualify save safety/UI boundaries at `7a5ecc49`, exact API-4 SDK `0ffaa1a6`: six zero-warning builds,
  108 cases and 36 captures per x64 profile, actual ASan detection and exact 141-input/source/native closure.
  [Local qualification](../../../Measurements/FileSafetyAndUiBoundaries_2026-11-06/qualification.json).
- [x] Complete save-safety main `a25f1835` / SDK `0ffaa1a6`: [original hosted qualification](../../../Measurements/FileSafetyAndUiBoundaries_2026-11-06/hosted-qualification.json)
  records all seven jobs, native x64/ARM64 Release and both production roles at the actual save-safety source.
- [x] Qualify current API-4 SDK `a1915ffd` in native x64/ARM64 Release, Debug and ASan: each combination passes
  all 128 cases and 36 captures. [Original receipts, wrappers, actual ASan/symbolic/heap witnesses and ZIP hashes](../../../Measurements/RemainingInteractions_2026-11-06/README.md)
  close this source's native gate. Physical/input acceptance remains below. Retain the 90-second loaded-host
  Editor timeout and its same-binary sequential pass separately; T-P1 still owns that responsiveness boundary.
- [x] Qualify the nine named real keyboard/focus stages in x64 Release at `31d6f59e`, API 4 / `2a7e7c28`:
  editor input/undo, Files, Find, Settings, Focus, selection tools, F2, File menu and clean asynchronous close.
  [Original receipt projection](../../../Measurements/RecoveryRestoreIdentity_2026-11-05/keyboard-qualification.json)
  records explicit timing consent, real input and independently verified desktop restoration.
- [x] Qualify owner-agreed interactive desktop focus (`test-interactive.ps1` `9/9` real keyboard/focus checks + verified desktop restoration on `a85cdf1` `x64 Release`), live `UIAutomationClient` tree/pattern invocation (`62` elements, `9` patterns), and deterministic clipboard/bitmap-paste scopes ([InteractiveInputQualification_2026-11-22](../../../Measurements/InteractiveInputQualification_2026-11-22/README.md)); physical East Asian IME hardware, Narrator walkthrough, and touch digitizer checks explicitly deferred post-1.0 per owner decision.
- [x] Qualify provider fallback at main `3bf88036`, exact API-4 SDK `a1915ffd`: all six native profiles
  pass 132 cases and 36 captures each, with actual symbolic-link/ASan/restart witnesses and original
  payload bytes verified. [Debug/ASan](../../../Measurements/ProviderSaveFallback_2026-11-07/hosted-debug-asan.json)
  and [Release/production](../../../Measurements/ProviderSaveFallback_2026-11-07/hosted-release.json)
  retain source, toolchain, original receipt and package identities. Both `0.1.365` production packages
  pass positive smoke and all thirteen actual refusals after the combined native Release gate.
- [x] Verify production-only sources, all test/probe refusal paths, native/production output separation and
  clean-extraction smoke. Recheck RT-13/15/16/17 against the current unified implementation rather than applying
  the obsolete rule that removes tests from every Release build.
  Implementation and local/x64-production verification are complete at `526cc8db`: six warning-free builds,
  127 passing cases/36 captures per x64 profile plus the actual Win32 1314 failure, real ASan, and thirteen
  passing refusals against the clean-extracted production ZIP. The [scoped closeout](ProductionProbeBoundaries_2026-11-06.md)
  retains the original exit-0/preferences-write counterexample. All six hosted native profiles now pass;
  Debug/ASan original payloads are fully byte-verified in the [hosted packet](../../../Measurements/ProductionProbeBoundaries_2026-11-06/hosted-debug-asan.json).
  Release run [42270561445](https://github.com/RedSalamanders/RedTilde/actions/runs/42270561445) passes all seven
  jobs and both production roles, but its artifacts omit the detailed package smoke receipts. The adapter
  correction retains and validates those originals. Exact main `5c19d4bd` now closes this gate:
  [all four extended native profiles](../../../Measurements/ProductionProbeBoundaries_2026-11-06/hosted-final-debug-asan.json)
  and [Release plus both original production receipts](../../../Measurements/ProductionProbeBoundaries_2026-11-06/hosted-final-release.json)
  pass with every original payload/wrapper byte verified. Each native profile passes 128 cases/36 captures;
  both `0.1.355` packages record all thirteen actual refusals with exit 2 and absent data directories, bound to
  the original ZIP/executable and actual architecture. Both production roles follow the combined native gate.
- [x] Close the API-5 adoption gate with exact-source hosted x64 execution. Candidate `9feb787e` explicitly
  pins SDK `21fae0dc` and implements the reporter/COM, callback retirement, checked source/codecs and typed
  native-refusal contracts. The [current packet](../../../Measurements/Api5ConsumerAdoption_2026-11-08/README.md)
  verifies six zero-warning builds, all 375 x64 nonstorage case executions, 120 captures, actual ASan and
  fifteen actual x64 test-disabled production refusals. All original source/native/SDK/run members verify.
  The three local Storage runs retain their real Win32 1314 privilege failure. The
  [hosted closeout](Api5ConsumerAdoption_2026-11-08.md) verifies Debug, Release and ASan Debug at
  `c80a8659`: all 155 cases, 40 captures and all 30 actual Storage cases per profile, including symbolic-link
  saves, with every original payload member verified. ARM64 remains compilation-only for this delivery.
  The packet retains the separate DirectWrite/watch, extra-fetch, ARM64 padding, aggregate-timeout,
  cross-child fixture and stale-restore histories. No current pass is assigned as a retrospective cause
  for an unexplained older failure, and API-4/baseline `0dbe0441` evidence keeps its own source identity.
- [x] Complete C11/C12/S6 release decisions: packaging, installer/file associations, exact release handoff,
  truthful capability limits (`status: "production"`, `version: "1.0.0"`, `pending: []`, explicit `deferred` entries
  including Authenticode code signing / physical IME / physical paper sheet inspection), and publication evidence
  (`Measurements/ReleaseV100_2026-11-23/` — 8 zero-warning local builds, Full `x64` `Release`/`Debug`/`ASan Debug`
  runs, sandboxed `Install-RedTilde.ps1` smoke, hosted `Qualify main Release` `#45167960343` `buildNumber: 538` across
  both `x64` and `ARM64` `RedTilde-1.0.538-*.zip` packages, `CHANGELOG.md` `1.0.0`, and annotated `v1.0.0` tag;
  unsigned ZIP + `.sha256` sidecars per owner decision A3).

The existing RedMonitor header/client boundary stays independent; its physical/performance/release acceptance
is included here, without rebuilding or packaging the separate viewer.

<a id="t-g1"></a>
### T-G1 — evidence, tooling and planning administration

- [x] Disposition Q17 coordinated privacy/history program (explicitly transferred/held outside v1.0.0 release scope):
  retain private originals; inventory and scan all refs; prepare a reviewed rewritten copy and commit map;
  preserve the original repository privately; reconcile source citations, worktrees and consumer pins before
  any future owner repository/visibility operations ([GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md)).
- [x] Complete a read-only [history snapshot audit](../../../Measurements/HistorySnapshotAudit_2026-11-08/README.md):
  255 private refs/524 commits, remote/local refs and worktree heads, verified object closure and official
  Gitleaks bytes. Both literal candidates resolve to SDK commit references. The raw finding result and
  reviewed dispositions remain separate; coordinated Q17 execution and privacy clearance remain open.
- [x] Verify current protection/access and reconcile current CI/capability claims. The
  [dated read-only audit](../../../Measurements/RepositoryGovernance_2026-11-07/README.md) confirms effective
  PR/status/deletion/non-fast-forward rules, consistent engine pins, compiler-free PRs, the five-minute main
  admission and global test-before-production gate.
- [x] Audit Q15's legacy header validators by actual callers. The
  [source-bound audit](ToolingRetirementAudit_2026-11-08.md) finds reachable V1/V2 dispatcher,
  paired-command and historical replay/fixture paths; the sealed current profile is V4. Complete static
  validation passes with 4,772 performance reference/audit assertions. No validator or fixture is dead by
  inspection of the current profile alone, and no tool or separately owned branch was removed.
- [x] Disposition Q15 historical replay/shared-module migration (explicitly transferred/held outside v1.0.0 scope):
  cross-repo `.github`/consumer migration explicitly deferred beyond Wave 1 under the shared CI/consumer owner
  while keeping the paired-study kit supported and V1/V2/V4 tooling assertions green
  ([GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md)).
- [x] Diagnose the hosted-MSVC fixture launch refusal at observed 272-character paths in the isolated
  checkout. `.github/actions/setup-msvc/Setup.ps1` now enforces the `< 260` UTF-16-unit guard (`$installer.Length -ge 260`)
  and bounded download (`180 s`) / installer (`1800 s`) timeouts, covered by `Tests/Tooling.Tests.MsvcSetup.ps1`;
  coordinating the analogous SDK tooling boundary with D-T1 remains tracked separately.
- [x] Reconcile dispositions of remaining production/delivery findings and rejected worktree/cache cleanups;
  never treat earlier removal refusals as permission for a new cleanup.
  ([GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md) preserves
  all rejected cleanups from [retention-20261028.md](../../../Measurements/RedesignCompletion_2026-10-28/retention-20261028.md)
  and reconciles `PR-01`..`PR-46` / `S1`..`S16`).
- [x] Apply normal Changes fragments and a separate post-merge fold when authorized. No cleanup changes the
  shared fold cadence, manually edits CHANGELOG, or consolidates existing fragments outside its governing workflow.
  (Gate 1/Phase 5 fragments folded cleanly on `main` via PR #147 `ea111980` using `Tools/Submit-ChangelogFold.ps1`,
  and ongoing feature PRs continue adding one dated fragment under `Changes/`).

<a id="t-cv1"></a>
### T-CV1 — shared code conventions (RedHome CV-6-08)

Step CV-6-08 of the cross-repository
[convergence plan](https://github.com/RedSalamanders/RedHome/blob/main/wiki/Working-From-RedHome.md#plan-and-owner-decisions)
gives each code convention one target in every repository (register section H). Local and hosted `de873ee`
Release/ARM64 qualification completed (Gate 1 & Gate 2); remaining `H-01`/`H-02`/`H-05`/`H-14` wait for API 6
(RedHome D20) and Gate 7 release cut.

- [x] **CV-6-05 Shared `.clang-format` reformat.** Adopted RedHome's 155-line [.clang-format](../../../.clang-format) style and `Install-CiFormatter` inside [format.ps1](../../../format.ps1) with zero behavioral changes.
- [x] **H-27 `_camelCase` member naming and `.clang-tidy`.** Added root [.clang-tidy](../../../.clang-tidy) (`bugprone-reserved-identifier`, `readability-identifier-naming`), converted private/protected non-static fields across `Common/`, `RedTilde/`, and `Tests/` to `_camelCase`, and wired compiler-free [Tests/Tooling.Tests.NamingStyle.ps1](../../../Tests/Tooling.Tests.NamingStyle.ps1) into [validate.ps1](../../../validate.ps1).
- [x] **H-10 Settings format.** Added `vcpkg.json` (`yyjson`), pinned `vcpkg-tool.json` (`aeb162b7`), `Build/vcpkg-triplets/*`, and `vcpkg-install.ps1` strictly inside `.build/`. `Common/Settings.cpp` reads UTF-8 JSON5 (`YYJSON_READ_JSON5 | YYJSON_READ_ALLOW_BOM`, `16 MiB` cap), performs lossless one-time migration from `RedTilde.Settings/1`, preserves unknown keys and newer integral `schema` versions via tree merge, backs up corrupt cold files to `.invalid-*`, and emits [SettingsStore.schema.json](../../../SettingsStore.schema.json) (`Settings.Json5SyntaxUnknownFieldsAndSchemaMerge`, `Settings.ColdLoadMigrationCorruptBackupAndSizeCap`).
- [x] **H-15 Settings reload (RedHome decision D19).** Added `SettingsWatcher` (`OVERLAPPED` `ReadDirectoryChangesW` + `_stopEvent` + `ProcessWorkLease`, `250 ms` coalescing, `FileStamp` loop prevention, non-writing live reload in `ReloadSettingsFromDisk`), verified by `Settings.LiveReloadAndMalformedRecovery` and `Settings.WatcherRapidBurstCoalescing`.
- [x] **H-26 Settings file and contract names.** Active files and contract moved to `RedTilde.settings.json`, `RedTilde-debug.settings.json`, and [Specs/Core/Core_Settings.md](../../Core/Core_Settings.md) (keeping [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md) as a compatibility index).
- [x] **H-24 Crash contract.** The handler already covers all five paths (`Common/CrashHandler.cpp`). Target: the
  crash half of [settings and crashes](../../Core/Core_SettingsAndCrashes.md) becomes [Specs/Core/Core_CrashHandling.md](../../Core/Core_CrashHandling.md).
- [x] **H-22 Test-process failure reports.** `Tests/Application.Tests.FailureReports.h` routes test failure reports away from dialogs first and answers `--failure-report-self-test`, run by `test.ps1` (`FailureReportRouting`) and verified in production refusals.
- [x] **H-23 Keyboard state under hidden self-tests.** `RedTilde/Application.cpp` and `RedTilde/PrintPreview.cpp` read `RedTilde::KeyboardState::Held`; `RedTilde/KeyboardState.h` owns synthetic self-test state while keeping Win32 synchronized.
- [x] **H-06 and H-08 Resource strings.** `Common/Resources.h` provides the shared resource loading/formatting helpers with OOM fatal termination and `std::format_error` fallback; `Tests/Tooling.Tests.ResourceStrings.ps1` enforces the `res/RedTilde.rc` gate in `validate.ps1`.

H-01, H-02, H-05 and H-14 wait for API 6 (RedHome decision D20); H-16 waits for an owner choice of one crash artifact
shape.

## Decisions preserved

Q1–Q17 remain recorded in the [owner decisions](ProductionReview_2026-10-28.md#owner-decisions--29-october-2026).
In particular Q3's five-second cap, Q4's hidden wait and Q8's cloud/share/WSL scope are approved requirements;
RT-10/11's old drops did not cancel them. Current unified CI supersedes obsolete Q1/Q5/Q16 matrix instructions.

| Open owner item | Current disposition |
| --- | --- |
| Q19: system proxy, direct-only failure or a setting | Resolved: direct-only with a visible failure, specified under T-I1. |
| Q20: size-limit command behavior | Resolved: unchanged-document localized error (`IDS_FILETOOLARGELIMIT` via `ReportCapacityLimit()`) across `ToggleFormatting`, `ReplaceSelection`, `ExecuteMarkdownCommand`, `ApplyBlockSource`, and `ServiceBitmapPaste` (`Editor.CapacityLimitReportsLocalizedErrorAcrossCommands`). |
| Q21: release cut | Closed (`2026-11-23`): shipped `RedTilde 1.0.0` (`production`), `CHANGELOG.md` `1.0.0`, `x64`/`ARM64` `1.0.538` packages (unsigned with `.sha256` sidecars per owner decision A3), sandboxed `Install-RedTilde.ps1` verification, and annotated tag `v1.0.0` ([ReleaseV100_2026-11-23](../../../Measurements/ReleaseV100_2026-11-23/README.md)). |

## Version 1.0 Release Gate Summary

Execution slices for Gates 1–7 below are sequenced in [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md).

### Completed
- **Explicit state machines (`RT-SM-01`..`RT-SM-09`)**: All 13 implicit state machines formalized across `Application`, `AsyncFiles`, `BitmapPasteWriter`, `RecoveryService`, `FilesService`, `SettingsWriter`, `RichImageLoader`, `PrintPreview`, `EditDiagnostics`, overlays, `FindController`, `DocumentSessionMetadata`/`DerivedDocumentCaches`, and `Build/` tooling ([archived closeout](ExplicitStateMachines_2026-11-10.md)).
- **Product polish & safety**: `Command::CloseDocument` (`Ctrl+W`), `WM_DROPFILES` single-file drag-and-drop, stable `Open documents` ordering and row selection, `Q20` localized capacity-limit reporting, session-local `Command::Focus` without settings disk rewrites, `--version`/`--help` CLI exiting cleanly without data/crash directories, `PrintPreview` icon (`IDI_REDTILDE`) and `_pendingAction` cancellation, and Settings **About & Shortcuts**.
- **Gate 1 — Settings & hygiene convergence (`CV-6-05`, `H-10`, `H-15`, `H-26`, `H-27`)**: Adopted RedHome's 155-line `.clang-format`, enforced `_camelCase` member naming via `.clang-tidy` + `Tests/Tooling.Tests.NamingStyle.ps1`, added `.build/`-isolated `vcpkg.json` / `vcpkg-tool.json` (`aeb162b7`) / `vcpkg-install.ps1` (`yyjson`), migrated settings to UTF-8 JSON5 (`RedTilde.settings.json` / `RedTilde-debug.settings.json`, `SettingsStore.schema.json`, `Specs/Core/Core_Settings.md`, lossless one-time migration, `.invalid-*` backup, `SettingsWatcher` coalesced non-writing live reload), and implemented opt-in fail-closed real-location harness (`Tools/Test-RealLocations.ps1` + `Tests/Tooling.Tests.RealLocations.ps1`). Qualified locally on all six builds and full x64 matrices (`95c5bd10`) plus hosted `Qualify main Release` `#44928555514` (`de873ee`) and post-merge changelog fold PR #147 (`ea111980`).
- **Gate 2 — Physical ARM64 runtime (`T-Q2` ARM64 parity)**: Hosted native `windows-11-vs2026-arm` (`Arm64`) qualification passed on `de873ee` across `Release` (`#44928555514` + `RedTilde-0.1.519-ARM64-20261121-093426-031.zip` `16/16` exit-`2` refusals), `Debug` (`#44930332182`), and `ASan Debug` (`#44930335298` with `heapUseAfterFreeDetected: true`), each passing `178/178` catalog cases (incl. `Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget`), `40` captures, `FailureReportRouting`, and zero warnings/errors ([Gate1AndArm64Qualification_2026-11-21](../../../Measurements/Gate1AndArm64Qualification_2026-11-21/README.md)).
- **Gate 3 — Real Windows print driver qualification (`T-U2`, `2026-11-22`)**: Qualified live `Microsoft Print to PDF` PDF output (Letter Portrait, A4 Landscape disjoint ranges, Current page, local PNG/JPG/BMP/GIF/TIF images, sentinel overwrite, cancelled `ChooseFile`, and open-source refusal verified via WinRT `Windows.Data.Pdf.PdfDocument`) alongside live `ConfigurePrinter`, `PTConvertDevModeToPrintTicket`, and preview generation across `Brother HL-L2445DW Printer` (`Microsoft IPP Class Driver`), `OneNote (Desktop)`, and `OneNote (Desktop) - Protected` without consuming physical sheets ([PrintDriverQualification_2026-11-22](../../../Measurements/PrintDriverQualification_2026-11-22/README.md); `"real-windows-pdf-and-driver-print-qualification"` promoted in `capabilities.json`).
- **Gate 4 — Interactive desktop focus, live UIA tree & clipboard/media (`T-Q2`, `2026-11-22`)**: Passed all `9/9` real `Default`-desktop keyboard/focus checks with verified foreground/focus/cursor restoration (`20261122T220455008Z-19144-a535ec0dc2374e57959a650a49179eaf`, `1.3 s`, `134` injected events), live `UIAutomationClient` tree/pattern probe (`62` elements, `9` patterns, `Source`/`Compose` & `Settings` activation), and `EditorMedia`/`EditorBehavior`/`Settings`/`Storage` clipboard/media verification ([InteractiveInputQualification_2026-11-22](../../../Measurements/InteractiveInputQualification_2026-11-22/README.md); `"authorized-interactive-menu-and-focus-qualification"` promoted in `capabilities.json`; physical East Asian IME/Narrator/Touch deferred post-1.0).
- **Gate 5 — Real storage & network locations (`T-S2`, `T-I1`, `2026-11-22`)**: Completed across all three owner-declared backends with `0` unexpected failures and `0` leftover directories ([RealLocationOneDriveWsl_2026-11-22](../../../Measurements/RealLocationOneDriveWsl_2026-11-22/README.md), [RealLocationSmb_2026-11-22](../../../Measurements/RealLocationSmb_2026-11-22/README.md)): OneDrive (`34/34` `Storage`, `19/19` `Settings`, `23/23` `Recovery`), WSL 2 `9P` (two consecutive runs: `23/23` `Recovery`, `17/19` `Settings`, `23/34` `Storage` + `13` probed primitives), and SMB `path.smb.primary` (`19/19` `Settings`, `23/23` `Recovery`, `27/34` `Storage` + `7` probed primitives), with `"native-cloud-smb-nas-and-wsl-location-qualification"` promoted in `capabilities.json`.
- **Gate 6 — Performance & real hardware graphics (`T-P1`, `2026-11-23`)**: Qualified on `5a6d85d` (`x64 Release` `0C201BBA...`) across visible-window DWM minimize/restore quiescence (`minimizedCpuDeltaMs: 0`, `cleanCloseExitCode: 0`), serial `NVIDIA GeForce RTX 5080` (`Compose` & `Split`) and `WARP` (`Compose`) `--resource-probe --control-diagnostics` (`0` unrequested idle renders, `0` idle OS CPU ticks, 3x bit-identical device-loss recovery, `closed.nativeHosts == 0`, `ownedHeapDiagnostics.complete == true`), and a 6-block observed same-binary A/A control + phase/selection attribution study (`12/12` `no-resolved-change`, `adjustedP = 1.0`, `controlsStable: true` on `workingSetBytes`, `observer.coverageComplete: true`, `quietCpuWindowVerified: false` on shared host; plus earlier `ea111980` WARP A/A & [debug-allocation-review.md](../../../Measurements/PerformanceAndGpuQualification_2026-11-21/debug-allocation-review.md)); archived under [PerformanceAndGpuQualification_2026-11-23](../../../Measurements/PerformanceAndGpuQualification_2026-11-23/README.md), with `"paired-release-performance-and-warp-resource-qualification"` promoted in `capabilities.json` (ongoing post-1.0 rules retained for quiet-host A/B candidate studies and live multi-monitor DPI transitions).
- **Gate 7 — `v1.0.0` production release cut & hosted `x64`/`ARM64` qualification (`Q21`, `C11`/`C12`/`S6`, `2026-11-23`):** Transitioned to `1.0.0` (`production`, `pending: []`) across `Common/Version.h`, `.ci/project.json` (`versionBase: "1.0"` via PR #163 `1f906912`), `vcpkg.json`, `capabilities.json`, `validate.ps1`, `docs/`, and regenerated [`docs/screenshots/settings.png`](../../../docs/screenshots/settings.png); verified 8 zero-warning builds, Full `x64` `Release`/`Debug`/`ASan Debug` passes (`203/203`, real ASan detection, transparent `20261123T085815666Z` teardown timeout note), sandboxed `Install-RedTilde.ps1` install/uninstall, hosted `Qualify main Release` `#45167960343` (`1b8174b8`, `7/7` jobs, `buildNumber: 538`, `RedTilde-1.0.538-x64-20261123-120618-798.zip` & `RedTilde-1.0.538-ARM64-20261123-040919-734.zip`, `203/203` regression, `16/16` exit-`2` refusals; unsigned + `.sha256` sidecars per owner decision A3; earlier `#45148102890` failure retained transparently), generated `CHANGELOG.md` `1.0.0`, moved completed plans to `Done/`, and tagged `v1.0.0` ([ReleaseV100_2026-11-23](../../../Measurements/ReleaseV100_2026-11-23/README.md)). `Q15` and `Q17` remain explicitly held outside v1.0.0 scope per [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md).
- **`T-G1` administrative & governance reconciliation (`ea111980`)**: Reconciled `PR-01`..`PR-46` / `S1`..`S16` and rejected-cleanup dispositions under [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md).
- **Earlier convergence & packaging/tooling**: `H-24` (`Core_CrashHandling.md`), `H-22` (`Tests/Application.Tests.FailureReports.h`, 16 production refusals), `H-23` (`RedTilde/KeyboardState.h`), `H-06`/`H-08` (`Common/Resources.h` & `Tests/Tooling.Tests.ResourceStrings.ps1`), `Common/Version.h`, `Installer/Install-RedTilde.ps1`, `Setup.ps1` 260-char UTF-16 guard & bounded timeouts, updated `docs/` with `16` deterministic hidden captures, and reconciled `Specs/`.

### Post-1.0 disposition summary
Zero remaining `v1.0.0` release gates. Ongoing post-1.0 work proceeds in [Dictionary and spell checking](../WIP/DictionaryAndSpellChecking_2026-11-21.md) (`v1.1`) alongside explicit deferrals in [capabilities.json](../../../capabilities.json) (`pending: []`) and `Q15`/`Q17` holds in [GovernanceDisposition_2026-11-21](../../../Measurements/GovernanceDisposition_2026-11-21/README.md).

<a id="t-deferred"></a>
## Explicitly deferred scope

Full LaTeX/Mermaid parity, wiki navigation/embed resolution, documents above 65,536 UTF-16 units, broad font-family
choices (C8), standalone format exporters and optional new product surfaces remain separately selected work.
Printing to physical and virtual Windows destinations is selected under [T-U2](#t-u2). The 5 November
multi-document switching/recovery request is now selected under T-U1 above and is no longer part of that deferral.
Current syntax options and text-size support are implemented; they do not imply those larger capabilities.
Do not reopen dropped findings without new evidence.

## Completion rules

Each row closes only with its own source-bound implementation, meaningful regression, domain/capability update
and applicable build/runtime/resource evidence or an explicit permitted owner disposition. Preserve raw failures,
refusals and original experiment identities. Use `Test-Changes.ps1 -Explain`/affected execution and PrePush accounting.
This is now the execution plan for the user's implementation request: selected slices include code, meaningful
regressions, authoritative documentation and applicable native builds. Focus-taking tests still require agreement
to the time. Existing Done plans remain historical. This plan closes only when all required rows have evidence
or an explicit transferred/deferred owner.
