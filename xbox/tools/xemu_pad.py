"""Drive the OpenJPB Xbox build in XEMU through its test-only remote pad.

The guest's jpb_XboxRemotePad record is written through XEMU's GDB stub
(boot_xemu.ps1 -GdbPort). Guest counters are read through the XEMU monitor and
screenshots use XEMU's own screenshot facility. Never sends host input.

Steps (each argument is one step):
  press A[,B...] [frames]   hold the buttons for frames (default 4), then release
  hold A[,B...]             hold until the next release/press/hold
  release                   release every button and centre the stick
  stick X Y [frames]        left stick, -1..1 (Y negative = up); frames > 0 recentres
  trig L R [frames]         analog triggers 0..1
  wait N                    wait for N guest frames
  sleep S                   wall-clock seconds
  shot NAME                 native screenshot copied to --out/NAME.png
  read SYMBOL [COUNT]       print guest words at a map symbol
  until SYMBOL OP VALUE [TIMEOUT]   poll a guest word (OP: == != >= <=)
  pressuntil BTN SYMBOL OP VALUE [TIMEOUT]  press BTN until a guest word matches
  poke SYMBOL[+OFF] VALUE [BYTES]  write guest memory (test fixtures only)
  hmp COMMAND               raw XEMU monitor command (e.g. system_reset)
  p2 STEP                   apply a pad step to controller port 2
  off                       disable the remote pad (real controller resumes)
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import socket
import subprocess
import sys
import time

MAGIC = 0x5242504A
BUTTONS = dict(A=0, B=1, X=2, Y=3, BACK=4, START=6, LS=7, RS=8, WHITE=9,
               LB=9, BLACK=10, RB=10, UP=11, DOWN=12, LEFT=13, RIGHT=14)
REPO = Path(__file__).resolve().parents[2]


class Monitor:
    def __init__(self, port):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=5)
        self.sock.settimeout(5)
        self.response()

    def response(self):
        data = b''
        while not data.endswith(b'(qemu) '):
            part = self.sock.recv(65536)
            if not part:
                raise RuntimeError('XEMU monitor closed')
            data += part
        return data.decode(errors='replace')

    def words(self, address, count=1):
        self.sock.sendall(f'x /{count}wx 0x{address:x}\n'.encode())
        values = []
        for line in self.response().splitlines():
            if re.match(r'^\s*[\da-fA-F]+:', line):
                values += [int(v, 16) for v in re.findall(r'0x([\da-fA-F]+)', line.split(':', 1)[1])]
        # Before the XBE is mapped (BIOS boot) the read fails.
        return values[:count] if len(values) >= count else [None] * count


class Gdb:
    """Minimal GDB remote client: interrupt, write memory, continue."""

    def __init__(self, port):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=10)
        self.sock.settimeout(10)
        self.buffer = b''
        self.packet('?')
        self.running = False
        self.resume()

    def recv_packet(self):
        while True:
            start = self.buffer.find(b'$')
            if start >= 0:
                end = self.buffer.find(b'#', start)
                if end >= 0 and len(self.buffer) >= end + 3:
                    body = self.buffer[start + 1:end]
                    self.buffer = self.buffer[end + 3:]
                    self.sock.sendall(b'+')
                    return body.decode(errors='replace')
            part = self.sock.recv(65536)
            if not part:
                raise RuntimeError('XEMU GDB stub closed')
            self.buffer += part

    def send(self, body):
        checksum = sum(body.encode()) & 0xff
        self.sock.sendall(f'${body}#{checksum:02x}'.encode())

    def packet(self, body):
        self.send(body)
        return self.recv_packet()

    def stop(self):
        if self.running:
            self.sock.sendall(b'\x03')
            self.recv_packet()
            self.running = False

    def resume(self):
        if not self.running:
            self.send('c')
            self.running = True

    def write(self, address, data):
        self.stop()
        try:
            reply = self.packet(f'M{address:x},{len(data):x}:{data.hex()}')
            if reply != 'OK':
                raise RuntimeError(f'GDB write failed: {reply}')
        finally:
            self.resume()

    def close(self):
        try:
            self.stop()
            self.send('D')
            self.sock.close()
        except OSError:
            pass


class Session:
    def __init__(self, args):
        self.args = args
        self.symbols = {}
        for match in re.finditer(r'(?m)^\s+\d{4}:[\da-fA-F]+\s+_(\w+)\s+([\da-fA-F]{8,16})\b',
                                 args.map.read_text()):
            self.symbols.setdefault(match[1], int(match[2], 16))
        self.monitor = Monitor(args.port)
        self.gdb = Gdb(args.gdb_port)
        self.pad = self.symbol('jpb_XboxRemotePad')
        self.player = 0
        self.state = [dict(buttons=0, stick=(0, 0), triggers=(0, 0)) for _ in range(2)]
        config = (REPO / 'xbox/build/xemu/xemu.toml').read_text()
        self.capture_dir = Path(re.search(r"(?m)^screenshot_dir\s*=\s*['\"]([^'\"]+)", config)[1])

    def symbol(self, name):
        name, _, offset = name.partition('+')
        if offset:
            return self.symbol(name) + int(offset, 0)
        if re.fullmatch(r'0x[\da-fA-F]+', name):
            return int(name, 16)
        if name not in self.symbols:
            raise KeyError(f'{name} is not in {self.args.map}')
        return self.symbols[name]

    def frame(self):
        return self.monitor.words(self.symbol('jpb_XboxSmokeFrame'))[0]

    # Steps act on the current player's record (prefix a step with p2).
    buttons = property(lambda self: self.state[self.player]['buttons'],
                       lambda self, v: self.state[self.player].__setitem__('buttons', v))
    stick = property(lambda self: self.state[self.player]['stick'],
                     lambda self, v: self.state[self.player].__setitem__('stick', v))
    triggers = property(lambda self: self.state[self.player]['triggers'],
                        lambda self, v: self.state[self.player].__setitem__('triggers', v))

    def publish(self, magic=MAGIC):
        x, y = (int(max(-1, min(1, v)) * 32767) & 0xffff for v in self.stick)
        lt, rt = (int(max(0, min(1, v)) * 255) for v in self.triggers)
        record = [magic, self.buttons, x | (y << 16), lt | (rt << 16)]
        self.gdb.write(self.pad + 16 * self.player,
                       b''.join(v.to_bytes(4, 'little') for v in record))

    def wait(self, frames):
        start = self.frame()
        deadline = time.monotonic() + max(10, frames / 5)
        while start is None:
            time.sleep(0.25)
            start = self.frame()
        while ((self.frame() or start) - start) & 0xffffffff < frames:
            if time.monotonic() > deadline:
                raise TimeoutError(f'guest did not advance {frames} frames (stuck at {self.frame()})')
            time.sleep(0.02)

    def mask(self, names):
        value = 0
        for name in names.upper().split(','):
            value |= 1 << BUTTONS[name]
        return value

    def shot(self, name):
        for attempt in range(3):
            result = subprocess.run([sys.executable, str(self.args.helper), '--pid', str(self.args.pid),
                                     '--xemu-exe', str(self.args.exe), '--screenshot-dir',
                                     str(self.capture_dir), '--timeout', '8'],
                                    capture_output=True, text=True)
            if result.returncode == 0:
                capture = json.loads(result.stdout.strip())
                if capture.get('ok'):
                    self.args.out.mkdir(parents=True, exist_ok=True)
                    saved = self.args.out / f'{name}.png'
                    shutil.move(capture['path'], saved)
                    return saved
            time.sleep(1)
        raise RuntimeError(f'screenshot failed: {result.stdout} {result.stderr}')

    def step(self, text):
        self.player = 0
        if text.lower().startswith('p2 '):
            self.player = 1
            text = text[3:]
        words = text.split()
        op = words[0].lower()
        if op == 'press':
            self.buttons = self.mask(words[1])
            self.publish()
            self.wait(int(words[2]) if len(words) > 2 else 4)
            self.buttons = 0
            self.publish()
            self.wait(2)
        elif op == 'hold':
            self.buttons = self.mask(words[1])
            self.publish()
        elif op == 'release':
            self.buttons, self.stick, self.triggers = 0, (0, 0), (0, 0)
            self.publish()
        elif op == 'stick':
            self.stick = (float(words[1]), float(words[2]))
            self.publish()
            if len(words) > 3:
                self.wait(int(words[3]))
                self.stick = (0, 0)
                self.publish()
        elif op == 'trig':
            self.triggers = (float(words[1]), float(words[2]))
            self.publish()
            if len(words) > 3:
                self.wait(int(words[3]))
                self.triggers = (0, 0)
                self.publish()
        elif op == 'wait':
            self.wait(int(words[1]))
        elif op == 'sleep':
            time.sleep(float(words[1]))
        elif op == 'shot':
            return str(self.shot(words[1]))
        elif op == 'read':
            count = int(words[2]) if len(words) > 2 else 1
            return [f'0x{v:08x}' for v in self.monitor.words(self.symbol(words[1]), count)]
        elif op == 'until':
            address = self.symbol(words[1])
            target = int(words[3], 0)
            timeout = float(words[4]) if len(words) > 4 else 60
            test = {'==': lambda v: v == target, '!=': lambda v: v != target,
                    '>=': lambda v: v >= target, '<=': lambda v: v <= target}[words[2]]
            deadline = time.monotonic() + timeout
            while True:
                value = self.monitor.words(address)[0]
                if value is not None and test(value):
                    return f'0x{value:08x}'
                if time.monotonic() > deadline:
                    raise TimeoutError(f'{words[1]} = {value} after {timeout}s')
                time.sleep(0.1)
        elif op == 'pressuntil':
            # pressuntil BUTTON SYMBOL OP VALUE [TIMEOUT]: press every ~1.5 s
            address = self.symbol(words[2])
            target = int(words[4], 0)
            test = {'==': lambda v: v == target, '!=': lambda v: v != target,
                    '>=': lambda v: v >= target, '<=': lambda v: v <= target}[words[3]]
            deadline = time.monotonic() + (float(words[5]) if len(words) > 5 else 60)
            while True:
                value = self.monitor.words(address)[0]
                if value is not None and test(value):
                    return f'0x{value:08x}'
                if time.monotonic() > deadline:
                    raise TimeoutError(f'{words[2]} = {value}')
                self.buttons = self.mask(words[1])
                self.publish()
                time.sleep(0.15)
                self.buttons = 0
                self.publish()
                time.sleep(1.5)
        elif op == 'poke':
            # poke SYMBOL[+OFFSET] VALUE [BYTES]: test-only guest memory write
            address = self.symbol(words[1])
            size = int(words[3]) if len(words) > 3 else 4
            self.gdb.write(address, (int(words[2], 0) & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
        elif op == 'hmp':
            self.monitor.sock.sendall((text.split(None, 1)[1] + "\n").encode())
            return self.monitor.response().strip()
        elif op == 'off':
            self.buttons, self.stick, self.triggers = 0, (0, 0), (0, 0)
            self.publish(0)
        else:
            raise ValueError(f'unknown step: {text}')
        return None


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--pid', type=int, help='XEMU process (default: build/xemu/pid.txt)')
    p.add_argument('--port', type=int, default=9247)
    p.add_argument('--gdb-port', type=int, default=9248)
    p.add_argument('--map', type=Path, default=REPO / 'xbox/build/OpenJPB.map')
    p.add_argument('--out', type=Path, default=REPO / 'xbox/test-artifacts/pad')
    p.add_argument('--helper', type=Path,
                   default=Path('C:/Programming/GitHub/OpenJKDF2ogx/scripts/xbox/xemu_native_screenshot.py'))
    p.add_argument('--exe', type=Path, default=Path('C:/Games/Emulators/Xemu/xemu.exe'))
    p.add_argument('steps', nargs='+')
    args = p.parse_args()
    if args.pid is None:
        args.pid = int((REPO / 'xbox/build/xemu/pid.txt').read_text().strip())
    session = Session(args)
    try:
        for step in args.steps:
            result = session.step(step)
            print(json.dumps(dict(step=step, frame=session.frame(), result=result)), flush=True)
    finally:
        session.gdb.close()


if __name__ == '__main__':
    main()
