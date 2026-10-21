[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug', [switch] $SkipRestore)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
$output=Join-Path $PSScriptRoot ".build/$Platform/$Configuration"
Assert-RedTildeOutputNotRunning -OutputDirectory $output
if (-not $SkipRestore) { & (Join-Path $PSScriptRoot 'restore-dxui.ps1') -Platform $Platform }
$msbuild=Get-RedTildeMSBuild
New-Item -ItemType Directory -Force -Path $output | Out-Null
$log=Join-Path $output 'build.log'
$started=[DateTime]::UtcNow
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$sourceIdentity=Get-RedTildeSourceIdentity -Root $PSScriptRoot
& $msbuild (Join-Path $PSScriptRoot 'RedTilde/RedTilde.vcxproj') /nologo /m /v:minimal "/p:Platform=$Platform" "/p:Configuration=$Configuration" "/flp:LogFile=$log;Verbosity=normal"
if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration build failed. See '$log'." }
if ($sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot)) { throw 'Source changed during the build. Rebuild before accepting this artifact.' }
Import-Module (Join-Path $PSScriptRoot 'Build/ScopedTesting.psm1') -Force
$artifactIdentity=Get-ScopedArtifactIdentity $PSScriptRoot $Platform $Configuration
$dependency=Get-Content (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.identity.$Platform.json") -Raw | ConvertFrom-Json
[ordered]@{schemaVersion=1;passed=$true;sourceIdentity=$sourceIdentity;platform=$Platform;
    configuration=$Configuration;dependency=$dependency;artifactIdentity=$artifactIdentity;binarySha256=(Get-FileHash (Join-Path $output 'RedTilde.exe')).Hash;
    startedUtc=$started.ToString('o');finishedUtc=[DateTime]::UtcNow.ToString('o');runtimeExecuted=$false} |
    ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'build-receipt.json') -Encoding utf8
Write-Host "Built RedTilde: $output/RedTilde.exe"
