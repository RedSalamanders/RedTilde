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
}
Check 'Skill packages and root guidance' { & (Join-Path $PSScriptRoot 'validate-skills.ps1') }
Check 'Exact dependency and capability status' {
    $pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
    if ($pin.commit -cnotmatch '^[0-9a-f]{40}$' -or $pin.apiRevision -ne 5 -or @($pin.targets).Count -ne 1 -or $pin.targets[0] -ne 'DxUi') { throw 'Invalid dependency pin.' }
    $capabilities=Get-Content (Join-Path $PSScriptRoot 'capabilities.json') -Raw | ConvertFrom-Json
    if ($capabilities.status -cne 'production' -or $capabilities.version -cnotmatch '^\d+\.\d+\.\d+$' -or @($capabilities.pending).Count -ne 0 -or @($capabilities.deferred).Count -eq 0) {
        throw 'Production capability status must declare a semantic version, zero pending release gates, and explicit post-release deferrals.'
    }
    $versionHeader=Get-Content (Join-Path $PSScriptRoot 'Common/Version.h') -Raw
    $majorMatch=[regex]::Match($versionHeader,'(?m)^#define\s+REDTILDE_VERSION_MAJOR\s+(\d+)\b')
    $minorMatch=[regex]::Match($versionHeader,'(?m)^#define\s+REDTILDE_VERSION_MINOR\s+(\d+)\b')
    if (-not $majorMatch.Success -or -not $minorMatch.Success) { throw 'Common/Version.h must define REDTILDE_VERSION_MAJOR and REDTILDE_VERSION_MINOR.' }
    $headerBase="$($majorMatch.Groups[1].Value).$($minorMatch.Groups[1].Value)"
    $ciProject=Get-Content (Join-Path $PSScriptRoot '.ci/project.json') -Raw | ConvertFrom-Json
    if ($ciProject.versionBase -cne $headerBase) {
        throw ".ci/project.json versionBase '$($ciProject.versionBase)' does not match Common/Version.h '$headerBase'."
    }
    $capBase=($capabilities.version -split '\.',3)[0..1] -join '.'
    if ($capBase -cne $headerBase) {
        throw "capabilities.json version '$($capabilities.version)' does not match Common/Version.h '$headerBase'."
    }
    $vcpkg=Get-Content (Join-Path $PSScriptRoot 'vcpkg.json') -Raw | ConvertFrom-Json
    $vcpkgVer=[string]$vcpkg.'version-string'
    $vcpkgBase=($vcpkgVer -split '\.',3)[0..1] -join '.'
    if ($vcpkgVer -cnotmatch '^\d+\.\d+\.\d+$' -or $vcpkgBase -cne $headerBase) {
        throw "vcpkg.json version-string '$vcpkgVer' does not match Common/Version.h '$headerBase'."
    }
}
Check 'Evaluated Release compiler and linker policy' {
    & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.ReleaseBuildPolicy.ps1')
}
Check 'Build console and short dependency paths' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.BuildConsole.ps1') }
Check 'Complete vcpkg checkout for the pinned DxUi restore' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.VcpkgToolCheckout.ps1') }
Check 'Qualified SDK transaction snapshots' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.SdkInputs.ps1') }
Check 'Local SDK candidate command' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.LocalSdkCandidate.ps1') }
Check 'Local SDK runtime admission' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.LocalSdkRuntime.ps1') }
Check 'Hosted SDK role routing' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.HostedSdk.ps1') }
Check 'Hosted MSVC setup' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.MsvcSetup.ps1') }
Check 'Shared desktop admission' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.DesktopAdmission.ps1') }
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
Check 'Resource strings' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.ResourceStrings.ps1') }
Check 'Naming style' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.NamingStyle.ps1') }
Check 'Real-location fail-closed contract' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.RealLocations.ps1') }
Check 'PII hygiene' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.PiiHygiene.ps1') }
Check 'Per-user installer behavioral tests' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.Installer.ps1') }
Check 'RedMonitor SDK consumption' { & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.RedMonitor.ps1') }
Check 'Scope contracts and public runner' {
    & (Join-Path $PSScriptRoot 'Tests/Tooling.Tests.ScopedTesting.ps1')
    if($LASTEXITCODE -ne 0){throw 'Scoped-testing regressions failed.'}
}
Check 'PowerShell syntax' {
    $files=@(Get-ChildItem $PSScriptRoot -File | Where-Object Extension -in @('.ps1','.psm1'))
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Build') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Tests') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Tools') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Installer') -PathType Container) {
        $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Installer') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    }
    $setupScript = Join-Path $PSScriptRoot '.github/actions/setup-msvc/Setup.ps1'
    if (Test-Path -LiteralPath $setupScript -PathType Leaf) {
        $files += Get-Item -LiteralPath $setupScript
    }
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
