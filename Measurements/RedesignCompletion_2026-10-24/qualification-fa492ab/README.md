# Final local qualification snapshot: fa492ab / baff43d8

**Status: local x64 matrix passed; ARM64 cross-builds passed. This is not final release acceptance.** The snapshot binds RedTilde `fa492ab3947ec309cd3bc52d782e58063aebf962` (source identity `a9443c02f50d522a48c2128e1b234ef6d72a1906d1c46328997c28be098febdd`) to DxUi `baff43d8c8402258f17aa77f7b790166d57eabc9`.

All three x64 profiles built and passed the official `All` suite excluding tooling: Debug, Release, and ASan Debug. Each profile passed 55 ordinary cases: Storage 10, Settings 4, Recovery 5, and Editor 36. The suite included recovery restart/crash coverage and the native-object resource lifecycle probe on default and WARP rendering. ASan Debug also ran the sanitizer detection suite; the positive probe observed the expected heap-use-after-free detection. Each profile retained 23 harness captures and their scenario sidecars. Payload integrity checks verified 619 file streams in Debug, 619 in Release, and 621 in ASan Debug, with no hash mismatches.

ARM64 Debug, Release, and ASan Debug all built successfully and their archived build receipts and artifacts were verified. They were cross-builds only: no ARM64 tests or runtime execution occurred.

A retained-capture comparison against the earlier `fd24623` snapshot found all 23 PNGs byte-identical in each x64 profile, with all image and sidecar hashes matching their respective receipts. The 22 non-settings sidecars differ only in the recorded effective system preference (dark in the earlier run, light in this run); the settings sidecar is identical. This is capture byte continuity, not full image-2 parity.

The hosted workflow for `fa492ab` (`40171816856`) executed zero steps because hosted billing prevented execution. Hosted ARM64 remains open. The latest input/performance study has not been qualified. DxUi references below are independent local matrix audits for `baff43d8`, not hosted CI results. No exact fresh documentation-validation run is claimed; documentation receipts are being refreshed separately.

See `summary.json` for run IDs and artifact digests. Local evidence is retained in the repository's qualification records; this projection omits machine, user, process, and private path details.
