# Skill packages and scoped guidance

Status: normative repository validation contract
Last reviewed: 2026-10-28

`validate.ps1` checks skill structure, required package files, local references and scoped-guidance inventory through
`Build/SkillPackageIntegrity.psm1`, then runs its focused negative/positive fixtures. This is tooling evidence;
it does not establish that a model makes good decisions or that native product behavior is qualified.

## Package structure

Every direct directory under `.agents/skills/` contains `SKILL.md` with fenced YAML frontmatter, a lowercase
hyphenated name matching the directory, a nonempty string description and a nonempty instruction body. Required
strings retain the skill format's length limits. Duplicate keys, typed descriptions and unsupported syntax fail.
The bounded local YAML grammar supports scalar values and one two-space-indented mapping; quoted strings use
single quotes with doubled apostrophes or JSON-compatible double quotes. Block scalars, aliases, tags, sequences
and deeper mappings are explicitly unsupported. Extend and test the grammar before adopting another form.

RedTilde's existing package convention requires `agents/openai.yaml`, with nonempty string `interface.display_name`,
`short_description` and `default_prompt`; the prompt must invoke that package's skill. These are UI metadata, not
independent specialist agent definitions. Optional metadata fields remain available in the supported mapping form.
The optional header `metadata` field must be a mapping; `license` and `allowed-tools` are strings in this subset.
Optional UI `policy` and `dependencies` sections must be mappings. Policy supports a boolean
`allow_implicit_invocation`; quoted booleans are not booleans. Dependency sequences remain outside the bounded
local grammar and fail explicitly. This validation does not change any skill's invocation policy.

All package Markdown and inventoried guidance files have inline and reference-style Markdown destinations checked. Encoded
filenames, balanced/escaped parentheses and angle-delimited paths are supported. External/protocol-relative URLs
and fragment-only links are not fetched. Quoted/parenthesized inline titles and next-line reference destinations
retain their target checks. Fenced examples and matching inline code spans within a blank-line-delimited paragraph
are ignored; unmatched backticks cannot pair across that paragraph boundary. File URI paths are decoded once,
preserving literal hash and percent characters. Missing local targets and
references escaping this repository fail; heading existence, command signatures and model behavior are not inferred.

## Scoped guidance ownership

`.agents/guidance.json` lists the root and scoped `AGENTS.md` files exactly once, using repository-relative paths.
The discovered owned files must equal the inventory and each file must be nonempty. New or removed scopes update
the inventory with the owning guidance change. Path segments may contain spaces and Unicode; rooted, traversing,
duplicate or invalid filename segments fail. Discovery excludes `.git`, generated `.build`, nested checkouts and
reparse directories; a dependency or test fixture must not become repository guidance by accident.

## Regression coverage and outputs

`Tests/Tooling.Tests.SkillPackages.ps1` exercises missing fences, empty bodies, duplicate names, wrong names/types,
invalid UI metadata, missing procedure files, scope deletion/addition/empty content and ownership exclusions.
The parser controls include balanced filenames, inline-code pseudo-links and protocol-relative URLs. Reparse
entrypoints are refused before opening them. The supplied root is admitted before directory discovery or metadata reads, and hidden skill
directories receive the same structural validation as visible packages. Quoted ISO timestamps remain strings on
all supported PowerShell 7 runtimes. Link creation is explicitly skipped only on hosts without that
capability, without adding the skipped control to the passing count. Every test link targets the same owned fixture
and is removed as a link before ordinary fixture cleanup.
Only the exact independent module/fixture paths `Build/SkillPackageIntegrity.psm1` and
`Tests/Tooling.Tests.SkillPackages.ps1` bypass generic Build policy to select `Tooling` alone. Shared
`Build/ScopedTesting.psm1` and unknown Build inputs continue to select conservative full coverage; planner fixtures
exercise both the narrow exception and those retained fallback cases.
Fixtures use a unique owned directory under repository `.build/skill-package-fixtures`, checked before cleanup.
The actual corpus check is read-only; fixtures do not build native code, initialize dependencies, use the network
or acquire desktop focus. Run `Test-Changes.ps1 -Explain` to account for affected tooling; native qualification
and physical-input/performance acceptance remain separately owned.
