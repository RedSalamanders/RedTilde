# Skill packages and root guidance

Status: normative repository validation contract
Last reviewed: 2026-11-18

`validate.ps1` checks skill structure, required package files, local references and the single root guidance file through
`Build/SkillPackageIntegrity.psm1`, then runs its focused negative/positive fixtures. This is tooling evidence;
it does not establish that a model makes good decisions or that native product behavior is qualified.

## Package structure

Every direct directory under `.agents/skills/` contains `SKILL.md` with fenced YAML frontmatter, a lowercase
hyphenated name matching the directory, a nonempty string description and a nonempty instruction body. Required
strings retain the skill format's length limits. Duplicate keys, typed descriptions and unsupported syntax fail.
The bounded local YAML grammar supports scalar values and one two-space-indented mapping; quoted strings use
single quotes with doubled apostrophes or JSON-compatible double quotes. Block scalars, aliases, tags, sequences
and deeper mappings are explicitly unsupported. Extend and test the grammar before adopting another form.

A package carries no Codex UI metadata: an `agents/openai.yaml` file fails validation, because no RedSalamanders
repository uses one (decided as "none" on `RedHome` convergence register row A-11). Optional metadata fields remain
available in the supported mapping form. The optional header `metadata` field must be a mapping; `license` and
`allowed-tools` are strings in this subset. This validation does not change any skill's invocation policy.

All package Markdown and inventoried guidance files have inline and reference-style Markdown destinations checked. Encoded
filenames, balanced/escaped parentheses and angle-delimited paths are supported. External/protocol-relative URLs
and fragment-only links are not fetched. Quoted/parenthesized inline titles and next-line reference destinations
retain their target checks. Fenced examples and matching inline code spans within a blank-line-delimited paragraph
are ignored; unmatched backticks cannot pair across that paragraph boundary. File URI paths are decoded once,
preserving literal hash and percent characters. Missing local targets and
references escaping this repository fail; heading existence, command signatures and model behavior are not inferred.
Opening backtick fences require info strings without backticks; line-leading triple-backtick inline spans do not
hide later prose. Tilde-fence info strings may contain backticks.
Fence markers use zero to three leading spaces (optionally followed by the supported list marker); four-space
or tab-indented code cannot open or close a fence over surrounding prose. This remains a bounded package parser.

## Root guidance

The root `AGENTS.md` is the only guidance file, as in every RedSalamanders repository (`RedHome` convergence register
row A-12). It must exist and be nonempty, and its local links are checked like package Markdown. A nested `AGENTS.md`
fails validation and names its path; fold its rules into the root file instead. `.agents/guidance.json`, the former
scoped-guidance inventory, also fails. Discovery excludes `.git`, generated `.build`, nested checkouts and reparse
directories, so a dependency or test fixture never counts as repository guidance.

## Regression coverage and outputs

`Tests/Tooling.Tests.SkillPackages.ps1` exercises missing fences, empty bodies, duplicate names, wrong names/types,
a refused UI metadata file, missing procedure files, missing, empty or nested guidance, a refused inventory and ownership exclusions.
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
