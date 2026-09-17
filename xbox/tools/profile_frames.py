"""Read Xbox frame timing markers through the isolated XEMU monitor.

Read-only guest memory access: no host input, window capture, or game control.
"""
import argparse
import json
from pathlib import Path
import re
import socket
import statistics
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
parser.add_argument('--port', type=int, default=9247)
parser.add_argument('--seconds', type=float, default=15)
parser.add_argument('--interval', type=float, default=0.1)
parser.add_argument('--start-frame', type=int,
                    help='Wait for this in-game frame before measuring')
parser.add_argument('--end-frame', type=int,
                    help='Stop once this in-game frame is reached')
parser.add_argument('--out', type=Path)
args = parser.parse_args()
if args.seconds <= 0 or args.interval <= 0:
    parser.error('seconds and interval must be positive')
if args.end_frame is not None and (args.start_frame is None or
                                   args.end_frame <= args.start_frame):
    parser.error('--end-frame requires a smaller --start-frame')

names = ('jpb_XboxSmokeFrame', 'jpb_XboxFrameMs', 'jpb_XboxRuntimeMs',
         'jpb_XboxWorldMs', 'jpb_XboxModelsMs', 'jpb_XboxEffectsMs',
         'jpb_XboxHudMs', 'jpb_XboxPresentMs', 'jpb_XboxFreePages',
         'jpb_XboxScreenPolyMs', 'jpb_XboxHudReplayMs',
         'jpb_XboxCompositeUploadMs', 'jpb_XboxCompositeFinishMs')
optional_names = ('jpb_XboxEnemyClasses', 'jpb_XboxBc1ModelLoads',
                  'jpb_XboxBc1Loads', 'jpb_XboxGpuTriangleCalls',
                  'jpb_XboxModelBatchFlushes', 'jpb_XboxSceneMs',
                  'jpb_XboxMenuMs', 'jpb_XboxGameStageMs',
                  'jpb_XboxTextureCount', 'jpb_XboxRawTextureBytes',
                  'jpb_XboxSubpixelTriangles')
mapping = args.map.read_text()
addresses = {}
for name in names:
    match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_' + name +
                      r'\s+([\da-fA-F]+)', mapping)
    if not match:
        parser.error(f'Missing marker in map: {name}')
    addresses[name] = int(match[1], 16)
for name in optional_names:
    match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_' + name +
                      r'\s+([\da-fA-F]+)', mapping)
    if match:
        addresses[name] = int(match[1], 16)
names = names + tuple(name for name in optional_names if name in addresses)

with socket.create_connection(('127.0.0.1', args.port), timeout=3) as monitor:
    monitor.settimeout(3)

    def response():
        result = b''
        while not result.endswith(b'(qemu) '):
            part = monitor.recv(16384)
            if not part:
                raise RuntimeError('XEMU monitor closed')
            result += part
        return result.decode(errors='replace')

    response()

    def read(name):
        address = addresses[name]
        monitor.sendall(f'x /1wx 0x{address:x}\n'.encode())
        match = re.search(r'(?m)^0*' + f'{address:x}' +
                          r':\s+0x([\da-fA-F]+)', response())
        return int(match[1], 16) if match else None

    if args.start_frame is not None:
        wait_deadline = time.monotonic() + 120
        while True:
            current = read('jpb_XboxSmokeFrame')
            if current is not None and args.start_frame <= current < 100000:
                break
            if time.monotonic() >= wait_deadline:
                raise TimeoutError(f'In-game frame {args.start_frame} not reached')
            time.sleep(.05)
    start = time.monotonic()
    samples = []
    while time.monotonic() - start < args.seconds:
        sample = {'time': time.monotonic() - start}
        for name in names:
            sample[name] = read(name)
        samples.append(sample)
        if (args.end_frame is not None and
                sample['jpb_XboxSmokeFrame'] is not None and
                sample['jpb_XboxSmokeFrame'] >= args.end_frame):
            break
        time.sleep(args.interval)

valid = [s for s in samples if s['jpb_XboxSmokeFrame'] is not None]
if len(valid) < 2:
    raise RuntimeError('Not enough mapped game frames were observed')
elapsed = valid[-1]['time'] - valid[0]['time']
frames = valid[-1]['jpb_XboxSmokeFrame'] - valid[0]['jpb_XboxSmokeFrame']
result = {'firstFrame': valid[0]['jpb_XboxSmokeFrame'],
          'lastFrame': valid[-1]['jpb_XboxSmokeFrame'],
          'elapsedSeconds': elapsed, 'fps': frames / elapsed,
          'samples': len(valid), 'timingsMs': {}}
for name in names[1:8] + names[9:13]:
    values = [s[name] for s in valid if s[name] is not None]
    if values:
        result['timingsMs'][name] = {
            'median': statistics.median(values),
            'p90': sorted(values)[int(.9 * (len(values) - 1))],
            'max': max(values)}
result['freePages'] = min(s['jpb_XboxFreePages'] for s in valid
                          if s['jpb_XboxFreePages'] is not None)
for name in optional_names:
    if name in addresses:
        result[name] = [valid[0][name], valid[-1][name]]
for name in ('jpb_XboxSceneMs', 'jpb_XboxMenuMs', 'jpb_XboxGameStageMs'):
    if name in addresses:
        values = [s[name] for s in valid if s[name] is not None]
        result['timingsMs'][name] = {
            'median': statistics.median(values),
            'p90': sorted(values)[int(.9 * (len(values) - 1))],
            'max': max(values)}
print(json.dumps(result, indent=2))
if args.out:
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps({'summary': result, 'samples': samples},
                                   indent=2) + '\n')
