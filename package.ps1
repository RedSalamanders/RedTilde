<#
.SYNOPSIS
Builds and verifies a production-only portable RedTilde ZIP.
.DESCRIPTION
Builds a test-disabled Release artifact, stages its runtime and licenses beneath .build/packages, verifies
the clean extraction through hidden capture, and checks that every test-only entry point refuses without
creating its private data directory. Never activates a window or launches a diagnostic probe.
.PARAMETER Platform
Native package architecture, x64 or ARM64.
.PARAMETER SkipBuild
Uses the exact receipt-verified Release artifact already present. Test-enabled artifacts are refused.
.PARAMETER DxUiSdkInputDescriptor Independent descriptor for the original production SDK; omitted uses the accepted build binding.
.OUTPUTS
Portable ZIP and source/binary-bound JSON receipt beneath .build/packages with retained staging and smoke evidence.
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
$output=Join-Path $PSScriptRoot '.build/packages'
Assert-RedTildeNoReparseAncestry -Base $PSScriptRoot -Target $output -AllowMissing -RequireDirectories `
    -ReparseErrorMessage 'The package output directory cannot be a reparse point.'
$exeVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
$version="$($exeVersion.FileMajorPart).$($exeVersion.FileMinorPart).$($exeVersion.FileBuildPart)"
if($exeVersion.FileBuildPart -ne $built.buildNumber){throw 'Production executable build number differs from its build receipt.'}
if($built.PSObject.Properties['version'] -and $built.version -cne $version){throw 'Production executable version differs from its build receipt.'}
$versionInfo = Get-RedTildeVersionBase -Root $PSScriptRoot
if($exeVersion.FileMajorPart -ne $versionInfo.Major -or $exeVersion.FileMinorPart -ne $versionInfo.Minor){
    throw "Production executable version $version differs from Common/Version.h."
}
$stage=Join-Path $output ('RedTilde-'+$version+'-'+$Platform+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SettingsStore.schema.json') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'capabilities.json') -Destination $stage
$utf8NoBom = [Text.UTF8Encoding]::new($false)
$diagnosticsDoc = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'docs/Diagnostics.md'), $utf8NoBom)
$diagnosticsDoc = $diagnosticsDoc.Replace('(../Specs/Core/Core_CrashHandling.md)', '(https://github.com/RedSalamanders/RedTilde/blob/main/Specs/Core/Core_CrashHandling.md)')
$diagnosticsDoc = $diagnosticsDoc.Replace('(README.md)', '(docs/README.md)')
[IO.File]::WriteAllText((Join-Path $stage 'Diagnostics.md'), $diagnosticsDoc, $utf8NoBom)
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'docs') | Out-Null
$userGuideDoc = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'docs/README.md'), $utf8NoBom)
$userGuideDoc = $userGuideDoc.Replace('(Diagnostics.md)', '(../Diagnostics.md)')
$userGuideDoc = $userGuideDoc.Replace('(Portable.md)', '(../README.md)')
$userGuideDoc = $userGuideDoc.Replace('](examples/', '](../examples/')
$userGuideDoc = $userGuideDoc.Replace('(../Specs/', '(https://github.com/RedSalamanders/RedTilde/blob/main/Specs/')
[IO.File]::WriteAllText((Join-Path $stage 'docs/README.md'), $userGuideDoc, $utf8NoBom)
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/screenshots') -Destination (Join-Path $stage 'docs/screenshots') -Recurse -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Installer') | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Installer/Install-RedTilde.ps1') -Destination (Join-Path $stage 'Installer/Install-RedTilde.ps1')
$portableReadme = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'docs/Portable.md'), $utf8NoBom)
$portableReadme = $portableReadme.Replace('(../Installer/Install-RedTilde.ps1)', '(Installer/Install-RedTilde.ps1)')
$portableReadme = $portableReadme.Replace('(../capabilities.json)', '(capabilities.json)')
$portableReadme = $portableReadme.Replace('[User Guide](README.md)', '[User Guide](docs/README.md)')
$stagedReadmePath = Join-Path $stage 'README.md'
[IO.File]::WriteAllText($stagedReadmePath, $portableReadme, $utf8NoBom)
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'docs/examples') -Destination (Join-Path $stage 'examples') -Recurse
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'res') | Out-Null
foreach ($asset in @('RedTilde.png','RedTilde.ico')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot "res/$asset") -Destination (Join-Path $stage 'res') }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE.txt') -Destination $stage
$noticesDoc = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'THIRD-PARTY-NOTICES.md'), $utf8NoBom)
$noticesDoc = $noticesDoc.Replace('(ThirdParty/RedMonitor/LICENSE.txt)', '(Dependencies/RedMonitor-LICENSE.txt)')
[IO.File]::WriteAllText((Join-Path $stage 'THIRD-PARTY-NOTICES.md'), $noticesDoc, $utf8NoBom)
$stageDependencies = Join-Path $stage 'Dependencies'
New-Item -ItemType Directory -Force -Path $stageDependencies | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'Dependencies') -Filter '*.lock.json' -File | Copy-Item -Destination $stageDependencies
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Dependencies/README.md') -Destination $stageDependencies
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ThirdParty/RedMonitor/LICENSE.txt') -Destination (Join-Path $stage 'Dependencies/RedMonitor-LICENSE.txt')
$yyjsonScope = $Platform.ToLowerInvariant()
$yyjsonTriplet = if ($Platform -eq 'ARM64') { 'arm64-windows-static-md' } else { 'x64-windows-static-md' }
$yyjsonCopyright = Join-Path $PSScriptRoot ".build/vcpkg_installed/$yyjsonScope/$yyjsonTriplet/share/yyjson/copyright"
if (-not (Test-Path -LiteralPath $yyjsonCopyright -PathType Leaf)) { throw "Missing yyjson copyright file: $yyjsonCopyright" }
Copy-Item -LiteralPath $yyjsonCopyright -Destination (Join-Path $stage 'Dependencies/yyjson-LICENSE.txt')
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
Copy-Item -LiteralPath (Join-Path $source 'ThirdParty/libfyaml/0.9.4/LICENSE') -Destination (Join-Path $stage 'Dependencies/libfyaml-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $source 'ThirdParty/libfyaml/0.9.4/LICENSE-xxHash.txt') -Destination (Join-Path $stage 'Dependencies/libfyaml-xxHash-LICENSE.txt')
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
$crt=@(Get-ChildItem -LiteralPath $redistRoot -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+' } |
    Sort-Object { [version]($_.Name -replace '[^0-9.].*$','') } -Descending |
    ForEach-Object { Join-Path $_.FullName "$runtimeArchitecture/Microsoft.VC145.CRT" } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Container } | Select-Object -First 1)
if($crt.Count -ne 1){throw 'The selected Visual Studio installation has no VC145 redistributable for this architecture.'}
$runtime=@(Get-ChildItem -LiteralPath $crt[0] -Filter '*.dll' -File)
if(-not @($runtime | Where-Object Name -like 'vcruntime140*.dll').Count -or
    -not @($runtime | Where-Object Name -like 'msvcp140*.dll').Count){throw 'The selected VC runtime closure is incomplete.'}
$runtime | Copy-Item -Destination $stage
$stageRootFull = [IO.Path]::GetFullPath($stage).TrimEnd('\') + '\'
$excludedNotice = [IO.Path]::GetFullPath((Join-Path $stage 'Dependencies/DxUi-THIRD-PARTY-NOTICES.md'))
foreach ($mdFile in Get-ChildItem -LiteralPath $stage -Filter '*.md' -Recurse -File) {
    if ([IO.Path]::GetFullPath($mdFile.FullName) -ceq $excludedNotice) { continue }
    $mdContent = [IO.File]::ReadAllText($mdFile.FullName, $utf8NoBom)
    foreach ($linkMatch in [regex]::Matches($mdContent, '\]\(([^)]+)\)')) {
        $rawTarget = $linkMatch.Groups[1].Value.Trim()
        if ($rawTarget -match '^([^\s"]+)\s+"[^"]*"$') { $rawTarget = $Matches[1] }
        $target = $rawTarget.Split('#')[0]
        if ($target -match '^(https?:|mailto:)' -or -not $target) { continue }
        $resolvedTarget = [IO.Path]::GetFullPath((Join-Path $mdFile.DirectoryName $target))
        if (-not $resolvedTarget.StartsWith($stageRootFull, [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-Path -LiteralPath $resolvedTarget)) {
            $relMd = [IO.Path]::GetRelativePath($stage, $mdFile.FullName)
            throw "Staged package file '$relMd' contains a broken or escaping relative link: '$target'."
        }
    }
}
$archive=$stage+'.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive
$smoke=Join-Path $output ('extracted-'+[guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $archive -DestinationPath $smoke
$smokeExe=Join-Path $smoke 'RedTilde.exe'
$smokeVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($smokeExe)
if($smokeVersion.FileMajorPart -ne $exeVersion.FileMajorPart -or
    $smokeVersion.FileMinorPart -ne $exeVersion.FileMinorPart -or
    $smokeVersion.FileBuildPart -ne $built.buildNumber -or
    (Get-FileHash -LiteralPath $smokeExe).Hash -cne $built.binarySha256){throw 'Extracted package identity differs from the production build.'}
$helpLog=Join-Path $smoke 'help'
$exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--help') -LogPrefix $helpLog -TimeoutSeconds 30 -SystemOnlyPath
$helpText=Get-Content -LiteralPath ($helpLog+'.stdout.log') -Raw
if($exit -ne 0 -or -not ($helpText -clike '*--document*' -and $helpText -clike '*--help*' -and $helpText -clike '*--version*')){throw 'Clean-extraction command help failed.'}
$versionLog=Join-Path $smoke 'version'
$exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--version') -LogPrefix $versionLog -TimeoutSeconds 30 -SystemOnlyPath
$versionText=Get-Content -LiteralPath ($versionLog+'.stdout.log') -Raw
if($exit -ne 0 -or $versionText -cnotlike "*RedTilde $version*"){throw 'Clean-extraction command version failed.'}
$exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--capture',(Join-Path $smoke 'smoke.png'),'--scenario','compose-light','--data-dir',(Join-Path $smoke 'smoke-data')) -LogPrefix (Join-Path $smoke 'smoke') -TimeoutSeconds 120 -SystemOnlyPath
if($exit -ne 0 -or -not(Test-Path -LiteralPath (Join-Path $smoke 'smoke.png'))){throw 'Clean-extraction hidden application smoke failed.'}
$relSmoke=Join-Path $smoke 'rel-smoke'
[void][IO.Directory]::CreateDirectory($relSmoke)
try{
    $exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @('--capture','rel-smoke.png','--scenario','compose-light','--data-dir','rel-smoke-data') -LogPrefix (Join-Path $smoke 'rel-smoke') -TimeoutSeconds 120 -SystemOnlyPath -WorkingDirectory $relSmoke
    if($exit -ne 0 -or -not(Test-Path -LiteralPath (Join-Path $relSmoke 'rel-smoke.png')) -or -not(Test-Path -LiteralPath (Join-Path $relSmoke 'rel-smoke.png.scenario.json'))){
        throw 'Clean-extraction relative-path hidden application smoke failed.'
    }
    foreach($emptyFlag in @('--capture','--benchmark','--performance-probe','--resource-probe')){
        $emptyData=Join-Path $relSmoke ('empty-'+$emptyFlag.TrimStart('-'))
        $exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments @($emptyFlag,'','--data-dir',$emptyData) -LogPrefix (Join-Path $smoke ('empty-'+$emptyFlag.TrimStart('-'))) -TimeoutSeconds 30 -SystemOnlyPath -WorkingDirectory $relSmoke
        if($exit -ne 1 -or (Test-Path -LiteralPath $emptyData)){throw "Empty path flag '$emptyFlag' did not cleanly refuse with exit 1."}
    }
}finally{
    if(Test-Path -LiteralPath $relSmoke){Remove-Item -LiteralPath $relSmoke -Recurse -Force -ErrorAction SilentlyContinue}
}
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
    @{name='interactive-missing-value';arguments=@('--design-interactive-probe')},
    @{name='failure-report';arguments=@('--failure-report-self-test')}
)
$refusals=@()
foreach($case in $refusalCases){
    $data=Join-Path $smoke ('refused-'+$case.name+'-data')
    if(Test-Path -LiteralPath $data){throw 'Production refusal fixture already exists.'}
    $arguments=@('--headless','--data-dir',$data)+$case.arguments
    $exit=Invoke-RedTildeHarness -Executable $smokeExe -Arguments $arguments -LogPrefix (Join-Path $smoke ('refused-'+$case.name)) -TimeoutSeconds 30 -SystemOnlyPath
    if($exit -ne 2 -or (Test-Path -LiteralPath $data)){throw ('Production test entry point did not refuse before data access: '+$case.name)}
    $refusals+=[ordered]@{name=$case.name;arguments=$case.arguments;exitCode=$exit;dataDirectoryAbsent=$true}
}
if($null -ne $sdk){Close-CiPinnedSdkInput $sdk}
$archiveHash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
$archiveFileName=[IO.Path]::GetFileName($archive)
[IO.File]::WriteAllText(($archive+'.sha256'),"$($archiveHash.ToLowerInvariant()) *$archiveFileName`n",[Text.UTF8Encoding]::new($false))
Write-Host "Production portable package: $archive"
[ordered]@{schemaVersion=1;sourceIdentity=$built.sourceIdentity;binarySha256=$built.binarySha256;platform=$Platform;
    archiveSha256=$archiveHash;status='production';version=$version;smokePassed=$true;
    testsEnabled=$built.testsEnabled;runtimeRedistributableBundled=$true;testOnlyEntryPointRefusals=$refusals} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($archive+'.receipt.json') -Encoding utf8
}catch{$packageFailure=$_;throw}
finally{if($null -ne $sdk -and -not $sdk.Closed){try{Close-CiPinnedSdkInput $sdk}catch{if($null -eq $packageFailure){throw}else{Write-Warning ('Package SDK close also failed: '+$_.Exception.Message)}}}}
