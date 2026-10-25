"""Fight each boss in XEMU (test only).

For every boss of the PC boss smoke (tools/smoke_bosses.ps1) this boots the
Xbox build straight into the level with the player at the PC spawn point and
the boss placement activated (xbox-boss.txt), keeps player one alive, walks
toward the boss and attacks through the remote pad, and records the boss's
energy over time with screenshots. Results go to --out/<boss>/ and
--out/bosses.json.
"""
import argparse
import json
import math
from pathlib import Path
import subprocess
import sys
import time

REPO = Path(__file__).resolve().parents[2]
PAD = REPO / 'xbox/tools/xemu_pad.py'
ACTIVE = REPO / 'xbox/test-config/active'

# Name, level, spawn, placement: the PC boss smoke matrix.
BOSSES = [
    ('fed-droid-fighter', 'fed', (-22656, 5376, -9856), 128),
    ('marsh-mtt', 'marsh', (21287, 4416, -12416), 78),
    ('theed-tank', 'theed', (-3200, 3328, -20007), 53),
    ('tato-maul', 'tato', (-20467, 9984, 12173), 92),
    ('core-maul', 'core', (31284, 2560, -17242), 11),
    ('corus1-thug', 'corus1', (-29056, 10752, -19968), 153),
    ('mini2-kadu', 'mini2', (21504, 4608, 24320), 0),
    ('mini3-boss-nass', 'mini3', (12800, 13824, -14093), 4),
]

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('names', nargs='*', help='boss names (default: all)')
p.add_argument('--out', type=Path, default=REPO / 'xbox/test-artifacts/pad/bosses')
p.add_argument('--rounds', type=int, default=60)
p.add_argument('--finish', action='store_true',
               help='after the fight rounds, set the boss to 1 energy and finish it')
p.add_argument('--attack', default='Y,Y,B', help='attack button(s), comma separated (Y and B attack; X uses an item)')
a = p.parse_args()


def pad(out, *steps):
    result = subprocess.run([sys.executable, str(PAD), '--out', str(out), *steps],
                            capture_output=True, text=True, cwd=REPO)
    if result.returncode:
        raise RuntimeError((result.stderr or 'pad failed').strip().splitlines()[-1])
    return [json.loads(l) for l in result.stdout.splitlines() if l.startswith('{')]


def words(out, symbol, count, signed=False, as_float=False):
    import struct
    values = [int(v, 16) for v in pad(out, f'read {symbol} {count}')[0]['result']]
    if as_float:
        return [struct.unpack('<f', struct.pack('<I', v))[0] for v in values]
    if signed:
        return [v - (1 << 32) if v & 0x80000000 else v for v in values]
    return values


def boot(level, spawn, placement):
    ACTIVE.mkdir(parents=True, exist_ok=True)
    (ACTIVE / 'xbox-skip-frontend.txt').write_text('1')
    (ACTIVE / 'xbox-start-level.txt').write_text(level)
    (ACTIVE / 'xbox-boss.txt').write_text(f'{spawn[0]} {spawn[1]} {spawn[2]} {placement}\n')
    subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(REPO / 'xbox/tools/boot_xemu.ps1'), '-GdbPort', '9248'],
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, check=True)
    time.sleep(15)  # XEMU boots the BIOS before the guest map is readable.


def stick_toward(player, boss, camera):
    """Camera-relative stick (x right, y negative = away from camera)."""
    fx, fz = player[0] - camera[0], player[2] - camera[2]
    length = math.hypot(fx, fz) or 1
    fx, fz = fx / length, fz / length
    rx, rz = fz, -fx
    dx, dz = boss[0] - player[0], boss[2] - player[2]
    distance = math.hypot(dx, dz) or 1
    dx, dz = dx / distance, dz / distance
    return dx * rx + dz * rz, -(dx * fx + dz * fz), distance


summary = []
chosen = [b for b in BOSSES if not a.names or b[0] in a.names]
for name, level, spawn, placement in chosen:
    out = a.out / name
    out.mkdir(parents=True, exist_ok=True)
    record = {'boss': name, 'level': level, 'placement': placement}
    try:
        boot(level, spawn, placement)
        pad(out, 'until jpb_XboxSessionStats >= 1 120',
            'pressuntil A jpb_XboxStartupPhase == 8 240',
            # Dismiss the level briefing; gameplay frames start after it.
            'pressuntil A jpb_XboxSmokeFrame >= 120 90', 'shot spawn')
        state = words(out, 'jpb_XboxBossTest', 8, signed=True)
        record['fixture'] = state[0]
        record['bossEnergyStart'] = state[2]
        record['bossSlot'] = state[4]
        pad(out, 'poke jpb_XboxBossKeepAlive 1')
        energies = [state[2]]
        attacks = a.attack.split(',')
        for round_index in range(a.rounds):
            state = words(out, 'jpb_XboxBossTest', 8, signed=True)
            world = words(out, 'jpb_XboxWorldDebug', 15, as_float=True)
            energies.append(state[2])
            if state[2] <= 0:
                break
            player, camera = world[0:3], world[12:15]
            boss = state[5:8]
            sx, sy, distance = stick_toward(player, boss, camera)
            steps = []
            if distance > 700:
                steps.append(f'stick {sx:.2f} {sy:.2f} 20')
            else:
                steps.append(f'stick {sx:.2f} {sy:.2f} 3')
                for button in attacks:
                    steps += [f'press {button} 4', 'wait 6']
            if round_index % 10 == 0:
                steps.append(f'shot fight{round_index:03d}')
            pad(out, *steps)
        state = words(out, 'jpb_XboxBossTest', 8, signed=True)
        if a.finish and state[2] > 0:
            # Defeat path: one hit from death, then keep attacking.
            pad(out, 'poke jpb_XboxBossSetEnergy 1', 'wait 2')
            for round_index in range(40):
                state = words(out, 'jpb_XboxBossTest', 8, signed=True)
                world = words(out, 'jpb_XboxWorldDebug', 15, as_float=True)
                energies.append(state[2])
                if state[2] <= 0:
                    break
                sx, sy, distance = stick_toward(world[0:3], state[5:8], world[12:15])
                steps = [f'stick {sx:.2f} {sy:.2f} {20 if distance > 700 else 3}']
                if distance <= 700:
                    for button in attacks:
                        steps += [f'press {button} 4', 'wait 6']
                pad(out, *steps)
            record['defeated'] = state[2] <= 0
            if record['defeated']:
                for index in range(6):
                    pad(out, 'wait 100', f'shot defeat{index}')
                record['sessionAfterDefeat'] = words(out, 'jpb_XboxSessionStats', 4)
        pad(out, 'wait 60', 'shot end')
        record['bossEnergyEnd'] = state[2]
        record['bossEnergyMin'] = min(e for e in energies if e >= 0) if any(e >= 0 for e in energies) else None
        record['playerEnergyEnd'] = state[3]
        record['frameMs'] = words(out, 'jpb_XboxFrameMs', 1)[0]
        record['freePages'] = words(out, 'jpb_XboxFreePages', 1)[0]
        record['energyTrace'] = energies
    except Exception as error:  # Keep going: one boss failing must not hide the rest.
        record['error'] = str(error)
    print(json.dumps({k: v for k, v in record.items() if k != 'energyTrace'}), flush=True)
    summary.append(record)
for marker in ('xbox-skip-frontend.txt', 'xbox-start-level.txt', 'xbox-boss.txt'):
    (ACTIVE / marker).unlink(missing_ok=True)
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'bosses.json').write_text(json.dumps(summary, indent=2) + '\n')
