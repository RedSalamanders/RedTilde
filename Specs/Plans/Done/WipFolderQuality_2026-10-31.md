# WIP folder quality — 31 October 2026

Status: DONE — scoped implementation/review record; outstanding acceptance is transferred.
Archived: 4 November 2026
Current work: [the single remaining-work plan](../WIP/RemainingWork_2026-11-04.md).
Original: `Specs/Plans/WIP/WipFolderQuality_2026-10-31.md` at `1cd59c08528cc1e20720a45ef833bb9e69c315e4`; SHA-256 `0B6407CD7C9D55B838917D38B9E52FFAB33A90E6F962F9485B21A11E228ECBD5`.
[Original committed record](https://github.com/RedSalamanders/RedTilde/blob/1cd59c08528cc1e20720a45ef833bb9e69c315e4/Specs/Plans/WIP/WipFolderQuality_2026-10-31.md).

The dated record below preserves decisions, source-specific evidence, failures and rejected hypotheses.
Transferred items are not completed features. Old pins, billing blocks, draft/merge claims and historical
queues describe their recorded checkpoints; the new plan owns current status. This archive does not
waive performance, consumer, native-platform, physical-input or release gates.

## Historical record

Historical status: review complete. This document does not change product behavior. It is the
navigation record for [the WIP index](README.md). The next product edit is the
first delivery slice in [the red-team sequence](DeliveryRedTeam_2026-10-31.md),
with a failing test written first.

Date: 2026-10-31
Documentation correction: 2026-11-01. Preserve the recorded owner decisions,
apply current unified CI precedence, and classify RT-18 as defensive hardening.
Reviewed source: `45651052e5868312a1908aabd4a93665abab97c5` (`main`).
Product tree compared with the delivery-review tip
`146a96493f2e69d02fbf00b78a468418031a9146`.

## Outcome

Ship an experimental 0.1 that can hold a real Markdown file. That work is the
unchecked delivery list, and it is still in the source. The WIP folder currently
sends the next session at a merged build-policy plan and a 24 October
qualification diary.

Do this, in order:

1. Follow Phase 1 in [DeliveryRedTeam_2026-10-31.md](DeliveryRedTeam_2026-10-31.md):
   bind Restore to the clicked snapshot, replace the busy-close dialog with
   the approved hidden wait for an admitted save (Q4), flush recovery on
   logoff, enforce the approved five-second recovery cap (Q3), quarantine an
   oversized recovery envelope, survive a second close during a prompt, and
   keep shortcut, save-completion, menu, and
   Find exceptions inside the process. Write the failing tests first.
2. Then the product half of line endings, the conflict dialog, the settings
   header, drafts during Open, the staging-file handle, and Q8's approved
   storage compatibility work. Its remaining locations/images work stays in
   Phase 5. RT-18 is optional defensive hardening, not a confirmed defect.
3. Then truthful `capabilities.json` and the production-ZIP probe gates.
4. Leave ARM64 runtime, physical IME and assistive technology, documents over
   65,536 UTF-16 units, full LaTeX and Mermaid, the installer, and paired
   Release performance on the pending list in `capabilities.json`.

The recorded owner decisions remain authoritative: Q3 requires the five-second
recovery cap, Q4 replaces the busy-close dialog with a hidden wait and restores
the window on failure, and Q8 requires OneDrive/cloud, SMB/NAS, and WSL support.
The later RT-10/RT-11 drops do not revoke those approvals. Update the owning
contracts and acceptance coverage when the requirements land; a missing spec
update does not turn an approved requirement into an unanswered question.

For CI routing and Release test/production roles, the current
[unified CI contract](../../Build/Build_UnifiedCI.md) supersedes conflicting
parts of Q1/Q5/Q16. PRs remain compiler-free. Both native Release test branches
must pass before either separate production Release/ZIP branch starts.

## Method and limits

The original 31 October review was read-only; nothing was built or run for
that review. The product paths `RedTilde/`, `Common/`,
`Tests/`, `capabilities.json`, `package.ps1`, `Dependencies/`, and `.github/`
have an empty diff from `146a964` to `4565105`. The commits in between record
the two delivery plans and a changelog fold.

Re-read against current bytes: Restore and Discard (`RedTilde/Application.cpp`
1120–1150), `AskThen` (1539–1567), Close and `BeginClose` (1724–1787),
`HandleMessage` through 1896, the four Insert commands (1274–1288),
`ReplaceFind` (`RedTilde/Application.Navigation.cpp` 478–480), `ParseRecovery`
(`Common/Recovery.cpp` 60–105), `ParseSettings` (`Common/Settings.cpp` 11–15),
`WriteBytesAtomic` and `SaveDocument` (`Common/Storage.cpp` 66–185), probe
dispatch (`RedTilde/Main.cpp` 149–206), `package.ps1` 1–56, and
`RedTilde/RedTilde.vcxproj` 74–84. The DxUi pin is
`6ed0512c68743c57ac209e22010679d211c4a42b`, API revision 4.

A second pass checked every file-and-line citation in the indexed plans
against this tree, and every DxUi citation against pin `6ed0512c`. The
23,926-line review inventory and the DxUi review ledgers are hash and
narrative records of older commits. They do not carry current line citations,
and their hashes were not recomputed. Hosted Actions was not queried again.

## Accuracy of the indexed plans

| Plan | Still accurate? | Remaining work still there? |
| --- | --- | --- |
| [Delivery review](DeliveryReview_2026-10-31.md) | Yes. Anchors still match. All 13 boxes are open. | Yes. This is the defect list. |
| [Delivery red team](DeliveryRedTeam_2026-10-31.md) | RT-14 through RT-17 remain confirmed. RT-18 is defensive hardening; no reachable capacity exception was demonstrated. The Q3/Q4/Q8 clarifications preserve existing approvals. | Remediation has not started. |
| [Production review](ProductionReview_2026-10-28.md) | The carried-forward product defects still match. Q3/Q4/Q8 remain approved. Current unified CI supersedes conflicting parts of Q1/Q5/Q16; the missing `push` claim is corrected. | Product work remains unimplemented. The delivery sequence owns order; the current CI contract owns routing and test/production gates. |
| [Creation](RedTilde_2026-10-20.md) | S5, S6, and the long-horizon C-items are still open. The 21 October pin, checkout, and "every item remains unchecked" line are stale. | The gates are real. The handoff is not a safe start. |
| [Design refresh](DesignRefresh_2026-10-22.md) | R1–R9 and V1–V4 are implemented. R10's memory half was accepted on 29 October. The status still cites producer `e0908338` and DxUi `086ad82a`. | Command findings, quiet-host, policy, and hosted ARM64 remain. They are shared gates, not a second design project. |
| [Redesign completion](RedesignCompletionAndRendering_2026-10-24.md) | The 29 October "stop memory experiments" decision is real. Later unchecked boxes contradict it. "The consumer retains SDK086" is false at the current pin. | A handful of shared gates remain. Most of the 67 open boxes are frozen snapshots. |
| [Native build policy](NativeBuildPolicyCoherence_2026-10-28.md) | The policy is on `main`. Nine of ten boxes are checked. | The last box is closeout, not compiler work. |
| [RedMonitor](RedMonitor_Adoption_2026-10-23.md) | The adapter checklist is done. "Main advanced to `74c3eb6`" and pin `98c795b` are historical. | The open line is the shared ARM64, input, performance, and release gates. |

### Delivery review and red team

`Command::RestoreRecovery` still copies `recoveryCandidates.front()` inside the
`AskThen` lambda (`Application.cpp` 1136) and erases `begin()` at 1147.
`AskThen` stores that lambda in `afterSave` when the document is dirty
(1562–1565). Discard still retires `front()` only through the worker
(1123). `HandleMessage` still has no `WM_QUERYENDSESSION` arm. `BeginClose`
still retires recovery and calls `files.Close()` without waiting for an
admitted save (1724–1731). The busy-close dialog is still Yes/No (1778–1782).
`ParseRecovery` is still `noexcept` and catches `std::bad_alloc` and
`std::invalid_argument` only, while the text bound is `4 * 65536` bytes
(86–105). [Core_FileSafety.md](../../Core/Core_FileSafety.md) still requires
retirement of the restored candidate only after the replacement is durable,
and a shutdown checkpoint that bypasses the 750 ms debounce.

The four Insert commands still pass LF literals (1274–1288). `ReplaceFind`
still sets `updatingFind` around `ApplySourceEdits` with no catch (Navigation
478–480). `package.ps1` still builds Release with no `-ProductionOnly`, and
the `--self-test` exit-2 check runs only when `testsEnabled` is already false.
`--crash-probe` still runs at `Main.cpp` 194, before the
`REDTILDE_ENABLE_TESTS` gate at 200. The recovery and preferences probes are
parsed at 151–160 with the same shape. `Application.Tests.Performance.cpp`
is the test file compiled with no `RedTildeEnableTests` condition (vcxproj 84);
the sibling test files at 74–83 have the condition. `SaveDocument` calls
`EncodeUtf8`, after rejecting text above `MaximumUnits` (158–185). At the
pinned codec that guard excludes the capacity `length_error`; no additional
reachable `length_error` path was demonstrated. RT-18 is therefore defensive
hardening, not a confirmed failure. `LoadDocument` calls `DecodeUtf8` and
does catch `length_error` (153–155).

`capabilities.json` still lists `strict-utf8-bom-eol-preservation` under
`implemented`. The file-safety contract requires original line endings. The
Insert and F2 Apply paths do not honor that yet.

### Production review

Q1–Q17 record owner decisions from 29 October. Product remediation remains
unimplemented, including Q3/Q4/Q8. Q18–Q22 are still open questions. The
second-pass coordinate is `468cd94`, which is behind `main`.

The current [unified CI contract](../../Build/Build_UnifiedCI.md) supersedes
the older CI and test-matrix instructions in Q1/Q5/Q16: compiler-free PR
checks; main qualification after five quiet minutes; test-enabled native
x64 and ARM64 Release branches; then separate production-only Release/ZIP
branches after their global AND gate. Production outputs exclude test code;
Release qualification still executes the real tests and failure probe.
Debug/ASan extended qualification remains separately required where applicable.
Do not restore the old six-native-job PR matrix or remove Release tests.
Q1's visibility and Q17's history actions remain owner operations; this
precedence note does not execute either.

The remediation order starts with a history rewrite and a new public
repository. That is an owner action with a force-new-repo consequence. It
does not fix Restore. Phase 4 of the red team is the matching place for it.

PR-01's private-repository and required-check claims were not re-queried in
the original review. Its original sentence "There is also no `push` trigger"
is false in current `.github/workflows/ci.yml` (lines 5–6 and the `coordinate`
job at 117–119); the 1 November documentation correction now says so in PR-01.

### Creation plan

S0–S4 are checked. S5 and S6 are open, and that matches `capabilities.json`:
status `experimental`, with upstream qualification, native ARM64, physical
IME and assistive technology, paired Release performance, and release
handoff still pending.

The open-work register says every C-item remains unchecked. That is false for
the parts that landed:

- C8's text scale is a live setting (`scale` 75–200 in `Common/Settings.cpp`
  40–41) and `live-reading-width-and-text-scale` is in `capabilities.json`.
  A user-chosen font family is still absent. `bodyFontFamily` is resolved
  from the DxUi monospace role for the source mirror
  (`RedTilde/Application.Chrome.cpp` 693).
- C9's section already says the bounded math and diagram slices are
  implemented. The checkbox stays open for the rest of the language, which
  matches the pending `full-latex-and-mermaid-language-compatibility` entry.
- Recovery, the Files tree, focus mode, and remote-image opt-in are
  implemented capabilities. They are not open C-items waiting to be started.

The handoff still names pin `dd619ef0`, checkout `Z:\src\RedTilde`, and DxUi
PRs #89–#91 as drafts to merge. The index and `README.md` already say those
pull requests merged on 21 October. The lock is `6ed0512c`. A session that
starts at "Remaining work and session handoff — 2026-10-21" will edit the
wrong tree. The same file contains local profile paths that Q17 still intends
to scrub.

### Design refresh

R0–R9 and V1–V4 are checked. The normative layout lives in
[UI_DesignSystem.md](../../UI/UI_DesignSystem.md). R10 is the one open box.
Its paragraph records the 29 October acceptance of the measured working-set
cost and leaves command findings, unstable controls, quiet-host, policy, CI,
and platform gates open. That split is right. The status banner's "exact
frozen current producer `e0908338` / DxUi `086ad82a`" is an evidence identity,
not the tree at `4565105`.

The HTML and PNG proposal boards are labeled as proposal art. They are not
remaining work.

### Redesign completion

The top checklist's owner decision is the one to keep: measured working-set
cost accepted, memory-repair experiments stopped, other gates left open.
These unchecked boxes below that decision tell the next session to do the
stopped work, or to qualify pins the consumer no longer uses:

- "The consumer retains SDK086" (closeout checklist). The lock is `6ed0512c`.
- "Implement and review private Debug allocation attribution" at `bae6e34e`
  and `bb1a32ff`, both SDK086 (resumed performance checklist).
- "Attribute current app working-set and command costs, implement supported
  feature-preserving improvements" in the same list.
- Repeated "resolve the original-UI working-set flags" boxes in the P22–P28
  chronology, with percentages the 29 October decision already disposed for
  the memory half.
- Snapshot boxes whose own text says a run "is in progress", a disk filled,
  or a draft PR is pending. Those are diary entries.

67 open boxes and 317 checked boxes sit in one 1,659-line file. The open
count is not 67 units of product work.

The sibling ledgers (about 2.4 MB, 23,926 lines of inventory plus the DxUi
source-review files) bind a 24–27 October review to RedTilde `c7456854` and
DxUi `ceffe76a`. Their own status is source-review complete, with runtime and
performance qualification left open. They are evidence. They are not a queue.

### Native build policy and RedMonitor

Both implementation checklists are done. Each file stays in WIP because its
last box repeats the shared ARM64, interactive, performance, and release
gates. The build-policy file is marked ACTIVE in the old index and sorts
first. Its evidence paths are machine-local (`E:\DxUi.Perf`,
`E:\RedTilde.Perf`) and are not in this checkout. The merge handoff already
says the merge did not close production qualification.

Closing either file by moving it to Done is safe only when those shared gates
are written once, in the delivery plan, and the moved plan's text says the
policy or the adapter landed with those gates explicitly still open.
[Specs/Plans/Done/README.md](../Done/README.md) already uses that shape for
the MSVC plans. This review does not move them.

## Priority for a deliverable 0.1

`capabilities.json` already tells the truth about the horizon: experimental,
65,536 UTF-16 units, and a pending list for everything that is not this
release. The folder's sequences do not.

| Order | Work | Why it is this high | Where it lives now |
| --- | --- | --- | --- |
| 1 | RT-04, RT-02 with Q4, RT-01, Q3/PR-04, RT-08, PR-34, one exception boundary (RT-12, PR-35, RT-14), with PR-46 tests that fail first | A writer can lose the file, crash, or terminate on these paths during ordinary Close, Restore, and logoff | Red team Phase 1. Production review had them behind a repository rewrite |
| 2 | RT-03 product half, RT-06, PR-09/PR-10 under Q8, RT-07, RT-09, RT-05 | Everyday edits and settings writes violate the file-safety and settings contracts | Red team Phase 2. RT-18 is separate optional hardening without a demonstrated failure |
| 3 | RT-13, RT-15, RT-16, RT-17, and a `capabilities.json` line that matches phase 2 | A local ZIP and the probe switches overstate what the experimental build is | Red team Phase 3 |
| 4 | Resolve remaining Q18–Q22 choices before their dependent implementations; Q18 is already sequenced as PR-34 | These are the unanswered owner questions. Q3/Q4/Q8 are already approved | Production review; later review drops do not revoke owner decisions |
| 5 | Q17 rewrite, public repository, required checks, PrePush native proof | Owner action. It makes later merges provable. It does not save the file a writer has open today | Production review step 1; red team Phase 4 |
| 6 | Remaining approved Q8/Q13 locations and images work, including cloud-tag handling and real-share/WSL validation | These requirements remain in scope for 0.1; implementation and evidence are still open | Red team Phase 5 and production review Q8/Q13 |
| Upstream | DxUi pin bump for Source-view Enter, the native IME ceiling, and quoted file paste | Real, and blocked on the DxUi plan. Reimplementing them in RedTilde is already forbidden | Delivery review DX-3, DX-4, and the DxUi half of RT-03 |
| Later | C1–C12 that are still actually open: performance acceptance, device removal, capacity, font family, broader math and diagrams, drag and drop, installer, ARM64, physical IME and assistive technology | They are the product after a trustworthy 0.1, or they are gates that stay pending | Creation register, design R10, every plan's last checkbox |

C6, the cross-process save race, stays a named future hardening item in
[Core_FileSafety.md](../../Core/Core_FileSafety.md). It is not Phase 1.
C7, documents above 65,536 units, stays pending on purpose.

## Adversarial study

Each view was checked against the same files. They disagree on what "deliver"
means. The sequence above is the synthesis, not a vote.

### The writer

The product is a local Markdown editor. The paths that matter are Restore on
a dirty document, Close while Save is in flight, logoff during typing, and
Enter or Insert in a CRLF file. A 2% working-set discussion, an allocation
oracle, and a public-history rewrite do not change whether tonight's notes
survive. Phase 1 and the product half of Phase 2 are the release. The
experimental label can stay.

### The release engineer

A fix that lands the way the last ten days landed is another local-only merge.
[ProductionReview_2026-10-28.md](ProductionReview_2026-10-28.md) is right that
the harness returns from `AskThen` without a dialog (`Application.cpp`
1556–1557), so today's tests cannot catch RT-04. The failing test belongs in
the same change as the fix. Hosted CI is an account and visibility problem.
Waiting on Q1 and Q17 before writing that test leaves the crash in the tree
for as long as the rewrite takes. Claim "qualified on `main`" only after a
run that actually executes the native steps. Claim "the crash is fixed" after
the local test goes from red to green.

### The owner-decision loyalist

On 29 October the owner required a five-second recovery cap (Q3) and full
OneDrive/cloud, SMB/NAS, and WSL behavior (Q8). The later delivery drops cited
the file-safety contract before those requirements had landed. Those review
drops do not revoke owner decisions. The 1 November correction carries Q3
into Phase 1 and Q8 into the storage and locations/images slices; update the
owning contracts and tests with their implementation.

Q4 also already chooses the close behavior: replace the busy-close dialog,
hide the window while waiting for an admitted save, retain recovery until
success, and reshow with the error on failure. A plain Open can be abandoned
without a prompt. RT-02's repair and acceptance follow that decision; the
dialog choice is not undecided.

### The plan librarian

[validate.ps1](../../../validate.ps1) requires the WIP index to name every
top-level plan once. It does not require those plans to agree, to be current,
or to be the work a session should start. Before this review the index marked
the build-policy plan ACTIVE and listed the delivery defects seventh and
eighth. The redesign file has 67 open boxes, including instructions to
continue experiments the same file stopped on 29 October. The creation
handoff names a pin that is not `Dependencies/DxUi.lock.json`. An agent that
obeys the folder as it stood will not deliver RedTilde. It will requalify a
DxUi pin from 24 October or fold a changelog for a policy that is already
merged.

The ledger pile is the weight. Markdown under `Specs/Plans/WIP` is about
2.9 MB. `review-inventory.md` is 1.7 MB, about 59 percent of that. The other
24 October review ledgers are about 0.7 MB. The eight product plans together
are about 0.2 MB. The inventory's own header says source review is complete.

### The qualification skeptic

Tidying the folder by moving plans to Done would hide S5, S6, and R10.
Those gates are real, they are already listed in `capabilities.json`, and
they are repeated as the last checkbox of four plans so that no plan can
close. The MSVC plans in Done already show the safer pattern: the completed
slice moves, and the sentence that remains says native ARM64 and hosted
provisioning stay open. Use that pattern for the build policy, RedMonitor,
and the implemented design refresh. Copy the shared gates into one list.
Do not mark the creation plan Done while S5 and S6 are open. Do not mark the
redesign plan Done while its diary still contains live instructions; strip
or banner the contradicted boxes first, in a separate edit.

### What this report refuses

Adding a ninth product queue would repeat the defect this file describes.
The red team already asked to stop review-only merges ahead of Phase 1.
This file exists because the index itself was the hazard, and the index now
points here. The next change is Phase 1 code and tests, not another review.

## Folder-quality findings

### [DOCS-01] Point the index at one delivery sequence

- **Evidence**: `Specs/Plans/WIP/README.md` previously marked
  `NativeBuildPolicyCoherence_2026-10-28.md` ACTIVE and listed it first.
  `validate.ps1` only checks that each top-level plan name appears once.
- **Impact**: The next session starts on changelog closeout for a merged
  policy, or on the redesign diary, and does not reach Restore.
- **Effort**: S. The index in this change is that fix.
- **Risk**: LOW. Plan files stay where they are. Each filename appears once,
  which is what `validate.ps1` enforces.
- **Confidence**: HIGH.
- **Fix sketch**: Keep one sequence paragraph and a register. Done in this
  change.

### [DOCS-02] Retire contradicted instructions inside the redesign plan

- **Evidence**: `RedesignCompletionAndRendering_2026-10-24.md` line 7 stops
  memory experiments; lines 78–80 are unchecked and restart allocation
  attribution and working-set work at SDK086. Line 27 says the consumer
  retains SDK086. The lock is `6ed0512c`.
- **Impact**: A literal reading reopens rejected performance work and
  qualifies the wrong pin.
- **Effort**: M. Banner or strike the contradicted boxes and add one line
  that shared gates live in the delivery sequence. Leave the diary bytes in
  place.
- **Risk**: MED. A careless deletion drops evidence identities the plan says
  to preserve.
- **Confidence**: HIGH for the contradiction. The exact edit should be a
  follow-up, not this review.
- **Fix sketch**: One banner at the top of each superseded checklist: "Do
  not execute. The 29 October decision and the delivery sequence win."

### [DOCS-03] Replace the creation handoff's start instructions

- **Evidence**: `RedTilde_2026-10-20.md` lines 30–47 and 72–91 still direct
  work at pin `dd619ef0`, `Z:\src\RedTilde`, and unmerged DxUi PRs #89–#91.
  Lines 112–117 say every C-item is unchecked. Text scale is implemented.
- **Impact**: The root `README.md` used to call this file the active plan.
  Following it edits the wrong checkout and redoes finished slices.
- **Effort**: S for a dated banner. L if the C-register is rewritten in
  place.
- **Risk**: LOW for a banner. MED if C-items are split without reading each
  section, because several "open" boxes are honest remainder gates.
- **Confidence**: HIGH.
- **Fix sketch**: Banner: start at the WIP index. Then split each C-item
  into landed, remainder, and shared gate. Do that in the Phase 1 change
  only if it stays a banner; a full rewrite can wait.

### [DOCS-04] Preserve approved Q3/Q4/Q8 decisions — corrected 1 November

- **Evidence**: `ProductionReview_2026-10-28.md` records owner approval for
  Q3's five-second cap, Q4's replacement of the busy-close dialog, and Q8's
  OneDrive/cloud, SMB/NAS, and WSL support. The later delivery drops did not
  record a revocation. The domain contracts are updated when the work lands.
- **Impact**: Treating review drops as new owner choices blocked authorized
  work and left RT-02 with the wrong dialog behavior.
- **Effort**: S for this documentation correction. Product implementation
  and its acceptance tests remain open.
- **Risk**: Preserve the recorded approvals and current CI precedence;
  change their scope only with an explicit owner decision.
- **Confidence**: HIGH. The owner choices are already recorded.
- **Fix**: The index, this report, and both delivery plans now preserve those
  choices. Q3 belongs in Phase 1; Q8's storage and locations/images work stays
  in Phases 2 and 5; RT-02 follows Q4. No repeated approval is required.

### [DOCS-05] Correct the stale half of PR-01 — documentation corrected 1 November

- **Evidence**: `ProductionReview_2026-10-28.md` originally said there was no `push`
  trigger. `.github/workflows/ci.yml` lines 5–6 and 117–119 push `main` into
  the shared coordinate workflow.
- **Impact**: A session could re-add a trigger that exists, and treat
  the whole CI finding as untouched.
- **Effort**: S.
- **Risk**: LOW. Leave the private-repo and required-check claims until
  someone re-queries Actions.
- **Confidence**: HIGH for the trigger. The red team's "jobs die at start"
  note was not rechecked.
- **Fix**: PR-01 now says the trigger and coordinator exist, and its Q1/Q5/Q16
  instructions follow current unified CI. Actual hosted acceptance remains a
  separate open gate; documentation changes do not establish execution.

### [DOCS-06] Stop treating the 24 October ledgers as plans

- **Evidence**: `RedesignCompletionAndRendering_2026-10-24/review-inventory.md`
  is 23,926 lines and about 1.7 MB, status
  `SOURCE_REVIEW_COMPLETE_RUNTIME_AND_PERFORMANCE_QUALIFICATION_OPEN`,
  planned at RedTilde `c7456854` and DxUi `ceffe76a`. The DxUi-named review
  files beside it are further hundreds of kilobytes. `validate.ps1` does not
  index the subfolder.
- **Impact**: Every search and every "read the WIP folder" pass spends
  itself on a finished source review.
- **Effort**: M to move the folder next to its measurements packet, with
  the parent plan linking the new path. S to leave the bytes and rely on
  the index.
- **Risk**: MED for a move, because the parent plan and the inventory cite
  each other. LOW for leaving them and not executing them.
- **Confidence**: HIGH.
- **Fix sketch**: This change leaves the files. A later edit can relocate
  them under `Measurements/` or beside the Done plan once the parent plan
  is closed. Do not delete them.

### [DOCS-07] Close finished plans without dropping shared gates

- **Evidence**: Native build policy has one open box, and it is closeout.
  RedMonitor has one open box, and it is the shared gates. Design refresh
  has one open box, R10, whose memory half is decided. Done's MSVC entries
  already keep hosted and ARM64 limits in the completion sentence.
- **Impact**: Four plans look equally active, so none of them closes and
  the delivery list looks like one more peer.
- **Effort**: M, and only after DOCS-02's banner exists for the redesign
  file. Design refresh and the two small plans can move first.
- **Risk**: MED. Moving the creation plan or the redesign diary to Done
  would launder S5, S6, and R10.
- **Confidence**: HIGH for the diagnosis. The moves are a follow-up.
- **Fix sketch**: Move the build-policy and RedMonitor plans to Done with
  an explicit "shared gates remain on the delivery plan" sentence. Keep
  the creation plan in WIP as the long-horizon register, behind a banner.

## Citation check

225 file-and-line citations were extracted from the indexed plans. The
delivery review and the red team match the current source. The behavior they
name is still in the code, and the normative sentences they cite are still
in the contracts.

Still present, and still required by the spec:

- Restore still takes `recoveryCandidates.front()` after the prompt
  (`Application.cpp` 1136 and 1147). [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  still retires that candidate only after the replacement is durable.
- Close still does not wait for an admitted save (`BeginClose` at 1724).
  The contract still says an admitted save stays dirty and keeps recovery.
- There is still no `WM_QUERYENDSESSION` arm (`HandleMessage` at 1759).
  The contract still says shutdown bypasses the 750 ms debounce
  (`Recovery.cpp` 196, `Recovery.h` 26–27).
- `ParseRecovery` still does not catch `length_error` (86–105). The contract
  still quarantines a partial or corrupt envelope.
- Insert still writes LF (1274–1288). F2 Apply still copies the draft
  (1431–1441). [Editor_Markdown.md](../../Editor/Editor_Markdown.md) still
  requires the original line endings, and `capabilities.json` still lists
  `strict-utf8-bom-eol-preservation`.
- The conflict dialog is still Yes/No (`Application.cpp` 1620, `RedTilde.rc`
  78). The contract still requires reload, Save As, or explicit overwrite.
- A bad settings header still returns defaults (`Settings.cpp` 14–15) and
  `LoadSettings` still returns `S_OK` (64–72).
  [Core_SettingsAndCrashes.md](../../Core/Core_SettingsAndCrashes.md) still
  says invalid settings fall back per field and the rejected file is retained.
- [Editor_Markdown.md](../../Editor/Editor_Markdown.md) still puts IME handling
  ahead of commands that could interrupt composition. `Execute` still returns
  while a composition is open (`Application.cpp` 1051–1054).

DxUi pin `6ed0512c` still contains the delivery-review library claims:
`TextInput.cpp` 2660 inserts LF, `NormalizeMultilineLineEndings` at 154 folds
CR to LF, `RichTextEditor.cpp` 2349 assigns CRLF, `Eol()` at 138 copies an
existing terminator, the native text store does not override the ceiling
(`TextStoreTarget.h` 35–38, `DxUi.TextStoreACP.cpp` 201),
`RichDocument.h` 94 is 65,536 units, paste falls back to raw quotes
(`RichTextEditor.cpp` 442–444), `FinalizeAsyncMenuController` is `noexcept`
and calls the host at 4679 (`Menu.cpp` 4643), detach resets the root at
`WindowHost.cpp` 1535, and both document edits throw `length_error`
(`RichDocument.cpp` 710, `RichTextCommands.cpp` 1006–1011).

These citations are off. The behavior is still in the tree, at a nearby line
or under another name:

| Cited | What is there now | The point |
| --- | --- | --- |
| `Application.cpp:2209` show state | `ShowWindow(SW_SHOWNORMAL)` is at 2217. No `nCmdShow` symbol. | The launch show state is still ignored. |
| `Main.cpp:195` throwing `exists` | `CrashProbe()` is at 195. `std::filesystem::exists` is at 208. | The throwing overload is still used. |
| `Application.cpp:1664-1665` draft Open | The direct `apply()` is at 1666–1667. | A slow Open still replaces text under an open draft. |
| `Application.cpp:1253` `"link text"` | The string is at 1255. | The placeholder is still inserted. |
| `RedTilde.vcxproj:53-63` test sources | Those lines are delay-load settings. The test files are at 74–84. | `Application.Tests.Performance.cpp` is still compiled with no test condition. The others compile when `RedTildeEnableTests` is true, which is the default. `package.ps1` still does not pass `-ProductionOnly`. |
| `DocumentInfo.cpp:62-75` outline | 62–75 is the fence branch. ATX headings and the 128 cap are at 81–96. | There is no Setext parser, so Setext headings are still omitted. |
| `Files.cpp:342-348` reparse fixture | `Common/Files.cpp` 342 is natural-order sort. The junction fixture is `Tests/Application.Tests.Files.cpp` 342–348. | The fixture exists. It is a junction, not a cloud-tag case. `Common/Files.cpp` 115–122 still refuses every reparse root. |
| `Application.Navigation.cpp:217-245` no equality check | `FolderCompleted` starts at 219 and checks root and generation, then always calls `RefreshFiles`. | A content compare is still absent. "No equality check" is too broad. |

These citations no longer match, and the described option is not in the pin
or the current workflow:

- Redesign F02, `DxUi.NativeMath.cpp:69-100`. Those lines map math alphabets.
  The text-mode scan collapses a newline to a space at line 143. It does not
  keep it as another line. `PreparedMath.h:332-340` is a style branch
  (`mathbf`), and that header has no multiline rejection.
- Redesign F03, `DxUi.NativeMath.cpp:240-241,389-412`. Line 240 is `lceil`.
  The symbol table at 389 is ordinary operators. `\limits` and `\nolimits`
  set `MathLimits` at 557–561. They are not a Space atom.
- Redesign F05, `DxUi.RichSemanticDocument.cpp:341-345`. That is the start of
  `Inline()`. A rejected formula's title is set on the formula element at 411,
  not on a parent link.
- Redesign F06, `DxUi.NativeFlowchart.cpp:253-277,290-298`. That is statement
  and direction parsing. State descriptions replace or append a label at
  110–126. The behavior exists. The line numbers do not.
- Redesign F04, `PreparedMath.h:41-52`. Those lines are geometry structs.
  The fixed script scales are at 68–73, and `displayStyle` exists at 75.
  "No display style model" overstates the pin.
- Production review `ci.yml:14` and `ci.yml:47-48`. Line 14 is a
  workflow-dispatch input. Line 47 is the pull-request resolve step.
  `CHANGELOG_BASE` is not in this workflow. The `push` trigger for `main`
  is present. The shared engine's changelog base was not re-read.
- Production review PR-13 says [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  requires image initialization to degrade. The contract says a recovery
  failure stays visible and leaves editing and Save available. It does not
  mention image startup. `Application.cpp` 203–204 still fails `Initialize`
  when either recovery or image startup fails.

`Build/NativeBuildPolicy.targets` 15–17 still injects the RedXe, RedPrism, and
RedSalamander vcpkg include paths, which is what S14 says to remove.
`RSBuildCompilerDebugInformation` is still at lines 48–49.
`Tests/Tooling.Tests.BuildConsole.ps1` 6–10 still names a RedSalamander
sandbox. The warning-suppression blocks listed for S12 are still at the cited
lines.

## Rejected conclusions

- The delivery items were fixed after the reviews. The product diff from
  `146a964` to `4565105` is empty, and the anchors still match.
- C8 is entirely unstarted. Text scale is implemented. Font-family choice
  is not.
- The native build policy is unfinished compiler work. The policy is merged.
  The remaining box is closeout.
- PR-01 is untouched. The missing `push` trigger is in the workflow. The
  private-repo and required-check claims were not re-proven.
- Every open redesign checkbox is current work. The file contradicts itself,
  and many boxes describe runs that the same file later supersedes.
- Moving every checked plan to Done is the quality fix. Shared gates would
  survive only if they are copied once onto the delivery plan first.
- This review should also patch Restore. That is Phase 1, and it was not
  the requested deliverable.

## Folder shape after this change

The index names one sequence and a register. The 1 November correction
clarifies owner-decision precedence in the linked delivery plans, marks
conflicting Q1/Q5/Q16 CI instructions superseded, and classifies RT-18 as
defensive hardening without a demonstrated failure. Historical source and
runtime evidence remains unchanged. `README.md` points at the index instead
of the 21 October handoff.

Still to do, as folder work, after Phase 1 starts and without blocking it:
DOCS-02, DOCS-03's banner, DOCS-05's hosted-state follow-up, and
DOCS-07. DOCS-06 can wait.

A further review-only pull request ahead of Phase 1 is a regression.
