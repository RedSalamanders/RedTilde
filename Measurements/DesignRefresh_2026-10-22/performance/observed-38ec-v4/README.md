# Observed RedTilde design-refresh v4 paired study

`cohorts.json` is a derived, privacy-filtered summary of one completed 12-block-per-view AA/AB study. AA is the same-binary control; AB compares original UI with refreshed UI. They remain separate. Each study contains 12 metric rows with 24 control drifts per row (288 per study; 576 total). The projection includes 48 unique pair receipts referenced by 192 native slots. The projection includes all 192 native slots, their safe numeric sample arrays and per-slot medians, source/binary/fixture/compiler/profile identities, and origin-relative integrity metadata. Processor placement, raw host/process names, absolute paths, binaries and raw logs are excluded.

The independent typed audit passed 7,988 checks for both comparison objects. All eight AB rows flagged `regressed` have adjusted p `0.005859375`; unfavorable controls and investigation bands remain visible. AA is descriptive and does not cancel AB findings. This is not policy acceptance, a non-regression result, or causal attribution.

Host observation was complete but the host was shared and did not meet quiet-host criteria. AA maximum/mean system busy was 33.46%/8.91%; AB was 96.69%/9.57%. Maximum observation gaps were about 1.03 seconds. The public host summary omits process names and machine/desktop details.

Original producer records are private and unchanged. Relative path, length and SHA-256 references support integrity review, but this derived projection is not an independent replay bundle: raw execution logs and executable binaries are not included.
