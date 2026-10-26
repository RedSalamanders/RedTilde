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
function AddEditorPreparationFixture([System.Collections.IDictionary]$native) {
    $sampleCount=2*(Get-Content (Join-Path $source 'study.json') -Raw|ConvertFrom-Json -AsHashtable).cycles
    $transactions=[Collections.Generic.List[object]]::new()
    $frameTransactions=[Collections.Generic.List[object]]::new()
    $attemptOffset=0; $hostRenderCount=500; $primaryCounter=100; $mirrorCounter=200
    for($transactionIndex=0;$transactionIndex -lt $sampleCount;++$transactionIndex) {
        $attemptCount=if($transactionIndex -eq 0) { 2 } else { 1 }
        $attempts=[Collections.Generic.List[object]]::new()
        $frameAttempts=[Collections.Generic.List[object]]::new()
        for($attemptIndex=0;$attemptIndex -lt $attemptCount;++$attemptIndex) {
            $finalAttempt=$attemptIndex -eq $attemptCount-1
            $primaryBefore=$primaryCounter; $mirrorBefore=$mirrorCounter
            ++$primaryCounter; ++$mirrorCounter
            $primaryStateBefore=[ordered]@{layoutReady=$false;layoutGeneration=$primaryBefore;retainedGeometryChunks=1;preparedGeometryChunks=0;
                reusedGeometryChunks=0;preparedSourceUnits=693;wholeGeometryFallback=$false;wholeRasterFallback=$false;wholeLayoutsPrepared=0;
                wholeLayoutSourceUnits=0;preparationPending=$true;preparationAdvances=$primaryBefore;cancelledPreparations=0;
                lastAdvanceLayouts=0;lastAdvanceBlockSteps=1;markdownPreparationPending=$true;markdownPreparationAdvances=$primaryBefore;
                lastAdvanceMarkdownWork=8;semanticPreparationPending=$false;semanticPreparationAdvances=0;lastAdvanceSemanticWork=0;
                sourceInkPreparationPending=$true;sourceInkRangeCount=0}
            $primaryStateAfter=[ordered]@{}
            foreach($key in $primaryStateBefore.Keys) { $primaryStateAfter[$key]=$primaryStateBefore[$key] }
            $primaryStateAfter.preparationAdvances=$primaryCounter
            $primaryStateAfter.markdownPreparationAdvances=$primaryCounter
            $primaryStateAfter.layoutGeneration=$primaryCounter
            $primaryStateAfter.lastAdvanceLayouts=1
            $primaryStateAfter.lastAdvanceMarkdownWork=9
            if($finalAttempt) { $primaryStateAfter.layoutReady=$true; $primaryStateAfter.preparationPending=$false; $primaryStateAfter.markdownPreparationPending=$false }
            $mirrorStateBefore=[ordered]@{layoutReady=$false;layoutGeneration=$mirrorBefore;retainedGeometryChunks=1;preparedGeometryChunks=0;
                reusedGeometryChunks=0;preparedSourceUnits=693;wholeGeometryFallback=$false;wholeRasterFallback=$false;wholeLayoutsPrepared=0;
                wholeLayoutSourceUnits=0;preparationPending=$true;preparationAdvances=$mirrorBefore;cancelledPreparations=0;
                lastAdvanceLayouts=1;lastAdvanceBlockSteps=0;markdownPreparationPending=$false;markdownPreparationAdvances=0;
                lastAdvanceMarkdownWork=0;semanticPreparationPending=$false;semanticPreparationAdvances=0;lastAdvanceSemanticWork=0;
                sourceInkPreparationPending=$true;sourceInkRangeCount=0}
            $mirrorStateAfter=[ordered]@{}
            foreach($key in $mirrorStateBefore.Keys) { $mirrorStateAfter[$key]=$mirrorStateBefore[$key] }
            $mirrorStateAfter.preparationAdvances=$mirrorCounter
            $mirrorStateAfter.layoutGeneration=$mirrorCounter
            $mirrorStateAfter.lastAdvanceLayouts=2
            if($finalAttempt) { $mirrorStateAfter.layoutReady=$true; $mirrorStateAfter.preparationPending=$false;
                $mirrorStateAfter.sourceInkPreparationPending=$false; $mirrorStateAfter.sourceInkRangeCount=3 }
            $hostBefore=$hostRenderCount
            $hostAfter=$hostBefore
            if($finalAttempt) { ++$hostAfter }
            $attempts.Add([ordered]@{frameMetricAttemptIndex=$attemptIndex;hostRenderCountBefore=$hostBefore;hostRenderCountAfter=$hostAfter;
                primary=[ordered]@{before=$primaryStateBefore;after=$primaryStateAfter};mirror=[ordered]@{before=$mirrorStateBefore;after=$mirrorStateAfter}})
            $frameAttempts.Add([ordered]@{totalUs=12;updateUs=3;renderUs=4;presentUs=4})
            $hostRenderCount=$hostAfter
        }
        $transactions.Add([ordered]@{complete=$true;firstAttemptIndex=$attemptOffset;attemptCount=$attemptCount;attempts=$attempts.ToArray()})
        $frameTransactions.Add([ordered]@{complete=$true;attemptCount=$attemptCount;requestedAttemptCount=$attemptCount;hostRenderCount=1;
            attemptCountMinusHostRenderCount=$attemptCount-1;attempts=$frameAttempts.ToArray()})
        $attemptOffset += $attemptCount
    }
    $attemptCapacity=$attemptOffset; $transactionCapacity=$sampleCount; $attemptRecordBytes=512; $transactionRecordBytes=16
    $native.frameDiagnostics=[ordered]@{protocol='dxui-frame-metrics-v1';editTransactions=$frameTransactions.ToArray();cleanTransactions=@()}
    $native.editorPreparationDiagnostics=[ordered]@{protocol='app-editor-preparation-v1';complete=$true;
        snapshotSource='DxUi.RichTextEditor.DebugGetState';attribution='state deltas only; no per-stage CPU attribution';
        storage=[ordered]@{reservedBytes=($attemptCapacity*$attemptRecordBytes+$transactionCapacity*$transactionRecordBytes);
            attemptCapacity=$attemptCapacity;transactionCapacity=$transactionCapacity;attemptRecordBytes=$attemptRecordBytes;
            transactionRecordBytes=$transactionRecordBytes;maxAttemptsPerTransaction=64;retainedThroughPostMeasurement=$true};
        transactions=$transactions.ToArray()}
}
$editorPrepCases=@(
    @{name='editor-preparation-valid';expected=$null;mutate={param($native) AddEditorPreparationFixture $native}},
    @{name='editor-preparation-index';expected='Invalid editor preparation attempt identity or render count.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attempts[1].frameMetricAttemptIndex=0}},
    @{name='editor-preparation-attempt-count';expected='Invalid editor preparation transaction or attempt count.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attemptCount=3}},
    @{name='editor-preparation-counter';expected='Editor preparation cumulative counter decreased during an attempt.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attempts[0].primary.after.preparationAdvances=0}},
    @{name='editor-preparation-render-continuity';expected='Editor preparation render counts are discontinuous within a transaction.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attempts[1].hostRenderCountBefore=501}},
    @{name='editor-preparation-render-reconciliation';expected='Editor preparation render delta differs from its frame metrics.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attempts[1].hostRenderCountAfter=500}},
    @{name='editor-preparation-work';expected='Invalid editor preparation work/counter field.';mutate={param($native)
        AddEditorPreparationFixture $native; $native.editorPreparationDiagnostics.transactions[0].attempts[0].primary.after.lastAdvanceMarkdownWork=-1}},
    @{name='editor-preparation-reserved-bytes';expected='Invalid editor preparation diagnostic protocol, storage or frame binding.';mutate={param($native)
        AddEditorPreparationFixture $native; ++$native.editorPreparationDiagnostics.storage.reservedBytes}}
)
$cases+=$editorPrepCases
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
    # Compiler command receipts are part of a current study's identity too.
    # Preserve all top-level evidence so each mutation reaches its semantic guard.
    Get-ChildItem -LiteralPath $source -File | Copy-Item -Destination $target
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
    if($null -eq $case.expected) {
        if($reason) { throw ('Expected valid optional editor preparation evidence: '+$case.name+'; actual='+$reason) }
        [pscustomobject]@{case=$case.name;accepted=$true;rejected=$false;receiptHashUpdated=$true;reason=$null}
    }
    else {
        if($reason -cne $case.expected) { throw ('Expected semantic rejection, not hash damage: '+$case.name+'; actual='+$reason) }
        [pscustomobject]@{case=$case.name;accepted=$false;rejected=$true;receiptHashUpdated=$true;reason=$reason}
    }
})
[ordered]@{passed=$true;nativeExecutablesRun=$false;positiveStudyAuditChecks=$positive.checks;optionalEditorPreparationFixtureAccepted=$true;cases=$rows;
    scope='Regression of the independent evidence contract on retained fixtures; no new native or performance acceptance.'} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutputRoot 'receipt.json') -Encoding utf8
Write-Host ('Diagnostic evidence: '+@($rows | Where-Object rejected).Count+' integrity-consistent malformed receipts rejected.')
$editorPreparationRows=@($rows | Where-Object case -Like 'editor-preparation-*')
Write-Host ('Editor preparation fixture: '+@($editorPreparationRows | Where-Object accepted).Count+' accepted schema and '+
    @($editorPreparationRows | Where-Object rejected).Count+' malformed variants checked.')
Write-Host ('Receipt: '+(Join-Path $OutputRoot 'receipt.json'))
