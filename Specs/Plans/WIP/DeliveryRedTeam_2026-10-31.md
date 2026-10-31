# Delivery red team — 31 October 2026

Status: review complete. Remediation has not started. This plan is a non-normative
execution record. It does not change product behavior. Durable behavior still
belongs in the owning domain contract when each item lands.

Date: 2026-10-31

## Coordinates

Review tip: `origin/main` `d126a047cbd969a1fec7858e4350aa9e8c5274b9` (31 Oct 2026,
merge of [DeliveryReview_2026-10-31.md](DeliveryReview_2026-10-31.md)). The last
15 days are the entire repository (root `6c4eff7`, 2026-10-20). DxUi pin
`6ed0512c`, API revision 4.

Method: the coordinator re-read Storage, Recovery, Settings, AsyncFiles, Files,
RemoteImage, ImageLoader, Application file/close/restore/menu/run paths, Main,
CI, `package.ps1` and `capabilities.json`. Independent passes covered security,
bugs, CI/packaging and the 15-day history freeze. Delegated findings were
re-checked against current bytes. Nothing was built or run. Hosted CI was
inspected live: the last 15 `ci.yml` runs failed in 3–9 s with zero steps. The
repository is private; branch protection returns 403.

Owning contracts: [File safety](../../Core/Core_FileSafety.md),
[Settings and crashes](../../Core/Core_SettingsAndCrashes.md),
[Editor](../../Editor/Editor_Markdown.md). The
[production-readiness review](ProductionReview_2026-10-28.md) and
[delivery review](DeliveryReview_2026-10-31.md) keep their own questions and
dropped lists. This plan does not reopen a dropped finding. It sequences the
confirmed holes and the new ones the first two reviews missed or under-weighted.

## Verdict

Not delivery-ready. The editing core is careful (strict UTF-8, 65,536-unit
admission, atomic sibling stage, recovery lease, remote images default off, no
HTML/script/URL execution). Everyday close, restore, logoff, menu, and
production-ZIP paths still lose bytes or terminate the process. Two review
plans already named most of that. Remediation has not started.

A truthful experimental 0.1 may ship only after Phase 1–3 below. ARM64 runtime,
physical IME/AT, installer, documents over 65,536, full LaTeX/Mermaid and paired
Release performance stay pending.

## Confirmed still open

Every DeliveryReview item is still unchecked at this tip. ProductionReview
PR-01, PR-26, PR-30, PR-34, PR-35, PR-46 and Q1, Q5, Q16, Q17 are still open.

| ID | Sev | Location | Trigger |
| --- | --- | --- | --- |
| RT-04 / PR-06 | P0 | `Application.cpp` 1131-1147 | Dirty Restore then Discard during the save prompt. `front()` on empty or the wrong snapshot. |
| RT-02 / PR-05 | P0 | `Application.cpp` 1724-1784; `AsyncFiles.h` 57-63 | Close while a save is admitted. Recovery retired; worker result dropped. |
| RT-01 / PR-03 | P0 | `Application.cpp` 1759-1896 | No `WM_QUERYENDSESSION`. Typing never satisfies 750 ms quiet. |
| RT-08 / PR-17 | P0 | `Recovery.cpp` 86-106 | Checksum-valid 200k envelope: `DecodeUtf8` `length_error` leaves `noexcept` ParseRecovery. |
| PR-34 | P0 | `Application.cpp` 1539-1787 | Second `WM_CLOSE` during a modal prompt. `host.Detach()` then the first continuation runs. |
| PR-35 / PR-07 | P0 | `Application.cpp` 1492-1500, 2051, 2230 | Menu `Execute` in a DxUi `noexcept` callback; shortcuts and pump `FileCompleted` outside `WindowProc`. |
| PR-01 | P0 | GitHub + `ci.yml` | Private repo. No required checks. Hosted jobs die at start. |
| RT-03 / PR-02 | P1 | `Application.cpp` 1274-1288, 1431 | F2 Apply and four Insert commands write LF. |
| RT-07 / PR-11 | P1 | `Application.cpp` 1620-1635 | Conflict Yes/No only. Failed readback dirties the next save. |
| RT-09 / PR-19 | P1 | `Settings.cpp` 11-15 | Bad header returns defaults as `S_OK`. Next view change overwrites the file. |
| RT-05 / PR-32 | P1 | `Application.cpp` 1656-1669 | Slow Open under an F2 draft applies or drops the file. |
| RT-13 | P1 | `Main.cpp` 149-195 | `--crash-probe` runs before the test gate. Real profile without `--data-dir`. |
| RT-06 / PR-08 | P1 | `Storage.cpp` 102-109 | `DeleteFileW` while the share-none handle is open. |
| PR-09 / PR-10 | P1 | `Storage.cpp` 42, 113 | `ReplaceFileW` flags 0; `ReadBytes` `FILE_SHARE_READ` only. |
| PR-14 | P1 | `Application.cpp` 188-193 | 1360×900 DIP; `ShowWindow(SW_SHOWNORMAL)`; `nCmdShow` unused. |
| PR-26 | P2 | `Tests/test-scopes.json` 168-176 | `Storage.cpp` does not select Settings or Editor. The tooling test still requires that skip. |
| PR-46 | P2 | `Tests/` | Headless `AskThen` returns. No Restore+Discard, close-during-save, logoff, or 200k envelope case. |

## New findings

Recorded so they are not lost. None of these reopen a DeliveryReview drop.

- [ ] **RT-14** — `ReplaceFind` leaves `updatingFind` true if `ApplySourceEdits` throws
  (`Application.Navigation.cpp` 478-480). `RefreshFind` then returns at line 269 for
  the rest of the session. Effort: S. Same exception boundary as RT-12 / PR-35.

- [ ] **RT-15** — `package.ps1` builds Release without `-ProductionOnly`. The
  `--self-test` exit-2 check runs only when `testsEnabled` is already false
  (`package.ps1` 5, 54-56). A local ZIP ships the test catalog. Hosted Production
  uses `-ProductionOnly` and has never run. Effort: S.

- [ ] **RT-16** — `Application.Tests.Performance.cpp` is always compiled
  (`RedTilde.vcxproj` 84, no `Condition`). `--performance-probe` may remain a
  product diagnostic; the test `.cpp` is not that remainder. Effort: S.

- [ ] **RT-17** — Gate `--recovery-crash-probe`, `--recovery-restart-probe` and
  `--preferences-probe` the same way as `--crash-probe` (RT-13). They are parsed
  and dispatched without `REDTILDE_ENABLE_TESTS` (`Main.cpp` 151-166;
  `Application.cpp` 2070-2113). Packaging smoke must refuse `--crash-probe` and
  `--recovery-crash-probe`, not only `--self-test`. Effort: S.

- [ ] **RT-18** — `SaveDocument` is `noexcept` and does not catch
  `std::length_error` (`Storage.cpp` 158-185). `LoadDocument` already does.
  The unit pre-check makes a throw unlikely. Catch it anyway. Effort: S.

- [ ] **RT-19** — Tracked `Measurements/` JSON still contains `C:\Users\…`
  worktree paths (272+ hits). Q17 must scrub or relocate them before any public
  repository. Effort: owner + rewrite.

`prCoverage` is empty, so PrePush cannot skip natives today. The
`workflow.state -eq 'active'` delegation hole is dormant, not closed
(`Build/ScopedTesting.psm1` 337-364). `Test-ScopedNativeCiProof` is never
called. Do not treat an `active` billing-blocked workflow as native proof.

## Owner decisions still required

These are not implementation items until the owner picks one. Do not implement
a dropped DeliveryReview item from this plan.

- **Q3 vs RT-10.** ProductionReview Q3 required a 5 s recovery write cap.
  DeliveryReview dropped RT-10 because `Core_FileSafety` only requires 750 ms
  of quiet. Continuous typing plus logoff still loses the burst. Either change
  the contract and implement the cap, or accept the loss in writing.
- **Q8 vs RT-11.** ProductionReview Q8 required OneDrive/SMB/WSL. DeliveryReview
  dropped RT-11 (Files reparse). Open/Save of a single file can work; the Files
  pane stays unavailable on a cloud reparse root. Either implement PR-31 or
  state that the Files pane is local-NTFS only in `capabilities.json`.

Q18–Q22 on the production-readiness review remain open (close-while-prompt,
proxy images, commands at the 65,536 cap, changelog release cut, process weight).

## Sequence

1. Phase 1 first: RT-04, RT-02, RT-01, RT-08, PR-34, one exception boundary
   (RT-12 / PR-07 / PR-35 / RT-14). Write the failing tests first (PR-46).
2. Phase 2: RT-03 product half, RT-06, PR-09, PR-10, RT-07, RT-09, RT-05, RT-18.
   Source-view Enter, DX-4 and DX-3 wait on the DxUi plan, then a pin bump.
3. Phase 3: RT-13, RT-15, RT-16, RT-17, PR-14, PR-13, truthful
   `capabilities.json` (remove `strict-utf8-bom-eol-preservation` until 2 lands).
4. Phase 4: owner — Q17 rewrite, Q1 public repo, required checks, PrePush
   native proof. Until hosted is real, PrePush runs local natives.
5. Phase 5: only after Q8 / Q13 are reaffirmed — PR-31, PR-12, Q13, PR-27, PR-25 pin.
6. Phase 6: six builds, x64 Debug/Release/ASan, real ASan probe, portable ZIP
   smoke including probe refusal. Interactive/IME/ARM64 stay explicit gates.

Do not merge more review-only or changelog-fold pull requests ahead of Phase 1.

## Phase 1 acceptance

- Two snapshots, dirty Restore, Discard the clicked id before the save
  callback. No crash. The other id remains.
- Admitted save, block the worker, confirm Close, fail the write. Previous
  file and recovery snapshot both still exist. Window reshown with the error.
- Post `WM_QUERYENDSESSION` while dirty. Snapshot bytes match the editor. No
  silent save of the user file.
- Checksum-valid 200,000-byte recovery envelope. Process starts. File
  quarantined.
- Second `WM_CLOSE` during the save prompt. No use-after-free. Close runs
  after the prompt returns.
- 65,530-unit document, Insert Fence and H1 from the context menu. Source
  unchanged. Status visible. Process stays up.
- `ApplySourceEdits` throw during Replace. Find still refreshes. Process stays
  up.

## Leave standing

Same as [DeliveryReview_2026-10-31.md](DeliveryReview_2026-10-31.md): atomic
replace shape, strict UTF-8, recovery lease, Files child reparse not entered
until Q8 is reaffirmed, Compose Enter through `Eol()`. Do not reimplement
Source-view Enter in RedTilde.

## Explicitly not this 0.1

Documents over 65,536 UTF-16 units, full LaTeX/Mermaid, native ARM64 runtime,
physical IME/AT, installer and file association, paired Release performance and
WARP resource qualification.
