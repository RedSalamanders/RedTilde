# Recovery envelope bounds

Status: six zero-warning/zero-error builds and three complete x64 native profiles qualified.

Candidate source: `bc024dd88d061a8530ead80a5c6df7505a88637d`.
Dependency: `2a7e7c28f95a70193e4a2f68a89001d54b4d2131`, API 4.

Recovery previously bounded encoded UTF-8 bytes but allowed decoding beyond the editor's 65,536 UTF-16
units to escape its noexcept parser. The parser now reports invalid data without publishing a partial output.
The real startup scanner retains complete originals in quarantine. An existing quarantine name preserves both
the earlier file and the unrenamed input, and valid full-capacity recovery copies remain available.

The independent regression constructs checksum-valid ASCII, BMP and supplementary-character envelopes
beyond the decoded limit, bypassing the serializer's existing limit. It checks every prior output field,
byte-for-byte scanner quarantine, name collisions and a valid 65,536-unit boundary. The unchanged parser at
`0c279c59` terminates the owned test process with its diagnostic exit 77 on the first oversized envelope.
Its executable/PDB, input, partial report and failed canonical receipt remain retained.

The first Release matrix run at `0b4bda83` passed the oversized-envelope case but failed three save/close
fixture writes: ten Recovery cases passed and three failed. Those fixtures relied on asynchronous recovery
initialization creating their parent directories. They now create their own parents explicitly. The original
failure, executable/PDB and receipt remain preserved; its outcome is not replaced by later passes.

The earlier Close-during-Save main qualification also exposed two Open fixture-seeding failures on native
ARM64 at `94b9d431` in [run 42016141956](https://github.com/RedSalamanders/RedTilde/actions/runs/42016141956).
That run passed x64 but withheld production packaging. Its original ARM64 report and logs are retained.
The repaired source subsequently passes its fresh hosted qualification below; the original failed run remains failed.

Current x64 runs execute all 82 cases and 36 standard hidden application captures in each profile:
246 case executions and 108 captures, with process/recovery/crash checks and real ASan detection.
These functional and unpaired performance observations do not accept R10 or physical input.
[The verified projection](qualification.json) binds original source, build, native/SDK archive, canonical
receipt and failure identities. ARM64's Debug, Release and ASan results are cross-builds only.

The original 139 compiled inputs are retained in a member-verified archive, SHA-256
`e572f240d56ae763bd4cf5be20dfd481d5f70abe3ca55a5f0561240ec3a862ac`.
Canonical integration matches those actual working bytes. Compiled identity:
`39b7d538bc367d9dfc56ff1c3d6de93067317862b7d3ac3735dda22659e5a96b`.

Raw compiler contexts, machine identities and physical paths stay private. Remaining work belongs to the
[active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).

## Hosted Release follow-up

[Run 42024995782](https://github.com/RedSalamanders/RedTilde/actions/runs/42024995782) completes all seven
jobs at main `3e1362d004431b721c2d110285b4feff146d7df0`, API 4 / `2a7e7c28`. Native x64 and ARM64 each
execute all 82 cases, 36 captures and the Release assertion probe with zero failures/skips. The combined
test gate completes at 12:41:06 UTC; both production roles start at 12:41:09 UTC. Both test-disabled
production ZIPs pass their native-host smoke checks. Downloaded ZIP sizes/hashes and original canonical
receipt hashes match their manifests. The outer GitHub artifact wrapper was extracted by `gh run download`;
its metadata digest is not presented as independently verified downloaded ZIP bytes.

[The hosted projection](hosted-qualification.json) binds each executable, receipt, manifest, production ZIP
and job ordering. This closes RT-08's repaired Release source only. It does not qualify newer pins, ARM64
Debug/ASan, physical input or R10 performance.
