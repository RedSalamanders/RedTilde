<#
.SYNOPSIS Builds one owned RedTilde profile and publishes its verified input/artifact receipt.
.PARAMETER DxUiSdkInputDescriptor Independent pinned Release SDK descriptor below .build; exact misses retain source.
.PARAMETER EnableTests Compile native test hooks. Debug and ASan Debug enable them by default.
.PARAMETER Analyze Run MSVC code analysis in an isolated source build under .build/analysis.
.PARAMETER Clean Clean the selected first-party profile without restoring or building dependencies.
.PARAMETER Rebuild Clean the selected profile before the verified native build.
.PARAMETER MaxCpuCount Bound the MSBuild and C++ compiler worker counts. Zero uses the tool defaults.
#>
[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug', [switch] $SkipRestore,
      [ValidateRange(0,65535)][int]$BuildNumber = $(if ($env:RedTildeBuildNumber) { [int]$env:RedTildeBuildNumber } else { 0 }),
      [switch]$ProductionOnly,[string]$DxUiSdkInputDescriptor='',[switch]$EnableTests,[switch]$Analyze,
      [switch]$Clean,[switch]$Rebuild,[ValidateRange(0,256)][int]$MaxCpuCount=0)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if ($env:CI_NATIVE_BUILDS_DISABLED -eq 'true') { throw 'Native builds are forbidden in compiler-free PR checks.' }
if ($ProductionOnly -and $Configuration -ne 'Release') { throw 'ProductionOnly requires Release.' }
if ($ProductionOnly -and $EnableTests) { throw 'ProductionOnly and EnableTests are mutually exclusive.' }
if ($Analyze -and ($ProductionOnly -or $DxUiSdkInputDescriptor)) { throw '-Analyze requires an ordinary source build.' }
if ($Clean -and ($Rebuild -or $Analyze -or $DxUiSdkInputDescriptor)) { throw '-Clean requires a standalone source profile.' }
if($DxUiSdkInputDescriptor -and $Configuration -cne 'Release'){throw 'Pinned SDK inputs require Release; Debug/ASan use source.'}
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$versionInfo = Get-RedTildeVersionBase -Root $PSScriptRoot
$versionMajor = $versionInfo.Major
$versionMinor = $versionInfo.Minor
$productVersion = "$($versionInfo.VersionBase).$BuildNumber"
$previousBuildNumber = [Environment]::GetEnvironmentVariable('RedTildeBuildNumber', 'Process')
$previousEnableTests = [Environment]::GetEnvironmentVariable('RedTildeEnableTests', 'Process')
Import-Module (Join-Path $PSScriptRoot 'Build/ScopedTesting.psm1') -Scope Local -ErrorAction Stop
$profileLease=Enter-ScopedProfileLease -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$sdkInput=$null;$sdkFailure=$null
try {
if (-not $Analyze) {
    $scopedReports=Get-ScopedReportRoot -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
    Remove-ScopedIteration -Path (Join-Path $scopedReports 'iteration.json') -Root $PSScriptRoot
}
$env:RedTildeBuildNumber = [string]$BuildNumber
$enableTestHooks = $EnableTests -or $Configuration -ne 'Release'
$env:RedTildeEnableTests = if ($enableTestHooks) { 'true' } else { 'false' }
$nativeEntryElapsed = [Diagnostics.Stopwatch]::StartNew()
Import-Module (Join-Path $PSScriptRoot 'Build/BuildConsole.psm1') -ErrorAction Stop
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
Import-Module (Join-Path $PSScriptRoot '.ci-engine/Tools/Modules/Ci.Policy.psm1') -ErrorAction Stop
$project=Join-Path $PSScriptRoot 'RedTilde/RedTilde.vcxproj'
$workers=if($MaxCpuCount){"/m:$MaxCpuCount"}else{'/m'}
$arguments=@($project,'/nologo',$workers,'/v:minimal',"/p:Platform=$Platform","/p:Configuration=$Configuration")
if($MaxCpuCount){$arguments+="/p:CL_MPCount=$MaxCpuCount"}
if ($Analyze) {
    $analysisRoot = Assert-CiContainedPath -Root $PSScriptRoot -Path (Join-Path $PSScriptRoot '.build/analysis')
    $analysisOutput = Assert-CiContainedPath -Root $analysisRoot -Path (Join-Path $analysisRoot "$Platform/$Configuration")
    Assert-RedTildeOutputNotRunning -OutputDirectory $analysisOutput
    $msbuild = Get-RedTildeMSBuild
    if (-not $SkipRestore) {
        & (Join-Path $PSScriptRoot 'restore-dxui.ps1') -Platform $Platform
        & (Join-Path $PSScriptRoot 'vcpkg-install.ps1') -Platform $Platform -Asan:($Configuration -eq 'ASan Debug') -MSBuildPath $msbuild
    }
    [void][IO.Directory]::CreateDirectory($analysisOutput)
    $log = Join-Path $analysisOutput "build-$([Guid]::NewGuid().ToString('N')).log"
    $arguments += '/p:RedTildeAnalyze=true'
    # Analyze every first-party source file while preserving the pinned DxUi archive.
    Invoke-CiNativeCommand -FilePath $msbuild -WorkingDirectory $PSScriptRoot -Arguments ($arguments + @(
        '/t:Clean', '/p:BuildProjectReferences=false', "/flp:LogFile=$log;Verbosity=normal"))
    if ($LASTEXITCODE -ne 0) { throw "RedTilde analysis clean failed. See '$log'." }
    Invoke-CiNativeCommand -FilePath $msbuild -WorkingDirectory $PSScriptRoot -Arguments ($arguments + @(
        "/flp:LogFile=$log;Verbosity=normal;Append=true"))
    if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration analysis build failed. See '$log'." }
    Assert-NativeBuildDiagnostics -LogPath $log
    $executable = Join-Path $analysisOutput 'RedTilde.exe'
    Write-NativeBuildResult -Project RedTilde -Platform $Platform -Configuration $Configuration -LogPath $log -Artifacts @($executable) -Executable $executable -Duration $nativeEntryElapsed.Elapsed
    return
}
$canonicalDirectories=Assert-RedTildeCanonicalBuildDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$output=$canonicalDirectories.output
Assert-RedTildeOutputNotRunning -OutputDirectory $output
$receiptPath=Join-Path $output 'build-receipt.json'
if((Test-Path -LiteralPath $receiptPath -PathType Leaf) -and ((Get-Item -LiteralPath $receiptPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){
    throw 'Build receipt cannot be a reparse point.'
}
New-Item -ItemType Directory -Force -Path $output | Out-Null
$priorReceipt=$null
if(Test-Path -LiteralPath $receiptPath -PathType Leaf){
    try{$priorReceipt=Get-Content -LiteralPath $receiptPath -Raw|ConvertFrom-Json -AsHashtable -ErrorAction Stop}
    catch [System.Management.Automation.RuntimeException]{$priorReceipt=$null}
}
$inProgress=[ordered]@{schemaVersion=2;passed=$false;platform=$Platform;configuration=$Configuration;
    invalidatedUtc=[DateTime]::UtcNow.ToString('o');reason='A new build attempt has started.'}
$receiptTemporary=$receiptPath+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
try{
    [IO.File]::WriteAllText($receiptTemporary,($inProgress|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
    if(Test-Path -LiteralPath $receiptPath -PathType Leaf){
        if((Get-Item -LiteralPath $receiptPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Build receipt cannot be a reparse point.'}
        [IO.File]::Move($receiptTemporary,$receiptPath,$true)
    }else{[IO.File]::Move($receiptTemporary,$receiptPath)}
}finally{if(Test-Path -LiteralPath $receiptTemporary){Remove-Item -LiteralPath $receiptTemporary -Force}}
$msbuild=Get-RedTildeMSBuild
$logDirectory=$canonicalDirectories.logs
[void][IO.Directory]::CreateDirectory($logDirectory)
$log=Join-Path $logDirectory ("build-$Platform-$($Configuration -replace ' ','-')-$([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))-$([guid]::NewGuid().ToString('N')).log")
$cleanArguments = $arguments + @('/t:Clean','/p:BuildProjectReferences=false',"/flp:LogFile=$log;Verbosity=normal")
if ($Clean) {
    $evaluated = @(& $msbuild @arguments '-getProperty:OutDir,IntDir')
    if ($LASTEXITCODE -ne 0) { throw 'MSBuild could not evaluate the clean paths.' }
    $paths = ($evaluated -join "`n") | ConvertFrom-Json
    Assert-RedTildeOwnedCleanDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration -Toolchain $paths
    Invoke-CiNativeCommand -FilePath $msbuild -WorkingDirectory $PSScriptRoot -Arguments $cleanArguments
    if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration owned-profile clean failed. See '$log'." }
    Assert-NativeBuildDiagnostics -LogPath $log
    Write-Host "Cleaned RedTilde $Platform $Configuration first-party outputs."
    return
}
$sdkProperties=@()
if($DxUiSdkInputDescriptor){
    Import-Module (Join-Path $PSScriptRoot '.ci-engine/Tools/Modules/Ci.SdkConsumer.psm1')
    $sdkInput=Open-CiApplicationSdkInput -ProjectRoot $PSScriptRoot -Descriptor $DxUiSdkInputDescriptor -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild
    if($null -ne $sdkInput -and $sdkInput.Evidence.recipe -ceq 'PinnedReleaseSdkV1'){
        # Tilde's evaluated before/after input snapshots require the portable v2
        # tool proof. Older dependency transport is retained as source fallback.
        Close-CiPinnedSdkInput $sdkInput
        $sdkInput=$null
        Write-Host 'Legacy SDK inputs do not supply the required Tilde v2 input snapshot; retaining source.'
    }
    if($null -ne $sdkInput){
        $sdkProperties=@(Get-RedTildeSdkBuildArguments -Root $PSScriptRoot -SdkInput $sdkInput)
        Write-Host "Using byte-verified pinned DxUi SDK input $($sdkInput.Evidence.inputKey)."
    }else{Write-Host 'Exact pinned SDK input unavailable; retaining the governed source recipe.'}
}
if (-not $SkipRestore -and $null -eq $sdkInput) { & (Join-Path $PSScriptRoot 'restore-dxui.ps1') -Platform $Platform }
if (-not $SkipRestore) {
    & (Join-Path $PSScriptRoot 'vcpkg-install.ps1') -Platform $Platform -Asan:($Configuration -eq 'ASan Debug') -MSBuildPath $msbuild
}
$started=[DateTime]::UtcNow
$sourceIdentity=Get-RedTildeSourceIdentity -Root $PSScriptRoot
$buildInputs=Get-RedTildeBuildInputSnapshot -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild -SdkInput $sdkInput
$allowIncremental=$false
if(-not $Rebuild -and $priorReceipt -and $priorReceipt.schemaVersion -eq 2 -and $priorReceipt.passed -eq $true -and
    $priorReceipt.platform -ceq $Platform -and $priorReceipt.configuration -ceq $Configuration -and
    $priorReceipt.buildInputs -and $priorReceipt.buildInputs.identity -ceq $buildInputs.identity -and
    $priorReceipt.compiledSourceIdentity -ceq $buildInputs.compiledSourceIdentity -and
    $priorReceipt.dependencyHeadersSha256 -ceq $buildInputs.dependencyHeadersSha256 -and
    $priorReceipt.dependencyResolvedPropsSha256 -ceq $buildInputs.dependencyResolvedPropsSha256 -and
    $priorReceipt.compilerContextSha256 -ceq $buildInputs.compilerContextSha256){
    try{
        $currentArtifact=Get-RedTildeArtifactIdentity $PSScriptRoot $Platform $Configuration
        $currentBinary=(Get-FileHash -LiteralPath (Join-Path $output 'RedTilde.exe') -Algorithm SHA256).Hash
        $allowIncremental=($priorReceipt.artifactIdentity -ceq $currentArtifact -and $priorReceipt.binarySha256 -ceq $currentBinary)
    }catch [System.Management.Automation.RuntimeException]{$allowIncremental=$false}
}
if(-not $allowIncremental){
    Invoke-CiNativeCommand -FilePath $msbuild -WorkingDirectory $PSScriptRoot -Arguments ($cleanArguments+$sdkProperties)
    if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration owned-profile clean failed. See '$log'." }
}
Invoke-CiNativeCommand -FilePath $msbuild -WorkingDirectory $PSScriptRoot -Arguments ($arguments+@("/flp:LogFile=$log;Verbosity=normal;Append=true")+$sdkProperties)
if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration build failed. See '$log'." }
Assert-NativeBuildDiagnostics -LogPath $log
if ($sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot)) { throw 'Source changed during the build. Rebuild before accepting this artifact.' }
$postBuildInputs=Get-RedTildeBuildInputSnapshot -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild -SdkInput $sdkInput
if($buildInputs.identity -cne $postBuildInputs.identity){throw 'Compiled source, dependency or compiler context changed during the build. Rebuild before accepting this artifact.'}
$artifactIdentity=Get-RedTildeArtifactIdentity $PSScriptRoot $Platform $Configuration
$version=[Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $output 'RedTilde.exe'))
if($version.FileMajorPart -ne $versionMajor -or $version.FileMinorPart -ne $versionMinor -or $version.FileBuildPart -ne $BuildNumber){
    throw "The compiled executable version ($($version.FileMajorPart).$($version.FileMinorPart).$($version.FileBuildPart)) does not match $productVersion."
}
if($null -ne $sdkInput){$dependency=@{Fingerprint=$sdkInput.Evidence.inputKey;Identity=$sdkInput.Evidence.identity};$dependencyRecipe=$sdkInput.Evidence}
else{$dependency=Get-Content (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.identity.$Platform.json") -Raw | ConvertFrom-Json;$dependencyRecipe=@{recipe='PinnedSource'}}
$success=[ordered]@{schemaVersion=2;passed=$true;sourceIdentity=$sourceIdentity;compiledSourceIdentity=$buildInputs.compiledSourceIdentity;
    buildInputs=[ordered]@{identity=$buildInputs.identity;dependencyIdentitySha256=$buildInputs.dependencyIdentitySha256;
        dependencyHeadersSha256=$buildInputs.dependencyHeadersSha256;dependencyResolvedPropsSha256=$buildInputs.dependencyResolvedPropsSha256;
        compilerContextSha256=$buildInputs.compilerContextSha256};
    dependencyHeadersSha256=$buildInputs.dependencyHeadersSha256;dependencyResolvedPropsSha256=$buildInputs.dependencyResolvedPropsSha256;
    compilerContextSha256=$buildInputs.compilerContextSha256;
    platform=$Platform;configuration=$Configuration;dependency=$dependency;dependencyRecipe=$dependencyRecipe;artifactIdentity=$artifactIdentity;
    version=$productVersion;buildNumber=$BuildNumber;testsEnabled=$enableTestHooks;
    binarySha256=(Get-FileHash (Join-Path $output 'RedTilde.exe') -Algorithm SHA256).Hash;
    startedUtc=$started.ToString('o');finishedUtc=[DateTime]::UtcNow.ToString('o');runtimeExecuted=$false}
if($null -ne $sdkInput){
    $success.sdkInputDescriptor=$sdkInput.Descriptor.Path
    $success.sdkInputDescriptorSha256=$sdkInput.Descriptor.Hash
    $linkLog=Join-Path $canonicalDirectories.intermediate 'RedTilde.tlog/link.command.1.tlog'
    $archive=Join-Path $sdkInput.SdkRoot "lib/$Platform/Release/DxUi.lib"
    if([IO.File]::ReadAllText($linkLog).IndexOf($archive.Replace('/','\'),[StringComparison]::OrdinalIgnoreCase) -lt 0){throw 'The actual linker input does not name the verified SDK archive.'}
    $success.sdkArchiveSha256=(Get-FileHash -LiteralPath $archive).Hash
    $success.sdkLinkCommandSha256=(Get-FileHash -LiteralPath $linkLog).Hash
    Close-CiPinnedSdkInput $sdkInput
    if($sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot) -or $artifactIdentity -cne (Get-RedTildeArtifactIdentity $PSScriptRoot $Platform $Configuration)){
        throw 'Source or output bytes changed during SDK finalization; no accepted build receipt was published.'
    }
}
$receiptTemporary=$receiptPath+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
try{
    [IO.File]::WriteAllText($receiptTemporary,($success|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
    [IO.File]::Move($receiptTemporary,$receiptPath,$true)
}finally{if(Test-Path -LiteralPath $receiptTemporary){Remove-Item -LiteralPath $receiptTemporary -Force}}
Write-NativeBuildResult -Project RedTilde -Platform $Platform -Configuration $Configuration -LogPath $log -Artifacts @((Join-Path $output 'RedTilde.exe')) -Executable (Join-Path $output 'RedTilde.exe') -Duration $nativeEntryElapsed.Elapsed
Write-Host "Version: $productVersion"
}catch{$sdkFailure=$_;throw}
finally{
    try{
        if($null -ne $sdkInput -and -not $sdkInput.Closed){try{Close-CiPinnedSdkInput $sdkInput}catch{if($null -eq $sdkFailure){throw}else{Write-Warning ('SDK close also failed: '+$_.Exception.Message)}}}
        [Environment]::SetEnvironmentVariable('RedTildeBuildNumber', $(if ($null -eq $previousBuildNumber) { [NullString]::Value } else { $previousBuildNumber }), 'Process')
        [Environment]::SetEnvironmentVariable('RedTildeEnableTests', $(if ($null -eq $previousEnableTests) { [NullString]::Value } else { $previousEnableTests }), 'Process')
    }finally{Exit-ScopedProfileLease -Lease $profileLease}
}
