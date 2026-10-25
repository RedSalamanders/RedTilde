[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../Build/Performance.psm1') -Force
function Require([bool]$Value,[string]$Message) { if(-not $Value) { throw $Message } }
Require ((Get-RedTildeMedian @(1,4,2,3)) -eq 2.5) 'Even medians preserve both central values.'
$first=New-RedTildePerformanceSchedule -Blocks 12 -Seed 20261021
$second=New-RedTildePerformanceSchedule -Blocks 12 -Seed 20261021
Require (($first | ConvertTo-Json -Compress) -ceq ($second | ConvertTo-Json -Compress)) 'The retained seed replays the literal schedule.'
Require ($first.Count -eq 24 -and @($first | Where-Object { $_.order -cnotin @('ABBA','BAAB') }).Count -eq 0) 'Every independent block has two slots per side.'
Require ((Get-RedTildeSignFlipP @(1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0)) -eq (2.0/4096)) 'The exact two-sided block test has the specified minimum p.'
function Pair([double]$Ratio) {
    $pairs=foreach($scenario in @('Compose','Split')) { for($index=0;$index -lt 12;++$index) {
        $a=@{}; $b=@{}
        foreach($metric in @('commandUs','ownerCpuCycles','completedEditUs','cleanCompletedUs','privateBytes','workingSetBytes')) {
            $a[$metric]=100.0; $b[$metric]=100.0*$Ratio
        }
        @{scenario=$scenario;block=$index;A1=$a;A2=$a;B1=$b;B2=$b}
    } }; return @($pairs)
}
$same=Compare-RedTildePerformanceBlocks (Pair 1.0)
Require ($same.Count -eq 12 -and @($same | Where-Object finding -cne 'no-resolved-change').Count -eq 0) 'Identical native metrics show no resolved change.'
$slower=Compare-RedTildePerformanceBlocks (Pair 1.1)
Require (@($slower | Where-Object finding -ceq 'regressed').Count -eq 12) 'A stable 10 percent regression survives the complete Holm family.'
$faster=Compare-RedTildePerformanceBlocks (Pair 0.8)
Require (@($faster | Where-Object finding -ceq 'improved').Count -eq 12) 'Improvement keeps its direction.'
$mixed=Pair 1.0
for($index=0;$index -lt 12;++$index) {
    $value=100.0*[math]::Exp($(if($index -lt 10) { 0.1 } else { -0.1 }))
    $mixed[$index].B1.commandUs=$value; $mixed[$index].B2.commandUs=$value
}
$row=@(Compare-RedTildePerformanceBlocks $mixed | Where-Object { $_.scenario -ceq 'Compose' -and $_.metric -ceq 'commandUs' })[0]
# Twelve equal-magnitude signs: two tails of C(12,0)+C(12,1)+C(12,2), independently known.
Require ([math]::Abs($row.p-158.0/4096) -lt 1e-12) 'Mixed signs retain their independently counted exact p-value.'
Require ([math]::Abs($row.adjustedP-0.462890625) -lt 1e-12 -and $row.finding -ceq 'no-resolved-change') 'Holm preserves fractional probabilities instead of integer rounding.'
$invalid=Pair 1.0; $invalid[0].B1.commandUs=0.0
$rejected=$false; try { Compare-RedTildePerformanceBlocks $invalid | Out-Null } catch { $rejected=$true }
Require $rejected 'Missing/zero metrics cannot become an accepted comparison.'
$publicHeaders='a'*64
$dependencyA=[ordered]@{Fingerprint=('1'*64);Identity=[ordered]@{commit=('a'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
$dependencyB=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
Assert-RedTildeDependencyComparison $dependencyA $dependencyA $publicHeaders $publicHeaders
Assert-RedTildeDependencyComparison $dependencyA $dependencyB $publicHeaders $publicHeaders -DependencyAdoption
Assert-RedTildeDependencyComparison $dependencyA $dependencyA $publicHeaders $publicHeaders -DependencyAdoption
$dependencyAPscustom=ConvertFrom-Json -InputObject (ConvertTo-Json -InputObject $dependencyA -Depth 10)
$dependencyBPscustom=ConvertFrom-Json -InputObject (ConvertTo-Json -InputObject $dependencyB -Depth 10)
Assert-RedTildeDependencyComparison $dependencyAPscustom $dependencyBPscustom $publicHeaders $publicHeaders -DependencyAdoption
function Require-DependencyRejected([string]$Name,$Baseline,$Candidate,[string]$BaselineHeaders=$publicHeaders,[string]$CandidateHeaders=$publicHeaders,[switch]$Adoption) {
    $rejected=$false
    try {
        if($Adoption) { Assert-RedTildeDependencyComparison $Baseline $Candidate $BaselineHeaders $CandidateHeaders -DependencyAdoption }
        else { Assert-RedTildeDependencyComparison $Baseline $Candidate $BaselineHeaders $CandidateHeaders }
    } catch { $rejected=$true }
    Require $rejected "Dependency comparison rejects $Name."
}
Require-DependencyRejected 'unqualified dependency change' $dependencyA $dependencyB
$unexpectedDependency=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)};newField='changed'}
Require-DependencyRejected 'unexpected outer field' $dependencyA $unexpectedDependency $publicHeaders $publicHeaders -Adoption
$changedToolset=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v146';runtimeFamily='MD';compilerSha256=('b'*64)}}
Require-DependencyRejected 'toolchain field change' $dependencyA $changedToolset $publicHeaders $publicHeaders -Adoption
$changedRuntime=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MT';compilerSha256=('b'*64)}}
Require-DependencyRejected 'runtime field change' $dependencyA $changedRuntime $publicHeaders $publicHeaders -Adoption
$changedApi=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=5;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
Require-DependencyRejected 'API revision change' $dependencyA $changedApi $publicHeaders $publicHeaders -Adoption
$booleanApi=$dependencyB|ConvertTo-Json -Depth 10|ConvertFrom-Json -AsHashtable
$booleanApi.Identity.apiRevision=$true
Require-DependencyRejected 'boolean API revision coercion' $dependencyA $booleanApi $publicHeaders $publicHeaders -Adoption
Require-DependencyRejected 'public header digest change' $dependencyA $dependencyB $publicHeaders ('d'*64) -Adoption
$missingMetadata=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD'}}
Require-DependencyRejected 'missing compiler identity metadata' $dependencyA $missingMetadata $publicHeaders $publicHeaders -Adoption
$missingCommit=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
Require-DependencyRejected 'missing dependency commit' $dependencyA $missingCommit $publicHeaders $publicHeaders -Adoption
$missingFingerprint=[ordered]@{Identity=[ordered]@{commit=('c'*40);apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
Require-DependencyRejected 'missing dependency fingerprint' $dependencyA $missingFingerprint $publicHeaders $publicHeaders -Adoption
$invalidCommit=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{commit='not-a-commit';apiRevision=4;platform='x64';toolset='v145';runtimeFamily='MD';compilerSha256=('b'*64)}}
Require-DependencyRejected 'malformed adoption commit' $dependencyA $invalidCommit $publicHeaders $publicHeaders -Adoption
$beforeA=ConvertTo-Json -InputObject $dependencyA -Depth 10 -Compress
$beforeB=ConvertTo-Json -InputObject $dependencyB -Depth 10 -Compress
Assert-RedTildeDependencyComparison $dependencyA $dependencyB $publicHeaders $publicHeaders -DependencyAdoption
Require ((ConvertTo-Json -InputObject $dependencyA -Depth 10 -Compress) -ceq $beforeA -and
    (ConvertTo-Json -InputObject $dependencyB -Depth 10 -Compress) -ceq $beforeB) 'Dependency adoption comparisons leave caller-owned objects unchanged.'
function New-CompilerComparisonFixture([string]$Commit,[string]$Fingerprint,[string]$Suffix,[string[]]$Extra=@()) {
    $source='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+$Commit+'\INCLUDE'
    $package='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+$Fingerprint+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE'
    $command='/c /I'+$source+' /I"'+$package+'" /I<ROOT>\include /O2 /MD /D TEXT=<ROOT>\opaque '+($Extra -join ' ')
    $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($Suffix+$command)))
    return [ordered]@{schemaVersion=2;sha256=$sha;commands=@($command)}
}
$compilerA=New-CompilerComparisonFixture ('a'*40) ('1'*64) 'a'
$compilerB=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'b'
Assert-RedTildeCompilerComparison $compilerA $compilerA $dependencyA $dependencyA
Assert-RedTildeCompilerComparison $compilerA $compilerB $dependencyA $dependencyB -DependencyAdoption
function Require-CompilerRejected([string]$Name,$Baseline,$Candidate,$BaselinePin=$dependencyA,$CandidatePin=$dependencyB) {
    $rejected=$false
    try { Assert-RedTildeCompilerComparison $Baseline $Candidate $BaselinePin $CandidatePin -DependencyAdoption } catch {$rejected=$true}
    Require $rejected "Compiler adoption rejects $Name."
}
Require-CompilerRejected 'optimization change' $compilerA (New-CompilerComparisonFixture ('c'*40) ('2'*64) 'c' @('/Od'))
Require-CompilerRejected 'runtime change' $compilerA (New-CompilerComparisonFixture ('c'*40) ('2'*64) 'd' @('/MT'))
Require-CompilerRejected 'macro change' $compilerA (New-CompilerComparisonFixture ('c'*40) ('2'*64) 'e' @('/D TEXT=<ROOT>\changed'))
Require-CompilerRejected 'boolean compiler schema version' $compilerA ([ordered]@{schemaVersion=$true;sha256=('f'*64);commands=@($compilerB.commands[0])})
Require-CompilerRejected 'unknown include path change' $compilerA (New-CompilerComparisonFixture ('c'*40) ('2'*64) 'f' @('/I<ROOT>\different'))
$separateUnknownA=New-CompilerComparisonFixture ('a'*40) ('1'*64) 'j'
$separateUnknownB=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'k'
$separateUnknownA.commands=@($separateUnknownA.commands[0].Replace('/I<ROOT>\include','/I <ROOT>\shared'))
$separateUnknownB.commands=@($separateUnknownB.commands[0].Replace('/I<ROOT>\include','/I <ROOT>\shared'))
Assert-RedTildeCompilerComparison $separateUnknownA $separateUnknownB $dependencyA $dependencyB -DependencyAdoption
$changedSeparateUnknown=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'l'
$changedSeparateUnknown.commands=@($changedSeparateUnknown.commands[0].Replace('/I<ROOT>\include','/I <ROOT>\different'))
Require-CompilerRejected 'changed separate unknown include operand' $separateUnknownA $changedSeparateUnknown
$missingSource=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'g'
$missingSource.commands=@($missingSource.commands[0].Replace('/I<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+('c'*40)+'\INCLUDE ',''))
Require-CompilerRejected 'missing declared source include' $compilerA $missingSource
$missingPackage=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'h'
$missingPackage.commands=@($missingPackage.commands[0].Replace('/I"<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('2'*64)+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE" ',''))
Require-CompilerRejected 'missing declared package include' $compilerA $missingPackage
$duplicateInclude=New-CompilerComparisonFixture ('c'*40) ('2'*64) 'i'
$duplicateInclude.commands=@($duplicateInclude.commands[0].Replace('/I"<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('2'*64)+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE" ',
    '/I"<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('2'*64)+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE" /I"<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('2'*64)+'\VCPKG_INSTALLED\X64\X64-WINDOWS\INCLUDE" '))
Require-CompilerRejected 'duplicate expected package include' $compilerA $duplicateInclude
$missingIdentity=[ordered]@{Fingerprint=('2'*64);Identity=[ordered]@{apiRevision=4}}
Require-CompilerRejected 'missing dependency identity commit' $compilerA $compilerB $dependencyA $missingIdentity
$fixtureRoot=Join-Path $PSScriptRoot ('../.build/performance-tooling/'+[guid]::NewGuid().ToString('N'))
function Write-CompilerFixture([string]$Name,[string[]]$Files,[string[]]$Flags) {
    $directory=[IO.Path]::GetFullPath((Join-Path $fixtureRoot $Name))
    $projectDirectory=Join-Path $directory 'RedTilde'
    $logs=Join-Path $directory '.build/obj/x64/Release/RedTilde/RedTilde.tlog'
    [void][IO.Directory]::CreateDirectory($projectDirectory)
    [void][IO.Directory]::CreateDirectory($logs)
    $items=($Files|ForEach-Object{'<ClCompile Include="'+$_+'" />'}) -join ''
    '<Project><ItemGroup>'+$items+'</ItemGroup></Project>'|Set-Content (Join-Path $projectDirectory 'RedTilde.vcxproj')
    $lines=@(for($index=0;$index -lt $Files.Count;++$index) {
        $source=Join-Path $projectDirectory $Files[$index]
        '^'+$source.ToUpperInvariant()
        '/c /I"'+$directory.ToUpperInvariant()+'\INCLUDE" '+$Flags[$index]+' /Fo"'+$directory.ToUpperInvariant()+'\.BUILD\OBJ\\" "'+$source.ToUpperInvariant()+'"'
    })
    $lines|Set-Content (Join-Path $logs 'CL.command.1.tlog')
    return $directory
}
$profileA=Get-RedTildeCompilerProfile (Write-CompilerFixture 'baseline' @('A.cpp','B.cpp') @('/O2 /W4 /WX','/O2 /W4 /WX'))
$profileB=Get-RedTildeCompilerProfile (Write-CompilerFixture 'candidate' @('A.cpp','B.cpp','New.cpp') @('/O2 /W4 /WX','/O2 /W4 /WX','/O2 /W4 /WX'))
Require ($profileA.schemaVersion -eq 2 -and $profileA.sha256 -ceq $profileB.sha256 -and
    $profileA.rawSha256 -cne $profileB.rawSha256 -and $profileA.translationUnits.Count -eq 2 -and $profileB.translationUnits.Count -eq 3) `
    'Only repository placement and translation-unit identity normalize; raw provenance remains distinct.'
foreach($flag in @('/Od /W4 /WX','/O2 /W3 /WX','/O2 /W4 /WX /D NEW_BEHAVIOR','/O2 /W4 /WX /fsanitize=address','/O2 /W4 /WX /IC:\External')) {
    $changed=Get-RedTildeCompilerProfile (Write-CompilerFixture ('changed-'+[guid]::NewGuid().ToString('N')) @('A.cpp') @($flag))
    Require ($profileA.sha256 -cne $changed.sha256) 'Optimization, diagnostics, macros, sanitizer and external includes remain exact.'
}
$mixed=Write-CompilerFixture 'mixed' @('A.cpp','B.cpp') @('/O2 /W4 /WX','/Od /W4 /WX')
$missing=Write-CompilerFixture 'missing' @('A.cpp') @('/O2 /W4 /WX')
$missingLog=Join-Path $missing '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
Set-Content $missingLog '^MISSING.CPP'
$duplicate=Write-CompilerFixture 'duplicate' @('A.cpp') @('/O2 /W4 /WX')
$duplicateLog=Join-Path $duplicate '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
@(Get-Content $duplicateLog;Get-Content $duplicateLog)|Set-Content $duplicateLog
$forced=Write-CompilerFixture 'forced' @('A.cpp') @('/O2 /W4 /WX /FIforced.h')
$wrongSource=Write-CompilerFixture 'wrong-source' @('A.cpp','B.cpp') @('/O2 /W4 /WX','/O2 /W4 /WX')
$wrongLog=Join-Path $wrongSource '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
$wrongLines=@(Get-Content $wrongLog);$wrongLines[1]=$wrongLines[1].Replace('\A.CPP','\B.CPP');$wrongLines|Set-Content $wrongLog
$unexpected=Write-CompilerFixture 'unexpected-source' @('A.cpp') @('/O2 /W4 /WX C:\Unreviewed.cpp')
$macroRootA=[IO.Path]::GetFullPath((Join-Path $fixtureRoot 'macro-a'))
$macroRootB=[IO.Path]::GetFullPath((Join-Path $fixtureRoot 'macro-b'))
$macroA=Get-RedTildeCompilerProfile (Write-CompilerFixture 'macro-a' @('A.cpp') @('/O2 /D "ASSET_ROOT='+$macroRootA+'\Fixture"'))
$macroB=Get-RedTildeCompilerProfile (Write-CompilerFixture 'macro-b' @('A.cpp') @('/O2 /D "ASSET_ROOT='+$macroRootB+'\Fixture"'))
Require ($macroA.sha256 -cne $macroB.sha256) 'Repository-like path bytes in macro values are never normalized.'
foreach($directory in @($mixed,$missing,$duplicate,$forced,$wrongSource,$unexpected)) {
    $rejected=$false
    try {Get-RedTildeCompilerProfile $directory|Out-Null} catch {$rejected=$true}
    Require $rejected 'Mixed, missing, duplicate, wrong-source, extra-source and forced-include commands refuse comparison.'
}
foreach($flag in @('/O2 "@C:\External\options.rsp"','/O2 @relative.rsp','/O2 "/FIforced.h"',
    '/O2 ..\External.cpp','/O2 External.cc','/O2 "TEXT=\"a  b\""','/O2 /D "TEXT=\"a b\""','/O2 /D"TEXT=\"a  b\""')) {
    $directory=Write-CompilerFixture ('unsupported-'+[guid]::NewGuid().ToString('N')) @('A.cpp') @($flag)
    $rejected=$false
    try {Get-RedTildeCompilerProfile $directory|Out-Null} catch {$rejected=$true}
    Require $rejected 'Quoted response/forced include, relative sources and escaped embedded quotes fail closed.'
}
$macroSpacesA=Get-RedTildeCompilerProfile (Write-CompilerFixture 'macro-spaces-a' @('A.cpp') @('/O2 /D "TEXT=a  b"'))
$macroSpacesB=Get-RedTildeCompilerProfile (Write-CompilerFixture 'macro-spaces-b' @('A.cpp') @('/O2 /D "TEXT=a b"'))
Require ($macroSpacesA.sha256 -cne $macroSpacesB.sha256) 'Supported quoted macro whitespace remains byte-exact.'
$pathProfiles=@(foreach($name in 'path operand a','path operand b') {
    $directory=[IO.Path]::GetFullPath((Join-Path $fixtureRoot $name))
    Get-RedTildeCompilerProfile (Write-CompilerFixture $name @('A.cpp') @('/O2 /I "'+$directory+'\Headers with spaces" /Fd"'+$directory+'\symbols.pdb"'))
})
Require ($pathProfiles[0].sha256 -ceq $pathProfiles[1].sha256) 'Only reviewed include/output path operands normalize in relocated roots with spaces.'
$nearProfiles=@(foreach($name in 'near-a','near-b') {
    $directory=[IO.Path]::GetFullPath((Join-Path $fixtureRoot $name))
    Get-RedTildeCompilerProfile (Write-CompilerFixture $name @('A.cpp') @('/O2 /I"'+$directory+'-external\Headers"'))
})
Require ($nearProfiles[0].sha256 -cne $nearProfiles[1].sha256) 'A root-prefix near miss remains an exact external include path.'

# Run the actual auditor's identity boundary against synthetic metadata/raw commands. Its typed tokenizer
# is independent of the production profile builder; these fixtures qualify neither a native measurement nor policy.
. (Join-Path $PSScriptRoot 'Scope.Tests.Performance.Evidence.ps1') -SelfTest
$sourceIncludeA='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+('a'*40)+'\INCLUDE'
$packageIncludeA='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('1'*64)+'\VCPKG_INSTALLED\x64\x64-windows\INCLUDE'
$sourceIncludeB='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+('c'*40)+'\INCLUDE'
$packageIncludeB='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+('2'*64)+'\VCPKG_INSTALLED\x64\x64-windows\INCLUDE'
$rawProfileA='/c /I'+$sourceIncludeA+' /I"'+$packageIncludeA+'" /I <ROOT>\shared /D TEXT=<ROOT>\opaque'
$rawProfileB='/c /I'+$sourceIncludeB+' /I"'+$packageIncludeB+'" /I <ROOT>\shared /D TEXT=<ROOT>\opaque'
$neutralA=[RedTilde.PairedEvidenceReference]::NeutralizeCompilerProfile($rawProfileA,$sourceIncludeA,$packageIncludeA)
$neutralB=[RedTilde.PairedEvidenceReference]::NeutralizeCompilerProfile($rawProfileB,$sourceIncludeB,$packageIncludeB)
Require ($neutralA -ceq '/c /I<DXUI_SOURCE_INCLUDE> /I"<DXUI_PACKAGE_INCLUDE>" /I <ROOT>\shared /D TEXT=<ROOT>\opaque' -and $neutralA -ceq $neutralB) `
    'Independent adoption parser changes only the exact source/package /I operands; separate unknown /I and macros remain byte-exact.'
$unknownChanged=[RedTilde.PairedEvidenceReference]::NeutralizeCompilerProfile($rawProfileB.Replace('/I <ROOT>\shared','/I <ROOT>\other'),$sourceIncludeB,$packageIncludeB)
Require ($unknownChanged -cne $neutralA) 'Independent adoption parser retains changed unknown separate include operands.'
$duplicateAdoptionRejected=$false
try {
    [RedTilde.PairedEvidenceReference]::NeutralizeCompilerProfile($rawProfileA.Replace('/I"'+$packageIncludeA+'"','/I"'+$packageIncludeA+'" /I"'+$packageIncludeA+'"'),$sourceIncludeA,$packageIncludeA)|Out-Null
} catch {$duplicateAdoptionRejected=$true}
Require $duplicateAdoptionRejected 'Independent adoption parser rejects duplicate expected package include operands.'
$sourceHeaderRow=[ordered]@{path='include/DxUi/Fixture.h';sha256=('F'*64)}
$packageHeaderRow=[ordered]@{path='package-include/wil/Fixture.h';sha256=('E'*64)}
function Get-HeaderFixtureDigest($Rows) {
    $lines=[string[]]@($Rows|ForEach-Object{$_.path+':'+$_.sha256});[Array]::Sort($lines,[StringComparer]::Ordinal)
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($lines -join "`n"))))
}
$headerDigest=Get-HeaderFixtureDigest @($sourceHeaderRow,$packageHeaderRow)
$sourceHeaderDigest=Get-HeaderFixtureDigest @($sourceHeaderRow)
$packageHeaderDigest=Get-HeaderFixtureDigest @($packageHeaderRow)
$headerEvidence=[ordered]@{schemaVersion=2;sha256=$headerDigest;sourceSha256=$sourceHeaderDigest;packageSha256=$packageHeaderDigest;files=@($sourceHeaderRow,$packageHeaderRow)}
$dependencyStudy=[ordered]@{dependencyComparisonMode='sdk-adoption';
    baseline=[ordered]@{build=@{platform='x64';dependency=$dependencyA};publicHeaders=$headerEvidence};
    candidate=[ordered]@{build=@{platform='x64';dependency=$dependencyB};publicHeaders=$headerEvidence}}
$dependencyComparison=@{dependencyComparisonMode='sdk-adoption'}
Assert-DependencyEvidence $dependencyStudy $dependencyComparison
function Require-RejectedDependencyEvidence([string]$Name,[scriptblock]$Mutation) {
    $copy=$dependencyStudy|ConvertTo-Json -Depth 30|ConvertFrom-Json -AsHashtable
    & $Mutation $copy
    $rejected=$false
    try {Assert-DependencyEvidence $copy $dependencyComparison} catch {$rejected=$true}
    Require $rejected ('Independent dependency evidence rejects '+$Name)
}
Require-RejectedDependencyEvidence 'missing header inventory' {param($study) $study.candidate.Remove('publicHeaders')}
Require-RejectedDependencyEvidence 'corrupt header checksum' {param($study) $study.candidate.publicHeaders.sha256='0'*64}
Require-RejectedDependencyEvidence 'header path traversal' {param($study) $study.candidate.publicHeaders.files[0].path='include/../Fixture.h'}
Require-RejectedDependencyEvidence 'package header path traversal' {param($study) $study.candidate.publicHeaders.files[1].path='package-include/../Fixture.h'}
Require-RejectedDependencyEvidence 'source header digest corruption' {param($study) $study.candidate.publicHeaders.sourceSha256='0'*64}
Require-RejectedDependencyEvidence 'boolean header schema version' {param($study) $study.candidate.publicHeaders.schemaVersion=$true}
Require-RejectedDependencyEvidence 'missing package inventory' {param($study) $study.candidate.publicHeaders.files=@($study.candidate.publicHeaders.files|Where-Object{$_.path -notlike 'package-include/*'})}
Require-RejectedDependencyEvidence 'valid but changed package bytes' {param($study)
    $row=$study.candidate.publicHeaders.files|Where-Object{$_.path -like 'package-include/*'}|Select-Object -First 1
    $row.sha256='A'*64
    $source=@($study.candidate.publicHeaders.files|Where-Object{$_.path -like 'include/*'})
    $package=@($study.candidate.publicHeaders.files|Where-Object{$_.path -like 'package-include/*'})
    $study.candidate.publicHeaders.sourceSha256=Get-HeaderFixtureDigest $source
    $study.candidate.publicHeaders.packageSha256=Get-HeaderFixtureDigest $package
    $study.candidate.publicHeaders.sha256=Get-HeaderFixtureDigest (@($source)+@($package))
}
Require-RejectedDependencyEvidence 'unknown ABI field' {param($study) $study.candidate.build.dependency.Identity.newFlag=$true}
Require-RejectedDependencyEvidence 'runtime change' {param($study) $study.candidate.build.dependency.Identity.runtimeFamily='MDd'}
Require-RejectedDependencyEvidence 'API change' {param($study) $study.candidate.build.dependency.Identity.apiRevision=5}
Require-RejectedDependencyEvidence 'boolean API coercion' {param($study) $study.candidate.build.dependency.Identity.apiRevision=$true}
Require-RejectedDependencyEvidence 'mode downgrade' {param($study) $study.dependencyComparisonMode='same-pin'}
Require-RejectedDependencyEvidence 'build/dependency platform mismatch' {param($study) $study.candidate.build.platform='ARM64'}
$legacyHeaderEvidence=[ordered]@{schemaVersion=1;sha256=(Get-HeaderFixtureDigest @($sourceHeaderRow));files=@($sourceHeaderRow)}
$samePinStudy=[ordered]@{dependencyComparisonMode='same-pin';baseline=[ordered]@{build=@{platform='x64';dependency=$dependencyA};publicHeaders=$legacyHeaderEvidence};
    candidate=[ordered]@{build=@{platform='x64';dependency=$dependencyA};publicHeaders=$legacyHeaderEvidence}}
Assert-DependencyEvidence $samePinStudy @{dependencyComparisonMode='same-pin'}
$legacyAdoptionStudy=[ordered]@{dependencyComparisonMode='sdk-adoption';baseline=[ordered]@{build=@{platform='x64';dependency=$dependencyA};publicHeaders=$legacyHeaderEvidence};
    candidate=[ordered]@{build=@{platform='x64';dependency=$dependencyB};publicHeaders=$legacyHeaderEvidence}}
$rejected=$false;try {Assert-DependencyEvidence $legacyAdoptionStudy $dependencyComparison}catch{$rejected=$true}
Require $rejected 'Historical schema-one headers remain same-pin-only.'
function Copy-IdentityFixture($Value) { return $Value|ConvertTo-Json -Depth 20|ConvertFrom-Json -AsHashtable }
$auditRoot=Join-Path $fixtureRoot 'identity-audit'
[void][IO.Directory]::CreateDirectory($auditRoot)
$profileRoot=Write-CompilerFixture 'audit-source' @('A.cpp','B.cpp') @('/O2 /W4 /WX','/O2 /W4 /WX')
$auditProfile=Get-RedTildeCompilerProfile $profileRoot
$groupedRoot=Write-CompilerFixture 'grouped-source' @('A.cpp','B.cpp') @('/O2 /W4 /WX','/O2 /W4 /WX')
$groupedLog=Join-Path $groupedRoot '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
$groupedLines=@(Get-Content -LiteralPath $groupedLog)
$groupedLines=@(($groupedLines[0]+'|'+$groupedLines[2].Substring(1)),($groupedLines[1]+' "'+$groupedLines[2].Substring(1)+'"'))
$groupedLines|Set-Content -LiteralPath $groupedLog
$grouped=Get-RedTildeCompilerProfile $groupedRoot
Require ($grouped.translationUnits.Count -eq 2 -and $grouped.sha256 -ceq $auditProfile.sha256) 'Grouped MSBuild headings retain both own-source bindings and one homogeneous option profile.'
[RedTilde.PairedEvidenceReference]::VerifyCompilerCommands([string[]]@(Get-Content -LiteralPath $groupedLog),$grouped.sourceRoot,
    [string[]]@($grouped.translationUnits|ForEach-Object source),[string]$grouped.commands[0])
$auditStudy=[ordered]@{baseline=@{compiler=(Copy-IdentityFixture $auditProfile)};candidate=@{compiler=(Copy-IdentityFixture $auditProfile)};
    compilerReceipts=[ordered]@{}}
foreach($side in 'baseline','candidate') {
    $leaf=$side+'-compiler-command.tlog'
    Copy-Item -LiteralPath (Join-Path $profileRoot '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog') -Destination (Join-Path $auditRoot $leaf)
    $auditStudy.compilerReceipts[$side]=@{file=$leaf;sha256=$auditProfile.rawSha256}
}
Assert-CompilerProfileEvidence -Study $auditStudy -Root $auditRoot
function New-RawAdoptionProfile([string]$Name,$Dependency,[string]$UnknownInclude='C:\AuditorFixture\Shared',[string]$Tail='/O2 /MD /D TEXT=<ROOT>\opaque') {
    $sourceRoot=[IO.Path]::GetFullPath((Join-Path $fixtureRoot $Name))
    $triplet='x64-windows'
    $sourceInclude=Join-Path $sourceRoot ('.build/dependencies/DxUi/source/'+$Dependency.Identity.commit+'/include')
    $packageInclude=Join-Path $sourceRoot ('.build/dependencies/DxUi/'+$Dependency.Fingerprint+'/vcpkg_installed/x64/'+$triplet+'/include')
    $flags=$Tail+' /I"'+$sourceInclude+'" /I"'+$packageInclude+'" /I '+$UnknownInclude
    $root=Write-CompilerFixture $Name @('A.cpp','B.cpp') @($flags,$flags)
    return [ordered]@{root=$root;profile=(Get-RedTildeCompilerProfile $root)}
}
$rawAdoptionA=New-RawAdoptionProfile 'raw-adopt-a' $dependencyA
$rawAdoptionB=New-RawAdoptionProfile 'raw-adopt-b' $dependencyB
$rawAdoptionStudy=[ordered]@{dependencyComparisonMode='sdk-adoption';baseline=[ordered]@{build=@{dependency=$dependencyA};compiler=$rawAdoptionA.profile};
    candidate=[ordered]@{build=@{dependency=$dependencyB};compiler=$rawAdoptionB.profile};compilerReceipts=[ordered]@{}}
foreach($side in 'baseline','candidate') {
    $item=if($side -ceq 'baseline'){$rawAdoptionA}else{$rawAdoptionB}
    $leaf='raw-adoption-'+$side+'-compiler-command.tlog'
    $from=Join-Path $item.root '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
    Copy-Item -LiteralPath $from -Destination (Join-Path $auditRoot $leaf)
    $rawAdoptionStudy.compilerReceipts[$side]=@{file=$leaf;sha256=$item.profile.rawSha256}
}
Assert-CompilerProfileEvidence -Study $rawAdoptionStudy -Root $auditRoot
$rawChangedUnknown=New-RawAdoptionProfile 'raw-adopt-unknown' $dependencyB 'C:\AuditorFixture\Different'
$rawChangedStudy=Copy-IdentityFixture $rawAdoptionStudy
$rawChangedStudy.candidate.compiler=$rawChangedUnknown.profile
$rawChangedLeaf='raw-adoption-changed-unknown-compiler-command.tlog'
Copy-Item -LiteralPath (Join-Path $rawChangedUnknown.root '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog') -Destination (Join-Path $auditRoot $rawChangedLeaf)
$rawChangedStudy.compilerReceipts.candidate=@{file=$rawChangedLeaf;sha256=$rawChangedUnknown.profile.rawSha256}
$rawChangedRejected=$false;try {Assert-CompilerProfileEvidence -Study $rawChangedStudy -Root $auditRoot}catch{$rawChangedRejected=$true}
Require $rawChangedRejected 'Independent raw-command adoption audit rejects a changed unknown separate include operand.'
$rawChangedMacro=New-RawAdoptionProfile 'raw-adopt-macro' $dependencyB 'C:\AuditorFixture\Shared' '/O2 /MD /D TEXT=<ROOT>\different'
$rawChangedStudy=Copy-IdentityFixture $rawAdoptionStudy
$rawChangedStudy.candidate.compiler=$rawChangedMacro.profile
$rawChangedLeaf='raw-adoption-changed-macro-compiler-command.tlog'
Copy-Item -LiteralPath (Join-Path $rawChangedMacro.root '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog') -Destination (Join-Path $auditRoot $rawChangedLeaf)
$rawChangedStudy.compilerReceipts.candidate=@{file=$rawChangedLeaf;sha256=$rawChangedMacro.profile.rawSha256}
$rawChangedRejected=$false;try {Assert-CompilerProfileEvidence -Study $rawChangedStudy -Root $auditRoot}catch{$rawChangedRejected=$true}
Require $rawChangedRejected 'Independent raw-command adoption audit rejects changed macro bytes.'
function Require-RejectedCompilerEvidence([string]$Name,[scriptblock]$Mutation) {
    $changed=Copy-IdentityFixture $auditStudy
    & $Mutation $changed
    $rejected=$false
    try {Assert-CompilerProfileEvidence -Study $changed -Root $auditRoot} catch {$rejected=$true}
    Require $rejected "Independent compiler identity audit rejects $Name."
}
Require-RejectedCompilerEvidence 'normalized digest corruption' {param($study) $study.baseline.compiler.sha256='0'*64}
Require-RejectedCompilerEvidence 'missing raw metadata' {param($study) $study.compilerReceipts.Remove('baseline')}
Require-RejectedCompilerEvidence 'raw path traversal' {param($study) $study.compilerReceipts.baseline.file='..\elsewhere.tlog'}
Require-RejectedCompilerEvidence 'raw digest corruption' {param($study) $study.compilerReceipts.baseline.sha256='0'*64}
Require-RejectedCompilerEvidence 'empty TU inventory' {param($study) $study.baseline.compiler.translationUnits=@()}
Require-RejectedCompilerEvidence 'TU flag mismatch' {param($study) $study.baseline.compiler.translationUnits[0].command='/Od'}
Require-RejectedCompilerEvidence 'case-only duplicate TU' {param($study)
    $profile=$study.baseline.compiler;$copy=Copy-IdentityFixture $profile.translationUnits[0]
    $copy.source=$copy.source.ToUpperInvariant();$profile.translationUnits+=@($copy)
}
Require-RejectedCompilerEvidence 'separator-alias duplicate TU' {param($study)
    $profile=$study.baseline.compiler;$copy=Copy-IdentityFixture $profile.translationUnits[0]
    $copy.source=$copy.source.Replace('\','/');$profile.translationUnits+=@($copy)
}
Require-RejectedCompilerEvidence 'relative parent traversal' {param($study) $study.baseline.compiler.translationUnits[0].source='..\A.cpp'}
Require-RejectedCompilerEvidence 'relative dot alias' {param($study) $study.baseline.compiler.translationUnits[0].source='RedTilde\.\A.cpp'}
Require-RejectedCompilerEvidence 'wrong recorded checkout root' {param($study) $study.baseline.compiler.sourceRoot=Join-Path $fixtureRoot 'other-source'}
Require-RejectedCompilerEvidence 'one-side schema downgrade' {param($study) $study.baseline.compiler.Remove('schemaVersion')}
Require-RejectedCompilerEvidence 'boolean schema version' {param($study) $study.baseline.compiler.schemaVersion=$true}
Require-RejectedCompilerEvidence 'both-side schema downgrade' {param($study)
    $study.baseline.compiler.Remove('schemaVersion');$study.candidate.compiler.Remove('schemaVersion')
}
Require-RejectedCompilerEvidence 'self-consistent invented normalized command' {param($study)
    $profile=$study.baseline.compiler;$profile.commands=@('/c /Od')
    foreach($unit in $profile.translationUnits){$unit.command='/c /Od'}
    $profile.sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes("redtilde-release-compiler-profile-v2`n/c /Od")))
}
Require-RejectedCompilerEvidence 'self-consistent changed raw optimization' {param($study)
    $raw=Join-Path $auditRoot 'altered-compiler.tlog'
    $text=[IO.File]::ReadAllText((Join-Path $auditRoot 'baseline-compiler-command.tlog')).Replace('/O2','/Od')
    [IO.File]::WriteAllText($raw,$text)
    $hash=(Get-FileHash -LiteralPath $raw).Hash
    $study.baseline.compiler.rawSha256=$hash;$study.compilerReceipts.baseline=@{file='altered-compiler.tlog';sha256=$hash}
}
Require-RejectedCompilerEvidence 'unsupported quoted compiler switch with consistent digests' {param($study)
    $raw=Join-Path $auditRoot 'quoted-compiler.tlog'
    $text=[IO.File]::ReadAllText((Join-Path $auditRoot 'baseline-compiler-command.tlog')).Replace('/O2','"/O2"')
    [IO.File]::WriteAllText($raw,$text)
    $profile=$study.baseline.compiler;$command=$profile.commands[0].Replace('/O2','"/O2"');$profile.commands=@($command)
    foreach($unit in $profile.translationUnits){$unit.command=$command}
    $profile.sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes("redtilde-release-compiler-profile-v2`n"+$command)))
    $hash=(Get-FileHash -LiteralPath $raw).Hash
    $profile.rawSha256=$hash;$study.compilerReceipts.baseline=@{file='quoted-compiler.tlog';sha256=$hash}
}
$legacyRoot=Join-Path $fixtureRoot 'legacy-audit'
[void][IO.Directory]::CreateDirectory($legacyRoot)
$legacy=[ordered]@{baseline=@{compiler=@{sha256='historical';commands=@('historical commands');rawSha256='historical'}};
    candidate=@{compiler=@{sha256='historical';commands=@('historical commands');rawSha256='historical'}}}
Assert-CompilerProfileEvidence -Study $legacy -Root $legacyRoot
[IO.File]::WriteAllText((Join-Path $legacyRoot 'baseline-compiler-command.tlog'),'synthetic new artifact')
$rejected=$false
try {Assert-CompilerProfileEvidence -Study $legacy -Root $legacyRoot} catch {$rejected=$true}
Require $rejected 'Raw command artifacts cannot masquerade as schema-less legacy evidence.'
Write-Host "Performance tooling: numerical policy and independent compiler identity checks passed ($script:checks reference/audit assertions)."
