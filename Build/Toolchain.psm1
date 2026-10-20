Set-StrictMode -Version Latest

function Get-RedTildeMSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2026 Desktop development with C++.' }
    $installation = & $vswhere -latest -prerelease -products '*' -version '[18.0,19.0)' -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $installation) { throw 'Visual Studio 2026 with MSBuild was not found.' }
    $path = Join-Path $installation 'MSBuild/Current/Bin/MSBuild.exe'
    if (-not (Test-Path -LiteralPath $path)) { throw "MSBuild is missing: $path" }
    return $path
}

function Assert-RedTildeOutputNotRunning {
    param([Parameter(Mandatory)][string] $OutputDirectory)
    $root = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\') + '\'
    foreach ($process in @(Get-Process -Name RedTilde -ErrorAction SilentlyContinue)) {
        if ($process.Path -and $process.Path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
            throw "RedTilde is running from '$root'. Close that instance before building."
        }
    }
}

Export-ModuleMember -Function Get-RedTildeMSBuild, Assert-RedTildeOutputNotRunning
