"""Read unique Xbox SFX failures through the isolated XEMU monitor."""
import argparse
import json
from pathlib import Path
import re
import socket
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
p.add_argument('--port', type=int, default=9247)
p.add_argument('--played', action='store_true', help='Include the last 64 successful play requests')
a = p.parse_args()
symbols = {}
mapping = a.map.read_text()
for name in ('jpb_XboxSfxFailurePathCount', 'jpb_XboxSfxFailurePaths',
             'jpb_XboxSfxFailureReasons', 'jpb_XboxSfxFailureErrnos',
             'jpb_XboxSfxFailureFreePages', 'jpb_XboxSfxFailureTinyAlloc'):
    match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_'+name+r'\s+([\da-fA-F]+)', mapping)
    if not match:
        p.error(f'Missing {name} in current build map')
    symbols[name] = int(match[1], 16)

with socket.create_connection(('127.0.0.1', a.port), timeout=3) as monitor:
    monitor.settimeout(3)
    def response():
        data = b''
        while not data.endswith(b'(qemu) '):
            part = monitor.recv(65536)
            if not part:
                raise RuntimeError('Emulator monitor closed')
            data += part
        return data.decode(errors='replace')
    response()
    def words(address, count):
        monitor.sendall(f'x /{count}wx 0x{address:x}\n'.encode())
        values = []
        for line in response().splitlines():
            if re.match(r'^\s*[\da-fA-F]+:', line):
                values.extend(int(value, 16) for value in
                              re.findall(r'0x([\da-fA-F]+)', line))
        if len(values) < count:
            raise RuntimeError(f'Expected {count} words at 0x{address:x}; found {len(values)}')
        return values[:count]
    count = min(words(symbols['jpb_XboxSfxFailurePathCount'], 1)[0], 32)
    records = []
    for i in range(count):
        data = struct.pack('<48I', *words(symbols['jpb_XboxSfxFailurePaths']+192*i, 48))
        reason = words(symbols['jpb_XboxSfxFailureReasons']+4*i, 1)[0]
        error = words(symbols['jpb_XboxSfxFailureErrnos']+4*i, 1)[0]
        free_pages = words(symbols['jpb_XboxSfxFailureFreePages']+4*i, 1)[0]
        tiny_alloc = words(symbols['jpb_XboxSfxFailureTinyAlloc']+4*i, 1)[0]
        records.append(dict(path=data.split(b'\0', 1)[0].decode(errors='replace'),
                            reason=reason, errno=error, free_pages=free_pages,
                            tiny_alloc=bool(tiny_alloc)))
    counters = {}
    for name in ('cached', 'texture_count', 'plays', 'failures', 'jpb_XboxSmokeFrame',
                 'jpb_XboxFrameMs', 'jpb_XboxRuntimeMs', 'jpb_XboxWorldMs',
                 'jpb_XboxModelsMs', 'jpb_XboxEffectsMs', 'jpb_XboxHudMs',
                 'jpb_XboxPresentMs',
                 'jpb_XboxFramePhase', 'jpb_XboxRuntimeStage',
                 'jpb_XboxRunStageStep', 'jpb_XboxRunStageGameState', 'jpb_XboxResetStep',
                 'jpb_XboxGpuStage', 'jpb_XboxGpuLastPass',
                 'jpb_XboxBc1WorldCalls', 'jpb_XboxBc1CacheHits',
                 'jpb_XboxBc1Eligible', 'jpb_XboxBc1OpenFailures',
                 'jpb_XboxBc1Loads', 'jpb_XboxBc1LastMaterialType',
                 'jpb_XboxBc1ModelLoads', 'jpb_XboxBc1ModelOpenFailures',
                 'jpb_XboxGlowScanCount', 'jpb_XboxGpuTriangleCalls',
                 'jpb_XboxGpuStateChanges',
                 'jpb_XboxModelTriangleUs', 'jpb_XboxEffectsTriangleUs',
                 'jpb_XboxModelBatchFlushes',
                 'jpb_XboxFreePages'):
        match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_'+name+r'\s+([\da-fA-F]+)', mapping)
        if match:
            counters[name] = words(int(match[1], 16), 1)[0]
    played = []
    if a.played:
        addresses = {}
        for name in ('jpb_XboxSfxPlayedCount', 'jpb_XboxSfxPlayedPaths',
                     'jpb_XboxSfxPlayedFrames'):
            match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_'+name+r'\s+([\da-fA-F]+)', mapping)
            if not match:
                p.error(f'Missing {name} in current build map')
            addresses[name] = int(match[1], 16)
        total = words(addresses['jpb_XboxSfxPlayedCount'], 1)[0]
        for sequence in range(max(0, total-64), total):
            index = sequence % 64
            data = struct.pack('<24I', *words(addresses['jpb_XboxSfxPlayedPaths']+96*index, 24))
            frame = words(addresses['jpb_XboxSfxPlayedFrames']+4*index, 1)[0]
            played.append(dict(sequence=sequence,frame=frame,
                               path=data.split(b'\0', 1)[0].decode(errors='replace')))
    print(json.dumps(dict(counters=counters, unique_failures=records,
                          recent_plays=played), indent=2))
