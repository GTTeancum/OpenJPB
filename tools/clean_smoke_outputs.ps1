[CmdletBinding()]
param(
    [string]$OutputRoot = '',
    [switch]$KeepPerCapturePng,
    [switch]$PruneLegacyHardwareProofs
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repoRoot 'out'
}

$repoRoot = [System.IO.Path]::GetFullPath($repoRoot)
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)

if (-not (Test-Path -LiteralPath $OutputRoot -PathType Container)) {
    throw "Output root not found: $OutputRoot"
}
if (-not $OutputRoot.StartsWith(
        $repoRoot,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean outside repository: $OutputRoot"
}

function Get-TreeBytes {
    param([string]$Path)
    $sum = (Get-ChildItem -LiteralPath $Path -Recurse -File -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum).Sum
    if ($null -eq $sum) {
        return 0
    }
    return [int64]$sum
}

$before = Get-TreeBytes $OutputRoot

$scratchDirs = @(
    'movie-visible-default-long-smoke',
    'movie-orientation-smoke',
    'boss-smoke-probe',
    'camera-prefetch-fed',
    'camera-prefetch-fed-staged'
)
foreach ($dir in $scratchDirs) {
    $path = Join-Path $OutputRoot $dir
    if (Test-Path -LiteralPath $path -PathType Container) {
        $resolved = (Resolve-Path -LiteralPath $path).Path
        if (-not $resolved.StartsWith(
                $OutputRoot,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove outside output root: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

$scratchAudits = @(
    'FMV_VISIBLE_DEFAULT_LONG_SMOKE_AUDIT.md',
    'FMV_ORIENTATION_SMOKE_AUDIT.md',
    'LEVEL_SMOKE_CAMERA_PREFETCH_FED.md',
    'LEVEL_SMOKE_CAMERA_PREFETCH_FED_STAGED.md'
)
foreach ($audit in $scratchAudits) {
    $path = Join-Path (Join-Path $repoRoot 'docs') $audit
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Remove-Item -LiteralPath $path -Force
    }
}

Get-ChildItem -LiteralPath $OutputRoot -Recurse -File -Filter '*.ppm' -ErrorAction SilentlyContinue |
    Remove-Item -Force

$scratchFiles = @(
    'staged-before-prefetch-test.exe'
)
foreach ($file in $scratchFiles) {
    $path = Join-Path $OutputRoot $file
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Remove-Item -LiteralPath $path -Force
    }
}

if (-not $KeepPerCapturePng) {
    Get-ChildItem -LiteralPath $OutputRoot -Recurse -File -Filter '*.png' -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne 'contact-sheet.png' } |
        Remove-Item -Force
}

if ($PruneLegacyHardwareProofs) {
    $legacyHardwareProofs = Join-Path $OutputRoot 'hardware_proofs'
    if (Test-Path -LiteralPath $legacyHardwareProofs -PathType Container) {
        $resolved = (Resolve-Path -LiteralPath $legacyHardwareProofs).Path
        if (-not $resolved.StartsWith(
                $OutputRoot,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to prune outside output root: $resolved"
        }
        Get-ChildItem -LiteralPath $resolved -File -Include '*.mp4', '*.exe' -ErrorAction SilentlyContinue |
            Remove-Item -Force
    }
}

$after = Get-TreeBytes $OutputRoot
[pscustomobject]@{
    OutputRoot = $OutputRoot
    BeforeMB = [math]::Round($before / 1MB, 2)
    AfterMB = [math]::Round($after / 1MB, 2)
    FreedMB = [math]::Round(($before - $after) / 1MB, 2)
    RemainingFiles = (Get-ChildItem -LiteralPath $OutputRoot -Recurse -File -ErrorAction SilentlyContinue |
        Measure-Object).Count
}
