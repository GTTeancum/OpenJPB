"""Verify a decoder audit against local Ogg granules and the shipped EXE clock.

WorkerThread 0x108366 sets TH_DECCTL_SET_GRANPOS before packet decode;
0x1083BD calls th_granule_time (0x85F0), then multiplies by 1000.
This independent packet reader retains that end-of-frame convention, not
FFmpeg's start-of-frame PTS convention. It does not rewrite media.
"""

import argparse
import csv
import struct
from pathlib import Path


def packets(path):
    partial = {}
    with path.open("rb") as source:
        while header := source.read(27):
            if len(header) != 27 or header[:5] != b"OggS\0":
                raise ValueError("Invalid Ogg page header")
            granule, serial = struct.unpack_from("<qI", header, 6)
            laces = source.read(header[26])
            last = max((i for i, size in enumerate(laces) if size < 255), default=-1)
            current = partial.setdefault(serial, bytearray())
            for index, size in enumerate(laces):
                data = source.read(size)
                if len(data) != size:
                    raise ValueError("Truncated Ogg packet")
                current.extend(data)
                if size < 255:
                    yield serial, bytes(current), granule if index == last else -1
                    current.clear()


def canonical_timestamps(path):
    video_serial = None
    current_frame = 0
    for serial, packet, granule in packets(path):
        if packet.startswith(b"\x80theora"):
            video_serial = serial
            numerator, denominator = struct.unpack_from(">II", packet, 22)
            shift = (int.from_bytes(packet[40:42], "big") >> 5) & 31
            bias = int(tuple(packet[7:10]) >= (3, 2, 1))
        elif serial == video_serial and (not packet or not packet[0] & 128):
            if granule >= 0:
                current_frame = (granule >> shift) - bias + (granule & ((1 << shift) - 1))
            current_frame += 1
            if packet:
                yield int((denominator / numerator) * current_frame * 1000)


def fingerprint(values):
    result = 2166136261
    for value in values:
        for byte in struct.pack("<I", value):
            result = ((result ^ byte) * 16777619) & 0xFFFFFFFF
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("movie", type=Path)
    parser.add_argument("audit_csv", type=Path)
    args = parser.parse_args()
    expected = list(canonical_timestamps(args.movie))
    with args.audit_csv.open(newline="") as source:
        actual = [int(row["playms"]) for row in csv.DictReader(source)]
    mismatches = [(i, a, b) for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
    print(f"frames={len(actual)}/{len(expected)} mismatches={len(mismatches)} "
          f"canonical_fnv32={fingerprint(expected):08x} last_ms={expected[-1]}")
    for index, actual_ms, expected_ms in mismatches[:20]:
        print(f"frame={index} actual_ms={actual_ms} canonical_ms={expected_ms}")
    return int(len(actual) != len(expected) or bool(mismatches))


if __name__ == "__main__":
    raise SystemExit(main())
