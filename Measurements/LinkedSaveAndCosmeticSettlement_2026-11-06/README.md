# Linked-file saves and independent cosmetic settlement

Implementation: `020cb0b9480e4560d61d4b2b181a3793cb4a7ef9`.
Dependency: API 4 `ba94f4eb0cd8bbdeecb0b96d5e21fb325fe2d9c9`.

The [local verification](local-verification.json) binds all six zero-warning builds and 141 original compiled
inputs at source identity `d61f23122799323153c044b6c3689266d5baadcd03b86984f1cbc4a730f84def`.
Each x64 profile passes 116 cases and 36 standard captures. The actual symbolic-link fixture fails with Win32
1314 before it can create its owned link. That failure remains visible: these are incomplete local runtime
results. ARM64 entries are cross-builds. Complete symbolic-link and native-platform acceptance requires
separately source-bound hosted execution.

The frozen original hardlink counterexample returned save success while breaking shared identity and leaving
the other alias unchanged. The new writer locks the shared file, validates its complete identity and expected
bytes, and durably publishes an independent full-source recovery record before its first in-place mutation.
Successful saves preserve all aliases. Write, partial-write and flush failures retain the old saved baseline
and complete admitted recovery source; successful writes retire only their own protection record. Admission
failure, bounded recovery inventory and blocked retirement have independent directed cases.

A real child process terminates after the partial shared-target write is flushed. A fresh process independently
checks both aliases, their shared identity, the actual partial bytes and the complete recovery source with its
path, BOM, selection and generation. Release, Debug and ASan all pass this crash/restart scenario. ASan also
detects the deliberately injected use-after-free. The application worker case verifies that later edits remain
dirty while only the admitted source is saved.

The new SDK makes source/code ink, line numbers and whitespace independent of content readiness. A new hidden
application test failed with the old capture-drain loop at this same SDK pin: 62 of 63 EditorBehavior cases
passed, while the independently constructed maximum-row source fixture captured unfinished cosmetics. The
application harness now waits separately for the visible editors' pending cosmetic work. It preserves the
production event loop's earlier frame/input boundary. All 69 Editor cases pass in each local x64 profile;
the 36 standard captures are counted separately from that additional directed fixture.

The [retained attempts](retained-attempts.json) preserve the original alias-splitting baseline, focused checks,
vendor-warning setup, local symbolic-link refusals, consumer counterexample and rejected C2664 compile attempt.
Original source archive members and retained archive digests were reverified. A later passing result never
changes those attempts' dispositions.

Durable behavior belongs to [file safety](../../Specs/Core/Core_FileSafety.md),
[validation](../../Specs/Testing/Testing_Validation.md) and
[build admission](../../Specs/Build/Build_ConsoleAndPaths.md); usage is in the
[application guide](../../docs/README.md). The [single active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md)
keeps Q12 open until actual symbolic-link execution is accepted. Physical input/AT, real-share/cloud/WSL and
paired-performance acceptance remain separate. Unpaired diagnostics establish no performance verdict.
