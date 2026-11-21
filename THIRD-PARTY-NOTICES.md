# Third-party notices

RedTilde is copyright (c) 2026 RedSalamander, under the MIT license in [LICENSE.txt](LICENSE.txt). The components below
are compiled into or shipped with the portable package and keep their own terms. A new dependency or asset extends
this list, with its full notice, before it is distributed.

- **DxUi**, at the commit in [Dependencies/DxUi.lock.json](Dependencies/DxUi.lock.json), copyright (c) 2026
  RedSalamander, MIT, including its native Markdown and Unicode components. The application-owned
  `RedTilde/ImageLoader.h` service is adapted from that revision's public `Samples/EmbeddedControls/RichImageLoader.h`
  under the same license; it is not a copied control implementation. That revision compiles in WIL (below); toml++
  3.4.0, Mark Gillard, MIT; a math symbol table derived from KaTeX 0.16.11, Khan Academy and contributors, MIT;
  libfyaml 0.9.4, Pantelis Antoniou, MIT, with its bundled xxHash, Yann Collet, BSD 2-Clause; tables generated from
  Unicode 17.0.0, copyright Unicode, Inc., Unicode License V3; and an HTML named-character table from the WHATWG
  snapshot of 18 October 2026, copyright WHATWG (Apple, Google, Mozilla, Microsoft), BSD 3-Clause for code and CC BY
  4.0. In pinned-source packages, DxUi's license and notices ship as `Dependencies/DxUi-LICENSE.txt` and
  `Dependencies/DxUi-THIRD-PARTY-NOTICES.md`, with the texts of its `docs/licenses` folder (`katex.txt`,
  `tomlplusplus.txt`, `Unicode.txt`, `WHATWG.txt`, `WIL.txt`, `rapidyaml.txt`, `rapidyaml-bundled.txt`, `toml-test.txt`)
  under `Dependencies/docs/licenses`, and the libfyaml and xxHash notices as `Dependencies/libfyaml-LICENSE.txt` and
  `Dependencies/libfyaml-xxHash-LICENSE.txt`. In qualified DxUi SDK packages, the complete SDK license closure ships
  under `Dependencies/DxUiSdk`.
- **WIL** (Windows Implementation Libraries) 1.0.260126.7 or later, Microsoft Corporation, MIT, header-only, restored
  by DxUi's manifest. Its vcpkg copyright file ships as `Dependencies/WIL-LICENSE.txt` (or
  `Dependencies/DxUiSdk/WIL-LICENSE.txt` in SDK packages).
- **yyjson** 0.13.0 or later, YaoYuan, MIT, statically linked and restored by RedTilde's vcpkg manifest. Its
  vcpkg copyright file ships as `Dependencies/yyjson-LICENSE.txt`.
- The **RedMonitor** client header, at the commit in [Dependencies/RedMonitor.lock.json](Dependencies/RedMonitor.lock.json),
  copyright (c) 2026 RedSalamander, MIT. Its [notice](ThirdParty/RedMonitor/LICENSE.txt) ships as
  `Dependencies/RedMonitor-LICENSE.txt`.
- The **Microsoft Visual C++ runtime** (`vcruntime140*.dll`, `msvcp140*.dll`) ships beside the executable under the
  Microsoft Visual Studio distributable-code terms. Windows SDK and system components (Direct2D, DirectWrite, Direct3D 11,
  DXGI, WIC, DbgHelp, WinHTTP, Win32, Print Ticket/XPS) are provided by Windows under Microsoft terms. There is no
  embedded JavaScript or web editor and no external math engine.
