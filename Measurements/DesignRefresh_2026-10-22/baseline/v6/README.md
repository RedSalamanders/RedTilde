# Original UI baseline v6: selected public provenance staging

Private staging for root review only. Nothing here has been published under Measurements.

The basis is: receipts quoted; binaries not rehashed; actual paired script will reverify before collection.

This packet describes the retained original RedTilde UI at baseline commit bffc12d7b89ae52fb4b54f94e4736b9cb23202c6, derived from original source f819395d36c55b93e3bf9559d12d0ec401e42b3d, with the same exact 1cff7656 DxUi dependency and seven common performance inputs as the candidate. The preparation receipt records unchanged original UI closure and a v5-to-v6 delta limited to Dependencies/DxUi.lock.json. These are quoted scoped provenance facts, not a fresh whole-repository source review.

provenance.json contains only selected commit/content identities, EXE hash/length, compiler-profile/raw-tlog digests, dependency pin/fingerprint, toolchain hashes, common-input identities and source-scope facts. It excludes raw environment, host CPU/model, machine/user/foreground data, absolute paths, raw compiler commands and raw baseline/build JSON. inventory.json gives repository-relative private origins and byte/hash identities plus hashes of the staged public files. No application/compiler/library binary is included or freshly rehashed.

baseline-v6.bundle is the byte-exact small source bundle from preparation. It records the baseline/common-instrumentation/pin commits and requires original source commit f819395d36c55b93e3bf9559d12d0ec401e42b3d. Its current bytes and length match the retained bundle receipt. Bundle validation was recorded during preparation; this staging did not launch Git or rebuild anything.

Only the small JSON/tlog inputs and source bundle received fresh digest checks here. They remain private originals; raw tlog and raw receipts were not copied. The packet supports provenance/integrity review, not independent replay without those original inputs and the retained baseline executable. It establishes no native runtime, paired performance, policy acceptance, quiet-host, platform, CI, or publication result. The actual paired script must reverify exact source, dependency, compiler profile, executable and fixture identities before collecting new data.