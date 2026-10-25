# Separate 9025be4 resource diagnostics

These observations compare original UI `c1579053` with application `9025be4`, both at DxUi `2fed8246`.
They precede the `1d39ee27` candidate and do not replace either fixed performance study.

The separately declared committed-region census uses eight owned processes, Compose ABBA then Split BAAB,
64 cycles and a 100 ms target schedule. Independent replay checks 32 files and 175,747 committed-region rows.
Actual median sampling gaps are approximately 95 ms, with p95 values of 110–115 ms; whole-scan overhead was
not bracketed. One empty exit snapshot remains retained and excluded from nonempty tail summaries.

The candidate image mapping is 0.719 MiB larger in both views. Split additionally maps `consola.ttf` at
0.434 MiB from sample 3 onward; the original source mirror did not explicitly request the current monospace
appearance. These are mapping observations. `MEM_PRIVATE`-type committed ranges are not the process-private
byte counter, image mapping size is not resident working set, and the font observation does not establish a
CPU or memory cause. Unresolved mappings remain present without invented identities.

Two earlier owner-thread samplers remain private investigation history. Stack walking occupied 84–88% of
the sampled span in measured suspension brackets and is too intrusive for CPU attribution. The RIP-only
sampler's brackets occupied 4.58–6.90%, excluded resume cost, and mostly observed waits; sparse instruction
pointers are not CPU shares. No optimization or acceptance conclusion is derived from either sampler.

The summary publishes only source/digest bindings, aggregate differences and limits. Raw addresses, local
paths, process identifiers and native diagnostic payloads remain in the preserved private custody records.
