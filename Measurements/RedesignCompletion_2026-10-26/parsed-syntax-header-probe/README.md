# Parsed snapshot header source probe evidence

This packet records the root-executed, source-only DxUi header probe using protocol `dxui-parsed-source-snapshot-source-probe-v2`.

## Compared SDK revisions

- Baseline SDK: `47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a`, tree `3ee2e12647ca2e05113823eb172bb0facf879021`.
- Candidate SDK: `4077f57df8207eac6c39c43437feb45b19d23cf8`, tree `18be66885916f67cafd8aeb761207bf15fbd9120`.
- The isolated source probe used old app context `0f68857bc383f6dccd1dbe793b01a5a1fd70291c` and current app context `7e9de544d077f704813031d391b6f1ed766ef208`. Those app contexts do **not** represent the actual PR-base comparison.

## Result

The header inventory contains 58 rows: 25 DxUi source headers and 33 package headers. Only these repository-relative public paths changed:

- `include/DxUi/DxUi.h`
- `include/DxUi/MarkdownSyntax.h`
- `include/DxUi/RichDocument.h`

The probe measured `MarkdownSyntax` at 304 to 320 bytes, with alignment unchanged at 8 bytes. `MarkdownParser` stayed 64 bytes, `RichTextEditor` 304 bytes, `SourceSnapshot` 16 bytes, and `Document` 88 bytes; each retained alignment 8 bytes. The exact method-signature checks were absent in the baseline headers and present in the candidate headers; the existing read-only source synchronization signature was present on both sides. See `source-probe-public.json` and the raw standalone JSON run logs.

## Evidence limits

This is a source/layout probe. It compiles the probe source with PCH disabled and links only the probe object; it does not link or call the DxUi library. The measured layout change is limited to `MarkdownSyntax`. This packet makes no binary ABI compatibility claim and does not establish full-application compile/link equivalence or a full consumer adoption claim.

The exact raw source-probe receipt SHA-256 is `A43614D4498017F932F52029603977E5F6211B2B3228BEC66777DC0935226114`. The probe source SHA-256 is `F05C8A89AA0E601BC8C7F3E424B1230B5C3E3DB5FD93A69A35AAD645F5B5536F`. The archived `ReviewedHeaderDeltaProfile.v2.json` is the private generated v2 profile draft, not the live profile. Its exact hash and the archived-file hashes are in `hash-index.json` and `archive-files.sha256.json`.

## Path handling

The private run directory was `.build/audit-20261024/performance-closeout-20261026/parsed-syntax-header-probe/run-20261026T081523907Z-440b1ab0b3814b3b8843f882c6d0126c`; `Probe.cpp` and the driver were in its parent directory. Public source/header names in this packet are synthetic, repository-relative paths from the header inventory, not claims about a reader's local checkout path.

The full raw `source-probe.json`, MSBuild compiler/linker tlogs, and expanded compiler/link command text contain machine-specific absolute paths and are not copied here. Their original hashes are retained in `source-probe-public.json` and `hash-index.json`; the public probe JSON is a selected-field projection of the raw receipt. The small standalone compile, link, and layout logs are archived unchanged. The profile receipt includes the exact source-probe hash and ordered per-side projection checksums.

