# Actual runtime routing and pre-discovery guards, without a compiler, native child or SDK verdict.
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$file=Join-Path $root 'test.ps1'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($file,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Runtime command syntax is invalid.'}
$open=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.CommandAst] -and $n.GetCommandName() -ceq 'Open-CiLocalSdkApplicationRuntimeInput'},$true))
if($open.Count -ne 1){throw 'Expected one shared runtime admission.'}
function Get-RedTildeMSBuild {'C:/synthetic-not-launched/MSBuild.exe'}
function Open-CiLocalSdkApplicationRuntimeInput {
    param($ProjectRoot,$BuildReceipt,$ExpectedBuildSha256,$MSBuildPath)
    $script:received=$PSBoundParameters
    [pscustomobject]@{sentinel='original admission'}
}
$LocalSdkBuildReceipt='C:/synthetic-not-read/build.json';$LocalSdkBuildSha256='a'*64
$fixture=Join-Path $root ('.build/local-sdk-runtime-routing/'+[guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$scriptFile=Join-Path $fixture 'actual-routing.ps1'
[IO.File]::WriteAllText($scriptFile,('$script:routingResult='+$open[0].Extent.Text))
. $scriptFile
if($received.ProjectRoot -cne $fixture -or $received.BuildReceipt -cne $LocalSdkBuildReceipt -or $received.ExpectedBuildSha256 -cne $LocalSdkBuildSha256 -or $routingResult.sentinel -cne 'original admission'){throw 'Actual runtime routing changed root/record/digest or result.'}
$saved=[Environment]::GetEnvironmentVariable('CI_NATIVE_BUILDS_DISABLED','Process')
try{
    $env:CI_NATIVE_BUILDS_DISABLED='true';$refused=$false
    try{& $file -Configuration Release -SkipBuild -Suites Storage -LocalSdkBuildReceipt $LocalSdkBuildReceipt -LocalSdkBuildSha256 $LocalSdkBuildSha256}
    catch{if($_.Exception.Message -notmatch 'requires local Windows native execution'){throw};$refused=$true}
    if(-not $refused){throw 'Hosted/compile-free runtime reached discovery.'}
}finally{[Environment]::SetEnvironmentVariable('CI_NATIVE_BUILDS_DISABLED',$saved,'Process')}
$allowed=$ast.Find({param($n)$n -is [Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -ceq '$localSdkSuites'},$true)
if($null -eq $allowed){throw 'Supported application scope declaration missing.'}
. ([scriptblock]::Create($allowed.Extent.Text))
$guards=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.IfStatementAst] -and $n.Extent.Text.Contains("throw 'Local SDK runtime requires the original build record/digest")},$true))
if($guards.Count -ne 1){throw 'Expected one runtime scope guard.'}
foreach($arguments in @(@{Suites=@('Tooling')},@{Suites=@('Sanitizer')},@{Full=$true},@{SkipBuild=$false},@{Configuration='Debug'})){
    $localSdkMode=$true;$Configuration='Release';$SkipBuild=$true;$Full=$false;$Suites=@('Storage')
    foreach($key in $arguments.Keys){Set-Variable -Name $key -Value $arguments[$key]}
    $refused=$false
    try{. ([scriptblock]::Create($guards[0].Extent.Text))}catch{if($_.Exception.Message -notmatch 'requires the original build record/digest'){throw};$refused=$true}
    if(-not $refused){throw 'Unsupported runtime scope was admitted.'}
}
$baselineGuards=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.IfStatementAst] -and $n.Extent.Text.StartsWith('if(-not $localSdkMode)') -and $n.Extent.Text.Contains('Enter-ScopedIterationLease')},$true))
if($baselineGuards.Count -ne 1){throw 'Ordinary iteration boundary is ambiguous.'}
function Enter-ScopedIterationLease {throw 'Local SDK touched ordinary iteration admission.'}
function Suspend-ScopedIteration {throw 'Local SDK invalidated ordinary iteration.'}
$localSdkMode=$true
. ([scriptblock]::Create($baselineGuards[0].Extent.Text))
Write-Host 'PASS actual runtime routing, hosted/scope refusal and untouched ordinary iteration; no native execution.'
$global:LASTEXITCODE=0
