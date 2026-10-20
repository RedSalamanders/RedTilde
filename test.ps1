[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug',
      [ValidateSet('Core','Captures','Crash','All')][string] $Suite='Core',[switch] $SkipBuild)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration $Configuration }
$architecture=[Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
if ($Platform -eq 'ARM64' -and $architecture -ne 'Arm64') { throw 'ARM64 runtime tests require a native ARM64 host.' }
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$exe=Join-Path $PSScriptRoot ".build/$Platform/$Configuration/RedTilde.exe"
$buildReceipt=Join-Path $PSScriptRoot ".build/$Platform/$Configuration/build-receipt.json"
if (-not (Test-Path -LiteralPath $buildReceipt)) { throw 'No successful build identity exists. Run build.ps1 before testing this artifact.' }
$built=Get-Content $buildReceipt -Raw | ConvertFrom-Json
$sourceIdentity=Get-RedTildeSourceIdentity -Root $PSScriptRoot
if (-not $built.passed -or $built.sourceIdentity -cne $sourceIdentity -or $built.platform -cne $Platform -or
    $built.configuration -cne $Configuration -or $built.binarySha256 -cne (Get-FileHash $exe).Hash) {
    throw 'The existing executable does not match the current source/profile/build receipt. Rebuild it before using -SkipBuild.'
}
$run=Join-Path $PSScriptRoot ('.build/tests/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+$Platform+'-'+($Configuration -replace ' ','-'))
New-Item -ItemType Directory -Force -Path $run | Out-Null
$pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
$receipt=[ordered]@{schemaVersion=1; sourceIdentity=$sourceIdentity; dependency=$pin.commit;
    binarySha256=(Get-FileHash $exe).Hash; platform=$Platform; configuration=$Configuration; hostArchitecture=$architecture;
    suite=$Suite; interactive=$false; environment=@{osVersion=[Environment]::OSVersion.VersionString;processor=$env:PROCESSOR_IDENTIFIER;
    logicalProcessors=[Environment]::ProcessorCount;renderMode='hidden native ControlHost, default device'};
    startedUtc=[DateTime]::UtcNow.ToString('o'); passed=$false; results=@()}
$receiptPath=Join-Path $run 'receipt.json'
try {
    if ($Suite -in @('Core','All')) {
        $core=Join-Path $run 'core'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--self-test','--data-dir',$core) -LogPrefix (Join-Path $run 'core')
        if ($exit -ne 0) { throw "Behavioral tests failed ($exit). Read '$core/tests.json'." }
        $summary=Get-Content (Join-Path $core 'tests.json') -Raw | ConvertFrom-Json
        $receipt.results+=@{scope='Core';passed=$summary.passed;failed=$summary.failed}
        Write-Host "$($summary.passed) behavioral tests passed."
        $preferences=Join-Path $run 'preferences-exit'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--preferences-probe','--data-dir',$preferences) -LogPrefix (Join-Path $run 'preferences')
        if ($exit -ne 0) { throw 'Preference shutdown probe failed.' }
        $savedPreferences=Get-Content (Join-Path $preferences 'RedTilde.settings') -Raw
        if ($savedPreferences -notmatch '(?m)^theme=3\r?$') { throw 'Latest preferences were not durable after process exit.' }
        $receipt.results+=@{scope='PreferencesProcessExit';latestStateDurable=$true}
        $recovery=Join-Path $run 'recovery-restart'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--recovery-crash-probe','--data-dir',$recovery) -LogPrefix (Join-Path $run 'recovery-crash')
        if ($exit -eq 0 -or $exit -eq 6) { throw 'Recovery crash probe failed to durably checkpoint before its intentional crash.' }
        $snapshots=@(Get-ChildItem (Join-Path $recovery 'Recovery') -Filter '*.rtd' -File)
        if ($snapshots.Count -ne 1) { throw 'Expected one isolated recovery snapshot after crash.' }
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--recovery-restart-probe','--data-dir',$recovery) -LogPrefix (Join-Path $run 'recovery-restart')
        if ($exit -ne 0) { throw 'Restart did not restore the exact unsaved source as a dirty Save As copy.' }
        $receipt.results+=@{scope='RecoveryCrashRestart';exactSourceRestored=$true;originalProtectedBySaveAs=$true}
        $performance=Join-Path $run 'performance.json'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--benchmark',$performance,'--data-dir',(Join-Path $run 'benchmark-data')) -LogPrefix (Join-Path $run 'performance')
        if ($exit -ne 0) { throw 'Application performance fixture failed.' }
        $metrics=Get-Content $performance -Raw | ConvertFrom-Json
        $receipt.results+=@{scope='PerformanceBaseline';metrics=$metrics}
        Write-Host ("Native forced render: {0:N1} FPS; private memory {1:N1} MiB; dirty edit/render {2:N2} ms. Initial app baseline only." -f $metrics.cleanRenderFps,($metrics.privateBytes/1MB),$metrics.dirtyEditAndRenderMs)
    }
    if ($Suite -in @('Captures','All')) {
        $scenarios=@('compose-light','compose-dark','source','split','context-selection','local-source','settings','math','math-dark','mermaid','recovery','narrow','high-contrast')
        foreach ($scenario in $scenarios) {
            $output=Join-Path $run ($scenario+'.png')
            $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--capture',$output,'--scenario',$scenario,'--data-dir',(Join-Path $run 'capture-data')) -LogPrefix (Join-Path $run $scenario)
            if ($exit -ne 0 -or -not (Test-Path -LiteralPath $output)) { throw "Capture '$scenario' failed ($exit)." }
            $receipt.results+=@{scope='Capture';scenario=$scenario;sha256=(Get-FileHash $output).Hash}
        }
        Write-Host "$($scenarios.Count) actual hidden application captures generated."
    }
    if ($Suite -in @('Crash','All')) {
        $crashData=Join-Path $run 'crash-probe'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--crash-probe','--data-dir',$crashData) -LogPrefix (Join-Path $run 'crash')
        $dumps=@(Get-ChildItem (Join-Path $crashData 'Crashes') -Filter 'RedTilde-*.dmp' -File)
        if ($exit -eq 0 -or $dumps.Count -ne 1 -or -not (Test-Path (Join-Path $crashData 'Crashes/pending.txt'))) { throw 'Crash probe did not produce one local dump and marker.' }
        $signature=[IO.File]::ReadAllBytes($dumps[0].FullName)
        if ([Text.Encoding]::ASCII.GetString($signature,0,4) -ne 'MDMP') { throw 'Crash report has no minidump signature.' }
        $receipt.results+=@{scope='Crash';exitCode=$exit;dumpSha256=(Get-FileHash $dumps[0].FullName).Hash;pendingMarker=$true}
        Write-Host 'Isolated local crash dump and marker verified.'
    }
    if ($Configuration -eq 'ASan Debug' -and $Suite -in @('Core','All')) {
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--asan-probe','--data-dir',(Join-Path $run 'asan-probe')) -LogPrefix (Join-Path $run 'asan')
        $diagnostic=(Get-Content (Join-Path $run 'asan.stderr.log') -Raw)+(Get-Content (Join-Path $run 'asan.stdout.log') -Raw)
        if ($exit -eq 0 -or $diagnostic -notmatch 'AddressSanitizer: heap-use-after-free') { throw 'ASan detection gate failed; an ordinary crash or unsanitized build is insufficient.' }
        $receipt.results+=@{scope='AddressSanitizer';heapUseAfterFreeDetected=$true;exitCode=$exit}
        Write-Host 'Real AddressSanitizer heap-use-after-free detection verified.'
    }
    if ($sourceIdentity -cne (Get-RedTildeSourceIdentity -Root $PSScriptRoot)) { throw 'Source changed during testing; this receipt cannot qualify the new source.' }
    $receipt.passed=$true
} finally {
    $receipt.finishedUtc=[DateTime]::UtcNow.ToString('o')
    $receipt | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $receiptPath -Encoding utf8
    Write-Host "Receipt: $receiptPath"
}
