# Settings and fighter audio — September 12, 2026

## Changes

- Closed former TODO 4 (startup/legal) and 5 (objectives), as requested. Display is now item 4; mod support remains item 3.
- Display: the portable mode list sorted ascending, while local `wHook.cpp::resolutionComparison` (RVA 0x122640) sorts descending. Startup read the saved index without reconciling dimensions and created a maximized framed window regardless of saved mode. Restore descending ordering, match saved dimensions to this machine’s table, and apply saved source size/mode through the existing resolution path. Hidden tests apply window styles without showing windows or changing desktop display settings.
- Audio: load saved options before front-end constructors can publish text/music or save defaults. Initialize the mixer from both Music and musicVolume, including Music OFF before any menu opens. The previous isolated menu already rendered saved values correctly; the precise navigation-dependent visual symptom was not reproduced. The new first-frame checks and mixer checks pass; keep live confirmation open.
- Fighter: shipped `droid_f.cad` motions 8/10 use projectile 20 via `ai_FireWeapon` events. `res/effects/project.eff` contains no firing name for type 20, and both CAD sound slots are `0`. Phase one’s explicit `dfrblstr` comes from local EXE/PDB `ai_StarFighter`/`boss_StarFighterBlaster` (RVAs 0x1B010/0x1C290). Extend that same fighter cue to phase-two events only. This repairs an inherited empty-sound path; it is not claimed to be an omitted call in the retail ai_FireWeapon disassembly. The projectile asset and phase-one owner remain unchanged.

## Verification

Evidence under `out/live-review-20260912/`:

- Release bullet, menu, and PC audio suites pass. Repeated phase-two event shots emit three fighter cues; no-event frames remain silent. Existing phase-one paired-volley test still passes.
- Hidden native fighter runs: `fighter-music-phase1-native.log` and `fighter-music-phase2-native.log`, each 550 frames. Both start boss stream 5 at music gain 30 and play FED dfrblstr on native mixer channels. Phase two reaches motion 10. `--review-fighter-audio` is a process-local fixture that restores the boss-music trigger skipped by forced placement.
- `fighter-pcm.txt`: non-silent post-mix PCM after the movie gate resumes. SDL dummy output used; physical speaker balance remains a live check.
- `audio-initial-off.txt`: Music OFF initializes gain zero; gains 0/27/30/75 persist through start, resume and deferred music changes.
- `audio-title-first.ppm` and `audio-pause-first.ppm`: first rendered frame, no simulated button input. Isolated saves use Music OFF/volume 12 at title and Music ON/volume 27 in pause, SFX 42. No navigation required.
- `display-apply-native.log`: deliberately stale saved index 200 is remapped to 1280x720/index 10. Process-local input changes mode 2 to 0 and resolution to the next enumerated mode, 1128x634/index 11, then applies. Native styles change from framed `04cf0000` to popup `84000000`; both windows remain hidden.
- `display-restart-native.log` and `.png`: reopening a fresh process retains 1128x634/index 11/mode 0, matching the rendered selector. Physical exclusive fullscreen is left to live review.

All tests use isolated saves; the user’s installed save files were not written. The preserved level-two autosave evidence remains unchanged.

## Deployment

Deployed to `C:/Games/Star Wars Jedi Power Battles/jpb_pc_game.exe` after confirming the game was closed. Build, staged and installed SHA-256 all match: `2ca93091a1adc795c06c6551837fa677f44ea5b591f1e00bebd6c8a306045556`. Previous installed executable: `out/live-fixes-20260908/before-settings-fighter-20260912.exe`.

## Startup ordering follow-up

The user reported Press A appearing before the legal screen. A one-frame native capture reproduced it. Retail `menu_demoMovie` blocks in movie playback before drawing the title; the portable callback queues playback, allowing the requesting frame to reach presentation. Both host loops now defer that title frame whenever it queues a movie. The next iteration starts the movie before any title presentation.

Native first-frame comparison shows the prior title/prompt replaced by the warning video's black opening frame. A 60-frame capture shows the warning itself. A 1231-frame isolated run verifies video indices 9, 8, 0, then intro skip and return to title prompt/music at the saved gain 27. All three runs exit zero. Captures and logs use the `startup-*` prefix under `out/live-review-20260912/`.

Startup follow-up candidate SHA-256: `73257eb12eb0e6b7c101e0e385e3faa404eb3375d2f26ae81274189971c0edbc`. Initial deployment was blocked by the running game. A hidden one-shot helper waits for exit, checks that the installed build has not been superseded, backs it up, copies this immutable candidate, and verifies the installed hash. Status: `out/live-review-20260912/startup-order-deployment.json`.
