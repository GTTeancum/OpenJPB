# FED ending and laser audio — September 11, 2026

## Progression

Started FED's authored ending controller, placement 166 / AI 60, using the real J3D and normal script interpreter. It activates the player run-off owners (167 / AI 55 and 168 / AI 56), which reach results through the normal completion signal. One-player and two-player runs then traverse results/awards and initialize Marsh in gameplay mode 6. Both players show authored AI run motion in the native log.

The opt-in fixtures are `--quickload fed --review-fed-ending` and `--review-fed-ending-two-player`, with `--control-harness`. They initialize menu assets without clearing the already-created gameplay state, activate controller 166, and place the players in its region. They do not set `nextLevel`, synthesize a completion flag, reduce boss health, or replace script data. They begin at the ending controller rather than automating a boss fight.

Evidence in `out/live-fixes-20260908/`:

- `boss-controller-one.log` and `boss-controller-one-native.log`: one player, score 20000, run-off then results, Marsh handoff at frame 488; exit 0.
- `boss-controller-two.log` and `boss-controller-two-native.log`: two players, score 40000 each, both run off and receive all three awards; Marsh handoff at frame 971; exit 0, final level=2/mode=6/players=2.
- `boss-ending-no-award.log`, native log, and `boss-ending-marsh.png`: final one-player low-score run and native capture of the destination level.

Use hidden-window rendering, a 426x240 framebuffer for flow tests, neutral input for 120 frames followed by `select` for one frame, and cycle those phases. The final native destination capture uses 960x540. The fixture requires FED and controller 166; no desktop input is sent.

Earlier diagnostic attempts that spawned fighter 128 alone omitted the ending controller. Those incomplete setup trials (`boss-ending-one*`) are not progression proof. Temporary health mutation used during those trials was removed from the final fixture.

## Laser audio: investigated, not closed

Authoritative local `decompiler-export/original/boss.c` shows `dfrblstr` played from both the inline fighter firing path and the helper at RVA 0x1C290, after the first projectile of the paired volley is allocated. Bank selection is min(player number + 1, 3). The reconstruction matches this behavior; the inline reconstruction calls the same helper.

The FED bank includes `fed/dfrblstr.wav`. The installed sample is mono 24-bit PCM, 44100 Hz, 26912 frames, peak about 0.986 and RMS about 0.215. It is not a silent asset.

- Actual fighter encounter playback with SDL's dummy audio output starts the FED sample repeatedly on native mixer channels. The captured parameters include pan 210/44, distance 171, and volume 30, with the movie gate inactive. `boss-audio-native.log` preserves this trace.
- Added a repeated-volley regression: three paired volleys produce three `dfrblstr` requests from bank 3. Bullet, game, enemy, and PC audio Release tests pass (`*-boss.log`).
- Added `--verify-pcm` to the audio probe. It observes float PCM after native mixing and fails if no non-silent buffers arrive. The FED laser test with `--movie-gate --verify-pcm` passes: one suppressed request, one resumed request, four non-silent output buffers. See `fighter-pcm.log`.
- Replaced unconditional fighter-specific logging with opt-in `JPB_TRACE_SFX` tracing of sample path, successful channel allocation, panning, distance, and volume.

The dummy output and PCM probe establish working game/mixer paths, not what the user heard through their physical output device in the reported session. No canonical gain, sound selection, or timing was changed. TODO #5 remains open because the reported silence has not been reproduced.

## Deployment

Deployed the tested candidate to `C:/Games/Star Wars Jedi Power Battles/jpb_pc_game.exe` with no game process running. Installed and candidate SHA-256 match: `48DC974F321563BB9096F1A73F2ABA14C80AE3B5BAAAD411B1346C95AD611DC6`. Previous installed executable preserved as `out/live-fixes-20260908/before-ending-audio-review.exe`.
