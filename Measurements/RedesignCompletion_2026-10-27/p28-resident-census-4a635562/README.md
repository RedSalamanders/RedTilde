# Resident-memory scopes and branding diagnostics

These two sealed diagnostic studies remain separate from the [primary performance comparison](../p28-policy-9cf8b221/README.md). They identify census accounting limits and test branding hypotheses; they establish no allocation owner, causal saving or performance acceptance.

| Dataset | Frozen producer / observer | Observations |
| --- | --- | --- |
| Original/current phase census | Original `4822f14d` / current `4a635562`, helper14 | Eight 64-cycle slots, plus a separate original-UI four-cycle smoke |
| Branding composite | Current `4a635562`, helper12 throughout | Eight original-v3 native slots, then four amended-v4 suffix slots |

The source-bound summary retains exact SDK pins, source/compiled identities, application and observer EXE/PDB hashes, helper source/build identities, declarations and fixture hashes. All 21 native observation identities were checked. Independent replays completed 2,351,424 checks for the phase census and 1,592,910 for the branding composite. Root additionally checked all 735 shared per-slot cells against those reports. The selected common-input map does not replace the retained complete build/dependency closure or create a new runtime receipt.

Helper14 collected both QueryWorkingSetEx and the complete traditional QueryWorkingSet address list. Across its eight study slots, EX-valid image/private/mapped bytes exceed the traditional list by 4,188 KiB. The list is 32 KiB below the observer-before working-set counter, leaving a 4,156 KiB EX/counter difference. The 32 KiB residual remains unexplained; the separate four-cycle smoke is not pooled. Affected private/mapped regions lack per-page EX flags, so their aggregates cannot identify an exact EX-only page set or allocation owner.

Helper12 collected EX observations and process counters, without traditional QueryWorkingSet membership. Its values must not be assigned helper14's traditional-list residual. Omitting branding upload/draw did not show a repeatable process-memory saving in this diagnostic; those conditions retained CPU icon extraction and do not test its startup cost.

The original branding-v3 controller failed at observer-only slot 9; its native slots 10–12 never started. Amended-v4 completed only slots 9–12. Both use the same helper12 observer. The composite is not a completed twelve-slot original-v3 run, and neither controller's failed state is rewritten.

The two per-slot tables retain their distinct query schemas. The hash index exposes 312 nonempty payload aliases, lengths and hashes, checked against all 417 retained parent files; 105 empty log/availability files remain in the private archive. The two parent report manifests and 45 report files were also rehashed. Raw payloads, logs, addresses, machine/process identifiers and absolute paths are not published. See the [performance contract](../../../Specs/Testing/Testing_Performance.md) for the census scope rules.
