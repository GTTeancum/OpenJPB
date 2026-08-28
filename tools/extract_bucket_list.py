#!/usr/bin/env python3
"""Extract bucketList from the authoritative shipped executable.

The matched PDB fixes the array at RVA 0x4A6DB0 with 4,419 char pointers.
This tool converts those pointers to C string literals for the reconstructed
source. It deliberately validates the canonical first entry and final
sentinel so a mismatched executable cannot silently generate substitute data.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


IMAGE_BASE = 0x140000000
BUCKET_LIST_RVA = 0x4A6DB0
BUCKET_LIST_COUNT = 4419


def parse_sections(image: bytes) -> list[tuple[int, int, int]]:
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if image[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("input is not a PE image")

    section_count = struct.unpack_from("<H", image, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", image, pe_offset + 20)[0]
    section_offset = pe_offset + 24 + optional_size
    sections: list[tuple[int, int, int]] = []
    for index in range(section_count):
        offset = section_offset + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", image, offset + 8
        )
        sections.append(
            (virtual_address, max(virtual_size, raw_size), raw_offset)
        )
    return sections


def rva_to_offset(
    rva: int, sections: list[tuple[int, int, int]]
) -> int:
    for virtual_address, size, raw_offset in sections:
        if virtual_address <= rva < virtual_address + size:
            return raw_offset + rva - virtual_address
    raise ValueError(f"RVA 0x{rva:X} is outside the image sections")


def read_c_string(
    image: bytes, rva: int, sections: list[tuple[int, int, int]]
) -> str:
    offset = rva_to_offset(rva, sections)
    end = image.index(0, offset)
    return image[offset:end].decode("ascii")


def c_literal(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    image = args.executable.read_bytes()
    sections = parse_sections(image)
    list_offset = rva_to_offset(BUCKET_LIST_RVA, sections)
    entries: list[str] = []
    for index in range(BUCKET_LIST_COUNT):
        pointer = struct.unpack_from("<Q", image, list_offset + index * 8)[0]
        entries.append(read_c_string(image, pointer - IMAGE_BASE, sections))

    if entries[0] != "@bucket\\main.buk":
        raise ValueError(f"unexpected first bucket entry: {entries[0]!r}")
    if entries[-1] != "lastfile":
        raise ValueError(f"unexpected bucket sentinel: {entries[-1]!r}")
    if entries.count("lastfile") != 1:
        raise ValueError("bucket list must contain exactly one wrap sentinel")

    lines = [
        "/* Mechanically extracted from game.exe RVA 0x4A6DB0. */",
        "char *bucketList[JPB_BUCKET_LIST_COUNT] = {",
    ]
    lines.extend(f"    {c_literal(entry)}," for entry in entries)
    lines.append("};")
    lines.append("")
    args.output.write_text("\n".join(lines), encoding="ascii", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
