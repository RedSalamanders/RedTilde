<#
.SYNOPSIS Runs RedTilde's real keyboard/focus checks under the exact pinned DxUi desktop lease.
.DESCRIPTION Requires the person's agreement to the time. The canonical DxUi runner owns the warning,
session lease, child watchdog and verified restoration of foreground, keyboard focus and pointer.
Builds the pinned lease target and retains source/binary/pin/scenario results under .build/evidence.
.PARAMETER InteractiveAutoStart Use only with explicit agreement; enables the visible 30-second Start countdown.
.PARAMETER SkipBuild Reuse an exact receipt-verified application build. The lease target is still checked by MSBuild.
.PARAMETER Platform x64 or native ARM64; cross-built ARM64 cannot run on x64.
.PARAMETER Configuration Debug, Release or ASan Debug.
#>
[CmdletBinding()]
param([ValidateSet('x64','ARM64')][string]$Platform='x64',
      [ValidateSet('Debug','Release','ASan Debug')][string]$Configuration='Debug',
      [switch]$SkipBuild,[switch]$InteractiveAutoStart)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/ScopedTesting.psm1') -Force
Assert-ScopedRuntimePlatform -Platform $Platform
if(-not $SkipBuild){& (Join-Path $PSScriptRoot 'build.ps1') -Platform $Platform -Configuration $Configuration}
Import-Module (Join-Path $PSScriptRoot 'Build/Harness.psm1') -Force
$artifact=Assert-RedTildeTestArtifact -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
$pinPath=Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json'
$pin=Get-Content -LiteralPath $pinPath -Raw|ConvertFrom-Json
$source=Join-Path $PSScriptRoot ".build/dependencies/DxUi/source/$($pin.commit.Substring(0,16))"
& (Join-Path $source 'Tools/validate_consumer.ps1') -DxUiRoot $source -LockFile $pinPath
if($LASTEXITCODE){throw 'The canonical interactive lease source does not match the exact dependency pin.'}
$resolved=[xml](Get-Content -LiteralPath (Join-Path $PSScriptRoot ".build/dependencies/DxUi/DxUi.resolved.$Platform.props") -Raw)
$output=[string]$resolved.Project.PropertyGroup.DxUiConsumerOutputRoot
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
$msbuild=Get-RedTildeMSBuild
$leaseLog=Join-Path $PSScriptRoot ('.build/logs/interactive-lease-'+[guid]::NewGuid().ToString('N')+'.log')
$pinnedBuildArguments=@(Get-RedTildePinnedBuildArguments -Configuration $Configuration -Identity $artifact.Build.dependency.Identity)
& $msbuild (Join-Path $source 'Tests/InteractiveLease/DxUi.InteractiveLease.vcxproj') /nologo /m /v:minimal "/p:Platform=$Platform" "/p:Configuration=$Configuration" "/p:DxUiOutputRoot=$output" @pinnedBuildArguments "/flp:LogFile=$leaseLog;Verbosity=normal"
if($LASTEXITCODE){throw "The exact pinned desktop lease did not build. See '$leaseLog'."}
$leaseExe=Join-Path $output "$Platform/$Configuration/DxUi.InteractiveLease.exe"
$leaseSha256=(Get-FileHash -LiteralPath $leaseExe).Hash
Import-Module (Join-Path $source 'Tools/InteractiveRun.psm1') -Force
$desktopProblem=Test-DxUiDesktopAvailable -Executable $leaseExe
if($desktopProblem){throw "Interactive tests need an available desktop: $desktopProblem"}
Import-Module (Join-Path $PSScriptRoot 'Build/TestRunEvidence.psm1') -Force
$evidence=Start-TestRunEvidence -Repository $PSScriptRoot -Context @{project='RedTilde';platform=$Platform;configuration=$Configuration;suites=@('DesignInteractive');sourceIdentity=$artifact.SourceIdentity}
$passed=$false
try {
    $run=$evidence.Path
    $result=Join-Path $run 'design-interactive.json'
    $arguments=@('--design-interactive-probe',$result,'--data-dir',(Join-Path $run 'app-data'))
    $child=[pscustomobject]@{Name='RedTildeDesignInteractive';Log=(Join-Path $run 'child.log');CommandLine=(ConvertTo-DxUiCommandLine -Program $artifact.Executable -Arguments $arguments)}
    $lease=Invoke-DxUiInteractiveLease -Executable $leaseExe -Runs @($child) -Label 'RedTilde keyboard and focus checks' -EstimateSeconds 70 -WorkDirectory $run -ChildTimeoutSeconds 120 -AutoStart:$InteractiveAutoStart
    $problems=@(Get-DxUiLeaseProblems -Result $lease.Result -ExitCode $lease.ExitCode -ExpectedChildren @($child.Name))+@($lease.PolicyProblems)
    if($lease.ExitCode -or $problems.Count){throw "The interactive lease failed: $($problems -join '; ') (exit $($lease.ExitCode))."}
    $checks=Get-Content -LiteralPath $result -Raw|ConvertFrom-Json
    $required=@('NativeEditor.KeyboardTextAndUndo','Files.ShortcutFocusAndEscape','Find.TypedQueryTraversalAndEscape',
        'Settings.KeyboardDrawerAndEscape','Focus.KeyboardToggleAndEscape','SelectionToolbar.AltF10TabEnterEscape',
        'Equation.F2InPlaceEditUndoAndExit','FileMenu.KeyboardOpenAndDismiss','Window.AltF4AsyncClose')
    if($checks.schemaVersion -ne 1 -or -not $checks.passed -or -not $checks.leaseGuarded -or -not $checks.realKeyboardInput -or
        $checks.clipboardTouched -or @($checks.tests).Count -ne $required.Count -or
        @($checks.tests|Where-Object{-not $_.passed}).Count -or (Compare-Object $required @($checks.tests.name))){
        throw 'The keyboard/focus child did not complete every required check.'
    }
    $finalArtifact=Assert-RedTildeTestArtifact -Root $PSScriptRoot -Platform $Platform -Configuration $Configuration
    if($finalArtifact.SourceIdentity -cne $artifact.SourceIdentity -or $finalArtifact.ReceiptSha256 -cne $artifact.ReceiptSha256 -or
        $finalArtifact.Build.binarySha256 -cne $artifact.Build.binarySha256 -or
        $finalArtifact.Build.artifactIdentity -cne $artifact.Build.artifactIdentity -or
        (Get-FileHash -LiteralPath $leaseExe).Hash -cne $leaseSha256){
        throw 'Application source, build receipt, runtime closure or lease binary changed during the desktop run.'
    }
    [ordered]@{schemaVersion=1;passed=$true;interactive=$true;sourceIdentity=$artifact.SourceIdentity;dependency=$pin.commit;
        binarySha256=$artifact.Build.binarySha256;buildReceiptSha256=$artifact.ReceiptSha256;platform=$Platform;configuration=$Configuration;
        arguments=$arguments;leaseBinarySha256=$leaseSha256;lease=$lease.Result;
        childResultSha256=(Get-FileHash -LiteralPath $result).Hash;tests=$checks.tests;finishedUtc=[DateTime]::UtcNow.ToString('o')}|
        ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $run 'receipt.json') -Encoding utf8
    $passed=$true
    Write-Host "Real keyboard/focus checks and desktop restoration checks passed. Evidence: $run"
} finally {Complete-TestRunEvidence -Run $evidence -Passed $passed}
