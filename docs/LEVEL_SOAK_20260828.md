# Level Soak Crash Hunt

Generated: 2026-08-28 07:44:33 -04:00

- Executable: `C:\Users\smmel\AppData\Local\Temp\jpb-full-soak-20260828\jpb_pc_game.exe`
- SHA-256: `B2D9D116E2DC874E02F3123B99670A086C5AFB9A3CB1EF2FBF254AF56C5DF171`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 36 passed / 29 warned / 0 failed
- Warning summary: 26 death/energy, 2 visibility-only, 1 visibility+death
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-20260828-staged-full\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

## Assessment

- The complete matrix has no crashes, timeouts, process failures, wrong-level results, missing frame summaries, or missing geometry.
- All 89 console logs contain zero FontAtlas errors. The executable was staged with the shipped resources, `SDL2.dll`, and `SDL2_ttf.dll`.
- `27` of the `29` warned rows finish with positive energy. Their death markers are retained historical telemetry after the game has recovered.
- Train3 placement `6` reaches its authored player-exit endpoint. Its zero-energy final state is not a stalled process.
- Train5 placement `5` ends the 600-frame sample shortly before recovery. The identical route extended to 750 frames records afterlife at `610`, player exit/game-death at `670`, and final energy `100`.
- Streets placement `0` leaves the authored camera track after the forced teleport. Train2 placement `0` is fully hidden behind foreground geometry. Their zero-visible-player results are harness placement artifacts.

Conclusion: this soak found no remaining crash or gameplay-lifecycle failure.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | WARN | 1800/1800 | 69.043 | 1,091.7 | 86 | 8/9/20 | 50 authored=1 | death=773 |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | PASS | 600/600 | 23.079 | 2,434.0 | 100 | 8/13/16 | 49 authored=1 | - |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 600/600 | 10.872 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 1800/1800 | 79.924 | 876.7 | 100 | 3/5/6 | 49 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 600/600 | 26.576 | 1,709.9 | 100 | 5/6/10 | 49 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 600/600 | 29.942 | 35,448.8 | 100 | 7/13/21 | 49 authored=1 | death=66 |
| theed | route | route | PASS | 1800/1800 | 61.047 | 1,307.7 | 100 | 5/5/5 | 49 authored=1 | - |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=0 | WARN | 600/600 | 17.856 | 16,740.9 | 100 | 5/5/6 | 49 authored=1 | death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=0 | WARN | 600/600 | 19.516 | 7,710.2 | 100 | 5/5/8 | 4 authored=1 | death=59 |
| palace | route | route | PASS | 1800/1800 | 86.354 | 992.5 | 100 | 7/12/28 | 22 authored=1 | - |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | PASS | 600/600 | 22.683 | 150.6 | 58 | 11/13/16 | 49 authored=1 | - |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=0 | PASS | 600/600 | 24.423 | 1,105.4 | 67 | 7/7/8 | 14 authored=1 | - |
| tato | route | route | PASS | 1800/1800 | 54.207 | 659.0 | 100 | 4/5/5 | 0 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime=0 | PASS | 600/600 | 20.424 | 1,769.6 | 100 | 5/5/6 | 49 authored=1 | - |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 17.061 | 1,268.8 | 100 | 2/2/2 | 49 authored=1 | - |
| corus1 | route | route | PASS | 1800/1800 | 139.116 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 600/600 | 45.253 | 1,443.3 | 100 | 3/5/8 | 22 authored=1 | death=53 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 44.243 | 539.8 | 94 | 4/6/6 | 50 authored=1 | - |
| ruins | route | route | PASS | 1800/1800 | 84.139 | 1,240.8 | 100 | 3/4/4 | 22 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 20.247 | 108.3 | 100 | 13/14/14 | 105 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime=0 | WARN | 600/600 | 22.257 | 16,903.7 | 91 | 9/9/17 | 49 authored=1 | death=93 |
| streets | route | route | PASS | 1800/1800 | 98.557 | 36,427.3 | 100 | 12/18/37 | 78 authored=1 | - |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 600/600 | 11.626 | 234.8 | 100 | 10/13/13 | 33 authored=1 | visible=0 |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=0 | PASS | 600/600 | 19.910 | 176.0 | 100 | 6/15/15 | 0 authored=1 | - |
| hangar | route | route | PASS | 1800/1800 | 96.445 | 347.6 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | WARN | 600/600 | 26.067 | 9,790.4 | 100 | 4/7/11 | 49 authored=1 | death=169 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 25.581 | 1,338.5 | 88 | 8/8/9 | 4 authored=1 | - |
| core | route | route | PASS | 1800/1800 | 109.755 | 2,335.8 | 100 | 3/5/5 | 0 authored=1 | - |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime=0 | WARN | 600/600 | 31.999 | 8,435.1 | 100 | 3/3/6 | 4 authored=1 | death=158 |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 600/600 | 35.999 | 14,043.3 | 100 | 4/12/261 | 0 authored=1 | death=246 |
| mini1 | route | route | PASS | 1800/1800 | 64.268 | 1,392.3 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 36.158 | 234.8 | 97 | 2/2/2 | 105 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 27.647 | 897.3 | 76 | 3/3/3 | 49 authored=1 | - |
| mini2 | route | route | PASS | 1800/1800 | 78.949 | 7,042.2 | 100 | 6/6/7 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 600/600 | 20.493 | 2,453.0 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=0 | PASS | 600/600 | 25.992 | 2,204.5 | 100 | 11/11/11 | 105 authored=1 | - |
| mini3 | route | route | PASS | 1800/1800 | 83.713 | 1,328.8 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | PASS | 600/600 | 27.396 | 74.2 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=0 | PASS | 600/600 | 29.062 | 1,328.1 | 100 | 9/10/10 | 4 authored=1 | - |
| mini4 | route | route | WARN | 1800/1800 | 39.338 | 1,324.2 | 100 | 3/3/6 | 49 authored=1 | death=158 |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 14.357 | 1,446.2 | 100 | 3/3/6 | 105 authored=1 | death=157 |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 14.035 | 1,526.4 | 100 | 3/6/9 | 105 authored=1 | death=157 |
| corus2 | route | route | PASS | 1800/1800 | 50.768 | 849.9 | 70 | 2/3/4 | 49 authored=1 | - |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime=0 | WARN | 600/600 | 21.137 | 16,249.8 | 100 | 2/10/13 | 4 authored=1 | death=232 |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=0 | PASS | 600/600 | 33.950 | 335.1 | 100 | 8/9/9 | 49 authored=1 | - |
| train1 | route | route | PASS | 1800/1800 | 100.624 | 1,517.6 | 100 | 0/0/0 | 49 authored=1 | - |
| train2 | route | route | PASS | 1800/1800 | 54.065 | 339.1 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 27.266 | 2,012.9 | 100 | 1/1/1 | 105 authored=1 | visible=0 |
| train3 | route | route | WARN | 1800/1800 | 23.389 | 532.0 | 100 | 0/0/0 | 49 authored=1 | death=91 |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 600/600 | 10.049 | 2,154.5 | 100 | 4/5/6 | 105 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime=-1 | WARN | 600/600 | 11.380 | 23,043.8 | 0 | 0/3/3 | 59 authored=1 | energy=0, death=80 |
| train5 | route | route | WARN | 1800/1800 | 43.195 | 356.0 | 100 | 4/6/23 | 22 authored=1 | death=70 |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 13.247 | 487.2 | 89 | 5/5/9 | 14 authored=1 | death=70 |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 600/600 | 12.209 | 57.9 | 0 | 7/7/7 | 59 authored=1 | visible=0, energy=0, death=369 |
| train6 | route | route | WARN | 1800/1800 | 49.791 | 297.9 | 100 | 3/5/88 | 22 authored=1 | death=56 |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 600/600 | 15.758 | 1,822.4 | 100 | 2/5/33 | 0 authored=1 | death=53 |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime=0 | WARN | 600/600 | 14.438 | 4,067.9 | 100 | 2/5/31 | 0 authored=1 | death=66 |
| train7 | route | route | WARN | 1800/1800 | 117.643 | 757.3 | 100 | 3/6/9 | 49 authored=1 | death=186 |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 600/600 | 34.486 | 1,061.0 | 100 | 4/6/10 | 49 authored=1 | death=52 |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 600/600 | 42.560 | 4,483.0 | 100 | 4/10/14 | 49 authored=1 | death=52 |
| train4 | route | route | WARN | 1800/1800 | 29.326 | 987.0 | 100 | 2/2/4 | 49 authored=1 | death=69 |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime=0 | WARN | 600/600 | 10.409 | 124.7 | 97 | 2/3/6 | 49 authored=1 | death=66 |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime=0 | WARN | 600/600 | 12.358 | 25,313.9 | 100 | 1/8/10 | 4 authored=1 | death=98 |
| arena | route | route | PASS | 1800/1800 | 66.969 | 766.0 | 200 | 1/1/1 | 22 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 19.355 | 1,491.4 | 200 | 1/1/1 | 49 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
