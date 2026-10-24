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
Export-ModuleMember -Function Get-RedTildeSourceIdentity,Invoke-RedTildeHarness,Assert-RedTildeTestArtifact
