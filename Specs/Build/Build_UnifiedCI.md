# Unified organization CI contract

Common sections are identical in every RedSalamanders repository; only the Project adapter carries this repository's facts.

## Ownership and engine pin

This specification owns the project's CI routing. The versioned implementation and policy tests live in
[RedSalamanders/.github](https://github.com/RedSalamanders/.github). The exact reviewed full commit is recorded in
.ci/project.json and every reusable workflow caller. Update those pins together through a reviewed PR. Older references
to automatic native PR matrices or nightly native runs are superseded by this contract.

## Inputs and classification

.ci/project.json declares exclusions and runtime exceptions. Prose, plans, historical measurements and Changes fragments
normally do not require a native build. Shipped schemas, test fixtures, runtime examples, generated build inputs,
resources, dependencies and tooling remain meaningful, including runtime files with .md suffix. Do not exclude all of
Specs or docs without retaining their product/runtime exceptions.

The pinned shared conformance rejects extension-wide Markdown exclusions. Pin maintenance migrates only the original
global default to owned prose locations and known guides. Runtime/test include exceptions retain precedence; historical
artifacts keep their original source identity.

## Pull request checks

PRs run changed-source clang-format feedback and the reviewed compiler-free static subset. Formatting drift fails and
retains a patch. The explicit Commit PR formatting action accepts an open same-repository PR, checks its current head,
commits only selected formatted source paths by ordinary fast-forward push and explicitly dispatches fresh compiler-free
checks for that exact head. Forks retain a patch for their author. PR checks never count as native acceptance.
The adapter's Static phase calls root `validate.ps1 -CompilerFree`. Use the same command locally
for the project configuration, workflows, changelog and skill checks. The complete `validate.ps1`
run includes compiler-dependent tooling in main qualification.

## Main admission and qualification state

After main has remained unchanged for five continuous minutes, its cheap coordinator submits one native admission.
Cumulative native identity uses the complete committed Git input tree, file modes, dependencies, project adapter and
exact engine/policy pins. It is not a truncated files API response or last-merge-only diff. A code merge followed by
prose still requires qualification. Excluded-only edits retain the previous actual success, failure, cancellation or
pending status. Failed unchanged inputs require an explicit fresh retry. Unknown paths are meaningful; unreadable inputs
fail closed. Renames inspect both paths and deletions count. There is no maximum-deferral override while merges
continue.

## Native qualification

The native workflow has a separate repository-plus-platform concurrency group that never cancels running jobs; it uses
GitHub's bounded `queue: max`, which is not an unlimited queue or a promise that jobs can never be cancelled by the
service. New pushes may cancel only obsolete cheap coordinators. A stale queued automatic candidate exits before
provisioning a compiler. Each admitted pipeline freezes one commit, input digest, engine and version. Both native x64
and native ARM64 execute:

1. Release with test targets/hooks enabled; matching source/dependency/toolchain/build evidence is retained.
2. Actual normal regression execution through the canonical test harness and an always-on Release failure probe.
3. A global AND gate verifies both architectures before either production Release job may begin.
4. Separate production-only Release outputs, clean-extraction native smoke and ZIP artifact retention.

These are separate test and production roles. They intentionally compile production code twice; neither a PR matrix nor
a second push matrix repeats the same role. Test receipts and production artifacts never share mutable cached object
outputs. Exact dependency binary cache keys include the engine, native architecture, manifests, locks, restore/toolchain
scripts, props/targets and adapter closure and remain the preferred lookup; a fallback may supply older package archives
within the same runner OS, host architecture and target platform. Cache reuse cannot waive a test gate.

Receipts bind repository, exact source/input identity, workflow run, architecture, role, count unit, positive
discovery/execution, active failure probe and artifact digests. Canonical per-case evidence remains authoritative where
the harness provides it; suite counts are explicitly suite counts, not invented native case counts.
Missing/zero/filtered execution, an unexpected skip, timeout, disabled failure checks or an identity mismatch cannot
qualify production. A focused pass is not Full repository or release qualification.

## Manual and extended qualification

Debug, ASan, paired benchmarks, repeated stress, and other extended obligations run through explicit manual
qualification. Their canonical warnings, native-host rules, budgets, evidence preservation and desktop leases remain
unchanged. There are no scheduled native builds.

## Dependency inputs

The pinned shared engine restores vcpkg's ABI-indexed package cache for normal and manual native qualification. Exact
engine/input keys are preferred; fallback lookup stays within the same runner OS, host architecture and target platform,
and the pinned vcpkg restore still selects each package by its computed ABI. A miss rebuilds the package. This supplies
dependency inputs only: product objects, SDK outputs and build/test receipts are not restored by this cache. Dependency
pins, native execution, failure detectors and production/publication gates remain required. The owning behavior is the
shared engine's Specs/CI.md.

## Evidence upload

Qualification uploads retain adapter evidence separately from workspace logs. Never mix an external sandbox root with
workspace paths in one artifact input; the Windows drive-root common ancestor can become drive-relative. This preserves
diagnostics after failure without moving test data or bypassing native acceptance.

## Publication

Publication is an explicit trusted-main action, defaulting to prepare-only. Select an original successful qualify.yml
run with complete matching receipts and retained ZIPs for both native architectures. Publication promotes those exact
bytes without building, restamping, signing afterwards or substituting an older green run for a later failure.
Missing/expired artifacts require a fresh explicit qualification. Existing conflicting tags, assets or winget
submissions are retained for review and never overwritten. Product releases publish ZIP assets only. Only the five
application products have hosted publication and winget support; the SDK producer, DxUi, keeps its SDK ZIPs as internal
artifacts and has no publication workflow. winget requires anonymous immutable release URLs, matching ZIP hashes, the
configured nested executable, an explicit product license and a working publication credential. Private visibility or
unavailable credentials are readiness gates, never permission to change visibility, billing, licensing or access.

## Security and dependency monitoring

Scheduled dependency, security and status automation remains enabled; CodeQL uses explicit C/C++ build mode none, which
does not invoke a native compiler. Compiler-free CodeQL does not analyze generated code as completely as a build-based
scan:
[GitHub build-mode contract](https://docs.github.com/en/code-security/how-tos/find-and-fix-code-vulnerabilities/manage-your-configuration/codeql-for-compiled-languages).

The organization's shared engine runs a weekly and explicit manual metadata observer across all six default branches. It
inventories the coordinated engine SHA, vcpkg baseline/tool/floors/overrides and custom dependency locks, and reports
available producer updates as advisories. It never restores or compiles native dependencies, edits pins or treats an
upstream main commit as qualified. Missing/private or undeployed repositories remain explicit incomplete coverage.
Built-in dependency review and security alerts retain their supported scopes; custom lock inventory is not a
native/transitive vulnerability scan. See the common
[dependency status contract](https://github.com/RedSalamanders/.github/blob/main/Specs/CI.md).

## Optional isolated native shadow execution

The generated manual `integration-execution.yml` caller runs one frozen candidate under a unique branch pointing at
trusted current main. Trusted operator admission checks exact candidate/controller/caller identity before requesting
read-only native x64/ARM64 test execution. It retains original attempt-bound inputs, receipts, profile bytes and
diagnostics. Shadow execution supplies no required merge check, local coverage delegation, production/package
authorization or main-result adoption. Those gates remain with the current contracts until their separate implementation
and hosted enforcement are qualified. Use the exact pinned shared engine's `Specs/Integration_Candidates.md` and
`Tools/Start-CandidateShadow.ps1`; no caller may run candidate code in a main cache/writer context or cancel another
execution.

## Emergency PR merge

An organization owner may explicitly bypass PR merge checks during a recorded emergency through the GitHub PR override
or gh pr merge --admin --match-head-commit FULL_SHA. Record the reason, blocked checks, exact reviewed head, rollback
and follow-up in the PR and merge commit. The default-branch ruleset uses OrganizationAdmin with pull_request bypass;
automation receives no bypass permission and direct/force pushes stay protected. Failed checks remain their actual
results, and normal main qualification still runs. Production ZIP/SDK and publication requirements remain unchanged.

Follow the common
[emergency merge runbook](https://github.com/RedSalamanders/.github/blob/main/Specs/EmergencyMerge.md), including the
reason file and exact-head CLI procedure. No emergency merge is an ordinary fallback after a failed merge attempt.

## Local commands

The engine implements some product commands once, and a product runs them from its pinned `.ci-engine` through a shim
that keeps the command's name and parameters, as the engine's
[ProjectCommands.md](https://github.com/RedSalamanders/.github/blob/main/Specs/ProjectCommands.md) specifies. A product
adopts a command by copying the template from its pinned engine, in one pull request that replaces its copy. The five
changelog shims are `Tools/Test-Changelog.ps1`, `Tools/Fold-Changelog.ps1`, `Tools/Submit-ChangelogFold.ps1`,
`Tools/Release-Changelog.ps1` and `Tools/Test-ChangelogWorkflow.ps1`. `Test-Changelog.ps1` is read-only and runs while
editing, with `-Base origin/main` in PrePush, and with `-Base` in the static phase of the compiler-free PR checks; main
pushes run no separate changelog comparison. `Fold-Changelog.ps1` folds fragments on the default branch after merge, and
`Submit-ChangelogFold.ps1` performs that fold in a temporary worktree and opens its pull request; hosted workflows never
fold. `Release-Changelog.ps1` is a separate release-only change after every fragment is folded on the base.
`Test-ChangelogWorkflow.ps1` runs the changelog fixtures in engine CI and in each product's tooling tests through its
shim.

## Hosted acceptance

Implementation closeout does not qualify a product. Actual hosted acceptance remains source-bound and is tracked
separately in the organization hosted-acceptance plan. Source changes, tests written, static checks, and the shared
engine's own behavior tests alone do not establish that all six native product pipelines passed.

## Project adapter

### Manifest and entrypoint

None.

### Workflows

Protection verification MUST inspect the effective rules for `main`, including repository or organization rulesets,
rather than relying only on the classic branch-protection endpoint. A missing classic policy alone does not establish an
unprotected branch. Verify pull-request enforcement, deletion/non-fast-forward refusal and the required
aggregate/security contexts against actual provider state; retain the dated read-only result separately from this
contract. A settings audit does not change visibility, waive checks or qualify native code.

### Static phase

None.

### Test build and regression

The adapter preserves exact source, dependency, public-header, compiler-context and artifact attestation.
RedTildeBuildNumber freezes the third native version field and participates in build input identity. RedTildeEnableTests
is also input-bound. `ENABLE_TESTS` compiles the reviewed native test catalog. Debug and ASan Debug enable it by
default. Release builds enable it through `build.ps1 -EnableTests` when the direct tests, shared scoped runner or
qualification request it. In that Release profile, Utf8.CI_ASSERTION_PROBE verifies that Require throws.

### Production and packaging

ProductionOnly removes Core/Files/DesignRefresh/DesignInteractive/RemoteImages test sources and refuses self-test and
interactive-test dispatch. Supported hidden capture and performance diagnostics remain product capabilities. Packaging
bundles the selected VS VC145 CRT, validates extracted executable bytes/version, renders a hidden fixture and verifies
every test-only entry point's rejection. Packaging emits the qualified portable ZIP without altering documented capability bounds.

`package.ps1` requests `build.ps1 -Configuration Release -ProductionOnly` by default. `-SkipBuild` accepts only an
otherwise valid receipt whose `testsEnabled` is the Boolean `false`; a test-enabled artifact is refused before creating
package staging. Test-enabled Release remains required for regression execution. The production artifact MUST
include the original package receipt as `portable-smoke.json`, preserving its exact bytes, positive-capture result and
individual test-only refusal records. Admit its platform, ZIP SHA-256, Boolean production/test mode, the complete
fifteen-case refusal union and each observed successful outcome before writing a successful outer CI receipt. Missing,
duplicate or unsuccessful cases fail admission. A missing or mismatched original receipt fails the production role. An
outer `smokePassed` Boolean alone does not establish independently reviewable clean-extraction evidence. Production
launch returns 2 when parsing self-test/list/scope, interactive-test, crash, recovery-crash/restart, preferences,
sanitizer or linked-save crash/restart switches, before application profile resolution, crash-handler installation,
probe dispatch or window creation. Operand-taking test switches also refuse with missing operands. The intentional
sanitizer implementation and crash/sanitizer dispatch compile only in test-enabled profiles. This boundary does not
disable supported hidden captures, benchmarks, performance/resource diagnostics or normal RedMonitor integration. The
retained performance implementation is product diagnostics, so the old blanket demand to remove every file under
`Tests/` from every Release role is superseded by this ownership rule.

### Publication

Application publication uses WINGET_TOKEN. The owner's 2026-10-31 policy selects the current public organization
repositories except DxUi for organization WINGET_TOKEN access; all six repositories are public, and private applications
remain excluded until their separate public transition. Repository-level secrets override the organization secret and
may be removed only with explicit owner authorization; no repository override exists. The explicitly dispatched
trusted-main RedSalamander promote-publication-credential.yml action retains its historical filename but now performs
manual read-only verification: authenticate the effective token with GET /user and inspect its declared classic
public_repo or repo scope. It never exposes account/token data, writes a secret, checks out code or publishes. Missing
permissions or failed native eligibility remain open acceptance gates.

### Dependencies and SDK role

Admitted matching-native main Release test and production builds can prepare the explicitly distributed SDK at the
unchanged application lock. Build/package pass the same independent descriptor; runtime admission reopens its accepted
binding. Only qualified v2 inputs support Tilde's evaluated before/after snapshots; legacy inputs and exact
compatibility misses retain source. Corrupt data refuses. Static, Debug/ASan, local default and manual qualification
keep source. Both native regression and production/package branches, failure detectors and domain gates remain required.

The deployed engine admits only the three reviewed QualifiedAbiV2 producer attempts with successful x64 and ARM64
qualification. Eligible Release roles authenticate the original run/attempt, caller/controller, archive digest and
independent ABI reference before reuse. An SDK hit supplies dependency inputs; all product regression, detector,
production and extraction gates still run. Record actual recipe selection on each architecture: Source fallback does not
establish live SDK-hit acceptance. Local Debug/ASan remain Source.

### Manual diagnostics

Manual diagnostic qualification selects `platform=All` (the compatibility default), `x64` or `ARM64`. A compiler-free
selector rejects other values before native setup. Single-architecture evidence establishes only that recorded scope and
does not advance the normal two-architecture Release/production gate. Explicit owner approval may scope delivery to all
three x64 runtime profiles and all three ARM64 compilation profiles while setting ARM64 runtime aside; record that
decision and retain older runtime failures in the delivery ledger.

### Dated decisions

None.
