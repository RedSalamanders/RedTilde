# RedTilde final native matrix — eed2abe9

**Status:** Retained sanitized evidence projection; results remain source-bound and acceptance gates are separate.

## Bound result

This packet is bound to RedTilde commit `eed2abe9885c57f44ab192155b341582782020f9`, source identity `d368102ab388b7e555047205bef8edcaf73a63acde954e76a161a9b55781df78`, and DxUi pin `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`.

The retained matrix contains six successful builds: x64 Debug, Release, and ASan Debug, plus ARM64 Debug, Release, and ASan Debug. The x64 profiles each completed the full native run with 65/65 catalogued cases: Storage 10, Settings 4, Recovery 5, Editor 46. Each x64 profile has 23 verified capture/provenance pairs. The x64 ASan profile also recorded the expected deliberate heap-use-after-free probe detection; this is the sanitizer self-check, not a product defect report.

All three ARM64 configurations were built on an x64 host. No native ARM64 runtime test is represented here. The independent read-only audit passed all 21 checks, including exact source/pin binding, retained build artifact hashes, raw native receipts, case partitions, and capture provenance.

## Evidence limits

This is native matrix evidence only. Hosted RedTilde ARM64 runtime qualification remains open. Interactive focus/keyboard acceptance and performance qualification are separate. No repository or policy acceptance is claimed.

## Sanitization

`native-hash-inventory.json` carries relative evidence roles, byte counts, and SHA-256 digests. It does not include executable, DLL, PDB, test payload, capture, or log contents. Absolute machine paths, usernames, hostnames, and security identifiers are omitted. Hash values for large retained build artifacts come from the independent matrix audit.
