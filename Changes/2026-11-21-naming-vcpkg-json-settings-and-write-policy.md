- Complete Gate 1 settings and hygiene convergence (`CV-6-05`, `H-10`, `H-15`, `H-26`, `H-27`) and add the
  fail-closed real-location qualification harness: adopt RedHome's 155-line `.clang-format` and `Install-CiFormatter`
  in `format.ps1`; enforce `_camelCase` private/protected non-static member naming across `Common/`, `RedTilde/`, and
  `Tests/` via root `.clang-tidy` and `Tests/Tooling.Tests.NamingStyle.ps1` in `validate.ps1`; vendor `yyjson` via
  `vcpkg.json`, pinned `vcpkg-tool.json` (`aeb162b7`), `Build/vcpkg-triplets/*`, and `vcpkg-install.ps1` strictly
  inside `.build/` with `vcpkgSha256` receipt binding and `Dependencies/yyjson-LICENSE.txt` packaging; migrate
  settings persistence (`Common/Settings.{h,cpp}`) to UTF-8 JSON5 (`RedTilde.settings.json` /
  `RedTilde-debug.settings.json`), `SettingsStore.schema.json`, `Specs/Core/Core_Settings.md`, lossless one-time
  legacy migration, `.invalid-*` backup, unknown-field tree merge, and an event-driven coalesced `SettingsWatcher`
  (`OVERLAPPED` `ReadDirectoryChangesW` + `_stopEvent` + `ProcessWorkLease`) with non-writing live reload (`178`
  registered native cases including `Tests/Application.Tests.Settings.{h,cpp}`); and add opt-in fail-closed
  `Tools/Test-RealLocations.ps1` with its compiler-free gate `Tests/Tooling.Tests.RealLocations.ps1` in `validate.ps1`.
  Verified locally across all six zero-warning builds, full x64 `Debug`/`Release`/`ASan Debug` test suites,
  `format.ps1 -Check`, `validate.ps1`, and `Tools/Test-Changelog.ps1 -Base origin/main`. Hosted CI qualification,
  physical ARM64 runtime, physical printing/IME/AT, owner-authorized interactive focus, live OneDrive/SMB/WSL
  execution (`\\rubygloom` offline / `selftest` manifests pending), paired quiet-host performance, and v1.0 release
  cut remain open in `Specs/Plans/WIP/V1ReleaseReadiness_2026-11-21.md` and `Specs/Plans/WIP/RemainingWork_2026-11-04.md`.
