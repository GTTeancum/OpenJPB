# Live review — September 13, 2026

User confirmed former TODO items 3, 4, 6, 7 and 8 complete. The numbers below preserve the prior list.

## 3. Startup Ordering Follow-up

- [x] Reproduce the first-frame Press A flash before the legal/warning video.
- [x] Defer title presentation when its menu owner queues a movie, matching the retail blocking movie boundary in both native and headless loops.
- [x] Native first-frame comparison and full sequence pass: opening video frame, warning, Aspyr, intro, then title prompt/music.
- [x] User confirmed September 13: Confirm Press A no longer flashes before the legal screen.

Evidence: `out/live-review-20260912/startup-before.png`, `startup-after.png`, `startup-warning-order.png`, and `startup-full-order-native.log`. This is a follow-up to the previously closed startup feature, not a reversal of the confirmed legal-screen implementation.

Latest live review: level 2 completed without issues; sound was not tested. Former items 2 (Audio settings) and 4 (Display) closed as requested. Boss audio remains unconfirmed.


## 4. Blender Character Import and Export â€” Complete

- [x] User confirmed closure on September 13.
- [x] Assess the installed 3.6.1 add-on against the reconstructed native BMD/CAD formats.
- [x] Fix CAD decoding, rig transforms and independent clip playback; implement CAD export with preserved gameplay metadata and events.
- [x] Correct BMD vertex counts, ownership/cache allocation, duplicate faces, normals, UVs and colors; validate edited model exports.
- [x] Compare 1,131 CAD clips / 22,890 frames with the native decoder, and evaluate 18,432 frames on seven rigs in Blender 4.5.
- [x] Verify byte-identical unedited round trips, edited exports, native geometry acceptance and an isolated game smoke run.
- [x] Build and install version 4.0.0 under the existing add-on module name, retaining a backup of 3.6.1.

Usage, limits and evidence: [BLENDER_CHARACTER_TOOLS.md](BLENDER_CHARACTER_TOOLS.md).

## 6. Animation State Blending

- [x] Add short, interruptible state crossfades for both players and AI.
- [x] Keep authored root motion, animation clocks and event data; blend joint poses using the existing angle convention.
- [x] Verify stock/mod characters and inspect native transition captures. Details: [ANIMATION_BLENDING.md](ANIMATION_BLENDING.md).
- [x] User confirmed September 13: Check movement/attack transitions and whether the 200 ms general default feels right (attacks remain capped at 100 ms).

## 7. Level 3 Water

- [x] Reproduce the terrace water in the native renderer and trace against local EXE.
- [x] Match the PC renderer's black clear color; remove the legacy bright background from the scene target.
- [x] Native capture confirms blue water with surface detail.
- [x] User confirmed September 13: Confirm water during level 3 play.

## 8. Rocket Droid Effects

- [x] Trace rifle CAD projectile 17, muzzle 22, slug 33, trail 13, and impact 17 (nested explosion 10 plus ring).
- [x] Match the EXE's null inherited velocity for trail emission; retain authored smoke drift.
- [x] Regression fails before the correction and passes after; native before/after captures show the restored trail.
- [x] User confirmed September 13: Confirm rocket effects in combat.

Both corrections and the 200 ms movement/recovery adjustment: [THEED_ROCKET_REVIEW.md](THEED_ROCKET_REVIEW.md).

## Second FED Boss Phase-Two Audio — Complete

- [x] User confirmed phase-one blasters are audible.
- [x] Trace phase-two firing: CAD motions 8/10 use ai_FireWeapon and projectile 20, whose authored sound field is empty; phase one explicitly plays dfrblstr.
- [x] Add the existing fighter cue to phase-two firing events; repeated-shot regression, both native phases with boss music, and non-silent mixer PCM checks pass.
- [x] User confirmed September 13: phase-two audio is fixed.

The selected music-gain fix remains in place. Earlier audio evidence: [AUDIO_MIX_20260911.md](AUDIO_MIX_20260911.md), [FED_ENDING_AUDIO_20260911.md](FED_ENDING_AUDIO_20260911.md).

