# Level Soak Crash Hunt

Generated: 2026-08-31 03:36:08 -04:00

- Executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`
- SHA-256: `B0D117D266A7FD973EE277BD94B209653926D2E8D2624C13F63D6BAF45CFD354`
- Route pass: 5400 frames per level at 640x360
- Placement pass: 1800 frames per sampled NPC/enemy placement; samples per level: 2; skipped: False
- Result: 25 passed / 40 warned / 0 failed
- Warning summary: 36 death/energy, 1 other, 1 pixel-proof miss, 2 visibility-only
- Machine-readable results: `C:\Programming\GitHub\Jedi Power Battles recomp\out\overnight-soak-20260831\results.json`

The route pass cycles movement, jump, attack, block, and force+jump inputs. The placement pass enumerates authored enemy placements, samples them across each level by placement id, spawns the player at the authored placement coordinate, forces that NPC/enemy active, then runs the same jump/attack route.

## Warning Assessment

- All `36` death/energy warnings completed their requested frame counts and recovered to full energy. They prove that death/restart paths were exercised; they are not crash failures.
- A source-audited Streets replay proved that the route reaches cube ending flag `0x8` and the death-game transition at frame `246`, after `2,235` visible-player frames. The fixed-length harness then continued the shipped ending-camera pan for more than 5,000 frames, producing the zero-pixel endpoint. This is terminal-state overrun, not a pre-terminal visibility failure.
- Streets placement `0` is the first authored boundary actor and is a poor forced-coordinate visibility sample. Placement sampling now uses interior quantiles instead of endpoint records.
- Train2 placement `0` is class `72` (`pwrdrink`), which has no enemy runtime owner (`runtime=-1`). Pickup actors are now excluded from NPC/enemy placement sampling; Train2 has no eligible placement run.
- Core accepted only one synthetic direction change because AI `36` keeps the player under the shipped opening camera/control lock. No player-control proxy is attached. This does not demonstrate a crash, but it also does not provide meaningful synthetic traversal coverage until that authored sequence ends.
- Across all `65` scenarios there were no process crashes, assertions, timeouts, nonzero exits, incomplete frame counts, missing level geometry, or texture-load failure signatures.

## Repairs and Verification

- Corrected Streets camera type `5` against the shipped executable: both initial world-position selection and maximum-X framing read fixed global player physics slots `maPhysicsData[0]` and `[1]`, not scene-linked physics pointers. Focused camera tests now deliberately diverge those sources and pass with the canonical globals.
- Tightened the soak contract so Streets zero-pixel output is accepted only when ending flag `0x8`, a completed death-game transition, and prior visible-player frames all prove terminal-state overrun. Genuine pre-terminal black output remains a warning.
- Excluded canonical class-72 pickup actors from NPC/enemy placement samples and changed multi-sample selection to interior quantiles, removing two known invalid endpoint probes without weakening route coverage.
- Restored the shipped `CheckCubeBlocking` clear-sweep continuation: the accepted candidate endpoint becomes the origin for finalization instead of being discarded after a wall contact. A focused synthetic wall-slide regression fails without this assignment and passes with it.
- Added a repeated FED doorway wall-jump abuse route. After four opposing wall assaults and a neutral settle, the player finishes grounded at `y=3840`, idle in Motion `0`, with no death or opening-sequence retrigger.
- Restored usable gameplay Pause dispatch. Keyboard Continue and full front-end Controls and Quit-to-title routes pass through the canonical menu owner.
- Supplemental installed-layout Pause routes enter Audio mode `16` and Combos mode `17`; the fresh-game Ultimate Saber entry remains locked `OFF`; Quit cancellation returns to Pause. The exact shipped `gamepauseMenuMdef` contains no Restart entry, while the separately tested death/reset owner performs level restart.
- The first seven-boss run exposed a real Coruscant crash in `CSteamAchievements::SetAchievement`. Headless and hidden diagnostic process modes now install the existing stateful achievement adapter even when no control harness is active; ordinary visible gameplay is unchanged. The repaired seven-boss rerun passes `7/7`; see `docs/OVERNIGHT_BOSS_SMOKE_FIXED_20260831.md`.
- The clean Release suite passes `929/929`. A post-stage focused set passes `11/11`, covering SDL_mixer effects/streaming, physics, FED wall abuse, pickup contact, death/restart, New Game and overwrite, and Pause Continue/Controls/Quit.
- A separate 180-frame staged FED smoke omits `--control-harness` entirely and reports `scripted=0`, zero P1/P2 input edges or held bits, `180/180` visible player frames, full energy, and no death.
- Final staged executable: `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`, SHA-256 `1D59966B547B3207824DF2633F966C0CC143E92AB619B704DBB6E7118C61DF3B`.

## Remaining Manual Evidence

- Audible crackle/pop behavior cannot be signed off by the dummy-device audio regressions.
- Foreground-focus keyboard neutrality is enforced at every live `GetAsyncKeyState` reader, but still requires a visible focus-switch review.
- FED collision, Pause submenus, pickup/death UI, and New Game overwrite remain on the live sign-off list despite their passing automated routes.

| Level | Kind | Target | Status | Frames | Seconds | Travel | Energy | Enemy actors | Motion | Failure |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| fed | route | route | WARN | 5400/5400 | 260.138 | 14.4 | 100 | 7/9/70 | 0 authored=1 | death=747 |
| fed | placement | id=0 `d:\old-d\wobi\weasel\startup\21b.baf` runtime=0 | WARN | 1800/1800 | 89.477 | 2,664.4 | 100 | 7/13/31 | 22 authored=1 | death=776 |
| fed | placement | id=209 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=-1 | PASS | 1800/1800 | 28.569 | 13,159.5 | 100 | 0/18/20 | 0 authored=1 | - |
| marsh | route | route | PASS | 5400/5400 | 270.679 | 265.5 | 100 | 2/6/30 | 22 authored=1 | - |
| marsh | placement | id=0 `d:\old-d\wobi\weasel\startup\ronto.baf` runtime=0 | PASS | 1800/1800 | 84.688 | 1,328.2 | 100 | 5/6/18 | 22 authored=1 | - |
| marsh | placement | id=219 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 1800/1800 | 79.343 | 35,764.8 | 100 | 6/13/27 | 22 authored=1 | death=66 |
| theed | route | route | WARN | 5400/5400 | 178.387 | 2,119.3 | 100 | 10/14/63 | 22 authored=1 | death=3727 |
| theed | placement | id=0 `d:\old-d\wobi\weasel\startup\handmaid.baf` runtime=0 | WARN | 1800/1800 | 55.822 | 16,185.8 | 100 | 10/12/31 | 22 authored=1 | death=52 |
| theed | placement | id=244 `d:\old-d\wobi\weasel\startup\tatkid.baf` runtime=0 | PASS | 1800/1800 | 58.970 | 17.6 | 100 | 2/3/3 | 22 authored=1 | - |
| palace | route | route | WARN | 5400/5400 | 223.173 | 2,246.8 | 100 | 12/18/81 | 22 authored=1 | death=2848 |
| palace | placement | id=0 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 1800/1800 | 86.749 | 1,051.6 | 100 | 10/13/24 | 23 authored=1 | death=1683 |
| palace | placement | id=210 `d:\old-d\wobi\weasel\startup\whale.baf` runtime=0 | PASS | 1800/1800 | 74.038 | 1,312.4 | 100 | 4/7/9 | 22 authored=1 | - |
| tato | route | route | PASS | 5400/5400 | 191.292 | 847.8 | 100 | 4/5/6 | 22 authored=1 | - |
| tato | placement | id=0 `d:\weasel\tusken.baf` runtime=0 | PASS | 1800/1800 | 65.720 | 473.7 | 100 | 4/5/6 | 22 authored=1 | - |
| tato | placement | id=230 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 1800/1800 | 60.593 | 330.5 | 100 | 2/2/6 | 22 authored=1 | - |
| corus1 | route | route | WARN | 5400/5400 | 519.656 | 525.3 | 100 | 3/4/48 | 22 authored=1 | death=614 |
| corus1 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 1800/1800 | 183.775 | 1,448.1 | 100 | 3/7/26 | 22 authored=1 | death=53 |
| corus1 | placement | id=236 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 1800/1800 | 195.896 | 29,167.3 | 100 | 8/9/16 | 22 authored=1 | death=197 |
| ruins | route | route | PASS | 5400/5400 | 175.063 | 2,128.6 | 100 | 7/8/9 | 22 authored=1 | - |
| ruins | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=0 | PASS | 1800/1800 | 52.134 | 106.4 | 100 | 13/14/14 | 0 authored=1 | - |
| ruins | placement | id=252 `d:\old-d\wobi\weasel\startup\swpcr2.baf` runtime=0 | WARN | 1800/1800 | 50.919 | 17,334.9 | 100 | 10/11/25 | 22 authored=1 | death=92 |
| streets | route | route | WARN | 5400/5400 | 153.293 | 41,941.5 | 100 | 9/17/44 | 78 authored=1 | pixels=0 |
| streets | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 1800/1800 | 26.301 | 693.7 | 100 | 9/13/13 | 0 authored=1 | visible=0 |
| streets | placement | id=235 `d:\weasel\destroyr.baf` runtime=0 | PASS | 1800/1800 | 40.509 | 476.4 | 100 | 6/15/15 | 0 authored=1 | - |
| hangar | route | route | WARN | 5400/5400 | 206.715 | 984.5 | 100 | 4/5/12 | 22 authored=1 | death=961 |
| hangar | placement | id=0 `d:\old-d\wobi\weasel\startup\destroyr.baf` runtime=0 | WARN | 1800/1800 | 73.615 | 9,575.3 | 100 | 4/7/11 | 22 authored=1 | death=157 |
| hangar | placement | id=119 `e:\obi\startup\pwrdrink.baf` runtime=0 | WARN | 1800/1800 | 78.304 | 7,249.8 | 100 | 4/7/12 | 22 authored=1 | death=297 |
| core | route | route | WARN | 5400/5400 | 280.091 | 2,335.8 | 100 | 3/5/5 | 0 authored=1 | direction=1 expected-at-least=54 |
| core | placement | id=0 `d:\old-d\wobi\weasel\startup\corguard.baf` runtime=0 | WARN | 1800/1800 | 99.834 | 7,809.0 | 100 | 3/5/8 | 0 authored=1 | death=87 |
| core | placement | id=80 `d:\old-d\wobi\weasel\startup\baronsec.baf` runtime=0 | WARN | 1800/1800 | 95.017 | 14,043.3 | 100 | 4/12/261 | 0 authored=1 | death=246 |
| mini1 | route | route | PASS | 5400/5400 | 166.773 | 775.9 | 100 | 1/1/1 | 22 authored=1 | - |
| mini1 | placement | id=0 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 1800/1800 | 91.272 | 1,471.6 | 100 | 2/2/2 | 22 authored=1 | - |
| mini1 | placement | id=22 `e:\obi\startup\nrguard.baf` runtime=0 | PASS | 1800/1800 | 81.246 | 396.2 | 100 | 2/3/8 | 22 authored=1 | - |
| mini2 | route | route | PASS | 5400/5400 | 189.557 | 21,759.6 | 100 | 5/7/14 | 78 authored=1 | - |
| mini2 | placement | id=0 `e:\obi\startup\horns.baf` runtime=0 | PASS | 1800/1800 | 40.886 | 2,997.3 | 100 | 2/2/2 | 78 authored=1 | - |
| mini2 | placement | id=27 `e:\obi\startup\hunter.baf` runtime=0 | WARN | 1800/1800 | 52.176 | 43,904.8 | 100 | 5/11/17 | 78 authored=1 | death=297 |
| mini3 | route | route | PASS | 5400/5400 | 227.560 | 1,926.6 | 100 | 10/10/11 | 22 authored=1 | - |
| mini3 | placement | id=0 `d:\weasel\gungrd.baf` runtime=0 | PASS | 1800/1800 | 90.698 | 2,632.3 | 100 | 10/10/11 | 22 authored=1 | - |
| mini3 | placement | id=10 `d:\weasel\gunggrd.baf` runtime=0 | PASS | 1800/1800 | 71.478 | 1,386.5 | 100 | 11/11/12 | 22 authored=1 | - |
| mini4 | route | route | WARN | 5400/5400 | 127.752 | 130.8 | 100 | 3/3/11 | 22 authored=1 | death=158 |
| mini4 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 1800/1800 | 44.156 | 799.3 | 100 | 3/3/8 | 22 authored=1 | death=157 |
| mini4 | placement | id=49 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 1800/1800 | 48.182 | 870.6 | 100 | 3/6/11 | 22 authored=1 | death=157 |
| corus2 | route | route | WARN | 5400/5400 | 175.452 | 637.1 | 100 | 3/4/17 | 22 authored=1 | death=3446 |
| corus2 | placement | id=0 `e:\obi\startup\spedblu.baf` runtime=0 | WARN | 1800/1800 | 70.612 | 15,623.4 | 100 | 3/10/18 | 22 authored=1 | death=233 |
| corus2 | placement | id=130 `d:\old-d\wobi\weasel\startup\corbum1.baf` runtime=0 | WARN | 1800/1800 | 58.330 | 29,696.1 | 100 | 3/9/15 | 22 authored=1 | death=59 |
| train1 | route | route | PASS | 5400/5400 | 228.444 | 2,603.7 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | route | route | PASS | 5400/5400 | 165.516 | 380.8 | 100 | 0/0/0 | 22 authored=1 | - |
| train2 | placement | id=0 `e:\obi\startup\pwrdrink.baf` runtime=-1 | WARN | 1800/1800 | 77.506 | 3,058.9 | 100 | 0/1/1 | 22 authored=1 | visible=0 |
| train3 | route | route | WARN | 5400/5400 | 84.886 | 214.7 | 100 | 0/0/0 | 22 authored=1 | death=88 |
| train3 | placement | id=0 `c:\obi\startup\spedcya.baf` runtime=0 | PASS | 1800/1800 | 24.478 | 3,109.4 | 100 | 3/5/6 | 22 authored=1 | - |
| train3 | placement | id=6 `c:\obi\startup\spedcya.baf` runtime=-1 | WARN | 1800/1800 | 27.801 | 23,272.0 | 100 | 0/3/3 | 22 authored=1 | death=83 |
| train5 | route | route | WARN | 5400/5400 | 148.931 | 414.5 | 100 | 3/4/109 | 22 authored=1 | death=70 |
| train5 | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | WARN | 1800/1800 | 49.439 | 542.4 | 100 | 3/4/37 | 22 authored=1 | death=70 |
| train5 | placement | id=5 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | PASS | 1800/1800 | 47.549 | 600.5 | 100 | 3/5/7 | 22 authored=1 | - |
| train6 | route | route | WARN | 5400/5400 | 160.597 | 297.9 | 100 | 3/5/260 | 22 authored=1 | death=56 |
| train6 | placement | id=0 `d:\old-d\wobi\weasel\startup\baron.baf` runtime=0 | WARN | 1800/1800 | 43.496 | 1,650.8 | 100 | 3/5/89 | 22 authored=1 | death=53 |
| train6 | placement | id=12 `d:\old-d\wobi\weasel\startup\jawa.baf` runtime=0 | WARN | 1800/1800 | 46.641 | 3,975.8 | 100 | 3/5/88 | 22 authored=1 | death=66 |
| train7 | route | route | WARN | 5400/5400 | 332.981 | 417.4 | 100 | 3/6/67 | 22 authored=1 | death=184 |
| train7 | placement | id=0 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 1800/1800 | 130.100 | 2,025.1 | 100 | 3/6/26 | 22 authored=1 | death=52 |
| train7 | placement | id=20 `d:\old-d\wobi\weasel\startup\gonk.baf` runtime=0 | WARN | 1800/1800 | 133.438 | 5,360.6 | 100 | 3/10/30 | 22 authored=1 | death=52 |
| train4 | route | route | WARN | 5400/5400 | 78.161 | 834.4 | 100 | 1/3/55 | 22 authored=1 | death=69 |
| train4 | placement | id=0 `c:\obi\startup\21b.baf` runtime=0 | WARN | 1800/1800 | 18.559 | 1,564.7 | 100 | 1/3/21 | 22 authored=1 | death=66 |
| train4 | placement | id=44 `c:\obi\startup\21b.baf` runtime=0 | WARN | 1800/1800 | 18.615 | 24,212.2 | 100 | 2/8/21 | 22 authored=1 | death=95 |
| arena | route | route | PASS | 5400/5400 | 167.622 | 745.7 | 200 | 1/1/1 | 22 authored=1 | - |
| arena | placement | id=0 `d:\old-d\wobi\weasel\startup\pwrdrink.baf` runtime=0 | PASS | 1800/1800 | 62.742 | 124.0 | 200 | 1/1/1 | 22 authored=1 | - |

## Crash Signals

A row fails for timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers. Missing gameplay pixels, no visible player samples, player death, low or zero travel, and runtime-current-enemy mismatches are retained as route-quality warnings rather than crash failures.

Each row has a matching console log under the output directory. Captures are omitted by default to keep soak output small.
