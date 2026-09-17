param([switch]$Execute)
$ErrorActionPreference = 'Stop'
$xboxRoot = (Resolve-Path -LiteralPath (Split-Path $PSScriptRoot -Parent)).Path
$buildRoot = (Resolve-Path -LiteralPath (Join-Path $xboxRoot 'build')).Path
if ([IO.Path]::GetDirectoryName($buildRoot) -ne $xboxRoot -or
    [IO.Path]::GetFileName($buildRoot) -ne 'build') {
    throw "Unexpected Xbox build path: $buildRoot"
}
$configPath = Join-Path $buildRoot 'xemu/xemu.toml'
$active = $null
if (Test-Path -LiteralPath $configPath) {
    $config = Get-Content -LiteralPath $configPath -Raw
    $match = [regex]::Match($config, "(?m)^dvd_path\s*=\s*'([^']+)'\s*$")
    if ($match.Success) {
        $active = [IO.Path]::GetFullPath($match.Groups[1].Value.Replace('/', '\'))
    }
}
$generatedPattern = '^(OpenJPB-\d{8}-\d{6}|OpenJPB-probe|OpenJPB|probe-current)\.iso$'
$candidates = @(Get-ChildItem -LiteralPath $buildRoot -File -Filter '*.iso' |
    Where-Object {
        $_.DirectoryName -eq $buildRoot -and
        $_.Name -match $generatedPattern -and
        $_.FullName -ne $active
    })
$bytes = ($candidates | Measure-Object -Property Length -Sum).Sum
Write-Output ("Disposable repo ISOs: {0}; reclaimable: {1:N2} GiB" -f
    $candidates.Count, ($bytes / 1GB))
if (-not $Execute) {
    Write-Output 'Preview only. Run with -Execute to remove these generated ISOs.'
    return
}
foreach ($candidate in $candidates) {
    if ([IO.Path]::GetFullPath($candidate.DirectoryName) -ne $buildRoot) {
        throw "Refusing path outside Xbox build directory: $($candidate.FullName)"
    }
    Remove-Item -LiteralPath $candidate.FullName -Force
}
Write-Output ("Removed {0} generated repo ISOs." -f $candidates.Count)
