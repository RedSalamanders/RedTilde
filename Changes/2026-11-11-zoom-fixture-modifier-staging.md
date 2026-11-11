# Stage hidden zoom modifiers at each dispatch

The hidden document-scale shortcut fixture now reapplies its requested calling-thread keyboard state
immediately before each direct shortcut and wheel message. Earlier commands can pump messages and refresh
that state. The fixture preserves all scale, AltGr, source, selection, history and focus expectations and
restores the original keyboard snapshot without injecting input or activating a window.
