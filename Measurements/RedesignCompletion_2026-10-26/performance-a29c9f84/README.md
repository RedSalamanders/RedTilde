# RedTilde P22 current-source performance comparison — 26 October 2026

This packet applies the existing three-reference comparison format to RedTilde source `a29c9f8447fe06b33b3a9c611dda5d6e6d29205e` with DxUi SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`. It contains six separate studies, 576 process slots, 72 metric rows, and the complete raw-report index. It uses the existing judge and thresholds; it is not a new protocol.

## Findings

- Original UI A/B has one resolved finding: Split owner CPU cycles increased 11.5179672254146%. Compose command time (+3.30%) and Split working set (+1.94%) remain unresolved.
- Starting-redesign A/B has four resolved improvements, all memory metrics. The actual PR #16-base A/B and all three A/A studies have zero resolved findings.
- 36 of 72 controls are unstable. Cadence coverage completed in all six studies, but none verifies a quiet CPU window. Memory trends remain close to the 2% decision band; no causal improvement is claimed.

The earlier 09f cohort remains a separate historical collection and is not pooled here. The actual PR #16-base anchor `f33471da51773e6a0ee786a5dd29e46ceb21ecdd` is not the current documentation PR #17 base. The canonical independent numerical audit completed 25,202 checks. A separate root-derived projection/integrity replay completed 3,824 checks and rehashed all 576 reports, six comparison digests, 72 rows, 1,728 control drifts, and 936 CSV cells. Neither audit is performance acceptance. Hosted RedTilde CI run `40368290271` started zero steps because of the payment/spending restriction; hosted ARM64 remains open.

## Related input and qualification

The separate keyboard run `20261026T212043550Z-40984-d90b5b6bf6c94464ad1b66d3a3cc4874` passed 9 stages with 168 injected events. It is not part of the 576-process matrix. The original focus-capture query returned null and focus restoration is unverified; foreground restoration and an unchanged pointer were observed.

One x64 Debug native run at SDK source 6cb678fa passed five filtered tests with zero skips (run 20261026T221017678Z-31780-e28a96c68bee4ddc891cd45a78d37c52); initial focus capture was valid and focus restoration was unchanged. The full Menu native run at x64 Debug/SDK 6cb lasted 91.7 seconds and failed during TestMenuPopupAcrylicLightVisualBaseline (run 20261026T221258356Z-9848-5bc2fba8c9cc413dbe42198c85608dc9). Foreground, keyboard-focus, and pointer state were unchanged. The diagnosed lease issue is an unset ProcessStartInfo working directory. Current focused CWD tooling passes 48/48 after adding a non-FileSystem test. PR112 remains OPEN at 8199566d635751a20a6e916e73410fb6cad317cc; the full local Menu run is pending that head. The launcher change requires a fresh strict build gate, so the prior SDK 6cb gate result does not qualify it. Combined Tooling plus SkipBuild refusals were reproducible; the prior fresh SkipTooling run passed its strict gate, while the refusal cause remains under investigation. These outcomes are not performance acceptance.

The private source-ink and 32-range experiments remain unpublished and unadopted; the production pin is unchanged. Source inspection measured 693 source units, 694 prepared units, one geometry chunk, and 26 published ink ranges; actual S5 draw callback/font/role-brush admission was not recorded. Its descriptive 16-process result receives no performance credit. The private one-case Release native diagnostic initially failed on a 26-line fixture; trace review showed a zero-glyph LF callback was rejected. A narrow exact-source CR/LF no-paint guard and CRLF/invalid-empty-callback tests were added privately. The focused raster check passes in 0.341 seconds, but is not canonical full qualification; an actual S5 callback trace remains pending.

## Evidence and limits

`summary.json`, `metric-rows.csv`, and `source-identity.json` provide the compact results and source bindings. `raw-report-index.json` binds the 576 report digests using synthetic cohort/phase/scenario/block/slot keys. Raw logs and payloads are not included. `root-replay.json` binds the root-derived integrity replay; `evidence-index.json` seals this packet projection.

The raw-report aggregate SHA-256 recipe is: sort by cohort, phase, scenario, numeric block, and slot ascending; encode each row as `cohort|phase|scenario|block|slot|sha256`, uppercase phase and lowercase digest; join with LF between rows and no trailing LF; encode UTF-8 without BOM; then SHA-256 the bytes and render lowercase hexadecimal.

No performance-policy qualification or acceptance is claimed. The current original-UI Split CPU finding, unstable controls, hosted ARM64, and separate focus/desktop gates remain open.

The current P22 checklist remains in the [WIP plan](../../../Specs/Plans/WIP/RedesignCompletionAndRendering_2026-10-24.md); it keeps performance acceptance, hosted ARM64, focus verification, and wrapper-admission issues open.
