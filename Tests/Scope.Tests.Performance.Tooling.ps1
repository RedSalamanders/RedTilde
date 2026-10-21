[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../Build/Performance.psm1') -Force
function Require([bool]$Value,[string]$Message) { if(-not $Value) { throw $Message } }
Require ((Get-RedTildeMedian @(1,4,2,3)) -eq 2.5) 'Even medians preserve both central values.'
$first=New-RedTildePerformanceSchedule -Blocks 12 -Seed 20261021
$second=New-RedTildePerformanceSchedule -Blocks 12 -Seed 20261021
Require (($first | ConvertTo-Json -Compress) -ceq ($second | ConvertTo-Json -Compress)) 'The retained seed replays the literal schedule.'
Require ($first.Count -eq 24 -and @($first | Where-Object { $_.order -cnotin @('ABBA','BAAB') }).Count -eq 0) 'Every independent block has two slots per side.'
Require ((Get-RedTildeSignFlipP @(1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0)) -eq (2.0/4096)) 'The exact two-sided block test has the specified minimum p.'
function Pair([double]$Ratio) {
    $pairs=foreach($scenario in @('Compose','Split')) { for($index=0;$index -lt 12;++$index) {
        $a=@{}; $b=@{}
        foreach($metric in @('commandUs','ownerCpuCycles','completedEditUs','cleanCompletedUs','privateBytes','workingSetBytes')) {
            $a[$metric]=100.0; $b[$metric]=100.0*$Ratio
        }
        @{scenario=$scenario;block=$index;A1=$a;A2=$a;B1=$b;B2=$b}
    } }; return @($pairs)
}
$same=Compare-RedTildePerformanceBlocks (Pair 1.0)
Require ($same.Count -eq 12 -and @($same | Where-Object finding -cne 'no-resolved-change').Count -eq 0) 'Identical native metrics show no resolved change.'
$slower=Compare-RedTildePerformanceBlocks (Pair 1.1)
Require (@($slower | Where-Object finding -ceq 'regressed').Count -eq 12) 'A stable 10 percent regression survives the complete Holm family.'
$faster=Compare-RedTildePerformanceBlocks (Pair 0.8)
Require (@($faster | Where-Object finding -ceq 'improved').Count -eq 12) 'Improvement keeps its direction.'
$invalid=Pair 1.0; $invalid[0].B1.commandUs=0.0
$rejected=$false; try { Compare-RedTildePerformanceBlocks $invalid | Out-Null } catch { $rejected=$true }
Require $rejected 'Missing/zero metrics cannot become an accepted comparison.'
Write-Host 'Performance tooling: median, schedule, exact inference, Holm family, direction and invalid-metric checks passed.'
