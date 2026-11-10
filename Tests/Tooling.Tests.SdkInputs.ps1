<# .SYNOPSIS Exercises actual SDK snapshot/argument boundaries using inert metadata; no native SDK or compiler verdict. #>
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'Build/Harness.psm1') -Force
Import-Module (Join-Path $root 'Build/Toolchain.psm1') -Force
$run=Join-Path $root ('.build/sdk-snapshot-controls/'+[guid]::NewGuid().ToString('N'))
$sdkRoot=Join-Path $run 'sdk';[void][IO.Directory]::CreateDirectory($sdkRoot)
[IO.File]::WriteAllText((Join-Path $sdkRoot 'DxUi.Sdk.props'),'<Project />')
$msbuild=Join-Path $run 'metadata.ps1'
[IO.File]::WriteAllText($msbuild,@'
param([Parameter(ValueFromRemainingArguments)]$Arguments)
Get-Content -LiteralPath (Join-Path $PSScriptRoot 'properties.json') -Raw
$global:LASTEXITCODE=0
'@)
$properties=[ordered]@{OutDir=(Join-Path $root '.build/x64/Release');IntDir=(Join-Path $root '.build/obj/x64/Release/RedTilde');
    PlatformToolset='v145';VCToolsInstallDir=(Join-Path $run 'msvc');VCToolsVersion='14.52.37025';WindowsSdkDir=(Join-Path $run 'windows');
    WindowsTargetPlatformVersion='10.0.26100.0';PreferredToolArchitecture='x64';VCToolArchitecture='Native64Bit';WindowsSDKToolArchitecture='Native64Bit';
    CLToolPath='';CLToolExe='';LinkToolPath='';LinkToolExe='';ResourceCompileToolPath='';ResourceCompileToolExe='';
    DxUiRoot='';DxUiSdkRoot=$sdkRoot;CiSdkContract='QualifiedAbiV2'}
$reader=@{Closed=$false;SdkRoot=$sdkRoot;LeasePath=(Join-Path $run 'lease.json');Descriptor=@{Hash=('a'*64)};
    Evidence=[ordered]@{recipe='PinnedQualifiedReleaseSdkV2';inputKey=('b'*64);manifestSha256=('c'*64);toolchainReference=('d'*64)}}
function Put([Collections.IDictionary]$Value){[IO.File]::WriteAllText((Join-Path $run 'properties.json'),(@{Properties=$Value}|ConvertTo-Json))}
function Snapshot {Get-RedTildeBuildInputSnapshot -Root $root -Platform x64 -Configuration Release -MSBuildPath $msbuild -SdkInput $reader}
function Refuses([scriptblock]$Body,[string]$Message){$failed=$false;try{& $Body|Out-Null}catch{$failed=$true};if(-not $failed){throw $Message}}
Put $properties
$first=Snapshot;$same=Snapshot
if($first.identity -cne $same.identity -or $first.dependencyHeadersSha256 -cne $reader.Evidence.manifestSha256){throw 'Identical authenticated-input references did not produce equal conservative snapshots.'}
$changed=$properties|ConvertTo-Json|ConvertFrom-Json -AsHashtable;$changed.VCToolsVersion='14.52.99999';Put $changed
if((Snapshot).identity -ceq $first.identity){throw 'Changed actual compiler metadata reused the prior snapshot.'}
foreach($field in @('OutDir','IntDir','DxUiRoot','DxUiSdkRoot','CiSdkContract')){
    $changed=$properties|ConvertTo-Json|ConvertFrom-Json -AsHashtable
    $changed[$field]=if($field -ceq 'CiSdkContract'){'LegacyV1'}else{Join-Path $run 'redirected'}
    Put $changed;Refuses {Snapshot} "Snapshot accepted redirected or downgraded $field."
}
$changed=$properties|ConvertTo-Json|ConvertFrom-Json -AsHashtable;$changed.Remove('VCToolsVersion');Put $changed
Refuses {Snapshot} 'Snapshot accepted missing actual compiler metadata.'
Put $properties
$reader.Closed=$true;Refuses {Snapshot} 'Closed reader accepted.';$reader.Closed=$false
$reader.Evidence.recipe='PinnedReleaseSdkV1';Refuses {Snapshot} 'Unsupported Tilde v1 snapshot accepted.';$reader.Evidence.recipe='PinnedQualifiedReleaseSdkV2'
$reader.SdkRoot+=';foreign';Refuses {Get-RedTildeSdkBuildArguments -Root $root -SdkInput $reader} 'MSBuild delimiter accepted.';$reader.SdkRoot=$sdkRoot
Refuses {Get-RedTildeBuildInputSnapshot -Root $root -Platform x64 -Configuration Debug -MSBuildPath $msbuild -SdkInput $reader} 'Debug SDK snapshot accepted.'
Write-Host "PASS qualified SDK snapshot determinism, mutation, output ownership, source isolation, downgrade, missing metadata and closed-owner controls: $run"

$global:LASTEXITCODE=0
