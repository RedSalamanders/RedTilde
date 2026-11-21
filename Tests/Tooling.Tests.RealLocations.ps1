<#
.SYNOPSIS
Compiler-free gate proving Tools/Test-RealLocations.ps1 fails closed before writing outside declared selftest roots.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$scriptPath = Join-Path $repoRoot 'Tools/Test-RealLocations.ps1'
if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
    throw "Missing real-location harness script: $scriptPath"
}

. $scriptPath

$testRunId = [guid]::NewGuid().ToString('N')
$fixtureRoot = Join-Path $repoRoot ".build/real-location-tests/$testRunId"
$checks = 0

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
    $script:checks++
}

function Assert-Refused {
    param([scriptblock]$Body, [string]$Description)
    $threw = $false
    try {
        $null = & $Body
    } catch {
        $threw = $true
    }
    if (-not $threw) {
        throw "Expected fail-closed refusal for: $Description"
    }
    $script:checks++
}

function Write-Utf8NoBom {
    param([string]$Path, [string]$Text)
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent $Path))
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

[void][IO.Directory]::CreateDirectory($fixtureRoot)
$junctionPaths = [Collections.Generic.List[string]]::new()

try {
    # 1. Manifest path rejections
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath '' } 'empty manifest path'
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath 'Tests\manifest.json' } 'relative manifest path'
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath (Join-Path $fixtureRoot 'missing.json') } 'non-existent manifest path'
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath (Join-Path $fixtureRoot '..\manifest.json') } 'dot-dot manifest path'

    $badSchemaManifest = Join-Path $fixtureRoot 'bad-schema.json'
    Write-Utf8NoBom $badSchemaManifest '{"schema":"unknown.v1","resources":[]}'
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath $badSchemaManifest } 'unsupported manifest schema'

    $dupIdManifest = Join-Path $fixtureRoot 'dup-id.json'
    $ownedSelftest = Join-Path $fixtureRoot 'selftest'
    [void][IO.Directory]::CreateDirectory($ownedSelftest)
    $dupJson = @{
        schema    = 'redtilde.real-locations.v1'
        resources = @(
            @{ id = 'dup'; kind = 'onedrive-selftest'; enabled = $true; root = $ownedSelftest },
            @{ id = 'dup'; kind = 'onedrive-selftest'; enabled = $false; root = $ownedSelftest }
        )
    } | ConvertTo-Json -Depth 6
    Write-Utf8NoBom $dupIdManifest $dupJson
    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath $dupIdManifest } 'duplicate resource id'

    # 2. Drive roots
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path 'C:\' -Kind 'onedrive-selftest' } 'drive root C:\'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path 'D:\' -Kind 'onedrive-selftest' } 'drive root D:\'

    # 3. User profile, Desktop, Documents, undeclared OneDrive
    $userProfile = if ($env:USERPROFILE) { $env:USERPROFILE } else { 'C:\Users\eric' }
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path $userProfile -Kind 'onedrive-selftest' } '%USERPROFILE%'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path (Join-Path $userProfile 'Desktop') -Kind 'onedrive-selftest' } 'Desktop folder'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path (Join-Path $userProfile 'Documents') -Kind 'onedrive-selftest' } 'Documents folder'
    $desktopReal = [Environment]::GetFolderPath('Desktop')
    if (-not [string]::IsNullOrWhiteSpace($desktopReal)) {
        Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path (Join-Path $desktopReal 'selftest') -Kind 'onedrive-selftest' } 'Desktop\selftest'
    }
    $docsReal = [Environment]::GetFolderPath('MyDocuments')
    if (-not [string]::IsNullOrWhiteSpace($docsReal)) {
        Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path (Join-Path $docsReal 'selftest') -Kind 'onedrive-selftest' } 'Documents\selftest'
    }
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path 'C:\Users\eric\OneDrive' -Kind 'onedrive-selftest' } 'undeclared C:\Users\eric\OneDrive'

    # 4. Non-selftest share \\rubygloom\Multimedia and shallow UNC roots
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path '\\rubygloom\Multimedia' -Kind 'smb-share' } '\\rubygloom\Multimedia'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path '\\rubygloom\Multimedia\Subdir' -Kind 'smb-share' } '\\rubygloom\Multimedia\Subdir'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path '\\server\share' -Kind 'smb-share' } 'shallow 2-segment UNC \\server\share'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path '\\wsl.localhost\Ubuntu' -Kind 'wsl-selftest' } 'shallow 2-segment WSL root'

    # 5. Relative, traversal, oversized, and control-character paths
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path 'relative\selftest' -Kind 'onedrive-selftest' } 'relative path'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path "$ownedSelftest\..\selftest" -Kind 'onedrive-selftest' } 'dot-dot traversal'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path "$ownedSelftest\." -Kind 'onedrive-selftest' } 'dot segment'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path ($ownedSelftest + '\' + ('a' * 1025)) -Kind 'onedrive-selftest' } 'length > 1024'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path "$ownedSelftest`0" -Kind 'onedrive-selftest' } 'embedded NUL'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path "$ownedSelftest`n" -Kind 'onedrive-selftest' } 'embedded newline'

    # 6. Reparse-point / junction manifest and root leaf strictly inside .build/real-location-tests/<guid>/
    $realTarget = Join-Path $fixtureRoot 'junction-target'
    $junctionManifestDir = Join-Path $fixtureRoot 'junction-manifest-dir'
    $junctionSelftest = Join-Path $fixtureRoot 'junction-selftest'
    [void][IO.Directory]::CreateDirectory($realTarget)
    $realManifestInTarget = Join-Path $realTarget 'manifest.json'
    Write-Utf8NoBom $realManifestInTarget '{"schema":"redtilde.real-locations.v1","resources":[]}'

    if ([OperatingSystem]::IsWindows()) {
        $null = & cmd.exe /c mklink /J "`"$junctionManifestDir`"" "`"$realTarget`"" 2>&1
        if ($LASTEXITCODE -ne 0) { throw 'Failed to create owned test junction for manifest test.' }
        $junctionPaths.Add($junctionManifestDir)

        $null = & cmd.exe /c mklink /J "`"$junctionSelftest`"" "`"$ownedSelftest`"" 2>&1
        if ($LASTEXITCODE -ne 0) { throw 'Failed to create owned test junction for root test.' }
        $junctionPaths.Add($junctionSelftest)
    } else {
        $null = [IO.Directory]::CreateSymbolicLink($junctionManifestDir, $realTarget)
        $junctionPaths.Add($junctionManifestDir)
        $null = [IO.Directory]::CreateSymbolicLink($junctionSelftest, $ownedSelftest)
        $junctionPaths.Add($junctionSelftest)
    }

    Assert-Refused { Test-RedTildeRealLocationManifest -ManifestPath (Join-Path $junctionManifestDir 'manifest.json') } 'manifest under junction'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path $junctionSelftest -Kind 'onedrive-selftest' } 'junction selftest root leaf'
    Assert-Refused { Assert-RedTildeAllowedSelftestRoot -Path (Join-Path $junctionManifestDir 'selftest') -Kind 'onedrive-selftest' } 'selftest under junction ancestor'

    # Manifest containing forbidden root fails closed before run
    $badRootManifest = Join-Path $fixtureRoot 'bad-root-manifest.json'
    Write-Utf8NoBom $badRootManifest (@{
            schema    = 'redtilde.real-locations.v1'
            resources = @(
                @{ id = 'bad.media'; kind = 'smb-share'; enabled = $true; root = '\\rubygloom\Multimedia' }
            )
        } | ConvertTo-Json -Depth 6)
    Assert-Refused { Invoke-RedTildeRealLocationRun -ManifestPath $badRootManifest -DryRun } 'manifest with \\rubygloom\Multimedia'

    # Unreachable selftest root fails closed (no fallback)
    $unreachableManifest = Join-Path $fixtureRoot 'unreachable-manifest.json'
    $missingSelftest = Join-Path $fixtureRoot 'does-not-exist\selftest'
    Write-Utf8NoBom $unreachableManifest (@{
            schema    = 'redtilde.real-locations.v1'
            resources = @(
                @{ id = 'missing.local'; kind = 'onedrive-selftest'; enabled = $true; root = $missingSelftest }
            )
        } | ConvertTo-Json -Depth 6)
    Assert-Refused { Invoke-RedTildeRealLocationRun -ManifestPath $unreachableManifest -DryRun } 'unreachable selftest root'

    # 7. Compatible red-salamander.test-machine-resources.v1 validation (with disabled SMB root)
    $compatManifest = Join-Path $fixtureRoot 'compat-manifest.json'
    Write-Utf8NoBom $compatManifest (@{
            schema    = 'red-salamander.test-machine-resources.v1'
            resources = @(
                @{ id = 'connection.ftp.primary'; kind = 'connection-profile'; enabled = $true; profile = 'unused' },
                @{ id = 'path.smb.primary'; kind = 'smb-share'; enabled = $false; root = '\\rubygloom\RedSalamander-SelfTest\selftest' }
            )
        } | ConvertTo-Json -Depth 6)
    $compatParsed = Test-RedTildeRealLocationManifest -ManifestPath $compatManifest
    Assert-True ($compatParsed.Schema -ceq 'red-salamander.test-machine-resources.v1' -and $compatParsed.Targets.Count -eq 0) 'compatible schema parsed without false positives'

    # 8. Positive -DryRun and direct CLI invocation against owned .build selftest fixture
    $validManifest = Join-Path $fixtureRoot 'valid-manifest.json'
    Write-Utf8NoBom $validManifest (@{
            schema    = 'redtilde.real-locations.v1'
            resources = @(
                @{ id = 'local.owned.selftest'; kind = 'onedrive-selftest'; enabled = $true; root = $ownedSelftest }
            )
        } | ConvertTo-Json -Depth 6)

    $dryResults = @(Invoke-RedTildeRealLocationRun -ManifestPath $validManifest -DryRun)
    Assert-True ($dryResults.Count -eq 1 -and $dryResults[0].status -ceq 'DRY_RUN_VERIFIED') 'DryRun verified owned selftest root'
    Assert-True (@(Get-ChildItem -LiteralPath $ownedSelftest -Force).Count -eq 0) 'DryRun created zero files inside selftest root'

    $pwshBinary = Join-Path $PSHOME $(if ([OperatingSystem]::IsWindows()) { 'pwsh.exe' } else { 'pwsh' })
    $cliOutput = (& $pwshBinary -NoProfile -File $scriptPath -ManifestPath $validManifest -DryRun | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "CLI DryRun exited with $LASTEXITCODE"
    }
    $cliParsed = $cliOutput | ConvertFrom-Json
    Assert-True ($cliParsed.status -ceq 'DRY_RUN_VERIFIED') 'CLI DryRun output verified'

    # 9. Controlled real capture execution inside owned .build/.../selftest if a built RedTilde.exe is present
    $candidateExe = $null
    $candidateConfig = 'Release'
    foreach ($cfg in @('Release', 'Debug')) {
        $probe = Join-Path $repoRoot ".build/x64/$cfg/RedTilde.exe"
        if (Test-Path -LiteralPath $probe -PathType Leaf) {
            $candidateExe = $probe
            $candidateConfig = $cfg
            break
        }
    }
    if ($null -ne $candidateExe) {
        $liveResults = @(Invoke-RedTildeRealLocationRun -ManifestPath $validManifest -Configuration $candidateConfig -ExecutablePath $candidateExe)
        Assert-True ($liveResults.Count -eq 1 -and $liveResults[0].status -ceq 'PASSED' -and $liveResults[0].captureByteCount -gt 0) 'Controlled capture run succeeded against owned .build selftest'
        Assert-True (@(Get-ChildItem -LiteralPath $ownedSelftest -Force).Count -eq 0) 'Controlled run cleaned up redtilde-run-<guid> leaf completely'
    }
} finally {
    foreach ($j in $junctionPaths) {
        if (Test-Path -LiteralPath $j) {
            [IO.Directory]::Delete($j, $false)
        }
    }
    Remove-RedTildeDirectoryNoReparse -Path $fixtureRoot
}

Write-Host "PASS: real-location fail-closed contract ($checks checks)"
