"""Rebase repeating BC1 V per XLV triangle without changing authored world positions.

The Xbox GPU addresses BC1 vertically at twice the nominal rate. Its world
texture upload appends a second payload so triangles may span one UV tile
boundary. This tool gives each triangle a local V origin and duplicates only
vertices whose UVs differ between adjacent triangles.
"""
import argparse
import math
from pathlib import Path
import struct


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--texture-dir', type=Path, required=True)
    a = p.parse_args()
    data = a.source.read_bytes()
    magic, version, level, count, total, _ = struct.unpack_from('<6I', data)
    if (magic, version) != (0x564c584a, 2):
        p.error('Expected indexed XLV v2')
    offset = 24
    records = []
    unique_total = 0
    rebased = 0
    wide = 0
    for batch in range(count):
        vertices, render_pass, mesh_index, mesh_count, unique = \
            struct.unpack_from('<5I', data, offset)
        offset += 20
        names = data[offset:offset + 384]
        texture = names[:256].split(bytes([0]), 1)[0].decode(errors='replace')
        offset += 384
        original = [data[offset + i*44:offset + (i+1)*44]
                    for i in range(unique)]
        offset += unique*44
        indices = struct.unpack_from(f'<{vertices}H', data, offset)
        offset += vertices*2
        xbt = a.texture_dir / (Path(texture).stem + '.xbt')
        if render_pass == 0 and xbt.is_file():
            revised = []
            lookup = {}
            new_indices = []
            for tri in range(0, vertices, 3):
                source_vertices = [original[indices[tri + j]] for j in range(3)]
                v = [struct.unpack_from('<f', vertex, 16)[0]
                     for vertex in source_vertices]
                first_tile = math.floor(min(v))
                if max(v) - min(v) > 1.0:
                    wide += 1
                for vertex, value in zip(source_vertices, v):
                    replacement = bytearray(vertex)
                    struct.pack_into('<f', replacement, 16, value - first_tile)
                    replacement = bytes(replacement)
                    if replacement not in lookup:
                        lookup[replacement] = len(revised)
                        revised.append(replacement)
                    new_indices.append(lookup[replacement])
                rebased += 1
        else:
            revised = original
            new_indices = indices
        for old_index, new_index in zip(indices, new_indices):
            before = original[old_index]
            after = revised[new_index]
            if before[:16] + before[20:] != after[:16] + after[20:]:
                p.error(f'Batch {batch} changed non-V vertex data')
        if len(revised) > 65536:
            p.error(f'Batch {batch} exceeds 16-bit index limit: {len(revised)}')
        records.append(struct.pack('<5I', vertices, render_pass, mesh_index,
                                   mesh_count, len(revised)) + names +
                       b''.join(revised) + struct.pack(f'<{vertices}H', *new_indices))
        unique_total += len(revised)
    if offset != len(data):
        p.error('Unexpected trailing XLV bytes')
    output = struct.pack('<6I', magic, version, level, count, total,
                         unique_total) + b''.join(records)
    a.output.write_bytes(output)
    print(f'batches={count} vertices={total} unique={unique_total} '
          f'rebased_triangles={rebased} triangles_spanning_multiple_tiles={wide} '
          f'bytes={len(output)} non_v_vertex_roundtrip=exact')


if __name__ == '__main__':
    main()
