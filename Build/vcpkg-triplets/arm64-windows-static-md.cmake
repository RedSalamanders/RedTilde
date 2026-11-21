# RedTilde (ARM64): static library linkage with dynamic MSVC CRT (/MD, /MDd).
# vcpkg-install.ps1 copies this triplet, followed by the Visual Studio installation and
# MSVC toolset pins of the build, to .build/vcpkg-triplets.
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
