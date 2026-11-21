<#
.SYNOPSIS
Checks RedTilde's H-27 naming and reserved-identifier hygiene without native compilation:
root .clang-tidy configuration, private/protected _camelCase member declarations, and no
leading-_ / _Uppercase / __double reserved declarations in Common/, RedTilde/, or Tests/.
#>
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$tidyPath = Join-Path $root '.clang-tidy'
$checks = 0
$violations = [Collections.Generic.List[string]]::new()

function Require([bool]$Condition, [string]$Message) {
    if (-not $Condition) { $script:violations.Add($Message) }
    $script:checks++
}

function Strip-CppCommentsAndStrings([string]$Source) {
    # Single-pass regex preserving line counts inside multiline comments/raw strings.
    $pattern = @'
(?sx)
R"([A-Za-z0-9_]{0,16})\(.*?\)\1" |
"(?:\\.|[^"\\])*" |
'(?:\\.|[^'\\])*' |
//[^\r\n]* |
/\*.*?\*/
'@
    return [regex]::Replace($Source, $pattern, {
        param($m)
        $val = $m.Value
        $nlCount = 0
        for ($i = 0; $i -lt $val.Length; $i++) {
            if ($val[$i] -eq "`n") { $nlCount++ }
        }
        if ($nlCount -eq 0) {
            if ($val.StartsWith('"')) { return '""' }
            if ($val.StartsWith('''')) { return '''''' }
            return ' '
        }
        return ("`n" * $nlCount)
    })
}

function Find-NamingViolations([string]$Label, [string]$Source) {
    $issues = [Collections.Generic.List[string]]::new()
    $clean = Strip-CppCommentsAndStrings $Source

    # 1. Check #define macro names for reserved identifiers (__*, _Uppercase*, or leading _ at global scope)
    $lines = $clean -split "`r?`n"
    $strippedLines = [string[]]::new($lines.Count)
    $inContinuation = $false
    for ($li = 0; $li -lt $lines.Count; $li++) {
        $line = $lines[$li]
        $trimmed = $line.TrimStart()
        if ($inContinuation) {
            $inContinuation = $line.TrimEnd().EndsWith('\')
            $strippedLines[$li] = ''
            continue
        }
        if ($trimmed.StartsWith('#')) {
            if ($trimmed -match '^#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)') {
                $macroName = $Matches[1]
                if ($macroName.StartsWith('_') -or $macroName.Contains('__')) {
                    $issues.Add("${Label}:$($li + 1) defines reserved macro identifier '$macroName'.")
                }
            }
            $inContinuation = $line.TrimEnd().EndsWith('\')
            $strippedLines[$li] = ''
            continue
        }
        $strippedLines[$li] = $line
    }

    # 2. Scan tokens for class/struct private/protected fields & namespace/global declarations
    $code = $strippedLines -join "`n"
    $tokenMatches = [regex]::Matches($code, '\n|::|[A-Za-z_][A-Za-z0-9_]*|[{}();:,<>\[\]=]')

    $scopeStack = [Collections.Generic.Stack[object]]::new()
    $scopeStack.Push([pscustomobject]@{ Kind = 'namespace'; Access = 'public'; BraceDepth = 0 })
    $braceDepth = 0
    $parenDepth = 0
    $bracketDepth = 0
    $stmtTexts = [Collections.Generic.List[string]]::new()
    $stmtLines = [Collections.Generic.List[int]]::new()
    $pendingTypeKind = $null
    $lineNum = 1

    function Inspect-ClassMemberStatement($Texts, $Lines, [string]$Access) {
        if ($Texts.Count -eq 0 -or $Access -notin @('private', 'protected')) { return }
        $first = $Texts[0]
        if ($first -in @('friend', 'using', 'typedef', 'static_assert', 'template', 'enum', 'struct', 'class', 'namespace', 'public', 'private', 'protected')) { return }

        $hasStatic = $false
        $angle = 0
        $bDepth = 0
        $sqDepth = 0
        $declTexts = [Collections.Generic.List[Collections.Generic.List[string]]]::new()
        $declLines = [Collections.Generic.List[Collections.Generic.List[int]]]::new()
        $curT = [Collections.Generic.List[string]]::new()
        $curL = [Collections.Generic.List[int]]::new()
        $sawEq = $false

        for ($k = 0; $k -lt $Texts.Count; $k++) {
            $tx = $Texts[$k]
            $ln = $Lines[$k]
            if ($tx -eq 'static' -and $angle -eq 0 -and $bDepth -eq 0 -and $sqDepth -eq 0 -and $curT.Count -eq 0) {
                $hasStatic = $true
            }
            if ($tx -eq '(' -and $angle -eq 0 -and $bDepth -eq 0 -and $sqDepth -eq 0 -and -not $sawEq) {
                # Function/method/constructor/operator declaration
                return
            }
            if ($tx -eq '=' -and $angle -eq 0 -and $bDepth -eq 0 -and $sqDepth -eq 0) { $sawEq = $true }
            if ($tx -eq '{') { $bDepth++ }
            elseif ($tx -eq '}') { if ($bDepth -gt 0) { $bDepth-- } }
            elseif ($tx -eq '(') { $bDepth++ }
            elseif ($tx -eq ')') { if ($bDepth -gt 0) { $bDepth-- } }
            elseif ($tx -eq '<') { $angle++ }
            elseif ($tx -eq '>') { if ($angle -gt 0) { $angle-- } }
            elseif ($tx -eq '[') { $sqDepth++ }
            elseif ($tx -eq ']') { if ($sqDepth -gt 0) { $sqDepth-- } }

            if ($tx -eq ',' -and $angle -eq 0 -and $bDepth -eq 0 -and $sqDepth -eq 0) {
                $declTexts.Add($curT)
                $declLines.Add($curL)
                $curT = [Collections.Generic.List[string]]::new()
                $curL = [Collections.Generic.List[int]]::new()
                $sawEq = $false
            } else {
                $curT.Add($tx)
                $curL.Add($ln)
            }
        }
        if ($curT.Count -gt 0) {
            $declTexts.Add($curT)
            $declLines.Add($curL)
        }
        if ($hasStatic) { return }

        for ($di = 0; $di -lt $declTexts.Count; $di++) {
            $dt = $declTexts[$di]
            $dl = $declLines[$di]
            $lastIdent = $null
            $lastLine = 0
            $a = 0; $b = 0; $sq = 0
            for ($k = 0; $k -lt $dt.Count; $k++) {
                $tx = $dt[$k]
                if ($tx -eq '<') { $a++ }
                elseif ($tx -eq '>') { if ($a -gt 0) { $a-- } }
                elseif ($tx -eq '(') { $b++ }
                elseif ($tx -eq ')') { if ($b -gt 0) { $b-- } }
                elseif ($tx -eq '[' -and $a -eq 0 -and $b -eq 0 -and $sq -eq 0) { break }
                elseif ($tx -in @('{', '=', ':') -and $a -eq 0 -and $b -eq 0 -and $sq -eq 0) { break }
                elseif ($a -eq 0 -and $b -eq 0 -and $sq -eq 0 -and $tx -match '^[A-Za-z_][A-Za-z0-9_]*$') {
                    $lastIdent = $tx
                    $lastLine = $dl[$k]
                }
            }
            if ($null -ne $lastIdent -and $lastIdent -notin @('default', 'delete', 'override', 'final', 'noexcept', 'const', 'volatile', 'mutable')) {
                if ($lastIdent -cmatch '^_[A-Z]' -or $lastIdent.Contains('__')) {
                    $issues.Add("${Label}:$lastLine uses reserved identifier '$lastIdent' for $Access member.")
                } elseif ($lastIdent -cnotmatch '^_[a-z][A-Za-z0-9]*$') {
                    $issues.Add("${Label}:$lastLine $Access member '$lastIdent' must use _camelCase.")
                }
            }
        }
    }

    function Inspect-NamespaceDeclaration($Texts, $Lines) {
        if ($Texts.Count -eq 0) { return }
        $first = $Texts[0]
        if ($first -in @('using', 'typedef', 'friend', 'static_assert', 'template', 'namespace', 'extern', 'return')) { return }
        # Check identifier being declared at namespace/global scope (before '=', '{', '[', '(', ';')
        $angle = 0
        $lastIdent = $null
        $lastLine = 0
        $prevTok = ''
        for ($k = 0; $k -lt $Texts.Count; $k++) {
            $tx = $Texts[$k]
            if ($tx -eq '__declspec' -and ($k + 1) -lt $Texts.Count -and $Texts[$k + 1] -eq '(') {
                $k++
                $pd = 1
                while (($k + 1) -lt $Texts.Count -and $pd -gt 0) {
                    $k++
                    if ($Texts[$k] -eq '(') { $pd++ }
                    elseif ($Texts[$k] -eq ')') { $pd-- }
                }
                $prevTok = ')'
                continue
            }
            if ($tx -ceq '__cdecl' -or $tx -ceq '__stdcall' -or $tx -ceq '__fastcall' -or $tx -ceq '__thiscall' -or $tx -ceq '__vectorcall' -or $tx -ceq '__forceinline' -or $tx -ceq '__inline') {
                $prevTok = $tx
                continue
            }
            if ($tx -eq '<') { $angle++ }
            elseif ($tx -eq '>') { if ($angle -gt 0) { $angle-- } }
            elseif ($angle -eq 0 -and $tx -in @('=', '{', '[', '(', ':', ';')) { break }
            elseif ($angle -eq 0 -and $tx -match '^[A-Za-z_][A-Za-z0-9_]*$') {
                # Out-of-line member definitions like ClassName::Method have '::' right before the member name;
                # only check unqualified declarations or the first name.
                if ($prevTok -ne '::') {
                    $lastIdent = $tx
                    $lastLine = $Lines[$k]
                } else {
                    $lastIdent = $null
                }
            }
            $prevTok = $tx
        }
        if ($null -ne $lastIdent -and ($lastIdent.StartsWith('_') -or $lastIdent.Contains('__'))) {
            $issues.Add("${Label}:$lastLine declares reserved/leading-underscore identifier '$lastIdent' at namespace/file scope.")
        }
    }

    for ($ti = 0; $ti -lt $tokenMatches.Count; $ti++) {
        $tx = $tokenMatches[$ti].Value
        if ($tx -eq "`n") { $lineNum++; continue }

        if ($tx -eq '(') { $parenDepth++; $stmtTexts.Add($tx); $stmtLines.Add($lineNum); continue }
        if ($tx -eq ')') { if ($parenDepth -gt 0) { $parenDepth-- }; $stmtTexts.Add($tx); $stmtLines.Add($lineNum); continue }
        if ($tx -eq '[') { $bracketDepth++; $stmtTexts.Add($tx); $stmtLines.Add($lineNum); continue }
        if ($tx -eq ']') { if ($bracketDepth -gt 0) { $bracketDepth-- }; $stmtTexts.Add($tx); $stmtLines.Add($lineNum); continue }

        if ($parenDepth -eq 0 -and $bracketDepth -eq 0) {
            $curScope = $scopeStack.Peek()
            if ($braceDepth -eq $curScope.BraceDepth) {
                $prev = if ($stmtTexts.Count -gt 0) { $stmtTexts[$stmtTexts.Count - 1] } else { '' }
                if ($tx -in @('class', 'struct', 'namespace') -and $prev -notin @('friend', 'using', 'typedef', 'enum', '::')) {
                    $pendingTypeKind = $tx
                }
            }

            if ($tx -eq '{') {
                $braceDepth++
                $curScope = $scopeStack.Peek()
                $isAggInit = $stmtTexts.Count -gt 0 -and ($stmtTexts[$stmtTexts.Count - 1] -eq '=' -or ($stmtTexts -contains '=') -or ($stmtTexts -contains '('))
                if ($braceDepth -eq ($curScope.BraceDepth + 1) -and $null -ne $pendingTypeKind -and -not $isAggInit) {
                    $access = if ($pendingTypeKind -eq 'class') { 'private' } else { 'public' }
                    $scopeStack.Push([pscustomobject]@{ Kind = $pendingTypeKind; Access = $access; BraceDepth = $braceDepth })
                    $stmtTexts.Clear()
                    $stmtLines.Clear()
                } elseif ($curScope.Kind -eq 'namespace' -and $braceDepth -eq ($curScope.BraceDepth + 1)) {
                    # Function body at namespace scope: inspect signature before entering body, then push block scope
                    Inspect-NamespaceDeclaration $stmtTexts $stmtLines
                    $scopeStack.Push([pscustomobject]@{ Kind = 'block'; Access = 'public'; BraceDepth = $braceDepth })
                    $stmtTexts.Clear()
                    $stmtLines.Clear()
                } else {
                    $stmtTexts.Add($tx)
                    $stmtLines.Add($lineNum)
                }
                $pendingTypeKind = $null
                continue
            }

            if ($tx -eq '}') {
                $curScope = $scopeStack.Peek()
                if ($scopeStack.Count -gt 1 -and $curScope.BraceDepth -eq $braceDepth) {
                    $popped = $scopeStack.Pop()
                    if ($braceDepth -gt 0) { $braceDepth-- }
                    $pendingTypeKind = $null
                    if ($popped.Kind -in @('namespace', 'block')) {
                        $stmtTexts.Clear()
                        $stmtLines.Clear()
                    } else {
                        # Popped class/struct; clear tokens if followed by ';' or keep for trailing declarator
                        $stmtTexts.Clear()
                        $stmtLines.Clear()
                    }
                    continue
                }
                if ($braceDepth -gt 0) { $braceDepth-- }
                $pendingTypeKind = $null
                $stmtTexts.Add($tx)
                $stmtLines.Add($lineNum)
                # If an inline method body inside a class just closed back to the class brace depth, clear statement
                if ($scopeStack.Peek().Kind -in @('class', 'struct') -and $braceDepth -eq $scopeStack.Peek().BraceDepth -and ($stmtTexts -contains '(')) {
                    $stmtTexts.Clear()
                    $stmtLines.Clear()
                }
                continue
            }

            $curScope = $scopeStack.Peek()
            if ($tx -eq ':' -and $curScope.Kind -in @('class', 'struct') -and $curScope.BraceDepth -eq $braceDepth -and $stmtTexts.Count -eq 1 -and $stmtTexts[0] -in @('private', 'protected', 'public')) {
                $curScope.Access = $stmtTexts[0]
                $stmtTexts.Clear()
                $stmtLines.Clear()
                continue
            }

            if ($tx -eq ';') {
                if ($curScope.BraceDepth -eq $braceDepth) {
                    if ($curScope.Kind -in @('class', 'struct')) {
                        Inspect-ClassMemberStatement $stmtTexts $stmtLines $curScope.Access
                    } elseif ($curScope.Kind -eq 'namespace') {
                        Inspect-NamespaceDeclaration $stmtTexts $stmtLines
                    }
                    $stmtTexts.Clear()
                    $stmtLines.Clear()
                } else {
                    $stmtTexts.Add($tx)
                    $stmtLines.Add($lineNum)
                }
                $pendingTypeKind = $null
                continue
            }
        }
        $stmtTexts.Add($tx)
        $stmtLines.Add($lineNum)
    }

    return $issues
}

# 1. Verify root .clang-tidy configuration
Require (Test-Path -LiteralPath $tidyPath -PathType Leaf) 'Missing repository-root .clang-tidy.'
if (Test-Path -LiteralPath $tidyPath -PathType Leaf) {
    $tidyText = [IO.File]::ReadAllText($tidyPath, [Text.UTF8Encoding]::new($false))
    Require ($tidyText -match '(?m)^Checks:\s*[''"]?[^''"\r\n]*bugprone-reserved-identifier' -and $tidyText -match '(?m)^Checks:\s*[''"]?[^''"\r\n]*readability-identifier-naming') '.clang-tidy Checks must enable bugprone-reserved-identifier and readability-identifier-naming.'
    Require ($tidyText -match '(?m)^WarningsAsErrors:\s*[''"]?[^''"\r\n]*bugprone-reserved-identifier' -and $tidyText -match '(?m)^WarningsAsErrors:\s*[''"]?[^''"\r\n]*readability-identifier-naming') '.clang-tidy WarningsAsErrors must enforce bugprone-reserved-identifier and readability-identifier-naming.'
    Require ($tidyText -match '(?m)^HeaderFilterRegex:\s*[''"]?[^''"\r\n]*Common[^''"\r\n]*RedTilde[^''"\r\n]*Tests') '.clang-tidy HeaderFilterRegex must cover Common, RedTilde, and Tests.'
    foreach ($pair in @(
        @{ Key = 'readability-identifier-naming.PrivateMemberCase'; Value = 'camelBack' },
        @{ Key = 'readability-identifier-naming.PrivateMemberPrefix'; Value = '_' },
        @{ Key = 'readability-identifier-naming.ProtectedMemberCase'; Value = 'camelBack' },
        @{ Key = 'readability-identifier-naming.ProtectedMemberPrefix'; Value = '_' }
    )) {
        $escapedKey = [regex]::Escape($pair.Key)
        $escapedVal = [regex]::Escape($pair.Value)
        $pattern = "(?ms)-\s*key:\s*$escapedKey\s+value:\s*['""]?$escapedVal['""]?(?:\r?$)"
        Require ($tidyText -match $pattern) ".clang-tidy CheckOptions must set $($pair.Key) to $($pair.Value)."
    }
}

# 2. Gate self-checks: verify forbidden forms fail and valid forms pass.
$selfBadCases = @(
    @{ Name = 'trailing underscore private'; Code = "class Foo {`nprivate:`n    int count_;`n};" },
    @{ Name = 'bare private'; Code = "class Foo {`n    int count{};`n};" },
    @{ Name = 'uppercase underscore protected'; Code = "struct Bar {`nprotected:`n    int _Count{};`n};" },
    @{ Name = 'double underscore member'; Code = "class Baz {`nprivate:`n    int __value{};`n};" },
    @{ Name = 'namespace leading underscore'; Code = "namespace Test {`nint _globalCount = 0;`n}" },
    @{ Name = 'reserved macro'; Code = "#define __MY_MACRO 1`n" }
)
foreach ($case in $selfBadCases) {
    $found = @(Find-NamingViolations 'self-test' $case.Code)
    Require ($found.Count -ge 1) "Naming style gate missed '$($case.Name)'."
}

$selfGoodCode = @"
namespace RedTilde {
struct PublicDto {
    int width{};
    int height{};
};
class Sample final {
public:
    static constexpr int MaxItems = 10;
    explicit Sample(int value) : _value(value) {}
    void Helper() const noexcept {}
private:
    int _value{};
    std::vector<int> _items;
    std::function<void(int)> _callback;
};
}
"@
$goodFound = @(Find-NamingViolations 'self-good' $selfGoodCode)
Require ($goodFound.Count -eq 0) "Naming style gate false positive on valid C++: $($goodFound -join '; ')"

# 3. Live scan of Common/, RedTilde/, Tests/
$sourceDirs = @('Common', 'RedTilde', 'Tests') | ForEach-Object { Join-Path $root $_ } | Where-Object { Test-Path -LiteralPath $_ -PathType Container }
$sourceFiles = @(Get-ChildItem -Path $sourceDirs -Recurse -File | Where-Object { $_.Extension -in @('.h', '.cpp') })
foreach ($file in $sourceFiles) {
    $rel = [IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
    $content = [IO.File]::ReadAllText($file.FullName, [Text.UTF8Encoding]::new($false))
    $fileIssues = @(Find-NamingViolations $rel $content)
    foreach ($issue in $fileIssues) {
        $violations.Add($issue)
    }
    $checks++
}

if ($violations.Count) {
    throw "Naming style gate failed ($($violations.Count) violation(s)):`n$($violations -join "`n")"
}
Write-Host "PASS: naming style ($checks checks, $($sourceFiles.Count) C++ files)"
