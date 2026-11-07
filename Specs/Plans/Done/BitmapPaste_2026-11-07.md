# Bitmap paste — completed local implementation scope

Status: complete for the app-owned bitmap asset worker, text/image preference,
deterministic tests, specs, docs, six local builds and six hosted native profiles.
Both test-disabled production packages pass their original smoke checks.

Compose and Split accept supported bitmap data through a bounded application worker.
The worker encodes and durably creates a unique local PNG before the UI owner
rechecks source, selection, document and lifetime. A successful insertion replaces
the selection in one Undo step. Edits, supersession, Close and final session end
retire the pending insertion; late results cannot change another document.
Text-first remains the default, with a persisted image-first preference.

The normative owners are [editing](../../Editor/Editor_Markdown.md),
[file safety](../../Core/Core_FileSafety.md) and [settings](../../Core/Core_SettingsAndCrashes.md).
The [usage guide](../../../docs/README.md) explains preference and asset portability.
The [evidence packet](../../../Measurements/BitmapPaste_2026-11-07/README.md) retains
the admission counterexample, original functional/visual failures, independent
pixel/identity oracles and corrected actual-theme harness captures.

At `e08cdb29`, all six profiles build with zero warnings/errors. Each x64 profile
passes 139 cases, retains the actual Win32 1314 symbolic-link privilege failure,
and generates 38 captures. Actual ASan and interrupted-write restart execute.
Every original source/native/SDK/run archive member is independently verified.
ARM64 entries are cross-builds. This local scope does not accept the symlink case,
hosted native platforms, production packages or the person's OS clipboard.

The subsequent [Debug/ASan receipts](../../../Measurements/BitmapPaste_2026-11-07/hosted-debug-asan.json)
at main `4761b264` and [Release/production receipts](../../../Measurements/BitmapPaste_2026-11-07/hosted-release.json)
at main `8d68693a` verify all six native x64/ARM64 profiles: each passes 140 cases,
including the actual symbolic-link save, and 38 harness captures. This is 840 cases
and 228 captures in total. Every original wrapper and canonical payload member is
verified. Both production ZIPs are version `0.1.378`, start only after the combined
native Release gate, and pass all 13 test-entry-point refusals each.

The [remaining plan](../WIP/RemainingWork_2026-11-04.md) continues to own API-5
adoption, real locations, remote-image policy, drag/drop/export,
physical input/AT and formal performance qualification.
