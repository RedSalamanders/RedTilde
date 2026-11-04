# Native build policy, terminal results and short DxUi paths

Status: DONE — scoped implementation/review record; outstanding acceptance is transferred.
Archived: 4 November 2026
Current work: [the single remaining-work plan](../WIP/RemainingWork_2026-11-04.md).
Original: `Specs/Plans/WIP/NativeBuildPolicyCoherence_2026-10-28.md` at `1cd59c08528cc1e20720a45ef833bb9e69c315e4`; SHA-256 `97B3FF2F68DB417D236AD440CF5D9AC3ABA50AA4F566455E8E090DD3F8965DAB`.
[Original committed record](https://github.com/RedSalamanders/RedTilde/blob/1cd59c08528cc1e20720a45ef833bb9e69c315e4/Specs/Plans/WIP/NativeBuildPolicyCoherence_2026-10-28.md).

The dated record below preserves decisions, source-specific evidence, failures and rejected hypotheses.
Transferred items are not completed features. Old pins, billing blocks, draft/merge claims and historical
queues describe their recorded checkpoints; the new plan owns current status. This archive does not
waive performance, consumer, native-platform, physical-input or release gates.

## Historical record

Historical status: ACTIVE
Date: 2026-10-28
Owner: build maintainers

The user's agreed policy is /Wall with /WX disabled and zero warnings accepted;
Debug uses EditAndContinue, ASan Debug uses ProgramDatabase. Release enables
explicit COMDAT folding (/OPT:ICF); Debug and ASan Debug disable it.
The owning build contract is [the toolchain contract](../../Build/Build_Toolchain.md).
Five repositories implement the same policy without implicitly advancing DxUi pins.

## Checklist

- [x] Preserve the pre-change effective-profile audit and isolate changes from concurrent tooling work.
- [x] Restore RedTilde ARM64 through its normal entrypoint and pass the required policy/lease checks.
- [x] Enforce and negatively test the common compiler/debug/linker policy in every supported native profile and pinned archive.
- [x] Unify green zero-warning build results, diagnostic/log links and automatic terminal executable hyperlinks where supported, preserving plain text and existing build/run semantics.
- [x] Shorten source-commit and output-fingerprint folders to 16 hex characters with full identity validation and collision refusal; retain existing data without cleanup.
- [x] Update source discovery, direct helper builds, provenance, test fixtures and dependency identities together.
- [x] Publish zero-warning guidance and normative build/path/terminal contracts, with meaningful tooling tests.
- [x] Execute affected tests, evaluated profiles, required native builds and applicable local x64 runtime qualification; retain every failed result and distinguish ARM64 cross-builds from runtime tests.
- [x] Freeze matching Release artifacts, report before/after image sizes and collect the prespecified quiet paired performance measurements; retain inconclusive outcomes.
- **Transferred to [T-G1](../WIP/RemainingWork_2026-11-04.md#t-g1).** Fold/check the changelog; validate formatting/specs/tooling and close this plan only after all required gates pass.

## Decisions and limits

Source pins and full SHA256 identities remain authoritative; folder prefixes are
locators. No source clone is reset for a prefix collision. Existing historical
output is retained in place. Build success is green only after successful
native exit, zero diagnostics and required attestation. Hyperlinks never launch
an application automatically and fall back to quoted launch commands.
The existing explicit RedSalamander Embedded compiler-debug recovery mode remains
an attested exception; ordinary Debug/ASan profiles use the shared defaults.

## Evidence

Baseline audit: E:/DxUi.Perf/flag-coherence-20261028-aea0f188.
Current task evidence: E:/DxUi.Perf/build-policy-evidence-20261028.
RedTilde ARM64 restore and required policy/lease logs are under that task's
RedTilde-ARM64-restore folder; both exited zero before implementation.
Candidate dependency: `6ed0512c68743c57ac209e22010679d211c4a42b` (API4), superseded on 2 November 2026 by the
[DxUi delivery adoption](DxUiDeliveryAdoption_2026-11-02.md) lock; results below remain evidence for the old pin only. This selects the reviewed warning/ICF repair for product qualification; it is not a final green receipt. The library's x64 ordinary suites passed; final native refresh and desktop focus qualification remain explicit gates. Product validation must use the normal restore entrypoint and matching headers/archive.


The first required post-pin policy check found a missing Platform property in the observer. It now requests the property and has independent x64/ARM64 exception controls. Native build attempt 1 began before this test-only repair and cannot be reused if its source guard rejects the changed input; refresh the matrix after tooling passes.


## Local qualification checkpoint, 2026-10-29

The current five-repository audit passes 504 native profiles and 73,502 evaluated setting groups. All four consumers
pass their 24 required pinned-archive profiles; the additional Tilde lease profiles and negative controls pass.
Exact source identities, raw metadata and field differences are retained under the task's final-audit-20261029
evidence. Ordinary Debug and ASan formats remain separate from the recorded RedSalamander Full Embedded recovery mode.

Source 183f passes six native builds, all three Full and all three interactive profiles with restoration, complete validation, normal ARM64 restore, required archive and interactive-lease policy controls. The task branch is pushed. Quiet measurements, final documentation and integration remain pending.


## Merge handoff, 2026-10-29

The developer explicitly requested merging all five repositories after the pending gates were identified.
This is an integration decision with the following recorded qualification limits; it does not change any test
verdict or establish complete production qualification. Focus-taking tests remain paused by the developer.

The prespecified quiet study completed once, without a selected repeat. Its native images and receipts remain
bound to the earlier source recorded in the evidence; these final handoff edits affect documentation only.
Baseline and candidate use the same discovered MSVC 14.52 toolchain; the comparison covers the complete change,
rather than isolating a compiler-version or individual-flag effect. Nonsignificance is not proof of equivalence.

| Scenario / metric | Baseline median | Candidate median | Paired change | Finding |
| --- | ---: | ---: | ---: | --- |
| Compose / cleanCompletedUs | 8302.55 | 8296.975 | -0.1868% | no significant fixture change detected; not non-regression proof |
| Compose / commandUs | 123.525 | 125.35 | 2.6404% | inconclusive - unstable controls |
| Compose / completedEditUs | 8318.7 | 8338.125 | 0.197% | no significant fixture change detected; not non-regression proof |
| Compose / ownerCpuCycles | 12155479.5 | 12316215 | 1.3871% | inconclusive - unstable controls |
| Compose / privateBytes | 32620544 | 31969280 | -1.1083% | inconclusive - unstable controls |
| Compose / workingSetBytes | 64497664 | 64473088 | 0.046% | inconclusive - unstable controls |
| Split / cleanCompletedUs | 8341.375 | 8305.2 | -0.3133% | no significant fixture change detected; not non-regression proof |
| Split / commandUs | 144.275 | 145.5 | 0.9815% | inconclusive - unstable controls |
| Split / completedEditUs | 8326.975 | 8327.2 | -0.089% | no significant fixture change detected; not non-regression proof |
| Split / ownerCpuCycles | 15612794 | 15822136.75 | 1.2888% | inconclusive - unstable controls |
| Split / privateBytes | 33806336 | 34191360 | 0.7386% | inconclusive - unstable controls |
| Split / workingSetBytes | 66025472 | 66054144 | -0.013% | inconclusive - unstable controls |

Native samples, collection plan, machine-activity observations, preserved receipts and statistical analysis:
`
E:\RedTilde.Perf\build-policy-evidence-20261028\studies\RedTilde\20261029T193058-5f3e386d088a4e82a904bfcfc9af40fd
`.

Local six-profile builds and all three Full runtime profiles passed. GitHub CI was blocked before runner allocation by account billing; those failed checks remain failed and cannot be represented as successful local equivalents. The developer requested local validation/publication and has now explicitly requested the merge.
