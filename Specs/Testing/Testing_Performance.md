# Completed-frame performance and resource validation

Status: S5 validation contract. A collected study is not automatically a qualified pass.

Keep the original forced-render results unchanged. They submit renders while edited semantic work can remain
pending; they are neither presentation FPS nor completed-frame latency. Comparisons that add native math or
diagram support also change the work. Measure an optimization against the same implemented features and pin.

The application harness must separately record the source transaction/callback, cooperative preparation and
submission, and GPU completion wait. A dirty sample starts from a fully prepared source, inserts into the fixed
prose edit site, waits for current syntax/semantic/native geometry and the GPU, and restores exact source by undo
before another sample. Refuse stale/pending preparation, failed rendering, changed source/history, an unavailable
completion fence or a deadline expiry. Keep raw sample arrays and app activity counters. Clean renders also wait
for GPU completion. These are hidden native completed frames, not screen presentation or physical input latency.

Use one common harness/fixture for baseline and candidate and retain the baseline before optimizing application
behavior. Compose and Split are distinct scenarios. Match Release/x64, compiler/options, dependency, fixture bytes,
dimensions/DPI, renderer, GPU/driver/WARP, OS/CPU and power policy. Verify source/build/executable/harness identities
before and after each process. Processes run serially. An independently drawn ABBA or BAAB order balances each
block; retain the seed and literal schedule. Twelve blocks provide 24 processes per side and scenario. Run a
separate same-executable A/A control with the same protocol; do not count within-block repeats as independent.

Report paired log-ratios and exact sign-flip inference with Holm correction across the predeclared timing and
memory slots. Timing and memory investigation bands are 5% and 2%, respectively, consistent with DxUi; they are
noise investigation bands, not permitted degradation. Keep same-side control drift, every flagged metric and all
raw rounds. Deterministic activity/resource bounds must hold in every process and cannot be averaged away.
The 12 primary slots are command wall time, complete edit/frame latency, clean complete-frame latency, owner-thread
CPU cycles, private bytes and working set in each view. Preparation/submission and GPU-wait medians/arrays remain
phase diagnostics: presentation pacing can redistribute their durations without increasing complete latency or
CPU work. Do not label a compensating wait increase as additional executed work. CPU cycles are not converted to
nanoseconds; retain power/CPU identity and A/A controls. This family is fixed before the locked paired study.
An uncalibrated/new judge or missing provenance leaves statistical acceptance open. Neither an A/A study nor a
candidate's own policy can qualify the upstream DxUi D4c gate. Confirmed degradation requires measured optimization,
scope/deferral options and developer advice before acceptance; never silently rebaseline.

Resource probes use the actual native app tree and preserve source/history/selection across default/WARP rendering,
dirty/undo cycles, hidden message-driven idle, resize and simulated device replacement. Check current native
geometry, image/cache bounds, generation changes and exact post-replacement pixels. Snapshot process private bytes,
working set, handles, GDI/USER objects, host caches and available DXGI usage at each named phase. A missing metric
is unavailable, never zero. Hidden/clean idle must request no additional preparation or rendering; production
workers continue blocking on events. Teardown probes retain before/after handle/resource observations.

Bracket each idle interval with cheap process-time and owner-cycle reads. Resource snapshots enumerate system
threads and can consume significant CPU; record their collection wall/CPU cost separately. Cumulative CPU
differences between resource snapshots include collection work and must never be labeled idle CPU. The v2
judge preserves fractional Holm probabilities; numerical tests include independently counted mixed-sign tails
and the exact adjusted value. Retain v1 raw studies and invalid judge outputs as investigation history.

Hidden diagnostics do not establish physical visibility/occlusion, actual device removal, kernel/GPU lifetime,
real-focus caret/IME/AT behavior or native ARM64 execution. Preserve those gates separately. A cross-build, library
fixture or screenshot alone cannot close product resource acceptance. Reviewed application receipts belong in
`Measurements/`; all generated processes, executables, logs and private data stay in `.build`.
