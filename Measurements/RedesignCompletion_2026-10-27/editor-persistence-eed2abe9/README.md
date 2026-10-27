# Editor preference persistence observer — evidence packet

**Status:** Retained sanitized evidence projection; results remain source-bound and acceptance gates are separate. This packet is diagnostic evidence, not a qualification or acceptance report.

## Historical Debug result

The retained x64 Debug Editor report for source revision `798fcbe79bdd4eb5dec08bef4d4cc5f7c8e9b7a3`, pinned to SDK commit `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`, records 45 of 46 tests passing. `Editor.NestedViewRequestSupersedesOuterSplit` failed only at the persisted-Source assertion; the retained settings value was Compose. The failed report did not record `SettingsWriter::Status()` or a busy-state sample. The historical cause therefore remains **unattributed**.

## Private discriminator

A headless private probe built the real `Common/Settings.cpp` and `Common/Storage.cpp` against the cached SDK98 x64 Debug library. With a reader held using the same `FILE_SHARE_READ` mode as `ReadBytes`, a Source write completed with status `0x80070020`; after releasing the reader and resubmitting, status was S_OK and the persisted view was Source.

The bounded normal-observer study ran 20 distinct scenarios using the fixture's up-to-400 `LoadSettings` attempts at 5 ms intervals. All 20 reached Source and all final writer statuses were S_OK after 41 reads total. One read in scenario 16, attempt 2 returned `0x80070020`; the original retry loop succeeded on attempt 3. This establishes a possible sharing-interference mechanism, not the cause of the earlier application failure.

## Observer repair and evidence boundary

Repair commit `eed2abe9885c57f44ab192155b341582782020f9` changes the test observer to wait up to two seconds for `SettingsWriter` to become idle, require its successful HRESULT, and load the file once to assert the exact view. It does not change production storage sharing or add write retries. The validation spec records this observer contract and the historical attribution limit.

The seven pre-repair x64 native output files (two Debug, two Release, three ASan Debug) have retained, hash-verified copies. Separately, the eed2abe9 final matrix passed x64 Debug, Release, and ASan Debug with Editor 46/46 in each profile; the independent matrix audit passed all 21 checks. Three ARM64 builds are build-only on this x64 host, with no runtime claim. The first private probe phase is also retained as a 29-file snapshot. The current native qualification is summarized in the sibling [qualification evidence packet](../qualification-eed2abe9/README.md). Performance evidence remains separate and is not included here. No acceptance claim is made.

## Included files

- `summary.json` — sanitized outcome summary and explicit evidence limits.
- `source-and-evidence-hashes.json` — source, SDK, compiler/tool, report, probe, and retained-output hashes.
- `manifest.json` — hashes and byte lengths for the other projection files.
- `.gitattributes` — LF rules limited to this projection's text files.
