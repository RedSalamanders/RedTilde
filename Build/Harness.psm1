Set-StrictMode -Version Latest

function Get-RedTildeSourceIdentity {
    param([Parameter(Mandatory)][string] $Root)
    $files=@(& git -c core.quotepath=false -C $Root ls-files --cached --others --exclude-standard) | Where-Object { $_ -notlike 'Measurements/*' } | Sort-Object -Unique
    $entries=foreach ($file in $files) {
        $path=Join-Path $Root $file
        if (Test-Path -LiteralPath $path -PathType Leaf) { $file + ':' + (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    }
    $bytes=[Text.Encoding]::UTF8.GetBytes(($entries -join "`n"))
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}

function Invoke-RedTildeHarness {
    param([Parameter(Mandatory)][string] $Executable,[Parameter(Mandatory)][string[]] $Arguments,
          [Parameter(Mandatory)][string] $LogPrefix,[int] $TimeoutSeconds=90)
    $start=[Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true
    $start.RedirectStandardError=$true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $process=[Diagnostics.Process]::new()
    $process.StartInfo=$start
    try {
        if (-not $process.Start()) { throw 'Could not start the owned harness process.' }
        $stdout=$process.StandardOutput.ReadToEndAsync()
        $stderr=$process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutSeconds*1000)) {
            $process.Kill($true)
            $process.WaitForExit()
            [IO.File]::WriteAllText($LogPrefix+'.stdout.log',$stdout.GetAwaiter().GetResult())
            [IO.File]::WriteAllText($LogPrefix+'.stderr.log',$stderr.GetAwaiter().GetResult())
            throw "Owned harness process timed out after $TimeoutSeconds seconds."
        }
        [IO.File]::WriteAllText($LogPrefix+'.stdout.log',$stdout.GetAwaiter().GetResult())
        [IO.File]::WriteAllText($LogPrefix+'.stderr.log',$stderr.GetAwaiter().GetResult())
        return $process.ExitCode
    } finally { $process.Dispose() }
}
function Assert-RedTildeTestArtifact {
    param([Parameter(Mandatory)][string] $Root,
          [Parameter(Mandatory)][string] $Platform,
          [Parameter(Mandatory)][string] $Configuration)
    $executable=Join-Path $Root ".build/$Platform/$Configuration/RedTilde.exe"
    $receipt=Join-Path $Root ".build/$Platform/$Configuration/build-receipt.json"
    if (-not (Test-Path -LiteralPath $receipt -PathType Leaf)) { throw 'No successful build identity exists. Run build.ps1 before testing this artifact.' }
    $built=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    $source=Get-RedTildeSourceIdentity -Root $Root
    if (-not $built.passed -or $built.sourceIdentity -cne $source -or $built.platform -cne $Platform -or
        $built.configuration -cne $Configuration -or $built.binarySha256 -cne (Get-FileHash -LiteralPath $executable).Hash) {
        throw 'The existing executable does not match the current source/profile/build receipt. Rebuild it before testing.'
    }
    Import-Module (Join-Path $Root 'Build/ScopedTesting.psm1') -Force
    if (-not $built.PSObject.Properties['artifactIdentity'] -or $built.artifactIdentity -cne (Get-ScopedArtifactIdentity $Root $Platform $Configuration)) {
        throw 'The complete executable/runtime closure does not match its successful build. Rebuild before testing.'
    }
    [pscustomobject]@{Executable=$executable;Build=$built;SourceIdentity=$source;ReceiptSha256=(Get-FileHash -LiteralPath $receipt).Hash}
}
function Get-RedTildeDependencyPublicHeaders {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)]$Dependency)
    $commit=[string]$Dependency.Identity.commit
    $fingerprint=[string]$Dependency.Fingerprint
    $platform=[string]$Dependency.Identity.platform
    if($commit -cnotmatch '^[0-9a-f]{40}$' -or $fingerprint -cnotmatch '^[0-9a-fA-F]{64}$' -or $platform -cnotin @('x64','ARM64')) {
        throw 'Exact dependency commit, fingerprint and platform are required for public-header evidence.'
    }
    $include=Join-Path $Root ".build/dependencies/DxUi/source/$commit/include"
    $triplet=if($platform -ceq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
    $packageInclude=Join-Path $Root ".build/dependencies/DxUi/$fingerprint/vcpkg_installed/$platform/$triplet/include"
    if(-not (Test-Path -LiteralPath $include -PathType Container)) {throw 'Pinned public-header source is missing.'}
    if(-not (Test-Path -LiteralPath $packageInclude -PathType Container)) {throw 'Restored package include directory is missing.'}
    $sdk=Split-Path $include -Parent
    $head=@(& git -C $sdk rev-parse HEAD)
    if($LASTEXITCODE -ne 0 -or $head.Count -ne 1 -or $head[0] -cne $commit) {throw 'Public-header checkout differs from the compiled dependency commit.'}
    $dirty=@(& git -C $sdk status --porcelain --untracked-files=all -- include)
    if($LASTEXITCODE -ne 0 -or $dirty.Count) {throw 'Public-header checkout has uncommitted changes.'}
    $ignored=@(& git -C $sdk ls-files --others --ignored --exclude-standard -- include)
    if($LASTEXITCODE -ne 0 -or $ignored.Count) {throw 'Public-header checkout has ignored files in the compiled source include tree.'}
    function Assert-NoReparsePath([string]$Base,[string]$Target) {
        $basePath=[IO.Path]::GetFullPath($Base).TrimEnd('\','/')
        $targetPath=[IO.Path]::GetFullPath($Target)
        $relative=[IO.Path]::GetRelativePath($basePath,$targetPath)
        if($relative -ceq '..' -or $relative.StartsWith('..'+[IO.Path]::DirectorySeparatorChar,[StringComparison]::Ordinal) -or
            [IO.Path]::IsPathRooted($relative)) {throw 'Dependency header path escapes its repository root.'}
        $current=$basePath
        if((Get-Item -LiteralPath $current -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw 'Dependency header evidence cannot follow a repository-root reparse point.'
        }
        foreach($part in @($relative -split '[\\/]')) {
            if(-not $part -or $part -ceq '.') {continue}
            $current=Join-Path $current $part
            $item=Get-Item -LiteralPath $current -Force -ErrorAction Stop
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {throw 'Dependency header evidence cannot follow reparse points.'}
        }
    }
    function Get-HeaderInventory([string]$Base,[string]$Prefix) {
        Assert-NoReparsePath $Root $Base
        $stack=[Collections.Generic.Stack[string]]::new();$stack.Push($Base)
        $files=[Collections.Generic.List[object]]::new()
        $paths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        while($stack.Count) {
            $directory=$stack.Pop()
            foreach($entry in Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop) {
                if($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) {throw 'Dependency header evidence cannot follow reparse points.'}
                if($entry.PSIsContainer) {$stack.Push($entry.FullName);continue}
                $relative=[IO.Path]::GetRelativePath($Base,$entry.FullName).Replace('\','/')
                $path=$Prefix+$relative
                if(-not $paths.Add($path)) {throw 'Dependency header paths must be unique without regard to case.'}
                $files.Add([ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $entry.FullName).Hash})
            }
        }
        $ordered=@($files | Sort-Object path -CaseSensitive)
        if(-not $ordered.Count) {throw "Dependency header inventory is empty: $Prefix"}
        $rows=[string[]]@($ordered | ForEach-Object {$_.path+':'+$_.sha256})
        [Array]::Sort($rows,[StringComparer]::Ordinal)
        return [ordered]@{sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($rows -join "`n"))));files=$ordered}
    }
    $sourceHeaders=Get-HeaderInventory $include 'include/'
    $packageHeaders=Get-HeaderInventory $packageInclude 'package-include/'
    $files=@($sourceHeaders.files)+@($packageHeaders.files)
    $rows=[string[]]@($files | ForEach-Object {$_.path+':'+$_.sha256})
    [Array]::Sort($rows,[StringComparer]::Ordinal)
    $hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($rows -join "`n"))))
    [ordered]@{schemaVersion=2;sha256=$hash;sourceSha256=$sourceHeaders.sha256;packageSha256=$packageHeaders.sha256;files=$files}
}
Export-ModuleMember -Function Get-RedTildeSourceIdentity,Invoke-RedTildeHarness,Assert-RedTildeTestArtifact,Get-RedTildeDependencyPublicHeaders
