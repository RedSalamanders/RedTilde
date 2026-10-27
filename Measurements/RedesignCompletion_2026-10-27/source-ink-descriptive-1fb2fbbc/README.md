# Source-ink renderer diagnostic — 27 October 2026

This is a filtered, descriptive 16-process study of an isolated source-ink renderer candidate. It is separate from the 576-process current-source performance comparison and does not qualify or adopt the renderer.

## Source and scope

The measured baseline is RedTilde `a29c9f8447fe06b33b3a9c611dda5d6e6d29205e` with SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`. The candidate is RedTilde `1fb2fbbc32d08baee36a2cdfdc9ceaeaa20d3b26` with SDK `bc2ac1d517d10e2a6713195790c0a4052e7b028b`. The actual next-PR base is recorded separately as RedTilde `07b232a7ac878cebebddef8631251aeb6358afed`, tree `06f69df5563cecaee7bb2e200b380f1ce827ff10`, compiled source identity `633302c45ac8da29e7bea24cf7909dc70d8aad1665ebe618252dd31f72e0d8f3`, and runtime SDK pin `4077f57df8207eac6c39c43437feb45b19d23cf8`. That base was not a study arm.

Both measured arms bind the same public dependency-header closure: 58 rows (25 source headers and 33 package headers), SHA-256 `AA7B65052E18BA0AF7F6B60AFC47E4F63EEB7552BEA2878972EC36495BFB69C8`. The candidate additionally contains private implementation header `src/Controls/DxUi.RichTextSourceInkRenderer.h`, SHA-256 `1F0877AA4A255CA35409E6B09B4BA9B80A4F4D242316D68890B03D61680F3ECD`; it is not part of the public header closure. One harness header was normalized from CRLF to LF for the canonical rebuild (`5A89343563D46AAD9D3FC5E81683000EDC3F9571BDEA3DDF82E2CA6DA46AB537` to `BCD650BA755F66CE28A30C81C8ECB9F07E937DBA6A9A2C7FDF34AF2F8836DCA9`). The other 40 native input files were byte-identical; no Git tree or frozen metadata was edited for that normalization.

The study has 16 processes, two blocks per view, 64 cycles per process, seed `20261027`, and control diagnostics enabled. The 12 metric rows are in `metric-rows.csv`; every row is `no-resolved-change`. Descriptive paired changes include Split owner CPU cycles `-7.2736445309%`, Split command time `+1.45859607%`, and Split private bytes `+1.45196991%`. These small-sample estimates are not causal or acceptance findings.

## Integrity and limits

The independent numerical/integrity replay reports 4,263,527 checks for this one study. Observer coverage records 91 samples with maximum gap `1.1006747` seconds; this does not establish a quiet host. `raw-report-index.json` lists hashes and byte counts for all 16 reports; raw reports, compiler logs, binaries, PDBs, machine details, and absolute paths are not published.

The separate 576-process comparison remains the larger current-source comparison record. Its original-UI Split CPU finding remains `+11.5179672254146%`; this diagnostic does not supersede it. The source-ink candidate is not adopted in the published production source: this diagnostic baseline and the frozen PR17 base use SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`, while the measured candidate arm uses SDK `bc2ac1d517d10e2a6713195790c0a4052e7b028b`. The current working-tree lock separately points to proposed SDK `98c795b765a10d1805ae0a7c35a8ade535fb7fd6` for PR114; that candidate pin is outside this study and is not qualified production adoption. Renderer SDK/consumer qualification and a fresh actual-base/full comparison remain pending. Performance policy, input qualification, and hosted RedTilde ARM64 remain open. No performance acceptance is claimed.

The raw-report aggregate is SHA-256 over UTF-8 without BOM, sorted by view (`Compose`, then `Split`), numeric block, and slot; each line is `view|block|slot|lowercase-sha256`, LF-separated with no trailing LF. The aggregate digest is `d47126f97b5649c904c13f45305ba3b16f50893ff0d04139c3a56bc9d1e37dba`.
