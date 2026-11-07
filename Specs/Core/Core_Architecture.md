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
| Image file notifications | Shared directory-notification machinery in `Common/Files.*`, one process-counted worker, at most 32 files/eight local directories, coalesced owner notification |
| Mathematics | DxUi source-backed inline/display objects and read-only inspector, native cached glyphs/lines, explicit diagnostics |
| Files navigation | Application-owned bounded directory enumeration/watch worker, cancellation and stale-result identity; the UI opens files through the normal file-safety path |
| Harness | Hidden window, direct control commands, render capture and immutable scenario/build receipts |

Library objects remain inside the executable. No plugin ABI is introduced. The native host is used directly;
RedTilde must not allocate a competing swap chain or render over an uncoordinated chrome HWND.

Application command admission, menu completion, Win32 dispatch and message-loop completion share an explicit
API-5 failure boundary. Expected refusal uses checked HRESULT, source or command results, logs its cause locally
and retains the document/history. Unexpected C++ exceptions, including allocation failure, require fatal local
diagnostics and termination; they cannot resume a partially executed UI action. A declined Close stays handled
and never falls through to default window destruction. Checked completion refusal cancels stale deferred file
actions, keeps the pending result and permits an explicit later retry. Status publication never recursively
retries UI after another failure. Native message dispatch and synchronous COM/Win32 calls use audited native
boundaries so native internally handled exceptions stay native and reentrant application/SDK callbacks regain
their own scopes. Direct construction is scoped; the outer message loop and final process drain stay outside.
Menu completion revalidates root lifetime after the command and keeps focus in a newly opened local/Link draft,
drawer or Find/Recovery surface. Hidden supplied-message/callback tests exercise these same entry points without
opening a desktop menu or injecting input.

The process owner starts local crash storage and a dedicated report writer, installs SDK protocol-2 hooks, then
opens worker admission before constructing the application. One counted lease covers each admitted callback
through its owned data, SDK objects and scopes. Files, recovery, preferences, bitmap creation/cleanup, image
decoding, link dispatch, optional monitor launch and final WinHTTP notification retirement participate.
Closing a view does not uninstall the failure host. Ordinary process shutdown closes producers, destroys app
owners, closes admission and drains work on the COM-initialized owner before process-wide UIA cleanup and the
failure-host BUSY drain. Each cooperative drain has a five-second deadline; a blocked callback cannot be
interrupted by it. Timeout is a failed shutdown with fatal diagnostics, never permission to unload live owners.
The final OS session-end path retains its existing recovery allowance and transfers the process owner to OS
teardown instead of adding these ordinary shutdown waits. See [crash/shutdown ownership](Core_SettingsAndCrashes.md).

RedTilde owns its Win32 non-client frame policy and caption behavior. The merged document bar is app-owned client
content coordinated with that frame; the native window title remains the filename plus dirty marker for taskbar
and Alt+Tab identity. The app handles non-client sizing/hit testing, drag regions, caption buttons, DPI and their
UI Automation behavior. DxUi's `ControlHost` owns and renders the coordinated client area only. Any shared caption,
button or rich-text behavior is implemented in canonical DxUi and consumed through its public API and reviewed pin;
consumer code must not copy shared controls or reach into library internals.

External link opening is an explicit application action, delegated by DxUi's Ctrl+click/Ctrl+Enter/UIA
activation callback or a source-context Open link command. Ordinary clicks never launch a destination.
Only bounded http, https and mailto targets without control characters, whitespace, embedded NUL or quotes
may reach confirmation; executable/file/custom schemes and unresolved wiki targets remain inert. This is an
application scheme/argument policy over canonical semantic destinations, not a competing Markdown/URL parser.
Copy the destination before confirmation, default to No, and preserve the exact source snapshot, selection,
workspace key and owner lifetime across that reentrant boundary. A changed/closed owner declines dispatch.
Bind an asynchronously completed Open link menu choice to its original source/selection as well.
Admit at most one dispatch. A fresh value-owned STA worker initializes COM and calls ShellExecuteEx without
requesting a browser process handle or displaying shell error UI. No shell/filesystem wait occurs on the UI
thread. Publish only a bounded status through the owning notification binding; Close clears that binding
without joining an arbitrary external handler. The worker retains no Application/control pointer. Hidden
tests use explicit confirmation/handoff seams and owned gates, exercise real STA initialization and complete
teardown, and never open a real external application or contact the network.
Native control-focus requests require the app's visible, non-minimized foreground window. Retain logical
focus without activating a hidden/background owner, including delayed startup/recovery completion.

The Files pane provides listing/activation; the application retains each opened document's source, oriented
selection, Undo/Redo history, saved baseline and recovery identity through the same editor's document exchange.
It lists `.md` and
`.markdown` paths from a persistent application-owned selected root using an application worker with depth-8 and
5,000-total-visited-entry limits. This limit counts entries examined, including names that are not Markdown files.
The worker publishes only to the UI owner;
request/root identities cancel obsolete work and reject stale completions. Coalesced directory notifications
refresh the view without polling. Reparse points are never traversed. The pane can open a Markdown path only by
submitting it to the workspace file path, preserving path validation, per-document recovery and conflict checks.
Switching retained documents does not prompt; explicit New/Open File/Open Folder/Close protect all dirty documents.
Directory enumeration and notification handles follow WIL ownership and close during cancellation/teardown.

The experimental upstream editor admits 65,536 UTF-16 units and 128 history transactions. Product file admission
must honor that actual limit and report rejection. Never truncate. Large-file support requires a separately
qualified upstream capacity increase. The UI must keep unsupported Markdown available as exact source.

No periodic autosave/polling timer: edited revisions arm one debounced recovery write; subsequent idle work sleeps.
File/decode results carry a document generation and are applied only on the owner. Closing invalidates the window
binding before workers can post. Image decoding must not join a blocked codec on UI teardown.
Pruning cancelled image work signals the network-abort event only for a cancelled current remote transfer.
An empty current request must not wake that event. Pruning still releases completion reservations and schedules
valid queued work; a cancelled transfer must not prevent a later admitted request from completing.

Network images have two application-owned admission layers: a live workspace-document grant or the persisted
always-allow preference, followed by a separate local-network preference. Only the authoritative editor issues
requests; neither permission nor reload changes its source/history. DNS lookup and WinHTTP callbacks retain
process-work leases and their native buffers through their own completion, including after owner cancellation.
The selected numeric endpoint is pinned per request while HTTP authority and TLS use the original hostname.
The direct-only transport does not discover proxies or run PAC scripts, and never silently falls back to a
different transport. A network requiring a proxy receives a visible load failure with preserved alt/source.
This delivery choice resolves Q19; adding proxy configuration is a separately specified feature.

Local image refresh reuses the shared directory-notification implementation, rather than adding a polling
timer or a second directory enumerator. Admit at most 32 current image request paths in eight directories.
Watch setup and native cancellation/drain stay on the retained worker; source-tagged completion and all editor
calls stay on the owner. Match bounded notification names and coalesce an active burst for at most 250 ms.
Overflow refreshes only the existing bounded image set. A first refresh after arming closes the initial decode/
notification gap. Changes and explicit Reload use the existing bounded SDK reload; network images are retried
only under their current independent permissions. This does not change source, selection, history or saved state.
Over-capacity/failed automatic watching retains editing and explicit Reload rather than admitting extra watchers.
Closing invalidates publication and wakes the worker; UI teardown never waits for directory I/O. The process-work
owner still counts that worker until its buffers and overlapped operations have actually drained.

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
