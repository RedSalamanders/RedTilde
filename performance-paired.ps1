[CmdletBinding()]
param([string]$Baseline='', [string]$OutputRoot='', [switch]$FreezeBaseline, [switch]$SameBinaryControl,
      [ValidateRange(2,16)][int]$Blocks=12,[ValidateRange(0,2147483647)][int]$Seed=20261021,
      [ValidateRange(4,64)][int]$Cycles=16,[switch]$Warp,[switch]$ControlDiagnostics,[switch]$DependencyAdoption)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'Build/Performance.psm1') -Force
$exe=Join-Path $PSScriptRoot '.build/x64/Release/RedTilde.exe'
$build=Get-Content (Join-Path $PSScriptRoot '.build/x64/Release/build-receipt.json') -Raw | ConvertFrom-Json
$source=Get-RedTildeSourceIdentity -Root $PSScriptRoot
if(-not $build.passed -or $build.sourceIdentity -cne $source -or $build.binarySha256 -cne (Get-FileHash $exe).Hash) { throw 'Rebuild the current Release source before measuring.' }
$fixture=Join-Path $PSScriptRoot 'docs/examples/s5-performance.md'
$inputs=[ordered]@{}
foreach($file in @('Tests/Application.Tests.Performance.cpp','Tests/Application.Tests.HeapDiagnostic.h','RedTilde/Main.cpp','RedTilde/EditDiagnostics.h','performance-paired.ps1','Build/Performance.psm1','Build/Harness.psm1','docs/examples/s5-performance.md')) {
    $inputs[$file]=(Get-FileHash (Join-Path $PSScriptRoot $file)).Hash
}
$compiler=Get-RedTildeCompilerProfile -Root $PSScriptRoot
$publicHeaders=Get-RedTildeDependencyPublicHeaders -Root $PSScriptRoot -Dependency $build.dependency
$rawCompiler=Join-Path $PSScriptRoot '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
$power=(& powercfg /getactivescheme | Out-String).Trim()
if($LASTEXITCODE -ne 0 -or -not $power) { throw 'Power policy identity is unavailable.' }
$environment=[ordered]@{machine=$env:COMPUTERNAME;cpu=$env:PROCESSOR_IDENTIFIER;os=[Environment]::OSVersion.VersionString;
    architecture=[Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString();powerPolicy=$power;
    d3d11=(Get-FileHash (Join-Path $env:SystemRoot 'System32/d3d11.dll')).Hash;
    warp=(Get-FileHash (Join-Path $env:SystemRoot 'System32/d3d10warp.dll')).Hash}
if(-not $OutputRoot) { $OutputRoot=Join-Path $PSScriptRoot ('.build/performance/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')) }
$OutputRoot=[IO.Path]::GetFullPath($OutputRoot)
$allowed=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '.build')).TrimEnd('\')+'\'
if(-not $OutputRoot.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $OutputRoot)) { throw 'OutputRoot must be a fresh directory under this repository .build.' }
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
if($FreezeBaseline) {
    if(@(& git -C $PSScriptRoot status --porcelain).Count) { throw 'Commit the instrumented baseline before freezing it.' }
    Copy-Item -LiteralPath $exe -Destination (Join-Path $OutputRoot 'RedTilde.exe')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '.build/x64/Release/build-receipt.json') -Destination (Join-Path $OutputRoot 'build.json')
    Copy-Item -LiteralPath $rawCompiler -Destination (Join-Path $OutputRoot 'compiler-command.tlog')
    $compiler['rawFile']='compiler-command.tlog'
    [ordered]@{schemaVersion=1;commit=(& git -C $PSScriptRoot rev-parse HEAD);build=$build;compiler=$compiler;inputs=$inputs;environment=$environment;publicHeaders=$publicHeaders} |
        ConvertTo-Json -Depth 14 | Set-Content (Join-Path $OutputRoot 'baseline.json') -Encoding utf8
    Write-Host "Immutable instrumented baseline: $OutputRoot"
    return
}
if(-not $Baseline) { throw 'Baseline must name a frozen instrumented baseline directory.' }
$baselinePath=Join-Path $Baseline 'RedTilde.exe'
$base=Get-Content (Join-Path $Baseline 'baseline.json') -Raw | ConvertFrom-Json -AsHashtable
if($base.build.binarySha256 -cne (Get-FileHash $baselinePath).Hash -or $base.compiler.sha256 -cne $compiler.sha256 -or
    ($base.inputs | ConvertTo-Json -Compress) -cne ($inputs | ConvertTo-Json -Compress) -or
    ($base.environment | ConvertTo-Json -Compress) -cne ($environment | ConvertTo-Json -Compress)) { throw 'Baseline/candidate harness, compiler, dependency, environment or binary identity differs.' }
$baseHeaderHash=if($base.Contains('publicHeaders')){[string]$base.publicHeaders.sha256}else{''}
Assert-RedTildeDependencyComparison -BaselineDependency $base.build.dependency -CandidateDependency $build.dependency -BaselinePublicHeadersSha256 $baseHeaderHash -CandidatePublicHeadersSha256 $publicHeaders.sha256 -DependencyAdoption:$DependencyAdoption
if(-not $SameBinaryControl -and $base.build.binarySha256 -ceq $build.binarySha256) { throw 'Use SameBinaryControl for the same executable.' }
$schedule=New-RedTildePerformanceSchedule -Blocks $Blocks -Seed $Seed
$candidateIdentity=if($SameBinaryControl) { $base } else { @{build=$build;compiler=$compiler;inputs=$inputs;environment=$environment;publicHeaders=$publicHeaders} }
$compilerReceipts=[ordered]@{}
foreach($side in @('baseline','candidate')) {
    $profile=if($side -ceq 'baseline'){$base.compiler}else{$candidateIdentity.compiler}
    if(-not $profile.Contains('schemaVersion') -or $profile.schemaVersion -ne 2) {throw 'A fresh compiler-profile-v2 baseline is required.'}
    $rawPath=if($side -ceq 'baseline' -or $SameBinaryControl) {
        if([IO.Path]::GetFileName([string]$profile.rawFile) -cne $profile.rawFile){throw 'Invalid baseline compiler receipt path.'}
        Join-Path $Baseline $profile.rawFile
    }else{$rawCompiler}
    if((Get-FileHash -LiteralPath $rawPath).Hash -cne $profile.rawSha256){throw 'Raw compiler command bytes changed.'}
    $leaf=$side+'-compiler-command.tlog'
    Copy-Item -LiteralPath $rawPath -Destination (Join-Path $OutputRoot $leaf)
    $compilerReceipts[$side]=[ordered]@{file=$leaf;sha256=$profile.rawSha256}
}
[ordered]@{protocol='independent-ABBA-BAAB-v1';blocks=$Blocks;seed=$Seed;cycles=$Cycles;sameBinaryControl=[bool]$SameBinaryControl;
    baseline=$base;candidate=$candidateIdentity;compilerReceipts=$compilerReceipts;schedule=$schedule;controlDiagnostics=[bool]$ControlDiagnostics;
    dependencyComparisonMode=$(if($DependencyAdoption){'sdk-adoption'}else{'same-pin'});
    selectionDiagnosticsProtocol=$(if($ControlDiagnostics){'app-selection-components-v1'}else{$null})} |
    ConvertTo-Json -Depth 18 | Set-Content (Join-Path $OutputRoot 'study.json') -Encoding utf8
$pairs=[Collections.Generic.List[object]]::new()
$nativeIdentities=@{}
foreach($block in $schedule) {
    $counts=@{A=0;B=0}; $pair=[ordered]@{scenario=$block.scenario;block=$block.block;order=$block.order}
    foreach($role in $block.order.ToCharArray()) {
        $label=[string]$role; ++$counts[$label]; $slot=$label+$counts[$label]
        $name=$block.scenario+'-'+$block.block.ToString('00')+'-'+$slot
        $selected=if($SameBinaryControl -or $label -ceq 'A') { $baselinePath } else { $exe }
        $expected=if($SameBinaryControl -or $label -ceq 'A') { $base.build.binarySha256 } else { $build.binarySha256 }
        if((Get-FileHash $selected).Hash -cne $expected -or (Get-RedTildeSourceIdentity -Root $PSScriptRoot) -cne $source) { throw 'Source or executable changed before a measured slot.' }
        $nativePath=Join-Path $OutputRoot ($name+'.native.json')
        $arguments=@('--performance-probe',$nativePath,'--document',$fixture,'--scenario',$block.scenario.ToLowerInvariant(),
            '--cycles',$Cycles.ToString(),'--data-dir',(Join-Path $OutputRoot ($name+'-data')))
        if($Warp) { $arguments+='--warp' }
        if($ControlDiagnostics) { $arguments+='--control-diagnostics' }
        $exit=Invoke-RedTildeHarness -Executable $selected -Arguments $arguments -LogPrefix (Join-Path $OutputRoot $name) -TimeoutSeconds 180
        if($exit -ne 0) { throw "Measured process $name failed ($exit); its original output is retained." }
        if((Get-FileHash $selected).Hash -cne $expected -or (Get-RedTildeSourceIdentity -Root $PSScriptRoot) -cne $source) { throw 'Source or executable changed during a measured slot.' }
        $native=Get-Content $nativePath -Raw | ConvertFrom-Json
        if($ControlDiagnostics -and ($null -eq $native.controlDiagnostics -or $native.controlDiagnostics.protocol -cne 'app-edit-phases-v2')) {
            throw 'Requested command-phase diagnostics are missing.'
        }
        if($ControlDiagnostics -and (-not $native.controlDiagnostics.PSObject.Properties['selectionComponents'] -or
            $native.controlDiagnostics.selectionComponents.protocol -cne 'app-selection-components-v1')) {
            throw 'Requested nested selection diagnostics are missing.'
        }
        $identity=[ordered]@{}
        foreach($key in @('fixture','view','renderer','adapter','vendorId','deviceId','driverVersion','dpi','widthPx','heightPx','sourceUnits','cycles')) { $identity[$key]=$native.$key }
        $identityJson=$identity | ConvertTo-Json -Compress
        if($nativeIdentities.ContainsKey($block.scenario) -and $nativeIdentities[$block.scenario] -cne $identityJson) { throw 'Native renderer/driver/DPI/fixture identity changed within the study.' }
        $nativeIdentities[$block.scenario]=$identityJson
        $pair[$slot]=Get-RedTildePerformanceMetrics $native
        $pair[$slot+'Receipt']=@{file=(Split-Path $nativePath -Leaf);sha256=(Get-FileHash $nativePath).Hash;executableSha256=$expected}
        if($native.view -cne $block.scenario -or ($Warp -and $native.renderer -cne 'WARP')) { throw 'Measured process used a different scenario/renderer.' }
    }
    $pairs.Add($pair)
    $pair | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $OutputRoot ($block.scenario+'-'+$block.block.ToString('00')+'.pair.json')) -Encoding utf8
    Write-Host ($block.scenario+' block '+($block.block+1)+'/'+$Blocks+' '+$block.order+' complete')
}
$comparison=Compare-RedTildePerformanceBlocks -Blocks $pairs.ToArray()
[ordered]@{schemaVersion=1;protocol='independent-ABBA-BAAB-v1';judge='redtilde-paired-log-sign-flip-holm-v2';
    independentBlocksPerScenario=$Blocks;familySize=$comparison.Count;sameBinaryControl=[bool]$SameBinaryControl;
    acceptance='review-required; new app judge does not qualify upstream D4c';metrics=$comparison;
    dependencyComparisonMode=$(if($DependencyAdoption){'sdk-adoption'}else{'same-pin'});
    sourceIdentity=$source;harnessInputs=$inputs;completedUtc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 14 | Set-Content (Join-Path $OutputRoot 'comparison.json') -Encoding utf8
$comparison | ForEach-Object { Write-Host ('{0} {1}: {2:P2}, p={3:N6}, {4}, controlsStable={5}' -f $_.scenario,$_.metric,$_.pairedChange,$_.adjustedP,$_.finding,$_.controlsStable) }
Write-Host "Raw paired study: $OutputRoot (acceptance remains review-required)"
