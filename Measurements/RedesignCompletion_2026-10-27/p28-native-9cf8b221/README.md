# Native qualification projection

`qualification.json` is a sanitized projection of native producer `9cf8b2215fceb38344f02d09430b2ad2870e8110`, source identity `6f1617c1862cc4a8593cc6a7653b46120a3b0df7a0c7b84151f34cd4f356356c`, and DxUi `98c795b765a10d1805ae0a7c35a8ade535fb7fd6`. `raw-hash-bindings.json` binds the retained inputs by semantic labels. `projection-validation.json` binds these two public files. The private generator is not part of the public projection.

Six profiles built: x64 and ARM64, each Debug, Release and ASan Debug. x64 executed 66 cases per profile (Storage 11, Settings 4, Recovery 5, Editor 46), plus 23 hidden capture/provenance pairs per profile. The ASan profile produced an actual heap-use-after-free diagnostic with allocation/free stacks and a nonzero probe exit. ARM64 is cross-build-only evidence. The authorized Release keyboard run passed nine stages with 168 injected keyboard events; the clipboard was untouched, and foreground/focus/cursor remained unchanged. Broader physical IME and assistive-technology coverage is not claimed.

Both Release applications retain `/GL /Gy` and `/LTCG /OPT:REF`. All 41 SDK compiler commands have `/GL` both before and after the fixed keyboard helper. The SDK archive and compiler-command bytes are identical across that run. The previous non-GL overwrite remains retained history.

The independent native audit ran **before primary collection**, passed 33,972 checks, and observed zero primary process slots at that time. Its status hash is a historical permit-wait snapshot, not the final 576-process collection status. Later documentation metadata does not replace the retained native source identity.

The projection retains qualification 1's unattributed combined DPI/desktop assertion failure, qualification 2's pre-launch driver-parameter refusal, and qualification 3's archive-policy mismatch followed by a source-guard stop. Native qualification does not decide performance acceptance. No screenshots, environment paths, process IDs, window handles, or desktop coordinates are published here.
