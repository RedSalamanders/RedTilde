Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'DxUiPaths.psm1') -Scope Local -ErrorAction Stop

function Get-RedTildeSourceIdentity {
    param([Parameter(Mandatory)][string] $Root)
    $files=@(& git -c core.quotepath=false -C $Root ls-files --cached --others --exclude-standard) | Where-Object { $_ -notlike 'Measurements/*' } | Sort-Object -Unique
    $entries=foreach ($file in $files) {
        $path=Join-Path $Root $file
        if (Test-Path -LiteralPath $path -PathType Leaf) { $file + ':' + (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    }
    if(Test-Path -LiteralPath (Join-Path $Root '.ci/project.json') -PathType Leaf){
        Import-Module (Join-Path $Root '.ci-engine/Tools/Modules/Ci.SourceInputs.psm1') -ErrorAction Stop
        $engineInputs=Get-CiPinnedEngineInputsIdentity -Root $Root
        $entries=@($entries)+@("engine_inputs`0$engineInputs")
    }
    $bytes=[Text.Encoding]::UTF8.GetBytes(($entries -join "`n"))
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}

function Get-RedTildeDependencyResolutionSnapshot {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$Platform,[Parameter(Mandatory)]$Dependency)
    $identity=$Dependency.Identity
    $fingerprint=[string]$Dependency.Fingerprint
    $sourceRoot=[IO.Path]::GetFullPath((Join-Path $Root ".build/dependencies/DxUi/source/$($identity.commit.Substring(0,16))"))
    $outputRoot=[IO.Path]::GetFullPath((Join-Path $Root ".build/dependencies/DxUi/$($fingerprint.Substring(0,16))"))
    $lockFile=[IO.Path]::GetFullPath((Join-Path $Root 'Dependencies/DxUi.lock.json'))
    $propsPath=Join-Path $Root ".build/dependencies/DxUi/DxUi.resolved.$Platform.props"
    if(-not(Test-Path -LiteralPath $propsPath -PathType Leaf)){throw 'Resolved DxUi consumer properties are missing.'}
    $propsItem=Get-Item -LiteralPath $propsPath -Force -ErrorAction Stop
    if($propsItem.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Resolved DxUi consumer properties cannot be a reparse point.'}
    $xml=[Xml.XmlDocument]::new()
    $settings=[Xml.XmlReaderSettings]::new();$settings.DtdProcessing=[Xml.DtdProcessing]::Prohibit;$settings.XmlResolver=$null
    $reader=[Xml.XmlReader]::Create($propsPath,$settings)
    try{$xml.Load($reader)}finally{$reader.Dispose()}
    function Read-ResolvedProperty([Xml.XmlDocument]$Document,[string]$Name){
        $nodes=$Document.SelectNodes("/*[local-name()='Project']/*[local-name()='PropertyGroup']/*[local-name()='$Name']")
        if($nodes.Count -ne 1){throw "Resolved DxUi properties must define $Name exactly once."}
        return [string]$nodes[0].InnerText
    }
    $values=[ordered]@{}
    foreach($name in @('DxUiRoot','DxUiConsumerOutputRoot','DxUiConsumerLockFile','DxUiConsumerToolset',
        'DxUiConsumerVCToolsVersion','DxUiConsumerSdkVersion','DxUiConsumerPreferredToolArchitecture')){
        $values[$name]=Read-ResolvedProperty $xml $name
        if([string]::IsNullOrWhiteSpace([string]$values[$name])){throw "Resolved DxUi property is empty: $name"}
    }
    function Normalize-ResolvedPath([string]$Path){[IO.Path]::GetFullPath($Path.TrimEnd('\','/')).TrimEnd('\','/')}
    if((Normalize-ResolvedPath ([string]$values.DxUiRoot)) -cne $sourceRoot.TrimEnd('\') -or
        (Normalize-ResolvedPath ([string]$values.DxUiConsumerOutputRoot)) -cne $outputRoot.TrimEnd('\') -or
        (Normalize-ResolvedPath ([string]$values.DxUiConsumerLockFile)) -cne $lockFile.TrimEnd('\')){
        throw 'Resolved DxUi properties redirect the pinned source, package root or consumer lock.'
    }
    if([string]$values.DxUiConsumerToolset -cne [string]$identity.toolset -or
        [string]$values.DxUiConsumerVCToolsVersion -cne [string]$identity.vcToolsVersion -or
        [string]$values.DxUiConsumerSdkVersion -cne [string]$identity.windowsSdkVersion -or
        [string]$values.DxUiConsumerPreferredToolArchitecture -cne [string]$identity.preferredToolArchitecture){
        throw 'Resolved DxUi compiler properties differ from the restored dependency identity.'
    }
    $triplet=if($Platform -ceq 'ARM64'){'arm64-windows'}else{'x64-windows'}
    return [ordered]@{
        sourceRoot=$sourceRoot;outputRoot=$outputRoot;lockFile=$lockFile;triplet=$triplet
        disableStlAnnotations=([string]$identity.disableStlAnnotations).ToLowerInvariant()
        resolvedPropsSha256=(Get-FileHash -LiteralPath $propsPath -Algorithm SHA256).Hash
        resolvedProperties=$values
    }
}

function Get-RedTildeBuildToolchainSnapshot {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$MSBuildPath,
          [Parameter(Mandatory)][string]$Platform,[Parameter(Mandatory)][string]$Configuration,
          [Parameter(Mandatory)]$DependencyResolution)
    $project=Join-Path $Root 'RedTilde/RedTilde.vcxproj'
    $propertyNames=@('OutDir','IntDir','PlatformToolset','VCToolsInstallDir','VCToolsVersion',
        'WindowsSdkDir','WindowsTargetPlatformVersion','PreferredToolArchitecture','ExecutablePath',
        'CLToolPath','CLToolExe','LinkToolPath','LinkToolExe','ResourceCompileToolPath','ResourceCompileToolExe',
        'DxUiRoot','DxUiResolvedRoot','DxUiConsumerOutputRoot','DxUiConsumerLockFile','DxUiDisableStlAnnotations',
        'DxUiConsumerToolset','DxUiConsumerVCToolsVersion','DxUiConsumerSdkVersion','DxUiConsumerPreferredToolArchitecture','DxUiConsumerTriplet')
    $arguments=@($project,'/nologo','/v:quiet',('-getProperty:'+($propertyNames -join ',')),"/p:Platform=$Platform","/p:Configuration=$Configuration")
    $output=@(& $MSBuildPath @arguments)
    if($LASTEXITCODE -ne 0){throw 'MSBuild property evaluation failed; build inputs cannot be attested.'}
    try{$evaluation=($output -join "`n")|ConvertFrom-Json -AsHashtable -ErrorAction Stop}
    catch [System.Management.Automation.RuntimeException]{throw 'MSBuild returned unrecognized build-context properties.'}
    if(-not $evaluation.ContainsKey('Properties') -or $evaluation.Properties -isnot [System.Collections.IDictionary]){throw 'MSBuild property evaluation did not return its Properties object.'}
    $properties=$evaluation.Properties
    foreach($name in @('OutDir','IntDir','PlatformToolset','VCToolsInstallDir','VCToolsVersion','WindowsSdkDir',
        'WindowsTargetPlatformVersion','PreferredToolArchitecture','ExecutablePath','DxUiRoot','DxUiResolvedRoot',
        'DxUiConsumerOutputRoot','DxUiConsumerLockFile','DxUiDisableStlAnnotations','DxUiConsumerToolset',
        'DxUiConsumerVCToolsVersion','DxUiConsumerSdkVersion','DxUiConsumerPreferredToolArchitecture','DxUiConsumerTriplet')){
        if(-not $properties.ContainsKey($name) -or [string]::IsNullOrWhiteSpace([string]$properties[$name])){throw "MSBuild build-context property is unavailable: $name"}
    }
    $evaluatedPaths=[ordered]@{DxUiRoot=[string]$properties.DxUiRoot;DxUiResolvedRoot=[string]$properties.DxUiResolvedRoot;
        DxUiConsumerOutputRoot=[string]$properties.DxUiConsumerOutputRoot;DxUiConsumerLockFile=[string]$properties.DxUiConsumerLockFile}
    $expectedPaths=[ordered]@{DxUiRoot=[string]$DependencyResolution.sourceRoot;DxUiResolvedRoot=[string]$DependencyResolution.sourceRoot;
        DxUiConsumerOutputRoot=[string]$DependencyResolution.outputRoot;DxUiConsumerLockFile=[string]$DependencyResolution.lockFile}
    foreach($name in $evaluatedPaths.Keys){
        $actual=[IO.Path]::GetFullPath($evaluatedPaths[$name].TrimEnd('\','/')).TrimEnd('\','/')
        $expected=[IO.Path]::GetFullPath($expectedPaths[$name].TrimEnd('\','/')).TrimEnd('\','/')
        if($actual -ine $expected){throw "MSBuild resolved DxUi property redirects $name outside its verified pin."}
    }
    $resolvedRoot=[IO.Path]::GetFullPath([string]$properties.DxUiResolvedRoot.TrimEnd('\','/')).TrimEnd('\','/')
    if($resolvedRoot -ine [string]$DependencyResolution.sourceRoot.TrimEnd('\') -or
        [string]$properties.DxUiDisableStlAnnotations -cne [string]$DependencyResolution.disableStlAnnotations -or
        [string]$properties.DxUiConsumerToolset -cne [string]$properties.PlatformToolset -or
        [string]$properties.DxUiConsumerVCToolsVersion -cne [string]$properties.VCToolsVersion -or
        [string]$properties.DxUiConsumerSdkVersion -cne [string]$properties.WindowsTargetPlatformVersion -or
        [string]$properties.DxUiConsumerPreferredToolArchitecture -cne [string]$properties.PreferredToolArchitecture -or
        [string]$properties.DxUiConsumerTriplet -cne [string]$DependencyResolution.triplet){
        throw 'MSBuild-resolved DxUi properties differ from the pinned consumer context.'
    }

    $toolFiles=[ordered]@{}
    foreach($tool in @(
        @{name='cl';directory=[string]$properties.CLToolPath;executable=[string]$properties.CLToolExe;default='cl.exe';owner=[string]$properties.VCToolsInstallDir},
        @{name='link';directory=[string]$properties.LinkToolPath;executable=[string]$properties.LinkToolExe;default='link.exe';owner=[string]$properties.VCToolsInstallDir},
        @{name='rc';directory=[string]$properties.ResourceCompileToolPath;executable=[string]$properties.ResourceCompileToolExe;default='rc.exe';owner=[string]$properties.WindowsSdkDir})){
        $executable=if([string]::IsNullOrWhiteSpace($tool.executable)){$tool.default}else{$tool.executable}
        $toolPath=$null
        if(-not [string]::IsNullOrWhiteSpace($tool.directory)){$toolPath=Join-Path $tool.directory $executable}
        else{
            $searchPaths=if($properties.ExecutablePath -is [array]){@($properties.ExecutablePath)}else{@(([string]$properties.ExecutablePath -split ';')|Where-Object{$_})}
            foreach($searchPath in $searchPaths){
                $candidate=Join-Path ([string]$searchPath) $executable
                if(-not(Test-Path -LiteralPath $candidate -PathType Leaf)){continue}
                $relative=[IO.Path]::GetRelativePath([IO.Path]::GetFullPath($tool.owner),[IO.Path]::GetFullPath($candidate))
                if($relative -ceq '..' -or $relative.StartsWith('..'+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::IsPathRooted($relative)){
                    throw "First MSBuild search-path match for $($tool.name) is outside its selected tool installation."
                }
                $toolPath=$candidate;break
            }
        }
        if(-not $toolPath){throw "MSBuild did not resolve $($tool.name) under its selected tool installation."}
        if(-not(Test-Path -LiteralPath $toolPath -PathType Leaf)){throw "Resolved compiler tool is missing: $($tool.name)"}
        $fullPath=[IO.Path]::GetFullPath($toolPath)
        $relative=[IO.Path]::GetRelativePath([IO.Path]::GetFullPath($tool.owner),$fullPath)
        if($relative -ceq '..' -or $relative.StartsWith('..'+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::IsPathRooted($relative)){throw "Resolved $($tool.name) is outside its selected tool installation."}
        $toolFiles[$tool.name]=[ordered]@{path=$fullPath;sha256=(Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash}
    }
    $msbuildFullPath=[IO.Path]::GetFullPath($MSBuildPath)
    if(-not(Test-Path -LiteralPath $msbuildFullPath -PathType Leaf)){throw 'Resolved MSBuild executable is missing.'}

    $selectedEnvironment=[ordered]@{}
    foreach($name in @('CL','_CL_','LINK','_LINK_','RC','INCLUDE','LIB','LIBPATH','VCToolsVersion','WindowsSDKVersion','WindowsSdkDir')){
        $selectedEnvironment[$name]=[string][Environment]::GetEnvironmentVariable($name)
    }
    $context=[ordered]@{
        platform=$Platform;configuration=$Configuration
        msbuild=[ordered]@{path=$msbuildFullPath;sha256=(Get-FileHash -LiteralPath $msbuildFullPath -Algorithm SHA256).Hash}
        properties=[ordered]@{OutDir=[IO.Path]::GetFullPath([string]$properties.OutDir);IntDir=[IO.Path]::GetFullPath([string]$properties.IntDir);
            PlatformToolset=[string]$properties.PlatformToolset;VCToolsInstallDir=[IO.Path]::GetFullPath([string]$properties.VCToolsInstallDir);
            VCToolsVersion=[string]$properties.VCToolsVersion;PreferredToolArchitecture=[string]$properties.PreferredToolArchitecture;
            ExecutablePathSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes([string]$properties.ExecutablePath)));
            WindowsSdkDir=[IO.Path]::GetFullPath([string]$properties.WindowsSdkDir);
            WindowsTargetPlatformVersion=[string]$properties.WindowsTargetPlatformVersion;
            DxUiRoot=[IO.Path]::GetFullPath([string]$properties.DxUiRoot);DxUiResolvedRootSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes([string]$properties.DxUiResolvedRoot)));
            DxUiConsumerOutputRoot=[IO.Path]::GetFullPath([string]$properties.DxUiConsumerOutputRoot);DxUiConsumerLockFile=[IO.Path]::GetFullPath([string]$properties.DxUiConsumerLockFile);
            DxUiDisableStlAnnotations=[string]$properties.DxUiDisableStlAnnotations;DxUiConsumerToolset=[string]$properties.DxUiConsumerToolset;
            DxUiConsumerVCToolsVersion=[string]$properties.DxUiConsumerVCToolsVersion;DxUiConsumerSdkVersion=[string]$properties.DxUiConsumerSdkVersion;
            DxUiConsumerPreferredToolArchitecture=[string]$properties.DxUiConsumerPreferredToolArchitecture;DxUiConsumerTriplet=[string]$properties.DxUiConsumerTriplet}
        tools=$toolFiles;environment=$selectedEnvironment
    }
    $digestBytes=[Text.Encoding]::UTF8.GetBytes(($context|ConvertTo-Json -Depth 8 -Compress))
    $context['sha256']=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($digestBytes))
    return $context
}

function Assert-RedTildeCanonicalBuildDirectories {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$Platform,
          [Parameter(Mandatory)][string]$Configuration)
    $rootPath=[IO.Path]::GetFullPath($Root)
    if(-not(Test-Path -LiteralPath $rootPath -PathType Container) -or
        (Get-Item -LiteralPath $rootPath -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint){
        throw 'Repository root must be an existing non-reparse directory before build outputs are touched.'
    }
    $paths=[ordered]@{
        output=[IO.Path]::GetFullPath((Join-Path $rootPath ".build/$Platform/$Configuration"))
        intermediate=[IO.Path]::GetFullPath((Join-Path $rootPath ".build/obj/$Platform/$Configuration/RedTilde"))
        dependency=[IO.Path]::GetFullPath((Join-Path $rootPath '.build/dependencies/DxUi'))
    }
    foreach($target in $paths.Values){
        $relative=[IO.Path]::GetRelativePath($rootPath,$target)
        if($relative -ceq '..' -or $relative.StartsWith('..'+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or
            [IO.Path]::IsPathRooted($relative)){throw 'Build output path escapes the repository root.'}
        $current=$rootPath
        foreach($part in @($relative -split '[\\/]')){
            if(-not $part -or $part -ceq '.') {continue}
            $current=Join-Path $current $part
            if(-not(Test-Path -LiteralPath $current)){continue}
            $item=Get-Item -LiteralPath $current -Force -ErrorAction Stop
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Build output or dependency path traverses a reparse point.'}
            if(-not $item.PSIsContainer){throw 'A build output path component is not a directory.'}
        }
    }
    return $paths
}

function Assert-RedTildeOwnedCleanDirectories {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$Platform,
          [Parameter(Mandatory)][string]$Configuration,[Parameter(Mandatory)]$Toolchain)
    $canonical=Assert-RedTildeCanonicalBuildDirectories -Root $Root -Platform $Platform -Configuration $Configuration
    $expectedOutput=$canonical.output.TrimEnd('\')
    $expectedIntermediate=$canonical.intermediate.TrimEnd('\')
    if([string]$Toolchain.properties.OutDir.TrimEnd('\') -cne $expectedOutput -or
        [string]$Toolchain.properties.IntDir.TrimEnd('\') -cne $expectedIntermediate){throw 'MSBuild output/intermediate directories are not the owned profile paths; refusing to clean.'}
}

function Get-RedTildeSdkBuildInputSnapshot {
    # SDK snapshots retain Tilde's before/after compiler and owned-directory
    # transaction. The active shared reader supplies authenticated dependency bytes.
    param([string]$Root,[string]$Platform,[string]$Configuration,[string]$MSBuildPath,$SdkInput)
    if($Configuration -cne 'Release' -or $SdkInput.Closed){throw 'SDK input snapshots require an active Release reader.'}
    Import-Module (Join-Path $Root 'Build/Toolchain.psm1') -Force
    $sdkArguments=@(Get-RedTildeSdkBuildArguments -Root $Root -SdkInput $SdkInput)
    $names=@('OutDir','IntDir','PlatformToolset','VCToolsInstallDir','VCToolsVersion','WindowsSdkDir',
        'WindowsTargetPlatformVersion','PreferredToolArchitecture','VCToolArchitecture','WindowsSDKToolArchitecture',
        'CLToolPath','CLToolExe','LinkToolPath','LinkToolExe','ResourceCompileToolPath','ResourceCompileToolExe',
        'DxUiRoot','DxUiSdkRoot','CiSdkContract')
    $query=@((Join-Path $Root 'RedTilde/RedTilde.vcxproj'),'/nologo','/v:quiet',('-getProperty:'+($names -join ',')),"/p:Platform=$Platform","/p:Configuration=$Configuration")+$sdkArguments
    $output=@(& $MSBuildPath @query)
    if($LASTEXITCODE){throw 'SDK actual-project metadata evaluation failed.'}
    $values=($output -join "`n")|ConvertFrom-Json -AsHashtable -ErrorAction Stop
    if(-not $values.Contains('Properties')){throw 'SDK actual-project metadata has no properties.'}
    $properties=[ordered]@{}
    foreach($name in $names){if(-not $values.Properties.Contains($name)){throw "Missing SDK compiler property: $name"};$properties[$name]=[string]$values.Properties[$name]}
    foreach($name in @('OutDir','IntDir','PlatformToolset','VCToolsInstallDir','VCToolsVersion','WindowsSdkDir','WindowsTargetPlatformVersion','PreferredToolArchitecture','DxUiSdkRoot')){
        if([string]::IsNullOrWhiteSpace($properties[$name])){throw "Empty SDK compiler property: $name"}
    }
    if($properties.DxUiRoot -or -not [IO.Path]::GetFullPath($properties.DxUiSdkRoot).TrimEnd('\','/').Equals([IO.Path]::GetFullPath($SdkInput.SdkRoot).TrimEnd('\','/'),[StringComparison]::OrdinalIgnoreCase)){
        throw 'SDK metadata redirects the verified payload or mixes source inputs.'
    }
    if($SdkInput.Evidence.recipe -ceq 'PinnedQualifiedReleaseSdkV2' -and $properties.CiSdkContract -cne 'QualifiedAbiV2'){throw 'SDK metadata downgraded its verified contract.'}
    $environment=[ordered]@{}
    foreach($name in @('CL','_CL_','LINK','_LINK_','RC','INCLUDE','LIB','LIBPATH','VCToolsVersion','WindowsSDKVersion','WindowsSdkDir')){$environment[$name]=[string][Environment]::GetEnvironmentVariable($name)}
    $hash={param([string]$Text) [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($Text)))}
    $toolchain=[ordered]@{properties=$properties;msbuildSha256=(Get-FileHash -LiteralPath $MSBuildPath).Hash;environment=$environment;dependencyRecipe=$SdkInput.Evidence}
    $toolchain.sha256=& $hash ($toolchain|ConvertTo-Json -Depth 10 -Compress)
    Assert-RedTildeOwnedCleanDirectories -Root $Root -Platform $Platform -Configuration $Configuration -Toolchain $toolchain
    $snapshot=[ordered]@{
        compiledSourceIdentity=(Get-ScopedSourceIdentity $Root -CompiledOnly)
        dependencyIdentitySha256=(& $hash (($SdkInput.Evidence|ConvertTo-Json -Depth 10 -Compress)+$SdkInput.Descriptor.Hash))
        # The authenticated original manifest covers the complete payload, including
        # its packaged public headers. A changed byte is refused by open/close.
        dependencyHeadersSha256=$SdkInput.Evidence.manifestSha256
        dependencyResolvedPropsSha256=(Get-FileHash -LiteralPath (Join-Path $SdkInput.SdkRoot 'DxUi.Sdk.props')).Hash
        compilerContextSha256=$toolchain.sha256
        buildNumber=$(if($env:RedTildeBuildNumber){[int]$env:RedTildeBuildNumber}else{0})
        testsEnabled=($env:RedTildeEnableTests -ne 'false')
        toolchain=$toolchain
    }
    $snapshot.identity=& $hash (@($snapshot.compiledSourceIdentity,$snapshot.dependencyIdentitySha256,$snapshot.dependencyHeadersSha256,
        $snapshot.dependencyResolvedPropsSha256,$snapshot.compilerContextSha256,$Platform,$Configuration,$snapshot.buildNumber,$snapshot.testsEnabled) -join "`n")
    return $snapshot
}

function Get-RedTildeBuildInputSnapshot {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$Platform,
          [Parameter(Mandatory)][string]$Configuration,[Parameter(Mandatory)][string]$MSBuildPath,$SdkInput=$null)
    Import-Module (Join-Path $Root 'Build/ScopedTesting.psm1') -Force
    if($null -ne $SdkInput){return Get-RedTildeSdkBuildInputSnapshot -Root $Root -Platform $Platform -Configuration $Configuration -MSBuildPath $MSBuildPath -SdkInput $SdkInput}
    $dependencyPath=Join-Path $Root ".build/dependencies/DxUi/DxUi.identity.$Platform.json"
    if(-not(Test-Path -LiteralPath $dependencyPath -PathType Leaf)){throw 'Restored DxUi build identity is missing.'}
    $dependency=Get-Content -LiteralPath $dependencyPath -Raw -ErrorAction Stop|ConvertFrom-Json -AsHashtable -ErrorAction Stop
    if(-not $dependency.ContainsKey('Identity') -or -not $dependency.ContainsKey('Fingerprint')){throw 'Restored DxUi build identity is incomplete.'}
    $headerInventory=Get-RedTildeDependencyPublicHeaders -Root $Root -Dependency $dependency
    $dependencyResolution=Get-RedTildeDependencyResolutionSnapshot -Root $Root -Platform $Platform -Dependency $dependency
    $toolchain=Get-RedTildeBuildToolchainSnapshot -Root $Root -MSBuildPath $MSBuildPath -Platform $Platform -Configuration $Configuration -DependencyResolution $dependencyResolution
    Assert-RedTildeOwnedCleanDirectories -Root $Root -Platform $Platform -Configuration $Configuration -Toolchain $toolchain
    $snapshot=[ordered]@{
        compiledSourceIdentity=(Get-ScopedSourceIdentity $Root -CompiledOnly)
        dependencyIdentitySha256=(Get-FileHash -LiteralPath $dependencyPath -Algorithm SHA256).Hash
        dependencyHeadersSha256=$headerInventory.sha256
        dependencyResolvedPropsSha256=$dependencyResolution.resolvedPropsSha256
        compilerContextSha256=$toolchain.sha256
        buildNumber=$(if($env:RedTildeBuildNumber){[int]$env:RedTildeBuildNumber}else{0})
        testsEnabled=($env:RedTildeEnableTests -ne 'false')
        toolchain=$toolchain
    }
    $rows=@($snapshot.compiledSourceIdentity,$snapshot.dependencyIdentitySha256,$snapshot.dependencyHeadersSha256,
        $snapshot.dependencyResolvedPropsSha256,$snapshot.compilerContextSha256,$Platform,$Configuration,
        $snapshot.buildNumber,$snapshot.testsEnabled)
    $snapshot['identity']=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($rows -join "`n"))))
    return $snapshot
}

function Get-RedTildeArtifactIdentity {
    param([Parameter(Mandatory)][string]$Root,[Parameter(Mandatory)][string]$Platform,
          [Parameter(Mandatory)][string]$Configuration)
    Import-Module (Join-Path $Root 'Build/ScopedTesting.psm1') -Force
    return Get-ScopedArtifactIdentity $Root $Platform $Configuration
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
    $canonical=Assert-RedTildeCanonicalBuildDirectories -Root $Root -Platform $Platform -Configuration $Configuration
    $executable=Join-Path $canonical.output 'RedTilde.exe'
    $receipt=Join-Path $Root ".build/$Platform/$Configuration/build-receipt.json"
    if (-not (Test-Path -LiteralPath $receipt -PathType Leaf)) { throw 'No successful build identity exists. Run build.ps1 before testing this artifact.' }
    if((Get-Item -LiteralPath $receipt -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Build receipt cannot be a reparse point.'}
    if((Get-Item -LiteralPath $executable -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Executable artifact cannot be a reparse point.'}
    $built=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    $source=Get-RedTildeSourceIdentity -Root $Root
    if ($built.schemaVersion -ne 2 -or -not $built.passed -or $built.sourceIdentity -cne $source -or $built.platform -cne $Platform -or
        $built.configuration -cne $Configuration -or $built.binarySha256 -cne (Get-FileHash -LiteralPath $executable).Hash) {
        throw 'The existing executable does not match the current source/profile/build receipt. Rebuild it before testing.'
    }
    if (-not $built.PSObject.Properties['artifactIdentity'] -or $built.artifactIdentity -cne (Get-RedTildeArtifactIdentity $Root $Platform $Configuration)) {
        throw 'The complete executable/runtime closure does not match its successful build. Rebuild before testing.'
    }
    Import-Module (Join-Path $Root 'Build/Toolchain.psm1') -Force
    $msbuild=Get-RedTildeMSBuild
    $sdk=$null;$sdkFailure=$null
    try{
    if($built.PSObject.Properties['dependencyRecipe'] -and $built.dependencyRecipe.recipe -cne 'PinnedSource'){
        if($built.dependencyRecipe.recipe -cne 'PinnedQualifiedReleaseSdkV2'){throw 'Unsupported Tilde build SDK recipe.'}
        if(-not $built.PSObject.Properties['sdkInputDescriptor'] -or -not $built.PSObject.Properties['sdkInputDescriptorSha256']){throw 'SDK build receipt has no independent descriptor binding.'}
        Import-Module (Join-Path $Root '.ci-engine/Tools/Modules/Ci.SdkConsumer.psm1')
        $sdk=Open-CiApplicationSdkInput -ProjectRoot $Root -Descriptor $built.sdkInputDescriptor -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild
        if($null -eq $sdk -or $sdk.Descriptor.Hash -cne $built.sdkInputDescriptorSha256 -or
            $sdk.Evidence.inputKey -cne $built.dependencyRecipe.inputKey -or $sdk.Evidence.manifestSha256 -cne $built.dependencyRecipe.manifestSha256){throw 'Existing artifact SDK inputs do not match their original accepted build.'}
    }
    $inputs=Get-RedTildeBuildInputSnapshot -Root $Root -Platform $Platform -Configuration $Configuration -MSBuildPath $msbuild -SdkInput $sdk
    if(-not $built.buildInputs -or $built.buildInputs.identity -cne $inputs.identity -or
        $built.compiledSourceIdentity -cne $inputs.compiledSourceIdentity -or
        $built.dependencyHeadersSha256 -cne $inputs.dependencyHeadersSha256 -or
        $built.dependencyResolvedPropsSha256 -cne $inputs.dependencyResolvedPropsSha256 -or
        $built.compilerContextSha256 -cne $inputs.compilerContextSha256){
        throw 'The build receipt does not attest the current compiled sources, dependency headers and compiler context. Rebuild before testing.'
    }
    if($null -ne $sdk){Close-CiPinnedSdkInput $sdk}
    }catch{$sdkFailure=$_;throw}
    finally{if($null -ne $sdk -and -not $sdk.Closed){try{Close-CiPinnedSdkInput $sdk}catch{if($null -eq $sdkFailure){throw}else{Write-Warning ('Artifact SDK close also failed: '+$_.Exception.Message)}}}}
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
    $lockPath=Join-Path $Root 'Dependencies/DxUi.lock.json'
    if(-not(Test-Path -LiteralPath $lockPath -PathType Leaf)){throw 'Exact DxUi consumer lock is missing.'}
    $pin=Get-Content -LiteralPath $lockPath -Raw -ErrorAction Stop|ConvertFrom-Json -ErrorAction Stop
    if($pin.repository -cne 'https://github.com/RedSalamanders/DxUi' -or $pin.commit -cnotmatch '^[0-9a-f]{40}$' -or
        $pin.commit -cne $commit -or $Dependency.Identity.apiRevision -ne $pin.apiRevision -or
        @($pin.targets).Count -ne 1 -or $pin.targets[0] -cne 'DxUi'){
        throw 'Restored DxUi build identity does not match the exact consumer lock.'
    }
    $include=Join-Path $Root ".build/dependencies/DxUi/source/$($commit.Substring(0,16))/include"
    $sdk=Split-Path $include -Parent
    $sourceDirectory=Join-Path $sdk 'src'
    $capabilitiesPath=Join-Path $sdk 'capabilities.json'
    $triplet=if($platform -ceq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
    $resolvedOutput=Read-DxUiResolvedOutputRoot -DependencyRoot (Join-Path $Root '.build/dependencies/DxUi') -Platform $platform -BuildIdentity $Dependency
    $packageInclude=Join-Path $resolvedOutput "vcpkg_installed/$platform/$triplet/include"
    if(-not (Test-Path -LiteralPath $include -PathType Container)) {throw 'Pinned public-header source is missing.'}
    if(-not (Test-Path -LiteralPath $sourceDirectory -PathType Container)) {throw 'Pinned DxUi implementation source is missing.'}
    if(-not (Test-Path -LiteralPath $capabilitiesPath -PathType Leaf)) {throw 'Pinned DxUi capabilities metadata is missing.'}
    if(-not (Test-Path -LiteralPath $packageInclude -PathType Container)) {throw 'Restored package include directory is missing.'}
    function Assert-NoReparsePath([string]$Base,[string]$Target) {
        $basePath=[IO.Path]::GetFullPath($Base).TrimEnd('\','/')
        $targetPath=[IO.Path]::GetFullPath($Target)
        $relative=[IO.Path]::GetRelativePath($basePath,$targetPath)
        if($relative -ceq '..' -or $relative.StartsWith('..'+[IO.Path]::DirectorySeparatorChar,[StringComparison]::Ordinal) -or
            [IO.Path]::IsPathRooted($relative)) {throw 'Dependency input path escapes its repository root.'}
        $current=$basePath
        if((Get-Item -LiteralPath $current -Force -ErrorAction Stop).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw 'Dependency input evidence cannot follow a repository-root reparse point.'
        }
        foreach($part in @($relative -split '[\\/]')) {
            if(-not $part -or $part -ceq '.') {continue}
            $current=Join-Path $current $part
            $item=Get-Item -LiteralPath $current -Force -ErrorAction Stop
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {throw 'Dependency input evidence cannot follow reparse points.'}
        }
    }
    Assert-NoReparsePath $Root $sdk
    Assert-NoReparsePath $Root $sourceDirectory
    Assert-NoReparsePath $Root $include
    Assert-NoReparsePath $Root $packageInclude
    Assert-NoReparsePath $Root $capabilitiesPath
    $api=(Get-Content -LiteralPath $capabilitiesPath -Raw -ErrorAction Stop|ConvertFrom-Json -ErrorAction Stop).apiRevision
    if($api -ne $pin.apiRevision){throw 'Pinned DxUi API revision differs from the exact consumer lock.'}
    $head=@(& git -C $sdk rev-parse HEAD)
    if($LASTEXITCODE -ne 0 -or $head.Count -ne 1 -or $head[0] -cne $pin.commit) {throw 'DxUi checkout differs from the exact consumer commit.'}
    $dirty=@(& git -C $sdk status --porcelain --untracked-files=normal)
    if($LASTEXITCODE -ne 0 -or $dirty.Count) {throw 'Pinned DxUi checkout has tracked or untracked changes.'}
    $ignoredSource=@(& git -C $sdk ls-files --others --ignored --exclude-standard -- src)
    if($LASTEXITCODE -ne 0 -or $ignoredSource.Count) {throw 'Pinned DxUi source contains ignored untracked files.'}
    $ignored=@(& git -C $sdk ls-files --others --ignored --exclude-standard -- include)
    if($LASTEXITCODE -ne 0 -or $ignored.Count) {throw 'Public-header checkout has ignored files in the compiled source include tree.'}
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
function Get-RedTildeCoreTestPartitions {
    [CmdletBinding()]
    param([Parameter(Mandatory)][ValidateSet('Storage','Settings','Recovery','Editor')][string]$Group,
          [Parameter(Mandatory)][object[]]$Cases)
    $owned=@($Cases|Where-Object scope -ceq $Group|ForEach-Object name)
    if(-not $owned.Count -or @($owned|Sort-Object -Unique).Count -ne $owned.Count){throw 'Missing or duplicate native group cases.'}
    if($Group -cne 'Editor'){return [pscustomobject]@{scope=$Group;cases=$owned}}
    $behavior=@($owned|Where-Object {$_ -cnotmatch '^(Performance|Rendering)\.'})
    $media=@($behavior|Where-Object {$_ -cmatch 'Image|Bitmap|Mermaid|Diagram|Math|Equation|CodeHighlighting|Http'})
    $workspace=@($behavior|Where-Object {$_ -cnotin $media -and $_ -cmatch '^(Window\.|Outline\.|Editor\.(DesignRefresh|Files|PersistentFolder|Workspace|Startup))'})
    $partitions=@(
        [pscustomobject]@{scope='EditorContent';cases=@($behavior|Where-Object {$_ -cnotin $media -and $_ -cnotin $workspace})},
        [pscustomobject]@{scope='EditorWorkspace';cases=$workspace},
        [pscustomobject]@{scope='EditorMedia';cases=$media},
        [pscustomobject]@{scope='EditorPerformance';cases=@($owned|Where-Object {$_ -cmatch '^Performance\.'})},
        [pscustomobject]@{scope='EditorRendering';cases=@($owned|Where-Object {$_ -cmatch '^Rendering\.'})}
    )|Where-Object {$_.cases.Count}
    $union=@($partitions|ForEach-Object cases)
    if($union.Count -ne $owned.Count -or @(Compare-Object $owned $union -CaseSensitive).Count -or
       @($union|Sort-Object -Unique).Count -ne $union.Count){throw 'Incomplete native Editor partition union.'}
    return $partitions
}
Export-ModuleMember -Function Get-RedTildeCoreTestPartitions,Get-RedTildeSourceIdentity,Get-RedTildeBuildToolchainSnapshot,Assert-RedTildeOwnedCleanDirectories,
    Assert-RedTildeCanonicalBuildDirectories,Get-RedTildeBuildInputSnapshot,Get-RedTildeArtifactIdentity,
    Invoke-RedTildeHarness,Assert-RedTildeTestArtifact,Get-RedTildeDependencyPublicHeaders
