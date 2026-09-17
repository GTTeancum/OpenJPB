"""Capture the Xbox game's fixed smoke sequence through XEMU-native facilities.

Reads only the game's public frame marker through the emulator monitor and
uses the local OpenJKDF2 native screenshot helper. Never sends host input.
"""
import argparse
import json
from pathlib import Path
import re
import socket
import shutil
import subprocess
import sys
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--pid', type=int, required=True)
p.add_argument('--port', type=int, default=9247)
p.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
p.add_argument('--out', type=Path, required=True)
p.add_argument('--capture-dir', type=Path,
               help='Override the active XEMU config screenshot_dir')
p.add_argument('--frames', default='20,45,105,165,195,255,450,650')
p.add_argument('--timeout', type=int, default=180)
p.add_argument('--telemetry-only', action='store_true',
               help='Read process-local frame/input state without taking native screenshots')
p.add_argument('--helper', type=Path, default=Path('C:/Programming/GitHub/OpenJKDF2ogx/scripts/xbox/xemu_native_screenshot.py'))
p.add_argument('--exe', type=Path, default=Path('C:/Games/Emulators/Xemu/xemu.exe'))
a = p.parse_args()
if a.capture_dir is None:
    config = Path('xbox/build/xemu/xemu.toml').read_text()
    screenshot_dir = re.search(r"(?m)^screenshot_dir\s*=\s*['\"]([^'\"]+)['\"]", config)
    if not screenshot_dir:
        p.error('XEMU config has no screenshot_dir; pass --capture-dir')
    a.capture_dir = Path(screenshot_dir[1])
targets = [int(x) for x in a.frames.split(',')]
if targets != sorted(set(targets)) or not targets or targets[0] < 0:
    p.error('Frames must be distinct, increasing and nonnegative')
match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_jpb_XboxSmokeFrame\s+([\da-fA-F]+)', a.map.read_text())
if not match:
    p.error('Current build map does not contain the smoke frame marker')
address = int(match[1], 16)
telemetry_match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_jpb_XboxPlayerTelemetry\s+([\da-fA-F]+)', a.map.read_text())
telemetry_address = int(telemetry_match[1], 16) if telemetry_match else None
counter_addresses = {}
for counter in ('plays', 'failures', 'jpb_XboxFreePages'):
    counter_match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_'+counter+r'\s+([\da-fA-F]+)', a.map.read_text())
    if counter_match:
        counter_addresses[counter] = int(counter_match[1], 16)
a.out = a.out.resolve()
a.out.mkdir(parents=True, exist_ok=True)
if (a.out/'captures.json').exists():
    p.error('Use a fresh evidence directory')
records = []
deadline = time.monotonic() + a.timeout
with socket.create_connection(('127.0.0.1', a.port), timeout=2) as monitor:
    monitor.settimeout(2)
    def response():
        result = b''
        while not result.endswith(b'(qemu) '):
            part = monitor.recv(16384)
            if not part:
                raise RuntimeError('Emulator monitor closed')
            result += part
        return result.decode(errors='replace')
    response()
    def frame():
        monitor.sendall(f'x /1wx 0x{address:x}\n'.encode())
        text = response()
        match = re.search(r'(?m)^0*'+f'{address:x}'+r':\s+0x([\da-fA-F]+)', text)
        return int(match[1], 16) if match else None
    def telemetry():
        if telemetry_address is None:
            return None
        monitor.sendall(f'x /6wx 0x{telemetry_address:x}\n'.encode())
        lines = response().splitlines()
        words = []
        for line in lines:
            if re.match(r'^\s*[\da-fA-F]+:', line):
                words.extend(int(value, 16) for value in re.findall(r'0x([\da-fA-F]+)', line))
        return [value if value < 0x80000000 else value - 0x100000000 for value in words[:6]] if len(words) >= 6 else None
    def counter(name):
        if name not in counter_addresses:
            return None
        at = counter_addresses[name]
        monitor.sendall(f'x /1wx 0x{at:x}\n'.encode())
        match = re.search(r'(?m)^0*'+f'{at:x}'+r':\s+0x([\da-fA-F]+)', response())
        return int(match[1], 16) if match else None
    for target in targets:
        while True:
            if time.monotonic() > deadline:
                raise TimeoutError(f'Frame {target} not reached; retain earlier evidence')
            current = frame()
            # The executable is not mapped during the initial BIOS boot.
            if current is not None and target <= current < 100000:
                break
            time.sleep(.25)
        saved=None
        if not a.telemetry_only:
            for attempt in range(3):
                result = subprocess.run([sys.executable, str(a.helper), '--pid', str(a.pid),
                                         '--xemu-exe', str(a.exe), '--screenshot-dir', str(a.capture_dir.resolve()),
                                         '--timeout', '8'], capture_output=True, text=True)
                if result.returncode == 0:
                    break
                if attempt == 2:
                    raise RuntimeError(f'Native screenshot failed after three attempts: '
                                       f'{result.stdout.strip()} {result.stderr.strip()}')
                time.sleep(1)
            capture = json.loads(result.stdout.strip())
            if not capture.get('ok'):
                raise RuntimeError(capture)
            saved=a.out/Path(capture['path']).name
            shutil.copyfile(capture['path'], saved)
        record = dict(target=target, before=current, after=frame(),
                      telemetry=telemetry(), sfx_plays=counter('plays'),
                      sfx_failures=counter('failures'), free_pages=counter('jpb_XboxFreePages'),
                      path=str(saved) if saved else None)
        records.append(record)
        (a.out/'captures.json').write_text(json.dumps(records, indent=2)+'\n')
        print(json.dumps(record), flush=True)
