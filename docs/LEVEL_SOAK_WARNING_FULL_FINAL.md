# Level Soak Crash Hunt

Generated: 2026-08-16 19:36:16 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `BAC174AE2145A2A82B56AA508CCC6D7CE9CA06AAC8E62CFF799E0948EE49A64A`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 35 passed / 30 warned / 0 failed
- Warning summary: 21 death/energy, 1 pixel-proof miss, 4 visibility+death, 4 visibility-only
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-warning-full-final\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | PASS | 1800/1800 | 83.809 | 1,086.5 | 95 | 8/9/12 | 49 authored=1 | - |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | PASS | 600/600 | 32.885 | 2,491.0 | 100 | 8/13/16 | 22 authored=1 | - |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 600/600 | 17.541 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 1800/1800 | 79.828 | 955.4 | 100 | 4/5/6 | 49 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 600/600 | 28.309 | 1,706.8 | 100 | 5/6/7 | 49 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=139 | WARN | 600/600 | 48.617 | 755.6 | 0 | 13/13/13 | 49 authored=1 | visible=0, energy=0, death=157 |
| theed | route | route | PASS | 1800/1800 | 43.243 | 229.8 | 100 | 7/7/7 | 49 authored=1 | - |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=-1 | WARN | 600/600 | 20.325 | 64.6 | 0 | 0/1/1 | 49 authored=1 | visible=0, energy=0, death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=244 | WARN | 600/600 | 21.800 | 0.0 | 100 | 3/3/3 | 22 authored=1 | visible=0 |
| palace | route | route | PASS | 1800/1800 | 126.070 | 743.9 | 100 | 13/13/30 | 22 authored=1 | - |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | PASS | 600/600 | 34.945 | 134.6 | 49 | 11/13/16 | 50 authored=1 | - |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=210 | PASS | 600/600 | 43.309 | 300.5 | 100 | 5/6/6 | 49 authored=1 | - |
| tato | route | route | PASS | 1800/1800 | 62.934 | 796.9 | 100 | 4/5/5 | 0 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime=167 | PASS | 600/600 | 14.782 | 658.1 | 97 | 3/5/5 | 105 authored=1 | - |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=230 | PASS | 600/600 | 12.591 | 1,120.0 | 100 | 3/3/3 | 49 authored=1 | - |
| corus1 | route | route | PASS | 1800/1800 | 157.022 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=-1 | WARN | 600/600 | 76.571 | 693.1 | 0 | 0/5/5 | 49 authored=1 | energy=0, death=157 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=60 | PASS | 600/600 | 21.842 | 1,232.4 | 94 | 4/6/6 | 4 authored=1 | - |
| ruins | route | route | PASS | 1800/1800 | 59.512 | 1,225.9 | 100 | 3/4/4 | 49 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=124 | PASS | 600/600 | 12.565 | 24.7 | 100 | 13/14/14 | 105 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime=252 | WARN | 600/600 | 11.607 | 181.5 | 100 | 7/7/7 | 49 authored=1 | visible=0 |
| streets | route | route | PASS | 1800/1800 | 103.778 | 2,651.1 | 100 | 1/2/2 | 78 authored=1 | - |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | PASS | 600/600 | 29.376 | 392.2 | 100 | 9/13/13 | 0 authored=1 | - |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=126 | PASS | 600/600 | 18.959 | 237.0 | 100 | 7/15/15 | 0 authored=1 | - |
| hangar | route | route | PASS | 1800/1800 | 91.182 | 212.3 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | WARN | 600/600 | 11.040 | 1,493.8 | 100 | 7/7/7 | 4 authored=1 | pixels=0 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=119 | WARN | 600/600 | 17.708 | 1,461.3 | 0 | 8/8/9 | 49 authored=1 | energy=0, death=297 |
| core | route | route | WARN | 1800/1800 | 56.935 | 3,429.7 | 0 | 3/5/5 | 49 authored=1 | energy=0, death=865 |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime=0 | WARN | 600/600 | 22.281 | 956.6 | 0 | 2/3/3 | 49 authored=1 | energy=0, death=157 |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=80 | WARN | 600/600 | 39.898 | 431.4 | 0 | 10/12/610 | 22 authored=1 | energy=0, death=366 |
| mini1 | route | route | PASS | 1800/1800 | 61.404 | 1,392.3 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 600/600 | 37.206 | 430.5 | 100 | 2/2/2 | 105 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=9 | PASS | 600/600 | 17.980 | 248.7 | 91 | 3/3/3 | 49 authored=1 | - |
| mini2 | route | route | PASS | 1800/1800 | 98.955 | 1,152.7 | 100 | 3/3/3 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 600/600 | 28.526 | 2,453.0 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=27 | PASS | 600/600 | 18.511 | 1,464.9 | 100 | 11/11/11 | 49 authored=1 | - |
| mini3 | route | route | PASS | 1800/1800 | 66.043 | 1,303.1 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | PASS | 600/600 | 29.589 | 1,118.2 | 100 | 8/9/9 | 22 authored=1 | - |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=10 | PASS | 600/600 | 30.409 | 1,788.0 | 100 | 9/10/10 | 49 authored=1 | - |
| mini4 | route | route | WARN | 1800/1800 | 30.882 | 1,393.1 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 5.819 | 1,933.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=49 | WARN | 600/600 | 8.122 | 1,968.9 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| corus2 | route | route | WARN | 1800/1800 | 64.947 | 1,297.0 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=747 |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime=0 | WARN | 600/600 | 33.569 | 2,113.6 | 0 | 13/13/15 | 49 authored=1 | energy=0, death=296 |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=130 | PASS | 600/600 | 31.185 | 335.1 | 100 | 8/9/9 | 49 authored=1 | - |
| train1 | route | route | PASS | 1800/1800 | 94.612 | 1,159.5 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | route | route | PASS | 1800/1800 | 72.727 | 442.4 | 100 | 0/0/0 | 49 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 31.979 | 1,747.1 | 100 | 1/1/1 | 105 authored=1 | visible=0 |
| train3 | route | route | WARN | 1800/1800 | 33.817 | 1,054.5 | 0 | 0/0/0 | 59 authored=1 | energy=0, death=182 |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 600/600 | 10.084 | 1,868.3 | 100 | 4/5/6 | 105 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime=6 | WARN | 600/600 | 4.711 | 1,085.3 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train5 | route | route | WARN | 1800/1800 | 40.919 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 600/600 | 13.241 | 942.0 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=157 |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=5 | WARN | 600/600 | 11.915 | 57.9 | 0 | 7/7/7 | 59 authored=1 | visible=0, energy=0, death=369 |
| train6 | route | route | WARN | 1800/1800 | 36.386 | 749.9 | 0 | 5/5/5 | 49 authored=1 | energy=0, death=157 |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 600/600 | 14.268 | 316.8 | 100 | 6/6/6 | 49 authored=1 | visible=0 |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime=12 | WARN | 600/600 | 18.721 | 936.8 | 0 | 7/7/7 | 49 authored=1 | visible=0, energy=0, death=157 |
| train7 | route | route | WARN | 1800/1800 | 122.284 | 2,872.7 | 0 | 8/8/8 | 49 authored=1 | energy=0, death=746 |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 600/600 | 36.393 | 93.6 | 0 | 6/6/6 | 49 authored=1 | energy=0, death=52 |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=20 | WARN | 600/600 | 20.810 | 85.5 | 0 | 10/10/10 | 49 authored=1 | energy=0, death=52 |
| train4 | route | route | WARN | 1800/1800 | 4.467 | 936.6 | 0 | 1/1/1 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime=0 | WARN | 600/600 | 2.056 | 938.6 | 0 | 3/3/3 | 49 authored=1 | energy=0, death=157 |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime=32 | WARN | 600/600 | 2.786 | 1,103.5 | 0 | 8/8/8 | 59 authored=1 | energy=0, death=193 |
| arena | route | route | PASS | 1800/1800 | 56.160 | 1,757.4 | 200 | 1/1/1 | 49 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 600/600 | 19.367 | 1,085.9 | 200 | 1/1/1 | 105 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
