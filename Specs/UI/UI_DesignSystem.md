# RedTilde design system

## Product direction

Write naturally. Keep the precision of Markdown. Use Word's familiar text actions without a permanently expanded
ribbon, pagination controls or modal formatting workflow. A quiet writing surface, one compact formatting row,
contextual selection tools and an optional outline leave the document dominant.

The app-owned Win32 non-client frame presents one merged document bar for brand, file identity, dirty state, view
selection, File menu, Save and caption controls. The native title text remains the current filename and dirty marker
for taskbar and Alt+Tab identity. The app owns non-client hit testing, caption actions, drag regions, DPI behavior
and caption-button UI Automation; DxUi's host owns only the client area and must not paint over or compete with the
app frame. Repeated edits with the same displayed identity skip redundant native title writes; save-state feedback,
Unicode, undo and document replacement stay exact.
Cold header layout reserves bounded identity space without initializing host typography. Once the host prepares
typography, measure the filename canonically and reuse it until text, available width, typography revision or DPI changes.

Brand: a flowing red tilde, with a restrained scarlet accent. App name is RedTilde. Short product line:
**Words, beautifully in flow.** The icon must read at 16/24/32 pixels as well as 256; preserve alpha.
The application extracts and retains bounded premultiplied BGRA pixels from its own 32-bit alpha icon resource
across device replacement; ordinary branding does not instantiate WIC. Native extraction must match the independent
WIC reference byte for byte, including transparent and partially transparent pixels, at native
16/24/32/48/64/128/256 sizes, scaled 20/40/96 sizes and Windows' default class-icon dimensions.
It prepares the device-dependent Direct2D bitmap through DxUi visual-content preparation. Paint only draws a bitmap
stamped for the host's current resource generation; resource queries and bitmap creation stay out of paint.

The native host opts into DxUi's bounded initial root-child submission policy after swap-chain creation or resize.
Only the first successfully completed D2D draw uses intermediate submissions, including when the hidden harness
reports occluded presentation. Warm frames keep the ordinary final submission. Flush/EndDraw failures retain retry
and device-loss recovery; source, draw order, overlays, focus and accessibility stay identical. Qualification includes
initial/reset completed work and retained memory alongside the fixed warm edit/frame comparison.

## Layout

The owner's image-2 Compose proposal is the visual target for the 2026-10-23 fidelity correction. The document
identity and small save-state chip share one baseline. View commands use underlined tabs; toolbar and caption
commands use subtle chrome, with persistent selection fill for checked toolbar commands. Only Save is a filled
primary action. Find has a leading search glyph and a separate keyboard hint. The Files pane has its own section
label, close action, root label, filter and aligned file/folder icons. Both navigation trees expose the pane ground
without an outer input frame. Block tools follow a short outline immediately; a long outline scrolls within the
space left for the inspector. A full-width command row omits overflow when every command fits.

The document's outer focus rectangle is decorative and omitted in ordinary themes. The editor retains its real
caret, selection, text services and keyboard routing; high contrast forces the shared focus indicator. In ordinary
light/dark themes links underline on hover or caret, with persistent underlines in high contrast.

The `reference-dark` harness scene uses the visible weekly-report source from image 2 with invented continuation,
the same twenty-file tree, seven headings and current section. Its canonical comparison is 1440x900 pixels at
96 DPI; additional 144/192-DPI captures keep the same DIP viewport. Source, pane state, requested/actual dimensions,
theme and SDK identity must accompany actual captures. Document counts always reflect the real fixture; the
proposal's illustrative word/character totals and unseen source are not fabricated.

Native document typography uses independent heading minima and a 10-DIP container indent; paragraph minima
are 18 DIP and list-item gaps 10 DIP. Real source blank lines remain navigable and contribute their actual height.
The reference fixture uses compact Markdown after headings rather than removing source sites. The inspector
shows the enclosing list's actual item count for list content, with the source-line diagnostic retained internally;
the visible and accessible label identifies bulleted versus numbered lists. File counts are quiet badges on
collapsed folders; the current edited file uses a dot with the textual Edited accessibility description. The
six-DIP splitter hit region remains usable while the idle presentation is a thin seam at the pane boundary.

The 56-DIP document bar groups brand, file identity/dirty state, global view selector, File menu, primary Save and
caption controls. A 48-DIP command row groups Files, text structure, inline emphasis, lists, insertion and contextual
commands; 20-DIP group dividers make these command families legible. The canvas starts below it with generous gutters and a configurable reading measure, defaulting to
760 DIP (480–1040 DIP). Source and Split use the available width. A 240-DIP right panel holds the hierarchical
outline, active-block actions and the equation inspector. Panel section labels use 11-DIP semibold uppercase text
with 0.08em tracking. It is hidden in Split and when the remaining width cannot
accommodate the reading measure plus the panel. Status shows words, characters, reading time, selection,
line/column, encoding and line endings; the document bar carries save state. Narrow windows collapse the inspector
before an open Files pane overlays the canvas; the reading measure is reduced only after that. At 800x560 DIP,
commands that do not fit move into overflow instead of wrapping or clipping. Focus mode hides optional chrome. No
overlapping or off-screen commands at 100/150/200% DPI.

Compose/Source/Split are visible, mutually exclusive underlined view commands with accessible names, mutually
exclusive toggle state exposed to UI Automation, and Ctrl+1/2/3. They are command buttons, not a RadioGroup; the
application enforces exclusivity. Use canonical DxUi Button toggle semantics; add shared support upstream and
consume its public API rather than copying control code into RedTilde. Theme and
preference choices are in a drawer rather than a second native settings app. Commands show labels/tooltips and
shortcut hints. Selection tools should appear
above selected text after pointer release, remain inside the viewport, and avoid following every pointer movement.
Escape dismisses; Alt+F10 or the context-menu key provides keyboard access. Context menus retain the source
selection and support arrows, Tab, Enter and Esc before returning focus.

Alt+F10 retains its first-action focus intent while native selection geometry is pending, keyed to the exact
source snapshot, selection and view. Fulfill it after preparation only while those keys still match; Escape,
navigation, a different source/selection or a view change cancels it. A prepared source range with no painted
extent is not pending work. Late syntax publication refreshes derived facts without reopening dismissed tools.

## Tokens and behavior

Use 4/8/12/16/24/32 DIP spacing; 6-DIP button corners, 10-DIP overlay corners. Body chrome uses Segoe UI Variable;
Markdown body uses DxUi typography, source uses its monospace role. Target at least 32-DIP compact buttons and
40-DIP primary actions. Colors derive from one host theme; respect high contrast and visible keyboard focus.
Do not communicate dirty/errors/mode only by color. UIA names identify mode buttons, formatting commands and
the Markdown document. Error messages explain the failed action and leave the source editable.
Hot localized status formatting borrows immutable executable resource text for the duration of formatting;
formatted control text remains owned. Resolve resources for each request rather than caching a thread's language.
Missing resource identifiers remain empty, and translated positional placeholders retain their order and meaning.

The application theme adapter supplies these default product roles over the pinned DxUi palette. System mode
chooses Light or Dark from the system setting; high contrast replaces product colors with system colors.
Chrome, panes and page correspond to `windowBackground`, `surfaceBackground` and `cardBackground`.

| Role | Light | Paper | Dark |
| --- | --- | --- | --- |
| Chrome / panes / page | `#F0F1F4` / `#F8F8FA` / `#FFFFFF` | `#EDE6D8` / `#F3EDDF` / `#FFFAEF` | `#17191E` / `#1C1F25` / `#22252B` |
| Text / subdued text | `#141414` / `#666666` | same | `#EBEDF2` / `#B3B5BA` |
| Accent / focus / links | `#C83C46` | `#C83C46` | `#FF7778` |
| Accent hover / pressed | `#B6303C` / `#A12A34` | same | `#FF999A` / `#FFABAC` |
| Primary action fill / text | `#C83C46` / white | same | `#C83C46` / white |
| Active and inactive selection / selected label | `#F2CFD3` / `#A12A34` | same | `#A63B46` / white |

Control borders and dividers retain the pinned DxUi roles. Current Files/outline rows and pressed toggles use
the selection tint; inactive rows retain its product color and contrast-resolved label. Primary actions retain
their explicit darker red fill in Dark mode so their white label stays readable. High contrast uses system
Highlight/HighlightText for active and inactive selection, Hotlight for links/focus, and Window/WindowText for
surfaces/text. Other Find matches use a subtle neutral fill distinct from the current selection.

Compose body is 16 DIP with a 1.65 line-height multiplier; H1 is 34/1.2 bold and H2 is 23/1.3 bold. List items
have 10-DIP separation and paragraphs retain a line of bottom space. Shared semantic minima credit real source
blank lines: one separator fills the paragraph margin; additional blank lines keep distinct caret sites. Headings and strong text use document
text ink. Split's rich side uses 15-DIP body. Focus uses 18-DIP body in a 680-DIP measure. Source and its
read-only Split mirror use the host Monospace family at 13.5/1.75. The text-size preference scales document
type, including headings and code. Source markers, subdued delimiters and colored URLs are prepared by DxUi;
the mirror enables the same immutable syntax analysis as the authoritative editor.

Chrome uses 14-DIP labels, 13-DIP tree/secondary labels and 12-DIP status text. Section labels are 11-DIP
semibold with 0.88-DIP total tracking. The selection toolbar's keyboard hint is an 11-DIP label in a bordered
shared CardPanel chip. Use 18-DIP Fluent line glyphs and 36-DIP command buttons (32-DIP compact minimum),
with 40-DIP primary actions. The brand is 28 DIP in the document bar; its owned icon source is premultiplied before
D2D upload and retained across device replacement. UTF-8 resource scripts declare code page 65001 explicitly.

In Light, Paper and Dark themes, rich-text links use the theme link color without a persistent underline; underline on
pointer hover or while keyboard focus/caret is within the link. High contrast keeps a persistent underline and
uses the system hyperlink color. Link rendering must retain hit testing, layout metrics,
semantic link exposure and keyboard navigation. The shared rich-text change belongs in canonical DxUi and is
consumed through its reviewed dependency pin; RedTilde must not fork or copy shared control implementation.

The 240-DIP right panel has a shared hierarchical Tree with 42-DIP rows, level indentation, semibold current
section and named UIA current state. Empty headings do not produce blank rows. Current block names its actual
Markdown kind and physical-line character count, or enclosing list kind and item count for list content;
equation controls appear only in current mathematical context.
Its read-only equation preview uses the same shared engine as the document and is labelled explicitly.
When the current-block label or its sidebar is hidden, retain the newest source-localized block facts without
formatting or publishing the label. Publish those facts before the sidebar becomes visible so the first paint and
UI Automation snapshot expose the current selection. The list summary and static current-block title retain their
existing immediate behavior.

Localize and publish the visible Paragraph/Math toolbar caption on initial synchronization and published context
transitions; its accessible command name remains Heading. Pending replacement syntax clears stale math and
temporarily publishes Paragraph, then current ready math restores Math. Repeating unchanged ready facts reuses
the caption while the equation preview still follows the exact current expression.

Derive the current block from the source-current canonical syntax tree's child/sibling links, whose source
order can differ from node-vector order. Descend the containing half-open source ranges; the deepest recognized
category wins, while ordinary paragraph/blank leaves retain their list-item or quote context. Categories include
headings, code, lists/items, tables, quotes, dividers, HTML source, link definitions, metadata, footnote definitions
and active source-edit regions. Mathematical context overrides that category. Pending syntax shows Paragraph
rather than a stale category. At final ACP an unterminated line retains its block; a trailing CR, LF or CRLF exposes
the new empty Paragraph line. Classification must preserve the actual source, ACP selection and undo history.

Publish an outline selection only when the selected or focused heading changes. Surviving rows keep the shared
Tree's reconciliation and reveal behavior after model updates; real heading changes and focus restoration remain
synchronous, including while the inspector is hidden. Repeated edits within one heading must not repeat the
same selection publication or change source selection/history.

Find/Replace is a 460-DIP overlay at the canvas's upper right. Its first row contains query, current/total
count, Previous (Shift+F3), Next (F3) and Close; the second contains replacement, Replace and Replace all.
The current match is the exact source selection. Other source highlights and nested per-section outline
counts are derived from the same cached query/revision, and disappear when Find closes.
Find preserves outline collapse choices; collapsed parents still count all nested matches without exposing hidden
descendants.

Enter/Space activation of a formatting button preserves one source transaction after returning editor focus.
The translated activation character must remain owned by that button action; it cannot become a second
editor insertion. The shared DxUi input dispatcher owns this rule for native and embedded consumers.

F2 source editing retains the rendered block's subtle source highlight and places a guarded draft directly
below its native geometry when space permits, clamping the complete panel into the viewport otherwise.
Retry placement after preparation if that geometry is unavailable; a provisional position is not settled
capture evidence. Show exact delimiters in the shared TextField Monospace role with an accent focus stroke,
an Equation source label for mathematical context, Apply (Ctrl+Enter) and Cancel (Escape). Apply is one exact
source transaction; Cancel preserves source revision, selection and undo/redo history.

## Files pane

The Files pane is available, closed by default, and lists Markdown files from the open document's containing folder
and subfolders. It is a listing/open surface only: it does not create, rename, delete, move, reveal or otherwise
operate on files. Its first command-row toggle uses Ctrl+Shift+E. The pane width is 280 DIP by default and persists
within 200–400 DIP. Opening moves focus into the tree; Escape or closing returns focus to the authoritative editor
with its source selection intact. The folder name is visible; its full path is a tooltip and UIA help text. An
untitled document explains that saving is required before files can be listed. Open/Save As re-roots the listing.

List `.md` and `.markdown` files. Exclude hidden and system entries. Show a directory only when Markdown exists
below it, directories before files, and natural Explorer-style ordering within each kind. Expand ancestors of the
current file; other directories start collapsed and show their Markdown file count. Reparse points may appear as
entries but are never traversed. A name filter retains matching files and their ancestor directories; Escape
clears the filter before dismissing the pane. The current file uses selection styling and keeps its filename as the
stable TreeItem Name. When edited, its localized state text is exposed through UI Automation ItemStatus. Tree
navigation supports arrows, Left/Right expansion, Home/End and type-ahead;
UI Automation exposes Tree/TreeItem, ExpandCollapse and SelectionItem. A completed single click invokes a file row
on release; the disclosure target continues to expand/collapse only, and keyboard activation remains Enter.
Notification refreshes preserve surviving expansion choices, initialize newly discovered directories collapsed and
retire state for removed directories.

Enumeration runs on a bounded application worker, never on the UI thread. Traverse to depth 8 and visit at most
5,000 total entries. When the visit limit is reached, the footer reports the actual number of Markdown files
returned and that traversal stopped at the 5,000-entry limit; it must not label the entry limit as a file count.
Report every applicable access/watch error and the snapshot's actual entry/depth bounds together; canonically measure
the wrapped footer and reserve its full height, with a 24-DIP minimum for a simple count, even in a 200-DIP pane.
Measure the footer while the Files pane is logically visible, and refresh stale width, text, typography and DPI
keys before arranging a reopened pane. Hidden layout retains its bounded cached geometry without requesting
text resources. Resolve Source and Split's monospace family from the public DxUi typography specification;
layout must not create a graphics device merely to query a font-family name.
Publish only on the UI owner, cancel by request and root identity, and reject stale results. Directory changes
refresh from coalesced change notifications, without a polling timer. Access/offline errors appear in the pane and
do not block editing. Selecting a file follows the ordinary Open flow, including the unsaved-changes prompt and
all file-safety checks.

## Preferences and focus

Settings are a non-modal right drawer with immediate effect. Appearance offers System, Light, Dark and Paper as
preview tiles. The System tile previews Windows' current light/dark preference independently of the selected
appearance; cache that preference at theme changes rather than reading the registry during layout.
The hidden Settings capture instead seeds a synthetic System Light preference before layout and rendering,
and declares that effective preference and fixture origin in its sidecar. This capture input does not change
the normal Windows-derived System preview or write the user's appearance preference.
reading width defaults to 760 DIP and is limited to 480–1040 DIP; text scale defaults to 100% and is limited to
75–200%; dimming inactive blocks is enabled by default.
The Files pane has an open/closed preference (closed by default) and a 200–400-DIP width
(default 280). Focus mode is session-local and is not persisted. The drawer scrolls when its contents do not fit,
and Escape/close returns focus to the editor. Focus mode leaves a 40-DIP strip with dimmed document identity,
dirty state, an Exit focus action and caption buttons; a bottom pill shows word count and save state. The status
line/column and encoding facts are hidden in Focus mode. Other content
blocks are dimmed when `dimInactiveBlocks` is enabled. Privacy offers “Load images from the internet”, off
by default, with an explanation that local images always show and enabling remote images contacts servers named
in the document. The preference applies immediately, persists per user and cancels/evicts remote work and pixels
when switched off. App workers fetch bounded anonymous HTTP(S); the shared editor never performs network I/O.

## Capture scenarios

Native mathematics reserves the full fraction/root/matrix height, aligns inline formulas with prose and uses
the host's text/selection palette. F2 exposes exact delimiters. The inspector is a read-only instance of the same
engine. A recovery sheet shows original location and a scrollable read-only source preview, followed in reading
order by Restore a copy, Discard snapshot and Keep for later. Restore returns focus to the authoritative editor;
the first save asks for a filename with a recovered suffix. Recovery errors remain visible in the document footer.

Additional deterministic scenarios: `math`, `math-dark`, `mermaid`, `recovery`. Recovery capture uses an explicitly
seeded synthetic candidate to show the actual sheet; crash/restart tests separately establish storage behavior.

The app harness owns `compose-light`, `compose-dark`, `source`, `split`, `context-selection`, `settings`,
`local-source`, `mermaid`, `math`, `math-dark`, `recovery`, `narrow`, `high-contrast`, `files`, `find`, `focus`, `equation`
and `reference-dark`. Captures must be actual application renders, tagged
with dependency SHA, source identity, binary hash, dimensions, theme and scenario. Proposal art is separately
labelled. Physical screen-reader/IME/touch acceptance and real focus restoration require separate evidence.
