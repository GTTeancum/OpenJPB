param(
    [Parameter(Mandatory = $true)]
    [string]$RetailTrail,

    [Parameter(Mandatory = $true)]
    [string]$PortableTrail,

    [string]$OutputPath,

    [ValidateRange(1, 3600)]
    [int]$PulseInterval = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ConvertTo-InvariantDouble {
    param([string]$Text)

    return [double]::Parse(
        $Text,
        [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture)
}

function ConvertTo-HexUInt32 {
    param([string]$Text)

    $value = $Text.Trim()
    if ($value.StartsWith('0x', [StringComparison]::OrdinalIgnoreCase)) {
        $value = $value.Substring(2)
    }
    return [Convert]::ToUInt32($value, 16)
}

function Format-Vector3 {
    param(
        [object]$Row,
        [string]$X,
        [string]$Y,
        [string]$Z
    )

    return '{0}/{1}/{2}' -f $Row.$X, $Row.$Y, $Row.$Z
}

$retailPath = (Resolve-Path -LiteralPath $RetailTrail).Path
$portablePath = (Resolve-Path -LiteralPath $PortableTrail).Path
$retail = @(Import-Csv -LiteralPath $retailPath)
$portable = @(Import-Csv -LiteralPath $portablePath)

if ($retail.Count -eq 0) {
    throw "Retail trail contains no rows: $retailPath"
}
if ($portable.Count -eq 0) {
    throw "Portable trail contains no rows: $portablePath"
}

function New-FrameMap {
    param(
        [object[]]$Rows,
        [string]$FrameColumn,
        [string]$Description
    )

    $map = @{}
    foreach ($row in $Rows) {
        $frame = [int64]$row.$FrameColumn
        if ($map.ContainsKey($frame)) {
            throw "$Description trail contains duplicate frame $frame"
        }
        $map[$frame] = $row
    }
    return $map
}

$fields = @(
    [pscustomobject]@{ Name = 'global_timer'; Retail = 'global_timer'; Portable = 'global_timer'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'global_frame_rate'; Retail = 'global_frame_rate'; Portable = 'global_frame_rate'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_type'; Retail = 'camera_type'; Portable = 'camera_type'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'new_camera_flag'; Retail = 'new_camera_flag'; Portable = 'new_camera_flag'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'input_x'; Retail = 'input_x'; Portable = 'axis_x'; Kind = 'float'; Tolerance = 0.00001 },
    [pscustomobject]@{ Name = 'input_y'; Retail = 'input_y'; Portable = 'axis_y'; Kind = 'float'; Tolerance = 0.00001 },
    [pscustomobject]@{ Name = 'buttons'; Retail = 'p0_pad1'; Portable = 'buttons'; Kind = 'hex'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'player_x'; Retail = 'p0_x'; Portable = 'player_x'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'player_y'; Retail = 'p0_y'; Portable = 'player_y'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'player_z'; Retail = 'p0_z'; Portable = 'player_z'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'player_vpos_x'; Retail = 'p0_vpos_x'; Portable = 'player_vpos_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'player_vpos_y'; Retail = 'p0_vpos_y'; Portable = 'player_vpos_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'player_vpos_z'; Retail = 'p0_vpos_z'; Portable = 'player_vpos_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'move_x'; Retail = 'p0_mov_x'; Portable = 'move_x'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'move_y'; Retail = 'p0_mov_y'; Portable = 'move_y'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'move_z'; Retail = 'p0_mov_z'; Portable = 'move_z'; Kind = 'float'; Tolerance = 0.01 },
    [pscustomobject]@{ Name = 'camera_x'; Retail = 'camera_x'; Portable = 'camera_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_y'; Retail = 'camera_y'; Portable = 'camera_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_z'; Retail = 'camera_z'; Portable = 'camera_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'eye_x'; Retail = 'eye_x'; Portable = 'eye_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'eye_y'; Retail = 'eye_y'; Portable = 'eye_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'eye_z'; Retail = 'eye_z'; Portable = 'eye_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_location_x'; Retail = 'camera_location_x'; Portable = 'camera_location_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_location_y'; Retail = 'camera_location_y'; Portable = 'camera_location_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_location_z'; Retail = 'camera_location_z'; Portable = 'camera_location_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_dest_x'; Retail = 'camera_dest_x'; Portable = 'camera_dest_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_dest_y'; Retail = 'camera_dest_y'; Portable = 'camera_dest_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'camera_dest_z'; Retail = 'camera_dest_z'; Portable = 'camera_dest_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'pitch'; Retail = 'pitch'; Portable = 'pitch'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'yaw'; Retail = 'yaw'; Portable = 'yaw'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dest_pitch'; Retail = 'dest_pitch'; Portable = 'dest_pitch'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dest_yaw'; Retail = 'dest_yaw'; Portable = 'dest_yaw'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'target_x'; Retail = 'target_x'; Portable = 'target_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'target_y'; Retail = 'target_y'; Portable = 'target_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'target_z'; Retail = 'target_z'; Portable = 'target_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly'; Retail = 'dolly'; Portable = 'dolly'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'override'; Retail = 'override'; Portable = 'override'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'flags'; Retail = 'flags'; Portable = 'flags'; Kind = 'hex'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'backup_flags'; Retail = 'backup_flags'; Portable = 'backup_flags'; Kind = 'hex'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'lead_x'; Retail = 'lead_x'; Portable = 'lead_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'lead_y'; Retail = 'lead_y'; Portable = 'lead_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'lead_z'; Retail = 'lead_z'; Portable = 'lead_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'lead_dot'; Retail = 'lead_dot'; Portable = 'lead_dot'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_offset_x'; Retail = 'dolly_offset_x'; Portable = 'dolly_offset_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_offset_y'; Retail = 'dolly_offset_y'; Portable = 'dolly_offset_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_offset_z'; Retail = 'dolly_offset_z'; Portable = 'dolly_offset_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_slack_x'; Retail = 'dolly_slack_x'; Portable = 'dolly_slack_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_slack_y'; Retail = 'dolly_slack_y'; Portable = 'dolly_slack_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_slack_z'; Retail = 'dolly_slack_z'; Portable = 'dolly_slack_z'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_off_x'; Retail = 'dolly_off_x'; Portable = 'dolly_off_x'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_off_y'; Retail = 'dolly_off_y'; Portable = 'dolly_off_y'; Kind = 'int'; Tolerance = 0.0 },
    [pscustomobject]@{ Name = 'dolly_off_z'; Retail = 'dolly_off_z'; Portable = 'dolly_off_z'; Kind = 'int'; Tolerance = 0.0 }
)

$retailColumns = @($retail[0].PSObject.Properties.Name)
$portableColumns = @($portable[0].PSObject.Properties.Name)
foreach ($field in $fields) {
    if ($field.Retail -notin $retailColumns) {
        throw "Retail trail is missing column '$($field.Retail)'"
    }
    if ($field.Portable -notin $portableColumns) {
        throw "Portable trail is missing column '$($field.Portable)'"
    }
}

$retailByFrame = New-FrameMap $retail 'frame' 'Retail'
$portableByFrame = New-FrameMap $portable 'total_frames' 'Portable'
$retailFrames = @($retailByFrame.Keys | Sort-Object)
$portableFrames = @($portableByFrame.Keys | Sort-Object)
$commonFrames = @(
    $retailFrames | Where-Object { $portableByFrame.ContainsKey($_) }
)
$missingPortableFrames = @(
    $retailFrames | Where-Object { !$portableByFrame.ContainsKey($_) }
)
$missingRetailFrames = @(
    $portableFrames | Where-Object { !$retailByFrame.ContainsKey($_) }
)
if ($commonFrames.Count -eq 0) {
    throw 'Retail and portable trails have no common frame numbers'
}
$comparedRows = $commonFrames.Count
$firstMismatches = [ordered]@{}
$firstOverallMismatch = $null

for ($index = 0; $index -lt $comparedRows; ++$index) {
    $frame = $commonFrames[$index]
    $retailRow = $retailByFrame[$frame]
    $portableRow = $portableByFrame[$frame]
    foreach ($field in $fields) {
        switch ($field.Kind) {
            'float' {
                $retailValue = ConvertTo-InvariantDouble $retailRow.($field.Retail)
                $portableValue = ConvertTo-InvariantDouble $portableRow.($field.Portable)
                $matches = [Math]::Abs($retailValue - $portableValue) -le $field.Tolerance
            }
            'hex' {
                $retailValue = ConvertTo-HexUInt32 $retailRow.($field.Retail)
                $portableValue = ConvertTo-HexUInt32 $portableRow.($field.Portable)
                $matches = $retailValue -eq $portableValue
            }
            default {
                $retailValue = [int64]$retailRow.($field.Retail)
                $portableValue = [int64]$portableRow.($field.Portable)
                $matches = $retailValue -eq $portableValue
            }
        }
        if (!$matches -and !$firstMismatches.Contains($field.Name)) {
            $mismatch = [pscustomobject]@{
                Row = $index
                Frame = $frame
                RetailFrame = $retailRow.frame
                PortableFrame = $portableRow.total_frames
                RetailValue = $retailRow.($field.Retail)
                PortableValue = $portableRow.($field.Portable)
            }
            $firstMismatches[$field.Name] = $mismatch
            if ($null -eq $firstOverallMismatch) {
                $firstOverallMismatch = [pscustomobject]@{
                    Field = $field.Name
                    Detail = $mismatch
                }
            }
        }
    }
}

$lines = [Collections.Generic.List[string]]::new()
$lines.Add('# FED Camera Trail Comparison')
$lines.Add('')
$lines.Add("- Retail: ``$retailPath``")
$lines.Add("- Portable: ``$portablePath``")
$lines.Add("- Rows: retail=$($retail.Count), portable=$($portable.Count), compared=$comparedRows")
$lines.Add("- Frame alignment: common=$($commonFrames.Count), missing portable=$($missingPortableFrames.Count), missing retail=$($missingRetailFrames.Count)")
$lines.Add("- Fields checked per row: $($fields.Count)")
if ($null -eq $firstOverallMismatch) {
    $lines.Add('- First divergence: none')
} else {
    $detail = $firstOverallMismatch.Detail
    $lines.Add(
        "- First divergence: ``$($firstOverallMismatch.Field)`` at frame $($detail.Frame) " +
        "(comparison row $($detail.Row)): " +
        "retail=$($detail.RetailValue), portable=$($detail.PortableValue)")
}
$lines.Add("- Missing portable frames: $((@($missingPortableFrames | Select-Object -First 12) -join ', ') -replace '^$', 'none')")
$lines.Add("- Missing retail frames: $((@($missingRetailFrames | Select-Object -First 12) -join ', ') -replace '^$', 'none')")
$lines.Add('')
$lines.Add('## First Divergence By Field')
$lines.Add('')
$lines.Add('| Field | Row | Retail frame | Portable total | Retail | Portable |')
$lines.Add('|---|---:|---:|---:|---:|---:|')
foreach ($field in $fields) {
    if ($firstMismatches.Contains($field.Name)) {
        $detail = $firstMismatches[$field.Name]
        $lines.Add(
            "| $($field.Name) | $($detail.Row) | $($detail.RetailFrame) | " +
            "$($detail.PortableFrame) | $($detail.RetailValue) | $($detail.PortableValue) |")
    } else {
        $lines.Add("| $($field.Name) | - | - | - | match | match |")
    }
}

$lines.Add('')
$lines.Add("## ${PulseInterval}-Frame Pulses")
$lines.Add('')
$lines.Add('| Row | Retail/portable frame | Player retail/portable | Eye retail/portable | Dolly retail/portable | Lead retail/portable |')
$lines.Add('|---:|---|---|---|---|---|')
for ($index = 0; $index -lt $comparedRows; $index += $PulseInterval) {
    $frame = $commonFrames[$index]
    $retailRow = $retailByFrame[$frame]
    $portableRow = $portableByFrame[$frame]
    $retailPlayer = Format-Vector3 $retailRow 'p0_x' 'p0_y' 'p0_z'
    $portablePlayer = Format-Vector3 $portableRow 'player_x' 'player_y' 'player_z'
    $retailEye = Format-Vector3 $retailRow 'eye_x' 'eye_y' 'eye_z'
    $portableEye = Format-Vector3 $portableRow 'eye_x' 'eye_y' 'eye_z'
    $retailLead = Format-Vector3 $retailRow 'lead_x' 'lead_y' 'lead_z'
    $portableLead = Format-Vector3 $portableRow 'lead_x' 'lead_y' 'lead_z'
    $lines.Add(
        "| $index | $($retailRow.frame)/$($portableRow.total_frames) | " +
        "$retailPlayer / $portablePlayer | $retailEye / $portableEye | " +
        "$($retailRow.dolly)/$($portableRow.dolly) | $retailLead / $portableLead |")
}

$report = ($lines -join [Environment]::NewLine) + [Environment]::NewLine
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $report
} else {
    $outputFullPath = [IO.Path]::GetFullPath($OutputPath)
    [IO.File]::WriteAllText(
        $outputFullPath,
        $report,
        [Text.UTF8Encoding]::new($false))
    Write-Output $outputFullPath
}
