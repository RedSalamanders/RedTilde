# RedTilde remaining delivery and qualification

Status: ACTIVE
Updated: 4 November 2026
Owner: RedTilde maintainers; shared controls stay with DxUi.

This is the **only active RedTilde plan**. It replaces the ten overlapping planning queues listed in the
[consolidation record](../Done/WipConsolidation_2026-11-04.md). Superseded records preserve findings, decisions,
source identities, failed runs and proposal assets; they are references, not additional execution queues.
Domain contracts under `Specs/Core`, `Specs/Editor`, `Specs/UI`, `Specs/Build` and `Specs/Testing` remain authoritative.

## Verified starting point

Reviewed main: `1cd59c08528cc1e20720a45ef833bb9e69c315e4` ([PR44](https://github.com/RedSalamanders/RedTilde/pull/44)).
The starting consumer lock was DxUi API 4 at `703a96c788a79c1e7b28097226a2ca0a735f11bc`, retained on
upstream main through [DxUi PR163](https://github.com/RedSalamanders/DxUi/pull/163). The current quote-margin
adoption uses published API-4 commit [`8c6c2818`](https://github.com/RedSalamanders/DxUi/commit/8c6c2818d084e6124fb2c08eddc00bdba7a9d2aa),
including the sequence-label Debug allocation repair;
its x64 Release/Debug/ASan consumer checks pass in the [scoped closeout](../Done/QuoteMarginsAndScrollbars_2026-11-04.md).
Other qualification stays with T-Q1/T-Q2. Do not restore an older
SDK086/SDK702 development pin merely because a historical checklist names it.

The current diagram-label adoption advances API 4 explicitly to `2a7e7c28` in
[PR47](https://github.com/RedSalamanders/RedTilde/pull/47). Its
[scoped closeout](../Done/DiagramLabelBackgroundsAndClosePrompt_2026-11-04.md) records six builds, all 75 cases
and 36 captures in each x64 profile, the real ASan probe and the accepted duplicate-Close rule. Current-pin
native ARM64, physical/interactive and performance acceptance remain separately owned below.

| Completed scope | Evidence and limit |
| --- | --- |
| Persistent folder root, Open Folder/Open File, restart restoration and expanded public examples | [Folder/sample closeout](../Done/FolderNavigationAndSamples_2026-11-03.md); [packet](../../../Measurements/FolderNavigation_2026-11-03/README.md). |
| Native Mermaid display corrections and bounded sequence syntax expansion | Published exact pin above; [syntax packet](../../../Measurements/MermaidSyntax_2026-11-03/README.md). Full Mermaid parity is deferred. |
| Measured arrow spacing, table/code insets, accent task markers and persistent optional completed-task strike-through | [Comparison and original evidence](../../../Measurements/DiagramSpacing_2026-11-03/README.md), [publication receipt](../../../Measurements/DiagramSpacing_2026-11-03/publication.json). |
| Current x64 Release implementation checks and source publication | Zero-warning build, 73 native cases, complete noninteractive pre-push, 32 standard captures, and 30 original focused Light/Dark/DPI captures. PR44 checks passed. These are source-bound results, not product-wide acceptance. |
| Design refresh, common build policy and independent RedMonitor client boundary | Scoped implementation is archived below. Their current-candidate performance, physical-input, platform and release gates remain open here. |

The approved narrow memory-cost disposition remains narrow. No functional pass, screenshot, PR merge or
ordinary CI success accepts R10, a measurement policy, quiet-host behavior or another candidate's performance.

## Execution order

T-U1's prepared diagram-label correction and T-S1's accepted duplicate-Close behavior have their scoped
closeout, following quote padding and themed scrollbars. Continue the remaining T-S1 work, followed by T-S2.
T-Q1 is the first **qualification** repair and can be
investigated independently. Do not start another redesign, repin the SDK or rerun unchanged failed studies
instead of resolving these owners. Every implementation slice updates its domain contract and has a
meaningful failing-then-passing regression where applicable.

<a id="t-s1"></a>
### T-S1 — recovery, shutdown and exception safety

- [ ] Bind Restore to the exact snapshot clicked before entering a dirty-document prompt (RT-04/PR-06).
- [ ] Complete the approved close-during-save contract: hide while an admitted save completes, retain recovery
  until success, and reshow with the failure; abandon an Open in flight (RT-02/PR-05, Q4).
- [ ] Implement immediate recovery submission on session end, bounded settlement and restart presentation
  (RT-01/PR-03, Q2); keep the 750 ms quiet debounce and enforce the approved five-second staleness cap
  under continuous typing (RT-10/PR-04, Q3).
- [ ] Quarantine oversized checksum-valid recovery envelopes and retain recoverable contents on failure (RT-08).
- [x] Implement and qualify Q18: keep the existing Save/Discard/Cancel prompt and ignore duplicate native
  or command Close (PR-34). The hidden regression covers Cancel and a fresh Discard in all three x64 profiles.
  [Scoped closeout](../Done/DiagramLabelBackgroundsAndClosePrompt_2026-11-04.md)
- [ ] Contain shortcut, menu, save-completion and Find failure paths without leaving `updatingFind` latched
  (RT-12/14, PR-07/35).

Owner: `RedTilde/Application.cpp`, `Application.Navigation.cpp`, `Common/Recovery.cpp`, `Common/AsyncFiles.h`.
Acceptance: deterministic Restore+Discard, close-during-save success/failure, continuous typing, logoff,
malformed envelope, second close and size-limit tests. See the retained
[Phase 1 detail](../Done/DeliveryRedTeam_2026-10-31.md#phase-1-acceptance).

<a id="t-s2"></a>
### T-S2 — lossless files and storage semantics

- [ ] Preserve the document terminator for F2 Apply and insertion commands (RT-03 product half), hold Open
  completion while a local-source draft is open (RT-05), and keep a bad settings header on disk (RT-09).
- [ ] Implement the approved single conflict surface: Reload, Save As, Overwrite, Cancel; distinguish failed
  readback from a real content conflict and avoid poisoning the next save stamp (RT-07, Q6).
- [ ] Close staging handles before failure cleanup (RT-06), document the achievable cross-process save-race
  contract (C6), and implement the approved replace fallbacks, sharing, cloud tags and same-target stamping
  for OneDrive/cloud, SMB/NAS and WSL (PR-09/10/31, Q8).
- [ ] Preserve symlink/hard-link identities under the approved distinct write policies (Q12), and expose
  specific unsupported-encoding/capacity errors under the current 65,536 UTF-16-unit limit (Q7).

Owner: `Common/Storage.cpp`, `Common/Settings.cpp`, application save/open paths and
[file safety](../../Core/Core_FileSafety.md). Real-share/WSL evidence is required before support claims.
RT-18 remains optional defensive hardening, not a newly confirmed defect.

<a id="t-u1"></a>
### T-U1 — remaining editing and product interaction

- [x] Adopt the shared contrast-aware diagram-label background correction and verify actual Light/Dark,
  96/192-DPI native captures at exact API-4 pin `2a7e7c28` without changing Mermaid source or history.
  [Original comparisons and exact qualification](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/README.md)
- [x] Adopt the shared quote/callout inner-margin correction and enable the native themed document scrollbar
  in Compose, Source and Split; cover overflow, paging/dragging, themes, DPI, narrow layout and source/history
  preservation with hidden native tests and captures. [Scoped closeout](../Done/QuoteMarginsAndScrollbars_2026-11-04.md)
  records the exact pin, all three x64 profiles, ASan detection and actual hidden captures.
- [ ] Qualify Source-view Enter, native IME capacity and quoted file-paste behavior through the exact adopted
  DxUi pin (DX-3/DX-4 and the RT-03 shared half); do not implement competing editor logic in the app.
- [ ] Complete the agreed real-keyboard `Equation.F2InPlaceEditUndoAndExit` check through the existing restoring
  desktop lease. A hidden test does not close this gate.
- [ ] Implement remaining Q9/Q14 window placement, work-area clamping, launch show state, Find case/repeat,
  confirmed explicit link opening, text-scale shortcuts and last-document/session startup policy.
  Last-folder restoration is already implemented; last-document/session startup is a separate requirement.
- [ ] Reconcile the current first-party review inventory and retained app/control findings before closing A2/A6,
  including source/history, responsive layout, accessibility and actual interaction coverage.

The approved Files root policy Q11 is completed by the folder-navigation slice; it is not a new task.
Details and original counterexamples remain in the
[delivery review](../Done/DeliveryReview_2026-10-31.md) and
[review inventory](../Done/RedesignCompletionAndRendering_2026-10-24/review-inventory.md).

<a id="t-i1"></a>
### T-I1 — images and supported locations

- [ ] Complete Q13: per-document consent, explicit local-network override, address validation at connection
  time, same-share image policy, useful failure reasons, Reload images and bounded event-driven refresh.
- [ ] Resolve proxy policy Q19 before changing direct-only remote fetching; preserve all original drops/refutations.
- [ ] Apply Q8/Q13 real-location acceptance, and make any managed-image-assets/drag-drop adoption an explicit
  app-owned scope (C10), with native resource, cancellation and teardown evidence.

<a id="t-p1"></a>
### T-P1 — R10 performance and resource acceptance

- [ ] Attribute current command/working-set costs using frozen, source-matched inputs. Preserve the accepted
  memory-cost advice disposition and the unresolved command/control, quiet-host and shared-policy gates.
- [ ] Review the private Debug allocation-attribution design before app collection; retain exact live IDs,
  public CRT quiet-point barriers, EXE/PDB caller identities, capacity/overflow refusals and all original failures.
- [ ] Implement only supported, feature-preserving improvements; predeclare fresh matched studies and
  independently replay them using qualified controls. Never rerun an unchanged cohort seeking a favorable result.
- [ ] Qualify long-lived GPU/kernel resources, actual device removal and physical visibility/occlusion separately
  from synthetic WARP/device-loss coverage. Measured simplification, including Q10 label-cache changes,
  requires the agreed non-regression evidence.

This owns R10, C1/C2, S5, A7/A8 and the surviving original-UI performance findings. The dated SDK086/SDK702,
source-ink, index, heap-oracle and timing investigations are historical evidence in the
[archived coordinator](../Done/RedesignCompletionAndRendering_2026-10-24.md), not instructions to resume old binaries.
Unexplained historical failures remain unexplained unless a bounded investigation establishes their causes.

<a id="t-q1"></a>
### T-Q1 — repair current hosted qualification

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
  pass at `330b9ac6`. PR48 records its separate publication; the combined main gate still requires a fresh successful run.
- [ ] After a reviewed correction, run the canonical main qualification and verify both native Release test roles
  and their failure probes before either separate production role. Production was skipped in the failed run.

This is a current tooling/qualification failure, not the older account/billing obstruction. PR44's compiler-free
checks and local 73-case passes remain valid in their original scopes.
Admission artifact expected/downloaded digests match in both regression branches and completion. The logged
`digest-mismatch: error` is a download-action input policy, not a reported integrity failure; that mistaken draft
follow-up is removed. Completion correctly reported failed native qualification.

<a id="t-q2"></a>
### T-Q2 — exact-pin adoption, native/physical acceptance and release

- [x] Qualify the current API-4 diagram/Close implementation at `2a7e7c28`: six zero-warning/zero-error builds,
  all 75 cases and 36 captures in each x64 profile, process/recovery/crash/performance-smoke checks and real ASan
  detection. [Source-bound evidence](../../../Measurements/DiagramLabelBackgrounds_2026-11-04/qualification.json)
  preserves the final-profile storage admission refusal and its exact resume. This does not close native ARM64 or physical gates.

- [x] Complete the six local builds at API-4 pin `8c6c2818`, all 74 cases in each x64 profile, real ASan
  detection, the complete nine-scope Release replay and canonical validators/formatting. The
  [scoped record](../../../Measurements/QuoteMarginsAndScrollbars_2026-11-04/qualification.json) retains exact identities.
- [ ] Complete applicable native ARM64 and physical/input qualification. Cross-builds and old-pin matrices
  do not establish current runtime support. Retain the 90-second loaded-host Editor timeout and its same-binary
  sequential pass as separate observations; diagnose that responsiveness boundary with T-P1 before accepting it.
- [ ] Qualify real keyboard/focus/clipboard, physical IME, UIA/assistive technology and touch on named scenarios;
  arrange interactive timing with the user before taking focus.
- [ ] Verify production-only sources, all test/probe refusal paths, native/production output separation and
  clean-extraction smoke. Recheck RT-13/15/16/17 against the current unified implementation rather than applying
  the obsolete rule that removes tests from every Release build.
- [ ] Coordinate future checked-failure/API changes with the
  [single DxUi plan](https://github.com/RedSalamanders/DxUi/blob/main/Specs/Plans/WIP/RemainingWork_2026-11-04.md) and active
  [API 5 PR162](https://github.com/RedSalamanders/DxUi/pull/162). Keep API 4 / `2a7e7c28` until a separate
  reviewed consumer migration has matching headers, library, adapters and fresh qualification.
- [ ] Complete C11/C12/S6 release decisions: packaging, any installer/file associations, exact release handoff,
  truthful capability limits and publication evidence. Source merge is not product release acceptance.

The existing RedMonitor header/client boundary stays independent; its physical/performance/release acceptance
is included here, without rebuilding or packaging the separate viewer.

<a id="t-g1"></a>
### T-G1 — evidence, tooling and planning administration

- [ ] Execute the previously approved Q17 privacy/history program only as its separate coordinated operation:
  retain private originals; inventory and scan all refs; prepare a reviewed rewritten copy and commit map;
  preserve the original repository privately; reconcile source citations, worktrees and consumer pins before
  the owner's repository/visibility operations. This cleanup does not rewrite history or change visibility.
- [ ] Verify current protection/access and reconcile obsolete CI/capability claims. Keep PR checks compiler-free,
  main native qualification after five quiet minutes and the global test-before-production gate.
- [ ] Audit Q15's legacy header validators and shared-module migration by actual callers before retiring tools;
  keep paired-study tooling supported and follow tooling governance for any later change.
- [ ] Diagnose the hosted-MSVC fixture launch refusal at observed 272-character paths in the isolated
  checkout. Canonical short-path full validation passes; retain the failed long-root run separately and
  coordinate the analogous SDK tooling boundary with D-T1.
- [ ] Reconcile dispositions of remaining production/delivery findings and rejected worktree/cache cleanups;
  never treat earlier removal refusals as permission for a new cleanup.
- [ ] Apply normal Changes fragments and a separate post-merge fold when authorized. No cleanup changes the
  shared fold cadence, manually edits CHANGELOG, or consolidates existing fragments outside its governing workflow.

## Decisions preserved

Q1–Q17 remain recorded in the [owner decisions](../Done/ProductionReview_2026-10-28.md#owner-decisions--29-october-2026).
In particular Q3's five-second cap, Q4's hidden wait and Q8's cloud/share/WSL scope are approved requirements;
RT-10/11's old drops did not cancel them. Current unified CI supersedes obsolete Q1/Q5/Q16 matrix instructions.

| Open owner item | Current disposition |
| --- | --- |
| Q19: system proxy, direct-only failure or a setting | Decide before T-I1's proxy work. |
| Q20: size-limit command behavior | Decide unchanged-document error versus preemptive disable before changing the capacity UX. |
| Q21: release cut | Settle the application release command and exact validator transition; the library's support does not migrate this app implicitly. |

<a id="t-deferred"></a>
## Explicitly deferred scope

Full LaTeX/Mermaid parity, wiki navigation/embed resolution, documents above 65,536 UTF-16 units, broad font-family
choices (C8), export/multi-document features and optional new product surfaces remain separately selected work.
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
