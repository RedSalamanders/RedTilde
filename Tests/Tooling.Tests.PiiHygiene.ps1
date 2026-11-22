Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot

function Test-RedTildePiiText {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Content,
        [string]$LocalUser = '',
        [string]$LocalHost = ''
    )

    $violations = [System.Collections.Generic.List[string]]::new()
    $normalizedPath = $RelativePath.Replace('\', '/')
    $isThirdPartyLicense = ($normalizedPath -like 'ThirdParty/*') -or
        ($normalizedPath -match '(?i)(?:^|/)(?:LICENSE(?:\.[^/]+)?|THIRD-PARTY-NOTICES\.md)$')
    $isCodeOwners = ($normalizedPath -ceq '.github/CODEOWNERS')

    $lines = $Content -split "\r?\n"
    for ($index = 0; $index -lt $lines.Length; $index++) {
        $line = $lines[$index]
        $lineNumber = $index + 1

        # 1. Hardcoded Windows user profile directory paths (C:\Users\<real-user>, D:\Users\..., etc.)
        if ($line -match '(?i)(?:^|[^A-Za-z0-9_])[A-Z]:[/\\]+Users[/\\]+(?!(?:Public|Default|All Users|example-user|\.\.\.|…|<[^>]+>)(?:[/\\''"`\s>,;)]|$))([A-Za-z0-9._-]+)') {
            $violations.Add("${normalizedPath}:${lineNumber}: hardcoded Windows user profile path (C:\Users\$($Matches[1]))")
        }

        # 2. Non-synthetic corporate/personal OneDrive - <Org> folder names (hyphen with surrounding spaces, not identifier tokens like onedrive-selftest)
        if ($line -match '(?i)\bOneDrive\s+-\s+(?!(?:Personal|Test|Example)\b)([A-Za-z0-9._-]+)') {
            $violations.Add("${normalizedPath}:${lineNumber}: non-synthetic OneDrive organization path (OneDrive - $($Matches[1]))")
        }

        # 3. Non-example SMB/UNC host\share literals (excluding *.example.invalid / example.test / generic placeholders / WSL)
        if ($line -match '(?:^|[\s''"`(])\\\\(?!(?:files\.example\.test|fileserver\.example\.invalid|server|host|share|wsl\.localhost|wsl\$|\?|\.)[\\])([A-Za-z0-9][A-Za-z0-9_.-]*)\\([A-Za-z0-9_.$ -]+)') {
            if ($Matches[1] -notmatch '^(?:<[^>]+>|[^\\]+example\.invalid)$' -and $line -notmatch '\\begin\{') {
                $violations.Add("${normalizedPath}:${lineNumber}: non-synthetic UNC share (\\$($Matches[1])\$($Matches[2]))")
            }
        }

        # 4. Credentials / tokens / private key blocks
        if ($line -match '(?:ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}|BEGIN (?:[A-Z ]+ )?PRIVATE KEY)') {
            $violations.Add("${normalizedPath}:${lineNumber}: credential token or private key marker")
        }

        # 5. Non-synthetic email addresses (outside third-party attribution/license files)
        if (-not $isThirdPartyLicense) {
            foreach ($m in [regex]::Matches($line, '(?i)\b([A-Za-z0-9._%+-]+)@([A-Za-z0-9.-]+\.[A-Za-z]{2,})\b')) {
                $domain = $m.Groups[2].Value.ToLowerInvariant()
                if ($domain -notin @('example.com', 'example.org', 'example.net', 'example.invalid', 'example.test', 'users.noreply.github.com')) {
                    $violations.Add("${normalizedPath}:${lineNumber}: non-synthetic email domain (@$domain)")
                }
            }
        }

        # 6. Dynamic local OS username and computer name (catches ambient developer machine leaks without hardcoding PII here)
        if (-not [string]::IsNullOrWhiteSpace($LocalUser) -and $LocalUser.Length -ge 3) {
            $genericUsers = @('user', 'admin', 'administrator', 'runner', 'guest', 'test', 'system', 'default', 'defaultaccount', 'wdagutilityaccount', 'containeradministrator')
            if ($genericUsers -notcontains $LocalUser.ToLowerInvariant() -and -not $isCodeOwners) {
                $nextLine = if ($index + 1 -lt $lines.Length) { $lines[$index + 1] } else { '' }
                $isGitHubNoreply = ($line -match '@users\.noreply\.github\.com') -or ($nextLine -match '@users\.noreply\.github\.com')
                if (-not $isGitHubNoreply -and $line -match "(?i)\b$([regex]::Escape($LocalUser))\b") {
                    $violations.Add("${normalizedPath}:${lineNumber}: ambient local username reference")
                }
            }
        }

        if (-not [string]::IsNullOrWhiteSpace($LocalHost) -and $LocalHost.Length -ge 4) {
            $genericHosts = @('localhost', 'runnervm', 'winhost', 'desktop', 'workstation')
            if ($genericHosts -notcontains $LocalHost.ToLowerInvariant()) {
                if ($line -match "(?i)\b$([regex]::Escape($LocalHost))\b") {
                    $violations.Add("${normalizedPath}:${lineNumber}: ambient local hostname reference")
                }
            }
        }
    }

    return @($violations)
}

# 1. Self-tests (positive rejections built dynamically so this file itself stays PII-free, plus benign allowances)
$selfChecks = 0

$badProfile = 'C:\Users\' + 'alice' + '\Desktop\selftest'
$badOrg = 'OneDrive - ' + 'AcmeCorp'
$badShare = '\\' + 'mynas01' + '\Multimedia'
$badEmail = 'alice@' + 'privatecorp.net'
$badKey = '-----BEGIN ' + 'RSA PRIVATE KEY-----'
$badHostText = 'Build ran on WIN99DEVBOX01 yesterday.'

foreach ($probe in @(
    @{ Name = 'hardcoded user path'; Text = $badProfile; User = ''; HostName = '' },
    @{ Name = 'corporate OneDrive'; Text = $badOrg; User = ''; HostName = '' },
    @{ Name = 'real UNC share'; Text = $badShare; User = ''; HostName = '' },
    @{ Name = 'non-example email'; Text = $badEmail; User = ''; HostName = '' },
    @{ Name = 'private key marker'; Text = $badKey; User = ''; HostName = '' },
    @{ Name = 'ambient user name'; Text = 'path for alice_dev here'; User = 'alice_dev'; HostName = '' },
    @{ Name = 'ambient computer name'; Text = $badHostText; User = ''; HostName = 'WIN99DEVBOX01' }
)) {
    $found = @(Test-RedTildePiiText -RelativePath 'Tests/synthetic.ps1' -Content $probe.Text -LocalUser $probe.User -LocalHost $probe.HostName)
    if ($found.Count -lt 1) {
        throw "PII hygiene self-test failed to reject $($probe.Name)."
    }
    $selfChecks++
}

$allowedBenign = @(
    'C:\Users\example-user\Desktop\selftest',
    '%USERPROFILE%\OneDrive - <Org>',
    'OneDrive - Personal',
    'OneDrive - Test',
    '\\fileserver.example.invalid\Multimedia',
    '\\files.example.test\share\cover.bmp',
    '\\<nas-host>\<selftest-share>\selftest',
    'writer@example.invalid',
    '1000+someone@users.noreply.github.com'
) -join "`n"
$benignFindings = @(Test-RedTildePiiText -RelativePath 'Tests/synthetic.ps1' -Content $allowedBenign -LocalUser 'alice_dev' -LocalHost 'WIN99DEVBOX01')
if ($benignFindings.Count -ne 0) {
    throw "PII hygiene self-test falsely flagged allowed synthetic inputs:`n$($benignFindings -join "`n")"
}
$selfChecks++

# 2. Scan active repository text files (excluding build outputs, binary assets, changelog, frozen Measurements/ sealed profile fixtures)
$filesRaw = & git -C $root ls-files -z --cached --others --exclude-standard
if ($LASTEXITCODE -ne 0) {
    throw "git ls-files failed while enumerating files for PII scan ($LASTEXITCODE)."
}

$candidateFiles = @($filesRaw.Split([char]0) | Where-Object {
    $_ -and
    $_ -notmatch '^\.build/' -and
    $_ -notmatch '^\.ci-engine/' -and
    $_ -notmatch '^Measurements/' -and
    $_ -cne 'Build/ReviewedHeaderDeltaProfile.json' -and
    $_ -cne 'Tests/Fixtures/ReviewedHeaderDeltaProfile.v4.json' -and
    $_ -cne 'CHANGELOG.md' -and
    $_ -notmatch '(?i)\.(png|ico|exe|dll|lib|pdb|zip|bmp|jpg|jpeg|gif|tif|tiff|bin)$'
} | Sort-Object -Unique)

$localUser = if ($env:USERNAME) { [string]$env:USERNAME } else { '' }
$localHost = if ($env:COMPUTERNAME) { [string]$env:COMPUTERNAME } else { '' }
$allViolations = [System.Collections.Generic.List[string]]::new()

foreach ($rel in $candidateFiles) {
    $full = Join-Path $root $rel
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        continue
    }
    $text = [IO.File]::ReadAllText($full, [Text.UTF8Encoding]::new($false))
    foreach ($v in @(Test-RedTildePiiText -RelativePath $rel -Content $text -LocalUser $localUser -LocalHost $localHost)) {
        $allViolations.Add($v)
    }
}

if ($allViolations.Count -gt 0) {
    throw "PII hygiene violations detected:`n$($allViolations -join "`n")"
}

Write-Host "PASS: PII hygiene ($($candidateFiles.Count) files scanned, $selfChecks self-checks)"
