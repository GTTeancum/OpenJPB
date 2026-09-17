"""Restage selected UI textures at their source-capped priority resolutions."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--game-root', type=Path, required=True)
p.add_argument('--destination', type=Path, required=True)
p.add_argument('--selection', type=Path,
               default=Path('xbox/ui-texture-quality.json'))
a = p.parse_args()
game = a.game_root.resolve()
destination = a.destination.resolve()
if game == destination or game in destination.parents or destination in game.parents:
    p.error('Source game and staged Xbox roots must be separate')
selection = json.loads(a.selection.read_text())
manifest_path = destination / 'asset-manifest.json'
manifest = json.loads(manifest_path.read_text())
records = {record['path'].lower(): record for record in manifest}
changed = []
for rel, limit in selection.items():
    if (not isinstance(rel, str) or not isinstance(limit, int) or
            limit < 16 or limit & (limit - 1) or
            Path(rel).suffix.lower() not in ('.tga', '.png')):
        p.error(f'Invalid UI texture selection: {rel}: {limit}')
    record = records.get(rel.lower())
    if not record:
        p.error(f'Texture is absent from the staged manifest: {rel}')
    source = game / 'res' / Path(record['path'])
    target = destination / 'res' / Path(record['path'])
    if hashlib.sha256(source.read_bytes()).hexdigest() != record['sourceSha256']:
        raise RuntimeError(f'Original texture changed since staging: {source}')
    with Image.open(source) as image:
        image = image.copy()
        image.thumbnail((limit, limit), Image.Resampling.LANCZOS)
        size = list(image.size)
        if source.suffix.lower() == '.tga':
            image.convert('RGBA' if 'A' in image.getbands() else 'RGB').save(
                target, compression=None)
        else:
            image.save(target)
    record['stagedSize'] = size
    record['stagedBytes'] = target.stat().st_size
    record['stagedSha256'] = hashlib.sha256(target.read_bytes()).hexdigest()
    changed.append({'path': rel, 'size': size, 'bytes': record['stagedBytes']})
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps(changed, indent=2))
