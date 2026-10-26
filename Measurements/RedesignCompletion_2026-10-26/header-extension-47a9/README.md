# DxUi header-extension ABI support packet

This packet contains a privacy-safe projection of one isolated x64 Release probe comparing the retained DxUi 2fed headers with candidate 47a9 headers. It is supporting source/API and class-layout evidence only; it is not a benchmark, performance acceptance result, or general ABI-compatibility proof.

## Contents

- `summary.json` contains the two SDK/app identities, exact public-header inventory hashes and changed header rows, compiler/toolchain hashes, observed class sizes and alignments, the exact method signature, and hashes of the retained probe artifacts.
- `probe-source.cpp.txt` is the exact probe source bytes used for both sides. Its SHA-256 is recorded in `summary.json` and is `54E197ABBCDF1AF578A0864BAB9A76B65908D4633B78E339D0F1A11F606C64C5`.
- `.gitattributes` disables text conversion so the copied source bytes remain byte-identical when this packet is copied or published.

The raw private run receipt has SHA-256 `3908A21AD3783FC97547B1D325B7A7014A4627BD210BACBB5FC9D216326CB258`; that is the hash of the original unsanitized receipt, which is not included here. The sanitized `summary.json` and other packet files have their own independent byte hashes and must not be represented by the private receipt hash.

## Result

The baseline and candidate both reported `RichTextEditor` size/alignment 304/8, `Document` 88/8, and `SourceSnapshot` 16/8. The exact member signature is unavailable with 2fed headers and available with 47a9 headers. The reviewed source delta is limited to `include/DxUi/DxUi.h` and `include/DxUi/RichDocument.h`: one public nonvirtual method declaration plus a forward declaration, friend declaration, and private adoption method declaration. The 56 other public-header rows are unchanged, and the package-header aggregate digest is unchanged.

The probe used the candidate's x64 Release application compiler command and evaluated MSBuild include/library paths. It disabled PCH and linked only the probe object against CRT defaults; it did not link or call DxUi. The raw application compiler/link command hashes are retained, while the packet intentionally omits raw commands and machine-specific absolute paths.

## Preserved attempts

Attempts 1 and 2 refused before compiler invocation because of inventory-order comparison and module reload issues. Attempts 3 and 4 invoked the compiler but produced no qualifying retained probe binaries: one passed the argument array as a single bundle, and one omitted evaluated standard include/library paths. Attempt 5 completed both probe builds and runs. Only the successful run's artifact hashes are included as qualifying probe artifacts.

## Limits

Equal sizes and alignments, plus source review showing no added fields or virtual methods, support this particular additive method change under this compiler context. They do not prove vtable identity, runtime semantics, or general binary compatibility. No application tests, foreground interaction, performance collection, or acceptance judgment is represented here.
