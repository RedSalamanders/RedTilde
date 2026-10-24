# Build and dependency contract

Visual Studio 2026 C++ v145, SDK 10.0.26100, Unicode, `/std:c++latest`, `/W4 /WX /sdl /permissive- /utf-8`.
Discover VS through vswhere including installed prerelease instances. Match the pinned library's compiler,
architecture, runtime (/MDd Debug/ASan, /MD Release) and SDK. Outputs are private to this repository under `.build`.

`restore-dxui.ps1` validates the exact lock, creates an isolated detached checkout and invokes the supported
consumer build-identity and dependency-restore APIs. It never resets/cleans a sibling checkout. Fingerprint source,
API, toolchain, platform and flags. Import the public consumer props/targets and reference the single archive.

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
