#Requires -Version 7.4
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{40}$')][string]$BaselineRevision,
    [Parameter(Mandatory)][string]$RecipientCertificate
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Security.Cryptography.Pkcs
$certificateBytes = [Convert]::FromBase64String($RecipientCertificate)
$certificateType = [Security.Cryptography.X509Certificates.X509Certificate2]::GetCertContentType($certificateBytes)
if ($certificateType -ne [Security.Cryptography.X509Certificates.X509ContentType]::Cert) {
    throw 'Provide an encoded public certificate in DER format.'
}
$certificate = [Security.Cryptography.X509Certificates.X509Certificate2]::new($certificateBytes)
$publicKey = [Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPublicKey($certificate)
if ($null -eq $publicKey -or $publicKey.KeySize -lt 2048) {
    throw 'The benchmark evidence recipient needs an RSA certificate of at least 2048 bits.'
}
$publicKey.Dispose()

$root = Split-Path $PSScriptRoot -Parent
$output = Join-Path $root '.build/hosted-benchmark'
$baselineRoot = Join-Path $root '.build/pb'
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $baselineRoot)) {
    throw 'Use a fresh benchmark checkout.'
}
$private = Join-Path $output 'private'
[void][IO.Directory]::CreateDirectory($private)
$env:TEMP = $env:TMP = Join-Path $output 'tmp'
[void][IO.Directory]::CreateDirectory($env:TEMP)
Import-Module (Join-Path $root 'Build/Harness.psm1') -Force
$pwsh = (Get-Command pwsh -CommandType Application | Select-Object -First 1).Source
$summary = [ordered]@{
    schemaVersion = 1
    baseline = $BaselineRevision
    candidate = ''
    verdict = 'failed'
    phase = 'source'
    evidenceSha256 = ''
}
try {
    $summary.candidate = git -C $root rev-parse HEAD
    if ($LASTEXITCODE -or $summary.candidate -cne $env:GITHUB_SHA -or $summary.candidate -ceq $BaselineRevision -or
        (git -C $root status --porcelain)) {
        throw 'Use the exact clean candidate checkout and a distinct baseline revision.'
    }
    $resolved = git -C $root rev-parse --verify "$BaselineRevision^{commit}"
    if ($LASTEXITCODE -or $resolved -cne $BaselineRevision) {
        throw 'The exact baseline is missing from the fetched history.'
    }
    $candidateProject = Get-Content -LiteralPath (Join-Path $root '.ci/project.json') -Raw | ConvertFrom-Json
    $baselineProject = (git -C $root show "${BaselineRevision}:.ci/project.json") | ConvertFrom-Json
    $engine = Join-Path $root '.ci-engine'
    if ($LASTEXITCODE -or $candidateProject.engine.ref -cne $baselineProject.engine.ref -or
        (git -C $engine rev-parse HEAD) -cne $candidateProject.engine.ref -or (git -C $engine status --porcelain)) {
        throw 'The two releases need the same clean pinned CI engine.'
    }
    git -C $root worktree add --detach $baselineRoot $BaselineRevision *> (Join-Path $private 'baseline-checkout.log')
    if ($LASTEXITCODE) { throw 'Baseline checkout failed.' }
    git -C $engine worktree add --detach (Join-Path $baselineRoot '.ci-engine') $candidateProject.engine.ref *>> (Join-Path $private 'baseline-checkout.log')
    if ($LASTEXITCODE) { throw 'Baseline engine checkout failed.' }

    foreach ($side in @('baseline', 'candidate')) {
        $summary.phase = "$side-build"
        $working = if ($side -ceq 'baseline') { $baselineRoot } else { $root }
        Push-Location -LiteralPath $working
        try {
            $arguments = @('-NoProfile', '-NonInteractive', '-File', (Join-Path $working 'build.ps1'),
                '-Platform', 'x64', '-Configuration', 'Release', '-EnableTests')
            $exitCode = Invoke-RedTildeHarness -Executable $pwsh -Arguments $arguments -LogPrefix (Join-Path $private "$side-build") -TimeoutSeconds 3000
            if ($exitCode -ne 0) { throw "The $side Release build failed with exit $exitCode." }
            Copy-Item -LiteralPath '.build/x64/Release/build-receipt.json' -Destination (Join-Path $private "$side-build.json")
            if ($side -ceq 'baseline') {
                $frozen = Join-Path $baselineRoot '.build/frozen'
                ./performance-paired.ps1 -FreezeBaseline -OutputRoot $frozen *> (Join-Path $private 'freeze.log')
                Copy-Item -LiteralPath (Join-Path $frozen 'baseline.json') -Destination (Join-Path $private 'baseline.json')
            }
        } finally {
            Pop-Location
        }
    }

    $summary.phase = 'measurement'
    $observed = Join-Path $private 'observed'
    $arguments = @('-NoProfile', '-NonInteractive', '-File', (Join-Path $root 'performance-observed.ps1'),
        '-Baseline', $frozen, '-OutputRoot', $observed, '-Warp')
    $exitCode = Invoke-RedTildeHarness -Executable $pwsh -Arguments $arguments -LogPrefix (Join-Path $private 'measure') -TimeoutSeconds 6000
    if ($exitCode -ne 0) { throw "The observed study failed with exit $exitCode." }
    $summary.phase = 'audit'
    $studies = @((Join-Path $observed 'aa'), (Join-Path $observed 'ab'))
    & (Join-Path $root 'Tests/Scope.Tests.Performance.Evidence.ps1') -StudyPath $studies -Output (Join-Path $private 'audit.json') *> (Join-Path $private 'audit.log')
    $control = Get-Content -LiteralPath (Join-Path $studies[0] 'comparison.json') -Raw | ConvertFrom-Json
    $comparison = Get-Content -LiteralPath (Join-Path $studies[1] 'comparison.json') -Raw | ConvertFrom-Json
    $hostSummary = Get-Content -LiteralPath (Join-Path $observed 'host-summary.json') -Raw | ConvertFrom-Json
    $summary.comparison = @($comparison.metrics | Select-Object scenario, metric, pairedChange, adjustedP, band, finding, controlsStable)
    $summary.quietHost = @($hostSummary | Where-Object quietCpuWindowVerified).Count -eq 2
    $controlStable = @($control.metrics | Where-Object { -not $_.controlsStable -or $_.finding -cne 'no-resolved-change' }).Count -eq 0
    $comparisonStable = @($comparison.metrics | Where-Object { -not $_.controlsStable }).Count -eq 0
    if (-not $summary.quietHost -or -not $controlStable -or -not $comparisonStable) {
        $summary.verdict = 'inconclusive'
    } elseif (@($comparison.metrics | Where-Object finding -CEQ 'regressed').Count) {
        $summary.verdict = 'regressed'
    } else {
        $summary.verdict = 'review-required'
    }
    $summary.phase = 'complete'
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $private 'failure.log')
} finally {
    $zip = Join-Path $output 'private.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory($private, $zip)
    $payload = [Security.Cryptography.Pkcs.ContentInfo]::new([IO.File]::ReadAllBytes($zip))
    $algorithm = [Security.Cryptography.Pkcs.AlgorithmIdentifier]::new([Security.Cryptography.Oid]::new('2.16.840.1.101.3.4.1.42'))
    $packet = [Security.Cryptography.Pkcs.EnvelopedCms]::new($payload, $algorithm)
    $packet.Encrypt([Security.Cryptography.Pkcs.CmsRecipient]::new($certificate))
    $encrypted = Join-Path $output 'evidence.p7m'
    [IO.File]::WriteAllBytes($encrypted, $packet.Encode())
    $summary.evidenceSha256 = (Get-FileHash -LiteralPath $encrypted).Hash
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding utf8NoBOM
    $certificate.Dispose()
}
if ($summary.verdict -cne 'review-required') {
    throw "The paired benchmark ended as $($summary.verdict) during $($summary.phase). The encrypted evidence packet is retained."
}
Write-Host 'The paired study passed its native and evidence checks. Performance acceptance remains in review.'
