"""Read the Xbox gameplay step from the isolated XEMU guest near a frame."""
import argparse
from pathlib import Path
import re
import socket
import struct
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--frame", type=int, default=350)
p.add_argument("--span", type=int, default=100,
               help="Rendered frames over which to measure game ticks")
p.add_argument("--map", type=Path, default=Path("xbox/build/OpenJPB.map"))
a = p.parse_args()
mapping = a.map.read_text()


def symbol(name):
    match = re.search(r"(?m)^\s+000[0-9]:[0-9a-fA-F]+\s+_" + name +
                      r"\s+([0-9a-fA-F]+)", mapping)
    if not match:
        p.error(f"Missing symbol: {name}")
    return int(match[1], 16)


with socket.create_connection(("127.0.0.1", 9247), timeout=5) as monitor:
    monitor.settimeout(5)

    def response():
        data = b""
        while not data.endswith(b"(qemu) "):
            part = monitor.recv(4096)
            if not part:
                raise RuntimeError("XEMU monitor closed")
            data += part
        return data.decode(errors="replace")

    def word(name):
        monitor.sendall(f"x /1wx 0x{symbol(name):x}\n".encode())
        output = response()
        if "Cannot access memory" in output:
            return None
        match = re.search(r":\s+0x([0-9a-fA-F]+)", output)
        if not match:
            raise RuntimeError(f"No value for {name}: {output}")
        return int(match[1], 16)

    response()
    deadline = time.monotonic() + 120
    while (word("jpb_XboxSmokeFrame") or 0) < a.frame:
        if time.monotonic() >= deadline:
            raise TimeoutError("Requested frame not reached")
        time.sleep(.01)
    frame = word("jpb_XboxSmokeFrame")
    ticks = word("totalframes")
    step_fixed = word("gGlobalFrameRate")
    step_float = struct.unpack("<f", struct.pack("<I", word("fGlobalFrameRate")))[0]
    frame_ms = word("jpb_XboxFrameMs")
    started = time.monotonic()
    while (word("jpb_XboxSmokeFrame") or 0) < frame + a.span:
        if time.monotonic() >= deadline:
            raise TimeoutError("Measurement span not reached")
        time.sleep(.01)
    end_frame = word("jpb_XboxSmokeFrame")
    end_ticks = word("totalframes")
    elapsed = time.monotonic() - started
    print(f"frame={frame} frameMs={frame_ms} fixedStep={step_fixed} "
          f"floatStep={step_float:.3f} PC60Step=2048/0.5; "
          f"frames={end_frame-frame} gameTicks={end_ticks-ticks} "
          f"wallSeconds={elapsed:.3f} ticksPerSecond={(end_ticks-ticks)/elapsed:.1f}")
