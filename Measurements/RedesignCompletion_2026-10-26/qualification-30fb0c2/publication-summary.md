# RedTilde 30fb native qualification

**Status:** Source-bound qualification. This packet records native product and build evidence for the pinned application and SDK sources below. It does not claim performance acceptance or an overall repository-green result.

**Policy acceptance:** Not evaluated.

## Source and configuration identity

- RedTilde commit: `30fb0c2dc0bcd055aad99dc49a8cec8efdc56691`
- DxUi SDK pin: `4077f57df8207eac6c39c43437feb45b19d23cf8`
- App source identity: `a2172f5db749915d45a39b3d6e0a60aced705548a822f89e79dea90b337e92a4`
- Qualification status: completed; status manifest SHA-256 `1D5AAD834845BC9A7496F552B2CBF4933C999848AE30CF8A68BB5F756ADADA7E`
- x64 configuration matrix SHA-256: `61C248238EB21D828A5C4CDFDE7B8F86925517C5A32F3187E9DA3922DFCD603A`
- ARM64 configuration matrix SHA-256: `4C2D2442E9DDCDD46801BD8316719611D2C73A8A0966195E3F7B13CD327A80FB`
- Matrix driver SHA-256: `93055C3A24271F940AB0284CBE27A3327A68484F668D30D0B3F013BE3147DEE2`
- x64 invocation: `test.ps1 -Platform x64 -Configuration <profile> -Suite All -SkipBuild -SkipTooling`; each matrix entry also records a successful build.

## x64 native results

Each profile passed the `All` suite with Tooling skipped: **65 behavioral cases** (46 Editor, 10 Storage, 4 Settings, 5 Recovery), no failed cases, and **23 capture records**. All capture image and provenance digests were present in the retained payloads. ASan Debug additionally confirmed the expected heap-use-after-free sanitizer probe.

| Profile | Test receipt SHA-256 | Build receipt SHA-256 | Artifact identity | App binary SHA-256 |
| --- | --- | --- | --- | --- |
| Debug | `850FD4BF0B051AF22731671D0FFDFB202AFADD7A4B96A3CB4DFAEFFF096E989E` | `B0934A365B70855FE975D19F544BDD309EB0DFFA6D3760CE87933F17C3BA4297` | `e61383d6b9d2094f1eb505777df5ae3e2637313f8278408e212da9e18857063c` | `6F66057CE8BAFE78C0CD18842DD9C5AD783D2A240C9EBB246C68EB5BE2935F3F` |
| Release | `FB5FD13B4BB8EAA31ABAC7433D8E5DEDA65FC32EFD98133C9E9C5FEB9EA6179D` | `5FBCB03378A6ED09AB26B16582B1C14A96A10A470C6F727E2D971A9DEF47B473` | `30d6286c11d70fabba544b1c663a2ff6dee82a592d42c1be88a8878c925aa6d1` | `FE192B9E41AF20E1532A472F06D543B21AD449118B590E6338D18F669E29815A` |
| ASan Debug | `AD06CB3BFC532AA11B768CD17578C35E7AB190C20B69ACA31450310E8249CD01` | `36D95EAB050AFCD642500ED490E856EC3752F28C50E09CE2B1A8E5F29653EBAB` | `76fe79186aea9169a5ae246bb8ba050da8921dd7f0069ce0f9db7e39a6d93f58` | `E817E79FE081D179381C1A18D1EE81EC294097DDC5B36CC01C215F33727FA410` |

## ARM64 build-only results

Debug, Release, and ASan Debug each built successfully for ARM64. No ARM64 tests or runtime execution occurred; these are cross-build results only.

| Profile | Build receipt SHA-256 | Artifact identity |
| --- | --- | --- |
| Debug | `359849B555350D16DE1E33D43300F439DB71D662E1F064493A406ADB2120623E` | `13454ed34bb5a5d91bc3014fef731c103872d76ec6c54b3a88aa501ae13328e6` |
| Release | `F206756A66AAF31A9A2ED4E6578B06CB0905BF1A274F51122F339117EED2379F` | `4d4b9e7258475f409018086fd52d86fb6f74b680866b14bd4af7314d175f8ff5` |
| ASan Debug | `06DBD3C1019590AA53F07AEA50D3DC6E3E0575979B38660ECDCC72E9367FF304` | `b1f64ff1ccad91a2fe91d58ac15739339296e84965f7625217bffe8373d7a157` |

## Authorized keyboard run and restoration

The separate Release keyboard report records **9 of 9 scenarios passed**, **168 real keyboard events**, and no clipboard access. The completed desktop lease independently records foreground, focus, and cursor unchanged, no moved items, and zero restoration or focus-capture problems. The report does not claim physical IME or assistive-technology qualification.

- Report SHA-256: `77929A10DF962AE78C4B76F71344615815C783577DA657338D0AE8C31AA07280`
- Run receipt SHA-256: `7425F7797AE1C6FACA896DE0D8536E2E67D69DAE5A53D1BC2378037318D625EC`
- Lease restoration report SHA-256: `D8F6C04CAB9402D5DC3DBD22B1344BAEF018253A9DA2EE765A11C462E8414BC9`

## Hosted checks and performance boundary

GitHub run `40320880695` was marked failed because account billing/spending limits prevented job startup: all three jobs report **zero steps**, and the matrix job was skipped. This is an environment blocker, not an executed source check. Hosted ARM64 qualification remains open.

The fresh performance study for app candidate `09f22877215a0084aae42f56c6e1ec1907079632` used native source `16bea061d17bee676f9f93d5f5bdadfd470ff8f2` and diagnostics disabled. It is a different harness/source point from this `30fb` native qualification; this candidate has newer optional diagnostics and has no paired performance run here. **Performance remains open; no performance-green claim is made.**

The restored colored Release checkpoint is a later, separate build at the same restored app source: its executable is `10B5B8BDF8CC5761A271B537DB88BC22C970A63B65B2E16162202EF8BBC30689`, versus `FE192B9E41AF20E1532A472F06D543B21AD449118B590E6338D18F669E29815A` for the original qualification matrix. Compiled inputs match, but PE/PDB and artifact identities differ; the matrix receipts stay bound to the original artifacts, and the restored checkpoint uses its own receipts. This separation does not close the three remaining original-UI performance flags, SDK policy acceptance, or the cursor-lease gate.

## Evidence index

`private-evidence-hashes.md` records independently recomputed SHA-256 digests and byte counts for retained manifests, build outputs, and every test/keyboard payload file. It uses synthetic relative aliases and omits original file names, absolute paths, and machine identifiers. Index SHA-256: `78423DA0B801779DD7CE8F4FC8D2F7FD2D18064010AECAC8B4A2564F92DC3982`.
