# RedTilde 0.1 — experimental

Run `RedTilde.exe`, or pass a Markdown filename on the command line. This portable development build requires
Windows and the matching Microsoft Visual C++ 2026 runtime. It uses local files and requires no account.

Portable ZIPs use the production-only Release role. Self-test, interactive-test, deliberate crash, recovery,
preferences and sanitizer probe switches return 2 without opening the application or creating profile data.
Supported hidden capture and performance diagnostics remain available. Building a ZIP with `package.ps1`
selects that production role automatically; `-SkipBuild` requires its verified test-disabled artifact.

Compose, Source and Split share one document and undo history. Select text for formatting; use F2 for local
block source. For ordinary text, Apply/Ctrl+Enter commits the draft and Esc cancels. Split's source mirror is
read-only. The outline navigates headings. Settings apply System/Light/Dark/Paper immediately.

Ctrl+N/O/S: New/Open/Save. Ctrl+Shift+S: Save As. Ctrl+1/2/3: views. Ctrl+B/I: bold/italic. Ctrl+Z/Y: undo/redo.
Ctrl+F: Find. Ctrl+comma: Settings. F11: Focus mode. Alt+F10: selected-text tools.

The editor accepts valid UTF-8 Markdown up to 65,536 UTF-16 units, preserving BOM and existing line endings.
It rejects invalid/oversized files without truncation and checks for external modifications before saving.
Recovery checkpoints unsaved writing after 750 ms of quiet and offers a source preview on restart. Restore a
copy requires Save As; it protects the original file. Settings can reopen pending recovery. Preferences,
recovery and local crash reports live in LocalAppData/RedTilde.
Nothing is uploaded automatically.

Local PNG/JPEG/BMP/GIF/TIFF references decode in the background. Assets are referenced, not copied. Network
resources are declined by default. Mermaid supports native flowcharts and bounded state/class diagrams.
Inline/display equations, fractions, indexed roots, scripts, matrices and aligned rows render in the document;
F2 edits exact source. Unsupported notation stays editable. Full language compatibility remains pending. Links,
Markdown scripts and HTML are never executed. The examples directory contains a starter document.

The dependency is an exact experimental DxUi API-5 pin with explicit application failure ownership. Fresh consumer qualification, upstream performance and
product qualification remain separate gates. Read `capabilities.json` for the current limits and pending work.
This build is not a production release. License notices are in `LICENSE.txt` and `Dependencies`.
