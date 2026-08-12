param(
    [Parameter(Mandatory = $true)]
    [string] $ModelDirectory,

    [string[]] $Name
)

$ErrorActionPreference = "Stop"

function Read-Int16 {
    param([byte[]] $Bytes, [int] $PayloadOffset)
    return [BitConverter]::ToInt16($Bytes, 4 + $PayloadOffset)
}

function Read-Int32 {
    param([byte[]] $Bytes, [int] $PayloadOffset)
    return [BitConverter]::ToInt32($Bytes, 4 + $PayloadOffset)
}

function Read-NodeName {
    param([byte[]] $Bytes, [int] $RecordOffset)

    $length = 0
    while ($length -lt 32 -and
           $Bytes[4 + $RecordOffset + $length] -ne 0) {
        ++$length
    }
    return [Text.Encoding]::ASCII.GetString(
        $Bytes, 4 + $RecordOffset, $length)
}

function Test-PayloadRange {
    param(
        [int] $Offset,
        [long] $Length,
        [long] $PayloadSize
    )

    return $Offset -ge 0 -and
           $Offset -le $PayloadSize -and
           $Length -ge 0 -and
           $Length -le $PayloadSize - $Offset
}

function Get-FaceIndex {
    param([int16] $StoredIndex)

    if ($StoredIndex -eq [int16]::MinValue) {
        return $null
    }
    if ($StoredIndex -lt 0) {
        return -[int]$StoredIndex
    }
    return [int]$StoredIndex
}

function Test-CompactFaceStream {
    param(
        [byte[]] $Bytes,
        [int] $IndexOffset,
        [int] $FaceCount,
        [long] $VertexCount,
        [long] $PayloadSize
    )

    if (-not (Test-PayloadRange $IndexOffset `
              ([long]$FaceCount * 4) $PayloadSize)) {
        return $false
    }
    for ($face = 0; $face -lt $FaceCount; ++$face) {
        $faceOffset = 4 + $IndexOffset + $face * 4
        $corners = if ($Bytes[$faceOffset + 3] -eq 0xff) {
            3
        } else {
            4
        }
        for ($corner = 0; $corner -lt $corners; ++$corner) {
            if ($Bytes[$faceOffset + $corner] -ge $VertexCount) {
                return $false
            }
        }
    }
    return $true
}

function Inspect-Bmd {
    param([IO.FileInfo] $File)

    $bytes = [IO.File]::ReadAllBytes($File.FullName)
    if ($bytes.Length -lt 292) {
        return [pscustomobject]@{
            File = $File.Name
            Node = ""
            Reason = "file is smaller than two geomData records"
        }
    }

    $payloadSize = [BitConverter]::ToUInt32($bytes, 0)
    if ($payloadSize -ne $bytes.Length - 4) {
        return [pscustomobject]@{
            File = $File.Name
            Node = ""
            Reason = "declared payload $payloadSize; actual $($bytes.Length - 4)"
        }
    }

    $pending = [Collections.Generic.Stack[int]]::new()
    $visited = [Collections.Generic.HashSet[int]]::new()
    $pending.Push(1)

    while ($pending.Count -ne 0) {
        $recordIndex = $pending.Pop()
        $recordOffset = $recordIndex * 144
        if (-not (Test-PayloadRange $recordOffset 144 $payloadSize)) {
            return [pscustomobject]@{
                File = $File.Name
                Node = "#$recordIndex"
                Reason = "geomData record is outside payload"
            }
        }
        if (-not $visited.Add($recordIndex)) {
            return [pscustomobject]@{
                File = $File.Name
                Node = "#$recordIndex"
                Reason = "geomData record is visited twice"
            }
        }

        $nodeName = Read-NodeName $bytes $recordOffset
        $numFaces = Read-Int32 $bytes ($recordOffset + 44)
        $numVerts = Read-Int32 $bytes ($recordOffset + 48)
        $numShareVerts = Read-Int32 $bytes ($recordOffset + 52)
        $pVertex = Read-Int32 $bytes ($recordOffset + 56)
        $pNormal = Read-Int32 $bytes ($recordOffset + 60)
        $pUV = Read-Int32 $bytes ($recordOffset + 64)
        $pColor = Read-Int32 $bytes ($recordOffset + 68)
        $pIndex = Read-Int32 $bytes ($recordOffset + 104)
        $numChildren = Read-Int32 $bytes ($recordOffset + 108)

        if ($numFaces -lt 0 -or $numVerts -lt 0 -or
            $numShareVerts -lt 0) {
            return [pscustomobject]@{
                File = $File.Name
                Node = "$nodeName#$recordIndex"
                Reason = "negative geometry count"
            }
        }

        $ranges = @(
            @("pVertex", $pVertex, [long]$numVerts * 3 * 4),
            @("pIndex", $pIndex, [long]$numFaces * 8),
            @("pUV", $pUV, [long]$numFaces * 32)
        )
        foreach ($range in $ranges) {
            if (-not (Test-PayloadRange $range[1] $range[2] $payloadSize)) {
                return [pscustomobject]@{
                    File = $File.Name
                    Node = "$nodeName#$recordIndex"
                    Reason = "$($range[0])=$($range[1]) + $($range[2]) exceeds payload"
                }
            }
        }

        # _RenderNode expands three packed vertices for each geomData.numVerts
        # unit, appending them after the inherited shared-vertex prefix.
        $totalVerts = [long]$numShareVerts + [long]$numVerts * 3
        $cornerCount = 0L
        for ($face = 0; $face -lt $numFaces; ++$face) {
            $faceOffset = $pIndex + $face * 8
            $stored = @(
                (Read-Int16 $bytes $faceOffset),
                (Read-Int16 $bytes ($faceOffset + 2)),
                (Read-Int16 $bytes ($faceOffset + 4)),
                (Read-Int16 $bytes ($faceOffset + 6))
            )
            $faceCorners = if ($stored[3] -eq [int16]::MaxValue) {
                3
            } else {
                4
            }
            for ($corner = 0; $corner -lt $faceCorners; ++$corner) {
                $index = Get-FaceIndex $stored[$corner]
                if ($null -eq $index -or $index -ge $totalVerts) {
                    if (Test-CompactFaceStream `
                            $bytes $pIndex $numFaces `
                            $totalVerts $payloadSize) {
                        return [pscustomobject]@{
                            File = $File.Name
                            Node = "$nodeName#$recordIndex"
                            Reason = "compact 8-bit face stream matching gl_RenderNode indices; audit remaining streams separately"
                        }
                    }
                    return [pscustomobject]@{
                        File = $File.Name
                        Node = "$nodeName#$recordIndex"
                        Reason = "face $face indices [$($stored -join ',')] exceed $totalVerts vertices"
                    }
                }
            }
            $cornerCount += $faceCorners
        }

        $cornerRanges = @(
            @("pNormal", $pNormal, $cornerCount * 4),
            @("pColor", $pColor, $cornerCount * 4)
        )
        foreach ($range in $cornerRanges) {
            if (-not (Test-PayloadRange $range[1] $range[2] $payloadSize)) {
                return [pscustomobject]@{
                    File = $File.Name
                    Node = "$nodeName#$recordIndex"
                    Reason = "$($range[0])=$($range[1]) + $($range[2]) exceeds payload ($cornerCount corners)"
                }
            }
        }

        for ($child = $numChildren - 1; $child -ge 0; --$child) {
            $childOffset = if ($child -lt 7) {
                $recordOffset + 112 + $child * 4
            } else {
                $recordOffset + 140
            }
            $pending.Push((Read-Int32 $bytes $childOffset))
        }
    }

    return $null
}

$files = Get-ChildItem -LiteralPath $ModelDirectory -Filter *.bmd |
    Sort-Object Name
if ($Name.Count -ne 0) {
    $wanted = [Collections.Generic.HashSet[string]]::new(
        [string[]]$Name,
        [StringComparer]::OrdinalIgnoreCase)
    $files = $files | Where-Object {
        $wanted.Contains($_.BaseName)
    }
}

$failures = foreach ($file in $files) {
    Inspect-Bmd $file
}

if ($null -eq $failures) {
    Write-Output "All $($files.Count) BMD files satisfy the geometry assumptions."
    exit 0
}

$failures | Format-Table -AutoSize -Wrap
exit 1
