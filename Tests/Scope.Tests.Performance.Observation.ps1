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
Write-Host "Performance observation: $checks behavioral checks passed."
