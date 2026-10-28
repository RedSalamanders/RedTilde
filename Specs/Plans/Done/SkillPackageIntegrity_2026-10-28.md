# Skill package and scoped-guidance integrity

Date: 2026-10-28. Source inspected: `6079cd8` plus current working-tree bytes.
Status: complete for the bounded structure/package/guidance scope; the aggregate environment failure below is retained.
Scope: A08's demonstrated missing-fence, empty-body, duplicate-key and unvalidated scoped-guidance cases.
No application behavior, dependency pins, native test code, screenshots or desktop input changes.

## Checklist

- [x] Replace substring checks with bounded metadata parsing and required UI package validation.
- [x] Inventory root/scoped AGENTS and validate owned package/guidance local routes.
- [x] Add isolated structural, package and scope positive/negative fixtures; actual corpus passes.
- [x] Integrate checks and fixture execution through `validate.ps1` and publish the normative contract.
- [x] Record the dated Changes fragment.
- [x] Account for exact tooling-only package paths while shared/unknown Build policy remains conservative.
- [x] Run focused combined validation, preserve the broader aggregate failure, fold/check and move this record to Done.

## Verification recorded

`Tests/Tooling.Tests.SkillPackages.ps1` passes 48 fixtures, including the independent review's balanced paths,
inline-code links, protocol-relative URLs, optional metadata shapes and scoped paths containing spaces. A same-fixture
reparse entrypoint is also rejected before reading; no controls were skipped on this host. The actual corpus reports nine skills,
six inventoried AGENTS files and zero problems. Fixtures reject the original malformed structures and missing/
new/empty scoped guidance; generated `.build` and nested-checkout files do not create spurious scopes.
`Test-Changes.ps1 -Explain` succeeds. Exact package module/fixture inputs now select Tooling alone; shared
`Build/ScopedTesting.psm1` and unknown Build inputs retain full selection. The combined scoped-planner suite passes
40 cases. This tooling-only work does not start product builds or foreground tests.

The first aggregate `validate.ps1` run failed solely on `Evaluated Release compiler and linker policy`:
`Restored ARM64 dependency identity differs from the actual canonical compiler/SDK discovery.` The skill/package,
guidance, links, scope, syntax and other aggregate blocks passed. The retained dependency was not restored or
rebuilt to hide that failure. Later local-reference boundary cases passed through the updated focused script;
they do not turn the earlier aggregate failure into a repository pass.

Independent parent follow-up, 2026-10-28: after the separate tooling-migration owner restored the ARM64
dependency identity, the parent reran the current public `./validate.ps1` to completion (exit 0,
`All RedTilde validators passed.`). The 48 package fixtures and 40 scope controls also passed independently.
This later environment-specific success preserves the earlier failed outcome; it neither qualifies native
runtime behavior nor changes the skill/tooling scope of this completed record.

## Durable owner and limits

`Specs/Testing/Testing_SkillPackages.md` owns grammar/package/inventory requirements and validation limits. The
separate tooling migration retains its Python-probe/planner ownership. Structural checks do not establish that
a model follows the instructions or that native product/platform/performance gates pass.
