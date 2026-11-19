- Converged the root files with every RedSalamanders repository (convergence CV-6-03, owner decision D18 and the
  B-12 default): the third-party notices move from `Dependencies/Notices.md` to a root `THIRD-PARTY-NOTICES.md`,
  corrected for what pin `21fae0dc` compiles in (libfyaml with xxHash instead of rapidyaml, the Unicode and WHATWG
  tables) and for the bundled Visual C++ runtime, and `package.ps1` stages it beside `LICENSE.txt`. The common
  `CONTRIBUTING.md`, pull request template and `.vsconfig` are added. Validated with `validate.ps1` and
  `Tools/Test-Changelog.ps1`; native packaging runs in main qualification.
