# Level Soak Crash Hunt

Generated: 2026-08-27 21:07:21 -04:00

- Executable: `C:\Programming\GitHub\Jedi Power Battles recomp\build\Release\jpb_pc_game.exe`
- SHA-256: `A274A7036E9B33F1A9A62E930316AF2AC01A492DBA84C2CE095A92CCC8C74A16`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 36 passed / 3 warned / 26 failed
- Warning summary: 1 death/energy, 2 visibility-only
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-20260827-current\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

## Post-Fix Resolution

- A freshly rebuilt matching RelWithDebInfo PDB resolves the shared access violation to `CSteamAchievements::SetAchievement + 0x1f`, with `this == NULL`. The repeated FontAtlas diagnostics precede the crash but are not its cause.
- The explicit `--control-harness` path now installs a stateful platform achievement adapter. Ordinary runs with no adapter retain the canonical direct Steam-owner call and receive no null fallback.
- All levels that contributed one of the 26 original failures were rerun at the same `1800` route / `600` placement thresholds: `15` pass / `27` warn / `0` fail across 42 rows. Evidence: `out/level-soak-20260827-fix-failed-levels`.
- The rebuilt optimized Release executable additionally completed FED, Mini4, and Train4 routes plus placements: `2` pass / `7` warn / `0` fail across nine rows. These cover the late, mid-run, and immediate variants of the former signature. Evidence: `out/level-soak-20260827-fix-release-triad`.
- Focused `jpb_achievement_tests` and `jpb_whook_input_tests` pass in Debug and Release.

### Warning Assessment

- None of the post-fix warnings demonstrates a gameplay failure. The death observer is intentionally sticky: `25` warned rows finish with positive energy after recovery.
- Train3 placement `6` reaches the training player-exit path and clears the forced placement. Its final zero energy is a terminal-state/harness mismatch, not a stalled process.
- Train5 placement `5` ends the 600-frame sample eight frames before the recovered afterlife threshold. An otherwise identical 750-frame run records afterlife at frame `610`, player exit/game-death at `670`, and final energy `100`.
- Train2 placement `0` projects the player on-screen but reports zero rendered player pixels. A retained current-binary frame proves the forced pickup-coordinate teleport places the player fully behind foreground geometry, so depth occlusion is correct.
- The build-tree proof logs contain `134,713` null-font diagnostics because the temporary `res` junction did not also stage the shipped `SDL2.dll` and `SDL2_ttf.dll`. A staged-install smoke renders text with zero FontAtlas errors; future soaks must run from a complete staged directory and treat these diagnostics as an infrastructure failure.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | FAIL | 0/1800 | 28.019 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=1, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | PASS | 600/600 | 22.630 | 2,434.0 | 100 | 8/13/16 | 49 authored=1 | - |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 600/600 | 8.216 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 1800/1800 | 75.401 | 876.7 | 100 | 3/5/6 | 49 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 600/600 | 27.985 | 1,709.9 | 100 | 5/6/10 | 49 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime= | FAIL | 0/600 | 4.321 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=2, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| theed | route | route | PASS | 1800/1800 | 50.196 | 1,307.7 | 100 | 5/5/5 | 49 authored=1 | - |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime= | FAIL | 0/600 | 2.056 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=3, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime= | FAIL | 0/600 | 2.404 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=3, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| palace | route | route | PASS | 1800/1800 | 83.332 | 992.5 | 100 | 7/12/28 | 22 authored=1 | - |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | PASS | 600/600 | 22.171 | 150.6 | 58 | 11/13/16 | 49 authored=1 | - |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=0 | PASS | 600/600 | 21.970 | 1,105.4 | 67 | 7/7/8 | 14 authored=1 | - |
| tato | route | route | PASS | 1800/1800 | 49.529 | 659.0 | 100 | 4/5/5 | 0 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime=0 | PASS | 600/600 | 16.020 | 1,769.6 | 100 | 5/5/6 | 49 authored=1 | - |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 14.372 | 1,268.8 | 100 | 2/2/2 | 49 authored=1 | - |
| corus1 | route | route | PASS | 1800/1800 | 133.770 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime= | FAIL | 0/600 | 4.950 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=6, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 18.243 | 539.8 | 94 | 4/6/6 | 50 authored=1 | - |
| ruins | route | route | PASS | 1800/1800 | 63.269 | 1,240.8 | 100 | 3/4/4 | 22 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 19.764 | 108.3 | 100 | 13/14/14 | 105 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime= | FAIL | 0/600 | 3.335 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=7, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| streets | route | route | PASS | 1800/1800 | 82.244 | 36,427.3 | 100 | 12/18/37 | 78 authored=1 | - |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 600/600 | 8.147 | 234.8 | 100 | 10/13/13 | 33 authored=1 | visible=0 |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=0 | PASS | 600/600 | 14.316 | 176.0 | 100 | 6/15/15 | 0 authored=1 | - |
| hangar | route | route | PASS | 1800/1800 | 79.663 | 347.6 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime= | FAIL | 0/600 | 5.336 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=9, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 22.756 | 1,338.5 | 88 | 8/8/9 | 4 authored=1 | - |
| core | route | route | PASS | 1800/1800 | 96.969 | 2,335.8 | 100 | 3/5/5 | 0 authored=1 | - |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime= | FAIL | 0/600 | 7.737 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=10, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 600/600 | 37.399 | 14,043.3 | 100 | 4/12/261 | 0 authored=1 | death=246 |
| mini1 | route | route | PASS | 1800/1800 | 64.209 | 1,392.3 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 35.100 | 234.8 | 97 | 2/2/2 | 105 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 38.805 | 897.3 | 76 | 3/3/3 | 49 authored=1 | - |
| mini2 | route | route | PASS | 1800/1800 | 69.067 | 7,042.2 | 100 | 6/6/7 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 600/600 | 21.280 | 2,453.0 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=0 | PASS | 600/600 | 27.513 | 2,204.5 | 100 | 11/11/11 | 105 authored=1 | - |
| mini3 | route | route | PASS | 1800/1800 | 80.112 | 1,328.8 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | PASS | 600/600 | 28.712 | 74.2 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=0 | PASS | 600/600 | 27.209 | 1,328.1 | 100 | 9/10/10 | 4 authored=1 | - |
| mini4 | route | route | FAIL | 0/1800 | 2.999 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=14, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime= | FAIL | 0/600 | 2.958 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=14, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime= | FAIL | 0/600 | 3.686 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=14, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| corus2 | route | route | PASS | 1800/1800 | 47.373 | 849.9 | 70 | 2/3/4 | 49 authored=1 | - |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime= | FAIL | 0/600 | 10.610 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=15, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=0 | PASS | 600/600 | 26.154 | 335.1 | 100 | 8/9/9 | 49 authored=1 | - |
| train1 | route | route | PASS | 1800/1800 | 98.019 | 1,517.6 | 100 | 0/0/0 | 49 authored=1 | - |
| train2 | route | route | PASS | 1800/1800 | 57.621 | 339.1 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 23.938 | 2,012.9 | 100 | 1/1/1 | 105 authored=1 | visible=0 |
| train3 | route | route | FAIL | 0/1800 | 1.830 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=18, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 600/600 | 11.197 | 2,154.5 | 100 | 4/5/6 | 105 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime= | FAIL | 0/600 | 1.361 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=18, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train5 | route | route | FAIL | 0/1800 | 1.642 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=19, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime= | FAIL | 0/600 | 1.650 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=19, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime= | FAIL | 0/600 | 8.419 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=19, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train6 | route | route | FAIL | 0/1800 | 1.787 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=20, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime= | FAIL | 0/600 | 1.728 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=20, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime= | FAIL | 0/600 | 1.981 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=20, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train7 | route | route | FAIL | 0/1800 | 12.726 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=21, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime= | FAIL | 0/600 | 3.251 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=21, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime= | FAIL | 0/600 | 2.683 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=21, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train4 | route | route | FAIL | 0/1800 | 0.940 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=22, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime= | FAIL | 0/600 | 0.769 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=22, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime= | FAIL | 0/600 | 1.809 | 0.0 | 0 |  |  | exit=-1073741819, level= expected=22, missing frame summary, triangles=, crash marker, pixels=, visible=, energy=, death= |
| arena | route | route | PASS | 1800/1800 | 50.100 | 766.0 | 200 | 1/1/1 | 22 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 21.656 | 1,491.4 | 200 | 1/1/1 | 49 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
