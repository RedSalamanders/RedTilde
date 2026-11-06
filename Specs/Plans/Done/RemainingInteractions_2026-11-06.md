# Find, zoom, window placement and recovery-first startup

Status: DONE — Q9/Q14 implementation and source-specific native functional qualification.
Implementation: `2709f933ce3bb385be0031f3ab7cf78ef4c7004b`.
Dependency: API 4 `a1915ffdd94b18fd1817371f1dc074f6a0b547f1`.

- [x] Implement ordinal case-insensitive Find, Match case, closed-bar F3/Shift-F3 repeat and one-action
  replacement advance with exact Unicode/CRLF, source, selection and history oracles.
- [x] Implement Ctrl/OEM/keypad/reset and high-resolution wheel text scaling with bounded accumulation,
  clamping, AltGr handling and unchanged source/history in Compose, Source and Split.
- [x] Validate normal window bounds/DPI/maximized preferences, clamp to attached work areas, coalesce
  persistence and preserve explicit launch-show precedence without stealing another window's focus.
- [x] Restore the remembered document only after recovery scanning settles, preserve every newer edit or
  explicit New/Open intent, and exercise actual asynchronous startup/cancellation paths.
- [x] Confirm explicit HTTP/HTTPS/mailto opening with a default-No prompt, revalidate source/selection/lifetime,
  and use an owned STA handoff with bounded admission and teardown-safe completion. Tests inject the launcher.
- [x] Replace the heap fixture's unconditional truncation assumption with independent native membership
  observations and eight controlled boundaries, retaining raw reports before assertions.
- [x] Adopt the exact text-first SDK pin, update durable editor/settings/UI/build/testing contracts and usage.
- [x] Retain six zero-warning local builds, real ASan detection and all original source/native/run artifacts.
  Each local x64 cohort retains its actual symbolic-link privilege failure; it is not relabelled.
- [x] Verify native x64/ARM64 Release, Debug and ASan Debug: 128 cases and 36 captures per profile, actual
  symbolic-link saves, both real ASan detectors and source-bound canonical/native build evidence.
- [x] Verify both test-disabled production ZIPs at `0.1.342`, smoke, original package hashes and combined
  test-gate ordering. Preserve the rejected CRLF fixture cohort and native compiler disk-full attempt.

The [packet](../../../Measurements/RemainingInteractions_2026-11-06/README.md) records the original wrappers,
canonical receipts, six-profile evidence and production identities. The original failed ARM64 ASan cohort
remains failed; the new one-entry observation does not establish its missing historical entry count.

Physical keyboard/IME/touch/AT, real browser association, cloud/SMB/WSL acceptance, image assets/settings,
application API-5 migration, installer/file associations and paired performance remain in the
[single active plan](../WIP/RemainingWork_2026-11-04.md). This is functional qualification of the recorded source,
not complete-repository or product-release publication acceptance.
