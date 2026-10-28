# Rejected private Os repair route

The predeclared diagnostic investment rule required both Compose and Split to show at least 65,536 bytes lower median working set, declines in at least three of four independent blocks, and command change no worse than +5%. Compose failed: its median working set increased by 124,928 bytes and only one of four blocks declined. Split met the rule, but both views were required. The repair route is rejected; the retained O2 UI findings remain open.

| View | O2 median WS | Os median WS | Os minus O2 | Declining WS blocks | Command |
|---|---:|---:|---:|---:|---:|
| Compose | 54,933,504B | 55,058,432B | +124,928B | 1/4 | -0.136% |
| Split | 55,652,352B | 55,457,792B | -194,560B | 3/4 | -0.192% |

| Metric | Compose change / signs | Split change / signs |
|---|---:|---:|
| commandUs | -0.136% / +--+ | -0.192% / +--- |
| commandOwnerCpuCycles | +0.425% / +--+ | -0.815% / +--+ |
| ownerCpuCycles | -0.617% / --+- | +2.041% / ++++ |
| completedEditUs | -0.055% / +--+ | -0.349% / -+-- |
| cleanCompletedUs | -0.098% / ++-- | +0.298% / +++- |
| privateBytes | +1.346% / ++++ | -0.523% / -+-+ |
| workingSetBytes | +0.274% / +++- | -0.377% / ---+ |
| preparationSubmissionUs | +0.216% / -++- | +2.105% / -+++ |
| gpuWaitUs | +0.525% / ++-+ | -1.096% / ---- |

Signs show Os minus O2 in chronological independent block order. Four blocks per view are descriptive; the minimum two-sided sign-flip p is .125. No canonical judge or V4 policy was changed.

The size control in the same workspace reduced code by 194 KiB and image size by 229,376 bytes. This establishes a binary-size effect; it does not establish a residency improvement. The frozen executable directory spellings differed. Hashing and observer costs remain explicit; there is no quiet-host claim.

V1 had one successful native four-cycle smoke but failed collector closure and ran zero study slots; its failed status is retained. V2 repaired only collector output/status hashing, then completed one separate smoke and all 32 fixed study processes. No study replacements or exclusions occurred.

All six candidate builds passed. The three x64 Full suites passed 201 cases and 96 capture/provenance pairs total; actual ASan detection and all nine interactive stages passed with verified restoration. Native ARM64, physical IME and assistive technology remain outside this packet's qualification.

The [machine-readable projection](projection.json) binds source, binary, collector and evidence identities. Full raw data and source/recovery bundles stay in the controlled archive. No production Os adoption or further compiler-policy qualification is proposed.
