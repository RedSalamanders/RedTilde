# Final production-code app comparison — 25 October

The [reviewed aggregate](summary.json) retains all 48 metric rows from 384 native process runs. It measures
RedTilde `138a648b292af0f810286ded05f59f37ee737f8f` at exact DxUi pin
`baff43d8c8402258f17aa77f7b790166d57eabc9`. The documentation-only app refresh produces the same six-profile
code artifacts as the [fully tested fa492ab revision](../qualification-fa492ab/README.md). Performance
qualification remains open.

## Baselines and protocol

- **Original UI:** PR #10 base `f819395d36c55b93e3bf9559d12d0ec401e42b3d`, followed by the disclosed common
  SDK/harness overlays to measured freeze `c04ea1e2f57fd4f323ddfc04c57b09611f17f147`.
- **Starting redesign:** PR #11 base `c7456854c8f964bb9ea067295e6435f99d426515`, followed by docs-only
  continuation `d53b4c6dc382ca87d3229c2c970eaaff0d07dbf5` and common SDK/harness overlays to measured freeze
  `0acee29a0e597bfd5367265ff4c7d44b91f4a649`.

These source-derived freezes preserve distinct lineages; neither is an untouched PR-base checkout. Both use
the same new exact SDK pin, as required by the dependency-adoption comparison contract. The
[upstream SDK actual-base comparison](../sdk-baff43d8/README.md) remains a separate study.

Each cohort has a separate 96-process baseline A/A control followed by 96 A/B processes: 12 independent
ABBA/BAAB blocks per Compose/Split view, 64 cycles, two warmups and seed 20261024. A/A uses the baseline
executable in both roles. All seven harness/fixture inputs, compiler profiles and dependency identities match.
The summary retains source/build/executable/toolchain hashes, all 12 log effects per row, exact sign-flip
probabilities, Holm adjustments, investigation bands and individual control flags. Bands are 5% for timing/CPU
and 2% for memory; they do not authorize degradation.

## A/B observations

Percentages are paired estimates. **Bold values are the official regression findings.** All four private-memory
and working-set decreases in the starting-redesign columns are classified improved. The other unbolded values
are classified no resolved change. The complete A/A and A/B families remain in the JSON.

| Metric | Original UI, Compose | Original UI, Split | Starting redesign, Compose | Starting redesign, Split |
| --- | ---: | ---: | ---: | ---: |
| Command wall time | **+12.364%** | **+18.747%** | +1.491% | +0.774% |
| Owner CPU cycles | +2.570% | **+13.756%** | -0.657% | -0.332% |
| Completed edit/frame | -0.152% | +0.465% | +0.048% | -0.199% |
| Clean completed frame | -0.355% | +1.004% | -0.209% | +0.294% |
| Private bytes | +1.966% | **+2.143%** | -8.729% | -9.194% |
| Working set | +1.895% | **+2.145%** | -2.460% | -2.473% |

The original-UI comparison retains five flags; only its flagged Split working-set control is stable. The
starting-redesign comparison has zero regression findings and four improvements. Completed-frame metrics have
no resolved change in either cohort. Command medians against the original UI are 254.475 → 283.550 microseconds
in Compose and 271.650 → 321.950 microseconds in Split; these descriptive medians are separate from the paired
estimates above. The current memory observations do not attribute the historical retained 4 MiB allocation.

## Performance advice still requiring disposition

The starting-redesign comparison supports the measured memory reduction from the correction work. It does not
accept the redesign's five flags against the original UI. The following options preserve that distinction;
none has an approved cost/scope disposition.

| Option | Measured basis and required next evidence |
| --- | --- |
| Further selection/status optimization | A separate two-block diagnostic places 18.400 microseconds of additional Compose command work and 24.075 microseconds in Split in the Selection bucket. Split that bucket before changing it; those values are not an achievable saving or an attribution. Preserve caret/EOL, block context, Find, math, selection and accessible status freshness. |
| Memory ownership investigation | Original-UI private-byte medians are 44,916,736 to 45,903,872 bytes in Compose and 45,621,248 to 46,661,632 bytes in Split. Obtain allocation/lifetime ownership before removing caches or document facts. Only the flagged Split working-set control is stable; retain the other flags with their uncertainty. |
| Scope reduction or explicit deferral | Reducing live position/block/status features changes product behavior and needs an explicit owner scope decision plus focused regression coverage. Deferral leaves R10/D4c and release acceptance open; it is not a non-regression verdict. |

The current source adds position, newline, block and search-status presentation to the original Selection path.
Changed labels request measure invalidation and accessibility publication; ordinary stable-focus publication
is deferred and coalesced. This is a source-backed candidate for a narrower diagnostic, not proof of the
command-time cause. Repeating the unchanged primary study on the same observed host is not an optimization.

## Audit and qualification limits

The independent typed reference passes 7,988 original-UI and 8,015 starting-redesign checks, 16,003 total.
A separate projection review matches all 48 rows and 576 block effects, four comparison digests, three role
bindings and seven harness hashes. The lead verifies the four comparison digests, exact projection copy and
privacy scan. Summary SHA-256: `0DFE0030C919CE99E9B840E81E232ACA8B3FF08D421D4981884F880C9E1E9CF2`.
Original native receipts, executables, compiler traces, observer records, failed authoring attempts and
source-bound audits remain private under the owning worktree's `.build`.

All four host windows have complete observer coverage. Mean system CPU ranges from 17.2% to 18.4%; none is
verified quiet. Mixed controls and observed background load remain explicit. A completed numerical audit does
not qualify upstream policy or accept the five flags. R10/D4c optimization/advice and performance qualification
remain open. The separate 16-process command-phase diagnostic is descriptive and is not pooled into this family.

The original-UI Core receipt predates its build-receipt hash field; the retained baseline audit separately binds
the official standalone build receipt, source, exact SDK and executable digests. The baseline catalogues are
revision-specific. No new runtime execution is inferred from a documentation-only artifact-equivalence check.
Current foreground-input and hosted RedTilde ARM64 obligations remain separate; the latter is still unstarted
because of the account payment/spending restriction and remains open at the owner's request.
