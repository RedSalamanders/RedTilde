# SDK qualification subset

Library/runtime producer: `eb0f10b91bb8de19b0d7671cc7078fd0c1f5e767`.
[The manifest](manifest.json) binds 33 untouched receipts, logs and status records.
The nineteen `*-asan.json` suite receipts from one final full run all have exit
code zero and no capability skips. [The detector](asan-detector.json) records the
expected nonzero child exit and actual heap-use-after-free detection; its
[raw diagnostic](asan-detector.log) is retained.

[Full run status](full-asan.status.json), [log](full-asan.log) and
[tooling status](tooling.status.json) pass. The standalone x64 Debug/ASan and
ARM64 ASan build status records retain their exact producer and timestamps.
An ARM64 cross-build does not establish native execution.

The [first full ASan run](first-full-asan.status.json) fails EditorControls at
the unchanged twelve-second dense-source deadline while compilation is active;
[the original log](first-editor-failure.log) retains that failure. The later
[complete EditorControls retry](editor-quiet-retry.json) and final full run pass
after compiler/linker processes exit. No timeout was relaxed and no code change
is claimed to fix or causally explain the first delay.

[Performance smoke data](performance-unpaired.json) and its
[comparison record](performance-comparison.json) are explicitly unpaired.
They cannot close non-regression, quiet-host or original performance findings.
The later packets below complete Debug/Release runtime and remaining build/consumer profiles; physical input, native ARM64 and paired performance remain open.

The subsequent [x64 Debug packet](debug/manifest.json) binds 24 untouched records,
including all nineteen successful suite receipts, their full log/status, the
canonical build attestation and unpaired performance metadata. That metadata
retains source, toolchain, input/artifact identities and the benchmark hash;
individual suite receipts bind their executable hashes. Native producer eb0f10b9
and metadata-only successor a0f67fd3 differ only in the WIP/changelog. An independent
read-only replay verifies every original 23-file packet entry, all nineteen
successful suites and the absence of native changes between those commits.
The subsequently copied build attestation retains its own original timestamp.
The [Release packet](release/manifest.json) binds 24 raw records: nineteen successful zero-skip suite receipts, full log/status, canonical build attestation and unpaired performance metadata. All six standalone profiles now build; native ARM64 execution remains open.
