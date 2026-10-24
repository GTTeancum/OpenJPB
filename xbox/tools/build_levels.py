"""Build the Xbox level geometry (.xlv) for every level with a PC FBX.

For each level: jpb_xbox_export_level (the exact PC FBX import) writes a v1
XLV, index_level.py removes duplicate vertices (verifying every vertex), and
tile_bc1_level.py rebases V per triangle for the BC1 world textures. The
result is written into the private staged disc only; PC assets are read-only.
"""
import argparse
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
# sLevelNames index order (src/reconstructed/original/level_world.c).
LEVELS = ["council", "fed", "marsh", "theed", "palace", "tato", "corus1",
          "ruins", "streets", "hangar", "core", "mini1", "mini2", "mini3",
          "mini4", "corus2", "train1", "train2", "train3", "train5",
          "train6", "train7", "train4", "train1", "train1", "arena"]

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--game-root', type=Path, default=Path('C:/Games/Star Wars Jedi Power Battles'))
p.add_argument('--staged', type=Path, default=REPO / 'xbox/build/staged-disc')
p.add_argument('--exporter', type=Path, default=REPO / 'build/Release/jpb_xbox_export_level.exe')
p.add_argument('--work', type=Path, default=REPO / 'xbox/test-artifacts/levels')
p.add_argument('--only', nargs='*', help='level names to build (default: all with an FBX)')
p.add_argument('--force', action='store_true', help='rebuild levels that already have an XLV')
a = p.parse_args()

a.work.mkdir(parents=True, exist_ok=True)
jpx = a.game_root / 'res' / 'level' / 'jpx'
directories = {d.name.lower(): d for d in jpx.iterdir() if d.is_dir()}
staged_jpx = a.staged / 'res' / 'level' / 'jpx'
staged_dirs = {d.name.lower(): d for d in staged_jpx.iterdir() if d.is_dir()}
done = set()
failures = []
for index, name in enumerate(LEVELS):
    if name in done or (a.only and name not in a.only):
        continue
    done.add(name)
    source_dir = directories.get(name)
    fbx = next((f for f in source_dir.iterdir() if f.name.lower() == name + '.fbx'), None) if source_dir else None
    if fbx is None or name not in staged_dirs:
        print(f'{name}: no FBX source; skipped')
        continue
    output = staged_dirs[name] / f'{name}.xlv'
    if output.exists() and not a.force:
        print(f'{name}: {output.name} present')
        continue
    raw = a.work / f'{name}-v1.xlv'
    indexed = a.work / f'{name}-v2.xlv'
    steps = [
        [str(a.exporter), str(fbx), str(index), str(raw)],
        [sys.executable, str(REPO / 'xbox/tools/index_level.py'), str(raw), str(indexed)],
        [sys.executable, str(REPO / 'xbox/tools/tile_bc1_level.py'), str(indexed), str(output),
         '--texture-dir', str(staged_dirs[name])],
    ]
    for command in steps:
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            failures.append(name)
            print(f'{name}: FAILED {Path(command[1] if command[0] == sys.executable else command[0]).name}: '
                  f'{(result.stderr or result.stdout).strip()[-400:]}')
            output.unlink(missing_ok=True)
            break
    else:
        raw.unlink(missing_ok=True)
        indexed.unlink(missing_ok=True)
        print(f'{name}: {output.stat().st_size / 1048576:.1f} MiB')
sys.exit(1 if failures else 0)
