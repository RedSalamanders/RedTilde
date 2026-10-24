# Dependencies

DxUi is pinned by [DxUi.lock.json](DxUi.lock.json), initially to the experimental rich-text PR #89, now merged.
The design refresh adopts reviewed shared control/input changes by exact commit; it does not follow a branch.
Restore with `restore-dxui.ps1`. Its WIL and native Markdown dependencies are restored through its own manifest.
No Red-family checkout is needed to build RedTilde. Exact third-party licenses remain in the pinned source;
portable packaging must include them. This development pin does not authorize a production release.
