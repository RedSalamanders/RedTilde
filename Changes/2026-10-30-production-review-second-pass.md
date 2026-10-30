- Extend the production-readiness review with a second pass over every merge through `468cd94` (DxUi
  `6ed0512c`): PR-01–PR-33 revalidated (31 unchanged; PR-24's orphan line and PR-26's fixture example resolved),
  PR-02 corrected (Enter in Split and an untouched draft preserve line endings), and new findings PR-34–PR-46 added:
  a re-entrant close prompt that can use freed controls, menu commands that terminate at the source limit, menu
  focus leaving the draft, untruthful EOL capability status, the paired adoption comparison still expecting the old
  DxUi folders, and tests that pass vacuously or assert known defects. Simplifications S12–S16, open questions
  Q18–Q22 and the remediation order are updated. Documentation only; nothing was built or run, and remediation has
  not started.
