# Boss Candidate Probe Audit

Generated: 2026-08-15 03:52:52 -04:00

This note records boss/set-piece candidates that were probed but not promoted
into `tools/smoke_bosses.ps1`. A candidate should only enter the boss smoke
matrix after it has an authoritative placement or trigger anchor that can be
forced and verified with `placementStatus=1` and `runtimePlacement=<target id>`.

## Core Darth Maul

Status: pending discovery; not promoted.

- Retail stream evidence exists: `10_CoreMaulFight1.wav` and
  `10_CoreMaulFight2.wav` are present in the recovered stream table.
- Quickload `core` initializes level `10`, but the camera director starts on
  placement `1`, actor `4`, AI `36`, actor `corguard.baf`; the loaded JPX
  placement dump does not expose a Maul or `sithjedi.baf` actor.
- Recovered code points to special handling for model `43` (`maul_d_model`) in
  damage/blocking behavior, which suggests the final duel may be staged through
  player/model state rather than a normal enemy placement.
- Explicit two-player Maul-D probing selected models `0/43` and loaded P2 with
  `energy:200/200`, but the 180-frame validation reported
  `second_player=(ready=1,triangles=823,pixels=0)` and failed because the
  second selected character was not posed/rendered visibly. That is useful
  load evidence, but not visual boss-smoke proof yet.

Probe logs:

- `out/boss-core-probe/core-with-actors.console.txt`
- `out/boss-candidate-probes/core-maul-d-p2.console.txt`
- `out/boss-candidate-probes/core-maul-d-p2-180.console.txt`

Next credible step: find the original Core duel trigger or add a narrowly
scoped debug spawn/pose hook for the second player, then require visible P2
pixels before considering a Core Maul smoke row.

## Palace Offscreen Bosses

Status: pending discovery; not promoted.

- Recovered `level_Palace()` calls
  `level_Palace_KillOffscreenBoss(163)` and
  `level_Palace_KillOffscreenBoss(164)`.
- Direct probes at placements `163` and `164` found both placements inactive
  (`status=0`) while the runtime camera/enemy target resolved to other active
  placements (`161` and `108` respectively).
- The recovered helper only kills those placements if they are already active
  and offscreen relative to player range; it does not itself start a boss fight.

Probe logs:

- `out/boss-candidate-probes/palace-offscreen-163.console.txt`
- `out/boss-candidate-probes/palace-offscreen-164.console.txt`

Next credible step: identify the trigger path that activates Palace placements
`163/164`, then verify an active runtime placement before adding coverage.

## Hangar Set Piece

Status: pending discovery; not promoted.

- Recovered `level_Hangar()` is a timer/rescue special: it initializes
  `hangarStart`, draws the countdown text, watches `pilotsKilled`, and toggles
  global bits when the timer expires or the rescue succeeds.
- Quickload `hangar` exposes authored encounter placements and active
  powerup/state rows, but not a distinct boss actor anchor comparable to the
  existing forced-placement boss smokes.

Probe log:

- `out/boss-candidate-probes/hangar.console.txt`

Next credible step: if Hangar needs smoke coverage, treat it as a set-piece HUD
or objective smoke with timer/pilot-state assertions instead of a boss-placement
row.
