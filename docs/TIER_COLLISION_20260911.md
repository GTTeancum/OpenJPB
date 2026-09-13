# FED tier collision — September 11, 2026

## Reproduction and fixes

The user identified the multi-tiered semicircular FED room (authored camera dolly 23). Two initial AI-disabled routes were insufficient to rule out player sticking. A wider sweep reproduced it with no enemy actors and no jump input.

At X=-1600, Y=5632, Z=-11392, sustained downward input repeatedly reversed during falling and returned the player to the same tier. `CalcMovement` compared `reversoi` (set from `gGlobalTimer`) with `totalframes`. The shipped executable compares with `gGlobalTimer` at RVA 0x581FC0. Restored that timer comparison. Regression checks cover the exact expiry boundary and deliberately different timer/frame values.

Diagonal runs from X=-2688 and X=-5000 then exposed a separate climb-back cycle: falling motion 4 switched to ledge climb 62 while walking away from the tier. `CheckCubeBlocking` used `face.vy`; the shipped code reads/writes physics offset +0x4C (`angle.vy`). Restored actual yaw. Its airborne descent checks and accepted-grab reset also use +0x10C (`airmov.vy`), not displacement +0x118 (`mov.vy`). Restored those fields, the canonical contact-counter update order, and the ledge-vector conversion factor (100.0f at shipped RVA 0x33B85C).

Authority: local `C:/Games/Star Wars Jedi Power Battles/game.exe`, recovered physics structure, and `decompiler-export/original/physics.c`. No geometry, authored barriers, or ledge permissions were weakened.

## Verification

- Release physics tests, including direct comparison with the shipped collision routine and real FED collision bank: pass.
- Added grounded and airborne semicircle samples. Explicit retained-ledge cases use disagreeing actual/desired facing: four valid grabs accepted and four facing-away cases rejected, with full physics/player state compared to retail.
- Enemy tests pass; AI suspension is opt-in through `--review-disable-ai` and normal gameplay retains AI.
- Process-local sweep: seven starting X positions and three downward angles, 240 frames each, without AI or jump input. Final results and trails are under `out/live-fixes-20260908/tier-sweep/`.
- Additional jump and AI-enabled encounter smoke runs: `tier-jump-final.*` and `tier-ai-final.*`. The AI-enabled player descends to the next tier amid combat; this is not a proof of every enemy's traversal.

Evidence before correction: `tier-reverse.csv` (persistent reverse timer), `tier-long-2688.csv` and `tier-long-5000.csv` (motion-62 climb-back loops after the timer-only fix), and `tier-sweep-before.json`. Sustained input eventually walks beyond the room and respawns on some routes; stalls after that respawn are outside this reproduction.

## Live status

Deployed the tested executable to C:/Games/Star Wars Jedi Power Battles/jpb_pc_game.exe on September 11. Installed and tested-candidate SHA-256 match: 0E39A2DB465AA5834F88B78C884D62FE3FC7E20ECC83E75E27C4DC274D8C7DE0. Previous installed executable preserved as out/live-fixes-20260908/before-tier-fix.exe. All 21 final sweep runs exited successfully with no motion-62 climb-back in the tier area. TODO #3 remains open for live confirmation with both players and enemies.
