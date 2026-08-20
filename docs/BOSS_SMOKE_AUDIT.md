# Boss Smoke Audit

Generated: 2026-08-15 04:15:24 -04:00

Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`

Mode: `Headless`; frames: `180`; framebuffer: `960x540`; result: `7 passed / 0 failed`.

Contact sheet: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\contact-sheet.png`.

Contact sheet manifest: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\contact-sheet.manifest.json`.

Contact sheet entries follow the smoke matrix order, and contact-sheet generation is required unless `-SkipProofImages` is passed. Capture bytes record the raw frame generated during the smoke run; by default the script prunes raw `.ppm` and per-capture `.png` files after the contact sheet is generated. Pass `-KeepRawCaptures` to retain them.

| Boss | Level | Status | Placement | Spawn | Runtime proof | Capture |
| --- | --- | --- | --- | --- | --- | --- |
| FED droid fighter | fed | PASS | id=128, actor=5 `drdfitr.baf`, ai=16, confirmed actor | -22656/5376/-9856 | frames=180, energy=100, visible=180, enemyActors=6/10/11, placementStatus=1, runtimePlacement=128 | `fed-droid-fighter.ppm` (1555215 bytes, pruned after contact sheet) |
| Marsh MTT | marsh | PASS | id=78, actor=8 `mtt.baf`, ai=3, confirmed actor | 21287/4416/-12416 | frames=180, energy=100, visible=180, enemyActors=4/5/5, placementStatus=1, runtimePlacement=78 | `marsh-mtt.ppm` (1555215 bytes, pruned after contact sheet) |
| Theed tank | theed | PASS | id=53, actor=1 `tank.baf`, ai=34, confirmed actor | -3200/3328/-20007 | frames=180, energy=100, visible=139, enemyActors=3/3/3, placementStatus=1, runtimePlacement=53 | `theed-tank.ppm` (1555215 bytes, pruned after contact sheet) |
| Tatooine Darth Maul | tato | PASS | id=92, actor=10 `sithjedi.baf`, ai=45, confirmed actor | -20467/9984/12173 | frames=180, energy=100, visible=180, enemyActors=6/6/6, placementStatus=1, runtimePlacement=92 | `tato-maul.ppm` (1555215 bytes, pruned after contact sheet) |
| Coruscant thug | corus1 | PASS | id=153, actor=12 `corhum4.baf`, ai=15, confirmed retail boss stream/actor | -29952/10342/-18432 | frames=180, energy=100, visible=180, enemyActors=6/6/6, placementStatus=1, runtimePlacement=153 | `corus1-thug.ppm` (1555215 bytes, pruned after contact sheet) |
| Mini2 Kadu | mini2 | PASS | id=0, actor=6 `horns.baf`, ai=0, confirmed actor/camera | 21504/4608/24320 | frames=180, energy=100, visible=180, enemyActors=2/2/2, placementStatus=1, runtimePlacement=0 | `mini2-kadu.ppm` (1555215 bytes, pruned after contact sheet) |
| Mini3 Boss Nass | mini3 | PASS | id=4, actor=1 `bossnass.baf`, ai=4, confirmed actor | 12800/13824/-14093 | frames=180, energy=100, visible=180, enemyActors=8/9/9, placementStatus=1, runtimePlacement=4 | `mini3-boss-nass.ppm` (1555215 bytes, pruned after contact sheet) |

## Notes

- FED droid fighter: High-hp authored droid-fighter placement near the FED boss arena; spawn uses the linked trigger cluster so the player stays alive and visible. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\fed-droid-fighter.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\fed-droid-fighter.ppm`; file pruned after contact-sheet generation.
- Marsh MTT: MTT actor with authored path down the Marsh boss corridor; spawn uses a later path point where the authored camera keeps the player visible. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\marsh-mtt.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\marsh-mtt.ppm`; file pruned after contact-sheet generation.
- Theed tank: High-hp tank placement tied to the Theed vehicle encounter cluster; spawn uses a grounded authored waypoint from the tank path. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\theed-tank.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\theed-tank.ppm`; file pruned after contact-sheet generation.
- Tatooine Darth Maul: Explicit Sith Jedi actor with 200 HP in the Tatooine arena. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\tato-maul.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\tato-maul.ppm`; file pruned after contact-sheet generation.
- Coruscant thug: Late Coruscant high-hp human placement matching the thug-boss asset family; retail assets include 06_CorThugBoss.wav and diagnostics place id 153 in the authored camera-director enemy set with mode 5. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\corus1-thug.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\corus1-thug.ppm`; file pruned after contact-sheet generation.
- Mini2 Kadu: Bonus Kadu encounter starts on active horns.baf placements in the authored camera-director enemy set; placement 0 reports 255 HP at the quickload spawn. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\mini2-kadu.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\mini2-kadu.ppm`; file pruned after contact-sheet generation.
- Mini3 Boss Nass: Explicit Boss Nass actor at the Gungan bonus encounter. Console: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\mini3-boss-nass.console.txt`. Raw frame path during run: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\mini3-boss-nass.ppm`; file pruned after contact-sheet generation.

Raw JSON: `C:\Programming\GitHub\Jedi Power Battles recomp\out\boss-smoke\results.json`.
