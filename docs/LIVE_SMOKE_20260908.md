# Staged build and TODO smoke audit — 2026-09-08

User instruction for this pass: stage, do not deploy; smoke the remaining TODOs
and close only items established by the evidence. Two-player ownership applies
to campaign and Versus: both keyboard and controller may drive P1 before entry;
the device used to enter two-player character selection becomes P1's device;
exit releases both devices back to P1 for the next session.

## Build

- Release executable: `out/live-stage-20260908/jpb_pc_game.exe` (3,285,504 bytes).
- SHA-256: `4030790446DA0AC862F8817E18620A5B21C3C4885E52D11E3F570534AC86AD3D`.
- Stage includes SDL2, SDL2_ttf and SDL2_mixer from the local installation, with
  a `res` junction to the installed assets. Saves stay in the stage/test paths.
- Installed executable was not replaced. Its SHA-256 remains
  `E7923A15E04E4CB2FB85B3736A4057276485B10B29FB7130AC1A971560147AA8`.
- Release and Debug builds succeeded. No desktop input or UI automation was used.

## Corrections and canonical evidence

The preview draw at retail RVA `0xC40F0` reads a clut at
`0x945AAE + LevelSelect * 12`. With `fontSpec` at `0x944780`, this is
`fontSpec[409 + LevelSelect].clut`, not `410 + LevelSelect`. The loader at
`0xCF240` already populated the correct bank beginning at fontSpec 410. The old
test masked the error by overwriting fontSpec 411. That overwrite is removed;
all 14 selectable levels now exercise the loaded bank, rectangle, layer and
label. The 15 source filenames were independently read from the executable's
pointer table at RVA `0x4C5BA8`, matched to the implementation and opened from
the installed assets. The final native 1920x1080 FED menu capture was inspected
at `out/level-select-fed-20260908.png`.

Versus already had the requested direct two-player destination in the working
tree. The input correction now locks the entering device for either two-player
character menu (0x0D/0x0E), lets P2 confirm first, retains ownership through
gameplay and clears it when returning to one-player ownership. Neutral scripted
device frames no longer silently change ownership. The eight process-local
ownership cases cover both modes, both entering devices and a second session.
The direct Versus entry is an explicit requested product behavior; retail's
0x9D destination still contains a player-count branch, so it is not claimed as
a byte-for-byte retail entry flow.

The re-entry tests exposed an actual handoff regression: the host republished
request mode 4 after constructing New Game, causing immediate score-screen entry.
Retail `game_gPlayTheGame` at RVA `0xA98F0` publishes active mode 6 after the load
constructor; modes 4/5 belong to the score transition. The host now publishes 6
after every successful gameplay load. The same executable shows that a pending
`nextLevel` signal is preserved while a menu is active; that boundary is now
covered in the enemy transition regression.

## Smoke results

| Item | Result and remaining boundary |
| --- | --- |
| 1 — two-player entry/ownership | Closed. Eight ownership lifecycle cases pass in Release and Debug. Fresh-boot and FED → Pause/Quit → Versus device orders reach the arena in active mode 6. Four final hidden hardware routes also prove campaign → Quit → swapped-device Versus and Versus → Quit → both devices driving P1. |
| 2 — enemy bars | Projection and player tests pass. Moving-camera 1920x1080 live placement remains open. |
| 3 — bolts | Bullet lifecycle tests pass. The real-asset door frame submits 1,490 screen polygons with zero drops, exceeding the old 832 limit. The 2,400-frame soak exits cleanly but records zero projectile launches; it is inconclusive for extended bolt visibility. |
| 4 — first door | A 600-frame forward run stops at `(12541.8, 3328, -9489.4)`, on the closed side of the door. Collision and strict enemy-clear range tests pass. The full encounter-clear/open/pass route remains live. |
| 5 — second boss ending | Authored next-level signal, active-to-score transition, next-level selection and pause preservation pass. The complete second-boss run-off remains live. |
| 6 — Pause Audio | Menu/slider and audio tests pass. Subjective playback and physical volume controls remain live. |
| 7 — first boss introduction | The real-asset 900-frame intro regression passes, as does boss damage/death. Ordinary traversal into the introduction remains live. |
| 8 — thumbnails | Closed by executable/table/asset audit, all-level regression and inspected native FED capture. |
| 9 — repeated FED loads | Closed by four real D3D11 loads in one process, three quit/re-entry cycles, 1,749 frames and exit 0. Registered as `jpb_pc_d3d11_four_fed_reloads`. |

Logs in `out/`:

- `smoke-todo-20260908.log`: initial focused matrix, 17/17 passed.
- `smoke-handoffs-final-20260908.log`: 9/10 passed; the failure is the blocking
  test below, not a menu/handoff test.
- `smoke-final-20260908.log`: focused Release matrix, 9/9 passed, including
  the registered four-load D3D11 regression.
- `smoke-debug-final-20260908.log`: final Debug ownership/menu/enemy cases, 3/3.
- `coop-controller-hardware-20260908.log` and `coop-keyboard-hardware-20260908.log`:
  final device-type correction tested through two-player campaign, Quit and
  swapped-device Versus. Both finish with two handoffs, two active players and
  the expected opposite input assignments. Matching `*-native-*` logs prove the
  initial FED load was two-player campaign (`players=2`, `versus=0`).
- `versus-exit-controller-hardware-20260908.log` and
  `versus-exit-keyboard-hardware-20260908.log`: both games are exited in each run;
  subsequent keyboard and controller inputs each move the P1 title selection.
  Both finish in one-player title ownership, selection 2, and exit 0. The final
  eight-case lifecycle test was also rerun under both build configurations.
- `door-smoke-20260908.log` and `projectile-smoke-20260908.log`: additional staged
  headless probes. Initial font-library warnings in those runs do not establish
  text rendering; the final inspected menu capture was regenerated after staging
  the required SDL libraries.

## Blocking smoke failure resolved

The initial smoke run found four failing expectations in `jpb_braindmg_tests`.
The follow-up audit traced them to the test fixture: it attempted to reject
reaction animations by setting `Motion.Seq` to 50, but retail
`anim_AddNextAnimSeq` (RVA `0x17750`) validates the unsigned short at Motion +6.
The matched PDB type `0x10AF` identifies that field as `globalID`; `Seq` is +4.
The blocking implementation already follows the executable's locked-target
condition and stun-reset order, so no gameplay change was needed.

The fixture now initializes and invalidates `globalID`, preserving its checks
that the locked target owns the special-block condition and that stun resets
even when animation activation fails. Blocking, animation, player, and enemy
suites pass in both Release and Debug (4/4 each). The resolved item was removed
from TODO, leaving six live-review items. The staged executable still matches
the Release build hash above, and the installed executable remains unchanged;
deployment is still on hold.
