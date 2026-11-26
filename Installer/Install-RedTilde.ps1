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
    [Parameter(DontShow)][Microsoft.Win32.RegistryKey]$TestRegistryRootKey = $null,
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

function Get-RedTildeRegistryRootKey {
    if ($null -ne $TestRegistryRootKey) {
        return $TestRegistryRootKey
    }
    return [Microsoft.Win32.Registry]::CurrentUser
}

function ConvertTo-Win32RegistryValueName {
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Name)
    if ($Name -ieq '(Default)') {
        return ''
    }
    return $Name
}

function Test-RedTildeRegistryKeyExists {
    param([Parameter(Mandatory)][string]$SubKey)
    if ($TestRegistryJsonPath) {
        $store = Read-TestRegistryStore
        if ($store.ContainsKey($SubKey)) {
            return $true
        }
        $prefix = $SubKey + '\'
        foreach ($key in $store.Keys) {
            if ($key.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                return $true
            }
        }
        return $false
    }
    $rootKey = Get-RedTildeRegistryRootKey
    $sub = $rootKey.OpenSubKey($SubKey, $false)
    if ($null -ne $sub) {
        $sub.Dispose()
        return $true
    }
    return $false
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
    $rootKey = Get-RedTildeRegistryRootKey
    $sub = $rootKey.OpenSubKey($SubKey, $false)
    if ($null -eq $sub) {
        return $null
    }
    try {
        $win32Name = ConvertTo-Win32RegistryValueName -Name $Name
        return $sub.GetValue($win32Name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
    } finally {
        $sub.Dispose()
    }
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
        if ($Type -eq 'DWord') {
            $store[$SubKey][$Name] = [int]$Value
        } else {
            $store[$SubKey][$Name] = [string]$Value
        }
        Write-TestRegistryStore $store
        return
    }
    $rootKey = Get-RedTildeRegistryRootKey
    $sub = $rootKey.CreateSubKey($SubKey, $true)
    if ($null -eq $sub) {
        throw "Failed to create or open registry subkey '$SubKey'."
    }
    try {
        $win32Name = ConvertTo-Win32RegistryValueName -Name $Name
        if ($Type -eq 'DWord') {
            $sub.SetValue($win32Name, [int]$Value, [Microsoft.Win32.RegistryValueKind]::DWord)
        } else {
            $sub.SetValue($win32Name, [string]$Value, [Microsoft.Win32.RegistryValueKind]::String)
        }
    } finally {
        $sub.Dispose()
    }
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
    $rootKey = Get-RedTildeRegistryRootKey
    $sub = $rootKey.OpenSubKey($SubKey, $true)
    if ($null -ne $sub) {
        try {
            $win32Name = ConvertTo-Win32RegistryValueName -Name $Name
            $sub.DeleteValue($win32Name, $false)
        } finally {
            $sub.Dispose()
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
    $rootKey = Get-RedTildeRegistryRootKey
    $rootKey.DeleteSubKeyTree($SubKey, $false)
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

function Assert-RelativeDirectoryComponentsFreeOfReparsePoints {
    param(
        [Parameter(Mandatory)][string]$RootDirectory,
        [Parameter(Mandatory)][AllowEmptyString()][string]$RelativeDirectory
    )
    if ([string]::IsNullOrWhiteSpace($RelativeDirectory)) {
        return
    }
    $current = [IO.Path]::GetFullPath($RootDirectory).TrimEnd('\', '/')
    foreach ($seg in @($RelativeDirectory -split '[\\/]' | Where-Object { $_ })) {
        $current = Join-Path $current $seg
        try {
            $attrs = [IO.File]::GetAttributes($current)
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to traverse or create directory across a reparse point: '$current'."
            }
        } catch [IO.FileNotFoundException] {
            continue
        } catch [IO.DirectoryNotFoundException] {
            continue
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
    try {
        $destRootAttrs = [IO.File]::GetAttributes($DestinationRoot)
        if (($destRootAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Destination root cannot be a reparse point: '$DestinationRoot'."
        }
    } catch [IO.FileNotFoundException] {
    } catch [IO.DirectoryNotFoundException] {
    }
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
        Assert-RelativeDirectoryComponentsFreeOfReparsePoints -RootDirectory $DestinationRoot -RelativeDirectory $relDir
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
                try {
                    $destAttrs = [IO.File]::GetAttributes($destFile)
                    if (($destAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                        throw "Refusing to overwrite reparse point in destination: '$destFile'."
                    }
                    if (($destAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                        [IO.File]::SetAttributes($destFile, ($destAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                    }
                } catch [IO.FileNotFoundException] {
                } catch [IO.DirectoryNotFoundException] {
                }
                [IO.File]::Copy($entry.FullName, $destFile, $true)
                $copiedRelativeFiles.Add($relFile)
            }
        }
    }
    return @($copiedRelativeFiles.ToArray())
}

function Resolve-ValidatedManifestRelativeFilePath {
    param(
        [Parameter(Mandatory)][string]$RootDirectory,
        [Parameter(Mandatory)][AllowEmptyString()][string]$RelativePath
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath) -or (Test-RedTildeContainsControlCharacters $RelativePath)) {
        throw "Manifest contains an empty or invalid relative path: '$RelativePath'."
    }
    if ($RelativePath.IndexOf(':') -ge 0) {
        throw "Manifest relative path cannot contain ':' (drive or alternate data stream): '$RelativePath'."
    }
    if ($RelativePath.StartsWith('\') -or $RelativePath.StartsWith('/') -or
        $RelativePath.EndsWith('\') -or $RelativePath.EndsWith('/')) {
        throw "Manifest relative path cannot start or end with a path separator: '$RelativePath'."
    }
    if ([IO.Path]::IsPathRooted($RelativePath)) {
        throw "Manifest relative path cannot be rooted: '$RelativePath'."
    }

    $segments = @($RelativePath -split '[\\/]')
    if ($segments.Count -eq 0) {
        throw "Manifest relative path has no segments: '$RelativePath'."
    }
    $invalidFileNameChars = [IO.Path]::GetInvalidFileNameChars()
    foreach ($seg in $segments) {
        if ([string]::IsNullOrEmpty($seg)) {
            throw "Manifest relative path contains an empty segment: '$RelativePath'."
        }
        if ($seg -eq '.' -or $seg -eq '..') {
            throw "Manifest relative path cannot contain '.' or '..' traversal segments: '$RelativePath'."
        }
        if ($seg.TrimEnd(' ', '.') -ne $seg) {
            throw "Manifest relative path segment cannot end with a space or dot: '$RelativePath'."
        }
        if ($seg.IndexOfAny($invalidFileNameChars) -ge 0) {
            throw "Manifest relative path contains invalid filename characters: '$RelativePath'."
        }
    }

    $rootFull = [IO.Path]::GetFullPath($RootDirectory).TrimEnd('\', '/')
    $normalizedRel = ($segments -join '\')
    $full = [IO.Path]::GetFullPath((Join-Path $rootFull $normalizedRel))
    if (-not $full.StartsWith(($rootFull + '\'), [StringComparison]::OrdinalIgnoreCase)) {
        throw "Manifest relative path escapes installation root '$rootFull': '$RelativePath'."
    }

    $current = $rootFull
    foreach ($seg in $segments) {
        $current = Join-Path $current $seg
        try {
            $attrs = [IO.File]::GetAttributes($current)
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Manifest path cannot traverse or reference a reparse point: '$current'."
            }
        } catch [IO.FileNotFoundException] {
            continue
        } catch [IO.DirectoryNotFoundException] {
            continue
        }
    }

    if ([IO.Directory]::Exists($full)) {
        throw "Manifest file entry cannot refer to a directory: '$RelativePath'."
    }

    return $full
}

function Read-RedTildeInstallMarker {
    param(
        [Parameter(Mandatory)][string]$TargetDirectory,
        [switch]$RequirePresent
    )

    $markerPath = Join-Path $TargetDirectory $script:MarkerFileName
    if ([IO.Directory]::Exists($markerPath)) {
        throw "Install ownership marker path is a directory, not a file: '$markerPath'."
    }

    $markerExistsAsReparse = $false
    try {
        $attrs = [IO.File]::GetAttributes($markerPath)
        if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            $markerExistsAsReparse = $true
        }
    } catch [IO.FileNotFoundException] {
    } catch [IO.DirectoryNotFoundException] {
    }
    if ($markerExistsAsReparse) {
        throw "Install ownership marker cannot be a reparse point: '$markerPath'."
    }

    if (-not [IO.File]::Exists($markerPath)) {
        if ($RequirePresent) {
            throw "Refusing operation on '$TargetDirectory' because verified RedTilde installation ownership marker '$markerPath' is missing."
        }
        return $null
    }

    $parsed = $null
    try {
        $raw = [IO.File]::ReadAllText($markerPath, [Text.UTF8Encoding]::new($false))
        if ([string]::IsNullOrWhiteSpace($raw)) {
            throw 'Install marker file is empty.'
        }
        $parsed = $raw | ConvertFrom-Json
    } catch {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' is invalid: $($_.Exception.Message)"
    }

    if ($null -eq $parsed) {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' is empty."
    }

    $schemaProp = $parsed.PSObject.Properties['schemaVersion']
    if ($null -eq $schemaProp -or $null -eq $schemaProp.Value -or ($schemaProp.Value -is [bool]) -or ($schemaProp.Value -is [string])) {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' has an invalid schemaVersion."
    }
    $schemaVersion = 0
    try {
        $schemaVersion = [int]$schemaProp.Value
    } catch {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' has a non-numeric schemaVersion."
    }
    if ($schemaVersion -ne 1) {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' has unsupported schemaVersion '$schemaVersion'."
    }

    $productProp = $parsed.PSObject.Properties['product']
    if ($null -eq $productProp -or ([string]$productProp.Value -cne 'RedTilde')) {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' belongs to another product."
    }

    $filesProp = $parsed.PSObject.Properties['files']
    if ($null -eq $filesProp -or $null -eq $filesProp.Value -or ($filesProp.Value -is [string]) -or ($filesProp.Value -isnot [Collections.IEnumerable])) {
        throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' lacks a valid 'files' list."
    }

    $validatedFiles = [Collections.Generic.List[object]]::new()
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $filesProp.Value) {
        if ($entry -isnot [string]) {
            throw "Refusing operation on '$TargetDirectory' because install ownership marker '$markerPath' contains a non-string 'files' entry."
        }
        $validatedFull = Resolve-ValidatedManifestRelativeFilePath -RootDirectory $TargetDirectory -RelativePath $entry
        $normalizedRel = (($entry -split '[\\/]') -join '\')
        if ($seen.Add($normalizedRel)) {
            $validatedFiles.Add([pscustomobject]@{
                RelativePath = $normalizedRel
                FullPath     = $validatedFull
            })
        }
    }

    return [pscustomobject]@{
        MarkerPath = $markerPath
        Raw        = $parsed
        Files      = @($validatedFiles.ToArray())
    }
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
        if (($info.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to traverse or delete a reparse point: '$current'."
        }
        if (@($info.GetFileSystemInfos()).Count -gt 0) {
            break
        }
        if (($info.Attributes -band [IO.FileAttributes]::ReadOnly) -ne 0) {
            $info.Attributes = $info.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
        }
        $info.Delete($false)
        $current = (Split-Path $current -Parent).TrimEnd('\', '/')
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
    if ($null -ne $TestRegistryRootKey) {
        throw 'TestRegistryRootKey requires explicit InstallDirectory or registered InstallLocation.'
    }
    if ($env:LOCALAPPDATA) {
        return (Join-Path $env:LOCALAPPDATA 'Programs\RedTilde')
    }
    return ''
}

if ($null -ne $TestRegistryRootKey) {
    if (-not [string]::IsNullOrWhiteSpace($TestRegistryJsonPath)) {
        throw 'TestRegistryRootKey and TestRegistryJsonPath cannot be used together.'
    }
    if ([string]::IsNullOrWhiteSpace($TestProgramsMenuDirectory)) {
        throw 'TestRegistryRootKey requires TestProgramsMenuDirectory so test runs cannot modify the real Start Menu.'
    }
}

$hasExplicitInstallDir = $PSBoundParameters.ContainsKey('InstallDirectory')
if ($null -ne $TestRegistryRootKey -and (-not $Uninstall) -and (-not $hasExplicitInstallDir)) {
    throw 'TestRegistryRootKey requires an explicit InstallDirectory so test runs cannot touch the real user install directory.'
}
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
        $manifest = Read-RedTildeInstallMarker -TargetDirectory $targetDirectory -RequirePresent

        $cwd = (Get-Location).ProviderPath.TrimEnd('\', '/')
        if ($cwd.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase) -or
            $cwd.StartsWith(($targetDirectory + '\'), [StringComparison]::OrdinalIgnoreCase)) {
            $safeCwd = if ($env:TEMP -and [IO.Directory]::Exists($env:TEMP)) { $env:TEMP } else { Split-Path $targetDirectory -Parent }
            Set-Location -LiteralPath $safeCwd
        }

        foreach ($entry in $manifest.Files) {
            if ($entry.RelativePath.Equals('Install-RedTilde.ps1', [StringComparison]::OrdinalIgnoreCase) -or
                $entry.RelativePath.Equals($script:MarkerFileName, [StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            $fileFull = Resolve-ValidatedManifestRelativeFilePath -RootDirectory $targetDirectory -RelativePath $entry.RelativePath
            try {
                $attrs = [IO.File]::GetAttributes($fileFull)
                if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw "Refusing to delete a reparse point: '$fileFull'."
                }
                if ([IO.File]::Exists($fileFull)) {
                    if (($attrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                        [IO.File]::SetAttributes($fileFull, ($attrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                    }
                    [IO.File]::Delete($fileFull)
                    Remove-EmptyParentDirectoriesWithinRoot -RootPath $targetDirectory -StartDirectory (Split-Path $fileFull -Parent)
                }
            } catch [IO.FileNotFoundException] {
            } catch [IO.DirectoryNotFoundException] {
            }
        }

        $installedScriptCheck = Join-Path $targetDirectory 'Install-RedTilde.ps1'
        try {
            $scriptAttrs = [IO.File]::GetAttributes($installedScriptCheck)
            if (($scriptAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to delete a reparse point: '$installedScriptCheck'."
            }
            if ([IO.File]::Exists($installedScriptCheck)) {
                if (($scriptAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                    [IO.File]::SetAttributes($installedScriptCheck, ($scriptAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                }
                [IO.File]::Delete($installedScriptCheck)
            }
        } catch [IO.FileNotFoundException] {
        } catch [IO.DirectoryNotFoundException] {
        }

        $markerFile = $manifest.MarkerPath
        try {
            $markerAttrs = [IO.File]::GetAttributes($markerFile)
            if (($markerAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to delete a reparse point: '$markerFile'."
            }
            if ([IO.File]::Exists($markerFile)) {
                if (($markerAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                    [IO.File]::SetAttributes($markerFile, ($markerAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                }
                [IO.File]::Delete($markerFile)
            }
        } catch [IO.FileNotFoundException] {
        } catch [IO.DirectoryNotFoundException] {
        }

        $rootInfo = [IO.DirectoryInfo]::new($targetDirectory)
        if (($rootInfo.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to traverse or delete a reparse point: '$targetDirectory'."
        }
        if (@($rootInfo.GetFileSystemInfos()).Count -eq 0) {
            if (($rootInfo.Attributes -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                $rootInfo.Attributes = $rootInfo.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
            }
            $rootInfo.Delete($false)
        } else {
            Write-Host "Preserved non-empty directory '$targetDirectory' because unrelated files remain."
        }
    } else {
        # Refuse dangling reparse points even if Directory.Exists returned false.
        try {
            $attrs = [IO.File]::GetAttributes($targetDirectory)
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to traverse or delete a reparse point: '$targetDirectory'."
            }
        } catch [IO.FileNotFoundException] {
        } catch [IO.DirectoryNotFoundException] {
        }
    }

    if ($shortcutPath) {
        try {
            $lnkAttrs = [IO.File]::GetAttributes($shortcutPath)
            if (($lnkAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to delete a reparse point shortcut: '$shortcutPath'."
            }
            if ([IO.File]::Exists($shortcutPath)) {
                if (($lnkAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                    [IO.File]::SetAttributes($shortcutPath, ($lnkAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                }
                [IO.File]::Delete($shortcutPath)
            }
        } catch [IO.FileNotFoundException] {
        } catch [IO.DirectoryNotFoundException] {
        }
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

if ([IO.File]::Exists($targetDirectory)) {
    throw "InstallDirectory cannot point to an existing regular file: '$targetDirectory'."
}

$priorMarker = $null
if ([IO.Directory]::Exists($targetDirectory)) {
    $targetInfo = [IO.DirectoryInfo]::new($targetDirectory)
    if (@($targetInfo.GetFileSystemInfos()).Count -gt 0) {
        $priorMarker = Read-RedTildeInstallMarker -TargetDirectory $targetDirectory -RequirePresent
    } else {
        $priorMarker = Read-RedTildeInstallMarker -TargetDirectory $targetDirectory
    }
}

$sourceExe = Join-Path $resolvedSource 'RedTilde.exe'
$versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($sourceExe)
$displayVersion = "$($versionInfo.FileMajorPart).$($versionInfo.FileMinorPart).$($versionInfo.FileBuildPart)"

Assert-DirectoryTreeFreeOfReparsePoints -Path $resolvedSource

[void][IO.Directory]::CreateDirectory($targetDirectory)

$copiedFiles = [Collections.Generic.List[string]]::new()
if (-not $resolvedSource.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase)) {
    foreach ($relPath in @(Copy-DirectoryTreeExact -SourceRoot $resolvedSource -DestinationRoot $targetDirectory)) {
        if (-not [string]::IsNullOrWhiteSpace($relPath)) {
            $copiedFiles.Add($relPath)
        }
    }
} else {
    if ($null -eq $priorMarker) {
        throw "Refusing in-place operation on '$targetDirectory' because verified RedTilde installation ownership marker is missing."
    }
    foreach ($entry in $priorMarker.Files) {
        if ($entry.RelativePath.Equals($script:MarkerFileName, [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        if ([IO.File]::Exists($entry.FullPath)) {
            $copiedFiles.Add($entry.RelativePath)
        }
    }
}

$installedExe = Join-Path $targetDirectory 'RedTilde.exe'
if (-not [IO.File]::Exists($installedExe)) {
    throw "Installed executable is missing after copy: '$installedExe'."
}

$installedInstaller = Join-Path $targetDirectory 'Install-RedTilde.ps1'
if (-not ([IO.Path]::GetFullPath($PSCommandPath)).Equals($installedInstaller, [StringComparison]::OrdinalIgnoreCase)) {
    try {
        $instAttrs = [IO.File]::GetAttributes($installedInstaller)
        if (($instAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Installed script target cannot be a reparse point: '$installedInstaller'."
        }
        if (($instAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
            [IO.File]::SetAttributes($installedInstaller, ($instAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
        }
    } catch [IO.FileNotFoundException] {
    } catch [IO.DirectoryNotFoundException] {
    }
    [IO.File]::Copy($PSCommandPath, $installedInstaller, $true)
}
$copiedSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($f in $copiedFiles) {
    [void]$copiedSet.Add($f)
}
[void]$copiedSet.Add('Install-RedTilde.ps1')

# Clean stale managed files left behind by an earlier RedTilde install.
if ($null -ne $priorMarker) {
    foreach ($oldEntry in $priorMarker.Files) {
        if ($oldEntry.RelativePath.Equals($script:MarkerFileName, [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        if (-not $copiedSet.Contains($oldEntry.RelativePath)) {
            $oldFull = $oldEntry.FullPath
            try {
                $attrs = [IO.File]::GetAttributes($oldFull)
                if (($attrs -band [IO.FileAttributes]::ReparsePoint) -eq 0 -and [IO.File]::Exists($oldFull)) {
                    if (($attrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
                        [IO.File]::SetAttributes($oldFull, ($attrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
                    }
                    [IO.File]::Delete($oldFull)
                    Remove-EmptyParentDirectoriesWithinRoot -RootPath $targetDirectory -StartDirectory (Split-Path $oldFull -Parent)
                }
            } catch [IO.FileNotFoundException] {
            } catch [IO.DirectoryNotFoundException] {
            }
        }
    }
}

$markerPath = Join-Path $targetDirectory $script:MarkerFileName
try {
    $markerAttrs = [IO.File]::GetAttributes($markerPath)
    if (($markerAttrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Install marker target cannot be a reparse point: '$markerPath'."
    }
    if ([IO.Directory]::Exists($markerPath)) {
        throw "Install marker target cannot be a directory: '$markerPath'."
    }
    if (($markerAttrs -band [IO.FileAttributes]::ReadOnly) -ne 0) {
        [IO.File]::SetAttributes($markerPath, ($markerAttrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
    }
} catch [IO.FileNotFoundException] {
} catch [IO.DirectoryNotFoundException] {
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
