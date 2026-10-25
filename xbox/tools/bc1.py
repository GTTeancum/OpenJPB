"""Offline BC1 encoder for opaque, power-of-two Xbox textures.

With a transparent mask, blocks holding masked texels use BC1's three-colour
mode, whose fourth index is transparent black: the 1-bit alpha the Xbox model
and effect passes alpha-test, matching the PC shaders' discard of black."""
import numpy as np
from PIL import Image


def encode(image: Image.Image, transparent=None) -> bytes:
    """transparent: optional boolean array (height x width) of texels to
    encode as transparent black."""
    rgba = np.asarray(image.convert('RGBA'), dtype=np.uint8)
    height, width = rgba.shape[:2]
    if width < 4 or height < 4 or width & (width - 1) or height & (height - 1):
        raise ValueError('BC1 requires power-of-two dimensions of at least 4')
    if np.any(rgba[:, :, 3] != 255):
        raise ValueError('Opaque BC1 cannot preserve this image alpha')
    rgb = rgba[:, :, :3]
    blocks = rgb.reshape(height // 4, 4, width // 4, 4, 3)
    blocks = blocks.transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3).astype(np.int16)
    if transparent is None:
        masked = np.zeros(blocks.shape[:2], dtype=bool)
    else:
        masked = np.asarray(transparent, dtype=bool).reshape(height // 4, 4, width // 4, 4)
        masked = masked.transpose(0, 2, 1, 3).reshape(-1, 16)
    # Endpoints come from the visible texels only.
    big = np.where(masked[:, :, None], 32767, blocks)
    small = np.where(masked[:, :, None], -1, blocks)
    lo = np.where(masked.all(axis=1)[:, None], 0, big.min(axis=1))
    hi = np.where(masked.all(axis=1)[:, None], 0, small.max(axis=1))

    def pack565(color):
        return ((color[:, 0] * 31 + 127) // 255 << 11) | \
               ((color[:, 1] * 63 + 127) // 255 << 5) | \
               ((color[:, 2] * 31 + 127) // 255)

    endpoint0 = pack565(hi).astype(np.uint16)
    endpoint1 = pack565(lo).astype(np.uint16)
    three = masked.any(axis=1)
    # Four-colour blocks need c0 > c1; three-colour (transparent) blocks c0 <= c1.
    swap = np.where(three, endpoint0 > endpoint1, endpoint0 < endpoint1)
    a = endpoint0.copy()
    endpoint0[swap] = endpoint1[swap]
    endpoint1[swap] = a[swap]
    equal = (endpoint0 == endpoint1) & ~three
    endpoint0[equal] = np.minimum(endpoint0[equal].astype(np.int32) + 1, 65535)
    endpoint1[equal & (endpoint0 == 65535)] -= 1

    def unpack565(packed):
        return np.stack((((packed >> 11) & 31) * 255 // 31,
                         ((packed >> 5) & 63) * 255 // 63,
                         (packed & 31) * 255 // 31), axis=1).astype(np.int16)

    c0 = unpack565(endpoint0)
    c1 = unpack565(endpoint1)
    four = np.stack((c0, c1, (2 * c0 + c1) // 3, (c0 + 2 * c1) // 3), axis=1)
    # Three-colour palette: c0, c1, midpoint; index 3 is transparent and is
    # never chosen for a visible texel.
    tri = np.stack((c0, c1, (c0 + c1) // 2, np.full_like(c0, 30000)), axis=1)
    palette = np.where(three[:, None, None], tri, four)
    delta = blocks[:, :, None, :].astype(np.int32) - palette[:, None, :, :]
    distance = np.sum(delta * delta, axis=3, dtype=np.int64)
    index = np.argmin(distance, axis=2).astype(np.uint32)
    index[masked] = 3
    selector = np.sum(index << (2 * np.arange(16, dtype=np.uint32)),
                      axis=1, dtype=np.uint32)
    output = np.empty(len(blocks), dtype=[('c0', '<u2'), ('c1', '<u2'),
                                          ('selector', '<u4')])
    output['c0'] = endpoint0
    output['c1'] = endpoint1
    output['selector'] = selector
    return output.tobytes()
