# File safety

Atomic staging uses a bounded temporary leaf in the destination directory, independently of the target filename's
length. Regression coverage includes first publication and replacement of a 240-unit target without a leaked temporary;
`Storage.AtomicSaveKeepsLongLeafWithinPathBudget` exercises that boundary and the saved bytes/stamp.

Open and save local `.md`, `.markdown` and text files using strict UTF-8. Preserve BOM and original line endings;
invalid UTF-8/NUL/oversized files fail before replacing the current document. New documents use UTF-8 without BOM.
The v0 source limit is 65,536 UTF-16 units. Bounded reads reject files above the maximum possible encoded size.

Atomic save stages a unique sibling temporary file with `CREATE_NEW`, writes all bytes, flushes and closes it,
then uses `ReplaceFileW` for an existing destination or `MoveFileExW` for a new destination. Remove only the
owned temporary file on failure. Never clear dirty state on failed/cancelled storage or change path/BOM prematurely.
An existing file identity (size, last-write and content fingerprint) is checked immediately before replacement;
external modifications require a choice to reload, Save As or explicitly overwrite. Atomicity does not eliminate
the cross-process check/replace race; a stronger conditional-write protocol is a future hardening item.

New/Open/Close prompt Save, Discard, Cancel when dirty. Save As cancellation cancels the original action. Undo
back to saved text clears dirty; the saved baseline is exact source content, not just a transaction counter.
While the Save/Discard/Cancel question is open, duplicate native or command Close requests are ignored.
They keep the original prompt and continuation; they neither cancel it nor queue another Close. Cancel
leaves the current source, selection and history intact. A later fresh Close may ask again normally.

Close during an admitted save hides the window and keeps its notification owner, file worker and recovery
alive until the actual write/replace completes. Duplicate Close requests and queued editing commands are ignored
while waiting. Close supersedes an older New/Open continuation attached to that save. Success updates the saved
The host still receives native hide, focus-loss, capture-cancellation and size/DPI lifecycle notifications;
waiting must not strand text input, logical focus, prepared content or window geometry.
baseline to the admitted text; close and retire recovery only when the current source is clean. If edits made
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
Restart offers a read-only source preview with Restore a copy, Discard snapshot and Keep for later. The settings
drawer can reopen pending snapshots. Restored source retains BOM, line endings, original location for relative
images and source selection; it is dirty and Save requires Save As. It never silently overwrites the original.

Restore captures the clicked candidate's identity and immutable snapshot before asking about unsaved changes.
After the prompt or a successful admitted save, revalidate that exact identity before changing the editor.
If it is no longer offered, stop with a localized explanation; retain the current source revision, selection,
history, path and dirty/recovered state and every other candidate. Never substitute the next list entry.
A later explicit Restore may choose another available copy. Successful retirement names only the captured id.

Each process owns a random session identity and an exclusive lease. A single event-driven worker coalesces
one immutable latest value and writes after 750 ms of quiet; explicit checkpoints and shutdown bypass debounce.
It never writes from the crash handler. A clean save/undo retires only this session's snapshot. Edits made during
an admitted save remain dirty and retain recovery. Clean/discard shutdown waits asynchronously for retirement.
Recovery failure is visible and leaves editing and explicit Save available.
An unapplied local source draft is checkpointed as a proposed replacement in the recovery copy. The live source
and history remain unchanged. Restart previews that proposed text before Restore; Cancel retires it when the
authoritative source is clean. Invalid or oversized drafts retain the last valid checkpoint and show failure.
Decide whether a checkpoint needs source before copying the cached flattened text. A clean document with no
changed, snapshot-guarded local draft still submits the same immediate null retirement request, without making
a recovery text copy. Changed valid drafts produce the same validated proposed source and selection; this must
not change live source/history, worker validation, latest-value coalescing or the quiet interval.

Snapshots are strict versioned UTF-8 envelopes with bounded path/source lengths, BOM, selection, generation and
a payload checksum. Partial/corrupt or newer-version data is renamed `.quarantine` without overwriting it.
Startup scans at most 512 entries and offers at most 64 valid inactive sessions; remaining files are retained.
Another live session's leased snapshot is never offered or deleted. Candidate retirement after Restore happens
only after the new session's replacement is durable. The checksum detects corruption, not hostile tampering.
