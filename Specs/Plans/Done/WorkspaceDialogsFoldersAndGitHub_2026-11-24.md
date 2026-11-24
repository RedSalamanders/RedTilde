# Workspace DxUi dialogs, multi-folder navigation, file actions, and GitHub repositories

## Metadata

- **Status**: `DONE` (`2026-11-24` — Phases **W1–W4**, normative contracts, tests, 96-DPI Release captures, documentation, and local 6-profile qualification completed; sibling `RedSalamander --select` CLI adoption, native ARM64 runtime execution, interactive desktop UIA/focus pass, and manual live GitHub OAuth Device Flow smoke remain explicit separate gates)
- **Created**: `2026-11-24`
- **Pinned DxUi baseline**: `0c371a6197eb8c6b876b4b2843d75078bc21035c` (API 5, [Dependencies/DxUi.lock.json](../../../Dependencies/DxUi.lock.json)) — Phases **W1–W4** are implementable directly on API 5 without blocking on a `DxUi` pin bump; optional future `DxUi` ergonomics are isolated in [Section 5.2](#52-optional-dxui-improvements-non-blocking-for-w1w4)
- **Prerequisite sibling capability (for RedSalamander launch reveal)**: `RedSalamander` startup/single-instance `--select=<path>` / `--left=<dir>` support ([Section 5.1](#51-redsalmander-cli-selection-prerequisite))
- **Normative contracts, machine files, and docs to update when implemented**:
  - [Specs/README.md](../../README.md)
  - [Core_Architecture.md](../../Core/Core_Architecture.md)
  - [Core_Settings.md](../../Core/Core_Settings.md)
  - [Core_FileSafety.md](../../Core/Core_FileSafety.md)
  - [Core_PerformanceAndResources.md](../../Core/Core_PerformanceAndResources.md)
  - [Editor_Markdown.md](../../Editor/Editor_Markdown.md)
  - [UI_DesignSystem.md](../../UI/UI_DesignSystem.md)
  - [Testing_Validation.md](../../Testing/Testing_Validation.md)
  - [SettingsStore.schema.json](../../../SettingsStore.schema.json)
  - [capabilities.json](../../../capabilities.json)
  - [docs/README.md](../../../docs/README.md), [docs/Portable.md](../../../docs/Portable.md), [docs/screenshots/README.md](../../../docs/screenshots/README.md)

---

## 1. Scope, objectives, and hard invariants

This specification covers four cohesive workspace upgrades centered on file clarity, modern DxUi interaction, safe workspace actions, and read-through GitHub repository browsing:

| Phase | Pillar | Summary |
| --- | --- | --- |
| **W1** | **DxUi Save-Confirmation Dialog & Hot-Exit Resume** | Replace native Win32 `MessageBoxW` save prompts with an in-window themed DxUi modal showing each document's **filename**, **relative path to its owning open folder** (or GitHub repo), and **full-path tooltip**, supporting both single-file and consolidated multi-file checklists, a **Keep & Exit** action (reopening ready to continue editing and save cleanly), and non-blocking automatic Keep & Resume by default during Windows shutdown/logoff. |
| **W2** | **Multi-Folder Left Files Pane & Sections** | Support opening and retaining multiple local folders simultaneously (`<= 16` folders) in clearly delineated collapsible sections alongside a **Modified documents** section, plus clean full-path discovery and `+ Folder` / `+ GitHub` actions. |
| **W3** | **Files Pane Context Menu & Safe File Ops** | Add right-click and keyboard (`Shift+F10` / `F2` / `Ctrl+X` / `Ctrl+C` / `Ctrl+V`) actions in `_filesTree`: **Reveal in File Explorer**, **Reveal in RedSalamander**, **Copy full path**, **Copy relative path**, **Rename**, **Cut**, **Copy**, and **Paste**, executed safely off the UI thread with reparse/workspace-rebasing guards. |
| **W4** | **GitHub Section & Open GitHub Dialog** | Add an **Open GitHub…** flow (`Ctrl+Shift+G`) near **Open Folder…** with URL/`owner/repo` input, **autosuggest from the connected user's repositories**, Windows Credential Manager authentication (OAuth Device Flow / PAT / opt-in `gh` import), bounded HTTPS listing of `.md`/`.markdown` files in collapsible GitHub sections, and read-through workspace opening. |
| **W5** *(Deferred)* | **GitHub Commit / Branch Writeback** | Optional future phase for committing/pushing modified GitHub documents back to a branch or Pull Request; explicitly out of scope for `W1`–`W4`, where saving a modified GitHub document uses local **Save As…**. |

### Hard engineering invariants

1. **Allowed Write Locations Only (Fail Closed)**:
   - Local directory listing, rename, cut/copy/paste, and reveal operate strictly on explicit canonical user-selected paths and opened folder trees, refusing reparse-point traversal (`IsReparseTagNameSurrogate`), relative paths, and unverified junctions before mutation.
   - Credential secrets (`GitHub` tokens) MUST NEVER be written to `RedTilde.settings.json`, logs, diagnostics, minidumps, or repository working trees; they live exclusively in Windows Credential Manager (`advapi32` `CredWriteW` / `CredReadW` / `CredDeleteW`) under the exact target prefix `RedTilde/GitHub/`.
2. **Zero unsolicited network I/O**:
   - No GitHub HTTP request runs on cold startup, idle timers, or background polling while the user has not opened the **Open GitHub** dialog, refreshed/expanded a GitHub section, or clicked a persisted GitHub document.
   - `WinHttpGitHubTransport` connects exclusively over HTTPS (`INTERNET_SCHEME_HTTPS`, TLS certificate validation enabled, HTTP->HTTP or cross-host non-allowlisted redirects refused) to `api.github.com` (and `github.com/login/device/*` strictly during an explicit user-started OAuth Device Flow).
3. **UI thread responsiveness & bounded shutdown**:
   - File rename, file copy/move, shell/RedSalamander reveal, credential lookup, and all GitHub HTTP calls execute on bounded worker leases (`ProcessWorkLease`), report completion via posted window messages validated by `SpellRequestKey`/`WorkspaceOpKey` monotonic identity, and cancel/drain cleanly inside `AreShutdownWorkersSettled()`.
4. **Code quality & build profiles**:
   - C++23 (`stdcpplatest`), Unicode Win32 APIs, `/Wall /permissive- /sdl` with `0 warnings, 0 errors` across all six `(x64, ARM64) × (Debug, Release, ASan Debug)` profiles, `_camelCase` private/protected members (`H-27` / `.clang-tidy`), WIL RAII, `noexcept` checked error paths, zero C++ exceptions across Win32/COM boundaries, and `DxUi::Failure::FailAllocation()` on fatal allocation failure.
5. **Hostile metadata & path sanitization**:
   - Repository names, branch names, file paths returned by remote APIs, repo descriptions, and API error messages MUST pass through a shared `SanitizeUiText` helper (stripping Unicode control chars, bidi overrides/isolates `U+202A..U+202E`, `U+2066..U+2069`, zero-width chars, and tag chars `U+E0000..U+E007F`, and normalizing separators/length bounds) before display or accessibility publication.
6. **Deterministic offline testing**:
   - All features in `W1`–`W4` expose clean abstract interfaces (`IShellReveal`, `ICredentialStore`, `IGitHubTransport`, `IWorkspaceFileOps`) so headless behavioral suites and `--capture` scenarios run 100% offline with synthetic fixtures and never touch `%LOCALAPPDATA%`, Windows Credential Manager, shell windows, or live GitHub endpoints.

---

## 2. Detailed architecture and component specifications

### 2.1 `W1` — DxUi Save-Confirmation Dialog (`OverlayKind::SaveConfirmModal`)

#### Why replace `MessageBoxW`
Currently `Application::AskThen()` (`RedTilde/Application.cpp:2882-2933`) calls native Win32 `MessageBoxW` with a generic resource string (`IDS_SAVEQUESTION`), ignoring dark/paper theming, custom app typography, active document identity, relative path within the open workspace folder, and forcing `N` sequential `MessageBoxW` popups on exit (`AskWorkspaceThen()`) when `N` workspace documents are dirty.

#### Visual & structural design (on pinned DxUi API 5)
- Add `OverlayKind::SaveConfirmModal` to the existing mutually exclusive overlay controller (`SetActiveOverlay(OverlayKind)`).
- Surface geometry: centered `DxUi::Surface` (`Surface::Kind::Floating`, `10`-DIP corners, themed card ground/border/shadow, translucent `theme.smokeOverlay` full-client backdrop scrim that blocks pointer clicks to underlying chrome and tracks owner resizes cleanly via `Layout()`).
- **Header**:
  - Single-document dirty exit / Close Document / New / Open: title `"Save changes before continuing?"` (exact action context: **Save changes before closing RedTilde?**, **Save changes to this document?**, etc.) + subtitle explaining that unsaved edits will be lost unless saved.
  - Multi-document dirty exit / Open Folder: title `"Save changes to {0} documents?"` + compact instruction line (`"Checked documents will be saved before closing."`).
- **Document item card(s)** (one card in single-document mode; bounded scrollable list up to `240`-DIP height in multi-document mode):
  - **Primary line (Semibold `14` DIP)**: Filename (e.g. `ArchitectureNotes.md`, or `Untitled.md` for unsaved buffers, with accent dirty dot).
  - **Secondary line (Muted `12` DIP, `theme.subtleText`)**:
    - When the file belongs to one of the open local folders (`Settings::filesRoots`): `<FolderSectionName> · <relative/path/to/folder>` (e.g. `RedTilde · docs/README.md`, or `Workspace-A (Parent) · sub/note.md` when multiple open folders share a basename).
    - When the file is a local file **outside** all currently open folders: full parent directory path (`C:\Users\...\Folder`).
    - When the document is from a GitHub repo (`DocumentOrigin::GitHub`): `GitHub · owner/repo · relative/path.md` (with localized note `"Will prompt Save As to save locally"` when checked).
    - When the document is unnamed/recovered: localized `"(Untitled — chooses location on Save)"` or original recovered target relative path.
  - **Tooltip & UIA Description**: Full canonical Windows path (or GitHub HTTPS URL) exposed via tooltip hover and `FullDescription`.
  - In **multi-document mode**: each row carries a DxUi `CheckBox` (default checked for all dirty documents) plus an optional `Select all / None` toggle when `>= 2` items exist.
- **Footer action row** (right-aligned, ordered per Windows convention with clear focus ring and keyboard accelerators):
  - Single-document mode:
    1. **Save** (`DxUi::Button`, `SetPrimary(true)`, `Alt+S` / `Enter` on default button — or **Save as…** if untitled/GitHub).
    2. **Keep & Exit** (`Alt+K`, shown when `reason == SaveConfirmReason::CloseWindow` — exits immediately without saving to disk while keeping edits safely so reopening restores the dirty document ready to edit and `Ctrl+S`).
    3. **Don't Save** (`Alt+D`, secondary outline/standard button).
    4. **Cancel** (`Esc`, standard button).
  - Multi-document mode:
    1. **Save ({n})** (`SetPrimary(true)` when checked count `> 0`; hidden/replaced when `0` checked).
    2. **Keep & Exit** (`Alt+K`, primary when `0` checked on `CloseWindow`, always present on `CloseWindow` — exits immediately leaving all dirty documents ready to resume on next launch).
    3. **Don't Save Any** (secondary outline/standard button, explicitly discards all dirty edits without resuming).
    4. **Cancel** (`Esc`, standard button).
    - Selecting a subset (`0 < k < N` checked) and clicking **Save ({k})** saves the `k` checked documents sequentially (stopping on first failure or Save As cancellation without losing the remaining documents) and discards only the unchecked `(N - k)` documents once the entire admitted batch succeeds.
- **Hot-Exit (`Keep & Exit`) & Optional Zero-Prompt Exit Setting**:
  - `Settings::exitUnsavedBehavior`: `"prompt"` (default for interactive window close) | `"keepAndResume"`, persisted in `RedTilde.settings.json` / `SettingsStore.schema.json` and configurable in **Settings → Writing** (plus an optional `"Always keep unsaved edits on exit"` checkbox inside `SaveConfirmModal`). When set to `"keepAndResume"`, manual window close (`Alt+F4` / Close button) flushes `SessionResume` snapshots immediately with zero modal prompt.
  - Note: **Keep & Exit** applies only to closing the window (`CloseWindow`). Actions that immediately replace or discard an active document (`CloseDocument`, `NewDocument`, `OpenFile`) continue to offer **Save / Don't Save / Cancel**.
- **OS Shutdown & Logoff Default (`WM_QUERYENDSESSION` / `WM_ENDSESSION`) — Never Block Shutdown**:
  - During OS shutdown, restart, or Windows logoff (`WM_QUERYENDSESSION` / `WM_ENDSESSION`), RedTilde **MUST NEVER** display `SaveConfirmModal`, call `MessageBoxW`, or call `ShutdownBlockReasonCreate` to block Windows sign-out/shutdown.
  - **Keep & Resume** is **always** the default behavior on OS shutdown/logoff regardless of `Settings::exitUnsavedBehavior`:
    - `WM_QUERYENDSESSION` returns `TRUE` immediately.
    - `WM_ENDSESSION(TRUE)` flushes immediate `RecoverySnapshotKind::SessionResume` checkpoints for every dirty workspace document (`_session` + `_inactiveDocuments`, preserving workspace order, active slot, `FileStamp` baseline, selections, BOM, and `DocumentOrigin`), bounds settlement to existing `RecoveryService::WaitForSettlement(2000)`, stops non-essential background workers (`_spellService`, `_settingsWatcher`, `_githubService`), and exits without retiring those `SessionResume` snapshots.
- **Session-Resume vs. Crash-Recovery Semantics (`Common/Recovery.*`)**:
  - Extend `RecoverySnapshot` with backward-compatible fields:
    - `RecoverySnapshotKind kind` (`CrashRecovery` default for legacy/crash snapshots vs `SessionResume` written by **Keep & Exit**, `exitUnsavedBehavior == "keepAndResume"`, or `WM_ENDSESSION(TRUE)`).
    - `uint32_t workspaceOrder` & `bool wasActive` (preserves exact workspace order and active document focus).
    - `std::optional<FileStamp> savedBaselineStamp` (volume serial / full 128-bit file ID / byte length / last-write timestamp / content fingerprint from the last saved baseline).
    - `DocumentOriginMetadata origin` (local path vs `GitHub` `{ owner, repo, branch, relativePath, blobSha }`).
  - **On next launch when `SessionResume` snapshots exist**:
    - Auto-restore every `SessionResume` snapshot directly into active/inactive workspace slots in original order **automatically without showing `RecoveryModal`** and **without** setting `_session.recovered = true`.
    - Each resumed document opens dirty (`DirtyDot`, listed in **Modified documents**), retains its original `path` (or `DocumentOrigin::GitHub`), caret/selection, BOM, and restored `savedBaselineStamp`, so the user is immediately back where they left off and ready to continue editing and save (`Ctrl+S` writes straight to the original file path!).
    - **External modification protection**: Because `savedBaselineStamp` is preserved across exit and reopen, if another program edited the file on disk while RedTilde was closed, pressing `Ctrl+S` cleanly triggers the standard **Save Conflict** dialog (**Reload / Save As / Overwrite / Cancel**) instead of silently clobbering external changes. Untitled and GitHub-origin resumed documents still route `Ctrl+S` to **Save As…** locally.
    - If launched with an explicit CLI document (`RedTilde.exe other.md`), `other.md` opens active while all `SessionResume` dirty documents remain loaded and dirty in `_inactiveDocuments` (immediately visible in **Modified documents** and left-rail dots).
    - Consumed `SessionResume` snapshots are retired **only after** live workspace adoption and fresh live recovery checkpoints succeed (eliminating any data-loss window if startup crashes).
    - Genuine unexpected crashes (`RecoverySnapshotKind::CrashRecovery` only) preserve the existing `RecoveryModal` preview (`Restore a copy` -> requires `Save As…`) contract completely unchanged. If both `SessionResume` and `CrashRecovery` snapshots exist simultaneously, auto-restore `SessionResume` into the workspace and surface only `CrashRecovery` candidates in `RecoveryModal`.

#### Continuation state machine & safety contracts
- Replaces synchronous nested `MessageBoxW` loops inside `AskThen()` / `AskWorkspaceThen()` with an explicit non-blocking `std::optional<SaveConfirmSession>` holding:
  - `uint64_t promptGeneration`
  - `SaveConfirmReason reason` (`CloseWindow`, `CloseDocument`, `NewDocument`, `OpenFile`, `OpenFolder`, `OpenGitHub`, `RestoreRecovery`)
  - `std::vector<SaveConfirmEntry>` (`workspaceKey`, `DxUi::RichText::SourceSnapshot expectedSnapshot`, normalized display name, owning folder display label, relative path, full path/URL, `bool checked`)
  - `std::function<void()> continuation`
- **Modal safety & Focus**:
  - `ModalPromptKind::UnsavedChanges` remains set while `OverlayKind::SaveConfirmModal` is open, preventing duplicate `WM_CLOSE` or overlapping modal commands (`HasActiveModalPrompt()`).
  - Re-validates `workspaceKey` and `SourceSnapshot::IsSameRevision()` before executing each save or discard in the admitted queue; if a background event changed source state, aborts safely and preserves dirty edits.
  - Opening `SaveConfirmModal` saves the previously focused control and traps `Tab` / `Shift+Tab` inside the modal (`Esc` = Cancel, restoring focus).
  - **Close-during-Save integration**: On `WM_CLOSE`, if saving one or more checked documents is asynchronous (already-named files), the window hides (`ClosingAfterSave`) only after all prompt choices (`Save As` pickers for any untitled/GitHub checked items) settle cleanly, and restores immediately on any save conflict or I/O failure (`Core_FileSafety.md` intact).
  - **Headless test seam**: `LaunchOptions::unsavedPromptForTest` continues to work identically for automated batch tests while new dedicated DxUi modal UI tests verify rendered geometry, relative paths, checklist toggling, `Esc` focus restoration, and multi-document save/discard ordering without `MessageBoxW`.

---

### 2.2 `W2` — Multi-Folder Left Files Pane & Section Organization

#### Settings & Workspace Data Model (`Common/Settings.*` & `SettingsStore.schema.json`)
- Expand folder configuration in `RedTilde::Settings`:
  - `std::vector<std::filesystem::path> filesRoots` (`<= 16` canonical, deduplicated ordinal-case-insensitively on Windows paths, strict UTF-16 valid non-control absolute paths).
  - Backward compatibility with existing `std::filesystem::path filesRoot`:
    - Cold load reads both `filesRoots` (array of strings) and legacy `filesRoot` (single string). If `filesRoots` is absent/empty and `filesRoot` is valid, seeds `filesRoots = { filesRoot }`.
    - `SerializeSettings` writes `filesRoots` **and** keeps `filesRoot` synchronized to `filesRoots.front()` (or omitted if empty) so mixed versions remain non-destructive.
  - `std::vector<GitHubRepoRef> githubRepos` (`<= 8` entries of `{ owner, repo, branch, alias }`), persisted as clean non-secret metadata in JSON5.
  - `std::optional<std::filesystem::path> redSalamanderPath` (optional user override for executable path, strictly validated non-reparse `.exe` if set).
- Enforce capacity cap: attempting to add a 17th local folder or 9th GitHub repository reports a localized status message (`IDS_FILES_MAX_FOLDERS`) instead of silently dropping an existing folder.

#### Left Files Pane visual hierarchy (`_filesTree`)
The left Files pane header (`_filesPane`) and `_filesTree` display clearly delineated collapsible sections:

1. **Pane Header & Action Strip**:
   - Title `FILES` + action icons (`+ Folder`, `Open GitHub`, header `Pin/Unpin`) sized cleanly inside `200..400`-DIP width (moving secondary text into icon tooltips cleanly without overlap at `200` DIP).
   - Empty state (when both `filesRoots` and `githubRepos` are empty and no dirty docs): friendly two-button empty state (**Open Folder…** and **Open GitHub…**).
2. **Section 0 — Modified documents** *(shown only when `>= 1` document is dirty)*:
   - Stable opening order (`WorkspaceGroupId`).
   - Row text: `filename` + accent dirty dot; secondary badge/tooltip shows owning section & relative path (`Docs · notes.md` or `owner/repo · README.md`) so two identically named dirty files from different open folders are immediately distinguishable.
3. **Sections `1..K` — Local Folders** *(one section per entry in `Settings::filesRoots`)*:
   - Top-level tree node ID range (`LocalRootBaseId + folderIndex * RootStride`):
     - Display label: `<FolderName>` (disambiguated automatically as `<FolderName> (<ParentName>)` if two open folders share a leaf name, e.g. `docs (RedTilde)` vs `docs (RedPrism)`).
     - Full canonical path in `item.tooltipText` (`C:\Src\RedTilde\docs`), always readable on hover and accessible via `GetTooltipText()` / UIA `ItemStatus` / right-click **Copy Full Path**.
     - Badge: total Markdown file count in that folder when collapsed (or refresh/error badge if `FAILED(snapshot.status)`).
     - Children: subdirectories & `.md` / `.markdown` files enumerated by `MultiFolderFilesService` (each independent folder honors `<= 5,000` examined entries, depth `<= 8`, natural Explorer ordering, no hidden/system files, no reparse-point traversal, independent coalesced `ReadDirectoryChangesW`).
     - Live filename filter (`_filesFilter`) filters across **all** open local folders and GitHub sections simultaneously, preserving matching section headers cleanly while hiding non-matching files.
4. **Sections `G1..GM` — GitHub Repositories** *(one section per connected repo in `Settings::githubRepos`)*:
   - Top-level tree node ID range (`GitHubRootBaseId + repoIndex * RootStride`):
     - Display label: `<owner>/<repo>` + branch badge (`main`) and lock/globe status glyph (`🔒` or `\xE72E` / `\xE774` Fluent icon where appropriate).
     - Tooltip: `https://github.com/<owner>/<repo>/tree/<branch>` + loading/truncated/rate-limit state.
     - Lazy-load policy: persisted GitHub sections render collapsed (`not loaded yet — click to load`) at cold startup so zero network traffic occurs until the user expands/clicks a section, adds a repo, or clicks **Refresh** on that section.

#### Relative path computation helper (`ResolveWorkspaceDisplayLocation`)
Shared across **Save-Confirmation Dialog**, **Modified documents**, and **Copy Relative Path**:
- Given a `std::filesystem::path` (or `DocumentOrigin`):
  1. If `origin == GitHub`: owner section is `owner/repo`, relative path is normalized repo-relative POSIX path `docs/guide.md`, full reference is `https://github.com/owner/repo/blob/<branch>/<path>`.
  2. Otherwise, check each `root` in `Settings::filesRoots` (longest canonical prefix match using case-insensitive Windows prefix comparison / root identity):
     - Found matching root -> owner section is that root's disambiguated label, relative path is `std::filesystem::relative(filePath, root)` (normalized with `/`).
  3. Outside all open roots -> owner label is parent folder basename or drive/folder path, relative path is omitted (parent directory shown instead).

---

### 2.3 `W3` — Files Pane Context Menu & Safe File Operations

#### Context menu trigger & dispatch
- Implement `ITreeDelegate::OnTreeContextMenu(uint64_t itemId, POINT screenPoint)` in `Application.Navigation.cpp` (`_filesTree` already calls `OnContextMenu` for right-click release, `Shift+F10`, and `VK_APPS` in DxUi!).
- Also handle `WM_KEYDOWN` / tree `OnKeyDown` bindings while `_filesTree` holds focus:
  - `F2` -> **Rename** (on local file or local subfolder)
  - `Ctrl+C` -> **Copy** (staged path / `CF_HDROP`)
  - `Ctrl+X` -> **Cut** (staged path / `CF_HDROP` + `DROPEFFECT_MOVE`)
  - `Ctrl+V` -> **Paste** (pastes into focused folder / sibling folder of focused file / owning root section)
- Show menu via non-blocking `DxUi::ContextMenu::ShowAsync(...)` anchored at `screenPoint`, capturing the exact item identity (`FileTreeNodeIdentity { Kind, folderIndex / repoIndex / itemPath / sessionKey }`) so completion revalidates item existence before acting.

#### Action matrix by row kind

| Action | Local File | Local Subfolder | Local Root Section | Modified Doc Row | GitHub File | GitHub Root Section |
| --- | --- | --- | --- | --- | --- | --- |
| **Open** | Open in editor | Expand/Collapse | Expand/Collapse | Switch to doc | Fetch/Open read-through | Load/Expand |
| **Reveal in File Explorer** | Enabled (`select`) | Enabled (`select`) | Enabled (open folder) | Enabled if disk-backed | Hidden/Disabled | Hidden/Disabled |
| **Reveal in RedSalamander** | Enabled when detected\* | Enabled when detected\* | Enabled when detected\* | Enabled if disk-backed\* | Hidden/Disabled | Hidden/Disabled |
| **Copy full path** | Absolute path | Absolute path | Absolute folder path | Absolute path / URL | HTTPS `blob` URL | HTTPS `tree` URL |
| **Copy relative path** | Rel to open root | Rel to open root | Folder basename | Rel to root (if rooted) | Repo-relative path | Branch name |
| **Rename (`F2`)** | Enabled | Enabled | Hidden *(use OS)* | Enabled if disk-backed | Disabled | Disabled |
| **Cut (`Ctrl+X`)** | Enabled | Enabled | Hidden | Enabled if disk-backed | Disabled | Disabled |
| **Copy (`Ctrl+C`)** | Enabled | Enabled | Enabled | Enabled if disk-backed | Enabled *(URL/name text only? Better: disabled for file clipboard, use Copy Path)* | Disabled |
| **Paste (`Ctrl+V`)** | Pastes to parent dir | Pastes into dir | Pastes into root | Disabled | Disabled | Disabled |
| **Refresh Folder / Repo** | Enabled *(root)* | Enabled *(root)* | Enabled | — | Enabled *(repo)* | Enabled |
| **Close Folder / Remove Repo** | — | — | Enabled | — | — | Enabled |

*\*Note on **Reveal in RedSalamander***: see exact discovery and prerequisite status below.

#### Implementation & safety contracts per operation

1. **Reveal in File Explorer (`IShellReveal::RevealInExplorer`)**:
   - Dispatched to a short-lived STA worker under `ProcessWorkLease` (never blocks UI thread with COM/Shell calls).
   - Validates canonical existence and rejects reparse surrogates (`FILE_ATTRIBUTE_REPARSE_POINT` + `IsReparseTagNameSurrogate`).
   - Uses `ILCreateFromPathW` + `SHOpenFolderAndSelectItems` (RAII `wil::unique_cotaskmem_itemlist`) when targeting a file or subfolder so Explorer opens the parent and highlights the exact item; falls back to `ShellExecuteExW` (`explore`) when opening a root folder itself.

2. **Reveal in RedSalamander (`IShellReveal::RevealInRedSalamander`)**:
   - **Discovery (read-only, trusted locations only — never searches ambient `CWD` or untrusted `%PATH%`)**:
     1. Optional explicit valid `Settings::redSalamanderPath` (must be a canonical non-reparse regular file).
     2. HKCU / HKLM `SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\RedSalamander.exe`.
     3. Known Winget portable alias / package install locations:
        - `%LOCALAPPDATA%\Programs\RedSalamander\RedSalamander.exe` (or sibling `RedLauncher.exe`)
        - `%LOCALAPPDATA%\Microsoft\WinGet\Links\RedSalamander.exe` (via `RedLauncher.exe` alias resolution; never execute a symlink directly from `Links\` unless it resolves through the trusted launcher/final target per `WingetIntegration.md`).
   - **Important truthfulness guard against current `RedSalamander.exe` (see Section 5.1)**:
     - Verified today in `z:\src\RedSalamander\RedSalamander\RedSalamander.cpp:7130-7162`: `RedSalamander.exe` does **not yet accept** a `--select=<path>` or folder path CLI argument.
     - Therefore:
       - Phase **W3** implements discovery + `--select=<canonicalPath>` launch contract (using `Common::Process` / `QuoteWindowsCommandLineArgument`-equivalent proper Windows `CreateProcessW` quoting and parent `lpCurrentDirectory` set to the trusted install directory, `ProcessWorkLease`), gated on RedSalamander CLI support landing (tracked in Section 5.1).
       - If RedSalamander is **not installed** (or until `--select` support is verified/shipped), the **Reveal in RedSalamander** menu item remains visible or cleanly disabled with an explanatory tooltip (`IDS_FILES_REDSALAMANDER_UNAVAILABLE` / status guidance) and **never silently drops the user into the wrong folder without explanation**.

3. **Copy Full Path & Copy Relative Path**:
   - Clipboard writes use STA/UI `OpenClipboard(_window.get())` + `EmptyClipboard` + `SetClipboardData(CF_UNICODETEXT, ...)`, failing cleanly with a localized status if the clipboard is locked (`IDS_CLIPBOARD_UNAVAILABLE`), without touching focus.
   - Offline tests use an injected clipboard test seam (`launch.clipboardWriterForTest` / `launch.clipboardReaderForTest`) — **never touching the desktop clipboard** in headless tests!

4. **Rename (`F2`, `OverlayKind::RenameFileModal`)**:
   - Opens compact DxUi modal (`Surface::Kind::Floating`) pre-filled with existing filename, validating:
     - Non-empty, `<= 255` UTF-16 units, no path separators (`\`, `/`, `:`), no control chars or Windows reserved characters (`<>:"/\|?*`), no reserved device names (`CON`, `PRN`, `AUX`, `NUL`, `COM1..9`, `LPT1..9`).
     - Extension guard: if renaming a file changes extension away from `.md`/`.markdown`, prompt inline warning (`"Changing extension removes it from the Markdown file list. Continue?"`).
     - Target does not already exist on disk (uses `MoveFileExW(..., 0)` without `MOVEFILE_REPLACE_EXISTING`).
     - Source and destination parent are validated non-reparse (`IsReparseTagNameSurrogate`).
   - Runs off UI thread (`IWorkspaceFileOps` / `FileOpsService`); completion updates:
     - Every open workspace slot (`_session` or `_inactiveDocuments`) referencing old path (or under a renamed directory!) updates `_session.path` / `metadata.path`, `_settings.lastDocument`, window title, **Dirty status preserved intact** (renaming a dirty open file does NOT save or discard unsaved edits!), and recovery slot metadata (`RecoveryService` re-submits current state with updated path).

5. **Cut (`Ctrl+X`), Copy (`Ctrl+C`), and Paste (`Ctrl+V`)**:
   - **Cut / Copy**:
     - Publishes standard Windows `CF_HDROP` + `"Preferred DropEffect"` (`DROPEFFECT_MOVE` for Cut, `DROPEFFECT_COPY` for Copy) onto the clipboard (or headless test seam), plus sets an internal `FilesCutPendingState { std::vector<std::filesystem::path> cutPaths }` to visually dim the cut row(s) until Paste completes, clipboard ownership changes, or `Esc` clears cut state.
   - **Paste**:
     - Reads `CF_HDROP` + `Preferred DropEffect` (always bounds-checked `<= 64` paths, canonicalized, reparse-rejected; folders or files within size/extension policy). Non-local/virtual paths are rejected with clear status.
     - Destination directory is resolved from the selected row (folder node -> itself; file node -> parent folder).
     - **Same-folder Copy**: generates non-colliding Windows-style suffix (`Notes - Copy.md`, `Notes - Copy (2).md`, bounded attempts `<= 99`).
     - **Cross-folder Collision**: refuses silent overwrite; shows a themed DxUi conflict card (**Keep Both / Replace / Cancel**, defaulting to **Cancel**) and blocks Move if the colliding target is currently dirty in another workspace slot without explicit resolution.
     - **Move completion (`DROPEFFECT_MOVE`)**: atomically updates open workspace slots matching moved files/subfolders identically to **Rename**, clears cut dimming and `"Preferred DropEffect"`, and triggers immediate targeted refresh on source & destination folder roots.

---

### 2.4 `W4` — GitHub Connection, `Open GitHub` Dialog & Connected-User Repository Autosuggest

#### Entry points & Settings UI
1. **Menu & Pane Buttons**:
   - **File** menu adds **Open GitHub…** (`Ctrl+Shift+G`) directly adjacent to **Open Folder…** (`Ctrl+Shift+O`) and **Add Folder to Workspace…**.
   - **Files pane** header and empty state expose **+ GitHub** alongside **+ Folder**.
2. **Settings → Accounts / GitHub** *(or section directly under Writing/Privacy before About)*:
   - Connection status card: `"Not signed in (public repositories only)"` vs `"Signed in as @<login>"` (sanitized via `SanitizeUiText`) with cached repo count (`"{0} accessible repositories"`), **Sign in with GitHub…** (OAuth Device Flow), **Use Personal Access Token…**, optional explicit **Import from GitHub CLI (`gh`)** button (only runs `gh auth token` when explicitly clicked by the user, off UI thread), **Refresh Repositories**, and **Sign out** (clears Credential Manager entry + drops in-memory token/repo list).

#### `Open GitHub` Modal (`OverlayKind::OpenGitHubModal`)
- Themed `DxUi::Surface` (`Surface::Kind::Floating`, width `520` DIP clamped to viewport) over `theme.smokeOverlay`:
  1. **Header**: Title `"Open GitHub Repository"`, subtle top-right close button (`×`), and compact connection status pill (`"@alex-writer · 42 repos"` or `"Sign in for private repos & suggestions"` link/button).
  2. **Repository URL / `owner/repo` input (`DxUi::TextField`)**:
     - Placeholder: `"Paste GitHub URL or type owner/repo…"`
     - Accepts:
       - `https://github.com/<owner>/<repo>`
       - `https://github.com/<owner>/<repo>.git`
       - `https://github.com/<owner>/<repo>/tree/<branch>[/...]`
       - `https://github.com/<owner>/<repo>/blob/<branch>/<file.md>` (auto-connects repo + opens `<file.md>` immediately once loaded!)
       - Shorthand `<owner>/<repo>`
     - Strictly rejects malformed inputs, embedded credentials (`https://user:pass@`), query-string injection, non-`github.com` hosts (unless future Enterprise GHE opt-in is separately designed), `..` traversal, and control characters, showing immediate inline validation hint without network calls.
  3. **Live Repository Autosuggest List (`DxUi::Tree` or list container directly below input)**:
     - Populated automatically on dialog open (when signed in) from an in-memory session cache `GitHubUserRepoCache` (refreshed on demand via **Refresh**, bounded `<= 500` repos, persisted **in memory only** for the active session).
     - As the user types in the URL/repo box, filters cached user repositories client-side with instant `O(N)` scoring:
       - Prefix match on `repo` or `owner/repo` ranks highest, followed by substring match on `owner/repo`, followed by sanitized `description` match.
       - Each suggestion row shows: repo `owner/repo` (bold match highlight), default branch badge (`main`), `Private` badge when `isPrivate`, and single-line sanitized truncated description.
       - Keyboard UX: `Down` / `Up` moves selection inside suggestions; `Enter` (or clicking a row) populates `<owner>/<repo>` + its `defaultBranch` (or immediately opens if `Enter` is pressed cleanly); `Esc` dismisses the dialog.
     - When signed out: shows a quiet inline banner `"Sign in with GitHub to browse and autosuggest your repositories, or paste any public GitHub repository URL above."` + one-click **Sign in with GitHub** button (starts RFC 8628 Device Flow inline inside the modal with user code + **Copy code & open github.com/login/device**!).
  4. **Branch input (`DxUi::TextField`, optional)**:
     - Auto-filled from pasted `/tree/<branch>` URL or selected autosuggest item (`defaultBranch`); if left blank on manual `owner/repo`, queries `GET /repos/{owner}/{repo}` to resolve `default_branch`.
  5. **Footer buttons**: **Open Repository** (primary), **Cancel** (`Esc`).

#### Authentication & Credential Storage (`Common/CredentialStore.*`)
- **OAuth Device Flow (RFC 8628 — zero client secret needed in desktop PKCE/Device flow or configurable client ID)**:
  1. `POST https://github.com/login/device/code` with `Accept: application/json`, scope `repo read:user` (or public `public_repo` toggle option).
  2. Display user verification code (`ABCD-1234`) + verification URL (`https://github.com/login/device`) inside a copyable card with explicit **Copy & Open Browser** button (`ExternalLink` STA launch policy).
  3. Event-driven wait timer respecting GitHub's returned `interval` (backing off `+5 s` cleanly on `slow_down`, stopping immediately on `expired_token`, `access_denied`, Cancel, or window Close — never busy-polling).
  4. On `access_token` arrival, fetch `GET https://api.github.com/user` to display verified `@login`, persist token in Windows Credential Manager (`CRED_TYPE_GENERIC`, `CRED_PERSIST_LOCAL_MACHINE`, target `RedTilde/GitHub/api.github.com`, `UserName = login`), and fetch accessible repos (`GET /user/repos?sort=pushed&per_page=100`).
- **Fine-Grained Personal Access Token (PAT) fallback**:
  - Compact masked `DxUi::TextField` (`PasswordRevealMode::Peek`) verifying token via `GET https://api.github.com/user` before storing in Windows Credential Manager; zeroes temporary token buffers (`SecureZeroMemory`) immediately after handoff/clear.

#### GitHub Transport & Markdown Loading (`Common/GitHubClient.*`, `RedTilde/GitHubService.*`)
- **Transport abstraction**: `IGitHubTransport` (pure virtual, injected into `GitHubService`) with two implementations:
  - `WinHttpGitHubTransport`: WinHTTP TLS (`WINHTTP_FLAG_SECURE`), pinned host checks (`api.github.com` / `github.com`), redirect disabled (`WINHTTP_OPTION_REDIRECT_POLICY_NEVER` for API calls), explicit `User-Agent: RedTilde/<version>`, `X-GitHub-Api-Version: 2022-11-28`, `8,000 ms` connect / `15,000 ms` read timeout, cancellable `HANDLE abortEvent` + `ProcessWorkLease`.
  - `FakeGitHubTransport`: deterministic offline in-memory GitHub API responder for tests and hidden capture scenarios (`open-github-autosuggest`, `files-multi-folder-and-github-light`).
- **Listing all `.md` / `.markdown` files in a repository**:
  1. Resolve branch (or default branch via `GET /repos/{owner}/{repo}`) -> call `GET /repos/{owner}/{repo}/git/trees/{branch}?recursive=1` (max response JSON `4 MiB`).
  2. Parse via strict bounded `yyjson`, filter `type == "blob"` whose normalized path ends with `.md` or `.markdown` (case-insensitive) and contains no `..`, backslashes, NUL, or control chars.
  3. Enforce `FilesLimits` parity (`<= 5,000` listed/examined entries, depth `<= 8`); surface GitHub `"truncated": true` explicitly in the section footer (`"Partial tree: repository exceeded GitHub API recursive limit"`).
  4. Build parent folder hierarchy inside the repo's collapsible section in `_filesTree`, reusing natural Explorer-style sorting (`dirs` before `files`).
- **Opening & editing a GitHub Markdown file**:
  - Fetch raw file via `GET https://api.github.com/repos/{owner}/{repo}/contents/{encodedPath}?ref={branch}` with `Accept: application/vnd.github.raw+json` (or bounded raw size check against `DxUi::RichText::Document::MaximumUnits` UTF-16 limit BEFORE opening/replacing current buffer).
  - Opens in existing `ActiveEditor()` (`Compose`, `Source`, or `Split`) with `DocumentOrigin::GitHub { owner, repo, branch, relativePath, blobSha, htmlUrl }`.
  - Relative image URLs inside a GitHub Markdown file (referencing assets relative to the Markdown file's repo folder):
    - Do **NOT** auto-download silently unless image consent is granted! Follow `Specs/Editor/Editor_Markdown.md` remote image consent contract (`allowRemoteImages` / per-document **Allow here**), resolving relative asset paths against `https://raw.githubusercontent.com/{owner}/{repo}/{branch}/<dir>/<asset>` through existing `FetchRemoteImage`.
  - Saving a modified GitHub document:
    - Dirty indicator works identically (`DirtyDot`, `Modified documents` section, `SaveConfirmModal`).
    - Pressing **Save** (`Ctrl+S`) or selecting it in **SaveConfirmModal** opens local **Save As…** (`PickFile(true)` defaulting to `<filename>.md`) cleanly without touching remote GitHub in `W4`.

---

## 3. Phased implementation plan & engineering breakdown

### Phase W1 — DxUi Save-Confirmation Dialog & Hot-Exit / Shutdown Resume
- **New/Modified files**:
  - `Common/Recovery.h` & `Common/Recovery.cpp`: Extend `RecoverySnapshot` with `RecoverySnapshotKind` (`CrashRecovery` vs `SessionResume`), `workspaceOrder`, `wasActive`, `savedBaselineStamp`, and `origin`; keep backward-compatible parsing for legacy recovery envelopes.
  - `Common/Settings.h` & `Common/Settings.cpp`: Add `Settings::exitUnsavedBehavior` (`"prompt"` | `"keepAndResume"`, default `"prompt"`) + schema update (`SettingsStore.schema.json`).
  - `RedTilde/Application.h`: Add `OverlayKind::SaveConfirmModal`, `SaveConfirmSession`, modal controls (`_saveConfirmPanel`, `_saveConfirmBackdrop`, `_saveConfirmTitle`, `_saveConfirmSubtitle`, `_saveConfirmList`, `_saveConfirmRows`, `_saveConfirmPrimary`, `_saveConfirmKeepExit`, `_saveConfirmDiscard`, `_saveConfirmCancel`).
  - `RedTilde/Application.Chrome.cpp`: Construct and lay out `SaveConfirmModal` surface + backdrop scrim and Settings **Writing** toggle (`exitUnsavedBehavior`); wire focus trap and high-contrast borders.
  - `RedTilde/Application.cpp`: Refactor `AskThen()` / `AskWorkspaceThen()` into non-blocking `BeginSaveConfirmation(...)` + `ContinueSaveConfirmQueue()`, implement **Keep & Exit** (`SessionResume` flush + clean exit without retirement), update `WM_QUERYENDSESSION`/`WM_ENDSESSION` to always flush `SessionResume` without blocking shutdown, and auto-adopt `SessionResume` snapshots into live dirty workspace slots on relaunch without triggering `RecoveryModal`.
  - `res/Resource.h` & `res/RedTilde.rc`: Add localized `IDS_SAVE_CONFIRM_*` strings including `Keep & Exit` and Settings toggle text (`492..508`).
- **Exit criteria**: Interactive save prompts render as themed DxUi modals showing filename + owning folder relative path + full-path tooltip; single & multi-document exit prompts consolidate into one checklist; **Keep & Exit** closes cleanly and reopens directly into editable dirty buffers ready for `Ctrl+S` (with `FileStamp` conflict check preserved); `WM_QUERYENDSESSION`/`WM_ENDSESSION` never block OS shutdown/logoff and auto-resume on reopen; all existing `Storage`/`Recovery`/`Close` tests + new `Tests/Application.Tests.SaveDialog.cpp` pass across `x64 Debug`, `ASan Debug`, `Release`.

### Phase W2 — Multi-Folder Files Pane & Relative/Full Path Model
- **New/Modified files**:
  - `Common/Settings.h` & `Common/Settings.cpp`: Add `filesRoots` (`<= 16`), `githubRepos` (`<= 8`), `redSalamanderPath`, lossless JSON5 serialization & legacy `filesRoot` migration; update `SettingsStore.schema.json`.
  - `Common/Files.h` & `Common/Files.cpp`: Add `MultiFolderFilesService` (or vector of per-root workers sharing bounded `FilesLimits`, each with independent `FilesSnapshot`, `generation`, and `ReadDirectoryChangesW` lease) and `ResolveWorkspaceDisplayLocation()`.
  - `RedTilde/Application.h`, `RedTilde/Application.Navigation.cpp`, `RedTilde/Application.Chrome.cpp`: Update `_filesTree` model builder to emit **Modified documents** + `LocalRoot` folder sections + section headers/badges/tooltips + **Add Folder…** (`Command::AddFolder`) & **Close Folder** per-section actions.
  - `res/Resource.h` & `res/RedTilde.rc`: Strings `IDS_FILES_ADD_FOLDER`, `IDS_FILES_REMOVE_FOLDER`, `IDS_FILES_LOCAL_FOLDERS`, `IDS_FILES_MAX_FOLDERS`, etc. (`509..525`).
- **Exit criteria**: Opening multiple folders shows distinct labeled sections in `_filesTree`; duplicate basenames disambiguate cleanly; live filtering works across all roots; hovering any row shows full canonical path; all existing Files tests + new multi-folder tests pass.

### Phase W3 — Files Pane Context Menu & Off-UI File Operations
- **New/Modified files**:
  - `Common/FileOps.h` & `Common/FileOps.cpp`: Shared `IShellReveal` (Explorer `SHOpenFolderAndSelectItems`, RedSalamander trusted discovery & launch), `IWorkspaceFileOps` (bounded safe rename/copy/move refusing reparse points & reserving `.md`/`.markdown`/`.txt`), clipboard `CF_HDROP` + `Preferred DropEffect` packing/unpacking.
  - `RedTilde/Application.Navigation.cpp`: Implement `OnTreeContextMenu`, keyboard shortcuts (`F2`, `Ctrl+X/C/V`), `OverlayKind::RenameFileModal`, `OverlayKind::FileConflictModal` (or shared DxUi dialog), cut-state visual styling, and atomic workspace/session/recovery rebasing after rename/move.
  - `res/Resource.h` & `res/RedTilde.rc`: Context menu labels & error messages (`IDS_MENU_REVEAL_EXPLORER`, `IDS_MENU_REVEAL_REDSALAMANDER`, `IDS_MENU_COPY_FULL_PATH`, `IDS_MENU_COPY_REL_PATH`, `IDS_MENU_RENAME`, `IDS_MENU_CUT`, `IDS_MENU_COPY`, `IDS_MENU_PASTE`, rename/paste validation errors, `526..560`).
- **Exit criteria**: Right-click & `Shift+F10` menu works on files, subfolders, and root sections; Explorer reveal succeeds cleanly; RedSalamander reveal invokes `--select=<path>` cleanly when detected with `--select` support (and disables/explains cleanly otherwise); Rename & Cut/Copy/Paste keep unsaved editor buffers and recovery intact; offline security tests reject reparse traversal and invalid filenames.

### Phase W4 — GitHub Integration, `Open GitHub` Dialog & User-Repo Autosuggest
- **New/Modified files**:
  - `Common/CredentialStore.h` & `Common/CredentialStore.cpp`: `ICredentialStore` + `WindowsCredentialStore` (`CredWriteW` / `CredReadW` / `CredDeleteW`) + `MemoryCredentialStore` (for headless tests).
  - `Common/GitHubClient.h` & `Common/GitHubClient.cpp`: `IGitHubTransport`, `WinHttpGitHubTransport` (WinHTTP TLS + bounded JSON parsing via `yyjson` + `SanitizeGitHubUiText`), `ParseGitHubUrl()`, `FilterAndRankRepos()`, and `FakeGitHubTransport` (for tests/captures).
  - `RedTilde/GitHubService.h` & `RedTilde/GitHubService.cpp`: Event-driven background worker (`ProcessWorkLease`) for OAuth Device Flow, `/user/repos` cache fetch, recursive git tree listing, and raw Markdown read-through.
  - `RedTilde/Application.h`, `RedTilde/Application.Chrome.cpp`, `RedTilde/Application.Navigation.cpp`, `RedTilde/Application.cpp`: `OverlayKind::OpenGitHubModal` with URL box + live autosuggest list + branch field + inline auth flow; Settings **Accounts / GitHub** section; `_filesTree` GitHub repository section(s); `DocumentOrigin::GitHub` handling (including `Save` -> `Save As…`).
  - `res/Resource.h` & `res/RedTilde.rc`: GitHub UI labels, error/rate-limit diagnostics, device flow text (`561..610`).
- **Exit criteria**: Signing in (mocked in tests / real Device Flow manually) populates autosuggest instantaneously as the user types; pasting any GitHub repo/tree/blob URL opens its `.md`/`.markdown` files in `_filesTree`; edits stay safe and save locally via `Save As…`; zero network I/O on startup or idle; zero tokens persisted on disk in JSON5 or logs.

---

## 4. Normative specifications, machine contracts, and documentation update checklist

When phases `W1`–`W4` are implemented (or phase-by-phase as each lands), the following authoritative files MUST be updated in the same change before closeout:

| File | Required Updates |
| --- | --- |
| [Specs/UI/UI_DesignSystem.md](../../UI/UI_DesignSystem.md) | Document `SaveConfirmModal` layout/checklist/tokens (`W1`), multi-folder & GitHub section headers in `_filesTree` (`W2`), `DxUi::ContextMenu` row actions + `RenameFileModal` (`W3`), `OpenGitHubModal` URL + autosuggest + Accounts section (`W4`), and register new deterministic `--scenario` captures. |
| [Specs/Core/Core_FileSafety.md](../../Core/Core_FileSafety.md) | Update `AskThen`/`AskWorkspaceThen` modal prompt contract for consolidated multi-document DxUi confirmation, **Keep & Exit** hot-exit (`RecoverySnapshotKind::SessionResume` restoring original path + `FileStamp` for direct `Ctrl+S` + external modification conflict protection, distinct from `CrashRecovery` copies), and non-blocking `WM_ENDSESSION` auto-resume (`W1`); add non-reparse Rename/Move/Copy invariants and workspace slot/recovery rebasing rules (`W3`). |
| [Specs/Core/Core_Settings.md](../../Core/Core_Settings.md) | Add `exitUnsavedBehavior` (`W1`), `filesRoots`, legacy `filesRoot` migration/sync, `githubRepos`, `redSalamanderPath`, fallback rules, live reload behavior (`W2`, `W3`, `W4`), and note always-enabled `SessionResume` behavior on OS shutdown/logoff regardless of manual-exit setting; explicitly state Credential Manager secret isolation. |
| [Specs/Core/Core_Architecture.md](../../Core/Core_Architecture.md) | Register new `OverlayKind` variants (`SaveConfirmModal`, `RenameFileModal`, `OpenGitHubModal`, `FileConflictModal`), `FileOpsService` and `GitHubService` explicit worker phase machines + `AreShutdownWorkersSettled()` gates, and `DocumentOrigin::GitHub`. |
| [Specs/Core/Core_PerformanceAndResources.md](../../Core/Core_PerformanceAndResources.md) | Bound multi-folder enumeration (`<= 16` roots × `5,000` entries), autosuggest cache (`<= 500` entries, `O(N)` zero-allocation filter pass), and zero-idle worker sleep invariants. |
| [Specs/Editor/Editor_Markdown.md](../../Editor/Editor_Markdown.md) | Document `MultiFolderFilesService`, Modified-documents cross-folder disambiguation, Rename/Cut/Copy/Paste rebinding, and `DocumentOrigin::GitHub` read-through + local Save As semantics + remote relative-image resolution. |
| [Specs/Testing/Testing_Validation.md](../../Testing/Testing_Validation.md) | Add new behavioral test suites (`SaveDialog`, `MultiFolderFiles`, `GitHub`), offline fault/security probes, and new screenshot capture assertions to `Editor` / `Captures` / `Preferences` scopes. |
| [SettingsStore.schema.json](../../../SettingsStore.schema.json) | Add JSON Schema definitions and bounds (`maxItems: 16`, etc.) for `filesRoots`, `githubRepos`, and `redSalamanderPath`. |
| [capabilities.json](../../../capabilities.json) | Promote completed capability slugs cleanly; keep unverified external OS/live-GitHub/live-RedSalamander handoff gates separated cleanly if manual verification remains separate. |
| [docs/README.md](../../../docs/README.md) | Update end-user documentation for multi-folder workspace sections, full/relative path copying, Reveal in Explorer / RedSalamander, Rename / Cut / Copy / Paste, the new Save confirmation dialog, and **Open GitHub** (`Ctrl+Shift+G`) + autosuggest. |
| [docs/Portable.md](../../../docs/Portable.md) | Update feature summary bullet for multi-folder workspace, file actions, and GitHub browsing, plus Credential Manager token location note (`RedTilde/GitHub/*`). |
| [docs/screenshots/README.md](../../../docs/screenshots/README.md) | Document new 96-DPI Release screenshots (`save-confirm-dialog-light.png`, `files-multi-folder-and-github-light.png`, `files-context-menu-dark.png`, `open-github-autosuggest-light.png`, `open-github-autosuggest-dark.png`) with exact dimensions and scenario provenance. |

---

## 5. Cross-repository prerequisites & planned upstream improvements

### 5.1 `RedSalamander` CLI selection prerequisite
- **Current state in `z:\src\RedSalamander`**: Audited `RedSalamander\RedSalamander.cpp:7130-7162` (`kRedSalamanderHelpText`) and `RedLauncher\Main.cpp` — `RedSalamander.exe` currently accepts only diagnostic/selftest switches (`--etw`, `--perf`, `--selftest`, etc.) and does **not** yet accept a file-selection or pane-directory startup argument (`--select=<path>`, `--left=<dir>`), nor forward reveal requests to an already-running instance.
- **Required upstream `RedSalamander` change** *(sequenced in `RedSalamander`, independent of RedTilde's own `W1`/`W2`/`W4` rollout)*:
  1. Add `--select=<absolute-path>` (and `--left=<dir>` / `--right=<dir>`) CLI option parsing in `RedSalamander.exe` (and clean pass-through in `RedLauncher.exe`), selecting the target item in the active folder pane without DLL-planting exposure (preserving `SetDefaultDllDirectories` + launcher package CWD hardening).
  2. Support safe single-instance activation/handoff (or opening a new instance per RedSalamander policy) when invoked from external consumers like `RedTilde`.
- **RedTilde contract while awaiting or missing RedSalamander**:
  - RedTilde discovers installed `RedSalamander.exe` safely from trusted App Paths / Winget / per-user installation locations without running arbitrary `PATH` searches.
  - When `--select` support is absent or `RedSalamander.exe` is not installed, **Reveal in RedSalamander** degrades gracefully (disabled with an explanatory tooltip rather than silently opening to an unrelated default folder).

### 5.2 Optional `DxUi` improvements (non-blocking for `W1`–`W4`)
All of **W1–W4** ship on the current `DxUi` API 5 baseline (`0c371a6197eb8c6b876b4b2843d75078bc21035c`) using `Surface::Kind::Floating`, `Tree`, `CheckBox`, `TextField`, `Button`, `ComboBox`, and `ContextMenu::ShowAsync`. A later standalone `DxUi` enhancement pass (paired with its own `DxUi.lock.json` bump) would simplify RedTilde by moving generic chrome primitives upstream:

1. **`DxUi::ModalDialog` / `ModalHost` helper**: Centered dialog framing, full-client `theme.smokeOverlay` scrim routing, focus-trap `Tab` cycling, default-Enter / `Esc`-Cancel wiring, and UIA `ControlType=Window/Dialog` pattern built once in `DxUi` rather than per modal (`SaveConfirmModal`, `RenameFileModal`, `OpenGitHubModal`, `BlockDraft`, `RecoveryPanel`).
2. **`DxUi::AutoSuggestBox` control**: Composite `TextField` + keyboard-navigable filterable dropdown popup (`Down`/`Up`/`Enter`/`Esc`, UIA `ExpandCollapse` + `Combobox` semantics) replacing custom suggestion-list orchestration in `OpenGitHubModal`.
3. **`DxUi::TreeItemData` presentation flags**: Native `isSectionHeader` styling (uppercase/subtle divider + inline section action slot) and `isCut` (50% dimmed icon/text opacity per Explorer Cut semantics) so RedTilde does not need custom row adornments for folder section headers or pending Cut operations.

---

## 6. Testing, captures, and validation gates

### Automated behavioral & negative security tests (all headless & offline)
1. **`Tests/Application.Tests.SaveDialog.cpp`** (`W1`):
   - Single-document dirty exit/New/Open shows **Save**, **Keep & Exit** (on `CloseWindow`), **Don't Save**, **Cancel**, displays exact filename and relative path to open folder (and parent folder when unrooted, repo-relative path for GitHub docs, and `(Untitled)` for unnamed buffers), exposes full path in tooltip/UIA, and restores focus on `Esc`.
   - Multi-document dirty exit consolidates `>= 2` dirty files across multiple open folders into one checklist, tests partial check selection (`1` saved, `1` discarded), tests `Cancel` preserving all edits, and verifies snapshot-change revalidation during async save.
   - **Keep & Exit & Shutdown Auto-Resume**:
     - Clicking **Keep & Exit** (or setting `exitUnsavedBehavior = "keepAndResume"`) closes cleanly without overwriting disk files and persists `SessionResume` snapshots.
     - Relaunch automatically restores all dirty documents into active/inactive slots in original order (preserving path, BOM, selection, and `FileStamp`) with **no `RecoveryModal`**, ready to continue editing and save directly with `Ctrl+S`.
     - Modifying an auto-resumed file externally on disk while RedTilde is closed triggers **SaveConflict** (`Reload / Save As / Overwrite / Cancel`) on the next `Ctrl+S` instead of silently overwriting disk changes.
     - Launching with an explicit CLI `--document` preserves auto-resumed dirty documents in `_inactiveDocuments` (**Modified documents**).
     - `WM_QUERYENDSESSION` returns `TRUE` immediately with zero modal prompt, and `WM_ENDSESSION(TRUE)` flushes `SessionResume` snapshots so next launch resumes cleanly while `CrashRecovery` snapshots still trigger `RecoveryModal`.
2. **`Tests/Application.Tests.MultiFolderFiles.cpp`** (`W2` & `W3`):
   - Persistence, cold load, live reload, legacy `filesRoot` migration, duplicate folder name disambiguation, and capacity cap (`16` roots) for `filesRoots`.
   - Simultaneous filtering, dirty-dot sync across **Modified documents** and nested local folder sections, and worker shutdown drainage.
   - `OnTreeContextMenu` / `Shift+F10` matrix verification across file, subfolder, root section, Modified doc, and GitHub items.
   - **Copy Full Path** & **Copy Relative Path** via injected clipboard test seam (exact Windows path, relative path, and HTTPS URLs).
   - **Rename (`F2`)** validation: illegal characters, reserved DOS names, extension-change warning, duplicate filename refusal, reparse-point refusal (`IsReparseTagNameSurrogate`), and preserving dirty edits/history/recovery across active and inactive workspace slot renames.
   - **Cut / Copy / Paste** via offline clipboard seam: `DROPEFFECT_COPY` unique-name generation (`- Copy.md`), cross-folder collision modal (`Keep Both / Replace / Cancel`), `DROPEFFECT_MOVE` workspace path rebasing, and non-reparse / non-allowed-extension refusal.
   - **Reveal in Explorer** & **Reveal in RedSalamander** tested through `IShellReveal` seam (recording exact arguments and trusted executable path discovery without launching real external processes or reading unverified `PATH`).
3. **`Tests/Application.Tests.GitHub.cpp`** (`W4`):
   - URL parser (`https://github.com/owner/repo`, `/tree/branch`, `/blob/branch/path`, `owner/repo`) plus rejection of non-HTTPS schemes, non-GitHub hosts, embedded credentials, path traversal `..`, and control characters.
   - `ICredentialStore` memory double verifying `CredWrite`/`CredDelete` target isolation, zero token leakage in serialized `settings.json5`, diagnostics logs, or UI tooltips.
   - `FakeGitHubTransport` verifying: OAuth Device Flow polling + cancel + expiration, `/user/repos` pagination up to `500`, instant substring/prefix autosuggest ranking & keyboard navigation, recursive `.md`/`.markdown` filtering, `truncated` warning banner, `403`/`429` rate-limit UI message without crash, `SanitizeUiText` neutralization of bidi overrides (`U+202E`) and control chars in remote repo names/descriptions, `DocumentOrigin::GitHub` local editing + `Save` -> `Save As…` routing, and zero network calls during cold startup with persisted `githubRepos`.

### Deterministic 96-DPI Release Capture Scenarios
Added to `Application.CaptureScenarios.cpp` (hidden `WS_EX_NOACTIVATE`, synthetic fixtures only) and documented in `docs/screenshots/README.md`:
1. `save-confirm-dialog-light` (`save-confirm-dialog-light.png`, `1440×900`) — multi-document DxUi save confirmation dialog displaying filename + folder-relative paths + checkboxes alongside **Save Selected**, **Keep & Exit**, **Don't Save Any**, and **Cancel**.
2. `files-multi-folder-and-github-light` (`files-multi-folder-and-github-light.png`, `1440×900`) — left Files pane showing **Modified documents**, two distinct local folder sections (`Notes` and `Engineering (Workspace)`), and a loaded GitHub repository section (`octocat/handbook · main`).
3. `files-context-menu-dark` (`files-context-menu-dark.png`, `1440×900`) — DxUi context menu open over a file row showing Reveal in File Explorer, Reveal in RedSalamander, Copy Full Path, Copy Relative Path, Rename, Cut, Copy, Paste.
4. `open-github-autosuggest-light` & `open-github-autosuggest-dark` (`1440×900`) — Open GitHub modal with URL input, signed-in `@alex-writer` pill, and live filtered autosuggest dropdown from `FakeGitHubTransport`.

### Required validation gate before moving this plan to `Done`
- `.\format.ps1 -Check`, `.\validate.ps1`, `.\Tools\Test-Changelog.ps1 -Base origin/main` green.
- All 6 build profiles (`x64` & `ARM64` × `Debug`, `Release`, `ASan Debug`) succeed with `0 warning(s), 0 error(s)`.
- `x64 Debug` (`Core`), `x64 ASan Debug` (`Core` + ASan probe), and `x64 Release` (`Storage,Settings,Recovery,Editor,Preferences,RecoveryRestart,Performance,Captures`) pass completely, with paired Release performance comparison confirming no idle CPU or steady-state typing/scroll regression.
- Affected normative specs (`Specs/`), schema (`SettingsStore.schema.json`), `capabilities.json`, `docs/`, and 96-DPI Release screenshots updated and inspected.

### Completion receipt (`2026-11-24`)
- **6-profile compilation (`0 warning(s), 0 error(s)`)**: `x64` & `ARM64` × `Debug`, `Release`, `ASan Debug` all built cleanly at DxUi pin `0c371a6197eb8c6b876b4b2843d75078bc21035c`.
- **`x64 Debug` `Core` (`221/221`)**: `Storage` (36), `Settings` (22, including `Settings.MultiFolderGitHubAndExitBehaviorRoundTrip`), `Recovery` (24, including `Recovery.SessionResumeSnapshotRoundTripAndCleanup`), `EditorContent` (68), `EditorWorkspace` (37, including `Editor.WorkspaceSaveConfirmSingleMultiAndChecklist`, `Editor.WorkspaceKeepAndExitResumeAndShutdown`, `Editor.FilesMultiFolderSectionsAndContextMenuOps`, `Editor.WorkspaceGitHubUrlAutosuggestAuthAndOpen`), `EditorMedia` (28), `EditorPerformance` (3), `EditorRendering` (3), plus `Failure report routing verified (Debug, exit 3)` (`125.6 FPS`, `87.0 MiB`, `3.51 ms`; receipt `20261124T172420563Z-89368-cc007a146edc4a27bdb016dd5a0457fa`).
- **`x64 ASan Debug` `Core` + `Sanitizer` (`221/221`)**: Zero AddressSanitizer errors across all partitions, failure-report routing (`exit 3`), and `Real AddressSanitizer heap-use-after-free detection verified` (`126.2 FPS`, `464.5 MiB`, `10.43 ms`; receipt `20261124T172815497Z-100640-d0348679539f4265bdd8c9e65aa7d28b`).
- **`x64 Release` (`Storage,Settings,Recovery,Editor,Preferences,RecoveryRestart,Performance,Captures`)**: `221/221` behavioral tests + `53` hidden captures including `save-confirm-dialog-light.png`, `files-multi-folder-and-github-light.png`, `files-context-menu-dark.png`, `open-github-autosuggest-light.png`, and `open-github-autosuggest-dark.png` copied and visually verified under `docs/screenshots/`; `Native forced render: 125.9 FPS; private memory 73.0 MiB; dirty edit/render 0.84 ms` (receipt `20261124T171912748Z-95548-c621321d506f419d8da3c611f35cd80d`).
- **Explicit deferred/external gates**: `RedSalamander --select` CLI support in sibling `RedSalamander`, physical ARM64 runtime qualification, interactive desktop focus/Narrator UIA pass (`DesignInteractive`), and manual live GitHub OAuth Device Flow smoke against `github.com`.
