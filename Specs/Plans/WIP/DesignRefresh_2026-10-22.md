# Design refresh — 2026-10-22

Status: PROPOSED; not implemented. Owner: RedTilde application UI (layout, commands, theme adapter, Files pane),
with DxUi owning shared controls and rich-text link rendering. No product behavior, capability status or pin
changes yet.

The owner judged the current window "not nice" and asked for a much better design, saved here for future
implementation. This plan is the implementation specification for that proposal and is complete without any
external link. When a slice is accepted and implemented, move its lasting rules into
[the design system](../../UI/UI_DesignSystem.md); until then that contract remains authoritative and this plan only
proposes changes to it.

## Proposal art

The folder [DesignRefresh_2026-10-22](DesignRefresh_2026-10-22/proposal.css) holds one static HTML page per board,
their shared stylesheet and a PNG rendered from each page. The pages work offline from a checkout: plain
HTML/CSS, Segoe UI Variable chrome, 6-DIP controls, 10-DIP overlays and the theme values below, with the shipped
icon loaded from [res/RedTilde.png](../../../res/RedTilde.png). They are proposal art, not application captures,
and must never be presented as captures ([C11](RedTilde_2026-10-20.md)); actual evidence comes from the
deterministic harness with scenario/build provenance.

| Board | Picture | Page |
| --- | --- | --- |
| Compose, light, Files pane open | [compose-light.png](DesignRefresh_2026-10-22/compose-light.png) | [compose-light.html](DesignRefresh_2026-10-22/compose-light.html) |
| Compose, dark, Files pane open | [compose-dark.png](DesignRefresh_2026-10-22/compose-dark.png) | [compose-dark.html](DesignRefresh_2026-10-22/compose-dark.html) |
| Split view | [split.png](DesignRefresh_2026-10-22/split.png) | [split.html](DesignRefresh_2026-10-22/split.html) |
| Selection toolbar and Find/Replace | [find-selection.png](DesignRefresh_2026-10-22/find-selection.png) | [find-selection.html](DesignRefresh_2026-10-22/find-selection.html) |
| Focus mode | [focus.png](DesignRefresh_2026-10-22/focus.png) | [focus.html](DesignRefresh_2026-10-22/focus.html) |
| Settings drawer | [settings.png](DesignRefresh_2026-10-22/settings.png) | [settings.html](DesignRefresh_2026-10-22/settings.html) |
| Equation editing (F2) | [equation.png](DesignRefresh_2026-10-22/equation.png) | [equation.html](DesignRefresh_2026-10-22/equation.html) |

![Compose, light, with the Files pane open](DesignRefresh_2026-10-22/compose-light.png)

The PNGs were rendered on 2026-10-22 by headless Microsoft Edge 155.0.4283.62 at a 1440×900 viewport and device
scale 1. After editing a page, re-render it from the repository root; the profile stays under ignored `.build`:

```powershell
$dir = (Resolve-Path Specs/Plans/WIP/DesignRefresh_2026-10-22).Path
$edge = "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
foreach ($name in 'compose-light','compose-dark','split','find-selection','focus','settings','equation') {
    & $edge --headless=new --disable-gpu --hide-scrollbars --no-first-run --force-device-scale-factor=1 `
        --window-size=1440,900 "--user-data-dir=$PWD\.build\proposal-render" "--screenshot=$dir\$name.png" `
        ('file:///' + ($dir -replace '\\', '/') + "/$name.html") | Out-Null
}
```

An earlier interactive draft was made on a private web canvas. It is not needed to read or implement this plan,
and these files supersede it wherever they differ.

## Checklist

- [ ] R0: owner review; resolve D1–D5 below.
- [ ] R1: merged document bar (identity, dirty state, view selector, File menu, Save, caption buttons).
- [ ] R2: one grouped command row; file commands leave it; Files toggle at its start.
- [ ] R3: writing surface without nested frames; 760-DIP measure and document type scale.
- [ ] R4: optional left Files pane for Markdown in the document's folder and subfolders.
- [ ] R5: right panel with real outline hierarchy, current section and contextual block/equation tools.
- [ ] R6: status bar content and placement of transient messages.
- [ ] R7: contextual tools: selection toolbar and docked Find/Replace.
- [ ] R8: Focus mode, Settings drawer and F2 equation editor presentation.
- [ ] R9: link presentation (DxUi rich text upstream, then pin adoption).
- [ ] R10: fold accepted rules into the design system contract, actual captures, tests, C5 acceptance; move to Done.

## Problems in the current build

Observed in the owner's 2026-10-22 screenshot of Compose (dark) with `weekly-report-2026-10-15.md`:

- Identity appears twice: the native title bar and the app document bar both show the file name and dirty state.
- Four chrome rows (title, document bar, command row, view row) sit above the document, about 200 DIP.
- New/Open/Save/Save as compete with formatting commands; every command has the same visual weight.
- Compose/Source/Split sit on their own row next to the product tagline.
- The document is a panel inside a darker frame inside the window; the text column is offset and boxed.
- The outline shows empty entries and centered labels, with no hierarchy or current-section indicator.
- The equation preview reserves an empty box when the caret is not in an equation.
- Every link is underlined, so link-dense reports read as noisy.
- Transient status ("Match selected in the document.") is centered far from what it describes.
- There is no way to see or switch between the other Markdown files next to the open document.

## Layout

All sizes in DIP at 100% scale; nothing overlaps or leaves the window at 100/150/200% DPI.

| Region | Size | Content |
| --- | --- | --- |
| Document bar | 56 tall | Brand icon (28), file name, "Edited" text chip, centered view selector, File menu, Save, caption buttons |
| Command row | 48 tall | Files toggle · Undo/Redo · Text style · Bold/Italic/Code/Link · Bullets/Task · Insert · (spacer) · Find, Focus, Panel, Settings |
| Files pane (optional) | 280 wide, resizable 200–400 | Folder name, filter, Markdown tree of the document's folder and subfolders, file count |
| Canvas | remaining | Page surface; 760 maximum measure, 48 side gutters, 56 top padding |
| Right panel | 240 wide | Outline, Current block, contextual Equation inspector |
| Status bar | 28 tall | Left: words, characters, reading time. Right: selection, Ln/Col, encoding, line endings |

- Document bar: the brand icon is the shipped red tilde. The dirty state is text ("Edited"), never color alone.
  The view selector stays visible and mutually exclusive with UIA names per mode and keeps Ctrl+1/2/3. File (New,
  Open, Save as…, with the existing Ctrl+N/Ctrl+O/Ctrl+Shift+S) is a menu button next to the primary Save (Ctrl+S,
  40 tall). The native window title text still follows the file name and dirty marker for the taskbar and Alt+Tab
  (see D1).
- Command row: groups are separated by 20-DIP dividers. Icon buttons are 36×36 (compact minimum 32) and carry
  tooltip names with shortcuts. Ctrl+E (inline code) and Ctrl+K (link) shown in the pages are proposed new
  shortcuts; Ctrl+B/Ctrl+I/Ctrl+F/Ctrl+Comma/F2/F11/Alt+F10 exist today. Find is a labeled button showing "Ctrl+F".
  Toggle buttons (Files, Panel, Settings, active inline format) show a pressed state with the selection tint and
  expose it through UIA. Redo is disabled when unavailable. In an equation, inline formatting is disabled and
  Text style reads "Equation".
- Canvas: one page surface, no nested card or frame. Narrow windows first collapse the right panel (contract
  order), then show an opened Files pane as an overlay above the canvas instead of beside it, and only then reduce
  the measure. At the 800×560 minimum, command groups move into an overflow menu instead of wrapping or clipping.
- Right panel: section labels are 11-DIP semibold uppercase with 0.08em tracking. Outline rows are left-aligned,
  at least 36 tall, indented by level, and the current section has the selection tint plus a semibold label and
  `aria-current`-equivalent UIA state. Empty headings never produce blank rows. "Current block" names the block
  type and count and hosts "Edit block source". The Equation section appears only while the caret is in an
  equation and shows the read-only rendering with a "Read-only" chip.
- Status bar: transient feedback moves next to its source (find count in the Find bar, errors in the document
  footer per the contract); the status bar keeps persistent facts only.

## Files pane

Requested by the owner on 2026-10-22: an optional left pane that can be opened and closed, listing the Markdown
files in the current document's folder and its subfolders.

- Toggle: the first command-row button and Ctrl+Shift+E (unused today). Opening moves keyboard focus into the tree;
  Esc or closing returns focus to the editor with the source selection intact. Open/closed state and width persist
  in settings and apply immediately; the default is closed.
- Root: the folder that contains the current document, shown by name with the full path as its tooltip and UIA
  help text. An untitled document shows "Save the document to see the Markdown files in its folder." When Save As
  or Open changes the document's folder, the tree re-roots.
- Content: `*.md` and `*.markdown`, the same Markdown types the Open dialog accepts. Folders appear only when they
  contain Markdown at some depth; folders come first, then files, each in Explorer-style natural order. The
  current document's ancestors are expanded and other folders start collapsed; a collapsed folder shows its
  Markdown file count. Hidden and system entries are excluded, and reparse points (junctions, symbolic links) are
  listed but never traversed.
- Current file: selection tint, semibold name and an "Edited" dot while dirty; UIA exposes the state as text.
- Filter: a name filter above the tree keeps matching files and their ancestor folders; Esc clears it first.
- Opening: click or Enter opens the file in this window through the normal Open path, which prompts before
  discarding unsaved changes and keeps every file-safety rule. Arrow keys move, Left/Right collapse/expand,
  Home/End and type-ahead work; the tree exposes UIA Tree/TreeItem with ExpandCollapse and SelectionItem.
- Work: enumeration runs on an application worker, never the UI thread, with bounded results (depth 8 and 5,000
  entries by default; the footer then says "Showing the first 5,000 Markdown files"). Publish on the UI owner,
  cancel by request/root identity and reject stale completions. Refresh is event-driven from directory change
  notifications, coalesced; no polling timer. Access errors and offline paths show inline in the pane and never
  block editing.

## Theme and typography

Colors come from the RedTilde theme adapter ([Application.cpp](../../../RedTilde/Application.cpp)) over DxUi palette
defaults; high contrast overrides all of them. Chrome = `windowBackground`, Files pane and right panel =
`surfaceBackground`, page = `cardBackground`.

| Role | Light | Paper | Dark |
| --- | --- | --- | --- |
| Chrome / panes / page | `#F0F1F4` / `#F8F8FA` / `#FFFFFF` | `#EDE6D8` / `#F3EDDF` / `#FFFAEF` | `#17191E` / `#1C1F25` / `#22252B` |
| Dividers / control borders | `#E0E0E0` / `#CDCDCD` | same | `#3D3D45` / `#4A4A52` |
| Text / subdued text | `#141414` / `#666666` | same | `#EBEDF2` / `#B3B5BA` |
| Accent, focus stroke, link | `#C83C46` | `#C83C46` | `#FF7778` |
| Accent hover / pressed | `#B6303C` / `#A12A34` | same | `#FF999A` / `#FFABAC` |
| Filled primary button | `#C83C46`, white text | same | `#C83C46`, white text (`#FF7778` fails 4.5:1 with white) |
| Selection, current row, pressed toggle | `#F2CFD3`, label `#A12A34` | same | `#A63B46`, white label |
| Find: other matches | neutral subtle fill, distinct in lightness from selection | same | same |

- DxUi values (text, subdued text, dividers, borders) were read from DxUi `b69f452f`; re-verify them at the pinned
  commit. Headings and strong text use the document text ink, not a lighter gray.
- Document scale: H1 34/1.2 bold, H2 23/1.3 bold, body 16/1.65, list items 10 apart, paragraph bottom margin 1 line.
  Split preview uses 15 body. Focus mode uses 18 body in a 680 measure. Source uses the monospace role at
  13.5/1.75, with subdued delimiters, accent heading/list markers and link-colored URLs.
- Chrome text: 14 labels, 13 tree/outline and secondary labels, 12 status, 11 keyboard hints in a 1-DIP bordered chip.
- Icons: 18-DIP line icons with consistent stroke, from the host's Fluent icon font where it has the glyph.

## Contextual states

- Selection toolbar: appears above the selection after pointer release (never follows pointer movement), stays in
  the viewport, is an overlay with 10-DIP corners and a soft shadow. Contents: Text style, Bold, Italic, Code, Link
  and an "Alt+F10" hint. Tab/Esc/Enter behave per the contract and return focus to the editor with the selection.
- Find/Replace: docked at the top-right of the canvas over the document, 460 wide. Row 1: query field, "n of m",
  previous (Shift+F3), next (F3), close (Esc). Row 2: replace field, Replace, Replace all. The current match is the
  selection; other matches use a subtle neutral fill. While Find is open, outline rows show per-section match counts.
- Focus mode: hides the command row, both panes and the status bar; a 40-DIP top strip keeps the dimmed file name
  and dirty state, "Exit focus" with an "Esc" hint and caption buttons. Blocks other than the current one use the
  subdued text color (a setting may disable dimming). A bottom status pill shows word count and save state.
- Settings drawer: docks at the right, about 380 wide, non-modal so changes are visible immediately. Appearance:
  theme tiles (System, Light, Dark, Paper) with previews, Reading width, Text size. Writing: Focus mode.
  Privacy: "Load images from the internet", off by default, with "Local images always show." Footer:
  "Changes apply right away." The drawer body scrolls when it does not fit. Esc and the close button return focus
  to the editor.
- Equation (F2): the rendered block gets a subtle selected fill; directly below it an attached panel shows
  "Equation source", the exact TeX with delimiters in the monospace role with an accent focus stroke, and Cancel
  (Esc) and Apply (Ctrl+Enter). Unsupported commands stay visible as source.

## Link presentation

Requested by the owner on 2026-10-22: color alone marks a link; the underline appears on hover.

- Rich text draws links in the theme link color without underline. The underline appears while the pointer
  hovers the link and while keyboard focus or the caret is inside it, so a non-color cue exists for keyboard users
  (WCAG 2.x technique G183). This matches the existing DxUi `ButtonVariant::Hyperlink` ("accent color, underline
  on hover").
- Contrast: light `#C83C46` is 5.0:1 on `#FFFFFF` and 3.7:1 against `#141414` text; Paper is 4.8:1 on `#FFFAEF`.
  Both meet G183's 3:1 link-to-text minimum. Dark `#FF7778` is 6.0:1 on `#22252B` but only about 2.2:1 against
  `#EBEDF2` text, so dark needs D3.
- High contrast keeps the system hyperlink color and a persistent underline.
- Source view is unchanged: link syntax stays visible, delimiters subdued, URL in the link color, no underline.
- Today DxUi underlines every `Link` run (`DxUi.RichTextLayout.cpp:264` at `b69f452f`) and reports UIA
  `UnderlineStyle` Single for links (`DxUi.Accessibility.cpp:5848`). The change belongs in the canonical DxUi
  repository: a hover/caret-dependent decoration that keeps layout metrics, hit testing and UIA link exposure
  stable, followed by a reviewed pin update here. Never patch or copy DxUi sources into this app.

## Open decisions

- D1: replace the native title bar with the merged document bar. This needs a custom non-client frame, caption
  hit testing including the Windows 11 maximize snap flyout, drag regions, DPI handling and UIA for caption
  buttons. The alternative keeps the native title bar and drops only the duplicated identity row.
- D2: view selector style: underline tabs (pages) or a segmented control. Either stays visible and exclusive.
- D3: dark-theme links: accept color-only links below 3:1 against text, keep a persistent underline in Dark, or
  find a dark link/text pair that meets both 4.5:1 on the page and 3:1 against text.
- D4: right panel width: the contract says 240 and the pages use 240; keep it unless captures show truncation.
- D5: Files pane scope: listing and opening only (this plan), or later file operations such as new, rename,
  delete or reveal in File Explorer, which need their own file-safety and explicit-navigation rules.

## Acceptance

- Recapture `compose-light`, `compose-dark`, `source`, `split`, `context-selection`, `settings`, `math`, `math-dark`,
  `narrow` and `high-contrast`. Add deterministic `find`, `focus` and `files` scenarios; `files` uses a seeded
  fixture folder with nested, hidden, reparse-point and non-Markdown entries. All are actual harness renders.
- Layout tests for region sizes, both panes open, minimum window and 100/150/200% DPI without overlap or
  off-screen commands.
- Files pane tests: Markdown-only filtering, natural ordering, empty-folder pruning, limits, reparse points not
  traversed, re-rooting after Save As/Open, change-notification refresh, cancellation and stale-completion
  rejection, and the unsaved-changes prompt when opening another file.
- UIA names for the view selector, File menu, formatting commands, Files tree items, outline rows and
  current-section/current-file state.
- Keyboard and focus behavior for the Files pane, selection toolbar, Find, Settings drawer, Focus mode and F2
  editing under [C5](RedTilde_2026-10-20.md) with the person's agreement to the desktop time; hidden invocation does
  not qualify.
- Link hover/caret underline in Compose and Split preview, unchanged source and history, and UIA link exposure.
- Icon legibility and alpha at 16/24/32 DIP in the document bar and taskbar.
