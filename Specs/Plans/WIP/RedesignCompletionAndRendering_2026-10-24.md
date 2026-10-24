# Finish RedTilde redesign and DxUi rendering — 2026-10-24

## Progress checklist

- [x] P0: inspect both clean checkouts, exact dependency pin, existing plans and current qualification packets.
- [x] P0: complete three cheaper-agent read-only reviews; lead re-read the admitted findings and primary references.
- [x] P0: reconcile ownership with the existing design, creation and DxUi rich-text audit plans.
- [x] P0: write the implementation sequence, file inventory, compatibility matrices, test gates and stop conditions.
- [x] P0: pass both repositories' validators/format checks, verify inventory integrity, and resolve the independent cold review.
- [ ] A0: revalidate drift, retain baselines, finish the file-by-file review ledger and classify every old finding.
- [ ] A1: fix table command memory safety and other reproduced data/selection corruption before refactoring.
- [ ] A2: contain preparation failures per object, preserve accessible ownership and characterize shutdown completion handling.
- [ ] A3: correct math recognition and accepted-but-wrong notation; reconcile capability/target data.
- [ ] A4: implement TeX layout fundamentals and qualify the expanded KaTeX compatibility profile.
- [ ] A5: correct Mermaid semantics, build complete outcome coverage and expand diagram rendering in ordered slices.
- [ ] A6: finish RedTilde interaction, responsive layout, accessible presentation and exact-pin integration tests.
- [ ] A7: simplify measured hotspots and qualify incremental preparation without changing source/history behavior.
- [ ] A8: repeat the current-source whole-feature and library performance studies; resolve R10/D4c advice gates.
- [ ] A9: complete the final-source build/runtime, input, platform, documentation and release dispositions.
- [ ] Closeout: every required row has evidence; update domain specs and capability claims, then move completed plans to Done.

Status: **IMPLEMENTING; implementation and qualification remain open**. The owner authorized execution on
2026-10-24. The completed planning-only pass is retained below; execution needs its own source-bound native and
performance evidence. Update this checklist with the precise change, regression case and receipt for each
completed item. A checked item needs evidence, not just edited code.

### Execution checkpoint — 2026-10-24

- The owner explicitly retained the seven last-complete performance regressions, including roughly 11% higher
  private memory. An extra retained 4 MiB allocation and shared DxUi focus-update cost are investigation leads,
  not proven causes. Recent optimizations require a fresh complete same-SDK comparison against the actual PR base.
- Revalidate final-source Debug, ASan, ARM64 and the keyboard/menu profiles whose authorization the owner
  reaffirmed. Use the existing warning/desktop lease and verify focus/pointer restoration; preserve original failures.
- Archive evidence, synchronize owning documentation and folded changelogs, and move completed plans to Done
  only when the corresponding gates really close. No previous merge request accepts a performance cost.
- DxUi advanced from the planning revision to `b06f534b36ffc2c0c9671a142d1e51545480be22` before execution.
  Reconcile the new host/focus changes and the Controls.Tests/Support.Tests/Embedded.Tests renames before fixes;
  retain the original planned-at identities as history. RedTilde remains at `c7456854c8f964bb9ea067295e6435f99d426515`.
- Isolate execution beneath `C:\Users\PVB\.codex\worktrees\rt24` in the existing Codex worktree workspace.
  Repository-local `.build` directories there own new outputs. D: has less than 4 GiB free; no retained evidence
  or independently launched application is removed to make room.

## Objective and starting point

Finish the accepted native writing experience, repair relevant defects in both repositories, simplify code where
the benefit is demonstrated, and bring math/Mermaid behavior under an explicit, substantially broader compatibility
and regression contract. Preserve one editable Markdown source and undo history through Compose, Source, Split,
local F2 source, file operations, recovery and all unsupported-content fallbacks.

| Checkout | Planned-at revision | State at inspection |
| --- | --- | --- |
| `D:\RedTilde` | `c7456854c8f964bb9ea067295e6435f99d426515` | clean `main`; redesign integrated |
| `D:\DxUi` | `ceffe76a429d15039ceff1f7dfae7e1d7126323e` | clean `main`; rich-text audit plan integrated |
| RedTilde dependency | `c600993861a337012e2633356c3d35c5e129bbf3`, API 4, target `DxUi` | exact lock; no implicit adoption of later main |

At inspection, `git diff c6009938..ceffe76a -- src include Tests` in DxUi is empty. That is source-diff evidence,
not an attestation that later builds, binaries or environments match earlier receipts. Verify those separately.
All following short paths are relative to the named repository. Read root/scoped AGENTS and local skills before
each implementation slice; source locations below were re-read against the revisions above.

Run first, before changing code:

```powershell
git -C D:\RedTilde status --short
git -C D:\RedTilde diff --stat c7456854c8f964bb9ea067295e6435f99d426515..HEAD -- RedTilde Common Tests Build Dependencies
git -C D:\DxUi status --short
git -C D:\DxUi diff --stat ceffe76a429d15039ceff1f7dfae7e1d7126323e..HEAD -- src include Tests ThirdParty Tools
```

Uncommitted in-scope changes also require review. Re-read changed excerpts, update this checkpoint and preserve
other people's work; do not reset an unrelated checkout. Work on focused `codex/` branches when implementation
starts. Keep independently reviewable fixes separate from geometry redesigns and performance work.

## Existing owners and completion boundaries

This is the execution coordinator for this request, not another copy of the 278-item DxUi audit register.

| Requirement | Existing owner | Use in this continuation |
| --- | --- | --- |
| Accepted appearance and R1–R9/V1–V4 | [Design refresh](DesignRefresh_2026-10-22.md) | preserve implemented behavior; A6 closes actual remaining defects |
| R10 performance/final qualification | Design refresh | A8 supplies evidence; R10 is a summary gate, not a separate optimization backlog |
| C1–C5/C11–C12 product qualification | [Creation plan](RedTilde_2026-10-20.md) | A6/A8/A9 execute the applicable gates and update the owner |
| C6 save race, C7 capacity, C8 typography, C9 compatibility, C10 assets | Creation plan | A0 classifies scope; C9 executes here; do not silently drop the other rows |
| Rich-text defects, simplification and language completion | [DxUi audit](../../../../DxUi/Specs/Plans/WIP/RichTextAudit_2026-10-21.md) | keep its W/T/C IDs; dated [current-source continuation](../../../../DxUi/Specs/Plans/WIP/RichTextAudit_2026-10-21/RedTildeCompletion_2026-10-24.md) supplies verified evidence |
| M5/M6, D4c and rich-editor product gates | [DxUi rich-text parent](../../../../DxUi/Specs/Plans/WIP/RichTextEditor_2026-10-17.md) | close milestones only with their own evidence |
| Shared platform/performance/consumer qualification | [DxUi qualification](../../../../DxUi/Specs/Plans/WIP/ConsumerUiQualification_2026-10-11.md) | no duplicate qualification ledger; exact-pin RedTilde adoption remains app-owned |
| Broader controls/host/tooling review | [DxUi production review](../../../../DxUi/Specs/Plans/WIP/ProductionReview_2026-10-05.md) | A0 revalidates relevant findings and all first-party files; preserve that plan's separately owned holds |
| File drop and other consumers | their existing plans | review relevant boundaries; no implicit RedXe/RedPrism/RedSalamander migration |

The design is already implemented. The [24 October integration packet](../../../Measurements/DesignRefresh_2026-10-24/README.md)
records a latest complete 192-process study with **seven regressions**. Its arms used SDK `cf1393a2`, not the
current `c6009938` pin; later optimizations are outside that cohort. Compose/Split changes were command time
`+13.75%/+5.60%`, private memory `+10.83%/+11.51%` and working set `+4.16%/+4.61%`. Owner CPU was
`+4.90%/+14.38%`; Compose CPU stayed below its band. Completed-edit/clean-frame timing had no resolved regression.
Controls and host activity limit attribution. Earlier v5r3's eight-regression result is distinct historical evidence.

The same packet records final-pin x64 Release **53 behavioral cases and 23 captures**. Older six-profile/image-2
packets use older pins. Neither publication nor this plan accepts a cost or qualifies final-pin Debug, ASan,
ARM64 runtime, physical input, hosted CI or release. R10 remains open until A8/A9 supply the required disposition.

## Review findings admitted for execution

These are source-backed findings or explicit coverage gaps, not results from a new native run. Reproduce each
defect with a regression that fails on the recorded original code before fixing it. Existing audit IDs remain
the implementation owners. S means hours, M roughly a day, L multiple days; estimates include tests.

| ID / priority | Evidence and consequence | Confidence | Effort / fix risk | Owner |
| --- | --- | --- | --- | --- |
| F01 / P0 | DxUi `src/Controls/DxUi.RichTextCommands.cpp:1918–1936` can use `slots[column - 1u]` with column zero in a short padded body row. The header's one-column guard does not protect that raw row. | High, static | S / medium | W1.1, commands-01; A1 |
| F02 / P1 | DxUi `DxUi.NativeMath.cpp:69–100` accepts newlines in text leaves; `DxUi.PreparedMath.h:332–340` rejects multiline/oversized geometry; `DxUi.RichTextEditor.cpp:1337–1347` can reject the document's semantic preview. One formula must not remove unrelated rich content. | High, static path | M / medium | W1.2; A2 |
| F03 / P1 | DxUi `DxUi.NativeMath.cpp:240–241,389–412` models limits/nolimits as a Space atom, so following scripts bind to an invisible atom. | High | M / medium | W1.6/C4; A3/A4 |
| F04 / P1 | DxUi `DxUi.PreparedMath.h:41–52,62–84,303–310` has upright default text and fixed scale ratios, without a display/text/script style model. Admitted ordinary variables, spacing and large operators do not have TeX semantics. | High, compatibility gap | L / high | C4/T1; A4 |
| F05 / P1 | DxUi `DxUi.RichSemanticDocument.cpp:341–345` writes a rejected formula's diagnostic into its parent's title, including a titled link. | High | S / low | W1.5; A2 |
| F06 / P1 | DxUi `DxUi.NativeFlowchart.cpp:253–277,290–298` replaces a state label for quoted aliases but appends colon descriptions to the ID. The pinned state documentation defines both description forms. | High | S / low | W1.7/T2; A5 |
| F07 / P1 | The same parser's generic Label validator at `69–119` is bypassed by state aliases/descriptions and transition descriptions at `253–277,290–298,342–348`. Markup/control/length policy can differ by syntax. This is a conformance defect; no execution of markup was observed. | High | M / medium | W1.7; A5 |
| F08 / P2 investigation | RedTilde `Common/Settings.cpp:120–123`, `Common/Recovery.cpp:124–127` ignore failed completion posts, but `Application.cpp:1992–2000` checks completion after each dispatched message. Queue exhaustion leaves queued wakeups; it does not by itself prove a shutdown hang. Characterize reachable failure before proposing a loop redesign. | Medium; unproved runtime defect | S investigation; any fix M / medium | A0/A2 app; creation C12 |
| F09 / P1 | DxUi `MarkdownInlines.h:1215,1230–1231` applies dollar whitespace/digit heuristics to explicit slash-parenthesis math too. Spaced slash math needs its own delimiter rules and code/HTML precedence tests. | High for heuristic reuse; precedence requires characterization | M / medium | W1.6; A3 |
| F10 / P1 | `ThirdParty/Markdown/profiles/DxUiMath1-target.json` says math is unimplemented and disagrees with effective limits; the Mermaid matrix marks state/class wholly unsupported despite native subsets. | High | S / low | W1.11/T1/T2; A3/A5 |
| F11 / P1 | Current Mermaid corpus asserts 13 Prepared and 92 Unsupported out of 105 original flowcharts; state/class cases mainly check status/counts. Math cases do not establish atom/style semantics. | High, test shape | M–L / low | T1/T2/T4; A4/A5 |

Load-bearing excerpts, retained for drift comparison:

```cpp
// DxUi.RichTextCommands.cpp:1933; column is semantic, slots are raw per-row cells.
if (column + 1u < slots.size())
    Add(slots[column].first, slots[column].last + 1u, L"");
else
    Add(slots[column - 1u].last, slots[column].last, L"");

// DxUi.NativeMath.cpp:240; these must become operator modifiers or reject.
if (name == L"limits" || name == L"nolimits")
    return Make(MathKind::Space);
```

The older audit's remaining 278 entries are **not all revalidated by this pass**. Its 241 confirmed, 16 disputed,
one uncertain and 20 refuted counts describe that earlier source. A0 must retain fixed/superseded/disputed/refuted
dispositions; an old finding is not automatically a current bug. Do not rerun refuted work as a feature request.

## A0 — establish review and test baselines

Priority P0; S–M for baseline/critical drift checks, L for the complete review; low risk. A0a is the source/pin,
critical-finding and baseline gate before implementation. A0b is the full file/finding sweep; it may continue
alongside focused A1–A6 work, but must finish before broad refactoring or a claim of complete code review.

1. Compare both revisions and dependency/source identities. Record actual available build receipts and failures.
   Use the [review inventory](RedesignCompletionAndRendering_2026-10-24/review-inventory.md): 441 tracked C++,
   script, build/workflow, JSON, resource and lock inputs at planning time. It excludes vendored corpora, docs,
   skills and ignored outputs. Its initial review state is pending; hashes are drift detection, not proof of review.
2. Review every first-party inventory row. Add `reviewer`, source hash, callers/ownership checked, relevant test
   names, findings and disposition. Review generated tables through generator/provenance plus boundary tests;
   inspect third-party code at its actual ownership/build/resource boundary. Add new/renamed inputs to the ledger.
   Set `reviewStatus` to `reviewed` or `generated-verified`, fill `reviewer`, `testIds`, `findingIds` and a nonempty
   `disposition`, and fill the four `reviewNotes` strings: `callers`, `ownershipAndLifetime`,
   `failureAndCancellation`, and `testCoverage`. Use an explicit reason where an axis is inapplicable. A pending
   row cannot be closed by assigning an owner alone. Tests may name an existing validator when the row is
   build/resource data rather than runtime code.
3. Partition the review: RedTilde storage/settings/recovery/files/images/crash; app state/chrome/navigation;
   DxUi document/commands/parsers; math; diagrams; semantic geometry/images; host/input/UIA; shared controls,
   rendering/resources; tests/build/tooling. Include failure/cancellation and teardown paths, not only happy paths.
4. Reconcile every historical audit row against current code. All confirmed data-loss/memory-safety defects are
   mandatory fixes. Optional simplifications need a measured or maintenance rationale. Route unrelated platform,
   other-consumer and product expansion rows to their named existing owners rather than deleting them.
5. Before code changes, run current affected explanation and a reproducible correctness baseline. Capture the
   paired Release baseline and semantic fixture before optimization. The semantic fixture must fit within the
   current 65,536-unit bound: about 1,000 short paragraphs, nested lists, a table, images, equations and a graph.
   Record exact bytes, work/owner turns to readiness, DirectWrite creations, retained/peak memory and cancellations.

Verify A0a from each checkout: `./Test-Changes.ps1 -Explain` produces the expected relevant scopes; current selected
native suites pass or each existing failure is retained with source identity; baseline fixtures/receipts are retained.
Verify A0b: the ledger has no unowned or pending row and every old finding has a current disposition. Full review
does not mean every module needs a code change or a new test that mirrors implementation.

Machine check for the eventual A0b completion, from RedTilde. This is expected to fail while the ledger is pending:

```powershell
$ledger = Get-Content Specs/Plans/WIP/RedesignCompletionAndRendering_2026-10-24/review-inventory.md -Raw
$data = [regex]::Match($ledger, '(?s)\x60{3}json\r?\n(.*?)\r?\n\x60{3}')
if (-not $data.Success) { throw 'Missing machine-readable review data.' }
$inventory = $data.Groups[1].Value | ConvertFrom-Json
if ($inventory.entryCount -ne @($inventory.files).Count) { throw 'Review inventory count changed without reconciliation.' }
foreach ($row in $inventory.files) {
    if ($row.reviewStatus -notin @('reviewed','generated-verified') -or
        [string]::IsNullOrWhiteSpace($row.reviewer) -or [string]::IsNullOrWhiteSpace($row.disposition) -or
        @($row.testIds).Count -eq 0) { throw "Incomplete review: $($row.repository)/$($row.path)" }
    foreach ($field in @('callers','ownershipAndLifetime','failureAndCancellation','testCoverage')) {
        if ([string]::IsNullOrWhiteSpace([string]$row.reviewNotes.$field)) { throw "Missing $field review: $($row.path)" }
    }
    $root = if ($row.repository -eq 'DxUi') { 'D:\DxUi' } else { 'D:\RedTilde' }
    if ((Get-FileHash -LiteralPath (Join-Path $root $row.path)).Hash.ToLowerInvariant() -cne $row.sha256) {
        throw "Review evidence is stale: $($row.path)"
    }
}
```

Also compare current tracked/new source paths with the declared inventory scope before this check; removed,
renamed and added files need explicit reconciliation. Never update a hash without repeating the affected review.

## A1 — repair source corruption and memory safety first

Priority P0; M–L depending on reproduced W1.1/W1.3 findings; medium risk. Requires A0a; does not wait for all of A0b.

- In DxUi, reproduce F01 with a multi-column header and a one-raw-cell body, deleting column zero. Cover leading
  and omitted pipes, empty/padded rows, last-column deletion, nested quote/list containers, LF/CRLF and one-column
  table removal. Assert exact resulting source, caret/selection, one transaction and exact undo/redo. Never index
  a previous raw slot without proving it exists; reject/stage atomically before applying edits.
- Revalidate the rest of W1.1 and W1.3. Preserve source punctuation, nested markers, EOLs and escaped pipes.
  Add each confirmed regression to the existing command/parser fixtures before fixing it. Test capacity at
  limit-minus-one/limit/limit-plus-one, including padded cells and reference/inline node admission.
- Use `src/Controls/DxUi.RichTextCommands.cpp`, Markdown block/inline units and the existing
  `Tests/Support/Scope.Tests.RichSemantic*.h` planner fixtures. Extend retained native and embedded editor cases
  for the same commands; a planner-only pass does not prove input/history integration.

Verify: DxUi `./test.ps1 -Configuration Debug -Platform x64 -Suites EditorControls,Embedded,Accessibility`, then
Release and ASan Debug for the affected suites. The original regression must fail against original code and pass
with the fix; preserve that evidence without crashing a person's running application.

## A2 — failure containment, ownership and reliable completion

Priority P1; M–L; medium risk. Depends on A0a; may run alongside A1 in disjoint files while A0b continues.

1. Implement existing DxUi W1.2/W1.5 with explicit domain-spec amendments. A malformed/unsupported/capacity-limited
   formula or graph retains its exact source locally. Unrelated paragraphs, images and other objects remain rich.
   Persistent preparation failure is memoized by the complete source/profile/font/width/direction/resource/binding
   key; device loss/resource generation changes retry through the proper lifecycle. Do not cache cancellation or
   a transient device failure permanently. Preserve allocation-failure policy at library ABI boundaries.
2. Create an owned semantic Math element for both success and rejection; diagnostics attach to it. Preserve
   parent link title/destination and its UIA HelpText. Share the native-object admission ledger for images/math/
   diagrams only after their different lifetime/resource semantics are made explicit.
3. Add injected persistent layout/upload/factory failures and repeated idle turns. Assert bounded attempt counts,
   stable source/history, valid source/caret map, local fallback, repaint/readiness and recovery after a changed key.
   Include a newline/control in math text and an oversized prepared object in otherwise valid Markdown.
4. In RedTilde, investigate F08 before changing the loop. Inspect `Application::Run`, BeginClose/FinishClose,
   SettingsWriter and RecoveryService together. Queue exhaustion has an existing dispatch backlog and therefore
   does not demonstrate a missing wake in the current loop; a fake post failure with an empty queue alone does
   not establish a reachable Windows defect. Characterize both completion orders, the final service to finish,
   queue-pressure recovery, close/cancel and service errors. Record a refutation if the existing checks suffice.
   The [PostMessage contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew)
   documents queue-limit failure and retrieval through GetMessage/PeekMessage. If a reachable failure is proved,
   use a WIL-owned completion event/message-loop integration or another proven bounded wake protocol; no polling
   timer, UI-thread join or post to a retired HWND. Test event reset/lifetime and finished-before-wait races.
5. For a justified completion repair, extend hidden process-exit tests in `Tests/Application.Tests.Core.cpp` and
   app design tests. Assert exact durable settings/recovery contents and real child-process exit. A Busy=false
   assertion or a post-failure hook that cannot occur in production is insufficient defect evidence.

Verify: DxUi A1 command plus resource/fault cases under Embedded; RedTilde
`./test.ps1 -Configuration Debug -Platform x64 -Suites Settings,Recovery,Preferences,RecoveryRestart,Editor`,
then Release/ASan Debug. No activation or real clipboard is needed for these regressions.

## A3/A4 — KaTeX compatibility with correct native mathematics

Priority P1; A3 M, A4 L; medium/high risk. Existing owners W1.6/W1.12/T1/T4/C4/C5.

Implementation anchors in DxUi: `DxUi.NativeMath.cpp` Parser::GroupText/Matrix/Atom/Row and ParseMath own grammar;
`include/DxUi/RichMath.h` owns public data/limits; `DxUi.PreparedMath.h` PreparedMath::Gather/Measure/Place/Advance
own native preparation/geometry. `DxUi.RichSemanticDocument.cpp` owns semantic Math elements;
`DxUi.RichSemanticGeometry.cpp` owns document placement and local prepared-object fallback. Keep parser atoms
independent of device resources. An internal lowering step can add atom/style layout data without exposing a
new public renderer; preserve every source association through that lowering. Review public compatibility before
changing MathNode/MathExpression rather than moving app policy into the library.

**Reference identity:** retain KaTeX 0.16.11 corpus commit `fbbea3573439d51e6568356f5886053b111b1f88` as the
current regression baseline. Live official docs inspected on 24 October identify 0.19.1; a new reference is a
separate versioned profile and corpus migration, never an unnoticed update to old expected output.

1. Fix explicit slash delimiter rules independently of dollar currency heuristics. Characterize closer precedence
   against code spans, escapes, HTML, links and container prefixes. Preserve exact source ranges across LF/CRLF,
   surrogate pairs, display envelopes and over-limit fallback. Add raw source/result assertions, not object counts.
2. Correct symbols and parser/admission defects from W1.6. Limits/nolimits modify an operator; until implemented,
   they must reject honestly rather than parse as invisible bases. Check budget before allocation/push and keep
   one shared node/depth/source budget through roots, groups and environments.
3. Introduce internal TeX atom classes Ord/Op/Bin/Rel/Open/Close/Punct/Inner, explicit text versus math mode, and
   display/text/script/scriptscript style. Model unary/binary reclassification, named upright operators, math
   italics, spacing and italic correction before increasing the command list. Preserve public source compatibility;
   any required public layout/API change follows the API-revision and exact consumer-pin contract.
4. Use measured glyph and OpenType MATH data where available for axis, script/fraction/radical clearances,
   operator limits and stretchy glyph assemblies. Define deterministic missing-table/glyph/font fallback and
   geometry bounds. Replace hard-coded proportional layout only with tested rules. Cambria Math presence is not
   a substitute for handling font fallback. Preparation owns resources; composition creates none.
5. Implement display versus inline placement, centered display envelopes, nested style propagation, fractions,
   roots, simultaneous scripts, large operators/limits, accents and delimiters. Then expand named operators,
   class-annotated symbols, binomials/over-under constructs, fonts and environments from existing C5. Annotation,
   colors, numbering/tags and trusted functions get explicit profile rows and resource/policy decisions.
6. Generate a command/environment compatibility manifest from pinned support data. Every row names accepted,
   partial, unsupported, malformed or policy-excluded behavior, an example, AST/geometry expectations, diagnostic
   and test IDs. Rejecting everything cannot satisfy an advertised supported category. Keep safe-language parity
   distinct from unavailable macros/packages/HTML/actions and from universal LaTeX compatibility.

| Math family | Required independent assertions |
| --- | --- |
| Variables, numbers, operators and spacing | atom kinds, default italic/upright choices, unary/binary cases, relation/punctuation spacing and source ownership |
| Styles and scripts | inline/display differences, style overrides, nested superscript/subscript sizes, baseline and limits/nolimits placement |
| Fractions, roots, accents, delimiters | glyph origins, axis/clearance, ink bounds, scalable assemblies, explicit sizing and missing-glyph fallback |
| Matrices/cases/aligned/gathered | rows/cells/alignment, empty cells, trailing row separators, mismatched rows/environments, line/column source ranges |
| Parser/resource limits | 4,096 source units, 512 syntax nodes, depth 24, at most 16 rows/16 columns and 128 effective cells; boundary and allocation/cancellation sweeps |
| Invalid or excluded notation | malformed groups, unknown commands, controls, forbidden resource/process/HTML/macro operations; exact local source and useful diagnostic |
| Integration | math in headings/links/lists/tables, mixed RTL, hit/caret, F2 apply/cancel/undo, UIA children/name/help, document and read-only inspector |

These are current effective limits, not proposed increases. `RichMath.h`'s 16x16 wording and the target profile
must be reconciled with the parser's 128-cell limit. Change a limit only with explicit measured capacity coverage.
Add a stable AST serializer test oracle and prepared glyph/line placement oracles, including operators whose
accepted output was wrong. Run the same fixtures native/embedded at 96/144/192 DPI; add fractional-DPI rule and
clipping cases at 120/168. Raster checks supplement semantic/geometry tests and account for pinned font/OS identity.

Use existing `Tests/Controls/Scope.Tests.RichSemanticDocument.cpp`, `Scope.Tests.RichSemanticGeometry.cpp`,
`Scope.Tests.RichTextEditor.cpp`, `Scope.Tests.RichTextAccessibility.cpp` and Embedded resource fixtures; add
`Tests/Support/Scope.Tests.MathCorpus.h` only if needed and register new active files. Tests must fail for wrong
spacing/limit placement and parent-title corruption, not merely for parse failure.

**MicroTeX decision:** recommended path is to retain the owned bounded engine and borrow reviewed ideas. The
[pinned MicroTeX row implementation](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/src/atom/atom_row.cpp)
shows atom classification, unary/binary adjustment and font kerning;
its [delimiter factory](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/src/box/box_factory.cpp)
illustrates larger glyph/extension assembly. Its [README](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/README.md)
documents C++17/CMake and a GDI+ Windows path; its [main license](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/LICENSE)
is MIT. Direct adoption is an alternative spike, not an assumed shortcut to KaTeX compatibility. Audit font/data
licenses separately and do not import global mutable state, unsafe ownership or backend assumptions.

Verify after each slice: DxUi A1 suites, deterministic corpus generation/checks, all original 1,324 Markdown
vectors, every chosen profile row and the resource sweeps; then RedTilde Math/Editor integration through A6.
The [KaTeX supported functions](https://katex.org/docs/supported.html) and
[options](https://katex.org/docs/options.html) are behavior references, not a product JavaScript dependency.

## A5 — Mermaid behavior, layout and broader coverage

Priority P1; L; medium/high risk. Existing owners W1.7/T2/T4/C6/C7/C8.

Implementation anchors in DxUi: `DxUi.NativeFlowchart.h/.cpp` NativeFlowchartParser::Preparation::Label/Node/
Statement own current grammar/model; `DxUi.NativeFlowchartLayout.h/.cpp` own CPU placement/routing;
`DxUi.PreparedFlowchart.h/.cpp` own measured text and native retained drawing. Semantic document/geometry
and SemanticExtensions integrate objects with source, diagnostics, admission and UIA. Extend the correct layer;
new families must not accumulate family-specific hacks inside generic flowchart label parsing.

**Reference identity:** keep Mermaid 11.4.1, commit `9868f3a4c3784e214d8b6c74277ca542774e7de9`, its 105
original flowchart examples and family inventory immutable as the regression baseline. Current live docs show
12.2.0 and changed defaults. Record any upgraded parser/layout version, theme, fonts and configuration in a new
oracle packet; never compare an unpinned live renderer to native output and call the difference a defect.

1. Correct state alias/colon equivalence, shared label validation, direction token whitespace and diagnostics.
   Reject flowchart-only shape/edge syntax inside state/class profiles unless that family's grammar admits it.
   Make relation kind explicit rather than inserting a synthetic inherits label. Check titles, members,
   transitions, escapes, shorthand styles and accessibility statements; preserve source fallback for exclusions.
2. Replace count-only expectations with independent normalized model goldens: family, direction, stable node IDs,
   labels, shapes, edge endpoints/kinds/labels, declarations and diagnostics. Check expected issue/reason, source
   range and no partial model on rejection. Cover accepted input, invalid input and valid out-of-profile input
   separately. Update family status data and generator together; a state special-case in an unsupported-family
   test is not the final compatibility design.
3. Prioritize flowchart parity: chained/fan-out edges, edge styles/text, shape coverage, quoted/multiline/entity
   labels, subgraphs/directions and accessible titles/descriptions. Native theme translation must be explicit;
   arbitrary CSS, scripts, actions and directives stay non-executing. Unsupported forms retain the whole fence.
4. Complete state and class semantics: aliases/descriptions, start/end/pseudo states, composites and notes;
   class blocks/compartments, relationships, cardinality, namespaces and generics. Then add sequence diagrams
   with participant/message/activation/loop/alt semantics. A new family needs its own typed model and bounded
   layout, not reinterpretation as a flowchart. W5 adapters may help, but a whole-engine rewrite is not prerequisite.
5. Continue the safe family matrix with ER, pie, gantt, mindmap, then every remaining pinned family. Each gets
   a prioritized implement/defer row with reason, estimated effort and tests. The overall compatibility goal
   stays open while requested families are deferred; report the actual implemented profile, never full Mermaid.
6. Repair layout with shape-aware ports, rank-cycle handling, independent edge lanes, crossing reduction,
   label wrapping/obstacles, self-loops, parallel/back edges and disconnected components. Bound work/memory;
   no over-wide object should silently invalidate unrelated content after A2's spec amendment.

| Mermaid test axis | Required scenarios and oracles |
| --- | --- |
| Original corpus | run all 105 flowcharts and every pinned family fence; zero omitted rows, explicit per-case outcome/reason and immutable provenance |
| Grammar and metadata | equivalence of aliases/descriptions, whitespace/comments/quotes/escapes, state/class members and edges, unsupported directives/styles/HTML/actions |
| Graph semantics | independent ID/label/shape/edge-kind goldens; duplicate declarations, missing targets, Unicode/surrogate/control boundaries and family isolation |
| Layout | all four directions, cycles, loops, fan-out, long/multiline labels, parallel/back edges, subgraphs; finite nonnegative bounds, ports on the actual shape, no unrelated-node intersection |
| Resources | source/node/edge/statement/identifier/label caps; limit boundaries, cancellation at each stage, allocation failure, replacement peaks, width/font/DPI/device changes and idle reuse |
| Source/editor/UIA | list/quote fences and CRLF, complete source spans/diagnostics, object caret/hit/selection, F2 apply/cancel/undo/redo, exact copy/source fallback and accessible alternatives |
| Rendering | native/embedded retained resources, shape-specific raster assertions, theme/high contrast/RTL, 96/144/192 and fractional DPI, WARP/device loss; approved upstream references with pinned fonts |

Use `Tests/Support/Scope.Tests.NativeFlowchartCorpus.h`, `Scope.Tests.MarkdownBlocks.h`,
`Scope.Tests.FlowchartLayout.h`, `Scope.Tests.PreparedFlowchart.h`, `Scope.Tests.SemanticDiagrams.h`, and
the RichMarkdown/SemanticDocument/SemanticGeometry/RichTextEditor control suites. Register added state/class/
sequence corpus fixtures in `Tests/native-test-files.json`. New generator/tool changes follow Tools governance,
inventory/help/callers and offline deterministic tests; do not modify generated expected data by hand.

**Reference-project use:** [mermaid-rs-renderer](https://github.com/1jehuang/mermaid-rs-renderer/tree/3726ccbffe0e8032361eb9668694b24f77858060)
is a useful native-rendering comparison and algorithm source, not a conformance authority. Its
[manifest](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/Cargo.toml)
identifies Rust 2024, version 0.3.1 and MIT. Study its
[stage validation](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/src/layout/flowchart/stage_validation.rs)
for missing/invalid nodes, ports and routes, and
[route-label obstacles](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/src/layout/flowchart/route_labels.rs)
for label clearance. Implement only the reviewed useful ideas with DxUi's ownership/source/UIA model. A Rust/FFI
or SVG backend would add a new build/resource/accessibility boundary and requires an explicit comparison spike.
Do not inherit its advertised speedups as evidence for this product.

Verify each accepted family with the A1 library suites and named corpus cases, then A6 consumer tests/captures.
Semantic equivalence does not require identical browser pixels or coordinates. Correct labels, relation meaning,
connectivity, source access, readability and tested layout invariants are mandatory. Latest-only syntax, including
newer shape/header aliases, needs a versioned probe before being reported as a defect in the 11.4.1 profile.

## A6 — finish the consumer design and interaction

Priority P1; M–L; medium risk. Preserve the accepted target rather than inventing a second visual direction.

- Review `RedTilde/Application.Chrome.cpp`, `Application.Navigation.cpp`, `Application.cpp`, `NavigationModel.h`,
  `UiControls.h` and canonical DxUi Button/Tree/Menu/SingleLineTextEditing/RichTextEditor behavior. Use public APIs;
  the library owns shared interaction, layout and resource fixes. App strings stay in `res/RedTilde.rc`.
- Verify merged caption drag/resize/maximize/system menu, view exclusivity, dirty identity, Save state, keyboard
  hints, overflow and disabled commands. At 800x560, collapse inspector before overlaying Files; never overlap
  controls. Cover long names, long localized strings, high contrast, reduced motion, RTL and text scaling.
- Test selection retention and focus return across formatting, menus, Find/Replace, Settings, Files and F2.
  Test the actual key-message pipeline so Enter/Space cannot insert a stray editor character after button
  activation. Preserve the existing fixed regression. Hidden tests need no desktop focus.
- Extend existing `Application.Tests.DesignRefresh.cpp` equation/source scenarios and
  `Application.Tests.Core.cpp` Math/Editor cases with new compatibility examples. Compose/Source/Split/F2 must
  preserve source, selection, read-only mirror, cancellation, history and recovery, including rejected objects.
- For Files, retain bounded depth/visited-entry limits, no reparse traversal, stale request rejection, filter,
  dirty prompts and the normal Open flow. For source/storage, retain BOM/EOL/UTF-8 rejection, no truncation,
  successful-save dirty reset and recovery Save As. C6's check/replace race remains an explicit storage workstream;
  an extra stamp check cannot be called an atomic conditional write.
- Adopt only a reviewed immutable DxUi revision through `Dependencies/DxUi.lock.json`, restoring via the single
  library targets. Run consumer qualification after each final pin; never copy shared sources or private headers.
- Capture the weekly report and actual math/Mermaid examples through `Application.CaptureScenarios.cpp` and
  `./test.ps1 -Suite Captures`. Keep invented fixtures and scenario/build/pin provenance. Inspect 96/144/192 DPI,
  narrow, light/dark/System/high-contrast, selection/find/focus/settings/F2/fallback states. Proposal art stays labeled.

Verify: RedTilde Core and Captures in x64 Debug/Release/ASan Debug, actual frame output plus behavior assertions,
and current-source UIA client checks where the accessible tree/event is the requirement. A visual match alone
does not qualify source/history, keyboard, physical monitor DPI, IME, touch or assistive technology.

## A7/A8 — simplification and measured performance closeout

Priority P1; L; medium/high risk. Depends on characterization and failure containment.

| Simplification | Order and acceptance |
| --- | --- |
| Reuse unchanged semantic geometry | W3.1 first; full key includes source/block identity, runs, tables/objects, width, direction, typography and resource generation; unchanged output/source oracles and fault-safe publication |
| Incremental parsing and scheduling | W3.2/W3.3; reuse only proven closed blocks, whole-parse fallback, bounded work/cancellation, exact old/new corpus equality |
| Images and native objects | W3.4/W3.5; retain valid decodes/prepared CPU objects across unrelated edits/reflow, separate device resources, reject stale tokens and avoid retaining old large documents |
| Shared utilities | W2 helpers only after searching canonical helpers; consolidate proven identical source/EOL/lexical/scanner semantics with caller tests |
| Lifecycle/diagnostic consolidation | W4/W5 only after profiling/characterization; adapters first, keep per-type ownership and source mapping, no speculative universal framework |
| Consumer structure | extract focused app state/presentation seams only where A0 finds duplication or unsafe coupling; no second editable model or parallel Files/Find cache |

Measure allocation ownership before claiming a cause for R10. Retained icon/text layouts and analysis caches
already have fixes; do not remove them or silently rebase the benchmark. Inspect app source copies/analysis,
editor parse/build/geometry stages, control layouts, SDK allocations and teardown peaks separately. Profiling
instrumentation may perturb results; diagnostic ablations never replace the fixed primary cohort.

Run two distinct app comparisons: correction work versus this continuation's starting product, and original
UI versus redesigned UI on the same final SDK/common harness for R10. Retain original-UI source and bring only
the common measurement harness/pin into that baseline. Do not use the already-redesigned main as evidence that
the redesign itself has no cost. Library changes also need their own paired DxUi comparison against its real base.

RedTilde command protocol, after the baseline has been built and committed in an approved repository-local
checkout and both arms use the same final SDK/common harness:

```powershell
# From the baseline RedTilde checkout, before candidate optimization:
./build.ps1 -Platform x64 -Configuration Release
./performance-paired.ps1 -FreezeBaseline -OutputRoot .build/redesign-baseline-20261024
# From the candidate; use the absolute frozen baseline path returned above:
$baseline = 'D:\RedTilde\.build\redesign-baseline-20261024'
./performance-observed.ps1 -Baseline $baseline -OutputRoot .build/redesign-final-20261024 -Blocks 12 -Cycles 64 -Seed 20261024
./Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest
./Tests/Scope.Tests.Performance.Evidence.ps1 -StudyPath @('.build/redesign-final-20261024/aa','.build/redesign-final-20261024/ab') -Output .build/redesign-final-20261024/audit.json
```

The sample absolute baseline assumes that checkout owns that exact root; substitute the actual returned path.
Every output directory must be fresh. Keep attempts separate; preserve source/compiler/dependency/fixture/render/
host identities, all A/A/A/B slots and refusals. Existing judge bands/families remain unchanged. Qualify hidden idle,
clean/dirty, long sessions, cancellation, close, resize, device recreation and actual device loss separately.

For DxUi use its existing `performance-paired.ps1 -BaselineRevision <recorded-base> -CandidateRevision <candidate>
-Configuration Release -Platform x64 -Blocks 12 -Seed 20261024`; run `-CalibrationAA` with identical explicit
base/candidate revisions separately. The placeholders must resolve to committed local refs; the tool rejects
incompatible controls. Record actual commands in the run ledger. Hosted paired checks, shared D4c and app R10
acceptance are separate decisions. Unstable controls, regressed costs or CI_PENDING cannot become a pass by wording.

Exit: deterministic work/resource tests pass; unchanged-output tests survive optimizations; complete paired
studies and independent audits exist; regressions/inconclusive controls have been resolved or an explicit
owner cost/scope decision is recorded under the owning contract. Do not infer that the owner's earlier merge
request accepted these costs. Retain performance-advice options with measured impact and uncertainty.

## Verification commands and qualification matrix

Commands below were checked against script parameters and test inventories during planning. They were not run
as native qualification in this pass. Execute from the indicated repository, checking `$LASTEXITCODE` after each
native/tool process. Do not continue a loop after a failed profile and report the last profile as the whole result.

| Purpose | Command | Required result |
| --- | --- | --- |
| Both: iteration | `./Test-Changes.ps1 -Explain`, then `./Test-Changes.ps1` | expected affected scopes, no unexplained failures; partial evidence labeled |
| Both: static/tooling | `./validate.ps1`; `./format.ps1 -Check` | all applicable validators pass, no formatting drift |
| DxUi: targeted engine/host | `./test.ps1 -Platform x64 -Configuration Debug -Suites EditorControls,Embedded,Accessibility,Control,Tree,WindowHost,Rendering` | every selected suite/case passes, zero capability skips; add affected Theme/Animation/other suites when explanation requires |
| RedTilde: app | `./test.ps1 -Platform x64 -Configuration Debug -Suite All` | all requested scopes/captures/crash children pass; original failures retained |
| Both: x64 qualification | repeat full applicable tests with `Debug`, `Release`, `'ASan Debug'`; DxUi `./test.ps1 -Full`, RedTilde `./test.ps1 -Suite All` | three distinct source-bound runtime results; real ASan detection probe succeeds |
| Both: six builds | `./build.ps1 -Platform <x64 or ARM64> -Configuration <Debug or Release or ASan Debug>` for all six pairs | six current-source build receipts; ARM64 cross-build is a build result only |
| DxUi: documentation/gallery | `./gallery.ps1 -PublishDocs` after library visual changes | reviewed affected themed sheets and public design/usage docs agree |
| DxUi: PrePush | `./Test-PrePush.ps1 -Explain`, then `./Test-PrePush.ps1` when hosts/CI are available | account for six profiles; native-host gaps/CI_PENDING remain visible |
| RedTilde: PrePush | `./Test-Changes.ps1 -Mode PrePush -Explain`, then `./Test-Changes.ps1 -Mode PrePush` per required profile | applicable local/PR obligations fulfilled or explicitly pending |
| Both: changes | `./Tools/Test-Changelog.ps1`; `./Tools/Fold-Changelog.ps1`; `./Tools/Fold-Changelog.ps1 -Check` | valid notes, generated changelog current, no pending fragments |

Interactive commands are deliberately separate. Honor any still-applicable agreement to the time and existing
warning/lease; a historical successful run alone is not a present scheduling agreement. RedTilde uses
`./test-interactive.ps1 -Platform x64 -Configuration Release -SkipBuild`. DxUi uses
`./test.ps1 -Platform x64 -Configuration Release -SkipBuild -Interactive -Suites Menu,NativeTextInput,MenuResources,MenuResourceScaling`.
Do not add automatic Start without the corresponding explicit agreement. Record foreground/focus/pointer
restoration and preserve refusals. Physical IME, touch, AT, monitor DPI, actual device removal/occlusion and
native ARM64 execution are independent gates; a synthetic event, simulated DPI or cross-build does not close them.

Update `Tests/NativeInventory.json`, `Tests/native-case-catalog.json` and `Tests/test-scopes.json` in RedTilde,
and `Tests/native-test-files.json`/`Tests/test-scopes.json` in DxUi for new cases/files. New files follow
`Scope.Tests.Something.h/.cpp`. In particular, confirm Rich/public-header changes select Accessibility as well
as EditorControls/Embedded; the existing audit's T3 records that scope gap. Use meaningful user-visible/source/
geometry/resource oracles, independent expected data and failure injection instead of implementation-mirroring tests.

## Alternatives and decisions carried into execution

| Choice | Default and tradeoff | When to reconsider |
| --- | --- | --- |
| Native engines versus replacement | extend the owned C++ pipeline and use references for tested algorithms; preserves bounded source/edit/UIA ownership | a measured spike proves a dependency improves correctness and total integration cost |
| MicroTeX adoption | investigate parser/box ideas first; no wholesale import | compare fixed formulas, font/data licenses, allocation/cancel/device lifetimes and native backend effort |
| Rust/SVG Mermaid backend | offline differential/reference tool first | a separate build/ABI/source-map/UIA plan is accepted and proves benefits on target platforms |
| Full parity versus a named profile | overall goal is broader safe compatibility; each supported slice needs semantic/render/editor/resource tests | any excluded family/command must stay visible in the matrix; full compatibility cannot be claimed while it remains |
| Large documents/typography/assets | retain C7/C8/C10 ownership; review their current state and necessary public API seams | explicit product target and paired capacity/resource evidence; no arbitrary constant increase |
| Save-race guarantee | C6 must state an achievable cooperating/noncooperating-writer contract | do not invent compare-and-swap from a second file-stamp check |

Red-team every slice: try to refute the bug, introduce a known-wrong implementation to verify the oracle,
force stale generation/cancel/teardown, inject every observed allocation failure for ownership transitions,
and verify that a rejecting fallback does not mask incomplete promised support. Re-run only checks justified
by changed inputs or an unresolved concern; preserve exact reusable evidence under the repository contract.

## Rejected claims and review limits

- A generic ban-pattern search is not a defect report; COM provider implementations can legitimately implement
  AddRef/Release. Review actual ownership rather than mass-replacing them.
- The Mermaid direction-substring expression is not established memory corruption. Whitespace behavior needs
  grammar tests; default-direction/new alias differences need a pinned-version comparison.
- A proposed ellipse spelling was not established by the local 11.4.1 documentation. Keep it in the version-
  upgrade probe, not the confirmed-defect count.
- App shutdown already checks completion after each dispatched message. The agent's queue-exhaustion hang claim
  is not established because a full queue supplies dispatch work. F08 is an investigation; do not schedule a
  message-loop rewrite solely because a PostMessage return value is ignored.
- A capture, 13 accepted graphs, 1,324 Markdown vectors, older audit count or a green Release build cannot prove
  whole-language compatibility, coverage completeness or current six-profile product qualification.
- Three focused agent audits and lead source rechecks are not a completed line-by-line review of every inventory
  file. A0 remains unchecked. No native tests, builds, interactive captures or performance studies ran in planning.

## A9 — completion and handoff

All must hold before calling the requested work finished:

- [ ] Every first-party review-inventory row has a current hash, reviewer, relevant tests and finding/disposition.
- [ ] Every mandatory confirmed defect has a fail-before/pass-after regression; disputed/refuted rows retain evidence.
- [ ] Supported Mermaid and math profile rows pass independent semantics, geometry/rendering, source/history,
  UIA, negative/admission, cancellation and resource tests. Deferred parity remains named and prevents a full-parity claim.
- [ ] Accepted RedTilde appearance and contextual workflows have current-source hidden captures and behavior coverage.
- [ ] Reviewed immutable DxUi pin restored/built independently; consumer storage/edit/recovery/resource tests pass.
- [ ] Six builds, x64 runtime profiles, ASan detection, applicable WARP/device/resource and native ARM64/input gates
  have exact evidence or an explicit allowed disposition. CI checks belong to the actual candidate bytes.
- [ ] R10, DxUi D4c and shared qualification retain their owning verdicts; no regression was silently accepted.
- [ ] Durable changes are in RedTilde Editor/Core/UI/Testing specs and DxUi Controls/Input/Performance/Build/Testing
  specs, user docs, design/gallery and `capabilities.json`. Plans do not become the sole normative home.
- [ ] Changes fragments are folded/checked in both repositories; no generated build products or private environment
  data are published. Move completed plans to Done and update indexes in the same closeout change.

Stop the affected slice and report if source drift invalidates an excerpt, a regression cannot demonstrate the
claimed defect, a library change needs an unplanned public ABI revision, a reference requires unsafe evaluation,
a resource limit cannot be kept finite, a baseline cannot satisfy identity/control requirements, or a verification
fails twice after a reasonable repair attempt. Continue independent authorized work while an external/native/input
gate is unavailable. Never mark a blocked gate complete, weaken its assertion or erase the original failure.

Future maintainers: a new admitted object/command/family must extend its compatibility row, source/geometry/UIA
oracles and lifecycle tests together. A font/theme/DPI/device change invalidates the appropriate prepared key.
Adding support must not create an alternative editable authority, leak stale object resources or add work to Paint.

## Planning verification — 2026-10-24

- RedTilde `./validate.ps1`: PASS, including specification/index, scope, evidence-retention, changelog and
  performance-protocol tooling checks. Final plan references/index were rechecked after edits.
- DxUi `./validate.ps1`: PASS, all five validators and the complete tooling suite. This includes offline
  Markdown/Unicode/flowchart generation checks; it is not a native renderer or new compatibility run.
- Both `./format.ps1 -Check` and final changelog fold/check: PASS. Affected explanations select documentation
  tooling/formatting only; no executable scope rules were weakened or changed.
- Inventory: 441 unique existing paths with matching source hashes and explicit pending review notes; full
  review remains open. All five PowerShell plan snippets parse. Independent cold review's A0 evidence gap was
  corrected and its follow-up found no remaining actionable issue.
- Native builds, app/runtime suites, interactive captures, paired performance and remote CI: NOT RUN for this
  planning-only change. No implementation or product-qualification checkbox is closed by the checks above.
