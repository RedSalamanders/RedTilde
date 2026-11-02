# RedTilde guide

The [root README](../README.md) covers launching and shortcuts. Start with Compose, select text for formatting,
and use Source to inspect exact Markdown. Switch views freely; the document, source selection and undo history
are shared. Split is an editable Compose pane next to a read-only source mirror.

**Local editing:** F2 on a code or diagram object opens its exact source in place. On ordinary text, F2 opens
the selected range/current line in a source draft while the rest stays rendered. Apply/Ctrl+Enter commits one
edit, and Esc cancels. Link insertion opens a reviewable source draft rather than replacing selected words
without a chance to edit the destination.

**Formatting:** the Text style menu offers paragraph/headings; the toolbar offers bold/italic/code, bullets and
tasks. Insert offers link, image, table, Mermaid, equation, quote and code. The context menu adds strike,
numbered lists and table row/column commands. Unavailable structural actions retain source and explain context.

**Markdown syntax:** tables, task lists, strikethrough and autolinks always work. Settings › Markdown syntax also
turns on `==highlight==`, `^superscript^`, `~subscript~`, `++inserted++`, wiki links `[[page]]`, definition lists
(a term line followed by `: definition` lines) and abbreviations (`*[HTML]: Hyper Text Markup Language`). All are on
by default, each option explains its syntax, and changes apply immediately without editing the document. Text for
a syntax you turn off stays exactly as typed. RedTilde does not open wiki pages or load `![[file]]` embeds yet.

**Files pane:** press Ctrl+Shift+E to list Markdown files in the saved document's folder and subfolders. Filter by
name, expand directories, and open a file with a single click or Enter. Opening follows the normal unsaved-changes
prompt and file-safety rules. The pane is listing/opening only; it does not manage files. Untitled documents show
why there is no listing yet. At most 5,000 directory entries are examined to depth 8; the pane reports the actual
Markdown result count separately from traversal-limit or access errors.

**Images:** save the document first when you want relative image paths. Insert an image from file or paste an
Explorer file path in rich text. RedTilde references that file; it does not copy the asset into a new folder.
The image must remain available when reopening the Markdown. Supported files decode asynchronously and
unsupported/missing images retain alt text. Internet images are off by default. Enable “Load images from the
internet” in Settings only when you want to contact the image servers named by the document; turning it off
removes remote images immediately. Links stay inert.

The HTTP client loads on demand on the image worker when a permitted image first needs it. Its system-library
imports stay loaded until exit so outstanding callbacks can retire safely. Windows library loading itself cannot
be interrupted; cancellation and the request deadline are checked before contacting the image server.

**Equations:** use `$...$` / `\(...\)` inline. For display notation, put `$$` (or `\[` and `\]`) on lines of
their own above and below the formula; a one-line `$$x$$` stays as typed. Fractions,
indexed roots, simultaneous scripts, Greek/operators, named functions, text, alphabets, accents, scalable
delimiters, matrices, cases and aligned/gathered rows render natively in the document. Matrix cells use `&`
and rows use `\\`. F2 edits the exact equation, including its
delimiters, as one undoable source edit. Unsupported notation stays literal with diagnostics. The optional
inspector uses the same native engine. This bounded notation is not full LaTeX/package compatibility.

**Mermaid:** flowcharts, simple state diagrams and simple class diagrams use native C++ rendering. State
transitions support initial/final markers, labels and descriptions; class diagrams support members, associations
and inheritance. Composite states, class blocks/namespaces/multiplicities, styles/directives and other families
remain fenced source. F2 edits a diagram in place. See the [editor contract](../Specs/Editor/Editor_Markdown.md).

**Files and diagnostics:** save explicitly to write your Markdown file. Recovery checkpoints unsaved writing
after 750 ms of quiet. On restart, inspect the source preview, Restore a copy, Discard snapshot or Keep for
later; reopen pending snapshots from Settings. Restored copies require Save As and suggest a recovered filename.
Unapplied local drafts are preserved as proposed edits in the previewable recovery copy. Cancelling a clean
draft retires that proposed snapshot. Another live app's snapshots are excluded. Corrupt/newer data is retained
as `.quarantine` under `RedTilde\Recovery`. A crash before the quiet interval can lose the newest keystrokes.
Undoing to the exact saved text clears dirty state; edits made during a save remain protected against the text
actually written.
Files over 65,536 UTF-16 units and malformed
UTF-8 are rejected, never truncated. Save As can preserve a separate version when another app changed the file.
Crash reports remain local, with a notification on the next launch. A dump may contain document text; sharing
one is your choice. Settings use `RedTilde.settings` in LocalAppData; System/Light/Dark/Paper and outline changes
apply immediately. High contrast takes precedence.

See [example.md](examples/example.md), [technical notes](examples/technical-notes.md), [the design system](../Specs/UI/UI_DesignSystem.md),
[actual captures](../Measurements/README.md) and [current capability status](../capabilities.json).
The [sample document guide](examples/README.md) also links complex math, flowchart,
state/class and intentional-fallback documents, with an edit/undo and presentation checklist.

[RedMonitor diagnostics](Diagnostics.md) explains the optional viewer and application logging.
