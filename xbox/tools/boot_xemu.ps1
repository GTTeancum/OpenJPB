param(
    [string]$AssetRoot = 'C:\Games\OpenJPB-Xbox',
    [string]$XemuRoot = 'C:\Games\Emulators\Xemu',
    [string]$NxdkRoot = 'C:\nxdk',
    [string]$IsoRoot = '',
    [string]$TestConfigRoot = '',
    [string]$CaptureRoot = '',
    [string]$AudioWavPath = '',
    [int]$MonitorPort = 9247,
    [ValidateRange(1,10)][int]$RenderScale = 2,
    [ValidateSet(64,128)][int]$MemoryMb = 64
)
$ErrorActionPreference = 'Stop'
$xboxRoot = Split-Path $PSScriptRoot -Parent
$buildRoot = Join-Path $xboxRoot 'build'
$isoRootPath = if ($IsoRoot) { [IO.Path]::GetFullPath($IsoRoot) } else { Join-Path $xboxRoot 'test-artifacts' }
$testConfigRootPath = if ($TestConfigRoot) { [IO.Path]::GetFullPath($TestConfigRoot) } else { Join-Path $xboxRoot 'test-config/active' }
New-Item -ItemType Directory -Path $isoRootPath -Force | Out-Null
$captureRootPath = if ($CaptureRoot) { [IO.Path]::GetFullPath($CaptureRoot) } else { Join-Path $isoRootPath 'captures' }
New-Item -ItemType Directory -Path $captureRootPath -Force | Out-Null
$runRoot = Join-Path $buildRoot 'xemu'
$configPath = Join-Path $runRoot 'xemu.toml'
if (!(Test-Path -LiteralPath $configPath)) { throw 'Create the isolated xemu.toml before booting.' }
$sourceEeprom = Join-Path $XemuRoot 'eeprom.bin'
$testEeprom = Join-Path $runRoot 'eeprom-test.bin'
$eepromArgs = @((Join-Path $PSScriptRoot 'prepare_test_eeprom.py'), '--source', $sourceEeprom, '--output', $testEeprom)
if (Test-Path -LiteralPath (Join-Path $testConfigRootPath 'xbox-720p.txt')) { $eepromArgs += '--720p' }
& python @eepromArgs
if ($LASTEXITCODE -ne 0) { throw 'Failed to prepare isolated XEMU EEPROM.' }
# Stop only the prior emulator using this exact isolated configuration.
Get-CimInstance Win32_Process -Filter "Name='xemu.exe'" | Where-Object {
    $_.CommandLine -and $_.CommandLine.Replace('/','\').ToLowerInvariant().Contains($configPath.Replace('/','\').ToLowerInvariant())
} | ForEach-Object { Stop-Process -Id $_.ProcessId }
# The previous timestamp-per-run naming left a full asset ISO after every
# smoke test. Reuse one image: the isolated prior XEMU process was stopped
# above, and extract-xiso overwrites an existing output image.
$isoName = 'OpenJPB-current.iso'
$builtXbe = Join-Path $buildRoot 'release/default.xbe'
$assetXbe = Join-Path $AssetRoot 'default.xbe'
if (!(Test-Path -LiteralPath $assetXbe) -or
    (Get-FileHash -LiteralPath $builtXbe).Hash -ne (Get-FileHash -LiteralPath $assetXbe).Hash) {
    Copy-Item -LiteralPath $builtXbe -Destination $assetXbe -Force
}
$testMarkers = @()
if (Test-Path -LiteralPath $testConfigRootPath -PathType Container) {
    $testMarkers = @(Get-ChildItem -LiteralPath $testConfigRootPath -File -Filter 'xbox-*.txt')
}
Push-Location $isoRootPath
try {
    foreach ($marker in $testMarkers) {
        Copy-Item -LiteralPath $marker.FullName -Destination (Join-Path $AssetRoot $marker.Name) -Force
    }
    & (Join-Path $NxdkRoot 'tools/extract-xiso/build/extract-xiso.exe') -c $AssetRoot $isoName *> (Join-Path $isoRootPath 'iso.log')
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $isoName)) { throw 'Disc image creation failed.' }
} finally {
    foreach ($marker in $testMarkers) {
        Remove-Item -LiteralPath (Join-Path $AssetRoot $marker.Name) -Force -ErrorAction SilentlyContinue
    }
    Pop-Location
}
$isoPath = (Join-Path $isoRootPath $isoName).Replace('\','/')
$config = Get-Content -LiteralPath $configPath -Raw
$testEepromConfig = $testEeprom.Replace('\','/')
$config = [regex]::Replace($config, '(?m)^eeprom_path\s*=.*$', "eeprom_path='$testEepromConfig'")
$capturePath = $captureRootPath.Replace('\','/')
$config = [regex]::Replace($config, '(?m)^screenshot_dir\s*=.*$', "screenshot_dir='$capturePath'")
$config = [regex]::Replace($config, '(?m)^startup_size\s*=.*$', "startup_size='1280x720'")
if ($config -notmatch '(?m)^\[display\.window\]') {
    $config = $config.TrimEnd() + "`n[display.window]`nstartup_size='1280x720'`n"
}
if ($config -match '(?m)^\[display\.quality\]') {
    $config = [regex]::Replace($config, '(?m)^surface_scale\s*=.*$', "surface_scale=$RenderScale")
} else {
    $config = $config.TrimEnd() + "`n[display.quality]`nsurface_scale=$RenderScale`n"
}
$config = [regex]::Replace($config, '(?ms)^\[display\.ui\]\s*.*?(?=^\[|\z)', '')
$config = $config.TrimEnd() + "`n[display.ui]`nfit='scale'`naspect_ratio='16x9'`n"
$config = [regex]::Replace($config, '(?ms)^\[sys\]\s*.*?(?=^\[|\z)', '')
$config = $config.TrimEnd() + "`n[sys]`nmem_limit='$MemoryMb'`n"
$config = [regex]::Replace($config, "(?m)^dvd_path\s*=.*$", "dvd_path='$isoPath'")
Set-Content -LiteralPath $configPath -Value $config
$previousTemp = $env:TEMP
$previousTmp = $env:TMP
try {
    $env:TEMP = $isoRootPath
    $env:TMP = $isoRootPath
    $launchArgs = @(
        '-config_path', ('"' + $configPath + '"'), '-monitor',
        "tcp:127.0.0.1:$MonitorPort,server,nowait", '-snapshot'
    )
    if ($AudioWavPath) {
        $audioPath = [IO.Path]::GetFullPath($AudioWavPath).Replace('\','/')
        $launchArgs += @('-audio', "driver=wav,path=$audioPath")
    }
    $process = Start-Process -FilePath (Join-Path $XemuRoot 'xemu.exe') -ArgumentList $launchArgs -WorkingDirectory $isoRootPath -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $isoRootPath 'stdout.log') -RedirectStandardError (Join-Path $isoRootPath 'stderr.log')
} finally {
    $env:TEMP = $previousTemp
    $env:TMP = $previousTmp
}
$process.Id | Set-Content -LiteralPath (Join-Path $runRoot 'pid.txt')
[pscustomobject]@{ ProcessId=$process.Id; Disc=$isoPath; Config=$configPath }
