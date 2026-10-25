Set-StrictMode -Version Latest

function Get-RedTildeMedian {
    param([double[]] $Values)
    if (-not $Values.Count -or @($Values | Where-Object { -not [double]::IsFinite($_) }).Count) { throw 'Finite nonempty samples are required.' }
    $ordered=@($Values | Sort-Object)
    $middle=[int][math]::Floor($ordered.Count/2)
    if ($ordered.Count%2) { return $ordered[$middle] }
    return ($ordered[$middle-1]+$ordered[$middle])/2
}

function Get-RedTildePerformanceMetrics {
    param($Native)
    $legacy=$Native.schemaVersion -eq 1 -and $Native.fixture -ceq 'redtilde-completed-native-v1'
    $revised=$Native.schemaVersion -eq 2 -and $Native.fixture -ceq 'redtilde-completed-native-v2'
    if (-not $Native.completed -or (-not $legacy -and -not $revised) -or
        $Native.view -cnotin @('Compose','Split') -or $Native.renderer -cnotin @('Hardware','WARP') -or
        $Native.cycles -lt 4 -or $Native.cycles -gt 64 -or $Native.widthPx -le 0 -or $Native.heightPx -le 0 -or $Native.dpi -lt 48) {
        throw 'Incomplete or incompatible native performance report.'
    }
    $result=[ordered]@{}
    $timings=@('commandUs','preparationSubmissionUs','gpuWaitUs','completedEditUs','cleanCompletedUs','ownerCpuCycles')
    if($revised) { $timings+='commandOwnerCpuCycles' }
    foreach($name in $timings) {
        $values=@($Native.samples.$name)
        $count=if($name -eq 'cleanCompletedUs') { $Native.cycles } else { 2*$Native.cycles }
        if($values.Count -ne $count -or @($values | Where-Object { -not [double]::IsFinite($_) -or $_ -le 0 }).Count) { throw "Invalid $name samples." }
        $result[$name]=Get-RedTildeMedian $values
    }
    if($revised) {
        if($Native.warmupCycles -ne 2 -or -not $Native.recoverySettledAtMemory -or
            $Native.initial.threadInventoryCollected -or $Native.final.threadInventoryCollected -or
            $null -ne $Native.initial.threads -or $null -ne $Native.final.threads) { throw 'Latency memory observations require settled recovery and cheap snapshots.' }
        foreach($name in @('ownerProcessorStart','ownerProcessorEnd')) {
            $values=@($Native.samples.$name)
            if($values.Count -ne 2*$Native.cycles -or @($values | Where-Object { -not [double]::IsFinite($_) -or $_ -lt 0 -or $_ -ge 4096 -or $_ -ne [math]::Floor($_) }).Count) {
                throw 'Invalid processor-boundary diagnostics.'
            }
        }
    }
    $null=Get-RedTildeControlDiagnostics $Native
    foreach($name in @('privateBytes','workingSetBytes')) {
        $value=[double]$Native.final.$name
        if(-not [double]::IsFinite($value) -or $value -le 0) { throw "Invalid $name." }
        $result[$name]=$value
    }
    return $result
}

function Get-RedTildeControlDiagnostics {
    param($Native)
    $diagnostic=if($Native -is [Collections.IDictionary]) { $Native['controlDiagnostics'] } else {
        $property=$Native.PSObject.Properties['controlDiagnostics']; if($property) { $property.Value } else { $null }
    }
    if($null -eq $diagnostic) { return $null }
    $names=@('enter','analysis','mirror','chrome','selection','recovery','exit')
    if($diagnostic.protocol -ceq 'app-edit-phases-v2') { $names=@('enter','analysis','mirror','identity','caption','chrome','selection','recovery','exit') }
    if($Native.schemaVersion -ne 2 -or $diagnostic.protocol -cnotin @('app-edit-phases-v1','app-edit-phases-v2') -or
        -not $diagnostic.primaryBeforeInventory -or $diagnostic.phases.Count -ne $names.Count -or $diagnostic.postMemory.Count -ne 3) {
        throw 'Incompatible control diagnostics.'
    }
    $count=2*$Native.cycles
    for($phase=0;$phase -lt $names.Count;++$phase) {
        if($diagnostic.phases[$phase].name -cne $names[$phase]) { throw 'Control diagnostic phases are missing, reordered or duplicated.' }
        foreach($key in 'wallUs','ownerCpuCycles') {
            $values=@($diagnostic.phases[$phase].$key)
            if($values.Count -ne $count -or @($values | Where-Object { -not [double]::IsFinite($_) -or $_ -lt 0 }).Count) {
                throw 'Invalid command phase samples.'
            }
        }
    }
    foreach($key in 'preparationOwnerCpuCycles','gpuWaitOwnerCpuCycles','renderCounts','preparationAdvances') {
        $values=@($diagnostic.$key)
        if($values.Count -ne $count -or @($values | Where-Object { -not [double]::IsFinite($_) -or $_ -lt 0 -or $_ -ne [math]::Floor($_) }).Count) {
            throw 'Invalid frame diagnostic samples.'
        }
    }
    for($sample=0;$sample -lt $count;++$sample) {
        $wall=0.0; $cpu=0.0
        foreach($phase in $diagnostic.phases) { $wall+=$phase.wallUs[$sample]; $cpu+=$phase.ownerCpuCycles[$sample] }
        $frame=$Native.samples.commandOwnerCpuCycles[$sample]+$diagnostic.preparationOwnerCpuCycles[$sample]+$diagnostic.gpuWaitOwnerCpuCycles[$sample]
        if($wall -gt $Native.samples.commandUs[$sample]+1e-6 -or $cpu -gt $Native.samples.commandOwnerCpuCycles[$sample] -or
            [math]::Abs($frame-$Native.samples.ownerCpuCycles[$sample]) -gt 0.5 -or $diagnostic.renderCounts[$sample] -lt 1) {
            throw 'Control diagnostic phases exceed or fail to reconstruct their primary transaction.'
        }
    }
    for($index=0;$index -lt 3;++$index) {
        $memory=$diagnostic.postMemory[$index]
        foreach($key in 'privateBytes','workingSetBytes') {
            if(-not [double]::IsFinite($memory.$key) -or $memory.$key -le 0) { throw 'Invalid post-primary memory observation.' }
        }
        if($index -lt 2 -and ($memory.threadInventoryCollected -or $null -ne $memory.threads)) { throw 'Early diagnostic memory must remain cheap.' }
        if($index -eq 2 -and (-not $memory.threadInventoryCollected -or $memory.threads -le 0)) { throw 'Post-primary thread inventory is missing.' }
    }
    return $diagnostic
}

function New-RedTildePerformanceSchedule {
    param([ValidateRange(2,16)][int] $Blocks=12,[ValidateRange(0,2147483647)][int] $Seed=20261021)
    $random=[Random]::new($Seed)
    $result=foreach($scenario in @('Compose','Split')) {
        for($block=0;$block -lt $Blocks;++$block) {
            $order=if($random.Next(2) -eq 0) { 'ABBA' } else { 'BAAB' }
            [ordered]@{scenario=$scenario;block=$block;order=$order}
        }
    }
    return @($result)
}

function Get-RedTildeSignFlipP {
    param([double[]]$Values)
    if($Values.Count -lt 2 -or $Values.Count -gt 16) { throw 'Sign-flip inference requires 2..16 independent blocks.' }
    $observed=[math]::Abs(($Values | Measure-Object -Sum).Sum)
    $extreme=0; $assignments=1 -shl $Values.Count
    for($mask=0;$mask -lt $assignments;++$mask) {
        $sum=0.0
        for($index=0;$index -lt $Values.Count;++$index) {
            $sum+=$(if($mask -band (1 -shl $index)) { -$Values[$index] } else { $Values[$index] })
        }
        if([math]::Abs($sum) -ge $observed-1e-12) { ++$extreme }
    }
    return $extreme/[double]$assignments
}

function Compare-RedTildePerformanceBlocks {
    param([object[]]$Blocks)
    $rows=foreach($scenario in @('Compose','Split')) {
        $group=@($Blocks | Where-Object scenario -ceq $scenario | Sort-Object block)
        if($group.Count -lt 2 -or $group.Count -gt 16 -or @($group.block | Sort-Object -Unique).Count -ne $group.Count) { throw 'Missing or duplicate independent blocks.' }
        # Submission and GPU wait are retained phase diagnostics. Their split
        # changes with presentation pacing; complete latency and owner CPU work
        # are the predeclared cost outcomes, avoiding a residual-wait penalty.
        foreach($metric in @('commandUs','ownerCpuCycles','completedEditUs','cleanCompletedUs','privateBytes','workingSetBytes')) {
            $logs=[Collections.Generic.List[double]]::new()
            $before=[Collections.Generic.List[double]]::new(); $after=[Collections.Generic.List[double]]::new()
            $control=[Collections.Generic.List[double]]::new()
            foreach($block in $group) {
                $a1=[double]$block.A1[$metric]; $a2=[double]$block.A2[$metric]; $b1=[double]$block.B1[$metric]; $b2=[double]$block.B2[$metric]
                if(@($a1,$a2,$b1,$b2 | Where-Object { -not [double]::IsFinite($_) -or $_ -le 0 }).Count) { throw 'Invalid paired metric.' }
                $logs.Add(([math]::Log($b1)+[math]::Log($b2)-[math]::Log($a1)-[math]::Log($a2))/2)
                $before.Add($a1); $before.Add($a2); $after.Add($b1); $after.Add($b2)
                $control.Add([math]::Abs($a2/$a1-1)); $control.Add([math]::Abs($b2/$b1-1))
            }
            $band=if($metric -like '*Bytes') { 0.02 } else { 0.05 }
            [ordered]@{scenario=$scenario;metric=$metric;blocks=$group.Count;baselineMedian=(Get-RedTildeMedian $before.ToArray());
                candidateMedian=(Get-RedTildeMedian $after.ToArray());pairedChange=[math]::Exp(($logs | Measure-Object -Average).Average)-1;
                p=(Get-RedTildeSignFlipP $logs.ToArray());band=$band;controlDrift=@($control.ToArray());controlsStable=(@($control | Where-Object { $_ -gt $band }).Count -eq 0)}
        }
    }
    $sorted=@($rows | Sort-Object { $_.p }); $previous=0.0
    for($index=0;$index -lt $sorted.Count;++$index) {
        $row=$sorted[$index]
        $adjusted=[math]::Min(1.0,[math]::Max($previous,$row.p*($sorted.Count-$index)))
        $previous=$adjusted; $row.adjustedP=$adjusted
        $row.finding=if($adjusted -le 0.05 -and $row.pairedChange -gt $row.band) { 'regressed' }
            elseif($adjusted -le 0.05 -and $row.pairedChange -lt -$row.band) { 'improved' } else { 'no-resolved-change' }
    }
    return @($rows)
}

function Get-RedTildeCompilerProfile {
    param([string]$Root)
    $rootPath=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    function Normalize-PathOperand([string]$Operand) {
        $quoted=$Operand.StartsWith('"') -and $Operand.EndsWith('"')
        $value=if($quoted){$Operand.Substring(1,$Operand.Length-2)}else{$Operand}
        if($value.Contains('"')){throw 'Unsupported compiler path quoting.'}
        if($value.Equals($rootPath,[StringComparison]::OrdinalIgnoreCase) -or
            $value.StartsWith($rootPath+'\',[StringComparison]::OrdinalIgnoreCase) -or
            $value.StartsWith($rootPath+'/',[StringComparison]::OrdinalIgnoreCase)) {
            $value='<ROOT>'+$value.Substring($rootPath.Length)
        }
        if($quoted){return '"'+$value+'"'}
        return $value
    }
    $path=Join-Path $Root '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
    if(-not (Test-Path -LiteralPath $path)) { throw 'The Release compiler command inventory is missing.' }
    [xml]$project=Get-Content -LiteralPath (Join-Path $Root 'RedTilde/RedTilde.vcxproj')
    $sources=@($project.SelectNodes('//*[local-name()="ClCompile"][@Include]') | ForEach-Object {
        [IO.Path]::GetFullPath((Join-Path (Join-Path $Root 'RedTilde') $_.Include))
    })
    $lines=@(Get-Content -LiteralPath $path)
    if(-not $sources.Count -or @($sources|Sort-Object -Unique).Count -ne $sources.Count) {
        throw 'The Release project must list unique translation units.'
    }
    $inventory=foreach($source in $sources) {
        $commandRows=@(
        for($index=0;$index -lt $lines.Count-1;++$index) {
            if($lines[$index].StartsWith('^') -and $source -iin @($lines[$index].Substring(1).Split('|'))) { $lines[$index+1] }
        }
        )
        if($commandRows.Count -ne 1 -or -not $commandRows[0] -or $commandRows[0] -match '(?i)(?:^|\s)/FI') {
            throw "Missing, duplicate or unsupported compiler command for $source."
        }
        $original=$commandRows[0]
        if($original -match '(?<!\\)(?:\\\\)*\\"') {throw 'Escaped compiler quotes require a separately reviewed parser.'}
        $tokenPattern='(?:[^\s"]+|"[^"]*")+'
        if([regex]::Replace($original,$tokenPattern,'').Trim()) {throw 'Unsupported compiler command quoting.'}
        $tokens=@([regex]::Matches($original,$tokenPattern)|ForEach-Object Value)
        $options=[Collections.Generic.List[string]]::new()
        $ownSource=0
        for($tokenIndex=0;$tokenIndex -lt $tokens.Count;++$tokenIndex) {
            $token=$tokens[$tokenIndex]
            # Macro values are opaque, even when they happen to contain a root
            # path or resemble a source filename. Never rewrite their bytes.
            if($token -cmatch '^/[DU]$') {
                if(++$tokenIndex -ge $tokens.Count){throw 'Missing compiler macro operand.'}
                $options.Add($token);$options.Add($tokens[$tokenIndex]);continue
            }
            $argument=$token.Trim('"')
            if($token.StartsWith('"') -and $argument.StartsWith('/')) {throw 'Quoted compiler switches are unsupported.'}
            if($argument -match '^/FI' -or $argument.StartsWith('@') -or $argument -cmatch '^/(?:Tp|Tc)') {
                throw 'Unreviewed compiler source/response/forced-include argument.'
            }
            $pathFlag=[regex]::Match($token,'^(?<flag>/I|/Fo|/Fd)(?<path>.*)$')
            if($pathFlag.Success) {
                $operand=$pathFlag.Groups['path'].Value
                if(-not $operand) {
                    if(++$tokenIndex -ge $tokens.Count){throw 'Missing compiler path operand.'}
                    $options.Add($token);$options.Add((Normalize-PathOperand $tokens[$tokenIndex]))
                } else {$options.Add($pathFlag.Groups['flag'].Value+(Normalize-PathOperand $operand))}
                continue
            }
            if($argument -iin $sources) {
                if($argument -ieq $source){++$ownSource}
                continue
            }
            if(-not $argument.StartsWith('/') -and [IO.Path]::GetExtension($argument) -iin @('.cpp','.c','.cc','.cxx')) {
                throw 'Unreviewed compiler source/response argument.'
            }
            $options.Add($token)
        }
        if($ownSource -ne 1) {throw "The compiler command must contain its own source exactly once for $source."}
        # Preserve every option, normalize only reviewed include/object/PDB path
        # operands, and retain external paths and macros exactly.
        [ordered]@{source=[IO.Path]::GetRelativePath($rootPath,$source);command=($options -join ' ')}
    }
    $commands=@($inventory.command | Sort-Object -Unique)
    if($commands.Count -ne 1) { throw 'Mixed per-file compiler options require a separately reviewed comparison protocol.' }
    $bytes=[Text.Encoding]::UTF8.GetBytes("redtilde-release-compiler-profile-v2`n"+$commands[0])
    return [ordered]@{schemaVersion=2;sourceRoot=$rootPath;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes));
        commands=$commands;translationUnits=@($inventory);rawSha256=(Get-FileHash $path).Hash}
}

function Assert-RedTildeDependencyComparison {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] $BaselineDependency,
        [Parameter(Mandatory)] $CandidateDependency,
        [Parameter(Mandatory)][AllowEmptyString()][string] $BaselinePublicHeadersSha256,
        [Parameter(Mandatory)][AllowEmptyString()][string] $CandidatePublicHeadersSha256,
        [switch] $DependencyAdoption
    )
    function ConvertTo-DependencyJson($Value) {
        return ConvertTo-Json -InputObject $Value -Depth 100 -Compress
    }
    function Get-DependencyField($Value,[string]$Name) {
        if($Value -is [Collections.IDictionary]) {
            foreach($key in $Value.Keys) { if([string]$key -ceq $Name) { return $Value[$key] } }
            return $null
        }
        $property=$null
        foreach($candidate in $Value.PSObject.Properties) { if($candidate.Name -ceq $Name) { $property=$candidate; break } }
        if($property) { return $property.Value }
        return $null
    }
    function Test-DependencyHasExactField($Value,[string]$Name) {
        if($Value -is [Collections.IDictionary]) {
            foreach($key in $Value.Keys) { if([string]$key -ceq $Name) { return $true } }
            return $false
        }
        foreach($candidate in $Value.PSObject.Properties) { if($candidate.Name -ceq $Name) { return $true } }
        return $false
    }
    $baselineJson=ConvertTo-DependencyJson $BaselineDependency
    $candidateJson=ConvertTo-DependencyJson $CandidateDependency
    if($baselineJson -ceq $candidateJson) {
        if($DependencyAdoption) {
            if($BaselinePublicHeadersSha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or
                $CandidatePublicHeadersSha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or
                $BaselinePublicHeadersSha256 -ine $CandidatePublicHeadersSha256) {
                throw 'Dependency adoption requires matching, valid public-header SHA-256 digests.'
            }
            $baselineIdentity=Get-DependencyField $BaselineDependency 'Identity'
            $candidateIdentity=Get-DependencyField $CandidateDependency 'Identity'
            if((Get-DependencyField $baselineIdentity 'apiRevision') -ne 4 -or
                (Get-DependencyField $candidateIdentity 'apiRevision') -ne 4) {
                throw 'Dependency adoption requires API revision 4 on both sides.'
            }
        }
        return
    }
    if(-not $DependencyAdoption) { throw 'Baseline and candidate dependency identities differ.' }

    if($BaselinePublicHeadersSha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or
        $CandidatePublicHeadersSha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or
        $BaselinePublicHeadersSha256 -ine $CandidatePublicHeadersSha256) {
        throw 'Dependency adoption requires matching, valid public-header SHA-256 digests.'
    }
    $baselineFingerprint=Get-DependencyField $BaselineDependency 'Fingerprint'
    $candidateFingerprint=Get-DependencyField $CandidateDependency 'Fingerprint'
    $baselineIdentity=Get-DependencyField $BaselineDependency 'Identity'
    $candidateIdentity=Get-DependencyField $CandidateDependency 'Identity'
    $baselineCommit=Get-DependencyField $baselineIdentity 'commit'
    $candidateCommit=Get-DependencyField $candidateIdentity 'commit'
    if($baselineFingerprint -isnot [string] -or $baselineFingerprint -cnotmatch '^[0-9a-fA-F]{64}$' -or
        $candidateFingerprint -isnot [string] -or $candidateFingerprint -cnotmatch '^[0-9a-fA-F]{64}$' -or
        $baselineCommit -isnot [string] -or $baselineCommit -cnotmatch '^[0-9a-f]{40}$' -or
        $candidateCommit -isnot [string] -or $candidateCommit -cnotmatch '^[0-9a-f]{40}$' -or
        $baselineFingerprint -ieq $candidateFingerprint -or $baselineCommit -ceq $candidateCommit -or
        (Get-DependencyField $baselineIdentity 'apiRevision') -ne 4 -or
        (Get-DependencyField $candidateIdentity 'apiRevision') -ne 4) {
        throw 'Dependency adoption metadata is missing, malformed, unchanged or has an unsupported API revision.'
    }

    # Work on JSON clones so this guard never edits either caller-owned object. Only the
    # reviewed dependency commit/fingerprint pair is omitted from the adoption comparison.
    $baselineClone=ConvertFrom-Json -InputObject $baselineJson -AsHashtable
    $candidateClone=ConvertFrom-Json -InputObject $candidateJson -AsHashtable
    if(-not ($baselineClone -is [Collections.IDictionary]) -or -not ($candidateClone -is [Collections.IDictionary]) -or
        -not (Test-DependencyHasExactField $baselineClone 'Identity') -or -not (Test-DependencyHasExactField $candidateClone 'Identity') -or
        -not ($baselineClone.Identity -is [Collections.IDictionary]) -or -not ($candidateClone.Identity -is [Collections.IDictionary]) -or
        -not (Test-DependencyHasExactField $baselineClone.Identity 'commit') -or -not (Test-DependencyHasExactField $candidateClone.Identity 'commit') -or
        -not (Test-DependencyHasExactField $baselineClone 'Fingerprint') -or -not (Test-DependencyHasExactField $candidateClone 'Fingerprint')) {
        throw 'Dependency adoption metadata must contain outer Fingerprint and nested Identity.commit fields.'
    }
    $baselineClone.Remove('Fingerprint'); $candidateClone.Remove('Fingerprint')
    $baselineClone.Identity.Remove('commit'); $candidateClone.Identity.Remove('commit')
    if((ConvertTo-DependencyJson $baselineClone) -cne (ConvertTo-DependencyJson $candidateClone)) {
        throw 'Dependency adoption changed fields beyond the pinned commit and dependency fingerprint.'
    }
}

Export-ModuleMember -Function Get-RedTildeMedian,Get-RedTildePerformanceMetrics,Get-RedTildeControlDiagnostics,New-RedTildePerformanceSchedule,Get-RedTildeSignFlipP,Compare-RedTildePerformanceBlocks,Get-RedTildeCompilerProfile,Assert-RedTildeDependencyComparison
