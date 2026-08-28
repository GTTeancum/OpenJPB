[CmdletBinding()]
param(
    [string]$Executable = 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe',
    [string]$OutputDirectory = '',
    [string]$LedgerPath = '',
    [int]$Frames = 1800,
    [int]$PlacementFrames = 600,
    [int]$PlacementSamplesPerLevel = 2,
    [int]$Width = 640,
    [int]$Height = 360,
    [int]$TimeoutSeconds = 300,
    [switch]$SkipPlacementRuns,
    [switch]$KeepCaptures,
    [string[]]$Level = @()
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'out\level-soak'
}
if ([string]::IsNullOrWhiteSpace($LedgerPath)) {
    $LedgerPath = Join-Path $repoRoot 'docs\LEVEL_SOAK_CRASH_HUNT.md'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$LedgerPath = [System.IO.Path]::GetFullPath($LedgerPath)
$Executable = [System.IO.Path]::GetFullPath($Executable)

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "OpenJPB executable not found: $Executable"
}
if ($Frames -lt 1 -or $PlacementFrames -lt 1 -or
    $PlacementSamplesPerLevel -lt 0 -or
    $Width -lt 320 -or $Height -lt 240) {
    throw 'Frame counts, placement samples, and framebuffer dimensions must be valid.'
}

$matrix = @(
    [pscustomobject]@{ Name = 'fed';     Index = 1;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'marsh';   Index = 2;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'theed';   Index = 3;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'palace';  Index = 4;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'tato';    Index = 5;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'corus1';  Index = 6;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'ruins';   Index = 7;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'streets'; Index = 8;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'hangar';  Index = 9;  Group = 'campaign' },
    [pscustomobject]@{ Name = 'core';    Index = 10; Group = 'campaign' },
    [pscustomobject]@{ Name = 'mini1';   Index = 11; Group = 'bonus' },
    [pscustomobject]@{ Name = 'mini2';   Index = 12; Group = 'bonus' },
    [pscustomobject]@{ Name = 'mini3';   Index = 13; Group = 'bonus' },
    [pscustomobject]@{ Name = 'mini4';   Index = 14; Group = 'bonus' },
    [pscustomobject]@{ Name = 'corus2';  Index = 15; Group = 'bonus' },
    [pscustomobject]@{ Name = 'train1';  Index = 16; Group = 'training' },
    [pscustomobject]@{ Name = 'train2';  Index = 17; Group = 'training' },
    [pscustomobject]@{ Name = 'train3';  Index = 18; Group = 'training' },
    [pscustomobject]@{ Name = 'train5';  Index = 19; Group = 'training' },
    [pscustomobject]@{ Name = 'train6';  Index = 20; Group = 'training' },
    [pscustomobject]@{ Name = 'train7';  Index = 21; Group = 'training' },
    [pscustomobject]@{ Name = 'train4';  Index = 22; Group = 'training' },
    [pscustomobject]@{ Name = 'arena';   Index = 25; Group = 'versus' }
)

if ($Level.Count -gt 0) {
    $requested = @{}
    foreach ($name in $Level) {
        $requested[$name.ToLowerInvariant()] = $true
    }
    $matrix = @($matrix | Where-Object { $requested.ContainsKey($_.Name) })
    if ($matrix.Count -ne $requested.Count) {
        $known = ($matrix.Name -join ', ')
        throw "One or more requested levels are not in the playable matrix. Matched: $known"
    }
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LedgerPath) | Out-Null

function Quote-ProcessArgument {
    param([string]$Value)
    if ($Value -notmatch '[\s"]') {
        return $Value
    }
    return '"' + ($Value -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

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

function Invoke-SoakProcess {
    param(
        [string]$Name,
        [string[]]$Arguments,
        [int]$Timeout
    )

    $consolePath = Join-Path $OutputDirectory "$Name.console.txt"
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Executable
    $psi.WorkingDirectory = Split-Path -Parent $Executable
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = (($Arguments | ForEach-Object {
        Quote-ProcessArgument ([string]$_)
    }) -join ' ')

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $psi
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($Timeout * 1000)
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

    [pscustomobject]@{
        name = $Name
        console = $consolePath
        text = $combined
        exitCode = $exitCode
        timedOut = $timedOut
        seconds = [Math]::Round($timer.Elapsed.TotalSeconds, 3)
    }
}

function New-RouteArguments {
    param([int]$FrameCount)

    $arguments = @(
        '--headless',
        '--control-harness',
        '--mute',
        '--camera-diagnostics',
        '--frames', ([string]$FrameCount),
        '--framebuffer-size', ([string]$Width), ([string]$Height),
        '--headless-keyboard-phase', 'w', '90',
        '--headless-keyboard-phase', 'w+space', '1',
        '--headless-keyboard-phase', 'w', '45',
        '--headless-keyboard-phase', 'k', '1',
        '--headless-keyboard-phase', 'none', '12',
        '--headless-keyboard-phase', 'd', '80',
        '--headless-keyboard-phase', 'd+space', '1',
        '--headless-keyboard-phase', 'd', '35',
        '--headless-keyboard-phase', 'l', '1',
        '--headless-keyboard-phase', 'none', '12',
        '--headless-keyboard-phase', 's', '70',
        '--headless-keyboard-phase', 's+space', '1',
        '--headless-keyboard-phase', 'j', '1',
        '--headless-keyboard-phase', 'a', '70',
        '--headless-keyboard-phase', 'a+space', '1',
        '--headless-keyboard-phase', 'shift', '8',
        '--headless-keyboard-phase', 'u', '1',
        '--headless-keyboard-phase', 'none', '20',
        '--cycle-input-phases'
    )
    return $arguments
}

function Convert-ToResult {
    param(
        [pscustomobject]$Entry,
        [string]$Kind,
        [pscustomobject]$Run,
        [object]$PlacementId,
        [string]$ActorName
    )

    $combined = $Run.text
    $actualFrames = Get-MatchValue $combined '^frames=(\d+) '
    $gameLevel = Get-MatchValue $combined '^game_state=\(level=(\d+)'
    $finalEnergy = Get-MatchValue $combined '^player_lifecycle=\(energy=-?\d+/(-?\d+)'
    $deathFrame = Get-MatchValue $combined '^player_lifecycle=.*?death=(\d+)'
    $exitFrame = Get-MatchValue $combined '^player_lifecycle=.*?exit=(\d+)'
    $visibleFrames = Get-MatchValue $combined '^frames=.*? player_visible_frames=(\d+)'
    $enemyActors = Get-MatchValue $combined '^frames=.*? enemy=\(actors=([^,]+),'
    $triangles = Get-MatchValue $combined '^frames=.*? triangles=(\d+)'
    $pixels = Get-MatchValue $combined '^frames=.*? pixels=(\d+)'
    $motion = Get-MatchValue $combined '^frames=.*? motion=([-0-9]+)'
    $motionName = Get-MatchValue $combined '^frames=.*? motion=-?[0-9]+ ([^ ]+)'
    $runX = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 1
    $runZ = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 3
    $finalX = Get-MatchValue $combined '^frames=.*? physics=\(([-0-9.]+),([-0-9.]+),([-0-9.]+),facing=' 1
    $finalZ = Get-MatchValue $combined '^frames=.*? physics=\(([-0-9.]+),([-0-9.]+),([-0-9.]+),facing=' 3
    $directionFrames = Get-MatchValue $combined '^control_edges=\(p1=.*?direction:(\d+)'
    $locomotionFrames = Get-MatchValue $combined '^control_edges=\(p1=.*?locomotion:(\d+)/'
    $jumpCallbacks = Get-MatchValue $combined '^frames=.*? callbacks=\d+/(\d+)/'
    $cameraCollision = Get-MatchValue $combined '^frames=.*? camera=\(dolly=-?\d+,flags=[0-9a-fA-F]+,initial=-?\d+,unique=\d+,transitions=\d+,authored=\d+,collision=([0-9.]+)'
    $runtimePlacement = Get-MatchValue $combined 'placement=([-0-9]+),motion=' 1
    $crashSignal = [regex]::IsMatch(
        $combined,
        'runtime init failed|fatal error|AddressSanitizer|unhandled exception|assertion failed|access violation',
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $displacement = 0.0
    if ($null -ne $runX -and $null -ne $runZ -and
        $null -ne $finalX -and $null -ne $finalZ) {
        $deltaX = [double]$finalX - [double]$runX
        $deltaZ = [double]$finalZ - [double]$runZ
        $displacement = [Math]::Sqrt($deltaX * $deltaX + $deltaZ * $deltaZ)
    }

    $failures = New-Object System.Collections.Generic.List[string]
    $warnings = New-Object System.Collections.Generic.List[string]
    if ($Run.timedOut) { $failures.Add('timeout') }
    if ($Run.exitCode -ne 0) { $failures.Add("exit=$($Run.exitCode)") }
    if ($gameLevel -ne ([string]$Entry.Index)) {
        $failures.Add("level=$gameLevel expected=$($Entry.Index)")
    }
    if ($null -eq $actualFrames) {
        $failures.Add('missing frame summary')
    }
    if ($null -eq $triangles -or [int64]$triangles -le 0) {
        $failures.Add("triangles=$triangles")
    }
    if ($null -eq $pixels -or [int64]$pixels -le 0) {
        $warnings.Add("pixels=$pixels")
    }
    if ($null -eq $visibleFrames -or [int]$visibleFrames -le 0) {
        $warnings.Add("visible=$visibleFrames")
    }
    if ($null -eq $finalEnergy -or [int]$finalEnergy -le 0) {
        $warnings.Add("energy=$finalEnergy")
    }
    if ($deathFrame -ne '0') {
        $warnings.Add("death=$deathFrame")
    }
    if ($crashSignal) {
        $failures.Add('crash marker')
    }
    $issues = @($failures) + @($warnings)
    [pscustomobject]@{
        level = $Entry.Name
        index = $Entry.Index
        group = $Entry.Group
        kind = $Kind
        placementId = $PlacementId
        actorName = $ActorName
        status = if ($failures.Count -ne 0) {
            'FAIL'
        } elseif ($warnings.Count -ne 0) {
            'WARN'
        } else {
            'PASS'
        }
        failed = @($failures)
        warnings = @($warnings)
        issues = @($issues)
        exitCode = $Run.exitCode
        timedOut = $Run.timedOut
        seconds = $Run.seconds
        frames = [int]$actualFrames
        expectedFrames = if ($Kind -eq 'route') { $Frames } else { $PlacementFrames }
        triangles = [int64]$triangles
        pixels = [int64]$pixels
        visibleFrames = [int]$visibleFrames
        finalEnergy = [int]$finalEnergy
        deathFrame = [int]$deathFrame
        exitFrame = [int]$exitFrame
        enemyActors = $enemyActors
        runtimePlacement = $runtimePlacement
        motion = $motion
        motionName = $motionName
        directionFrames = [int]$directionFrames
        locomotionFrames = [int]$locomotionFrames
        jumpCallbacks = [int]$jumpCallbacks
        displacement = [Math]::Round($displacement, 1)
        cameraCollision = [double]$cameraCollision
        console = $Run.console
    }
}

function Get-LevelPlacements {
    param([pscustomobject]$Entry)

    $arguments = @(
        '--headless',
        '--control-harness',
        '--mute',
        '--quickload', $Entry.Name,
        '--enemy-placement-diagnostics',
        '--frames', '1',
        '--framebuffer-size', ([string]$Width), ([string]$Height)
    )
    $run = Invoke-SoakProcess "placements.$($Entry.Name)" $arguments $TimeoutSeconds
    $placements = @()
    foreach ($match in [regex]::Matches(
        $run.text,
        '^enemy_placement=\(id=(\d+),actor=(-?\d+),ai=(-?\d+),actor_name=([^,]+),.*?class=(-?\d+),status=(-?\d+),.*?loc=(-?\d+)/(-?\d+)/(-?\d+)',
        [System.Text.RegularExpressions.RegexOptions]::Multiline)) {
        if ([int]$match.Groups[5].Value -lt 0) {
            continue
        }
        $placements += [pscustomobject]@{
            id = [int]$match.Groups[1].Value
            actor = [int]$match.Groups[2].Value
            ai = [int]$match.Groups[3].Value
            actorName = $match.Groups[4].Value
            class = [int]$match.Groups[5].Value
            status = [int]$match.Groups[6].Value
            x = [int]$match.Groups[7].Value
            y = [int]$match.Groups[8].Value
            z = [int]$match.Groups[9].Value
        }
    }
    return @($placements)
}

function Select-PlacementSamples {
    param([object[]]$Placements)

    if ($PlacementSamplesPerLevel -eq 0 -or $Placements.Count -eq 0) {
        return @()
    }
    $sorted = @($Placements | Sort-Object id)
    $wanted = [System.Collections.Generic.List[int]]::new()
    $count = [Math]::Min($PlacementSamplesPerLevel, $sorted.Count)
    for ($i = 0; $i -lt $count; ++$i) {
        if ($count -eq 1) {
            $index = [int][Math]::Floor(($sorted.Count - 1) / 2.0)
        } else {
            $index = [int][Math]::Round(
                $i * (($sorted.Count - 1) / [double]($count - 1)))
        }
        if (-not $wanted.Contains($index)) {
            $wanted.Add($index)
        }
    }
    return @($wanted | ForEach-Object { $sorted[$_] })
}

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash
$results = [System.Collections.Generic.List[object]]::new()
$totalRuns = $matrix.Count
if (-not $SkipPlacementRuns) {
    $totalRuns += $matrix.Count * $PlacementSamplesPerLevel
}
$runIndex = 0

foreach ($entry in $matrix) {
    ++$runIndex
    Write-Host ("[{0}/~{1}] route {2}" -f $runIndex, $totalRuns, $entry.Name)
    $routeArgs = @('--quickload', $entry.Name) + (New-RouteArguments $Frames)
    $routeRun = Invoke-SoakProcess "route.$($entry.Name)" $routeArgs $TimeoutSeconds
    $routeResult = Convert-ToResult $entry 'route' $routeRun $null ''
    $results.Add($routeResult)
    Write-Host ("  {0} frames={1} travel={2} energy={3} failures={4}" -f
        $routeResult.status,
        $routeResult.frames,
        $routeResult.displacement,
        $routeResult.finalEnergy,
        (($routeResult.issues -join ',') -replace '^$', 'none'))

    if ($SkipPlacementRuns) {
        continue
    }
    $placements = Get-LevelPlacements $entry
    $samples = Select-PlacementSamples $placements
    foreach ($placement in $samples) {
        ++$runIndex
        $name = "placement.$($entry.Name).$($placement.id)"
        Write-Host ("[{0}/~{1}] {2} actor={3}" -f
            $runIndex, $totalRuns, $name, $placement.actorName)
        $placementArgs = @(
            '--quickload', $entry.Name,
            '--spawn-position',
            ([string]$placement.x), ([string]$placement.y), ([string]$placement.z),
            '--force-enemy-placement', ([string]$placement.id)
        ) + (New-RouteArguments $PlacementFrames)
        $placementRun = Invoke-SoakProcess $name $placementArgs $TimeoutSeconds
        $placementResult = Convert-ToResult `
            $entry 'placement' $placementRun $placement.id $placement.actorName
        $results.Add($placementResult)
        Write-Host ("  {0} frames={1} placement={2} energy={3} failures={4}" -f
            $placementResult.status,
            $placementResult.frames,
            $placementResult.runtimePlacement,
            $placementResult.finalEnergy,
            (($placementResult.issues -join ',') -replace '^$', 'none'))
    }
}

$jsonPath = Join-Path $OutputDirectory 'results.json'
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $jsonPath -Encoding utf8

if (-not $KeepCaptures) {
    Get-ChildItem -LiteralPath $OutputDirectory -File -Filter '*.ppm' -ErrorAction SilentlyContinue |
        Remove-Item -Force
}

function Get-WarningCategory {
    param([object[]]$Issues)

    $issueList = @($Issues)
    $hasPixel = @($issueList | Where-Object { $_ -like 'pixels=*' }).Count -gt 0
    $hasVisible = @($issueList | Where-Object { $_ -like 'visible=*' }).Count -gt 0
    $hasDeathOrEnergy = @(
        $issueList | Where-Object { $_ -like 'death=*' -or $_ -like 'energy=*' }
    ).Count -gt 0

    if ($hasPixel) { return 'pixel-proof miss' }
    if ($hasVisible -and $hasDeathOrEnergy) { return 'visibility+death' }
    if ($hasVisible) { return 'visibility-only' }
    if ($hasDeathOrEnergy) { return 'death/energy' }
    return 'other'
}

$passCount = @($results | Where-Object status -eq 'PASS').Count
$warnCount = @($results | Where-Object status -eq 'WARN').Count
$failCount = @($results | Where-Object status -eq 'FAIL').Count
$warningSummary = @(
    $results |
        Where-Object status -eq 'WARN' |
        ForEach-Object { Get-WarningCategory $_.issues } |
        Group-Object |
        Sort-Object Name |
        ForEach-Object { "$($_.Count) $($_.Name)" }
)
$warningSummaryText = if ($warningSummary.Count -gt 0) {
    $warningSummary -join ', '
} else {
    'none'
}
$now = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('# Level Soak Crash Hunt')
$lines.Add('')
$lines.Add("Generated: $now")
$lines.Add('')
$lines.Add("- Executable: ``$Executable``")
$lines.Add("- SHA-256: ``$hash``")
$lines.Add("- Route pass: $Frames frames per level at ${Width}x${Height}")
$lines.Add("- Placement pass: $PlacementFrames frames per sampled NPC/enemy placement; samples per level: $PlacementSamplesPerLevel; skipped: $SkipPlacementRuns")
$lines.Add("- Result: $passCount passed / $warnCount warned / $failCount failed")
$lines.Add("- Warning summary: $warningSummaryText")
$lines.Add("- Machine-readable results: ``$jsonPath``")
$lines.Add('')
$lines.Add('The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.')
$lines.Add('')
$lines.Add('| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |')
$lines.Add('| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |')
foreach ($result in $results) {
    $target = if ($result.kind -eq 'placement') {
        "id=$($result.placementId) ``$($result.actorName)`` runtime=$($result.runtimePlacement)"
    } else {
        'route'
    }
    $failure = if ($result.issues.Count -eq 0) { '-' } else { $result.issues -join ', ' }
    $motionText = if ([string]::IsNullOrWhiteSpace($result.motionName)) {
        [string]$result.motion
    } else {
        "$($result.motion) $($result.motionName)"
    }
    $lines.Add((
        '| {0} | {1} | {2} | {3} | {4}/{5} | {6:N3} | {7:N1} | {8} | {9} | {10} | {11} |' -f
        $result.level,
        $result.kind,
        $target,
        $result.status,
        $result.frames,
        $result.expectedFrames,
        $result.seconds,
        $result.displacement,
        $result.finalEnergy,
        $result.enemyActors,
        $motionText,
        $failure))
}
$lines.Add('')
$lines.Add('## Crash Signals')
$lines.Add('')
$lines.Add('A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.')
$lines.Add('')
$lines.Add('Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.')
$lines | Set-Content -LiteralPath $LedgerPath -Encoding utf8

Write-Host "Ledger: $LedgerPath"
Write-Host "JSON:   $jsonPath"
Write-Host "Summary: $passCount passed, $warnCount warned, $failCount failed"
if ($failCount -gt 0) {
    exit 1
}
