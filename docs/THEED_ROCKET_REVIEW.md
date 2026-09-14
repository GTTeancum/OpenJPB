# September 13: Water, Rocket Trail, and Blending

## Water

The native Theed terrace capture reproduced pale cyan water. The original `CD3DApplication::StartRender` (RVA `0x38820`) calls ClearRenderTargetView at `0x38B53` using the float4 literal at `0x334460`: `(0,0,0,1)`. The portable runtime instead cleared to `WorldData.bkColor`. That legacy drawing-surface color added a bright base beneath the procedural additive water. The runtime now clears the PC scene target to black. Water geometry, authored colors, materials, and UV animation are unchanged.

Native 960x540 captures at player position `(30208,2816,-25344)` are `out/theed-rocket-review/before.png` and `after.png`. The sampled water pixel `(30,400)` changed from approximately `(224,255,255)` to `(39,62,68)`. This is a diagnostic comparison, not a color chosen to match a video. Canonical evidence is the local executable and reconstructed PC renderer.

## Rocket

The installed rifle CAD's firing motions select projectile 17. Its `project.eff` record specifies muzzle effect 22, slug sprite 33 (`a_slug`), smoke effect 13 every two projectile updates, and impact effect 17. Effect 17 dispatches nested explosion 10 and a ring. The projectile setup and muzzle/impact dispatch were checked against `bullet_ShootProjectile` (RVA `0x23370`), `bullet_CallBack` (`0x227E0`), and `sprite_AddSpriteEffect` (`0xF9AA0`), with the installed effect records as data.

One concrete mismatch was found in trail dispatch: the port passed the scaled rocket direction as inherited smoke velocity. The executable still calculates that temporary vector, but explicitly zeroes R9 at `0x2304B` before calling `sprite_AddSpriteEffect` at `0x23058`. Passing NULL restores the authored smoke drift. No replacement effects or guessed sizes were added. The shared correction applies to AI and player projectiles.

`test_rocket_trail_drift` exercises an AI-owned projectile at rifle speed and asserts that the emitted smoke keeps its authored velocity. It failed on the previous implementation and passes after the correction. Native captures `before-trail-45.png` and `after-trail-45.png` show the trail restored behind the shot. Additional captures cover frames 35 and 65. Live combat confirmation remains open.

## Blending and verification

Movement/recovery defaults to 200 ms, with incoming damaging/attack motions capped at 100 ms. The pose regression checks the halfway pose at 100 ms and completion at 200 ms, plus unchanged event/root data and attack caps. Native control sequences cover stock Obi-Wan and the two-player mod pair (121/128).

Evidence, executable disassembly, native logs, regression output and deployment hashes are under `out/theed-rocket-review`. The full 97-test regression run passed 96 tests; the updated 200 ms test initially retained a 150 ms midpoint assertion. Correcting that assertion to the exact halfway angle made the remaining model-pose test pass. All 97 test suites therefore pass, with no production change after that run.
