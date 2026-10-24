"""Write the authored sizes of 2D images that staging reduced.

Menu and HUD code addresses sub-images in authored pixel coordinates. The
Xbox GPU path reads res/xbox-image-sizes.txt to rescale those rectangles
onto the smaller staged images. Run after stage_assets.py and the texture
upgrade tools (they all update the manifest).
"""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--manifest', type=Path, default=Path('xbox/build/asset-manifest.json'))
p.add_argument('--staged', type=Path, default=Path('xbox/build/staged-disc'))
a = p.parse_args()
# World and model textures use normalized UVs; only 2D screen art matters.
SCREEN_ROOTS = ('front/', 'platform/', 'effects/', 'default/')
lines = []
for record in json.loads(a.manifest.read_text()):
    path = record['path'].replace('\\', '/')
    original, staged = record.get('originalSize'), record.get('stagedSize')
    if not original or not staged or original == staged:
        continue
    if not path.lower().startswith(SCREEN_ROOTS):
        continue
    lines.append(f'{path.lower()} {original[0]} {original[1]}')
lines.sort()
out = a.staged / 'res' / 'xbox-image-sizes.txt'
out.write_text('\n'.join(lines) + '\n', newline='\n')
print(f'{len(lines)} reduced screen images -> {out}')
