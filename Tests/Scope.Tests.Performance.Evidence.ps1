[CmdletBinding()]
param([string[]]$StudyPath=@(), [string]$Output='', [switch]$SelfTest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

# A typed reference implementation avoids the PowerShell numeric binder and the
# production judge's bit-mask enumeration. It does not grant policy acceptance.
if(-not ('RedTilde.PairedEvidenceReference' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Linq;
namespace RedTilde {
public static class PairedEvidenceReference {
    public static double Median(double[] values) {
        if(values.Length == 0 || values.Any(v => double.IsNaN(v) || double.IsInfinity(v)))
            throw new ArgumentException("Finite nonempty observations required.");
        var ordered = (double[])values.Clone();
        Array.Sort(ordered);
        int middle = ordered.Length / 2;
        return ordered.Length % 2 == 1 ? ordered[middle] : (ordered[middle-1] + ordered[middle]) / 2.0;
    }
    public static double Tail(double[] differences) {
        if(differences.Length < 2 || differences.Length > 16) throw new ArgumentException("2..16 blocks required.");
        var sums = new List<double> { 0.0 };
        foreach(double value in differences) {
            var next = new List<double>(sums.Count * 2);
            foreach(double sum in sums) { next.Add(sum + value); next.Add(sum - value); }
            sums = next;
        }
        double threshold = Math.Abs(differences.Sum()) - 1e-12;
        return (double)sums.Count(v => Math.Abs(v) >= threshold) / sums.Count;
    }
    public static double Effect(double[] differences) { return Math.Exp(differences.Average()) - 1.0; }
    public static double[] Holm(double[] probabilities) {
        int[] ranks = Enumerable.Range(0, probabilities.Length).OrderBy(i => probabilities[i]).ToArray();
        var adjusted = new double[probabilities.Length];
        double previous = 0.0;
        for(int rank=0; rank<ranks.Length; ++rank) {
            int index = ranks[rank];
            previous = Math.Max(previous, Math.Min(1.0, probabilities[index] * (ranks.Length-rank)));
            adjusted[index] = previous;
        }
        return adjusted;
    }
    // Independent supported-grammar reader. Preserve token bytes, including whitespace inside quoted macros;
    // reject embedded escaped quotes rather than attempting a different Windows command-line interpretation.
    static string[] CompilerTokens(string command) {
        var tokens = new List<string>();
        int start = -1, slashes = 0;
        bool quoted = false;
        for(int index = 0; index < command.Length; ++index) {
            char ch = command[index];
            if(ch == '"') {
                if(slashes % 2 != 0) throw new ArgumentException("Unsupported escaped compiler quote.");
                quoted = !quoted;
            }
            if(char.IsWhiteSpace(ch) && !quoted) {
                if(start >= 0) { tokens.Add(command.Substring(start,index-start)); start = -1; }
            } else if(start < 0) start = index;
            slashes = ch == '\\' ? slashes + 1 : 0;
        }
        if(quoted) throw new ArgumentException("Unbalanced compiler quote.");
        if(start >= 0) tokens.Add(command.Substring(start));
        return tokens.ToArray();
    }
    static string Unquote(string token) {
        if(token.Length >= 2 && token[0] == '"' && token[token.Length-1] == '"')
            return token.Substring(1,token.Length-2);
        return token;
    }
    static string CompilerPath(string path) { return path.Replace('/','\\').TrimEnd('\\'); }
    static bool SamePath(string left,string right) {
        return string.Equals(CompilerPath(left),CompilerPath(right),StringComparison.OrdinalIgnoreCase);
    }
    static string NormalizeCompilerPath(string operand,string root) {
        bool quoted = operand.Length >= 2 && operand[0] == '"' && operand[operand.Length-1] == '"';
        string value = Unquote(operand);
        if(value.Length == 0 || value.IndexOf('"') >= 0) throw new ArgumentException("Unsupported compiler path operand.");
        if(string.Equals(value,root,StringComparison.OrdinalIgnoreCase) ||
            value.StartsWith(root+"\\",StringComparison.OrdinalIgnoreCase) || value.StartsWith(root+"/",StringComparison.OrdinalIgnoreCase))
            value = "<ROOT>" + value.Substring(root.Length);
        return quoted ? "\""+value+"\"" : value;
    }
    static string NormalizeCompilerCommand(string command,string root,string source,string[] reviewed) {
        string[] tokens = CompilerTokens(command);
        var options = new List<string>();
        int own = 0;
        for(int index = 0; index < tokens.Length; ++index) {
            string token = tokens[index], argument = Unquote(token);
            if(token.StartsWith("\"",StringComparison.Ordinal) && argument.StartsWith("/",StringComparison.Ordinal))
                throw new ArgumentException("Unsupported quoted compiler switch.");
            if(argument == "/D" || argument == "/U") {
                if(index+1 == tokens.Length) throw new ArgumentException("Missing macro operand.");
                options.Add(token); options.Add(tokens[++index]); continue;
            }
            if(argument.StartsWith("/D",StringComparison.Ordinal) || argument.StartsWith("/U",StringComparison.Ordinal)) {
                options.Add(token); continue;
            }
            if(argument.StartsWith("/FI",StringComparison.OrdinalIgnoreCase) || argument.StartsWith("@",StringComparison.Ordinal) ||
                argument.StartsWith("/Tp",StringComparison.Ordinal) || argument.StartsWith("/Tc",StringComparison.Ordinal))
                throw new ArgumentException("Unsupported forced include, response or per-source argument.");
            string flag = new[]{"/I","/Fo","/Fd"}.FirstOrDefault(value => token.StartsWith(value,StringComparison.Ordinal));
            if(flag != null) {
                string operand = token.Substring(flag.Length);
                if(operand.Length == 0) {
                    if(index+1 == tokens.Length) throw new ArgumentException("Missing compiler path operand.");
                    options.Add(token); options.Add(NormalizeCompilerPath(tokens[++index],root));
                } else options.Add(flag+NormalizeCompilerPath(operand,root));
                continue;
            }
            if(reviewed.Any(value => SamePath(argument,value))) {
                if(SamePath(argument,source)) ++own;
                continue;
            }
            string extension = System.IO.Path.GetExtension(argument);
            if(new[]{".cpp",".c",".cc",".cxx"}.Contains(extension,StringComparer.OrdinalIgnoreCase))
                throw new ArgumentException("Unreviewed compiler source argument.");
            options.Add(token);
        }
        if(own != 1) throw new ArgumentException("Each compiler row must contain its own source exactly once.");
        return string.Join(" ",options);
    }
    static string NeutralizeDependencyIncludes(string command,string sourceInclude,string packageInclude) {
        string[] tokens = CompilerTokens(command);
        var options = new List<string>();
        int sourceCount = 0, packageCount = 0;
        for(int index=0; index<tokens.Length; ++index) {
            string token=tokens[index], flag, operand;
            bool separate=false;
            if(token == "/D" || token == "/U") {
                if(index+1 == tokens.Length) throw new ArgumentException("Missing macro operand.");
                options.Add(token); options.Add(tokens[++index]); continue;
            }
            if(token == "/I") {
                if(index+1 == tokens.Length) throw new ArgumentException("Missing include operand.");
                flag=token; operand=tokens[++index]; separate=true;
            } else if(token.StartsWith("/I",StringComparison.Ordinal)) {
                flag="/I"; operand=token.Substring(2);
                if(operand.Length == 0) throw new ArgumentException("Missing include operand.");
            } else { options.Add(token); continue; }
            bool quoted=operand.Length >= 2 && operand[0] == '"' && operand[operand.Length-1] == '"';
            string path=Unquote(operand), marker=null;
            if(path.IndexOf('"') >= 0) throw new ArgumentException("Unsupported include quoting.");
            if(string.Equals(path,sourceInclude,StringComparison.OrdinalIgnoreCase)) { ++sourceCount; marker="<DXUI_SOURCE_INCLUDE>"; }
            else if(string.Equals(path,packageInclude,StringComparison.OrdinalIgnoreCase)) { ++packageCount; marker="<DXUI_PACKAGE_INCLUDE>"; }
            if(marker == null) {
                if(separate) { options.Add(flag); options.Add(operand); }
                else options.Add(flag+operand);
                continue;
            }
            string normalized=quoted ? "\""+marker+"\"" : marker;
            if(separate) { options.Add(flag); options.Add(normalized); }
            else options.Add(flag+normalized);
        }
        if(sourceCount != 1 || packageCount != 1)
            throw new ArgumentException("Each command must contain exactly one declared DxUi source and package include.");
        return string.Join(" ",options);
    }
    public static string VerifyCompilerCommands(string[] lines,string root,string[] relativeSources,string expected,string[] adoptionIncludes=null) {
        string[] reviewed = relativeSources.Select(value => root.TrimEnd('\\','/')+"\\"+value.Replace('/','\\')).ToArray();
        string adopted = null;
        foreach(string source in reviewed) {
            var commands = new List<string>();
            for(int index = 0; index+1 < lines.Length; ++index) {
                string line = lines[index].TrimStart('\uFEFF');
                if(line.StartsWith("^",StringComparison.Ordinal) && line.Substring(1).Split('|').Any(value => SamePath(value,source)))
                    commands.Add(lines[index+1]);
            }
            if(commands.Count != 1 || string.IsNullOrWhiteSpace(commands[0]))
                throw new ArgumentException("Missing or repeated raw compiler source row.");
            string normalized = NormalizeCompilerCommand(commands[0],root,source,reviewed);
            if(!string.Equals(normalized,expected,StringComparison.Ordinal))
                throw new ArgumentException("Raw compiler options do not reproduce the recorded normalized profile.");
            string candidate=adoptionIncludes == null ? normalized :
                NeutralizeDependencyIncludes(normalized,adoptionIncludes[0],adoptionIncludes[1]);
            if(adopted != null && !string.Equals(adopted,candidate,StringComparison.Ordinal))
                throw new ArgumentException("Per-source compiler commands differ after exact dependency include normalization.");
            adopted=candidate;
        }
        return adopted;
    }
    public static string NeutralizeCompilerProfile(string command,string sourceInclude,string packageInclude) {
        return NeutralizeDependencyIncludes(command,sourceInclude,packageInclude);
    }
}
}
'@
}
$script:checks=0
function Require([bool]$Condition,[string]$Message) {
    ++$script:checks
    if(-not $Condition) { throw $Message }
}
function Near([double]$Actual,[double]$Expected,[string]$Message) {
    Require ([double]::IsFinite($Actual) -and [math]::Abs($Actual-$Expected) -le 1e-10*[math]::Max(1.0,[math]::Abs($Expected))) $Message
}
function Test-NonnegativeEvidenceNumber([object]$Value) {
    return ($Value -is [int] -or $Value -is [long] -or $Value -is [double] -or $Value -is [decimal] -or $Value -is [bigint]) -and
        [double]::IsFinite([double]$Value) -and [double]$Value -ge 0
}
Near ([RedTilde.PairedEvidenceReference]::Median(@(4.0,1.0,3.0,2.0))) 2.5 'Reference even median.'
$signs=[double[]]@(1,1,1,1,1,1,1,1,1,1,-1,-1)
Near ([RedTilde.PairedEvidenceReference]::Tail($signs)) (158.0/4096) 'Independent binomial-tail oracle.'
Near ([RedTilde.PairedEvidenceReference]::Tail([double[]]@(1,1,1,1,1,1,1,1,1,1,1,1))) (2.0/4096) 'Independent minimum tail probability.'
$p=[double[]]@((158.0/4096),1,1,1,1,1,1,1,1,1,1,1)
Near ([RedTilde.PairedEvidenceReference]::Holm($p)[0]) 0.462890625 'Fractional Holm oracle.'

function Assert-CompilerProfileEvidence {
    param([Collections.IDictionary]$Study,[string]$Root)
    # Legacy retained studies predate sourceRoot/TU/raw artifacts. New artifacts cannot downgrade themselves
    # by removing only the schema marker; historical metadata stays readable without inventing raw proof.
    $current=$Study.Contains('compilerReceipts')
    foreach($side in 'baseline','candidate') {
        $profile=$Study[$side].compiler
        foreach($key in 'schemaVersion','translationUnits','sourceRoot','rawFile') { if($profile.Contains($key)) {$current=$true} }
    }
    foreach($leaf in 'compiler-command.tlog','baseline-compiler-command.tlog','candidate-compiler-command.tlog') {
        if(Test-Path -LiteralPath (Join-Path $Root $leaf)) {$current=$true}
    }
    if(-not $current) {return}
    $adoption=$Study.Contains('dependencyComparisonMode') -and $Study.dependencyComparisonMode -ceq 'sdk-adoption'
    $reviewed=$Study.Contains('dependencyComparisonMode') -and $Study.dependencyComparisonMode -ceq 'reviewed-header-delta'
    $adoptionCommands=[ordered]@{}
    foreach($side in 'baseline','candidate') {
        $profile=$Study[$side].compiler
        Require ($profile.Contains('schemaVersion') -and ($profile.schemaVersion -is [int] -or $profile.schemaVersion -is [long]) -and
            $profile.schemaVersion -eq 2 -and @($profile.commands).Count -eq 1 -and
            @($profile.translationUnits).Count -gt 0 -and $profile.Contains('sourceRoot') -and
            [IO.Path]::IsPathFullyQualified([string]$profile.sourceRoot)) 'Compiler profile schema/inventory is incomplete or downgraded.'
        $command=[string]$profile.commands[0]
        Require (-not [string]::IsNullOrWhiteSpace($command)) 'Compiler option profile cannot be empty.'
        $profileBytes=[Text.Encoding]::UTF8.GetBytes("redtilde-release-compiler-profile-v2`n"+$command)
        $profileHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($profileBytes))
        Require ($profileHash -ceq $profile.sha256) 'Normalized compiler profile hash cannot be reproduced.'
        $units=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach($unit in $profile.translationUnits) {
            $segments=@([string]$unit.source -split '[\\/]')
            Require ($unit.source -and -not [IO.Path]::IsPathRooted([string]$unit.source) -and
                @($segments|Where-Object {$_ -cin @('','..','.')}).Count -eq 0 -and
                $units.Add(([string]$unit.source).Replace('\','/')) -and $unit.command -ceq $command) 'Translation units must be unique and share the exact compiler options.'
        }
        Require ($Study.Contains('compilerReceipts') -and $Study.compilerReceipts.Contains($side)) 'Raw compiler receipt is missing.'
        $raw=$Study.compilerReceipts[$side]
        Require ($raw.file -and [IO.Path]::GetFileName([string]$raw.file) -ceq $raw.file -and
            $raw.sha256 -ceq $profile.rawSha256 -and
            (Get-FileHash -LiteralPath (Join-Path $Root $raw.file)).Hash -ceq $raw.sha256) 'Raw compiler command bytes differ from their profile.'
        $adoptionIncludes=$null
        if($adoption -or $reviewed) {
            $dependency=$Study[$side].build.dependency
            $platform=[string]$dependency.Identity.platform
            $triplet=if($platform -ceq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
            $sourceInclude='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+[string]$dependency.Identity.commit+'\INCLUDE'
            $packageInclude='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+[string]$dependency.Fingerprint+'\VCPKG_INSTALLED\'+$platform+'\'+$triplet+'\INCLUDE'
            $adoptionIncludes=[string[]]@($sourceInclude,$packageInclude)
        }
        $adoptionCommands[$side]=[RedTilde.PairedEvidenceReference]::VerifyCompilerCommands(
            [string[]]@(Get-Content -LiteralPath (Join-Path $Root $raw.file)),[string]$profile.sourceRoot,
            [string[]]@($profile.translationUnits|ForEach-Object source),$command,$adoptionIncludes)
    }
    if($adoption -or $reviewed) { Require ($adoptionCommands.baseline -ceq $adoptionCommands.candidate) 'Raw compiler commands differ beyond the two declared SDK include paths.' }
}

function Assert-ReviewedHeaderProbeContext($Profile) {
    Require ($Profile.abiProbe.fullApplicationCompileContextExact -is [bool] -and
        $Profile.abiProbe.fullApplicationCompileContextExact -ceq $false -and
        $Profile.abiProbe.fullApplicationLinkContextExact -is [bool] -and
        $Profile.abiProbe.fullApplicationLinkContextExact -ceq $false) 'The isolated source probe must not be represented as full application compile/link context equivalence.'
}

function Assert-V4ProfileProbeEvidence($Profile) {
    Require ($Profile.abiProbe.protocol -ceq 'dxui-header-compat-probe-v4' -and
        $Profile.abiProbe.sourceSha256 -cmatch '^[0-9A-F]{64}$' -and
        $Profile.abiProbe.scope -match 'no.*binary interchangeability') 'V4 probe summary or its explicit scope limit is missing.'
    foreach($side in 'baseline','candidate') {
        $probe=$Profile[$side].probe
        try {$source=[Convert]::FromBase64String([string]$probe.sourceBase64);$result=[Convert]::FromBase64String([string]$probe.resultBase64)} catch {throw "V4 $side raw probe source/result is not valid base64."}
        Require ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($source)) -ceq $probe.sourceSha256 -and
            [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($result)) -ceq $probe.resultSha256) "V4 $side source/result proof hash does not recompute."
        $parsed=[Text.Encoding]::UTF8.GetString($result)|ConvertFrom-Json -AsHashtable
        Require ((ConvertTo-Json $parsed -Depth 30 -Compress) -ceq (ConvertTo-Json $probe.result -Depth 30 -Compress)) "V4 $side raw probe output differs from its parsed record."
        Require ($probe.protocol -ceq 'dxui-header-compat-probe-v4' -and
            ($probe.result.apiRevision -is [int] -or $probe.result.apiRevision -is [long]) -and $probe.result.apiRevision -eq 4 -and
            $probe.result.diagnosticsEnabled -is [bool] -and $probe.result.diagnosticsEnabled -and
            $probe.result.legacyCaptureLinked -is [bool] -and $probe.result.legacyCaptureLinked) "V4 $side must preserve diagnostics and link the legacy capture API."
        Require (($probe.result.markdownDefaultsAreDefault -is [bool]) -or ($side -ceq 'baseline' -and $null -eq $probe.result.markdownDefaultsAreDefault)) "V4 $side Markdown-default result has invalid applicability/type."
        Require ($probe.result.newCandidateSymbolsLinked -is [bool] -and $probe.result.newCandidateSymbolsLinked -eq ($side -ceq 'candidate')) "V4 $side candidate-only symbols are not correctly linked."
        foreach($type in 'ThemePalette','ProgressBar','RichTextEditorAppearance','LayoutOptions') {
            $layout=$probe.result.layout[$type]
            Require (($layout.size -is [int] -or $layout.size -is [long]) -and $layout.size -gt 0 -and
                ($layout.align -is [int] -or $layout.align -is [long]) -and $layout.align -gt 0) "V4 $side $type layout is invalid."
        }
    }
    $base=$Profile.baseline.probe.result;$candidate=$Profile.candidate.probe.result
    Require ($null -eq $base.markdownDefaultsAreDefault -and $candidate.markdownDefaultsAreDefault -is [bool] -and $candidate.markdownDefaultsAreDefault) 'V4 default-style evidence differs from applicability/expected candidate behavior.'
    Require (($base.legacyButtonAppearanceValues -join ',') -ceq '0,1,2,3' -and
        ($candidate.legacyButtonAppearanceValues -join ',') -ceq '0,1,2,3' -and
        ($candidate.newButtonAppearanceValues -join ',') -ceq '4,5' -and $null -eq $base.newButtonAppearanceValues) 'V4 enum values changed the legacy surface or omitted appended values.'
    Require ($base.layout.ProgressBar.size -eq $candidate.layout.ProgressBar.size) 'V4 ProgressBar size must remain unchanged.'
}
function Refresh-V4ProbeResult($Profile,[string]$Side) {
    $probe=$Profile[$Side].probe;$bytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json $probe.result -Depth 30 -Compress)+"`n")
    $probe.resultBase64=[Convert]::ToBase64String($bytes);$probe.resultSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))
}

function Get-ReviewedHeaderProfileEvidence {
    param([Collections.IDictionary]$Study,$Comparison,[string]$StudyRoot,$Inventories)
    function Get-ProfileRows($Headers,[string]$Side) {
        Require ($Headers.schemaVersion -eq 2 -and @($Headers.files).Count -eq 58) "$Side profile inventory must be schema 2 with 58 entries."
        $paths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $source=[Collections.Generic.List[string]]::new();$package=[Collections.Generic.List[string]]::new()
        foreach($row in $Headers.files) {
            $path=[string]$row.path;$segments=@($path -split '/')
            Require ($path -cmatch '^(include|package-include)/[A-Za-z0-9_.+/-]+$' -and
                @($segments|Where-Object {$_ -cin @('','.','..')}).Count -eq 0 -and
                [string]$row.sha256 -cmatch '^[0-9A-F]{64}$' -and $paths.Add($path)) "$Side profile contains a malformed or duplicate path/hash row."
            $record=$path+':'+[string]$row.sha256
            if($path.StartsWith('include/',[StringComparison]::Ordinal)){$source.Add($record)}else{$package.Add($record)}
        }
        if($source.Count -ne 25 -or $package.Count -ne 33) { throw "$Side profile inventory counts are not 25/33." }
        [string[]]$sourceRows=$source.ToArray();[string[]]$packageRows=$package.ToArray();[string[]]$allRows=[string[]]@($sourceRows+$packageRows)
        [Array]::Sort($sourceRows,[StringComparer]::Ordinal);[Array]::Sort($packageRows,[StringComparer]::Ordinal);[Array]::Sort($allRows,[StringComparer]::Ordinal)
        $hashRows={param([string[]]$Rows) [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($Rows -join "`n"))))}
        Require ((& $hashRows $sourceRows) -ceq $Headers.sourceSha256 -and (& $hashRows $packageRows) -ceq $Headers.packageSha256 -and
            (& $hashRows $allRows) -ceq $Headers.combinedSha256) "$Side profile inventory digests do not recompute."
        return [ordered]@{source=$sourceRows;package=$packageRows}
    }
    $repoRoot=Split-Path $PSScriptRoot -Parent
    $profileId=[string]$Study.reviewedHeaderDelta.profileId
    if($profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1') {
        $path=Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v1.json'
    } elseif($profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2') {
        $path=Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v2.json'
    } elseif($profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4') {
        $path=Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v4.json'
    } else { throw 'The independent auditor does not recognize this reviewed-header profile ID.' }
    Require (Test-Path -LiteralPath $path -PathType Leaf) 'The fixed reviewed-header profile evidence is missing.'
    $raw=[IO.File]::ReadAllBytes($path);$hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($raw))
    Require ($Study.reviewedHeaderDelta.file -ceq 'reviewed-header-profile.json' -and
        (Get-FileHash -LiteralPath (Join-Path $StudyRoot $Study.reviewedHeaderDelta.file)).Hash -ceq $hash -and
        [Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $StudyRoot $Study.reviewedHeaderDelta.file))) -ceq [Convert]::ToBase64String($raw)) 'Copied study profile bytes differ from the fixed repository profile.'
    $profile=[Text.Encoding]::UTF8.GetString($raw)|ConvertFrom-Json -AsHashtable
    $isV1=$profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1'
    $isV2=$profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2'
    $isV4=$profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4'
    if($isV1) {
        Require ($profile.schema -ceq 'redtilde-reviewed-dxui-header-delta-profile-v1' -and
            $profile.profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1' -and
            $profile.status -ceq 'ready-reviewed-exact-pair') 'The fixed v1 profile is blocked or has an unknown ID.'
        Require ($profile.baseline.commit -ceq '2fed82465dcee9f213753754e0e8e54f353cf38f' -and $profile.baseline.tree -ceq '2c0641bc363bf713cfc709d721c3f6549dd5aef8' -and $profile.candidate.commit -ceq '47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a' -and $profile.candidate.tree -ceq '3ee2e12647ca2e05113823eb172bb0facf879021' -and $profile.expectedApiRevision -eq 4) 'The v1 profile does not bind the exact SDK commit/tree/API pair.'
        $expectedPairPaths=[string[]]@('include/DxUi/DxUi.h','include/DxUi/RichDocument.h')
    } elseif($isV2) {
        Require ($profile.schema -ceq 'redtilde-reviewed-dxui-header-delta-profile-v2' -and
            $profile.profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2' -and
            $profile.status -ceq 'ready-reviewed-exact-pair') 'The fixed v2 profile is blocked or has an unknown ID.'
        Require ($profile.baseline.commit -ceq '47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a' -and $profile.baseline.tree -ceq '3ee2e12647ca2e05113823eb172bb0facf879021' -and $profile.candidate.commit -ceq '4077f57df8207eac6c39c43437feb45b19d23cf8' -and $profile.candidate.tree -ceq '18be66885916f67cafd8aeb761207bf15fbd9120' -and $profile.expectedApiRevision -eq 4) 'The v2 profile does not bind the exact SDK commit/tree/API pair.'
        $expectedPairPaths=[string[]]@('include/DxUi/DxUi.h','include/DxUi/MarkdownSyntax.h','include/DxUi/RichDocument.h')
    } else {
        Require ($profile.schema -ceq 'redtilde-reviewed-dxui-header-delta-profile-v4' -and $profile.status -ceq 'ready-reviewed-exact-pair' -and ($profile.expectedApiRevision -is [int] -or $profile.expectedApiRevision -is [long]) -and $profile.expectedApiRevision -eq 4) 'The fixed v4 profile is blocked or has an unknown ID/revision.'
        Require ($profile.baseline.commit -ceq '98c795b765a10d1805ae0a7c35a8ade535fb7fd6' -and $profile.baseline.tree -ceq '02694dfd718673d2d8e50241c015f908318154e5' -and $profile.candidate.commit -ceq '086ad82a5872768db23ea83862ab4af13dff0c5b' -and $profile.candidate.tree -ceq '0c674a6a8f17cd5e7f142d584f59d4ed12ddda99') 'The v4 profile does not bind the exact SDK commit/tree pair.'
        $expectedPairPaths=[string[]]@('include/DxUi/DxUi.h','include/DxUi/RichTextLayout.h','include/DxUi/RichTextPresentation.h')
    }
    Require ($profile.allOtherInventoryRowsMustBeIdentical -ceq $true -and $profile.packageHeadersUnchanged -ceq $true -and
        ($profile.dependencyFieldChanges -join '|') -ceq 'Fingerprint|Identity.commit' -and
        ($profile.compilerPathOperandsOnly -join '|') -ceq 'one exact DxUi source /I|one exact fingerprinted package /I') 'The fixed profile broadens the exact dependency/compiler allowlist.'
    Require ($Study.reviewedHeaderDelta.profileId -ceq $profile.profileId -and
        $Comparison.reviewedHeaderDelta.profileId -ceq $profile.profileId -and
        $Study.reviewedHeaderDelta.profileSha256 -ceq $hash -and
        $Comparison.reviewedHeaderDelta.profileSha256 -ceq $hash) 'Study/comparison do not bind the exact fixed profile bytes.'
    Require ((ConvertTo-Json $Study.reviewedHeaderDelta.abiProbe -Depth 20 -Compress) -ceq
        (ConvertTo-Json $profile.abiProbe -Depth 20 -Compress)) 'Study ABI probe evidence differs from the fixed profile.'
    $profilePairPaths=[string[]]@($profile.allowedHeaderHashPairs|ForEach-Object {[string]$_.sourcePath}|Sort-Object -CaseSensitive)
    Require ($profilePairPaths.Count -eq $expectedPairPaths.Count -and
        ($profilePairPaths -join "`n") -ceq (($expectedPairPaths|Sort-Object -CaseSensitive) -join "`n")) 'The fixed profile must enumerate exactly its reviewed SDK source header changes.'
    foreach($pair in $profile.allowedHeaderHashPairs) {
        Require ($pair.baselineSha256 -ceq $profile.baseline.publicHeaders.files.Where({$_.path -ceq $pair.sourcePath}).sha256 -and
            $pair.candidateSha256 -ceq $profile.candidate.publicHeaders.files.Where({$_.path -ceq $pair.sourcePath}).sha256) 'A profile pair differs from its complete inventory rows.'
    }
    foreach($side in 'baseline','candidate') {
        $dependency=$Study[$side].build.dependency;$expected=$profile[$side]
        Require ($dependency.Identity.commit -ceq $expected.commit -and $dependency.Fingerprint -ceq $expected.dependencyFingerprint -and
            $dependency.Identity.apiRevision -eq 4) "The $side SDK identity differs from the exact profile."
        if($isV4) {
            $actualKeys=[string[]]@($dependency.Identity.Keys);[Array]::Sort($actualKeys,[StringComparer]::Ordinal)
            $expectedKeys=[string[]]@($expected.identity.Keys);[Array]::Sort($expectedKeys,[StringComparer]::Ordinal)
            Require (($actualKeys -join '|') -ceq ($expectedKeys -join '|')) "$side V4 identity field set differs from the exact profile."
            foreach($field in $expectedKeys) {
                $actualValue=$dependency.Identity[$field];$expectedValue=$expected.identity[$field]
                Require ($null -ne $actualValue -and $null -ne $expectedValue -and
                    $actualValue.GetType() -eq $expectedValue.GetType() -and $actualValue -ceq $expectedValue) "$side V4 identity field type/value differs: $field."
            }
        } else {
            Require (($dependency.Identity|ConvertTo-Json -Depth 20 -Compress) -ceq
                ($expected.identity|ConvertTo-Json -Depth 20 -Compress)) "$side complete SDK toolchain identity differs from the exact profile."
        }
        $profileRows=Get-ProfileRows $expected.publicHeaders $side
        Require (($Inventories[$side].source -join "`n") -ceq ($profileRows.source -join "`n") -and
            ($Inventories[$side].package -join "`n") -ceq ($profileRows.package -join "`n")) "$side order-independent complete inventory differs from the fixed profile."
    }
    if($isV4) {
        Assert-V4ProfileProbeEvidence $profile
    } elseif($isV1) {
    Require ($profile.abiProbe.protocol -ceq 'dxui-readonly-source-snapshot-abi-probe-v1' -and
        $profile.abiProbe.sourceSha256 -cmatch '^[0-9A-F]{64}$' -and
        $profile.abiProbe.fullReceiptSha256 -cmatch '^[0-9A-F]{64}$' -and
        $profile.abiProbe.normalizedProbeCompileArgsEqual -ceq $true -and
        $profile.abiProbe.classLayoutEqual -ceq $true -and
        $profile.abiProbe.newMethodAvailableOnlyOnCandidate -ceq $true -and
        $profile.abiProbe.fullContextLimit) 'ABI probe source/full receipt, required results or scope limit are missing.'
    Assert-ReviewedHeaderProbeContext $profile
    try {$probeSource=[Convert]::FromBase64String([string]$profile.abiProbe.sourceBase64)} catch {throw 'ABI probe source base64 is invalid.'}
    Require ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($probeSource)) -ceq $profile.abiProbe.sourceSha256) 'Embedded ABI probe source hash differs.'
    foreach($side in 'baseline','candidate') {
        foreach($type in 'RichTextEditor','Document') {
            $layout=$profile.abiProbe[$side][$type]
            Require (($layout.sizeBytes -is [int] -or $layout.sizeBytes -is [long]) -and $layout.sizeBytes -gt 0 -and
                ($layout.alignmentBytes -is [int] -or $layout.alignmentBytes -is [long]) -and $layout.alignmentBytes -gt 0) "ABI probe has no valid $side $type layout."
            $layoutSizeField=if($type -ceq 'RichTextEditor'){'sizeofRichTextEditor'}else{'sizeofRichTextDocument'}
            $layoutAlignmentField=if($type -ceq 'RichTextEditor'){'alignofRichTextEditor'}else{'alignofRichTextDocument'}
            Require ($layout.sizeBytes -eq $profile.abiProbe[$side].values[$layoutSizeField] -and
                $layout.alignmentBytes -eq $profile.abiProbe[$side].values[$layoutAlignmentField]) "ABI $side $type labels differ from the compiler-probe values."
        }
        $sideEvidence=$profile.abiProbe[$side];$binding=$profile[$side]
        Require ($sideEvidence.receiptSha256 -cmatch '^[0-9A-F]{64}$' -and
            $sideEvidence.normalizedApplicationCompilerProfileSha256 -cmatch '^[0-9A-F]{64}$' -and
            $sideEvidence.probeApplicationCompilerTlogSha256 -cmatch '^[0-9A-F]{64}$' -and
            $sideEvidence.applicationLinkCommandSha256 -cmatch '^[0-9A-F]{64}$') "$side ABI side projection lacks compiler receipt hashes."
        $projection=[ordered]@{side=$side;sdkCommit=$binding.commit;fingerprint=$binding.dependencyFingerprint;apiRevision=4;
            headerInventorySha256=$binding.publicHeaders.combinedSha256;compilerSha256=$binding.identity.compilerSha256;
            linkerSha256=$binding.identity.linkerSha256;normalizedApplicationCompilerProfileSha256=$sideEvidence.normalizedApplicationCompilerProfileSha256;
            probeApplicationCompilerTlogSha256=$sideEvidence.probeApplicationCompilerTlogSha256;
            applicationLinkCommandSha256=$sideEvidence.applicationLinkCommandSha256;values=$sideEvidence.values}
        $projectionHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $projection -Depth 12 -Compress))) )
        Require ($projectionHash -ceq $sideEvidence.receiptSha256) "$side ABI side-projection checksum differs."
    }
    Require ($profile.abiProbe.baseline.RichTextEditor.sizeBytes -eq $profile.abiProbe.candidate.RichTextEditor.sizeBytes -and
        $profile.abiProbe.baseline.RichTextEditor.alignmentBytes -eq $profile.abiProbe.candidate.RichTextEditor.alignmentBytes -and
        $profile.abiProbe.baseline.Document.sizeBytes -eq $profile.abiProbe.candidate.Document.sizeBytes -and
        $profile.abiProbe.baseline.Document.alignmentBytes -eq $profile.abiProbe.candidate.Document.alignmentBytes -and
        $profile.abiProbe.baseline.values.hasReadOnlySourceSynchronization -ceq $false -and
        $profile.abiProbe.candidate.values.hasReadOnlySourceSynchronization -ceq $true -and
        $profile.abiProbe.candidate.newMethodSignatureVerified -ceq $true) 'ABI probe layout or candidate method signature is not verified.'
    foreach($field in @('sizeofSourceSnapshot','alignofSourceSnapshot','controlIsPolymorphic','richTextEditorIsPolymorphic','msvcVersion','msvcLang','iteratorDebugLevel')) {
        Require ($profile.abiProbe.baseline.values[$field] -ceq $profile.abiProbe.candidate.values[$field]) "ABI probe $field differs across the reviewed pair."
    }
    } else {
        $probe=$profile.abiProbe
        Require ($probe.protocol -ceq 'dxui-parsed-source-snapshot-source-probe-v2' -and
            $probe.sourceSha256 -cmatch '^[0-9A-F]{64}$' -and $probe.fullReceiptSha256 -cmatch '^[0-9A-F]{64}$' -and
            $probe.normalizedProbeCompileArgsEqual -ceq $true -and $probe.layoutDeltaVerified -is [bool] -and
            $probe.layoutDeltaVerified -ceq $true -and $probe.newMethodSignaturesVerified -is [bool] -and
            $probe.newMethodSignaturesVerified -ceq $true -and $probe.fullContextLimit -is [string] -and
            -not [string]::IsNullOrWhiteSpace($probe.fullContextLimit) -and
            $probe.fullContextLimit -match '(?i)isolated' -and $probe.fullContextLimit -match '(?i)\bPCH\b' -and
            $probe.fullContextLimit -match '(?i)\blibrary\b' -and $probe.fullContextLimit -match '(?i)\b(no|not|without)\b') 'Parsed-snapshot source probe source, receipts, measured rules or scope limit are missing.'
        foreach($side in 'baseline','candidate') {
            foreach($field in 'hasReadOnlySourceSynchronization','hasSourceSnapshotMatchesText','hasParsedSnapshotAdoption','hasReadOnlyMarkdownSyntaxAdoption') {
                Require ($probe[$side].values[$field] -is [bool]) "Parsed-snapshot $side method predicate $field must be a JSON boolean."
            }
        }
        Assert-ReviewedHeaderProbeContext $profile
        try {$probeSource=[Convert]::FromBase64String([string]$probe.sourceBase64)} catch {throw 'Parsed-snapshot probe source base64 is invalid.'}
        Require ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($probeSource)) -ceq $probe.sourceSha256) 'Embedded parsed-snapshot probe source hash differs.'
        foreach($side in 'baseline','candidate') {
            foreach($type in 'MarkdownSyntax','MarkdownParser','RichTextEditor','SourceSnapshot','Document') {
                $layout=$probe[$side][$type]
                Require (($layout.sizeBytes -is [int] -or $layout.sizeBytes -is [long]) -and $layout.sizeBytes -gt 0 -and
                    ($layout.alignmentBytes -is [int] -or $layout.alignmentBytes -is [long]) -and $layout.alignmentBytes -gt 0) "Parsed-snapshot probe has no valid $side $type layout."
                $fieldBase=if($type -ceq 'RichTextEditor'){'RichTextEditor'}elseif($type -ceq 'SourceSnapshot'){'SourceSnapshot'}elseif($type -ceq 'Document'){'RichTextDocument'}else{$type}
                Require ($layout.sizeBytes -eq $probe[$side].values["sizeof$fieldBase"] -and
                    $layout.alignmentBytes -eq $probe[$side].values["alignof$fieldBase"]) "Parsed-snapshot $side $type labels differ from compiler-probe values."
            }
            $sideEvidence=$probe[$side];$binding=$profile[$side]
            Require ($sideEvidence.receiptSha256 -cmatch '^[0-9A-F]{64}$' -and
                $sideEvidence.normalizedApplicationCompilerProfileSha256 -cmatch '^[0-9A-F]{64}$' -and
                $sideEvidence.probeApplicationCompilerTlogSha256 -cmatch '^[0-9A-F]{64}$' -and
                $sideEvidence.applicationLinkCommandSha256 -cmatch '^[0-9A-F]{64}$') "$side parsed-snapshot projection lacks compiler receipt hashes."
            $projection=[ordered]@{side=$side;sdkCommit=$binding.commit;fingerprint=$binding.dependencyFingerprint;apiRevision=4;
                headerInventorySha256=$binding.publicHeaders.combinedSha256;compilerSha256=$binding.identity.compilerSha256;
                linkerSha256=$binding.identity.linkerSha256;normalizedApplicationCompilerProfileSha256=$sideEvidence.normalizedApplicationCompilerProfileSha256;
                probeApplicationCompilerTlogSha256=$sideEvidence.probeApplicationCompilerTlogSha256;
                applicationLinkCommandSha256=$sideEvidence.applicationLinkCommandSha256;values=$sideEvidence.values}
            $projectionHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $projection -Depth 12 -Compress))))
            Require ($projectionHash -ceq $sideEvidence.receiptSha256) "$side parsed-snapshot side-projection checksum differs."
        }
        Require ($probe.candidate.MarkdownSyntax.sizeBytes -eq ($probe.baseline.MarkdownSyntax.sizeBytes + 16) -and
            $probe.candidate.MarkdownSyntax.alignmentBytes -eq $probe.baseline.MarkdownSyntax.alignmentBytes) 'MarkdownSyntax must grow exactly 16 bytes with unchanged alignment.'
        foreach($type in 'MarkdownParser','RichTextEditor','SourceSnapshot','Document') {
            Require ($probe.baseline[$type].sizeBytes -eq $probe.candidate[$type].sizeBytes -and
                $probe.baseline[$type].alignmentBytes -eq $probe.candidate[$type].alignmentBytes) "Parsed-snapshot probe detects a $type layout change."
        }
        Require ($probe.baseline.values.hasReadOnlySourceSynchronization -ceq $true -and
            $probe.candidate.values.hasReadOnlySourceSynchronization -ceq $true -and
            $probe.baseline.values.hasSourceSnapshotMatchesText -ceq $false -and
            $probe.candidate.values.hasSourceSnapshotMatchesText -ceq $true -and
            $probe.baseline.values.hasParsedSnapshotAdoption -ceq $false -and
            $probe.candidate.values.hasParsedSnapshotAdoption -ceq $true -and
            $probe.baseline.values.hasReadOnlyMarkdownSyntaxAdoption -ceq $false -and
            $probe.candidate.values.hasReadOnlyMarkdownSyntaxAdoption -ceq $true) 'Parsed-snapshot method availability flags differ from the exact API pair.'
        foreach($field in @('controlIsPolymorphic','richTextEditorIsPolymorphic','msvcVersion','msvcLang','iteratorDebugLevel')) {
            Require ($probe.baseline.values[$field] -ceq $probe.candidate.values[$field]) "Parsed-snapshot probe $field differs across the reviewed pair."
        }
    }
    return $profile
}

function Assert-CommonHarnessInputIdentity($Study,$Comparison) {
    $legacyKeys=[string[]]@('Tests/Application.Tests.Performance.cpp','Tests/Application.Tests.HeapDiagnostic.h',
        'RedTilde/Main.cpp','RedTilde/EditDiagnostics.h','performance-paired.ps1','Build/Performance.psm1',
        'Build/Harness.psm1','docs/examples/s5-performance.md')
    $historicalSevenKeys=[string[]]@('Tests/Scope.Tests.Performance.cpp','RedTilde/Main.cpp','RedTilde/EditDiagnostics.h',
        'performance-paired.ps1','Build/Performance.psm1','Build/Harness.psm1','docs/examples/s5-performance.md')
    $profileKey='Build/ReviewedHeaderDeltaProfile.json'
    $currentKeys=[string[]]@($legacyKeys+$profileKey)
    $mode=''
    if($Study.Contains('dependencyComparisonMode')) {$mode=[string]$Study.dependencyComparisonMode}
    $legacySchemaLessReceipt=(-not $Study.Contains('dependencyComparisonMode') -and
        -not $Study.Contains('reviewedHeaderDelta') -and -not $Study.Contains('compilerReceipts'))
    if($legacySchemaLessReceipt) {
        $expectedCompilerKeys=[string[]]@('sha256','commands','rawSha256')
        foreach($side in @('baseline','candidate')) {
            $compiler=$Study[$side].compiler
            if($null -eq $compiler -or $compiler.Contains('schemaVersion') -or $compiler.Contains('translationUnits') -or
                $compiler.Contains('sourceRoot') -or $compiler.Contains('rawFile') -or
                @($compiler.Keys|Sort-Object -CaseSensitive).Count -ne $expectedCompilerKeys.Count -or
                @(Compare-Object ([string[]]@($compiler.Keys|Sort-Object -CaseSensitive)) ([string[]]@($expectedCompilerKeys|Sort-Object -CaseSensitive)) -CaseSensitive).Count -ne 0 -or
                $compiler.sha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or $compiler.rawSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
                @($compiler.commands).Count -eq 0 -or @($compiler.commands|Where-Object {$_ -isnot [string] -or [string]::IsNullOrWhiteSpace($_)}).Count -gt 0) {
                $legacySchemaLessReceipt=$false
                break
            }
        }
    }
    $maps=@($Study.baseline.inputs,$Study.candidate.inputs,$Comparison.harnessInputs)
    $labels=@('baseline','candidate','comparison')
    $keySets=[Collections.Generic.List[string[]]]::new()
    for($mapIndex=0;$mapIndex -lt $maps.Count;++$mapIndex) {
        $map=$maps[$mapIndex]
        Require ($map -is [Collections.IDictionary]) "$($labels[$mapIndex]) common harness inputs must be a key/value map."
        $keys=[string[]]@($map.Keys|ForEach-Object {[string]$_})
        $caseFold=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach($key in $keys) {
            Require ($caseFold.Add($key) -and $map[$key] -is [string] -and $map[$key] -cmatch '^[0-9A-Fa-f]{64}$') "$($labels[$mapIndex]) common input key/hash is malformed or duplicated."
        }
        [Array]::Sort($keys,[StringComparer]::Ordinal)
        $isLegacy=$keys.Count -eq $legacyKeys.Count -and @(Compare-Object $keys ([string[]]@($legacyKeys|Sort-Object -CaseSensitive)) -CaseSensitive).Count -eq 0
        $isCurrent=$keys.Count -eq $currentKeys.Count -and @(Compare-Object $keys ([string[]]@($currentKeys|Sort-Object -CaseSensitive)) -CaseSensitive).Count -eq 0
        $isHistoricalSeven=$legacySchemaLessReceipt -and $keys.Count -eq $historicalSevenKeys.Count -and
            @(Compare-Object $keys ([string[]]@($historicalSevenKeys|Sort-Object -CaseSensitive)) -CaseSensitive).Count -eq 0
        Require ($isLegacy -or $isCurrent -or $isHistoricalSeven) "$($labels[$mapIndex]) inputs must be an exact declared 8/9-key current set or the exact 7-key unmarked schema-less legacy set."
        $keySets.Add($keys)
    }
    for($mapIndex=1;$mapIndex -lt $keySets.Count;++$mapIndex) {
        Require ((@($keySets[0])-join "`n") -ceq (@($keySets[$mapIndex])-join "`n")) 'Baseline, candidate and comparison common-input key sets/cardinality differ.'
    }
    foreach($key in $keySets[0]) {
        Require ($Study.baseline.inputs[$key] -ceq $Study.candidate.inputs[$key] -and
            $Study.baseline.inputs[$key] -ceq $Comparison.harnessInputs[$key]) "Common harness input digest differs: $key"
    }
    if($mode -ceq 'reviewed-header-delta') {
        Require ($keySets[0].Count -eq 9 -and $keySets[0] -ccontains $profileKey -and
            $Study.reviewedHeaderDelta.profileSha256 -cmatch '^[0-9A-Fa-f]{64}$' -and
            $Study.baseline.inputs[$profileKey] -ceq $Study.reviewedHeaderDelta.profileSha256) 'Reviewed-header mode requires the ninth fixed-profile common input bound to its profile digest.'
    }
}

function Assert-DependencyEvidence($Study,$Comparison,[string]$StudyRoot) {
    $base=$Study.baseline.build.dependency; $candidate=$Study.candidate.build.dependency
    if(-not $Study.Contains('dependencyComparisonMode')) {
        Require ($base.Fingerprint -ceq $candidate.Fingerprint) 'Compiler/dependency identity differs.'
        return
    }
    $mode=$Study.dependencyComparisonMode
    Require ($mode -cin @('same-pin','sdk-adoption','reviewed-header-delta') -and $Comparison.dependencyComparisonMode -ceq $mode) 'Invalid dependency comparison mode.'
    foreach($dependency in @($base,$candidate)) {
        Require ($dependency.Fingerprint -cmatch '^[0-9a-fA-F]{64}$' -and $dependency.Identity.commit -cmatch '^[0-9a-f]{40}$' -and
            ($dependency.Identity.apiRevision -is [int] -or $dependency.Identity.apiRevision -is [long]) -and $dependency.Identity.apiRevision -eq 4) 'Invalid dependency build metadata.'
    }
    Require ($base.Identity.platform -ceq $Study.baseline.build.platform -and $candidate.Identity.platform -ceq $Study.candidate.build.platform -and
        $Study.baseline.build.platform -ceq 'x64' -and $Study.candidate.build.platform -ceq 'x64') 'Dependency platform must match the measured x64 build.'
    $inventories=[ordered]@{}
    foreach($side in @('baseline','candidate')) {
        Require ($Study[$side].Contains('publicHeaders')) 'Declared dependency header inventory is missing.'
        $headers=$Study[$side].publicHeaders
        Require (($headers.schemaVersion -is [int] -or $headers.schemaVersion -is [long]) -and
            $headers.schemaVersion -cin @(1,2) -and $headers.files.Count -gt 0 -and $headers.sha256 -cmatch '^[0-9A-F]{64}$') 'Invalid public-header evidence.'
        if($mode -cin @('sdk-adoption','reviewed-header-delta')) { Require ($headers.schemaVersion -eq 2) 'SDK adoption requires source and package header inventories.' }
        $paths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $sourceRows=[Collections.Generic.List[string]]::new();$packageRows=[Collections.Generic.List[string]]::new();$allRows=[Collections.Generic.List[string]]::new()
        foreach($file in $headers.files) {
            $validPath=if($headers.schemaVersion -eq 1) { $file.path -cmatch '^include/[^:\\]+$' } else { $file.path -cmatch '^(?:include|package-include)/[^:\\]+$' }
            Require ($validPath -and $file.path -notmatch '(^|/)\.\.?(/|$)' -and
                $paths.Add($file.path) -and $file.sha256 -cmatch '^[0-9A-F]{64}$') 'Invalid public-header inventory row.'
            $row=$file.path+':'+$file.sha256;$allRows.Add($row)
            if($file.path.StartsWith('include/',[StringComparison]::Ordinal)) {$sourceRows.Add($row)} else {$packageRows.Add($row)}
        }
        [string[]]$sourceArray=$sourceRows.ToArray();[string[]]$packageArray=$packageRows.ToArray();[string[]]$allArray=$allRows.ToArray()
        [Array]::Sort($sourceArray,[StringComparer]::Ordinal);[Array]::Sort($packageArray,[StringComparer]::Ordinal);[Array]::Sort($allArray,[StringComparer]::Ordinal)
        $hashRows={param([string[]]$Rows) [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($Rows -join "`n"))))}
        Require ((& $hashRows $allArray) -ceq $headers.sha256) 'Public-header inventory checksum differs.'
        if($headers.schemaVersion -eq 2) {
            Require ($sourceArray.Count -gt 0 -and $packageArray.Count -gt 0 -and
                $headers.sourceSha256 -cmatch '^[0-9A-F]{64}$' -and $headers.packageSha256 -cmatch '^[0-9A-F]{64}$' -and
                ((& $hashRows $sourceArray) -ceq $headers.sourceSha256) -and ((& $hashRows $packageArray) -ceq $headers.packageSha256)) 'Source or package header inventory checksum differs.'
        } else { Require ($sourceArray.Count -gt 0 -and $packageArray.Count -eq 0) 'Schema-one header inventories cover source headers only.' }
        $sourceHash=if($headers.Contains('sourceSha256')) {[string]$headers.sourceSha256} else {''}
        $packageHash=if($headers.Contains('packageSha256')) {[string]$headers.packageSha256} else {''}
        $inventories[$side]=[ordered]@{schemaVersion=$headers.schemaVersion;source=$sourceArray;package=$packageArray;sourceSha256=$sourceHash;packageSha256=$packageHash}
    }
    if($mode -ceq 'same-pin') {
        Require (($base|ConvertTo-Json -Depth 100 -Compress) -ceq ($candidate|ConvertTo-Json -Depth 100 -Compress)) 'Same-pin dependency metadata differs.'
        Require (($inventories.baseline.source -join "`n") -ceq ($inventories.candidate.source -join "`n") -and
            ($inventories.baseline.package -join "`n") -ceq ($inventories.candidate.package -join "`n")) 'Same-pin public header bytes differ.'
        return
    }
    if($mode -ceq 'reviewed-header-delta') {
        $profile=Get-ReviewedHeaderProfileEvidence $Study $Comparison $StudyRoot $inventories
        $allowed=if($Study.reviewedHeaderDelta.profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1') {
            @('include/DxUi/DxUi.h','include/DxUi/RichDocument.h')
        } elseif($Study.reviewedHeaderDelta.profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2') {
            @('include/DxUi/DxUi.h','include/DxUi/MarkdownSyntax.h','include/DxUi/RichDocument.h')
        } elseif($Study.reviewedHeaderDelta.profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4') {
            @('include/DxUi/DxUi.h','include/DxUi/RichTextLayout.h','include/DxUi/RichTextPresentation.h')
        } else { throw 'Unknown reviewed-header dependency profile.' }
        $baselineHeaderRows=@{};foreach($row in $inventories.baseline.source+$inventories.baseline.package){$parts=$row.Split(':',2);$baselineHeaderRows[$parts[0]]=$parts[1]}
        $candidateHeaderRows=@{};foreach($row in $inventories.candidate.source+$inventories.candidate.package){$parts=$row.Split(':',2);$candidateHeaderRows[$parts[0]]=$parts[1]}
        Require ((@($baselineHeaderRows.Keys|Sort-Object -CaseSensitive)-join "`n") -ceq (@($candidateHeaderRows.Keys|Sort-Object -CaseSensitive)-join "`n")) 'Reviewed-header path sets differ.'
        $changed=@(foreach($key in $baselineHeaderRows.Keys){if($baselineHeaderRows[$key] -cne $candidateHeaderRows[$key]){$key}})
        Require ($changed.Count -eq $allowed.Count -and @($changed|Where-Object {$allowed -cnotcontains $_}).Count -eq 0 -and
            $inventories.baseline.source.Count -eq 25 -and $inventories.candidate.source.Count -eq 25 -and
            $inventories.baseline.package.Count -eq 33 -and $inventories.candidate.package.Count -eq 33) 'The reviewed profile must contain only its exact source-header delta and 33 unchanged package rows.'
        foreach($pair in $profile.allowedHeaderHashPairs) {
            Require ($pair.sourcePath -cin $allowed -and $pair.baselineSha256 -ceq $baselineHeaderRows[$pair.sourcePath] -and
                $pair.candidateSha256 -ceq $candidateHeaderRows[$pair.sourcePath]) 'A reviewed source-header hash pair differs from its full inventory.'
        }
        if($Study.reviewedHeaderDelta.profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4') {
            $expectedOuter=[string[]]@('Fingerprint','Identity')
            $expectedIdentity=[string[]]@('apiRevision','platform','toolset','vcToolsVersion','windowsSdkVersion','runtimeFamily',
                'preferredToolArchitecture','disableStlAnnotations','releaseOptimizationPolicy','msbuildSha256','compilerSha256',
                'linkerSha256','windowsHeaderSha256','kernel32Sha256','ucrtSha256','commit')
            foreach($entry in @(@{name='baseline';value=$base},@{name='candidate';value=$candidate})) {
                $outerKeys=[string[]]@($entry.value.Keys);[Array]::Sort($outerKeys,[StringComparer]::Ordinal)
                $wantedOuter=[string[]]@($expectedOuter);[Array]::Sort($wantedOuter,[StringComparer]::Ordinal)
                Require (($outerKeys -join '|') -ceq ($wantedOuter -join '|')) "$($entry.name) V4 dependency has missing/extra/case-changed outer keys."
                $identityKeys=[string[]]@($entry.value.Identity.Keys);[Array]::Sort($identityKeys,[StringComparer]::Ordinal)
                $wantedIdentity=[string[]]@($expectedIdentity);[Array]::Sort($wantedIdentity,[StringComparer]::Ordinal)
                Require (($identityKeys -join '|') -ceq ($wantedIdentity -join '|')) "$($entry.name) V4 identity has missing/extra/case-changed keys."
            }
            foreach($field in $expectedIdentity) {
                if($field -ceq 'commit'){continue}
                $left=$base.Identity[$field];$right=$candidate.Identity[$field]
                Require ($null -ne $left -and $null -ne $right -and
                    $left.GetType() -eq $right.GetType() -and $left -ceq $right) "V4 dependency identity changed type/value for $field."
            }
        } else {
            $baselineClone=ConvertFrom-Json -InputObject ($base|ConvertTo-Json -Depth 100 -Compress) -AsHashtable
            $candidateClone=ConvertFrom-Json -InputObject ($candidate|ConvertTo-Json -Depth 100 -Compress) -AsHashtable
            $baselineClone.Remove('Fingerprint');$candidateClone.Remove('Fingerprint')
            $baselineClone.Identity.Remove('commit');$candidateClone.Identity.Remove('commit')
            Require (($baselineClone|ConvertTo-Json -Depth 100 -Compress) -ceq ($candidateClone|ConvertTo-Json -Depth 100 -Compress)) 'Reviewed profile permits only its exact dependency commit/fingerprint pair.'
        }
        return
    }
    Require ($Study.baseline.publicHeaders.sourceSha256 -ceq $Study.candidate.publicHeaders.sourceSha256 -and
        $Study.baseline.publicHeaders.packageSha256 -ceq $Study.candidate.publicHeaders.packageSha256 -and
        ($inventories.baseline.source -join "`n") -ceq ($inventories.candidate.source -join "`n") -and
        ($inventories.baseline.package -join "`n") -ceq ($inventories.candidate.package -join "`n") -and
        $base.Identity.apiRevision -eq 4 -and $candidate.Identity.apiRevision -eq 4) 'Adoption public-header/API identity differs.'
    Require ($base.Fingerprint -cmatch '^[0-9a-fA-F]{64}$' -and $candidate.Fingerprint -cmatch '^[0-9a-fA-F]{64}$' -and
        $base.Identity.commit -cmatch '^[0-9a-f]{40}$' -and $candidate.Identity.commit -cmatch '^[0-9a-f]{40}$') 'Invalid adoption dependency metadata.'
    Require ((@($base.Keys|Sort-Object)|ConvertTo-Json -Compress) -ceq (@($candidate.Keys|Sort-Object)|ConvertTo-Json -Compress) -and
        (@($base.Identity.Keys|Sort-Object)|ConvertTo-Json -Compress) -ceq (@($candidate.Identity.Keys|Sort-Object)|ConvertTo-Json -Compress)) 'Adoption dependency field sets differ.'
    foreach($key in $base.Keys) {
        if($key -ceq 'Fingerprint'){continue}
        if($key -ceq 'Identity') {
            foreach($field in $base.Identity.Keys) {
                if($field -ceq 'commit'){continue}
                Require (($base.Identity[$field]|ConvertTo-Json -Depth 100 -Compress) -ceq
                    ($candidate.Identity[$field]|ConvertTo-Json -Depth 100 -Compress)) 'Adoption changed dependency build or ABI fields.'
            }
        } else {
            Require (($base[$key]|ConvertTo-Json -Depth 100 -Compress) -ceq ($candidate[$key]|ConvertTo-Json -Depth 100 -Compress)) 'Adoption changed outer dependency fields.'
        }
    }
    Require (($base.Identity.commit -ceq $candidate.Identity.commit) -eq ($base.Fingerprint -ceq $candidate.Fingerprint)) 'Adoption commit/fingerprint changes must agree.'
}
if($SelfTest) {
    $repoRoot=Split-Path $PSScriptRoot -Parent
    $profilePath=Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v2.json'
    $profileBytes=[IO.File]::ReadAllBytes($profilePath)
    $profileHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($profileBytes))
    $profile=[Text.Encoding]::UTF8.GetString($profileBytes)|ConvertFrom-Json -AsHashtable
    $profileV2=$profile;$profileV2Bytes=$profileBytes;$profileV2Hash=$profileHash
    $v4Path=Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v4.json'
    $v4Bytes=[IO.File]::ReadAllBytes($v4Path)
    Require ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($v4Bytes)) -ceq '94435355531812AE70BE124E185A5D685C16A94C8052A6C1E4BAC4DD5D381997') 'The final LF-normalized V4 fixture must remain byte exact.'
    $v4=[Text.Encoding]::UTF8.GetString($v4Bytes)|ConvertFrom-Json -AsHashtable
    Assert-V4ProfileProbeEvidence $v4
    foreach($case in @(
        @{name='rehash malformed diagnostics boolean';edit={param($p)$p.candidate.probe.result.diagnosticsEnabled=1;Refresh-V4ProbeResult $p candidate}},
        @{name='rehash malformed enum values';edit={param($p)$p.candidate.probe.result.newButtonAppearanceValues=@(4,6);Refresh-V4ProbeResult $p candidate}},
        @{name='rehash malformed layout';edit={param($p)$p.candidate.probe.result.layout.LayoutOptions.size='256';Refresh-V4ProbeResult $p candidate}},
        @{name='raw proof hash mismatch';edit={param($p)$p.baseline.probe.resultSha256='0'*64}})) {
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $v4 -Depth 100 -Compress) -AsHashtable
        & $case.edit $bad
        $rejected=$false;try {Assert-V4ProfileProbeEvidence $bad} catch {$rejected=$true}
        Require $rejected "V4 actual-profile selftest rejects $($case.name)."
    }
    Require ($profile.status -ceq 'ready-reviewed-exact-pair') 'Private new-mode selftest requires the approved fixed profile data.'
    $fixtureRoot=Join-Path $repoRoot ('.build/independent-header-profile-selftest/'+[guid]::NewGuid().ToString('N'))
    [void][IO.Directory]::CreateDirectory($fixtureRoot)
    $profileCopy=Join-Path $fixtureRoot 'reviewed-header-profile.json'
    [IO.File]::WriteAllBytes($profileCopy,$profileBytes)
    Assert-ReviewedHeaderProbeContext $profile
    foreach($field in @('fullApplicationCompileContextExact','fullApplicationLinkContextExact')) {
        foreach($invalidValue in @($true,0,'False')) {
            $badContext=ConvertFrom-Json -InputObject (ConvertTo-Json $profile -Depth 40 -Compress) -AsHashtable
            $badContext.abiProbe[$field]=$invalidValue
            $rejected=$false;try {Assert-ReviewedHeaderProbeContext $badContext}catch{$rejected=$true}
            Require $rejected "Independent source-probe context guard rejects non-false/$($invalidValue.GetType().Name) value for $field."
        }
    }
    $makeStudy={
        $study=[ordered]@{dependencyComparisonMode='reviewed-header-delta'}
        $commonInputs=[ordered]@{}
        foreach($key in @('Tests/Application.Tests.Performance.cpp','Tests/Application.Tests.HeapDiagnostic.h',
            'RedTilde/Main.cpp','RedTilde/EditDiagnostics.h','performance-paired.ps1','Build/Performance.psm1',
            'Build/Harness.psm1','docs/examples/s5-performance.md')) {$commonInputs[$key]='A'*64}
        $commonInputs['Build/ReviewedHeaderDeltaProfile.json']=$profileHash
        foreach($side in 'baseline','candidate') {
            $binding=$profile[$side]
            $identity=ConvertFrom-Json -InputObject (ConvertTo-Json $binding.identity -Depth 30 -Compress) -AsHashtable
            $dependency=[ordered]@{Fingerprint=$binding.dependencyFingerprint;Identity=$identity}
            $headers=ConvertFrom-Json -InputObject (ConvertTo-Json $binding.publicHeaders -Depth 30 -Compress) -AsHashtable
            $headers['sha256']=$headers.combinedSha256
            $inputs=ConvertFrom-Json -InputObject (ConvertTo-Json $commonInputs -Depth 10 -Compress) -AsHashtable
            $study[$side]=[ordered]@{build=[ordered]@{platform='x64';dependency=$dependency};publicHeaders=$headers;inputs=$inputs}
        }
        $study.reviewedHeaderDelta=[ordered]@{file='reviewed-header-profile.json';profileId=$profile.profileId;profileSha256=$profileHash;
            abiProbe=ConvertFrom-Json -InputObject (ConvertTo-Json $profile.abiProbe -Depth 30 -Compress) -AsHashtable}
        $comparisonInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $commonInputs -Depth 10 -Compress) -AsHashtable
        $comparison=[ordered]@{dependencyComparisonMode='reviewed-header-delta';reviewedHeaderDelta=[ordered]@{profileId=$profile.profileId;profileSha256=$profileHash};harnessInputs=$comparisonInputs}
        return @($study,$comparison)
    }
    function Assert-HeaderProfileRejected($Study,$Comparison,[string]$Label) {
        $rejected=$false
        try {Assert-DependencyEvidence $Study $Comparison $fixtureRoot} catch {$rejected=$true}
        Require $rejected "Independent auditor accepted the invalid reviewed-profile fixture: $Label."
    }
    function Assert-CommonInputsRejected($Study,$Comparison,[string]$Label) {
        $rejected=$false
        try {Assert-CommonHarnessInputIdentity $Study $Comparison} catch {$rejected=$true}
        Require $rejected "Independent auditor accepted the invalid common-input fixture: $Label."
    }
    $fixture=& $makeStudy;$study=$fixture[0];$comparison=$fixture[1]
    Assert-CommonHarnessInputIdentity $study $comparison
    Assert-DependencyEvidence $study $comparison $fixtureRoot
    Require $true 'Independent auditor accepts the exact reviewed profile and complete two-sided inventories.'
    $defaultNine=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $defaultComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $comparison -Depth 40 -Compress) -AsHashtable
    $defaultNine.dependencyComparisonMode='same-pin';$defaultComparison.dependencyComparisonMode='same-pin'
    $defaultNine.reviewedHeaderDelta=$null
    Assert-CommonHarnessInputIdentity $defaultNine $defaultComparison
    Require $true 'Independent input auditor accepts the current nine-file common set on a historical comparison mode.'
    $historicalEight=ConvertFrom-Json -InputObject (ConvertTo-Json $defaultNine -Depth 40 -Compress) -AsHashtable
    $historicalComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $defaultComparison -Depth 40 -Compress) -AsHashtable
    foreach($side in 'baseline','candidate') {$historicalEight[$side].inputs.Remove('Build/ReviewedHeaderDeltaProfile.json')}
    $historicalComparison.harnessInputs.Remove('Build/ReviewedHeaderDeltaProfile.json')
    Assert-CommonHarnessInputIdentity $historicalEight $historicalComparison
    Require $true 'Independent input auditor preserves historical eight-file common sets.'
    $historicalSevenInputs=[ordered]@{}
    foreach($key in @('Tests/Scope.Tests.Performance.cpp','RedTilde/Main.cpp','RedTilde/EditDiagnostics.h',
        'performance-paired.ps1','Build/Performance.psm1','Build/Harness.psm1','docs/examples/s5-performance.md')) {$historicalSevenInputs[$key]='C'*64}
    $legacyCompiler=[ordered]@{sha256='D'*64;commands=@('/c /O2');rawSha256='E'*64}
    $historicalSeven=[ordered]@{baseline=[ordered]@{compiler=$legacyCompiler;inputs=(ConvertFrom-Json -InputObject (ConvertTo-Json $historicalSevenInputs -Compress) -AsHashtable)};
        candidate=[ordered]@{compiler=(ConvertFrom-Json -InputObject (ConvertTo-Json $legacyCompiler -Compress) -AsHashtable);inputs=(ConvertFrom-Json -InputObject (ConvertTo-Json $historicalSevenInputs -Compress) -AsHashtable)}}
    $historicalSevenComparison=[ordered]@{harnessInputs=(ConvertFrom-Json -InputObject (ConvertTo-Json $historicalSevenInputs -Compress) -AsHashtable)}
    Assert-CommonHarnessInputIdentity $historicalSeven $historicalSevenComparison
    Require $true 'Independent input auditor accepts exactly seven keys only for unmarked schema-less legacy compiler metadata.'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $badInputs.candidate.inputs.Remove('Build/ReviewedHeaderDeltaProfile.json')
    Assert-CommonInputsRejected $badInputs $comparison 'candidate common-input key missing'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $badComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $comparison -Depth 40 -Compress) -AsHashtable
    $badComparison.harnessInputs['Unreviewed.extra']='B'*64
    Assert-CommonInputsRejected $study $badComparison 'extra comparison common input'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $badInputs.baseline.inputs['Build/ReviewedHeaderDeltaProfile.json']='0'*64
    $badInputs.candidate.inputs['Build/ReviewedHeaderDeltaProfile.json']='0'*64
    $badComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $comparison -Depth 40 -Compress) -AsHashtable
    $badComparison.harnessInputs['Build/ReviewedHeaderDeltaProfile.json']='0'*64
    Assert-CommonInputsRejected $badInputs $badComparison 'ninth profile common-input hash differs from profile digest'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $badComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $comparison -Depth 40 -Compress) -AsHashtable
    foreach($side in 'baseline','candidate') {$badInputs[$side].inputs.Remove('Build/ReviewedHeaderDeltaProfile.json')}
    $badComparison.harnessInputs.Remove('Build/ReviewedHeaderDeltaProfile.json')
    Assert-CommonInputsRejected $badInputs $badComparison 'reviewed mode cannot downgrade to seven/eight input shape'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $badComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $comparison -Depth 40 -Compress) -AsHashtable
    foreach($side in 'baseline','candidate') {$badInputs[$side].inputs=$historicalSevenInputs}
    $badComparison.harnessInputs=$historicalSevenInputs
    Assert-CommonInputsRejected $badInputs $badComparison 'reviewed mode cannot downgrade to the exact historical seven-key set'
    $badInputs=ConvertFrom-Json -InputObject (ConvertTo-Json $historicalSeven -Depth 20 -Compress) -AsHashtable
    $badComparison=ConvertFrom-Json -InputObject (ConvertTo-Json $historicalSevenComparison -Depth 20 -Compress) -AsHashtable
    $badInputs.reviewedHeaderDelta=[ordered]@{profileId=$profile.profileId;profileSha256=$profileHash}
    Assert-CommonInputsRejected $badInputs $badComparison 'profile-marked study cannot use legacy seven-key exception'
    $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $bad.candidate.build.dependency.Identity.commit='0000000000000000000000000000000000000000'
    Assert-HeaderProfileRejected $bad $comparison 'candidate pin drift'
    $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $bad.candidate.publicHeaders.files[0].sha256='A'*64
    Assert-HeaderProfileRejected $bad $comparison 'header inventory hash drift'
    $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    $bad.reviewedHeaderDelta.abiProbe.baseline.RichTextEditor.sizeBytes=$null
    Assert-HeaderProfileRejected $bad $comparison 'missing ABI layout evidence'
    $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
    if($profile.profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1') {
        $bad.reviewedHeaderDelta.abiProbe.candidate.newMethodSignatureVerified=$false
    } else {
        $bad.reviewedHeaderDelta.abiProbe.newMethodSignaturesVerified=$false
    }
    Assert-HeaderProfileRejected $bad $comparison 'unverified candidate method signature'
    if($profile.profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2') {
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
        $bad.reviewedHeaderDelta.abiProbe.candidate.MarkdownSyntax.sizeBytes++
        Assert-HeaderProfileRejected $bad $comparison 'MarkdownSyntax growth differs from the exact 16-byte allowance'
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
        $bad.reviewedHeaderDelta.abiProbe.candidate.values.hasSourceSnapshotMatchesText=$false
        Assert-HeaderProfileRejected $bad $comparison 'SourceSnapshot::MatchesText signature flag is missing'
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $study -Depth 40 -Compress) -AsHashtable
        $bad.reviewedHeaderDelta.abiProbe.fullContextLimit=$true
        Assert-HeaderProfileRejected $bad $comparison 'probe scope limit is not descriptive text'
    }
    [IO.File]::WriteAllBytes($profileCopy,[byte[]]@($profileBytes+@(0x20)))
    Assert-HeaderProfileRejected $study $comparison 'copied profile bytes differ from the fixed profile'
    [IO.File]::WriteAllBytes($profileCopy,$profileBytes)
    $profile=$v4;$profileBytes=$v4Bytes;$profileHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($profileBytes))
    [IO.File]::WriteAllBytes($profileCopy,$profileBytes)
    $v4Fixture=& $makeStudy;$v4Study=$v4Fixture[0];$v4Comparison=$v4Fixture[1]
    Assert-CommonHarnessInputIdentity $v4Study $v4Comparison
    Assert-DependencyEvidence $v4Study $v4Comparison $fixtureRoot
    Require $true 'Full reviewed-header dependency auditor accepts the exact final V4 fixture and three-header delta.'
    # Keep the portable self-test independent of private retained-study locations.
    # The full retained four-study audit is run separately with explicit arguments.
    $fullV4Study=ConvertFrom-Json -InputObject (ConvertTo-Json $v4Study -Depth 100 -Compress) -AsHashtable
    $fullV4Comparison=ConvertFrom-Json -InputObject (ConvertTo-Json $v4Comparison -Depth 100 -Compress) -AsHashtable
    Assert-DependencyEvidence $fullV4Study $fullV4Comparison $fixtureRoot
    Require $true 'Full reviewed-header dependency auditor accepts a portable exact V4 fixture.'
    $reorderedV4Study=ConvertFrom-Json -InputObject (ConvertTo-Json $fullV4Study -Depth 100 -Compress) -AsHashtable
    foreach($side in 'baseline','candidate') {
        $identity=$reorderedV4Study[$side].build.dependency.Identity
        $pairs=@($identity.GetEnumerator());[Array]::Reverse($pairs);$identity.Clear()
        foreach($pair in $pairs) {$identity[$pair.Key]=$pair.Value}
    }
    Assert-DependencyEvidence $reorderedV4Study $fullV4Comparison $fixtureRoot
    Require $true 'Full V4 auditor compares SDK identity fields semantically when JSON key order differs.'
    $refreshInventory={
        param($headers)
        $source=[string[]]@($headers.files|Where-Object {$_.path -clike 'include/*'}|ForEach-Object {$_.path+':'+$_.sha256})
        $package=[string[]]@($headers.files|Where-Object {$_.path -clike 'package-include/*'}|ForEach-Object {$_.path+':'+$_.sha256})
        [Array]::Sort($source,[StringComparer]::Ordinal);[Array]::Sort($package,[StringComparer]::Ordinal)
        $all=[string[]]@($source+$package);[Array]::Sort($all,[StringComparer]::Ordinal)
        $hashRows={param([string[]]$rows)[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($rows -join [char]10))))}
        $headers.sourceSha256=& $hashRows $source
        $headers.packageSha256=& $hashRows $package
        $headers.combinedSha256=& $hashRows $all
        $headers.sha256=$headers.combinedSha256
    }
    foreach($case in @(
        @{label='wrong allowed header';edit={param($s)$row=$s.candidate.publicHeaders.files|Where-Object {$_.path -ceq 'include/DxUi/RichTextLayout.h'}|Select-Object -First 1;$row.sha256='A'*64}},
        @{label='changed package row';edit={param($s)$row=$s.candidate.publicHeaders.files|Where-Object {$_.path -ceq 'package-include/wil/com_apartment_variable.h'}|Select-Object -First 1;$row.sha256='B'*64}},
        @{label='extra identity field';edit={param($s)$s.candidate.build.dependency.Identity['unreviewed']='x'}},
        @{label='case-changed identity key';edit={param($s)$value=$s.candidate.build.dependency.Identity.apiRevision;$s.candidate.build.dependency.Identity.Remove('apiRevision');$s.candidate.build.dependency.Identity['ApiRevision']=$value}},
        @{label='string identity type';edit={param($s)$s.candidate.build.dependency.Identity.apiRevision='4'}})) {
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $fullV4Study -Depth 100 -Compress) -AsHashtable
        & $case.edit $bad
        if($case.label -in @('wrong allowed header','changed package row')) { & $refreshInventory $bad.candidate.publicHeaders }
        $rejected=$false
        try {Assert-DependencyEvidence $bad $fullV4Comparison $fixtureRoot} catch {$rejected=$true}
        Require $rejected "V4 full dependency auditor rejects $($case.label) on portable fixture data."
    }
    foreach($case in @(
        @{label='null identity type';edit={param($s)$s.candidate.build.dependency.Identity.apiRevision=$null}},
        @{label='changed numeric identity type';edit={param($s)$s.candidate.build.dependency.Identity.apiRevision=[double]4}})) {
        $bad=ConvertFrom-Json -InputObject (ConvertTo-Json $fullV4Study -Depth 100 -Compress) -AsHashtable
        & $case.edit $bad
        $rejected=$false
        try {Assert-DependencyEvidence $bad $fullV4Comparison $fixtureRoot} catch {$rejected=$true}
        Require $rejected "V4 full dependency auditor rejects $($case.label) on portable fixture data."
    }
    $profile=$profileV2;$profileBytes=$profileV2Bytes;$profileHash=$profileV2Hash
    [IO.File]::WriteAllBytes($profileCopy,$profileBytes)
    $v1Bytes=[IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'Fixtures/ReviewedHeaderDeltaProfile.v1.json'))
    $v1Hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($v1Bytes))
    $v1Profile=[Text.Encoding]::UTF8.GetString($v1Bytes)|ConvertFrom-Json -AsHashtable
    Require ($v1Hash -ceq 'B8003310580F17A58327FA28482595C699F6D8BC1CA712AEC90677C5C726E751' -and
        $v1Profile.profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1') 'The historical v1 auditor oracle must retain the exact frozen profile.'
    $profile=$v1Profile;$profileBytes=$v1Bytes;$profileHash=$v1Hash
    [IO.File]::WriteAllBytes($profileCopy,$profileBytes)
    $v1Fixture=& $makeStudy;$v1Study=$v1Fixture[0];$v1Comparison=$v1Fixture[1]
    Assert-CommonHarnessInputIdentity $v1Study $v1Comparison
    Assert-DependencyEvidence $v1Study $v1Comparison $fixtureRoot
    Require $true 'Independent auditor accepts the frozen v1 profile after current-profile evolution.'
    $badV1=ConvertFrom-Json -InputObject (ConvertTo-Json $v1Study -Depth 40 -Compress) -AsHashtable
    $badV1.reviewedHeaderDelta.abiProbe.candidate.Document.sizeBytes++
    Assert-HeaderProfileRejected $badV1 $v1Comparison 'frozen v1 layout mismatch'
}
$results=@(foreach($directory in $StudyPath) {
    $root=[IO.Path]::GetFullPath($directory).TrimEnd('\','/')
    $study=Get-Content (Join-Path $root 'study.json') -Raw | ConvertFrom-Json -AsHashtable
    $comparison=Get-Content (Join-Path $root 'comparison.json') -Raw | ConvertFrom-Json -AsHashtable
    Require ($study.protocol -ceq 'independent-ABBA-BAAB-v1' -and $comparison.protocol -ceq $study.protocol) 'Recorded protocols must agree.'
    Require ($comparison.judge -ceq 'redtilde-paired-log-sign-flip-holm-v2') 'Only corrected v2 output can be audited.'
    Require ($comparison.acceptance.StartsWith('review-required')) 'Evidence must not self-qualify its policy.'
    Require ($comparison.familySize -eq 12 -and $comparison.metrics.Count -eq 12) 'The predeclared family has twelve slots.'
    Require ($study.schedule.Count -eq 2*$study.blocks -and $study.blocks -ge 2 -and $study.blocks -le 16) 'Complete independent schedule required.'
    Require ($study.cycles -ge 4 -and $study.cycles -le 64 -and $comparison.independentBlocksPerScenario -eq $study.blocks -and
        $comparison.sameBinaryControl -eq $study.sameBinaryControl) 'Comparison scope differs from its study.'
    foreach($side in @('baseline','candidate')) {
        Require ($study[$side].build.passed -and $study[$side].build.platform -ceq 'x64' -and
            $study[$side].build.configuration -ceq 'Release') 'Successful x64 Release identities are required.'
    }
    $compilerAdoption=$study.Contains('dependencyComparisonMode') -and $study.dependencyComparisonMode -cin @('sdk-adoption','reviewed-header-delta')
    if(-not $compilerAdoption) { Require ($study.baseline.compiler.sha256 -ceq $study.candidate.compiler.sha256) 'Compiler/dependency identity differs.' }
    Assert-CommonHarnessInputIdentity $study $comparison
    Assert-DependencyEvidence $study $comparison $root
    Assert-CompilerProfileEvidence -Study $study -Root $root
    foreach($key in $study.baseline.environment.Keys) {
        Require ($study.baseline.environment[$key] -ceq $study.candidate.environment[$key]) 'Recorded environments differ.'
    }
    if(-not $study.sameBinaryControl) {
        Require ($comparison.sourceIdentity -ceq $study.candidate.build.sourceIdentity -and
            $study.baseline.build.binarySha256 -cne $study.candidate.build.binarySha256) 'Candidate source/binary binding differs.'
    }
    $identity=@{}; $blocks=@{}; $leaves=@{}
    foreach($scheduled in $study.schedule) {
        $scheduledBlockName=$scheduled.scenario+'-'+([int]$scheduled.block).ToString('00')
        Require ($scheduled.scenario -cin @('Compose','Split') -and $scheduled.block -ge 0 -and $scheduled.block -lt $study.blocks) 'Scheduled block is outside its declared scope.'
        Require (-not $blocks.ContainsKey($scheduledBlockName)) 'Duplicate scheduled block.'
        $pair=Get-Content (Join-Path $root ($scheduledBlockName+'.pair.json')) -Raw | ConvertFrom-Json -AsHashtable
        Require ($pair.scenario -ceq $scheduled.scenario -and $pair.block -eq $scheduled.block -and $pair.order -ceq $scheduled.order -and
            $pair.order -cin @('ABBA','BAAB')) 'Pair must match its retained random schedule.'
        foreach($slot in @('A1','A2','B1','B2')) {
            $receipt=$pair[$slot+'Receipt']; $leaf=[string]$receipt.file
            Require ([IO.Path]::GetFileName($leaf) -ceq $leaf) 'Native receipt must be a leaf in its study.'
            Require (-not $leaves.ContainsKey($leaf)) 'Each native slot requires its own receipt.'
            $leaves[$leaf]=$true
            $path=Join-Path $root $leaf
            Require ((Get-FileHash -LiteralPath $path).Hash -ceq $receipt.sha256) 'Native bytes do not match their retained hash.'
            $expected=if($study.sameBinaryControl -or $slot.StartsWith('A')) { $study.baseline.build.binarySha256 } else { $study.candidate.build.binarySha256 }
            Require ($receipt.executableSha256 -ceq $expected) 'Native slot executable identity differs.'
            $native=Get-Content $path -Raw | ConvertFrom-Json -AsHashtable
            $legacy=$native.schemaVersion -eq 1 -and $native.fixture -ceq 'redtilde-completed-native-v1'
            $revised=$native.schemaVersion -eq 2 -and $native.fixture -ceq 'redtilde-completed-native-v2'
            Require ($native.completed -and ($legacy -or $revised) -and
                $native.view -ceq $scheduled.scenario -and $native.cycles -eq $study.cycles) 'Native protocol did not complete its scheduled work.'
            Require ($native.activity.changes -eq 2*($study.cycles+2)) 'Warmup and timed edit/undo counts must be intact.'
            if($revised) {
                Require ($native.warmupCycles -eq 2 -and $native.recoverySettledAtMemory -and
                    -not $native.initial.threadInventoryCollected -and -not $native.final.threadInventoryCollected -and
                    $null -eq $native.initial.threads -and $null -eq $native.final.threads) 'Revised latency memory observations must be cheap and settled.'
                foreach($processorKey in @('ownerProcessorStart','ownerProcessorEnd')) {
                    $processors=[double[]]$native.samples[$processorKey]
                    Require ($processors.Count -eq 2*$study.cycles -and @($processors | Where-Object {
                        -not [double]::IsFinite($_) -or $_ -lt 0 -or $_ -ge 4096 -or $_ -ne [math]::Floor($_) }).Count -eq 0) 'Invalid processor boundary observations.'
                }
            }
            $hasControl=$native.Contains('controlDiagnostics') -and $null -ne $native.controlDiagnostics
            if($study.Contains('controlDiagnostics')) {
                Require ($hasControl -eq [bool]$study.controlDiagnostics -and
                    (-not $study.controlDiagnostics -or $native.controlDiagnostics.protocol -ceq 'app-edit-phases-v2')) 'Native diagnostics differ from the declared study mode.'
            }
            if($hasControl) {
                $diagnostic=$native.controlDiagnostics
                $phaseNames=@('enter','analysis','mirror','chrome','selection','recovery','exit')
                if($diagnostic.protocol -ceq 'app-edit-phases-v2') { $phaseNames=@('enter','analysis','mirror','identity','caption','chrome','selection','recovery','exit') }
                Require ($revised -and $diagnostic.protocol -cin @('app-edit-phases-v1','app-edit-phases-v2') -and
                    $diagnostic.primaryBeforeInventory -and $diagnostic.phases.Count -eq $phaseNames.Count -and
                    $diagnostic.postMemory.Count -eq 3) 'Invalid command diagnostic protocol or inventory order.'
                if($diagnostic.protocol -ceq 'app-edit-phases-v2') {
                    Require ($native.activity.captionRequests -eq $native.activity.changes -and
                        $native.activity.captionCalls -eq $native.activity.changes) 'The alternating saved/dirty fixture must execute every synchronous native caption write.'
                }
                $sampleCount=2*$study.cycles
                for($phaseIndex=0;$phaseIndex -lt $phaseNames.Count;++$phaseIndex) {
                    $phase=$diagnostic.phases[$phaseIndex]
                    Require ($phase.name -ceq $phaseNames[$phaseIndex]) 'Diagnostic phase names/order differ.'
                    foreach($phaseMetric in 'wallUs','ownerCpuCycles') {
                        $values=[double[]]$phase[$phaseMetric]
                        Require ($values.Count -eq $sampleCount -and @($values | Where-Object {
                            -not [double]::IsFinite($_) -or $_ -lt 0 }).Count -eq 0) 'Missing or invalid diagnostic phase samples.'
                    }
                }
                foreach($frameMetric in 'preparationOwnerCpuCycles','gpuWaitOwnerCpuCycles','renderCounts','preparationAdvances') {
                    $values=[double[]]$diagnostic[$frameMetric]
                    Require ($values.Count -eq $sampleCount -and @($values | Where-Object {
                        -not [double]::IsFinite($_) -or $_ -lt 0 -or $_ -ne [math]::Floor($_) }).Count -eq 0) 'Missing or invalid frame diagnostic samples.'
                }
                for($sampleIndex=0;$sampleIndex -lt $sampleCount;++$sampleIndex) {
                    $phaseWall=0.0; $phaseCpu=0.0
                    foreach($phase in $diagnostic.phases) { $phaseWall+=$phase.wallUs[$sampleIndex]; $phaseCpu+=$phase.ownerCpuCycles[$sampleIndex] }
                    Require ($phaseWall -le $native.samples.commandUs[$sampleIndex]+1e-6 -and
                        $phaseCpu -le $native.samples.commandOwnerCpuCycles[$sampleIndex] -and $diagnostic.renderCounts[$sampleIndex] -ge 1) 'Diagnostic subphases exceed their command or lack a rendered frame.'
                    Near ($native.samples.commandOwnerCpuCycles[$sampleIndex]+$diagnostic.preparationOwnerCpuCycles[$sampleIndex]+
                        $diagnostic.gpuWaitOwnerCpuCycles[$sampleIndex]) $native.samples.ownerCpuCycles[$sampleIndex] 'Diagnostic CPU accounting does not reconstruct the primary sample.'
                }
                for($memoryIndex=0;$memoryIndex -lt 3;++$memoryIndex) {
                    $memory=$diagnostic.postMemory[$memoryIndex]
                    foreach($memoryMetric in 'privateBytes','workingSetBytes') {
                        Require ([double]::IsFinite($memory[$memoryMetric]) -and $memory[$memoryMetric] -gt 0) 'Invalid post-primary memory sample.'
                    }
                    if($memoryIndex -lt 2) { Require (-not $memory.threadInventoryCollected -and $null -eq $memory.threads) 'Early post-primary snapshots must remain cheap.' }
                    else { Require ($memory.threadInventoryCollected -and $memory.threads -gt 0) 'Late diagnostic thread inventory is missing.' }
                }
                $hasSelection=$diagnostic.Contains('selectionComponents') -and $null -ne $diagnostic.selectionComponents
                if($study.Contains('selectionDiagnosticsProtocol') -and $null -ne $study.selectionDiagnosticsProtocol) {
                    Require ($study.selectionDiagnosticsProtocol -ceq 'app-selection-components-v1' -and $hasSelection) 'Declared nested selection diagnostics are missing.'
                }
                if($hasSelection) {
                    $detail=$diagnostic.selectionComponents
                    $componentNames=@('selection-snapshot','document-analysis-refresh','selection-facts-cache','mirror','inline-semantics',
                        'tool-caption-publication','outline','block-lookup','block-labels','position-status','find','routing')
                    Require ($diagnostic.protocol -ceq 'app-edit-phases-v2' -and $detail.protocol -ceq 'app-selection-components-v1' -and
                        $detail.transactions.Count -eq $sampleCount) 'Invalid selection diagnostic protocol or transaction count.'
                    for($sampleIndex=0;$sampleIndex -lt $sampleCount;++$sampleIndex) {
                        $transaction=$detail.transactions[$sampleIndex]
                        Require ($transaction.complete -is [bool] -and $transaction.complete -and
                            (Test-NonnegativeEvidenceNumber $transaction.invocations) -and $transaction.invocations -eq 1 -and
                            $transaction.components.Count -eq $componentNames.Count) 'Selection diagnostics must contain exactly one complete invocation.'
                        $componentWall=0.0; $componentCpu=0.0
                        for($componentIndex=0;$componentIndex -lt $componentNames.Count;++$componentIndex) {
                            $component=$transaction.components[$componentIndex]
                            Require ($component.name -ceq $componentNames[$componentIndex]) 'Selection component names or ordering differ.'
                            Require ((Test-NonnegativeEvidenceNumber $component.calls) -and
                                $component.calls -eq [math]::Floor($component.calls)) 'Invalid selection component call count.'
                            if($component.calls -eq 0) {
                                Require ($null -eq $component.wallUs -and $null -eq $component.ownerCpuCycles) 'Unexecuted selection components must remain unavailable.'
                                continue
                            }
                            foreach($value in @($component.wallUs,$component.ownerCpuCycles)) {
                                Require (Test-NonnegativeEvidenceNumber $value) 'Invalid selection component samples.'
                            }
                            Require ($component.ownerCpuCycles -eq [math]::Floor($component.ownerCpuCycles)) 'Selection CPU cycles must be integral.'
                            $componentWall+=[double]$component.wallUs; $componentCpu+=[double]$component.ownerCpuCycles
                        }
                        $parentWall=[double]$diagnostic.phases[6].wallUs[$sampleIndex]
                        $parentCpu=[double]$diagnostic.phases[6].ownerCpuCycles[$sampleIndex]
                        Require ((Test-NonnegativeEvidenceNumber $transaction.wallTotalUs) -and
                            (Test-NonnegativeEvidenceNumber $transaction.ownerCpuCyclesTotal) -and
                            $transaction.ownerCpuCyclesTotal -eq [math]::Floor($transaction.ownerCpuCyclesTotal) -and
                            $componentWall -le $transaction.wallTotalUs+1e-6 -and $componentCpu -le $transaction.ownerCpuCyclesTotal -and
                            $transaction.wallTotalUs -le $parentWall+1e-6 -and $transaction.ownerCpuCyclesTotal -le $parentCpu) 'Selection component totals exceed their enclosing selection.'
                        Near ($componentWall+[double]$transaction.parentReconciliation.wallUs) $parentWall 'Selection component reconciliation differs.'
                        Near ($componentCpu+[double]$transaction.parentReconciliation.ownerCpuCycles) $parentCpu 'Selection component reconciliation differs.'
                    }
                    Require ($native.Contains('appBuffersAfterPrimary') -and $native.appBuffersAfterPrimary.sourceUnits -eq $native.sourceUnits -and
                        $native.heapDiagnosticCoverage -ceq 'explicit-default-and-crt-only') 'Selection storage diagnostics do not match their source or heap coverage.'
                }
            }
            $hasEditorPreparation=$native.Contains('editorPreparationDiagnostics') -and $null -ne $native.editorPreparationDiagnostics
            if($study.Contains('editorPreparationDiagnosticsProtocol') -and $null -ne $study.editorPreparationDiagnosticsProtocol) {
                Require ($study.editorPreparationDiagnosticsProtocol -ceq 'app-editor-preparation-v1' -and $hasEditorPreparation) 'Declared editor preparation diagnostics are missing.'
            }
            if($hasEditorPreparation) {
                $detail=$native.editorPreparationDiagnostics
                $sampleCount=2*$study.cycles
                Require ($detail.protocol -ceq 'app-editor-preparation-v1' -and $detail.complete -is [bool] -and $detail.complete -and
                    $detail.snapshotSource -ceq 'DxUi.RichTextEditor.DebugGetState' -and
                    $detail.attribution -ceq 'state deltas only; no per-stage CPU attribution' -and
                    $detail.transactions.Count -eq $sampleCount -and
                    (Test-NonnegativeEvidenceNumber $detail.storage.reservedBytes) -and
                    $detail.storage.reservedBytes -eq [math]::Floor($detail.storage.reservedBytes) -and
                    (Test-NonnegativeEvidenceNumber $detail.storage.attemptCapacity) -and
                    $detail.storage.attemptCapacity -eq [math]::Floor($detail.storage.attemptCapacity) -and
                    (Test-NonnegativeEvidenceNumber $detail.storage.transactionCapacity) -and
                    $detail.storage.transactionCapacity -eq [math]::Floor($detail.storage.transactionCapacity) -and
                    (Test-NonnegativeEvidenceNumber $detail.storage.attemptRecordBytes) -and $detail.storage.attemptRecordBytes -gt 0 -and
                    $detail.storage.attemptRecordBytes -eq [math]::Floor($detail.storage.attemptRecordBytes) -and
                    (Test-NonnegativeEvidenceNumber $detail.storage.transactionRecordBytes) -and $detail.storage.transactionRecordBytes -gt 0 -and
                    $detail.storage.transactionRecordBytes -eq [math]::Floor($detail.storage.transactionRecordBytes) -and
                    $detail.storage.reservedBytes -eq ($detail.storage.attemptCapacity*$detail.storage.attemptRecordBytes+
                        $detail.storage.transactionCapacity*$detail.storage.transactionRecordBytes) -and
                    $detail.storage.attemptCapacity -ge $sampleCount -and $detail.storage.transactionCapacity -ge $sampleCount -and
                    $detail.storage.reservedBytes -gt 0 -and $detail.storage.maxAttemptsPerTransaction -eq 64 -and
                    $detail.storage.retainedThroughPostMeasurement -is [bool] -and $detail.storage.retainedThroughPostMeasurement -and
                    $native.Contains('frameDiagnostics') -and $native.frameDiagnostics.protocol -ceq 'dxui-frame-metrics-v1' -and
                    $native.frameDiagnostics.Contains('editTransactions') -and
                    $native.frameDiagnostics.editTransactions.Count -eq $sampleCount) 'Invalid editor preparation diagnostic protocol, storage or frame binding.'
                $counterNames=@('preparationAdvances','cancelledPreparations','markdownPreparationAdvances','semanticPreparationAdvances')
                $workNames=@('retainedGeometryChunks','preparedGeometryChunks','reusedGeometryChunks','preparedSourceUnits',
                    'wholeLayoutsPrepared','wholeLayoutSourceUnits','lastAdvanceLayouts','lastAdvanceBlockSteps',
                    'lastAdvanceMarkdownWork','lastAdvanceSemanticWork','sourceInkRangeCount','layoutGeneration')
                $booleanNames=@('layoutReady','wholeGeometryFallback','wholeRasterFallback','preparationPending',
                    'markdownPreparationPending','semanticPreparationPending','sourceInkPreparationPending')
                $previousPrimary=$null; $previousMirror=$null; $nextAttemptOffset=0
                for($transactionIndex=0;$transactionIndex -lt $sampleCount;++$transactionIndex) {
                    $transaction=$detail.transactions[$transactionIndex]
                    $frame=$native.frameDiagnostics.editTransactions[$transactionIndex]
                    Require ($transaction.complete -is [bool] -and $transaction.complete -and
                        $transaction.firstAttemptIndex -eq $nextAttemptOffset -and
                        (Test-NonnegativeEvidenceNumber $transaction.attemptCount) -and
                        $transaction.attemptCount -eq [math]::Floor($transaction.attemptCount) -and
                        $transaction.attemptCount -gt 0 -and $transaction.attemptCount -le 64 -and
                        $transaction.attempts.Count -eq $transaction.attemptCount -and
                        $frame.complete -is [bool] -and $frame.complete -and
                        $frame.attemptCount -eq $transaction.attemptCount -and
                        $frame.requestedAttemptCount -eq $transaction.attemptCount -and
                        $frame.attempts.Count -eq $transaction.attemptCount) 'Invalid editor preparation transaction or attempt count.'
                    $renderDelta=0.0
                    $previousAttemptRender=$null; $transactionPrimary=$null; $transactionMirror=$null
                    for($attemptIndex=0;$attemptIndex -lt $transaction.attemptCount;++$attemptIndex) {
                        $attempt=$transaction.attempts[$attemptIndex]
                        $frameAttempt=$frame.attempts[$attemptIndex]
                        Require ($attempt.frameMetricAttemptIndex -eq $attemptIndex -and
                            (Test-NonnegativeEvidenceNumber $attempt.hostRenderCountBefore) -and
                            $attempt.hostRenderCountBefore -eq [math]::Floor($attempt.hostRenderCountBefore) -and
                            (Test-NonnegativeEvidenceNumber $attempt.hostRenderCountAfter) -and
                            $attempt.hostRenderCountAfter -eq [math]::Floor($attempt.hostRenderCountAfter) -and
                            $attempt.hostRenderCountAfter -ge $attempt.hostRenderCountBefore -and
                            $attempt.hostRenderCountAfter-$attempt.hostRenderCountBefore -le 1 -and
                            $attempt.primary.Contains('before') -and $attempt.primary.Contains('after') -and
                            $attempt.mirror.Contains('before') -and $attempt.mirror.Contains('after')) 'Invalid editor preparation attempt identity or render count.'
                        if($attemptIndex -gt 0) {
                            Require ($attempt.hostRenderCountBefore -eq $previousAttemptRender) 'Editor preparation render counts are discontinuous within a transaction.'
                        }
                        $previousAttemptRender=[double]$attempt.hostRenderCountAfter
                        $renderDelta += [double]$attempt.hostRenderCountAfter-[double]$attempt.hostRenderCountBefore
                        foreach($editorName in @('primary','mirror')) {
                            $editor=$attempt[$editorName]
                            foreach($edge in @('before','after')) {
                                $state=$editor[$edge]
                                foreach($name in $workNames+$counterNames) {
                                    Require ($state.Contains($name) -and (Test-NonnegativeEvidenceNumber $state[$name]) -and
                                        $state[$name] -eq [math]::Floor($state[$name])) 'Invalid editor preparation work/counter field.'
                                }
                                foreach($name in $booleanNames) {
                                    Require ($state.Contains($name) -and $state[$name] -is [bool]) 'Invalid editor preparation state flag.'
                                }
                            }
                            $before=$editor.before; $after=$editor.after
                            foreach($name in $counterNames) {
                                Require ([double]$after[$name] -ge [double]$before[$name]) 'Editor preparation cumulative counter decreased during an attempt.'
                            }
                            if($editorName -ceq 'primary') {
                                if($null -ne $transactionPrimary) {
                                    foreach($name in $counterNames) { Require ([double]$before[$name] -ge [double]$transactionPrimary[$name]) 'Primary editor preparation counter decreased between attempts.' }
                                }
                                $transactionPrimary=$after
                            }
                            else {
                                if($null -ne $transactionMirror) {
                                    foreach($name in $counterNames) { Require ([double]$before[$name] -ge [double]$transactionMirror[$name]) 'Mirror editor preparation counter decreased between attempts.' }
                                }
                                $transactionMirror=$after
                            }
                        }
                        foreach($editorName in @('primary','mirror')) {
                            $editor=$attempt[$editorName]
                            if($null -ne $previousPrimary -and $editorName -ceq 'primary') {
                                foreach($name in $counterNames) { Require ([double]$editor.before[$name] -ge [double]$previousPrimary[$name]) 'Primary editor preparation counter decreased between transactions.' }
                            }
                            if($null -ne $previousMirror -and $editorName -ceq 'mirror') {
                                foreach($name in $counterNames) { Require ([double]$editor.before[$name] -ge [double]$previousMirror[$name]) 'Mirror editor preparation counter decreased between transactions.' }
                            }
                        }
                        $previousPrimary=$attempt.primary.after
                        $previousMirror=$attempt.mirror.after
                        Require ((Test-NonnegativeEvidenceNumber $frameAttempt.totalUs) -and
                            (Test-NonnegativeEvidenceNumber $frameAttempt.updateUs) -and
                            (Test-NonnegativeEvidenceNumber $frameAttempt.renderUs) -and
                            (Test-NonnegativeEvidenceNumber $frameAttempt.presentUs) -and
                            $frameAttempt.totalUs -eq [math]::Floor($frameAttempt.totalUs) -and
                            $frameAttempt.updateUs -eq [math]::Floor($frameAttempt.updateUs) -and
                            $frameAttempt.renderUs -eq [math]::Floor($frameAttempt.renderUs) -and
                            $frameAttempt.presentUs -eq [math]::Floor($frameAttempt.presentUs) -and
                            $frameAttempt.updateUs+$frameAttempt.renderUs+$frameAttempt.presentUs -le $frameAttempt.totalUs+3) 'Invalid bound frame metric attempt.'
                    }
                    Require ((Test-NonnegativeEvidenceNumber $frame.hostRenderCount) -and
                        $renderDelta -eq [double]$frame.hostRenderCount) 'Editor preparation render delta differs from its frame metrics.'
                    $nextAttemptOffset += $transaction.attemptCount
                }
                Require ($detail.storage.attemptCapacity -ge $nextAttemptOffset) 'Editor preparation attempt capacity does not cover retained attempts.'
            }
            $nativeIdentity=@($native.fixture,$native.view,$native.renderer,$native.adapter,$native.vendorId,$native.deviceId,$native.driverVersion,
                $native.dpi,$native.widthPx,$native.heightPx,$native.sourceUnits) | ConvertTo-Json -Compress
            if($identity.ContainsKey($native.view)) { Require ($identity[$native.view] -ceq $nativeIdentity) 'Renderer/driver/DPI/source identity drift.' }
            else { $identity[$native.view]=$nativeIdentity }
            $metrics=@('commandUs','preparationSubmissionUs','gpuWaitUs','completedEditUs','cleanCompletedUs','ownerCpuCycles','privateBytes','workingSetBytes')
            if($revised) { $metrics+='commandOwnerCpuCycles' }
            foreach($metric in $metrics) {
                if($metric -like '*Bytes') { $value=[double]$native.final[$metric] }
                else {
                    $samples=[double[]]$native.samples[$metric]
                    $count=if($metric -ceq 'cleanCompletedUs') { $study.cycles } else { 2*$study.cycles }
                    Require ($samples.Count -eq $count -and @($samples | Where-Object { -not [double]::IsFinite($_) -or $_ -le 0 }).Count -eq 0) 'Missing/invalid native samples.'
                    $value=[RedTilde.PairedEvidenceReference]::Median($samples)
                }
                Require ([double]::IsFinite($value) -and $value -gt 0) 'Native metric must be finite and positive.'
                Near ([double]$pair[$slot][$metric]) $value "Native/pair median differs: $scheduledBlockName $slot $metric"
            }
        }
        $blocks[$scheduledBlockName]=$pair
    }
    $reference=[Collections.Generic.List[object]]::new()
    foreach($scenario in @('Compose','Split')) {
        $group=@($blocks.Values | Where-Object scenario -ceq $scenario | Sort-Object block)
        Require ($group.Count -eq $study.blocks) 'A complete independent group is required per view.'
        foreach($metric in @('commandUs','ownerCpuCycles','completedEditUs','cleanCompletedUs','privateBytes','workingSetBytes')) {
            $logs=[Collections.Generic.List[double]]::new(); $a=[Collections.Generic.List[double]]::new(); $b=[Collections.Generic.List[double]]::new()
            $drift=[Collections.Generic.List[double]]::new()
            foreach($block in $group) {
                $a1=[double]$block.A1[$metric]; $a2=[double]$block.A2[$metric]; $b1=[double]$block.B1[$metric]; $b2=[double]$block.B2[$metric]
                $logs.Add(([math]::Log($b1/$a1)+[math]::Log($b2/$a2))/2.0)
                $a.Add($a1); $a.Add($a2); $b.Add($b1); $b.Add($b2)
                $drift.Add([math]::Abs($a2/$a1-1.0)); $drift.Add([math]::Abs($b2/$b1-1.0))
            }
            $reference.Add([ordered]@{scenario=$scenario;metric=$metric;p=[RedTilde.PairedEvidenceReference]::Tail($logs.ToArray());
                pairedChange=[RedTilde.PairedEvidenceReference]::Effect($logs.ToArray());baselineMedian=[RedTilde.PairedEvidenceReference]::Median($a.ToArray());
                candidateMedian=[RedTilde.PairedEvidenceReference]::Median($b.ToArray());drift=$drift.ToArray()})
        }
    }
    $adjusted=[RedTilde.PairedEvidenceReference]::Holm([double[]]@($reference | ForEach-Object { $_.p }))
    for($index=0;$index -lt $reference.Count;++$index) {
        $row=$reference[$index]; $actual=@($comparison.metrics | Where-Object { $_.scenario -ceq $row.scenario -and $_.metric -ceq $row.metric })
        Require ($actual.Count -eq 1) 'A unique predeclared result is required.'
        foreach($key in @('p','pairedChange','baselineMedian','candidateMedian')) { Near ([double]$actual[0][$key]) ([double]$row[$key]) "Reference $key differs." }
        Near ([double]$actual[0].adjustedP) $adjusted[$index] 'Independent Holm result differs.'
        $band=if($row.metric -like '*Bytes') { 0.02 } else { 0.05 }
        Near ([double]$actual[0].band) $band 'Investigation band differs from the fixed family.'
        Require ($actual[0].blocks -eq $study.blocks) 'Metric independent-block count differs.'
        $stable=@($row.drift | Where-Object { $_ -gt $band }).Count -eq 0
        Require ($actual[0].controlsStable -eq $stable -and $actual[0].controlDrift.Count -eq $row.drift.Count) 'Control flags must retain all same-side drift.'
        for($item=0;$item -lt $row.drift.Count;++$item) { Near ([double]$actual[0].controlDrift[$item]) $row.drift[$item] 'A raw control drift differs.' }
        $finding=if($adjusted[$index] -le 0.05 -and $row.pairedChange -gt $band) { 'regressed' }
            elseif($adjusted[$index] -le 0.05 -and $row.pairedChange -lt -$band) { 'improved' } else { 'no-resolved-change' }
        Require ($actual[0].finding -ceq $finding) 'Directional finding differs from the independent reference.'
    }
    [ordered]@{study=$root;comparisonSha256=(Get-FileHash (Join-Path $root 'comparison.json')).Hash;nativeSlots=4*$blocks.Count;
        familySize=$reference.Count;independentReferenceMatches=$true;policyAcceptance='review-required'}
})
if($Output) {
    [ordered]@{schemaVersion=1;reference='typed-CSharp-subset-enumeration-v1';knownValueOracles=$true;checks=$script:checks;
        studies=@($results);completedUtc=[DateTime]::UtcNow.ToString('o');acceptance='numerical/integrity audit only; no policy self-approval'} |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Output -Encoding utf8
}
if(-not $SelfTest -and $StudyPath.Count -eq 0) { throw 'Supply a study path or SelfTest.' }
Write-Host "Independent evidence reference: $script:checks checks passed, $(@($results).Count) studies."
