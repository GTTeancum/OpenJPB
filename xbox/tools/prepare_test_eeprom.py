"""Create an isolated XEMU EEPROM with Xbox widescreen/HD mode flags.

The user checksum is the end-around sum of the 24 little-endian words at
0x60..0xbf. Only that checksum and the video flags at 0x94 are modified.
"""

import argparse
import struct
from pathlib import Path


def checksum(data: bytes) -> int:
    total = sum(struct.unpack_from("<24I", data, 0x60))
    while total > 0xFFFFFFFF:
        total = (total & 0xFFFFFFFF) + (total >> 32)
    return total


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--720p", action="store_true", dest="enable_720p")
    args = parser.parse_args()

    data = bytearray(args.source.read_bytes())
    if len(data) != 256 or checksum(data) != 0xFFFFFFFF:
        raise ValueError("Invalid source EEPROM or user checksum")
    original = bytes(data)
    flags = struct.unpack_from("<I", data, 0x94)[0]
    # Xbox EEPROM display bits: widescreen, 480p, and optionally 720p.
    flags |= 0x10000 | 0x80000
    if args.enable_720p:
        flags |= 0x20000
    struct.pack_into("<I", data, 0x94, flags)
    struct.pack_into("<I", data, 0x60, 0)
    struct.pack_into("<I", data, 0x60, checksum(data) ^ 0xFFFFFFFF)
    assert checksum(data) == 0xFFFFFFFF
    assert all(a == b or 0x60 <= i < 0x64 or 0x94 <= i < 0x98
               for i, (a, b) in enumerate(zip(original, data)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Test EEPROM video flags: 0x{flags:08X}")


if __name__ == "__main__":
    main()
