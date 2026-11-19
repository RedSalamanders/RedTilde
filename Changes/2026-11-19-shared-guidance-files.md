- Agent guidance now matches the other RedSalamanders repositories (`RedHome` convergence step CV-6-02). There is
  one identical `.github/copilot-instructions.md` pointer to `AGENTS.md`, and `## Skills` (formerly "Focused
  skills") carries the shared-skill pointer directly under it. The drive-root, scoped-testing, Changes, native build
  and Unified CI sections use the text every repository shares. A new shared "Interactive desktop tests" section
  states the focus rule, and RedTilde's own scoped-testing facts move into Workflow. The five nested `AGENTS.md` files
  and `.agents/guidance.json` are gone: their rules are in a new "Folder guidance" section, and
  `Build/SkillPackageIntegrity.psm1` now refuses nested guidance or an inventory, with its fixtures and
  `Testing_SkillPackages.md` updated. The common `.gitignore` block ignores `.claude/settings.local.json` and
  `.claude/scheduled_tasks.lock`, and `capabilities.json` says `supported` instead of `implemented`. Nothing compiles
  differently.
