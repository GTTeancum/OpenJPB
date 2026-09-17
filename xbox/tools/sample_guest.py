"""Sample only the isolated emulator's guest CPU through its monitor."""
import argparse
import bisect
import collections
from pathlib import Path
import re
import socket
import time

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=9247)
parser.add_argument('--count', type=int, default=30)
parser.add_argument('--start-frame', type=int)
parser.add_argument('--inspect', help='Print registers and stack at the first matching symbol')
parser.add_argument('--stack-words',type=int,default=24)
parser.add_argument('--map', type=Path, default=Path('xbox/build/OpenJPB.map'))
args = parser.parse_args()
symbols = sorted((int(m[2],16),m[1]) for line in args.map.read_text().splitlines()
    if (m:=re.match(r'\s+0001:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8,16})\s',line)))
addresses = [x[0] for x in symbols]
counts = collections.Counter()
with socket.create_connection(('127.0.0.1',args.port)) as monitor:
    monitor.settimeout(.15)
    def command(text):
        monitor.sendall((text+'\n').encode())
        output=b''
        while True:
            try: output+=monitor.recv(16384)
            except socket.timeout: break
            if output.endswith(b'(qemu) '): break
        return output.decode(errors='replace')
    command('')
    if args.start_frame is not None:
        frame_match = re.search(
            r'(?m)^\s+0000:[0-9a-fA-F]+\s+_jpb_XboxSmokeFrame\s+([0-9a-fA-F]+)',
            args.map.read_text())
        if frame_match is None:
            parser.error('The build map has no smoke frame marker')
        frame_symbol = int(frame_match[1],16)
        deadline=time.monotonic()+120
        while time.monotonic()<deadline:
            value=command(f'x /1wx 0x{frame_symbol:x}')
            match=re.search(r'0x([0-9a-fA-F]{1,8})',value.split(':')[-1])
            if match and int(match[1],16)>=args.start_frame:
                break
            time.sleep(.05)
        else:
            raise TimeoutError('Start frame not reached')
    try:
        for _ in range(args.count):
            command('stop')
            registers=command('info registers')
            if match:=re.search(r'EIP=([0-9a-fA-F]+)',registers):
                address=int(match[1],16)
                index=bisect.bisect_right(addresses,address)-1
                label=symbols[index][1] if index>=0 and address<0x300000 else hex(address)
                counts[label]+=1
                if args.inspect and args.inspect in label:
                    print('\n'.join(line for line in registers.splitlines() if line and '\x1b' not in line))
                    stack=re.search(r'ESP=([0-9a-fA-F]+)',registers)
                    if stack:
                        data=command(f'x /{args.stack_words}wx 0x'+stack[1])
                        for line in data.splitlines():
                            if re.match(r'^[0-9a-fA-F]{8}:',line):
                                print(line)
                                for value in re.findall(r'0x([0-9a-fA-F]{8})',line):
                                    address=int(value,16)
                                    index=bisect.bisect_right(addresses,address)-1
                                    if index>=0 and address<0x300000:
                                        print(' ',value,symbols[index][1],'+',hex(address-addresses[index]))
                    break
            command('cont')
            time.sleep(.037)
    finally:
        command('cont')
for name,count in counts.most_common(): print(count,name)
