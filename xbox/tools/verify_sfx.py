"""Match one staged Xbox SFX against native XEMU AC97 output.

The optional aligned music reference is removed by least-squares gain before
matching. This establishes waveform identity for a selected effect; it does
not validate the whole mix or listening quality.
"""
import argparse
import json
from pathlib import Path

import numpy as np
from scipy.signal import resample_poly

from verify_music import best_match, read_left


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('effect', type=Path)
    p.add_argument('--music', type=Path)
    p.add_argument('--music-offset', type=float, default=0)
    a = p.parse_args()

    capture, capture_rate = read_left(a.capture)
    effect, effect_rate = read_left(a.effect)
    if capture_rate != 48000 or effect_rate != 48000:
        p.error('Expected 48 kHz capture and staged Xbox SFX')
    music_gain = None
    if a.music:
        music, music_rate = read_left(a.music)
        music_48k = resample_poly(music, 48000, music_rate)
        offset = round(a.music_offset * 48000)
        if offset < 0 or offset >= len(capture):
            p.error('Music offset lies outside the capture')
        length = min(len(music_48k), len(capture) - offset)
        reference = music_48k[:length]
        music_gain = float(np.dot(capture[offset:offset + length], reference)
                           / np.dot(reference, reference))
        capture[offset:offset + length] -= music_gain * reference
    result = best_match(resample_poly(capture, 1, 12),
                        resample_poly(effect, 1, 12))
    print(json.dumps({'capture': str(a.capture), 'effect': str(a.effect),
                      'music_gain': music_gain, **result}, indent=2))


if __name__ == '__main__':
    main()
