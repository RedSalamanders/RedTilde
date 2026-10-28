$ErrorActionPreference='Stop'
# Synthetic validator cases. Only isolated copies receive temporary test seals.
$draftRoot=Split-Path $PSScriptRoot -Parent
$moduleSource=Join-Path $draftRoot 'Build/Performance.psm1'
$testOutputRoot=Join-Path $draftRoot '.build/v4-validator-tests'
$testRoot=Join-Path $testOutputRoot ([Guid]::NewGuid().ToString('N'))
$resolvedOutputRoot=[IO.Path]::GetFullPath($testOutputRoot).TrimEnd('\')+'\'
$resolvedTestRoot=[IO.Path]::GetFullPath($testRoot)
if(-not $resolvedTestRoot.StartsWith($resolvedOutputRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Test output escaped its private .build root.'}
$privateBuild=Get-Item -LiteralPath (Join-Path $draftRoot '.build') -Force
if($privateBuild.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Private .build root is a reparse point.'}
[void][IO.Directory]::CreateDirectory((Join-Path $testRoot 'Build'))
foreach($checkPath in @($testOutputRoot,$testRoot,(Join-Path $testRoot 'Build'))){if((Get-Item -LiteralPath $checkPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Generated test output contains a reparse point.'}}
$profilePath=Join-Path $testRoot 'Build/ReviewedHeaderDeltaProfile.json'
$templatePath=Join-Path $draftRoot 'Build/ReviewedHeaderDeltaProfile.json'
Copy-Item -LiteralPath $templatePath -Destination $profilePath
$testModule=Join-Path $testRoot 'Performance.psm1'
$failures=[Collections.Generic.List[string]]::new()
function HashText([string]$text) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($text))) }
function New-Inventory([bool]$candidate) {
    $paths=[Collections.Generic.List[string]]::new()
    foreach($name in @('AccessibilityTextUnits.h','DxUi.h','RichTextLayout.h','RichTextPresentation.h')){$paths.Add("include/DxUi/$name")}
    for($i=0;$paths.Count -lt 25;++$i){$paths.Add("include/DxUi/TestHeader$i.h")}
    for($i=0;$i -lt 33;++$i){$paths.Add("package-include/test/package$i.h")}
    $files=foreach($path in $paths){$changed=$path -in @('include/DxUi/DxUi.h','include/DxUi/RichTextLayout.h','include/DxUi/RichTextPresentation.h');[ordered]@{path=$path;sha256=if($candidate -and $changed){'B'*64}else{'A'*64}}}
    $source=@($files|Where-Object path -clike 'include/*'|ForEach-Object {$_.path+':'+$_.sha256});[Array]::Sort($source,[StringComparer]::Ordinal)
    $package=@($files|Where-Object path -clike 'package-include/*'|ForEach-Object {$_.path+':'+$_.sha256});[Array]::Sort($package,[StringComparer]::Ordinal)
    $all=@($source+$package);[Array]::Sort($all,[StringComparer]::Ordinal)
    return [ordered]@{schemaVersion=2;sourceCount=25;packageCount=33;sourceSha256=HashText ($source -join "`n");packageSha256=HashText ($package -join "`n");combinedSha256=HashText ($all -join "`n");files=@($files)}
}
function New-Profile {
    $retainedProfile=Get-Content -LiteralPath $templatePath -Raw|ConvertFrom-Json -AsHashtable
    $sourceB64=[string]$retainedProfile.candidate.probe.sourceBase64
    $sourceBytes=[Convert]::FromBase64String($sourceB64)
    $baseId=[ordered]@{commit='98c795b765a10d1805ae0a7c35a8ade535fb7fd6';apiRevision=4;platform='x64';toolset='v145';vcToolsVersion='14.52.37025';windowsSdkVersion='10.0.26100.0';runtimeFamily='MD';preferredToolArchitecture='x86';disableStlAnnotations=$false;releaseOptimizationPolicy='Gy-REF-GL-LTCG-full-v1';msbuildSha256='1'*64;compilerSha256='2'*64;linkerSha256='3'*64;windowsHeaderSha256='4'*64;kernel32Sha256='5'*64;ucrtSha256='6'*64}
    $candId=[ordered]@{commit='086ad82a5872768db23ea83862ab4af13dff0c5b';apiRevision=4;platform='x64';toolset='v145';vcToolsVersion='14.52.37025';windowsSdkVersion='10.0.26100.0';runtimeFamily='MD';preferredToolArchitecture='x86';disableStlAnnotations=$false;msbuildSha256='1'*64;compilerSha256='2'*64;linkerSha256='3'*64;windowsHeaderSha256='4'*64;kernel32Sha256='5'*64;ucrtSha256='6'*64;releaseOptimizationPolicy='Gy-REF-GL-LTCG-full-v1'}
    $sides=[ordered]@{}
    foreach($side in @('baseline','candidate')){
        $isCandidate=$side -ceq 'candidate';$id=if($isCandidate){$candId}else{$baseId};$commit=$id.commit;$fingerprint=if($isCandidate){'C'*64}else{'D'*64};$lib=if($isCandidate){'F'*64}else{'E'*64};$headers=New-Inventory $isCandidate
        $result=[ordered]@{schema='dxui-header-compat-probe-v4';profile=$side;apiRevision=4;diagnosticsEnabled=$true;markdownDefaultsAreDefault=if($isCandidate){$true}else{$null};legacyButtonAppearanceValues=@(0,1,2,3);newButtonAppearanceValues=if($isCandidate){@(4,5)}else{$null};layout=[ordered]@{ThemePalette=@{size=256;align=4};ProgressBar=@{size=512;align=8};RichTextEditorAppearance=@{size=192;align=8};LayoutOptions=@{size=160;align=8}};legacyCaptureLinked=$true;newCandidateSymbolsLinked=$isCandidate}
        $resultBytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json $result -Depth 20 -Compress)+"`n")
        $probe=[ordered]@{protocol='dxui-header-compat-probe-v4';hostArchitecture='X86';sourceBase64=$sourceB64;sourceSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($sourceBytes));exeSha256='E'*64;resultBase64=[Convert]::ToBase64String($resultBytes);resultSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($resultBytes));librarySha256=$lib;result=$result}
        $sides[$side]=[ordered]@{commit=$commit;tree=if($isCandidate){'b'*40}else{'a'*40};dependencyFingerprint=$fingerprint;librarySha256=$lib;buildReceiptSha256='9'*64;identity=$id;publicHeaders=$headers;buildEvidence=@{configuration='Release';platform='x64';hostArchitecture='X86';passed=$true;receiptSha256='9'*64};probe=$probe}
    }
    $pairs=@(foreach($path in @('include/DxUi/DxUi.h','include/DxUi/RichTextLayout.h','include/DxUi/RichTextPresentation.h')){@{sourcePath=$path;baselineSha256='A'*64;candidateSha256='B'*64}})
    $profile=[ordered]@{schema='redtilde-reviewed-dxui-header-delta-profile-v4';profileId='redtilde-dxui-markdown-caption-api4-v4';status='ready-reviewed-exact-pair';expectedApiRevision=4;baseline=$sides.baseline;candidate=$sides.candidate;allOtherInventoryRowsMustBeIdentical=$true;packageHeadersUnchanged=$true;dependencyFieldChanges=@('Fingerprint','Identity.commit');compilerPathOperandsOnly=@('one exact DxUi source /I','one exact fingerprinted package /I');allowedHeaderHashPairs=$pairs;appCompilerContext=@{}}
    foreach($side in @('baseline','candidate')){$dep=$sides[$side];$src='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+$dep.commit+'\INCLUDE';$pkg='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+$dep.dependencyFingerprint+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE';$cmd='cl.exe /nologo /c /MD /I"'+$src+'" /I"'+$pkg+'" probe.cpp';$cl=[byte[]](1,2,3,4);$link=[byte[]](5,6,7,8);$linkCmd='link.exe /OUT:app.exe DxUi.lib';$profile.appCompilerContext[$side]=@{target='x64';configuration='Release';hostArchitecture='X86';normalizedProfileSha256=HashText ("redtilde-release-compiler-profile-v2`n"+$cmd);tlogSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($cl));tlogBase64=[Convert]::ToBase64String($cl);linkTlogSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($link));linkTlogBase64=[Convert]::ToBase64String($link);linkCommand=$linkCmd;normalizedOptionsSha256='3'*64;applicationLinkCommandSha256=HashText $linkCmd;translationUnitCount=10;commands=@($cmd)}}
    return $profile
}
function Save-SealedTestProfile($profile) {
    $json=ConvertTo-Json -InputObject $profile -Depth 100
    [IO.File]::WriteAllText($profilePath,$json,[Text.UTF8Encoding]::new($false))
    $bytes=[IO.File]::ReadAllBytes($profilePath);$seal=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))
    $moduleText=[IO.File]::ReadAllText($moduleSource)
    $sealPattern='(?m)^\$script:ReviewedHeaderDeltaV4ProfileSha256 = ''(?:[0-9A-Fa-f]{64}|ROOT_REVIEW_BIND_REQUIRED)''\r?$'
    $sealMatches=[regex]::Matches($moduleText,$sealPattern)
    if($sealMatches.Count -ne 1){throw 'The copied validator must contain exactly one anchored V4 test seal assignment.'}
    $moduleText=[regex]::Replace($moduleText,$sealPattern,"`$script:ReviewedHeaderDeltaV4ProfileSha256 = '$seal'")
    [IO.File]::WriteAllText($testModule,$moduleText,[Text.UTF8Encoding]::new($false))
    Remove-Module Performance -ErrorAction SilentlyContinue;Import-Module $testModule -Force
}
function Refresh-ProbeResult($profile,[string]$side) {
    $probe=$profile[$side].probe;$bytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json $probe.result -Depth 20 -Compress)+"`n")
    $probe.resultBase64=[Convert]::ToBase64String($bytes);$probe.resultSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))
}
function Expect-Reject($profile,[string]$label) {
    Save-SealedTestProfile $profile;$rejected=$false
    try{Get-RedTildeReviewedHeaderDeltaProfile -Root $testRoot|Out-Null}catch{$rejected=$true}
    if(-not $rejected){$failures.Add($label)}
}
try {
    $unboundText=[IO.File]::ReadAllText($moduleSource)
    $sealPattern='(?m)^\$script:ReviewedHeaderDeltaV4ProfileSha256 = ''(?:[0-9A-Fa-f]{64}|ROOT_REVIEW_BIND_REQUIRED)''\r?$'
    if([regex]::Matches($unboundText,$sealPattern).Count -ne 1){throw 'Expected exactly one canonical seal assignment.'}
    $unboundText=[regex]::Replace($unboundText,$sealPattern,'$script:ReviewedHeaderDeltaV4ProfileSha256 = ''ROOT_REVIEW_BIND_REQUIRED''')
    [IO.File]::WriteAllText($testModule,$unboundText,[Text.UTF8Encoding]::new($false))
    Remove-Module Performance -ErrorAction SilentlyContinue;Import-Module $testModule -Force
    $unboundRejected=$false;try{Get-RedTildeReviewedHeaderDeltaProfile -Root $testRoot|Out-Null}catch{$unboundRejected=$true}
    if(-not $unboundRejected){throw 'An isolated unbound validator unexpectedly approved the retained profile.'}
    Remove-Module Performance -ErrorAction SilentlyContinue
    $profile=New-Profile;Save-SealedTestProfile $profile
    $accepted=Get-RedTildeReviewedHeaderDeltaProfile -Root $testRoot
    Assert-RedTildeReviewedHeaderDelta -Profile $accepted -BaselineDependency ([ordered]@{Fingerprint=$accepted.baseline.dependencyFingerprint;Identity=$accepted.baseline.identity}) -CandidateDependency ([ordered]@{Fingerprint=$accepted.candidate.dependencyFingerprint;Identity=$accepted.candidate.identity}) -BaselinePublicHeaders $accepted.baseline.publicHeaders -CandidatePublicHeaders $accepted.candidate.publicHeaders -BaselineBuild @{platform='x64'} -CandidateBuild @{platform='x64'}
    foreach($case in @(
        @{name='inventory boolean numeric';edit={param($p)$p.allOtherInventoryRowsMustBeIdentical=1}},
        @{name='package boolean string';edit={param($p)$p.packageHeadersUnchanged='true'}},
        @{name='build passed numeric';edit={param($p)$p.candidate.buildEvidence.passed=1}},
        @{name='legacy capture boolean string';edit={param($p)$p.baseline.probe.result.legacyCaptureLinked='true';Refresh-ProbeResult $p baseline}},
        @{name='candidate linked boolean numeric';edit={param($p)$p.candidate.probe.result.newCandidateSymbolsLinked=1;Refresh-ProbeResult $p candidate}},
        @{name='string API';edit={param($p)$p.expectedApiRevision='4'}},
        @{name='extra header pair';edit={param($p)$p.allowedHeaderHashPairs+=@{sourcePath='include/DxUi/Other.h';baselineSha256='A'*64;candidateSha256='B'*64}}},
        @{name='case-changed header path';edit={param($p)$p.allowedHeaderHashPairs[0].sourcePath='include/DxUi/dxui.h'}},
        @{name='package inventory row';edit={param($p)$p.candidate.publicHeaders.files[25].sha256='C'*64}},
        @{name='identity extra field';edit={param($p)$p.candidate.identity.Unreviewed='x'}},
        @{name='identity numeric type';edit={param($p)$p.candidate.identity.apiRevision='4'}},
        @{name='compiler context hash';edit={param($p)$p.appCompilerContext.candidate.normalizedProfileSha256='0'*64}},
        @{name='TLOG hash missing';edit={param($p)$p.appCompilerContext.baseline.tlogSha256=$null}},
        @{name='probe result hash';edit={param($p)$p.candidate.probe.resultSha256='0'*64}},
        @{name='probe schema';edit={param($p)$p.candidate.probe.protocol='other'}},
        @{name='candidate default wrong';edit={param($p)$p.candidate.probe.result.markdownDefaultsAreDefault=$false;Refresh-ProbeResult $p candidate}},
        @{name='candidate default wrong type';edit={param($p)$p.candidate.probe.result.markdownDefaultsAreDefault='true';Refresh-ProbeResult $p candidate}},
        @{name='diagnostics wrong type';edit={param($p)$p.candidate.probe.result.diagnosticsEnabled=1;Refresh-ProbeResult $p candidate}},
        @{name='capture link false';edit={param($p)$p.baseline.probe.result.legacyCaptureLinked=$false;Refresh-ProbeResult $p baseline}},
        @{name='new symbol link false';edit={param($p)$p.candidate.probe.result.newCandidateSymbolsLinked=$false;Refresh-ProbeResult $p candidate}},
        @{name='enum drift';edit={param($p)$p.candidate.probe.result.newButtonAppearanceValues=@(4,6);Refresh-ProbeResult $p candidate}},
        @{name='layout zero';edit={param($p)$p.candidate.probe.result.layout.ProgressBar.size=0;Refresh-ProbeResult $p candidate}},
        @{name='library mismatch';edit={param($p)$p.candidate.probe.librarySha256='0'*64}})){
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $profile -Depth 100 -Compress) -AsHashtable;& $case.edit $bad;Expect-Reject $bad $case.name
    }
    if($failures.Count){throw ('V4 invalid fixtures accepted: '+($failures -join ', '))}
    Write-Output "V4 synthetic positive and negative cases passed; evidence retained at $testRoot"
} finally {Remove-Module Performance -ErrorAction SilentlyContinue;Import-Module $moduleSource -Force}
