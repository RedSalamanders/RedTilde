# Gate 4 Interactive Desktop Focus, Live UIA Tree & Clipboard/Media Qualification — 22 November 2026

Source state: `a85cdf1` (`main` after Gate 3 PR #158, `sourceIdentity`: `98fde609e5dd9e5cf8fc9dd7fe660d00a2f5964d7a8933478903b6e53b98e6f8`, `x64 Release` binary SHA-256 `29CC8C389BE4B4E1A6379E07C35F167944279FE0EAC0F6E83DA7FC9E3745FB9F`, pinned DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
Ledgers: [interactive-lease-receipt.json](interactive-lease-receipt.json) · [uia-probe-receipt.json](uia-probe-receipt.json).

## 1. Authorized Real-Desktop Interactive Lease (`test-interactive.ps1 -Platform x64 -Configuration Release -SkipBuild -InteractiveAutoStart`)

Executed directly on the owner's `Default` interactive desktop with explicit timing agreement (`ConfirmationSeconds = 30`, `AutoStart = true`, `leaseBinarySha256`: `EA24BBC92573BDB05C978D6BE69B4A67185B1AB67AD85F20FF3A59F84097C5F4`):

- **Passing Run ID:** `20261122T220455008Z-19144-a535ec0dc2374e57959a650a49179eaf` (`outcome: passed`, `Exit: 0`, `Seconds: 1.3`, `injectedEvents: 134`, `clipboardTouched: false`).
- **Desktop restoration verified by `DxUi.InteractiveLease.exe`:** `Foreground: unchanged` (`WindowsTerminal.exe` `CASCADIA_HOSTING_WINDOW_CLASS`), `Focus: unchanged` (`QueryError: 0`), `Cursor: restored` (`2414,1569`).
- **All `9/9` real keyboard & focus stages passed:**
  1. `NativeEditor.KeyboardTextAndUndo`
  2. `Files.ShortcutFocusAndEscape`
  3. `Find.TypedQueryTraversalAndEscape`
  4. `Settings.KeyboardDrawerAndEscape`
  5. `Focus.KeyboardToggleAndEscape`
  6. `SelectionToolbar.AltF10TabEnterEscape`
  7. `Equation.F2InPlaceEditUndoAndExit`
  8. `FileMenu.KeyboardOpenAndDismiss`
  9. `Window.AltF4AsyncClose`
- **Retained prior attempt (`20261122T220335534Z-49492-cd8d77d3059b49658d06d596536d2475`):** All `9/9` child checks (`134` injected events) passed in `1.3 s` (`childExitCode: 0`, `childResultSha256: 9311C2E...`), while `DxUi.InteractiveLease.exe` exited `25` because the operator shifted the cursor during execution (`2313,1401 -> 2192,1415`); re-run immediately hands-free produced full exit-`0` restoration parity above.

## 2. Automated Live UI Automation (`UIAutomationClient`) & Clipboard/Media Verification

Executed against the same `29CC8C389BE4B4E1A6379E07C35F167944279FE0EAC0F6E83DA7FC9E3745FB9F` binary:

- **Live UIA tree & pattern probe (`uia-probe-receipt.json`):**
  - Top-level window name (`uia-fixture.md`) verified; **`62` descendant elements** (`56` named controls: `26` Buttons, `16` Text, `6` Separators, `1` Document `"Markdown document"`, `1` Table + `2` HeaderItems + `2` DataItems, `1` Tree + `1` TreeItem, `1` List + `2` ListItems, `2` task CheckBoxes, `1` Image).
  - Exposed patterns: `GridPattern`, `GridItemPattern`, `InvokePattern`, `SelectionPattern`, `SelectionItemPattern`, `TablePattern`, `TableItemPattern`, `TextPattern`, `TogglePattern`.
  - Programmatic UIA activation succeeded for `Source` -> `Compose` view switching (`sourceAndComposeInvokeVerified: true`) and toggling `Settings` (`settingsToggleVerified: true`), followed by clean `CloseMainWindow()` (`processExitCode: 0`).
- **Deterministic clipboard & media scopes on exact binary (`29CC8C38...`):**
  - `--self-test --test-scope EditorMedia` (`0`), `--test-scope EditorBehavior` (`0`), `--test-scope Settings` (`0`), and `--test-scope Storage` (`0`) cover `Editor.ClipboardBitmapPaste*`, `Editor.ClipboardBitmapAssetHandler`, `Editor.FilePasteAdmissionRejectsHostileNames`, `Settings.ImageFirstPasteRoundTripAndFallback`, and `Storage.BitmapAssetsEncodeAndRetireOwnedFiles`.

## 3. Explicit Post-1.0 Scope Decision

Per explicit owner decision for v1.0.0:
- Physical East Asian IME composition hardware walk, interactive Windows Narrator screen-reader walkthrough, and physical Touch/Pen digitizer hardware qualification remain explicitly deferred post-1.0 (`"physical-ime-touch-and-assistive-technology"`).
