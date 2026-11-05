# Close during an admitted save

Qualified source: `15ac77cf68521570c21d1a161445a55305cdeee9`.
Dependency: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.

Close hides the window while the admitted write/replace settles, retaining its notification owner, result and
recovery. Success updates the exact saved baseline and protects later edits. Failure cancels Close, preserves
original bytes/source/history/recovery and restores the previous placement without activation with its error.
Duplicate Close and queued edits are ignored; native hide/focus/capture/size lifecycle still reaches the host.
Open is abandoned before protecting later edits, and its late result cannot replace source after Cancel.

Five hidden real-I/O barrier cases cover durable success, denied replacement, exact source/selection/history
and recovery retention, usable retry, later edits, duplicate Close, logical focus loss, queued input and clean/
edited Open abandonment. They acquire no desktop input or clipboard. The unchanged busy-close path at
`7b2fcfe1` passes eight and fails four recovery cases. The early waiting-close filter at `0fa03f5c` passes
eleven and fails focus loss. Matching programs/PDBs, raw reports and failed canonical receipts remain retained.

Two whole-Editor ASan runs at `56aab58f` hit the original 90-second watchdog. The first wrote its Compose
report; the isolated follow-up also wrote Split and both renderer reports before termination. Both remain
failed. Current execution checks all 53 Editor cases, then runs 47 behavior, three performance and three
rendering cases in separate hidden children with the same 90-second limit and mandatory exact unique union.
Every assertion remains. These passes do not retrospectively qualify the failures or establish their slowdown's cause.

[The qualification projection](qualification.json) binds six zero-warning/zero-error builds, all 81 cases and
36 standard captures in each x64 profile: 243 case executions and 108 captures. Process/recovery/crash checks,
functional performance smoke and real ASan detection pass. ARM64's three profiles are cross-builds only.
Canonical tooling/format validation and all 42 scoped-tooling fixtures pass. Five completed profile archives
were rechecked after storage admission stopped the matrix; only the last ARM64 ASan build resumed.
Application/SDK archive, executable, receipt and SDK-member hashes are verified. Reproducible objects and
redundant SDK stages were removed only after inventory and artifact preservation; original failures remain.

The original 139 compiled inputs are retained in a member-verified source archive, SHA-256
`971be3cf44e7069f1ebc9915b3647d3858b922842bbb10d1b3cfd89313afcecc`.
Two new test files had LF working bytes while governed checkout uses CRLF. Canonical working inputs were
byte-matched from those retained originals before closeout. This is original working data; a Git-reconstructed
checkout has its own identity. Compiled identity:
`6dc6d7c6881bc3dd544d4e1e5ac4cce42fb3e8a50d239b50fbb729b9206b508f`.

Raw compiler contexts and physical paths stay private. Native ARM64 runtime, foreground/physical IME/AT,
release and paired R10 acceptance remain in the [active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
