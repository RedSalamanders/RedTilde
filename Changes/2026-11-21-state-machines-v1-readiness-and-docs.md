- Formalize RedTilde's 13 implicit UI, transaction, overlay, Find, workspace, worker, and tooling state machines
  (`RT-SM-01`..`RT-SM-09`) with explicit phases, `ActiveFileTransaction` sum types, `OverlayKind` mutual exclusion,
  `FindController`, `DocumentSessionMetadata`, `DerivedDocumentCaches`, and `ReentrantPublication<T>`; complete
  `CloseDocument` (`Ctrl+W`), `WM_DROPFILES` single-file drag-and-drop, localized `IDS_FILETOOLARGELIMIT` command refusal
  reporting (`Q20`), session-local Focus mode without settings rewrites, `--version`/`--help` CLI handling without
  creating data/crash directories, `Common/Version.h`, `Installer/Install-RedTilde.ps1`, Settings About & Shortcuts,
  per-package `.sha256` sidecars and notices, `H-24`/`H-22`/`H-23`/`H-06`/`H-08` convergence, `Setup.ps1` 260-char
  guard and bounded timeouts, `174` native test cases, updated user documentation with `16` deterministic hidden
  captures, and reconciled normative `Specs/` while moving `ExplicitStateMachines_2026-11-10.md` to `Specs/Plans/Done/`.
  Blocked `CV-6-05`/`H-10`/`H-15`/`H-26` settings convergence, physical ARM64 execution, physical printing/IME/AT,
  owner-authorized interactive focus tests, real cloud/SMB/WSL locations, paired quiet-host performance, and v1.0
  release cut remain tracked in `Specs/Plans/WIP/RemainingWork_2026-11-04.md`.
