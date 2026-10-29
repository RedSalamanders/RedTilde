# Performance disposition advice — 29 October 2026

**Owner decision, 29 October 2026:** after reviewing this advice, the user selects “Accept the measured memory
cost; keep other gates open.” This accepts +2.0045% Compose and +2.2195% Split working set for the current feature
set and closes the owner memory cost/advice item. Further memory-repair experiments are no longer required
for this disposition. The raw study and numerical findings remain unchanged. Command/control stability,
shared performance policy, quiet-host, CI, hosted RedTilde ARM64 and other platform gates remain open.

No measured repair from this investigation satisfies its criterion in both views. A further speculative rewrite
is not justified; any future implementation still needs a specific owner or repeated-operation basis.

The original-UI comparison retains stable working-set findings of +2.0045% in Compose and +2.2195% in Split.
Command findings are +10.3583% and +6.9521%, with unstable controls. The actual PR-base comparison has zero
resolved regressions. Those are different reference questions. Unqualified policy, unverified quiet-host
coverage and the hosted consumer ARM64 restriction remain independent holds.

| Measured option | Observation | Disposition |
| --- | --- | --- |
| Fold identical functions | File, image and code sizes unchanged | Reject as a repair |
| Suppress icon layouts globally | Working-set medians −96,256/−79,872 bytes; inconsistent Split behavior and increased preparation/CPU cost | Reject |
| Delegate caption messages differently | Command +7.89%/+0.27%, unstable controls; trial changes the routing contract | Reject |
| Compile application for size | Code −198,656 bytes; Compose working set +124,928 bytes with only 1/4 declining blocks | Reject despite complete functional qualification |
| Construct optional panes on demand | Working-set medians −268,288/−147,456 bytes, but Split only 2/4 declining blocks | Reject under the predeclared 3/4 criterion |
| Disable initial-frame batching | Working-set medians −2,048/−65,536 bytes with only 1/4 declining blocks per view; private bytes +5,113,856/+3,934,208 | Reject |

Negative byte signs describe reductions in the relevant diagnostic; they are not statistically accepted
production benefits. The [packet index](README.md#optimization-disposition) links the complete projections.
The small scouts have four blocks, limited inferential power and shared-host observations. None replaces the
original complete comparisons or changes their bands/family. All fixed slots and failures are retained.

Source review confirms that unchanged native captions and control text already skip publication, the folder
service starts lazily, and preparation skips hidden ancestors. It finds no small source-evident dead allocation
or redundant focus update with a defensible expected saving. This does not exhaust every allocator, font/driver
cache or runtime path. The state-aligned census localizes several contributions but does not identify an owner.
The private Debug trace reaches its event bound at the first 64-cycle slot; its partial four-cycle observation
does not establish Release working-set ownership. Increasing the cap and repeating until success is not a repair.

The following choices were presented for owner review. Option 3 was explicitly selected for working set;
options 1 and 2 are retained as the advice considered, not outstanding instructions:

1. Continue a separately designed ownership investigation, keeping the current gates open. First define a bounded
   diagnostic that addresses the retained trace's coverage/overflow limits and admits exact source/product/input
   identities. Distinguish requested Debug bytes from resident Release memory. Implement a narrow optimization
   only when the diagnostic supports it; qualify changed source and run fresh matched comparisons afterward.
2. Defer the remaining optimization and performance qualification, preserving the current implementation, all
   measured findings and an open WIP. Functional corrections can be reviewed within their exact evidence, but
   deferred performance and CI_PENDING obligations cannot be described as passed or bypassed for merge.
3. Explicitly accept the observed memory cost for this feature set after owner review. Such a decision would
   dispose of the owner cost/advice question only. It would not stabilize command controls, approve the upstream
   policy, qualify a quiet host, pass hosted RedTilde ARM64 or make the current failed CI gate green.

Removing requested Markdown styling, controls or accessibility behavior has no supporting repair measurement.
It would need a new product scope decision, rather than being introduced as a simplification. The earlier merge
request did not supply cost acceptance; the separate explicit decision above does. There is no rebaseline,
policy approval or CI waiver.
