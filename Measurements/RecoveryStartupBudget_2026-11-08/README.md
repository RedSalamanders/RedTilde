# Recovery startup entry budget

This packet binds the startup correction to implementation
`ace3417f73bc5de2259d83a34de65c0c0f989a97`, with API-5 SDK
`21fae0dc66000b04504ba7862169533ac2943be8`.

The old loop filtered names before consuming its 512-entry budget. A directory
filled with unrelated names could therefore bypass the limit. The corrected
worker consumes the entry budget before name filtering, publishes synchronized
inspection statistics before Ready, and retains every unrelated file.

The focused actual-worker counterexample inspected 577 entries, including its own
lease, and returned native exit 11 at the expected failed assertion. The corrected
worker inspected 512 entries, reported a remainder and returned exit 0. Its retained
EXE/PDB, relevant source bytes and raw logs are separate from application qualification;
the standalone diagnostic does not claim a pre-compilation freeze of every transitive input.

[Local qualification](qualification.json) preserves 279 actual application source
inputs, including the 153 inputs in the compiled scope, and verifies every original
source/native/SDK/run archive member. All six builds have zero warnings/errors.
All 126 nonstorage cases and 40 hidden captures pass in each x64 Debug, Release and
ASan Debug profile, including the actual startup-worker regression. ASan detects its
real fault. Local symbolic-link privilege failures remain actual, unwaived results.
The three ARM64 profiles are compilation-only.

Hosted x64 acceptance has its own source/build/run identities and must verify all
156 cases, including all 30 Storage cases, before the scoped closeout moves to Done.
It cannot be inferred from an earlier API-5 run or from local cross-compilation.

The editing and native checkouts now have identical source bytes. Their prior 26
line-ending differences, original editing bytes and matching Git-blob proofs remain
retained. Setup, helper and cache-import refusals are preserved separately; no failed
setup is counted as a native product pass.

The enduring contract is in [file safety](../../Specs/Core/Core_FileSafety.md) and
[validation](../../Specs/Testing/Testing_Validation.md), with user guidance in
[the writing guide](../../docs/README.md). The broader
[parent plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md) keeps its remaining
review, provider, physical-input, performance, history and release requirements.
This work changes startup bounds rather than ordinary pixels; new public screenshots
are unnecessary. An entry count does not establish filesystem latency or paired performance.
