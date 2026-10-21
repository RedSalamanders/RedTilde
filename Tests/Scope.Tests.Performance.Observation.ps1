[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../Build/Performance.psm1') -Force
Import-Module (Join-Path $PSScriptRoot '../Build/PerformanceObservation.psm1') -Force
$checks=0
function Require([bool]$Value,[string]$Message) { ++$script:checks; if(-not $Value) { throw $Message } }
function Refuse([scriptblock]$Body,[string]$Message) {
    $rejected=$false; try { & $Body | Out-Null } catch { $rejected=$true }
    Require $rejected $Message
}
function Native {
    $samples=@{}
    foreach($metric in @('commandUs','preparationSubmissionUs','gpuWaitUs','completedEditUs','ownerCpuCycles','commandOwnerCpuCycles')) {
        $samples[$metric]=[double[]]@(100,1000,100,1000,100,1000,100,1000)
    }
    $samples.cleanCompletedUs=[double[]]@(20,20,20,20)
    $samples.ownerProcessorStart=@(0,0,0,0,0,0,0,0); $samples.ownerProcessorEnd=@(0,0,0,0,0,0,0,1)
    @{completed=$true;schemaVersion=2;fixture='redtilde-completed-native-v2';view='Compose';renderer='Hardware';
        cycles=4;warmupCycles=2;widthPx=800;heightPx=600;dpi=96;recoverySettledAtMemory=$true;samples=$samples;
        initial=@{threads=$null;threadInventoryCollected=$false};final=@{threads=$null;threadInventoryCollected=$false;privateBytes=100;workingSetBytes=200}}
}
$native=Native
$noise=Get-RedTildeNativeNoise $native
Require ($noise.phases[0].allMedian -eq 550 -and $noise.phases[0].insertMedian -eq 100 -and $noise.phases[0].undoMedian -eq 1000) 'Alternating operations remain separate diagnostics.'
Require ($noise.phases[0].firstHalfMedian -eq 550 -and $noise.phases[0].lastHalfMedian -eq 550) 'Chronological diagnostics retain every sample.'
Require ($noise.processors.differentBoundarySamples -eq 1 -and $noise.processors.uniqueBoundaryProcessors.Count -eq 2) 'Boundary movement includes processor zero.'
$native.samples.commandUs[1]=200
$shifted=Get-RedTildeNativeNoise $native
Require ($shifted.phases[0].allMedian -eq 150 -and $shifted.phases[0].insertMedian -eq 100 -and $shifted.phases[0].undoMedian -eq 1000) 'A combined median can move while operation medians remain unchanged; do not silently replace the primary metric.'
$native=Native; $native.recoverySettledAtMemory=$false
Refuse { Get-RedTildePerformanceMetrics $native } 'Unsettled revised memory observations must be refused.'
$native=Native; $native.final.threadInventoryCollected=$true
Refuse { Get-RedTildePerformanceMetrics $native } 'Global thread inventory must not masquerade as a cheap latency snapshot.'
$native=Native; $native.samples.ownerProcessorStart[0]=0.5
Refuse { Get-RedTildePerformanceMetrics $native } 'Processor IDs must be integers.'
$native=Native; $native.samples.commandOwnerCpuCycles=@(1)
Refuse { Get-RedTildePerformanceMetrics $native } 'Missing command CPU samples must be refused.'
$native=Native; $native.schemaVersion=1; $native.fixture='redtilde-completed-native-v1'
$legacy=Get-RedTildePerformanceMetrics $native
Require ($legacy.commandUs -eq 550) 'Historical native-v1 receipts remain readable.'
function DiagnosticNative {
    $result=Native
    $result.samples.ownerCpuCycles=@($result.samples.commandOwnerCpuCycles | ForEach-Object { $_+60 })
    $phases=@(foreach($phase in 'enter','analysis','mirror','identity','caption','chrome','selection','recovery','exit') {
        @{name=$phase;wallUs=@(1,1,1,1,1,1,1,1);ownerCpuCycles=@(1,1,1,1,1,1,1,1)}
    })
    $post=@(foreach($index in 0..2) {
        @{privateBytes=100;workingSetBytes=200;threadInventoryCollected=($index -eq 2);threads=$(if($index -eq 2){10}else{$null})}
    })
    $result.controlDiagnostics=@{protocol='app-edit-phases-v2';primaryBeforeInventory=$true;phases=$phases;postMemory=$post;
        preparationOwnerCpuCycles=@(30,30,30,30,30,30,30,30);gpuWaitOwnerCpuCycles=@(30,30,30,30,30,30,30,30);
        renderCounts=@(1,1,1,1,1,1,1,1);preparationAdvances=@(2,2,2,2,2,2,2,2)}
    $result
}
$native=DiagnosticNative
Require ((Get-RedTildeControlDiagnostics $native).phases.Count -eq 9 -and (Get-RedTildePerformanceMetrics $native).commandUs -eq 550) 'Phase observations preserve the primary metric and all nine ordered phases.'
$native=DiagnosticNative; $native.controlDiagnostics.protocol='app-edit-phases-v1'
$native.controlDiagnostics.phases=@($native.controlDiagnostics.phases | Where-Object name -NotIn @('identity','caption'))
Require ((Get-RedTildeControlDiagnostics $native).phases.Count -eq 7) 'The initial seven-phase prototype remains readable without rewriting its raw observations.'
$native=DiagnosticNative; $native.controlDiagnostics.phases[0].name='chrome'
Refuse { Get-RedTildePerformanceMetrics $native } 'Reordered or duplicate phases are refused.'
$native=DiagnosticNative; $native.controlDiagnostics.phases[0].wallUs[0]=101
Refuse { Get-RedTildePerformanceMetrics $native } 'Subphase time cannot exceed its primary command.'
$native=DiagnosticNative; $native.controlDiagnostics.phases[0].ownerCpuCycles[0]=101
Refuse { Get-RedTildePerformanceMetrics $native } 'Subphase CPU cannot exceed its primary command.'
$native=DiagnosticNative; $native.controlDiagnostics.gpuWaitOwnerCpuCycles[0]=31
Refuse { Get-RedTildePerformanceMetrics $native } 'CPU phase accounting reconstructs the full primary sample.'
$native=DiagnosticNative; $native.controlDiagnostics.renderCounts[0]=0
Refuse { Get-RedTildePerformanceMetrics $native } 'A completed diagnostic sample requires an actual render.'
$native=DiagnosticNative; $native.controlDiagnostics.preparationAdvances[0]=0.5
Refuse { Get-RedTildePerformanceMetrics $native } 'Preparation/render counters are integer observations.'
$native=DiagnosticNative; $native.controlDiagnostics.postMemory[0].threadInventoryCollected=$true
Refuse { Get-RedTildePerformanceMetrics $native } 'Global inventory remains after primary observations.'
$native=DiagnosticNative; $native.controlDiagnostics.postMemory[2].threads=0
Refuse { Get-RedTildePerformanceMetrics $native } 'The final diagnostic inventory cannot report a missing count as zero.'
$native=DiagnosticNative; $native.controlDiagnostics.primaryBeforeInventory=$false
Refuse { Get-RedTildePerformanceMetrics $native } 'Diagnostic inventory must not contaminate primary memory.'
$native=DiagnosticNative; $native.controlDiagnostics.phases[1].wallUs=@(1)
Refuse { Get-RedTildePerformanceMetrics $native } 'Missing diagnostic phase samples are refused.'
$native=DiagnosticNative; $native.controlDiagnostics.phases[1].ownerCpuCycles[0]=-1
Refuse { Get-RedTildePerformanceMetrics $native } 'Negative diagnostic observations are refused.'
function Sample([double]$Busy=1.0,[object[]]$Processes=@(),[int]$Groups=1) {
    @{systemBusyPercent=$Busy;unavailableProcesses=0;processorGroups=$Groups;processes=$Processes}
}
$quiet=Get-RedTildeHostActivitySummary @((Sample),(Sample 2.0))
Require ($quiet.quietCpuObserved -and $quiet.maximumSystemBusyPercent -eq 2.0) 'Two low CPU intervals with no selected activity meet the declared CPU observation.'
$external=Get-RedTildeHostActivitySummary @((Sample),(Sample 1.0 @(@{name='cl';deltaCpuSeconds=0.5})))
Require (-not $external.quietCpuObserved -and $external.selectedExternalCpuSeconds -eq 0.5) 'A powerful lightly used host still records competing compiler activity.'
Require (-not (Get-RedTildeHostActivitySummary @((Sample),(Sample 5.01))).quietCpuObserved) 'Aggregate activity above the predeclared ceiling remains visible.'
Require (-not (Get-RedTildeHostActivitySummary @((Sample))).quietCpuObserved) 'One observation cannot establish a quiet interval.'
Require (-not (Get-RedTildeHostActivitySummary @((Sample),(Sample -Groups 2))).quietCpuObserved) 'Partial multi-group CPU observations cannot establish whole-host quiet.'
Refuse { Get-RedTildeHostActivitySummary @((Sample 101)) } 'Invalid CPU percentages are rejected.'
$previous=@{}
$process=[pscustomobject]@{Id=10;ProcessName='cl';StartTime=[DateTime]::UtcNow;CPU=1.0}
Require ((Get-RedTildeProcessObservation $process $previous).available) 'A newly observed process is represented.'
$process.CPU=2.5
Require ((Get-RedTildeProcessObservation $process $previous).observation.deltaCpuSeconds -eq 1.5) 'CPU deltas are bound to process creation identity.'
$process.StartTime=$null
Require (-not (Get-RedTildeProcessObservation $process $previous).available) 'Disappearing process properties are unavailable rather than fatal or zero.'
Require (-not (Get-RedTildeProcessObservation $null $previous).available) 'A null process race is tolerated explicitly.'
$process=[pscustomobject]@{Id=11;ProcessName='cl';CPU=1.0}
$reads=[Collections.Generic.List[int]]::new()
$getter={ $reads.Add(1); if($reads.Count -gt 1){ return $null }; return [DateTime]'2026-10-21T08:00:00Z' }.GetNewClosure()
$process | Add-Member -MemberType ScriptProperty -Name StartTime -Value $getter
Require ((Get-RedTildeProcessObservation $process $previous).available -and $reads.Count -eq 1) 'Volatile process properties are read once.'
$start=[DateTime]'2026-10-21T08:00:00Z'; $end=$start.AddSeconds(4)
$samples=@(@{utc=$start.AddSeconds(1).ToString('o')},@{utc=$start.AddSeconds(2).ToString('o')},@{utc=$start.AddSeconds(3).ToString('o')})
Require ((Get-RedTildeObserverCoverage $samples $start $end).complete) 'A sampled full window passes its declared coverage bound.'
$dateObjects=@($samples | ConvertTo-Json | ConvertFrom-Json)
Require ((Get-RedTildeObserverCoverage $dateObjects $start.ToUniversalTime() $end.ToUniversalTime()).complete) 'JSON date objects, UTC and local representations describe the same window.'
Require (-not (Get-RedTildeObserverCoverage $samples $start ($end.AddSeconds(10))).complete) 'An observer that dies cannot claim full coverage.'
Require (-not (Get-RedTildeObserverCoverage @() $start $end).complete) 'No observer data is not a quiet window.'
Write-Host "Performance observation: $checks behavioral checks passed."
