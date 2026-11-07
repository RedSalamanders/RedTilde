[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$failures=[Collections.Generic.List[string]]::new()
function Check([string] $Name,[scriptblock] $Body) {
    try { & $Body; Write-Host "PASS $Name" }
    catch { $failures.Add($Name+': '+$_.Exception.Message); Write-Host ('FAIL '+$failures[-1]) -ForegroundColor Red }
}
Check 'Changes workflow' {
    & (Join-Path $PSScriptRoot 'Tools/Test-Changelog.ps1')
    $scratch = Join-Path $PSScriptRoot '.build/changelog-fixtures'
    [void][IO.Directory]::CreateDirectory($scratch)
    & (Join-Path $PSScriptRoot 'Tools/Test-ChangelogWorkflow.ps1') -ScratchRoot $scratch
}
Check 'Specifications and links' {
    $files=@(Get-ChildItem (Join-Path $PSScriptRoot 'Specs'),(Join-Path $PSScriptRoot '.agents') -Recurse -Filter '*.md')
    $files+=Get-Item (Join-Path $PSScriptRoot 'README.md'),(Join-Path $PSScriptRoot 'AGENTS.md')
    foreach ($file in $files) {
        foreach ($match in [regex]::Matches((Get-Content $file.FullName -Raw),'\]\(([^)]+)\)')) {
            $target=$match.Groups[1].Value.Split('#')[0]
            if ($target -match '^(https?:|mailto:)' -or -not $target) { continue }
            if (-not (Test-Path -LiteralPath (Join-Path $file.DirectoryName $target))) { throw "Broken link in $($file.Name): $target" }
        }
    }
    $plans=@(Get-ChildItem (Join-Path $PSScriptRoot 'Specs/Plans/WIP') -Filter '*.md' | Where-Object Name -ne 'README.md')
    $index=Get-Content (Join-Path $PSScriptRoot 'Specs/Plans/WIP/README.md') -Raw
    foreach ($plan in $plans) { if ([regex]::Matches($index,[regex]::Escape($plan.Name)).Count -ne 1) { throw "WIP index must name $($plan.Name) once." } }
}
Check 'Skill structure, package references and scoped guidance' {
    Import-Module (Join-Path $PSScriptRoot 'Build/SkillPackageIntegrity.psm1') -Force
    $result = Get-SkillPackageIntegrity -Root $PSScriptRoot
    if ($result.Problems.Count) { throw ($result.Problems -join "`n") }
    Write-Host "Validated structure/packages for $($result.Skills) skills and $($result.Guidance) guidance files; model decisions were not evaluated."
}
Check 'Skill package regression fixtures' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.SkillPackages.ps1') }
Check 'Exact dependency and capability status' {
    $pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
    if ($pin.commit -cnotmatch '^[0-9a-f]{40}$' -or $pin.apiRevision -ne 5 -or @($pin.targets).Count -ne 1 -or $pin.targets[0] -ne 'DxUi') { throw 'Invalid dependency pin.' }
    $capabilities=Get-Content (Join-Path $PSScriptRoot 'capabilities.json') -Raw | ConvertFrom-Json
    if ($capabilities.status -ne 'experimental' -or @($capabilities.pending).Count -eq 0) { throw 'Experimental dependency gates must stay explicit.' }
}
Check 'Evaluated Release compiler and linker policy' {
    & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.ReleaseBuildPolicy.ps1')
}
Check 'Build console and short dependency paths' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.BuildConsole.ps1') }
Check 'Hosted MSVC setup' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.MsvcSetup.ps1') }
Check 'Six build configurations and native inventory' {
    [xml]$project=Get-Content (Join-Path $PSScriptRoot 'RedTilde/RedTilde.vcxproj')
    foreach ($platform in @('x64','ARM64')) { foreach ($configuration in @('Debug','Release','ASan Debug')) {
        if (@($project.SelectNodes('//*[local-name()="ProjectConfiguration"]') | Where-Object Include -eq "$configuration|$platform").Count -ne 1) { throw "Missing $configuration|$platform" }
    } }
    # The shared checker covers both translation units and contextual support headers,
    # including added/untracked files; do not maintain a narrower cpp-only inventory.
    Import-Module (Join-Path $PSScriptRoot 'Build/ScopedTesting.psm1') -Force
    [void](Assert-ScopedTestNames $PSScriptRoot)
}
Check 'Run evidence retention' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.RunEvidence.ps1') }
Check 'RedMonitor SDK consumption' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.RedMonitor.ps1') }
Check 'Scope contracts and public runner' {
    Import-Module (Join-Path $PSScriptRoot 'Build/ScopedTesting.psm1') -Force
    [void](Assert-ScopedTestNames $PSScriptRoot)
    & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.ScopedTesting.ps1')
    if($LASTEXITCODE -ne 0){throw 'Scoped-testing regressions failed.'}
}
Check 'PowerShell syntax' {
    $files=@(Get-ChildItem $PSScriptRoot -File | Where-Object Extension -in @('.ps1','.psm1'))
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Build') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Tests') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Tools') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    foreach ($file in $files) {
        $tokens=$null; $errors=$null
        [void][Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors)
        if ($errors.Count) { throw "$($file.Name): $($errors[0].Message)" }
    }
}
Check 'Performance protocol' {
    & (Join-Path $PSScriptRoot 'Tests/Scope.Tests.Performance.Tooling.ps1')
    & (Join-Path $PSScriptRoot 'Tests/Scope.Tests.Performance.Observation.ps1')
    & (Join-Path $PSScriptRoot 'Tests/Scope.Tests.Performance.Evidence.ps1') -SelfTest
    & (Join-Path $PSScriptRoot 'Tests/Scope.Tests.Performance.Diagnostics.ps1')
}
if ($failures.Count) { throw ($failures -join "`n") }
Write-Host 'All RedTilde validators passed.'
