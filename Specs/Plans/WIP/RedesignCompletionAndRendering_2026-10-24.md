# Finish RedTilde redesign and DxUi rendering — 2026-10-24

## Current closeout checklist — 29 October 2026

**Owner decision, 29 October:** the user explicitly accepts the measured original-UI working-set costs
of +2.0045% in Compose and +2.2195% in Split and keeps every other gate open. This disposes of the memory
cost/advice item; it does not alter the measurements, bands, command findings, SDK policy or CI obligations.

- [x] Remove 49.44 GiB of proved-obsolete caches and binaries from the four existing Red-project Perf roots; preserve live dependencies, useful failures and recovery data. The eight original automatically blocked removals, a later stray documentation-draft directory and a mislabeled private controller copy remain intact; no blocked deletion was retried.
- [x] Recheck both latest main workflows. DxUi main `0e372f56` adds only the separate production-review plan; run `40861307533` is cancelled and its overall gate fails. The previous native-bearing main2cf run retains its ARM64 popup geometry-readiness failure. RedTilde mainbae remains explicitly billing-blocked with zero executed steps; hosted consumer ARM64 stays open.
- [x] Correct DxUi's stationary-pointer setup at source9d and pass the exact eight-case Debug Menu smoke, zero skips, with automatic-start lease and verified focus/cursor restoration.
- [x] Correct and qualify the tooling journal publisher at SDK `dbb6f1be`: recognized non-destructive replacement errors get bounded retries after file-ID/length/hash verification, with a deadline check after verification. The copied 12-case suite and final full static run pass. Preserve the prior source9d failure and its unassigned initiating cause; the real deadline control distinguishes old publication from corrected refusal.
- [ ] Finish SDK final-source runtime and integration at `dbb6f1be` ([PR131](https://github.com/RedSalamanders/DxUi/pull/131)). All six local builds and full static checks pass. All six hosted native jobs pass. Final local qualification passes 57 ordinary suites, actual ASan detection and twelve foreground outcomes with zero skips and three completed automatic-start leases that preserve focus, foreground and cursor. Independent replay checks all 69 suite receipts and six actual process exits. Paired CI remains inconclusive: all 144 receipts and 78 metric rows reconcile, but all 72 controls are unstable and base policy remains unqualified; the paired job and overall CI gate fail. Functional success does not close this performance gate. Prior source9d passes all six hosted native profiles; retain it as historical.
- [x] Pass all fourteen private Debug allocation modes and three raw reconciliations. Diagnostic symbol queries resolve all fourteen retained frames; the redundant SYMFLAG_FUNCTION requirement rejects valid PDB functions with Flags0. No app recapture or ownership claim follows.
- [x] Complete the private oracle protocol: all 28 original obligations close after two exact caller queries, caller replay and four reason-specific PDB negatives. Retain all failed drafts and reuse the allocator captures. This proves the oracle protocol only; app caller coverage and ownership remain unproved.
- [x] Close the bounded Debug allocation route with its explicit limits. Both identical SDK086 overlays pass canonical Debug builds and compiled audits, including all 41 archive members and five Debug CRT imports. The actual current Compose four-cycle smoke passes once at `422317dc`, with 67 retained hashes independently checked. Replay joins 167,689 events to 3,917 live blocks/397,916 requested Debug bytes; 169,352 bytes have a known creator within the recorded prefix and 228,564 remain unknown. Every live stack reaches the twelve-frame cap. The fixed eight-process, 64-cycle family stops at slot one: 1,664,689 attempts exceed the unchanged 262,144-event limit; the exact 760-byte refusal reports EventOverflow plus CountMismatch, with no reentry, paused-allocation, predecessor-denial or in-flight fault. Seven later slots remain NotRun, and no final snapshot is admitted. Independent replay preserves all partial outputs, verifies 1,374 bound hashes and observes no remaining owned process. This route does not establish Release working-set ownership or resolve either memory finding; do not increase the cap or rerun it until successful.
- [x] Evaluate and reject lazy optional-pane creation as this repair route. Source `07191110` preserves root order and creates the ten Files/twenty Settings controls on first use, with one construction state per pane. Debug and Release builds and all 49 Editor cases in each profile pass; full validators and formatting pass. Two new cases verify first opening, public tree/UIA/focus behavior, reuse, preloaded/live preferences and source/history. Preserve the initial protected-accessor compilation error and subsequent 47/49 run: the overlay fixture then used a raw preference flag, and the caption case failed a combined foreground/visibility assertion with an unassigned cause. The corrected fixture uses the actual Files command and retains both caption checks with separate diagnostics; its later passes do not explain the earlier caption failure. Preserve the two overlong-path restore refusals and successful shorter-worktree restore. The fixed 32-process, four-block Release scout passes collection and independent replay (1,635 typed checks): Compose median working set drops 268,288 bytes in 4/4 blocks; Split drops 147,456 bytes by median but only 2/4 blocks decline. Command geometric changes are +1.0764%/+1.7173%. The predeclared consistency criterion requires 3/4 in both views, so reject the route without reruns, extensions or threshold changes. Host coverage is complete but CPU is not quiet. Preserve the source and exact final products compactly before reusing the checkout. No full six-profile qualification or production adoption follows this rejected scout.
- [x] Isolate and reject disabling initial root-child batching as the memory repair. Private source `8d0fdee2` changes only the current option and its strict zero-flush test expectation; the canonical Release build, 47 Editor cases, validators and formatting pass. All four resource processes and 32 fixed scout processes complete. Median working-set decreases are only 2,048 bytes in Compose and 65,536 in Split, with only 1/4 declining blocks in each; both required at least 3/4. Command changes are −1.2150%/−1.4591%, but median private bytes increase by 5,113,856/3,934,208 bytes and all eight block contrasts are positive. The independently replayed 1,635-check scout retains 253 input/output hashes, all twelve primary decisions and all resource operations. First-completion wall decreases, while resize/recreation costs are mixed; these post-setup windows are not startup-to-visible measurements. The route is rejected without reruns, full qualification or adoption. Preserve the exact source/products and raw evidence compactly. Historical allocation ownership, the original two stable working-set findings, two command findings with unstable controls and shared performance policy remain open.
- [ ] Requalify any adopted consumer/SDK change, collect the predeclared fresh comparisons, archive the small public projection, update authoritative specs/changelogs and close only completed plans. Preserve R10, policy and hosted ARM64 holds.
- [x] Record concrete [performance disposition advice](../../../Measurements/RedesignCompletion_2026-10-28/performance-advice.md) after rejecting the bounded repair routes. A further source review finds existing unchanged-caption/text guards, lazy folder-service startup and hidden-ancestor preparation guards, without a defensible small new repair. The user then explicitly selects acceptance of the measured memory cost while keeping other gates open. Stop further memory-repair experiments for this disposition; retain every measured finding and rejected route. No policy approval, unstable-control acceptance, CI waiver or rebaseline follows.
- [x] Close superseded SDK draft PR124/125/126/128/130 and remove their five remote and five unused local branch names. Each exact head is an ancestor of retained PR131/F and recoverable from a verified complete Git bundle; remote deletions use exact-head leases. Keep PR131, PR132, the active diagnostic branch, useful evidence/worktrees and all dirty primary files. This retires duplicate review refs, not unqualified functionality or CI obligations.
- [x] Qualify the focused SDK clean-Prepare diagnostic correction at native producer `8ccb8a14`, recorded by metadata child `c1c207a8` in [PR134](https://github.com/RedSalamanders/DxUi/pull/134). Every measured/warmup call requires `S_FALSE`; sticky negative oracles reject an earlier `S_OK`, failure or alternate success. Six canonical local builds, three x64 real native oracles, three full Embedded runs and actual ASan detection pass. Full affected Tooling/validators and formatting pass. Independent replay verifies 737 file bindings and all six header/ASan compilation profiles; 76 existing products and 301 reports remain unchanged. Local ARM64 profiles are cross-builds, and the direct isolated runs are scoped evidence rather than a default full-run attestation.
- [x] Complete the diagnostic's actual hosted x64/ARM64 Debug, Release and ASan qualification at SDK compiled merge `b7d43757`, whose tree equals metadata head `c1c207a8`. All six jobs, 126 declared Full outcomes and eight consumer modes pass with zero skips; two expected ASan detections remain separate. Independent replay verifies 505 payload hashes. The [reviewed native/precision evidence in PR136](https://github.com/RedSalamanders/DxUi/pull/136) preserves exact identities, literal interaction states and the native ZIPs' lack of raw executable custody. Physical IME/AT remains unqualified.
- [x] Review the final [PR134 CI result](https://github.com/RedSalamanders/DxUi/actions/runs/40869356681). Both builds/collection and artifact upload succeed; verdict publication and overall CI fail because the measured-base policy is not independently qualified. The final verdict is inconclusive. The source-bound numerical replay belongs to the SDK packet; neither native success nor this separate diagnostic disposes of unstable command controls or approves policy.
- [x] Complete one separate eight-process precision diagnostic at SDK producer `8ccb8a14`: all 1,024 rows and 74,880 timed calls per role reconcile. All 64 per-process summaries retain outliers and separate empty-loop context. The median of sixteen 512-call normalized batch means per process is 16.016–16.211 ns/call; this is not isolated latency, accuracy or canonical per-call p95. A compact 71-member archive verifies every retained member without duplicating the native product archive.
- [x] Recheck RedTilde PR27 at exact documentation head `f9f5d1cb`: [run 40872751037](https://github.com/RedSalamanders/RedTilde/actions/runs/40872751037) starts zero steps and records explicit account-payment/spending-limit annotations; native jobs skip. Hosted RedTilde ARM64 stays open.
- [ ] Close shared control/policy, consumer adoption, integration and platform/release obligations only with their required evidence. The consumer retains SDK086; acceptance of the measured application working-set cost does not waive CI, qualify a quiet host or approve SDK policy.
- [x] Archive the rejected optional-pane/batching worktree and remove its two unused private branch names after source/product/raw recovery verification. Remove its separately approved 1,424,469,920-byte pinned restore cache and unlink two fixture junctions without traversing them. The managed archive leaves a dangling checkout; all remaining files are removed after rechecking their recovery and unchanged source. Only an empty directory remains in use by another process; its owner is unassigned, and no process is killed to remove it. Earlier automatically blocked targets stay intact.

The detailed chronology below retains source identities, rejected routes and evidence boundaries. This checklist is the latest execution status.

The [lazy optional-pane projection](../../../Measurements/RedesignCompletion_2026-10-28/lazy-pane-counterfactual/README.md)
and [initial-batching projection](../../../Measurements/RedesignCompletion_2026-10-28/initial-batching-counterfactual/README.md)
record every scout estimate and block sign behind their rejections. The earlier failed Debug caption run retains
its source, logs and binary digest; its exact executable/PDB were not separately frozen before the corrected rebuild.
Do not represent that historical failure as a recoverable binary pair or assign its cause from the later pass.

## Resumed performance and retention checklist — 28 October 2026

- [x] Recheck latest integrated DxUi main CI: run `40681696889` at `2cf30d7e` fails ARM64 Release at the acrylic popup geometry-readiness assertion. The subsequent runner correction is under separate source-bound qualification; the earlier all-six-native pass at `881e5d5f` remains historical. Neither result qualifies the performance policy.
- [x] Recheck latest RedTilde main qualification: run `40740813230` at exact main `bae6e34e` again executes zero steps under an explicit account-payment/spending-limit annotation; native jobs skip. Keep hosted consumer ARM64 open. Earlier main runs and PR refusals remain historical.
- [x] Inventory all four existing Red-project `.Perf` roots without following reparse points. Retire 192 obsolete cache directories and 2,187 old binary files: 53,082,468,395 logical bytes (49.44 GiB), 981,708 files, zero deletion failures. Keep markers, six live dependency target roots, current comparisons/frozen outputs, source bundles, dirty copies and recovery records. Explicit retirement notices supersede historical retention of the removed payloads; no new copy of disposable caches is retained.
- [x] Parse and hash-check all 7,748 retained SDK host observations and join 144 observed native process lifetimes to raw completions. Independent replay confirms the mapping. The distinct-grid B01 slowdown overlaps high service/update-process CPU; B02 has much less such activity. No compiler CPU advance appears in the sampled native-lifetime intervals. These are sampled overlaps, not exact-frame scheduling, a quiet-host guarantee or causal attribution.
- [x] Complete the new SDK scenario/phase diagnostics and independently replay the fixed 34-process collection: 32 factor processes plus two separate functional smokes, with two fresh processes per factor cell. All raw reports and 1,195 host observations reconcile. Default-WARP clean composite tails are dominated by blocking readback; dirty tails share preparation and readback costs. Calling-thread WARP is slower on composite paths and is rejected as a production optimization. No canonical threshold, policy or benchmark input changes.
- [x] Qualify diagnostic SDK producer `0b8e76c5` in six local build profiles and three x64 Full runs: all 57 suite executions pass without capability skips. Full validators/tooling, clang-format 22.1.3 and scoped Explain pass. Public library source/headers and the six canonical benchmark inputs are unchanged; this is diagnostic coverage in [PR122](https://github.com/RedSalamanders/DxUi/pull/122), not performance acceptance.
- [x] Prepare and independently check the two state-aligned app census builds, their diagnostic setup overhead, 93 driver controls and 80 bound driver/input hashes. Preserve the first smoke refusal: the retained observer rejects an output prefix outside its original workspace, before any app process starts. A path-only continuation must retain the same binaries, fixed order and measurement rules.
- [x] Complete the path-only continuation: one separate smoke and all eight fixed census processes pass. Independent replay performs 6,801,476 checks over 189 raw payloads. Current-minus-original primary working-set medians are 933,888 bytes in Compose and 1,273,856 bytes in Split; diagnostic setup adds 12,288 resident bytes in every process. Images, private ranges and font-cache mappings contribute, but allocation ownership remains unproved. Two processes per condition are descriptive diagnostics, not a replacement acceptance comparison.
- [x] Test and reject global icon-layout suppression as a production repair. Repaired private source `631588e0` passes the registry/pixel oracle, separate smoke and all eight fixed processes under declaration `521321D6…`; independent replay passes 6,828,382 checks. Observed savings are only 96,256 bytes in Compose and 79,872 in Split, with inconsistent Split effects and added preparation/CPU cost. The [packet](../../../Measurements/RedesignCompletion_2026-10-28/icon-layout-counterfactual/README.md) retains the failed initial smoke and corrected preservation scope. This does not identify the original allocation owner or reject font-cache ownership.
- [x] Profile the exact frozen current E090/C994 binary without new source/builds: four 64-cycle processes in Compose/Split/Split/Compose order, complete host coverage, nine edit phases and twelve selection components. Independent replay passes 1,014,275 checks. Caption, identity and chrome dominate current command cost; individual selection/status components are smaller. Instrumentation retains about 6.9 MB per process, so its memory is not acceptance evidence. Keep historical profiles separate.
- [x] Integrate diagnostic SDK PR122 as `592b9fb9` with a merge tree equal to qualified source `0b8e76c5`; all six PR native profiles pass. Paired collection/upload pass but publication stays inconclusive under the unqualified base policy, with 71/72 unstable controls. Main run `40668541474` passes five native profiles but fails ARM64 Release at a Menu popup assertion; investigate that distinct failure. The completed diagnostic scope moves to SDK Done; shared policy/control and consumer R10 remain open.
- [x] Reject the caption-message delegation trial `00ae9103` as a performance repair. Its trial-specific routing oracle fails before the change (47/48) and passes after it (48/48): it deliberately disables the root and requires caption traffic to leave invalid logical focus for the next ordinary host message to prune. This confirms the proposed dispatch change, not an existing source/history defect. The predeclared 16-process, diagnostics-off scout finds no repeatable gain: Compose command +7.89%, Split +0.27%, with unstable controls. All twelve results are unresolved at this small sample; no full qualification or production adoption is justified. Preserve its source, frozen artifacts and raw scout separately.
- [x] Reject the hidden-file-pane worker hypothesis on exact current source: an empty hidden root never initializes its service, whose default constructor owns no worker, event or buffer. Retained diagnostics show zero visible-file capacity. Both exact executables reserve 1 MiB per thread; the historical extra 4 MiB remains unattributed.
- [x] Qualify SDK RTL fallback producer `85716c8f` locally: six builds, 57 normal x64 suite executions, twelve foreground/resource suite executions and two focused Menu cases pass without skips; ASan detection and restoration are verified. Hosted PR124 run `40682230783` passes all six native profiles. Its paired result has zero resolved primary regressions across 78 metrics but 70/72 unstable controls; publication remains inconclusive under the unqualified base policy. Neither library work nor metadata integration changes the consumer's exact pin.
- [x] Diagnose two qualification weaknesses with controlled checks: replace an existing progress journal atomically with `File.Replace` while delete-sharing readers retain coherent snapshots; clear the intentional ASan child status only after successful finalization. Preserve historical caller status separately from sealed native outcomes. Seven focused publisher cases and 37 scoped-tooling cases pass; the original generic child-readiness failure remains unattributed.
- [x] Investigate latest SDK main `2cf30d7e` run `40681696889`: its ARM64 Release failure waits for nonzero popup geometry before blur assertions. The acrylic fixture now uses the existing modal-dispatch handshake with one unchanged discovery/readiness deadline. Blur thresholds and production Menu behavior are unchanged. The generic failure does not establish its original external cause.
- [x] Finish all six local builds at `103dff20`; hosted Windows/Linux validators and format pass. Its local validator fails only the profiling-tool suite; a captured focused run passes 167/168 and fails the three-second end-to-end inherited-pipe assertion. Controlled setup of 3.2 seconds reproduces that boundary error while the exact holder stays alive at return. Preserve the original untraced failure separately.
- [x] Repair the profiling fixture's startup/drain boundary at final SDK producer `6e90427b`: keep the 0.2-second drain budget and three-second drain observation bound, require exact live-holder identity and acknowledged release, and reject an EOF-wait substitute even with plausible timing metadata. Independent review and all 168 profiling-tool cases pass; no product wait or performance threshold changes.
- [x] Preserve source `6e90427b`'s ASan Menu failure with its six builds, 57 normal suites and 11/12 foreground suite results. The failed sibling-hover case has B1 already hovered but no delayed-open timer. A controlled test-only red reproduces the initialization gap; the correction starts the existing delay after popup display. This is a product correction, with final-source qualification tracked below. Every lease restored desktop state; no unchanged failed case was retried.
- [x] Complete the predeclared Release `/Os` image-size feasibility gate and a same-workspace `/Os` → `/O2` → `/Os` control at candidate `abc2255a`. All 19 app compilation units differ only by the intended `/Os` option; the exact DxUi archive is retained. Code decreases by 198,656 bytes and the image by 229,376 bytes, with matching repeated `/Os` geometry. This establishes binary-size causality only; it supplies no working-set, latency or acceptance result.
- [x] Build the original-UI and actual-PR-base `/Os` reference overlays without runtime-source, support, resource, pin or common-harness edits. Preserve the first strict refusal: their managed-path quoting differs from the candidate's compiler profile. Independent review also confirms that sealed V4 binds `/O2` compiler contexts and cannot admit `/Os`. No samples were collected and no profile normalization or seal was weakened.
- [x] Complete candidate `/Os` local native qualification at `abc2255a`: all six builds and three x64 Full runs pass, each with 67 cases/32 capture pairs; ASan detection and all nine interactive stages pass with verified restoration. Source, Release0090 and all 172 diagnostic-bound inputs remain unchanged.
- [x] Preserve the private diagnostic's first failed setup smoke. Native `/Os` Compose at four cycles passes with clean observer coverage, but collector completion cannot hash its still-open status file under the reader's sharing mode. Zero study slots ran. Repair this collector-only issue with a synthetic sharing regression and a separately sealed continuation; preserve the original declaration and all partial files.
- [x] Complete and reject the private `/O2` versus `/Os` repair route. After preserving the failed setup smoke, a separately sealed collector continuation passes its synthetic sharing regression, one smoke and all 32 fixed study slots. Independent replay reconciles all 18 estimates, 72 block rows, 100 raw hashes and 184 bound inputs. Compose working set increases by 124,928 bytes and declines in only one of four blocks; Split decreases by 194,560 bytes in three blocks. The predeclared criterion requires both views to improve by at least 65,536 bytes in at least three blocks, so the route fails. Preserve source, exact binaries and raw evidence compactly; do not adopt `/Os`, create a new compiler protocol, rerun samples or alter V4 on this evidence. The original stable working-set and unstable command-time findings remain open.
- [x] Independently recheck the compact rejection archive: all 26 outer files and 2,684 archive payload hashes match; 308 additional process-median, estimate and decision checks reproduce the rejection. Exact Git blobs and recovery bundles remain separate from runtime working-tree bytes. Publish the [small projection](../../../Measurements/RedesignCompletion_2026-10-28/size-policy-rejection/README.md) and retain raw evidence compactly.
- [x] Qualify SDK `d4a36c09` through six builds, six focused Menu cases and 57 ordinary x64 suite executions. Preserve its new Debug Menu keyboard assertion and the distinct hosted ARM64 Debug info-row assertion. Five hosted native profiles pass; the older paired job is cancelled by the newer push. The uninstrumented failure causes remain unproved; do not relabel those runs as green.
- [x] Reproduce pending-hover timer priority on test-only SDK `92193a77`: ordered Home input leaves the pending-open timer live before Right navigation. Preserve the single red, then cancel pending timers across the popup chain on keyboard takeover while preserving AltGr. Restore existing modal readiness, held foreground and owned-pointer fixtures for info-row coverage without weakening the original 80 ms assertion. The original hosted info-row cause remains unassigned.
- [x] Preserve SDK `762983a0`'s fixture compilation failure and correct its nonexistent `itemCount` access using the public item vectors. Final candidate `c155de84` passes full static validation, formatting, scoped Explain, ten focused Menu cases and the entire Debug Menu suite, with zero skips and verified restoration. Its independently checked Menu packet contains ten files/139,595 bytes, manifest SHA-256 `B09BC03FB579F5BD05E0ADE549C9A6B52E81E0914CEFFB512642E3A477C17E7F`.
- [x] Finish candidate `c155de84`'s local qualification: all six builds, 57 ordinary x64 suites and twelve foreground suites pass without skips; the actual ASan detector returns its expected child exit one. Every lease verifies desktop restoration. Independent review matches thirty output hashes (1,842,486,830 bytes), matrix SHA-256 `CE59E0E1577CF75F4E2629326E7D7D58ED66300B69CC4D3DA66319BC24EEE5DE`, plus 134 manifest files and all 57 ordinary suite receipts. Preserve the duplicate private Release wrapper's log-lock failure separately: the actual first process exits zero, independently observed by a retained native process handle. The transient first caller JSON was not retained byte-for-byte; its reconstruction is explicitly labeled. ASan then uses a reviewed single-use launcher and exits zero.
- [x] Audit all six hosted native artifacts for [run 40757949246](https://github.com/RedSalamanders/DxUi/actions/runs/40757949246): provider hashes/lengths, 724 archive entries and 468 extracted files match. All six MSVC 14.52 builds and relocated consumer checks pass. Four full native profiles pass; ARM64 Release fails the Right-arrow root-popup assertion and ARM64 ASan fails the stationary-mouse root-switch assertion. The 126 core scope/lease-child rows contain 124 passes, two failures and zero skips. ARM64 initial focus is unavailable on the minimized foreground, so its lease does not prove a focus round trip.
- [x] Independently replay that run's paired artifact: all 144 receipt hashes, 78 primary metrics, exhaustive sign-flip/Holm decisions and 24 exact budget maxima match. Zero resolved primary regressions coexist with 71/72 unstable same-side comparisons; the literal verdict is inconclusive/policy-review-required. All 57,600 preparation samples and 1,440 round p95 values occupy 100 ns QPC bins. The nominal 5% band for 600-900 ns clean preparation is smaller than one timer tick. This proves a resolution limit for those short observations, not a general explanation of composite/frame instability; no policy, bands or retained results change.
- [x] Correct the two root-switch fixtures at SDK `0a2279c7`: restore existing held-foreground attempts, owner-bound traces, modal readiness within the same discovery deadline and owned physical-pointer coordinates. Preserve the original assertions and 120 ms/800 ms bounds. Its canonical Debug build and six focused Menu cases pass with zero skips and verified desktop restoration. The original initiating causes remain unassigned. The test-only correction is [PR128](https://github.com/RedSalamanders/DxUi/pull/128), separate from the production correction in PR126; the consumer remains pinned to SDK086.
- [ ] Complete corrected-source SDK qualification at `9d974c0e`. Preserved `0a2279c7` passes all six builds, 57 ordinary x64 suites and actual ASan detection, but all six hosted Full native qualifications fail. Verified x64 artifacts and the ARM64 ASan log identify the new initial-A setup defect; other ARM64 failure causes are not independently mapped. The new test-only correction parks at A until readiness, then aligns B before the original single-switch/keyboard assertions. The canonical Debug build passes, with both build and incremental receipts accepted. Full static validation fails only the external-timeout tooling case: its child exits on File.Replace before publishing readiness; 7/8 consumer-progress cases pass. All five validators, format and both Explain commands pass. The final consolidation is SDK PR130 and the eight-case Debug Menu smoke passes with zero skips and verified restoration. Preserve the observed publisher failure; no unchanged retry or readiness timeout increase. Preserve original0a and the failed prelaunch wrapper separately; no unchanged foreground retry. Main consumer ARM64 remains billing-blocked.
- [x] Preserve metadata producer `e343d351`'s failed static run: format and both read-only Explain commands pass, but the auxiliary observer JSON does not fit the native-measurement schema and the reviewed CI digest needs the new tooling fixture input. Scoped digest correction passes 37/37 and the changelog check passes. Automatic review blocks removal of the observer JSON; leave it intact. Prepare a selective publication patch from base0a without adding that auxiliary file, retaining its exact private/superseded-branch copy and compact public summary. This changes no validation policy or measurement data.
- [x] Preserve the `0a2279c7` full-static failure: five validators and all other tooling groups pass, but the external-timeout journal fixture observes `Test-Path` success followed by a missing-file exception from `File.Open`. The temporary fixture was removed by its existing cleanup, so the exact initiating replacement cause is unassigned. Format passes; Explain was not reached. Do not rerun the unchanged failed group. A separate fixture-only correction will acknowledge child-body entry after active-phase publication and read the mutable journal after owned-child closure, retaining all state assertions and existing time bounds. Full tooling qualification remains open.
- [x] Prove the pinned Debug CRT allocation-hook semantics in an isolated private oracle: live allocation/reallocation request IDs and sizes reconcile, prior decisions are chained, overflow fails closed and missing-stack coverage is rejected. Bind all nineteen artifacts, three runtime modules and three UCRT source hashes. Owning-image frames alone do not prove named callers, allocation ownership or Release memory cost.
- [x] Audit dormant controls against pinned SDK086: the files pane adds ten controls and the settings drawer adds sixteen over the original UI. Invisible subtrees are pruned from UIA walks; the three additional hidden text fields acquire neither TSF sessions nor UIA providers eagerly. Those native-resource and hidden-UIA traversal hypotheses are unsupported. Per-instance retained bytes remain unquantified; no lazy-construction optimization is adopted from object counts alone.
- [x] Preserve and diagnose the private allocation-oracle setup history. Split storage resolves the initial compile refusal; later include/enum and collector path-case failures remain archived. The reviewed collector-only continuation reuses frozen V5 binaries/raw normal without recompilation or normal recollection. All fourteen logical native modes and three live-ID/count/type replays now pass, including overflow, concurrency, bootstrap, prior-hook and lifetime controls. All realloc outcome checks pass. Exact EXE/PDB GUID/age and loaded PDB bytes match, but the first caller-symbol query returns unresolved functions; stop with 16 Passed, five RetainedPassed, one Failed and six NotRun. The diagnostic-only helper now resolves all fourteen retained frames to SymTagFunction with positive bounds and source lines, but Flags is zero. Its extra SYMFLAG_FUNCTION requirement is the sole rejecting symbol predicate, consistent with the documented DbgHelp function example. A minimal helper-only correction removes that redundant flag requirement; both captured callers, pure caller replay and all four reason-specific negatives pass. All 28 original protocol obligations close, with zero repeated allocator/app captures. This is oracle-only proof; app caller coverage, ownership and performance acceptance remain open.
- [ ] Implement and review private Debug allocation attribution in two separate managed worktrees at current `bae6e34e` and original-UI `bb1a32ff`, both SDK086. Use identical bounded instrumentation, early process-lifetime hook publication, an audited pre-snapshot CRT heap barrier and exact offline EXE/PDB caller checks. Reject live-window heap activity, ambiguous IDs, missing frames or capacity failures. First pass isolated fault tests and one separate four-cycle smoke; only then release the fixed eight-process descriptive collection. Do not run app experiments during SDK native qualification or treat instrumented Debug memory as Release acceptance evidence.
- [ ] Retire the four rejected experiment worktrees after their redundant SDK caches can be removed. Automatic approval review rejected the exact cache-removal action with `blocked by policy` before execution; all four caches/worktrees remain intact. Earlier three blocked historical object-directory removals remain untouched. Two verified merged consumer remote refs are absent; no blanket local-branch deletion is justified.
- [ ] Attribute current app working-set and command costs, implement supported feature-preserving improvements, and run meaningful regression tests.
- [ ] Predeclare fresh comparisons for changed source, independently replay results, and synchronize normative documentation/changelogs. Existing unresolved results retain their original source identities.

> **ACTIVE — resumed by the user, 28 October 2026.** The current Markdown/caption styling work is implemented and locally qualified; current-source performance has now been collected and independently audited, but R10 remains open because the original-UI findings, unstable controls, unverified quiet CPU and unqualified base policy remain. Continue coding/tests/integration and remove only proved-obsolete branches/worktrees. The public projection is [here](../../../Measurements/RedesignCompletion_2026-10-28/README.md); private receipts remain in the controlled archive.
>
> **Historical pause snapshot:** SDK source33c was clean/published in draft PR121; Foundation12 and full NativeTextInput173 passed with verified restoration. Full SDK tooling validation for that older queue was user-cancelled, its then-proposed source33c consumer adoption remained unfinished, and CI replay, PrePush, merge and paired performance stayed pending. Hosted RedTilde ARM64 and the performance findings remain open.
>
> **Current measured source:** exact frozen RedTilde producer `e0908338cf5373978b5ef8d69cdd4ef6c3eba33b` (EXE SHA-256 `C994554BD46FD4663A6187A26D67443941D534096CDB5CCBA53D40446D402511`) with SDK `086ad82a`. Six consumer build profiles passed; latest x64 native runs `005858152Z`, `010003066Z`, `010103588Z` each pass 67 cases/32 capture pairs with ASan detection, and keyboard run `010330174Z` passes nine stages. The earlier producer `a5e75197` remains historical. Four matched performance studies (actual-PR-base and original-UI, each A/A and A/B) pass independent audit at 16,891 checks and 384 native slots. Actual-PR-base anchor `ad8f2ce2` was measured through overlay `38b1b7bd`; original-UI anchor `604bbaf3` through overlay `bb1a32ff`. Actual-base A/B has zero resolved regressions; original-UI A/B retains four. The base policy remains unqualified, two command findings have unstable controls, quiet CPU is unverified, hosted RedTilde ARM64 is billing-blocked, and SDK CI/tooling plus physical-platform limits remain open. The [ICF size scout](../../../Measurements/RedesignCompletion_2026-10-28/README.md#optimization-disposition) found no size change and was rejected. Older `eb0` results retain their original scope.
## Resumed integration and cleanup — 28 October 2026

The user explicitly resumed all tests/coding, remote-main integration and removal
of useless local/remote branches and worktrees. The preceding pause paragraph is
historical. Preserve original queue/STOPPED files byte-for-byte; the live continuation
is `.build/integration-20261028/`. Source-bound Markdown/caption results remain valid
only within their recorded native/pin/toolchain/profile identities.

- [x] Fetch both remotes, verify actual main SHAs and inventory both Git common roots, branches, worktrees, dirt and ignored evidence.
- [x] Reconcile exact local qualification: six consumer builds; all three latest x64 runtime configurations at `e0908338` pass, with 67 cases/32 capture pairs each and ASan detection; the nine-stage keyboard/focus run passes. Six standalone SDK builds and nineteen zero-skip native suites per x64 SDK configuration pass at `086ad82a`.
- [x] Complete authorized keyboard/menu/resource input runs for SDK eb0 and consumer af2 through the existing thirty-second countdown, warning and restoration lease: SDK Debug/Release/ASan four suites each, consumer Release nine scenarios, no capability skips and verified restoration. Preserve the ASan caller's inherited intentional detector exit separately from its passed canonical result; a new SDK fix below needs affected final qualification.
- [x] Reconcile current PR/CI producers and publish final code/auditor source. SDK086 passes all six hosted native profiles and external consumers; its paired measurement/upload pass but verdict is inconclusive under the unqualified base policy. Consumer source41f's hosted run40618106476 refuses before startup under billing; no native result is claimed.
- [ ] Pass hosted consumer qualification, including native ARM64, after Actions becomes available. The owner explicitly keeps this gate open while requesting integration.
- [x] Prepare current-source/common-harness comparisons against actual PR main and original UI, with the exact V4 public-header profile. Preserve earlier cohorts and do not reuse PB-only V3 seals.
- [x] Collect and independently audit fresh A/A and A/B studies for both references. Publish all 48 metric rows, same-side vectors, and source/profile/input hashes in the [public projection](../../../Measurements/RedesignCompletion_2026-10-28/README.md); private raw studies remain separate.
- [x] Disposition the two stable original-UI working-set findings through the owner's explicit 29 October cost acceptance (+2.0045% Compose/+2.2195% Split), after measured optimization options were rejected. Raw findings and bands remain unchanged.
- [ ] Resolve the two command findings with unstable controls and independently qualify the shared policy/control gate. The memory decision supplies no quiet-host finding, CI waiver or rebaseline.
- [x] Complete local final build/runtime/tooling/format and PrePush accounting, and synchronize normative specs/changelog. SDK documentation continuation `6c201b71` additionally passes all nineteen Debug suites and full tooling; all three native input identities and executable hashes remain unchanged. Deferred CI remains explicitly pending rather than passed; physical/platform and performance holds stay separate.
- [x] Merge reviewed SDK PR121 as `881e5d5ff252d5988c1357a5c64d14f480b46793`, preserving SDK086 ancestry, then consumer PR22 as `5d91134454693f5c1b4f05991b9cb719df491e4a`. Both remote merge trees equal their reviewed heads. The owner requested integration with disclosed holds; no performance cost is accepted.
- [x] Seal dirty work and useful evidence, remove thirty-six proved-obsolete local branches and nine merged worktrees, and verify both feature branches absent remotely. Both remotes retain only main. Dirty and frozen qualification/diagnostic worktrees remain useful and protected; cleanup is not a qualification pass.

No review-required performance policy, skipped job, unstable control, cross-build
or manual-platform hold becomes a pass because integration/cleanup was requested.

### Current source correction and preserved cleanup

The resumed ICF investigation reproduces a real SDK ownership defect: distinct
payload types have folded identical deleters, so deleter addresses cannot authorize
Take/Reclaim. Adopt exact SDK `086ad82a5872768db23ea83862ab4af13dff0c5b`, using
private writable per-type identity and a permanent Release regression that demands
actual folding. Its initial dcabe291 focused Release run passes; final native code
is identical, with a later test-only failure-path cleanup. The resulting exact-pin
consumer rebuild and qualification results follow. Earlier eb0 evidence remains historical.

Earlier exact-pin qualification used consumer producer `a5e75197` in `rtq`.
The latest six builds and three complete runtime/capture runs were repeated at
`e0908338`, as recorded above; the earlier executions retain their own identities:

- [x] Build all six consumer profiles with exact source/header/library/compiler bindings.
- [x] Run all native behavioral/capture/crash/recovery/resource obligations in x64 Debug, Release and ASan: each passes 67 cases and 32 capture/provenance pairs. ASan diagnoses the deliberate heap-use-after-free; independent tooling remains separate.
- [x] Run the final Release nine-scenario real keyboard/focus probe under the authorized thirty-second default Start lease: child exit zero; foreground, focus and cursor unchanged.
- [x] Build actual PR-base anchor `ad8f2ce2` through measured overlay `38b1b7bd` with SDK98 and original-UI anchor `604bbaf3` through measured overlay `bb1a32ff` with SDK086; all nine performance-harness input hashes match the separately reviewed V4 declaration.
- [x] Run matched-header/library compatibility probes for SDK98 and SDK086 using the actual consumer HostX86 toolchain: both pass, old capture links on both sides, new APIs link on the candidate, and Default Markdown initialization is verified. Record layout changes without claiming ABI interchangeability.
- [x] Independently audit and bind the new exact-pair V4 header/compiler/link proof. Both 58-row inventories, matching libraries/app binaries, source/result/TLOG/build receipts, probe arguments/project and Native32Bit build states reconcile; only the three declared headers differ. Normalize the generated profile to canonical LF before binding SHA94435355; old PB-only seals remain unused.
- [x] Test canonical V4 admission and independent evidence replay: 2,851 tooling assertions, 2,403 evidence self-tests and full repository validators pass. Preserve the first integration refusal from the draft-only unbound assumption; isolate that negative in a temporary module and use embedded retained probe source rather than a private-path dependency.
- [x] Refresh performance-reference harness and source/build identities, freeze exact binaries and predeclare the final collection before primary samples. Candidate is exact producer `e0908338`, EXE `C994554B…`; the earlier `a5e75197` native result is historical.
- [x] Verify all six current SDK hosted native profiles and external consumers at head `086ad82a`, tested merge `f2bad236`: x64 and native ARM64 Debug/Release/ASan all pass in [run 40605395948](https://github.com/RedSalamanders/DxUi/actions/runs/40605395948). Retain the actual sanitizer detections and zero-skip suite receipts. The paired job is a separate unfinished obligation.
- [x] Fix and exercise full V4 dependency-auditor closure with portable fixtures: 4,324 self-test checks and 16,891 checks over the four retained studies pass. Root additionally reconciles 1,817 public projection/source/privacy checks. No native implementation, thresholds, primary data or sealed profile changed in this audit-only correction.
- [x] Complete SDK086's final local four-suite input lease in Debug, Release and ASan: all twelve receipts pass without skips, and foreground/focus/cursor restoration is verified. The actual sanitizer detector passes separately. Retain earlier failed wrapper output rather than treating its caller status as a native failure.
- [x] Complete SDK086's paired collection and checksum-verified artifact12786737643. Independent review verifies all144 raw receipts and recomputes all78 metric medians exactly; 69/72 same-side controls are unstable and five raw trend flags remain, with no resolved set regression. Verdict publication fails/inconclusive because the base policy is unqualified. This is not performance acceptance.
- [ ] Complete independent SDK policy/control qualification. Keep billing-blocked hosted RedTilde ARM64 and physical-platform acceptance separate from the six passed SDK native jobs and twelve passed local foreground suites.
- [x] Collect and independently audit fresh original-UI and actual-PR-base A/A and A/B studies with the fixed protocol: four studies, 96 slots each/384 total, and 16,891 independent checks. Findings and limitations remain open as recorded in the public packet.

The earlier a5e source-bound x64 runs are `20261028T001713469Z`, `001823614Z` and
`001929887Z`; the final input lease is `20261028T002320880Z`. Live logs and raw
proofs remain in `.build/integration-20261028/` until final archival.

The historical SDK33c controls investigation uses recovered workflow40553171683,
not a substituted older cohort. All 72 same-side pairs are unstable: 292 of 1,656
finite metric cells exceed their unchanged bands, with 216 explicit zero-to-zero
ratios. Tiny clean preparation p95 values show timer/report quantization; position
and shared-load explanations remain confounded. This is an investigation result,
not a stable calibration, policy approval or current-source performance verdict.

Both draft PRs are published. RedTilde's current hosted jobs again refuse before
startup because of billing. SDK performance acceptance remains review-required
under the actual base's unqualified policy; no rebaseline or waiver is applied.

Remove only proved-obsolete branches/worktrees after preservation. Twenty-three
already-main-merged local branches and nine further merged DxUi worktrees/branches
are retired. The first four build trees preserve 44,918 files (7,288,443,634 bytes), with
verified source ZIPs and persistent ledgers under the approved
`C:\DxUi.Perf\qualification\integration-20261028\retired-worktree-evidence`.
Stale remote-tracking refs are pruned; no live remote branch has yet been removed.
Dirty primary work and active qualification/performance references stay protected.

The ninth retirement is the obsolete frame-phase checkout: all23,839 build files
(1,004,054,551 bytes) were moved within the approved evidence volume, and all10,809
source ZIP entries were byte-verified before removal. Its branch was main-merged.
Protected work was rechecked: all222 preserved copies, all20 current dirty source
files and the original user deletion remain unchanged. The newer SDK documentation
continuation does not change source/header/build/benchmark implementation from SDK086.

Both primary checkouts now use main. RedTilde's untracked review file conflicted
with a newly tracked upstream path: the fast-forward initially refused, its exact
bytes were safely put aside and then restored after integration. It is now a
modified tracked file rather than an untracked one; its SHA-256 remains unchanged.
The user's deleted example remains deleted. Neither local change was committed.
GitHub had already removed the two merged remote feature refs automatically;
explicit deletion requests therefore found no ref, and a fresh remote inventory
confirmed only main remains. Exact native/raw evidence and recovery maps remain
under the approved qualification archive. R10 and the hosted/manual gates stay WIP.
## Progress checklist

- [x] Added title-bar follow-up: connect nonclient input to canonical button hover/press/release/cancel handling, retain maximize hit testing for Snap, and validate neutral/red hover without taking desktop focus. At that checkpoint it did not resume the saved performance/CI queue; the user resumed that queue on 28 October.

- [x] Separate styling follow-up: user selected the first supplied image's Markdown styling. Implement shared heading separators, intrinsic tables, centered display objects and shaded quotes in an isolated SDK branch. The queue was still paused at that checkpoint and resumed on 28 October.
- [x] Debug and Release styling/caption qualification: adopt SDK `eb0f10b91bb8de19b0d7671cc7078fd0c1f5e767`; both configurations pass all 67 native cases and 32 capture/provenance pairs, including distinct hover pixels at 96/144/192 DPI. Source/history/focus and cancellation checks pass. New public presentation headers invalidate the earlier private PB-only V3 compatibility profile for this candidate; old proofs retain their original scope.
- [x] Complete all six standalone SDK and consumer build profiles for native SDK eb0f10b9 and consumer compiled-source identity 2166a79bc250d6042ad5698927eb0d3ac3e95a22e1525ff4d7934808590e0e87. Preserve exact producer/compiler/header/output identities in the archive.
- [x] Complete consumer x64 Debug, Release and ASan Full: each passes all 67 native cases and all 32 capture/provenance pairs, with crash/recovery probes and real ASan detection where applicable. All three DPIs distinguish rest, Close and Maximize pixels.
- [x] Complete standalone SDK x64 Debug, Release and ASan Full: each passes all nineteen noninteractive suites with zero capability skips, including the actual ASan detector. Preserve the earlier deadline failure and unchanged successful retries.
- [ ] Complete native ARM64, physical-input, performance/advice and release qualification before closing this follow-up. The queue-paused wording in this earlier checkpoint is historical; the user resumed the remaining work on 28 October as recorded at the top.

- [x] At the user's separate request after pausing, add the [complex example corpus](../../../docs/examples/README.md): 27 math sections (35 formulas), four flowcharts, two state machines, two class diagrams and an isolated fallback document. An SDK-local CPU probe linked only the existing exact33c Debug library; all 58 expectations pass (45 accepted, 13 rejected), with exact graph source retained and no partial rejected graphs. Preserve both the initial 55-case and final 58-case inputs/results privately. This is parser coverage, not native layout/raster or application qualification. Queue-paused wording records the earlier checkpoint; work resumed 28 October. Fixed S5/capture fixtures are unchanged.

## Styling and caption follow-up evidence — 27 October 2026 UTC

**Historical pre-resumption status (27 October):** the original queue was paused at this checkpoint. Exact SDK producer/pin is
`eb0f10b91bb8de19b0d7671cc7078fd0c1f5e767`. Its metadata-only successor a0f67fd3
changes no native source. Consumer native caption/capture correction is a0dfad8f;
the later C: qualification producer c4ee0459 preserves compiled-source identity
`2166a79bc250d6042ad5698927eb0d3ac3e95a22e1525ff4d7934808590e0e87`.

All six standalone SDK builds and all six consumer builds pass. Each x64 consumer
configuration passes 67 cases and 32 hidden captures, crash/recovery and resource
probes; ASan verifies actual heap-use-after-free detection. Each x64 SDK
configuration passes nineteen noninteractive suites with zero capability skips.
The app validators and SDK full tooling pass at their recorded producers.

Distinct consumer runtime source identities remain unchanged:
Debug `49c149e715e1f9fe41c87b061d48f2d852e91e41e2ed26e1741192d05241dc1a`,
Release `97c6f28c81f658b8955295cd7441c3631919bb431668e29f4f0e8bae14fa2e8e`,
and ASan `ea4a571538334b4da017157997b9a4f04ce0506c2e28ba928df536f3cdd2d5b5`.
The [qualification archive](../../../Measurements/MarkdownAndCaption_2026-10-27/README.md)
retains raw receipts, logs, build/header/compiler bindings and actual three-DPI
captures. Final metadata updates do not relabel those executions.

Actual pixels show the heading separators, intrinsic horizontal-rule tables,
centered objects, quote shading, red Close and neutral Maximize hover. Capture
stages before Present; prepared interaction snapshots preserve synthetic pointer
state without pumping ambient leave messages. No desktop takeover, foreground
transfer or cursor movement occurs. Native typography and routing retain their
own implementation; this is not a pixel-identical browser claim.

Preserve the earlier stale-readback captures and first corrected-readback Editor
failure. The first full SDK ASan run passes eighteen suites and misses the
unchanged twelve-second dense-source deadline while compilation is active.
An unchanged complete EditorControls retry and final full run subsequently pass
after compiler/linker work ends. No deadline/assertion was relaxed and no cause
or quiet-host acceptance is inferred from that sequence.

Preserve the source-drift-refused first Release attempt and capacity-stopped SDK
build/tooling runs. Updating WIP during a build invalidates its source guard;
checklists are now updated only between transactions. Current SDK Release /GL
defaults are reflected in the evaluated policy regression and normative build
specification. Independent audits verify the receipt hashes, native pin/compiler
bindings and metadata-only SDK change. Native ARM64 compiler/link/archive commands
use 14.52.37025; the restore's separate 14.53 detection is an x64 CMake-helper
triplet observation.

Automatic approval review rejected compiler-cache cleanup and then both the
user-authorized .Perf cache/fixture batch and a single inactive fixture removal
with "blocked by policy" before launch. Zero files were removed by those
commands; 405 candidates and 495 preserved records were checked at that time.
Later capacity/path changes are unattributed. The original D: cleanup record
became unavailable; the C: continuation explicitly labels its saved observation
as reconstructed, not recovered raw evidence. Remaining qualification used the
existing C: workspace. Never bypass the declined deletions.

Native ARM64, physical input, paired performance and release/hosted CI remain
open. The unpaired timing/memory banners cannot close the original performance
findings. Both WIP plans remain open; the original queue/STOPPED files retain
verified hashes and additional C: copies. No new CI run, push, merge or real-focus
suite was performed.
## P28 performance and hosted-toolchain repair — 27 October 2026

Current acceptance holds: the latest complete study retains two stable original-UI working-set findings (Compose +2.0457%, Split +2.4615%) and a third private-byte finding with unstable controls (+2.0952%); the private result is not dismissed. Compose command time is unresolved. The latest complete native producer is `9cf8b221`; later draft source needs fresh qualification. The icon-decoder scout is rejected without adoption. SDK compiler repair is merged and latest main CI passes; shared input fixes and explicit consumer adoption are still being qualified. Hosted RedTilde ARM64 remains open under the billing restriction.

- [x] Resume from merged app main `5b545e2204bdf613bd9ee5b8b4597b069caa7d9c` and SDK main `9291e259331c1dd376c028dcda8e70798765139f`; the consumer remains pinned to renderer `98c795b7`. P26 measurements remain historical source-bound evidence.
- [x] Identify the hosted SDK setup failure from the actual job: strict-mode access to unset `$LASTEXITCODE` occurs while the GUI Visual Studio bootstrapper is still extracting. Repair owned-process waiting and explicit exit-code handling, with delayed-child/exit/path regression controls and no local installation.
- [x] Retain fresh current-policy Release baselines before application optimization. Original-UI, starting-redesign and actual implementation PR18-base `07b232a7` source references receive the same reviewed current compiler policy and common harness, including Monitor startup support. The continuation PR base is app main `5b545e2`; its product runtime source/header equality is checked separately. Test-only diagnostics and the direct helper invocation-policy repair retain their own scope and fresh qualification, without inventing an A/B product optimization effect. Preserve earlier binaries, receipts, failed attempts and declarations; the earlier-policy executables cannot substitute for current-policy references.
- [x] Independently review the existing selection/resource diagnostics and select the already merged Release whole-program/link optimization policy as the first qualification target. Current guards already cover repeated analysis/mirror/caption work; measured formatter savings remain too small. Preserve production C++ bytes from `5b545e2`, native syntax/source/history, localization, mirrored Source, focus/UIA and resource bounds. Both historical sides already used MSVC 14.52.37025; the changed code-generation policy, rather than a compiler-version upgrade, requires new matched references. A smaller executable alone does not prove either flag resolved.
- [x] Preserve the first P28 x64 matrix at `96e026ee`: Debug and ASan Full pass, including actual ASan detection; Release fails `Window.DesignRefreshResponsiveLayoutAndDpi` at its combined foreground/hidden-window assertion. The receipt does not distinguish those predicates or attribute the change. Add failure-only predicate/owner-query diagnostics while keeping the exact assertion, and synchronize the current catalog documentation to 66 cases (Storage 11, Editor 46). The original failure remains unattributed; no primary A/A or A/B process has started.
- [x] Retain qualification 2's unsupported `-Suite Editor` parameter refusal before build/application launch. At source `7ea7ddc6`, independently verify qualification 3's three Full x64 profiles (66 cases and 23 capture pairs each), separate 46-case Release Editor run, real ASan detection, three ARM64 cross-builds and nine-stage authorized keyboard lease. These are source-bound observations, not native ARM64, performance or policy acceptance.
- [x] Diagnose qualification 3's material helper-policy mismatch: its direct lease MSBuild call rebuilds the policy-fingerprinted SDK Release archive with 41 compiler commands lacking explicit /GL and /Gy. Preserve the 59,718,832-byte non-GL archive (`5C643FD3...B5FF3C`), raw commands and complete independent audit. The already linked app EXE remains `7EFF3A36...07FA96`; this does not establish current archive-policy consistency or attribute the earlier DPI failure. Keep the permit absent and zero primary slots; authorized repair edits intentionally stop the waiting driver at its source guard, with the failed state retained.
- [x] Repair the direct helper invocation through receipt-pinned compiler/SDK/host/WPO arguments. Real MSBuild evaluation passes six lease and six pinned-SDK profiles across x64/ARM64 Debug, Release and ASan Debug, four omitted-policy non-GL counterfactuals and four toolset-override controls. Release selects GL; Debug/ASan retain non-GL, ASan instrumentation and exact MSVC 14.52.37025. Application C++/headers and the SDK pin remain unchanged. This is evaluation only: root review, canonical Release archive restoration and fresh final qualification/keyboard execution remain required before a new primary declaration.
- [x] Correct the independent review's helper casing defect: ValidateSet accepts `release`/`rElEaSe`, so the policy decision must be case-insensitive. Retain the evaluated lower-case failure before repair; eight mixed-case lease/archive controls now use actual restored dependency identities (compiler host x86 here), independently rechecked against canonical discovery, rather than application-evaluator host defaults. Existing canonical profiles and toolset controls remain covered. This focused correction performs no native build/input and closes no runtime/performance gate.
- [x] Complete exact-source qualification 4 at `9cf8b221`: six builds, three Full x64 profiles with 66 cases and 23 capture pairs each, separate 46-case Release Editor coverage, real ASan detection and the nine-stage/168-event authorized keyboard lease. Foreground/focus/pointer restore exactly. Both retained SDK archive/command snapshots stay byte-identical with all 41 units using /GL. Independent native replay passes 33,972 checks over 2,340 files; the [public native projection](../../../Measurements/RedesignCompletion_2026-10-27/p28-native-9cf8b221/README.md) retains the pre-primary scope and earlier failures. ARM64 remains cross-build only.
- [x] Declare and collect all 576 current-policy processes against original UI, starting redesign and actual implementation PR18 base: twelve blocks per view, 64 cycles, seed `20261027`, diagnostics OFF, fixed twelve-slot family and unchanged 5% timing/2% memory bands. All slots, overlap observations and findings remain intact; no pooling, exclusions or sequence rerolls. Collection ends at 11:06:42 UTC and the canonical numerical audit passes.
- [x] Independently replay and [archive the complete comparison](../../../Measurements/RedesignCompletion_2026-10-27/p28-policy-9cf8b221/README.md): canonical 25,138 checks, root 13,322 and separate reviewer 13,940. All 576 raw hashes, 864 effects, 1,728 controls, 72 rows and 936 CSV cells reconcile. Preserve the first replay's stale 25,120 exact-count refusal; twelve additional translation-unit checks and six release-policy identity checks explain the count under unchanged auditor source. Neither study rules nor native results change.
- [x] Complete the separately declared eight-slot frozen-binary memory diagnostic (Compose ABBA, Split BAAB; 64 cycles, diagnostics ON), with 1,147 independent checks. All final/post-memory counters stay constant within each slot. Both sides reserve the same 6,891,008 diagnostic bytes; all explicit default/CRT heap walks complete. Different format/brush counts and heap payloads are descriptive, not allocation ownership. Original fields absent from its source remain unavailable rather than zero. All observer brackets complete; none verifies quiet CPU. These slots are excluded from the primary family.
- [ ] Resolve the current original-UI flags: Compose working set **+2.0457%** and Split working set **+2.4615%** with stable controls, plus Compose private bytes **+2.0952%** with unstable controls. Compose command **+2.2192%** is unresolved with unstable controls. Starting-redesign and actual PR18-base A/B have zero regressions; across the six studies 36/72 controls remain unstable. Preserve the prior cohorts without inferring a causal cross-study gain. Complete a separately reviewed phase-bound memory census before selecting any unsupported owner hypothesis or new optimization.
- [x] Preserve SDK `631d8a95` run `40485094386`: all six real compiler setups/builds and eight consumer receipts succeed, including the measured ARM64 Release step beyond the old 15-minute bound. Two native Menu assertions fail in ARM64 Debug/Release, and strict toolchain-profile preflight refuses before benchmark samples. Four native profile jobs pass; downstream consumers do not erase failed Full tests. Retain both unattributed Menu failures, add narrow failure-only diagnostics, and fix the reproduced omission of library-project timestamp staging without weakening admission.
- [x] Integrate the SDK compiler repair from [PR119](https://github.com/RedSalamanders/DxUi/pull/119), head `767dffc5`, run `40497253396`. All six actual compiler setups, native builds and Full jobs pass, including all eight consumer checks and both ASan detections. Root rehashes the 31-file native seal; a separate raw-ZIP review verifies all 740 members, 505 completion streams, 130 suite receipts and 124 Menu cases per profile. Each Full profile has 1,902 cases with zero skips/failures. ARM64 Release consumer qualification takes 16m04s, beyond the old 15-minute limit and within the targeted 20-minute allowance; Windows tooling takes 15m37s. Actual paired preflight matches both complete toolchain identities (`AC9167C8...E258B`), with four staged project timestamps. The externally completed merge `e3447765` has exactly the reviewed `0eaba4ae` tree and parents `9291e259`/`767dffc5`; integration does not qualify performance. Local Release Menu also passes the focused four cases and all 124 cases with verified restoration. Current passes do not attribute the earlier ARM64 Menu failures or close the ARM64 initial-focus limit.
- [x] Replay the SDK paired collection separately from qualification: all 144 raw receipts, 78 metric rows, 936 block effects, 72 comparisons and 72 same-side controls reproduce. The three groups report zero regressions, but the measured base policy is unqualified, all three judge agreements are false, and 70/72 controls are unstable (249/1,872 drift observations). Verdict publication correctly remains inconclusive/policy-review-required. Preserve every report and reader refusal; no rerun, band change or waiver.
- [ ] Resolve the newer SDK main `e3447765` NativeTextInput failure in run `40509208565`: x64 Debug loses an independently focused TSF document before the retiring document is popped. Earlier two current-turn cases and Menu pass; the whole-queue counterfactual case fails. Preserve the complete failed artifact and all 82 streams. The tested PR producer `38d9b21e` remains a separate six-profile pass. Trace association/transition ordering without retaining an extra COM reference across retirement; use a pure controlled-COM regression for the shared restore predicate, retain third-document/newer-generation guards, and coordinate any real-input run through the authorized default-Start lease. No causal attribution or semantic fix is claimed yet.
- [x] Prepare the separately declared, phase-bound memory census and preserve its launch failures. Private source `4a635562` and helper attempt 12 compile and freeze successfully; 97 owned launcher checks preserve collision, reparse, cancellation and watchdog failures. Declaration v1 stays unexecuted; v2's failed nonce-absent smoke retains exit 1 and no report/phase. PowerShell 7.6.5/.NET 10 null-to-empty binding is independently reproduced in the parent and actual child; native removal and exact missing/empty/value restoration pass 18 checks. V3 passes all five app smoke boundaries. Fixed native slots 1–8 complete; slot 9 stops before app launch when the readiness marker's writer briefly denies reading. Preserve all eight observations and the observer-only failure, then repair bounded prelaunch publication handling and explicitly amend only the four unexecuted positions, as recorded below. These versions are not one unchanged launcher declaration. Render/upload omission is a causal diagnostic only and cannot become a shipped feature removal. Retain the original eight-slot diagnostic and all 576 primary reports.
- [x] Finish the explicitly amended branding suffix (9–12 only) after 17 real sharing/publication controls and the existing 97 launcher checks. Independent replay passes 1,592,910 checks over all twelve native observations while preserving all 169 original files and the failed/partial v3 disposition. Drawing omission consistently changes three D2D image pages (12 KiB), but process-memory savings do not repeat across Compose and Split; explicit heap payload differences are small and nonmonotonic. Do not remove branding or claim that hypothesis resolves the three flags. The EX-valid page sum is 4,156 KiB above the traditional process working-set counter in every slot; retain that unreconciled API-scope difference rather than treating it as an application allocation.
- [x] Compare the original UI and current implementation at the same owned phase: original `4822f14d` versus current `4a635562`, shared performance/hook source, helper 14, separate four-cycle original smoke and fixed eight-slot Compose ABBA/Split BAAB at 64 cycles. All complete; canonical replay checks 189 payload hashes and independent replay passes 2,351,424 checks. Original 48-byte versus current 152-byte activity and missing fields remain explicit. Fourteen common files are byte-identical; the sole full-hash-pinned Toolchain module difference is the non-invoked keyboard-argument function/export, with all existing AST statements/function bodies unchanged. No new dependency, rebaseline, pooling or acceptance is implied.
- [x] Retain both memory APIs' actual scope: helper 14 preserves every traditional address/raw flag plus exact derived per-page membership under the total 20-second budget. In all eight study slots, extended-valid totals exceed traditional membership by 4,188 KiB, entirely in private ranges, mostly with write-combining protection. The process-counter/list difference is a separate unreconciled 32 KiB, reproducing the older net 4,156 KiB gap. The four-cycle smoke has a distinct 28 KiB counter/list difference and is never pooled. These facts do not identify an allocator or change the primary counters. Current image residency grows 324/320 KiB (Compose/Split), font-cache mappings also grow, and explicit heap payload grows about 0.49/0.46 MB; retain per-slot values and unavailable file identities.
- [x] Test a feature-equivalent startup resource decoder as a private candidate. Pixel and device-replacement coverage pass, but the source-bound 32-process scout shows no repeatable material working-set saving. Reject production adoption and restore the canonical HICON extractor; preserve candidate commit `4855c6f2`, exact frozen EXE/PDB bytes and every attempted run. Retain useful native/scaled/default WIC coverage and foreground diagnostics. The two stable original-UI working-set findings remain open.
- [x] Preserve all six in-progress candidate/planning files with hashes, rebuild clean actual PR base `ad8f2ce2` canonically and freeze its Release EXE (`ED49C74D...6837FF86A`), PDB, compiler commands and original build receipt. Restore all six files byte-exact. This fresh base is separate from the earlier `9cf8b221` producer and no older runtime result is retagged.
- [x] Close the direct-resource experiment without adoption. No final matrix is required for the rejected prototype; fresh final qualification is still required for the retained canonical extractor, expanded pixel tests and proposed SDK input changes. Malformed-resource decoder tests are removed with their rejected implementation rather than becoming an unsupported public contract.
- [x] Preserve the first candidate Release Editor failure (45/46): the extended icon oracle required an opaque pixel at every scaled size. Keep that requirement for the seven native frames; scaled frames require visible ink and transparency, with size/count diagnostics and the unchanged independent byte-for-byte WIC comparisons. This test correction and candidate runtime behavior still need fresh execution.
- [x] Preserve the second candidate Release Editor result (44/46). The corrected native/scaled/default icon, repeated-paint and device-replacement case passes. Responsive-layout and outline no-activation assertions fail; the responsive case reports a different unowned foreground while the test window remains hidden, but does not distinguish a null initial foreground from a vanished window. Add initial-boundary/null/ownership diagnostics to both cases without changing assertions or deadlines. Keep cause unattributed and rerun final qualification after this environment investigation; no failed attempt is pooled into a pass. Complete independent tooling validation passes on the second attempt's source.
- [x] Rebuild the candidate with those failure diagnostics and run Release Editor in an isolated local runtime window: all 46 cases pass, including the seven native and three scaled icon sizes, default selection and device replacement. Preserve run `20261027T150911291Z-57272-240fbe1eab3c4f958219a081fe72dc93` separately from both failed attempts. This focused source-bound pass is not the final six-build/native/performance qualification.
- [x] Review and merge SDK [evidence PR120](https://github.com/RedSalamanders/DxUi/pull/120) after all applicable checks pass and native scope is positively skipped. Remote merge `5d983536` has exactly reviewed head `a2a239a8`'s tree (`25aaaf54`) and parents `e3447765`/`a2a239a8`; the owned docs checkout is clean and changelog fold/check passes. This publishes the compiler-repair evidence and preserves all policy/noise/native-input limits.
- [x] Confirm a separate SDK input contract gap with the unchanged helper: non-null previous association followed by null TSF focus loses a retained foreign document. At proposed SDK `21cda758`, 12 controlled Foundation cases (including 64 reference-balanced replacement cycles), the original three focused NativeTextInput cases and the full NativeTextInput suite pass with no skips; foreground/focus/pointer restoration is verified. The exact older hosted failure remains unattributed. The proposed fix is not yet the consumer pin or a six-profile SDK verdict.
- [ ] Investigate a second shared input path before final SDK adoption: `SetFocusControl`'s non-text branch lacks the hidden/zero-size native-focus guard already present for text input. Require a controlled owned-window negative, the minimal guard and positive visible-host coverage without weakening assertions. This source finding does not attribute the earlier RedTilde foreground changes.
- [x] Execute the controlled SDK non-text negative at `0a9b20f2`: the canonical Debug build passes with zero warnings/errors, and one native run proves hidden/nonzero plus visible/zero-height Button and Tree targets steal native focus (four valid inactive observations). All four visible-host/explicit-logical-only controls pass, and foreground/focus/pointer restoration is verified. The zero-width fixture is clamped to 120 pixels and fails setup; preserve that limitation rather than claim width coverage. Correct fixture geometry and qualify the minimal shared guard before adoption.
- [x] Review further retained-allocation and compiler candidates before changing production. Hidden primary/mirror/equation editor inventory predates the redesign; current hidden rich-editor preparation is already avoided. New line-index/hidden Files objects are bounded for S5, with no measured material owner. The actual Release linker uses `/DEBUG /OPT:REF /LTCG` and no ICF. Do not enable ICF as a speculative memory fix: the pinned SDK uses destructor-function addresses as payload type discriminators, so folding can invalidate that assumption. No new optimization, savings estimate or causal attribution follows from these read-only audits.
- [x] Preserve the second SDK negative at `cde6d909` without width attribution: the minimum-track override still yields 32-pixel client width. Two independent hidden Win32 probes (twelve rows each, including the actual extended style) verify that `SWP_NOSENDCHANGING` reaches real zero-width/height without focus changes. Remove the failed hook from final source. Root reviews all thirteen files at clean public SDK `33c3086c` (parent main `5d983536`): only the TSF null-focus predicate and the non-text visible/positive-size guard change runtime. Five source validators, pinned formatting and affected-plan explanation pass; fixed Debug build and fresh Foundation/native-input qualification remain pending.
- [x] Complete full RedTilde source/tooling validation after restoring canonical extraction and replacing public raw receipt names with sanitized slot aliases. All validators pass; root alias/projection replay checks 850 predicates with unchanged native hashes and scalar values. Preserve the earlier 818-check stage review separately. Final consumer native qualification still awaits the explicit SDK pin and final performance-harness binding.
- [x] Independently review exact public SDK `33c3086c` and propose it as the consumer pin in draft source. Fresh local Debug Foundation controls pass 12/12 and full NativeTextInput passes 173/173 with zero skips; all ten new geometry/focus observations pass and desktop restoration is verified. SDK [draft PR121](https://github.com/RedSalamanders/DxUi/pull/121) is published; run `40553171683` has started all six native profiles, with outcomes and actual checkout producer still pending. The new lock is an explicit candidate, not completed adoption: all consumer builds, native/input/ASan/capture and fresh comparisons remain required. Keep source `98c795b7` freezes and their historical observations unchanged.
- [ ] Qualify and explicitly adopt the final canonical SDK input fixes. Current main also changes the public ProgressBar class; preserve exact dependency ownership and add a new reviewed header/ABI profile if needed, retaining separate exact SDK4077 and SDK98 baseline identities. Keep existing freezes untouched and create fresh common-harness references before any final comparison; never repin or relabel the original-UI baseline.
- [x] Publish the pixel-qualified candidate `4855c6f2` in draft [PR22](https://github.com/RedSalamanders/RedTilde/pull/22). Fresh run `40535839283` again starts zero native-scope/tooling steps under the explicit payment/spending-limit annotation; native jobs are skipped. Hosted RedTilde ARM64 remains **OPEN**. The draft is not merge-ready or performance-qualified.
- [x] Collect and independently replay the separate 32-process same-pin icon scout against actual base `ad8f2ce2`: two blocks per view, 64 cycles, seed `20261027`, unchanged metrics/bands and diagnostics OFF. Preserve the pre-start ordering-only amendment and all AA/AB slots. Canonical audit passes 1,993 checks; independent replay matches 288 raw-derived scalars, 24 rows and ten frozen artifacts. Compose working set +0.4216% increases in both blocks; Split -0.2454% changes direction. Private-memory controls are unstable, including AA. No repeatable material memory saving, owner or acceptance is established. Report SHA `FB9C6BDB19FE31B71CCFBB97DD4657730B9C9F3A5CE5A05567D3C94EF227C845`; preserve the original complete comparison and all flags.
- [x] Archive the [rejected decoder scout](../../../Measurements/RedesignCompletion_2026-10-27/icon-scout-4855c6f2/README.md). Root independently checks 850 projection/binding predicates, all 288 raw-derived scalars, 24 canonical rows and ten frozen hashes. Preserve the first reader schema-mapping refusal privately. Canonical production extraction is restored byte-exact to actual base; new scaled/default independent WIC tests and unchanged foreground assertions remain for final qualification.
- [x] Archive the [source-reviewed resident census projection](../../../Measurements/RedesignCompletion_2026-10-27/p28-resident-census-4a635562/README.md): helper14 for eight original/current slots plus separate smoke, helper12 for both branding controller parts. Root checks 735 shared table cells; independent review checks all 312 projected aliases against 417 parent files, 14 helper artifacts, 21 native slot bindings and 45 report files. Correct draft metadata defects before publication and retain both rejected staging copies privately. No owner, saving, quiet-host or acceptance claim is added.
- [x] Preserve SDK `21cda758` sources, all executed binary/PDB/dependency bytes and all three local test runs before the next controlled experiment. Relocate exactly 113 idle diagnostic cache files (549,986,998 bytes) through verified recoverable copies; ReFS physical free space does not improve, with cause unproven. Keep frozen evidence untouched and move the next focused build to an existing NTFS checkout with verified inherited compression and an owned-process capacity guard. No unrelated source or application is removed.
- [x] Recover build capacity by losslessly compressing 374 closed object/compiler-PDB caches (1,813,885,292 bytes) and 19 canonical PDB/library caches (1,290,717,436 bytes). Verify bytes, lengths and LastWrite; record compression-attribute changes and retain the first summary-writer refusal separately. Source, executables and frozen capture copies remain unchanged. Preserve the SDK's capacity-stopped build and subsequent test-fixture compile failure; correct the protected-method query and add the pre-skip owned-focus assertion before the next attempt. The new regression has not yet executed.
- [x] Check latest SDK main `5d983536`, run `40528944780`: workflow metadata reports success for all six native profiles and consumer checks. This is a distinct producer without an independent artifact replay yet; it does not attribute the earlier `e3447765` failure or qualify the unmerged hidden-control test/fix.
- [x] Recheck draft PR22 at `a25bc635`: run `40545719371` again fails native-scope/tooling before either starts a step, with explicit payment/spending-limit annotations; the native matrix is skipped. Archive both original annotations and the current SDK main job summary separately. Hosted RedTilde ARM64 remains **OPEN**; SDK main success does not substitute for that consumer gate.
- [x] Verify RedTilde's externally completed PR21 merge `ad8f2ce2`: its tree exactly equals final head `44415f78`, which adds the bounded hosted-setup implementation after the qualified `472ee60a` checkpoint. Latest PR run `40510015445` starts zero native-scope/tooling steps under the explicit account-payment/spending-limit annotation; the native matrix is skipped. Main has no automatic push workflow. Preserve the two in-progress planning/spec files byte-exact while continuing from merged main on `ej/performance-memory-20261027`. Hosted RedTilde ARM64 remains **OPEN**, as requested. Independent SDK policy, quiet-host, physical-input and broader platform holds also remain open. Keep this plan in WIP until its actual acceptance obligations close.

## P27 remote-main integration — historical checkpoint (27 October 2026)

- [x] Integrate the final evidence branch with remote main `b8bf708ab096720686a7fb4c28afeed03ff13f91`, preserving the already merged RedMonitor, MSVC 14.52 and atomic-save changes. Evidence PR [#20](https://github.com/RedSalamanders/RedTilde/pull/20) adds documentation and sealed evidence relative to that main; it introduces no native source, dependency-pin or build-policy change.
- [x] Preserve the independently source-bound P26 packets and metadata continuation. Their native and performance executions still belong to app `eed2abe9` / SDK `98c795b7`. The incoming main has different native/build inputs, so those earlier runs do not qualify the integration tree and are not retagged. The separate MSVC/RedMonitor qualification records retain their own scope.
- [ ] Resolve the two current original-UI performance flags: Compose command **+8.64%** with unstable controls and Split working set **+2.027%** with stable controls. The actual PR18-base comparison has zero regressions; this does not close the original-UI findings or SDK policy acceptance.
- [ ] Close the independently owned performance-policy, quiet-host, physical-input and native hosted RedTilde ARM64 gates. Hosted run `40461044026` at evidence head `81aec56c` starts no test steps under the account payment/spending restriction. Keep this plan in WIP; documentation publication is not performance or platform acceptance.

## P26 qualification — historical; superseded by P27 integration status (27 October 2026)

This checkpoint supersedes P25 status. Runtime evidence still belongs to app `eed2abe9` / SDK `98c795b7`; metadata revisions do not retag those executions.

- [x] Complete and independently verify combined SDK `7020121dce2fc36d6c3b1219a17ea2d20e5a6983` native CI. All six x64/ARM64 Debug, Release and ASan Debug jobs pass. The six provider ZIPs match their digests and lengths; a separate Python replay passes 8,307 checks over 732 actual files, 505 completion-manifest data streams and 130 suite reports (128 ordinary plus two expected ASan detectors). Each profile runs 124 individual Menu cases; the 125th raw START/DONE marker is the suite banner. Hosted ARM64 initial keyboard focus is unavailable in three receipts, so those do not establish full physical-input restoration.
- [x] Archive and independently replay the completed SDK paired comparison against actual base `4a05f62c`: 144 raw reports, 78 metric rows, 936 block effects and 1,872 control observations. All three scenario sets have zero regressed metrics. Controls are unstable in 69/72 comparisons (229 drifted observations); MultilineGrid's producer legacy/migrated verdict agreement is false. Verdict publication and aggregate CI fail because the base policy is independently unqualified. **Performance acceptance remains OPEN.**
- [x] Complete the separate V2 and V3 selection diagnostics, preserving their instrumentation-only sources and binaries. Fresh resource resolution remains part of every localized lookup. The standalone formatter experiment passes its differential oracle and all 168 sample rows have matching strategy checksums; its custom formatter saves only about 30–50 ns in the tested ordinary patterns. Do not adopt the extra parser or infer an app-path gain from this private microbenchmark. Its build succeeds; a separate audit preserves and explains the script's incorrect command-log postcondition without relabeling the failed receipt.
- [x] Collect the fixed eight-slot resident-memory diagnostic on the unchanged original/candidate binaries (Compose ABBA, Split BAAB; 64 cycles, diagnostics OFF). All eight native slots and the final source/pin/input gate pass. A tooling scope failure stopped the earlier attempt before any native or observer launch; preserve it separately. This diagnostic is excluded from the primary comparison.
- [x] Complete independent resident CSV/JSONL replay: all eight slots, 297 snapshots and 160,882 region rows pass identity, hash, page arithmetic, type-total and budget checks. The first pure summary writer fails after collection; retain that failure separately from the successful Python replay. Near-terminal Split main-image membership is 486 versus 547 pages, MEM_PRIVATE 6,903.5 versus 7,030 pages, and MEM_MAPPED 843.5 versus 902.5 pages (two slots per side, 4,096-byte pages). These are descriptive point-in-time memberships with unknown phase, not the primary snapshot or an allocation-owner attribution. Mapping lookup failures remain explicit despite zero mapping-cache omissions.
- [x] Preserve the six app builds, three x64 Full results (65 cases and 23 capture/provenance pairs each), actual ASan detection, authorized app keyboard lease and exact-head SDK702 Debug Menu lease described in P25. No later runtime run or performance reroll is claimed.
- [ ] Resolve the two original-UI flags retained by the fresh primary comparison: Compose command **+8.64%** with unstable controls; Split working set **+2.027%** with stable controls. Actual PR18-base A/B has zero regressions. The older seven/five-flag and approximately 11% private-memory snapshots are historical, not the current verdict. Keep policy and quiet-host limitations explicit.
- [x] Publish the reviewed [SDK native/paired/control packet](../../../Measurements/RedesignCompletion_2026-10-27/sdk-7020121d/README.md) and [diagnostic follow-up](../../../Measurements/RedesignCompletion_2026-10-27/diagnostics-eed2abe9/README.md), synchronize qualification documentation and fold the changelog fragments. Keep all producer identities and historical failures separate.
- [x] Complete the authorized implementation merges: SDK PRs #113–#116 merge as `6382b30646211be825c0275672b4986e65dc2ceb`; RedTilde PR #18 merges as `74c3eb6e227b26f4428cb51411864d7d059232f1`. Both remote trees match the reviewed heads. The consumer pin remains the reachable renderer98 ancestor; the merges do not close performance, policy or hosted-platform holds. Keep this plan in WIP.
- [x] Refresh hosted RedTilde status at metadata head `2b79f3ea1bd8860e7bd5d241d1a4420fbd0fd571`: run `40454250131` starts zero test steps under the explicit account payment/spending restriction. Hosted RedTilde ARM64 remains **OPEN**; SDK native ARM64 and local app cross-builds do not close it.

The user reports remote control during the earlier one-pixel Menu restoration incident. This is context only; it does not attribute the writer or cause. Preserve the original failed lease and the later exact-restoration runs separately.

## P25 qualification — historical; superseded by P26 (27 October 2026)

This checkpoint supersedes the progress status in P24 and earlier snapshots; their exact sources, failures and evidence remain historical records.

- [x] Qualify the Editor observer repair at app `eed2abe9885c57f44ab192155b341582782020f9` / SDK `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`: all six canonical builds pass; each x64 Full profile passes 65 cases (46 Editor) and 23 capture/provenance pairs, including actual x64 ASan detection. ARM64 remains cross-build only. A separate 21-check audit binds the actual artifacts, compiler/dependency identities and raw case partitions. The historical `798fcbe` failure remains unattributed; the controlled sharing discriminator demonstrates a possible mechanism only.
- [x] Complete the declared fresh six-study comparison: 576 processes, 12 blocks/view, 64 cycles, seed `20261027`, diagnostics OFF, all nine common inputs and equal public-header closure. The canonical audit passes 25,120 checks and the separate raw Python replay passes 13,322 checks. All 576 raw hashes, 72 rows, 864 effects and 1,728 control drifts are retained in the [eed2abe9 packet](../../../Measurements/RedesignCompletion_2026-10-27/performance-eed2abe9/README.md). The actual PR18 base is `07b232a7`, not either historical reference overlay.
- [ ] Resolve the two current original-UI flags: Compose command **+8.64%**, unstable controls; Split working set **+2.027%**, stable controls. Split CPU is -0.575% and unresolved in this cohort; the older +11.5179% finding is not reproduced. Starting-redesign A/B has four memory improvements and one CPU improvement; actual PR18-base A/B has zero regressions and Split CPU -9.441% with unstable controls. Across six studies 36/72 controls are unstable and no quiet CPU window is verified. No causal gain, flag waiver or performance-policy acceptance is claimed.
- [x] Complete a separate 16-process diagnostics-enabled localization on the same exact original-UI/candidate binaries: canonical replay passes 4,249,182 checks; 93 observer samples have complete coverage (maximum gap 1.10945 seconds). Inclusive selection/status presentation is the main observed extra command work. Explicit default/CRT heap aggregates and bounded app/cache counts do not identify allocation ownership; diagnostic storage is included in memory. This study is excluded from primary acceptance and does not remove either flag.
- [x] Repeat the authorized keyboard lease at app `eed2abe9`: nine stages and 168 real events pass, initial keyboard focus is recorded, and foreground/focus/pointer are unchanged after restoration. The separate SDK `f4827792` Debug Menu run passes **124 individual cases**; its 125 START/DONE markers include the suite banner. Focus and pointer restoration are exact. Preserve the earlier failed leases separately and do not retag this run as SDK702.
- [x] Combined SDK `7020121dce2fc36d6c3b1219a17ea2d20e5a6983` passes its local canonical Debug rebuild and the separate full Menu lease: 124 individual cases, zero skips/failures, recorded initial focus and unchanged foreground/focus/pointer. Run `20261027T042058664Z-44668-d97eeb91d3f94b179184e423af83c0d4` is independently source-bound; the earlier f482 receipt is not retagged. Before replacing outputs, all 32 old preservation-manifest entries matched the actual retained archive.
- [ ] Finish combined SDK702 CI and provider-artifact review. Production source, public headers and production build inputs match renderer98; fixtures/tooling differ. Formatting, validation, tooling and native-scope pass; all six Full native qualification steps pass, while three ARM64 consumer steps and the paired job are still pending at this snapshot. Passing SDK steps do not qualify hosted RedTilde ARM64 or performance policy.
- [x] Publish the source-bound [native matrix](../../../Measurements/RedesignCompletion_2026-10-27/qualification-eed2abe9/README.md), [keyboard lease](../../../Measurements/RedesignCompletion_2026-10-27/input-eed2abe9/README.md), and [Editor persistence discriminator](../../../Measurements/RedesignCompletion_2026-10-27/editor-persistence-eed2abe9/README.md) as sanitized, sealed projections. Actual retained payloads/native files remain separate; no runtime receipt is relabeled as a metadata revision.
- [ ] Publish SDK control-investigation evidence, refresh metadata receipts without retagging runtime runs, synchronize authoritative documentation/changelogs, then push and merge reviewed work. Keep this plan in WIP while performance/policy gates remain open.
- [ ] Hosted RedTilde ARM64 remains **OPEN**: run `40439267400` at `eed2abe9` starts no test steps under the explicit account payment/spending restriction. Local cross-builds and SDK ARM64 results do not close it.

## P24 qualification — historical; superseded by P25 (27 October 2026)

- [x] App798fcbe / SDK98 passes all three x64 builds and Release/ASan Full (65 behavioral cases and 23 captures each, plus actual ASan detection). Debug fails only the nested Source persisted-view check (45/46 Editor). Preserve this failed matrix; ARM64 and performance collection did not start. All seven candidate x64 native files are retained and hash-verified.
- [x] A controlled reader using the existing sharing mode reproduces writer failure `0x80070020`; releasing it and resubmitting succeeds. Twenty ordinary scenarios pass, so the historical Editor failure remains unattributed. Independent source review confirms the nested request submits Source and supersedes the outer Split request.
- [ ] Qualify the observer repair: await the writer's existing two-second idle/success boundary before one settings read. Keep exact Source/Split, history and supersession assertions. Production storage policy is unchanged. Repeat the required final matrix and declared comparisons against actual PR18 base `07b232a7`.
- [x] SDK AEB's full Tooling-to-SkipBuild regression passes: receipt4867CD26 is accepted, nine environment inputs match, and Foundation exits 0 with zero skips. The CMake-dependent optional fixture remains unevaluated locally; unpaired performance is not acceptance. Older local819 PE/PDB bytes were not retained; measured baselines are preserved.
- [x] The corrected UTC join covers 144 SDK A/A receipts and confirms broad instability without attribution. Independent recalculation verifies saved scalar results. The clean prepare statistic spans 5–6 QPC ticks; its 5% band is below one tick. Late memory drift rises without a corresponding aggregate host-CPU increase. No bands or policy flags change.
- [ ] SDK pointer-readback fixture qualification, remaining hosted renderer jobs, final keyboard/menu leases and performance policy remain open. Latest RedTilde run40434727892 starts no steps under the explicit payment/spending restriction; hosted RedTilde ARM64 remains OPEN. Keep this plan in WIP.

## P23 renderer diagnostic — historical; superseded by P25 (27 October 2026)

- [x] PR17 merge/publication completed at RedTilde `07b232a7ac878cebebddef8631251aeb6358afed` (tree `06f69df5563cecaee7bb2e200b380f1ce827ff10`). The next PR base freeze remains bound to that app tree, executable `68D8799E76F8C0056AC7897FEFD8F2518C61CEBD3C04F38E9FEF4F1B45E190E1`, compiled-source identity `633302c45ac8da29e7bea24cf7909dc70d8aad1665ebe618252dd31f72e0d8f3`, and runtime pin SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`.
- [x] A separate 16-process source-ink renderer diagnostic compares app `a29c9f84`/SDK `4077f57d` with isolated candidate app `1fb2fbbc`/SDK `bc2ac1d5`. All 12 metric rows are unresolved. Split owner CPU cycles are -7.2736445309% descriptively, Split command time +1.45859607%, and Split private bytes +1.45196991%; two blocks/view and control diagnostics enabled do not support causal attribution or acceptance. The 4,263,527-check independent integrity/numerical replay and observer coverage are archived in the [separate descriptive packet](../../../Measurements/RedesignCompletion_2026-10-27/source-ink-descriptive-1fb2fbbc/README.md). Its 58-row public dependency-header closure is unchanged; the candidate's additional renderer header is private implementation detail.
- [ ] This diagnostic is not the 576-process comparison or a fresh actual-base/full comparison. The original-UI Split CPU finding remains +11.5179672254146%. The diagnostic candidate is not adopted in the published production source: its baseline and the frozen PR17 base use SDK `4077f57d`, while its measured candidate arm uses SDK `bc2ac1d5`. The current working-tree lock separately points to proposed SDK `98c795b7` for PR114; that candidate pin is not part of this study or a qualified production adoption. Final renderer SDK/consumer matrix and fresh actual-base/full comparisons remain pending. Performance policy, input qualification, and hosted RedTilde ARM64 remain OPEN.
- [x] Before the proposed SDK98 adoption, preserve actual copies of all 14 qualified native files and six build receipts, with matching SHA-256 and sizes. Keep the frozen actual PR base at `07b232a7` and all prior performance evidence unchanged; no runtime result is retagged.
- [ ] Proposed SDK `98c795b765a10d1805ae0a7c35a8ade535fb7fd6` is frozen in draft [DxUi PR114](https://github.com/RedSalamanders/DxUi/pull/114). Consumer source/dependency validation, formatting, and complete tooling validation pass; all six consumer builds and three x64 Full runs remain pending. The first ARM64 dependency restore failed extracting CMake because the disk filled; retain that failure and retry after byte-preserving compression of idle build artifacts.
- [ ] Latest hosted RedTilde CI run `40425848035` at `23cbe67` started zero steps under the payment/spending restriction. PR113 environment-fix focused checks passed 40 cases and hosted x64 and ARM64 Release passed; ARM64 Debug failed immediate physical cursor readback after an accepted move, with later convergence and successful lease restoration. The writer/timing cause remains unproven; a separate bounded fixture-readback investigation retains exact menu/restoration assertions. ARM64 ASan is pending. The single full-Tooling positive rerun is still underway on the shorter SDK workspace; do not treat the focused pass as full Tooling qualification.

## P22 a29 checkpoint — historical; superseded by P23 (26 October 2026)

- [x] Fresh six-study comparison and independent checks complete at RedTilde a29c9f84 / DxUi 4077f57d: 576 slots, 72 rows, 25,202 canonical numerical checks, and separate 3,824-check root-derived projection/integrity replay. Original UI A/B has one resolved Split owner CPU finding (+11.5179672254146%); Compose command (+3.30%) and Split working set (+1.94%) remain unresolved. Starting-redesign A/B has four memory improvements; actual PR #16-base and all A/A comparisons have zero findings. 36/72 controls are unstable; no quiet CPU window was verified. No causal gain or policy acceptance is claimed. See [current performance packet](../../../Measurements/RedesignCompletion_2026-10-26/performance-a29c9f84/README.md). Keep the earlier 09f cohort distinct.
- [x] Current local native matrix: the three x64 profiles each pass 65 behavioral cases/23 captures; x64 ASan detects the expected use-after-free; ARM64 is cross-build only. The separate keyboard run passes 9 stages/168 events; focus query was null and restoration remains unverified, while foreground was restored and pointer unchanged. One x64 Debug native run at SDK source 6cb678fa passed five filtered tests with zero skips; initial focus capture was valid and restoration unchanged. At SDK head 8199566d635751a20a6e916e73410fb6cad317cc, Debug build and strict -SkipTooling gate passed. One Debug Menu run passed 124 cases with zero skips in 95.1 seconds, but outer qualification failed because its target window was absent during restoration checks; pointer unchanged. A second Debug attempt completed 89 cases before a pointer-alignment test failed after 85.4 seconds; foreground and keyboard focus were unchanged and pointer restored. These separate attempts do not combine into a pass; the pointer writer is unattributed and remote activity is under investigation. PR112 merged at 47fe389680a6c224ce2b76cc1e5c0af0c414a11f with exact tested tree 122b53a258972175085effa950a5ecc8f2fd51ba. Hosted run 40413609067 passed all 12 provider checks, including all six native profiles; seven provider archives were digest/size checked and the independent replay passed 1,293 checks (126 standard-scope reports, two ASan detectors and two additional Release reports). The paired job was skipped, so this is not performance qualification. Focused CWD tooling passes 48/48. Combined Tooling plus -SkipBuild refusal remains unexplained; full input qualification remains OPEN. See [native packet](../../../Measurements/RedesignCompletion_2026-10-26/qualification-a29c9f84/README.md) and its [Measurements-only metadata continuation](../../../Measurements/RedesignCompletion_2026-10-26/qualification-a29c9f84/metadata-continuation-pending.json).
- [x] SDK PR108/109 were previously merged at 091214d217ee82b1b8a29af4a24e94b6d022ae97 with tree ec4f10702055928c8f99c3b872ea4019d6f84aed, matching earlier SDK CI source 6cb678fabcfa95e974834234cffbc431640e8d43. That hosted run passed six native jobs; paired results showed zero set-level regressions across three scenes but remained inconclusive because the base policy was unqualified and 70/72 controls were unstable. The later PR112 merge and qualification snapshot are recorded above. RedTilde continues to pin 4077f57d; SDK CI does not qualify the consumer or close performance. RedTilde hosted run 40368290271 starts zero steps under the payment/spending restriction; hosted RT ARM64 stays OPEN.
- [ ] Current performance and qualification acceptance remain OPEN. Private source-ink and 32-range experiments remain unadopted and unpublished; production code and the SDK pin are unchanged. Source review records 693 source units and 694 edited units; 41 CRLF pairs produce 652 display units before edit and 653 after edit, with 26 ranges. The private S5 trace recorded CALL_LINKS 653/26, SPAN_SUMMARY 26/26/653, SOURCE_BREAK_NO_PAINT 25/1, and REJECT_FONT_NOT_MONOSPACED 2; preflight rejected and canonical fallback ran without BeginPaint or Drawn. The bounded 32-range path is therefore ineligible for this S5 case. The prototype remains unadopted; its focused raster check passed in 0.341 seconds, not canonical qualification, and no gain is claimed.
- [ ] Focus remains unverified for the separate keyboard run. Preserve the original focus query result of null; foreground restoration and unchanged pointer are observed, not proof of full desktop restoration.
- [ ] Latest hosted RedTilde run 40368290271 starts zero steps because of the payment/spending restriction. Hosted RedTilde ARM64 remains OPEN.

### Earlier P22 snapshots (historical; superseded by the current checkpoint above)
- [ ] P22 final qualification remains OPEN; the current a29 checkpoint above supersedes the prior status snapshots. Performance policy, hosted RedTilde ARM64 and focus verification remain separate gates.
- [x] P22 documentation continuation: all six canonical builds at `0f71482c` preserve the exact `45c2895` native artifact bytes and compiled-source map `633302c4`. No new native tests ran and the earlier runtime receipts were not retagged. Publish the separate metadata-continuation proof with the original packet.
- [x] P22 local Menu diagnostic checkpoint: SDK `18e1da1c` builds in Debug and passes the focused hover case and all 124 Menu cases with zero skips and verified desktop restoration. Its bounded diagnostics record physical alignment, clipping and live-popup DPI without retries or weakened assertions. The earlier cursor/alignment failures are retained and remain unattributed.
- [ ] P22 SDK CI: prior run `40351518044` at `8ac9ff63` passes five native profiles; ARM64 Debug fails the second sibling-with-children hover-target assertion, with successful foreground/pointer restoration. Its paired step later cancels and the verdict step is skipped; retain the partial artifact without a numerical verdict. Published source `d849177b` shares one exact alignment helper between both hover cases and adds per-leg diagnostics. Current run `40366949236` is in progress; format and native-scope pass, while matching native qualification remains pending. This is separate from the earlier cursor-restoration failure.
- [ ] P22 current-source calibration: the reviewed 18-case auditor binds SDK `4077f57d`, the fixed judge/assignment hashes, three scenes, 12 blocks and seed `20261027`. The first attempt stops before build/measurement on Git filename length; its logs and observer are retained. The unchanged full 144-process study continues in a shorter repository-local output with a process-only long-path setting. Policy remains unqualified; collection cannot approve itself.
- [x] P22 source-ink applicability decision: SDK `2532ab65` passes the narrow English direct-path/pixel oracle and mixed-bidi fallback, and both isolated consumers pass 46/46 Editor checks. The fixed 16-process A/B records Split owner CPU -7.15%, working set -0.42% and private bytes +0.43% descriptively, with no resolved row. Independent source/receipt review proves the actual S5 Source draw has one 693-unit layout and 26 color ranges; the eight-range prototype rejects it before painting. Do not attribute the CPU movement to the renderer or spend a full A/B on this ineligible path. The unpublished prototype is excluded from shipping; its failed attempts and unexecuted expanded font/DPI sweep remain retained.
- [x] P22 hosted RedTilde snapshot: PR17 run `40349750074` at `0f71482c` starts zero native-scope/tooling steps under the explicit payment/spending restriction; the native matrix is skipped. This is admission failure, with no executed hosted application test result. Hosted RedTilde ARM64 remains OPEN as requested.

### Earlier checkpoints (historical; P22 owns the current closeout)

- [x] P21 implementation publication: [PR16](https://github.com/RedSalamanders/RedTilde/pull/16) merges as `20c3ab6f` with the exact reviewed `45c2895` tree (`b9fcb074`). This is the qualified implementation checkpoint; the performance flags and hosted ARM64 remain OPEN. No plan is moved to Done.
- [ ] P21 shared follow-up: DxUi PR108 source `9675dcf4` combines the physical-hover fixture, exact cursor-failure diagnostics and PR109 pending-Update accounting. All local validators passed before integration; merged specs and the unchanged CI contract digest revalidate. The first integrated build exposed a regression placed before its fixture declaration; preserve that compile failure and move the unchanged body after the fixture. Fresh local Debug ordinary/input checks and matching CI are running; no green claim is made yet.
- [ ] P21 source-ink prototype: the isolated renderer compiles, and public raster comparisons complete, but the direct-path assertion rejects the positioned mixed-bidi fixture. Preserve the failure and use callback diagnostics to identify the unsupported condition. The prototype is unpublished and unadopted; pixel parity, direct-path admission and measured gain remain required before any shipping change.
- [x] P21 fresh native checkpoint: app 45c2895bd5a298550fe88cb9f905048497be94a7 / SDK 4077f57df8207eac6c39c43437feb45b19d23cf8 passes six local builds, three x64 Full profiles (65 behavioral cases, 46 Editor, 23 captures each), real x64 ASan detection and the separate 9-stage/168-event keyboard run. ARM64 is cross-build only. The compiled-source map is independently bound as 633302c45ac8da29e7bea24cf7909dc70d8aad1665ebe618252dd31f72e0d8f3; .gitattributes and two PowerShell files are conservatively included, with no C++ edits. Hosted run 40341402876 starts zero steps under the payment/spending restriction. Three original-UI performance flags, SDK policy acceptance, cursor lease and hosted ARM64 remain OPEN. See [45c native packet](../../../Measurements/RedesignCompletion_2026-10-26/qualification-30fb0c2/final-45c2895/README.md).
- [x] P20 native checkpoint: app `30fb0c2` / SDK4077 passes all six canonical builds, all three x64 Full profiles (65 behavioral cases, including 46 Editor, and 23 captures each), real ASan detection and 9 keyboard stages/168 events with unchanged desktop restoration. ARM64 remains cross-build only. Hosted run `40320880695` starts zero steps under the payment/spending restriction; hosted application ARM64 stays OPEN. The matrix Release artifact remains bound to its original binary; the separately restored colored Release build has its own identity and receipts. The three original-UI performance flags, SDK policy acceptance and cursor lease remain open. See the [native packet](../../../Measurements/RedesignCompletion_2026-10-26/qualification-30fb0c2/README.md).
- [x] P20 coloring attribution: a local one-line variant `135b48ff` disables only source coloring, retaining parsing and immutable syntax sharing. It fails exactly the predicted color-contract case (45/46 Editor pass). The fixed 16-process diagnostic records Split owner CPU -10.72% and completed-render time -13.67% descriptively; two blocks cannot resolve acceptance. Memory changes only about -0.2% to -0.4%. Restore the exact colored source at `30fb0c2`, rebuild, and pass fresh 65/23 Full plus 9/168 keyboard checks. Compiled inputs match the earlier colored build; PE/PDB bytes differ and the fresh artifact set is recorded separately. No feature reduction is shipped.
- [x] P20 auditor correction: full new diagnostic reports expose a field-loop variable overwriting the scheduled-block key. Preserve that failed audit; use a distinct scheduled-block variable and add a valid fixture with diagnostics in every slot. The corrected real 16-process replay passes 4,263,509 independent checks; the all-slot fixture passes alongside both valid schemas and all 20 malformed receipts. This changes tooling only.
- [x] P20 reference refresh: overlays `604bbaf3`, `54825846` and `ac73cb42` change only the common performance C++ harness over the retained original/starting/actual-base sources. All nine common inputs match; source trees and pins remain unchanged (4077/4077/47a9). All three canonical Release builds and new EXE/PDB freezes pass; actual PR16 anchor remains `f33471da`.
- [ ] P20 remaining performance work: preserve all three original-UI flags until a qualified final comparison or explicit measured advice closes them. Investigate a conservative source-ink drawing optimization that preserves colors, copied-layout behavior, selection, color fonts and fallback; retain working-set and Compose command-cost investigation separately. The current-source SDK A/A policy calibration and final refreshed 576-process comparison remain required; no bands, workloads or policy flags change silently.
- [ ] P20 upstream timing: [DxUi PR109](https://github.com/RedSalamanders/DxUi/pull/109), source `b93d1527`, fixes pending-frame Update accounting in both native render paths. Its focused regression and all 19 ordinary local Release suites pass; remaining native/platform/CI/performance qualification stays open. The consumer remains pinned to SDK4077 and earlier reports retain their incomplete Update-field boundary.
- [ ] P20 desktop lease: PR108 `fcd14542` passes all six hosted Menu suites (124 cases each, no skips) and focused/full local Debug Menu checks. Hosted run `40320360351` has five successful native profiles; ARM64 ASan fails exact cursor restoration after testing. A separate source-bound diagnostic extension records setter/readback, clipping and desktop context without changing restore behavior or weakening failure. Native execution of that extension and the cursor failure cause remain open.
- [x] P19 fresh primary comparison: retain all 576 fixed processes at app `09f22877` / SDK `4077f57d`, diagnostics OFF, with 25,202 independent audit checks. Original-UI A/B reports three findings: Compose command +17.4824%, Split owner CPU +11.1539%, and Split working set +2.0931%. Split private bytes +1.7899% does not resolve beyond the fixed 2% band. Starting-redesign A/B has five improvements and no regressions; actual PR16-base A/B has no resolved changes. Across six studies, 36/72 controls are unstable; all observer windows have complete cadence coverage but none verifies quiet CPU. No original-UI performance acceptance or causal cross-study improvement is claimed.
- [x] P19 preparation attribution: the four fixed current-only processes at `30fb0c2` retain 512 transactions and 8,704 attempts. Primary layout completes on attempt 16; Split mirror ink completes on attempt 2. The matched earlier 16-process frame study points to terminal rendering, without stage CPU attribution. Diagnostic storage is explicitly 6,490,112 bytes per process; instrumented memory does not replace primary evidence. See the [diagnostic packet](../../../Measurements/RedesignCompletion_2026-10-26/editor-attempts-30fb0c2/README.md).
- [ ] P19 Menu CI follow-up: SDK PR108 `6d8aa6e` passes the focused and full local Release Menu runs, but hosted run `40309866176` fails the hover case in x64 Debug. Its physical pointer remains over B1 during synthetic B2-to-B1 admission; preserve that mismatch as a candidate cause, split per-leg diagnostics and align owned physical input before validation. ARM64 Release's Menu case passes; its job separately fails cursor restoration after the suite. Neither failure is waived.
- [x] P19 Menu fixture checkpoint: SDK PR108 source `fcd14542` aligns and verifies the physical pointer for each B2/B1 leg and splits dispatch/hover/timer diagnostics. Focused and full local Debug Menu checks and all six hosted Menu suites pass. The separate ARM64 lease-restoration failure remains open; production Menu code is unchanged.
- [x] P19 integration checkpoint: SDK PR107 merges as `3bcf7ed9` with the exact reviewed/tested `18be6688` tree. App PR16 remains open. SDK performance policy and original-UI costs remain open; merge is not qualification.
- [x] P18 final native/input checkpoint: app `16bea061` / SDK `4077f57d` passes all six canonical local builds, all three x64 Full runs (65 behavioral cases and 23 hidden captures each), the real x64 ASan detector and the nine-stage/168-event automatic-Start keyboard slice. Foreground, focus and pointer are unchanged; clipboard is untouched. ARM64 local evidence is cross-build only. Hosted RedTilde run `40295372800` starts no steps because of payment/spending restrictions; native hosted ARM64 remains OPEN.
- [x] P18 upstream checkpoint: SDK4077 passes all six local builds, all 19 ordinary scopes in each x64 configuration and all four final foreground suites. Hosted run `40286504840` passes six actual native profiles and consumers at merge commit `646b257a`, whose tree exactly equals reviewed head4077 (`18be6688`). Both ASan probes pass. Hosted ARM64 initial focus is unavailable; broader focus/physical IME/AT remain separate.
- [x] P18 latest upstream performance diagnosis: the final hosted paired verdict is inconclusive solely because its measured base policy is unqualified. All 78 numerical metric rows are within-noise-budget, but 62/72 scheduled controls are unstable (163/1,872 out-of-band observations). `StrictControls=false` is a setting, not another failure reason. Retain the eight provider ZIP digests, actual merge/base bindings and observer gaps; do not claim overall CI/performance acceptance.
- [x] P18 final reference freezes: canonical builds and freezes complete for original UI `8c20c27a`, starting redesign `9734f7ea` (both SDK4077) and actual PR16 base overlay `6b9aa29f` over `f33471da` (unchanged SDK47a9). Root replay verifies manifest/EXE/PDB identities and all nine common inputs. The declared fresh 576-process A/A/A/B sequence has not started.
- [ ] P18 submenu admission repair: a separate test-only SDK checkout at `31711cc0` deterministically starts with the pointer over B1 and requires B2-to-B1 movement before the unchanged submenu assertions. Production Menu code is unchanged. A counterfactual removing only that transition is building; require the predicted failure and repaired focused/full Menu passes before publication. This is not yet proof of the historical CI event.
- [x] P18 final SDK input: source `4077f57d` passes Menu, NativeTextInput, MenuResources and MenuResourceScaling under the authorized default-Start lease. All four exit zero; foreground, keyboard focus and pointer are unchanged. The remaining local Debug/ASan and ARM64 build matrix is running; this input result does not replace it.
- [x] P18 consumer Release: corrected app source `7e9de544` at SDK `4077f57d` builds and passes 46/46 Editor checks, then all 65 behavioral checks and 23 deterministic captures in the ordinary Full run. Debug, ASan, ARM64, final input and fresh paired comparisons remain open.
- [x] P18 changed-header qualification: the root-executed compiler/layout probe binds exactly SDK47a9 to SDK4077, all 58 header rows and the three changed headers. MarkdownSyntax grows 16 bytes; the other four measured layouts remain equal. The new fixed v2 profile passes 2,768 tooling assertions and 2,321 independent evidence checks, while the byte-exact v1 fixture stays supported. This is source-coordinated rebuild evidence, not a general binary ABI or actual-PR-base performance result.
- [x] P18 reference source preparation: preserve both modified experiment files and their hashes before restoring full production source at actual merged PR base `f33471da`. Its harness-only overlay `6b9aa29f` retains SDK47a9; refreshed original/starting overlays `8c20c27a` and `9734f7ea` adopt SDK4077. All nine common input byte hashes match. Canonical builds, new freezes and all three A/A plus A/B studies remain pending.
- [ ] P18 SDK performance policy: current CI native profiles pass at SDK4077, but performance acceptance still requires a trustworthy versioned base policy. Historical A/A evidence uses older judge/assignment sources, reports unstable controls and explicitly leaves the policy unqualified. Prepare a fresh current-source calibration and independent review; never self-qualify the candidate or change the fixed bands/workloads.
- [x] P18 consumer build correction: preserve the first SDK407 Release attempt, which fails C++ protected-access checks in the mirror adapter before a runnable artifact exists. The primary now uses a small app-owned Editor wrapper with a private sibling/host predicate, avoiding a shared SDK API expansion. The repeated Release build and all consumer runtime gates remain pending.
- [x] P18 corrected SDK Release: exact source `4077f57d` passes the Release build, the identical re-adoption counterfactual probe (exit 0) and all 19 ordinary suites with zero skips. The source `282da2b1` input checkpoint passes all four foreground suites with restoration; final repaired-source input and remaining profiles are still pending.
- [x] P18 clipping diagnostic: v5 retains all 64 cases and clipped-layout A/A; using a separate per-primitive clip makes all 64 compact-glyph comparisons byte exact. Aliased/no-clip paths still differ in the original eight cases. This identifies the clip-AA difference only; no production cache change or achievable memory saving is claimed.
- [x] P18 parser correction: a retained native probe at SDK `282da2b1` reproduces re-adoption leaving superseded parsing pending (exit 1). At repaired source `4077f57d`, the identical probe exits 0, cancels staging and preserves the same immutable wrapper. The candidate pin moves to that exact repaired source; the earlier Release/input checkpoint remains bound to `282da2b1`. Final consumer/source-profile qualification is still pending.
- [x] P18 source checkpoint: SDK `282da2b1` implements exact immutable parsed-data adoption and the callback-free editor API, with source-coordinated API revision 4 and an explicit MarkdownSyntax layout change. The parser case and both native/embedded accessibility cases pass focused Release qualification, including exact source-color pixels and preserved scroll/geometry/history. An observed allocation-failure sweep and bounded Menu failure diagnostics are added; their execution and the final matrix are still pending. The consumer adapter and shared-storage/fallback tests are implemented but not yet built at the new pin.
- [x] P18 feature-cost localization: the admitted fixed 16-process diagnostic retains all processes and both blocks per view, with matching nine-input/compiler/SDK preflights. Removing only mirror parsing/coloring changes Split owner cycles by -9.8859% and Compose by -0.6296%. The disabled source fails exactly two existing checks (44/46); restored full source passes 46/46. This supports the added mirror analysis as a major Split cost; it is not a valid feature reduction or primary acceptance result.
- [x] P18 icon prototype qualification boundary: after preserved build/setup failures, the corrected standalone probe reaches 64 real pixel comparisons: 56 exact and eight one-channel, one-level clipping mismatches at 96 DPI. No tolerance is changed and no prototype enters production. A further constrained experiment must explain and eliminate the mismatch before any cache proposal.
- [x] P18 keep the full tested source-color behavior with optional immutable syntax reuse upstream. Exact text/profile/region validation, target-local revision/document namespace, immutable owner lifetime, independent parser fallback, allocation failures and changed-header qualification pass the named source-bound checks. Consumer wiring stays on the app owner thread with lifetime guards; fresh primary performance acceptance remains separate.
- [ ] P18 complete a fresh code review, counterfactuals and final native/input/performance qualification at the adopted SDK pin. Retain all four primary original-UI findings until the new revision is actually measured; native RedTilde ARM64 stays OPEN while billing prevents execution.


- [x] P17 integration: RedTilde PR #15 merges as `f33471da` with the reviewed `c3a11bdd` tree. DxUi PR #105 merges as `6c3205cf` with the reviewed `47a9d259` tree; evidence PR #106 merges as `5a6ea951` with the reviewed `7e34c817` tree. Remote identities and exact trees are verified. Integration does not accept the remaining performance costs.
- [x] P17 upstream foreground qualification: exact SDK47a9 passes Menu (123 named cases), NativeTextInput (172), MenuResources and MenuResourceScaling with zero skips under the authorized automatic 30-second Start countdown. Independent and lead replay verify 17 retained payload streams and exact test/lease binaries; foreground, focus and pointer are unchanged. See the [upstream evidence](https://github.com/RedSalamanders/DxUi/blob/main/Measurements/RedesignCompletion_2026-10-26/native-input-47a9/README.md); physical IME/AT remains separate.
- [x] P17 hosted boundary: RedTilde source `c3a11bdd`, run `40265947625`, starts no native steps under the explicit payment/spending restriction. Hosted RedTilde ARM64 remains OPEN. SDK47a9's completed six-profile native qualification remains valid for that named source; newer SDK main `5a6ea951` completes with five native passes and an ARM64 ASan Menu right-arrow assertion failure (run `40270999389`). The failed trace and provider artifact are retained for investigation; overall CI is not green.
- [x] P17 captured-event audit: the fixed four-process Split ABBA packet replays 12,494,229 events and 4,143,900 exact stack references with zero remaining errors. Independent review matches all 130 checkpoints, live byte/block totals, ordinals and top-stack records across decoder versions; 1,525 guarded ReAlloc summaries change none of those quantities. Exact PE/PDB checks and the mismatched-PDB refusal are archived. Matching large graphics groups and a differing button-paint request group narrow the next experiment; no full-process ownership, aligned steady-state delta or achievable savings is claimed. See the [heap packet](../../../Measurements/RedesignCompletion_2026-10-26/heap-split-0f68857b/README.md).

- [x] P17 bounded experiments: retain the first compact-icon probe's build/setup failures; its initial summary misclassified setup errors as pixel mismatches and is not parity evidence. A versioned probe separates those outcomes and includes actual Fluent icon cases. A separate private checkout at merged `f33471da` removes only the mirror Markdown-profile/rendering configuration for feature-cost attribution; the main application keeps its tested colored mirror. The admitted full source passes 46/46 Editor checks; disabled source fails exactly two (44/46). All 16 descriptive reports and both per-view blocks are independently replayed and archived in the mirror-color packet. V4 preserves all 64 cases and exact clipped-layout A/A, but all three alternate paths, including the same layout without CLIP, differ in the same eight cases. No icon/cache/clipping prototype is adopted. Neither experiment is a shippable optimization or primary acceptance result.

- [x] P13 SDK menu checkpoint: PR #104 merges as `619dc88b` with the exact tested `30fccffe` tree. Independent lead replay rehashes all six provider ZIPs and checks 126 zero-skip scope reports, 738 individual Menu case completions (123 per profile), both acrylic cases in each profile and both ASan detectors. The earlier 124-case projection counted the suite banner; the published packet and independent replay now use the corrected individual-case count. Hosted ARM64 initial keyboard focus is unavailable (`foreground-minimized`), so parent focus-restoration qualification stays open. Paired collection/verdict steps were skipped.
- [x] P16 final native replay/publication: independently verify 1,871 retained payload files and 69 capture/provenance pairs for the six-build `fc6c62d` matrix. All three x64 Full profiles pass 65 behavioral cases (46 Editor) and 23 captures; real x64 ASan detection passes. ARM64 is cross-build only. The [native packet](../../../Measurements/RedesignCompletion_2026-10-26/qualification-fc6c62d/README.md) preserves those exact source and artifact identities. Six later documentation receipt refreshes retain every native EXE/PDB/ASan runtime byte and compiled source identity; they do not rerun native tests.
- [x] P16 final input/publication: the authorized default-Start keyboard lease at `0f68857b` passes nine stages and 168 injected events on the qualified native bytes. Independent and lead replay match all nine payload hashes, Release EXE/PDB and receipt bindings; foreground/focus/cursor are unchanged and clipboard is untouched. The [keyboard packet](../../../Measurements/RedesignCompletion_2026-10-26/input-0f68857b/README.md) retains physical IME/AT and broader focus qualification as separate gates.
- [x] P16 fresh comparison collection/replay: retain the declared 576-process sequence at `0f68857b`, diagnostics OFF, seed 20261026, all nine common inputs. Canonical independent audit passes 25,177 checks; lead raw replay rehashes 576 reports and recomputes 72 rows, 864 effects, 1,728 drifts and 936 CSV cells. Independent read-only projection review matches every raw hash and projected cell. The [public packet](../../../Measurements/RedesignCompletion_2026-10-26/performance-0f68857b/README.md) keeps all three source lineages and their A/A controls separate.
- [ ] P16 original-UI performance acceptance: four findings remain—Compose command +9.5339%, Split owner cycles +12.1749%, Split private bytes +2.0231%, Split working set +2.3906%. Only the last has stable same-side controls. Starting-redesign A/B has four improved memory rows and no regressions; actual PR15-base A/B has no resolved changes. All A/A rows are unresolved. Across six studies, 39/72 controls are unstable and no quiet CPU window is verified. These results neither establish a causal gain over the older study nor accept the original-UI costs.
- [x] P16 SDK CI diagnosis: workflow `40253024676` at `47a9d259` passes all six native jobs but fails paired verdict publication and `ci-gate`. The 144-receipt paired collection completes; its base policy is unqualified, so the provider verdict is inconclusive. Full control replay has 71/72 unstable comparisons (an earlier 55 count used one-direction verdicts instead of two-direction controls), 271/1,872 out-of-band observations and observer cadence breaches. No pairing or implementation defect is established; do not report overall CI or performance green.
- [ ] P16 allocation attribution: source review rules out blanket eager preparation of hidden descendants, and both original/current apps construct three rich editors. Preserve the first heap-only ETW probe, named-kernel invalid-flags refusal and canonical kernel access-denied refusal. A separately declared four-process Split ABBA diagnostic retains exact original/candidate binaries, heap stacks, exact live child module maps and module hashes. Replay/schema/stack association and allocation-owner attribution remain pending; observer overhead and two processes per role prohibit acceptance claims.
- [x] P15 source review: the current 635-row ledger retains historical notes, adds the fixed profile input and records the upstream Menu plan rename. Lead replay verifies 635 unique repository/path entries and every raw file hash. This source-binding check is not a runtime or performance acceptance result.
- [x] P15 SDK hosted native archive: PR #105 at `47a9d259` passes all six actual native jobs. Independent replay verifies all six provider ZIPs, 496 payload files, 126 zero-skip scope reports, 738 Menu completions, both snapshot cases in every profile and both ASan detectors. The provider merge tree equals the reviewed head. No native PE/PDB is retained in those ZIPs. ARM64 initial keyboard focus is unavailable (`foreground-minimized`); focus-restoration qualification stays open. The later paired job completes measurement and fails policy publication, as recorded in P16; no overall workflow/performance acceptance is claimed.
- [x] P15 final input/comparison execution: complete the authorized keyboard lease and fixed 576-process sequence. Original and starting references use SDK47a9; actual PR15-base production and SDK2fed stay unchanged under a harness-only overlay. All nine common inputs and exact compiler/header preflights match. Preserve every result/refusal; unresolved flags remain the separate P16 acceptance item.
- [x] P14 pre-reentry-fix native qualification: pushed source `2119a03a` passes all six canonical builds. All three x64 Full runs pass 63 behavioral cases and 23 hidden captures each; x64 ASan detects the intentional heap-use-after-free. ARM64 is cross-build only. Preserve the earlier private-wrapper argument and module-rebind failures separately; no native tests ran in those failed wrapper attempts.
- [x] P14 view-transition implementation and regression: the new transition case fails on unchanged behavior with 45/46 Editor cases passing (run `20261026T020037341Z-37596-37399e3f06f44f2ebc0dff0d37675578`). Its binary/PDB/receipt/source remain archived. The repaired continuation passes 46/46 in run `20261026T020303221Z-37092-746556aeaa484597912b834e83858e92`; source-only nested edits complete the requested transition, while a newer view or retired root aborts it. Additional guards cover hint/visibility publication. Final native/input/performance qualification remains OPEN.
- [x] P14 comparison tooling implementation: adopt the fixed two-header SDK extension profile and correct A/A admission to validate the frozen executable actually executed on both sides. Lead replay passes 1,839 tooling assertions and 1,402 independent evidence checks, including invalid identity/header/layout/method/profile cases. The isolated probe supports equal measured class sizes and the additive nonvirtual API under the named compiler; it does not prove general ABI/runtime compatibility. Fresh common-harness reference overlays and paired study receipt replay remain required before acceptance.
- [x] P13 shared-source implementation and focused qualification: SDK `47a9d259` adds the owner-thread read-only Source snapshot API; the consumer adopts that exact pin for visible Split. Independent target parsing/revisions, hidden deferral and exact fallback remain. Release passes all 44 Editor cases, then all 63 behavioral cases and 23 hidden captures (run `20261026T012247045Z-4164-00ea2dbd380443d6a397ca67ba8361a9`). New app cases cover fallback recovery, nested edits and root retirement. SDK source-sharing counterfactual fails as expected; restored Window and Embedded cases pass with zero skips. Retain the initial hidden bitmap fixture failure separately. Final matrix, input and performance qualification remain OPEN; no gain is claimed.
- [x] P13 actual-base comparison admission: retain the ordinary dependency-adoption/header guard and qualify the separate exact-pair extension profile with all 58 source/package headers, supporting ABI probe and negative cases. The refreshed actual-base overlay `b69bb2a0` changes only three harness inputs over `45493e63`, preserving all production files and SDK2fed. Its completed P16 A/B study uses `reviewed-header-delta`; its A/A uses the same frozen baseline twice. Original/starting SDK47a9 references remain separately named and are never relabeled as the actual PR base. Admission and arithmetic replay do not accept performance.
- [x] P13 latest RedTilde CI observation: draft PR #15 at `00b86fcf`, run `40248574679`, again starts no native work under the explicit payment/spending restriction. Hosted RedTilde ARM64 remains OPEN.
- [x] P13 resident-memory localization: retain one fixed eight-process original-UI/454 diagnostic (Compose ABBA, Split BAAB), 34–38 non-atomic snapshots per process at roughly 109–111 ms spacing. Independent raw replay matches all 32 slot summaries. Median private committed deltas are about +0.94 MiB Compose and +1.99 MiB Split; private resident deltas are about +0.90 MiB in each view. Image resident growth includes the application image; system font-cache mappings are not app-owned allocations. The observer adds overhead, samples only two processes per role/view, and establishes neither an allocation owner nor performance acceptance.
- [x] P13 diagnostic interpretation: raw original-UI/454 transactions execute 4/13 active selection scopes in Compose and 5/14 in Split. Those nine extra scopes add 18 clock reads and 18 thread-cycle queries per transaction to the enclosing diagnostic phase. The observed phase delta therefore mixes product work with added tracing. Diagnostics-off acceptance gates out those timing calls, retaining lightweight scope branches; do not subtract a guessed cost. The durable rule is in Testing Performance.

- [x] P13 checkpoint integration: PR #14 is merged as `45493e63`; its tree equals reviewed `8c37dcc`. All 634 source-review inventory hashes pass independent lead replay. The source, input and performance packets are published; performance acceptance and hosted RedTilde ARM64 remain OPEN.
- [x] P13 pre-change localization: freeze exact merged base `45493e63` with unchanged qualified CD95 EXE/PDB bytes, then retain a separately declared 16-process original-UI A/B diagnostic (2 blocks/view, 64 cycles, seed 20261026). The canonical audit passes 120,456 checks and 13 malformed-record rejections. Visible block/position publication remains measured work; these inclusive spans are not achievable savings or causal attribution.
- [x] P13 bounded index implementation and focused regression: `c9ab2c59` stages source facts before a no-throw swap, reuses one retired line-index buffer under the current-line bound, and releases oversized buffers. The legacy reserve-hint path is unchanged. Release Editor passes 41/41. Restoring the original application analysis call produces exactly one expected failure (40/41); restoring the candidate passes 41/41. Independent lead replay rehashes 348 payload files and binds both executed binaries, source identities and the 60-case catalog. The first separately compiled positive EXE was not retained; only the retained final positive artifact is replayable.
- [x] P13 bounded index localization: retain the predeclared 16-process merged-PR14-base/C9 diagnostic (2 blocks/view, 64 cycles, seed 20261026). The canonical audit passes 134,801 checks and rejects all 13 malformed diagnostic receipts; nine phases and twelve selection components are complete. The host observation is not quiet. This descriptive run supplies no performance acceptance or causal gain claim.
- [ ] P13 bounded index final qualification: complete final profile/input and fresh paired performance validation on the final production revision. Focused Editor/counterfactual evidence does not replace these gates.

- [x] P12 documentation continuation: six canonical builds at `49cfb9a` preserve the exact compiled identity and all qualified EXE/PDB/ASan DLL bytes from `1d39ee27`. Independent replay checks six new receipts, 20 frozen artifacts and 14 live native files. See the [documentation rebind](../../../Measurements/RedesignCompletion_2026-10-25/docs-rebind-49cfb9a/README.md); no tests or performance workloads reran on this documentation source.

- [x] P11 local matrix and input: source `1d39ee27` passes six canonical builds and three x64 Full runs (59 cases and 23 captures each), including real ASan detection. Independent and root replay bind 20 artifacts, 1,862 payload streams and 69 capture pairs. The separate automatic-start keyboard run passes all nine stages/168 events with no clipboard use, unchanged foreground/focus and restored pointer. See the [matrix](../../../Measurements/RedesignCompletion_2026-10-25/qualification-1d39ee27/README.md) and [input](../../../Measurements/RedesignCompletion_2026-10-25/input-1d39ee27/README.md) packets; ARM64 remains cross-build only.
- [x] P11 fresh comparison: the fixed 576-process `1d39ee27` study and its three independent audits complete. Root replay matches all 72 rows, 864 effects, 1,728 drifts and 936 CSV cells. [Original-UI A/B](../../../Measurements/RedesignCompletion_2026-10-25/performance-1d39ee27/README.md) retains four findings: Compose command +10.8579%, Split command +7.1672%, Split CPU cycles +12.3126%, Split working set +2.0204%. Private bytes are within the investigation band in this cohort; no across-study causal improvement is claimed. Starting-redesign A/B has four memory improvements/eight no-resolved-change rows; actual PR14-base A/B has twelve no-resolved-change rows. All six envelopes lack a verified quiet CPU window; 36/72 metric controls are unstable.
- [ ] P12 remaining performance: investigate and optimize the four original-UI findings without rebaselining or changing bands/family. Source review identifies duplicated mirror syntax parsing and per-revision line-index allocation as hypotheses; neither is attributed a measured cost. Preserve bounded storage, source/selection/history, pending previews and source-color parity.
- [x] P12 SDK menu CI/archive/integration: PR #104 merges as `619dc88b` with the tested `30fccffe` tree. Six provider ZIPs rehash correctly; all 126 scope reports and 738 individual Menu cases pass with zero skips, plus both real ASan detectors. Initial failed/ambiguous attempts are preserved. The menu fixture plan is Done upstream; parent performance and ARM64 initial-focus gates remain OPEN.
- [x] P11 SDK evidence integration: DxUi PR #103 merges as `b6060881`, with the same tree as reviewed `24b963e4`. The SDK main `97b26bad` run also completes all six native profiles successfully. The separate paired benchmark policy and unstable controls remain unresolved.
- [x] P11 build attestation repair: bind schema-2 receipts to compiled first-party bytes, the exact clean SDK source, source/package headers, resolved consumer properties and evaluated compiler context. All 38 focused tooling cases and six real compiler preflights pass. The canonical guard rebuilds restored source whose timestamp predates the counterfactual objects. Final matrix qualification remains below; retain every earlier failed or invalid attempt.
- [x] P11 integration checkpoint: DxUi PR #102 merges as `97b26bad` and RedTilde PR #13 as `661858a2`; both provider trees exactly match their reviewed heads. Six documentation-only RedTilde builds at `ce53d57` preserve all native bytes from tested `9025be4`. The primary checkouts remain untouched; integration does not accept the five original-UI findings or billing-blocked hosted ARM64.
- [x] P11 hidden inspector implementation and focused qualification: retain newest facts while hidden and publish before reveal; independent lifetime/reentry and list/selection review is complete. All 40 Release Editor cases pass, including 64 hidden edit/undo cycles, initial/narrow/Focus/outline/Split/Files reveal, first visible UIA text and visible undo/redo. The unchanged-production regression and separately retained eager-publication negative control fail as expected. Visible pending-paragraph preview publication is tested separately from hidden ready-list publication. Full matrix and performance remain below; no gain is inferred from source changes.
- [x] P11 evidence integration: publish the new packets and diagnostics, independently bind the 634-file source-review ledger and unchanged native bytes, then merge PR #14 as `45493e63`. This plan remains in WIP while required performance/platform gates stay open.
- [x] P10 later integration: push and merge DxUi PR #101 as `4faf16ed` and RedTilde PR #12 as `45a8bba3`; both merge trees exactly match their reviewed heads. The primary checkouts remain untouched. Integration leaves the performance, input and platform gates open.
- [x] P10 publication checkpoint: publish the [c60 paired packet](../../../Measurements/RedesignCompletion_2026-10-24/performance-c60e9ca/README.md) and [334/c60 local matrix](../../../Measurements/RedesignCompletion_2026-10-24/local-qualification-33481d3-c60e9ca/README.md). Lead replay matches all 576 native report hashes, 72 rows, 864 block effects, 1,728 drift values and 1,080 CSV cells; the matrix replay checks 14 native artifacts, 1,862 payload files, 174 ordinary outcomes and 69 capture/sidecar pairs. Preserve the private empty-CSV attempt separately.
- [x] P10 current diagnostic checkpoint: a separately declared, fixed 16-process c60/SDK1714 cohort passes 134,800 independent checks and all 13 malformed-record rejection cases. Block-label and position-status work remains prominent; the original A/B raw CPU decomposition places most Split CPU growth after command handling. These descriptive observations neither attribute an allocation owner nor replace any primary finding.
- [x] P10 active SDK adoption: pin `2fed8246`, which includes default-format cache reuse and skips redundant zero-spacing mutations in Label measure/paint while retaining cached layouts. SDK PR #102 merges as `97b26bad` with exactly the reviewed tree. All 25 public source headers have identical Git blobs to SDK1714; the common source/package inventory contains 58 matching headers. The common zero-spacing parity case passes on unchanged `cd35e988`; the candidate passes both Label cases and the WindowHost alias case with zero skips. Both repositories pass six local builds and all three x64 Full profiles; SDK native ARM64 also passes. The compiler/header preflight, final keyboard/menu runs and fresh three-reference comparison are complete at the named source revisions. No CPU, memory or five-flag resolution is inferred.
- [x] P10 new reference preparation: retain the prior freezes and build/freeze original UI `c1579053` and starting redesign `4ce958a8` with the same SDK2fed pin and common frame harness. Preserve the actual PR13 base as `45a8bba3` plus harness-only overlay `17bb36bf` at SDK1714, with no application production changes. All three compiler/header/eight-input preflights pass. The first actual-base freeze had a raw CRLF/LF header mismatch despite identical Git content; retain it and the refused preflight, then use the rebuilt freeze with matching common bytes for the declared study. This preparation supplies no timing result.
- [x] P10 prior app input: the authorized default-Start countdown completes all nine keyboard/focus stages with 168 injected events on the c60 binary at SDK1714. The exact dirty-documentation source snapshot, native artifacts and nine payload streams pass independent and lead replay; foreground/focus are unchanged and the cursor is restored. Preserve the snapshot separately; this is not execution of later harness or SDK changes.
- [x] P10 frame diagnosis checkpoint: publish the [fixed 32-process attempt diagnostic](../../../Measurements/RedesignCompletion_2026-10-24/frame-stages-bae1572/README.md). Original reference `b555da3` and candidate `bae1572` share SDK1714, all eight harness inputs and the 58-header inventory. Independent protocol audit passes 226,562 checks; independent and lead replay retain 6,144 transactions, 71,680 attempts and 286,720 raw values. All 12 phase/role/view/kind summaries use transaction sums, then process medians, with A/A and A/B separate. Keep the 576-run primary packet unchanged; this does not attribute CPU or accept performance.
- [x] P10 prior native matrix: publish [bae1572 qualification](../../../Measurements/RedesignCompletion_2026-10-24/qualification-bae1572/README.md), with all six builds, three x64 Full runs (58 ordinary cases and 23 captures each), default/WARP lifecycle and crash/recovery checks, and real ASan detection. Independent and lead replay rehash all 20 frozen artifact/receipt files and 1,862 payload files. The three ARM64 rows are cross-build only; native ARM64 and focus remain separate.
- [x] P10 later hosted checkpoint: workflow `40216497197` at `a043de20` cannot start tooling/native-scope because of the explicit payment/spending annotations, and skips the native matrix. Hosted RedTilde ARM64 stays **OPEN**. DxUi PR #102 workflow `40211282439` at `cd35e988` passes all six native jobs; paired collection remains pending. Preserve its six provider-hash-verified archives separately from the new `2fed8246` candidate. The new ARM64 Debug pass does not explain the earlier menu failure.
- [x] P10 frame collector correction: preserve the two 38/39 Editor attempts caused by equating metric attempts with the host's narrower render counter. The diagnostic trace records three attempts versus one counter increment, including two early preparation returns with unassigned stage timings. Bind every record to explicit probe requests and keep the positive render-counter delta separate; the corrected Release run passes all 39 Editor cases (`20261025T164559423Z-39804-bd98a3a4d4c847d59b6a1a66ded17697`). The subsequent bae1572 matrix and fixed diagnostic comparison are recorded above; neither accepts performance.
- [x] P10 integration: at the owner's explicit 25 October request, push and merge DxUi PR #100 as `dc67aed6` and RedTilde PR #11 as `72d5152`. Both merge trees exactly match the previously qualified documentation heads. Integration does not accept performance costs, pending CI or product qualification.
- [x] P10 baseline preservation: retain the original-UI, starting-redesign and SDK actual-base packets and frozen artifacts; continue on `codex/performance-closeout-20261025` in both managed worktrees without changing the primary checkouts.
- [x] P10 CI checkpoint: RedTilde workflow `40188473067` executes zero steps because of the account payment/spending restriction; hosted RedTilde ARM64 stays open. At 11:46 UTC, the latest SDK workflow `40187599911` passes x64 Debug/Release and static/tooling checks while ASan, ARM64 and paired collection remain pending.
- [x] P10 later SDK native checkpoint: workflow `40187599911` subsequently passes all six native jobs, including ARM64 and ASan; paired collection remains running. These are the merged implementation's `9a642c52` checks, not tests of new diagnostic source.
- [x] P10 diagnostic Release checkpoint: 38 Editor cases pass, including the new owned-heap and invalid-selection-attribution cases. Preserve the first catalog-admission refusal before updating the two new case registrations; it executed no behavioral cases.
- [x] P10 diagnostic source checkpoint: freeze clean pre-optimization `1ab8903` and its exact tested executable; collect the declared 16-process, two-block-per-view, 64-cycle cohort. All 134,670 independent checks and 13 integrity-consistent semantic rejection cases pass. Preserve the first negative-fixture refusal caused by omitted compiler receipts; copying the complete top-level evidence fixes the fixture. Host coverage is complete but mean CPU is 22.61%, so this is attribution evidence without a quiet-host or performance-acceptance claim.
- [x] P10 first repair checkpoint: `1b83960` removes transient localized-resource copies and passes 39 Release Editor cases. Its fixed small matched cohort passes 134,671 checks and 13 malformed-record cases, but establishes no command-time gain or acceptance; retain that result. The original-UI attribution packet at `e6f4114` also passes 120,326 checks and the same negative cases. Its supplied-heap busy delta is about 0.48–0.49 MB, with no allocation-owner attribution.
- [x] P10 remote checkpoint: follow-ups are pushed as RedTilde PR #12 and DxUi PR #101. RedTilde run `40195142426` again executes no native steps because of the payment/spending restriction. SDK run `40187599911` completes all six native jobs successfully, collects the paired packet, then fails verdict publication and `ci-gate` with unqualified base policy; its provider verdict is inconclusive. Neither workflow closes performance acceptance.
- [x] P10 diagnostics: characterize selection substeps and after-primary heap/storage on the fixed existing app fixture, with opt-in instrumentation, bounded storage and unchanged primary metrics. Retain the instrumented `1ab8903` pre-optimization freeze, its executable, independent numerical/semantic audits and the separate original-UI attribution cohort; these diagnose work without attributing the process-private residual to an allocation owner.
- [ ] P10 app repair: identify and remove measured avoidable work/retention, preserve source/history/UIA/captions/focus and independently verify the five original-UI slots with a fresh fixed cohort. A source hypothesis or isolated microbenchmark is not resolution.
- [x] P10 shared fix checkpoint: SDK `1714f8b0` passes three focused Release accessibility regressions. With identical current test source, retained actual-PR-base production fails the new repeated-work guard at 1,820 visits; the candidate passes at 14. The exact experimental consumer pin and original-UI reference now adopt this revision for fresh qualification. Wider SDK/consumer matrices and whole-feature comparisons remain open; no speedup or memory-owner claim follows from the visit count.
- [x] P10 consumer matrix checkpoint: at `d73f423`, all six local builds and the complete ordinary x64 Debug/Release profiles pass with SDK `1714f8b0`. ASan exposes a test-oracle error in the new native-heap diagnostic: its logical 64 KiB allocation is not represented in the observed OS heap because sanitizer interception redirects it. Keep the failed run. At `a532d00`, the independent membership observation records `false`, both physical busy-payload observations remain zero, and all 39 ASan Editor cases pass. Complete final-source profiles remain required below.
- [x] P10 comparison provenance checkpoint: retain the failed deep actual-PR-base dependency restore (vcpkg path length) and create a shorter repository-local reference without deleting the first checkout. Original-UI/same-pin and actual-PR-base/dependency-adoption comparisons remain separate; complete the strict compiler/header evidence guard before collecting either final cohort.
- [x] P10 adoption guard implementation: retain strict default compiler/dependency equality and independently reconstruct raw compiler commands. Explicit adoption permits only the declared source/package include operands and exact SDK commit/fingerprint to differ, with identical API/build metadata and matching source plus installed-package header inventories. Focused checks pass 415 assertions plus four independent reference checks; complete validation, pinned formatting and the independent cold review also pass at `33481d3`. The post-study live rehash covers the current candidate; the baseline retains its audited frozen manifest.
- [x] P10 final ordinary matrix: `33481d3` passes all six local builds and the complete three x64 profiles, including all 58 registered cases (Editor 39), 23 capture scenarios, default/WARP resources, crash/recovery and the real ASan probe. Preserve the earlier ASan heap-oracle failure separately. Correct the normative guide's stale 55/36 counts, then rebind documentation-only heads only after proving unchanged native artifacts; current-source independent replay remains separate from performance acceptance.
- [x] P10 prior SDK CI checkpoint: workflow `40197354712` at `1714f8b0` completed all six native jobs and consumer checks; paired verdict publication and `ci-gate` failed because the measured base policy was unqualified. This is a historical checkpoint, not the latest main-branch CI state.
- [x] P10 final fixed collection: on candidate `c60e9ca6` / source identity `d2a54e46` and DxUi pin `1714f8b0`, collect separate original-UI, starting-redesign and actual-PR12-base cohorts. Each has one A/A followed by one A/B, 12 blocks per view, 64 cycles, seed `20261025`, eight common harness inputs, diagnostics disabled and the unchanged twelve primary slots/bands. Preserve the actual PR12 anchor (`72d51520`), measured overlay (`a26c4dfe`) and adopted dependency (`baff43d8`) as distinct lineage fields. The 576 native receipts are hash-bound and the cohorts are not pooled.
- [x] P10 projection replay: retain all 72 metric rows, 864 block effects, 144 block-pair records and 1,728 same-side repeat drifts. Independent audit checks are 8,250 original UI, 8,277 starting redesign and 8,313 actual PR12 base. Across A/B, Original UI has five regressions; starting redesign has four memory rows classified improved (Compose private `-8.30%`, working set `-2.35%`; Split private `-9.04%`, working set `-2.46%`); actual PR12 base has no resolved changes. Overall: 4 improved, 5 regressed, 63 no resolved change. The private replay is staged for lead review; this is not policy acceptance.
- [x] P10 final local qualification refresh: candidate `9025be4b` / source identity `95566c57` at DxUi `2fed8246` passes all six local build profiles and all 58 ordinary cases in each x64 Release, Debug and ASan Debug run, with 23 hidden captures per profile and a real ASan detection probe. The three ARM64 profiles are cross-build only. Preserve the first Release attempt: three hidden-focus checks failed while an SDK foreground lease overlapped; a later serial Full run on the same executable passed all 39 Editor cases with unchanged source and assertions. The overlap is consistent with possible interference, but no per-case event trace proves an activation event or root cause. Hosted CI, focus and native ARM64 remain separate.
- [x] P10 final interactive keyboard input: the separate authorized x64 Release lease on `9025be4b` / DxUi `2fed8246` completes all nine required keyboard stages with 168 injected events, no clipboard use, unchanged foreground/focus/cursor, and no restoration problems. Strict lead replay rehashes nine copied artifacts, all nine completed payload streams, and the 168-file source input inventory; it reconstructs source identity `95566c57`. Physical IME/AT and broader focus qualification remain open. This is separate from the noninteractive local matrix.
- [x] P10 final fixed performance projection: 576 reports at `9025be4` / SDK `2fed8246`, split into A/A and A/B in original-UI, starting-redesign and actual-PR13-base cohorts. The private replay rehashes all reports and independently recomputes all 72 metric rows, 864 effects, 1,728 same-side drifts, exact p-values, Holm adjustments and classifications. Original-UI A/B retains five regressions: Compose command `+7.4456%` and private bytes `+2.1001%`; Split command `+9.3709%`, owner CPU cycles `+12.0103%` and working set `+2.2311%`. Starting-redesign A/B has four memory improvements and no regressions. Actual-PR13-base A/B has no resolved changes; that adoption lineage does not close the original-UI gate. Cohorts are not pooled and no cross-lineage gain is claimed.
- [ ] P10 current control/host interpretation: 36/72 metric rows have all 24 same-side drifts within band; 36/72 do not; 280/1,728 drift observations exceed their band. The six observer windows have complete sample coverage, but no verified quiet CPU window; selected-process activity is not exhaustive. Keep these as descriptive observations without causal attribution. Review controls by metric and chronology; do not change thresholds, repeat until green, pool cohorts or rebaseline.
- [x] P10 prior hosted CI snapshot: RedTilde workflow `40204648985` cannot start native steps under the payment/spending restriction; no post-merge main workflow is listed. SDK main workflow `40204528951` completes with five native passes and one ARM64 Debug Menu failure: a pointer-driven root switch occurs during the overlapping-popup guard. Menu implementation and test blobs match the earlier passing `1714f8b0` run. Its source is unresolved because pointer trace capture was disabled. Keep the menu and billing gates open; the earlier six-profile green snapshot is historical.
- [x] P10 current merged-SDK hosted checkpoint: DxUi PR #102 is merged at `97b26bad`; all six native jobs pass in PR run `40222621151` on synthetic merge `9ac8cc0`, and in manual run `40219420793` on exact head `2fed8246`. Their archives and payloads remain separate. This is SDK evidence, not a RedTilde runtime or performance result.
- [x] P10 current RedTilde hosted snapshot: run `40219483666` at `9025be4` again executes no native steps under the explicit account payment/spending restriction. Hosted RedTilde ARM64 remains **OPEN**; local cross-builds and passing SDK ARM64 do not close it.
- [ ] P10 current paired hosted check: the SDK paired benchmark check is still in progress. Do not describe it as passed or use the merged-SDK native jobs to close RedTilde hosted CI, ARM64, input or performance gates.
- [ ] P10 qualification: the current local app matrix at `9025be4` / SDK `2fed8246` passes six builds and three x64 noninteractive Full profiles, and the separate final keyboard/menu runs pass. The current packets do not close performance acceptance. Investigate the earlier SDK ARM64 Menu failure without weakening its assertions, keep hosted RedTilde ARM64 open pending current remote results, and retain the physical IME/AT and native ARM64 gates.
- [ ] P10 publication: publish the c60, bae1572 and latest 9025be4 primary/native/frame/input packets with their exact source boundaries. Refresh the changed-file review ledger, synchronize durable specs and folded changelogs, and retain failed attempts and unresolved gates.
- [x] P0: inspect both clean checkouts, exact dependency pin, existing plans and current qualification packets.
- [x] P0: complete three cheaper-agent read-only reviews; lead re-read the admitted findings and primary references.
- [x] P0: reconcile ownership with the existing design, creation and DxUi rich-text audit plans.
- [x] P0: write the implementation sequence, file inventory, compatibility matrices, test gates and stop conditions.
- [x] P0: pass both repositories' validators/format checks, verify inventory integrity, and resolve the independent cold review.
- [x] A0: revalidate drift, retain baselines, finish the file-by-file review ledger and classify current-profile findings.
- [x] A0 initial inventory: reconcile all 482 file reviews (446 source and 36 generated provenance) and record lead finding dispositions.
- [x] A0 prior final inventory at `83d3b1a`: reconcile the late source/documentation review deltas and verify all 536 recorded path hashes, including the changed historical measurement projections. The applied audit and lead replay have no missing or unresolved paths at that revision; P10 owns review of subsequent changes.
- [x] A0: verify design PR #10's actual base and retain the separate starting app Release baseline.
- [x] A1/A3/A5: reproduce table bounds failure, math ownership/limits/recognition defects and corrected Mermaid outcome mismatch before fixes.
- [x] A1/A2/A3/A5: pass 24 combined Debug regression diagnostics, including source/history, local fallback and UIA ownership; retain their original failures.
- [x] A3/A5: reproduce and correct shared-budget root degrees and hollow inheritance-line rendering; frozen SDK `cfe875fd` passes all six hosted Full native profiles.
- [x] A8 tooling: reproduce overlay rollback losing later edits; preserve created/replaced edits and pass all 34 paired-run tooling cases.
- [x] A5: reproduce and correct state-description label semantics and a Debug allocation-failure termination; pass normalized model and expanded parser failure sweeps.
- [x] A7: characterize interested focus-update work, reuse validated published paths and pass stale-path/hidden-focus regression checks.
- [x] A0 controls: reproduce and fix missing ProgressBar range events with a real subscribed client regression.
- [x] A9 tooling: reproduce uncontained interactive child admission and descendant completion races; pass the retained 98-check lease diagnostic.
- [x] A6/A9 prior Release: pass 54 app cases, 23 captures, crash/recovery probes and all 19 normal SDK suites with zero skips.
- [x] A6/A9 prior input: pass the nine-stage app keyboard slice and all four SDK foreground suites with verified desktop restoration.
- [x] A8 prior collection: retain both earlier 192-process app studies and their independent audits, including the historical seven original-base regressions.
- [x] A0 delta: re-review SDK `8a38e598` gallery ordering and its then-current source hash; subsequent frozen `cfe875fd` hosted gallery execution passes.
- [x] A9 checkpoint Debug: SDK `8a38e598` passes all 19 ordinary x64 Debug suites with zero skips.
- [x] A9 checkpoint builds: SDK `8a38e598` passes all six x64/ARM64 Debug, Release and ASan Debug builds with zero warnings/errors.
- [x] A9 checkpoint ASan: SDK `8a38e598` passes the real detection probe and all 19 ordinary x64 ASan Debug suites with zero skips.
- [x] A7 branding: remove ordinary-startup WIC conversion, retain the prepared bitmap and pass seven-size independent pixel parity plus device-loss/warm-raster tests (Release Editor 35/35).
- [x] A8 prior archive: publish all 48 `fa0757d` A/A/A/B metric rows with source/audit bindings and explicit exclusions; preserve this historical projection.
- [x] A7 host functional delta: pass all 14 targeted red/green cases and final frozen-source profiles for bounded submissions and live logical membership; performance acceptance remains A8-owned.
- [x] A8 resource delta: retain 24 processes covering initial, resize and three device recreations on hardware/WARP; n=3 results are mixed descriptive evidence, not acceptance.
- [x] A1: fix the reproduced table command memory-safety and data/selection defects; frozen-source native profiles pass.
- [ ] A2: contain preparation failures per object, preserve accessible ownership and characterize shutdown completion handling.
- [x] A2 fault delta: preserve failed native line-metrics HRESULTs, classify only formula-local unsupported operations as persistent, and prove same-key suppression/retry plus changed-key recovery with injected failures; counterfactuals and all three local SDK profiles pass at `baff43d8`.
- [x] A2 focused Debug: pass the injected math failure-containment case and full Embedded suite on the retained late-source build; counterfactuals and later profile qualification are recorded separately below.
- [x] A2 counterfactuals: reproduce the exact line-metrics, HRESULT-preservation and formula-local unsupported-operation failures, with independently replayed source/build/native bindings.
- [x] A3: correct math recognition and accepted-but-wrong notation; reconcile bounded capability/target data at frozen SDK `cfe875fd`.
- [x] A4: qualify the current bounded math profile on the named native profiles; full KaTeX/TeX font assemblies remain explicitly outside scope.
- [x] A5: qualify corrected Mermaid semantics and current-profile outcomes; retain MM-36 and the separately owned broader-family roadmap.
- [x] A9 frozen matrix: both repositories pass six local builds; x64 Debug/Release/ASan runtime and real ASan probes pass at app `fd24623` / SDK `cfe875fd`.
- [x] A9 frozen input: the owner clicked Start for the then-current nine-stage app keyboard run and four SDK foreground suites; restoration passed and earlier timeouts remain retained.
- [x] A9 frozen SDK hosted matrix: `cfe875fd` source tree passes all six Full native profiles, relocated consumers and galleries; per-profile optional skips are recorded separately.
- [x] A8 fd24623 collection: retain 384 native runs and 16,003 independent checks; starting-redesign comparison has no new flags, original-design comparison retains five.
- [x] A8 fd24623 stage diagnostic: retain 16 separate descriptive runs and 5,589 integrity/numerical checks; additional command work concentrates in selection/status publication.
- [x] A8 frozen SDK evidence: retain 144 same-executable calibration receipts and 144 hosted actual-base receipts, independently recompute both 78-slot families, and retain policy/control/provenance limits.
- [x] A0/A6 functional review delta: qualify Tree badge accessibility publication and Files dirty-transition selection/scroll/model-rebuild behavior in the final local SDK/app matrices; performance acceptance remains A8-owned.
- [x] A0 Tree regression: reproduce the missing dot-badge ItemStatus against the exact original accessibility source and independently verify the Release executable/run binding; fixed-source and prepared-row execution remain separate.
- [x] A0 Tree focused Debug: pass both corrected subscribed-client and prepared-row cases with zero skips; final-source matrix/performance qualification remains open.
- [x] A0 shared-control delta: guard Label text/multiline setters against embedded root retirement; retain separate sanitizer red/green evidence and pass the final `baff43d8` local matrix.
- [x] A0 Label focused ASan: pass the retirement regression for both setters with zero skips; original-setter counterfactuals and the final matrix remain separate gates.
- [x] A0 Label counterfactuals: reproduce each original setter's ASan use-after-free separately, then pass the same focused case with both guards restored.
- [x] A2 reentrant close delta: stop document-change publication after a native caption notification accepts Close; prove synchronous host retirement without assuming immediate notification-window destruction, including sanitizer red/green evidence and the final `fa492ab` local matrix.
- [x] A2 caption-close regression: reproduce the resumed `Changed` status-label use-after-free under ASan by removing only the post-caption closing guard; fixed-source Editor execution remains separate.
- [x] A2/A6 focused consumer ASan: pass all 36 Editor cases at the exact reviewed SDK pin, including Files accessibility, pending/ready caption facts and the corrected caption-close regression.
- [x] A9 late local matrix: app `fa492ab` and SDK `baff43d8` pass all six builds and all applicable ordinary x64 suites; both real ASan detection probes pass. ARM64 evidence is cross-build only.
- [x] A9 SDK failure diagnosis: preserve the baff43d8 and be1ad177 failures; local and hosted ASan traces at be1ad177 omit Beta's first event because row enqueues are not an atomic batch. A longer timeout does not repair that fixture assumption.
- [x] A9 SDK current matrix: validate the deterministic test-only correction at `594d67fb` across all 19 ordinary x64 suites, six builds and all six hosted native profiles. Local suites have zero capability skips; the real ASan probe passes. All hosted consumer/gallery steps pass, with optional instances reported separately.
- [x] A9 SDK hosted publication: independently replay 496 Full payload files, all 126 suite outcomes and both deliberate ASan probes; publish exact head/merge bindings and the native-binary/optional-payload retention limits.
- [x] A8 fresh SDK final publication: publish the `594d67fb` actual-base 78-row/936-effect/144-receipt study and its exact mirror, with repeated lead arithmetic/projection checks and both measured executable byte bindings. Preserve zero flags, 69/72 unstable controls, observer cadence breaches and the inconclusive policy gate.
- [x] A9 current input execution: the four SDK suites pass after normal Start confirmation; the nine app stages pass at `da8aafb` through the canonical visible 30-second countdown following the owner's instruction to start them. Both leases restore desktop state. Preserve the first app setup failure and the unused normal-prompt timeout separately.
- [x] A9 current input publication: publish both independently audited foreground packets; lead replay verifies the app's nine and SDK's 17 completed payload files without retagging earlier attempts.
- [x] A6 consumer counterfactuals: finish the exact Files/caption Release negative controls and intended-source green run; preserve the first wrapper receipt-binding failure separately.
- [x] A2 caller review: guard the continuation after Files badge publication if a conditional synchronous accessibility flush accepts Close; all three final app profiles pass after the guard. No dedicated notification-close reproduction is claimed.
- [x] A0 late validation: pass pinned-format checking, all RedTilde validators and the folded-changelog checks before the final source freeze.
- [x] A8 baseline refresh: independently verify the original-UI and actual-starting-redesign Release/Core freezes at SDK `baff43d8`, including all seven common harness inputs and compiler profiles.
- [x] A7 caption functional delta: publish localized Paragraph/Math text only on context transitions, retain exact equation updates and stable command Name; pending/ready oracle and all final local profiles pass. Fresh measurement remains below.
- [x] A8 prior archive: publish the reviewed `fd24623` 48-row projection and bind source, frozen inputs, host observations and independent audits.
- [x] A8 current SDK collection: retain and independently recompute the `baff43d8` actual-base hosted 144-receipt, 78-row comparison; zero regression findings coexist with 71/72 unstable controls and an unqualified acceptance policy.
- [x] A8 current app collection: retain all 384 runs at `138a648`/SDK `baff43d8` and 16,003 independent checks. The starting-redesign comparison has zero flags and four improvements; the original-UI comparison retains five flags. Both use source-derived freezes with the common SDK/harness overlays.
- [x] A8 current phase collection: retain and independently audit a separate, fixed 16-process, two-block `app-edit-phases-v2` diagnostic at `138a648`; all native/sample/source bindings pass, with no primary-family pooling or allocation-ownership claim.
- [x] A8 current publication: independently review and publish all 48 current primary rows, source/artifact bindings, host observations and the separate phase diagnostic. The lead matches all 36,864 phase values, 36 summary rows and eight collection digests; performance acceptance remains open.
- [ ] A6: finish RedTilde interaction, responsive layout, accessible presentation and exact-pin integration tests.
- [ ] A7: simplify measured hotspots and qualify incremental preparation without changing source/history behavior.
- [ ] A8: finish current-source whole-feature and library performance qualification; resolve R10/D4c advice gates without repeating unchanged-host studies until a preferred outcome appears.
- [ ] A9: complete the final-source build/runtime, input, platform, documentation and release dispositions.
- [ ] Closeout: every required row has evidence; update domain specs and capability claims, then move completed plans to Done.

Status: **BOUNDED PROFILES IMPLEMENTED; REVIEW DELTA, QUALIFICATION AND CLOSEOUT OPEN**. The owner authorized execution on
2026-10-24. The completed planning-only pass is retained below; execution needs its own source-bound native and
performance evidence. Update this checklist with the precise change, regression case and receipt for each
completed item. A checked item needs evidence, not just edited code.

### Current qualification continuation — 2026-10-25
Current native code is `1d39ee27` on SDK `2fed8246`: six builds, all three x64 Full profiles, nine keyboard stages,
and the fresh 576-process study are published in the latest packets linked at the top. The `49cfb9a` documentation
continuation retains exactly those native bytes. The latest study has four original-UI findings; performance
acceptance remains open. RedTilde workflow `40242127778` at `49cfb9a` cannot start because of the explicit billing
annotations, so hosted native ARM64 remains OPEN. DxUi PR #104 is separately qualifying its test-fixture repair.


The prior local qualification, interactive input and paired study are published as the [9025be4 native matrix](../../../Measurements/RedesignCompletion_2026-10-25/qualification-9025be4/README.md),
the [9025be4 keyboard input packet](../../../Measurements/RedesignCompletion_2026-10-25/input-9025be4/README.md),
and [9025be4 performance packet](../../../Measurements/RedesignCompletion_2026-10-25/performance-9025be4/README.md).
They supersede earlier local snapshots only for this source and SDK pin; they do not supersede prior cohort
lineages or acceptance rules. The fresh study confirms five original-UI A/B flags. Its actual PR13-base A/B
comparison has no resolved change but is a separate dependency-adoption lineage, so the original-UI repair gate
remains open. The initial Release focus failures and later serial pass are both retained without a proven cause.
Performance acceptance, hosted CI, real-focus and native ARM64 remain separate.

The remaining SDK fixture failure is corrected at `594d67fb0fcc3829541211a7c8f100df046c87e3`. The test first
receives Alpha and Beta's first statuses within the unchanged ordinary 3-second deadline, then holds one unrelated
Gamma event while queuing middle/final values and replacing Gamma's ID. A separate mark preserves held silence;
the exact first/final counts, superseded-value rejection, stale-ID rejection and replacement getter remain.
The active Gamma event is outside pending-queue pruning and has no delivery assertion. Runtime code, public APIs,
dependencies and the existing gated allowance are unchanged. Full static/tooling validation, pinned format and
official changelog folding pass. The final local matrix passes all six builds and all 19 ordinary suites in
each x64 Debug, Release and ASan profile with zero capability skips and real ASan detection. Independent replay
verifies 74 artifacts, all 57 suite/executable bindings and 288 completed payload files without a mismatch.
Local ARM64 evidence is cross-build only. Hosted workflow `40177964080` passes all six actual Full native
steps and native jobs at this head (compiled merge `8c78b5ad`, whose tree matches the head), plus every relocated
consumer and gallery. The optional STL-annotations-disabled consumer runs in both ASan profiles only;
menu-resource scaling runs in x64 Release only. Other optional instances are skipped. The
[SDK hosted native packet](https://github.com/RedSalamanders/DxUi/blob/9a642c5243d44b3637d14dc93d4bb82bbaf0ae6f/Measurements/RedTildeCompletion_2026-10-24/hosted-594d67fb/README.md)
retains six matching provider ZIP digests, all 126 Full-suite outcomes, independent replay of 496 payload files,
both deliberate ASan probes and the lead's repeated verification. Hosted Full includes Menu and NativeTextInput
under the runner's owned lease (21 suites); the local noninteractive catalogue has 19. Native build binaries
are not retained in these ZIPs, so reported compiler/executable digests are not independently rehashed binary
artifacts; optional payload inventories are not separately replayed. Its
[fresh paired packet and exact app mirror](../../../Measurements/RedesignCompletion_2026-10-24/sdk-594d67fb/README.md)
retain all 78 rows/936 block effects and 144 receipt hashes after repeated independent and lead checks. The paired
ZIP retains 24 EXE/PDB/library files, including both exact measured EmbeddedTests binaries; it has no source/object
or compiler/linker bytes. Baseline receipts report a dirty checkout with no complete dirty-path inventory;
candidate receipts report a clean compiled merge. The study has zero regression flags and 69/72 unstable controls,
while the policy gate is inconclusive. Observer replay finds 28 QPC and 29 UTC sample-start periods above the
retained 2.5-second consistency bound and about 36.6% whole-machine weighted CPU; neither continuous/quiet-host
coverage nor total observer overhead is qualified. No overall workflow or performance pass is claimed. Hosted RedTilde ARM64 is a
different gate and remains blocked below.

The final app measurements bind `138a648b292af0f810286ded05f59f37ee737f8f`, source identity
`12b446ab071132d0ead4254ee5083b50bb1e15c5a9d71ddca8cb9cb99d28d2ab`, and SDK `baff43d8`. The docs-only
refresh at that head preserves the exact six-profile `fa492ab` code artifacts and their runtime evidence.
The common-SDK original-UI freeze derives from PR #10 base `f819395d` through the disclosed harness/pin overlays
to `c04ea1e`; the starting-redesign freeze derives from PR #11 base `c7456854`, its docs-only `d53b4c6` continuation
and harness/pin overlays to `0acee29`. Neither measured freeze is an untouched PR-base checkout.

Both cohorts retain 96 A/A and 96 A/B processes, 12 blocks per view, 64 cycles and seed 20261024. Independent
audits pass 7,988 and 8,015 checks. Original-UI A/B flags Compose command +12.364%, Split command +18.747%,
Split owner CPU +13.756%, Split private memory +2.143% and Split working set +2.145%; only the flagged working-set
control is stable. The starting-redesign A/B family has zero regression findings and four improvements: private
memory -8.729%/-9.194% and working set -2.460%/-2.473% in Compose/Split. Completed-frame timing has no resolved
change in either cohort. All observer windows have complete coverage; mean system CPU spans 17.2–18.4%, with no
verified quiet window. These observations preserve every flag and do not close performance advice or policy.
The separate 16-process phase diagnostic passes its source/receipt/sample audit: 18,432 wall and 18,432 CPU
phase values, plus 2,048 reconstructed CPU transactions, have no binding, range, order or reconstruction failures.
Its descriptive four-process medians place 18.400 microseconds of additional Compose work and 24.075
microseconds in Split in Selection. This is not attribution or an achievable saving; the diagnostic has two
blocks per view and no independent host observer. It does not attribute the historical 4 MiB allocation or
replace any primary metric. The [public phase packet](../../../Measurements/RedesignCompletion_2026-10-24/command-phases-138a648/README.md)
retains all 36,864 raw phase values; the lead compares every value, all 36 summary rows and eight collection
digests to the source-bound originals, with no mismatch or private identifiers.

The current [performance advice](../../../Measurements/RedesignCompletion_2026-10-24/performance-138a648/README.md)
retains three concrete options: split Selection/status diagnostics before another optimization; obtain allocation
ownership before removing retained facts/caches; or make an explicit owner scope/deferral decision. The original
UI private-byte medians increase by 987,136 bytes in Compose and 1,040,384 bytes in Split, while the starting
redesign memory improves. Current Selection additionally publishes caret/EOL, block and search facts. Ordinary
stable-focus label updates defer and coalesce layout/UIA work; a whole-bucket increase does not identify a shared
focus-resolution defect. No option is silently accepted, and the unchanged primary study will not be repeated
until it produces a preferred result.

The documentation-only app checkpoint `be415641b5c8c07773d5674b3d8e6bb8a0178d02` passes six fresh build-only
profiles with zero warnings/errors and byte-identical EXE/PDB/ASan-runtime artifacts to `138a648`, transitively
matching the tested `fa492ab` artifacts. Its aggregate source identity is
`ffe8c8d65c6d295584f2df2607bda615dcaa8f906af181f9c1c2d8ce7e232b81`. This is an explicit unchanged-input/artifact
check, not new ordinary, sanitizer, interaction or performance execution. The current private build-only wrapper
also corrects its misleading inherited ordinary-suite console wording; the original logs/records remain intact.

App workflow `40182090717` at `be41564` again executes no steps because of the account payment/spending limit;
native jobs are skipped. Earlier workflow `40174110913` at `138a648` has the same restriction. Hosted RedTilde
ARM64 remains **OPEN**, as requested. The current input runs have completed: SDK passes all four suites after
normal Start confirmation, and app `da8aafb` passes all nine stages with 168 injected events through the
canonical visible 30-second countdown, following the owner's instruction to start it. Both preserve or restore
foreground, focus and pointer. The first app attempt fails the composite setup-focus predicate before sending
any key; a second normal prompt times out without launching a child. Those attempts remain retained. The later
exact-build pass does not establish the first failure's cause or claim a source fix. The
[app keyboard packet](../../../Measurements/RedesignCompletion_2026-10-24/keyboard-da8aafb/README.md) and
[SDK foreground packet](https://github.com/RedSalamanders/DxUi/blob/9a642c5243d44b3637d14dc93d4bb82bbaf0ae6f/Measurements/RedTildeCompletion_2026-10-24/foreground-594d67fb/README.md)
retain separate source/build and lease bindings; independent plus lead replay verifies all nine and 17 payload
files respectively. The coordinator stays in
WIP while those gates and R10/D4c remain open.

### Final production-code local checkpoint — 2026-10-25

RedTilde `fa492ab3947ec309cd3bc52d782e58063aebf962` at SDK
`baff43d8c8402258f17aa77f7b790166d57eabc9` passes x64 Debug, Release and ASan Debug with 55 ordinary cases
per profile (Storage 10, Settings 4, Recovery 5, Editor 36), 23 captures and sidecars, recovery/crash probes,
and native default/WARP lifecycle coverage. The ASan detection probe reports its expected heap-use-after-free.
All three ARM64 cross-builds pass. Independent audits verify every artifact and completed payload: 619, 619
and 621 payload files respectively, with no missing entries or hash mismatches. The additional Files callback
closing guard is present in this matrix; its own notification-close path has no dedicated runtime reproduction.
The Files regression establishes no model rebuild on badge changes, not constant-time lookup.

SDK `baff43d8` passes all 19 ordinary suites in each x64 profile with zero capability skips and all three ARM64
cross-builds. Its hosted workflow `40154492633` passes four ordinary native profiles but fails both ASan Embedded
jobs on the held four-notification batch's 3-second deadline; expected events arrive later and no sanitizer
memory report occurs. A narrowly scoped test adjustment uses the existing 20-second request allowance only after
that artificial gate, preserving the ordinary deadline and every coalescing/identity/pruning assertion. Fresh
test-source validation remains open. Its production library fingerprint and all six declared benchmark inputs
are unchanged; a rebuilt Embedded test executable is a distinct artifact.

The current SDK paired packet reports zero regression findings in 78 rows, but controls and unqualified policy
prevent an accepted verdict. Older `cfe875fd` calibration and `fd24623` app measurements below remain historical.
Both refreshed app baselines use `baff43d8` and the identical common harness/compiler profile. The final app
comparison had not started at this checkpoint; the later `138a648` collection is recorded above. A premature documentation checkpoint edit was caught before any timed collection;
its patch and failed byte-restoration checks are retained. This deliberate checklist update requires fresh
build receipts and an explicit unchanged-compiled-input/artifact check before measuring; that six-profile refresh
subsequently passed with byte-identical `fa492ab` artifacts.

App workflow `40171816856` again executes zero steps because of the payment/spending restriction. Hosted native
ARM64 stays **OPEN** as requested. Current interactive time agreement is pending; earlier Start-confirmed runs
do not establish a new-source execution claim. These limits keep R10/D4c and plan closeout open.

### Earlier frozen-source qualification checkpoint — 2026-10-25

The preceding completed full runtime snapshot is app `fd24623a9f27d04b85deb27a43a9af724dc030ca` with
SDK `cfe875fd815b0a39336100df9bde8f29ea2f0aa0` (API 4). The following observations belong to those frozen
sources; later review changes require their own build/runtime disposition and must not inherit a new execution claim.

| Gate | Retained result and boundary |
| --- | --- |
| Local build matrix | All six x64/ARM64 Debug, Release and ASan Debug builds pass in each repository. ARM64 cross-builds are not native execution. |
| App x64 runtime | Each profile passes 54 cases, 23 deterministic captures and recovery/crash/native-object probes. Runs: Release `20261024T214136040Z-34848-1634d1ea10a14d8aa34c4a2dbdc75950`, Debug `20261024T214623780Z-54152-b9213f1c4f9f4446a49f2868eefaa397`, ASan `20261024T215036320Z-54152-c63966abd6b7487eb893da9dbf9e9a12`. |
| SDK x64 runtime | All 19 ordinary suites pass with zero skips in Debug, Release and ASan Debug, including the real ASan detection probe. The serialized ASan retry is `20261024T220924385Z-51868-e09ec2547ede441489d629a7bf367d5c`. |
| Human-confirmed input | App nine-stage keyboard run `20261024T222243673Z-34040-0f7867fcb7a74c5ea9c558069fc831b0` and SDK four-suite run `20261024T222331185Z-43520-f649944f551e4e21bc67cbda8cbedb59` pass with foreground/focus/cursor restoration. Earlier prompt timeouts executed no native cases and remain retained. |
| SDK hosted | PR #100 head `cfe875fd`, workflow `40135398414`: all six Full native steps and native jobs, relocated consumers and galleries pass. Annotations-disabled consumer runs on x64 Debug, x64 ASan and ARM64 ASan; menu resource scaling runs on x64 Release. Other optional instances are skipped. Ubuntu validation/native-scope, Windows tooling and format pass. Paired acceptance is separate. Six ZIPs match their provider digests. |
| App hosted ARM64 | Workflow `40135560498` never started its jobs because GitHub reported an account payment/spending-limit restriction. On 25 October the owner explicitly chose **keep the hosted ARM64 gate open**. This is an acknowledged open gate, not a waiver or pass. |

The six SDK native receipts identify compiled merge ref `0d484babceb62f7191dc464c329ec46269affe92`, whose
parents are the actual PR base `b06f534b` and head `cfe875fd`. Its Git tree
`3bfa95c60deee0bf07794fb40ac22bbff1200e3c` exactly equals the head's tree. Preserve the actual compiled commit in
the receipts; this verifies source-tree equivalence without rewriting it as a head checkout.

Private source-bound audits, original receipts, copied native artifacts and failed attempts remain beneath
`.build/qualification-20261024` and `.build/audit-20261024` in the isolated owning checkouts. Preserve the original
SDK shared-test-lock refusal and app wrapper bookkeeping failures: the later additive successful runs and independent
payload checks establish only what they actually executed. The ARM64 short restore/cache workaround changes no source,
dependency ABI or platform claim.

The fresh common-harness app freezes are original UI `ca19da7`, starting redesign `bbc9525` and candidate `fd24623`,
all using SDK `cfe875fd`, seven identical harness inputs and compiler profile SHA-256
`490bb8b4cc127227adbbede387cde236b7a208bbedeba25bb86ed7be9e77c9313`.
`common-harness-frozen-host-policy-bindings-20261025.json` retains their full receipt/artifact/source bindings.

- Original-design cohort: 96 A/A plus 96 A/B native runs, 12 blocks/view, 64 cycles, seed 20261024; the independent
  typed reference passes 7,988 checks. A/B flags Compose command +12.54% and private memory +2.013%, and Split
  command +14.58%, owner CPU +13.44% and working set +2.61%. Only the flagged Split working-set control is stable;
  do not erase the other flags or their control uncertainty. This supersedes the historical seven-flag result.
- Actual starting-redesign cohort: the same complete protocol, 8,015 independent checks. All 12 A/B slots have no
  new regression finding. Private memory improves 9.19% in Compose and 7.98% in Split; Compose working set improves
  2.45%. Controls remain mixed. Both cohorts have complete host observations, mean total CPU around 19–21%, and no
  verified quiet window. These are reviewed measurements under observed load, not approved non-regression policy.
- The separate 16-process, two-block stage diagnostic passes 5,589 integrity/numerical checks. Its descriptive
  medians concentrate roughly 23 microseconds of additional command work in selection/status presentation, while
  Split preparation also costs more. It is not a new primary family, significance test, or causal allocation trace.
- The 24-process hardware/WARP resource-cost study is n=3 per actor/view/adapter, with mixed initial-preparation/reset results.
  Use only `resource-cost-independent-audit-corrected-20261025`; its sorted triples and medians supersede the
  unaccepted first private draft. It grants no cost acceptance or 4 MiB allocation ownership claim.

SDK same-executable A/A calibration completed at explicit revision `cfe875fd` in one separate physical worktree;
both role labels used that one executable. All 144 receipts are retained and the independent reference recomputes
all 78 metric slots with no resolved regression. The observer audit passes: 7,158 samples, 99.982304% collection
coverage, weighted CPU 25.083073% and maximum UTC sample-start period 1.2089528 seconds. This is observed load,
not a quiet-host result. The legacy judge calls one distinct-grid dirty composition slot improved while the
migrated judge calls it within noise; both report zero regressions, and the disagreement remains recorded.
All 24 within-side control comparisons are unstable in each of the three A/A scenarios. The completed
collection therefore does not qualify a noise floor or establish performance acceptance.
The current measured-base policy remains unqualified. Final-source library A/B, publication and R10/D4c
dispositions remain open.
The SDK hosted paired collection subsequently completed, but its verdict step failed with **inconclusive** because
the base policy is not independently qualified. Overall workflow `40135398414` therefore fails despite its six
successful native profiles. Its provider-bound artifact is now preserved locally and independently audited:
144 receipts and all 78 slots match the numerical reference, with no resolved regression. The 24 side-control
comparisons per scenario contain 23 unstable in Default, 23 in MultilineGrid and 22 in MultilineGridDistinct.
The measured baseline identifies `b06f534b` with `sourceDirty=true`; the lead and independent reviewer reproduced
its library fingerprint exactly from the clean actual-base checkout with the correct Windows CRLF attributes.
The retained manifest identifies three intentional tooling overlays outside that fingerprint scope. The archive
has no complete dirty-path listing, so no claim is made that those were the only working-tree changes, and no
separate compiler/source byte archive is inferred from a reported digest. This frozen evidence
does not measure the later fixes. Do not describe the workflow as green or alter candidate policy to approve itself.

A late source review also confirmed that documented Tree state-dot text is absent from UIA snapshot publication.
The bounded correction and Files dirty-badge update are being developed in the non-measured working checkouts.
The label text/multiline paths also need weak-lifetime guards after synchronous embedded preparation can retire
their owning root. The existing retirement test covers both setters and passes with zero skips in official x64
ASan Debug run `20261025T022224940Z-19880-ef222af848c049d0a63c5171d80a5a17`.
Attempt `review-delta-LabelGreen-20261025T021509802Z` binds executable SHA-256
`639F0788D8B0D5F1F3E59609147BA524776200DA6D3D3A562877990B972130F5`, canonical build identity
`a8fdb41131019d9867658ac0d83275af341e29879b69788758c1fc386f7e8fcd` and exact source restoration.
The separate original-setter sanitizer diagnostics are now retained; final-source matrix execution remains pending.
No measured guard-cost or frozen-source pass is inferred.
The Files pane is disabled in the headless performance fixture, so its rebuild cannot explain the five app flags.
The late Tree counterfactual now has a retained native red result: run
`20261025T014211314Z-2752-b42c53974c4b43de80bcde585f4b5c7b` fails at the missing dot-badge ItemStatus assertion.
Its executed accessibility source exactly matches the original `cfe875fd` Git blob and its x64 Release executable
matches the native receipt. Only the first selected test starts before failure; the prepared-row test is not
claimed as executed. `accessibility-red-independent-20261025` records the independent audit. Earlier build-only
failures remain retained and count as no native red result. Fixed-source attempt
`review-delta-Green-20261025T014741080Z` built successfully, but the Tree test exposed an incorrect style-only
getter oracle: an unchanged Saved badge still exposes Saved even when no new ItemStatus event is expected.
That failed attempt is a test-authoring failure. After correcting the oracle, both named cases start and finish
successfully in official Debug run `20261025T021304993Z-27628-17cf022cb9d24837bad2d536b899bb18`, with zero skips.
Attempt `review-delta-AccessibilityGreen-20261025T020600631Z` binds executable SHA-256
`D16F722CD16D57F12249CF7CE2968B738519DE9E48B67FE63DD52E720BEBF14B` and canonical build identity
`97960e264bbaa380d89a242a7a076865eecef18771000a77474509b41c2cc1e8`; exact source restoration passes.
The same build passes the injected math containment case in official Debug run
`20261025T015619165Z-47744-17c82175da7846ef9b9048f8a2372f5b` and the full Embedded suite in
`20261025T015815367Z-45528-48a04e2b701849a08533f314b035b44d` (455,561 checks). These are focused
late-source results, not a final matrix or performance verdict.
F08's alleged completion-post missing wake is refuted by the dispatch-level completion check and worker/HWND
retirement ordering. Existing tests cover persistence and close; explicit queue saturation and both service finish
orders are not claimed. A2's native-object persistent HRESULT/factory/upload coverage still needs its own disposition.
The separate caption callback can synchronously re-enter accepted Close before `Changed` resumes. A post-caption
closing guard and an extension to `Window.CaptionRetriesFailedAndSupersededNativeWrites` are source-reviewed.
The regression asserts immediate host/control retirement while allowing the notification HWND to be destroyed
or retained hidden for admitted persistence. The original immediate-destruction assertion was corrected during
independent review before execution; the subsequent sanitizer results are recorded below and final-source qualification remains open.
The subsequent exact guard-removal counterfactual reproduces an ASan heap-use-after-free in `Label::SetText`,
called by `Application::UpdateCounts` and `Changed`, with the caption case at `Application.Tests.Core.cpp:924`.
Official run `20261025T023443744Z-20388-06c01651c30b403d8f52a2793f5b8d97` binds variant source
`3d81f332176d61324a541e890a8e6559a27930d1c5f28100f46220a1d1db277e`, SDK `baff43d8` and executable SHA-256
`86D64D2DD278998B48330C319574644030E393ABDD4F8B0898F298E45D813E68`. Post-test canonical artifact/source
verification and exact source restoration pass. The native runner does not flush a literal case name before an
ASan abort; the verified source-span stack identifies the case. The preceding build-only WIL variant test error
is retained separately and counts as no native proof.
Fixed-source x64 ASan Debug run `20261025T064635182Z-26416-0a107f273b514f2488e25aa25fbbf315` subsequently
passes all 36 selected Editor cases (55 registered), including the caption-close case. It binds source
`86eb21cfe8414c0c3ee04aeda3d238e7b64fa5392023b55f33c7dce43d5db845`, SDK `baff43d8` and executable
`1728CAD020DA77983D4896F47EDBE96B201948530DA7980F94F52E989AD60A7C`; post-test binding and restoration pass.
The preceding 35/36 attempt is retained: its caption-close case passed, while the new caption test incorrectly
omitted the real pending-syntax Paragraph transition. The corrected test asserts stale-math clearing, current
ready restoration and allocation-free caption reuse for repeated ready facts. Production behavior is unchanged
by that test correction; the final matrix and paired performance gates remain open.
The review found that `PreparedMath::Advance` converts any failed `GetLineMetrics` HRESULT into a capacity
exception. This can incorrectly memoize a memory or temporary native failure as permanent formula rejection.
The bounded correction must propagate that HRESULT, preserve allocation/device recovery, and use the existing
complete rejection key only for a genuinely unsupported operation within one formula's native preparation.
The three retained math counterfactuals now each fail at their intended assertion. The two separate original
Label setter variants each reproduce an ASan use-after-free, and the following Label green control passes with
zero capability skips. `late-counterfactual-independent-20261025` replays all six source snapshots, successful
build identities, executable hashes, native outcomes and exact restoration. Its author discloses authorship of
the math implementation; independent source review remains the lead's responsibility.

At SDK `baff43d8`, hosted workflow `40154492633` passes the four ordinary x64/ARM64 Debug and Release native
jobs. Both ASan jobs pass the detection probe but fail the new intentionally gated, multi-publication Embedded
Tree notification test's three-second delivery wait. Failure diagnostics contain all four expected first/final
statuses after the deadline and no sanitizer memory report. This is a retained timing/assertion failure, not a
full native pass. The ordinary status-only wait and all identity/coalescing/pruning assertions must remain;
the bounded correction is reviewed separately from performance acceptance. The paired collection completes,
but its verdict remains rejected because the measured-base policy is unqualified. Fresh local SDK matrices
are running; neither those jobs nor a future correction inherit a pass from the earlier source.

The first Release Files negative control builds and executes 36 Editor cases: exactly
`Editor.FilesDirtyBadgeUpdatesOneVisibleRow` fails, at the existing-row/no-rebuild assertion. Its private wrapper
then incorrectly requires an individual suite receipt which the official runner does not produce. Preserve
attempt `review-delta-FilesRed-20261025T064810689Z` and official run
`20261025T065126884Z-14528-4bdc2035602e4ed9a5712cda8808de07`; source restoration passes. Correct the private
binding wrapper to use the real run, catalog, native report and executable/build receipts before claiming the
completed diagnostic queue. This wrapper failure does not invalidate or convert the retained native result.
The corrected private wrapper subsequently completes all three source-bound Release diagnostics. FilesRed run
`20261025T070100746Z-49404-38f5c073899744d78acc94be9966964d` fails only the no-rebuild Files assertion;
CaptionRed run `20261025T070206463Z-50068-872fc7d139e6445da97b46b414dc889d` fails only the persistent-status
caption assertion. Intended-source Green run `20261025T070354514Z-53832-d6bb4c93b846443d83f4b9a8856da4ef`
passes all 36 Editor cases (55 registered), at SDK `baff43d8`, with executable SHA-256
`21284104806ADDD253C8076E50771D35DF4E17B6A2A29250543D9A0A665C3C21`. Each exact single source transformation,
official source/build/executable receipt, pre/post canonical verification and restoration is independently replayed
in `review-delta-regression-queue-20261025`. This is focused evidence, not the final matrix or performance result.

A subsequent caller review traces a conditional synchronous WindowHost accessibility flush when focus changes.
That publication can dispatch an accepted Close. Although `SelectionChanged` already returns when closing,
the later recovery checkpoint dereferences the cached editor. `Changed` now returns immediately if closing
became true during its Files badge update. The stable-focus route posts publication instead. This is a bounded
source-reviewed defense, not a newly reproduced runtime failure; the preceding focused runs predate this guard
and the next matrix must execute the final source. It adds no new test seam, dependency or benchmark hook.

Pinned-format checking, the complete RedTilde validators and all four folded changelog entries pass after the
Editor test-oracle correction. The refreshed starting baseline `0acee29a0e597bfd5367265ff4c7d44b91f4a649`
and original-UI baseline `c04ea1e2f57fd4f323ddfc04c57b09611f17f147` each build x64 Release and pass their own
complete Core catalogues (53/53 and 27/27). `review-baseline-independent-20261025` verifies their SDK pin,
source/executable/build identities, compiler profile and seven identical harness inputs. The original baseline's
older Core receipt omits the newer build-receipt digest field; the audit retains that limitation and verifies its
other identity fields against the standalone build record. Both fresh freezes are separate from the preserved
`cfe875fd` freezes. The next current-source matrix and observed comparisons must use the reviewed new freezes.
No derived-snapshot API, new language family, full TeX font assembly or external dependency is authorized by this
checkpoint. Keep the plan in WIP until its remaining source, test, evidence, documentation and owner gates close.

### Execution checkpoint — 2026-10-24

- The owner clarified that no new dependency is wanted. Extract useful algorithms and independent test ideas
  into the owned implementation. MicroTeX, Mermaid renderers and KaTeX are reference material only; dependency
  adoption, Rust/FFI backends and imported rendering/font packages are outside this execution.
- The owner explicitly selected **finish current profiles and qualification** on 24 October. Full TeX font
  assemblies and additional Mermaid families are outside this completion scope; they remain owned by the existing
  DxUi rich-text audit/language roadmap. The broader design below is retained as future direction, not a claim that
  current profiles support it. Closing this coordinator cannot close those parent roadmap items or claim full parity.
- The next SDK host diagnostic first stopped at four test-access compilation errors; no runtime regression result
  is claimed for that attempt. Its source was restored and the failed build retained. Review also found that a live
  control moved out through legacy `GetChildren()` can retain its identity, token and interaction revision. Cache
  reuse must therefore validate bounded logical membership paths, with a full-prune fallback when a path cannot
  be represented; token/revision equality alone is insufficient. Final-source red/green and platform runs remain open.
- The owner explicitly retained the seven last-complete performance regressions, including roughly 11% higher
  private memory. An extra retained 4 MiB allocation and shared DxUi focus-update cost are investigation leads,
  not proven causes. Recent optimizations require a fresh complete same-SDK comparison against the actual PR base.
- Revalidate final-source Debug, ASan, ARM64 and the keyboard/menu profiles whose authorization the owner
  reaffirmed. Use the existing warning/desktop lease and verify focus/pointer restoration; preserve original failures.
- Archive evidence, synchronize owning documentation and folded changelogs, and move completed plans to Done
  only when the corresponding gates really close. No previous merge request accepts a performance cost.
- DxUi advanced from the planning revision to `b06f534b36ffc2c0c9671a142d1e51545480be22` before execution.
  Reconcile the new host/focus changes and the Controls.Tests/Support.Tests/Embedded.Tests renames before fixes;
  retain the original planned-at identities as history. RedTilde remains at `c7456854c8f964bb9ea067295e6435f99d426515`.
- Isolate execution in the existing managed Codex worktree workspace; exact local paths remain in private audit records.
  Repository-local `.build` directories own new outputs. The primary volume has less than 4 GiB free; no retained evidence
  or independently launched application is removed to make room.
- GitHub confirms design PR #10 merged as `c7456854` with actual base
  `f819395d36c55b93e3bf9559d12d0ec401e42b3d`. Preserve that original UI closure for the final whole-feature
  comparison, with the reviewed common harness and identical final SDK on both sides. The separate starting
  redesign baseline is `d53b4c6` (documentation-only continuation of `c7456854`); it cannot replace the original UI.
- Baseline app Release built and passed Core: Storage 10, Settings 4, Recovery 5 and Editor 34. Its immutable
  freeze is `.build/performance-20261024-start/frozen` in the isolated baseline checkout. The independent DxUi
  Release baseline built and retained `.build/reports/baseline-20261024-Default.json`; that single benchmark is
  explicitly unpaired. The source-identical library suite baseline is collected separately.
- The candidate Debug tests-before-fix diagnostic is retained in `.build/regression-20261024-red` in DxUi.
  Four independent math tests fail, table and retained structural tests assert on a vector bounds violation,
  and the corrected Mermaid model/outcome test fails. The first Mermaid run had a golden separator typo;
  it is a test-authoring failure, not parser evidence. Its corrected test-only build and failing run are retained.
- First implementation slices cover structural command corruption, per-object geometry fallback, typed math
  atoms/styles/limits, native math box preparation, shared Mermaid label validation, typed inheritance and
  real stadium shapes. They remain pending combined native verification and independent review corrections.
  Do not check the parent acceptance rows from source edits or offline generator checks alone.
- The first combined diagnostic build passed. Whitespace, operator modifiers, explicit delimiters, the 30-formula
  bounded compatibility corpus, matrix alignment and all pinned Mermaid outcomes passed individually. Its remaining
  failures included inline nested-list outdent and geometry fallback after a local font change. Both have corrections
  awaiting the next combined run. Two additional failures were independent test-oracle errors (paragraph terminal LF,
  canonical materialization of an absent table cell); their original logs remain unchanged. The reported partial-Code
  conversion defect was refuted by re-reading `CodeValue`; characterization tests now cover the resolved entity path.
- The next Mermaid slice expands grouped/chained flowchart endpoints into stable bounded edge products. State/class
  groups remain outside their admitted grammar. All 105 pinned flowcharts and all 20 actual family examples remain
  represented; the support matrix still names the unsupported families rather than claiming full Mermaid parity.
- The independent baseline DxUi runner passed all tooling, then refused its benchmark `SkipBuild` gate. A read-only
  recomputation matches the current receipt/source/dependency/artifact identities; the historical cause is unknown.
  Retain this failure and retry native baseline suites separately with the already-passed tooling omitted.
- The owned-child DbgEng diagnostic produced no allocation stack. A separate helper attempt was stopped by the
  Windows security scanner before a source file or child was created; it is not bypassed. Memory attribution remains
  open. No diagnostic attempt replaces the required fresh paired comparison.
- Windows denied heap-stack tracing (`0x80070005`) before recording. The original refusal is retained in
  `.build/performance-20261024-memory`; the unique diagnostic image remains unconfigured. Continue with an
  owned-child non-admin diagnostic. No 4 MiB attribution or performance acceptance has been established.
- The `combined-8` diagnostic in DxUi passes all 24 focused cases, including every original Mermaid outcome and
  admitted graph routing, structural transactions, math spacing/scripts/limits, local fallback and UIA. The full
  Accessibility suite also passes (122.6 s). EditorControls/Embedded expose an older whole-document capacity
  expectation that must follow A2's local-source contract, plus an inheritance triangle crossed by its connector.
  Their failed logs are retained; these are not full-suite passes. A separate first suite invocation had a shell
  argument-array authoring error; its logs are not product failures.
- The follow-up root-degree test fails before the parser fix and passes after it. Nested optional degrees now
  use the existing parser/depth/node budget instead of a fresh parser on the first raw closing bracket; matrix
  cell admission happens before parsing/pushing the next cell. Hollow inheritance raster checks now cover all
  four directions and pass; the new capsule raster oracle is still being corrected/qualified.
- Independent pinned `flow.jison` review establishes `(-text-)` as ellipse and `[|field:value|text]` as a property
  production. Their former silent rounded/rectangle admission is reproduced, then rejected locally with an
  UnsupportedShape explanation. Quoted literal controls remain admitted. The earlier planning-only uncertainty
  about the ellipse spelling is superseded by this primary-source evidence.
- The paired harness rollback regression reproduces deletion of a later edit at an overlay-created path. The
  corrected runner records actual installed bytes, preserves/report later edits at created and replaced paths,
  retains original backups, continues unaffected restorations and passes all 34 `Test-PairedRun` cases. This
  changes the harness source identity; it does not change random assignment, metric bands or approve a policy.
- The full EditorControls diagnostic passes in `combined-14` (69.9 s). An earlier Embedded run ended with exit 3;
  its log and source/artifact hashes were preserved, but its matching executable/PDB had already been replaced
  by the next diagnostic build. A focused reproduction retained the exact executable/PDB and terminates at
  injected allocation 6/62. Offline symbol resolution identifies the new endpoint-group vector's Debug proxy
  allocation inside a noexcept default constructor. Explicit fallible empty construction restores the existing
  failure-atomic boundary. The expanded sweep passes seven parser scenarios and 322 allocation points, plus
  layout/paint/semantic/scoped-source sweeps. These are diagnostics, not final-profile qualification.
- The pinned Mermaid state implementation replaces the implicit ID on its first nonempty description and
  appends later descriptions. The new test fails before the correction and passes afterward for both orders
  of alias/colon declarations, repeated descriptions, an ID-equal description, empty descriptions and the full
  label limit. Class members retain their separate ID-plus-member rule. The complete Embedded diagnostic passes
  455,446 checks in 55.2 s, with zero C++ allocations in 1,000 warm composites.
- The next A7 diagnostic establishes 66,048 focus-resolution visits for 128 interested label mutations with
  512 unrelated siblings. Reusing the existing validated published-path helper reduces this to 1,792 visits;
  shifted paths fall back to live search and hidden focus stays unavailable. No new cache or dependency is added.
  This deterministic work result does not establish app latency/memory improvement or close R10.
- Broader review reproduces missing ProgressBar minimum/maximum accessibility events. A real subscribed client
  fails before the setter publication fix; its failed run and matching executable/PDB are retained. The fix
  passes its after-run in `combined-17`. Full tooling exposed two additional failures: the rollback repair changed the proposed
  unqualified assignment-source hash, and a nested test closure lost its fixture in composed invocation. Their
  failures are retained; updating the review target keeps `qualified=false` and all judgment rules unchanged.
  The 39 policy tests and composed ConsumerUpdate tests now pass.
- The reconciled inventory has 446 source-reviewed rows and 36 generated-provenance-verified rows, with no pending
  file rows. Hash equality alone does not complete review. All supplements are archived beside the ledger; lead
  delta review covers subsequent focus/range/lease changes and the unqualified policy identity correction. The
  [finding disposition](RedesignCompletionAndRendering_2026-10-24/review-disposition.md) separates confirmed fixes,
  refuted concerns, nonblocking maintenance and explicitly owned future work.
- The lease previously resumed a child even when job creation/configuration/assignment failed; injected denials
  reproduce this in `combined-17`. Required admission corrects those failures. A new real descendant fixture then
  exposes job accounting reaching zero before its process handle signals. Capturing verified member handles before
  terminating the job corrects the observed completion race; `combined-19` passes all 98 checks. This is not an atomic
  whole-process-tree join, and capture/wait failures remain failures. The first broader Accessibility invocation omitted
  the required `--no-activate` flag and failed its native-focus invariant; its log and exact binary are retained as an
  invocation error, separately from the governed rerun. Final-profile qualification remains open.
- The governed full Accessibility rerun passes in 123.6 s. Both repositories' validators pass; the installed CMake
  also passes the 12 triplet checks separately. SDK checkpoint `f5a2cea9ff798dcb0aa29a37766bcec460f41864` is clean and
  committed, its official x64 Release build passes, and RedTilde adopts that exact API-4 pin. No dependency is added.
- The first official Full run catches an obsolete termination source guard; the native lease self-test itself passes.
  Its correction permits only the creation-time-verified descendant fixture cleanup, keeps all other ownership bans,
  passes 7/7 and independently rejects unrelated-handle and removed-identity-barrier mutations. A stale build receipt
  is then refused as required. Final SDK pin `94bf9801f51a544422ecb8aa6ae86d2e33aadcb2` passes the official Release rebuild;
  its Full run passes all 19 requested suites with zero capability skips, retained in DxUi
  `.build/evidence/test-runs/20261024T181402400Z-48140-c06d963691f94f6e92c7b8b1155e002c`.
  RedTilde's final static/tooling validators and pinned formatter also pass. Both app baselines and the candidate
  restore that same pin and the same WIL package ABI.
  Their seven common performance-harness inputs match byte-for-byte; the original-UI header delta is CLI-only.
- App code checkpoint `fa0757d568652560ea05563d09ab3bb6f46298b8` passes all 54 native cases (Editor now 35),
  23 captures, recovery restart and isolated crash checks. Its nine keyboard/focus stages pass; the SDK's Menu,
  NativeTextInput, MenuResources and MenuResourceScaling pass with zero skips and verified desktop restoration.
  Independent reviewers verify receipts, executable hashes, catalog union and capture hashes. Exact Release binaries
  and PDBs are retained before further diagnostic builds. Later documentation does not rewrite those observations.
- Both fresh app cohorts complete on SDK `94bf9801`: original PR-base UI `b5e15cc0` versus `fa0757d`, and starting
  redesign `3128855e` versus `fa0757d`, each with separate 96-process A/A and 96-process A/B studies. The independent
  auditor passes 7,988 and 8,015 checks respectively. The original-base A/B still reports seven regressions:
  Compose/Split command time +16.77%/+20.48%, private bytes +12.23%/+12.68%, working set +4.63%/+4.61%, and
  Split owner CPU +11.62%. The incremental study resolves no directional change. All observations stay retained;
  controls are not uniformly stable and neither host window is verified quiet. R10 is still open.
- Continue memory attribution with a separately labelled diagnostic: three fixed Compose trials at 64 cycles for
  each of current raster behavior, bitmap preparation without drawing, WIC construction without bitmap preparation,
  and a control with no WIC/bitmap resources. Preserve every variant source/binary/receipt before replacement.
  These feature ablations cannot replace the primary study, prove a 4 MiB allocation owner by themselves, or
  establish timing/working-set acceptance while other builds run. No new dependency or profiling-policy bypass.
- The exact SDK94 scoped-tooling fixture passes 36/36 under portable official PowerShell 7.6.8 in Ubuntu 24.04,
  including the assertion that failed in the older hosted main run. Files were on the WSL NTFS mount; this is
  current non-reproduction, not an explanation of the historical failure or full hosted CI. A separate harness-only
  SDK checkpoint `8a38e598` corrects synthetic gallery tooltip capture ordering; its native verification is pending.
  Production `src`, `include`, `Build` and samples are identical to SDK94, so the app's exact runtime pin stays94.

### Current qualification and memory investigation — 24 October, 19:50 UTC

SDK draft [PR #100](https://github.com/RedSalamanders/DxUi/pull/100) carries `8a38e598` for hosted qualification.
Its production `src/include/Build/Samples` inputs are identical to the app's exact `94bf9801` pin; the later
commit changes only the gallery fixture and documentation. The current x64 Debug Full run passes all 19 normal
suites with zero capability skips. ASan Debug has passed its real isolated detection probe and is still running
its ordinary suites. Four of six current SDK build profiles pass. Hosted native jobs, including ARM64, are running;
they are not yet passes. Ubuntu validation found a broken sibling-checkout reference and the retained scoped
identity fixture failure. A separate isolated tooling checkout investigates that failure without mutating running
qualification inputs. The proposed policy remains unqualified and the PR remains draft.

The [reviewed performance projection](../../../Measurements/RedesignCompletion_2026-10-24/performance-fa0757d/README.md)
retains all 48 aggregate metric rows across both completed cohorts' A/A and A/B phases. It binds the original
comparison/audit hashes, preserves the actual-base lineage and starting-redesign distinction, and names excluded
private producer records. Neither current nor historical regressions were accepted or replaced.

Private staged diagnostics locate the retained memory difference after the first native frame: the added tree
accounts for about 0.2 MiB before rendering, while the original arm has one and the candidate two retained
4,198,400-byte private regions in three trials. Removing brand construction, icon text, DWM extension, outline
painting or the sidebar does not remove the private-byte difference. Omitting WIC construction consistently lowers
absolute working set by about 1.6–1.8 MiB, which warrants a separate module/pixel-path investigation. It does not
explain the extra private region. The initial brand control source representation was LF-normalized; a separate
correction retains the byte-exact original alongside its frozen executable without rewriting raw observations.

Per-control initial-frame submission reduces the retained private-region count to one in all three instrumented
trials. One submission before or after the editor, and both together, do not reproduce that reduction. Submitting
the initial frame after each visible top-level app surface does reduce final private bytes to about 44–45 MiB in
three further trials while retaining the full UI. These are unpaired diagnostic contrasts during concurrent SDK
work, not allocation-stack attribution or performance acceptance. Rendering policy remains owned by DxUi; no
diagnostic flush or feature-removal variant is a production fix. All modified production files were byte-restored;
the app output executable must be rebuilt from restored source before any new acceptance run.

Candidate-only edit-phase diagnostics retain three Compose and three Split trials. Ordinary prose edits perform
zero layouts; Split's mirror work and alternating dirty/title/count/undo states are required. The source review
and phase data have not established a substantial redundant operation that explains the original-base command
regression. Do not remove recovery freshness, source/history, accessible updates or visible context to improve it.

### Branding and qualification checkpoint — 24 October, 20:12 UTC

All six official SDK `8a38e598` builds pass with zero warnings/errors. Both current-head x64 Debug and ASan
Debug Full runs pass all 19 ordinary suites with zero skips; ASan's real use-after-free child is detected. Their
private receipts and the six-profile build matrix remain retained separately from performance acceptance.
Hosted x64 Debug and Release also pass. ARM64 runtime lanes and the hosted paired comparison are still running.
Hosted validation identifies a broken cross-repository Markdown link; the Ubuntu tooling fixture also exposes
an exact-case path-identity hazard. The focused correction preserves ordinal Git identities without changing
case-insensitive scope matching. A local reproduction has not established the complete hosted failure cause.

The production brand mark now retains bounded, premultiplied BGRA pixels from its own alpha icon instead of a
WIC converter. WIL owns the temporary bitmaps/DC; the generation-bound D2D bitmap still prepares outside Paint
and rebuilds after device loss. Release Editor passes 35/35, including independent WIC byte equality at
16/24/32/48/64/128/256 pixels and device-loss/warm-raster retention, in run
`20261024T200629511Z-52096-a8d90cadfef246058b36c087141dc788`. The first extended test incorrectly applied
the existing small-icon 25% ink threshold to the unchanged 128-pixel asset (23.5%); its failures remain retained.
The original 16/24/32 legibility oracle is unchanged, and full-byte conversion parity now covers all seven sizes.
Independent source review finds no concrete extraction/lifetime defect; this does not prove a full cross-version
raster/DPI matrix. Three unpaired memory probes are retained in `.build/qualification-20261024/brand-native-pixels`.
They are diagnostics, not a paired benefit or an explanation of the additional private region.

The successful top-level initial-frame submission diagnostic motivates a default-off, bounded, native-only
DxUi option. Its implementation and deterministic lifetime/error/order/recovery tests are in progress. No app
diagnostic flush is adopted as product policy. All final-source matrix, input and paired gates remain open;
the seven-regression cohort is preserved and no cost or policy has been accepted.

### Host cache and batching diagnostic checkpoint — 24 October

The retained `host-regressions-red-v2` run records 13 selected outcomes, including two expected-exit
mismatches; it is not an all-green result. The separate membership/reset red run matched its 9/9 expected
outcomes and restored the source afterward. The first production-green host run matched 12/13 expectations.
Its tab-reorder case did not actually reorder because the interaction landed exactly on the strict midpoint
boundary. The corrected fixture performs and asserts the page identity swap from index 0 to index 1; it passed
in the current-source green-v2 run.

The interaction-epoch red matched 2/2 expectations: the focus-only case exposed the unnecessary full-tree
scan, and the corrected tab case passed its actual identity-swap assertion. The green-v2 run then passed all 14
selected Release cases at 21:29 UTC against WindowHost source SHA-256
`866fb8fc62eb06756f65845e5130a10a8e8bb4fb3a805deba0ebd0ccc7713187`. SDK commit
`cfe875fd815b0a39336100df9bde8f29ea2f0aa0` is clean and committed; RedTilde now pins that exact API-4
revision in `Dependencies/DxUi.lock.json`. The x64 and ARM64 SDK matrix is running on this commit. The pinned
formatter passed; final `fullvalidate` is still running. The earlier `validate-host-final.log` records a bounded
all-six-validator/tooling pass at its then-current revision, not final validation of cfe. The app exact-pin
restore/build and resource harness have not run on cfe. This focused result does not close the native/platform,
consumer or performance gates. Keep the seven whole-feature regressions open; no cost or allocation cause is
accepted.

## Objective and starting point

Finish the accepted native writing experience, repair relevant defects in both repositories, simplify code where
the benefit is demonstrated, and bring math/Mermaid behavior under an explicit, substantially broader compatibility
and regression contract. Preserve one editable Markdown source and undo history through Compose, Source, Split,
local F2 source, file operations, recovery and all unsupported-content fallbacks.

| Checkout | Planned-at revision | State at inspection |
| --- | --- | --- |
| `D:\RedTilde` | `c7456854c8f964bb9ea067295e6435f99d426515` | clean `main`; redesign integrated |
| `D:\DxUi` | `ceffe76a429d15039ceff1f7dfae7e1d7126323e` | clean `main`; rich-text audit plan integrated |
| RedTilde dependency | `c600993861a337012e2633356c3d35c5e129bbf3`, API 4, target `DxUi` | exact lock; no implicit adoption of later main |

At inspection, `git diff c6009938..ceffe76a -- src include Tests` in DxUi is empty. That is source-diff evidence,
not an attestation that later builds, binaries or environments match earlier receipts. Verify those separately.
All following short paths are relative to the named repository. Read root/scoped AGENTS and local skills before
each implementation slice; source locations below were re-read against the revisions above.

Run first, before changing code:

```powershell
git -C D:\RedTilde status --short
git -C D:\RedTilde diff --stat c7456854c8f964bb9ea067295e6435f99d426515..HEAD -- RedTilde Common Tests Build Dependencies
git -C D:\DxUi status --short
git -C D:\DxUi diff --stat ceffe76a429d15039ceff1f7dfae7e1d7126323e..HEAD -- src include Tests ThirdParty Tools
```

Uncommitted in-scope changes also require review. Re-read changed excerpts, update this checkpoint and preserve
other people's work; do not reset an unrelated checkout. Work on focused `codex/` branches when implementation
starts. Keep independently reviewable fixes separate from geometry redesigns and performance work.

## Existing owners and completion boundaries

This is the execution coordinator for this request, not another copy of the 278-item DxUi audit register.

| Requirement | Existing owner | Use in this continuation |
| --- | --- | --- |
| Accepted appearance and R1–R9/V1–V4 | [Design refresh](DesignRefresh_2026-10-22.md) | preserve implemented behavior; A6 closes actual remaining defects |
| R10 performance/final qualification | Design refresh | A8 supplies evidence; R10 is a summary gate, not a separate optimization backlog |
| C1–C5/C11–C12 product qualification | [Creation plan](RedTilde_2026-10-20.md) | A6/A8/A9 execute the applicable gates and update the owner |
| C6 save race, C7 capacity, C8 typography, C9 compatibility, C10 assets | Creation plan | A0 classifies scope; C9 executes here; do not silently drop the other rows |
| Rich-text defects, simplification and language completion | [DxUi audit](https://github.com/RedSalamanders/DxUi/blob/33c3086cfbf7d02118775c2af2eb6f9eb05d0dbf/Specs/Plans/WIP/RichTextAudit_2026-10-21.md) | keep its W/T/C IDs; dated [current-source continuation](https://github.com/RedSalamanders/DxUi/blob/33c3086cfbf7d02118775c2af2eb6f9eb05d0dbf/Specs/Plans/WIP/RichTextAudit_2026-10-21/RedTildeCompletion_2026-10-24.md) supplies verified evidence |
| M5/M6, D4c and rich-editor product gates | [DxUi rich-text parent](https://github.com/RedSalamanders/DxUi/blob/33c3086cfbf7d02118775c2af2eb6f9eb05d0dbf/Specs/Plans/WIP/RichTextEditor_2026-10-17.md) | close milestones only with their own evidence |
| Shared platform/performance/consumer qualification | [DxUi qualification](https://github.com/RedSalamanders/DxUi/blob/33c3086cfbf7d02118775c2af2eb6f9eb05d0dbf/Specs/Plans/WIP/ConsumerUiQualification_2026-10-11.md) | no duplicate qualification ledger; exact-pin RedTilde adoption remains app-owned |
| Broader controls/host/tooling review | [DxUi production review](https://github.com/RedSalamanders/DxUi/blob/33c3086cfbf7d02118775c2af2eb6f9eb05d0dbf/Specs/Plans/WIP/ProductionReview_2026-10-05.md) | A0 revalidates relevant findings and all first-party files; preserve that plan's separately owned holds |
| File drop and other consumers | their existing plans | review relevant boundaries; no implicit RedXe/RedPrism/RedSalamander migration |

The design is already implemented. The [24 October integration packet](../../../Measurements/DesignRefresh_2026-10-24/README.md)
records a latest complete 192-process study with **seven regressions**. Its arms used SDK `cf1393a2`, not the
current `c6009938` pin; later optimizations are outside that cohort. Compose/Split changes were command time
`+13.75%/+5.60%`, private memory `+10.83%/+11.51%` and working set `+4.16%/+4.61%`. Owner CPU was
`+4.90%/+14.38%`; Compose CPU stayed below its band. Completed-edit/clean-frame timing had no resolved regression.
Controls and host activity limit attribution. Earlier v5r3's eight-regression result is distinct historical evidence.

The same packet records final-pin x64 Release **53 behavioral cases and 23 captures**. Older six-profile/image-2
packets use older pins. Neither publication nor this plan accepts a cost or qualifies final-pin Debug, ASan,
ARM64 runtime, physical input, hosted CI or release. R10 remains open until A8/A9 supply the required disposition.

## Review findings admitted for execution

These are source-backed findings or explicit coverage gaps, not results from a new native run. Reproduce each
defect with a regression that fails on the recorded original code before fixing it. Existing audit IDs remain
the implementation owners. S means hours, M roughly a day, L multiple days; estimates include tests.

| ID / priority | Evidence and consequence | Confidence | Effort / fix risk | Owner |
| --- | --- | --- | --- | --- |
| F01 / P0 | DxUi `src/Controls/DxUi.RichTextCommands.cpp:1918–1936` can use `slots[column - 1u]` with column zero in a short padded body row. The header's one-column guard does not protect that raw row. | High, static | S / medium | W1.1, commands-01; A1 |
| F02 / P1 | DxUi `DxUi.NativeMath.cpp:69–100` accepts newlines in text leaves; `DxUi.PreparedMath.h:332–340` rejects multiline/oversized geometry; `DxUi.RichTextEditor.cpp:1337–1347` can reject the document's semantic preview. One formula must not remove unrelated rich content. | High, static path | M / medium | W1.2; A2 |
| F03 / P1 | DxUi `DxUi.NativeMath.cpp:240–241,389–412` models limits/nolimits as a Space atom, so following scripts bind to an invisible atom. | High | M / medium | W1.6/C4; A3/A4 |
| F04 / P1 | DxUi `DxUi.PreparedMath.h:41–52,62–84,303–310` has upright default text and fixed scale ratios, without a display/text/script style model. Admitted ordinary variables, spacing and large operators do not have TeX semantics. | High, compatibility gap | L / high | C4/T1; A4 |
| F05 / P1 | DxUi `DxUi.RichSemanticDocument.cpp:341–345` writes a rejected formula's diagnostic into its parent's title, including a titled link. | High | S / low | W1.5; A2 |
| F06 / P1 | DxUi `DxUi.NativeFlowchart.cpp:253–277,290–298` replaces a state label for quoted aliases but appends colon descriptions to the ID. The pinned state documentation defines both description forms. | High | S / low | W1.7/T2; A5 |
| F07 / P1 | The same parser's generic Label validator at `69–119` is bypassed by state aliases/descriptions and transition descriptions at `253–277,290–298,342–348`. Markup/control/length policy can differ by syntax. This is a conformance defect; no execution of markup was observed. | High | M / medium | W1.7; A5 |
| F08 / P2 investigation | RedTilde `Common/Settings.cpp:120–123`, `Common/Recovery.cpp:124–127` ignore failed completion posts, but `Application.cpp:1992–2000` checks completion after each dispatched message. Queue exhaustion leaves queued wakeups; it does not by itself prove a shutdown hang. Characterize reachable failure before proposing a loop redesign. | Medium; unproved runtime defect | S investigation; any fix M / medium | A0/A2 app; creation C12 |
| F09 / P1 | DxUi `MarkdownInlines.h:1215,1230–1231` applies dollar whitespace/digit heuristics to explicit slash-parenthesis math too. Spaced slash math needs its own delimiter rules and code/HTML precedence tests. | High for heuristic reuse; precedence requires characterization | M / medium | W1.6; A3 |
| F10 / P1 | `ThirdParty/Markdown/profiles/DxUiMath1-target.json` says math is unimplemented and disagrees with effective limits; the Mermaid matrix marks state/class wholly unsupported despite native subsets. | High | S / low | W1.11/T1/T2; A3/A5 |
| F11 / P1 | Current Mermaid corpus asserts 13 Prepared and 92 Unsupported out of 105 original flowcharts; state/class cases mainly check status/counts. Math cases do not establish atom/style semantics. | High, test shape | M–L / low | T1/T2/T4; A4/A5 |

Load-bearing excerpts, retained for drift comparison:

```cpp
// DxUi.RichTextCommands.cpp:1933; column is semantic, slots are raw per-row cells.
if (column + 1u < slots.size())
    Add(slots[column].first, slots[column].last + 1u, L"");
else
    Add(slots[column - 1u].last, slots[column].last, L"");

// DxUi.NativeMath.cpp:240; these must become operator modifiers or reject.
if (name == L"limits" || name == L"nolimits")
    return Make(MathKind::Space);
```

The older audit's remaining 278 entries are **not all revalidated by this pass**. Its 241 confirmed, 16 disputed,
one uncertain and 20 refuted counts describe that earlier source. A0 must retain fixed/superseded/disputed/refuted
dispositions; an old finding is not automatically a current bug. Do not rerun refuted work as a feature request.

## A0 — establish review and test baselines

Priority P0; S–M for baseline/critical drift checks, L for the complete review; low risk. A0a is the source/pin,
critical-finding and baseline gate before implementation. A0b is the full file/finding sweep; it may continue
alongside focused A1–A6 work, but must finish before broad refactoring or a claim of complete code review.

1. Compare both revisions and dependency/source identities. Record actual available build receipts and failures.
   Use the [review inventory](RedesignCompletionAndRendering_2026-10-24/review-inventory.md): 441 tracked C++,
   script, build/workflow, JSON, resource and lock inputs at planning time. It excludes vendored corpora, docs,
   skills and ignored outputs. Its initial review state is pending; hashes are drift detection, not proof of review.
2. Review every first-party inventory row. Add `reviewer`, source hash, callers/ownership checked, relevant test
   names, findings and disposition. Review generated tables through generator/provenance plus boundary tests;
   inspect third-party code at its actual ownership/build/resource boundary. Add new/renamed inputs to the ledger.
   Set `reviewStatus` to `reviewed` or `generated-verified`, fill `reviewer`, `testIds`, `findingIds` and a nonempty
   `disposition`, and fill the four `reviewNotes` strings: `callers`, `ownershipAndLifetime`,
   `failureAndCancellation`, and `testCoverage`. Use an explicit reason where an axis is inapplicable. A pending
   row cannot be closed by assigning an owner alone. Tests may name an existing validator when the row is
   build/resource data rather than runtime code.
3. Partition the review: RedTilde storage/settings/recovery/files/images/crash; app state/chrome/navigation;
   DxUi document/commands/parsers; math; diagrams; semantic geometry/images; host/input/UIA; shared controls,
   rendering/resources; tests/build/tooling. Include failure/cancellation and teardown paths, not only happy paths.
4. Reconcile every historical audit row against current code. All confirmed data-loss/memory-safety defects are
   mandatory fixes. Optional simplifications need a measured or maintenance rationale. Route unrelated platform,
   other-consumer and product expansion rows to their named existing owners rather than deleting them.
5. Before code changes, run current affected explanation and a reproducible correctness baseline. Capture the
   paired Release baseline and semantic fixture before optimization. The semantic fixture must fit within the
   current 65,536-unit bound: about 1,000 short paragraphs, nested lists, a table, images, equations and a graph.
   Record exact bytes, work/owner turns to readiness, DirectWrite creations, retained/peak memory and cancellations.

Verify A0a from each checkout: `./Test-Changes.ps1 -Explain` produces the expected relevant scopes; current selected
native suites pass or each existing failure is retained with source identity; baseline fixtures/receipts are retained.
Verify A0b: the ledger has no unowned or pending row and every old finding has a current disposition. Full review
does not mean every module needs a code change or a new test that mirrors implementation.

Machine check for the eventual A0b completion, from RedTilde. This is expected to fail while the ledger is pending:

```powershell
$ledger = Get-Content Specs/Plans/WIP/RedesignCompletionAndRendering_2026-10-24/review-inventory.md -Raw
$data = [regex]::Match($ledger, '(?s)\x60{3}json\r?\n(.*?)\r?\n\x60{3}')
if (-not $data.Success) { throw 'Missing machine-readable review data.' }
$inventory = $data.Groups[1].Value | ConvertFrom-Json
if ($inventory.entryCount -ne @($inventory.files).Count) { throw 'Review inventory count changed without reconciliation.' }
foreach ($row in $inventory.files) {
    if ($row.reviewStatus -notin @('reviewed','generated-verified') -or
        [string]::IsNullOrWhiteSpace($row.reviewer) -or [string]::IsNullOrWhiteSpace($row.disposition) -or
        @($row.testIds).Count -eq 0) { throw "Incomplete review: $($row.repository)/$($row.path)" }
    foreach ($field in @('callers','ownershipAndLifetime','failureAndCancellation','testCoverage')) {
        if ([string]::IsNullOrWhiteSpace([string]$row.reviewNotes.$field)) { throw "Missing $field review: $($row.path)" }
    }
    $root = if ($row.repository -eq 'DxUi') { 'D:\DxUi' } else { 'D:\RedTilde' }
    if ((Get-FileHash -LiteralPath (Join-Path $root $row.path)).Hash.ToLowerInvariant() -cne $row.sha256) {
        throw "Review evidence is stale: $($row.path)"
    }
}
```

Also compare current tracked/new source paths with the declared inventory scope before this check; removed,
renamed and added files need explicit reconciliation. Never update a hash without repeating the affected review.

## A1 — repair source corruption and memory safety first

Priority P0; M–L depending on reproduced W1.1/W1.3 findings; medium risk. Requires A0a; does not wait for all of A0b.

- In DxUi, reproduce F01 with a multi-column header and a one-raw-cell body, deleting column zero. Cover leading
  and omitted pipes, empty/padded rows, last-column deletion, nested quote/list containers, LF/CRLF and one-column
  table removal. Assert exact resulting source, caret/selection, one transaction and exact undo/redo. Never index
  a previous raw slot without proving it exists; reject/stage atomically before applying edits.
- Revalidate the rest of W1.1 and W1.3. Preserve source punctuation, nested markers, EOLs and escaped pipes.
  Add each confirmed regression to the existing command/parser fixtures before fixing it. Test capacity at
  limit-minus-one/limit/limit-plus-one, including padded cells and reference/inline node admission.
- Use `src/Controls/DxUi.RichTextCommands.cpp`, Markdown block/inline units and the existing
  `Tests/Support/Support.Tests.RichSemantic*.h` planner fixtures. Extend retained native and embedded editor cases
  for the same commands; a planner-only pass does not prove input/history integration.

Verify: DxUi `./test.ps1 -Configuration Debug -Platform x64 -Suites EditorControls,Embedded,Accessibility`, then
Release and ASan Debug for the affected suites. The original regression must fail against original code and pass
with the fix; preserve that evidence without crashing a person's running application.

## A2 — failure containment, ownership and reliable completion

Priority P1; M–L; medium risk. Depends on A0a; may run alongside A1 in disjoint files while A0b continues.

1. Implement existing DxUi W1.2/W1.5 with explicit domain-spec amendments. A malformed/unsupported/capacity-limited
   formula or graph retains its exact source locally. Unrelated paragraphs, images and other objects remain rich.
   Persistent preparation failure is memoized by the complete source/profile/font/width/direction/resource/binding
   key; device loss/resource generation changes retry through the proper lifecycle. Do not cache cancellation or
   a transient device failure permanently. Preserve allocation-failure policy at library ABI boundaries.
2. Create an owned semantic Math element for both success and rejection; diagnostics attach to it. Preserve
   parent link title/destination and its UIA HelpText. Share the native-object admission ledger for images/math/
   diagrams only after their different lifetime/resource semantics are made explicit.
3. Inject a formula-local unsupported native operation and repeated idle turns. Assert one rejection per complete
   key, stable source/history/selection and source/caret map, local fallback beside other rich content, and recovery
   after a changed key. Separately inject transient factory/line-metrics errors and allocation failure: preserve
   the original HRESULT and permit same-key retry. Image upload/device-target failures belong to resource recovery,
   not a persistent formula rejection cache; exercise their retry and resource-generation sweep. Include a
   newline/control in math text and an oversized prepared object in otherwise valid Markdown.
4. In RedTilde, investigate F08 before changing the loop. Inspect `Application::Run`, BeginClose/FinishClose,
   SettingsWriter and RecoveryService together. Queue exhaustion has an existing dispatch backlog and therefore
   does not demonstrate a missing wake in the current loop; a fake post failure with an empty queue alone does
   not establish a reachable Windows defect. Characterize both completion orders, the final service to finish,
   queue-pressure recovery, close/cancel and service errors. Record a refutation if the existing checks suffice.
   The [PostMessage contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew)
   documents queue-limit failure and retrieval through GetMessage/PeekMessage. If a reachable failure is proved,
   use a WIL-owned completion event/message-loop integration or another proven bounded wake protocol; no polling
   timer, UI-thread join or post to a retired HWND. Test event reset/lifetime and finished-before-wait races.
5. For a justified completion repair, extend hidden process-exit tests in `Tests/Application.Tests.Core.cpp` and
   app design tests. Assert exact durable settings/recovery contents and real child-process exit. A Busy=false
   assertion or a post-failure hook that cannot occur in production is insufficient defect evidence.

Verify: DxUi A1 command plus resource/fault cases under Embedded; RedTilde
`./test.ps1 -Configuration Debug -Platform x64 -Suites Settings,Recovery,Preferences,RecoveryRestart,Editor`,
then Release/ASan Debug. No activation or real clipboard is needed for these regressions.

## A3/A4 — KaTeX compatibility with correct native mathematics

Priority P1; A3 M, A4 L; medium/high risk. Existing owners W1.6/W1.12/T1/T4/C4/C5.

Execution scope selected by the owner: finish and qualify the current bounded profile. Full TeX font assemblies,
MathKern/variant expansion and other excluded notation in the planning direction below stay with the existing
language roadmap. Their exclusion remains explicit in the supported/target data and prevents a full KaTeX claim.

Implementation anchors in DxUi: `DxUi.NativeMath.cpp` Parser::GroupText/Matrix/Atom/Row and ParseMath own grammar;
`include/DxUi/RichMath.h` owns public data/limits; `DxUi.PreparedMath.h` PreparedMath::Gather/Measure/Place/Advance
own native preparation/geometry. `DxUi.RichSemanticDocument.cpp` owns semantic Math elements;
`DxUi.RichSemanticGeometry.cpp` owns document placement and local prepared-object fallback. Keep parser atoms
independent of device resources. An internal lowering step can add atom/style layout data without exposing a
new public renderer; preserve every source association through that lowering. Review public compatibility before
changing MathNode/MathExpression rather than moving app policy into the library.

**Reference identity:** retain KaTeX 0.16.11 corpus commit `fbbea3573439d51e6568356f5886053b111b1f88` as the
current regression baseline. Live official docs inspected on 24 October identify 0.19.1; a new reference is a
separate versioned profile and corpus migration, never an unnoticed update to old expected output.

1. Fix explicit slash delimiter rules independently of dollar currency heuristics. Characterize closer precedence
   against code spans, escapes, HTML, links and container prefixes. Preserve exact source ranges across LF/CRLF,
   surrogate pairs, display envelopes and over-limit fallback. Add raw source/result assertions, not object counts.
2. Correct symbols and parser/admission defects from W1.6. Limits/nolimits modify an operator; until implemented,
   they must reject honestly rather than parse as invisible bases. Check budget before allocation/push and keep
   one shared node/depth/source budget through roots, groups and environments.
3. Introduce internal TeX atom classes Ord/Op/Bin/Rel/Open/Close/Punct/Inner, explicit text versus math mode, and
   display/text/script/scriptscript style. Model unary/binary reclassification, named upright operators, math
   italics, spacing and italic correction before increasing the command list. Preserve public source compatibility;
   any required public layout/API change follows the API-revision and exact consumer-pin contract.
4. Use measured glyph and OpenType MATH data where available for axis, script/fraction/radical clearances,
   operator limits and stretchy glyph assemblies. Define deterministic missing-table/glyph/font fallback and
   geometry bounds. Replace hard-coded proportional layout only with tested rules. Cambria Math presence is not
   a substitute for handling font fallback. Preparation owns resources; composition creates none.
5. Implement display versus inline placement, centered display envelopes, nested style propagation, fractions,
   roots, simultaneous scripts, large operators/limits, accents and delimiters. Then expand named operators,
   class-annotated symbols, binomials/over-under constructs, fonts and environments from existing C5. Annotation,
   colors, numbering/tags and trusted functions get explicit profile rows and resource/policy decisions.
6. Generate a command/environment compatibility manifest from pinned support data. Every row names accepted,
   partial, unsupported, malformed or policy-excluded behavior, an example, AST/geometry expectations, diagnostic
   and test IDs. Rejecting everything cannot satisfy an advertised supported category. Keep safe-language parity
   distinct from unavailable macros/packages/HTML/actions and from universal LaTeX compatibility.

| Math family | Required independent assertions |
| --- | --- |
| Variables, numbers, operators and spacing | atom kinds, default italic/upright choices, unary/binary cases, relation/punctuation spacing and source ownership |
| Styles and scripts | inline/display differences, style overrides, nested superscript/subscript sizes, baseline and limits/nolimits placement |
| Fractions, roots, accents, delimiters | glyph origins, axis/clearance, ink bounds, scalable assemblies, explicit sizing and missing-glyph fallback |
| Matrices/cases/aligned/gathered | rows/cells/alignment, empty cells, trailing row separators, mismatched rows/environments, line/column source ranges |
| Parser/resource limits | 4,096 source units, 512 syntax nodes, depth 24, at most 16 rows/16 columns and 128 effective cells; boundary and allocation/cancellation sweeps |
| Invalid or excluded notation | malformed groups, unknown commands, controls, forbidden resource/process/HTML/macro operations; exact local source and useful diagnostic |
| Integration | math in headings/links/lists/tables, mixed RTL, hit/caret, F2 apply/cancel/undo, UIA children/name/help, document and read-only inspector |

These are current effective limits, not proposed increases. `RichMath.h`'s 16x16 wording and the target profile
must be reconciled with the parser's 128-cell limit. Change a limit only with explicit measured capacity coverage.
Add a stable AST serializer test oracle and prepared glyph/line placement oracles, including operators whose
accepted output was wrong. Run the same fixtures native/embedded at 96/144/192 DPI; add fractional-DPI rule and
clipping cases at 120/168. Raster checks supplement semantic/geometry tests and account for pinned font/OS identity.

Use existing `Tests/Controls/Controls.Tests.RichSemanticDocument.cpp`, `Controls.Tests.RichSemanticGeometry.cpp`,
`Controls.Tests.RichTextEditor.cpp`, `Controls.Tests.RichTextAccessibility.cpp` and Embedded resource fixtures; add
`Tests/Support/Support.Tests.MathCorpus.h` only if needed and register new active files. Tests must fail for wrong
spacing/limit placement and parent-title corruption, not merely for parse failure.

**MicroTeX decision:** recommended path is to retain the owned bounded engine and borrow reviewed ideas. The
[pinned MicroTeX row implementation](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/src/atom/atom_row.cpp)
shows atom classification, unary/binary adjustment and font kerning;
its [delimiter factory](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/src/box/box_factory.cpp)
illustrates larger glyph/extension assembly. Its [README](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/README.md)
documents C++17/CMake and a GDI+ Windows path; its [main license](https://github.com/NanoMichael/MicroTeX/blob/0e3707f6dafebb121d98b53c64364d16fefe481d/LICENSE)
is MIT. The owner's implementation constraint excludes direct adoption. Reimplement the useful principles
with the existing native stack; do not import code, font packages, mutable global state or backend assumptions.

Verify after each slice: DxUi A1 suites, deterministic corpus generation/checks, all original 1,324 Markdown
vectors, every chosen profile row and the resource sweeps; then RedTilde Math/Editor integration through A6.
The [KaTeX supported functions](https://katex.org/docs/supported.html) and
[options](https://katex.org/docs/options.html) are behavior references, not a product JavaScript dependency.

## A5 — Mermaid behavior, layout and broader coverage

Priority P1; L; medium/high risk. Existing owners W1.7/T2/T4/C6/C7/C8.

Execution scope selected by the owner: finish and qualify the current native flowchart, limited state and limited
class profiles. New diagram families and excluded grammar below remain future direction for the existing language
roadmap; no current-profile closeout can claim complete Mermaid support.

Implementation anchors in DxUi: `DxUi.NativeFlowchart.h/.cpp` NativeFlowchartParser::Preparation::Label/Node/
Statement own current grammar/model; `DxUi.NativeFlowchartLayout.h/.cpp` own CPU placement/routing;
`DxUi.PreparedFlowchart.h/.cpp` own measured text and native retained drawing. Semantic document/geometry
and SemanticExtensions integrate objects with source, diagnostics, admission and UIA. Extend the correct layer;
new families must not accumulate family-specific hacks inside generic flowchart label parsing.

**Reference identity:** keep Mermaid 11.4.1, commit `9868f3a4c3784e214d8b6c74277ca542774e7de9`, its 105
original flowchart examples and family inventory immutable as the regression baseline. Current live docs show
12.2.0 and changed defaults. Record any upgraded parser/layout version, theme, fonts and configuration in a new
oracle packet; never compare an unpinned live renderer to native output and call the difference a defect.

1. Correct state alias/colon equivalence, shared label validation, direction token whitespace and diagnostics.
   Reject flowchart-only shape/edge syntax inside state/class profiles unless that family's grammar admits it.
   Make relation kind explicit rather than inserting a synthetic inherits label. Check titles, members,
   transitions, escapes, shorthand styles and accessibility statements; preserve source fallback for exclusions.
2. Replace count-only expectations with independent normalized model goldens: family, direction, stable node IDs,
   labels, shapes, edge endpoints/kinds/labels, declarations and diagnostics. Check expected issue/reason, source
   range and no partial model on rejection. Cover accepted input, invalid input and valid out-of-profile input
   separately. Update family status data and generator together; a state special-case in an unsupported-family
   test is not the final compatibility design.
3. Prioritize flowchart parity: chained/fan-out edges, edge styles/text, shape coverage, quoted/multiline/entity
   labels, subgraphs/directions and accessible titles/descriptions. Native theme translation must be explicit;
   arbitrary CSS, scripts, actions and directives stay non-executing. Unsupported forms retain the whole fence.
4. Complete state and class semantics: aliases/descriptions, start/end/pseudo states, composites and notes;
   class blocks/compartments, relationships, cardinality, namespaces and generics. Then add sequence diagrams
   with participant/message/activation/loop/alt semantics. A new family needs its own typed model and bounded
   layout, not reinterpretation as a flowchart. W5 adapters may help, but a whole-engine rewrite is not prerequisite.
5. Continue the safe family matrix with ER, pie, gantt, mindmap, then every remaining pinned family. Each gets
   a prioritized implement/defer row with reason, estimated effort and tests. The overall compatibility goal
   stays open while requested families are deferred; report the actual implemented profile, never full Mermaid.
6. Repair layout with shape-aware ports, rank-cycle handling, independent edge lanes, crossing reduction,
   label wrapping/obstacles, self-loops, parallel/back edges and disconnected components. Bound work/memory;
   no over-wide object should silently invalidate unrelated content after A2's spec amendment.

| Mermaid test axis | Required scenarios and oracles |
| --- | --- |
| Original corpus | run all 105 flowcharts and every pinned family fence; zero omitted rows, explicit per-case outcome/reason and immutable provenance |
| Grammar and metadata | equivalence of aliases/descriptions, whitespace/comments/quotes/escapes, state/class members and edges, unsupported directives/styles/HTML/actions |
| Graph semantics | independent ID/label/shape/edge-kind goldens; duplicate declarations, missing targets, Unicode/surrogate/control boundaries and family isolation |
| Layout | all four directions, cycles, loops, fan-out, long/multiline labels, parallel/back edges, subgraphs; finite nonnegative bounds, ports on the actual shape, no unrelated-node intersection |
| Resources | source/node/edge/statement/identifier/label caps; limit boundaries, cancellation at each stage, allocation failure, replacement peaks, width/font/DPI/device changes and idle reuse |
| Source/editor/UIA | list/quote fences and CRLF, complete source spans/diagnostics, object caret/hit/selection, F2 apply/cancel/undo/redo, exact copy/source fallback and accessible alternatives |
| Rendering | native/embedded retained resources, shape-specific raster assertions, theme/high contrast/RTL, 96/144/192 and fractional DPI, WARP/device loss; approved upstream references with pinned fonts |

Use `Tests/Support/Support.Tests.NativeFlowchartCorpus.h`, `Support.Tests.MarkdownBlocks.h`,
`Support.Tests.FlowchartLayout.h`, `Support.Tests.PreparedFlowchart.h`, `Support.Tests.SemanticDiagrams.h`, and
the RichMarkdown/SemanticDocument/SemanticGeometry/RichTextEditor control suites. Register added state/class/
sequence corpus fixtures in `Tests/native-test-files.json`. New generator/tool changes follow Tools governance,
inventory/help/callers and offline deterministic tests; do not modify generated expected data by hand.

**Reference-project use:** [mermaid-rs-renderer](https://github.com/1jehuang/mermaid-rs-renderer/tree/3726ccbffe0e8032361eb9668694b24f77858060)
is a useful native-rendering comparison and algorithm source, not a conformance authority. Its
[manifest](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/Cargo.toml)
identifies Rust 2024, version 0.3.1 and MIT. Study its
[stage validation](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/src/layout/flowchart/stage_validation.rs)
for missing/invalid nodes, ports and routes, and
[route-label obstacles](https://github.com/1jehuang/mermaid-rs-renderer/blob/3726ccbffe0e8032361eb9668694b24f77858060/src/layout/flowchart/route_labels.rs)
for label clearance. Implement only the reviewed useful ideas with DxUi's ownership/source/UIA model.
No Rust/FFI, SVG rendering backend or new package is admitted by this plan.
Do not inherit its advertised speedups as evidence for this product.

Verify each accepted family with the A1 library suites and named corpus cases, then A6 consumer tests/captures.
Semantic equivalence does not require identical browser pixels or coordinates. Correct labels, relation meaning,
connectivity, source access, readability and tested layout invariants are mandatory. Latest-only syntax, including
newer shape/header aliases, needs a versioned probe before being reported as a defect in the 11.4.1 profile.

## A6 — finish the consumer design and interaction

Priority P1; M–L; medium risk. Preserve the accepted target rather than inventing a second visual direction.

- Review `RedTilde/Application.Chrome.cpp`, `Application.Navigation.cpp`, `Application.cpp`, `NavigationModel.h`,
  `UiControls.h` and canonical DxUi Button/Tree/Menu/SingleLineTextEditing/RichTextEditor behavior. Use public APIs;
  the library owns shared interaction, layout and resource fixes. App strings stay in `res/RedTilde.rc`.
- Verify merged caption drag/resize/maximize/system menu, view exclusivity, dirty identity, Save state, keyboard
  hints, overflow and disabled commands. At 800x560, collapse inspector before overlaying Files; never overlap
  controls. Cover long names, long localized strings, high contrast, reduced motion, RTL and text scaling.
- Test selection retention and focus return across formatting, menus, Find/Replace, Settings, Files and F2.
  Test the actual key-message pipeline so Enter/Space cannot insert a stray editor character after button
  activation. Preserve the existing fixed regression. Hidden tests need no desktop focus.
- Extend existing `Application.Tests.DesignRefresh.cpp` equation/source scenarios and
  `Application.Tests.Core.cpp` Math/Editor cases with new compatibility examples. Compose/Source/Split/F2 must
  preserve source, selection, read-only mirror, cancellation, history and recovery, including rejected objects.
- For Files, retain bounded depth/visited-entry limits, no reparse traversal, stale request rejection, filter,
  dirty prompts and the normal Open flow. For source/storage, retain BOM/EOL/UTF-8 rejection, no truncation,
  successful-save dirty reset and recovery Save As. C6's check/replace race remains an explicit storage workstream;
  an extra stamp check cannot be called an atomic conditional write.
- Adopt only a reviewed immutable DxUi revision through `Dependencies/DxUi.lock.json`, restoring via the single
  library targets. Run consumer qualification after each final pin; never copy shared sources or private headers.
- Capture the weekly report and actual math/Mermaid examples through `Application.CaptureScenarios.cpp` and
  `./test.ps1 -Suite Captures`. Keep invented fixtures and scenario/build/pin provenance. Inspect 96/144/192 DPI,
  narrow, light/dark/System/high-contrast, selection/find/focus/settings/F2/fallback states. Proposal art stays labeled.

Verify: RedTilde Core and Captures in x64 Debug/Release/ASan Debug, actual frame output plus behavior assertions,
and current-source UIA client checks where the accessible tree/event is the requirement. A visual match alone
does not qualify source/history, keyboard, physical monitor DPI, IME, touch or assistive technology.

## A7/A8 — simplification and measured performance closeout

Priority P1; L; medium/high risk. Depends on characterization and failure containment.

| Simplification | Order and acceptance |
| --- | --- |
| Reuse unchanged semantic geometry | W3.1 first; full key includes source/block identity, runs, tables/objects, width, direction, typography and resource generation; unchanged output/source oracles and fault-safe publication |
| Incremental parsing and scheduling | W3.2/W3.3; reuse only proven closed blocks, whole-parse fallback, bounded work/cancellation, exact old/new corpus equality |
| Images and native objects | W3.4/W3.5; retain valid decodes/prepared CPU objects across unrelated edits/reflow, separate device resources, reject stale tokens and avoid retaining old large documents |
| Shared utilities | W2 helpers only after searching canonical helpers; consolidate proven identical source/EOL/lexical/scanner semantics with caller tests |
| Lifecycle/diagnostic consolidation | W4/W5 only after profiling/characterization; adapters first, keep per-type ownership and source mapping, no speculative universal framework |
| Consumer structure | extract focused app state/presentation seams only where A0 finds duplication or unsafe coupling; no second editable model or parallel Files/Find cache |

Measure allocation ownership before claiming a cause for R10. Retained icon/text layouts and analysis caches
already have fixes; do not remove them or silently rebase the benchmark. Inspect app source copies/analysis,
editor parse/build/geometry stages, control layouts, SDK allocations and teardown peaks separately. Profiling
instrumentation may perturb results; diagnostic ablations never replace the fixed primary cohort.

Run two distinct app comparisons: correction work versus this continuation's starting product, and original
UI versus redesigned UI on the same final SDK/common harness for R10. Retain original-UI source and bring only
the common measurement harness/pin into that baseline. Do not use the already-redesigned main as evidence that
the redesign itself has no cost. Library changes also need their own paired DxUi comparison against its real base.

RedTilde command protocol, after the baseline has been built and committed in an approved repository-local
checkout and both arms use the same final SDK/common harness:

```powershell
# From the baseline RedTilde checkout, before candidate optimization:
./build.ps1 -Platform x64 -Configuration Release
./performance-paired.ps1 -FreezeBaseline -OutputRoot .build/redesign-baseline-20261024
# From the candidate; use the absolute frozen baseline path returned above:
$baseline = 'D:\RedTilde\.build\redesign-baseline-20261024'
./performance-observed.ps1 -Baseline $baseline -OutputRoot .build/redesign-final-20261024 -Blocks 12 -Cycles 64 -Seed 20261024
./Tests/Scope.Tests.Performance.Evidence.ps1 -SelfTest
./Tests/Scope.Tests.Performance.Evidence.ps1 -StudyPath @('.build/redesign-final-20261024/aa','.build/redesign-final-20261024/ab') -Output .build/redesign-final-20261024/audit.json
```

The sample absolute baseline assumes that checkout owns that exact root; substitute the actual returned path.
Every output directory must be fresh. Keep attempts separate; preserve source/compiler/dependency/fixture/render/
host identities, all A/A/A/B slots and refusals. Existing judge bands/families remain unchanged. Qualify hidden idle,
clean/dirty, long sessions, cancellation, close, resize, device recreation and actual device loss separately.

For DxUi use its existing `performance-paired.ps1 -BaselineRevision <recorded-base> -CandidateRevision <candidate>
-Configuration Release -Platform x64 -Blocks 12 -Seed 20261024`; run `-CalibrationAA` with identical explicit
base/candidate revisions separately. The placeholders must resolve to committed local refs; the tool rejects
incompatible controls. Record actual commands in the run ledger. Hosted paired checks, shared D4c and app R10
acceptance are separate decisions. Unstable controls, regressed costs or CI_PENDING cannot become a pass by wording.

Exit: deterministic work/resource tests pass; unchanged-output tests survive optimizations; complete paired
studies and independent audits exist; regressions/inconclusive controls have been resolved or an explicit
owner cost/scope decision is recorded under the owning contract. Do not infer that the owner's earlier merge
request accepted these costs. Retain performance-advice options with measured impact and uncertainty.

## Verification commands and qualification matrix

Commands below were checked against script parameters and test inventories during planning. They were not run
as native qualification in this pass. Execute from the indicated repository, checking `$LASTEXITCODE` after each
native/tool process. Do not continue a loop after a failed profile and report the last profile as the whole result.

| Purpose | Command | Required result |
| --- | --- | --- |
| Both: iteration | `./Test-Changes.ps1 -Explain`, then `./Test-Changes.ps1` | expected affected scopes, no unexplained failures; partial evidence labeled |
| Both: static/tooling | `./validate.ps1`; `./format.ps1 -Check` | all applicable validators pass, no formatting drift |
| DxUi: targeted engine/host | `./test.ps1 -Platform x64 -Configuration Debug -Suites EditorControls,Embedded,Accessibility,Control,Tree,WindowHost,Rendering` | every selected suite/case passes, zero capability skips; add affected Theme/Animation/other suites when explanation requires |
| RedTilde: app | `./test.ps1 -Platform x64 -Configuration Debug -Suite All` | all requested scopes/captures/crash children pass; original failures retained |
| Both: x64 qualification | repeat full applicable tests with `Debug`, `Release`, `'ASan Debug'`; DxUi `./test.ps1 -Full`, RedTilde `./test.ps1 -Suite All` | three distinct source-bound runtime results; real ASan detection probe succeeds |
| Both: six builds | `./build.ps1 -Platform <x64 or ARM64> -Configuration <Debug or Release or ASan Debug>` for all six pairs | six current-source build receipts; ARM64 cross-build is a build result only |
| DxUi: documentation/gallery | `./gallery.ps1 -PublishDocs` after library visual changes | reviewed affected themed sheets and public design/usage docs agree |
| DxUi: PrePush | `./Test-PrePush.ps1 -Explain`, then `./Test-PrePush.ps1` when hosts/CI are available | account for six profiles; native-host gaps/CI_PENDING remain visible |
| RedTilde: PrePush | `./Test-Changes.ps1 -Mode PrePush -Explain`, then `./Test-Changes.ps1 -Mode PrePush` per required profile | applicable local/PR obligations fulfilled or explicitly pending |
| Both: changes | `./Tools/Test-Changelog.ps1`; `./Tools/Fold-Changelog.ps1`; `./Tools/Fold-Changelog.ps1 -Check` | valid notes, generated changelog current, no pending fragments |

Interactive commands are deliberately separate. Honor any still-applicable agreement to the time and existing
warning/lease; a historical successful run alone is not a present scheduling agreement. RedTilde uses
`./test-interactive.ps1 -Platform x64 -Configuration Release -SkipBuild`. DxUi uses
`./test.ps1 -Platform x64 -Configuration Release -SkipBuild -Interactive -Suites Menu,NativeTextInput,MenuResources,MenuResourceScaling`.
Do not add automatic Start without the corresponding explicit agreement. Record foreground/focus/pointer
restoration and preserve refusals. Physical IME, touch, AT, monitor DPI, actual device removal/occlusion and
native ARM64 execution are independent gates; a synthetic event, simulated DPI or cross-build does not close them.

Update `Tests/NativeInventory.json`, `Tests/native-case-catalog.json` and `Tests/test-scopes.json` in RedTilde,
and `Tests/native-test-files.json`/`Tests/test-scopes.json` in DxUi for new cases/files. New files follow their repository's real component context (`Controls.Tests`, `Support.Tests`, or `Application.Tests`) and inventory. In particular, confirm Rich/public-header changes select Accessibility as well
as EditorControls/Embedded; the existing audit's T3 records that scope gap. Use meaningful user-visible/source/
geometry/resource oracles, independent expected data and failure injection instead of implementation-mirroring tests.

## Alternatives and decisions carried into execution

| Choice | Default and tradeoff | When to reconsider |
| --- | --- | --- |
| Owned native engines | extend the existing C++ pipeline and use references for tested algorithms; preserve bounded source/edit/UIA ownership | implementation choice is fixed by the owner: no new dependencies |
| MicroTeX reference | derive atom, box, style and delimiter ideas, then independently implement and test them | reference algorithms must fit the existing ownership/resource contracts |
| Mermaid renderer references | derive parsing, stage-validation and routing test ideas in the existing native renderer | reference behavior is checked against pinned Mermaid semantics; no backend adoption |
| Full parity versus a named profile | overall goal is broader safe compatibility; each supported slice needs semantic/render/editor/resource tests | any excluded family/command must stay visible in the matrix; full compatibility cannot be claimed while it remains |
| Large documents/typography/assets | retain C7/C8/C10 ownership; review their current state and necessary public API seams | explicit product target and paired capacity/resource evidence; no arbitrary constant increase |
| Save-race guarantee | C6 must state an achievable cooperating/noncooperating-writer contract | do not invent compare-and-swap from a second file-stamp check |

Red-team every slice: try to refute the bug, introduce a known-wrong implementation to verify the oracle,
force stale generation/cancel/teardown, inject every observed allocation failure for ownership transitions,
and verify that a rejecting fallback does not mask incomplete promised support. Re-run only checks justified
by changed inputs or an unresolved concern; preserve exact reusable evidence under the repository contract.

## Rejected claims and review limits

- A generic ban-pattern search is not a defect report; COM provider implementations can legitimately implement
  AddRef/Release. Review actual ownership rather than mass-replacing them.
- The Mermaid direction-substring expression is not established memory corruption. Whitespace behavior needs
  grammar tests; default-direction/new alias differences need a pinned-version comparison.
- The initial ellipse uncertainty was superseded during execution by pinned `flow.jison` evidence and a failing
  admission regression. The unsupported ellipse/property-vertex guards now have source-bound diagnostic coverage.
- App shutdown already checks completion after each dispatched message. The agent's queue-exhaustion hang claim
  is not established because a full queue supplies dispatch work. F08 is an investigation; do not schedule a
  message-loop rewrite solely because a PostMessage return value is ignored.
- A capture, 13 accepted graphs, 1,324 Markdown vectors, older audit count or a green Release build cannot prove
  whole-language compatibility, coverage completeness or current six-profile product qualification.
- The three initial focused agent audits were planning evidence only. Execution subsequently completed all 482
  inventory rows with full source/provenance notes; runtime and performance qualification remain separate gates.

## A9 — completion and handoff

All must hold before calling the requested work finished:

- [ ] Every first-party review-inventory row has a current hash, reviewer, relevant tests and finding/disposition after P10 changes; the `83d3b1a` 536-file audit remains retained as prior evidence.
- [ ] Every mandatory confirmed defect has a fail-before/pass-after regression; disputed/refuted rows retain evidence.
- [ ] Supported Mermaid and math profile rows pass independent semantics, geometry/rendering, source/history,
  UIA, negative/admission, cancellation and resource tests. Deferred parity remains named and prevents a full-parity claim.
- [ ] Accepted RedTilde appearance and contextual workflows have current-source hidden captures and behavior coverage.
- [ ] Reviewed immutable DxUi pin restored/built independently; consumer storage/edit/recovery/resource tests pass.
- [ ] Six builds, x64 runtime profiles, ASan detection, applicable WARP/device/resource and native ARM64/input gates
  have exact evidence or an explicit allowed disposition. CI checks belong to the actual candidate bytes.
- [ ] R10, DxUi D4c and shared qualification retain their owning verdicts; no regression was silently accepted.
- [ ] Durable changes are in RedTilde Editor/Core/UI/Testing specs and DxUi Controls/Input/Performance/Build/Testing
  specs, user docs, design/gallery and `capabilities.json`. Plans do not become the sole normative home.
- [ ] Changes fragments are folded/checked in both repositories; no generated build products or private environment
  data are published. Move completed plans to Done and update indexes in the same closeout change.

Stop the affected slice and report if source drift invalidates an excerpt, a regression cannot demonstrate the
claimed defect, a library change needs an unplanned public ABI revision, a reference requires unsafe evaluation,
a resource limit cannot be kept finite, a baseline cannot satisfy identity/control requirements, or a verification
fails twice after a reasonable repair attempt. Continue independent authorized work while an external/native/input
gate is unavailable. Never mark a blocked gate complete, weaken its assertion or erase the original failure.

Future maintainers: a new admitted object/command/family must extend its compatibility row, source/geometry/UIA
oracles and lifecycle tests together. A font/theme/DPI/device change invalidates the appropriate prepared key.
Adding support must not create an alternative editable authority, leak stale object resources or add work to Paint.

## Planning verification — 2026-10-24

- RedTilde `./validate.ps1`: PASS, including specification/index, scope, evidence-retention, changelog and
  performance-protocol tooling checks. Final plan references/index were rechecked after edits.
- DxUi `./validate.ps1`: PASS, all five validators and the complete tooling suite. This includes offline
  Markdown/Unicode/flowchart generation checks; it is not a native renderer or new compatibility run.
- Both `./format.ps1 -Check` and final changelog fold/check: PASS. Affected explanations select documentation
  tooling/formatting only; no executable scope rules were weakened or changed.
- Inventory: 441 unique existing paths with matching source hashes and explicit pending review notes; full
  review remains open. All five PowerShell plan snippets parse. Independent cold review's A0 evidence gap was
  corrected and its follow-up found no remaining actionable issue.
- Native builds, app/runtime suites, interactive captures, paired performance and remote CI: NOT RUN for this
  planning-only change. No implementation or product-qualification checkbox is closed by the checks above.
