"""Raise selected opaque Xbox BC1 textures from original assets, in place.

The source game is read-only. The staged manifest records exact resolution and
hash changes so the private Xbox instance remains reproducible and auditable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image

from bc1 import encode as encode_bc1

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-root', type=Path, required=True)
parser.add_argument('--destination', type=Path, required=True)
parser.add_argument('--selection', type=Path,
                    default=Path('xbox/texture-quality.json'))
args = parser.parse_args()
game = args.game_root.resolve()
destination = args.destination.resolve()
if game == destination or game in destination.parents or destination in game.parents:
    parser.error('Source game and staged Xbox roots must be separate')

selection = json.loads(args.selection.read_text())
if not isinstance(selection, dict) or not selection:
    parser.error('Selection must be a nonempty path-to-size JSON object')
manifest_path = destination / 'asset-manifest.json'
manifest = json.loads(manifest_path.read_text())
records = {record['path'].lower(): record for record in manifest}
changed = []
for rel, limit in selection.items():
    if not isinstance(rel, str) or not isinstance(limit, int) or limit < 16 or limit & (limit - 1):
        parser.error(f'Invalid texture selection: {rel}: {limit}')
    key = rel.lower()
    record = records.get(key)
    if not record:
        parser.error(f'Texture is absent from the staged manifest: {rel}')
    source = game / 'res' / Path(record['path'])
    staged = destination / 'res' / Path(record['path'])
    if hashlib.sha256(source.read_bytes()).hexdigest() != record['sourceSha256']:
        raise RuntimeError(f'Original texture changed since staging: {source}')
    if 'gpuTexture' not in record:
        raise RuntimeError(f'No existing BC1 texture to upgrade: {rel}')
    with Image.open(source) as image:
        image = image.copy()
        image.thumbnail((limit, limit), Image.Resampling.LANCZOS)
        payload = encode_bc1(image)
        width, height = image.size
    target = destination / 'res' / record['gpuTexture']
    if target != staged.with_suffix('.xbt'):
        raise RuntimeError(f'Unexpected staged BC1 location: {target}')
    data = struct.pack('<4sIII', b'XBT1', width, height, len(payload)) + payload
    target.write_bytes(data)
    record['gpuSize'] = [width, height]
    record['gpuBytes'] = len(data)
    record['gpuSha256'] = hashlib.sha256(data).hexdigest()
    changed.append({'path': rel, 'size': [width, height], 'bytes': len(data)})
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps(changed, indent=2))
