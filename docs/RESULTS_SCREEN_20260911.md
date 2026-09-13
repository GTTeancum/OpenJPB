# Results screen verification — September 11, 2026

## Canonical evidence and changes

Authority: the locally shipped `C:/Games/Star Wars Jedi Power Battles/game.exe`, recovered PDB declarations/data, `decompiler-export/original/menu.c`, and the installed menu assets. User screenshots establish the reported defect, not the intended layout.

- The results routine at RVA 0xC5800 reads paired header coordinates from 0x4C8D48. Corrected the pair stride; initial header Y values are 110, 85, and 60 rather than 81, 282, and 60.
- The same routine clears the animated score counter at 0x987E64 when initializing each player's results. Restored that reset, including the transition to player two.
- The hardware title renderer previously submitted all textured panels before all text. It now interleaves texture batches and text in depth order, preserving foreground panels' coverage of older award labels. Equal-depth panel/text ownership and render-hook failure are regression tested.
- Retained the preceding repairs to full menu font/texture banks, portraits and score digits, combo text pivot, and post-award continuation. Award rendering and formatting were checked against routines at RVAs 0xBF020, 0xBFC90, and 0xC2290.

The completion fixture is opt-in (`--review-level-complete` or `--review-level-complete-two-player`, with `--review-score`). It starts at the ordinary completion owner and logs menu/player/award transitions. It does not simulate the boss fight or send operating-system input.

## Verification

- Release `jpb_menu_tests` and `jpb_game_runtime_title_tests`: pass. New assertions cover header coordinates, player-two score reset, interleaved hardware/text composition, and failure propagation.
- Native hardware captures at 960x540: inspected all three award stages separately for both players. Files `results-two-{100,220,340,460,580,700}.png` show combo, energy, and bonus stages in order.
- Native 1920x1080 capture: inspected; average 59.67 FPS over 340 frames, with average render time 10.094 ms.
- One-player score 40000 and score 100 runs: successful exit, final game state level=2, mode=6, players=1.
- Two-player score 40000 and score 100 runs: successful exit, final game state level=2, mode=6, players=2. Award runs exercise all three stages for each player; low-score runs exercise no-award continuation.
- Simulated XInput confirmation also completes both players' awards and reaches level two. Hidden-window capture uses actual controller discovery for glyph selection, so those captures show keyboard glyphs when no controller is discovered; this is not physical-controller visual validation.

Evidence is under `out/live-fixes-20260908/`: `results-one-all.log`, `results-one-none.log`, `results-two-small.log`, `results-two-none.log`, `results-controller-two-flow.log`, `results-controller-1080.png/log`, and the six two-player captures. `results_final.py` records the five final smoke commands; `results_captures.py` records the six-stage capture commands. Final test output is in `jpb_menu_tests-final.log` and `jpb_game_runtime_title_tests-final.log`.

Live confirmation after a normal boss completion remains open in TODO #1/#2. The fixture establishes the results-to-level-two handoff, not the full boss-run-off sequence.

## Deployment

Deployed the tested candidate to `C:/Games/Star Wars Jedi Power Battles/jpb_pc_game.exe` on September 11, with no game process running. Candidate and installed SHA-256 match: `B71AFB470F685B86621F6C02FE15C98AD1C805FF624201E106BEB8EC2EE2015B`. Preserved the previous installed executable as `out/live-fixes-20260908/before-results-fix.exe`.
