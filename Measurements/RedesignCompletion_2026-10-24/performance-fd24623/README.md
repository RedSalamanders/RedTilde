# Redesign completion performance — 2026-10-25

**Status: review-required.** This sanitized aggregate projection preserves two distinct baseline questions: the original UI baseline and the starting-redesign baseline. It contains all 48 metric rows (two cohorts × two study types × 12 slots). It is descriptive evidence, not performance acceptance, a rebaseline, or qualification of the SDK measured-base policy.

Each study used 12 independent ABBA/BAAB blocks per scenario, four serial processes per block, 64 measured cycles per process, two warmup cycles, and seed 20261024. The A/A phase repeats that cohort's frozen baseline executable as both roles; A/B uses the baseline as A and `fd24623` as B. Both measured role identities are retained separately for every phase. The 12-slot Holm family covers Compose and Split. Bands of 5% for timing/CPU and 2% for memory are investigation thresholds, not permitted degradation limits.

## Original UI baseline versus candidate

Five of 12 A/B slots are classified regressed after Holm adjustment: Compose command time and private bytes; Split command time, owner CPU cycles, and working set. Seven slots show no resolved change. Control stability remains recorded per slot in the structured projection, including unstable controls. These observations do not identify cause or establish acceptance.

## Starting redesign versus candidate

Zero of 12 A/B slots are classified regressed after Holm adjustment. Three are classified improved (Compose private bytes, Compose working set, Split private bytes); nine show no resolved change. Control stability remains recorded per slot, including unstable controls. This is not an acceptance or rebaseline decision.

## Host and evidence limits

All four studies recorded observed host activity and complete sampling coverage, but none verified a quiet CPU window. The host was observed loaded, not quiet. No exclusive lease, physical isolation, or GPU isolation is claimed. Late fixes were not measured. The independent audit files are SHA-256 bound in the structured cohort, phase, and source records of `performance-proposal.json`: the original-lineage audit has 7,988 checks and digest `1A633E09A47FB3D68EB9B083EEF35EBC39A8A502D0FD677DD20773DCD9AB1951`; the starting-redesign audit has 8,015 checks and digest `1740D37EEDBE3B7B50076FB5E31027110AFFBEC2EE2448CBEF4D3EF70E496988`.

Raw receipts, process and native-slot records, observer event streams, toolchain details, private manifests, user paths, machine identifiers, and host-specific environment data are excluded. The prior `performance-fa0757d` projection remains unchanged and is not pooled with this collection. See [performance-proposal.json](performance-proposal.json) for the complete 48-row projection and frozen source/input hash bindings.
