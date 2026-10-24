# A57 app-review diagnostic archive

This directory preserves selected raw logs, receipts and clause-review records from the 2026-10-22 A57 review packet. The archive contains 24 copied source artifacts; `sha256-manifest.json` records each original absolute path, byte length and SHA-256, and each copy was checked against its source. `clause-audit.md` is retained as `clause-audit.original.txt` so its historical wording and links remain visibly archival.

## Observed results

- The first focused `red` receipt is source-bound to `f0a8e76d451c0b7cd22821f0b8d4c82ff9b6a9ebe035a0d70b4fa3ec04a712e2`; it reports 31/33 selected Editor cases passing, with Outline Find descendant visibility and newly discovered folder expansion failures. The `red` run log itself is incomplete and records null counters.
- `red-all` is source-bound to `e2d6068f0d7f142b508e491b4c4e9fb8317939164e35c61b7b05165471ddf633`; it reports 29/33 selected cases passing and four failures: responsive Files fit threshold, Outline collapse during Find, folder notification expansion, and modified-key routing.
- `green-build.log` records a `/WX` C4244 failure at `Application.Chrome.cpp(534)` for float-to-`UINT` conversion in the DPI-cache path. The subsequent `green2` build log completes successfully. Its source-bound receipt reports 33/33 Editor cases passing at source identity `8f137ee8a85887fe21b2b746886315dd8f6b9857492169c961bc62566f851b4e`.
- `green-storage2` uses that same source identity and reports 10/10 focused Storage cases passing.

## Evidence boundary

These are focused diagnostics against the `a17f7d7b59536741a0496638ac44d0968bf74e78` DxUi dependency pin in their receipts. They are not substitutes for final qualification against the later pin. The old clause audit's “spacing missing” note is historical; semantic paragraph/list spacing now has a canonical implementation and contract in the current source/specification. Its preserved audit JSON and text remain unchanged records of what that review observed at the time.

This archive intentionally omits source snapshots, executables, fixtures, and helper scripts. The receipts retain original source IDs and selected-test native-file hashes for provenance.