# Architecture and Red-family conventions

Status: intended contract, with implemented scope in `capabilities.json`.

RedTilde edits local Markdown on Windows. It shares the Red family's native C++/Win32, DxUi, WIL, Unicode,
PowerShell build, deterministic harness, theme/settings and local crash-report conventions. It is an independent
repository and executable. It must launch and test without RedXe, RedPrism or RedSalamander present.

| Owner | Responsibility |
| --- | --- |
| Application | Window lifetime, command dispatch, prompts, dirty state, settings and session identity |
| DxUi ControlHost | Native text services, UIA, retained controls, rendering, device loss and presentation |
| RichTextEditor | Authoritative UTF-16 Markdown source, immutable snapshots, selection and undo history |
| Storage | Strict Unicode codecs, bounded reads, same-directory atomic replacement and file identity |
| Image service | Bounded application-owned decode jobs, COM/WIC, cancellation and owner completion |
| Mathematics | DxUi source-backed inline/display objects and read-only inspector, native cached glyphs/lines, explicit diagnostics |
| Harness | Hidden window, direct control commands, render capture and immutable scenario/build receipts |

Library objects remain inside the executable. No plugin ABI is introduced. The native host is used directly;
RedTilde must not allocate a competing swap chain or render over an uncoordinated chrome HWND.

The experimental upstream editor admits 65,536 UTF-16 units and 128 history transactions. Product file admission
must honor that actual limit and report rejection. Never truncate. Large-file support requires a separately
qualified upstream capacity increase. The UI must keep unsupported Markdown available as exact source.

No periodic autosave/polling timer: edited revisions arm one debounced recovery write; subsequent idle work sleeps.
File/decode results carry a document generation and are applied only on the owner. Closing invalidates the window
binding before workers can post. Image decoding must not join a blocked codec on UI teardown.

Performance qualification uses paired Release measurements on the same machine and fixture, recording source,
dependency, binaries, WARP/hardware mode, dimensions, dirty/idle/hidden memory and CPU. Initial app measurements
establish a baseline; library receipts do not establish application non-regression.
