# Consumer x64 ASan qualification

[The manifest](manifest.json) binds 26 untouched raw files from the successful
full run: the canonical receipt, four native scope reports, full log/status,
real sanitizer diagnostic, and nine selected capture/provenance pairs.
The receipt records all 67 passing cases, all 32 captures and actual
heap-use-after-free detection. The expected detector child exits nonzero.

Producer source identity:
`ea4a571538334b4da017157997b9a4f04ce0506c2e28ba928df536f3cdd2d5b5`.
Its metadata checkpoint is c4ee0459, native source is a0dfad8f and exact SDK pin
is eb0f10b9. The [build receipt](../profiles/consumer-x64-asan.build.json) binds
compiled-source identity 2166a79b, the public headers, compiler and executable.
The original full producer identity is retained after later documentation edits.

Root visually reviewed the actual ASan Close-hover 96-DPI and Markdown 192-DPI
captures. Automated gates also distinguish rest, Close and Maximize pixels at
96/144/192 DPI. Captures use the owned hidden harness with invented document
content. Simulated DPI, native window DPI and actual host DPI remain explicit;
this is not physical-monitor or native ARM64 acceptance.

The timing/memory banner is unpaired instrumentation evidence. Original
performance, CI, physical-input and native ARM64 gates remain open or paused.

The raw sanitizer diagnostic retains the compiler's trailing hex-legend space; its exact file alone disables Git whitespace linting. The manifest still binds every byte.
