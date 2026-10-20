[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',[switch] $SkipBuild)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration Release }
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$exe=Join-Path $PSScriptRoot ".build/$Platform/Release/RedTilde.exe"
$built=Get-Content (Join-Path $PSScriptRoot ".build/$Platform/Release/build-receipt.json") -Raw | ConvertFrom-Json
if (-not $built.passed -or $built.sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot) -or
    $built.binarySha256 -cne (Get-FileHash $exe).Hash) { throw 'Package source/binary identity is stale. Rebuild Release first.' }
$output=Join-Path $PSScriptRoot '.build/package'
$stage=Join-Path $output ('RedTilde-0.1.0-experimental-'+$Platform+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/Portable.md') -Destination (Join-Path $stage 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'capabilities.json') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/examples') -Destination (Join-Path $stage 'examples') -Recurse
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'res') | Out-Null
foreach ($asset in @('RedTilde.png','RedTilde.ico')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot "res/$asset") -Destination (Join-Path $stage 'res') }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE.txt') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Dependencies') -Destination $stage -Recurse
$pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
$source=Join-Path $PSScriptRoot ".build/dependencies/DxUi/source/$($pin.commit)"
Copy-Item -LiteralPath (Join-Path $source 'LICENSE.txt') -Destination (Join-Path $stage 'Dependencies/DxUi-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $source 'THIRD-PARTY-NOTICES.md') -Destination (Join-Path $stage 'Dependencies/DxUi-THIRD-PARTY-NOTICES.md')
[xml]$resolved=Get-Content (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.resolved.$Platform.props")
$triplet=if ($Platform -eq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
$wilCopyright=Join-Path ([string]$resolved.Project.PropertyGroup.DxUiConsumerOutputRoot) "vcpkg_installed/$Platform/$triplet/share/wil/copyright"
Copy-Item -LiteralPath $wilCopyright -Destination (Join-Path $stage 'Dependencies/WIL-LICENSE.txt')
$archive=$stage+'.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive
Write-Host "Experimental portable package: $archive"
[ordered]@{schemaVersion=1;sourceIdentity=$built.sourceIdentity;binarySha256=$built.binarySha256;platform=$Platform;
    archiveSha256=(Get-FileHash $archive).Hash;status='experimental';runtimeRedistributableBundled=$false} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($archive+'.receipt.json') -Encoding utf8
