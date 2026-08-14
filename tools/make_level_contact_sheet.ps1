[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$InputDirectory = [System.IO.Path]::GetFullPath($InputDirectory)
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
$resultsPath = Join-Path $InputDirectory 'results.json'
$ffmpeg = (Get-Command ffmpeg -ErrorAction Stop).Source
$results = @(
    (Get-Content -LiteralPath $resultsPath -Raw | ConvertFrom-Json) |
        ForEach-Object { $_ }
)
$columns = 5
$imageWidth = 320
$imageHeight = 180
$labelHeight = 30
$cellHeight = $imageHeight + $labelHeight
$rows = [Math]::Ceiling($results.Count / [double]$columns)
$proofDirectory = Join-Path $InputDirectory 'proofs'

New-Item -ItemType Directory -Force -Path $proofDirectory | Out-Null
New-Item -ItemType Directory -Force -Path (
    Split-Path -Parent $OutputPath) | Out-Null

Add-Type -AssemblyName System.Drawing
$sheet = [System.Drawing.Bitmap]::new(
    $columns * $imageWidth,
    $rows * $cellHeight,
    [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$graphics = [System.Drawing.Graphics]::FromImage($sheet)
$graphics.Clear([System.Drawing.Color]::FromArgb(8, 10, 14))
$graphics.InterpolationMode =
    [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.PixelOffsetMode =
    [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$font = [System.Drawing.Font]::new(
    'Segoe UI',
    11,
    [System.Drawing.FontStyle]::Regular,
    [System.Drawing.GraphicsUnit]::Pixel)
$brush = [System.Drawing.SolidBrush]::new(
    [System.Drawing.Color]::FromArgb(236, 240, 245))
$statusBrush = [System.Drawing.SolidBrush]::new(
    [System.Drawing.Color]::FromArgb(85, 220, 135))

try {
    for ($index = 0; $index -lt $results.Count; ++$index) {
        $result = $results[$index]
        $source = Join-Path $InputDirectory "$($result.level).ppm"
        $converted = Join-Path $proofDirectory "$($result.level).png"
        $x = ($index % $columns) * $imageWidth
        $y = [Math]::Floor($index / $columns) * $cellHeight

        & $ffmpeg -hide_banner -loglevel error -y -i $source $converted
        if ($LASTEXITCODE -ne 0) {
            throw "ffmpeg failed to convert $source"
        }
        $image = [System.Drawing.Image]::FromFile($converted)
        try {
            $graphics.DrawImage(
                $image,
                [System.Drawing.Rectangle]::new(
                    $x, $y, $imageWidth, $imageHeight))
        } finally {
            $image.Dispose()
        }

        $label = '{0:D2} {1}   {2:N1} fps   player {3}px' -f (
            [int]$result.index),
            ([string]$result.level).ToUpperInvariant(),
            [double]$result.throughputFps,
            [int64]$result.finalPlayerPixels
        $graphics.DrawString(
            $label,
            $font,
            $brush,
            [float]($x + 8),
            [float]($y + $imageHeight + 7))
        $graphics.DrawString(
            [string]$result.status,
            $font,
            $statusBrush,
            [float]($x + $imageWidth - 38),
            [float]($y + $imageHeight + 7))
    }
    $sheet.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $statusBrush.Dispose()
    $brush.Dispose()
    $font.Dispose()
    $graphics.Dispose()
    $sheet.Dispose()
}

Write-Host "Contact sheet: $OutputPath"
