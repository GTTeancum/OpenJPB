# Native mod integration — work in progress

The active goal is native character and resource packages under the game-root `mods` folder, copying legacy assets and preserving `game.exe`, its proxy and original files. This is not yet ready for live review. The separate, already-tested startup-order candidate was deployed and hash-verified on September 12; the incomplete mod build remains isolated under `out/mods-integration/runtime`.

## Implemented foundation

- `mods/<package-id>/mod.json`, version 1, with `character` and `assets` package types; package-relative `res/...` structure.
- Transactional discovery, stable package identifiers, explicit character IDs 115–254, enabled/hidden flags, animation and force donor metadata, sound-bank metadata, colors and icons.
- Required character files are checked for existence and containment. Duplicate identifiers, malformed manifests and conflicting resources reject the set. Identical shared resources are accepted. Disabled packages are skipped.
- Resource overlays through the original resolver, file-I/O boundary, PC default assets, images and audio. Paths originating inside a package resolve other enabled overlays first, then the installed resource tree. JPX, FBX, collision/cameras, textures, effects, animations and sound resources retain their existing `res/...` names. New level registration and authoring/export are not implemented.
- Copy-only migration of all 14 configured additions, including referenced textures, portraits, CAD/CMB and their shared Huffman tables: 132 copied files, with source/destination SHA-256 verification. Staged packages are in `out/mods-integration/packages`; no live `mods` directory has been populated yet.
- Native startup discovery; diagnostic `--mods-root <game-root>` and `--no-mods` switches. Enabled characters participate in campaign, VS and training selector bounds and validity checks. Names, portraits and color toggles use mod metadata. One- and two-player campaign menu rendering is verified; VS/training still need runtime verification. Asset paths are bounded to the native 255-byte limit.
- Runtime combat retains stock animation-donor IDs, with separate per-player mod identity. Character assets use package paths, audio resolves the declared bank, and saber attachments use the model's own named weapon/collision nodes. Saber colors are per package rather than shared donor colors.

## Verification

- `jpb_mod_tests`: JSON validation, disabled packages, transactional failure, traversal rejection, resource conflicts, identical duplicate resources and reset.
- All 14 configured characters pass a 30-frame native hidden-window FED smoke run at a visible fixed position. All 13 Jedi have matched saber attachments and cores; Jango has no saber. These are idle/load checks, not combat validation.
- Kit Fisto's native image shows the custom model and green attached blade: `out/mods-integration/kit-saber.png`.
- Coleman and Ima-Gun Di run together using the same Maul animation donor, with separate green/blue saber colors and matched attachments: `out/mods-integration/two-mod-colors-run.log`.
- Mod, menu, game, title-runtime and PC audio regression suites pass: `out/mods-integration/regressions.log`.
- Smoke logs and migration hashes: `out/mods-integration/runtime-smoke.json`, `out/mods-integration/migration.json`, and per-character logs.

## Required next work

1. Finish VS/training and return-to-stock selection verification. Campaign menu selection, names, portraits, color toggles and handoff are implemented. Selector entry clears stale runtime mod identity; normal level handoff resolves an active mod independently of its donor ID.
2. Complete identity persistence through returning to menus, both-player changes, save/reload and removal. The one-player level transition now retains the package without a diagnostic model override. `GameStruct` and the legacy save have fixed stock arrays; do not expand their binary layouts. Stable package identity now has a native save sidecar and a verified fresh-process Continue path. Current progression is still donor-based; two-player persistence and return-to-stock lifecycle coverage remain open.
3. Implement mixed animation/force donor behavior. Activation now selects `forceDonor` recipes, including Maul motion adaptation, and the delayed ring callback uses the Force donor. Further callback/ability and native combat verification remain open. The legacy proxy's hybrid templates and existing progress sidecar are migration references, not already migrated functionality.
4. Audit combat, hurt/death/respawn, combo timing, collision sizes and audio for each donor family, then campaign/VS and two-player lifecycle tests. Native skeleton attachment lookup fixes idle blades but does not establish combat parity.
5. Review stock roster hidden settings. Sound overrides are now read from the roster with new-entity fallback; all 14 current additions specify inheritance. Current migration covers runtime dependencies, not Blender source projects under `ZMODS`.
6. Document the finished user package contract, handle missing/removed mod saves, copy tested packages to live `mods`, deploy the final executable and verify originals remain unchanged. Keep the broader Blender character/animation/level authoring work explicit in TODO.

Canonical stock behavior must continue to come from the local EXE/PDB/assets. Mod-specific extensions use the configured legacy features and authored model data.

## Menu and transition checkpoint

- `menu-kit-headless-run.log`: normal single-player selector passes its compositing validation; Kit Fisto's custom name and portrait are rendered.
- `menu-two-mods.png` / `menu-two-mods-run.log`: Kit Fisto and Aayla shown together. Smaller mod-specific color prompts avoid colliding with stock Exit/Select prompts; stock prompt sizing is unchanged.
- `menu-mod-next-run.log`: navigation crosses from mod ID 115 back into the valid stock roster. Forward wrap and all selector families still need explicit tests.
- `menu-handoff-native.log`: selected ID 115 passes through level selection, gameplay construction, intro FMV, and gameplay.
- `mod-transition-no-override-native.log`: level-completion fixture goes to level 2 / marsh with ID 115 and its package CAD/BMD, without the diagnostic override reapplying it. This proves in-process continuation, not save/reload.
- The `--review-menu 14` fixture now establishes one player before invoking that owner; previously it had zero players and could render but not process selection input.
- `menu-regressions.log`: mod/menu/game/title-runtime/audio suites pass. New mod tests cover selector validity, tab exclusion, color/icon toggles and runtime donor/package identity mapping.

## Shared-resource checkpoint

- The registry tracks the installed asset root separately from package roots. This also supports diagnostic packages outside the installed game tree. Package-relative resource paths fall back to the base installation; saves and unrelated files are excluded from this remapping.
- Shared effect and audio roots use the base installation even when JPX comes from a package. Individual shared resources still honor overlays.
- The low-level read path and image loader use the same resolution policy. FBX discovery honors FBX-only overlays and falls back to base FBX when omitted, matching the normal level constructor. As in the base game, FBX supplies the rendered level geometry when present; changing JPX alone does not replace that separate FBX geometry.
- Saber attachment logic is in `mod_saber.cpp`, separate from the filesystem registry. This keeps resource-only tests and consumers from linking gameplay/rendering owners.
- `resource-regressions.log`: mod, resource, file-I/O and PC audio suites pass. Tests prove package-to-base fallback through actual file reads, stock-path-to-overlay resolution, base-path derivation and exclusion of save files.
- `level-overlay-complete-native.log`: FED JPX is loaded from an isolated assets package alongside Kit Fisto; base FBX, collision/cameras, effects, textures and audio remain available.
- `fbx-only-overlay-native.log`: a package containing only FED FBX replaces the geometry file while JPX and other resources come from the base game.
- `level-overlay-handoff-native.log`: normal character/level-menu handoff and canonical level construction pass with packaged JPX and character assets.
- These fixtures copy stock level files to prove resolution and loading. They do not establish authoring, a new level, or altered collision/script correctness. They live in separate `level-overlay-game` and `fbx-overlay-game` test roots and must not be deployed with migrated character packages.

Next implementation priority is hybrid force behavior and the remaining two-player/VS/training lifecycle checks. Legacy save files are shared under `SAVEDATA0`; the native sidecar is separate from legacy proxy progress/configuration files.


## Save/reload checkpoint — September 13

- Native identity and selected saber color/icon are stored in `Game.mods.json`, leaving the raw 4,624-byte `Game` layout and legacy `jpb_progress.json` unchanged. Stable package IDs survive reassignment of numeric model IDs. The raw file retains stock donor model IDs for the original executable.
- Sidecar snapshots match the exact raw payload and its file modification time. Rewriting a save with the original executable invalidates stale native identity even when its payload is identical. Atomic temporary-file replacement retains the prior matching snapshot if replacement of the raw save fails.
- Tests cover actual Windows file-lock replacement failure, recovery of the previous identity, numeric-ID reassignment, disabled/missing packages, stale timestamps, malformed metadata and the normal save API round trip.
- The title owner resets stock model selections. Choosing Continue now republishes saved native package identity before player-count handoff; stock selections retain their existing behavior. This is covered by a menu-boundary regression.
- `save-create-native.log`: Kit Fisto completes the FED fixture, transitions to marsh, and writes the native sidecar into an isolated persistence directory.
- `save-continue-native.log`: a separate process, without a player-model override, chooses Continue and loads level 2 with package ID 115 and Kit Fisto CAD/BMD assets.
- `save-missing-native.log`: launching without mods rejects the save with `Required mod package is unavailable: kitfisto`; SHA-256 checks confirm the raw save and sidecar remain unchanged.
- Mod/game/menu/title regressions pass; mod/menu tests were rebuilt and rerun after the Continue correction. This checkpoint does not establish independent per-mod progression or full two-player persistence. No mod build or packages have been deployed.


## Hybrid Force checkpoint — September 13

- Activation copies the configured Force donor's stock recipe into a local map. Single-blade Maul Force inheritance uses playable Maul's recipe, matching the legacy package implementation. Shared stock maps and runtime animation-donor identity are not mutated.
- For Maul-animation packages, recipes with motions outside the loaded CAD's bounds use legacy fallback motion 67. One chain entry is retained if present because Plo's spin effect is installed by that entry. Other animation donors retain their original recipe motions.
- The delayed ring callback uses the Force donor for the Force-cost gate and projectile type. This fixes Qui-Gon-ring selection on Maul bodies after activation has returned.
- `jpb_mod_tests` exercises Plo's recipe on a Mace body and a 92-motion Maul body, verifies installed callback IDs and unchanged stock maps, and checks the delayed callback's depleted-Force gate. These are activation/branch tests, not native animation-completion proof.
- Stock Force activation and the full Force callback suite pass. Existing fixtures needed their `globalID` fields initialized to match the current animation implementation; this included the mesmerize target's motion 61. No animation production changes were made for those fixture repairs.
- Still required: native repeated activation/recovery and combat checks, further donor-specific callback/ability handling, and multiplayer lifecycle verification. The mod build remains isolated.


## Native Force activation/recovery checkpoint

- The gameplay brain's Force-capability gate now accepts an active Jedi package even when its animation donor is single-blade Maul (stock ID 9). Previously that gate prevented the newly implemented Force map from being reached. Stock/no-package capability rules are unchanged.
- `smoke_force.py`, `force-120/121/126/128-run.log` and `force-smoke-verified.json`: two primary Force activations followed by walking in native hidden-window runs. Tosan and Ima-Gun Di consume 30 Force through Qui-Gon's recipe; Temple Guard and Shaak Ti consume 40 through Plo's recipe. Every run ends with no Force callback and 29 walking frames after the two activations.
- `force-two-player-run.log`: Ima-Gun Di and Shaak Ti (same Maul animation donor, different Force donors) activate simultaneously twice. Their remaining Force is independently 70/60, both callbacks clear, and both resume walking. This checks the per-player map copy in the runtime as well as the individual recovery path.
- Native tests explicitly select Classic controls and use process-local `lb+y` input. An initial run using persisted Modern controls did not activate the intended power and is not counted as Force evidence.
- Mod, Force activation, Force callback and brain-control regression suites pass after rebuilding. Higher Force slots, remaining capability inheritance, combat/death/respawn, VS/training and save lifecycle checks remain open. No deployment yet.


## VS and training checkpoint

- `versus-mod-play-native.log`: the native VS selector accepts Kit Fisto (115) and Aayla (116), hands both into arena level 25, and constructs two players using their separate Mace/Adi animation donors. After the level-introduction prompt, both receive 30 walking frames and return to idle. No load error occurs.
- `training-mod-play-native.log`: normal training character navigation wraps from stock Obi-Wan to Shaak Ti (128), without a command-line player override. Selection and training-level confirmation load train1, and the player walks for 30 frames after dismissing the introduction.
- Native output images are `versus-mod.png` and `training-mod.png`. Initial captures were stopped at the introduction and were not treated as active-gameplay proof; the follow-up runs dismiss it before exercising movement.
- The direct `--review-menu 13` diagnostic now establishes two players, matching the normal VS entry requirement. Previously entering this owner directly could have zero configured players. Normal menu ownership is unchanged.
- These checks establish menu selection, asset handoff and controlled gameplay in VS/training. Full rounds, death/respawn, exits back to stock characters and two-player save lifecycle are still separate requirements.


## Shared-donor campaign transition and checkpoint save

- The two-player completion fixture now preserves an explicit P2 model and marks its seeded campaign continuable. Its old fallback always replaced P2 with Qui-Gon; earlier results using that fallback do not prove two-mod save behavior.
- Two different packages sharing Maul's animation donor exposed two canonical-constructor assumptions: `maModelData[donor]` points to the last loaded archive, and model caching uses the supplied character name. Mod player views now bind from their own constructed root/archive, and character construction uses the package model name to prevent donor-name cache aliasing. Stock construction retains its names and lookup behavior.
- The corrected pair (Ima-Gun Di 121 and Shaak Ti 128) now completes both summaries and loads/renders marsh successfully. The previous failure stages were `canonical-constructor:player-model-view` and then `frame:model-player-two`; both are resolved by the two changes above.
- Native mod campaigns now write a stable-ID checkpoint after successful campaign construction when AutoSave is enabled. This covers summaries without an upgrade/combo award, which otherwise do not call the save hook. The checkpoint marks the campaign continuable; VS/training and failed constructors do not take this path.
- `save-two-create-native.log` / `save-two-create-run.log`: successful level-two transition and a save containing both stable package IDs. This is isolated under `save-two-player`; live saves and legacy proxy files remain untouched.
- Loader, game, mod and title-runtime regression suites pass after rebuilding. Fresh-process two-player Continue and AutoSave-disabled verification are the next gates for this checkpoint.

- `save-two-continue-native.log`: a fresh process selects two-player Continue without model overrides and loads marsh with package IDs 121/128; it exits successfully after rendering both players.
- `save-two-disabled-run.log`: the same fresh-process Continue with AutoSave disabled records zero writes. SHA-256 comparisons prove the raw save and native sidecar remain unchanged; the isolated test's options are restored afterward.


## Package contract and cache validation

- [MODS.md](MODS.md) documents version-one manifests, installation/removal, resource fallback/conflicts, save coexistence, the copy-only migration command and current limitations.
- Canonical construction now generates bounded cache names (`native_mod_<modelId>`) instead of using the resource stem directly. This prevents aliasing with stock names, other packages or truncated long names without restricting asset filenames.
- Manifest icon values are checked against the actual 249-entry menu texture table, matching the sidecar's existing bounds check. An out-of-range-icon regression proves invalid manifests are rejected transactionally.
- Mod and loader tests pass. `model-cache-key-native.log` and `model-cache-key-run.log` confirm fresh-process two-player Continue with IDs 121/128 still loads and renders marsh after the cache-key change.
- Broader combat/exit checks, independent progression, final migration/deployment and completion audit remain open.


## Upgraded Force slots and effect colors

- `smoke_force_slots.py` and `force-slots-verified.json` cover the two upgraded Force slots for Tosan, Ima-Gun Di, Temple Guard and Shaak Ti with progression unlocked in the process-local harness. All eight cases activate repeatedly, clear the callback and resume walking for 29 frames.
- Qui-Gon hybrids retain 80/60 Force after two uses of the respective upgraded slots. Plo hybrids retain 20 after two zap uses. Their middle slot is the held reflection callback: two 45-frame holds retain 56 Force. One-frame taps selected that animation but did not exercise the sustained effect and are not counted as consumption evidence.
- Reflection now reads the active package's current saber color rather than its animation donor's color. `jedi_GetColour32` accepts a registered mod ID; color-toggle tests cover its output. Plo zap and Adi cloak color branches now use the declared Force donor; the ring callback shares that same donor resolver.
- Mod and stock Force callback tests pass after rebuilding. Native Shaak Ti reflection/zap runs with the effect-color changes complete and regain movement (`force-128-slot2-effects-run.log`, `force-128-slot3-effects-run.log`). This is runtime safety/recovery evidence; no pixel-color parity claim is made.
- Progression isolation/migration, exit/return-to-stock and wider combat/respawn checks remain open before final deployment.


## Progression storage checkpoint

- Native completion storage now maintains 30 completion flags and best scores per stable package ID outside `GameStruct` and the raw legacy save. `jpb_ModRecordLevel`, `jpb_ModLevelPlayed` and `jpb_ModLevelScore` expose bounded access; recording a lower score preserves the best score.
- Native sidecar snapshots include these records, including records for currently absent packages. Reading validates the entire completion object before changing identity, colors or progression. Older sidecars without this optional field still load.
- Tests cover persistence, numeric-ID reassignment, rejection of stock/out-of-range recording, best-score retention and transactional rejection of a score above UINT32_MAX. The mod test suite passes.
- This is storage groundwork, not completed independent progression. Results recording, selector queries, upgrades/combo masks, legacy aggregate-progress conversion, new-game/reset semantics and coexistence when the original executable changes the raw save still need integration. The running native test executable has not yet been rebuilt with this storage change, and no deployment occurred.


## Results-to-completion integration

- `jedi_GetAwardFlags` records completion and best scores by the active package ID. Mod completions do not mark stock donor completion/score arrays or call the stock-character completion achievement path. Existing award calculation and upgrade storage are not yet isolated.
- `jedi_CalcSkillLevels` reads a mod's own completion flags for its highest completed level. Skill percentage still uses donor upgrade data until the remaining upgrade integration is complete.
- Tests exercise the real results function, prove the donor completion flag remains clear, and verify different scores for two packages sharing the same donor. Mod, game and menu regression suites pass.
- `progress-two-create-native.log` / `progress-two-create-run.log`: Ima-Gun Di and Shaak Ti complete FED through the native two-player results flow, continue into marsh and checkpoint. The sidecar holds separate FED completion records, each with score 20020. Inspection of the raw save confirms Maul's FED completion byte remains zero.
- Remaining progression work: per-package upgrade/combo state, legacy aggregate migration, reset/stale-sidecar semantics and complete selector/progression integration. Deployment is still pending.


## Independent combos and upgrades

- Combat combo checks, award menus and initial combo setup resolve a stable package's six-byte mask. Canonical starting masks are seeded once; stock masks stay separate. Save/restore and numeric-ID reassignment tests cover independent masks for two packages sharing a donor. `combo-mask-continue-native.log` confirms fresh-process two-player Continue and clean shutdown.
- Package records now also retain `Upgrades`: health/Force counts, attack/defence bonus bits, lives, Force-power flags and 12 award tiers. The optional sidecar `upgrades` object is fully validated before any saved state is applied. Older snapshots initialize canonical new-character defaults lazily.
- Results award calculation, skill queries, level-menu award queries, extra lives, Force availability and combat damage bonuses resolve package upgrades. Health/Force award menu triggers increment the package counters. Loading/respawn and HUD scale use capacities derived from those counters (100 + 20/count; bar length 25 + 5/count), matching the local stock initialization and award increment code. The playable Maul donor's initial Force mask is retained without overwriting package progress on each actor initialization.
- Tests cover real result awards and health-menu triggers, two mods with the same donor, stock donor isolation, malformed upgrade rejection, save/reload and numeric-ID reassignment. Mod/game/menu/combo/Force activation/brain-control/loader/player/damage suites pass after rebuilding the affected targets.
- `upgrades-two-continue-native.log` and `upgrades-two-continue-run.log`: a fresh native process loaded an isolated two-player save with different synthetic upgrades for Ima-Gun Di and Shaak Ti. Gameplay reported P1 health 160/160 and Force 140/140, P2 health 120/120 and Force 180/180; the resulting checkpoint retained both records. The raw legacy payload remained 4624 bytes, and Maul's stock maximum health remained 100. Clean shutdown completed. An initial evidence-copy command used the wrong log filename; the actual native `jpb_pc_game.log` was then preserved and checked without rerunning the game.
- Remaining before live deployment: legacy aggregate-progress conversion, reset/original-executable rewrite coexistence, exit/return-to-stock and wider combat/respawn validation. This build remains in the isolated runtime staging directory; live game executables and legacy assets have not been changed by these tests.


## Legacy aggregate migration and reset coexistence

- Local proxy source `sdl2_image_proxy.c` lines 3323–3509 establishes that `jpb_progress.json` holds aggregate highest level and skill percentage, not gameplay point totals. It explicitly documents no in-session stat tracking and default 10/100 for new Jedi (0/0 for other additions).
- The copy-only migrator now preserves that file byte-for-byte under `mods/_legacy/` and generates `mods/legacy-progress.json`, keyed by stable package ID. All 14 configured additions receive their persisted aggregate or the proxy's documented default. The installed originals are not edited. Repeating migration succeeds; all 133 copies match their source SHA-256 values (`migration-with-progress.json`, `migration-with-progress-repeat.json`).
- The registry validates the optional import transactionally. Skill/level queries use the greater of earned native progress and the imported baseline, without synthesizing completed levels, point scores or award tiers. Unit tests cover bounds, failed-load preservation and those distinctions.
- The explicit New Game menu action now clears earned mod records and active package slots alongside stock progress. The shared `newGameGameInit` initializer also runs during level construction, so it must not clear native package records. Imported unlock baselines remain immutable initial settings. A stale native sidecar after original-executable save activity restores only independent progression from its newest record; it cannot impose old package selection/colors or require a now-disabled character. Exact matching snapshots retain normal identity validation.
- Mod, game and menu suites pass after rebuilding the new migration/reset/coexistence implementation. Tests cover a disabled package, a changed raw-save timestamp, preservation of progression without identity, re-enabling the package and New Game reset.
- Deployment still awaits final runtime exit/combat checks. The first Kit Fisto death/restart harness passes; full-roster evidence is in progress.


## Full-roster restart and attack recovery checkpoint

Delivery follow-up: the user explicitly requests finishing mod support, committing and pushing, then implementing animation blending to smooth state changes. Current branch is master with remote OpenJPB. Preserve unrelated working-tree edits when preparing the commit.

Exit validation: the normal Continue flow now exits two-player gameplay to title mode with stock selections 0/1 and clean shutdown (continue-exit-two-mods-native.log, frame 461). The harness must dismiss the level-two objective before Start; earlier runs that did not reach exit are not evidence. Direct quickload exit still crashes in newDrawControllerIcon at RVA 0x8cdfb because that entry path skips front-end texture initialization. Fix that lifecycle path before delivery. The direct quickload evidence is exit-two-mods-native.log.

- `smoke_combat_restart.py` and `combat-restart-smoke.json` cover all 14 configured additions. Each passes the process-local death/restart validator, then a hidden native run exercising the three attack inputs and walking recovery. Every run exits cleanly and reports 29 walking frames after recovery. This proves loading, attack execution/recovery and the restart path; it is not a full campaign combat-balance or damage-contact test.
- The first roster batch used the build before the reset boundary was narrowed. Inspection found the shared `newGameGameInit` also runs during portable level construction. Only the explicit New Game menu action now calls `jpb_ModResetProgress`; a regression checks that shared initialization preserves upgrades and the menu action clears them.
- `reset-boundary-continue-native.log` / `reset-boundary-continue-run.log` verify the corrected build in a fresh two-player Continue: IDs 121/128 reach level two with health/Force 160/140 and 120/180 respectively, then checkpoint and exit cleanly. Mod/menu tests also pass.
- Remaining immediate delivery gate: exit/return-to-stock validation and final package/executable deployment with legacy-source preservation checks.


## Delivery audit (pending final regression/deployment)

| Requirement | Current evidence |
| --- | --- |
| Game-root mods with preserved resource substructure | live-migration.json: 14 installed packages, 133 verified copies; manifests and res subtrees match staging. |
| Copy originals and retain original-executable compatibility | Migrator never moves/deletes inputs; hashes of all sources and installed copies agree. Original proxy/config/patcher and executable are retained. |
| Character and animation integration | All 14 selection/loading/attachment and attack/restart checks; native Force hybrid and two-player same-donor evidence in preceding checkpoints. |
| Level/resource support | JPX/FBX/resource overlay loading, stock fallback and conflict checks documented above. New level registration and Blender authoring remain separate pipeline work in TODO. |
| Stable progress and save coexistence | Completion/combo/upgrade tests, two-player fresh Continue, missing-package handling, original-save rewrite and New Game boundary tests; imported legacy aggregates retain unlocks without fabricated history. |
| Menu exit and stock selection reset | continue-exit-two-mods-native.log: campaign return to title, models 0/1; versus-exit-mods-native.log: VS return, models 0/1, clean shutdown. Existing two-player input lifecycle test passes. |
| Direct quickload return | quickload-exit-fixed-native.log: lazy front-end resource initialization before first title frame, clean shutdown; keeps game/menu state intact. |
| TODO closure and level-two report | TODO records Audio and Display closed and level-two completion without sound testing. Native runtime checkboxes are completed; authoring/editor items remain open. |
| Final executable delivery | deployment.json: installed executable SHA-256 86842f2d83f637ef051be5510bb18dc3d2b525c2f338148f07ada20a10d41df6; previous executable backed up and six protected legacy files unchanged. |
| Commit/push then animation blending | Explicitly authorized by user; pending delivery checkpoint. |


## Native runtime delivered

- Full Release build succeeds. All 97 unit regression tests pass, plus the two-player input lifecycle check. The texture regression previously asserted an unusable cached material after failed loading; it now verifies the existing cache fix retries failures and successfully recovers when the resource becomes available.
- Installed 14 packages and 133 copied files in the game-root mods folder. All copies and original inputs match their migration hashes. Six protected original-executable/config/patcher/proxy/progress files are unchanged in deployment.json.
- Deployed jpb_pc_game.exe, preserving the prior executable as out/mods-integration/before-native-mod-support.exe. SHA-256: 86842f2d83f637ef051be5510bb18dc3d2b525c2f338148f07ada20a10d41df6.
- installed-smoke-native.log verifies the installed executable discovers all 14 packages without a mods-root override and loads the installed Ima-Gun Di assets. Two-player gameplay and clean shutdown pass using isolated persistence; the native framebuffer was inspected. Original saves were not used by the smoke run.
- This completes native character/animation/resource-package integration and copy-only legacy migration. Blender tooling, additional-level registration/export and new-level authoring remain explicitly separate TODO items. User-requested commit/push is next, followed by animation-state blending.
