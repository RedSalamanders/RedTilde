# Find, zoom, window placement and recovery-first startup

Implementation: `2709f933ce3bb385be0031f3ab7cf78ef4c7004b`.
Dependency: API 4 `a1915ffdd94b18fd1817371f1dc074f6a0b547f1`.

[Local verification](local-verification.json) binds all six zero-warning builds to the 141 original compiled
input files and their retained source archive. Each native x64 Release, Debug and ASan Debug cohort passes
127 of the 128 registered cases and all 36 standard captures. The actual symbolic-link fixture fails with
Win32 1314 because this machine cannot create that fixture. These cohorts remain incomplete; ARM64 entries
are cross-builds. The real ASan detector and actual partial-write crash/restart checks pass.

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
The [single remaining-work plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md) retains current-source
hosted native acceptance, physical input/AT, real browser association, supported remote locations and paired
performance. No API-5 application adoption or complete-repository acceptance is claimed here.
