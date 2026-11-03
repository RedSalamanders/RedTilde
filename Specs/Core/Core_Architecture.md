# Architecture and Red-family conventions

Status: intended contract, with implemented scope in `capabilities.json`.

RedTilde edits local Markdown on Windows. It shares the Red family's native C++/Win32, DxUi, WIL, Unicode,
PowerShell build, deterministic harness, theme/settings and local crash-report conventions. It is an independent
repository and executable. It must launch and test without RedXe, RedPrism or RedSalamander present.

| Owner | Responsibility |
| --- | --- |
| Application | Window lifetime, command dispatch, prompts, dirty state, settings and session identity |
| DxUi ControlHost | Native text services, UIA, retained controls, rendering, device loss and presentation |
| RichTextEditor | Authoritative UTF-16 Markdown source, immutable snapshots, selection and undo history |
| Storage | Strict Unicode codecs, bounded reads, same-directory atomic replacement and file identity |
| Image service | Bounded application-owned decode jobs, COM/WIC, cancellation and owner completion |
| Mathematics | DxUi source-backed inline/display objects and read-only inspector, native cached glyphs/lines, explicit diagnostics |
| Files navigation | Application-owned bounded directory enumeration/watch worker, cancellation and stale-result identity; the UI opens files through the normal file-safety path |
| Harness | Hidden window, direct control commands, render capture and immutable scenario/build receipts |

Library objects remain inside the executable. No plugin ABI is introduced. The native host is used directly;
RedTilde must not allocate a competing swap chain or render over an uncoordinated chrome HWND.

RedTilde owns its Win32 non-client frame policy and caption behavior. The merged document bar is app-owned client
content coordinated with that frame; the native window title remains the filename plus dirty marker for taskbar
and Alt+Tab identity. The app handles non-client sizing/hit testing, drag regions, caption buttons, DPI and their
UI Automation behavior. DxUi's `ControlHost` owns and renders the coordinated client area only. Any shared caption,
button or rich-text behavior is implemented in canonical DxUi and consumed through its public API and reviewed pin;
consumer code must not copy shared controls or reach into library internals.

The available Files pane owns no separate document model and provides listing/opening only. It lists `.md` and
`.markdown` paths from a persistent application-owned selected root using an application worker with depth-8 and
5,000-total-visited-entry limits. This limit counts entries examined, including names that are not Markdown files.
The worker publishes only to the UI owner;
request/root identities cancel obsolete work and reject stale completions. Coalesced directory notifications
refresh the view without polling. Reparse points are never traversed. The pane can open a Markdown path only by
submitting it to the normal Open flow, preserving dirty prompts, path validation, conflict checks and atomic-save
rules. Directory enumeration and notification handles follow WIL ownership and close during cancellation/teardown.

The experimental upstream editor admits 65,536 UTF-16 units and 128 history transactions. Product file admission
must honor that actual limit and report rejection. Never truncate. Large-file support requires a separately
qualified upstream capacity increase. The UI must keep unsupported Markdown available as exact source.

No periodic autosave/polling timer: edited revisions arm one debounced recovery write; subsequent idle work sleeps.
File/decode results carry a document generation and are applied only on the owner. Closing invalidates the window
binding before workers can post. Image decoding must not join a blocked codec on UI teardown.
Pruning cancelled image work signals the network-abort event only for a cancelled current remote transfer.
An empty current request must not wake that event. Pruning still releases completion reservations and schedules
valid queued work; a cancelled transfer must not prevent a later admitted request from completing.

Native window captions cache only successfully applied identity text. Skip identical writes during an already-dirty
typing sequence, but apply filename and dirty-state transitions synchronously. Invalidate the cache before entering
Win32 and after a failed or reentrantly superseded write, so later requests remain retryable. Require the own-window
`WM_SETTEXT` receiver's positive acknowledgment before caching a value. Do not defer caption
work beyond the measured source transaction merely to reduce command latency.

Application-derived text is cached against the editor's immutable source snapshot, never a revision number alone.
Keep at most one cached string/snapshot and one last-mirrored source snapshot within the admitted document bound.
Dirty-state checks compare exact source with the last successful save, including same-length edits, undo and
direct document resets that bypass the change callback. Recovery consumes that current source and still stages
an unapplied local draft only in its proposed recovery copy. A cached view never becomes an editable authority.

`AnalyzeDocument` builds line starts/content lengths and the newline-kind facts with the other derived facts
for one immutable source revision. The owner retains that value with the revision it analyzed and replaces it
on a source change; caret-only movement never rebuilds it. `DocumentPositionAt` uses binary search over that
index without scanning/copying source or allocating. Line and column are one-based source UTF-16 positions;
line content length excludes CR/LF. Empty source has one empty line and a final newline adds an empty final
line. A CRLF pair is one terminator: an invalid ACP inside the pair clamps to the preceding content end, and
the next line begins only after both units. Oversized offsets clamp to source end. Newline facts distinguish
none, LF, CRLF, lone CR and mixed kinds. The line count agrees with the index, and the heading limit remains 128.
The prior revision's physical line count may hint the next index allocation, bounded by the incoming source's
UTF-16 length plus one and the vector's representable capacity. This hint changes no metadata: scan the complete
incoming source and publish the completed replacement as before. Do not retain the predecessor's large index
storage after a small replacement or keep a second document-analysis/text cache. Reuse the current refresh's
heading-identity comparison for ordinary commit reconciliation; an active preview still compares against its
saved committed outline so cancellation, collapse restoration and result commits retain their existing behavior.

Refresh the read-only source mirror only while Split is visible and its source snapshot has changed. Entering
Split synchronizes source and selection before revealing it without resetting the editable history. Ordinary
source changes refresh counts and heading offsets. Heading labels/levels/count refresh the Tree model and its
rows without recalculating the fixed application regions. Resize, view, theme and explicit panel commands retain
their own layout paths. Outline navigation always uses the latest offsets even if the displayed heading labels
keep their existing bounds.

Before visible Source-mirror preparation, the app may supply the primary's completed canonical Markdown syntax
through the pinned SDK's read-only adoption API. Both editors must still share the exact immutable source,
parent/host/root and compatible grammar on the UI owner thread. The app adapter borrows the primary behind a
weak lifetime token and rechecks both editors and the owning root after scheduling. It retains no second
editable authority. The SDK shares immutable parse storage under the mirror's own revision and document identity;
literal glyph geometry, source coloring, source ACPs, history and selection/scroll behavior remain unchanged.
Unavailable/incompatible syntax uses ordinary independent parsing. Hidden mirrors stay deferred, and Rich-view
mirrors retain the independent fallback. This reuse does not change math/Mermaid language support.

Source/selection/view/analysis identity changes refresh persistent position facts and visible Split selection.
A later syntax-only publication refreshes syntax-derived formatting, current block and equation context without
repeating unchanged position formatting or mirror selection/scroll work. Same-ACP text-service previews and
cancellation still change source identity and synchronize the visible mirror before applying selection; no
history or recovery transaction is introduced. Retain deterministic work counters beside source/history,
outline navigation and settled rendering checks so reduced work cannot hide stale presentation.

Construct and publish the localized current-block label only when its resource category or physical-line
character count changes. Invalidate the publication cache before calling the control and retain newer nested
publication facts. This cache does not suppress source/syntax analysis, inline-format toggles, equation source,
command enablement or preview/cancellation synchronization. Reset it when creating the label.

Performance qualification uses paired Release measurements on the same machine and fixture, recording source,
dependency, binaries, WARP/hardware mode, dimensions, dirty/idle/hidden memory and CPU. Initial app measurements
establish a baseline; library receipts do not establish application non-regression.
