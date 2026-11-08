# Private history snapshot audit

The [ledger](qualification.json) binds 255 private snapshot refs and 524 reachable
commits, including remote/local refs and detached worktree heads. Git object closure
verifies. The official Gitleaks v8.30.1 archive checksum and executable are recorded.

The scanner returned exit 1 and two generic-key candidates. Both exact values are
SDK Git commit references in qualification prose, resolving to actual commit
objects in the retained SDK snapshot. The original redacted report, candidate
context and scanner result remain retained. No scanner rule/allowlist was changed;
no unresolved candidate remains. This does not certify the absence of every secret
or complete the identity/workstation privacy review.

This is a snapshot audit, not the coordinated Q17 rewrite. Active source is not
frozen and the mirrors are not complete dirty-worktree backups. Public refs,
history and visibility were untouched. Private originals, a reviewed rewrite and
commit map, consumer coordination and worktree repair remain with the
[single parent](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
