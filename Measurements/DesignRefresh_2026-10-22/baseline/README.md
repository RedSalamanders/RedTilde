# Design-refresh baseline v4 archive

This packet records the final x64 Release baseline at commit `d3e80a3cbd8d0e74a67d16c5f98b738dab42dce9`, compared from original UI prerequisite `f819395d36c55b93e3bf9559d12d0ec401e42b3d` with the same updated DxUi dependency pinned by the candidate: `e62dd9d9a91a6ad68d88f7f01a6dd075ae31dbd3` (API revision 4). The verified bundle contains the four baseline commits `19a0f88d211f95c5b9878b3ba2f20c8061f7c31b`, `98d03b2e18ef2c532eaa4c915dd64f8f0f36aaa6`, `d41d01874e47fcd18149da4327a9e623e9ac6c8f`, and `d3e80a3cbd8d0e74a67d16c5f98b738dab42dce9` and requires the f819 prerequisite.

`preparation/` contains the 15 original preparation/evidence files, including the raw zero-length bundle-creation log. `frozen/` contains only `baseline.json`, `build.json`, and `compiler-command.tlog`; the executable and all other binaries remain private. The private executable SHA-256 is `DA5CE23C925543A5A32D73E58D3FD69C13DEB82D4154DCF776F3AF27D071DE9F`, and source identity is `4ef3aa833e2795243055a6fbff3b2cbfae53732720f49fc57da265154ab32cb6`.

`validation/visual-review-debug/` contains the updated visual assessment, its source capture/scenario manifest, and the original first-pass review. The original review’s overlay observations are retained as historical, then resolved against explicit Files, Find, and selection-toolbar overlay requirements in the plan and UI design-system contract.

This archive documents the build/freeze and source comparison only. It contains no paired performance samples and makes no performance-policy or quiet-host claim. `archive.json` lists every copied source artifact with its original relative path, byte length, and SHA-256 so the archive can be checked byte-for-byte.
