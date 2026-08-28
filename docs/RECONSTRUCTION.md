# Reconstruction method

## Target

The target is a complete, behaviorally faithful, portable PC reconstruction
of the game, followed by a complete original-Xbox platform implementation
using nxdk.
The Windows D3D12, Steam/GOG, FBX, WinRT, and similar modern platform layers
are evidence about behavior; they are not the architecture to reproduce on
Xbox.

## Evidence classes

Every recovered implementation should retain one of these labels:

- **direct** — names, types, layouts, addresses, source paths, or behavior
  directly observed in the matching PDB, executable, or game data.
- **decompiled** — control flow emitted by Ghidra and reviewed against the
  matching machine code.
- **inferred** — behavior reconstructed from callers, callees, data flow,
  formats, or observable reference-game behavior.
- **substituted** — a deliberately portable implementation with equivalent
  externally visible behavior.
- **unknown** — unresolved evidence or a placeholder that must not silently
  become an assumption.

Generated module shells are not reconstructed implementations. They are an
index that gives every emitted routine a stable home and preserves the
evidence needed to recover it.

## Naming and readability

- Preserve original PDB function, type, member, global, source-module, and
  meaningful parameter/local names whenever they survive.
- Keep reviewed implementations organized by their original source modules.
- Descriptive parameter/local names may replace terse names such as `ct`,
  `n`, or `s`; record that choice and retain the exact PDB spelling in the
  generated inventory.
- Give extracted portable boundaries descriptive `jpb_` names and state that
  they are additions rather than original symbols.
- Never promote a Ghidra placeholder such as `FUN_...`, `DAT_...`, or
  `param_...` into reviewed source when a PDB name or a defensible descriptive
  name is available.
- Mark genuinely unresolved concepts as unknown instead of hiding them behind
  confident-looking names.

## World geometry evidence

The matched 2024 executable's live PC level path is FBX. Its embedded
`ufbx_source_version` is 6001, exactly matching ufbx 0.6.1:
`loader_LevelLoad` calls `ufbx_load_file`, `_InitFBXLevelData` publishes the
meshes, `cube_NewWorldRender` submits the level transform, and
`CD3DApplication::DrawLevel` draws them. The older `InitJPX`/`WorldmeshData`
path has no live visual consumer in this executable. The PC reconstruction
therefore imports the shipped FBX through a narrow exact-ufbx 0.6.1 adapter
and immediately flattens it to the portable `JPBSoftwareLevelMesh`. JPX
remains the collision world. A platform that uses JPX as visual geometry must
label that choice as a portable substitution.

The placement itself is direct/decompiled evidence. PDB globals
`sLevelNames`, `level_offset`, `level_scale`, and `startPos` retain their exact
names and layouts. `cube_NewWorldRender` plus the shipped
`LevelVertexShader.hlsl` establish the authoring-to-game mapping:

```text
game.x = -fbx.x * level_scale[level][0] + level_offset[level][0] * 256
game.y =  fbx.z * level_scale[level][2] - level_offset[level][2] * 256
game.z =  fbx.y * level_scale[level][1] - level_offset[level][1] * 512
```

This is cross-validated against `startPos` and all available shipped FBX
levels. Streets maps the exact FBX vertex `{-120,-97,13}` to player zero's
fixed-unit start `{30976,3328,-24576}`.

## Model material evidence

The matched `_RenderNode` at RVA `0x129030` passes each triangle or quad to
exact `_StartPoly(int, _Material*)`, then calls exact
`_SetVert(int, float, float, float, unsigned long, float, float)` once per
corner before exact `_NoScaleEndPoly()`. PDB types `pairUV` (8 bytes) and
`faceUV` (four pairs, 32 bytes) define the UV stream. The matched x64
`_Material` is 296 bytes and retains its exact fields: `texture`, `type`,
`iw`/`ih`/`ix`/`iy`, `filename`, `m_isTransparent`, `flags`, `samplerType`,
and `colorOverride`.

The live material policy is now bounded by named producers and the exact
renderer consumer. `SetTextureColorOverride` owns the level-specific
`Loadbody.tga`, `boulder.tga`, `ful_body.tga`, `bus.tga`, and `qui_hair.tga`
cases, with `ClearCachedTextureIndices`, `IsBusTextureForCorus2`, and
`IsCoffinTextureForPalace` retaining their PDB names and exact cache globals.
`_NoScaleEndPoly` consumes all observed flag values: zero rejects a negative
projected shoelace winding, one disables that rejection, and two also forces
submitted depth to the executable float `0.0001f`. `_RenderNode` replaces RGB
with the low byte of a nonnegative `colorOverride`; the `-1000` sentinel maps
near-black red inputs to `0x12` on all three channels.

The renderer consumes one `faceUV` per face and packed `CVECTOR` entries per
actual corner. `_RenderNode` forms opaque `0xffRRGGBB` vertex colors, except
for its directly observed `colorOverride` cases. The shipped
`PixelShader.hlsl` samples the selected texture, multiplies RGB by the
interpolated vertex color only when every input color component is nonzero,
and discards black texels. These records and shader rules are direct evidence.

`jpb_SoftwareRenderBmdMaterialized`,
`jpb_SoftwareRenderJpxMaterialized`, their callback-based texture resolver,
and the TGA decoder are portable additions. They keep world/model traversal
independent of D3D and Win32. Perspective-correct interpolation, repeating
world UVs, clamped model UVs, bilinear sampling, reusable depth testing, and
alpha blending are explicit portable realizations. Convex JPX and BMD faces
are clipped against camera Z `1.0` before division, with camera-linear UV and
color interpolation at each introduced edge; wholly-behind faces are rejected
and retained polygons are triangulated for the software rasterizer. Exact
`render_RenderModel` also applies `_animFrame.v3RootTranslation` through the
scaled world rotation before traversing the BMD root. The portable API now
carries that decoded key frame explicitly instead of substituting the static
BMD root-node translation. The complete recovered
`levelTextures[26]` initializer and exact PDB `isTextureTransparent` now
select the portable world's transparent queue. A separately labeled matcher
maps collision-resolved JPX 8.3 IDs back to those exact FBX-era names;
unidentified legacy mirrors retain `MATHEAD.listtype` only as a fallback.
The exact `glassTextures` initializer and `isGlassTexture` select the
reference glass queue whose copied pipeline descriptor changes
`DepthWriteMask` to zero. Both transparent queues apply the shipped
`LevelTransparencyPixelShader.hlsl` all-zero discard, 0.1 sampled-alpha
cutoff, minimum vertex RGB, and full RGBA modulation. The portable BMD path now carries and
applies the recovered `_Material` policy. For the live FBX world,
`_InitFBXLevelData` calls exact PDB `isTextureTransparent` and
`isGlassTexture` to populate three mesh vectors consumed by exact
`DrawLevel`, `DrawLevelTransparent`, and `DrawLevelTransparentGlass`; the
last uses the no-depth-write glass pipeline. Those material boundaries now
execute over the flattened FBX mesh without exposing ufbx to the renderer or
gameplay core. Exact `cullmesh` is PDB type
`int[32]`; its initializer and the policies in all three draw procedures are
recovered and tested. Streets does not index it by vector position: each
procedure calls public CRT `atoi` on
`mesh.name + 4`, maps 12-character `WallNN_Solid` names to `NN*2`, maps
13-character `WallNN_Broken` names to `NN*2-1`, and leaves `streets_A0` at
slot zero. Outside Streets, only the opaque procedure uses vector position;
it draws unconditionally when its vector has more than 31 entries, while the
transparent and glass procedures are always unconditional. Exact
`cube_InitVisibility` first fills all 32 entries, then applies the FED,
Tatooine, or Streets variant; Streets preserves even solid slots and clears
odd broken slots 1 through 23.

The exact-ufbx evidence probe maps all 728 nondegenerate dynamic triangles in
the shipped Streets JPX mirror back to those source surfaces.
The resulting sparse range table is enabled only when the entire JPX file
matches its FNV-1a fingerprint, so modified assets cannot accidentally inherit
shipped-file offsets. That table remains available to the JPX fallback. The
PC renderer instead executes live cull state directly over the FBX mesh names
imported by its isolated adapter. Thirty-nine JPX triangles that cross
source-face diagonals use the unique owning wall bounds; all other fallback
ownership comes from source-surface correspondence.

The level rasterizer itself is deliberately two-sided. In
`CD3DApplication::CreatePipelineStateObject` at RVA `0x315E0`, the recovered
rasterizer descriptor sets `CullMode` to `D3D12_CULL_MODE_NONE`; this is
independent of the mesh-visibility `cullmesh` array above. The portable level
path therefore accepts both projected windings, with a reversed-strip
regression fixture covering that behavior.

The reconstruction keeps two projection contracts distinct. Exact legacy
`PerspectiveTransformFV` remains the 640x480, focal-460, center-(320,240),
depth-/10240 operation used by reviewed game procedures. The live PC software
scene instead uses `jpb_ProjectPcToViewport`: the matched D3D application
creates `XMMatrixPerspectiveFovLH` from executable constant `0.9250245094`
radians (53 degrees), the current render width/height aspect ratio, near
`1.0`, and far `10000.0`. The portable adapter reproduces its aspect-correct
screen scale while retaining the common `/10240` software depth convention so
recovered screen effects and materialized geometry share one depth surface.
Its camera-space companion accepts points on the near plane, allowing the
portable face clipper to reproduce the GPU's pre-divide primitive boundary
without changing exact legacy `PerspectiveTransformFV` semantics.

All 29 `vectors.c` procedures are reviewed against the PDB and executable
RVAs `0x103970..0x1047A4`. The three rotation wrappers now retain their exact
`PushMatrix`/`PopMatrix` ownership, including the saturated depth-15 mutation
and the low 32 bits of `gte_matrix_stack` left in `EAX` by retail
`PopMatrix`. `vec_gDefinePlane` also calls the shipped inert `debug_printf`
with its exact normal/constant format and argument order. Focused tests cover
every procedure, fixed-width wrap/truncation, NaN range behavior, aliases,
matrix-stack restoration, planes, and projection.

Camera initialization is part of that projection contract. Exact
`scene_gInitRoot` calls `camera_SetViewType(0x901)`; bit `0x100` causes
`camera_Camera2ViewVector` to run `camera_CameraSlide` before publishing the
environment. The PC loader preserves the complete mode rather than narrowing
it to absolute focus (`0x800`). A sustained authored-motion gate requires the
slide bit, an authored camera update for every rendered frame, and a final
player target inside the framebuffer. This distinguishes a valid two-sided
level from a frozen camera looking across it at the wrong composition.

## Physics contact evidence

The matched PDB names the moving-solid coordinate boundary directly:
`CalcRelativePosFromWorld`, `CalcSolidRelativePos`,
`CalcWorldPosFromRelative`, and the local `CalcWorldRelativePos`.
Their solid origin is the exact `Mnode.v3RotCenter` member. The reviewed
implementations retain the original `InvertMatrix`/`fScaleMatrix` ordering,
relative-facing subtraction, and `0x1800` flag transition.

Exact PDB records `FVECTOR4`, `CollisionData`, `_collide_info`, and
`_movement_packet` are represented with their original field names and
asserted layouts. Exact `VectorNormalize`, `VectorNormalize2`, and
`VectorNormalize3` preserve their distinct double/float normalization
boundaries. `CalcNewBox` projects the first four collision planes around
`gpWorld->start`; `buildfrustrum` and `buildplane` preserve the original
460-pixel focal scale, percentage offsets, float-to-int truncation, and
instruction-verified fifth plane `(0, 0, -4096)`.

The exact 240-byte x64 `_collidevars` record names the sphere/polygon
solver's pointer arrays, edges, normals, bounds, distances, and masks. The
original local `planecheck` retains the instruction ordering for plane
approach, penetration clamping, and whole-record best-contact copying,
including the machine-code-only-visible promotion from type `1` to type `9`.
Its public `jpb_PhysicsPlaneCheck` facade is documented portability/test code;
it does not claim to be an original PDB procedure.

The original local `sphereAndPoly` consumes those exact records directly.
Its reviewed implementation retains polygon-bound calculation, normalized
edge planes, integer side masks, initial face/edge overlap categories,
swept face/edge/point contacts, and the original float/double boundaries
around square roots. The local `polycollidecheck` applies the executable's
bit-4 priority, truncated-distance comparison, upward-normal tie break, and
type-2 edge publication. Their `jpb_PhysicsSphereAndPoly` and
`jpb_PhysicsPolyCollideCheck` facades are test/integration seams only.

The original local `generalCollide` is also reviewed. It uses PDB-named
`_solid.geometry`, `coords`, and `normals`, exact `geomData.numFaces` and
`pIndex`, and the portable exact-API `getPtr` registry to expand triangle or
quad faces. Its normal conversion is the instruction-verified 1/4096 scale;
the unused optimized `from` and `radius` parameters remain in the exact PDB
signature instead of being silently removed. `jpb_PhysicsGeneralCollide`
only exposes this original local routine for testing and staged integration.

Exact `ApplyMatrixMany10Bit`, file-local `BuildNodeVertexList`, and
file-local `BuildSolids` now provide the original moving-solid realization
path. Packed signed ten-bit normals and vertices retain the executable's
shift, float-matrix, truncation, low-word translation, and fixed-point scale
ordering. `BuildNodeVertexList` retains the shipped `VECTOR*` local mismatch
over `_svector*` node rotation storage through documented unaligned 32-bit
loads, avoiding undefined C aliasing while matching the machine code.
`BuildSolids` preserves the 2..19 enemy scan, nearby-object bit mask,
`0xc000` enemy and model-bit eligibility, early allocation exits, and
single-allocation ownership in which `normals` aliases the tail of `coords`.
The descriptive `jpb_PhysicsBuildSolids` facade exposes these original
file-local procedures for tests and later scheduler integration.

The complete 49-procedure `fmath.c` module is now reviewed. The packed-vector
variants (`ApplyMatrixMany10BitFV`, normalized, long, and strided forms),
short/float batches, integer and legacy perspective entries, float and
padded-float rotate/translate batches, and CameraMatrix-backed
`TransformPoints` paths preserve their PDB names, exact element strides,
truncation points, near-plane constants, and packed screen-coordinate format.
These are dependency-free game math procedures rather than host-renderer
substitutions.

The complete 32-procedure `flex.c` companion module is also reviewed. Its
fixed-point scalar operations, cross/dot products, float and short segment
projection, packed-normal extraction, and short-vector arithmetic preserve
the executable's wrap, shift, truncation, output-alias, and untouched-padding
behavior. The three-byte retail `intersec_2dlines` body is recorded as an
empty stub and returns deterministic `NULL` in portable source.

The exact global `ProcessPhysicsObjects` procedure now owns the reconstructed
physics frame. It retains the executable's 20-record, scene-flag-aware pass
ordering across `BuildSolids`, processed-guard reset, optional AI collision
node refresh, local `CalcMovement`, `MovePlayer`, file-local `checkdriving`,
and `UpdateSceneObject`. Camera modes 0/1/2 retain the exact `0.5f` blended or
per-player ground-height conversion before `CalcNewBox`; the value was read
directly from matched RVA `0x45F90C`. Exact `cliptofrustrum` and the
`PushMatrix`/`PopMatrix` pair provide its remaining PDB-named dependencies
without a renderer framework. The final publication copies both integer
player positions into `WorldData`, balances the original matrix stack, and
preserves the Streets countdown's energy, game-state, component-flag,
physics-mask, and `maRange[1][4..5]` teardown. A descriptive `jpb_` seam
exposes only that otherwise file-local countdown for integration testing; it
is not claimed as an original symbol.

The live PC STREETS harness now calls `ProcessPhysicsObjects` when its real
Jonny collision archive is available. Its frusta use exact 515-byte PDB
`twatcameramatrix`, recovered from RVA `0x2CB30`, rather than feeding the
ordinary renderer view matrix into collision math. The procedure copies the
input, publishes the gameplay camera location with the reference coordinate
offsets and axis swap, transposes and swaps matrix rows, and performs the
original bitwise sign changes. A focused matrix test checks the full
transformation, and the real-asset smoke proves that scheduled movement
survives `WorldBlocking` and changes the actor's world position.

The remaining STREETS terminal trigger was located at
`CheckCubeBlocking` RVAs `0xDCFEC..0xDD1FC` rather than in `CalcMovement`.
It now preserves the reference's level and frame gates, collision-type and
dynamic-solid exclusions, exact `0.9f` positive-X normal shortcut, and the
short/long collision-time selection from `GameStruct.difficulty`. The direct
`game_initPerLevel` audit proved RVA `0x10DB3A4` is the PDB-backed
`gamestruct.difficulty` field rather than a separate anonymous byte. Active
player and scene-flag checks retain exact PDB
`stapbikeindex[2]`; on success the effect, cube runtime bit, timer reset,
scene disable, both Jedi resets, `explomed` audio, bike-index clear, and
`stapsound` shutdown occur in executable order before the scheduler consumes
the `0x13800` countdown.

Exact `brainutl_ElapsedTime`, `combo_ResetComboEngine`, and
`player_ResetJedi` provide the named reset leaves. The complete PDB-named
`physics_ResetJedi` now expresses all 1,574 executable bytes. In addition to
the common motion, collision, ground, scene-publication, and 20-slot userdata
reset, it preserves the Level 10 exception: in two-player mode, physics slots
whose matching `playerID` is `0x4C` retain `userdata[0]`. One-player mode
clears every slot, and the executable's otherwise-unused player-count cases
retain them.

The portable `MOVE_NORMAL` extraction now covers a standee
whose platform physics has already been processed, including platform world
displacement and facing. The original recursive scheduling of an unprocessed
standee remains an explicit recovery boundary. The portable state core of
exact local `CheckCubeBlocking` now preserves its iterative contacts, slide
projection, ledge state, timers, height fallback, and final publication;
exact PDB `Sprite`, `SCB`, and `SControl` layouts let it call
`sprite_gHideSprite` directly. Its broad contact callback has been removed:
exact `ExtraCharacterEnvironmentEffectExceptions`, `HitsHit`, `MTV`, and
`clear_eventlist` now preserve force thresholds, half-word map mutation,
circular undo logging, packed event coordinates, and matching-channel
propagation. Both the impact and post-height-query ground call sites invoke
that chain directly.

Exact `LaunchMapAnimEffects` consumes the executable-backed
`eventarray[30][15]` and `maphitsounds[16]`, launches the first eligible sound,
performs the level 1/5/8 mesh transitions through exact
`cube_HideMesh`/`cube_ShowMesh`, and calls exact `StopNearestFan` for the
level-10 fan event. Exact pointer-free `EffectData` and `EffectHeader` layouts
preserve the loaded effect format on PC and Xbox. Exact
`sprite_AddSpriteEffect`, `sprite_AddCallBack`, `sprite_MainCallBack`,
`sprite_FireRing`, and the sprite/SCB allocation leaves now preserve the
effect bank dispatch, recursive launcher, ring construction, fixed-point
motion, lifetime, brightness/scale controls, and intrusive-list ownership.
The temporary `jpb_` effect backend is removed. Exact PDB `CControl`,
`RingData`, `Ring`, and `optionstruct` layouts keep the implementation
human-readable and pointer-width-aware without adding a PC-only dependency.
Exact `hurtplayer` now handles prolonged damage, including
`player_AfterLife`, component state resets, and one-based tank-driver cleanup.
Exact `sound_Play` retains the reference bank fallback order over a portable
`jpb_` backend seam. Its two
height queries now call exact
`intersec_FindWalkHeightFV` directly. Exact `CharacterData`,
`JEDICOMBOMASK`, and `gamestruct` layouts now also support the reviewed
energy, item, power, and game-flag leaves used by exact
`player_AfterLife`. The initialized PDB arrays retain the human-readable
character combo names, while exact `game_initCombos`, `game_initEnergy`, and
`newGameGameInit` restore the executable's authored masks, 23 resource rows,
persistent progress clears, upgrade clears, and default two-player model
selection. The portable PC runtime calls this game-owned lifecycle function
instead of maintaining a parallel set of host constants. Exact PDB-named
`pwrup_LoadPoop`, `pwrup_Init`, `pwrup_LevelStart`, `pwrup_LevelEnd`,
`pwrup_JumpCheckPoint`, and the 4,144-byte `pwrup_CheckPowerUps` now own the
adjacent authored power-up/checkpoint lifecycle. The dispatcher preserves
both intrusive render lists, the exact post-frame list swap, range and pause
gates, W3D type-13 emitter timers, random-power resolution, every collection
award, achievement updates, checkpoint/continue state, artifact bits, pickup
effects/audio, and the two-player after-life revival route. Its PDB
`powerUpScales`, `pwrIcons`, `mRandomPower`, `powerUpNames`, and
`powerUpFiles` initialized data come from the matched executable. `powerPoop`
retains its native intrusive pointer while the pointer-free 12-byte `loadPoop`
disk record is decoded explicitly, keeping the code readable and pointer-width
portable. The PC runtime loads the selected level's `.pwr` through
`resource_getPath`, executes the dispatcher after player processing, and
proves 55 FED records enter the live frame. All 52 installed files are
validated independently. Exact `loadPowerupModels` now loads each nonempty
PDB table entry, relocates all five geometry streams, resolves its authored
material, and invokes exact `FixDrawPowerUp`. Exact `DrawPowerUp` builds the
scene/model matrix, decodes signed packed 10-bit vertices through
`RotTransPersMany10bit`, walks authored triangle/quad faces, and submits them
through the immediate-poly owner. The former glint and duplicate post-scene
model renderer are removed. The
instruction-reviewed
`WorldBlocking` state core now preserves
its exact early-outs, blocked/landing/map/air-ground transitions, and snap
rules. It calls exact local `CheckCubeBlocking` and exact `brainutl_Land`
directly. The original inline splash path retains exact file-local
`splasheffects[30]`, integer walk-height placement, `sprite_AddSpriteEffect`,
and `sound_PlayFV`; only the platform achievement and remaining original
sound-service owners remain
narrow dependency-light service boundaries. Exact `newclosestPoly` now
traverses player clip planes, the Uber box, dynamic solids, and compressed
fat/thin world cubes; exact `jon_getlibpartfloat` supplies its thin-library
vertex and polygon stream. Exact integer `jon_getlibpart` now supplies the
corresponding signed-16-bit `gaScratch` stream. The exact static-map
walk-height chain is now recovered through file-local
`intersec_WankCheck`, `jon_plumbline`, file-local
`intersec_GeneralCheck`, and all three `intersec_FindWalkHeight` entry
points, including dynamic-solid comparison and map-info publication.
The complete 15-procedure `intersec.c` module is reviewed at 7,822/7,822
bytes. Exact plane/polygon/mesh tests drive `RaycastCheck` and
`RaycastCheckSV` through compressed map and dynamic-solid geometry, while
`MoveObject`, `MoveObjectNormal`, `HitSomething`, and exact physics-owned
`BlowUp` retain fixed-direction motion, output pointer roles, packed normals,
and destructible-map effects. Local typed ray scratch is the only portable
storage substitution for the executable's shared temporary arena.
The exact PDB `model_id` enum, 24-byte `ExtraCharacter` record, 14-entry
table, and all five `extracharacters.c` procedures are reviewed. Direct
checks of executable RVAs `0x4B8B00`, `0x335810`, and
`0x4BB60..0x4BC95` prove every table field, scan bound, record stride,
capability offset, and unknown-character default. `CheckCubeBlocking` calls
the resulting exact `extracharacter_CanLedgeClimb` without a portable
dependency seam.

Exact `brainutl_Land` retains the long-fall energy penalty, player-zero
achievement completion, owner-type death counter, landing motion/chain
selection, flags, and ground-delay behavior. Exact `achievement_complete`
retains its ID 2..43 platinum scan while the Windows/Steam-specific platform
implementation is explicitly substituted by a portable service seam. Exact
`sound_PlayFV` performs the original `CVTTSS2SI`-compatible coordinate
conversion before exact `sound_Play` bank fallback. All 31 PDB-named
procedures in original `win32/sound.c` are now reconstructed, including the
retail bare returns, four declared loaded-bank slots, the latent accepted
slot `4`, pause/halt/fade/stop boundaries, and the 100-entry positional-loop
table. The matched executable's 43 exact 80-byte `tAudioSFX_Bank` descriptors
use the exact 504-byte `tBankHandle` and 16-byte `tSFXHandle` layouts, retain
696 unique pointer-array slots and intentional prefix aliases; those
descriptors expose 1,242 path references in total. Exact
`defaultOptionStruct`, `game_setAudioOptions`, `game_setControlsOptions`, and
`game_setDefaultOptions` restore the shipped audio/control defaults instead
of initializing the live PC runtime to zeroes.

The PC host binds only the external mixer operations to a 64-voice WinMM
backend. It loads the shipped PCM and IEEE-float RIFF/WAVE corpus directly,
while the reconstructed owner performs the exact basename lookup, six-name
loop selection, cosine stereo pan, distance, quiet/voice scaling, and live
loop-position updates. Retail applies the `0.92` SFX-volume factor to 2D
channels and `1.0` to spatial channels; the adapter consumes those recovered
channel values without recomputing their classification.
Resolution searches only the exact paths in the currently loaded banks, so
`jar_jar_playable`/`gungan_2`, Coruscant, alias/prefix subsets, and the
seven-entry cross-directory training mapping behave without filename guesses.
Headless runs never bind the output hooks, preventing automated validation
from opening an audio device or producing desktop dialogs. Exact
`sound_SetLoopingFadeTime` remains the two-argument mixer fade boundary; the
PC adapter realizes it with a time-based volume ramp and stop. Focused and
real-asset tests validate the procedure lifecycle, all 1,887 installed WAVs,
and all 1,242 bank-descriptor references.
All nine procedures in `win_audioStream.c` now retain their PDB names. The
exact 103-entry `ptrStreamWAVNames` table continues to select music, while a
separate play/control callback boundary maps loading, looping, pause/resume,
stop, volume, startup, shutdown, and channel-mode requests onto one WinMM
music voice. The installed stream corpus is uncompressed stereo PCM, so this
path also requires no codec or third-party runtime.

The complete `brain_ControlPlayer` parent retains the exact lock-on
direction selection for Motions 26, 29, 30, and 12, including its asymmetric
wrapped-angle edge cases, motion edits, target-facing call, equal-lock
activation, and 16-bit run counter. Exact `physics_gFaceTarget`,
`physics_gForceFaceTarget`, and `physics_gGetFaceTargetDelta` provide the
PDB-named target-position boundary without a new platform dependency.

Exact PDB-named `brain_GroundControl` now retains all 680 executable bytes:
energy exhaustion and current-animation death delays, unsigned timer
comparisons, Motion 14 knockdown recovery, afterlife scene publication,
both animation loop-handle stops, player game-exit flags, and the NPC
`wsl_ENEMY::exit_flag`. Its unused `target` parameter and the executable's
no-op `debug_printf` call are documented rather than replaced with anonymous
decompiler names or a new logging dependency.

Exact `brain_CheckForEffects` walks `modelObject::eventMask` with exact
`brainutl_FindLSB_LV`, copies the PDB-named `Motion::snd[1]` slot, and routes
it through exact `brainutl_PlayMotionSound` at the corresponding player
physics position. Exact `brain_DoRingOnEffect` and
`brain_DoRingOffEffect` consume `paEffects[44]` and `[63]`, move the locked
target's PDB-named `vpos`, and create/expire exact `Ring` objects. Exact
`jedi_GetColour` and the executable-backed `gJediColourCurrent` table supply
their high-byte ring colour without a platform graphics dependency.

Exact `brain_LockOn` consumes the recovered initialized
`gaButtonMap[5][6]`, toggles the original lock flag, selects the nearest
eligible actor through exact `brainutl_gGetNearestTarget`, creates the
effect-44 ring, and emits `xlockon` from the player physics position.
Exact `brain_ValidateLockOn` applies the original object-state, flag,
distance, and energy predicates; refreshes ring lifetime, colour, and
predicted target position; force-faces the target; and performs the
effect-63 teardown on invalidation. Exact `physics_gGetRange` retains the
triangular `maRange` cache and `vec_DistanceLV` fallback.

The remaining exact callback leaves preserve their PDB names and frame
thresholds. `brain_HangCallback` releases an event-marked collision node and
unhides the shadow sprite; `brain_SkidCallBack` launches effect 13 at node 6;
`brain_TakeOff` activates Motion 4 and the trajectory callback; and
`brain_ThrowEnder` activates Motion 51 with its snapshot, velocity, timer,
and delayed-motion stores. All fifteen procedures are therefore reviewed
(7,081 of 7,081 bytes), including the complete 3,954-byte
`brain_ControlPlayer` parent. Its final direct exact named cross-module
dependency, the complete 4,255-byte `braindmg_DamageControl`, is now
integrated. Exact `enemy_KillKill`,
`combo_CheckHeldPad`, `combo_ReadCombo`, `combo_CheckCombo`, the
comment-sprite path, and `force_gActivate` are already linked. All twelve
PDB-named damage procedures are reviewed (8,389 of 8,389 bytes), covering
projectile conversion, blocking, damage scaling, score/achievement updates,
feedback, hit/death reactions, and hazardous surfaces. The recovered
floating-score path keeps the exact `Draw3dText` call surface over a
dependency-free, still-partial PC renderer hook. Exact `force_PlaySeq` now
decodes the signed authored motion map, assigns the original Motion callback
indices, applies force costs and per-power state, queues chained animations,
and launches the chained effect-74 path. The exact PDB-named 50-entry
`funcArray` and `game_setFuncArray` preserve the authored callback-index
contract, including callback transfer when a queued Motion becomes active.
Every nonzero retail slot now publishes its exact PDB-named owner; slot zero
remains intentionally null. The completed set includes `ai_FireWeapon`,
`ai_Throw`, all Force callbacks, `jedi_FireWeapon`, `ai_Tank`, `ai_Stap`,
`jedi_Main`, and `tusken_stab`.
Exact `force_AbsorbReflectCallBack`, `force_AttackCallBack`,
`force_AttackSpinCallBack`, `force_CloakCallBack`,
`force_FlameCallBack`, `force_HealingCallBack`, `force_MesmerizeCallBack`,
`force_PushCallBack`, `force_Ranged3CallBack`, `force_ReflectCallBack`,
`force_RingCallBack`, `force_SabreSpinCallBack`,
`force_SabreTossCallBack`, `force_SabreYoYoBack`, `force_ShieldCallBack`,
`force_StarCallBack`, `force_TossCallBack`,
`force_TossGrenadeCallBack`, and `force_ZapCallBack` now own the reviewed force
drains, held-input continuation and cutscene-cancellation gates, masked
target scans, authored reactions, effect cadence, flags, scales, sprite
lifetimes, and item consumption. Exact `physics_FindWithinRange` restores
the mesmerize callback's 20-actor iterator. Every persistent callback
selection now points directly to its PDB-named body. The shipped
immediate-mode geometry behind `fx_screenGlow` is reconstructed exactly from
the executable and emits its six `a_glow.tga` quads; the optional `jpb_` hook
only observes those calls. `fx_GlowingMan` remains bounded. `force_ReflectCallBack` also
publishes all sixteen authored `drawCylinderG` bands through a portable
renderer boundary while preserving the exact radius, height, color, and
rotation evolution. The original SDL-specific
five-key cheat query passes through a portable optional input provider.
Exact `jedi_FireWeapon` restores single/paired ranged-player muzzle selection,
versus-aware targeting, powered projectile overrides, and the production
projectile/sprite/audio handoff. `force_FlameCallBack` restores its two
drawing-surface-dependent muzzle pairs; `tusken_stab` restores the authored
frame-10/11 collision-node window.
Exact `brainutil_PlotTrajectory` and `brainutil_PlotMaulTrajectory` occupy
their original callback-table slots and preserve authored air steering,
double-jump, timing, recovery, and fall behavior. The complete 559-byte
`jedi_InitPlayer` now supplies the live PC actor directly. It publishes the
original player-specific `combos1`/`combos2` buffer, chooses among eight exact
initialized collision tables, applies character scale and movement settings,
and installs the adjacent exact 263-byte `jedi_Main` callback. The PDB-named
`jedi_HasProgression` and `jedi_IsMelee` leaves are restored with it. An
authored CMB continues to replace the built-in combo pointer through the
existing exact loader. The real-asset jump gate proves that the actor becomes
airborne under this path, with no AI-initializer layering in player startup.
The battle droid's authored WAI payload is loaded into caller-owned storage
and exposed through exact `ai_GetAIHandle`, `ai_GetAiDataValue`,
`ai_GetAiDataValueN`, and `ai_GetAiSeqValue`. Exact `ai_LoadAI` owns all four
difficulty files for each level actor slot, including the shipped
missing/zero-length-file fallback to `dummy.wai`, rounded allocation, and
fatal missing-dummy behavior. Exact no-op `ai_Main` retains its source
signature; a named `jpb_` adapter documents the mixed callback table ABI and
supplies the deterministic continuation result used by the portable
controller.
The surrounding script records now retain their exact PDB names and fields:
`UDATA`, `BAP_AINODE`, `wsl_BAP_WAYPOINT`, and `kfNode`. The reviewed leaf
layer covers BAP tree traversal, AI-mode stacking, arithmetic and comparison
operators, timed flag restoration, waypoint bounds, near/far player
selection, the exact move/range/scan evaluators, `ai_WalkWayPoints`,
`physics_FindNearestEnemy`, post-frame state publication, enemy activation,
and the PDB-named Shaolin fixed-pool/list lifecycle. Exact `_addEnemy` and
`_checkForNewEnemies` now own the fixed enemy pool's placement-to-active-list
transition, authored/override AI binding, failure rollback, `aRange`
activation cube, and the executable's level-15 placement override. Exact
`enemy_HandleMapTriggers` resolves direct and leveldata-relative trigger
records, and exact `enemy_getPointerIndex` retains its level-dependent
one-based adjustment. The original 1,114-byte `loader_CreateEnemy` is fully
restored: it resolves the placement's canonical model and animation globals,
delegates allocation to exact `loader_CreateCharacter`, binds the world target,
enemy, and authored actor-slot WAI handle, then applies shadow ownership,
placement position/facing/energy, `player_RefreshPlayer`, signed animation-pool
relocation, the first eight sequence repairs and motion lock, movement-mode
mapping, model-specific motion callback/flag patches, and exact
`ai_ValidateData`. The live PC runtime publishes its immutable BMD/CAD class
archives through the exact loader globals and only observes completed actors
for renderer diagnostics; that observer cannot create an actor or alter the
loader result. Exact `_addEnemy` therefore owns each supported actor's pool,
placement, and active-list transition without a parallel manual insertion
path. The asset boundary does not force
a convenient nearest placement: the normal first
`enemy_HandleEnemies`/`_checkForNewEnemies` frame applies exact authored
`activeFlags` and `aRange` activation. Asset loading likewise leaves the
single-player target paired with the valid inactive player-1 slot; it does not
substitute a convenient enemy. The runtime's diagnostic primary-enemy view
follows a live target only after gameplay has selected one, and cumulative
damage, energy-minimum, reaction-motion, and recoil observations span all active
actors instead of being overwritten by whichever actor is published as primary.
The restored tail follows `loader_CreateEnemy` through exact
`player_RefreshPlayer` rather than starting CAD motion zero directly. That
owner restores placement position/facing/energy, clears transient player
state, and seeds the animation queue before the loader applies its remaining
animation and movement repairs. This is observable gameplay behavior: the FED
room's owner-3 AI 31 director otherwise becomes stuck in model motion 3 and
never executes its authored `0x604` camera/letterbox command.
The complete seven-procedure `shaolin.c` module is now reviewed, including
attacker budgets, formation points, low-chi scheduling, attack delays, WAI
choice/sequence lookup, movement, target switching, committed exact facing,
and authored motion ownership. Its exact AI-side
`ai_HthAttack`, `ai_RangedAttack`, `ai_SeqAttack`, `ai_SetTarget`,
`ai_WalkToPoint`, and `ai_WalktoPlayer` procedures are also restored. The
live field keeps the original valid-but-inactive player-1 invariant.
The persistent multi-class archive owner publishes the matched executable's
BAF table (`0x4BD2B0`),
`sModelNames` (`0x4BCF10`), and model-animation records (`0x4BD730`) map
FED `baron`, `21b`, and `baronsec` to model/animation pairs
17 `battle_d`/`battle_d`, 15 `pilot`/`pilot_d`, and
62 `security`/`battle_d`. The same tables map `r3po` to model 12
`protocol`/`droid` and `destroyr` to model 26
`destroye`/`destroye`, `hovdroid` to model 30 `loader`/`loader`, `drdfitr` to
model 47 `droid_f`/`droid_f`, `pwrdrink` to model 72 `beacon`/`beacon`,
`pwrserv1` to model 86 `fed_door`/`fedship`, `reeyees` to model 87
`piston`/`fedship`, and `twilek1`/`twilek2` to models 94/95
`lift_1`/`lift_2` with `fedship` into `maModelID`, `maModelData`, and
`maAnimData`. Each active placement gets independent original-pool scene,
model, physics, animation, player, and enemy state from the canonical loader.
Direct executable RE also confirms that `_initEnemy` leaves the embedded
`wsl_ENEMY.actorNum` field zeroed; the placement's `actorNum` is the sole
loader/AI class key, and the runtime validator now follows that exact owner.
The complete 114-pointer `sModelNames` table is now readable source rather
than runtime-only evidence. Exact `player_gConnectMotionData` consumes each
CAD header and supplies the original relative Motion table, legacy hit-name
normalization, and callback-index publication for those player objects.
The complete seven-procedure `unpack.c` owner is reviewed against PDB layouts
and executable RVAs `0x103260..0x10396B`. It preserves the two-word bit
reservoir, direct and slow-tree Huffman lookup, cached value count, first-row
signed translation rules, later-row doubled deltas, optional pad word, ignored
`tree_size`, CAD-relative context pointers, and byte-offset seek truncation.
A dedicated regression crosses refill boundaries through both compressed and
raw paths and locks every `_dpcontext` field offset.
Immutable archives, textures, and WAI storage are shared only within the
class, and each actor releases its per-object animation slot when exact
range/despawn ownership removes it. The WAI registry key is the authored
level actor slot passed by the exact loader, not the global model ID; this
also keeps security's model ID 62 out of the 20-slot AI registry.
The portable frame processes the exact intrusive enemy lists once and then
controls, poses, class-renders, and collision-checks every surviving record.
The FED regression gate loads all 12 actor-table classes and distinguishes the
11 classes that occur in authored enemy placements; `twilek2` has an actor
record and valid assets but no FED placement. The nine-frame pass proves an
18-actor peak, 30 authored spawns, seven classes simultaneously active, and
all 11 placement-backed classes activated and submitted through exact
`_addEnemy`. Collision-enabled machinery models also prove the bounded
portable resolver for immutable BMD geometry streams at the original
`loader_CreateModel` `addPtr` relocation seam; exact registry-backed models
continue to fall through to `getPtr`.

The model archive boundary now flows through the exact PDB-named `model.c`
owner rather than a host-only hierarchy builder. Recovered `modelSpace` and
`TextureTracker` layouts establish the original 20-model registry, 32-node
per-model storage, 128 texture trackers, and 256-row texture packer. Live PC
player and enemy creation calls `model_gInitModelRoot`/`model_MakeNode`, while
the bounded geometry extent is supplied by the portable archive owner. The
retail five-stream `addPtr` mutation and `transabr.bmp` saber substitution are
now exact; the portable BMD view resolves those stored indices through
`getPtr`. The exact `TT_TEXTYPE` enum, PDB-named 800-entry `g_material` pool,
`texture_GetMaterial`, `texture_FreeMaterial`, `_LoadTexture`, and
`_FreeTexture` ownership are now live. When the PC pixel-resource hook is
installed, `model_MakeNode` performs the retail replacement of the texture
name union with `_Material*`; the software renderer consumes that handle.
Without a platform hook, bounded probes preserve the source name for readable
archive inspection.
`levTexParseFarce` also retains its PDB name and exact high-bit level/number
encoding, leaving debug-only `console_NodeCommand` as the module's sole
bodyless PDB entry.

The complete 3,822-byte PDB-named `ai_InitPlayer` now owns live PC player and
enemy settings. Both executable switches cover all 62 explicit model IDs and
the default path, including asymmetric scale, clip radius, closing distance,
mass/height, callback-table selection, movement overrides, and model/player/
physics/root flag mutations. Its common tail writes `minClosingDist` before
publishing the physics radius, restores the exact 22-record fallback `combos`
block, and publishes the seven shared movement words. Hash gates compare all
21 selected collision tables with the matched executable bytes; this exposed
and corrected the older `maDesert_BNodeSizes`/`maWormNodeSizes` initializers.
The three-byte PDB-named `ai_InitModelData` is the exact empty companion, so
both `settings.c` procedures have reviewed bodies. Model 26 reaches complete
six-byte `ai_Destroyer`; model 30 reaches the complete PDB-named
`ai_LoaderDroid`, which retains the Loader Droid's arm-node effects/detachment,
target grab/throw sequence, named static state, and level/boss reset gates.
Its exact `ai_ShowFlags` dependency and the matched release build's inert
`debug_printf` are also recovered. The settings-selected callbacks
`ai_Blades`, `ai_Worm`, `ai_Krakis`, `ai_Mtt`, `ai_AAT`, and `ai_Deadly`
now occupy their exact table slots and retain blade spin-up, collision-sphere
diagnostics, the MTT proximity damage volume, AAT turret/projectile timing,
and deadly-force flag behavior. Exact `debug_drawsphere` arguments cross a
dependency-free optional renderer hook; with no hook, its optimized release
behavior remains inert. All 18 PDB-emitted `boss.c` procedures are reviewed
(11,000/11,000 bytes), and callback-table stores 27 through 29 and 34 through
47 now reach their exact PDB-named owners. Alongside the Maul, Thug, Jar Jar,
and Starfighter paths, `ai_Kadu` retains rider attachment, race cadence,
camera focus, and authored progress bars. `ai_TurretDroid` retains event-node
projectiles, zap raycasts, aimed head rotation, shield-sprite ownership,
strafe shots, and detachable arm physics. Exact `centreturret` preserves the
signed-12-bit,
eight-unit-clamped return step used by the tank path. All six PDB-emitted
`vehicle.c` procedures are reviewed (6,860/6,860 bytes). Exact PDB-named
`ai_Stap` and `ai_Tank` restore multi-driver ownership/dismount, STAP
lean/speed and catch-up, tracked movement, articulated gun/turret aiming,
projectile cooldowns, and looped vehicle audio. Their shared exact 428-byte
`FindBestMachineGunTarget` dependency preserves the actor, energy, range,
height, and wrapped-facing filter.
The complete 1,730-byte neighboring gameplay cluster is also readable source:
`physics_GetPoly`, `physics_InitPhysics`, `physics_MapAnimCallBack`,
`physics_gCalcTargetPos`, `physics_gCheckGround`,
`physics_gCreateObject`, `physics_gGetNearestTarget`, and the 462-byte
`player_DoCollisions`. These restore the range-cache reset, polygon/surface
ground classification, two-player target transform, physics component
linkage, typed target selection, and the exact player attack/contact filter
and hit-publication stores. The portable PC frame calls the recovered
collision owner directly; only diagnostic hit counting remains host-side.
Models 72, 86, 87, 94, and 95 have their exact machinery branches, including
scale, collision table selection, dimensions, flags, and node depth offsets.
Model 47 has its exact six-entry collision profile at `0x4CC7D0`,
model/physics flags, callback slot 40, and
the reviewed PDB-named `ai_StarFighter`, including exact
`physics_ForceFaceLock` and its twin-shot emission. Exact `Projectile`
layout/allocation, projectile sound initialization, 3D sprite ownership, and
`bullet_ShootProjectile` now support that path. The large
`coll_CheckProjectileCollision` body now supplies collision-node size tests,
the authored Starfighter radius case, normal hit publication, and saber/Force
ricochet re-fire behavior. The complete 2,178-byte `bullet_CallBack` now owns
the reference ballistic and target-homing steering, one-eighth stored-speed
movement through exact `MoveObjectNormal`, ordinary and alternate sprite
updates, lifetime/impact/trail effects, termination and bounce audio, masked
nearby-player scan, piercing/persistent/reflected hit transitions, both
authored level-specific terminal-hit rules, and the type-6 map explosion and
camera-shake tail. Its plasma path calls exact PDB-named `fx_PlasmaZap` over
the exact 156-byte `_plasma_zapvars` state. Exact `bullet_Explosion` publishes
radial hits. All eight `bullet.c` procedures are reviewed.
All 27 `collisn.c` procedures are also checked through their complete
`0x25630..0x26665` executable range. The owner preserves masked public node
lookups but uses the shipped signed-byte indexing for hot-node and parent-node
access, wraps the emitted 32-bit contact arithmetic before division, performs
three independent reflection random draws, returns `-1` after reflection, and
terminates invalid registration through the CRT `exit(1)` path. Focused
normal/fatal tests and the direct combat, player, physics, AI, model, powerup,
enemy, and scene consumers pass.
The PDB-named `enemy_ParseOpcodes` call surface implements the matched
node/branch traversal and all 35 executable top-level opcode
values found across all 27 shipped J3D archives. The audit covers 42,581
decoded nodes, including 35,335 executable nodes after structural zero/one
records are excluded. It resolves immediate and relocated operands and
performs exact mode/child/sibling selection. All 22 shipped `0x606` subcommand
IDs and all 530 authored nodes route through recovered behavior. This includes
power-up control,
effect/audio dispatch, streaming-music selection, destructible-map events,
live-player replacement, special-menu messages, and command 11's exact
`level_SparkRoom` five-arc hazard. Its PDB-named `zapcheck`,
`vecpointlinesquared`, and `PlotZap` dependencies are recovered as well. Exact
tank/STAP entry through `0x607` and live-player attach/detach through status-4/5
`0x60f` are recovered. Their dead-player routes now pass through exact
PDB-named `player_RefreshPlayer`, including start/checkpoint/placement
position, facing, character-state, shadow, reset, and animation-queue
publication. A `jpb_` diagnostic companion retains unknown-data reporting and
a malformed-cycle bound while the original valid-data surface preserves the
matched no-op/debug-reset continuation rules. The bounded
672-byte `anim_ForceNextAnimSeq` owns forced queue activation from the six
reviewed `animctrl.c` wrappers and player reset, including prior-motion flag
propagation, recovery, rate and physics setup, callbacks, sounds, tween
selection, and immediate frame publication. It reaches exact PDB-local
`anim_CreateTweenFrame`; its fixed-point
root/joint deltas, 17-entry fraction table, 16-frame clamp, pose countdown,
slack transition, sequence-end, and `SpeedAcc` behavior publish through the
reviewed `anim_GoNextAnimFrame` boundary. Exact `anim_ProcessAnimations` now
owns the live 20-slot player/enemy PC pass after gameplay callbacks select
motions. Exact local `anim_SkipToStartFrame` consumes authored pre-roll pose
data without publishing pre-roll events, and target motions use the target
animation's secondary depack context.
All 22 emitted `animutil.c` procedures have reviewed bodies.
Exact local `anim_SoundStart` and `anim_HandleSound` retain both authored
sound channels, immediate/delayed/negative-loop timing, stop markers, bank
selection, and tank/taxi replay vetoes through the portable sound hook.
Exact `anim_GlobalInit`, `console_AnimCommand`, and the complete four-owner
`resources.c` path surface restore animation bootstrap and debug mutation;
SDL base-path discovery and the loading delay remain narrow dependency-free
host seams. Audio playback and menu
realization use narrow dependency-free host hooks. Exact `ai_DefendCheck`,
`bapenemy_preFrame`, `bapenemy_postFrame`, `_deleteEnemy`,
`player_FreePlayer`, and `obj_gClearObject` restore the normal defend,
state-transfer, placement-link, loop-sound, target-repair, and object-cleanup
leaves. The live runtime now enters the dependency-light
`jpb_enemy_ProcessActiveFrame` owner, which performs activation, normal BAP
dispatch, Shaolin resolution, range/despawn decisions, and enemy-list
double-buffering as one frame. Its exact recovered preamble handles the
process timer, timed AI flags, tank countdowns, achievement/score/next-level
events, point sprites, and the level-13 player-count bit. The two per-enemy
special cases are evidence-complete: level 6 enemy `0x75` receives a 10,000
deactivation range, while level 7 enemy `0x3a` clamps its physics Z to
`-14800`. Exact `enemy_ResetEnemies` and `enemy_SetTeleport` now own reset
and teleport publication used around that frame path. The complete 7,884-byte
`enemy_ParseOpcodes` valid-data call surface is now reviewed across every
matched dispatch branch. The
1,929-byte `enemy_HandleEnemies` call surface now owns the reviewed frame and
its exact debug-level branch. Exact 947-byte `enemy_Radar` retains the
matched screen scaling, camera rotation, range/type filtering, marker colors,
and `_DrawTexture` ordering; the portable runtime supplies a dependency-light
solid-rectangle realization after scene presentation.

The remaining `enemy.c` inventory now has no bodyless procedures. Exact
nearest-waypoint selection, positive per-placement point totals, per-actor
model accounting, console-driven flag/placement mutation, and deferred
teleport application are represented. The teleport owner moves every nearby
non-source enemy plus the active player set, republishes scene transforms,
retains both shipped coordinate exceptions, and clears the one-shot state at
the end of the portable PC frame. Its original call into `camera_SetCameraPos`
is now direct.

The `camera.c` inventory also has no bodyless procedures: all 23 PDB-listed
entries have source bodies. The recovered 2,093-byte `camera_SetCameraPos`,
832-byte `camera_SetCameras`, and 2,785-byte local `camera_StuffCamera` own
collision-selected authored dollies, transition-frustum rejection, one- and
two-player focus modes, follow/slack/uber clamps, focused and Streets/JarJar
special modes, shake, and final camera publication. The portable frame calls
that owner directly and no longer carries a duplicate host camera builder.

The post-scene saber boundary now retains its human-readable PDB ownership.
Exact `player_HandleSabre` iterates the 20-player pool with the executable's
scene/model/player eligibility and model-ID gates. PDB-named
`jedi_HandleSabre` selects the authored node pairs and executable-backed
colors, constructs fixed-point normal and long blades, handles the double
blade, performs damaging-motion world sweeps and feedback, and publishes the
tip-node timing state. `fx_screenGlow` now reproduces the executable's
camera-space 12-vertex construction and six textured no-scale quads. Software
and D3D consume those same immediate polygons; the former host-authored line
renderers are removed. Textured blur and powered cylinders remain separate, so this closes the live ordinary saber path
without overstating immediate-mode presentation parity.

The adjacent PDB-named `player_gProcessPlayers` now owns the live post-render
gameplay schedule. Its executable-backed source runs `player_DoCollisions`
first, iterates all 20 player records, sets the exact two/20 attacker budgets,
updates player map triggers, routes independent controller or AI input through
`mPlayerRead`, preserves `mCharliePad`, applies level and pause suppression,
and calls exact `brain_ControlPlayer` once per eligible actor. The portable
runtime observes that boundary only for regression metrics. Exact
`_AddLifeTile`, `_DrawTile`, `_DrawTile2D`, `fRotTransPers`, viewport scale
helpers, and scaled energy/Force getters now restore its ordinary life/Force
bars. The scheduler's interleaved human-player damage indicator also draws and
decays through exact PDB-named owners. A narrow dependency-free adapter
projects these solid rectangles from the published `CameraMatrix`; developer
diagnostic labels remain explicitly tracked HUD work.

### Immediate polygon finalizers

The complete matched bodies for `el_chavo::EndPoly` (RVA `0x115E90`, 5,208
bytes) and `el_chavo::NoScaleEndPoly` (RVA `0x119280`, 5,529 bytes) now own
the portable immediate-polygon finalization path. `StartPoly` selects its
ordinary/additive/alpha queue from the low byte of `_LoadTexture`'s option;
the separate lowercase `p_`/`a_` filename classification remains only the
retail `Texture::m_tpfDesired` field. This fixes sprite materials loaded from
the default white texture, whose filenames cannot carry the queue class.

`EndPoly` applies the executable's exact `0.0035`, `2/3`, and `1.16` NDC
conversion and submits through a cull-none PSO. `NoScaleEndPoly` applies the
recovered projection and `ApplyCulling` tests, always treats opaque material
flags as zero, preserves the Corus2 bus and Palace coffin exceptions, and
uses flags `0/1/2` only for transparent polygons. Flag 2 forces transparent
depth to `0.0001`; it does not alter opaque depth. Transparent quads use the
retail triangle-list indices `0,1,2` and `1,3,2`. Focused regressions separate
all of these cases, including the former extra winding test that caused saber
caps to flash.

The dependency pass also restores the timing and interpolation rules around
those finalizers. `StartPoly` copies `_Material.flags` into the selected
texture before any vertex is written, and the portable publication boundary
therefore snapshots that value rather than rereading a mutable material at
deferred-render time. `SetVert` computes retail's four-bit screen outcode;
only ordinary `EndPoly` ANDs all submitted outcodes and drops a polygon wholly
outside one side. `NoScaleEndPoly` deliberately does not use that gate.

Both finalizers build the recovered 52-byte renderer vertex with position W
equal to `1.0f`. The D3D11 immediate path now has a separate vertex shader that
preserves that W and the post-projection Z, so UV and color interpolation are
affine in projected space. The earlier shared model shader reconstructed a
camera-space W and silently made these polygons perspective-correct. The
software path now follows the same affine rule while retaining its separate
linear-depth value for world compositing. The adjacent `LoadTexture` body also
performs the executable's case-sensitive final-extension rewrites from `sgi`
to `tim` and `pvr` to `tga` before the platform resource handoff.

### Model projection and opening-camera evidence

The opening FED composition was checked through the full matched model path
before changing camera or projection code. `scene_UpdateWorld2ScreenMatrix`
publishes the recovered 180-degree X coordinate conversion, and
`render_CreateRenderPacket`/`_RenderNode` submit camera-space model vertices.
The 614-byte `el_chavo::ApplyProjectionPolyArray` at RVA `0x113050`
multiplies those vertices by the application projection matrix and performs
the perspective divide. The 5,529-byte `el_chavo::NoScaleEndPoly` at RVA
`0x119280` negates divided Y, assigns W=1, and submits to the model vertex
shader; that installed shader is a passthrough, so the D3D viewport produces
the same positive-screen-Y convention as the portable rasterizer. The
existing 53-degree PC projection is therefore retained. A movement capture
confirms the authored camera naturally brings Obi-Wan from the distant
doorway into foreground scale.

The remaining opening mismatch was not projection or mesh winding. FED AI 31,
attached to the `pwrserv1`/`fed_door` placement, changes from initialization
mode to its `cam1` mode on the first authored AI pass. On frame two it executes
`0x604(144, 1)`, selecting dolly 144, setting the retail control lock, and
enabling letterbox ownership; its following timed mode selects dolly 146. The
director's gas sequence then depends on opcode `0x108`: the original reads the
first `BAP_AINODE` short (`iParent`) and lets normal traversal resume at that
parent branch's sibling. Treating it as `iChild` stranded the sequence on the
dramatic close-up. The recovered `bapEnemySetContinue` owner now supplies the
exact operation, allowing all gas branches to rejoin and advancing to authored
dolly 145. Real-asset regression gates load the complete FED enemy set and
require both the frame-three dolly-144/control-lock state and the frame-180
dolly-145 director state, while the collision-only spawn gate continues to
prove that the underlying walk polygon correctly selects dolly 3.

A second instruction audit corrected the local 2,785-byte
`camera_StuffCamera` movement-lead owner. Retail first normalizes the existing
module-local `lead`, projects it onto `(sin(yaw), 0, cos(yaw))`, and only then
uses that `cameralead` to advance and decay the next vector. The earlier source
sampled the already-updated vector and transposed sine/cosine, producing a
different sustained-movement framing. Static references prove
`gGlobalFrameRate` and `fGlobalFrameRate` are initialized to `0x800` and `0.5`
and never written by the matched executable. At that 60 Hz scale,
`camera_CameraSlide` can advance pitch by one unit and
`camera_Camera2ViewVector`'s required even-angle mask would erase that same
unit on the next frame indefinitely. The recovered camera owner retains both
operations exactly. An earlier portable PC boundary carried the odd result
into a new even value, but a complete `game_OneGameLoop` audit found no
corresponding executable operation between `camera_SetCameras` and
`scene_middleRender`. The compensation is therefore removed. A focused unit
regression preserves the exact half-rate mask/slide result rather than
inventing a new camera owner.

An additional reference comparison covers the director-to-gameplay handoff.
The executable path and shipped `fed.cam` were re-audited down through
`camera_SetCameraPos`, `CalcNewBox`, and the raw 256-record table. Subsequent
visual review rejected the PC composition and prompted a second ownership
audit. That audit proved the portable rule which retained dolly 145 while the
player remained in collision region 3 had no executable owner. It has been
removed rather than retuned: the exact director clears its override, the
stationary player selects raw collision dolly 3, and the authored 420-frame
traversal reaches collision dolly 0. Installed-asset gates now protect not
only those indices but target projections `(526.2,217.0)`, `(639.7,238.5)`,
and `(429.3,263.9)` at the frame-360, frame-480, and moving frame-420
boundaries. The first two remain green. The current staged baseline produces
`(444.2,300.8)` for the moving boundary, so the third stays red and is not
rebaselined by unrelated menu work.

Those gates prove deterministic recovered ownership and projectability. A
later settled-room comparison against the supplied gameplay frame aligns the
doorway, horizon, and player scale closely enough that the earlier static
composition rejection is no longer reproduced. Dynamic follow, transitions,
and the remaining levels still require a full camera survey; no geometry,
projection, depth, or winding change is implied by that remaining work.

The candidate-rejection branch was then checked independently rather than
assuming dolly 3 should be accepted. The recovered `camera_SetCameraPos`,
`twatcameramatrix`, `buildfrustrum`, `buildplane`, `CalcNewBox`, and
`cliptofrustrum` paths match the decompilation's arithmetic order, truncation,
single-player radius, and executable-backed `90/320/180` frustum constants.
For the installed FED start record that test accepts collision dolly 3 exactly
as the current runtime does. Keeping dolly 145 after its AI owner clears the
override would therefore add policy absent from the matched binary. The
settled handoff now matches the available static reference landmarks, while
reference timing around the transition remains part of the dynamic survey.

### Portable matched-PC text boundary

The default initialized `optionstruct` selects overlay mode 1, whose score
owner calls `SDLTextWriteScale` with style 2 and scale 3.24. Instruction and
PDB traces for exact `getFontFile` (RVA `0x176E0`), `LoadFont` (RVA
`0xFDC90`), and `SDLTextWriteScale` (RVA `0xFDF40`) establish:

- regular, italic, and bold selection from the shipped NotoSans files, with
  the language-6 branch selecting `NotoSansSC-Light.ttf` for italic;
- point size `trunc(scale * scaleAdjustment * 24)`, giving 87 at the PC
  runtime's 960x540 framebuffer for the score call;
- width-based left/center/right alignment from the low seven mode bits; and
- the exact 17 initialized CVECTOR tints at RVA `0x4CD100`, with caller alpha
  replacing the table entry's high byte.

`portable/portable_text.c` realizes this original SDL_ttf platform boundary by
dynamically resolving the shipped `SDL2.dll` and `SDL2_ttf.dll`. It reads the
original `res/font` assets through `resource_getPath`, caches faces by shipped
filename, measures and rasterizes glyphs through SDL_ttf, and alpha-composites
their surfaces into the caller-owned software framebuffer. There is no compact
or synthetic missing-font fallback: an unavailable SDL/font boundary remains
an observable failure. Focused and installed-asset tests cover the recovered
selection/constants and real bold-font rendering.

The matched initialized `allTextEverything` global at RVA `0x4A1000` is also
represented in source under its PDB name. Retail uses a compact 2,725-pointer
aggregate: slots 0..126 are shared and slots 127..497 select one of seven
371-entry language tails. Display targets are UTF-8 byte strings despite the
PDB's `wchar_t*` aggregate type. `alltext_data.c` retains the exact compact
aggregate, including the `sModelNames` and `sLevelNames` pointers in slots 0
and 1, all 68,317 text bytes including per-entry terminators, and the terminal
empty pointer. Exact `generateAllText` (RVA `0x17640`),
`UpdateCurrentlyLoadedFont` (RVA `0xFEB00`), and `MarkFontAtlasForRefresh`
(RVA `0x1236D0`) now publish the selected byte pointers during runtime startup.
Each live `SDLTextWrite*` owner performs the recovered per-draw
`ConvertToUTF16` call; there is no eager host-wide conversion cache. A
whole-table FNV-1a regression hash (`61eb339ccd58c0ef`) guards the recovered
corpus.

### Recovered title-menu ownership

The title/menu path is no longer represented only by isolated HUD helpers.
PDB type `MENUVARS` (`0x6DBE`, 984 bytes in the matched x64 build) now has a
named source layout through its complete title stack, input buffers, score
state, and player-selection tail. Offset assertions protect `menuMode`,
`menuModeSP`, `mmvTriggers`, `frKeyBuff`, and `pointSeek`. Native pointers
remain fields rather than serialized offsets, allowing the runtime structure
to compact on a later 32-bit target without changing game-owned access.

The recovered procedures now include `menu_mainInitMenu`,
`menu_enterTitleMode`, `menu_pushMenu`, `menu_popMenu`, player-count/model
selection, objective and game-over transitions, training/level transitions,
title/screen-saver leaves, controller-shock selection, and the original menu
eligibility predicates. Platform input enumeration, texture loading, bucket
setup, and single-controller reassignment are explicit hooks at the same call
sites. The PC gameplay initializer consumes `menu_setNumPlayers` and
`menu_setPlayer` rather than publishing those global states independently.

Ten exact initialized title-definition streams from RVAs
`0x4C5E70..0x4C6638` are retained under their PDB names, including `mainMdef`,
the Continue variants, the No-Load variants, and registration-disabled
variants. The interpreter no longer substitutes a replacement menu schema.
Exact PDB
types `MMVDEF` (`0x6E17`, 48 bytes) and `MDEF_MOD` (`0x6EC4`, 32 bytes), the
75-entry initialized `mmsizes` table, and the PDB-named `mmNextCode`,
`mmDrawItem`, `mmDrawsub`, `mmDraw`, and `menu_mainMenu` owners now decode the
ordinary title stream directly. That includes anchored 16:9 positioning,
scrolling selection offsets, localized selected/unselected text, the title
selection panel, nested accept/decline prompts, edge-driven wrapping, and the
destination activation seam.

Sixteen additional streams are now retained as well: the three title player-
count variants, `difficultyMdef`, `newgameconfirmMdef`, `optionsMdef`, and
`rusureQuitMenuMdef`, followed by both Controller selectors and player panels,
Language, Video, `audioMdef`, `audioMdef_Game`, and `audioMusicMdef`. Their PDB
array types establish exact extents at RVAs `0x4C66D0..0x4C68C4`,
`0x4C6ED0..0x4C7498`, and `0x4C7DF0..0x4C7E54`. Together the 26 definitions
contain 3,896 exact bytes.
A whole-stream FNV-1a check (`d4be42c48c32b0a2`) and focused state-transition
tests guard the complete recovered command corpus and behavior.
PDB procedure `menu_handleMenuTriggers` now represents the matched
executable's complete compressed destination table. It owns ordinary pushes,
New Game/VS setup, player-count and difficulty routing, character
confirmation, level eligibility and scanning, score awards and upgrades,
controller fallback, video apply, save requests, sound/music controls, cheats,
and direct gameplay state changes.
The recovered `menu_cameraChange`, `menu_menuMusic`, and `menu_sound` leaves
remain dependency-light through the existing camera/audio platform boundaries;
`tempPlayersVs` keeps its PDB global name.

The PC host's `--title` presentation uses Windows Imaging Component only at
the Win32 edge to decode the installed frontend images; no image library
enters portable game code. Exact PDB global `menuTextureList` retains all 132
records from RVA `0x4CAC10`, and PDB-named `menu_winLoadTextures` resolves
them through `resource_getPath`, `_LoadTexture`, `menuTextures`, and the exact
460-entry `fontSpec` layout. It then loads the matched 77-record Controls
tail: generic controller, KBM, KBM Force keys, PS4, PS5, Switch, Switch Pro,
Joy-Con, and Xbox Series X. The Win32 hook publishes native dimensions and
opaque `JPBSoftwareTexture` resources. The exact post-table level-preview
loop additionally loads the 15 `loadscreens/src/orig` images into
`menuTextures[80..94]` and publishes their quarter-size records through
`fontSpec[410..424]`, yielding 209 unique decoded images across the installed
front-end and controller banks.
`jpb_GameRuntimeTitleFrame` composites the recovered menu
commands through the existing caller-owned framebuffer and TrueType hook. It
replays screen tiles in descending layer depth with stable equal-depth order,
matching the depth-tested menu bucket rather than hiding nearer portraits
under later-submitted overlays. The text publication record carries its recovered scale owner,
so title draws use `scaleAdjustmentMM` while gameplay HUD draws continue to
use `scaleAdjustment`. `jpb_pc_title_smoke` verifies all 13 initial localized
draws; scripted real-asset gates additionally select New Game, navigate to
Options, and enter Audio, verifying the resulting states and a real Music
toggle. A separate `--title-character-select` diagnostic enters state `0x0E`
without claiming to resolve the still-open retail handoff; its installed-asset
gate verifies 19 P1 presentation draws, no drops, all 209 unique installed
frontend/controller images (including all 128 unique frontend-bank images),
and a bright selected-character pixel after layer composition. The exact
74-entry `modVars` table is rebound to recovered globals.
`mmGetModVal`, `mmSetModVal`, `mmIncVar`, `mmDecVar`, `mmUpdateModSet`, and
`mmDrawMod` preserve typed storage, selection locks, range behavior, known
side effects, localized formatting, and editing. Exact
`menu_playerSelectCheck` now gives player two its retail reversed left/right
modifier edits, activation trigger, and back path; `menu_menuExit` owns the
stack exit and both input-mask refreshes. PDB-named `jedi_CheckValidLevel` and
`menu_setScoreMode` own level gating and score-award progression. PC storage
uses the recovered pointer-free `saveGameStruct` and `optionstruct` layouts at
the original `SAVEDATA0\\Game` and `SAVEDATA0\\Options` names. Interactive
startup loads both, dispatcher save requests write Game, and clean shutdown
writes Options. Exact-size/version validation applies through a C-only file
boundary and leaves headless runs deterministic. Controller enumeration,
video-mode application, and level resource work remain narrow dependency-free
platform callbacks. PDB-named `runControlsMenu` and
`getControllerTextures` now own states `0x23`/`0x24`, the exact
Classic/Modern and Force mapping arrays, localized labels, controller overview
layout, and all retail PC controller-family selection. Unresolved in-game
modifier behavior and full-game camera framing fidelity remain to be
recovered. Every ordinary user-reachable title branch now has a navigated
installed-asset gate: Language regenerates `allText`, both Quit destinations
cross their recovered return/host-exit boundaries, and VS carries independently
scripted P1/P2 confirmation through the two-character selector into a rendered
Arena frame. Explicit development world paths derive the installed asset root
from their `res/level/jpx` ancestry; normal staged runs retain executable-root
ownership.

The matched title flow retains an unresolved control-flow fact inside the
recovered owner. `newgameconfirmMdef` destination 9 enters state 3;
`titlePlayerCountMdef` destinations `0x6A/0x6B` enter state `0x37`; and
`difficultyMdef` destinations `0x68/0x69` enter state 4. The exact state-4
owner then calls `menu_setNumPlayers` and interprets
`playerCountSelectMdef`, whose selectable destinations are again
`0x6A/0x6B`. No recovered caller or global stack writer supplies a proven
state-`0x0E` character-selection handoff. All of those destinations, streams,
and stack operations were rechecked against the executable, so
`menu_mainLoop` records the loop as an evidence gap. The installed PC host
now supplies an explicitly named presentation bridge at that boundary. It
enters the independently recovered character selector, whose success pushes
state `0x1A`. PDB-named `menu_initLevelSelectScreen`,
`menu_drawLevelSelectScreen`, and `menu_levelSelectMenu` then own the retail
selector using exact `levelSelectMdef`, level gating, localized labels, and
the 15 installed preview images. Its recovered composition now includes the
shipped orb backdrop, nine-piece carousel/selection marker, bold stage digits,
and the executable's 2.5 title/name scale. Main-menu cyclic items are clipped
at rasterization time to the exact command-`0x44` selection-panel bounds, so
the compact command stream no longer leaks duplicate labels into the surrounding
presentation. Confirmation pushes recovered load state
`0x66`; only then does the host invoke the exact `menu_levelSelect` leaf and
reinitialize the selected JPX/FBX map with the selected character's installed
CAD, BMD, and combo data. This integration policy does not alter or
misattribute the matched menu procedure.

The PC persistence bridge also distinguishes a valid recovered save payload
from a missing or rejected file. In the latter case it replaces the default
overwrite-confirmation push with recovered destination 9, the no-save New
Game path. User-reachable mode `0x0C` runs the recovered PDB-named
`newMenu_Training` owner and its installed presentation. Modes `0x13` and
`0x1B` now call the
recovered PDB-named `menu_drawCredits` and `menuConceptMenu` owners directly.
Credits uses the exact 706-entry `theCredits` pointer array, heading marker,
scroll timing, and eight-track rotation; Concept Art uses all 42 recovered
texture-bank pages, original wrap controls, arrow states, page counter, and
16:9 framing. Training selection continues into its installed level asset,
while the Quit
confirmation destinations `0x93` and `0x94` now cross the explicit host-exit
boundary or return to the prior menu. The obsolete external registration
destination is removed from ordinary PC presentation.

PDB-named `newMenu_P1CharacterSelect` and `newMenu_P2CharacterSelect` now own
the one- and two-player selection state machines independently of that open
handoff. The latter preserves its shared confirmation bitmask, duplicate
model rejection, single-controller second-selection routing, disconnect
guards, base/New Game+ persistence, VS defaults, and exact success/abort
semantics. Main-loop state `0x0D` owns the VS entry setup; state `0x0E`
dispatches the appropriate owner from `GameStruct.NumPlayers`. The local
4,470-byte `newMenu_DrawP1CharacterSelect` now owns the installed background,
main/adjacent portraits, player containers and overlays, arrows, name/skill
panels, weapon icon, progression values, and localized labels. PDB-named
`newMenu_DrawArrows`, `winDrawBackground`, `jedi_CalcSkillLevels`,
`jedi_ConvertToTextIndex`, and `jedi_GetColorSprite` supply their recovered
constants and tables. Its recovered completed-game controller tabs select the
active player's Classic/Modern controller glyph pair and use the exact centered
positions, dimensions, and pivot. The local 6,195-byte
`newMenu_DrawP2CharacterSelect` now owns the recovered mirrored headings,
arrows, divider, portrait/overlay/detail stacks, resolution-aware name panels,
weapon-color icons, and progression panels for both players. Its Win32
real-asset gates composite the base 22-draw/eight-string view and the exact
mirrored P1/P2 controller tabs at 26 draws/twelve strings without adding a
frontend image dependency.

The PC handoff now consumes state `0x0D`'s recovered VS completion at
`LevelSelect == 25`, resolves the installed `arena` JPX/FBX pair, and promotes
the runtime's formerly inactive second world-player slot into a live player.
That portable state owns the second selected character's CAD, BMD, combo
table, model, animation, physics position, controller channel, target
relationship, pose publication, rendering, and shutdown. The original arena
start-position pair remains the spawn authority. An instruction-level recheck
proves `loader_LoadJedi` selects camera type zero for every two-player game,
including Arena. Applying it directly exposes the shipped Arena library
floor's derived camera record 1,152 bytes beyond the finite relocated J3D
allocation and leaves player two behind dolly zero. Until the missing
allocation/library ownership is recovered, the PC VS bridge keeps its isolated
midpoint orbit so both players remain visible; the bounded record check
prevents the exact address calculation from becoming a host heap read.

The title frame now enters exact `menu_readControl` rather than duplicating a
player-one edge poll in the host adapter. Both pads pass through the reviewed
input mask owner, both 16-entry command histories advance, Zoom-Out is removed
from cheat samples, and the recovered screen-saver count/alpha state runs at
the original 18,000-frame threshold. PDB-named `checkKeyboardBuffer`,
`cheatCheckKeyboard`, `cheatCheck`, and `menu_rotControls` restore the adjacent
consumers. Keyboard state is a portable callback; the Win32 implementation
publishes SDL-compatible USB scancodes using native key polling, so no SDL
dependency enters game code. MMV trigger remapping and every decoded
dispatcher branch are also live.

## Milestones

1. **Evidence inventory and export**
   - Exact EXE/PDB pairing check.
   - Core module, source, function, local, and line inventory.
   - Ghidra export into original source units.
2. **Portable foundations**
   - Fixed-width primitive types, math, containers, memory, file I/O, timing.
   - Rebuild 32-bit layouts explicitly; never copy pointer-bearing x64 layouts.
3. **PC integration milestones**
   - Incrementally prove asset loading, input semantics, camera, scene update,
     and caller-owned platform seams in the dependency-light PC build.
   - Treat every playable build as a regression gate, never as a reduction
     of the final scope.
4. **Complete PC reconstruction**
   - Finish player, collision, animation, model/skeleton, rendering, audio,
     AI, combat, force powers, menus, saving, multiplayer, and all remaining
     content and platform behavior.
   - Validate full installed-game behavior and content on PC.
5. **Complete original-Xbox port**
   - Begin only after full PC functionality is demonstrated and the user
     explicitly approves the Xbox phase.
   - Implement controller, timing, audio, storage, and NV2A adapters with nxdk.
   - Validate the complete game on original-Xbox-compatible hardware/runtime.
6. **Native mod support and conversion**
   - Integrate the existing proxy behavior as normal engine code.
   - Convert the final installed PC asset/configuration state into a separate
     Xbox-ready output tree.

## Dormant Saber Trail Module

All ten PDB procedures from `sabre.c` are restored. Direct-call scanning over
the complete executable `.text` section finds no callers outside the module;
the matched remaster therefore retains this legacy trail pool as dormant code.
It is intentionally not inserted into the live `jedi_HandleSabre` draw path.

The exact PDB layouts are retained for `POLY_G4`, `subSabreEdge`, `SabreEdge`,
and `Sabre`. Machine-code field accesses establish an important naming quirk:
the procedure family uses `Sabre.tail` at offset `0x36d2` as the active edge
count, while the PDB-named `length` field at `0x36d4` is only cleared by
`sabre_gCreateSabre`. `prim_gRendSabre` follows the same offsets and restores
the retail brightness/head decay traversal. The PDB array contains two Sabres,
although the exact creation guard permits an index of two; that unreachable
retail overrun is documented rather than hidden behind a new fallback.

The real-asset differential test copies the five self-contained interpolation
procedures directly from the installed `game.exe` into executable memory. It
compares thousands of randomized scalar cases and hundreds of complete data
structures byte-for-byte against the reconstruction. The live FED saber smoke
continues to validate the separate remaster blade owner after this dormant
module was restored.

## Legacy Primitive Module

All eleven PDB procedures from `prim.c` are restored with the exact 4,628-byte
`primDrawingSurface`, draw/display environments, 40-byte `POLY_FT4`, texture
window records, score globals, and the executable's initialized 244-byte TIM
payload. `prim_gSetBkColor` writes the two PDB-owned draw environments at
offsets `25..27`; the earlier private color copies were not canonical and have
been removed.

The matched executable leaves its PSX compatibility helpers as bare returns.
That detail affects generated code: `AddBlur` receives its texture-page value
from the caller's surviving AX value, and the texture-window/translucency
procedures have no memory effect. Direct-call scanning finds no callers for the
legacy blur, quick-draw, score-digit initialization, score-number, callback,
texture-window, or translucency procedures. `initscoredigits` therefore retains
the installed behavior: its absolute developer path fails before the dormant
tail consumes a `TIM_IMAGE` that the no-op `OpenTIM` and `ReadTIM` never fill.
No parser or synthetic success path is supplied.

The `jpb_prim_retail_differential` test maps the shipped PE into executable
memory. It compares 256 randomized complete blur buffers, 64 randomized full
draw-surface mutations, quick-surface pointer ownership, and every non-pointer
byte of emitted score quads against the retail machine code. Ordering-table
tags are checked independently against each run's required low-24-bit pointer
chain.

## Legacy Controller Text

The final `player.c` procedure, `player_ControllerDump`, is restored from RVA
`0xE6D90`. When screenshots are not being captured it takes the last sixteen
bytes of player zero's `PreMotion`, capitalizes the five lowercase controller
tokens, removes `L`, `M`, `R`, and `S`, and writes that result plus the current
motion name through the exact legacy text path. The executable performs no
null check before dereferencing the current motion, and the reconstruction does
not add one.

That dependency required restoring `Text_gWrite`, `Text_gWriteSub`, and
`winDrawTexture`. Their exact initialized `asciiRemap[256]`, `SmallFont[102]`,
`Colors[17]`, and `MonospaceWidth` globals are copied from the matched
executable. The recovered renderer preserves first-line width calculation,
alignment and wrapping flags, signed brightness math, newline behavior,
texture flipping, explicit-size scaling, alpha encodings, and the `0.001`
`frontZ` increment. It also preserves a generated-code quirk in which wrapped
negative X coordinates pass through an unsigned 64-bit-to-float conversion and
become `INT_MIN` at the final screen rectangle.

`jpb_text_legacy_retail_differential` maps the shipped executable, redirects
only its `_DrawTexture` boundary to the test observer, and compares complete
destination/source rectangles, colors, alpha, depth, return widths, and
`frontZ` mutations against the reconstruction. The player regression verifies
the screenshot gate, token filtering, and both emitted diagnostic lines.

## Jonny Win32 Transforms

All four PDB procedures from `win32/jonnywin.c` are restored. Direct executable
disassembly establishes that `SetupWorldmeshMatrix` mutates the supplied
`MATRIX` in place: every basis float is multiplied by the shared `256.0f`
constant, while each signed integer translation is converted to float, scaled,
and truncated back to an integer. The float-vector transform retains retail's
`count != 0` loop condition, and focused tests cover packed vectors, float
vectors, transform setup, and world-mesh scaling.

## DirectDraw Error Reporting

Both procedures from `d3derr.cpp` are restored from the shipped executable.
The PDB-owned `alldderrs` global retains its exact 199-record `_d3derr`
layout: 198 error/message pairs followed by the zero/null sentinel. Both
procedures independently perform the original linear scan and unknown values
use `?Noerror?`. The diagnostic path ignores its legacy message argument,
masks the printed error to 15 bits, formats `%s(%d): err %d - %s\n`, and sends
the result to `OutputDebugStringA`. The focused regression resolves every
table entry and checks both sentinel and unknown-code behavior.

## Slot Sampling And Animation Compression

Both `slots.c` procedures now preserve the matched 704-byte `cubeStack`
layout, the six-frame `libpartanimtick` cadence, target/tracker resets, bounds
tracking, and the exact rectangular accumulation written into `animTrans`.
The `slot_levelstart` procedure deliberately retains its undefined integer
return after resetting the two PDB-owned counters.

All four `compress.c` procedures are restored across the 0-, 4-, 6-, 7-,
9-, and 12-bit packed vector forms, optional one- or two-byte pad payloads,
raw translation frames, and 12-bit rotational frame addition. The focused
test maps the shipped executable and compares every possible control byte in
both pad-width modes, 2,048 randomized complete frame additions, and raw
translation copies directly against the retail machine code.

## Legacy Color Basic Commands

All eight `colorb.c` procedures are restored with the PDB-owned `cb_list`,
`pen`, and `penColor` globals. The command parser allocates the exact 16-,
24-, and 32-byte intrusive records for line, point, circle, and move entries,
including each argument-count overload and inherited pen color. The shipped
draw procedure remains a bare no-op; clear and shutdown paths remove and free
every queued record. Focused tests cover every successful overload, list
ordering, the no-op draw, and both cleanup entry points.

## Typed Zero-BSS Allocation

Both project procedures in `zerobss.cpp` are restored from the matched PDB and
executable. `ZeroBSS` preserves the unchecked 66-entry variable cache and all
eleven enum-selected allocation classes: byte, short, 32-bit scalar, vector,
pointer, collision record, and complete player record. Requests below two
elements allocate one value, recognized allocations are fully zeroed, cached
pointers ignore later type and size requests, and unknown types remain
uncached. The retail `ZeroBSS_ClearAll` procedure remains its exact bare no-op,
so cached storage intentionally persists for process lifetime.

## Win32 Memory Card

All fourteen `win_memcard.c` procedures are restored from direct executable
disassembly. They retain the fixed `c:\\katavo\\winver\\memcard` path, the
`-1` current-card substitution, one shared 256-byte path buffer, Win32 file
enumeration state, exact load error codes, and save diagnostics. Seven retail
entry points remain bare no-ops. The original null-only search-handle test and
double `fclose` on an opened load path are intentionally preserved rather than
silently repaired; focused tests exercise path selection, missing-file state,
unchanged output pointers, and the inert entry points without writing files.

## Legacy VRAM Palette Utilities

All six `vram.c` procedures now retain the exact 16-entry page and subpage
coordinate tables, unclamped page lookup, clamped subpage lookup, CLUT address
decoding, 5-bit channel maxima, shared fixed-12 normalization scale, and
translucency-bit update. Direct callee review also moved the three-byte
`LoadClut` body from beneath the wrong later PDB marker back to its own entry
in `linkstubs.c`. As in the shipped PC
executable, `StoreImage`, `LoadClut`, and `DrawSync` remain bare compatibility
returns; no invented backing VRAM or palette fallback has been introduced.
The palette routines consequently preserve the retail stack-data behavior
rather than pretending those inert transfers produced valid image data.

## Map Animation

All seven `mapanim.c` procedures are reconstructed with the PDB's exact
2,716-byte `wsl_BT_ANIMMAP`, 2,644-byte `wsl_BT_PARTNODE`, and compact part
entry layouts. The recovered path preserves fixed-12 frame interpolation,
strict end-frame wrapping, the 32-entry child/sibling traversal stack,
orientation matrix construction, parent-relative pivots, the authored Z
offset, enemy activation flags, and all type `0..7` state transitions and
early returns. The three public compatibility entries remain their shipped
bare returns. Focused tests cover interpolation and matrix publication plus
every animation type, delay/state changes, looping, and pause behavior.

## Bucket Archive Registry

All 29 project procedures in `bucket.c` are reconstructed from the matched PDB
and direct executable disassembly. The PDB-sized 4,419-pointer `bucketList` is
mechanically extracted from shipped `game.exe` RVA `0x4A6DB0`, with the exact
main, character, level, train, arena, save, and front archive spans plus its
single circular-search sentinel. `bucketCom[13]` and `bucketModels[25]` retain
their initialized retail values.

The restored code preserves JBUK directory records, RLCP metadata and
checksums, cached-bucket reuse, full and offset run-length decoding, byte-sum
checksums, circular bucket lookup, and the original persistent lookup-string
allocations. Writer/log entry points that ship inert remain inert; no archive,
texture, or filesystem fallback is introduced. Focused tests cover registry
landmarks, wraparound lookup, raw and compressed directory entries, complete
and partial decompression, cached archive access, initialization, and utility
commands.

## Win32 File IO

All eleven `win32/IO.c` procedures are checked instruction by instruction
against shipped RVAs `0x128630..0x12897F`. The reconstruction restores the
exact inert diagnostics, successful-open `updateBucket` call, signed size and
seek conversions, stream ownership, load wrappers, and the three-byte
register-preserving `file_ReadPC` return.

Retail `file_AppendFile` passes the literal mode `"awb"` to its statically
linked CRT. Direct disassembly of `__acrt_stdio_parse_mode<char>` proves that
the second `w` is rejected, `errno` becomes `EINVAL`, and the invalid-parameter
path is entered. No game code installs a replacement invalid-parameter
handler. The reconstruction therefore retains this dormant retail failure;
it does not silently substitute working `"ab"` append behavior. The focused
test uses a test-only returning handler to observe the null-open branch without
terminating its process, then validates the remaining stream, memory, whole-
file, wrapper, and chunk-loading paths.

## Win32 Model Nodes

All six `win32/nodes.c` procedures are checked against the complete shipped
range `0x129030..0x12A6CB`. PDB type recovery replaces every former reserved
range with the exact 136-byte `primRendPacket` and 1,032-byte
`SramModelStack` fields. The renderer now publishes `Mnode.v3CurrentRotation`
from animation or absolute rotation before matrix construction and preserves
retail's mandatory, unchecked scene-physics and detached-effect ownership.
The exact explicit-zero `jon_otagpos` body is restored at RVA `0xB5A60` and
used by the node path.

The PDB declares `gRendPacket[512]`, while both packet writers accept indices
through `0x200`; index 512 therefore writes into the shipped BSS gap beyond
the declared array. That latent off-by-one remains visible rather than being
masked by a 513-record replacement. Focused coverage exercises packed vertex
decoding, shared point-cache ownership, triangle and quad submission, material
color rules, hierarchy transforms, current/absolute rotation publication,
clipping, event/effect masks, linked physics state, and the packet rejection
boundary. Rendering, combat, physics, model, and BMD dependent suites pass.

## Jedi Gameplay Owner

All 30 `jedi.c` procedures are reviewed against the matched PDB, raw Ghidra
output, initialized executable data, and direct shipped-instruction checks.
The completed surface includes progression and award calculation, player
validation, initialization and callbacks, ranged weapon creation, saber
presentation and contact, color selection, secrets and stats, and both-player
combo presentation. The reconstruction preserves retail's branch-specific
saber node fetches and unchecked failure points instead of compressing them
behind a shared fallback.

The audit corrected inverted health, Force, and combo award gates and restored
the exact combo glyph mapping, pivots, scales, spacing, player-two overrides,
and return-register residue. Its coupled `game_checkCompleteAchievements`
review also removed a reconstruction-only call to achievement ID `0` for
levels 11 through 14. Focused tests cover award thresholds and side effects,
combo draw calls and coordinates, saber behavior, player initialization, and
completion-only versus score-bearing achievement routes.

## Game Lifecycle And HUD Owner

All 84 `game.c` procedures are now represented and reviewed against matched
PDB module 0039 and shipped RVAs `0xA5620..0xAB4DB`. The five formerly absent
owners are split into archive members for the complete frame loop, main game
loop, per-level initialization, variable/system initialization, and level-mode
loader. Their recovered paths preserve signed mode and player-count tests,
SDL event ordering, input-clear cadence, the level-eight render exception,
music gates, stage/continue transitions, and the exact integer quit result.

The sweep also restored `clearzerobss` in system reset, the low-byte-only
`menuVars.fcount` clear, signed-byte `GameState` suppression, and the retail
`LevelExit` clear. Difficulty setup now reads the selected level and exposes
invalid state instead of substituting a reconstruction fallback. Callback
consumers use exact `funcArray` slots 6 and 48; the two private callback
mirrors and the prefill of all 50 slots are removed.

A full-owner direct-callee comparison found two older omissions. `_AddBar`
now always calls `_DrawTile2D` with the authored rectangle and forced `0x7f`
alpha instead of disappearing unless a reconstruction-only hook was installed,
and `game_ProcessStatus` again invokes the shipped completion linkstub on the
level-complete path. The remaining structural call differences are proven
compiler inlining, branch factoring, or exact shared helpers. Focused game,
frame, play, runtime-title, AI, physics, collision, combat, pickup, camera,
model/scene, and boss/vehicle matrices pass 19/19 in Release and Debug; the
Release PC executable also builds.

## FX And Particle Owner

All 17 `fx.c` procedures are represented and reviewed against matched PDB
module 0038 and shipped RVAs `0xA2CB0..0xA5612`. The exact PDB particle
launcher, particle, eight-particle block, and list layouts replace the former
cleanup-only byte views. The restored owner includes model-hierarchy glow
traversal, fixed-to-float glow forwarding, two-sided screen sections, seeded
color generation, particle allocation/cleanup, lifetime updates, transformed
trails, color interpolation, and the two shipped bare-return compatibility
procedures.

The audit removes the reconstruction-only glowing-man callback that previously
replaced all retail traversal and restores pointer-only material ownership in
`fx_Init`; texture load failure is no longer hidden by a later texture-field
retry. It also finds an omitted per-segment `SetCameraMatrix` call in
`fx_PlasmaZap` and proves from PDB symbols that the particle launch basis calls
`sin` at RVA `0x206740` and `cos` at RVA `0x216A40`, correcting a plausible but
reversed decompiler interpretation. The retained executable contains no direct
particle-launch or particle-update callers, and its launch body leaves several
list fields unwritten; those dormant facts remain exposed instead of being
filled with inferred behavior. Direct glowing-man and float-conversion
regressions pass in Release and Debug, and the Release PC executable builds.

## Windows Input Translation

The completed `wInput.c` audit recovered the exact `_SDL_GameController *`
owners `gGameControllers[2]` and `sdlPads[5]` plus the initialized nine-byte
`controlLimits` table at RVA `0x4D4A58`: `16, 26, 36, 46, 56, 66, 76, 86,
106`. `ReadJoystickInput` treats option bytes as unchecked indices into this
table, caps the selected byte at `127.0f`, and divides it by `127.0f`. Stick
axes are independently divided by `32767.0f`, preserving the shipped signed
full-scale asymmetry. The XInput port had instead treated the selected bytes
as percentages divided by `100.0f`, then applied XInput's 7,849-unit
left-stick dead-zone. The port now passes raw signed axes through the exact
table-derived threshold path; the default walk/run thresholds are `36/127`
and `106/127`, and no extra dead-zone remains.

The initialized-data pass also restores all three 17-entry SDL button maps.
Xbox uses the executable's generic `0,1,2,3` face order, PS4/PS5 changes map
slot 6 from Back (`4`) to Touchpad (`20`), and Switch swaps both A/B and X/Y.
The two input-type globals and `lastUsedInputType` initialize to `-1`, both
joystick indices initialize to `-1`, `firstRun` initializes to `1`, and the
two mutable threshold temporaries initialize to `0.25f` and `0.98f`, matching
the bytes in the shipped image rather than zero-filled reconstruction state.

Direct executable checks also distinguish P2's controller handle from the
frontend `p2Connected` flag: `P2Disconnected` sets `p2Disconnected` and clears
`gGameControllers[1]` without writing `p2Connected`. Rumble start retains its
signed `controllerIndex < 3` test, while rumble stop retains the unsigned
`controllerIndex < 3` test and therefore rejects negative indices.

The audit also restores `ReadKeyboardInput`, `ReadJoystickInput`, the initialized
`ReadGameInput` function pointer, controller open/detach ownership, the complete
P2 event loop, and start-device selection. The shipped Switch join buttons are
reversed, a Steam Deck guard refuses P1 reuse, and P1 replacement scans all five
slots in ascending order so the highest eligible slot wins. The DirectInput
compatibility block retains its zero-initialized interface GUID and exact COM
vtable operations. Direct instructions prove that three non-void compatibility
exports return without defining a value and `InitKeyboardInput` dereferences an
uninitialized local device; those dormant defects remain visible rather than
being replaced with substitute behavior. The coverage ledger records 26/26
procedures. Focused input and Win32/XInput ownership tests pass in Release and
Debug, and the Release PC executable builds.

## Texture Cache Ownership

Direct checks of `_LoadTexture` (`0x1262E0`), `_TryLoadTexture`
(`0x126770`), `_FreeTexture` (`0x125F50`), and `_ClearTextureCache`
(`0x125B70`) establish two independent caches. `_LoadTexture` owns an
unordered full-path cache; `_TryLoadTexture` owns an ordered base-name cache
cleared only by `_ClearTextureCache`. `_FreeTexture` removes the full-path
entry only when the material still owns a texture, releases that texture, and
leaves the material type and filename untouched.

The shipped failure behavior is deliberately retained. A failed primary file
load retries exact path `../../../res/default\\o_default.tga`; if both loads
fail, the material is returned to the pool but its pointer remains in the
full-path cache. A later request therefore returns that stale, texture-less
material. Likewise, `_TryLoadTexture` returns any non-null cached material
without rechecking its texture pointer. Focused regressions lock both quirks,
the separate cache lifetimes, arena-to-FED path rewriting, and case-sensitive
filename conversion/classification.

## Direct3D Enumeration

All ten project-owned `d3denum.cpp` procedures are reconstructed from the
matched PDB layouts and direct executable instructions. The exact 1,256-byte
device record, 20-record global list, DXGI adapter enumeration, default-adapter
selection, DirectDraw driver walk, D3D7 device filtering, mode sorting, and
default-device priority are restored.

The retail defects remain observable: the public confirmation callback is
ignored, enumeration return values are discarded, global counts are not reset,
hardware capability is stored as raw bit `0x80000`, and allocations made for
rejected devices are leaked. Final enumeration status is selected solely from
the enumerated and accepted counters (`0x81000002`, `0x81000003`, or `S_OK`).
Focused tests cover deterministic mode/device branches and the host's live DXGI
and DirectDraw paths in Debug and Release; the full PC executable links in both
configurations.

## D3D Framework and Texture Backend

The first dependency-closed D3D12 texture slice is reconstructed from the
matched PDB layouts and direct executable instructions. `CD3DFramework12` now
uses the exact 640-byte release layout for the restored fields, including the
two-element fence and allocator arrays, frame index, descriptor owner, and
command-list flag. Its constructor preserves the executable's exact write
footprint rather than zeroing fields the retail constructor leaves untouched.
Descriptor allocation, recycling, index calculation, command-list close, and
texture fence waits are restored with the shipped LIFO and MSVC vector-growth
behavior.

The 416-byte concrete `Texture` owner now restores both constructor paths,
intrusive-list ownership, destruction, `Restore`, `UpdateTexture`, empty
resource creation, SRV publication, all legacy texture factories, global
texture deletion, bitmap and raw-RGBA surface copying, image decoding, and the direct `PHL::Texture2D`
factory. The factory path
preserves retail's ignored empty-texture dimensions and private base-type
write; global deletion preserves the shipped explicit release followed by
`ComPtr::Reset` before scalar deletion. The former production texture-factory
test override has been removed. The seven- and ten-argument
`UpdateSubresources` procedures retain their exact footprint allocation,
row-copy, and buffer/texture copy branches.

`CopyRGBADataToSurface` retains the shipped 256x256 RGBA8 destination,
256-byte-aligned source footprint, unchecked default-resource creation, and
transient upload-resource lifetime. Its focused GPU test intercepts only the
copy call to hold a test-side upload reference through execution; production
continues to release that upload immediately after command recording, exposing
the retail lifetime defect instead of silently extending it.

`CopyBitmapToSurface` retains the shipped oversized bitmap buffer, separate
four-byte-per-pixel staging buffer, upload prefill, and subsequent
`UpdateSubresources` overwrite from the original bitmap buffer. It also keeps
the executable's bit-count row pitch, `m_dwPitch` slice pitch, unchecked bitmap
query, ignored map/update results after successful upload allocation, and the
same transient upload lifetime. A real GPU round trip verifies the resulting
first-row copy and zero-filled second row for a 32-bpp two-row bitmap.

The image-loading chain now uses only the shipped SDL2/SDL2_image decoder
boundary. `LoadImageData`, `LoadPNGFile`, and `LoadTargaFile` preserve their
distinct format conversion, locking, alpha-state, pitch, vertical-flip,
diagnostic, and cleanup behavior. Real-asset tests decode the installed
`a_blob.png` and `a_blob.tga` and compare every flipped row. The dynamic Windows
binding resolves only the mandatory canonical DLL/export names and fails fast
when that import surface is unavailable; it does not provide an alternate
decoder.

`CreateSRVHeap` restores the exact command-list close/execute/wait/reset cycle,
COPY_DEST RGBA8 resource, `/res/` debug-name trimming, shared descriptor
allocation, bitmap-byte upload, SRV creation, and shipped lack of a final
shader-resource transition. `CreateTextureFromFile` restores its pre-load
`PAIN`/`PAINTEX` releases, private type and desired-format writes, and the
canonical 256x256 two-color DIB fallback used after load failure. Focused GPU
readback verifies uploaded bitmap rows and the fallback's exact pixel pattern.

The shared `d3dframe` module is complete at 33/33 project procedures. The
legacy 96-byte `CD3DFramework7` owner restores DirectDraw creation and
cooperative flags, fullscreen and windowed surface chains, clipper ownership,
hardware/system-memory selection, Z-format enumeration and attachment,
viewport setup, presentation, repaint, restoration, initialization cleanup,
and exact HRESULT mappings. The 640-byte `CD3DFramework12` owner restores its
window movement, swap-chain presentation, SDL texture/UI bridge, RTV and D32
depth creation, resize lifecycle, and exact selective teardown behavior.
Focused ABI-level DirectDraw probes and real hidden-window D3D12 resources
cover these paths without substituting host-specific fallback behavior.

Focused Debug and Release tests use real D3D12 devices, queues, descriptor
heaps, command lists, fences, upload/default/readback resources, and pixel
round trips. They verify constructor bytes, descriptor exhaustion and reuse,
state transitions, lazy upload allocation, factory-created resource metadata,
SRV indexing, destructor recycling, legacy factory field writes, intrusive
list teardown, and the exact resource-release call count. `d3dtextr` is
complete at 29/29 project procedures and `d3dframe` is complete at 33/33;
both focused suites and the full PC executable build in Debug and Release.

## Ghidra export

`ghidra_scripts/ExportReconstruction.java` expects an analyzed program whose
image base matches the reference executable. Invoke it with:

```text
<output-directory> <absolute-path-to-inventory/function_map.tsv>
```

For example, with Ghidra's headless analyzer:

```powershell
analyzeHeadless <project-dir> <project-name> `
  -process game.exe `
  -scriptPath .\ghidra_scripts `
  -postScript ExportReconstruction.java `
  .\decompiler-export `
  .\inventory\function_map.tsv
```

The script does not claim that Ghidra output is compilable or original source.
It records decompiler failures and writes recovered C-like bodies beneath the
provided output directory for review and staged integration.
