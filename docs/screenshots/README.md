# RedTilde Documentation Screenshots

All screenshots in this directory are deterministic renders produced by RedTilde's own hidden, non-activating application harness (`WS_EX_NOACTIVATE`, `!IsWindowVisible`) using synthetic showcase content. No desktop screenshots, foreground activation, or manual cropping were used. Sidecar `.scenario.json` provenance files and harness logs are kept out of `docs/screenshots/`.

## Provenance

- **Application build:** `RedTilde.exe` (`x64` `Release`, test-enabled harness build)
- **Pinned DxUi dependency:** `0c371a6197eb8c6b876b4b2843d75078bc21035c` (API revision `5`, from [`Dependencies/DxUi.lock.json`](../../Dependencies/DxUi.lock.json))
- **Capture command pattern:**

```powershell
.\.build\x64\Release\RedTilde.exe --capture <out.png> --scenario <scenario> --capture-dpi 96 --data-dir <unique-temp-dir>
```

- **Viewport:** `1440×900` DIPs at `96` DPI (`1440×900` px) for standard main-window scenarios; `1080×820` DIPs (`1080×820` px) for `panes-collapsed-rails-light` and `pane-hover-reveal-dark`; `1040×840` DIPs window (`1018×784` client px at `96` DPI) for `print-preview`.

## Scenario Inventory

| File | Scenario | Dimensions (px) | What it shows |
| --- | --- | --- | --- |
| [`compose-light.png`](compose-light.png) | `compose-light` | `1440×900` | Compose view in Light theme with Files pane, Outline, and Current Block |
| [`compose-dark.png`](compose-dark.png) | `compose-dark` | `1440×900` | Compose view in Dark theme with Files pane and Outline |
| [`compose-paper.png`](compose-paper.png) | `compose-paper` | `1440×900` | Compose view in warm Paper theme |
| [`panes-collapsed-rails-light.png`](panes-collapsed-rails-light.png) | `panes-collapsed-rails-light` | `1080×820` | Both side panes auto-collapsed into `28`-DIP edge rails with left modified-file dots, right heading dots, and hover-parity toggle chrome |
| [`pane-hover-reveal-dark.png`](pane-hover-reveal-dark.png) | `pane-hover-reveal-dark` | `1080×820` | Hovering the right rail trigger zone slides open the floating Outline overlay (`Surface::Kind::Floating`) over the Dark canvas with zero reflow |
| [`split.png`](split.png) | `split` | `1440×900` | Split view in Light theme with Compose `Editing`, `Read-only` Source mirror, center linked-scroll toggle (`Ctrl+Shift+Y`), and collapsed right Outline dot rail |
| [`split-dark.png`](split-dark.png) | `split-dark` | `1440×900` | Split view in Dark theme with active-pane indicator, center linked-scroll toggle, and collapsed right Outline dot rail |
| [`split-source-active-light.png`](split-source-active-light.png) | `split-source-active-light` | `1440×900` | Bidirectional Split view with right Source pane active (`Editing`), passive Compose preview (`Read-only`), and Find & Replace targeting Source |
| [`split-unlinked-dark.png`](split-unlinked-dark.png) | `split-unlinked-dark` | `1440×900` | Split view in Dark theme with Linked scrolling toggled Off (dashed seam) so Source scrolls independently while Find highlights active Compose and dot match rings update on the right rail |
| [`source.png`](source.png) | `source` | `1440×900` | Source view with syntax-colored Markdown, Files pane, and Outline |
| [`focus.png`](focus.png) | `focus` | `1440×900` | Focus mode with minimal top bar, dimmed inactive blocks, and word-count pill |
| [`context-selection.png`](context-selection.png) | `context-selection` | `1440×900` | Floating contextual selection toolbar (`Alt+F10` hint) above selected prose |
| [`local-source.png`](local-source.png) | `equation` | `1440×900` | In-place `F2` LaTeX source editing alongside the live read-only Equation inspector |
| [`math.png`](math.png) | `math` | `1440×900` | Rendered inline and display LaTeX math, matrices, aligned rows, fallback source, and Equation inspector |
| [`mermaid.png`](mermaid.png) | `mermaid` | `1440×900` | Native Mermaid flowchart, sequence diagram, and state diagram rendered on the document canvas |
| [`find.png`](find.png) | `find` | `1440×900` | Find & Replace overlay (`Ctrl+F`) with `Match case`, highlighted matches, and Outline section counts |
| [`print-preview.png`](print-preview.png) | `print-preview` | `1018×784` | Print Preview (`Ctrl+P`) with synthetic printer destinations, page setup controls, and Letter Portrait sheet |
| [`settings.png`](settings.png) | `settings` | `1440×900` | Settings drawer (`Ctrl+,`) showing theme preview tiles, reading width, text size, Writing, and Markdown syntax |
| [`image-consent-light.png`](image-consent-light.png) | `image-consent-light` | `1440×900` | Non-modal remote-image consent bar with blocked remote image alt text |
| [`recovery.png`](recovery.png) | `recovery` | `1440×900` | Startup crash-recovery sheet with Unicode source preview and Restore / Discard / Keep actions |
