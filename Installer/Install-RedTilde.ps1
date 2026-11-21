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
Registers per-user HKCU\Software\Classes OpenWithProgids entries for .md and .markdown.
.PARAMETER Uninstall
Removes the installed RedTilde files, Start Menu shortcut, file-association entries, and Uninstall registry key.
#>
[CmdletBinding()]
param(
    [string]$SourceDirectory = '',
    [string]$InstallDirectory = $(if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Programs/RedTilde' } else { '' }),
    [switch]$RegisterFileAssociation,
    [switch]$Uninstall
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-SafePerUserDirectory {
    param([Parameter(Mandatory)][string]$TargetDirectory)

    if (-not [IO.Path]::IsPathRooted($TargetDirectory)) {
        throw "InstallDirectory must be an absolute path: '$TargetDirectory'."
    }
    $full = [IO.Path]::GetFullPath($TargetDirectory).TrimEnd('\', '/')
    $root = [IO.Path]::GetPathRoot($full).TrimEnd('\', '/')
    if ($full -ceq $root) {
        throw "InstallDirectory cannot be a drive root: '$full'."
    }

    if ($env:LOCALAPPDATA) {
        $userDataRoot = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'RedTilde')).TrimEnd('\', '/')
        if ($full.Equals($userDataRoot, [StringComparison]::OrdinalIgnoreCase) -or
            $full.StartsWith($userDataRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "InstallDirectory cannot overlap user settings and recovery storage at '$userDataRoot'."
        }
    }

    $driveRoot = [IO.Path]::GetPathRoot($full)
    $relative = $full.Substring($driveRoot.Length)
    $current = $driveRoot
    foreach ($segment in @($relative -split '[\\/]' | Where-Object { $_ })) {
        $current = Join-Path $current $segment
        if (-not (Test-Path -LiteralPath $current)) {
            continue
        }
        $item = Get-Item -LiteralPath $current -Force -ErrorAction Stop
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "InstallDirectory path cannot traverse a reparse point: '$current'."
        }
    }
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

function Resolve-RedTildeSourceDirectory {
    param([string]$Requested)

    if ($Requested) {
        $full = [IO.Path]::GetFullPath($Requested)
        if (-not (Test-Path -LiteralPath (Join-Path $full 'RedTilde.exe') -PathType Leaf)) {
            throw "SourceDirectory does not contain RedTilde.exe: '$full'."
        }
        return $full
    }

    $candidates = [Collections.Generic.List[string]]::new()
    $candidates.Add($PSScriptRoot)
    $parent = Split-Path $PSScriptRoot -Parent
    if ($parent) {
        $candidates.Add($parent)
        $packageRoot = Join-Path $parent '.build/package'
        if (Test-Path -LiteralPath $packageRoot -PathType Container) {
            foreach ($dir in @(Get-ChildItem -LiteralPath $packageRoot -Directory |
                    Where-Object { $_.Name -like 'RedTilde-*' } |
                    Sort-Object LastWriteTimeUtc -Descending)) {
                $candidates.Add($dir.FullName)
            }
        }
    }

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'RedTilde.exe') -PathType Leaf) {
            return [IO.Path]::GetFullPath($candidate)
        }
    }

    throw 'Could not locate a staged or extracted RedTilde folder containing RedTilde.exe. Pass -SourceDirectory.'
}

function Remove-PerUserFileAssociation {
    $progIdKey = 'HKCU:\Software\Classes\RedTilde.Markdown'
    if (Test-Path -LiteralPath $progIdKey) {
        Remove-Item -LiteralPath $progIdKey -Recurse -Force
    }
    $appKey = 'HKCU:\Software\Classes\Applications\RedTilde.exe'
    if (Test-Path -LiteralPath $appKey) {
        Remove-Item -LiteralPath $appKey -Recurse -Force
    }
    foreach ($extension in @('.md', '.markdown')) {
        $openWithKey = "HKCU:\Software\Classes\$extension\OpenWithProgids"
        if (Test-Path -LiteralPath $openWithKey) {
            $item = Get-ItemProperty -LiteralPath $openWithKey -Name 'RedTilde.Markdown' -ErrorAction SilentlyContinue
            if ($null -ne $item) {
                Remove-ItemProperty -LiteralPath $openWithKey -Name 'RedTilde.Markdown' -Force
            }
        }
    }
}

if (-not $InstallDirectory) {
    throw 'InstallDirectory is required when LOCALAPPDATA is not set.'
}

$targetDirectory = Assert-SafePerUserDirectory -TargetDirectory $InstallDirectory
Assert-NoRunningRedTildeInDirectory -TargetDirectory $targetDirectory

$programsMenu = [Environment]::GetFolderPath('Programs')
$shortcutPath = if ($programsMenu) { Join-Path $programsMenu 'RedTilde.lnk' } else { '' }
$uninstallKeyPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\RedTilde'

if ($Uninstall) {
    if ($shortcutPath -and (Test-Path -LiteralPath $shortcutPath -PathType Leaf)) {
        Remove-Item -LiteralPath $shortcutPath -Force
    }
    Remove-PerUserFileAssociation
    if (Test-Path -LiteralPath $uninstallKeyPath) {
        Remove-Item -LiteralPath $uninstallKeyPath -Recurse -Force
    }
    if (Test-Path -LiteralPath $targetDirectory -PathType Container) {
        Remove-Item -LiteralPath $targetDirectory -Recurse -Force
    }
    Write-Host "RedTilde uninstalled from '$targetDirectory'."
    if ($env:LOCALAPPDATA) {
        $preservedData = Join-Path $env:LOCALAPPDATA 'RedTilde'
        Write-Host "User settings, recovery snapshots, and crash reports under '$preservedData' were preserved."
    }
    return
}

$resolvedSource = Resolve-RedTildeSourceDirectory -Requested $SourceDirectory
$sourceExe = Join-Path $resolvedSource 'RedTilde.exe'
$versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($sourceExe)
$displayVersion = "$($versionInfo.FileMajorPart).$($versionInfo.FileMinorPart).$($versionInfo.FileBuildPart)"

[void](New-Item -ItemType Directory -Path $targetDirectory -Force)
if (-not $resolvedSource.Equals($targetDirectory, [StringComparison]::OrdinalIgnoreCase)) {
    foreach ($entry in @(Get-ChildItem -LiteralPath $resolvedSource -Force)) {
        Copy-Item -LiteralPath $entry.FullName -Destination (Join-Path $targetDirectory $entry.Name) -Recurse -Force
    }
}

$installedExe = Join-Path $targetDirectory 'RedTilde.exe'
if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
    throw "Installed executable is missing after copy: '$installedExe'."
}

$installedInstaller = Join-Path $targetDirectory 'Install-RedTilde.ps1'
if (-not ([IO.Path]::GetFullPath($PSCommandPath)).Equals($installedInstaller, [StringComparison]::OrdinalIgnoreCase)) {
    Copy-Item -LiteralPath $PSCommandPath -Destination $installedInstaller -Force
}

if ($shortcutPath) {
    $shortcutParent = Split-Path $shortcutPath -Parent
    if ($shortcutParent) {
        [void](New-Item -ItemType Directory -Path $shortcutParent -Force)
    }
    $shell = New-Object -ComObject WScript.Shell
    try {
        $shortcut = $shell.CreateShortcut($shortcutPath)
        $shortcut.TargetPath = $installedExe
        $shortcut.WorkingDirectory = $targetDirectory
        $shortcut.IconLocation = "$installedExe,0"
        $shortcut.Description = 'RedTilde Markdown editor'
        $shortcut.Save()
    }
    finally {
        [void][Runtime.InteropServices.Marshal]::ReleaseComObject($shell)
    }
}

[void](New-Item -Path $uninstallKeyPath -Force)
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'DisplayName' -Value 'RedTilde' -Type String
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'DisplayVersion' -Value $displayVersion -Type String
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'Publisher' -Value 'RedSalamanders' -Type String
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'InstallLocation' -Value $targetDirectory -Type String
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'DisplayIcon' -Value "$installedExe,0" -Type String
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'NoModify' -Value 1 -Type DWord
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'NoRepair' -Value 1 -Type DWord
$uninstallCommand = "pwsh.exe -NoProfile -ExecutionPolicy Bypass -File `"$installedInstaller`" -InstallDirectory `"$targetDirectory`" -Uninstall"
Set-ItemProperty -LiteralPath $uninstallKeyPath -Name 'UninstallString' -Value $uninstallCommand -Type String

if ($RegisterFileAssociation) {
    $progIdPath = 'HKCU:\Software\Classes\RedTilde.Markdown'
    [void](New-Item -Path $progIdPath -Force)
    Set-ItemProperty -LiteralPath $progIdPath -Name '(Default)' -Value 'Markdown Document' -Type String
    [void](New-Item -Path (Join-Path $progIdPath 'DefaultIcon') -Force)
    Set-ItemProperty -LiteralPath (Join-Path $progIdPath 'DefaultIcon') -Name '(Default)' -Value "$installedExe,0" -Type String
    [void](New-Item -Path (Join-Path $progIdPath 'shell\open\command') -Force)
    Set-ItemProperty -LiteralPath (Join-Path $progIdPath 'shell\open\command') -Name '(Default)' -Value "`"$installedExe`" `"%1`"" -Type String

    foreach ($extension in @('.md', '.markdown')) {
        $openWithKey = "HKCU:\Software\Classes\$extension\OpenWithProgids"
        [void](New-Item -Path $openWithKey -Force)
        Set-ItemProperty -LiteralPath $openWithKey -Name 'RedTilde.Markdown' -Value '' -Type String
    }

    $supportedTypes = 'HKCU:\Software\Classes\Applications\RedTilde.exe\SupportedTypes'
    [void](New-Item -Path $supportedTypes -Force)
    Set-ItemProperty -LiteralPath $supportedTypes -Name '.md' -Value '' -Type String
    Set-ItemProperty -LiteralPath $supportedTypes -Name '.markdown' -Value '' -Type String
}

Write-Host "RedTilde $displayVersion installed to '$targetDirectory'."
if ($shortcutPath) {
    Write-Host "Start Menu shortcut: '$shortcutPath'."
}
if ($RegisterFileAssociation) {
    Write-Host 'Per-user .md and .markdown file associations registered under HKCU\Software\Classes.'
}
