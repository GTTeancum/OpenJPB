# Boss Candidate Probe Audit

Generated: 2026-08-15 03:52:52 -04:00

This note records boss/set-piece candidates that were probed but not promoted
into `tools/smoke_bosses.ps1`. A candidate should only enter the boss smoke
matrix after it has an authoritative placement or trigger anchor that can be
forced and verified with `placementStatus=1` and `runtimePlacement=<target id>`.

## Core Darth Maul

Status: authoritative anchor found; promoted to boss smoke.

- Retail stream evidence exists: `10_CoreMaulFight1.wav` and
  `10_CoreMaulFight2.wav` are present in the recovered stream table.
- The earlier probe confused the J3D actor-path label with the loaded model.
  Direct executable reconstruction of `loader_loadEnemies` shows that
  `corguard.baf` matches `sObiNames[43]`, then loads `sModelNames[43]`,
  `maul_d`. Core actor slot `4` is therefore Darth Maul model `43`.
- Placement `11` is the authoritative final-arena anchor: actor `4`, AI `33`,
  owner `2`, 250 HP, position `31284/2560/-17242`.
- A 1,800-frame process-local probe resolved placement `11` as live object `2`,
  model `43`, energy `250/250`, with authored AI and rendered geometry. The
  smoke matrix now asserts the placement and the loader-resolved model instead
  of looking for a literal `sithjedi.baf` label.
- Direct comparison with the installed executable found a reconstruction field
  error in `loader_FinalizeEnemyPlayer`: retail writes model exclusion bit
  `0x200` at player offset `+0x144` (`forceFlags`), while the reconstructed
  source wrote it at `+0x140` (`pFlags`). That made listed models, including
  both Mauls, fail the exact `player_DoCollisions` `0x602` mask. The corrected
  loader preserves `pFlags` and writes the canonical field.
- After the field correction, a process-local canonical-contact run against
  placement `11` records model `43`, attack contact, one processed damage event,
  and energy changing from `250` to `238`.

Probe logs:

- `out/boss-core-probe/core-with-actors.console.txt`
- `out/boss-candidate-probes/core-maul-d-p2.console.txt`
- `out/boss-candidate-probes/core-maul-d-p2-180.console.txt`

Lifecycle proof is complete. A bounded canonical-contact run drives placement
`11` from one HP through its death animation to terminal placement status `2`
with a cleared handle. Direct `core.j3d` inspection also corrects an earlier
assumption: AI `33` contains no extension-spawn opcode and placement `11` has
zero delete-links. Its `enemyExt[0]=64` is AI data, while later AI-47 Maul
placements `31..34` are independent range-activated encounter stages.

## Palace Offscreen Bosses

Status: lifecycle resolved; correctly not promoted as bosses.

- Recovered `level_Palace()` calls
  `level_Palace_KillOffscreenBoss(163)` and
  `level_Palace_KillOffscreenBoss(164)`.
- Installed `palace.j3d` resolves the actual activation chain. Range-active
  placement `113` (owner `3`, AI `60`) references guards `163/164` and starts
  controller `183`; controller `183` (owner `0`, AI `11`) references and starts
  the same guards. Both guards are actor `13`, AI `58`, owner `2`, with no
  independent range-activation flag.
- A 240-frame native process-local run at placement `113`'s authored camera-21
  coordinate retires trigger `113` to status `2`, leaves controller `183`
  active at status `1`, and activates guards `163/164` at status `1` with their
  real model, AI, physics, and animation owners.
- `level_Palace_KillOffscreenBoss` is the terminal cleanup owner, not an
  activator: an active guard is reduced to zero energy only after its tracked X
  position reaches `-8500` or greater and its player range is at least `750`.
  The focused object/scene regression covers both the kill and threshold
  rejection branches.
- `jpb_pc_palace_authored_activation_lifecycle` now locks the real-asset
  `113 -> 183 -> 163/164` chain and all actor/AI/owner references.

Probe logs:

- `out/boss-candidate-probes/palace-offscreen-163.console.txt`
- `out/boss-candidate-probes/palace-offscreen-164.console.txt`

These guards remain absent from `tools/smoke_bosses.ps1` because the canonical
data identifies them as scripted cleanup actors, not boss placements.

## Hangar Set Piece

Status: lifecycle resolved; correctly covered as an objective rather than a boss.

- Recovered `level_Hangar()` is a timer/rescue special: it initializes
  `hangarStart`, draws the countdown text, watches `pilotsKilled`, and toggles
  global bits when the timer expires or the rescue succeeds.
- `braindmg_DeathReaction` is the canonical pilot-death producer: model `59` on
  level `9` increments `pilotsKilled`, restores the saved player position, and
  clears global bit `0`.
- `level_Hangar` starts a 400-second timer, publishes the timer objective bit,
  and completes when the timer expires or two pilots are killed. Counter values
  above `5` signal the authored completion bit; lower values also set the stage
  reset flags, checkpoint `0`, and restart score `0`.
- Focused regressions now cover reset initialization, timer/HUD ownership, UV
  scrolling, pilot-death production, rescue success, timeout, and both terminal
  counter branches.
- Quickload `hangar` exposes authored encounter placements and active
  powerup/state rows, but no distinct boss actor anchor comparable to the
  forced-placement boss smokes.

Probe log:

- `out/boss-candidate-probes/hangar.console.txt`

Hangar therefore remains outside the boss matrix. Its canonical objective path
is covered directly instead of being represented by a fabricated boss row.

## Hardware Boss Matrix Refresh

The separate native hardware boss matrix was refreshed after these lifecycle
closures. All eight authoritative entries pass at `960x540` for 180 frames,
including Core Maul. The Coruscant thug harness now starts at placement `138`
waypoint `0`, the nearby authored camera-66 anchor; the prior coordinate had no
camera record and caused a player fall before the proof frame. The retained
eight-entry contact sheet is `out/boss-smoke-hardware/contact-sheet.png` and the
ledger is `docs/BOSS_HARDWARE_SMOKE_AUDIT.md`.
