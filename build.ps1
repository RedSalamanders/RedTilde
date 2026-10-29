[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug', [switch] $SkipRestore)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$nativeEntryElapsed = [Diagnostics.Stopwatch]::StartNew()
Import-Module (Join-Path $PSScriptRoot 'Build/BuildConsole.psm1') -ErrorAction Stop
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$canonicalDirectories=Assert-RedTildeCanonicalBuildDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$output=$canonicalDirectories.output
Assert-RedTildeOutputNotRunning -OutputDirectory $output
$canonicalDirectories=Assert-RedTildeCanonicalBuildDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$receiptPath=Join-Path $output 'build-receipt.json'
if((Test-Path -LiteralPath $receiptPath -PathType Leaf) -and ((Get-Item -LiteralPath $receiptPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){
    throw 'Build receipt cannot be a reparse point.'
}
New-Item -ItemType Directory -Force -Path $output | Out-Null
$canonicalDirectories=Assert-RedTildeCanonicalBuildDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
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
if (-not $SkipRestore) { & (Join-Path $PSScriptRoot 'restore-dxui.ps1') -Platform $Platform }
$canonicalDirectories=Assert-RedTildeCanonicalBuildDirectories -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$msbuild=Get-RedTildeMSBuild
$logDirectory=Join-Path $PSScriptRoot '.build/logs'
[void][IO.Directory]::CreateDirectory($logDirectory)
$log=Join-Path $logDirectory ("build-$Platform-$($Configuration -replace ' ','-')-$([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))-$([guid]::NewGuid().ToString('N')).log")
$started=[DateTime]::UtcNow
$sourceIdentity=Get-RedTildeSourceIdentity -Root $PSScriptRoot
$buildInputs=Get-RedTildeBuildInputSnapshot -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild
$allowIncremental=$false
if($priorReceipt -and $priorReceipt.schemaVersion -eq 2 -and $priorReceipt.passed -eq $true -and
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
$project=Join-Path $PSScriptRoot 'RedTilde/RedTilde.vcxproj'
if(-not $allowIncremental){
    & $msbuild $project /nologo /m /v:minimal "/p:Platform=$Platform" "/p:Configuration=$Configuration" /t:Clean /p:BuildProjectReferences=false "/flp:LogFile=$log;Verbosity=normal"
    if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration owned-profile clean failed. See '$log'." }
}
& $msbuild $project /nologo /m /v:minimal "/p:Platform=$Platform" "/p:Configuration=$Configuration" "/flp:LogFile=$log;Verbosity=normal;Append=true"
if ($LASTEXITCODE -ne 0) { throw "RedTilde $Platform $Configuration build failed. See '$log'." }
Assert-NativeBuildDiagnostics -LogPath $log
if ($sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot)) { throw 'Source changed during the build. Rebuild before accepting this artifact.' }
$postBuildInputs=Get-RedTildeBuildInputSnapshot -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild
if($buildInputs.identity -cne $postBuildInputs.identity){throw 'Compiled source, dependency or compiler context changed during the build. Rebuild before accepting this artifact.'}
$artifactIdentity=Get-RedTildeArtifactIdentity $PSScriptRoot $Platform $Configuration
$dependency=Get-Content (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.identity.$Platform.json") -Raw | ConvertFrom-Json
$success=[ordered]@{schemaVersion=2;passed=$true;sourceIdentity=$sourceIdentity;compiledSourceIdentity=$buildInputs.compiledSourceIdentity;
    buildInputs=[ordered]@{identity=$buildInputs.identity;dependencyIdentitySha256=$buildInputs.dependencyIdentitySha256;
        dependencyHeadersSha256=$buildInputs.dependencyHeadersSha256;dependencyResolvedPropsSha256=$buildInputs.dependencyResolvedPropsSha256;
        compilerContextSha256=$buildInputs.compilerContextSha256};
    dependencyHeadersSha256=$buildInputs.dependencyHeadersSha256;dependencyResolvedPropsSha256=$buildInputs.dependencyResolvedPropsSha256;
    compilerContextSha256=$buildInputs.compilerContextSha256;
    platform=$Platform;configuration=$Configuration;dependency=$dependency;artifactIdentity=$artifactIdentity;
    binarySha256=(Get-FileHash (Join-Path $output 'RedTilde.exe') -Algorithm SHA256).Hash;
    startedUtc=$started.ToString('o');finishedUtc=[DateTime]::UtcNow.ToString('o');runtimeExecuted=$false}
$receiptTemporary=$receiptPath+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
try{
    [IO.File]::WriteAllText($receiptTemporary,($success|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
    [IO.File]::Move($receiptTemporary,$receiptPath,$true)
}finally{if(Test-Path -LiteralPath $receiptTemporary){Remove-Item -LiteralPath $receiptTemporary -Force}}
Write-NativeBuildResult -Project RedTilde -Platform $Platform -Configuration $Configuration -LogPath $log -Artifacts @((Join-Path $output 'RedTilde.exe')) -Executable (Join-Path $output 'RedTilde.exe') -Duration $nativeEntryElapsed.Elapsed