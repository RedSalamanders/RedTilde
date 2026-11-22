# Dictionary and spell-checking architecture and implementation specification

## Metadata

- **Status**: `ACTIVE` (Option E — Pluggable `ISpellProvider` Hybrid selected by owner on `2026-11-22`)
- **Created**: `2026-11-21` · **Updated**: `2026-11-22`
- **Pinned DxUi baseline**: `0c371a6197eb8c6b876b4b2843d75078bc21035c` (API 5, [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json)) — `S1`–`S3` ship immediately against the current pin; planned upstream `DxUi` simplifications are specified in [Section 6](#6-planned-upstream-dxui-improvements-to-simplify-redtilde)
- **Companion active plans**: [Remaining work and qualification](RemainingWork_2026-11-04.md), [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md)
- **Domain contracts to update upon implementation**:
  - [Specs/README.md](../../README.md)
  - [Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Core_Settings.md](../../Core/Core_Settings.md)
  - [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  - [Core_CrashHandling.md](../../Core/Core_CrashHandling.md)
  - [Core_PerformanceAndResources.md](../../Core/Core_PerformanceAndResources.md)
  - [Editor_Markdown.md](../../Editor/Editor_Markdown.md)
  - [UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
  - [Build_Toolchain.md](../../Build/Build_Toolchain.md)
  - [Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [Testing_Performance.md](../../Testing/Testing_Performance.md)
  - [SettingsStore.schema.json](../../../SettingsStore.schema.json)
  - [capabilities.json](../../../capabilities.json)
  - [THIRD-PARTY-NOTICES.md](../../../THIRD-PARTY-NOTICES.md) (only if optional `S4` introduces a third-party Hunspell DLL)
  - [docs/README.md](../../../docs/README.md)

---

## 1. Architecture decision, scope, and hard invariants

RedTilde adopts **Option E — Pluggable `ISpellProvider` Hybrid**:

- **Phases `S1`–`S3` Primary Engine (`WinSpellProvider`)**: Windows 11 native Spell Checking API (`<spellcheck.h>`: `ISpellCheckerFactory`, `ISpellChecker`, `ISpellChecker2`) hosted on a lazy-initialized single-threaded STA COM worker (`SpellService`) behind an abstract `ISpellProvider` interface.
- **Deterministic Test Double (`FakeSpellProvider`)**: In-memory deterministic provider implementing `ISpellProvider` so native unit, UI, decoration-cap, and `--capture` tests run identically across `x64`, `ARM64`, and CI runners regardless of host OS language packs.
- **App-Owned Portable Custom Dictionary (`CustomDictionary`)**: Persistent UTF-8 `custom-dictionary.txt` under the active data directory (`--data-dir` or `%LOCALAPPDATA%\RedTilde`) plus optional freestanding Microsoft Word `CUSTOM.DIC` interoperability (UTF-8, UTF-8 BOM, UTF-16LE BOM, UTF-16BE BOM).
- **Optional Deferred Extension (`S4`)**: Local-only `HunspellProvider` (`.aff`/`.dic` user-supplied folders) behind the same `ISpellProvider` seam, and optional user-supplied MyThes (`.idx`/`.dat`) lookup.

### Settled design parameters

| Parameter | Settled Decision |
| --- | --- |
| **Primary scope (`S1`–`S3`)** | Real-time Markdown-aware spell checking, wavy red squiggle underlines + UIA `SpellingError` annotations, on-demand right-click / `Shift+F10` suggestions, `Ignore Once`, session `Ignore All`, persistent `Add to Dictionary` / `CUSTOM.DIC`, multi-language checking (`1..3` tags), skip filters, status bar language/status chip, Settings drawer **Proofing** section, and `F7` Proofing review pane. |
| **Deferred scope (`S4`)** | Optional local user-supplied Hunspell `.aff`/`.dic` fallback and optional local user-supplied MyThes (`.idx`/`.dat`) thesaurus lookup; zero bundled corpora, zero automatic pack downloads. |
| **Default enabled state** | `spelling.enabled = true`, with lazy STA initialization triggered only when enabled and non-empty prose needs checking (`0 B` overhead on cold start or empty documents). |
| **Custom dictionary write boundary** | Writes go **only** to RedTilde's `custom-dictionary.txt` (under `--data-dir` / `%LOCALAPPDATA%\RedTilde`) or an explicitly configured freestanding `CUSTOM.DIC`. Windows OS dictionaries are read via `ISpellChecker::Check`/`Suggest` strictly read-only; RedTilde never calls `ISpellChecker::Add` or `ISpellChecker2::Remove` by default. |
| **Milestone target** | Flagship **`v1.1`** feature; `v1.0.0` stabilization keeps existing Gates 3–7 in [Version 1.0 release readiness](V1ReleaseReadiness_2026-11-21.md) undisturbed while `S1` proceeds cleanly on a dedicated feature branch. |

### Hard engineering invariants

1. **Zero unsolicited network I/O**: No remote dictionary pack downloads or telemetry. Only locally installed Windows language packs (and optional user-supplied local files in `S4`) are read.
2. **Zero third-party build/license bloat in `S1`–`S3`**: [vcpkg.json](../../../vcpkg.json) remains `yyjson`-only; `S1`–`S3` stay 100% MIT + standard Windows SDK components.
3. **Allowed Write Locations Only (Fail Closed)**: Never write outside the app data directory (`--data-dir` / `%LOCALAPPDATA%\RedTilde`) or an explicitly user-chosen writable regular `CUSTOM.DIC`; refuse symlinks and reparse points.
4. **Zero idle CPU and zero steady-state cold tax**: Worker sleeps event-driven (`idleProcessCpu100ns == 0`); Print Preview and Printing omit all spelling decorations.
5. **Code & safety standards**: C++23 (`stdcpplatest`), Unicode, `/Wall /permissive- /sdl` with zero warnings across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` profiles, `_camelCase` private/protected members (`.clang-tidy` / `H-27`), WIL RAII, `noexcept` checked interfaces, no exceptions across Win32/COM boundaries, and `DxUi::Failure::FailAllocation()` on fatal OOM.

---

## 2. System architecture and module contracts

### 2.1 End-to-end data flow

```text
Editor Snapshot + SemanticDocument (Compose/Split) or MarkdownSyntax (Source)
   │
   ▼  (UI Owner Thread — fast exclusion & chunking)
Common/ProseExtractor
   │  Offset-preserving prose segments (contractions & elisions intact)
   │  + Exclusion mask (Code, Math, Diagram, HTML, Metadata, URLs, Paths, Digits)
   │  + Language precedence list (session override → front-matter hint → settings)
   ▼  (Posted via event wake to dedicated STA worker)
RedTilde/SpellService  [COINIT_APARTMENTTHREADED + ProcessWorkLease]
   ├──► ISpellProvider (WinSpellProvider / FakeSpellProvider / [S4] HunspellProvider)
   ├──► Common/CustomDictionary (NFC + lowercase lookup + session Ignore All)
   └──► Bounded 128-entry Suggestion LRU Cache (queried on demand only)
   │
   ▼  (Completion validated against SpellRequestKey on UI Owner Thread)
RedTilde/Application.Spelling.cpp (SpellController)
   ├──► SpellIssueTable (up to 4,096 issues/document; F7 navigation & hit-testing)
   ├──► Active-word typing suppression (300 ms quiet debounce or caret exit)
   ├──► Viewport DecorationProjection (<= 256 decorations, visible +/- 2,048 units)
   │       └──► RichTextEditor::TrySetDecorations(Squiggle, Error, SpellingError)
   ├──► Context Menu (top 5 suggestions, Ignore All, Add to Dictionary)
   ├──► Status Bar chip (active language, "(no dictionary)", "(dictionary unreadable)")
   └──► Settings Drawer (Proofing) & F7 Proofing Review Pane
```

### 2.2 Module specifications

#### A. `Common/SpellProvider.h` — Abstract provider ABI & deterministic test double
- Pure C++ value/interface header with zero Win32 window, DxUi control, or COM header leak into consumers.
- Core types:
  - `enum class ProviderAvailability : std::uint8_t { Ready, NoDictionary, DictionaryUnreadable, Unavailable };`
  - `struct SpellCheckOptions`: `{ bool ignoreUppercase = true; bool ignoreWordsWithDigits = true; bool ignoreInternetAndPaths = true; }`.
  - `struct SpellErrorSpan`: `{ std::size_t start{}; std::size_t length{}; }` (UTF-16 offsets relative to the submitted segment, guaranteed on valid UTF-16 code-point boundaries).
  - `struct ProviderDiagnostic`: sanitized, length-bounded (`<= 256` chars) status message and language tag; control/bidi/format/tag characters stripped before UI exposure.
  - `class ISpellProvider`:
    - `virtual HRESULT EnumerateInstalledLanguages(std::vector<std::wstring>& outTags) noexcept = 0;`
    - `virtual ProviderAvailability CheckAvailability(std::wstring_view languageTag, ProviderDiagnostic* outDiag = nullptr) noexcept = 0;`
    - `virtual HRESULT CheckSegment(std::wstring_view languageTag, std::wstring_view segment, const SpellCheckOptions& options, std::vector<SpellErrorSpan>& outErrors) noexcept = 0;`
    - `virtual HRESULT SuggestWord(std::wstring_view languageTag, std::wstring_view word, std::size_t maxSuggestions, std::vector<std::wstring>& outSuggestions) noexcept = 0;`
- `class FakeSpellProvider final : public ISpellProvider`: deterministic word-set + edit-distance suggestion double controlled in unit/UI/capture tests via explicit in-memory wordlists and simulated availability states (`NoDictionary`, `DictionaryUnreadable`).

#### B. `Common/WinSpellProvider.{h,cpp}` — Windows 11 native primary provider
- Wraps Windows SDK `<spellcheck.h>` via `wil::com_ptr_nothrow` and `DxUi::Failure::InvokeNative`.
- **Worker-thread-only COM ownership**: Instantiates `CLSID_SpellCheckerFactory` lazily on first active check/suggest/enumeration call on the STA worker thread; retains a per-thread cache of up to **8** `ISpellChecker` instances keyed by normalized BCP-47 tag (evicting LRU entries beyond 8).
- **Document pass**: Calls `ISpellChecker::Check` (never `Suggest` during scanning). Maps each `ISpellingError` whose `CORRECTIVE_ACTION` is `CORRECTIVE_ACTION_GET_SUGGESTIONS`, `CORRECTIVE_ACTION_REPLACE`, or `CORRECTIVE_ACTION_DELETE` into a `SpellErrorSpan`; skips `CORRECTIVE_ACTION_NONE`. (Note: autocorrect replacements from `CORRECTIVE_ACTION_REPLACE` may optionally surface first in on-demand `SuggestWord` without auto-mutating typed text silently.)
- **On-demand suggestions**: Calls `ISpellChecker::Suggest` only when requested by context menu or `F7` Proofing pane, capped to `maxSuggestions`.
- **Strict OS isolation**: Never invokes `ISpellChecker::Add`, `ISpellChecker::Ignore`, `ISpellChecker2::Remove`, or `IUserDictionariesRegistrar`; custom additions and session ignores live solely in `Common/CustomDictionary`.

#### C. `Common/CustomDictionary.{h,cpp}` — Portable custom dictionary & session ignore store
- **Default file**: `<dataDir>\custom-dictionary.txt` (UTF-8 without BOM, LF or CRLF preserved on rewrite, created lazily on first `Add to Dictionary`).
- **Freestanding `CUSTOM.DIC` interoperability**:
  - Reads and preserves original encoding across edits: **UTF-8**, **UTF-8 with BOM**, **UTF-16LE with BOM** (Word `CUSTOM.DIC` default on Windows), **UTF-16BE with BOM**, and **BOM-less UTF-16LE** (detected via null-byte distribution check before UTF-8 parsing).
  - Refuses legacy single-byte encodings (e.g., Windows-1252 / invalid UTF-8 / odd UTF-16 byte lengths / unpaired surrogates) with an explicit error and **leaves the file untouched**.
  - Preserves comments, blank lines, non-word lines, and original entry casing verbatim when writing back; appends newly added words cleanly using the file's detected line ending.
- **Unicode normalization & key rules**:
  - Lookup key = Unicode **NFC** (`NormalizeString(NormalizationC, ...)`) + locale-invariant lowercase (`LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, ...)`). Decomposed combining inputs (`e + U+0301`) match composed entries (`é`).
  - Validates single words: trimmed, non-empty, `<= 64` Unicode scalar characters, zero whitespace, zero control chars (`< 0x20`), zero replacement chars (`U+FFFD`).
- **Safety & capacity bounds**:
  - File size cap: **`4 MiB`** (`4,194,304` bytes); word entry cap: **`100,000`** words.
  - Rejects symlinks and reparse points (`FILE_ATTRIBUTE_REPARSE_POINT` / handle check).
  - Persists edits via [Common/Storage.h](../../../Common/Storage.h) `WriteBytesAtomic` (same-directory staging, flush, atomic replace with `FileStamp` check). If disk write fails (e.g., read-only file or conflict), retains the added word in session memory and surfaces a localized non-modal footer notice so the user never loses the session exemption mid-edit.
- **Session ignore sets & generation counter**:
  - `Ignore All`: in-memory NFC-lowercased set; cleared on app exit; never written to disk.
  - `Ignore Once`: occurrence-scoped set keyed by `(workspaceKey, normalizedWord, occurrenceIndexOrSourceRange)` within the current workspace session.
  - Monotonic `uint64_t _dictionaryGeneration`: incremented atomically on every `AddWord`, `RemoveWord`, `IgnoreAll`, or custom dictionary path switch/reload.

#### D. `Common/ProseExtractor.{h,cpp}` — Markdown-aware offset-preserving extractor *(API-5 adapter)*
- **Critical design requirement**: Must **not** split words using `DxUi::RichText::TryVisitSemanticWords`, because DxUi's `IsWordCharacter` excludes `'`, `’`, and `-`, which would break English contractions (`don't`, `won't`) and French elisions/hyphenations (`l'arbre`, `aujourd'hui`, `peut-être`) before the provider sees them.
- **Compose / Split extraction**:
  - Walks `SemanticDocument::Map().Display()` once per revision into a reusable buffer (`proseBuffer`), replacing excluded ranges 1:1 with `U+FFFC` (spaces/sentinels of equal UTF-16 length so every character index in `proseBuffer` equals its display offset in `Display()`):
    - `SemanticElement` roles: `Code`, `Html`, `Metadata`, `Math`, `Diagram`, `Image`.
    - `StyleRun` entries with `DxUi::RichText::Style::Code`.
    - `DisplaySpan` entries where `kind == DisplaySpanKind::Generated || kind == DisplaySpanKind::Object`.
    - Autolink labels (`label == destination` or scheme prefixes `http://`, `https://`, `www.`, `mailto:`).
  - Splits `proseBuffer` only at paragraph/block or `<= 4,096`-UTF-16-unit boundaries aligned to whitespace/sentinel boundaries so checks stay bounded.
  - Applies configured skip filters (`ignoreUppercase` strictly when `any(C1_UPPER) && all(!C1_LOWER)` so caseless scripts like Hebrew, Arabic, Thai, and CJK are never skipped as acronyms; `ignoreWordsWithDigits`; `ignoreInternetAndPaths` for email/URL/Windows & UNC paths).
  - Projects each provider-reported display range `[d0, d1)` back to exact source fragments via `SourceDisplayMap::TrySourceRanges(MarkdownRange{d0, d1}, fragments)`.
- **Source-view extraction**:
  - Uses `MarkdownSyntax` (`Nodes()`, `Inlines()`, `Blocks()`) to collect prose intervals inside `Paragraph`, `Heading`, `Quote`, `ListItem`, and `Table` cells while excluding `Code` blocks, inline `Code`, `Math`, `Html`, `Metadata`/`FrontMatter`, link/image destinations (`destination`, `title`), and `Diagnostic` non-prose spans. Maps straight to raw source offsets.
- **Front-matter language hint**:
  - Scans only `MarkdownBlockKind::FrontMatter` (`Yaml` or `Toml`) for top-level `lang` / `language` scalar keys (`<= 32` ASCII `[A-Za-z0-9-]`), read-only, with zero external parser allocation.
- *Adapter isolation note*: `ProseExtractor` will be largely replaced once `DX-SPELL-1` and `DX-SPELL-6` land upstream in DxUi.

#### E. `RedTilde/SpellService.{h,cpp}` — Dedicated STA COM background worker
- **Threading model**: One dedicated background worker thread initialized with `CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)` (explicit **STA**) holding one [ProcessWorkLease](../../../Common/ProcessWork.h) so `ISpellCheckerFactory`/`ISpellChecker` never incur cross-apartment `MTA <-> STA` proxy overhead and never contend on locks.
- **Explicit phase machine**:
  - `enum class WorkerPhase : std::uint8_t { Idle, Initializing, Checking, Closing, Stopped };`
  - Uses manual-reset `_stopEvent` and auto/manual `_workEvent` (`WaitForMultipleObjects`) — **zero polling timers**.
- **Lazy startup**: Worker thread is created on demand only when `spelling.enabled == true` and a non-empty prose job is enqueued; if `spelling.enabled == false` or all open documents are empty/code-only, `WinSpellProvider` never initializes COM spellcheckers.
- **Coalesced latest-request queue & cancellation**:
  - Keeps only **one** pending check request slot (coalescing rapid edits cleanly with a `300 ms` quiet-typing window before dispatching full-document check on the worker).
  - Every request and result carries `SpellRequestKey`:
    - `std::uint64_t workspaceKey` (stable document identity in `DocumentSessionMetadata`),
    - `DxUi::RichText::SourceSnapshot sourceSnapshot`,
    - `std::uint64_t semanticIdentity` (0 in Source view),
    - `std::uint64_t dictionaryGeneration`,
    - `std::uint64_t languageConfigVersion`.
  - Worker aborts mid-document between `<= 4,096`-unit chunks if `_stopEvent` signals or a newer request key supersedes the current job.
- **On-demand suggestion queue & cache**:
  - Separate priority slot for on-demand `SuggestWord` requests (context menu / `F7` active issue) backed by a bounded **128-entry LRU cache** keyed by `(languageTag, normalizedWord, dictionaryGeneration)`.
- **Shutdown drain**:
  - Registered alongside `SettingsWatcher`, `RecoveryService`, `AsyncFiles`, and `BitmapPasteWriter`: `BeginClose()` / `WM_ENDSESSION(TRUE)` signals `_stopEvent`, detaches UI completion posting, and `AreShutdownWorkersSettled()` verifies `SpellService::Finished()` without blocking the UI thread indefinitely.

#### F. `RedTilde/Application.Spelling.cpp` — Controller, `SpellIssueTable`, & Viewport Projection
- **`SpellIssueTable`** (stored in `DerivedDocumentCaches` / swapped atomically in `SwitchWorkspaceDocument()`):
  - Bounded to at most **`4,096` issues** per `65,536`-unit document, sorted by `(displayStart, displayEnd)`.
  - Each entry stores `{ displayRange, sourceFragments (inline small vector), normalizedKey, rawWord, primaryLanguageIndex }`.
- **Viewport `DecorationProjection`** *(isolated API-5 adapter slated for removal under `DX-SPELL-2`)*:
  - Filters `SpellIssueTable` via binary search to `[visible.first - 2048, visible.last + 2048)` (or Source-view visible source range), excludes any issue overlapping the active suppressed typing word or matching `Ignore Once`/`Ignore All`/`CustomDictionary`, and emits up to **`256` `RichTextDecoration`** structs (`Kind::Squiggle`, `Role::Error`, `Annotation::SpellingError`).
  - Calls `_editor->TrySetDecorations(expectedSource, projected)`. If `TrySetDecorations` returns `E_BOUNDS` (rectangle cap) or `GetDecorationStatus() == RichTextDecorationStatus::CapacityExceeded`, halves the emitted window size (`256 -> 128 -> 64`, visible viewport first) once without looping.
  - On `SetOnScrollChanged(...)`, re-runs `ProjectVisibleDecorations()` synchronously in `O(log N)` from `SpellIssueTable` without touching `SpellService`.
- **Atomic suggestion replacement** *(isolated API-5 adapter slated for simplification under `DX-SPELL-5`)*:
  - Verifies `(workspaceKey, sourceSnapshot, revision, selection, controlLifetime)` still match; validates every source fragment against `source.IsBoundary`; builds a single ascending disjoint `SourceEdit` array placing `replacement` in `fragments[0]` and `L""` in `fragments[1..N-1]` and submits via `_editor->ApplySourceEdits(...)` as one undoable transaction.

---

## 3. Behavioral rules and edge-case specifications

1. **Language resolution precedence**:
   - **(1) Session-local document override** (stored in `DocumentSessionMetadata`, never modifies file on disk or settings).
   - **(2) Document front-matter hint** (`lang:` / `language:` scalar in `MarkdownBlockKind::FrontMatter`, validated ASCII BCP-47 tag `1..32` chars `[A-Za-z0-9-]`).
   - **(3) Persisted `spelling.languages`** (default `["auto"]`, resolved via `GetUserDefaultLocaleName`, fallback `"en-US"`).
2. **Multi-language checking (`1..3` languages)**:
   - A word is **valid** if **any** active provider (`languages[0..N-1]`), `Ignore All`, `Ignore Once` (for that occurrence), or `CustomDictionary` accepts it.
   - To avoid `N×` full-document work, `SpellService` checks prose segments against `languages[0]` first, then checks **only the flagged error words** from `languages[0]` against `languages[1]`, and so on. Suggestions merge unique candidates across active `Ready` languages, preserving primary-language priority.
3. **Truthful missing/unreadable dictionary state**:
   - If a configured language has `ProviderAvailability::NoDictionary` or `DictionaryUnreadable`, **never** fall back silently to English (`en-US`). If **all** active languages lack a usable dictionary, emit **0** squiggles and display `(<Language> — no dictionary)` or `(<Language> — dictionary unreadable)` in the status bar with sanitized tooltip/UIA details.
4. **Editor view policy**:

   | View / Surface | Checking Scope | Decoration Output |
   | --- | --- | --- |
   | **Compose** | `SemanticDocument` prose via `ProseExtractor` | Primary `_editor` (`<= 256` viewport squiggles + `SpellingError` UIA) |
   | **Split (left rich editor)** | Same as Compose | Primary `_editor` only |
   | **Split (right `_mirror`)** | Not checked separately | **Zero decorations** (no duplicate squiggles) |
   | **Source** | `MarkdownSyntax`-filtered prose ranges | Primary `_editor` raw source squiggles |
   | **Focus mode** | Same as active editor view | Active editor squiggles preserved; status pill stays quiet unless tapped |
   | **Print Preview / Print / PDF** | Never checked | **Zero decorations** (`TryDrawPreparedPage` omits transient decorations) |

5. **Typing suppression, IME, and `F2` local source guards**:
   - Suppress squiggles overlapping the word currently touching a collapsed caret while editing; clear suppression `300 ms` after typing stops or immediately when the caret moves to a different word.
   - Suspend check publication and refuse spelling edits while native TSF/IME composition is active or while a `BlockDraftModal` (`F2`) is open.
   - Do not check prose inside opaque F2 `Code`, `Diagram`, `Math`, `Html`, or `Metadata` source regions.
6. **Hostile-input & metadata sanitization**:
   - Strip all C0/C1 control chars, directional overrides (`U+061C`, `U+200E..U+200F`, `U+202A..U+202E`, `U+2066..U+2069`), zero-width chars (`U+200B..U+200D`, `U+FEFF`, `U+2060`), and tag chars (`U+E0000..U+E007F`) from external locale names, dictionary paths, and error strings before UI display; clamp display lengths (`<= 48` chars in status chip, `<= 256` chars in tooltips).

*(Provenance note: The nine edge-case proofing invariants enforced across Sections 2–4 trace directly to the `storytold/wordcraft` primary-source analysis documented in [Annex B.1](#b1-storytoldwordcraft-v0181--storytoldcraft-libs-craft-proof-243d68e1).)*

---

## 4. Settings schema, UI, accessibility, and localization

### 4.1 JSON5 settings schema (`RedTilde.settings.json` / `SettingsStore.schema.json`)

Follows RedTilde's `yyjson` JSON5 contract ([Core_Settings.md](../../Core/Core_Settings.md)): per-field fallback on invalid entries, document-tree merge preserving unknown keys and newer integral `schema` versions, and non-writing live reload via `SettingsWatcher`.

```json5
{
  spelling: {
    enabled: true,                     // boolean (default true)
    languages: ["auto"],               // 1..3 BCP-47 tags (e.g. ["en-US", "fr-FR"]) or ["auto"]
    ignoreUppercase: true,             // skip short all-caps tokens in cased scripts
    ignoreWordsWithDigits: true,       // skip tokens containing digits (e.g. B2B, v1.0, C23)
    ignoreInternetAndPaths: true,      // skip URLs, emails, Windows/UNC file paths
    customDictionaryPath: "",          // optional freestanding CUSTOM.DIC path; empty uses app default custom-dictionary.txt
    dictionariesFolder: "",            // optional S4 local .aff/.dic directory
    engine: "auto"                     // "auto" | "windows" (S4 adds "hunspell")
  }
}
```

- **`languages`**: Array of `1..3` strings, each `1..32` ASCII `[A-Za-z0-9-]`. Invalid elements are dropped individually; an empty surviving array falls back to `["auto"]`.
- **`customDictionaryPath` / `dictionariesFolder`**: Validated identically to `filesRoot` / `lastDocument` (valid UTF-16, `<= 32,767` code units, no control chars `< 0x20`, absolute Windows path when non-empty).
- **`engine`**: `"auto"` or `"windows"` (`"hunspell"` admitted in `S4`); invalid values fall back to `"auto"`.

### 4.2 UI, accessibility, and resources (`res/RedTilde.rc`)

Aligned with [UI_DesignSystem.md](../../UI/UI_DesignSystem.md) — keeps the writing surface dominant with zero permanent proofing ribbon:

1. **In-document squiggles & UIA**: `RichTextDecorationKind::Squiggle` + `RichTextDecorationRole::Error` (theme error/scarlet ink in Light/Paper/Dark; high-contrast system ink override) + `RichTextAnnotationKind::SpellingError` (`AnnotationType_SpellingError` and `ITextRangeProvider2::RangeFromAnnotation`).
2. **Context menu (`Right-click` / `Shift+F10` / Menu key)**:
   - When the clicked/caret offset hits a `SpellIssue`:
     - Top section: up to **5** suggested replacements (or one disabled localized **"No suggestions"** entry).
     - Separator -> **Ignore All** (session-only) -> **Add to Dictionary** (persisted + immediate squiggle retirement).
     - Separator -> ordinary RedTilde editing/formatting items (`Cut`, `Copy`, `Paste`, `Bold`, `Italic`, `Code`, `Local source`, etc.).
   - Replacement execution revalidates `(workspaceKey, sourceSnapshot, selection, controlLifetime)` across the async menu callback, matching `Command::OpenLink` in [Application.cpp](../../../RedTilde/Application.cpp).
   - *Keyboard invocation / context-menu UX note*: Synchronize suggestions cleanly for keyboard (`Shift+F10`) invocation as well as pointer right-click (`WinSpellProvider::SuggestWord` runs on STA worker or fast bounded cache lookup).
3. **Status bar indicator**:
   - Compact button/label next to word/character counts showing active proofing language (e.g., `English (US)` or `EN / FR`) or `(no dictionary)` / `(dictionary unreadable)` state; tooltip/UIA HelpText shows sanitized details. Clicking opens quick language picker or scrolls Settings drawer to **Proofing**.
4. **Settings drawer — Proofing section**:
   - **Check spelling while typing** toggle (`spelling.enabled`).
   - **Languages** selector (choose `Auto (Windows)` or `1..3` languages from OS-installed language packs + status badges).
   - **Ignore rules**: **Ignore words in UPPERCASE**, **Ignore words with numbers**, **Ignore internet addresses and file paths**.
   - **Custom dictionary**: displays active path (`custom-dictionary.txt` or chosen `CUSTOM.DIC`), **Choose file…** / **Use default**, and **Manage custom words** list (add/remove normalized entries).
5. **`F7` Proofing review pane (`Slice S3`)**:
   - Non-modal right panel integrated under `OverlayKind` mutual exclusion (`SetActiveOverlay`): displays sentence context, up to **8** suggestions, **Ignore Once**, **Ignore All**, **Add to Dictionary**, **Previous / Next Issue**, **Change**, and **Check Again**, strictly guarded against stale document revisions.
6. **Localization**: All new labels, menu items, status strings, tooltips, errors, and UIA names live in UTF-8 `STRINGTABLE` entries in [res/RedTilde.rc](../../../res/RedTilde.rc) using positional `{0}` placeholders ([Common/Resources.h](../../../Common/Resources.h)).

---

## 5. Phased implementation plan (`S1`–`S4`)

| Slice | Scope & Deliverables | Required Checks |
| --- | --- | --- |
| **Slice S1 — Core Abstraction, Extractor, Custom Dictionary & Settings** | 1. Implement `Common/SpellProvider.h` (`ISpellProvider`, `FakeSpellProvider`).<br>2. Implement `Common/CustomDictionary.{h,cpp}` (`custom-dictionary.txt` + `CUSTOM.DIC` UTF-8/UTF-16LE/BE codecs, NFC+lowercase normalization, verbatim comment/casing preservation, `4 MiB`/`100,000`/`64` caps, symlink refusal, atomic `Storage.h` saves, `Ignore All` / `Ignore Once`).<br>3. Implement `Common/ProseExtractor.{h,cpp}` (Compose/Split `SemanticDocument` + Source `MarkdownSyntax` extraction, exclusions, skip filters, `SourceDisplayMap::TrySourceRanges` back-projection, front-matter `lang:` reader).<br>4. Extend `Common/Settings.{h,cpp}` & [SettingsStore.schema.json](../../../SettingsStore.schema.json) with `spelling` + live reload.<br>5. Register unit suites (`Tests/Application.Tests.Spelling.cpp`) in `NativeInventory.json` / `native-case-catalog.json`. | Headless x64 Debug/Release/ASan unit suites (`FakeSpellProvider` + `CustomDictionary` + `ProseExtractor` + `Settings`), `validate.ps1`, `format.ps1 -Check`, `Tools/Test-Changelog.ps1` |
| **Slice S2 — Windows Provider, STA `SpellService`, Squiggles, Context Menu & UI** | 1. Implement `Common/WinSpellProvider.{h,cpp}` (`<spellcheck.h>`, lazy STA COM cache, read-only OS dictionary, `Check` + `Suggest`, sanitized diagnostics).<br>2. Implement `RedTilde/SpellService.{h,cpp}` (STA thread, `ProcessWorkLease`, `WorkerPhase`, `300 ms` quiet debounce, `128`-entry LRU suggestion cache, `SpellRequestKey` cancellation, `AreShutdownWorkersSettled()` integration).<br>3. Implement `RedTilde/Application.Spelling.cpp`: `SpellIssueTable` (`<= 4,096`), viewport `DecorationProjection` (`<= 256` + scroll re-projection + `E_BOUNDS` halving), active-word typing suppression, IME/`F2` draft guards, atomic mid-word `ApplySourceEdits`, context-menu hit-testing & suggestions, status bar chip, Settings drawer **Proofing** UI, UTF-8 `STRINGTABLE` strings.<br>4. Add hidden `--capture` scenarios (`spelling-light`, `spelling-dark`, `spelling-high-contrast`). | All S1 checks + UI/context-menu/cancellation/`--capture` suites across x64 Debug/Release/ASan + read-only real `WinSpellProvider` OS probe + 6-profile zero-warning compilation (`x64` & `ARM64`) |
| **Slice S3 — `F7` Proofing Pane, Multi-Language & Release Qualification** | 1. Add non-modal `F7` Proofing review pane (`OverlayKind` + `spelling-proofing-light` capture) with sentence context, `Ignore Once`/`Ignore All`/`Add to Dictionary`, `Change`, `Previous`/`Next`, `Check Again`.<br>2. Enable `1..3` iterative multi-language checking + YAML `lang:` hint UI surfacing.<br>3. Update domain specs (`Core_Architecture.md`, `Core_Settings.md`, `Editor_Markdown.md`, `UI_DesignSystem.md`, `Core_PerformanceAndResources.md`, `Testing_Validation.md`), `capabilities.json`, and `docs/README.md`.<br>4. Run paired Release performance & quiet-host resource probes and full qualification. | Paired Release performance packet (`performance-paired.ps1`, `idleProcessCpu100ns == 0`), full x64 + ARM64 6-profile runtime/ASan qualification, and owner-agreed interactive desktop & Narrator UIA `SpellingError` acceptance |
| **Slice S4 — Optional Portable Hunspell & MyThes Lookup (Deferred / Opt-In)** | Only if requested later: implement `Common/HunspellProvider.{h,cpp}` (`hunspell 1.7.5` dynamic DLL via `vcpkg` or clean-room reader) reading local user folders (`dictionariesFolder`, `%LOCALAPPDATA%\RedTilde\dictionaries`, `%LOCALAPPDATA%\hunspell`, LibreOffice `dict-*`), plus optional local MyThes `.idx`/`.dat` reader; update `THIRD-PARTY-NOTICES.md` and `package.ps1`. | Independent Hunspell & hostile-`.aff`/`.dic` tests, package ZIP license smoke check, updated perf baseline |

---

## 6. Planned upstream `DxUi` improvements to simplify RedTilde

While `S1`–`S3` ship cleanly against the pinned `DxUi` API-5 baseline (`0c371a61`), building RedTilde against the current SDK forces RedTilde to maintain ~5 non-trivial adapter layers in application code (`ProseExtractor` semantic/syntax walking, viewport `<= 256` decoration slicing, scroll re-projection, screen-to-source context hit-testing, mid-word Markdown splice calculation, and front-matter scanning).

**Boundary rule**: Upstream `DxUi` MUST remain pure text, Markdown, decoration, layout, input, and UIA infrastructure — **zero dictionaries, zero COM spellcheckers, zero user settings, zero network I/O**. The six upstream `DxUi` additions below belong canonically in `DxUi` so every Markdown/text consumer (`RedTilde`, gallery controls, future editors) gets simpler, faster, single-source behavior.

### `DX-SPELL-1` — Canonical prose-segment visitors (`TryVisitSemanticProseSegments` + `TryVisitSyntaxProseRanges`)

- **Current friction in pinned `DxUi`**: `RichSemanticDocument.cpp` (`TryVisitSemanticWords`) already computes excluded semantic intervals (`Code`, `Html`, `Metadata`, `Math`, `Diagram`, `Image`, `Style::Code`, `Generated`/`Object` spans, autolinks, bare URLs), but then slices those segments using `IsWordCharacter` (`DxUi.SingleLineTextEditing.cpp`), which excludes `'`, `’`, and `-` — splitting contractions (`don't`) and elisions (`l'arbre`, `aujourd'hui`). Furthermore, Source mode has no equivalent visitor over `MarkdownSyntax`.
- **Proposed `DxUi` improvement**:
  - In `include/DxUi/RichSemanticDocument.h`: add `TryVisitSemanticProseSegments(const SemanticDocument& document, MarkdownRange display, SemanticProseVisitor visitor, void* context)` invoking the synchronous callback for each **contiguous unexcluded prose segment** `(std::wstring_view displayText, MarkdownRange displayRange)` rather than splitting sub-words. Keep `TryVisitSemanticWords` / `CountWords` intact for word counting.
  - In `include/DxUi/MarkdownSyntax.h`: add `TryVisitSyntaxProseRanges(const MarkdownSyntax& syntax, SyntaxProseVisitor visitor, void* context)` yielding raw source prose intervals for Source mode while excluding fences, inline code, math, HTML, metadata, and link/image URLs.
- **Simplifies RedTilde**: Deletes ~80% of `Common/ProseExtractor` (all duplicated `SemanticElement`/`StyleRun`/`DisplaySpan` exclusion-merging logic and all `MarkdownSyntax` Source-mode block/inline walkers).

### `DX-SPELL-2` — Layered decoration channels (`RichTextDecorationLayer`) + viewport-clipped geometry preparation

- **Current friction in pinned `DxUi`**: `RichTextEditor::_state->decorations` (`DxUi.RichTextEditor.cpp`) is a single flat `std::vector<RichTextDecoration>` capped at **256** entries and **4,096** total rectangles across the entire document, and `TrySetDecorations` maps **all** rectangles eagerly regardless of whether they sit 20 pages off-screen.
- **Proposed `DxUi` improvement**:
  - Add a layer tag `enum class RichTextDecorationLayer : uint8_t { Consumer, Spelling, Grammar, Diagnostic }` and `TrySetDecorations(expectedSource, layer, decorations)` so independent owners (Find/Consumer, Spelling, Grammar, Markdown Diagnostics) never overwrite or collide with each other.
  - Raise logical storage per layer to `4,096` `RichTextDecoration` entries (matching RedTilde's document bound) while **lazily mapping D2D geometry rectangles only for the visible viewport + bounded overscan** during `PrepareContent`, preserving the `4,096`-rectangle paint budget.
  - Diff UIA annotation peers per layer (`SpellingError` vs `GrammarError`) rather than retiring all accessibility peers when unrelated visual decorations change.
- **Simplifies RedTilde**: Deletes RedTilde's viewport `DecorationProjection` slicer, `[visible.first - 2048, visible.last + 2048)` window math, `SetOnScrollChanged` decoration re-projection hook, and `E_BOUNDS` halving backoff loop. `SpellController` simply pushes the document's `SpellIssueTable` once per checked revision.

### `DX-SPELL-3` — Rich context-menu payload (`SetOnRichContextMenu` / `RichTextContextMenuRequest`)

- **Current friction in pinned `DxUi`**: `RichTextEditor::OnContextMenu` (`DxUi.RichTextEditor.cpp:6970`) already calls `HitSource(host, point)` internally before invoking `Control::OnContextMenu`, yet `SetOnContextMenu` forwards only `(POINT screenPoint, bool keyboardInvocation)`, forcing RedTilde to convert coordinates back to host DIPs and call `TryHitTestTextInputPoint` a second time (which also diverges slightly between pointer click and keyboard caret invocation).
- **Proposed `DxUi` improvement**: Add an additive overload `SetOnRichContextMenu(std::function<void(const RichTextContextMenuRequest&)> callback)` where `RichTextContextMenuRequest` delivers `{ POINT screenPoint; bool keyboardInvocation; RichText::SourceSnapshot source; RichText::Selection selection; std::optional<size_t> hitSourceOffset; }`. Preserve existing `SetOnContextMenu` intact for binary/source compatibility.
- **Simplifies RedTilde**: Deletes coordinate round-tripping and redundant hit-test code in `Application.Spelling.cpp`; one canonical offset handles both right-click and `Shift+F10`.

### `DX-SPELL-4` — Lightweight per-layer active-range suppression (`SetDecorationSuppressionRange`)

- **Current friction in pinned `DxUi`**: Hiding a squiggle on the single word the user is currently typing requires cloning and re-calling `TrySetDecorations`, triggering overlay dirtiness, UIA peer retirement (`RetireSemanticAccessibility()`), and re-layout churn once the user types or leaves the word.
- **Proposed `DxUi` improvement**: Add `void SetDecorationSuppressionRange(RichTextDecorationLayer layer, std::optional<RichText::MarkdownRange> sourceRange) noexcept` in `RichTextEditor` so the paint/UIA filter skips any decoration overlapping `sourceRange` in `O(1)` without mutating stored decorations or rebuilding rectangles.
- **Simplifies RedTilde**: Eliminates decoration-list rebuilds on every caret movement / `300 ms` typing-debounce expiration.

### `DX-SPELL-5` — Semantic display-range replacement planner (`TryPlanDisplayRangeReplacement`)

- **Current friction in pinned `DxUi`**: When a misspelled word crosses inline Markdown delimiters (e.g., `**re**commnd`), `SourceDisplayMap::TrySourceRanges` returns disjoint source fragments `[r0, r1]`. Every Markdown consumer implementing spell-replace or synonym-replace has to invent identical `SourceEdit` multi-fragment replacement logic.
- **Proposed `DxUi` improvement**: Add a pure helper `TryPlanDisplayRangeReplacement(const SemanticDocument& document, MarkdownRange displayRange, std::wstring_view replacementText, std::vector<SourceEdit>& outEdits)` in `RichSemantic.h` / `RichTextCommands.h` producing minimal, lossless, boundary-safe `SourceEdit` spans ready for `ApplySourceEdits`.
- **Simplifies RedTilde**: Deletes RedTilde's custom multi-fragment Markdown splicing logic.

### `DX-SPELL-6` — Read-only front-matter scalar query (`MarkdownSyntax::TryGetFrontMatterScalar`)

- **Current friction in pinned `DxUi`**: `MarkdownParser` already parses and validates leading YAML 1.2.2 (`libfyaml`) and TOML 1.0.0 (`toml++`) metadata envelopes (`MarkdownBlockKind::FrontMatter`), but exposes only raw source slices and validation diagnostics (`MarkdownDiagnostic`).
- **Proposed `DxUi` improvement**: Add `std::optional<std::wstring> MarkdownSyntax::TryGetFrontMatterScalar(std::wstring_view key) const` (or `std::optional<std::wstring_view>`) returning top-level string scalars (such as `lang` or `language`) when a valid front-matter envelope is present.
- **Simplifies RedTilde**: Deletes RedTilde's custom YAML/TOML line scanner in `Common/ProseExtractor`, reusing `libfyaml`/`toml++` parsing already compiled into `DxUi.lib`.

### RedTilde Simplification & Producer-First Adoption Map

| Upstream Item | Current RedTilde API-5 Adapter (`S1`–`S3`) | RedTilde Code Deleted After Pin Adoption |
| --- | --- | --- |
| **`DX-SPELL-1`** | `Common/ProseExtractor` full exclusion-interval & Source-AST walkers | ~80% of `ProseExtractor` (keeps only skip filters like `ignoreUppercase`) |
| **`DX-SPELL-2`** | `DecorationProjection` viewport windowing, scroll re-projection, `E_BOUNDS` halving | All viewport slicing, scroll-hook re-projection, and backoff state in `Application.Spelling.cpp` |
| **`DX-SPELL-3`** | Screen-to-DIP conversion + `TryHitTestTextInputPoint` on menu open | Replaced by direct `request.hitSourceOffset` |
| **`DX-SPELL-4`** | Re-submitting filtered decorations on typing-debounce expiry / caret move | Replaced by single `SetDecorationSuppressionRange` call |
| **`DX-SPELL-5`** | Custom multi-fragment `SourceEdit` builder for split inline formatting | Replaced by `TryPlanDisplayRangeReplacement` |
| **`DX-SPELL-6`** | Custom YAML/TOML front-matter `lang:` scanner | Replaced by `syntax->TryGetFrontMatterScalar(L"lang")` |

**Sequencing rule (producer-first)**: None of `DX-SPELL-1`–`DX-SPELL-6` block RedTilde `S1`–`S3`. When scheduled, each upstream change is implemented, tested, and qualified in `DxUi` first (new `apiRevision` if ABI changes), then adopted in RedTilde via an explicit [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json) bump PR that simultaneously deletes the superseded RedTilde adapter code.

---

## 7. Verification and qualification gates

1. **Deterministic Unit & Behavior Suite (`FakeSpellProvider`, zero OS language-pack dependency)**:
   - English contractions (`don't`, `won't`, `it's`), French elisions & accents (`l'arbre`, `aujourd'hui`, `peut-être`), NFC/NFD equivalence (`e + U+0301` vs `é`), supplementary Unicode scalars adjacent to prose.
   - Markdown exclusion fidelity: inline code, fenced blocks, inline/display LaTeX math, Mermaid diagrams, HTML blocks, metadata envelopes, autolinks/bare URLs, Windows/UNC file paths, cased short acronyms (`NASA`), alphanumeric tokens (`C23`, `v1.0`), and caseless scripts (never skipped as acronyms).
   - Mid-word inline formatting (`**re**commnd`): squiggles on each split fragment; single atomic `ApplySourceEdits` replacement + single `Ctrl+Z` restoring exact source, formatting, selection, and dirty state.
   - Capacity & viewport projection: `500+` misspelled words retained in `SpellIssueTable`; at most `256` `RichTextDecoration` items projected per viewport; scrolling updates squiggles without calling `ISpellProvider::CheckSegment`; `E_BOUNDS` backoff tested cleanly.
   - `CustomDictionary` file safety: UTF-8, UTF-8 BOM, UTF-16LE BOM (`CUSTOM.DIC`), UTF-16BE BOM, and BOM-less UTF-16LE round-trips preserving comments/EOLs/casing verbatim; legacy ANSI refused untouched; `> 4 MiB`, `> 100,000` words, `> 64` chars, and symlinks/reparse points refused fail-closed; atomic saves under `--data-dir`; failed disk save retains session words.
   - Cancellation, typing suppression & lifetime safety: stale worker completions across rapid edits, `ExchangeDocumentState` workspace switching, or `Close` discarded cleanly; active typing word unsquiggled until `300 ms` quiet or caret exit; IME preview and `F2` block draft suppress/refuse premature replacements; `SpellService` drains cleanly under `ProcessWork::Wait` and `WM_ENDSESSION`.
2. **Real Windows API Probe (`WinSpellProvider`)**:
   - Directed non-mutating test verifying `CoCreateInstance(CLSID_SpellCheckerFactory)` on STA thread, `IsSupported(L"en-US")`, `Check` offsets on known typo vs clean prose, `Suggest` candidate, missing-locale graceful `NoDictionary` status, and **zero writes** to `%APPDATA%\Microsoft\Spelling`.
3. **Visual Harness Captures (`--capture`)**:
   - Non-activating captures in `spelling-light`, `spelling-dark`, `spelling-high-contrast`, and (`S3`) `spelling-proofing-light` using synthetic fixtures, preserving build/DPI/SDK provenance.
4. **Performance & Resource Qualification ([Core_PerformanceAndResources.md](../../Core/Core_PerformanceAndResources.md))**:
   - Paired Release study (`performance-paired.ps1`) against pre-feature baseline on identical fixture proving `idleProcessCpu100ns == 0`, `0 B` spellcheck overhead when disabled or on empty document, bounded private memory delta when active, and zero statistically resolved typing-frame regression.
5. **Toolchain, Platform & Interactive Acceptance**:
   - Zero `/Wall` warnings across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` builds, full x64 & ARM64 runtime execution, real ASan detection probe, and owner-agreed interactive desktop test validating right-click / `Shift+F10` keyboard navigation, focus restoration, and Narrator / `Inspect.exe` `AnnotationType_SpellingError` announcements.

---

## Annex A — Alternative architectures evaluated and not selected *(Reference Only)*

> **Non-plan reference section.** The options below were evaluated during architecture selection and **rejected** in favor of Option E. They are retained here strictly as decision rationale.

| Option | Summary | Footprint & Build Impact | Licensing | Reason Not Selected |
| --- | --- | --- | --- | --- |
| **A. Pure Windows Spellcheck (no `ISpellProvider` seam)** | Direct `<spellcheck.h>` calls embedded inside `SpellService` without an abstract provider interface | `+0 KB` binary, `0 B` cold RAM | MIT + OS SDK | Prevents deterministic CI testing when a Windows VM lacks non-English packs (`fr-FR`, `de-DE`) and blocks future optional local `.aff`/`.dic` support. |
| **B. Pure Hunspell (`vcpkg` + `.aff`/`.dic`)** | `hunspell 1.7.5` (`vcpkg` baseline `aeb162b7`) + `libiconv` as sole spellchecker | `+2–4 MB` DLLs, `+10–30 MiB` private heap/dict, heavier 6-profile builds | `MPL-1.1 OR LGPL-2.1-or-later OR GPL-2.0-or-later` (`libiconv` `LGPL-2.1`) | Forces every Windows user to obtain/configure external `.aff`/`.dic` files (or forces RedTilde to bundle multi-license dictionaries), adds memory/build cost even when Windows already includes dictionaries. |
| **C. Pure Nuspell + ICU4C (`vcpkg`)** | `nuspell 5.1.9` (`vcpkg` baseline `aeb162b7`) + `icu 78.3` | `+30+ MB` ICU data + heavy 6-profile ARM64/ASan compile cost | **`LGPL-3.0-or-later`** | **Disqualified**: `LGPL-3.0-or-later` conflicts with MIT static linking without relinking compliance, and ICU4C adds prohibitive distribution and build overhead. |
| **D. Clean-Room C++ Hunspell / SymSpell** | Custom C++ `.aff`/`.dic` parser or SymSpell wordlist engine | Minimal binary if SymSpell; complex parser state | 100% MIT | SymSpell lacks affix morphology and compounding for European/agglutinative languages; writing and security-hardening a full clean-room C++ `.aff`/`.dic` + compound engine duplicates OS functionality already present on Windows 11. |

---

## Annex B — Open-source reference study *(Reference Only)*

> **Non-plan reference section.** Findings below summarize primary-source research across `storytold/wordcraft` and related open-source proofing systems that informed Option E's design safeguards and edge-case rules.

### B.1 `storytold/wordcraft` (`v0.18.1`) & `storytold/craft-libs` (`craft-proof`, `243d68e1`)

- **Core design**: WordCraft uses `craft-proof`, a pure-Rust `#![forbid(unsafe_code)]` clean-room Hunspell `.aff`/`.dic` + compounding engine (`crates/proof/src/affix/`), compiled across Windows, macOS, Linux, and WebAssembly (`wasm32-unknown-unknown`).
- **Dictionary sourcing**:
  - English embeds SCOWL `rel-2026.02.25` (`~2.05 MB` for `en-US`, `en-GB`, `en-CA`, `en-AU` + `en-extra.txt` + dialect variants) with a public-domain Moby Hyphenator II fallback (`data/en-words.dic`, `344 KB` deflated).
  - Non-English uses SHA-256-verified downloadable `language-packs-2` (75 languages, `256 MB` LRU memory cap, explicit download consent) plus local/LibreOffice Hunspell folders (`MAX_FILE = 64 MiB` read cap, `SET` encodings `ISO8859-1`/`-2`/`-9`, `KOI8-R`, `Windows-1251`).
- **Custom dictionary (`crates/engine/src/custom_dict.rs`, `#5840`, `#5899`)**: Supports UTF-8, UTF-8 BOM, UTF-16LE BOM (`CUSTOM.DIC`), UTF-16BE BOM, and BOM-less UTF-16LE; refuses ANSI untouched; preserves comments/EOLs/casing verbatim; caps at `4 MB` / `100,000` words / `64` chars; normalizes keys via lowercase + Unicode NFC (`#3562`).
- **Layout & review (`crates/layout/src/para/proof.rs`, `crates/ui-egui/src/proofing_pane.rs`, `docs/spelling-review.md`)**: Offset-preserving sentinel masking (`proof_text()`), active-typing-word squiggle suppression, script-aware `ignore_uppercase` (`<= 6` chars), `F7` right-side Proofing pane guarded by `(document_id, ime_body_epoch, selection)`.
- **Thesaurus (`crates/engine/src/cmd/review/proofing.rs`)**: Static 29-word English lookup stub (not a full lexical database).
- **Why RedTilde diverges on engine choice while adopting WordCraft's edge-case rules**: WordCraft needed a custom cross-platform/WASM Rust engine and remote pack store because Linux, macOS, and browsers lack a unified OS proofing API; RedTilde targets Windows 11 exclusively (`x64` & `ARM64`) with zero unsolicited network I/O, making Windows `<spellcheck.h>` superior as the primary backend while adopting WordCraft's exact edge-case proofing invariants below.

#### Nine WordCraft proofing invariants adopted verbatim in Option E

1. **Zero silent English fallback for unsupported languages** *(Sections 2.2.A, 3.3, 4.2)*: If the user configures a language whose dictionary is unavailable, report `(no dictionary)` / `(dictionary unreadable)` and emit zero squiggles rather than marking foreign prose wrong against English.
2. **Active-word typing suppression** *(Sections 2.2.F, 3.5, 6 `DX-SPELL-4`)*: Never paint a red squiggle under the word currently touching the caret while actively typing; surface it after caret exit or a short quiet debounce (`300 ms`).
3. **Offset-preserving prose masking** *(Sections 2.2.D, 6 `DX-SPELL-1`)*: Replace non-prose objects and inline decorations with neutral sentinels of identical UTF-16 length so spell-checker ranges align cleanly without drift.
4. **Microsoft Word `CUSTOM.DIC` interoperability** *(Section 2.2.C)*: Read/write UTF-16LE BOM, UTF-16BE BOM, UTF-8 BOM, and UTF-8 (plus BOM-less UTF-16LE detection); preserve encoding, EOLs, comments, and original casing verbatim; refuse legacy ANSI/Windows-1252 cleanly without overwriting; refuse symlinks/reparse points; enforce `4 MiB` / `100,000` words / `64`-char word caps.
5. **NFC + case-folded user dictionary keys** *(Section 2.2.C)*: Normalize user/ignored entries with `NormalizeString(NormalizationC, ...)` + `LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, ...)` so NFD decomposed accents match NFC document text (`#3562`).
6. **Script-aware `ignoreUppercase`** *(Section 2.2.D)*: Only treat words as acronyms when they contain cased uppercase letters and no lowercase letters (`C1_UPPER` present, `C1_LOWER` absent), never skipping caseless scripts (Hebrew, Arabic, Thai, CJK, etc.).
7. **Strict `Ignore Once` vs `Ignore All` vs `Add to Dictionary`** *(Sections 2.2.C, 4.2)*: `Ignore Once` tracks occurrence identity; `Ignore All` lives strictly in session memory; only `Add to Dictionary` writes to disk.
8. **Revision/selection-guarded corrections** *(Sections 2.2.F, 4.2)*: Revalidate `(workspaceKey, sourceSnapshot, selection)` across async or menu boundaries before mutating the document, matching RedTilde's existing `OpenLink` and `BlockDraft` invariants.
9. **Hostile metadata sanitization** *(Sections 2.2.A, 3.6, 4.2)*: Strip control, bidi-override, zero-width, and tag characters (`U+202A..U+202E`, `U+2066..U+2069`, `U+E0000..U+E007F`, etc.) and cap lengths before displaying dictionary names or load errors in UI.

### B.2 Other open-source ecosystems surveyed

- **Chromium (`WinHybridSpellchecker`)**: Combines Windows `ISpellCheckerFactory` primary checking (reading OS shared dictionaries asynchronously off the UI thread) with a Hunspell fallback when an OS language pack is absent — proving the hybrid strategy pattern behind Option E.
- **Notepad++ `DSpellCheck` & AbiWord `Enchant`**: Decouple editor tokenization, underline rendering, and context menus from concrete engines via an abstract backend interface — directly informing `ISpellProvider` and `FakeSpellProvider`.
- **Harper (`automattic/harper`)**: Apache-2.0 Rust Markdown grammar/spell checker; primarily English-focused and introduces a Rust toolchain into RedTilde's MSVC C++23 build.
- **Offline Lexical / Thesaurus Formats (MyThes, Open English WordNet, StarDict, FreeDict)**: LibreOffice MyThes (`.idx`/`.dat`), Open English WordNet (`CC-BY-4.0`, ~15–25 MiB), and StarDict (`.ifo`/`.idx`/`.dict.dz`) provide viable local formats if user-supplied lexical/thesaurus lookup is ever added in `S4`, without bundling heavy corpora inside RedTilde's portable distribution.
