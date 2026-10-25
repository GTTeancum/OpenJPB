"""Write the authored sizes of 2D images that staging reduced.

Menu and HUD code addresses sub-images in authored pixel coordinates. The
Xbox GPU path reads res/xbox-image-sizes.txt to rescale those rectangles
onto the smaller staged images. Run after stage_assets.py and the texture
upgrade tools (they all update the manifest).
"""
import argparse
import json
from pathlib import Path

from xbox_formats import screen_size_line

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--manifest', type=Path, default=Path('xbox/build/asset-manifest.json'))
p.add_argument('--staged', type=Path, default=Path('xbox/build/staged-disc'))
a = p.parse_args()
lines = [line for line in map(screen_size_line, json.loads(a.manifest.read_text()))
         if line]
lines.sort()
out = a.staged / 'res' / 'xbox-image-sizes.txt'
out.write_text('\n'.join(lines) + '\n', newline='\n')
print(f'{len(lines)} reduced screen images -> {out}')
