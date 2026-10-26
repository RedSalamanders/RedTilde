# RedTilde final 09f performance study

This packet projects the completed six-study performance collection for application candidate `09f22877215a0084aae42f56c6e1ec1907079632`, with DxUi SDK candidate `4077f57df8207eac6c39c43437feb45b19d23cf8`. It retains 576 native process reports: three cohort lineages, each with a 12-block A/A control and a 12-block A/B comparison. Each study uses 64 cycles, seed `20261026`, diagnostics disabled, and the same frozen observation protocol. All rows remain separated by cohort and phase.

## Findings

| Lineage | Phase | Rows | Improvements | Regressions | Unstable controls |
| --- | --- | ---: | ---: | ---: | ---: |
| Original UI | A/A | 12 | 0 | 0 | 6 |
| Original UI | A/B | 12 | 0 | 3 | 6 |
| Starting redesign | A/A | 12 | 0 | 0 | 6 |
| Starting redesign | A/B | 12 | 5 | 0 | 6 |
| Actual PR #16 base | A/A | 12 | 0 | 0 | 6 |
| Actual PR #16 base | A/B | 12 | 0 | 0 | 6 |

The original UI A/B has three resolved findings: Compose command time `+17.4824%`, Split owner-thread cycles `+11.1539%`, and Split working set `+2.09308%`. The Split working-set controls were stable. Split private bytes changed `+1.7899%`, which is below the 2% memory investigation band and has no resolved finding; its controls were unstable. Starting-redesign A/B has five improvements and no regressions. The actual PR #16 base A/B has no resolved findings.

Across the 72 metric rows, 36 controls are unstable. Observer cadence coverage is complete in all six studies, but none verifies a quiet CPU window. System activity, process start/exit work, unselected external processes, GPU activity, and physical occlusion limit attribution. These results do not establish causality or performance acceptance.

## Source and evidence boundaries

The original UI and starting-redesign comparisons use SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`. The actual-base lineage is rooted at application commit `6b9aa29f2bf13b60c1c6c6189a26a0528b26b7ff` over comparison base `f33471da51773e6a0ee786a5dd29e46ceb21ecdd`; its A/A baseline uses SDK `47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a`, and its A/B comparison pairs that baseline with SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`. The full application candidate is `09f22877215a0084aae42f56c6e1ec1907079632`; its native source is `16bea061d17bee676f9f93d5f5bdadfd470ff8f2`. A passed documentation/checklist receipt refresh confirms byte-identical compiled inputs and artifacts; no native tests were rerun during that refresh.

The independent typed audit passed 25,202 checks. A separate raw replay rehashed all 576 reports, recomputed medians, 864 paired block effects, exact sign-flip values, Holm adjustments, and 1,728 control drifts, then round-tripped the CSV. These checks validate evidence integrity and arithmetic; they do not make an acceptance decision.

The projection uses synthetic cohort labels and repository-relative source paths. It omits absolute filesystem paths, machine identifiers, raw native payloads, and private receipt filenames. The hash index binds every raw report by cohort, phase, scenario, block, and slot. Raw reports and full observer streams remain in the private evidence store.

## Files

- `summary.json` contains all 72 rows, six lineages, observer summaries, and the raw-report hash-set digest.
- `metric-rows.csv` provides the 72-row table.
- `raw-report-index.json` binds each of the 576 raw report hashes without exposing source filenames.
- `source-identity.json` records application, SDK, and source-identity bindings.
- `evidence-index.json` records the independent audit and replay totals.
- `root-replay.json` records the independent numerical and integrity replay result.