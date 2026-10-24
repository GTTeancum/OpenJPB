# Boot the current XBE in the isolated XEMU and drive the remote pad past the
# boot movies and title prompt to the main menu. Test helper only.
param([string]$Out = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo
& (Join-Path $PSScriptRoot 'boot_xemu.ps1') -GdbPort 9248 | Out-Null
# A skips each boot movie and then the title prompt; mode 0 is the main menu.
$steps = @('until jpb_XboxStartupPhase >= 4 90', 'until jpb_XboxFrontendMode != 0 90',
    'pressuntil A jpb_XboxFrontendMode == 0 90', 'sleep 1')
$args = @()
if ($Out) { $args += @('--out', $Out) }
python (Join-Path $PSScriptRoot 'xemu_pad.py') @args @steps

