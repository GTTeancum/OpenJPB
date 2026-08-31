# Overnight Smoke and Soak Report - 2026-08-29

## Result

- Broad non-interactive user-route coverage completed with no unresolved crash or hard failure.
- Long level soak: `65` scenarios, `34` pass / `31` warn / `0` fail.
- Clean optimized Release regression suite: `922/922` passed.
- Repeated lifecycle set: six startup, menu, versus, and gameplay-handoff routes passed ten consecutive runs each.
- The verified Release executable is staged at `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`; its SHA-256 is `492061D128866D9936D50A970123B32E344E74D5E370F458BB9124A96F07ACB5`. A game-directory FED startup without `--control-harness` completed with exit code `0`.

## Coverage

- Focused route suite: `12/12` passed, covering FED powerup contact, death/restart, New Game, Continue, movie resolution, D3D overwrite, one-player handoff, presentation New Game and Continue handoffs, versus flow, and two-player audio handoff.
- Long soak: all 23 playable level entries ran a 5,400-frame route plus 1,800-frame sampled placements where available. Every one of the 65 child processes completed its exact frame budget.
- Hardware level smoke: `20/23` passed at 900 frames. Extended 2,400-frame Core, Train5, and Train6 checks showed no crash; Train5 and Train6 recovered after death, while Core remained in its authored quickload scripted-control state.
- Hardware boss smoke: `7/7` exact forced placements passed at 900 frames.
- FMV smoke: all eight movies decoded and presented with nonzero audio; all eight explicit skip-edge runs recorded one skip and zero failures.

## Warning Assessment

The 31 long-soak warnings are route-quality telemetry, not hard failures:

- Recovered deaths with positive final energy.
- Known forced-coordinate visibility misses, including Streets and Train2 placement samples.
- Core's authored quickload control lock. Direct executable and PDB checks match the reconstructed `pFlags`, movement handling, and `256.0f` constant; the repository's existing level smoke already documents that the authored Core start does not expose stable player-directed locomotion.

No long-soak row crashed, timed out, ended at zero energy, or observed death without recovery.

## Failures Found and Resolved

The first clean Release suite exposed three stale test contracts:

- `jpb_enemy_tests` dereferenced the absent Steam achievement owner when its frame fixture deliberately fired achievement 34. The fixture now installs and removes an explicit achievement observer.
- `jpb_bmd_tests` marked its synthetic material handles as relocated without supplying the native texture pointer required by that contract. The fixture now owns a valid one-pixel texture and exercises its color override directly.
- `jpb_pc_enemy_radar` expected null textures even though `enemy_Radar` canonically submits `transHandle`. The validator now requires that exact loaded material.

All three pass in Debug and Release, followed by the clean `922/922` Release run.

## Evidence

- `out/overnight-20260828/LEVEL_SOAK_LONG.md`
- `out/overnight-20260828/level-soak-long/results.json`
- `out/overnight-20260828/LEVEL_HARDWARE_SMOKE.md`
- `out/overnight-20260828/EXTENDED_HARDWARE.md`
- `out/overnight-20260828/BOSS_HARDWARE_FINAL.md`
- `out/overnight-20260828/MOVIE_HARDWARE.md`
- `out/overnight-20260828/movie-skip/results.json`
- `out/overnight-20260828/CORE_RERUN.md`
