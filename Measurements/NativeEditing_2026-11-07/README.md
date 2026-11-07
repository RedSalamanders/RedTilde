# Native editing through the adopted SDK

At `abf5ee69aa76db3a38abde8c4182dd77741afd9a`, RedTilde qualifies four software
paths through its unchanged API-4 SDK pin
`a1915ffdd94b18fd1817371f1dc074f6a0b547f1`. No competing editor implementation or
SDK private test header is introduced.

- The hidden application's actual Source Enter/Shift Enter messages preserve LF,
  CRLF and CR, exact caret position, one-step Undo/Redo and recovery activity.
- The app-owned editor component uses the actual public native ACP store on a
  host without an HWND. Query and locked insertion enforce 65,536 UTF-16 units,
  preserve rejected source, count notifications/history and reject a retired target.
- A separate hidden application test preserves the last valid Unicode composition
  preview, source snapshot, selection, mirror and history on overflow, then admits
  an exact-limit preview and one committed Undo step.
- Compose, Source and Split reject hostile quote/newline file names before
  replacement and still prepare ordinary file references. No OS clipboard is used.

The [qualification record](qualification.json) verifies all 145 original compiled
inputs and every original native/SDK/run archive member. All six profiles build
with zero warnings/errors. Each x64 profile passes 143 cases and produces 38
captures; the actual symbolic-link fixture fails with Win32 1314 and remains an
unaccepted local capability boundary. Actual ASan detection and interrupted-write
restart execute. ARM64 entries are cross-builds. Full validators and formatting
pass on the final implementation; later closeout metadata is checked separately.

The [first-attempt record](first-attempts.json) keeps two rejected cohorts separate.
The first fixture requested a live ACP store on a deliberately hidden HWND and
failed before evaluating capacity. The correction separates the headless native
component from the hidden app preview path, preserving the native focus policy.
The next ARM64 Debug build correctly refused 12 WRL volatile-warning records.
The consumer COM fixture now uses an atomic reference count and RAII retirement;
no warning policy was suppressed. The final six-profile matrix is a fresh source
cohort, not an acceptance of either failed attempt.

At exact main `1a654d17824672e55b5555417d566a9bea5fe09b`, the original
[Debug/ASan receipts](hosted-debug-asan.json) and
[Release/production receipts](hosted-release.json) verify all six native x64/ARM64
profiles: 144 passing cases and 38 captures each, totaling 864 cases and 228
captures. Each profile includes the four named editing cases and the actual
symbolic-link save. ASan detects the real controlled memory error, and the
interrupted-write restart retains complete admitted source.

Both version 0.1.389 production ZIPs pass their native smoke checks and all 13
test-only entry-point refusals each. Both production jobs start after the combined
native test gate. Every original wrapper and canonical payload member is byte
verified; package and executable hashes match the original smoke receipts. These
are internal qualified packages, with no GitHub release publication implied.

The [active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md) retains
physical IME/AT/keyboard, formal performance and API-5 adoption. The
headless ACP component and hidden preview checks do not qualify physical IME or
foreground input. Existing gallery pixels are unchanged by this test-only scope.
