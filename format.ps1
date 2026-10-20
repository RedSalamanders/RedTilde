[CmdletBinding()]
param([switch] $Check)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$pin=Get-Content (Join-Path $PSScriptRoot 'Dependencies/DxUi.lock.json') -Raw | ConvertFrom-Json
$source=Join-Path $PSScriptRoot ".build/dependencies/DxUi/source/$($pin.commit)"
$formatter = Get-Command clang-format.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
if (-not $formatter) {
    $siblingFormatter=Join-Path (Split-Path $PSScriptRoot -Parent) 'DxUi/.build/format/clang_format/data/bin/clang-format.exe'
    if (Test-Path -LiteralPath $siblingFormatter) { $formatter=$siblingFormatter }
}
if (-not $formatter) {
    & (Join-Path $source 'Tools/Install-ClangFormat.ps1')
    $formatter=Join-Path $source '.build/format/clang_format/data/bin/clang-format.exe'
}
$files=@(Get-ChildItem (Join-Path $PSScriptRoot 'Common'),(Join-Path $PSScriptRoot 'RedTilde'),(Join-Path $PSScriptRoot 'Tests') -Recurse -File | Where-Object Extension -in @('.cpp','.h') | Select-Object -ExpandProperty FullName)
if ($Check) { & $formatter --dry-run --Werror @files }
else { & $formatter -i @files }
if ($LASTEXITCODE -ne 0) { throw 'C++ formatting failed.' }
