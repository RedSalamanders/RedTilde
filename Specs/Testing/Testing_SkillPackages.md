# Skill packages and root guidance

Status: normative repository validation contract
Last reviewed: 2026-11-24

Run the root skill validator directly. The full repository validator and compiler-free pull request checks call the
same command.

```powershell
./validate-skills.ps1
```

The pinned CI engine owns agent guidance rules G1 through G13 in its ProjectCommands specification. It checks skill
metadata and the “Use when” description, supporting files and links, PowerShell example parameters, repository
guidance and pointers, and plan indexes. The validator reads repository files without executing the examples.

RedTilde keeps its product rules in the root [AGENTS.md](../../AGENTS.md). [The guidance inventory](../../.agents/guidance.json)
declares that file. Each package under .agents/skills has a SKILL.md and includes any supporting files it references.

The CI engine runs the shared parser and package regression fixtures on Windows and Linux. RedTilde's scoped planning
fixtures verify that changes to the validator entry points select Tooling. Use the product's native suites for runtime evidence.
