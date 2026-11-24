<#
.SYNOPSIS
Validates local Markdown references in RedTilde specifications, skills and root guidance.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$files = @(
    Get-ChildItem (Join-Path $PSScriptRoot 'Specs'), (Join-Path $PSScriptRoot '.agents') -Recurse -File -Filter '*.md'
    Get-Item (Join-Path $PSScriptRoot 'README.md'), (Join-Path $PSScriptRoot 'AGENTS.md')
)
foreach ($file in $files) {
    foreach ($match in [regex]::Matches((Get-Content -LiteralPath $file.FullName -Raw), '\]\(([^)]+)\)')) {
        $target = $match.Groups[1].Value.Split('#')[0]
        if ($target -match '^(https?:|mailto:)' -or -not $target) { continue }
        if (-not (Test-Path -LiteralPath (Join-Path $file.DirectoryName $target))) {
            throw "Broken link in $($file.Name): $target"
        }
    }
}
Write-Host "Validated local Markdown links in $($files.Count) files."
