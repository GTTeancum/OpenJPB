"""Convert PC mod packages (mods/<id>/mod.json + res/) for the Xbox build.

Each package keeps its PC layout and unchanged mod.json; its resources are
written in their Xbox form by the same functions that stage the stock game
(xbox_formats.py). The converted package gains:

  xbox-package.json     conversion stamp; the game rejects packages whose
                        xboxFormat differs from the one it reads
  xbox-image-sizes.txt  authored sizes of screen images conversion reduced

Package files byte-identical to the installed game's own resource at the same
path are left out when their Xbox form matches the stock staging: the stock
copy already serves that path. Large character atlases are kept at higher
detail than stock textures (xbox_formats.Limits.model_atlas_detail), so they
are always written into the package.

To add a character, put its PC package in a mods folder, run this tool, and
place the output folder under the Xbox game's mods\\ directory (the staged
disc's mods\\ for XEMU). Re-run the tool for every package whenever the game
reports that a package needs converting again.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import sys

from PIL import Image

from xbox_formats import (Limits, XBOX_FORMAT, convert, cpu_copy_candidates,
                          gpu_model_limit, screen_size_line, shrink_cpu_copy)

REPO = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__,
                            formatter_class=argparse.RawDescriptionHelpFormatter)
p.add_argument('packages', nargs='*', help='package folder names (default: every enabled package)')
p.add_argument('--mods', type=Path, default=Path(r'C:\Games\Star Wars Jedi Power Battles\mods'),
               help='PC mods folder to read')
p.add_argument('--game-root', type=Path, default=Path(r'C:\Games\Star Wars Jedi Power Battles'),
               help='installed PC game, for resources packages repeat unchanged')
p.add_argument('--destination', type=Path, default=REPO / 'xbox/build/staged-disc/mods',
               help='Xbox mods folder to write')
p.add_argument('--ui-selection', type=Path, default=REPO / 'xbox/ui-texture-quality.json')
p.add_argument('--cpu-copy-size', type=int, default=64)
a = p.parse_args()

mods = a.mods.resolve()
destination = a.destination.resolve()
if destination == mods or mods in destination.parents or destination in mods.parents:
    p.error('Destination must be separate from the PC mods folder')
stock = a.game_root.resolve() / 'res'
limits = Limits(ui_quality={k.lower(): v for k, v in
                            json.loads(a.ui_selection.read_text()).items()},
                model_atlas_detail=True)
stock_limits = Limits()


def converts_like_stock(rel, source):
    """Whether this package's Xbox form of a file equals the stock staging."""
    if rel.suffix.lower() != '.tga' or rel.parts[0].lower() != 'model':
        return True
    with Image.open(source) as im:
        size = max(im.size)
    return gpu_model_limit(size, limits) == gpu_model_limit(size, stock_limits)

if a.packages:
    folders = [mods / name for name in a.packages]
else:
    folders = sorted(d for d in mods.iterdir() if (d / 'mod.json').is_file())

summary = []
for folder in folders:
    manifest_path = folder / 'mod.json'
    if not manifest_path.is_file():
        sys.exit(f'{folder} has no mod.json')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest.get('enabled', True) is False and not a.packages:
        summary.append({'package': folder.name, 'skipped': 'disabled'})
        continue
    if manifest.get('version') != 1:
        sys.exit(f'{folder.name}: unsupported manifest version {manifest.get("version")}')
    referenced = {manifest[key].replace('\\', '/').lower()
                  for key in ('bmd', 'cad', 'cmb', 'portrait') if key in manifest}
    referenced |= {path.replace('\\', '/').lower() for path in manifest.get('saberIcons', [])}

    out = destination / folder.name
    if out.exists():
        shutil.rmtree(out)  # Reconversion replaces the whole package.
    out.mkdir(parents=True)
    shutil.copy2(manifest_path, out / 'mod.json')
    records, omitted = [], 0
    for source in sorted((folder / 'res').rglob('*')):
        if not source.is_file():
            continue
        rel = PurePosixPath(source.relative_to(folder / 'res').as_posix())
        stock_file = stock / rel
        if (f'res/{rel.as_posix()}'.lower() not in referenced and stock_file.is_file() and
                stock_file.read_bytes() == source.read_bytes() and
                converts_like_stock(rel, source)):
            omitted += 1
            continue
        records.append(convert(source, out / 'res' / rel, rel, limits))

    by_path = {r['path'].lower(): r for r in records}
    for directory in sorted({d.parent for d in (out / 'res').rglob('*.xbt')}):
        for image in cpu_copy_candidates(directory):
            result = shrink_cpu_copy(image, a.cpu_copy_size)
            record = by_path.get(image.relative_to(out / 'res').as_posix().lower())
            if result and record:
                record['stagedSize'] = result[1]
                record['cpuCopyForBc1'] = True

    sizes = sorted(filter(None, map(screen_size_line, records)))
    if sizes:
        (out / 'xbox-image-sizes.txt').write_text('\n'.join(sizes) + '\n', newline='\n')
    stamp = {
        'xboxFormat': XBOX_FORMAT,
        'id': manifest.get('id'),
        'source': {
            'manifestSha256': hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
            'files': len(records) + omitted,
            'omittedAsStock': omitted,
        },
    }
    (out / 'xbox-package.json').write_text(json.dumps(stamp, indent=2) + '\n', newline='\n')
    size = sum(f.stat().st_size for f in out.rglob('*') if f.is_file())
    summary.append({'package': folder.name, 'files': len(records), 'omittedAsStock': omitted,
                    'bytes': size})
    print(json.dumps(summary[-1]), flush=True)
print(json.dumps({'converted': sum('files' in s for s in summary),
                  'bytes': sum(s.get('bytes', 0) for s in summary)}))
