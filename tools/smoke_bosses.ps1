[CmdletBinding()]
param(
    [string]$Executable = 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe',
    [string]$OutputDirectory = '',
    [string]$LedgerPath = '',
    [int]$Frames = 180,
    [ValidateSet('Headless', 'Hardware')]
    [string]$RunMode = 'Headless',
    [int]$Width = 960,
    [int]$Height = 540,
    [int]$TimeoutSeconds = 90,
    [string]$Python = '',
    [switch]$SkipProofImages,
    [switch]$KeepRawCaptures,
    [string[]]$Boss = @()
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'out\boss-smoke'
}
if ([string]::IsNullOrWhiteSpace($LedgerPath)) {
    $LedgerPath = Join-Path $repoRoot 'docs\BOSS_SMOKE_AUDIT.md'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$LedgerPath = [System.IO.Path]::GetFullPath($LedgerPath)
$Executable = [System.IO.Path]::GetFullPath($Executable)

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "OpenJPB executable not found: $Executable"
}
if ($Frames -lt 1 -or $Width -lt 320 -or $Height -lt 240) {
    throw 'Frames and framebuffer dimensions must be positive runtime values.'
}

$matrix = @(
    [pscustomobject]@{
        Name = 'fed-droid-fighter'
        Level = 'fed'
        LevelIndex = 1
        Label = 'FED droid fighter'
        Spawn = @(-22656, 5376, -9856)
        PlacementId = 128
        Actor = 5
        Ai = 16
        ActorName = 'drdfitr.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor'
        Notes = 'High-hp authored droid-fighter placement near the FED boss arena; spawn uses the linked trigger cluster so the player stays alive and visible.'
    },
    [pscustomobject]@{
        Name = 'marsh-mtt'
        Level = 'marsh'
        LevelIndex = 2
        Label = 'Marsh MTT'
        Spawn = @(21287, 4416, -12416)
        PlacementId = 78
        Actor = 8
        Ai = 3
        ActorName = 'mtt.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor'
        Notes = 'MTT actor with authored path down the Marsh boss corridor; spawn uses a later path point where the authored camera keeps the player visible.'
    },
    [pscustomobject]@{
        Name = 'theed-tank'
        Level = 'theed'
        LevelIndex = 3
        Label = 'Theed tank'
        Spawn = @(-3200, 3328, -20007)
        PlacementId = 53
        Actor = 1
        Ai = 34
        ActorName = 'tank.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor'
        Notes = 'High-hp tank placement tied to the Theed vehicle encounter cluster; spawn uses a grounded authored waypoint from the tank path.'
    },
    [pscustomobject]@{
        Name = 'tato-maul'
        Level = 'tato'
        LevelIndex = 5
        Label = 'Tatooine Darth Maul'
        Spawn = @(-20467, 9984, 12173)
        PlacementId = 92
        Actor = 10
        Ai = 45
        ActorName = 'sithjedi.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor'
        Notes = 'Explicit Sith Jedi actor with 200 HP in the Tatooine arena.'
    },
    [pscustomobject]@{
        Name = 'core-maul'
        Level = 'core'
        LevelIndex = 10
        Label = 'Core Darth Maul'
        Spawn = @(31284, 2560, -17242)
        PlacementId = 11
        Actor = 4
        Ai = 33
        ActorName = 'corguard.baf'
        ExpectedModel = 43
        ForcePlacement = $true
        Confidence = 'confirmed executable actor/model mapping and authored finale controller'
        Notes = 'The J3D actor label is intentionally indirect: loader_loadEnemies maps corguard.baf through sObiNames[43] to sModelNames[43], maul_d. Placement 11 is the 250-HP AI 33 arena fighter/controller linked to placements 64/65 and 31..34.'
    },
    [pscustomobject]@{
        Name = 'corus1-thug'
        Level = 'corus1'
        LevelIndex = 6
        Label = 'Coruscant thug'
        Spawn = @(-29952, 10342, -18432)
        PlacementId = 153
        Actor = 12
        Ai = 15
        ActorName = 'corhum4.baf'
        ForcePlacement = $true
        Confidence = 'confirmed retail boss stream/actor'
        Notes = 'Late Coruscant high-hp human placement matching the thug-boss asset family; retail assets include 06_CorThugBoss.wav and diagnostics place id 153 in the authored camera-director enemy set with mode 5.'
    },
    [pscustomobject]@{
        Name = 'mini2-kadu'
        Level = 'mini2'
        LevelIndex = 12
        Label = 'Mini2 Kadu'
        Spawn = @(21504, 4608, 24320)
        PlacementId = 0
        Actor = 6
        Ai = 0
        ActorName = 'horns.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor/camera'
        Notes = 'Bonus Kadu encounter starts on active horns.baf placements in the authored camera-director enemy set; placement 0 reports 255 HP at the quickload spawn.'
    },
    [pscustomobject]@{
        Name = 'mini3-boss-nass'
        Level = 'mini3'
        LevelIndex = 13
        Label = 'Mini3 Boss Nass'
        Spawn = @(12800, 13824, -14093)
        PlacementId = 4
        Actor = 1
        Ai = 4
        ActorName = 'bossnass.baf'
        ForcePlacement = $true
        Confidence = 'confirmed actor'
        Notes = 'Explicit Boss Nass actor at the Gungan bonus encounter.'
    }
)

if ($Boss.Count -gt 0) {
    $requested = @{}
    foreach ($name in $Boss) {
        $requested[$name.ToLowerInvariant()] = $true
    }
    $matrix = @($matrix | Where-Object { $requested.ContainsKey($_.Name.ToLowerInvariant()) })
    if ($matrix.Count -ne $requested.Count) {
        $known = (($matrix | ForEach-Object { $_.Name }) -join ', ')
        throw "One or more requested bosses are not in the matrix. Matched: $known"
    }
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LedgerPath) | Out-Null

function Get-MatchValue {
    param(
        [string]$Text,
        [string]$Pattern,
        [int]$Group = 1
    )
    $match = [regex]::Match(
        $Text,
        $Pattern,
        [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if (-not $match.Success) {
        return $null
    }
    return $match.Groups[$Group].Value
}

function Quote-ProcessArgument {
    param([string]$Value)
    if ($Value -notmatch '[\s"]') {
        return $Value
    }
    return '"' + ($Value -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

function Resolve-ProofPython {
    param([string]$ExplicitPython)

    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($ExplicitPython)) {
        $candidates.Add($ExplicitPython)
    }
    $command = Get-Command python -ErrorAction SilentlyContinue
    if ($null -ne $command -and
        -not [string]::IsNullOrWhiteSpace($command.Source)) {
        $candidates.Add($command.Source)
    }

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        & $candidate -c 'from PIL import Image, ImageDraw' *> $null
        if ($LASTEXITCODE -eq 0) {
            return $candidate
        }
    }
    return $null
}

function New-ProofImages {
    param(
        [string]$Directory,
        [string]$PythonExecutable,
        [string[]]$OrderedNames
    )

    $scriptPath = [System.IO.Path]::GetTempFileName() + '.py'
    $script = @'
import sys
import json
from pathlib import Path
from PIL import Image, ImageDraw

out = Path(sys.argv[1])
ordered_names = sys.argv[2:]
imgs = []

seen = set()
ordered_ppms = []
missing = []
for name in ordered_names:
    ppm = out / f"{name}.ppm"
    if ppm.exists():
        ordered_ppms.append(ppm)
        seen.add(ppm.resolve())
    else:
        missing.append(name)
if missing:
    raise SystemExit("missing ordered PPM captures: " + ", ".join(missing))
for ppm in sorted(out.glob("*.ppm")):
    resolved = ppm.resolve()
    if resolved not in seen:
        ordered_ppms.append(ppm)
        seen.add(resolved)

for ppm in ordered_ppms:
    im = Image.open(ppm).convert("RGB")
    png = ppm.with_suffix(".png")
    im.save(png)
    thumb = im.copy()
    thumb.thumbnail((320, 180))
    imgs.append((ppm.stem, thumb))

if not imgs:
    raise SystemExit("no PPM captures found")

w = 640
h = 180 * ((len(imgs) + 1) // 2)
sheet = Image.new("RGB", (w, h), (24, 24, 24))
draw = ImageDraw.Draw(sheet)
for i, (name, thumb) in enumerate(imgs):
    x = (i % 2) * 320
    y = (i // 2) * 180
    sheet.paste(thumb, (x, y))
    draw.rectangle([x, y, x + 319, y + 20], fill=(0, 0, 0))
    draw.text((x + 6, y + 4), name, fill=(255, 255, 255))

contact = out / "contact-sheet.png"
sheet.save(contact)
manifest = out / "contact-sheet.manifest.json"
manifest.write_text(json.dumps({
    "contact_sheet": str(contact),
    "width": w,
    "height": h,
    "entries": [name for name, _ in imgs],
    "entry_count": len(imgs),
}, indent=2), encoding="utf-8")
print(contact)
'@
    try {
        Set-Content -LiteralPath $scriptPath -Value $script -Encoding ASCII
        $output = & $PythonExecutable $scriptPath $Directory $OrderedNames
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Boss smoke proof image generation failed."
            return $null
        }
        return [System.IO.Path]::GetFullPath(($output | Select-Object -Last 1))
    } finally {
        Remove-Item -LiteralPath $scriptPath -Force -ErrorAction SilentlyContinue
    }
}

function Assert-ProofImage {
    param(
        [string]$ContactSheetPath,
        [string[]]$ExpectedNames
    )

    if ([string]::IsNullOrWhiteSpace($ContactSheetPath)) {
        throw 'Boss smoke visual proof contact sheet was not generated.'
    }
    if (-not (Test-Path -LiteralPath $ContactSheetPath -PathType Leaf)) {
        throw "Boss smoke visual proof contact sheet not found: $ContactSheetPath"
    }
    $contactSheet = Get-Item -LiteralPath $ContactSheetPath
    if ($contactSheet.Length -le 0) {
        throw "Boss smoke visual proof contact sheet is empty: $ContactSheetPath"
    }

    $manifestPath = Join-Path (Split-Path -Parent $ContactSheetPath) 'contact-sheet.manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Boss smoke visual proof manifest not found: $manifestPath"
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $entries = @($manifest.entries)
    if ($entries.Count -ne $ExpectedNames.Count) {
        throw "Boss smoke visual proof manifest has $($entries.Count) entries, expected $($ExpectedNames.Count)."
    }
    for ($i = 0; $i -lt $ExpectedNames.Count; $i++) {
        if ($entries[$i] -ne $ExpectedNames[$i]) {
            throw "Boss smoke visual proof manifest entry $i is '$($entries[$i])', expected '$($ExpectedNames[$i])'."
        }
    }
    $expectedHeight = 180 * [int][Math]::Ceiling($ExpectedNames.Count / 2.0)
    if ([int]$manifest.width -ne 640 -or
        [int]$manifest.height -ne $expectedHeight) {
        throw "Boss smoke visual proof contact sheet is $($manifest.width)x$($manifest.height), expected 640x$expectedHeight."
    }
}

function Remove-RawCaptureFiles {
    param([string]$Directory)

    Get-ChildItem -LiteralPath $Directory -File -Filter '*.ppm' |
        Remove-Item -Force
    Get-ChildItem -LiteralPath $Directory -File -Filter '*.png' |
        Where-Object { $_.Name -ne 'contact-sheet.png' } |
        Remove-Item -Force
}

function Invoke-BossSmoke {
    param([pscustomobject]$Entry)

    $consolePath = Join-Path $OutputDirectory "$($Entry.Name).console.txt"
    $framePath = Join-Path $OutputDirectory "$($Entry.Name).ppm"
    $arguments = @(
        '--mute',
        '--quickload', $Entry.Level,
        '--spawn-position',
        ([string]$Entry.Spawn[0]), ([string]$Entry.Spawn[1]), ([string]$Entry.Spawn[2]),
        '--camera-diagnostics',
        '--enemy-placement-diagnostics',
        '--validate-enemy-class-placement',
        ([string]$Entry.PlacementId),
        '--frames', ([string]$Frames),
        '--framebuffer-size', ([string]$Width), ([string]$Height),
        '--output', $framePath
    )
    if ($Entry.PSObject.Properties.Name -contains 'ForcePlacement' -and
        $Entry.ForcePlacement) {
        $arguments += @(
            '--force-enemy-placement',
            ([string]$Entry.PlacementId))
    }
    if ($RunMode -eq 'Headless') {
        $arguments = @('--headless', '--control-harness') + $arguments
    } else {
        $arguments = @('--hidden-window', '--control-harness') + $arguments
    }

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Executable
    $psi.WorkingDirectory = Split-Path -Parent $Executable
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = (($arguments | ForEach-Object {
        Quote-ProcessArgument ([string]$_)
    }) -join ' ')

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $psi
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) {
        try { $process.Kill($true) } catch { $process.Kill() }
        $process.WaitForExit()
    }
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    $timer.Stop()
    $exitCode = if ($timedOut) { -1 } else { $process.ExitCode }
    $combined = $stdout
    if (-not [string]::IsNullOrEmpty($stderr)) {
        $combined += "`r`n--- stderr ---`r`n$stderr"
    }
    [System.IO.File]::WriteAllText($consolePath, $combined)

    $gameLevel = Get-MatchValue $combined '^game_state=\(level=(\d+)'
    $runX = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 1
    $runY = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 2
    $runZ = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 3
    $actualFrames = Get-MatchValue $combined '^frames=(\d+) '
    $runtimePattern = ('^enemy_placement_runtime=\(id=' +
        $Entry.PlacementId + ',.*$')
    $runtimeMatch = [regex]::Match(
        $combined,
        $runtimePattern,
        [System.Text.RegularExpressions.RegexOptions]::Multiline)
    $runtimePlacement = if ($runtimeMatch.Success) {
        [string]$Entry.PlacementId
    } else {
        $null
    }
    $runtimeModel = if ($runtimeMatch.Success -and
        $runtimeMatch.Value -match 'model=([-0-9]+)') {
        $Matches[1]
    } else {
        $null
    }
    $runtimeEnergy = if ($runtimeMatch.Success -and
        $runtimeMatch.Value -match 'energy=([0-9]+)/([0-9]+)') {
        $Matches[1]
    } else {
        $null
    }
    $playerEnergy = Get-MatchValue $combined '^player_lifecycle=\(energy=(\d+)/' 1
    $playerDeath = Get-MatchValue $combined '^player_lifecycle=\(energy=\d+/\d+,min=\d+,zero=\d+,death=(\d+)' 1
    $visibleFrames = Get-MatchValue $combined 'player_visible_frames=(\d+)'
    $enemyActors = Get-MatchValue $combined 'enemy=\(actors=([^,]+),' 1
    $placementPattern = ('^enemy_placement=\(id=' + $Entry.PlacementId +
        ',actor=' + $Entry.Actor + ',ai=' + $Entry.Ai + ',.*$')
    $placementMatch = [regex]::Match(
        $combined,
        $placementPattern,
        [System.Text.RegularExpressions.RegexOptions]::Multiline)
    $placementPresent = $placementMatch.Success
    $placementStatus = if ($placementPresent -and
        $placementMatch.Value -match 'status=([0-9]+)') {
        $Matches[1]
    } else {
        $null
    }
    $frameBytes = if (Test-Path -LiteralPath $framePath -PathType Leaf) {
        (Get-Item -LiteralPath $framePath).Length
    } else {
        0
    }

    $failures = New-Object System.Collections.Generic.List[string]
    if ($timedOut) { $failures.Add('timed out') }
    if ($exitCode -ne 0) { $failures.Add("exit=$exitCode") }
    if ($gameLevel -ne ([string]$Entry.LevelIndex)) {
        $failures.Add("level=$gameLevel expected=$($Entry.LevelIndex)")
    }
    if ($actualFrames -ne ([string]$Frames)) {
        $failures.Add("frames=$actualFrames expected=$Frames")
    }
    if (-not $placementPresent) {
        $failures.Add("placement $($Entry.PlacementId)/actor $($Entry.Actor)/ai $($Entry.Ai) missing")
    }
    if (-not $runtimeMatch.Success) {
        $failures.Add("runtime placement $($Entry.PlacementId) missing")
    }
    if ($Entry.PSObject.Properties.Name -contains 'ExpectedModel' -and
        $runtimeModel -ne ([string]$Entry.ExpectedModel)) {
        $failures.Add("runtime model=$runtimeModel expected=$($Entry.ExpectedModel)")
    }
    if ($runtimeMatch.Success -and
        ($null -eq $runtimeEnergy -or [int]$runtimeEnergy -le 0)) {
        $failures.Add("runtime enemy energy=$runtimeEnergy")
    }
    if ($null -eq $playerEnergy -or [int]$playerEnergy -le 0) {
        $failures.Add("player energy=$playerEnergy")
    }
    if ($null -eq $visibleFrames -or [int]$visibleFrames -le 0) {
        $failures.Add("visible frames=$visibleFrames")
    }
    if ($frameBytes -le 32) {
        $failures.Add("capture bytes=$frameBytes")
    }

    [pscustomobject]@{
        Name = $Entry.Name
        Label = $Entry.Label
        Level = $Entry.Level
        Status = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
        Confidence = $Entry.Confidence
        PlacementId = $Entry.PlacementId
        Actor = $Entry.Actor
        Ai = $Entry.Ai
        ActorName = $Entry.ActorName
        PlacementStatus = $placementStatus
        RuntimePlacement = $runtimePlacement
        RuntimeModel = $runtimeModel
        RuntimeEnergy = $runtimeEnergy
        Spawn = ($Entry.Spawn -join '/')
        RunOrigin = @($runX, $runY, $runZ) -join '/'
        Frames = $actualFrames
        PlayerEnergy = $playerEnergy
        PlayerDeath = $playerDeath
        VisibleFrames = $visibleFrames
        EnemyActors = $enemyActors
        CaptureBytes = $frameBytes
        DurationMs = [int]$timer.ElapsedMilliseconds
        ConsolePath = $consolePath
        FramePath = $framePath
        Notes = $Entry.Notes
        Failure = ($failures -join '; ')
    }
}

$results = @()
foreach ($entry in $matrix) {
    Write-Host "smoke boss $($entry.Name) ($($entry.Level))..."
    $results += Invoke-BossSmoke $entry
}

$jsonPath = Join-Path $OutputDirectory 'results.json'
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$contactSheetPath = $null
if (-not $SkipProofImages) {
    $proofPython = Resolve-ProofPython $Python
    if ($null -ne $proofPython) {
        $orderedNames = @($results | ForEach-Object { $_.Name })
        $contactSheetPath = New-ProofImages $OutputDirectory $proofPython $orderedNames
        Assert-ProofImage $contactSheetPath $orderedNames
        if (-not $KeepRawCaptures) {
            Remove-RawCaptureFiles $OutputDirectory
        }
    } else {
        throw 'Python with Pillow was not found; pass -SkipProofImages for a smoke-only run without contact-sheet proof.'
    }
}

$now = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
$passed = @($results | Where-Object { $_.Status -eq 'PASS' }).Count
$failed = @($results | Where-Object { $_.Status -ne 'PASS' }).Count
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add('# Boss Smoke Audit')
$lines.Add('')
$lines.Add("Generated: $now")
$lines.Add('')
$lines.Add("Executable: ``$Executable``")
$lines.Add('')
$lines.Add("Mode: ``$RunMode``; frames: ``$Frames``; framebuffer: ``${Width}x${Height}``; result: ``$passed passed / $failed failed``.")
if ($null -ne $contactSheetPath) {
    $lines.Add('')
    $lines.Add("Contact sheet: ``$contactSheetPath``.")
    $lines.Add('')
    $lines.Add("Contact sheet manifest: ``$(Join-Path (Split-Path -Parent $contactSheetPath) 'contact-sheet.manifest.json')``.")
    $lines.Add('')
    $lines.Add('Contact sheet entries follow the smoke matrix order, and contact-sheet generation is required unless `-SkipProofImages` is passed. Capture bytes record the raw frame generated during the smoke run; by default the script prunes raw `.ppm` and per-capture `.png` files after the contact sheet is generated. Pass `-KeepRawCaptures` to retain them.')
}
$lines.Add('')
$lines.Add('| Boss | Level | Status | Placement | Spawn | Runtime proof | Capture |')
$lines.Add('| --- | --- | --- | --- | --- | --- | --- |')
foreach ($result in $results) {
    $proof = "frames=$($result.Frames), energy=$($result.PlayerEnergy), visible=$($result.VisibleFrames), enemyActors=$($result.EnemyActors), placementStatus=$($result.PlacementStatus), runtimePlacement=$($result.RuntimePlacement)"
    if ($result.Status -ne 'PASS') {
        $proof = $proof + "; failure=$($result.Failure)"
    }
    $captureName = Split-Path -Leaf $result.FramePath
    $captureText = if ($KeepRawCaptures -or $SkipProofImages) {
        "``$captureName`` ($($result.CaptureBytes) bytes)"
    } else {
        "``$captureName`` ($($result.CaptureBytes) bytes, pruned after contact sheet)"
    }
    $lines.Add("| $($result.Label) | $($result.Level) | $($result.Status) | id=$($result.PlacementId), actor=$($result.Actor) ``$($result.ActorName)``, ai=$($result.Ai), $($result.Confidence) | $($result.Spawn) | $proof | $captureText |")
}
$lines.Add('')
$lines.Add('## Notes')
$lines.Add('')
foreach ($result in $results) {
    $frameNote = if ($KeepRawCaptures -or $SkipProofImages) {
        "Frame: ``$($result.FramePath)``."
    } else {
        "Raw frame path during run: ``$($result.FramePath)``; file pruned after contact-sheet generation."
    }
    $lines.Add("- $($result.Label): $($result.Notes) Console: ``$($result.ConsolePath)``. $frameNote")
}
$lines.Add('')
$lines.Add("Raw JSON: ``$jsonPath``.")
$lines | Set-Content -LiteralPath $LedgerPath -Encoding UTF8

$results | Format-Table -AutoSize Name,Status,Level,PlacementId,Actor,Ai,PlacementStatus,RuntimePlacement,Frames,PlayerEnergy,VisibleFrames,CaptureBytes,Failure

if ($failed -gt 0) {
    throw "$failed boss smoke check(s) failed. See $LedgerPath"
}
