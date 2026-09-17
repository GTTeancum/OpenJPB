"""List XLV batches intersecting a world-space box around a smoke position."""
import argparse
from pathlib import Path
import struct


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('level', type=Path)
    p.add_argument('x', type=float)
    p.add_argument('y', type=float)
    p.add_argument('z', type=float)
    p.add_argument('--radius', type=float, default=5000)
    a = p.parse_args()
    data = a.level.read_bytes()
    magic, version, index, count, _ = struct.unpack_from('<5I', data)
    if (magic, version) != (0x564c584a, 2):
        p.error('Expected an indexed XLV v2 file')
    offset = 24  # XLV v2 includes total unique-vertex count after the v1 header.
    for batch in range(count):
        vertices, render_pass, mesh_index, mesh_count, unique = \
            struct.unpack_from('<5I', data, offset)
        offset += 20
        texture = data[offset:offset + 256].split(b'\0', 1)[0].decode(errors='replace')
        name = data[offset + 256:offset + 384].split(b'\0', 1)[0].decode(errors='replace')
        offset += 384
        positions = [struct.unpack_from('<3f', data, offset + 44*i)
                     for i in range(unique)]
        offset += unique*44 + vertices*2
        bounds = [(min(v[axis] for v in positions), max(v[axis] for v in positions))
                  for axis in range(3)]
        if all(lo <= center + a.radius and hi >= center - a.radius
               for (lo, hi), center in zip(bounds, (a.x, a.y, a.z))):
            print(f'{batch:3} pass={render_pass} triangles={vertices//3:5} '
                  f'texture={texture!r} mesh={name!r} bounds={bounds}')


if __name__ == '__main__':
    main()
