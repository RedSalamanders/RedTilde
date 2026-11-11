<#
.SYNOPSIS Builds RedTilde against one explicitly frozen local SDK into separate unpinned outputs.
.PARAMETER Descriptor Absolute local SDK snapshot descriptor from the producer capture command.
.PARAMETER DescriptorSha256 Independently frozen SHA-256 of that descriptor.
.PARAMETER Platform x64 or ARM64 Release target. Cross-builds establish no native runtime result.
.PARAMETER BuildNumber Resource build number for this local operation; defaults to zero.
.OUTPUTS Separate .build/local-unpinned operation, logs, actual linker-input proof and a local-only build record. Returns its paths and identities.
.NOTES Requires Windows, the exact pinned shared engine with local build support, selected Visual Studio, a matching API/ABI snapshot and existing SDK producer provenance. Never restores or edits the managed DxUi pin, replaces ordinary outputs, launches an application, runs tests, packages or grants qualification. Hosted/compiler-free execution refuses before discovery. Failed outputs remain diagnostic evidence.
.EXAMPLE ./Tools/Build-LocalSdkCandidate.ps1 -Descriptor Z:/src/DxUi/.build/local-sdk-capture/example/local-unpinned-snapshot.json -DescriptorSha256 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -Platform x64
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Descriptor,
    [Parameter(Mandatory)][ValidatePattern('^[a-f0-9]{64}$')][string]$DescriptorSha256,
    [ValidateSet('x64','ARM64')][string]$Platform='x64',[ValidateRange(0,65535)][int]$BuildNumber=0)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if(-not $IsWindows -or $env:GITHUB_ACTIONS -ceq 'true' -or $env:CI_NATIVE_BUILDS_DISABLED -ceq 'true'){throw 'Local SDK candidate builds require local Windows native execution.'}
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'Build/Toolchain.psm1')
Import-Module (Join-Path $root '.ci-engine/Tools/Modules/Ci.LocalSdkBuild.psm1') -ErrorAction Stop
$buildContract=[ordered]@{buildFile='RedTilde.sln';metadataProject='RedTilde/RedTilde.vcxproj';target='RedTilde';sdkProperty='RedTildeDxUiSdkRoot';engineProperty='RedTildeCiEngineRoot';outputProperty='RedTildeOutputRoot';testsProperty='RedTildeEnableTests';versionProperty='RedTildeBuildNumber';executable='RedTilde.exe'}
$result=Invoke-CiLocalSdkApplicationBuild -ProjectRoot $root -Descriptor $Descriptor -ExpectedDescriptorSha256 $DescriptorSha256 -Platform $Platform -MSBuildPath (Get-RedTildeMSBuild) -BuildContract $buildContract -BuildNumber $BuildNumber
Write-Host 'LOCAL_UNPINNED_BUILD: tests, repository and production qualification remain NOT_EVALUATED.'
$result
