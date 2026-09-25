"""Compare the live a_glow upload with its staged pixels through XEMU monitor.

Read-only guest-memory diagnostic; does not inject input or inspect the desktop.
"""
from pathlib import Path
import re
import socket
import struct

from PIL import Image


symbols = Path("xbox/build/OpenJPB.map").read_text()


def address(name):
    match = re.search(r"\s+\d{4}:[\da-fA-F]+\s+_" + name + r"\s+([\da-fA-F]+)", symbols)
    if not match:
        raise RuntimeError(f"Missing map symbol: {name}")
    return int(match[1], 16)


with socket.create_connection(("127.0.0.1", 9247), timeout=2) as monitor:
    monitor.settimeout(10)

    def response():
        data = b""
        while not data.endswith(b"(qemu) "):
            data += monitor.recv(32768)
        return data.decode(errors="replace")

    response()

    def read(addr, words):
        monitor.sendall(f"x /{words}wx 0x{addr:x}\n".encode())
        result = []
        for line in response().splitlines():
            if re.match(r"^[\da-fA-F]+:", line):
                result.extend(int(x, 16) for x in re.findall(r"0x([\da-fA-F]{8})", line))
        if len(result) != words:
            raise RuntimeError(f"Guest memory read incomplete: {len(result)} / {words}")
        return result

    source = read(address("glow_source"), 1)[0]
    count = read(address("texture_count"), 1)[0]
    if not source or count > 512:
        raise RuntimeError(f"Glow not loaded or invalid texture count: {source:x}, {count}")
    entries = read(address("textures"), count * 8)
    matches = [entries[i * 8 : i * 8 + 8] for i in range(count) if entries[i * 8] == source]
    if len(matches) != 1:
        raise RuntimeError(f"Expected one glow cache entry, found {len(matches)}")
    src, gpu, width, height, fmt, pitch, linear, zero_rgb = matches[0]
    if (width, height) != (64, 64):
        raise RuntimeError(f"Unexpected glow size: {width}x{height}")

    def swizzle_offset(x, y, w, h):
        out, destination_bit, bit = 0, 1, 1
        while bit < w or bit < h:
            if bit < w:
                if x & bit:
                    out |= destination_bit
                destination_bit <<= 1
            if bit < h:
                if y & bit:
                    out |= destination_bit
                destination_bit <<= 1
            bit <<= 1
        return out

    cpu_words = read(source, width * height)
    gpu_words = read(gpu, pitch * height // 4)
    expected = Image.open("xbox/build/staged-disc/res/default/a_glow.tga").convert("RGBA")
    issues = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = expected.getpixel((x, y))
            pixel = (a << 24) | (r << 16) | (g << 8) | b
            linear_index = y * width + x
            gpu_index = y * (pitch // 4) + x if linear else swizzle_offset(x, y, width, height)
            if cpu_words[linear_index] != pixel or gpu_words[gpu_index] != pixel:
                issues.append((x, y, hex(pixel), hex(cpu_words[linear_index]), hex(gpu_words[gpu_index])))
    print({
        "source": hex(source), "gpu": hex(gpu), "format": hex(fmt),
        "pitch": pitch, "linear": linear, "zero_rgb": zero_rgb,
        "pixels": width * height, "mismatches": len(issues), "first_mismatches": issues[:12],
    })
