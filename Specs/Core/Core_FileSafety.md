# File safety

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

Recovery is an independent per-session UTF-8 snapshot under LocalAppData, containing document path/encoding and
source. It must never replace the user's Markdown file automatically. A clean save retires the matching snapshot.
Restart offers a read-only source preview with Restore a copy, Discard snapshot and Keep for later. The settings
drawer can reopen pending snapshots. Restored source retains BOM, line endings, original location for relative
images and source selection; it is dirty and Save requires Save As. It never silently overwrites the original.

Each process owns a random session identity and an exclusive lease. A single event-driven worker coalesces
one immutable latest value and writes after 750 ms of quiet; explicit checkpoints and shutdown bypass debounce.
It never writes from the crash handler. A clean save/undo retires only this session's snapshot. Edits made during
an admitted save remain dirty and retain recovery. Clean/discard shutdown waits asynchronously for retirement.
Recovery failure is visible and leaves editing and explicit Save available.
An unapplied local source draft is checkpointed as a proposed replacement in the recovery copy. The live source
and history remain unchanged. Restart previews that proposed text before Restore; Cancel retires it when the
authoritative source is clean. Invalid or oversized drafts retain the last valid checkpoint and show failure.

Snapshots are strict versioned UTF-8 envelopes with bounded path/source lengths, BOM, selection, generation and
a payload checksum. Partial/corrupt or newer-version data is renamed `.quarantine` without overwriting it.
Startup scans at most 512 entries and offers at most 64 valid inactive sessions; remaining files are retained.
Another live session's leased snapshot is never offered or deleted. Candidate retirement after Restore happens
only after the new session's replacement is durable. The checksum detects corruption, not hostile tampering.
