# TODO

## Tracked Items

1. Shared DLC implementation and Blender authoring.
2. LIVE: Blender level importer review.
3. Original Xbox port (nxdk/XEMU).
4. LIVE: Level 2 pause-menu rendering.

## 1. Shared DLC Implementation and Blender Authoring

Implement the completed [shared package design](DLC_PACKAGE_DESIGN.md). Keep existing version-1 mods and legacy assets compatible.

- [ ] Implement version-2 manifests, dependencies, stable export references, scoped resource bindings, shared animation libraries and save migration.
- [ ] Extend the existing Blender BMD/CAD tools into the shared package pipeline: rig/profile validation, animation and gameplay-event mapping, materials, and package build/export validation. Existing BMD/CAD import/export is already available; see [character tools](BLENDER_CHARACTER_TOOLS.md).
- [ ] Complete Blender level authoring and export for collision, placements, cameras, triggers and scripted encounters. Build on the existing geometry/gameplay importer and script inspectors; preserve source references and unknown data where a lossless round trip is supported.
- [ ] Implement level package loading, collision/spatial rebuilding, script compilation, new-level registration and progression. Preserve explicit visual-source selection and reject unsupported or lossy exports.
- [ ] Validate the shared pipeline with an updated animated character, a modified stock level and a small new playable level, covering visuals/audio, collision, encounters, camera/cutscene completion, save/load and two-player play. Document remaining limitations.

## 2. LIVE: Blender Level Importer Review

- [ ] Review the imported scene in Blender. Complete level export remains separate implementation work above.

Usage and limits: [Blender level import](BLENDER_LEVEL_IMPORT.md).

## 3. Original Xbox Port (nxdk/XEMU)

Maintain both PC and Xbox builds with shared gameplay and Xbox-only code under `xbox/`. Use nxdk, a separate copy of game assets, and XEMU at 2× resolution. Crowded-wave phase timing found about 3.9 ms of vertex transforms and 11.3 ms of face processing per frame; tested node-frustum rejection and inline transforms did not raise tick-aligned FPS. The next performance change must tackle face processing or GPU model submission at a broader level.

- [x] Establish an acceptable 480p Xbox performance baseline at 2× XEMU resolution while keeping Windows `OpenJPB.exe` at its 60 Hz clock. The Xbox 30 FPS cap is removed; gameplay scales by measured wall time and its frame counters retain fractional 60 Hz ticks. Five prominent FED/Obi-Wan BC1 textures use 512 px sources. HUD texture cards and TrueType gameplay text draw at the active output viewport, with placement derived from the logical canvas and video mode. The true 16:9 gameplay pass runs at anamorphic 720×480. After reverting the model-array experiment that dropped polygons, a native FED combat capture showed the doorway, Obi-Wan, droids, and HUD intact. A 512-frame ring measured 23 ms median (about 43 FPS), 28 ms at the 95th percentile (about 36 FPS), and 38 ms worst; the run recorded 150 SFX plays, zero allocation failures, and 392 free pages. The user accepted approximately 30–40 FPS at 480p provided visual glitches are resolved. Broader level and physical-pad validation remain open below.
- [ ] Keep gameplay true 16:9 without stretching. When the Xbox frontend and media paths are implemented, center their original 4:3 picture in the 16:9 output with black bars on the left and right (pillarboxing), including the main menu and FMVs. The current Xbox test executable boots straight into a level, so menu/FMV presentation cannot yet be validated. Evaluate 720p separately: an isolated EEPROM enables the mode, and a 32-byte packed GPU level vertex saves about 1.3 MiB for FED while retaining full-precision position/UV/scroll values. Both staged levels render in native 480p captures with the packed format, and the PC build passes. Reserving 720p buffers before level assets now gets through the FED constructor, but leaves only 58 free 4 KiB pages; USB controller initialization asserts before the first frame. Reduce more resident memory while preserving higher-resolution textures and audio before treating 720p as playable.
- [ ] Make Xbox gameplay text genuinely sharp at 2×. The direct SDL_ttf HUD path still renders into a 640×480 Xbox video mode; XEMU's 2× scale enlarges those pixels. A point-sampling change removes a second texture-filtering step but the inspected score remains visibly soft. Validate a higher-resolution video mode with correctly mapped world, HUD, and font coordinates; retain the 60 Hz game clock and measure combat performance.
- [ ] Finish GPU rendering and visual parity, including saber glow, world geometry, HUD, and broader texture quality. Selected FED world and Obi-Wan textures now use 512 px BC1; most other staged opaque textures remain capped at 256 px. Rebased world UVs and duplicated BC1 payloads resolve the level-two Marsh black floor and visible tiling artifacts; its dark backdrop also appears in a headless PC reference. Later levels and final quality remain open.
- [ ] Verify level progression and longer-run stability through process-local virtual input and sequential native captures. Movement, jumping, and combat pass the level-one smoke. After publishing Xbox pad axes, level-two forward travel reaches the same position as a headless PC control run. The current 256 px BC1 build passes a StageExit reset and reaches frame 9,600 with 333 free memory pages and zero SFX allocation failures. Wider stability and level progression remain open.
- [ ] Verify specific sound effects and remaining audio behavior through native capture and gameplay review. Level-one music and a named Obi-Wan saber swing match native AC97 output; level-two Marsh ambient music also matches. The 9,599-frame soak recorded 765 SFX plays with zero allocation failures. Remaining effect identities and listening quality remain open.

Current boot, diagnostics, and unresolved visual evidence: [Xbox port](../xbox/README.md).

## 4. LIVE: Level 2 Pause-Menu Rendering

- [ ] Level 2 pause menu appeared white during a process-local harness run. Recheck in normal interactive play to determine whether this is a game rendering issue or specific to harness-driven capture.

Completed-work evidence: [native mod support](NATIVE_MOD_SUPPORT.md), [roster verification](MOD_ROSTER_REVIEW_20260913.md), and [live review](LIVE_REVIEW_20260913.md).
