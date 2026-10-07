"""Transcode the canonical English Ogg/Theora movies for nxdk playback."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


MOVIES = [
    "1080/flipped/IntroFlippedVertical_converted.ogg",
    "1080/flipped/English1920Vertical_converted.ogg",
    "1080/flipped/HorizontalFlippedQui_converted.ogg",
    "1080/flipped/HorizontalFlippedObi_converted.ogg",
    "1080/flipped/HorizontalFlippedMace_converted.ogg",
    "1080/flipped/HorizontalFlippedPlo_converted.ogg",
    "1080/flipped/HorizontalFlippedAdi_converted.ogg",
    "1080/flipped/End1080Flipped_converted.ogg",
    "1080/flipped/Aspyr_Logo_1080_Flipped.ogg",
    "1080/flipped/photo_warning_English_1080_Flipped.ogg",
]
BOOT_MOVIES = [MOVIES[9], MOVIES[8], MOVIES[0]]


def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


parser = argparse.ArgumentParser()
parser.add_argument("--game-root", type=Path, required=True)
parser.add_argument(
    "--destination", type=Path, default=Path("xbox/build/staged-disc")
)
parser.add_argument(
    "--manifest", type=Path, default=Path("xbox/build/movie-manifest.json")
)
parser.add_argument("--set", choices=("boot", "all"), default="boot")
parser.add_argument("--width", type=int, default=512)
parser.add_argument("--height", type=int, default=288)
args = parser.parse_args()

if args.width < 160 or args.height < 90 or args.width % 16 or args.height % 16:
    raise SystemExit("Movie dimensions must be multiples of 16 and at least 160x90")

ffmpeg = shutil.which("ffmpeg")
ffprobe = shutil.which("ffprobe")
if not ffmpeg or not ffprobe:
    raise SystemExit("ffmpeg and ffprobe are required to stage Xbox movies")

source_root = (args.game_root / "res" / "movies").resolve()
destination_root = (args.destination / "res" / "movies").resolve()
selected = BOOT_MOVIES if args.set == "boot" else MOVIES
records: list[dict[str, object]] = []

for relative_name in selected:
    source = source_root / relative_name
    target = destination_root / relative_name
    pending = target.with_name(target.stem + ".pending.ogg")
    if not source.is_file():
        raise SystemExit(f"Missing canonical movie: {source}")
    target.parent.mkdir(parents=True, exist_ok=True)
    command = [
        ffmpeg,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-i",
        str(source),
        "-map",
        "0:v:0",
        "-map",
        "0:a:0",
        "-vf",
        f"scale={args.width}:{args.height}:flags=lanczos",
        "-c:v",
        "libtheora",
        "-q:v",
        "5",
        # FFmpeg's libtheora interframes at this size produce block-QPI decode
        # errors (and visible corruption in XEMU). Intra frames decode cleanly.
        "-g",
        "1",
        "-pix_fmt",
        "yuv420p",
        "-c:a",
        "libvorbis",
        "-q:a",
        "3",
        "-ar",
        "48000",
        "-ac",
        "2",
        # Keep audio interleaved within the bounded decoder queue.
        "-page_duration",
        "20000",
        str(pending),
    ]
    try:
        subprocess.run(command, check=True)
        # Probe only checks headers. Decode the complete video and audio before
        # replacing the last known good staged asset.
        for stream in ("0:v:0", "0:a:0"):
            subprocess.run(
                [ffmpeg, "-hide_banner", "-loglevel", "error", "-xerror",
                 "-i", str(pending), "-map", stream, "-f", "null", "-"],
                check=True,
            )
        pending.replace(target)
    finally:
        pending.unlink(missing_ok=True)
    probe = subprocess.run(
        [
            ffprobe,
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=codec_name,width,height,pix_fmt",
            "-of",
            "json",
            str(target),
        ],
        check=True,
        text=True,
        capture_output=True,
    )
    stream = json.loads(probe.stdout)["streams"][0]
    if (
        stream.get("codec_name") != "theora"
        or stream.get("width") != args.width
        or stream.get("height") != args.height
        or stream.get("pix_fmt") != "yuv420p"
    ):
        raise RuntimeError(f"Unexpected staged movie format: {target}: {stream}")
    records.append(
        {
            "path": relative_name,
            "sourceBytes": source.stat().st_size,
            "sourceSha256": file_hash(source),
            "stagedBytes": target.stat().st_size,
            "stagedSha256": file_hash(target),
            "width": args.width,
            "height": args.height,
            "video": "theora/yuv420p",
            "audio": "vorbis/48000Hz/stereo",
        }
    )

args.manifest.parent.mkdir(parents=True, exist_ok=True)
args.manifest.write_text(json.dumps(records, indent=2) + "\n", encoding="utf-8")
print(
    json.dumps(
        {
            "movies": len(records),
            "sourceBytes": sum(int(r["sourceBytes"]) for r in records),
            "stagedBytes": sum(int(r["stagedBytes"]) for r in records),
        }
    )
)
