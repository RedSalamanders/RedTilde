Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Performance.psm1')

function Get-RedTildeNativeNoise {
    param($Native)
    $null=Get-RedTildePerformanceMetrics $Native
    $names=@('commandUs','ownerCpuCycles')
    if($Native.schemaVersion -eq 2) { $names+='commandOwnerCpuCycles' }
    $phases=@(foreach($name in $names) {
        $samples=[double[]]$Native.samples.$name
        $insert=[Collections.Generic.List[double]]::new(); $undo=[Collections.Generic.List[double]]::new()
        for($i=0;$i -lt $samples.Count;$i+=2) { $insert.Add($samples[$i]); $undo.Add($samples[$i+1]) }
        $median=Get-RedTildeMedian $samples
        $deviations=[double[]]@($samples | ForEach-Object { [math]::Abs($_-$median) })
        $half=[int]($samples.Count/2)
        [ordered]@{metric=$name;allMedian=$median;insertMedian=(Get-RedTildeMedian $insert.ToArray());undoMedian=(Get-RedTildeMedian $undo.ToArray());
            relativeMedianAbsoluteDeviation=(Get-RedTildeMedian $deviations)/$median;
            firstHalfMedian=(Get-RedTildeMedian $samples[0..($half-1)]);lastHalfMedian=(Get-RedTildeMedian $samples[$half..($samples.Count-1)])}
    })
    $processors=$null
    $control=Get-RedTildeControlDiagnostics $Native
    if($Native.schemaVersion -eq 2) {
        $starts=@($Native.samples.ownerProcessorStart); $ends=@($Native.samples.ownerProcessorEnd)
        $changed=0; for($i=0;$i -lt $starts.Count;++$i) { if($starts[$i] -ne $ends[$i]) { ++$changed } }
        $processors=[ordered]@{uniqueBoundaryProcessors=@(@($starts)+@($ends) | Sort-Object -Unique);differentBoundarySamples=$changed;
            scope='boundary observations only; migrations returning to the same processor are not detected'}
    }
    [ordered]@{view=$Native.view;cycles=$Native.cycles;phases=$phases;processors=$processors;initial=$Native.initial;final=$Native.final;
        controlDiagnostics=$control;
        interpretation='diagnostics only; no sample exclusion, changed family or causal attribution'}
}

function Get-RedTildeHostActivitySummary {
    param([object[]]$Samples)
    if(@($Samples | Where-Object { $null -ne $_.systemBusyPercent -and
        (-not [double]::IsFinite($_.systemBusyPercent) -or $_.systemBusyPercent -lt 0 -or $_.systemBusyPercent -gt 100) }).Count) {
        throw 'Invalid total CPU observation.'
    }
    $busy=@($Samples | Where-Object { $null -ne $_.systemBusyPercent })
    $external=@($Samples | ForEach-Object { $_.processes | Where-Object { $_.name -cne 'RedTilde' -and $null -ne $_.deltaCpuSeconds -and $_.deltaCpuSeconds -gt 0 } })
    $unknown=@($Samples | Where-Object { $_.unavailableProcesses -gt 0 -or $_.processorGroups -ne 1 }).Count
    $maximum=if($busy.Count) { [double]($busy.systemBusyPercent | Measure-Object -Maximum).Maximum }else{ $null }
    [ordered]@{samples=$Samples.Count;validSystemIntervals=$busy.Count;maximumSystemBusyPercent=$maximum;
        meanSystemBusyPercent=$(if($busy.Count){[double]($busy.systemBusyPercent | Measure-Object -Average).Average}else{$null});
        selectedExternalCpuSeconds=$(if($external.Count){[double]($external.deltaCpuSeconds | Measure-Object -Sum).Sum}else{0.0});
        externalNames=@($external | ForEach-Object { $_.name } | Sort-Object -Unique);unavailableProcessObservations=$unknown;
        quietCpuObserved=($busy.Count -ge 2 -and $external.Count -eq 0 -and $unknown -eq 0 -and $maximum -le 5.0);
        definition='CPU observation only: at least two intervals, max total busy <=5%, no selected external CPU or inaccessible processes';
        limitations='selected names are not exhaustive; process start/exit work can escape deltas; GPU activity and physical occlusion are unmeasured'}
}

function Get-RedTildeProcessObservation {
    param($Process,[Collections.IDictionary]$Previous)
    $missing=[ordered]@{available=$false;observation=$null}
    try {
        # Enumerated processes can exit between property reads. Read each once;
        # an inaccessible/disappeared process is unavailable, never zero CPU.
        if($null -eq $Process) { return $missing }
        $identity=$Process.Id; $name=$Process.ProcessName; $started=$Process.StartTime; $cpu=$Process.CPU
        if($null -eq $identity -or $null -eq $started -or $null -eq $cpu -or -not $name) { return $missing }
        $key=$identity.ToString()+'-'+$started.ToUniversalTime().Ticks
        $delta=if($Previous.Contains($key)) { [double]$cpu-$Previous[$key] }else{ $null }
        if($null -ne $delta -and $delta -lt 0) { return $missing }
        $Previous[$key]=[double]$cpu
        [ordered]@{available=$true;observation=[ordered]@{id=$identity;name=$name;startUtc=$started.ToUniversalTime().ToString('o');cpuSeconds=[double]$cpu;deltaCpuSeconds=$delta}}
    } catch [ComponentModel.Win32Exception] { return $missing }
      catch [InvalidOperationException] { return $missing }
      catch [Management.Automation.RuntimeException] { return $missing } # PowerShell adapters can expose null after exit.
}

function Get-RedTildeObserverCoverage {
    param([object[]]$Samples,[DateTime]$StartUtc,[DateTime]$EndUtc)
    $StartUtc=$StartUtc.ToUniversalTime(); $EndUtc=$EndUtc.ToUniversalTime()
    if($EndUtc -le $StartUtc) { throw 'An ordered observation window is required.' }
    $points=@($Samples | ForEach-Object { ([DateTime]$_.utc).ToUniversalTime() } | Where-Object { $_ -ge $StartUtc -and $_ -le $EndUtc } | Sort-Object)
    $previous=$StartUtc; $maximum=0.0
    foreach($point in $points) { $maximum=[math]::Max($maximum,($point-$previous).TotalSeconds); $previous=$point }
    $maximum=[math]::Max($maximum,($EndUtc-$previous).TotalSeconds)
    [ordered]@{maximumGapSeconds=$maximum;samples=$points.Count;complete=($points.Count -ge 2 -and $maximum -le 2.5);maximumAllowedGapSeconds=2.5}
}

function Write-RedTildeHostActivity {
    param([string]$Path,[string]$ReadyPath)
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class RedTildeSystemTimes {
    [DllImport("kernel32.dll", SetLastError=true)]
    private static extern bool GetSystemTimes(out long idle, out long kernel, out long user);
    [DllImport("kernel32.dll")]
    public static extern ushort GetActiveProcessorGroupCount();
    public static long[] Read() {
        long idle, kernel, user;
        if(!GetSystemTimes(out idle, out kernel, out user)) throw new Win32Exception(Marshal.GetLastWin32Error());
        return new long[] { idle, kernel+user };
    }
}
'@
    $previous=@{}; $system=$null
    while($true) {
        $now=[RedTildeSystemTimes]::Read()
        $busy=$null
        if($null -ne $system) {
            $total=$now[1]-$system[1]; $idle=$now[0]-$system[0]
            if($total -le 0 -or $idle -lt 0 -or $idle -gt $total) { throw 'Invalid system CPU interval.' }
            $busy=100.0*($total-$idle)/$total
        }
        $system=$now; $unavailable=0
        $rows=[Collections.Generic.List[object]]::new()
        foreach($observed in @(Get-Process cl,link,MSBuild,csc,ninja,cmake,RedPrism,RedSalamander,RedXe,RedTilde,DxUi.Tests,DxUiTestHost -ErrorAction SilentlyContinue)) {
            $result=Get-RedTildeProcessObservation -Process $observed -Previous $previous
            if($result.available) { $rows.Add($result.observation) }else{ ++$unavailable }
        }
        [ordered]@{utc=[DateTime]::UtcNow.ToString('o');systemBusyPercent=$busy;unavailableProcesses=$unavailable;
            processorGroups=[RedTildeSystemTimes]::GetActiveProcessorGroupCount();processes=$rows.ToArray()} |
            ConvertTo-Json -Depth 6 -Compress | Add-Content -LiteralPath $Path -Encoding utf8
        if(-not (Test-Path -LiteralPath $ReadyPath)) { [IO.File]::WriteAllText($ReadyPath,'ready') }
        Start-Sleep -Seconds 1
    }
}
Export-ModuleMember -Function Get-RedTildeNativeNoise,Get-RedTildeHostActivitySummary,Get-RedTildeProcessObservation,Get-RedTildeObserverCoverage,Write-RedTildeHostActivity
