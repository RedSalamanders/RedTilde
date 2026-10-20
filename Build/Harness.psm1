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
            throw "Owned harness process timed out after $TimeoutSeconds seconds."
        }
        [IO.File]::WriteAllText($LogPrefix+'.stdout.log',$stdout.GetAwaiter().GetResult())
        [IO.File]::WriteAllText($LogPrefix+'.stderr.log',$stderr.GetAwaiter().GetResult())
        return $process.ExitCode
    } finally { $process.Dispose() }
}
Export-ModuleMember -Function Get-RedTildeSourceIdentity,Invoke-RedTildeHarness
