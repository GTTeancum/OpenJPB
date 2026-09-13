# Startup, title, objectives, and Audio parity — September 11

User comparison reopened Audio and identified missing startup and level-opening screens. Both main-menu and pause-menu Audio are included.

## Confirmed causes and corrections

- Normal startup forcibly replaced menu state 1 with state 0. Retain the canonical state-1 EULA/prompt/demo owner. Restore the missing state-1 confirm branch from `menu_mainMenu` (RVA 0xCD560); root-menu cancel returns to the prompt as requested.
- `menu_demoMovie` (0xC3010) requests videos 9, 8, 0 synchronously. The portable adapter omitted 8/9 and overwrote pending movie requests. Add the local warning and Aspyr assets from `ptrMovies` (0x4D4910), preserving request order and each request's VideoVolume through asynchronous playback. The recovered menu owner starts track 1, `00_SplashScreen.wav`, on return to the title. Existing selected mixer gain remains authoritative.
- `game_initPerLevel` opens the objective menu, but the host subsequently cleared `inMenuFlag`. Preserve the constructor's menu state. Restore the invisible opcode-0x0f confirm item's pop behavior from 0xCD560, rather than interpreting the next command word as a destination. The objective's cancel guard remains intact.
- Runtime text capture truncated each string to 63 UTF-16 units. Expand capture storage to 1024 so the full multiline FED objective reaches the renderer.
- Audio's common panel command 0x45 (and wide objective command 0x46) passed depth 0.0. The shipped `mmDrawsub` passes the float at RVA 0x33C880, which is 0.99. Restore that depth so panels sit behind sliders.
- `mmDrawMod` (0xD0C30) renders slider type 0x3801 as prefix-only, not decimal value text. Restore its flag handling. Selected Music/Mode values retain the closing `<`; selected volume rows retain their leading `>` without a closing arrow, matching the user's follow-up.

## Verification

Release `jpb_menu_tests` and `jpb_game_runtime_title_tests` pass. Regressions cover both Audio layouts, panel depth, label-only volume text, selected-volume arrow behavior, existing volume limits/direction/persistence, prompt open/close/reopen, and objective confirmation into gameplay.

Native hidden-window captures and process-local input, using SDL's dummy audio device:

- `audio-layer-before.png` versus `audio-title.png`: reproduced the occluded bars and corrected title Audio.
- `audio-pause.png`: corrected gameplay Audio with unobscured segmented bars.
- `objective-full.png`: complete FED objective text, all three lines.
- `startup-warning.png`: actual shipped warning video.
- `startup-sequence-native.log`: videos 9, 8, 0 launch in order; intro skip returns to state 1 and starts the splash track at selected gain 30. `startup-sequence.png` shows the prompt and copyright text.
- `title-close.log` / `title-close.png`: process-local A opens the menu and B returns to mode 1 with its prompt visible.
- `level-intro-handoff-native.log` / `level-intro-handoff.png`: level selection loads FED and its movie, then preserves objective mode 42.
- `level-intro-continue.log` / `level-intro-continue.png`: a separate confirm dismisses the objective through mode 65; active game mode 6 resumes with the objective game-state bit cleared.

Artifacts are under `out/live-fixes-20260908`. `--review-menu ID` is a process-local presentation fixture; combine with `--quickload fed` for gameplay versions. It skips EULA/movies only within that explicit diagnostic. Normal launch uses the recovered startup owner.

No desktop input or capture tools were used. Native captures verify presentation; audible playback with the user's device and physical controls remains a live check. Existing TODO items remain open for their separate live confirmations.

## Build

Staged executable: `out/live-fixes-20260908/jpb_pc_game.exe`.
SHA256: `4545925EDC60EC6FC62EE15896D82BEDA3E85FC8721EF0C7F4D9EC16C1975303`.
Deployed to `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe` with the game closed. Installed, staged, and Release build hashes match. Previous installed executable preserved as `out/live-fixes-20260908/before-frontend-parity-fix.exe`. User save/settings files were not changed.
