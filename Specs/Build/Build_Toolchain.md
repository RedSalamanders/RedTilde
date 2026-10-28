# Build and dependency contract

Visual Studio 2026 C++ v145, SDK 10.0.26100, Unicode, `/std:c++latest`, `/W4 /WX /sdl /permissive- /utf-8`.
Discover VS through vswhere including installed prerelease instances. Match the pinned library's compiler,
architecture, runtime (/MDd Debug/ASan, /MD Release) and SDK. Outputs are private to this repository under `.build`.

A build transaction freezes all non-ignored source files except `Measurements/`, including WIP and prose.
Editing those during compilation causes the full-source guard to refuse the result even when native bytes
are unchanged. Update checklists between operations. Between builds, unchanged compiled inputs may support
receipt-verified incremental reuse; this never relabels an earlier run's full source identity.

`restore-dxui.ps1` validates the exact lock, creates an isolated detached checkout and invokes the supported
consumer build-identity and dependency-restore APIs. It never resets/cleans a sibling checkout. Fingerprint source,
API, toolchain, platform and flags. Import the public consumer props/targets and reference the single archive.
The initial-frame host option is adopted only with its exact SDK revision and a complete consumer rebuild;
API revision 4 alone does not permit mixing a newer host header with an older static library or consumer object.

Optional read-only parsed-snapshot reuse follows that same source-coordinated rule. API revision 4 is retained
for the additive methods, while MarkdownSyntax gains an immutable shared owner and changes object layout.
Adopt the exact source pin, rebuild every consumer profile, and retain its exact source/package header inventory.
An isolated method/layout probe supports a fixed comparison profile; it does not substitute for native linking,
consumer behavior, platform execution or performance qualification.

The ProgressBar animation-pause API also adds private class state while retaining API revision 4. A dependency
update across that addition requires matching exact public headers and library plus all consumer profile rebuilds.
A measured source/layout probe does not imply binary interchangeability with the older archive.

Deeply nested workspaces can exceed the compiler's path limit while CMake probes the dependency build, even
with Git long paths enabled. Retain the original configure/compiler logs. The pinned SDK's supported
`vcpkg-install.ps1 -OutputRoot <shorter repository-local .build directory>` can populate the normal package cache
using the same discovered installation, default toolset, triplet and manifest. Then rerun `restore-dxui.ps1`
through its canonical output and exact-pin checks. Verify the package ABI matches; do not copy installed packages,
edit generated consumer props or substitute a drive alias/junction to make the failed restore appear successful.

Support x64/ARM64 Debug, Release and ASan Debug. ASan Debug must have compiler instrumentation and the matching
staged runtime. Native tests must execute on the target architecture. `build.ps1` rejects a running executable
from its selected output directory and does not terminate it. Packaging produces a portable app with examples,
license notices and a capability-status note. A release cannot depend on an unqualified moving PR branch.

`build.ps1` hashes the compiled first-party source map by file bytes, so restored or preserved timestamps cannot make
changed source appear current. Before MSBuild it verifies the exact DxUi lock commit and a clean pinned source checkout,
binds that dependency identity, its source/package public-header inventories and the resolved consumer properties file,
then evaluates the selected compiler context and verifies the effective output and intermediate directories
are the owned profile paths. Before writing a receipt or restoring the dependency, it rejects reparse points anywhere
in the repository's output, intermediate or dependency-restore path ancestry. A missing or changed prior input identity,
an old receipt, or changed output closure causes an app-project-only `Clean` with project-reference builds disabled
before the normal build. If the clean boundary or required input identity cannot be verified, the build fails closed.
An unchanged compiled-input identity permits incremental reuse, including documentation-only edits; the receipt still
updates its full source identity.

After path preflight, each build attempt revokes the profile's prior successful receipt before dependency restore or
MSBuild. Only a completed build whose source, dependency headers, compiler context, output closure and executable remain
unchanged receives a schema-2 passed receipt. Failed or interrupted attempts leave no reusable success. Test, package
and performance entrypoints admit an existing executable only through that current receipt and matching artifact
closure. Historical evidence retains its original receipt protocol and is not rewritten. The guard binds first-party
compiled-source bytes, the exact clean DxUi source commit, its source/package public headers, resolved consumer-property
bytes and evaluated source/output/lock properties, plus the MSBuild/compiler/linker/resource-compiler context. It uses
the first matching executable in MSBuild's evaluated tool search path and refuses an unowned first match; it does not
claim to fingerprint arbitrary external imports outside those declared inputs.
## MSVC 14.52 Release compilation and linkage

All native profiles require the installed default MSVC **14.52.x** family with the v145 platform toolset. Discovery remains installation-independent. Restore rejects another default family before dependency compilation; MSBuild validates the selected family before compilation. Record exact patch/compiler/linker identities; GL archives and their final links require exactly matching tools.

Release explicitly uses /Gy and /GL, /OPT:REF and full /LTCG, with ordinary incremental linking disabled. Debug/ASan diagnostic optimization settings are unchanged. Preserve existing project-specific size/speed, warning, ISA, CRT and subsystem choices. Evaluated-profile tests check all six native configurations, pinned archives, and negative compiler/linker controls; resource-only satellites contain no C++ bodies.

Project references explicitly forward WholeProgramOptimization and VCToolsVersion into the adopted DxUi archive. Dependency identity includes releaseOptimizationPolicy=Gy-REF-GL-LTCG-full-v1 and is rehashed before choosing writable outputs. This separates the new archive policy without changing the adopted API/source pin. Keep header/library/compiler/CRT identities matched.

Direct SDK helper builds that use this fingerprinted output must preserve the same policy. The interactive lease
entrypoint uses `Get-RedTildePinnedBuildArguments` with its receipt-verified dependency identity to forward exact
VCToolsVersion, PlatformToolset, WindowsTargetPlatformVersion and PreferredToolArchitecture globals, plus
WholeProgramOptimization=true for Release and false for Debug/ASan Debug. SDK project defaults are not that
consumer policy. A helper's project reference can rebuild the archive even when LinkLibraryDependencies=false;
never let it replace a policy-fingerprinted archive using default non-GL settings. Missing or malformed pinned
policy/compiler metadata refuses before invoking MSBuild.

The evaluated ReleaseBuildPolicy regression covers the real lease and referenced pinned SDK in all six profiles
using the same argument factory as the entrypoint. It preserves Debug/ASan behavior and exact toolchain properties,
checks the adopted SDK's governed default /GL Release settings, and verifies that an evaluated
SDK toolset-default override cannot replace the pinned VCToolsVersion global. Evaluation compiles and
launches nothing; actual archive restoration and source-bound runtime qualification remain separate obligations.
Accepted configuration casing must select the same policy. Lease evaluations use the actual restored dependency
identity when present, independently rechecked through canonical dependency discovery, including its compiler host;
application-project evaluator defaults are not a substitute for that identity. Mixed-case Release controls cover
both lease and archive targets with those same identity arguments.

LTO is scoped to each final EXE/DLL and does not guarantee reduced size or improved speed. Retain baseline/candidate image hashes, sizes, build/link durations, fixture and dependency identities, every runtime/resource sample and concurrent-activity observations. Different compiler policies are an intentional experiment, not identical-profile evidence reuse. Cross-built ARM64 and headless functional passes cannot close native ARM64, interactive, or existing product/performance acceptance gates.

### CI toolchain preparation and audit evidence

Active Windows compilation/tooling workflows use the local setup-msvc composite action. It reads RequiredMsvcToolsVersion, verifies the discovered default compiler, linker, ASan target libraries, SDK and x64 LLVM scanner tools, and provisions Visual Studio 2026 Insiders only on GitHub-hosted runners when necessary. The Microsoft-signed bootstrapper runs under RUNNER_TEMP; existing installations are retained. A channel that no longer supplies 14.52 fails rather than accepting another compiler. Local -VerifyOnly is read-only; automatic local installation is refused. Hosted provisioning remains a separate unexecuted branch until its actual CI receipt exists.

[Full evaluated settings and differences](../../Measurements/Toolchain/2026-10-23-msvc1452/Settings.md) include both platform/configuration matrices and exact per-file metadata. [Microsoft installation guidance](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc) documents channel-specific acquisition. The published [runner image inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md) does not establish that the required toolset is installed; runtime versions alone are also not compiler proof.

RedMonitor is installed independently. Builds verify the local pinned client header/license and never compile or package the helper; see [RedMonitor integration](../Core/Core_RedMonitorIntegration.md).

Hosted setup waits for the hidden, owned GUI bootstrapper with both its --wait switch and PowerShell Start-Process -Wait. It reads that process's explicit exit code (0 or 3010), then rechecks the required compiler family and complete target tools. Ambient LASTEXITCODE cannot establish installer success. Paths with spaces stay one argument; installer stdout, stderr and fresh dd_* logs remain downloadable on failure. Delayed-GUI, stale-exit, wrong-family, missing-tool, signature and local/read-only refusal controls exercise the actual setup script without running an installer.
