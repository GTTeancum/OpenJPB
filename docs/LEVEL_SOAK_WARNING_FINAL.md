# Level Soak Crash Hunt

Generated: 2026-08-15 18:19:23 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `BAC174AE2145A2A82B56AA508CCC6D7CE9CA06AAC8E62CFF799E0948EE49A64A`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 5 passed / 4 warned / 0 failed
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-warning-final\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| theed | route | route | PASS | 1800/1800 | 51.084 | 229.8 | 100 | 7/7/7 | 49 authored=1 | - |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=-1 | WARN | 600/600 | 26.936 | 64.6 | 0 | 0/1/1 | 49 authored=1 | visible=0, energy=0, death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=244 | WARN | 600/600 | 21.456 | 0.0 | 100 | 3/3/3 | 22 authored=1 | visible=0 |
| hangar | route | route | PASS | 1800/1800 | 105.598 | 212.3 | 100 | 4/4/6 | 49 authored=1 | - |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | WARN | 600/600 | 13.748 | 1,493.8 | 100 | 7/7/7 | 4 authored=1 | pixels=0 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=119 | WARN | 600/600 | 26.773 | 1,461.3 | 0 | 8/8/9 | 49 authored=1 | energy=0, death=297 |
| mini3 | route | route | PASS | 1800/1800 | 70.826 | 1,303.1 | 100 | 8/9/9 | 49 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | PASS | 600/600 | 24.305 | 1,118.2 | 100 | 8/9/9 | 22 authored=1 | - |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=10 | PASS | 600/600 | 22.955 | 1,788.0 | 100 | 9/10/10 | 49 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
