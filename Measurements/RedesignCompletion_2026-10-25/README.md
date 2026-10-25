# Redesign completion evidence — 2026-10-25

The current checkpoint is source `1d39ee27` at DxUi `2fed8246`:

- [Local matrix](qualification-1d39ee27/README.md): six builds and three x64 Full runs, with 59 ordinary cases
  and 23 hidden captures per run; genuine ASan detection passes. ARM64 is cross-build only.
- [Keyboard/menu input](input-1d39ee27/README.md): nine stages and 168 events pass through the authorized
  automatic-start lease; foreground/focus are unchanged and the pointer is restored.
- [Fresh performance](performance-1d39ee27/README.md): all 576 reports and 72 rows replay independently.
  Four original-UI findings remain; starting-redesign has four memory improvements and no regression findings,
  and the actual PR14-base comparison has twelve no-resolved-change rows. These comparisons remain separate.

Latest SDK main `b6060881` passes five native profiles and fails ARM64 Release in the Menu acrylic-capture
fixture. Its cause and the fixture repair's qualification remain open. RedTilde hosted ARM64 remains blocked
by billing; neither local cross-builds nor earlier green SDK runs replace those observations.

The [separate resource diagnostics](diagnostics-9025be4/README.md) retain the earlier committed-region census
and intrusive-sampler limits. Mapping categories do not identify an allocation owner.

## Earlier checkpoint

These packets retain separate local native qualification and paired-performance evidence for application source
`9025be4b1dcf2753e0cab70cdcd355e219c879fb` / DxUi `2fed82465dcee9f213753754e0e8e54f353cf38f`. They do not close
performance acceptance or the WIP plan.

- [Local native matrix](qualification-9025be4/README.md): all six builds and three noninteractive x64 Full profiles
  pass. ARM64 is cross-build only. The first Release attempt's three hidden-focus failures during an overlapping
  SDK foreground lease and the later serial pass remain distinct; the overlap does not prove root cause.
- [Interactive keyboard input](input-9025be4/README.md): the separate authorized x64 Release run passes all nine
  keyboard stages with 168 injected events and reports foreground, focus and cursor restored. This does not qualify
  physical IME/assistive technology or broader focus behavior, and does not change the noninteractive matrix scope.
- [Paired performance study](performance-9025be4/README.md): 576 reports across separate original-UI,
  starting-redesign and actual PR13-base cohorts. Independent replay verifies 72 rows, 864 block effects and
  1,728 same-side drifts. Five original-UI A/B regressions remain. The actual PR13-base A/B has no resolved change
  and does not close that original-UI gate.

Observer windows do not establish a quiet CPU window. Hosted CI, native ARM64, physical IME/assistive technology
and performance acceptance retain their own gates. The required nine-stage keyboard run is recorded separately above.

- [Documentation-only 49cf continuation](docs-rebind-49cfb9a/README.md): six new receipts with identical qualified native bytes; no repeated runtime or performance execution.
