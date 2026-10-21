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

function Get-RedTildeClangFormat {
    $command=Get-Command clang-format.exe -CommandType Application -ErrorAction SilentlyContinue|Select-Object -First 1
    if($command){return $command.Source}
    $msbuild=Get-RedTildeMSBuild
    $installation=([IO.FileInfo]$msbuild).Directory.Parent.Parent.Parent.FullName
    foreach($relative in @('VC/Tools/Llvm/x64/bin/clang-format.exe','VC/Tools/Llvm/ARM64/bin/clang-format.exe','VC/Tools/Llvm/bin/clang-format.exe')){
        $candidate=Join-Path $installation $relative
        if(Test-Path -LiteralPath $candidate -PathType Leaf){return $candidate}
    }
    throw 'Install the Visual Studio C++ Clang tools component or put clang-format.exe on PATH.'
}
Export-ModuleMember -Function Get-RedTildeClangFormat
