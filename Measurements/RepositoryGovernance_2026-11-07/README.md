# Repository governance verification

Read-only verification on 7 November 2026 used application main
`fe3363cdb1871aeefea69c08edb5d91474041aa9`, API-4 DxUi
`a1915ffdd94b18fd1817371f1dc074f6a0b547f1`, and CI engine
`19710a8aeac979a82e83bec82e8b866cee3d3deb`.

The repository is public, active and defaults to `main`. Its effective branch
rules require pull requests and the three aggregate/security contexts
`checks / ci-gate`, `security / codeql` and `security / dependency-review`;
deletion and non-fast-forward updates are blocked. The classic branch-protection
endpoint reports no classic policy; that does not negate the active repository
ruleset. No repository setting, visibility or rule was changed.

Current workflow callers agree with the manifest's engine pin. PR checks remain
compiler-free, and the main coordinator admits native work after five quiet
minutes. Native x64 and ARM64 Release tests precede the global production gate.
Manual Debug/ASan qualification and explicit publication remain separate roles.
The authoritative [CI contract](../../Specs/Build/Build_UnifiedCI.md) owns these
requirements. Future verification must re-read effective rules and actual source.

Current README, usage documentation and capabilities retain the 65,536 UTF-16
limit, explicit remote-image consent, API-4 dependency and separate physical,
performance, release and full-language gates. The six-profile
[native editing qualification](../NativeEditing_2026-11-07/README.md) remains
bound to its own original source. This settings/routing audit introduces no
native, performance, API-5 adoption or release qualification claim.

The private original API responses and seven input hashes are retained with the
repository review evidence. They contain no credentials and were collected
without changing application state or taking desktop focus.
