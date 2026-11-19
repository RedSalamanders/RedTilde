- Retired four repository-local agent skills now folded into shared skills in `RedHome` (convergence CV-2-12):
  `build-redtilde` into `native-build`, `file-safety` into `file-io-safety`, `settings-crashes` into `app-settings`
  and `crash-dumps`, and `test-harness` into `test-evidence`. Every rule of the four is in the shared skills, with
  RedTilde's contracts, paths and product rules in each one's `references/repository-facts.md`. Claude Code (the
  `redhome` plugin), Codex, and Copilot list the shared skills from this repository. `AGENTS.md` points at them. No
  build, test or product input changes.
