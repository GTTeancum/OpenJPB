"""Correlate native XEMU AC97 output with the canonical staged WAV track."""
import argparse
import json
from pathlib import Path
import wave

import numpy as np
from scipy.signal import fftconvolve, resample_poly


def read_left(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getnchannels() != 2 or wav.getsampwidth() != 2:
            raise ValueError(f"Unsupported PCM format: {path}")
        rate = wav.getframerate()
        pcm = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2").reshape(-1, 2)
        return pcm[:, 0].astype(np.float32) / 32768, rate


def best_match(recording, source, sample_rate=4000):
    source = source - np.mean(source)
    n = len(source)
    if n > len(recording):
        raise ValueError("Reference segment exceeds captured audio")
    energy = float(np.sum(source * source))
    if energy == 0:
        raise ValueError("Silent reference segment")
    numerator = fftconvolve(recording, source[::-1], mode="valid")
    sums = np.cumsum(np.pad(recording, (1, 0)), dtype=np.float64)
    squares = np.cumsum(np.pad(recording * recording, (1, 0)), dtype=np.float64)
    window_sum = sums[n:] - sums[:-n]
    window_energy = squares[n:] - squares[:-n] - window_sum**2 / n
    correlation = np.zeros_like(numerator)
    audible = window_energy > 1e-4
    correlation[audible] = numerator[audible] / np.sqrt(window_energy[audible] * energy)
    at = int(np.argmax(np.abs(correlation)))
    return {"capture_second": at / sample_rate, "correlation": float(correlation[at])}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_wav", type=Path)
    parser.add_argument("source_wav", type=Path)
    args = parser.parse_args()
    native, native_rate = read_left(args.native_wav)
    source, source_rate = read_left(args.source_wav)
    if (native_rate, source_rate) != (48000, 22050):
        raise ValueError(f"Expected 48 kHz capture and 22.05 kHz source: {native_rate}, {source_rate}")
    capture = resample_poly(native, 1, 12)
    reference = resample_poly(source, 80, 441)
    segments = []
    for source_second, duration in ((0, 6), (5, 6), (10, 2)):
        first = source_second * 4000
        last = first + duration * 4000
        if last <= len(reference) and duration * 4000 <= len(capture):
            match = best_match(capture, reference[first:last])
            segments.append({"source_second": source_second,
                             "duration_seconds": duration,
                             "timeline_offset_seconds": match["capture_second"] - source_second,
                             **match})
    print(json.dumps({"capture": str(args.native_wav), "source": str(args.source_wav),
                      "segments": segments}, indent=2))


if __name__ == "__main__":
    main()
