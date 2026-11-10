<#
.SYNOPSIS
Builds and verifies an experimental production-only portable RedTilde ZIP.
.DESCRIPTION
Builds a test-disabled Release artifact, stages its runtime and licenses beneath .build/package, verifies
the clean extraction through hidden capture, and checks that every test-only entry point refuses without
creating its private data directory. Never activates a window or launches a diagnostic probe.
.PARAMETER Platform
Native package architecture, x64 or ARM64.
.PARAMETER SkipBuild
Uses the exact receipt-verified Release artifact already present. Test-enabled artifacts are refused.
.PARAMETER DxUiSdkInputDescriptor Independent descriptor for the original production SDK; omitted uses the accepted build binding.
.OUTPUTS
Portable ZIP and source/binary-bound JSON receipt beneath .build/package; retained staging and smoke evidence.
#>
[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',[switch] $SkipBuild,[string]$DxUiSdkInputDescriptor='')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration Release -ProductionOnly -DxUiSdkInputDescriptor $DxUiSdkInputDescriptor }
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$artifact=Assert-RedTildeTestArtifact -Root $PSScriptRoot -Platform $Platform -Configuration Release
$exe=$artifact.Executable
$built=$artifact.Build
if($built.testsEnabled -isnot [bool] -or $built.testsEnabled){throw 'Portable packaging requires a receipt-verified production-only Release build.'}
$sdk=$null;$packageFailure=$null
try{
if($built.PSObject.Properties['dependencyRecipe'] -and $built.dependencyRecipe.recipe -cne 'PinnedSource'){
    if($built.dependencyRecipe.recipe -cne 'PinnedQualifiedReleaseSdkV2'){throw 'Unsupported Tilde production SDK recipe.'}
    if(-not $DxUiSdkInputDescriptor){$DxUiSdkInputDescriptor=$built.sdkInputDescriptor}
    Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
    Import-Module (Join-Path $PSScriptRoot '.ci-engine/Tools/Modules/Ci.SdkConsumer.psm1')
    $sdk=Open-CiApplicationSdkInput -ProjectRoot $PSScriptRoot -Descriptor $DxUiSdkInputDescriptor -Platform $Platform -Configuration Release -MSBuildPath (Get-RedTildeMSBuild)
    if($null -eq $sdk -or $sdk.Descriptor.Hash -cne $built.sdkInputDescriptorSha256 -or $sdk.Evidence.inputKey -cne $built.dependencyRecipe.inputKey -or $sdk.Evidence.manifestSha256 -cne $built.dependencyRecipe.manifestSha256){throw 'Package SDK inputs differ from the original production build.'}
}
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
if($null -ne $sdk){
    Copy-Item -LiteralPath (Join-Path $sdk.SdkRoot 'licenses') -Destination (Join-Path $stage 'Dependencies/DxUiSdk') -Recurse
}else{
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
}
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
$refusalCases=@(
    @{name='self-test';arguments=@('--self-test')},
    @{name='test-scope';arguments=@('--test-scope','All')},
    @{name='list-tests';arguments=@('--list-tests')},
    @{name='interactive';arguments=@('--design-interactive-probe','keyboard')},
    @{name='crash';arguments=@('--crash-probe')},
    @{name='recovery-crash';arguments=@('--recovery-crash-probe')},
    @{name='recovery-restart';arguments=@('--recovery-restart-probe')},
    @{name='preferences';arguments=@('--preferences-probe')},
    @{name='asan';arguments=@('--asan-probe')},
    @{name='linked-save-crash';arguments=@('--linked-save-crash-probe')},
    @{name='linked-save-restart';arguments=@('--linked-save-restart-probe')},
    @{name='failure-host';arguments=@('--failure-host-probe','throw')},
    @{name='failure-host-missing-value';arguments=@('--failure-host-probe')},
    @{name='test-scope-missing-value';arguments=@('--test-scope')},
    @{name='interactive-missing-value';arguments=@('--design-interactive-probe')}
)
$refusals=@()
foreach($case in $refusalCases){
    $data=Join-Path $smoke ('refused-'+$case.name+'-data')
    if(Test-Path -LiteralPath $data){throw 'Production refusal fixture already exists.'}
    $arguments=@('--headless','--data-dir',$data)+$case.arguments
    $exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments $arguments -LogPrefix (Join-Path $smoke ('refused-'+$case.name)) -TimeoutSeconds 30
    if($exit -ne 2 -or (Test-Path -LiteralPath $data)){throw ('Production test entry point did not refuse before data access: '+$case.name)}
    $refusals+=[ordered]@{name=$case.name;arguments=$case.arguments;exitCode=$exit;dataDirectoryAbsent=$true}
}
if($null -ne $sdk){Close-CiPinnedSdkInput $sdk}
Write-Host "Experimental portable package: $archive"
[ordered]@{schemaVersion=1;sourceIdentity=$built.sourceIdentity;binarySha256=$built.binarySha256;platform=$Platform;
    archiveSha256=(Get-FileHash $archive).Hash;status='experimental';version=$version;smokePassed=$true;
    testsEnabled=$built.testsEnabled;runtimeRedistributableBundled=$true;testOnlyEntryPointRefusals=$refusals} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($archive+'.receipt.json') -Encoding utf8
}catch{$packageFailure=$_;throw}
finally{if($null -ne $sdk -and -not $sdk.Closed){try{Close-CiPinnedSdkInput $sdk}catch{if($null -eq $packageFailure){throw}else{Write-Warning ('Package SDK close also failed: '+$_.Exception.Message)}}}}
