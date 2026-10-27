# Editor preparation and native frame diagnostics

These two bounded studies investigate the remaining original-UI costs. They are descriptive diagnostics, not performance acceptance or CPU attribution. The diagnostics-off 576-process primary study at `09f22877` remains unchanged and retains its three original-UI findings.

## Current-only state progression

Four fixed processes at application `30fb0c2dc0bcd055aad99dc49a8cec8efdc56691`, SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`, retain 512 edit/undo transactions and 8,704 render attempts. Every process uses the 693 UTF-16-unit fixture and 64 cycles. Each transaction has 17 preparation attempts, with the sole host-render increment on attempt 16.

Primary Markdown and semantic preparation advance on attempt 0. Primary layout remains pending through attempts 0–15, then completes on attempt 16. In Split, mirror source-ink preparation completes on attempt 2 and publishes 26 ranges; Compose's hidden mirror remains idle. These are debug-state and work-counter observations. They do not measure stage CPU or establish that an earlier stage can safely be skipped.

The optional collector reserves 6,490,112 bytes per process before measured edits and retains that storage through the memory observation. Those instrumented memory values cannot replace the diagnostics-off primary memory comparison. All four raw reports and every attempt are retained privately; the public projection binds their hashes.

## Earlier matched frame study

The separate, earlier 16-process study compares original UI overlay `8c20c27a` with app `09f22877` (native source `16bea061`), using the same SDK4077 and matched phase/frame instrumentation. It has two blocks per view, four processes per role/view and 64 cycles. The table below is the median of four process medians for attempt 16, retaining all 128 transactions per process.

| View | Role | Total microseconds | Update | Render | Present |
| --- | --- | ---: | ---: | ---: | ---: |
| Compose | Original | 2,092.00 | 122.75 | 1,440.00 | 474.75 |
| Compose | Candidate09f | 1,957.00 | 110.00 | 1,317.00 | 492.50 |
| Split | Original | 2,820.25 | 98.75 | 2,492.50 | 201.75 |
| Split | Candidate09f | 3,028.25 | 105.50 | 2,705.50 | 188.75 |

These independently aggregated field medians need not add to the total median. The larger terminal Split Render observation is a drawing-cost lead, not proof that source coloring or any particular control causes it. No inference, sample exclusion, cross-study timing comparison or product gain is claimed.

SDK4077 has an accounting defect: pending native frames return before assigning `dxui.frame.update_us`, leaving Update zero despite positive total time. Attempts 0–15 retain their outer `totalUs`, but their Update subfield is incomplete. Both WindowHost paths are being corrected with a pending/completed regression in [DxUi PR109](https://github.com/RedSalamanders/DxUi/pull/109). Earlier reports are not rewritten or retagged.

## Integrity

`current-only-patterns.json` retains all four per-attempt counter projections. `matched-old-frame-attempts.json` retains all 16 per-process medians and the 34 role/view/attempt rows. `source-and-inputs.json` binds source, fixture, collector and all 20 independently rehashed raw reports. Raw paths, machine identities, observer process inventories and binary payloads remain private. `packet-hashes.json` binds the four other files.
