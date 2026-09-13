# TODO

## Open Items

September 12: removed live-confirmed items and renumbered. Autosave into level two is independently confirmed. Details and prior numbering: [LIVE_REVIEW_20260912.md](LIVE_REVIEW_20260912.md).

1. Fix second FED boss phase-two blaster audibility.
2. Mod support: characters, animation, and level design through JSON DLC and Blender.
3. Confirm startup legal/video ordering without an early Press A flash.
4. Updated Blender plugin for character model/animation
5. Level editor plugin for Blender
6. Animation blending for smooth transitions (next after mod-support deployment, commit and push)

## 1. Second Boss Phase-Two Audio

- [x] User confirmed phase-one blasters are audible.
- [x] Trace phase-two firing: CAD motions 8/10 use ai_FireWeapon and projectile 20, whose authored sound field is empty; phase one explicitly plays dfrblstr.
- [x] Add the existing fighter cue to phase-two firing events; repeated-shot regression, both native phases with boss music, and non-silent mixer PCM checks pass.
- [ ] LIVE: Confirm phase-two shots are audible.

The selected music-gain fix remains in place. Earlier audio evidence: [AUDIO_MIX_20260911.md](AUDIO_MIX_20260911.md), [FED_ENDING_AUDIO_20260911.md](FED_ENDING_AUDIO_20260911.md).

## 2. Mod Support: Characters, Animation, and Level Design

- [x] Quick assessment of the existing proxy source, patcher source/configuration, and reconstructed character registry completed on 2026-09-08.
- [x] Quick level-editor assessment: the runtime already imports FBX level geometry, and J3D loading exposes authored scripts and placements. Blender is a feasible authoring foundation; complete level editing still requires game-specific import/export and validation.
- [ ] Design a shared, versioned DLC package structure for characters, animation resources, and levels, with per-package subfolders, stable IDs, dependencies, and JSON manifests. Use Blender's existing editing tools and standard asset formats with a JPB add-on; avoid a bespoke standalone editor.
- [x] Define a versioned `mods/<package-id>/mod.json` package with its own asset subfolders, stable character ID, display name, portrait, model, base-character inheritance, combat/Force abilities, saber settings, and inherited or custom sound/animation resources.
- [x] Implement native package discovery and validation, package-relative asset loading, and a character registry that handles roster/menu growth without colliding with stock player or level-object IDs. Audit fixed arrays and narrow ID fields throughout selection, loading, combat, and saves.
- [x] Replace donor-animation/combo, weapon/bone, ability, sound, and progression hooks with source-level support. Preserve stock behavior when no DLC is installed.
- [x] Support adding/removing or disabling packages without executable patching; retain progress by stable package ID and handle missing packages, duplicate IDs, invalid manifests, and missing assets cleanly.
- [x] Convert existing Kit Fisto and Aayla packages/configuration as initial examples, document the package format, and test selection, loading, combat, save/reload, removal, and both players in campaign and VS.
- [ ] Add a Blender model/animation pipeline for improved assets: define supported rigs, bone/weapon attachments, scale and axes, animation mapping, combo timing and gameplay events, materials, and export validation. Support inherited and custom animation sets through native runtime loading.
- [ ] Add Blender level creation and modification: geometry, collision, props/enemies, spawn points, waypoints, cameras, triggers, and scripted encounters. Import existing geometry and J3D gameplay data while preserving references and behavior; provide editable event properties or nodes for the engine's existing script commands.
- [ ] Implement level package loading and export/build processing for collision and spatial data, script references, level registration, and progression. Assess reliable round-trip support for existing JPX/J3D assets versus a new runtime format; new formats and replacement of the old pipeline are authorized.
- [ ] Validate the shared pipeline with an updated animated character, a modified stock level, and a small new playable level, covering collision, encounters, camera/cutscene completion, save/load, and two-player play. Document supported features and any import/export limitations.

Assessment: the existing code is at `C:\Games\Star Wars Jedi Power Battles\proxy_dll\sdl2_image_proxy.c` (the supplied `proxy\_dll` path does not exist). The game root includes `JPB Patcher.exe`, its `jpb_patcher.py` source, and `jpb_config.json`. The patcher rewrites executable tables; the proxy adds runtime roster, asset, donor-behavior, and save hooks, including a `SAVEDATA0/jpb_progress.json` sidecar. The current configuration contains 14 additions, including Kit Fisto and Aayla. The reconstruction still has a fixed 14-entry extra-character table and stock model IDs that also cover level objects, so this requires registry and loader integration beyond reading JSON. A native DLC system is a viable direction; replacing the old format/system is authorized. Use the mod code as a feature/migration reference and local canonical artifacts for inherited stock behavior. Native integration is now in progress. User requests a mods folder with the same resource substructure and copy-only migration, preserving legacy originals.

Native implementation checkpoint: [NATIVE_MOD_SUPPORT.md](NATIVE_MOD_SUPPORT.md). Copy-only staging and initial native load/attachment checks pass for all 14 additions. Campaign character selection and in-process level continuation now pass; fresh-process save/reload now retains the package. VS/training selection and gameplay movement also pass. Hybrid Force use and fresh-process two-player persistence now pass. Completion, combos and upgrades are isolated by package, including separate runtime health/Force capacities for two characters sharing a donor. Legacy aggregate unlock conversion and reset/coexistence regression checks now pass. All 14 additions pass attack/recovery and death/restart smoke tests. Campaign and VS exit/return-to-stock checks now pass. The direct quickload exit also initializes its missing title resources correctly. Native mod support is deployed: all 97 unit regressions pass, the installed executable discovers all 14 copied packages automatically, and the installed two-player smoke test passes. Legacy originals are retained. Blender authoring and new-level registration remain open below.

## 3. Startup Ordering Follow-up

- [x] Reproduce the first-frame Press A flash before the legal/warning video.
- [x] Defer title presentation when its menu owner queues a movie, matching the retail blocking movie boundary in both native and headless loops.
- [x] Native first-frame comparison and full sequence pass: opening video frame, warning, Aspyr, intro, then title prompt/music.
- [ ] LIVE: Confirm Press A no longer flashes before the legal screen.

Evidence: `out/live-review-20260912/startup-before.png`, `startup-after.png`, `startup-warning-order.png`, and `startup-full-order-native.log`. This is a follow-up to the previously closed startup feature, not a reversal of the confirmed legal-screen implementation.

Latest live review: level 2 completed without issues; sound was not tested. Former items 2 (Audio settings) and 4 (Display) closed as requested. Boss audio remains unconfirmed.
