# Find, zoom, window placement and recovery-first startup

Implementation: `2709f933ce3bb385be0031f3ab7cf78ef4c7004b`.
Dependency: API 4 `a1915ffdd94b18fd1817371f1dc074f6a0b547f1`.

[Local verification](local-verification.json) binds all six zero-warning builds to the 141 original compiled
input files and their retained source archive. Each native x64 Release, Debug and ASan Debug cohort passes
127 of the 128 registered cases and all 36 standard captures. The actual symbolic-link fixture fails with
Win32 1314 because this machine cannot create that fixture. These cohorts remain incomplete; ARM64 entries
are cross-builds. The real ASan detector and actual partial-write crash/restart checks pass.

The [hosted Release record](hosted-qualification.json) verifies main `daa022eb`, all seven completed jobs,
the original thirteen artifact wrappers, canonical receipts and both test-disabled production ZIPs at
`0.1.342`. Native x64 and ARM64 each pass all 128 cases, 36 captures and the Release assertion probe. The
combined test gate completes before either production role starts; both ZIPs pass clean-extraction smoke.

The [extended hosted record](extended-hosted-qualification.json) verifies source `bb6e12ff` in native x64
and ARM64 Debug and ASan Debug. Every profile passes all 128 cases and 36 captures; both ASan detectors
report the real expected use-after-free. All four actual symbolic-link cases pass. Original native heap
observations and all eight controlled provider boundaries are independently checked. The current ARM64
ASan walk observes one entry and correctly completes at a one-entry budget; that observation does not
reconstruct the unknown entry count in the original failed cohort. Together the hosted records cover
768 native cases and 216 captures across all six profile/architecture combinations.

The 78 Editor cases cover ordinal case-insensitive Find, Match case and closed-bar repeat; exact Unicode/CRLF
selection and history during text scaling; validated window placement; recovery-first asynchronous startup
that preserves newer intent; and explicit link confirmation with an owned STA handoff and teardown. Link
tests use an injected launcher and do not open a browser. The hidden Find capture was visually reviewed.
Eight deterministic heap-provider boundaries supplement independently observed native heap membership.

[Retained attempts](retained-attempts.json) preserve the original invalid-CRLF fixture cohort and ARM64 compiler
disk-full failure. Neither is relabelled as passed. The original ARM64 ASan heap-fixture failure remains in the
[earlier hosted packet](../LinkedSaveAndCosmeticSettlement_2026-11-06/extended-hosted-qualification.json).

Durable contracts are in [editor behavior](../../Specs/Editor/Editor_Markdown.md),
[settings and startup](../../Specs/Core/Core_SettingsAndCrashes.md),
[architecture](../../Specs/Core/Core_Architecture.md), [UI](../../Specs/UI/UI_DesignSystem.md), and
[validation](../../Specs/Testing/Testing_Validation.md). Usage is in the [user guide](../../docs/README.md).
The [scoped Done record](../../Specs/Plans/Done/RemainingInteractions_2026-11-06.md) closes the implementation
and these source-specific functional gates. The [single remaining-work plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md)
retains physical input/AT, real browser association, supported remote locations, API-5 adoption and paired
performance. No complete-repository or product-release publication acceptance is claimed here.
