# TODO

## Tracked Items

1. Shared DLC implementation and Blender authoring.
2. Original Xbox port (nxdk/XEMU).

## 1. Shared DLC Implementation and Blender Authoring

Implement the completed [shared package design](DLC_PACKAGE_DESIGN.md). Keep existing version-1 mods and legacy assets compatible.

- [ ] Implement version-2 manifests, dependencies, stable export references, scoped resource bindings, shared animation libraries and save migration.
- [ ] Extend the existing Blender BMD/CAD tools into the shared package pipeline: rig/profile validation, animation and gameplay-event mapping, materials, and package build/export validation. Existing BMD/CAD import/export is already available; see [character tools](BLENDER_CHARACTER_TOOLS.md).
- [ ] Validate the shared pipeline with an updated animated character. Document remaining limitations.

## 2. Original Xbox Port (nxdk/XEMU)

Maintain both PC and Xbox builds with shared gameplay and Xbox-only code under `xbox/`. Use nxdk, a separate copy of game assets, and XEMU at 2× resolution. Crowded-wave phase timing found about 3.9 ms of vertex transforms and 11.3 ms of face processing per frame; tested node-frustum rejection and inline transforms did not raise tick-aligned FPS. The next performance change must tackle face processing or GPU model submission at a broader level.

- [ ] Xbox saves across a console reset: XEMU's `-snapshot` test setup loses them with old and new builds alike; verify on hardware.
- [ ] Bosses (PC and Xbox, shared logic): all eight spawn, render and fight (`xbox/tools/boss_smoke.py`). With energy forced to zero six fall and award points; the Theed tank ignores the write and the Coruscant thug stops at 10. No forced defeat sets the level-finish flag (`abGlobalBits[0]` bit 1, set by level scripts) within 1,200 frames, so the level continues. Whether a real kill, or walking on to an exit afterwards, ends each level needs a playthrough.
- [ ] Xbox level play-throughs: all 15 levels load and play, but each has only been spot-checked, not played start to finish.
- [ ] Live review on hardware or XEMU with a physical pad: menu flow, two-player co-op, saves after power-off, movie audio.
- [ ] Complete Xbox boot/menu/FMVs with the shared UI. Current display contract: all content fills 16:9 output; standard 4:3 480i displays the complete 16:9 composition with top/bottom bars, never cropping. `boot_xemu.ps1` must set both the isolated EEPROM and XEMU display aspect for each test. FED gameplay and the shared menus have been visually checked in both modes. Movie corruption is fixed by validated all-intra Theora staging; native audio buffering and the full menu-to-gameplay handoff are still under verification. Keep 720p separate from this 480-line work.
- [ ] Finish GPU rendering and visual parity, including saber glow, world geometry, HUD, and broader texture quality. Selected FED world and Obi-Wan textures now use 512 px BC1; most other staged opaque textures remain capped at 256 px. Rebased world UVs and duplicated BC1 payloads resolve the level-two Marsh black floor and visible tiling artifacts; its dark backdrop also appears in a headless PC reference. Later levels and final quality remain open.
- [ ] Verify longer-run stability through real play. Forced completion chains all 14 story levels in one XEMU session without a crash (`xbox/tools/chain_levels.py`); free memory stays at 4 MB or more (Palace lowest). Long continuous play within a level remains open.
- [ ] Verify specific sound effects and remaining audio behavior through native capture and gameplay review. Level-one music and a named Obi-Wan saber swing match native AC97 output; level-two Marsh ambient music also matches. The 9,599-frame soak recorded 765 SFX plays with zero allocation failures. Remaining effect identities and listening quality remain open.

- [ ] Hardware phase: in-game reset to the dashboard with LT+RT+Back+Black on any controller, as in Perfect-Dark-X (`port/src/input.c`, NKPatcher-style IGR). Read the physical buttons below the game's input mapping so it works in every state; latch per pad; stop audio and rumble; write an `LDT_LAUNCH_DASHBOARD` launch page, persist it, flush caches (`sfence`, `wbinvd`; XEMU hides the need) and `HalReturnToFirmware(HalQuickRebootRoutine)`, falling back to a full reboot if the page allocation fails.
- [ ] Hardware phase: guard against the moving black band seen on hardware in Perfect-Dark-X (issue 3, 480i). This port already refuses to draw into the scanout buffer (`guard_shown_buffer`). Add the other half from Perfect-Dark-X `port/src/xbox/pbkit_core.c` and `PBKIT_PATCH.md`: a local copy of nxdk's pbkit core whose VBlank DPC retires a ready frame only when `PCRTC_START` already shows it, so a completed frame is never consumed unshown. Verify on hardware at 480i, ideally with a presentation log like Perfect-Dark-X v1.1.1.

Current boot, diagnostics, and unresolved visual evidence: [Xbox port](../xbox/README.md).

Completed-work evidence: [native mod support](NATIVE_MOD_SUPPORT.md), [roster verification](MOD_ROSTER_REVIEW_20260913.md), and [live review](LIVE_REVIEW_20260913.md).
