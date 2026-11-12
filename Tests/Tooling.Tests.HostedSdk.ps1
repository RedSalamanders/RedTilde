<# .SYNOPSIS Executes actual hosted SDK role/argument routing with inert build/package commands. #>
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $repo 'Tools/Invoke-ProjectCi.ps1'),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Actual adapter syntax failed.'}
$build=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.IfStatementAst] -and $n.Extent.Text.StartsWith("if (`$Phase -in @('ReleaseTestBuild','Qualification','Production'))")},$true))
$package=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.IfStatementAst] -and $n.Extent.Text.StartsWith("if (`$Phase -eq 'Production')") -and $n.Extent.Text.Contains("Run 'package.ps1'")},$true))
if($build.Count -ne 1 -or $package.Count -ne 1){throw 'Expected one actual build and production branch.'}
$fixture=Join-Path $repo ('.build/hosted-sdk-controls/'+[guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory((Join-Path $fixture 'Dependencies'))
[void][IO.Directory]::CreateDirectory((Join-Path $fixture '.build/package'))
Copy-Item (Join-Path $repo 'Dependencies/DxUi.lock.json') (Join-Path $fixture 'Dependencies/DxUi.lock.json')
$stage=[scriptblock]::Create(@'
param($Phase,$Configuration,$Platform,$Available,$root,$Probe)
$profile=@('-Platform',$Platform,'-Configuration',$Configuration);$number='123';$EngineRoot='controlled-engine';$context=@{root=$root}
function Import-Module {param($Name) if(-not $Name.EndsWith('Ci.SdkPreparation.psm1')){throw 'Unexpected SDK module.'};$Probe.imports++}
function Initialize-CiPinnedSdkInput {
    param($Context,$SourceCommit,$ApiRevision)
    $Probe.prepares++;$Probe.source=$SourceCommit;$Probe.api=$ApiRevision
    if($Available){return 'C:/synthetic workspace/.build/descriptor.json'}
}
function Run {
    param($Script,$CommandArguments)
    $Probe.commands.Add(@{script=$Script;arguments=@($CommandArguments)})
    if($Script -cne 'package.ps1'){return}
    $archive=Join-Path $root ".build/package/synthetic-$Platform-0.zip"
    [IO.File]::WriteAllText($archive,'inert package fixture')
    $names=@('self-test','test-scope','list-tests','interactive','crash','recovery-crash','recovery-restart',
        'preferences','asan','linked-save-crash','linked-save-restart','failure-host','failure-host-missing-value',
        'test-scope-missing-value','interactive-missing-value')
    $proof=@{platform=$Platform;testsEnabled=$false;smokePassed=$true;archiveSha256=(Get-FileHash $archive).Hash;
        testOnlyEntryPointRefusals=@($names|ForEach-Object{@{name=$_;exitCode=2;dataDirectoryAbsent=$true}})}
    [IO.File]::WriteAllText(($archive+'.receipt.json'),($proof|ConvertTo-Json -Depth 5))
}
function Write-CiProductionReceipt {
    param($Context,$Archives,$TestModeDisabled,$SmokePassed)
    $Probe.receipts++
    [void][IO.Directory]::CreateDirectory((Join-Path $root ".build/ci/production/$Platform"))
}
'@+$build[0].Extent.Text+"`n"+$package[0].Extent.Text)
$count=0
foreach($phase in @('Static','ReleaseTestBuild','ReleaseTest','Production','Qualification')){
    foreach($configuration in @('Debug','Release','ASan Debug')){foreach($platform in @('x64','ARM64')){foreach($available in @($true,$false)){
        $probe=@{imports=0;prepares=0;source='';api=0;commands=[Collections.Generic.List[object]]::new();receipts=0}
        & $stage $phase $configuration $platform $available $fixture $probe
        $eligible=$configuration -ceq 'Release' -and $phase -cin @('ReleaseTestBuild','Production')
        if($probe.prepares -ne [int]$eligible -or $probe.imports -ne [int]$eligible){throw "Wrong SDK preparation: $phase/$configuration/$platform"}
        if($eligible -and ($probe.source -cne '21fae0dc66000b04504ba7862169533ac2943be8' -or $probe.api -ne 5)){throw 'Adapter changed the actual API-5 pin.'}
        $expected=if($phase -ceq 'Production'){2}elseif($phase -cin @('ReleaseTestBuild','Qualification')){1}else{0}
        if($probe.commands.Count -ne $expected){throw 'Build/package command count changed.'}
        foreach($command in $probe.commands){
            $hasDescriptor='-DxUiSdkInputDescriptor' -cin $command.arguments
            if($hasDescriptor -ne ($eligible -and $available)){throw 'Descriptor eligibility or fallback changed.'}
            if($hasDescriptor -and 'C:/synthetic workspace/.build/descriptor.json' -cnotin $command.arguments){throw 'Descriptor split or substituted.'}
        }
        $count++
    }}}
}
Write-Host "PASS hosted SDK adapter: $count actual role/profile/architecture/availability combinations; no native qualification."
$global:LASTEXITCODE=0
