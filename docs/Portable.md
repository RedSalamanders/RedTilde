# RedTilde 1.0 — Portable & Per-User Guide

Extract the portable ZIP (and verify its companion `.zip.sha256` sidecar; `v1.0.0` packages are unsigned without a public CA Authenticode certificate) into any writable folder and run `RedTilde.exe`, or pass a Markdown filename on the command line. RedTilde requires Windows 11 (build `22000.2600` or later) and the matching Microsoft Visual C++ runtime. All editing stays local and requires no account.

## Portable ZIP & Per-User Installer

You can run `RedTilde.exe` directly from the extracted portable folder, or install it per user without administrator privileges using [`Install-RedTilde.ps1`](../Installer/Install-RedTilde.ps1):

```powershell
# Install to %LOCALAPPDATA%\Programs\RedTilde and create a Start Menu shortcut
pwsh -NoProfile -File .\Installer\Install-RedTilde.ps1 -SourceDirectory <extracted-portable-folder>

# Also register per-user .md and .markdown OpenWithProgids under HKCU\Software\Classes
pwsh -NoProfile -File .\Installer\Install-RedTilde.ps1 -SourceDirectory <extracted-portable-folder> -RegisterFileAssociation

# Uninstall program files, Start Menu shortcut, and HKCU associations (preserves %LOCALAPPDATA%\RedTilde)
pwsh -NoProfile -File .\Installer\Install-RedTilde.ps1 -Uninstall
```

Pass `-InstallDirectory <path>` to target a different per-user folder (it must not be a drive root, traverse a reparse point, or overlap `%LOCALAPPDATA%\RedTilde`). Uninstalling removes the installed application files, Start Menu shortcut, Uninstall registry entry, and HKCU file associations while keeping your user settings, recovery snapshots, and crash reports intact.

## Production-Only Portable Role

Portable ZIPs packaged with `package.ps1` use the **production-only Release** role (`!defined(REDTILDE_ENABLE_TESTS)`). Self-test, interactive-test, deliberate crash, recovery, preferences, failure-host, and sanitizer probe switches return exit code `2` without opening the application or creating profile data. Ordinary launching, `--document`, `--version` (`-v`), `--help` (`-h`, `/?`), `--headless`, `--capture`, and performance diagnostics remain available.

## Supported Features

- **Views & Focus:** Switch freely between **Compose** (`Ctrl+1`), **Source** (`Ctrl+2`), and **Split** (`Ctrl+3`) with one shared document and undo history, or enter **Focus mode** (`F11` / `Esc`). Split shows editable Compose beside a read-only Source mirror.
- **Contextual & Local-Source Editing:** Select text in Compose or Split for the floating selection toolbar (`Alt+F10`), or press `F2` to edit code blocks, Mermaid diagrams, metadata, and equations in place (or open a local source draft for prose with `Ctrl+Enter` to apply and `Esc` to cancel).
- **Files, Open Documents & Drag-and-Drop:** Browse `.md` and `.markdown` folders in the **Files** pane (`Ctrl+Shift+E`, **Open Folder** `Ctrl+Shift+O`), switch between up to `64` open documents from **Open documents** without prompting, close the active document with `Ctrl+W`, or drag and drop a `.md`, `.markdown`, or `.txt` file onto the window.
- **Find & Replace:** `Ctrl+F` searches exact Markdown source with optional **Match case**, section match badges in the Outline, **Replace** / **Replace all**, and `F3` / `Shift+F3` navigation even after closing the bar.
- **Printing & Print Preview:** `Ctrl+P` / **File → Print…** previews formatted Letter/A4 pages, supports installed Windows printers and **Microsoft Print to PDF**, page ranges, orientation, duplex, copies, and **Fit width** / **Fit page** navigation.
- **Themes, Zoom & Syntax Options:** Settings (`Ctrl+,`) provides **System**, **Light**, **Dark**, and **Paper** themes (automatically overridden by Windows High Contrast), reading width (`480–1040` DIPs), text scale (`75%–200%`, `Ctrl++`/`-`/`0` or `Ctrl+Wheel`), completed-task strikethrough, and seven optional RichMarkdown syntax toggles.
- **Mathematics & Mermaid Diagrams:** Inline (`$...$`, `\(...\)`) and own-line display (`$$`, `\[` / `\]`, `math`/`latex` fences) equations render natively with a live **Equation** inspector in the sidebar. Eight bounded static Mermaid families are rendered natively in C++: **flowchart**, **state**, **class**, **sequence**, **pie**, **ER**, **Gantt**, and **mind map**. Unsupported syntax stays editable source.
- **Images & Clipboard Paste:** Local `PNG`, `JPEG`, `BMP`, `GIF` (first frame), and `TIFF` files decode off the UI thread and refresh on disk changes; pasted bitmaps save as local PNGs under `%LOCALAPPDATA%\RedTilde\Assets` (text wins over images by default unless **Prefer images when pasting** is enabled).

## User Data, Privacy & Recovery

- **Data Location:** Preferences (`RedTilde.settings.json`, or `RedTilde-debug.settings.json` in debug builds), recovery snapshots (`Recovery/`), pasted clipboard assets (`Assets/`), and local crash reports (`Crashes/`) live under `%LOCALAPPDATA%\RedTilde`. Nothing is uploaded automatically.
- **Encoding & Safe Saves:** Accepts valid UTF-8 Markdown up to `65,536` UTF-16 units, preserving UTF-8 BOM and `LF`/`CRLF`/`CR`/mixed line endings. Rejects invalid UTF-8, NUL bytes, or oversized files without truncation, saves atomically with external-change conflict prompts, and preserves hardlink aliases via independent recovery protection.
  An overwrite confirmation applies to the observed file version. A later external edit opens a fresh conflict question.
- **Auto-Recovery:** Checkpoints unsaved edits after `750 ms` of quiet (at most `5 s` under continuous typing) and offers a source preview on restart (`Restore a copy`, `Discard snapshot`, `Keep for later`, `Restore all copies`). Restored copies require **Save as…**.
- **Privacy Defaults:** Remote `http(s)://` images, local-network/UNC images, `data:` URIs, and `![[wiki]]` embeds are blocked by default. Markdown HTML, scripts, and Mermaid interaction directives never execute. External `http:`, `https:`, and `mailto:` links open only on explicit `Ctrl+Click` or **Open link** confirmation.

## Troubleshooting & Further Reading

- [User Guide](README.md) — full feature walkthrough, screenshots, and keyboard shortcut reference
- [Diagnostics & RedMonitor](Diagnostics.md) — `--version`, local `Crashes/` minidumps, and optional RedMonitor logging
- [Sample Documents](examples/README.md) — showcase Markdown, math, all eight Mermaid profiles, and fallback examples
- [Capabilities](../capabilities.json) — current `production` capability boundaries and explicit `deferred` post-1.0 items
- License notices live in `LICENSE.txt`, `THIRD-PARTY-NOTICES.md`, and `Dependencies`.
