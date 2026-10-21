[CmdletBinding()]
param([switch] $Check)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Build/Toolchain.psm1') -Force
$formatter=Get-RedTildeClangFormat
$files=@(Get-ChildItem (Join-Path $PSScriptRoot 'Common'),(Join-Path $PSScriptRoot 'RedTilde'),(Join-Path $PSScriptRoot 'Tests') -Recurse -File | Where-Object Extension -in @('.cpp','.h') | Select-Object -ExpandProperty FullName)
if ($Check) { & $formatter --dry-run --Werror @files }
else { & $formatter -i @files }
if ($LASTEXITCODE -ne 0) { throw 'C++ formatting failed.' }
