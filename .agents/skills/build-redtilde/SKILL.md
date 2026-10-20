---
name: build-redtilde
description: Build, restore or package RedTilde and diagnose its Windows compiler and dependency outputs.
---

Read [the build contract](../../../Specs/Build/Build_Toolchain.md) and root AGENTS.md.
Use `build.ps1 -Platform x64|ARM64 -Configuration Debug|Release|'ASan Debug'`. Restore through
`restore-dxui.ps1`; it discovers Visual Studio and fingerprints the exact library/compiler. Never compile the
sibling checkout or repair a pin mismatch by resetting it. Check selected output process locks without killing
apps. Keep build logs and test receipts separate. Cross-build success does not establish native runtime support.
Package only after validation; retain dependency notices and the experimental-capability note.
