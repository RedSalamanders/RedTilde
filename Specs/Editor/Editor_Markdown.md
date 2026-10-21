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
- **Local source** uses DxUi's source-region API for a prepared code, HTML, metadata or diagram block. F2 enters;
  Esc/F2/Ctrl+Enter returns. IME composition and stale semantic publications refuse the transition.
  For ordinary text without a native source-region action, open a local source draft for the selected range or
  physical line. The rest stays rendered; Apply/Ctrl+Enter commits one snapshot-guarded source transaction,
  Esc cancels, and the main editor is read-only while the draft is open. A draft is not a second committed document.

All commands preserve selection while opening a toolbar/menu. Apply one source transaction per user action,
restore editor focus, and keep exact undo. Source view formatting may use the editor's source marker commands;
structural semantic actions require current rich analysis. Unavailable actions explain the compatible context.

## Formatting and navigation

Provide New, Open, Save, Save As, undo/redo, bold, italic, strike, inline code, headings, paragraphs, bullets,
numbered lists, tasks, quotes, links, images, fenced code, tables and table-row/column actions. Commands use the
public planner when supported. An outline derives from ATX headings outside fences and navigates source ranges.
Refresh its source offsets on every edit, including prose inserted before a heading; unchanged labels and levels
do not require chrome layout. Renaming, changing the level or adding/removing a heading updates the visible outline.
Find operates on source, wraps with a visible result, and selects/reveals the match. Never search rendered-only text.

Shortcuts: Ctrl+N/O/S, Ctrl+Shift+S, Ctrl+Z/Y, Ctrl+B/I, Ctrl+F, Ctrl+1/2/3 for Compose/Source/Split,
F2 local source, F11 focus mode, Ctrl+, settings, Esc dismiss/restore. Keep native IME handling ahead of commands
that could interrupt composition. Keyboard-only access must reach formatting, outline, settings and menus.

## Images, diagrams and mathematics

Use the pinned native image API with an explicit absolute document path. Relative paths resolve from the
document directory, never process CWD. Paste uses upstream structured file-reference policy. Decode supported
local PNG/JPEG/BMP/GIF/TIFF off the UI thread; retain alt text on failure. No HTTP/data URI fetch, UNC access by
default, script execution or automatic external link navigation. Asset copying and drag/drop remain separate work.

Mermaid uses the exact library pin's native flowchart subset plus bounded state/class profiles. State syntax
supports direction, simple declarations/quoted aliases, initial/final `[*]`, transitions with colon labels and
state descriptions. Class syntax supports simple declarations, colon members, associations and inheritance.
The deterministic native graph router need not match Mermaid coordinates. Unsupported families, composite
states, block class declarations, namespaces, multiplicities, styles and directives remain exact fenced source.

Mathematics uses native source-backed objects throughout the document and in the read-only inspector. RichMarkdown
recognizes inline `$...$` / `\(...\)` and display `$$...$$` / `\[...\]` inside one logical Markdown paragraph.
Code, escaped delimiters and CommonMark/GFM profiles retain literal interpretation. Single-dollar notation has
nonspace content edges and cannot close immediately before a digit. A formula reserves its full native height,
shares an inline baseline and exposes its TeX notation through semantic accessibility. View changes do not edit it.

The library parser handles fractions, indexed roots, simultaneous scripts, Greek/operators, named functions,
text and alphabets, accents, scalable paired delimiters, matrices, cases and aligned/gathered rows. Limits are
4,096 source units, 512 nodes, depth 24 and 16 rows/columns/128 cells per equation; total mathematical records
are 4,096 and combined native objects 128. Each preparation event shapes at most one mathematical leaf. Paint
uses cached glyphs/lines and inherits theme/selection brushes. No web runtime, external TeX, packages or network fonts.

F2 on a math object opens its exact source envelope in the guarded local draft. Apply is one source transaction;
Cancel preserves source/history. Unsupported/malformed/oversized notation remains exact literal source with a
diagnostic. Full LaTeX/MathML and arbitrary Mermaid conformance remain separate compatibility goals.

For inspector selection changes, use current Markdown syntax before falling back to bounded delimiter extraction
while analysis is pending. Current syntax excludes code/escaped notation and covers both dollar and slash delimiters.

Aligned equations share right/left operand edges at each ampersand pair; cases align both columns left and
ordinary matrices center each column. Oversized notation conservatively keeps the remaining inline physical
line (display: logical leaf) literal with a diagnostic. The inspector clears over-capacity values.
