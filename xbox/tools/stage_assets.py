"""Copy a private Xbox asset instance; downscale textures without changing originals."""
from pathlib import Path
import argparse
import json
from xbox_formats import Limits, convert

p = argparse.ArgumentParser()
p.add_argument('--game-root', type=Path, required=True)
p.add_argument('--destination', type=Path, required=True)
p.add_argument('--manifest', type=Path,
               default=Path('xbox/build/asset-manifest.json'),
               help='Repository-local staging manifest path')
p.add_argument('--max-texture', type=int, default=128)
p.add_argument('--front-texture', type=int, default=256,
               help='Default cap for front-end images before explicit UI overrides')
p.add_argument('--ui-selection', type=Path,
               default=Path('xbox/ui-texture-quality.json'),
               help='Source-capped UI texture resolutions')
p.add_argument('--gpu-world-texture', type=int, default=256,
               help='BC1 resolution cap for opaque JPX level textures')
p.add_argument('--gpu-model-texture', type=int, default=256,
               help='BC1 resolution cap for opaque MODEL textures')
p.add_argument('--gpu-level', action='append',
               help='Stage BC1 for a selected level; repeat for more levels')
a = p.parse_args()
source = (a.game_root / 'res').resolve()
destination = a.destination.resolve()
manifest_path = a.manifest.resolve()
if destination == source or source in destination.parents or destination in source.parents:
    raise SystemExit('Destination must be separate from original resources')
if a.max_texture < 16 or a.max_texture & (a.max_texture - 1):
    raise SystemExit('Texture limit must be a power of two, at least 16')
if a.front_texture < 16 or a.front_texture & (a.front_texture - 1):
    raise SystemExit('Front-end texture limit must be a power of two, at least 16')
if a.gpu_world_texture < 16 or a.gpu_world_texture & (a.gpu_world_texture - 1):
    raise SystemExit('GPU texture limit must be a power of two, at least 16')
if a.gpu_model_texture < 16 or a.gpu_model_texture & (a.gpu_model_texture - 1):
    raise SystemExit('GPU model texture limit must be a power of two, at least 16')
gpu_levels = {name.lower() for name in a.gpu_level} if a.gpu_level else None
ui_quality = {path.lower(): limit for path, limit in
              json.loads(a.ui_selection.read_text()).items()}
if any(not isinstance(limit, int) or limit < 16 or limit & (limit - 1)
       for limit in ui_quality.values()):
    raise SystemExit('UI texture limits must be powers of two, at least 16')
limits = Limits(a.max_texture, a.front_texture, a.gpu_world_texture,
                a.gpu_model_texture, ui_quality, gpu_levels)
records = []
for f in sorted(source.rglob('*')):
    if not f.is_file():
        continue
    rel = f.relative_to(source)
    if rel.parts[0].lower() == 'movies':
        continue  # Movies are staged separately (stage_movies.py).
    records.append(convert(f, destination / 'res' / rel, rel.as_posix(), limits))
manifest_path.parent.mkdir(parents=True, exist_ok=True)
manifest_path.write_text(json.dumps(records, indent=2))
print(json.dumps({'files': len(records), 'sourceBytes': sum(r['sourceBytes'] for r in records),
                  'stagedBytes': sum(r['stagedBytes'] for r in records)}))
