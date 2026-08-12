# Menu and HUD review evidence

This document records the current review evidence for the installed PC
recomp executable:

`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`

The goal of this file is to keep the proof points inspectable without relying
on chat history.

## Installed executable

Current installed SHA-256:

`E92FE608BE5C54A4053E1B9C0450109994422597FC9CE2056B9B36DABF7B5CD7`

Current build SHA-256:

`E92FE608BE5C54A4053E1B9C0450109994422597FC9CE2056B9B36DABF7B5CD7`

The installed executable and `build\Release\jpb_pc_game.exe` matched when
this evidence was captured.

## Exact-path review captures

All captures below were produced by running the installed executable at
`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`. Durable copies are
stored in this repository under `docs\review_artifacts`.

- Main menu:
  `docs\review_artifacts\jpb_exact_main_installed.png`
- Level select:
  `docs\review_artifacts\jpb_exact_level_installed.png`
- Overwrite-save popup:
  `docs\review_artifacts\jpb_exact_popup_installed.png`
- Overwrite-save popup trace:
  `docs\review_artifacts\jpb_exact_popup_installed_trace.txt`
- Gameplay HUD after FED handoff:
  `docs\review_artifacts\jpb_exact_gameplay_hud_installed.png`

Side-by-side review composites place the user-provided reference on the left
and the exact installed result on the right:

- Main menu comparison:
  `docs\review_artifacts\jpb_side_by_side_main_menu.png`
- Level select comparison:
  `docs\review_artifacts\jpb_side_by_side_level_select.png`

Combined visual proof sheet:

- `docs\review_artifacts\jpb_review_visual_proofs_contact_sheet.png`

The gameplay HUD capture was generated through the front-end handoff and
logged:

- `game_state=(level=1,mode=6,players=1,versus=0)`
- `control_sources=(p1=keyboard,p2=xinput,last=keyboard)`
- `spawn_view=...collision=1.000`
- `frames=23 mode=game-camera`
- `text=2/2/0`

The overwrite-save popup capture was generated from the exact installed
executable and logged:

- `framebuffer=960x540,scale=1.125/0.500`
- `screen_draw[0]=...confirm_Box_green.png,dst=330/292/630/426`
- `color=255/255/255/190`

## Source-backed menu assertions

The overwrite-save popup uses the recovered `newgameconfirmMdef` command
stream. Tests assert:

- Popup prompt text is `Overwrite save game?`
- Text is center anchored.
- Selected `YES` row is at y `357`.
- Confirm box resolves to destination `330/292/630/426`.
- `newgameconfirmMdef` carries the popup x operand as `0`, command `0x48`,
  and top operand `585`; `setPivotPositionMM` pivot `1` converts that x
  operand to the screen midpoint before applying the `-300..300` popup width.

Relevant test locations:

- `tests\test_menu.c:603`
- `tests\test_menu.c:605`
- `tests\test_menu.c:608`
- `tests\test_menu.c:1413`
- `tests\test_menu.c:1414`
- `tests\test_menu.c:1418`
- `tests\test_menu.c:1435`

## Source-backed HUD assertions

`tests\test_game.c` covers the `game_DisplayOverlay`,
`game_DrawScore`, and `game_DrawItems` HUD owners:

- Full overlay P1 score and item text positions.
- Full overlay P2 score and item text positions.
- P1 and P2 score-panel and item-panel SCB rectangles.
- Continue icon rectangles, including the second continue icon at
  `312/44/366/98`.
- Continue icon wrap layout for six remaining credits, including the first
  row at `217/44/271/98` and wrapped sixth icon at `293/82/347/136`.
- P1 and P2 life/Force bar final draw rectangles through `_DrawTexture`.
- Compact overlay P1 and P2 score anchors, including right-aligned P2
  compact score at x `592`.
- Rescue counter and level-11 pilot counter branches, including the
  single-digit pilot counter at x `368`.

Relevant test locations:

- `tests\test_game.c:679`
- `tests\test_game.c:680`
- `tests\test_game.c:691`
- `tests\test_game.c:713`
- `tests\test_game.c:714`
- `tests\test_game.c:732`
- `tests\test_game.c:743`
- `tests\test_game.c:777`
- `tests\test_game.c:801`
- `tests\test_game.c:806`

`tests\test_player.c` covers player-owned HUD-adjacent overlays:

- Compact P1 damage overlay rectangle `50/127/70/140`.
- Full-overlay P1 damage bar rectangle `211/127/231/140`.
- Full-overlay mirrored P2 damage bar rectangle `399/127/429/140`.

Relevant test locations:

- `tests\test_player.c:1816`
- `tests\test_player.c:1838`
- `tests\test_player.c:1843`

## Verification commands

Focused checks:

```powershell
build\Release\jpb_game_tests.exe
build\Release\jpb_player_tests.exe
build\Release\jpb_menu_tests.exe
```

Combined menu, HUD, player overlay, and title-flow regression:

```powershell
ctest --test-dir build -C Release -R "jpb_menu_tests|jpb_game_tests|jpb_player_tests|jpb_pc_title" --output-on-failure
```

Latest recorded result:

`28/28` tests passed.

Installed/build hash check:

```powershell
Get-FileHash 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' -Algorithm SHA256
Get-FileHash 'build\Release\jpb_pc_game.exe' -Algorithm SHA256
```
