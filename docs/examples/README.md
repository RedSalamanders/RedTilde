# Documents to try in RedTilde

Open these files in RedTilde and switch between Compose, Source and Split. They
are editable sample documents, not a claim of full LaTeX, KaTeX or Mermaid
compatibility. All content is invented and needs no network resources.

| Document | What to inspect |
| --- | --- |
| [Starter](example.md) | Ordinary writing and mixed Markdown |
| [Markdown showcase](markdown-showcase.md) | Typography, Unicode, nested/task lists, aligned tables, colored code, equations and a flowchart |
| [Syntax options](syntax-options.md) | Every independently configurable RichMarkdown extension |
| [Native Mermaid family gallery](mermaid-supported-families.md) | Small positive examples of all eight bounded native profiles |
| [Sequence actors, notes and numbers](mermaid-sequence.md) | Actor figures, chronological notes, automatic message numbers and plain arrows |
| [Shared display regressions](mermaid-display-regressions.md) | Styled labels, ER/class fans, task markers and readable code chrome |
| [Layout and mathematical spacing](layout-and-math-regressions.md) | The BT preparation pipeline, LR inheritance, close accents and gathered vectors |
| [Tasks and text insets](tasks-and-insets.md) | Theme accent, optional completed-task strike-through, table cells and inline-code spacing |
| [YAML metadata](front-matter-yaml.md) | A valid document-start YAML front-matter envelope |
| [TOML metadata](front-matter-toml.md) | A valid document-start TOML front-matter envelope |
| [Folder notebook](workspace/README.md) | A realistic nested tree of `.md` and `.markdown` documents for Open Folder and restart |
| [Technical notes](technical-notes.md) | Small equations, state and class diagrams |
| [Complex mathematics](complex-math.md) | Nested fractions, roots, scripts, operators, matrices, cases and aligned systems |
| [Complex flowcharts](complex-flowcharts.md) | Branches, joins, cycles, self/parallel edges, grouped endpoints and four directions |
| [Complex state and class diagrams](complex-state-and-class-diagrams.md) | Longer flat state machines, descriptions, members, associations and inheritance |
| [Intentional fallback cases](rendering-fallback-cases.md) | Unsupported and malformed notation that must remain exact editable source |

The three complex documents target the current bounded native profiles. The
fallback document deliberately contains examples outside those profiles; a
literal result there is expected. Large equations and graphs can require more
room or fall back at a narrow viewport; parser acceptance alone does not prove
readable layout.

## Manual check sequence

1. Open a supported sample in Compose. Inspect baselines, brackets, matrix cells,
   labels, arrowheads and edge-label backgrounds. Scroll through the entire file.
2. Repeat in Split and Source. Source must preserve every delimiter, backslash,
   label and line break. The source mirror remains read-only.
3. Use F2 on an equation or diagram. Change one coefficient, label or edge. Apply,
   undo and redo; then try another edit and cancel. Check that unrelated source
   and selection stay intact.
4. Resize the window and try light, dark and high-contrast presentation. Where
   available, repeat at 96, 144 and 192 DPI. Look for clipping and collisions.
5. Open the fallback sample. Its unsupported formulas and complete Mermaid fences
   must remain available for source editing; neighboring supported content should
   still work. No directive, package, file access or link should execute.

These are manual review steps, not recorded pass results. Use the governed
hidden harness for screenshots and reproducible application validation.

Choose **File → Open Folder...** (Ctrl+Shift+O) and select this examples folder to
keep all samples in the Files pane. Opening a sample, saving elsewhere, or using
New keeps the chosen root. Restart restores that folder and the pane's visibility.
**Open File...** (Ctrl+O) remains available; the first file establishes its parent
as the root when no folder has been chosen yet.

The existing `s5-performance.md` and `performance-fixture.md` are measurement inputs.
The reference-dark capture input lives in `Tests/Fixtures/reference-dark.md`.
This sample expansion does not change or replace them.
