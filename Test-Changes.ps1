[CmdletBinding()]
param([ValidateSet('Affected','PrePush','Full')][string] $Mode='Affected',[switch] $Explain,
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $Explain -and $Mode -eq 'PrePush') { & (Join-Path $PSScriptRoot 'Tools/Fold-Changelog.ps1') -Check }
$changes=@(& git -C $PSScriptRoot ls-files --others --exclude-standard)
$changes+=@(& git -C $PSScriptRoot diff --name-only HEAD 2>$null)
if ($LASTEXITCODE -ne 0) { $changes+=@(& git -C $PSScriptRoot ls-files --cached) }
$changes=@($changes | Sort-Object -Unique)
$code=@($changes | Where-Object { $_ -match '^(Common|RedTilde|Tests|res|Dependencies|Build)/|\.vcxproj$|Directory\.Build' }).Count -gt 0
if ($Explain) {
    Write-Host ("Selected: validate + format"+$(if ($code) {' + Core native application tests'} else {'; documentation/guidance only'}))
    if ($Mode -ne 'Affected') { Write-Host 'PrePush/Full adds all six builds, three x64 test profiles, crash probes and captures. Native ARM64/physical input/performance remain separate gates. No enabled remote CI is credited.' }
    return
}
& (Join-Path $PSScriptRoot 'validate.ps1')
& (Join-Path $PSScriptRoot 'format.ps1') -Check
if ($Mode -eq 'Affected') {
    if ($code) { & (Join-Path $PSScriptRoot 'test.ps1') -Configuration $Configuration }
    return
}
foreach ($platform in @('x64','ARM64')) {
    & (Join-Path $PSScriptRoot 'restore-dxui.ps1') -Platform $platform
    foreach ($profile in @('Debug','Release','ASan Debug')) {
        & (Join-Path $PSScriptRoot 'build.ps1') -Platform $platform -Configuration $profile -SkipRestore
        if ($platform -eq 'x64') { & (Join-Path $PSScriptRoot 'test.ps1') -Platform x64 -Configuration $profile -Suite $(if ($profile -eq 'Release') {'All'} else {'Core'}) -SkipBuild }
    }
}
