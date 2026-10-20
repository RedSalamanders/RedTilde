# Tests

Run `test.ps1 -Configuration Debug -Suite Core`; use `-SkipBuild` for the exact existing artifact.
`-Suite Captures` generates non-activating application renders; `-Suite Crash` runs an isolated child fault.
`-Suite All` includes all three. ASan Core/All must detect an isolated intentional heap-use-after-free.

Native cases are compiled from `Scope.Tests.Core.cpp` into the same executable and run with `--self-test`.
Core also runs a hidden child-process probe that changes preferences, closes immediately and verifies the final state after process exit.
They exercise public controls and app commands without desktop input, clipboard access or foreground activation.
Receipts and failed logs remain under `.build/tests/<run>`. The validation contract lists broader gates.
