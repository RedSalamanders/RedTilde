# Design refresh integration — 2026-10-24

The owner requested commit, push and merge after receiving the open performance and qualification status.
This checkpoint records integration, not performance acceptance or a completed product qualification.
The [design-refresh plan](../../Specs/Plans/Done/DesignRefresh_2026-10-22.md) remains in WIP with R10 open.

## Completed implementation

The application includes the matched report layout and fixture, Files navigation, contextual tools, source-preserving
editor presentation, deterministic System-theme capture, dirty/recovery reuse and bounded document-analysis allocation.
Shared control behavior stays in canonical DxUi. The dependency lock names exact SDK revision
`c600993861a337012e2633356c3d35c5e129bbf3`, including retained icon-layout preparation, its lifecycle/reentry guards,
pending-scroll handling, warm-cache factory-path avoidance and visual re-preparation when configured formats are evicted.

The private app history is not imported into public ancestry. Reviewed product/test/specification bytes were ported
onto the publication branch. Existing public evidence exclusions remain in force. The separately reviewed
[visual-fidelity packet](../VisualFidelity_2026-10-23/README.md) retains its original source and dependency identities;
it does not qualify the later dependency pin. Raw diagnostic traces, machine identifiers, foreground snapshots and
private build/test records are not part of this public packet.

## Previous source-bound checks

The corrected final pin `c600993861a337012e2633356c3d35c5e129bbf3` passes the official x64 Release app build,
all 53 behavior cases and 23 deterministic hidden captures, including process-exit, recovery and crash checks.
The [reviewed derived qualification record](qualification.json) binds the native/build records and each published
PNG/sidecar pair. [Current reference capture](captures/reference-dark-dpi-96.png) is an actual harness render.
The packet is a reviewed selection, not a claim that the complete private producer set was published.

At SDK revision `d54522c14bd85578e48019268e08756aaf50a6dc`, original and refreshed app arms pass 27/13 and 53/23
behavior/capture counts respectively. All 23 refreshed PNGs and all 23 sidecars match the preceding analysis-optimized
candidate bytewise. Its capture-seal result SHA-256 is
`CC61664466780CE155EE6892BE90D7D1C48C272880A76DB42CC67AC30ABEB584`.
The SDK itself passes 19 ordinary x64 Release suites with zero skips at that revision. These observations predate
the final warm-cache and cache-eviction changes and do not qualify the final pin.

The first final-pin check at `b7cd1ffe` built cleanly and passed all 53 app cases and 23 captures. All PNGs and
sidecars matched the preceding candidate bytewise. The SDK's 19-suite run then caught a real missing visual
invalidation on configured-format eviction (18 passed, Control failed). The corrected SDK schedules visual-only
preparation in the same frame and trims embedded formats before preparation. Its native and embedded assertions
retain exact layout/factory counts, actual retained-layout drawing and pixel parity. The failed run remains retained.

## Last complete whole-feature comparison

The retained fixed cohort has 192 unique processes: 96 A/A and 96 A/B, 12 blocks per view/study, 64 edit cycles,
two warmup cycles and seed 20261024. Both arms use SDK `cf1393a2c6bc1fa42254e1beb4dc9f9217f43992`.
Baseline app commit is `f99464ed4a5044856433062ccde4eeb3d5d2f9ec`; refreshed app commit is
`587a72c88da2aad2a5205c0a43981b1d700841dd`. Later analysis and SDK optimizations are outside this cohort.
The collection result SHA-256 is `FBE9F596861369B13779002B225AD7DE47BB10639163297D74823925E9A76646`.

| Metric | Compose paired change | Split paired change |
| --- | ---: | ---: |
| Command time | +13.75% | +5.60% |
| Owner CPU | +4.90% | +14.38% |
| Private memory | +10.83% | +11.51% |
| Working set | +4.16% | +4.61% |

The unchanged judge reports seven regressions. Compose CPU stays below its 5% band; completed-edit and clean-frame
timing have no resolved regression. Several A/A control spreads are flagged. Host activity averaged about 12% CPU;
this was not a verified quiet host, and GPU activity was not measured. No silent rebaseline, pooled retry or cost
acceptance is recorded. Diagnostic allocation tracing is unfinished and supplies no proven allocation-owner claim.

## Remaining qualification

Performance/policy acceptance against the actual PR base, a new same-SDK whole-feature comparison, final-pin Debug,
ASan and ARM64 coverage, the authorized foreground suites and physical input/assistive-technology/native ARM64
acceptance remain separate obligations. Cross-builds and earlier revisions do not establish current native runtime
qualification. The experimental capability and release gates remain unchanged after integration.
