<#
.SYNOPSIS
Opt-in, fail-closed real-location qualification harness for RedTilde (SMB/NAS, OneDrive selftest, WSL selftest).
.DESCRIPTION
Never invoked automatically by default headless runs or hosted CI. Validates a manifest-declared selftest root,
refuses drive roots, user profile/Desktop/Documents/undeclared OneDrive roots, non-selftest network shares
(such as \\fileserver.example.invalid\Multimedia), relative paths, '.'/'..' traversal, shallow 2-segment UNC roots, and surrogate
symlink/junction reparse paths. When reachable and -DryRun is not set, stages a UTF-8 showcase Markdown document
inside a unique per-run redtilde-run-<guid> subfolder beneath the verified selftest root, executes RedTilde.exe
--capture, verifies capture output and untouched document bytes/BOM, and cleans up without following reparse points.
.PARAMETER ManifestPath
Absolute non-reparse path to a redtilde.real-locations.v1 or red-salamander.test-machine-resources.v1 manifest JSON.
.PARAMETER IncludeKind
Optional filter selecting one or more kinds: smb-share, onedrive-selftest, wsl-selftest.
.PARAMETER Configuration
Build configuration for RedTilde.exe (Debug or Release; defaults to Release).
.PARAMETER DryRun
Validate manifest, path policy, and live reachability without creating or modifying any files.
.PARAMETER ExecutablePath
Optional explicit path to RedTilde.exe (defaults to .build/x64/<Configuration>/RedTilde.exe).
.PARAMETER IncludeNativeSuites
Also execute the Storage, Settings, and Recovery native self-test suites (--self-test --test-scope <Scope> --data-dir <runRoot>\<Scope>) inside each per-run selftest folder.
.PARAMETER NativeScopes
Native self-test scopes to execute when -IncludeNativeSuites is set (defaults to Storage, Settings, Recovery).
.PARAMETER RetainLogs
Retain suite stdout/stderr and copied tests.json files under .build/real-location-runs/<runId> even when all checks pass.
#>
[CmdletBinding()]
param(
    [string]$ManifestPath = '',
    [ValidateSet('smb-share', 'onedrive-selftest', 'wsl-selftest')]
    [string[]]$IncludeKind = @(),
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$DryRun,
    [string]$ExecutablePath = '',
    [switch]$IncludeNativeSuites,
    [ValidateSet('Storage', 'Settings', 'Recovery')]
    [string[]]$NativeScopes = @('Storage', 'Settings', 'Recovery'),
    [switch]$RetainLogs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:RedTildeRealLocationSchema = 'redtilde.real-locations.v1'
$script:RedTildeCompatResourceSchema = 'red-salamander.test-machine-resources.v1'
$script:RedTildeAllowedKinds = @('smb-share', 'onedrive-selftest', 'wsl-selftest')

function Test-RedTildePathText {
    param(
        [AllowEmptyString()][string]$Value,
        [Parameter(Mandatory)][string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Value) -or $Value.Length -gt 1024 -or $Value.IndexOf([char]0) -ge 0 -or $Value -match '[\r\n]') {
        throw "$Description must be non-empty, at most 1024 characters, and contain no NUL or newline."
    }
    return $Value
}

function Assert-RedTildeNoDotSegments {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Description
    )

    $trimmed = $Path.TrimStart('\', '/')
    $rawSegments = @($trimmed -split '[\\/]' | Where-Object { $_ -ne '' })
    foreach ($segment in $rawSegments) {
        if ($segment -ceq '.' -or $segment -ceq '..') {
            throw "$Description must not contain '.' or '..' traversal segments: $Path"
        }
    }
}

function Test-RedTildeSurrogateLinkItem {
    param([Parameter(Mandatory)][IO.FileSystemInfo]$Item)

    if ($Item.LinkType -in @('SymbolicLink', 'Junction')) {
        return $true
    }
    if (-not [string]::IsNullOrEmpty($Item.LinkTarget)) {
        return $true
    }
    return $false
}

function Get-RedTildeExistingItemOrReparseAttributes {
    param([Parameter(Mandatory)][string]$Path)
    try {
        return [IO.File]::GetAttributes($Path)
    } catch [IO.FileNotFoundException] {
        return $null
    } catch [IO.DirectoryNotFoundException] {
        return $null
    }
}

function Invoke-RedTildeRealLocationProcess {
    param(
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$StdoutPath,
        [Parameter(Mandatory)][string]$StderrPath,
        [Parameter(Mandatory)][int]$TimeoutMs
    )
    $start = [Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        $start.ArgumentList.Add($argument)
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    try {
        if (-not $process.Start()) {
            throw "Failed to launch process '$Executable'."
        }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $timedOut = -not $process.WaitForExit($TimeoutMs)
        if ($timedOut) {
            try { $process.Kill($true) } catch [InvalidOperationException] { }
            [void]$process.WaitForExit(15000)
        }
        $utf8NoBom = [Text.UTF8Encoding]::new($false)
        [IO.File]::WriteAllText($StdoutPath, $stdoutTask.GetAwaiter().GetResult(), $utf8NoBom)
        [IO.File]::WriteAllText($StderrPath, $stderrTask.GetAwaiter().GetResult(), $utf8NoBom)
        return [pscustomobject]@{
            TimedOut = $timedOut
            ExitCode = if ($timedOut) { -1 } else { $process.ExitCode }
        }
    } finally {
        $process.Dispose()
    }
}

function Assert-RedTildeSafePathAncestry {
    param(
        [Parameter(Mandatory)][string]$FullPath,
        [Parameter(Mandatory)][string]$Description,
        [switch]$AllowNonSurrogateCloudReparse
    )

    $pathRoot = [IO.Path]::GetPathRoot($FullPath)
    if ([string]::IsNullOrWhiteSpace($pathRoot)) {
        throw "$Description has no filesystem root: $FullPath"
    }

    $remainder = $FullPath.Substring($pathRoot.Length).Trim('\', '/')
    $current = $pathRoot.TrimEnd('\')
    if ($current.Length -eq 2 -and $current[1] -eq ':') {
        $current = "$current\"
    }

    $parts = @()
    if (-not [string]::IsNullOrWhiteSpace($remainder)) {
        $parts = @($remainder -split '[\\/]' | Where-Object { $_ -ne '' })
    }

    $candidates = [Collections.Generic.List[string]]::new()
    if (-not $FullPath.StartsWith('\\')) {
        $candidates.Add($current)
    }
    foreach ($part in $parts) {
        $current = Join-Path $current $part
        $candidates.Add($current)
    }

    foreach ($candidate in $candidates) {
        $attrs = Get-RedTildeExistingItemOrReparseAttributes -Path $candidate
        if ($null -eq $attrs) {
            continue
        }
        $item = Get-Item -LiteralPath $candidate -Force -ErrorAction SilentlyContinue
        if ($null -eq $item) {
            if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Description must not traverse or target an inaccessible or dangling reparse point: $candidate"
            }
            continue
        }
        if (Test-RedTildeSurrogateLinkItem -Item $item) {
            throw "$Description must not be or traverse a symlink or junction: $candidate"
        }
        if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            if (-not $AllowNonSurrogateCloudReparse) {
                throw "$Description must not be or traverse a reparse point: $candidate"
            }
        }
    }
}

function Assert-RedTildeAllowedSelftestRoot {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$Path,
        [ValidateSet('smb-share', 'onedrive-selftest', 'wsl-selftest')]
        [string]$Kind = 'onedrive-selftest'
    )

    $raw = Test-RedTildePathText -Value $Path -Description "Selftest root ($Kind)"
    if ($raw -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])') {
        throw "Selftest root must use ordinary Win32 or UNC path syntax, not device-namespace syntax: $raw"
    }
    if (-not [IO.Path]::IsPathFullyQualified($raw)) {
        throw "Selftest root must be an absolute path: $raw"
    }
    Assert-RedTildeNoDotSegments -Path $raw -Description 'Selftest root'

    $normalized = [IO.Path]::GetFullPath($raw).TrimEnd('\', '/')
    if ($normalized -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])') {
        throw "Selftest root resolves to forbidden device-namespace syntax: $raw"
    }
    if ($normalized.Length -eq 2 -and $normalized[1] -eq ':') {
        $normalized = "$normalized\"
    }

    $pathRoot = [IO.Path]::GetPathRoot($normalized)
    if ($normalized.TrimEnd('\') -ieq $pathRoot.TrimEnd('\')) {
        throw "Selftest root must never be a drive or share root: $raw"
    }

    $isUnc = $normalized.StartsWith('\\')
    if ($Kind -ceq 'smb-share') {
        if (-not $isUnc -or $normalized -imatch '^\\\\(wsl\.localhost|wsl\$)\\') {
            throw "SMB selftest root must be an absolute UNC share subdirectory (non-WSL): $raw"
        }
    } elseif ($Kind -ceq 'wsl-selftest') {
        if ($normalized -inotmatch '^\\\\(wsl\.localhost|wsl\$)\\[^\\]+\\') {
            throw "WSL selftest root must be an absolute \\wsl.localhost\<distro>\... or \\wsl$\<distro>\... subdirectory: $raw"
        }
    } elseif ($Kind -ceq 'onedrive-selftest') {
        if ($isUnc -or $normalized -inotmatch '^[A-Za-z]:\\') {
            throw "OneDrive/local selftest root must be an absolute local drive path: $raw"
        }
    }

    if ($isUnc) {
        $uncSegments = @($normalized.Trim('\').Split('\', [StringSplitOptions]::RemoveEmptyEntries))
        if ($uncSegments.Count -lt 3) {
            throw "UNC selftest root must specify a subdirectory below \\server\share (at least 3 segments): $raw"
        }
        $subSegments = @($uncSegments[2..($uncSegments.Count - 1)])
    } else {
        $driveRemainder = $normalized.Substring($pathRoot.Length).Trim('\')
        $subSegments = @($driveRemainder.Split('\', [StringSplitOptions]::RemoveEmptyEntries))
        if ($subSegments.Count -lt 1) {
            throw "Local selftest root must not be a drive root: $raw"
        }
    }

    if (@($subSegments | Where-Object { $_ -imatch 'selftest' }).Count -eq 0) {
        throw "Selftest root must include an explicit subdirectory segment containing 'selftest': $raw"
    }

    $forbiddenCandidates = @(
        [Environment]::GetFolderPath('Desktop'),
        [Environment]::GetFolderPath('MyDocuments'),
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Desktop' }),
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'Documents' })
    )
    foreach ($forbiddenFolder in $forbiddenCandidates) {
        if (-not [string]::IsNullOrWhiteSpace($forbiddenFolder)) {
            $forbiddenFull = [IO.Path]::GetFullPath($forbiddenFolder).TrimEnd('\', '/')
            if ($normalized.Equals($forbiddenFull, [StringComparison]::OrdinalIgnoreCase) -or
                $normalized.StartsWith("$forbiddenFull\", [StringComparison]::OrdinalIgnoreCase)) {
                throw "Selftest root must never target Desktop or Documents folders: $raw"
            }
        }
    }

    if ($Kind -ceq 'onedrive-selftest') {
        Assert-RedTildeSafePathAncestry -FullPath $normalized -Description 'Selftest root' -AllowNonSurrogateCloudReparse
    } else {
        Assert-RedTildeSafePathAncestry -FullPath $normalized -Description 'Selftest root'
    }

    return $normalized
}

function Test-RedTildeRealLocationManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$ManifestPath,
        [ValidateSet('smb-share', 'onedrive-selftest', 'wsl-selftest')]
        [string[]]$IncludeKind = @()
    )

    $rawManifest = Test-RedTildePathText -Value $ManifestPath -Description 'ManifestPath'
    if ($rawManifest -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])') {
        throw "ManifestPath must use ordinary path syntax: $rawManifest"
    }
    if (-not [IO.Path]::IsPathFullyQualified($rawManifest)) {
        throw "ManifestPath must be an absolute path: $rawManifest"
    }
    Assert-RedTildeNoDotSegments -Path $rawManifest -Description 'ManifestPath'

    $fullManifest = [IO.Path]::GetFullPath($rawManifest)
    if ($fullManifest -match '^(?:[\\/]{2}[?.][\\/]|[\\/]\?\?[\\/])') {
        throw "ManifestPath resolves to forbidden device-namespace syntax: $rawManifest"
    }
    if (-not (Test-Path -LiteralPath $fullManifest -PathType Leaf)) {
        throw "Real-location manifest does not exist: $fullManifest"
    }
    Assert-RedTildeSafePathAncestry -FullPath $fullManifest -Description 'ManifestPath'

    $json = Get-Content -LiteralPath $fullManifest -Raw -ErrorAction Stop
    $manifest = $json | ConvertFrom-Json -Depth 20 -ErrorAction Stop
    if ($null -eq $manifest -or -not $manifest.PSObject.Properties['schema']) {
        throw "Real-location manifest is missing required 'schema' property: $fullManifest"
    }

    $schema = [string]$manifest.schema
    if ($schema -cne $script:RedTildeRealLocationSchema -and $schema -cne $script:RedTildeCompatResourceSchema) {
        throw "Unsupported real-location manifest schema '$schema'."
    }
    if (-not $manifest.PSObject.Properties['resources'] -or $null -eq $manifest.resources) {
        throw "Real-location manifest is missing 'resources' array: $fullManifest"
    }

    $seenIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $validated = [Collections.Generic.List[object]]::new()
    foreach ($entry in @($manifest.resources)) {
        if ($null -eq $entry -or
            -not $entry.PSObject.Properties['id'] -or
            -not $entry.PSObject.Properties['kind'] -or
            -not $entry.PSObject.Properties['enabled'] -or
            $entry.enabled -isnot [bool]) {
            throw "Every manifest resource entry must define 'id', 'kind', and boolean 'enabled'."
        }

        $id = Test-RedTildePathText -Value ([string]$entry.id) -Description 'Resource id'
        $kind = Test-RedTildePathText -Value ([string]$entry.kind) -Description "Resource kind for '$id'"
        if (-not $seenIds.Add($id)) {
            throw "Duplicate real-location resource id '$id'."
        }

        if ($schema -ceq $script:RedTildeRealLocationSchema -and $kind -cnotin $script:RedTildeAllowedKinds) {
            throw "Unsupported resource kind '$kind' in $script:RedTildeRealLocationSchema manifest."
        }
        if ($kind -cnotin $script:RedTildeAllowedKinds) {
            continue
        }
        if (-not [bool]$entry.enabled) {
            continue
        }

        if (-not $entry.PSObject.Properties['root']) {
            throw "Enabled real-location resource '$id' ($kind) must define 'root'."
        }
        $verifiedRoot = Assert-RedTildeAllowedSelftestRoot -Path ([string]$entry.root) -Kind $kind
        $validated.Add([pscustomobject]@{
                id      = $id
                kind    = $kind
                enabled = $true
                root    = $verifiedRoot
            })
    }

    $selected = if ($IncludeKind.Count -gt 0) {
        @($validated | Where-Object { $_.kind -cin $IncludeKind })
    } else {
        @($validated)
    }

    return [pscustomobject]@{
        Schema       = $schema
        ManifestPath = $fullManifest
        AllTargets   = @($validated)
        Targets      = @($selected)
    }
}

function Remove-RedTildeDirectoryNoReparse {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    $attrs = Get-RedTildeExistingItemOrReparseAttributes -Path $Path
    if ($null -eq $attrs) {
        return
    }
    if (($attrs -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        if (($attrs -band [IO.FileAttributes]::Directory) -ne 0) {
            [IO.Directory]::Delete($Path, $false)
        } else {
            [IO.File]::Delete($Path)
        }
        return
    }
    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if (-not $item.PSIsContainer) {
        if ((($attrs -band [IO.FileAttributes]::ReadOnly) -ne 0)) {
            [IO.File]::SetAttributes($Path, ($attrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
        }
        [IO.File]::Delete($Path)
        return
    }

    foreach ($child in @(Get-ChildItem -LiteralPath $Path -Force -ErrorAction Stop)) {
        if ((($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)) {
            if ($child.PSIsContainer) {
                [IO.Directory]::Delete($child.FullName, $false)
            } else {
                [IO.File]::Delete($child.FullName)
            }
        } elseif ($child.PSIsContainer) {
            Remove-RedTildeDirectoryNoReparse -Path $child.FullName
        } else {
            if ((($child.Attributes -band [IO.FileAttributes]::ReadOnly) -ne 0)) {
                [IO.File]::SetAttributes($child.FullName, ($child.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)))
            }
            [IO.File]::Delete($child.FullName)
        }
    }
    if ((($attrs -band [IO.FileAttributes]::ReadOnly) -ne 0)) {
        [IO.File]::SetAttributes($Path, ($attrs -band (-bnot [IO.FileAttributes]::ReadOnly)))
    }
    [IO.Directory]::Delete($Path, $false)
}

function Initialize-RedTildeRealLocationProbeNative {
    if ('RedTildeRealLocationProbe.Native' -as [type]) {
        return
    }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace RedTildeRealLocationProbe {
    [StructLayout(LayoutKind.Sequential)]
    public struct FILETIME {
        public uint dwLowDateTime;
        public uint dwHighDateTime;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BY_HANDLE_FILE_INFORMATION {
        public uint dwFileAttributes;
        public FILETIME ftCreationTime;
        public FILETIME ftLastAccessTime;
        public FILETIME ftLastWriteTime;
        public uint dwVolumeSerialNumber;
        public uint nFileSizeHigh;
        public uint nFileSizeLow;
        public uint nNumberOfLinks;
        public uint nFileIndexHigh;
        public uint nFileIndexLow;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct FILE_ID_INFO {
        public ulong VolumeSerialNumber;
        public ulong FileIdLow;
        public ulong FileIdHigh;
    }

    public static class Native {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern uint GetDriveTypeW(string lpRootPathName);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr CreateFileW(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetFileInformationByHandle(IntPtr hFile, out BY_HANDLE_FILE_INFORMATION lpFileInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetFileInformationByHandleEx(IntPtr hFile, int FileInformationClass, out FILE_ID_INFO lpFileInformation, uint dwBufferSize);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CreateHardLinkW(string lpFileName, string lpExistingFileName, IntPtr lpSecurityAttributes);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CreateSymbolicLinkW(string lpSymlinkFileName, string lpTargetFileName, uint dwFlags);
    }
}
'@
}

function Get-RedTildeLocationCapabilities {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Directory)

    Initialize-RedTildeRealLocationProbeNative
    $probeDir = Join-Path $Directory 'cap-probe'
    [void][IO.Directory]::CreateDirectory($probeDir)
    $invalidHandle = [IntPtr]::new(-1)

    $driveRoot = [IO.Path]::GetPathRoot($probeDir)
    if (-not $driveRoot.EndsWith('\')) {
        $driveRoot = "$driveRoot\"
    }
    $driveType = [int][RedTildeRealLocationProbe.Native]::GetDriveTypeW($driveRoot)

    $fileIdInfoSupported = $false
    $fileIdInfoWin32Error = 0
    $hardlinkCreateSucceeded = $false
    $hardlinkLinkCountReported = $false
    $hardlinkSharesFileIndex = $false
    $symlinkCreateSupported = $false
    $symlinkWin32Error = 0
    $junctionCreateSupported = $false

    try {
        $targetFile = Join-Path $probeDir 'target.txt'
        [IO.File]::WriteAllText($targetFile, 'cap-probe', [Text.UTF8Encoding]::new($false))

        $hTarget = [RedTildeRealLocationProbe.Native]::CreateFileW($targetFile, 0, 7, [IntPtr]::Zero, 3, 0x80, [IntPtr]::Zero)
        if ($hTarget -ne [IntPtr]::Zero -and $hTarget -ne $invalidHandle) {
            try {
                $idInfo = [RedTildeRealLocationProbe.FILE_ID_INFO]::new()
                if ([RedTildeRealLocationProbe.Native]::GetFileInformationByHandleEx($hTarget, 18, [ref]$idInfo, [uint32][Runtime.InteropServices.Marshal]::SizeOf($idInfo))) {
                    $fileIdInfoSupported = $true
                } else {
                    $fileIdInfoWin32Error = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
                }
            } finally {
                [void][RedTildeRealLocationProbe.Native]::CloseHandle($hTarget)
            }
        }

        $aliasFile = Join-Path $probeDir 'alias.txt'
        if ([RedTildeRealLocationProbe.Native]::CreateHardLinkW($aliasFile, $targetFile, [IntPtr]::Zero)) {
            $hardlinkCreateSucceeded = $true
            $hAfter = [RedTildeRealLocationProbe.Native]::CreateFileW($targetFile, 0, 7, [IntPtr]::Zero, 3, 0x80, [IntPtr]::Zero)
            $hAlias = [RedTildeRealLocationProbe.Native]::CreateFileW($aliasFile, 0, 7, [IntPtr]::Zero, 3, 0x80, [IntPtr]::Zero)
            try {
                $afterInfo = [RedTildeRealLocationProbe.BY_HANDLE_FILE_INFORMATION]::new()
                $aliasInfo = [RedTildeRealLocationProbe.BY_HANDLE_FILE_INFORMATION]::new()
                if (($hAfter -ne [IntPtr]::Zero) -and ($hAfter -ne $invalidHandle) -and
                    ([RedTildeRealLocationProbe.Native]::GetFileInformationByHandle($hAfter, [ref]$afterInfo))) {
                    $hardlinkLinkCountReported = ($afterInfo.nNumberOfLinks -gt 1)
                    if (($hAlias -ne [IntPtr]::Zero) -and ($hAlias -ne $invalidHandle) -and
                        ([RedTildeRealLocationProbe.Native]::GetFileInformationByHandle($hAlias, [ref]$aliasInfo))) {
                        $hardlinkSharesFileIndex = ($afterInfo.dwVolumeSerialNumber -eq $aliasInfo.dwVolumeSerialNumber) -and
                            ($afterInfo.nFileIndexHigh -eq $aliasInfo.nFileIndexHigh) -and
                            ($afterInfo.nFileIndexLow -eq $aliasInfo.nFileIndexLow) -and
                            (($afterInfo.nFileIndexHigh -ne 0) -or ($afterInfo.nFileIndexLow -ne 0))
                    }
                }
            } finally {
                if ($hAfter -ne [IntPtr]::Zero -and $hAfter -ne $invalidHandle) {
                    [void][RedTildeRealLocationProbe.Native]::CloseHandle($hAfter)
                }
                if ($hAlias -ne [IntPtr]::Zero -and $hAlias -ne $invalidHandle) {
                    [void][RedTildeRealLocationProbe.Native]::CloseHandle($hAlias)
                }
            }
        }

        $symFile = Join-Path $probeDir 'sym.txt'
        if ([RedTildeRealLocationProbe.Native]::CreateSymbolicLinkW($symFile, 'target.txt', 2)) {
            $symlinkCreateSupported = $true
        } else {
            $symlinkWin32Error = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        }

        $jTarget = Join-Path $probeDir 'jtarget'
        $jLink = Join-Path $probeDir 'jlink'
        [void][IO.Directory]::CreateDirectory($jTarget)
        try {
            $null = New-Item -ItemType Junction -Path $jLink -Target $jTarget -ErrorAction Stop
            $jItem = Get-Item -LiteralPath $jLink -Force -ErrorAction Stop
            if ($jItem.LinkType -ceq 'Junction') {
                $junctionCreateSupported = $true
            }
        } catch {
            $junctionCreateSupported = $false
        }
    } finally {
        Remove-RedTildeDirectoryNoReparse -Path $probeDir
    }

    return [pscustomobject]@{
        driveType                 = $driveType
        fileIdInfoSupported       = $fileIdInfoSupported
        fileIdInfoWin32Error      = $fileIdInfoWin32Error
        hardlinkCreateSucceeded   = $hardlinkCreateSucceeded
        hardlinkLinkCountReported = $hardlinkLinkCountReported
        hardlinkSharesFileIndex   = $hardlinkSharesFileIndex
        symlinkCreateSupported    = $symlinkCreateSupported
        symlinkWin32Error         = $symlinkWin32Error
        junctionCreateSupported   = $junctionCreateSupported
    }
}

function Split-RedTildeRealLocationFailures {
    [CmdletBinding()]
    param(
        [AllowEmptyCollection()][object[]]$FailedCases = @(),
        [Parameter(Mandatory)][object]$Capabilities
    )

    $hardlinkDependentCases = @(
        'Storage.HardlinkSavePreservesAliasesAndProtectsBeforeMutation',
        'Storage.HardlinkFailuresRetainCompleteRecoveryAndBaseline',
        'Storage.HardlinkAdmissionFailureNeverMutatesTarget',
        'Storage.HardlinkSaveSuccessSurvivesBlockedRecoveryRetirement',
        'Storage.NewHardlinkDuringStagingSelectsProtectedInPlaceSave'
    )

    $unexpected = [Collections.Generic.List[object]]::new()
    $unsupported = [Collections.Generic.List[object]]::new()

    foreach ($case in @($FailedCases)) {
        if ($null -eq $case) {
            continue
        }
        $name = [string]$case.name
        $err = if ($case.PSObject.Properties['error']) { [string]$case.error } else { '' }
        $isUnsupported = $false

        if ($name -cin $hardlinkDependentCases -and -not [bool]$Capabilities.hardlinkLinkCountReported) {
            $isUnsupported = $true
        } elseif ($name -ceq 'Storage.SharedWriterReadAndSaveAsBaseline' -and
            -not [bool]$Capabilities.hardlinkSharesFileIndex -and
            ($err -match 'create an owned alias|Save As through an alias')) {
            $isUnsupported = $true
        } elseif ($name -ceq 'Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget' -and
            -not [bool]$Capabilities.symlinkCreateSupported) {
            $isUnsupported = $true
        } elseif ($name -ceq 'Files.ReparseLeavesNeverTraverse' -and
            -not [bool]$Capabilities.junctionCreateSupported) {
            $isUnsupported = $true
        }

        if ($isUnsupported) {
            $unsupported.Add($case)
        } else {
            $unexpected.Add($case)
        }
    }

    return [pscustomobject]@{
        unexpectedFailures            = @($unexpected.ToArray())
        unsupportedProviderPrimitives = @($unsupported.ToArray())
    }
}

function Invoke-RedTildeRealLocationRun {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$ManifestPath,
        [ValidateSet('smb-share', 'onedrive-selftest', 'wsl-selftest')]
        [string[]]$IncludeKind = @(),
        [ValidateSet('Debug', 'Release')]
        [string]$Configuration = 'Release',
        [switch]$DryRun,
        [string]$ExecutablePath = '',
        [switch]$IncludeNativeSuites,
        [ValidateSet('Storage', 'Settings', 'Recovery')]
        [string[]]$NativeScopes = @('Storage', 'Settings', 'Recovery'),
        [switch]$RetainLogs
    )

    $repoRoot = Split-Path -Parent $PSScriptRoot
    $manifest = Test-RedTildeRealLocationManifest -ManifestPath $ManifestPath -IncludeKind $IncludeKind
    if ($manifest.Targets.Count -eq 0) {
        throw "No enabled selftest targets matched in manifest '$($manifest.ManifestPath)'."
    }

    $exe = if (-not [string]::IsNullOrWhiteSpace($ExecutablePath)) {
        [IO.Path]::GetFullPath($ExecutablePath)
    } else {
        Join-Path $repoRoot ".build/x64/$Configuration/RedTilde.exe"
    }

    if (-not $DryRun) {
        if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
            throw "RedTilde executable not found for real-location run: $exe"
        }
        $exeItem = Get-Item -LiteralPath $exe -Force -ErrorAction Stop
        if (($exeItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "RedTilde executable must not be a reparse point: $exe"
        }
        if ($IncludeNativeSuites -and $NativeScopes.Count -eq 0) {
            throw 'NativeScopes must select at least one scope when -IncludeNativeSuites is specified.'
        }
    }

    $results = [Collections.Generic.List[object]]::new()
    foreach ($target in $manifest.Targets) {
        $verifiedRoot = Assert-RedTildeAllowedSelftestRoot -Path $target.root -Kind $target.kind
        if (-not (Test-Path -LiteralPath $verifiedRoot -PathType Container)) {
            throw "Real-location selftest root is unreachable or not an existing directory (failing closed without fallback): $verifiedRoot"
        }

        if ($DryRun) {
            $results.Add([pscustomobject]@{
                    id        = $target.id
                    kind      = $target.kind
                    root      = $verifiedRoot
                    reachable = $true
                    dryRun    = $true
                    status    = 'DRY_RUN_VERIFIED'
                })
            continue
        }

        $runId = [guid]::NewGuid().ToString('N')
        $runRoot = Join-Path $verifiedRoot ("redtilde-run-$runId")
        if (Test-Path -LiteralPath $runRoot) {
            throw "Unique per-run directory already exists: $runRoot"
        }
        $runScratchRoot = Join-Path $repoRoot ".build/real-location-runs/$runId"
        $dataDir = Join-Path $runScratchRoot 'data'

        $createdRunRoot = $false
        $createdDataDir = $false
        $keepLogs = [bool]$RetainLogs
        try {
            [void][IO.Directory]::CreateDirectory($runRoot)
            $createdRunRoot = $true
            $runItem = Get-Item -LiteralPath $runRoot -Force -ErrorAction Stop
            if (Test-RedTildeSurrogateLinkItem -Item $runItem) {
                throw "Created per-run directory unexpectedly resolved to a symlink or junction: $runRoot"
            }

            [void][IO.Directory]::CreateDirectory($dataDir)
            $createdDataDir = $true

            $docPath = Join-Path $runRoot 'showcase.md'
            $capturePath = Join-Path $runRoot 'capture.png'
            $sidecarPath = "$capturePath.scenario.json"

            $bom = [byte[]](0xEF, 0xBB, 0xBF)
            $markdown = @(
                '# RedTilde Real-Location Qualification'
                ''
                'Unicode fidelity: café, naïve, 東京, Ελληνικά, and 🌙.'
                ''
                '- [x] Stage UTF-8 BOM + Markdown showcase'
                '- [ ] Complete target qualification'
                ''
                'Inline math $E=mc^2$.'
                ''
                '```mermaid'
                'flowchart LR'
                'Stage[Stage] --> Capture[Capture]'
                '```'
            ) -join "`r`n"
            $contentBytes = [Text.UTF8Encoding]::new($false).GetBytes($markdown + "`r`n")
            $expectedBytes = [byte[]]::new($bom.Length + $contentBytes.Length)
            [Array]::Copy($bom, 0, $expectedBytes, 0, $bom.Length)
            [Array]::Copy($contentBytes, 0, $expectedBytes, $bom.Length, $contentBytes.Length)
            [IO.File]::WriteAllBytes($docPath, $expectedBytes)
            $beforeHash = (Get-FileHash -LiteralPath $docPath -Algorithm SHA256).Hash

            $stdoutPath = Join-Path $runScratchRoot 'stdout.log'
            $stderrPath = Join-Path $runScratchRoot 'stderr.log'
            $capProc = Invoke-RedTildeRealLocationProcess -Executable $exe -Arguments @(
                '--capture', $capturePath,
                '--scenario', 'sample-document',
                '--document', $docPath,
                '--data-dir', $dataDir
            ) -StdoutPath $stdoutPath -StderrPath $stderrPath -TimeoutMs 120000
            if ($capProc.TimedOut) {
                $keepLogs = $true
                throw "RedTilde.exe --capture timed out after 120s for '$($target.id)' ($verifiedRoot)."
            }

            if ($capProc.ExitCode -ne 0) {
                $keepLogs = $true
                throw "RedTilde.exe --capture exited with code $($capProc.ExitCode) for '$($target.id)' ($verifiedRoot)."
            }
            if (-not (Test-Path -LiteralPath $capturePath -PathType Leaf)) {
                $keepLogs = $true
                throw "Capture PNG was not written under '$runRoot'."
            }
            $captureBytes = [IO.File]::ReadAllBytes($capturePath)
            if ($captureBytes.Length -lt 8 -or $captureBytes[0] -ne 0x89 -or $captureBytes[1] -ne 0x50 -or $captureBytes[2] -ne 0x4E -or $captureBytes[3] -ne 0x47) {
                $keepLogs = $true
                throw "Capture output is empty or not a valid PNG: $capturePath"
            }
            if (-not (Test-Path -LiteralPath $sidecarPath -PathType Leaf)) {
                $keepLogs = $true
                throw "Capture scenario sidecar was not written: $sidecarPath"
            }
            $sidecar = Get-Content -LiteralPath $sidecarPath -Raw | ConvertFrom-Json
            if (-not $sidecar.hidden -or [string]$sidecar.scenario -cne 'sample-document') {
                $keepLogs = $true
                throw "Capture scenario sidecar failed verification: $sidecarPath"
            }

            $afterHash = (Get-FileHash -LiteralPath $docPath -Algorithm SHA256).Hash
            $afterBytes = [IO.File]::ReadAllBytes($docPath)
            if ($afterHash -cne $beforeHash -or $afterBytes.Length -ne $expectedBytes.Length -or
                $afterBytes[0] -ne 0xEF -or $afterBytes[1] -ne 0xBB -or $afterBytes[2] -ne 0xBF) {
                $keepLogs = $true
                throw "Staged document bytes or UTF-8 BOM changed during capture: $docPath"
            }

            $capabilities = $null
            $suiteResults = [Collections.Generic.List[object]]::new()
            $allSuitesPassed = $true
            if ($IncludeNativeSuites) {
                $capabilities = Get-RedTildeLocationCapabilities -Directory $runRoot
                foreach ($scopeName in $NativeScopes) {
                    $suiteDataDir = Join-Path $runRoot "suite-$scopeName"
                    $suiteStdout = Join-Path $runScratchRoot "$scopeName.stdout.log"
                    $suiteStderr = Join-Path $runScratchRoot "$scopeName.stderr.log"
                    $suiteReportDest = Join-Path $runScratchRoot "$scopeName.tests.json"
                    $suiteProc = Invoke-RedTildeRealLocationProcess -Executable $exe -Arguments @(
                        '--self-test',
                        '--test-scope', $scopeName,
                        '--data-dir', $suiteDataDir
                    ) -StdoutPath $suiteStdout -StderrPath $suiteStderr -TimeoutMs 180000
                    $timedOut = $suiteProc.TimedOut
                    if ($timedOut) {
                        $keepLogs = $true
                    }
                    $suiteExit = $suiteProc.ExitCode

                    $reportSource = Join-Path $suiteDataDir 'tests.json'
                    $passedCount = 0
                    $failedCount = 0
                    $selectedCount = 0
                    $catalogOnly = $false
                    $failedCases = @()
                    if (Test-Path -LiteralPath $reportSource -PathType Leaf) {
                        Copy-Item -LiteralPath $reportSource -Destination $suiteReportDest -Force
                        $report = Get-Content -LiteralPath $suiteReportDest -Raw | ConvertFrom-Json
                        $passedCount = [int]$report.passed
                        $failedCount = [int]$report.failed
                        $selectedCount = [int]$report.selected
                        $catalogOnly = if ($report.PSObject.Properties['catalogOnly']) { [bool]$report.catalogOnly } else { $false }
                        $failedCases = @($report.tests | Where-Object { -not $_.passed } | ForEach-Object {
                                [pscustomobject]@{
                                    name  = [string]$_.name
                                    error = [string]$_.error
                                }
                            })
                    }

                    $split = Split-RedTildeRealLocationFailures -FailedCases $failedCases -Capabilities $capabilities
                    $exitMatchesReport = (($failedCount -eq 0) -and ($suiteExit -eq 0)) -or
                        (($failedCount -gt 0) -and ($suiteExit -eq 1) -and ($failedCases.Count -eq $failedCount) -and ($split.unexpectedFailures.Count -eq 0))
                    $suiteOk = (-not $timedOut) -and ($selectedCount -gt 0) -and (-not $catalogOnly) -and
                        (($passedCount + $failedCount) -eq $selectedCount) -and $exitMatchesReport
                    if ($failedCount -gt 0) {
                        $keepLogs = $true
                    }
                    if (-not $suiteOk) {
                        $allSuitesPassed = $false
                        $keepLogs = $true
                    }

                    $suiteResults.Add([pscustomobject]@{
                            scope                         = $scopeName
                            exitCode                      = $suiteExit
                            timedOut                      = $timedOut
                            passed                        = $passedCount
                            failed                        = $failedCount
                            selected                      = $selectedCount
                            failedCases                   = @($failedCases)
                            unexpectedFailures            = @($split.unexpectedFailures)
                            unsupportedProviderPrimitives = @($split.unsupportedProviderPrimitives)
                        })
                }
            }

            $results.Add([pscustomobject]@{
                    id               = $target.id
                    kind             = $target.kind
                    root             = $verifiedRoot
                    runLeaf          = "redtilde-run-$runId"
                    reachable        = $true
                    dryRun           = $false
                    captureByteCount = $captureBytes.Length
                    documentSha256   = $afterHash
                    capabilities     = $capabilities
                    nativeSuites     = @($suiteResults)
                    logDirectory     = if ($keepLogs) { ".build/real-location-runs/$runId" } else { $null }
                    status           = if ($allSuitesPassed) { 'PASSED' } else { 'NATIVE_SUITE_FAILED' }
                })
        } catch {
            $keepLogs = $true
            throw
        } finally {
            if ($createdRunRoot) {
                Remove-RedTildeDirectoryNoReparse -Path $runRoot
                if (Test-Path -LiteralPath $runRoot) {
                    throw "Per-run selftest directory still exists after cleanup: $runRoot"
                }
            }
            if ($createdDataDir -and -not $keepLogs) {
                Remove-RedTildeDirectoryNoReparse -Path $runScratchRoot
            }
        }
    }

    return @($results)
}

if ($MyInvocation.InvocationName -ne '.' -and -not [string]::IsNullOrEmpty($MyInvocation.InvocationName)) {
    $runResult = Invoke-RedTildeRealLocationRun -ManifestPath $ManifestPath -IncludeKind $IncludeKind -Configuration $Configuration -DryRun:$DryRun -ExecutablePath $ExecutablePath -IncludeNativeSuites:$IncludeNativeSuites -NativeScopes $NativeScopes -RetainLogs:$RetainLogs
    $runResult | ConvertTo-Json -Depth 8
    if (@($runResult | Where-Object { $_.status -cnotin @('PASSED', 'DRY_RUN_VERIFIED') }).Count -gt 0) {
        exit 1
    }
}
