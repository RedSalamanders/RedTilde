# Git/content/process fixtures exercise the public runner without a native build or desktop use.
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
Import-Module (Join-Path $root 'Build/ScopedTesting.psm1') -Force
$manifest=Read-ScopedTestManifest $root
$fixture=Join-Path $root ('.build/ToolTests/scoped-'+[guid]::NewGuid().ToString('N'))
$passed=0
function Assert([bool]$Value,[string]$Message){if(-not $Value){throw $Message}}
function Case([string]$Name,[scriptblock]$Body){&$Body;$script:passed++;Write-Host "PASS $Name"}
function Put([string]$Path,[string]$Text){[void](New-Item -ItemType Directory -Path (Split-Path $Path) -Force);[IO.File]::WriteAllText($Path,$Text,[Text.UTF8Encoding]::new($false))}
function Invoke-FixtureGit([string]$Directory,[string[]]$Arguments){& git -C $Directory @Arguments *> $null;if($LASTEXITCODE){throw "Fixture Git failed: $Arguments"}}
function New-Repository([string]$Directory){
    [void](New-Item -ItemType Directory -Path $Directory -Force)
    Put (Join-Path $Directory '.gitignore') ".build/`n"
    Invoke-FixtureGit $Directory @('init','-q','-b','main')
    # Each nested repository has its own config; the checkout's Git config is not inherited.
    Invoke-FixtureGit $Directory @('config','core.longpaths','true')
    Invoke-FixtureGit $Directory @('config','user.name','Scope fixture');Invoke-FixtureGit $Directory @('config','user.email','fixture@example.invalid')
}
try{
    Case 'actual ignored engine bytes bind build and tooling evidence without changing iteration maps' {
        $product=Join-Path $fixture 'engine-input-project'
        [void][IO.Directory]::CreateDirectory($product)
        New-Repository $product
        Put (Join-Path $product '.gitignore') ".build/`n.ci-engine/`n"
        Put (Join-Path $product '.ci/project.json') ([IO.File]::ReadAllText((Join-Path $root '.ci/project.json')))
        Put (Join-Path $product 'Tests/test-scopes.json') ([IO.File]::ReadAllText((Join-Path $root 'Tests/test-scopes.json')))
        Put (Join-Path $product 'runtime.cpp') 'synthetic source'
        $engine=Join-Path $product '.ci-engine'
        $cloneOutput=& git clone --quiet --no-hardlinks (Join-Path $root '.ci-engine') $engine 2>&1
        if($LASTEXITCODE){throw "Owned engine fixture clone failed: $cloneOutput"}
        $pin=([IO.File]::ReadAllText((Join-Path $product '.ci/project.json')) | ConvertFrom-Json).engine.ref
        Invoke-FixtureGit $engine @('checkout','--detach','--quiet',$pin)
        $mapBefore=Get-ScopedMapIdentity (Get-ScopedSourceMap $product)
        $toolingBefore=Get-ScopedSourceIdentity $product
        $buildBefore=Get-ScopedSourceIdentity $product -CompiledOnly
        Import-Module (Join-Path $root 'Build/Harness.psm1') -ErrorAction Stop
        $directBuildBefore=Get-RedTildeSourceIdentity -Root $product
        if([OperatingSystem]::IsWindows()){
            Assert ((Get-ScopedSourceIdentity $product.ToUpperInvariant()) -ceq $toolingBefore) 'Windows path casing invalidated tooling evidence'
            Assert ((Get-ScopedSourceIdentity $product.ToUpperInvariant() -CompiledOnly) -ceq $buildBefore) 'Windows path casing invalidated build evidence'
        }
        Put (Join-Path $engine '.build/fixture-output.json') '{"generated":true}'
        Assert ((Get-ScopedSourceIdentity $product) -ceq $toolingBefore) 'Generated engine output invalidated tooling'
        Assert ((Get-ScopedSourceIdentity $product -CompiledOnly) -ceq $buildBefore) 'Generated engine output invalidated build evidence'
        Assert ((Get-RedTildeSourceIdentity -Root $product) -ceq $directBuildBefore) 'Generated engine output invalidated direct build identity'
        $policy=Join-Path $engine 'Tools/Modules/Ci.Policy.psm1'
        $original=[IO.File]::ReadAllBytes($policy)
        try{
            [IO.File]::AppendAllText($policy,"`n# Synthetic changed engine implementation.`n")
            foreach($compiled in @($false,$true)){
                $refusal=''
                try{$null=Get-ScopedSourceIdentity $product -CompiledOnly:$compiled}catch [System.Management.Automation.RuntimeException]{$refusal=$_.Exception.Message}
                Assert ($refusal.Contains('Local engine checkout must be clean')) 'Modified ignored engine retained reusable build/tooling evidence'
            }
            Assert ((Get-ScopedMapIdentity (Get-ScopedSourceMap $product)) -ceq $mapBefore) 'Engine identity polluted product iteration maps'
            $directRefusal=''
            try{$null=Get-RedTildeSourceIdentity -Root $product}catch [System.Management.Automation.RuntimeException]{$directRefusal=$_.Exception.Message}
            Assert ($directRefusal.Contains('Local engine checkout must be clean')) 'Direct source/SDK build identity ignored modified engine implementation'
        }finally{[IO.File]::WriteAllBytes($policy,$original)}
        Assert ((Get-ScopedSourceIdentity $product) -ceq $toolingBefore) 'Restored exact engine did not recover its tooling identity'
        Assert ((Get-ScopedSourceIdentity $product -CompiledOnly) -ceq $buildBefore) 'Restored exact engine did not recover its build identity'

    }
    foreach($mode in @('Complete','Missing','Mismatch','Incomplete','Duplicate','Failed','Created','WrongPlatform','TestEnabled','SmokeFailed')){
    Case ('production CI retains original smoke proof before reporting success ('+$mode+')') {
        $adapter=Join-Path $root 'Tools/Invoke-ProjectCi.ps1'
        $tokens=$null;$errors=$null
        $ast=[Management.Automation.Language.Parser]::ParseFile($adapter,[ref]$tokens,[ref]$errors)
        Assert ($errors.Count -eq 0) 'Actual production adapter did not parse'
        $clauses=@($ast.FindAll({param($node)$node -is [Management.Automation.Language.IfStatementAst]},$true)|ForEach-Object Clauses|
            Where-Object {$_.Item1.Extent.Text -ceq '$Phase -eq ''Production''' -and $_.Item2.Extent.Text.Contains("Run 'package.ps1'")})
        Assert ($clauses.Count -eq 1) 'Actual production phase was not selected exactly once'
        $body=$clauses[0].Item2.Extent.Text
        $production=[scriptblock]::Create($body.Substring(1,$body.Length-2))
        $root=Join-Path $fixture ('production-proof-'+$mode)
        $Platform='x64';$context=[pscustomobject]@{root=$root};$sdkDescriptor=$null
        function Run([string]$Script,[string[]]$CommandArguments){
            Assert ($Script -ceq 'package.ps1' -and ($CommandArguments -join '|') -ceq '-Platform|x64|-SkipBuild') 'Production did not use the public package command'
            $package=Join-Path $root '.build/package/synthetic-x64-0.zip'
            Put $package 'original synthetic package bytes'
            if($mode -ceq 'Missing'){return}
            $names=@('self-test','test-scope','list-tests','interactive','crash','recovery-crash','recovery-restart',
                'preferences','asan','linked-save-crash','linked-save-restart','failure-host','failure-host-missing-value',
                'test-scope-missing-value','interactive-missing-value','failure-report')
            $refusals=@($names|ForEach-Object {@{name=$_;exitCode=2;dataDirectoryAbsent=$true}})
            if($mode -ceq 'Incomplete'){$refusals=$refusals[0..11]}
            if($mode -ceq 'Duplicate'){$refusals[12].name=$refusals[0].name}
            if($mode -ceq 'Failed'){$refusals[12].exitCode=0}
            if($mode -ceq 'Created'){$refusals[12].dataDirectoryAbsent=$false}
            $record=[ordered]@{platform='x64';testsEnabled=$false;smokePassed=$true;
                archiveSha256=$(if($mode -ceq 'Mismatch'){'0'*64}else{(Get-FileHash $package).Hash});
                testOnlyEntryPointRefusals=$refusals}
            if($mode -ceq 'WrongPlatform'){$record.platform='ARM64'}
            if($mode -ceq 'TestEnabled'){$record.testsEnabled=$true}
            if($mode -ceq 'SmokeFailed'){$record.smokePassed=$false}
            Put ($package+'.receipt.json') (($record|ConvertTo-Json -Depth 5)+"`n ")
        }
        function Write-CiProductionReceipt($Context,$Archives,[bool]$TestModeDisabled,[bool]$SmokePassed){
            Assert ($TestModeDisabled -and $SmokePassed -and $Archives.Count -eq 1) 'Production summary arguments changed'
            Put (Join-Path $root '.build/ci/production/x64/outer.json') 'synthetic successful summary'
        }
        $refused=$false
        try{& $production}catch{$refused=$_.Exception.Message.StartsWith('Original production package smoke receipt',[StringComparison]::Ordinal)}
        $output=Join-Path $root '.build/ci/production/x64/portable-smoke.json'
        if($mode -ceq 'Complete'){
            Assert (-not $refused -and (Test-Path -LiteralPath $output)) 'Production discarded its actual smoke receipt'
            $original=Join-Path $root '.build/package/synthetic-x64-0.zip.receipt.json'
            Assert ((Get-FileHash $output).Hash -ceq (Get-FileHash $original).Hash) 'Production normalized or replaced the original smoke bytes'
        }else{
            Assert $refused 'Production accepted missing or mismatched original smoke proof'
            Assert (-not (Test-Path -LiteralPath $output) -and -not (Test-Path -LiteralPath (Join-Path $root '.build/ci/production/x64/outer.json'))) 'Production reported success before admitting original proof'
        }
    }
    }
    Case 'Editor child partitions preserve the exact reviewed union without extending child timeouts' {
        Import-Module (Join-Path $root 'Build/Harness.psm1') -Force
        $cases=@(
            @{name='Editor.Typed';scope='Editor'},@{name='Window.Close';scope='Editor'},
            @{name='Editor.DesignRefreshSettings';scope='Editor'},@{name='Editor.LocalImage';scope='Editor'},
            @{name='Math.Formula';scope='Editor'},@{name='Editor.HttpBounds';scope='Editor'},
            @{name='Performance.Frames';scope='Editor'},@{name='Rendering.Resources';scope='Editor'},
            @{name='Storage.Bytes';scope='Storage'}
        )
        $parts=@(Get-RedTildeCoreTestPartitions -Group Editor -Cases $cases)
        Assert (($parts.scope -join ',') -ceq 'EditorContent,EditorWorkspace,EditorMedia,EditorPerformance,EditorRendering') 'Stable bounded child scopes'
        Assert ((@($parts[0].cases) -join ',') -ceq 'Editor.Typed') 'Content retains independently authored editing cases'
        Assert ((@($parts[1].cases) -join ',') -ceq 'Window.Close,Editor.DesignRefreshSettings') 'Workspace retains independently authored shell cases'
        Assert ((@($parts[2].cases) -join ',') -ceq 'Editor.LocalImage,Math.Formula,Editor.HttpBounds') 'Media retains independently authored image, math and transport cases'
        Assert ((@($parts[3].cases) -join ',') -ceq 'Performance.Frames' -and (@($parts[4].cases) -join ',') -ceq 'Rendering.Resources') 'Independent diagnostic/render partition expectations'
        $storage=@(Get-RedTildeCoreTestPartitions -Group Storage -Cases $cases)
        Assert ($storage.Count -eq 1 -and $storage[0].scope -ceq 'Storage' -and $storage[0].cases[0] -ceq 'Storage.Bytes') 'Ordinary groups remain intact'
        $refused=$false
        try{Get-RedTildeCoreTestPartitions -Group Editor -Cases @($cases[0],$cases[0])|Out-Null}catch [System.Management.Automation.RuntimeException]{$refused=$_.Exception.Message -like '*duplicate native group*'}
        Assert $refused 'Duplicate case ownership is refused'
    }
    Case 'active native naming and inventory are contextual and complete' {
        Assert ((Assert-ScopedTestNames -Root $root -Manifest $manifest) -gt 0) 'Empty inventory'
        $paths=([IO.File]::ReadAllText((Join-Path $root $manifest.nativeInventory.path)) | ConvertFrom-Json).files.path
        Assert (-not @($paths | Where-Object { $_ -match '(^|/)Scope\.Tests\.' }).Count) 'Placeholder test scope'
    }
    Case 'data preserves product ownership and widens on unknown inputs' {
        $plan=Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @('Common/Storage.cpp') -Configuration Debug
        Assert (($plan.scopes -join ',') -ceq 'Storage,Recovery,RecoveryRestart,Format') 'Storage source routing drifted'
        $plan=Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @('Tests/Application.Tests.Performance.cpp') -Configuration Debug
        Assert (('Editor' -in $plan.scopes) -and ('Performance' -in $plan.scopes)) 'Performance fixture lost its native consumers'
        foreach($file in 'Build/UnknownNewBuildPolicy.psm1','Unmapped/New.cpp','res/Welcome.md'){
            Assert ((Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @($file)).full) "Expected full coverage for $file"
        }
        foreach($file in 'validate-skills.ps1','Tools/Test-AgentGuidance.ps1'){
            $plan=Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @($file)
            Assert (($plan.scopes -join ',') -ceq 'Tooling') "Guidance validation lost its independent scope: $file"
        }
    }
    Case 'shipped Markdown remains an input and ordinary prose selects only independent scopes' {
        $map=Get-ScopedSourceMap -Root $root -Manifest $manifest -CompiledOnly
        Assert ($map.Contains('docs/examples/README.md') -and $map.Contains('Tests/Fixtures/reference-dark.md')) 'Shipped runtime Markdown is missing from compiled evidence'
        Assert (-not $map.Contains('README.md') -and -not $map.Contains('docs/README.md')) 'Ordinary guide entered compiled evidence'
        Assert ((Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @('docs/examples/README.md')).full) 'Shipped Markdown did not select native work'
        $prose=Get-ScopedTestPlan -Manifest $manifest -ChangedPaths @('README.md')
        Assert (($prose.scopes -join ',') -ceq 'Tooling,Format') 'Ordinary documentation selected runtime tests'
    }
    Case 'PrePush profiles and interactive design admission are explicit' {
        $profiles=@($manifest.prePushProfiles | ForEach-Object { "$($_.platform)/$($_.configuration)/$($_.buildOnly)" })
        Assert (($profiles -join '|') -ceq 'x64/Debug/False|x64/Release/False|x64/ASan Debug/False|ARM64/Debug/True|ARM64/Release/True|ARM64/ASan Debug/True') 'PrePush matrix changed'
        Assert (-not @($manifest.scopes | Where-Object name -ceq 'DesignInteractive').Count) 'Scoped runner admitted an interactive suite'
        $obligations=@(Get-ScopedInteractiveObligations -Manifest $manifest -ChangedPaths @('Tests/Application.Tests.DesignInteractive.cpp'))
        Assert ($obligations.Count -eq 1 -and $obligations[0].suite -ceq 'DesignInteractive') 'Interactive design obligation is missing'
        $tools=@($manifest.scopes | Where-Object name -ceq 'Format')
        Assert ($tools.Count -eq 1 -and $tools[0].commands[0].path -ceq 'format.ps1' -and ($tools[0].commands[0].arguments -join ',') -ceq '-Check') 'Format scope lost the pinned formatter command'
    }
    Case 'native group catalog retains the actual case registration union' {
        $catalog=[IO.File]::ReadAllText((Join-Path $root 'Tests/native-case-catalog.json')) | ConvertFrom-Json
        $inventory=[IO.File]::ReadAllText((Join-Path $root $manifest.nativeInventory.path)) | ConvertFrom-Json
        $actual=@(foreach($unit in $inventory.files | Where-Object {$_.suite -ceq 'Core' -and [IO.Path]::GetExtension($_.path) -in @('.cpp','.h')}){
            $source=[IO.File]::ReadAllText((Join-Path $root $unit.path))
            [regex]::Matches($source,'\btest\s*\(\s*"([^"]+)"') | ForEach-Object {$_.Groups[1].Value}
        })
        Assert (-not (Compare-Object $actual @($catalog.cases.name))) 'Reviewed native catalog differs from source'
        Assert (@($catalog.cases.name | Sort-Object -Unique).Count -eq $actual.Count) 'Duplicate native case'
    }
    Case 'direct test revokes a failed attempt before the build boundary' {
        $direct=Join-Path $fixture 'engine-input-project'
        Put (Join-Path $direct 'test.ps1') ([IO.File]::ReadAllText((Join-Path $root 'test.ps1')))
        Put (Join-Path $direct 'Build/ScopedTesting.psm1') ([IO.File]::ReadAllText((Join-Path $root 'Build/ScopedTesting.psm1')))
        Put (Join-Path $direct 'build.ps1') "throw 'Scoped fixture stopped before native execution.'"
        $reports=Get-ScopedReportRoot -Root $direct -Platform x64 -Configuration Debug
        $baseline=Join-Path $reports 'iteration.json'
        $selected=Join-Path $reports 'Storage.json'
        $unrelated=Join-Path $reports 'Settings.json'
        foreach($receipt in $baseline,$selected,$unrelated){Put $receipt '{"fixture":"prior success"}'}
        $pwsh=Join-Path $PSHOME $(if($IsWindows){'pwsh.exe'}else{'pwsh'})
        $log=Join-Path $fixture 'direct-native-refusal.log'
        & $pwsh -NoProfile -NonInteractive -File (Join-Path $direct 'test.ps1') -Platform x64 -Configuration Debug -Suites Storage *> $log
        Assert ($LASTEXITCODE -ne 0 -and [IO.File]::ReadAllText($log).Contains('Scoped fixture stopped before native execution.')) 'Direct runner did not reach the safe build boundary'
        Assert (-not ([IO.File]::Exists($baseline) -or [IO.File]::Exists($selected))) 'A failed direct run retained selected evidence'
        Assert ([IO.File]::Exists($unrelated)) 'A failed direct run removed an unrelated receipt'
    }
    Case 'package and performance entrypoints share current build artifact admission' {
        foreach($script in @('package.ps1','performance-paired.ps1')){
            $content=[IO.File]::ReadAllText((Join-Path $root $script))
            Assert ($content.Contains('Assert-RedTildeTestArtifact')) "$script bypasses the shared current build admission check"
            Assert ($content -notmatch '(?im)Get-Content[^\r\n]*build-receipt\.json') "$script reads a build receipt without the shared admission check"
        }
    }
    Case 'public build receipt invalidates content changes despite restored timestamps and preserves docs-only outputs' {
        $buildRoot=Join-Path $fixture 'build-input-guard';New-Repository $buildRoot
        Put (Join-Path $buildRoot '.gitignore') ".build/`n"
        Put (Join-Path $buildRoot 'RedTilde/RedTilde.vcxproj') '<Project />'
        Put (Join-Path $buildRoot 'Build/RedTilde.DxUi.props') '<Project />'
        Put (Join-Path $buildRoot 'Tests/test-scopes.json') ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'test-scopes.json')))
        Put (Join-Path $buildRoot 'Common/Version.h') "#define REDTILDE_VERSION_MAJOR 0`n#define REDTILDE_VERSION_MINOR 0`n"
        Put (Join-Path $buildRoot 'Tests/GuardFixture.cpp') 'first source bytes'
        foreach($name in @('Harness.psm1','DxUiPaths.psm1')){Copy-Item -LiteralPath (Join-Path $root "Build/$name") -Destination (Join-Path $buildRoot "Build/$name")}
        # Native commands remain stubbed. Use the shared planner implementation for real source, artifact and lease controls.
        $planner=(Join-Path $root '.ci-engine/Tools/Modules/Ci.ScopedTesting.psm1').Replace("'","''")
        $moduleText=@"
`$shared=Import-Module '$planner' -Force -Scope Local -PassThru
Export-ModuleMember -Function @(`$shared.ExportedFunctions.Keys)
"@
        Put (Join-Path $buildRoot 'Build/ScopedTesting.psm1') $moduleText
        # Build/BuildConsole.psm1 is an engine module shim; this mocked-engine fixture takes the implementation itself.
        Copy-Item -LiteralPath (Join-Path $root '.ci-engine/Tools/Modules/Ci.BuildConsole.psm1') -Destination (Join-Path $buildRoot 'Build/BuildConsole.psm1')
        Copy-Item -LiteralPath (Join-Path $root 'build.ps1') -Destination (Join-Path $buildRoot 'build.ps1')
        # Keep this input/receipt fixture compiler-free. The real shared bridge
        # has its own process/argv/service controls; this adapter records the
        # actual build entrypoint's clean/build calls through the same boundary.
        Put (Join-Path $buildRoot '.ci-engine/Tools/Modules/Ci.Policy.psm1') @'
function Invoke-CiNativeCommand {
    param([string]$FilePath,[string]$WorkingDirectory,[string[]]$Arguments)
    if($FilePath -cne (Join-Path $WorkingDirectory '.build/mock-msbuild.ps1')){throw 'Unexpected fixture native target.'}
    Push-Location -LiteralPath $WorkingDirectory
    try { & $FilePath @Arguments } finally { Pop-Location }
}
Export-ModuleMember -Function Invoke-CiNativeCommand
'@
        Put (Join-Path $buildRoot 'Build/Toolchain.psm1') @'
Set-StrictMode -Version Latest
function Get-RedTildeMSBuild { Join-Path (Split-Path $PSScriptRoot -Parent) '.build/mock-msbuild.ps1' }
function Assert-RedTildeOutputNotRunning { param([string]$OutputDirectory) }
Export-ModuleMember -Function Get-RedTildeMSBuild,Assert-RedTildeOutputNotRunning
'@
        Put (Join-Path $buildRoot 'restore-dxui.ps1') @'
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '.build/restore-called'),'restore reached')
'@
        Put (Join-Path $buildRoot '.build/mock-msbuild.ps1') @'
$root=Split-Path $PSScriptRoot -Parent
$platform=([string](@($args|Where-Object{$_ -like '/p:Platform=*'})[0]) -split '=',2)[1]
$configuration=([string](@($args|Where-Object{$_ -like '/p:Configuration=*'})[0]) -split '=',2)[1]
$output=Join-Path $root ".build/$platform/$configuration"
$intermediate=Join-Path $root ".build/Intermediate/$platform/$configuration/RedTilde"
$tools=Join-Path $root 'Tools'
if($args -contains '-getProperty:OutDir,IntDir'){
    if(Test-Path (Join-Path $root '.build/mock-clean-redirect')){$output=Join-Path $root '.build/protected-profile'}
    @{Properties=@{OutDir=$output+'\';IntDir=$intermediate+'\'}}|ConvertTo-Json -Depth 3 -Compress|Write-Output
    $global:LASTEXITCODE=0;return
}
if($args -match '^-getProperty:'){
    $executablePath=((Join-Path $tools 'VC/bin/HostX86/x64')+';'+(Join-Path $tools 'SDK/bin/10.0.26100.0/x86'))
    if(Test-Path (Join-Path $root '.build/mock-foreign-first')){$executablePath=(Join-Path $root '.build/foreign')+';'+$executablePath}
    $dependency=Get-Content (Join-Path $root '.build/dependencies/DxUi/DxUi.identity.x64.json') -Raw|ConvertFrom-Json
    [xml]$resolved=Get-Content (Join-Path $root '.build/dependencies/DxUi/DxUi.resolved.x64.props') -Raw
    $source=[string]$resolved.Project.PropertyGroup.DxUiRoot
    $dependencyOutput=[string]$resolved.Project.PropertyGroup.DxUiConsumerOutputRoot
    $lockFile=[string]$resolved.Project.PropertyGroup.DxUiConsumerLockFile
    [ordered]@{Properties=[ordered]@{OutDir=$output+'\';IntDir=$intermediate+'\';PlatformToolset='v145';VCToolsInstallDir=(Join-Path $tools 'VC')+'\';
        VCToolsVersion='14.50.00000';WindowsSdkDir=(Join-Path $tools 'SDK')+'\';WindowsTargetPlatformVersion='10.0.26100.0';PreferredToolArchitecture='x86';
        ExecutablePath=$executablePath;
        CLToolPath='';CLToolExe='';LinkToolPath='';LinkToolExe='';ResourceCompileToolPath='';ResourceCompileToolExe='';
        DxUiRoot=$source;DxUiResolvedRoot=$source+'\';DxUiConsumerOutputRoot=$dependencyOutput+'\';DxUiConsumerLockFile=$lockFile;
        DxUiDisableStlAnnotations='false';DxUiConsumerToolset=[string]$resolved.Project.PropertyGroup.DxUiConsumerToolset;
        DxUiConsumerVCToolsVersion=[string]$resolved.Project.PropertyGroup.DxUiConsumerVCToolsVersion;
        DxUiConsumerSdkVersion=[string]$resolved.Project.PropertyGroup.DxUiConsumerSdkVersion;
        DxUiConsumerPreferredToolArchitecture=[string]$resolved.Project.PropertyGroup.DxUiConsumerPreferredToolArchitecture;DxUiConsumerTriplet='x64-windows'}}|ConvertTo-Json -Depth 4 -Compress|Write-Output
    $global:LASTEXITCODE=0;return
}
$calls=Join-Path $root '.build/mock-calls.txt';[IO.File]::AppendAllText($calls,($args -join ' ') + "`n")
if($args -contains '/t:Clean'){
    $logOption=[string](@($args|Where-Object{$_ -like '/flp:logfile=*'})[0])
    $nativeLog=($logOption.Substring('/flp:logfile='.Length) -split ';',2)[0]
    [IO.File]::WriteAllText($nativeLog,'Fixture profile clean completed.')
    if(Test-Path $intermediate){Remove-Item -LiteralPath $intermediate -Recurse -Force}
    $exe=Join-Path $output 'RedTilde.exe';if(Test-Path $exe){Remove-Item -LiteralPath $exe -Force}
    $global:LASTEXITCODE=0;return
}
if(Test-Path (Join-Path $root '.build/mock-fail')){$global:LASTEXITCODE=9;return}
[void](New-Item -ItemType Directory -Path $output -Force);[void](New-Item -ItemType Directory -Path $intermediate -Force)
$logOption=[string](@($args|Where-Object{$_ -like '/flp:logfile=*'})[0])
$nativeLog=($logOption.Substring('/flp:logfile='.Length) -split ';',2)[0]
[IO.File]::WriteAllText($nativeLog,$(if(Test-Path (Join-Path $root '.build/mock-warning')){'fixture.cpp(1): warning C4100: injected diagnostic'}else{'Fixture native build completed.'}))
$token=Join-Path $intermediate 'compiled-input.hash';$exe=Join-Path $output 'RedTilde.exe'
if(-not(Test-Path $token)){
    $sourceHash=(Get-FileHash (Join-Path $root 'Tests/GuardFixture.cpp')).Hash
    $headerHash=(Get-FileHash (Join-Path $root '.build/dependencies/DxUi/aaaaaaaaaaaaaaaa/vcpkg_installed/x64/x64-windows/include/guard.h')).Hash
    $compilerHash=(Get-FileHash (Join-Path $root 'Tools/VC/bin/HostX86/x64/cl.exe')).Hash
    $sdkCommit=([string](Get-Content (Join-Path $root '.build/dependencies/DxUi/DxUi.identity.x64.json') -Raw|ConvertFrom-Json).Identity.commit)
    $sdkSourceHash=(Get-FileHash (Join-Path $root ".build/dependencies/DxUi/source/$($sdkCommit.Substring(0,16))/src/GuardFixture.cpp")).Hash
    $bytes=[Text.Encoding]::UTF8.GetBytes($sourceHash+$headerHash+$compilerHash+$sdkSourceHash)
    $compiled=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))
    [IO.File]::WriteAllText($token,$compiled);[IO.File]::WriteAllText($exe,$compiled)
}
$global:LASTEXITCODE=0
'@
        foreach($tool in @('VC/bin/HostX86/x64/cl.exe','VC/bin/HostX86/x64/link.exe','SDK/bin/10.0.26100.0/x86/rc.exe')){Put (Join-Path $buildRoot "Tools/$tool") "tool $tool"}
        $sdkRoot=Join-Path $buildRoot '.build/dependencies/DxUi/source';[void](New-Item -ItemType Directory -Path $sdkRoot -Force)
        $sdkSource=Join-Path $sdkRoot 'fixture-source';New-Repository $sdkSource;Put (Join-Path $sdkSource 'include/source.h') 'source header';Put (Join-Path $sdkSource 'src/GuardFixture.cpp') 'pinned SDK implementation bytes';Put (Join-Path $sdkSource 'capabilities.json') '{"apiRevision":4}'
        Invoke-FixtureGit $sdkSource @('add','-A');Invoke-FixtureGit $sdkSource @('commit','-qm','fixture SDK source and header')
        $commit=(& git -C $sdkSource rev-parse HEAD).Trim();$sdkSourceCommitted=Join-Path $sdkRoot $commit.Substring(0,16);Move-Item -LiteralPath $sdkSource -Destination $sdkSourceCommitted
        $fingerprint=('a'*64) -join ''
        $identity=@{commit=$commit;apiRevision=4;platform='x64';toolset='v145';vcToolsVersion='14.50.00000';windowsSdkVersion='10.0.26100.0';runtimeFamily='MD';preferredToolArchitecture='x86';disableStlAnnotations=$false}
        Import-Module (Join-Path $buildRoot 'Build/DxUiPaths.psm1') -Force
        $verifiedOutput=Initialize-DxUiOutputRoot -DependencyRoot (Join-Path $buildRoot '.build/dependencies/DxUi') -BuildIdentity @{Fingerprint=$fingerprint;Identity=$identity}
        $packageInclude=Join-Path $verifiedOutput 'vcpkg_installed/x64/x64-windows/include'
        Put (Join-Path $packageInclude 'guard.h') 'package header v1'
        Put (Join-Path $buildRoot '.build/dependencies/DxUi/DxUi.identity.x64.json') (@{Fingerprint=$fingerprint;Identity=$identity}|ConvertTo-Json -Depth 4)
        Put (Join-Path $buildRoot 'Dependencies/DxUi.lock.json') (@{repository='https://github.com/RedSalamanders/DxUi';commit=$commit;apiRevision=4;targets=@('DxUi')}|ConvertTo-Json -Depth 4)
        $resolvedOutput=(Join-Path $buildRoot ".build/dependencies/DxUi/$($fingerprint.Substring(0,16))")+'\'
        $resolvedXml='<Project><PropertyGroup><DxUiRoot>'+[Security.SecurityElement]::Escape($sdkSourceCommitted)+'</DxUiRoot><DxUiConsumerOutputRoot>'+[Security.SecurityElement]::Escape($resolvedOutput)+'</DxUiConsumerOutputRoot><DxUiConsumerLockFile>'+[Security.SecurityElement]::Escape((Join-Path $buildRoot 'Dependencies/DxUi.lock.json'))+'</DxUiConsumerLockFile><DxUiConsumerToolset>v145</DxUiConsumerToolset><DxUiConsumerVCToolsVersion>14.50.00000</DxUiConsumerVCToolsVersion><DxUiConsumerSdkVersion>10.0.26100.0</DxUiConsumerSdkVersion><DxUiConsumerPreferredToolArchitecture>x86</DxUiConsumerPreferredToolArchitecture></PropertyGroup></Project>'
        Put (Join-Path $buildRoot '.build/dependencies/DxUi/DxUi.resolved.x64.props') $resolvedXml
        Invoke-FixtureGit $buildRoot @('add','-A');Invoke-FixtureGit $buildRoot @('commit','-qm','fixture build inputs')
        $buildBinary=Join-Path $PSHOME 'pwsh.exe'
        function Invoke-BuildFixture([switch]$NoRestore,[string[]]$Extra=@()){
            $start=[Diagnostics.ProcessStartInfo]::new($buildBinary);$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
            # Hosted qualification supplies a nonzero product build number. The mocked text executable has no
            # version resource, so freeze zero explicitly while retaining a hostile inherited value as a regression.
            $start.Environment['RedTildeBuildNumber']='137'
            $arguments=@('-NoProfile','-File',(Join-Path $buildRoot 'build.ps1'),'-Platform','x64','-Configuration','Debug','-BuildNumber','0')
            if(-not $NoRestore){$arguments+= '-SkipRestore'}
            foreach($argument in $arguments+$Extra){$start.ArgumentList.Add($argument)}
            $process=[Diagnostics.Process]::Start($start)
            try{$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync();if(-not $process.WaitForExit(30000)){$process.Kill($true);throw 'Mocked public build fixture timed out'};[pscustomobject]@{exit=$process.ExitCode;text=$stdout.GetAwaiter().GetResult()+$stderr.GetAwaiter().GetResult()}}finally{$process.Dispose()}
        }
        function Assert-GuardArtifact([string]$Reason,[switch]$Allowed,[uint32]$BuildNumber=0){
            $pwsh=Join-Path $PSHOME 'pwsh.exe'
            $probe=Join-Path $buildRoot '.build/assert-artifact.ps1'
            Put $probe "Import-Module '$($buildRoot.Replace("'","''"))/Build/Harness.psm1' -Force; Assert-RedTildeTestArtifact -Root '$($buildRoot.Replace("'","''"))' -Platform x64 -Configuration Debug"
            $start=[Diagnostics.ProcessStartInfo]::new($pwsh);$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
            # Admission must use the same explicit synthetic version as the build. Inheriting the hosted
            # product version would reject intact fixtures and make unrelated negative controls vacuous.
            $start.Environment['RedTildeBuildNumber']=[string]$BuildNumber
            foreach($argument in @('-NoProfile','-File',$probe)){$start.ArgumentList.Add($argument)}
            $process=[Diagnostics.Process]::Start($start)
            try{
                $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
                if(-not $process.WaitForExit(30000)){$process.Kill($true);throw 'Mocked artifact admission timed out'}
                $output=$stdout.GetAwaiter().GetResult()+$stderr.GetAwaiter().GetResult()
                if($Allowed){Assert ($process.ExitCode -eq 0) "Current build receipt refused for ${Reason}: $output"}
                else{Assert ($process.ExitCode -ne 0) "Stale build receipt admitted after $Reason"}
            }finally{$process.Dispose()}
        }
        Case 'public build refuses a profile junction before receipt or restore writes' {
            $protected=Join-Path $buildRoot '.build/protected-profile';Put (Join-Path $protected 'marker.txt') 'keep this target unchanged'
            $profileAlias=Join-Path $buildRoot '.build/x64';[void](New-Item -ItemType Junction -Path $profileAlias -Target $protected)
            try{
                $run=Invoke-BuildFixture -NoRestore;Assert ($run.exit -ne 0) 'Reparse-point profile path was accepted'
                Assert ((Get-Content (Join-Path $protected 'marker.txt') -Raw) -ceq 'keep this target unchanged') 'Profile preflight changed the protected target'
                Assert (-not(Test-Path (Join-Path $protected 'Debug/build-receipt.json'))) 'Profile preflight wrote the receipt through a junction'
                Assert (-not(Test-Path (Join-Path $buildRoot '.build/restore-called'))) 'Profile preflight reached dependency restore before path validation'
            }finally{
                if(Test-Path -LiteralPath $profileAlias){[IO.Directory]::Delete($profileAlias,$false)}
            }
            Assert (Test-Path -LiteralPath (Join-Path $protected 'marker.txt')) 'Removing the test junction removed its owned target'
        }
        Case 'first build cleans and records guarded receipt' {
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Fixture initial build failed: $($run.text)"
            $receipt=Get-Content (Join-Path $buildRoot '.build/x64/Debug/build-receipt.json') -Raw|ConvertFrom-Json
            Assert ($receipt.schemaVersion -eq 2 -and $receipt.passed) 'Build did not publish the guarded receipt'
            Assert ($receipt.buildNumber -eq 0) 'Mock build inherited the hosted product version instead of its explicit fixture version'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 1) 'Unattested first build did not clean owned profile'
        }
        Case 'docs-only changes reuse byte-identical output without clean' {
            $exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put (Join-Path $buildRoot 'README.md') 'documentation only';Invoke-FixtureGit $buildRoot @('add','README.md')
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Docs-only build failed: $($run.text)"
            Assert ((Get-FileHash $exe).Hash -ceq $before) 'Docs-only build changed executable bytes'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 1) 'Docs-only edit unnecessarily cleaned profile'
            Assert-GuardArtifact 'documentation-only build' -Allowed
        }
        Case 'artifact admission uses the explicit fixture version and refuses a mismatched product version' {
            Assert-GuardArtifact 'matching fixture version zero' -Allowed
            Assert-GuardArtifact 'mismatched product version' -BuildNumber 137
        }
        Case 'source bytes with a restored older mtime clean before MSBuild reuse' {
            $source=Join-Path $buildRoot 'Tests/GuardFixture.cpp';$oldTime=[IO.File]::GetLastWriteTimeUtc($source);$exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put $source 'changed compiled source bytes';[IO.File]::SetLastWriteTimeUtc($source,$oldTime)
            Assert-GuardArtifact 'source bytes changed with an old timestamp'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Restored-mtime build failed: $($run.text)"
            Assert ((Get-FileHash $exe).Hash -cne $before) 'Changed source bytes were reused from old object despite timestamp restoration'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 2) 'Changed source did not clean owned profile'
        }
        Case 'changed installed package-header bytes invalidate the build input identity' {
            $exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put (Join-Path $packageInclude 'guard.h') 'package header v2'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Changed header build failed: $($run.text)"
            Assert ((Get-FileHash $exe).Hash -cne $before) 'Changed installed header was reused'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 3) 'Changed dependency header did not clean owned profile'
        }
        Case 'changed selected compiler bytes invalidate the build context' {
            $exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put (Join-Path $buildRoot 'Tools/VC/bin/HostX86/x64/cl.exe') 'updated compiler bytes'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Changed compiler build failed: $($run.text)"
            Assert ((Get-FileHash $exe).Hash -cne $before) 'Changed compiler was reused'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 4) 'Changed compiler context did not clean owned profile'
        }
        Case 'dirty tracked or untracked pinned SDK implementation source refuses build reuse' {
            $trackedSource=Join-Path $sdkSourceCommitted 'src/GuardFixture.cpp'
            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            Put $trackedSource 'dirty tracked SDK implementation'
            Assert-GuardArtifact 'dirty pinned SDK implementation source'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Dirty tracked SDK source was accepted by the public build entrypoint'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Dirty SDK source reached profile clean'
            Assert (-not (Get-Content (Join-Path $buildRoot '.build/x64/Debug/build-receipt.json') -Raw|ConvertFrom-Json).passed) 'Dirty SDK source retained a passed receipt'
            Invoke-FixtureGit $sdkSourceCommitted @('checkout','--','src/GuardFixture.cpp')
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Clean pinned SDK source did not recover: $($run.text)"

            $untrackedSource=Join-Path $sdkSourceCommitted 'src/Unexpected.cpp';Put $untrackedSource 'untracked SDK implementation'
            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            Assert-GuardArtifact 'untracked pinned SDK implementation source'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Untracked SDK source was accepted by the public build entrypoint'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Untracked SDK source reached profile clean'
            Remove-Item -LiteralPath $untrackedSource -Force
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Untracked SDK source removal did not recover: $($run.text)"
        }
        Case 'resolved DxUi consumer properties cannot redirect the SDK source or package root' {
            $propsPath=Join-Path $buildRoot '.build/dependencies/DxUi/DxUi.resolved.x64.props'
            $original=[IO.File]::ReadAllText($propsPath)
            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            $redirected=$original.Replace([Security.SecurityElement]::Escape($sdkSourceCommitted),[Security.SecurityElement]::Escape((Join-Path $buildRoot '.build/foreign-sdk')))
            Put $propsPath $redirected
            Assert-GuardArtifact 'redirected DxUiRoot in resolved properties'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Redirected SDK source root was accepted'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Redirected resolved properties reached profile clean'
            Put $propsPath $original
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Restored resolved properties did not recover: $($run.text)"

            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            $redirectedOutput=(Join-Path $buildRoot '.build/foreign-package')+'\'
            $redirected=$original.Replace([Security.SecurityElement]::Escape($resolvedOutput),[Security.SecurityElement]::Escape($redirectedOutput))
            Put $propsPath $redirected
            Assert-GuardArtifact 'redirected DxUiConsumerOutputRoot in resolved properties'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Redirected package root was accepted'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Redirected package root reached profile clean'
            Put $propsPath $original
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Restored package root did not recover: $($run.text)"
        }
        Case 'foreign first executable search match fails closed instead of attesting a later compiler' {
            $output=Join-Path $buildRoot '.build/x64/Debug';$receiptPath=Join-Path $output 'build-receipt.json'
            $foreign=Join-Path $buildRoot '.build/foreign';Put (Join-Path $foreign 'cl.exe') 'foreign compiler'
            Put (Join-Path $buildRoot '.build/mock-foreign-first') 'enable foreign-first search path'
            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Foreign first compiler search match was accepted'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Unverified compiler context reached profile clean'
            Assert (-not (Get-Content $receiptPath -Raw|ConvertFrom-Json).passed) 'Unknown tool context retained a passed receipt'
            Remove-Item -LiteralPath (Join-Path $buildRoot '.build/mock-foreign-first') -Force
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Build did not recover after restoring selected compiler search path: $($run.text)"
        }
        Case 'legacy or incomplete receipts force a clean and are never admitted for SkipBuild' {
            $receiptPath=Join-Path $buildRoot '.build/x64/Debug/build-receipt.json';$receipt=Get-Content $receiptPath -Raw|ConvertFrom-Json
            $receipt.schemaVersion=1;$receipt|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $receiptPath -Encoding utf8
            Assert-GuardArtifact 'legacy receipt'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Legacy receipt recovery failed: $($run.text)"
            $receipt=Get-Content $receiptPath -Raw|ConvertFrom-Json;$receipt.buildInputs=$null
            $receipt|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $receiptPath -Encoding utf8
            Assert-GuardArtifact 'incomplete receipt'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Incomplete receipt recovery failed: $($run.text)"
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 11) 'Invalid receipt did not force clean'
        }
        Case 'failed build leaves a revoked receipt that SkipBuild admission rejects' {
            Put (Join-Path $buildRoot '.build/mock-fail') 'force failure';$run=Invoke-BuildFixture
            Assert ($run.exit -ne 0) 'Mock build failure reported success'
            $receipt=Get-Content (Join-Path $buildRoot '.build/x64/Debug/build-receipt.json') -Raw|ConvertFrom-Json
            Assert (-not $receipt.passed) 'Failed attempt retained a passed build receipt'
            Assert-GuardArtifact 'failed build attempt'
        }
        Case 'explicit rebuild cleans a compatible profile and forwards the worker limit' {
            Remove-Item -LiteralPath (Join-Path $buildRoot '.build/mock-fail') -Force
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Fixture recovery failed: $($run.text)"
            $run=Invoke-BuildFixture -Extra @('-Rebuild','-MaxCpuCount','2')
            Assert ($run.exit -eq 0) "Explicit rebuild failed: $($run.text)"
            $calls=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt') -Tail 2)
            Assert ($calls.Count -eq 2 -and $calls[0] -match '/t:Clean' -and $calls[0] -match '/p:BuildProjectReferences=false' -and
                $calls[1] -notmatch '/t:Clean') 'Explicit rebuild did not run selected Clean before Build'
            Assert (@($calls|Where-Object{$_ -match '/m:2' -and $_ -match '/p:CL_MPCount=2'}).Count -eq 2) 'Worker limit was lost'
            Assert-GuardArtifact 'explicit rebuild' -Allowed
        }
        Case 'explicit clean rejects redirected paths before cleanup and revokes the selected receipt' {
            $log=Join-Path $buildRoot '.build/mock-calls.txt'
            $callsBefore=@(Get-Content $log).Count
            $redirect=Join-Path $buildRoot '.build/mock-clean-redirect'
            Put $redirect 'redirected evaluated path'
            $run=Invoke-BuildFixture -NoRestore -Extra @('-Clean')
            Assert ($run.exit -ne 0 -and $run.text -match 'not the owned profile paths') 'Explicit clean accepted a foreign evaluated output'
            Assert (@(Get-Content $log).Count -eq $callsBefore) 'The refused clean reached the build graph'
            Remove-Item -LiteralPath $redirect -Force
            $run=Invoke-BuildFixture -NoRestore -Extra @('-Clean','-MaxCpuCount','2')
            Assert ($run.exit -eq 0) "Explicit clean failed: $($run.text)"
            $calls=@(Get-Content $log)
            Assert ($calls.Count -eq $callsBefore+1 -and $calls[-1] -match '/t:Clean' -and $calls[-1] -match '/p:BuildProjectReferences=false') 'Explicit clean invoked work outside its selected project'
            Assert (-not(Test-Path (Join-Path $buildRoot '.build/restore-called'))) 'Explicit clean invoked dependency restore'
            Assert-GuardArtifact 'explicit clean'
        }
    }
    Write-Host "Scoped testing: $passed cases passed."
}finally{
    $absolute=[IO.Path]::GetFullPath($fixture);$allowed=[IO.Path]::GetFullPath((Join-Path $root '.build/ToolTests'))+[IO.Path]::DirectorySeparatorChar
    if(-not $absolute.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture cleanup escaped its owned root'}
    if(Test-Path -LiteralPath $absolute){Remove-Item -LiteralPath $absolute -Recurse -Force}
}
exit 0
