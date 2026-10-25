"""Boot each level straight into gameplay and capture it (test only).

Writes --out/<level>.png plus free memory and frame time per level to
--out/levels.json, for a quick visual sweep after renderer changes.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

REPO = Path(__file__).resolve().parents[2]
PAD = REPO / 'xbox/tools/xemu_pad.py'
ACTIVE = REPO / 'xbox/test-config/active'
LEVELS = ['fed', 'marsh', 'theed', 'palace', 'tato', 'corus1', 'ruins', 'streets',
          'hangar', 'core', 'mini1', 'mini2', 'mini3', 'mini4', 'corus2']

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('levels', nargs='*', default=LEVELS)
p.add_argument('--out', type=Path, default=REPO / 'xbox/test-artifacts/pad/level-sweep')
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=True)
results = []
for level in a.levels:
    ACTIVE.mkdir(parents=True, exist_ok=True)
    (ACTIVE / 'xbox-skip-frontend.txt').write_text('1')
    (ACTIVE / 'xbox-start-level.txt').write_text(level)
    (ACTIVE / 'xbox-boss.txt').unlink(missing_ok=True)
    subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(REPO / 'xbox/tools/boot_xemu.ps1'), '-GdbPort', '9248'],
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, check=True)
    time.sleep(15)
    record = {'level': level}
    run = subprocess.run([sys.executable, str(PAD), '--out', str(a.out),
                          'until jpb_XboxSessionStats >= 1 120',
                          'pressuntil A jpb_XboxStartupPhase == 8 240',
                          'pressuntil A jpb_XboxSmokeFrame >= 150 90',
                          'stick 0 -1 30', 'wait 30', f'shot {level}',
                          'read jpb_XboxFreePages', 'read jpb_XboxFrameMs'],
                         capture_output=True, text=True, cwd=REPO)
    lines = [json.loads(l) for l in run.stdout.splitlines() if l.startswith('{')]
    if run.returncode:
        record['error'] = (run.stderr.strip().splitlines() or ['failed'])[-1]
    else:
        record['freePages'] = int(lines[-2]['result'][0], 16)
        record['frameMs'] = int(lines[-1]['result'][0], 16)
    print(json.dumps(record), flush=True)
    results.append(record)
for marker in ('xbox-skip-frontend.txt', 'xbox-start-level.txt'):
    (ACTIVE / marker).unlink(missing_ok=True)
(a.out / 'levels.json').write_text(json.dumps(results, indent=2) + '\n')
