# API-5 consumer adoption

Status: DONE — selected implementation and x64 adoption scope
Completed: 8 November 2026

Implementation `9feb787e3e789db2f9cfc045e75a6b03f3737ccb` explicitly pins API-5
DxUi `21fae0dc66000b04504ba7862169533ac2943be8`. Protocol-2 reporter ownership,
original fault context, counted worker retirement, checked adapters and typed
native capacity/unsupported causes are implemented in the application. The
architecture and failure contracts live in
[Core_Architecture](../../Core/Core_Architecture.md),
[Core_SettingsAndCrashes](../../Core/Core_SettingsAndCrashes.md),
[Editor_Markdown](../../Editor/Editor_Markdown.md) and the
[toolchain contract](../../Build/Build_Toolchain.md).

- [x] Six local builds pass with zero warnings/errors; all three ARM64 profiles
  are compilation-only under the owner's delivery scope.
- [x] Local x64 Debug, Release and ASan Debug pass all 125 nonstorage cases and
  40 captures per profile. Preserve each original Storage Win32 1314 failure.
- [x] Hosted source `c80a86596aee8dc9c8a049c79fbb46f8d72e260b` passes all 155
  cases and 40 captures in each x64 profile, including all 30 actual Storage
  cases and symbolic-link save. Original wrapper digests and payload bytes verify.
- [x] The ASan profile detects its intended heap-use-after-free. Named reporter,
  worker-drain and session-end cases pass in all three hosted profiles.
- [x] The local test-disabled x64 portable package passes clean-extraction smoke
  and all fifteen actual test-command refusals before data-directory creation.
- [x] Update the exact-pin, ownership, editor, qualification and usage contracts;
  retain reviewed hidden native light/dark captures with build/DPI provenance.

The [evidence packet](../../../Measurements/Api5ConsumerAdoption_2026-11-08/README.md)
retains the earlier fatal/watch, redundant-fetch, padding, timeout, cross-child
fixture and stale-restore histories separately. Hosted and local canonical source
identities are not interchangeable. No earlier failed run is relabeled green.

This completes T-Q2's API-5 software adoption row. It does not close the
[parent's](../WIP/RemainingWork_2026-11-04.md) physical IME/AT/touch, provider
locations, physical GPU lifecycle, paired performance, history rewrite or release
publication requirements. Normal automatic two-architecture production gates
remain unchanged.
