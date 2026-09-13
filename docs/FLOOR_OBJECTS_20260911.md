# Hidden floor objects — September 11, 2026

## Identification and authority

Reproduced thin red/black floor protrusions matching the reported appearance. FED placement 157 at (12544,3328,-10752), AI 39, owns actor 7 (`pwrdrink.baf`, model ID 72). It is an active script owner with model flags 0x10 and scene flags 0. Nearby placements 36 and 159 use the same hidden model. The visible green pickup is a separate rendered object and must remain visible.

The shipped `render_RenderNode` checks model flags bit 0x10 before emitting geometry; see `decompiler-export/original/win32/nodes.c` and reconstructed `src/reconstructed/original/win32/nodes.c`. This is distinct from the scene-root 0x20 flag already supported. The portable BMD renderer lacked the model check and emitted eight triangles for each hidden placeholder. Placement 157 contributed 59 pixels in the diagnostic reproduction.

Added the missing model flag check to the shared software/hardware triangle path. The owner remains active and its AI is untouched. Player hierarchies continue traversing while geometry is hidden; hidden non-player owners return at the node as in the shipped routine. No actor IDs, placements, or pickup classes are blanket-filtered.

## Verification

- Release build and BMD tests pass. Regression checks hide an active owner's model and then clear the flag, verifying zero triangles followed by restored visible geometry.
- Native 960x540 captures at two affected locations show the slivers disappear. Hidden owners still have active AI state and emit zero triangles/pixels. Nearby visible actors continue rendering.
- Separate native capture at the green pickup confirms the pickup, player, and enemies remain visible.
- Evidence under `out/live-fixes-20260908`: `floor-place-21.png` / `floor-fixed-21.png`, `floor-place-39.png` / `floor-fixed-39.png`, `floor-fixed-36.png`, associated logs, `floor-flags.log`, and `floor-tests.log`.
- Reproduce the junction with `--quickload fed --hidden-window --mute --control-harness --frames 30 --framebuffer-size 960 540 --spawn-position 11687 3328 -11302 --enemy-placement-diagnostics --output capture.ppm`, using the staged executable/resource directory. Native capture only; no desktop input or capture.

The user's exact camera position is not known. The reproduced object class and visibility defect are confirmed; TODO #4 retains normal-play confirmation.

## Deployment

Deployed to `C:/Games/Star Wars Jedi Power Battles/jpb_pc_game.exe` with no game process running. Candidate and installed SHA-256 match: `951EB97DB2E37FBFC47D5D288727422201A1730C5B7BF9345C25E7D3C7D4AE7B`. Previous executable preserved as `out/live-fixes-20260908/before-floor-fix.exe`.
