# Design refresh evidence — 2026-10-22

The latest [tooling-r2 native qualification](documentation/tooling-r2/README.md) uses source `7380e49da3a27f2766812846c75781e927f62c701dc06a021ae751c9c55f0bc7`, compiled inputs `786c0c38759c7338dd82cb32ba6e05b47df61e8cbbfdaf514000e83f55eb6a40` and canonical DxUi `38ec36be42bf693dc649f1fe6b1dfc6cf34d4064`. All six builds pass; three fresh x64 Full runs each pass 52 cases and 20 actual harness captures, with separate ASan detection and crash/restart probes. A fresh authorized keyboard run passes 9 stages/168 inputs with actual `Confirmation=started` during approved AutoStart=true/30 seconds; foreground/focus unchanged and cursor restored. Its raw producer records remain private and are bound by length/hash in the derived record. Publication-checkout validation, formatting, scope explanation and changelog checks pass; no native execution in that checkout is claimed.

The original v5r3 native and current primary performance checkpoint uses source `538dd84941293cff8e7ecc6d2f8535cb597f0c0baea295e560cd795a0a221873` and compiled inputs `fde0747ac8b8100d67949d68a20e58fa2103e9b375f17fb0f22791b829c68270`. The only compiled-input-map change in tooling-r2 is the long-checkout hard-link tooling fixture; seven common harness inputs and all 14 app EXE/DLL/PDB files match that original checkpoint. Root directly compared all 60 new captures to the original images bytewise; no new direct visual inspection or primary performance run is claimed.

Retained original v5r3 archives: [Debug](native/38ec-v5r3-x64-Debug/archive.json), [Release](native/38ec-v5r3-x64-Release/archive.json), [ASan Debug](native/38ec-v5r3-x64-ASan-Debug/archive.json), [keyboard](native/38ec-v5r3-keyboard-x64-Release/archive.json). Their original selected member counts are 130/130/132 + 5; later publication exclusions below apply separately. V4 and all predecessors remain separately retained. V5's original combined remote-close assertion failure and v5r2 diagnostic-authoring compile failures remain source-bound and unattributed to a production root cause.

**Current fixed primary: COMPLETE, eight AB regressions; performance optimization/advice and R10 remain OPEN. Documentation continuation is verified for the original checkout.** No cost acceptance, quiet-host claim, silent rebaseline or trusted non-regression result is recorded. Creation-plan C1–C12/S5/S6 and broader product/platform gates remain separate.

Historical e62 candidate source `e61384940962103b9d91301d1fd134391b9c73e430af27218e61af572c03c4b8`, compiled `a2426b77fccc662d19c235df21e01d0fe3ae4ae913fc4fbd5615ca54b3a9adbd`, pin `e62dd9d9a91a6ad68d88f7f01a6dd075ae31dbd3`, retains its original observations. Nothing below is transferred to current v5r3.
The [superseded a57 packet](history-a57/README.md) retains the earlier six builds, three x64 Full runs,
20 capture pairs per profile, real sanitizer detection, nine-stage app keyboard sequence and baseline v2.
The [review diagnostic packet](failures/review-a57/README.md) preserves the subsequent failing regressions,
focused repairs and their exact source identities. The earlier [keyboard result](failures/keyboard-4cb/design-interactive.json)
and [translation diagnostics](failures/native-translation-4cb/tests.json) also remain unmodified.

The lasting contracts live in the [design system](../../Specs/UI/UI_DesignSystem.md),
[editor specification](../../Specs/Editor/Editor_Markdown.md),
[architecture](../../Specs/Core/Core_Architecture.md) and [validation rules](../../Specs/Testing/Testing_Validation.md).
The [design plan](../../Specs/Plans/WIP/DesignRefresh_2026-10-22.md) remains open during qualification.
Paired performance, the separately requested shared desktop window, native ARM64 runtime,
physical IME/touch/assistive technology and release acceptance remain distinct gates.

## Preserved performance history

The derived [observed 2f-v3 cohorts](performance/observed-2f-v3/README.md),
[2f startup phases](performance/startup-phases-2f/README.md) and
[38ec-v4 cohorts](performance/observed-38ec-v4/README.md) remain source-bound
historical observations. The latter retains eight AB regressions and its reviewed
projection structure (24 metrics, 576 drifts, 192 slots, 15 origins, 48 pairs and
192 references). Root verified the six derived files copied byte-exactly; producer
receipts remain unchanged and private. These results do not qualify current v5r3
or imply cost acceptance. The separate fixed v5r3 primary is now complete, as recorded below; eight regressions and the optimization/advice gate remain open.

## SDK documentation continuation

SDK documentation commit `9dec55676ac60332315d898f75af49250d952b96` normally continues exact producer `38ec36be42bf693dc649f1fe6b1dfc6cf34d4064`. Its four final checks and private exact docs-only continuation pass. [SDK draft PR #99](https://github.com/RedSalamanders/DxUi/pull/99) is open; no SDK merge or performance acceptance is implied. These are separate owner SDK results, not new app execution.


## Completed fixed v5r3 primary cohort — performance gate open

The predeclared fixed AA/AB collection completed successfully with no observer
failure. AA ran 2026-10-23 10:52:47–10:59:11 UTC and AB 10:59:11–11:05:36 UTC:
96 native slots per study, 192 total, 12 blocks per view, 64 cycles, seed 20261022.
This is the retained original cohort, unpooled with predecessors. The independent
typed C# numerical/integrity audit matches all 7,988 checks; it does not self-approve
the performance policy or establish improvement causality.

AA resolves no directional change in its twelve metrics. Command/CPU/private-memory
controls remain unstable; working-set and completed-frame controls are stable.
AB reports eight regressions. Completed-frame medians remain about 16.6 ms and clean
preparation about 17 ms without a resolved directional finding. Functional success
and these frame observations do not remove command CPU or retained-memory findings.

The following is a derived Markdown projection of the two original comparison
records. Values retain the stored metric units; change is the stored paired estimate,
not the ratio of displayed medians. No raw receipt or judge metadata is rewritten.

| Study | View | Metric | Baseline median | Candidate median | Paired change % | Adjusted p | Controls stable | Finding |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| AA | Compose | `commandUs` | 301.65 | 311.9 | 0.8844797646 | 1 | False | no-resolved-change |
| AA | Compose | `ownerCpuCycles` | 7657952 | 7672373.75 | -0.347941491917 | 1 | False | no-resolved-change |
| AA | Compose | `completedEditUs` | 16584.675 | 16591.5 | 0.0904913819598 | 1 | True | no-resolved-change |
| AA | Compose | `cleanCompletedUs` | 16918.75 | 16901.775 | -0.124341330497 | 1 | True | no-resolved-change |
| AA | Compose | `privateBytes` | 45338624 | 45287424 | -0.163753009681 | 1 | False | no-resolved-change |
| AA | Compose | `workingSetBytes` | 54198272 | 54298624 | 0.15589751985 | 1 | True | no-resolved-change |
| AA | Split | `commandUs` | 353.7 | 352.4 | 0.448994836606 | 1 | False | no-resolved-change |
| AA | Split | `ownerCpuCycles` | 9848463 | 9965377.75 | 0.473884705704 | 1 | False | no-resolved-change |
| AA | Split | `completedEditUs` | 16527.275 | 16540.5 | 0.0332943403101 | 1 | True | no-resolved-change |
| AA | Split | `cleanCompletedUs` | 17042.075 | 16975.4 | -0.262824201514 | 1 | True | no-resolved-change |
| AA | Split | `privateBytes` | 45551616 | 45076480 | -0.722468221115 | 1 | False | no-resolved-change |
| AA | Split | `workingSetBytes` | 54548480 | 54501376 | -0.160295476382 | 1 | True | no-resolved-change |
| AB | Compose | `commandUs` | 304.2 | 351.8 | 15.3122496301 | 0.005859375 | False | regressed |
| AB | Compose | `ownerCpuCycles` | 7634151 | 8142184 | 5.80605185096 | 0.009765625 | False | regressed |
| AB | Compose | `completedEditUs` | 16574.125 | 16563.6 | -0.00361137085094 | 1 | True | no-resolved-change |
| AB | Compose | `cleanCompletedUs` | 16908.25 | 17023.55 | 0.140640804956 | 1 | True | no-resolved-change |
| AB | Compose | `privateBytes` | 44883968 | 49682432 | 10.427869688 | 0.005859375 | False | regressed |
| AB | Compose | `workingSetBytes` | 54200320 | 56166400 | 3.55054340712 | 0.005859375 | True | regressed |
| AB | Split | `commandUs` | 360.05 | 390.575 | 9.7936920256 | 0.005859375 | False | regressed |
| AB | Split | `ownerCpuCycles` | 9704452.75 | 11301005 | 16.6372233743 | 0.005859375 | False | regressed |
| AB | Split | `completedEditUs` | 16589.7 | 16521.8 | -0.332573419995 | 0.228515625 | True | no-resolved-change |
| AB | Split | `cleanCompletedUs` | 16922.875 | 16943.875 | -0.130491614825 | 1 | True | no-resolved-change |
| AB | Split | `privateBytes` | 45305856 | 50378752 | 11.3333101601 | 0.005859375 | False | regressed |
| AB | Split | `workingSetBytes` | 54544384 | 57016320 | 4.60329047642 | 0.005859375 | True | regressed |

Both host records have complete 378-sample coverage. AA maximum sample gap is 1.0328 s, mean total busy 9.0984%, maximum 29.1866%; AB maximum gap is 1.0307 s, mean 9.3312%, maximum 36.8269%. Neither window is verified quiet or exclusive. Selected-process observations are incomplete measures of competing work; GPU activity and physical occlusion are unmeasured. These limitations do not erase the recorded regression findings or establish their cause.

The reviewed [derived v5r3 projection](performance/observed-38ec-v5r3/README.md) retains all 24 metric rows, 576 control drifts, 192 native slots and 15 origin metadata references. Root independently reran its verifier against the original records, including all 7,988 numerical audit checks. Original metadata stays private at `.build/paired-design-refresh-38ec36be-app-v5r3-20261023/{aa,ab}/comparison.json`, `.build/paired-design-refresh-38ec36be-app-v5r3-20261023/collection.json` and `.build/paired-design-refresh-38ec36be-app-v5r3-20261023/host-summary.json`. The independent audit is `.build/design-refresh/performance-investigation-38ec-v5r3/independent-audit.json`. These private paths are source-bound evidence references, not links to absent checkout payloads.

**Disposition: performance optimization/advice gate OPEN; R10 and this plan remain WIP.** No cost acceptance, threshold relaxation, silent rebaseline, quiet-host claim, improvement attribution or trusted non-regression verdict is recorded. Current code may be published for review in a DRAFT PR while deeper memory diagnosis continues. An ablation/diagnostic is not this fixed primary cohort and cannot replace or be pooled with it.

## Verified documentation continuation

The native and performance records identify tested source `538dd84941293cff8e7ecc6d2f8535cb597f0c0baea295e560cd795a0a221873`, compiled inputs `fde0747ac8b8100d67949d68a20e58fa2103e9b375f17fb0f22791b829c68270` and DxUi pin `38ec36be42bf693dc649f1fe6b1dfc6cf34d4064`. They are not execution of a later documentation commit.

The [derived continuation record](documentation/v5r3/continuation.derived.json) binds documentation source `b97be14e8cda559f4a2fec1e7db4b7231b6be88821d4e913550439afb2ff57c6` to six fresh source-bound build receipts and the unchanged 97 compiled inputs, seven common harness inputs and 14 required EXE/DLL/PDB files. Root verified preserved and live bytes, dependency/compiler/profile/scope joins and the original run identities in the original checkout. Validation, formatting, scope explanation and changelog fold checks pass separately. Environment binding derives from the saved source freezes, current verifier digest and original ordinary-run OS/CPU/render subset; it is not a new or continuous environment observation. The private verifier's initial array-wrapping error and corrected successful attempt remain preserved.

This continuation retains the original runtime observations and supplies no new native, capture, resource, clipboard or benchmark execution. It does not qualify the separate publication checkout or native ARM64, establish performance acceptance, or satisfy hosted CI. The hosted six-Full-step continuation rule remains separate.

Creation-plan C1–C12 and S5/S6, broader C5 keyboard/menu/clipboard/physical IME/touch/AT, native ARM64 runtime, shared SDK policy, CI and release requirements remain open under their owning conditions. No parent checklist or active plan link changes here.

## Baseline recovery and selected publication

[Baseline v8 provenance](baseline/v8/README.md) retains the original UI with the same SDK pin and seven common performance inputs, including a verified source-only recovery bundle. Its private executable and raw compiler records remain outside this packet. The paired studies independently checked the executable and compiler identities before collection.

The selected publication omits the ten historical raw files listed in [PrivacyExclusions.json](PrivacyExclusions.json). Their byte lengths and hashes remain auditable; the exact originals remain in private evidence storage. The unchanged producer ledgers describe their original sets. Nine historical refusal records and one restore log referenced by those ledgers are deliberately absent from the selected public packet.

Each of the three ordinary profiles in 2f-v3, 38ec-v4 and 38ec-v5r3 also omits its one private refusal payload, with the omission recorded in that profile's archive. Their original keyboard selections omit zero private refusal payloads; the additional foreground-metadata selection below still applies. These nine refusal omissions are separate from the ten-file historical manifest. Selection preserves failing results and unfavorable performance observations; it does not turn a producer archive into a complete public replay bundle.

[Three additional baseline-metadata exclusions](PrivacyExclusions.additional3.json) record machine identifiers found in historical `environment.machine` or `candidateEnvironment.machine` fields. The three never-committed publication copies were moved into verified private recovery storage, and the original records and producer ledgers remain unchanged. The second manifest preserves their exact lengths, hashes and ledger bindings separately from the original ten exclusions and nine later refusal omissions. No identifying value is published or replaced inside a producer record.

[Six saved-foreground exclusions](PrivacyExclusions.foreground6.json) withhold whole historical keyboard receipt files containing `lease.SavedForeground` snapshots. Their never-committed publication copies were moved into hash-verified private recovery storage; original producer records and archive ledgers remain unchanged. The public manifest records paths, lengths, hashes and the field category, without the saved values. The selected JSON scan found no actual account SID, user-profile path, or machine/user value; field-name-only and boolean matches were rejected as false positives. This JSON review is separate from the changed-text identifier scan and synthetic screenshot provenance, and does not certify every file format. The latest tooling-r2 keyboard result is published only as a derived summary with original-record hashes.
