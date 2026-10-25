"""Rough visible geometry for the Jedi Council chamber (feasibility preview).

The shipped game has no visual mesh for the council level (level 0): only
its collision/world archive (W3D/council.j3d), cameras, pickups and four
textures (level/3ds/council0..3). This builds a stand-in XLV from the
collision polygons, textured with those four images, and with --install
temporarily puts the chamber in place of the versus arena on the staged disc
(arena files are backed up to xbox/build/arena-backup and restored with
--restore). Not canonical content.
"""
import argparse
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import numpy as np

REPO = Path(__file__).resolve().parents[2]
GAME = Path(r'C:\Games\Star Wars Jedi Power Battles\res')
STAGED = REPO / 'xbox/build/staged-disc/res'
BACKUP = REPO / 'xbox/build/arena-backup'
SWAPPED = ['level/W3D/arena.j3d', 'level/CAMERAS/arena.cam', 'level/powerups/arena.pwr',
           'level/powerups/arena2.pwr', 'level/jpx/arena/arena.xlv']

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--install', action='store_true')
p.add_argument('--restore', action='store_true')
p.add_argument('--work', type=Path, default=REPO / 'xbox/test-artifacts/council')
a = p.parse_args()


def restore():
    for rel in SWAPPED:
        src = BACKUP / rel
        if src.exists():
            shutil.copy2(src, STAGED / rel)
    for n in range(4):
        (STAGED / f'level/jpx/arena/council{n}.tga').unlink(missing_ok=True)
    print('arena restored')


if a.restore:
    restore()
    sys.exit()

a.work.mkdir(parents=True, exist_ok=True)
dump = a.work / 'council.json'
subprocess.run([str(REPO / 'build/Release/jpb_level_import.exe'), '-',
                str(GAME / 'level/W3D/council.j3d'), '0', str(dump)], check=True)
faces = json.loads(dump.read_text())['collision']

floors, walls = [], []
for face in faces:
    v = np.array(face['vertices'], float)
    if len(v) < 3:
        continue
    cross = np.cross(v[1] - v[0], v[2] - v[0])
    length = np.linalg.norm(cross)
    if length == 0:
        continue
    (floors if abs(cross[1]) / length > 0.7 else walls).append(v)
main = [v for v in floors if abs(v[:, 1].mean() - 2816) < 40]
points = np.concatenate(main)
cx, cz = (points[:, 0].min() + points[:, 0].max()) / 2, (points[:, 2].min() + points[:, 2].max()) / 2
radius = max(np.hypot(points[:, 0] - cx, points[:, 2] - cz))
wall_points = np.concatenate(walls)
y_low, y_high = wall_points[:, 1].min(), wall_points[:, 1].max()

batches = {name: [] for name in ('council0.tga', 'council1.tga', 'council2.tga')}
for v in floors:
    centre = np.hypot(v[:, 0].mean() - cx, v[:, 2].mean() - cz)
    if abs(v[:, 1].mean() - 2816) < 40 and centre < radius * 0.95:
        # Quarter medallion, mirrored about the chamber centre.
        uv = np.stack([np.abs(v[:, 0] - cx) / radius, np.abs(v[:, 2] - cz) / radius], 1)
        batches['council0.tga'].append((v, np.clip(uv, 0, 1) * 0.99))
    else:
        uv = np.stack([(v[:, 0] % 512) / 512 * 0.25, (v[:, 2] % 512) / 512 * 0.25], 1)
        batches['council1.tga'].append((v, uv))
for v in walls:
    centre = np.hypot(v[:, 0].mean() - cx, v[:, 2].mean() - cz)
    if centre > radius * 0.85:
        # Outer windows: the Coruscant skyline wrapped around the chamber.
        angle = np.arctan2(v[:, 2] - cz, v[:, 0] - cx)
        u = (angle / (2 * math.pi) + 0.5) * 3
        if u.max() - u.min() > 1.5:
            u = np.where(u < 1.5, u + 3, u)
        u -= math.floor(u.min())
        vv = 1 - (v[:, 1] - y_low) / (y_high - y_low)
        batches['council2.tga'].append((v, np.stack([u, vv], 1)))
    else:
        uv = np.stack([((v[:, 0] + v[:, 2]) % 384) / 384 * 0.25,
                       1 - (v[:, 1] - y_low) / (y_high - y_low)], 1)
        batches['council1.tga'].append((v, uv))

records = []
total = unique_total = 0
for texture, polys in batches.items():
    vertices, indices = [], []
    for v, uv in polys:
        base = len(vertices)
        for point, (u, w) in zip(v, uv):
            vertices.append(struct.pack('<11f', point[0], point[1], point[2], float(u), float(w),
                                        200, 200, 200, 255, 0, 0))
        for k in range(1, len(v) - 1):  # fan; collision polygons are convex
            indices += [base, base + k, base + k + 1]
    names = texture.encode().ljust(256, b'\0') + b'council'.ljust(128, b'\0')
    records.append(struct.pack('<5I', len(indices), 0, len(records), len(batches), len(vertices)) +
                   names + b''.join(vertices) + struct.pack(f'<{len(indices)}H', *indices))
    total += len(indices)
    unique_total += len(vertices)
xlv = struct.pack('<6I', 0x564c584a, 2, 25, len(records), total, unique_total) + b''.join(records)
(a.work / 'council.xlv').write_bytes(xlv)
print(json.dumps({'triangles': total // 3, 'vertices': unique_total,
                  'batches': {k: len(v) for k, v in batches.items()},
                  'centre': [cx, cz], 'radius': radius}))

if a.install:
    for rel in SWAPPED:
        if not (BACKUP / rel).exists() and (STAGED / rel).exists():
            (BACKUP / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(STAGED / rel, BACKUP / rel)
    shutil.copy2(GAME / 'level/W3D/council.j3d', STAGED / 'level/W3D/arena.j3d')
    shutil.copy2(GAME / 'level/CAMERAS/council.cam', STAGED / 'level/CAMERAS/arena.cam')
    shutil.copy2(GAME / 'level/powerups/council.pwr', STAGED / 'level/powerups/arena.pwr')
    shutil.copy2(GAME / 'level/powerups/council2.pwr', STAGED / 'level/powerups/arena2.pwr')
    shutil.copy2(a.work / 'council.xlv', STAGED / 'level/jpx/arena/arena.xlv')
    from PIL import Image
    for n in range(4):
        Image.open(GAME / f'level/3ds/council{n}.tga').convert('RGB').save(
            STAGED / f'level/jpx/arena/council{n}.tga', compression=None)
    print('council installed in place of the versus arena (restore with --restore)')
