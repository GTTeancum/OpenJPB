# FMV Granule Timing Audit

## Root Cause

The reconstructed Theora worker synthesized video timestamps by counting decoded
pictures at 30 Hz. The shipped files contain sparse sections with authored gaps
between pictures. Removing those gaps made video run ahead of unchanged audio.
The previous decode and wall-clock checks could not detect this: they checked
the incorrect timestamps produced by the same decoder.

In the boot movie, picture 4 was labeled 133 ms although its file PTS is 1233 ms.
The offset reaches 2800 ms near the end. In the FED intro, the corresponding
offset grows to 5266 ms. These are decoder errors, not subjective audio offsets.

## Canonical Evidence

Local `game.exe` SHA256:
`88932504124D58315030C4CE8D03B5B6AC02AB57E90BB84A8BC7D1F0CE1AE05F`.

- PDB-named `WorkerThread`, RVA `0x107AC0`, size 3070.
- `0x108366..0x108388`: if packet granule position is nonnegative, call
  `th_decode_ctl` with request 5 (`TH_DECCTL_SET_GRANPOS`) and size 8.
- `0x10838D..0x1083A0`: decode the packet and accept result zero.
- `0x1083BD..0x1083C5`: call `th_granule_time` with the returned granule.
- `0x1083F0..0x1083F9`: multiply seconds by 1000, truncate, store `playms`.
- `th_granule_time`, RVA `0x85F0`, and decode control, RVA `0xED10`, were
  disassembled directly to verify the frame-bias and end-of-frame convention.
- `PlayVideo`, RVA `0x123780`, preserves the 2000 ms prebuffer, starts audio
  with its wall clock, and tests frame deadlines against that clock.
- `audio_callback`, RVA `0x126FF0`, consumes audio samples sequentially.

The fix restores the missing granule control and time conversion. It adds no
guessed offset, resampling, texture changes, or replacement playback clock.

## Decoder Throughput

Real-device testing also exposed decoder starvation in a sustained FED run:
its queued audio drained around 68 seconds, and the audio device eventually
trailed the wall clock by 6303 ms. This was a failed run, not sync proof. Another
attempt had a 109 ms missed presentation deadline. Both logs were retained.

Direct disassembly of `ConvertVideoFrame420ToRGB` (RVA `0x1062C0`, 1867 bytes)
and the identical RGBA implementation (`0x106A10`) showed sixteen-pixel SIMD
blocks missing from the reconstruction's scalar pixel loop. Restoring packed
conversion preserves the original operation order, clamps, MXCSR rounding,
RGBA packing, vertical inversion, and scalar tail. No colors are tuned.

The executable's float coefficients were checked by their bit patterns:

| Constant | Bits |
| --- | --- |
| Luma scale | `3b95a025` |
| Chroma scale | `3b924925` |
| Red coefficient | `43b2c147` |
| Green Cb coefficient | `42af826e` |
| Green Cr coefficient | `43361ad0` |
| Blue coefficient | `43e1ee14` |

`jpb_theoraplay_rgb_exhaustive` checks all 16,777,216 Y/Cb/Cr combinations
against the EXE-derived scalar math, including alpha, row inversion, and a
six-pixel tail. It passes byte-for-byte. Complete media-timeline tests still
pass after this throughput correction.

## Complete Media Verification

`tools/audit_movie_timeline.py` independently reads local Ogg packet granules
and applies the EXE-derived clock. Its expected values are independent of the
production decoder. The source convention is end-of-frame time, unlike FFmpeg's
start-of-frame PTS; sparse frames must not be compared using a constant offset.

| Movie | Frames checked | Timestamp mismatches | Timeline FNV32 | Final timestamp | Audio sample frames |
| --- | ---: | ---: | --- | ---: | ---: |
| Boot | 2977 | 0 | `7cd7fa94` | 102400 ms | 4931584 |
| FED intro | 2929 | 0 | `4668f027` | 102899 ms | 4939776 |

All 5906 decoded picture checksums match FFmpeg decoding of the same local
assets, in sequence. FFmpeg is an independent decoder check, not implementation
evidence for canonical scheduling. Both audio streams retain identical sample
counts, and their decoded float PCM is byte-identical before/after the fix.
Independent FFmpeg PCM differs by less than `8.4e-7` per float sample.

Asset SHA256:

- Boot `IntroFlippedVertical_converted.ogg`:
  `D9595D540711AE92546B1FD5A9FAD287CC830CFADF20B77EAD72845C2C164031`.
- FED `English1920Vertical_converted.ogg`:
  `3667849C9B4539137B933F48B5EF0D7578730542EF9D892085EB390339F94EE2`.

## Permanent Regressions

- `jpb_theoraplay_boot_timeline` and `jpb_theoraplay_fed_timeline` decode complete
  files and check every timestamp through a fingerprint plus complete frame and
  audio counts. Both pass, as does the existing Theora test.
- Existing boot/FED decode and `jpb_pc_audio_tests` also pass.
- `tools/smoke_movie_timing.py` runs six opt-in hardware/audio-device cases:
  natural completion, a 750 ms presentation stall, and keyboard skip, separately
  for boot and FED. It uses hidden D3D11 windows, 1920x1080 rendering, process-local
  input, and build-local persistence. It requires real audio output.
- `--movie-timing-probe` samples the audio device and next video deadline after
  actual presentation, emits one-second reports, and records startup/maximum
  clock difference and missed deadlines. Diagnostic tolerances are test limits,
  not modifications to canonical timing.
- `--movie-timing-stall-ms` is gated by the probe, which requires the explicit
  control harness. Normal launches do not run these diagnostics or input.
- The movie skip path now excludes physical XInput during an opted-in harness
  run. A physical-controller edge interrupted the first hardware attempt; that
  attempt is rejected, not counted as natural playback proof.
- Opted-in harness rumble is also process-local: canonical non-gameplay
  suppression still runs and is checked. Harness runs do not initialize physical
  XInput, so neither ordinary rumble nor shutdown motor resets reach hardware.
- New Game and Continue cases traverse the actual front end, allow the entire
  FED movie to finish, and check real audio output, deferred music release,
  load-screen presentation, and gameplay handoff. They redirect persistence
  and do not overwrite installed saves. Their source short-smoke skip commands
  are removed explicitly; short or accidentally skipped movies do not pass.
- `jpb_pc_audio_movie_sfx_gate` submits a real resident SFX through SDL_mixer,
  checks that movie playback suppresses it, then checks successful channel
  playback after the gate ends. The probe is also run with the real default
  audio device, without the CTest dummy driver.

## End-to-End Status

New Game and Continue both traversed the real front end, naturally completed
the entire FED movie, and entered gameplay. Each checked two movie launches,
2929 FED pictures, a final video timestamp of 102899 ms, the complete 102912 ms
audio stream, and an empty audio queue at completion. Music was deferred until
the movie ended, then `01_FedFight1.wav` started on the gameplay audio owner.
Both runs passed the existing presentation/audio-handoff validations, including
non-gameplay rumble suppression and load-screen presentation.

| Route | Timing samples | Maximum device-clock difference | Maximum next-frame lateness |
| --- | ---: | ---: | ---: |
| New Game | 6169 | 16.604 ms | 27 ms |
| Continue | 6156 | 23.750 ms | 100 ms |

The probe samples `waveOutGetPosition(TIME_SAMPLES)` after D3D11 presentation
submission in a hidden 1920x1080 window on the AMD Radeon 780M. Its acceptance
limits are a device/wall-clock difference below 100 ms and no next-frame
deadline more than 100 ms overdue after presentation. These are regression
limits, not timing corrections or a promise of zero presentation jitter.
The measurements do not establish physical scanout timing or subjective sound
quality. LIVE listening remains the user's review.

The subsequent fully isolated eight-case run passed six cases. Boot stall
recovery and New Game failed the presentation-delay gate, despite device clocks
remaining aligned. No failure threshold was relaxed and no deployment was made.

| Case | Result | Maximum device-clock difference | Late deadlines over 100 ms | Maximum lateness |
| --- | --- | ---: | ---: | ---: |
| FED natural | Pass | 16.750 ms | 0 | 28 ms |
| Boot natural | Pass | 16.625 ms | 0 | 37 ms |
| FED 750 ms stall | Pass | 24.417 ms | 0 | 48 ms |
| Boot 750 ms stall | Fail | 31.000 ms | 2 | 135 ms |
| Boot skip | Pass | 28.458 ms | 0 | 25 ms |
| FED skip | Pass | 18.667 ms | 0 | 12 ms |
| New Game | Fail | 20.917 ms | 39 | 649 ms |
| Continue | Pass | 21.729 ms | 0 | 72 ms |

Logs and complete results are in `build/movie-timing-isolated`. Rebuilt
decoder, exhaustive conversion, complete timelines, boot/FED decode, audio,
XInput, and hook input regressions pass (9/9). Three negative command-line
checks confirm the timing probe requires explicit harness/audio output and
stall injection requires the probe.

A follow-up probe using `GetThreadTimes` reproduced 129-205 ms frames with
0-16 ms of main-thread CPU, and a 1133 ms New Game frame with zero measured
CPU. CPU accounting is coarse, but it distinguishes these long waits from
heavy pixel computation. Audio queuing, frame retrieval, copying, and
presentation are being timed separately to identify the blocking operation.
One 146 ms frame spent 140 ms in audio queuing; another run instead spent
65 ms retrieving video and 134 ms in presentation on separate late frames.
The per-call audio probe subsequently observed no Windows audio call over
20 ms during a complete passing boot-stall run. This does not establish a
single faulty audio API as the cause of all intermittent stalls.

Further direct checks confirm the shipped decoder uses the same kernel mutex
for packet retrieval (`THEORAPLAY_getAudio`, RVA `0x107510`) and a 240-frame
decode queue (`PlayVideo`, `0x12435A..0x124376`). Neither was changed.
The installed SDL2 WinMM driver prepares buffers during device opening
(`SDL2.dll` RVA `0x152929`), writes during playback (`0x15248C`), and unprepares
at close (`0x15234D`/`0x15236D`). The port's WinMM bridge is not claimed to be
identical to SDL's device backend; no speculative backend rewrite was made
without evidence connecting that difference to the observed waits.
SDL2 SHA256: `09A4DDB9C48D49B392C604913065162A6F3B72E37024C34C0A152D3DE7216A9F`.

## Final Verification

The final unchanged executable passed the complete eight-case matrix in one
serial run. Evidence: `build/movie-timing-release-gate/handoff-results.json`,
with per-case stdout and native logs beside it. No earlier failure was erased
or used as a pass, and neither timing threshold was relaxed.

| Case | Timing samples | Maximum device-clock difference | Maximum next-frame lateness |
| --- | ---: | ---: | ---: |
| Boot natural | 6141 | 22.375 ms | 37 ms |
| FED natural | 6167 | 27.375 ms | 27 ms |
| Boot 750 ms stall | 6096 | 23.583 ms | 44 ms |
| FED 750 ms stall | 6118 | 34.708 ms | 78 ms |
| Boot skip | 602 | 15.000 ms | 28 ms |
| FED skip | 602 | 19.479 ms | 12 ms |
| New Game | 6170 | 24.500 ms | 27 ms |
| Continue | 6166 | 26.583 ms | 31 ms |

All cases had zero clock-query errors and zero deadlines more than 100 ms
late after presentation. Full movies reached their exact expected frame counts,
final timestamps, and complete device audio lengths. Skip cases returned to
the title. Both gameplay routes passed presentation/audio-handoff validation,
released deferred music, and suppressed non-gameplay rumble.

These results establish correction of the authored-timestamp mismatch and
successful tested playback/transition paths. They do not establish that the
intermittent waits seen under concurrent workloads can never recur, or replace
human listening and visible-playback review. TODO #1 remains LIVE, unsigned.

## Deployment

The final core regression recheck passed 9/9 in 43.60 seconds. The additional
SDL_mixer SFX, stream lifecycle, and movie-SFX gate tests passed 3/3; the gate
also passed on the real audio device with no dummy-driver override. Only
`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe` was replaced;
its SHA256 matches the tested `build/Release/jpb_pc_game.exe`:
`ABCC1D879FE587649904F4E34FFA1DD77B12F388CE98B6006BB63105EF42C030`.
No harness launch arguments, test executable, save, or settings were staged.
The shipped `game.exe` retains the canonical hash recorded above.

Installed persistence hashes are unchanged from the pre-test baseline:

- `SAVEDATA0/Game`: `FD42A5EB80046F5A24117D236EA1E2B5A721356582AA871E43E06A508EE78FE8`.
- `SAVEDATA0/Options`: `46D800877B1B65157C65D4F3CFCB8A16D22F0121B28AC5045A9D87FF807FAF38`.

Normal launches do not enable diagnostics or simulated controls. Source changes
remain uncommitted; no push was requested for this goal.

One pre-isolation uninterrupted FED attempt failed its presentation gate with
six next-frame deadlines more than 100 ms late (maximum 166 ms), while the
audio-device clock remained within 23.396 ms and its decoded queue did not
starve. Native logs show both render and present delays. Other emulators and
builds were observed running concurrently, but that observation alone does not
prove their responsibility. This attempt remains a failure in the evidence;
it is not counted as a passing full playback test.
