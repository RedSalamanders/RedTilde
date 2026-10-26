# Markdown document and editing

## Source and views

The exact Markdown source is authoritative. Preserve UTF-8 encoding, optional BOM, Unicode scalar pairs and
existing line endings. View changes never reserialize Markdown, reset history or move selection.

- **Compose** renders supported Markdown and edits its source through DxUi semantic commands.
- **Source** displays exact Markdown in the same editor/document/history.
- **Split** keeps Compose editable and shows a read-only exact-source mirror. Entering Split synchronizes
  current source and selection; edits update the mirror while it is visible. Compose/Source defer its hidden
  updates. Selecting a mirror range may navigate the authoritative editor. The mirror never becomes a second
  independently editable document.
- The read-only Source mirror shares the primary immutable snapshot through the pinned public DxUi API while
  retaining its own revision counter and independent parser. Changed source retires derived state; same source
  and selection skip repeated work. An incompatible mirror profile retains exact `SetText` fallback. After any
  callback, abort an older update if source/revision/selection/view changes or an editor/mirror/root lifetime is
  retired. These cached controls are app-owned and never reparented independently of that root. Source sharing
  does not publish an old cache over a nested edit or change authoritative undo/history/dirty state.
- Text-service composition previews and cancellation can change live source and selection without a committed
  edit. A visible Split mirror synchronizes immutable source identity before applying their UTF-16 ACPs, even
  when lengths match but scalar/CRLF boundaries differ. This never adds authoritative history or invokes the
  committed document-change/recovery workflow. A result commit remains one undoable source transaction.
- Heading offsets, visible outline rows, line positions and Find section totals derive from the exact current
  source snapshot, including non-notifying previews and cancellation. Repeated selection/syntax notifications for
  one revision reuse its analysis. Structural previews do not apply committed collapse indices to different
  headings; cancellation restores the entry outline and its collapse choices, while a committed heading-structure
  change resets them. Previews and cancellation do not increment committed activity or submit recovery snapshots.
- **Local source** uses DxUi's source-region API for a prepared code, HTML, metadata or diagram block. F2 enters;
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

Provide New, Open, Save, Save As, undo/redo, bold, italic, strike, inline code, headings, paragraphs, bullets,
numbered lists, tasks, quotes, links, images, fenced code, tables and table-row/column actions. Commands use the
public planner when supported. An outline derives from ATX headings outside fences and navigates source ranges.
Refresh its source offsets on every edit, including prose inserted before a heading; unchanged labels and levels
do not require chrome layout. Renaming, changing the level or adding/removing a heading updates the visible outline.
Find operates on source, wraps with a visible result, and selects/reveals the match. Never search rendered-only text.
Cache exact match ranges by source revision and query, and section totals by that identity plus the outline rebuild
identity. Selection-only caret updates reuse match ranges and section totals; rebuild the bounded highlight list only
when its source/query or active current match changes. Render at most 4,096 match highlights, including the active
match even when it falls beyond that prefix, while Find navigation and Replace All continue to use the complete match
set. Closing Find clears highlights and section badges once; subsequent closed selection/syntax updates do not scan
source or rebuild badges. Reopening recomputes from the current source and query. Same-ACP text-service preview,
successive preview revisions and cancellation each use the exact current source revision without creating committed
history or losing the entry source's results on cancel.
The guarded local draft owns its separate exact-source highlight while open. Syntax refreshes must retain that
highlight; Cancel and Apply clear it explicitly before ordinary Find presentation resumes. A closed-Find cache
must never leave a draft highlight behind or clear an active draft's highlight as incidental work.

Shortcuts: Ctrl+N/O/S, Ctrl+Shift+S, Ctrl+Z/Y, Ctrl+B/I, Ctrl+F, Ctrl+1/2/3 for Compose/Source/Split,
Ctrl+Shift+E for the Files pane, F2 local source, F11 focus mode, Ctrl+, settings, Esc dismiss/restore. Keep native
IME handling ahead of commands that could interrupt composition. Ctrl accelerators exclude Alt so AltGr text reaches
native translation; F2/F11 are unmodified, F3 permits only optional Shift, and Alt+F10 excludes Ctrl/Shift.
Keyboard-only access must reach formatting,
outline, Files, settings and menus. Compose/Source/Split use segmented command buttons with consumer-owned,
mutually exclusive toggle state exposed through UI Automation; they are not a RadioGroup.

## Files navigation

The available optional Files tree lists `.md` and `.markdown` files in the current document's folder and subfolders. It
supports listing and opening only. Opening a row uses the ordinary Open path, prompts before replacing dirty
source, and retains the existing file-safety behavior. Save As and Open change the tree root. An unsaved document
shows an explanation in place of a listing.

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
navigation remain disabled. Asset copying and drag/drop remain separate work.

Mermaid uses the exact library pin's native flowchart subset plus bounded state/class profiles. State syntax
supports direction, simple declarations/quoted aliases, initial/final `[*]`, transitions with colon labels and
state descriptions. Class syntax supports simple declarations, colon members, associations and inheritance.
The deterministic native graph router need not match Mermaid coordinates. Unsupported families, composite
states, block class declarations, namespaces, multiplicities, styles and directives remain exact fenced source.

Mathematics uses native source-backed objects throughout the document and in the read-only inspector. RichMarkdown
recognizes inline `$...$` / `\(...\)` and display `$$...$$` / `\[...\]` inside one logical Markdown paragraph.
Code, escaped delimiters and CommonMark/GFM profiles retain literal interpretation. Single-dollar notation has
nonspace content edges and cannot close immediately before a digit. Explicit slash delimiters allow whitespace
around the formula and use their own closing-delimiter rules, independent of dollar-currency heuristics. `\text{...}`
preserves internal spaces as upright text. Typed atom classes, display/text/script styles and explicit operator
limits survive parsing into native layout. A formula reserves its full native height, shares an inline baseline and
exposes its TeX notation through semantic accessibility. View changes do not edit it.

The library parser handles fractions, indexed roots, simultaneous scripts, Greek/operators, named functions,
text and alphabets, accents, scalable paired delimiters, matrices, cases and aligned/gathered rows. Limits are
4,096 source units, 512 nodes, depth 24 and 16 rows/columns/128 cells per equation; total mathematical records
are 4,096 and combined native objects 128. Each preparation event shapes at most one mathematical leaf. Paint
uses cached glyphs/lines and inherits theme/selection brushes. No web runtime, external TeX, packages or network fonts.

F2 on a math object opens its exact source envelope in the guarded local draft. Apply is one source transaction;
Cancel preserves source/history. Unsupported/malformed/oversized notation remains exact literal source with a
diagnostic. Full LaTeX/MathML and arbitrary Mermaid conformance remain separate compatibility goals.

Inspector selection changes use only the shared immutable Markdown syntax for the current source revision.
While analysis is pending or unavailable, clear the inspector temporarily instead of rescanning the whole source
with a delimiter fallback. Current syntax excludes code/escaped notation and covers both dollar and slash delimiters.

Aligned equations share right/left operand edges at each ampersand pair; cases align both columns left and
ordinary matrices center each column. Oversized notation conservatively keeps the remaining inline physical
line (display: logical leaf) literal with a diagnostic. The inspector clears over-capacity values.

### Remote image policy

Remote image requests are disabled in the shared editor and application preferences by default. The explicit
Settings privacy toggle enables only absolute HTTP(S) image requests; links remain inert. DxUi resolves and
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
