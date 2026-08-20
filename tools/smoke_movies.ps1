[CmdletBinding()]
param(
    [string]$Executable = 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe',
    [string]$Mesh = 'C:\Games\Star Wars Jedi Power Battles\res\level\jpx\fed\fed.jpx',
    [string]$Cad = 'C:\Games\Star Wars Jedi Power Battles\res\animation\obi_wan.cad',
    [string]$Bmd = 'C:\Games\Star Wars Jedi Power Battles\res\MODEL\obi_wan.bmd',
    [string]$OutputDirectory = '',
    [string]$LedgerPath = '',
    [int]$Frames = 60,
    [ValidateSet('Headless', 'Hardware', 'Visible')]
    [string]$RunMode = 'Headless',
    [int]$Width = 960,
    [int]$Height = 540,
    [int]$TimeoutSeconds = 90,
    [string]$Python = '',
    [switch]$SkipProofImages,
    [switch]$UseDefaultFramebuffer,
    [int[]]$Movie = @()
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'out\movie-smoke'
}
if ([string]::IsNullOrWhiteSpace($LedgerPath)) {
    $LedgerPath = Join-Path $repoRoot 'docs\FMV_SMOKE_AUDIT.md'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$LedgerPath = [System.IO.Path]::GetFullPath($LedgerPath)
$Executable = [System.IO.Path]::GetFullPath($Executable)
$Mesh = [System.IO.Path]::GetFullPath($Mesh)
$Cad = [System.IO.Path]::GetFullPath($Cad)
$Bmd = [System.IO.Path]::GetFullPath($Bmd)

foreach ($path in @($Executable, $Mesh, $Cad, $Bmd)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required file not found: $path"
    }
}
if ($Frames -lt 1 -or (-not $UseDefaultFramebuffer -and ($Width -lt 320 -or $Height -lt 240))) {
    throw 'Frames and framebuffer dimensions must be positive runtime values.'
}

$matrix = @(
    [pscustomobject]@{ Index = 0; Name = 'intro';         Label = 'Intro';              Expected = 'IntroFlippedVertical_converted.ogg' },
    [pscustomobject]@{ Index = 1; Name = 'english-crawl'; Label = 'English crawl';      Expected = 'English1920Vertical_converted.ogg' },
    [pscustomobject]@{ Index = 2; Name = 'qui-gon';       Label = 'Qui-Gon intro';      Expected = 'HorizontalFlippedQui_converted.ogg' },
    [pscustomobject]@{ Index = 3; Name = 'obi-wan';       Label = 'Obi-Wan intro';      Expected = 'HorizontalFlippedObi_converted.ogg' },
    [pscustomobject]@{ Index = 4; Name = 'mace';          Label = 'Mace intro';         Expected = 'HorizontalFlippedMace_converted.ogg' },
    [pscustomobject]@{ Index = 5; Name = 'plo';           Label = 'Plo Koon intro';     Expected = 'HorizontalFlippedPlo_converted.ogg' },
    [pscustomobject]@{ Index = 6; Name = 'adi';           Label = 'Adi Gallia intro';   Expected = 'HorizontalFlippedAdi_converted.ogg' },
    [pscustomobject]@{ Index = 7; Name = 'ending';        Label = 'Ending';             Expected = 'End1080Flipped_converted.ogg' }
)

if ($Movie.Count -gt 0) {
    $requested = @{}
    foreach ($index in $Movie) {
        $requested[[int]$index] = $true
    }
    $matrix = @($matrix | Where-Object { $requested.ContainsKey([int]$_.Index) })
    if ($matrix.Count -ne $requested.Count) {
        $known = (($matrix | ForEach-Object { $_.Index }) -join ', ')
        throw "One or more requested movies are not in the matrix. Matched: $known"
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

function Get-PpmDimensions {
    param([string]$FramePath)

    if (-not (Test-Path -LiteralPath $FramePath -PathType Leaf)) {
        return $null
    }
    $bytes = [System.IO.File]::ReadAllBytes($FramePath)
    $length = [Math]::Min($bytes.Length, 256)
    if ($length -le 0) {
        return $null
    }
    $header = [System.Text.Encoding]::ASCII.GetString($bytes, 0, $length)
    $match = [regex]::Match($header, '^P6\s+(\d+)\s+(\d+)\s+255\s')
    if (-not $match.Success) {
        return $null
    }
    return [pscustomobject]@{
        Width = [int]$match.Groups[1].Value
        Height = [int]$match.Groups[2].Value
        Text = "$($match.Groups[1].Value)x$($match.Groups[2].Value)"
    }
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
        [string]$PythonExecutable
    )

    $scriptPath = [System.IO.Path]::GetTempFileName() + '.py'
    $script = @'
import sys
from pathlib import Path
from PIL import Image, ImageDraw

out = Path(sys.argv[1])
imgs = []
for ppm in sorted(out.glob("*.ppm")):
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
print(contact)
'@
    try {
        Set-Content -LiteralPath $scriptPath -Value $script -Encoding ASCII
        $output = & $PythonExecutable $scriptPath $Directory
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "FMV smoke proof image generation failed."
            return $null
        }
        return [System.IO.Path]::GetFullPath(($output | Select-Object -Last 1))
    } finally {
        Remove-Item -LiteralPath $scriptPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-MovieOrientation {
    param(
        [string]$FramePath,
        [string]$PythonExecutable
    )

    $scriptPath = [System.IO.Path]::GetTempFileName() + '.py'
    $script = @'
import json
import sys
from pathlib import Path
from PIL import Image

path = Path(sys.argv[1])
im = Image.open(path).convert("RGB")
w, h = im.size
top = 0
bottom = 0
total = 0

for y in range(h):
    row_is_top = y < h // 2
    for r, g, b in [im.getpixel((x, y)) for x in range(w)]:
        if g >= 30 and g > r * 1.25 and g >= b * 0.8 and b > 10 and (r + g + b) > 70:
            total += 1
            if row_is_top:
                top += 1
            else:
                bottom += 1

minimum = max(200, (w * h) // 1000)
ratio = (top + 1.0) / (bottom + 1.0)
status = "PASS" if total >= minimum and ratio >= 2.0 else "FAIL"
print(json.dumps({
    "status": status,
    "top": top,
    "bottom": bottom,
    "total": total,
    "minimum": minimum,
    "ratio": ratio,
}))
'@
    try {
        Set-Content -LiteralPath $scriptPath -Value $script -Encoding ASCII
        $output = & $PythonExecutable $scriptPath $FramePath
        if ($LASTEXITCODE -ne 0) {
            return [pscustomobject]@{
                Status = 'FAIL'
                Detail = 'orientation analyzer failed'
            }
        }
        $metrics = $output | Select-Object -Last 1 | ConvertFrom-Json
        return [pscustomobject]@{
            Status = $metrics.status
            Detail = ('english-crawl teal-top={0}, teal-bottom={1}, teal-total={2}, min={3}, top-bottom-ratio={4:N2}' -f
                $metrics.top,
                $metrics.bottom,
                $metrics.total,
                $metrics.minimum,
                $metrics.ratio)
        }
    } finally {
        Remove-Item -LiteralPath $scriptPath -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-MovieSmoke {
    param([pscustomobject]$Entry)

    $name = ('{0:00}-{1}' -f $Entry.Index, $Entry.Name)
    $consolePath = Join-Path $OutputDirectory "$name.console.txt"
    $framePath = Join-Path $OutputDirectory "$name.ppm"
    $arguments = @($Mesh)
    if ($RunMode -eq 'Headless') {
        $arguments += @('--headless')
    } elseif ($RunMode -eq 'Hardware') {
        $arguments += @('--hidden-window', '--control-harness')
    } else {
        $arguments += @('--control-harness')
    }
    $arguments += @(
        '--cad', $Cad,
        '--bmd', $Bmd,
        '--mute',
        '--title',
        '--validate-title-movie', ([string]$Entry.Index),
        '--frames', ([string]$Frames)
    )
    if (-not $UseDefaultFramebuffer) {
        $arguments += @(
            '--framebuffer-size', ([string]$Width), ([string]$Height)
        )
    }
    $arguments += @('--output', $framePath)

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

    $requests = Get-MatchValue $combined 'movie_state=\(requests=(\d+)'
    $resolved = Get-MatchValue $combined 'movie_state=\(requests=\d+,resolved=(\d+)'
    $launched = Get-MatchValue $combined 'movie_state=\(requests=\d+,resolved=\d+,launched=(\d+)'
    $decoded = Get-MatchValue $combined 'decoded=(\d+)'
    $presented = Get-MatchValue $combined 'presented=(\d+)'
    $audioBytes = Get-MatchValue $combined 'audio_bytes=(\d+)'
    $audioSamples = Get-MatchValue $combined 'audio_samples=(\d+)'
    $audioChunks = Get-MatchValue $combined 'audio_chunks=(\d+)'
    $failures = Get-MatchValue $combined 'failures=(\d+)'
    $last = Get-MatchValue $combined 'last=(\d+)'
    $path = Get-MatchValue $combined 'path=([^,]+),error='
    $errorText = Get-MatchValue $combined 'error=([^\)\r\n]+)'
    $runtimeFramebuffer = Get-MatchValue $combined 'framebuffer=(\d+x\d+)'
    $frameBytes = if (Test-Path -LiteralPath $framePath -PathType Leaf) {
        (Get-Item -LiteralPath $framePath).Length
    } else {
        0
    }
    $captureDimensions = Get-PpmDimensions $framePath

    $problems = New-Object System.Collections.Generic.List[string]
    if ($timedOut) { $problems.Add('timed out') }
    if ($exitCode -ne 0) { $problems.Add("exit=$exitCode") }
    if ($requests -ne '1') { $problems.Add("requests=$requests") }
    if ($resolved -ne '1') { $problems.Add("resolved=$resolved") }
    if ($launched -ne '1') { $problems.Add("launched=$launched") }
    if ($null -eq $decoded -or [int]$decoded -le 0) {
        $problems.Add("decoded=$decoded")
    }
    if ($null -eq $presented -or [int]$presented -le 0) {
        $problems.Add("presented=$presented")
    }
    if ($null -eq $audioBytes -or [int]$audioBytes -le 0) {
        $problems.Add("audio_bytes=$audioBytes")
    }
    if ($null -eq $audioSamples -or [int]$audioSamples -le 0) {
        $problems.Add("audio_samples=$audioSamples")
    }
    if ($null -eq $audioChunks -or [int]$audioChunks -le 0) {
        $problems.Add("audio_chunks=$audioChunks")
    }
    if ($failures -ne '0') { $problems.Add("failures=$failures") }
    if ($last -ne ([string]$Entry.Index)) {
        $problems.Add("last=$last expected=$($Entry.Index)")
    }
    if ($path -notlike "*$($Entry.Expected)") {
        $problems.Add("path=$path expected=*$(($Entry.Expected))")
    }
    if ($errorText -ne 'none') {
        $problems.Add("error=$errorText")
    }
    if ($frameBytes -le 32) {
        $problems.Add("capture bytes=$frameBytes")
    }
    if ($null -eq $captureDimensions) {
        $problems.Add('capture dimensions=unreadable')
    } elseif ($null -ne $runtimeFramebuffer -and
              $captureDimensions.Text -ne $runtimeFramebuffer) {
        $problems.Add("capture dimensions=$($captureDimensions.Text) runtime framebuffer=$runtimeFramebuffer")
    }

    [pscustomobject]@{
        Name = $name
        Label = $Entry.Label
        Index = $Entry.Index
        Status = if ($problems.Count -eq 0) { 'PASS' } else { 'FAIL' }
        Expected = $Entry.Expected
        Decoded = $decoded
        Presented = $presented
        AudioBytes = $audioBytes
        AudioSamples = $audioSamples
        AudioChunks = $audioChunks
        Path = $path
        CaptureBytes = $frameBytes
        CaptureDimensions = if ($null -ne $captureDimensions) { $captureDimensions.Text } else { '' }
        RuntimeFramebuffer = $runtimeFramebuffer
        DurationMs = [int]$timer.ElapsedMilliseconds
        ConsolePath = $consolePath
        FramePath = $framePath
        Orientation = 'not-checked'
        OrientationProof = ''
        Failure = ($problems -join '; ')
    }
}

$results = @()
foreach ($entry in $matrix) {
    Write-Host "smoke movie $($entry.Index) ($($entry.Name))..."
    $results += Invoke-MovieSmoke $entry
}

$jsonPath = Join-Path $OutputDirectory 'results.json'
$contactSheetPath = $null
if (-not $SkipProofImages) {
    $proofPython = Resolve-ProofPython $Python
    if ($null -ne $proofPython) {
        $contactSheetPath = New-ProofImages $OutputDirectory $proofPython
        if ($RunMode -eq 'Hardware' -or $RunMode -eq 'Visible') {
            $crawlResult = $results | Where-Object { $_.Index -eq 1 } | Select-Object -First 1
            if ($null -ne $crawlResult -and
                (Test-Path -LiteralPath $crawlResult.FramePath -PathType Leaf)) {
                $orientation = Test-MovieOrientation $crawlResult.FramePath $proofPython
                $crawlResult.Orientation = $orientation.Status
                $crawlResult.OrientationProof = $orientation.Detail
                if ($orientation.Status -ne 'PASS') {
                    $crawlResult.Status = 'FAIL'
                    if ([string]::IsNullOrWhiteSpace($crawlResult.Failure)) {
                        $crawlResult.Failure = $orientation.Detail
                    } else {
                        $crawlResult.Failure = "$($crawlResult.Failure); $($orientation.Detail)"
                    }
                }
            }
        }
    } else {
        Write-Warning 'Python with Pillow was not found; leaving raw PPM captures without PNG/contact-sheet proofs.'
    }
}

$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8

$now = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
$passed = @($results | Where-Object { $_.Status -eq 'PASS' }).Count
$failed = @($results | Where-Object { $_.Status -ne 'PASS' }).Count
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add('# FMV Smoke Audit')
$lines.Add('')
$lines.Add("Generated: $now")
$lines.Add('')
$lines.Add("Executable: ``$Executable``")
$lines.Add('')
$framebufferLabel = if ($UseDefaultFramebuffer) { 'default' } else { "${Width}x${Height}" }
$lines.Add("Mode: ``$RunMode``; frames: ``$Frames``; framebuffer: ``$framebufferLabel``; result: ``$passed passed / $failed failed``.")
if ($null -ne $contactSheetPath) {
    $lines.Add('')
    $lines.Add("Contact sheet: ``$contactSheetPath``.")
}
$lines.Add('')
$lines.Add('| Movie | Status | Expected file | Runtime proof | Framebuffer | Orientation | Capture |')
$lines.Add('| --- | --- | --- | --- | --- | --- | --- |')
foreach ($result in $results) {
    $proof = "decoded=$($result.Decoded), presented=$($result.Presented), audio_bytes=$($result.AudioBytes), audio_samples=$($result.AudioSamples), audio_chunks=$($result.AudioChunks)"
    if ($result.Status -ne 'PASS') {
        $proof = $proof + "; failure=$($result.Failure)"
    }
    $orientation = $result.Orientation
    if (-not [string]::IsNullOrWhiteSpace($result.OrientationProof)) {
        $orientation = "$orientation ($($result.OrientationProof))"
    }
    $captureName = Split-Path -Leaf $result.FramePath
    $framebufferProof = "runtime=$($result.RuntimeFramebuffer), capture=$($result.CaptureDimensions)"
    $lines.Add("| $($result.Index) $($result.Label) | $($result.Status) | ``$($result.Expected)`` | $proof | $framebufferProof | $orientation | ``$captureName`` ($($result.CaptureBytes) bytes) |")
}
$lines.Add('')
$lines.Add('## Notes')
$lines.Add('')
foreach ($result in $results) {
    $lines.Add("- $($result.Index) $($result.Label): path ``$($result.Path)``. Console: ``$($result.ConsolePath)``. Frame: ``$($result.FramePath)``.")
}
$lines.Add('')
$lines.Add("Raw JSON: ``$jsonPath``.")
$lines | Set-Content -LiteralPath $LedgerPath -Encoding UTF8

$results | Format-Table -AutoSize Name,Status,Decoded,Presented,AudioBytes,AudioSamples,AudioChunks,CaptureBytes,Failure

if ($failed -gt 0) {
    throw "$failed FMV smoke check(s) failed. See $LedgerPath"
}
