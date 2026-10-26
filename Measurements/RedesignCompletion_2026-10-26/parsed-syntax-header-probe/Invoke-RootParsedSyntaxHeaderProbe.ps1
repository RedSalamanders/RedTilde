[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $RepositoryRoot,
    [Parameter(Mandatory)][ValidatePattern('(?i)^[0-9a-f]{40}$')][string] $ExpectedAppCommit,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{40}$')][string] $ExpectedOldAppCommit,
    [Parameter(Mandatory)][string] $OldBaselineFreeze,
    [switch] $Execute
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath($RepositoryRoot)
$oldSdkCommit = '47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a'
$newSdkCommit = '4077f57df8207eac6c39c43437feb45b19d23cf8'
$oldIdentityPath = Join-Path $repo '.build/audit-20261024/performance-closeout-20261026/parsed-syntax-adoption-282da2b1/DxUi.identity.x64.json.before'
$oldPropsPath = Join-Path $repo '.build/audit-20261024/performance-closeout-20261026/parsed-syntax-adoption-282da2b1/DxUi.resolved.x64.props.before'
$newIdentityPath = Join-Path $repo '.build/dependencies/DxUi/DxUi.identity.x64.json'
$newPropsPath = Join-Path $repo '.build/dependencies/DxUi/DxUi.resolved.x64.props'
$probeSource = Join-Path $PSScriptRoot 'Probe.cpp'
$expectedOldBaseCommit = '0f68857bc383f6dccd1dbe793b01a5a1fd70291c'
$expectedOldFreezePath = [IO.Path]::GetFullPath((Join-Path $repo '.build/performance-closeout-20261026/frozen-final-candidate-0f68857b/baseline.json'))
if (-not $Execute) {
    Write-Output 'PREPARED ONLY. This script has not run a compiler or native probe.'
    Write-Output "Expected SDK pair: $oldSdkCommit -> $newSdkCommit"
    Write-Output 'After the exact x64 Release restore/build and clean source commit, rerun with -Execute and exact app/base identities.'
    return
}
if (-not (Test-Path -LiteralPath $probeSource -PathType Leaf)) { throw 'Probe source is missing.' }
$probeHash = (Get-FileHash -LiteralPath $probeSource -Algorithm SHA256).Hash
if ($probeHash -cne 'F05C8A89AA0E601BC8C7F3E424B1230B5C3E3DB5FD93A69A35AAD645F5B5536F') { throw 'Parsed-snapshot probe source bytes differ from the reviewed source hash.' }
$head = ([string](& git -C $repo rev-parse --verify HEAD)).Trim()
if ($LASTEXITCODE -ne 0 -or $head -cne $ExpectedAppCommit) { throw 'Candidate app HEAD differs from ExpectedAppCommit.' }
if (@(& git -C $repo status --porcelain=v1 --untracked-files=all).Count -ne 0) { throw 'Candidate worktree must be clean.' }
if ($ExpectedOldAppCommit -cne $expectedOldBaseCommit) { throw 'ExpectedOldAppCommit must identify the preserved 0f68857b application baseline.' }

Import-Module (Join-Path $repo 'Build/Harness.psm1') -Force
Import-Module (Join-Path $repo 'Build/Toolchain.psm1') -Force
Import-Module (Join-Path $repo 'Build/Performance.psm1') -Force
$artifact = Assert-RedTildeTestArtifact -Root $repo -Platform x64 -Configuration Release
if ($artifact.SourceIdentity -cne (Get-RedTildeSourceIdentity -Root $repo)) { throw 'Release artifact source binding changed.' }
if ([IO.Path]::GetFullPath($OldBaselineFreeze) -ine $expectedOldFreezePath -or
    -not (Test-Path -LiteralPath $expectedOldFreezePath -PathType Leaf)) { throw 'OldBaselineFreeze must be the preserved 0f68857b SDK-47 baseline.json.' }
$oldFreeze = Get-Content -LiteralPath $expectedOldFreezePath -Raw | ConvertFrom-Json -AsHashtable
if ($oldFreeze.commit -cne $ExpectedOldAppCommit -or $oldFreeze.build.dependency.Identity.commit -cne $oldSdkCommit) {
    throw 'Old freeze is not the declared preserved SDK-47 source/compiler context.'
}
foreach ($inputPath in @($oldIdentityPath,$oldPropsPath,$newIdentityPath,$newPropsPath)) {
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Required identity/props input is missing: $inputPath" }
}
$oldIdentity = Get-Content -LiteralPath $oldIdentityPath -Raw | ConvertFrom-Json -AsHashtable
$newIdentity = Get-Content -LiteralPath $newIdentityPath -Raw | ConvertFrom-Json -AsHashtable
if ($oldIdentity.Identity.commit -cne $oldSdkCommit -or $newIdentity.Identity.commit -cne $newSdkCommit -or
    $oldIdentity.Identity.platform -cne 'x64' -or $newIdentity.Identity.platform -cne 'x64' -or
    $oldIdentity.Identity.apiRevision -ne 4 -or $newIdentity.Identity.apiRevision -ne 4) {
    throw 'SDK identity pair is not the declared x64 47a9 -> parsed-snapshot pair.'
}
if ($oldFreeze.build.dependency.Fingerprint -cne $oldIdentity.Fingerprint -or
    ($oldFreeze.build.dependency.Identity | ConvertTo-Json -Depth 20 -Compress) -cne ($oldIdentity.Identity | ConvertTo-Json -Depth 20 -Compress) -or
    $artifact.Build.dependency.Fingerprint -cne $newIdentity.Fingerprint -or
    ($artifact.Build.dependency.Identity | ConvertTo-Json -Depth 20 -Compress) -cne ($newIdentity.Identity | ConvertTo-Json -Depth 20 -Compress)) {
    throw 'Baseline/candidate SDK identity differs from the preserved freeze or current Release receipt.'
}
$oldIdentityComparable = ConvertFrom-Json -AsHashtable -InputObject (ConvertTo-Json -Depth 20 -InputObject $oldIdentity)
$newIdentityComparable = ConvertFrom-Json -AsHashtable -InputObject (ConvertTo-Json -Depth 20 -InputObject $newIdentity)
$oldIdentityComparable.Remove('Fingerprint'); $newIdentityComparable.Remove('Fingerprint')
$oldIdentityComparable.Identity.Remove('commit'); $newIdentityComparable.Identity.Remove('commit')
if (($oldIdentityComparable | ConvertTo-Json -Depth 20 -Compress) -cne ($newIdentityComparable | ConvertTo-Json -Depth 20 -Compress)) {
    throw 'SDK identity metadata differs beyond the expected commit/fingerprint pair.'
}
$oldProps = [xml](Get-Content -LiteralPath $oldPropsPath -Raw)
$newProps = [xml](Get-Content -LiteralPath $newPropsPath -Raw)
$oldProperty = $oldProps.Project.PropertyGroup
$newProperty = $newProps.Project.PropertyGroup
$oldSourceRoot = [IO.Path]::GetFullPath([string]$oldProperty.DxUiRoot)
$newSourceRoot = [IO.Path]::GetFullPath([string]$newProperty.DxUiRoot)
$oldOutputRoot = [IO.Path]::GetFullPath(([string]$oldProperty.DxUiConsumerOutputRoot).TrimEnd('\','/'))
$newOutputRoot = [IO.Path]::GetFullPath(([string]$newProperty.DxUiConsumerOutputRoot).TrimEnd('\','/'))
$oldPackageInclude = Join-Path $oldOutputRoot 'vcpkg_installed/x64/x64-windows/include'
$newPackageInclude = Join-Path $newOutputRoot 'vcpkg_installed/x64/x64-windows/include'
if ($oldSourceRoot -cne (Join-Path $repo ".build/dependencies/DxUi/source/$oldSdkCommit") -or
    $newSourceRoot -cne (Join-Path $repo ".build/dependencies/DxUi/source/$newSdkCommit") -or
    $oldProperty.DxUiConsumerToolset -cne $oldIdentity.Identity.toolset -or $newProperty.DxUiConsumerToolset -cne $newIdentity.Identity.toolset -or
    $oldProperty.DxUiConsumerVCToolsVersion -cne $oldIdentity.Identity.vcToolsVersion -or $newProperty.DxUiConsumerVCToolsVersion -cne $newIdentity.Identity.vcToolsVersion -or
    $oldProperty.DxUiConsumerSdkVersion -cne $oldIdentity.Identity.windowsSdkVersion -or $newProperty.DxUiConsumerSdkVersion -cne $newIdentity.Identity.windowsSdkVersion -or
    $oldProperty.DxUiConsumerPreferredToolArchitecture -cne $oldIdentity.Identity.preferredToolArchitecture -or
    $newProperty.DxUiConsumerPreferredToolArchitecture -cne $newIdentity.Identity.preferredToolArchitecture -or
    $oldOutputRoot -cne (Join-Path $repo ".build/dependencies/DxUi/$($oldIdentity.Fingerprint)") -or
    $newOutputRoot -cne (Join-Path $repo ".build/dependencies/DxUi/$($newIdentity.Fingerprint)")) {
    throw 'Resolved props do not map to the exact recorded SDK source and package roots.'
}
foreach ($inputPath in @((Join-Path $oldSourceRoot 'include'),$oldPackageInclude,(Join-Path $newSourceRoot 'include'),$newPackageInclude)) {
    if (-not (Test-Path -LiteralPath $inputPath -PathType Container)) { throw "Header input is missing: $inputPath" }
}
foreach ($entry in @(@{root=$oldSourceRoot;commit=$oldSdkCommit;label='baseline'},@{root=$newSourceRoot;commit=$newSdkCommit;label='candidate'})) {
    $sourceHead=([string](& git -C $entry.root rev-parse --verify HEAD)).Trim()
    if ($LASTEXITCODE -ne 0 -or $sourceHead -cne $entry.commit) { throw "$($entry.label) SDK source checkout differs from its fixed commit." }
    $sourceTree=([string](& git -C $entry.root rev-parse --verify 'HEAD^{tree}')).Trim()
    $expectedTree=if($entry.label -ceq 'baseline'){'3ee2e12647ca2e05113823eb172bb0facf879021'}else{'18be66885916f67cafd8aeb761207bf15fbd9120'}
    if ($LASTEXITCODE -ne 0 -or $sourceTree -cne $expectedTree) { throw "$($entry.label) SDK source checkout has an unexpected tree." }
    if (@(& git -C $entry.root status --porcelain=v1 --untracked-files=all).Count -ne 0) { throw "$($entry.label) SDK source checkout must be clean." }
}

function Get-HeaderInventory([string] $Base, [string] $Prefix) {
    $basePath = [IO.Path]::GetFullPath($Base)
    $stack = [Collections.Generic.Stack[string]]::new()
    $stack.Push($basePath)
    $files = [Collections.Generic.List[object]]::new()
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    while ($stack.Count) {
        $directory = $stack.Pop()
        foreach ($entry in Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop) {
            if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing linked header input: $($entry.FullName)" }
            if ($entry.PSIsContainer) { $stack.Push($entry.FullName); continue }
            $relative = [IO.Path]::GetRelativePath($basePath,$entry.FullName).Replace('\','/')
            $name = $Prefix + $relative
            if (-not $seen.Add($name)) { throw "Duplicate header path: $name" }
            $files.Add([ordered]@{path=$name;sha256=(Get-FileHash -LiteralPath $entry.FullName -Algorithm SHA256).Hash})
        }
    }
    $ordered = @($files | Sort-Object path -CaseSensitive)
    $rows = [string[]]@($ordered | ForEach-Object { $_.path + ':' + $_.sha256 })
    [Array]::Sort($rows,[StringComparer]::Ordinal)
    return [ordered]@{sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($rows -join ([string][char]10)))));files=$ordered}
}
function Get-CombinedInventory($Source,$Package) {
    $rows = @($Source.files) + @($Package.files)
    $sorted = @($rows | Sort-Object path -CaseSensitive)
    $values = [string[]]@($sorted | ForEach-Object { $_.path + ':' + $_.sha256 })
    [Array]::Sort($values,[StringComparer]::Ordinal)
    return [ordered]@{schemaVersion=2;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($values -join ([string][char]10)))));
        sourceSha256=$Source.sha256;packageSha256=$Package.sha256;files=$sorted}
}
$oldHeaders = Get-CombinedInventory (Get-HeaderInventory (Join-Path $oldSourceRoot 'include') 'include/') (Get-HeaderInventory $oldPackageInclude 'package-include/')
$newHeaders = Get-RedTildeDependencyPublicHeaders -Root $repo -Dependency $newIdentity
$oldRecordedHeaders = $oldFreeze.publicHeaders
function ConvertTo-UniqueHeaderMap($Inventory,[string] $Label) {
    $map = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
    $seenPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $Inventory.files) {
        if ([string]::IsNullOrWhiteSpace([string]$file.path) -or
            [string]::IsNullOrWhiteSpace([string]$file.sha256) -or
            -not $seenPaths.Add([string]$file.path) -or
            -not $map.TryAdd([string]$file.path,[string]$file.sha256)) {
            throw "$Label public-header inventory contains an empty or duplicate path/hash row."
        }
    }
    return ,$map
}
$oldMap = ConvertTo-UniqueHeaderMap $oldHeaders 'Rehashed old'
$oldRecordedMap = ConvertTo-UniqueHeaderMap $oldRecordedHeaders 'Frozen old'
$newMap = ConvertTo-UniqueHeaderMap $newHeaders 'Candidate'
if ($oldHeaders.files.Count -ne 58 -or $oldRecordedHeaders.files.Count -ne 58 -or
    $oldHeaders.sha256 -cne $oldRecordedHeaders.sha256 -or
    $oldHeaders.sourceSha256 -cne $oldRecordedHeaders.sourceSha256 -or
    $oldHeaders.packageSha256 -cne $oldRecordedHeaders.packageSha256 -or
    $oldMap.Count -ne $oldRecordedMap.Count -or
    @($oldMap.Keys | Where-Object { -not $oldRecordedMap.ContainsKey($_) -or $oldMap[$_] -cne $oldRecordedMap[$_] }).Count -ne 0) {
    throw 'Rehashed old 47a9 header inventory differs from the preserved actual-base freeze.'
}
$expectedChanges = @('include/DxUi/DxUi.h','include/DxUi/MarkdownSyntax.h','include/DxUi/RichDocument.h')
if ($newMap.Count -ne 58 -or $oldMap.Count -ne $newMap.Count -or
    @($oldMap.Keys | Where-Object { -not $newMap.ContainsKey($_) }).Count -or
    @($newMap.Keys | Where-Object { -not $oldMap.ContainsKey($_) }).Count) { throw 'Public header path sets/counts differ.' }
$changedPaths = @($oldMap.Keys | Where-Object { $oldMap[$_] -cne $newMap[$_] } | Sort-Object)
$expectedSorted = @($expectedChanges | Sort-Object)
if (($changedPaths -join '|') -cne ($expectedSorted -join '|')) { throw 'Header delta differs from the exact three-source-header expected set.' }
if (@($oldMap.Keys | Where-Object { $_.StartsWith('include/',[StringComparison]::Ordinal) }).Count -ne 25 -or
    @($oldMap.Keys | Where-Object { $_.StartsWith('package-include/',[StringComparison]::Ordinal) }).Count -ne 33 -or
    @($newMap.Keys | Where-Object { $_.StartsWith('include/DxUi/',[StringComparison]::Ordinal) }).Count -ne 25 -or
    @($newMap.Keys | Where-Object { $_.StartsWith('package-include/',[StringComparison]::Ordinal) }).Count -ne 33) {
    throw 'Baseline/candidate public header inventories must each contain exactly 25 source and 33 package rows.'
}

Import-Module (Join-Path $repo 'Build/Toolchain.psm1') -Force
$msbuild = Get-RedTildeMSBuild
$buildInputs = Get-RedTildeBuildInputSnapshot -Root $repo -Platform x64 -Configuration Release -MSBuildPath $msbuild
$compilerProfile = Get-RedTildeCompilerProfile -Root $repo
if ($buildInputs.compiledSourceIdentity -cne $artifact.Build.compiledSourceIdentity -or
    $buildInputs.dependencyHeadersSha256 -cne $newHeaders.sha256) {
    throw 'Canonical Release build inputs do not match the current candidate receipt/header inventory.'
}
foreach ($field in @('toolset','vcToolsVersion','windowsSdkVersion','runtimeFamily','preferredToolArchitecture','compilerSha256','linkerSha256')) {
    if (([string]$oldIdentity.Identity[$field]) -cne ([string]$newIdentity.Identity[$field])) { throw "Compiler/runtime context differs: $field" }
}
if ($oldIdentity.Identity.compilerSha256 -cne $buildInputs.toolchain.tools.cl.sha256 -or
    $oldIdentity.Identity.linkerSha256 -cne $buildInputs.toolchain.tools.link.sha256) {
    throw 'Selected canonical x64 Release compiler/linker hashes differ from preserved identities.'
}
$compileLog = Join-Path $repo '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
$linkCommandPath = Join-Path $repo '.build/obj/x64/Release/RedTilde/RedTilde.tlog/Link.command.1.tlog'
if (-not (Test-Path -LiteralPath $compileLog -PathType Leaf) -or -not (Test-Path -LiteralPath $linkCommandPath -PathType Leaf)) { throw 'Canonical Release compiler/link command receipts are missing.' }
$rawLinkCommand = Get-Content -LiteralPath $linkCommandPath -Raw
$rawLines = @(Get-Content -LiteralPath $compileLog)
$applicationCpp = [IO.Path]::GetFullPath((Join-Path $repo 'RedTilde/Application.cpp'))
$compileCommand = $null
for ($index=0; $index -lt $rawLines.Count-1; ++$index) {
    if ($rawLines[$index].StartsWith('^') -and @($rawLines[$index].Substring(1).Split('|') | Where-Object { [IO.Path]::GetFullPath($_) -ieq $applicationCpp }).Count) {
        $compileCommand = [string]$rawLines[$index+1]
        break
    }
}
if (-not $compileCommand) { throw 'Could not select the canonical Application.cpp compiler command.' }
if ($compileCommand -match '(?<!\\)(?:\\\\)*\\"') { throw 'Escaped quotes in compiler command need separate parser review.' }
$tokenPattern = '(?:[^\s"]+|"[^"]*")+'
if ([regex]::Replace($compileCommand,$tokenPattern,'').Trim()) { throw 'Unsupported compiler command quoting.' }
$commandTokens = @([regex]::Matches($compileCommand,$tokenPattern) | ForEach-Object Value)
if ($commandTokens.Count -and [IO.Path]::GetFileName($commandTokens[0].Trim('"')) -ieq 'cl.exe') { $commandTokens = @($commandTokens | Select-Object -Skip 1) }

function Invoke-Process([string] $Executable,[string[]] $Arguments,[string] $WorkingDirectory,[string] $LogPath,[hashtable] $Environment) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $Executable
    $start.WorkingDirectory = $WorkingDirectory
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    foreach ($name in $Environment.Keys) { $start.Environment[$name] = [string]$Environment[$name] }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    if (-not $process.Start()) { throw "Could not start $Executable" }
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $out = $stdout.GetAwaiter().GetResult()
    $err = $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText($LogPath,('stdout:'+[Environment]::NewLine+$out+[Environment]::NewLine+'stderr:'+[Environment]::NewLine+$err),[Text.UTF8Encoding]::new($false))
    $exit = $process.ExitCode
    $process.Dispose()
    if ($exit -ne 0) { throw "$Executable failed ($exit); see $LogPath" }
    return $out
}
function Get-ProbeArguments([string] $Role,[string] $SdkSourceInclude,[string] $SdkPackageInclude,[string] $OutDirectory) {
    $tokens = [Collections.Generic.List[string]]::new()
    $sourceCount = 0; $packageCount = 0; $sourceFileCount = 0; $outputCount = 0; $pdbCount = 0
    for ($index=0; $index -lt $commandTokens.Count; ++$index) {
        $token = [string]$commandTokens[$index]
        if ($token -cmatch '^/Yu' -or $token -cmatch '^/Fp' -or $token -cmatch '^/Yc') { continue }
        if ($token -match '^/FI') { throw 'Forced include in app command prevents a faithful probe context.' }
        if ($token -cmatch '^/Y') { continue }
        if ($token -ceq '/Fo' -or $token -ceq '/Fd') {
            if (++$index -ge $commandTokens.Count) { throw "Missing operand for $token." }
            if ($token -ceq '/Fo') {
                ++$outputCount; $tokens.Add('/Fo'+(Join-Path $OutDirectory ($Role+'.obj')))
            } else {
                ++$pdbCount; $tokens.Add('/Fd'+(Join-Path $OutDirectory ($Role+'.compile.pdb')))
            }
            continue
        }
        if ($token -match '^/Fo(?<value>.*)$') {
            ++$outputCount; $tokens.Add('/Fo'+(Join-Path $OutDirectory ($Role+'.obj'))); continue
        }
        if ($token -match '^/Fd(?<value>.*)$') {
            ++$pdbCount; $tokens.Add('/Fd'+(Join-Path $OutDirectory ($Role+'.compile.pdb'))); continue
        }
        if ($token -ceq '/I') {
            if (++$index -ge $commandTokens.Count) { throw 'Missing /I operand.' }
            $path = $commandTokens[$index].Trim('"')
            if ([IO.Path]::GetFullPath($path).TrimEnd('\','/') -ieq [IO.Path]::GetFullPath($newSdkSourceInclude).TrimEnd('\','/')) {
                ++$sourceCount; $tokens.Add('/I'+$SdkSourceInclude); continue
            }
            if ([IO.Path]::GetFullPath($path).TrimEnd('\','/') -ieq [IO.Path]::GetFullPath($newSdkPackageInclude).TrimEnd('\','/')) {
                ++$packageCount; $tokens.Add('/I'+$SdkPackageInclude); continue
            }
            $tokens.Add('/I'+$path); continue
        }
        if ($token -match '^/I(?<path>.*)$') {
            $path = $Matches.path.Trim('"')
            if ([IO.Path]::GetFullPath($path).TrimEnd('\','/') -ieq [IO.Path]::GetFullPath($newSdkSourceInclude).TrimEnd('\','/')) {
                ++$sourceCount; $tokens.Add('/I'+$SdkSourceInclude); continue
            }
            if ([IO.Path]::GetFullPath($path).TrimEnd('\','/') -ieq [IO.Path]::GetFullPath($newSdkPackageInclude).TrimEnd('\','/')) {
                ++$packageCount; $tokens.Add('/I'+$SdkPackageInclude); continue
            }
            $tokens.Add('/I'+$path); continue
        }
        if ($token.Trim('"') -ieq $applicationCpp) { ++$sourceFileCount; $tokens.Add($probeSource); continue }
        if ($token -match 'DXUI_PROBE_EXPECT_PARSED_ADOPTION') { throw 'Parsed-snapshot probe macro already exists in canonical application command.' }
        $tokens.Add($token)
    }
    if ($sourceCount -ne 1 -or $packageCount -ne 1 -or $sourceFileCount -ne 1 -or $outputCount -ne 1 -or $pdbCount -ne 1) {
        throw "Expected one source include, package include, source file, object output and compiler PDB; observed $sourceCount/$packageCount/$sourceFileCount/$outputCount/$pdbCount."
    }
    if (@($tokens | Where-Object { $_.Contains('"') }).Count -ne 0) {
        throw 'Compiler command contains quoted option syntax that needs explicit argv normalization.'
    }
    $tokens.Add('/Y-')
    $tokens.Add('/DDXUI_PROBE_EXPECT_PARSED_ADOPTION=' + $(if ($Role -ceq 'candidate') { '1' } else { '0' }))
    return ,$tokens.ToArray()
}
$newSdkSourceInclude = Join-Path $newSourceRoot 'include'
$newSdkPackageInclude = $newPackageInclude
$environment = @{}
foreach ($key in @('CL','_CL_','LINK','_LINK_','RC','INCLUDE','LIB','LIBPATH','VCToolsVersion','WindowsSDKVersion','WindowsSdkDir')) {
    $environment[$key] = [string]$buildInputs.toolchain.environment[$key]
}
$evaluatedJson = (& $msbuild (Join-Path $repo 'RedTilde/RedTilde.vcxproj') /nologo /p:Platform=x64 /p:Configuration=Release '-getProperty:IncludePath,LibraryPath,VCToolsInstallDir,WindowsSdkDir,WindowsTargetPlatformVersion' | Out-String)
if ($LASTEXITCODE -ne 0) { throw 'Selected MSBuild failed to return probe include/library paths.' }
$evaluatedProperties = ($evaluatedJson | ConvertFrom-Json -AsHashtable).Properties
foreach ($field in @('VCToolsInstallDir','WindowsSdkDir','WindowsTargetPlatformVersion')) {
    if ([string]$evaluatedProperties[$field] -ine [string]$buildInputs.toolchain.properties[$field]) { throw "Evaluated probe context differs: $field" }
}
if (-not $evaluatedProperties.IncludePath -or -not $evaluatedProperties.LibraryPath) { throw 'Evaluated include/library paths are empty.' }
$environment['INCLUDE'] = [string]$evaluatedProperties.IncludePath
$environment['LIB'] = [string]$evaluatedProperties.LibraryPath
$cl = [string]$buildInputs.toolchain.tools.cl.path
$link = [string]$buildInputs.toolchain.tools.link.path
$expectedRoot = [IO.Path]::GetFullPath($PSScriptRoot)
$outputRoot = [IO.Path]::GetFullPath((Join-Path $expectedRoot ('run-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ') + '-' + [guid]::NewGuid().ToString('N'))))
if (-not $outputRoot.StartsWith($expectedRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $outputRoot)) {
    throw 'Probe output must be a fresh directory below this driver folder.'
}
[void][IO.Directory]::CreateDirectory($outputRoot)
$sideResults = [ordered]@{}
foreach ($role in @('baseline','candidate')) {
    $sideDir = Join-Path $outputRoot $role
    [void][IO.Directory]::CreateDirectory($sideDir)
    $sourceInclude = if ($role -ceq 'baseline') { Join-Path $oldSourceRoot 'include' } else { $newSdkSourceInclude }
    $packageInclude = if ($role -ceq 'baseline') { $oldPackageInclude } else { $newPackageInclude }
    [string[]]$arguments = Get-ProbeArguments $role $sourceInclude $packageInclude $sideDir
    $compileRaw = $arguments -join ' '
    $normalizedArguments = [Collections.Generic.List[string]]::new()
    foreach ($argument in $arguments) {
        if ($argument -match '^/Fo' -or $argument -match '^/Fd' -or
        $argument -match '^/DDXUI_PROBE_EXPECT_PARSED_ADOPTION=' -or $argument -ceq '/Y-') { continue }
        $normalized = [regex]::Replace([string]$argument,[regex]::Escape($oldSourceRoot+'\include'),'<DXUI_SOURCE_INCLUDE>',[Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $normalized = [regex]::Replace($normalized,[regex]::Escape($newSourceRoot+'\include'),'<DXUI_SOURCE_INCLUDE>',[Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $normalized = [regex]::Replace($normalized,[regex]::Escape($oldPackageInclude),'<DXUI_PACKAGE_INCLUDE>',[Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $normalized = [regex]::Replace($normalized,[regex]::Escape($newPackageInclude),'<DXUI_PACKAGE_INCLUDE>',[Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $normalizedArguments.Add($normalized)
    }
    $compileLogPath = Join-Path $sideDir 'compile.log'
    [void](Invoke-Process $cl $arguments $sideDir $compileLogPath $environment)
    $object = Join-Path $sideDir ($role+'.obj')
    $compilePdb = Join-Path $sideDir ($role+'.compile.pdb')
    if (-not (Test-Path -LiteralPath $object -PathType Leaf) -or -not (Test-Path -LiteralPath $compilePdb -PathType Leaf)) {
        throw "Probe compile artifacts missing for $role."
    }
    $executable = Join-Path $sideDir ($role+'.exe')
    $linkPdb = Join-Path $sideDir ($role+'.pdb')
    $linkArguments = @('/NOLOGO','/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/DEBUG',
        ('/OUT:'+$executable),('/PDB:'+$linkPdb),$object)
    $linkLogPath = Join-Path $sideDir 'link.log'
    [void](Invoke-Process $link $linkArguments $sideDir $linkLogPath $environment)
    $runtimeOutput = Invoke-Process $executable @() $sideDir (Join-Path $sideDir 'run.log') $environment
    $probeValues = $runtimeOutput | ConvertFrom-Json
    $identity = if ($role -ceq 'baseline') { $oldIdentity } else { $newIdentity }
    $sideHeaders = if ($role -ceq 'baseline') { $oldHeaders } else { $newHeaders }
    $sideResults[$role] = [ordered]@{
        sdkCommit=$identity.Identity.commit;fingerprint=$identity.Fingerprint;apiRevision=$identity.Identity.apiRevision;
        headers=$sideHeaders;compilerSha256=$identity.Identity.compilerSha256;linkerSha256=$identity.Identity.linkerSha256;
        rawApplicationCompileCommand=$compileCommand;rawApplicationLinkCommand=$rawLinkCommand;rawProbeCompileArguments=$arguments;normalizedProbeCompileArguments=$normalizedArguments.ToArray();probeLinkArguments=$linkArguments;
        applicationCompilerProfile=$compilerProfile;buildInputSnapshotIdentity=$buildInputs.identity;
        pchMode='disabled for the standalone layout probe';pdbSha256=(Get-FileHash -LiteralPath $linkPdb -Algorithm SHA256).Hash;
        compilePdbSha256=(Get-FileHash -LiteralPath $compilePdb -Algorithm SHA256).Hash;
        objectSha256=(Get-FileHash -LiteralPath $object -Algorithm SHA256).Hash;executableSha256=(Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash;
        values=$probeValues
    }
}
$layoutTypes = @('MarkdownParser','RichTextEditor','SourceSnapshot','Document')
$layoutDeltaVerified = ($sideResults.baseline.values.sizeofMarkdownSyntax -gt 0 -and
    $sideResults.candidate.values.sizeofMarkdownSyntax -eq ($sideResults.baseline.values.sizeofMarkdownSyntax + 16) -and
    $sideResults.baseline.values.alignofMarkdownSyntax -eq $sideResults.candidate.values.alignofMarkdownSyntax)
foreach ($type in $layoutTypes) {
    $sizeKey = if ($type -ceq 'Document') { 'RichTextDocument' } else { $type }
    foreach ($prefix in @('sizeof','alignof')) {
        $key = $prefix + $sizeKey
        if ($sideResults.baseline.values.$key -cne $sideResults.candidate.values.$key) { $layoutDeltaVerified = $false }
    }
}
$stableFields = @('controlIsPolymorphic','richTextEditorIsPolymorphic','msvcVersion','msvcLang','iteratorDebugLevel')
$stableProbeContext = $true
foreach ($key in $stableFields) {
    if ($sideResults.baseline.values.$key -cne $sideResults.candidate.values.$key) { $stableProbeContext = $false }
}
$oldMethodOk = ($sideResults.baseline.values.hasReadOnlySourceSynchronization -ceq $true -and
    $sideResults.candidate.values.hasReadOnlySourceSynchronization -ceq $true)
$newMethodOk = ($sideResults.baseline.values.hasSourceSnapshotMatchesText -ceq $false -and
    $sideResults.candidate.values.hasSourceSnapshotMatchesText -ceq $true -and
    $sideResults.baseline.values.hasParsedSnapshotAdoption -ceq $false -and
    $sideResults.candidate.values.hasParsedSnapshotAdoption -ceq $true -and
    $sideResults.baseline.values.hasReadOnlyMarkdownSyntaxAdoption -ceq $false -and
    $sideResults.candidate.values.hasReadOnlyMarkdownSyntaxAdoption -ceq $true)
$normalizedProbeCompileArgsEqual = (($sideResults.baseline.normalizedProbeCompileArguments -join ([string][char]10)) -ceq
    ($sideResults.candidate.normalizedProbeCompileArguments -join ([string][char]10)))
if (-not $normalizedProbeCompileArgsEqual) { throw 'Normalized baseline/candidate probe compiler arguments differ beyond declared SDK include operands.' }
if (-not $layoutDeltaVerified -or -not $stableProbeContext -or -not $oldMethodOk -or -not $newMethodOk) {
    throw 'Parsed-snapshot source probe results do not match the exact reviewed layout and method-availability rules.'
}
$compileTlogCopy = Join-Path $outputRoot 'application-compiler.tlog'
$linkTlogCopy = Join-Path $outputRoot 'application-link.tlog'
Copy-Item -LiteralPath $compileLog -Destination $compileTlogCopy
Copy-Item -LiteralPath $linkCommandPath -Destination $linkTlogCopy
$result = [ordered]@{
    schemaVersion=2;protocol='dxui-parsed-source-snapshot-source-probe-v2';executed=$true;
    expectedPair=@{oldSdkCommit=$oldSdkCommit;newSdkCommit=$newSdkCommit;oldAppCommit=$ExpectedOldAppCommit;candidateAppCommit=$ExpectedAppCommit};
    probeSourceSha256=$probeHash;probeSourceFile='Probe.cpp';probeEvaluatedMSBuildProperties=$evaluatedProperties;
    candidateBuildInputSnapshot=$buildInputs;candidateReleaseReceiptSha256=$artifact.ReceiptSha256;candidateSourceIdentity=$artifact.SourceIdentity;
    oldPropsSha256=(Get-FileHash -LiteralPath $oldPropsPath).Hash;newPropsSha256=(Get-FileHash -LiteralPath $newPropsPath).Hash;
    oldIdentitySha256=(Get-FileHash -LiteralPath $oldIdentityPath).Hash;newIdentitySha256=(Get-FileHash -LiteralPath $newIdentityPath).Hash;
    applicationCompilerTlogFile='application-compiler.tlog';applicationCompilerTlogSha256=(Get-FileHash -LiteralPath $compileTlogCopy).Hash;
    applicationLinkTlogFile='application-link.tlog';applicationLinkTlogSha256=(Get-FileHash -LiteralPath $linkTlogCopy).Hash;
    rawApplicationCompileCommand=$compileCommand;rawApplicationLinkCommand=$rawLinkCommand;
    rawApplicationLinkCommandSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($rawLinkCommand)));
    rawApplicationCompileCommandSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($compileCommand)));
    normalizedProbeCompileArgsEqual=$normalizedProbeCompileArgsEqual;
    fullApplicationCompileContextExact=$false;fullApplicationLinkContextExact=$false;
    fullContextLimit='PCH is disabled for this isolated source/layout probe. The standalone linker uses the selected link.exe, evaluated include/library environment, and only the probe object; no DxUi library is linked or called. This does not establish full-application compile/link equivalence or binary ABI compatibility.';
    layoutDeltaVerified=$layoutDeltaVerified;newMethodSignaturesVerified=$newMethodOk;sides=$sideResults;
    outputDirectory=$outputRoot
}
$result | ConvertTo-Json -Depth 40 | Set-Content -LiteralPath (Join-Path $outputRoot 'source-probe.json') -Encoding utf8
