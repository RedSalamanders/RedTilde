#Requires -Version 7.2
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent
$installerScript = Join-Path $repoRoot 'Installer/Install-RedTilde.ps1'
if (-not (Test-Path -LiteralPath $installerScript -PathType Leaf)) {
    throw "Missing installer script: $installerScript"
}

$scratchRoot = Join-Path $repoRoot ('.build/installer-tests/' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($scratchRoot)

function Assert-True {
    param([bool]$Condition, [Parameter(Mandatory)][string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ThrowsContaining {
    param(
        [Parameter(Mandatory)][scriptblock]$Action,
        [Parameter(Mandatory)][string]$ExpectedFragment,
        [Parameter(Mandatory)][string]$CaseName
    )
    $thrown = $false
    try {
        & $Action | Out-Null
    } catch {
        $thrown = $true
        $msg = $_.Exception.Message
        if ($msg.IndexOf($ExpectedFragment, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            throw "${CaseName}: unexpected error '$msg' (expected '$ExpectedFragment')."
        }
    }
    if (-not $thrown) {
        throw "${CaseName}: expected refusal containing '$ExpectedFragment', but succeeded."
    }
}

$fakeExeSource = Join-Path $scratchRoot 'FakeRedTilde.cs'
[IO.File]::WriteAllText($fakeExeSource, @'
using System;
using System.Reflection;
[assembly: AssemblyVersion("0.1.42.0")]
[assembly: AssemblyFileVersion("0.1.42.0")]
class FakeRedTilde { static int Main() { return 0; } }
'@)
$csc = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (-not (Test-Path -LiteralPath $csc -PathType Leaf)) {
    throw "Required C# compiler missing for synthetic PE version fixture: $csc"
}

$fakeRegistry = Join-Path $scratchRoot 'registry.json'
$fakePrograms = Join-Path $scratchRoot 'programs-menu'
[void][IO.Directory]::CreateDirectory($fakePrograms)

$sourceV1 = Join-Path $scratchRoot 'portable-v1'
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV1 'res'))
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV1 'examples/nested'))
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV1 'Dependencies'))
$exeV1 = Join-Path $sourceV1 'RedTilde.exe'
$null = & $csc /nologo /out:$exeV1 $fakeExeSource 2>&1
if ($LASTEXITCODE -ne 0) { throw 'Failed to compile fake v1 RedTilde.exe fixture.' }
[IO.File]::WriteAllText((Join-Path $sourceV1 'res/RedTilde.png'), 'png-v1')
[IO.File]::WriteAllText((Join-Path $sourceV1 'examples/nested/sample.md'), '# Sample v1')
[IO.File]::WriteAllText((Join-Path $sourceV1 'Dependencies/stale-v1.txt'), 'old-dep')
[IO.File]::WriteAllText((Join-Path $sourceV1 'obsolete-top.txt'), 'remove-on-upgrade')

$sourceV2 = Join-Path $scratchRoot 'portable-v2'
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV2 'res'))
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV2 'examples/nested'))
[void][IO.Directory]::CreateDirectory((Join-Path $sourceV2 'Dependencies'))
$fakeExeV2Source = Join-Path $scratchRoot 'FakeRedTildeV2.cs'
[IO.File]::WriteAllText($fakeExeV2Source, @'
using System;
using System.Reflection;
[assembly: AssemblyVersion("0.1.43.0")]
[assembly: AssemblyFileVersion("0.1.43.0")]
class FakeRedTilde { static int Main() { return 0; } }
'@)
$exeV2 = Join-Path $sourceV2 'RedTilde.exe'
$null = & $csc /nologo /out:$exeV2 $fakeExeV2Source 2>&1
if ($LASTEXITCODE -ne 0) { throw 'Failed to compile fake v2 RedTilde.exe fixture.' }
[IO.File]::WriteAllText((Join-Path $sourceV2 'res/RedTilde.png'), 'png-v2')
[IO.File]::WriteAllText((Join-Path $sourceV2 'examples/nested/sample.md'), '# Sample v2')
[IO.File]::WriteAllText((Join-Path $sourceV2 'Dependencies/new-v2.txt'), 'new-dep')

$installCustom = Join-Path $scratchRoot 'install-custom'
$installOther = Join-Path $scratchRoot 'install-other'

# 1. Refusal checks (path syntax, protected locations, ancestors of user data, overlap, dangling link, unowned uninstall).
foreach ($badCase in @(
    @{ name = 'empty'; dir = '   '; err = 'non-empty' },
    @{ name = 'drive-relative-slash'; dir = '\RedTilde'; err = 'drive-qualified' },
    @{ name = 'drive-relative-cur'; dir = 'C:RedTilde'; err = 'drive-qualified' },
    @{ name = 'drive-root'; dir = 'C:\'; err = 'drive root' },
    @{ name = 'unc'; dir = '\\server\share\RedTilde'; err = 'UNC' },
    @{ name = 'device-ns'; dir = '\\?\C:\Temp\RedTilde'; err = 'device-namespace' },
    @{ name = 'device-ns-fwd'; dir = '//?/C:/Temp/RedTilde'; err = 'device-namespace' },
    @{ name = 'dotdot'; dir = "$scratchRoot\..\RedTilde"; err = "parent '..'" }
)) {
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $badCase.dir -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    } $badCase.err "refuse-$($badCase.name)"
}

if ($env:LOCALAPPDATA) {
    foreach ($dataBad in @(
        (Join-Path $env:LOCALAPPDATA 'RedTilde'),
        (Join-Path $env:LOCALAPPDATA 'RedTilde\Nested'),
        $env:LOCALAPPDATA,
        (Join-Path $env:LOCALAPPDATA 'Programs')
    )) {
        Assert-ThrowsContaining {
            & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $dataBad -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
        } 'cannot' "refuse-userdata-$dataBad"
    }
}
if ($env:USERPROFILE) {
    foreach ($profileBad in @($env:USERPROFILE, (Join-Path $env:USERPROFILE 'Desktop'), (Join-Path $env:USERPROFILE 'Documents'))) {
        Assert-ThrowsContaining {
            & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $profileBad -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
        } 'cannot' "refuse-profile-$profileBad"
    }
}

# Parent/child source-target overlap refusal
Assert-ThrowsContaining {
    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory (Join-Path $sourceV1 'subinstall') -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
} 'overlap' 'refuse-child-overlap'

# Dangling junction in ancestry refusal
$junctionParent = Join-Path $scratchRoot 'junction-ancestry'
[void][IO.Directory]::CreateDirectory($junctionParent)
$danglingTarget = Join-Path $scratchRoot 'nonexistent-target-for-junction'
[void][IO.Directory]::CreateDirectory($danglingTarget)
$danglingLink = Join-Path $junctionParent 'link'
$null = New-Item -ItemType Junction -Path $danglingLink -Target $danglingTarget
[IO.Directory]::Delete($danglingTarget)
Assert-ThrowsContaining {
    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory (Join-Path $danglingLink 'RedTilde') -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
} 'reparse' 'refuse-dangling-junction'

# Unowned directory uninstall refusal (has RedTilde.exe ONLY, no install marker or installer or InstallLocation)
$unownedDir = Join-Path $scratchRoot 'unowned-app'
[void][IO.Directory]::CreateDirectory($unownedDir)
[IO.File]::Copy($exeV1, (Join-Path $unownedDir 'RedTilde.exe'), $true)
[IO.File]::WriteAllText((Join-Path $unownedDir 'other-user-file.txt'), 'must-survive')
Assert-ThrowsContaining {
    & $installerScript -InstallDirectory $unownedDir -Uninstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
} 'ownership' 'refuse-unowned-uninstall'
Assert-True (Test-Path -LiteralPath (Join-Path $unownedDir 'other-user-file.txt')) 'unowned directory files survived refused uninstall'

# 2. Fresh install with file associations into custom directory (with trailing slash)
& $installerScript -SourceDirectory ($sourceV1 + '\') -InstallDirectory ($installCustom + '\') -RegisterFileAssociation -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
Assert-True (Test-Path -LiteralPath (Join-Path $installCustom 'RedTilde.exe') -PathType Leaf) 'v1 installed RedTilde.exe'
Assert-True (Test-Path -LiteralPath (Join-Path $installCustom 'Install-RedTilde.ps1') -PathType Leaf) 'v1 copied installer'
Assert-True (Test-Path -LiteralPath (Join-Path $installCustom '.redtilde-install.json') -PathType Leaf) 'v1 wrote install marker'
Assert-True (Test-Path -LiteralPath (Join-Path $fakePrograms 'RedTilde.lnk') -PathType Leaf) 'v1 created Start Menu shortcut'

$reg1 = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
$uninstallReg = $reg1.'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'
Assert-True ($uninstallReg.InstallLocation -ceq $installCustom) 'InstallLocation matches normalized custom dir'
Assert-True ($uninstallReg.DisplayVersion -ceq '0.1.42') 'DisplayVersion matches v1 PE version'
Assert-True ($uninstallReg.UninstallString -match '^"[^"]+(powershell|pwsh)\.exe"\s+-NoProfile\s+-ExecutionPolicy\s+Bypass\s+-File\s+"') 'UninstallString uses quoted resolved shell path'
$appCmd = $reg1.'Software\Classes\Applications\RedTilde.exe\shell\open\command'.'(Default)'
Assert-True ($appCmd -ceq "`"$(Join-Path $installCustom 'RedTilde.exe')`" `"%1`"") 'Applications\RedTilde.exe open command registered'
$progIdCmd = $reg1.'Software\Classes\RedTilde.Markdown\shell\open\command'.'(Default)'
Assert-True ($progIdCmd -ceq $appCmd) 'ProgId open command registered'

# 3. Upgrade over v1 in place: must NOT nest res/res or examples/examples, must update files & clean obsolete v1 files!
& $installerScript -SourceDirectory $sourceV2 -InstallDirectory $installCustom -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
Assert-True (-not (Test-Path -LiteralPath (Join-Path $installCustom 'res/res'))) 'upgrade did not nest res/res'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $installCustom 'examples/examples'))) 'upgrade did not nest examples/examples'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $installCustom 'Dependencies/Dependencies'))) 'upgrade did not nest Dependencies/Dependencies'
Assert-True (([IO.File]::ReadAllText((Join-Path $installCustom 'res/RedTilde.png'))) -ceq 'png-v2') 'upgrade updated nested res asset'
Assert-True (([IO.File]::ReadAllText((Join-Path $installCustom 'examples/nested/sample.md'))) -ceq '# Sample v2') 'upgrade updated nested example asset'
Assert-True (Test-Path -LiteralPath (Join-Path $installCustom 'Dependencies/new-v2.txt')) 'upgrade copied new dependency notice'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $installCustom 'Dependencies/stale-v1.txt'))) 'upgrade pruned stale dependency notice'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $installCustom 'obsolete-top.txt'))) 'upgrade pruned stale top-level file'
$regUp = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
Assert-True ($regUp.'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'.DisplayVersion -ceq '0.1.43') 'upgrade updated DisplayVersion to 0.1.43'

# 4. Re-install to another custom directory WITHOUT -RegisterFileAssociation: existing associations must update to new exe!
& $installerScript -SourceDirectory $sourceV2 -InstallDirectory $installOther -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
$regMoved = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
$expectedMovedExe = Join-Path $installOther 'RedTilde.exe'
Assert-True ($regMoved.'Software\Classes\RedTilde.Markdown\shell\open\command'.'(Default)' -ceq "`"$expectedMovedExe`" `"%1`"") 'custom-path reinstall updated ProgId command'
Assert-True ($regMoved.'Software\Classes\Applications\RedTilde.exe\shell\open\command'.'(Default)' -ceq "`"$expectedMovedExe`" `"%1`"") 'custom-path reinstall updated Applications command'

# 5. Uninstall WITHOUT -InstallDirectory while CWD is inside installOther -> reads registered InstallLocation, steps out of CWD, removes files & registry!
$savedLocation = Get-Location
try {
    Set-Location -LiteralPath $installOther
    & $installerScript -Uninstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
} finally {
    Set-Location -LiteralPath $savedLocation.ProviderPath
}
Assert-True (-not (Test-Path -LiteralPath $installOther)) 'uninstall without -InstallDirectory removed registered custom InstallLocation even when CWD was inside it'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $fakePrograms 'RedTilde.lnk'))) 'uninstall removed Start Menu shortcut'
$regAfterUninstall = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
Assert-True ($null -eq $regAfterUninstall.PSObject.Properties['Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde']) 'uninstall removed Uninstall key'
Assert-True ($null -eq $regAfterUninstall.PSObject.Properties['Software\Classes\RedTilde.Markdown']) 'uninstall removed ProgId key'
Assert-True ($null -eq $regAfterUninstall.PSObject.Properties['Software\Classes\Applications\RedTilde.exe\SupportedTypes']) 'uninstall removed SupportedTypes'
Assert-True ($null -eq $regAfterUninstall.PSObject.Properties['Software\Classes\Applications\RedTilde.exe\shell\open\command']) 'uninstall removed Applications command'

# 6. Check Windows PowerShell 5.1 parser compatibility of Install-RedTilde.ps1 if available
$winPs = if ($env:WINDIR) { Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe' } else { '' }
if ($winPs -and (Test-Path -LiteralPath $winPs -PathType Leaf)) {
    $escapedInstaller = $installerScript.Replace("'", "''")
    $psCommand = '$e=$null; [void][Management.Automation.Language.Parser]::ParseFile(''' + $escapedInstaller + ''',[ref]$null,[ref]$e); if($e.Count){throw $e[0].Message}'
    $null = & $winPs -NoProfile -Command $psCommand
    Assert-True ($LASTEXITCODE -eq 0) 'Install-RedTilde.ps1 parses cleanly in Windows PowerShell 5.1'
}

Remove-Item -LiteralPath $scratchRoot -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "PASS: per-user installer behavioral regressions (fresh install, non-nesting upgrade, association updates, CWD-safe uninstall, path & ownership refusals)."
$global:LASTEXITCODE = 0
