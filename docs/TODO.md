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

- [ ] Hardware phase: confirm saves survive a power-off on the console. In XEMU (`boot_xemu.ps1 -PersistentHdd`) a save followed by killing the emulator loads cleanly on the next boot; the console's own disk cache is the remaining unknown.
- [ ] Bosses (PC and Xbox, shared logic): all eight spawn, fight and fall when their hit points are forced to zero (`xbox/tools/boss_smoke.py --finish`). No forced defeat sets the level-finish flag (`abGlobalBits[0]` bit 1, set by level scripts) within 1,200 frames. Whether a real kill, or walking on to an exit afterwards, ends each level needs a playthrough.
- [ ] Xbox level play-throughs: all 15 levels load and play, but each has only been spot-checked, not played start to finish.
- [ ] Live review on hardware or XEMU with a physical pad: menu flow, two-player co-op, saves after power-off, movie audio.
- [ ] Xbox texture sharpness: a PC/XEMU side-by-side of all 15 level starts matches apart from softer textures (most staged opaque textures are capped at 128-256 px; selected FED and Obi-Wan textures use 512 px BC1).
- [ ] Verify longer-run stability through real play. Forced completion chains all 14 story levels in one XEMU session without a crash (`xbox/tools/chain_levels.py`); free memory stays at 4 MB or more (Palace lowest). Long continuous play within a level remains open.
- [ ] Verify specific sound effects and remaining audio behavior through native capture and gameplay review. Level-one music and a named Obi-Wan saber swing match native AC97 output; level-two Marsh ambient music also matches. The 9,599-frame soak recorded 765 SFX plays with zero allocation failures. Remaining effect identities and listening quality remain open.

- [ ] Hardware phase: verify the in-game reset (LT+RT+Back+Black, `xbox/src/controller.c`) reaches the dashboard. In XEMU the combo stops the game and enters the kernel's quick reboot; the test disk has no dashboard to land on.
- [ ] Hardware phase: verify the black band guard at 480i. Both halves are in: `guard_shown_buffer` and the local pbkit core (`xbox/src/pbkit_core.c`) that retires a frame only once scanout shows it. `E:\OpenJPB-flicker.txt` logs held frames and pbkit deferrals.
- [ ] PC direct level start with enemies (`OpenJPB.exe mini2.jpx ... --enemy-cad`, windowed): the Kaadu rider's camera follows a point near the map origin, so the frame is black. Menu play and the headless Mini2 camera tests are unaffected. That path skips the canonical constructor the menu handoff runs; adding it there crashes because game_initPerLevel opens the objective menu, whose controller icons (newDrawControllerIcon) are never loaded on that path. Without it the rider's camera uses type 5, whose target is the rider's mount-relative physics position (cube.c/camera.c); the menu path ends up on type 6 (gJarJarPos).

Current boot, diagnostics, and unresolved visual evidence: [Xbox port](../xbox/README.md).

Completed-work evidence: [native mod support](NATIVE_MOD_SUPPORT.md), [roster verification](MOD_ROSTER_REVIEW_20260913.md), and [live review](LIVE_REVIEW_20260913.md).
