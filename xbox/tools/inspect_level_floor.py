"""Find XLV triangles below a game-space X/Z point and name their materials."""
import argparse
from pathlib import Path
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('level', type=Path)
    parser.add_argument('x', type=float)
    parser.add_argument('z', type=float)
    parser.add_argument('--y', type=float, default=0)
    parser.add_argument('--radius', type=float, default=0)
    args = parser.parse_args()
    data = args.level.read_bytes()
    magic, version, _, count, _, _ = struct.unpack_from('<6I', data)
    if (magic, version) != (0x564c584a, 2):
        parser.error('Expected an indexed XLV v2 file')
    offset = 24
    hits = []
    for batch in range(count):
        vertex_count, render_pass, mesh_index, _, unique_count = \
            struct.unpack_from('<5I', data, offset)
        offset += 20
        texture = data[offset:offset + 256].split(b'\0', 1)[0].decode(errors='replace')
        name = data[offset + 256:offset + 384].split(b'\0', 1)[0].decode(errors='replace')
        offset += 384
        points = [struct.unpack_from('<3f', data, offset + 44*i)
                  for i in range(unique_count)]
        offset += unique_count*44
        indices = struct.unpack_from(f'<{vertex_count}H', data, offset)
        offset += vertex_count*2
        for tri in range(0, vertex_count, 3):
            a, b, c = (points[indices[tri+i]] for i in range(3))
            den = (b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
            if abs(den) < 0.001:
                continue
            u = ((b[2]-c[2])*(args.x-c[0])+(c[0]-b[0])*(args.z-c[2]))/den
            v = ((c[2]-a[2])*(args.x-c[0])+(a[0]-c[0])*(args.z-c[2]))/den
            w = 1-u-v
            if min(u, v, w) < -args.radius:
                continue
            y = u*a[1]+v*b[1]+w*c[1]
            hits.append((abs(y-args.y), y, batch, tri//3, render_pass,
                         texture, name, mesh_index))
    for _, y, batch, tri, render_pass, texture, name, mesh_index in sorted(hits)[:30]:
        print(f'y={y:.1f} batch={batch} tri={tri} pass={render_pass} '
              f'mesh_index={mesh_index} texture={texture!r} mesh={name!r}')
    print(f'{len(hits)} projected triangles at X={args.x}, Z={args.z}')


if __name__ == '__main__':
    main()
