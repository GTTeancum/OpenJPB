# Boot to the main menu, then New Game > 1 Player > Jedi Mode > character to
# the level select screen. -Right moves the character carousel first.
param([string]$Out = '', [int]$Right = 0)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'to_main_menu.ps1') -Out $Out
$steps = @('press A', 'wait 45', 'press A', 'wait 45', 'press A', 'wait 60')
foreach ($i in 1..$Right) { if ($Right -gt 0) { $steps += @('press RIGHT', 'wait 30') } }
$steps += @('press A', 'until jpb_XboxFrontendMode == 0x1a 30', 'wait 30')
$padArgs = @()
if ($Out) { $padArgs += @('--out', $Out) }
python (Join-Path $PSScriptRoot 'xemu_pad.py') @padArgs @steps
