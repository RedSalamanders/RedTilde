# RedTilde guide

Source-view Enter and Shift+Enter keep your document's LF, CRLF or CR line endings.
Each Enter is one Undo step. Documents are limited to 65,536 UTF-16 units; text
input that would exceed the limit leaves your last accepted text intact.
Invalid file names containing quotes or line breaks are refused when preparing a paste.

**Pasting pictures:** paste a bitmap into Compose or Split to create a local PNG and replace the selection
with its image reference in one Undo step. When the clipboard also contains text, text wins by default;
Settings → Privacy → Prefer images when pasting switches that preference. Pasting never waits for image
encoding on the UI thread. Editing or changing documents while a paste is pending cancels its insertion.
Pictures live in RedTilde's local data `Assets` folder and survive Undo/Redo and recovery. When sharing or
moving a document, copy its pictures and update the Markdown references; export does not bundle them automatically.
The [bitmap paste evidence](../Measurements/BitmapPaste_2026-11-07/README.md) includes actual Light/Dark
preference captures, cancellation and Undo checks, and the remaining platform boundaries.

Invalid or oversized recovery copies are kept intact in quarantine instead of interrupting recovery. Valid
copies remain available. An existing quarantine file is preserved; the original input stays intact if it
cannot be moved beside that earlier file. Recovery never silently truncates your writing.

Saving through a symbolic link keeps the link and updates its target. A file with several hardlinks keeps those
aliases and their shared identity: RedTilde first stores an independent complete recovery copy, then writes the
shared file in place. If that write fails or is interrupted, it can leave partial file bytes; restart offers the
complete admitted source as a recovery copy. Unavailable recovery storage prevents that write from starting.

If a storage provider explicitly does not support replacing an existing file, document Save uses that same
recovery-protected in-place method. It preserves the existing file identity and rechecks conflicts first.
An interrupted or failed write can leave partial file bytes; the complete admitted writing remains available
for recovery. Other save failures keep their ordinary error. Actual cloud, network-share and WSL support still
requires separate location qualification.

The [linked-save evidence](../Measurements/LinkedSaveAndCosmeticSettlement_2026-11-06/README.md) records the
actual interrupted-write recovery checks and the remaining symbolic-link runtime verification.

Every modified workspace document has its own recovery copy. Healthy editing checkpoints after a short pause,
with a five-second maximum admission delay while typing continuously. After a crash and relaunch, preview the
last durable copies and choose **Restore a copy** or **Restore all copies**. Each restored document remains dirty
and requires Save As; the original files stay untouched. The live Undo/Redo history is retained while switching
files, but is not saved in recovery copies.

The [root README](../README.md) covers launching and shortcuts. Start with Compose, select text for formatting,
and use Source to inspect exact Markdown. Switch views freely; the document, source selection and undo history
are shared. Split is an editable Compose pane next to a read-only source mirror.
Source stays editable while syntax colours and code decorations finish appearing. Those updates preserve
your selection and never cancel a selection drag or withhold an otherwise ready document frame.

Quotes and callouts leave a small inner margin on both sides. Long documents show a scrollbar matching the
current theme in Compose, Source and each Split pane; click its track or drag the thumb to move through the document.
See the [actual native captures and matched comparisons](../Measurements/QuoteMarginsAndScrollbars_2026-11-04/README.md).

Diagram arrow labels blend into the graph background when contrast is sufficient. Otherwise a subtle light
or dark backing improves readability. Notes and filled groups keep their own surface; Mermaid source and
undo history stay unchanged.

**Closing files:** Close hides the window while an already admitted Save finishes. A failed write restores
the window with an error and keeps your source and recovery copy. Edits made after Save still need a
Save/Discard/Cancel choice. Close abandons an Open that is still loading; any edits typed meanwhile remain
protected, and a cancelled Close cannot later apply that abandoned file.

**Local editing:** F2 on a code or diagram object opens its exact source in place. On ordinary text, F2 opens
the selected range/current line in a source draft while the rest stays rendered. Apply/Ctrl+Enter commits one
edit, and Esc cancels. Link insertion opens a reviewable source draft rather than replacing selected words
without a chance to edit the destination.
New templates and changed drafts follow the first line ending in the document; an empty document uses CRLF.
Untouched source keeps its original line endings, including mixed files. Recovery previews use the same rule
as Apply, and Undo restores the complete original source.

**Formatting:** the Text style menu offers paragraph/headings; the toolbar offers bold/italic/code, bullets and
tasks. Insert offers link, image, table, Mermaid, equation, quote and code. The context menu adds strike,
numbered lists and table row/column commands. Unavailable structural actions retain source and explain context.
Select text before **Insert → Quote** or **Insert → Code Block** to turn that text into the block. This works
in Compose, Source and Split, preserves the selected wording and line endings, and takes one Undo to reverse.
With no selection, the commands insert their usual editable example.

**Find and replace:** Ctrl+F searches the exact Markdown source without changing it. Search ignores letter case
unless **Match case** is selected. F3 finds the next occurrence and Shift+F3 the previous one, wrapping at the
ends; both work after closing the Find bar. Replace changes the current occurrence in one undoable edit and
selects the following match. Replace All is also one undoable edit.

**Text size:** Ctrl+plus/equal or Ctrl+minus changes document text size by 5%; the numeric-keypad keys also
work. Ctrl+mouse wheel changes the same preference, including high-resolution wheels, and Ctrl+0 restores
100%. Size stays between 75% and 200%, is remembered in Settings, and leaves your writing and Undo history intact.

**Window:** RedTilde remembers its normal window bounds and maximized state. It fits the window inside an
attached monitor's work area when reopening, including after changing monitors or display scale. A launcher's
explicit minimized or other show-state request takes precedence. Minimized bounds are never saved as the normal size.

**Starting again:** launching without a file reopens the last active saved document after checking recovery.
Pending recovery copies take precedence. A missing document leaves a blank Untitled document and an explanatory
status. Writing while startup is loading keeps your new text; New cancels that automatic load. Untitled and
recovered copies become a normal startup file only after Save As. The Files root is remembered separately.

**Markdown syntax:** tables, task lists, strikethrough and autolinks always work. Settings › Markdown syntax also
turns on `==highlight==`, `^superscript^`, `~subscript~`, `++inserted++`, wiki links `[[page]]`, definition lists
(a term line followed by `: definition` lines) and abbreviations (`*[HTML]: Hyper Text Markup Language`). All are on
by default, each option explains its syntax, and changes apply immediately without editing the document. Text for
a syntax you turn off stays exactly as typed. RedTilde does not open wiki pages or load `![[file]]` embeds yet.

**Files pane:** choose **File → Open Folder...** (Ctrl+Shift+O) to select a notebook root, or use **Open File...**
(Ctrl+O); the first file establishes its containing folder when no root exists. The selected root stays in place
when opening another file, using New, or saving elsewhere. Restart restores the folder and the pane's saved
visibility. Switching file rows keeps each document's edits, selection and Undo/Redo without a save question.
Open documents also keeps untitled files and recovered copies reachable, including copies with the same original
path. Explicit Open File, Open Folder, New and Close ask about every modified document; cancelling a question or
picker keeps the workspace. The workspace holds up to 64 documents and explains the limit before refusing another
file. Press Ctrl+Shift+E to show or hide the pane. Filter by name, expand directories, and open a file with a single
click or Enter. The pane is listing/opening only; it does not manage files. Without a selected root,
the empty pane explains how to open a folder or file. An unavailable remembered folder reports an error while
editing remains available; choose Open Folder to replace it. At most 5,000 directory entries are examined to depth 8; the pane reports the actual
Markdown result count separately from traversal-limit or access errors.

**Images:** save the document first when you want relative image paths. Insert an image from file or paste an
Explorer file path in rich text. RedTilde references that file; it does not copy the asset into a new folder.
The image must remain available when reopening the Markdown. Supported files decode asynchronously and
unsupported/missing images retain alt text. Remote images are off by default. Choose **Allow here** in the
document's image bar to load them for that document, or **Always allow remote images** in Settings. Each open
document keeps its own permission; a replacement document and restart ask again unless the global setting applies.
Private/local-network addresses require the separate **Allow images from local networks** choice, off by default.
That choice also permits an image on the current document's exact network share. Other shares and redirected
image paths remain blocked. Local image changes refresh automatically within the bounded watch capacity;
completed image loads and ordinary prose edits keep those watches active. Removing or retargeting an image
retires its previous file binding, and repeated reloads of one image use one watch slot.
Changing a remote image also keeps unchanged local-image watches and avoids an extra network reload.
**File → Reload images** retries images without editing the document. Networks that require a proxy show a
load failure; image fetching uses a direct connection.
Use **Block here** to remove a document's remote images or **Reload images** to retry failures without changing
text or Undo history. The bar explains blocked, oversized or failed image loads. Ordinary link clicks keep editing.

**Open a link:** put the caret in a link and choose **Open link** from its context menu, or Ctrl+click a rendered
link. Review the destination and confirm before RedTilde hands it to your default application. Ordinary clicks
keep editing. External http, https and mailto links are supported; local files, custom schemes and unresolved
wiki targets stay inert. External dispatch runs away from the UI thread and never changes your source or Undo history.

The HTTP client loads on demand on the image worker when a permitted image first needs it. Its system-library
imports stay loaded until exit so outstanding callbacks can retire safely. Windows library loading itself cannot
be interrupted; cancellation and the request deadline are checked before contacting the image server.

**Equations:** use `$...$` / `\(...\)` inline. For display notation, put `$$` (or `\[` and `\]`) on lines of
their own above and below the formula; a one-line `$$x$$` stays as typed. Fractions,
indexed roots, simultaneous scripts, Greek/operators, named functions, text, alphabets, accents, scalable
delimiters, matrices, cases and aligned/gathered rows render natively in the document. Matrix cells use `&`
and rows use `\\`. F2 shows the exact equation source, delimiters included, in place; type to edit it and
press F2, Esc or Ctrl+Enter to see it rendered again. Each edit is undoable, and the inspector renders the TeX as
you type. Unsupported notation stays literal with diagnostics. Rendering capacity and unsupported notation
retain distinct diagnostic and accessibility explanations. The optional
inspector uses the same native engine. This bounded notation is not full LaTeX/package compatibility.

**Mermaid:** bounded flowchart, state, class, sequence, pie, ER, Gantt and mind-map profiles use native C++
rendering. The current pin already includes these eight profiles. Supported grouping, members, cardinalities
and native style subsets follow the exact pinned DxUi guide; advanced control blocks, arbitrary CSS,
interaction directives and other unsupported constructs retain exact fenced source. F2 edits a diagram in
place. Try the [family gallery](examples/mermaid-supported-families.md) and
[display regressions](examples/mermaid-display-regressions.md); see the [editor contract](../Specs/Editor/Editor_Markdown.md).

The [sequence sample](examples/mermaid-sequence.md) covers the expanded actor, plain note, automatic
numbering and unheaded-message syntax. Numbering starts at one and excludes notes; authored source stays exact.

**Files and diagnostics:** save explicitly to write your Markdown file. Recovery checkpoints unsaved writing
after 750 ms of quiet. On restart, inspect the source preview, Restore a copy, Discard snapshot or Keep for
later; reopen pending snapshots from Settings. Restored copies require Save As and suggest a recovered filename.
Restore stays bound to the copy you selected while answering an unsaved-change question. If that snapshot
has since been discarded, the current document stays unchanged. You can then choose another available copy.
Unapplied local drafts are preserved as proposed edits in the previewable recovery copy. Cancelling a clean
draft retires that proposed snapshot. Another live app's snapshots are excluded. Corrupt/newer data is retained
as `.quarantine` under `RedTilde\Recovery`. A crash before the quiet interval can lose the newest keystrokes.
Undoing to the exact saved text clears dirty state; edits made during a save remain protected against the text
actually written.
Recoverable command, shortcut, menu or completion failures keep the document open and report a status. A failed
Close does not silently destroy the window. Menu actions that open a local source or Link draft keep typing
focus in that draft.
Files over 65,536 UTF-16 units and malformed UTF-8 are rejected with a specific explanation, never truncated.
When another app changes the file, choose Reload, Save As, Overwrite or Cancel in one question. Cancel keeps
editing. Save As writes a separate version; selecting the current file or its alias still checks the saved
baseline. Reload keeps the other open documents and applies only after a successful read. New edits made while
the question, picker or Reload is pending are retained and require a fresh choice. Touching only the timestamp
of an otherwise identical file does not require a conflict question.
Crash reports remain local, with a notification on the next launch. A dump may contain document text; sharing
one is your choice. Settings use `RedTilde.settings` in LocalAppData; System/Light/Dark/Paper and outline changes
apply immediately. High contrast takes precedence.

See [example.md](examples/example.md), [technical notes](examples/technical-notes.md), [the design system](../Specs/UI/UI_DesignSystem.md),
[actual captures](../Measurements/README.md) and [current capability status](../capabilities.json).
The [sample document guide](examples/README.md) also links complex math, flowchart,
state/class and intentional-fallback documents, with an edit/undo and presentation checklist.

[RedMonitor diagnostics](Diagnostics.md) explains the optional viewer and application logging.
