# RedTilde proofing simplification after DxUi upstream adoption

## Metadata

- **Status**: `HOLD` — blocked on upstream DxUi producer wave (`DxUi/Specs/Plans/WIP/RichTextProofingSupport_2026-11-24.md`) landing and being pinned via [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json).
- **Created**: `2026-11-24` · **Updated**: `2026-11-24`
- **Owner**: RedTilde maintainers
- **Current implemented baseline**: Slices `S1` and `S2` in [Dictionary and spell checking](DictionaryAndSpellChecking_2026-11-21.md) on the pinned `DxUi` API-5 commit (`0c371a6197eb8c6b876b4b2843d75078bc21035c`).
- **Owning contracts**: [Core architecture](../../Core/Core_Architecture.md), [Editor & Markdown contract](../../Editor/Editor_Markdown.md), [UI design system](../../UI/UI_DesignSystem.md), [Build toolchain](../../Build/Build_Toolchain.md), and [Testing & validation](../../Testing/Testing_Validation.md).

---

## 1. Objective

RedTilde's current S1/S2 spell-checking implementation ships complete behavioral functionality against the pinned DxUi API-5 SDK (`0c371a6197eb8c6b876b4b2843d75078bc21035c`), but carries ~650 lines of compensatory adapter code for prose interval walking, front-matter header scanning, `<= 256` viewport decoration slicing, scroll re-projection, `trySetDecorations` typing suppression churn, duplicate context-menu hit-testing, mid-word `SourceEdit` splicing, and `WM_PAINT` deferred dispatch.

Once DxUi ships `DX-SPELL-1`–`DX-SPELL-8` in a future qualified release and RedTilde bumps [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json), this plan executes the **pin bump + code deletion wave** to leave RedTilde with a thin, fast consumer adapter owning only:

- `ISpellProvider` / `WinSpellProvider` / `FakeSpellProvider`
- `CustomDictionary` (`custom-dictionary.txt` / Microsoft Word `CUSTOM.DIC`)
- `SpellService` STA worker, 128-entry suggestion cache, and multi-language filtering
- RedTilde policy filters (`IsScriptAwareAllUppercase`, `ContainsAsciiOrUnicodeDigit`, `LooksLikeInternetOrFilePath`, `ShouldSkipSpellError`)
- App UI chrome (context menu items, status bar chip, Settings Proofing drawer, `F7` Proofing pane).

---

## 2. Exact simplification and deletion map

| Upstream DxUi Feature | Code Removed / Simplified in RedTilde | Replacement Using New DxUi API |
| --- | --- | --- |
| **`DX-SPELL-1`** (`TryVisitSemanticProseSegments` + `TryVisitSyntaxProseRanges`) | In [Common/ProseExtractor.h](../../../Common/ProseExtractor.h) & [Common/ProseExtractor.cpp](../../../Common/ProseExtractor.cpp): remove `ExtractSemanticProse` element/run/span exclusion walkers, `ExtractSourceProse` syntax walkers, `MaskInlineSyntaxNode`, `MaskRange`, `UnmaskRange`, and `MaskSentinel` staging buffers. | Call `DxUi::RichText::TryVisitSemanticProseSegments` (Compose/Split) and `TryVisitSyntaxProseRanges` (Source), applying only RedTilde's `Chunk` <= 4,096 packing and `LooksLikeInternetOrFilePath` masking. |
| **`DX-SPELL-2`** (`RichTextDecorationLayer` + `4,096` lazy viewport decorations) | In [RedTilde/Application.Spelling.cpp](../../../RedTilde/Application.Spelling.cpp) & [RedTilde/Application.Chrome.cpp](../../../RedTilde/Application.Chrome.cpp): delete `ViewportOverscanUnits`, `MaximumVisibleSquiggles`, `ProjectVisibleSpellDecorations()` window slicing, `{256, 128, 64}` `E_BOUNDS` retry loop, `OnSpellScrollChanged()`, and the scroll-callback hook in `Application.Chrome.cpp`. | Push the document's full `SpellIssue` array (up to `4,096`) once per completed check to `RichTextDecorationLayer::Spelling`, allowing DxUi to manage viewport geometry and Find/`Consumer` layer isolation cleanly. |
| **`DX-SPELL-3`** (`SetOnRichContextMenu`) | In [RedTilde/Application.Spelling.cpp](../../../RedTilde/Application.Spelling.cpp): remove `ScreenToClient`, DIP scaling (`96.f / _host.GetDpi()`), and redundant `TryHitTestTextInputPoint` lookup inside `PopulateSpellContextMenu`. | Consume `request.hitSourceOffset`, `request.source`, and `request.selection` delivered directly by `SetOnRichContextMenu`. |
| **`DX-SPELL-4`** (`SetDecorationSuppressionRange`) | In [RedTilde/Application.Spelling.cpp](../../../RedTilde/Application.Spelling.cpp): eliminate full decoration-vector rebuilds and `TrySetDecorations` re-submissions from `OnSpellEditorChanged()`, `OnSpellSelectionChanged()`, and `OnSpellDebounceTimer()`. | Call `_editor->SetDecorationSuppressionRange(RichTextDecorationLayer::Spelling, _spell.suppressedTypingWord)` on caret entry/exit and clear with `std::nullopt` after `300 ms` quiet debounce. |
| **`DX-SPELL-5`** (`TryPlanDisplayRangeReplacement`) | In [Common/ProseExtractor.cpp](../../../Common/ProseExtractor.cpp): delete custom `TryBuildSpellReplacementEdits` multi-fragment splicing helper. | Call `DxUi::RichText::TryPlanDisplayRangeReplacement` in `HandleSpellMenuSelection`. |
| **`DX-SPELL-6`** (`MarkdownSyntax::TryGetFrontMatterScalar`) | In [Common/ProseExtractor.cpp](../../../Common/ProseExtractor.cpp): delete custom YAML/TOML line scanner `ExtractFrontMatterLanguage`. | Query `syntax->TryGetFrontMatterScalar(L"lang")` with fallback to `L"language"`. |
| **`DX-SPELL-7`** (`SetOnSemanticPublished` / `SetOnSyntaxPrepared`) | In [RedTilde/Application.h](../../../RedTilde/Application.h), [RedTilde/Application.cpp](../../../RedTilde/Application.cpp), and [RedTilde/Application.Spelling.cpp](../../../RedTilde/Application.Spelling.cpp): remove `_spell.immediatePending`, `WM_PAINT` `placementPending` spell flag, `ReconcilePostPreparePlacement()` spell dispatch branch, and `_spell.immediatePending` wait condition in `Prepare()`. | Subscribe to `SetOnSemanticPublished` / `SetOnSyntaxPrepared` and dispatch any deferred initial spell check cleanly when DxUi publishes immutable semantics/syntax. |
| **`DX-SPELL-8`** (Smooth wavy `Squiggle` underline) | In [RedTilde/Docs](../../../docs/screenshots/README.md): pinned API-5 `DxUi.lib` draws disconnected diagonal slashes instead of a smooth wave. | Automatically inherit smooth continuous Bezier wave rendering from `DxUi` once `Dependencies/DxUi.lock.json` is bumped, and recapture `docs/screenshots/spelling-{light,dark,high-contrast}.png`. |

---

## 3. Adoption preconditions and execution sequence

Do **not** start edits until all preconditions hold:

1. **Upstream landed & qualified**: `DxUi/Specs/Plans/WIP/RichTextProofingSupport_2026-11-24.md` moves to `DxUi/Specs/Plans/Done/` on `main` with a verified commit and updated `DXUI_API_REVISION`.
2. **Pin update PR scope**:
   - Bump [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json) to the qualified DxUi commit.
   - Apply the cleanups in Section 2 in the **same PR** (keeping `ProseExtractor` tests verifying identical extraction/exclusion/contraction/mid-word replacement behavior).
   - Keep `S3` capabilities (`F7` Proofing pane if already present or in progress) intact and re-verify layer non-interference with Find highlights (`Consumer` layer vs `Spelling` layer).
3. **Required validation before closing this plan to `Done/`**:
   - Run `format.ps1 -Check`, `validate.ps1`, and `Tools/Test-Changelog.ps1 -Base origin/main`.
   - Zero `/Wall` warnings across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` build profiles.
   - Full `x64` `Debug`, `Release`, and `ASan Debug` runtime runs (including all `Editor.Spelling*`, `Settings.Spelling*`, `Sanitizer`, and `Captures` suites — `spelling-light`, `spelling-dark`, `spelling-high-contrast`).
   - Paired Release performance comparison confirming reduced typing/scroll work vs the S1/S2 baseline.
   - Update normative descriptions in [Editor_Markdown.md](../../Editor/Editor_Markdown.md) and [Core_Architecture.md](../../Core/Core_Architecture.md), archive a `Changes/` fragment, and move this plan to `Specs/Plans/Done/`.
