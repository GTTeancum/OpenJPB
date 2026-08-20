[CmdletBinding()]
param(
    [string]$OutputRoot = '',
    [string]$DocsRoot = '',
    [string]$ReportPath = '',
    [string[]]$RunDirectory = @('boss-smoke', 'boss-smoke-hardware'),
    [switch]$AllowRawCaptures,
    [switch]$NoReport
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repoRoot 'out'
}
if ([string]::IsNullOrWhiteSpace($DocsRoot)) {
    $DocsRoot = Join-Path $repoRoot 'docs'
}

$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$DocsRoot = [System.IO.Path]::GetFullPath($DocsRoot)
if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    $ReportPath = Join-Path $DocsRoot 'BOSS_PROOF_VERIFICATION.md'
}
$ReportPath = [System.IO.Path]::GetFullPath($ReportPath)

function Assert-File {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -le 0) {
        throw "$Description is empty: $Path"
    }
    return $item
}

function Assert-Equal {
    param(
        [object]$Actual,
        [object]$Expected,
        [string]$Description
    )

    if ([string]$Actual -ne [string]$Expected) {
        throw "$Description is '$Actual', expected '$Expected'."
    }
}

function Get-LedgerPath {
    param([string]$RunName)

    switch ($RunName) {
        'boss-smoke' { return Join-Path $DocsRoot 'BOSS_SMOKE_AUDIT.md' }
        'boss-smoke-hardware' { return Join-Path $DocsRoot 'BOSS_HARDWARE_SMOKE_AUDIT.md' }
        default { return $null }
    }
}

function Get-ShortHash {
    param([string]$Path)

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.Substring(0, 16)
}

$summaries = @()
foreach ($runName in $RunDirectory) {
    $runPath = Join-Path $OutputRoot $runName
    if (-not (Test-Path -LiteralPath $runPath -PathType Container)) {
        throw "Boss proof run directory not found: $runPath"
    }

    $resultsPath = Join-Path $runPath 'results.json'
    $contactSheetPath = Join-Path $runPath 'contact-sheet.png'
    $manifestPath = Join-Path $runPath 'contact-sheet.manifest.json'
    $resultsFile = Assert-File $resultsPath 'Boss smoke result JSON'
    $contactSheet = Assert-File $contactSheetPath 'Boss smoke contact sheet'
    $manifestFile = Assert-File $manifestPath 'Boss smoke contact-sheet manifest'

    $parsedResults = Get-Content -LiteralPath $resultsFile.FullName -Raw | ConvertFrom-Json
    $results = @($parsedResults | ForEach-Object { $_ })
    $manifest = Get-Content -LiteralPath $manifestFile.FullName -Raw | ConvertFrom-Json
    $entries = @($manifest.entries)
    if ($results.Count -le 0) {
        throw "Boss smoke result JSON has no entries: $resultsPath"
    }
    Assert-Equal $manifest.entry_count $results.Count "$runName manifest entry_count"
    Assert-Equal $entries.Count $results.Count "$runName manifest entry count"
    Assert-Equal $manifest.width 640 "$runName contact sheet width"
    $expectedHeight = 180 * [int][Math]::Ceiling($results.Count / 2.0)
    Assert-Equal $manifest.height $expectedHeight "$runName contact sheet height"
    Assert-Equal $manifest.contact_sheet $contactSheet.FullName "$runName manifest contact_sheet path"

    for ($i = 0; $i -lt $results.Count; $i++) {
        $result = $results[$i]
        Assert-Equal $entries[$i] $result.Name "$runName manifest entry $i"
        Assert-Equal $result.Status 'PASS' "$runName $($result.Name) status"
        Assert-Equal $result.PlacementStatus '1' "$runName $($result.Name) placement status"
        Assert-Equal $result.RuntimePlacement $result.PlacementId "$runName $($result.Name) runtime placement"
        if ([int]$result.VisibleFrames -le 0) {
            throw "$runName $($result.Name) has no visible frames."
        }
        if ([int64]$result.CaptureBytes -le 32) {
            throw "$runName $($result.Name) capture byte count is too small: $($result.CaptureBytes)"
        }
    }

    $rawPpmCount = (Get-ChildItem -LiteralPath $runPath -File -Filter '*.ppm' -ErrorAction SilentlyContinue |
        Measure-Object).Count
    if ($rawPpmCount -gt 0 -and -not $AllowRawCaptures) {
        throw "$runName has $rawPpmCount raw PPM capture(s); rerun cleanup or pass -AllowRawCaptures."
    }

    $ledgerPath = Get-LedgerPath $runName
    $ledgerStatus = 'not checked'
    if ($null -ne $ledgerPath) {
        $ledgerFile = Assert-File $ledgerPath 'Boss smoke audit ledger'
        $ledgerText = Get-Content -LiteralPath $ledgerFile.FullName -Raw
        if ($ledgerText -notmatch [regex]::Escape($contactSheet.FullName)) {
            throw "$runName ledger does not reference contact sheet: $($contactSheet.FullName)"
        }
        if ($ledgerText -notmatch [regex]::Escape($manifestFile.FullName)) {
            throw "$runName ledger does not reference contact-sheet manifest: $($manifestFile.FullName)"
        }
        if ($ledgerText -notmatch ([regex]::Escape("result: ``$($results.Count) passed / 0 failed``"))) {
            throw "$runName ledger does not report $($results.Count) passed / 0 failed."
        }
        $ledgerStatus = 'PASS'
    }

    $summaries += [pscustomobject]@{
        Run = $runName
        ResultEntries = $results.Count
        ContactSheetKB = [math]::Round($contactSheet.Length / 1KB, 1)
        ManifestEntries = $entries.Count
        ContactSheet = $contactSheet.FullName
        ContactSheetSha256 = Get-ShortHash $contactSheet.FullName
        Manifest = $manifestFile.FullName
        ManifestSha256 = Get-ShortHash $manifestFile.FullName
        Results = $resultsFile.FullName
        ResultsSha256 = Get-ShortHash $resultsFile.FullName
        RawPpmCount = $rawPpmCount
        Ledger = $ledgerStatus
        Status = 'PASS'
    }
}

if (-not $NoReport) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ReportPath) | Out-Null
    $now = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('# Boss Proof Verification')
    $lines.Add('')
    $lines.Add("Generated: $now")
    $lines.Add('')
    $lines.Add("Output root: ``$OutputRoot``")
    $lines.Add('')
    $lines.Add('This report verifies retained boss proof artifacts without rerunning the game.')
    $lines.Add('It checks result JSON pass status, exact runtime placement, contact-sheet manifest order/count, contact-sheet dimensions, audit-ledger references, raw PPM cleanup, and retained artifact hashes.')
    $lines.Add('')
    $lines.Add('| Run | Status | Entries | Contact sheet | Manifest | Results | Raw PPMs | Ledger |')
    $lines.Add('| --- | --- | --- | --- | --- | --- | --- | --- |')
    foreach ($summary in $summaries) {
        $sheetName = Split-Path -Leaf $summary.ContactSheet
        $manifestName = Split-Path -Leaf $summary.Manifest
        $resultsName = Split-Path -Leaf $summary.Results
        $lines.Add("| $($summary.Run) | $($summary.Status) | $($summary.ResultEntries) | ``$sheetName`` ($($summary.ContactSheetKB) KB, sha256 ``$($summary.ContactSheetSha256)``) | ``$manifestName`` ($($summary.ManifestEntries) entries, sha256 ``$($summary.ManifestSha256)``) | ``$resultsName`` (sha256 ``$($summary.ResultsSha256)``) | $($summary.RawPpmCount) | $($summary.Ledger) |")
    }
    $lines.Add('')
    $lines.Add('## Proof Paths')
    $lines.Add('')
    foreach ($summary in $summaries) {
        $lines.Add("- $($summary.Run): results ``$($summary.Results)``; contact sheet ``$($summary.ContactSheet)``; manifest ``$($summary.Manifest)``.")
    }
    $lines | Set-Content -LiteralPath $ReportPath -Encoding UTF8
}

$summaries | Format-Table -AutoSize
