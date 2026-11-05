# Hosted build-admission fixture version

The x64 regression role in [main qualification 41814935803](https://github.com/RedSalamanders/RedTilde/actions/runs/41814935803/job/126523416445)
failed in `Scope contracts and public runner` at `ca16d32ecb16ef2cdd386a7393b68c74d1957736`.
The mock build explicitly requested version zero, matching its text executable without version resources.
The subsequent positive artifact-admission call ran in the parent process and inherited the hosted product
version. Its combined input identity therefore differed, producing the correct fail-closed refusal.

The unchanged test reproduces that refusal with inherited `RedTildeBuildNumber=137`. Its retained raw log
SHA-256 is `8F3E49BD514410D2FB5EF7107ECBDFD5EE2A59C2ED86D44230575D63F5059512`.
After the fixture correction, all 41 scoped-tooling cases pass under that same inherited value. The passing
raw log has SHA-256 `4FA5E3F8270F9F67B6BD11F02C4430537A1AF3893F961DEB40BFD4F951ED28C3`.
Original logs and the failing source checkpoint remain in the approved private workspace.
[Verification hashes](verification.json) bind the corrected test script, both runs, complete tooling validation
under inherited version 137 and formatting. These are projections of retained observations, not a native receipt.

Every positive and negative admission probe now runs in an isolated child with explicit fixture version zero.
The build child still inherits hostile product version 137 and must honor its explicit `-BuildNumber 0`.
A separate admission probe explicitly requests version 137 against the valid version-zero receipt and must
be refused. Source-byte, installed-header, compiler-context and resolved-dependency negative controls keep
their matching version, so unrelated version drift cannot make them pass vacuously. Real build admission,
source identity, compiler/dependency checks and product version policy are unchanged.

The preceding observations are tooling-fixture verification. They do not establish native runtime or production qualification.
The original main run passed its independent native ARM64 role: 74 cases, 34 captures and the Release
assertion probe at SDK `8c6c2818d084e6124fb2c08eddc00bdba7a9d2aa`; the original native receipt has SHA-256
`CB216DF3DA5FD69E1225130FEDD41DAEAE83EB8257D1E0EF2348A87A86203285`. Its failed x64 role still prevented
the combined test gate and production. That historical failed run remains failed.

## Repaired main test gate

[Main run 41853861400](https://github.com/RedSalamanders/RedTilde/actions/runs/41853861400) at
`1c600d6018686c8ae486ee13fce56c6ca2f13ba8`, SDK API 4 / `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`,
passes both native Release regression roles. Each discovers and executes all 75 cases with zero failures or
skips, records all 36 capture pairs and passes its Release assertion probe. The ARM64 receipt identifies an
Arm64 host; this is native execution, separate from the local cross-builds.

[The test-gate projection](main-test-gate.json) retains the regression metadata and original canonical receipt
SHA-256 values, executable identities and exact source/native digest. Original downloaded receipts remain
private and their hashes match the hosted regression metadata. The combined test gate completes at
`2026-11-04T15:33:24Z`; both production roles start afterward. This closes the repaired test-before-production
gate without claiming production completion, physical input or paired performance acceptance.
