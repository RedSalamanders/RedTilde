# Reduced historical performance retention

The owner requested removal of redundant `.Perf` measurements and build payloads on 28 October 2026. Cleanup inventoried the four existing approved Red-project roots, checked live dependency targets, and did not traverse reparse points. No new drive-root output directory was created.

The reviewed batches removed 190 obsolete SDK cache directories, two archived file-manager dependency caches, and 2,187 old executable/library/debug/intermediate files from retired worktree and historical archive payloads. The total is **53,082,468,395 logical bytes (49.437 GiB), 981,708 files**, with zero deletion failures or target remnants. Filesystem free-space gains can differ from logical byte counts.

All four ownership markers stayed unchanged. Six live consumer dependency target roots remained intact. Current source-bound comparisons and required frozen binaries, source ZIPs, Git bundles, dirty-source copies and recovery records were retained. No replacement copy of disposable caches was made. Historical full-retention promises are superseded for the explicitly retired payloads; their old manifests remain historical records.

Root review checked 9,135 target-closure, marker and protected-exclusion conditions and rehashed the three frozen app reference executables. Protected subtree file counts and byte totals stayed unchanged. This is not a full rehash of every retained archive and does not establish a build, test or performance pass.

Post-cleanup logical inventory: SDK root 37,930,167,841 bytes / 407,590 files; file-manager roots 3,145,962,904 bytes / 31,089 files and 100,714 bytes / 1,016 files; Prism root 35 bytes / one marker. These are the cleanup checkpoint values, before subsequent builds. Detailed target plans, per-target results and retention notices remain in the private cleanup packet; private host paths are omitted here.
