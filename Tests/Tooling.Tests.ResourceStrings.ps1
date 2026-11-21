<#
.SYNOPSIS
Checks RedTilde's STRINGTABLE resources without native compilation: UTF-8 code page, unique IDs,
Resource.h parity, positional placeholders only, no printf specifiers, and all C++ IDS_* symbols defined.
#>
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$rcPath=Join-Path $root 'res/RedTilde.rc'
$rhPath=Join-Path $root 'res/Resource.h'
$checks=0
$violations=[Collections.Generic.List[string]]::new()
function Require([bool]$Condition,[string]$Message){
    if(-not $Condition){$script:violations.Add($Message)}
    $script:checks++
}
function Test-PrintfToken([string]$Text){
    $match=[regex]::Match($Text,'%(?:[-+#0'']*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|j|z|t|L|w|I64|I32|I)?[sSdiuxXoOfeEgGaAcCpPn])')
    if($match.Success){return $match.Value}
    return $null
}
function Get-Placeholders([string]$Text){
    $found=[Collections.Generic.List[object]]::new()
    $scrubbed=$Text.Replace('{{','').Replace('}}','')
    if($scrubbed -match '(?<!\{)\{[^{}]*$' -or $scrubbed -match '^[^{}]*\}(?!\})'){
        $found.Add([pscustomobject]@{Token='{unmatched}';Index=$null})
    }
    foreach($match in [regex]::Matches($scrubbed,'(?<!\{)\{([^{}]*)\}(?!\})')){
        $body=$match.Groups[1].Value
        $indexed=[regex]::Match($body,'^(?<index>\d+)(?::[^}]*)?$')
        $found.Add([pscustomobject]@{Token=$match.Value;Index=if($indexed.Success){[int]$indexed.Groups['index'].Value}else{$null}})
    }
    return ,$found
}

# Gate self-checks: ensure literal percent signs pass and printf/non-positional forms fail.
foreach($allowed in @('100%','{0}%','Text size (%)')){
    Require ($null -eq (Test-PrintfToken $allowed)) "Resource string gate false positive on '$allowed'."
}
foreach($forbidden in @('%s','%d','%u','%zu','%ls','%ws','%x','%f','%c','%p','%.2f')){
    Require ($null -ne (Test-PrintfToken $forbidden)) "Resource string gate missed printf token '$forbidden'."
}
Require (@(Get-Placeholders '{}' | Where-Object { $null -eq $_.Index }).Count -eq 1) 'Resource string gate missed bare {}.'
Require (@(Get-Placeholders '{:08X}' | Where-Object { $null -eq $_.Index }).Count -eq 1) 'Resource string gate missed unindexed {:08X}.'

$rcLines=[IO.File]::ReadAllLines($rcPath,[Text.UTF8Encoding]::new($false))
Require (@($rcLines | Where-Object { $_.Trim() -ceq '#pragma code_page(65001)' }).Count -ge 1) 'res/RedTilde.rc must declare #pragma code_page(65001).'

$entries=[Collections.Generic.List[object]]::new()
$inTable=$false;$inBlock=$false
for($index=0;$index -lt $rcLines.Count;$index++){
    $line=$rcLines[$index].Trim()
    if(-not $inTable){if($line -ceq 'STRINGTABLE'){$inTable=$true};continue}
    if(-not $inBlock){if($line -ceq 'BEGIN' -or $line -ceq '{'){$inBlock=$true};continue}
    if($line -ceq 'END' -or $line -ceq '}'){$inTable=$false;$inBlock=$false;continue}
    if($line -eq '' -or $line.StartsWith('//')){continue}
    $match=[regex]::Match($line,'^(?<id>[A-Za-z_][A-Za-z0-9_]*)\s*,?\s*"(?<text>(?:[^"]|"")*)"\s*$')
    if(-not $match.Success){$script:violations.Add("${rcPath}:$($index+1) is not a one-line STRINGTABLE entry: $line");continue}
    $entries.Add([pscustomobject]@{Id=$match.Groups['id'].Value;Text=$match.Groups['text'].Value.Replace('""','"');Path=$rcPath;Line=$index+1})
}
Require ($entries.Count -gt 0) 'res/RedTilde.rc has no STRINGTABLE entries.'

$defined=@{}
$definedValues=@{}
foreach($match in [regex]::Matches([IO.File]::ReadAllText($rhPath),'(?m)^\s*#define\s+(?<id>IDS_[A-Za-z0-9_]+)\s+(?<value>\d+)')){
    $idName=$match.Groups['id'].Value
    $idValue=[int]$match.Groups['value'].Value
    Require (-not $defined.ContainsKey($idName)) "res/Resource.h defines duplicate symbol $idName."
    Require (-not $definedValues.ContainsKey($idValue)) "res/Resource.h assigns duplicate STRINGTABLE numeric value $idValue ($idName vs $($definedValues[$idValue]))."
    $defined[$idName]=$idValue
    $definedValues[$idValue]=$idName
}

$rcById=@{}
foreach($entry in $entries){
    Require (-not $rcById.ContainsKey($entry.Id)) "$($entry.Path):$($entry.Line) repeats $($entry.Id)."
    $rcById[$entry.Id]=$entry
    Require ($defined.ContainsKey($entry.Id)) "$($entry.Path):$($entry.Line) $($entry.Id) is not defined in res/Resource.h."
    $placeholders=Get-Placeholders $entry.Text
    foreach($placeholder in $placeholders | Where-Object { $null -eq $_.Index }){
        Require $false "$($entry.Path):$($entry.Line) $($entry.Id) uses non-positional or unmatched placeholder $($placeholder.Token)."
    }
    $indexes=@($placeholders | Where-Object { $null -ne $_.Index } | ForEach-Object Index | Sort-Object -Unique)
    if($indexes.Count){
        Require (($indexes -join ',') -ceq ((0..($indexes.Count-1)) -join ',')) "$($entry.Path):$($entry.Line) $($entry.Id) skips a placeholder index ($($indexes -join ','))."
    }
    $printf=Test-PrintfToken $entry.Text
    Require ($null -eq $printf) "$($entry.Path):$($entry.Line) $($entry.Id) uses printf specifier $printf."
}

$sourceDirs=@('Common','RedTilde','Tests') | ForEach-Object { Join-Path $root $_ } | Where-Object { Test-Path -LiteralPath $_ -PathType Container }
$sourceFiles=@(Get-ChildItem -Path $sourceDirs -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.h') -and $_.Name -ne 'Resource.h' })
foreach($file in $sourceFiles){
    $content=[IO.File]::ReadAllText($file.FullName)
    foreach($match in [regex]::Matches($content,'\bIDS_[A-Z0-9_]+\b')){
        $relative=[IO.Path]::GetRelativePath($root,$file.FullName).Replace('\','/')
        Require ($rcById.ContainsKey($match.Value)) "$relative uses $($match.Value), which res/RedTilde.rc does not define."
    }
}

if($violations.Count){throw "Resource string gate failed:`n$($violations -join "`n")"}
Write-Host "PASS: resource strings ($checks checks, $($entries.Count) base strings)"
