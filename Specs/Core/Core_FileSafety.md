# File safety

## Image file admission and refresh

Image transport uses `ReadImageFileBytes`, independently bounded to 16 MiB; ordinary document reads keep
their smaller limit. Open only absolute canonical paths. While opening each component, retain checked parent
directory handles that exclude cooperative rename/metadata writers. Open entries without following their reparse
points and reject actual name-surrogate links and mount points; non-name-surrogate cloud/provider tags retain
their provider semantics. Use the final retained file handle for the complete bounded read and stable metadata
check, then publish bytes only after cancellation is checked again. Refusal preserves the caller's bytes.
The component walk admits at most 128 retained parents. This is a cooperative sharing boundary, not a guarantee
against a filesystem/filter that ignores Windows sharing or a privileged provider changing its semantics.

Network-file images require both image permission and the local-network preference. The document must itself
be on the same literal UNC server/share; compare server names ordinal-insensitively and share names exactly.
Never reinterpret a URI, local `file://localhost` result or another share as same-share authorization. SDK requests
already supply canonical decoded paths. Real OneDrive/cloud, SMB/NAS and WSL qualification remains separate
from these local ordinary-file, owned-junction and lexical policy cases.

`OpenImageDirectoryForNotifications` reuses the same component/reparse admission for worker-side watch setup.
Its final overlapped directory handle permits ordinary writes/rename and observes that opened directory object.
Parent pins end after final handle acquisition; no later path reopen is used for that watch. Notification names
are hints for bounded refresh, never authorization to read a different path.

## Bounded clipboard asset files

`WriteNewBytesDurable` is the shared binary-asset writer. It accepts only absolute paths and an explicit
positive limit no larger than 64 MiB, uses `CREATE_NEW`, and never overwrites an existing entry. Complete
write, flush, final timestamp/flush and full identity/content stamp capture precede publication of the caller's
stamp. Failure removes only the writer's newly created partial file through its held ownership handle.
The ordinary Markdown reader/writer retains its existing smaller encoded-document capacity.

`RemoveCreatedBytesIfUnchanged` opens the exact file without following a reparse entry and holds the
cooperative write/delete exclusion while comparing full identity, size, timestamp and content. Reparse,
directory and multiply linked entries are refused; changed bytes remain even if size/time were restored.
Only an exact match may be marked for deletion. Missing files are already retired. Cleanup never follows
links, widens the document read limit or removes a same-named replacement. Bitmap assets use these helpers
off the UI thread; their source/undo ownership belongs to the [editor contract](../Editor/Editor_Markdown.md).

Atomic staging uses a bounded temporary leaf in the destination directory, independently of the target filename's
length. Regression coverage includes first publication and replacement of a 240-unit target without a leaked temporary;
`Storage.AtomicSaveKeepsLongLeafWithinPathBudget` exercises that boundary and the saved bytes/stamp.

Open and save local `.md`, `.markdown` and text files using strict UTF-8. Preserve BOM and original line endings;
invalid UTF-8/NUL/oversized files fail before replacing the current document. New documents use UTF-8 without BOM.
The v0 source limit is 65,536 UTF-16 units. Bounded reads reject files above the maximum possible encoded size.
File pickers label `.txt` as UTF-8 only. Unsupported encoding/NUL has a localized UTF-8 explanation; oversized
input has a localized limit explanation, including the actual decoded UTF-16-unit count when the bounded valid
input can be counted. Do not scan arbitrarily large files just to invent a precise count. Caller source/BOM/stamp
remain unchanged on rejection.

An ordinary, singly linked file save stages a unique sibling temporary file with `CREATE_NEW`, writes all bytes, flushes and closes it,
then uses `ReplaceFileW` for an existing destination or `MoveFileExW` for a new destination. Remove only the
owned temporary file on failure. Never clear dirty state on failed/cancelled storage or change path/BOM prematurely.
An existing document whose actual `ReplaceFileW` operation returns `ERROR_NOT_SUPPORTED`,
`ERROR_INVALID_FUNCTION` or `ERROR_CALL_NOT_IMPLEMENTED` may use the same recovery-protected in-place writer
as a hardlink save. Reopen under the cooperative writer/deletion lock and recheck the expected identity/content
before admitting an independently durable recovery copy of the complete intended source. Preserve the existing
file object and publish its new stamp only after complete write, truncation, flush and identity verification.
This fallback is non-atomic: failure may leave partial target bytes, but must retain the old saved baseline and
complete recovery copy. Missing or unavailable recovery declines before target mutation. Only provenance from
the replacement operation authorizes fallback; an equal HRESULT from staging, flush, readback or metadata does
not. Access/sharing failures never authorize fallback. Raw settings/recovery atomic writes retain their ordinary
failure result. Real cloud/share/WSL qualification remains separate from directed local provider refusals.
Close the exclusive staging handle before failure cleanup, including failed WriteFile and FlushFileBuffers
paths. Directed atomic-staging faults must leave the target bytes/stamp unchanged and no owned `.tmp` behind.
An existing file identity (volume/file index where supplied by the filesystem, size, last-write and content
fingerprint) is checked **after staging**, immediately before replacement. A different file object is a conflict
even if its size, timestamp and bytes match. A timestamp-only touch of the same object with identical size and
fingerprint does not require a conflict choice. Save As to the current path, an alias of its saved object, or an
alias of the currently replaced original still checks the original saved baseline. A separately chosen different
destination has the native picker's overwrite consent. Unknown identities remain conservative.
Keep the complete 64-bit volume serial and 128-bit identifier returned by
[`FileIdInfo`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info), including
ReFS identifiers. Only explicit unsupported-information errors permit a provider's legacy handle identity;
other metadata failures propagate. A missing identity never establishes automatic overwrite consent. Native
NTFS/ReFS diagnostics compare the actual complete handle identity independently of stamp equality.
Atomicity does not eliminate the cross-process check/replace race; no conditional-write or cross-process lock is
claimed. Native OneDrive/cloud, SMB/NAS and WSL retain their independent qualification.

Saving through a symbolic link resolves its final followed target using handle-derived identity and path. Keep
the symbolic entry intact and protect that chosen entry against cooperative replacement while saving the target.
Relative targets, Unicode paths and a symbolic link to a hardlinked target use the same target policy. A dangling
link fails without replacing the link entry. Non-name-surrogate provider reparse tags retain their provider path.
This does not claim an atomic transaction against retargeted ancestor directories or a chain of changed links.

When the followed file has multiple hardlinks, preserve its native identity and all aliases by writing in place.
Atomic replacement would split the aliases and is therefore not used. Recheck link count immediately before an
ordinary staged publication; if a new alias appeared, discard that owned temporary and reevaluate once. Check the
expected source baseline under the cooperative read/write/delete exclusion of the in-place writer before any
mutation. Save As still applies its ordinary destination-consent and alias conflict rules.

In-place saves require an independently durable recovery snapshot of the exact admitted source, original requested
path, BOM, selection and generation before the first target write. The file worker owns that immutable context.
The checkpoint uses its own random identity and exclusive lease, outside the ordinary coalesced document slots;
newer edits and ordinary clean retirement cannot overwrite or delete it. Recovery admission validates the envelope,
bounds its directory examination to 512 entries and flushes/publishes the checkpoint before target mutation. Missing,
invalid, full or unavailable recovery storage declines the save with original target bytes and caller stamp intact.

Hardlink writes are explicitly non-atomic: a write, truncate or flush failure can leave partial target bytes in every
alias. Retain dirty state and the previous saved baseline, and keep the complete independent recovery copy. Never
attempt to write recovery from a crash handler. A successful complete write, truncation, flush and final identity
check publishes the known-byte stamp, then retires only its own checkpoint. Failed retirement logs a warning and
retains an extra valid recovery copy without changing the successful save result. Interruption releases the lease;
ordinary restart offers the retained snapshot through the existing Restore a copy flow and never overwrites originals.

Read handles allow cooperative read/write/delete sharing. Compare metadata on the same object before and after
the bounded content read; an observed unstable read returns `ERROR_RETRY` without changing caller outputs.
This does not promise a coherent snapshot against an uncooperative writer that changes bytes without an observed
metadata change. An ordinary atomic save derives the successful stamp from the owned staging object after its exclusive writer
closes and before publication, combined with the known written bytes. It does not reopen published content to
decide success. Publication returns that stamp only after Replace/Move succeeds, so an immediately denied read
cannot report a completed save as failed or poison the next baseline. A later external edit conflicts normally.

An ordinary save conflict has one surface with **Reload / Save As / Overwrite / Cancel**, defaulting to Cancel.
Cancel, closing the question and cancelling Save As preserve source, selection, history, path, dirty state and
recovery without another generic error question. Duplicate Close requests keep the existing question.
Only the explicit Overwrite choice bypasses the expected stamp, for that selected destination and immutable
document context. Choice/picker continuations revalidate workspace identity, source revision, baseline, location,
BOM and recovered state before admission. A changed context retains its newer edits and shows a localized status.
Reload reads asynchronously and replaces only the current document after validating the exact approved source
and workspace identity again; other retained documents and the root stay intact. Failed or stale Reload retains
the editable source/history/dirty protection. Closing during an admitted save retains the separate failure
contract below and does not automatically retry Overwrite or Close.
Checked refusal before taking a completed worker result retains that pending result and dirty source for later
processing, cancels older continuations/Close and reports a localized action failure. An unexpected C++ exception
takes the process-owned fatal diagnostic path. After a successful write
has been admitted to application state, publish its path, saved text and stamp together before callbacks or
fallible view refresh; a later UI failure cannot leave a new saved baseline paired with the old file stamp.

Use API-5 checked source and codec admission for user, file and recovery content. Invalid or over-capacity source
does not reset the current source, selection, saved baseline, workspace identity or undo/redo. Validate document
metadata before replacing source. A new Files slot whose content is refused restores the previous active
document and removes only its empty provisional slot. A rejected explicit Open does not retire the existing
workspace. Native I/O failure remains an ordinary checked result; unexpected C++ allocation failure is fatal.

New/explicit Open File/Open Folder/Close prompt Save, Discard, Cancel for every dirty workspace document.
Files-pane activation retains each document and never prompts. Discard approval is staged until the requested
action is admitted; cancelling a later question or picker retains even earlier approved documents. An approval
is specific to the document's workspace identity and immutable source, so later edits are protected again.
Save As cancellation cancels the original action. Undo
back to saved text clears dirty; the saved baseline is exact source content, not just a transaction counter.
While the Save/Discard/Cancel question is open, duplicate native or command Close requests are ignored.
They keep the original prompt and continuation; they neither cancel it nor queue another Close. Cancel
leaves the current source, selection and history intact. A later fresh Close may ask again normally.

Close during an admitted save hides the window and keeps its notification owner, file worker and recovery
alive until the actual write/replace completes. Duplicate Close requests and queued editing commands are ignored
while waiting. Close supersedes an older New/Open continuation attached to that save.
The host still receives native hide, focus-loss, capture-cancellation and size/DPI lifecycle notifications;
waiting must not strand text input, logical focus, prepared content or window geometry.
Success updates the saved baseline to the admitted text; close and retire recovery only when the current source is clean. If edits made
after admission remain, restore the window and apply the ordinary Save/Discard/Cancel protection to those edits.
Failure cancels the pending Close, preserves source/path/history and recovery, and restores the previous window
placement without activation with the storage error visible in the status area. It does not automatically retry
an overwrite or the Close; another explicit Save remains available. Hidden harness windows remain hidden.
An Open in flight is abandoned without a busy-operation prompt or filesystem join. Invalidate its worker/session
before asking about any edits made during Open, so Cancel retains those edits and a late Open cannot replace them.
An affirmative Save in that question can admit a new operation independently of the abandoned reader.

The application may reuse that exact dirty result only for the same immutable source revision, saved baseline
and recovered-copy state. Replacing the saved baseline invalidates the result, including successful asynchronous
save completion without another live edit. Equal-length replacements, direct resets, undo/redo and text-service
previews/cancellation must still compare exact content when the fact is no longer current. Length mismatch may
short-circuit that comparison; source revision alone never establishes that a document is clean.

Recovery is an independent per-session UTF-8 snapshot under LocalAppData, containing document path/encoding and
source. It must never replace the user's Markdown file automatically. A clean save retires the matching snapshot.
Restart offers a read-only source preview with Restore a copy, Discard snapshot and Keep for later. When several
copies are available, **Restore all copies** retains them as independent documents without overwriting the current
workspace. Capacity is checked before starting. Restored documents with identical original paths and untitled
copies remain separately reachable through Open documents. The settings
drawer can reopen pending snapshots. Restored source retains BOM, line endings, original location for relative
images and source selection; it is dirty and Save requires Save As. It never silently overwrites the original.

Restore captures the clicked candidate's identity and immutable snapshot before asking about unsaved changes.
After the prompt or a successful admitted save, revalidate that exact identity before changing the editor.
If it is no longer offered, stop with a localized explanation; retain the current source revision, selection,
history, path and dirty/recovered state and every other candidate. Never substitute the next list entry.
A later explicit Restore may choose another available copy. Successful retirement names only the captured id.

Each process owns a random session identity and up to 64 distinct, exclusively leased document identities.
A single event-driven worker coalesces one immutable latest value per document and writes after 750 ms of quiet,
or admits a checkpoint after at most five seconds of continuous submissions. The bound concerns admission to
filesystem I/O, not an unbounded filesystem completion time. Explicit checkpoints and shutdown bypass debounce.
Continuous typing in one document cannot postpone another's due checkpoint. Retirement and storage errors remain
document-specific; a later successful write to a different document cannot clear an outstanding snapshot failure.
It never writes from the crash handler. A clean save/undo retires only this session's snapshot. Edits made during
an admitted save remain dirty and retain recovery. Clean/discard shutdown waits asynchronously for retirement.
Recovery failure is visible and leaves editing and explicit Save available.
All dirty retained documents are checkpointed independently. After a crash/relaunch, the last durable source,
selection, BOM and original location of each copy remain available; live Undo/Redo histories are not serialized.
An isolated child-process regression edits two actual files through Files navigation, checkpoints them, crashes,
then relaunches and restores both exact sources while proving both originals unchanged and requiring Save As.
An unapplied local source draft is checkpointed as a proposed replacement in the recovery copy. The live source
and history remain unchanged. Restart previews that proposed text before Restore; Cancel retires it when the
authoritative source is clean. Invalid or oversized drafts retain the last valid checkpoint and show failure.
Decide whether a checkpoint needs source before copying the cached flattened text. A clean document with no
changed, snapshot-guarded local draft still submits the same immediate null retirement request, without making
a recovery text copy. Changed valid drafts produce the same validated proposed source and selection; this must
not change live source/history, worker validation, latest-value coalescing or the quiet interval.

Recovery parsing bounds decoded UTF-16 units independently of encoded UTF-8 byte lengths. A checksum-valid
payload beyond the editor's unit capacity returns invalid-data status from the noexcept boundary, preserving
every caller output field. Startup quarantines complete original bytes without truncation; if the quarantine
name already exists, retain both the unrenamed input and earlier quarantine. Other valid full-capacity snapshots
remain available. Expected malformed/oversized input must never terminate the parser or recovery worker.
Snapshots are strict versioned UTF-8 envelopes with bounded path/source lengths, BOM, selection, generation and
a payload checksum. Partial/corrupt or newer-version data is renamed `.quarantine` without overwriting it.
Startup examines at most 512 directory entries, counting unrelated names and entry types before filtering for
session envelopes. A directory filled with unrelated files cannot bypass that bound. It offers at most 64 valid
inactive sessions; remaining files are retained. The worker publishes the examined-entry count and whether the
entry limit stopped a nonempty remainder under the service mutex before reporting Ready. Those counters measure
actual startup work; they do not assert a filesystem completion-time bound or change snapshot ownership.
Another live session's leased snapshot is never offered or deleted. Candidate retirement after Restore happens
only after the new session's replacement is durable. The checksum detects corruption, not hostile tampering.
