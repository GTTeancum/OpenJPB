# Jedi Power Battles reconstruction

This repository reconstructs the portable game code from the 2024 Windows
release of *Star Wars: Episode I: Jedi Power Battles*. Full PC fidelity is
the only active target. An original-Xbox/nxdk port is a separately gated
later phase that cannot begin until the user approves full PC functionality.

The reference `game.exe` and `game.pdb` are inputs only. They are not copied
into this repository.

The current scope is the complete PC game. `jpb_pc_game` is the
dependency-light native PC game executable and the integration point for the
ongoing full-game reconstruction.

## Current milestone

Milestone 0 established the reproducible evidence inventory and complete raw
Ghidra export. Milestone 1 is now converting that evidence into reviewed,
portable foundations:

1. Recover named data symbols and exact game-owned type layouts.
2. Review decompiler output against machine code.
3. Preserve reference quirks with behavioral tests.
4. Keep native-pointer and fixed-width layouts valid for the 32-bit Xbox.

The fully reviewed modules are `list.c`, `timer.c`, `alloc.c`, `memory.c`,
`fmath.c`, `flex.c`, `vectors.c`, the original 11-procedure `IO.c` boundary,
and all 25 procedures in `filesys.c`. All 49 PDB-named transform procedures
and all 32 companion fixed-point/vector procedures are readable source,
including packed ten-bit and strided batches, integer and float matrix paths,
camera-space conversion, projection, cross/dot products, and packed
screen-coordinate output.
Archive loading now includes exact nested actor, AI, enemy, animation, library,
Jonny-map, effect, and resident-sprite behavior behind portable hooks. See
[docs/MILESTONE_1.md](docs/MILESTONE_1.md) and the generated
[foundation queue](inventory/FOUNDATIONS.md).

Run the inventory generator from PowerShell:

```powershell
python tools/pdb_inventory.py `
  --exe "C:\Games\Star Wars Jedi Power Battles\game.exe" `
  --pdb "C:\Games\Star Wars Jedi Power Battles\game.pdb" `
  --pdbutil "C:\Program Files\LLVM\bin\llvm-pdbutil.exe"
```

Generated evidence is written to `inventory/`; module source shells are
written to `src/reconstructed/`. Reviewed source is preserved when the
inventory is regenerated.

Regenerate the foundation queue after updating its status:

```powershell
python tools/foundation_inventory.py
```

The PC build is the current priority. Its reviewed core depends only on the C
and C++ runtimes plus the standard math library. The PC level-import adapter
uses the executable-matched ufbx 0.6.1 source, then flattens each level into a
dependency-free renderer structure; gameplay and renderer code do not include
ufbx or desktop graphics types. Platform services remain behind narrow
interfaces so a later platform port does not inherit desktop dependencies.

Build and test it independently of nxdk:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build -C Release --output-on-failure
```

Owners of the original PC data can enable the optional real-asset gate. This
registers every `res/level/W3D/*.j3d` archive, `res/level/jpx/**/*.jpx` world
mesh, and `res/animation/*.cad` animation archive as an independent CTest and
does not copy game data into the repository:

```powershell
cmake -S . -B build `
  -DJPB_GAME_DATA_DIR="C:\Games\Star Wars Jedi Power Battles" `
  -DJPB_UFBX_SOURCE_DIR="C:\path\to\ufbx-0.6.1"
cmake --build build --config Release
ctest --test-dir build -C Release -L real_assets --output-on-failure
```

To inspect one archive directly:

```powershell
build\Release\jpb_asset_probe.exe `
  "C:\Games\Star Wars Jedi Power Battles\res\level\W3D\council.j3d"
```

Pass `--placements` to list authored enemy records, including activation and
deactivation ranges, `--ai <index>` to dump
one relocated BAP tree with its exact node links/opcodes and resolved attack
arguments, `--opcode <value>` to locate an opcode and print up to four
resolved operands across every AI record, or `--opcode-summary` to count all
decoded opcodes in one archive:

```powershell
build\Release\jpb_asset_probe.exe `
  "C:\Games\Star Wars Jedi Power Battles\res\level\W3D\fed.j3d" `
  --opcode 0x302
```

The companion `jpb_jpx_probe` loads and relocates the collision/legacy
preprocessed world mesh through renderer-neutral hooks without loading FBX or
a desktop graphics API.
`jpb_jpx_preview` decodes the bounded 16-byte JPX vertex records and renders
the triangle strips as a height-colored wireframe binary PPM using only
standard C. Recognized level paths apply the exact PDB `level_offset` and
`level_scale` tables, including the FBX x/z/y-to-game-axis conversion. Pass
`perspective` to use the recovered game projection with a portable inspection
camera; omit it for the top-down format view:

```powershell
build\Release\jpb_jpx_preview.exe `
  "C:\Games\Star Wars Jedi Power Battles\res\level\jpx\STREETS\STREETS.jpx" `
  "build\streets_preview.ppm" perspective
```

`jpb_cad_probe` validates the zero-copy CAD layout, sequence table, encoded
data region, and exact 100-byte `Motion` table:

```powershell
build\Release\jpb_cad_probe.exe `
  "C:\Games\Star Wars Jedi Power Battles\res\animation\obi_wan.cad"
```

`jpb_animation_probe` additionally loads the original three Huffman companion
files and accumulates every authored frame through the bounded
`anim_GetAnimFrame` path. The loader and validator use only the existing
standard-C file seam:

```powershell
build\Release\jpb_animation_probe.exe `
  "C:\Games\Star Wars Jedi Power Battles\res\animation\obi_wan.cad" `
  "C:\Games\Star Wars Jedi Power Battles\res\animation\huffman.tab" `
  "C:\Games\Star Wars Jedi Power Battles\res\animation\huffman.val" `
  "C:\Games\Star Wars Jedi Power Battles\res\animation\huffman.opt"
```

The exact pointer-free `Camera` and `sceneGeometryEnv` layouts, camera slide
interpolation, view-vector conversion, and world-to-screen matrix construction
are now reviewed. `jpb_ProjectCameraToViewport` carries that gameplay state
through the recovered projection without a graphics API. The portable live
scene now combines that path with the recovered pad mask/edge state machine
and a standard-C renderer writing a caller-owned X8R8G8B8 framebuffer. On PC,
the exact FBX level source and posed BMD models are both filled and textured;
JPX remains the collision world and an explicitly labeled visual fallback.
The separate wireframe path remains available for format inspection. World UVs repeat,
model UVs clamp, and a reusable depth surface preserves actor-to-actor
occlusion. Perspective triangles and quads are clipped in camera space at the
matched D3D near plane before division, including interpolated UV and vertex
color at newly introduced edges. BMD hierarchy setup now consumes the decoded
animation frame's root translation exactly as `render_RenderModel` does; it no
longer substitutes or doubles the static BMD root-node offset. Live PC
presentation uses the matched application's 53-degree
vertical field of view and render-target aspect ratio; the exact focal-460
legacy projection remains available to reconstructed callers. For recognized installed levels it also derives and relocates the
matching `res/level/W3D/*.j3d` archive, so the exact `MovePlayer` entry sees
the original `WorldData` and Jonny collision grid rather than a no-contact
substitute. A thin Win32 host supplies only keyboard polling, timing, window
creation, and `StretchDIBits` presentation; there is no SDL or third-party
runtime dependency:

The portable level loader also preserves exact `scene_gInitRoot` camera mode
`0x901`. Its `0x100` bit is the owner of `camera_CameraSlide`; replacing the
mode with absolute-focus-only `0x800` had allowed authored dolly destinations
to advance while leaving the live camera frozen at its opening pose. The
203-frame FED combat gate now verifies that the recovered slide mode remains
active and the moving player stays inside the 960x540 viewport. The movement
lead inside `camera_StuffCamera` now also matches the executable's order and
axes: it normalizes and samples the previous lead before advancing it, using
`(sin(yaw), 0, cos(yaw))`. The matched globals remain at their initialized
`0x800`/`0.5` cadence and the recovered fixed-point/even-angle operations stay
unchanged. A prior portable PC frame-boundary rule carried a completed odd
pitch step into the next even value, but no such store exists between
`camera_SetCameras` and `scene_middleRender` in the executable. That host-only
  compensation has been removed; a focused test now preserves the recovered
  half-rate mask/slide result. A current settled-room comparison places the
  doorway, horizon, and player at the same reference landmarks; moving and
  level-wide camera validation remains open.
The PC boundary no longer retains the scripted FED dolly after the director
releases it. That continuity rule had no executable owner. Ordinary play now
returns directly to recovered `camera_SetCameraPos` and the shipped `.cam`
table: stationary opening-room frames select raw collision dolly 3, while the
420-frame traversal selects dolly 0. Projection-sensitive regression gates
protect those exact transitions while the broader camera survey remains open.
Mini2 now restores the special PDB-named `gJarJarPos`/camera-type-6 ownership
normally published by `ai_Kadu`; until the portable runtime instantiates the
two Kaadu mounts, the live rider position supplies that camera target instead
of the zero-initialized global. A real Mini2 gate verifies a filled world and
visible player through the shipped camera table.

For camera reconstruction work, `--camera-diagnostics` prints the selected
walk polygon, its resolved library record, authored camera byte, current
camera owner/type and view flags, and the live-to-destination angle/focus
transition.
`--camera-dolly N` temporarily applies the original `overRideDolly` field for
side-by-side authored-shot comparisons; neither option changes ordinary play.

For an installed-game run, place `jpb_pc_game.exe` in the game root beside
the `res` directory and launch it directly. A normal launch enters the
original front-end presentation, including the save-aware New Game route,
player count, difficulty, character selection, and the recovered level
selector before loading the chosen gameplay map:

```powershell
& "C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe"
```

The executable resolves its initial FED presentation assets relative to its
own location, independent of the process working directory. Character and
level selection then resolve the matching installed CAD, BMD, combo, JPX,
and optional FBX data before gameplay begins. If the installed
save is missing or rejected, New Game follows the recovered no-save route
instead of displaying the overwrite prompt. Training has a dependency-free
selection presentation and hands the chosen entry to its installed training
map. VS now carries both selected characters into the installed `arena` map,
with independent CAD/BMD/combo ownership, two live player/physics slots, and
a midpoint arena camera while the retail Arena setup needed by the recovered
camera owner remains an explicit reconstruction boundary. The PDB-named
Credits presentation scrolls
  the exact recovered 706-line sequence with its heading styles and rotating
  music selection. Concept Art presents all 42 installed pages with the
  recovered wrapping, pressed-arrow states, counter, and aspect-correct frame.
  Quit is wired to the host, and the retired registration-service entry is
  omitted. For development,
`--quickload <level>` bypasses the presentation and loads any named installed
map directly with the player, for example `--quickload theed`. This is the
deterministic map-inspection path; normal presentation-owned gameplay retains
the reconstructed enemy population. An
explicit world path and the `--cad`,
`--bmd`, `--cmb`, `--enemy-cad`, and `--enemy-bmd` overrides remain available
for development and asset validation. The corresponding
`--player-two-cad`, `--player-two-bmd`, `--player-two-cmb`, and
`--player-two-model` switches provide a deterministic two-player runtime gate.

The PC bindings now follow matched `ReadKeyboardInput` rather than the former
host placeholders. Use arrows or WASD to move; J/K/L perform the south/west/
north attacks; Shift blocks; Space or Enter jumps; H toggles lock-on; T zooms;
and Escape supplies Start/pause. U/I/O/Y retain the executable's dedicated
Force-modified jump/north/south/west chords. XInput uses the left stick or
D-pad to move. The shipped modern gameplay configuration maps A to jump,
B/X/Y to north/west/south attacks, left shoulder to block, left trigger to
modify Force actions, right shoulder to lock on, Back to zoom, and Start to
pause. The alternate classic configuration maps A/X/Y to south/west/north
attacks and B to jump; menus always retain classic A-confirm/B-back semantics.
The recovered per-player Walk and Run percentages control stick direction and
run qualification, while D-pad movement remains digital walking. Keyboard is
P1-only, as in the recovered Windows input owner.
Stick axes retain the original signed 32767 normalization, and movement/facing
uses the recovered camera-relative destination independently for P1 and P2.
After a sustained run, releasing movement or pressing block enters the shipped
Motion 25 run-stop/skid path rather than retaining the looping run vector.
Holding a direction beyond the exact 21-frame threshold also enables the
character-authored south/west/north running attacks (motions 92/93/94); all
nine base-character data sets and all three directions are covered by real-
asset regressions.
Its input has the matched immediate priority over P1 controller input rather
than being merged with it. In two-player play, P2 retains one joined controller;
P1 can use the keyboard and fall back to any other connected controller. This
supports keyboard P1 plus a single-controller P2, while a controller-owned P1
reserves the first pad and assigns the next pad to P2. Rumble follows those
logical-to-physical assignments. The XInput boundary retains the exact
physical user index rather than reinterpreting it as a connected-device
ordinal, so sparse slots such as users 2 and 3 route independently and all
four possible users are silenced during shutdown. Close
the window to quit. Interactive runs now connect authored animation,
combat, projectile, and map-event sound requests to the shipped loose WAV
banks through a 64-voice WinMM host. All 31 PDB-named procedures in original
`win32/sound.c` are reconstructed, with the exact 43 bank descriptors,
requested/level/resident fallback, 100-entry positional-loop owner,
`jar_jar_playable` and Coruscant aliases, seven-file training bank,
quiet/non-spatial prefixes, six looping names, and shipped default audio
options retained without SDL or another third-party audio dependency. The
host resolves only exact members of each loaded bank. The same host plays the
PDB-named `playXA` table's shipped stereo WAV music, including loop,
pause/resume, stop, and volume control. Use `--mute` for an interactive silent
run; all `--headless` validation remains silent by construction. The
interactive host replaces `jpb_pc_game.log` beside the executable on each
launch. It flushes startup, asset resolution, menu transitions, gameplay
handoff, decoded pressed/held controls, analog axes, control and active-motion
transitions, clean shutdown, and unhandled-exception address/RVA records
immediately, so a terminating fault leaves a useful final record. Headless runs continue
to report through their captured standard streams instead of sharing that
file. The
`--headless --headless-move --frames 3 --output frame.ppm` and
`--headless --headless-attack --frames 16 --output frame.ppm` forms are the
automated authored-movement and authored-attack visibility gates. The
`--headless-phase jump 8 --headless-phase none 1 --validate-jump
--frames 10` form is the authored jump/trajectory gate. The
movement-facing collision foundation now also includes
24 reviewed `collisn.c` procedures and the exact native-pointer `Mnode`
layout. Exact player pool storage and its embedded two-channel `Pad` state are
also reconstructed. The live game runtime now calls the PDB-named
`player_gProcessPlayers` at the original post-render boundary. That owner runs
collision first, sets the exact two/20 attacker budgets, handles player map
triggers, routes independent controller or AI channels through `mPlayerRead`,
applies the level and pause gates, and invokes `brain_ControlPlayer` once for
every eligible actor. Its ordinary HUD path is now live as well: exact
`_AddLifeTile`, `_DrawTile`, `_DrawTile2D`, the scaled energy/Force getters,
and the interleaved damage indicator feed dependency-free PC framebuffer
draws through the original camera matrix. Developer diagnostic text remains
an explicit HUD boundary. Runtime startup now passes through exact
`newGameGameInit`, including the executable's named initial Jedi combo arrays,
23 energy/Force resource rows, persistent level/score clears, upgrade and AI
bit clears, and default model pair. The host retains only a full storage clear
needed for safe repeated test initialization; authored defaults stay in the
original `game.c` owner. The PDB-named `pwrup_LoadPoop`, `pwrup_Init`,
`pwrup_LevelStart`, `pwrup_LevelEnd`, `pwrup_JumpCheckPoint`, and 4,144-byte
`pwrup_CheckPowerUps` now load each level's authored 12-byte `.pwr` records,
preserve the two native-pointer `powerPoop` lists and their post-render swap,
run authored emitters, apply every pickup award/checkpoint/artifact branch,
and restore eligible two-player after-life actors. Exact initialized scale,
icon, random-power, display-name, and model-name tables retain their PDB names.
The PC runtime enters this original lifecycle through the dependency-light
resource owner in the matched scene order; its FED smoke proves 55 records are
live, and all 52 installed power-up files are parser regression gates. The
full PDB-named `scene_middleRender` now owns that order directly: camera and
frusta, animation, overlay, world/model submission, sabre, player, power-up,
sprite, enemy, backdrop, physics, and level-specific work. The Win32 host
supplies renderer callbacks at the original submission boundaries and no
longer advances those gameplay systems or timers a second time. Exact
`level_Fed`, `level_Corus`, `level_Palace`, `level_Hangar`,
`level_CountDown`, and `level_Arena` are now direct level-special owners.
Their recovered behavior includes four-corner clamp volumes, Palace's
offscreen-boss rule, Hangar's pilot/timer objective, all eight retail
countdown parameter sets, Arena outcome/refill logic, UV motion, checkpoint
failure, level-exit and secret-unlock publication. Exact
`menu_drawBigNum`/`menu_drawBigNums`, `level_Mini4`, and `standingonit`
restore the adjacent numeric-HUD and platform-contact dependencies. The
removed immediate-mode power-up mesh renderer remains isolated behind the
exact `DrawPowerUp` call surface; the current PC realization is a temporary
world-space glint rather than a claim of mesh-rendering parity.
The controller's exact `atan2f` conversion,
camera-relative fixed angle, `physics_gTurnToFace`/`physics_gTurnToAttack`
selection, and `Motion[2]` path now drive ordinary movement. A second
real-asset gate presses the configured attack input and requires authored
`Motion[15]`.
Stationary input follows the recovered normal,
low-energy, or lock-on idle selection (`Motion[0]`, `[19]`, or `[20]`), and
the delayed `Motion[2]` run-stop path activates `Motion[25]` with the
original counter, movement-clear, and player-flag behavior.
Omnidirectional control now uses the reference wrapped-facing threshold to
select `Motion[26]` or reverse `Motion[8]`, including the exact speed,
freeze-window, tween, facing, and equal-lock stores.
Exact `sceneObject`/`modelObject`/`physicsObject` layouts and 41 reviewed
physics procedures provide the movement side of that connection.
The exact `FVECTOR4`, `CollisionData`, `_collide_info`, and
`_movement_packet` records now name the contact boundary. Original
`CalcRelativePosFromWorld`, `CalcSolidRelativePos`,
`CalcWorldPosFromRelative`, and local `CalcWorldRelativePos` preserve moving
solid rotation, scale, `Mnode.v3RotCenter`, relative facing, and flags; the
bounded movement path applies an already-processed standee's world delta.
Exact `VectorNormalize`, `VectorNormalize2`, `VectorNormalize3`,
`CalcNewBox`, `buildfrustrum`, and `buildplane` now supply the
dependency-free collision-frustum math used by the camera and world-contact
paths. The exact `_collidevars` scratch layout and original local
`planecheck` extend that boundary through signed plane approach, penetration
clamping, and best-contact selection. Original module-local
`sphereAndPoly` and `polycollidecheck` now provide dependency-free swept
sphere contacts against polygon faces, edges, and points, including the
reference contact-priority rules. Original `generalCollide` now feeds
triangle/quad `_solid` vertex, index, and fixed-normal streams into that
kernel without adding a platform dependency. Exact `newclosestPoly` and
`jon_getlibpartfloat` extend the same dependency-free path through player
clip planes, the Uber box, dynamic solids, and compressed fat/thin world-cube
streams while preserving packed normals and exact hit-pointer publication.
The exact `ProcessPhysicsObjects` frame entry now performs frustum setup,
moving-solid construction, processed-guard reset, movement and position
passes, driver synchronization, scene/world publication, matrix-stack
balancing, pause gating, and the terminal Streets teardown in original order.
The live STREETS runtime now enters that scheduler directly. Exact
`twatcameramatrix` converts the current gameplay camera into the coordinate
and axis convention consumed by both collision and clipping frusta, so its
real-asset smoke advances Obi-Wan through collision-aware world movement.
The missing STREETS ending trigger is also restored inside
`CheckCubeBlocking`: its exact STAP ownership, wall-normal, collision-timer,
effect/audio, reset, scene-disable, and `0x13800` countdown behavior now feeds
the scheduler's terminal sequence. Exact `brainutl_ElapsedTime`,
`combo_ResetComboEngine`, `player_ResetJedi`, and the complete 1,574-byte
`physics_ResetJedi` keep that reset path in PDB-named, human-readable code.
The physics reset retains the executable's Level 10 two-player vehicle-ID
`0x4C` userdata exception rather than specializing the implementation to
STREETS.
Exact 100-byte `Motion`, 560-byte `_animFrame`, and 2,496-byte x64
`animObject` records establish the animation side; the reviewed handoff
preserves the original `vel + Charge`, `ChargeAcc`, and axis-swap behavior.
The exact eight-node `anim_AddNextAnimSeq` queue now preserves normal and
target template-context selection, replacement, duplicate suppression,
per-motion tween/speed/lock data, and pool exhaustion without dynamic
allocation.
All 93 installed CAD files pass bounded format/loading and full sequence
decode gates: 7,603 sequences produce 150,272 accumulated authored frames
(7,562 raw and 142,710 compressed). All seven PDB procedures in `unpack.c`
are reviewed: bit-reservoir
refill, direct/cached/tree Huffman lookup, raw and compressed vector decode,
table binding, context initialization, and seek. The companion-table loader
validates the installed table sizes and internal indexes before binding them.
Exact `anim_CreateObject` binds the original sequence and motion tables; all
22 `animutil.c` controls and all six `animctrl.c` wrappers are reviewed. Exact
`anim_ForceNextAnimSeq` now owns forced queue activation from those wrappers
and player reset, including flag propagation, recovery, frame-rate and physics
setup, callbacks, sounds, tween selection, and immediate frame publication.
The assembly-checked original local `anim_GetAnimFrame` publishes
decoded poses, 12-bit joint-angle/
13-bit root-Y delta accumulation, event bytes, exclusive frame endpoints, and
double-buffer state. PDB-local `anim_CreateTweenFrame` now supplies the exact
fixed-point root/joint blend and countdown between authored motions, and the
reviewed `anim_GoNextAnimFrame` publication boundary owns per-object frame
stepping, slack, sequence ends, and `SpeedAcc`. Exact global
`anim_ProcessAnimations` owns the live 20-slot PC pass. Exact local
`anim_SkipToStartFrame` restores pre-roll pose decoding without leaking its
events, and target-relative motions use their secondary depack context. An
instruction-level `anim_GetAnimFrame` audit also restores the first-publication
count to `Motion.cutin` at offset `+0x0A`; the previously used `Motion.disp` at
`+0x0C` is the separate slack/recovery field consumed by `anim_CheckSlack`.
Exact
`anim_GlobalInit` reaches the three companion files through the recovered
32-way resource-path owner and a dependency-light base-path seam. Exact
`anim_SoundStart`, `anim_HandleSound`, `shouldPlayAnimSound`, and
`shouldReplayAnimSound` now preserve both motion-sound channels, delays,
loop-stop state, bank selection, and authored replay exceptions through the
dependency-light sound boundary. Exact
`scene_gSetSceneModelKeyFrame` now publishes each
decoded frame to the actor scene. A renderer-derived portable pose boundary
copies only the original x/y/z joint-angle fields into recovered `Mnode`
trees, including static/virtual-node zeroing and absolute-rotation override
semantics. The exact 144-byte PDB `geomData` BMD record is now reconstructed.
The live PC player and enemy paths now enter the exact PDB-named `model.c`
owner: `model_gInitModelRoot`/`model_MakeNode` allocate their models from the
20-entry `modelSpace` registry and build each contiguous 32-node `Mnode`
hierarchy. Exact registered-name reuse, collision registration, fixed model
scale, five-way `addPtr` geometry-stream relocation, saber substitution to
`transabr.bmp`, the exact `TT_TEXTYPE` values, 800-entry PDB-named
`g_material` pool, `_LoadTexture` cache, and texture-packer leaves are
retained without adding a graphics or asset-library dependency. With the PC
texture hook active, `model_MakeNode` performs the retail replacement of the
`geomData` texture-name union with an `_Material*`; headless inspection keeps
the names when that hook is absent. The bounded BMD view resolves mutated
stream indices through exact `getPtr` and material handles through
`jpb_BmdGetMaterial`. The renderer-derived geometry
view preserves `_RenderNode`'s three packed vertices per `numVerts` unit,
signed-10-bit coordinate decode, signed-16-bit triangle/quad faces, shared
vertex prefix, per-corner normal/color streams, and four UV slots per face.
A dependency-free posed model boundary reproduces the `render_RenderNode`
hierarchy transform order and `_RenderPackets`' exact 3,072-`FVECTOR` scratch
capacity. It also publishes the exact authored node-event flags and promotes
a hot node to scene attack flag `0x10`, which the recovered
`player_DoCollisions` owner consumes and clears. Exact PDB types `pairUV`,
`faceUV`, `_Material`, and
`TEXTURE_SAMPLE_TYPE`, plus `_StartPoly`, `_SetVert`, and `_NoScaleEndPoly`,
define the recovered material input. The filled path perspective-correctly
interpolates UV/color data, bilinearly samples textures owned by the live
`_Material` (with a name resolver retained for inspection paths),
depth-tests model faces, and follows the shipped `PixelShader.hlsl`
texture-times-vertex-color and black-discard behavior. A bounded,
dependency-free TGA decoder covers every installed TGA encoding class, while
the PC runtime maps BMD `.bmp` references to sibling `tga/*.tga` assets during
the exact model materialization step. Pixel-resource creation stays behind a
small platform callback.
The runtime now carries the live `_Material.flags`, `samplerType`, and
`colorOverride` fields through that callback instead of reducing a material
to pixels alone. Exact `SetTextureColorOverride` restores the shipped
`Loadbody.tga`, `boulder.tga`, `ful_body.tga`, `bus.tga`, and `qui_hair.tga`
level exceptions. `_NoScaleEndPoly` evidence establishes all observed flag
modes: zero rejects negative projected winding, one is two-sided, and two is
two-sided with every submitted depth forced to `0.0001f`. The portable BMD
path applies those policies, including `_RenderNode`'s grayscale and
dark-red color-override rules.
The live runtime loads
`obi_wan.cad`, links actor, scene, model, physics, animation, and player
records through the recovered hierarchy, selects its authored walk motion,
executes recovered `CalcMovement` followed by exact `MovePlayer` against the
matching J3D collision archive, and draws all 21 posed and textured Obi-Wan
nodes every frame.
All fifteen PDB-named `brain.c` procedures are now reviewed
(7,081 of 7,081 procedure bytes).
They include effect dispatch, both ring effects, `brain_GroundControl`,
the complete lock-on lifecycle, hang/skid/takeoff/throw callbacks, all three
trajectory producers, and the constant-motion axis swap. They preserve
motion-event sounds, target selection and validation, knockdown recovery,
player/NPC death scheduling, jump/fall selection, fixed-point air-vector
construction, animation-end transitions, timer/flag changes, and loop-sound
shutdown. Exact supporting leaves now include
`brainutl_gGetNearestTarget`, `physics_gGetRange`,
`sprite_AddSpriteEffectAtNode`, `sprite_gUnHideSprite`,
`brainutl_FindLSB_LV`, `brainutl_PlayMotionSound`, and `jedi_GetColour`.
The full 3,954-byte `brain_ControlPlayer` parent now joins those exact
procedures in a separate link-isolated object. Its one portability seam is
an optional provider for the original SDL-only five-key cheat chord. The
exact enemy kill, held-pad, comment-sprite, force activation, and combo-read
paths are now integrated, as is exact `combo_CheckCombo` with its PDB
`Combo` layout and motion-chain behavior. All twelve PDB-named
`braindmg.c` procedures are now reviewed (8,389 of 8,389 procedure bytes),
including the complete 4,255-byte `braindmg_DamageControl`. Projectile
conversion, blocking, difficulty scaling, scoring, achievements, feedback,
hit/death reactions, and hazardous-surface damage now execute through
production code; the last `brain_ControlPlayer` damage test substitute has
been removed. Exact floating-score creation and lifetime callbacks are also
live. `Draw3dText` retains its PDB-facing API while its still-partial
projection/font backend is supplied through a dependency-free PC renderer
hook. The headless gate
also requires an authored
Obi-Wan frame to reach `animObject.pCurrentAnimFrame`,
`sceneObject.pKeyFrameModel`, and the real `obi_wan.bmd` node tree. All 151
installed BMDs have explicit gates; 150 validate structurally, while
`skin2.bmd` is retained as a known shipped truncation whose declared payload
is 720 bytes longer than the file. The exact `gl_RenderNode` packed-8 face
path is also supported alongside `_RenderNode`'s signed-16 path: 145 archives
satisfy a complete matched renderer layout at every node. Four `gate*.bmd`
variants have packed indices but only 8 UV bytes per face where
`gl_RenderNode` advances 32, and `weasel.bmd` has one out-of-range face node;
those explicit unsupported gates keep structural loading separate from
renderer compatibility. The recovered special direction handoff accepts
only original motions 2 and 60, applies the exact lock-15 store, and returns
through the reviewed idle/run-stop selection. The attack tail and alternate
jump launch are now source blocks inside the complete reviewed
`brain_ControlPlayer` parent. Exact animation pre-roll decoding is restored;
the remaining renderer parity work remains explicitly bounded.

The normal one-player field now loads the shipped 8,192-byte
`CAMERAS/<level>.cam` image into the exact `WorldData.aDolly`/`aBkDolly`
arrays. It selects the dolly through exact `intersec_FindWalkHeight`, uses
the walk polygon's authored camera index, and applies the recovered
`camera_SetCameraPos` transition visibility plus `camera_StuffCamera` axis,
offset, and slack clamps before the exact
`camera_Camera2ViewVector` scene-matrix path. JPX world and BMD actors now
share depth normally; the dependency-light JPX segment clip remains only as
a fallback for scenes without usable authored camera data.
The FED spawn diagnostic additionally proves that collision cube 38098 has a
single applicable library polygon (`0x531c3562/0x03e394c4`), whose authored
camera byte is 3. This distinguishes an intentional dolly-3 selection from a
collision traversal or polygon-overlap error while view composition continues
to be checked separately.

World materials are submitted in the live executable's three recovered
opaque, transparent, and glass passes. The complete 26-entry `levelTextures`
database and exact PDB procedures `isTextureTransparent` and
`isGlassTexture` now drive the dependency-light renderer. A labeled bridge
matches collision-resolved JPX 8.3 IDs to those FBX-era names; unidentified
legacy mirrors alone fall back to `MATHEAD.listtype`. Transparent strips use
the shipped shader's all-zero discard, 0.1 sampled-alpha cutoff, minimum
vertex RGB, interpolated vertex alpha, and source-alpha blending. Exact
`glassTextures` members use the third pass with depth writes disabled.
Dynamic and special JPX spatial metadata remain opaque until their layouts
are supported by executable or data evidence; their bounded strip vertex
payloads are still decoded.

The live PC gameplay field now calls the recovered production
`brain_ControlPlayer` directly rather than its former bounded movement
helper. The real-asset smoke uses the Federation Battleship level because
the executable intentionally short-circuits this controller when
`GameStruct.CurrentLevel == 8` (the STREETS special case). On FED, held
input selects authored `Motion[2]`, decodes the Obi-Wan pose, and produces
collision-aware world movement through `ProcessPhysicsObjects`.
The PC host exposes the exact keyboard/XInput translation through a pure,
directly tested seam. FED real-asset gates require all three Obi-Wan attacks
to enter authored motions 107/105/106 (`jd_hack_base`, `jd_slash_r_base`, and
`jd_hack_h_base`) and require raw block bit `0x4` to enter Motion 15,
`jd_block_active_core`, while the textured model remains visible and posed.
The battle droid now receives the original `battle_d.01` WAI payload chosen
by its authored AI level, the exact `aiData` accessors, and its generic
`ai_Main` callback-table ownership before entering the common actor
controller.
The adjacent enemy-script foundation now also uses the exact PDB
`UDATA`, `BAP_AINODE`, `wsl_BAP_WAYPOINT`, and `kfNode` layouts rather than
anonymous byte offsets. Exact AI-node traversal and mode-stack procedures,
script arithmetic/comparison helpers, timed flag restoration, waypoint
bounds, near/far player selection, the move/range/scan BAP evaluators,
`ai_WalkWayPoints`, `physics_FindNearestEnemy`, post-frame publication, enemy
activation, map-trigger activation, placement-pointer adjustment, and the
Shaolin pool/list lifecycle are reviewed and unit-tested. The complete seven-function,
2,404-byte `shaolin.c` module now preserves the attack coordinator, attacker
budgeting, player-relative formation points, attack-choice deduplication,
combo and delay selection, target switching, and motion scheduling. Its exact
AI-side owners `ai_HthAttack`, `ai_RangedAttack`, `ai_SeqAttack`,
`ai_SetTarget`, `ai_WalkToPoint`, and `ai_WalktoPlayer` are also recovered.
Exact `_addEnemy` and
`_checkForNewEnemies` now preserve the 20-record pool, authored AI selection,
active-range cube, level-15 placement override, failure rollback, and
intrusive-list publication. The 1,114-byte `loader_CreateEnemy` model and
animation construction remains an explicit boundary behind one dependency-
free `jpb_LoaderSetEnemyCreateProvider` seam. The live PC runtime now supplies
that provider with its portable multi-class BMD/CAD/WAI actor owner, so
supported actors enter through exact `_addEnemy` rather than a manual
pool/list insertion; this does not claim the provider is the recovered
original loader body. Loading those assets does not manufacture an enemy;
exact `enemy_HandleEnemies`/`_checkForNewEnemies` performs the first and later
placement activations from authored flags and ranges. It also does not assign a
convenient combat target: the valid inactive player-1 slot remains the player's
initial target until recovered gameplay selects a live opponent. The host's
descriptive primary-enemy view follows an already-selected live target but never
writes that selection, and its damage/reaction counters aggregate observations
from all active enemies rather than whichever actor happens to be published as
primary. Direct tables in the matched executable map FED
`baron.baf` to model 17 / `battle_d.bmd` / `battle_d.cad`, `21b.baf` to
model 15 / `pilot.bmd` / `pilot_d.cad`, and `baronsec.baf` to model 62 /
`security.bmd` / `battle_d.cad`. The same evidence connects `r3po.baf` to
model 12 / `protocol.bmd` / `droid.cad` and `destroyr.baf` to model 26 /
`destroye.bmd` / `destroye.cad`, and `hovdroid.baf` to model 30 /
`loader.bmd` / `loader.cad`. The remaining table records connect `drdfitr` to
model 47 `droid_f`/`droid_f`, `pwrdrink` to model 72 `beacon`/`beacon`,
`pwrserv1` to model 86 `fed_door`/`fedship`, `reeyees` to model 87
`piston`/`fedship`, and `twilek1`/`twilek2` to models 94/95
`lift_1`/`lift_2` with `fedship` animation. The complete 3,822-byte PDB-named
`ai_InitPlayer` is now the live PC enemy owner: all 62 model-ID specializations,
the default path, 21 executable-byte-verified collision profiles, asymmetric
scales, dimensions, movement constants, callback indices, and model/player/
physics flags are reviewed. Its exact fallback `combos` block and empty
`ai_InitModelData` companion are present as named source. The same byte audit
corrected the previously misidentified global `maDesert_BNodeSizes` and
`maWormNodeSizes` records. Models 26 and 30 reach PDB-named
`ai_Destroyer`/`ai_LoaderDroid`; the Loader Droid's exact `ai_ShowFlags`
dependency and the matched release build's inert `debug_printf` are also
recovered. Settings-selected slots 36, 39, 41, 43, 45, and 47 now reach the
exact PDB-named `ai_Blades`, `ai_Worm`, `ai_Krakis`, `ai_Mtt`, `ai_AAT`, and
`ai_Deadly` bodies. These restore blade rotation spin-up, collision-sphere
diagnostics, the MTT's 512-unit two-player damage volume, AAT turret tracking
and projectile cadence, and the deadly-force flag. The matched
`debug_drawsphere` call surface publishes its exact arguments through a
dependency-free renderer hook while retaining inert release behavior when no
hook is installed. The exact table stores also connect `maul_PushCallBack`,
`maul_RingCallBack`, and `maul_ZapCallBack` at slots 27 through 29, plus
`ai_Thug`, `ai_Maul`, and `ai_JarJar` at slots 35, 37, and 38. Those bodies
restore Maul's projectile and zap windows, lock/engagement state, Thug shield
sprite ownership, and Jar Jar's camera/level-state handoff. Exact
`centreturret` supplies the vehicle turret's signed-12-bit return-to-center
step. Model 47 reaches the reviewed PDB-named `ai_StarFighter`.
Its exact heap-backed `Projectile` layout, allocator/free leaves, sound-table
initialization, sprite-owned 3D beam construction, firing setup, and authored
twin-shot emission are now connected. Exact `coll_CheckProjectileCollision` supplies
the authored collision-node radius rules, damage-state reflection gate,
saber and Force ricochets, hit location/motion publication, and ricochet
sound. The complete PDB-named `bullet_CallBack` now performs ballistic and
homing steering, exact stored-speed movement, both sprite update modes,
lifetime/impact/trail effects, termination and bounce audio, the original
masked nearby-player scan, piercing/persistent/reflected transitions, authored
level-specific terminal hits, and the type-6 map explosion/camera-shake tail.
Its special arc path uses exact `fx_PlasmaZap` and the matched 156-byte
`_plasma_zapvars` state; exact `bullet_Explosion` owns radial hit publication.
The fully reviewed 15-procedure `intersec.c` module now supplies exact
map/dynamic-solid raycasts and movement. All eight `bullet.c` procedures are
reviewed, including the complete 2,178-byte callback.
The provider resolves their level-local
actor slots from the authored J3D actor-name chunk, caches immutable assets
per class, and gives each in-range supported placement independent scene,
model-node, physics, animation, player, and enemy records in original pool
slots 2 through 19. WAI handles use that level-local actor slot, matching the
exact loader call; the model ID remains the player/model identity.
The frame owner controls, decodes, renders with each actor's own class
assets, collides, despawns, and reuses those records independently. The FED
headless gate loads all 12 table-mapped classes and discovers 11 with authored
enemy placements; `twilek2` exists in the actor table and assets but has no
FED placement record. Across nine frames the gate proves an 18-actor peak,
30 authored spawns, seven classes simultaneous, and all 11 placement-backed
classes activated, posed, and submitted. Collision-enabled immutable BMD
views cross one validated geometry-stream resolver at the still-bounded
`loader_CreateModel` relocation seam, falling back to the exact `getPtr`
registry for originally relocated models.
The PC field also restores the valid inactive player-1 slot assumed by the
exact AI selectors. The PDB-named `enemy_ParseOpcodes` call surface now
executes the exact node/branch traversal and all 35 executable top-level
opcode values found across all 27 shipped J3D archives. The archive audit
covers 42,581 decoded nodes (35,335 after excluding structural opcodes zero
and one). Recovered paths include scan targeting, counter/random/cyclic branch
selection, movement speed/mode changes, camera-dolly and letterbox control,
global flags, authored motion 84's extra-character gate, combat, teleport
setup, tank/STAP entry through `0x607`, and linked or live-player placement
through `0x60f`. All 22 shipped `0x606` subcommand IDs and all 530 authored
nodes now enter recovered routes. Those routes include power-up rate/control,
effects and sound, streaming-music selection, destructible-map events, live
player replacement, and special-menu messages in addition to the earlier
gameplay controls. Command 11 now owns the exact `level_SparkRoom` five-arc
hazard, including its PDB-named `zapcheck`, `vecpointlinesquared`, and
`PlotZap` dependencies. Command 18 and status-4/5 `0x60f` no longer fall out
of the authored tree for a dead player: exact PDB-named
`player_RefreshPlayer` restores start/checkpoint/placement position, facing,
energy/Force/score state, shadow ownership, animation queue state, and player
runtime fields before the matched transition resumes. Audio playback and menu
realization cross narrow dependency-free host hooks while their recovered
game-state behavior remains in the original modules. A deliberately prefixed
diagnostic companion reports unknown data and bounds malformed cycles without
changing the matched valid-data owner. Exact
`ai_DefendCheck`, `bapenemy_preFrame`, `bapenemy_postFrame`, `_deleteEnemy`,
`player_FreePlayer`, and `obj_gClearObject` now preserve the normal defend
gate, energy/location transfer, placement links, animation-loop shutdown,
player-target repair, and scene/object cleanup. The dependency-light PC loop
enters those leaves through `jpb_enemy_ProcessActiveFrame`, a deliberately
prefixed reconstruction of the normal active-enemy pass. It owns placement
activation, double-buffered enemy lists, authored BAP dispatch, Shaolin
resolution, range retention, and despawn instead of calling the BAP parser as
a detached leaf. Its recovered frame preamble now includes the exact timer,
AI-flag, tank-countdown, score/achievement, next-level, point-sprite, debug
selection, and level-13 player-count state. The level-6 enemy `0x75` range
override and level-7 enemy `0x3a` vertical clamp are also exact. Exact
`enemy_ResetEnemies` and `enemy_SetTeleport` complete active-list teardown,
placement/global-bit reset, and teleport-state publication. The full
7,884-byte `enemy_ParseOpcodes` call surface is now reviewed across every
matched dispatch branch. The 1,929-byte `enemy_HandleEnemies` call surface now owns the reviewed normal
frame and its exact `DebugLevel == 3` branch. Exact 947-byte `enemy_Radar`
draws the scaled background, centered player marker, camera-relative enemy
markers, and original red/green owner colors through `_DrawTexture`; the
dependency-light PC runtime realizes those solid screen rectangles after the
world and actor passes without adding a graphics framework.
All 48 PDB-listed `enemy.c` procedures now have source bodies. The final live
gap, `enemy_CheckTeleport`, applies deferred offsets to nearby enemies and the
correct active player set, retains the level-8/9 coordinate exceptions and
camera-mode selection, and is called by the portable PC frame owner. Exact
`aisub_findNearestWaypnt`, positive enemy-point accounting through the original
80x3 `maModelID` table, and the state-changing `console_EnemyCommand` routes
are also restored. Its camera refresh now calls the recovered downstream
`camera_SetCameraPos` owner directly; console text presentation remains a
separate explicit boundary.
All 23 PDB-listed `camera.c` procedures now have source bodies. The 2,093-byte
`camera_SetCameraPos`, 832-byte `camera_SetCameras`, and 2,785-byte local
`camera_StuffCamera` restore collision-selected authored dollies, one- and
two-player focus modes, follow/slack/uber clamps, candidate visibility
rejection, focused and Streets/JarJar modes, screen shake, and camera-facing
publication. The portable PC frame calls this owner directly instead of
maintaining a parallel host-side authored-camera builder.
The exact 50-entry PDB-named player callback table is also live: all 49
nonzero retail stores now point to their PDB-named source bodies, while slot
zero remains intentionally null. Authored Motion callback indices transfer
into the player record during animation activation. The complete Force
callback family now includes absorb/reflect, attack, attack-spin, cloak,
flame, healing, mesmerize, push, ranged, reflect-sphere, ring, sabre spin,
sabre toss/yoyo, shield, star, grenade/toss, and zap behavior. Reflect-sphere
emits its authored sixteen gradient-cylinder bands through a dependency-light
renderer seam. Exact `ai_FireWeapon` and `ai_Throw` restore the first two
nonzero callback slots, including event-node projectiles, shot spread, paired
facing, and attacker/victim animation ownership. Exact
`brainutil_PlotTrajectory` and
`brainutil_PlotMaulTrajectory` own ordinary and Maul airborne continuation,
and the reviewed exact boss/vehicle callbacks occupy slots 27 through 29 and
34 through 47. All 18 PDB-emitted `boss.c` procedures are now reviewed
(11,000/11,000 bytes). The recovered `ai_Kadu` owns rider mounting, race
input/speed state, camera focus, and authored HUD bars; `ai_TurretDroid` owns
event-node fire, aimed zap rays, shield sprites, strafe fire, and detachable
arm state. All six PDB-emitted `vehicle.c` procedures are also reviewed
(6,860/6,860 bytes): `ai_Stap` restores the two-rider steering, speed,
catch-up, camera, sound, and paired-gun path, while `ai_Tank` restores driver
ownership/dismount, tracked movement, articulated aiming, projectile timing,
and engine/turret audio. Their exact PDB-named 428-byte
`FindBestMachineGunTarget` dependency is readable source. The PC player is now
constructed through the complete 559-byte PDB-named `jedi_InitPlayer`, which
selects its exact character collision table, scale, movement profile,
48-record `combos1`/`combos2` buffer, and exact `jedi_Main` callback. The eight
initialized collision tables and both combo globals retain their executable
values in readable source; authored external CMB data still replaces the
built-in pointer through the existing exact loader.
The adjacent ground/combat boundary now uses the exact PDB-named
`physics_InitPhysics`, `physics_GetPoly`, `physics_gCheckGround`,
`physics_gCalcTargetPos`, `physics_gCreateObject`,
`physics_gGetNearestTarget`, and `player_DoCollisions` owners. The live PC
field no longer duplicates the successful hot-node contact stores in a host
loop: the original 20-player eligibility, versus-mode, range, relationship,
and hit-publication pass owns them directly.
The adjacent player-state path now uses exact `player_gConnectMotionData`
for both the playable character and every spawned enemy. It resolves the CAD
motion table from the authored header, normalizes both legacy saber-hit name
slots, and restores motion callback indices from the original flags. Six
formerly null exact callback slots now publish `force_FlameCallBack`,
`jedi_FireWeapon`, `ai_Tank`, `ai_Stap`, `jedi_Main`, and `tusken_stab`.
Ranged player motions consequently use their authored single/paired muzzle
nodes, versus-aware targeting, powered projectile override, production sprite
allocation, and sound path; flamethrower and Tusken frame-window behavior are
also live. The
complete 114-entry PDB `sModelNames` table and the four exact
`loader_Get*Name` accessors provide its original readable name boundary.
Exact `player_gRefreshPlayers` also owns the complete 20-slot refresh and
level-eight physics/collision reset tail.
Exact `player_HandleSabre` now owns its original 20-player post-scene
eligibility pass and calls the PDB-named `jedi_HandleSabre` with each live
supported actor. The reviewed Jedi path restores model-specific blade-node
pairs, color selection, normal and long-blade endpoints, white core and
colored glow emission, attack-motion world sweeps, feedback, blade-node
timers, and double-blade handling. The immediate textured blur/cylinder
backend remains an explicit portable glow realization. The PC runtime invokes
the owner at `scene_middleRender`'s original post-model/pre-player-processing
boundary and additively composites the recovered `fx_screenGlow` segments.
The real FED smoke requires two zero-drop blade passes and visible saber
pixels; its current three-frame Debug sample produces 3,304 composited glow
pixels.
The cloak, shield, and star paths retain their original 600-frame lifetimes;
mesmerize uses the exact 20-actor masked range iterator and authored target
reactions. All persistent selections point directly at PDB-named bodies.
The original glow draw calls retain exact names and argument contracts over
dependency-free renderer hooks while their immediate-mode realization is
still reconstructed.

The Theed level path now includes the complete PDB-named `level_Theed` ->
`drawsomecrappywater` -> `fx_Water` chain. It uses the exact initialized
water tables, seven authored patch records, two offset wave layers, nested
sine/UV math, and camera-space immediate polygons. The portable PC runtime
loads the shipped default water texture and realizes those original triangle
strips through the same caller-owned software framebuffer and depth surface
as the world and actors. The installed-asset Theed gate publishes 208 water
polygons in its three-frame validation run.

The ordinary overlay text no longer uses the temporary 5x7 bitmap in a real
installed-game run. The matched `getFontFile`, `LoadFont`, and
`SDLTextWriteScale` instruction paths establish the shipped default overlay
mode, NotoSans face selection, `trunc(scale * ScreenHeight/480 * 24)` point
size, 17-entry tint table, alignment, and alpha contract. A platform-neutral
TrueType adapter rasterizes the original files under `res/font` directly into
the caller-owned framebuffer. It vendors only stb_truetype source, adds no
SDL/SDL_ttf or other runtime library, and retains the compact bitmap strictly
as a missing-asset diagnostic fallback.

The executable's initialized `allTextEverything` localization aggregate is
also retained as readable source under its PDB name. It covers seven retail
languages and 498 published slots per language; the 68,317-byte display-text
corpus is guarded by a whole-table regression hash. Runtime startup follows
the recovered `generateAllText`/font-atlas sequence and widens the retail
UTF-8 strings at a dependency-free platform boundary.

The original title-menu state and data are now source-owned as well.
`MENUVARS`, `menu_mainInitMenu`, title entry, push/pop, player selection,
objective/game-over flow, and related state leaves retain their PDB names.
All ten initialized `mainMdef` title variants are present and hash-verified.
Platform input, texture, bucket, and controller reassignment operations are
narrow hooks; the PC gameplay initializer already consumes the recovered
player-count/model owners. Exact PDB types `MMVDEF` and `MDEF_MOD`, the
75-entry `mmsizes` opcode-width table, `mmNextCode`, the ordinary
`mmDraw`/`mmDrawsub` title commands, localized `mmDrawItem`, and
`menu_mainMenu` selection/activation ownership are now live. Sixteen additional
PDB-named definitions cover player count, difficulty, New Game confirmation,
Options, Quit confirmation, Controls, Language, Video, and Audio,
bringing the source-owned title command data to 28 streams and 3,992 exact
bytes. The ordinary command-stream portion of PDB-named `menu_mainLoop` now
owns Main/Continue, startup, player-count, difficulty, confirmation, Options,
Audio, Language, Video, and controller state dispatch. States `0x23` and
`0x24` call the dedicated 4,991-byte PDB-named `runControlsMenu` owner,
not a generic command-stream substitute. Exact Classic/Modern action and
Force mappings, localized label-index tables, popup stream, Player 1/Player 2
option streams, controller overview layout, and KBM/controller-family artwork
selection are readable source. `getControllerTextures` retains the retail
PS4, PS5, Switch, Switch Pro, Joy-Con, Xbox Series X, generic-controller, and
keyboard/mouse mapping behind one optional platform-name hook. Its state `0x0E`
branch now runs the recovered PDB-named `newMenu_P1CharacterSelect` or
`newMenu_P2CharacterSelect` according to the selected player count. Base and
New Game+ tabs, unlock-aware model cycling, confirmation, abort, duplicate
rejection, controller routing, and saber-color toggling retain the executable's
state and global names. State `0x0D` also owns the matched VS entry and its
exact level/player/mode setup. Exact
`jedi_CheckValidPlayer*`, `jedi_CheckValidVersus`,
`menu_initPlayerSelect`, and `updatePlayerSelectIndex` owners support that
path. The 4,470-byte `newMenu_DrawP1CharacterSelect` and 6,195-byte
`newMenu_DrawP2CharacterSelect` presentation owners now draw their recovered
one- and two-player panels from the original frontend texture bank. Their exact
completed-game controller tabs use the selected player controller's recovered
Classic/Modern glyph pair, including the mirrored two-player placement. Their
selection state machines remain independently testable at the exact callback
boundaries.
`menu_handleMenuTriggers` now
represents the executable's complete
compressed jump table: ordinary destinations, New Game/VS setup, player and
difficulty routing, character confirmation, level eligibility and scanning,
score awards and upgrades, controller fallback, video apply, save requests,
sound/music controls, cheats, and gameplay transitions. Exact
PDB-named `menu_cameraChange`, `menu_menuMusic`, and `menu_sound` own their
recovered leaves, while `tempPlayersVs` retains its executable-backed global
name. The PC
`--title` path decodes the shipped splash through Windows' built-in WIC
boundary and overlays the original definitions with the same TrueType hook as
gameplay. Ordinary HUD text and 16:9 menu text retain their distinct recovered
scale adjustments at that framebuffer boundary. Deterministic real-asset
gates protect the initial carousel, its New Game flow through player count and
difficulty, its confirmation transition, the
navigated Options screen, Title → Options → Controls presentation,
Title → Options → Audio, and a live Video Window Mode edit. The complete exact
74-entry `modVars` table now binds recovered byte, word, and dword fields.
`mmGetModVal`, `mmSetModVal`, `mmIncVar`, `mmDecVar`, `mmUpdateModSet`, and
`mmDrawMod` provide typed access, clamping/wrapping, player locks, known side
effects, localized value formatting, and left/right editing. PDB-named
`menu_playerSelectCheck` and `menu_menuExit` now route player-two modifier
input, activation/back transitions, stack exit, and pad-mask refreshes through
their retail owners. PC storage now uses the exact pointer-free PDB
`saveGameStruct`: the 4,624-byte `SAVEDATA0\\Game` payload preserves named
progression, checkpoints, character state, combos, unlocks, difficulty, and
player metadata. `SAVEDATA0\\Options` likewise retains the exact 56-byte
`optionstruct`. Interactive runs load both, menu save requests write Game,
and clean shutdown writes Options; headless gates remain isolated from user
saves. Both readers reject wrong-size or wrong-version payloads before
mutating runtime state. Controller enumeration, video-mode application, and
level resource work remain narrow dependency-free platform callbacks.
Unresolved in-game modifier behavior and a full-game camera survey remain
open. Every ordinary user-reachable title branch now has a navigated
installed-asset gate, including Language regeneration, both Quit outcomes,
and the complete two-controller VS route into Arena. The
matched executable's New Game stack retains one explicitly documented
evidence gap: its exact streams and trigger table route player count to
difficulty and then back to the state-4 player-count stream, while no recovered
owner supplies a demonstrated state-`0x0E` handoff. The PDB-named menu owner
remains unchanged; the PC host now owns a clearly isolated presentation bridge
from that state into the independently recovered character selector. A
successful character selection now enters the recovered state-`0x1A`
`menu_drawLevelSelectScreen`/`menu_levelSelectMenu` owner. That selector uses
the exact `levelSelectMdef`, unlock filtering, localized labels, and all 15
installed retail preview images. Its draw owner now also restores the shipped
orb backdrop, nine-piece carousel/selection marker, bold stage digits, and
the executable's 2.5 stage-heading scale. The title command stream's cyclic
items are clipped to its authored command-`0x44` selection panel rather than
bleeding into the surrounding presentation. Confirmation advances through recovered
state `0x66`, after which the PC host invokes the exact `menu_levelSelect`
leaf and reinitializes the chosen installed map with the selected character
assets. The gameplay renderer geometry/depth result is visually accepted. The
latest settled FED frame also aligns its doorway, horizon, and player scale
with the supplied reference; authored transitions and other camera regions
still require full-game visual validation.

The title runtime now uses PDB-named `menu_readControl` for both controllers,
including the original edge masks, two 16-sample cheat histories, Zoom-Out
filter, keyboard-scancode ring, and 18,000-frame screen-saver state.
`checkKeyboardBuffer`, `cheatCheckKeyboard`, `cheatCheck`, and
`menu_rotControls` restore the adjacent consumers. Win32 publishes the same
SDL/USB scancode indices through a dependency-light callback rather than
introducing SDL into portable code. Recovered MMV trigger remapping and every
decoded dispatcher branch now replace their former default-menu fallback.

Current regression coverage includes a dedicated installed-asset Controls
gate in addition to the dependency-light and other real-asset tests in both
Debug and optimized Release, including a direct shipped-font
rasterization gate, the headless original-title flows, and both direct and
fully navigated two-character Arena/VS runtime gates. Headless phases retain
independent P1/P2 input ownership instead of mirroring one scripted pad onto
both players. Dedicated FED camera gates additionally
lock the collision-selected dolly, first-frame authored update count, camera
location, and angles, then prove that the opening room director advances to
its authored dollies 144 and 145 while holding the retail control lock. They
then prove the linked protocol-droid cue spawns both battle droids, retains
the authored director sequence, and releases the player lock. The second transition
specifically protects opcode `0x108`'s recovered `bapEnemySetContinue`
traversal through the director's gas branches. A stationary post-handoff gate
protects raw collision dolly 3 and target projection `(639.7,238.5)` at frame
480. The dolly-0 movement assertion deliberately remains strict at
`(429.3,263.9)` at frame 420; the currently staged baseline instead produces
`(444.2,300.8)`, so that known-off dynamic camera remains an open red gate
rather than being silently rebaselined during front-end work.

The source-driven control matrix currently covers every base character's
one-button and running attacks, analog walk/run thresholds, camera-relative
movement, four lock-on directions, block, all four maximum-progression Force
chords, standing/running jumps, and exact authored CMB chains from two through
six hits. Maximum-progression runs unlock the complete combo mask before CMB
initialization. The matrix covers all nine two- and three-hit chains, all eight
non-Force four-hit chains, four five-hit chains, and Obi-Wan's six-hit chain,
including exact input timing, queued motion names, combo records, and tally
progression. The player frame also restores the PDB-named
`brainutl_ConformGeomNodes` owner immediately after the scene/input update.
It applies the executable's normal and small-mode model scale, minimum closing
distance, head scale, and eight-entry big-hands/feet/saber node table for each
active player. Its exact controller chords, independent P1 keyboard chords,
and one-press debounce are preserved, including keyboard polling when XInput
was the most recently used device. Adjacent exact leaves restore saber-edge
sampling, the 16-bit `brainutl_FindLSB` contract, and `WInput_IsKBM` without
adding a new runtime dependency.
Chained attacks are validated against the selected character's loaded CMB
records instead of being mistaken for failures to remain in the base attack.
The focused Debug combo matrix passes 33/33 (31 playable chains plus real-asset
metadata and source-provenance gates). All eight PDB-named `combo.obj`
procedures are now reviewed against the raw decompiler export. The formerly
omitted `combo_ValidComboAward` owner is restored with its exact per-character
`numHits <= threshold` test. Source-level coverage also protects character
threshold selection, unavailable-combo and level/versus bypasses, early/late
input markers, held-key admission, and reset-time held/released masks. Saber
validation now samples every
authored damage/callback frame rather than only the final idle pose: it checks
the PDB-derived per-character color, primary/secondary attachment nodes,
matched white core, 14-19-unit outer width, and 112-unit normal blade length.
The 27 base attack cases cover the full nine-character roster, dedicated
Mace/Ki-Adi gates cover 59-frame saber-toss actions, and an Arena gate proves
simultaneous Obi-Wan/Qui-Gon ownership for both players. The optimized Release
controls label passes 307/307 (212 roster cases), its complementary
non-controls partition passes 532/532, and the complete optimized Release
suite passes 839/839. The block path now has a full shipped-asset lifecycle
gate rather than only an entry-frame assertion: Obi-Wan enters authored
Motion 15, queues Motion 21, records exactly one release edge, and recovers
to Motion 0 with an empty queue and lock 0. Focused source-level tests also
protect the executable's idle priority (lock-on Motion 20 before low-energy
Motion 19, with the low-energy threshold strictly below 26) and the
`braindmg_Blocking` reaction path through Motions 16-18, chained Motion 21,
hit-delay/stun/Force accounting, and its defense-upgrade-adjusted accumulated
damage threshold.
Focused instruction review now also preserves two non-obvious damage-control
owners. The special Obi-Wan/Mace forced-block test reads the player's locked
target, not the separate actor that supplied the current hit; tests exercise
both disagreeing relationships so they cannot be conflated again. AAT player
ID 35 forces death effect 18 instead of trusting the current motion's `fx1`.
That branch is protected with an intentionally invalid motion effect and is
consistent with the installed `tank.cad`, `aat.bmd`, and `aat.cmb` assets.
The Force subset now validates more than its entry pose. Phase telemetry names
the exact active PDB callback owner, publishes energy/Force/item resources and
all six `playerObject.forceData` values, and keeps absent P2 resources distinct
from the allocated inactive-player slot. Shipped Adi assets prove mesmerize
ownership through control tick 300 and exact clearing at tick 301; shipped Mace
assets traverse `mw_absorb_a_force`, `mw_absorb_b_force`, and
`mw_absorb_c_force` before recovering to idle. The shield, star, cloak, and
Ki-Adi mesmerize variants prove their first callback tick and exact resource
cost. Focused source tests additionally preserve `force_gActivate`'s original
upgrade and inventory gates, callback/motion-owner re-entry gates, unconditional
north/south feedback placement, and its unusual simultaneous item+north and
low-Force item ordering rather than replacing those behaviors with a cleaner
but inaccurate policy.
Classic XInput is also exercised end to end rather than only at the mapper
boundary: the recovered diagnostic override selects the Classic scheme for a
single player, A/X/Y/B retain their original south/west/north/jump layout, and
LB is validated as the Force modifier for the four Force/item chords.
Two longer real-asset sequences now exercise the control owners as continuous
gameplay rather than isolated inputs. A 246-frame logical sequence covers run,
running attack, recovery, block, lock-on, locked shuffle, authored jump and
landing, and Force; a 291-frame physical XInput sequence alternates P1/P2
attacks, lock-on, jump, Force, simultaneous block, and recovery while checking
per-player edge telemetry and action-window sabers. Phase diagnostics expose
both players' motion/frame/lock state and distinguish the live lock flag from a
cached target pointer. Cross-character and cross-level recovery now has a
separate source-provenance matrix. Matched `WorldBlocking` reads
`playerObject+0x88` (`playernum`) when it selects the live collision-node row;
the former use of adjacent `playerObject+0x8a` (`playerID`) only happened to
work for Obi-Wan in slot 0 and Qui-Gon in slot 1. The same executable trace
identifies `physicsObject+0x180` as `currentmapinfo.poly`, not the neighboring
cube pointer. Treating the cube header as a polygon flag word could falsely
mark Coruscant and Marsh contacts as liquid and suppress `brainutl_Land`. All
nine base characters now launch and recover to their exact authored idle
motion, Mace does so as player two, and Palace/Coruscant sequences recover
into a follow-up attack in both Debug and Release. Phase telemetry publishes
both the accumulated airborne velocity and collision-adjusted frame movement.
A source-order unit gate additionally preserves the
retail main-animation callback rule: an active callback owns the frame, and a
callback that completes does not reinterpret held input until the next frame.
The input reconstruction also restores the PDB-named six-channel shocker
lifecycle. Exact `ShockEffect` field order now drives timer, low motor, high
motor, and priority state; per-frame updates expire effects at their authored
tick count and stop vibration. Focused tests cover option gating, invalid
effects, immediate clearing, the retail `startRumble` callback behavior, and a
four-tick hard-hit effect that would have lasted 255 ticks under the former
shifted stores.
The remaining controller lifecycle now uses the exact 34-byte
`ControllerPacket` and `PadGone` rules for packet status, `'A'`/`'s'` format
changes, presence/type masks, shock capability, and vibration shutdown.
PDB-named Windows owners preserve the retail no-op P1 disconnect fallback,
P2 disconnect flags, button queries, and menu-binding state. The PC adapter
tracks physical additions separately from already attached pads: losing P2
cannot silently substitute another connected controller, while a newly
attached eligible controller restores the slot as `AddControllerDevice` did.
`input_ReadControlPad` and `maskPadBits` again publish the exact menu/title
state before reading input, and keyboard Shift, Escape, and Enter/Space now
emit their distinct retail title-menu, submenu, and gameplay bits.
The source had incorrectly treated `Motion.disp` as an entry-frame skip; the
exact `Motion.cutin` field correction changes 1,048 records across 61 of the 93
shipped CAD files and allows Ki-Adi's first south attack to begin at frame 1
instead of being consumed immediately. Ki-Adi's authored five-hit `s.s.s.w.w`
remains an open asset/runtime discrepancy for a narrower reason: Motion 130's
`disp=24` slack clamps to its last playable frame and recovery wins before the
second south input can queue. Its real metadata and reachable four-hit
`w.w.w.w` chain are regression-covered. The newer source build is not copied
over the installed game-root executable while the reported post-character-
select crash remains an open interactive gate.
Interactive diagnostics now keep a silent last-checkpoint snapshot before
every title/gameplay frame and at each destructive menu-to-gameplay handoff
stage. An unhandled exception records that checkpoint together with the
exception code, module RVA, thread, and x64 RIP/RSP/RBP values. Control-state
transition records also include the animation frame/lock, combo prefix and
held/released masks, chain slack, active callbacks, and player flags. This
keeps the host dependency-free while making a character-select or live-control
failure attributable to a precise source boundary.
Physical-controller integration tests now drive XInput-format A/B/D-pad state
through the same mapper used by a connected pad instead of injecting already-
translated game bits. One-player New Game and two-player Versus paths cross
character selection, replace the runtime, retain their controller ownership,
apply the gameplay button scheme, and execute their first authored attacks.
Direct real-asset gates cover the complete modern gameplay face-button and
context surface: A jump; B/X/Y attacks; all four LT+A/B/X/Y Force/item chords;
LB block; RB lock-on against a live authored target; Back zoom with the exact
eight-frame orbit contraction; and Start's recovered pause flag. Each gate
checks physical-to-logical bits, pressed/held/released counts, and the authored
motion or state reached by production gameplay.
The same production mapper is now driven from physical keyboard-format
headless phases rather than pretranslated gameplay bits. Fourteen real-asset
gates cover W run, Left-Control walk, Space jump, J/K/L attacks, Shift block,
H lock-on, all four U/I/O/Y Force/item chords, T zoom, and Escape pause while
checking keyboard ownership, exact emitted bits, input edges, and authored
motions. An eight-case Arena matrix independently drives P2's three attacks,
all four Force/item chords, and lock-on movement through physical XInput state.
That matrix exposed and corrected two diagnostic-fixture asymmetries: P2-only
bits no longer trigger P1 movement validation, and completed-save item
inventory is reapplied after P2's source-matched initialization clears its
character data. P2's `LT+X` now reaches Qui-Gon's authored Motion 65 instead
of remaining idle despite correctly encoded input.
The airborne audit also corrected two adjacent PDB-field identifications in
the central gameplay brain. Matched x64 offsets keep `currentMotion` at
`playerObject+0x1B4`, clear `ACTION_LOCK` at `+0x1BC`, and store trajectory
owners in `pMotionCallBack` at `+0x220` rather than `pMainCallBack` at
`+0x218`. The old mapping erased the active motion every frame and prevented
`brainutil_PlotTrajectory` from recognizing jump motions 4/22, so airborne
steering and the second jump press could never execute. The same correction
now preserves active motion through damage recovery and hang release.
Real-asset regressions prove Obi-Wan's standing and directional Motion 22
double jumps, in-air steering without a ground locomotion substitution, and
independent player-two Mace double jump. The combat isolation gate now freezes
only its selected validation target's existing AI locomotion while retaining
the real collision, `DamageControl`, energy, and reaction pipeline.
The portable loading bridge now consumes the character-selection confirmation
and waits for both participating devices to return to neutral before actor
construction can observe gameplay input, matching the edge clear owned by the
retail loading path. Physical LT+B Force chords cross the same boundary for
P1 and P2. The latter also protects the matched executable's unusual split:
physical pads translate through their per-player configurations, while the
gameplay brain deliberately uses P1's scheme byte for both players' Force,
block, lock-on, and combo-name lookups. All 26 authored level starts are
covered for both players' exact position, facing, and first-frame input/lock
reset state. That start-state gate now executes the input owner rather than
checking only cleared fields: after each refresh, P1's first direction/action
sample appears once on the pressed channel and continuously on the held
channel, its next held sample does not repeat the edge, release re-arms both,
and P2's independent history remains neutral. A physical-XInput front-end
gate also keeps the final A confirmation held for seven frames across runtime
replacement. The neutral guard suppresses every one of those frames; only the
later scheme-1 B attack reaches gameplay. Exact `player_RefreshPlayer` and
`input_ReadControlPad` comparison found no production mismatch in this audit.
This specifically covers the boundary implicated by the reported crash; the
instrumented candidate completes it in Debug and Release, while the interactive
installed-build report remains open until it is retested by the user.

The ranged-control path now retains the renderer-owned CAD event boundary as
well as its input and motion selection. Exact `jedi_FireWeapon` instruction
review shows that character-specific muzzle/aim nodes are selected by the
human-readable PDB field `playerObject.playerID`, including paired-shot and
powered-projectile branches. The installed pistol motions themselves carry
flag `0x00100000`; exact `player_gConnectMotionData` maps that flag to callback
slot 1, the PDB-named `ai_FireWeapon`, while `jedi_FireWeapon` remains covered
for the model families that select it. The portable model publisher rebuilds
`modelObject.eventMask` and `effectMask` from every decoded frame before the
player scheduler runs, matching `render_RenderModel`/`render_RenderNode`
instead of treating event bytes only as collision-hot markers. A
diagnostic-only bullet observer lets headless tests prove successful live
launches without suppressing or substituting projectile behavior. Against the
installed CAD/BMD/CMB assets, Amidala's base pistol produces three type-18
shots, the Amidala and Panaka single-pistol actions produce one each, and
their authored melee actions produce none. The full Debug controls partition
passes 308/308 and the complete optimized Release suite passes 840/840.

The menu-to-player control boundary also has non-headless regressions now.
`--hidden-window --scripted-input` creates the ordinary Win32 window and drives
the normal title/menu input owner without displaying a test window; the public
scripted phases use the readable `--input-phase*` switches. One installed-asset
test traverses title, New Game, one-player story selection, character selection,
FED selection, and runtime replacement. It asserts 36 successful
`StretchDIBits` presentations, exactly one handoff, and 21 stable gameplay
frames after the handoff. Its explicit build-tree persistence directory also
round-trips the exact 56-byte options record while proving that New Game creates
no game-save write. A second real-window test traverses title, VS Mode, two
players, and Arena, then proves that both players enter gameplay with neutral
controls. `--silent-audio` retains full audio-bank and hook construction while
disabling device output; both tests require successful title and gameplay audio
generations. That gate exposed and fixed a real ownership bug where interactive
audio rebuilds omitted P2's selected CAD: the shared rebuild path now follows
`GameStruct.ModelSelect[1]`, and the Arena regression resolves Qui-Gon's
bank-2 `vjdie.wav`. The active optimized Release controls partition now covers
317/317 cases, and the complete optimized Release suite passes 849/849. These
tests cover the real interactive construction and presentation path implicated
by the reported post-character-select failure without touching the user's
saves. The 849/849 Release candidate is now installed as
`C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe`.
The installed New Game path has also been retested with a visible window, real
keyboard `J` confirms, and a connected XInput controller. That route reaches
FED gameplay from the installed front end. Runtime replacement now checks actual
FX material readiness instead of requiring a fixed number of newly loaded
default textures, and the persistent log records live key edges plus runtime
init failure stages.

The FBX-specific
PC gate requires the shipped FED FBX sidecar and verifies that ufbx produces
nonempty material batches,
vertices, and triangles instead of silently returning to JPX visuals.

The matched 2024 PC executable's live level path loads
`res/level/jpx/<name>/<name>.fbx` through embedded ufbx 0.6.1,
`ufbx_load_file`,
`_InitFBXLevelData`, and `CD3DApplication::DrawLevel`. `InitJPX`,
`SetupWorldmeshMatrix`, `_CubeRender`, `_FatRender`, and `_ThinRender` are
retained legacy paths with no live caller in this executable. The PC
reconstruction now follows that evidence: a narrow ufbx 0.6.1 adapter imports
the shipped FBX, applies the exact level transform, and publishes flattened
triangle/material batches to the portable software renderer. This removes the
coplanar floor fighting and malformed wall/door surfaces produced when the
quantized JPX collision mirror was incorrectly used as the visual level.
JPX remains the gameplay collision source and a fallback for format tools.
The live FBX boundary is now mapped further: `_InitFBXLevelData` classifies
materials with the exact PDB procedures `isTextureTransparent` and
`isGlassTexture` into three mesh vectors, and `DrawLevel`,
`DrawLevelTransparent`, and `DrawLevelTransparentGlass` consume them with
distinct opaque, transparent, and no-depth-write glass pipelines. The exact
level rasterizer uses `D3D12_CULL_MODE_NONE`, so the dependency-light renderer
also draws both projected windings. The exact `cullmesh` global controls the
level-8 mesh exception in all three passes. Its `int[32]`
layout and initializer are recovered, as is the non-obvious Streets lookup:
`DrawLevel` maps `WallNN_Broken` to slot `NN*2-1`, `WallNN_Solid` to
`NN*2`, and the base `streets_A0` mesh to slot zero. In non-Streets levels,
the opaque pass uses vector position when it has at most 31 meshes; the
transparent and glass passes draw unconditionally. Exact `cube_InitVisibility`
initializes the FED, Tatooine, and Streets variants, including Streets' solid
even slots and hidden broken odd slots. These policies are tested independently
of an FBX dependency. The shipped Streets JPX mirror now has a sparse,
fingerprint-guarded ownership map for all 728 nondegenerate dynamic-wall
triangles, so the three dependency-light render passes respond to the same
`cullmesh` state as the named FBX meshes. Modified JPX files safely bypass the
asset-specific map.

The PC level adapter and optional evidence probes are built against an
external checkout of exact ufbx 0.6.1. ufbx is compiled only into the Win32
host/probes; the portable game runtime consumes `JPBSoftwareLevelMesh` and has
no ufbx dependency:

```powershell
cmake -S . -B build `
  -DJPB_UFBX_SOURCE_DIR="C:\path\to\ufbx-0.6.1"
cmake --build build --config Release `
  --target jpb_pc_game jpb_fbx_level_probe jpb_fbx_jpx_match_probe
```

An early nxdk bootstrap remains available as historical toolchain/ABI
evidence, but no further nxdk work or XBE builds are authorized while the
complete, dependency-light PC reconstruction is built and validated. The
Xbox phase begins only after the user approves full PC functionality. At
that point, the bootstrap can be built from an MSYS2 MinGW64
shell:

```bash
./tools/build_nxdk.sh
```

The resulting XBE is written to `build/xbox/default.xbe`.

See [docs/XBOX.md](docs/XBOX.md) for the pinned local nxdk revision, current
target contents, and runtime-validation status.

See [docs/RECONSTRUCTION.md](docs/RECONSTRUCTION.md) for provenance rules and
the next milestones.
