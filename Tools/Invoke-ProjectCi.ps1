<#
.SYNOPSIS
Adapts the project commands to the pinned organization CI engine.
.PARAMETER Phase
Compiler-free PR checks, Release test build, regression execution, production ZIP, or explicit qualification.
.PARAMETER EngineRoot
Checkout of the exact shared engine commit in .ci/project.json.
.PARAMETER Platform
Matching native x64 or ARM64 hosted runner.
.PARAMETER Descriptor
Frozen main admission descriptor; required for native phases.
.PARAMETER Base
Immutable PR comparison base for changelog validation.
.PARAMETER Configuration
Debug, Release or ASan Debug for an explicit qualification only.
.PARAMETER QualificationScope
Normal regression or explicit Full qualification; never advances automatic Release state.
.PARAMETER DevelopmentEngine
Allows a local engine under development for compiler-free checks only.
.OUTPUTS
Canonical test evidence, source-bound receipts and qualified ZIPs; original portable-smoke.json production proof and GitHub job output paths.
.NOTES
Requires PowerShell 7.4 and Git. Native work is hosted-only and uses the matching architecture.
Static never invokes compiler fixtures. No publication or source/pin updates.
See Specs/Build/Build_UnifiedCI.md for roles, input rules, evidence and manual actions.
.EXAMPLE
./Tools/Invoke-ProjectCi.ps1 -Phase Static -EngineRoot ../RedSalamanders.CI -Base origin/main -DevelopmentEngine
#>
[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('Static','ReleaseTestBuild','ReleaseTest','Production','Qualification')][string]$Phase,
    [Parameter(Mandatory)][string]$EngineRoot, [ValidateSet('x64','ARM64')][string]$Platform = 'x64',
    [string]$Descriptor = '', [string]$Base = 'origin/main',
    [ValidateSet('Debug','Release','ASan Debug')][string]$Configuration = 'Release', [ValidateSet('Normal','Full')][string]$QualificationScope = 'Normal', [switch]$DevelopmentEngine)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
Import-Module (Join-Path $EngineRoot 'Tools/Modules/Ci.Policy.psm1') -ErrorAction Stop
Import-Module (Join-Path $EngineRoot 'Tools/Modules/Ci.Project.psm1') -ErrorAction Stop
$context = New-CiProjectContext -Root $root -EngineRoot $EngineRoot -Phase $Phase -Platform $Platform -Descriptor $Descriptor -DevelopmentEngine:$DevelopmentEngine
function Run([string]$Script, [string[]]$CommandArguments = @()) {
    Invoke-CiProjectScript -Context $context -Script $Script -Arguments $CommandArguments
}
if ($Phase -eq 'Static') {
    $env:CI_NATIVE_BUILDS_DISABLED = 'true'
    Test-CiProjectConfiguration $context
    & (Join-Path $EngineRoot 'Tools/Test-Workflows.ps1') -Root $root
    $baseCommit = Resolve-CiCommit $root $Base
    Run 'Tools/Test-Changelog.ps1' @('-Base',$baseCommit)
    # Source syntax, manifest, native-PR delegation and Changes are checked above.
    # validate.ps1 includes compiler fixtures and belongs to the native main lane.
    return
}
if ($Phase -ne 'Qualification') { $Configuration = 'Release' }
$number = [string]$context.batch.buildNumber
$profile = @('-Platform',$Platform,'-Configuration',$Configuration)
$started = [DateTime]::UtcNow
$receiptDirectory = Join-Path $root ".build/ci/regression/$Platform"
$evidenceDirectory = Join-Path $root '.build/evidence/test-runs'
if ($env:GITHUB_OUTPUT) { Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value ('evidence_directory=' + $evidenceDirectory) }
$env:RedTildeBuildNumber = $number
$env:RedTildeEnableTests = if ($Phase -eq 'Production') { 'false' } else { 'true' }
if ($Phase -in @('ReleaseTestBuild','Qualification','Production')) {
    $buildArguments = $profile + @('-BuildNumber',$number)
    if ($Phase -eq 'Production') { $buildArguments += '-ProductionOnly' }
    Run 'build.ps1' $buildArguments
    if ($Phase -eq 'ReleaseTestBuild') { return }
}
if ($Phase -eq 'Production') {
    Run 'package.ps1' @('-Platform',$Platform,'-SkipBuild')
    $archives = @(Get-ChildItem (Join-Path $root '.build/package') -Filter "*-$Platform-*.zip" -File | ForEach-Object FullName)
    if ($archives.Count -ne 1) { throw 'Expected one original production package and smoke receipt.' }
    $smokePath = $archives[0]+'.receipt.json'
    if (-not (Test-Path -LiteralPath $smokePath -PathType Leaf)) { throw 'Original production package smoke receipt is missing.' }
    $smoke = Get-Content -LiteralPath $smokePath -Raw | ConvertFrom-Json
    $expectedRefusals = @('self-test','test-scope','list-tests','interactive','crash','recovery-crash','recovery-restart',
        'preferences','asan','linked-save-crash','linked-save-restart','test-scope-missing-value','interactive-missing-value')
    if ($smoke.platform -cne $Platform -or $smoke.testsEnabled -isnot [bool] -or $smoke.testsEnabled -or
        $smoke.smokePassed -isnot [bool] -or -not $smoke.smokePassed -or
        $smoke.archiveSha256 -ine (Get-FileHash -LiteralPath $archives[0] -Algorithm SHA256).Hash -or
        @($smoke.testOnlyEntryPointRefusals).Count -ne $expectedRefusals.Count -or
        @(Compare-Object $expectedRefusals @($smoke.testOnlyEntryPointRefusals.name) -CaseSensitive).Count -or
        @($smoke.testOnlyEntryPointRefusals | Where-Object { $_.exitCode -ne 2 -or $_.dataDirectoryAbsent -isnot [bool] -or -not $_.dataDirectoryAbsent }).Count) {
        throw 'Original production package smoke receipt is incomplete or mismatched.'
    }
    Write-CiProductionReceipt -Context $context -Archives $archives -TestModeDisabled $true -SmokePassed $true
    # The workflow uploads this directory. A summary boolean cannot replace the
    # original clean-extraction capture and per-switch refusal results.
    Copy-Item -LiteralPath $smokePath -Destination (Join-Path $root ".build/ci/production/$Platform/portable-smoke.json")
    return
}
if ($Platform -eq 'x64') { Run 'validate.ps1' }
Assert-CiPeArchitecture -Path (Join-Path $root ".build/$Platform/$Configuration/RedTilde.exe") -Platform $Platform
Run 'test.ps1' ($profile + @('-Full','-SkipTooling','-SkipBuild'))
if ($Phase -eq 'Qualification') { Write-Host 'Explicit native qualification finished.'; return }
$receipts = @(Get-ChildItem $evidenceDirectory -Recurse -Filter 'receipt.json' -File | Where-Object LastWriteTimeUtc -ge $started)
if ($receipts.Count -ne 1) { throw 'Expected one fresh native test receipt.' }
$receipt = Get-Content $receipts[0].FullName -Raw | ConvertFrom-Json -AsHashtable
$cases = @($receipt.results | Where-Object { $_.Contains('cases') } | ForEach-Object cases)
$catalog = Get-Content (Join-Path $root 'Tests/native-case-catalog.json') -Raw | ConvertFrom-Json -AsHashtable
if (-not $receipt.passed -or @($cases).Count -ne @($catalog.cases).Count -or
    @($receipt.results | Where-Object { $_.Contains('failed') -and $_.failed -ne 0 }).Count -or
    (Compare-Object @($catalog.cases.name) @($cases))) { throw 'Native case catalog was not completely executed.' }
$probe = @($cases | Where-Object { $_ -ceq 'Utf8.CI_ASSERTION_PROBE' }).Count -eq 1
$evidence = @{ discovered = @($catalog.cases).Count; executed = @($cases).Count; evidence = (Get-CiEvidenceReference @($receipts[0].FullName)) }
$binaries = Get-CiProfileArtifacts -Context $context


if ($Phase -eq 'Qualification') { Write-Host 'Explicit qualification finished; it does not authorize production or publication.'; return }
$countUnit=if($evidence.Contains('countUnit')){$evidence.countUnit}else{'case'}
$registered=if($evidence.Contains('registered')){[int]$evidence.registered}else{[int]$evidence.discovered}
$expectedSkipped=if($evidence.Contains('expectedSkipped')){[int]$evidence.expectedSkipped}else{0}
Write-CiRegressionReceipt -Context $context -Discovered $evidence.discovered -Executed $evidence.executed -AssertionProbePassed $probe -Artifacts $binaries -EvidenceDirectory $receiptDirectory -CanonicalEvidence $evidence.evidence -CountUnit $countUnit -Registered $registered -ExpectedSkipped $expectedSkipped
if ($env:GITHUB_OUTPUT) { Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value ('evidence_directory=' + $evidenceDirectory) }
