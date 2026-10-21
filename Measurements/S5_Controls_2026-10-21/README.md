# S5 remaining-control investigation — 2026-10-21

Status: investigation and synchronous caption repair implemented; new paired collection and final qualification
are pending. All hardware, physical input, native ARM64, upstream D4c and release gates remain open.

## Scope and findings

The previous [follow-up](../S5_Followup_2026-10-21/README.md) remains unchanged. This continuation first retains
its current executable, then runs a fixed sixteen-process hardware/WARP contrast (four per view/backend). It
adds bounded opt-in command phases and repeats that sixteen-process diagnostic with identical instrumentation.
Both renderers vary. Every instrumented transaction renders once; hardware process thread counts are all 111.
Private bytes are unchanged between the primary snapshot and observations after 100/500 ms of hidden idle.
These observations do not support extra render count, different hardware thread counts or short settlement as
universal explanations. WARP thread counts vary (33–37); changing renderer performs different work and is not a
performance rebaseline. The CPU/VirtualAllocation profiler request is denied by host policy (`0xc5585011`);
there is no successful ETW trace or stack-based attribution. Policy is unchanged.

The original combined medians are also compared offline with balanced-operation/cycle estimators across all
four latest retained studies. Numerous controls remain flagged under those alternatives. The original primary
median, twelve-slot family and 5% timing/CPU and 2% memory investigation bands are therefore retained.

Explicit application phases show substantial native-caption cost (hardware diagnostic process medians roughly
32–50 microseconds). Ordinary edits already dirty often request the same title repeatedly. The repair caches
only positively acknowledged native identity text and skips identical writes, retaining synchronous Unicode,
filename and dirty-state updates. A failed or superseded request invalidates that cache. Hidden-window tests
cover typing, burst/undo, unchanged dirty state, Unicode replacement, rejected receiver writes and reentrant
document replacement. The own-window sender checks the explicit
[WM_SETTEXT acknowledgment](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-settext); it does not infer
acceptance from a wrapper return or defer native work outside the timed transaction.

An earlier queued-caption prototype is not adopted: it would introduce a new asynchronous readiness obligation
and could shift caption work outside the existing command boundary. Its source/artifact and passing focused
receipt remain private diagnostic history. The final fix is synchronous. Earlier failed retry receipts remain
retained, and the final failure/supersession regressions must pass before closeout.

## New baseline and declared paired scope

The incremental repair baseline is commit `24ef979b34ce86556a6ea1e1c2ad4dd57fea560d` on
`ej/redtilde-s5-baseline-v4`, frozen before the production repair. It already contains the earlier production
optimizations and identical optional diagnostics; it is not a replacement for the original pre-optimization
baseline. Both sides keep exact DxUi `dd619ef0c5855307eb57758072658f2e3b777fd8` (API 4), MSVC 14.52.37025/v145
and SDK 10.0.26100.0. The common input set additionally binds `RedTilde/EditDiagnostics.h`.

The prescribed smoke is one two-block, four-cycle A/B protocol check. The main collection is one observed A/A
then A/B cohort, each twelve independent ABBA/BAAB blocks per Compose/Split view, 64 timed cycles, two warmups,
seed 20261021 and `-ControlDiagnostics`. Retain every process, control and observer interval without pooling or
repeating to obtain favorable controls. Independently audit all raw slots and malformed diagnostic receipts.

The fixed fixture alternates saved/dirty on every transaction; both sides therefore still need 132 caption
writes per process. It can validate the incremental change, but cannot quantify the saving from eliminating
same-title writes in ordinary dirty typing. That saving is established by deterministic call-count behavior
tests, without claiming a measured typing speedup. Full edited-frame owner CPU includes all synchronous caption
work. Post-primary idle/inventory observations never replace primary memory samples.

Final six-build/three-x64-profile, default/WARP resource, capture, portable and paired receipts will be recorded
below. Until independently assessed, collection, local correctness and performance acceptance are separate gates.
