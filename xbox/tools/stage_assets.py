"""Copy a private Xbox asset instance; downscale textures without changing originals."""
from pathlib import Path
import argparse
import hashlib
import json
import math
import shutil
from PIL import Image
import numpy as np
import soundfile as sf
from scipy.signal import resample_poly
from bc1 import encode as encode_bc1
import struct

p = argparse.ArgumentParser()
p.add_argument('--game-root', type=Path, required=True)
p.add_argument('--destination', type=Path, required=True)
p.add_argument('--manifest', type=Path,
               default=Path('xbox/build/asset-manifest.json'),
               help='Repository-local staging manifest path')
p.add_argument('--max-texture', type=int, default=128)
p.add_argument('--ui-selection', type=Path,
               default=Path('xbox/ui-texture-quality.json'),
               help='Source-capped UI texture resolutions')
p.add_argument('--gpu-world-texture', type=int, default=256,
               help='BC1 resolution cap for opaque JPX level textures')
p.add_argument('--gpu-model-texture', type=int, default=256,
               help='BC1 resolution cap for opaque MODEL textures')
p.add_argument('--gpu-level', action='append',
               help='Stage BC1 for a selected level; repeat for more levels')
a = p.parse_args()
source = (a.game_root / 'res').resolve()
destination = a.destination.resolve()
manifest_path = a.manifest.resolve()
if destination == source or source in destination.parents or destination in source.parents:
    raise SystemExit('Destination must be separate from original resources')
if a.max_texture < 16 or a.max_texture & (a.max_texture - 1):
    raise SystemExit('Texture limit must be a power of two, at least 16')
if a.gpu_world_texture < 16 or a.gpu_world_texture & (a.gpu_world_texture - 1):
    raise SystemExit('GPU texture limit must be a power of two, at least 16')
if a.gpu_model_texture < 16 or a.gpu_model_texture & (a.gpu_model_texture - 1):
    raise SystemExit('GPU model texture limit must be a power of two, at least 16')
gpu_levels = {name.lower() for name in a.gpu_level} if a.gpu_level else None
ui_quality = {path.lower(): limit for path, limit in
              json.loads(a.ui_selection.read_text()).items()}
if any(not isinstance(limit, int) or limit < 16 or limit & (limit - 1)
       for limit in ui_quality.values()):
    raise SystemExit('UI texture limits must be powers of two, at least 16')
records = []
for f in sorted(source.rglob('*')):
    if not f.is_file():
        continue
    rel = f.relative_to(source)
    if rel.parts[0].lower() == 'movies':
        continue  # In-level milestone; media playback is not yet implemented.
    target = destination / 'res' / rel
    target.parent.mkdir(parents=True, exist_ok=True)
    record = {'path': rel.as_posix(), 'sourceBytes': f.stat().st_size}
    original_hash = hashlib.sha256(f.read_bytes()).hexdigest()
    record['sourceSha256'] = original_hash
    if f.suffix.lower() in ('.tga', '.png', '.bmp', '.jpg', '.jpeg'):
        with Image.open(f) as im:
            record['originalSize'] = list(im.size)
            gpu_image = im.copy()
            limit = ui_quality.get(rel.as_posix().lower(), a.max_texture)
            im.thumbnail((limit, limit), Image.Resampling.LANCZOS)
            record['stagedSize'] = list(im.size)
            # Uncompressed TGA remains compatible with the native reader.
            if f.suffix.lower() == '.tga':
                im.convert('RGBA' if 'A' in im.getbands() else 'RGB').save(target, compression=None)
            else:
                im.save(target)
        gpu_limit = None
        if f.suffix.lower() == '.tga' and rel.parts[0].lower() == 'model':
            gpu_limit = a.gpu_model_texture
        elif (len(rel.parts) >= 4 and f.suffix.lower() == '.tga' and
              tuple(part.lower() for part in rel.parts[:2]) == ('level', 'jpx') and
              (gpu_levels is None or rel.parts[2].lower() in gpu_levels)):
            gpu_limit = a.gpu_world_texture
        if gpu_limit is not None:
            gpu_target = target.with_suffix('.xbt')
            if gpu_target.exists():
                gpu_target.unlink()
            gpu_image.thumbnail((gpu_limit, gpu_limit),
                                Image.Resampling.LANCZOS)
            try:
                compressed = encode_bc1(gpu_image)
            except ValueError:
                pass  # Alpha and irregular dimensions keep the existing RGBA path.
            else:
                width, height = gpu_image.size
                gpu_target.write_bytes(struct.pack('<4sIII', b'XBT1', width, height,
                                                   len(compressed)) + compressed)
                record['gpuTexture'] = gpu_target.relative_to(destination / 'res').as_posix()
                record['gpuSize'] = [width, height]
                record['gpuBytes'] = gpu_target.stat().st_size
                record['gpuSha256'] = hashlib.sha256(gpu_target.read_bytes()).hexdigest()
    elif f.suffix.lower() == '.wav' and len(rel.parts) >= 2 and tuple(
            part.lower() for part in rel.parts[:2]) == ('sound', 'sfx'):
        # SDL's on-device conversion uses a worst-case temporary allocation;
        # prepare SFX in mixer format in the private Xbox asset copy instead.
        info = sf.info(str(f))
        if info.subtype == 'PCM_16' and info.samplerate == 48000 and info.channels == 2:
            shutil.copy2(f, target)
        else:
            audio, rate = sf.read(str(f), dtype='float32', always_2d=True)
            if rate != 48000:
                divisor = math.gcd(rate, 48000)
                audio = resample_poly(audio, 48000 // divisor, rate // divisor, axis=0)
            if audio.shape[1] == 1:
                audio = np.repeat(audio, 2, axis=1)
            elif audio.shape[1] != 2:
                raise RuntimeError(f'Unsupported SFX channels: {f}')
            sf.write(str(target), audio, 48000, format='WAV', subtype='PCM_16')
            record['audioConversion'] = (
                f'{info.subtype} {info.samplerate}Hz {info.channels}ch '
                'to PCM_16 48000Hz 2ch')
    elif f.suffix.lower() == '.wav' and sf.info(str(f)).subtype != 'PCM_16':
        info = sf.info(str(f))
        audio, rate = sf.read(str(f), dtype='float32')
        sf.write(str(target), audio, rate, format='WAV', subtype='PCM_16')
        record['audioConversion'] = f'{info.subtype} to PCM_16'
    else:
        shutil.copy2(f, target)
    record['stagedBytes'] = target.stat().st_size
    record['stagedSha256'] = hashlib.sha256(target.read_bytes()).hexdigest()
    if hashlib.sha256(f.read_bytes()).hexdigest() != original_hash:
        raise RuntimeError(f'Source changed during staging: {f}')
    records.append(record)
manifest_path.parent.mkdir(parents=True, exist_ok=True)
manifest_path.write_text(json.dumps(records, indent=2))
print(json.dumps({'files': len(records), 'sourceBytes': sum(r['sourceBytes'] for r in records),
                  'stagedBytes': sum(r['stagedBytes'] for r in records)}))
