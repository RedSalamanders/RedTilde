# API-5 consumer, image policy and native refusal causes

Implementation `9feb787e3e789db2f9cfc045e75a6b03f3737ccb` explicitly consumes
DxUi `21fae0dc66000b04504ba7862169533ac2943be8` at API 5. The
[native ledger](qualification.json) verifies all 280 original application inputs,
six zero-warning builds and each original application/SDK/run/capture member.
Executed binaries match their preserved originals.

Each x64 Debug, Release and ASan Debug profile passes all 125 nonstorage cases and
40 captures, plus preferences, restart, crash and performance-smoke processes.
The actual ASan probe detects its intended error. Each separate Storage run
passes 29 cases and retains the actual symbolic-link privilege failure
`Win32 1314`; it is unwaived. All three ARM64 profiles are compilation-only.

The [current hosted x64 record](hosted-x64.json) closes this candidate's native
adoption gate at merged source `c80a86596aee8dc9c8a049c79fbb46f8d72e260b`:
Debug, Release and ASan Debug each pass all 155 cases and 40 captures. Each
actually executes all 30 Storage cases, including symbolic-link save, with no
substitution for the original local failure. Original wrapper digests and every
native payload member verify; all three builds have zero warnings/errors. The
ASan profile also records actual heap-use-after-free detection. Hosted canonical
source identities remain separate from the local checkout identity above.

The [x64 production record](production.json) binds the test-disabled Release
image, original portable ZIP and clean-extraction hidden smoke. All fifteen
test-only command forms actually exit 2 before creating a data directory. This
is a local experimental portable package, not a published release or installer
decision. Normal two-architecture production admission remains unchanged.

The application owns the API-5 reporter, original fault context and protocol-2
prestarted writer. Counted callback leases drain on ordinary Close with a
five-second cap; session-end uses its separate bounded path. Checked document,
configuration and codec refusals preserve source. A native capacity diagnostic
stays distinct from unsupported notation, with exact source owners, an intact
rich neighbor and unchanged selection/history.

Q13 implements per-document image consent, explicit local-network permission,
connection-time address validation and a pinned numeric connection with the
original TLS name. Direct anonymous transport uses no PAC, proxy, credentials,
cookies or redirect fallback. Local/same-share access is guarded; decode and
event-driven refresh remain bounded. An unchanged local watch survives decode
completion and remote-only semantic changes. These localhost/owned-fixture tests
do not qualify real cloud, SMB/NAS or WSL locations.

The current [light](image-consent-light.png) and [dark](image-consent-dark.png)
captures come from the hidden application harness. Their
[portable provenance](captures.json) binds source, binary, DPI, dimensions and
original sidecar hashes. Both were reviewed for expected theme, readable controls,
clipping and invented fixture content. No desktop takeover was used.

Earlier attempts remain separate:

- `1d843628` retained the DirectWrite fatal context and independent image-watch
  failure. Replacing only the SDK layout object removed the fatal; the watch
  failure remained an ordinary assertion.
- `0daf9924` exposed a third HTTP fetch after a remote-only semantic change.
  The local-watch comparison now uses only locally relevant source references.
- `2776053` exposed ARM64 owner-padding warnings. Explicit owner layout and size
  assertions correct them without warning suppression.
- `5ce4a000` exceeded the unchanged 90-second whole-behavior guard after 55
  passing cases. Content/workspace/media now have independent owned children;
  their exact union retains all 95 Editor cases. No single-case hang is inferred.
- `93d730a5` exposed an image fixture borrowed from another child's data root.
  The Media case now uses its own already-created image; the admission assertion
  remains unchanged.
- The first `9feb787e` matrix attempt correctly refused a previous-pin restore
  receipt before compilation. Canonical restoration refreshed both platforms;
  the original refusal bytes remain retained.

The complete native case catalog remains 155. Child inputs are independent;
active-case/timing diagnostics flush before execution. Complete static checks
and formatting pass for the frozen source. The source-bound prior baseline at
`0dbe0441` stays separate and is not relabeled as this candidate.

Current foreground/physical input/AT, named provider locations,
paired-performance policy, long-lived physical GPU resources, coordinated privacy
history and release publication retain their separate gates in the
[single parent plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
