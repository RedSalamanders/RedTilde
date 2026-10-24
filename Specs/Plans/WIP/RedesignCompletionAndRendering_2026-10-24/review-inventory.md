# Code review inventory - 2026-10-24

Machine-readable review data for the parent plan. All rows start pending; source hashes do not establish review completion. Keep this fenced JSON under the documentation tree so planning records are validated as documentation. The parent plan defines the required review evidence and completion check.

```json
{
  "schemaVersion": 1,
  "date": "2026-10-24",
  "status": "NOT_COMPLETE",
  "description": "Planning inventory and source hashes; pending means a complete per-file review has not been claimed. Targeted reads are recorded in the plans only. Add reviewer, semantic/lifetime/caller review, test IDs and finding dispositions before closing A0.",
  "plannedAt": {
    "RedTilde": "c7456854c8f964bb9ea067295e6435f99d426515",
    "DxUi": "ceffe76a429d15039ceff1f7dfae7e1d7126323e"
  },
  "scope": "Tracked first-party C/C++ headers and sources, scripts, build/workflow inputs, JSON data/manifests and native resources; plus the exact DxUi consumer lock. Generated code stays listed for generator/provenance review. Vendored source/corpora, skills, documentation, assets and ignored products are not a line-by-line audit target; their ownership/provenance and boundary behavior still require review.",
  "entryCount": 441,
  "files": [
    {
      "repository": "RedTilde",
      "path": ".github/workflows/ci.yml",
      "sha256": "b9d237b55efd7e0f29a685e9fa7b8e82daabf0795eab90bae52fa79122f50a56",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "build.ps1",
      "sha256": "ab218b05f15a512ee599ab42f40c55f3e8fa3783a57a1d7b1382b2a731c48a28",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/Harness.psm1",
      "sha256": "e53552e8cf7002f52a09cdca5d593757365919c646c76c5031804cbb267f454e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/Performance.psm1",
      "sha256": "a5850d16712cc8d8c7ae494eb3366be791805fc933e3bd9dc15cc4aab49396cb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/PerformanceObservation.psm1",
      "sha256": "30a4715b2a29da67c00af77a3ae43c9af5ddaea59b1a10b176b2acb2e8b828ea",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/RedTilde.DxUi.props",
      "sha256": "5955c0931e04fbe59d09692244ff319e5f0cfeb586ce8dfed3a7156a4caf99cf",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/ScopedTesting.psm1",
      "sha256": "50d0f479281f4ead2a72beb6152f92ee8a34730dbe37973374e309d2ad0bc9de",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Build/Toolchain.psm1",
      "sha256": "4638ad95d1f48106ed09a7654196462b2aa97c144fb5b44c4946342cf41d55f4",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "capabilities.json",
      "sha256": "7c479509e93ce756e5666eef77aa191376e5d511ce2ccb939afc9ef282194a98",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/CrashHandler.cpp",
      "sha256": "8f585fb51670551318ebb196880bfb627d2c748720425458f037073671fbe98b",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/CrashHandler.h",
      "sha256": "ef82c354e4432ca8bfaae01fe5f4a1917f95b03c2f9c9b2eb653ae42c17738ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/DocumentInfo.cpp",
      "sha256": "9e7eeece032dadee3f79a4044ff148dad4d0434f9e6e60f3422af766bc1949ff",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/DocumentInfo.h",
      "sha256": "0414aea3bdbc67f49b97109178dcb6e42a6d9b51460a62766b4bfc59bc005b8b",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Files.cpp",
      "sha256": "08e78096ea35aed558c42d4318af0f829f661ffef29ae037c0e8a0d40ad3a76e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Files.h",
      "sha256": "2d3f9bf94e0faaeb1c0b7ed47b369dfffa12e6d6ffb434a5b1517662c6c49603",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Recovery.cpp",
      "sha256": "3f7e3b5ecd9619cb30d99e39307e2063782da8328a435b9c1c2be7426c19cc63",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Recovery.h",
      "sha256": "a2791dbb672905631bb2b1cc54b9913017fac1a11410ad8b385ce6ae0e7cedd9",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImage.cpp",
      "sha256": "a1c0ad49201146c61e4625dbc12a890b13dd17a915371cd08a1b9f9986ec914e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImage.h",
      "sha256": "ea55a04cb361bab5dae7da0c190f700543de2e98ab2bb217f6a64cc58ec0d0e5",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/RemoteImageImports.h",
      "sha256": "9544bf865901ef66981a755e66c7efe36e152aff2bfd26b3537ad1c2a464fcc4",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Settings.cpp",
      "sha256": "1f4134148d810cee761d797f722e0826565eb3ef6b416139ab3c85a654ae61ae",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Settings.h",
      "sha256": "73310426bff33169ca187be4e4c5cb24b1df9c2826b8f35e3ac6e2d179c70be3",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Storage.cpp",
      "sha256": "db4d62d8503aad67cfb73712dec9284f7ba97b986d0bbc1dc445c98871cef341",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Common/Storage.h",
      "sha256": "bbc228765adf14031b90f54f6f2d66f34f915a2264e7c05094e5aa90e7297e04",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Dependencies/DxUi.lock.json",
      "sha256": "74f687de9d4c5ab7a7601035da3ae61a13194e81f6fd4dc226362f98dc995d70",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Directory.Build.props",
      "sha256": "fc4a983cc286eb8b459766f22f2638aacd1486cce53d417aa893a89331c0cc07",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "format.ps1",
      "sha256": "3eda7cff89ade8359987235247d30b69a64c0b438991f98b10aa0178378b3724",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "package.ps1",
      "sha256": "725505f3d323ea244eb73356277e0b566099196bb9b0324692502ad2159cf281",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "performance-observed.ps1",
      "sha256": "2fc4a67b9a07bf78d47d11c16e1ba05940c717f0133152ed1e0857368fcd5dcd",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "performance-paired.ps1",
      "sha256": "ec17f60be95d513c6db4314ebe78201fc680da795563c835eb1241517640777b",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde.sln",
      "sha256": "6e39173e22e39a02b705ddffebb3366c7c045131c57dfcfab89ebba0dd4d0da2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.CaptureScenarios.cpp",
      "sha256": "acff958a17b2288b203cb9a0fa97b2dfeb9149d50ee1a3c0d36255a326e7f929",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.Chrome.cpp",
      "sha256": "fcbc97ceb109675cada7090c1c0fb556cb2c9aff62d1fd33f1cc8243b54e7f6e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.cpp",
      "sha256": "15e84820a8ab6a9cda325ecfe7877dc154312cf4b4b7941d444ba35ff8f46b1e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.h",
      "sha256": "efc666d69f8a0b0b0b68ae39b40269fd33e3ecb40eb11ce5597025dd8cbb6bff",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Application.Navigation.cpp",
      "sha256": "ac2765c5d422960afae07d73e1f4c816fa1253fd1ebc33e884333fac679c6886",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/AsyncFiles.h",
      "sha256": "8eb3cda6b6e6362d7ebeda811e0a628c461b739f83b56e3110215a35a3bf85e1",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/EditDiagnostics.h",
      "sha256": "b789d0a0dbd16e5936582719915cf873f63e672f0bda009d4df02ca7b5cfab48",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/ImageLoader.h",
      "sha256": "a3c800d467186f8599b7357ac49ce59d5dba206d09d90b15b3b2d0d15873ec78",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/Main.cpp",
      "sha256": "7918862fd087de59c563b21c9be196d6c1f4a620c2f66468a1914d72d0f070af",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/NavigationModel.h",
      "sha256": "72a77ffc90965572ff011fd187e6ff831e87c73070b20a9b22d83adfed14f613",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/RedTilde.vcxproj",
      "sha256": "99240363fa0b926aabaf6b13035a9cced00d55dbb2bafed7ec8c3d5446ea7ded",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "RedTilde/UiControls.h",
      "sha256": "de0e8d6bdf69a6f0bb88c2b2d53beffd87a43b2ed03aba45d7372ece05bff780",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "res/RedTilde.manifest",
      "sha256": "a2ab46534b26273903bc0dc9df374d9cd352e9e54b70534f901c1ee08904e484",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "res/RedTilde.rc",
      "sha256": "71df47846061f1dd880d6a0bc290c668ab943e314d2069e92f45076832603d86",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "res/Resource.h",
      "sha256": "d3723f905d3f0d8c70def4725dec5f0f858b50871767cf90e0502268ade3b475",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "restore-dxui.ps1",
      "sha256": "20122314d7c72b24ab8ddccd72796e8e41ef30f0e4d51725959334a0b4751b02",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Test-Changes.ps1",
      "sha256": "ece5628054570552d3ef363ce27c1a172ea383726e197b5eeb7adea66cb7c1ce",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "test-interactive.ps1",
      "sha256": "6ffb4c8a623796e814072463186b719ab86910052dd6480edbe14443c7d54c60",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "test.ps1",
      "sha256": "f11dc5abdea3bc90965443e1cd34927373dfe79f9fe763ca70e5c77d79972686",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Core.cpp",
      "sha256": "8e6ae80baa9edc6b4154a4623b195075167ec72f3d97ac5eef2ac0066de2e3b8",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignInteractive.cpp",
      "sha256": "d66ca92c1469035a6365bb2f08c617d7e1ece01da88ef56354ca2e10f617e269",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignInteractive.h",
      "sha256": "41c4c541952ff245b4e4cc5e6145a5e5df1bc7cf2caba3baa6e134b456ce86e6",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignRefresh.cpp",
      "sha256": "f6b00df14da0d41ddc4d0d2a4aa5fb1a17f8795da1e3ce7efa71aa40f9b5989e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.DesignRefresh.h",
      "sha256": "bfa91d004cc818f23aec07609b23a985245c16fd4afebcaa4c7dd4a4bb2251c7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Files.cpp",
      "sha256": "ce324aa55e067c0006d6e027ec9663458c191fd69ad40d9a9ce2881c1a73c150",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Files.h",
      "sha256": "3df3de8a09e0bfe3e747050306d2066837a400f8d0bfe29a55ba192402953382",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.Performance.cpp",
      "sha256": "ac7f6912cce121af62752a2e606ece5c40bd37a9bebcd81eb299a9782fcbb164",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.RemoteImages.cpp",
      "sha256": "0f524f78a4ae959cf3681475c39530baa7ffc2f81d96fb0295fac55e9c35c012",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Application.Tests.RemoteImages.h",
      "sha256": "76516bd27adcd39961edec246d45d4ef0f66000912fcc75920a84159fbb84f27",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/native-case-catalog.json",
      "sha256": "942e80483caf9d91d3a8600ec5874971b5b47f70948c77efbd04bde0aa2304be",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/NativeInventory.json",
      "sha256": "8f1060c529717296214efc7879299065c56909c0cd76fc87802841da223c1c9b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Diagnostics.ps1",
      "sha256": "3386f7a85904c7a00e50b5f784598987698c87546955ff7411cfaf8d5877079c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Evidence.ps1",
      "sha256": "d4b38a749c6824ff1ce344e472f077c7b4f3094676fb8c1313f46287dad950c2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Observation.ps1",
      "sha256": "e65621638f44e3e8aa2e9f346afe9f893939fa2881b5f9d530121fc5e38a6e2b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Scope.Tests.Performance.Tooling.ps1",
      "sha256": "da854c344a56ffc05fa62db2925309508c04e59d9e44c67482cdd1ba4ce1756a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Support/Support.Tests.FilesFixtures.h",
      "sha256": "f2306c9fee4e7bd54a8f66c06e471723e5a479b6bb8bf178207bec22abb425bd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Support/Support.Tests.TextServices.h",
      "sha256": "81a6866c450d1a0c5035f68c9f232c3dbb538cfa1ea8f00812fb166efdf37ccb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/test-scopes.json",
      "sha256": "5e02ac35d2f4671331471e364497b1e0f6b9700d5ab205eb51e9bf01eaa70b29",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Tooling.Tests.RunEvidence.ps1",
      "sha256": "04e6aa403a22a8921b7c5e8548fa92d12533508d8a5098cfdecedfda9268a2ef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tests/Tooling.Tests.ScopedTesting.ps1",
      "sha256": "507db3426d59f1a7095454a62676c45d7d031a2458e6e950d1e1ea7e2a32e80d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Get-CiNativeScope.ps1",
      "sha256": "52d66cfd12a2f68f745f69f57c3f3c6f4ed235b2b82656cc8a639bb6ad7a6db1",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "RedTilde",
      "path": "validate.ps1",
      "sha256": "c3cfa6959f7389f89ce452f4d62c9dabfb022dc22499c8d31febb9b2219ebb58",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/ci.yml",
      "sha256": "79e071d8bf16708f8e075ae9141502c2645b8b4cf8c75b8334a917adc96855af",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/format.yml",
      "sha256": "1cdaa9cb299e5985abae3e641e45f503f4b0451213c1a217768b437af43c81d1",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": ".github/workflows/gallery.yml",
      "sha256": "6546eb19d959f7171600e453e3605105bec9d62b1ba2e04182ac6af5891c5bac",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "build.ps1",
      "sha256": "357dccefd0839bac6acfc8833a2c80b0493577ab599d673b1320fb773eb0f2a5",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.AddressSanitizer.targets",
      "sha256": "25ebf22bd76677353b2c689c9b08793fccd99252a5b982a15dbd50aafb0ca059",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.Consumer.props",
      "sha256": "52d2cf3eeaffc860c7e3ba9ff54c3cd9e02c5615cd9561da7b7e28f6deece57c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Build/DxUi.Consumer.targets",
      "sha256": "0d62da7c448da3cb49c6971a6ef373349841727ef5c82931774a521352b76120",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "capabilities.json",
      "sha256": "782da4cee202c526e932665011de9360aa224033d7c9df71cef6075034b33b16",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Directory.Build.props",
      "sha256": "a9835e32463f717079de40bb1294b0fe1d1b83e71c160be605683fb366e92be2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Directory.Build.targets",
      "sha256": "cc008a910e83c5fc4b213060f29b77df1bf8de929fa0d904e3181cfbd618c0cb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "DxUi.sln",
      "sha256": "6cd86f1e17aea6e7518082afd3e1730e24becc31bc2acd30c2b106a9f468b50e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "format.ps1",
      "sha256": "4b120fbcffe2c6f74afd8ba880c401f6dab4f7e082ee356b42c916acecfafdef",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "gallery.ps1",
      "sha256": "a29695b042f77c25026756299dd8179272d11746bcedee6bc21b35bdf48036e6",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/AccessibilityTextUnits.h",
      "sha256": "051b4b92289854cd9a404f24e8f34403763c3bd519e60472ee4a2cf59c249f1c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Configuration.h",
      "sha256": "bd193165742a2204e5a0ed71cef2d1663a8345579173d5c948e6f0853947441f",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/ControlCatalog.h",
      "sha256": "8ae54ea18460382b1fc6b2c819f4ca109863bacb552ef1a5717281fff6c28e97",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Diagnostics.h",
      "sha256": "1539bdc27c5f2d07a49f84bcd51f406e51c567e4e4482545101ce6e92093a51c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/DxUi.h",
      "sha256": "4369858bb69ebdf89a13b20c37b32011e5c1b94b8156d2c6ce885912260ea369",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Embedded.h",
      "sha256": "6ceacf9dacb6dc75a4d7e4dbd3f97df18086a31d9501324c68d520791af09c5c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/EmbeddedAccessibility.h",
      "sha256": "ed19e79e7ff1c98c3e524aeacf91c4687e847f97e2f50225a9b09c5cf2de2ebf",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/FocusRestore.h",
      "sha256": "34dc2c6b4213f845819b5a4da8feb6232c79388efb23e6853677821ccb927853",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/FrameRuntime.h",
      "sha256": "7e00d607d6ec6c8e5d34cbfd30e7f01f384aa73cd3025b2b7581fa6df45c8aac",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/MarkdownSyntax.h",
      "sha256": "9417156cf96e3d7dd80b77dba33cd9e334dc4adfcd6851b8bdd28cb4b06da4e8",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/NativeMenuInterop.h",
      "sha256": "e4b12127af6dcdb4c158b75fee5800cb94ae0a2a0e5e6805b16b85f109da28c4",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/PointerInput.h",
      "sha256": "c16de418c74f1f50e0d9568736b9914a7b947716844706b299eeacdeec4b928d",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/PreparedTreeAccessibility.h",
      "sha256": "d8f29f824a20fd48adf3e97345988275f4777eeeb73c6773b784a0da32ec1a0a",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichDocument.h",
      "sha256": "567884f5dbc1caa7895736f073b10b9c1ec15f1c5c6f5eac227a17d50fb2e359",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichImage.h",
      "sha256": "2641ebf4c5d2d053f87de96a3d2584bb9e9a0f274a5b705ac113a297a5bec4cc",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMarkdown.h",
      "sha256": "de050951881f6de69c9e3c21a933720495ea6b6134eeadb21381476f06412ae1",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMath.h",
      "sha256": "be4838496d5c14ba6f85613677d1331663e81d7c4f2d9c40ee27a0f00eeb4e08",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemantic.h",
      "sha256": "f5f24b5634463f39f564bc02986fdeeda2bbbbb3e69f64e2a937cf18186e29a9",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemanticDocument.h",
      "sha256": "9619b4f38b662c02e68ebaaea6eafb63f0d3dc52aa6c37fb25fed95d0ff31a89",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextCommands.h",
      "sha256": "1770a9b2b492e85ab9558c8adae4e1e4e333b05592cc4bd2f21efdeb3c446620",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextLayout.h",
      "sha256": "8a23fbec041a2d2b0e90dcca19ff77eaff1725697d67868f20683b92f32163f8",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextPresentation.h",
      "sha256": "d96715b8b40f75bbd31e4d4a00b505cd8e6fac92db12da65b925ef2b948652e0",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/TextInputServices.h",
      "sha256": "4c50590f3c523900bfa5d2fb1df737fd79871e67b3d9d03674f79bf8b0a60a1f",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/ThemeColors.h",
      "sha256": "b8f43321fb894a39bd16ec10314bbde753eb3658d44baa9b91062cc74c44e95c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/Typography.h",
      "sha256": "ed7d2f50731fd6756646125c6a7ee04ddc62baa93944959bc902588d4884a4a7",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "performance-forms.ps1",
      "sha256": "0201a5910f36d8b69d93965f3601de6037f4b1943d13c44a21514fa0b104e979",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "performance-paired.ps1",
      "sha256": "bd29873180db31b9509a4804c1bf1f5ddeb1939c487f66529d6e7451cfe1c4f0",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "performance-profile.ps1",
      "sha256": "8ad8cf24470a7521739b7c5ad7d56ab9466ed6b66c7cc2e63db4f94493b0eceb",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "performance.ps1",
      "sha256": "52c863186486b235914d5558a982ddbe9c1d72efb29d41c68a474e604b19ab0f",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/ComplexUi/ComplexUiScene.h",
      "sha256": "425c830c22ec3672f52f6569bebd5fe2ababc45dfc47f530523eeb0f4c3a3041",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/DxUi.EmbeddedControls.vcxproj",
      "sha256": "21124b1f5d13204b57f37d10ce3d960306c455713e532ecb13156ce3178f61ec",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/EmbeddedScene.h",
      "sha256": "e7da318e233c1f6a9a465589494a47b99013374af4d2f5b8b5d1834daf4b3c9d",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/EmbeddedTextClient.h",
      "sha256": "67f3dbf109a3e72e7be2a7d9bc89f5f9351c4d2e968666f7cd97612c44bf2f73",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/GraphicsFixture.h",
      "sha256": "b242c5efa16862ccf416cc2c7a277a2e78354273ef43d3cfdf48c69bbef5c19d",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/Main.cpp",
      "sha256": "1dce92700688ca4efd6c7744b56a821ba0ebb608d7f9855599239b09cbca84d4",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Samples/EmbeddedControls/RichImageLoader.h",
      "sha256": "533fee661a99338dec13adb775986d413f0a0b62b276e70afa3e550b716f7801",
      "category": "samples",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/ControlCatalog.cpp",
      "sha256": "d6fb32eb76cff6fcd1befff2303fa0f237654a5c7cd3e0cfd11e80249c7fb29a",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Accessibility.cpp",
      "sha256": "dc265921d1518f8a2fae3d81cfbac2388f58059b9780747332cf62768dc48e3d",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.AccessibilityTextUnits.h",
      "sha256": "27f77b8fb0a2c02c1f447a32d6d5a9fbd85d7cb9ab9c312a19b46781867531d7",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.ComboBox.cpp",
      "sha256": "aff827d2485537f824232fc917b65b3e831b65bae595b0fc796c3cac1bf0b11b",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Controls.cpp",
      "sha256": "6ba631060b799191acbdcc47819ff70bc857f39d9b6f93033a89611a8f96e121",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.cpp",
      "sha256": "207017d799dd6a5ca8b51727bb2a741f785d07614f48417da730d0317c858635",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.EditorControls.cpp",
      "sha256": "44ef22a3bf667fa608e86aa838f212747575f327c0963a82c652b0fa6dd77e00",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.FocusRestore.h",
      "sha256": "ba672d41cdd7c3f289cd0cda7873d00f9782c642de19eaa9b48cca803f68249f",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Forms.cpp",
      "sha256": "f9fa953e13b31aae558e45e96d3aa6a6f196a372334e859d48018cd79d5ffba2",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Grid.cpp",
      "sha256": "9ac239a10f086afaa86e1998f2aec1c4c0201de463e66a4994cdd091ed8f9b72",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.h",
      "sha256": "f6f32ba08edcd3d0f97b5ae5cda7dab74bb81c894a593cf1e5f11b5083429089",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Internal.h",
      "sha256": "4b75cc434c199b80dc299f2b64f94d1736abbd2f66904f0b06263b5d550bbda4",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Layout.cpp",
      "sha256": "5a59546af1d55ca6ce9838cb3558c90183b38004380448ce95fb85f28f56c893",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownBlocks.h",
      "sha256": "3c9c206a2acd05358cc281a26e75dbc5ce2d3b3fc743192d72b79ac8fcc6c5ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownCharacterReference.h",
      "sha256": "4fb5f5eac34311d605f9916f2889f13f26a5dae00b02799b7813eae1c240f265",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownEntityData.h",
      "sha256": "fe27d6f6fd7bdb89dd252729524aa835a0ea798087e409a81bcaa4e255ecaf33",
      "category": "production",
      "generated": true,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownInlines.h",
      "sha256": "4d6fe2886398af9b6021769544bbb0c3633910f8b9ac92a563622187d96f9c67",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownLabels.h",
      "sha256": "13031872578ddb54e53efe212a337baefeb195074f23b6e783b45735c6f2682c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSourceCorrelation.h",
      "sha256": "a7af9b55d38cb7691fb4ce2469b0d2a3c8b00d4f7c8a667b21f4c1ab3c3f4b1c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSyntax.cpp",
      "sha256": "735e314ec52cd8b2d44c55c704254c387f822a2bb259c57ac845fb0a5074414a",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownTableCells.h",
      "sha256": "f548d2132cf17c931d8a894fc9a6354d39e6ff17cd9291bec8ea8c8d929812da",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownUnicodeData.h",
      "sha256": "1e49ee2858300241526b6a257b44aa0c45d90b6ed35c43c900403aa449bca262",
      "category": "production",
      "generated": true,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Menu.cpp",
      "sha256": "dc274149227c2a842429ea49e337f93b8421864e0a4abbad92e5ea9077699324",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.cpp",
      "sha256": "d425140cf2409ecfb1f82c4950840b2faaa7e3dfb692ca3ff1828188df5cea90",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.h",
      "sha256": "8cdf3e1471050b088490273d7357eda242c794456083d3b72f9912c9d8bf62a8",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.cpp",
      "sha256": "2ff4ecadf2ad418ebe044ea5d8146a0e6c9f6aea6a1f7d23a3ce9ff72d76c974",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.h",
      "sha256": "6ca5af87c500447bb6c4fc65b848f5d20cee889a2dcbc067b22b83e9aa642387",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeMath.cpp",
      "sha256": "0d80b6b671de3461643c92fb62f95f83580084ece4ad25d2f8371c4c0b2ffa27",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeTextInput.cpp",
      "sha256": "ea0cdd57636994fb8359783c8e79b07c342c65febef39c2f5c08f28c055be608",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PointerInput.cpp",
      "sha256": "2520cf55044fc8da14071054011d252bc9a83a0397797226ec23afeed7f69c03",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PointerInput.h",
      "sha256": "d9cc5f1ed8c7cc8e61fe9bed609ebffb2c8d9bb6e7d8539da4577a3807f82824",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.cpp",
      "sha256": "399741ef95f6c9911be6a9afd39627304c3e0f1628cce2b6684987594570f0a4",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.h",
      "sha256": "b9469918cdb2068d4cdb65f6d4ed0308eea258456360b2b072732d508d6f44fb",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedMath.h",
      "sha256": "61ceb37a016d2f709f5371412d03016f34c951f4b9ba5bee6f80992488d19329",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibility.h",
      "sha256": "5ec593951306aab44bfd529af2e3e3928965354f64af271e0c3b019662235aea",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibilityEpoch.h",
      "sha256": "65b95f8d202c883d755aadd018f81467912740a0f89213635aaabc4364d620e6",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocument.cpp",
      "sha256": "c016485231a4bc39f53415159fb8a3a8e6e80488738c1910ffdbd88c88e9b271",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocumentLayout.cpp",
      "sha256": "f03cd7aade94d6a8d88725dad7158f14335ff65e6ff83dea22069dd87f58c5d4",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichEditorLayout.h",
      "sha256": "fe83b32163b16431015a56ba7849b280f67c07291271fbc6ad3cb33ca4c96937",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.cpp",
      "sha256": "84047cf4d3f8765f03d7564561405082d751bd4b7387a8653adfc4fdec60244d",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.h",
      "sha256": "34d27813d652032e61f48accb25719fbef6369943513d8a9bd07a4fb2274ebcd",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImagePaths.h",
      "sha256": "864e905b5674f26b6d3da143117312eadb0524a278a1a25b67832f7b0e5bf5e1",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImageResources.h",
      "sha256": "a658daf91c20acff83cf3c44ca89762fb1467b408384c438321b46adfaa5e490",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichMarkdownProjection.cpp",
      "sha256": "15df5d3c3ecdd6faf431cd2a54fede054a6fe643cc09c9c090cc8a63b0900a97",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemantic.cpp",
      "sha256": "b90e450504c76b5d12808b46d796419ee4dd747817b42e70d3d946a75a5cd209",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticDocument.cpp",
      "sha256": "da13ec1895dd296f658c96d6dbf467e80b4ccb6f983d49451c4c39006fedaf0c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.cpp",
      "sha256": "7843829edfa306f1947c3eb8aeecc4c27de65877295d3046351f1e22e1457998",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.h",
      "sha256": "5ed3b1fe7b6ed24ee1e588519f53c2196eec9f62f42ebd7572439ae6e0ec4757",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticHtml.h",
      "sha256": "2c8417f26eca42b5cc0eac43d89848eaccdc3b6ae37f3f756892ad4936f1c80e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticValues.h",
      "sha256": "3031f1706c7c011ccf049794586cc94efe05f666dd1aed33726d2619102a6861",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSourceInk.h",
      "sha256": "829529fc34f2c1e4a565a398b85ec3b7154caa2585c170066af329e9cf0a9343",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextCommands.cpp",
      "sha256": "1378a6d337601e32bcbe3ce2c5367340d2be01c9ed7964ef1e63db7ee1c28bc7",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextEditor.cpp",
      "sha256": "3dc84b9428521e04b37e55ccc29a3333bfb47a3830ac9bd9fc43a670352e4508",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextLayout.cpp",
      "sha256": "886be4434a3b72dc8c3338fb6be08e6a8589bc76261bb1534a55fb72f0ffac51",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Scrollbar.cpp",
      "sha256": "f70bd75d85454b5dc307ef7388550aa4f19b7a8db11503a38c1db45998d49776",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.SemanticExtensions.h",
      "sha256": "4b620f5bee8add648004b00824f878ccd884685a2fbf18f8b2c5ca31ee0a7ce6",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.SingleLineTextEditing.cpp",
      "sha256": "6e24615e3149d6487d5ab6f215310dbd60c39dda4a339e89eaa3c5de02bab03b",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElements.cpp",
      "sha256": "870cb07abc38b13c94b8c0982df664936d282a0c6c23b801ca719c0c8ee629cc",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElements.h",
      "sha256": "ca82bec5f26ef487f1c3e344ebf75ec9f0b6ae2c2d238bdd9a30cc4a90802153",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElementsData.h",
      "sha256": "634788ce7e2fae26aea98501c6ad932a65dac6b9474bf8aa599434d51ca5af70",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextInput.cpp",
      "sha256": "884ff05dfc89aa1893e40488f9d9bd218adef5653eba55328dcd5191ac1a3913",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextStoreACP.cpp",
      "sha256": "5e9afcc4c26bbe451051545cac74289edf51b89ecd44e424ca1340ea3b14c251",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Theme.cpp",
      "sha256": "8dee2d2373282a11cfc629fd758d7cc82c76245a379d7f92b8c3008af45fcaa5",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Tree.cpp",
      "sha256": "7ed830492ebeffa836ecf98644d112005461366d7ca41bf740f39f4ef2d6a26c",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Typeahead.cpp",
      "sha256": "05ce67c7c18b65c32a8388e49ebc35791233b3ce2a027763e39317b42cfed23e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Typography.h",
      "sha256": "cd25c87f67134154a930a9cc3bfa112a40f87569e7ab9770f79d7d3538ff92ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.Win32Hooks.h",
      "sha256": "c5bea4da02db45f90c70203ed1c88de9fed8656ecb8c9873e7169e9cc4218c23",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.WindowHost.cpp",
      "sha256": "ed1f004aabd559c69ce2d3665363ff74a3d86bf659d6b3be71f3667c1dde9880",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUiNativeMenuInterop.h",
      "sha256": "c12d6b4c041c73911f8b3be92841d4b28cf48620b4bf83ec65ca5534e4b68646",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/NativeMenuInterop.cpp",
      "sha256": "e1807475db64c3bdf6f4fba97acf5c0d73055a0abe759fb104bcd7be54841d58",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextClipboard.cpp",
      "sha256": "305d0c1a5113c4c48d5743564cc2b7334f848701dbf4eeada7cc9b9a6c56cae1",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextClipboard.h",
      "sha256": "82df11645b24522d7c1bd56a51ffa99fe670d049ccaa9222033babbce918681e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextInputServices.cpp",
      "sha256": "01bcf48761ba51b66cc2920c6d4a88f32a20a3e463cf0220a810dbdebb74ee2f",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TextStoreTarget.h",
      "sha256": "6a9a7eb6028f13f70e32c6bf7fb3cae926c7c84dc44387fd5a5a840a31492540",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/TsfFocus.h",
      "sha256": "fc42f41bf5253d708bab5cbab1cd533c54b66fb70aca72dfdb3388ce01b8da8a",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/DxUi.vcxproj",
      "sha256": "66d813eba8f3ec3d78a1a8e6b5d0e20483812e01deeb87d3388aed5e89b067e0",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Foundation/FrameRuntime.cpp",
      "sha256": "e4d747f471cde94d1bb368a99ef3a4f496daa4365b6831b04b84215a56d80be3",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Rendering/Embedded.cpp",
      "sha256": "e08cbd13c9d0313c7251e08a81fb2e9dc1294a7e54001beb46f6f216b5914a20",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/AnimationDispatcher.h",
      "sha256": "ee967b0a098f4dd114c21e264f504e5a4908116fdc33e18f164d8e3aa4df463b",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/Diagnostics.h",
      "sha256": "baceffa050b9f7feba93879cf1e949f24e44ebccb42bd4d9b2b8efce8c813702",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/LiveResourceCount.h",
      "sha256": "d9073bc9720e38988dd4a034bd40f050df8fb9c30275c57583cad42e0f274e9e",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/PostedPayload.h",
      "sha256": "04a87d28d23f40d7330383e1c5bb0dbcfbb1b5775145bdd63624092bf8ef4950",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/RequireAddressSanitizer.h",
      "sha256": "afe0989ccbe34c9e2ffc8c3c6b9394924586e605c7ab371182d6affec5e29f32",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/Utilities.h",
      "sha256": "b83184424f0cc02bb79bff1c99a8368dcb9e8635b8188ebe1d5285a1fdafd789",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Support/WindowMessages.h",
      "sha256": "df2b86d51b959bf26b424b069ebaf56c37ff5b6d8354a4857443b1efebcb47fc",
      "category": "production",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Test-Changes.ps1",
      "sha256": "5b9fabad89c8ef2e810a2f4784d3b04e40962c7a59ac2d2449ef023f35e583ec",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "test-consumer.ps1",
      "sha256": "e5b7102b2c6662e30fb9d6fb1125508a10bfddb4c53051f5d35c4ec35e8ddaec",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Test-PrePush.ps1",
      "sha256": "c161c3569d27bf15c462e693d314c0e3058949b115d19889561596735c9a96ff",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "test.ps1",
      "sha256": "9cd0a4fc6d0179424f5461774916ee183d935de320bd3f996ce537556bcf587d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/capability-skip-allowlist.json",
      "sha256": "c11b73a49ef858a76e6335db4e18e2abb1b086374c6bdb8fa89853d70e0be008",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/ConsumerModules/ConsumerModules.Tests.Driver.cpp",
      "sha256": "4ba696fd015f1727b121c3cb5e584f2b17d877265a7911f3293a983726f25a06",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/ConsumerModules/ConsumerModules.Tests.NativeModule.cpp",
      "sha256": "e3998097b59b15649f801beb8c2fef8b53453149e284731d6ed638520348332a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.ClipboardTransport.h",
      "sha256": "8f0bf0da68516dae156b1a16c89641ec7722310764ff7f255a88be865856ec14",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiFocusEventClient.h",
      "sha256": "402e777dba824f6129313bd2337c765fec33ecf040b8fad93d86819c29b8fcfd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiTestHelpers.h",
      "sha256": "7b6c7349a11dcc9b48ac5a3f129beb784025fd048de9ccefead49162eb200c6a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.DxUiTestMovedControls.h",
      "sha256": "7ed7e2f8f227de220fdc97559b5d5ed9a616093ddfa2e4fdd89a7a71950ed6b2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridMultilineFixtures.h",
      "sha256": "615f3d851e354d68d66085a6168e81cd860e63e60929cfc239ab65230efdabba",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.GridMultilineRendering.h",
      "sha256": "f0a363167255c0a1315ea5514d0a15a2e7fc5e815042126fd1d63be4ca791e6b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.LocalizedLayoutFixture.h",
      "sha256": "f14e4e4dafd54df43b5207290df160c2117d94d04dba4b9f0df99d75cdf285ad",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextInputServices.h",
      "sha256": "10c0f41450d32d5a574377c2725f3e107e477b4ece9531c61b106a0f8ab8729b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.TextStoreComposition.h",
      "sha256": "ec60870948d48465544419c9a0fc155d39073e53b7a5889018e6247f3c51fc28",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.ControlTests.vcxproj",
      "sha256": "ce63772309f12c04624f19a71cc11883864fc66f69b51f21780f7117f57e77f6",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Accessibility.cpp",
      "sha256": "fce48b89bb0236cd8842f37d50523ab177be013f7a189205acb43262fb37bab8",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Animation.cpp",
      "sha256": "22445bfa531ec29a232d8e2e5a480b6f11f7ec3df048736cd565c8941613ab57",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.ComboBox.cpp",
      "sha256": "3eca13d18e5ada0b5e85e9344ef2135f37ff55f2360ff7b09c4bdd7d5bdfa234",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Controls.cpp",
      "sha256": "e03c0a6e9374f7734f0360b119ad92ac9b4d03343b73191e5df019d6bb894631",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.EditorControls.cpp",
      "sha256": "9651ce1190c5e9f2d30d146bf7707506bfbff4e696c7f99e7a7b41d038b29ca6",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Gallery.cpp",
      "sha256": "16577e6990042b855b94ba6e710a3a7c92c7772a532b9fbf68313ab941aaa56a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Grid.cpp",
      "sha256": "392831acb286886672f70fc565c4ff5bcf23a11879d7abbe84076d8255607f2e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.InteractiveLease.cpp",
      "sha256": "6e7813a0370e751561a9eeec1b1a4d4b30c8a893a4d35e0b3f61e81edf406417",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Menu.cpp",
      "sha256": "d79f1e0391190a70a1749a8d84cb47871871ac1be30e17a49c44b2fd1f8f4d14",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.MenuResources.h",
      "sha256": "8c025ee902b6caf5b6af1dfbde2ce7997ff916346351fd4fa82fb9b7eab33c48",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.MultilineText.cpp",
      "sha256": "e15f6c59d0664ff636e2b4d5bd7ce9631bd1eba75d20c3ea3c752c4b4e9daaac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.NativeTextInput.cpp",
      "sha256": "09b3a8ba36655a60a7108d1ef462d69eecb61dae8cf181ed17830bc22b548aef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.NewControls.cpp",
      "sha256": "e5aede42c305cc03622c3889ee82d1df488ad45316fc4b6e674a2593c23c4703",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.ReadOnly.cpp",
      "sha256": "d41645dbff91678b11031d60f7901e2a1aecd5bae5cbdabb12785f1fac6e229d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Rendering.cpp",
      "sha256": "475f4313f36544c2f363b0e39cf08001b4ab945a37e2ba9eeabbe4533cba68f1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Runner.cpp",
      "sha256": "dee55732f3452b9d6adf999f98a11089d027301ccafe238d5214df24f7baaedb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.TextField.cpp",
      "sha256": "e4826402bbc874c08ef57bb54fdbb05b04b074f17e209ff310078fd1e68f350b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Theme.cpp",
      "sha256": "d6160e78a88f4d29307aa289fa8a08836dd9d4df2559739a708078573b35a2cb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Tooltip.cpp",
      "sha256": "c9a258a29508a27eac35bb6606870f114f48ae3a8377cc8a0fe4aa6091d42ed4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.Tree.cpp",
      "sha256": "27da2d3bde8c8d226f998baca35b918fd58b50e8c48e5861836e6e3749376bf8",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/DxUi.Tests.WindowHost.cpp",
      "sha256": "44dc726ecf7a6b22e40b3dcd7a854c3693f3f67acebe9342793c03edc2597d2e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.FormAccessibility.cpp",
      "sha256": "1d9a05932d3600edd22c0afb5accb946b541d6a7d2ffa67c279258ec5e3e592e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.FormAccessibility.h",
      "sha256": "bea70d975efbf0bce2f76d999c34b6b78d176b46809af8061068cea507f14d01",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.Forms.cpp",
      "sha256": "09b92f152bc515ca99daec0b87ddc3fb12c2cf60ce062bab73b948b4794cc854",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.Forms.h",
      "sha256": "800b49c94d28a7b9dc806a9773341b25494d7753d596a2c9cbaf8c7980f9c08c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.GridUiaPattern.cpp",
      "sha256": "5c7fa0233ed32055015acb53bae47fdbfcd261b84fdcbaee1758bf89821f02ed",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.GridUiaPattern.h",
      "sha256": "2875f14647575d6423df34cd1449d6710220d33df0292726e6c85e28460e72df",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.HostControlRegressions.cpp",
      "sha256": "a44c1ae8965f50bb3689da2923e06a80c1683c4819de142cc8410ac4428902e3",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.HostControlRegressions.h",
      "sha256": "71cda21acdefbca35c3d69bd93238b44a8a989f221a53950698e547740741240",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.IntrinsicLayout.cpp",
      "sha256": "71680a8a743b4d20081839c375f604621e59ee9b160b932f2ca5535727f8f0f7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.IntrinsicLayout.h",
      "sha256": "168ac50dd72d8878c27d8d424f3c99b2708598a93e2253363788eb442246a71b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.MarkdownCorpus.h",
      "sha256": "d617f056df524bfc6eb76bbe990c7c83a5b594143e730c5d39f940f27861fc02",
      "category": "tests",
      "generated": true,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.MenuPopupReadiness.cpp",
      "sha256": "fa4604b87da37f426f93c3c03b4f2a2e9f5e0f90d7e2449231c1ba753c2e7d59",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.MenuPopupReadiness.h",
      "sha256": "86413cc4ad57cb5e363e93ba449a2fdd170bce38d3f732339bc13dd994a09260",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.NativeDevicePolicy.h",
      "sha256": "9f39b7b78d703eedde6ed348a4521c730e3709dab5566fdd8244e3895b9061fb",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.PreparedTreeAccessibilityEvents.h",
      "sha256": "dabba04d347eb9799999a5f45d70a6227c76550597f4626189cab2eca105e52f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichDocument.cpp",
      "sha256": "f28fe9a52e9a11570c8ffe3487f879a7b9e8e41e0c4a38d68ce3cd84f32c1bbf",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichDocument.h",
      "sha256": "06bc81affee80252523e07198cf1c4b1ae4b7a9d84c6e9b5b8d13957bce7075a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichDocumentLayout.cpp",
      "sha256": "b175d0780ae6d487403aa7cf9ffcf42f7573e41234ce23b3679d9747e915003a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichDocumentLayout.h",
      "sha256": "2a2518e1d6f1486081d92b9c7c65585e9ae35b012f2291d25fb6de1488024aee",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichMarkdown.cpp",
      "sha256": "9e5ce69e0126010bf7f33738c31330e0b7844f17f14293e7986f9ae62a62b304",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichMarkdown.h",
      "sha256": "c57c2df2a3dbae9068f1c75989ededb0417727d6354dc93df361682f9ef147bd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichProjectionBlocks.cpp",
      "sha256": "d665404ab5469f1ff7d839debe2fc946aa58611b44b70a8356031b9323b086b4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichProjectionBlocks.h",
      "sha256": "aff2d33a58e4ee190f725d0a45aa74f9c191757f91cf839e211db7d7ab1a0fa3",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemantic.cpp",
      "sha256": "3b28a5fc696c861e226b6a0291f638d23881aa8b806eaefcbef8c35bc9cc7b68",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemantic.h",
      "sha256": "20b62e004bb09d02a3a055f235922dced1c14e519236dcca0ce7370b9fb05d3e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemanticDocument.cpp",
      "sha256": "f0edce5c4f2cfb2a7c6481e1d809ef76028c52f5ede32b23a8c62363b78001bc",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemanticDocument.h",
      "sha256": "0132c82758ed7a5ba3753936e08cc62c693e73e4d4bd77ccba2af5a675ea6b72",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemanticGeometry.cpp",
      "sha256": "feb3d8abd25e9968183b3bb8311814557810bc499eb11bef7fed16e2cadee002",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSemanticGeometry.h",
      "sha256": "e9aadf9681b4914d8777c9a95e22247e3fc684aae2f27742b100cfbaf241f635",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSourceChanges.cpp",
      "sha256": "f710c841ac5d5be9288c646d5a7af5baf1458861fd5a5d261281d6efc6373b51",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichSourceChanges.h",
      "sha256": "dd68969d800689b9e06ac859dade916d76eebe393ef1a58e851671767830217f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextAccessibility.cpp",
      "sha256": "02d65dcdab6de60f85571263e181a1ee26f7ba67027e5a20d4dcde4b0f48d5f2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextAccessibility.h",
      "sha256": "51e75ae7a64a18386718b4a69c56fac5e8713b299a2cb92d34e4b9c8aad82c1d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextEditor.cpp",
      "sha256": "faefe84ac44d668a652e27cf578bcba4d9db283ea058cc12bdab8da805d6b62a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextEditor.h",
      "sha256": "f7eba1c7d65e589e81a146d1c8312f7e1548c0e695835fa019d0d511cf70dd64",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextInput.cpp",
      "sha256": "b4d791fc2ae231e171da656923dde6a6503a79c5014bb1a7549e2e7eadf36d56",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextInput.h",
      "sha256": "16c8bb007e1af52583f3203e3fe3b02830e77ae41c849c42fbc427d6c9924bb9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextLayout.cpp",
      "sha256": "56efd06b5f4e2a39752dc8fea46f92098529a813eed6097eb40706d6f40ed029",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.RichTextLayout.h",
      "sha256": "193d05a45396851b88b45a0b77124d7af53130d3f02641901e11a2046f0b42ce",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.TextElementMovement.cpp",
      "sha256": "08042ed9f3a5d08b40d491cf165b4f9eb30c57abadfc9c3c5f39c237f59f8c6f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.TextElementMovement.h",
      "sha256": "3bd8b34daa5f3fe3a02556e69a85a46f0ee6767664b5a313397d640f02b93c76",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.TextElements.cpp",
      "sha256": "3569f184f605ac0e3b4231a50be91e43640caf844e45df0e2e3f6232eba5593d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.TextElements.h",
      "sha256": "f8b224508bb27808d648bf58c158424c6252fc68c38f222bcb41d3ed0e49304a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Scope.Tests.UnicodeGraphemeCorpus.h",
      "sha256": "55ed4eff16a837bdc22e4390ddc67ec3e29fe6e22d50c8a61ab3915f3b1c52f4",
      "category": "tests",
      "generated": true,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/DxUi.EmbeddedTests.vcxproj",
      "sha256": "fbbcba65d43ab27cb63dd762223355d2b335c314922846fdc3f6ba0e9e1f4927",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.BenchmarkMain.h",
      "sha256": "ec05a1004c8968625d2b6a4d8312d9fc6da4877aa308d94e57f6bc52b3d482b2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.ComplexUiBenchmark.h",
      "sha256": "4126fdce2b80e8294a1ed880f963ec67d25546a1b321f6a1fd0c282b64c967f9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.Embedded.cpp",
      "sha256": "9ad95b9ea93afa1ba7b9ac33aa4a6a847358b5b3de3e5bed0d407f860b6a7596",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedAccessibility.h",
      "sha256": "6fb50a12ca673c6101169ce33f0d14c830bc3e48d299e9ccb28eb3a5d9479b75",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedTextInput.h",
      "sha256": "2ffb25aaa65eb0273b51a26b1c0e61e895401703ef56a0d696349fb628a8694b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedUia.h",
      "sha256": "1462308f3cd2452ac77be0c8243c5533d13cb653ac227fca446b65cbc7f6732d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.EmbeddedUiaBridge.h",
      "sha256": "183aff6d375d32f848c9cb9d5960790c6800e08fdce44b6f40c11cbd5171865c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.GridSelectionBenchmark.h",
      "sha256": "692201c512d8285ccf29f50a7b7041fa38aad9e371b46d1d2ae766cbead69a26",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.InteractionLayout.h",
      "sha256": "114a90815c9fd3650e247be5e838ac1c252f5f844e6e4e9d885bc99c37d35969",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.LocalizedLayout.h",
      "sha256": "6c21102d15b0826d7776b101c5aab2d7d95dfdb491145d501badec4ee2228922",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityBenchmark.h",
      "sha256": "ad9d87bd9677c7d6b41dae0f21a7eedd1406ab17c5e4d552298f4fac08d9c4d0",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityFixture.h",
      "sha256": "0494cf3af7ed90d4b52816e2983c565f952ac2a6c063de7c53db596579f7a8e7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityRowsFixture.h",
      "sha256": "016746cfa75fcdd3cbf1513ba7b0536caf47ac10cf70445df2490992102d8715",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilitySemantics.h",
      "sha256": "78ec16b0f2f641f03ac0d3129d2beb7d0cbb3fb99894ecb74342dd01204f9289",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeEvents.h",
      "sha256": "e9066ca1dbeebe69d8bcc81467af2948302bcbbe46d8939074322dc9c47063ff",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeRootLifetime.h",
      "sha256": "8dff9321137b6c61536ad7b03f5e000f2f41a6c43c8cc1c568d61f9db69332f4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.ComplexUiBoundaries.h",
      "sha256": "31b2e90e8a0da446cf570eef896b199e50fc7996a09c5c4144ea6b0b402c5aae",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.ComplexUiProfiling.h",
      "sha256": "beee3e4c80fb9a6fc85c2d65f699405a82cd71838cdf7c07a5c6e0d3b285f61c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.FormLayoutBenchmark.h",
      "sha256": "78af68623bad96ff47cf8a02839d9b787ce9f954de3f0db3ed6f5c95906594f2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichDocumentStorageResources.h",
      "sha256": "acc5e0701101b22711ced325c2b26cc862d7d56dab1b6ba53a1f924b9230f686",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichGeometryResources.h",
      "sha256": "e47e11ce969bca72996df2c3d94eb69f079eb33ee2d8c39cb3ef801fcc83df5e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichImageCapture.h",
      "sha256": "5fc76d8d2f5dbe6cde884f4af54ca7ac66d3b61aaffa1d3d7a0a097e3b19100a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichImageFailures.h",
      "sha256": "0055810050e1fdf338fdac59062f812b0e1b56c557563cbff1b856cd430aec3c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichProjectionResources.h",
      "sha256": "e3406c1d831e61d630326a0e4d434ecaa702cb4155e18081ca8469db3ddbf034",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichSemanticDocumentResources.h",
      "sha256": "e49adaa270b97ca4c20ab6aee31199b2a77265d282c895e58998f696606f350b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichSemanticGeometryResources.h",
      "sha256": "b501f5d0ee57d73a30520263ae190c9578d679083ef94b2ecab064fc39c6a83f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichSemanticResources.h",
      "sha256": "906ff556c1313bb54c2671121bcf341b44fb5afad706609fcfc18f317d1a6b1d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Scope.Tests.RichTextResources.h",
      "sha256": "78c6e3c4d533971fd6e367ad4ae2521ffa498531f0bdd33c97a51b6820ceb005",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/DxUi.FoundationTests.vcxproj",
      "sha256": "060c899500e304f2e5bee1514be8e66ace3f62a8d8c67d19c039be2e7890f3f7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/Foundation.Tests.Foundation.cpp",
      "sha256": "2fb3abda3ff1305177b0e3262690910cc2e5528f4012933b99c729d27923656d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Foundation/Foundation.Tests.PublicHelpers.cpp",
      "sha256": "e5812bda1c73aaa0afc2bca0277af3871e1fdf48bfea9aa524e4da95432cc82d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/DxUi.InteractiveLease.vcxproj",
      "sha256": "971abec0fab1092c3243a294df676e1049c70c92d5a8739895399a47e56ced7e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.ChildProcess.h",
      "sha256": "dec6f57e16ca012163fc0796d38d188b37eb141bbeb9c17ff6fa50511db9e167",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.Confirmation.h",
      "sha256": "f0b8570fea64a5feafd27eaedc668b5c3866ba8121fabdd03126f04bef89ce2a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.Runner.cpp",
      "sha256": "d9716dbb3324f8aa86a1f497a3aa275595554dc8386558a1dd2a61822f540477",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/InteractiveLease/InteractiveLease.Tests.WarningBanner.h",
      "sha256": "18323d806f669703cea8c0ff3fde377cd326549cc465e75a2db983b43587119e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/native-test-files.json",
      "sha256": "01c3aca460d9325d324b4d7cfd8b0381294cd8dbd462f9338da729c64b269804",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.ClipboardFixture.h",
      "sha256": "10184b263a4c988d37d17192017004c3b452e00778c01320962ccbacf2804e54",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.CountingWriteFactory.h",
      "sha256": "a81960fd1d83e77515c1ad2e4893a3cbca220295ef123f9c4902ab402a1e2eef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.FlowchartLayout.h",
      "sha256": "a4df647191d043110e9215dff2c4c95f927198990a76f466f309aa39f63aa853",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.Footnotes.h",
      "sha256": "f131f2b60c54d8ce1c2166e023242925bf71e16c22ad550347188d592b044450",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.MarkdownBlocks.h",
      "sha256": "a8b91d491b5a8609ae1ddb3206780c1cbbbc7a70ee37d34eda456840c9838bff",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.MarkdownCapacity.h",
      "sha256": "5ce29010056b90d2a55c69440244017a3afd59dc3aa0adedf56b00bd4506c88e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.MarkdownHtml.h",
      "sha256": "a4d40be77823bbc132c571247cd85b49bbca369281f10dae4d7b102349149bac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.MarkdownInlines.h",
      "sha256": "b415afb134e3141f33f82efbc6d8514f22d9a95a82b8a99f76c78976eb59793d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.MarkdownSourceCorrelation.h",
      "sha256": "3fa85960dae726967a49772bbfe06f9e883647c9b2839619d8b3165eed324cef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.NativeFlowchartCorpus.h",
      "sha256": "01256cee713abb65697535c9601dfbc1a3eed3efd2c057bb27e61f28cdce2374",
      "category": "tests",
      "generated": true,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.PreparedFlowchart.h",
      "sha256": "ef41d64f31e6444476be45f9917feefa529bf6395ee98986ece4c998b157a095",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichImage.h",
      "sha256": "1535e9a96e6a57675f9f3119305ae6f7e98385bde75f09b6106676973df3b171",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichImagePaths.h",
      "sha256": "ba1db2f67609d4fdce5d1a6b0ee79d49138eb661faa3b8beaeb42611ffc70634",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichImageResources.h",
      "sha256": "165ce889c454ae5d8ff081f6cd8d4e6f691f16f63de0afd31abc0e3b3f52a745",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichSemantic.h",
      "sha256": "462522bf82712089022591c08021ac887fbedd62b88562804d52940117983d75",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichSemanticDocument.h",
      "sha256": "1dca03e06e841512315efeb0e4b2fac2e317873a66575f0fa5837f1a3d44588a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichSemanticEditor.h",
      "sha256": "5074b6c728ecff4f004147f00154764605a192db8bf0fe8c657c9f2b2c614544",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichSemanticGeometry.h",
      "sha256": "ecf78951cbb5d7de36e582c11baa93ee87b321cc43b6a0871e19b12d8e6a3b08",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.RichTextStateReplacement.h",
      "sha256": "ff74dde6002acb577b05d4b28f56657e81b7a81bf8b935fd1fde9bd32c0eda1a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.SemanticDiagrams.h",
      "sha256": "e195d32e3a6d5ee51282a152dfcfb9d11f1fa499ca1924ae8be47dce0a3d5402",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Scope.Tests.SemanticSourceBlock.h",
      "sha256": "b4aed4a08089232eab4b19afa512bb59e0459a88f1b5cbacee5c55ead51095dc",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.DesktopLease.h",
      "sha256": "2da04048ea0b49ef97d6b0f4696423e30c99073dba5fda30dd5ec3ce18689a8c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.FailureReports.h",
      "sha256": "a69833453838901bf2fe5061daf5cdf6e607afc80649a1cd351c7788ef5d6f9a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.ForegroundThief.h",
      "sha256": "a540fedce6feeb844e1c315e3196e6288759b9005bb35c4085662cc0908e6edf",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.HeapDiagnostic.h",
      "sha256": "c9c5e000fd2a5ab888ef730959d8e88fb166177a8667f077d1ae3e1d8d6afa64",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.InteractiveLease.h",
      "sha256": "9d7e0658830db619ad9edb3ab3a34d404de3ef5e0b2c47a228f126a9947bf3e4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PerformanceCapture.h",
      "sha256": "52f23d55215ed49afa57183bd9e49f425348ebef31d0c4bd387e6017ad7b9c4a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PreparedTreeSemanticFixture.h",
      "sha256": "09704ea7fcbe7944fcf4e084d4c0678ba5e46e9284c099af50391e2cbeae61dd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SelectionEventInterruption.h",
      "sha256": "09a019cf73dba7315becec0a765fd46ca6f18881d08be5fe413d0693e80c642e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.TestWatchdog.h",
      "sha256": "a6fbc903c4f81febfa215ba65527f355a2e34b739937f502ee3e640a92c66892",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.TestWindowActivationGuard.h",
      "sha256": "28f8764f37a06db502f3f02e49fd87426d4fad00bdbb89b7e97374ca5cd442ac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.UiaTestClient.h",
      "sha256": "6f3a5400d5abe5185ec38eaacc00e53c241e0152f070793d1041d64d88874d7b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/test-scopes.json",
      "sha256": "a35b8619f45aa872b026a5dc978d5d6dff54ebc0ff2f2e9fadadca6413bf8c65",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/BenchmarkGate.psm1",
      "sha256": "cbc568f05ead14e964b9b8d7556deb5d1d27c2f255eff73a2c6c17c553ee87ab",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/CapabilitySkipPolicy.psm1",
      "sha256": "95bb89554c49b6c2c2526c21aba7d862b201fb1d637948e699d1aa493e613824",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Changelog.psm1",
      "sha256": "05e987cf29bfb8aedbda5f77feba56d0145f733ccc8c577f0fcc96e1e877dcc3",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Commit-Gallery.ps1",
      "sha256": "2e2c6ff28384d7e07d507188cd28421708bbfc0e71470844dbe8fabc6170d08c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Compare-Performance.ps1",
      "sha256": "a00bceef3722deb15acabc1f5fb2598dfa1ce662934baadb16eeb3e741a07178",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerApi/Revision4.cpp",
      "sha256": "f31a504d79394f3cf717e2702b7e59b1745744a0b1abb4f49bdb02b8bb579361",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerBuild.psm1",
      "sha256": "833607c3f020a47642dcda2485811abd86d8f1e87f0a408b82fb71d7150bd587",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/ConsumerUpdate.psm1",
      "sha256": "6a1398bbf9a8700bf9058ca46d20f0eada631252026e86e23c252917f6266ff5",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Fold-Changelog.ps1",
      "sha256": "8b84956ab9920d54f5081edf46e021fda7fc27dc287d9c72e29faa916dee47ba",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownEntityData.ps1",
      "sha256": "5db8807d744de4a3c250ca533c25a2f2e93c4132bdeff392164e29fea3061b7c",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownTestData.ps1",
      "sha256": "4e332e0227d6bbfef0fbe221a85b77f96a252a03d4ee79ca3fdefcc2aa6ea819",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-MarkdownUnicodeData.ps1",
      "sha256": "3d2dea582dc9e1fd207c4853ba2cd610d6aa07284e7899a52184b2e92dfbfb4d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-NativeFlowchartTestData.ps1",
      "sha256": "a56b9089219f81cd1b3c234a3a0a0c4cbb355819ecc3323a1b8523444fd90300",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Generate-UnicodeGraphemeData.ps1",
      "sha256": "b95dc08c5150e0a698605bb033362da1660f6ed224480808f2af18888502d2c4",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Get-BenchmarkScope.ps1",
      "sha256": "e13d9f5d1f85f15321b6c10913dc334ed9ba0a02a69367b6ec27e7574374db97",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Get-NativeScope.ps1",
      "sha256": "01dfcb03f5d9211031e6ff17ddf73f02346f79c87e0a77f6cd0edd20fa0ed9ce",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Install-ClangFormat.ps1",
      "sha256": "a9b634be43b77764e9e6f0d15cb145c9f9bb4610065053d843f1c1770af945e8",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/InteractiveRun.psm1",
      "sha256": "4beb87a7f5cd05e72c37cc4beaceaae39bd6f21caf9db4bcbc39235f9fa9fb13",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Modules/Documentation/Changelog.psm1",
      "sha256": "3e90fec9835c8df3669da0395a76065cc933f4dbd1910c5bfb05b9700390df1d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/NativeScope.psm1",
      "sha256": "35f95b66205fd5baaeedaa10eb45aceae8a919ae51584f77cd2757ae5e390772",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/PairedRun.psm1",
      "sha256": "358b7013b4a0a238ef3d3e2f757a49baa86bdb2b2fe604c84d5fda0a76dcfed5",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceAcceptancePolicy.v1.json",
      "sha256": "bac3f8a8e1b0f31fd109041029439ad2fd35531c24fa4dde33a49d3ff34241a7",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceComparison.psm1",
      "sha256": "d080952fbec4d35b8d2a26ccaf27612611876a1f3c015f360780b3065032b378",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformancePolicy.psm1",
      "sha256": "b1a4d8546be93601ea7f7042a6e4eee0d948d35091a6f85256e425369924ae4a",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/PerformanceProfiling.psm1",
      "sha256": "2a0a1c9edca392040616929155214634accf46e572633466dc7e6aaa288347ed",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Prepare-HostedArmDesktop.ps1",
      "sha256": "9cb229629883f3e2959aa855fa47f81bbc6ace6f4a0bf5ca5d2dab9021a2a7ac",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Publish-BenchmarkVerdict.ps1",
      "sha256": "56243977a0ef64badd234a599079b229689630f4186ca6c6f203667e660130fe",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/ScopedTesting.psm1",
      "sha256": "fd728facd01e19100b7b01234a4808c52e003628076ab0cda7ab42e09ab43a5e",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/SuiteFailure.psm1",
      "sha256": "86e18de96dded74172f10e6d5c45b0e19ff4ca7578281b1ffd683ffa9541f946",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-Changelog.ps1",
      "sha256": "6e457ddf10c2386f7b02f43e703472eaf9801b3684d091a0ab7c726b8026ae18",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-ChangelogWorkflow.ps1",
      "sha256": "e351c7e3d6f11373e6284b523c5561306814598556f30f8b7af4fdf2cd879e94",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Test-CiGate.ps1",
      "sha256": "e75713cb5e95977783fc0f013689ce85ed8a2e31867906b9d063db84980117aa",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/TestRunEvidence.psm1",
      "sha256": "bc0a6c2b4563ae60aeaf28fb5b4493283361973e7a7306d115f3d64242119b63",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Invoke-ToolingTests.ps1",
      "sha256": "eeb2e0570a4c7d82bc0280a797fa8a9426c1ae51cca2135e3527e53292296551",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-AsanRuntime.ps1",
      "sha256": "c83660650e6a1fbaf482b47b36a603643d991190b4f48eb4cc9318acadde12d1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BenchmarkGate.ps1",
      "sha256": "dced1b09968771116c85ec82016693500fae447a99e1a38b009af4cac31290f8",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BenchmarkHost.ps1",
      "sha256": "ee97874bb4a97ccfe1cd151d9f79e1445892458d07272ee73562be3be244709a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-BuildMatrix.ps1",
      "sha256": "81c1330283c84a25ffee178792d23a9902ca5adf10ab294dd62ed2af1ec81c4c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-CapabilitySkipPolicy.ps1",
      "sha256": "9eaaafd4d096cb83958dbdb0a1e2de6bfd9b98d2288aba9fdb9cce2321997ea1",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-CiGate.ps1",
      "sha256": "ade184704a87c9605aa02575e0845732b4269b73adb8da6a3b11e6bb070add86",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-ConsumerUpdate.ps1",
      "sha256": "4760c3dd60c46a531d4af3242b439f7e0cb1c8c60858221e2cfd63412b6b6281",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-Docs.ps1",
      "sha256": "e667ac28e2f80f529dba64702fd57323f9fc51ca70d6aeb7cdbb1ae4299f88b9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-InteractiveLease.ps1",
      "sha256": "f4f6121ad5a3e81b71985f66ea0f5873acd850d71dd7b7c05be1c94c11201589",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-InteractiveMode.ps1",
      "sha256": "cc2d8a520ceb7a436a1a652352c8477fdcef6f05c4c7004d68afa54931ae0122",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-MarkdownData.ps1",
      "sha256": "31fc079ffc669d665aea9537db714b7e8412c0197d7da4871889fb3b0cd8e035",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-NativeScope.ps1",
      "sha256": "3f49f43fa4f1c4d453624188755f3e501e8936656f39e8a29505fc4905b29f82",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PairedRun.ps1",
      "sha256": "e8a6acd3e5446fdb15fc35285caa0f610f5f1e65f08d2088cea6332b8fe31487",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceComparison.ps1",
      "sha256": "4c0e76280ab378b9bc031da114138cb28f64ec23074615c9c8474c14c01001f4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceForms.ps1",
      "sha256": "03c6d6bd14e81b83fe5f1c52238ef1ae0a21c7ce5225605c1cec81f035280e7e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformancePolicy.ps1",
      "sha256": "9ec18c5e43d920fd016a3476a73b5e4eb61429d09b8a3dd8bef827ed44f2c0c2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-PerformanceProfiling.ps1",
      "sha256": "59e1cc732d6c3a7f29f6ae0bacd68b7e48023d986061b2bc6db1fdd76c517c36",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-RunEvidence.ps1",
      "sha256": "3e4cdfe9a3f6a8952386a12e016e79a08eb91786a804778ea18483c7cc6409e2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-ScopedTesting.ps1",
      "sha256": "6930ecce087d0cf9e52b1f1bdb4c7435655f5dba2d6a7da065b30d8c1bffc4f9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-SuiteFailure.ps1",
      "sha256": "5c4159c466d614bcc83dd5a8c411275fb14fec92fc1644f340cda0a906859201",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestFilter.ps1",
      "sha256": "2a1608b9382a10be8ca4c7136af8ef80be93c597191c428ce8bf1a57cbcf0834",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestPort.ps1",
      "sha256": "abdd1e7d3990a1ba148ae83357a8d5d40ad863e4e4de3090da933a28e8477d23",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-TestWatchdog.ps1",
      "sha256": "af65119f893743c4140a4d6a0d8442769f3ffc1f8c30eea6e1c635c40fce4dfd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-UnicodeGraphemeData.ps1",
      "sha256": "5316963582c72f6a20121d30102aa93a5d1d6935c4dd94138fe1ce8421a8832b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-Validation.ps1",
      "sha256": "629c3ae4201d298903bb86f5fd02dd830a8a7c20a1a88aa42bd8efa58194b141",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-VcpkgTriplet.ps1",
      "sha256": "26c947b5d024ed41a0522cd4de34f4b242a5f29d22f540f91ce31473af61d6ba",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/TestSupport.psm1",
      "sha256": "7b85c5f706cfd12e2381d831ff10a2d28712c89e2bc670f2019815e3306916ef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/validate_consumer.ps1",
      "sha256": "b8f4882a1c5183250dd5437570e696ac995bad04c46d4575cafc0a65fe97a82a",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Validation.psm1",
      "sha256": "15c7ef95430c293400859446cd51e2202bbe5d0ab1ac506b8539ce1de983e756",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/VcpkgTriplet.psm1",
      "sha256": "7bb17cc7dd2377e1eb4cc62de710b7ecff9e614ff7b6dc31cd32ccc47af488b7",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/VisualStudio.psm1",
      "sha256": "bf2ac054c30feaea2b8cc1322fc1d67854660e90fc2dcce17e26ee21ad6071fd",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/Watch-BenchmarkHost.ps1",
      "sha256": "dd31f0ceff0a383fee1dc971893a85ad803a6962d6ad2c9f0b34007dd156807d",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate-build-matrix.ps1",
      "sha256": "621dc39261b63c43ab6c2b4ea532e5340969910bc96d5e5ab5acad672f532a44",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate-dependencies.ps1",
      "sha256": "c6dffa8774dadfbe63ed9e3e5889eeae1993570edd74f01e08d40b87ce687bb1",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate-skills.ps1",
      "sha256": "cce3e47124352398a60bb7c79f286341e659ba00b0db853aa8672c72edf6253f",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate-specs.ps1",
      "sha256": "139b7257056fb441be1dbdc36649886b9c86b235c0ffb8707078ad073d7d889a",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate-test-port.ps1",
      "sha256": "ed545a8aca8b290b041bbed5de84398a990ff7fbc7b8b1e9c13cd2674502c2b3",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "validate.ps1",
      "sha256": "68ba34c47acccc7742b0884ae78d8de69f7f9a6c772c74c53c4ef00373d1d2b2",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-configuration.json",
      "sha256": "7c8f088b33838d9f1c6481d2120bcf8445cdee45232779fbb9b20964caa54de3",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-install.ps1",
      "sha256": "be905aeb852327bf4823d8ce82c6e3cf10103980767f9044ce6675ad67207fec",
      "category": "build-and-tooling",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "vcpkg-tool.json",
      "sha256": "e38884614404c881238070ce5ae2c25a72a68718a3b03c8dccb26fa22586756e",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    },
    {
      "repository": "DxUi",
      "path": "vcpkg.json",
      "sha256": "fe54dd43ba2d384eee8a0034daa3e22f0204ffc4390be1cd8eb8456e295b6bfc",
      "category": "data-or-resources",
      "generated": false,
      "reviewStatus": "pending",
      "reviewer": null,
      "testIds": [],
      "findingIds": [],
      "disposition": null,
      "reviewNotes": {
        "callers": null,
        "ownershipAndLifetime": null,
        "failureAndCancellation": null,
        "testCoverage": null
      }
    }
  ],
  "reviewStatusValues": [
    "pending",
    "reviewed",
    "generated-verified"
  ]
}
```
