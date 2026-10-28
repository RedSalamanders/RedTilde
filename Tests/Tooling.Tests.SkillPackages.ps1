#Requires -Version 7.0
[CmdletBinding()] param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = Split-Path -Parent $PSScriptRoot
Import-Module (Join-Path $repository 'Build/SkillPackageIntegrity.psm1') -Force
$prefix = "---`nname: example-skill`ndescription: Read the owning contract.`n---`n"
$cases = @(
    @{ Name = 'valid'; Text = $prefix + 'Read the contract.'; Valid = $true },
    @{ Name = 'Unicode'; Text = $prefix.Replace('Read the owning contract.', 'Préserver 日本語.') + 'Read.'; Valid = $true },
    @{ Name = 'double quoted timestamp'; Text = $prefix.Replace('Read the owning contract.', '"2026-10-28T09:00:00Z"') + 'Read.'; Valid = $true },
    @{ Name = 'CRLF'; Text = ($prefix + 'Read.').Replace("`n", "`r`n"); Valid = $true },
    @{ Name = 'optional metadata'; Text = $prefix.Replace('description: Read the owning contract.', "description: 'Preserve: the contract'`nmetadata:`n  author: Example") + 'Read.'; Valid = $true },
    @{ Name = 'ordinary words'; Text = $prefix.Replace('Read the owning contract.', 'Notify owners of yesterday events.') + 'Read.'; Valid = $true },
    @{ Name = 'optional metadata scalar refused'; Text = $prefix.Replace('description: Read the owning contract.', "description: Valid`nmetadata: invalid-scalar") + 'Read.'; Valid = $false },
    @{ Name = 'optional license map refused'; Text = $prefix.Replace('description: Read the owning contract.', "description: Valid`nlicense:`n  nested: value") + 'Read.'; Valid = $false },
    @{ Name = 'missing fences'; Text = "name: example-skill`ndescription: valid`nRead."; Valid = $false },
    @{ Name = 'empty body'; Text = $prefix; Valid = $false },
    @{ Name = 'duplicate conflicting name'; Text = $prefix.Replace('description:', "name: wrong-skill`ndescription:") + 'Read.'; Valid = $false },
    @{ Name = 'wrong name'; Text = $prefix.Replace('example-skill', 'wrong-skill') + 'Read.'; Valid = $false },
    @{ Name = 'wrong case'; Text = $prefix.Replace('example-skill', 'Example-Skill') + 'Read.'; Valid = $false },
    @{ Name = 'typed description'; Text = $prefix.Replace('Read the owning contract.', 'true') + 'Read.'; Valid = $false },
    @{ Name = 'empty description'; Text = $prefix.Replace('Read the owning contract.', "''") + 'Read.'; Valid = $false },
    @{ Name = 'mapping description'; Text = $prefix.Replace('description: Read the owning contract.', "description:`n  nested: value") + 'Read.'; Valid = $false },
    @{ Name = 'alias'; Text = $prefix.Replace('Read the owning contract.', '*borrowed') + 'Read.'; Valid = $false },
    @{ Name = 'unsupported structure'; Text = $prefix.Replace('Read the owning contract.', '[one, two]') + 'Read.'; Valid = $false }
)
$checks = 0
$ownedLinks = [Collections.Generic.List[string]]::new()
foreach ($case in $cases) {
    $problem = Get-SkillStructureProblem -FolderName 'example-skill' -Text $case.Text
    if ((-not $problem) -ne $case.Valid) { throw "Structure fixture '$($case.Name)' expected valid=$($case.Valid): $problem" }
    $checks++
}
$scratchParent = [IO.Path]::GetFullPath((Join-Path $repository '.build/skill-package-fixtures'))
$fixture = Join-Path $scratchParent ([guid]::NewGuid().ToString('N'))
$utf8 = [Text.UTF8Encoding]::new($false)
function Write-Fixture([string] $Relative, [string] $Text) {
    $path = Join-Path $fixture $Relative
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
    [IO.File]::WriteAllText($path, $Text, $utf8)
}
function Expect-Integrity([string] $Name, [string] $ExpectedProblem = '') {
    $result = Get-SkillPackageIntegrity $fixture
    if ($ExpectedProblem) {
        if (-not @($result.Problems | Where-Object { $_ -like "*$ExpectedProblem*" }).Count) { throw "$Name did not reject the intended defect: $($result.Problems -join '; ')" }
    } elseif ($result.Problems.Count) { throw "$Name unexpectedly failed: $($result.Problems -join '; ')" }
    $script:checks++
    return $result
}
try {
    Write-Fixture 'AGENTS.md' '# Canonical fixture guidance'
    Write-Fixture 'Tests/AGENTS.md' '# Scoped fixture guidance'
    $inventory = '{"schemaVersion":1,"files":["AGENTS.md","Tests/AGENTS.md"]}'
    Write-Fixture '.agents/guidance.json' $inventory
    Write-Fixture 'Specs/contract with spaces.md' '# Owning contract'
    $body = '[Contract](../../../Specs/contract%20with%20spaces.md#scope) [Web](https://example.invalid/) [Local](#local)'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + $body)
    $ui = 'interface:' + "`n  display_name: Example`n  short_description: Exercise packages`n  default_prompt: " + '"Use $example-skill for this task."'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' $ui
    $result = Expect-Integrity 'valid package and scope inventory'
    if ($result.Skills -ne 1 -or $result.Guidance -ne 2) { throw 'Package/guidance accounting was incorrect' }
    Write-Fixture 'Specs/contract(v1).md' '# Versioned contract'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Balanced](../../../Specs/contract(v1).md)')
    $null = Expect-Integrity 'balanced parentheses in a destination'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Escaped](../../../Specs/contract\(v1\).md)')
    $null = Expect-Integrity 'escaped parentheses in a destination'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Missing](../../../Specs/missing(v1).md)')
    $null = Expect-Integrity 'missing balanced destination is still checked' 'missing(v1).md'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '`[example](missing.md)` and ' + $body)
    $null = Expect-Integrity 'inline-code pseudo-link ignored'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '``inline ` [example](missing.md)' + "`n" + 'still code`` and ' + $body)
    $null = Expect-Integrity 'matching multiline code span ignored'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + 'first `' + "`n `t`n[Missing](missing.md)`n`n" + 'last `')
    $null = Expect-Integrity 'unmatched code delimiters cannot cross paragraphs' 'Missing local reference'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Read](missing.md (Owning contract))')
    $null = Expect-Integrity 'parenthesized title does not hide a missing target' 'Missing local reference'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Read](../../../Specs/contract(v1).md (Owning contract))')
    $null = Expect-Integrity 'valid parenthesized title'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + "[procedure]:`n  references/missing.md`n[Read][procedure]")
    $null = Expect-Integrity 'continued reference definition checks missing target' 'Missing local reference'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + "[procedure]:`n  <../../../Specs/contract with spaces.md>`n[Read][procedure]")
    $null = Expect-Integrity 'valid continued reference definition'
    foreach ($name in @('guide#v1.md', 'literal%20.md')) {
        Write-Fixture "Specs/$name" '# Literal filename'
        Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[File](' + ([Uri](Join-Path $fixture "Specs/$name")).AbsoluteUri + ')')
        $null = Expect-Integrity "file URI preserves literal filename $name"
    }
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Network](//example.invalid/reference)')
    $null = Expect-Integrity 'protocol-relative external URL is not a local target'
    Write-Fixture 'owned-skill.md' ($prefix + 'Read the owned fixture.')
    $skillPath = Join-Path $fixture '.agents/skills/example-skill/SKILL.md'
    [IO.File]::Delete($skillPath)
    $linkCreated = $false
    try {
        $null = New-Item -ItemType SymbolicLink -Path $skillPath -Target (Join-Path $fixture 'owned-skill.md') -ErrorAction Stop
        $ownedLinks.Add($skillPath); $linkCreated = $true
    } catch {
        if ($_.CategoryInfo.Category -ne 'PermissionDenied' -and $_.Exception -isnot [NotSupportedException]) { throw }
        Write-Host 'SKIP reparse-input fixture: this host cannot create a symbolic link.'
    }
    if ($linkCreated) {
        $null = Expect-Integrity 'reparse skill entrypoint is refused before reading' 'reparse'
        [IO.File]::Delete($skillPath)
    }
    $rootAlias = Join-Path $fixture 'root-alias'
    $rootLinkCreated = $false
    try {
        $null = New-Item -ItemType SymbolicLink -Path $rootAlias -Target $fixture -ErrorAction Stop
        $rootLinkCreated = $true
    } catch {
        if ($_.CategoryInfo.Category -ne 'PermissionDenied' -and $_.Exception -isnot [NotSupportedException]) { throw }
        Write-Host 'SKIP linked-root fixture: this host cannot create a symbolic link.'
    }
    if ($rootLinkCreated) {
        try {
            $linked = Get-SkillPackageIntegrity $rootAlias
            if ($linked.Skills -ne 0 -or $linked.Guidance -ne 0 -or -not @($linked.Problems | Where-Object { $_ -like '*reparse*' }).Count) { throw 'Linked root was traversed before package admission' }
            $checks++
        } finally { [IO.Directory]::Delete($rootAlias) }
    }
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + $body)
    $hidden = Join-Path $fixture '.agents/skills/hidden-skill'
    Write-Fixture '.agents/skills/hidden-skill/SKILL.md' 'Malformed hidden skill'
    [IO.File]::SetAttributes($hidden, [IO.FileAttributes]::Hidden -bor [IO.FileAttributes]::Directory)
    try { $null = Expect-Integrity 'hidden packages still receive structure validation' 'Missing YAML frontmatter fences' }
    finally { [IO.File]::SetAttributes($hidden, [IO.FileAttributes]::Directory); [IO.File]::Delete((Join-Path $hidden 'SKILL.md')); [IO.Directory]::Delete($hidden) }
    Write-Fixture '.agents/skills/example-skill/SKILL.md' $prefix
    $null = Expect-Integrity 'empty instructions through package API' 'Empty skill instructions'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Missing](references/missing.md)')
    $null = Expect-Integrity 'missing linked procedure' 'Missing local reference'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[contract]: references/missing.md')
    $null = Expect-Integrity 'missing reference-style destination' 'Missing local reference'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[contract]: <../../../Specs/contract with spaces.md>' + "`n" + '[Named contract][contract]')
    $null = Expect-Integrity 'valid reference-style destination with spaces'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[Absolute](' + (Join-Path $scratchParent 'foreign.md').Replace('\', '/') + ')')
    $null = Expect-Integrity 'absolute local path outside fixture root' 'Reference escapes repository'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '[File URI](' + ([Uri](Join-Path $scratchParent 'foreign.md')).AbsoluteUri + ')')
    $null = Expect-Integrity 'local file URI outside fixture root' 'Reference escapes repository'
    Write-Fixture '.agents/skills/example-skill/SKILL.md' ($prefix + '```text' + "`n[Example](missing.md)`n" + '```' + "`n" + $body)
    $null = Expect-Integrity 'fenced examples ignored'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui + "`n  display_name: Duplicate")
    $null = Expect-Integrity 'duplicate UI key' 'Duplicate YAML key'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui.Replace('$example-skill', '$other-skill'))
    $null = Expect-Integrity 'foreign default invocation' 'Default prompt does not invoke this skill'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui + "`npolicy:`n  allow_implicit_invocation: false")
    $null = Expect-Integrity 'optional boolean UI policy'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui + "`npolicy: invalid-scalar")
    $null = Expect-Integrity 'optional UI policy must be a mapping' 'non-mapping UI section'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui + "`npolicy:`n  allow_implicit_invocation: 'false'")
    $null = Expect-Integrity 'optional UI policy value must be boolean' 'boolean allow_implicit_invocation'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' ($ui + "`ndependencies: invalid-scalar")
    $null = Expect-Integrity 'optional UI dependencies must be a mapping' 'non-mapping UI section'
    [IO.File]::Delete((Join-Path $fixture '.agents/skills/example-skill/agents/openai.yaml'))
    $null = Expect-Integrity 'missing required UI package file' 'missing agents/openai.yaml'
    Write-Fixture '.agents/skills/example-skill/agents/openai.yaml' $ui
    [IO.File]::Delete((Join-Path $fixture 'Tests/AGENTS.md'))
    $null = Expect-Integrity 'missing scoped guidance' 'Scoped-guidance inventory mismatch'
    Write-Fixture 'Tests/AGENTS.md' ''
    $null = Expect-Integrity 'empty scoped guidance' 'Empty guidance'
    Write-Fixture 'Tests/AGENTS.md' '[Missing](missing.md)'
    $null = Expect-Integrity 'broken scoped-guidance route' 'Missing local reference'
    Write-Fixture 'Tests/AGENTS.md' '# Scoped fixture guidance'
    Write-Fixture 'Common/AGENTS.md' '# Uninventoried scope'
    $null = Expect-Integrity 'new scoped guidance must enter inventory' 'Scoped-guidance inventory mismatch'
    [IO.File]::Delete((Join-Path $fixture 'Common/AGENTS.md'))
    Write-Fixture 'Module Space/AGENTS.md' '# Valid scope path with spaces'
    Write-Fixture '.agents/guidance.json' '{"schemaVersion":1,"files":["AGENTS.md","Tests/AGENTS.md","Module Space/AGENTS.md"]}'
    $null = Expect-Integrity 'inventoried scope path may contain spaces'
    [IO.File]::Delete((Join-Path $fixture 'Module Space/AGENTS.md'))
    Write-Fixture '.agents/guidance.json' '{"schemaVersion":1,"files":["AGENTS.md","../AGENTS.md"]}'
    $null = Expect-Integrity 'scope inventory cannot escape' 'Guidance paths must be unique repository-relative'
    Write-Fixture '.agents/guidance.json' $inventory
    Write-Fixture '.agents/skills/example-skill/references/flow.md' '[Missing](missing-contract.md)'
    $null = Expect-Integrity 'supporting procedure closure' 'missing-contract.md'
    Write-Fixture '.agents/skills/example-skill/references/flow.md' '[Contract](../../../../Specs/contract%20with%20spaces.md)'
    $null = Expect-Integrity 'repaired package'
    # Excluded generated trees and nested checkouts must not manufacture extra scopes.
    Write-Fixture '.build/fixture/AGENTS.md' '# Generated, not owned guidance'
    Write-Fixture 'nested/.git' 'gitdir: synthetic-fixture'
    Write-Fixture 'nested/AGENTS.md' '# Another checkout'
    $null = Expect-Integrity 'ownership exclusions'
} finally {
    $resolved = [IO.Path]::GetFullPath($fixture)
    if (-not $resolved.StartsWith($scratchParent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Refused fixture cleanup outside its repository-local scratch parent' }
    foreach ($link in $ownedLinks) { if ([IO.File]::Exists($link) -and ([IO.File]::GetAttributes($link) -band [IO.FileAttributes]::ReparsePoint)) { [IO.File]::Delete($link) } }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
Write-Host "Skill structure/package/guidance: $checks behavioral fixtures passed; model decisions were not evaluated."
