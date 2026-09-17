"""Read Xbox game input/camera globals from the isolated XEMU monitor."""
import argparse
from pathlib import Path
import re
import socket
import struct


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
    p.add_argument('--port', type=int, default=9247)
    a = p.parse_args()
    mapping = a.map.read_text()
    names = ('jpb_XboxSmokeFrame', 'player1InputType', 'mCameraAngleDest',
             'g_p1X', 'g_p1Y', 'jpb_XboxPlayerTelemetry')
    addresses = {}
    for name in names:
        match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_' + name +
                          r'\s+([\da-fA-F]+)', mapping)
        if not match:
            p.error(f'Missing map symbol {name}')
        addresses[name] = int(match[1], 16)
    with socket.create_connection(('127.0.0.1', a.port), timeout=5) as monitor:
        monitor.settimeout(5)

        def response():
            result = b''
            while not result.endswith(b'(qemu) '):
                part = monitor.recv(16384)
                if not part:
                    raise RuntimeError('Monitor closed')
                result += part
            return result.decode(errors='replace')

        response()
        for name in names:
            address = addresses[name]
            count = 6 if name == 'jpb_XboxPlayerTelemetry' else 1
            monitor.sendall(f'x /{count}wx 0x{address:x}\n'.encode())
            words = []
            for line in response().splitlines():
                if re.match(r'^\s*[\da-fA-F]+:', line):
                    words.extend(int(word, 16) for word in
                                 re.findall(r'0x([\da-fA-F]+)', line))
            if name in ('g_p1X', 'g_p1Y'):
                value = struct.unpack('<f', struct.pack('<I', words[0]))[0]
            else:
                value = [word if word < 0x80000000 else word - 0x100000000
                         for word in words[:count]]
            print(name, value)


if __name__ == '__main__':
    main()
