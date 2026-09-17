"""Validate pointer-free XLV records and report bounds/material counts."""
import argparse
import collections
import hashlib
import math
from pathlib import Path
import struct
import json

parser = argparse.ArgumentParser()
parser.add_argument('level', type=Path)
args = parser.parse_args()
data = args.level.read_bytes()
magic, version, level, batches, count = struct.unpack_from('<5I', data)
assert magic == 0x564c584a and version in (1, 2)
offset = 20
unique_total = count
if version == 2:
    unique_total, = struct.unpack_from('<I', data, offset)
    offset += 4
observed_unique = 0
total = 0
materials = set()
passes = collections.Counter()
low = [math.inf]*3
high = [-math.inf]*3
for index in range(batches):
    vertices, render_pass, mesh_index, mesh_count = struct.unpack_from('<4I', data, offset)
    offset += 16
    unique = vertices
    if version == 2:
        unique, = struct.unpack_from('<I', data, offset)
        offset += 4
        assert unique <= 65536
    assert vertices % 3 == 0 and render_pass in (0, 1, 2) and mesh_index < mesh_count
    names = data[offset:offset+384]
    assert len(names) == 384 and names[255] == 0 and names[383] == 0
    materials.add(names[:256].split(b'\0')[0].decode('utf-8'))
    offset += 384
    for vertex in range(unique):
        fields = struct.unpack_from('<11f', data, offset)
        assert all(math.isfinite(v) for v in fields)
        for axis in range(3):
            low[axis] = min(low[axis], fields[axis])
            high[axis] = max(high[axis], fields[axis])
        offset += 44
    if version == 2:
        indices = struct.unpack_from(f'<{vertices}H', data, offset)
        assert all(i < unique for i in indices)
        offset += vertices*2
    observed_unique += unique
    total += vertices
    passes[render_pass] += vertices // 3
assert offset == len(data) and total == count and observed_unique == unique_total
assert any(high[i] > low[i] for i in range(3)), 'all geometry has collapsed to one point'
print(json.dumps(dict(version=version, level=level, batches=batches, vertices=count,
    unique_vertices=unique_total,
    triangles=count//3, materials=len(materials), bounds=[low, high], passes=dict(passes),
    bytes=len(data), sha256=hashlib.sha256(data).hexdigest()), indent=2))
