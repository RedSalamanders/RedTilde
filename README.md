# RedTilde

**Words, beautifully in flow.** A native Windows Markdown editor in the RedSalamander, RedXe and RedPrism family.

Source repository: [RedSalamanders/RedTilde](https://github.com/RedSalamanders/RedTilde).

<img src="res/RedTilde.png" alt="RedTilde red tilde icon" width="128" />

RedTilde keeps a single Markdown source and undo history while you switch between **Compose**, **Source** and
**Split**. Select text for contextual tools, navigate the outline, edit a block's source, insert local images,
or draw supported Mermaid flowchart, state and class diagrams. Inline/display equations render natively in the document. Your files stay local.

This is an **experimental 0.1 application**, built against an exact commit of
[DxUi's rich-text PR #89](https://github.com/RedSalamanders/DxUi/pull/89) plus its tested native math/diagram
feature revision. Full language compatibility, large files and final product qualification remain open. Current limits and gates are in
[capabilities.json](capabilities.json) and the [active plan](Specs/Plans/WIP/RedTilde_2026-10-20.md).

## Build and run

Requires Windows, PowerShell 7, Git, Visual Studio 2026/v145 C++ and Windows SDK 10.0.26100. The scripts discover
Visual Studio and restore pinned DxUi/WIL into this repository's private `.build` tree. Other Red checkouts are
not build inputs.

```powershell
.\build.ps1 -Configuration Debug
.\.build\x64\Debug\RedTilde.exe
.\.build\x64\Debug\RedTilde.exe C:\Notes\idea.md
.\test.ps1 -Configuration Debug -SkipBuild
.\Test-Changes.ps1 -Explain
```

`build.ps1` also accepts `-Platform ARM64` and `-Configuration Release` or `'ASan Debug'`.
ARM64 runtime tests require a native ARM64 computer. Build/test products are never committed.

## Writing

| Action | Shortcut |
| --- | --- |
| New / Open / Save / Save As | Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S |
| Compose / Source / Split | Ctrl+1 / Ctrl+2 / Ctrl+3 |
| Bold / Italic / Undo / Redo | Ctrl+B / Ctrl+I / Ctrl+Z / Ctrl+Y |
| Local block source | F2 |
| Apply ordinary source draft / cancel | Ctrl+Enter / Esc |
| Find / settings / focus mode | Ctrl+F / Ctrl+, / F11 |
| Focus selected-text tools | Alt+F10 |

Native code/diagram source regions edit in place. Other selected text or the current line opens a local source
draft; Apply commits one undoable edit and Esc cancels. Split's source mirror is read-only and follows selection.
Right-click or use the context-menu key for headings, inline styles, lists, insertion and table-row/column actions.
Theme and outline preferences apply to the running window and persist under `%LOCALAPPDATA%\RedTilde`.

Open/save preserves UTF-8 BOM and existing line endings. Invalid UTF-8/NUL and documents above 65,536 UTF-16
units are rejected without replacing the current source. Saves are atomic and check for external modifications;
New/Open/Close protect unsaved edits. Crash reports stay under `RedTilde\Crashes` and are never uploaded.
Debounced session snapshots offer review, restore or discard after a crash. Restoring creates a dirty copy requiring Save As; original files are protected.

## Media and previews

Local PNG/JPEG/BMP/GIF/TIFF images resolve relative to the document file and decode on a worker. Missing or
unsupported images retain their alternative text. Network paths and remote resources are declined by default.
Mermaid supports the pinned native flowchart, state and class subsets; unsupported syntax stays editable fenced source.
Inline/display mathematics supports fractions, indexed roots, simultaneous scripts, symbols, text, alphabets,
accents, paired delimiters, matrices, cases and aligned/gathered rows, preserving `$...$`/`$$...$$` and slash delimiters.
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
