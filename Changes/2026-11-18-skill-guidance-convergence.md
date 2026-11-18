- Every local skill description now says what the skill does and then when to use it ("Use when ..."), the form
  the shared `RedHome` skills already use, because an agent picks a skill from its description alone (`RedHome`
  convergence step CV-2-13). All eight skills gain the clause, taken from each skill's own body; only their
  front-matter descriptions change. Skill packages also stop carrying Codex UI metadata (CV-2-11, register A-11
  decided "none"): the eight `agents/openai.yaml` files are removed, and `Build/SkillPackageIntegrity.psm1` now
  refuses such a file instead of requiring one, with its fixtures and `Specs/Testing/Testing_SkillPackages.md`
  updated to match. Historical receipts under `Measurements/` keep the hashes they recorded. `validate.ps1` and
  the skill package fixtures pass; no build or product input changes.
