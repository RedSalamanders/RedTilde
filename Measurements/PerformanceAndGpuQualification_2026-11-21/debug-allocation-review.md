# Debug Allocation-Attribution & Resource Diagnostic Design Review — 21 November 2026

Authoritative contract: [Testing_Performance.md](../../Specs/Testing/Testing_Performance.md)
App heap-walk helper: [Application.Tests.HeapDiagnostic.h](../../Tests/Application.Tests.HeapDiagnostic.h)
Historical reference: [Four-process Split heap diagnostic](../RedesignCompletion_2026-10-26/heap-split-0f68857b/README.md) & [Performance advice](../RedesignCompletion_2026-10-28/performance-advice.md)

## Scope Reviewed

This review reconciles the second open `T-P1` item against the current codebase on `ea111980`:

1. **Aggregate Owned-Heap Diagnostic (`Tests/Application.Tests.HeapDiagnostic.h`)**
   - Caps inspection strictly to caller-supplied unique handles (`MaximumDiagnosticHeaps = 32`, `MaximumDiagnosticEntriesPerHeap = 1,000,000`), avoiding process-wide enumeration or heap purges/working-set trimming.
   - Currently walks ONLY `GetProcessHeap()` and `_get_heap_handle()` (`Tests/Application.Tests.Performance.cpp`), explicitly labelling coverage as `"explicit-default-and-crt-only"` and reporting `busyPayloadBytes`, `freePayloadBytes`, `entryOverheadBytes`, `regionCommittedBytes`, `regionUncommittedBytes`, lock errors, and budget truncation (`budgetExceeded`, at most `entryLimit + 1` reads).
   - Timing covers only `HeapLock`/`HeapWalk`/`HeapUnlock` and excludes JSON serialization.
   - Tested across all profiles under controlled mock providers (`empty`, `oneEntry`, `truncated`, `exact`, `invalidBudget`, `deniedLock`, `deniedWalk`, `deniedUnlock`) plus native heap membership (distinguishing ASan quarantine/redirection from ordinary native allocators).

2. **Private ETW Heap-Trace Design Rules (If Deeper Debug Allocation Ownership Collection Is Triggered)**
   - **Live IDs & payload binding:** StackWalk events must be joined using the payload's original QPC timestamp, process ID, and thread ID — never the StackWalk event header's later delivery timestamp or unknown process.
   - **Public CRT quiet-point barriers:** Any snapshot alignment must bracket state transitions explicitly outside timed frames (`after primary` and `100 ms`/`500 ms` idle checkpoints without message pumping inside the observer barrier).
   - **Exact PE/PDB caller identities:** Public artifacts publish only relative module/symbol classifications and hashes; exact PE/PDB paths, raw ETL streams, and machine identifiers stay in owner-authorized private `.Perf` / `.build` evidence directories.
   - **Capacity & overflow refusals:** Bound total captured ETW/event buffer records explicitly; when a short trace saturates its event cap (as observed in the 26 Oct 64-cycle Split trace at 12.49M events / 4.14M stack refs), fail/truncate honestly and never silently re-run with inflated caps or drop partial data.
   - **Preserve prior owner disposition:** The 29 Oct owner decision (*"Accept the measured memory cost; keep other gates open"* for `+2.0045%` Compose / `+2.2195%` Split working set relative to the legacy pre-redesign UI) remains settled without speculative rewrites or silent rebaselining.

## Review Outcome

- **Design status:** Sound and aligned with [Testing_Performance.md](../../Specs/Testing/Testing_Performance.md).
- **Important nuance confirmed on current `ea111980` Release:** Direct comparisons between Hardware (`NVIDIA GeForce RTX 5080`: Compose ~77.5 MiB private / 64.6 MiB WS) and WARP (`Microsoft Basic Render Driver`: Compose ~34.8 MiB private / 66.4 MiB WS) confirm ~43 MiB of process `privateBytes` in Hardware runs originates from the GPU driver/WDDM runtime mapping rather than C++ CRT/default heaps (`~3.83 MiB` busy payload in `ownedHeapDiagnostics`). Any future heap investigation must separate GPU/driver private commit from application CRT/default heap allocations.
