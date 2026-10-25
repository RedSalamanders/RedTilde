[CmdletBinding()]
param([Parameter(Mandatory)][string]$Baseline,[Parameter(Mandatory)][string]$OutputRoot,
    [ValidateRange(2,16)][int]$Blocks=12,[ValidateRange(4,64)][int]$Cycles=64,
    [ValidateRange(0,2147483647)][int]$Seed=20261021,[switch]$ControlDiagnostics,[switch]$DependencyAdoption)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/PerformanceObservation.psm1') -Force
$OutputRoot=[IO.Path]::GetFullPath($OutputRoot)
$allowed=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '.build')).TrimEnd('\')+'\'
if(-not $OutputRoot.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $OutputRoot)) {
    throw 'OutputRoot must be a fresh directory under this repository .build.'
}
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$activity=Join-Path $OutputRoot 'activity.jsonl'; $ready=Join-Path $OutputRoot 'observer.ready'
$module=Join-Path $PSScriptRoot 'Build/PerformanceObservation.psm1'
$job=Start-Job -ArgumentList $module,$activity,$ready -ScriptBlock {
    param($Module,$Activity,$Ready)
    $ErrorActionPreference='Stop'
    Import-Module $Module
    Write-RedTildeHostActivity -Path $Activity -ReadyPath $Ready
}
$windows=[Collections.Generic.List[object]]::new(); $passed=$false
try {
    $deadline=[DateTime]::UtcNow.AddSeconds(15)
    while(-not (Test-Path -LiteralPath $ready) -and [DateTime]::UtcNow -lt $deadline -and $job.State -ne 'Failed') { Start-Sleep -Milliseconds 100 }
    if(-not (Test-Path -LiteralPath $ready)) { throw 'Activity observer failed to initialize.' }
    Start-Sleep -Seconds 3
    foreach($kind in @('aa','ab')) {
        $window=[ordered]@{study=$kind;startUtc=[DateTime]::UtcNow.ToString('o');endUtc=$null}
        $windows.Add($window)
        $arguments=@{Baseline=$Baseline;OutputRoot=(Join-Path $OutputRoot $kind);Blocks=$Blocks;Cycles=$Cycles;Seed=$Seed}
        if($ControlDiagnostics) { $arguments.ControlDiagnostics=$true }
        if($DependencyAdoption) { $arguments.DependencyAdoption=$true }
        if($kind -ceq 'aa') { $arguments.SameBinaryControl=$true }
        & (Join-Path $PSScriptRoot 'performance-paired.ps1') @arguments *> (Join-Path $OutputRoot ($kind+'.log'))
        $window.endUtc=[DateTime]::UtcNow.ToString('o')
        Write-Host ($kind.ToUpperInvariant()+' study completed.')
        if($job.State -eq 'Failed') { throw 'Activity observer failed during the study.' }
    }
    $passed=$true
} finally {
    $observerFailed=$job.State -eq 'Failed'
    $observerErrors=@($job.ChildJobs[0].Error | ForEach-Object { [ordered]@{message=$_.ToString();stack=$_.ScriptStackTrace;
        position=$(if($_.InvocationInfo){$_.InvocationInfo.PositionMessage}else{$null})} })
    [ordered]@{state=$job.State;errors=$observerErrors} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutputRoot 'observer-status.json') -Encoding utf8
    Stop-Job -Job $job
    Receive-Job -Job $job -ErrorAction Continue | Out-Null
    Remove-Job -Job $job
    [ordered]@{blocks=$Blocks;cycles=$Cycles;seed=$Seed;passed=$passed;observerFailed=$observerFailed;windows=$windows.ToArray();
        direction='proceed directly; record actual host activity without coordinating or claiming an exclusive lease';
        primaryFamily='unchanged twelve slots, 5% timing/CPU and 2% memory investigation bands; judge review-required'} |
        ConvertTo-Json -Depth 8 | Set-Content (Join-Path $OutputRoot 'collection.json') -Encoding utf8
}
if($observerFailed) { throw 'Observer failure leaves host activity unavailable.' }
$observations=@(Get-Content $activity | ForEach-Object { $_ | ConvertFrom-Json -AsHashtable })
$summaries=@(foreach($window in $windows) {
    $start=([DateTime]$window.startUtc).ToUniversalTime(); $end=([DateTime]$window.endUtc).ToUniversalTime()
    $samples=@($observations | Where-Object { ([DateTime]$_.utc).ToUniversalTime() -ge $start -and ([DateTime]$_.utc).ToUniversalTime() -le $end })
    $hostActivity=Get-RedTildeHostActivitySummary $samples
    $coverage=Get-RedTildeObserverCoverage -Samples $samples -StartUtc ([DateTime]$window.startUtc) -EndUtc ([DateTime]$window.endUtc)
    [ordered]@{study=$window.study;activity=$hostActivity;coverage=$coverage;quietCpuWindowVerified=($hostActivity.quietCpuObserved -and $coverage.complete)}
})
$summaries | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $OutputRoot 'host-summary.json') -Encoding utf8
if(@($summaries | Where-Object { -not $_.coverage.complete }).Count) { throw 'Activity coverage is incomplete; retain this attempt without a quiet-host claim.' }
$noise=@(foreach($kind in @('aa','ab')) {
    foreach($file in Get-ChildItem (Join-Path $OutputRoot $kind) -Filter '*.native.json' -File) {
        $native=Get-Content $file.FullName -Raw | ConvertFrom-Json
        [ordered]@{study=$kind;file=$file.Name;sha256=(Get-FileHash $file.FullName).Hash;diagnostics=(Get-RedTildeNativeNoise $native)}
    }
})
$noise | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $OutputRoot 'noise.json') -Encoding utf8
Write-Host 'Observed studies retained. Quiet CPU observations, control stability and policy acceptance are separate results.'
