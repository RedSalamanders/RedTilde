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
    function Test-DependencyApi4($Value) {
        $api=Get-DependencyField $Value 'apiRevision'
        return (($api -is [int] -or $api -is [long]) -and $api -eq 4)
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
            if(-not (Test-DependencyApi4 $baselineIdentity) -or -not (Test-DependencyApi4 $candidateIdentity)) {
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
        -not (Test-DependencyApi4 $baselineIdentity) -or -not (Test-DependencyApi4 $candidateIdentity)) {
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

function Get-RedTildeReviewedHeaderInventory {
    param([Parameter(Mandatory)]$Headers,[Parameter(Mandatory)][string]$Side)
    $files=@($Headers.files)
    if(($Headers.schemaVersion -isnot [int] -and $Headers.schemaVersion -isnot [long]) -or $Headers.schemaVersion -ne 2 -or $files.Count -ne 58) {
        throw "The $Side reviewed inventory must be schema 2 with 58 rows."
    }
    $caseFold=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $exact=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $map=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
    $sourceRows=[Collections.Generic.List[string]]::new();$packageRows=[Collections.Generic.List[string]]::new()
    foreach($row in $files) {
        $path=[string]$row.path;$hash=[string]$row.sha256;$segments=@($path -split '/')
        if($path -cnotmatch '^(include|package-include)/[A-Za-z0-9_.+/-]+$' -or
            @($segments|Where-Object {$_ -cin @('','.','..')}).Count -or
            $hash -cnotmatch '^[0-9A-Fa-f]{64}$' -or -not $caseFold.Add($path) -or -not $exact.Add($path)) {
            throw "The $Side reviewed inventory has a malformed or duplicate row: $path"
        }
        $map.Add($path,$hash)
        $record=$path+':'+$hash
        if($path.StartsWith('include/',[StringComparison]::Ordinal)) {$sourceRows.Add($record)}else{$packageRows.Add($record)}
    }
    if($sourceRows.Count -ne 25 -or $packageRows.Count -ne 33) { throw "The $Side inventory must contain 25 source and 33 vcpkg package rows." }
    [string[]]$source=$sourceRows.ToArray();[string[]]$package=$packageRows.ToArray();[string[]]$combined=[string[]]@($source+$package)
    [Array]::Sort($source,[StringComparer]::Ordinal);[Array]::Sort($package,[StringComparer]::Ordinal);[Array]::Sort($combined,[StringComparer]::Ordinal)
    $hashRows={param([string[]]$Rows) [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($Rows -join "`n"))))}
    $sourceDigest=& $hashRows $source;$packageDigest=& $hashRows $package;$combinedDigest=& $hashRows $combined
    $expectedCombined=if($Headers.Contains('combinedSha256')){[string]$Headers.combinedSha256}else{[string]$Headers.sha256}
    if($sourceDigest -cne $Headers.sourceSha256 -or $packageDigest -cne $Headers.packageSha256 -or $combinedDigest -cne $expectedCombined) {
        throw "The $Side reviewed inventory digests do not recompute from its order-independent rows."
    }
    return [pscustomobject]@{rows=$map;sourceCount=$source.Count;packageCount=$package.Count;sourceSha256=$sourceDigest;packageSha256=$packageDigest;combinedSha256=$combinedDigest}
}

function Assert-RedTildeReviewedHeaderDeltaProfileV1 {
    param([Parameter(Mandatory)]$Profile)
    $profile=$Profile
    if($profile.schema -cne 'redtilde-reviewed-dxui-header-delta-profile-v1' -or
        $profile.profileId -cne 'redtilde-dxui-readonly-snapshot-api4-v1' -or
        $profile.status -cne 'ready-reviewed-exact-pair') {
        throw 'The fixed reviewed-header profile is blocked, unknown, or not the approved exact profile.'
    }
    if($profile.allOtherInventoryRowsMustBeIdentical -cne $true -or $profile.packageHeadersUnchanged -cne $true -or
        ($profile.dependencyFieldChanges -join '|') -cne 'Fingerprint|Identity.commit' -or
        ($profile.compilerPathOperandsOnly -join '|') -cne 'one exact DxUi source /I|one exact fingerprinted package /I') {
        throw 'The fixed profile broadens the reviewed dependency/compiler exception.'
    }
    if($profile.baseline.commit -cne '2fed82465dcee9f213753754e0e8e54f353cf38f' -or
        $profile.baseline.tree -cne '2c0641bc363bf713cfc709d721c3f6549dd5aef8' -or
        $profile.candidate.commit -cne '47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a' -or
        $profile.candidate.tree -cne '3ee2e12647ca2e05113823eb172bb0facf879021' -or
        ($profile.expectedApiRevision -isnot [int] -and $profile.expectedApiRevision -isnot [long]) -or $profile.expectedApiRevision -ne 4) {
        throw 'The fixed reviewed-header profile is not bound to its reviewed SDK pair/API.'
    }
    foreach($side in @('baseline','candidate')) {
        $binding=$profile[$side]
        if($binding.dependencyFingerprint -isnot [string] -or $binding.dependencyFingerprint -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            ($binding.publicHeaders.schemaVersion -isnot [int] -and $binding.publicHeaders.schemaVersion -isnot [long]) -or $binding.publicHeaders.schemaVersion -ne 2 -or
            $binding.publicHeaders.files -isnot [array] -or $binding.publicHeaders.files.Count -ne 58 -or
            ($binding.publicHeaders.sourceCount -isnot [int] -and $binding.publicHeaders.sourceCount -isnot [long]) -or $binding.publicHeaders.sourceCount -ne 25 -or
            ($binding.publicHeaders.packageCount -isnot [int] -and $binding.publicHeaders.packageCount -isnot [long]) -or $binding.publicHeaders.packageCount -ne 33 -or
            $binding.publicHeaders.sourceCount + $binding.publicHeaders.packageCount -ne 58 -or
            $binding.publicHeaders.sourceSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.publicHeaders.packageSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.publicHeaders.combinedSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side reviewed-header profile inventory is incomplete."
        }
        $null=Get-RedTildeReviewedHeaderInventory -Headers $binding.publicHeaders -Side $side
        if($binding.identity.commit -cne $binding.commit -or $binding.identity.apiRevision -ne 4 -or
            $binding.identity.platform -cne 'x64' -or $binding.identity.toolset -cne 'v145' -or
            -not $binding.identity.vcToolsVersion -or -not $binding.identity.windowsSdkVersion -or
            -not $binding.identity.runtimeFamily -or -not $binding.identity.preferredToolArchitecture -or
            $binding.identity.disableStlAnnotations -isnot [bool] -or
            $binding.identity.msbuildSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.compilerSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.linkerSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.windowsHeaderSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.kernel32Sha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.ucrtSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side complete SDK identity is missing or does not match the reviewed x64 API-4 toolchain."
        }
    }
    if($profile.abiProbe.protocol -cne 'dxui-readonly-source-snapshot-abi-probe-v1' -or
        $profile.abiProbe.sourceSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $profile.abiProbe.fullReceiptSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $profile.abiProbe.normalizedProbeCompileArgsEqual -cne $true -or
        $profile.abiProbe.classLayoutEqual -cne $true -or
        $profile.abiProbe.newMethodAvailableOnlyOnCandidate -cne $true -or
        $profile.abiProbe.fullApplicationCompileContextExact -isnot [bool] -or $profile.abiProbe.fullApplicationCompileContextExact -cne $false -or
        $profile.abiProbe.fullApplicationLinkContextExact -isnot [bool] -or $profile.abiProbe.fullApplicationLinkContextExact -cne $false -or
        -not $profile.abiProbe.fullContextLimit) {
        throw 'The reviewed-header ABI probe lacks its source, full receipt, required positive results, or scope limit.'
    }
    try { $probeSource=[Convert]::FromBase64String([string]$profile.abiProbe.sourceBase64) } catch { throw 'The fixed ABI probe source is not valid base64.' }
    if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($probeSource)) -cne $profile.abiProbe.sourceSha256) {
        throw 'The embedded ABI probe source bytes differ from the fixed source hash.'
    }
    $allowed=@('include/DxUi/DxUi.h','include/DxUi/RichDocument.h')
    if($profile.allowedHeaderHashPairs.Count -ne 2) { throw 'The reviewed profile must contain exactly two SDK header rows.' }
    $baseRows=(Get-RedTildeReviewedHeaderInventory -Headers $profile.baseline.publicHeaders -Side baseline).rows
    $candidateRows=(Get-RedTildeReviewedHeaderInventory -Headers $profile.candidate.publicHeaders -Side candidate).rows
    $pathDifferences=@(Compare-Object @($baseRows.Keys|Sort-Object -CaseSensitive) @($candidateRows.Keys|Sort-Object -CaseSensitive) -CaseSensitive)
    if($pathDifferences.Count -gt 0) {
        throw 'The reviewed SDK inventories do not contain the same exact path set.'
    }
    $changed=@(foreach($path in $baseRows.Keys){if($baseRows[$path] -cne $candidateRows[$path]){$path}})
    if($changed.Count -ne 2 -or @($changed|Where-Object {$allowed -cnotcontains $_}).Count -ne 0) {
        throw 'Only the two exact reviewed SDK source header rows may change; all vcpkg package rows and other SDK headers remain identical.'
    }
    $pairedSources=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach($pair in $profile.allowedHeaderHashPairs) {
        $sourcePath=[string]$pair.sourcePath
        if($sourcePath -cnotin @('include/DxUi/DxUi.h','include/DxUi/RichDocument.h') -or
            -not $pairedSources.Add($sourcePath) -or
            $pair.baselineSha256 -cne $baseRows[$sourcePath] -or
            $pair.candidateSha256 -cne $candidateRows[$sourcePath]) {
            throw 'An allowed SDK source header hash pair differs from the profile inventories.'
        }
    }
    if($pairedSources.Count -ne 2) { throw 'The reviewed profile must bind both exact source header paths once.' }
    foreach($side in @('baseline','candidate')) {
        foreach($type in @('RichTextEditor','Document')) {
            $layout=$profile.abiProbe[$side][$type]
            if(($layout.sizeBytes -isnot [int] -and $layout.sizeBytes -isnot [long]) -or $layout.sizeBytes -le 0 -or
                ($layout.alignmentBytes -isnot [int] -and $layout.alignmentBytes -isnot [long]) -or $layout.alignmentBytes -le 0) {
                throw "The reviewed-header ABI probe has no valid $side $type layout."
            }
            $layoutSizeField=if($type -ceq 'RichTextEditor'){'sizeofRichTextEditor'}else{'sizeofRichTextDocument'}
            $layoutAlignmentField=if($type -ceq 'RichTextEditor'){'alignofRichTextEditor'}else{'alignofRichTextDocument'}
            if($layout.sizeBytes -ne $profile.abiProbe[$side].values[$layoutSizeField] -or
                $layout.alignmentBytes -ne $profile.abiProbe[$side].values[$layoutAlignmentField]) {
                throw "The reviewed-header ABI $side $type layout labels differ from the compiler-probe values."
            }
        }
        $sideEvidence=$profile.abiProbe[$side];$binding=$profile[$side]
        if($sideEvidence.receiptSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.normalizedApplicationCompilerProfileSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.probeApplicationCompilerTlogSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.applicationLinkCommandSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side ABI side projection lacks compiler receipt hashes."
        }
        $projection=[ordered]@{side=$side;sdkCommit=$binding.commit;fingerprint=$binding.dependencyFingerprint;apiRevision=4;
            headerInventorySha256=$binding.publicHeaders.combinedSha256;compilerSha256=$binding.identity.compilerSha256;
            linkerSha256=$binding.identity.linkerSha256;normalizedApplicationCompilerProfileSha256=$sideEvidence.normalizedApplicationCompilerProfileSha256;
            probeApplicationCompilerTlogSha256=$sideEvidence.probeApplicationCompilerTlogSha256;
            applicationLinkCommandSha256=$sideEvidence.applicationLinkCommandSha256;values=$sideEvidence.values}
        $projectionBytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $projection -Depth 12 -Compress))
        $projectionHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($projectionBytes))
        if($projectionHash -cne $sideEvidence.receiptSha256) { throw "The $side ABI side-projection checksum does not recompute." }
    }
    if($profile.abiProbe.baseline.RichTextEditor.sizeBytes -ne $profile.abiProbe.candidate.RichTextEditor.sizeBytes -or
        $profile.abiProbe.baseline.RichTextEditor.alignmentBytes -ne $profile.abiProbe.candidate.RichTextEditor.alignmentBytes -or
        $profile.abiProbe.baseline.Document.sizeBytes -ne $profile.abiProbe.candidate.Document.sizeBytes -or
        $profile.abiProbe.baseline.Document.alignmentBytes -ne $profile.abiProbe.candidate.Document.alignmentBytes -or
        $profile.abiProbe.baseline.values.hasReadOnlySourceSynchronization -cne $false -or
        $profile.abiProbe.candidate.values.hasReadOnlySourceSynchronization -cne $true -or
        $profile.abiProbe.candidate.newMethodSignatureVerified -cne $true) {
        throw 'The reviewed-header ABI probe is incomplete or detects an ABI/layout change.'
    }
    foreach($field in @('sizeofSourceSnapshot','alignofSourceSnapshot','controlIsPolymorphic','richTextEditorIsPolymorphic','msvcVersion','msvcLang','iteratorDebugLevel')) {
        if($profile.abiProbe.baseline.values[$field] -cne $profile.abiProbe.candidate.values[$field]) { throw "The ABI probe $field differs across the reviewed pair." }
    }
    return $profile
}

function Assert-RedTildeReviewedHeaderDeltaProfileV2 {
    param([Parameter(Mandatory)]$Profile)
    $profile=$Profile
    if($profile.schema -cne 'redtilde-reviewed-dxui-header-delta-profile-v2' -or
        $profile.profileId -cne 'redtilde-dxui-parsed-snapshot-api4-v2' -or
        $profile.status -cne 'ready-reviewed-exact-pair') {
        throw 'The fixed parsed-snapshot profile is blocked, unknown, or not the approved exact profile.'
    }
    if($profile.allOtherInventoryRowsMustBeIdentical -cne $true -or $profile.packageHeadersUnchanged -cne $true -or
        ($profile.dependencyFieldChanges -join '|') -cne 'Fingerprint|Identity.commit' -or
        ($profile.compilerPathOperandsOnly -join '|') -cne 'one exact DxUi source /I|one exact fingerprinted package /I') {
        throw 'The parsed-snapshot profile broadens the reviewed dependency/compiler exception.'
    }
    if($profile.baseline.commit -cne '47a9d25995f8dc84b5ca71a2ab75ba405ed8d74a' -or
        $profile.baseline.tree -cne '3ee2e12647ca2e05113823eb172bb0facf879021' -or
        $profile.candidate.commit -cne '4077f57df8207eac6c39c43437feb45b19d23cf8' -or
        $profile.candidate.tree -cne '18be66885916f67cafd8aeb761207bf15fbd9120' -or
        ($profile.expectedApiRevision -isnot [int] -and $profile.expectedApiRevision -isnot [long]) -or $profile.expectedApiRevision -ne 4) {
        throw 'The parsed-snapshot profile is not bound to its reviewed SDK pair/API.'
    }
    foreach($side in @('baseline','candidate')) {
        $binding=$profile[$side]
        if($binding.dependencyFingerprint -isnot [string] -or $binding.dependencyFingerprint -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            ($binding.publicHeaders.schemaVersion -isnot [int] -and $binding.publicHeaders.schemaVersion -isnot [long]) -or $binding.publicHeaders.schemaVersion -ne 2 -or
            $binding.publicHeaders.files -isnot [array] -or $binding.publicHeaders.files.Count -ne 58 -or
            ($binding.publicHeaders.sourceCount -isnot [int] -and $binding.publicHeaders.sourceCount -isnot [long]) -or $binding.publicHeaders.sourceCount -ne 25 -or
            ($binding.publicHeaders.packageCount -isnot [int] -and $binding.publicHeaders.packageCount -isnot [long]) -or $binding.publicHeaders.packageCount -ne 33 -or
            $binding.publicHeaders.sourceCount + $binding.publicHeaders.packageCount -ne 58 -or
            $binding.publicHeaders.sourceSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.publicHeaders.packageSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.publicHeaders.combinedSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side parsed-snapshot profile inventory is incomplete."
        }
        $null=Get-RedTildeReviewedHeaderInventory -Headers $binding.publicHeaders -Side $side
        if($binding.identity.commit -cne $binding.commit -or $binding.identity.apiRevision -ne 4 -or
            $binding.identity.platform -cne 'x64' -or $binding.identity.toolset -cne 'v145' -or
            -not $binding.identity.vcToolsVersion -or -not $binding.identity.windowsSdkVersion -or
            -not $binding.identity.runtimeFamily -or -not $binding.identity.preferredToolArchitecture -or
            $binding.identity.disableStlAnnotations -isnot [bool] -or
            $binding.identity.msbuildSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.compilerSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.linkerSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.windowsHeaderSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.kernel32Sha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $binding.identity.ucrtSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side parsed-snapshot profile identity is incomplete or does not match the reviewed x64 API-4 toolchain."
        }
    }
    $probe=$profile.abiProbe
    if($probe.protocol -cne 'dxui-parsed-source-snapshot-source-probe-v2' -or
        $probe.sourceSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $probe.fullReceiptSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $probe.normalizedProbeCompileArgsEqual -cne $true -or
        $probe.layoutDeltaVerified -isnot [bool] -or $probe.layoutDeltaVerified -cne $true -or
        $probe.newMethodSignaturesVerified -isnot [bool] -or $probe.newMethodSignaturesVerified -cne $true -or
        $probe.fullApplicationCompileContextExact -isnot [bool] -or $probe.fullApplicationCompileContextExact -cne $false -or
        $probe.fullApplicationLinkContextExact -isnot [bool] -or $probe.fullApplicationLinkContextExact -cne $false -or
        $probe.fullContextLimit -isnot [string] -or [string]::IsNullOrWhiteSpace($probe.fullContextLimit) -or
        $probe.fullContextLimit -notmatch '(?i)isolated' -or $probe.fullContextLimit -notmatch '(?i)\bPCH\b' -or
        $probe.fullContextLimit -notmatch '(?i)\blibrary\b' -or $probe.fullContextLimit -notmatch '(?i)\b(no|not|without)\b') {
        throw 'The parsed-snapshot source probe lacks its source, full receipt, measured layout rule, method signatures, or scope limit.'
    }
    try { $probeSource=[Convert]::FromBase64String([string]$probe.sourceBase64) } catch { throw 'The parsed-snapshot probe source is not valid base64.' }
    if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($probeSource)) -cne $probe.sourceSha256) {
        throw 'The embedded parsed-snapshot probe source bytes differ from the fixed source hash.'
    }
    $allowed=@('include/DxUi/DxUi.h','include/DxUi/MarkdownSyntax.h','include/DxUi/RichDocument.h')
    if($profile.allowedHeaderHashPairs.Count -ne 3) { throw 'The parsed-snapshot profile must contain exactly three SDK source header rows.' }
    $baseRows=(Get-RedTildeReviewedHeaderInventory -Headers $profile.baseline.publicHeaders -Side baseline).rows
    $candidateRows=(Get-RedTildeReviewedHeaderInventory -Headers $profile.candidate.publicHeaders -Side candidate).rows
    if(@(Compare-Object @($baseRows.Keys|Sort-Object -CaseSensitive) @($candidateRows.Keys|Sort-Object -CaseSensitive) -CaseSensitive).Count -gt 0) {
        throw 'The parsed-snapshot SDK inventories do not contain the same exact path set.'
    }
    $changed=@(foreach($path in $baseRows.Keys){if($baseRows[$path] -cne $candidateRows[$path]){$path}})
    if($changed.Count -ne 3 -or @($changed|Where-Object {$allowed -cnotcontains $_}).Count -ne 0) {
        throw 'Only the three exact parsed-snapshot SDK source header rows may change; every other source and package row remains identical.'
    }
    $pairedSources=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach($pair in $profile.allowedHeaderHashPairs) {
        $sourcePath=[string]$pair.sourcePath
        if($sourcePath -cnotin $allowed -or -not $pairedSources.Add($sourcePath) -or
            $pair.baselineSha256 -cne $baseRows[$sourcePath] -or $pair.candidateSha256 -cne $candidateRows[$sourcePath]) {
            throw 'A parsed-snapshot SDK source header hash pair differs from the exact profile inventories.'
        }
    }
    if($pairedSources.Count -ne 3) { throw 'The parsed-snapshot profile must bind all three exact source header paths once.' }
    $layoutTypes=@('MarkdownSyntax','MarkdownParser','RichTextEditor','SourceSnapshot','Document')
    foreach($side in @('baseline','candidate')) {
        foreach($type in $layoutTypes) {
            $layout=$probe[$side][$type]
            if(($layout.sizeBytes -isnot [int] -and $layout.sizeBytes -isnot [long]) -or $layout.sizeBytes -le 0 -or
                ($layout.alignmentBytes -isnot [int] -and $layout.alignmentBytes -isnot [long]) -or $layout.alignmentBytes -le 0) {
                throw "The parsed-snapshot source probe has no valid $side $type layout."
            }
            $fieldBase=if($type -ceq 'RichTextEditor'){'RichTextEditor'}elseif($type -ceq 'SourceSnapshot'){'SourceSnapshot'}elseif($type -ceq 'Document'){'RichTextDocument'}else{$type}
            if($layout.sizeBytes -ne $probe[$side].values["sizeof$fieldBase"] -or
                $layout.alignmentBytes -ne $probe[$side].values["alignof$fieldBase"]) {
                throw "The parsed-snapshot $side $type layout labels differ from compiler-probe values."
            }
        }
        $sideEvidence=$probe[$side];$binding=$profile[$side]
        if($sideEvidence.receiptSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.normalizedApplicationCompilerProfileSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.probeApplicationCompilerTlogSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $sideEvidence.applicationLinkCommandSha256 -cnotmatch '^[0-9A-Fa-f]{64}$') {
            throw "The $side parsed-snapshot probe projection lacks compiler receipt hashes."
        }
        $projection=[ordered]@{side=$side;sdkCommit=$binding.commit;fingerprint=$binding.dependencyFingerprint;apiRevision=4;
            headerInventorySha256=$binding.publicHeaders.combinedSha256;compilerSha256=$binding.identity.compilerSha256;
            linkerSha256=$binding.identity.linkerSha256;normalizedApplicationCompilerProfileSha256=$sideEvidence.normalizedApplicationCompilerProfileSha256;
            probeApplicationCompilerTlogSha256=$sideEvidence.probeApplicationCompilerTlogSha256;
            applicationLinkCommandSha256=$sideEvidence.applicationLinkCommandSha256;values=$sideEvidence.values}
        $projectionBytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $projection -Depth 12 -Compress))
        $projectionHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($projectionBytes))
        if($projectionHash -cne $sideEvidence.receiptSha256) { throw "The $side parsed-snapshot side-projection checksum does not recompute." }
    }
    if($probe.candidate.MarkdownSyntax.sizeBytes -ne ($probe.baseline.MarkdownSyntax.sizeBytes + 16) -or
        $probe.candidate.MarkdownSyntax.alignmentBytes -ne $probe.baseline.MarkdownSyntax.alignmentBytes) {
        throw 'The parsed-snapshot probe must measure exactly a 16-byte MarkdownSyntax increase with unchanged alignment.'
    }
    foreach($type in @('MarkdownParser','RichTextEditor','SourceSnapshot','Document')) {
        if($probe.baseline[$type].sizeBytes -ne $probe.candidate[$type].sizeBytes -or
            $probe.baseline[$type].alignmentBytes -ne $probe.candidate[$type].alignmentBytes) {
            throw "The parsed-snapshot probe detects an unreviewed $type layout change."
        }
    }
    foreach($side in @('baseline','candidate')) {
        foreach($field in @('hasReadOnlySourceSynchronization','hasSourceSnapshotMatchesText',
            'hasParsedSnapshotAdoption','hasReadOnlyMarkdownSyntaxAdoption')) {
            if($probe[$side].values[$field] -isnot [bool]) { throw "The parsed-snapshot $side probe $field must be a JSON boolean." }
        }
    }
    if($probe.baseline.values.hasReadOnlySourceSynchronization -cne $true -or
        $probe.candidate.values.hasReadOnlySourceSynchronization -cne $true -or
        $probe.baseline.values.hasSourceSnapshotMatchesText -cne $false -or
        $probe.candidate.values.hasSourceSnapshotMatchesText -cne $true -or
        $probe.baseline.values.hasParsedSnapshotAdoption -cne $false -or
        $probe.candidate.values.hasParsedSnapshotAdoption -cne $true -or
        $probe.baseline.values.hasReadOnlyMarkdownSyntaxAdoption -cne $false -or
        $probe.candidate.values.hasReadOnlyMarkdownSyntaxAdoption -cne $true) {
        throw 'The parsed-snapshot probe method availability flags do not match the exact reviewed pair.'
    }
    foreach($field in @('controlIsPolymorphic','richTextEditorIsPolymorphic','msvcVersion','msvcLang','iteratorDebugLevel')) {
        if($probe.baseline.values[$field] -cne $probe.candidate.values[$field]) { throw "The parsed-snapshot probe $field differs across the reviewed pair." }
    }
    return $profile
}


# V4 remains fail-closed until a root reviewer binds the exact serialized profile bytes.
# Never replace this placeholder from profile contents or status; bind it out-of-band.
$script:ReviewedHeaderDeltaV4ProfileSha256 = '94435355531812AE70BE124E185A5D685C16A94C8052A6C1E4BAC4DD5D381997'

function Assert-RedTildeReviewedHeaderDeltaProfileV4 {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Profile,[Parameter(Mandatory)][string]$ProfileBytesSha256)
    $p=$Profile
    if($p.schema -cne 'redtilde-reviewed-dxui-header-delta-profile-v4' -or
        $p.profileId -cne 'redtilde-dxui-markdown-caption-api4-v4' -or
        $p.status -cne 'ready-reviewed-exact-pair' -or
        ($p.expectedApiRevision -isnot [int] -and $p.expectedApiRevision -isnot [long]) -or $p.expectedApiRevision -ne 4 -or
        $p.baseline.commit -cne '98c795b765a10d1805ae0a7c35a8ade535fb7fd6' -or
        $p.candidate.commit -cne '086ad82a5872768db23ea83862ab4af13dff0c5b') {
        throw 'The fixed V4 Markdown/caption profile is blocked, unknown, or not its exact reviewed API-4 pair.'
    }
    if($script:ReviewedHeaderDeltaV4ProfileSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $ProfileBytesSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
        $ProfileBytesSha256 -cne $script:ReviewedHeaderDeltaV4ProfileSha256) {
        throw 'The V4 profile has no independently root-bound immutable byte seal.'
    }
    if($p.allOtherInventoryRowsMustBeIdentical -isnot [bool] -or $p.allOtherInventoryRowsMustBeIdentical -cne $true -or
        $p.packageHeadersUnchanged -isnot [bool] -or $p.packageHeadersUnchanged -cne $true -or
        ($p.dependencyFieldChanges -join '|') -cne 'Fingerprint|Identity.commit' -or
        ($p.compilerPathOperandsOnly -join '|') -cne 'one exact DxUi source /I|one exact fingerprinted package /I') {
        throw 'V4 broadens its reviewed inventory, dependency, or compiler exception.'
    }
    $allowed=@('include/DxUi/DxUi.h','include/DxUi/RichTextLayout.h','include/DxUi/RichTextPresentation.h')
    if($p.allowedHeaderHashPairs -isnot [array] -or $p.allowedHeaderHashPairs.Count -ne 3) { throw 'V4 must bind exactly three changed source-header hash pairs.' }
    $baseRows=(Get-RedTildeReviewedHeaderInventory -Headers $p.baseline.publicHeaders -Side baseline).rows
    $candidateRows=(Get-RedTildeReviewedHeaderInventory -Headers $p.candidate.publicHeaders -Side candidate).rows
    if(@(Compare-Object @($baseRows.Keys|Sort-Object -CaseSensitive) @($candidateRows.Keys|Sort-Object -CaseSensitive) -CaseSensitive).Count -ne 0) {
        throw 'V4 baseline and candidate public-header path sets differ.'
    }
    $changed=@(foreach($path in $baseRows.Keys){if($baseRows[$path] -cne $candidateRows[$path]){$path}})
    if($changed.Count -ne 3 -or @($changed|Where-Object {$allowed -cnotcontains $_}).Count -ne 0) { throw 'V4 allows only its three exact reviewed source-header changes.' }
    $bound=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach($pair in $p.allowedHeaderHashPairs) {
        if($pair.sourcePath -cnotin $allowed -or -not $bound.Add([string]$pair.sourcePath) -or
            $pair.baselineSha256 -cne $baseRows[[string]$pair.sourcePath] -or
            $pair.candidateSha256 -cne $candidateRows[[string]$pair.sourcePath]) { throw 'A V4 changed-header pair does not match its inventory.' }
    }
    if($bound.Count -ne 3) { throw 'V4 omits a reviewed header pair.' }
    $identityFields=@('apiRevision','platform','toolset','vcToolsVersion','windowsSdkVersion','runtimeFamily','preferredToolArchitecture','disableStlAnnotations','releaseOptimizationPolicy','msbuildSha256','compilerSha256','linkerSha256','windowsHeaderSha256','kernel32Sha256','ucrtSha256')
    foreach($name in $identityFields) {
        if(-not $p.baseline.identity.Contains($name) -or -not $p.candidate.identity.Contains($name) -or
            $p.baseline.identity[$name].GetType() -ne $p.candidate.identity[$name].GetType() -or
            $p.baseline.identity[$name] -cne $p.candidate.identity[$name]) { throw "V4 changes or omits matched SDK identity field $name." }
    }
    if(@($p.baseline.identity.Keys|Where-Object {$identityFields -cnotcontains $_ -and $_ -cne 'commit'}).Count -or
        @($p.candidate.identity.Keys|Where-Object {$identityFields -cnotcontains $_ -and $_ -cne 'commit'}).Count) { throw 'V4 SDK identity contains an unreviewed field.' }
    foreach($side in @('baseline','candidate')) {
        $app=$p.appCompilerContext[$side]
        $b=$p[$side]
        if($app.target -cne 'x64' -or $app.configuration -cne 'Release' -or $app.hostArchitecture -cne 'X86' -or
            $app.normalizedProfileSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or $app.tlogSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $app.normalizedOptionsSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or $app.applicationLinkCommandSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $app.commands -isnot [array] -or $app.commands.Count -ne 1 -or $app.commands[0] -isnot [string] -or
            $app.translationUnitCount -le 0) { throw "V4 $side actual app compiler/link TLOG context is missing or mismatched." }
        try { $rawCl=[Convert]::FromBase64String([string]$app.tlogBase64); $rawLink=[Convert]::FromBase64String([string]$app.linkTlogBase64) } catch { throw "V4 $side raw app TLOG evidence is not base64." }
        if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($rawCl)) -cne $app.tlogSha256 -or
            [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($rawLink)) -cne $app.linkTlogSha256 -or
            $app.linkCommand -isnot [string] -or
            [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($app.linkCommand))) -cne $app.applicationLinkCommandSha256) {
            throw "V4 $side app TLOG/link command evidence hashes do not recompute."
        }
        $compilerBytes=[Text.Encoding]::UTF8.GetBytes("redtilde-release-compiler-profile-v2`n"+$app.commands[0])
        if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($compilerBytes)) -cne $app.normalizedProfileSha256) { throw "V4 $side normalized app compiler profile hash does not recompute from its command." }
        if($b.tree -cnotmatch '^[0-9a-f]{40}$' -or $b.dependencyFingerprint -cnotmatch '^[0-9a-fA-F]{64}$' -or
            $b.librarySha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or $b.buildReceiptSha256 -cnotmatch '^[0-9a-fA-F]{64}$' -or
            $b.identity.commit -cne $b.commit -or ($b.identity.apiRevision -isnot [int] -and $b.identity.apiRevision -isnot [long]) -or $b.identity.apiRevision -ne 4 -or $b.identity.platform -cne 'x64' -or
            $b.identity.toolset -cne 'v145' -or $b.identity.vcToolsVersion -cne '14.52.37025' -or
            $b.identity.windowsSdkVersion -cne '10.0.26100.0' -or $b.identity.runtimeFamily -cne 'MD' -or
            $b.identity.preferredToolArchitecture -cne 'x86' -or $b.identity.disableStlAnnotations -isnot [bool] -or
            $b.buildEvidence.configuration -cne 'Release' -or $b.buildEvidence.platform -cne 'x64' -or
            $b.buildEvidence.hostArchitecture -cne 'X86' -or $b.buildEvidence.passed -isnot [bool] -or $b.buildEvidence.passed -cne $true -or
            $b.buildEvidence.receiptSha256 -cne $b.buildReceiptSha256) {
            throw "The V4 $side matched Release identity/build evidence is incomplete or mismatched."
        }
        foreach($name in @('msbuildSha256','compilerSha256','linkerSha256','windowsHeaderSha256','kernel32Sha256','ucrtSha256')) {
            if($b.identity[$name] -cnotmatch '^[0-9A-Fa-f]{64}$') { throw "V4 $side identity lacks $name." }
        }
        $probe=$b.probe
        try { $sourceBytes=[Convert]::FromBase64String([string]$probe.sourceBase64); $resultBytes=[Convert]::FromBase64String([string]$probe.resultBase64) } catch { throw "V4 $side probe raw source/result is not valid base64." }
        if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($sourceBytes)) -cne $probe.sourceSha256 -or
            [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($resultBytes)) -cne $probe.resultSha256) { throw "V4 $side probe raw source/result hash does not recompute." }
        $parsedProbeResult=[Text.Encoding]::UTF8.GetString($resultBytes)|ConvertFrom-Json -AsHashtable
        if((ConvertTo-Json -InputObject $parsedProbeResult -Depth 20 -Compress) -cne (ConvertTo-Json -InputObject $probe.result -Depth 20 -Compress)) {
            throw "V4 $side probe parsed result differs from its hashed raw output."
        }
        if($probe.sourceSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or $probe.exeSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            $probe.resultSha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or $probe.protocol -cne 'dxui-header-compat-probe-v4' -or
            $probe.hostArchitecture -cne 'X86' -or $probe.librarySha256 -cne $b.librarySha256 -or
            $probe.result.profile -cne $side -or ($probe.result.apiRevision -isnot [int] -and $probe.result.apiRevision -isnot [long]) -or $probe.result.apiRevision -ne 4 -or
            $probe.result.diagnosticsEnabled -isnot [bool] -or $probe.result.diagnosticsEnabled -cne $true -or
            $probe.result.legacyCaptureLinked -isnot [bool] -or $probe.result.legacyCaptureLinked -cne $true -or
            $probe.result.newCandidateSymbolsLinked -isnot [bool] -or $probe.result.newCandidateSymbolsLinked -ne ($side -ceq 'candidate')) {
            throw "The V4 $side matched-header probe evidence is missing or mismatched."
        }
        if($side -ceq 'candidate' -and ($probe.result.markdownDefaultsAreDefault -isnot [bool] -or $probe.result.markdownDefaultsAreDefault -cne $true)) { throw 'Candidate Markdown defaults are not the documented Default style.' }
        if($side -ceq 'baseline' -and $null -ne $probe.result.markdownDefaultsAreDefault) { throw 'Baseline probe must mark the new Markdown default as not applicable.' }
        foreach($type in @('ThemePalette','ProgressBar','RichTextEditorAppearance','LayoutOptions')) {
            if($probe.result.layout[$type].size -isnot [long] -and $probe.result.layout[$type].size -isnot [int] -or
                $probe.result.layout[$type].size -le 0 -or
                $probe.result.layout[$type].align -isnot [long] -and $probe.result.layout[$type].align -isnot [int] -or
                $probe.result.layout[$type].align -le 0) { throw "V4 $side probe lacks valid $type layout data." }
        }
    }
    if($p.appCompilerContext.baseline.normalizedOptionsSha256 -cne $p.appCompilerContext.candidate.normalizedOptionsSha256) { throw 'V4 normalized app options differ outside reviewed include operands.' }
    $baseDependency=[ordered]@{Fingerprint=$p.baseline.dependencyFingerprint;Identity=$p.baseline.identity}
    $candidateDependency=[ordered]@{Fingerprint=$p.candidate.dependencyFingerprint;Identity=$p.candidate.identity}
    $baseCompiler=[ordered]@{schemaVersion=2;sha256=$p.appCompilerContext.baseline.normalizedProfileSha256;commands=$p.appCompilerContext.baseline.commands}
    $candidateCompiler=[ordered]@{schemaVersion=2;sha256=$p.appCompilerContext.candidate.normalizedProfileSha256;commands=$p.appCompilerContext.candidate.commands}
    Assert-RedTildeCompilerComparison -BaselineCompiler $baseCompiler -CandidateCompiler $candidateCompiler -BaselineDependency $baseDependency -CandidateDependency $candidateDependency -ReviewedHeaderDelta -ReviewedProfile $p
    if($p.baseline.probe.result.newButtonAppearanceValues -ne $null -or
        ($p.candidate.probe.result.newButtonAppearanceValues -join ',') -cne '4,5') { throw 'V4 candidate appended enum values are missing or baseline enum surface changed.' }
    $baselineLegacy=@($p.baseline.probe.result.legacyButtonAppearanceValues)
    $candidateLegacy=@($p.candidate.probe.result.legacyButtonAppearanceValues)
    $candidateNew=@($p.candidate.probe.result.newButtonAppearanceValues)
    if($baselineLegacy.Count -ne 4 -or $candidateLegacy.Count -ne 4 -or $candidateNew.Count -ne 2 -or
        @($baselineLegacy|Where-Object {($_ -isnot [int] -and $_ -isnot [long])}).Count -or
        @($candidateLegacy|Where-Object {($_ -isnot [int] -and $_ -isnot [long])}).Count -or
        @($candidateNew|Where-Object {($_ -isnot [int] -and $_ -isnot [long])}).Count -or
        ($baselineLegacy -join ',') -cne '0,1,2,3' -or ($candidateLegacy -join ',') -cne '0,1,2,3' -or ($candidateNew -join ',') -cne '4,5') {
        throw 'V4 probe enum values must be JSON integer arrays with stable legacy and appended candidate values.'
    }
    if($p.baseline.probe.sourceSha256 -cne $p.candidate.probe.sourceSha256 -or
        $p.baseline.probe.hostArchitecture -cne 'X86' -or $p.candidate.probe.hostArchitecture -cne 'X86' -or
        $p.baseline.probe.result.legacyButtonAppearanceValues -join ',' -cne '0,1,2,3' -or
        $p.candidate.probe.result.legacyButtonAppearanceValues -join ',' -cne '0,1,2,3') { throw 'V4 probe sources, host architecture, or legacy enum contract differ.' }
    return $p
}
function Get-RedTildeReviewedHeaderDeltaProfile {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    $path=Join-Path $Root 'Build/ReviewedHeaderDeltaProfile.json'
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw 'The fixed reviewed-header profile is missing.' }
    $profile=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -AsHashtable
    if($profile.profileId -ceq 'redtilde-dxui-readonly-snapshot-api4-v1') {
        return Assert-RedTildeReviewedHeaderDeltaProfileV1 -Profile $profile
    }
    if($profile.profileId -ceq 'redtilde-dxui-parsed-snapshot-api4-v2') {
        return Assert-RedTildeReviewedHeaderDeltaProfileV2 -Profile $profile
    }
    if($profile.profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4') {
        $raw=[IO.File]::ReadAllBytes($path)
        $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($raw))
        return Assert-RedTildeReviewedHeaderDeltaProfileV4 -Profile $profile -ProfileBytesSha256 $sha
    }
    throw 'The fixed reviewed-header profile has an unknown profile ID.'
}

function Assert-RedTildeReviewedHeaderDelta {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Profile,[Parameter(Mandatory)]$BaselineDependency,
        [Parameter(Mandatory)]$CandidateDependency,[Parameter(Mandatory)]$BaselinePublicHeaders,
        [Parameter(Mandatory)]$CandidatePublicHeaders,[Parameter(Mandatory)]$BaselineBuild,
        [Parameter(Mandatory)]$CandidateBuild)
    function Json($Value) { ConvertTo-Json -InputObject $Value -Depth 100 -Compress }
    function DependencyField($Value,[string]$Name) {
        if($Value -is [Collections.IDictionary]) { foreach($key in $Value.Keys) { if([string]$key -ceq $Name){return $Value[$key]} } }
        else { foreach($property in $Value.PSObject.Properties) { if($property.Name -ceq $Name){return $property.Value} } }
        return $null
    }
    foreach($entry in @(@{name='baseline';dependency=$BaselineDependency;headers=$BaselinePublicHeaders;build=$BaselineBuild},
        @{name='candidate';dependency=$CandidateDependency;headers=$CandidatePublicHeaders;build=$CandidateBuild})) {
        $expected=$Profile[$entry.name];$identity=DependencyField $entry.dependency 'Identity'
        $commit=DependencyField $identity 'commit';$fingerprint=DependencyField $entry.dependency 'Fingerprint'
        $api=DependencyField $identity 'apiRevision'
        if($commit -cne $expected.commit -or $fingerprint -cne $expected.dependencyFingerprint -or
            ($api -isnot [int] -and $api -isnot [long]) -or $api -ne 4 -or $entry.build.platform -cne 'x64') {
            throw "The $($entry.name) build is not the exact reviewed SDK identity."
        }
        if((Json $identity) -cne (Json $expected.identity)) { throw "The $($entry.name) complete SDK identity differs from the fixed profile." }
        $actualInventory=Get-RedTildeReviewedHeaderInventory -Headers $entry.headers -Side $entry.name
        $expectedInventory=Get-RedTildeReviewedHeaderInventory -Headers $expected.publicHeaders -Side ($entry.name+' profile')
        if($actualInventory.sourceSha256 -cne $expectedInventory.sourceSha256 -or
            $actualInventory.packageSha256 -cne $expectedInventory.packageSha256 -or
            $actualInventory.combinedSha256 -cne $expectedInventory.combinedSha256 -or
            @(Compare-Object @($actualInventory.rows.Keys|Sort-Object -CaseSensitive) @($expectedInventory.rows.Keys|Sort-Object -CaseSensitive) -CaseSensitive).Count -gt 0) {
            throw "The $($entry.name) full source/package inventory differs from the exact reviewed profile."
        }
        foreach($path in $actualInventory.rows.Keys) {
            if($actualInventory.rows[$path] -cne $expectedInventory.rows[$path]) { throw "The $($entry.name) inventory row differs from the fixed profile: $path" }
        }
    }
    $baselineClone=ConvertFrom-Json -InputObject (Json $BaselineDependency) -AsHashtable
    $candidateClone=ConvertFrom-Json -InputObject (Json $CandidateDependency) -AsHashtable
    $baselineClone.Remove('Fingerprint');$candidateClone.Remove('Fingerprint')
    $baselineClone.Identity.Remove('commit');$candidateClone.Identity.Remove('commit')
    if($Profile.profileId -ceq 'redtilde-dxui-markdown-caption-api4-v4') {
        function CanonicalJsonValue($Value) {
            if($Value -is [Collections.IDictionary]) {
                $ordered=[ordered]@{}
                foreach($key in @($Value.Keys|Sort-Object -CaseSensitive)) { $ordered[$key]=CanonicalJsonValue $Value[$key] }
                return $ordered
            }
            if($Value -is [array]) { return ,@($Value|ForEach-Object { CanonicalJsonValue $_ }) }
            return $Value
        }
        if((Json (CanonicalJsonValue $baselineClone)) -cne (Json (CanonicalJsonValue $candidateClone))) {
            throw 'V4 permits only the exact profile-bound commit/fingerprint value changes; identity property order is ignored.'
        }
    } elseif((Json $baselineClone) -cne (Json $candidateClone)) {
        throw 'Reviewed-header mode permits only the exact profile-bound commit and fingerprint changes.'
    }
}

function Get-RedTildePairedAdmissionCandidate {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Baseline,[Parameter(Mandatory)]$Current,[switch]$SameBinaryControl)
    if($SameBinaryControl) { return $Baseline }
    return $Current
}


function Assert-RedTildeCompilerComparison {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] $BaselineCompiler,
        [Parameter(Mandatory)] $CandidateCompiler,
        [Parameter(Mandatory)] $BaselineDependency,
        [Parameter(Mandatory)] $CandidateDependency,
        [switch] $DependencyAdoption,
        [switch] $ReviewedHeaderDelta,
        $ReviewedProfile
    )
    function Get-CompilerComparisonField($Value,[string]$Name) {
        if($Value -is [Collections.IDictionary]) {
            foreach($key in $Value.Keys) { if([string]$key -ceq $Name) { return $Value[$key] } }
            return $null
        }
        foreach($property in $Value.PSObject.Properties) { if($property.Name -ceq $Name) { return $property.Value } }
        return $null
    }
    function Get-AdoptionIncludePaths($Dependency) {
        $fingerprint=Get-CompilerComparisonField $Dependency 'Fingerprint'
        $identity=Get-CompilerComparisonField $Dependency 'Identity'
        $commit=Get-CompilerComparisonField $identity 'commit'
        $platform=Get-CompilerComparisonField $identity 'platform'
        if($fingerprint -isnot [string] -or $fingerprint -cnotmatch '^[0-9a-fA-F]{64}$' -or
            $commit -isnot [string] -or $commit -cnotmatch '^[0-9a-f]{40}$' -or $platform -cnotin @('x64','ARM64')) {
            throw 'Dependency adoption requires a valid DxUi fingerprint, lowercase commit and platform.'
        }
        $triplet=if($platform -ceq 'ARM64') { 'arm64-windows' } else { 'x64-windows' }
        return @{
            source='<ROOT>\.BUILD\DEPENDENCIES\DXUI\SOURCE\'+$commit+'\INCLUDE'
            package='<ROOT>\.BUILD\DEPENDENCIES\DXUI\'+$fingerprint+'\VCPKG_INSTALLED\'+$platform+'\'+$triplet+'\INCLUDE'
        }
    }
    function Normalize-AdoptionCommand([string]$Command,$ExpectedPaths) {
        if($Command -match '(?<!\\)(?:\\\\)*\\"') { throw 'Escaped compiler quotes require a separately reviewed parser.' }
        $tokenPattern='(?:[^\s"]+|"[^"]*")+'
        if([regex]::Replace($Command,$tokenPattern,'').Trim()) { throw 'Unsupported compiler command quoting.' }
        $tokens=@([regex]::Matches($Command,$tokenPattern)|ForEach-Object Value)
        $options=[Collections.Generic.List[string]]::new()
        $sourceCount=0; $packageCount=0
        for($index=0;$index -lt $tokens.Count;++$index) {
            $token=$tokens[$index]
            $separateIncludeFlag=$false
            if($token -cmatch '^/[DU]$') {
                if(++$index -ge $tokens.Count) { throw 'Missing compiler macro operand.' }
                $options.Add($token);$options.Add($tokens[$index]);continue
            }
            if($token.StartsWith('"') -and $token.Trim('"').StartsWith('/')) { throw 'Quoted compiler switches are unsupported.' }
            if($token -cmatch '^/I$') {
                if(++$index -ge $tokens.Count) { throw 'Missing compiler include operand.' }
                $flag=$token;$operand=$tokens[$index];$separateIncludeFlag=$true
            } else {
                $includeFlag=[regex]::Match($token,'^(?<flag>/I)(?<operand>.*)$')
                if(-not $includeFlag.Success) { $options.Add($token);continue }
                $flag=$includeFlag.Groups['flag'].Value;$operand=$includeFlag.Groups['operand'].Value
                if(-not $operand) { throw 'Missing compiler include operand.' }
            }
            $quoted=$operand.StartsWith('"') -and $operand.EndsWith('"')
            $path=if($quoted) { $operand.Substring(1,$operand.Length-2) } else { $operand }
            if($path.Contains('"')) { throw 'Unsupported compiler include quoting.' }
            if($path.Equals($ExpectedPaths.source,[StringComparison]::OrdinalIgnoreCase)) {
                ++$sourceCount;$marker='<DXUI_SOURCE_INCLUDE>'
            } elseif($path.Equals($ExpectedPaths.package,[StringComparison]::OrdinalIgnoreCase)) {
                ++$packageCount;$marker='<DXUI_PACKAGE_INCLUDE>'
            } else {
                if($separateIncludeFlag) { $options.Add($flag);$options.Add($operand) }
                else { $options.Add($flag+$operand) }
                continue
            }
            $normalizedOperand=if($quoted) { '"'+$marker+'"' } else { $marker }
            if($token -cmatch '^/I$') { $options.Add($flag);$options.Add($normalizedOperand) }
            else { $options.Add($flag+$normalizedOperand) }
        }
        if($sourceCount -ne 1 -or $packageCount -ne 1) {
            throw 'Each compiler command must contain exactly one declared DxUi source include and package include.'
        }
        return $options -join ' '
    }

    $baselineSha=Get-CompilerComparisonField $BaselineCompiler 'sha256'
    $candidateSha=Get-CompilerComparisonField $CandidateCompiler 'sha256'
    if($baselineSha -isnot [string] -or $candidateSha -isnot [string]) { throw 'Compiler profile SHA-256 metadata is missing.' }
    if($baselineSha -ceq $candidateSha -and -not $ReviewedHeaderDelta) { return }
    if(-not $DependencyAdoption -and -not $ReviewedHeaderDelta) { throw 'Baseline and candidate compiler profiles differ.' }
    $baselineSchema=Get-CompilerComparisonField $BaselineCompiler 'schemaVersion'
    $candidateSchema=Get-CompilerComparisonField $CandidateCompiler 'schemaVersion'
    if(($baselineSchema -isnot [int] -and $baselineSchema -isnot [long]) -or $baselineSchema -ne 2 -or
        ($candidateSchema -isnot [int] -and $candidateSchema -isnot [long]) -or $candidateSchema -ne 2 -or
        $baselineSha -cnotmatch '^[0-9a-fA-F]{64}$' -or $candidateSha -cnotmatch '^[0-9a-fA-F]{64}$') {
        throw 'Compiler adoption requires valid v2 compiler profiles.'
    }
    $baselineCommands=@(Get-CompilerComparisonField $BaselineCompiler 'commands')
    $candidateCommands=@(Get-CompilerComparisonField $CandidateCompiler 'commands')
    if($baselineCommands.Count -ne 1 -or $candidateCommands.Count -ne 1 -or
        $baselineCommands[0] -isnot [string] -or $candidateCommands[0] -isnot [string]) {
        throw 'Compiler adoption requires one homogeneous normalized command on each side.'
    }
    if($ReviewedHeaderDelta -and $null -eq $ReviewedProfile) { throw 'Reviewed-header compiler comparison requires the fixed profile.' }
    $baselinePaths=Get-AdoptionIncludePaths $BaselineDependency
    $candidatePaths=Get-AdoptionIncludePaths $CandidateDependency
    $normalizedBaseline=Normalize-AdoptionCommand $baselineCommands[0] $baselinePaths
    $normalizedCandidate=Normalize-AdoptionCommand $candidateCommands[0] $candidatePaths
    if($normalizedBaseline -cne $normalizedCandidate) {
        throw 'Compiler adoption changed options beyond the declared DxUi include operands.'
    }
}
Export-ModuleMember -Function Get-RedTildeMedian,Get-RedTildePerformanceMetrics,Get-RedTildeControlDiagnostics,New-RedTildePerformanceSchedule,Get-RedTildeSignFlipP,Compare-RedTildePerformanceBlocks,Get-RedTildeCompilerProfile,Get-RedTildeReviewedHeaderDeltaProfile,Assert-RedTildeReviewedHeaderDelta,Get-RedTildePairedAdmissionCandidate,Assert-RedTildeDependencyComparison,Assert-RedTildeCompilerComparison
