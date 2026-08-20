# Level Soak Crash Hunt

Generated: 2026-08-16 18:51:23 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `BAC174AE2145A2A82B56AA508CCC6D7CE9CA06AAC8E62CFF799E0948EE49A64A`
- Route pass: 1800 frames per level at 640x360
- Placement pass: 600 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 2 passed / 1 warned / 0 failed
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\level-soak-corus1-repro\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| corus1 | route | route | PASS | 1800/1800 | 152.293 | 374.9 | 94 | 3/3/3 | 22 authored=1 | - |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=-1 | WARN | 600/600 | 65.751 | 693.1 | 0 | 0/5/5 | 49 authored=1 | energy=0, death=157 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=60 | PASS | 600/600 | 29.143 | 1,232.4 | 94 | 4/6/6 | 4 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
