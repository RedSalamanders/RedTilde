# Retained documents, recovery and rendering

Implementation: `fe9b7d15b522630ed54fc8891516b24137a4d5fc`.
DxUi API 4: `8dc328dec0a80cc0e36fa08e5364265ebd25d37e`.
[Exact qualification](qualification.json) records six zero-warning builds, all 91 cases and 36 captures in
each x64 profile, real ASan detection, app/SDK archives and 140 original compiled working inputs. ARM64 rows
are cross-builds, with native/physical/performance qualification still separately owned.

## Retained files and crash recovery

Files-pane activation retains up to 64 authoritative documents with independent source, oriented selection,
undo/redo, saved bytes/stamp/BOM and recovery identity. Switching a known file needs no prompt or disk reload.
The root, filter and folder expansion remain stable, with dirty indicators and a reachable Open Documents group
for untitled, missing and duplicate-path recovered copies. A full workspace refuses admission atomically.

New, explicit Open File/Open Folder and Close protect every dirty document. Prompt approvals bind the exact
document/source snapshot, and Discard is staged until the action actually commits. Later Cancel, picker Cancel,
pending composition, a local source draft or callback-driven retirement cannot silently discard another document.
Save completion binds its original document and preserves edits made after the saved snapshot. Open completion
waits for an active local-source draft and is abandoned by Close.

Independent recovery slots use the existing 750 ms quiet debounce and force admission by five seconds during
continuous typing; this bounds admission, not filesystem completion. A replacement checkpoint becomes durable
before retiring its predecessor. Relaunch can restore all modified documents as independent Save As copies,
including duplicate original paths. Recovery stores the last durable source, selection, BOM and path; it does
not serialize undo history. The two-file crash/relaunch regression verifies exact recovered text and unchanged
original files. Crash safety therefore applies to completed checkpoints, not every keystroke before a crash.

Session-end messages submit all dirty documents immediately and allow up to two seconds for settlement.
Cancelled session end resumes editing; confirmed session end exits without ordinary clean-Close retirement.
Ordinary visible launch registers restart presentation. Hidden tests simulate the messages; they do not claim
actual Windows logoff/restart acceptance.

## Lossless insertion and failure paths

Collapsed Quote, Code, Mermaid and Math templates use the document's first physical line terminator, defaulting
to CRLF for a document without one. Only inserted text is normalized. An unchanged F2 draft is the exact original
slice; changed draft text and its recovery checkpoint use the same policy and preserve outside mixed terminators.
Selected Quote/Code transformations keep the already supported exact-selection behavior and one undo item.
Independent fixtures cover LF, CRLF, CR, mixed source and empty/no-terminator input in all three views.

Failed staging writes/flushes close the owned handle before deleting the temporary file. Injected write and
flush failures preserve original bytes/stamps and leave no temporary staging file. Unknown or malformed settings
headers return a format error without changing the caller or overwriting the on-disk settings, including async
view changes. Find's updating guard unwinds after a rejected source edit, so a later valid replacement succeeds
and remains undoable. The exact failure probes and original unaccepted warning build remain retained.

Network-file image requests from the adopted SDK have a dedicated switch arm. RedTilde's default loader refuses
them before I/O, including localhost and scheme-relative paths; SDK resource opt-in alone is not transport consent.
This does not claim the remaining per-document network-consent UI or local-network override is implemented.

## Native rendering comparison

The four supplied examples are in [the public sample](../../docs/examples/layout-and-math-regressions.md).
The harness captured each example in Light/Dark at 96/192 DPI with the qualified Release image, without desktop
activation. [Capture identities](captures.json) bind fixture bytes, binary, exact source/pin and image hashes;
baseline captures retain their own source identity. The app uses the shared correction rather than competing math
or Mermaid code.

| Example | Corrected Light / 96 DPI | Original Light / 96 DPI |
| --- | --- | --- |
| Class direction | [Current](captures/Classes-light-96.png) | [Original](baseline/Classes-light-96.png) |
| BT pipeline | [Current](captures/Pipeline-light-96.png) | [Original](baseline/Pipeline-light-96.png) |
| Accents | [Current](captures/Accents-light-96.png) | [Original](baseline/Accents-light-96.png) |
| Gathered matrices | [Current](captures/Gathered-light-96.png) | [Original](baseline/Gathered-light-96.png) |

The durable owners are [file safety](../../Specs/Core/Core_FileSafety.md),
[settings/recovery](../../Specs/Core/Core_SettingsAndCrashes.md),
[editing](../../Specs/Editor/Editor_Markdown.md), [validation](../../Specs/Testing/Testing_Validation.md)
and [usage](../../docs/README.md). Current generated outputs are under an existing approved workspace after
byte-verifying all 140 compiled inputs against the earlier checkout; no receipt is relabelled as another root's run.
The [remaining plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md) retains conflict/location policy,
managed image assets, R10 and physical/native platform/release gates.

[Retained attempts](retained-attempts.json) keep counterexamples and unaccepted builds distinct. 
[Validation provenance](validation.json) records the byte-verified canonical short-checkout pass and the 
original long-checkout mock-installer refusal. It does not relabel that failed attempt as a pass.

[Hosted main Release run 42146749203](https://github.com/RedSalamanders/RedTilde/actions/runs/42146749203)
passed all seven jobs at merge `75c0675b`: both native platforms pass all 91 cases, 36 captures and assertion/
crash checks, followed by both test-disabled production ZIP smoke tests. The
[hosted record](hosted-qualification.json) verifies all thirteen raw GitHub artifact wrapper digests, both
canonical native receipts and both inner production ZIP hashes. The original admission batch remains its
original `running` snapshot; completion is established by the finished jobs and their receipts. This qualifies
the `fe9b7d15` / `8dc328de` Release scope, separately from the later save-safety candidate, Debug/ASan ARM64,
physical input and performance acceptance. Recorded test executable identities are not local re-execution.
