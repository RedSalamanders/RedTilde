# Independent tooling continuation qualification

Code candidate: a513b3cd5a28fb7413aff7b08cef1fc90ca7c6ed.
Dependency pin: 21fae0dc66000b04504ba7862169533ac2943be8, unchanged.
[Original qualification references](qualification.json) retain exact source and log hashes.

All 61 focused public-runner/tooling controls, complete repository validators,
static caller checks and clean formatting feedback passed. Actual noninteractive
x64 Debug, Release and ASan Debug passed, including all rendering partitions,
40 hidden captures per profile, isolated crash reports and the real ASan detector.
All six native build profiles passed with zero warnings/errors; ARM64 is cross-build
evidence only and grants no local ARM64 runtime claim.

Actual Release and ASan runs reused the byte-identical completed Tooling and Format
receipts from the initial successful Debug run. Recording controls separately prove
retention after native failure, no receipt for failed/mutated tooling, and refusal
to reuse earlier evidence after source mutation. Native and whole-run baseline
checks remain separate.

The initial long worktree failed design-fixture seeding at the Windows path budget.
Its failed result and completed independent tooling records remain preserved. The
same commit then passed in a fresh shorter owned checkout without changed assertions
or copied native/tooling receipts. General extended-path product support is unchanged.

The orchestration driver lost an imported helper after its first ARM64 build force-
imported canonical modules. That build itself passed. Continuation reopened all four
existing profiles through canonical artifact admission, then completed the remaining
two builds; passed native tests were not repeated. The original driver error remains
preserved. No paired 3x/2x performance or hosted native acceptance is claimed here.