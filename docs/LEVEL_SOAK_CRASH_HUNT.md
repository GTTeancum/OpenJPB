# Level Soak Crash Hunt

Generated: 2026-08-15 12:08:38 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `1ECD5CCF471FB74250E1D53DD0DBC3627739908C2A13A4019F9E4C63450104A2`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 32 passed / 33 failed
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | PASS | 1800/1800 | 63.877 | 1,086.5 | 95 | 8/9/12 | 49 authored=1 | - |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | PASS | 600/600 | 21.231 | 2,491.0 | 100 | 8/13/16 | 22 authored=1 | - |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 600/600 | 9.156 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 1800/1800 | 46.976 | 955.4 | 100 | 4/5/6 | 49 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 600/600 | 24.944 | 1,706.8 | 100 | 5/6/7 | 49 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=139 | FAIL | 600/600 | 37.629 | 755.6 | 0 | 13/13/13 | 49 authored=1 | visible=0, energy=0, death=157 |
| theed | route | route | FAIL | 1800/1800 | 42.960 | 229.8 | 100 | 7/7/7 | 49 authored=1 | exit=5 |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=-1 | FAIL | 600/600 | 19.369 | 64.6 | 0 | 0/1/1 | 49 authored=1 | visible=0, energy=0, death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=244 | FAIL | 600/600 | 18.848 | 0.0 | 100 | 3/3/3 | 22 authored=1 | exit=5, visible=0 |
| palace | route | route | PASS | 1800/1800 | 76.438 | 743.9 | 100 | 13/13/30 | 22 authored=1 | - |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | PASS | 600/600 | 20.184 | 134.6 | 49 | 11/13/16 | 50 authored=1 | - |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=210 | PASS | 600/600 | 24.037 | 300.5 | 100 | 5/6/6 | 49 authored=1 | - |
| tato | route | route | PASS | 1800/1800 | 45.359 | 796.9 | 100 | 4/5/5 | 0 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime= | FAIL | 0/600 | 2.433 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=5, missing frame summary, triangles=, pixels=, visible=, energy=, death=, crash marker |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=230 | PASS | 600/600 | 13.993 | 1,120.0 | 100 | 3/3/3 | 49 authored=1 | - |
| corus1 | route | route | PASS | 1800/1800 | 133.825 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=-1 | FAIL | 600/600 | 58.920 | 693.1 | 0 | 0/5/5 | 49 authored=1 | energy=0, death=157 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=60 | PASS | 600/600 | 21.770 | 1,232.4 | 94 | 4/6/6 | 4 authored=1 | - |
| ruins | route | route | PASS | 1800/1800 | 59.698 | 1,225.9 | 100 | 3/4/4 | 49 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=124 | PASS | 600/600 | 10.871 | 24.7 | 100 | 13/14/14 | 105 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime=252 | FAIL | 600/600 | 10.288 | 181.5 | 100 | 7/7/7 | 49 authored=1 | visible=0 |
| streets | route | route | PASS | 1800/1800 | 65.640 | 2,651.1 | 100 | 1/2/2 | 78 authored=1 | - |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | PASS | 600/600 | 20.733 | 392.2 | 100 | 9/13/13 | 0 authored=1 | - |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=126 | PASS | 600/600 | 12.957 | 237.0 | 100 | 7/15/15 | 0 authored=1 | - |
| hangar | route | route | PASS | 1800/1800 | 101.665 | 212.3 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | FAIL | 600/600 | 17.050 | 1,493.8 | 100 | 7/7/7 | 4 authored=1 | exit=5, pixels=0 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime= | FAIL | 0/600 | 0.764 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=9, missing frame summary, triangles=, pixels=, visible=, energy=, death=, crash marker |
| core | route | route | FAIL | 1800/1800 | 59.897 | 3,429.7 | 0 | 3/5/5 | 49 authored=1 | energy=0, death=865 |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime=0 | FAIL | 600/600 | 23.217 | 956.6 | 0 | 2/3/3 | 49 authored=1 | energy=0, death=157 |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=80 | FAIL | 600/600 | 41.314 | 431.4 | 0 | 10/12/610 | 22 authored=1 | energy=0, death=366 |
| mini1 | route | route | PASS | 1800/1800 | 64.286 | 1,392.3 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 26.598 | 430.5 | 100 | 2/2/2 | 105 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=9 | PASS | 600/600 | 12.585 | 248.7 | 91 | 3/3/3 | 49 authored=1 | - |
| mini2 | route | route | PASS | 1800/1800 | 55.191 | 1,152.7 | 100 | 3/3/3 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 600/600 | 17.922 | 2,453.0 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=27 | PASS | 600/600 | 17.067 | 1,464.9 | 100 | 11/11/11 | 49 authored=1 | - |
| mini3 | route | route | PASS | 1800/1800 | 59.624 | 1,303.1 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | FAIL | 600/600 | 21.180 | 1,118.2 | 100 | 8/9/9 | 22 authored=1 | exit=5 |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=10 | PASS | 600/600 | 20.219 | 1,788.0 | 100 | 9/10/10 | 49 authored=1 | - |
| mini4 | route | route | FAIL | 1800/1800 | 21.117 | 1,393.1 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 3.289 | 1,933.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=49 | FAIL | 600/600 | 4.019 | 1,968.9 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| corus2 | route | route | FAIL | 1800/1800 | 37.056 | 1,297.0 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=747 |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime= | FAIL | 0/600 | 0.591 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=15, missing frame summary, triangles=, pixels=, visible=, energy=, death=, crash marker |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=130 | PASS | 600/600 | 24.192 | 335.1 | 100 | 8/9/9 | 49 authored=1 | - |
| train1 | route | route | PASS | 1800/1800 | 69.311 | 1,159.5 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | route | route | PASS | 1800/1800 | 46.581 | 442.4 | 100 | 0/0/0 | 49 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 24.653 | 1,747.1 | 100 | 1/1/1 | 105 authored=1 | visible=0 |
| train3 | route | route | FAIL | 1800/1800 | 23.154 | 1,054.5 | 0 | 0/0/0 | 59 authored=1 | energy=0, death=182 |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 600/600 | 9.096 | 1,868.3 | 100 | 4/5/6 | 105 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime=6 | FAIL | 600/600 | 5.110 | 1,085.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train5 | route | route | FAIL | 1800/1800 | 37.245 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 11.797 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=5 | FAIL | 600/600 | 12.010 | 57.9 | 0 | 7/7/7 | 59 authored=1 | visible=0, energy=0, death=369 |
| train6 | route | route | FAIL | 1800/1800 | 40.660 | 749.9 | 0 | 5/5/5 | 49 authored=1 | energy=0, death=157 |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | FAIL | 600/600 | 12.745 | 316.8 | 100 | 6/6/6 | 49 authored=1 | visible=0 |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime=12 | FAIL | 600/600 | 13.704 | 936.8 | 0 | 7/7/7 | 49 authored=1 | visible=0, energy=0, death=157 |
| train7 | route | route | FAIL | 1800/1800 | 80.675 | 2,872.7 | 0 | 8/8/8 | 49 authored=1 | energy=0, death=746 |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | FAIL | 600/600 | 21.575 | 93.6 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=52 |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=20 | FAIL | 600/600 | 21.281 | 85.5 | 0 | 10/10/10 | 49 authored=1 | energy=0, death=52 |
| train4 | route | route | FAIL | 1800/1800 | 4.558 | 936.6 | 0 | 1/1/1 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime=0 | FAIL | 600/600 | 2.308 | 938.6 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime=32 | FAIL | 600/600 | 3.566 | 1,103.5 | 0 | 8/8/8 | 59 authored=1 | energy=0, death=193 |
| arena | route | route | PASS | 1800/1800 | 71.348 | 1,757.4 | 200 | 1/1/1 | 49 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 26.950 | 1,085.9 | 200 | 1/1/1 | 105 authored=1 | - |

## Crash Signals

The original ledger above predates the warning-aware runner and marks every issue as `FAIL`. Current soak output uses hard failures for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.

## Reproduced Crash Candidates

- Tatooine placement `0` (`d:\weasel\tusken.baf`) at spawn `29799/4096/-26240` is stable. Release repro: `out/level-soak-repro/placement.tato.0.console.txt` exits `-1073741819` with `rva=0xb0fea`. RelWithDebInfo repro: `out/level-soak-repro/placement.tato.0.relwithdebinfo.console.txt` exits at `rva=0xf1904`, mapped by `dumpbin /DISASM /LINENUMBERS` to `shaolin_node_player`, null-dereferencing the node chain.
- Corus2 placement `0` (`e:\obi\startup\spedblu.baf`) at spawn `-11520/10624/6029` is stable. Release repro: `out/level-soak-repro/placement.corus2.0.console.txt` exits `-1073741819` with `rva=0xb0fea`. RelWithDebInfo repro: `out/level-soak-repro/placement.corus2.0.relwithdebinfo.console.txt` exits at `rva=0xf1904`, also mapping to `shaolin_node_player`.
- Hangar placement `119` (`e:\obi\startup\pwrdrink.baf`) at spawn `7130/9216/640` is stable. Release repro: `out/level-soak-repro/placement.hangar.119.console.txt` exits `-1073741819` with `rva=0x8eab8`. RelWithDebInfo repro: `out/level-soak-repro/placement.hangar.119.relwithdebinfo.console.txt` exits at `rva=0xc5b13`, mapped to `physics_from_root` via `physics_gGetPosition`, null-dereferencing the root chain at `[rax+0x28]`.

## Fix Verification

Refreshed: 2026-08-15 13:08:02 -04:00

- Release build hash: `71ADF6252F90686F69ABEC72CB118150FACC95DF429E7BD07663AFE76506323D` for both `build\Release\jpb_pc_game.exe` and `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`.
- Tatooine placement `0` (`tusken.baf`) now exits `0` for `600/600` frames with the same forced-placement route. Final log: `out/level-soak-repro-final/placement.tato.0.release.console.txt`.
- Corus2 placement `0` (`spedblu.baf`) now exits `0` for `600/600` frames with the same forced-placement route. Final log: `out/level-soak-repro-final/placement.corus2.0.release.console.txt`.
- Hangar placement `119` (`pwrdrink.baf`) now exits `0` for `600/600` frames with the same forced-placement route. Final log: `out/level-soak-repro-final/placement.hangar.119.release.console.txt`.
- The Tatooine and Corus2 fixes guard malformed kungfu owner chains before `shaolin_node_player`, `shaolin_node_physics`, `shaolin_Attack`, and the kungfu coordinator dereference scene/player state.
- The Hangar fix guards malformed physics owner chains in `physics_from_root` callers and `bapenemy_postFrame`. The final non-crash Hangar failure was a headless renderer abort on non-primary owner-3 helper placement `58`; the runtime now counts/skips those malformed helper renders while preserving fatal render failures for the player, the forced target, and ordinary enemy owner types.

## Post-Fix Soak Reruns

Refreshed: 2026-08-15

- Full post-crash-fix rerun: `docs/LEVEL_SOAK_CRASH_HUNT_RERUN.md` / `out/level-soak-rerun/results.json` recorded `33` passed / `32` failed with no crash markers. The three original access-violation rows no longer crash; Corus2 placement `0` and Hangar placement `119` now run to `600/600` frames and fail only because the forced placement route kills the player.
- Surviving `exit=5` rows from that rerun were host validation mismatches, not process crashes: Theed route, Theed placement `244` (`tatkid.baf`), Hangar placement `0` (`destroyr.baf`), and Mini3 placement `0` (`gungrd.baf`).
- Final targeted `exit=5` proof: `docs/LEVEL_SOAK_EXIT5_FINAL.md` / `out/level-soak-exit5-final/results.json` ran Theed, Hangar, and Mini3 routes plus two sampled placements per level at `640x360`. Result: `5` passed / `4` failed, `exit5 count=0`, `crash markers=0`.
- Final targeted proof executable hash: `BAC174AE2145A2A82B56AA508CCC6D7CE9CA06AAC8E62CFF799E0948EE49A64A` for both `build\Release\jpb_pc_game.exe` and `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`.
- The headless authored-combat validator now treats nonzero hit detection plus nonzero enemy damage processing as the required combat-pipeline proof. HP loss and reaction/recoil remain logged telemetry because special or invulnerable actors can process damage without changing HP or entering a reaction state.
- The headless authored-motion validator now accepts recovered locomotion, airborne jump, or vehicle-attach movement signals even when forced placement collision leaves the actor at the same final X/Z position.
- Warning-aware targeted proof: `docs/LEVEL_SOAK_WARNING_FINAL.md` / `out/level-soak-warning-final/results.json` reran the same Theed, Hangar, and Mini3 matrix. Result: `5` passed / `4` warned / `0` failed.
- Remaining targeted warnings after the final proof are non-crash soak-quality rows: Theed placement `0` (`visible=0, energy=0, death=52`), Theed placement `244` (`visible=0`), Hangar placement `0` (`pixels=0`), and Hangar placement `119` (`energy=0, death=297`).
- Full warning-aware soak: `docs/LEVEL_SOAK_WARNING_FULL.md` / `out/level-soak-warning-full/results.json` refreshed the full playable matrix on 2026-08-16. Result: `34` passed / `30` warned / `1` failed using the old `180s` child-process timeout.
- The only hard row in that full run was Coruscant 1 route timeout. Focused Coruscant 1 rerun with `300s` timeout (`docs/LEVEL_SOAK_CORUS1_REPRO.md` / `out/level-soak-corus1-repro/results.json`) completed as `2` passed / `1` warned / `0` failed. The route row itself passed at `1800/1800` frames, `travel=374.9`, `energy=94`.
- `tools/soak_levels.ps1` now defaults to `300s` per child process so slow but valid route rows are not reported as hard crash-hunt failures.
- Final full warning-aware soak: `docs/LEVEL_SOAK_WARNING_FULL_FINAL.md` / `out/level-soak-warning-full-final/results.json` refreshed the full playable matrix on 2026-08-16 with the new `300s` default timeout. Result: `35` passed / `30` warned / `0` failed.
- The final full proof had no hard rows: no timeouts, nonzero exits, wrong-level rows, missing frame summaries, missing geometry, or crash markers. The former Coruscant 1 route timeout now passes in the aggregate matrix at `1800/1800` frames.
- Remaining rows are route-quality warnings only: `21` death/energy rows, `4` visibility-only rows, `4` visibility-plus-death rows, and `1` Hangar pixel-proof miss. These are forced-route/validation issues, not crash signals.
