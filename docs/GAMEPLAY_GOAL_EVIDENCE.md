# Gameplay Goal Evidence

Installed executable under test:

`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`

Current installed/build SHA-256:

`75B9A53436AF87D59DE92BFCF800E6C94964B28FE83FD79A07D88B82069CC9AF`

## Evidence Artifacts

- Character-select keyboard/XInput arbitration log:
  `docs\review_artifacts\goal_character_select_keyboard_xinput_installed.log`
- FED spawn, camera, saber, and depth-rejection log:
  `docs\review_artifacts\goal_fed_spawn_saber_installed.log`
- FED frame capture from the same installed command:
  `docs\review_artifacts\goal_fed_spawn_saber_installed.png`
  This frame intentionally shows the FED doorway ambush: NPCs have moved
  into the foreground while the player is behind the door. The player's saber
  is queued and attached in the log, but the visible blade is culled by the
  completed scene depth.
- Direct FED saber-depth diagnostic log:
  `docs\review_artifacts\goal_saber_depth_installed.log`

## Key Proof Lines

- Character select: `character=(state=24,select=0,players=1,models=1/1,...)`
  after a seeded keyboard owner, held XInput left, and one deliberate keyboard
  right input.
- FED spawn/camera: `spawn_view=(model=0,input=keyboard,...collision=1.000)`
  and `frames=36 mode=game-camera`.
- Saber attachment: `player_weapon=(model=0,saber=1,...outer=1,...core=1,attached=1,unmatched=0,...)`.
- Saber occlusion diagnostics: `saber_glow=11/0/0/306` in the FED handoff
  proof and `saber_glow=11/0/0/612` in the direct FED diagnostic. The fourth
  field is depth-rejected glow samples.

## Verification Commands

```powershell
ctest --test-dir build -C Release -R "jpb_projection_tests|jpb_pc_xinput_tests|jpb_menu_tests|jpb_pc_title_character_select|jpb_pc_.*_gameplay_handoff" --output-on-failure

& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' 'C:\Games\Star Wars Jedi Power Battles\res\level\jpx\fed\fed.jpx' --cad 'C:\Games\Star Wars Jedi Power Battles\res\animation\obi_wan.cad' --bmd 'C:\Games\Star Wars Jedi Power Battles\res\MODEL\obi_wan.bmd' --headless --title-character-select --headless-keyboard-phase none 1 --headless-xinput-phase left 12 --headless-keyboard-phase d 1 --headless-keyboard-phase none 12 --frames 26

& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' 'C:\Games\Star Wars Jedi Power Battles\res\level\jpx\fed\fed.jpx' --cad 'C:\Games\Star Wars Jedi Power Battles\res\animation\obi_wan.cad' --bmd 'C:\Games\Star Wars Jedi Power Battles\res\MODEL\obi_wan.bmd' --cmb 'C:\Games\Star Wars Jedi Power Battles\res\combo\obi_wan.cmb' --enemy-cad 'C:\Games\Star Wars Jedi Power Battles\res\animation\battle_d.cad' --enemy-bmd 'C:\Games\Star Wars Jedi Power Battles\res\MODEL\battle_d.bmd' --validate-player-saber --headless --mute --front-end-flow --headless-phase select 1 --headless-phase none 1 --headless-phase select 1 --headless-phase none 1 --headless-phase select 1 --headless-phase none 1 --headless-phase select 1 --headless-phase none 3 --headless-phase select 1 --headless-phase none 3 --headless-phase select 1 --headless-phase none 20 --frames 36
```
