"""Shrink CPU-side copies of textures that the Xbox GPU draws from BC1.

The GPU renderer loads <name>.xbt for opaque model and world surfaces; the
staged TGA/PNG of the same name is then only a lookup key (and a fallback for
rare translucent uses). Texture coordinates are normalized, so a smaller CPU
copy does not move them. Run after stage_assets.py and upgrade_textures.py.
Only the private Xbox asset copy and its manifest change.
"""
import argparse
import json
from pathlib import Path

from xbox_formats import cpu_copy_candidates, sha256, shrink_cpu_copy

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
    for image_path in cpu_copy_candidates(root):
        result = shrink_cpu_copy(image_path, a.size)
        if result is None:
            continue
        rel = image_path.relative_to(res).as_posix().lower()
        record = records.get(rel)
        if record is not None:
            record['stagedSize'] = result[1]
            record['stagedBytes'] = image_path.stat().st_size
            record['stagedSha256'] = sha256(image_path)
            record['cpuCopyForBc1'] = True
        saved += result[0]
        changed += 1
a.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
print(f'{changed} CPU texture copies reduced to {a.size} px, {saved / 1048576:.1f} MiB smaller on disc')
