# Execute the actual entrypoint's desktop/native call branch at recording boundaries. No input or native process.
[CmdletBinding()]param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'test-interactive.ps1'),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Interactive entrypoint does not parse.'}
$assignments=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -ceq '$lease' -and $n.Right.Extent.Text.Contains('Invoke-CiInteractiveDesktopOperation')},$true))
if($assignments.Count -ne 1){throw 'Expected one shared-admission lease assignment.'}
$body=[scriptblock]::Create($assignments[0].Extent.Text)
foreach($fail in @($false,$true)){
    $probe=@{outer=0;native=0;released=$false;required=$false;parameters=$null}
    function Invoke-CiInteractiveDesktopOperation {param($RequiresInteractiveDesktop,$Operation)
        $probe.outer++;$probe.required=$RequiresInteractiveDesktop
        try{& $Operation}finally{$probe.released=$true}
    }
    function Invoke-DxUiInteractiveLease {
        param($Executable,$Runs,$Label,$EstimateSeconds,$WorkDirectory,$ChildTimeoutSeconds,[switch]$AutoStart)
        $probe.native++;$probe.parameters=$PSBoundParameters
        if($fail){throw 'controlled original native failure'}
        return [pscustomobject]@{ExitCode=0;Result='original native result'}
    }
    $leaseExe='C:/synthetic-not-launched/lease.exe';$child=[pscustomobject]@{Name='literal child';CommandLine='literal 日 "argument"'}
    $run='C:/synthetic-not-created/evidence';$InteractiveAutoStart=$true
    $rejected=$false
    try{. $body}catch{if($_.Exception.Message -cne 'controlled original native failure'){throw};$rejected=$true}
    if($probe.outer -ne 1 -or $probe.native -ne 1 -or -not $probe.required -or -not $probe.released -or $rejected -ne $fail){throw 'Actual interactive call lost admission, original failure or release ordering.'}
    $p=$probe.parameters
    if($p.Executable -cne $leaseExe -or $p.Runs[0].CommandLine -cne $child.CommandLine -or $p.ChildTimeoutSeconds -ne 120 -or $p.EstimateSeconds -ne 70 -or $p.WorkDirectory -cne $run -or -not $p.AutoStart){throw 'Actual native lease arguments or consent changed.'}
    if(-not $fail -and $lease.Result -cne 'original native result'){throw 'Shared admission replaced the native lease result.'}
}
$global:LASTEXITCODE=0
Write-Host 'PASS actual Tilde shared desktop routing, original literal arguments, consent, result and failure propagation; no input.'
