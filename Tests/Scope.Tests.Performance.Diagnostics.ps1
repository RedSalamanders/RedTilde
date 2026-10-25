[CmdletBinding()]
param([string]$StudyPath=(Join-Path $PSScriptRoot '../Measurements/S5_Controls_2026-10-21/Smoke'),[string]$OutputRoot='')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=[IO.Path]::GetFullPath($StudyPath)
if(-not $OutputRoot) { $OutputRoot=Join-Path $repo ('.build/diagnostic-evidence/'+[Guid]::NewGuid().ToString('N')) }
$OutputRoot=[IO.Path]::GetFullPath($OutputRoot)
$allowed=[IO.Path]::GetFullPath((Join-Path $repo '.build')).TrimEnd('\')+'\'
if(-not $OutputRoot.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $OutputRoot)) {
    throw 'Diagnostic regression output must be fresh and under this repository .build.'
}
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$auditor=Join-Path $PSScriptRoot 'Scope.Tests.Performance.Evidence.ps1'
& $auditor -StudyPath $source -Output (Join-Path $OutputRoot 'positive.json')
$positive=Get-Content (Join-Path $OutputRoot 'positive.json') -Raw | ConvertFrom-Json
$cases=@(
    @{name='phase-order';expected='Diagnostic phase names/order differ.';mutate={param($native) $native.controlDiagnostics.phases[0].name='caption'}},
    @{name='cpu-accounting';expected='Diagnostic CPU accounting does not reconstruct the primary sample.';mutate={param($native) ++$native.controlDiagnostics.gpuWaitOwnerCpuCycles[0]}},
    @{name='inventory-order';expected='Invalid command diagnostic protocol or inventory order.';mutate={param($native) $native.controlDiagnostics.primaryBeforeInventory=$false}},
    @{name='caption-write';expected='The alternating saved/dirty fixture must execute every synchronous native caption write.';mutate={param($native) --$native.activity.captionCalls}},
    @{name='declared-diagnostics';expected='Native diagnostics differ from the declared study mode.';mutate={param($native) $native.controlDiagnostics=$null}},
    @{name='protocol-downgrade';expected='Native diagnostics differ from the declared study mode.';mutate={param($native)
        $native.controlDiagnostics.protocol='app-edit-phases-v1'
        $native.controlDiagnostics.phases=@($native.controlDiagnostics.phases | Where-Object name -NotIn @('identity','caption'))
        $native.activity.captionCalls=0
    }}
)
$selectionFixture=Get-Content (Join-Path $source 'Compose-00-A1.native.json') -Raw|ConvertFrom-Json -AsHashtable
if($selectionFixture.controlDiagnostics.Contains('selectionComponents')) {
    $cases+=@(
        @{name='selection-declared-missing';expected='Declared nested selection diagnostics are missing.';mutate={param($native) $native.controlDiagnostics.selectionComponents=$null}},
        @{name='selection-protocol';expected='Invalid selection diagnostic protocol or transaction count.';mutate={param($native) $native.controlDiagnostics.selectionComponents.protocol='unknown'}},
        @{name='selection-incomplete';expected='Selection diagnostics must contain exactly one complete invocation.';mutate={param($native) $native.controlDiagnostics.selectionComponents.transactions[0].complete=$false}},
        @{name='selection-reentry';expected='Selection diagnostics must contain exactly one complete invocation.';mutate={param($native) $native.controlDiagnostics.selectionComponents.transactions[0].invocations=2}},
        @{name='selection-order';expected='Selection component names or ordering differ.';mutate={param($native) $native.controlDiagnostics.selectionComponents.transactions[0].components[0].name='find'}},
        @{name='selection-remainder';expected='Selection component reconciliation differs.';mutate={param($native) ++$native.controlDiagnostics.selectionComponents.transactions[0].parentReconciliation.ownerCpuCycles}},
        @{name='selection-null-work';expected='Unexecuted selection components must remain unavailable.';mutate={param($native)
            $native.controlDiagnostics.selectionComponents.transactions[0].components[0].calls=0
        }}
    )
}
$rows=@(foreach($case in $cases) {
    $target=Join-Path $OutputRoot $case.name
    New-Item -ItemType Directory -Path $target | Out-Null
    Get-ChildItem -LiteralPath $source -File -Filter '*.json' | Copy-Item -Destination $target
    $path=Join-Path $target 'Compose-00-A1.native.json'
    $native=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -AsHashtable
    & $case.mutate $native | Out-Null
    $native | ConvertTo-Json -Depth 40 | Set-Content -LiteralPath $path -Encoding utf8
    $pairPath=Join-Path $target 'Compose-00.pair.json'
    $pair=Get-Content -LiteralPath $pairPath -Raw | ConvertFrom-Json -AsHashtable
    $pair.A1Receipt.sha256=(Get-FileHash -LiteralPath $path).Hash
    $pair | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $pairPath -Encoding utf8
    $reason=$null
    try { & $auditor -StudyPath $target | Out-Null } catch { $reason=$_.Exception.Message }
    if($reason -cne $case.expected) { throw ('Expected semantic rejection, not hash damage: '+$case.name+'; actual='+$reason) }
    [pscustomobject]@{case=$case.name;rejected=$true;receiptHashUpdated=$true;reason=$reason}
})
[ordered]@{passed=$true;nativeExecutablesRun=$false;positiveStudyAuditChecks=$positive.checks;cases=$rows;
    scope='Regression of the independent evidence contract on retained fixtures; no new native or performance acceptance.'} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutputRoot 'receipt.json') -Encoding utf8
Write-Host ('Diagnostic evidence: '+$rows.Count+' integrity-consistent malformed receipts rejected.')
Write-Host ('Receipt: '+(Join-Path $OutputRoot 'receipt.json'))
