# Markdown styling and caption-button evidence

These are actual hidden RedTilde harness renders with invented content. No desktop
capture, foreground activation, cursor movement or real input was used.

The SDK pin is `eb0f10b91bb8de19b0d7671cc7078fd0c1f5e767`. The Debug run's full
source identity is `49c149e715e1f9fe41c87b061d48f2d852e91e41e2ed26e1741192d05241dc1a`;
its compiled-source identity is
`2166a79bc250d6042ad5698927eb0d3ac3e95a22e1525ff4d7934808590e0e87` and executable
SHA-256 is `26DCA1E789DA0BF9A289EE4C4D1ACB2086A98585E8EF04AE9D82A2A0DE43BA6D`.
Later WIP/evidence edits do not relabel that run's whole-checkout identity.

[The original receipt](debug-receipt.json) records 67 passing native cases, all
32 capture/provenance pairs, process-exit/recovery probes and the isolated crash
probe. [Editor results](debug-editor-cases.json) contain the caption lifecycle,
DPI, source/history/focus, resource and device-replacement cases. The nine images
below are the reviewed subset; their untouched sidecars record actual viewport
and simulated host DPI. [The manifest](manifest.json) binds all twenty-two copied files.

The [Release receipt](release-receipt.json) and [Editor cases](release-editor-cases.json)
also pass all 67 native cases and 32 capture pairs. Its full source identity is
`97c6f28c81f658b8955295cd7441c3631919bb431668e29f4f0e8bae14fa2e8e`, with executable
SHA-256 `60C2E478853C5A198041CBAF388E5C5A95C26D45B453CFF348D63C516990C3D2`.

| Host DPI | Resting Markdown | Close hover | Maximize hover |
| --- | --- | --- | --- |
| 96 | [Capture](captures/markdown-style-dpi-96.png) | [Capture](captures/caption-close-hover-dpi-96.png) | [Capture](captures/caption-maximize-hover-dpi-96.png) |
| 144 | [Capture](captures/markdown-style-dpi-144.png) | [Capture](captures/caption-close-hover-dpi-144.png) | [Capture](captures/caption-maximize-hover-dpi-144.png) |
| 192 | [Capture](captures/markdown-style-dpi-192.png) | [Capture](captures/caption-close-hover-dpi-192.png) | [Capture](captures/caption-maximize-hover-dpi-192.png) |

At each DPI the three PNG hashes must differ. The native caption case also checks
idle/hover/pressed regions, release outside, cancellation, capture loss, repeated
clicks and the complete double-click sequence. Application commands are counted
through the real button lifecycle without actually minimizing, maximizing or
closing a desktop window. Maximize hit testing and native default hover processing
remain intact; these captures do not qualify the physical Snap flyout.

Capture stages the rendered backbuffer before Present. Synthetic interaction
probes settle content first and snapshot between input steps, without pumping
ambient leave messages for their hidden HWND. Earlier identical hover images and
the first Editor capture failure are retained privately as failed attempts.

The visible Markdown changes are heading separators, content-sized tables with
horizontal neutral rules, centered standalone diagrams/display equations and
shaded quotes. Native typography and diagram routing retain their own implementation;
this is not a pixel-identical browser rendering claim. Exact source, selection and
history remain authoritative.

Broader configuration/native ARM64, physical-input and paired-performance gates
remain separately tracked in the [WIP coordinator](../../Specs/Plans/WIP/RedesignCompletionAndRendering_2026-10-24.md).
The smoke timing/memory banner is unpaired and cannot establish non-regression.
The older performance/CI command queue stays paused.
