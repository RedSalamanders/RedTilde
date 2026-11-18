- `restore-dxui.ps1` prepares a complete vcpkg checkout in the DxUi output root before it runs the pinned DxUi
  installer, which reuses that checkout instead of making its `--filter=blob:none` partial clone. vcpkg 2026-07-27
  extracts each versioned port with `git read-tree` and `git checkout-index` in an environment that holds only
  `GIT_INDEX_FILE`; without `SystemRoot` that Git resolves no host, so a blob the partial clone left on the server
  failed the restore with "Could not resolve host: github.com". It started on 2026-11-18, when upstream vcpkg
  de36db0455 moved `vcpkg-cmake` on `master` away from the tree the baseline selects, and failed `main` runs
  44312518479 and 44328718604 on x64 and ARM64. The new `Build/VcpkgToolCheckout.psm1` (the same text in every
  consumer) clones completely, completes an earlier partial clone in place and never deletes anything. The DxUi
  pin is unchanged; DxUi #267 fixes its own installer. `Tests/Tooling.Tests.VcpkgToolCheckout.ps1`, run by
  `validate.ps1`, covers a fresh, repeated and completed checkout, refusals, and an offline extraction in
  vcpkg's stripped environment. The native compile is proved by the post-merge qualification.
