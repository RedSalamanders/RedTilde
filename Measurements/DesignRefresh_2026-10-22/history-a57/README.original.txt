# Design refresh evidence index — 2026-10-22

Status: the current pinned candidate has completed its six-profile build matrix and all three x64 native Full
runs and the nine-stage RedTilde keyboard sequence. This is build/runtime and application interaction evidence,
not final product acceptance. Paired performance and the external gates below remain open.

## Candidate identity and qualification

The candidate uses DxUi pin [`a17f7d7b`](../../Dependencies/DxUi.lock.json), full commit
`a17f7d7b59536741a0496638ac44d0968bf74e78`. Its source identity is
`a57da06500767414d25dbae458330f03891c877fbc083edc53a8fddadc3b79fd`. The current
[runtime source-input record](validation/RuntimeSourceInputs.json) hashes the source closure, and the
[runtime artifact-closure record](validation/RuntimeArtifactClosures.json) binds each built profile to its
artifact, build-receipt and binary hashes.

| Profile | Build | Runtime | Runtime evidence |
| --- | --- | --- | --- |
| x64 Debug | Pass | Full pass: 52 cases, 20 captures | [receipt](native/x64-Debug/payload/receipt.json), [archive](native/x64-Debug/archive.json) |
| x64 Release | Pass | Full pass: 52 cases, 20 captures | [receipt](native/x64-Release/payload/receipt.json), [archive](native/x64-Release/archive.json) |
| x64 ASan Debug | Pass | Full pass: 52 cases, 20 captures; real sanitizer-detection probe passed | [receipt](native/x64-ASan-Debug/payload/receipt.json), [archive](native/x64-ASan-Debug/archive.json) |
| ARM64 Debug | Pass | Build only; native runtime not executed | [build record](validation/ARM64-Debug-build.json) |
| ARM64 Release | Pass | Build only; native runtime not executed | [build record](validation/ARM64-Release-build.json) |
| ARM64 ASan Debug | Pass | Build only; native runtime not executed | [build record](validation/ARM64-ASan-Debug-build.json) |

The [matrix summary](validation/Matrix.json) records all six profiles, the shared source identity, case and
capture counts, and the ASan detector result. The three x64 receipts each report successful execution of the
52-case Full union and 20 actual captures. ARM64 results establish compilation only.

Selected final Debug captures: [Compose Light](native/x64-Debug/payload/compose-light.png),
[Settings](native/x64-Debug/payload/settings.png), and [narrow layout at 96 DPI](native/x64-Debug/payload/narrow-dpi-96.png),
[144 DPI](native/x64-Debug/payload/narrow-dpi-144.png), and [192 DPI](native/x64-Debug/payload/narrow-dpi-192.png).
The [Release capture comparison](validation/CaptureComparison.json) reports 19 of 20 PNGs byte-identical to the
prior QA set. `settings.png` is the sole changed capture; the final System-theme preview was visually checked
against the corrected system appearance.

The archived packets retain selected raw JSON records, the top-level PNGs and logs, and each original outer
manifest as metadata. They intentionally omit private fixture trees, built binaries and crash dumps; the
hashes and identities remain available for audit without claiming those omitted files are included.

## Real keyboard and restoration

The final [nine-stage keyboard child](native/keyboard-x64-Release/payload/design-interactive.json) passed all
stages with 168 real input events through the ordinary message pump. It covers typing/undo, Files, Find,
Settings, Focus, Alt+F10/Tab/Enter/Escape formatting, F2 Apply/Undo/Cancel, the File menu and Alt+F4 asynchronous
close. The [source-bound receipt](native/keyboard-x64-Release/payload/receipt.json) binds the same Release binary
and pin as the Full run. The canonical [lease restoration result](native/keyboard-x64-Release/payload/interactive-result.txt)
records foreground, focus and cursor unchanged after the 2.5-second child; clipboard access was not used.
Physical IME, touch and assistive technology are separate gates. The longer canonical DxUi desktop matrix is
awaiting a separate timing choice and is not covered by this application's nine-stage pass.

## Baseline and comparison scope

The [baseline archive manifest](baseline/archive.json) inventories the retained raw baseline receipts,
compiler-command record, preparation metadata and Git bundle. The [bundle manifest](baseline/bundle.json)
identifies a verified thin two-commit bundle with prerequisite `f819395d36c55b93e3bf9559d12d0ec401e42b3d`,
tip `98d03b2e18ef2c532eaa4c915dd64f8f0f36aaa6`, and commits `19a0f88d211f95c5b9878b3ba2f20c8061f7c31b`
and `98d03b2e18ef2c532eaa4c915dd64f8f0f36aaa6`. The comparison is original RedTilde UI versus refreshed UI
with the same updated DxUi pin and seven common unchanged inputs, as recorded in
[`preparation.json`](baseline/preparation.json). This is a scoped paired-study baseline, not a measure of the
historical total SDK cost. The baseline executable remains private; its recorded binary SHA-256 is in
[`baseline.json`](baseline/baseline.json).

The performance comparison itself has not run. Baseline preparation and matching compiler/build metadata do
not establish paired latency, throughput or non-regression.

## Superseded and failed evidence

The earlier hidden Debug Full run `20261022T215221918Z-28080-8ffc487303d94a2bb848a1bfa8608aed` and Release
Full run `20261022T215554379Z-8844-fd8bd3753c1a47c19cef7fee6547142c` each recorded 52 cases, 20 captures and
probes at the superseded `4cb` candidate. They are historical and are superseded by the current `a17f7d7b`
matrix above.

The failed `4cb` [native translation diagnostics](failures/native-translation-4cb/tests.json) and
[receipt](failures/native-translation-4cb/receipt.json) remain unchanged. The failed desktop run
`20261022T215719296Z-43944-85336cee450d48e89b82344df4a42afb` passed keyboard stages 1–5 and failed stage 6
(Bold) when activation leaked a character into the editor. Foreground focus was restored and the cursor was
unchanged. The original [interactive child result](failures/keyboard-4cb/design-interactive.json),
[restoration record](failures/keyboard-4cb/interactive-result.txt), and
[file-hash manifest](failures/files.json) are preserved. The final `a17f7d7b` keyboard sequence above passes the
formerly failing activation and all remaining stages.

## Remaining gates

- Resolve the timing choice for the longer shared DxUi keyboard/menu matrix; omitted profiles remain open.
- Run the paired performance study and archive its valid measurements.
- Close plan R10 and the broader creation C1–C12 requirements; this evidence packet does not close them.
- Complete external native ARM64 runtime, physical IME, touch, assistive-technology and release acceptance.

The durable behavior contracts are in the [design system](../../Specs/UI/UI_DesignSystem.md),
[Markdown editor contract](../../Specs/Editor/Editor_Markdown.md),
[application architecture](../../Specs/Core/Core_Architecture.md),
[settings and crash contract](../../Specs/Core/Core_SettingsAndCrashes.md), and
[validation contract](../../Specs/Testing/Testing_Validation.md). The
[WIP plan](../../Specs/Plans/WIP/DesignRefresh_2026-10-22.md) remains open until its remaining acceptance gates close.
