# Boss Smoke Audit

Generated: 2026-08-31 03:53:18 -04:00

Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`

Mode: `Headless`; frames: `600`; framebuffer: `640x360`; result: `7 passed / 0 failed`.

| Boss | Level | Status | Placement | Spawn | Runtime proof | Capture |
| --- | --- | --- | --- | --- | --- | --- |
| FED droid fighter | fed | PASS | id=128, actor=5 `drdfitr.baf`, ai=16, confirmed actor | -22656/5376/-9856 | frames=600, energy=100, visible=600, enemyActors=12/12/15, placementStatus=1, runtimePlacement=0 | `fed-droid-fighter.ppm` (691215 bytes) |
| Marsh MTT | marsh | PASS | id=78, actor=8 `mtt.baf`, ai=3, confirmed actor | 21287/4416/-12416 | frames=600, energy=100, visible=516, enemyActors=3/5/6, placementStatus=1, runtimePlacement=0 | `marsh-mtt.ppm` (691215 bytes) |
| Theed tank | theed | PASS | id=53, actor=1 `tank.baf`, ai=34, confirmed actor | -3200/3328/-20007 | frames=600, energy=100, visible=541, enemyActors=6/6/6, placementStatus=1, runtimePlacement=0 | `theed-tank.ppm` (691215 bytes) |
| Tatooine Darth Maul | tato | PASS | id=92, actor=10 `sithjedi.baf`, ai=45, confirmed actor | -20467/9984/12173 | frames=600, energy=100, visible=595, enemyActors=6/6/6, placementStatus=1, runtimePlacement=0 | `tato-maul.ppm` (691215 bytes) |
| Coruscant thug | corus1 | PASS | id=153, actor=12 `corhum4.baf`, ai=15, confirmed retail boss stream/actor | -29952/10342/-18432 | frames=600, energy=100, visible=533, enemyActors=3/6/9, placementStatus=0, runtimePlacement=0 | `corus1-thug.ppm` (691215 bytes) |
| Mini2 Kadu | mini2 | PASS | id=0, actor=6 `horns.baf`, ai=0, confirmed actor/camera | 21504/4608/24320 | frames=600, energy=100, visible=600, enemyActors=2/2/2, placementStatus=1, runtimePlacement=0 | `mini2-kadu.ppm` (691215 bytes) |
| Mini3 Boss Nass | mini3 | PASS | id=4, actor=1 `bossnass.baf`, ai=4, confirmed actor | 12800/13824/-14093 | frames=600, energy=100, visible=600, enemyActors=10/10/11, placementStatus=1, runtimePlacement=0 | `mini3-boss-nass.ppm` (691215 bytes) |

## Notes

- FED droid fighter: High-hp authored droid-fighter placement near the FED boss arena; spawn uses the linked trigger cluster so the player stays alive and visible. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\fed-droid-fighter.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\fed-droid-fighter.ppm`.
- Marsh MTT: MTT actor with authored path down the Marsh boss corridor; spawn uses a later path point where the authored camera keeps the player visible. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\marsh-mtt.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\marsh-mtt.ppm`.
- Theed tank: High-hp tank placement tied to the Theed vehicle encounter cluster; spawn uses a grounded authored waypoint from the tank path. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\theed-tank.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\theed-tank.ppm`.
- Tatooine Darth Maul: Explicit Sith Jedi actor with 200 HP in the Tatooine arena. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\tato-maul.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\tato-maul.ppm`.
- Coruscant thug: Late Coruscant high-hp human placement matching the thug-boss asset family; retail assets include 06_CorThugBoss.wav and diagnostics place id 153 in the authored camera-director enemy set with mode 5. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\corus1-thug.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\corus1-thug.ppm`.
- Mini2 Kadu: Bonus Kadu encounter starts on active horns.baf placements in the authored camera-director enemy set; placement 0 reports 255 HP at the quickload spawn. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\mini2-kadu.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\mini2-kadu.ppm`.
- Mini3 Boss Nass: Explicit Boss Nass actor at the Gungan bonus encounter. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\mini3-boss-nass.console.txt`. Frame: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\mini3-boss-nass.ppm`.

Raw JSON: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-boss-smoke-fixed-20260831\results.json`.
