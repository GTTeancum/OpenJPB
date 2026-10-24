"""Shrink CPU-side copies of textures that the Xbox GPU draws from BC1.

The GPU renderer loads <name>.xbt for opaque model and world surfaces; the
staged TGA/PNG of the same name is then only a lookup key (and a fallback for
rare translucent uses). Texture coordinates are normalized, so a smaller CPU
copy does not move them. Run after stage_assets.py and upgrade_textures.py.
Only the private Xbox asset copy and its manifest change.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--staged', type=Path, default=Path('xbox/build/staged-disc'))
p.add_argument('--manifest', type=Path, default=Path('xbox/build/asset-manifest.json'))
p.add_argument('--size', type=int, default=64)
a = p.parse_args()

res = a.staged / 'res'
manifest = json.loads(a.manifest.read_text())
records = {r['path'].lower(): r for r in manifest}
saved = changed = 0
roots = [res / 'MODEL' / 'tga'] + [d for d in (res / 'level' / 'jpx').iterdir() if d.is_dir()]
for root in roots:
    stems = {x.stem.lower() for x in root.glob('*.xbt')}
    for image_path in root.iterdir():
        if image_path.suffix.lower() not in ('.tga', '.png', '.bmp') or \
                image_path.stem.lower() not in stems:
            continue
        with Image.open(image_path) as im:
            if max(im.size) <= a.size:
                continue
            before = image_path.stat().st_size
            mode = 'RGBA' if 'A' in im.getbands() else 'RGB'
            small = im.convert(mode)
            small.thumbnail((a.size, a.size), Image.Resampling.LANCZOS)
        if image_path.suffix.lower() == '.tga':
            small.save(image_path, compression=None)
        else:
            small.save(image_path)
        rel = image_path.relative_to(res).as_posix().lower()
        record = records.get(rel)
        if record is not None:
            record['stagedSize'] = list(small.size)
            record['stagedBytes'] = image_path.stat().st_size
            record['stagedSha256'] = hashlib.sha256(image_path.read_bytes()).hexdigest()
            record['cpuCopyForBc1'] = True
        saved += before - image_path.stat().st_size
        changed += 1
a.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
print(f'{changed} CPU texture copies reduced to {a.size} px, {saved / 1048576:.1f} MiB smaller on disc')
