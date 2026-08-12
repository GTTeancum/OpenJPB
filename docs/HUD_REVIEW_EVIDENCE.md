# HUD review evidence

This document records HUD-only review evidence for the installed PC recomp
executable:

`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`

## Installed executable

Current installed SHA-256:

`7F21C811D5860DBB3AEC75C40C5159F3B991E5D9B9A76461E3A2F7B1B2488E7E`

Current build SHA-256:

`7F21C811D5860DBB3AEC75C40C5159F3B991E5D9B9A76461E3A2F7B1B2488E7E`

The installed executable and `build\Release\jpb_pc_game.exe` matched when
this evidence was captured.

## Current retail-scale HUD correction

This section supersedes older HUD scale notes below that mention
`ScreenHeight / 480`. A fresh executable trace of retail
`C:\Games\Star Wars Jedi Power Battles\game.exe` resolves
`getScaleAdjustment` at `0x1400F8AF0` to a divisor load from
`0x14029C204`. The bytes at that address are `00 00 87 44`, which is the
float `1080.0f`. The recomp now uses:

```c
return (float)OptionStruct.ScreenHeight / 1080.0f;
```

Retained proof artifacts:

- Retail disassembly/evidence:
  `build\re_hud\capstone_hud_layout_scaler_current.txt`
- Final compact 1920x1080 installed capture:
  `docs\review_artifacts\hud_retail_scaler_final_compact_1080.ppm`
- Final compact 1920x1080 installed trace:
  `docs\review_artifacts\hud_retail_scaler_final_compact_1080.log`
- Final reference-vs-recomp proof sheet:
  `docs\review_artifacts\hud_retail_scaler_final_side_by_side.png`
- Full-audit final compact 1920x1080 installed capture:
  `docs\review_artifacts\hud_full_audit_final_compact_1080.ppm`
- Full-audit final compact 1920x1080 installed trace:
  `docs\review_artifacts\hud_full_audit_final_compact_1080.log`
- Full-audit final reference-vs-recomp proof sheet:
  `docs\review_artifacts\hud_full_audit_final_side_by_side.png`

The final installed compact trace records `framebuffer=1920x1080` and
`scale=1.000/1.000`, with the compact item HUD at `44/856/236/1048`,
score text `0000000` at `48/62`, item count at `188/968`, and item sprite
size `192/192`. This matches the current retail-scale compact HUD footprint
instead of the earlier oversized `/480` reconstruction.

The staged executable now has explicit normal-gameplay HUD gates for the
PDB/EXE-recovered `OptionStruct.overlayMode == 1` branch of
`game_DrawScore` / `game_DrawItems` plus the `_AddLifeTile` projected
health/Force owner. These gates require the normal branch to publish score
text, the bottom-left item panel, and exactly six player HUD tile draws,
while rejecting the full-overlay `a_meter_main` score panel. The retained
strict logs are `docs\review_artifacts\hud_normal_strict_gate_960.log`,
`docs\review_artifacts\hud_normal_strict_gate_1080.log`, and
`docs\review_artifacts\hud_normal_strict_owner_1080.log`. End-of-turn
visual proof from the same staged executable is retained at
`docs\review_artifacts\hud_normal_strict_proof_sheet.png`, with gameplay
capture traces in
`docs\review_artifacts\hud_normal_gameplay_move_360_1080.log` and
`docs\review_artifacts\hud_normal_gameplay_attack_360_1080.log`.

The current installed validator sweep passed 42 exact HUD gates from
`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`: core, P2 core,
continue credits, rescue counter, pilot counter, compact lifetile,
projected lifetile, countdown timer/kill/success/fail, Hangar timer, damage
tracker, P2 damage tracker, compact damage tracker, Kadu race bars,
offscreen arrows, DebugLevel 2/3 labels, and enemy radar at both `960x540`
and `1920x1080`. The retained current logs are
`docs\review_artifacts\hud_exact_current_gate_1_core_960.log` through
`docs\review_artifacts\hud_exact_current_gate_42_radar_1080.log`, with the
summary in `docs\review_artifacts\hud_exact_current_status.txt`.

The same installed executable also passed a 36-scenario HUD owner-coverage
matrix with `--validate-hud-owner-coverage`. That umbrella gate rejects HUD
submissions outside recovered PDB/source owner buckets for screen draws,
HUD text draws, DebugLevel draw3d text, sprite displays, PSX digit draws,
and the `playerOffScreenArrow` screen-poly owner. The retained logs are
`docs\review_artifacts\hud_owner_coverage_matrix_1_core_owner_960.log`
through `docs\review_artifacts\hud_owner_coverage_matrix_36_radar_owner_1080.log`,
with the summary in
`docs\review_artifacts\hud_owner_coverage_matrix_status.txt`.

Current exact HUD gate commands:

```powershell
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-normal
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-normal-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-core
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-core-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-p2-core
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-p2-core-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 12 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-continue
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 12 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-continue-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload theed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-rescue
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload theed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-rescue-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload hangar --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-pilot-counter
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload hangar --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-pilot-counter-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-lifetile
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-lifetile-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-kill
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-kill-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 3 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-success
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 3 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-success-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 3 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-fail
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 3 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-fail-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload hangar --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-hangar
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload hangar --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-hangar-1080
```

## Exact-path HUD captures

All captures below were produced by running the installed executable at
`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`.

- Exact gameplay HUD frame:
  `docs\review_artifacts\jpb_exact_gameplay_hud_installed.png`
- Exact gameplay HUD trace:
  `docs\review_artifacts\jpb_exact_gameplay_hud_installed_trace.txt`
- Full-overlay HUD frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_full_overlay_installed.png`
- Full-overlay HUD trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_full_overlay_installed_trace.txt`
- HUD gate sweep frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_gate_sweep_installed.png`
- HUD gate sweep trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_gate_sweep_installed_trace.txt`
- Expanded RE HUD frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_expanded_re_installed.png`
- Expanded RE HUD trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_expanded_re_installed_trace.txt`
- Theed-chain HUD frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_theed_chain_installed.png`
- Theed-chain HUD trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_theed_chain_installed_trace.txt`
- Mini4 HUD audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_mini4_audit_installed.png`
- Mini4 HUD audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_mini4_audit_installed_trace.txt`
- HUD material-trace frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_material_trace_installed.png`
- HUD material-trace log from the staged installed exe:
  `docs\review_artifacts\jpb_hud_material_trace_installed_trace.txt`
- PSX HUD leaf audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_psx_leaf_audit_installed.png`
- PSX HUD leaf audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_psx_leaf_audit_installed_trace.txt`
- Text HUD leaf audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_text_leaf_audit_installed.png`
- Text HUD leaf audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_text_leaf_audit_installed_trace.txt`
- DrawText HUD bridge audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_drawtext_bridge_audit_installed.png`
- DrawText HUD bridge audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_drawtext_bridge_audit_installed_trace.txt`
- Current item-icon 128 material audit frame from the staged installed exe:
  `docs\review_artifacts\hud_full_sweep_item128_core_960.ppm`
- Current item-icon 128 material audit trace from the staged installed exe:
  `docs\review_artifacts\hud_full_sweep_item128_core_960.log`
- Current HUD item proof sheet:
  `docs\review_artifacts\jpb_hud_item128_proofs_installed.png`
- Current HUD owner-label audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_owner_labels_installed.png`
- Current HUD owner-label audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_owner_labels_installed_trace.txt`
- Current RE owner sweep HUD frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_core_sweep_installed.png`
- Current RE owner sweep HUD trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_core_sweep_installed_trace.txt`
- Current RE radar sweep HUD frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_radar_sweep_installed.png`
- Current RE radar sweep HUD trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_re_radar_sweep_installed_trace.txt`
- Continue-render audit frame from the staged installed exe:
  `docs\review_artifacts\jpb_hud_continue_render_audit_installed.png`
- Continue-render audit trace from the staged installed exe:
  `docs\review_artifacts\jpb_hud_continue_render_audit_installed_trace.txt`
- HUD-only visual proof sheet:
  `docs\review_artifacts\jpb_hud_visual_proofs_installed.png`
- Current HUD-only visual proof sheet:
  `docs\review_artifacts\jpb_hud_visual_proofs_installed_current.png`
- Current CountDown branch HUD proof sheet:
  `docs\review_artifacts\jpb_hud_countdown_branch_proofs_installed.png`
- Current RE HUD proof sheet from the installed 46-case sweep:
  `docs\review_artifacts\jpb_hud_re_proof_current_installed.png`
- Current full-frame HUD proof sheet from the staged installed exe:
  `docs\review_artifacts\jpb_hud_full_frame_proof_installed.png`

The capture was generated through the front-end FED handoff and logged:

- `game_state=(level=1,mode=6,players=1,versus=0)`
- `control_sources=(p1=keyboard,p2=xinput,last=keyboard)`
- `spawn_view=...collision=1.000`
- `player_resources=(p1=energy:100/100,force:100/100,items:0,force_owner:none;...)`
- `frames=23 mode=game-camera`
- `text=2/2/0`

## Legacy HUD Assertions

The section below is retained historical evidence from earlier HUD passes.
Where it conflicts with the "Current retail-scale HUD correction" section
above, the current section is authoritative.

PDB/function-map HUD owner audit:

- `inventory\function_map.tsv:1641`: `_AddBar`, `game.obj`, RVA `0xA6E60`,
  size `112`; direct rectangle forwarding tested.
- `inventory\function_map.tsv:1642`: `_AddLifeTile2D`, `game.obj`, RVA
  `0xA6ED0`, size `561`; direct P1/P2 life and Force rectangles tested.
- `inventory\function_map.tsv:1653`: `game_DisplayOverlay`, `game.obj`, RVA
  `0xA7630`, size `2848`; score, item, continue, rescue/counter, and pilot
  branches tested.
- Expanded byte-level retail trace in
  `build\re_hud\capstone_hud_expanded.txt` anchors
  `game_DisplayOverlay` at `0x1400A7630..0x1400A8150`: visible-overlay
  brightness update calls `UpdateBright` at `0x1400A769F`, suppressed-overlay
  fade calls it at `0x1400A79A0`, continue icons call
  `sprite_DisplaySprite` at `0x1400A7CCB`, score/item owners are invoked at
  `0x1400A7E4D`, `0x1400A7E54`, `0x1400A7E67`, and `0x1400A7E71`, rescue
  and pilot-counter branches load the same `192.0f` sprite size constants
  at `0x1400A7E8A/0x1400A7E95` and `0x1400A7FCF/0x1400A7FDA`, call
  `setPivotPositionAndFixScale` at `0x1400A7ECA` and `0x1400A7FFF`, call
  `sprite_DisplaySprite` at `0x1400A7F40` and `0x1400A803F`, and publish
  text through `_DrawText` at `0x1400A7FB0`.
- `inventory\function_map.tsv:1654`: `game_DrawBigNum`, `game.obj`, RVA
  `0xA8150`, size `257`; PSX digit texture id, size, x advance, alpha, and
  color call surface tested against the installed disassembly.
- `inventory\function_map.tsv:1655`: `game_DrawItems`, `game.obj`, RVA
  `0xA8260`, size `434`; item text and `charStuff` sprite identity tested.
- Matched installed `game.exe` disassembly for `game_DrawItems` loads the
  authored item constants `192.0f`, `192.0f`, `44.0f`, `32.0f`,
  `144.0f`, and `112.0f`, then calls `setPivotPositionAndFixScale`,
  `setPositionOffPivot`, `_DrawText`, and `sprite_DisplaySprite`.
- Byte-level retail trace in `build\re_hud\capstone_retail_hud.txt`
  anchors those `game_DrawItems` values at exact addresses:
  `0x1400A8293` width `192.0f`, `0x1400A829E` height `192.0f`,
  `0x1400A82B2` sprite x `44.0f`, `0x1400A82BE` sprite y `32.0f`,
  `0x1400A82C6` text x `144.0f`, `0x1400A82CE` text y `112.0f`,
  `0x1400A82D6` call `setPivotPositionAndFixScale`,
  `0x1400A82F7` call `setPositionOffPivot`, `0x1400A8399` call
  `_DrawText`, and `0x1400A83E5` call `sprite_DisplaySprite`.
- Raw installed `game.exe` data at VA `0x1404BABD0` contains the exact
  `charStuff` words `45, 46, 45, 48, 47, 45, 45, 45, 48, 46`.
- `inventory\function_map.tsv:1656`: `game_DrawScore`, `game.obj`, RVA
  `0xA8420`, size `822`; score text, panel sprite identity, and 2D
  life/Force bars tested.
- Expanded byte-level retail trace anchors `game_DrawScore` panel and text
  publication at `0x1400A8420..0x1400A8756`: score panel width
  `480.0f` at `0x1400A845D`, text y `24.0f` at `0x1400A8490` /
  `0x1400A84E2`, pivot/scaled placement calls at `0x1400A84AD`,
  `0x1400A84C4`, `0x1400A84DB`, `0x1400A84F8`, `0x1400A850F`, and
  `0x1400A8526`, score text through `_DrawText` at `0x1400A85BE`,
  `_AddLifeTile2D` at `0x1400A85E7`, and the score-panel sprite through
  `sprite_DisplaySprite` at `0x1400A861F`.
- `inventory\function_map.tsv:1915`: `level_Arena`, `level.obj`, RVA
  `0xB8B00`, size `563`; versus winner message position, scale, tint, and
  fade alpha tested through `_DrawText`.
- Expanded byte-level retail trace anchors `level_Arena` winner text through
  `_DrawText` at `0x1400B8D0C`.
- `inventory\function_map.tsv:1875`: `file_LoadResidentSprites`,
  `filesys.obj`, RVA `0x99F50`, size `214`; resident item icon names
  `a_detonator.tga`, `a_bolt.tga`, `a_battery.tga`, and `a_shield.tga`
  are loaded with retail option `1`.
- Byte-level retail trace in `build\re_hud\capstone_retail_hud.txt`
  anchors the resident sprite load loop at `0x140099F50..0x140099FD6`;
  the item resident sprites are loaded by the `rbx - 0x27 <= 0xa`
  option branch at `0x140099FAB..0x140099FBF`.
- The installed resident item textures `a_detonator.tga`, `a_bolt.tga`,
  `a_battery.tga`, and `a_shield.tga` are restored from their shipped
  `128x128` PNG sidecars into the `.tga` filenames requested by the
  recovered `file_LoadResidentSprites` path. Retail requests the `.tga`
  basenames, copies loaded image dimensions into material `iw/ih`, and
  `_RenderSprite` uses those material dimensions as the source window; the
  installed validators now fail if the item HUD owner regresses to a
  `512x512` resident material.
- `inventory\function_map.tsv:2769`: `_LoadTexture`, `wHook.obj`, RVA
  `0x1262E0`, size `1001`; material `iw/ih` are copied from
  `Texture+0x138/+0x13c`.
- Byte-level retail trace in `build\re_hud\capstone_retail_hud.txt`
  anchors `_RenderSprite` material source handling at
  `0x1400F7F22..0x1400F7F53`, and its `_DrawTexture` call at
  `0x1400F7FAF`.
- `inventory\function_map.tsv:2289`: `el_chavo::LoadTexture`,
  `wHook.obj`, RVA `0x119070`, size `289`; `.sgi` and `.pvr` names are
  rewritten to retail image extensions before `CreateTextureFromFile`.
- `inventory\function_map.tsv:683`: `CreateTextureFromFile`,
  `d3dtextr.obj`, RVA `0x3F2C0`, size `1705`; the published texture
  dimensions come from the loaded image object.
- `inventory\function_map.tsv:694` and `697`: `Texture::LoadImageData`
  and `Texture::LoadTargaFile`; SDL surface width/height are copied
  unchanged to `Texture+0x138/+0x13c`.
- `inventory\function_map.tsv:2988` and `2992`:
  `setPivotPositionAndFixScale` / `setPositionOffPivot`; HUD SCB
  rectangles and text anchors use retail `scaleAdjustment` math.
- `inventory\function_map.tsv:2982`: `_RenderSprite`, `sprite.obj`, RVA
  `0xF7E60`, size `1608`; HUD SCBs with flag `2` convert their recovered
  vertex rectangle to a `SCREENRECT`, use material `iw/ih` as the source
  rectangle, and submit through `_DrawTexture` at layer `0.001`.
- `inventory\function_map.tsv:3004`: `sprite_DisplaySprite`, `sprite.obj`,
  RVA `0xFA3B0`, size `234`; SCB vertices are direct integer x/y/w/h
  publications from the HUD owner.
- `inventory\function_map.tsv:3103`: `psxDrawTexture`, `text.obj`, RVA
  `0xFFE30`, size `687`; PSX texture ids `0xb5..0xbe` are digit glyphs.
  The PC runtime maps those ids to text draws at scale `2.0` from the same
  captured x/y and low-byte alpha.
- `inventory\function_map.tsv:783`: `_DrawText`, `debugtext.obj`, RVA
  `0x45350`, size `179`; HUD owners that call it share the same bridge into
  `SDLTextWriteScale`: packed high-byte alpha, integer-truncated x/y, fixed
  tint `11`, mode `0`, font style `2`, and `scale * 3.0f`.
- `inventory\function_map.tsv:2462`: `_AddLifeTile`, `player.obj`, RVA
  `0xE6660`, size `1195`; projection-dependent in-world health/Force tiles
  tested under a controlled camera.
- `inventory\function_map.tsv:1918`: `level_CountDown`, `level.obj`, RVA
  `0xB8FA0`, size `1591`; timer, objective, success, and failure HUD text
  anchors tested through the exact text call surface.
- Expanded byte-level retail trace anchors `level_CountDown` timer/objective
  pivots through `setPivotPositionAndFixScale` calls at `0x1400B9128`,
  `0x1400B9145`, `0x1400B9254`, `0x1400B926E`, and `0x1400B950D`, and text
  publication through `_DrawText` calls at `0x1400B9182`, `0x1400B91C2`,
  `0x1400B92C1`, and `0x1400B9301`.
- `inventory\function_map.tsv:1920`: `level_Hangar`, `level.obj`, RVA
  `0xB9770`, size `738`; hangar countdown timer and `sec` label anchors
  tested through the same `_DrawText` / `SDLTextWriteScale` call surface.
- Expanded byte-level retail trace anchors `level_Hangar` pivots through
  `setPivotPositionAndFixScale` at `0x1400B98CF` and `0x1400B98F6`, and
  text publication through `_DrawText` at `0x1400B9945` and `0x1400B9987`.
- `inventory\function_map.tsv:1925`: `level_Mini4`, `level.obj`, RVA
  `0xBA7B0`, size `155`; mini-level remaining-kill HUD digits tested
  through `menu_drawBigNums` and the PSX texture call surface.
- Expanded byte-level retail trace anchors `level_Mini4` at
  `0x1400BA7B0..0x1400BA84B`: gameplay-state suppression branches at
  `0x1400BA7B4..0x1400BA7BE`, reset clears death counters at
  `0x1400BA7D2..0x1400BA7E6`, and the remaining-kill HUD publishes through
  `menu_drawBigNums` with fixed x/y inputs `0xf0/0xb8`, digit count `2`,
  and RGB `0x80/0x80/0x80` at `0x1400BA7FF..0x1400BA826`.
- `inventory\function_map.tsv:1930`: `level_Theed`, `level.obj`, RVA
  `0xBB0A0`, size `384`; Theed itself does not publish screen rectangles,
  but it is now asserted as the level owner that sets game flag
  `0x01000000`, which `game_DisplayOverlay` consumes for the rescue/counter
  HUD sprite and text.
- Expanded byte-level retail trace anchors `level_Theed` at
  `0x1400BB0A0..0x1400BB220`: reset/start state is handled at
  `0x1400BB0BA..0x1400BB0F3`, the rescue/counter HUD flag is set by
  `game_gSetGameFlags(0x01000000)` at `0x1400BB0F9..0x1400BB0FE`, and the
  checkpoint/global-bit reset path sets flags `0x20` and `0x40` through
  calls at `0x1400BB1D7` and `0x1400BB1E1`.
- `inventory\function_map.tsv:2102`: `menu_drawBigNums`, `menu.obj`, RVA
  `0xC3190`, size `289`; zero-padded digit order, 5-pixel x advance,
  alpha, and RGB inputs tested.
- `inventory\function_map.tsv:86`: `ai_Kadu`, `boss.obj`, RVA `0x19ED0`,
  size `1977`; Kadu race HUD bar owners at
  `src\reconstructed\original\boss.c:363` and `:377` tested through the real
  rider-mounting owner path.
- `inventory\function_map.tsv:2946`: `playerOffScreenArrow`, `scene.obj`, RVA
  `0xF5050`, size `1458`; offscreen HUD arrow projection vertices tested.
- `inventory\function_map.tsv:841`: `enemy_Radar`, `enemy.obj`, RVA
  `0x4AD30`, size `947`; debug radar background, player marker, and
  owner-type enemy markers tested through `_DrawTexture`.
- Expanded byte-level retail trace anchors `enemy_Radar` draw submission
  through `_DrawTexture` at `0x14004AE3E`, `0x14004AEE3`, and
  `0x14004B060`.
- `src\reconstructed\original\player.c:542`: `player_DrawDamageTracker`,
  static player owner; compact/full damage overlay rectangles tested.
- `inventory\function_map.tsv:2482`: `player_gProcessPlayers`,
  `player.obj`, RVA `0xE8170`, size `2317`; installed disassembly at
  `0x1400E8352..0x1400E84BE` and `0x1400E85F8..0x1400E890B` restores the
  `OptionStruct.DebugLevel` developer HUD labels through
  `scr_debugPrintfXYZ`.
- Expanded byte-level retail trace anchors player HUD dispatch inside
  `player_gProcessPlayers`: `_AddLifeTile` is called at `0x1400E8340`,
  DebugLevel 2/3 text through `scr_debugPrintfXYZ` at `0x1400E843C`,
  `0x1400E8483`, and `0x1400E84BE`, and damage-tracker compact/full
  overlay pivots use `setPivotPositionAndFixScale` /
  `setPositionOffPivot` at `0x1400E8649/0x1400E8664`,
  `0x1400E86EC/0x1400E8707`, `0x1400E87DE/0x1400E87F9`, and
  `0x1400E887C/0x1400E8897`.
- `inventory\function_map.tsv:794`: `scr_debugPrintfXYZ`, `debugtext.obj`,
  RVA `0x45B80`, size `441`; installed disassembly confirms the formatted
  string is published to debug text with color `0xff8090a0`.
- `inventory\function_map.tsv:2259` through `2268`: `ovrlay_*`,
  `overlay.obj`, all size `3`; current reconstruction has generated shells
  only, so there is no live size/position owner to assert.

`tests\test_game.c` covers the `game_DisplayOverlay`,
`game_DrawScore`, and `game_DrawItems` HUD owners:

- Full overlay P1 score text `0000042` at `212/60`, scale `3.24`.
- Full overlay P1 item text `3` at `188/368`, scale `1.8`.
- `game_DrawBigNum(42, 10, 20)` draws digits `4` and `2` as PSX textures
  `0xb9` and `0xb7`, each `16x14`, with x positions `10` and `28`, y `20`,
  transparency `200`, and RGB `0x80/0x80/0x80`.
- `game_DrawBigNum(7, 100, 50)` draws the leading zero texture `0xb5` at
  x `100` and the `7` texture `0xbc` at x `118`.
- `test_psx_texture_hud_leaf_contract` now drives
  `jpb_PsxDrawTextureLayer` without the PSX hook installed, proving the
  renderer-facing leaf math: destination x/y are scaled by
  `gPSXDrawScaleX/Y`, destination width/height by `gPSXDrawScaleW/H`,
  source rects are `fontSpec` cells multiplied by four, `frontRGBoff`
  saturates RGB at `255`, and transparency `0x8400` / `0x8100` maps to
  alpha `0x3f` / `0x7f`.
- `test_sdl_text_hud_leaf_contract` now drives `SDLTextWriteScale` and
  `SDLTextWriteScaleMM` through the same draw hook used by the runtime,
  proving text x/y, mode, alpha, font style, formatted wide text, and the
  ordinary HUD `scaleAdjustment` versus menu/MM `scaleAdjustmentMM`
  propagation before portable text point-size selection.
- `test_draw_text_hud_bridge_contract` now drives `_DrawText` itself,
  proving the shared HUD text bridge: formatted narrow text to wide text,
  packed color alpha to text alpha, integer-truncated x/y, fixed tint/mode/font
  style, `scale * 3.0f`, and the null-format no-draw guard.
- Full overlay P1 score-panel SCB uses resident sprite `40` at
  `48/36/528/240`.
- Full overlay P1 item-panel SCB uses the `charStuff[0]` resident sprite at
  `44/256/236/448`.
- At `960x540`, the same PDB-backed owner/scaler produces P1 item text `3`
  at `211/414` and the item-panel SCB at `49/288/265/504` after the
  retail `sprite_DisplaySprite` integer call boundary.
- The item-panel destination is source-derived, not screenshot-derived:
  `game_DrawItems` loads authored width/height `192.0f` at
  `0x1400A8293` / `0x1400A829E`, sprite anchor `44.0f,32.0f` at
  `0x1400A82B2` / `0x1400A82BE`, calls
  `setPivotPositionAndFixScale` at `0x1400A82D6`, and the pivot-6
  `scaleAdjustment == 1.125` path yields `49/288/265/504`.
- That same `960x540` item-panel SCB is rendered by `_RenderSprite` through
  `_DrawTexture` with destination `49/288/265/504`, source `0/0/128/128`,
  and alpha `200`, matching the installed restored `a_detonator.tga` material size.
- The `960x540` item-panel path is also asserted across the PDB-backed
  `charStuff` resident icon slots: detonator `45`, bolt `46`, battery `47`,
  shield `48`, the Maul/Plo clamp cases, and the model `>=9` clamp to index
  `9`. Each case keeps the same `49/288/265/504` `sprite_DisplaySprite`
  rectangle and `_RenderSprite` destination. The staged installed item
  materials use the shipped source rect `0/0/128/128`, clut `8`, and
  alpha `200`.
- Continue icons use resident sprite `49` at `274/44/328/98` and
  `312/44/366/98`.
- Continue wrap layout for six remaining credits, including first-row icon
  `217/44/271/98` and wrapped sixth icon `293/82/347/136`, both using
  resident sprite `49`.
- Continue-credit visual state is asserted from the `game_DisplayOverlay`
  source order: normal credits use clut `8`, the six-credit first-row icon
  publishes alpha pad `10`, the wrapped sixth icon publishes alpha pad `0`,
  and a low-energy final credit at `312/44/366/98` switches to clut `2` with
  glow-added alpha pad `3` in the deterministic overlay-owner fixture.
- Continue-credit render source is now asserted through `_RenderSprite`:
  installed `a_credit.tga` is `128x128`, and the first-row, wrapped sixth, and
  low-energy final icons all publish source rect `0/0/128/128` while retaining
  the PDB-authored `scaleAdjustment * 54.0f` destination size.
- P1 life and Force bar final draw rectangles through `_DrawTexture`:
  `211/151/527/164` and `211/175/527/188`.
- Full overlay P2 score text `0009001` at `136/60`, scale `3.24`.
- Full overlay P2 item text `4` at `548/368`, scale `1.8`.
- Full overlay P2 score-panel uses resident sprite `40` at
  `592/36/112/240`.
- Full overlay P2 item-panel uses the `charStuff[1]` resident sprite at
  `404/256/596/448`.
- P2 life and Force bar final draw rectangles through `_DrawTexture`:
  `113/151/429/164` and `113/175/429/188`.
- At 960x540, `getScaleAdjustment` is exactly `ScreenHeight / 480 = 1.125`.
  Full-overlay P1 score panel follows the `game_DrawScore` float-scale then
  `(int)` truncation path into `sprite_DisplaySprite`, yielding SCB rectangle
  `54/40/594/269`, score text `0000042` at `238/67`, `_RenderSprite`
  destination `54/40/594/269`, installed `a_meter_main.tga` source rect
  `0/0/1024/512`, clut `8`, and alpha `200`.
- The same 960x540 P1 owner path asserts `_AddLifeTile2D` bars at
  `237/169/592/183` and `237/196/592/210`; these come from the recovered
  `158.0f * 2.0f * scaleAdjustment` width and
  `scaleAdjustment * 12.0f + 1.0f` height constants.
- At 960x540, full-overlay item icons are constrained by `game_DrawItems`
  through `setPivotPositionAndFixScale(..., pivot 6)` to `49/288/265/504`
  with the restored `128x128` TGA source rect and alpha `200`. The
  item-material switch still asserts every recovered `charStuff` mapping for
  Obi-Wan, Qui-Gon, Mace, Plo, and the extra-character aliases, preventing
  oversized grenade/item regressions.
- At 960x540, full-overlay P2 is now asserted through the same recovered
  owner chain. `game_DrawScore(1)` uses pivot `2`, then the authored negative
  width mirror, yielding SCB rectangle `906/40/366/269`, score text
  `0009001` at `393/67`, `_RenderSprite` destination `366/40/906/269`,
  flipped source rect `1024/0/0/512`, clut `8`, and alpha `200`.
- The 960x540 mirrored P2 `_AddLifeTile2D` bars are asserted at
  `366/169/722/183` and `366/196/722/210` from the same
  `158.0f * 2.0f * scaleAdjustment` and
  `scaleAdjustment * 12.0f + 1.0f` constants.
- The 960x540 P2 item owner uses `setPivotPositionAndFixScale(..., pivot 8)`
  to publish item text `4` at `856/414` and the bolt icon SCB/render
  rectangle `694/288/910/504`, staged `0/0/128/128` source rect,
  `128x128` material, clut `8`, and alpha `200`.
- Compact overlay P1 score anchor `48/62`, left aligned.
- Compact overlay P2 score anchor `592/62`, right aligned.
- The installed compact-overlay path (`OptionStruct.overlayMode == 1`) is
  now asserted at both `640x480` and `960x540` through
  `game_DisplayOverlay`, not only the helper leaf. At `960x540`, the branch
  publishes P1 score text `0000042` at `54/69`, item text `3` at
  `211/414`, and the `a_detonator` SCB/render rectangle
  `49/288/265/504`, source `0/0/128/128`, clut `8`, and alpha `200`.
- Current installed proof trace `hud_full_sweep_item128_core_960.log`
  records the staged installed exe loading `fed.fbx` and publishing the full-overlay HUD
  owners at `960x540`: `screen_draw[2]` owner `score-panel` uses
  `a_meter_main.tga`, destination `54/40/594/269`, source `0/0/1024/512`,
  alpha `200`; `screen_draw[3]` owner `item-panel` uses
  `a_detonator.tga`, destination `49/288/265/504`, source `0/0/128/128`,
  alpha `200`; score text is `0000000` at `238/67`, scale `3.240`; item
  text is `0` at `211/414`, scale `1.800`.
- Installed proof trace `jpb_hud_expanded_re_installed_trace.txt` records the
  same staged-exe full-overlay owner publication after the expanded retail
  trace pass: `fed.fbx` loaded from the installed path, framebuffer
  `960x540`, `a_meter_main.tga` at `54/40/594/269`, `a_detonator.tga` at
  `49/288/265/504`, score text at `238/67`, and item text at `211/414`.
- Current installed proof trace `hud_full_sweep_item128_core_960.log`
  records the same exact installed path after material-dimension logging:
  `a_meter_main.tga` publishes `material=1024x512`, `dst=54/40/594/269`,
  and source `0/0/1024/512`; `a_detonator.tga` publishes
  `material=128x128`, `dst=49/288/265/504`, source `0/0/128/128`, and
  alpha `200`.
- Matching visual proof frame
  `jpb_hud_item128_proofs_installed.png` was captured headless from
  `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe` with
  `fed.fbx`, framebuffer `960x540`, and overlay mode `2`.
- Runtime trace owner labels now classify `_DrawTile2D` life/Force bar draws
  through the recovered `_AddLifeTile2D` no-source green/blue bar colors as
  `meter-bar`, while projected `_AddLifeTile` player-HUD tile submissions are
  labeled `player-hud-tile`.
  They also classify resident sprite `49` / `a_credit.tga` as
  `continue-credit`. This keeps live installed traces aligned with the
  recovered `game_DrawScore` and `game_DisplayOverlay` owner chain instead of
  hiding bars as `other`.
- Current installed owner-label trace `jpb_hud_owner_labels_installed_trace.txt`
  records `fed.fbx` loaded from the exact installed path at `960x540`.
  `screen_draw[0]` and `screen_draw[1]` are now labeled `meter-bar` with
  rectangles `237/169/592/183` and `237/196/592/210`; `screen_draw[2]`
  remains `score-panel` at `54/40/594/269`; `screen_draw[3]` remains
  `item-panel` at `49/288/265/504`, source `0/0/128/128`,
  `material=128x128`.
- Current installed core HUD validator
  `jpb_hud_core_960_validated_installed_trace.txt` was captured from the
  exact staged executable with `--validate-hud-core`, `fed.fbx`,
  framebuffer `960x540`, and overlay mode `2`. It gates the same
  source-backed owner chain at the CLI boundary: meter bars
  `237/169/592/183` and `237/196/592/210`, score-panel
  `a_meter_main.tga` at `54/40/594/269` with source `0/0/1024/512`,
  item-panel `a_detonator.tga` at `49/288/265/504` with source `0/0/128/128`, score text `0000000` at `238/67`, and item text `0`
  at `211/414`. The gate permits recovered fade alpha variation but
  requires nonzero composited screen and text pixels, zero draw drops, and
  exact owner geometry/material/source/text anchors.
- Current installed RE owner sweep
  `jpb_hud_re_core_sweep_installed_trace.txt` records the same exact
  installed exe hash at `960x540` after the screen-poly capture expansion.
  It loads `fed.fbx`, runs `180` gameplay frames, keeps
  `screen_poly=28/0/0`, and emits every captured `_StartPoly` payload through
  `screen_poly[...]` rows with `dropped=0`. The live HUD draw stream records
  the staged item-panel contract as `owner=item-panel`,
  `dst=49/288/265/504`, `src=yes:0/0/128/128`, `material=128x128`, and
  alpha `200`.
- Runtime trace owner labels now classify additional non-core HUD owners from
  recovered draw inputs: `player_DrawDamageTracker` textureless
  `0x7ffc*d400/c000` rectangles as `damage-tracker`, `ai_Kadu` textureless
  `0x7fff4010` / `0x7f1040ff` rectangles as `kadu-race-bar`, and
  `enemy_Radar` translucent/marker rectangles as `enemy-radar`. The same
  `enemy-radar` predicate backs the headless `--validate-radar` gate so
  validation proves radar-owned draw submission instead of depending on an
  unrelated player life-tile side effect.
- `test_overlay_hud_gate_boundaries` asserts negative HUD gates from the
  same PDB-backed owners: overlay mode `0` returns before score/item text,
  invalid level `13` publishes no score or item SCB, level `11` publishes no
  item SCB, and screenshot suppression preserves the recovered score/item
  positions while dimming text alpha to `113` and sprite pad to `175`.
- The same gate-boundary fixture now renders its score-visible full-overlay
  item and continue-credit SCBs through `_RenderSprite`, proving the installed
  resident item `128x128` source rect and continue-credit `128x128` source rect
  remain intact while the owner/gate logic toggles visibility.
- Rescue/counter branch uses resident sprite `43` at `224/256/416/448`,
  text `7` at `368/368`.
- The same rescue/counter branch now asserts the render contract through
  `_RenderSprite`: sprite `43` uses clut `8`, alpha `200`, destination
  `224/256/416/448`, full installed `a_pilot.tga` source rect `0/0/512/512`,
  and `_DrawTexture` alpha `200`.
- When `GameStruct.CurrentLevel == 3`, the rescue/counter branch switches to
  resident sprite `44` (`a_maiden.tga`) at the same `224/256/416/448`
  rectangle, with clut `8`, alpha `200`, full source rect `0/0/512/512`, and
  counter text `5` at `368/368`.
- `test_level_theed` now asserts the source-owner chain from `level_Theed`
  into `game_DisplayOverlay`: at `640x480`, Theed sets `0x01000000`, then
  the rescue/counter HUD publishes `a_maiden.tga` at `224/256/416/448` and
  counter text `5` at `368/368`; at `960x540`, the same pivot-7 owner
  publishes `a_maiden.tga` at `372/288/588/504` and text `5` at `534/414`.
- Level-11 pilot counter branches: single digit `7` at `368/368`; two digit
  `11` at `377/371`.
- Level-11 pilot counter sprite state is asserted for both one- and two-digit
  text paths: resident sprite `43`, rectangle `224/256/416/448`, clut `8`,
  and alpha `200`.
- Direct `_AddLifeTile2D` P1 and mirrored P2 rectangles are asserted,
  including P1 life/Force `100/200/258/213` and `100/224/175/237`, and P2
  life/Force `342/200/500/213` and `424/224/500/237`.
- Direct `_AddBar` forwards exact x/y/width/height and alpha-forced color
  `0x7f123456`.
- `level_CountDown` timer HUD text is asserted at `231/328` (`002`, scale
  `3.0`) and the `sec` label at `353/359` (scale `1.5`) for a 640x480 HUD
  surface.
- At 960x540, the same `level_CountDown` timer branch uses
  `setPivotPositionAndFixScale(..., pivot 7)` and
  `scaleAdjustment == 1.125`: timer text `002` at `379/369`, scale `3.0`,
  and `sec` at `517/403`, scale `1.5`.
- `level_CountDown` objective HUD text is asserted at `48/200` (`003`, scale
  `3.0`) and the objective label draw at `48/260` (scale `1.5`) through the
  same PDB owner. The text-hook fixture observes the first code unit of the
  cast `allText[427]` label while preserving the exact draw position/scale.
- At 960x540, the objective branch uses pivot `0` and publishes `003` at
  `54/225`, scale `3.0`, and the objective label at `54/292`, scale `1.5`.
- Installed branch gate `--validate-hud-countdown-kill` captures the live
  level-19 kill-counter branch after recovered state reset: text `015` at
  `54/225`, scale `3.0`, alpha `127`, and the `allText[427]` label first
  code unit `t` at `54/292`, scale `1.5`, alpha `127`.
- Installed branch gate `--validate-hud-countdown-kill-1080` gates the same
  owner at `1920x1080`: text `015` at `108/450` and label `t` at `108/585`,
  preserving the same scales and alpha.
- `level_CountDown` result banners are asserted through the exact state
  machine: global-bit success and timer-expiry failure trigger on one frame
  and draw centered result text at `320/200`, mode `2`, on the next frame.
- At 960x540, the same success and failure result-banner state machines draw
  centered result text at `480/225`, mode `2`, scale `1.0`.
- Installed branch gate `--validate-hud-countdown-success` drives the recovered
  success state machine and gates `allText[435]` text `SUCCESS` at `480/225`,
  mode `2`, scale `1.0`, alpha `128`.
- Installed branch gate `--validate-hud-countdown-success-1080` gates
  `SUCCESS` at `960/450`, mode `2`, scale `1.0`, alpha `128`.
- Installed branch gate `--validate-hud-countdown-fail` drives the recovered
  timer-expiry state machine and gates `allText[436]` text `FAILED` at
  `480/225`, mode `2`, scale `1.0`, alpha `128`.
- Installed branch gate `--validate-hud-countdown-fail-1080` gates `FAILED`
  at `960/450`, mode `2`, scale `1.0`, alpha `128`.
- `level_Hangar` timer HUD text is asserted from its authored PDB owner
  coordinates `timerX=-89/timerY=267` and `labelX=33/labelY=236` after
  `setPivotPositionAndFixScale(..., pivot 7)` at 640x480. The captured text
  calls are mode `0` text: `400` at `231/213`, scale `3.0`, and `sec` at
  `353/244`, scale `1.5`; the one-second-decrement path asserts `399` at the
  same anchor.
- At 960x540, `level_Hangar` timer/label anchors are asserted at `379/239`
  and `517/274` from the same pivot-7 owner math and `scaleAdjustment`
  source.
- `level_Arena` winner-message HUD text is asserted at the direct
  PDB-authored anchor `(ScreenWidth >> 1) - 0x80` /
  `(ScreenHeight >> 1) - 0x80`, which is `192/112` at 640x480. The P1,
  P2, and draw branches are asserted with distinct first code units from the
  recovered `(char *)(void *)allText[...]` boundary (`O`, `T`, and `D`) at
  scale `3.0`, mode `0`, tint `11`; the first public draw after the owner
  sets `arenaDelay=0xff` publishes fade alpha `254`.
- At 960x540, `level_Arena` uses the same direct screen expression rather
  than `scaleAdjustment`, yielding winner-message anchor `352/142`. P1, P2,
  and draw branches are asserted at that anchor with the same first-code-unit
  text boundary, scale `3.0`, mode `0`, tint `11`, and alpha `254`.
- `menu_drawBigNums(42, 4, 100, 50, 9, 8, 7)` publishes zero-padded PSX digit
  textures `0xb5`, `0xb5`, `0xb9`, `0xb7` at x `100`, `105`, `110`, `115`,
  y `50`, with raw width/height inputs `0/0`, alpha `0xff`, and RGB
  `9/8/7`.
- `level_Mini4` publishes the remaining-kill count `075` through
  `menu_drawBigNums`: textures `0xb6`, `0xb5`, `0xb5` at `240/184`,
  `245/184`, and `250/184`, raw width/height inputs `0/0`, alpha `0xff`,
  and RGB `0x80/0x80/0x80`.
- The same `level_Mini4` PSX-digit HUD is asserted as a fixed-position
  owner at `960x540` as well: it still publishes `075` at `240/184`,
  `245/184`, and `250/184`, with raw width/height `0/0`, alpha `0xff`, and
  RGB `0x80/0x80/0x80`, matching the retail owner inputs rather than
  applying `scaleAdjustment`.
- `src\reconstructed\original\overlay.c` currently contains PDB-named
  generated shells only; no live size/position owner exists there to assert.

Relevant test locations:

- `tests\test_game.c:707`
- `tests\test_game.c:728`
- `tests\test_game.c:736`
- `tests\test_game.c:747`
- `tests\test_game.c:769`
- `tests\test_game.c:795`
- `tests\test_game.c:803`
- `tests\test_game.c:807`
- `tests\test_game.c:818`
- `tests\test_game.c:838`
- `tests\test_game.c:851`
- `tests\test_game.c:867`
- `tests\test_game.c:870`
- `tests\test_game.c:882`
- `tests\test_game.c:894`
- `tests\test_game.c:901`
- `tests\test_game.c:933`
- `tests\test_game.c:958`
- `tests\test_game.c:983`

`tests\test_player.c` covers player-owned HUD-adjacent overlays:

- Projection-dependent `_AddLifeTile` health and Force tile owner under a
  controlled camera, including left cap x `-22.55`, bar start x `-18.75`,
  right cap x `19.55`, health row y `-9`, Force row y `-3`, and bar widths
  `37.5` and `30.0`. At the controlled projection depth `500`, the owner
  publishes cap color `0xa5808080`, health color `0xbd108010`, and Force
  color `0xbd101080` through the exact `color_interpolate` path.
- `_AddLifeTile` force-shield suppression of the health row is asserted:
  `forceFlags & 0x10` leaves only the three Force-row tiles at y `-3`, with
  the same `30.0` Force width, cap/Force colors `0xa5808080` /
  `0xbd101080`, and `0.0001` HUD z for player `0`.
- `_AddLifeTile` non-player health-only row is asserted through the exact
  `game_gSetMaxEnergy` / `game_gSetEnergy` scaling path: 50/100 energy for
  player index `2` produces integer-scaled width `18.0`, y `-9`, projection
  depth `500`, world z `430.0`, cap color `0xa5808080`, and health color
  `0xbd108010`.
- `_AddLifeTile` local suppress/cull gates are asserted for full-overlay
  world players, overlay mode `0`, screenshot mode, and behind-camera
  projection.
- Compact P1 damage overlay rectangle `50/127/70/140`, color
  `0x7ffcd400` after `player_DrawDamageTracker` interpolates total `20.0`.
- Full-overlay P1 damage bar rectangle `211/127/231/140`, color
  `0x7ffcd400`.
- Full-overlay mirrored P2 damage bar rectangle `399/127/429/140`, color
  `0x7ffcc000` from total `30.0`.
- At 960x540, `player_DrawDamageTracker` now has scaled assertions through
  the same recovered `setPivotPositionAndFixScale` /
  `setPositionOffPivot` owner chain: compact P1 damage rectangle
  `56/142/78/157`; full-overlay P1 rectangle `237/142/259/157`; and
  mirrored full-overlay P2 rectangle `688/142/722/157`.
- Those 960x540 damage rectangles preserve the recovered color interpolation
  and timer behavior: P1 total `20.0` publishes color `0x7ffcd400`, P2 total
  `30.0` publishes color `0x7ffcc000`, and both decay by `0.75` after the
  draw when `gGlobalFrameRate != 0` and the player is not force-suppressed.
- `player_gProcessPlayers` now asserts the previously tracked debug-HUD
  boundary. At `DebugLevel == 2`, P1 publishes a verbose label at
  `18/136/50`, scale `1.0`, color `0xff8090a0`, while mirrored P2 publishes
  at `487/136/519`; both use the installed retail format
  `%d-%s ai %d\nt-%s\n%s %s %s %s`. At `DebugLevel > 2`, enemy-backed
  labels publish at the actor physics position (`100/200/300` and
  `400/500/600` in the fixture) through retail formats `%d-%s %s` and
  `%d-%s`.

Relevant test locations:

- `tests\test_player.c:1778`
- `tests\test_player.c:1783`
- `tests\test_player.c:1786`
- `tests\test_player.c:1788`
- `tests\test_player.c:1793`
- `tests\test_player.c:1849`
- `tests\test_player.c:1871`
- `tests\test_player.c:1876`
- `tests\test_player.c:2045`

`tests\test_boss_vehicle.c` covers boss-owned HUD race bars:

- `ai_Kadu` P1 Kadu race bar through `_AddBar`: x/y/width/height
  `-30/428/64/12`, color `0x7fff4010`.
- `ai_Kadu` P2 mirrored Kadu race bar through `_AddBar`: x/y/width/height
  `606/428/64/12`, color `0x7f1040ff`.
- At 960x540, the same `ai_Kadu` owner uses
  `setPivotPositionAndFixScale(..., pivot 7)` with authored base
  `x=-200/+200`, `y=40`, `width=300`, and `height=12`. The asserted
  published bars are P1 `86/481/64/13` and P2 `809/481/64/13`, preserving
  the recovered speed clamp (`speed * 4 == 64`) and colors `0x7fff4010` /
  `0x7f1040ff`.

Relevant test locations:

- `tests\test_boss_vehicle.c:362`
- `tests\test_boss_vehicle.c:373`
- `tests\test_boss_vehicle.c:383`
- `tests\test_boss_vehicle.c:396`

`tests\test_projection.c` covers scene-owned HUD arrow projection:

- `playerOffScreenArrow` emits one four-vertex screen poly for an offscreen
  player, marks `playeronscreen[0] == 0`, and keeps every vertex at z
  `0.0001`. The controlled-camera fixture asserts the exact authored
  triangle geometry: left outer `73/572`, transparent center `56/565`,
  clipped point `24/565`, and right outer `73/558`. It also asserts the
  outer opaque color `0xff4080ff`, center color `0`, and interpolated point
  color `0xdae3ecff`.
- At `960x540`, the same PDB-backed `TransformPoints` -> `cliptoscreen`
  -> screen-center path asserts the signed-quarter and `flexmul` arrow math:
  left outer `74/315`, transparent center `56/308`, clipped point `24/310`,
  and right outer `72/301`, with every vertex still at z `0.0001` and the
  same opaque/transparent/interpolated colors.

Relevant test location:

- `tests\test_projection.c:151`
- `tests\test_projection.c:173`

`tests\test_enemy.c` covers the enemy debug-radar HUD owner:

- At `640x480`, `enemy_Radar` draws the translucent background at
  `284/31/355/137`, alpha `0x9f`, layer `0.0001`, no source rect, using
  `transHandle`. It draws the player marker at `318/82/320/85`, white
  `0xffffffff`, then owner-type `2` at the same `318/82/320/85` rectangle
  with red `0xffff2020`, and owner-type `3` with green `0xff20ff20`.
- At `960x540`, the same PDB-backed scale constants draw the background at
  `426/34/532/154`, player marker at `478/93/480/95`, and owner-type
  markers at the same `478/93/480/95` rectangle with the same white/red/green
  colors and alpha values.
- Current installed radar sweep
  `jpb_hud_re_radar_sweep_installed_trace.txt` was captured from the exact
  installed path with `--validate-radar` and exits cleanly. Its runtime draw
  stream records the recovered `enemy_Radar` background as
  `owner=enemy-radar`, `dst=426/34/532/154`, `color=0/0/0/159`, and marker
  examples at `478/93/480/95`, `507/90/509/92`, and `475/83/477/85` with
  the recovered white/red/green marker colors. The same trace also records
  `screen_poly=1/0/0`, proving the expanded screen-poly capture retained its
  no-drop invariant in the radar validation path.

Relevant test location:

- `tests\test_enemy.c:2738`

## Verification commands

Focused HUD checks:

```powershell
build\Release\jpb_game_tests.exe
build\Release\jpb_sprite_tests.exe
build\Release\jpb_io_filesys_tests.exe
build\Release\jpb_player_tests.exe
build\Release\jpb_boss_vehicle_tests.exe
build\Release\jpb_projection_tests.exe
build\Release\jpb_enemy_tests.exe
```

Installed/build hash check:

```powershell
Get-FileHash 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' -Algorithm SHA256
Get-FileHash 'build\Release\jpb_pc_game.exe' -Algorithm SHA256
```

Legacy installed HUD validator sweep (superseded):

- This older installed sweep is retained only as history. It predates the
  retail `getScaleAdjustment == ScreenHeight / 1080.0f` correction and is
  superseded by the current hash and 42 installed gates in the
  "Current retail-scale HUD correction" section above.
- Runtime screen-texture compositing now applies the `_DrawTexture` CVECTOR
  alpha to sampled TGA alpha on the same nonzero-color modulation path used by
  `_RenderSprite`. This preserves opaque mask assets while honoring HUD SCB
  fade owners such as `game_DrawItems` / `a_detonator.tga`, whose PDB-traced
  direct owner is still `99/576`, size `432/432` at `1920x1080`, but whose
  visible contribution is correctly dimmed by the owner alpha.
- The installed core, P2 core, compact-overlay, continue, and rescue HUD gates
  now require resident texture alpha modulation counters. The `screen_alpha`
  tuple is `total/item/credit/rescue`, so the resident item, continue-credit,
  and rescue-counter HUD textures cannot regress to ignoring the PDB-traced SCB
  alpha. Current installed 1080 traces report `screen_alpha=375448/90313/0/0`
  for P1 full HUD, `750896/180626/0/0` for P1+P2 full HUD,
  `90313/90313/0/0` for compact HUD,
  `380413/90313/4965/0` for continue credits, and
  `465761/90313/0/90313` for the rescue counter. The level-11 pilot-counter
  branch now reports `screen_alpha=93936/0/0/22527` at `960x540` and
  `375448/0/0/90313` at `1920x1080`, proving that `a_pilot` participates in
  the same rescue/counter alpha ownership bucket as `a_maiden`.
- The installed item HUD footprint is now exact-gated, not only nonzero.
  The gate is tied to the recovered `game_DrawItems` `192.0f` width/height
  path and installed resident `128x128` item materials after the retail-scale
  correction. One visible item icon must contribute `4381` item
  alpha-modulated pixels at `960x540` and `17489` at `1920x1080`; the P1+P2
  full HUD must contribute `8762` and `34975`; the level-11 pilot-counter
  branch must contribute `0` item pixels. These assertions are present in
  core, P2 core, compact/lifetile, continue, rescue, and pilot-counter
  validators.
- Exact HUD text validators now also require nonzero per-draw rendered pixels,
  not just a matching submitted text record. `JPBGameRuntimeTextDraw` records
  `compositePixels` from the TrueType/fallback compositor, and
  `pc_runtime_has_text_draw*` rejects exact text owners that publish the right
  PDB-traced text, position, scale, and alpha but render no visible pixels.
  The installed trace printer now emits each draw's `pixels=` count so the
  visibility gate is inspectable per HUD owner. Current retained exact-path
  traces include `hud_turn_core_texttrace_960.log`,
  `hud_turn_core_texttrace_1080.log`,
  `hud_turn_lifetile_texttrace_1080.log`, and
  `hud_turn_rescue_texttrace_1080.log`. Examples from those traces:
  score text `0000000` at `238/67`, scale `3.24`, `pixels=10276`;
  item text `0` at `211/414`, scale `1.8`, `pixels=401`; 1080 score
  text at `477/135`, `pixels=43267`; 1080 item text at `423/828`,
  `pixels=1832`; rescue count `5` at `1068/828`, `pixels=1678`.
- `--validate-hud-core` gates the P1 full-overlay score panel, item panel,
  life/Force bars, score text, and item text at `960x540` with exact draw
  counts and installed frame alpha: panels `alpha=2`, bars/text `alpha=1`.
  The strict gate also publishes the `sprite_DisplaySprite` owners directly:
  score resident type `40`, `a_meter_main.tga`, `54/40`, size `540/229`,
  clut `8`, material `1024x512`; item resident type `45`,
  `a_detonator.tga`, `49/288`, size `216/216`, clut `8`, material `128x128`.
- `--validate-hud-core-1080` gates the same P1 full-overlay owner path at
  the installed maximized framebuffer size, `1920x1080`, using the exact
  live `sprite_DisplaySprite` trace and queued draw values. The score panel
  is `108/81/1188/540`, source `0/0/1024/512`, alpha `2`; the item panel is
  `99/576/531/1008`, source `0/0/128/128`, alpha `2`; the life/Force bars
  are `474/339/1185/367` and `474/393/1185/421`; score text `0000000` is
  `477/135`, scale `3.24`, and item text `0` is `423/828`, scale `1.8`.
  The direct owner gate also requires score type `40`, `a_meter_main.tga`,
  `108/81`, size `1080/459`, clut `8`, material `1024x512`; and item type
  `45`, `a_detonator.tga`, `99/576`, size `432/432`, clut `8`, material `128x128`.
  The current installed damage-HUD sweep independently confirms the same
  grenade material path: `a_detonator.tga` is `material=128x128` at both
  `49/288/265/504` for `960x540` and `99/576/531/1008` for `1920x1080`.
- `--validate-hud-p2-core` gates the mirrored P2 score panel
  `366/40/906/269`, flipped source `1024/0/0/512`, P2 bars
  `366/169/722/183` and `366/196/722/210`, bolt icon
  `694/288/910/504`, score text `0009001` at `393/67`, and item text `4`
  at `856/414`, all with exact counts and alpha.
  The direct `sprite_DisplaySprite` owner gate also requires P2 score
  type `40` at `906/40`, size `-540/229`, and P2 bolt type `46` at
  `694/288`, size `216/216`, clut `8`, material `128x128`.
- `--validate-hud-p2-core-1080` gates the mirrored P2 owner path at
  `1920x1080`: score panel `732/81/1812/540`, flipped source
  `1024/0/0/512`; P2 bars `734/339/1445/367` and
  `734/393/1445/421`; bolt icon `1389/576/1821/1008`; score text
  `0009001` at `786/135`; and item text `4` at `1713/828`.
  The direct owner gate also requires P2 score type `40` at `1812/81`,
  size `-1080/459`, and P2 bolt type `46` at `1389/576`, size
  `432/432`, clut `8`, material `128x128`.
- `--validate-hud-continue` gates the installed `a_credit.tga` material
  source `0/0/128/128` for every six-credit draw: `364/49/424/109`
  with `alpha=110`, `406/49/466/109`, `449/49/509/109`,
  `492/49/552/109`, `535/49/595/109`, and wrapped
  `449/92/509/152` with `alpha=0`.
  The same gate now also requires the six direct credit
  `sprite_DisplaySprite` owner calls: type `49`, `a_credit.tga`,
  `60/60`, clut `8`, material `128x128` at `364/49`, `406/49`,
  `449/49`, `492/49`, `535/49`, and `449/92`.
- `--validate-hud-continue-1080` gates the same six-credit owner path at
  `1920x1080`: draw rectangles `728/99/849/220`,
  `813/99/934/220`, `899/99/1020/220`, `984/99/1105/220`,
  `1070/99/1191/220`, and wrapped `899/184/1020/305`.
  Direct owner calls require type `49`, `a_credit.tga`, size `121/121`,
  clut `8`, material `128x128`, at the same top-lefts.
- `--validate-hud-rescue` gates the installed `a_maiden.tga` material
  source `0/0/512/512`, rescue counter `372/288/588/504`, and text `5`
  at `534/414`, with exact draw/text alpha.
  It also gates the direct `sprite_DisplaySprite` owner call: type `44`,
  `a_maiden.tga`, `372/288`, size `216/216`, clut `8`, material `512x512`.
- `--validate-hud-rescue-1080` gates the maiden/counter owner at
  `1920x1080`: `a_maiden.tga` draw rectangle `744/576/1176/1008`,
  source `0/0/512/512`, text `5` at `1068/828`, and the direct
  `sprite_DisplaySprite` type `44` call at `744/576`, size `432/432`,
  clut `8`, material `512x512`.
- `--validate-hud-pilot-counter` gates the sibling level-11
  `game_DisplayOverlay` pilot-counter owner at `960x540`: `a_pilot.tga`
  draw rectangle `372/288/588/504`, source `0/0/512/512`, text `7` at
  `534/414`, and the direct `sprite_DisplaySprite` type `43` call at
  `372/288`, size `216/216`, clut `8`, material `512x512`.
- `--validate-hud-pilot-counter-1080` gates the same level-11
  pilot-counter owner at `1920x1080`: `a_pilot.tga` draw rectangle
  `744/576/1176/1008`, source `0/0/512/512`, text `7` at `1068/828`,
  and the direct `sprite_DisplaySprite` type `43` call at `744/576`,
  size `432/432`, clut `8`, material `512x512`.
- `--validate-hud-countdown` gates the level-countdown owner at
  `960x540`: text `035` at `379/369`, text `sec` at `517/403`, scale
  `3.0` / `1.5`, alpha `127`.
- `--validate-hud-countdown-1080` gates the same `level_CountDown`
  timer owner at `1920x1080`: text `035` at `759/738`, text `sec` at
  `1034/807`, scale `3.0` / `1.5`, alpha `127`.
- `--validate-hud-countdown-kill` gates the non-timer
  `level_CountDown` kill-counter owner at `960x540`: text `015` at
  `54/225`, text `t` at `54/292`, scale `3.0` / `1.5`, alpha `127`.
- `--validate-hud-countdown-kill-1080` gates the same kill-counter owner
  at `1920x1080`: text `015` at `108/450`, text `t` at `108/585`, scale
  `3.0` / `1.5`, alpha `127`.
- `--validate-hud-countdown-success` gates the exact recovered
  `level_CountDown` success state-machine branch at `960x540`: text
  `SUCCESS` at `480/225`, mode `2`, scale `1.0`, alpha `128`.
- `--validate-hud-countdown-success-1080` gates the same success branch at
  `1920x1080`: text `SUCCESS` at `960/450`, mode `2`, scale `1.0`,
  alpha `128`.
- `--validate-hud-countdown-fail` gates the exact recovered
  `level_CountDown` timer-expiry branch at `960x540`: text `FAILED` at
  `480/225`, mode `2`, scale `1.0`, alpha `128`.
- `--validate-hud-countdown-fail-1080` gates the same fail branch at
  `1920x1080`: text `FAILED` at `960/450`, mode `2`, scale `1.0`,
  alpha `128`.
- `--validate-hud-hangar` gates the Hangar timer owner at `960x540`: text
  `400` at `379/239`, text `sec` at `517/274`, scale `3.0` / `1.5`,
  alpha `127`.
- `--validate-hud-hangar-1080` gates the same `level_Hangar` timer owner
  at `1920x1080`: text `400` at `759/479`, text `sec` at `1034/549`,
  scale `3.0` / `1.5`, alpha `127`.
- `--validate-hud-arena` gates the Arena winner text owner at `960x540`:
  text `O` at `352/142`, scale `3.0`, alpha `252`.
- `--validate-hud-arena-1080` gates the direct `level_Arena` winner text
  expression at `1920x1080`: text `O` at `832/412`, scale `3.0`,
  alpha `252`.
- `--validate-hud-mini4` gates raw PSX digit submissions from
  `level_Mini4`: textures `0xb6`, `0xb5`, `0xb5` at `240/184`,
  `242/184`, and `244/184`, raw size `0/0`, alpha `0xff`, RGB
  `0x80/0x80/0x80`.
- `--validate-hud-mini4-1080` gates the same `level_Mini4` /
  `game_DrawBigNum` digit owner at `1920x1080`: the raw PSX submissions
  remain fixed at textures `0xb6`, `0xb5`, `0xb5`, positions `240/184`,
  `242/184`, and `244/184`, raw size `0/0`, alpha `0xff`, RGB
  `0x80/0x80/0x80`; the current renderer bridge also publishes matching
  text digits `1`, `0`, `0` at `240/184`, `242/184`, and `244/184`,
  scale `2.0`, alpha `255`.
- `--validate-hud-damage` now gates the exact full-overlay
  `player_DrawDamageTracker` rectangle at `237/142/259/157`, color
  `0x7ffcd400`, at `960x540`.
- `--validate-hud-damage-1080` gates the same
  `player_DrawDamageTracker` owner at `1920x1080`: rectangle
  `474/285/519/313`, color `0x7ffcd400`.
- `--validate-hud-damage-compact` gates the exact compact-overlay
  `player_DrawDamageTracker` rectangle at `56/142/78/157`, color
  `0x7ffcd400`, at `960x540`.
- `--validate-hud-damage-compact-1080` gates the same compact owner at
  `1920x1080`: rectangle `112/285/157/313`, color `0x7ffcd400`.
- `--validate-hud-damage-p2` gates the mirrored full-overlay P2
  `player_DrawDamageTracker` owner at `960x540`: rectangle
  `688/142/722/157`, color `0x7ffcc000`, with an active installed P2
  runtime.
- `--validate-hud-damage-p2-1080` gates the same mirrored P2 owner at
  `1920x1080`: rectangle `1377/285/1445/313`, color `0x7ffcc000`,
  with an active installed P2 runtime.
- `--validate-hud-damage-p2-compact` gates the compact-overlay P2
  `player_DrawDamageTracker` owner at `960x540`: rectangle
  `872/142/906/157`, color `0x7ffcc000`, with an active installed P2
  runtime. The value was first captured from the staged installed trace
  `hud_damage_p2_compact_trace_960.log`, then pinned by the passing
  installed gate `hud_damage_p2_compact_pass_960.log`.
- `--validate-hud-damage-p2-compact-1080` gates the same compact P2 owner
  at `1920x1080`: rectangle `1744/285/1812/313`, color `0x7ffcc000`,
  with an active installed P2 runtime. The value was first captured from
  the staged installed trace `hud_damage_p2_compact_trace_1080.log`, then
  pinned by the passing installed gate `hud_damage_p2_compact_pass_1080.log`.
- `--validate-hud-kadu` now gates the exact Mini2 installed
  `ai_Kadu` bars at `960x540`: P1 `86/481/150/494`, color
  `0x7fff4010`, and P2 `773/481/873/494`, color `0x7f1040ff`.
  The gate matches the recovered `_AddBar` speed clamp (`speed * 4 == 64`)
  and the P2 owner boundary asserted by `tests\test_boss_vehicle.c`:
  x/y/width/height `809/481/64/13`.
- `--validate-hud-kadu-1080` gates the same Mini2 frame-10 installed
  `ai_Kadu` race-bar owners at `1920x1080`: P1
  `172/963/236/990`, color `0x7fff4010`, and P2
  `1647/963/1747/990`, color `0x7f1040ff`.
- `--validate-hud-offscreen` now gates the exact FED frame-1 installed
  `playerOffScreenArrow` screen poly at `960x540`: vertices
  `1848/437`, `1863/446`, `1896/449`, and `1846/453`, z `0.0001`,
  colors `0xff3880f8`, `0`, `0xdae2ecfd`, `0xff3880f8`.
- `--validate-hud-offscreen-1080` gates the same seeded
  `playerOffScreenArrow` owner at `1920x1080`: vertices `1848/628`,
  `1863/637`, `1896/639`, and `1846/644`, z `0.0001`, colors
  `0xff3880f8`, `0`, `0xdae2ecfd`, `0xff3880f8`, with no dropped
  screen polys.
- `--validate-hud-lifetile` gates the exact installed compact-overlay
  `_AddLifeTile` owner at `960x540`: health row caps/bar
  `680/195/685/199`, `685/195/738/199`, `738/195/743/199`,
  Force row caps/bar `680/201/685/205`, `685/201/738/205`,
  `738/201/743/205`, with colors `0x4c808080`, `0x57108010`,
  `0x4c808080`, `0x4c808080`, `0x57101080`, `0x4c808080`, six
  total player HUD tile draws, no drops, and compact score/item text
  anchors `0000000` at `54/69` and `0` at `211/414`.
- `--validate-hud-lifetile-1080` gates the same default compact-overlay
  owner path at the installed maximized framebuffer size, `1920x1080`.
  The gate follows `getScaleAdjustment == 1080 / 1080 == 1.0`,
  `game_DrawItems` authored constants `192.0f`, `44.0f`, `32.0f`,
  `144.0f`, and `112.0f`, and `_AddLifeTile`'s compact player-HUD tile
  path. It requires the detonator item SCB/render at `44/856/236/1048`,
  direct `sprite_DisplaySprite` type `45`, `a_detonator.tga`, `44/856`,
  size `192/192`, compact text anchors `0000000` at `48/62` and `0`
  at `188/968`, and health/Force caps/bars at
  `1361/391/1370/398`, `1371/391/1476/398`,
  `1477/391/1486/398`, `1361/403/1370/410`,
  `1371/403/1476/410`, and `1477/403/1486/410`.
- `--validate-hud-lifetile-projected` and
  `--validate-hud-lifetile-projected-1080` gate the exact installed
  full-overlay `_AddLifeTile` projected owner path from frame 2 with
  DebugLevel 3 enabled by the same `player_gProcessPlayers` trace that
  publishes enemy labels. The gates require 15 player-HUD tile submissions,
  zero drops, nonzero composite pixels, and exact destination/color/material
  and layer records for the five health-only projected rows. Baseline
  anchors include `230/1026/238/1032`,
  `237/1026/269/1032`, `267/1026/275/1032`,
  `0/278/4/282`, `5/278/30/282`, and
  `30/278/35/282`; the 1080 gate pins their installed-scaled partners
  `461/2052/477/2064`, `475/2052/538/2064`,
  `535/2052/551/2064`, `0/557/9/564`,
  `10/557/60/564`, and `61/557/71/564`.
- `--validate-hud-debug-labels` gates the exact installed DebugLevel 2
  `scr_debugPrintfXYZ` owner at `960x540`: one captured draw3D text call,
  text `0- ai 0\nt-LAND\n   ` at `22/147/58`, scale `1.0`, color
  `0xff8090a0`, with no draw3D text drops.
- `--validate-hud-debug-labels-1080` gates the same DebugLevel 2
  `scr_debugPrintfXYZ` owner at `1920x1080`: one captured draw3D text
  call, text `0- ai 0\nt-LAND\n   ` at `83/318/155`, scale `1.0`,
  color `0xff8090a0`, with no draw3D text drops.
- `--validate-hud-debug-labels3` gates the installed frame-2 DebugLevel 3
  enemy-label `scr_debugPrintfXYZ` owners at `960x540`: exactly seven
  captured draw3D text calls, no drops, scale `1.0`, color
  `0xff8090a0`, and these PDB-owner world anchors: `2-enemy000 enemy000`
  at `20224/3328/-14848`, `3-enemy001 enemy001` at
  `22157/3328/-14976`, `4-enemy004 enemy004` at
  `22823/3328/-15181`, `5-enemy005 enemy005` at
  `22580/3328/-15386`, `6-enemy008 enemy008` at
  `23271/3328/-14554`, `7-enemy014 enemy014` at
  `22272/3328/-13671`, and `8-enemy129 enemy129` at
  `21492/3328/-14861`.
- `--validate-hud-debug-labels3-1080` gates the same frame-2 DebugLevel 3
  enemy-label owner set at `1920x1080`; the labels are world-space
  `scr_debugPrintfXYZ` owners, so the exact text/world coordinates match the
  `960x540` gate while the installed framebuffer path changes.- `--validate-radar` now gates the exact installed frame-2 960x540
  `enemy_Radar` HUD owner: translucent background `426/34/532/154`,
  color `0x9f000000`; player marker `478/93/480/95`, color
  `0xffffffff`; ownerType-2 enemy marker `492/136/494/138`, color
  `0xffff2020`; and ownerType-3 enemy marker `476/103/478/105`, color
  `0xff20ff20`, rather than only counting broad radar-colored draws.
- `--validate-radar-1080` gates the same frame-2 DebugLevel 3
  `enemy_Radar` owner at `1920x1080`: translucent background
  `852/69/1065/308`, color `0x9f000000`; player marker
  `956/186/960/191`, color `0xffffffff`; ownerType-2 enemy marker
  `970/229/974/234`, color `0xffff2020`; and ownerType-3 enemy marker
  `954/196/958/201`, color `0xff20ff20`.

```powershell
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-core
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-core-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-p2-core
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-p2-core-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 12 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-continue
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 12 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-continue-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-rescue
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-rescue-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-pilot-counter
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-pilot-counter-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-kill
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-kill-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-success
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-success-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-countdown-fail
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-countdown-fail-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-hangar
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-hangar-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 4 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-arena
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 4 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-arena-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload Mini4 --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-mini4
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload Mini4 --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-mini4-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-damage
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-damage-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-damage-compact
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-damage-compact-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-damage-p2
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-damage-p2-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-damage-p2-compact
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-damage-p2-compact-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload Mini2 --frames 10 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-kadu
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload Mini2 --frames 10 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-kadu-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 2 --validate-hud-offscreen
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-hud-offscreen-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-lifetile
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-lifetile-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 960 540 --overlay-mode 1 --validate-hud-debug-labels
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 1 --framebuffer-size 1920 1080 --overlay-mode 1 --validate-hud-debug-labels-1080
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 2 --framebuffer-size 960 540 --overlay-mode 2 --validate-radar
& 'C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe' --headless --quickload fed --frames 2 --framebuffer-size 1920 1080 --overlay-mode 2 --validate-radar-1080
```




