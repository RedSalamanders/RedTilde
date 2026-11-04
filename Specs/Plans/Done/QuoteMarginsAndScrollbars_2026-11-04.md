# Quote inner margins and themed document scrollbars

Status: DONE — scoped implementation, x64 native regressions, hidden captures and domain documentation

## Delivered behavior

- [x] Adopt DxUi API 4 at `8c6c2818d084e6124fb2c08eddc00bdba7a9d2aa`, including six-DIP inner quote/callout margins and recoverable Debug sequence-label allocation.
- [x] Enable the existing native themed scrollbar in Compose, Source, the Split mirror and the equation inspector.
- [x] Exercise actual view commands, theme-responsive scrollbar pixels, paging, thumb capture/drag/release, overflow and fitting content at 96/144/192 DPI with Light/Dark/Paper themes.
- [x] Preserve exact source, ACP selection, source revision and retained redo history across those operations.
- [x] Retain failing baselines, actual hidden application captures, scenario/build provenance and original artifact identities.
- [x] Update the design system, usage guide, validation contract, case catalog and Changes fragment.

The durable UI contract is in [UI_DesignSystem](../../UI/UI_DesignSystem.md). Shared geometry and allocation
requirements belong to DxUi; the application only adopts its public archive and appearance options.
The [measurement packet](../../../Measurements/QuoteMarginsAndScrollbars_2026-11-04/README.md) preserves matched
comparisons and the final-pin capture identities. No desktop screenshot or image editing was used.

## Native evidence

The exact pin passes all 74 native cases in x64 Release, Debug and ASan Debug: Storage 12, Settings 4,
Recovery 5 and Editor 53. Preferences exit, recovery crash/restart, isolated crash reporting and performance
smoke also pass in those profiles. Release generates all 34 standard hidden captures. All three builds
report zero warnings and zero errors. The actual ASan heap-use-after-free detection probe passes separately.

| Profile / check | Retained run |
| --- | --- |
| x64 Release | `20261104T090859180Z-45360-b0f3f358029c4666be37e662e6e241e9` |
| x64 Debug | `20261104T091704105Z-49000-af49a3b851d6452bb25597dc05e24a0a` |
| x64 ASan Debug | `20261104T092139712Z-836-dd306e9800a946d2ae241fbe94ad0bb4` |
| ASan detection | `20261104T092400410Z-46972-76e1f758d70a432db4416347de94c415` |

The original f498 capture/matrix checkpoint and both historical hosted Editor failures remain separately
identified. Declared hidden viewports correct those two fixtures without changing production responsive
behavior. The mocked executable fixture explicitly selects version zero under a hostile inherited CI build
number; the production frozen-version guard is unchanged.

## Remaining acceptance

The [single active plan](../WIP/RemainingWork_2026-11-04.md) retains native ARM64 verification of the two
corrected fixtures (T-Q1), current platform/physical-input/release qualification (T-Q2) and paired performance
acceptance (T-P1). Cross-compilation, unpaired diagnostic timings and source publication do not close those gates.
Recovery, file safety and the other product work remain active under their existing owners.
