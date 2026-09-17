"""Read completed frame diagnostics from the isolated XEMU guest."""
import argparse
import json
from pathlib import Path
import re
import socket
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
p.add_argument('--port', type=int, default=9247)
p.add_argument('--frame', type=int, default=700)
p.add_argument('--out', type=Path)
a = p.parse_args()
mapping = a.map.read_text()

def symbol(name):
    match = re.search(r'(?m)^\s+0000:[0-9a-fA-F]+\s+_' + name +
                      r'\s+([0-9a-fA-F]+)', mapping)
    if not match:
        p.error(f'Missing symbol: {name}')
    return int(match[1], 16)

frame_address = symbol('jpb_XboxSmokeFrame')
ring_address = symbol('jpb_XboxPerfRing')
field_count = 39
word_count = 512 * field_count
with socket.create_connection(('127.0.0.1', a.port), timeout=3) as monitor:
    monitor.settimeout(5)

    def response():
        data = b''
        while not data.endswith(b'(qemu) '):
            chunk = monitor.recv(32768)
            if not chunk:
                raise RuntimeError('XEMU monitor closed')
            data += chunk
        return data.decode(errors='replace')

    def command(value):
        monitor.sendall((value + '\n').encode())
        return response()

    response()
    deadline = time.monotonic() + 120
    while True:
        output = command(f'x /1wx 0x{frame_address:x}')
        match = re.search(r':\s+0x([0-9a-fA-F]+)', output)
        if match and int(match[1], 16) >= a.frame:
            break
        if time.monotonic() >= deadline:
            raise TimeoutError('Requested frame not reached')
        time.sleep(.05)
    command('stop')
    try:
        words = []
        for offset in range(0, word_count, 64):
            count = min(64, word_count - offset)
            output = command(f'x /{count}wx 0x{ring_address + offset * 4:x}')
            for line in output.splitlines():
                if re.match(r'^[0-9a-fA-F]+:', line):
                    words.extend(int(value, 16) for value in
                                 re.findall(r'0x([0-9a-fA-F]{1,8})',
                                            line.split(':', 1)[1]))
        if len(words) != word_count:
            raise RuntimeError(f'Expected {word_count} words, found {len(words)}')
    finally:
        command('cont')

fields = ('frame', 'frameMs', 'runtimeMs', 'sceneMs', 'worldMs',
          'modelsMs', 'effectsMs', 'textures', 'freePages',
          'setupMs', 'animationsMs', 'overlayMs', 'sabreMs',
          'playerMs', 'powerupsMs', 'spritesMs', 'enemiesMs',
          'backdropMs', 'physicsMs', 'levelOwnerMs',
          'spriteUpdateMs', 'spriteDrawMs', 'numSprite', 'numSCB',
          'slowSpriteMs', 'slowSpriteFunction', 'slowSpriteNum',
          'gameTicks', 'enemyDamageEvents', 'modelBatchFlushes',
          'gpuStateChanges', 'modelTriangles', 'gpuTriangleCalls',
          'bmdGeometryCacheHits', 'bmdGeometryCacheMisses',
          'bmdGeometryCalls', 'bmdLastGeneration', 'modelBatchUs',
          'modelTriangleUs')
records = [dict(zip(fields, words[i:i+field_count]))
           for i in range(0, len(words), field_count)]
records = sorted((row for row in records if row['frame'] > 0),
                 key=lambda row: row['frame'])
if a.out:
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(records, indent=2) + '\n')
for start, end in ((300, 350), (350, 400), (400, 450),
                   (450, 500), (500, 550), (550, 600), (600, 700)):
    section = [row for row in records if start <= row['frame'] < end]
    if section:
        elapsed = sum(row['frameMs'] for row in section)
        slow = sum(row['frameMs'] > 33 for row in section)
        print(f'{start}-{end}: {1000 * len(section) / elapsed:.2f} FPS, '
              f'{slow}/{len(section)} slow, '
              f'textures {section[0]["textures"]}->{section[-1]["textures"]}, '
              f'free pages {section[0]["freePages"]}->{section[-1]["freePages"]}')
