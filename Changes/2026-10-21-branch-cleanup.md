- Retire three merged RedTilde working branches and two frozen-baseline branch names after verifying recovery
  bundles and exact archive tags in both the application and continuation repositories. Preserve the intentional
  unmerged v2/v3 benchmark histories, all native/evidence files and the useful unmerged DxUi worktree.
- Record the live DxUi #89–#91 draft stack, ARM64 Debug failure, conflicting integration and failed/incomplete
  paired gates in the indexed WIP continuation. The refreshed #89 paired/CI gate also fails. No PR merge,
  ready promotion, pin update or gate waiver is claimed.
- Keep only main as an application local branch and remove the continuation's obsolete archival-bundle remote.
  This changes documentation and Git refs only; native functionality and experimental capability status are unchanged.
