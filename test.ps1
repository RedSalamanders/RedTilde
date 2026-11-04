[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string] $Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string] $Configuration='Debug',
      [ValidateSet('Core','Captures','Crash','All')][string] $Suite='Core',[switch] $SkipBuild,[string[]]$Suites=@(),[switch]$Full,[switch]$SkipTooling)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$available=@('Storage','Settings','Recovery','Editor','Preferences','RecoveryRestart','Performance','Captures','Crash','Sanitizer','Tooling')
if($Full -or (-not $Suites.Count -and $Suite -eq 'All')){$Suites=$available}
elseif(-not $Suites.Count){$Suites=if($Suite -eq 'Core'){@('Storage','Settings','Recovery','Editor','Preferences','RecoveryRestart','Performance','Sanitizer')}else{@($Suite)}}
$Suites=@($Suites|ForEach-Object{$_ -split ','}|ForEach-Object{$_.Trim()}|Where-Object{$_}|Sort-Object -Unique)
foreach($name in $Suites){if($name -notin $available){throw "Unknown suite '$name'."}}
if($SkipTooling){$Suites=@($Suites|Where-Object{$_ -ne 'Tooling'})}
if($Configuration -ne 'ASan Debug'){$Suites=@($Suites|Where-Object{$_ -ne 'Sanitizer'})}
if('Tooling' -in $Suites){& (Join-Path $PSScriptRoot 'validate.ps1')}
if(-not @($Suites|Where-Object{$_ -ne 'Tooling'}).Count){Write-Host 'Independent tooling passed; native coverage NOT_EVALUATED';exit 0}
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration $Configuration }
$architecture=[Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
if ($Platform -eq 'ARM64' -and $architecture -ne 'Arm64') { throw 'ARM64 runtime tests require a native ARM64 host.' }
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$artifact=Assert-RedTildeTestArtifact -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$exe=$artifact.Executable
$built=$artifact.Build
$sourceIdentity=$artifact.SourceIdentity
Import-Module (Join-Path $PSScriptRoot 'Build/TestRunEvidence.psm1') -Force
$evidence=Start-TestRunEvidence -Repository $PSScriptRoot -Context @{project='RedTilde';platform=$Platform;configuration=$Configuration;suites=$Suites;sourceIdentity=$sourceIdentity;binarySha256=$built.binarySha256}
$run=$evidence.Path
$receipt=$null
try {
$pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
$receipt=[ordered]@{schemaVersion=1; sourceIdentity=$sourceIdentity; dependency=$pin.commit;
    binarySha256=(Get-FileHash $exe).Hash; buildReceiptSha256=$artifact.ReceiptSha256;toolchain=$built.dependency.Identity;
    platform=$Platform; configuration=$Configuration; hostArchitecture=$architecture;
    suite=($Suites -join ','); interactive=$false; environment=@{osVersion=[Environment]::OSVersion.VersionString;processor=$env:PROCESSOR_IDENTIFIER;
    logicalProcessors=[Environment]::ProcessorCount;renderMode='hidden native ControlHost, default device'};
    startedUtc=[DateTime]::UtcNow.ToString('o'); passed=$false; results=@()}
$receiptPath=Join-Path $run 'receipt.json'
try {
    $nativeGroups=@('Storage','Settings','Recovery','Editor'|Where-Object{$_ -in $Suites})
    if($nativeGroups.Count){
        $catalog=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Tests/native-case-catalog.json') -Raw|ConvertFrom-Json
        foreach($group in $nativeGroups){
            $listing=Join-Path $run ('catalog-'+$group)
            $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--self-test','--test-scope',$group,'--list-tests','--data-dir',$listing) -LogPrefix (Join-Path $run ('catalog-'+$group))
            if($exit -ne 0){throw "Native catalog failed: $group"}
            $listed=Get-Content (Join-Path $listing 'tests.json') -Raw|ConvertFrom-Json
            $expected=@($catalog.cases|Where-Object scope -ceq $group|ForEach-Object name)
            if(-not $listed.catalogOnly -or $listed.passed -ne 0 -or $listed.failed -ne 0 -or
                $listed.registered -ne $catalog.cases.Count -or $listed.selected -ne $expected.Count -or
                (Compare-Object $expected @($listed.tests.name))){throw "Native catalog/partition differs from its reviewed contract: $group"}
        }
        $execute=$nativeGroups
        foreach($group in $execute){
            $core=Join-Path $run ('core-'+$group)
            $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--self-test','--test-scope',$group,'--data-dir',$core) -LogPrefix (Join-Path $run ('core-'+$group))
            if($exit -ne 0){throw "Behavioral tests failed: $group ($exit). Read '$core/tests.json'."}
            $summary=Get-Content (Join-Path $core 'tests.json') -Raw|ConvertFrom-Json
            $expected=@($catalog.cases|Where-Object{$group -ceq 'All' -or $_.scope -ceq $group}|ForEach-Object name)
            if($summary.catalogOnly -or $summary.failed -ne 0 -or $summary.passed -ne $expected.Count -or
                $summary.selected -ne $expected.Count -or $summary.registered -ne $catalog.cases.Count -or
                (Compare-Object $expected @($summary.tests.name)) -or @($summary.tests|Where-Object{-not $_.passed}).Count){throw "Incomplete or catalog-only behavioral execution: $group"}
            $receipt.results+=@{scope=$group;passed=$summary.passed;failed=$summary.failed;cases=$summary.tests.name}
            Write-Host "$($summary.passed) behavioral tests passed ($group)."
        }
        if($nativeGroups.Count -eq 4){
            foreach($bad in @(
                @{name='unknown';arguments=@('--self-test','--test-scope','Unknown')},
                @{name='not-self-test';arguments=@('--test-scope','Storage')},
                @{name='duplicate';arguments=@('--self-test','--test-scope','Storage','--test-scope','Editor')}
            )){
                $exit=Invoke-RedTildeHarness -Executable $exe -Arguments ($bad.arguments+@('--data-dir',(Join-Path $run ('bad-'+$bad.name)))) -LogPrefix (Join-Path $run ('bad-'+$bad.name))
                if($exit -ne 1){throw "Native scope argument refusal failed: $($bad.name) ($exit)"}
            }
            $refusal=Join-Path $run 'interactive-refusal.json'
            $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--design-interactive-probe',$refusal,'--data-dir',(Join-Path $run 'interactive-refusal-data')) -LogPrefix (Join-Path $run 'interactive-refusal')
            $refused=Get-Content -LiteralPath $refusal -Raw|ConvertFrom-Json
            if($exit -ne 1 -or $refused.passed -or $refused.leaseGuarded -or $refused.realKeyboardInput -or
                $refused.injectedEvents -ne 0 -or @($refused.tests).Count -ne 1 -or $refused.tests[0].name -cne 'Lease.Setup'){
                throw 'An unleased interactive child did not refuse before showing or injecting input.'
            }
            $receipt.results+=@{scope='InteractiveProbeRefusal';noLeaseNoInput=$true;childSha256=(Get-FileHash -LiteralPath $refusal).Hash}
        }
    }
    if('Preferences' -in $Suites){
        $preferences=Join-Path $run 'preferences-exit'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--preferences-probe','--data-dir',$preferences) -LogPrefix (Join-Path $run 'preferences')
        if ($exit -ne 0) { throw 'Preference shutdown probe failed.' }
        $savedPreferences=Get-Content (Join-Path $preferences 'RedTilde.settings') -Raw
        if ($savedPreferences -notmatch '(?m)^theme=3\r?$') { throw 'Latest preferences were not durable after process exit.' }
        $receipt.results+=@{scope='PreferencesProcessExit';latestStateDurable=$true}
    }
    if('RecoveryRestart' -in $Suites){
        $recovery=Join-Path $run 'recovery-restart'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--recovery-crash-probe','--data-dir',$recovery) -LogPrefix (Join-Path $run 'recovery-crash')
        if ($exit -eq 0 -or $exit -eq 6) { throw 'Recovery crash probe failed to durably checkpoint before its intentional crash.' }
        $snapshots=@(Get-ChildItem (Join-Path $recovery 'Recovery') -Filter '*.rtd' -File)
        if ($snapshots.Count -ne 1) { throw 'Expected one isolated recovery snapshot after crash.' }
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--recovery-restart-probe','--data-dir',$recovery) -LogPrefix (Join-Path $run 'recovery-restart')
        if ($exit -ne 0) { throw 'Restart did not restore the exact unsaved source as a dirty Save As copy.' }
        $receipt.results+=@{scope='RecoveryCrashRestart';exactSourceRestored=$true;originalProtectedBySaveAs=$true}
    }
    if('Performance' -in $Suites){
        $performance=Join-Path $run 'performance.json'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--benchmark',$performance,'--data-dir',(Join-Path $run 'benchmark-data')) -LogPrefix (Join-Path $run 'performance')
        if ($exit -ne 0) { throw 'Application performance fixture failed.' }
        $metrics=Get-Content $performance -Raw | ConvertFrom-Json
        $receipt.results+=@{scope='PerformanceBaseline';metrics=$metrics}
        Write-Host ("Native forced render: {0:N1} FPS; private memory {1:N1} MiB; dirty edit/render {2:N2} ms. Initial app baseline only." -f $metrics.cleanRenderFps,($metrics.privateBytes/1MB),$metrics.dirtyEditAndRenderMs)
    }
    if ('Captures' -in $Suites) {
        $scenarios=@('compose-light','compose-dark','source','split','context-selection','local-source','settings','math','math-dark','mermaid','recovery','narrow','high-contrast','files','find','focus','equation','quote-scroll-light','quote-scroll-dark','diagram-label-light','diagram-label-dark')
        $captures=@($scenarios|ForEach-Object{@{name=$_;scenario=$_;arguments=@()}})
        $captures+=@(96,144,192|ForEach-Object{@{name="narrow-dpi-$_";scenario='narrow';arguments=@('--capture-dpi',"$_")}})
        $captures+=@(96,144,192|ForEach-Object{@{name="reference-dark-dpi-$_";scenario='reference-dark';arguments=@('--capture-dpi',"$_",'--document',(Join-Path $PSScriptRoot 'Tests/Fixtures/reference-dark.md'))}})
        $captures+=@(96,144,192|ForEach-Object{@{name="markdown-style-dpi-$_";scenario='markdown-style';arguments=@('--capture-dpi',"$_")}})
        foreach($hover in @('caption-close-hover','caption-maximize-hover')) {
            $captures+=@(96,144,192|ForEach-Object{@{name="$hover-dpi-$_";scenario=$hover;arguments=@('--capture-dpi',"$_")}})
        }
        foreach ($capture in $captures) {
            $scenario=$capture.scenario
            $name=$capture.name
            $output=Join-Path $run ($name+'.png')
            $exit=Invoke-RedTildeHarness -Executable $exe -Arguments (@('--capture',$output,'--scenario',$scenario,'--data-dir',(Join-Path $run ('capture-data-'+$name)))+$capture.arguments) -LogPrefix (Join-Path $run $name)
            if ($exit -ne 0 -or -not (Test-Path -LiteralPath $output)) { throw "Capture '$scenario' failed ($exit)." }
            $sidecar=$output+'.scenario.json'
            $geometry=Get-Content -LiteralPath $sidecar -Raw|ConvertFrom-Json
            if($geometry.schemaVersion -ne 1 -or $geometry.scenario -cne $scenario -or -not $geometry.hidden -or
                $geometry.actualWidthDip -lt 800 -or $geometry.actualHeightDip -lt 560 -or
                ($geometry.requestedDpi -and $geometry.requestedDpi -ne $geometry.actualHostDpi)) { throw "Capture '$name' has invalid geometry provenance." }
            $png=[IO.File]::ReadAllBytes($output)
            if($png.Length -lt 24 -or [Convert]::ToHexString($png[0..7]) -cne '89504E470D0A1A0A'){throw "Capture '$name' has no PNG header."}
            $width=[uint32]$png[16]*16777216+[uint32]$png[17]*65536+[uint32]$png[18]*256+[uint32]$png[19]
            $height=[uint32]$png[20]*16777216+[uint32]$png[21]*65536+[uint32]$png[22]*256+[uint32]$png[23]
            if($width -ne $geometry.actualWidthPx -or $height -ne $geometry.actualHeightPx) { throw "Capture '$name' pixels differ from their scenario provenance." }
            $receipt.results+=@{scope='Capture';name=$name;scenario=$scenario;sha256=(Get-FileHash $output).Hash;
                provenanceSha256=(Get-FileHash $sidecar).Hash;geometry=$geometry}
        }
        foreach($dpi in @(96,144,192)) {
            $rest=(Get-FileHash -LiteralPath (Join-Path $run "markdown-style-dpi-$dpi.png")).Hash
            $close=(Get-FileHash -LiteralPath (Join-Path $run "caption-close-hover-dpi-$dpi.png")).Hash
            $maximize=(Get-FileHash -LiteralPath (Join-Path $run "caption-maximize-hover-dpi-$dpi.png")).Hash
            if($rest -eq $close -or $rest -eq $maximize -or $close -eq $maximize){throw "Caption hover captures must differ from resting pixels and each other at $dpi DPI."}
        }
        Write-Host "$($captures.Count) actual hidden application captures generated with viewport/DPI provenance."
    }
    if ('Crash' -in $Suites) {
        $crashData=Join-Path $run 'crash-probe'
        $exit=Invoke-RedTildeHarness -Executable $exe -Arguments @('--crash-probe','--data-dir',$crashData) -LogPrefix (Join-Path $run 'crash')
        $dumps=@(Get-ChildItem (Join-Path $crashData 'Crashes') -Filter 'RedTilde-*.dmp' -File)
        if ($exit -eq 0 -or $dumps.Count -ne 1 -or -not (Test-Path (Join-Path $crashData 'Crashes/pending.txt'))) { throw 'Crash probe did not produce one local dump and marker.' }
        $signature=[IO.File]::ReadAllBytes($dumps[0].FullName)
        if ([Text.Encoding]::ASCII.GetString($signature,0,4) -ne 'MDMP') { throw 'Crash report has no minidump signature.' }
        $receipt.results+=@{scope='Crash';exitCode=$exit;dumpSha256=(Get-FileHash $dumps[0].FullName).Hash;pendingMarker=$true}
        Write-Host 'Isolated local crash dump and marker verified.'
    }
    if ($Configuration -eq 'ASan Debug' -and 'Sanitizer' -in $Suites) {
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

} finally { Complete-TestRunEvidence -Run $evidence -Passed ([bool]($receipt -and $receipt.passed)) }
