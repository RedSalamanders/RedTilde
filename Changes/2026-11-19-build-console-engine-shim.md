- Import the build console from the pinned shared CI engine (convergence CV-5-03). `Build/BuildConsole.psm1` is now the
  engine's module shim (`Templates/Project/Build/BuildConsole.psm1`): `build.ps1` and the tooling tests keep importing
  the same path and get the same four functions from `.ci-engine/Tools/Modules/Ci.BuildConsole.psm1`, which is this
  repository's former implementation unchanged; the import refuses with one message when that checkout is missing, at
  another commit or changed. The mocked build-receipt fixture copies the engine implementation. Tooling-only; no
  compiler or linker setting changes.
