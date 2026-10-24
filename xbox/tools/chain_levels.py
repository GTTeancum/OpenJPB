"""Chain through story levels in one XEMU session (test only).

Starts from the level-select screen (to_level_select.ps1), then for each
level: wait for gameplay, play briefly, capture, force completion through
the game's own nextLevel owner, and confirm the results screen until the
next level loads. Records free memory per level.
"""
import argparse
import json
import subprocess
import sys
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--levels', type=int, default=14)
p.add_argument('--direct', action='store_true',
               help='already loading a level (boot_xemu with xbox-skip-frontend/start-level markers)')
p.add_argument('--out', type=Path, default=Path('xbox/test-artifacts/pad/chain'))
a = p.parse_args()
pad = Path(__file__).with_name('xemu_pad.py')


def run(*steps):
    result = subprocess.run([sys.executable, str(pad), '--out', str(a.out), *steps],
                            capture_output=True, text=True)
    lines = [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip().splitlines()[-1] if result.stderr else 'failed')
    return lines


summary = []
if not a.direct:
    run('press A')
for index in range(1, a.levels + 1):
    record = dict(order=index)
    try:
        run(f'until jpb_XboxSessionStats >= {index} 90',
            'pressuntil A jpb_XboxStartupPhase == 8 180',
            'pressuntil A jpb_XboxSmokeFrame >= 240 60',
            'stick 0 -1 90', 'press Y', 'wait 10', 'press Y', 'wait 30')
        reads = run('read jpb_XboxSessionStats+8', 'read jpb_XboxFreePages',
                    'read jpb_XboxFrameMs', f'shot level{index:02d}')
        record['level'] = int(reads[0]['result'][0], 16)
        record['free_pages'] = int(reads[1]['result'][0], 16)
        record['frame_ms'] = int(reads[2]['result'][0], 16)
        record['shot'] = reads[3]['result']
        if index < a.levels:
            run('poke nextLevel 2 1', f'until jpb_XboxSessionStats+4 >= {index} 60',
                'wait 60', f'shot results{index:02d}',
                f'pressuntil A jpb_XboxSessionStats >= {index + 1} 120')
    except Exception as error:
        record['error'] = str(error)
        print(json.dumps(record), flush=True)
        summary.append(record)
        break
    print(json.dumps(record), flush=True)
    summary.append(record)
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'chain.json').write_text(json.dumps(summary, indent=2) + '\n')
