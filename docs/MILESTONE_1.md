# Milestone 1: portable foundations

Milestone 1 converts raw symbol/decompiler evidence into reviewed C and C++.
The dependency-light PC build is the primary integration target. Xbox
portability constrains data layout and platform boundaries. The PC
reconstruction is intended to become complete; nxdk work resumes only after
the portable PC game is
fully functional and the user explicitly approves beginning the Xbox phase.

## Integration gates

A source shell becomes reviewed reconstruction only when:

1. Its public signatures and used data layouts are traced to the exact PDB.
2. Its control flow is checked against both decompiler output and machine code.
3. Pointer-bearing layouts have explicit 32-bit target assertions or avoid
   serialized-layout assumptions.
4. Reference quirks are preserved unless a portable substitution is labeled.
5. Host tests cover normal behavior and any surprising reference behavior.
6. Regenerating the evidence inventory leaves the reviewed source untouched.

## Current foundation order

The generated queue is in
[`inventory/FOUNDATIONS.md`](../inventory/FOUNDATIONS.md). `list`, `timer`,
the boundary-tag allocator, and the four game memory pools are fully reviewed.
All 29 `vectors.c`, all 49 `fmath.c`, all 32 `flex.c`, and all 23 `camera.c`
routines are reviewed. Twenty-three of 25 `scene.c` routines now own the exact
gameplay-camera-to-matrix path, scene transform publication, player-death
transition, and post-render timing/double-buffer state. Unnamed backing
objects retain explicitly inferred `jpb_` names. The scene geometry object is
now expressed as the exact offset-8 member of the 200-byte `gSceneRoot` PDB
global, and its developer-command tail uses reviewed `console_AddCommand`
storage rather than being omitted. The queue reports
reviewed and total procedure/byte counts separately so partial modules cannot
be mistaken for completed ones.

The current math foundation includes both square-root scales, fixed-angle
sine/cosine/atan2, three normalization entry points and their tail-call alias,
four distance variants, quick and planar range checks, vector length,
normal-to-rotation conversion, plane definition/projection, transform-global
setters, axis and ordered rotations, matrix/vector application, matrix
multiplication and inversion, scaling, translation, transpose, identity
initialization, and screen-depth evaluation. It preserves the executable's
unusual 57.2957 conversion constant, float-rounding point in `FindSinCos`,
32-bit length overflow, different float/double normalization paths, local
linkage for `gte_matrix`, translation retention/reset behavior, strict segment
bounds, unordered float-comparison behavior, low-word return values, and
non-alias-safe assignment sequences. Three vector rotation wrappers omit a
balanced pair of renderer matrix-stack calls to keep this layer portable; the
normally neutral state effect and otherwise indeterminate, unused return are
explicitly documented in the implementation.

The original `win32/IO.c` call surface has been retained as a platform seam,
but its reviewed PC implementation uses only standard C and a host-width
opaque handle. All 25 `filesys.c` procedures now sit above it, including the
exact 46-entry identifier decoder, complete target-format dispatcher, and
structure-heavy actor, AI, enemy, animation, library, and Jonny-map
relocations. Effects and the exact 50 resident sprites use narrow path,
texture, event, and UV hooks. The original C++ set/vector pointer registry was
replaced with an insertion-ordered C registry that preserves observable
indices without carrying a desktop container dependency into nxdk.

The dependency-light `jpb_asset_probe` exercises this boundary against
original data. All 27 installed level archives currently relocate with exact
byte consumption. Real files established two bounded-adapter rules that unit
fixtures alone did not expose: zero-record chunks remain valid after their
16-byte header has been consumed, and the terminal Jonny chunk's recovered
standalone handler advances 16 bytes beyond the header-declared payload.
The portable dispatcher keeps the exact handler but normalizes that terminal
cursor to the archive boundary. Setting `JPB_GAME_DATA_DIR` at CMake configure
time registers the owner's archives as labeled `real_assets` tests.

The dependency-light world seam uses the preprocessed JPX mirror loaded by the
legacy `InitJPX` procedure. PDB types `MATHEAD` (`0x1306`) and `_BINHEADER`
(`0x137F`) are asserted, and the bounded implementation reads through the
portable IO seam into caller-owned storage, resolves 16-byte renderer
descriptors through a hook, reproduces the reference's per-material and
per-site progress updates, and publishes `WorldmeshData` at `OfsToWorld`.
The caller-owned capacity replaces the reference's fixed 5 MiB arena without
changing the serialized format.

All 25 shipped JPX meshes pass the runtime path, including 3.6 MB Palace and
the 11,446-site Coruscant mesh. A pre-patch visitor covers 100,489 binding
sites without interpreting renderer data. Of these, 96,860 carry the stock
`STRPHEAD` marker; the 3,629 sites in `modhangar.jpx` carry a different
payload and are deliberately accepted because the relocation chain remains
valid.

Reference tracing established that the matched 2024 executable embeds exact
ufbx source version 6001 (ufbx 0.6.1) and does not call
`InitJPX` or the old `_CubeRender`/`_FatRender`/`_ThinRender` consumers during
live level rendering. Its executed path is
`ufbx_load_file` -> `_InitFBXLevelData` ->
`cube_NewWorldRender`/`_ApplyLevelTransformation` ->
`CD3DApplication::DrawLevel`. The PC reconstruction now follows that executed
FBX path through an isolated ufbx 0.6.1 adapter. It publishes flattened
triangle/material batches to the portable renderer, while JPX supplies
gameplay collision. Selecting JPX for visuals remains an explicit portable
fallback, not a claim about the matched PC executable.

The exact PDB globals `sLevelNames` (`char *[28]`), `level_offset`
(`float[26][3]`), `level_scale` (`float[26][3]`), and `startPos`
(`_svector[26][2]`) are now reconstructed. Review of
`cube_NewWorldRender` and the shipped `LevelVertexShader.hlsl` recovered the
complete x/z/y authoring-axis conversion and its 256/512 translation
constants. The transform was cross-checked against every available shipped
FBX level and its PDB start position; Streets lands exactly on FBX vertex
`{-120,-97,13}` and game position `{30976,3328,-24576}`.

The 28 bytes immediately preceding a binding site have also been decoded for
the exact static signature (`tag=-1`, node flags `1`, and zero low vertex
flags). Across the 25 installed meshes, 54,579 records match and expose an
index, x/z/y center, radius, and high-half vertex count. The other observed
node-flag classes, `0x80001` and `0x100001`, include non-coordinate payloads
and deliberately remain opaque rather than being guessed into a common
layout.

The strip payload itself is now decoded independently of those spatial
metadata classes. Every one of the 100,489 sites has a zero-low-half vertex
count in the preceding word, followed by a 16-byte renderer descriptor,
exactly `count * 16` vertex bytes, and 4, 8, or 28 bytes of metadata before
the next descriptor. Across the installed meshes this covers 2,000,131
vertices without exceptions. Position is signed Q7 x/z plus float y; texture
coordinates are signed Q12 u/v, while the final 32-bit attribute remains raw
until its bit semantics are proven.

`jpb_jpx_preview` now draws this stream as a height-colored triangle-strip
wireframe in top-down or perspective mode. `STREETS.jpx` produces 10,519
strips, 185,859 vertices, and 150,927 non-degenerate triangles with coherent
level geometry.
The reviewed `PerspectiveTransformFV` and `PerspectiveTransformManyFV` add
the executable's exact `1.0` near clip, `460.0` focal scale, `(320,240)`
screen center, and `10240.0` depth normalization. A separately labeled
portable look-at adapter now drives a perspective preview of the same stream;
it remains an inspection-camera option rather than a source of live gameplay
state. PDB types `Camera` (`0x121A`, 76 bytes) and `sceneGeometryEnv`
(`0x125B`, 176 bytes) are now asserted as pointer-free cross-target records.
The recovered camera slide, absolute/relative view conversion, and
`scene_UpdateWorld2ScreenMatrix` feed `jpb_ProjectCameraToViewport`, completing
an exact gameplay-state-to-projection path.

The minimal live PC scene/input loop is now operational. Four reviewed
`input.c` procedures preserve the game's mask, held-button, rising-edge, and
continuous-button semantics behind a raw-pad provider callback. JPX drawing is
isolated in a standard-C software renderer with caller-owned X8R8G8B8 memory.
The Win32 executable owns only keyboard polling, a high-resolution clock,
window messages, and `StretchDIBits`; no SDL or other desktop framework enters
the portable boundary.

The loop loads the original `STREETS.jpx`, constructs an exact absolute-focus
`Camera`, passes it through `camera_Camera2ViewVector` and
`scene_UpdateWorld2ScreenMatrix`, and draws 150,927 triangles as 326,267
visible line segments. Its headless CTest asserts that nonempty geometry
produces visible pixels, so a clipped or sign-inverted camera cannot silently
pass.

The movement-facing collision-node layer is also established. PDB type
`Mnode` (`0x119A`) is asserted at 152 bytes on the matched x64 ABI and 136
bytes on the future 32-bit target. Reviewed `collisn.c` procedures preserve
its registry, fallback, flag, rotation, velocity, translation, reset, and
registration behavior, including wrapped 16-bit increments and low-word
stores. Both large collision solvers are now readable: node-vs-node contact
publishes normalized hit velocity and motion, while projectile contact owns
authored radius modifiers, collision-node filtering, normal hit state, and
saber/Force reflection. Exact `_svector mReflects[5]` data retains its PDB
name and initialized executable values.

The player-state boundary now has exact `objectRoot`, `Pad`, `_mvector`,
`playerSettings`, and `playerObject` layouts plus the original 20-entry pool.
Six reviewed `player.c` routines preserve initialization, lookup,
requested-ID allocation, first-free fallback, `WorldData.player0/player1`
publication, and exact `player_AfterLife` power/item/lock-ring/shadow cleanup.
The 1,904-byte exact `player_RefreshPlayer` additionally restores authored
level/checkpoint/placement position, facing, score/Counter, energy/Force,
shadow ownership, reset fields, animation queue state, and scene publication.
The gameplay scheduler inside PDB-named `player_gProcessPlayers` is now
reconstructed and tested across RVAs `0xE8170..0xE8A7C`. Exact collision-first
ownership, the 20-slot stride, two/20 attacker budgets, two-player global bit,
map-trigger call, independent `input_ReadControlPad` channels, `mPlayerRead`
and `mCharliePad`, controller/AI routing, level/pause suppression, and
`brain_ControlPlayer` dispatch are readable source. The live PC field calls
that owner at the original post-render boundary. Exact `_AddLifeTile`, its
two tile emitters, camera-space transform, scaled energy/Force accessors, and
the scheduler's damage indicator now restore ordinary player HUD ownership
through the dependency-free PC framebuffer. The similarly named
`player_ControllerDump` and developer diagnostic labels remain presentation
work rather than hidden gameplay substitutes.

The pointer-free game-state boundary now has exact `CharacterData`
(`0x108C`, 28 bytes), `JEDICOMBOMASK` (`0x108A`, 6 bytes), and `gamestruct`
(`0x9ADA`, 4,716 bytes) layouts. The reviewed `game.c` procedures preserve
energy reads and changes, player-specific clamps and life-line scaling,
maximum energy/Force line ratios, Force and score clamps, level-eight
exceptions, game flags, item counts, timer-relative power state, and all nine
authored initial Jedi combo masks. Exact `game_initEnergy`, `game_initCombos`,
and `newGameGameInit` now own startup of the 23 model-resource rows, persistent
level/score tables, character data, AI bits, upgrades, completion flag, and
default 0/1 model pair. The PC field calls that original owner after clearing
its reusable host storage. These routines use only fixed-width C data and the
existing player pool.

The adjacent level lifecycle now includes exact PDB-named
`pwrup_LoadPoop`, `pwrup_Init`, `pwrup_LevelStart`, `pwrup_LevelEnd`,
`pwrup_JumpCheckPoint`, and the 4,144-byte `pwrup_CheckPowerUps` dispatcher.
Pointer-free 12-byte authored records are decoded into the exact native
intrusive `powerPoop` lists; the dispatcher preserves the double-buffer move,
range/pause checks, authored type-13 emitters, all pickup award branches,
checkpoint/continue publication, artifact bits, and after-life revival. The
matched scale, icon, random-power, display-name, and model-name tables retain
their PDB names. The PC field loads the selected `.pwr` through the original
resource owner and invokes the dispatcher after player processing; the FED
smoke proves all 55 records enter that live path. All 52 installed files are
independent real-asset gates. The removed immediate-mode `DrawPowerUp` mesh
backend remains a declared fidelity gap behind a narrow dependency-free draw
seam; the PC adapter currently realizes it as a world-space glint.

Exact `hurtplayer` now connects those leaves to scene death behavior. It
preserves the object/player gates, death flags, `player_AfterLife`,
hit/movement/model resets, and the instruction-resolved one-based
`gaPlayerData[playertankindex - 1].pEnemy` tank cleanup. Exact `sound_Play`
preserves the original bank fallback control flow over a game-owned
`sound_playSfx` callback. The dependency-free PC adapter binds that boundary
to WinMM while gameplay code remains independent of a desktop mixer API.

The exact map-contact event path is now connected as well. Pointer-free
`EffectData` and `EffectHeader` preserve the effect-file ABI, while
`ExtraCharacterEnvironmentEffectExceptions`, `HitsHit`, `MTV`, and
`clear_eventlist` recover threshold checks, map mutation, circular undo
records, and adjacent matching-channel propagation. Exact
`LaunchMapAnimEffects` uses the executable's `eventarray` and `maphitsounds`
tables, exact cube mesh visibility leaves, the reviewed `sound_Play` seam, and
exact `StopNearestFan`. `CheckCubeBlocking` therefore no longer needs a broad
contact callback. Exact `sprite_AddSpriteEffect`, `sprite_AddCallBack`,
`sprite_MainCallBack`, `sprite_FireRing`, and the sprite/SCB allocation leaves
now complete that path through real effect allocation, recursive emitters,
ring construction, callback motion, and list ownership. The temporary `jpb_`
effect backend has been removed.

The actor-state boundary now also has exact `sceneObject`, `modelObject`,
`physicsObject`, `_solid`, `_jheightstuff`, `Motion`, `_animFrame`,
`_dpcontext`, `animListNode`, and `animObject` layouts. Thirteen reviewed
`physics.c` routines originally established the exact pool
defaults/allocation cleanup,
float-to-public position/movement conversion, constant-motion vector access,
public position lookup, facing access, animation-authored charge/acceleration
and axis swapping, and the original scene-ready face-lock gate.
`anim_InitAnimations` and `anim_CheckFreeze` recover the 20-object animation
pool and the exact movement freeze-window decision. `anim_AddNextAnimSeq`
recovers the eight-node fixed motion queue, including normal/target template
selection, queue replacement, duplicate suppression, and per-motion
tween/speed/lock state. The extracted motion handoff from
`anim_ForceNextAnimSeq` preserves the original
`Motion.vel + Motion.Charge` and `Motion.ChargeAcc` inputs without inventing a
direct pad-to-velocity path.

The CAD boundary is now data-backed rather than inferred. Its first word is
the payload size; the payload carries sequence, encoded-data, and final
`Motion` table offsets plus part/sequence counts. All seven `unpack.c`
procedures are reviewed, covering the bit reservoir, direct/cached/tree
Huffman lookup, raw and compressed vectors, table binding, context
initialization, and sequence seek. A dependency-light zero-copy loader and
validated Huffman companion-table seam accumulate all 150,272 authored frames
across 7,603 sequences in all 93 installed CAD files. The corpus
also preserves non-word-aligned seek values (rounded down by the original)
and terminal decoder overlap into the following `Motion` records. Exact
`anim_CreateObject` binds those views. Exact 672-byte
`anim_ForceNextAnimSeq` owns forced queue activation from all six reviewed
`animctrl.c` wrappers and player reset, including recovery, rate and physics
setup, callbacks, sounds, tween selection, and immediate frame publication.
The assembly-checked local PDB symbol
`anim_GetAnimFrame` preserves
exclusive endpoints, double buffering, raw reset, first/later delta
accumulation, 12-bit joint-angle and 13-bit root-Y wrapping, and event-byte
publication. Exact `anim_SkipToStartFrame` restores pre-roll pose decoding
without publishing pre-roll events, and exact `anim_ProcessAnimations` owns
the live 20-slot player/enemy pass. All 22 `animutil.c` procedures cover
animation queries and control state.

The PC game runtime now executes all six recovered `CalcMovement` modes and the
complete exact `MovePlayer` entry. It preserves charge decay, current-motion
staging, facing rotation, model scale, airborne gravity, frame-rate scaling,
ordinary and large-character contact sweeps, world collision, fall and ledge
transitions, special-player ground snapping, moving-platform publication,
map-contact dispatch, and final flags. Exact `FVECTOR4`, `CollisionData`,
`_collide_info`, and
`_movement_packet` layouts now name the collision boundary. Exact original
helpers `CalcRelativePosFromWorld`, `CalcSolidRelativePos`,
`CalcWorldPosFromRelative`, and local `CalcWorldRelativePos` implement solid
rotation/scale, `Mnode.v3RotCenter` translation, relative facing, and flag
transitions. Exact `VectorNormalize`, `VectorNormalize2`,
`VectorNormalize3`, `CalcNewBox`, `buildfrustrum`, and `buildplane` now
provide the collision-frustum math, including `gpWorld->start`, the
460-pixel focal constant, and the instruction-verified fifth
`(0, 0, -4096)` plane. The exact 240-byte `_collidevars` record and original
module-local `planecheck` preserve signed plane approach, penetration
clamping, best-distance selection, and type-`9` best-contact promotion.
Original module-local `sphereAndPoly` and `polycollidecheck` now preserve
polygon bounds rejection, initial-overlap handling, swept face/edge/point
contacts, double-precision square-root boundaries, and best-contact
priority. Synthetic gates exercise every reachable contact category plus
back-face and AABB rejection.
Original module-local `generalCollide` now resolves the `_solid` face-index
stream through exact `getPtr`, expands triangle/quad vertices, converts
fixed normals by 1/4096, and applies the original contact-priority and edge
publication rules. A synthetic solid-stream test reaches the kernel through
this real traversal boundary.
Exact `newclosestPoly` now carries that kernel through player clip planes,
the Uber box, the 20-object dynamic-solid pool, and the compressed
256-column world grid. Exact `jon_getlibpartfloat` and integer
`jon_getlibpart` decode thin-cube library parts, including base corners,
height scaling, packed extra vertices, the original `gaScratch` workspace,
and the returned polygon stream. Synthetic gates validate a compressed thin-map
hit with exact cube/entry/poly output pointers, back-face rejection, and a
dynamic-solid hit with the reference flag and `whichsolid` publication.
The exact walk-height path now continues through file-local
`intersec_WankCheck`, `jon_plumbline`, file-local
`intersec_GeneralCheck`, and all three `intersec_FindWalkHeight` entry
points. It preserves the original 32-bit edge arithmetic, fat/thin cube
selection, packed normal planes, dynamic-solid priority, map-info pointers,
and sentinel values. Synthetic map gates exercise the integer and FVECTOR
paths and the compressed-library traversal.
All 15 procedures in `intersec.c` are now reviewed (7,822/7,822 bytes).
`LineAndPlane`, file-local `raycastpoly`/`raycheckgeneral`,
`RaycastCheck`/`RaycastCheckSV`, `MoveObject`, `MoveObjectNormal`, and
`HitSomething` preserve nearest map/dynamic-solid impact selection, exact
cube/entry/poly output roles, packed-normal decoding, fixed-direction motion,
and the `BlowUp` map-event handoff. Synthetic map and solid gates validate
both ray APIs, both movement wrappers, and hit normals. The complete
2,178-byte `bullet_CallBack` now uses that path for ordinary world impacts and
reflected bounces and also preserves ballistic/homing steering, both sprite
update modes, lifetime/effect/audio behavior, masked character hits, authored
level exceptions, plasma arcs, and the type-6 explosion tail. All eight
`bullet.c` procedures are reviewed.
The instruction-reviewed `WorldBlocking` state core now consumes the recovered
cube-solver result through the exact PDB signature and preserves early-outs,
blocked/unblocked flags, landing and map-info transitions, air-ground state,
and ground snapping. It reaches `CheckCubeBlocking` directly; required `jpb_`
hooks keep the still-pending landing-animation and splash/audio dependencies
visible.
The portable core of exact local `CheckCubeBlocking` now runs the complete
four-contact iteration over `newclosestPoly`, world/dynamic slide response,
air-stick/nonmoving rules, ledge capture and relative-solid conversion,
collision timers, height fallback, final snap, and position publication.
No-contact, no-floor fallback, and real dynamic-solid streams are gated.
Height queries now call exact `intersec_FindWalkHeightFV` directly. Exact
`Sprite`, `SCB`, `SControl`, `sprite_gHideSCB`, and
`sprite_gHideSprite` recover the shadow path without a hook. Exact `HitsHit`,
`LaunchMapAnimEffects`, cube-mesh switching, and `StopNearestFan` now recover
the former broad contact callback path. Exact `CControl`, `RingData`, `Ring`,
and `optionstruct` layouts plus the recovered sprite-effect allocator and
callbacks remove the last typed effect backend from this path.
Exact `hurtplayer` owns prolonged damage.
All five `extracharacters.c` routines now use the exact PDB `model_id` enum,
24-byte `ExtraCharacter` record, and paired-executable 14-entry table.
`CheckCubeBlocking` therefore reaches exact
`extracharacter_CanLedgeClimb` directly instead of an inferred hook.
A processed standee contributes its exact moving-solid world delta and
facing, while recursive scheduling evaluates an unprocessed platform first.
The live collision-backed PC graph links actor, scene, model,
physics, animation, and player records and follows the resulting physics
position. It derives and relocates the original matching W3D archive,
providing authentic `WorldData`, Jonny collision grids, and map pointers to
`MovePlayer`. The real-asset active-controller smoke uses FED because exact
`brain_ControlPlayer` intentionally exits when `GameStruct.CurrentLevel` is
8, the STREETS special case. With `obi_wan.cad`, the production controller
reproduces the executable's `atan2f(g_p1X, g_p1Y)` camera-relative angle,
selects authored `Motion[2]`, and feeds its velocity into the exact scheduler.
The smoke gate requires the actor's world position to change. Stationary
input executes
the recovered `brain_ControlPlayer` idle selection: normal `Motion[0]`,
low-energy `Motion[19]`, or lock-on `Motion[20]`. The same bounded seam
preserves `runCounter`, constant-vector and flag cleanup, and the delayed
`Motion[2]` to `Motion[25]` run-stop exit.
Omnidirectional input now preserves the reference wrapped-facing threshold
between `Motion[26]` and reverse `Motion[8]`, along with the exact per-motion
speed, freeze-window, tween, facing, and equal-lock updates.
The recovered special direction handoff accepts only original motions 2 and
60, applies exact `animutl_SetCurrentLock(..., 15)`, and returns through the
reviewed idle/run-stop selection.

All fifteen PDB-named procedures now establish the reviewed
`brain.c` boundary (7,081 of 7,081 procedure bytes). The exact set covers
motion-event sound dispatch, target discovery and lock-on validation,
ring creation/update/teardown, knockdown recovery, energy-driven player/NPC
death scheduling, afterlife and loop-sound shutdown, hang/skid/takeoff/throw
animation callbacks, jump/fall parameter choice, fixed-12 air-vector
construction, timer and player-flag transitions, and the constant-motion
axis swap. Supporting exact procedures include
`brainutl_gGetNearestTarget`, `physics_gGetRange`,
`sprite_AddSpriteEffectAtNode`, and `sprite_gUnHideSprite`. The alternate
jump launch and Motion 15/21 attack tail are now reviewed source blocks in
the complete 3,954-byte `brain_ControlPlayer` parent. Its final direct
cross-module dependency, the complete 4,255-byte
`braindmg_DamageControl`, is now integrated with the other eleven PDB-named
`braindmg.c` procedures. Exact `enemy_KillKill`,
`combo_CheckHeldPad`, `combo_ReadCombo`, `combo_CheckCombo`,
`sprite_GetCommentsSprite`, and `force_gActivate` are already integrated.
The complete PDB-named `enemy_ParseOpcodes` valid-data call surface now owns
every matched dispatch branch, including `0x411` and the original unknown
debug-reset continuation. Its `jpb_` diagnostic companion reports unknown
data and bounds malformed cycles for portable tests.
The normal active-enemy owner now also preserves the matched frame timer,
timed AI flags, tank countdowns, score/achievement and next-level events,
level-13 player-count bit, and both shipped per-enemy level overrides. Exact
`enemy_ResetEnemies` and `enemy_SetTeleport` own active-list teardown,
placement/global-bit reset, and teleport publication without copying VECTOR
padding that the original instructions leave untouched. The PDB-named
`enemy_HandleEnemies` call surface now includes its exact debug-level gate,
and exact `enemy_Radar` preserves authored scaling, camera-relative markers,
owner colors, and draw order. A narrow PC `_DrawTexture` realization queues
those solid rectangles until after world/model rendering.
The complete 48-entry `enemy.c` procedure inventory now has source bodies.
The final live owner, `enemy_CheckTeleport`, consumes deferred teleport state,
moves nearby non-source enemies and the correct active player set, republishes
scene transforms, and retains the level-8/9 exceptions and camera-mode rules.
The PC frame calls it at end of frame, and a real-asset smoke verifies the
deferred offset. Nearest-waypoint selection, point accounting through the
exact 80x3 actor/model table, and console-driven flag/placement mutation are
also restored. The teleport refresh now calls `camera_SetCameraPos` directly.
All 23 PDB-listed `camera.c` procedures have source bodies: the recovered
camera-position/frame owners select collision-derived authored dollies,
reject invalid transitions through the exact frustum, handle the one-player,
two-player, focused, Streets/JarJar, uber, and shake modes, and publish the
resulting gameplay camera. The PC frame uses this owner in place of its former
parallel authored-camera builder.
The portable loader seam now persists for the runtime lifetime and owns
independent scene/model/physics/animation/player records for each active
supported placement in original pool slots 2..19. Exact matched-executable
tables connect all 12 FED actor-table classes: battle-droid, pilot, security,
protocol-droid, Destroyer, Loader Droid, Droid Starfighter, beacon, door,
piston, and both lift records. BMD/CAD,
textures, and per-level WAI storage remain immutable within each class;
per-object animation slots are reset and reused independently. The authored
FED gate proves an 18-actor peak, 30 authored spawns, seven classes active
together, and all 11 placement-backed classes activated and rendered through
exact `_addEnemy` during nine frames. `twilek2` is the twelfth loaded
actor/assets class but has no authored FED enemy placement. Collision-enabled
machinery models also cover the validated immutable-BMD stream resolver at
the bounded original-loader relocation seam.
The sprite path now includes exact floating-score creation and lifetime
callbacks; `Draw3dText` retains its PDB API over a dependency-free, still
partial PC renderer hook. Exact `force_PlaySeq` now preserves the signed
motion-map decode, force costs, callback assignments, per-power state, and
authored combo-chain scheduling. The parent's sole portability substitution is an
optional input provider for the original SDL-only five-key cheat chord.
Six formerly null exact motion-table slots now publish
`force_FlameCallBack`, `jedi_FireWeapon`, `ai_Tank`, `ai_Stap`, `jedi_Main`,
and `tusken_stab`. Ranged player motions therefore enter the production
projectile allocator, sprite owner, and sound path using their authored
single/paired muzzle nodes and power-up override; flamethrower and Tusken
frame-gated collision behavior are restored without a host substitute.
`scene_gGetNewSceneObject`, complete `scene_gInitScenes`,
`scene_gCreateObject`, `obj_gSetChildObject`, and `obj_gSetObjectFlag` now
create and mutate the same
scene-to-physics-to-player ownership topology in the PC game runtime. The
three developer-console registrations preserve their exact long names, short
names, callback pointers, case-insensitive duplicate handling, and 255-entry
registry bound.

The live player side of that topology now enters the complete PDB-named
`jedi_InitPlayer` directly. Its exact character switch selects one of eight
initialized collision tables, applies model/player scale, publishes the
48-record `combos1` or `combos2` global, writes the movement profile, and
installs exact `jedi_Main`. `jedi_HasProgression` and `jedi_IsMelee` retain the
original character classifications, while the per-frame callback preserves
the Maul/Force-node cleanup exceptions. External authored CMB data still
replaces the built-in combo pointer through the existing load path.

Exact `scene_gSetSceneModelKeyFrame` publishes every decoded authored frame to
the scene. The renderer-derived `jpb_ModelApplyAnimFrame` boundary recursively
copies only authored x/y/z rotations into recovered `Mnode` trees, preserving
padding plus the original static/virtual and absolute-rotation behavior. Its
scene-aware form also reproduces `render_RenderNode`'s exact event publication:
any hot node sets scene attack flag `0x10` for the exact
`player_DoCollisions` owner to consume and clear. It is already connected
conditionally in the PC game runtime; the actor model-node archive loader now
populates `modelObject.pRootNode` from real BMD data.

The exact 144-byte, pointer-free PDB `geomData` layout now defines the shipped
BMD boundary. Exact PDB `modelSpace` and `TextureTracker` layouts also restore
the 20-model registry, 32-node per-model storage, and texture-packer state.
The live PC player and enemy paths call `model_gInitModelRoot` and
`model_MakeNode` to validate the record tree, build contiguous `Mnode` child
arrays, register collision nodes, relocate all five geometry streams through
exact `addPtr`, substitute saber textures with `transabr.bmp`, and connect
real `obi_wan.bmd` nodes to the live authored pose stream. The bounded
renderer and physics paths consume the resulting `getPtr` indices. The exact
`TT_TEXTYPE` values and 800-entry `g_material` owner are restored, and the
live PC texture hook lets `model_MakeNode` replace each texture-name union
with the retail `_Material*` while dependency-free probes retain readable
names. All 151
installed BMDs are gated, including
the observed payload-end sentinel, `worm.bmd` eight-child spill, and an
explicit expected-malformed gate for the shipped `skin2.bmd` truncation.

The matched PC `_RenderNode` geometry path is bounded separately from
structural archive loading. It consumes three signed-10-bit packed vertices
per `geomData.numVerts` unit after `numShareVerts`, signed-16-bit
triangle/quad face records with a `0x7fff` triangle sentinel, four float UV
pairs per face, and per-corner normal/color streams. The alternate matched
`gl_RenderNode` path supplies 4-byte packed-8 face records and a `0xff`
triangle sentinel. The installed-data audit now validates 145 archives
completely through one of those exact paths. Four `gate*.bmd` variants have
packed indices but only 8 UV bytes per face where `gl_RenderNode` advances
32; `weasel.bmd` contains one out-of-range node, and `skin2.bmd` remains the
known truncation. Each is an explicit gate rather than a silently accepted
renderer assumption.

The dependency-free BMD renderer reproduces the inner `render_RenderNode`
hierarchy matrix order, translation inheritance, authored pose rotations,
optional per-node scaling, and the exact 3,072-`FVECTOR` scratch bound visible
in `_RenderPackets`. The portable pose boundary now also preserves the exact
authored node-event mask and the hot-node promotion to scene attack flag
`0x10`; the exact `player_DoCollisions` pass consumes and clears that flag.
PDB types `pairUV` (8 bytes), `faceUV` (32 bytes),
`_Material` (296 bytes in the matched x64 build), and
`TEXTURE_SAMPLE_TYPE`, together with exact `_StartPoly`, `_SetVert`, and
`_NoScaleEndPoly`, define the material boundary. The portable filled path
perspective-correctly interpolates per-corner UV and color, performs bilinear
clamped sampling and model depth testing, alpha blends, and follows the
shipped `PixelShader.hlsl` texture-times-color and black-discard behavior.
The texture callback also retains `_Material.flags`, `samplerType`, and
`colorOverride`. Reviewed `SetTextureColorOverride` supplies the exact
level-specific exceptions. Flag zero performs negative projected-winding
rejection, flag one is two-sided, and flag two is two-sided at forced depth
`0.0001f`; the renderer also applies the exact grayscale and `-1000`
dark-red color overrides.

A caller-owned, dependency-free TGA decoder covers all encoding classes
observed in the installed corpus: uncompressed color-mapped, true-color,
grayscale, and RLE true-color images with 8-, 16-, 24-, or 32-bit pixels.
The PC game runtime resolves each BMD `.bmp` texture name into the sibling
`tga/*.tga` directory through `_LoadTexture` during model construction; JPX
materials remain lazy in the level's adjacent TGA file. Pixel-resource
ownership stays behind a narrow platform callback. World UVs use
repeating bilinear sampling while model UVs retain their clamped path. The
STREETS smoke renders
the real 21-node animated and textured Obi-Wan hierarchy and requires nonzero
model triangles and filled pixels. The exact `model_gInitModelRoot` initial
fixed scale of `0x2000` is preserved. Filled JPX strips now share the
perspective-correct material rasterizer and reusable depth surface with BMD
actors. The complete 26-level `levelTextures` initializer and PDB-named
`isTextureTransparent` now select the transparent queue. A labeled 8.3-name
bridge maps the JPX mirror to those exact FBX-era IDs; unidentified mirrors
retain `MATHEAD.listtype` as a fallback. Exact `isGlassTexture` selects the
third no-depth-write pass. The queues reproduce the shipped transparency
shader cutoff, color, vertex-alpha, and blending rules. This matches
`_InitFBXLevelData`'s three vectors and the exact `DrawLevel`,
`DrawLevelTransparent`, and `DrawLevelTransparentGlass` consumers. Exact
`cullmesh` is recovered as `int[32]`. All three draw procedures derive Streets
slots from the FBX names (`Broken=NN*2-1`, `Solid=NN*2`, base mesh zero).
Outside Streets, only the opaque pass consults vector position when it has at
most 31 entries; larger opaque vectors and all transparent/glass vectors draw
unconditionally. Exact `cube_InitVisibility` supplies the FED, Tatooine, and
Streets initial states. An optional ufbx-0.6.1 evidence probe maps every one of
the shipped Streets mirror's 728 nondegenerate dynamic-wall triangles to the
named source surfaces. Its sparse output is fingerprint-guarded and now drives
the JPX fallback directly. The PC host now imports the named FBX surfaces, but
ufbx remains absent from the game runtime and portable renderer. All 18
PDB-emitted `boss.c` procedures are now reviewed
(11,000/11,000 bytes). Exact callback slots 34 and 46 now reach `ai_Kadu` and
`ai_TurretDroid`, including the mounted race controller and the turret's full
projectile, zap, shield, aiming, and detachable-arm state machines.
All six `vehicle.c` procedures are reviewed (6,860/6,860 bytes). PDB-named
`ai_Stap` and `ai_Tank` retain the authored multi-driver controls, vehicle
movement/lean, articulated gun nodes, targeting, projectile cooldowns,
dismount state, and looped sound ownership. The shared exact 428-byte
`FindBestMachineGunTarget` dependency is restored in `physics.c`.
The adjacent eight-function, 1,730-byte ground/contact cluster is now
recovered under its PDB names. It includes the complete range-cache
initializer, polygon accessor, ground classifier, two-player target
transform, physics component creator, typed nearest-target scan, inert map
callback, and 462-byte `player_DoCollisions` owner. The PC field calls the
exact contact pass directly instead of maintaining its former host-side
successful-contact stores.
The adjacent exact `player_gConnectMotionData` owner now replaces manual CAD
motion pointer/count assignment for the player and every spawned enemy. It
preserves relative-table resolution, both legacy saber-hit name rewrites, and
the flag-derived callback indices. The complete executable-backed 114-entry
PDB `sModelNames` table supports the four exact `loader_Get*Name` leaves.
Exact 323-byte `player_gCreateObject` now owns requested-ID/first-free player
allocation, `WorldData.player0/player1` publication, scene component link 4,
animation-motion/keyframe publication, player/type IDs, gameplay and Pad
resets, and the typed character initializer. The live PC player and all real
enemy actors enter through this owner after model, animation, and physics
attachment, matching `loader_CreateCharacter` ordering. The deliberately
inactive single-player safety slot remains a raw pool record because it has
no model or animation and is not a constructed character.
Exact `player_gRefreshPlayers` restores the 20-slot refresh plus the authored
level-eight physics/collision reset tail. Exact `player_HandleSabre` now owns
the original 20-slot post-scene eligibility pass. Its PDB-named
`jedi_HandleSabre` dependency restores model-specific node pairs, blade
colors, normal/long endpoints, double-blade handling, attack world sweeps,
feedback, and node timing. The dependency-free PC renderer projects and
additively composites the original `fx_screenGlow` calls; the real FED smoke
proves two zero-drop blade passes and 3,304 composited pixels in the current
three-frame Debug sample. The textured
blur and powered cylinder realization remains bounded and is not claimed as
immediate-mode parity. The scheduler's interleaved HUD/debug owners remain a
separate presentation target.

The ordinary mode-1 overlay now crosses a recovered text contract rather than
the temporary compact bitmap in installed-game runs. PDB and instruction
evidence for `getFontFile`, `LoadFont`, and `SDLTextWriteScale` fixes the
NotoSans language/style map, `scale * ScreenHeight/480 * 24` truncated point
size, measured alignment, tint table, and alpha. A source-vendored,
platform-neutral TrueType rasterizer consumes the shipped `res/font` assets
without an SDL/SDL_ttf runtime dependency; missing assets alone use the old
diagnostic glyph fallback. The dedicated real-font gate renders the exact
87-point bold score configuration at 960x540.

The same boundary now owns the recovered `allTextEverything` localization
aggregate rather than test-authored message fragments. Source retains all
seven retail language blocks and all display slots under the original PDB
name. Exact `generateAllText`, `UpdateCurrentlyLoadedFont`, and
`MarkFontAtlasForRefresh` leaves are active during PC runtime startup; only
UTF-8-to-native-`wchar_t` widening is substituted for portability. A
whole-corpus hash verifies 68,317 recovered bytes.

The title flow now also has its original game-owned state foundation. Exact
PDB type `MENUVARS`, `menu_mainInitMenu`, title entry, the eight-slot push/pop
stack, player selection, objective/game-over transitions, and the small flow
leaves are source-visible under their matched names. Ten initialized
`mainMdef` title streams retain their exact command data; platform
input/texture/bucket calls remain narrow hooks. The live PC initializer uses
the recovered player-count/model owners. Exact `MMVDEF`, `MDEF_MOD`,
`mmsizes`, `mmNextCode`, `mmDrawItem`, `mmDrawsub`, `mmDraw`, and the ordinary
selection/activation portion of `menu_mainMenu` now realize the shipped title
carousel. Sixteen additional PDB-named arrays bring the recovered definitions
to 26 streams and 3,896 exact bytes, covering player count, difficulty, New
Game confirmation, Options, Quit, Controller, Language, Video, and Audio. The
reviewed portion of `menu_handleMenuTriggers` follows the executable's
compressed jump table for ordinary pushes, New Game/VS setup, player-count and
difficulty routing, character confirmation, training, sound/music controls,
combo/secret toggles, checkpoint cheat, and direct gameplay transitions.
Exact `menu_cameraChange`, `menu_menuMusic`, and `menu_sound` leaves are live.
The PC `--title` path decodes the
installed splash at the Win32 edge and composites the recovered definitions
through the portable TrueType framebuffer boundary with the distinct
`scaleAdjustmentMM` owner. Real-asset gates verify the 13-draw initial menu,
five-draw New Game confirmation, nine-draw Options state, the variable-backed
Audio submenu with a real Music toggle, and Video with a real Window Mode
toggle. The exact 74-entry `modVars`
table and its typed get/set/increment/decrement/draw owners are now live. The
remaining textured panel commands, save-card/level-start side effects,
unresolved modifier behavior, trigger cases, and complete frontend state loop
are still open.

Debug and optimized Release each pass all 507 registered tests, including 458
real-asset/gameplay gates (151 BMD model, 93 CAD layout, 93 sequence-decoding,
52 power-up-stream, the shipped-font gate, and the gameplay set), plus the
original-splash title gates, authored-motion, debug-radar, and multi-enemy FED
controller smokes and the authored `Motion[15]` FED attack smoke. The strengthened enemy
gate loads all 12 exact table-mapped FED classes and proves all 11 placement-backed
classes across an 18-actor peak and 30 authored spawns. Model 47 includes its
exact six-entry profile, callback slot, reviewed Starfighter control path,
and sprite-backed twin-shot emission through the exact projectile allocator
and shooter. Exact projectile-node impacts, saber/Force ricochets, the masked
nearby-player callback scan, hit-state publication, and radial explosion hits
are now active. Exact map/dynamic-solid motion, ordinary world impact removal,
and reflected bounces are also active through the complete `intersec.c`
module. The callback remains bounded at its explosive/termination and special
authored effect branches. This remains on the PC-fidelity path; console work
is deferred until the complete PC result is approved. The exact
`ProcessPhysicsObjects` scheduler is now recovered, tested, and used directly
by the real PC loop. Exact `twatcameramatrix` supplies its collision
and clipping frusta; the smoke gate proves scheduler-driven,
collision-aware world movement. The normal one-player field also loads the
exact 256-entry `.cam` image, selects its dolly from the
`intersec_FindWalkHeight` polygon result, and applies the recovered
`camera_SetCameraPos`/`camera_StuffCamera` selection, visibility, offset, and
slack paths. The loader retains exact initial view mode `0x901`, including the
`camera_CameraSlide` bit; the long FED combat gate now requires both that bit
and an on-screen player target after sustained movement. Its world and actors
share depth without the former orbit-camera clear. The portable PC frame
boundary also retains the centered FED opening dolly
after its director override clears while the player stays in collision-camera
region 3, then returns immediately to the recovered collision camera on exit.
Exact `camera.c`, AI, and `.cam` ownership remain untouched. The next
integration steps are the remaining menu, multiplayer,
special-camera, mod-integration, and other still-bounded presentation owners.

The scheduler's countdown now has its authentic producer as well as its
consumer. `CheckCubeBlocking` retains the reference STAP owner, collision,
normal, timeout, player-count, scene-state, effect, audio, and reset gates
before starting `streetsending` at `0x13800`. Exact
`brainutl_ElapsedTime`, `combo_ResetComboEngine`, and `player_ResetJedi`
recover the human-readable named leaves. The complete 1,574-byte
`physics_ResetJedi` retains the common motion/collision/ground reset and its
unrelated Level 10 player-count/vehicle-ID `0x4C` userdata exception, so this
path no longer depends on a STREETS-only helper.

The scheduler's exact `UpdateSceneObject` dependency is now complete. It
preserves target-facing, face-vector selection, transform publication, and
the 512-unit valid-ground update. File-local `checkdriving` is also recovered
and mirrors the driven physics record back into player slots zero and one.
Exact file-local `BuildSolids` and `BuildNodeVertexList` are now complete.
They use exact `ApplyMatrixMany10Bit` to realize packed BMD normals and
vertices, preserve nearby-object masks and enemy/model eligibility, and
retain the original single-allocation `coords`/`normals` ownership. Exact
`cliptofrustrum`, `PushMatrix`, and `PopMatrix` complete the dependency chain
for the 953-byte `ProcessPhysicsObjects` entry. Its complete-graph test covers
the original pass order, pause/game-flag movement gate, scene and
`WorldData` publication, balanced matrix stack, and terminal Streets state
transition. The PC game runtime can now replace its explicit per-actor calls with
this frame entry. That replacement is now complete: the real collision path
enters the scheduler, while only the explicit no-collision diagnostic mode
retains the smaller validating fallback.
PDB-local `anim_CreateTweenFrame` now restores authored motion blending with
the executable's exact 17-entry reciprocal table, 16-frame clamp, wrapped
fixed-point deltas, root/joint publication, and countdown. The live player
and enemy paths enter the reviewed `anim_GoNextAnimFrame` pose owner rather
than duplicating frame accumulation in the PC harness; the FED regression
records two tween publications in its first eight frames. Exact local
`anim_SoundStart` and `anim_HandleSound` now schedule immediate, delayed, and
looped motion sounds through the portable sound boundary, including authored
tank/taxi vetoes. The WinMM adapter provides audible SFX and streamed music
for interactive PC runs.

`WorldData` uses native pointers: its x64 offsets are asserted against PDB
type `0x1251`, while a 32-bit Xbox build compacts the runtime structure.
Pointer-bearing library archive headers are explicitly marked for the later
PC-to-Xbox asset conversion rather than silently copying x64 pointers.

Timer callback wiring is intentionally outside `timer.c`: the recovered module
owns state transitions, while the PC platform layer provides the actual
vertical-blank event. Any future target layer remains separately gated.

## Dependency policy

Portable reconstruction modules may use the C/C++ runtimes and standard math
library. File, input, rendering, audio, timing, and window services must enter
through small game-owned interfaces. Desktop APIs and framework types must not
leak into recovered gameplay state or serialized structures. This keeps the PC
build simple now and minimizes the adapter surface when nxdk work resumes.

## Evidence added in this milestone

- `inventory/globals.json` inventories linked PDB data symbols so decompiler
  addresses can be replaced with names and types.
- `inventory/type_layouts.json` records reviewed type layouts and their TPI
  indices, including the math/vector and camera/scene sets plus archive, world,
  AI, animation, placement, and library structures.
- `tools/foundation_inventory.py` makes the selected dependency order and its
  reviewed/total procedure and byte counts reproducible.
