[CmdletBinding()]
param(
    [string]$Executable = 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe',
    [string]$OutputDirectory = '',
    [string]$LedgerPath = '',
    [int]$Frames = 720,
    [ValidateSet('Headless', 'Hardware')]
    [string]$RunMode = 'Headless',
    [int]$Width = 960,
    [int]$Height = 540,
    [int]$TimeoutSeconds = 120,
    [Alias('InputKey')]
    [ValidateSet('w', 'a', 's', 'd')]
    [string[]]$InputKeys = @('w', 'a', 's', 'd'),
    [string[]]$Level = @()
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'out\level-smoke'
}
if ([string]::IsNullOrWhiteSpace($LedgerPath)) {
    $LedgerPath = Join-Path $repoRoot 'docs\LEVEL_SMOKE_AUDIT.md'
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
$InputKeys = @($InputKeys | Select-Object -Unique)
if ($InputKeys.Count -eq 0) {
    throw 'At least one virtual control-probe key is required.'
}
$controlFrameMinimum = [Math]::Min(
    30,
    [Math]::Max(1, [int][Math]::Floor($Frames / 3.0)))

# Order and indices come from the executable/PDB-owned sLevelNames table in
# src/reconstructed/original/level_world.c. Duplicate train1 slots collapse to
# one package because they resolve to the same installed asset and runtime ID.
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

function Invoke-LevelSmokeAttempt {
    param(
        [pscustomobject]$Entry,
        [string]$InputKey
    )

    $name = "$($Entry.Name).$InputKey"
    $consolePath = Join-Path $OutputDirectory "$name.console.txt"
    $framePath = Join-Path $OutputDirectory "$name.ppm"
    $arguments = @(
        '--mute',
        '--quickload', $Entry.Name,
        '--camera-diagnostics',
        '--frames', ([string]$Frames),
        '--framebuffer-size', ([string]$Width), ([string]$Height),
        '--output', $framePath
    )
    if ($Entry.Name -eq 'mini2') {
        # ai_Kadu masks classic-config input to 0xa0 and accelerates only
        # when 0x80/0x20 alternate. K and Space are those exact keyboard
        # owners; cycling keeps the probe virtual and deterministic.
        $arguments += @(
            '--headless-keyboard-phase', 'k', '1',
            '--headless-keyboard-phase', 'space', '1',
            '--cycle-input-phases'
        )
    } elseif ($Entry.Name -eq 'corus1') {
        # Coruscant begins on the edge of dolly 0's establishing region.
        # The retail start heading (0xc00) and JPX camera polygons put the
        # playable route on virtual D; release after crossing into dolly 53
        # so the smoke probe validates the handoff instead of walking back
        # out of the finite camera region for the rest of the capture.
        $routeFrames = [Math]::Min(170, $Frames)
        $arguments += @(
            '--headless-keyboard-phase', 'd', ([string]$routeFrames)
        )
        if ($routeFrames -lt $Frames) {
            $arguments += @(
                '--headless-keyboard-phase', 'none',
                ([string]($Frames - $routeFrames))
            )
        }
    } else {
        $arguments += @(
            '--headless-keyboard-phase', $InputKey, ([string]$Frames)
        )
    }
    if ($RunMode -eq 'Headless') {
        $arguments = @('--headless') + $arguments
    } else {
        $arguments = @('--hidden-window', '--scripted-input') + $arguments
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

    $levelFbx = Get-MatchValue $combined '^level_fbx=(.+?) batches='
    $gameLevel = Get-MatchValue $combined '^game_state=\(level=(\d+)'
    $inputSource = Get-MatchValue $combined '^spawn_view=\(model=\d+,input=([^,]+)'
    $spawnX = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 1
    $spawnZ = Get-MatchValue $combined '^run_origin=\(player=([-0-9.]+)/([-0-9.]+)/([-0-9.]+)/facing:' 3
    $actualFrames = Get-MatchValue $combined '^frames=(\d+) '
    $mode = Get-MatchValue $combined '^frames=\d+ mode=([^ ]+)'
    $presentationFrames = Get-MatchValue $combined '^presentation=\(frames=(\d+)'
    $presentationHardware = Get-MatchValue $combined '^presentation_backend=\(hardware=(\d+)'
    $presentationBackend = Get-MatchValue $combined '^presentation_backend=\(hardware=\d+,name=([^,]+)'
    $triangles = Get-MatchValue $combined '^frames=.*? triangles=(\d+)'
    $pixels = Get-MatchValue $combined '^frames=.*? pixels=(\d+)'
    $modelPixels = Get-MatchValue $combined '^frames=.*? model_triangles=\d+ model_lines=\d+ model_pixels=(\d+)'
    $playerVisibleFrames = Get-MatchValue $combined '^frames=.*? player_visible_frames=(\d+)'
    $hardwareWorldDepthPixels = Get-MatchValue $combined '^hardware_final_frame=\(world_depth_pixels=(\d+)'
    $hardwareWorldTextures = Get-MatchValue $combined '^hardware_final_frame=\(world_depth_pixels=\d+,world_textures=(\d+)/'
    $hardwareWorldMaterials = Get-MatchValue $combined '^hardware_final_frame=\(world_depth_pixels=\d+,world_textures=\d+/(\d+)'
    $finalPlayerPixels = Get-MatchValue $combined '^hardware_final_frame=\(world_depth_pixels=\d+,world_textures=\d+/\d+,player_pixels=(\d+)'
    $target = Get-MatchValue $combined '^frames=.*? target=([^\( ]+)'
    $cameraCollision = Get-MatchValue $combined '^frames=.*? camera=\(dolly=\d+,authored=\d+,collision=([0-9.]+)'
    $locomotionFrames = Get-MatchValue $combined '^control_edges=\(p1=.*?locomotion:(\d+)/'
    $directionFrames = Get-MatchValue $combined '^control_edges=\(p1=.*?direction:(\d+)'
    $heldBits = Get-MatchValue $combined '^control_edges=\(p1=.*?held_bits:([0-9a-fA-F]+)'
    $vehicleAttach = Get-MatchValue $combined '^vehicle_control=\(opcode607=\d+,stap_candidates=\d+,attach=(\d+)/'
    $kaduRaceWidth = 0
    foreach ($match in [regex]::Matches(
        $combined,
        '^screen_draw\[\d+\]=\(owner=kadu-race-bar,file=.*?dst=(-?\d+)/(-?\d+)/(-?\d+)/(-?\d+)',
        [System.Text.RegularExpressions.RegexOptions]::Multiline)) {
        $width = [Math]::Abs(
            [int]$match.Groups[3].Value - [int]$match.Groups[1].Value)
        if ($width -gt $kaduRaceWidth) {
            $kaduRaceWidth = $width
        }
    }
    $enemyActors = Get-MatchValue $combined '^frames=.*? enemy=\(actors=(\d+)/'
    $finalX = Get-MatchValue $combined '^frames=.*? physics=\(([-0-9.]+),([-0-9.]+),([-0-9.]+),facing=' 1
    $finalZ = Get-MatchValue $combined '^frames=.*? physics=\(([-0-9.]+),([-0-9.]+),([-0-9.]+),facing=' 3
    $crashSignal = [regex]::IsMatch(
        $combined,
        'runtime init failed|fatal error|AddressSanitizer|unhandled exception|assertion failed',
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $captureBytes = if (Test-Path -LiteralPath $framePath) {
        (Get-Item -LiteralPath $framePath).Length
    } else {
        0
    }
    $displacement = 0.0
    if ($null -ne $spawnX -and $null -ne $spawnZ -and
        $null -ne $finalX -and $null -ne $finalZ) {
        $deltaX = [double]$finalX - [double]$spawnX
        $deltaZ = [double]$finalZ - [double]$spawnZ
        $displacement = [Math]::Sqrt($deltaX * $deltaX + $deltaZ * $deltaZ)
    }
    $assetMode = if ([string]::IsNullOrEmpty($levelFbx)) { 'JPX' } else { 'FBX+JPX' }
    $elapsedSeconds = $timer.Elapsed.TotalSeconds
    $throughput = if ($elapsedSeconds -gt 0.0) {
        [double]$actualFrames / $elapsedSeconds
    } else {
        0.0
    }

    $gates = [ordered]@{
        exited = (-not $timedOut -and $exitCode -eq 0)
        loaded = ($gameLevel -eq [string]$Entry.Index)
        rendered = ([int64]$triangles -gt 0 -and $captureBytes -gt 32 -and
            $(if ($RunMode -eq 'Hardware') {
                [int64]$hardwareWorldDepthPixels -gt 0
            } else {
                [int64]$pixels -gt 0
            }))
        textured = ($RunMode -eq 'Headless' -or
            ([int]$hardwareWorldMaterials -gt 0 -and
             [int]$hardwareWorldTextures -eq
                [int]$hardwareWorldMaterials))
        framed = ([int]$playerVisibleFrames -gt 0 -and
            ($RunMode -eq 'Headless' -or [int64]$finalPlayerPixels -gt 0))
        camera = (-not [string]::IsNullOrEmpty($cameraCollision) -and
            [double]$cameraCollision -gt 0.0)
        keyboard = ($inputSource -eq 'keyboard' -and
            -not [string]::IsNullOrEmpty($heldBits))
        control = if ($Entry.Name -eq 'mini2') {
            $kaduRaceWidth -ge 128 -and $displacement -gt 16.0
        } else {
            [int]$directionFrames -ge $controlFrameMinimum -and
                ([int]$locomotionFrames -ge $controlFrameMinimum -or
                 [int]$vehicleAttach -gt 0) -and
                $displacement -gt 16.0
        }
        complete = ([int]$actualFrames -eq $Frames)
        presenter = ($RunMode -eq 'Headless' -or
            ([int]$presentationHardware -eq 1 -and
             [int]$presentationFrames -eq $Frames))
        performance = ($RunMode -eq 'Headless' -or $throughput -ge 45.0)
        crashFree = (-not $crashSignal)
    }
    $failedGates = @($gates.GetEnumerator() |
        Where-Object { -not $_.Value } | ForEach-Object Key)
    $status = if (-not $gates.exited -or -not $gates.loaded -or
        -not $gates.rendered -or -not $gates.textured -or
        -not $gates.framed -or
        -not $gates.camera -or -not $gates.keyboard -or
        -not $gates.control -or -not $gates.complete -or
        -not $gates.presenter -or
        -not $gates.crashFree) {
        'FAIL'
    } elseif ($failedGates.Count -gt 0) {
        'WARN'
    } else {
        'PASS'
    }

    return [pscustomobject]@{
        level = $Entry.Name
        inputKey = $InputKey
        index = $Entry.Index
        group = $Entry.Group
        runMode = $RunMode
        status = $status
        failedGates = $failedGates
        exitCode = $exitCode
        timedOut = $timedOut
        seconds = [Math]::Round($elapsedSeconds, 3)
        throughputFps = [Math]::Round($throughput, 2)
        expectedFrames = $Frames
        actualFrames = [int]$actualFrames
        mode = $mode
        presentationFrames = [int]$presentationFrames
        presentationHardware = [int]$presentationHardware
        presentationBackend = $presentationBackend
        assetMode = $assetMode
        fbx = $levelFbx
        triangles = [int64]$triangles
        pixels = [int64]$pixels
        modelPixels = [int64]$modelPixels
        playerVisibleFrames = [int]$playerVisibleFrames
        hardwareWorldDepthPixels = [int64]$hardwareWorldDepthPixels
        hardwareWorldTextures = [int]$hardwareWorldTextures
        hardwareWorldMaterials = [int]$hardwareWorldMaterials
        finalPlayerPixels = [int64]$finalPlayerPixels
        target = $target
        cameraCollision = [double]$cameraCollision
        inputSource = $inputSource
        heldBits = $heldBits
        directionFrames = [int]$directionFrames
        locomotionFrames = [int]$locomotionFrames
        vehicleAttach = [int]$vehicleAttach
        kaduRaceWidth = $kaduRaceWidth
        displacement = [Math]::Round($displacement, 1)
        enemyActors = [int]$enemyActors
        capture = $framePath
        captureBytes = $captureBytes
        console = $consolePath
        gates = $gates
    }
}

function Invoke-LevelSmoke {
    param([pscustomobject]$Entry)

    $attempts = [System.Collections.Generic.List[object]]::new()
    $selected = $null
    $probeKeys = if ($Entry.Name -eq 'mini2') {
        @('race')
    } elseif ($Entry.Name -eq 'corus1') {
        @('route')
    } else {
        $InputKeys
    }
    foreach ($key in $probeKeys) {
        $attempt = Invoke-LevelSmokeAttempt $Entry $key
        $attempts.Add($attempt)
        if ($attempt.status -ne 'FAIL') {
            $selected = $attempt
            break
        }
    }
    if ($null -eq $selected) {
        $selected = $attempts[0]
        foreach ($attempt in $attempts) {
            if ($attempt.displacement -gt $selected.displacement) {
                $selected = $attempt
            }
        }
    }

    $canonicalConsole = Join-Path $OutputDirectory "$($Entry.Name).console.txt"
    $canonicalCapture = Join-Path $OutputDirectory "$($Entry.Name).ppm"
    Copy-Item -LiteralPath $selected.console -Destination $canonicalConsole -Force
    if (Test-Path -LiteralPath $selected.capture -PathType Leaf) {
        Copy-Item -LiteralPath $selected.capture -Destination $canonicalCapture -Force
    }
    $selected.console = $canonicalConsole
    $selected.capture = $canonicalCapture
    $selected | Add-Member -NotePropertyName probes -NotePropertyValue @(
        $attempts | ForEach-Object {
            [pscustomobject]@{
                key = $_.inputKey
                status = $_.status
                exitCode = $_.exitCode
                displacement = $_.displacement
                directionFrames = $_.directionFrames
                locomotionFrames = $_.locomotionFrames
                console = (Join-Path $OutputDirectory "$($Entry.Name).$($_.inputKey).console.txt")
                capture = (Join-Path $OutputDirectory "$($Entry.Name).$($_.inputKey).ppm")
            }
        }
    )
    return $selected
}

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash
$results = [System.Collections.Generic.List[object]]::new()
foreach ($entry in $matrix) {
    Write-Host ("[{0}/{1}] {2}" -f ($results.Count + 1), $matrix.Count, $entry.Name)
    $result = Invoke-LevelSmoke $entry
    $results.Add($result)
    Write-Host ("  {0} ({1}s, key={2}, probes={3}) gates={4}" -f
        $result.status,
        $result.seconds,
        $result.inputKey.ToUpperInvariant(),
        $result.probes.Count,
        (($result.failedGates -join ',') -replace '^$', 'all'))
}

$jsonPath = Join-Path $OutputDirectory 'results.json'
$results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding utf8

$passCount = @($results | Where-Object status -eq 'PASS').Count
$warnCount = @($results | Where-Object status -eq 'WARN').Count
$failCount = @($results | Where-Object status -eq 'FAIL').Count
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('# Level Smoke Audit')
$lines.Add('')
$lines.Add('Generated by `tools/smoke_levels.ps1` from the exact staged replacement executable.')
$lines.Add('')
$lines.Add("- Executable: ``$Executable``")
$lines.Add("- SHA-256: ``$hash``")
$lines.Add("- Matrix: $($results.Count) unique playable packages")
$probeLabel = (($InputKeys | ForEach-Object { $_.ToUpperInvariant() }) -join '/')
$lines.Add("- Run: $RunMode, $Frames frames per level at ${Width}x${Height}, isolated virtual ``$probeLabel`` control probes")
$lines.Add("- Result: $passCount pass, $warnCount warning, $failCount fail")
$lines.Add("- Machine-readable results: ``$jsonPath``")
$lines.Add('')
$lines.Add('| Level | ID | Group | Assets | Status | Failed gates | Frames | Seconds/FPS | Control frames | Travel | Camera | Scene/model pixels | Final world/player pixels | Textures | Player visible frames | Actors |')
$lines.Add('|---|---:|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|')
foreach ($result in $results) {
    $failed = if ($result.failedGates.Count -eq 0) {
        '-'
    } else {
        $result.failedGates -join ', '
    }
    $lines.Add((
        '| {0} | {1} | {2} | {3} | {4} | {5} | {6}/{7} | {8:N3}/{9:N2} | {10}:{11}/{12} | {13:N1} | {14:N3} | {15:N0}/{16:N0} | {17:N0}/{18:N0} | {19}/{20} | {21} | {22} |' -f
        $result.level,
        $result.index,
        $result.group,
        $result.assetMode,
        $result.status,
        $failed,
        $result.actualFrames,
        $result.expectedFrames,
        $result.seconds,
        $result.throughputFps,
        $result.inputKey.ToUpperInvariant(),
        $result.directionFrames,
        $result.locomotionFrames,
        $result.displacement,
        $result.cameraCollision,
        $result.pixels,
        $result.modelPixels,
        $result.hardwareWorldDepthPixels,
        $result.finalPlayerPixels,
        $result.hardwareWorldTextures,
        $result.hardwareWorldMaterials,
        $result.playerVisibleFrames,
        $result.enemyActors))
}
$lines.Add('')
$lines.Add('## Gate Definitions')
$lines.Add('')
$lines.Add('- `loaded`: the canonical runtime level ID was reported; FBX is optional for JPX-only levels.')
$lines.Add('- `rendered`: hardware mode requires final-frame world-only GPU linear-depth coverage before actors/HUD; headless mode requires scene pixels. Both require scene triangles and a final PPM capture.')
$lines.Add('- `textured`: hardware mode successfully resolved every FBX-declared world material texture; headless mode marks this not applicable.')
$lines.Add('- `framed`: the actual player model contributed pixels in at least one sampled frame, and hardware mode also requires a nonzero final-frame player contribution.')
$lines.Add('- `camera`: the recovered camera collision result is positive.')
$lines.Add('- `keyboard`: the spawn input owner is keyboard and virtual key state was recorded.')
$lines.Add("- ``control``: ordinary levels require an isolated virtual ``$probeLabel`` probe with at least $controlFrameMinimum directional frames, at least $controlFrameMinimum locomotion frames or a real vehicle attachment, and more than 16 world units of horizontal travel. Corus1 follows its retail 0xc00 start heading with 170 virtual D frames, then releases input after the JPX collision handoff to dolly 53. Mini2 instead requires its PDB-owned alternating K/Space Kaadu cadence to grow a race bar to at least 128 pixels and move the mounted rider. Each attempted protocol has its own console log and frame capture.")
$lines.Add('- `complete`: the requested frame budget completed without timeout.')
$lines.Add('- `presenter`: hardware mode created the D3D11 hardware presenter and presented every requested frame; headless mode marks this not applicable.')
$lines.Add('- `performance`: hardware mode sustained at least 45 measured frames per second over the capture; headless mode marks this not applicable.')
$lines.Add('- `crashFree`: no fatal, assertion, sanitizer, or unhandled-exception marker appeared.')
$lines.Add('')
$lines.Add('## Asset Coverage Notes')
$lines.Add('')
$lines.Add('The PDB/EXE-owned table has 28 slots but only 23 unique playable JPX packages in the retail install. Slots 23 and 24 duplicate `train1`. `council`, `derek`, and `june` have no installed JPX package, so the replacement runtime cannot quickload them. Retail W3D-only `train8` and `train9` are not named by the runtime table and are not selectable shipped levels.')
$lines.Add('')
$lines.Add('Each row has a matching `<level>.console.txt` and `<level>.ppm` under the output directory. Warnings are issue-hunt leads, not passes.')
$lines | Set-Content -LiteralPath $LedgerPath -Encoding utf8

Write-Host "Ledger: $LedgerPath"
Write-Host "JSON:   $jsonPath"
Write-Host "Summary: $passCount pass, $warnCount warning, $failCount fail"
if ($failCount -gt 0) {
    exit 1
}
