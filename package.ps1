[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',[switch] $SkipBuild)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration Release }
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$artifact=Assert-RedTildeTestArtifact -Root $PSScriptRoot -Platform $Platform -Configuration Release
$exe=$artifact.Executable
$built=$artifact.Build
$output=Join-Path $PSScriptRoot '.build/package'
$version='0.1.'+[string]$built.buildNumber
$stage=Join-Path $output ('RedTilde-'+$version+'-'+$Platform+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/Portable.md') -Destination (Join-Path $stage 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'capabilities.json') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/examples') -Destination (Join-Path $stage 'examples') -Recurse
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'res') | Out-Null
foreach ($asset in @('RedTilde.png','RedTilde.ico')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot "res/$asset") -Destination (Join-Path $stage 'res') }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE.txt') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Dependencies') -Destination $stage -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ThirdParty/RedMonitor/LICENSE.txt') -Destination (Join-Path $stage 'Dependencies/RedMonitor-LICENSE.txt')
$pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
$source=Join-Path $PSScriptRoot ".build/dependencies/DxUi/source/$($pin.commit.Substring(0,16))"
Copy-Item -LiteralPath (Join-Path $source 'LICENSE.txt') -Destination (Join-Path $stage 'Dependencies/DxUi-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $source 'THIRD-PARTY-NOTICES.md') -Destination (Join-Path $stage 'Dependencies/DxUi-THIRD-PARTY-NOTICES.md')
# The notice links its full component licenses relative to itself as docs/licenses/*.txt.
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Dependencies/docs') | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'docs/licenses') -Destination (Join-Path $stage 'Dependencies/docs/licenses') -Recurse
[xml]$resolved=Get-Content (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.resolved.$Platform.props")
$triplet=if ($Platform -eq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
$wilCopyright=Join-Path ([string]$resolved.Project.PropertyGroup.DxUiConsumerOutputRoot) "vcpkg_installed/$Platform/$triplet/share/wil/copyright"
Copy-Item -LiteralPath $wilCopyright -Destination (Join-Path $stage 'Dependencies/WIL-LICENSE.txt')
# This standalone product owns its runtime closure; it cannot import a sibling producer's packager.
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
$installation=(Get-RedTildeMSBuild) -replace '(?i)[\\/]MSBuild[\\/].*$',''
$redistRoot=Join-Path $installation 'VC/Redist/MSVC'
$runtimeArchitecture=if($Platform -eq 'ARM64'){'arm64'}else{'x64'}
$crt=@(Get-ChildItem -LiteralPath $redistRoot -Directory | Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName "$runtimeArchitecture/Microsoft.VC145.CRT" } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Container } | Select-Object -First 1)
if($crt.Count -ne 1){throw 'The selected Visual Studio installation has no VC145 redistributable for this architecture.'}
$runtime=@(Get-ChildItem -LiteralPath $crt[0] -Filter '*.dll' -File)
if(-not @($runtime | Where-Object Name -like 'vcruntime140*.dll').Count -or
    -not @($runtime | Where-Object Name -like 'msvcp140*.dll').Count){throw 'The selected VC runtime closure is incomplete.'}
$runtime | Copy-Item -Destination $stage
$archive=$stage+'.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive
$smoke=Join-Path $output ('extracted-'+[guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $archive -DestinationPath $smoke
$smokeExe=Join-Path $smoke 'RedTilde.exe'
$smokeVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($smokeExe)
if($smokeVersion.FileBuildPart -ne $built.buildNumber -or
    (Get-FileHash -LiteralPath $smokeExe).Hash -cne $built.binarySha256){throw 'Extracted package identity differs from the production build.'}
$exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--capture',(Join-Path $smoke 'smoke.png'),'--scenario','compose-light','--data-dir',(Join-Path $smoke 'smoke-data')) -LogPrefix (Join-Path $smoke 'smoke') -TimeoutSeconds 120
if($exit -ne 0 -or -not(Test-Path -LiteralPath (Join-Path $smoke 'smoke.png'))){throw 'Clean-extraction hidden application smoke failed.'}
if(-not $built.testsEnabled){
    $exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--self-test','--list-tests','--data-dir',(Join-Path $smoke 'test-disabled')) -LogPrefix (Join-Path $smoke 'test-disabled')
    if($exit -ne 2){throw 'A production ZIP unexpectedly exposes the native self-test catalog.'}
}
Write-Host "Experimental portable package: $archive"
[ordered]@{schemaVersion=1;sourceIdentity=$built.sourceIdentity;binarySha256=$built.binarySha256;platform=$Platform;
    archiveSha256=(Get-FileHash $archive).Hash;status='experimental';version=$version;smokePassed=$true;
    testsEnabled=$built.testsEnabled;runtimeRedistributableBundled=$true} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($archive+'.receipt.json') -Encoding utf8
