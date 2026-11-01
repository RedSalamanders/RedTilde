# Delivery review — 31 October 2026

Status: not started
Date: 2026-10-31

This plan is a non-normative execution record. It does not change product behavior. Durable
behavior still belongs in the owning domain contract when each item lands. Nothing in the
checklist has been started.

## Coordinates

The review tip is `origin/main` `146a96493f2e69d02fbf00b78a468418031a9146` (31 Oct 2026).
The branch this plan is recorded on is current `origin/main` `978c41fefddec1a1c64c2e09ac36dde91d02b70c`,
a changelog fold whose first parent is that tip. `RedTilde/`, `Common/`, `Tests/`, `res/`,
`Dependencies/`, `Specs/Core/` and `Specs/Editor/` are identical between those two commits.
Anchors below are that product tree.

DxUi pin: `Dependencies/DxUi.lock.json` commit `6ed0512c68743c57ac209e22010679d211c4a42b`, API
revision 4. RedTilde links the restored pin. It does not compile a sibling DxUi checkout.
Library defects below are fixed by a later pin bump after the DxUi delivery-review plan lands.
Do not reimplement them in RedTilde.

Owning contracts: [File safety](../../Core/Core_FileSafety.md),
[Settings and crashes](../../Core/Core_SettingsAndCrashes.md),
[Editor](../../Editor/Editor_Markdown.md). The earlier
[production-readiness review](ProductionReview_2026-10-28.md) keeps its own questions.
Correction, 1 November 2026: its recorded Q3/Q4/Q8 owner decisions remain approved. The
earlier RT-10/RT-11 drops did not revoke them. The delivery sequence carries that work;
update the owning contracts and acceptance coverage when it lands. Current CI routing
and Release test/production roles follow [Build_UnifiedCI.md](../../Build/Build_UnifiedCI.md),
which supersedes conflicting parts of Q1/Q5/Q16.

## Sequence

1. RT-04 first.
2. The product half of RT-03 (F2 draft Apply, and Mermaid, math, quote and fence inserts) can
   proceed without a pin bump.
3. The DxUi half of RT-03 (Source-view Enter), DX-4 and DX-3 wait on the DxUi plan, then a pin
   bump to a commit that contains those fixes.

## High

- [ ] **RT-04** — Bind Restore to the snapshot that was clicked. Effort: S.

  `Command::RestoreRecovery` (`RedTilde/Application.cpp` 1131-1150) only checks that
  `recoveryCandidates` is non-empty, then `AskThen` runs later. The lambda reads
  `recoveryCandidates.front()` at 1136 and erases `begin()` at 1147. On a dirty document
  `AskThen` (1539-1566) shows the save prompt and stores that lambda in `afterSave`. The prompt
  pumps messages, so `RecoveryReadyMessage` still calls `RecoveryCompleted` (1856-1859).

  Discard does not erase the vector itself. `Command::DiscardRecovery` (1120-1129) calls
  `RecoveryService::Discard` (`Common/Recovery.cpp` 336-347), which only queues the id.
  `RecoveryCompleted` (671-672) erases that id when the worker retires it. `front()` on the
  empty vector is undefined behavior. If another candidate is now first, Restore applies that
  other snapshot and retires it.

  [Core_FileSafety.md](../../Core/Core_FileSafety.md) says candidate retirement after Restore
  happens only after the new session's replacement is durable. Restoring a different id, or
  crashing, violates that.

  Repair: copy the candidate id at the click, with the snapshot needed to restore it. After the
  prompt and after the save, look that id up and refuse if it is gone. Do not call `front()` in
  the continuation, and do not retire a different id.

  Files: `RedTilde/Application.cpp`. Acceptance: two snapshots, Restore on a dirty document,
  Discard the front id before the save callback. No crash, and the other id remains.
  `Recovery.LocalDraftCancelPreviewRestoreProtectsOriginal`
  (`Tests/Application.Tests.Core.cpp` 1360) restores on a clean document and does not catch this.
  Headless `AskThen` returns without a message box (1556-1557), so the test has to drive the
  dirty continuation directly.

  Do not treat a non-empty list at click time as proof that the same element is still `front()`
  when the lambda runs.

## Medium

- [ ] **RT-03** — Convert F2 draft Apply and the Mermaid, math, quote and fence inserts to the
  document newline. Source-view Enter is a DxUi pin fix, not a RedTilde reimplementation.
  Effort: M.

  [Core_FileSafety.md](../../Core/Core_FileSafety.md) requires the original line endings.
  `documentInfo.newlineKind` is already classified (`Common/DocumentInfo.h` 15-21,
  `Common/DocumentInfo.cpp` 114-118) and shown in the status bar (`RedTilde/Application.cpp`
  929-943).

  Product inserts are hardcoded LF:

  - Mermaid, math, quote and fence (`Application.cpp` 1274-1288).
  - F2 Apply copies the draft unchanged (`ApplyBlockSource`, 1431-1441). The draft field inserts
    LF on Enter (`DxUi.TextInput.cpp` 2651-2660 at the pin) and paste folds CR and CRLF to LF
    (`NormalizeMultilineLineEndings`, 154-180). A changed draft is also spliced into the recovery
    copy with that same text (`Application.cpp` 644-649).

  Compose Enter already preserves endings. In rich view, Enter without Shift calls
  `InsertParagraph` (`DxUi.RichTextEditor.cpp` 2329-2347). That command uses `Eol()`
  (`DxUi.RichTextCommands.cpp` 138-145), which copies an existing terminator and uses CRLF only
  when the document has none. `InsertParagraph` is `builder.Continue()` (2231). Source view,
  Shift+Enter, and a refused paragraph command assign `L"\r\n"` at 2349. That assignment is the
  DxUi half.

  Repair, product only: one helper converts inserted text to the document ending. LF, CRLF and CR
  use that ending. `DocumentNewlineKind::None` uses CRLF, matching `Eol()` when no terminator
  exists. Mixed uses the first existing terminator, else CRLF. Apply it to draft Apply, the
  recovery splice of a changed draft, and the four insert commands. Do not rewrite bytes already
  in the file. Do not reimplement Source-view Enter in RedTilde.

  The DxUi half waits on the DxUi plan, then a pin bump. Acceptance for the product half: draft
  Apply and those inserts in LF, CRLF and CR files save with the original ending. Acceptance for
  the pin half: Enter and Shift+Enter in Source, in all three endings, save byte-exact. Compose
  Enter must keep preserving the existing ending.

- [ ] **DX-4** — Native IME 64 Ki ceiling is a DxUi pin fix (`NativeTextStoreTarget`). Effort: M,
  in DxUi. Not a `RedTilde.cpp` change.

  `TextStoreTarget::GetMaximumDocumentTextUnits` returns `std::numeric_limits<size_t>::max()`
  (`src/Controls/TextStoreTarget.h` 35-38 at the pin). `NativeTextStoreTarget`
  (`src/Controls/DxUi.TextStoreACP.cpp` 201) does not override it. The embedded client does:
  `ClientTextStoreTarget` returns 65536 (`src/Controls/TextInputServices.cpp` 17 and 75-77).
  `ReplacementFitsCapacity` uses the target's ceiling (1764-1770). The rich document rejects
  text above 65,536 UTF-16 units (`include/DxUi/RichDocument.h` 94;
  `DecodeUtf8` throws `std::length_error` above `MaximumUnits * 3` bytes or 65,536 units,
  `src/Controls/DxUi.RichDocument.cpp` 51-63). RedTilde uses the native editor. No public setter
  for that ceiling is on the pin. Do not invent a RedTilde workaround.

  The pin bump is blocked until the DxUi plan overrides the native ceiling. Acceptance, in DxUi:
  a native store at 65,530 units whose final composition would reach 65,540 fails the insert, and
  the last in-range preview remains.

- [ ] **DX-3** — Quoted file-paste fallback is a DxUi pin fix. Effort: M, in DxUi.

  `RichTextEditor::TryPrepareClipboardPaste` (`DxUi.RichTextEditor.cpp` 375 at the pin) falls
  back to raw quotes when `BuildMarkdownFileReference` fails (427-447). The path is appended
  between quotes with no escape. A quote or newline inside a `CF_HDROP` path can become live
  Markdown, and a later image load can request it. Ordinary Explorer paths are safe.

  Repair is in DxUi. RedTilde acceptance after the pin bump: the same hostile paste does not
  create an image node. RedTilde already refuses a `\\server\share` image unless network files
  are allowed (`RedTilde/ImageLoader.h` 265-267; `allowNetworkFiles` defaults false).

- [ ] **RT-02** — Wait for an admitted save before exit, and retire recovery only after success.
  Effort: M.

  Close while `files.Busy()` shows `IDS_CLOSEBUSY` (`res/RedTilde.rc` 130), default No
  (`Application.cpp` 1771-1784). Yes calls `BeginClose` (1724-1747). That submits a null recovery
  snapshot, stops recovery, and calls `files.Close()`. `AsyncFiles` says closure invalidates
  posting without joining a filesystem call (`RedTilde/AsyncFiles.h` 10-11). `Close` (77-85) sets
  `closing` and clears the window. The worker still runs `SaveDocument`, then drops the result
  when `closing` is set and does not post (47-63). `waitingForPreferences` is preferences or
  recovery (1735), not the file worker. Once `closing` is set, `HandleMessage` returns before
  `FileReady` (1861-1870), and the pump calls `FileCompleted` only while `!closing` (2228-2231).
  The dialog says an admitted save may finish. That is not enforced.

  [Core_FileSafety.md](../../Core/Core_FileSafety.md) says edits during an admitted save stay
  dirty and keep recovery, and clean or discard shutdown waits for retirement.

  Repair, following approved Q4: replace `IDS_CLOSEBUSY`, hide the window, and keep the
  process and recovery snapshot until the admitted save returns. Retire recovery only after
  success. On failure, show the window again and report the error. A plain Open in flight
  is abandoned without a prompt.

  Acceptance: start a save, block the worker, request Close. No busy-close dialog appears;
  the window hides while the process and recovery remain. Fail the write: the previous file
  and recovery snapshot still exist, and the window returns with the error.

- [ ] **RT-01** — Flush recovery on `WM_QUERYENDSESSION`. Do not save the user's file on logoff.
  Effort: M.

  `HandleMessage` has no `WM_QUERYENDSESSION` or `WM_ENDSESSION` arm. An unhandled message
  returns `DefWindowProcW` (1896). Logoff skips the Close prompt and the recovery flush. The
  worker waits 750 ms of quiet unless the submission is immediate (`Common/Recovery.cpp` 187-197;
  `Common/Recovery.h` 26-27). [Core_FileSafety.md](../../Core/Core_FileSafety.md) says explicit
  checkpoints and shutdown bypass that debounce, and that recovery must never replace the user's
  Markdown file automatically. That shutdown sentence describes `BeginClose`'s immediate
  retirement, not a required save of the user's file.

  Repair: on query-end-session, checkpoint the pending recovery payload immediately and wait
  until `recovery.Settled()` or a short bound. Do not add a shutdown block that tries to save
  the user's file.

  Acceptance: post `WM_QUERYENDSESSION` while dirty; the snapshot bytes match the editor. A
  snapshot that was already settled can stay as it is.

- [ ] **RT-05** — Hold Open completion while an F2 draft is open. Effort: S.

  Starting Open goes through `AskThen` (1568-1582), so a visible draft already blocks a new Open
  (1541-1544). A slow Open that was started first is different. The draft does not change the
  editor snapshot (`LocalSource` only sets the editor read-only, 1410-1411). Completion with the
  same snapshot calls `SetDocument` immediately (1656-1667), which replaces the text, leaves the
  draft open, and leaves the editor read-only until Cancel. Apply then fails the snapshot guard
  (1426-1429). If the snapshot changed, `AskThen` sees the draft and drops the loaded file with
  no error (1668-1669, 1541-1544).

  Repair: if a draft is visible when the load completes, hold the result until the draft closes.
  Do not call `SetDocument` under an open draft.

  Acceptance: a delayed Open completion, then an F2 draft. The draft text survives, and the
  loaded file is not applied until the draft is closed.

- [ ] **RT-09** — A bad settings header must stay on disk. Effort: S.

  A missing `RedTilde.Settings/1` header makes `ParseSettings` return defaults
  (`Common/Settings.cpp` 11-15). `LoadSettings` still returns `S_OK` (64-72). Startup ignores
  that result (`Application.cpp` 178). The next view change calls `SavePreferences`
  (`SetView`, 745-746), and `SerializeSettings` (56-61) writes a fresh header over the file.
  A bad field inside a valid header already falls back per field (24-46).
  [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md) says invalid settings fall
  back per field and the rejected file is retained.

  Repair: a rejected header stays on disk and is not overwritten by defaults until the user
  changes a setting that was actually read.

  Acceptance: a wrong header survives a view change.
  `Settings.VersionAndFieldFallback` (`Tests/Application.Tests.Core.cpp` 312-324) covers field
  fallback and an unknown schema's in-memory defaults. It does not cover this overwrite.

- [ ] **RT-07** — Conflict dialog offers Reload, Save As and Overwrite. A post-replace read
  failure is not a conflict. Effort: M.

  [Core_FileSafety.md](../../Core/Core_FileSafety.md) requires reload, Save As, or explicit
  overwrite. The dialog is Yes/No (`Application.cpp` 1620-1628). Yes force-saves. No falls
  through to a second error box and clears `afterSave` (1630-1635). `IDS_CONFLICT`
  (`res/RedTilde.rc` 78) tells the user to use Save As, and nothing starts it.

  `FileStamp` is size, last-write and content hash (`Common/Storage.h` 10-14). Keep that
  identity. Do not drop last-write unless the file-safety contract changes in the same work.

  Residual, same item: `SaveDocument` writes, then reads the result back (`Common/Storage.cpp`
  165-176). A failed read returns that read `HRESULT` (170-172). Only a successful read whose
  bytes differ returns `ERROR_REVISION_MISMATCH` (173-174). `ReadBytes` opens with
  `FILE_SHARE_READ` only (42), so a sharing failure is the read error, not a revision mismatch.
  The failure path in `FileCompleted` does not update the stamp, so the next save can then
  conflict with this process's own write (`WriteBytesAtomic` 77-88).

  Repair: one dialog with Reload, Save As, Overwrite and Cancel, and no second dialog. A
  post-replace read failure must not use the conflict dialog, and must not leave the next save
  to raise that dialog about this process's own write.

  Acceptance: Reload, Save As and Overwrite each do what they say, and a failed readback after
  a successful replace does not show the conflict dialog.

- [ ] **RT-08** — Quarantine an oversized checksum-valid recovery envelope instead of terminating.
  Effort: M.

  `DecodeUtf8` throws `std::length_error` above `MaximumUnits * 3` bytes or 65,536 UTF-16 units
  (pin `DxUi.RichDocument.cpp` 55-63). `ParseRecovery` allows text up to `4 * 65536` bytes
  (`Common/Recovery.cpp` 86-91) and catches only `std::bad_alloc` and `std::invalid_argument`
  (99-105). `length_error` leaves the `noexcept` function (`Common/Recovery.h` 24) and
  terminates. `Scan` quarantines only a failed `HRESULT` (154-160), which a throw never reaches.
  The editor writer cannot emit this. The trigger is a checksum-valid envelope under
  `%LOCALAPPDATA%\RedTilde\Recovery`. [Core_FileSafety.md](../../Core/Core_FileSafety.md) says
  the checksum detects corruption, not hostile tampering, and that partial or corrupt data is
  renamed `.quarantine` without overwrite.

  A text payload of 200,000 bytes passes the `4 * 65536` check and fails `MaximumUnits * 3`
  (196,608).

  Repair: bound the payload before `DecodeUtf8`, and catch `std::exception` around the parse so
  the worker quarantines the file and continues.

  Acceptance: a checksum-valid envelope with 200,000 UTF-8 text bytes still starts, and the
  file is quarantined. Existing corrupt-envelope tests reject a bad checksum before
  `DecodeUtf8`.

## Low

- [ ] **RT-06** — Close the staging handle before deleting a failed temp. Effort: S.

  `removeTemporary` runs `DeleteFileW` while the share-none handle can still be open
  (`Common/Storage.cpp` 102-109). An early return from a write or flush failure leaves the
  `.redtilde-*.tmp` in place. The success path resets the handle at 110 before replace.

  Repair: `file.reset()` before `DeleteFileW`. Acceptance: an injected flush failure leaves no
  temp. Do not claim the destination file is deleted.

- [ ] **RT-12** — Shortcut and save-completion exceptions stay in the process. Effort: S.

  `RouteShortcut` (1994) and the pump's `FileCompleted` (2231) run from `Run` (2223-2231),
  outside `WindowProc`'s catch (1698-1721). They are inside `wWinMain`'s catch, which returns 1
  (`RedTilde/Main.cpp` 229-233). The window goes away, and edits since the last recovery write
  are lost.

  Repair: the same "source retained" fallback `WindowProc` already uses (1718-1720). Acceptance:
  a throw from save completion leaves the process up and the document dirty.

- [ ] **RT-13** — Gate `--crash-probe` the way production self-test is gated. Leave
  `--performance-probe`. Effort: S.

  `--crash-probe` is parsed and run before the self-test gate (`RedTilde/Main.cpp` 149-150 and
  194-195). Production self-test returns 2 when `REDTILDE_ENABLE_TESTS` is absent (200-206).
  `Directory.Build.props` defines that macro only when `RedTildeEnableTests` is true (line 22).
  [Build_UnifiedCI.md](../../Build/Build_UnifiedCI.md) says ProductionOnly removes the test
  sources and refuses self-test, and that hidden capture and performance diagnostics remain
  product capabilities. `--performance-probe` is parsed at `Main.cpp` 101 and is supposed to
  ship.

  Repair: refuse `--crash-probe` in a production build the same way self-test is refused. Do
  not gate `--performance-probe`.

## Approved requirements carried forward — corrected 1 November 2026

The original RT-10/RT-11 drops described existing behavior against the current domain
contract. They did not supersede the owner's earlier approved changes:

- **RT-10 / Q3, five-second recovery cap.** Keep the 750 ms quiet debounce and write within
  five seconds of the first unsubmitted change, including continuous typing. Update
  [Core_FileSafety.md](../../Core/Core_FileSafety.md) and deterministic deadline coverage
  when this lands. This remains approved Phase 1 work under PR-04.
- **RT-11 / Q8, locations.** Implement the approved OneDrive/cloud, SMB/NAS, and WSL support,
  including cloud reparse tags (PR-31), save fallbacks (PR-09), tolerant sharing (PR-10), and
  same-share images (Q13c). Update the owning contract and `capabilities.json` with the work;
  test a real share and WSL before claiming support. Preserve refusal to traverse actual
  symlinks and mount points. The current reparse-root rejection does not cancel this scope.

## Dropped

The following refuted findings remain dropped.

- **Every Enter is CRLF.** Compose Enter already copies the existing terminator through `Eol()`.
  See RT-03.
- **`ApplySourceEdits` returning false drops a draft edit.** False means the spelling was
  unchanged. The pin says so (`include/DxUi/RichSemantic.h` 72). `ApplyBlockSource` ignores the
  return (`Application.cpp` 1441) after it has already rejected a stale snapshot (1426-1429).
- **IME composition swallows `WM_CLOSE`.** `Execute` returns while a native composition is open
  (`Application.cpp` 1051-1054). [Editor_Markdown.md](../../Editor/Editor_Markdown.md) keeps IME
  handling ahead of commands that could interrupt composition. An open draft refuses Close until
  Apply or Cancel (1765-1769) by design.
- **A save read-back sharing failure becomes `ERROR_REVISION_MISMATCH`.** It does not. A failed
  `ReadBytes` returns the read `HRESULT` (`Common/Storage.cpp` 83-85 and 170-172). Only a
  successful read with different bytes is a mismatch (87-88 and 173-174). The follow-on dirty
  stamp is RT-07, not a new error code.

## Leave standing

Do not simplify these while fixing the items above.

- Atomic replace stages a unique sibling with `CREATE_NEW`, writes, flushes, closes, then
  `ReplaceFileW` or `MoveFileExW` (`Common/Storage.cpp` 90-118). Failed or cancelled storage does
  not clear dirty state.
- Strict UTF-8: `DecodeUtf8` rejects invalid UTF-8, and oversized documents fail before they
  replace the current one (`Common/Storage.cpp` 139-155;
  [Core_FileSafety.md](../../Core/Core_FileSafety.md)).
- The recovery worker owns its lease. Another live session's leased snapshot is not offered or
  deleted (`Common/Recovery.cpp` 146-148).
- A Files child that is a reparse point is not entered (`Common/Files.cpp` 115-122).
