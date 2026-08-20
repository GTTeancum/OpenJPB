# Level Soak Crash Hunt

Generated: 2026-08-15 14:49:25 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `71ADF6252F90686F69ABEC72CB118150FACC95DF429E7BD07663AFE76506323D`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 33 passed / 32 failed
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-rerun\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | PASS | 1800/1800 | 72.068 | 1,086.5 | 95 | 8/9/12 | 49 authored=1 | - |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | PASS | 600/600 | 22.422 | 2,491.0 | 100 | 8/13/16 | 22 authored=1 | - |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 600/600 | 9.533 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 1800/1800 | 47.532 | 955.4 | 100 | 4/5/6 | 49 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 600/600 | 22.359 | 1,706.8 | 100 | 5/6/7 | 49 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=139 | FAIL | 600/600 | 31.483 | 755.6 | 0 | 13/13/13 | 49 authored=1 | visible=0, energy=0, death=157 |
| theed | route | route | FAIL | 1800/1800 | 37.506 | 229.8 | 100 | 7/7/7 | 49 authored=1 | exit=5 |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=-1 | FAIL | 600/600 | 16.763 | 64.6 | 0 | 0/1/1 | 49 authored=1 | visible=0, energy=0, death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=244 | FAIL | 600/600 | 17.648 | 0.0 | 100 | 3/3/3 | 22 authored=1 | exit=5, visible=0 |
| palace | route | route | PASS | 1800/1800 | 75.490 | 743.9 | 100 | 13/13/30 | 22 authored=1 | - |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | PASS | 600/600 | 21.994 | 134.6 | 49 | 11/13/16 | 50 authored=1 | - |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=210 | PASS | 600/600 | 24.920 | 300.5 | 100 | 5/6/6 | 49 authored=1 | - |
| tato | route | route | PASS | 1800/1800 | 46.928 | 796.9 | 100 | 4/5/5 | 0 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime=167 | PASS | 600/600 | 11.923 | 658.1 | 97 | 3/5/5 | 105 authored=1 | - |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=230 | PASS | 600/600 | 11.713 | 1,120.0 | 100 | 3/3/3 | 49 authored=1 | - |
| corus1 | route | route | PASS | 1800/1800 | 119.630 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=-1 | FAIL | 600/600 | 55.278 | 693.1 | 0 | 0/5/5 | 49 authored=1 | energy=0, death=157 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=60 | PASS | 600/600 | 19.556 | 1,232.4 | 94 | 4/6/6 | 4 authored=1 | - |
| ruins | route | route | PASS | 1800/1800 | 54.248 | 1,225.9 | 100 | 3/4/4 | 49 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=124 | PASS | 600/600 | 11.338 | 24.7 | 100 | 13/14/14 | 105 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime=252 | FAIL | 600/600 | 10.132 | 181.5 | 100 | 7/7/7 | 49 authored=1 | visible=0 |
| streets | route | route | PASS | 1800/1800 | 66.549 | 2,651.1 | 100 | 1/2/2 | 78 authored=1 | - |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | PASS | 600/600 | 20.874 | 392.2 | 100 | 9/13/13 | 0 authored=1 | - |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=126 | PASS | 600/600 | 14.508 | 237.0 | 100 | 7/15/15 | 0 authored=1 | - |
| hangar | route | route | PASS | 1800/1800 | 97.790 | 212.3 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | FAIL | 600/600 | 12.749 | 1,493.8 | 100 | 7/7/7 | 4 authored=1 | exit=5, pixels=0 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=119 | FAIL | 600/600 | 19.937 | 1,461.3 | 0 | 8/8/9 | 49 authored=1 | energy=0, death=297 |
| core | route | route | FAIL | 1800/1800 | 62.794 | 3,429.7 | 0 | 3/5/5 | 49 authored=1 | energy=0, death=865 |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime=0 | FAIL | 600/600 | 26.230 | 956.6 | 0 | 2/3/3 | 49 authored=1 | energy=0, death=157 |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=80 | FAIL | 600/600 | 43.661 | 431.4 | 0 | 10/12/610 | 22 authored=1 | energy=0, death=366 |
| mini1 | route | route | PASS | 1800/1800 | 57.827 | 1,392.3 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 26.690 | 430.5 | 100 | 2/2/2 | 105 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=9 | PASS | 600/600 | 14.500 | 248.7 | 91 | 3/3/3 | 49 authored=1 | - |
| mini2 | route | route | PASS | 1800/1800 | 67.638 | 1,152.7 | 100 | 3/3/3 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 600/600 | 20.109 | 2,453.0 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=27 | PASS | 600/600 | 20.526 | 1,464.9 | 100 | 11/11/11 | 49 authored=1 | - |
| mini3 | route | route | PASS | 1800/1800 | 70.493 | 1,303.1 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | FAIL | 600/600 | 21.811 | 1,118.2 | 100 | 8/9/9 | 22 authored=1 | exit=5 |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=10 | PASS | 600/600 | 29.684 | 1,788.0 | 100 | 9/10/10 | 49 authored=1 | - |
| mini4 | route | route | FAIL | 1800/1800 | 23.918 | 1,393.1 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 3.799 | 1,933.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=49 | FAIL | 600/600 | 4.948 | 1,968.9 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| corus2 | route | route | FAIL | 1800/1800 | 45.131 | 1,297.0 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=747 |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime=0 | FAIL | 600/600 | 28.358 | 2,113.6 | 0 | 13/13/15 | 49 authored=1 | energy=0, death=296 |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=130 | PASS | 600/600 | 29.469 | 335.1 | 100 | 8/9/9 | 49 authored=1 | - |
| train1 | route | route | PASS | 1800/1800 | 69.673 | 1,159.5 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | route | route | PASS | 1800/1800 | 43.363 | 442.4 | 100 | 0/0/0 | 49 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 20.891 | 1,747.1 | 100 | 1/1/1 | 105 authored=1 | visible=0 |
| train3 | route | route | FAIL | 1800/1800 | 23.671 | 1,054.5 | 0 | 0/0/0 | 59 authored=1 | energy=0, death=182 |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 600/600 | 9.048 | 1,868.3 | 100 | 4/5/6 | 105 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime=6 | FAIL | 600/600 | 5.353 | 1,085.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train5 | route | route | FAIL | 1800/1800 | 37.321 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | FAIL | 600/600 | 12.163 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=5 | FAIL | 600/600 | 12.244 | 57.9 | 0 | 7/7/7 | 59 authored=1 | visible=0, energy=0, death=369 |
| train6 | route | route | FAIL | 1800/1800 | 43.326 | 749.9 | 0 | 5/5/5 | 49 authored=1 | energy=0, death=157 |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | FAIL | 600/600 | 12.154 | 316.8 | 100 | 6/6/6 | 49 authored=1 | visible=0 |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime=12 | FAIL | 600/600 | 12.259 | 936.8 | 0 | 7/7/7 | 49 authored=1 | visible=0, energy=0, death=157 |
| train7 | route | route | FAIL | 1800/1800 | 92.012 | 2,872.7 | 0 | 8/8/8 | 49 authored=1 | energy=0, death=746 |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | FAIL | 600/600 | 24.989 | 93.6 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=52 |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=20 | FAIL | 600/600 | 22.373 | 85.5 | 0 | 10/10/10 | 49 authored=1 | energy=0, death=52 |
| train4 | route | route | FAIL | 1800/1800 | 4.670 | 936.6 | 0 | 1/1/1 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime=0 | FAIL | 600/600 | 2.265 | 938.6 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime=32 | FAIL | 600/600 | 3.436 | 1,103.5 | 0 | 8/8/8 | 59 authored=1 | energy=0, death=193 |
| arena | route | route | PASS | 1800/1800 | 60.803 | 1,757.4 | 200 | 1/1/1 | 49 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 20.614 | 1,085.9 | 200 | 1/1/1 | 105 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, missing frame summary, missing draw activity, no visible player samples, death, or explicit crash markers. Low or zero travel and runtime-current-enemy mismatches are retained as navigation/NPC telemetry rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
