# RedTilde interactive keyboard input — eed2abe9

**Status:** Retained sanitized evidence projection; results remain source-bound and acceptance gates are separate.

## Bound interaction result

This packet is bound to RedTilde commit `eed2abe9885c57f44ab192155b341582782020f9`, DxUi pin `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`, and the x64 Release binary identified in `source-and-evidence-hashes.json`. The lease-guarded probe passed all nine stages and recorded 168 real keyboard events. Clipboard use was false. Physical IME or assistive technology (AT) input was not qualified.

The nine completed stages were `NativeEditor.KeyboardTextAndUndo`, `Files.ShortcutFocusAndEscape`, `Find.TypedQueryTraversalAndEscape`, `Settings.KeyboardDrawerAndEscape`, `Focus.KeyboardToggleAndEscape`, `SelectionToolbar.AltF10TabEnterEscape`, `Equation.F2ApplyUndoAndCancel`, `FileMenu.KeyboardOpenAndDismiss`, and `Window.AltF4AsyncClose`.

Initial focus capture was recorded. The separate canonical parent desktop lease result reports foreground, focus, and cursor unchanged and `runMovedSomething=0`. This packet intentionally omits foreground application/window identity, process ID, cursor coordinates, and absolute paths.

## Separate SDK Menu evidence

SDK Menu evidence at DxUi commit `f482779212f73ac8f3726bff4e3a309ef0be9112` is separately bound: 124 individual cases have matching raw START/DONE records; the raw totals are 125 START and 125 DONE when the single suite banner is included. Restoration was verified separately. That receipt does not qualify combined SDK revision `7020121d`; its later build/Menu/CI receipts have separate source bindings and are outside this app input packet. No SDK Menu raw file is copied here.

## Evidence limits

This packet records one interactive keyboard probe, not complete product acceptance. It does not qualify physical IME or assistive technology (AT) input. No performance or repository acceptance claim is made.
