"""Opt-in real-device FMV regressions in a hidden 1080p D3D11 window.

Audio is audible. All input is process-local; saves are redirected to the
output directory. Requires installed canonical assets and an audio device.
"""

import argparse
import json
import re
import subprocess
import shutil
from pathlib import Path


def run_case(exe, assets, output, movie, mode):
    name = f"{'boot' if movie == 0 else 'fed'}-{mode}"
    command = [str(exe), str(assets / "res/level/jpx/fed/fed.jpx"),
               "--cad", str(assets / "res/animation/obi_wan.cad"),
               "--bmd", str(assets / "res/MODEL/obi_wan.bmd"),
               "--hidden-window", "--control-harness", "--title",
               "--validate-title-movie", str(movie), "--movie-timing-probe",
               "--framebuffer-size", "1920", "1080",
               "--persistence-directory", str(output / "persistence"),
               "--frames", "680" if mode == "skip" else "6400"]
    if mode == "skip":
        command += ["--headless-phase", "none", "600",
                    "--headless-keyboard-phase", "escape", "1",
                    "--headless-phase", "none", "79"]
    else:
        command += ["--headless-phase", "none", "6400"]
    if mode == "stall":
        command += ["--movie-timing-stall-ms", "750"]
    log = output / f"{name}.log"
    print(f"RUN {name}", flush=True)
    try:
        with log.open("w") as target:
            process = subprocess.run(command, stdout=target, stderr=subprocess.STDOUT,
                                     timeout=190)
    finally:
        native_log = exe.with_suffix(".log")
        if native_log.exists():
            shutil.copyfile(native_log, output / f"{name}.native.log")
    if process.returncode:
        raise AssertionError(f"{name}: exit={process.returncode}; {log}")
    return check_log(log, movie, mode, command)


def check_log(log, movie, mode, command):
    name = f"{'boot' if movie == 0 else 'fed'}-{mode}"
    text = log.read_text(errors="replace")
    summaries = re.findall(r"movie_timing_summary=\(([^)]+)\)", text)
    if len(summaries) != 1:
        raise AssertionError(f"{name}: summaries={summaries}; {log}")
    stats = dict(part.split("=") for part in summaries[0].split(","))
    assert int(stats["clock_errors"]) == 0, stats
    assert float(stats["max_clock_skew_ms"]) < 100, stats
    assert int(stats["deadline_misses"]) == 0, stats
    assert int(stats["samples"]) >= (600 if mode == "skip" else 5000), stats
    assert int(stats["stall_ms"]) == (750 if mode == "stall" else 0), stats
    state = re.search(r"movie_state=\(([^\n]+)\)", text).group(1)
    assert "failures=0" in state and "audio_output=1" in state, state
    assert f"skips={int(mode == 'skip')}" in state, state
    final = re.findall(r"movie_timing=\(([^\n]+final=1)\)", text)[-1]
    final_stats = dict(part.split("=") for part in final.split(","))
    if mode != "skip":
        assert int(final_stats["decoded"]) == (2977 if movie == 0 else 2929), final_stats
        assert int(final_stats["video_ms"]) == (102400 if movie == 0 else 102899), final_stats
        assert int(final_stats["pending"]) == 0, final_stats
        assert abs(float(final_stats["audio_ms"]) -
                   (102741.333 if movie == 0 else 102912)) < 1, final_stats
    assert "presentation_timing=(frames=" in text, "Missing hardware presentation summary"
    assert "framebuffer=1920x1080" in text, "Wrong framebuffer dimensions"
    assert "menu_state=(active=1,mode=0,stack=0," in text, "Movie did not return to title"
    result = {"case": name, "stats": stats, "final": final_stats, "command": command}
    print(f"PASS {name}: {summaries[0]}", flush=True)
    return result


def run_handoff(exe, output, route):
    (output / route).mkdir(parents=True, exist_ok=True)
    build = exe.parent.parent
    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "-C", exe.parent.name,
         "--show-only=json-v1"], text=True))["tests"]
    test_name = ("jpb_pc_presentation_gameplay_handoff" if route == "new-game"
                 else "jpb_pc_presentation_continue_gameplay_handoff")
    command = next(test["command"] for test in tests if test["name"] == test_name)
    command = [part for part in command if part not in (
        "--silent-audio", "--validate-neutral-handoff", "--validate-persistence-handoff")]
    # The short handoff smokes' last confirm deliberately skips the FED
    # movie. These routes must instead allow the complete movie to finish.
    last_confirm = max(index for index, part in enumerate(command[:-1])
                       if part == "--headless-phase" and command[index + 1] == "select")
    del command[last_confirm:last_confirm + 3]
    command[command.index("--persistence-directory") + 1] = str(output / route / "persistence")
    command[command.index("--frames") + 1] = "6600"
    command += ["--movie-timing-probe", "--framebuffer-size", "1920", "1080"]
    if "--validate-audio-handoff" not in command:
        command.append("--validate-audio-handoff")
    log = output / f"{route}.log"
    print(f"RUN {route}", flush=True)
    with log.open("w") as target:
        process = subprocess.run(command, stdout=target, stderr=subprocess.STDOUT,
                                 timeout=210)
    native_log = exe.with_suffix(".log")
    if native_log.exists():
        shutil.copyfile(native_log, output / f"{route}.native.log")
    assert process.returncode == 0, f"{route}: exit={process.returncode}; {log}"
    return check_handoff(log, route, command)


def check_handoff(log, route, command):
    text = log.read_text(errors="replace")
    summaries = re.findall(r"movie_timing_summary=\(([^)]+)\)", text)
    assert len(summaries) == 2, summaries
    stats = dict(part.split("=") for part in summaries[-1].split(","))
    assert int(stats["samples"]) >= 5000, stats
    assert int(stats["clock_errors"]) == 0, stats
    assert float(stats["max_clock_skew_ms"]) < 100, stats
    assert int(stats["deadline_misses"]) == 0, stats
    final = re.findall(r"movie_timing=\(([^\n]+final=1)\)", text)[-1]
    final_stats = dict(part.split("=") for part in final.split(","))
    assert int(final_stats["decoded"]) == 2929, final_stats
    assert int(final_stats["video_ms"]) == 102899, final_stats
    assert abs(float(final_stats["audio_ms"]) - 102912) < 1, final_stats
    assert int(final_stats["pending"]) == 0, final_stats
    state = re.search(r"presentation=\(([^\n]+)\)", text).group(1)
    assert "handoffs=1,hidden=1,scripted=1" in state, state
    assert "validation failed" not in text, text[-4000:]
    native = log.with_suffix(".native.log").read_text(errors="replace")
    for movie in (0, 1):
        assert native.count(f"movie launch index={movie} started=1") == 1, native[-4000:]
    assert "rumble_policy=(gameplay_enabled=1,non_gameplay_suppressed=" in text
    audio = re.search(r"audio_handoff=\(([^\n]+)\)", text).group(1)
    assert "output=1" in audio and "movie_gate=1/1" in audio, audio
    assert "deferred=1/1" in audio, audio
    audio_stats = dict(part.split("=") for part in audio.split(","))
    assert int(audio_stats["generations"]) == 2, audio
    assert all(int(audio_stats[key]) > 0 for key in ("requests", "resolved", "started")), audio
    print(f"PASS {route}: {state}\n{audio}", flush=True)
    return {"case": route, "command": command, "movie": state, "audio": audio,
            "stats": stats, "final": final_stats}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", choices=[f"{m}-{s}" for m in ("boot", "fed")
                                          for s in ("natural", "stall", "skip")], action="append")
    parser.add_argument("--handoff", choices=["new-game", "continue"], action="append")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    failures = 0
    cases = args.case or ([] if args.handoff else [
        "boot-natural", "fed-natural", "boot-stall", "fed-stall", "boot-skip", "fed-skip"])
    for case in cases:
        movie, mode = case.split("-")
        try:
            results.append(run_case(args.exe.resolve(), args.assets.resolve(), args.output.resolve(),
                                    int(movie == "fed"), mode))
        except (AssertionError, subprocess.TimeoutExpired) as error:
            failures += 1
            results.append({"case": case, "error": str(error)})
            print(f"FAIL {case}: {error}", flush=True)
        (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    for route in args.handoff or []:
        try:
            results.append(run_handoff(args.exe.resolve(), args.output.resolve(), route))
        except (AssertionError, subprocess.TimeoutExpired) as error:
            failures += 1
            results.append({"case": route, "error": str(error)})
            print(f"FAIL {route}: {error}", flush=True)
        (args.output / "handoff-results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(failures != 0)


if __name__ == "__main__":
    raise SystemExit(main())
