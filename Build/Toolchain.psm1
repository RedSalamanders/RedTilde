Set-StrictMode -Version Latest

function Get-RedTildeMSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2026 Desktop development with C++.' }
    $installation = & $vswhere -latest -prerelease -products '*' -version '[18.0,19.0)' -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $installation) { throw 'Visual Studio 2026 with MSBuild was not found.' }
    $nativeHost=if([Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq 'Arm64'){'arm64'}else{'amd64'}
    $path = Join-Path $installation ("MSBuild/Current/Bin/$nativeHost/MSBuild.exe")
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

function Get-RedTildeSdkBuildArguments {
    # Product-specific property names and the same evaluated/native invocation
    # are owned here; the shared engine owns SDK authenticity and ABI admission.
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)]$SdkInput)
    if($SdkInput.Closed -or $SdkInput.Evidence.recipe -cne 'PinnedQualifiedReleaseSdkV2'){throw 'Tilde SDK snapshots require an active qualified v2 reader.'}
    $engine=Join-Path $Root '.ci-engine'
    foreach($path in @($Root,$engine,$SdkInput.SdkRoot,$SdkInput.LeasePath)){if($path -match '[%;"\r\n]'){throw 'SDK paths contain unsupported MSBuild delimiters.'}}
    $hostTool=[Environment]::GetEnvironmentVariable('PreferredToolArchitecture','Process')
    if(-not $hostTool){$hostTool=[Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()}
    if($hostTool -cnotin @('x86','x64','arm64')){throw 'Unsupported SDK compiler host.'}
    $arguments=@("/p:RedTildeDxUiSdkRoot=$($SdkInput.SdkRoot)","/p:CiSdkRoot=$($SdkInput.SdkRoot)","/p:CiSdkProjectRoot=$Root","/p:CiSdkLeasePath=$($SdkInput.LeasePath)","/p:RedTildeCiEngineRoot=$engine","/p:PreferredToolArchitecture=$hostTool")
    if($SdkInput.Evidence.recipe -ceq 'PinnedQualifiedReleaseSdkV2'){$arguments+=@('/p:CiSdkContract=QualifiedAbiV2')}
    return $arguments
}
Export-ModuleMember -Function Get-RedTildeSdkBuildArguments

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
    # Native MSBuild has an extra host directory; derive the installation by its
    # MSBuild boundary rather than a fixed number of parents.
    $installation=$msbuild -replace '(?i)[\\/]MSBuild[\\/].*$',''
    foreach($relative in @('VC/Tools/Llvm/x64/bin/clang-format.exe','VC/Tools/Llvm/ARM64/bin/clang-format.exe','VC/Tools/Llvm/bin/clang-format.exe')){
        $candidate=Join-Path $installation $relative
        if(Test-Path -LiteralPath $candidate -PathType Leaf){return $candidate}
    }
    throw 'Install the Visual Studio C++ Clang tools component or put clang-format.exe on PATH.'
}
Export-ModuleMember -Function Get-RedTildeClangFormat
