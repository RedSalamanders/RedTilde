# RedTilde's package contract uses a bounded YAML mapping subset and a scoped-guidance
# inventory. Validation is local/read-only and does not load code from another repository.
#Requires -Version 7.0
Set-StrictMode -Version Latest

function Get-PackageReparseProblem([string] $Path, [string] $Root) {
    # Inspect ancestry from the owned root outward before opening a package input or target.
    $rootPath = [IO.Path]::GetFullPath($Root)
    $relative = [IO.Path]::GetRelativePath($rootPath, [IO.Path]::GetFullPath($Path))
    if ([IO.Path]::IsPathRooted($relative) -or $relative -eq '..' -or $relative.StartsWith('../') -or $relative.StartsWith('..\')) { return "Non-repository package input: $Path" }
    $component = $rootPath
    foreach ($segment in @('.') + @($relative -split '[\\/]' | Where-Object { $_ -and $_ -ne '.' })) {
        if ($segment -ne '.') { $component = [IO.Path]::Combine($component, $segment) }
        if (-not [IO.File]::Exists($component) -and -not [IO.Directory]::Exists($component)) { return $null }
        if ([IO.File]::GetAttributes($component) -band [IO.FileAttributes]::ReparsePoint) { return "Reparse package path: $component" }
    }
    return $null
}

function Read-PackageMarkdownDestination([string] $Text, [int] $Start) {
    $position = $Start
    while ($position -lt $Text.Length -and [char]::IsWhiteSpace($Text[$position])) { $position++ }
    $angle = $position -lt $Text.Length -and $Text[$position] -eq '<'
    if ($angle) { $position++ }
    $value = [Text.StringBuilder]::new()
    $depth = 0
    while ($position -lt $Text.Length) {
        $character = $Text[$position]
        if ($character -eq '\' -and $position + 1 -lt $Text.Length -and $Text[$position + 1] -in @('\','(',')','[',']','<','>')) {
            [void]$value.Append($Text[$position + 1]); $position += 2; continue
        }
        if ($angle) {
            if ($character -eq '>') { return [pscustomobject]@{ Target = $value.ToString(); End = $position + 1 } }
            if ($character -eq "`n") { return $null }
        } else {
            if ([char]::IsWhiteSpace($character) -or ($character -eq ')' -and $depth -eq 0)) { break }
            if ($character -eq '(') { $depth++ }
            elseif ($character -eq ')') { $depth-- }
        }
        [void]$value.Append($character); $position++
    }
    if ($angle -or $depth -ne 0) { return $null }
    return [pscustomobject]@{ Target = $value.ToString(); End = $position }
}

function Get-PackageMarkdownLink([string] $Text) {
    # Bounded Markdown parsing: hide fenced/code-span content, then read inline/reference
    # destinations with balanced parentheses and Markdown punctuation escapes. Never evaluate text.
    $prose = [Text.StringBuilder]::new()
    $fence = $null
    foreach ($line in $Text.Replace("`r`n", "`n") -split "`n") {
        $marker = [regex]::Match($line, '^\s*(?:(?:[-+*]|\d+[.)])\s+)?(?<fence>`{3,}|~{3,})(?<rest>.*)$')
        $hidden = $null -ne $fence
        if ($marker.Success) {
            $token = $marker.Groups['fence'].Value
            if ($null -eq $fence) { $fence = $token }
            elseif ($token[0] -ceq $fence[0] -and $token.Length -ge $fence.Length -and -not $marker.Groups['rest'].Value.Trim()) { $fence = $null }
            $hidden = $true
        }
        [void]$prose.Append($(if ($hidden) { ' ' * $line.Length } else { $line })).Append("`n")
    }
    $characters = $prose.ToString().ToCharArray()
    for ($i = 0; $i -lt $characters.Length; $i++) {
        if ($characters[$i] -ne '`') { continue }
        $slashes = 0
        for ($j = $i - 1; $j -ge 0 -and $characters[$j] -eq '\'; $j--) { $slashes++ }
        if ($slashes % 2) { continue }
        $length = 1
        while ($i + $length -lt $characters.Length -and $characters[$i + $length] -eq '`') { $length++ }
        $closing = -1
        for ($j = $i + $length; $j -lt $characters.Length; $j++) {
            # A blank line ends the paragraph; unmatched delimiters cannot hide later blocks.
            if ($characters[$j] -eq "`n") {
                $next = $j + 1
                while ($next -lt $characters.Length -and $characters[$next] -in @(' ',"`t")) { $next++ }
                if ($next -lt $characters.Length -and $characters[$next] -eq "`n") { break }
            }
            if ($characters[$j] -ne '`') { continue }
            $run = 1
            while ($j + $run -lt $characters.Length -and $characters[$j + $run] -eq '`') { $run++ }
            if ($run -eq $length) { $closing = $j + $run; break }
            $j += $run - 1
        }
        if ($closing -ge 0) {
            for ($j = $i; $j -lt $closing; $j++) { if ($characters[$j] -ne "`n") { $characters[$j] = ' ' } }
            $i = $closing - 1
        } else { $i += $length - 1 }
    }
    $textProse = [string]::new($characters)
    $lines = $textProse -split "`n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = $lines[$lineIndex]
        $definition = [regex]::Match($line, '^[ \t]{0,3}\[[^\]]+\]:[ \t]*')
        if ($definition.Success) {
            $destination = Read-PackageMarkdownDestination $line $definition.Length
            if (($null -eq $destination -or -not $destination.Target) -and $lineIndex + 1 -lt $lines.Count -and $lines[$lineIndex + 1].Trim()) {
                $destination = Read-PackageMarkdownDestination $lines[$lineIndex + 1] 0
            }
            if ($null -ne $destination -and $destination.Target) { [pscustomobject]@{ Target = $destination.Target; Line = $lineIndex + 1 } }
        }
    }
    $brackets = [Collections.Generic.Stack[int]]::new()
    for ($i = 0; $i -lt $textProse.Length; $i++) {
        $character = $textProse[$i]
        if ($character -eq '\' -and $i + 1 -lt $textProse.Length -and $textProse[$i + 1] -in @('\','[',']')) { $i++; continue }
        if ($character -eq '[') { $brackets.Push($i); continue }
        if ($character -ne ']' -or -not $brackets.Count) { continue }
        $start = $brackets.Pop()
        if ($i + 1 -ge $textProse.Length -or $textProse[$i + 1] -ne '(') { continue }
        $destination = Read-PackageMarkdownDestination $textProse ($i + 2)
        if ($null -eq $destination -or -not $destination.Target) { continue }
        $end = $destination.End
        while ($end -lt $textProse.Length -and [char]::IsWhiteSpace($textProse[$end])) { $end++ }
        if ($end -lt $textProse.Length -and $textProse[$end] -in @('"',"'",'(')) {
            $quote = if ($textProse[$end] -eq '(') { ')' } else { $textProse[$end] }; $end++
            while ($end -lt $textProse.Length -and $textProse[$end] -ne $quote) { if ($textProse[$end] -eq '\') { $end++ }; $end++ }
            $end++
            while ($end -lt $textProse.Length -and [char]::IsWhiteSpace($textProse[$end])) { $end++ }
        }
        if ($end -lt $textProse.Length -and $textProse[$end] -eq ')') {
            [pscustomobject]@{ Target = $destination.Target; Line = ($textProse.Substring(0, $start) -split "`n").Length }
            $i = $end
        }
    }
}

function ConvertFrom-PackageScalar([string] $Value) {
    if (-not $Value) { return $null }
    if ($Value.StartsWith("'")) {
        $match = [regex]::Match($Value, "^'(?<value>(?:[^']|'')*)'(?: +#.*)?$")
        if (-not $match.Success) { throw 'Invalid single-quoted scalar' }
        return $match.Groups['value'].Value.Replace("''", "'")
    }
    if ($Value.StartsWith('"')) {
        $match = [regex]::Match($Value, '^(?<value>"(?:[^"\\]|\\.)*")(?: +#.*)?$')
        if (-not $match.Success) { throw 'Invalid double-quoted scalar' }
        # JsonDocument preserves strings on every supported PowerShell 7 runtime, including ISO dates.
        $document = [System.Text.Json.JsonDocument]::Parse($match.Groups['value'].Value)
        try { return $document.RootElement.GetString() } finally { $document.Dispose() }
    }
    $valueText = [regex]::Replace($Value, ' +#.*$', '')
    if ($valueText -match '^[-?:,\[\]{}#&*!|>''"%@`]' -or $valueText.Contains(': ') -or $valueText.EndsWith(':')) { throw 'Unsupported YAML scalar; quote the value' }
    if ($valueText -match '^(?:true|false)$') { return [bool]::Parse($valueText) }
    if ($valueText -match '^(?:~|null|true|false|yes|no|on|off|[-+]?[0-9][0-9_.:eE+-]*|[-+]?\.[0-9]+|0[xX][0-9A-Fa-f]+|\.(?:inf|nan)|[0-9]{4}-[0-9]{1,2}-[0-9]{1,2})$') {
        return [pscustomobject]@{ NonStringScalar = $valueText }
    }
    return $valueText
}

function ConvertFrom-PackageMapping([string] $Text) {
    # Current package metadata needs scalar fields and one two-space-indented mapping.
    # Tags, aliases, sequences, block scalars and deeper mappings fail explicitly.
    $root = [ordered]@{}
    $section = $null
    foreach ($line in $Text.Replace("`r`n", "`n") -split "`n") {
        if (-not $line.Trim() -or $line.TrimStart().StartsWith('#')) { continue }
        $match = [regex]::Match($line, '^(?<indent>  )?(?<key>[A-Za-z_][A-Za-z0-9_-]*):(?: +(?<value>.*?))? *$')
        if (-not $match.Success) { throw 'Unsupported YAML mapping syntax' }
        $key = $match.Groups['key'].Value
        $raw = $match.Groups['value'].Value
        if ($match.Groups['indent'].Success) {
            if ($null -eq $section -or -not $raw) { throw 'Expected a scalar in an existing two-space mapping' }
            $mapping = $root[$section]
        } else {
            $mapping = $root
            $section = $null
        }
        if ($mapping.Contains($key)) { throw "Duplicate YAML key: $key" }
        if (-not $raw) {
            $mapping[$key] = [ordered]@{}
            $section = $key
        } else { $mapping[$key] = ConvertFrom-PackageScalar $raw }
    }
    return $root
}

function Get-SkillStructureProblem {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string] $FolderName, [AllowNull()][AllowEmptyString()][string] $Text)
    if (-not $Text) { return 'Missing or empty SKILL.md' }
    $match = [regex]::Match($Text.Replace("`r`n", "`n"), '\A---\n(?<header>.*?)\n---(?:\n|$)(?<body>[\s\S]*)\z', [Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) { return 'Missing YAML frontmatter fences' }
    try { $metadata = ConvertFrom-PackageMapping $match.Groups['header'].Value }
    catch { return "Invalid frontmatter: $($_.Exception.Message)" }
    foreach ($key in $metadata.Keys) {
        if ($key -cnotin @('name', 'description', 'license', 'allowed-tools', 'metadata')) { return "Unsupported frontmatter key: $key" }
    }
    if ($metadata.Contains('metadata') -and $metadata['metadata'] -isnot [Collections.IDictionary]) { return 'Optional metadata must be a mapping' }
    foreach ($key in @('license', 'allowed-tools')) {
        if ($metadata.Contains($key) -and $metadata[$key] -isnot [string]) { return "Optional $key must be a string" }
    }
    $name = if ($metadata.Contains('name')) { $metadata['name'] } else { $null }
    $description = if ($metadata.Contains('description')) { $metadata['description'] } else { $null }
    if ($name -isnot [string] -or $name -cne $FolderName -or $name.Length -gt 64 -or $name -cnotmatch '^[a-z0-9]+(?:-[a-z0-9]+)*$') { return 'Skill name must match its folder in lowercase hyphenated form' }
    if ($description -isnot [string] -or -not $description.Trim() -or $description.Length -gt 1024 -or $description -match '[<>]' -or $description.TrimStart().StartsWith('[TODO:')) { return 'Invalid or unfinished skill description' }
    if (-not $match.Groups['body'].Value.Trim()) { return 'Empty skill instructions' }
    return $null
}

function Get-PackageMarkdownProblem([string] $Path, [string] $Root) {
    $reparse = Get-PackageReparseProblem $Path $Root
    if ($reparse) { $reparse; return }
    $text = [IO.File]::ReadAllText($Path, [Text.UTF8Encoding]::new($false, $true))
    if (-not $text.Trim()) { "Empty guidance: $([IO.Path]::GetRelativePath($Root, $Path))" }
    foreach ($link in Get-PackageMarkdownLink $text) {
        $target = $link.Target
        if ($target.StartsWith('#') -or $target.StartsWith('//')) { continue }
        $fileUri = $false
        if ($target -match '^[A-Za-z][A-Za-z0-9+.-]*:' -and -not [IO.Path]::IsPathRooted($target)) {
            if ($target.StartsWith('file:', [StringComparison]::OrdinalIgnoreCase)) { $target = ([Uri]$target).LocalPath }
            else { continue }
            $fileUri = $true
        }
        if (-not $fileUri) { $target = [Uri]::UnescapeDataString($target.Split('#', 2)[0]) }
        if (-not $target) { continue }
        $resolved = [IO.Path]::GetFullPath([IO.Path]::Combine([IO.Path]::GetDirectoryName($Path), $target))
        $relative = [IO.Path]::GetRelativePath($Root, $resolved)
        if ([IO.Path]::IsPathRooted($relative) -or $relative -eq '..' -or $relative.StartsWith('../') -or $relative.StartsWith('..\')) { "Reference escapes repository in ${Path}: $target" }
        elseif (($reparse = Get-PackageReparseProblem $resolved $Root)) { $reparse }
        elseif (-not (Test-Path -LiteralPath $resolved)) { "Missing local reference in ${Path}: $target" }
    }
}

function Get-SkillPackageIntegrity {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string] $Root)
    $rootPath = [IO.Path]::GetFullPath($Root)
    $problems = [Collections.Generic.List[string]]::new()
    $markdown = [Collections.Generic.List[string]]::new()
    $actualGuidance = [Collections.Generic.List[string]]::new()
    $skills = 0
    $rootProblem = Get-PackageReparseProblem $rootPath $rootPath
    if ($rootProblem) { return [pscustomobject]@{ Problems = @($rootProblem); Skills = 0; Guidance = 0 } }
    $pending = [Collections.Generic.Stack[string]]::new()
    $pending.Push($rootPath)
    while ($pending.Count) {
        $directory = $pending.Pop()
        foreach ($entry in Get-ChildItem -LiteralPath $directory -Force) {
            if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                if ([IO.Path]::GetRelativePath($rootPath, $entry.FullName).Replace('\', '/').StartsWith('.agents/skills/', [StringComparison]::Ordinal)) { $problems.Add("Reparse skill package entry: $($entry.FullName)") }
                continue
            }
            if ($entry.PSIsContainer) {
                if ($entry.Name -in @('.git', '.build') -or (Test-Path -LiteralPath (Join-Path $entry.FullName '.git'))) { continue }
                $pending.Push($entry.FullName)
            } elseif ($entry.Name -ceq 'AGENTS.md') {
                $actualGuidance.Add([IO.Path]::GetRelativePath($rootPath, $entry.FullName).Replace('\', '/'))
                $markdown.Add($entry.FullName)
            } elseif ($entry.Extension -ieq '.md' -and [IO.Path]::GetRelativePath($rootPath, $entry.FullName).Replace('\', '/').StartsWith('.agents/skills/', [StringComparison]::Ordinal)) {
                $markdown.Add($entry.FullName)
            }
        }
    }
    $inventoryPath = Join-Path $rootPath '.agents/guidance.json'
    if (-not [IO.File]::Exists($inventoryPath)) { $problems.Add('Missing scoped-guidance inventory .agents/guidance.json') }
    else {
        try {
            $reparse = Get-PackageReparseProblem $inventoryPath $rootPath
            if ($reparse) { throw $reparse }
            $inventory = [IO.File]::ReadAllText($inventoryPath) | ConvertFrom-Json -AsHashtable
            if ($inventory.schemaVersion -ne 1 -or $inventory.files -isnot [array] -or -not $inventory.files.Count -or 'AGENTS.md' -cnotin $inventory.files) { throw 'Expected schema 1 and a nonempty files array including AGENTS.md' }
            $expected = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
            foreach ($path in $inventory.files) {
                if ($path -isnot [string] -or [IO.Path]::IsPathRooted($path) -or $path.Contains('\')) { throw 'Guidance paths must be unique repository-relative AGENTS.md paths' }
                $segments = $path.Split('/')
                if ($segments[-1] -cne 'AGENTS.md') { throw 'Guidance paths must name AGENTS.md' }
                foreach ($segment in $segments) {
                    if (-not $segment -or $segment -in @('.', '..') -or $segment.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or $segment -match '[<>:"|?*]') { throw 'Guidance paths must be unique repository-relative AGENTS.md paths' }
                }
                if (-not $expected.Add($path)) { throw 'Guidance paths must be unique repository-relative AGENTS.md paths' }
            }
            if (-not $expected.SetEquals($actualGuidance)) { $problems.Add("Scoped-guidance inventory mismatch. Expected: $($inventory.files -join ', '); found: $($actualGuidance -join ', ')") }
        } catch { $problems.Add("Invalid scoped-guidance inventory: $($_.Exception.Message)") }
    }
    $skillRoot = Join-Path $rootPath '.agents/skills'
    if (-not [IO.Directory]::Exists($skillRoot)) { $problems.Add('Missing .agents/skills directory') }
    elseif (@((Join-Path $rootPath '.agents'), $skillRoot) | Where-Object { [IO.File]::GetAttributes($_) -band [IO.FileAttributes]::ReparsePoint }) { $problems.Add('Skill package ancestry contains a reparse point') }
    else {
        foreach ($folder in Get-ChildItem -LiteralPath $skillRoot -Directory -Force) {
            if ($folder.Attributes -band [IO.FileAttributes]::ReparsePoint) { $problems.Add("Reparse skill package: $($folder.Name)"); continue }
            $skills++
            $path = Join-Path $folder.FullName 'SKILL.md'
            $reparse = Get-PackageReparseProblem $path $rootPath
            if ($reparse) { $problems.Add($reparse) }
            $text = if (-not $reparse -and [IO.File]::Exists($path)) { [IO.File]::ReadAllText($path, [Text.UTF8Encoding]::new($false, $true)) } else { $null }
            $problem = Get-SkillStructureProblem $folder.Name $text
            if ($problem) { $problems.Add("$($folder.Name): $problem") }
            $uiPath = Join-Path $folder.FullName 'agents/openai.yaml'
            if (-not [IO.File]::Exists($uiPath)) { $problems.Add("$($folder.Name): missing agents/openai.yaml") }
            else {
                try {
                    $reparse = Get-PackageReparseProblem $uiPath $rootPath
                    if ($reparse) { throw $reparse }
                    $ui = ConvertFrom-PackageMapping ([IO.File]::ReadAllText($uiPath, [Text.UTF8Encoding]::new($false, $true)))
                    foreach ($section in $ui.Keys) {
                        if ($section -cnotin @('interface', 'policy', 'dependencies') -or $ui[$section] -isnot [Collections.IDictionary]) { throw "Unsupported or non-mapping UI section: $section" }
                    }
                    if (-not $ui.Contains('interface') -or $ui['interface'] -isnot [Collections.IDictionary]) { throw 'Missing interface mapping' }
                    foreach ($key in @('display_name', 'short_description', 'default_prompt')) {
                        if (-not $ui['interface'].Contains($key) -or $ui['interface'][$key] -isnot [string] -or -not $ui['interface'][$key].Trim()) { throw "Missing or invalid interface.$key" }
                    }
                    foreach ($key in $ui['interface'].Keys) {
                        if ($ui['interface'][$key] -isnot [string] -or -not $ui['interface'][$key].Trim()) { throw "Invalid interface.$key string" }
                    }
                    if ($ui.Contains('policy')) {
                        foreach ($key in $ui['policy'].Keys) {
                            if ($key -cne 'allow_implicit_invocation' -or $ui['policy'][$key] -isnot [bool]) { throw 'UI policy supports boolean allow_implicit_invocation only' }
                        }
                    }
                    if ($ui.Contains('dependencies') -and $ui['dependencies'].Count) { throw 'Dependency sequences are outside the supported local UI metadata grammar' }
                    if ($ui['interface']['default_prompt'] -cnotmatch ('\$' + [regex]::Escape($folder.Name) + '(?![a-z0-9-])')) { throw 'Default prompt does not invoke this skill' }
                } catch { $problems.Add("$($folder.Name): invalid UI metadata: $($_.Exception.Message)") }
            }
        }
    }
    if (-not $skills) { $problems.Add('No skill packages found') }
    foreach ($path in $markdown) { foreach ($problem in Get-PackageMarkdownProblem $path $rootPath) { $problems.Add($problem) } }
    return [pscustomobject]@{ Problems = $problems.ToArray(); Skills = $skills; Guidance = $actualGuidance.Count }
}

Export-ModuleMember -Function Get-SkillStructureProblem, Get-SkillPackageIntegrity
