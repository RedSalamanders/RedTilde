<#
.SYNOPSIS
Installs or uninstalls the per-user portable RedTilde distribution without elevation.
.DESCRIPTION
Copies a staged or extracted RedTilde portable folder into %LOCALAPPDATA%\Programs\RedTilde (or -InstallDirectory),
creates a per-user Start Menu shortcut, registers HKCU Add/Remove Programs metadata, and optionally registers
per-user .md / .markdown file-association ProgIds under HKCU\Software\Classes.
-Uninstall removes installed program files, shortcuts, and per-user registry entries while preserving %LOCALAPPDATA%\RedTilde.
.PARAMETER SourceDirectory
Path to an extracted or staged portable RedTilde directory containing RedTilde.exe.
.PARAMETER InstallDirectory
Target per-user installation directory. Defaults to %LOCALAPPDATA%\Programs\RedTilde.
.PARAMETER RegisterFileAssociation
Registers per-user HKCU\Software\Classes OpenWithProgids and Application command entries for .md and .markdown.
.PARAMETER Uninstall
Removes the installed RedTilde files, Start Menu shortcut, file-association entries, and Uninstall registry key.
#>
[CmdletBinding()]
param(
    [string]$SourceDirectory = '',
    [string]$InstallDirectory = '',
    [switch]$RegisterFileAssociation,
    [switch]$Uninstall,
    [Parameter(DontShow)][string]$TestRegistryJsonPath = '',
    [Parameter(DontShow)][string]$TestProgramsMenuDirectory = '',
    [Parameter(DontShow)][switch]$SkipShellNotify
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:UninstallSubKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'
$script:ProgIdSubKey = 'Software\Classes\RedTilde.Markdown'
$script:AppSubKey = 'Software\Classes\Applications\RedTilde.exe'
$script:MarkerFileName = '.redtilde-install.json'

function Test-RedTildeContainsControlCharacters {
    param([string]$Value)
    foreach ($ch in $Value.ToCharArray()) {
        if ([int]$ch -lt 32) {
            return $true
        }
    }
    return $false
}

function Assert-NoReparseComponent {
    param(
        [Parameter(Mandatory)][string]$FullPath,
        [Parameter(Mandatory)][string]$Description
    )

    $trimmed = $FullPath.TrimEnd('\', '/')
    $root = [IO.Path]::GetPathRoot($trimmed)
    $relative = $trimmed.Substring($root.Length)
    $current = $root
    foreach ($segment in @($relative -split '[\\/]' | Where-Object { $_ })) {
        $current = Join-Path $current $segment
        try {
            $attributes = [IO.File]::GetAttributes($current)
            if (($attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Description cannot traverse a reparse point or symlink: '$current'."
            }
        } catch [IO.FileNotFoundException] {
            continue
        } catch [IO.DirectoryNotFoundException] {
            continue
        }
    }
}

function ConvertTo-NormalizedDirectory {
    param([string]$Candidate)
    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        return $null
    }
    $normalized = [IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        return $null
    }
    return $normalized
}

function Assert-SafePerUserDirectory {
    param([Parameter(Mandatory)][AllowEmptyString()][string]$TargetDirectory)

    if ([string]::IsNullOrWhiteSpace($TargetDirectory) -or (Test-RedTildeContainsControlCharacters $TargetDirectory)) {
        throw "InstallDirectory must be a non-empty valid drive path: '$TargetDirectory'."
    }
    if ($TargetDirectory -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])') {
        throw "InstallDirectory cannot use device-namespace syntax: '$TargetDirectory'."
    }
    if ($TargetDirectory -match '^[\\/]{2}') {
        throw "InstallDirectory cannot be a UNC or network path: '$TargetDirectory'."
    }
    if ($TargetDirectory -notmatch '^[A-Za-z]:[\\/]') {
        throw "InstallDirectory must be a drive-qualified absolute path (for example C:\Users\...\RedTilde): '$TargetDirectory'."
    }
    if ($TargetDirectory -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "InstallDirectory cannot contain parent '..' traversal segments: '$TargetDirectory'."
    }

    $full = [IO.Path]::GetFullPath($TargetDirectory).TrimEnd('\', '/')
    if ($full -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])' -or $full -match '^[\\/]{2}') {
        throw "InstallDirectory resolves outside ordinary local drive paths: '$full'."
    }
    $root = [IO.Path]::GetPathRoot($full).TrimEnd('\', '/')
    if ($full.Equals($root, [StringComparison]::OrdinalIgnoreCase)) {
        throw "InstallDirectory cannot be a drive root: '$full'."
    }

    if ($env:LOCALAPPDATA) {
        $userDataRoot = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'RedTilde')).TrimEnd('\', '/')
        if ($full.Equals($userDataRoot, [StringComparison]::OrdinalIgnoreCase) -or
            $full.StartsWith(($userDataRoot + '\'), [StringComparison]::OrdinalIgnoreCase) -or
            $userDataRoot.StartsWith(($full + '\'), [StringComparison]::OrdinalIgnoreCase)) {
            throw "InstallDirectory cannot be or overlap user settings and recovery storage at '$userDataRoot'."
        }
    }

    $exactCandidates = @(
        $env:USERPROFILE,
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Desktop' }),
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Documents' }),
        $env:LOCALAPPDATA,
        $(if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Programs' }),
        $env:APPDATA,
        ([Environment]::GetFolderPath('Desktop')),
        ([Environment]::GetFolderPath('MyDocuments')),
        ([Environment]::GetFolderPath('StartMenu')),
        ([Environment]::GetFolderPath('Programs'))
    )
    foreach ($candidate in $exactCandidates) {
        $blockedExact = ConvertTo-NormalizedDirectory $candidate
        if ($blockedExact -and $full.Equals($blockedExact, [StringComparison]::OrdinalIgnoreCase)) {
            throw "InstallDirectory cannot target shared user folder '$blockedExact': '$full'."
        }
    }

    $treeCandidates = @(
        $env:SystemRoot,
        $env:ProgramFiles,
        ${env:ProgramFiles(x86)},
        $env:ProgramData,
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Desktop' }),
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Documents' }),
        ([Environment]::GetFolderPath('Desktop')),
        ([Environment]::GetFolderPath('MyDocuments'))
    )
    foreach ($candidate in $treeCandidates) {
        $blockedTree = ConvertTo-NormalizedDirectory $candidate
        if ($blockedTree -and ($full.Equals($blockedTree, [StringComparison]::OrdinalIgnoreCase) -or
                $full.StartsWith(($blockedTree + '\'), [StringComparison]::OrdinalIgnoreCase))) {
            throw "InstallDirectory cannot target protected or user-document directory '$blockedTree': '$full'."
        }
    }

    Assert-NoReparseComponent -FullPath $full -Description 'InstallDirectory'
    return $full
}

function Assert-NoRunningRedTildeInDirectory {
    param([Parameter(Mandatory)][string]$TargetDirectory)

    $prefix = $TargetDirectory.TrimEnd('\', '/') + '\'
    foreach ($process in @(Get-Process -Name RedTilde -ErrorAction SilentlyContinue)) {
        if ($process.Path -and $process.Path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "RedTilde is currently running from '$TargetDirectory'. Close RedTilde before continuing."
        }
    }
}

function Get-RedTildeHostArchLabel {
    $arch = $env:PROCESSOR_ARCHITEW6432
    if ([string]::IsNullOrWhiteSpace($arch)) {
        $arch = $env:PROCESSOR_ARCHITECTURE
    }
    if ($arch -ieq 'ARM64') {
        return 'ARM64'
    }
    return 'x64'
}

function Resolve-RedTildeSourceDirectory {
    param([string]$Requested)

    if ($Requested) {
        if (Test-RedTildeContainsControlCharacters $Requested) {
            throw "SourceDirectory contains invalid characters: '$Requested'."
        }
        $full = [IO.Path]::GetFullPath($Requested).TrimEnd('\', '/')
        Assert-NoReparseComponent -FullPath $full -Description 'SourceDirectory'
        $exe = Join-Path $full 'RedTilde.exe'
        if (-not ([IO.File]::Exists($exe))) {
            throw "SourceDirectory does not contain RedTilde.exe: '$full'."
        }
        $exeAttrs = [IO.File]::GetAttributes($exe)
        if (($exeAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Source executable cannot be a reparse point: '$exe'."
        }
        return $full
    }

    $candidates = [Collections.Generic.List[string]]::new()
    $candidates.Add($PSScriptRoot)
    $parent = Split-Path $PSScriptRoot -Parent
    if ($parent) {
        $candidates.Add($parent)
        $hostArch = Get-RedTildeHostArchLabel
        foreach ($packagePath in '.build\packages', '.build\package') {
            $packageRoot = Join-Path $parent $packagePath
            if (-not [IO.Directory]::Exists($packageRoot)) { continue }
            Assert-NoReparseComponent -FullPath $packageRoot -Description 'Package staging directory'
            foreach ($dir in @(Get-ChildItem -LiteralPath $packageRoot -Directory |
                    Where-Object { $_.Name -like "RedTilde-*-$hostArch-*" -and (($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) } |
                    Sort-Object LastWriteTimeUtc -Descending)) {
                $candidates.Add($dir.FullName)
            }
        }
    }

    foreach ($candidate in $candidates) {
        $normalized = [IO.Path]::GetFullPath($candidate).TrimEnd('\', '/')
        $exe = Join-Path $normalized 'RedTilde.exe'
        if ([IO.File]::Exists($exe)) {
            Assert-NoReparseComponent -FullPath $normalized -Description 'SourceDirectory'
            return $normalized
        }
    }

    throw 'Could not locate a staged or extracted RedTilde folder containing RedTilde.exe for this host architecture. Pass -SourceDirectory.'
}

function Read-TestRegistryStore {
    if (-not [IO.File]::Exists($TestRegistryJsonPath)) {
        return @{}
    }
    $raw = [IO.File]::ReadAllText($TestRegistryJsonPath, [Text.UTF8Encoding]::new($false))
    if ([string]::IsNullOrWhiteSpace($raw)) {
        return @{}
    }
    $parsed = $raw | ConvertFrom-Json
    $map = @{}
    if ($null -ne $parsed) {
        foreach ($prop in $parsed.PSObject.Properties) {
            $values = @{}
            if ($null -ne $prop.Value) {
                foreach ($valProp in $prop.Value.PSObject.Properties) {
                    $values[$valProp.Name] = $valProp.Value
                }
            }
            $map[$prop.Name] = $values
        }
    }
    return $map
}

function Write-TestRegistryStore {
    param([Parameter(Mandatory)][hashtable]$Store)
    $parent = Split-Path $TestRegistryJsonPath -Parent
    if ($parent -and -not [IO.Directory]::Exists($parent)) {
        [void][IO.Directory]::CreateDirectory($parent)
    }
    $json = $Store | ConvertTo-Json -Depth 6
    [IO.File]::WriteAllText($TestRegistryJsonPath, $json, [Text.UTF8Encoding]::new($false))
}

function Test-RedTildeRegistryKeyExists {
    param([Parameter(Mandatory)][string]$SubKey)
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        return $store.ContainsKey($SubKey)
    }
    return Test-Path -LiteralPath ("HKCU:\$SubKey")
}

function Get-RedTildeRegistryValue {
    param(
        [Parameter(Mandatory)][string]$SubKey,
        [Parameter(Mandatory)][string]$Name
    )
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        if ($store.ContainsKey($SubKey) -and $store[$SubKey].ContainsKey($Name)) {
            return $store[$SubKey][$Name]
        }
        return $null
    }
    $regPath = "HKCU:\$SubKey"
    if (-not (Test-Path -LiteralPath $regPath)) {
        return $null
    }
    $item = Get-ItemProperty -LiteralPath $regPath -Name $Name -ErrorAction SilentlyContinue
    if ($null -ne $item) {
        return $item.$Name
    }
    return $null
}

function Set-RedTildeRegistryValue {
    param(
        [Parameter(Mandatory)][string]$SubKey,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyString()]$Value,
        [ValidateSet('String', 'DWord')][string]$Type = 'String'
    )
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        if (-not $store.ContainsKey($SubKey)) {
            $store[$SubKey] = @{}
        }
        $store[$SubKey][$Name] = $Value
        Write-TestRegistryStore $store
        return
    }
    $regPath = "HKCU:\$SubKey"
    [void](New-Item -Path $regPath -Force)
    Set-ItemProperty -LiteralPath $regPath -Name $Name -Value $Value -Type $Type
}

function Remove-RedTildeRegistryProperty {
    param(
        [Parameter(Mandatory)][string]$SubKey,
        [Parameter(Mandatory)][string]$Name
    )
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        if ($store.ContainsKey($SubKey) -and $store[$SubKey].ContainsKey($Name)) {
            $store[$SubKey].Remove($Name)
            Write-TestRegistryStore $store
        }
        return
    }
    $regPath = "HKCU:\$SubKey"
    if (Test-Path -LiteralPath $regPath) {
        $item = Get-ItemProperty -LiteralPath $regPath -Name $Name -ErrorAction SilentlyContinue
        if ($null -ne $item) {
            Remove-ItemProperty -LiteralPath $regPath -Name $Name -Force
        }
    }
}

function Remove-RedTildeRegistryTree {
    param([Parameter(Mandatory)][string]$SubKey)
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        $prefix = $SubKey + '\'
        $keysToRemove = @($store.Keys | Where-Object {
                $_.Equals($SubKey, [StringComparison]::OrdinalIgnoreCase) -or
                $_.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)
            })
        if ($keysToRemove.Count -gt 0) {
            foreach ($k in $keysToRemove) {
                $store.Remove($k)
            }
            Write-TestRegistryStore $store
        }
        return
    }
    $regPath = "HKCU:\$SubKey"
    if (Test-Path -LiteralPath $regPath) {
        Remove-Item -LiteralPath $regPath -Recurse -Force
    }
}

function Invoke-RedTildeAssocChangedNotification {
    if ($SkipShellNotify) {
        return
    }
    if (-not ('RedTildeNativeShellNotify' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class RedTildeNativeShellNotify {
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    public static extern void SHChangeNotify(int wEventId, uint uFlags, IntPtr dwItem1, IntPtr dwItem2);
}
'@
    }
    [RedTildeNativeShellNotify]::SHChangeNotify(0x08000000, 0x0000, [IntPtr]::Zero, [IntPtr]::Zero)
}

function Remove-PerUserFileAssociation {
    $hadAssoc = (Test-RedTildeRegistryKeyExists $script:ProgIdSubKey) -or (Test-RedTildeRegistryKeyExists $script:AppSubKey)
    Remove-RedTildeRegistryTree $script:ProgIdSubKey
    Remove-RedTildeRegistryTree $script:AppSubKey
    foreach ($extension in @('.md', '.markdown')) {
        $openWithSubKey = "Software\Classes\$extension\OpenWithProgids"
        if ($null -ne (Get-RedTildeRegistryValue -SubKey $openWithSubKey -Name 'RedTilde.Markdown')) {
            Remove-RedTildeRegistryProperty -SubKey $openWithSubKey -Name 'RedTilde.Markdown'
            $hadAssoc = $true
        }
    }
    if ($hadAssoc) {
        Invoke-RedTildeAssocChangedNotification
    }
}

function Set-PerUserFileAssociation {
    param([Parameter(Mandatory)][string]$InstalledExe)

    Set-RedTildeRegistryValue -SubKey $script:ProgIdSubKey -Name '(Default)' -Value 'Markdown Document'
    Set-RedTildeRegistryValue -SubKey ($script:ProgIdSubKey + '\DefaultIcon') -Name '(Default)' -Value "$InstalledExe,0"
    Set-RedTildeRegistryValue -SubKey ($script:ProgIdSubKey + '\shell\open\command') -Name '(Default)' -Value "`"$InstalledExe`" `"%1`""

    foreach ($extension in @('.md', '.markdown')) {
        Set-RedTildeRegistryValue -SubKey "Software\Classes\$extension\OpenWithProgids" -Name 'RedTilde.Markdown' -Value ''
    }

    Set-RedTildeRegistryValue -SubKey ($script:AppSubKey + '\SupportedTypes') -Name '.md' -Value ''
    Set-RedTildeRegistryValue -SubKey ($script:AppSubKey + '\SupportedTypes') -Name '.markdown' -Value ''
    Set-RedTildeRegistryValue -SubKey ($script:AppSubKey + '\shell\open\command') -Name '(Default)' -Value "`"$InstalledExe`" `"%1`""

    Invoke-RedTildeAssocChangedNotification
}

function Assert-DirectoryTreeFreeOfReparsePoints {
    param([Parameter(Mandatory)][string]$Path)
    if (-not [IO.Directory]::Exists($Path)) {
        return
    }
    $dirInfo = [IO.DirectoryInfo]::new($Path)
    if (($dirInfo.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Directory cannot be a reparse point: '$Path'."
    }
    foreach ($entry in $dirInfo.GetFileSystemInfos()) {
        if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Directory cannot contain a reparse point or symlink: '$($entry.FullName)'."
        }
        if ($entry -is [IO.DirectoryInfo]) {
            Assert-DirectoryTreeFreeOfReparsePoints -Path $entry.FullName
        }
    }
}

function Copy-DirectoryTreeExact {
    param(
        [Parameter(Mandatory)][string]$SourceRoot,
        [Parameter(Mandatory)][string]$DestinationRoot
    )

    $copiedRelativeFiles = [Collections.Generic.List[string]]::new()
    $sourceInfo = [IO.DirectoryInfo]::new($SourceRoot)
    if (-not [IO.Directory]::Exists($DestinationRoot)) {
        [void][IO.Directory]::CreateDirectory($DestinationRoot)
    }

    $stack = [Collections.Generic.Stack[string]]::new()
    $stack.Push($sourceInfo.FullName)
    while ($stack.Count -gt 0) {
        $currentSourceDir = $stack.Pop()
        $relDir = ''
        if ($currentSourceDir.Length -gt $sourceInfo.FullName.Length) {
            $relDir = $currentSourceDir.Substring($sourceInfo.FullName.Length).TrimStart('\', '/')
        }
        $currentDestDir = if ($relDir) { Join-Path $DestinationRoot $relDir } else { $DestinationRoot }
        if (-not [IO.Directory]::Exists($currentDestDir)) {
            [void][IO.Directory]::CreateDirectory($currentDestDir)
        }

        $dirInfo = [IO.DirectoryInfo]::new($currentSourceDir)
        foreach ($entry in $dirInfo.GetFileSystemInfos()) {
            if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to copy reparse point from source: '$($entry.FullName)'."
            }
            if ($entry -is [IO.DirectoryInfo]) {
                $stack.Push($entry.FullName)
            } else {
                $relFile = if ($relDir) { Join-Path $relDir $entry.Name } else { $entry.Name }
                $destFile = Join-Path $DestinationRoot $relFile
                if ([IO.File]::Exists($destFile)) {
                    $destAttrs = [IO.File]::GetAttributes($destFile)
                    if (($destAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                        throw "Refusing to overwrite reparse point in destination: '$destFile'."
                    }
                    if (($destAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                        [IO.File]::SetAttributes($destFile, ($destAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                    }
                }
                [IO.File]::Copy($entry.FullName, $destFile, $true)
                $copiedRelativeFiles.Add($relFile)
            }
        }
    }
    return @($copiedRelativeFiles.ToArray())
}

function Remove-DirectoryNoReparse {
    param([Parameter(Mandatory)][string]$Path)

    if (-not [IO.Directory]::Exists($Path)) {
        try {
            $attrs = [IO.File]::GetAttributes($Path)
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to traverse or delete a reparse point: '$Path'."
            }
        } catch [IO.FileNotFoundException] {
            return
        } catch [IO.DirectoryNotFoundException] {
            return
        }
        return
    }

    Assert-DirectoryTreeFreeOfReparsePoints -Path $Path

    $removeInner = {
        param([string]$CurrentPath, [scriptblock]$Self)
        $dirInfo = [IO.DirectoryInfo]::new($CurrentPath)
        foreach ($entry in $dirInfo.GetFileSystemInfos()) {
            if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to traverse or delete a reparse point: '$($entry.FullName)'."
            }
            if ($entry -is [IO.DirectoryInfo]) {
                & $Self $entry.FullName $Self
            } else {
                if (($entry.Attributes -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                    $entry.Attributes = $entry.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
                }
                $entry.Delete()
            }
        }
        if (($dirInfo.Attributes -band [IO.FileAttributes]::ReadOnly) -ne 0) {
            $dirInfo.Attributes = $dirInfo.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
        }
        $dirInfo.Delete($false)
    }
    & $removeInner $Path $removeInner
}

function Remove-EmptyParentDirectoriesWithinRoot {
    param(
        [Parameter(Mandatory)][string]$RootPath,
        [Parameter(Mandatory)][string]$StartDirectory
    )
    $rootFull = [IO.Path]::GetFullPath($RootPath).TrimEnd('\', '/')
    $current = [IO.Path]::GetFullPath($StartDirectory).TrimEnd('\', '/')
    while ($current.Length -gt $rootFull.Length -and $current.StartsWith(($rootFull + '\'), [StringComparison]::OrdinalIgnoreCase)) {
        if (-not [IO.Directory]::Exists($current)) {
            break
        }
        $info = [IO.DirectoryInfo]::new($current)
        if ($info.GetFileSystemInfos().Count -gt 0) {
            break
        }
        $info.Delete($false)
        $current = Split-Path $current -Parent
    }
}

function Resolve-EffectiveInstallDirectoryForUninstall {
    param([Parameter(Mandatory)][bool]$HasExplicitInstallDirectory)
    if ($HasExplicitInstallDirectory) {
        return $InstallDirectory
    }
    $registered = Get-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'InstallLocation'
    if (-not [string]::IsNullOrWhiteSpace([string]$registered)) {
        return [string]$registered
    }
    if ($env:LOCALAPPDATA) {
        return (Join-Path $env:LOCALAPPDATA 'Programs\RedTilde')
    }
    return ''
}

$hasExplicitInstallDir = $PSBoundParameters.ContainsKey('InstallDirectory')
$effectiveInstallDirectory = if ($Uninstall) {
    Resolve-EffectiveInstallDirectoryForUninstall -HasExplicitInstallDirectory $hasExplicitInstallDir
} elseif ($hasExplicitInstallDir) {
    $InstallDirectory
} elseif ($env:LOCALAPPDATA) {
    Join-Path $env:LOCALAPPDATA 'Programs\RedTilde'
} else {
    ''
}

$targetDirectory = Assert-SafePerUserDirectory -TargetDirectory $effectiveInstallDirectory
if (-not [string]::IsNullOrWhiteSpace($TestRegistryJsonPath)) {
    $sandboxRoot = [IO.Path]::GetFullPath((Split-Path $TestRegistryJsonPath -Parent)).TrimEnd('\', '/')
    if (-not $targetDirectory.StartsWith(($sandboxRoot + '\'), [StringComparison]::OrdinalIgnoreCase)) {
        throw "Test-mode InstallDirectory '$targetDirectory' must remain strictly inside test sandbox '$sandboxRoot'."
    }
}
Assert-NoRunningRedTildeInDirectory -TargetDirectory $targetDirectory

$programsMenu = if ($TestProgramsMenuDirectory) {
    [IO.Path]::GetFullPath($TestProgramsMenuDirectory)
} else {
    [Environment]::GetFolderPath('Programs')
}
$shortcutPath = if ($programsMenu) { Join-Path $programsMenu 'RedTilde.lnk' } else { '' }

if ($Uninstall) {
    if ([IO.Directory]::Exists($targetDirectory)) {
        Assert-DirectoryTreeFreeOfReparsePoints -Path $targetDirectory
        $installedExeCheck = Join-Path $targetDirectory 'RedTilde.exe'
        $markerCheck = Join-Path $targetDirectory $script:MarkerFileName
        $installedScriptCheck = Join-Path $targetDirectory 'Install-RedTilde.ps1'
        $registeredLocation = [string](Get-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'InstallLocation')
        $matchesRegisteredLocation = (-not [string]::IsNullOrWhiteSpace($registeredLocation)) -and
            ([IO.Path]::GetFullPath($registeredLocation).TrimEnd('\', '/')).Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase)

        $hasOwnership = [IO.File]::Exists($installedExeCheck) -and (
            [IO.File]::Exists($markerCheck) -or
            [IO.File]::Exists($installedScriptCheck) -or
            $matchesRegisteredLocation
        )
        if (-not $hasOwnership) {
            throw "Refusing to uninstall '$targetDirectory' because verified RedTilde installation ownership is absent."
        }

        $cwd = (Get-Location).ProviderPath.TrimEnd('\', '/')
        if ($cwd.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase) -or
            $cwd.StartsWith(($targetDirectory + '\'), [StringComparison]::OrdinalIgnoreCase)) {
            $safeCwd = if ($env:TEMP -and [IO.Directory]::Exists($env:TEMP)) { $env:TEMP } else { Split-Path $targetDirectory -Parent }
            Set-Location -LiteralPath $safeCwd
        }

        Remove-DirectoryNoReparse -Path $targetDirectory
    } else {
        # Refuse dangling reparse points even if Directory.Exists returned false.
        Remove-DirectoryNoReparse -Path $targetDirectory
    }

    if ($shortcutPath -and (Test-Path -LiteralPath $shortcutPath -PathType Leaf)) {
        Remove-Item -LiteralPath $shortcutPath -Force
    }
    Remove-PerUserFileAssociation
    Remove-RedTildeRegistryTree -SubKey $script:UninstallSubKey

    Write-Host "RedTilde uninstalled from '$targetDirectory'."
    if ($env:LOCALAPPDATA) {
        $preservedData = Join-Path $env:LOCALAPPDATA 'RedTilde'
        Write-Host "User settings, recovery snapshots, and crash reports under '$preservedData' were preserved."
    }
    return
}

$resolvedSource = Resolve-RedTildeSourceDirectory -Requested $SourceDirectory
if (-not $resolvedSource.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase)) {
    if ($targetDirectory.StartsWith(($resolvedSource + '\'), [StringComparison]::OrdinalIgnoreCase) -or
        $resolvedSource.StartsWith(($targetDirectory + '\'), [StringComparison]::OrdinalIgnoreCase)) {
        throw "SourceDirectory ('$resolvedSource') and InstallDirectory ('$targetDirectory') cannot overlap as parent and child."
    }
}

$sourceExe = Join-Path $resolvedSource 'RedTilde.exe'
$versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($sourceExe)
$displayVersion = "$($versionInfo.FileMajorPart).$($versionInfo.FileMinorPart).$($versionInfo.FileBuildPart)"

$markerPath = Join-Path $targetDirectory $script:MarkerFileName
$previousManagedFiles = [Collections.Generic.List[string]]::new()
if ([IO.File]::Exists($markerPath)) {
    try {
        $priorMarker = [IO.File]::ReadAllText($markerPath, [Text.UTF8Encoding]::new($false)) | ConvertFrom-Json
        if ($null -ne $priorMarker -and $priorMarker.PSObject.Properties['files'] -and $null -ne $priorMarker.files) {
            foreach ($rel in @($priorMarker.files)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$rel)) {
                    $previousManagedFiles.Add([string]$rel)
                }
            }
        }
    } catch {
        Write-Warning "Could not parse existing install marker '$markerPath': $($_.Exception.Message)"
    }
}

Assert-DirectoryTreeFreeOfReparsePoints -Path $resolvedSource
if ([IO.Directory]::Exists($targetDirectory)) {
    Assert-DirectoryTreeFreeOfReparsePoints -Path $targetDirectory
}

[void][IO.Directory]::CreateDirectory($targetDirectory)

$copiedFiles = [Collections.Generic.List[string]]::new()
if (-not $resolvedSource.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase)) {
    foreach ($relPath in @(Copy-DirectoryTreeExact -SourceRoot $resolvedSource -DestinationRoot $targetDirectory)) {
        if (-not [string]::IsNullOrWhiteSpace($relPath)) {
            $copiedFiles.Add($relPath)
        }
    }
} else {
    $stack = [Collections.Generic.Stack[string]]::new()
    $stack.Push($targetDirectory)
    while ($stack.Count -gt 0) {
        $curr = $stack.Pop()
        $relDir = ''
        if ($curr.Length -gt $targetDirectory.Length) {
            $relDir = $curr.Substring($targetDirectory.Length).TrimStart('\', '/')
        }
        foreach ($entry in ([IO.DirectoryInfo]::new($curr)).GetFileSystemInfos()) {
            if ($entry -is [IO.DirectoryInfo]) {
                $stack.Push($entry.FullName)
            } elseif (-not $entry.Name.Equals($script:MarkerFileName, [StringComparison]::OrdinalIgnoreCase)) {
                $copiedFiles.Add($(if ($relDir) { Join-Path $relDir $entry.Name } else { $entry.Name }))
            }
        }
    }
}

$installedExe = Join-Path $targetDirectory 'RedTilde.exe'
if (-not [IO.File]::Exists($installedExe)) {
    throw "Installed executable is missing after copy: '$installedExe'."
}

$installedInstaller = Join-Path $targetDirectory 'Install-RedTilde.ps1'
if (-not ([IO.Path]::GetFullPath($PSCommandPath)).Equals($installedInstaller, [StringComparison]::OrdinalIgnoreCase)) {
    if ([IO.File]::Exists($installedInstaller)) {
        $instAttrs = [IO.File]::GetAttributes($installedInstaller)
        if (($instAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Installed script target cannot be a reparse point: '$installedInstaller'."
        }
        if (($instAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
            [IO.File]::SetAttributes($installedInstaller, ($instAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
        }
    }
    [IO.File]::Copy($PSCommandPath, $installedInstaller, $true)
}
$copiedSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($f in $copiedFiles) {
    [void]$copiedSet.Add($f)
}
[void]$copiedSet.Add('Install-RedTilde.ps1')

# Clean stale managed files left behind by an earlier RedTilde install.
foreach ($oldRel in $previousManagedFiles) {
    if ($oldRel -match '(^|[\\/])\.\.([\\/]|$)' -or [IO.Path]::IsPathRooted($oldRel)) {
        continue
    }
    if (-not $copiedSet.Contains($oldRel)) {
        $oldFull = Join-Path $targetDirectory $oldRel
        if ([IO.File]::Exists($oldFull)) {
            $attrs = [IO.File]::GetAttributes($oldFull)
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -eq 0) {
                if (($attrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                    [IO.File]::SetAttributes($oldFull, ($attrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                }
                [IO.File]::Delete($oldFull)
                Remove-EmptyParentDirectoriesWithinRoot -RootPath $targetDirectory -StartDirectory (Split-Path $oldFull -Parent)
            }
        }
    }
}

$markerPayload = [ordered]@{
    schemaVersion    = 1
    product          = 'RedTilde'
    version          = $displayVersion
    installDirectory = $targetDirectory
    installedUtc     = [DateTime]::UtcNow.ToString('o')
    files            = @($copiedSet | Sort-Object)
} | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText($markerPath, $markerPayload, [Text.UTF8Encoding]::new($false))

if ($shortcutPath) {
    $shortcutParent = Split-Path $shortcutPath -Parent
    if ($shortcutParent) {
        [void][IO.Directory]::CreateDirectory($shortcutParent)
    }
    $shell = New-Object -ComObject WScript.Shell
    try {
        $shortcut = $shell.CreateShortcut($shortcutPath)
        $shortcut.TargetPath = $installedExe
        $shortcut.WorkingDirectory = $targetDirectory
        $shortcut.IconLocation = "$installedExe,0"
        $shortcut.Description = 'RedTilde Markdown editor'
        $shortcut.Save()
    } finally {
        [void][Runtime.InteropServices.Marshal]::ReleaseComObject($shell)
    }
}

$winPsPath = if ($env:WINDIR) { Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe' } else { '' }
$psExecutableName = if ($PSVersionTable.PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
$psExecutablePath = Join-Path $PSHOME $psExecutableName
if ($winPsPath -and [IO.File]::Exists($winPsPath)) {
    $psExecutablePath = $winPsPath
}
$uninstallCommand = "`"$psExecutablePath`" -NoProfile -ExecutionPolicy Bypass -File `"$installedInstaller`" -InstallDirectory `"$targetDirectory`" -Uninstall"

Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'DisplayName' -Value 'RedTilde'
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'DisplayVersion' -Value $displayVersion
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'Publisher' -Value 'RedSalamanders'
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'InstallLocation' -Value $targetDirectory
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'DisplayIcon' -Value "$installedExe,0"
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'NoModify' -Value 1 -Type DWord
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'NoRepair' -Value 1 -Type DWord
Set-RedTildeRegistryValue -SubKey $script:UninstallSubKey -Name 'UninstallString' -Value $uninstallCommand

$hasExistingAssociations = (Test-RedTildeRegistryKeyExists $script:ProgIdSubKey) -or (Test-RedTildeRegistryKeyExists $script:AppSubKey)
$updatedAssociations = $false
if ($RegisterFileAssociation -or $hasExistingAssociations) {
    Set-PerUserFileAssociation -InstalledExe $installedExe
    $updatedAssociations = $true
}

Write-Host "RedTilde $displayVersion installed to '$targetDirectory'."
if ($shortcutPath) {
    Write-Host "Start Menu shortcut: '$shortcutPath'."
}
if ($updatedAssociations) {
    Write-Host 'Per-user .md and .markdown file associations registered under HKCU\Software\Classes.'
}
