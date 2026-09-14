# TODO

## Tracked Items

September 13: removed user-confirmed items 3, 4, 6, 7 and 8, then closed the second FED boss audio issue and renumbered. Closure details: [LIVE_REVIEW_20260913.md](LIVE_REVIEW_20260913.md). Earlier review: [LIVE_REVIEW_20260912.md](LIVE_REVIEW_20260912.md).

1. Mod support: fix custom saber textures and Coleman Trebor crash; continue character, animation and level authoring support.
2. LIVE: Blender level importer (import only).

## 1. Mod Support: Characters, Animation, and Level Design

- [x] Fix custom saber icons for all mod characters, including alternate variants. Copy-only migration and explicit package image paths replace legacy concept-art slot references; stale current icon/color pairs are handled.
- [x] Fix Coleman Trebor's post-intro BMD rendering failure by reusing P1's package model registration for the hidden single-player hierarchy.
- [x] Earlier direct-selection smoke completed, but was insufficient for roster approval; superseded by the title-driven review below. Historical evidence: [MOD_CHARACTER_FIXES_20260913.md](MOD_CHARACTER_FIXES_20260913.md).
- [x] Deploy the tested mod-fix executable; installed OpenJPB.exe matches the final tested build (2026-09-13).
- [x] Fix the reopened LIVE failure: canonical loading retains package BMD/CAD/CMB paths, and the menu texture bank accommodates added portraits/saber icons. All 14 additions pass normal boot/title/menu virtual-input review: actual custom geometry identity, all 13 alternate sabers, attacks in both sessions, and quit/reselect/reload. 28 handoffs pass; ready for LIVE confirmation. Evidence: [MOD_ROSTER_REVIEW_20260913.md](MOD_ROSTER_REVIEW_20260913.md).

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

## 2. Blender Level Import — Import Only

- [x] Import native FBX visual geometry and paired J3D gameplay data using the runtime readers.
- [x] Add collision, placements, waypoints, powerups and start points in matching character-tool coordinates.
- [x] Preserve camera/script metadata, references and the original archive without changing source files.
- [x] Validate all 24 available stock level pairs; Blender 4.5 imports, textures and save/reopen pass for FED, Theed and Palace.
- [x] Install version 4.1.0 with the native helper, retain a 4.0 backup, and verify the installed importer with a Theed render.
- [x] Replace placement/pickup crosses with actual game models in 4.1.1; Theed verifies 245 placements and 31 pickups. Add labeled start indicators and a JPB object inspector.
- [x] Extend authored-data import in 4.2: active CAM records and camera regions, map-trigger regions and target references, complete script node graphs, placement/waypoint connections, both PWR layouts, level animation tracks, source-sidecar preservation and direct JPX geometry. FBX/JPX navigation and save/reopen checks pass.
- [ ] LIVE: Review the imported scene in Blender. Level export remains deferred as requested.

Usage and limits: [BLENDER_LEVEL_IMPORT.md](BLENDER_LEVEL_IMPORT.md).
Restart Blender and reimport the original BMD/CAD into a fresh scene; old broken imports are not repaired retroactively. Level editing remains item 3.
