# Saber fan review — 2026-09-17

This is a new visual effect, not a claim about reconstructed retail behavior.

The previous implementation joined the current and preceding blade with a flat
triangle and drew separate glow rods along its trailing edge and tip chord. The
native PC capture showed a hard wedge and a bright outlined corner.

`jpb_saber_fan_sector` now builds a curved sector from hilt-relative blade
vectors. Normalized direction interpolation preserves the interpolated blade
length instead of cutting a straight chord across the tips. A contiguous mesh
fades from a solid white interior into the blade color at the outer edge and
zero alpha at the perimeter. The live blade supplies its own leading glow.
The hilt stays a single apex; attacks, history reset, translation suppression,
and gameplay collision remain owned by the existing caller.

Validation:
- PC Release and nxdk Xbox builds pass and are deployed to the game folders.
- Projection, saber, and player unit tests: 3/3 pass. Coverage includes attack
  gating, white core/feather, hilt anchoring, translation, stale history and teleport reset.
- Native PC captures inspected at frames 5, 7, 10, 14, 21 and 31: curved soft
  attack sweep, narrow edge-on sweep, and no lingering fan in recovery/idle.
- PC proof: C:/Users/smmel/AppData/Local/Temp/jpb-fan-curved-sequence/
- Xbox boot and six native captures inspected; these caught recovery/idle
  rather than the brief sweeping pose. Xbox fan appearance remains unverified.
- Xbox evidence: C:/Users/smmel/AppData/Local/Temp/jpb-fan-curved-xbox-proof/
- An overly broad test selection also ran 34 tests: 28 passed, six failed
  expectation matching (title character selection 580/581, menu handoffs
  757/758/759, attack smoke 779). These failures are untriaged; do not claim
  the entire suite passes or assume they predate this change.

Before borrowing for JK: validate the actual Xbox swing, preserve per-player
and per-blade history, and evaluate sweep duration against update cadence.
The current fan uses consecutive rendered blade poses, not a fixed-time
motion-history window. No JK port has been made as part of this review.

## Full-length tip correction

The white core now reaches 100% of the physical blade length (previously 78%).
Its colored tip fade occupies only the outer 5%, with alpha falling through
65%, 18%, then zero. The leading edge joins the live blade without an inward
alpha gap. The trailing angular fade remains gradual.

PC native frames 7, 10 and 14 inspected in
C:/Users/smmel/AppData/Local/Temp/jpb-fan-full-tip-proof/.
A new regression check requires an opaque white vertex at the live blade tip.
All three focused tests pass; PC and nxdk builds pass and both game-folder
executables are updated. The running Xbox ISO/session is the preceding build;
this latest tip correction has not been visually verified on Xbox.

## Yellow trail: material alias fixed

The earlier alpha-core getter returned `translucent_glowtexture`, but both glow
loads resolve to the same filename-cached material on PC and Xbox. The first
load selects renderer class 1 (additive), so the supposed alpha fix was a no-op.
The renderer contract is 0=ordinary, 1=additive, 2=alpha; the existing variable
names and filename prefixes do not establish the actual blend mode.

The fan now uses a separate non-owning material/texture descriptor view with
materialType=2 and no color override. Pixel ownership stays with the cached
glow resource. Other effects retain their original material. The view is
refreshed from the current source after level changes, not registered as an
owning texture. Regression tests verify descriptor separation, alpha mode,
shared pixels and no color override.

Native PC comparison, identical Marsh swing at frame 7, region x=295:402,
y=240:304, bright pixels defined as every RGB channel >180: yellow excess
min(R,G)-B >8 occurred at 326 pixels before and 0 after. Maximum yellow excess
among those bright pixels dropped from 22 to 8. There are 1052 changed pixels
in that region, maximum channel change 30. This verifies an actual rendering
correction; it does not measure physical monitor response.

Corrected captures inspected at frames 5, 7, 10 and 14:
C:/Users/smmel/AppData/Local/Temp/jpb-fan-independent-alpha/.
Federation automated captures remained in the scripted opening, so those do
not establish a saber appearance check. No monitor fault is diagnosed.
PC and nxdk builds pass; three focused tests pass; both deployed executables
are updated. Xbox appearance for this revision remains unverified.

## Persistent yellow report — goal reopened

The user still saw yellow after the descriptor fix. Do not treat the prior
single-frame bright-pixel metric as resolution of the motion symptom.

Captured and sequentially inspected all 32 frames of the neutral-edge
North/overhead attack in jpb-fan-neutral-allframes. Frame 8's wide sweep
exposed a warm trailing feather outside the earlier frame-7 measurement.
The user also reports the overhead attack is less representative.

Candidate adjustment: retain a pure-white full-length core; blend the saber
hue into all fading vertices, including the angular trailing edge and hilt,
instead of fading neutral white over the scene. Tip length/fade width are
unchanged. This is not a global cool-white core tint.

Captured 32 corrected overhead frames in jpb-fan-blue-edge-allframes (full
corrected sequence review still pending). Captured and sequentially inspected
all 24 South and all 24 West attack frames in jpb-fan-all-attacks. West frame
10 matches the user's horizontal pose. Chained attacks remain to check.
All evidence folders are under C:/Users/smmel/AppData/Local/Temp/.
Three focused tests pass, including saber-colored partial-alpha vertices.
PC and nxdk builds pass and are deployed. User-perceived motion tint remains
OPEN; Xbox appearance for this candidate remains unverified.

## Controlled cool-white diagnostic

Added opt-in process environment JPB_SABER_FAN_COOL_WHITE=1. This selects
RGB 240/248/255 for the fan core; default remains pure white. It changes no
geometry, timing, attack gating or fade width. Both PC and nxdk compile;
projection tests pass with the default. This is a diagnostic, not confirmation
of symptom resolution and not a global display color adjustment.

Isolated test executable and read-only asset junction:
C:/Users/smmel/AppData/Local/Temp/jpb-saber-color-lab/
32 horizontal-attack frames captured with cool white. Active swing frames
7, 8, 9, 10, 12 and 14 visually inspected and show the expected subtle cooler
core. Also captured 48 consecutive frames with horizontal attack presses at
1, 9, 17, 25, 33 and 41; chain images await sequential inspection. Original
neutral and blue-feather sequences are retained for direct comparison.

Still cannot establish physical display motion color from native stills.
The user's observation overrides earlier completion claims; goal remains OPEN.

## Horizontal sequence: presentation gaps

Sequentially inspected all 48 baseline chain frames (individual saber-region
crops, with full-frame follow-up wherever the tip crossed a crop boundary).
Frames 11/13 and many later frames dropped the fan between wider sweeps.
Removing the eight-unit movement threshold alone did not fix those gaps:
repeated animation poses also replaced sweep history with a zero-width segment.

The renderer now permits small sweeps and retains the previous segment for at
most 256 timer units when a pose repeats (one 60 Hz presentation interval).
Attack termination, stale history, teleport and large-angle resets still apply.
The core remains default pure white; the cool-white environment opt-in is not
enabled for the relaunched normal game. No animation or gameplay timing changed.

Captured 48 revised native frames under jpb-saber-color-lab/retained-*.
Visually compared frames 11, 13, 15 and 28 against the baseline: the previously
missing broad sweep is present in 11, 13 and 28, and small motion narrows in 15.
This verifies the presentation-gap correction, not the reported motion hue.
The entire revised sequence has not yet been sequentially inspected.
Three focused tests pass, including small motion, repeated-pose retention and
expiry. PC and nxdk builds pass; both installed/staged binaries updated.
Xbox appearance and user-perceived yellow remain unverified. The diagnostic
harness reported unavailable audio-bank paths; it was not an audio test.

Reviewed all 48 retained horizontal-chain frames sequentially. The fan persists
across the repeated poses at 10-11, 12-13, 27-28, 30-31, 32-33, 35-36,
and 44-45, then narrows or disappears with subsequent poses. No persistent
large stale fan was visible. Native output still shows a white interior and
blue feather; this does not verify the user's perceived motion color.

User clarified that they keep closing the review game and that headless mode
exists. Confirmed no OpenJPB process remains. Further automated work must use
headless execution; do not relaunch an interactive review game automatically.

## Unattended follow-up

User requests autonomous completion while asleep; do not require live review
or reopen the interactive game. Headless marsh attack run completed with exit 0;
--output produced an actual rendered frame, inspected at frame 12, showing
Obi-Wan, scene, HUD and white/blue fan. --session-review-prefix currently writes
only in the windowed branch, so a headless run with that option alone creates
no phase images. Do not count its successful exit as capture evidence.

Extended projection regression coverage verifies retained fans cannot leak
between players or blade slots, and ending/restarting an attack discards a
newly retained broad fan immediately. Projection test passed. No executable
changes in this follow-up, so deployed PC/Xbox binaries remain current.
Full 48-frame corrected horizontal review was completed in the preceding turn.
The reported physical motion hue is not independently established as resolved.

## Completion audit / unresolved evidence boundary

- Full-length fan and short feather: geometry and projection regressions pass;
  corrected horizontal native sequence inspected through all 48 frames.
- Blend/material isolation: source descriptor tests and native rendering verify
  the alpha core uses a distinct material; original glow ownership is unchanged.
- Temporal behavior: repeated poses retain one intervening sweep; expiry,
  interruption, independent player/blade slots and discontinuities are tested.
- Controlled cool-white comparison: opt-in candidate captured and active swing
  frames reviewed. It is not the normal deployed default and is not proven to
  solve the reported motion color.
- PC and Xbox: builds succeeded; current build/deployment hash pairs match.
- Remaining requirement: the persistent yellow perceived in moving output is
  not reproduced in the inspected native frames. Xbox visual behavior of the
  latest revision is also unverified. These are not completion claims.

No further source change is justified by the currently observed color evidence.
Native frame inspection cannot establish the color perceived on the physical
moving display. Do not invent a monitor diagnosis or treat the user's earlier
reports as disproved. This is the first consecutive blocked-evidence audit after
concrete implementation/test progress; keep the goal active under its audit rule.
The game remains closed. No interactive operation or review request was made.
