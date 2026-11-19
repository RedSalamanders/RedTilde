# RedTilde development guidance

RedTilde is a Windows-native Markdown writing application in the Red family. The product is C++/Win32 with
DxUi controls, DirectWrite and Direct2D. Read [Specs/README.md](Specs/README.md), the owning domain contract and
[README.md](README.md) before changing behavior. Public capability status belongs in `capabilities.json`.

## Ownership

- `RedTilde/` owns the window, commands, native DxUi host, theme adapter and editor presentation.
- `Common/` owns Unicode file storage, settings, recovery, crash reporting and document-derived data.
- DxUi is an exact pinned `DxUi.lib` dependency. Edit shared controls in their canonical DxUi repository, never
  copy their implementation into this app or compile an enumerated set of DxUi source files.
- The rich editor owns the authoritative source and undo history. View switches never serialize rendered text.
  A read-only split mirror and equation inspector are derived views. No second editable document may diverge.
- App policy, paths, settings, file commands and release decisions stay here. A library change never updates
  this consumer implicitly. Rich-text PR #89 is merged; its performance and product qualification gates remain
  separate from that merge and from this consumer's exact-pin adoption.

## Engineering

- Unicode Win32 APIs, modern C++, `stdcpplatest`, `/Wall /permissive- /sdl /utf-8`. Discover Visual Studio;
  do not hard-code a developer installation. Keep x64/ARM64 Debug, Release and ASan Debug configurations.
- WIL owns COM and Windows resources. No manual `Release` or destruction of an owned HWND.
- No exception escapes Win32/callback boundaries. Catch named exceptions with an explicit fallback; allocation
  failure is fatal at application boundaries. Never silently discard an edit or storage error.
- Ordinary file saves stage in the destination directory and atomically replace; preserve original UTF-8 BOM and EOLs.
  Symbolic links save their followed target. Multiple hardlinks require an independent durable recovery copy before
  an in-place write that preserves all aliases; failed writes retain that copy and the old saved baseline.
  A successful write/replace is required before clearing dirty state. Reject invalid/oversized input before
  replacing the current document. Prompt before New, Open or Close would discard unsaved changes.
- Filesystem/codec work belongs to application workers. Keep queues bounded, publish on the UI owner, cancel
  by request/session identity and reject stale completion. Idle work blocks on events; no polling timer.
- Preserve source ACP selection, history, IME, keyboard focus and UI Automation. A floating toolbar must retain
  the selection, accept Tab/Esc/Enter, and return focus to the editor after actions.
- User-visible chrome strings live in `res/RedTilde.rc`. Document examples are content, not chrome.
- Never execute Markdown HTML, scripts or Mermaid directives. Ordinary link clicks stay inert; Open link,
  Ctrl+click and the shared explicit navigation action require confirmation before external dispatch.
  Network image access is declined by default. Unsupported content stays editable.
- Review the design system and usage docs on every UI change. Screenshots come from the app's deterministic,
  hidden, non-activating harness, with scenario/build provenance. Never substitute desktop capture.
- No generated products are committed under `.build`. Do not kill independently launched applications.

## Folder guidance

These rules were the five nested `AGENTS.md` files; the root file is now the only guidance file (RedHome convergence A-12).

- `Common/`: read the owning Core or Editor contract. Keep services independent of UI controls except public
  immutable DxUi value types. Storage validates all input before changing a live session. Return errors explicitly;
  tests use unique directories under the harness output. Workers own snapshots, never mutable controls or HWND
  lifetime. Crash-path code has separately documented allocation restrictions; ordinary code uses WIL.
- `RedTilde/` (application and native UI): read the Editor contract and UI design system. The authoritative
  RichTextEditor remains stable for the session; changing views must not call SetText on it. Return focus and
  preserve source selection after formatting and menu actions. High contrast overrides custom themes. Use resource
  strings for chrome and the app harness for actual captures. Do not add desktop automation, a web editor or a
  second presentation engine.
- `Specs/`: start with README.md. Domain contracts own lasting requirements, capabilities.json owns current status,
  and one indexed WIP plan owns sequence and unresolved gates. A passing build or focused test is not product
  qualification. Move a plan to Done only after all of its implementation, test and documentation gates close.
- `Tests/`: follow the validation contract. Native filenames are Scope.Tests.Behavior.cpp/.h. Use behavioral oracles
  and independent expected values. Hidden tests must not activate windows, take the clipboard, move the pointer or
  inject OS input. Interactive focus/IME tests require agreement to the time and verified restoration. Store
  generated results in .build and only reviewed, provenance-bearing evidence in Measurements.
- `res/` (resources and brand): keep the generated red-tilde master PNG and multi-resolution Windows ICO together.
  Preserve alpha and verify 16/24/32/48/64/128/256 entries. Resource IDs remain stable. Chrome strings use
  STRINGTABLE entries; positional format placeholders permit translation. Document samples are separate under
  docs/examples.

## Drive-root output policy

Do not create scratch, caches, evidence, copied checkouts, or other output directories directly at a drive root.
Use repository-local generated outputs or an existing approved workspace. The only permitted new drive-root
output name is `<Project>.Perf` (for example `RedTilde.Perf`, `RedSalamander.Perf`, or `RedPrism.Perf`), and the
user must explicitly authorize the exact path before its first creation or initialization. A matching name,
environment variable, low disk space, or authorization for another drive/project is not authorization.
Keep outputs beneath the approved root; do not invent sibling root folders. Preserve runner-specific ownership
markers and authorization. Historical cleanup inventories and preserves useful evidence/recovery data, checks
process use, repairs live junction/worktree references, and verifies preservation before removal. Never follow
junctions during cleanup or leave compatibility junctions at forbidden old roots.

## Workflow

Keep multi-step work in one indexed WIP plan. Durable behavior belongs in domain specs, not the plan. Move a
plan to Done only when its implementation, tests and documentation are complete. Keep upstream, physical
IME/AT, interactive focus, native ARM64 and release gates explicit; a cross-build is not a runtime pass.


Use `Test-Changes.ps1 -Explain` and affected testing while editing. `-Mode PrePush` accounts for enabled CI and
remaining local obligations. Never reuse evidence without the exact source, pin, toolchain, binary and scenario
identity. Run `validate.ps1`, `format.ps1 -Check` and the affected suites. Qualification requires all six builds,
x64 tests in all three configurations, a real ASan detection probe and native platform execution for claims.
Real-focus/clipboard desktop tests require the person's agreement to the time. Headless harness tests do not
take foreground focus. Capture paired performance on the same fixture before claiming non-regression.

## Focused skills

Read the appropriate skill under `.agents/skills/`:

| Skill | Work |
| --- | --- |
| [consumer-dxui](.agents/skills/consumer-dxui/SKILL.md) | Exact dependency pin and API adoption |
| [markdown-editing](.agents/skills/markdown-editing/SKILL.md) | Source/history, rich/source/split and local source |
| [desktop-ui](.agents/skills/desktop-ui/SKILL.md) | Layout, contextual commands, focus, theming and accessibility |
| [media-math](.agents/skills/media-math/SKILL.md) | Images, Mermaid and equation previews |

Cross-repository C++/WIL, localization, and `spec-workflow` skills live in the sibling `RedHome` repository. So do the shared skills that replaced four local ones, each with RedTilde's facts in its `references/repository-facts.md`:

| Shared skill | Work |
| --- | --- |
| `native-build` | Build, restore, package and toolchain failures (was `build-redtilde`) |
| `file-io-safety` | Open/save, conflict detection and recovery (was `file-safety`) |
| `app-settings` and `crash-dumps` | Live settings, minidumps and restart recovery (was `settings-crashes`) |
| `test-evidence` | Deterministic scenarios, captures and evidence (was `test-harness`) |

Shared skill index: `../RedHome/AGENTS.md`; packages: `../RedHome/.agents/skills/`. If `../RedHome` is missing, clone https://github.com/RedSalamanders/RedHome.git to that path. This repository's guidance and domain specs own product behavior and validation.

## Scoped testing policy (2026-10-21)

Use `Test-Changes.ps1 -Explain` and the affected default while editing. Use explicit Full/PrePush for qualification; this accepted workflow replaces unconditional full-test iteration. Successful default affected runs advance their source/index baseline; explicit partial, failed, mutated or CI-pending runs do not. Reuse only exact successful evidence. CI_PENDING requires all named candidate checks to pass before merge, including when this private account cannot enforce protected checks. Preserve native/manual/platform/performance gates in Specs/Testing/Testing_Validation.md and contextual Scope.Tests.Something.h/.cpp names.
## Changes workflow

Record each change as one dated Markdown fragment under `Changes/`; follow [Changes/README.md](Changes/README.md)
and [Specs/Core/Core_Changes.md](Specs/Core/Core_Changes.md). Do not hand-edit generated `CHANGELOG.md` entries.
Keep the fragment in the change: never fold it in a feature change and never edit `CHANGELOG.md`. Fragments are
folded on `main` after merge by `Tools/Submit-ChangelogFold.ps1`, which opens a fold pull request of its own. Run
`Tools/Test-Changelog.ps1` while editing; PrePush and CI run it with `-Base`, which refuses any `CHANGELOG.md` change
that is not an exact fold of fragments already on `main`. Validation is read-only.

## Shared native build policy (2026-10-28)

Use /Wall with /WX disabled in every first-party profile. A build is accepted
only with zero warnings and zero errors; fix actionable diagnostics rather than
adding blanket suppressions. Ordinary Debug uses /ZI, ASan Debug uses /Zi.
Release explicitly enables /OPT:ICF; Debug/ASan explicitly disable it.
Follow Specs/Build/Build_ConsoleAndPaths.md for the common green result, safe
terminal launch links and short dependency folders with full identity checking.

## Unified CI policy

Use [Specs/Build/Build_UnifiedCI.md](Specs/Build/Build_UnifiedCI.md) for hosted CI. PRs run compiler-free checks and formatting feedback. Main native qualification starts after five continuous quiet minutes; both native Release test branches must pass before either production Release/ZIP branch starts. Scheduled dependency/security/status checks remain compiler-free. Formatting commits, extended qualification and ZIP/winget publication are explicit manual actions. PR checks do not satisfy native acceptance or waive applicable local scoped tests. Shared-engine pins in .ci/project.json and workflow callers must agree. Preserve dependency pins, native toolchain, evidence and desktop ownership contracts.
