# IME Split mirror diagnostic history

This staging packet preserves four ordinary x64 Release Editor-scope results against canonical DxUi commit `5a2131adf734cb91dcc350da888dcd2c0a71e986`. Every native `tests.json` is copied byte for byte, including unfavorable results. `ledger.json` extracts only source/compiled identity, dependency pin, binary/build-receipt hashes, profile, counts, status and test errors from the original evidence. `inventory.json` identifies private origins using repository-relative paths, sizes and SHA-256 hashes; it does not include their private contents.

| Case | Result | Interpretation |
| --- | --- | --- |
| hidden-message-refusal | 32 passed, 1 failed | The draft WM_IME fixture targeted a hidden/unfocused window. The native host correctly refused it, and the intended production defect was not reached. This failure is fixture evidence, not a reproduction. |
| growth-mirror-failure | 32 passed, 1 failed | The revised test invokes the canonical text-service import on the actual attached editor. It reaches the real stale-mirror defect after a transient preview changes source and selection. |
| single-preview-repair-pass | 33 passed, 0 failed | Synchronizing the mirror before applying preview ACPs passes the limited single-preview, cancellation and result-commit checks. This result does not cover successive candidates whose ACPs remain unchanged. |
| same-acp-revision-failure | 32 passed, 1 failed | A second source candidate with unchanged ACPs exposes canonical selection-publication deduplication. The mirror is not synchronously updated for that source revision. Later analysis reconciliation can update it; permanent staleness was not established. |

The text-service bridge forms an inherited protected member pointer and applies it to the actual canonical editor; it uses no downcast, replacement editor, callback substitution or foreground exception. These service-import checks establish application callback/mirror integration and source/history behavior. They do not qualify OS IME message delivery, native composition ownership, physical IME, touch or assistive technology.

This is a source-bound diagnostic packet, not a complete independent rebuild/replay bundle. No native binaries, full source copies, raw environment records, absolute paths or CI payloads are included. Binary hashes identify the original observations; they are not binaries that can be replayed from this packet. No new build, test, native execution or SDK successor qualification was performed while staging. These results do not close performance, native-platform, physical-input, integration or release gates. The separate qualification/performance status remains open.

The packet is staged privately for independent verification and later publication by the task owner.
