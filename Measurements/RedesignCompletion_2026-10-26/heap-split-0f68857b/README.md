# Split heap request-path diagnostic

Four fixed processes run in original/candidate/candidate/original order on the same Split fixture, SDK47a9 and
64 edit/undo cycles. The original executable is `37FC5417...A522`; the candidate is the qualified `E5828F58...ECC`
from native source `fc6c62d`, documented at `0f68857b`. This is an intrusive allocation diagnostic with two
processes per role. It supplies no timing or primary performance acceptance.

| Slot | Events | Exact stack references | ReAlloc summaries | Checkpoints |
| --- | ---: | ---: | ---: | ---: |
| A1 original | 3,024,154 | 1,001,725 | 322 | 30 |
| B1 candidate | 3,226,455 | 1,070,225 | 442 | 34 |
| B2 candidate | 3,222,279 | 1,070,227 | 440 | 34 |
| A2 original | 3,021,341 | 1,001,723 | 321 | 32 |

The [projection](summary.json) retains all 130 eligible point snapshots. Root replay passes 757 integrity,
record and symbol-binding checks. Independent read-only review matches every checkpoint, live byte/block total,
event ordinal and top-stack record between the earlier conservative decoder and the corrected decoder. The
1,525 reconciled summaries change no live byte/block count or allocation stack. All four final replays have
zero event loss, schema failure, unknown process, malformed stack, ambiguous join or unmatched heap operation.
These checks establish consistency of the captured event sequence, not complete process allocation coverage.

Stack references use the payload's original QPC/process/thread, rather than the later StackWalk header. All
frames in the bounded x64 payload are decoded, including tails beyond the 32 named TDH fields. A moved ReAlloc
is interpreted as a redundant summary only after an exact successful same-thread Free of the old heap/address/
size and an unchanged, directly observed destination Alloc generation with joined stacks. Numeric thread
generation and causal linkage beyond this event sequence remain unproven. Earlier decoder and setup refusals
are retained separately. The [Microsoft StackWalk contract](https://learn.microsoft.com/en-us/windows/win32/etw/stackwalk-event)
describes the original-event fields and variable stack payload.

All four captures contain the same three large graphics-creation request groups: 4,194,320 bytes in two blocks,
4,194,319 in one, and 1,081,359 in one. These observations do not identify an extra candidate-only 4 MiB owner.
Both original and candidate snapshots also show temporary doubled math/diagram geometry during preparation.
The snapshots are not aligned to the harness's native phases and include startup and teardown; A2's final
snapshot is already much smaller. Do not select a final-looking snapshot or average these into a steady-state
memory result.

Both candidate traces have a 229,376-byte, 14-block group whose application request path passes through
`Button::Paint`; neither original trace has that group among its 32 largest live stack groups. Exact matching
local PE/PDB pairs resolve application RVAs; a mismatched supplied PDB is rejected without resolving symbols.
This narrows a chrome-text resource hypothesis. A request stack does not establish ownership of a particular
COM cache, achievable savings or a safe replacement. Category byte totals are partial and may overlap.

The module observer is intrusive: nominal 50 ms queries actually cost much of the short capture window.
Module ranges are point observations without load/unload lifetimes. The later file-hash inventory was not bound
by the original completion manifest, so module attribution remains unverified. Raw ETLs, module maps, compiler
context, versioned helpers and local symbol paths stay in the owned private evidence tree. The
[raw index](raw-report-index.json), [archive bindings](archive-bindings.json) and [root replay](root-replay.json)
publish their digests and exact limits without local identifying paths.

The [576-process comparison](../performance-0f68857b/README.md) remains the primary study. Its four original-UI
flags and unstable controls remain OPEN. This packet neither resolves them nor changes any band or baseline.
