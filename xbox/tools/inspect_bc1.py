"""Decode a staged Xbox XBT1/BC1 texture for offline visual inspection."""
import argparse
from pathlib import Path
import struct

import numpy as np
from PIL import Image


def rgb565(value):
    return ((value >> 11 & 31) * 255 // 31,
            (value >> 5 & 63) * 255 // 63,
            (value & 31) * 255 // 31)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('input', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--source', type=Path)
    a = p.parse_args()
    data = a.input.read_bytes()
    magic, width, height, size = struct.unpack_from('<4sIII', data)
    if magic != b'XBT1' or size != width * height // 2 or len(data) != 16 + size:
        p.error('Invalid XBT1 header or payload')
    image = np.empty((height, width, 3), dtype=np.uint8)
    offset = 16
    for by in range(0, height, 4):
        for bx in range(0, width, 4):
            c0, c1, bits = struct.unpack_from('<HHI', data, offset)
            offset += 8
            if c0 <= c1:
                p.error(f'Non-opaque BC1 block at ({bx},{by})')
            color0, color1 = rgb565(c0), rgb565(c1)
            palette = (color0, color1,
                       tuple((2*x+y)//3 for x, y in zip(color0, color1)),
                       tuple((x+2*y)//3 for x, y in zip(color0, color1)))
            for pixel in range(16):
                image[by+pixel//4, bx+pixel%4] = palette[bits >> (2*pixel) & 3]
    Image.fromarray(image, 'RGB').save(a.output)
    if a.source:
        source = np.asarray(Image.open(a.source).convert('RGB').resize(
            (width, height), Image.Resampling.LANCZOS), dtype=np.float32)
        delta = source - image.astype(np.float32)
        mse = float(np.mean(delta * delta))
        print({'size': [width, height], 'mse': mse,
               'psnr': float(10*np.log10(255*255/mse)) if mse else float('inf'),
               'decoded_min': image.min(axis=(0, 1)).tolist()})


if __name__ == '__main__':
    main()
