# TODO

## Open Items at a Glance

- [ ] Live-review the VS arena handoff and first gameplay frame.
- [ ] Finish FED camera parity and framing.
- [ ] Correct the FED scripted-opening timing and composition.
- [ ] Live-test pickups, death/retry, continues, and game over.
- [ ] Diagnose the unattended upside-down/death-like failure.
- [ ] Audit scripted player control across all levels.
- [ ] Live-review the source-reconstructed saber blade.
- [ ] Complete manual FMV review.
- [ ] Triage the remaining soak warnings.
- [ ] Finish FMV hardening and decoder packaging.
- [ ] Expand boss coverage and repeat visible boss smoke.

## 1. Active Priorities

### 1.1 Reachable Menu Parity - Pending Live Review

- [x] Inventory every menu and submenu reachable from the main menu, including state IDs, destinations, entry conditions, and return paths. See `docs/MENU_PARITY_AUDIT.md`.
- [x] Audit each reachable menu's command stream, state handling, input, transitions, audio cues, and presentation against the PDB and shipped executable.
- [x] Correct every proven mismatch without introducing approximations or fallback behavior.
- [x] Add transition and rendering regressions for every reachable branch, including alternate player-count, continue, cancel, and confirmation paths.
- [x] Capture a complete visual and navigation proof set before requesting manual review.
- [x] Recover the VS Character Select `SHIFT`/`H` keycap rectangles and adjacent text anchors directly from the shipped EXE; both use the exact character-select positions and shared vertical composition.
- [x] Restore the shipped menu Start-to-accept remap so Space/Enter performs the same select action advertised by the keyboard prompt.
- [x] Restore the shipped two-player loader order: select camera type `0`, refresh both players after P2 is complete, and use the arena's authored camera instead of the portable midpoint orbit.
- [x] Restore the shipped `DrawUITextUTF16` control-marker path for every menu prompt. It inserts the canonical leading spaces, draws the installed key/controller material at the text pen with the recovered `0.35` scale and five-pixel bearing correction, then removes the marker from glyph rendering. The 1920x1080 proof is `out/menu-glyph-proof/character-select-1080.png`.
- [x] Live-approved the staged 1920x1080 VS Character Select glyph alignment on 2026-08-21, including the upper Shift/H labels and lower Esc/Space prompts.
- [ ] Live-review the first VS arena frame and handoff; confirm both players are upright, the arena faces the camera, and no top-right void is visible.

The focused menu matrix passes `29/29`. The original 21-frame settled 1920x1080 proof set and per-frame logs are under `out/menu-parity-hd`; corrected VS proofs are under `out/versus-fix-proof`. The canonical evidence ledger is `docs/MENU_PARITY_AUDIT.md`.

The corrected no-harness arena hold remains on authored dolly `45` and camera type `0` for all `600` frames, keeps `567/567` framing samples onscreen, reports neutral input and idle motion `0`, and retains both upright players in the final 1920x1080 frame. A physical-keyboard headless run also proves `space` enters New Game state `3`.

### 1.2 FED Camera - Blocker

- [ ] Record a retail FED run from first left-stick movement through shutdown, then compare it frame-for-frame with the portable trace.
- [ ] Prove later-track framing sequentially; treat sustained black void or loss of visible gameplay geometry as a failure requiring another source audit.
- [ ] Remove or replace every remaining local camera remap, forced dolly, or nearest-track rule that is not proven by the PDB or shipped executable.
- [ ] Correct the FED scripted-opening duration from authoritative local evidence.
- [ ] Recheck the FED opening-director composition after the cube-selector fix.

### 1.3 Gameplay Lifecycle

- [ ] Manually verify staged FED pickup collection, healing/score, death restart, continue consumption, and game-over behavior.
- [ ] Reproduce the unattended upside-down/death-like failure and identify whether it is death, physics, or camera state.
- [ ] Audit every canonical scripted player-control takeover across all levels for correct ownership, timing, release, and neutral input afterward.

### 1.4 Pending Manual Review

- [ ] Verify startup and level-1 FMVs in a visible, unmuted window, including volume, A/V sync, skip, and return state.
- [ ] Review the source-reconstructed saber blade staged on 2026-08-21.
- [ ] Repeat boss smoke visibly with local display and audio available.

### 1.5 Follow-up

- [ ] Triage the remaining soak warnings: `21` death/energy, `4` visibility-only, `4` visibility-plus-death, and `1` Hangar pixel-proof miss.
- [ ] Harden FMV failure and cleanup paths, settle decoder shipping, and extend framebuffer proof coverage.
- [ ] Find authoritative anchors for final/Core Maul and Palace/Hangar set pieces before expanding boss coverage.

## 2. Status Snapshot

- [x] Default visible launch is full-frame `1920x1080` at 16:9; `--framebuffer-size` remains an explicit test override.
- [x] The initial Video-menu resolution now matches the active valid framebuffer mode; a dedicated 1920x1080 regression prevents the old 640x480 label mismatch.
- [x] Camera defects in fixed-dolly lead gating, movement normalization, and two-player movement averaging are patched and regression-covered.
- [x] Retail capture, portable replay, one-second camera pulses, and trail comparison tooling are ready.
- [x] Unwanted controller movement is gone in live testing; simulated controls require explicit `--control-harness`.
- [x] Powerup contact and the shipped death/reset lifecycle are implemented and regression-covered.
- [x] Current targeted staged smoke reports `2 pass / 0 warning / 0 fail`.
- **Pending review:** the static saber blade now uses the source-reconstructed six-quad `a_glow.tga` path; noncanonical host line/disc glow fallbacks are removed.
- **Monitor:** FPS stalls are not reproducing in the latest live report; retain the current smoke results as the baseline.
- **Paused pending review:** do not expand or rerun FMV smoke until the existing proof has been reviewed.

## 3. FED Camera

### 3.1 Canonical Findings and Fixes

- Proven defect: `camera_SetCameraPos` selected the active dolly from `_jheightstuff.poly`. Direct shipped-EXE disassembly selects `_jheightstuff.cube`, resolves library cubes from the cube word, and reads the camera byte at record offset `7`. The reconstruction now follows that path.
- Proven secondary mismatch: FED X-axis camera lead used `flexmul`; the shipped executable uses `flexmul12`. The corrected signed rounding behavior has focused regression coverage.
- Proven camera-module mismatch: `camera_StuffCamera` advanced the hidden movement-lead accumulator before testing dolly flag `0x400`. Retail jumps around the entire lead update for fixed authored cameras; FED opening dollies `144`, `145`, and `146` all carry `0x00000406`. The update is now gated exactly and a regression requires the lead vector to remain unchanged on fixed dollies.
- Raw-instruction camera audit found that retail normalizes the truncated movement vector with `normalize_svector`, while the reconstruction called the separate four-argument `normalize` routine. The exact call and its in-place signed-16-bit overflow reduction are restored; a `-32768` movement regression distinguishes the routines. Retail also computes the type-0/two-player movement average by adding P2's float before truncation, and that operation order is now covered. These are real camera-module corrections, but the normalization difference occurs only at the signed-16-bit boundary and the averaging branch is two-player-only, so neither by itself explains the reported ordinary solo traversal pull-out.
### 3.2 Regression and Runtime Evidence

- Focused regressions: `test_camera_dolly_comes_from_collision_cube` gives the cube and polygon different camera IDs and requires the cube ID. `test_camera_preserves_retail_transition_mask` locks down the shipped candidate-mask behavior described below so a noncanonical rejection fallback is not introduced.
- Corrected FED `D` route: `265/265` framing samples onscreen, final dolly `56`, target at `368,214`, camera/player distance about `973`, and no late camera drift after the player stopped. The old selector ended on dolly `0` with target Y about `1005` on a `960x540` frame.
- Corrected FED `W` route at `1920x1080`: `265/265` framing samples onscreen, `0` offscreen, `0` deaths, `0` missed presentation intervals, and no black void in the retained final frame.
- Staged no-harness FED check: `scripted=0`, neutral pad state, `25/25` available framing samples onscreen, `0` missed presentation intervals, and `59.81 FPS` over `360` frames.
- Direct retail-EXE audit currently confirms the single-player FED focus/offset/clamp math, angle selection, slide interpolation, cube-record dolly selection, candidate acceptance, dynamic yaw, and final snap/smoothing branches after the fixed-dolly lead correction. This is a scoped result, not a declaration that the entire camera subsystem is correct.
- FED type-1 execution audit: a function-by-function comparison against the raw Ghidra export and direct shipped-EXE disassembly now covers the solo-player alias, target construction, `camera_SetCameraPos`/`camera_StuffCamera` branch order, movement normalization, X/Z fixed-point lead asymmetry, transition acceptance, snap/slide state, one view conversion per frame, camera-before-render ordering, and fixed `0x800` gameplay step. The portable FED frame cannot fall through to its legacy orbit-camera builder after collision load. The following view-matrix conversion and D3D projection also match retail's 53-degree vertical FOV and live `width / height` aspect. The corrected normalization mismatch does not affect the retained ordinary-range solo traces; later-track collision/selector inputs still require the retail movement trace before the blocker can close.
- Retail cadence audit: `game_gPlayTheGame` calls `game_OneGameLoop` once per outer render iteration, `__EndRender`/presentation is the pacing boundary, and the gameplay globals remain fixed at `gGlobalFrameRate=0x800` and `fGlobalFrameRate=0.5`. There is no separate half-rate camera/simulation gate to restore. The short portable FED opening therefore remains an authored takeover/interpreter timing defect, not evidence that the whole game loop advances twice per retail frame.
- Exact scene-readiness mismatch corrected: portable level initialization promoted `gSCENE_READY` as soon as collision loaded, while retail `scene_gInitRoot` leaves it clear until `scene_postRender` completes frame two. The port now follows that lifecycle. Focused camera/scene/control tests pass, and the 380-frame FED replay retains the same authored dolly transitions (`144` at frame `2`, `146` at `92`, `145` at `158`, release at `334`, normal dolly `56` at `335`), confirming this startup fix is not the later pull-out cause.
- Retail candidate-mask audit: raw instructions call `CalcNewBox`, which returns `0x10`, then compute each player's contribution as `cliptofrustrum(...) & 0x10 & 0x0f`. The generic offscreen value is consequently zero and ordinary camera candidates are accepted; only the separate level-10/camera-15 exception can veto afterward. This is confirmed executable behavior, so black-void safety must remain a retail-versus-portable observation gate. Do not add a local candidate-rejection fallback and do not assume this code protects framing.
- FED selector-input audit: direct disassembly of `intersec_FindWalkHeight` and the complete 1,169-byte `jon_plumbline` confirms the caller-owned `_jheightstuff` contract and exact cube/entry/poly writes used by `camera_SetCameraPos`. The collision chunk dispatcher, terminal Jonny chunk relocation, `leveldata` base/offset arithmetic, and exact 8,192-byte FED `.cam` load were also checked against the shipped executable. No selector or camera-record load mismatch was found. A focused `flags=1` regression requires all three installed-format pointers; Jonny, intersec, and camera suites pass.
- Integrated FED camera re-audit: `ProcessPhysicsObjects` publishes `pos` into `vpos` through `UpdateSceneObject` after movement in both builds, and both cameras consume that published state on the following frame before `scene_middleRender`. Direct executable/PDB checks also reconfirm the type-1 target, dolly selection/acceptance, corrected `camera_StuffCamera`, `camera_CameraSlide`, view publication, and live-aspect projection path. Retained FED traces show `physics.mov` matching actual per-frame player displacement within sub-unit collision corrections, so there is no current evidence of an inflated movement producer; the live differential trail is still required because it can expose a later-track state divergence that static parity cannot.
- FED camera integration gates: all seven authored intro/director/handoff/settle/spawn/follow tests pass. Their diagnostic regexes now tolerate additional fields inside `camera=(...)` while preserving dolly, transition, collision, player-state, AI-release, world-camera, and visibility assertions.
### 3.3 Capture and Comparison Tooling

- Manual camera-trail recording now arms only on real cached input (or explicit `--control-harness` input), rather than ORing `playerPad.mask1 == 0xffffffff` into the recorded controls. The shared visible/headless writer also records world target, inactive P2 position/movement/flags, camera type, lead, projection, and dolly transition counters. A 400-frame no-harness FED run did not arm during authored opening movement; an explicit harness run did arm and produced the expanded rows.
- Retail traversal capture now has matching state/pulse logging, an independent opening-sequence watcher, and compressed visual recording armed. The state watchers request 1 ms Windows timer resolution while attached and report every observed frame gap plus a final missed-frame count. Portable replay preserves the opening delay and supplies the exact game-seen held mask and analog axes only when the next retail sample has the exact expected `totalframes`; a gap is a hard error because interpolating input would hide a capture failure. Replay cannot activate without the explicit control harness and FED quickload switches.
- `tools/compare_camera_trails.ps1` checks 52 mapped retail/portable fields on common authoritative frame numbers, reports rows missing from either trail, reports the first divergence per field, and emits side-by-side one-second player/eye/dolly/lead pulses. Synthetic exact-match, single-field mismatch, and missing-frame fixtures passed, then were removed.
- Supplemental FED navigation ran 3,599 recorded rows but stalled in the first combat pocket, so it is not later-track evidence. It exercised fixed opening dollies `144/145/146` and normal dolly `56`, kept every sampled gameplay frame onscreen, and exposed no black void before an unrelated authored-attack validation failure; the temporary executable and trail were removed.
- Lifecycle audit: shipped `game_ResetGameSystems` restores events, physics, players, enemies, then cameras before powerups, damage, zero-BSS flags, visibility/material/audio, music, and Pikobi visibility. The portable `game_ProcessStatus`, `game_ResetGameSystems`, and `game_runStage` owners now reproduce that sequence after a completed gameplay frame when `StageExit` is set. A real FED regression forces the shipped death state and verifies spawn, energy, player flags, continue count, camera/system reset, and pickup restoration on the next frame.
- No-harness opening audit after the fixed-dolly repair: dollies `144 -> 146 -> 145` retain lead `0/0/0`, normal dolly `56` resumes at frame `335`, authored post-cutscene movement peaks at `191` units of lead, and lead returns to zero by frame `540` with no later drift through frame `899`. The disposable per-frame trace was removed after the audit.
- FED dolly `8` has an exact installed-data regression at player position `7040,4096,-6400`: flags `0x103a` must produce focus `8099,4576,-8252` and angle `220,1024`. A layered FED collision sweep resolves selector-owned dolly `8` across 274 samples (`x=6528..7552`, `z=-25472..-5504`, `y=3872..4128`), and its sampled entry near `7296,4096,-8320` projects onscreen. A local collision-resolved harness from that entry never reached `-6400`; it entered an invalid fall/death state near `7090,4359,-7590`, so it is not canonical traversal evidence. Review confirmed the retained final composition is within the bounds of normal game behavior, so its visible black edge is not a blocker. FED has no placements for camera-mutating AI `71` or `91`, and the six placed camera-flag scripts do not mutate dolly `8` at this boundary. Keep the broader live retail/manual selector trail open, but do not retune this authored dolly or add a rejection fallback.
- Headless input-trail recording now works under the explicit control harness and records camera type, dolly/flags, lead/dot, projected player position, on-screen counts, and transition counts per frame. The first post-fix sweep returned from fixed dollies `144/146/145` to dolly `56` with `1265/1265` projected gameplay samples on-screen, but collision kept that sweep inside the opening room, so it is not later-track proof.
- Adaptive traversal is not acceptable later-track proof: precise per-frame animation telemetry proves damage motions `92`, `94`, `105`, `106`, and `107` reach their sequence ends and return to locomotion with an empty motion queue and seven free nodes. The harness cannot infer the playable route from straight-line placement distance and repeatedly drives into opening-room geometry; doorway and wider-enemy experiments were rejected and removed. It still uses only explicit `--control-harness` input and does not teleport or edit gameplay state, but it is not a substitute for the retail/manual movement trace.
### 3.4 Open Evidence Gap

- Retail-versus-portable FED movement trace comparison and sequential later-track black-void proof remain open. The restored death/reset owner only runs after `GameStruct.StageExit`; it does not explain ordinary movement pull-in/pull-out before a death or restart.

## 4. Completed Blocker Work

### 4.1 Rendering and Performance

- Done: staged the rebuilt executable to `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`.
- Done: removed the noncanonical coordinate/glint/powerup fallback path; powerups now render only through canonical pickup BMD geometry and resolved canonical textures.
- Done: canonical default-white texture resolution is wired for the runtime cache. Exact default white material requests (`white.png`) and the model-stream `s/white.tga` request resolve through installed `res\default\white.tga`; the FED `d` route at `1920x1080` now reports no `texture_load_failed` rows in `out/texture-white-fed-d-900.stderr.txt`.
- Done: powerup pickup BMD presentation now uses the existing D3D screen-poly triangle sink instead of CPU rasterizing the post-scene pickup pass. The 1800-frame FED harness improved from repeated mid-run `247..352ms` render stalls to a single startup stall; steady run reported `fps=59.14`, `render_avg=5.008ms`, and `powerups_ms=18.272` max at frame 0.
- Done: D3D model vertex-buffer growth now allocates to retained capacity instead of exact high-water vertex count, removing the earlier model-buffer recreate spike.
- Done: D3D model and immediate-effect submissions use the same full `1920x1080` gameplay viewport and scissor rect as the world path.
- Superseded: the former projected-radius/capped-radius D3D glow experiment and matching software disc renderer were removed after direct `fx_screenGlow` reconstruction proved that the executable itself builds the six camera-space blade quads.
- Done: default runtime logging stays quiet; opt-in `--profile-runtime` stall details now log at `25ms` so short hitches are diagnosable without affecting ordinary in-window play.
- Done: runtime enemy/animation deep profiling is now opt-in behind `--profile-runtime`, so normal in-window play no longer pays the per-frame `clock()` cost or prints deep stall details by default.
- Done: live Win32 pad reads are cached once per frame per pad, so the original repeated `input_ReadControlPad` calls no longer resample XInput multiple times inside a frame. Current staged no-harness FED smoke improved from `render_max=57.398ms`, `missed=6/899`, `input_ms=14.000` to `render_max=21.213ms`, `missed=0/899`, `input_ms=0.000`.
- Done: WinMM SFX playback now reuses compatible inactive voice outputs and pre-seeds a small output pool during bank preload. FED `--profile-runtime` reduced the enemy sound activation hotspot from `sound_ms=21.000` to `sound_ms=5.000`; default no-harness `1920x1080` smoke reports `fps=59.78`, `render_max=29.095ms`, and `missed=2/899`.
- Done: hardware gameplay now caches the exact black/white HUD composite layers when the ordered HUD draw list hashes identically, avoiding the previous 1080p full-frame HUD replay/upload cost without changing the compositor math. Hangar `1920x1080` movement improved from `fps=38.03`, `effects_avg=11.777ms` to `fps=57.22`, `effects_avg=1.449ms`; proof build hash was `EF7E49EEA446F63BCFF0F3A1D2DD65B3EDE4A1C0C6E228854937441809828DFB`.
- Partial: enemy class BMD geometry/materials and AI data are prewarmed at level load with real registered model names only; the frame-188 activation spike remains and needs substep timing before adding any broader actor pre-instantiation.
### 4.2 Gameplay

- Done: canonical powerup smoke passed at the known level-1 pickup coordinate: `out/immediate_blockers/level1_powerup_spawn_staged.ppm`, with `powerups=55/5/0/models:2/8411`.
- Done: canonical FED powerup contact now consumes the pickup, applies `HEAL`, awards `+50`, removes the model, and restores the pickup after a death restart. The old gate incorrectly read `initialLevelPauseDelay`; direct shipped-EXE disassembly reads `GameStruct.ComboLevel`.
- Done: the shipped death lifecycle is restored through `game_ProcessStatus`, `game_ResetGameSystems`, and `game_runStage`. Normal remaster deaths consume one of the initialized `1000+` continues and automatically reset FED; exhausted continues show the exact game-over stream, whose confirm item uses destination `6` and pushes retail menu state `6`.
- Done: earlier camera smoke regression was clean for the immediate FED/Hangar/Core set, but the current staged build intentionally no longer remaps FED camera candidate `3` to dolly `0` because that remap caused sustained level-1 movement to expose black void. Hangar now starts on authored dolly `80`, the nearest valid dolly that frames exact `startPos[9]`, and keeps that spawn track instead of switching to bad waterfall dolly `1`.
- Done: 16:9 FED camera probe passes with the centered authored viewport: `docs/LEVEL_SMOKE_CAMERA_AUTHORED_VIEWPORT_FED.md` reports `1 pass / 0 warning / 0 fail`, `player_framing=264/264`, `streets_culled=0`, and retained proof `out/camera-authored-viewport-fed/fed.d.png`.
### 4.3 FMV

- Done: startup FMV hidden-window smoke passed against the staged exe: `out/immediate_blockers/title_movie_validate_hidden_window.ppm`, with `requests=1,resolved=1,launched=1,presented=119,failures=0,last=0`.
- Done: level-1 FMV asset hidden-window smoke passed against the staged exe: `out/immediate_blockers/level1_movie_validate_hidden_window.ppm`, with `requests=1,resolved=1,launched=1,presented=120,failures=0,last=1`.
- Done: staged `ffmpeg.exe` beside the game exe because decoder lookup no longer searches `PATH`. This adds `227,398,656` bytes to `C:\Games\Star Wars Jedi Power Battles`.
- Done: FMV audio now writes decoded PCM to the default WinMM output during unmuted in-window playback. Proof: `out/immediate_blockers/title_movie_audio_output_hidden_window.ppm`, with `audio_output=1` and `audio_queued=737792/65`.
- Done: FMVs can now be skipped with a fresh keyboard confirm/back press or XInput A/B/Start/Back press. Proof: `out/immediate_blockers/title_movie_skip_hidden_window.ppm`, with `presented=20` and `skips=1`.
- Partially reviewed: the current staged build has had a live/manual in-window test. Audible FMV volume/sync and later FED camera behavior beyond the exercised route remain open.

## 5. Saber Tuning

### 5.1 Current Status

- Pending user review: direct shipped-EXE reconstruction of `fx_screenGlow` now emits the exact 12-vertex/six-quad `a_glow.tga` geometry, UV topology, packed color, no-scale submission, and depth bias.
- Done: restored caller constants for normal, Blade Extender, and Blade Amplifier paths. Normal blades use extent `112`, outer width `14..19`, and white-core width `2`; Extender uses extent `196`, outer width `24..31`, and core width `6`; Amplifier uses width `16`.
- Done: removed the separate D3D procedural line shader and software glow-disc renderer. Both backends now consume only the canonical immediate polygons.
- Done: added exact all-six-quad regression coverage and staged 1920x1080 FED software/D3D smoke.
- Pending: replace the rejected distant FED screenshot with a close, unobstructed native-1080 visual proof before requesting blade sign-off.
- Scope note: attack blur/trails and powered-cylinder flourishes remain separate effect paths; this item concerns the blade, as clarified on 2026-08-21.
- The user-provided remaster screenshot from 2026-08-21 and retained online footage are visual context only. They must not supply implementation values or substitute for RE evidence.
- Exact findings, historical context, and proof are recorded in `docs/SABER_TUNING_AUDIT.md`.

### 5.2 Historical Visual Context Only

- These links document the earlier visual tuning pass only. They are not authoritative implementation evidence and must not drive further blade changes.
- Official remaster color-toggle reference: PlayStation Blog says the remaster can switch classic colors to new colors aligned with broader Star Wars media, with Mace `blue -> purple`, Adi Gallia `red -> blue`, Plo Koon `yellow -> blue`, and Ki-Adi-Mundi `purple -> blue`: `https://blog.playstation.com/2024/12/05/star-wars-episode-i-jedi-power-battles-reveals-new-lightsaber-color-toggle-feature/`.
- `Star Wars Episode I: Jedi Power Battles [REMASTERED]` playlist: `https://www.youtube.com/playlist?list=PLWoB_QSNfUdDKoTc5JvSWiC3opVjJl1Ia`
- `Episode I: Jedi Power Battles Remastered (PC)` playlist: `https://www.youtube.com/playlist?list=PLnJPQhyLkUVxozxg1bhEcTBJrZ1Ajtpws`
- Nintendo Switch remaster gameplay: `https://www.youtube.com/watch?v=AgltWhMIQbA`
- Original-vs-remaster comparison candidate: `https://www.youtube.com/watch?v=10LdI0Yb5Tk`
- Focused character samples: Mace `https://www.youtube.com/watch?v=O5x3HdMnJzk`, Plo `https://www.youtube.com/watch?v=Jqq-ddeuwXo`, Maul `https://www.youtube.com/watch?v=FKK1YfFSQvQ`, Adi `https://www.youtube.com/watch?v=k-aVtG30nDk`, Ki-Adi `https://www.youtube.com/watch?v=YKZNvSjMw54`.

## 6. Level Soak

### 6.1 Harness

- Done: added `tools/soak_levels.ps1`, a long headless crash-hunt runner that cycles movement, jumps, attacks, block, and jump/block chord input across playable quickload levels.
- Done: the soak runner enumerates authored enemy placements, samples them across each level, spawns the player at sampled placement coordinates, forces the placement active, and reruns the same jump/attack route.

### 6.2 Crash and Validation Fixes

- Done: fixed the three stable access-violation repros from the long-soak run.
  - Tatooine placement `0` (`tusken.baf`) and Corus2 placement `0` (`spedblu.baf`) now survive the malformed kungfu owner chain that previously crashed `shaolin_node_player`.
  - Hangar placement `119` (`pwrdrink.baf`) now survives the malformed physics owner path from `physics_gGetPosition`; the headless renderer also skips non-primary owner-3 helper actors that expose malformed BMD geometry.
  - Focused verification refreshed `2026-08-15 13:08:02 -04:00`: all three repros exit `0` for `600/600` frames. Final logs are under `out/level-soak-repro-final`.
- Done: fixed the surviving host-validation `exit=5` rows found after the crash fixes.
  - A full post-crash-fix rerun recorded `33` pass / `32` fail with no crash markers; failures were non-crash validation/death rows.
  - The headless authored-combat validator now treats `combatHitCount > 0` plus `enemyDamageProcessedCount > 0` as the required pipeline proof; HP loss and enemy reaction/recoil remain telemetry because special/invulnerable actors may process damage without either side effect.
  - The authored-motion validator now accepts recovered locomotion/jump/vehicle movement signals even when forced placement collision leaves the actor at the same final X/Z.
  - Final targeted `exit=5` proof refreshed `2026-08-15`: `docs/LEVEL_SOAK_EXIT5_FINAL.md` reports Theed/Hangar/Mini3 at `5` pass / `4` fail with `exit5 count=0` and `crash markers=0`; all fail rows were route-quality issues.
- Done: the soak runner now separates hard crash-hunt failures from route-quality warnings.
  - Hard failures are timeout, nonzero exit, wrong level, missing frame summary, missing geometry, or explicit crash markers.
  - Warnings are missing gameplay pixels, no visible player samples, player death, low travel, and runtime-current-enemy mismatches.
  - Final warning-aware targeted proof refreshed `2026-08-15`: `docs/LEVEL_SOAK_WARNING_FINAL.md` reports Theed/Hangar/Mini3 at `5` pass / `4` warn / `0` fail.
- Done: full warning-aware soak refreshed `2026-08-16`: `docs/LEVEL_SOAK_WARNING_FULL.md` reports `34` pass / `30` warn / `1` fail at the old `180s` timeout.
  - The only hard row was Coruscant 1 route timeout. Focused rerun with `300s` timeout (`docs/LEVEL_SOAK_CORUS1_REPRO.md`) reports `2` pass / `1` warn / `0` fail, so the hard row was a timeout-budget miss rather than a crash.
  - `tools/soak_levels.ps1` now defaults to `300s` per child process.
- Done: final full warning-aware soak refreshed `2026-08-16`: `docs/LEVEL_SOAK_WARNING_FULL_FINAL.md` reports `35` pass / `30` warn / `0` fail with the new `300s` default timeout.
  - The former Coruscant 1 timeout row now passes in the aggregate matrix at `1800/1800` frames.
  - Remaining soak work is route-quality triage only: `21` death/energy rows, `4` visibility-only rows, `4` visibility-plus-death rows, and `1` Hangar pixel-proof miss.

### 6.3 Evidence Locations

- Historical crash repro logs are under `out/level-soak-repro`; current focused crash-fix logs are under `out/level-soak-repro-final`; full rerun logs are under `out/level-soak-rerun`; final `exit=5` proof logs are under `out/level-soak-exit5-final`; warning-aware targeted proof logs are under `out/level-soak-warning-final`; old full warning-aware proof logs are under `out/level-soak-warning-full`; Coruscant 1 timeout-budget proof logs are under `out/level-soak-corus1-repro`; final full warning-aware proof logs are under `out/level-soak-warning-full-final`.

## 7. FMV Playback

### 7.1 Review Status

- Pending review: FMV smoke/proof work is paused for review. Do not spend more cycles expanding FMV checks until the current in-window visual proof, muted PCM proof, orientation proof, decoder lookup, and visible/default framebuffer audits have been reviewed.

### 7.2 Playback

- Done: in-window FMV video decode/presentation now runs through the Win32 title framebuffer and D3D presenter.
  - Current proof: `docs/FMV_HARDWARE_SMOKE_AUDIT.md` reports `8 passed / 0 failed` for 60-frame hidden hardware/window checks refreshed on `2026-08-15 03:12:00 -04:00`.
  - Current muted audio proof: `docs/FMV_SMOKE_AUDIT.md` and `docs/FMV_HARDWARE_SMOKE_AUDIT.md` validate all known movie indices `0..7` with nonzero video decode/present counters and nonzero `audio_bytes`, `audio_samples`, and `audio_chunks`.
  - Focused regression proof: `jpb_pc_title_movie_resolution` still validates FMV index `7` with nonzero video decode/present counters and nonzero muted PCM counters.
  - FMV proof build hash after the upright fix, boss-jump diagnostics, muted FMV audio decode proof, orientation smoke proof, game-local decoder lookup, actor-name placement diagnostics, and exact boss-placement forcing: `4DB99DDEFBC4F847F7565DBD25D978449C1A3A6CD3E02FBD0152F0AD4E866AEF`.

### 7.3 Audio

- Done: decoded FMV PCM now queues to the default WinMM output during unmuted in-window playback.
- Done: the muted/headless diagnostic now proves PCM samples are produced even when audible output cannot be checked.
- Open: run a live verification pass for volume and A/V sync once speakers or headphones can be used.

### 7.4 Decoder Dependency

- Done: the launcher now requires `ffmpeg.exe` beside `jpb_pc_game.exe`; it does not search `PATH`, so an unstaged decoder remains visible.
- Current implementation still relies on an external `ffmpeg.exe` binary, currently staged beside the game exe for manual testing.
- Open: either bundle a known ffmpeg build beside `jpb_pc_game.exe` or replace the child-process decoder with a linked library/backend.
- Open: improve the visible error path when `ffmpeg.exe` is missing so the title menu reports the issue cleanly.

### 7.5 Coverage

- Done: `tools/smoke_movies.ps1` validates all known movie indices `0..7`, including localized text scroll index `1`.
- Current proof: `docs/FMV_SMOKE_AUDIT.md` reports `8 passed / 0 failed` for 60-frame headless checks refreshed on `2026-08-15 02:58:43 -04:00`.
- Current in-window proof: `docs/FMV_HARDWARE_SMOKE_AUDIT.md` reports `8 passed / 0 failed` for 60-frame hidden hardware/window checks refreshed on `2026-08-15 03:12:00 -04:00`; each row records matching runtime/capture framebuffer dimensions.
- Current visible default-window proof: `docs/FMV_VISIBLE_DEFAULT_SMOKE_AUDIT.md` reports `8 passed / 0 failed` for 60-frame visible/windowed checks at the default framebuffer refreshed on `2026-08-15 03:11:16 -04:00`; each row records `runtime=1793x1009, capture=1793x1009`.
- Current larger-frame proof: `docs/FMV_HARDWARE_1080_SMOKE_AUDIT.md` reports `8 passed / 0 failed` for 60-frame hidden hardware/window checks at `1920x1080` refreshed on `2026-08-15 03:05:40 -04:00`.
- Open: confirm retail behavior for movie flags, skip/cancel input, movie end, and returning to the correct title/menu state.

### 7.6 Visual Proof

- Done: `tools/smoke_movies.ps1` emits PPM/PNG captures and `contact-sheet.png` when Python/Pillow is available.
- Current visual artifacts are in `out/movie-smoke` and `out/movie-smoke-hardware`, with the hidden-window contact sheet showing all eight FMVs upright/in-window.
- Done: hidden and visible hardware/window FMV smoke now include a deterministic English-crawl orientation assertion; the `2026-08-15 03:12:00 -04:00`, `2026-08-15 03:11:16 -04:00`, and `2026-08-15 03:05:40 -04:00` audits record `Orientation=PASS`.
- Done: raw generated `.ppm` captures and per-capture PNGs were pruned after review handoff; compact `contact-sheet.png` visual proofs and audit ledgers are retained.
- Open: extend `jpb_pc_title_movie_resolution` beyond decode/present counters when the framebuffer proof hook exists.

### 7.7 Performance and Cleanup

- Done: short visible default-framebuffer FMV smoke passes all movie indices `0..7`.
- Pending review: do not run more long FMV playback smoke until the current FMV work is reviewed.
- Done: `tools/clean_smoke_outputs.ps1 -PruneLegacyHardwareProofs` pruned stale legacy hardware proof videos/exe snapshots from `out/hardware_proofs`, freeing `46.27 MB` and leaving current contact-sheet proof artifacts intact.
- Open: watch process/thread cleanup on early exit, skip, and repeated movie triggers.
- Open: consider frame dropping/latest-frame semantics if decode ever falls behind realtime.

## 8. Boss Smoke Coverage

### 8.1 Core Coverage

- Done: diagnostic boss-area quick jumps now run through `tools/smoke_bosses.ps1`.
  - Current proof: `docs/BOSS_SMOKE_AUDIT.md` reports `7 passed / 0 failed` for 180-frame headless checks refreshed on `2026-08-15 04:15:24 -04:00`.
  - Current in-window proof: `docs/BOSS_HARDWARE_SMOKE_AUDIT.md` reports `7 passed / 0 failed` for 180-frame hidden hardware/window checks refreshed on `2026-08-15 04:16:01 -04:00`.
  - Done: boss smoke now forces and verifies the exact target placement for every covered row; each audit row records `placementStatus=1` and `runtimePlacement=<target id>` so the smoke cannot silently pass on a nearby generic enemy.
  - Covered anchors: FED droid fighter, Marsh MTT, Theed tank, Tatooine Darth Maul, Coruscant thug, Mini2 Kadu, and Mini3 Boss Nass.
  - Done: Coruscant thug is upgraded from candidate to confirmed retail boss-stream/actor evidence; `06_CorThugBoss.wav` exists in the installed streams and placement `153` is in the camera-director enemy set with `mode=5`.
  - Done: Mini2 Kadu is covered by active `horns.baf` placements `0/1` in the authored camera-director enemy set; placement `0` reports `255/255` energy at the quickload spawn.
  - Visual proof artifacts are in `out/boss-smoke`, including `contact-sheet.png`; raw per-boss capture files are generated during smoke and pruned after contact-sheet generation to control repo size.

### 8.2 Visual Proof

- Done: boss smoke visual proof generation is integrated into the smoke runner.
- `tools/smoke_bosses.ps1` now emits PNG captures and `contact-sheet.png` when Python/Pillow is available.
- Done: contact-sheet proof generation is required by default; pass `-SkipProofImages` only for an explicit smoke-only run.
- Done: each boss contact sheet now emits a `contact-sheet.manifest.json` and the smoke runner asserts that the retained visual proof has exactly the smoke-matrix entries in order (`7` entries, `640x720` for the full matrix).
- Done: `tools/verify_boss_proofs.ps1` verifies retained boss proof artifacts without rerunning the game and writes `docs/BOSS_PROOF_VERIFICATION.md`; current verification refreshed on `2026-08-15 04:22:45 -04:00` reports `PASS` for `out/boss-smoke` and `out/boss-smoke-hardware` with `7` entries each, manifest/ledger agreement, retained artifact SHA256 identities, and `0` raw PPM captures.
- Done: boss contact sheets now follow the smoke matrix order so visual review lines up with the ledger rows.
- The audit ledger records the generated contact sheet path for both headless and hidden-window runs.
- Done: audit rows now distinguish raw capture byte counts from pruned `.ppm` files, so the retained contact sheet is clearly the authoritative visual artifact by default.
- Done: raw generated `.ppm` captures and per-capture PNGs were pruned after review handoff; compact boss contact sheets and audit ledgers are retained.

### 8.3 Matrix Expansion

- Open: add final/Core Maul and Hangar/Palace set pieces only after their authoritative placement or trigger anchors are identified.
- Current candidate probe audit: `docs/BOSS_CANDIDATE_PROBE_AUDIT.md` records why Core Maul, Palace offscreen bosses, and Hangar are not promoted yet.
- Current Core probe evidence: quickload `core` starts on camera-director placement `1` with `corguard.baf`, not a Maul actor; retail `10_CoreMaulFight1.wav` and `10_CoreMaulFight2.wav` exist; explicit P2 Maul-D loading selects models `0/43` and gives P2 resources, but the 180-frame proof has `second_player` pixels at `0`, so the gameplay trigger/visibility path still needs to be found before adding it to boss smoke.
- Current Palace/Hangar probe evidence: Palace placements `163/164` are referenced by offscreen kill bookkeeping but remain inactive in direct probes; Hangar is a timer/rescue set piece rather than a distinct boss placement anchor.
- Keep using actor/AI placement IDs in the ledger so boss-area smoke does not silently turn into generic-level smoke.

### 8.4 Hardware Review

- Current pass is hidden-window and muted; repeat visibly when a local display check is convenient.
- Compare future captures against both `out/boss-smoke/contact-sheet.png` and `out/boss-smoke-hardware/contact-sheet.png`.

### 8.5 Vehicle HUD

- Done: Kadu vehicle HUD regression coverage is green again.
- The 960x540 Kadu race bar expectations now match the current 1080p-relative gameplay HUD scaling.
- `jpb_boss_vehicle_tests` should stay in the focused boss/FMV verification set.
