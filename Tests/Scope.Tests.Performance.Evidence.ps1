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
    public static void VerifyCompilerCommands(string[] lines,string root,string[] relativeSources,string expected) {
        string[] reviewed = relativeSources.Select(value => root.TrimEnd('\\','/')+"\\"+value.Replace('/','\\')).ToArray();
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
        }
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
    foreach($side in 'baseline','candidate') {
        $profile=$Study[$side].compiler
        Require ($profile.Contains('schemaVersion') -and $profile.schemaVersion -eq 2 -and @($profile.commands).Count -eq 1 -and
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
        [RedTilde.PairedEvidenceReference]::VerifyCompilerCommands([string[]]@(Get-Content -LiteralPath (Join-Path $Root $raw.file)),
            [string]$profile.sourceRoot,[string[]]@($profile.translationUnits|ForEach-Object source),$command)
    }
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
    Require ($study.baseline.compiler.sha256 -ceq $study.candidate.compiler.sha256 -and
        $study.baseline.build.dependency.Fingerprint -ceq $study.candidate.build.dependency.Fingerprint) 'Compiler/dependency identity differs.'
    Assert-CompilerProfileEvidence -Study $study -Root $root
    foreach($key in $study.baseline.inputs.Keys) {
        Require ($study.baseline.inputs[$key] -ceq $study.candidate.inputs[$key] -and
            $study.baseline.inputs[$key] -ceq $comparison.harnessInputs[$key]) 'Common harness identities differ.'
    }
    foreach($key in $study.baseline.environment.Keys) {
        Require ($study.baseline.environment[$key] -ceq $study.candidate.environment[$key]) 'Recorded environments differ.'
    }
    if(-not $study.sameBinaryControl) {
        Require ($comparison.sourceIdentity -ceq $study.candidate.build.sourceIdentity -and
            $study.baseline.build.binarySha256 -cne $study.candidate.build.binarySha256) 'Candidate source/binary binding differs.'
    }
    $identity=@{}; $blocks=@{}; $leaves=@{}
    foreach($scheduled in $study.schedule) {
        $name=$scheduled.scenario+'-'+([int]$scheduled.block).ToString('00')
        Require ($scheduled.scenario -cin @('Compose','Split') -and $scheduled.block -ge 0 -and $scheduled.block -lt $study.blocks) 'Scheduled block is outside its declared scope.'
        Require (-not $blocks.ContainsKey($name)) 'Duplicate scheduled block.'
        $pair=Get-Content (Join-Path $root ($name+'.pair.json')) -Raw | ConvertFrom-Json -AsHashtable
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
                Near ([double]$pair[$slot][$metric]) $value "Native/pair median differs: $name $slot $metric"
            }
        }
        $blocks[$name]=$pair
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
