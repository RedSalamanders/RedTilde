# Build and dependency contract

Visual Studio 2026 C++ v145, SDK 10.0.26100, Unicode, `/std:c++latest`, `/W4 /WX /sdl /permissive- /utf-8`.
Discover VS through vswhere including installed prerelease instances. Match the pinned library's compiler,
architecture, runtime (/MDd Debug/ASan, /MD Release) and SDK. Outputs are private to this repository under `.build`.

`restore-dxui.ps1` validates the exact lock, creates an isolated detached checkout and invokes the supported
consumer build-identity and dependency-restore APIs. It never resets/cleans a sibling checkout. Fingerprint source,
API, toolchain, platform and flags. Import the public consumer props/targets and reference the single archive.

Support x64/ARM64 Debug, Release and ASan Debug. ASan Debug must have compiler instrumentation and the matching
staged runtime. Native tests must execute on the target architecture. `build.ps1` rejects a running executable
from its selected output directory and does not terminate it. Packaging produces a portable app with examples,
license notices and a capability-status note. A release cannot depend on an unqualified moving PR branch.

## MSVC 14.52 Release compilation and linkage

All native profiles require the installed default MSVC **14.52.x** family with the v145 platform toolset. Discovery remains installation-independent. Restore rejects another default family before dependency compilation; MSBuild validates the selected family before compilation. Record exact patch/compiler/linker identities; GL archives and their final links require exactly matching tools.

Release explicitly uses /Gy and /GL, /OPT:REF and full /LTCG, with ordinary incremental linking disabled. Debug/ASan diagnostic optimization settings are unchanged. Preserve existing project-specific size/speed, warning, ISA, CRT and subsystem choices. Evaluated-profile tests check all six native configurations, pinned archives, and negative compiler/linker controls; resource-only satellites contain no C++ bodies.

Project references explicitly forward WholeProgramOptimization and VCToolsVersion into the adopted DxUi archive. Dependency identity includes releaseOptimizationPolicy=Gy-REF-GL-LTCG-full-v1 and is rehashed before choosing writable outputs. This separates the new archive policy without changing the adopted API/source pin. Keep header/library/compiler/CRT identities matched.

LTO is scoped to each final EXE/DLL and does not guarantee reduced size or improved speed. Retain baseline/candidate image hashes, sizes, build/link durations, fixture and dependency identities, every runtime/resource sample and concurrent-activity observations. Different compiler policies are an intentional experiment, not identical-profile evidence reuse. Cross-built ARM64 and headless functional passes cannot close native ARM64, interactive, or existing product/performance acceptance gates.

### CI toolchain preparation and audit evidence

Active Windows compilation/tooling workflows use the local setup-msvc composite action. It reads RequiredMsvcToolsVersion, verifies the discovered default compiler, linker, ASan target libraries, SDK and x64 LLVM scanner tools, and provisions Visual Studio 2026 Insiders only on GitHub-hosted runners when necessary. The Microsoft-signed bootstrapper runs under RUNNER_TEMP; existing installations are retained. A channel that no longer supplies 14.52 fails rather than accepting another compiler. Local -VerifyOnly is read-only; automatic local installation is refused. Hosted provisioning remains a separate unexecuted branch until its actual CI receipt exists.

[Full evaluated settings and differences](../../Measurements/Toolchain/2026-10-23-msvc1452/Settings.md) include both platform/configuration matrices and exact per-file metadata. [Microsoft installation guidance](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc) documents channel-specific acquisition. The published [runner image inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md) does not establish that the required toolset is installed; runtime versions alone are also not compiler proof.
