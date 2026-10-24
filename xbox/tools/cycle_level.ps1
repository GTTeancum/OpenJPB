# From the main menu: start the first level with the given character, play a
# few seconds, quit through the pause menu and report free memory pages.
# Repeat -Count times. Test helper for front-end/gameplay round trips.
param([int]$Count = 1, [int]$Start = 0, [int]$Right = 0, [string]$Out = '')
$ErrorActionPreference = 'Stop'
$pad = Join-Path $PSScriptRoot 'xemu_pad.py'
$padArgs = @()
if ($Out) { $padArgs += @('--out', $Out) }
foreach ($cycle in 1..$Count) {
    $steps = @('read jpb_XboxFrontendFreePages', 'press A', 'wait 45', 'press A', 'wait 45', 'press A', 'wait 60')
    foreach ($i in 1..$Right) { if ($Right -gt 0) { $steps += @('press RIGHT', 'wait 30') } }
    $steps += @('press A', 'until jpb_XboxFrontendMode == 0x1a 30', 'wait 30', 'press A',
        "until jpb_XboxSessionStats >= $($Start + $cycle) 60", 'pressuntil A jpb_XboxStartupPhase == 8 120',
        'pressuntil A jpb_XboxSmokeFrame >= 150 40', 'read jpb_XboxFreePages',
        "shot cycle$cycle", 'press START', 'wait 20', 'press UP', 'wait 10', 'press A',
        'wait 20', 'press DOWN', 'wait 10', 'press A', 'until jpb_XboxFrontendMode == 0 30',
        'wait 64', 'read jpb_XboxFrontendFreePages', 'read jpb_XboxSessionStats 4', 'read jpb_XboxSessionPages 8')
    python $pad @padArgs @steps | Select-String 'FreePages|Session|shot'
    if ($LASTEXITCODE -ne 0) { throw "cycle $cycle failed" }
}




