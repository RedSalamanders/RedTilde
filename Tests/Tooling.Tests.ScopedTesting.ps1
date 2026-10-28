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
    Case 'active native naming and inventory are contextual and complete' {
        Assert ((Assert-ScopedTestNames $root) -gt 0) 'Empty inventory'
        $paths=@(Get-Content (Join-Path $root $manifest.nativeInventory) -Raw|ConvertFrom-Json)
        if($manifest.project -ceq 'RedTilde'){$paths=@($paths[0].files.path)}
        Assert (@($paths|Where-Object{$_ -match '(^|/)Scope\.Tests\.'}).Count -eq 0) 'Placeholder Scope was retained instead of a context'
    }
    Case 'unknown executable inputs widen even though formatting also matches' {
        foreach($path in @('Unmapped/New.cpp','Unmapped/New.h','Specs/Unmapped.schema.json','NewGenerator.inl')){
            Assert ((Get-ScopedTestPlan $manifest @($path)).full) "Unmapped input narrowed coverage: $path"
        }
    }
    Case 'prose omits native tests, declared runtime Markdown selects its consumer, and empty changes select nothing' {
        $plan=Get-ScopedTestPlan $manifest @('README.md','CHANGELOG.md','docs/examples/ordinary-notes.md')
        Assert (@($manifest.scopes|Where-Object{$_.native -and $_.name -in $plan.scopes}).Count -eq 0) 'Prose selected native tests'
        foreach($scope in @('Tooling','Format')){Assert ($scope -in $plan.scopes) "Prose validation omitted: $scope"}
        if($manifest.project -ceq 'RedTilde'){
            $plan=Get-ScopedTestPlan $manifest @('docs/examples/weekly-report-2026-10-15.md')
            foreach($scope in @('Captures','Tooling','Format')){Assert ($scope -in $plan.scopes) "Runtime capture fixture consumer omitted: $scope"}
            $native=@($manifest.scopes|Where-Object{$_.native -and $_.name -in $plan.scopes})
            Assert (-not $plan.full -and $native.Count -eq 1 -and $native[0].name -ceq 'Captures') 'Runtime fixture widened unrelated native coverage'
        }
        Assert ((Get-ScopedTestPlan $manifest @('res/Welcome.md')).full) 'Embedded content mistaken for prose'
        Assert ((Get-ScopedTestPlan $manifest @()).scopes.Count -eq 0) 'Empty changes selected tests'
    }
    Case 'actual consumers and standalone test owners are selected' {
        if($manifest.project -ceq 'RedPrism'){
            $plan=Get-ScopedTestPlan $manifest @('Common/Jobs.cpp')
            foreach($consumer in @('Host','Runtime')){Assert ($consumer -in $plan.scopes) "Job consumer omitted: $consumer"}
            $test=@(Get-Content (Join-Path $root $manifest.nativeInventory) -Raw|ConvertFrom-Json)|Where-Object{$_ -like 'Tests/HostTests/*.h'}|Select-Object -First 1
            $plan=Get-ScopedTestPlan $manifest @($test)
            Assert ('Host' -in $plan.scopes -and 'IO' -notin $plan.scopes) 'A standalone Host helper ran unrelated IO'
        }else{
            $plan=Get-ScopedTestPlan $manifest @('Common/Storage.cpp')
            foreach($consumer in @('Storage','Recovery','RecoveryRestart')){Assert ($consumer -in $plan.scopes) "Storage consumer omitted: $consumer"}
            Assert ('Captures' -notin $plan.scopes -and 'Editor' -notin $plan.scopes) 'Storage-only edit selected unrelated native render cases'
            $plan=Get-ScopedTestPlan $manifest @('Tests/Application.Tests.Performance.cpp')
            Assert ('Editor' -in $plan.scopes -and 'Performance' -in $plan.scopes) 'Native performance helper lost its behavioral consumer'
            foreach($path in @('Common/Files.cpp','Common/Files.h','Common/RemoteImage.cpp','Common/RemoteImage.h')){
                $plan=Get-ScopedTestPlan $manifest @($path)
                Assert ('Storage' -in $plan.scopes -and 'Editor' -in $plan.scopes -and 'Crash' -notin $plan.scopes) "Service consumer omitted or widened: $path"
            }
            $plan=Get-ScopedTestPlan $manifest @('Tests/Support/Support.Tests.FilesFixtures.h')
            Assert ('Storage' -in $plan.scopes -and 'Editor' -in $plan.scopes -and 'Captures' -in $plan.scopes) 'Shared folder fixture lost a consumer'
            $catalog=Get-Content (Join-Path $root 'Tests/native-case-catalog.json') -Raw|ConvertFrom-Json
            # The reviewed native inventory owns registration units, including
            # split suites; a single-file scan silently omitted new Core cases.
            $native=Get-Content (Join-Path $root $manifest.nativeInventory) -Raw|ConvertFrom-Json
            $declared=@(foreach($unit in @($native.files|Where-Object{$_.suite -ceq 'Core' -and $_.path.EndsWith('.cpp')})){
                [regex]::Matches([IO.File]::ReadAllText((Join-Path $root $unit.path)),'\btest\s*\(\s*"([^"]+)"')|ForEach-Object{$_.Groups[1].Value}
            })
            Assert (-not(Compare-Object $declared @($catalog.cases.name))) 'Reviewed native catalog differs from source'
            Assert (@($catalog.cases.name|Sort-Object -Unique).Count -eq $declared.Count) 'Duplicate native case'
        }
    }
    Case 'invalid selectors and absolute/traversing paths are rejected' {
        foreach($action in @({Get-ScopedTestPlan $manifest @() @('Unknown')},{Get-ScopedTestPlan $manifest @('../code.cpp')},{Get-ScopedTestPlan $manifest @('C:/code.cpp')})){
            $threw=$false;try{&$action|Out-Null}catch [System.Management.Automation.RuntimeException]{$threw=$true}
            Assert $threw 'Invalid selector/path accepted'
        }
    }
    $gitRoot=Join-Path $fixture 'discovery';New-Repository $gitRoot
    foreach($name in @('code.cpp','staged.cpp','deleted.cpp','rename 東京.cpp')){Put (Join-Path $gitRoot $name) 'initial'}
    $longName='long.cpp';$longSegments=[System.Collections.Generic.List[string]]::new()
    $longPath=Join-Path $gitRoot $longName
    while ([IO.Path]::GetFullPath($longPath).Length -le 260 -and $longSegments.Count -lt 8) {
        $segment=('segment{0:D2}-' -f ($longSegments.Count+1))+('x'*20)
        $longSegments.Add($segment)
        $longPath=$gitRoot
        foreach($part in $longSegments){$longPath=Join-Path $longPath $part}
        $longPath=Join-Path $longPath $longName
    }
    $longRelative=(@($longSegments.ToArray())+@($longName)) -join '/'
    $components=@($longSegments.ToArray())+@($longName)
    Assert ($longSegments.Count -le 8 -and [IO.Path]::GetFullPath($longPath).Length -gt 260) 'Long-path fixture did not exceed the Windows path limit within eight segments'
    Assert (@($components|Where-Object{$_.Length -ge 255}).Count -eq 0) 'Long-path fixture component exceeded the Windows filename limit'
    Put $longPath 'tracked long-path fixture'
    Invoke-FixtureGit $gitRoot @('add','-A');Invoke-FixtureGit $gitRoot @('commit','-qm','baseline with long path')
    $tracked=@(& git -C $gitRoot ls-tree -r --name-only HEAD)
    if($LASTEXITCODE -ne 0){throw 'Fixture Git failed: ls-tree'}
    Assert ($longRelative -cin $tracked) 'Long-path fixture was not committed'
    $base=(& git -C $gitRoot rev-parse HEAD).Trim()
    Put (Join-Path $gitRoot 'committed.cpp') 'committed';Invoke-FixtureGit $gitRoot @('add','-A');Invoke-FixtureGit $gitRoot @('commit','-qm','candidate')
    Put (Join-Path $gitRoot 'staged.cpp') 'staged';Invoke-FixtureGit $gitRoot @('add','staged.cpp');Put (Join-Path $gitRoot 'staged.cpp') 'initial'
    Put (Join-Path $gitRoot 'code.cpp') 'working';Remove-Item -LiteralPath (Join-Path $gitRoot 'deleted.cpp')
    Move-Item -LiteralPath (Join-Path $gitRoot 'rename 東京.cpp') -Destination (Join-Path $gitRoot 'renamed 東京.cpp')
    Put (Join-Path $gitRoot 'untracked space.cpp') 'new'
    Case 'committed/staged/working/deleted/rename sides/Unicode/untracked are independently discovered' {
        $paths=@(Get-ScopedChangedPaths $gitRoot $base)
        foreach($name in @('committed.cpp','staged.cpp','code.cpp','deleted.cpp','rename 東京.cpp','renamed 東京.cpp','untracked space.cpp')){Assert ($name -cin $paths) "Missing change: $name"}
    }
    Case 'missing comparison refs fail with a full-run remedy' {
        $message='';try{Get-ScopedChangedPaths $gitRoot missing-ref|Out-Null}catch [System.Management.Automation.RuntimeException]{$message=$_.Exception.Message}
        Assert ($message -match 'BaseRef|full gate|fetch') 'Missing base silently selected nothing'
    }
    $mini=[ordered]@{version=1;project='Fixture';repository='fixture/example';defaultBranch='main';nativeInventory='Tests/native-test-files.json';runtimeFiles=@('input.json');
        scopes=@(@{name='Example';native=$true;reuse=$true});rules=@();toolingCommands=@();prCoverage=@();prWorkflowDigest='none'}
    Put (Join-Path $gitRoot 'Tests/test-scopes.json') ($mini|ConvertTo-Json -Depth 6)
    foreach($testConfiguration in @('Debug','Release')){Put (Join-Path $gitRoot ".build/x64/$testConfiguration/example.exe") 'binary';Put (Join-Path $gitRoot ".build/x64/$testConfiguration/input.json") 'runtime'}
    Case 'native identity changes with source/binary/runtime/profile/environment' {
        $before=Get-ScopedRunIdentity $gitRoot x64 Debug Example
        Put (Join-Path $gitRoot '.build/x64/Debug/example.exe') 'changed binary'
        Assert ((Get-ScopedRunIdentity $gitRoot x64 Debug Example) -cne $before) 'Binary mutation omitted'
        Put (Join-Path $gitRoot '.build/x64/Debug/example.exe') 'binary';Put (Join-Path $gitRoot '.build/x64/Debug/input.json') 'changed runtime'
        Assert ((Get-ScopedRunIdentity $gitRoot x64 Debug Example) -cne $before) 'Runtime mutation omitted'
        Put (Join-Path $gitRoot '.build/x64/Debug/input.json') 'runtime'
        Assert ((Get-ScopedRunIdentity $gitRoot x64 Release Example) -cne $before) 'Profile omitted'
        $old=$env:ASAN_OPTIONS
        try{$env:ASAN_OPTIONS='scope-fixture=changed';Assert ((Get-ScopedRunIdentity $gitRoot x64 Debug Example) -cne $before) 'Sanitizer environment omitted'}finally{$env:ASAN_OPTIONS=$old}
        Put (Join-Path $gitRoot 'code.cpp') 'changed source';Assert ((Get-ScopedRunIdentity $gitRoot x64 Debug Example) -cne $before) 'Source omitted'
    }
    Case 'missing required runtime inputs and JSON-only profiles fail before identity creation' {
        Remove-Item -LiteralPath (Join-Path $gitRoot '.build/x64/Debug/input.json')
        $message='';try{Get-ScopedArtifactIdentity $gitRoot x64 Debug|Out-Null}catch [System.Management.Automation.RuntimeException]{$message=$_.Exception.Message}
        Assert ($message -like 'Missing deployed runtime input:*') 'Missing initial runtime closure accepted'
        Put (Join-Path $gitRoot '.build/x64/Debug/input.json') 'runtime';Remove-Item -LiteralPath (Join-Path $gitRoot '.build/x64/Debug/example.exe')
        $message='';try{Get-ScopedArtifactIdentity $gitRoot x64 Debug|Out-Null}catch [System.Management.Automation.RuntimeException]{$message=$_.Exception.Message}
        Assert ($message -eq 'No executable build artifacts were found.') 'JSON-only profile accepted'
        Put (Join-Path $gitRoot '.build/x64/Debug/example.exe') 'binary'
    }
    Case 'only intact identical successful receipts are reusable' {
        $path=Join-Path $gitRoot '.build/pass.json';$key=Get-ScopedDigest 'identity'
        Assert (-not(Test-ScopedReceipt $path $key)) 'Missing receipt reused'
        Write-ScopedReceipt $path $key;Assert (Test-ScopedReceipt $path $key) 'Identical pass not reused'
        Assert (-not(Test-ScopedReceipt $path (Get-ScopedDigest 'changed'))) 'Wrong identity reused'
        Put $path '{broken';Assert (-not(Test-ScopedReceipt $path $key)) 'Corrupt receipt reused'
        Write-ScopedReceipt $path $key;$record=Get-Content $path -Raw|ConvertFrom-Json;$record.outcome='FAILED';Put $path ($record|ConvertTo-Json)
        Assert (-not(Test-ScopedReceipt $path $key)) 'Failed receipt reused'
    }
    Case 'compiled identity excludes prose but includes embedded Markdown and inline generators' {
        $native=Get-ScopedSourceIdentity $gitRoot -CompiledOnly;$all=Get-ScopedSourceIdentity $gitRoot
        Put (Join-Path $gitRoot 'README.md') 'prose'
        Assert ((Get-ScopedSourceIdentity $gitRoot -CompiledOnly) -ceq $native) 'Prose invalidated compilation'
        Assert ((Get-ScopedSourceIdentity $gitRoot) -cne $all) 'Validator prose omitted'
        Put (Join-Path $gitRoot 'res/Welcome.md') 'embedded content'
        Assert ((Get-ScopedSourceIdentity $gitRoot -CompiledOnly) -cne $native) 'Embedded Markdown omitted'
        $native=Get-ScopedSourceIdentity $gitRoot -CompiledOnly;Put (Join-Path $gitRoot 'generator.inl') 'new generator'
        Assert ((Get-ScopedSourceIdentity $gitRoot -CompiledOnly) -cne $native) 'New executable input extension omitted'
    }
    Case 'iteration state is source/index/profile/environment bound and rejects corruption' {
        $path=Join-Path $gitRoot '.build/iteration.json';$source=Get-ScopedSourceMap $gitRoot;$index=Get-ScopedIndexMap $gitRoot
        Write-ScopedIteration $path $gitRoot x64 Debug environment $source $index artifact
        Assert ($null -ne (Read-ScopedIteration $path $gitRoot x64 Debug environment)) 'Valid iteration rejected'
        Assert ($null -eq (Read-ScopedIteration $path $gitRoot x64 Release environment)) 'Wrong profile accepted'
        Assert ($null -eq (Read-ScopedIteration $path $gitRoot x64 Debug changed)) 'Wrong environment accepted'
        Put $path '{}';Assert ($null -eq (Read-ScopedIteration $path $gitRoot x64 Debug environment)) 'Corrupt state accepted'
    }
    Case 'CI covers six exact native profiles and tooling once with no duplicate main-push run' {
        $workflow=[IO.File]::ReadAllText((Join-Path $root '.github/workflows/ci.yml')) -replace "`r`n","`n"
        Assert ((Get-ScopedDigest $workflow) -ceq $manifest.prWorkflowDigest) 'Workflow contract digest stale'
        Assert ($workflow.Contains('test.ps1 -Full -SkipTooling') -and $workflow.Contains('platform: [x64, ARM64]')) 'Full native matrix narrowed'
        Assert ($workflow -notmatch '(?m)^  push:') 'Identical merged tree automatically repeats the PR matrix'
        Assert ((@($manifest.prCoverage|Where-Object{$_.PSObject.Properties['platform']})).Count -eq 6) 'Missing native profile contracts'
        foreach($platform in @('x64','ARM64')){foreach($configuration in @('Debug','Release','ASan Debug')){
            $declared=@($manifest.prCoverage|Where-Object{$_.PSObject.Properties['platform'] -and $_.platform -eq $platform -and $_.configuration -eq $configuration})
            Assert ($declared.Count -eq 1) "Profile duplicated/omitted: $platform $configuration"
        }}
    }
    Case 'live project/include consumers follow added dependencies and terminate include cycles' {
        $graph=Join-Path $fixture 'graph'
        Put (Join-Path $graph 'host.vcxproj') '<Project><ItemGroup><ClCompile Include="host.cpp"/></ItemGroup></Project>'
        Put (Join-Path $graph 'document.vcxproj') '<Project><ItemGroup><ClCompile Include="document.cpp"/></ItemGroup></Project>'
        Put (Join-Path $graph 'host.cpp') 'int host;'
        Put (Join-Path $graph 'document.cpp') '#include "shared.h"'
        Put (Join-Path $graph 'shared.h') '#include "cycle.h"'
        Put (Join-Path $graph 'cycle.h') '#include "shared.h"'
        New-Repository $graph
        $bindings=@([pscustomobject]@{path='host.vcxproj';scope='Host'},[pscustomobject]@{path='document.vcxproj';scope='Document'})
        $before=@(Get-ScopedProjectRules $graph $bindings|Where-Object pattern -ceq 'shared.h')
        Assert ('Document' -in $before.scopes -and 'Host' -notin $before.scopes) 'Initial include graph is wrong'
        Put (Join-Path $graph 'host.cpp') '#include "shared.h"'
        $after=@(Get-ScopedProjectRules $graph $bindings|Where-Object pattern -ceq 'shared.h')
        Assert ('Document' -in $after.scopes -and 'Host' -in $after.scopes) 'New native caller was omitted after adding an include'
        Put (Join-Path $graph 'host.cpp') '#include SELECTED_HEADER'
        Assert (@(Get-ScopedProjectRules $graph $bindings|Where-Object{$_.pattern -ceq '**' -and '*' -in $_.scopes}).Count -eq 1) 'Macro include silently narrowed coverage'
    }
    $gate=Join-Path $fixture 'gate';New-Repository $gate
    $workflow="name: fixture`non:`n  pull_request:`n"
    Put (Join-Path $gate '.github/workflows/ci.yml') $workflow
    Put (Join-Path $gate 'code.cpp') 'baseline'
    Invoke-FixtureGit $gate @('add','-A');Invoke-FixtureGit $gate @('commit','-qm','baseline');Invoke-FixtureGit $gate @('update-ref','refs/remotes/origin/main','HEAD');Invoke-FixtureGit $gate @('switch','-qc','candidate')
    Put (Join-Path $gate 'code.cpp') 'candidate';Invoke-FixtureGit $gate @('add','code.cpp');Invoke-FixtureGit $gate @('commit','-qm','candidate')
    $coverage=[pscustomobject]@{repository='fixture/example';defaultBranch='main';toolingCommands=@();prWorkflowDigest=(Get-ScopedDigest $workflow);
        scopes=@([pscustomobject]@{name='Example';native=$true});prCoverage=@([pscustomobject]@{check='native';platform='x64';configuration='Release';scopes=@('Example')})}
    $previousGh=Get-Item Function:\global:gh -ErrorAction SilentlyContinue
    $global:ScopedGateCalls=0;$global:ScopedGateRoot=$gate;$global:ScopedGateMutation='';$global:ScopedGateState='active';$global:ScopedGateError=$false
    function global:gh {
        param([Parameter(ValueFromRemainingArguments=$true)][string[]]$FixtureArguments)
        $global:ScopedGateCalls++;$global:LASTEXITCODE=0
        if($global:ScopedGateError){$global:LASTEXITCODE=1;return}
        if($global:ScopedGateMutation -eq 'working'){[IO.File]::WriteAllText((Join-Path $global:ScopedGateRoot 'during-api.cpp'),'changed')}
        if($global:ScopedGateMutation -eq 'commit'){
            [IO.File]::WriteAllText((Join-Path $global:ScopedGateRoot 'code.cpp'),'concurrent commit')
            & git -C $global:ScopedGateRoot -c commit.gpgsign=false add code.cpp *> $null
            & git -C $global:ScopedGateRoot -c commit.gpgsign=false commit -qm 'concurrent commit' *> $null
        }
        '{"state":"'+$global:ScopedGateState+'"}'
    }
    try{
        Case 'only a clean committed candidate can delegate matching enabled CI, always as pending' {
            Assert ('Example' -in @(Get-ScopedPrCoverage $gate $coverage x64 Release)) 'Clean reviewed candidate not delegated'
            foreach($state in @('untracked','working','staged')){
                $path=Join-Path $gate $(if($state -eq 'untracked'){'pending.cpp'}else{'code.cpp'})
                Put $path 'pending'
                if($state -eq 'staged'){Invoke-FixtureGit $gate @('add','code.cpp')}
                $calls=$global:ScopedGateCalls
                Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) "Dirty candidate delegated: $state"
                Assert ($global:ScopedGateCalls -eq $calls) 'Dirty candidate queried GitHub before retaining local work'
                if($state -eq 'untracked'){Remove-Item -LiteralPath $path}else{Invoke-FixtureGit $gate @('restore','--staged','--worktree','--','code.cpp')}
            }
        }
        Case 'disabled/unavailable CI, stale workflow and mismatched profiles keep obligations local' {
            $global:ScopedGateState='disabled_manually';Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'Disabled workflow delegated'
            $global:ScopedGateState='active';$global:ScopedGateError=$true;Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'API failure delegated';$global:ScopedGateError=$false
            Assert (@(Get-ScopedPrCoverage $gate $coverage ARM64 Release).Count -eq 0) 'Unsupported profile delegated'
            $digest=$coverage.prWorkflowDigest;$coverage.prWorkflowDigest='stale'
            Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'Unreviewed workflow delegated';$coverage.prWorkflowDigest=$digest
        }
        Case 'working and clean-commit races during coverage lookup refuse delegation' {
            $global:ScopedGateMutation='working'
            Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'Concurrent working edit delegated'
            Remove-Item -LiteralPath (Join-Path $gate 'during-api.cpp')
            $global:ScopedGateMutation='commit'
            Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'Concurrent clean commit delegated'
            $global:ScopedGateMutation=''
        }
        Case 'direct default-branch pushes have no forthcoming PR coverage' {
            Invoke-FixtureGit $gate @('switch','-q','main')
            Assert (@(Get-ScopedPrCoverage $gate $coverage x64 Release).Count -eq 0) 'Default branch push incorrectly assumed a PR'
        }
    }finally{
        if($previousGh){Set-Item Function:\global:gh -Value $previousGh.ScriptBlock}else{Remove-Item Function:\global:gh}
        Remove-Variable -Name ScopedGateCalls,ScopedGateRoot,ScopedGateMutation,ScopedGateState,ScopedGateError -Scope Global
    }

    Case 'tooling identity includes the selected formatter bytes' {
        Import-Module (Join-Path $root 'Build/Toolchain.psm1') -Force
        $formatter=Get-RedTildeClangFormat
        $expected='formatter='+$formatter+':'+(Get-FileHash -LiteralPath $formatter).Hash
        Assert ($expected -cin @(Get-ScopedEnvironmentInputs -Tooling)) 'Selected formatter bytes were omitted'
    }
    Case 'package and performance entrypoints share current build artifact admission' {
        foreach($script in @('package.ps1','performance-paired.ps1')){
            $content=[IO.File]::ReadAllText((Join-Path $root $script))
            Assert ($content.Contains('Assert-RedTildeTestArtifact')) "$script bypasses the shared current build admission check"
            Assert ($content -notmatch '(?im)Get-Content[^\r\n]*build-receipt\.json') "$script reads a build receipt without the shared admission check"
        }
    }
    Case 'retained CI proof requires all six actual successful native executions' {
        $expected=@($manifest.prCoverage|Where-Object{$_.PSObject.Properties['platform']}|ForEach-Object check)
        $jobs=@($expected|ForEach-Object{[pscustomobject]@{name=$_;status='completed';conclusion='success';steps=@([pscustomobject]@{name='Full native qualification';conclusion='success'})}})
        Assert (Test-ScopedNativeCiProof $expected $jobs) 'Complete native matrix rejected'
        Assert (-not(Test-ScopedNativeCiProof @($expected[0],$expected[0],$expected[0],$expected[0],$expected[0],$expected[0]) @($jobs[0]))) 'One successful profile counted six times'
        foreach($outcome in @('failure','cancelled','skipped','timed_out')){
            $jobs[0].conclusion=$outcome;Assert (-not(Test-ScopedNativeCiProof $expected $jobs)) "Nonpassing native result inherited: $outcome"
        }
        $jobs[0].conclusion='success';$jobs[0].steps[0].conclusion='skipped'
        Assert (-not(Test-ScopedNativeCiProof $expected $jobs)) 'A skipped/listed/inherited row was treated as actual execution'
        Assert (-not(Test-ScopedNativeCiProof $expected @($jobs|Select-Object -Skip 1))) 'Incomplete matrix inherited'
    }
    $public=Join-Path $fixture 'public';New-Repository $public
    foreach($name in @('Example','Other')){Put (Join-Path $public "Tests/$name.Tests.Case.cpp") 'initial'}
    Put (Join-Path $public 'Tests/native-test-files.json') '["Tests/Example.Tests.Case.cpp","Tests/Other.Tests.Case.cpp"]'
    Copy-Item -LiteralPath (Join-Path $root 'Test-Changes.ps1') -Destination (Join-Path $public 'Test-Changes.ps1')
    Put (Join-Path $public 'Build/ScopedTesting.psm1') ([IO.File]::ReadAllText((Join-Path $root 'Build/ScopedTesting.psm1')))
    $mini.runtimeFiles=@();$mini.scopes=@(@{name='Example';native=$true;reuse=$true},@{name='Other';native=$true;reuse=$true},@{name='Tooling';native=$false;reuse=$true;commands=@(@{path='validate.ps1';arguments=@()},@{path='switch.ps1';arguments=@('-Expected')})})
    $mini.rules=@(@{pattern='Tests/Example.Tests.Case.cpp';scopes=@('Example');reason='example owner'},@{pattern='Tests/Other.Tests.Case.cpp';scopes=@('Other');reason='other owner'})
    Put (Join-Path $public 'Tests/test-scopes.json') ($mini|ConvertTo-Json -Depth 8)
    Put (Join-Path $public 'build.ps1') @'
param($Platform,$Configuration)
$output=Join-Path $PSScriptRoot ".build/$Platform/$Configuration"
[void](New-Item -ItemType Directory -Path $output -Force)
$bytes=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'Tests/Example.Tests.Case.cpp'))+[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'Tests/Other.Tests.Case.cpp'))
[IO.File]::WriteAllText((Join-Path $output 'example.exe'),$bytes)
exit 0
'@
    Put (Join-Path $public 'test.ps1') @'
param($Platform,$Configuration,[string[]]$Suites,[switch]$SkipBuild,[switch]$SkipTooling)
[IO.File]::AppendAllText((Join-Path $PSScriptRoot '.build/calls.txt'),($Suites -join ',')+"`n")
if(Test-Path (Join-Path $PSScriptRoot '.build/fail')){exit 1}
if(Test-Path (Join-Path $PSScriptRoot '.build/mutate')){[IO.File]::AppendAllText((Join-Path $PSScriptRoot 'Tests/Other.Tests.Case.cpp'),'mutated')}
exit 0
'@
    Put (Join-Path $public 'validate.ps1') @'
[CmdletBinding()]
param()
[void](New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot '.build') -Force)
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '.build/no-arguments.txt'),'ran')
'@
    Put (Join-Path $public 'switch.ps1') @'
[CmdletBinding()]
param([switch]$Expected)
if(-not $Expected){throw 'The manifest switch was not bound'}
[void](New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot '.build') -Force)
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '.build/explicit-switch.txt'),'ran')
'@
    Invoke-FixtureGit $public @('add','-A');Invoke-FixtureGit $public @('commit','-qm','baseline');Invoke-FixtureGit $public @('update-ref','refs/remotes/origin/main','HEAD');Invoke-FixtureGit $public @('switch','-qc','candidate')
    $platform=if([Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() -eq 'Arm64'){'ARM64'}else{'x64'}
    $state=Join-Path $public ".build/reports/scoped-tests/$platform-Debug/iteration.json"
    function Run([string[]]$Arguments=@(),[string]$Directory=$public){
        $binary=Join-Path $PSHOME $(if($IsWindows){'pwsh.exe'}else{'pwsh'})
        $start=[Diagnostics.ProcessStartInfo]::new($binary);$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
        foreach($arg in @('-NoProfile','-File',(Join-Path $Directory 'Test-Changes.ps1'),'-Platform',$platform)+$Arguments){$start.ArgumentList.Add($arg)}
        $process=[Diagnostics.Process]::Start($start)
        try{$out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync();if(-not $process.WaitForExit(120000)){$process.Kill($true);throw 'Owned protocol fixture timed out'};[pscustomobject]@{Exit=$process.ExitCode;Text=$out.GetAwaiter().GetResult()+$err.GetAwaiter().GetResult()}}finally{$process.Dispose()}
    }
    Case 'public runner expands empty and explicit tooling argument arrays exactly' {
        $r=Run @('-Scopes','Tooling')
        Assert ($r.Exit -eq 0) "Strict tooling commands failed through the public runner: $($r.Text)"
        Assert ((Get-Content (Join-Path $public '.build/no-arguments.txt') -Raw) -ceq 'ran') 'Empty manifest arguments reached the strict no-parameter script as an argument'
        Assert ((Get-Content (Join-Path $public '.build/explicit-switch.txt') -Raw) -ceq 'ran') 'Named manifest switch did not bind in the strict script'
    }
    Case 'public default records a successful affected baseline and then runs nothing unchanged' {
        Put (Join-Path $public 'Tests/Example.Tests.Case.cpp') 'example changed'
        $r=Run;Assert ($r.Exit -eq 0 -and (Test-Path $state)) "Initial public run failed: $($r.Text)"
        $r=Run;Assert ($r.Exit -eq 0 -and $r.Text.Contains('NOTHING_SELECTED')) "Identical default repeated: $($r.Text)"
        Assert (@(Get-Content (Join-Path $public '.build/calls.txt')).Count -eq 1) 'Identical code executed twice'
    }
    Case 'a later edit selects only the newly affected scope, not earlier branch changes' {
        Put (Join-Path $public 'Tests/Other.Tests.Case.cpp') 'other changed'
        $r=Run;Assert ($r.Exit -eq 0) "Later affected run failed: $($r.Text)"
        Assert (@(Get-Content (Join-Path $public '.build/calls.txt'))[-1] -ceq 'Other') 'Earlier Example scope reran after unrelated edit'
        Invoke-FixtureGit $public @('add','Tests');Invoke-FixtureGit $public @('commit','-qm','same tested bytes')
        $r=Run;Assert ($r.Exit -eq 0 -and $r.Text.Contains('NOTHING_SELECTED')) "Staging/committing the same bytes repeated tests: $($r.Text)"
    }
    Case 'explicit identical success is reused and partial selection never advances the default baseline' {
        $r=Run @('-Scopes','Example');Assert ($r.Exit -eq 0) "Explicit current-binary proof failed: $($r.Text)"
        $r=Run @('-Scopes','Example','-SkipBuild');Assert ($r.Exit -eq 0 -and $r.Text.Contains('REUSED Example')) "Exact explicit success repeated: $($r.Text)"
        $before=(Get-FileHash $state).Hash
        Put (Join-Path $public 'Tests/Example.Tests.Case.cpp') 'second example';Put (Join-Path $public 'Tests/Other.Tests.Case.cpp') 'second other'
        $r=Run @('-Scopes','Example');Assert ($r.Exit -eq 0) "Partial run failed: $($r.Text)"
        Assert ((Get-FileHash $state).Hash -ceq $before) 'Partial selection advanced the aggregate baseline'
        $r=Run;Assert ($r.Exit -eq 0 -and $r.Text.Contains('scopes: Example, Other')) "Partial selection concealed the other edit: $($r.Text)"
    }
    Case 'failures and post-run mutations never advance the affected baseline or retain reusable success' {
        $before=(Get-FileHash $state).Hash;Put (Join-Path $public 'Tests/Example.Tests.Case.cpp') 'failure candidate';Put (Join-Path $public '.build/fail') 'fail'
        $r=Run;Assert ($r.Exit -ne 0) 'Failure qualified';Assert ((Get-FileHash $state).Hash -ceq $before) 'Failure advanced baseline'
        $receipt=Join-Path $public ".build/reports/scoped-tests/$platform-Debug/Example.json"
        Assert (-not(Test-Path $receipt)) 'Failure retained earlier success'
        Remove-Item -LiteralPath (Join-Path $public '.build/fail');$r=Run;Assert ($r.Exit -eq 0) "Failed candidate was not rerun: $($r.Text)"
        $before=(Get-FileHash $state).Hash;Put (Join-Path $public 'Tests/Example.Tests.Case.cpp') 'mutation candidate';Put (Join-Path $public '.build/mutate') 'mutate'
        $r=Run;Assert ($r.Exit -ne 0) 'Mutation qualified';Assert ((Get-FileHash $state).Hash -ceq $before) 'Mutation advanced baseline'
        Remove-Item -LiteralPath (Join-Path $public '.build/mutate')
        $r=Run @('-SkipBuild');Assert ($r.Exit -ne 0 -and $r.Text.Contains('SkipBuild refused')) 'Stale compiled source accepted'
        $r=Run;Assert ($r.Exit -eq 0) "Mutated sources did not recover: $($r.Text)"
    }
    Case 'public build receipt invalidates content changes despite restored timestamps and preserves docs-only outputs' {
        $buildRoot=Join-Path $fixture 'build-input-guard';New-Repository $buildRoot
        Put (Join-Path $buildRoot '.gitignore') ".build/`n"
        Put (Join-Path $buildRoot 'RedTilde/RedTilde.vcxproj') '<Project />'
        Put (Join-Path $buildRoot 'Build/RedTilde.DxUi.props') '<Project />'
        Put (Join-Path $buildRoot 'Tests/test-scopes.json') '{"runtimeFiles":[]}'
        Put (Join-Path $buildRoot 'Tests/GuardFixture.cpp') 'first source bytes'
        foreach($name in @('Harness.psm1','ScopedTesting.psm1','BuildConsole.psm1','DxUiPaths.psm1')){Copy-Item -LiteralPath (Join-Path $root "Build/$name") -Destination (Join-Path $buildRoot "Build/$name")}
        Copy-Item -LiteralPath (Join-Path $root 'build.ps1') -Destination (Join-Path $buildRoot 'build.ps1')
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
$intermediate=Join-Path $root ".build/obj/$platform/$configuration/RedTilde"
$tools=Join-Path $root 'Tools'
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
            $arguments=@('-NoProfile','-File',(Join-Path $buildRoot 'build.ps1'),'-Platform','x64','-Configuration','Debug')
            if(-not $NoRestore){$arguments+= '-SkipRestore'}
            foreach($argument in $arguments+$Extra){$start.ArgumentList.Add($argument)}
            $process=[Diagnostics.Process]::Start($start)
            try{$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync();if(-not $process.WaitForExit(30000)){$process.Kill($true);throw 'Mocked public build fixture timed out'};[pscustomobject]@{exit=$process.ExitCode;text=$stdout.GetAwaiter().GetResult()+$stderr.GetAwaiter().GetResult()}}finally{$process.Dispose()}
        }
        function Assert-GuardRefusesArtifact([string]$Reason){
            $pwsh=Join-Path $PSHOME 'pwsh.exe'
            $probe=Join-Path $buildRoot '.build/assert-artifact.ps1'
            Put $probe "Import-Module '$($buildRoot.Replace("'","''"))/Build/Harness.psm1' -Force; Assert-RedTildeTestArtifact -Root '$($buildRoot.Replace("'","''"))' -Platform x64 -Configuration Debug"
            $start=[Diagnostics.ProcessStartInfo]::new($pwsh);$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
            foreach($argument in @('-NoProfile','-File',$probe)){$start.ArgumentList.Add($argument)}
            $process=[Diagnostics.Process]::Start($start)
            try{$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync();if(-not $process.WaitForExit(30000)){$process.Kill($true);throw 'Mocked artifact admission timed out'};Assert ($process.ExitCode -ne 0) "Stale build receipt admitted after $Reason"}finally{$process.Dispose()}
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
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 1) 'Unattested first build did not clean owned profile'
        }
        Case 'docs-only changes reuse byte-identical output without clean' {
            $exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put (Join-Path $buildRoot 'README.md') 'documentation only';Invoke-FixtureGit $buildRoot @('add','README.md')
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Docs-only build failed: $($run.text)"
            Assert ((Get-FileHash $exe).Hash -ceq $before) 'Docs-only build changed executable bytes'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 1) 'Docs-only edit unnecessarily cleaned profile'
            Import-Module (Join-Path $buildRoot 'Build/Harness.psm1') -Force
            Assert-RedTildeTestArtifact -Root $buildRoot -Platform x64 -Configuration Debug|Out-Null
        }
        Case 'source bytes with a restored older mtime clean before MSBuild reuse' {
            $source=Join-Path $buildRoot 'Tests/GuardFixture.cpp';$oldTime=[IO.File]::GetLastWriteTimeUtc($source);$exe=Join-Path $buildRoot '.build/x64/Debug/RedTilde.exe';$before=(Get-FileHash $exe).Hash
            Put $source 'changed compiled source bytes';[IO.File]::SetLastWriteTimeUtc($source,$oldTime)
            Assert-GuardRefusesArtifact 'source bytes changed with an old timestamp'
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
            Assert-GuardRefusesArtifact 'dirty pinned SDK implementation source'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Dirty tracked SDK source was accepted by the public build entrypoint'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Dirty SDK source reached profile clean'
            Assert (-not (Get-Content (Join-Path $buildRoot '.build/x64/Debug/build-receipt.json') -Raw|ConvertFrom-Json).passed) 'Dirty SDK source retained a passed receipt'
            Invoke-FixtureGit $sdkSourceCommitted @('checkout','--','src/GuardFixture.cpp')
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Clean pinned SDK source did not recover: $($run.text)"

            $untrackedSource=Join-Path $sdkSourceCommitted 'src/Unexpected.cpp';Put $untrackedSource 'untracked SDK implementation'
            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            Assert-GuardRefusesArtifact 'untracked pinned SDK implementation source'
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
            Assert-GuardRefusesArtifact 'redirected DxUiRoot in resolved properties'
            $run=Invoke-BuildFixture;Assert ($run.exit -ne 0) 'Redirected SDK source root was accepted'
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq $cleanCount) 'Redirected resolved properties reached profile clean'
            Put $propsPath $original
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Restored resolved properties did not recover: $($run.text)"

            $cleanCount=@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count
            $redirectedOutput=(Join-Path $buildRoot '.build/foreign-package')+'\'
            $redirected=$original.Replace([Security.SecurityElement]::Escape($resolvedOutput),[Security.SecurityElement]::Escape($redirectedOutput))
            Put $propsPath $redirected
            Assert-GuardRefusesArtifact 'redirected DxUiConsumerOutputRoot in resolved properties'
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
            Assert-GuardRefusesArtifact 'legacy receipt'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Legacy receipt recovery failed: $($run.text)"
            $receipt=Get-Content $receiptPath -Raw|ConvertFrom-Json;$receipt.buildInputs=$null
            $receipt|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $receiptPath -Encoding utf8
            Assert-GuardRefusesArtifact 'incomplete receipt'
            $run=Invoke-BuildFixture;Assert ($run.exit -eq 0) "Incomplete receipt recovery failed: $($run.text)"
            Assert (@(Get-Content (Join-Path $buildRoot '.build/mock-calls.txt')|Where-Object{$_ -match '/t:Clean'}).Count -eq 11) 'Invalid receipt did not force clean'
        }
        Case 'failed build leaves a revoked receipt that SkipBuild admission rejects' {
            Put (Join-Path $buildRoot '.build/mock-fail') 'force failure';$run=Invoke-BuildFixture
            Assert ($run.exit -ne 0) 'Mock build failure reported success'
            $receipt=Get-Content (Join-Path $buildRoot '.build/x64/Debug/build-receipt.json') -Raw|ConvertFrom-Json
            Assert (-not $receipt.passed) 'Failed attempt retained a passed build receipt'
            Assert-GuardRefusesArtifact 'failed build attempt'
        }
    }
    Write-Host "Scoped testing: $passed cases passed."
}finally{
    $absolute=[IO.Path]::GetFullPath($fixture);$allowed=[IO.Path]::GetFullPath((Join-Path $root '.build/ToolTests'))+[IO.Path]::DirectorySeparatorChar
    if(-not $absolute.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture cleanup escaped its owned root'}
    if(Test-Path -LiteralPath $absolute){Remove-Item -LiteralPath $absolute -Recurse -Force}
}
exit 0
