# Frame-stage diagnostic cohort: curated audit

This is descriptive diagnostic evidence only. It is not a primary performance verdict, quiet-host qualification, or policy approval.

## Cohort and bindings

- Owner `RedTilde.EditorPerformance`; fixture `redtilde-completed-native-v2`; fixture input SHA-256 `9CE1CA2DE3983070C40DC5A9DF331C9AA386FE796371E5527BF370AC70D3A10E`.
- 2 blocks, 64 cycles, seed 20261025; 32 reports (AA 16 same-binary baseline; AB 8 baseline and 8 candidate); 2 source and executable bindings.
- Baseline commit `b555da30e93f20047c5864bcba3e788bfecfd093`, source `f47fe2000156ae3bab76f8d9f987af49b809055576b82b75218398db0dbaaa35`, executable SHA-256 `03A93C3A5DEFB9CAF1ED731A5BB2DC394059442FD6ED58E89A82DA3480BC9794`.
- Candidate commit `bae15722fa1ad47b97239ee1b32e53cdfd6f1fab`, source `f2d0cd8f138465ac2e53f52210d49868c163a324309663bb95306623672a3169`, executable SHA-256 `7351A1E123834285EF61C5491ADB631B00031FF7DCE55DC53241FD4B5BE8DC7A`.
- Shared SDK commit `1714f8b0c0fa628bcef56de76078f85c869fc50c`. Both builds bind 58 identical public headers with inventory SHA-256 `3F93DB4B8729292118664AC0E513A9DFE73705BD04C944B2AA57888E196543EC`; source set `36AE5E2C8796110F71E4D77DA4D682CB69E8292F18CE0DF53226ABEDC531A7D5` and package set `38C35CF4C74E24BEE7B08D67D8AE89E7A5A100EF705DB1CB54B55738A4D5D2E7`.

Eight shared workload/harness/fixture input hashes are in [the numerical projection](summary.json); only stable relative names and hashes are included.

## Integrity counts

- 32 distinct native-report SHA-256 values, 8 pair-receipt SHA-256 values, 6,144 transaction records (4,096 edit, 2,048 clean), and 71,680 attempt tuples / 286,720 raw scalar values. 6,143 distinct transaction raw hashes were recorded; the repeated digest remains preserved in the raw audit.
- All 6,144 transactions have complete requested attempt sequences. Reports, executable bindings, pair schedule, and source roles were verified by hash.
- Correct per-field zero counts across the 71,680 tuples: totalUs 0; updateUs 66,333; renderUs 65,536; presentUs 65,536. This is 197,405 zero stage fields total; 66,333 tuples had at least one zero stage field. These fields are retained literally and do not classify success or failure.

## Per-process transaction wall summaries

For each transaction, sum all attempt totalUs; compute the median transaction sum inside each native process; then report the median of those process medians for each phase, role, view, and kind. AA and AB remain separate. The adjacent per-attempt median is an independent descriptive statistic, not a transaction total. All times are microseconds.

| Phase | Role | View | Kind | Processes | Transactions per process | Median process-median transaction total us | Median process-median attempt us |
|:--|:--|:--|:--|--:|--:|--:|--:|
| AA | baseline | Compose | cleanTransactions | 8 | 64 | 1996 | 1996 |
| AA | baseline | Compose | editTransactions | 8 | 128 | 3360.75 | 53 |
| AA | baseline | Split | cleanTransactions | 8 | 64 | 2579.75 | 2579.75 |
| AA | baseline | Split | editTransactions | 8 | 128 | 4516.25 | 51.5 |
| AB | baseline | Compose | cleanTransactions | 4 | 64 | 2031.5 | 2031.5 |
| AB | baseline | Compose | editTransactions | 4 | 128 | 3437.5 | 53.75 |
| AB | candidate | Compose | cleanTransactions | 4 | 64 | 1916 | 1916 |
| AB | candidate | Compose | editTransactions | 4 | 128 | 3336 | 54 |
| AB | baseline | Split | cleanTransactions | 4 | 64 | 2620.25 | 2620.25 |
| AB | baseline | Split | editTransactions | 4 | 128 | 4549.5 | 51.5 |
| AB | candidate | Split | cleanTransactions | 4 | 64 | 2765.75 | 2765.75 |
| AB | candidate | Split | editTransactions | 4 | 128 | 4727.75 | 52.5 |

These wall durations are not CPU attribution and do not replace primary benchmark measurements. Callback attempt tuples can include early-return attempts.

## Observer evidence and limits

- The observer status file records `Running` with no errors. Source inspection confirms this is written in `finally` before the script calls `Stop-Job`, `Receive-Job`, and `Remove-Job`; it is an intentional pre-stop snapshot, not a post-cleanup job state.
- Sampling coverage was complete: AA 80 samples, maximum gap 1.0583 s; AB 78 samples, maximum gap 1.0608 s. Quiet CPU was false for both, so there is no quiet-host qualification.
- Independent typed audit: 226,562 checks, reference matches for both studies, but `policyAcceptance=review-required`; the audit explicitly does not self-approve policy.

Input artifact hashes and all twelve phase/role/view/kind summary rows are in the adjacent JSON.

