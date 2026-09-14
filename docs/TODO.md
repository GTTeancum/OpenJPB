# TODO

## Tracked Items

September 12: removed live-confirmed items and renumbered. Autosave into level two is independently confirmed. Details and prior numbering: [LIVE_REVIEW_20260912.md](LIVE_REVIEW_20260912.md).

1. Fix second FED boss phase-two blaster audibility.
2. Mod support: characters, animation, and level design through JSON DLC and Blender.
3. Confirm startup legal/video ordering without an early Press A flash.
4. COMPLETE: Blender 4.5 BMD/CAD character import and export
5. LIVE: Blender level importer (import only)
6. LIVE: Review animation state blending (200 ms general / 100 ms attacks)
7. LIVE: Review level 3 water rendering correction.
8. LIVE: Review rocket droid smoke trail correction.

## 1. Second Boss Phase-Two Audio

- [x] User confirmed phase-one blasters are audible.
- [x] Trace phase-two firing: CAD motions 8/10 use ai_FireWeapon and projectile 20, whose authored sound field is empty; phase one explicitly plays dfrblstr.
- [x] Add the existing fighter cue to phase-two firing events; repeated-shot regression, both native phases with boss music, and non-silent mixer PCM checks pass.
- [ ] LIVE: Confirm phase-two shots are audible.

The selected music-gain fix remains in place. Earlier audio evidence: [AUDIO_MIX_20260911.md](AUDIO_MIX_20260911.md), [FED_ENDING_AUDIO_20260911.md](FED_ENDING_AUDIO_20260911.md).

## 2. Mod Support: Characters, Animation, and Level Design

- [x] Quick assessment of the existing proxy source, patcher source/configuration, and reconstructed character registry completed on 2026-09-08.
- [x] Quick level-editor assessment: the runtime already imports FBX level geometry, and J3D loading exposes authored scripts and placements. Blender is a feasible authoring foundation; complete level editing still requires game-specific import/export and validation.
- [ ] Design a shared, versioned DLC package structure for characters, animation resources, and levels, with per-package subfolders, stable IDs, dependencies, and JSON manifests. Use Blender's existing editing tools and standard asset formats with a JPB add-on; avoid a bespoke standalone editor.
- [x] Define a versioned `mods/<package-id>/mod.json` package with its own asset subfolders, stable character ID, display name, portrait, model, base-character inheritance, combat/Force abilities, saber settings, and inherited or custom sound/animation resources.
- [x] Implement native package discovery and validation, package-relative asset loading, and a character registry that handles roster/menu growth without colliding with stock player or level-object IDs. Audit fixed arrays and narrow ID fields throughout selection, loading, combat, and saves.
- [x] Replace donor-animation/combo, weapon/bone, ability, sound, and progression hooks with source-level support. Preserve stock behavior when no DLC is installed.
- [x] Support adding/removing or disabling packages without executable patching; retain progress by stable package ID and handle missing packages, duplicate IDs, invalid manifests, and missing assets cleanly.
- [x] Convert existing Kit Fisto and Aayla packages/configuration as initial examples, document the package format, and test selection, loading, combat, save/reload, removal, and both players in campaign and VS.
- [ ] Add a Blender model/animation pipeline for improved assets: define supported rigs, bone/weapon attachments, scale and axes, animation mapping, combo timing and gameplay events, materials, and export validation. Support inherited and custom animation sets through native runtime loading.
- [ ] Add Blender level creation and modification: geometry, collision, props/enemies, spawn points, waypoints, cameras, triggers, and scripted encounters. Import existing geometry and J3D gameplay data while preserving references and behavior; provide editable event properties or nodes for the engine's existing script commands.
- [ ] Implement level package loading and export/build processing for collision and spatial data, script references, level registration, and progression. Assess reliable round-trip support for existing JPX/J3D assets versus a new runtime format; new formats and replacement of the old pipeline are authorized.
- [ ] Validate the shared pipeline with an updated animated character, a modified stock level, and a small new playable level, covering collision, encounters, camera/cutscene completion, save/load, and two-player play. Document supported features and any import/export limitations.

Assessment: the existing code is at `C:\Games\Star Wars Jedi Power Battles\proxy_dll\sdl2_image_proxy.c` (the supplied `proxy\_dll` path does not exist). The game root includes `JPB Patcher.exe`, its `jpb_patcher.py` source, and `jpb_config.json`. The patcher rewrites executable tables; the proxy adds runtime roster, asset, donor-behavior, and save hooks, including a `SAVEDATA0/jpb_progress.json` sidecar. The current configuration contains 14 additions, including Kit Fisto and Aayla. The reconstruction still has a fixed 14-entry extra-character table and stock model IDs that also cover level objects, so this requires registry and loader integration beyond reading JSON. A native DLC system is a viable direction; replacing the old format/system is authorized. Use the mod code as a feature/migration reference and local canonical artifacts for inherited stock behavior. Native integration is now in progress. User requests a mods folder with the same resource substructure and copy-only migration, preserving legacy originals.

Native implementation checkpoint: [NATIVE_MOD_SUPPORT.md](NATIVE_MOD_SUPPORT.md). Copy-only staging and initial native load/attachment checks pass for all 14 additions. Campaign character selection and in-process level continuation now pass; fresh-process save/reload now retains the package. VS/training selection and gameplay movement also pass. Hybrid Force use and fresh-process two-player persistence now pass. Completion, combos and upgrades are isolated by package, including separate runtime health/Force capacities for two characters sharing a donor. Legacy aggregate unlock conversion and reset/coexistence regression checks now pass. All 14 additions pass attack/recovery and death/restart smoke tests. Campaign and VS exit/return-to-stock checks now pass. The direct quickload exit also initializes its missing title resources correctly. Native mod support is deployed: all 97 unit regressions pass, the installed executable discovers all 14 copied packages automatically, and the installed two-player smoke test passes. Legacy originals are retained. Blender authoring and new-level registration remain open below.

## 3. Startup Ordering Follow-up

- [x] Reproduce the first-frame Press A flash before the legal/warning video.
- [x] Defer title presentation when its menu owner queues a movie, matching the retail blocking movie boundary in both native and headless loops.
- [x] Native first-frame comparison and full sequence pass: opening video frame, warning, Aspyr, intro, then title prompt/music.
- [ ] LIVE: Confirm Press A no longer flashes before the legal screen.

Evidence: `out/live-review-20260912/startup-before.png`, `startup-after.png`, `startup-warning-order.png`, and `startup-full-order-native.log`. This is a follow-up to the previously closed startup feature, not a reversal of the confirmed legal-screen implementation.

Latest live review: level 2 completed without issues; sound was not tested. Former items 2 (Audio settings) and 4 (Display) closed as requested. Boss audio remains unconfirmed.


## 4. Blender Character Import and Export — Complete

- [x] Assess the installed 3.6.1 add-on against the reconstructed native BMD/CAD formats.
- [x] Fix CAD decoding, rig transforms and independent clip playback; implement CAD export with preserved gameplay metadata and events.
- [x] Correct BMD vertex counts, ownership/cache allocation, duplicate faces, normals, UVs and colors; validate edited model exports.
- [x] Compare 1,131 CAD clips / 22,890 frames with the native decoder, and evaluate 18,432 frames on seven rigs in Blender 4.5.
- [x] Verify byte-identical unedited round trips, edited exports, native geometry acceptance and an isolated game smoke run.
- [x] Build and install version 4.0.0 under the existing add-on module name, retaining a backup of 3.6.1.

Usage, limits and evidence: [BLENDER_CHARACTER_TOOLS.md](BLENDER_CHARACTER_TOOLS.md).

## 5. Blender Level Import — Import Only

- [x] Import native FBX visual geometry and paired J3D gameplay data using the runtime readers.
- [x] Add collision, placements, waypoints, powerups and start points in matching character-tool coordinates.
- [x] Preserve camera/script metadata, references and the original archive without changing source files.
- [x] Validate all 24 available stock level pairs; Blender 4.5 imports, textures and save/reopen pass for FED, Theed and Palace.
- [x] Install version 4.1.0 with the native helper, retain a 4.0 backup, and verify the installed importer with a Theed render.
- [x] Replace placement/pickup crosses with actual game models in 4.1.1; Theed verifies 245 placements and 31 pickups. Add labeled start indicators and a JPB object inspector.
- [x] Extend authored-data import in 4.2: active CAM records and camera regions, map-trigger regions and target references, complete script node graphs, placement/waypoint connections, both PWR layouts, level animation tracks, source-sidecar preservation and direct JPX geometry. FBX/JPX navigation and save/reopen checks pass.
- [ ] LIVE: Review the imported scene in Blender. Level export remains deferred as requested.

Usage and limits: [BLENDER_LEVEL_IMPORT.md](BLENDER_LEVEL_IMPORT.md).
Restart Blender and reimport the original BMD/CAD into a fresh scene; old broken imports are not repaired retroactively. Level editing remains item 5.

## 6. Animation State Blending

- [x] Add short, interruptible state crossfades for both players and AI.
- [x] Keep authored root motion, animation clocks and event data; blend joint poses using the existing angle convention.
- [x] Verify stock/mod characters and inspect native transition captures. Details: [ANIMATION_BLENDING.md](ANIMATION_BLENDING.md).
- [ ] LIVE: Check movement/attack transitions and whether the 200 ms general default feels right (attacks remain capped at 100 ms).

## 7. Level 3 Water

- [x] Reproduce the terrace water in the native renderer and trace against local EXE.
- [x] Match the PC renderer's black clear color; remove the legacy bright background from the scene target.
- [x] Native capture confirms blue water with surface detail.
- [ ] LIVE: Confirm water during level 3 play.

## 8. Rocket Droid Effects

- [x] Trace rifle CAD projectile 17, muzzle 22, slug 33, trail 13, and impact 17 (nested explosion 10 plus ring).
- [x] Match the EXE's null inherited velocity for trail emission; retain authored smoke drift.
- [x] Regression fails before the correction and passes after; native before/after captures show the restored trail.
- [ ] LIVE: Confirm rocket effects in combat.

Both corrections and the 200 ms movement/recovery adjustment: [THEED_ROCKET_REVIEW.md](THEED_ROCKET_REVIEW.md).
