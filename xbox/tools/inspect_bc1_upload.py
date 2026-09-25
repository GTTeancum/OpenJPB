"""Match live XEMU BC1 uploads against staged XBT files (read-only monitor)."""
import argparse
from pathlib import Path
import re
import socket
import struct


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port', type=int, default=9247)
    p.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
    p.add_argument('--assets', type=Path, default=Path('xbox/build/staged-disc/res'))
    p.add_argument('--verify-full', action='store_true',
                   help='Read full live payloads and compare with matched XBT files')
    p.add_argument('--index', type=int, help='Inspect one texture-cache index')
    args = p.parse_args()
    mapping = args.map.read_text()

    def address(name):
        match = re.search(r'\s+\d{4}:[\da-fA-F]+\s+_' + name +
                          r'\s+([\da-fA-F]+)', mapping)
        if not match:
            raise RuntimeError(f'Missing symbol: {name}')
        return int(match[1], 16)

    expected = {}
    for path in args.assets.rglob('*.xbt'):
        data = path.read_bytes()
        if len(data) >= 48:
            _, width, height, size = struct.unpack_from('<4sIII', data)
            expected.setdefault((width, height, data[16:48]), []).append(str(path))

    with socket.create_connection(('127.0.0.1', args.port), timeout=5) as monitor:
        monitor.settimeout(10)

        def response():
            data = b''
            while not data.endswith(b'(qemu) '):
                part = monitor.recv(65536)
                if not part:
                    raise RuntimeError('Monitor disconnected')
                data += part
            return data.decode(errors='replace')

        response()

        def words(addr, count):
            monitor.sendall(f'x /{count}wx 0x{addr:x}\n'.encode())
            values = []
            for line in response().splitlines():
                if re.match(r'^\s*[\da-fA-F]+:', line):
                    values.extend(int(x, 16) for x in
                                  re.findall(r'0x([\da-fA-F]+)', line))
            if len(values) < count:
                raise RuntimeError(f'Guest read incomplete: {len(values)}/{count}')
            return values[:count]

        count = words(address('texture_count'), 1)[0]
        try:
            print({'probe_resets': words(address('jpb_XboxProbeResetCount'), 3)})
        except RuntimeError:
            pass
        entries = words(address('textures'), count * 10)
        for i in range(count):
            if args.index is not None and i != args.index:
                continue
            src, gpu, width, height, fmt, pitch, srcw, srch, linear, zero = \
                entries[10*i:10*i+10]
            if (fmt >> 8) & 0xff != 0x0c:
                continue
            prefix = struct.pack('<8I', *words(gpu, 8))
            matches = expected.get((width, height, prefix), [])
            result = {'index': i, 'source': hex(src), 'gpu': hex(gpu),
                   'size': [width, height], 'sourceSize': [srcw, srch],
                   'format': hex(fmt), 'pitch': pitch, 'linear': linear,
                   'matches': matches}
            if args.verify_full and matches:
                expected_data = Path(matches[0]).read_bytes()[16:]
                live = bytearray()
                for offset in range(0, len(expected_data), 256):
                    length = min(256, len(expected_data) - offset)
                    live.extend(struct.pack('<%dI' % (length // 4),
                                            *words(gpu + offset, length // 4)))
                result['fullMatch'] = live == expected_data
                if live != expected_data:
                    result['firstMismatch'] = next(j for j, (a, b) in
                                                   enumerate(zip(live, expected_data))
                                                   if a != b)
            print(result)


if __name__ == '__main__':
    main()
