# Folder navigation, public samples and rendering review — 3 November 2026

This packet records the persistent Files root, distinct Open File/Open Folder commands, expanded public
examples, native fenced-code colors and corrected display-math examples. It does not establish full Markdown,
KaTeX, Mermaid, native ARM64, physical-input or performance qualification.

This is the original app-only checkpoint. The later canonical DxUi
[display fixes](../MermaidDisplay_2026-11-03/README.md) and
[syntax expansion](../MermaidSyntax_2026-11-03/README.md) have their own exact pins and evidence;
the historical pin below describes this slice only.

## Source and baseline

DxUi remains pinned at `5fc4d4c383bfa4891ee57a138924107bbbf11a2e` (API 4). Its private toml++ 3.4.0 and
rapidyaml 0.16.0 dependencies validate closed document-start metadata; they are not RedTilde's settings codec
and do not execute document content. No DxUi sources or pin changed in this slice.

The baseline is the clean RedTilde branch head before these edits. Frozen binary/build/compiler/header evidence
is retained locally at `.build/performance/folder-rendering-20261103-baseline`. Its successful native run is
`.build/evidence/test-runs/20261103T102734755Z-40676-5d30b966bf704499a8b4d2664057fc12/payload/receipt.json`:
12 Storage, 4 Settings and 49 Editor cases passed. Its single-process initial benchmark reported 56.4 FPS,
40.0 MiB private memory and 0.20 ms dirty edit/render; these are not paired acceptance measurements.

The first candidate built x64 Release with zero warnings/errors and passed 12 Storage, 4 Settings and 51 Editor
cases, including restart, nested/external/failed opens, unsaved edits during folder selection, hide/reopen,
idle quiescence and an actual colored-versus-ordinary code pixel comparison. Receipt:
`.build/evidence/test-runs/20261103T103501168Z-2724-e9ae7c9dbd6c4477ba6aaf9c05f04fa1/payload/receipt.json`.
The later math-capture correction requires and receives fresh final evidence; this earlier run keeps its own identity.

## Actual sample renders

Nine public documents were loaded and rendered through the owned hidden, non-activating app harness at 96 DPI,
1440×900. The sample receipt retains document, executable, source and capture hashes. Source identity:
`5eabd933b4e43cf390f7e5cb91ecc08490aa5e92025a52714e0b8b5c52c09a26`;
executable SHA-256: `B31CFAB1456829266CEEA988113A06D26AB95E11BE19293BA4FDDC952A0F0A81`.
Raw captures and per-image scenario provenance: `.build/folder-samples-20261103/`.
These show invented sample data, not the desktop. Rendered startup views do not establish that every lower
section was visually reviewed at every DPI. Native ARM64 and real focus/file-picker interaction remain separate.

Inspected captures confirm headings, emphasis, Unicode, quotes, tasks, extension presentation and a collapsed
metadata Properties section. The dedicated code capture confirms distinct keyword/literal/comment colors and
the native language/line presentation. It also shows that code labels/line numbers have weak light-theme
contrast, a remaining shared presentation issue.

## Rendering gaps

Correction after the shared-renderer follow-up on 3 November: the exact pin already supports bounded sequence,
pie, ER, Gantt and mind-map profiles as well as flowchart/state/class. The earlier feature comparison understated
that support. Its code/task observations and all historical source/binary/performance identities below remain
unchanged. Shared fixes and consumer adoption are tracked in the delivery-adoption plan, with new evidence kept
separately from this folder-navigation packet.

Reviewed captures: [showcase](markdown-showcase-md.png), [syntax options](syntax-options-md.png),
[YAML metadata](front-matter-yaml-md.png), [TOML metadata](front-matter-toml-md.png).
Each PNG has an adjacent `.scenario.json`; [the sample receipt](sample-captures.json) retains all nine raw identities.

The reference's [published feature description](https://markdownviewer.org/) was read on 3 November 2026.
Its editor/screenshot endpoints could not be fetched by the web reader. The comparison is based on that primary
feature description, RedTilde/DxUi source and actual RedTilde captures, not a cross-renderer visual-equivalence claim.

- Fixed here: the app had not opted into DxUi's existing bounded code tokenizer; the welcome and math capture
  fixtures had stale one-line display delimiters. Public examples now exercise a wider ordinary Markdown surface.
- Observed in the new showcase capture: task rows display both a bullet and a checkbox. DxUi's pinned
  `src/Controls/DxUi.RichSemanticDocument.cpp` emits `Marker(...)` before emitting the task object (around lines
  1577–1589). The canonical fix belongs in DxUi, with a reviewed new consumer pin and task interaction/source tests.
- Code language badges and line numbers need contrast review in light themes; the screenshot is evidence of
  the appearance, not a measured WCAG contrast result. Broader tokenizer languages and an application code-copy
  button remain future work.
- The reference advertises sequence diagrams and KaTeX; the current pin already has bounded native sequence
  support. Unsupported Mermaid/math notation stays editable source. Full language support remains with the
  existing rendering coordinator.
- Tabs, multi-document comparison, synchronized columns and PDF/HTML/PNG export/print are separate product
  features advertised by the reference; RedTilde's current app commands expose a single document and Save/Save As.

## Retained refusals and environment repairs

The original stale `-SkipBuild` refusal was retained. Two sandbox baseline build attempts failed while pinned
7zip tried to open stdin (`CreateFileW`, access denied); the authorized outside-sandbox build passed. The first
sandbox baseline test run failed creating its owned junction in `Files.ReparseLeavesNeverTraverse`, at
`.build/evidence/test-runs/20261103T102714150Z-55384-54daf99f216045e6bee9490d98b0a647/payload/receipt.json`.
The subsequent outside-sandbox run is separate evidence, not a rewrite of that failure.

The initial general validator retained sandbox Git/junction/access refusals, a pre-existing missing
`WipFolderQuality_2026-10-31.md` index entry, stale ARM64 dependency identity, and an absent `.ci-engine` checkout.
The index was repaired; ARM64 was restored through the canonical pinned workflow; the ignored CI engine was
checked out at the exact `.ci/project.json` pin `19710a8aeac979a82e83bec82e8b866cee3d3deb`.
The first affected `-SkipBuild` attempt then refused without a matching scoped build key, despite the native
receipt being valid. Its original log is `.build/folder-samples-20261103/affected-release.log`; the ordinary
affected workflow builds/registers its own evidence instead of bypassing that refusal.

The first complete native execution passed all 72 cases, produced 32 actual captures with DPI provenance,
and verified the isolated crash dump/marker. It retains source identity
`5eabd933b4e43cf390f7e5cb91ecc08490aa5e92025a52714e0b8b5c52c09a26` and the same executable hash as the samples.
[Its native receipt](native-and-captures.json) is retained separately. The scoped runner then correctly refused
reusable success because this evidence Markdown had been added during execution, changing its Tooling identity.
[The original attempt log](affected-first-attempt.log) preserves the result; no earlier failure is rewritten.
The later exact frozen-document execution is recorded in [final-validation.json](final-validation.json).
That result file is evidence output, not an input to native behavior or the Markdown/link validators.

Additional inspected captures: [Files pane](files.png), [rendered display mathematics](math.png).
These use only invented fixture data and have adjacent scenario sidecars. The math capture now shows actual
fractions, matrices, vectors and aligned rows instead of the stale one-line display delimiters.

## Paired diagnostic

A preserved baseline/candidate Release comparison ran two independent ABBA/BAAB blocks in each of Compose and
Split, four cycles per process: **16 processes and 12 metric rows**. Five control rows are unstable; every row
is `no-resolved-change`. The independent evidence replay passed **207,071 checks**. This small diagnostic is
**inconclusive and review-required**, not an accepted performance non-regression or a qualified base policy.
It characterizes the existing S5 editor fixture; folder-root behavior is separately covered by exact scan-generation
and idle-quiescence assertions, not by a folder-throughput acceptance claim.

| Metric | Compose paired change | Split paired change |
| --- | ---: | ---: |
| Command time | -5.49% | -4.06% |
| Owner CPU cycles | -1.19% | +4.03% |
| Completed edit | -1.90% | +0.97% |
| Clean completed frame | +2.03% | -2.33% |
| Private bytes | +0.76% | +0.43% |
| Working set | +0.06% | +0.36% |

Descriptive changes do not establish improvements or acceptance. The original raw directory remains
`.build/performance/folder-rendering-20261103-paired`. [The complete study archive](paired-study.zip),
[comparison](comparison.json) and [independent audit](performance-audit.json) preserve all observations,
compiler receipts, input/header identities and control drift. Broader performance, native ARM64, physical input,
real folder-picker interaction, release and upstream gates remain open with their existing owners.
