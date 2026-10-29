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

function Get-RedTildePinnedBuildArguments {
    param([Parameter(Mandatory)][ValidateSet('Debug','Release','ASan Debug')][string]$Configuration,
          [Parameter(Mandatory)]$Identity)
    # Direct builds inside the SDK do not import this consumer's late policy targets.
    # Use the receipt-verified dependency identity so helper project references cannot
    # replace a policy-fingerprinted archive using SDK defaults or another toolset.
    if($Identity.releaseOptimizationPolicy -cne 'Gy-REF-GL-LTCG-full-v1' -or
        $Identity.vcToolsVersion -cnotmatch '^\d+\.\d+\.\d+$' -or $Identity.toolset -cnotmatch '^v\d+$' -or
        $Identity.windowsSdkVersion -cnotmatch '^\d+\.\d+\.\d+\.\d+$' -or
        $Identity.preferredToolArchitecture -cnotin @('x86','x64','arm64')){
        throw 'A verified pinned dependency compiler and Release policy identity is required.'
    }
    $wholeProgram=if($Configuration -eq 'Release'){'true'}else{'false'}
    return @("/p:WholeProgramOptimization=$wholeProgram","/p:VCToolsVersion=$($Identity.vcToolsVersion)",
        "/p:PlatformToolset=$($Identity.toolset)","/p:WindowsTargetPlatformVersion=$($Identity.windowsSdkVersion)",
        "/p:PreferredToolArchitecture=$($Identity.preferredToolArchitecture)",
        "/p:ForceImportAfterCppTargets=$(Join-Path $PSScriptRoot 'PinnedDxUiPolicy.targets')")
}
Export-ModuleMember -Function Get-RedTildePinnedBuildArguments

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
