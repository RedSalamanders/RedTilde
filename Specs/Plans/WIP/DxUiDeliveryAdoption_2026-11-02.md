# DxUi 2 November delivery adoption

Status: ACTIVE
Date: 2026-11-02
Owner: RedTilde maintainers

Adopt the DxUi 2 November 2026 delivery ([DxUi PR #159](https://github.com/RedSalamanders/DxUi/pull/159), runtime
commit `6df17007875ae731a3808b475b10bd056328bbdf`) through the reviewed main commit that adds its consumer upgrade
guide ([DxUi PR #160](https://github.com/RedSalamanders/DxUi/pull/160), `docs/upgrade.md` at the new pin). Owning
contracts: [toolchain](../../Build/Build_Toolchain.md), [editor](../../Editor/Editor_Markdown.md),
[settings](../../Core/Core_SettingsAndCrashes.md) and [design system](../../UI/UI_DesignSystem.md).

## Coordinates

### Shared renderer follow-up — 3 November 2026

The latest screenshot follow-up selects `703a96c788a79c1e7b28097226a2ca0a735f11bc` (API 4) for label-aware
inter-box and branch spacing, table/code insets, accent task markers and the persistent Writing preference to
strike through completed tasks. Source-bound native results and actual hidden captures belong in
[the spacing/task packet](../../../Measurements/DiagramSpacing_2026-11-03/README.md). Its before checkpoint remains
`2e775e8d81ff415587a4d885366566ed89df655f`; the setting changes Compose/Split presentation without editing Markdown.

The owner previously requested syntax expansion as well as defects. The validated prior development lock
`2e775e8d81ff415587a4d885366566ed89df655f` (API 4) adds readable default diagram strokes, sequence actor figures, chronological plain notes,
bare automatic numbering, unheaded messages and default top-to-bottom flowcharts. Its source-bound native
and independent consumer results are owned by [the syntax packet](../../../Measurements/MermaidSyntax_2026-11-03/README.md),
whose evidence summary records the actual outcome separately from this pin selection.
This commit was local and unpublished at its validation checkpoint. The earlier fixed checkpoint below remains independently preserved; it is
the immediate implementation rollback for the expansion, while the original app-only slice remains intact.

The intermediate `9ed15504` syntax checkpoint also passes 19 library suites, 73 consumer cases and 32 standard
captures with 18 additional DPI/theme images. Review exposes faint diagram connections inherited from decorative
borders. The current pin reuses effective foreground ink for meaningful diagram structure; its source-specific
results are recorded separately, and the intermediate source, binaries and images remain preserved.

The current pin already contains bounded sequence, pie, ER, Gantt and mind-map profiles in addition to the older
flowchart/state/class list. Earlier app docs and fallback samples understated that support. Correct those claims
and verify every named family through the app's acknowledged native document, view switches and exact source.
The canonical DxUi branch `ej/mermaid-display-fixes-20261103` fixes trailing-space style spans, ER/class shared
attachment collisions, duplicate unordered-task bullets and code-chrome foreground ink. The fixed checkpoint selected
immutable local commit `33ce97ce2b29aed4eadb9270bf66e7080d56b9fc` after its zero-warning x64 Release build,
EditorControls/Embedded/Accessibility/Rendering suites with zero skips, and themed gallery review. This commit
has not been pushed. Its clean independent consumer cache is validated through the ordinary restore/build
interfaces. Immediate rollback is `5fc4d4c383bfa4891ee57a138924107bbbf11a2e` and does not undo the unrelated folder
navigation slice. [Consumer evidence](../../../Measurements/MermaidDisplay_2026-11-03/README.md) records fresh
app tests and captures as they complete; the existing dirty files and original evidence remain preserved.
Other profile, physical input and performance gates remain open.

- [x] Canonical immutable DxUi fix and exact local consumer lock.
- [x] Complete fixed-checkpoint consumer Release validation: 73 native cases, 32 standard captures and 20 custom captures; current syntax-candidate results have their separate packet.
- [ ] Publish the upstream commit before expecting a fresh remote checkout to restore this development pin.

### Historical 2 November delivery coordinates

| Item | Value |
| --- | --- |
| Previous lock (rollback) | `6ed0512c68743c57ac209e22010679d211c4a42b` |
| New lock | `5fc4d4c383bfa4891ee57a138924107bbbf11a2e` (827 commits after the previous lock) |
| API revision | 4, unchanged. Public object layouts changed, so every profile rebuilds against matching headers and archive. |
| Toolchain | Visual Studio 2026 Insiders, MSVC 14.52.37025 / v145, Windows SDK 10.0.26100, `/MDd` Debug and ASan, `/MD` Release |

Rollback restores the preceding commit's lock together with this change's adapter, settings, test and document
edits, runs `restore-dxui.ps1` for each platform and rebuilds every profile. Never mix the old archive with these
headers or objects.

## Adapter review against the upgrade guide

- `RichTextEditorAppearance` and `ContextMenuSessionCallbacks` were already assigned by member.
- No application switch covers `PointerAction`, `WindowHostCursorKind` or Markdown diagnostic, inline or semantic
  kinds; `/Wall` reports no unhandled enumerator. `Separator()` is only constructed through `AddChild`.
- Embedded `Prepare`, `SetNativePresentationVisible`, OLE file drop, rich clipboard interfaces, data/SVG images and
  direct source ink are not adopted; each remains an explicit library opt-in.
- `RichImageSourceKind` gained `WikiTarget` and `Data`. The image worker branches on every kind and declines both
  with `ERROR_NOT_SUPPORTED` before any file or network access.
- `DecodeUtf8` now admits U+0000 for counted editor source. `LoadDocument` keeps the file contract by rejecting NUL
  with the strict `IsValidUtf16`.
- RichMarkdown's default extension mask grew from GFM to `RichAll`. Owner decision (2 November 2026): enable all
  optional syntax, including definition lists and abbreviations, with a Settings option and explanation for each.
- Display math is now a source-owned block: `$$` or `\[` / `\]` must stand on their own lines, and one-line
  `$$x$$` / `\[x\]` stays literal. Insert Equation, the inspector preview, capture fixtures and sample documents
  use own-line delimiters; the inspector also follows display blocks.
- The pin offers a native source region for math. Owner decision (2 November 2026): F2 edits equations in place
  like code and diagrams, and the inspector previews the TeX being typed. The guarded draft remains the fallback
  when the native region refuses.
- Tree model refresh now keeps the viewport. A real section transition still reveals the current heading.
- Library limit changes adopted in tests: math group depth 64, 1,024 matrix cells; undo/redo decline during a
  changed text-service preview and return after cancellation.

## Checklist

- [x] Lock the reviewed main pin and restore x64 through `restore-dxui.ps1`.
- [x] x64 Debug zero-warning build with the lock change alone.
- [x] Explicit image-kind handling; wiki embeds and data images stay unresolved.
- [x] Seven persisted syntax preferences, a Settings section with explanations and live reanalysis of the editor,
  mirror and inspector.
- [x] Preserve NUL rejection, own-line display math and the inspector rendering.
- [x] In-place equation F2 with a live inspector preview; hidden tests and the equation capture updated.
- [ ] Interactive `Equation.F2InPlaceEditUndoAndExit` (real keyboard and focus) rewritten for in-place editing:
  NOT RUN; it needs the owner's agreement to take desktop focus.
- [x] Tests: settings round trip and fallback, live syntax reanalysis and explanation fit, wiki embed decline,
  inspector rendering, display blocks, updated library limits and history.
- [x] Convert one-line display equations in the sample and performance fixtures to own-line delimiters.
- [x] Package the pin's notice texts (toml++, rapidyaml, KaTeX symbols, WIL) beside its third-party notice.
- [x] x64 Debug, Release and ASan Debug: zero-warning builds, every native suite, Preferences, RecoveryRestart,
  Performance smoke, Crash, 32 hidden captures and a real ASan detection probe.
- [x] ARM64 restore and zero-warning Debug, Release and ASan Debug cross-builds (no ARM64 runtime claim).
- [x] Review the refreshed Settings, equation and markdown-style captures from the hidden harness.
- [x] Replace fallback specimens X01, X03 and X04, which the pin now renders (`array`, `\color`, padded matrix
  rows), with forms upstream's math corpus rejects: repeated `||` array rules, an unknown color name and an
  invalid starred-matrix alignment. They were not separately executed in RedTilde.
- [ ] Commit, push and merge this change through the normal PR workflow; hosted PR checks are compiler-free.
- [ ] [Delivery review](DeliveryReview_2026-10-31.md) DX-3, DX-4 and the RT-03 DxUi half: upstream fixes are present
  at this pin; RedTilde acceptance remains that plan's work.
- [ ] Wiki-target navigation and embed resolution: not designed; tracked as a pending capability.
- [ ] Native ARM64 runtime, physical IME/AT/touch, interactive focus and paired performance: NOT RUN.

## Evidence

Generated receipts stay under `.build/evidence/test-runs`; none are committed.

- Lock change alone, x64 Debug: zero warnings and errors. Its Editor run failed five existing cases. Forcing the old
  GFM-only mask left the same five failing, so all five came from the pin: math group depth, matrix cell budget,
  undo during a text-service preview, Tree viewport on refresh and one-line display math. Each is resolved above
  by product code or by a test updated to the library's documented constant.
- The first complete x64 Debug run also found the NUL admission (`Utf8.InvalidInputDoesNotReplaceOutput`) and a
  failing equation capture (draft ended with the display block's line terminator). Both are repaired and covered.
- A later review moved the caret test for display blocks to the closing delimiter, so the following line is outside
  the equation for F2 and the inspector; covered by the in-document math test.
- Qualification of the first PR commit (math F2 still used the guarded draft), MSVC 14.52.37025:
  - x64 Debug `-Full`: validators, Storage 12, Settings 4, Recovery 5, Editor 49, Preferences, RecoveryRestart,
    Performance smoke (117 FPS forced render), 32 captures and Crash pass.
  - x64 ASan Debug `-Full`: the same suites pass, and real heap-use-after-free detection is verified.
  - x64 Release: Storage, Settings, Recovery, Preferences, RecoveryRestart, Performance smoke, 32 captures and Crash
    pass. Editor first failed one foreground guard: the foreground moved between two windows outside the test
    process (desktop activity, not a hidden-window activation). The immediate Editor rerun passed all 49 cases.
  - ARM64 Debug, Release and ASan Debug: zero-warning cross-builds only.
- Qualification of the in-place equation commit, same toolchain, on the final product and test source:
  - x64 Debug `-Full`: validators and every suite pass (Editor 49, 32 captures, Crash).
  - x64 ASan Debug `-Full`: every suite passes and real heap-use-after-free detection is verified.
  - x64 Release: Storage, Settings and Recovery pass. Editor first failed a foreground guard while the owner was
    using the desktop (both observed windows were outside the test process). The rerun of Editor, Preferences,
    RecoveryRestart, Performance smoke, 32 captures and Crash passed.
  - One earlier x64 Debug Editor run failed `Editor.RemoteImagePrivacyOptInRevocationAndShutdown` (Close 4,844 ms,
    limit 500 ms); its rerun and every later run passed. It is recorded here as a transient.
  - ARM64 Debug, Release and ASan Debug: zero-warning cross-builds only.
