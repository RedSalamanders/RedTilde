# RedTilde current-source performance evidence — 2026-10-28

## Scope and result

This packet projects four separately retained matched studies: actual PR-base A/A and A/B, plus original-UI A/A and A/B. Each comparison contains 12 primary metric rows (Compose and Split, six metrics each) and all 24 same-side control-drift observations per row. The machine-readable projection also binds the source and binary identities, the reviewed SDK header-profile digest, the collection-declaration digest, and every relative common-input manifest hash. It deliberately omits machine/process names, local absolute paths, and private receipt filenames.

The independent audit passed 16,891 checks across four studies (96 native slots per study; 384 total), and all four raw comparisons matched the independent reference. The actual PR-base A/B has zero resolved regressions. The original-UI A/B retains four resolved regression findings: two stable working-set findings and two command-time findings whose controls are unstable. Quiet CPU coverage is not verified and the upstream base policy is unqualified. The verdict remains **review required**; this packet does not claim acceptance or close the optimization/advice gate.

The paired judge's `Running` observer value is a pre-stop snapshot, not a terminal failure. The source records the observer's state before requesting stop; report the literal observation with that timing boundary, not as a completed-run failure or verified quiet window.

## Current source and functional qualification

The measured candidate is the exact frozen RedTilde producer `e0908338cf5373978b5ef8d69cdd4ef6c3eba33b`, executable SHA-256 `C994554BD46FD4663A6187A26D67443941D534096CDB5CCBA53D40446D402511`, with DxUi `086ad82a5872768db23ea83862ab4af13dff0c5b`. The actual PR-base anchor `ad8f2ce2` was measured through overlay `38b1b7bdea8bc878f606e2c3b27d2f86b6690cc9`; the original-UI anchor `604bbaf32f7bfc13c4a5169ebe32804ceac71509` was measured through overlay `bb1a32ff059e812920da4465989e1f6d910d6522`. Anchors and measured overlays are distinct. The prior native producer `a5e75197` is historical, not the candidate tested by this performance declaration. The exact measured V4 SDK profile is recorded in `performance-projection.json`; the preceding PB-only V3 profile does not bind the current public headers and is not reused.

At exact producer `e0908338`, six consumer build profiles passed. The latest x64 native runs `005858152Z`, `010003066Z`, and `010103588Z` passed 67 cases and 32 capture/provenance pairs each, including ASan detection. The nine-stage keyboard/input run `010330174Z` passed. These focused/current-source results are not repository-wide or platform-wide acceptance. Hosted RedTilde ARM64 remains open under the billing restriction. SDK CI/tooling admission and the upstream performance policy remain separate open gates.

The historical `9cf8b221` result remains historical and is not silently relabeled as current-source evidence. The change in public-header/profile identity requires the fresh current-source study shown here.

## Optimization disposition

A same-source ICF size scout was rejected: file size remained 3,966,976 bytes and PE `SizeOfImage` remained 3,981,312 bytes; `.text` was unchanged. No executable-size or runtime saving is supported, so do not adopt ICF on this evidence. No alternative optimization has yet been demonstrated to resolve the remaining flags.

The next useful diagnostic is a current-source ownership trace with matching symbols and phase-aligned resident-page observations. Earlier heap/image traces cover different producers and partial, sometimes overlapping allocation paths; they cannot identify the owner of this cohort's roughly 1 MiB increase. Preserve the requested controls and styling while that work is open. Integration of the tested fixes and deferred release/performance acceptance are separate decisions; neither changes these findings or the 2% memory band.

## Files

- `performance-projection.json` contains all 48 metric rows and every same-side control-drift vector, plus public source/pin/binary identities and common-input hashes.
- `manifest.json` hashes the public packet files.

Private raw studies and full receipts remain in the controlled evidence archive and are not copied here.
