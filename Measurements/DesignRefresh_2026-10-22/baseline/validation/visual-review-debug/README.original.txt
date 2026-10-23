# Design refresh visual review — e62 Debug captures

Reviewed all 20 top-level PNG captures from test run `20261023T010157518Z-44088-12806e08d6304481bf12f020761a3c65`. The capture packet records a passing, hidden x64 Debug run using DxUi `e62dd9d9a91a6ad68d88f7f01a6dd075ae31dbd3`; source identity `e61384940962103b9d91301d1fd134391b9c73e430af27218e61af572c03c4b8`; executable SHA-256 `4ADF9E437929C02BCDA1B2F9E92119D2C0B078C7198C56CC6B6E44AA0A7C8756`. Original `receipt.json` and each adjacent `.scenario.json` remain unchanged. `capture-manifest.json` records each source filename, PNG hash/length/dimensions, scenario JSON hash, and capture geometry.

## Findings

1. **Responsive reading surface is clipped at the left edge in every narrow-width capture.** In `narrow.png` and `narrow-dpi-96.png`, `narrow-dpi-144.png`, `narrow-dpi-192.png`, the reading column begins under the fixed Files pane: the screenshot shows only the tail of the heading (“…or big ideas”) and body lines start mid-sentence. The scenario metadata confirms the same 800 × 560 DIP client size at 96/144/192 DPI; the 96/192 captures are simulated DPI and do not establish physical-monitor behavior. This is visible content loss under the intended narrow layout, not just vertical viewport cropping.

2. **Transient overlays cover unrelated document text in two states.** `context-selection.png` places the selection toolbar over the lower part of the main heading while the selection is in the following paragraph. `find.png` places the floating Find/Replace panel over the right end of that heading. The controls are legible and the selected/search matches remain visible, but document text is obscured until the surface is dismissed or repositioned.

## Otherwise visually sound

The two theme captures are coherent; the high-contrast capture uses clear system-blue selection and focus treatment. Source and Split remain readable, the diagrams and equation render cleanly, and the recovery/source dialogs show their full controls. Settings content continues below its own scrollbar, which is an intentional scroll viewport. Fixture names and paths are synthetic (for example `Ideas.md`, `Chapter2.md`, and `C:\Notes\My next idea.md`). Focus mode’s intentionally reduced chrome and dimmed inactive blocks remain legible at this capture size.

This is visual inspection of hidden harness captures only. It does not qualify physical monitor DPI behavior, native ARM64, touch, assistive technology, or release builds. No source or capture image was edited.
