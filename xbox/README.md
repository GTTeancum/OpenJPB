# OpenJPB — original Xbox / nxdk

Toolchain: **nxdk only**, Clang, nxdk libraries, `nxdk-link` and `cxbe`. No
official Xbox SDK is used. Shared gameplay sources are compiled through
generated wrappers; platform adapters live in this directory.

The current Xbox build boots from a separate asset copy in XEMU at 2× internal
resolution. Process-local virtual input has exercised FED and Marsh in-level
movement, jump and combat. Native captures show their geometry, characters,
sabers and HUD, and native AC97 captures verify named music and a saber swing.
The PC installation remains separate. The initial bring-up observations below
are historical; the later sections record current evidence. Physical-pad feel,
later-level progression, final visual fidelity, remaining effects and steady
30 fps in heavy combat are still unverified or unfinished.

## Current evidence (2026-09-16)

- `build/release/default.xbe` links and boots under XEMU 0.8.136.
- Native XEMU capture `build/xemu/captures/xemu-2026-09-16-19-49-39.png`
  shows the Federation environment, Obi-Wan, a blue saber, and the HUD.
- Rendering is preliminary: the JPX scene has conspicuous defects. This is
  not yet the completed PC FBX rendering path.
- Controller and opt-in process-local movement/jump/attack test input are now
  implemented. Their gameplay results still require inspection.
- Native capture `xemu-2026-09-16-19-52-55.png` reports 29 frames in 30,403 ms
  (about 0.95 fps) on the software backend. This fails the playability target;
  the GPU renderer is required. Do not treat more aggressive texture shrinking
  as a substitute for fixing the rendering backend.
- The opt-in `xbox-gpu.txt` prototype uses nxdk/pbkit and compiled Cg shaders.
  Capture `xemu-2026-09-16-20-03-14.png` shows textured geometry/Obi-Wan and
  reports frame 44 at 20,480 ms. A sampled frame is 436 ms, of which world
  submission is 381 ms and models 6 ms. This is still not playable. Static
  level geometry needs GPU buffers instead of per-triangle CPU submission.
  HUD composition, saber/effect passes and canonical FBX geometry are pending.
- PC regression checks for textutil, loader, model pose and level world pass
  after rebuilding the changed shared sources (2026-09-16).
- Enemy setup, canonical constructor, combat, audio, frame pacing, memory
  budgets, and PC regression checks are **not verified**. The playability goal
  remains open. Booting/rendering alone is not a passing gameplay smoke test.

### Subsequent GPU work (2026-09-16)

The static FBX path now uses `tools/export_level.c` with the existing PC FBX
adapter to produce a validated `fed.xlv`. FED's internal level index is **1**;
index 0 is not FED. `tools/inspect_level.py` checks counts, bounds and layout.
The packed FED contains 223,926 vertices and 438 material batches. Conservative
chunk frustum culling reduced submission to about 6,300 vertices at the start.

Movement previously stopped on nxdk's unimplemented `lroundf`. The Xbox math
adapter replaces it with tested halfway-away-from-zero rounding. Captures now
show movement and the scripted jump proceeding beyond that fault. The GPU HUD
compositor implements the PC black/white composition formula. Its textures use
linear storage, avoiding per-frame swizzle overhead. GPU material/effect parity,
especially the saber, is still unfinished; the visible scene is not a fidelity
pass. The current branch also tests an x86 `memset` implementation after guest
CPU samples found repeated scalar clearing dominated effect setup.

XEMU now defaults to **2x internal resolution** and a 1280x960 window, per user
request; native capture `xemu-2026-09-16-20-46-50.png` verifies 1280x960 output.
`tools/sample_guest.py` samples only the isolated guest CPU through its monitor,
always resumes it, and does not inject desktop input.

## Ongoing maintenance

### 60 Hz clock and performance update (2026-09-17)

The PC runtime continues to use the matched 60 Hz gameplay step and a 60 FPS
presentation cap. The Xbox build no longer waits for 30 FPS. It scales the
authored gameplay step by measured wall time and retains fractional 60 Hz ticks
for `totalframes` and `globaltimer`, so missed render deadlines do not slow
these gameplay clocks to half speed. The current staged disc has five selected
512 px BC1 textures while most other opaque textures remain 256 px.

With the in-process FED combat smoke under XEMU at 2× internal resolution,
final-build frames 300–500 average **32.6 FPS** (models 15.6 ms, effects 6.5
ms, scene 21.2 ms on average). Lighter post-wave sections reach roughly 60
FPS. A guest clock sample at frame 351 showed a 30 ms frame and a gameplay
step of 4424 / 1.080, compared with the PC 60 Hz step of 2048 / 0.5. Across
the following 101 rendered frames, 175 game ticks advanced in 2.86 real seconds
(61.2 ticks/s, including monitor timing uncertainty). Native captures at
frames 195, 350 and 450 show the opening droid encounter, Obi-Wan, saber, HUD
and movement. The heavy encounter still misses the 60 FPS rendering target;
16:9 at 480p and then 720p follows that target. Physical-pad feel remains
untested.

The Xbox BMD geometry cache now assigns a load generation to relocated
executable-owned model views as well as inspected BMD views. Previously, the
relocated views had generation zero and bypassed the cache. In a FED combat
smoke at 2× XEMU scale, frames 300–500 improved from 28.1 FPS in the
generation diagnostic to 38.0 FPS with the corrected 256-entry cache; frame
700 recorded 82,423 hits, 19,952 misses, and 102,375 geometry calls. One
native frame after this change showed the room, droids, and HUD. A 1024-entry
cache cut misses but reduced the same segment to 31.3 FPS, so the smaller
table remains. Doubling the GPU command-batch limit also failed to improve
the measured rate and was reverted. Model submission and pbkit cache flushing
remain the primary sampled CPU costs. Steady 60 FPS and later widescreen
targets remain open.

An opt-in model vertex-array probe (`xbox-model-vertex-array.txt` containing
`enable`) replaces immediate per-vertex commands with a bounded 384 KiB GPU
buffer. The frame-300–500 comparison was 37.1 FPS versus 38.0 FPS for the
immediate path and used 384 KiB more RAM. A native capture still showed
Obi-Wan, droids, the room, and HUD. The staged flag is `disabled`, so the
measured faster, lower-memory immediate path remains active. The same capture
shows the HUD score and icons remain blurry because the HUD is composed at
320×240 and enlarged; restoring high-resolution source textures alone cannot
fix that. Native-resolution HUD drawing is prioritized before further world
texture increases.

The staged UI priority set preserves both original 256 px debug font atlases
in PNG and TGA, raises the main HUD meter TGA to 512×256, preserves the native
256×128 PNG meter and 256×64 bars/lights, and keeps both front-end window
atlas variants at 512 px. The HUD is
still composited at 320×240 before the 2× XEMU output; texture restoration
alone does not make its small text pixel-sharp.

PC and Xbox remain supported build targets once Xbox is running. Shared
gameplay changes must preserve both; Xbox platform adapters and packaging live
under `xbox/`. Run relevant PC regression checks alongside Xbox verification.
Keep the PC installation separate from the Xbox asset copy. Windows builds
retain the executable name `OpenJPB.exe`; the Xbox disc uses `default.xbe` with
the title OpenJPB.

## Build

From the repository root in PowerShell:

```powershell
python xbox/tools/configure.py --ufbx C:/Users/smmel/.codex/deps/ufbx-0.6.1
$env:MSYSTEM='MINGW64'
& C:/msys64/usr/bin/bash.exe --noprofile --norc tools/build_nxdk.sh -C xbox -j4
```

The matched local ufbx 0.6.1 dependency is compiled using its native 32-bit
header layout and standard C I/O. The PC executable/PDB's 64-bit in-memory
layout must not be used as the Xbox library ABI.

## Separate assets and emulator

`tools/stage_assets.py` copies resources to `xbox/build/staged-disc`; original
files stay intact. The current **128px maximum is only a boot-test dataset**,
not a final quality target. Its manifest records original/staged dimensions
and hashes at repository-local `xbox/build/asset-manifest.json`. Final resizing must be selective and supported by measured RAM
budgets, preserving higher resolution where it matters. Movies are currently
excluded from staging.

`tools/boot_xemu.ps1` builds an ISO and launches the isolated configuration in
`build/xemu/xemu.toml`. It only stops prior XEMU processes using that exact
configuration. EEPROM is copied, HDD is snapshot-backed, input auto-binding
is disabled, and no host keyboard/controller events are injected.
The launcher now overwrites `OpenJPB-current.iso` after stopping that isolated
XEMU instance, so repeated tests no longer accumulate full asset images.
The ISO, captures, telemetry and other test outputs live under the ignored
repository-local `xbox/test-artifacts/` directory. Test marker files live in
`xbox/test-config/active/`; the launcher adds them only while constructing the
ISO and removes them from the repository-local staged disc afterward.
`tools/cleanup_test_isos.ps1` previews the old timestamped images and accepts
`-Execute` for manual cleanup; it excludes the active image and checks that
each candidate is directly inside the chosen ISO directory.
`tools/cleanup_repo_artifacts.ps1` likewise previews generated ISOs left in
`xbox/build`; its `-Execute` switch removes only named disposable ISO files
directly under that build directory. All Xbox outputs remain under the repository;
PC `OpenJPB.exe` releases continue to deploy to the installed PC game folder.

An `xbox-smoke.txt` file in `xbox/test-config/active/` opts into in-process input:
Confirm the level intro at frame 30, 60 right frames starting at frame 60,
60 left frames, two jumps, and continuing attack presses every 20 frames.
The overlay reports frame, elapsed milliseconds, pad bits and motion ID.
These counters support diagnosis, not a claim that combat is correct.

Native capture currently uses the local OpenJKDF2ogx reference harness:

```powershell
python 'C:/Programming/GitHub/OpenJKDF2ogx/scripts/xbox/xemu_native_screenshot.py' --pid <XEMU-PID> --xemu-exe 'C:/Games/Emulators/Xemu/xemu.exe' --screenshot-dir '<repo>/xbox/build/xemu/captures'
```

This requests XEMU's own screenshot facility; it does not capture the desktop
or send interactive host input. Inspect the captured scene and behavior.

## Current verification (2026-09-16)

The canonical FED constructor runs with Obi-Wan and active enemies. Native
XEMU captures are 1280×960 with internal scale 2. The level-intro panel, text,
and confirm prompt render; process-local confirm enters gameplay. Captures
`build/xemu/captures/xemu-2026-09-16-21-23-26.png` and
`build/xemu/captures/xemu-2026-09-16-21-23-48.png` show those two states.

The back-buffer binding enables perspective depth by default in pbkit.
This backend supplies projected Z, so it explicitly disables perspective
depth after each binding. This corrects the door incorrectly obscuring
the player and enemies. It retains perspective texture interpolation.

This is not yet a playability pass: saber transparency is visibly wrong,
combat and controls need clearer sequential verification, frame time with
enemies remains variable, audio is not connected, and final texture quality
and memory budgeting remain open. PC loader, level-world and portable-text
tests passed after the shared portability changes; these are limited
regression checks, not a complete PC gameplay validation.

`tools/index_level.py input-v1.xlv output-v2.xlv` removes byte-identical
duplicate vertices within each batch and stores 16-bit indices. It verifies
every reconstructed vertex against the input before writing. FED retains
all 74,642 triangles and 139 materials while saving 4,527,536 bytes of vertex
storage. Both file versions are accepted by the Xbox loader; PC assets and
the shared vertex structure are unchanged. Use `tools/inspect_level.py` to
validate either version.

Capture `build/xemu/captures/xemu-2026-09-16-21-34-43.png` shows the indexed
build in combat, six processed enemy-damage events and knocked-down droids,
with 3,668 KiB free at that instant. Effect artifacts remain visible; this
does not establish a complete visual or combat pass. Effects now use their
existing clip-depth value, matching PC VSImmediate, instead of projecting
that value a second time.

The next rendering investigation adds CPU near/far clipping before the
NV2A program divides by W, and suppresses wholly RGB-zero model textures
in accordance with PC PSModel's discard rule. The latter includes the
all-zero `transabr.tga` hidden saber mask. Neither change establishes visual
parity: capture `build/xemu/captures/xemu-2026-09-16-21-42-12.png` still shows
black saber quads, stretched effect polygons and missing world surfaces
later in the combat run. These must be resolved before live-review sign-off.
Guest sampling also found substantial time in pbkit's cache flush; respect
its documented 128-DWORD begin/end limit when changing submission batching.

Indexed submissions now split bindings from draws and limit each index block
to 240 entries (125 DWORDs including primitive begin/end). The diagnostic
capture `build/xemu/captures/xemu-2026-09-16-21-48-00.png` confirms the hidden
mask reaches the model pass as class 2 and wholly black. No opaque effect
with zero-alpha texels was reported in that observed flow. Visual artifacts
persist, so incorrect effect classification is not established as the cause.
The temporary material-name tracing was subsequently removed.

An opt-in `xbox-transparency-probe.txt` disc-root flag draws four known
quadrants through the effects path. Swizzled 1×1/2×2 textures produced a
black probe while the equivalent 128×128 data worked. Textures smaller than
4 pixels in either dimension now use a pitched linear layout with matching
pixel-space UV scaling. Capture
`build/xemu/captures/xemu-2026-09-16-21-56-54.png` verifies the 2×2 probe:
transparent over white, opaque blue, half-red over white, and opaque green.
This validates the small-texture fix, not overall scene rendering. Remove
the flag for ordinary gameplay inspection.

### Audio and command-buffer investigation (2026-09-16)

At this stage the SDL SFX adapter decoded WAVs on demand into a bounded 2 MiB
cache, with shared bank selection, playback, panning, distance and channel
controls. Music streaming was not connected yet. `src/audio_dma.c` is a locally modified
nxdk HAL backend (source revision and MIT attribution are retained in the file;
full license in `licenses/nxdk-audio-MIT.txt`). It enables analog PCM only:
XEMU 0.8.136 does not drain the digital SO_INDEX queue, so nxdk's normal
dual-output completion wait leaves SDL's mixer asleep. Digital output is not
supported by this adapter. It also accounts for coalesced buffer completions
after underruns rather than dropping the next callback.

An early analog-only run produced 152.054 seconds of native AC97 WAV output
(`build/xemu/analog-output.wav`), peak 8751, 6,846,460 nonzero samples and no
full-scale clipping. This proves non-silent samples reach the emulated output;
sound identity, timing, listening quality and music were not verified by that run. Native
capture uses XEMU's `-audio wav,...` backend, not OS loopback or a microphone.
`tools/capture_audio.py` provides isolated, bounded capture runs. A repeat run
failed with a GPU PFIFO assertion; preserve that failure in
`build/xemu/audio-analog-verified/`, rather than interpreting it as an audio pass.
The subsequent `audio-buffer-20260916` capture ended with an unfinalized WAV
header and black native captures and is also not a passing result.

The GPU submitted more than pbkit's 512 KiB command capacity in a single frame.
`gpu.c` now resets between complete command blocks before capacity is exhausted,
and gives pbkit's diagnostic text a fresh buffer. The normal-output capture
`build/xemu/captures/xemu-2026-09-16-22-27-33.png` shows the room and droids with
two bounded wraps in that frame. Longer stability and rendering correctness
remain open; the earlier saber/effect artifacts are not closed by this change.

The launcher also accepts XEMU's rewritten TOML whitespace and normalized
Windows path separators, so a subsequent launch selects the newly built ISO
and only stops the matching isolated instance. PC code/assets are unchanged by
these Xbox-only audio and submission changes.

The completion-accounting build reached frame 545 with 2,470 mixer callbacks
and six damage events (`xemu-2026-09-16-22-30-15.png`), but recorded eleven sound
request failures with a full cache. SDL conversion workspace was being kept
as resident PCM; the cache now shrinks successful conversions to their actual
PCM length. At frame 235 in `xemu-2026-09-16-22-32-26.png`, the compacted build
shows 1,534 callbacks, three plays, zero failures, a 225 KiB sound cache and
4,428 KiB free. The later capture `xemu-2026-09-16-22-33-11.png` reaches frame
834 (67.234 seconds of runtime), 3,831 callbacks, 82 plays, zero failures,
an 844 KiB sound cache, eight damage events and one command wrap in that frame.
Obi-Wan, an active droid and the room are visible. This clears the observed
cache-failure reproduction, not all possible sound requests or visual parity.
That test image was `build/OpenJPB-20260916-223134.iso`; XEMU remains at 2x.

### Timed visual smoke and glow investigation

The Xbox effects path now uses affine UV interpolation, matching PC
VSImmediate's W=1. Models retain perspective UV interpolation. The opt-in
smoke stops attacking at frame 600, allowing recovery/idle inspection.
`jpb_XboxSmokeFrame` exposes a read-only frame marker to the emulator monitor;
`tools/capture_smoke.py` uses it with the local OpenJKDF2 native screenshot
helper. The JSON records actual before/after frames, so late captures cannot
be mistaken for the requested exact moment. The helper must watch the active
XEMU screenshot directory; evidence copies then go into the test directory.

All eight captures in `build/xemu/glow-probe/captures.json` were inspected in
sequence (actual frames 23, 51, 110, 169, 199, 256, 451, 655). The level intro
appears and closes after the simulated confirm. Enemies move and the later
combat frames show Obi-Wan, knocked-down droids and seven damage events.
The early movement/jump captures do not show the player clearly enough to
certify those controls. Rendering still has broad rectangular saber planes
and some stretched triangles, so this is not a complete playability pass.

The glow trace resolves `a_glow.tga`, class 1/additive, with expected normalized
UVs; the opt-in native probe draws its gradient successfully. A missing glow
texture is therefore ruled out. Inspect the model versus effects contribution
and their geometry/render state next. Probe builds display the loaded glow
texture over dark gray in the lower-right corner; when no glow is loaded they
fall back to the synthetic upload test. The current diagnostic ISO is
`build/OpenJPB-20260916-224456.iso` (2x XEMU). Remove the disc-root probe flag
before ordinary gameplay review; its source asset flag has been removed.

The frame-650 glow diagnostic captured all 72 submitted vertices. An affine UV
reference rasterization using the staged `a_glow.tga` and PC SrcAlpha/One blend
produces a feathered blade, while native XEMU capture still has rectangular
planes. Suppressing only those glow draws removes the planes; that diagnostic
suppression was reverted. Forcing point sampling on the live glow also leaves
the defect (`build/xemu/glow-point`); that diagnostic was reverted as well.
The swizzled glow GPU path remained unresolved at that point.

`tools/inspect_texture.py` read the live source and swizzled GPU allocation
through XEMU's monitor and compared every pixel with the staged TGA: all 4,096
matched. Forcing the glow UVs to the transparent (0,0) texel removed the
visible saber planes in native captures at frames 453 and 656. That temporary
UV change was reverted. The texture is present and sampling affects the
artifact; GPU coordinate interpolation or blend/depth state remains to trace.
The clamp mode now uses NV2A mode 5 (clamp to edge), matching the PC sampler
and nxdk's sample description, although that change alone did not remove the
rectangles. A separate constant-Q shader diagnostic caused an uninitialized
constant to black out textures and was reverted; the rebuilt XBE restores the
pass-through shader. Generated Windows Clang dependency paths are normalized
before each nxdk build so subsequent shader edits build reliably.

Translucent materials now use a linear A8R8G8B8 GPU allocation with pixel-space
UVs. Native combat captures at frames 450, 595, and 650 in
`build/xemu/translucent-linear` show a feathered blue saber instead of the wide
rectangular card, and the former magenta explosion rectangle is gone. The live
glow source and GPU pixels match staged `a_glow.tga` exactly (4,096/4,096).
Tracing the largest immediate triangle identified the long streak as
`a_td_laser.tga`; its alpha also benefits from the linear path. Radial effect
rays at frame 595 and the remaining stretched polygons still need comparison
with the canonical PC renderer before visual parity can be claimed.

The Xbox audio adapter now streams staged PCM WAV music through a bounded
64 KiB ring alongside SDL sound effects. Its Xbox-local SDL AC97 driver queues
eight buffers, and a dedicated thread polls the analog DMA descriptor pointer;
XEMU's digital DMA never drains and its analog completion interrupts were not
reliable enough to sustain playback. A native AC97 capture without the recovery
was silent; the current polling build produces sustained output with no observed
analog halts. Two later native captures in `build/xemu/mix-native-pair` and
`build/xemu/music-repeat` contain over 1.3 million nonzero samples apiece with
no clipping. Their music matches staged `01_FedFight1.wav` at 0.99+ normalized
correlation in consecutive two-second segments at the expected source offsets.
A temporary three-second guest mixer capture confirmed the same content before
AC97 DMA. The memory probe is opt-in at compile time
(`JPB_XBOX_CAPTURE_MIX`) and absent from ordinary builds. This verifies the
level-one music path in XEMU. Specific sound effects and listening quality
still need review.

The intermittent music discontinuity was caused by the AC97 descriptor ring
crossing a 4 KiB virtual page boundary. AC97 receives only the first physical
address for the ring, so a noncontiguous following page can make DMA read the
wrong descriptors. The Xbox-local device record is now page-aligned; its PCM
ring lies entirely within one page. Three ordinary-build captures after this
change keep identical source-to-capture offsets across the tested music
segments, with 0.89–0.99 normalized correlation. One separate capture instance
ended in XEMU's existing PFIFO pusher assertion; the retry produced correct
music. This emulator/GPU stability issue remains open.

The diagnostic text overlay is hidden in ordinary builds so game visuals can be
reviewed. Place `openjpb-diagnostics.flag` at the root of the Xbox asset copy
before creating an ISO to opt into the live counters.

The current pbkit push buffer is 1 MiB, up from its 512 KiB default. A 2 MiB
trial avoided the intermittent XEMU PFIFO out-of-range/reserved-command abort,
but exhausted guest physical pages around frame 700. The 1 MiB build reached
frame 1,508 without a PFIFO abort in `build/xemu/pushbuffer-1mb`; it retained
about 150 free 4 KiB pages after frame 500. Longer runs then stopped inside
`game_runStage()` when `StageExit` called `restore_events()`. The reconstructed
event-list cursor started at the ring base, although `clear_eventlist()`
establishes it after a zero sentinel; the restore loop stepped before the
ring. Startup now uses the cleared cursor, and restore wraps only after
crossing the ring's lower bound. The fixed XEMU smoke reached frame 9,599
after the stage reset, with 765 SFX plays, zero SFX failures and 150 free
pages. Native images in `build/xemu/event-reset-fixed` and
`build/xemu/event-reset-soak` show Obi-Wan and respawned droids with the score
reset. The corresponding PC build,
game/game-frame/event-list tests and three-frame real-asset PC smoke pass. This is
still a level-one smoke; all-level stability and the precise PFIFO cause
remain open. The final pointer-safe ring change was rechecked in the PC build
and at frame 4,103 in a fresh Xbox boot after StageExit; that boot still had
zero SFX failures and 148 free pages. That event-ring regression ISO was
`build/OpenJPB-20260917-023910.iso` at 2x XEMU resolution; see the newer
BC1 run below.

The opt-in smoke now exercises right/left movement and jump after the opening
encounter releases P1. `tools/capture_smoke.py` saves read-only player telemetry
beside each native screenshot. In `build/xemu/post-encounter-controls`, P1's
world Z changes from -13954 to -14278 during held right input and to -14647
during held left input; at frame 830 the world Y rises from 3328 to 3479 with
jump motion 4. All ten captured images were inspected in sequence and show
Obi-Wan and the remaining droids as he moves. The earlier smoke's movement
inputs occurred during the scripted hold and therefore did not test control.
Combat in `build/xemu/controls-telemetry` registers seven damage events.

The isolated Xbox asset copy now stages all 1,809 SFX WAVs as 48 kHz stereo
PCM16. The Xbox audio adapter reads them directly into a fixed 2 MiB sample
pool, avoiding SDL conversion allocations during play. The 1 MiB push-buffer
smoke recorded 127 SFX plays and zero failures at frame 1,508. This does not
prove that each effect has the right identity or audible mix.

`build/xemu/audio-1mb-sfxpool/analog-output.wav` is a 24.59-second native
XEMU AC97 capture from the 1 MiB/audio-pool build, with 1,704,570 nonzero samples and
no clipping. Consecutive segments align with staged `01_FedFight1.wav` at the
same ~6.825-second offset (correlations 0.675, 0.779 and 0.855 while effects
are mixed in). The earlier `build/xemu/audio-2mb-controls/analog-output.wav`
music-only comparison correlated 0.977, 1.000 and 1.000 at a 6.758-second
offset. A second native capture in `build/xemu/audio-sfx-trace` contains the
current build's audio. The bounded play trace identifies `sabrsw05.wav` in
Obi-Wan's attack sequence; `tools/verify_sfx.py` matches its staged waveform
at 29.224 seconds in the capture with 0.708 correlation after subtracting
aligned music. This verifies one saber swing reaches AC97. Other effect
identities and listening quality remain to be verified.

### 256 px FED world texture trial (2026-09-17)

The private Xbox asset copy now keeps the 128 px TGA files used by the CPU
loader and additionally stages opaque FED JPX textures as 256 px BC1 `.xbt`
files. The Xbox GPU loads these with nxdk's DXT1 format. This does not change
the PC resources. The first trial silently fell back to the 128 px textures
because its guest path used forward slashes; native XEMU frame 300 was exactly
identical to the previous capture. With Xbox path separators fixed, 30 BC1
textures load by frame 700 with zero open failures. Native 1280×960 captures
in `build/xemu/bc1-fed-256-fixed` and `build/xemu/bc1-state-cache` show the
changed wall and panel details while Obi-Wan, droids, HUD, and saber remain
visible. At frame 700 the compressed build has 388 free 4 KiB pages and zero
SFX failures, compared with about 150 pages in the prior 128 px GPU-texture
soak. This is a selective texture improvement, not final visual parity or a
replacement for higher quality model and later-level assets.

The 2x XEMU timing probe measures about 50–66 ms per combat frame in the
opening encounter. Actor rendering accounts for roughly 18–37 ms; world
submission is around 1–4 ms. Consecutive model triangles now reuse unchanged
GPU texture/sampler state, but the native scene and timing still need a longer
review before claiming a frame-rate improvement or full-speed playability.
The 1 MiB push-buffer build with BC1 and state reuse passed another StageExit
reset: the native frame 4,100 capture in `build/xemu/bc1-state-cache-soak`
shows Obi-Wan and live droids with score reset. Its monitor recorded 329 SFX
plays, zero failures, and 386 free 4 KiB pages.

The same ISO's isolated native AC97 capture in
`build/xemu/bc1-state-cache-audio/analog-output.wav` contains 36.33 seconds
of audio, 2,839,824 nonzero PCM samples, and no clipping. Three segments match
staged `01_FedFight1.wav` at a common 6.738-second offset with correlations
0.896–0.940. After subtracting that music, `sabrsw05.wav` matches at
26.352 seconds with 0.946 correlation. This rechecks music and one combat SFX
after the texture and GPU-state changes; other sound identities and listening
quality remain open.

### XEMU frame pacing trial (2026-09-17)

`pb_finished()` already queues a VBlank swap through pbkit's triple buffering.
The Xbox loop no longer waits for an additional VBlank before beginning every
frame. It sleeps only when a frame finishes inside 33 ms, preserving the
game's 30 Hz simulation pace when there is spare time. At 2x XEMU resolution,
native captures in `build/xemu/nonblocking-vbl` reached frames 300, 700,
1,500, and 4,100. The same 800-frame segment from about frame 700 to 1,500
took roughly 28 seconds here versus 38 seconds with the extra wait; screenshot
timestamps make these approximate wall-clock measurements. The frame-4,100
capture still shows Obi-Wan, droids, saber, world and HUD after StageExit.
The monitor reports 386 free pages, 293 SFX plays and zero failures there.
Individual heavy combat frames can still take 50 ms or more, so the port is
not yet a verified steady 30 fps experience.
The matching native AC97 capture in `build/xemu/nonblocking-vbl-audio`
contains 36.29 seconds of audio with no clipped PCM samples. Three segments
match `01_FedFight1.wav` at a shared 6.69675-second offset (correlations
0.834–0.895). After removing that music, `sabrsw05.wav` matches at
25.394 seconds with 0.909 correlation. Audio still requires broader effect
identity and listening review.

### 256 px character texture investigation (2026-09-17)

The private asset pipeline stages 256 px BC1 MODEL textures alongside the
128 px CPU TGAs. The first test loaded Obi-Wan and droid BC1 resources, but
native captures in `xbox/test-artifacts/model-bc1-review` showed dark and
noisy patches. `tools/inspect_bc1.py` decoded the staged images correctly;
`tools/inspect_bc1_upload.py` confirmed the entire live `obi_arm.xbt` GPU
payload matched its file. A diagnostic quad showed that its normal V range
sampled the adjacent torso texture. There was no push-buffer reset between
its triangles, and a forced texture rebind did not help. A 256 px uncompressed
Obi-Wan head rendered correctly. Halving BC1 V coordinates kept sampling
within the intended texture; the same correction is applied to world and
model paths in `src/gpu.c`. The diagnostic quad was removed.

The corrected build's 2× native captures at frames 300, 700, 1,500 and 4,100
are in `xbox/test-artifacts/bc1-vhalf-world-model`. All four were inspected:
characters, FED geometry, saber and HUD render coherently throughout combat
and after the StageExit reset. The frame-4,100 monitor reports 587 free pages,
330 SFX plays and zero allocation failures. A fresh native AC97 recording in
`xbox/test-artifacts/bc1-vhalf-audio` has no clipped PCM samples. Three
segments match `01_FedFight1.wav` at a shared 7.34175-second offset
(correlations 0.956–0.984), and `sabrsw05.wav` matches after subtracting the
music with correlation 0.981. Wider visual and audio coverage remains open.

### Level-two Marsh smoke (2026-09-17)

The private asset copy now includes an indexed `marsh.xlv` (83,358 triangles,
67,586 unique vertices) and 140 opaque Marsh textures at 256 px BC1. An
optional `D:\xbox-start-level.txt` marker containing `marsh` starts this
level for isolated testing; the default remains FED, and the PC installation
is unchanged. The marker was removed from the staged asset copy after review.

The first 2× native XEMU captures exposed a large black floor despite a green
source texture. A diagnostic per-batch color pass traced it to the `o_grass1`
floor batch. Its authored V coordinates range outside 0–1 (at the player,
about -10), which defeated the compressed-texture half-V workaround. A first
CPU vertex-wrap trial restored the floor but could distort triangles crossing
UV tile boundaries. The current `tools/tile_bc1_level.py` instead rebases V
per triangle in the private indexed XLV asset, duplicating only vertices that
need different UVs. Each GPU BC1 world upload places a second copy directly
after the first so a triangle can cross one tile boundary without sampling a
neighboring texture. The staged FED and Marsh XLVs were rebuilt with this
tool; the source geometry positions and PC assets remain untouched. Run the
tool on a fresh indexed XLV after `tools/index_level.py`, with the staged JPX
folder as `--texture-dir`. Keep that raw indexed XLV as the input and write the
tiled output to a separate path. The tool checks that every reconstructed
vertex is byte-identical except for V; both staged levels passed that check
and the XLV structure validator.

Native 2× XEMU captures in `xbox/test-artifacts/marsh-rebased-bc1` show the
grass floor, trees, Obi-Wan and HUD without the dark speckled strips seen in
the earlier BC1 trial. Marsh frame 660 reaches the same player position and
records 23 SFX plays with zero allocation failures. FED captures at frames
500, 1,000 and 4,100 in `xbox/test-artifacts/fed-rebased-bc1` retain the
opening fight, droids, saber, world and HUD through a StageExit reset; frame
4,100 has 333 free 4 KiB pages, 202 SFX plays and zero failures. Wider visual
parity, later levels and level progression remain open.
The same staged FED build also reached native XEMU frame 9,600 in
`xbox/test-artifacts/fed-rebased-longrun`: the captured frame still shows
Obi-Wan, live droids, level geometry and HUD. The monitor recorded 599 SFX
plays, zero allocation failures and the same 333 free 4 KiB pages, so the
second BC1 payload did not cause a late memory decline in this route.

The dark opening behind Marsh foliage also appears in a process-local,
headless Windows `OpenJPB.exe --quickload marsh` capture, so it is present in
the current PC rendering of this level. A separate native XEMU AC97 capture
in `xbox/test-artifacts/marsh-audio-review` contains 20.57 seconds of
audio, no clipped samples, and three segments matching staged
`02_MarshAmbient1.wav` at the same 6.0395-second offset (correlations
0.634–0.967). The capture establishes that ambient music is playing; it
does not verify every effect or listening quality.

The initial Marsh movement smoke was invalid: its input sequence sent Up
while the authored level card still awaited A. After dismissing the card,
Up played a running animation but moved the player backward into the spawn
boundary. The Xbox pad adapter had left `player1InputType` at -1 and had not
published the pad axes that the shared controller path requires. The adapter
now sets controller mode and supplies axes with its digital direction bits.
The native 2× XEMU captures in `xbox/test-artifacts/marsh-pad-axes` show
Obi-Wan travelling into the forest and reaching Z=-6199 from Z=-7168. An
independent 300-frame headless PC virtual-W run reaches the same final Z.
Marsh frame 660 records 23 SFX plays with zero allocation failures; level-one
regression frames 500 and 1,000 in
`xbox/test-artifacts/fed-pad-axes-regression` retain visible world, combat,
HUD and later movement with 75 SFX plays and zero failures. The staged
level-select marker was removed after that review, restoring FED as the Xbox
build's default start. The pad adapter now feeds raw Xbox stick axes and
button state through the shared `jpb_InputMapControllerState` path. A
process-local synthetic stick test in Marsh recorded `JPB_PAD_UP` at half
deflection (frames 80-180), then `JPB_PAD_UP | JPB_PAD_ANALOG_MOVEMENT` at full
deflection (frames 188-285); Obi-Wan advanced from Z=-7168 to Z=-6199. The
native emulator-monitor trace is in
`xbox/test-artifacts/marsh-analog-telemetry/captures.json`. This verifies
game-side stick mapping and the walk/run threshold; a physical controller
and subjective analog feel remain unverified.

After removing the Marsh and analog selectors, a fresh default FED disc
booted at XEMU surface scale 2 with the shared controller mapper. Native
captures in `xbox/test-artifacts/fed-shared-controller-regression-2` show
Obi-Wan, a live droid, textured corridor, saber and HUD during combat.
The monitor reported zero SFX allocation failures and 333 free 4 KiB pages.
`capture_smoke.py` now reads the active emulator configuration's screenshot
directory by default; a follow-up native capture in
`xbox/test-artifacts/fed-auto-capture-path` verified that path resolution.

### 512 px selective quality and combat profiling (2026-09-17)

`texture-quality.json` selects two prominent FED world textures plus
Obi-Wan's head and torso from original 512–1024 px sources. Run
`tools/upgrade_textures.py` against the original game and the private Xbox
staging directory after initial staging; it updates only the selected BC1
files and their manifest hashes. Native 2× captures in
`xbox/test-artifacts/fed-512-quality-retry` and
`xbox/test-artifacts/fed-512-batch384-visual` show textured FED geometry,
Obi-Wan, droids, the saber and HUD. The latter recorded zero SFX allocation
failures and 185 free 4 KiB pages. The installed Windows executable was not
replaced.

`tools/profile_frames.py` reads guest timing counters through the XEMU monitor.
An early combat baseline around frames 417–763 was 23.2 FPS. Larger contiguous
model batches and reuse of BMD color, camera and projected vertex work improve
the current frames 300–700 interval to 26.6 FPS (sparse monitor sampling;
`xbox/test-artifacts/perf-fed-512-batch384.json`). When the active wave
thins after frame 585, one-second intervals measure about 30 FPS. The frame
300–700 target is still unmet: approximately 4,600 triangles per frame in the
busy section fall to roughly 1,000–1,500 afterward. All 12 enemy classes and
the sampled BC1 textures were already loaded by frame 300, so the dip tracks
active geometry rather than first-use BC1 loading. PC BMD, model-pose and
projection tests pass after the shared renderer changes. The next optimization
must address sustained model/effect submission under multiple live actors.

The September 17 model-command build batches eight model draws per PBKit
submission and reuses model transform scratch. Comparing equal *game-tick*
windows, rather than frame numbers that shift with rendering speed, shows
30.0–36.8 FPS over ticks 300–800 versus 21.7–28.9 FPS before batching.
The subsequent Xbox-only inverse-depth cache gave mixed interval results
(28.5–39.1 FPS); it is not evidence of a stable 60 FPS. One native 2× combat
capture in `xbox/test-artifacts/inverse-depth-smoke` shows Obi-Wan, a droid,
saber, FED room and HUD without visible corruption. It recorded 57 SFX plays,
zero allocation failures and 238 free 4 KiB pages. XEMU's native audio WAV
contained 79 seconds of nonzero stereo PCM. The latest PC build and BMD,
model-pose and projection tests pass.

The Xbox HUD now submits source texture cards directly to the active video
viewport after the world composite. Its placement derives from the logical
HUD canvas and `XVideoGetMode()`; changing the canvas width for 16:9 will not
require a fixed 640×480 HUD mapping. A native 2× FED capture from the
September 17 build shows the HUD artwork and gameplay together, and its
frame-300–450 timing intervals remain about 33–45 FPS. TrueType score text
still comes from the low-resolution software composite and looks soft; sharp
text and full widescreen world projection remain open. The test used the
single overwritten `OpenJPB-current.iso` and XEMU was stopped afterward.

Subsequent native combat profiling narrowed the model cost: a 130-frame
window averaged 14.5 ms for models, of which GPU model-batch submission took
about 4.0 ms. A temporary triangle-sink timer showed roughly 5.4 ms for all
model sink calls including the batch work, leaving approximately 10.6 ms in
model geometry transformation, culling, and clipping. The timer was disabled
again after the diagnostic run. An Xbox-only direct face-access experiment
failed to improve tick-aligned combat and was reverted. One native capture
from each run was inspected; both retain the FED arena, Obi-Wan, live droids,
and HUD. The next optimization should target model processing before the sink.
After these runs, `cleanup_test_isos.ps1 -Execute` removed seven historical
timestamped ISOs (7.17 GiB). Only the overwritten current ISO remains in the
test directory.

Gameplay TrueType text now emits cached SDL_ttf glyph textures directly at
the active output viewport. The score in the native 2× capture
`xbox/test-artifacts/captures/xemu-2026-09-17-11-00-56.png` is visibly
sharper than the earlier enlarged 320×240 score. When both HUD card and text
GPU hooks are active, the game skips the unused black/white software HUD
composite; the first text attempt kept those buffers, exhausted free guest
pages and fell to about 21 FPS, so that attempt was replaced. With the
composite skipped, the tick-500–600 and 600–700 combat windows measured 38.55
and 46.54 FPS, respectively, versus 33.27 and 37.33 FPS before direct text.
Guest free pages were about 202 at frame 430. The Windows game build still
compiles. Gameplay text and cards scale from the current logical canvas to
`XVideoGetMode()`; world projection and 16:9 validation remain separate work.
A second 2× run reached frame 1,200 with 197 free pages and a stable 114
loaded textures through frames 700–1,200. The lighter post-wave intervals
were about 58–60 FPS, but the crowded earlier combat still missed 60 FPS.
Its one inspected native capture shows the room, enemies, HUD, and sharp score.
XEMU's native audio capture contained 84.85 seconds of stereo PCM, 6.79
million nonzero samples, a peak of 20,345, and no clipped samples; this
checks activity and headroom, not every effect identity. Both ISO cleanup
scripts then found zero new disposable images; one overwritten current ISO
remains.

The first widescreen pass uses a 426×240 logical gameplay canvas and a
720×480 anamorphic Xbox output presented at 16:9 in XEMU. A native 2×
capture shows the FED room, Obi-Wan, droids, and HUD across the full output.
The Xbox executable currently boots directly into gameplay; 4:3 menus and
FMVs still need to be centered with side bars when those paths are ported.
An Xbox-only phase timer in the FED crowded wave measured about 3.5 ms per
frame for model vertex transforms and 12.0 ms for face processing, including
4.9 ms in GPU model-batch submission. Moving face culling ahead of UV/color
assembly did not improve the measured wave and was reverted. The diagnostic
timer was removed after the measurement so it does not affect staged timing.
For frame-aligned performance comparisons, an isolated test disc may contain
`xbox-fixed-step-benchmark.txt`. It advances the in-process smoke simulation
at exactly 1/60 second per frame and ties its virtual controller sequence to
game ticks. The marker is absent from the staged game, which retains measured
wall-time simulation. Two fixed-step baseline runs matched game ticks and
initial triangle counts, though XEMU wall-time performance still varied. An
inline model push-buffer writer was slower than pbkit in this comparison and
was reverted.
The optional 720p test mode uses a disposable EEPROM copy under `xbox/build`
and never changes the shared emulator EEPROM. Late pbkit initialization
returns `-11` while allocating the 720p depth buffer. Reserving it early
works, but originally left too little room for the FED level's 4.7 MiB vertex
array. The Xbox GPU loader now packs vertices to 32 bytes (full-precision
position, UV and UV scroll, byte colors), saving about 1.3 MiB for FED. It
loads XLV batches through a small fixed buffer instead of a transient 0.29
MiB allocation. Native 2× captures of FED and Marsh show their textured
scenes, character and HUD in 480p; Marsh recorded 17 SFX plays, zero
allocation failures and 1,486 free pages at frame 536. The Windows game
build also passes. At 720p, GPU initialization and the FED constructor now
both succeed, but only 58 free 4 KiB pages remain and USB controller setup
asserts before frame zero. The same effective low-memory condition appears
with a 128 MB XEMU guest. More residency work is needed before 720p is
playable; the black 720p native capture is failure evidence, not a pass.

Further phase timing on the crowded FED wave put vertex transformation at
roughly 3.9 ms and face processing at roughly 11.3 ms per frame; about 3.9 ms
of the latter was model-batch submission. A conservative node-frustum test
rejected about 22 nodes per frame and reduced submitted triangles, but did
not improve tick-aligned FPS. Inlining vertex decode and transforms reduced
the vertex segment by about 1.5 ms, yet early-wave frame time regressed as
face/batch time rose. Both experiments and their phase timers were removed;
the staged XBE again uses the verified direct-HUD baseline. This evidence
points toward a broader change in face processing or GPU model submission,
not another per-node check or small call-site rewrite.

The next 2× native capture, `xbox/test-artifacts/captures/xemu-2026-09-17-11-34-43.png`,
still shows soft gameplay score text. The glyph sampler now uses point sampling
because SDL_ttf already rasterizes antialiased coverage at the 640×480 output
pixel size, but changing the sampler alone does not add detail to XEMU's
1280×960 enlargement. The current XBE remains at 640×480; a higher-resolution
video mode and matching world/HUD viewport transforms remain open. The
frame-430 smoke rendered the FED room, Obi-Wan, droids, and HUD. Xbox-only
`-O3` and opaque-culling bypass experiments regressed crowded combat and were
reverted before staging.

The 480p acceptance pass after the model-array rollback used the staged direct
renderer and one overwritten ISO. Its frame-650 native capture in
`xbox/test-artifacts/acceptance-480p-20260917` shows the FED doorway, Obi-Wan,
droids, and HUD with no missing polygons in that view. A 512-frame performance
ring measured 23 ms median, 28 ms at the 95th percentile, and 38 ms worst.
At frame 1,619, audio counters showed 150 SFX plays with zero allocation
failures; 392 free pages remained. This establishes the accepted 480p combat
baseline, not a full visual pass through every level or menu/FMVs. The emulator
was stopped after capture, and cleanup retained only the current test ISO.

A matched fixed-step combat comparison at game ticks 300–499 separated XEMU
presentation scale from guest rendering cost. At surface scale 1×, the 200
frames averaged 26.2 ms with 15.6 ms in models; at the requested 2× scale,
they averaged 25.5 ms with 15.1 ms in models. Both runs held the same 200 game
ticks and passed SFX allocation checks. The small difference is within run
variation, so lowering XEMU's display scale is not a meaningful route to 60
FPS. The benchmark marker was removed and the single current ISO rebuilt from
the normal staged game after the comparison.

The BMD geometry view validates face indices and UV spans at load time. The
Xbox model loop now reads those validated records directly instead of calling
the generic checked accessors once per corner; PC keeps the checked path.
At 2× XEMU scale, matched game ticks 300–499 averaged 25.51 ms/frame and
15.08 ms in models before this change, versus 24.82 and 14.49 ms afterward.
The new frame-500 native capture shows the FED doorway, Obi-Wan, droids, floor,
and HUD intact; it recorded 57 SFX plays and zero allocation failures with
390 free pages. The gain is small and crowded combat remains below 60 FPS.
The PC Debug/Release builds and projection/BMD/model/model-pose tests passed;
the Release `OpenJPB.exe` and Xbox XBE are staged. Both test markers were
removed, the one ISO rebuilt, and XEMU stopped after the run.

Sharing projected corners across the two triangles of an in-front BMD quad
was also tested at the same ticks and 2× scale. It measured 24.70 ms/frame
against 24.82 ms for the prior build, an inconclusive 0.12 ms difference. The
native frame-500 capture looked intact, but the extra branch was reverted to
avoid adding rendering complexity without a clear gain. The direct validated
face-access build above is again staged on PC and Xbox; the fixed-step marker
was removed and the one ISO rebuilt for normal wall-time gameplay.

A second Xbox-only experiment cached no-scale clip coordinates per transformed
model vertex. In the same 2× fixed-step combat window, frames 300–500 fell to
about 29–32 FPS, versus roughly 38–43 FPS on the staged baseline. The native
capture showed the scene but the cost regression was large, so the cache was
removed. The validated-face build was rebuilt and restaged on both targets;
the benchmark marker is absent, the owned XEMU process is stopped, and only
the current ISO remains. Further 60 FPS work needs a different model path.

The restored build was also run through a current Marsh level-two smoke at
2× XEMU scale. Its frame-550 native capture shows Obi-Wan, saber, trees,
rebased grass floor, and HUD. The dark opening behind foliage matches the
previous headless PC reference described above; one capture cannot establish
full-level visual parity. Frames 300–550 ranged from 29.36 to 32.38 FPS in
50-frame intervals, with 1,486 free pages, 32 SFX plays, and zero allocation
failures. The Marsh selector was removed and the single normal FED ISO rebuilt
after the run.
