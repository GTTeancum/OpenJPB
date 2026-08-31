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
