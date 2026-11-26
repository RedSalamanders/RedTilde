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

if (-not ('RedTildeTestHiveLoader' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;

public static class RedTildeTestHiveLoader {
    private const uint KEY_ALL_ACCESS = 0xF003F;
    private const uint REG_PROCESS_APPKEY = 0x00000001;

    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = false)]
    private static extern int RegLoadAppKeyW(
        string lpFile,
        out IntPtr phkResult,
        uint samDesired,
        uint dwOptions,
        uint Reserved);

    public static RegistryKey LoadAppHive(string hiveFilePath) {
        IntPtr rawHandle;
        int rc = RegLoadAppKeyW(hiveFilePath, out rawHandle, KEY_ALL_ACCESS, REG_PROCESS_APPKEY, 0);
        if (rc != 0) {
            throw new Win32Exception(rc, "RegLoadAppKeyW failed with error " + rc);
        }
        SafeRegistryHandle safeHandle = new SafeRegistryHandle(rawHandle, true);
        return RegistryKey.FromHandle(safeHandle);
    }
}
'@
}

$scratchRoot = Join-Path $repoRoot ('.build/installer-tests/' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($scratchRoot)
$junctions = [Collections.Generic.List[string]]::new()
$hiveRoot = $null

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

function Set-HiveValue {
    param(
        [Parameter(Mandatory)][Microsoft.Win32.RegistryKey]$RootKey,
        [Parameter(Mandatory)][string]$SubKey,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Name,
        [Parameter(Mandatory)][AllowEmptyString()]$Value
    )
    $sub = $RootKey.CreateSubKey($SubKey, $true)
    if ($null -eq $sub) {
        throw "Failed to create hive subkey '$SubKey'."
    }
    try {
        $sub.SetValue($Name, [string]$Value, [Microsoft.Win32.RegistryValueKind]::String)
    } finally {
        $sub.Dispose()
    }
}

function Get-HiveSubKeySnapshot {
    param(
        [Parameter(Mandatory)][Microsoft.Win32.RegistryKey]$RootKey,
        [Parameter(Mandatory)][string]$SubKey
    )
    $sub = $RootKey.OpenSubKey($SubKey, $false)
    if ($null -eq $sub) {
        return $null
    }
    try {
        $values = @{}
        $kinds = @{}
        $names = @($sub.GetValueNames())
        foreach ($name in $names) {
            $values[$name] = $sub.GetValue($name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
            $kinds[$name] = $sub.GetValueKind($name)
        }
        return [pscustomobject]@{
            ValueNames = $names
            Values     = $values
            Kinds      = $kinds
        }
    } finally {
        $sub.Dispose()
    }
}

try {
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
    $junctions.Add($danglingLink)
    [IO.Directory]::Delete($danglingTarget)
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory (Join-Path $danglingLink 'RedTilde') -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    } 'reparse' 'refuse-dangling-junction'

    # Unowned directory uninstall refusal (has RedTilde.exe ONLY, no install marker)
    $unownedDir = Join-Path $scratchRoot 'unowned-app'
    [void][IO.Directory]::CreateDirectory($unownedDir)
    [IO.File]::Copy($exeV1, (Join-Path $unownedDir 'RedTilde.exe'), $true)
    [IO.File]::WriteAllText((Join-Path $unownedDir 'other-user-file.txt'), 'must-survive')
    Assert-ThrowsContaining {
        & $installerScript -InstallDirectory $unownedDir -Uninstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    } 'ownership' 'refuse-unowned-uninstall'
    Assert-True (Test-Path -LiteralPath (Join-Path $unownedDir 'other-user-file.txt')) 'unowned directory files survived refused uninstall'

    # 2. (a1) Pre-seeded JSON-mock sibling OpenWithProgids + Uninstall survival across install, upgrade, and uninstall
    $seededJson = [ordered]@{
        'Software\Classes\.md\OpenWithProgids' = [ordered]@{
            'VSCode.md'        = ''
            'Obsidian.md'      = ''
            'NotepadPlusPlus.md' = ''
        }
        'Software\Classes\.markdown\OpenWithProgids' = [ordered]@{
            'VSCode.md'      = ''
            'OtherEditor.md' = ''
        }
        'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde' = [ordered]@{
            'PreExisting' = 'keep'
        }
    } | ConvertTo-Json -Depth 4
    [IO.File]::WriteAllText($fakeRegistry, $seededJson, [Text.UTF8Encoding]::new($false))

    & $installerScript -SourceDirectory ($sourceV1 + '\') -InstallDirectory ($installCustom + '\') -RegisterFileAssociation -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    Assert-True (Test-Path -LiteralPath (Join-Path $installCustom 'RedTilde.exe') -PathType Leaf) 'v1 installed RedTilde.exe'
    Assert-True (Test-Path -LiteralPath (Join-Path $installCustom 'Install-RedTilde.ps1') -PathType Leaf) 'v1 copied installer'
    Assert-True (Test-Path -LiteralPath (Join-Path $installCustom '.redtilde-install.json') -PathType Leaf) 'v1 wrote install marker'
    Assert-True (Test-Path -LiteralPath (Join-Path $fakePrograms 'RedTilde.lnk') -PathType Leaf) 'v1 created Start Menu shortcut'

    $reg1 = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
    $uninstallReg = $reg1.'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'
    Assert-True ($uninstallReg.DisplayName -ceq 'RedTilde') 'DisplayName is RedTilde'
    Assert-True ($uninstallReg.DisplayVersion -ceq '0.1.42') 'DisplayVersion matches v1 PE version'
    Assert-True ($uninstallReg.Publisher -ceq 'RedSalamanders') 'Publisher is RedSalamanders'
    Assert-True ($uninstallReg.InstallLocation -ceq $installCustom) 'InstallLocation matches normalized custom dir'
    Assert-True ($uninstallReg.DisplayIcon -ceq "$(Join-Path $installCustom 'RedTilde.exe'),0") 'DisplayIcon points to installed exe'
    Assert-True ($uninstallReg.NoModify -eq 1) 'NoModify is 1'
    Assert-True ($uninstallReg.NoRepair -eq 1) 'NoRepair is 1'
    Assert-True ($uninstallReg.UninstallString -match '^"[^"]+(powershell|pwsh)\.exe"\s+-NoProfile\s+-ExecutionPolicy\s+Bypass\s+-File\s+"') 'UninstallString uses quoted resolved shell path'
    Assert-True ($uninstallReg.PreExisting -ceq 'keep') 'Pre-existing Uninstall value survived'
    $appCmd = $reg1.'Software\Classes\Applications\RedTilde.exe\shell\open\command'.'(Default)'
    Assert-True ($appCmd -ceq "`"$(Join-Path $installCustom 'RedTilde.exe')`" `"%1`"") 'Applications\RedTilde.exe open command registered'
    $progIdCmd = $reg1.'Software\Classes\RedTilde.Markdown\shell\open\command'.'(Default)'
    Assert-True ($progIdCmd -ceq $appCmd) 'ProgId open command registered'
    $supportedTypes1 = $reg1.'Software\Classes\Applications\RedTilde.exe\SupportedTypes'
    Assert-True ($null -ne $supportedTypes1.PSObject.Properties['.md']) 'SupportedTypes includes .md'
    Assert-True ($null -ne $supportedTypes1.PSObject.Properties['.markdown']) 'SupportedTypes includes .markdown'
    $mdProgids1 = $reg1.'Software\Classes\.md\OpenWithProgids'
    Assert-True ($null -ne $mdProgids1.PSObject.Properties['RedTilde.Markdown']) '.md OpenWithProgids has RedTilde.Markdown'
    Assert-True ($null -ne $mdProgids1.PSObject.Properties['VSCode.md']) '.md OpenWithProgids preserved VSCode.md'
    Assert-True ($null -ne $mdProgids1.PSObject.Properties['Obsidian.md']) '.md OpenWithProgids preserved Obsidian.md'
    Assert-True ($null -ne $mdProgids1.PSObject.Properties['NotepadPlusPlus.md']) '.md OpenWithProgids preserved NotepadPlusPlus.md'
    $markdownProgids1 = $reg1.'Software\Classes\.markdown\OpenWithProgids'
    Assert-True ($null -ne $markdownProgids1.PSObject.Properties['RedTilde.Markdown']) '.markdown OpenWithProgids has RedTilde.Markdown'
    Assert-True ($null -ne $markdownProgids1.PSObject.Properties['VSCode.md']) '.markdown OpenWithProgids preserved VSCode.md'
    Assert-True ($null -ne $markdownProgids1.PSObject.Properties['OtherEditor.md']) '.markdown OpenWithProgids preserved OtherEditor.md'

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
    Assert-True ($regUp.'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'.PreExisting -ceq 'keep') 'upgrade preserved PreExisting Uninstall value'
    Assert-True ($null -ne $regUp.'Software\Classes\Applications\RedTilde.exe\SupportedTypes'.PSObject.Properties['.md']) 'upgrade preserved SupportedTypes .md'
    Assert-True ($null -ne $regUp.'Software\Classes\Applications\RedTilde.exe\SupportedTypes'.PSObject.Properties['.markdown']) 'upgrade preserved SupportedTypes .markdown'
    Assert-True ($null -ne $regUp.'Software\Classes\.md\OpenWithProgids'.PSObject.Properties['VSCode.md']) 'upgrade preserved .md sibling VSCode.md'
    Assert-True ($null -ne $regUp.'Software\Classes\.markdown\OpenWithProgids'.PSObject.Properties['OtherEditor.md']) 'upgrade preserved .markdown sibling OtherEditor.md'

    # 4. Re-install to another custom directory WITHOUT -RegisterFileAssociation: existing associations must update to new exe!
    & $installerScript -SourceDirectory $sourceV2 -InstallDirectory $installOther -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    $regMoved = Get-Content -LiteralPath $fakeRegistry -Raw | ConvertFrom-Json
    $expectedMovedExe = Join-Path $installOther 'RedTilde.exe'
    Assert-True ($regMoved.'Software\Classes\RedTilde.Markdown\shell\open\command'.'(Default)' -ceq "`"$expectedMovedExe`" `"%1`"") 'custom-path reinstall updated ProgId command'
    Assert-True ($regMoved.'Software\Classes\Applications\RedTilde.exe\shell\open\command'.'(Default)' -ceq "`"$expectedMovedExe`" `"%1`"") 'custom-path reinstall updated Applications command'

    # 5. Uninstall WITHOUT -InstallDirectory while CWD is inside installOther -> reads registered InstallLocation, steps out of CWD, removes files & registry, preserves siblings!
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
    $mdAfterUninstall = $regAfterUninstall.'Software\Classes\.md\OpenWithProgids'
    Assert-True ($null -ne $mdAfterUninstall) '.md OpenWithProgids key survived uninstall'
    Assert-True ($null -eq $mdAfterUninstall.PSObject.Properties['RedTilde.Markdown']) '.md OpenWithProgids removed RedTilde.Markdown'
    Assert-True ($null -ne $mdAfterUninstall.PSObject.Properties['VSCode.md']) '.md OpenWithProgids preserved VSCode.md after uninstall'
    Assert-True ($null -ne $mdAfterUninstall.PSObject.Properties['Obsidian.md']) '.md OpenWithProgids preserved Obsidian.md after uninstall'
    Assert-True ($null -ne $mdAfterUninstall.PSObject.Properties['NotepadPlusPlus.md']) '.md OpenWithProgids preserved NotepadPlusPlus.md after uninstall'
    $markdownAfterUninstall = $regAfterUninstall.'Software\Classes\.markdown\OpenWithProgids'
    Assert-True ($null -ne $markdownAfterUninstall) '.markdown OpenWithProgids key survived uninstall'
    Assert-True ($null -eq $markdownAfterUninstall.PSObject.Properties['RedTilde.Markdown']) '.markdown OpenWithProgids removed RedTilde.Markdown'
    Assert-True ($null -ne $markdownAfterUninstall.PSObject.Properties['VSCode.md']) '.markdown OpenWithProgids preserved VSCode.md after uninstall'
    Assert-True ($null -ne $markdownAfterUninstall.PSObject.Properties['OtherEditor.md']) '.markdown OpenWithProgids preserved OtherEditor.md after uninstall'

    # 6. (a2) Real Windows Kernel Registry Hive (RegLoadAppKey + -TestRegistryRootKey) verifying production [Microsoft.Win32.RegistryKey] path!
    $hivePath = Join-Path $scratchRoot 'test-hive.dat'
    $hiveRoot = [RedTildeTestHiveLoader]::LoadAppHive($hivePath)
    $hivePrograms = Join-Path $scratchRoot 'hive-programs'
    [void][IO.Directory]::CreateDirectory($hivePrograms)
    $installHive = Join-Path $scratchRoot 'install-hive'

    # Guard: TestRegistryRootKey without TestProgramsMenuDirectory must throw.
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $installHive -TestRegistryRootKey $hiveRoot -SkipShellNotify
    } 'TestProgramsMenuDirectory' 'refuse-hive-without-programs-dir'

    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Classes\.md\OpenWithProgids' -Name 'VSCode.md' -Value ''
    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Classes\.md\OpenWithProgids' -Name 'Obsidian.md' -Value ''
    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Classes\.md\OpenWithProgids' -Name 'NotepadPlusPlus.md' -Value ''
    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Classes\.markdown\OpenWithProgids' -Name 'VSCode.md' -Value ''
    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Classes\.markdown\OpenWithProgids' -Name 'OtherEditor.md' -Value ''
    Set-HiveValue -RootKey $hiveRoot -SubKey 'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde' -Name 'PreExisting' -Value 'keep'

    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $installHive -RegisterFileAssociation -TestRegistryRootKey $hiveRoot -TestProgramsMenuDirectory $hivePrograms -SkipShellNotify

    $hiveUninstall = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'
    Assert-True ($null -ne $hiveUninstall) 'hive created Uninstall\RedTilde'
    Assert-True ($hiveUninstall.Values['DisplayName'] -ceq 'RedTilde') 'hive DisplayName is RedTilde'
    Assert-True ($hiveUninstall.Values['DisplayVersion'] -ceq '0.1.42') 'hive DisplayVersion is 0.1.42'
    Assert-True ($hiveUninstall.Values['Publisher'] -ceq 'RedSalamanders') 'hive Publisher is RedSalamanders'
    Assert-True ($hiveUninstall.Values['InstallLocation'] -ceq $installHive) 'hive InstallLocation matches'
    Assert-True ($hiveUninstall.Values['DisplayIcon'] -ceq "$(Join-Path $installHive 'RedTilde.exe'),0") 'hive DisplayIcon matches'
    Assert-True ($hiveUninstall.Values['NoModify'] -eq 1 -and $hiveUninstall.Kinds['NoModify'] -eq [Microsoft.Win32.RegistryValueKind]::DWord) 'hive NoModify is DWord 1'
    Assert-True ($hiveUninstall.Values['NoRepair'] -eq 1 -and $hiveUninstall.Kinds['NoRepair'] -eq [Microsoft.Win32.RegistryValueKind]::DWord) 'hive NoRepair is DWord 1'
    Assert-True (-not [string]::IsNullOrWhiteSpace([string]$hiveUninstall.Values['UninstallString'])) 'hive UninstallString set'
    Assert-True ($hiveUninstall.Values['PreExisting'] -ceq 'keep') 'hive preserved PreExisting Uninstall value'

    $expectedHiveExe = Join-Path $installHive 'RedTilde.exe'
    $expectedHiveCmd = "`"$expectedHiveExe`" `"%1`""
    foreach ($defaultCheck in @(
        @{ SubKey = 'Software\Classes\RedTilde.Markdown'; Expected = 'Markdown Document' },
        @{ SubKey = 'Software\Classes\RedTilde.Markdown\DefaultIcon'; Expected = "$expectedHiveExe,0" },
        @{ SubKey = 'Software\Classes\RedTilde.Markdown\shell\open\command'; Expected = $expectedHiveCmd },
        @{ SubKey = 'Software\Classes\Applications\RedTilde.exe\shell\open\command'; Expected = $expectedHiveCmd }
    )) {
        $snap = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey $defaultCheck.SubKey
        Assert-True ($null -ne $snap) "hive created $($defaultCheck.SubKey)"
        Assert-True ($snap.Values[''] -ceq $defaultCheck.Expected) "hive unnamed default value matches for $($defaultCheck.SubKey)"
        Assert-True ($snap.ValueNames -notcontains '(Default)') "hive $($defaultCheck.SubKey) does not have bogus literal '(Default)' value name"
    }

    $hiveSupported = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\Applications\RedTilde.exe\SupportedTypes'
    Assert-True ($null -ne $hiveSupported) 'hive created SupportedTypes'
    Assert-True ($hiveSupported.ValueNames -contains '.md') 'hive SupportedTypes has .md'
    Assert-True ($hiveSupported.ValueNames -contains '.markdown') 'hive SupportedTypes has .markdown'

    $hiveMdProgids = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\.md\OpenWithProgids'
    Assert-True ($hiveMdProgids.ValueNames -contains 'RedTilde.Markdown') 'hive .md OpenWithProgids has RedTilde.Markdown'
    Assert-True ($hiveMdProgids.ValueNames -contains 'VSCode.md') 'hive .md OpenWithProgids preserved VSCode.md'
    Assert-True ($hiveMdProgids.ValueNames -contains 'Obsidian.md') 'hive .md OpenWithProgids preserved Obsidian.md'
    Assert-True ($hiveMdProgids.ValueNames -contains 'NotepadPlusPlus.md') 'hive .md OpenWithProgids preserved NotepadPlusPlus.md'

    $hiveMarkdownProgids = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\.markdown\OpenWithProgids'
    Assert-True ($hiveMarkdownProgids.ValueNames -contains 'RedTilde.Markdown') 'hive .markdown OpenWithProgids has RedTilde.Markdown'
    Assert-True ($hiveMarkdownProgids.ValueNames -contains 'VSCode.md') 'hive .markdown OpenWithProgids preserved VSCode.md'
    Assert-True ($hiveMarkdownProgids.ValueNames -contains 'OtherEditor.md') 'hive .markdown OpenWithProgids preserved OtherEditor.md'

    # Upgrade v1 -> v2 on real kernel hive without -RegisterFileAssociation (tests Test-RedTildeRegistryKeyExists + non-destructive update)
    & $installerScript -SourceDirectory $sourceV2 -InstallDirectory $installHive -TestRegistryRootKey $hiveRoot -TestProgramsMenuDirectory $hivePrograms -SkipShellNotify
    $hiveUninstallV2 = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'
    Assert-True ($hiveUninstallV2.Values['DisplayVersion'] -ceq '0.1.43') 'hive upgrade updated DisplayVersion to 0.1.43'
    Assert-True ($hiveUninstallV2.Values['PreExisting'] -ceq 'keep') 'hive upgrade preserved PreExisting'

    # Uninstall on real kernel hive WITHOUT -InstallDirectory (reads InstallLocation from hive)
    & $installerScript -Uninstall -TestRegistryRootKey $hiveRoot -TestProgramsMenuDirectory $hivePrograms -SkipShellNotify
    Assert-True (-not (Test-Path -LiteralPath $installHive)) 'hive uninstall removed install directory'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $hivePrograms 'RedTilde.lnk'))) 'hive uninstall removed Start Menu shortcut'
    Assert-True ($null -eq (Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde')) 'hive uninstall removed Uninstall\RedTilde'
    Assert-True ($null -eq (Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\RedTilde.Markdown')) 'hive uninstall removed ProgId key'
    Assert-True ($null -eq (Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\Applications\RedTilde.exe')) 'hive uninstall removed Applications\RedTilde.exe'
    $hiveMdAfter = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\.md\OpenWithProgids'
    Assert-True ($null -ne $hiveMdAfter) 'hive .md OpenWithProgids survived uninstall'
    Assert-True ($hiveMdAfter.ValueNames -notcontains 'RedTilde.Markdown') 'hive .md OpenWithProgids removed RedTilde.Markdown'
    Assert-True ($hiveMdAfter.ValueNames -contains 'VSCode.md' -and $hiveMdAfter.ValueNames -contains 'Obsidian.md' -and $hiveMdAfter.ValueNames -contains 'NotepadPlusPlus.md') 'hive .md OpenWithProgids preserved all siblings after uninstall'
    $hiveMarkdownAfter = Get-HiveSubKeySnapshot -RootKey $hiveRoot -SubKey 'Software\Classes\.markdown\OpenWithProgids'
    Assert-True ($null -ne $hiveMarkdownAfter) 'hive .markdown OpenWithProgids survived uninstall'
    Assert-True ($hiveMarkdownAfter.ValueNames -notcontains 'RedTilde.Markdown') 'hive .markdown OpenWithProgids removed RedTilde.Markdown'
    Assert-True ($hiveMarkdownAfter.ValueNames -contains 'VSCode.md' -and $hiveMarkdownAfter.ValueNames -contains 'OtherEditor.md') 'hive .markdown OpenWithProgids preserved all siblings after uninstall'

    $hiveRoot.Dispose()
    $hiveRoot = $null

    # 7. (a3) AST guard banning New-Item and HKCU:\ RegistryProvider usage in Install-RedTilde.ps1
    $parseErrors = $null
    $installerAst = [Management.Automation.Language.Parser]::ParseFile($installerScript, [ref]$null, [ref]$parseErrors)
    Assert-True ($parseErrors.Count -eq 0) 'Install-RedTilde.ps1 has zero PowerShell parse errors'
    $newItemCmds = @($installerAst.FindAll({
        param($node)
        $node -is [Management.Automation.Language.CommandAst] -and
        $node.GetCommandName() -ieq 'New-Item'
    }, $true))
    Assert-True ($newItemCmds.Count -eq 0) 'Install-RedTilde.ps1 AST must not invoke New-Item'
    $hkcuStrings = @($installerAst.FindAll({
        param($node)
        ($node -is [Management.Automation.Language.StringConstantExpressionAst] -or
         $node -is [Management.Automation.Language.ExpandableStringExpressionAst]) -and
        $node.Value -match '(?i)HKCU:\\'
    }, $true))
    Assert-True ($hkcuStrings.Count -eq 0) 'Install-RedTilde.ps1 AST must not reference HKCU:\ provider paths'

    # 8. (b) Non-empty unowned install dir refusal vs empty/owned dir acceptance & in-place non-sweeping
    $isolatedReg = Join-Path $scratchRoot 'isolated-reg.json'
    $isolatedPrograms = Join-Path $scratchRoot 'isolated-programs'
    [void][IO.Directory]::CreateDirectory($isolatedPrograms)

    # (b1) Non-empty dir with user files only
    $unownedInstall = Join-Path $scratchRoot 'unowned-install'
    [void][IO.Directory]::CreateDirectory((Join-Path $unownedInstall 'sub'))
    [IO.File]::WriteAllText((Join-Path $unownedInstall 'notes.md'), 'user-notes')
    [IO.File]::WriteAllText((Join-Path $unownedInstall 'sub/keep.txt'), 'keep-me')
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $unownedInstall -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    } 'ownership' 'refuse-install-nonempty-unowned'
    Assert-True (([IO.File]::ReadAllText((Join-Path $unownedInstall 'notes.md'))) -ceq 'user-notes') 'unowned notes.md preserved'
    Assert-True (([IO.File]::ReadAllText((Join-Path $unownedInstall 'sub/keep.txt'))) -ceq 'keep-me') 'unowned sub/keep.txt preserved'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $unownedInstall 'RedTilde.exe'))) 'refused install did not copy RedTilde.exe'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $unownedInstall '.redtilde-install.json'))) 'refused install did not create marker'
    Assert-True (-not (Test-Path -LiteralPath $isolatedReg)) 'refused install did not write registry'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $isolatedPrograms 'RedTilde.lnk'))) 'refused install did not create shortcut'

    # (b2) Non-empty dir with RedTilde.exe + user file, no marker
    $unownedWithExe = Join-Path $scratchRoot 'unowned-with-exe'
    [void][IO.Directory]::CreateDirectory($unownedWithExe)
    [IO.File]::Copy($exeV1, (Join-Path $unownedWithExe 'RedTilde.exe'), $true)
    [IO.File]::WriteAllText((Join-Path $unownedWithExe 'user-doc.md'), 'keep-doc')
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $unownedWithExe -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    } 'ownership' 'refuse-install-exe-only-no-marker'
    Assert-True (([IO.File]::ReadAllText((Join-Path $unownedWithExe 'user-doc.md'))) -ceq 'keep-doc') 'user-doc.md preserved'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $unownedWithExe '.redtilde-install.json'))) 'no marker written on refused exe-only dir'

    # (b3) Regular file at target path
    $regularFileTarget = Join-Path $scratchRoot 'file-at-target'
    [IO.File]::WriteAllText($regularFileTarget, 'regular-file-data')
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $regularFileTarget -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    } 'regular file' 'refuse-install-over-regular-file'
    Assert-True (([IO.File]::ReadAllText($regularFileTarget)) -ceq 'regular-file-data') 'regular file at target untouched'

    # (b4) Corrupt and foreign-product markers
    $corruptMarkerDir = Join-Path $scratchRoot 'corrupt-marker-dir'
    [void][IO.Directory]::CreateDirectory($corruptMarkerDir)
    [IO.File]::WriteAllText((Join-Path $corruptMarkerDir '.redtilde-install.json'), '{bad json')
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $corruptMarkerDir -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    } 'ownership' 'refuse-install-corrupt-marker'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $corruptMarkerDir 'RedTilde.exe'))) 'corrupt marker install did not write RedTilde.exe'

    $foreignMarkerDir = Join-Path $scratchRoot 'foreign-marker-dir'
    [void][IO.Directory]::CreateDirectory($foreignMarkerDir)
    [IO.File]::WriteAllText((Join-Path $foreignMarkerDir '.redtilde-install.json'), '{"schemaVersion":1,"product":"OtherApp","files":["app.txt"]}')
    Assert-ThrowsContaining {
        & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $foreignMarkerDir -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    } 'ownership' 'refuse-install-foreign-marker'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $foreignMarkerDir 'RedTilde.exe'))) 'foreign marker install did not write RedTilde.exe'

    # (b5) Existing empty directory is allowed
    $emptyTargetDir = Join-Path $scratchRoot 'precreated-empty-dir'
    [void][IO.Directory]::CreateDirectory($emptyTargetDir)
    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $emptyTargetDir -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    Assert-True (Test-Path -LiteralPath (Join-Path $emptyTargetDir '.redtilde-install.json') -PathType Leaf) 'empty dir install succeeded and wrote marker'

    # (b6) Owned dir upgrade + in-place execution must preserve user files and NOT sweep them into .redtilde-install.json
    [IO.File]::WriteAllText((Join-Path $emptyTargetDir 'user-note.md'), 'personal-note')
    [IO.File]::WriteAllText((Join-Path $emptyTargetDir 'examples/my-note.md'), 'personal-example')
    & $installerScript -SourceDirectory $sourceV2 -InstallDirectory $emptyTargetDir -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    Assert-True (([IO.File]::ReadAllText((Join-Path $emptyTargetDir 'user-note.md'))) -ceq 'personal-note') 'owned upgrade preserved user-note.md'
    Assert-True (([IO.File]::ReadAllText((Join-Path $emptyTargetDir 'examples/my-note.md'))) -ceq 'personal-example') 'owned upgrade preserved examples/my-note.md'
    $markerAfterUpgrade = Get-Content -LiteralPath (Join-Path $emptyTargetDir '.redtilde-install.json') -Raw | ConvertFrom-Json
    Assert-True ($markerAfterUpgrade.files -notcontains 'user-note.md') 'upgrade did not sweep user-note.md into marker'
    Assert-True ($markerAfterUpgrade.files -notcontains 'examples\my-note.md') 'upgrade did not sweep examples\my-note.md into marker'

    & $installerScript -SourceDirectory $emptyTargetDir -InstallDirectory $emptyTargetDir -TestRegistryJsonPath $isolatedReg -TestProgramsMenuDirectory $isolatedPrograms -SkipShellNotify
    $markerAfterInPlace = Get-Content -LiteralPath (Join-Path $emptyTargetDir '.redtilde-install.json') -Raw | ConvertFrom-Json
    Assert-True ($markerAfterInPlace.files -notcontains 'user-note.md') 'in-place install did not sweep user-note.md into marker'
    Assert-True ($markerAfterInPlace.files -notcontains 'examples\my-note.md') 'in-place install did not sweep examples\my-note.md into marker'

    # 9. (c) Manifest-only -Uninstall preserves co-located user files, user subdirs, and non-empty root directory
    $sharedDir = Join-Path $scratchRoot 'shared-install'
    $sharedReg = Join-Path $scratchRoot 'shared-reg.json'
    $sharedPrograms = Join-Path $scratchRoot 'shared-programs'
    [void][IO.Directory]::CreateDirectory($sharedPrograms)
    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $sharedDir -RegisterFileAssociation -TestRegistryJsonPath $sharedReg -TestProgramsMenuDirectory $sharedPrograms -SkipShellNotify

    [IO.File]::WriteAllText((Join-Path $sharedDir 'user-notes.md'), 'root-user-note')
    [void][IO.Directory]::CreateDirectory((Join-Path $sharedDir 'my-docs'))
    [IO.File]::WriteAllText((Join-Path $sharedDir 'my-docs/journal.md'), 'journal-entry')
    [IO.File]::WriteAllText((Join-Path $sharedDir 'examples/user-example.md'), 'user-example')
    [IO.File]::WriteAllText((Join-Path $sharedDir 'res/custom-badge.txt'), 'custom-badge')

    $savedLoc2 = Get-Location
    try {
        Set-Location -LiteralPath (Join-Path $sharedDir 'my-docs')
        & $installerScript -Uninstall -InstallDirectory $sharedDir -TestRegistryJsonPath $sharedReg -TestProgramsMenuDirectory $sharedPrograms -SkipShellNotify
    } finally {
        Set-Location -LiteralPath $savedLoc2.ProviderPath
    }

    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'RedTilde.exe'))) 'manifest uninstall deleted RedTilde.exe'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'Install-RedTilde.ps1'))) 'manifest uninstall deleted Install-RedTilde.ps1'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir '.redtilde-install.json'))) 'manifest uninstall deleted .redtilde-install.json'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'res/RedTilde.png'))) 'manifest uninstall deleted res/RedTilde.png'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'examples/nested/sample.md'))) 'manifest uninstall deleted examples/nested/sample.md'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'Dependencies'))) 'manifest uninstall pruned empty Dependencies directory'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedDir 'examples/nested'))) 'manifest uninstall pruned empty examples/nested directory'
    Assert-True (Test-Path -LiteralPath $sharedDir -PathType Container) 'manifest uninstall preserved non-empty root directory'
    Assert-True (([IO.File]::ReadAllText((Join-Path $sharedDir 'user-notes.md'))) -ceq 'root-user-note') 'manifest uninstall preserved user-notes.md'
    Assert-True (([IO.File]::ReadAllText((Join-Path $sharedDir 'my-docs/journal.md'))) -ceq 'journal-entry') 'manifest uninstall preserved my-docs/journal.md'
    Assert-True (([IO.File]::ReadAllText((Join-Path $sharedDir 'examples/user-example.md'))) -ceq 'user-example') 'manifest uninstall preserved examples/user-example.md'
    Assert-True (([IO.File]::ReadAllText((Join-Path $sharedDir 'res/custom-badge.txt'))) -ceq 'custom-badge') 'manifest uninstall preserved res/custom-badge.txt'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $sharedPrograms 'RedTilde.lnk'))) 'manifest uninstall removed shortcut'

    # 10. (d) Tampered / pathological .redtilde-install.json entries fail closed on -Uninstall without deleting canary or target files
    $maliciousDir = Join-Path $scratchRoot 'malicious-manifest-dir'
    $maliciousReg = Join-Path $scratchRoot 'malicious-reg.json'
    $maliciousPrograms = Join-Path $scratchRoot 'malicious-programs'
    [void][IO.Directory]::CreateDirectory($maliciousPrograms)
    & $installerScript -SourceDirectory $sourceV1 -InstallDirectory $maliciousDir -TestRegistryJsonPath $maliciousReg -TestProgramsMenuDirectory $maliciousPrograms -SkipShellNotify

    $canaryOutside = Join-Path $scratchRoot 'canary-outside.txt'
    [IO.File]::WriteAllText($canaryOutside, 'canary-untouched')
    $juncTarget = Join-Path $scratchRoot 'junction-target'
    [void][IO.Directory]::CreateDirectory($juncTarget)
    $juncSecret = Join-Path $juncTarget 'secret.txt'
    [IO.File]::WriteAllText($juncSecret, 'secret-untouched')
    $maliciousJunction = Join-Path $maliciousDir 'link-sub'
    $null = New-Item -ItemType Junction -Path $maliciousJunction -Target $juncTarget
    $junctions.Add($maliciousJunction)
    $resUserKeep = Join-Path $maliciousDir 'res/user-keep.txt'
    [IO.File]::WriteAllText($resUserKeep, 'res-user-untouched')

    foreach ($badEntry in @(
        '..\canary-outside.txt',
        'sub\..\..\canary-outside.txt',
        "$canaryOutside",
        '\canary-outside.txt',
        'C:canary-outside.txt',
        '\\server\share\x.txt',
        '\\?\C:\canary-outside.txt',
        '.',
        '',
        'RedTilde.exe:ads',
        'RedTilde.exe.',
        'link-sub\secret.txt',
        'res'
    )) {
        $badPayload = [ordered]@{
            schemaVersion    = 1
            product          = 'RedTilde'
            version          = '0.1.42'
            installDirectory = $maliciousDir
            installedUtc     = [DateTime]::UtcNow.ToString('o')
            files            = @('RedTilde.exe', $badEntry)
        } | ConvertTo-Json -Depth 4
        [IO.File]::WriteAllText((Join-Path $maliciousDir '.redtilde-install.json'), $badPayload, [Text.UTF8Encoding]::new($false))
        Assert-ThrowsContaining {
            & $installerScript -Uninstall -InstallDirectory $maliciousDir -TestRegistryJsonPath $maliciousReg -TestProgramsMenuDirectory $maliciousPrograms -SkipShellNotify
        } 'Manifest' "refuse-tampered-manifest-$badEntry"
        Assert-True (([IO.File]::ReadAllText($canaryOutside)) -ceq 'canary-untouched') "canary outside survived ($badEntry)"
        Assert-True (([IO.File]::ReadAllText($juncSecret)) -ceq 'secret-untouched') "junction target secret survived ($badEntry)"
        Assert-True (([IO.File]::ReadAllText($resUserKeep)) -ceq 'res-user-untouched') "res/user-keep.txt survived ($badEntry)"
        Assert-True (Test-Path -LiteralPath (Join-Path $maliciousDir 'RedTilde.exe') -PathType Leaf) "RedTilde.exe not deleted before validation failure ($badEntry)"
    }

    # Check local staged output discovery with the real installer and isolated registry.
    $hostArchitecture = if ($env:PROCESSOR_ARCHITEW6432) { $env:PROCESSOR_ARCHITEW6432 } else { $env:PROCESSOR_ARCHITECTURE }
    $hostLabel = if ($hostArchitecture -ieq 'ARM64') { 'ARM64' } else { 'x64' }
    $localRepo = Join-Path $scratchRoot 'local-package-tree'
    $localInstallerFolder = Join-Path $localRepo 'Installer'
    [void][IO.Directory]::CreateDirectory($localInstallerFolder)
    $localInstaller = Join-Path $localInstallerFolder 'Install-RedTilde.ps1'
    Copy-Item -LiteralPath $installerScript -Destination $localInstaller
    $priorStage = Join-Path $localRepo ".build/package/RedTilde-0.1.42-$hostLabel-older"
    [void][IO.Directory]::CreateDirectory((Split-Path $priorStage -Parent))
    Copy-Item -LiteralPath $sourceV1 -Destination $priorStage -Recurse
    $stagedInstall = Join-Path $scratchRoot 'staged-install'
    & $localInstaller -InstallDirectory $stagedInstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    $installed = Join-Path $stagedInstall 'RedTilde.exe'
    Assert-True ((Get-FileHash -LiteralPath $installed).Hash -ceq (Get-FileHash -LiteralPath $exeV1).Hash) 'existing singular package staging is discoverable'
    $currentStage = Join-Path $localRepo ".build/packages/RedTilde-0.1.43-$hostLabel-current"
    [void][IO.Directory]::CreateDirectory((Split-Path $currentStage -Parent))
    Copy-Item -LiteralPath $sourceV2 -Destination $currentStage -Recurse
    & $localInstaller -InstallDirectory $stagedInstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify
    Assert-True ((Get-FileHash -LiteralPath $installed).Hash -ceq (Get-FileHash -LiteralPath $exeV2).Hash) 'canonical package staging takes precedence'
    & $localInstaller -Uninstall -InstallDirectory $stagedInstall -TestRegistryJsonPath $fakeRegistry -TestProgramsMenuDirectory $fakePrograms -SkipShellNotify

    # Check Windows PowerShell 5.1 parser compatibility of Install-RedTilde.ps1 if available.
    $winPs = if ($env:WINDIR) { Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe' } else { '' }
    if ($winPs -and (Test-Path -LiteralPath $winPs -PathType Leaf)) {
        $escapedInstaller = $installerScript.Replace("'", "''")
        $psCommand = '$e=$null; [void][Management.Automation.Language.Parser]::ParseFile(''' + $escapedInstaller + ''',[ref]$null,[ref]$e); if($e.Count){throw $e[0].Message}'
        $null = & $winPs -NoProfile -Command $psCommand
        Assert-True ($LASTEXITCODE -eq 0) 'Install-RedTilde.ps1 parses cleanly in Windows PowerShell 5.1'
    }

    Write-Host "PASS: per-user installer behavioral regressions (non-destructive HKCU/hive registry, AST guard, ownership refusal, manifest-only uninstall, CWD-safe uninstall, path & reparse refusals)."
    $global:LASTEXITCODE = 0
} finally {
    if ($null -ne $hiveRoot) {
        $hiveRoot.Dispose()
        $hiveRoot = $null
    }
    foreach ($j in $junctions) {
        if ($j -and [IO.Directory]::Exists($j)) {
            [IO.Directory]::Delete($j, $false)
        } else {
            try {
                $attrs = [IO.File]::GetAttributes($j)
                if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    [IO.Directory]::Delete($j, $false)
                }
            } catch {
            }
        }
    }
    Remove-Item -LiteralPath $scratchRoot -Recurse -Force -ErrorAction SilentlyContinue
}
