# Production-readiness review — 28 October 2026

Status: second pass complete on 30 October 2026 at `468cd94`. The owner decided Q1–Q17 on 29 October 2026. The
second pass adds PR-34–PR-46, S12–S16 and open questions Q18–Q22, and corrects PR-02. Remediation has not started.
This plan sequences fixes; durable behavior changes still belong in the owning domain contracts when each item lands.

Scope:
- **First pass:** `main` at `1c3554c` — the entire repository history (first commit 2026-10-20, PRs #1–#24), with
  DxUi pinned at `086ad82a`. Product code (`RedTilde/`, `Common/`, `res/`), the test sources compiled into the
  executable, CI and PowerShell tooling.
- **Second pass:** every merge of the last ten days through `468cd94` (PRs #1–#32; #25–#32 are new), with DxUi
  pinned at `6ed0512c`. See [Second pass](#second-pass--30-october-2026).

Read-only: nothing was built or run for either pass.

Method: the coordinator read Storage, Files, Recovery, Settings, CrashHandler, RemoteImage, ImageLoader,
AsyncFiles, DocumentInfo, Main and the Application file/command/close/message-loop paths, plus the pinned DxUi
sources they depend on. Five delegated reviewers covered file safety, recovery/lifetime, images/network, the UI
host and tooling. Every delegated finding below was re-checked against the code; refuted candidates are listed
so they are not re-raised.

## Verdict

Not production-ready; the second pass at `468cd94` confirms this. The editing core is careful (strict codecs, exact
65,536-unit admission, atomic replace, lease-protected recovery, single-flight file worker, bounded remote fetch),
but five classes of problem block a release:

1. **No working test gate.** Hosted CI has not passed since 2026-10-21; everything merged after that rests on local
   evidence only, and the local selector can skip dependent suites (PR-01, PR-26).
2. **Line endings are not preserved** on several everyday editing paths, silently violating the file-safety
   contract (PR-02).
3. **Unsaved work can be lost** at session end, during continuous typing, and when closing during a save
   (PR-03, PR-04, PR-05).
4. **Crash/undefined-behavior paths** in Restore-after-save, exception handling, menu commands and a close
   request during a prompt (PR-06, PR-07, PR-34, PR-35).
5. **The tests cannot catch these defects.** Every prompt is bypassed in the harness, Storage has no failure
   injection, and several tests pass vacuously or assert the defect (PR-46).

## Findings

Severity: **High** = data loss/corruption or a missing release gate; **Medium** = wrong behavior a user will hit,
or a contract violation with a realistic trigger; **Low** = narrow trigger, hygiene or spec drift.

### High

**PR-01 — CI does not gate merges.** 58 of the last 64 hosted runs failed at job start (0 steps, about 4 s; the
account spending restriction recorded in the redesign review disposition). The last successful hosted run is
2026-10-21. Branch protection is unavailable on this plan (`403 Upgrade to GitHub Pro`). PRs #7–#24 merged without
a passing hosted run, contrary to AGENTS.md (“CI_PENDING requires all named candidate checks to pass before
merge”). There is also no `push` trigger, so `main` itself is never validated after merge.
Fix: restore hosted minutes, a self-hosted runner, or a public repository; until then treat every merge as
CI_PENDING and record the local PrePush receipt in the PR. Add a `push: main` trigger.
The local gate does not compensate. PrePush checks only that the workflow is `active` before deferring every native
obligation to CI (`Build/ScopedTesting.psm1:361-362`). A billing-blocked workflow stays `active`, so a clean branch
gets `LOCAL_OBLIGATIONS_NONE; CI_PENDING` with zero local native evidence. Require proof that a recent run actually
executed the native steps; `Test-ScopedNativeCiProof` already exists for that.

**PR-02 — Line endings are not preserved.** Core_FileSafety requires preserving original line endings, but:
- DxUi `RichTextEditor::OnChar` inserts a hard-coded `\r\n` for Enter in Source view, for Shift+Enter in Compose
  and Split, inside semantic source regions, and whenever the paragraph command is unavailable
  (`DxUi.RichTextEditor.cpp:2349` at `6ed0512c`). Typing Enter in Source view on an LF file produces a mixed-EOL
  file. Plain Enter in Compose and Split (whose editor stays in Rich mode) goes through `InsertParagraph`, which
  reuses the document's own line ending (`DxUi.RichTextCommands.cpp:138-146`); paste is EOL-aware too
  (`DxUi.RichTextEditor.cpp:381-387`). Upstream fix. *Corrected 30 October: the first pass also named Split.*
- The local source draft is a multiline `TextField`, which inserts `\n` on Enter and normalizes pasted text to
  `\n` (`DxUi.TextInput.cpp:2651-2660`, `154-192`). `ApplyBlockSource` writes that text into the source
  unchanged (`Application.cpp:1431-1441`). In a CRLF document every line break typed or pasted into a draft,
  including in an equation, adds an LF line. *Corrected 30 October:* `TextField::SetText` and `GetText` are
  verbatim (`DxUi.TextInput.cpp:963-995`), so an untouched draft compares equal and stores no recovery proposal.
- The app's Insert commands use LF literals (`Application.cpp:1275-1287`: Mermaid, Math, Quote, Fence) and
  produce mixed endings in CRLF documents.
Fix: one app helper converts inserted text to the document's dominant newline (`documentInfo.newlineKind`;
LF/CRLF/CR, defaulting to CRLF for new documents, as DxUi's paragraph command does). Apply it to draft Apply, the
recovery copy of a changed draft, and Insert commands. Raise the Enter case in canonical DxUi and adopt the fixed
pin. Add tests that type Enter and Shift+Enter in LF, CRLF and CR documents in all three views and save
byte-exact. Until this lands, `capabilities.json` overstates support (PR-37).

**PR-03 — Session end loses work.** `HandleMessage` has no `WM_QUERYENDSESSION`/`WM_ENDSESSION` handling, and the
app never calls `ShutdownBlockReasonCreate` or `RegisterApplicationRestart` (`Application.cpp:1757-1895`; no match
in RedTilde, Common or DxUi). On logoff, restart or a Windows Update reboot there is no prompt and no immediate
checkpoint. The process is killed after `WM_ENDSESSION` returns. Core_FileSafety says shutdown bypasses debounce.
Fix: on `WM_QUERYENDSESSION` while dirty, submit `CheckpointRecovery(true)` and set a shutdown block reason. On
`WM_ENDSESSION(TRUE)`, wait a bounded time (for example 2 s) for `recovery.Settled()`. Register for restart so
the next launch offers the snapshot. See Q2.

**PR-04 — Recovery debounce has no maximum latency.** The worker re-waits 750 ms on every `Submit`
(`Recovery.cpp:185-194`). Continuous typing, a held key or repeated programmatic edits never write a checkpoint,
so a crash loses the whole burst. This compounds PR-03.
Fix: cap the quiet wait with a deadline from the first unsaved submission (for example 5 s); this needs a
Core_FileSafety change (Q3).

### Medium

**PR-05 — Closing during a save can lose both the save and the recovery copy.** With a save in flight, WM_CLOSE
asks IDS_CLOSEBUSY. On Yes, `BeginClose` retires the recovery snapshot, closes the file service, and does not wait
for the admitted save. `waitingForPreferences` ignores `files.Busy()`, so `Run` returns and the process exits
mid-write (`Application.cpp:1722-1746, 1769-1783`). A slow or network write is killed; if it would have failed,
neither file nor snapshot remains, and a temporary file may be orphaned. The dialog also appears when only an Open
is in flight on a clean document.
Fix: when a save is admitted, keep the hidden-window wait (as for preferences/recovery) until it completes, retire
recovery only after success, and report failure. Plain loads can be abandoned without prompting.

**PR-06 — Restore after Save dereferences a possibly empty candidate list.** The Restore continuation reads
`recoveryCandidates.front()` when it runs (`Application.cpp:1131-1135`). When the document is dirty, it runs only
after an asynchronous save. Discard stays clickable meanwhile and can empty the list (crash, undefined behavior),
or remove the chosen candidate so a different snapshot is restored and then retired.
Fix: capture the candidate id when Restore is clicked; look it up in the continuation and stop if it is gone.

**PR-07 — Command exceptions either quit the app or vanish.**
- **Shortcuts and file completion quit the app.** Keyboard shortcuts run from the message loop, outside
  `WindowProc`'s try/catch: `RouteShortcut` → `Execute` (`Application.cpp:2215`), and so does the level-check
  `FileCompleted()` (`:2223`). A `std::exception` there (a `THROW_IF_FAILED` in `Layout`, `SetText`, a
  `format_error`) leaves `Run()`, and `wWinMain` returns 1. The window disappears, and edits since the last
  recovery write are lost.
- **Window messages swallow failures.** Inside `WindowProc`, the catch writes a debug string and calls
  `DefWindowProcW` (`:1716-1720`). For WM_CLOSE that destroys the window without the save prompt. Elsewhere a failed
  edit or command is invisible, against AGENTS.md (“Never silently discard an edit or storage error”).

Fix: one named-exception boundary used by both the loop dispatch and `WindowProc`. It returns the message's
“handled” value (0 for WM_CLOSE/WM_SYSCOMMAND), shows an IDS-based status, logs to RedMonitor, and never ends the
process for a recoverable error.

**PR-08 — Atomic write leaks its temporary on write/flush failure.** `removeTemporary` is declared after `file`,
so on an early return it runs while the share-0 handle is still open; `DeleteFileW` fails and the partial
`.redtilde-*.tmp` stays (`Storage.cpp:90-109`). This affects document saves, every recovery retry on a full disk
(one leaked file per attempt), and settings. Fix: close the handle inside the guard before deleting.

**PR-09 — `ReplaceFileW` failure handling can delete the only copy of the new bytes.** With no backup name,
`ERROR_UNABLE_TO_MOVE_REPLACEMENT` (1176) or `_2` (1177) can leave the original renamed or gone while the
replacement survives under its temporary name. The scope guard then deletes it (`Storage.cpp:101, 110-113`). Flags
are 0, so ACL/attribute merge failures on SMB, WSL or cloud folders fail the save outright.
Fix: keep the temporary on 1176/1177 and retry `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`;
pass `REPLACEFILE_IGNORE_MERGE_ERRORS`; retry briefly on sharing violations. Add failure-injection tests.

**PR-10 — A successful save can be reported as failed, then raise a false conflict.** `ReadBytes` opens with
`FILE_SHARE_READ` only (`Storage.cpp:41`). The post-replace readback (`Storage.cpp:167-173`) fails when an
antivirus, sync or indexer process holds the new file for write. The app stays dirty with the old stamp, and the
next Save reports “changed outside RedTilde” about its own write. A readback mismatch also reuses
`ERROR_REVISION_MISMATCH`, which triggers the overwrite prompt. The same share mode prevents opening a file
another application holds open for write.
Fix: read with `FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE` and re-check size; make readback failure
non-fatal, or derive the stamp from the written bytes plus handle metadata; use a distinct error for readback
mismatch.

**PR-11 — Conflict flow does not match the contract.** Core_FileSafety promises reload, Save As or overwrite. The
dialog offers only overwrite Yes/No; No falls through to a second error box with the Win32 text for
`ERROR_REVISION_MISMATCH` and clears `afterSave` (`Application.cpp:1618-1634`). There is no Reload command
(`OpenPath` ignores the current path, `Application.Navigation.cpp:248`). The wording is wrong when the file was
deleted rather than changed. An mtime-only change (touch, sync client) also counts as a conflict, because the
stamp includes `modified` although the content hash already decides it.
Fix: a TaskDialog with Reload / Save as / Overwrite / Cancel; no second dialog; compare size and hash.

**PR-12 — Files pane interferes with other tools.** Scans open every directory with `FILE_SHARE_READ` only and pin
the descent chain (`Files.cpp:36-44`). The recursive watch rescans on any subtree change, up to every 250 ms
(`Files.cpp:486-520`), including RedTilde's own save temporaries. With a document in a busy folder (repository
root during a build, Downloads, the home folder), other processes' directory rename/delete operations (git,
MSBuild, npm, Explorer) intermittently fail with sharing violations. `FolderCompleted` also rebuilds all rows on
every rescan without an equality check (`Application.Navigation.cpp:217-245`).
Fix: share read/write/delete — the `FILE_FLAG_OPEN_REPARSE_POINT` attribute check already blocks redirection —
and skip the model rebuild when the snapshot is unchanged.

**PR-13 — Startup failure is silent.** A non-headless `Initialize` failure returns 2, and any `std::exception`
returns 1, with no UI (`Main.cpp:203-221`). That includes DxUi host attach (graphics), recovery or image service
initialization (`Application.cpp:203-204`, which Core_FileSafety says must degrade, not abort), a `BuildUi`
`THROW_IF_FAILED`, and the throwing `std::filesystem::exists(pending.txt)` (`Main.cpp:195`). A bad argument such
as a second file path or a filename starting with `--` also exits silently.
Fix: show a resource-string error for non-headless failures; let recovery/image failures degrade with visible
status; use the non-throwing `exists` overload.

**PR-14 — Initial window ignores the monitor work area.** 1360×900 DIP scaled by `GetDpiForSystem` with
`CW_USEDEFAULT` (`Application.cpp:188-193`). At 1920×1080 and 150% that is about 2040×1350 px, so the custom caption
buttons and status bar are off-screen; 1366×768 is clipped too. `ShowWindow(SW_SHOWNORMAL)` ignores the launch
show state (`Application.cpp:2209`), and placement is never persisted.
Fix: clamp to the work area of the target monitor, honor `STARTUPINFO`, and persist placement (Q9).

**PR-15 — Modal guards block window commands.** `Execute` returns silently during IME composition
(`Application.cpp:1050-1052`). That includes Save and the caption Minimize/Maximize/Close buttons, which are
non-focusable, so clicking them does not end composition: X does nothing while a CJK composition is open. The
open-draft guard (`:1054-1058`) also runs before Minimize/Maximize, so with an F2 draft open those caption buttons
only change the footer text. Clicking X with even an unmodified draft just refocuses the draft
(`:1763-1768`).
Fix: exempt window commands from both guards; complete the composition before file commands; cancel an
unmodified draft on Close, or prompt.

**PR-31 — Files pane is unavailable in OneDrive folders.** OneDrive (Files On-Demand) folders carry
`FILE_ATTRIBUTE_REPARSE_POINT` with a cloud tag; this machine's OneDrive root and its subfolders all report it.
`Scan::Directory` and `DirectoryWatch::Open` refuse any reparse root (`Files.cpp:115-123, 251-252`), and child
cloud folders become dead leaves (`:164-166, 188`). Any document under a redirected Documents or Desktop folder
shows “unavailable”.
Fix: read `FILE_ATTRIBUTE_TAG_INFO::ReparseTag` and refuse only name-surrogate tags (symlink, mount point;
`IsReparseTagNameSurrogate`), letting cloud and other non-surrogate tags through. Add a cloud-folder test.

**PR-32 — Open completion ignores an open local draft.** A draft does not change the editor revision, so a slow
Open that finishes while an F2 draft is open applies directly (`Application.cpp:1664-1665`). The draft is
orphaned and the editor stays read-only until Cancel. Going through `AskThen` instead would silently drop the
loaded file, because `AskThen` returns early when a draft is visible (`:1539-1543`).
Fix: keep the loaded result pending and ask once the draft is closed.

**PR-16 — Test and probe code ships in `RedTilde.exe`.** The product project compiles about 7,900 lines of tests
(`RedTilde.vcxproj:53-63`: Core, Files, DesignRefresh, DesignInteractive, RemoteImages, Performance). Release
builds therefore accept `--self-test`, `--crash-probe`, `--recovery-crash-probe`, `--preferences-probe` and
`--asan-probe`. `--crash-probe` is not isolated: without `--data-dir` it writes a dump and `pending.txt` into the
real profile. `Application.h` also carries three `friend` probe classes. Fix: see S1 and Q5.

**PR-25 — Every edit restarts image work.** DxUi `RichImageResources::Begin` calls `Cancel()`, which marks every
pending request cancelled, before `Advance` looks for reusable entries. Its `held` test therefore never matches a
pending entry, and never matches a failed one (`DxUi.RichImageResources.h:88-95, 121-126, 177-179`). While the user
types, in-flight local decodes and remote downloads restart on each edit, so large images may not appear until
typing stops. Missing files and dead URLs are re-requested once per edit; with remote images on, the remote host
sees one GET per edit (a typing-cadence signal). The app's loader cannot deduplicate them because each carries a
new request id. Fix upstream (carry pending and failed entries across an unchanged source, base and policy) and
adopt the pin. Until then, consider a bounded negative cache in `RichImageLoader`.

**PR-26 — Test selection can skip required tests.**
- **Under-selected dependents.** The hand-written rules map `Common/Storage.cpp`/`.h` to Storage, Recovery and
  RecoveryRestart only (`Tests/test-scopes.json:167-216`). `Settings.cpp` and `AsyncFiles.h` (open/save) also
  depend on Storage, so the Settings, Preferences and Editor scopes are skipped. The fixes in this plan would
  therefore not run their own regressions. The include-graph generator exists (`ScopedTesting.psm1:476-527`), but
  the manifest has no `projectBindings`, so it never runs.
- **CI change key excludes all of `docs/`** (`ScopedTesting.psm1:397`). *30 Oct:* the reference-dark capture
  fixture moved to `Tests/Fixtures/reference-dark.md` (#31), which is in the key, and no CI runtime input remains
  under `docs/`. The blanket exclusion still lets a future runtime fixture there go unnoticed. The ARM64 runner
  image version is not part of the key either (`ci.yml:14` reads it from the x64 job).
- **Linked Measurements files select nothing.** Edits there select no scope (`ScopedTesting.psm1:172-173`), yet
  Specs link into them and `validate.ps1` fails on broken links.
- **Interactive refusal controls can be skipped locally.** They run only when all four native groups run in one
  `test.ps1` call (`test.ps1:64-81`). After receipt reuse, `FULL_NONINTERACTIVE_PASSED` still prints. CI `-Full`
  does run them.

Fix: add `projectBindings` and delete most per-file rules; key CI on a positive list of real compile and runtime
inputs; select Tooling for linked Measurements files; give the refusal controls their own scope.

**PR-34 — A second close request during a prompt can crash the app (30 Oct).** The save prompt in `AskThen`, the
conflict box and the file dialogs run modal loops with no re-entrancy guard (`Application.cpp:1539-1567,
1620-1628, 1505-1537`). An external `WM_CLOSE` delivered while one is open — `taskkill` without `/F`, a script, or
another tool closing windows — runs the `WM_CLOSE` branch again (`:1761-1787`) and opens a second save prompt.
- “Don't save” there calls `BeginClose`, and `host.Detach()` destroys the control tree (DxUi
  `WindowHost.cpp:1535`). When the user then answers the first prompt, its continuation (New, Open, Restore, the
  Open-completion apply, or the conflict box's forced save) dereferences freed controls.
- “Save” there instead starts a save while the first prompt's continuation is still pending. Its completion can
  then be applied to a different document, because file results carry no document generation (PR-24).

Confirmed by reading; not executed. Fix: count modal depth. While a prompt or dialog is open, record a close request
and replay it after the prompt returns (Q18). After any modal returns, stop if `closing` is set. Test it through
the interaction seam (PR-46) under ASan.

**PR-35 — Menu commands end the process on any exception (30 Oct).** A worse variant of PR-07. `Menu` and
`FileMenu` run `Execute` from the `ContextMenu::ShowAsync` callback (`Application.cpp:1492-1502`,
`Application.Navigation.cpp:520-531`). DxUi invokes it inside the `noexcept` `FinalizeAsyncMenuController`
(`DxUi.Menu.cpp:4643, 4679`), so an exception calls `std::terminate`. Every Heading and Insert command, and the
context menu, goes through this path. Near the 65,536-unit cap:
- Insert > Mermaid, Math, Quote or Fence throw `length_error` from `ReplaceSelection`
  (`DxUi.RichDocument.cpp:710-711`);
- heading, list and table commands throw it from the command planner (`DxUi.RichTextCommands.cpp:1006-1011`).

The crash handler writes a dump, and edits since the last checkpoint are lost (PR-04). Fix: one `OnMenuClosed`
member that runs commands through the PR-07 boundary and reports a status (Q20). Upstream, report capacity as
Unavailable instead of throwing. Test: a 65,530-unit document, then Fence and H1; expect unchanged source and a
visible status.

**PR-36 — The context and Insert menus take focus away from the draft (30 Oct).** After the chosen command runs,
`Menu`'s callback always focuses the editor (`Application.cpp:1500`). Insert > Link and “Edit source (F2)” open the
block draft, make the editor read-only and focus the draft (`:1411, 1418`). Focus then lands on the read-only
editor, so the user's typing goes nowhere. `FileMenu` already has the right guard
(`Application.Navigation.cpp:528`). Fix: the shared `OnMenuClosed` (PR-35) applies that guard. Test: invoke it with
LocalSource and Link and require focus on the draft.

**PR-42 — The paired adoption comparison expects the old DxUi folders (30 Oct).** `Get-AdoptionIncludePaths` builds
40-character source and 64-character output paths (`Build/Performance.psm1:895-896`). Builds since #26 use
16-character folders, so any `-DependencyAdoption` or `-ReviewedHeaderDelta` comparison of a new-layout build throws
(“exactly one declared DxUi source include”). It fails closed, but it blocks the paired evidence that AGENTS
requires before claiming non-regression on a pin adoption, including R10. The fixtures still encode the old layout,
so they stay green (`Tests/Scope.Tests.Performance.Tooling.ps1:289-290`,
`Tests/Scope.Tests.Performance.Evidence.ps1:253-254`, `Tests/ReviewedHeaderDeltaV4.Tests.ps1:47`). Fix: accept
either spelling with exactly one match in total, and build the fixtures from the shared path helper (S14).

**PR-46 — The tests cannot catch the release-critical defects (30 Oct).**
- **Prompts are never exercised.** Every prompt returns early or branches on `options.headless`
  (`Application.cpp:1448, 1507, 1556, 1620, 1632, 1773`). Save / Don't save / Cancel, the conflict choice and
  close during a save never run. Fourteen commands are never executed, among them Close, New, Open, Save As,
  Insert, Link, Image, Table and the Insert snippets.
- **Vacuous tests.**
  - “No owned temporary leaked” (`Tests/Application.Tests.Core.cpp:256-258`) cannot fail. Both failure cases
    return before a temporary exists (`Storage.cpp:88, 97-98`), and the scan is not recursive (PR-08).
  - “Debounce” ends with an immediate flush (`Core.cpp:452-453`), so the 750 ms path and PR-04 are untested.
- **Tests that assert the defect.**
  - A `Storage.cpp` edit must not select Editor (`Tests/Tooling.Tests.ScopedTesting.ps1:63`; PR-26).
  - A workflow reported `active` is delegated (`:208-233`; PR-01).
  - 128 headings is required behavior (`Tests/Application.Tests.Files.cpp:141`; PR-22).
  - The caption test replaces the production handlers with counters, bypassing `Execute`'s guards
    (`Tests/Application.Tests.DesignRefresh.cpp:801-803`; PR-15).
  - Only a mount-point fixture exists for reparse roots (`Files.cpp:342-348`), so cloud tags (PR-31) are
    untested.
- **No failure injection.** Storage has none for write, flush, `ReplaceFileW` 1176/1177 or readback sharing.
  AsyncFiles has no direct test. The ImageLoader local-path policy has no test.
- **Fragile waits.** Foreground-window equality checks (`Core.cpp:537`, about 24 in DesignRefresh) fail if the
  person switches windows during a run. The async save test pumps a fixed number of turns
  (`Core.cpp:1183-1204`) instead of waiting for completion.

Fix, before the step-2 and step-3 fixes so each lands with a failing test first:
- an `Interaction` seam (pick file, ask, conflict, error) that replaces the headless early returns in Debug and
  ASan builds (Q5);
- a small file-operation seam in Storage for failure injection;
- one test-support header (S15).

Release test list, in order:
1. PR-02: Enter, Shift+Enter, the Insert snippets and multi-line drafts in LF, CRLF and CR documents across all
   three views, saved byte-exact.
2. Through the seam: close during a save, every conflict choice, a touch-only change, a deleted file, Restore
   after Save with Discard in between, Open completing under a draft, commands under IME composition, and PR-34.
3. Storage failure injection: PR-08, PR-09, PR-10.
4. PR-03 and PR-04: session-end messages leave a durable snapshot within a bound, and continuous submission
   writes by the cap.
5. PR-17, PR-18, PR-19, PR-22, PR-31, PR-35 and PR-38 regressions.

### Low

- **PR-17** — `ParseRecovery` is `noexcept` but catches only `bad_alloc` and `invalid_argument`. A checksum-valid
  snapshot with 196,609–262,144 text bytes makes `DecodeUtf8` throw `length_error`, so `std::terminate` runs on
  every launch, and the file is never quarantined (`Recovery.cpp:86-106`). Derive the limits from `MaximumUnits`
  and catch `std::exception` there. The same uncaught `runtime_error` gap exists in `LoadDocument`/`SaveDocument`.
- **PR-18** — Recovery worker resilience. One `directory_iterator` error during `Scan` ends the service for the
  session (every edit then shows “recovery failed”) (`Recovery.cpp:129-161, 252-263`). A failed write is not
  retried until the next edit, and a failed write also drops the pending `retire` id, so a restored candidate is
  offered again (`Recovery.cpp:195-243`). Paths over 1,024 units fail every submission despite `longPathAware`
  (`Recovery.cpp:24`). Stale `.lease`, `.quarantine` and `.tmp` files are never aged out. Candidates appear in
  random GUID order with no timestamp.
- **PR-19** — Settings read failure falls back to defaults silently (`Application.cpp:178`). The next preference
  change overwrites the saved file. Core_SettingsAndCrashes requires retaining a rejected file for inspection;
  nothing does (`Settings.cpp:14-15`). A settings file saved with CRLF line endings (for example after editing it
  in Notepad) fails the header check, so every value silently reverts to its default.
- **PR-20** — Non-client messages are handled before the `closing` guard. After `BeginClose`, `host.Detach()`
  frees the control tree (DxUi `WindowHost.cpp:1531-1535`), but `WM_NCHITTEST` still calls `FrameHitTest`, which
  reads the freed caption buttons (`Application.cpp:1787-1820` versus `1859`). Reachable only while the hidden
  window waits for services. Move the guard up.
- **PR-21** — `PickFile` treats every dialog failure as Cancel (`Application.cpp:1510-1534`). A shell failure
  silently cancels Save, Open or a pending Close. Only `ERROR_CANCELLED` is a cancel.
- **PR-22** — Outline parsing diverges from the editor. `AnalyzeDocument` opens a fence for a line such as
  ```` ```code``` ```` (CommonMark forbids backticks in a backtick info string), so every later heading vanishes
  from the outline (`DocumentInfo.cpp:62-75`). It also ignores Setext headings and caps the outline at 128
  headings with no indicator. It does not know front matter, `$$` blocks or HTML blocks either, so a YAML
  `# comment` becomes a heading, and `# ###` yields the text “###” (`DocumentInfo.cpp:76-97`). The editor already
  owns a Markdown parser (see S6).
- **PR-23** — Hard-coded chrome strings, against AGENTS.md:
  - the caption format `"{}{} — RedTilde"`, `"• "` and `" · "` (`Application.cpp:509, 517`), and `" · "` again at
    `Application.Chrome.cpp:822` (30 Oct);
  - the File menu accelerators `Ctrl+N`, `Ctrl+O` and `Ctrl+Shift+S` (`Application.Navigation.cpp:500-502`, 30 Oct);
  - `" BOM"` (`Application.cpp:925`);
  - the `"link text"` placeholder (`Application.cpp:1253`);
  - file-dialog filter names (`Application.cpp:1512-1513`);
  - menu accelerator text (`Application.cpp:1465-1470`);
  - `"Error 0x…"` (`Storage.cpp:192`), with untrimmed `FormatMessage` CRLF in every error dialog.
- **PR-24** — Spec drift. Core_Architecture says file results carry a document generation; `AsyncFiles::Result`
  has none. That is safe only because of single-flight, which PR-34 can bypass. After Open the status reads
  “Saved”. *The orphan WIP-index line from merge `bdfd26d` was already removed by 30 October.*
- **PR-27** — Local image network policy is lexical only. `LoadFile` follows reparse points, so a cloned directory
  symlink to `\\host\share` passes the `\\` check and opens SMB (and tries NTLM) when the user can create symlinks
  (developer mode or privilege). The Files pane already refuses reparse points; images should match. With remote
  images enabled, any host is reachable, including loopback, RFC 1918 and link-local addresses (blind GET to a
  router or a dev server). The worker has no remote-policy gate of its own; it relies on DxUi never emitting
  remote requests while the setting is off (`ImageLoader.h:253-263`). See Q13.
- **PR-28** — One image worker and one FIFO queue (`ImageLoader.h:199-264`). Up to 32 remote fetches of 8 s each
  delay local images, and a hung local read (dead mapped drive, cloud placeholder) wedges image loading for the
  session. `Submit`'s `ERROR_BUFFER_OVERFLOW` result is ignored (`Application.cpp:1678`); that is safe only while
  `MaximumJobs == MaximumSources`, so add a `static_assert`. The same goes for
  `MaximumImageBytes < MaximumResultBytes`. Image failures are always silent alt text (see Q13).
- **PR-29** — `SetDocumentFilePath` can throw for a path DxUi refuses, such as a reserved name on a NAS or WSL share
  (`Application.cpp:489, 1644`). On save success this skips `Changed()` and leaves `afterSave` armed for a later,
  unrelated save. Catch the named exceptions and fall back to an empty image base.
- **PR-30** — Tooling robustness:
  - fixed artifact names without `overwrite: true` make CI re-runs fail (`ci.yml:31-36, 92-106`);
  - `tooling` and `native-scope` have no `timeout-minutes`;
  - `clang-format` resolves from PATH before the VS-bundled binary (`Build/Toolchain.psm1:45-46`);
  - `test.ps1` exits 0 when argument filtering empties the suite list (`test.ps1:12-15`);
  - `Get-RedTildeSourceIdentity` ignores git failure (`Build/Harness.psm1:5`);
  - any first-party byte change, PowerShell included, forces a clean `/GL` rebuild (`build.ps1:43-60`);
  - the entry scripts lack `#Requires -Version 7`.
- **PR-33** — UI polish and accessibility.
  - `RouteShortcut` acts on held-key auto-repeat (Ctrl+E and F11 toggle repeatedly).
  - Editor shortcuts (Ctrl+E, Ctrl+K) fire while focus is in the Find or Files text field.
  - Alt+F10 bypasses the IME guard.
  - Every Open or New rewrites the settings file, because `SetDocument` → `SetView` → `SavePreferences`.
  - Find is case-sensitive only, and F3 with the bar closed does nothing.
  - The Save As dialog never calls `SetFolder`.
  - Startup recovery steals focus to Restore while the user may be typing.
  - Status text is never announced to assistive technology, although DxUi exposes
    `RaiseWindowHostAccessibilityNotification`.
  - The maximized frame leaves no edge for an auto-hide taskbar (`Application.cpp:1698-1708`).
  - The overflow Insert menu anchors at a hidden tool's stale bounds (`:1187-1188`).
  - `IDS_NOTAVAILABLE` is reused for unrelated failures.
  - Multi-line failure text goes into a one-line footer.
  - 18 `IDS_*` strings are unused.
- **PR-37 (30 Oct)** — `capabilities.json` lists `strict-utf8-bom-eol-preservation` as implemented while PR-02 is
  open. Move EOL preservation to `pending` until PR-02 lands; AGENTS requires truthful capability status.
- **PR-38 (30 Oct)** — Find stops updating after a failed replace.
  - `ReplaceFind` sets `updatingFind` without a scope guard (`Application.Navigation.cpp:478-480`), and
    `RefreshFind` returns early while it is set (`:269`).
  - Trigger: a document with a lone CR. Replace All of `X` with nothing in `a\rX\nb` forms a new CRLF join, so
    `Document::ReplaceImpl` throws `out_of_range` (`DxUi.RichDocument.cpp:730-731`). `WindowProc` swallows it
    (PR-07), and the match count and highlights never update again.
  - `ApplyBlockSource` likewise makes the editor writable before an `ApplySourceEdits` that can throw
    (`Application.cpp:1440-1441`), leaving an editable document under an open draft.
  - Fix: `wil::scope_exit` for both, plus a visible failure status.
- **PR-39 (30 Oct)** — Keyboard and UI Automation order differs from the visual order
  (`Application.Chrome.cpp:201-250`). Link is created after Settings, and Files, Panel and More after Link. The
  Files pane, which is on the left, comes after the editor. Tab therefore jumps around the command bar. Create the
  controls in visual order; this pairs with S9's named pointers.
- **PR-40 (30 Oct)** — Window frame details.
  - The top resize band (border metrics, about 8 px at 96 DPI) is tested before the caption buttons
    (`Application.Navigation.cpp:572-586` versus `:597-602`). In the restored state, the top of
    Minimize/Maximize/Close and the right edge of Close resize the window.
  - Minimizing runs `Layout()` at 0×0 (`Application.cpp:1883`). The editor gets negative bounds and retires its
    layout, so restoring reflows the whole document (cost not measured). Return early at zero size.
- **PR-41 (30 Oct)** — Find, Escape and outline polish.
  - A single Replace leaves a collapsed caret, so each replacement takes two presses
    (`Application.Navigation.cpp:472-479`).
  - The replace field has no Enter binding, and Find has no Shift+Enter for the previous match.
  - Wrapping is silent, although Editor_Markdown says Find wraps “with a visible result”.
  - Escape with focus anywhere hides the docked Files pane and persists that (`:548-563`).
  - Changing any heading's text clears every collapsed outline section, because ids are heading indexes
    (`Application.cpp:393-401`).
  - Save and open failures log fixed text without the HRESULT (`:1618-1619`).
- **PR-43 (30 Oct)** — Gaps in the zero-warning gate.
  - When a build fails the gate on a DxUi warning, the retry cleans only RedTilde (`build.ps1:60`,
    `/p:BuildProjectReferences=false`). DxUi is up to date, its warnings are absent from the new log, and the gate
    passes around a warning-built `DxUi.lib`.
  - `test-interactive.ps1:31-34` builds the interactive lease without the gate.
  - The `.build/mock-warning` hook (`Tests/Tooling.Tests.ScopedTesting.ps1:442`) is never used, so no test proves
    that a warning revokes the receipt.
  - Side effect of the 28 October policy: with `/WX` off, Visual Studio and plain MSBuild builds no longer stop on a
    warning. Only `build.ps1` enforces zero warnings.
  - Fix: record failed diagnostics in the receipt and clean references on the retry, gate the lease build, and
    add the mock-warning case.
- **PR-44 (30 Oct)** — Robustness of the short DxUi folders.
  - The output identity marker is created and then written in place (`Build/DxUiPaths.psm1:37-45`). An
    interrupted write leaves an empty marker, and every later restore fails until someone deletes the folder by
    hand. Write a temporary file and rename it.
  - `PinnedDxUiPolicy.targets` is a no-op at this pin, because DxUi's own `Directory.Build.targets` imports a
    byte-identical policy first. Yet RedTilde's copy is hashed into the DxUi fingerprint
    (`DxUiPaths.psm1:89-99`), so the identity attests a file that does not govern the archive, and any edit to it
    forces a full re-restore.
  - The 16-character rule is hand-copied at `Build/Harness.psm1:19-20, 307`, `package.ps1:23`,
    `test-interactive.ps1:24` and two tests instead of calling `Get-DxUiSourcePath`; that is how PR-42 was missed
    (S14).
- **PR-45 (30 Oct)** — Changelog validation details.
  - `Test-Changelog -Base` takes the merge base from `HEAD` but reads `CHANGELOG.md` and `Changes/` from the
    working tree (`Tools/Modules/Documentation/Changelog.psm1:156-171`), so PrePush can pass on uncommitted state.
  - `Test-Changes.ps1:20` hard-codes `origin/main`, although the manifest names the default branch.
  - The `push` branch of `CHANGELOG_BASE` (`ci.yml:47-48`) is dead without a push trigger, yet Core_Changes still
    describes push coverage.
  - There is no sanctioned release cut: renaming `## Unreleased` is refused (`Changelog.psm1:165`; Q21).

### Checked and refuted

- **High-contrast primary button overridden by `primaryButtonFill`:** no bug; DxUi resolves high contrast
  first (`DxUi.Theme.cpp:389-404`).
- **`WM_DPICHANGED` suggested rectangle not applied:** no bug; DxUi `OnDpiChanged` calls `SetWindowPos` with it
  (`DxUi.WindowHost.cpp:2972-2981`).
- **NTLM leak through `//server/x.png`, `file://server/…` or `\\?\UNC\…` image paths:** no bug. DxUi
  canonicalizes UNC and `file://` forms to a leading `\\` (`DxUi.RichImagePaths.h:66-120, 287-330`) and refuses
  `//server/x.png` without a share (`:127-135`); `ImageLoader` declines the `\\` prefix (`ImageLoader.h:265` at
  `468cd94`).
- **Remote fetch:** sound. No proxy, redirects, cookies or authentication; autologon high; TLS 1.2/1.3; `identity`
  encoding only; streamed byte cap and declared-length check; 8 s deadline; the callback-owned lifetime survives
  late WinHTTP notifications.
- **F08 shutdown hang:** remains refuted (redesign review disposition).
- **Files root comparison fails for non-normalized command-line paths:** no bug. MSVC's
  `std::filesystem::absolute` calls `GetFullPathNameW`, which removes `.`/`..` segments and converts `/`, so
  `folderRoot` already matches the normalized root.
- **Detaching the host inside a DxUi click callback:** safe; DxUi copies the callback before invoking it.

## Second pass — 30 October 2026

Scope: the eight merges since the first pass, plus a fresh read of the product code around them at `468cd94`, with
DxUi at `6ed0512c`. The eight merges are:
- allocation-attribution evidence (#27);
- the shared `/Wall`-without-`/WX` build policy, with short DxUi folders and the console result (#26);
- the changelog fold-on-main workflow and its two folds (#28, #30, #32);
- this review (#29);
- the review-ledger correction and capture-fixture move (#31).

Since `1c3554c`, product code changed only by `/Wall` casts and warning pragmas, and the DxUi pin change is warning
fixes. Read-only: nothing was built or run. Hosted CI statistics (PR-01) and the OneDrive reparse tag (PR-31) were
not re-checked offline.

Method:
- The coordinator re-read all of `Application.cpp`, plus Storage, Recovery, Settings, AsyncFiles, ImageLoader,
  RemoteImage, CrashHandler and Main.
- It also read the DxUi host-detach, menu, Enter and source-edit paths.
- Four delegated reviewers covered the tooling delta, revalidation of PR-01–PR-33 at the new pin, the UI and
  navigation files, and the test suites.
- Every finding kept here was re-checked against the code by the coordinator.

**Revalidation.** 31 of 33 findings are unchanged. PR-24's orphan index line is gone. PR-26's capture-fixture
example is resolved by the move to `Tests/Fixtures/reference-dark.md`. First-pass line references are at
`1c3554c`, except where a finding is marked 30 Oct. To read them at `468cd94`, add:
- 2 in `Application.cpp` from line 728;
- 2 in `Application.Navigation.cpp` from line 155;
- 1 in `Storage.cpp` from line 30;
- 4 in `Recovery.cpp` and `Settings.cpp` after about line 105;
- 4 in `Files.cpp` after line 208, and 4 more after line 400;
- 7 in `Main.cpp`;
- 4 in `ImageLoader.h` from line 178;
- 6 in `ci.yml` from line 41.

In DxUi, only `RichTextEditor.cpp` moved: 5 after line 151.

**Corrections to the first pass:**
- PR-02: Split is not affected by Enter, and an untouched draft creates no recovery proposal.
- PR-16 has about 7,900 test lines.
- S7 has 14 scopes.
- S10 counts and the S11 PowerShell size are updated.
- PR-24's orphan line was already fixed.
- PR-26's fixture bullet is rewritten.
- In the refuted list: the NTLM path citations and the DPI rectangle lines.

**New findings:**

| ID | Severity | Summary |
| --- | --- | --- |
| PR-34 | Medium | A second close request during a prompt can crash the app |
| PR-35 | Medium | Menu commands end the process on any exception |
| PR-36 | Medium | The context and Insert menus take focus away from the draft |
| PR-42 | Medium | The paired adoption comparison expects the old DxUi folders |
| PR-46 | Medium | The tests cannot catch the release-critical defects |
| PR-37 | Low | Capability status overstates EOL preservation |
| PR-38 | Low | Find stops updating after a failed replace |
| PR-39 | Low | Keyboard and UI Automation order differs from the visual order |
| PR-40 | Low | Window frame details |
| PR-41 | Low | Find, Escape and outline polish |
| PR-43 | Low | Gaps in the zero-warning gate |
| PR-44 | Low | Robustness of the short DxUi folders |
| PR-45 | Low | Changelog validation details |

Extensions to existing findings: PR-19 (CRLF settings file), PR-22 (front matter, `$$` and HTML blocks), PR-23 (two
more sites) and PR-24 (PR-34 bypasses single-flight).

**Checked and refuted in the second pass:**
- **Replace All and undo:** correct. One `ApplySourceEdits` envelope makes one history step in the authoritative
  editor (DxUi `RichSemantic.cpp:230-290`), and the test at `DesignRefresh.cpp:1485-1500` checks it. The capacity
  pre-check is correct.
- **Outline offsets after edits:** current. Analysis is swapped before `RefreshOutline` (`Application.cpp:412-421`).
- **`GetLogicalChild(N)` and `IDS_X + i` arithmetic:** every use matches the `BuildUi` order today. It remains S9
  for maintainability.
- **Gate bypass through a stale log or `Append=true`:** not possible. Each build log name is unique (`build.ps1:40`).
  A failed gate leaves a `passed=false` receipt, so the next run is a clean build that re-emits RedTilde's own
  warnings. The DxUi case is PR-43.
- **Diagnostics pattern:** 24 sample lines were classified correctly: cl, rc, LINK, MSBuild, `0 Warning(s)` and
  prefixed lines. Only localized or clang-style output would be missed, and the build produces neither.
- **Changelog fold:** exact. Validation compares the result for equality with the merged fragments.
  `Submit-ChangelogFold` pushes only `changelog/fold-<sha>`, without force, from a detached temporary worktree.
- **`/OPT:ICF` in Release:** no code in RedTilde or the pinned DxUi depends on distinct function addresses.
- **Test file names:** `Application.Tests.*` already follows `<Component>.Tests.<Topic>`.
- **`TextField` round trip in drafts:** verbatim. `SetText` and `GetText` keep the text unchanged
  (`DxUi.TextInput.cpp:963-995`); this corrected PR-02.

## Simplification opportunities

- **S1 — Split the binary.** `RedTilde.Core.lib` (Common plus app logic), `RedTilde.exe` (product plus the
  capture harness that AGENTS requires), and `RedTilde.Tests.exe` (Core/Files/DesignRefresh/Performance suites
  and probes). This removes test code and probe switches from the shipping binary and the probe `friend`s from
  `Application.h`.
- **S2 — One owner-notification helper.** Five workers hand-roll the same “mutex + HWND + message + post under
  lock + Close nulls under lock” object (`AsyncFiles.h`, `Settings.cpp`, `Recovery.cpp`, `Files.cpp`,
  `ImageLoader.h`) with inconsistent `PostMessage`-failure handling. One audited type, plus one
  `ServicesIdle()` predicate that also covers PR-05, replaces the three copies at `Application.cpp:1733, 1749,
  2220`.
- **S3 — One newline/insert helper** for PR-02, used by drafts, Insert commands and recovery comparison.
- **S4 — Retire redundant label caches.** DxUi `Label::SetText` already ignores equal text
  (`DxUi.Controls.cpp:2698`). `currentBlockPublishedFacts`, `listSummaryFacts` and
  `selectionCaptionPublishedEquationContext`, with their request counters, save only one `std::format` each.
  Core_Architecture currently mandates the current-block cache (Q10).
- **S5 — A reentrancy guard type** replacing the repeated three-lifetime-token `current()` lambdas in `SetView`,
  `SynchronizeMirror`, `SelectionChanged` and `PublishCurrentBlockLabelIfVisible`.
- **S6 — Derive the outline from the editor's `MarkdownSyntax`** instead of a second heading parser (PR-22),
  keeping `DocumentInfo` for counts and the line index.
- **S7 — Make performance instrumentation conditional.** Fourteen `EditDiagnostics::SelectionMeasure` scopes in
  `SelectionChanged`/`Changed`, plus `DocumentLineIndexStorageStats`, are diagnostic scaffolding in the hot path.
  Compile them out of Release once the R10 performance gates close.
- **S8 — Small cleanups:**
  - `Open()` and `OpenPath()` duplicate the begin-open sequence;
  - `afterSave`, `openingSnapshot` and busy state could be one `PendingFileOp`;
  - `MaximumBytes` and the recovery limits should be derived from `MaximumUnits`;
  - Focus mode writes identical settings bytes (`Application.cpp:1314`);
  - clean documents submit a redundant immediate retire on every `Changed`/`CloseDrawers`;
  - `GetLogicalChild(3)` should be a named pointer (`Application.cpp:699`).
- **S9 — Untangle the UI files.**
  - `Application.Navigation.cpp` actually holds the outline, Files pane, Find/Replace, the File menu and frame hit
    testing. Split it into `Application.Find.cpp`, `Application.Files.cpp` and `Application.Frame.cpp`, the last
    taking the `WM_NCCALCSIZE` code from `WindowProc`.
  - Group the ~25 `find*` members into a `FindState`.
  - Move the `Run()` probe branches and the benchmark writer (`Application.cpp:2058-2200`) beside the capture
    scenarios.
  - Replace `GetLogicalChild(N)` and `IDS_X + i` arithmetic with named pointers or tables.
  - Delete dead UI: `tools[10]` and `commandDividers[5]` are created then hidden every layout.
  - Share one menu builder between `Menu()` and `FileMenu()`.
  - At the 65,536-unit cap, `DocumentLineIndexStorage`'s spare-buffer and statistics machinery is not worth its
    code: the worst case is about 1 MB.
- **S10 — Move evidence out of git.** `Measurements/` holds 5,133 tracked files at `468cd94` (151 MB on disk at
  `1c3554c`, 26 MB packed; 2,757 JSON, 1,551 logs, 620 PNG, 5 zip). The 620 PNGs are only 104 distinct blobs, and 2,334 files are verbatim
  `payload/` copies of `.build/evidence` runs. Each checkout and worktree pays the full working-tree cost, and the
  tree has grown about 16 MB a day. Keep a per-packet README, a SHA-256 manifest and the decisive small JSON in
  git; put payloads in release assets, a separate evidence repository, or an owner-authorized `RedTilde.Perf`
  store.
- **S11 — Delete dead and one-shot tooling.**
  - **Reviewed-header-delta apparatus, about 2,500 lines.** The V1 and V2 validators (`Build/Performance.psm1:369-652`)
    and their `v1`/`v2` fixtures (about 1,300 lines) are dead. The remainder records a single, already-adopted
    pin bump.
  - **Other-repository leftovers.** RedPrism branches in `Test-Changes.ps1`, `Get-CiNativeScope.ps1` and
    `ScopedTesting.psm1`; the unused `Get-ScopedComparisonProblem`; `HostPluginTests` conditions in
    `Build/RedMonitor.props`.
  - **`TestRunEvidence.psm1`** has functions used only by its own tests.
  - **`NativeInventory.json`.** Only `.path` is read.
  - **Performance study kit.** About 3,500 of the PowerShell lines (paired judge plus tests); the repository's
    PowerShell grew from about 8,400 lines at `1c3554c` to 9,345 at `468cd94`. It could live in its own tool or
    repository (Q15).
- **S12 — Replace the warning pragmas with explicit deleted members (30 Oct).** Twelve identical
  `#pragma warning(disable : 4625 4626 [5026 5027])` blocks, each with the same comment and review date, were added
  for the /Wall policy:
  - `Application.h:106-110`, `AsyncFiles.h:27-30`, `ImageLoader.h:179-182`, three in `UiControls.h`;
  - `Application.Chrome.cpp:99-102`, two in `Files.cpp`;
  - `Recovery.cpp:108-111`, `Settings.cpp:100-103`, `RemoteImage.cpp:43-46`.

  The warnings only report that copy and move are implicitly deleted. Declaring them `= delete` on those types is
  the actionable fix that AGENTS asks for, and it removes the pragmas and the twelve review dates.
- **S13 — Files pane cleanups (30 Oct).**
  - Delete `visibleFilePaths` (`Application.h:239`). It copies every entry path on each filter keystroke or rescan,
    and tree ids already index `folderSnapshot->entries`.
  - Remove the inert `rootChanged` parameter of `RefreshFiles` (`Application.Navigation.cpp:120-122`).
  - Stop `DismissSurface` rebuilding the tree twice (`:552-553`).
  - Route the eight `std::vformat(ResourceView(…))` sites in `Application.cpp` through the existing `Text()` helper.
- **S14 — One DxUi path helper and an owned build policy (30 Oct).**
  - Add `Get-DxUiOutputPath` beside `Get-DxUiSourcePath` and use both everywhere (PR-42, PR-44).
  - Remove the other-repository content from the shared policy: the RedXe, RedPrism and RS triplets
    (`Build/NativeBuildPolicy.targets:15-17`), `RSBuildCompilerDebugInformation` (`:48-49`), the six DxUi project
    names (`:54`), and the RedSalamander paths in `Tests/Tooling.Tests.BuildConsole.ps1:6-10, 59-60`. Alternatively,
    keep it byte-identical and consume it from the shared tooling repository of Q15.
- **S15 — One test-support header (30 Oct).** `Require`, `Success` and an `Eventually(predicate, pump, deadline)`
  wait, plus a hidden-app fixture and a foreground guard, would replace six `Require` variants, two `WriteFixture`
  helpers and seven hand-rolled waits. Split `Application.Tests.DesignRefresh.cpp` (3,073 lines) by contract:
  chrome, outline and find, draft, Files pane, surfaces, theme, selection.
- **S16 — Process weight (30 Oct).** Six WIP plans are active, and the redesign plan alone is 281 KB. Every merge
  is followed by a second fold pull request (#28, #30, #32). See Q22.

## Owner decisions — 29 October 2026

The owner answered Q1–Q17. Each decision names what it changes; the owning domain contract is updated when the
item lands, not in this plan.

- **Q1 — CI: make the repository public.** That gives free hosted Windows x64 and ARM64 runners and branch
  protection.
  - Before publishing, rewrite the history (Q17).
  - After publishing, protect `main` with the tooling job and the six native jobs as required checks, and add a
    `push: main` trigger.
  - Make PrePush require proof that the native steps actually ran (PR-01).
  - Remove the “private account cannot enforce protected checks” wording from AGENTS.md and Testing_Validation.
  - Changing the repository's visibility is the owner's action.
- **Q2 — Session end: flush, do not block.**
  - `WM_QUERYENDSESSION` submits an immediate recovery checkpoint; `WM_ENDSESSION(TRUE)` waits a bounded time
    for it to settle. No `ShutdownBlockReasonCreate`.
  - `RegisterApplicationRestart` relaunches after a Windows Update restart.
  - The relaunch opens straight into a polished recovery presentation: document name, when it was last saved,
    the change, a preview, and Restore as the default action.
  - Updates Core_FileSafety and UI_DesignSystem.
- **Q3 — Recovery staleness: 5 s cap.** Keep the 750 ms quiet debounce, and always write within 5 s of the first
  unsubmitted change (PR-04). Updates Core_FileSafety.
- **Q4 — Close during a save: wait for it.**
  - Hide the window and keep the recovery snapshot until the save succeeds.
  - On failure, reshow the window with the error.
  - An Open in flight is abandoned without a prompt.
  - This replaces IDS_CLOSEBUSY (PR-05).
- **Q5 — Test code: compile it out of Release.**
  - Release excludes the test sources, the probe classes and friends, and the probe switches.
  - Debug and ASan Debug keep them.
  - The hidden capture harness required by AGENTS stays in every configuration.
  - Release qualification changes accordingly (Q16).
- **Q6 — Conflicts: one dialog offering Reload, Save as, Overwrite and Cancel**, with no second error box.
  Timestamp-only changes with identical content are not conflicts (PR-11).
- **Q7 — Unsupported files: clear specific errors.**
  - Keep strict UTF-8 and the 65,536-unit cap.
  - Report “not UTF-8” or “too large: N characters, limit 65,536” instead of Win32 text.
  - The `*.txt` filter states that it accepts UTF-8 only.
- **Q8 — Locations: OneDrive/cloud, SMB/NAS and WSL must work fully.** This makes these mandatory:
  - PR-31 (cloud reparse tags);
  - PR-09 fallbacks (`REPLACEFILE_IGNORE_MERGE_ERRORS`, bounded retries, a 1176/1177 recovery path);
  - PR-10 (tolerant sharing);
  - same-share images (Q13c).

  Test on a real share and a WSL path before claiming support. Updates Core_FileSafety and capabilities.json.
- **Q9 — Window placement: persist and clamp.**
  - Store normal bounds and the maximized state in settings.
  - Restore them clamped to an attached monitor's work area.
  - On first run, size the window to fit the work area.
  - Honor the launch show state (PR-14).
- **Q10 — Label caches: drop them** and remove the current-block cache requirement from Core_Architecture (S4).
  Confirm non-regression with the paired performance run before merging.
- **Q11 — Files pane root: pin to the folder where the pane was first opened.**
  - Opening other documents does not move the root.
  - Add commands to choose a folder and to use the current document's folder.
  - A document outside the root opens normally and simply has no tree row.
  - Updates Core_Architecture.
- **Q12 — Links: save through them.**
  - For a symlink, resolve the final target (`GetFinalPathNameByHandle`) and atomically replace that file; the
    conflict stamp and the staging directory belong to the target.
  - For a hard link (link count > 1), write in place, non-atomically, with recovery holding the content until the
    write succeeds.
  - The link stays intact. Updates Core_FileSafety.
- **Q13 — Images.**
  - **Network targets.** Remote fetches refuse loopback, private, link-local and single-label hosts. A separate
    “allow local network images” setting, off by default, lifts that. The address is checked again at connect
    time so DNS cannot point back inside. Tests use an internal seam for their loopback server.
  - **Consent.** Remote images are opt-in per document: a bar offers “Load remote images for this document”, and
    the global setting may say “always”.
  - **Local paths.** Allow `\\server\share` images only when the document is on the same server and share. Refuse
    paths that pass through symlinks or mount points (cloud tags allowed, matching PR-31).
  - **Failures.** The placeholder shows why an image is missing: blocked, missing, too large or not decodable.
  - **Refresh.** Add a Reload images command, and refresh local images when their files change. Use a bounded,
    event-driven watch with no polling, owned by the app worker.
  - Updates Core_Architecture, Core_SettingsAndCrashes, Editor_Markdown and UI_DesignSystem, plus upstream DxUi
    for failure reasons and reload.
- **Q14 — Editing conveniences** for the release:
  - case-insensitive Find with a Match case toggle;
  - F3 / Shift+F3 repeat the last search with the bar closed;
  - an explicit Open link command (context menu and Ctrl+click) that confirms before handing the URL to the shell,
    with links otherwise inert (AGENTS allows this as a separate explicit command);
  - Ctrl+wheel and Ctrl+± for text size;
  - a launch without a file reopens the last session's document if it still exists. A pending recovery snapshot
    takes precedence; otherwise the app shows a blank Untitled document. The sample moves to `docs/examples`.
- **Q15 — Tooling.**
  - Delete the dead V1/V2 reviewed-header validators and fixtures, archive the one-shot header-delta record into
    `Measurements/`, and keep the paired-study kit as a supported tool.
  - Move the modules copied across the Red repositories into one shared-tooling repository consumed by exact pin,
    then delete RedTilde's copies and the RedPrism branches.
- **Q16 — Release tests: drop them from the matrix, keep an on-demand command.**
  - The test matrix runs x64 tests in Debug and ASan Debug only.
  - The shipped Release binary gets a smoke run: launch, open and save a fixture, then close cleanly, with no
    test code present.
  - One documented command builds Release with the tests included and runs them, for use when an optimizer-only
    defect is suspected. It builds into its own output directory, which packaging never reads.
  - S1 (a separate test exe) becomes optional; the opt-in Release test build does not depend on it.
  - When this lands, update AGENTS.md (“x64 tests in all three configurations”), Testing_Validation, the CI
    native profiles and `Test-Changes.ps1 -Mode PrePush`.
- **Q17 — History: rewrite it before publishing.**
  - Before any rewrite, run a secret scan across every ref, not just the current tree.
    - **29 October scan.** A pattern scan of every text blob (3,728) and every commit message reachable from any
      ref found no credential shapes (tokens, cloud keys, private keys, JWTs, assigned secrets) and no employer
      name in file contents.
    - Run gitleaks as well before publishing; it is not installed here.
  - **Identity (owner, 29 October).** Map every author and committer to `Eric Jésover
    <eric-jesover@users.noreply.github.com>` with `git filter-repo --mailmap`:
    - the employer address (96 commits) and the personal-domain address (60 commits) are remapped;
    - the 25 GitHub web merges already use that address and keep GitHub as committer.
  - **Evidence (owner, 29 October): scrub in place, keep every file.** S10 is no longer a prerequisite; it stays a
    later ordinary change. `git filter-repo --replace-text` rewrites every blob in every commit:
    - the local profile path, in its plain and JSON-escaped forms, becomes `%USERPROFILE%`;
    - the machine name becomes `REDACTED-HOST`.

    Scope at `1c3554c`:
    - the profile path is in 64 tracked files, 63 under `Measurements/` (58 in
      `MathMermaidRecovery_2026-10-20/shared-dxui`) plus `Specs/Plans/WIP/RedTilde_2026-10-20.md`;
    - the machine name is in 19 further evidence files (S5 studies and performance JSON).

    Links and validators keep working. The hashes of the scrubbed files change; the private original stays the
    byte-exact record for them.
  - **Open PRs (owner, 29 October): merge first**, then rewrite only `main` and the 5 `archive/*` tags. The
    owner merged #26; #27 and this plan's PR follow, then #28 (whose check expects earlier folds to be merged).
  - **Repository (owner, 29 October).**
    - Rename the current repository to `RedSalamanders/RedTilde-archive`, which stays private and unchanged.
    - Create a new `RedSalamanders/RedTilde`, push the rewritten `main` and tags, then make it public.
    - Force-pushing over the current repository is not enough: GitHub keeps every pull request's `refs/pull/*`
      refs (PRs #1–#29), which would still expose the old commits.
    - Run both GitHub steps only after the owner reviews the rewritten copy.
  - **Dry run, 29 October, on a bare clone of `main` at `c851b3c` plus the tags.**
    - 173 commits rewritten in 2 s, and the commit map written.
    - One identity remains; the tags are lightweight, so they carry no tagger.
    - At the tip, 84 of 5,338 files differ, each exactly by the replace rules, with no missed occurrence.
    - A rescan of every blob and message on every rewritten ref finds no profile path, machine name, employer
      name or credential shape.
  - **After the rewrite**, every commit SHA changes. Recreate or re-clone the worktrees (two under
    `E:/RedTilde.Perf`, one under `.build`, one in `Z:/src/RedTilde-worktrees`) and every clone. Do not carry the
    local `refs/agents/*` checkpoint ref. Set each clone's `user.email` to the public identity so new commits do
    not reintroduce the old addresses.
  - 61 distinct RedTilde commit SHAs are cited in tracked Specs and Measurements files, and evidence identity
    depends on exact source SHAs. Keep filter-repo's commit map as an archived old-to-new table, and rewrite the
    citations in the current tree in one follow-up commit.
  - Keep the original repository private and unchanged as the provenance record for evidence captured before the
    rewrite.

## Open questions — 30 October 2026

The second pass raises these questions; each lists the recommended answer first.

- **Q18 — A close request while a prompt is open (PR-34).**
  - *Recommended:* queue it and act once the prompt is answered. Bring the open prompt to the front, then run the
    normal close (with its own save prompt) afterwards.
  - Alternative: cancel the open prompt and run the close immediately.
- **Q19 — Remote images behind a proxy.** Fetches use `WINHTTP_ACCESS_TYPE_NO_PROXY` (`RemoteImage.cpp:205`), so on
  a network that requires a proxy, remote images never load and nothing says why.
  - Honor the system proxy; the proxy then sees which image hosts are contacted.
  - Keep direct connections only, and show “blocked: proxy required” through Q13's failure placeholder.
  - Make it a setting.
- **Q20 — Commands at the 65,536-unit limit (PR-35).**
  - *Recommended:* run the command and, when it cannot fit, leave the document unchanged and show “The document is
    at its size limit”.
  - Alternative: disable the Insert and formatting commands whenever the document is within a margin of the limit.
- **Q21 — Changelog release cut (PR-45).** Validation refuses any change outside `## Unreleased`, so a release
  cannot be recorded today.
  - *Recommended:* a release command that renames `## Unreleased` to the version heading, with validation
    accepting exactly that transformation.
  - Alternative: keep a single `## Unreleased` section and record releases only as Git tags.
- **Q22 — Process weight (S16).**
  - Consolidate the remaining gates of the six active WIP plans into one release-readiness plan, and move
    completed parts to Done?
  - Fold changelog fragments at release time instead of after every merge? The fold-after-merge rule is shared
    by six Red repositories, so changing it affects all of them.

## Remediation order

1. **Gate.**
   - Merge the open PRs, scan every ref, rewrite the history into a new repository (Q17); the owner makes
     that repository public (Q1).
   - Require the tooling job and the six native checks on `main`, and add the `push: main` trigger.
   - Make PrePush require native proof (PR-01).
   - Fix test selection (PR-26) and CI artifact re-runs (PR-30).
   - Make capability status truthful (PR-37).
   - Restore the paired adoption comparison (PR-42) so later performance claims can be measured.
   - Add the interaction and Storage test seams, and fix the vacuous tests and the tests that assert defects
     (PR-46). Every later fix lands with a test that fails first.
2. **File safety, with failure-injection tests.** PR-08; PR-09 including the SMB/WSL fallbacks (Q8); PR-10;
   PR-02 app side; PR-17; save through links (Q12); the conflict dialog (PR-11, Q6); specific open errors (Q7).
3. **Data-loss and crash windows.** PR-03 (flush plus restart registration, Q2), PR-04 (5 s cap, Q3), PR-05 (wait
   for the save, Q4), PR-06, PR-07 together with PR-35 (one command boundary for loop, window and menu dispatch),
   PR-34 (Q18), PR-20, PR-38.
4. **Upstream DxUi**, adopted under one exact pin:
   - Enter EOL (PR-02);
   - image-request reuse (PR-25);
   - image failure reasons and reload (Q13).
5. **Locations and window.** PR-31 OneDrive; PR-14 with persisted placement (Q9); PR-13, PR-15, PR-32, PR-12,
   PR-21, PR-23, PR-33, PR-36, PR-39, PR-40, PR-41.
6. **Images (Q13).** Same-share UNC and no reparse traversal (PR-27); the LAN block and its setting; the
   per-document consent bar; failure placeholders; Reload images and auto-refresh; loader fairness (PR-28); PR-29.
7. **Features.**
   - Find case and F3 repeat, Open link and zoom (Q14a); last session (Q14b).
   - The recovery relaunch presentation (Q2).
   - The Files pane pinned root and folder commands (Q11).
8. **Architecture.**
   - compile tests out of Release (Q5), with the Release smoke run and on-demand Release test command (Q16);
     S1 optional;
   - S2;
   - drop the label caches (S4, Q10) with a paired performance run;
   - S5–S7 and S9;
   - evidence relocation (S10), no longer a prerequisite of Q17;
   - tooling per Q15 (S11), with S14;
   - S12, S13 and S15; S16 per Q22.
9. **Hygiene.** PR-18, PR-19, PR-22, PR-24, the rest of PR-30, PR-43, PR-44, PR-45 (Q21).

Each step needs `Test-Changes.ps1` affected runs, a Changes fragment, and the domain-contract and
`capabilities.json` updates named in its decision or finding. Until Q5 and Q16 land, qualification remains the
six builds plus x64 tests in three configurations; afterwards it is the six builds, x64 tests in Debug and ASan
Debug, and the Release smoke run.
