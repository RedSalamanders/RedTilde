# Review disposition — 25 October 2026

## Current checkpoint

The last completed native snapshot is RedTilde `fd24623` with DxUi `cfe875fd`. Both repositories pass six
local builds and all ordinary x64 Debug, Release and ASan runtime profiles, with real ASan detection probes.
The owner-started app keyboard and four SDK foreground suites pass with desktop restoration. All six SDK
hosted Full native profiles pass; optional per-profile instances and compiled checkout identities retain their
own receipts. RedTilde's hosted run never started because of the account payment/spending restriction. The owner
explicitly chose to keep that native ARM64 gate open. Cross-builds do not close it.

Fresh app cohorts retain 384 native runs and 16,003 independent checks. Against the actual starting redesign,
all twelve A/B slots have no new regression flag. Against the original UI, five flags remain: Compose command
+12.54% and private memory +2.013%; Split command +14.58%, owner CPU +13.44% and working set +2.61%.
Only the flagged Split working-set control is stable. These observed-load results supersede the earlier seven
flags without accepting the remaining costs or attributing the retained allocation. The separate sixteen-run
diagnostic concentrates about 23 microseconds of additional command work in selection/status presentation;
it is descriptive evidence, not the primary family or a causal trace. The 24-process hardware/WARP resource
study is likewise descriptive, with mixed n=3 results; only its corrected independent audit is accepted.

The bounded Tree badge ItemStatus correction and Files dirty-transition update are now source changes awaiting
red/green execution and fresh qualification. They do not inherit the frozen snapshot's execution results. The
Files pane is disabled in the headless benchmark, so its rebuild is not the cause of those five app flags.
The inventory must be reconciled again after these edits. The SDK A/A calibration at explicit revision
`cfe875fd` and actual-base A/B qualification remain separate; the measured-base policy is unqualified.
SDK hosted paired collection has since completed, but the verdict step reports inconclusive because the base
policy lacks independent qualification; its overall workflow fails. All six native jobs remain successful. The
completed paired artifact is now provider-digest-bound, preserved locally and independently recomputed across
144 receipts and all 78 slots, with no resolved regression. Its controls are predominantly unstable. The baseline
library fingerprint matches the clean actual PR base with Windows checkout attributes, while `sourceDirty=true`
is consistent with three recorded tooling overlays outside that scope; no complete dirty-path listing is retained.
The separate 144-receipt same-executable
calibration also recomputes all 78 slots; its observer audit passes under roughly 25% mean system CPU load.
Each calibration scenario has 24/24 unstable within-side controls, so this collection does not qualify a noise floor.
The one legacy/migrated disagreement is improved versus within-noise, with zero regressions in both judges.
All six hosted Full payload inventories were byte-replayed, with the two deliberate ASan detection probes
validated separately. These results do not qualify policy or execute the later source changes.

| Review delta | Current disposition |
| --- | --- |
| Tree state dots/count text omitted from UIA | Confirmed against the documented contract. Publish owned badge text as TreeItem ItemStatus with stable Name and a real subscribed-client property-event regression. Source patch pending execution. |
| Files dirty transition rebuilds the model and resets tree state | Confirmed source path through `Changed`/`RefreshFiles`. A targeted badge update and a nontrivial synthetic-folder regression are patched; runtime red/green and source-bound pin adoption remain pending. |
| Label text/multiline setters continue after embedded root retirement | Confirmed source callback path through intrinsic-measure invalidation. Match the existing Button/typography weak-lifetime boundary, extend the existing retirement case, and retain separate original-setter ASan diagnostics and final-source qualification. Source patch is unrun; guard cost is unmeasured. |
| F08 completion-post shutdown-hang suspicion | Investigated/refuted as the alleged missing-wake mechanism. `Application::Run` checks writer/recovery completion after every dispatched message; a full queue that rejects a post has queued work which triggers that check. `BeginClose` stops producers and retains the notification HWND until both services finish; `FinishClose` requires both finished states before retirement. |
| F08 evidence limits | Existing settings, recovery, hidden close and foreground Alt+F4 cases cover persistence and service retirement, but do not deliberately force both completion orders or queue saturation. No impossible empty-queue post-failure seam or message-loop rewrite is introduced. A2's independent preparation/factory/upload failure obligations remain under review. |
| Native math line-metrics HRESULT converted to capacity rejection | Confirmed source defect in `PreparedMath::Advance`: `FAILED(hr)` shares the permanent capacity branch with a genuine multi-line leaf. Separate native error propagation from that structural rejection, retain the OOM/device error channel, and add injected-failure regression evidence before closing A2. |

### F08 completion-order characterization

The current source has one retirement predicate: preferences idle **and** recovery finished. `BeginClose` stops
producers before reading those states, retains a hidden notification HWND while either is pending, and detaches
the editor host. Both completion notifications and the dispatch-level check reach `FinishClose`; worker state is
published under the owning lock before completion notification. This is source reasoning, not a new runtime run.

| Order observed by the UI | Current source behavior |
| --- | --- |
| Preferences finish first | Recovery remains pending, so `FinishClose` retains the HWND; recovery completion permits retirement. |
| Recovery finishes first | The writer remains busy, so `FinishClose` retains the HWND; preferences completion permits retirement. |
| Both finish before the initial check | `BeginClose` closes the idle writer binding and destroys the window directly. Recovery has already finished. |
| Completion post is rejected by a full queue | Existing queued messages supply dispatch work; the level check reads both completion states after each dispatch. This does not establish an empty-queue missing wake. |

The durable close/HWND ownership contract now lives in `Specs/Core/Core_SettingsAndCrashes.md`. The existing
`Settings.AsyncLatestValueSurvivesWriterClose`, `Recovery.DebounceLeaseCoalescingAndRetirement`,
`Editor.DesignRefreshTransientSurfacesAndClose` and `Window.AltF4AsyncClose` cases passed at the frozen sources.
Their evidence does not force the above service orders or queue saturation. No loop rewrite or synthetic
empty-queue post-failure hook is justified by the refuted mechanism. This disposition does not close the separate
native preparation failure tests or any final-source qualification gate.

## Retained 24 October review

The inventory records 446 first-party source/build/test/resource reviews and 36 generated-fixture provenance reviews (482 primary inputs). It also contains seven narrowly scoped 24 October documentation delta rows: six normative/planning documents and one generated changelog. These seven rows are an explicit exception to the general documentation exclusion; they are not counted as source reviews. The 21-row delta reconciles fourteen existing source/test/spec rows at updated hashes plus those seven document/provenance rows. All 489 hashes were checked against tracked files, using the newest intended-green WindowHost.cpp snapshot (which byte-matches the frozen live file); the selection audit found 482/482 first-party tracked code/build/test/config/manifest/resource inputs represented, with no unexplained omissions under the declared exclusions. Every inventory row has four substantive notes; source review remains separate from native execution and performance acceptance.
Every row has its current source hash and four review notes. Source review is separate from native execution;
the qualification packet owns build, runtime and performance results. Reports that retain reassigned pending rows
are partial supplements; their union, not any individual report, supplies the complete inventory.

The owner selected completion of the current native math/Mermaid profiles. Full TeX font assemblies, additional
diagram families, general incremental parsing and optional width-reflow/lane-width optimizations remain with the
existing DxUi rich-text audit and language/performance roadmap. These are not claims of supported behavior or
unreported regressions. The detailed math and Mermaid dispositions retain their explicit unsupported outcomes.

| Finding or observation | Disposition |
| --- | --- |
| Table bounds, structural selection/history, local object fallback and semantic ownership | Fixed; original failures and combined Debug regressions retained. Final profiles qualify the immutable SDK pin. |
| Math recognition, shared root/matrix budgets, atom styles/spacing/scripts/limits and prepared bounds | Fixed for the declared bounded profile; expanded positive/negative/symbol and allocation cases pass diagnostics. No full KaTeX claim. |
| Mermaid state descriptions, fan-out/chains, typed inheritance, hollow heads and true stadiums | Fixed for the declared bounded profiles; all 105 flowchart and 20 family outcomes remain represented. Final profiles remain separate. |
| Interested focus-update work and ProgressBar range events | Corrected with validated existing path reuse and accessible publication; stale/hidden focus and real subscribed-client regressions pass. Paired app benefit is not inferred from work counts. |
| Lease job admission and descendant completion | Corrected after two independent failures. Owned admission is mandatory before Resume; retained member handles are verified/waited. Membership races prevent an atomic whole-tree completion guarantee. |
| Overlay rollback and composed credential-helper test closure | Corrected and focused tooling passes. The assignment-source review hash changes without qualifying the policy or changing judgment bands. |
| `F-PERFCAP-SWALLOW-01` | Advisory limitation: optional ControlTests JSONL diagnostics may lose records on write/allocation failure. This stream is not the paired benchmark acceptance receipt. No blanket fatal-allocation rule is imported from another repository. Route diagnostic-sink hardening to the existing production-review owner. |
| `F-EMBED-ACCESSIBILITY-SAFEARRAY-01` | Test maintenance: two existing UIA cases destroy arrays explicitly; no current behavior or failure-path leak was established. Prefer the WIL array owner already used later in the same fixture when that maintenance is undertaken. This does not replace current runtime coverage. |
| Supposed unreferenced CountingWriteFactory/Footnotes helpers | Refuted by current renamed includes and their native/embedded callers; original review notes corrected. |
| Extra PowerShell assertion for descendant completion | Unnecessary: its required successful native `--self-test` already executes PID/creation-verified descendant completion checks and propagates failures. |
| Menu mnemonic lifetime concern and partial-Code entity corruption | Refuted or unconfirmed after caller/lifetime review; no speculative production fix is introduced. |
| Seven original-base performance regressions | Fresh same-SDK actual-base and starting-redesign cohorts complete and independent audits pass. The actual-base cohort still reports seven regressions, including +12.23%/+12.68% private memory. The retained 4 MiB region has no allocation-stack attribution; production fixes require new paired evidence. No rebaseline or cost acceptance is implied. |
| Brand WIC activation and bitmap lifecycle | Retain bounded native premultiplied pixels instead of a WIC converter at ordinary startup. Seven-size independent WIC byte parity and device-loss/warm-raster tests pass in Release Editor 35/35. Earlier small-icon heuristic extension failures remain test-authoring evidence, not a production defect. Memory observations remain unpaired. |

The SDK implementation checkpoint is `f5a2cea9ff798dcb0aa29a37766bcec460f41864`; qualification and both app arms
pin `94bf9801f51a544422ecb8aa6ae86d2e33aadcb2`, which also corrects the narrow descendant source guard. The original
source-guard failure, subsequent stale-build refusal, focused 7/7 pass and independent mutation probe are retained.
Both repositories' checkpoint validators and source-bound x64 Release runs pass, including the authorized app
keyboard and four SDK foreground suites with restoration. SDK `8a38e598` additionally passes all six official builds
and all 19 ordinary suites in both x64 Debug and ASan Debug, plus the sanitizer detection probe. Hosted x64 Debug
and Release pass; native ARM64 and the newer rendering/tooling changes require their own final-source evidence.
Final pin adoption, resources, input, documentation and performance gates remain on the coordinator checklist.

## 24 October source-identity delta

The latest reconciliation updates the SDK ordinal-path change at commit 392b0135 and reviews its seven files, including four governance/document rows added because this delta explicitly requires them. It also updates eight native cache/initial-root batching rows using the newest intended-green `host-regressions-green-v2/WindowHost.green.cpp` snapshot (SHA-256 `866fb8fc62eb06756f65845e5130a10a8e8bb4fb3a805deba0ebd0ccc7713187`), two trace-only menu rows, and three RedTilde app/resource-probe rows. The snapshot hash matches the frozen live `src/Controls/DxUi.WindowHost.cpp` bytes.

The cache review covers weak lifetimes, root and observer identity, live logical-child membership paths, mutation during path validation, path overflow fallback, and serial-guarded publication. Removing the old-vs-current epoch equality permits reuse after unrelated prior mutations; the entry revision, root lifetime/identity, serial and all five observer identities/lifetimes are rechecked after path validation. The stable query contract is reviewed in the header and `Specs/UI/UI_ControlsAndLayout.md`; the independent reviewer found no concrete cache/full-resolver semantic mismatch. The epoch-cost red matched 2/2 expectations: the focus-only case exposed the unnecessary resolver scan, and the corrected tab case performed the actual index-0-to-index-1 page identity swap. The earlier `host-regressions-red-v2` run retained 13 outcomes with two expected-exit mismatches. The first production green had 12/13 expected outcomes because its tab fixture did not reorder at the strict midpoint; the corrected fixture is covered in the later run. The `host-regressions-green-v2` run passed all 14 selected Release cases at 21:29 UTC against source SHA-256 `866fb8fc62eb06756f65845e5130a10a8e8bb4fb3a805deba0ebd0ccc7713187`, including the query-change guards and ordinary WM_PAINT Flush/EndDraw failure/retry assertions. This is focused host/rendering evidence, not the final platform matrix or performance acceptance.

SDK commit `cfe875fd815b0a39336100df9bde8f29ea2f0aa0` is clean and committed, and the RedTilde lock now points to that exact API-4 source. The x64/ARM64 SDK matrix is running; the pinned formatter passed while final `fullvalidate` remains in progress. RedTilde restore/build and its resource harness have not run on cfe, and hosted publication is deferred while the earlier hosted paired job remains active. These bounded results do not close current consumer, platform or performance gates.

The resource probe/app opt-in records diagnostic completed preparation costs. These additions do not establish performance acceptance or attribute memory changes. The latest retained red run is diagnostic only; expected failures and the intended-green source snapshot remain separately tracked. Menu dismissal-cause additions are trace-only; the test helper recognizes phase names but does not assert a separate expected trace event for each cause.

No repository-wide final runtime or performance verdict is made here. Focused build/test execution, final-source platform matrix, and SDK/app paired performance qualification remain separate gates. Retained hosted ARM64 ASan artifacts are revision-bound and do not by themselves qualify the later SDK392/native-cache/batching delta.
