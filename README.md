# RedTilde

**Words, beautifully in flow.** A native Windows Markdown editor in the RedSalamander, RedXe and RedPrism family.

Source repository: [RedSalamanders/RedTilde](https://github.com/RedSalamanders/RedTilde).

<img src="res/RedTilde.png" alt="RedTilde red tilde icon" width="128" />

RedTilde keeps a single Markdown source and undo history while you switch between **Compose**, **Source** and
**Split**. Select text for contextual tools, navigate the outline, edit a block's source, insert local images,
or draw supported Mermaid flowchart, state, class, sequence, pie, ER, Gantt and mind-map diagrams. Inline/display equations render natively in the document. Your files stay local.

This is an **experimental 0.1 application**, built against an exact DxUi API-4 commit with native rich text,
math/diagrams and shared presentation APIs. [Rich-text PR #89](https://github.com/RedSalamanders/DxUi/pull/89)
merged on 2026-10-21; upstream performance and final product qualification remain separate open gates.
Full language compatibility and large files also remain open. Current limits and gates are in
[capabilities.json](capabilities.json) and the [active plan index](Specs/Plans/WIP/README.md).

## Build and run

Requires Windows, PowerShell 7, Git, Visual Studio 2026/v145 C++ and Windows SDK 10.0.26100. The scripts discover
Visual Studio and restore pinned DxUi/WIL into this repository's private `.build` tree. Other Red checkouts are
not build inputs.

```powershell
.\build.ps1 -Configuration Debug
.\.build\x64\Debug\RedTilde.exe
.\.build\x64\Debug\RedTilde.exe C:\Notes\idea.md
.\Test-Changes.ps1 -Explain
.\Test-Changes.ps1
```

`build.ps1` also accepts `-Platform ARM64` and `-Configuration Release` or `'ASan Debug'`.
ARM64 runtime tests require a native ARM64 computer. Build/test products are never committed.

Everyday testing selects affected scopes from the last successful default run; unchanged input selects no tests.
Use `Test-Changes.ps1 -Mode PrePush` to account for local and matching PR obligations without duplicating
identical CI tests, or `-Mode Full` for an explicit full profile. Pending CI is not a pass. See the
[test guide](Tests/README.md) for explicit scope selection and exact successful-result reuse.

## Writing

| Action | Shortcut |
| --- | --- |
| New / Open / Save / Save As | Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S |
| Open Folder | Ctrl+Shift+O |
| Compose / Source / Split | Ctrl+1 / Ctrl+2 / Ctrl+3 |
| Bold / Italic / Undo / Redo | Ctrl+B / Ctrl+I / Ctrl+Z / Ctrl+Y |
| Local block source | F2 |
| Toggle Files listing | Ctrl+Shift+E |
| Apply ordinary source draft / cancel | Ctrl+Enter / Esc |
| Find / settings / focus mode | Ctrl+F / Ctrl+, / F11 |
| Focus selected-text tools | Alt+F10 |

Native code, diagram and equation source regions edit in place. Other selected text or the current line opens a local source
draft; Apply commits one undoable edit and Esc cancels. Split's source mirror is read-only and follows selection.
Right-click or use the context-menu key for headings, inline styles, lists, insertion and table-row/column actions.
Theme and outline preferences apply to the running window and persist under `%LOCALAPPDATA%\RedTilde`.
Settings › Markdown syntax turns highlight, superscript, subscript, inserted text, wiki links, definition lists and
abbreviations on or off individually; each is on by default and explains its syntax.

**File → Open Folder...** chooses a persistent root for the Files pane. The first opened file establishes its
parent when no root exists. Opening other files, Save As, and New keep that root; restarting restores it and the
pane's saved visibility. The optional pane lists `.md` and `.markdown` files throughout its bounded folder tree.
Single-click or Enter opens through the existing Open and unsaved-changes flow; it does not manage files. See the
[design contract](Specs/UI/UI_DesignSystem.md). The [design-refresh record](Specs/Plans/Done/DesignRefresh_2026-10-22.md)
keeps the proposal art; its remaining gate is R10, sequenced from the [plan index](Specs/Plans/WIP/README.md).

Open/save preserves UTF-8 BOM and existing line endings. Invalid UTF-8/NUL and documents above 65,536 UTF-16
units are rejected without replacing the current source. Saves are atomic and check for external modifications;
New/Open/Close protect unsaved edits. Crash reports stay under `RedTilde\Crashes` and are never uploaded.
Debounced session snapshots offer review, restore or discard after a crash. Restoring creates a dirty copy requiring Save As; original files are protected.

## Media and previews

Fenced C-like, JSON, PowerShell and diff code uses DxUi's bounded native syntax colors. Unknown languages retain
ordinary code ink. The [Markdown showcase](docs/examples/markdown-showcase.md) includes code, tables, Unicode,
lists, math and diagrams; the [folder notebook](docs/examples/workspace/README.md) exercises nested navigation.

Local PNG/JPEG/BMP/GIF/TIFF images resolve relative to the document file and decode on a worker. Missing or
unsupported images retain their alternative text. Network paths and remote resources are declined by default. Settings can explicitly enable bounded anonymous
HTTP(S) image loading; switching it off cancels remote work and removes remote pixels. Links remain inert.
Mermaid supports the pin's bounded flowchart, state, class, sequence, pie, ER, Gantt and mind-map profiles;
unsupported syntax stays editable fenced source. The [family gallery](docs/examples/mermaid-supported-families.md)
shows one small example of each profile; these native profiles do not imply full Mermaid compatibility.
Inline/display mathematics supports fractions, indexed roots, simultaneous scripts, symbols, text, alphabets,
accents, paired delimiters, matrices, cases and aligned/gathered rows, preserving `$...$`/`$$...$$` and slash delimiters.
Display `$$` or `\[` / `\]` delimiters stand on their own lines; a one-line `$$x$$` stays literal.
Wiki-link embeds and data images are declined without file or network access.
Try the [complex sample documents](docs/examples/README.md) for longer equations,
larger diagrams and clearly labeled fallback cases.
Unsupported notation stays editable. Full LaTeX compatibility remains a separate goal. Markdown HTML/scripts and links are never executed.

Read [the user guide](docs/README.md), [specifications](Specs/README.md), [development guidance](AGENTS.md),
[validation](Specs/Testing/Testing_Validation.md) and [measurement index](Measurements/README.md).

The [math/diagram/recovery validation packet](Measurements/MathMermaidRecovery_2026-10-20/README.md) records
that feature slice's tested pin, screenshots, portable smoke test and remaining qualification gates. The
[S5 performance contract](Specs/Testing/Testing_Performance.md) defines completed native-frame measurements
and paired controls for the current continuation; the active plan retains its unverified gates.
The [completed local S5 assessment](Measurements/S5_2026-10-21/Final/README.md) records the current pin's six
builds, 25 cases per x64 profile, 192 paired processes, default/WARP lifecycle checks and independent audits.
No directional performance change was resolved; noisy controls, competing activity and an unaccepted judge
leave performance qualification open. The application remains experimental.
The [S5 noise investigation](Measurements/S5_Noise_2026-10-21/README.md) follows those unresolved controls with
cheap settled-memory observations, command CPU/residency diagnostics and explicit host activity recording.
Both new attempts are retained. The replacement has complete observations but reaches 100% CPU under competing
builds; noisy controls and quiet-host acceptance remain unresolved despite passing local correctness checks.
The [later user-requested follow-up](Measurements/S5_Followup_2026-10-21/README.md) adds 192 independently audited
processes. Its measured CPU averages 35.3%/30.2%; noisy controls keep the reported command improvements and
performance acceptance provisional. Hardware and upstream gates remain open.
The [remaining-control investigation](Measurements/S5_Controls_2026-10-21/README.md) removes redundant native
caption writes and adds bounded phase/idle diagnostics. Six builds, 81 x64 cases and 192 newly audited processes
pass local checks. Noisy controls and a retained small positive Compose frame signal keep performance acceptance
open; the experimental and hardware/upstream status is unchanged.

## Recording changes

Follow the [Changes workflow](Changes/README.md): each change adds one fragment under `Changes/` and never edits `CHANGELOG.md`; after merges, `Tools/Submit-ChangelogFold.ps1` folds the fragments on `main` through a pull request of its own.

Builds require the default MSVC **14.52.x** tools (v145). Release uses /Gy /GL and final /OPT:REF /LTCG; archive and final-link tools must match exactly. The build contract documents the preserved API pin, explicit archive forwarding and policy-specific output identity.

Native builds use the [shared build policy](Specs/Build/Build_ConsoleAndPaths.md):
/Wall without /WX, zero-warning validation, Debug /ZI and ASan /Zi, and
Release-only /OPT:ICF. A passing build prints a green result with artifact/log
links and a quoted launch command. OSC 8 capable terminals can open the local
file links. Cross-built ARM64 applications still need ARM64 Windows to run.
CI behavior and manual actions: [Unified CI contract](Specs/Build/Build_UnifiedCI.md).
