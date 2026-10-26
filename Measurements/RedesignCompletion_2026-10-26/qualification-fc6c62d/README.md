# RedTilde final source and native qualification projection

This packet binds the completed native matrix to RedTilde source `fc6c62ddd23f3e3c086684aeecc2197343b04869` and DxUi `47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a` (API revision 4). It records six retained build profiles, including each build receipt and executable, PDB, and sanitizer-runtime digest.

The three x64 profiles each passed the application’s `All` suite excluding Tooling: 10 Storage, 4 Settings, 5 Recovery, and 46 Editor cases (65 total), with 23 application captures. Coverage includes the default and WARP resource lifecycle probe, crash-dump/marker checks, and Recovery/RecoveryRestart. The x64 ASan Debug profile also detected the expected heap-use-after-free probe.

The three ARM64 profiles built successfully and their build receipts and artifacts are retained. They are cross-build evidence only; no ARM64 runtime was executed.

This packet does not represent interactive keyboard or focus input, paired performance collection, hosted CI, or policy acceptance. It is not an overall repository-green statement. Earlier focused red/green SetView counterfactuals are listed separately from the final matrix. Wrapper attempts that ended before invoking native tests are not reported as native test outcomes.
