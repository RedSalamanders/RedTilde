<#
.SYNOPSIS
Installs the RedTilde vcpkg manifest dependencies strictly under .build for x64, ARM64, or both.
.PARAMETER Platform
Target platform: x64, ARM64, or All.
.PARAMETER Asan
Install the AddressSanitizer-instrumented tree (x64-windows-static-md-asan or arm64-windows-static-md-asan).
.PARAMETER MSBuildPath
Optional MSBuild.exe path; defaults to Get-RedTildeMSBuild discovery.
#>
[CmdletBinding()]
param(
    [ValidateSet('x64', 'ARM64', 'All')]
    [string] $Platform = 'x64',
    [switch] $Asan,
    [string] $MSBuildPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSCommandPath
Import-Module (Join-Path $repoRoot 'Build/Toolchain.psm1') -Force
Import-Module (Join-Path $repoRoot 'Build/VcpkgToolCheckout.psm1') -Force
Import-Module (Join-Path $repoRoot 'Build/VcpkgToolset.psm1') -Force

$buildRoot = Join-Path $repoRoot '.build'
$toolRoot = Join-Path $buildRoot 'vcpkg-tool'
$toolStampPath = Join-Path $buildRoot 'vcpkg-tool.commit'
$toolIdentityPath = Join-Path $repoRoot 'vcpkg-tool.json'
$manifestPath = Join-Path $repoRoot 'vcpkg.json'
$downloadsRoot = Join-Path $buildRoot 'vcpkg-downloads'

if (-not (Get-Command 'git.exe' -ErrorAction SilentlyContinue)) {
    throw 'Git was not found. Install Git for Windows and ensure git.exe is on PATH.'
}
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "The vcpkg manifest was not found: $manifestPath"
}

$toolIdentity = Get-Content -Raw -LiteralPath $toolIdentityPath | ConvertFrom-Json
$commit = ([string] $toolIdentity.commit).ToLowerInvariant()
if ($commit -notmatch '^[0-9a-f]{40}$') {
    throw 'vcpkg-tool.json must contain a full 40-character commit hash.'
}

$msbuild = if ($MSBuildPath) { $MSBuildPath } else { Get-RedTildeMSBuild }
if (-not (Test-Path -LiteralPath $msbuild -PathType Leaf)) {
    throw "MSBuild was not found at '$msbuild'."
}
$installation = $msbuild -replace '(?i)[\\/]MSBuild[\\/].*$', ''
if ([string]::IsNullOrWhiteSpace($installation) -or -not (Test-Path -LiteralPath $installation -PathType Container)) {
    throw "The Visual Studio installation of '$msbuild' could not be determined."
}
$toolset = Get-RedTildeDefaultToolset -Installation $installation
[xml]$msvcPolicy = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Directory.Build.props')
$requiredTools = $msvcPolicy.SelectSingleNode('/Project/PropertyGroup/RequiredMsvcToolsVersion').InnerText
if ($toolset.MajorMinor -cne $requiredTools) {
    throw "This product requires default MSVC $requiredTools.x; selected $($toolset.Version)."
}
Write-Host "Visual Studio: $installation" -ForegroundColor Cyan
Write-Host "MSVC toolset:  $($toolset.Version), the default MSBuild compiles with; vcpkg is pinned to $($toolset.MajorMinor)" -ForegroundColor Cyan

[void][IO.Directory]::CreateDirectory($buildRoot)
[void][IO.Directory]::CreateDirectory($downloadsRoot)

Initialize-VcpkgToolCheckout -ToolIdentityPath $toolIdentityPath -ToolRoot $toolRoot

$dirty = & git.exe -C $toolRoot status --porcelain
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect the managed vcpkg checkout.'
}
if ($dirty) {
    throw "The managed vcpkg checkout contains local changes: $toolRoot"
}

$previousMetrics = [Environment]::GetEnvironmentVariable('VCPKG_DISABLE_METRICS', 'Process')
$previousDownloads = [Environment]::GetEnvironmentVariable('VCPKG_DOWNLOADS', 'Process')
try {
    $env:VCPKG_DISABLE_METRICS = '1'
    $env:VCPKG_DOWNLOADS = $downloadsRoot

    $vcpkg = Join-Path $toolRoot 'vcpkg.exe'
    $stampedCommit = if (Test-Path -LiteralPath $toolStampPath -PathType Leaf) {
        (Get-Content -Raw -LiteralPath $toolStampPath).Trim()
    } else {
        ''
    }
    if (-not (Test-Path -LiteralPath $vcpkg -PathType Leaf) -or $stampedCommit -cne $commit) {
        Write-Host 'Bootstrapping vcpkg...' -ForegroundColor Cyan
        & (Join-Path $toolRoot 'bootstrap-vcpkg.bat') -disableMetrics
        if ($LASTEXITCODE -ne 0) {
            throw "vcpkg bootstrap failed with exit code $LASTEXITCODE."
        }
        Set-Content -LiteralPath $toolStampPath -Value $commit -Encoding ascii
    }

    $overlayDirectory = Join-Path $buildRoot 'vcpkg-triplets'
    $toolchainFromOverlay = [IO.Path]::GetFullPath((Join-Path $overlayDirectory '..\..\.build\vcpkg-tool\scripts\toolchains\windows.cmake'))
    if ($toolchainFromOverlay -cne [IO.Path]::GetFullPath((Join-Path $toolRoot 'scripts\toolchains\windows.cmake'))) {
        throw "asan-toolchain.cmake would not find vcpkg's windows.cmake from $overlayDirectory; keep the overlay directory two levels below the repository root."
    }

    $tripletSources = [ordered]@{
        'x64-windows'                  = Join-Path $toolRoot 'triplets\x64-windows.cmake'
        'arm64-windows'                = Join-Path $toolRoot 'triplets\arm64-windows.cmake'
        'x64-windows-static-md'        = Join-Path $repoRoot 'Build\vcpkg-triplets\x64-windows-static-md.cmake'
        'arm64-windows-static-md'      = Join-Path $repoRoot 'Build\vcpkg-triplets\arm64-windows-static-md.cmake'
        'x64-windows-static-md-asan'   = Join-Path $repoRoot 'Build\vcpkg-triplets\x64-windows-static-md-asan.cmake'
        'arm64-windows-static-md-asan' = Join-Path $repoRoot 'Build\vcpkg-triplets\arm64-windows-static-md-asan.cmake'
    }
    foreach ($name in $tripletSources.Keys) {
        $overlay = Update-RedTildeVcpkgOverlayTriplet -SourceTripletPath $tripletSources[$name] -Toolset $toolset -OutputDirectory $overlayDirectory
        Write-Host "Overlay triplet $($overlay.Path) $(if ($overlay.Changed) { 'written' } else { 'unchanged' })" -ForegroundColor DarkGray
    }

    $platforms = if ($Platform -eq 'All') { @('x64', 'ARM64') } else { @($Platform) }
    foreach ($targetPlatform in $platforms) {
        $platformScope = $targetPlatform.ToLowerInvariant()
        $triplet = if ($targetPlatform -eq 'ARM64') { 'arm64-windows-static-md' } else { 'x64-windows-static-md' }
        if ($Asan) {
            $platformScope = "$platformScope-asan"
            $triplet = if ($targetPlatform -eq 'ARM64') { 'arm64-windows-static-md-asan' } else { 'x64-windows-static-md-asan' }
        }
        $installRoot = Join-Path $buildRoot "vcpkg_installed\$platformScope"
        $buildTreesRoot = Join-Path $buildRoot "vcpkg_buildtrees\$platformScope"
        $packagesRoot = Join-Path $buildRoot "vcpkg_packages\$platformScope"

        Write-Host "Installing RedTilde dependencies ($triplet)" -ForegroundColor Cyan
        $arguments = @(
            'install',
            "--triplet=$triplet",
            "--x-manifest-root=$repoRoot",
            "--x-install-root=$installRoot",
            "--x-buildtrees-root=$buildTreesRoot",
            "--x-packages-root=$packagesRoot",
            "--downloads-root=$downloadsRoot",
            "--overlay-triplets=$overlayDirectory",
            '--binarysource=clear'
        )
        & $vcpkg @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "vcpkg install failed for $triplet with exit code $LASTEXITCODE."
        }
    }
} finally {
    [Environment]::SetEnvironmentVariable('VCPKG_DISABLE_METRICS', $previousMetrics, 'Process')
    [Environment]::SetEnvironmentVariable('VCPKG_DOWNLOADS', $previousDownloads, 'Process')
}

Write-Host 'vcpkg dependencies are ready.' -ForegroundColor Green
