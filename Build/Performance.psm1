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
    if (-not $Native.completed -or $Native.schemaVersion -ne 1 -or $Native.fixture -cne 'redtilde-completed-native-v1' -or
        $Native.view -cnotin @('Compose','Split') -or $Native.renderer -cnotin @('Hardware','WARP') -or
        $Native.cycles -lt 4 -or $Native.cycles -gt 64 -or $Native.widthPx -le 0 -or $Native.heightPx -le 0 -or $Native.dpi -lt 48) {
        throw 'Incomplete or incompatible native performance report.'
    }
    $result=[ordered]@{}
    foreach($name in @('commandUs','preparationSubmissionUs','gpuWaitUs','completedEditUs','cleanCompletedUs','ownerCpuCycles')) {
        $values=@($Native.samples.$name)
        $count=if($name -eq 'cleanCompletedUs') { $Native.cycles } else { 2*$Native.cycles }
        if($values.Count -ne $count -or @($values | Where-Object { -not [double]::IsFinite($_) -or $_ -le 0 }).Count) { throw "Invalid $name samples." }
        $result[$name]=Get-RedTildeMedian $values
    }
    foreach($name in @('privateBytes','workingSetBytes')) {
        $value=[double]$Native.final.$name
        if(-not [double]::IsFinite($value) -or $value -le 0) { throw "Invalid $name." }
        $result[$name]=$value
    }
    return $result
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
        $adjusted=[math]::Min(1,[math]::Max($previous,$row.p*($sorted.Count-$index)))
        $previous=$adjusted; $row.adjustedP=$adjusted
        $row.finding=if($adjusted -le 0.05 -and $row.pairedChange -gt $row.band) { 'regressed' }
            elseif($adjusted -le 0.05 -and $row.pairedChange -lt -$row.band) { 'improved' } else { 'no-resolved-change' }
    }
    return @($rows)
}

function Get-RedTildeCompilerProfile {
    param([string]$Root)
    $path=Join-Path $Root '.build/obj/x64/Release/RedTilde/RedTilde.tlog/CL.command.1.tlog'
    if(-not (Test-Path -LiteralPath $path)) { throw 'The Release compiler command inventory is missing.' }
    [xml]$project=Get-Content -LiteralPath (Join-Path $Root 'RedTilde/RedTilde.vcxproj')
    $sources=@($project.SelectNodes('//*[local-name()="ClCompile"][@Include]') | ForEach-Object {
        [IO.Path]::GetFullPath((Join-Path (Join-Path $Root 'RedTilde') $_.Include))
    })
    $lines=@(Get-Content -LiteralPath $path)
    $commands=foreach($source in $sources) {
        $found=$null
        for($index=0;$index -lt $lines.Count-1;++$index) {
            if($lines[$index].StartsWith('^') -and $source -iin @($lines[$index].Substring(1).Split('|'))) { $found=$lines[$index+1] }
        }
        if(-not $found -or $found -match '(?i)/FI\b') { throw "Missing or unsupported compiler command for $source." }
        foreach($input in $sources) { $found=$found -ireplace ('\s+"?'+[regex]::Escape($input)+'"?(?=\s|$)'),' ' }
        $found=$found.Trim()
        $source.ToUpperInvariant()+' '+$found
    }
    $commands=@($commands | Sort-Object -Unique)
    if(-not $commands.Count) { throw 'The Release compiler command inventory is empty.' }
    $bytes=[Text.Encoding]::UTF8.GetBytes(($commands -join "`n"))
    return [ordered]@{sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes));commands=$commands;rawSha256=(Get-FileHash $path).Hash}
}

Export-ModuleMember -Function Get-RedTildeMedian,Get-RedTildePerformanceMetrics,New-RedTildePerformanceSchedule,Get-RedTildeSignFlipP,Compare-RedTildePerformanceBlocks,Get-RedTildeCompilerProfile
