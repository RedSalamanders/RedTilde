# Redesign completion evidence — 2026-10-25

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

Observer windows have complete sample coverage but no verified quiet CPU window. Hosted CI, focus, native ARM64
and performance acceptance remain separate gates.
