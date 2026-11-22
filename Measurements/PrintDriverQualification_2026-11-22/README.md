# Gate 3 Real Windows Print Driver Qualification — 22 November 2026

Source state: `fc33843f2db0e856c071b3422f2c52a412d35e80` (`main`, `x64 Release` binary SHA-256 `B4AD60EB15D62C62B171821F81AC504A680ADEB930BABD913E0AF5537B0A20E1`, pinned DxUi API 5 `0c371a6197eb8c6b876b4b2843d75078bc21035c`).
OS environment: Windows `10.0.26300.9974` (`26H2`).
Receipt ledger: [print-driver-qualification.json](print-driver-qualification.json).

## 1. Live `Microsoft Print to PDF` Qualification (`Microsoft Print To PDF` `10.0.26100.4484`)

Executed against the real Windows spooler (`PORTPROMPT:`, `IPrintDocumentPackageTargetFactory`, `ID2D1PrintControl`, `PTConvertDevModeToPrintTicket`, `FileOutput::Publish`) through `.build/gate3-print-probe/PrintDriverProbe.exe` linked against `RedTilde` `x64 Release` objects and independently verified via `Windows.Data.Pdf.PdfDocument` + FlateDecode stream inspection + rendered WinRT bitmap ink checks (`leftoverPrintTemps = 0`):

| Case | Document & Settings | Preview Pages | WinRT PDF Pages | Size (DIP) | Bytes | Highlights |
| --- | --- | --- | --- | --- | --- | --- |
| `pdf-showcase-letter-portrait` | `docs/examples/markdown-showcase.md`, Letter Portrait, all pages | `3` | `3` | `816 × 1056` | `1,395,041` | `14` font objects, `2` image XObjects, text operators & page-1 dark ink verified |
| `pdf-showcase-a4-landscape-range` | `docs/examples/markdown-showcase.md`, A4 Landscape, range `1, 2-3, 2` | `4` | `3` | `1122.56 × 793.76` | `1,389,291` | Disjoint/overlapping range deduplicated & emitted in order; `14` font objects; landscape orientation verified |
| `pdf-fallback-current-page` | `docs/examples/rendering-fallback-cases.md`, Letter Portrait, `Current page = 2` | `4` | `1` | `816 × 1056` | `1,060,063` | Single selected page rendered & verified |
| `pdf-local-images-letter-portrait` | `docs/examples/local-images.md`, Letter Portrait (`PNG`, `JPG`, `BMP`, `GIF`, `TIF`) | `2` | `2` | `816 × 1056` | `1,152,279` | `6` image XObjects across all 5 local image formats verified |
| `pdf-overwrite-existing` | Existing sentinel PDF target, Letter Portrait page `1` | `4` | `1` | `816 × 1056` | `1,056,017` | `FileOutput::Publish` atomic `ReplaceFileW` succeeded (`hr = 0`) |
| `pdf-cancel-preserves-existing` | Cancelled `ChooseFile` (`ERROR_CANCELLED`) against sentinel PDF (`32` B) | `4` | `—` | `—` | `32` | Sentinel SHA-256 `E3F3DB5F...` preserved unchanged; `savedFile = false` |
| `pdf-refuse-overwrite-open-source` | `ChooseFile` targeting open `document.path` (`833` B) | `1` | `—` | `—` | `833` | Refused (`E_ACCESSDENIED`); source SHA-256 `AD44850A...` preserved |

## 2. Installed Physical & Secondary Virtual Drivers (`ConfigurePrinter`, `DEVMODEW` → `PrintTicket`, Preview Render)

Verified directly against live installed queues (`previewReady = true`, `previewFallbackPaper = false`, `previewHasInk = true`) **without spooling physical sheets** (`spoolSubmitted = false`) per explicit owner scope decision:

| Destination | Driver (`Version`) | Forms | Default Size / Printable (DIP) | `DEVMODEW` / `PrintTicket` | Capabilities Verified |
| --- | --- | --- | --- | --- | --- |
| `Microsoft Print to PDF` | `Microsoft Print To PDF` (`10.0.26100.4484`) | `84` (Letter, A4) | `793.76 × 1122.56` / full | `5,428` B / `9,143` B (`S_OK`) | Portrait & A4 Landscape transitions, `maxCopies = 1` |
| `Brother HL-L2445DW Printer` | `Microsoft IPP Class Driver` (`10.0.26100.9974`) | `20` (Letter, A4) | `793.76 × 1122.56` / `[16, 16, 777.76, 1106.56]` | `33,188` B / `56,149` B (`S_OK`) | Landscape (`1122.56 × 793.76`), A4 Landscape, duplex `OneSided`/`LongEdge`/`ShortEdge`, `maxCopies = 999` + collation |
| `OneNote (Desktop)` | `Send to Microsoft OneNote 16 Driver` (`16.0.7629.4000`) | `10` (Letter, A4) | `793.76 × 1122.56` / `[22.72, 16, 770.88, 1106.24]` | `1,052` B / `3,207` B (`S_OK`) | Portrait & A4 Landscape transitions + preview render |
| `OneNote (Desktop) - Protected` | `Microsoft Virtual Print Class Driver` (`10.0.26100.9974`) | `10` (Letter, A4) | `793.76 × 1122.56` / full | `3,512` B / `6,601` B (`S_OK`) | Portrait & A4 Landscape transitions + preview render |

## 3. Explicit Scope Boundary

- Physical toner/paper sheet consumption on `Brother HL-L2445DW Printer` was explicitly waived for v1.0.0 (`physicalPaperSpooled = false`); physical paper output inspection remains a separate post-1.0 hardware check (`"physical-printer-paper-sheet-inspection"`).
- Deterministic headless print/preview regression suites (`9` tests in `Tests/Application.Tests.Printing.cpp`) continue passing across all compiled profiles in `Tests/NativeInventory.json`.
