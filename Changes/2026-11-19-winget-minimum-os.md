- The winget block of `.ci/project.json` now declares `minimumOSVersion` `10.0.22000.2600` (Windows 11 22000.2600),
  the field every other application declares and the value the owner took as the default minimum Windows version on
  2026-11-18 (RedHome CV-6-07, register G-05; register C-09). Generated winget manifests carry it from the next
  publication. Compiler-free checks pass.
