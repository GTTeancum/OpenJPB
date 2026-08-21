# Main Menu Parity Audit

## Scope and Evidence Rules

This ledger covers every state and conditional branch reachable from the ordinary main menu. A passing route smoke proves reachability only. A row is complete only when its command data, entry initialization, input, transitions, audio, and presentation are all supported by the shipped executable or PDB and regression-covered.

Canonical evidence is restricted to the installed PDB, the shipped `game.exe`, and extracted local game data. Online references and visual approximation are not acceptable implementation evidence.

## Reachable Graph

| Route | State | Owner or command stream | Entry and exit | Status |
| --- | ---: | --- | --- | --- |
| Main Menu | `0x00` | `mainMdef*`, `continuemainMdef*` | Root; conditionally includes Continue and Register | **Covered:** exact streams, root stack initialization, automatic-load state, audio/model/global-bit/title setup, rendering, and conditional items are tested |
| New Game overwrite | `0x90` | `newgameconfirmMdef` | Main New Game when `continueAble != 0`; Yes starts new game, No returns | **Covered:** exact stream, prompt rendering, cancel, and conditional bypass are tested |
| Player Count | `0x03` | `titlePlayerCountMdef` | New Game or overwrite Yes; one/two players -> Difficulty | **Covered:** exact stream and executable route tests |
| Difficulty | `0x37` | `difficultyMdef` | Player Count; Jedi/Easy -> internal state `0x04` | **Covered:** exact stream, difficulty mutation, and transition tests |
| Player handoff | `0x04` / `0x05` | internal `playerCountSelectMdef` state | Retail entry immediately resets level and pushes Character Select | **Covered:** missing initializer restored; regression forbids exposing the internal stream in New Game |
| Character Select | `0x0e` | `newMenu_P1CharacterSelect`, `newMenu_P2CharacterSelect` | New Game; confirm -> Level Select, cancel -> Main | **Covered:** direct owners, selection/tabs, saber toggle, ready state, reconnect panel, one/two-player routes, and rendering are tested |
| Level Select | `0x1a` | `menu_drawLevelSelectScreen`, `levelSelectMdef` | Character Select; level confirm starts game, cancel unwinds | **Covered:** exact stream, entry state, four byte-exact movers, texture/depth/draw order, selectors, prompts, preview, fade, confirm, and cancel are tested |
| Continue player count | `0x99` | `titlePlayerCountContinueMdef` | Main Continue; one/two players resume game | **Covered:** exact stream, both triggers, handoff, and settled rendering are tested |
| Training | `0x0c` | `newMenu_Training` | Main Training; selection starts training, cancel returns | **Covered:** direct owner, input, route, and rendering are tested |
| VS player count | `0x9c` | `titlePlayerCountVSMdef` | Main VS via destination `0x9d`; one/two controllers -> VS Character Select | **Covered:** exact stream, player ownership, transitions, and rendering are tested |
| VS Character Select | `0x0d` | `newMenu_P2CharacterSelect(1)` | VS Player Count; confirm starts VS, cancel returns | **Covered; glyph alignment live-approved:** direct owner, two-player state, transitions, exact keyboard-tab positions, Space/Enter acceptance, rendering, and arena handoff are tested; arena handoff remains pending live review |
| Options | `0x0b` | `optionsMdef` | Main Options; branches below, cancel returns | **Covered:** exact stream, bounded retail `slider`, title/audio state, save-before-edit call, transitions, and rendering are tested |
| Video | `0x95` | `videoMdef` | Options Video; apply/cancel returns | **Covered:** exact stream, dynamic valid-mode list, startup selection, live apply, window-mode mapping, return path, and 1080p rendering are tested |
| Audio | `0x10` | `audioMdef`, `audioMdef_Game` | Options Audio; toggles/sliders, cancel returns | **Covered:** exact streams, slider geometry, resolution offsets, volume clamp, and live music volume are tested |
| Controls | `0x23` / `0x24` | `runControlsMenu`, `titlecontrolsMdef`, `controls1Mdef`, `controls2Mdef` | Options Controls; state `0x23` resets ownership after drawing, while `0x24` retains it | **Covered:** direct call graph proves there is no separate player-choice page; exact streams, P1/P2 conditional ownership, mutations, return, and rendering are tested |
| Language | `0x91` | `languageMdef` | Options Language; change language and return | **Covered:** exact stream, `xopt_sel` cue, `generateAllText`, mutation, return, and rendering are tested |
| Credits | `0x13` | `menu_drawCredits`, `creditsMdef` | Options Credits; exit returns | **Covered:** exact initialized data, clipping, scaled scrolling, reset, input, route, and visible-credit rendering are tested |
| Concept Art | `0x1b` | `menuConceptMenu` | Conditional Options item when secret bit 7 is set; exit returns | **Covered:** direct owner, paging, wrap, art, arrows, route, and rendering are tested |
| Quit confirmation | `0x92` | `rusureQuitMenuMdef` | Main Quit; Yes exits, No returns | **Covered:** exact stream, both executable branches, and rendering are tested |
| Register Your Game | `0x8f` | retired external-service action | Conditional Main item when `m_canShowRegisterGame == 1` | **Covered behavior:** the exact executable URL action and immediate return to Main are regression-covered; ordinary builds continue to hide the conditional item |

## Proven Corrections

- Restored `menu_initNewMenu` as the frame-end transition owner for reachable states.
- Restored the no-save New Game bypass of the overwrite confirmation.
- Restored VS requested player-count ownership on entry.
- Restored the exact shipped-EXE VS keyboard-tab centers (`120` and `820`) so keycaps no longer overlap `Classic` and `New Game +`.
- Restored the shipped `DrawUITextUTF16` control-marker pass globally: its six Unicode markers and `<A/B/X/Y/F>` forms now select the installed controller/keyboard materials, preserve the pre-mutation text anchor, use the recovered text-pen baseline, `0.35` icon scale, and five-pixel vertical bearing correction, and leave the localized label in the shared text draw.
- Restored the title-menu call to `menu_startAcceptDecline`, mapping Start (Space/Enter on keyboard) to the accepted south-button action before menu dispatch.
- Restored the shipped two-player loader order after P2's final motion data is attached: camera type `0`, then refresh P1 and P2. The VS runtime now uses authored arena dolly `45` instead of the portable midpoint-orbit fallback.
- Restored Audio volume clamps and both canonical slider renderers.
- Restored state `0x04`/`0x05` entry into Character Select, eliminating the duplicate Player Count/Difficulty loop.
- Restored the localized reconnect presentation and its title/in-game geometry for one- and two-player character select.
- Restored the resolved root-entry audio, model, global-bit, selector, title-art, and title-music operations.
- Restored Options entry persistence, title state, menu-music restart, and its bounded retail `slider` input.
- Restored all four Level Select frame-mover streams and their exact `MMVDEF` installation order, then moved selector/box/art/fade behavior back to the PDB-owned `dstSelector`, `selbox`, `selCount`, `artload`, `artLevel`, and `artloadPos` fields.
- Restored Level Select's exact 40-draw owner order and executable depth constants; removed the invented mask bands and null/index suppression.
- Restored the direct Controls call graph and removed required-material suppression; states `0x23` and `0x24` are ownership variants of the same full controls page, not two visual submenus.
- Restored Language's direct mutation, text rebuild, cue, and return sequence.
- Restored Video's dynamic resolution presentation and live apply path, then synchronized fresh startup to the active valid display mode so a 1080p framebuffer cannot report 640x480.
- Restored the conditional Register URL action and its immediate return to Main.

## Regression And Visual Proof

- The focused menu matrix passes `29/29`, covering unit ownership plus every executable route named `jpb_pc_title_*` (excluding the separately tracked FMV/FED handoff tests).
- The settled 1920x1080 proof set contains 21 frames under `out/menu-parity-hd`; `contact-sheet.png` is the compact review surface and each frame retains its final draw log.
- The proof set covers Main, New Game player count/difficulty, overwrite, one/two-player Character Select and ready states, Level Select, Continue, Training, VS, Options, Video, Audio, Controls, Language, Credits, Concept Art, and Quit.
- Fresh `1920x1080` proofs under `out/versus-fix-proof` show separated VS keyboard tabs, material-backed Esc/SPACE prompts, and an authored front-facing arena composition with both players upright and no top-right void. The Character Select glyph alignment was live-approved on 2026-08-21; the arena handoff remains pending live review.
- The no-harness arena proof holds that composition for `600` frames: authored dolly `45`, camera type `0`, `567/567` framing samples onscreen, neutral input, and P1 idle motion `0`. The physical-keyboard Space regression enters New Game state `3` through the complete PC input/menu path.
- Proof images are presentation evidence only. Canonical implementation decisions above remain grounded in the local PDB, shipped executable, and installed data.

No known reachable-menu implementation failure remains in the focused regressions. The VS Character Select glyph alignment is live-approved; the corrected arena handoff remains open until live review.
