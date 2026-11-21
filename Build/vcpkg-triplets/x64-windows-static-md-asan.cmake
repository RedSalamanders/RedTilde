# RedTilde `ASan Debug` (x64): static library linkage with dynamic CRT and AddressSanitizer instrumentation.
# vcpkg-install.ps1 copies this triplet, followed by the Visual Studio installation and
# MSVC toolset pins of the build, to .build/vcpkg-triplets.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CXX_FLAGS "/fsanitize=address")
set(VCPKG_C_FLAGS "/fsanitize=address")
# vcpkg's Windows toolchain adds /RTC1 to debug builds, which cl.exe rejects together with /fsanitize=address.
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/asan-toolchain.cmake")
# A chainloaded toolchain switches the Visual Studio environment off by default; the MSVC compiler still comes from it.
set(VCPKG_LOAD_VCVARS_ENV ON)
