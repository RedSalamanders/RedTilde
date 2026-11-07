# Markdown document and editing

The editor defaults to DxUi's Unicode-first clipboard preference. When Unicode text and a bitmap coexist,
ordinary Paste selects text in one ownership attempt. Settings provides an independently persisted image-first
option. File-list priority and forced plain-text paste remain governed by the shared input contract.

## Source and views

The exact Markdown source is authoritative. Preserve UTF-8 encoding, optional BOM, Unicode scalar pairs and
existing line endings. View changes never reserialize Markdown, reset history or move selection.

Source-view Enter and Shift+Enter use that same first physical terminator, retain
the exact ending UTF-16 caret position, and create one undoable edit and recovery
submission. RedTilde routes these messages through the adopted DxUi editor.
The native text store must reject a staged edit beyond 65,536 UTF-16 units before
changing source, selection or history; an in-range retry remains usable and the
visible Split mirror follows only accepted source changes. Retained text stores
disconnect when their application owner is destroyed.

File-paste preparation rejects file names containing quotes, CR or LF before
producing a Markdown replacement in Compose, Source and Split. A refusal leaves
the source, output replacement, selection, history and recovery unchanged. This
prevents an invalid quoted file name from introducing an image or link. Ordinary
valid file references remain supported; files are not opened during preparation.

New Mermaid, equation, empty-selection quote and fenced-code templates use the document's first physical
terminator (LF, CRLF or CR), with CRLF for an empty or unterminated document. Mixed outside source remains
unchanged. Changed ordinary F2 drafts and their recovery previews normalize only the replacement text by
that same policy. An unchanged draft preserves its exact original mixed endings. Validate editor capacity
after normalization, and keep each accepted replacement in one source/history transaction.

- **Compose** renders supported Markdown and edits its source through DxUi semantic commands.
- **Source** displays exact Markdown in the same editor/document/history.
- **Split** keeps Compose editable and shows a read-only exact-source mirror. Entering Split synchronizes
  current source and selection; edits update the mirror while it is visible. Compose/Source defer its hidden
  updates. Selecting a mirror range may navigate the authoritative editor. The mirror never becomes a second
  independently editable document.
- The read-only Source mirror shares the primary immutable snapshot through the pinned public DxUi API while
  retaining its own revision counter and independent parser. Changed source retires derived state; same source
  and selection skip repeated work. An incompatible mirror profile retains exact `SetText` fallback. After any
  callback, abort an older source-bound update if source/revision/selection/view changes or an editor/mirror/root lifetime is
  retired. These cached controls are app-owned and never reparented independently of that root. Source sharing
  does not publish an old cache over a nested edit or change authoritative undo/history/dirty state.
  View routing checks the captured editor, mirror and root lifetimes after synchronous callbacks. If only source
  changes during the requested view transition, finish its hint, layout, focus and persisted preference using
  the latest source. A newer view request or retired owner aborts the outer transition.
- Text-service composition previews and cancellation can change live source and selection without a committed
  edit. A visible Split mirror synchronizes immutable source identity before applying their UTF-16 ACPs, even
  when lengths match but scalar/CRLF boundaries differ. This never adds authoritative history or invokes the
  committed document-change/recovery workflow. A result commit remains one undoable source transaction.
- Heading offsets, visible outline rows, line positions and Find section totals derive from the exact current
  source snapshot, including non-notifying previews and cancellation. Repeated selection/syntax notifications for
  one revision reuse its analysis. Structural previews do not apply committed collapse indices to different
  headings; cancellation restores the entry outline and its collapse choices, while a committed heading-structure
  change resets them. Previews and cancellation do not increment committed activity or submit recovery snapshots.
- **Local source** uses DxUi's source-region API for a prepared code, HTML, metadata or diagram block, or an inline
  or display equation. F2 enters;
  Esc/F2/Ctrl+Enter returns. IME composition and stale semantic publications refuse the transition.
  For ordinary text without a native source-region action, open a local source draft for the selected range or
  physical line. The rest stays rendered; Apply/Ctrl+Enter commits one snapshot-guarded source transaction,
  Esc cancels, and the main editor is read-only while the draft is open. A draft is not a second committed document.

Dirty-state and recovery-copy reuse follow the [file-safety contract](../Core/Core_FileSafety.md). Silent source
replacement and text-service preview/cancellation participate through immutable source identity; cached facts
never make a new revision clean without checking its exact saved content and recovered-copy state.

All commands preserve selection while opening a toolbar/menu. Apply one source transaction per user action,
restore editor focus, and keep exact undo. Source view formatting may use the editor's source marker commands;
structural semantic actions require current rich analysis. Unavailable actions explain the compatible context.
Keyboard activation characters belong to the command that consumed their key: Enter or Space on a toolbar
button must not insert a newline/space into the editor after its callback returns focus. Canonical DxUi owns
that dispatch contract. After application/TSF shortcut pretranslation, each live host's message pump calls
`ControlHost::DispatchKeyboardMessage` before the ordinary `TranslateMessage`/`DispatchMessage` path. The shared
host dispatches Return/Space once and permits translation only when the original text-input owner still owns
the character. RedTilde must not filter ordinary editor characters locally to compensate.

## Formatting and navigation

Insert Quote and Insert Code Block transform a nonempty selection through
`RichTextEditor::WrapSelectionInMarkdownBlock` in Compose, Source and the primary Split editor. Quote prefixes
each selected physical line; Code Block encloses the exact payload with a fence longer than any selected run
of backticks and the `text` language. Retain unselected spelling and every original selected CRLF, LF or CR.
Use the document's first line-ending style for new separators (CRLF when there is none), adding the blank
separation required to make a block without deleting outside content. Preserve selection direction and
logical editor focus. One Undo restores the original source, selection and clean/dirty identity; one Redo
restores the whole transformation. A selected command refused for capacity, read-only/disabled input or an
active text-service preview/composition must retain source, selection and history and show the unavailable
status. Never replace a refused selection with sample content. With a collapsed selection, retain the
existing Quote and Code Block insertion templates. These source transactions require no prepared rich geometry.

Provide New, Open, Save, Save As, undo/redo, bold, italic, strike, inline code, headings, paragraphs, bullets,
numbered lists, tasks, quotes, links, images, fenced code, tables and table-row/column actions. Commands use the
public planner when supported. An outline derives from ATX headings outside fences and navigates source ranges.
Refresh its source offsets on every edit, including prose inserted before a heading; unchanged labels and levels
do not require chrome layout. Renaming, changing the level or adding/removing a heading updates the visible outline.
Find operates on source, wraps with a visible result, and selects/reveals the match. Never search rendered-only text.
Find is case-insensitive by default, using locale-independent ordinal Unicode comparison. A localized Match case
toggle enables exact case matching. Preserve original UTF-16 ACPs and source edit boundaries in either mode.
Cache exact match ranges by source revision, query and case preference, and section totals by that identity plus the outline rebuild
identity. Selection-only caret updates reuse match ranges and section totals; rebuild the bounded highlight list only
when its source/query or active current match changes. Render at most 4,096 match highlights, including the active
match even when it falls beyond that prefix, while Find navigation and Replace All continue to use the complete match
set. Closing Find clears highlights and section badges once; subsequent closed selection/syntax updates do not scan
source or rebuild badges. F3/Shift+F3 repeat the retained query while the bar stays closed. Keep that bounded
closed navigation cache separate from visible highlights and badges; refresh it only on an explicit repeat when
its source/query/case identity changes. Transfer its storage on close/reopen rather than duplicating the match set.
Reopening recomputes from the current source and query. A successful single Replace selects the following match
without requiring a second navigation action; each replacement remains one undoable source transaction. Same-ACP text-service preview,
successive preview revisions and cancellation each use the exact current source revision without creating committed
history or losing the entry source's results on cancel.
The guarded local draft owns its separate exact-source highlight while open. Syntax refreshes must retain that
highlight; Cancel and Apply clear it explicitly before ordinary Find presentation resumes. A closed-Find cache
must never leave a draft highlight behind or clear an active draft's highlight as incidental work.

Ctrl+plus/equal, Ctrl+minus and their numeric-keypad equivalents change the persisted document text scale by
five percentage points within 75–200%; Ctrl+0 restores 100%. Ctrl+vertical-wheel uses the same command
admission and accumulates sub-notch deltas; a multi-notch message applies its bounded change in one action.
An ordinary wheel message ends that partial gesture. Do not intercept AltGr. Consume admitted Ctrl-wheel
without additionally scrolling. A clamped no-op causes no layout or preference submission. These operations
preserve exact source revision, directional selection, undo/redo, saved state and logical focus in every view;
the existing active-composition and pending-local-draft guards still apply.

### Markdown syntax options

The editor, Split mirror and equation inspector analyse with the RichMarkdown profile and one explicit extension
mask. GFM tables, task lists, strikethrough, autolinks and the tag filter are always on. Seven optional families
are individually controlled preferences, each enabled by default: highlight `==text==`, superscript `^text^`,
subscript `~text~`, inserted `++text++`, wiki links `[[target]]` / `[[target|label]]` / `![[target]]`, definition
lists (a term followed by `: description` lines) and single-line abbreviation definitions `*[term]: text`.
Toggling one applies the new mask to all three controls at once, so the mirror stays eligible for parsed-syntax
adoption. Reanalysis changes presentation only: source, selection, undo history, revision and dirty state are
unchanged. Text for a disabled family remains exact literal source. Undo and redo decline while a changed
text-service preview is active and return when it is cancelled or committed.

Shortcuts: Ctrl+N/O/S, Ctrl+Shift+O for Open Folder, Ctrl+Shift+S, Ctrl+Z/Y, Ctrl+B/I, Ctrl+F, Ctrl+1/2/3 for Compose/Source/Split,
Ctrl+Shift+E for the Files pane, F2 local source, F11 focus mode, Ctrl+, settings, Esc dismiss/restore. Keep native
IME handling ahead of commands that could interrupt composition. Ctrl accelerators exclude Alt so AltGr text reaches
native translation; F2/F11 are unmodified, F3 permits only optional Shift, and Alt+F10 excludes Ctrl/Shift.
Keyboard-only access must reach formatting,
outline, Files, settings and menus. Compose/Source/Split use segmented command buttons with consumer-owned,
mutually exclusive toggle state exposed through UI Automation; they are not a RadioGroup.

## Files navigation

The available optional Files tree lists `.md` and `.markdown` files in the persistent selected root and subfolders. It
supports listing and opening only. Opening a row activates a retained document or reads a new document into the
workspace without a save question. Each of at most 64 documents retains its authoritative source, oriented ACP
selection, Undo/Redo history, exact saved-content baseline, BOM, file stamp and recovered-copy state. Reopening a
retained path does no read and does not replace edits with an externally changed file. Global Compose/Source/Split
presentation and the read-only Split mirror use the newly active source; presentation does not merge histories.
The first opened/saved file seeds an empty root. After that, Open File, Save As and New keep that root. Explicit
Open File, Open Folder, New and Close protect every dirty document through Save/Discard/Cancel. A decision applies
only to the exact document/source revision shown; edits made during an asynchronous save/open need fresh protection.
Cancel, including cancelling the file/folder picker, retains all sources and histories. Successful Open File/New
starts a new workspace; successful Open Folder starts an empty workspace at the new root. Restart restores the
root and pane visibility. Without a root, the pane explains Open Folder and Open File.

When multiple documents are retained, the Files tree also exposes an **Open documents** group. Its stable
workspace identities distinguish untitled/missing files and separate recovered copies with the same original
path. Folder listing, expansion, filtering and the persistent root remain independent. Dirty dots cover retained
documents as well as the active file. Capacity refusal explains the limit and preserves every source, selection
and history; it must never evict an edited document to admit a completed read. Switching declines atomically
while a file operation, local draft, native composition or an unsaved question owns the current binding.

An Open completion arriving while a local source draft is active retains its staged result. It cannot replace
the editor, close the draft or discard its text. After Apply/Cancel, the ordinary source-specific unsaved protection
runs before that exact staged result is applied. A later Close invalidates both the reader and any held result.

### Fenced code presentation

The authoritative editor registers DxUi's public `TokenizeBuiltinCode` callback. C-like, JSON, PowerShell and
diff fences receive bounded keyword, literal and comment ink during native preparation. Unknown languages or
capacity refusal retain ordinary complete code ink. Tokenization never changes source, selection, history or
dirty state, and composition never parses code. The existing Subtle code frame supplies language/line presentation;
an application code-copy command remains separate work.
Unordered task-list presentation should use the checkbox as its sole visible list marker while preserving its exact source
anchor, interaction and accessibility. Ordered tasks retain their number. Task markers use the theme accent; high contrast
uses foreground ink. The Writing preference **Strike through completed tasks** is disabled by default and changes only
Compose/Split presentation. It strikes the nearest checked item's body, including inline formatting and continuation
paragraphs; nested unchecked or ordinary list items keep their own state. Toggling it never inserts `~~`, changes source,
selection, revision, undo/redo history or dirty state. Explicit source strikethrough remains independent of this preference.

Subtle tables reserve ten DIP on each horizontal cell side when their width permits; intrinsic column sizing includes
those insets. Inline-code chips reserve three DIP at both text endpoints in native glyph metrics so their background
does not touch the letters or cover adjacent prose. Wrapping, RTL carets and source mapping use those same metrics.
Fenced-code content retains its separate frame/gutter spacing. Adjacent labeled diagram ranks reserve measured label
space along the graph direction, including endpoint cardinalities; simple labeled arrows use that inter-box corridor.

Enumeration is bounded and asynchronous: maximum depth 8 and 5,000 total visited entries. Hidden/system entries
are excluded; reparse points can be listed but are never traversed. Prune directories without Markdown below
them, order directories before files, then use Explorer-style natural order. Expand the current file's ancestors;
other directories start collapsed and show their Markdown count. A name filter keeps matching files and ancestor
directories. The footer distinguishes the actual returned Markdown-file count from the total-entry visit limit.
Directory change notifications are coalesced; no polling timer is used. Requests are canceled and results rejected
when their root/request identity becomes stale. Access and offline errors stay local to the pane and do not block
editing.

The tree supports arrow navigation, Left/Right collapse and expand, Home/End, type-ahead, and Tree/TreeItem UI
Automation with ExpandCollapse and SelectionItem. Opening moves focus into the tree. Escape/close restores focus to
the authoritative editor without changing its source selection. The active file keeps its filename as the TreeItem
Name; when edited, its localized state text is exposed as UI Automation ItemStatus, not by color alone. A row invokes
on single-click release; file rows open through the guarded Open flow,
while directory rows are ignored by the application and their separate disclosure target expands/collapses.
Arrow keys navigate/select; Enter invokes the selected row. Outline rows also invoke on single-click release and
navigate to the source heading, including headings with descendants.

The persistent status line shows word count, character count and estimated reading time; its position side reports
selection length, line/column, UTF-8 BOM state and line-ending kind. Transient command feedback appears in the
document footer without replacing those facts. Focus mode hides position status and keeps the count/save summary
in its bottom pill.

Word count is a simple source-unit run count, not full Unicode word segmentation. ASCII A–Z, a–z and 0–9 are
word units; non-ASCII UTF-16 units retain the existing C-runtime `iswalnum` or U+2E80-and-above rule. Consecutive
word units count once; any other unit separates runs. Supplementary scalars are classified as their two UTF-16
units, consistent with the editor's source positions. Full language compatibility remains a separate gate.

Position and current physical-line length come from the source revision's `DocumentInfo` index, using a
logarithmic `DocumentPositionAt` query on caret movement. Do not scan the complete document again for status,
newline kinds or a physical line's character count. Lines and columns count source UTF-16 units, start at one,
and exclude terminators from the line-content length. CRLF is one terminator; the next line starts after its
LF. An interior CRLF offset reports the preceding line's content end. Empty source is line 1/column 1 with zero
content; a final CR, LF or CRLF exposes an empty final line. Newline facts remain revision-bound and distinguish
no terminator, LF, CRLF, lone CR and mixed kinds.

Analysis stages the next revision's complete facts while the previous `DocumentInfo` remains published, then
swaps them without throwing. The application may reuse one retired physical-line index. Its retained spare
capacity is at most `min(16384, max(64, 2 * currentLineCount))` entries; oversized retired buffers are released
at publication. The active index is bounded to `max(64, 2 * currentLineCount)` entries after analysis. Capacity
bounds do not truncate source or lines. Reserve hints are capped by the incoming source length plus one;
documents above the spare limit retain that hint rather than repeatedly growing from zero. Caret-only refreshes
do not consume storage or reanalyze source. Preview, cancellation, commit and undo/redo each publish exact
source-bound facts, with no changes to the editor's source, selection or history.

## Images, diagrams and mathematics

Use the pinned native image API with an explicit absolute document path. Relative paths resolve from the
document directory, never process CWD. Paste uses upstream structured file-reference policy. Decode supported
local PNG/JPEG/BMP/GIF/TIFF off the UI thread; retain alt text on failure. HTTP(S) requests require the explicit
privacy opt-in below. Data URI fetch, UNC access by default, script execution and automatic external link
navigation remain disabled. External asset copying and drag/drop remain separate work. The image worker branches on every
request kind: wiki-target (`![[target]]`) and data requests are declined with `ERROR_NOT_SUPPORTED` before any file
or network access, even when a same-named local file exists. RedTilde resolves no wiki targets. The explicit
external-link activation handler declines wiki destinations, so unresolved wiki links stay inert.

### Clipboard bitmap assets

RedTilde owns the pinned editor's bitmap-paste callback in Compose and Split. The callback copies at most
32 MiB of borrowed CF_DIB/CF_DIBV5 bytes and admits one asynchronous operation through creation, owner
disposition and cleanup. It returns immediately without inserting source. WIC decoding/PNG encoding and all
filesystem work run on an application-owned COM MTA worker. Validated 24/32-bit RGB/bitfield input is bounded
to 8192 pixels per side and 64 MiB of decoded/output bytes; embedded/linked color profiles are refused.

The worker creates a unique PNG beneath the local fixed-drive application data directory's `Assets` folder.
It never replaces an existing file and publishes success only after durable flush and full file-stamp
capture. The source uses an absolute escaped file URI, so an unsaved document can render the asset immediately.
These references depend on this local asset store: moving/exporting a document to another machine does not
copy assets. Accepted assets remain after Undo so Redo, saved documents and crash-recovered Markdown retain
their image references. Automatic unreferenced-asset collection is outside this contract.

The owner inserts the returned URI through the public Markdown image command only while the admitted source
snapshot, selection, document identity/path, view, control lifetime and editing context still match. Read-only,
Source-only, source-draft, prompt, gesture or closing contexts decline. A later source edit retires the request
permanently even if Undo restores its wording. New/Open/Open Folder, document switching and accepted Close
also retire pending insertion. Errors, stale results and busy refusal leave source/selection/history unchanged.
Successful insertion replaces the selection as one undoable edit. A leased result keeps the one-operation
bound across reentrant change notifications and Close.

Unreferenced declined assets retire on a worker only when identity, size, timestamp, link count and content
still match the created file. Changed, linked or reparse entries are preserved. Failed cleanup retains the
complete file and records a diagnostic; it must never delete an unrelated replacement. Shutdown waits
asynchronously for this worker alongside preferences and recovery, without joining storage on the UI thread.

Mermaid uses the exact library pin's bounded flowchart, state, class, sequence, pie, entity-relationship, Gantt and
mind-map profiles. Supported grouping, members, multiplicities and native style subsets follow that pin's
`docs/rich-text.md`; do not freeze an older family list into app behavior or examples. The deterministic native
graph/timeline router need not match Mermaid coordinates. Advanced constructs outside each documented grammar,
arbitrary CSS and interaction directives remain exact fenced source. Each supported family must remain a native
diagram in Compose, with exact source/history through Source, Split and F2. Source fallback is local to an
unsupported object; a supported styled label must not force unrelated Markdown into Source presentation.

The adopted sequence extension adds actor figures and plain aliases; chronological left/right/over notes;
bare `autonumber` counting messages from one, excluding notes; and solid/dotted messages without arrowheads.
Derived numbers do not enter the Markdown source. Notes retain their text and order in accessibility, and
F2 edits use ordinary source undo/redo. Directionless flowchart headers default to top-to-bottom placement.
Custom number arguments, activations, control fragments and creation/destruction remain whole-fence fallbacks.
Default diagram connections, arrowheads, lifelines and outlines use effective foreground ink in every theme;
faint decorative Markdown borders do not control meaningful diagram structure. Authored palettes and explicit
snapshot/export stroke choices retain the library's separate contract.

Mathematics uses native source-backed objects throughout the document and in the read-only inspector. RichMarkdown
recognizes inline `$...$` / `\(...\)`. Display math is a source-owned block whose `$$` or `\[` / `\]` delimiters
stand on their own logical lines; closed `math` and `latex` fences also render as display formulas. At the
current pin a one-line `$$x$$` or `\[x\]` stays literal. Insert Equation writes own-line delimiters.
Code, escaped delimiters and CommonMark/GFM profiles retain literal interpretation. Single-dollar notation has
nonspace content edges and cannot close immediately before a digit. Explicit slash delimiters allow whitespace
around the formula and use their own closing-delimiter rules, independent of dollar-currency heuristics. `\text{...}`
preserves internal spaces as upright text. Typed atom classes, display/text/script styles and explicit operator
limits survive parsing into native layout. A formula reserves its full native height, shares an inline baseline and
exposes its TeX notation through semantic accessibility. View changes do not edit it.

The library parser handles fractions, indexed roots, simultaneous scripts, Greek/operators, named functions,
text and alphabets, accents, scalable paired delimiters, matrices, cases and aligned/gathered rows. The pinned
limits are 16,384 source units, 4,096 nodes, group depth 64 and 64 rows/columns/1,024 cells per equation; total
mathematical records are 32,768 and combined native objects 128. Each preparation event shapes at most one mathematical leaf. Paint
uses cached glyphs/lines and inherits theme/selection brushes. No web runtime, external TeX, packages or network fonts.

F2 on a math object shows its exact source envelope, delimiters included, in place through DxUi's native source
region, like code and diagram blocks. Typing edits the authoritative source directly; each edit is ordinary undoable
history. F2, Esc or Ctrl+Enter returns to the rendered formula and keeps the edits. A display block ends at its
closing delimiter, excluding that line's terminator; a caret at the start of the following line is outside the
equation for F2 and the inspector. If the native region refuses (IME composition or a stale or pending
publication), F2 falls back to the guarded local draft of the same envelope: Apply is one source transaction and
Cancel preserves source/history. Unsupported/malformed/oversized notation remains exact literal source with a
diagnostic. Full LaTeX/MathML and arbitrary Mermaid conformance remain separate compatibility goals.

Inspector selection changes use only the shared immutable Markdown syntax for the current source revision.
While analysis is pending or unavailable, clear the inspector temporarily instead of rescanning the whole source
with a delimiter fallback. Current syntax excludes code/escaped notation and covers both dollar and slash delimiters,
inline formulas and closed display blocks. The preview places its `\[` and `\]` delimiters on their own lines.
While a math source region is open, the inspector previews the TeX currently between that region's delimiters,
reading only the region's range, and the current block reads Equation; it clears when the delimiters are incomplete.

Aligned equations share right/left operand edges at each ampersand pair; cases align both columns left and
ordinary matrices center each column. Oversized notation conservatively keeps the remaining inline physical
line (display: logical leaf) literal with a diagnostic. The inspector clears over-capacity values.

### Remote image policy

Remote image requests are disabled in the shared editor and application preferences by default. The explicit
Settings privacy toggle enables only absolute HTTP(S) image requests; loading an image never opens a link.
Explicit confirmed link dispatch is independent of remote-image permission. DxUi resolves and
cancels immutable requests and decodes bounded encoded images. RedTilde's existing single image worker owns the
transport and applies the same 32-job/64-MiB completion budget as local images. Opt-off immediately retires remote
requests/pixels, preserves local identities and prevents stale completion publication.

The EXE delay-loads `WINHTTP.dll` and the selftest-only `WS2_32.dll`; ordinary startup must not resolve these
imports on behalf of unused image support. The image worker resolves all WinHTTP imports after pure URI,
limit and pre-cancellation admission and before its first WinHTTP call. Only the delay loader's missing-module
and missing-procedure structured exceptions become failing HRESULTs, preserving its captured Win32 error when
available; all other structured exceptions continue search. The Windows 11 system-API dependency remains
required. Imports remain loaded until process exit so late asynchronous handle callbacks cannot outlive their
code; disabling remote images cancels requests without unloading the client. OS library loading cannot be
preempted by the request abort event. The transfer deadline starts before resolution, and cancellation/deadline
are checked again before entering transport. This does not promise a hard bound on the OS loader itself.

`Common/RemoteImage.*` performs anonymous direct GET with no proxy discovery, cookies, redirects, authentication,
referrer or client certificate. Standard TLS 1.2/1.3 validation is retained. Require status 200, identity encoding, at
most 32-KiB headers, 16-MiB encoded bytes and an 8-second whole-transfer deadline; decoded limits remain 4,096 per
dimension and 4,194,304 pixels. Failures retain alt text. Request cancellation is checked within 50 ms during active I/O and
owner-close signals an event; the UI never waits for or closes a network transfer. WinHTTP callbacks retain the
fixed transfer buffer through the documented final
[HANDLE_CLOSING notification](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nc-winhttp-winhttp_status_callback).
Tests use only an owned loopback server, proving default zero requests, explicit opt-in, exact pixels, refusal,
cancellation and off/teardown behavior without contacting live internet services.

Source view and the read-only Split mirror use the host's monospace family at 13.5/1.75, scaled with text size.
Canonical DxUi source ink prepares syntax ranges cooperatively; delimiters are subdued, heading/list markers
and URL destinations use the theme accent. It never removes syntax, changes ACP mapping or edits source/history;
high contrast retains ordinary system text. Rich document headings retain their 34/23 scale independently.
