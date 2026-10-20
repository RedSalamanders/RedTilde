# RedTilde design system

## Product direction

Write naturally. Keep the precision of Markdown. Use Word's familiar text actions without a permanently expanded
ribbon, pagination controls or modal formatting workflow. A quiet writing surface, one compact formatting row,
contextual selection tools and an optional outline leave the document dominant.

Brand: a flowing red tilde, with a restrained scarlet accent. App name is RedTilde. Short product line:
**Words, beautifully in flow.** The icon must read at 16/24/32 pixels as well as 256; preserve alpha.

## Layout

The 56-DIP document bar groups brand, document identity/dirty state, global view selector and Save. A 48-DIP
command row groups text structure, inline emphasis, lists and insertion. The canvas starts below it with generous
gutters and a readable 760-DIP maximum measure. Source and Split use the available width. A 240-DIP right panel
holds outline, active-block actions and the equation inspector. Status gives words, characters, encoding and save
feedback. Narrow windows collapse the inspector before reducing the writing measure; focus mode hides optional
chrome. Minimum supported window is 800x560 DIP. No overlapping or off-screen commands at 100/150/200% DPI.

Compose/Source/Split are visible, mutually exclusive states. Theme and preference choices are in a drawer rather
than a second native settings app. Commands show labels/tooltips and shortcut hints. Selection tools should appear
above selected text after pointer release, remain inside the viewport, and avoid following every pointer movement.
Escape dismisses; Alt+F10 or the context-menu key provides keyboard access. Context menus retain the source
selection and support arrows, Tab, Enter and Esc before returning focus.

## Tokens and behavior

Use 4/8/12/16/24/32 DIP spacing; 6-DIP button corners, 10-DIP overlay corners. Body chrome uses Segoe UI Variable;
Markdown body uses DxUi typography, source uses its monospace role. Target at least 32-DIP compact buttons and
40-DIP primary actions. Colors derive from one host theme; respect high contrast and visible keyboard focus.
Do not communicate dirty/errors/mode only by color. UIA names identify mode buttons, formatting commands and
the Markdown document. Error messages explain the failed action and leave the source editable.

## Capture scenarios

Native mathematics reserves the full fraction/root/matrix height, aligns inline formulas with prose and uses
the host's text/selection palette. F2 exposes exact delimiters. The inspector is a read-only instance of the same
engine. A recovery sheet shows original location and a scrollable read-only source preview, followed in reading
order by Restore a copy, Discard snapshot and Keep for later. Restore returns focus to the authoritative editor;
the first save asks for a filename with a recovered suffix. Recovery errors remain visible in the document footer.

Additional deterministic scenarios: `math`, `math-dark`, `mermaid`, `recovery`. Recovery capture uses an explicitly
seeded synthetic candidate to show the actual sheet; crash/restart tests separately establish storage behavior.

The app harness owns `compose-light`, `compose-dark`, `source`, `split`, `context-selection`, `settings`,
`local-source`, `mermaid`, `image`, `math`, `narrow` and `high-contrast`. Captures must be actual application renders, tagged
with dependency SHA, source identity, binary hash, dimensions, theme and scenario. Proposal art is separately
labelled. Physical screen-reader/IME/touch acceptance and real focus restoration require separate evidence.
