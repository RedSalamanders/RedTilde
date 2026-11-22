<#
.SYNOPSIS
Exercises Build/VcpkgToolCheckout.psm1 offline against a file:// fixture repository.
.DESCRIPTION
The pinned DxUi installer reuses <OutputRoot>\vcpkg-tool when it is a clean checkout at its vcpkg-tool.json commit.
vcpkg reads versioned ports with git read-tree and checkout-index in an environment that holds only
GIT_INDEX_FILE, so that checkout must hold every object. This covers a fresh clone, a rerun, completing the
partial shallow clone the pinned installer leaves, refusing another repository, and an offline extraction of an
older tree in that stripped environment. restore-dxui.ps1 must prepare the checkout before the pinned installer.
The fixture is retained under .build for diagnosis.
#>
[CmdletBinding()]param([string]$Root='', [string]$ScratchRoot='')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if(-not $Root){$Root=(& git -C $PSScriptRoot rev-parse --show-toplevel).Trim()}
if(-not $ScratchRoot){$ScratchRoot=Join-Path $Root ('.build/VcpkgToolCheckoutTests/'+[guid]::NewGuid().ToString('N'))}
[void][IO.Directory]::CreateDirectory($ScratchRoot)
Import-Module (Join-Path $Root 'Build/VcpkgToolCheckout.psm1') -Scope Local -Force -ErrorAction Stop
function Require([bool]$Condition,[string]$Message){if(-not $Condition){throw $Message}}
function Invoke-FixtureGit([string[]]$Arguments){
    $output=& git.exe @Arguments
    if($LASTEXITCODE -ne 0){throw "Fixture git $($Arguments -join ' ') failed with exit code $LASTEXITCODE."}
    return $output
}

$source=Join-Path $ScratchRoot 'source'
Invoke-FixtureGit @('init','-q','-b','master',$source) | Out-Null
Invoke-FixtureGit @('-C',$source,'config','uploadpack.allowFilter','true') | Out-Null
Invoke-FixtureGit @('-C',$source,'config','uploadpack.allowAnySHA1InWant','true') | Out-Null
foreach($revision in 1..5){
    [IO.File]::WriteAllText((Join-Path $source "port$revision.txt"),"port $revision")
    [IO.File]::WriteAllText((Join-Path $source 'shared.txt'),"shared $revision")
    Invoke-FixtureGit @('-C',$source,'add','-A') | Out-Null
    Invoke-FixtureGit @('-C',$source,'-c','user.name=fixture','-c','user.email=fixture@example.invalid','commit','-q','-m',"revision $revision") | Out-Null
}
$pinned=([string](Invoke-FixtureGit @('-C',$source,'rev-parse','HEAD~2'))).Trim()
$olderTree=([string](Invoke-FixtureGit @('-C',$source,'rev-parse','HEAD~3^{tree}'))).Trim()
$repository='file:///'+$source.Replace('\','/')
$dxui=Join-Path $ScratchRoot 'dxui'
[void][IO.Directory]::CreateDirectory($dxui)
$identityPath=Join-Path $dxui 'vcpkg-tool.json'
[IO.File]::WriteAllText($identityPath,(@{repository=$repository;commit=$pinned}|ConvertTo-Json))

function Assert-CompleteCheckout([string]$ToolRoot,[string]$Label){
    $origin=([string](Invoke-FixtureGit @('-C',$ToolRoot,'remote','get-url','origin'))).Trim()
    Require ($origin -ceq $repository) "$Label uses another repository: $origin"
    Require (@(Invoke-FixtureGit @('-C',$ToolRoot,'status','--porcelain')).Count -eq 0) "$Label is not clean."
    Require (([string](Invoke-FixtureGit @('-C',$ToolRoot,'rev-parse','HEAD'))).Trim() -ceq $pinned) "$Label is not at the pinned commit."
    Require (([string](Invoke-FixtureGit @('-C',$ToolRoot,'rev-parse','--is-shallow-repository'))).Trim() -ceq 'false') "$Label is shallow."
    $filter=& git.exe -C $ToolRoot config --get remote.origin.partialclonefilter
    Require (-not $filter) "$Label keeps the partial clone filter '$filter'."
    $missing=@(Invoke-FixtureGit @('-C',$ToolRoot,'rev-list','--objects','--missing=print','--all') | Where-Object { ([string]$_).StartsWith('?') })
    Require ($missing.Count -eq 0) "$Label misses $($missing.Count) object(s)."
}
# vcpkg's git: an environment that holds only GIT_INDEX_FILE. Lazy fetch is disabled because a file:// promisor
# would otherwise be reachable without the network that vcpkg's git lacks.
function Invoke-StrippedExtraction([string]$ToolRoot,[string]$Name){
    $target=Join-Path $ScratchRoot "extract-$Name"
    [void][IO.Directory]::CreateDirectory($target)
    $gitDirectory=Join-Path $ToolRoot '.git'
    foreach($arguments in @(@('--git-dir',$gitDirectory,'read-tree',$olderTree),
                            @('--git-dir',$gitDirectory,'--work-tree',$target,'checkout-index','-af',"--prefix=$target/"))){
        $start=[Diagnostics.ProcessStartInfo]::new((Get-Command git.exe).Source)
        foreach($argument in $arguments){$start.ArgumentList.Add($argument)}
        $start.UseShellExecute=$false
        $start.RedirectStandardOutput=$true
        $start.RedirectStandardError=$true
        $start.Environment.Clear()
        $start.Environment['GIT_INDEX_FILE']=Join-Path $ScratchRoot "extract-$Name.index"
        $start.Environment['GIT_NO_LAZY_FETCH']='1'
        $process=[Diagnostics.Process]::Start($start)
        try{
            $null=$process.StandardOutput.ReadToEndAsync()
            $null=$process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if($process.ExitCode -ne 0){return $false}
        }finally{$process.Dispose()}
    }
    return (@(Get-ChildItem -LiteralPath $target -File).Count -eq 3)
}

$fresh=Join-Path $ScratchRoot 'fresh/vcpkg-tool'
Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $fresh
Assert-CompleteCheckout $fresh 'A fresh checkout'
Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $fresh
Assert-CompleteCheckout $fresh 'A repeated checkout'
Require (Invoke-StrippedExtraction $fresh 'fresh') 'A fresh checkout cannot extract an older tree offline.'

# What the pinned installer leaves: a blob-filtered, shallow clone detached at the pin.
$partial=Join-Path $ScratchRoot 'partial/vcpkg-tool'
Invoke-FixtureGit @('clone','-q','-c','core.longpaths=true','--filter=blob:none',$repository,$partial) | Out-Null
Invoke-FixtureGit @('-C',$partial,'fetch','-q','--depth','1','origin',$pinned) | Out-Null
Invoke-FixtureGit @('-C',$partial,'checkout','-q','--detach',$pinned) | Out-Null
Require (-not (Invoke-StrippedExtraction $partial 'partial-before')) 'The control failed: a partial clone extracted an older tree offline.'
Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $partial
Assert-CompleteCheckout $partial 'A completed partial checkout'
Require (Invoke-StrippedExtraction $partial 'partial-after') 'A completed partial checkout cannot extract an older tree offline.'

$dirty=$partial
$dirtyMarker=Join-Path $dirty 'dirty-untracked.txt'
[IO.File]::WriteAllText($dirtyMarker,'dirty')
$refused=$false
try{Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $dirty}catch{$refused=$_.Exception.Message.Contains('contains local changes')}
Require $refused 'A dirty managed vcpkg checkout was not refused before checkout.'
[IO.File]::Delete($dirtyMarker)

$shallowOnly=Join-Path $ScratchRoot 'shallow-only/vcpkg-tool'
Invoke-FixtureGit @('clone','-q','-c','core.longpaths=true','--depth','1',$repository,$shallowOnly) | Out-Null
Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $shallowOnly
Assert-CompleteCheckout $shallowOnly 'A non-partial shallow checkout'
Require (Invoke-StrippedExtraction $shallowOnly 'shallow-only') 'A completed non-partial shallow checkout cannot extract an older tree offline.'

$foreign=Join-Path $ScratchRoot 'foreign/vcpkg-tool'
Invoke-FixtureGit @('clone','-q',$repository,$foreign) | Out-Null
Invoke-FixtureGit @('-C',$foreign,'remote','set-url','origin','https://example.invalid/other') | Out-Null
$refused=$false
try{Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $foreign}catch{$refused=$_.Exception.Message.Contains('does not use the pinned repository')}
Require $refused 'A checkout of another repository was accepted.'

$notGit=Join-Path $ScratchRoot 'not-git/vcpkg-tool'
[void][IO.Directory]::CreateDirectory($notGit)
[IO.File]::WriteAllText((Join-Path $notGit 'keep.txt'),'Retain this data.')
$refused=$false
try{Initialize-VcpkgToolCheckout -ToolIdentityPath $identityPath -ToolRoot $notGit}catch{$refused=$_.Exception.Message.Contains('not a Git checkout')}
Require ($refused -and (Test-Path -LiteralPath (Join-Path $notGit 'keep.txt'))) 'A non-Git directory was not refused and retained.'

$restore=[IO.File]::ReadAllText((Join-Path $Root 'restore-dxui.ps1'))
$prepare=$restore.IndexOf("Initialize-VcpkgToolCheckout -ToolIdentityPath (Join-Path `$source 'vcpkg-tool.json') -ToolRoot (Join-Path `$output 'vcpkg-tool')",[StringComparison]::Ordinal)
$install=$restore.IndexOf("& (Join-Path `$source 'vcpkg-install.ps1') -Platform `$Platform -OutputRoot `$output",[StringComparison]::Ordinal)
Require ($prepare -ge 0 -and $install -gt $prepare) 'restore-dxui.ps1 must prepare the complete vcpkg checkout before the pinned DxUi installer runs.'
Write-Host "PASS complete vcpkg checkout: fresh, repeated, completed partial, foreign and non-Git refusals, offline extraction; retained $ScratchRoot"
