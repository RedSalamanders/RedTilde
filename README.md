# RedTilde

**Words, beautifully in flow.** A native Windows Markdown editor in the RedSalamander, RedXe, and RedPrism family.

Source repository: [RedSalamanders/RedTilde](https://github.com/RedSalamanders/RedTilde).

<img src="res/RedTilde.png" alt="RedTilde red tilde icon" width="128" />

RedTilde keeps a single authoritative UTF-8 Markdown source and undo history while you switch between **Compose**, **Source**, and **Split** views. Select text for contextual tools, edit blocks or equations in place (`F2`), browse folders and open documents, preview and print formatted pages (`Ctrl+P`), insert local or pasted images, draw eight bounded native Mermaid diagram families (flowchart, state, class, sequence, pie, ER, Gantt, and mind map), and render inline or display LaTeX math natively. All editing stays local.

> **Start here:** Read the [**User Guide**](docs/README.md), [**Portable & Installer Guide**](docs/Portable.md), [**Screenshot Gallery**](docs/screenshots/README.md), and [**Per-User Installer**](Installer/Install-RedTilde.ps1).

![RedTilde Compose view in Light theme](docs/screenshots/compose-light.png)

## Status

RedTilde is an **experimental 0.1 preview** built against an exact DxUi API-5 pin (`0c371a6197eb8c6b876b4b2843d75078bc21035c` in [Dependencies/DxUi.lock.json](Dependencies/DxUi.lock.json)). Documents are currently bounded to `65,536` UTF-16 units and `128` undo steps. Upstream performance, physical IME/AT/input qualification, real cloud/SMB/WSL location qualification, and v1.0 release handoff remain open gates tracked in [capabilities.json](capabilities.json) and the [active plan index](Specs/Plans/WIP/README.md).

Expected refusals retain your edits; unexpected C++ failures record local minidumps and `.record` sidecars under `%LOCALAPPDATA%\RedTilde\Crashes` without uploading anything (see [Diagnostics](docs/Diagnostics.md)).

## Features & Documentation

- **User Guide & Shortcuts:** [docs/README.md](docs/README.md) covers Compose/Source/Split/Focus views, contextual selection tools, in-place `F2` editing, Find & Replace (`Ctrl+F`), Print Preview (`Ctrl+P`), Themes (`System`, `Light`, `Dark`, `Paper`, High Contrast), crash recovery, and the full shortcut table.
- **Portable & Per-User Install:** [docs/Portable.md](docs/Portable.md) covers running from a portable ZIP or installing per user with [Installer/Install-RedTilde.ps1](Installer/Install-RedTilde.ps1) (including optional `.md` / `.markdown` HKCU file associations).
- **Sample Documents:** [docs/examples/README.md](docs/examples/README.md) provides starter notes, the [Markdown showcase](docs/examples/markdown-showcase.md), [local images](docs/examples/local-images.md), [syntax options](docs/examples/syntax-options.md), [Mermaid family gallery](docs/examples/mermaid-supported-families.md), [complex math](docs/examples/complex-math.md), and [intentional fallback cases](docs/examples/rendering-fallback-cases.md).
- **Hidden Harness Screenshots:** [docs/screenshots/README.md](docs/screenshots/README.md) documents all 16 deterministic hidden-window captures.

## Build and Test

Requires Windows 11 (`10.0.22000.2600`+), PowerShell 7, Git, Visual Studio 2026 (`v145` / MSVC `14.52.x`), and Windows SDK `10.0.26100`. Build scripts discover Visual Studio and restore pinned DxUi/WIL into this repository's private `.build` tree. Native launchers also require a clean `.ci-engine` checkout at the exact `.ci/project.json` engine commit (`https://github.com/RedSalamanders/.github.git`).

```powershell
.\build.ps1 -Platform x64 -Configuration Debug
.\.build\x64\Debug\RedTilde.exe
.\.build\x64\Debug\RedTilde.exe C:\Notes\idea.md
.\Test-Changes.ps1 -Explain
.\Test-Changes.ps1
```

- **Configurations & Platforms:** `build.ps1` supports `-Platform x64|ARM64` and `-Configuration Debug|Release|'ASan Debug'`, plus `-ProductionOnly` (Release) and `package.ps1` for production portable ZIP packaging. ARM64 cross-builds compile locally; ARM64 runtime tests require an ARM64 Windows host.
- **Native Test Catalog:** `174` behavioral cases (`Storage` `32`, `Settings` `11`, `Recovery` `22`, `Editor` `109`), plus Preferences, RecoveryRestart, Performance, Captures, Crash, Sanitizer, and Tooling suites. See [Tests/README.md](Tests/README.md) and [Specs/Testing/Testing_Validation.md](Specs/Testing/Testing_Validation.md).
- **Validation & Formatting:** `.\validate.ps1` checks specifications, links, skill packages, resource strings, build policies, and PowerShell tests; `.\format.ps1 -Check` checks C++ formatting.

## Specifications, Measurements & Contributing

- [Specifications Index](Specs/README.md) · [Editor Contract](Specs/Editor/Editor_Markdown.md) · [Design System](Specs/UI/UI_DesignSystem.md) · [Development Guidance](AGENTS.md)
- [Measurements Index](Measurements/README.md) · [Performance Contract](Specs/Testing/Testing_Performance.md) · [Unified CI Contract](Specs/Build/Build_UnifiedCI.md)
- [Changes Workflow](Changes/README.md): each code or docs change adds one dated fragment under `Changes/` without hand-editing `CHANGELOG.md`.
