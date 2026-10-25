# RedTilde bae1572 local qualification

This packet records fresh local qualification of source `bae15722fa1ad47b97239ee1b32e53cdfd6f1fab`, source identity `f2d0cd8f138465ac2e53f52210d49868c163a324309663bb95306623672a3169`, and DxUi `1714f8b0c0fa628bcef56de76078f85c869fc50c` (API revision 4). It is a new execution on bae1572; it does not reuse or retag the earlier c60 evidence or the SDK 4904/cd35 experiment.

All six local build profiles passed. The three x64 Full runs passed noninteractively: 58 behavioral cases each, including 39 Editor cases, and 23 hidden captures each. The frame diagnostic regression and default/WARP resource lifecycle cases passed in all three runs. Crash/recovery checks passed, and the x64 ASan profile detected the intentional heap-use-after-free probe. Independent verification matched the frozen profile artifact hashes and PE architectures, source identity, SDK dependency and build receipts; it rehashed 1,862 payload files and bound all 69 capture images to their scenario sidecars.

Public-safe artifact records use ordinal, role, size and digest. Payload records use ordinal, size and digest without source filenames or local paths. The summary omits run identifiers, process identifiers, environment-derived host names and raw payload paths.

ARM64 is cross-build only. No focus or foreground qualification is claimed. The paired performance gate remains open, including the five original-UI regressions and unstable controls. RedTilde hosted workflow [40213720426](https://github.com/RedSalamanders/RedTilde/actions/runs/40213720426) was blocked by billing before native jobs (two billing annotations); this is not a CI pass. The DxUi ARM64 Debug menu investigation remains separate.

The source frame-metric collector had two preserved 38/39 attempts before the corrected 39/39 Release Editor run. This packet contains the corrected bae1572 matrix only and does not erase or relabel those earlier attempts.

