# Persistent folder navigation, samples and rendering review

Owner request, 3 November 2026: explain DxUi's TOML/YAML dependencies; expand RedTilde's public examples;
restore the last folder; provide distinct Open File and Open Folder commands; keep a stable Markdown root;
review rendering omissions against [Markdown Viewer](https://markdownviewer.org/).

## Execution

- [x] Inspect exact DxUi pin `5fc4d4c383bfa4891ee57a138924107bbbf11a2e` and parser ownership.
- [x] Retain a clean Release baseline and the first sandbox build/test refusals.
- [x] Implement independent persisted root, native folder dialog and ordinary protected file opening.
- [x] Expand samples with a showcase, syntax switches, metadata and a nested notebook.
- [x] Enable the existing bounded DxUi tokenizer and correct built-in display math.
- [x] Correct the math capture scenario's stale inline display fences and restore the existing WIP-quality plan's missing index entry.
- [x] Update owning specs, user guide, capabilities and changelog fragment.
- [x] Execute validation, formatting, builds and native tests; inspect harness captures and retain their original identities.
- [x] Retain before/after performance evidence with its actual acceptance boundary.

Local implementation and review are complete. The candidate's repository validator passed, 72 native cases passed
and 32 hidden captures were generated. Adding the evidence notes during that first scoped execution caused its
run-wide Tooling identity to change; the runner correctly refused reusable success, and that original run is retained.
The final frozen-document scoped outcome is recorded separately in
[the current validation receipt](../../../Measurements/FolderNavigation_2026-11-03/final-validation.json).
This completed implementation record does not close the parent consumer/platform/performance qualification gates.

The 16-process paired diagnostic used two independent blocks per view and four cycles per process on the preserved
S5 fixture. All 12 rows have no resolved change; five control rows are unstable. An independent replay passed
207,071 checks. The study is **inconclusive, review-required**, not performance acceptance. See
[the retained evidence packet](../../../Measurements/FolderNavigation_2026-11-03/README.md).

The clean baseline built x64 Release with zero warnings/errors. Its focused hidden run passed 12 Storage,
4 Settings and 49 Editor cases; initial benchmark: 56.4 FPS, 40.0 MiB private memory, 0.20 ms dirty edit/render.
These single-run values are not performance acceptance. Frozen baseline:
`D:\RedTilde\.build\performance\folder-rendering-20261103-baseline`.
Baseline native receipt: `.build/evidence/test-runs/20261103T102734755Z-40676-5d30b966bf704499a8b4d2664057fc12/payload/receipt.json`.
The first `-SkipBuild` attempt refused a stale receipt; two sandbox builds failed in pinned 7zip extraction
(`CreateFileW stdin`, access denied). The sandbox baseline run retained a failure in
`Files.ReparseLeavesNeverTraverse` while creating its owned junction. Outside-sandbox execution then passed.
The original failed receipt remains at
`.build/evidence/test-runs/20261103T102714150Z-55384-54daf99f216045e6bee9490d98b0a647/payload/receipt.json`.

## Rendering review and follow-up ownership

The reference site's [feature description](https://markdownviewer.org/) was inspected on 3 November 2026.
Its screenshot/editor URLs were unavailable through the web reader, so this is a feature/source comparison,
not a claim that every sample was visually compared in both applications.

| Area | Finding and action |
| --- | --- |
| Basic Markdown | Both offer tables, tasks, lists, emphasis and code; new showcase covers these together with Unicode. No broad parity claim. |
| Code highlighting | The reference advertises language-aware ink. DxUi already supplies it; RedTilde had not registered the callback. Enable its bounded C-like/JSON/PowerShell/diff profile. Broader languages and a code-copy button remain future app/library work. |
| Display equations | The built-in welcome text still used one-line `$$...$$`, which the current pin deliberately keeps literal. Change it to own-line fences; the public starter already used that form. |
| Mathematics | RedTilde uses bounded native mathematics; the reference uses KaTeX. Package/macro and full LaTeX compatibility remain with the existing rendering coordinator. |
| Mermaid | Corrected after the 3 November shared-renderer follow-up: the exact pin already supports bounded flowchart, state, class, sequence, pie, ER, Gantt and mind-map profiles. Older app docs and several negative samples understated that support. Their correction and shared display defects belong to the delivery-adoption follow-up; full compatibility remains unclaimed. |
| Images and links | Local supported images and opt-in bounded HTTP(S) images exist; links stay inert by product policy. CSS/web/HTML execution is not an implied requirement of visual parity. |
| Documents and delivery | The reference advertises tabs, comparison, synchronized scrolling, PDF/HTML/PNG export, print and rendered copy. RedTilde exposes a single authoritative document and Save/Save As; these are separate product features, not fixed by enabling a parser. |
| Metadata parsers | Private static toml++ 3.4.0 and rapidyaml 0.16.0 validate closed Markdown front matter without source rewriting or application schemas. General UI controls do not need them. DxUi's 2 November production review retains metadata defects and incomplete qualification; this change does not certify or modify those parsers. |

Remaining language/performance/product qualification stays in
[the rendering coordinator](RedesignCompletionAndRendering_2026-10-24.md),
[delivery adoption](DxUiDeliveryAdoption_2026-11-02.md) and
[validation](../../Testing/Testing_Validation.md). No platform or focus gate is waived by this focused work.
