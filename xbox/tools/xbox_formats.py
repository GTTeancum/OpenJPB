"""Xbox representations of game resource files, shared by stock staging
(stage_assets.py, shrink_cpu_textures.py, write_image_sizes.py) and mod
package conversion (stage_mods.py).

Stock files and mod files go through the same functions, so a change to how
the Xbox stores a resource changes both. When such a change alters what a
converted mod package contains, raise XBOX_FORMAT together with
JPB_XBOX_MOD_FORMAT in xbox/src/mods_xbox.h: the game then rejects packages
converted by an older tool and asks for them to be converted again.
"""
from dataclasses import dataclass, field
import hashlib
import math
from pathlib import Path, PurePosixPath
import shutil
import struct

import numpy as np
from PIL import Image
import soundfile as sf
from scipy.signal import resample_poly

from bc1 import encode as encode_bc1

XBOX_FORMAT = 1
IMAGE_SUFFIXES = ('.tga', '.png', '.bmp', '.jpg', '.jpeg')
# World and model textures use normalized UVs; only 2D screen art is cut in
# authored pixel coordinates and needs its authored size recorded.
SCREEN_ROOTS = ('front/', 'platform/', 'effects/', 'default/')


@dataclass
class Limits:
    max_texture: int = 128
    front_texture: int = 256
    gpu_world_texture: int = 256
    gpu_model_texture: int = 256
    ui_quality: dict = field(default_factory=dict)  # lower-case path -> cap
    gpu_levels: set | None = None  # None: every level
    # Mod characters often map a whole body onto one large atlas where stock
    # characters spread it over several textures; at the stock cap such an
    # atlas loses most of its detail. When set, model textures above 512 px
    # keep half their source size (2048 -> 1024, 1024 -> 512).
    model_atlas_detail: bool = False


def gpu_model_limit(source_size, limits):
    """BC1 size cap for a model texture whose larger side is source_size."""
    if limits.model_atlas_detail and source_size > 512:
        return max(limits.gpu_model_texture, min(1024, source_size // 2))
    return limits.gpu_model_texture


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def convert(source, target, rel, limits):
    """Write the Xbox form of one resource; rel is relative to res/.

    Returns the staging record. A BC1 copy (<name>.xbt) is written beside
    opaque model and world textures for the GPU renderer."""
    source, target, rel = Path(source), Path(target), PurePosixPath(rel)
    target.parent.mkdir(parents=True, exist_ok=True)
    record = {'path': rel.as_posix(), 'sourceBytes': source.stat().st_size}
    original_hash = sha256(source)
    record['sourceSha256'] = original_hash
    suffix = source.suffix.lower()
    parts = [part.lower() for part in rel.parts]
    if suffix in IMAGE_SUFFIXES:
        with Image.open(source) as im:
            record['originalSize'] = list(im.size)
            gpu_image = im.copy()
            default_limit = (limits.front_texture if parts[0] == 'front'
                             else limits.max_texture)
            limit = limits.ui_quality.get(rel.as_posix().lower(), default_limit)
            im.thumbnail((limit, limit), Image.Resampling.LANCZOS)
            record['stagedSize'] = list(im.size)
            # Uncompressed TGA remains compatible with the native reader.
            if suffix == '.tga':
                im.convert('RGBA' if 'A' in im.getbands() else 'RGB').save(target, compression=None)
            else:
                im.save(target)
        gpu_limit = None
        if suffix == '.tga' and parts[0] == 'model':
            gpu_limit = gpu_model_limit(max(record['originalSize']), limits)
        elif (len(parts) >= 4 and suffix == '.tga' and parts[:2] == ['level', 'jpx'] and
              (limits.gpu_levels is None or parts[2] in limits.gpu_levels)):
            gpu_limit = limits.gpu_world_texture
        if gpu_limit is not None:
            gpu_target = target.with_suffix('.xbt')
            if gpu_target.exists():
                gpu_target.unlink()
            gpu_image.thumbnail((gpu_limit, gpu_limit), Image.Resampling.LANCZOS)
            try:
                compressed = encode_bc1(gpu_image)
            except ValueError:
                pass  # Alpha and irregular dimensions keep the existing RGBA path.
            else:
                width, height = gpu_image.size
                gpu_target.write_bytes(struct.pack('<4sIII', b'XBT1', width, height,
                                                   len(compressed)) + compressed)
                record['gpuTexture'] = rel.with_suffix('.xbt').as_posix()
                record['gpuSize'] = [width, height]
                record['gpuBytes'] = gpu_target.stat().st_size
                record['gpuSha256'] = sha256(gpu_target)
    elif suffix == '.wav' and parts[:2] == ['sound', 'sfx']:
        # SDL's on-device conversion uses a worst-case temporary allocation;
        # prepare SFX in mixer format in the private Xbox asset copy instead.
        info = sf.info(str(source))
        if info.subtype == 'PCM_16' and info.samplerate == 48000 and info.channels == 2:
            shutil.copy2(source, target)
        else:
            audio, rate = sf.read(str(source), dtype='float32', always_2d=True)
            if rate != 48000:
                divisor = math.gcd(rate, 48000)
                audio = resample_poly(audio, 48000 // divisor, rate // divisor, axis=0)
            if audio.shape[1] == 1:
                audio = np.repeat(audio, 2, axis=1)
            elif audio.shape[1] != 2:
                raise RuntimeError(f'Unsupported SFX channels: {source}')
            sf.write(str(target), audio, 48000, format='WAV', subtype='PCM_16')
            record['audioConversion'] = (
                f'{info.subtype} {info.samplerate}Hz {info.channels}ch '
                'to PCM_16 48000Hz 2ch')
    elif suffix == '.wav' and sf.info(str(source)).subtype != 'PCM_16':
        info = sf.info(str(source))
        audio, rate = sf.read(str(source), dtype='float32')
        sf.write(str(target), audio, rate, format='WAV', subtype='PCM_16')
        record['audioConversion'] = f'{info.subtype} to PCM_16'
    else:
        shutil.copy2(source, target)
    record['stagedBytes'] = target.stat().st_size
    record['stagedSha256'] = sha256(target)
    if sha256(source) != original_hash:
        raise RuntimeError(f'Source changed during staging: {source}')
    return record


def shrink_cpu_copy(image_path, size):
    """Reduce the CPU copy of a texture the GPU draws from its BC1 file.

    Texture coordinates are normalized, so a smaller copy does not move
    them. Returns bytes saved, or None when the image is already small."""
    image_path = Path(image_path)
    with Image.open(image_path) as im:
        if max(im.size) <= size:
            return None
        before = image_path.stat().st_size
        mode = 'RGBA' if 'A' in im.getbands() else 'RGB'
        small = im.convert(mode)
        small.thumbnail((size, size), Image.Resampling.LANCZOS)
    if image_path.suffix.lower() == '.tga':
        small.save(image_path, compression=None)
    else:
        small.save(image_path)
    return before - image_path.stat().st_size, list(small.size)


def cpu_copy_candidates(directory):
    """Images in directory that have a BC1 sibling."""
    directory = Path(directory)
    stems = {x.stem.lower() for x in directory.glob('*.xbt')}
    return [p for p in sorted(directory.iterdir())
            if p.suffix.lower() in ('.tga', '.png', '.bmp') and p.stem.lower() in stems]


def screen_size_line(record):
    """'<path> <width> <height>' for a reduced screen image, else None."""
    path = record['path'].replace('\\', '/')
    original, staged = record.get('originalSize'), record.get('stagedSize')
    if not original or not staged or original == staged:
        return None
    if not path.lower().startswith(SCREEN_ROOTS):
        return None
    return f'{path.lower()} {original[0]} {original[1]}'
