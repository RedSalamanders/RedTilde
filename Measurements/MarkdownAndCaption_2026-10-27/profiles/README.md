# Additional build profiles

[The raw manifest](manifest.json) binds the additional SDK and consumer build
statuses, logs and canonical build receipts. Earlier app Debug/Release and SDK
Debug/ASan/ARM64 ASan evidence retain their own producer identities in the parent
packets. All six consumer profiles use native source identity
`2166a79bc250d6042ad5698927eb0d3ac3e95a22e1525ff4d7934808590e0e87`
and exact SDK `eb0f10b91bb8de19b0d7671cc7078fd0c1f5e767`.

The four new consumer build receipts bind their compiled inputs, dependency,
public headers, compiler context, output closure and executable SHA-256. Their
producer is metadata checkpoint `c4ee0459`; the native caption/capture correction
is `a0dfad8f`. The SDK native source is eb0f10b9, with metadata successor a0f67fd3.
These identities are preserved rather than relabelled after documentation updates.

Independent read-only replay verified the first fourteen raw entries, all four
successful consumer receipts and exact compiler/pin identities. Actual ARM64
compile/link/archive commands use MSVC 14.52.37025. The restore log's separate
14.53 detection concerns vcpkg's x64 CMake helper triplet; the native ARM64 target
and its metadata identify 14.52.37025.

The ARM64 receipts explicitly record `runtimeExecuted: false`. Cross-builds do
not establish native execution, hosted CI, physical input, release readiness or
paired performance. The original performance/CI queue remains paused.
