param(
    [string]$IsoRoot = 'D:\OpenJPB-Xbox-ISOs',
    [switch]$Execute
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $IsoRoot).Path
if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw "ISO directory does not exist: $root"
}
$configPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'build/xemu/xemu.toml'
$active = $null
if (Test-Path -LiteralPath $configPath) {
    $config = Get-Content -LiteralPath $configPath -Raw
    $match = [regex]::Match($config, "(?m)^dvd_path\s*=\s*'([^']+)'\s*$")
    if ($match.Success) {
        $active = [IO.Path]::GetFullPath($match.Groups[1].Value.Replace('/', '\'))
    }
}
$candidates = @(Get-ChildItem -LiteralPath $root -File -Filter 'OpenJPB-*.iso' |
    Where-Object {
        $_.DirectoryName -eq $root -and
        $_.Name -match '^OpenJPB-\d{8}-\d{6}\.iso$' -and
        $_.FullName -ne $active
    })
$bytes = ($candidates | Measure-Object -Property Length -Sum).Sum
Write-Output ("Old test ISOs: {0}; reclaimable: {1:N2} GiB" -f
    $candidates.Count, ($bytes / 1GB))
if (-not $Execute) {
    Write-Output 'Preview only. Run with -Execute to remove these old timestamped ISOs.'
    return
}
foreach ($candidate in $candidates) {
    if ([IO.Path]::GetFullPath($candidate.DirectoryName) -ne $root) {
        throw "Refusing path outside ISO directory: $($candidate.FullName)"
    }
    Remove-Item -LiteralPath $candidate.FullName -Force
}
Write-Output ("Removed {0} old ISOs; retained active/current images." -f
    $candidates.Count)
