# TODO

## Tracked Items

1. Shared DLC implementation and Blender authoring.
2. Original Xbox port (nxdk/XEMU).
4. LIVE: Level 2 pause-menu rendering.

## 1. Shared DLC Implementation and Blender Authoring

Implement the completed [shared package design](DLC_PACKAGE_DESIGN.md). Keep existing version-1 mods and legacy assets compatible.

- [ ] Implement version-2 manifests, dependencies, stable export references, scoped resource bindings, shared animation libraries and save migration.
- [ ] Extend the existing Blender BMD/CAD tools into the shared package pipeline: rig/profile validation, animation and gameplay-event mapping, materials, and package build/export validation. Existing BMD/CAD import/export is already available; see [character tools](BLENDER_CHARACTER_TOOLS.md).
- [ ] Validate the shared pipeline with an updated animated character. Document remaining limitations.

## 2. Original Xbox Port (nxdk/XEMU)

Maintain both PC and Xbox builds with shared gameplay and Xbox-only code under `xbox/`. Use nxdk, a separate copy of game assets, and XEMU at 2× resolution. Crowded-wave phase timing found about 3.9 ms of vertex transforms and 11.3 ms of face processing per frame; tested node-frustum rejection and inline transforms did not raise tick-aligned FPS. The next performance change must tackle face processing or GPU model submission at a broader level.

- [x] Establish an acceptable 480p Xbox performance baseline at 2× XEMU resolution while keeping Windows `OpenJPB.exe` at its 60 Hz clock. The Xbox 30 FPS cap is removed; gameplay scales by measured wall time and its frame counters retain fractional 60 Hz ticks. Five prominent FED/Obi-Wan BC1 textures use 512 px sources. HUD texture cards and TrueType gameplay text draw at the active output viewport, with placement derived from the logical canvas and video mode. The true 16:9 gameplay pass runs at anamorphic 720×480. After reverting the model-array experiment that dropped polygons, a native FED combat capture showed the doorway, Obi-Wan, droids, and HUD intact. A 512-frame ring measured 23 ms median (about 43 FPS), 28 ms at the 95th percentile (about 36 FPS), and 38 ms worst; the run recorded 150 SFX plays, zero allocation failures, and 392 free pages. The user accepted approximately 30–40 FPS at 480p provided visual glitches are resolved. Broader level and physical-pad validation remain open below.
- [x] Complete game loop on Xbox (2026-10-24): menus at 60 FPS on the GPU, saves on the hard disk, no license screen, quit/results/next level, two players, movie audio at the game's movie volume, reduced menu sheets mapped correctly. See [Xbox port](../xbox/README.md).
- [x] Every story level loads and plays on Xbox (2026-10-24): XLV geometry for all FBX levels, Mini2 from its JPX scene, Kaadu mount visible (shared fix), Corus1 → Corus2 hand-off (shared fix), Palace plant alpha edges fixed, Hangar font thrash fixed, free memory no longer stranded after several levels.
- [x] PC mod characters on Xbox (2026-10-24): shared loader through a per-platform file layer, `stage_mods.py` conversion with a format stamp, 14 packages selectable and playable in XEMU.
- [ ] Xbox saves across a console reset: XEMU's `-snapshot` test setup loses them with old and new builds alike; verify on hardware.
- [x] Xbox texture garbage bands/lines (FED floor, Core floor, Gungan arena): texture format border source; also black colour key on models/effects (power-up glows). See [Xbox port](../xbox/README.md).
- [ ] Xbox 2P: character select overlaps player 2's "Lightsaber Color" prompt with "Select"; compare with retail before moving it. Versus checked (match end fixed on PC and Xbox); two-mod-character versus is 36 ms per frame vs 16 ms stock.
- [ ] Xbox bosses: all eight spawn, render and fight (`boss_smoke.py`). With energy forced to zero, six fall (Tatooine and Core Maul award their points); the Theed tank ignores the write and the Coruscant thug stops at 10. Whether each defeat then ends the level as in retail needs a real playthrough of those fights.
- [ ] Xbox two mod characters together: 23 ms per frame in versus against 16 ms stock (perf ring average over 300 frames). The difference is model drawing, 14.5 ms against 3.9 ms; mod BMDs are 2-3x the size of stock ones (Shaak Ti 139 KB, Temple Guard 111 KB, Obi-Wan 48 KB). Needs faster model submission (the vertex-array path, `xbox-model-vertex-array.txt`, previously dropped polygons).
- [ ] Xbox memory: about 60 KB of free memory is lost per level-to-menu round trip.
- [ ] Xbox Palace performance: 41 ms per frame (~24 fps), from 66-86 ms. Remaining cost is effects (~13 ms: per-triangle texture/state switching and the shared screen-poly code) and models (~10 ms). Other levels now run at 16-21 ms (~50-60 fps) in XEMU.
- [ ] Xbox Kaadu race (Mini2) looks very dark; compare with PC (same before and after the speed work).
- [x] Training levels: enemies do not work. Both hosts skipped loading enemy classes for training starts (game mode 2); training levels 3-7 place droids, Gonks and droid fighters. Fixed on PC and Xbox; Training Six on Xbox shows 20 enemies and a droid fighter attacking.
- [ ] Xbox level play-throughs: all 15 levels load and play, but each has only been spot-checked, not played start to finish.
- [x] Xbox Corus2 grey surface: matches PC from the same start view after the texture border-source fix (the grey floor beside the pool is plain on PC too).
- [ ] Live review on hardware or XEMU with a physical pad: menu flow, two-player co-op, saves after power-off, movie audio.
- [ ] Complete Xbox boot/menu/FMVs with the shared UI. Current display contract: all content fills 16:9 output; standard 4:3 480i displays the complete 16:9 composition with top/bottom bars, never cropping. `boot_xemu.ps1` must set both the isolated EEPROM and XEMU display aspect for each test. FED gameplay and the shared menus have been visually checked in both modes. Movie corruption is fixed by validated all-intra Theora staging; native audio buffering and the full menu-to-gameplay handoff are still under verification. Keep 720p separate from this 480-line work.
- [ ] Make Xbox gameplay text genuinely sharp at 2×. The direct SDL_ttf HUD path still renders into a 640×480 Xbox video mode; XEMU's 2× scale enlarges those pixels. A point-sampling change removes a second texture-filtering step but the inspected score remains visibly soft. Validate a higher-resolution video mode with correctly mapped world, HUD, and font coordinates; retain the 60 Hz game clock and measure combat performance.
- [ ] Finish GPU rendering and visual parity, including saber glow, world geometry, HUD, and broader texture quality. Selected FED world and Obi-Wan textures now use 512 px BC1; most other staged opaque textures remain capped at 256 px. Rebased world UVs and duplicated BC1 payloads resolve the level-two Marsh black floor and visible tiling artifacts; its dark backdrop also appears in a headless PC reference. Later levels and final quality remain open.
- [ ] Verify level progression and longer-run stability through process-local virtual input and sequential native captures. Movement, jumping, and combat pass the level-one smoke. After publishing Xbox pad axes, level-two forward travel reaches the same position as a headless PC control run. The current 256 px BC1 build passes a StageExit reset and reaches frame 9,600 with 333 free memory pages and zero SFX allocation failures. Wider stability and level progression remain open.
- [ ] Verify specific sound effects and remaining audio behavior through native capture and gameplay review. Level-one music and a named Obi-Wan saber swing match native AC97 output; level-two Marsh ambient music also matches. The 9,599-frame soak recorded 765 SFX plays with zero allocation failures. Remaining effect identities and listening quality remain open.

Current boot, diagnostics, and unresolved visual evidence: [Xbox port](../xbox/README.md).

## 3. LIVE: Level 2 Pause-Menu Rendering

- [ ] Level 2 pause menu appeared white during a process-local harness run. Recheck in normal interactive play to determine whether this is a game rendering issue or specific to harness-driven capture.

Completed-work evidence: [native mod support](NATIVE_MOD_SUPPORT.md), [roster verification](MOD_ROSTER_REVIEW_20260913.md), and [live review](LIVE_REVIEW_20260913.md).
