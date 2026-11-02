# Third-party notices

RedTilde links the exact DxUi source named in `DxUi.lock.json`, including its native Markdown/Unicode components.
See that checkout's `LICENSE.txt` and third-party notices for the full licenses. WIL is restored by the DxUi
manifest. The current pin also compiles toml++ 3.4.0 and rapidyaml 0.16.0 privately and derives its math symbol
table from KaTeX 0.16.11 (all MIT); packages carry their texts under `Dependencies/docs/licenses`. The application-owned `RedTilde/ImageLoader.h` service is adapted from that pin's public
`Samples/EmbeddedControls/RichImageLoader.h`; it is not a copied control implementation.

Windows SDK components (Direct2D, DirectWrite, WIC, DbgHelp, Win32) are provided by Windows. The C++ runtime is
provided by the matching Microsoft Visual C++ redistributable/toolchain. Portable development packages require
that runtime on the destination machine. There is no embedded JavaScript/web editor or external math engine.
