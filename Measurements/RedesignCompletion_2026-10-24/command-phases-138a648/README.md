# Separate command-phase diagnostic — 25 October

The [complete phase records](summary.json) retain all nine phases from 16 native processes at app `138a648`
and SDK `baff43d8`, compared with the same-SDK original-UI freeze `c04ea1e`. The disclosed baseline overlays
and distinct primary comparisons are in the [primary packet](../performance-138a648/README.md).
This diagnostic is descriptive only and is not pooled with the primary performance family.

The `app-edit-phases-v2` protocol uses two independent ABBA/BAAB blocks per view, 64 cycles, seed 20261024,
and 128 timed callbacks per process. Each value below is the median of four process medians for that view
and arm; the JSON also retains their ranges. The callbacks within a process are not independent experiments.

| Phase | Compose wall, microseconds | Split wall, microseconds | Compose owner CPU cycles | Split owner CPU cycles |
| --- | ---: | ---: | ---: | ---: |
| Enter | 11.500 → 11.025 | 10.450 → 10.500 | 22,854.25 → 22,055.75 | 20,808.25 → 20,908.25 |
| Analysis | 14.350 → 17.775 | 12.475 → 14.300 | 28,643 → 35,329 | 24,600.75 → 28,393 |
| Mirror | 0.650 → 0.725 | 45.475 → 43.950 | 1,198 → 1,297.5 | 90,718.5 → 87,524.75 |
| Identity | 41.075 → 43.550 | 35.000 → 35.900 | 80,588.25 → 85,628.5 | 69,560.5 → 70,958.25 |
| Caption | 88.625 → 86.075 | 83.725 → 84.175 | 176,745.75 → 171,207 | 166,466 → 165,518 |
| Chrome | 79.275 → 78.350 | 68.500 → 71.550 | 74,201 → 71,906 | 59,930.5 → 60,378.75 |
| Selection | 5.325 → 23.725 | 6.475 → 30.550 | 8,633 → 40,169.5 | 11,327.5 → 45,309.25 |
| Recovery | 9.550 → 9.050 | 8.075 → 8.700 | 19,062 → 17,764.5 | 15,967.75 → 16,916 |
| Exit | 0.850 → 1.475 | 0.700 → 1.250 | 1,596.75 → 2,794.5 | 1,397 → 2,295.75 |

Selection adds 18.400 microseconds in Compose and 24.075 microseconds in Split in this small diagnostic.
Current code additionally derives and publishes caret/EOL, heading, block/list and search presentation facts.
Changed labels invalidate measure and request accessibility publication; ordinary stable-focus layout and
accessibility flushes are deferred/coalesced. These are source-backed candidates for narrower investigation.
The whole-method timing does not identify which operation causes the increase or establish an achievable saving.

The independent native audit verifies report/source/receipt bindings, phase order and bounds, render coverage
and 2,048 CPU reconstructions. The lead additionally compares every projected sample to its digest-bound
native report: 288 arrays, 18,432 wall and 18,432 CPU values, all 36 summary rows and eight collection digests
match. The JSON retains the 16 report digests, compiler profiles, common harness hashes, source/executable
identities and independent-audit digest. Original reports remain private beneath the owning `.build`.
Summary SHA-256: `56D79DDEB020A11394169E29B090A9BB2AD15B8F4A155F14715B1184D590B2D9`.

There is no independent host observer or verified quiet window for this diagnostic. Two blocks per view
support descriptive investigation only. Hidden completed frames do not establish physical input/presentation,
allocation ownership or causal attribution. Performance acceptance, the original-UI flags, current input
qualification and hosted RedTilde ARM64 remain separate obligations.
