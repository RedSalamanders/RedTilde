[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$failures=[Collections.Generic.List[string]]::new()
function Check([string] $Name,[scriptblock] $Body) {
    try { & $Body; Write-Host "PASS $Name" }
    catch { $failures.Add($Name+': '+$_.Exception.Message); Write-Host ('FAIL '+$failures[-1]) -ForegroundColor Red }
}
Check 'Specifications and links' {
    $files=@(Get-ChildItem (Join-Path $PSScriptRoot 'Specs'),(Join-Path $PSScriptRoot '.agents') -Recurse -Filter '*.md')
    $files+=Get-Item (Join-Path $PSScriptRoot 'README.md'),(Join-Path $PSScriptRoot 'AGENTS.md')
    foreach ($file in $files) {
        foreach ($match in [regex]::Matches((Get-Content $file.FullName -Raw),'\]\(([^)]+)\)')) {
            $target=$match.Groups[1].Value.Split('#')[0]
            if ($target -match '^(https?:|mailto:)' -or -not $target) { continue }
            if (-not (Test-Path -LiteralPath (Join-Path $file.DirectoryName $target))) { throw "Broken link in $($file.Name): $target" }
        }
    }
    $plans=@(Get-ChildItem (Join-Path $PSScriptRoot 'Specs/Plans/WIP') -Filter '*.md' | Where-Object Name -ne 'README.md')
    $index=Get-Content (Join-Path $PSScriptRoot 'Specs/Plans/WIP/README.md') -Raw
    foreach ($plan in $plans) { if ([regex]::Matches($index,[regex]::Escape($plan.Name)).Count -ne 1) { throw "WIP index must name $($plan.Name) once." } }
}
Check 'Skills and scoped agent guidance' {
    foreach ($folder in Get-ChildItem (Join-Path $PSScriptRoot '.agents/skills') -Directory) {
        $text=Get-Content (Join-Path $folder.FullName 'SKILL.md') -Raw
        if ($text -notmatch ('(?m)^name: '+[regex]::Escape($folder.Name)+'\r?$') -or $text -notmatch '(?m)^description: .+') { throw "Invalid skill frontmatter: $($folder.Name)" }
        $yaml=Get-Content (Join-Path $folder.FullName 'agents/openai.yaml') -Raw
        if (-not $yaml.Contains('$'+$folder.Name)) { throw "Missing invocation example: $($folder.Name)" }
    }
}
Check 'Exact dependency and capability status' {
    $pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
    if ($pin.commit -cnotmatch '^[0-9a-f]{40}$' -or $pin.apiRevision -ne 4 -or @($pin.targets).Count -ne 1 -or $pin.targets[0] -ne 'DxUi') { throw 'Invalid dependency pin.' }
    $capabilities=Get-Content (Join-Path $PSScriptRoot 'capabilities.json') -Raw | ConvertFrom-Json
    if ($capabilities.status -ne 'experimental' -or @($capabilities.pending).Count -eq 0) { throw 'Experimental dependency gates must stay explicit.' }
}
Check 'Six build configurations and native inventory' {
    [xml]$project=Get-Content (Join-Path $PSScriptRoot 'RedTilde/RedTilde.vcxproj')
    foreach ($platform in @('x64','ARM64')) { foreach ($configuration in @('Debug','Release','ASan Debug')) {
        if (@($project.SelectNodes('//*[local-name()="ProjectConfiguration"]') | Where-Object Include -eq "$configuration|$platform").Count -ne 1) { throw "Missing $configuration|$platform" }
    } }
    foreach ($file in Get-ChildItem (Join-Path $PSScriptRoot 'Tests') -Filter '*.cpp') {
        if ($file.Name -notmatch '^Scope\.Tests\..+\.cpp$') { throw "Invalid active native test name: $($file.Name)" }
    }
    $inventory=Get-Content (Join-Path $PSScriptRoot 'Tests/NativeInventory.json') -Raw | ConvertFrom-Json
    $actual=@(Get-ChildItem (Join-Path $PSScriptRoot 'Tests') -Filter '*.cpp' | ForEach-Object { 'Tests/'+$_.Name })
    if (Compare-Object @($inventory.files.path) $actual) { throw 'Native test inventory does not match active files.' }
}
Check 'PowerShell syntax' {
    $files=@(Get-ChildItem $PSScriptRoot -File | Where-Object Extension -in @('.ps1','.psm1'))
    $files+=Get-ChildItem (Join-Path $PSScriptRoot 'Build') -Recurse -File | Where-Object Extension -in @('.ps1','.psm1')
    foreach ($file in $files) {
        $tokens=$null; $errors=$null
        [void][Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors)
        if ($errors.Count) { throw "$($file.Name): $($errors[0].Message)" }
    }
}
if ($failures.Count) { throw ($failures -join "`n") }
Write-Host 'All RedTilde validators passed.'
