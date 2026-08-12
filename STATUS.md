# Project status

## Milestone 0 - evidence inventory and decompiler export

Completed:

- Exact PE CodeView/PDB GUID and age verification.
- Reproducible parsing of 107 core object modules.
- Inventory of 3,865 emitted core procedures and 22,575 named
  parameters/locals.
- Recovery of 109 original project source-file paths and SHA-256 checksums.
- Direct `DEBUG_S_LINES` mapping for 3,188 procedures covering 1,130,040 of
  1,148,675 emitted procedure bytes (98.38%).
- Generation of 107 module-shaped source shells.
- Host CMake build and smoke-test executable.
- Unit tests for module classification and decimal/hexadecimal CodeView
  address handling.
- Ghidra headless exporter keyed by exact PDB function addresses.
- Ghidra 12.1.2 analysis with all 3,865 exact PDB procedure boundaries seeded.
- Raw decompiler export of all 3,865 procedures into 102 recovered source-unit
  files, with zero missing exact entries and zero decompiler failures.
- Representative-address audit: `jedi_InitPlayer` exported from RVA
  `0xB3220` with the PDB-recorded 559-byte extent.

## Milestone 1 - portable foundations

Completed:

- Extended the reproducible PDB inventory to 13,212 linked data symbols,
  including the named timer state that Ghidra initially rendered as anonymous
  addresses.
- Recorded reviewed `Node`, `List`, `_graph_entry`, and `MemoryPool` layouts
  with exact TPI indices and target-width assertions.
- Recorded reviewed `VECTOR`, `_svector`, `_sfvector`, `MATRIX`, `FVECTOR`,
  and `Plane` layouts with exact TPI indices and cross-target assertions.
- Selected and regenerated a 23-module foundation dependency queue.
- Reviewed and integrated all eight `list.c` procedures (269 procedure bytes).
- Reviewed and integrated all seven `timer.c` procedures (159 procedure
  bytes), its five data symbols, and the exact five-entry round-timer table.
- Reviewed and integrated all four `alloc.c` procedures (427 procedure bytes),
  including the original 64 KiB boundary-tag heap, four-byte alignment,
  first-fit search, and bidirectional free-block coalescing.
- Reviewed and integrated all ten `memory.c` procedures (1,072 procedure
  bytes) and the four-bank `MemoryPool` layout, preserving the bump allocator's
  strict capacity rule and non-clearing behavior.
- Reviewed and integrated all 49 fixed-point/transform `fmath.c` procedures
  (20,012 procedure bytes), preserving the reference constants,
  float-vs-double rounding boundaries, SSE conversion sentinel,
  normalization padding, local transform state, matrix multiplication order,
  rotation order, sequential aliasing quirks, translation behavior, and
  screen-depth evaluation. Exact packed ten-bit float/long/normalized/strided
  batches, short- and float-vector batches, all legacy perspective variants,
  `RotTransPersFloat`/`RotTransPersSFV`, and CameraMatrix-backed
  `TransformPoints`/`TransformPointsFV` now replace the final bodyless records.
  Projection retains the executable's near clip (`1.0`), focal scale
  (`460.0`), screen center (`320,240`), depth divisor (`10240.0`), and packed
  low-x/high-y screen-coordinate format. That exact legacy API remains
  separate from the live PC adapter: `jpb_ProjectPcToViewport` uses the
  matched D3D initialization's 53-degree vertical field of view, current
  render-target aspect ratio, and `1.0` near plane. Its standard-C software
  depth stays on the shared legacy `10240.0` scale rather than importing D3D.
  The camera-space companion retains vertices exactly on that near plane so
  the portable rasterizer can clip primitives before perspective division.
- Reviewed and integrated all 32 `flex.c` procedures (3,118 procedure bytes),
  preserving fixed-12 truncation, 32-bit wrapping, sequential cross-product
  aliasing, short-field overflow, packed-normal extraction, segment rejection,
  normalization padding, and the retail empty `intersec_2dlines` stub. The
  portable stub returns deterministic `NULL`; its otherwise indeterminate
  retail return register is documented at the implementation.
- Reviewed and integrated all 29 foundational `vectors.c` procedures (3,425
  procedure bytes), covering matrix inversion, distances and range tests,
  polar conversion, rotation derivation, scaling and normalization, and
  plane definition/projection. The reconstruction preserves 32-bit arithmetic
  wrapping, strict/open comparison boundaries, NaN comparison behavior,
  low-word return quirks, padding retention, and the identity matrix's
  translation reset. Balanced renderer matrix-stack calls around three local
  rotation helpers are deliberately omitted; their normally neutral side
  effect is documented and the unused, otherwise indeterminate return is
  defined as zero.
- Reviewed and integrated all 11 procedures in `win32/IO.c` (757 procedure
  bytes) as the game's portable file seam. Its implementation uses only
  standard C stream, memory-copy, and allocation facilities; the host-width
  opaque handle avoids embedding a Windows type in gameplay code.
- Reviewed and integrated all 25 `filesys.c` procedures (6,745 procedure
  bytes): stream/pool ownership, exact decoding of all 46 chunk identifiers,
  the complete target-format dispatch loop, and actor, AI, enemy, animation,
  library, map, palette, material, dolly, emitter, tag, Jonny-map, effect, and
  resident-sprite loading. Added exact `wapChunk`, `rdVECTOR`, `CVECTOR`,
  `BAP_CAMERADOLLY`, `BAP_AI`, animation, placement, library-part, and
  native-pointer `WorldData` layouts. Desktop resource/texture/event/UV
  behavior is isolated behind game-owned hooks; the AI pointer registry uses
  a dependency-light C vector with the reference insertion-index behavior.
- Added a dependency-light PC asset probe and validated every one of the 27
  installed `res/level/W3D/*.j3d` archives (65,808 through 431,184 bytes).
  Each archive is consumed exactly. The real-data audit recovered the
  reference treatment of header-only chunks and the terminal Jonny cursor
  quirk; both now have bounded portable behavior and regression coverage.
- Added an optional `JPB_GAME_DATA_DIR` CMake setting that registers each
  original level archive and JPX mesh as a labeled `real_assets` CTest without
  copying or redistributing game data.
- Recovered the exact four-byte `MATHEAD` and 4,102-byte `_BINHEADER` layouts
  used by `InitJPX`, then reviewed and integrated that 644-byte procedure as
  an allocation-free, bounded JPX runtime path. It reads through the portable
  IO seam into caller-owned storage, resolves renderer descriptors through a
  narrow hook, preserves the reference progress cadence, and publishes the
  recovered `WorldmeshData` boundary. All 25 shipped meshes (26,768 through
  3,602,984 bytes, up to 11,446 patch sites) pass the real-data gate.
- Added renderer-neutral pre-patch iteration across all 100,489 shipped JPX
  binding sites. The 96,860 stock sites carry `STRPHEAD`; all 3,629 sites in
  `modhangar.jpx` intentionally use an alternate payload and remain accepted.
  This preserves the supplied mod mesh rather than imposing a stock-only
  marker rule.
- Recovered the complete bounded strip-payload relationship across those
  sites: a 16-byte renderer descriptor, a zero-low-half vertex count,
  `count * 16` vertex bytes, and 4-, 8-, or 28-byte metadata before the next
  descriptor. All 2,000,131 shipped vertices decode as signed Q7 x/z, float y,
  signed Q12 u/v, and an uninterpreted 32-bit attribute word.
- Recovered a conservative renderer-neutral view of 54,579 static JPX spatial
  records across the 25 installed meshes: index, x/z/y center, radius, and
  vertex count. Only the exact `tag=-1`, `flags=1` form is decoded; the
  `0x80001` and `0x100001` dynamic/special spatial forms remain opaque.
- Added a standard-C `jpb_jpx_preview` that renders the recovered triangle
  strips as a height-colored wireframe binary PPM without a windowing or
  graphics dependency. It supports the top-down format view and a perspective
  view driven by the recovered game projection plus a clearly substituted
  look-at inspection camera. `STREETS.jpx` yields 10,519 strips, 185,859
  vertices, and 150,927 non-degenerate triangles with coherent level geometry.
- Recovered the matched PC BMD geometry consumption at `_RenderNode` RVA
  `0x129030`: three signed-10-bit packed vertices per `geomData.numVerts`
  unit, a shared transformed-vertex prefix, 8-byte signed-16-bit face records,
  `0x7fff` triangle sentinels, four float UV pairs per face, and per-corner
  packed normal/color streams. The matched executable's `gl_RenderNode` RVA
  `0x12E6E0` supplies the alternate 4-byte packed-8 face record and `0xff`
  triangle sentinel. The bounded view selects only a fully valid exact
  layout: 145 installed archives validate at every node. Four `gate*.bmd`
  variants expose packed indices but an incompatible 8-byte-per-face UV
  stream, `weasel.bmd` contains one out-of-range face node, and `skin2.bmd`
  remains the recorded 720-byte truncation.
- Integrated all six original `win32/nodes.c` procedures (5,732/5,732
  procedure bytes). Exact `_RenderNode` now expands each
  signed-10-bit vertex into `_RenderPackets`' shared 3,072-`FVECTOR` cache,
  resolves signed triangle/quad indices, preserves the `0x7fff` triangle
  sentinel, advances variable per-corner color streams, applies both retail
  material color-override modes, and submits the original UV/color/position
  payload through `_StartPoly`/`_SetVert`/`_NoScaleEndPoly`. Exact
  `_RenderPackets` retains packet order and the single cache shared across
  packets. The matched 0x88-byte `primRendPacket` stride and all observed
  field offsets are asserted while its unread byte ranges remain explicitly
  named as reserved rather than assigned speculative semantics. Focused tests
  cover transforms, signed indices, UV/color publication, overrides, and
  cross-packet shared vertices. Exact `render_CreateRenderPacket` composes the
  scene and model rotations, scene-relative translation, transposed light
  matrix, geometry owner, and scene index in the asserted 0x88-byte packet.
  `render_RenderModel` and `render_RenderNode` now own root-motion snapshots,
  parent/child transforms, authored node rotation/scale, exploding-node
  movement, frustum results, model/node visibility, event/effect masks, and
  recursive packet publication. Exact `render_RenderScene` restores the
  20-object iteration and the retail shared-physics alias of `pos`, `angle`,
  `mov`, and `mapinfo`. The complete asserted 0x408-byte PDB-named
  `SramModelStack` replaces anonymous decompiler offsets. Focused integration
  tests exercise a two-node hierarchy, scaling, event/effect publication,
  scene-to-model packet transforms, capacity limits, clipping inputs, and
  physics aliases. The exact three-byte `_HandleBackDrop` retail compatibility
  stub is also restored for the eventual scene scheduler.
- Restored the original sprite presentation scheduler and its draw owner.
  Exact `_RenderSprite` now handles screen rectangles (including source/dest
  flips and the 32-entry retail `cluts` table), direct world quads, and
  camera-relative rotated billboards before publishing the original UV,
  color, and depth payload. Exact `sprite_SpriteRotScale` rebuilds each
  rotated/scaled SCB quad, while `sprite_SpriteWork` owns callback execution,
  node attachment, surface-dependent flips, sprite/SCB counts, deferred
  frees, expiring SCBs, and both double-buffered intrusive lists. The retail
  `sprite_SwapData` no-op and both 33-byte `SetRotMatrix`/`SetTransMatrix`
  compatibility copies are restored. Ring lifetime, motion, acceleration,
  limit, and reflection behavior now flows through the same scheduler; its
  cylinder geometry remains on the renderer-neutral semantic seam. Focused
  tests cover every sprite draw mode, color/depth/UV publication, rotated
  geometry, live/free transitions, counts, list swaps, and exact
  `gGTEMATRIX` publication.
- Added dependency-free posed BMD material realization. It reproduces the
  `render_RenderNode` parent/local matrix order, translation inheritance,
  authored node rotations, optional node scale, signed face-index behavior,
  and `_RenderPackets`' exact 36,864-byte/3,072-`FVECTOR` scratch bound.
  Exact `render_RenderModel` root setup now receives the decoded key frame's
  `v3RootTranslation`; the static BMD root-node translation is neither used as
  a substitute nor applied twice.
  `model_gInitModelRoot`'s exact fixed `0x2000` initial model scale is also
  restored. Exact PDB `pairUV`, `faceUV`, `_Material`, and
  `TEXTURE_SAMPLE_TYPE` layouts and the `_StartPoly`/`_SetVert`/
  `_NoScaleEndPoly` boundary establish the per-face UV, color, texture, and
  sampler inputs. The portable filled path uses perspective-correct UV/color
  interpolation, bilinear clamp sampling, model depth testing, alpha blending,
  and the shipped `PixelShader.hlsl` texture-times-color/black-discard rule.
- Recovered the complete observed `_Material.flags` policy and carried
  `flags`, `samplerType`, and `colorOverride` through the portable texture
  seam. Exact `SetTextureColorOverride`, `ClearCachedTextureIndices`,
  `IsBusTextureForCorus2`, and `IsCoffinTextureForPalace` now have reviewed
  PDB-named bodies and exact backing globals. Flag zero performs the shipped
  negative-winding rejection, flag one is two-sided, and flag two is
  two-sided with depth forced to `0.0001f`; the BMD renderer also reproduces
  `_RenderNode`'s grayscale and `-1000` dark-red overrides. Focused tests
  exercise the per-level texture exceptions, caches, winding modes, and
  resulting vertex color.
- Added a dependency-free TGA decoder for the installed corpus's uncompressed
  color-mapped, true-color, grayscale, and RLE true-color variants at 8, 16,
  24, and 32 bits. The PC game runtime lazily maps BMD `.bmp` names to sibling
  `tga/*.tga` assets through a bounded per-model cache. Renderer texture lookup
  is callback-based so the later nxdk path can upload or sample the same
  decoded assets without inheriting Win32 or D3D.
- Recovered the exact pointer-free 76-byte `Camera` (TPI `0x121A`) and
  176-byte `sceneGeometryEnv` (TPI `0x125B`) layouts. All 23 PDB-listed
  `camera.c` procedures now have reviewed source bodies, including fixed-point
  slide interpolation, absolute and relative view conversion, state accessors,
  shake state, snap behavior, the exact current-camera-type getter/setter,
  `camera_ScrollCamera`, `lerp`, `map`, `mapClamped`, and `mapDouble`. The
  2,093-byte `camera_SetCameraPos`, 832-byte `camera_SetCameras`, and 2,785-byte
  local `camera_StuffCamera` restore collision-selected dollies, candidate
  visibility rejection, follow/slack/uber constraints, all authored focus
  modes, focused and Streets/JarJar special modes, and camera publication. The
  PDB did not retain names
  for the latter pair's two backing objects, so their descriptive
  `jpb_current_camera*` names are explicitly marked inferred.
- Reviewed 24 of 25 `scene.c` procedures (3,613 of 5,021 procedure bytes):
  current/raw matrix and view accessors, `hurtplayer`, transform getters and
  setters, single-point projection, pool allocation, four retail no-op stubs,
  key-frame publication, and `scene_postRender`. Their exact low-word
  coordinate stores, partial snapshot write, fixed-frame timer update,
  scene-ready gate, strobe countdown, background-color quarter scale, and
  double-buffer toggle are preserved. The exact 200-byte `sceneRoot` PDB type
  now owns `GeometryEnv` at offset 8, eliminating the former duplicate
  descriptive state object. `scene_gInitRoot`, `scene_gInitScenes`, and
  `scene_gCreateObject` restore root/model ownership and all three developer
  command registrations through the reviewed 255-entry `console_AddCommand`
  registry. `scene_postRender` now owns these state
  transitions in the PC runtime instead of a duplicated host-side toggle.
  Exact `playerOffScreenArrow` restores packed projection/clipping, both
  players' authored warning cadence, the sixth-warning energy penalty, dolly
  and level suppression, and the four-vertex translucent direction fan. Its
  `_StartPoly`/`_SetVert`/`_EndPoly` path publishes the original vertex
  payload through a dependency-light renderer seam.
  Its exact rotation, coordinate conversion, translation, destination-matrix
  construction, and `gGTEMATRIX` state remain built from reviewed math
  primitives.
  `jpb_ProjectCameraToViewport` now carries gameplay camera state through that
  recovered matrix path and the exact perspective projection without a
  desktop rendering dependency.
- Recovered four of the 11 `input.c` procedures (445 procedure bytes) and the
  exact eight-byte `PADLOAD` layout. `ClearInput`, `initPSXPad`,
  `input_ReadControlPad`, and `maskPadBits` preserve the reference pad-index
  masking, held-button suppression, rising-edge behavior, continuous masks,
  bit-15 clearing, and store order behind one platform-neutral raw-pad
  callback.
- Extracted JPX traversal, projection, clipping, wireframe inspection, and
  filled material rasterization into a standard-C renderer that writes a
  caller-owned X8R8G8B8 framebuffer. The filled path decodes shipped UV and
  packed CVECTOR attributes, lazily resolves adjacent JPX TGA materials,
  applies repeating bilinear texture sampling, and uses a reusable depth
  surface shared by the actor pass. The complete exact 26-level
  `levelTextures` database now selects the authored-order transparent queue
  through PDB-named `isTextureTransparent`; exact `isGlassTexture` selects
  the third no-depth-write queue. A separately labeled matcher resolves the
  shipped JPX mirror's collision-generated 8.3 IDs, while mirrors with no
  database correspondence retain `MATHEAD.listtype` fallback behavior. The
  transparency probe audits all 25 installed JPX archives and reports each
  material's resolved pass. The renderer has no window-system or desktop
  graphics dependency. The filled level path is two-sided: decompilation of
  `CD3DApplication::CreatePipelineStateObject` at RVA `0x315E0` establishes
  `D3D12_CULL_MODE_NONE`, and an opposite-winding JPX regression prevents the
  portable renderer from silently restoring backface rejection.
  Both the JPX and BMD perspective paths clip convex source faces against
  camera Z `1.0` before the perspective divide, interpolate UV/color at the
  resulting edge, triangulate the retained polygon, and reject wholly-behind
  faces. Focused straddle and fully-behind regressions cover both paths.
- Established that the matched 2024 PC executable's live world path is FBX,
  not JPX: its embedded parser reports exact source version 6001 (ufbx
  0.6.1), and `loader_LevelLoad` calls `ufbx_load_file`, then
  `_InitFBXLevelData`, `cube_NewWorldRender`, and
  `CD3DApplication::DrawLevel`. The legacy `InitJPX` and world-mesh renderer
  procedures have no live caller. JPX remains an explicit dependency-free
  geometry substitution because its shipped coordinates match the FBX data.
- Mapped the remaining live FBX pass boundary without adding an FBX library:
  `_InitFBXLevelData` uses exact PDB `isTextureTransparent` and
  `isGlassTexture` to split materials into opaque, transparent, and glass
  mesh vectors. Exact `DrawLevel`, `DrawLevelTransparent`, and
  `DrawLevelTransparentGlass` consume them with three pipeline states; the
  common level rasterizer disables culling and the glass state disables depth
  writes. These material group semantics now run
  over the dependency-light world renderer. The exact `cullmesh` global is
  recovered as PDB type `int[32]` with its executable initializer. Decompiling
  all three draw procedures corrected their non-obvious gate: Streets derives
  the slot from each FBX node name (`Broken=NN*2-1`, `Solid=NN*2`, base mesh
  zero). Outside Streets, only the opaque procedure uses vector position for
  vectors of at most 31 entries; transparent/glass draw unconditionally and
  larger opaque vectors bypass the array. Exact `cube_InitVisibility` is now
  implemented as well, including its 1,000-byte visibility reset and FED,
  Tatooine, and Streets branches. The shipped Streets mirror's 728
  nondegenerate wall triangles are associated with those named FBX owners in
  a sparse runtime map guarded by the exact JPX fingerprint. The software
  renderer now responds to live `cube_HideMesh`/`cube_ShowMesh` state without
  acquiring an FBX dependency; modified mirrors safely bypass the map.
- Reconstructed the exact PDB globals `sLevelNames`, `level_offset`,
  `level_scale`, and `startPos`, and extracted the complete
  `cube_NewWorldRender`/`LevelVertexShader.hlsl` authoring-to-game transform.
  All recognized JPX paths now render in fixed game units. The Streets
  start-position check maps FBX/JPX `{-120,-97,13}` exactly to
  `{30976,3328,-24576}` and visually places the posed model at human scale.
- Added a live PC scene loop that loads the original `STREETS.jpx`, consumes
  movement state through the recovered pad semantics, runs the recovered
  `camera_SetCameras` frame owner, and carries its `Camera` through the exact
  `sceneGeometryEnv` world-to-screen path and
  presents it through a thin Win32 adapter. The gameplay path now requires
  filled JPX pixels, visible list-type-8 transparency, and at least one
  successfully decoded shipped world texture while keeping wireframes as a
  separate diagnostic mode. The normal one-player FED path loads the exact
  256-entry `.cam` image, uses `intersec_FindWalkHeight` to recover its
  authored walk-polygon dolly index, and applies the normal
  `camera_SetCameraPos`/`camera_StuffCamera` selection, visibility, offset, and
  slack behavior. The previous parallel host camera builder has been removed.
  World and actors retain one shared depth surface; the JPX segment clip is
  only the unauthored fallback.
- Added real-asset PC integration smoke tests that fail when nonempty
  geometry produces no visible lines or pixels, or when requested authored
  motion fails to move the actor or enter attack `Motion[15]`. Release and
  Debug each pass all 497 tests, including 450 installed-real-asset gates,
  151 BMD model gates, 93 CAD layout gates, and 93 sequence-decoding gates.
- Recovered all 27 `collisn.c` procedures (3,987 procedure bytes) and exact
  152-byte x64 / 136-byte 32-bit `Mnode` layouts. The module covers the
  20-player by 32-node registry, reset and fallback behavior, flag mutation,
  rotations, velocity/translation, dynamic/virtual registration,
  `coll_4DCollision`, the hot-node driver, full node-vs-node contact, and full
  projectile-node contact. Projectile contact includes exact radius rules,
  `mReflects[5]`, normal hit publication, and saber/Force ricochet re-fire.
- Recovered the exact 24-byte `objectRoot`, 32-byte `Pad`, eight-byte
  `_mvector`, 20-byte `playerSettings`, and 568-byte x64 `playerObject`
  layouts. Native pointers deliberately compact the runtime-only player
  record for a later 32-bit target. The recovered player pool boundary now
  provides exact player-pad lookup, indexed player lookup, 20-object pool
  initialization, and requested-ID/first-free allocation with
  `WorldData.player0/player1` publication. Exact 323-byte
  `player_gCreateObject` additionally owns scene link 4, animation-motion and
  model-keyframe publication, player/type IDs, gameplay/Pad resets, and the
  typed character-initializer callback. The live PC player and all real enemy
  actors now use that owner in the original model/animation/component order;
  only the host's deliberately inactive single-player safety slot remains a
  raw pool allocation because it is not a constructed character. Exact
  `player_AfterLife` resets
  power type/expiry and item count, clears its lock ring, and frees its
  shadow sprite. Exact 1,904-byte `player_RefreshPlayer` now owns level-start,
  checkpoint, world-resume, and enemy-placement positioning; facing; walk
  height; score/Counter and energy/Force setup; shadow creation; player-state
  reset; idle queue publication; model visibility; and scene-state
  publication. Its exact PDB checkpoint globals and base-node marker path are
  restored. The reference's corrupting negative initialization start is the
  one explicitly bounded portable deviation.
- Added the exact pointer-free `CharacterData` (28 bytes),
  `JEDICOMBOMASK` (6 bytes), and `gamestruct` (4,716 bytes) PDB layouts.
  The reviewed `game.c` boundary now provides energy access/modification,
  player-specific clamping and life-line scaling, game-flag mutation, item
  clamping, power type/expiry state, and the exact authored combo/energy
  defaults. PDB-named `newGameGameInit` owns the original persistent-table,
  character-data, AI-bit, upgrade, completion, and two-model reset sequence;
  the PC runtime enters that owner instead of reproducing its defaults in
  host code. The level-eight player/enemy exceptions and timer-relative power
  expiry remain instruction-gated without a desktop or third-party
  dependency.
- Recovered the exact PDB `powerPoop` native-pointer layout, serialized
  12-byte `loadPoop` layout, two intrusive lists, 32-entry checkpoint table,
  initialized power-up tables, and PDB-named `pwrup_LoadPoop`, `pwrup_Init`,
  `pwrup_LevelStart`, `pwrup_LevelEnd`, `pwrup_JumpCheckPoint`,
  `cheat_nextCheckPoint`, `mrktng_GoToNextCheckpoint`, `fixPowColor`,
  `kmAudioSFX_DumpBank`, and 4,144-byte `pwrup_CheckPowerUps` procedures. The
  core dispatcher now owns exact list alternation, culling/pause gates,
  authored emitter timers, every collection award and audiovisual call,
  checkpoint/continue state, artifact flags, achievements, and after-life
  revival. The PC field resolves and loads its level `.pwr`, invokes the owner
  in original scene order, publishes the post-render list index, and its FED
  smoke reports `powerups=55/3/0`. All 52 installed power-up files have
  individual real-asset gates. The original immediate-mode `DrawPowerUp` mesh
  submission remains open behind its exact dependency-free call seam; the PC
  renderer currently displays a temporary world-space glint.
- Recovered exact `hurtplayer` (408 procedure bytes), including its death
  eligibility gates, game-state and object flags, `player_AfterLife`,
  hit/movement/model resets, and one-based tank-driver enemy cleanup.
  Exact `sound_Play` (131 procedure bytes) preserves the requested-bank,
  bank-3, and bank-0 fallback order—including the reference's duplicate
  terminal bank-0 attempt—over a game-owned portable `sound_playSfx` backend
  seam. `CheckCubeBlocking` now calls `hurtplayer` directly.
- Recovered the gameplay scheduler inside the 2,317-byte
  `player_gProcessPlayers` owner. It runs exact `player_DoCollisions` first,
  preserves the 20-slot stride, two/20 attacker budgets, two-player global
  bit, map-trigger handoff, controller/AI routing, independent Pad channel
  sampling, `mCharliePad`, level/pause suppression, and one exact
  `brain_ControlPlayer` call per eligible actor. The PC frame now calls this
  owner at its original post-render boundary instead of sampling, controlling,
  and colliding through separate host calls. Exact `_AddLifeTile`, `_DrawTile`,
  `_DrawTile2D`, `fRotTransPers`, the four scaled resource getters, and the
  interleaved damage indicator now restore the ordinary life/Force HUD. The
  PC adapter projects those game-authored rectangles from the exact published
  `CameraMatrix` into its dependency-free framebuffer; the real-asset radar
  gate separately proves HUD queuing and compositing. Developer debug text
  remains an explicit presentation-only remainder.
- Recovered the exact 152-byte x64 `sceneObject`, 504-byte x64
  `physicsObject`, and 24-byte `_jheightstuff` layouts with native-pointer
  compaction for the later 32-bit target. Forty-two `physics.c` routines
  (28,175 procedure bytes) now preserve the exact 20-object pool defaults,
  requested/first-free allocation, prior-solid heap cleanup, float-to-public
  movement conversion, scene-root position/constant-vector/facing access,
  animation-authored charge/acceleration and axis swapping, the
  `gSCENE_READY`/face-lock gate, scene-facing publication,
  `physics_gTurnToFace`, and animation-freeze-aware
  `physics_gTurnToAttack`. The latter uses the recovered exact PDB-named
  fixed-12 `flexmul` helper. Exact `scene_gGetSceneModelMatrixFV` and
  `physics_gSnapShotPosition` now preserve signed scene-position conversion,
  snapshot Y offset, the player/model flag gates, optional pelvis-center
  override, and scene-state republication used when a jump begins.
- Recovered exact file-local `CharBlocking` (791 procedure bytes) with all 23
  PDB-named parameters and locals. It preserves predicted-position and
  vertical-overlap tests, full and horizontal normalization boundaries,
  radius penetration, immovable-player and `INT16_MAX` mass rules,
  mass-weighted separation, the exact 48-unit per-character cap, and physics
  overlap flags. A bounded `MovePlayer` extraction now sweeps later object
  slots, publishes exact PDB global `maRange[20][20]`, and applies ordinary
  pairwise contacts once. Its large-character path now also preserves exact
  globals `maDesert_BNodeSizes` and `maWormNodeSizes`, their four/six
  executable-verified `CollisionData` records, node-center/flag gates,
  truncated vertical separation, horizontal penetration, and the original
  movement/overlap-state update.
- Recovered exact `newclosestPoly` and its exact `jon_getlibpartfloat`
  dependency. Exact integer `jon_getlibpart` now decodes the same thin-part
  stream into the original PDB-named `gaScratch` workspace, including the
  instruction-verified low-16-bit cube index and signed 16-bit coordinate
  stores. The dependency-free traversal now covers player clip planes,
  the Uber box, all 20 dynamic `_solid` objects, compressed 256-column world
  cube lists, fat polygons, thin library parts, packed 10-bit normals, and
  the reference best-contact/output-pointer rules. Synthetic gates exercise
  both a real compressed thin-map stream and a dynamic solid stream.
- Recovered the complete exact static-map walk-height chain:
  file-local `intersec_WankCheck`, global `jon_plumbline`, file-local
  `intersec_GeneralCheck`, and the three exact
  `intersec_FindWalkHeight` entry points (3,191 procedure bytes total).
  The reviewed code retains the original 32-bit wrapped edge tests, packed
  10-bit plane normals, fat/thin cube traversal, map-info output pointers,
  dynamic-solid priority, and height sentinels. Synthetic gates cover the
  direct polygon test, a compressed library cube, integer/FVECTOR entry
  points, and exact normal publication.
- Completed all 15 PDB-named `intersec.c` procedures (7,822/7,822 bytes).
  Exact `LineAndPlane`, `raycastpoly`, and `raycheckgeneral` feed
  `RaycastCheck`/`RaycastCheckSV` across compressed fat/thin map polygons and
  all 20 dynamic-solid slots, preserving nearest-hit type, cube/entry/poly
  outputs, packed normals, integer hit distances, and hitpoint publication.
  Exact `MoveObject`, `MoveObjectNormal`, `HitSomething`, and `BlowUp` now
  carry projectile/world motion through map mutation and authored map-effect
  dispatch. Synthetic gates cover map and solid rays, both movement wrappers,
  short-vector conversion, and map/solid normal publication.
- Recovered the instruction-reviewed `WorldBlocking` state core through its
  exact PDB signature and named `physicsObject`, `playerObject`, map, motion,
  and collision-node fields. Exact early-outs, blocked/unblocked publication,
  landing state, map-info restoration, air-ground tracking, and ground snap
  are covered by synthetic gates. `WorldBlocking` now reaches exact local
  `CheckCubeBlocking` and exact `brainutl_Land` directly. Its original inline
  splash sequence uses the exact file-local `splasheffects[30]`, walk-height
  query, `sprite_AddSpriteEffect`, and `sound_PlayFV`; the broad temporary
  world hook is removed.
- Recovered exact `brainutl_Land` (382 procedure bytes), retaining the
  fall-duration energy penalty, player-zero achievement completion,
  owner-type death count, landing motion/chain selection, airborne-flag
  clear, special ground delay, and animation calls. Exact
  `achievement_complete` retains the ID 2..43 platinum scan over a narrow
  portable platform-service boundary. Exact `sound_PlayFV` retains x64
  `CVTTSS2SI` position conversion and delegates to exact `sound_Play` bank
  fallback over the existing dependency-light audio boundary.
- Recovered the portable state core of exact local `CheckCubeBlocking`
  (RVA `0xDC5F0`, 4,566 bytes, 56 PDB-named locals). It now executes the
  reference height sentinel, up-to-four `newclosestPoly` loop, world/dynamic
  contact response, slide projection, nonmoving-contact and air-stick rules,
  ledge capture/solid-relative conversion, collision timers, damage gate,
  final height query, snap, and fallback publication. Gates cover no-contact,
  no-floor fallback, and a real dynamic `_solid` face-stream contact.
  `CheckCubeBlocking` now calls exact `intersec_FindWalkHeightFV` directly.
  Exact PDB `Sprite`, `SCB`, and `SControl` layouts plus
  `sprite_gHideSprite` now handle shadow hiding directly, and exact
  `hurtplayer` handles prolonged collision damage directly. Its recovered
  contact path now calls exact `HitsHit`, `LaunchMapAnimEffects`, and the
  recovered sprite-effect allocator directly.
- Recovered all five `extracharacters.c` procedures (278 procedure bytes),
  exact PDB `model_id` names, the 24-byte `ExtraCharacter` layout, the exact
  14-entry `ExtraCharacters` table, and `ExtraCharactersSize`. Capability
  defaults and mutable `Unlocked` state are gated. `CheckCubeBlocking` now
  calls exact `extracharacter_CanLedgeClimb` directly, removing that inferred
  dependency hook.
- Recovered the complete exact 240-byte x64 `wsl_ENEMY` PDB layout rather
  than retaining the anonymous `enemyFlags` offset used by the decompiler.
- Added exact PDB layouts for `FVECTOR4`, `CollisionData`, `_collide_info`,
  and `_movement_packet`. Exact original helpers `CalcRelativePosFromWorld`,
  `CalcSolidRelativePos`, `CalcWorldPosFromRelative`, and local
  `CalcWorldRelativePos` now preserve the solid's `Mnode.v3RotCenter`
  translation, rotation/scale transforms, relative facing, and flag
  transitions without anonymous offset names.
- Recovered exact `VectorNormalize`, `VectorNormalize2`, and
  `VectorNormalize3` from `flex.c`, retaining their float-sum/double-square-
  root boundaries, zero-length identity divisor, alias-safe stores, and
  length returns. Exact `CalcNewBox`, `buildfrustrum`, and `buildplane`
  establish the collision-frustum boundary from `gpWorld->start`, the
  original 460-pixel focal scale, percentage offsets, normalized planes,
  signed float-to-int truncation, and the instruction-verified
  `(0, 0, -4096)` fifth plane.
- Recovered the exact 240-byte x64 `_collidevars` scratch record with all
  PDB member names and native-pointer compaction for Xbox. The original
  module-local `planecheck` now preserves signed plane approach, penetration
  clamping, the best-distance gate, the complete `_collide_info` copy, and
  the assembly-verified promotion from contact type `1` to best-hit type `9`.
- Recovered the original module-local `sphereAndPoly` and
  `polycollidecheck` contact kernel. It retains polygon AABB rejection,
  face-approach gating, face/edge/point swept-sphere contacts, initial
  penetration types, integer side-mask classification, double-precision
  square-root boundaries, contact priority, and exact type-2 edge endpoint
  publication. Synthetic tests cover face, overlap, edge, point, back-face,
  broad-phase rejection, and best-hit selection.
- Recovered original module-local `generalCollide`, connecting `_solid`
  vertex/index/normal streams to the swept-sphere kernel. It preserves
  triangle sentinels, quad handling, exact 1/4096 fixed-normal conversion,
  the `solidhack` reverse-normal path, contact priority, and type-2 edge
  publication. A synthetic `_solid` regression exercises both the forward
  face hit and back-face rejection through the portable pointer registry.
- Recovered the exact pointer-free 100-byte `Motion` and 560-byte
  `_animFrame` layouts, plus the pointer-bearing 64-byte `_dpcontext`,
  40-byte `animListNode`, 2,496-byte x64 `animObject`, and 104-byte x64
  `modelObject` layouts. `anim_InitAnimations` now reconstructs the exact
  20-object/twenty-by-eight queue-node pool, while `anim_CheckFreeze`
  preserves the frame-window gate consumed by movement.
  `anim_AddNextAnimSeq` now reconstructs the exact fixed-pool motion enqueue,
  including target animation contexts, replacement, duplicate suppression,
  tween/speed/lock fields, and exhaustion. The extracted motion-to-physics
  block from
  `anim_ForceNextAnimSeq` now applies `Motion.vel + Motion.Charge`,
  clamps negative `ChargeAcc` to zero through `physics_gSetCharge`, and
  preserves the motion-flag axis swap. This is the authentic ground-motion
  producer feeding `CalcMovement`; direct stick-to-velocity mapping is not
  used.
- Recovered the complete seven-procedure `unpack.c` module: exact
  bit-reservoir refill, direct/cached/tree Huffman lookup, raw and compressed
  vector decoding, table binding, context initialization, and seek behavior.
  A dependency-free companion-table loader checks exact file sizes and every
  direct-value/tree reference before activation. `anim_CreateObject` attaches
  the final 100-byte-per-entry `Motion` table to the scene animation. A
  bounded, zero-copy CAD inspector/loader plus real-data decoder accumulates
  all 150,272 authored frames across 7,603 sequences in all 93 installed
  archives. The corpus also establishes the original rounded-down
  sequence seek and terminal depack-window overlap with following `Motion`
  records. The PDB-named local `anim_GetAnimFrame` now preserves exclusive
  `Lframe`, double buffering,
  raw-frame reset, first and later compressed-delta accumulation, 12-bit
  joint-angle wrapping, 13-bit root-Y wrapping, and event-byte publication.
  The activation subset's initial fixed-rate product is correctly stored in
  `animFrameAcc` at PDB offset `0x514`. PDB-local `anim_CreateTweenFrame` is
  now reconstructed with the exact 17-entry fixed-point fraction table,
  16-frame clamp, wrapped signed deltas, root/joint pose publication, and
  frame countdown. Exact local `anim_SkipToStartFrame` consumes authored
  pre-roll poses without publishing their event bytes, and target-relative
  motions select the target animation's secondary depack context. Exact
  `anim_GetAnimFrame` now forms its initial publication count from
  `Motion.cutin` at offset `+0x0A`; an instruction audit proved that the
  formerly used `Motion.disp` at `+0x0C` belongs only to `anim_CheckSlack`.
  The correction affects 1,048 records across 61 of 93 shipped CAD files and
  is protected by an unequal-field unit fixture. Exact
  global `anim_ProcessAnimations` now owns the live 20-slot PC frame pass,
  after player and enemy callbacks have selected motions. A real FED run
  records two live tween publications in its first eight frames. Exact
  672-byte `anim_ForceNextAnimSeq` now owns forced queue activation from all
  six reviewed `animctrl.c` wrappers and player reset, including prior-motion
  flag propagation, recovery, frame-rate/physics setup, callbacks, sounds,
  tween selection, and immediate frame publication. All 22 `animutil.c`
  procedures are reviewed. Exact
  local `anim_SoundStart` and `anim_HandleSound`, plus global
  `shouldPlayAnimSound`/`shouldReplayAnimSound`, now preserve both immediate
  and delayed channels, signed loop delays, bank choice/fallback, stop
  markers, and the authored tank/taxi vetoes through the dependency-light
  sound hook. Exact `console_AnimCommand` restores the play/chain and effect
  mutation commands. Exact `anim_GlobalInit` loads the three Huffman files
  through the reviewed `resource_getPath` owner; SDL base-path discovery and
  loading delays are isolated behind dependency-light host seams. The
  resource owner preserves all 32 recovered directory mappings and the
  PDB-named 256-byte publication buffer.
- Recovered the authored-pose publication block at the front of exact PDB
  procedure `render_RenderNode`. The descriptive portable boundary
  `jpb_ModelApplyAnimFrame` recursively publishes authored x/y/z joint angles
  to contiguous `Mnode` children, zeroes static/virtual nodes, preserves
  16-bit padding, and implements the absolute-rotation override and dirty-bit
  clear. The scene-aware boundary also preserves the exact authored event
  mask and promotes hot node bit `0x1` to scene attack flag `0x10`, closing
  the original producer/consumer edge into `player_DoCollisions`. The PC path
  publishes every decoded frame through exact
  `scene_gSetSceneModelKeyFrame` and applies the pose to the loaded actor
  `modelObject.pRootNode`.
- Recovered the exact pointer-free 144-byte `geomData` PDB layout used by BMD
  model archives, plus the 4,976-byte x64 `modelSpace` and 16-byte x64
  `TextureTracker` layouts. The live player, enemy, and BMD-probe paths now
  call the exact PDB-named `model_gInitModelRoot`/`model_MakeNode` owner over
  its 20 registered models and 32 nodes per model. It preserves record 1 root
  selection, name reuse, contiguous immediate-child allocation,
  translation/ID publication, collision-node registration, fixed `0x2000`
  scale, recursive child-index traversal, all five exact `addPtr` stream
  relocations, saber substitution to `transabr.bmp`, and texture-packer
  state. The bounded renderer and physics views consume the resulting exact
  `getPtr` indices. Fourteen of the module's 15 PDB-named entries now have
  reviewed bodies; only debug-only `console_NodeCommand` remains. All 151
  installed BMDs have gates. The corpus confirms payload-end offset sentinels
  and `worm.bmd`'s eighth child in the word following the PDB-declared
  seven-entry array; `skin2.bmd` is explicitly gated as a shipped file 720
  bytes shorter than its declared payload.
- Completed all six `CalcMovement` movement modes across RVAs
  `0xDAC60..0xDBC84`: `MOVE_NORMAL`, `MOVE_HOVER`, `MOVE_HOVER3D`,
  `MOVE_FLY`, `MOVE_BLOWN`, and `MOVE_COREDEATH`. The reviewed extraction
  preserves the entry processed guard, exact recursive standee scheduling,
  charge decay, animation freeze gating,
  `currentmov`/`mov` staging, airborne gravity, reverse-air steering and
  expiry, facing rotation, model scale, slope-normal forcing, directional
  floor conveyors, landing-flag publication, map-normal impulse decoding,
  both blown-motion phases, and the exact `0xA000` exit back to airborne
  normal control. Hover recovery adds the exact 90/120-unit height band,
  damping, ascent and Streets-specific descent caps, yaw-relative turn-phase
  motion, and target-relative 3D/fly steering. Core-death recovery adds its
  map trigger, return-to-origin drift, rising phase, loop-sound shutdown,
  energy/game-state teardown, exact `afterLife` publication, AI exit, and
  return to normal movement. The inferred `jpb_TrajectoryCallbackSlot` and
  `jpb_CubeRuntimeFlags` names preserve anonymous runtime state without
  leaking decompiler placeholders; the exact `sound_StopSound` entry uses a
  dependency-light backend hook suitable for PC and nxdk. The processed bit
  breaks authentic standee cycles while ensuring every movement-mode platform
  is evaluated once before its rider; the platform's world delta takes
  precedence over conveyor motion exactly as in the executable. The exact
  global `MovePlayer` entry is now complete across RVAs
  `0xDDBB0..0xDE649`. It joins the ordinary and desert-beast/worm character
  sweeps to `WorldBlocking`, preserves the normal fall transition through
  motion 4 and `brain_SetFallTrajectory`/`brain_SetTrajectory`, and covers
  direct fly/core-death commits, inactive/special players, world and
  moving-solid ledge grabs, ground snapping, map-contact force 12,
  relative-platform publication, and final landing/temporary flag state.
  The anonymous callback slots at RVAs `0x10EFBB0` and `0x10EFD00` have
  documented portable names tied to the exact PDB procedures
  `brainutil_PlotTrajectory` and `brainutil_PlotMaulTrajectory`.
  The exact `UpdateSceneObject` entry at RVAs `0xDEAB0..0xDEBA4` now
  publishes integer and scene transforms, preserves target-facing and the
  body-versus-face angle selection, and advances `validairground` with the
  executable's 512-unit threshold. Exact file-local `checkdriving` mirrors
  vehicle position, angle, movement, and map contact state into active player
  slots through a validated portable facade. Exact
  `ApplyMatrixMany10Bit`, file-local `BuildNodeVertexList`, and file-local
  `BuildSolids` now decode and transform packed BMD collision geometry,
  construct nearby-enemy masks, and preserve the original allocation and
  aliasing ownership of `_solid.coords`/`normals`. The PDB-visible
  `VECTOR*`/`_svector*` node-rotation mismatch is retained through defined
  unaligned loads rather than an opaque cast.
  Exact `cliptofrustrum`, `PushMatrix`, and `PopMatrix` complete the remaining
  dependency chain for the 953-byte `ProcessPhysicsObjects` frame entry.
  The scheduler now preserves the executable's active-object predicates and
  pass ordering: clipping, matrix save, moving solids, camera-dependent
  collision box, flag reset, optional AI-node refresh, movement calculation,
  pause/game-flag-gated position commits, driver synchronization, scene and
  `WorldData` publication, matrix restore, and the Streets countdown teardown.
  Its type-zero component flags, both energy clears, game-state bits
  `0x20`/`0x40`, physics masks, and the otherwise surprising zero of
  `maRange[1][4..5]` are covered by a complete-graph scheduler test.
  The real STREETS PC loop now calls this frame entry directly. Exact
  515-byte `twatcameramatrix` supplies the original axis swaps, translation
  offsets, transpose, and sign-bit changes expected by `buildfrustrum`.
  The real-asset smoke proves collision-aware scheduler movement from
  `(30976, 3328, -24576)` to approximately
  `(30979.8, 3328.0, -24555.3)` over two frames.
  `CheckCubeBlocking` now also contains the missing level-eight trigger that
  starts this countdown. It retains the exact 0.9 positive-X wall-normal
  shortcut, `0xf000`/`0x1e000` collision timeouts, one/two-player
  `stapbikeindex` checks, effect 18, `explomed` audio, STAP sound shutdown,
  scene disable, and reset ordering before publishing `0x13800`.
  Exact `brainutl_ElapsedTime`, 596-byte `combo_ResetComboEngine`, and
  119-byte `player_ResetJedi` recover its small named reset dependencies.
  The complete 1,574-byte `physics_ResetJedi` now preserves all common reset
  writes and the instruction-verified Level 10 behavior: one-player slots
  clear userdata, while two-player slots with player ID `0x4C` retain it.
  The executable's otherwise-unused player-count cases are retained as well.
- Recovered all fifteen exact `brain.c` procedures
  (7,081 of 7,081 procedure bytes). In addition to effect dispatch,
  both ring effects, `brain_GroundControl`, and the trajectory leaves,
  the reviewed boundary now includes exact `brain_HangCallback`,
  `brain_LockOn`, `brain_SkidCallBack`, `brain_TakeOff`,
  `brain_ThrowEnder`, and `brain_ValidateLockOn`. The effect and lock-on
  procedures preserve per-bit Motion sound dispatch, nearest valid target
  selection, input-map toggling, ring creation/update/teardown, target
  facing, distance/energy invalidation, and lock audio through exact
  `brainutl_gGetNearestTarget`, `physics_gGetRange`,
  `sprite_AddSpriteEffectAtNode`, `sprite_gUnHideSprite`,
  `brainutl_FindLSB_LV`, `brainutl_PlayMotionSound`, and
  `jedi_GetColour`. `brain_GroundControl` preserves energy-driven
  death scheduling, current-animation delay for NPCs, Motion 14 recovery,
  unsigned timer clamping, player afterlife and loop-sound shutdown, game
  exit flags, and the NPC `exit_flag`. The trajectory procedures preserve
  character-specific fall selection, stand/running jump choice, the
  reference 12-bit trigonometric multiplication and truncation, air timer and
  player-flag updates, and the motion-axis callback. Hang, skid, takeoff, and
  throw-end handling retains collision-node release, frame thresholds,
  node-attached effects, Motion 4/51 activation, snapshot/trajectory setup,
  and exact delays. The 3,954-byte `brain_ControlPlayer` PDB procedure is
  now reviewed in a separate link-isolated object, with a portable optional
  provider replacing only its SDL-specific five-key cheat query. Its
  ordinary directional branch preserves the
  exact `atan2f(g_p1X, g_p1Y)` conversion, camera-relative 12-bit facing,
  ordinary `physics_gTurnToFace` or lock-on `physics_gTurnToAttack`,
  `Motion[1]` lock activation, and player velocity rewrite. The
  assembly-checked stationary branch now selects normal,
  low-energy, and lock-on idles (`Motion[0]`, `[19]`, and `[20]`), clears
  movement and stale action flags, resets `runCounter`, and preserves the
  delayed `Motion[2]` to `Motion[25]` run-stop exit. The omnidirectional
  branch now preserves wrapped-facing selection of `Motion[26]` or reverse
  `Motion[8]`, including exact speed, freeze-window, tween, facing, and
  equal-lock stores. The lock-on directional branch now preserves its exact
  wrapped angle windows for forward/left/right/rear Motions
  `[26]`/`[29]`/`[30]`/`[12]`, per-motion flag and tween edits, target-facing
  call, equal-lock activation, and 16-bit `runCounter` update. Its newly
  recovered exact dependencies `physics_gFaceTarget`,
  `physics_gForceFaceTarget`, and `physics_gGetFaceTargetDelta` use the
  PDB-named player/target position locals and original `ratan2` boundary.
  The successful jump-launch block now preserves the map material `0x4000`
  veto, `Motion[4]` lock, exact jump and air-trajectory producers, callback
  assignment, 60-unit snapshot, and `reversoi` clear. Its descriptive
  `jpb_BrainJumpLaunchState` boundary accepts the runtime callback-table value
  explicitly: the original slot is initialized to exact PDB
  `brainutil_PlotTrajectory` and remains replaceable by the mod-extension
  layer rather than being hard-wired to a PC-only global.
  Exact `brainutil_PlotTrajectory` now owns the authored ordinary-air
  continuation, camera-relative steering, double jump, frame timing, long-air
  recovery, and fatal-fall path. Exact `brainutil_PlotMaulTrajectory`
  preserves the companion Maul continuation. The complete 559-byte PDB-named
  `jedi_InitPlayer` now owns live PC player construction: it selects the exact
  character collision table, model scale, built-in `combos1`/`combos2` data,
  movement settings, and `jedi_Main` callback. All eight initialized collision
  tables and both 48-record combo globals retain their executable values in
  readable source. The adjacent exact `jedi_HasProgression`, `jedi_IsMelee`,
  and 263-byte `jedi_Main` bodies restore progression classification and the
  per-frame Force/node cleanup path. An external authored CMB still replaces
  the built-in combo pointer through the exact existing load path. Space or
  headless jump input consequently launches the actor vertically on the live
  field without the retired AI-initializer bridge.
  The special direction handoff also preserves its
  motion-2/motion-60 gate, exact `animutl_SetCurrentLock(..., 15)` call, and
  return through the reviewed idle/run-stop selection. The alternate jump
  launch now preserves its material veto, facing quantization, lock-level-30
  Motion 4 activation, trajectory callback, and snapshot. The attack tail
  preserves Motion 15/21 chaining and the Motion 2/25 run-stop alternative.
  Exact `enemy_KillKill`, `combo_CheckHeldPad`, and
  `sprite_GetCommentsSprite` now supply the parent enemy-exit, held-input,
  and power-battle message paths. The exact combo, force-activation, and
  complete twelve-procedure `braindmg` owners now replace those direct
  test substitutes in `brain_ControlPlayer`. The live PC FED field now calls
  that production controller directly, initializes its authentic idle
  motion before the first effect pass, publishes held input on `cpad[1]`,
  selects authored `Motion[2]`, and advances the actor through exact
  `ProcessPhysicsObjects`.
- Recovered `scene_gGetNewSceneObject` and complete `scene_gInitScenes`,
  including the three developer-console registrations and their already
  reviewed camera, animation, and enemy callbacks. `obj_gSetChildObject` now
  preserves the original
  parent-if-unset rule and subsystem-slot mapping, while exact
  `obj_gSetObjectFlag` mutates the selected component through the same
  mapping. The live PC game uses
  these routines to own and link real actor, scene, model, physics,
  animation, and player records without adding a third-party physics or
  rendering dependency. The collision-backed PC loop derives and relocates
  the matching original W3D archive, owns its
  `WorldData` and Jonny collision storage, and runs the reviewed
  `CalcMovement` plus exact `MovePlayer` path each frame. The active-controller
  smoke now uses FED because exact `brain_ControlPlayer` intentionally exits
  on `GameStruct.CurrentLevel == 8`, the STREETS special case. With
  `obi_wan.cad`, FED selects authored `Motion[2]`, publishes decoded frames,
  applies authored velocity, publishes the current frame to the scene, and
  verifies a changed world position.
- Extended that PC game path through real authored combat. The host now owns
  a battle-droid player/model/animation record in the original enemy object
  range, targets it from Obi-Wan, runs the exact common actor controller,
  detects the authored Motion 106 hot-frame node contact, applies the exact
  difficulty-scaled damage path, and selects the battle droid's authored
  airborne hit reaction. The corresponding headless combat gate validates
  contact, energy loss, damage consumption, and reaction state without
  producing routine screenshots. A portable bounds guard preserves the
  reference behavior of `braindmg_ResetDamageTracker` without relying on the
  original executable's inert BSS padding after `damageTracking[2]`.
  The droid also owns its authored `battle_d.01` WAI payload selected from
  the placement's AI level. Exact `ai_GetAIHandle`, `ai_GetAiDataValue`,
  `ai_GetAiDataValueN`, `ai_GetAiSeqValue`, and no-op `ai_Main` now retain
  their PDB names; a documented callback-signature adapter gives callback
  slot 33 a deterministic portable return. The gameplay gates require that
  this main-AI owner executes before damage/reaction callbacks take over.
- Replaced the remaining anonymous enemy-tree records at this boundary with
  the exact PDB `UDATA`, `BAP_AINODE`, `wsl_BAP_WAYPOINT`, and `kfNode`
  layouts. Reviewed enemy support now includes `_countChildNodes`,
  `enemy_GetNodePointer`, all four small BAP traversal procedures, both
  AI-mode stack operations, script integer/float arithmetic and comparisons,
  global/timed flag management, waypoint selection and bounds,
  `bapenemy_postFrame`, `enemy_ActivateEnemy`, and exact near/far player
  selection.
- Reconstructed the complete seven-procedure, 2,404-byte `shaolin.c` module.
  Exact `shaolin_Attack` and `shaolin_DoKungfu` now own attacker budgeting,
  player-relative formation points, low-chi scheduling, HTH delay/rate
  scaling, WAI attack and combo selection, group positioning, target
  switching, authored motion locks, and jump handoff. The adjacent exact
  `ai_HthAttack`, `ai_RangedAttack`, `ai_SeqAttack`, `ai_SetTarget`,
  `ai_WalkToPoint`, and `ai_WalktoPlayer` procedures restore the opcode-facing
  attack and movement owners with their PDB names and locals. Expanded
  Shaolin tests cover scheduler lifecycle, walking/reached paths, direct and
  WAI-selected attacks, ranged reload scaling, queued sequences, and
  coordinator selection.
- Connected the exact Shaolin scheduler to the live PC battle droid. The
  runtime now preserves a valid inactive player in reserved object slot 1,
  matching the invariant assumed by the exact two-player target selectors.
  The earlier direct zero-move HTH registration has now been removed in favor
  of the authored BAP traversal described below.
- Extended `jpb_asset_probe` with bounded BAP-tree inspection. It can dump an
  authored AI record with every resolved variable, parent/child/sibling links,
  placement linkage, and resolved attack arguments, or find a decoded opcode
  plus up to four resolved operands across every AI record in a shipped J3D
  archive. `--opcode-summary` provides a per-archive frequency table. The
  complete 27-archive audit found 37 decoded values across 42,581 nodes:
  7,246 structural zero/one records and 35,335 executable records. It gives
  parser work an asset-backed opcode map rather than a guessed dispatch table.
- Recovered the exact 904-byte `aisub_handleMoveFunction`, 960-byte
  `aisub_handleRangeFunction`, 496-byte `aisub_handleScanFunction`, 835-byte
  `ai_WalkWayPoints`, and 146-byte `physics_FindNearestEnemy` procedures.
  Focused fixtures cover authored waypoint movement, explicit and extension
  range tests, scan/facing behavior, and owner-filtered nearest-enemy search.
- Recovered the complete PDB-named `enemy_ParseOpcodes` valid-data call
  surface and retained `jpb_enemy_ParseOpcodes` as its bounded diagnostic
  companion. The traversal implements all 35 executable
  top-level opcode values present in shipped archives. This pass added exact
  scan routing (`0x100`), counter/random/cyclic branch selection (`0x105`),
  camera-dolly bit control (`0x20c`), movement speed/mode changes
  (`0x408`/`0x409`), range/exit and taxi state (`0x40c`), letterbox/camera
  override (`0x604`), global-bit mutation (`0x610`), and the original
  extra-character gate for motion 84. The large `0x606` family now handles
  all 22 shipped subcommand IDs and all 530 occurrences: teleport setup,
  player flag control, screen shake, authored motion IDs, camera override,
  nearby-enemy removal, physics flag toggling, `uberPos`/range/`uberLock`
  publication, facing, player-camera flag release, power-up rate/control,
  effect/audio dispatch, streaming-music selection, destructible-map events,
  live-player replacement, special-menu messages, and command 11's exact
  `level_SparkRoom` hazard. Exact tank/STAP entry through `0x607` and
  live-player attach/detach through owner-type-4/5 `0x60f` are recovered.
  Command 18 and owner-type-4/5 now execute their matched dead/after-life player
  rebuild through exact `player_RefreshPlayer`, preserve Force, score,
  Counter, and current-position state as appropriate, restore shadow
  ownership, and return the camera to type zero in the original two-player
  branches. Immediate and relocated operands, child/sibling branch selection,
  and mode jumps follow the matched machine code. The final `0x411` no-op and
  the original unknown-opcode debug reset/continue behavior are represented;
  diagnostic runs still report unknown data and bound malformed cycles.
- Corrected the FED intro's live-player handoff with direct executable and
  PDB-layout evidence. Matched `enemy_ParseOpcodes` RVAs `0x4A660..0x4A670`
  read placement offset `0x34` (`wsl_BAPAI_DEFAULTS.ownerType`) for the
  owner-4/5 P1/P2 branch, not mutable placement offset `0xC8` (`status`). The
  portable cutscene-motion adapter now also preserves the animation-owned
  motion for a temporarily BAP-driven world player, allowing FED AI 32 to
  turn toward its exit waypoint. The real FED archive regression proves P1
  attaches to placement 151, reaches the authored exit, releases the AI and
  control lock, and hands the camera back without spawning a battle-droid
  substitute.
- Recovered exact PDB-named `playXA`, its 103-entry authored stream-name table,
  `menu_specialMess`, `BlockBuster`, `BigBlowMe`, and `anim_ResetJedi`.
  All nine `win_audioStream.c` procedures now retain their exact names, and
  dependency-free play/control hooks preserve portable music and menu realization
  without moving the recovered game-state and dispatch behavior out of the
  original source modules. Focused tests cover music selection and gating,
  destructive-map propagation, live-player reset, menu IDs, tank entry, and
  owner-type-4/5 player attach/detach.
- Recovered exact PDB-named `level_SparkRoom`, `zapcheck`,
  `vecpointlinesquared`, and `PlotZap`. The authored five-arc room hazard now
  retains its reset state, pulse timing, player-line damage and reactions,
  small-spark sound transition, exact colors, and glow layering. This closes
  the last wholly unsupported shipped `0x606` command.
- Restored exact PDB global name `uberLock` at RVA `0x4F1AC0` after the global
  inventory resolved the earlier inferred placeholder. Exact no-op leaves
  `game_clearLetterBox` and `game_setLetterBox` now retain their PDB names at
  the `0x604` call surface.
- Replaced the live PC runtime's detached BAP-parser call with the normal
  active-enemy owner. `jpb_enemy_ProcessActiveFrame` now brackets exact
  `shaolin_StartKungfu`/`shaolin_DoKungfu` while also owning placement
  activation, exact pre/post state transfer, double-buffered active lists,
  range retention, and despawn. Exact `ai_DefendCheck`,
  `bapenemy_preFrame`, `bapenemy_postFrame`, `_deleteEnemy`,
  `player_FreePlayer`, and `obj_gClearObject` preserve the normal block gate,
  energy/location publication, linked-placement activation, animation-loop
  shutdown, target repair, and component cleanup. Real-asset gameplay gates
  execute the authored FED AI 12 tree through this owner without a parser
  boundary. The shipped near-start placement walks, waits, looks, checks
  global flag 111, and uses linked-placement opcode `0x60f`; it is not
  presented as an HTH tree.
- Extended that owner through the matched `enemy_HandleEnemies` frame
  preamble and special-level stores. It now preserves exact elapsed-process
  timing, timed AI-flag restoration, tank countdowns, debug-enemy selection,
  level-3/9 achievement/score/counter changes, deferred next-level state,
  point-sprite events, and the level-13 player-count bit. The level-6 enemy
  `0x75` deactivation-range override and level-7 enemy `0x3a` physics-Z clamp
  to `-14800` are instruction- and data-backed. Exact PDB-named
  `game_ModGameCounter`, `enemy_ResetEnemies`, and `enemy_SetTeleport` now
  own their counter wrap, active-list/placement/global-bit reset, and
  teleport-state behavior. Focused tests cover both level overrides, the
  preserved global-bit interval, and the level-9 saved-position exception.
- Completed the remaining `enemy_HandleEnemies` debug presentation path. Its
  exact `DebugLevel == 3` gate calls the reviewed 947-byte `enemy_Radar`;
  authored 640x480-relative scaling, black translucent background, centered
  white player marker, camera-relative owner-type 2/3 markers, range filter,
  red/green colors, and draw order all retain their matched constants and
  PDB-named `_DrawTexture` surface. The PC runtime queues that platform draw
  seam and alpha-composites its solid rectangles after world/player/enemy
  rendering, keeping the implementation dependency-light.
- Filled the final four procedure shells in the 48-procedure `enemy.c`
  inventory. Exact `aisub_findNearestWaypnt` selects the strictly nearest
  authored waypoint inside the original 8,192-unit ceiling.
  `enemy_CalcPoints` restores the shipped positive-value scoring pass and the
  exact 80x3 `maModelID` per-actor accounting table. `console_EnemyCommand`
  restores case-insensitive global-bit mutation and authored placement
  coordinate/activation stores. The 1,008-byte `enemy_CheckTeleport` now
  consumes deferred teleport state, shifts in-range enemies and the correct
  active player set, republishes scene transforms, applies level-8/9 special
  offsets, selects the original camera mode, and clears its one-shot state.
  The dependency-light PC frame invokes it at the matched end-of-frame
  boundary; a real-asset gate proves the deferred 64-unit offset is applied.
- Reconstructed the exact placement-spawn owner around that leaf layer.
  `_addEnemy` retains all four PDB-named parameters (including optimized-out
  `forceon`), the global-bit veto, authored/override AI selection, fixed-pool
  rollback, active-list publication, and placement flag stores.
  `_checkForNewEnemies` retains the authored `aRange` axis-aligned activation
  cube, zero-range immediate spawn, placement status transition, and the
  level-15 placement 24/27/29 override. Exact `enemy_HandleMapTriggers`
  resolves both direct and leveldata-relative cube records into deferred
  placement activation, while exact `enemy_getPointerIndex` preserves its
  level- and enemy-ID-specific archive-index adjustment. The underlying
  1,114-byte `loader_CreateEnemy` model/animation constructor still crosses
  unrecovered object-loader code, so a single dependency-free `jpb_` provider
  marks that boundary rather than importing a host graphics framework or
  pretending the constructor is complete. The live PC runtime now binds its
  portable BMD/CAD/WAI actor owner to that provider; its battle droid reaches
  exact `_addEnemy`, including placement state and active-list publication,
  instead of bypassing the recovered owner with manual pool/list operations.
  Asset loading itself no longer preselects and force-spawns the nearest
  placement: the first simulation frame reaches exact
  `enemy_HandleEnemies` -> `_checkForNewEnemies`, so only authored
  `activeFlags`/`aRange` placements enter the active list.
- Removed the PC runtime's remaining single-enemy and single-class host
  ownership. The persistent loader provider now resolves FED actor names
  against the matched executable's BAF/model/animation tables at RVAs
  `0x4BD2B0`, `0x4BCF10`, and `0x4BD730`. It connects `baron` to model 17
  `battle_d`/`battle_d`, `21b` to model 15 `pilot`/`pilot_d`, and
  `baronsec` to model 62 `security`/`battle_d`, `r3po` to model 12
  `protocol`/`droid`, `destroyr` to model 26 `destroye`/`destroye`,
  `hovdroid` to model 30 `loader`/`loader`, `drdfitr` to model 47
  `droid_f`/`droid_f`, `pwrdrink` to model 72 `beacon`/`beacon`,
  `pwrserv1` to model 86 `fed_door`/`fedship`, `reeyees` to model 87
  `piston`/`fedship`, and `twilek1`/`twilek2` to models 94/95
  `lift_1`/`lift_2` with `fedship`.
  Every active placement gets
  independent scene/model/physics/animation/player state in original object
  slots 2..19, while immutable BMD/CAD, textures, and per-level WAI storage
  are shared only within its class. Exact loader behavior registers WAI by
  the level-local actor slot; using the model ID would be both incorrect and
  out of range for security's model ID 62. The frame loop advances the exact
  active-enemy owner once, then controls, decodes, class-renders with shared
  world depth, checks hot-node contact, despawns, and reuses each record.
  Enemy asset creation does not retarget the player: the original valid,
  inactive player-1 slot remains the one-player target until recovered
  collision/combat behavior selects a live opponent. The runtime's descriptive
  primary-enemy view follows that selected live target when one exists and
  otherwise exposes the first surviving actor without mutating gameplay state.
  Damage, energy-minimum, motion-reaction, and recoil diagnostics accumulate
  across every processed enemy, so a hit on a non-primary actor cannot be
  hidden by later publication of another actor. The FED
  real-asset gate loads all 12 mapped classes and identifies 11 with authored
  enemy placements (`twilek2` has valid actor/assets records but no placement).
  Its nine-frame traversal proves an 18-actor peak, 30 authored spawns, seven
  classes active together, and all 11 placement-backed classes activated and
  rendered through exact `_addEnemy`.
  `jpb_AnimResetObjectSlot` isolates safe per-object animation-pool reuse; a
  focused test proves the neighboring slot is unchanged. All FED actor-table
  classes are now connected.
- Recovered the complete 3,822-byte PDB-named `ai_InitPlayer` and made it the
  live PC player/enemy settings owner. All 62 model-ID specializations plus
  the default path now preserve their exact normal/asymmetric scales, clip
  radii, closing distances, mass/height pairs, movement overrides, callback
  indices, collision profiles, and model/player/physics/root flag mutations.
  The common tail stores `minClosingDist` before publishing the physics
  radius, restores the exact fallback `combos` block, and writes the seven
  shared movement words. All 21 selected collision tables are checked by
  hashes of the matched executable bytes; that audit corrected the older,
  incorrect `maDesert_BNodeSizes` and `maWormNodeSizes` initializers. Exact
  three-byte `ai_InitModelData` is the matched empty owner, completing both
  emitted `settings.c` procedures. Model 26 reaches exact six-byte
  `ai_Destroyer`, while model 30 reaches the complete
  1,143-byte PDB-named `ai_LoaderDroid` now preserves its readable
  `box`, `arms`, `count`, `toss`, and `zeroBSSCheck` state, arm-node effects
  and detachment, target grab/throw timing, and reset behavior. Exact
  `ai_ShowFlags` and the matched release build's inert `debug_printf`
  dependency are restored as named source. Focused settings/callback tests
  and the authored Loader Droid field phase cover the branch. Six more
  settings-selected callback slots now point to exact PDB-named bodies:
  `ai_Blades` (36), `ai_Worm` (39), `ai_Krakis` (41), `ai_Mtt` (43),
  `ai_AAT` (45), and `ai_Deadly` (47). Their reviewed code restores blade
  rotation spin-up, collision-sphere diagnostics, the MTT's 512-unit
  two-player damage volume, AAT wrapped turret tracking/projectile cooldown,
  and the deadly-force flag. Exact `debug_drawsphere` arguments cross an
  optional dependency-free renderer hook while its unhooked release behavior
  remains inert.
- Recovered seven more exact `boss.c`/`vehicle.c` procedures and their six
  authored callback stores: `maul_PushCallBack` (27),
  `maul_RingCallBack` (28), `maul_ZapCallBack` (29), `ai_Thug` (35),
  `ai_Maul` (37), and `ai_JarJar` (38), plus the PDB-named `centreturret`
  leaf. The Maul paths retain the exact frame windows, projectile type 14,
  768-unit raycast, zap colors, player-line damage publication, Tatooine
  timer, motion callback IDs, and lock engagement. Thug owns its effect-61
  shield sprite array through the original heap and Force-reflect flag;
  Jar Jar owns the level-13 global-bit publication and camera transition.
  The remaining 1,977-byte `ai_Kadu` and 2,927-byte `ai_TurretDroid` are now
  recovered as readable source, completing all 18 `boss.c` procedures
  (11,000/11,000 bytes). Kaadu callback slot 34 preserves rider mounting,
  race-input cadence, speed/animation coupling, camera focus, and authored
  bars. Turret Droid slot 46 preserves event-node projectiles, aimed zap
  raycasts, shield sprite ownership, strafe fire, head tracking, and both
  detachable-arm state machines. The remaining PDB-named `ai_Stap` and
  `ai_Tank` bodies are now readable source, completing all six `vehicle.c`
  procedures (6,860/6,860 bytes). STAP preserves its two-rider input,
  lean/steer/speed coupling, paired machine guns, race catch-up, camera, and
  looped-sound state. The tank preserves one/two-driver ownership, dismount,
  tracked motion, turret and machine-gun aiming, four projectile paths,
  recoil/cooldowns, node publication, and engine/turret sound state. Exact
  428-byte `FindBestMachineGunTarget` supplies the shared actor, energy,
  height, range, and wrapped-facing selection.
- Recovered the adjacent 1,730-byte ground, target, physics-object, and
  player-contact cluster under its original PDB names. Exact
  `physics_InitPhysics` resets the complete 20-by-20 range cache;
  `physics_GetPoly`, `physics_gCheckGround`, and
  `physics_gCalcTargetPos` restore polygon/surface ground checks and the
  two-player rotated target-position publication; `physics_gCreateObject`
  owns component allocation/linkage; and `physics_gGetNearestTarget`
  preserves the original `daDelay`-typed nearest scan. The complete
  462-byte `player_DoCollisions` restores attack/Force eligibility,
  versus-mode and enemy-relationship gates, range overrides, hot-node
  contact, and the original target/hit/who-hit-me stores. The live PC frame
  now invokes this owner instead of duplicating its successful-contact tail.
  Focused tests cover every recovered owner and the existing PC attack and
  multi-enemy smokes exercise the integrated collision path.
- Recovered the adjacent 687-byte player motion/refresh and loader-name
  cluster. Exact `player_gConnectMotionData` reads the CAD-relative Motion
  table and count, rewrites both legacy `sabrhit`/`jedihit` name positions,
  and restores callback indices 1/2 from motion flags. Both the PC player and
  every enemy actor now use it instead of direct host pointer/count stores.
  The exact 114-entry PDB `sModelNames` table at RVA `0x4BCF10` and
  `loader_GetALevelName`, `loader_GetEnemyName`, `loader_GetLevelName`, and
  `loader_GetModelName` are readable source. Exact `player_gRefreshPlayers`
  runs all 20 refresh slots and retains the level-eight total-frame, Jedi,
  Streets, collision-info, per-physics collision-time, and scene-ready reset.
  Focused tests cover the authored-header mutation and full reset tail; all
  authored player/enemy gameplay gates exercise the integrated motion link.
- Recovered exact 153-byte `player_HandleSabre` as the original 20-slot
  post-scene dispatcher. Its object, model, scene, player-flag, and supported
  model-ID gates now call PDB-named `jedi_HandleSabre` for every eligible live
  actor. The reviewed Jedi saber path restores the exact model-specific node
  pairs, executable-backed blade colors, fixed-point normal/long endpoints,
  white core and colored glow calls, double-blade direction, attack-motion
  world sweep and feedback, powered-state timing, and tip-node time stores.
  Textured motion blur and the powered cylinder backend remain explicitly
  represented by dependency-free glow primitives rather than being claimed
  as exact immediate-mode rendering. The PC frame invokes the dispatcher at
  `scene_middleRender`'s original post-model/pre-player-processing boundary.
  Its new additive software pass projects the game-owned `fx_screenGlow`
  endpoints without a graphics dependency. The real FED smoke now requires
  at least two zero-drop blade passes and nonzero framebuffer composition;
  the three-frame Debug gate records `saber_glow=2/0/3304`.
- Models 72, 86, 87, 94, and 95 retain their exact machinery settings,
  including scales, collision-table selection, dimensions, model/root flags,
  and node depth offsets. Their newly exercised
  solid geometry exposed the still-bounded `loader_CreateModel` relocation
  seam. A validated immutable-BMD geometry resolver now supplies those
  streams and falls back to exact `getPtr` ownership for relocated models;
  focused resolver, physics, and real-asset gates cover both paths.
- Recovered model 47's exact six-record collision table at RVA `0x4CC7D0`,
  scale, dimensions, model/player flags, and callback slot 40. PDB-named
  `ai_StarFighter` now preserves its reset, mode, motion tuning, target/turn,
  wing rotation, sound, and firing-countdown paths, with exact 116-byte
  `physics_ForceFaceLock`. The exact 88-byte `Projectile` layout,
  heap allocator/free leaves, sound-table initialization, sprite-backed 3D
  beam construction, firing setup, and authored twin-projectile emission are
  connected. Exact `coll_CheckProjectileCollision` now preserves projectile
  radii and modifiers, collision-node filtering, the Starfighter exception,
  saber/Force ricochet and re-fire paths, ricochet audio, and hit location /
  motion publication. The complete 2,178-byte `bullet_CallBack` preserves
  ballistic and target-homing steering, the original one-eighth stored-speed
  step through exact `MoveObjectNormal`, both sprite update modes, plasma arc,
  lifetime/impact/trail effects, termination and bounce audio, masked nearby
  scans, ordinary/piercing/persistent/reflected hit transitions, both authored
  level-specific terminal-hit rules, and the type-6 map explosion and camera
  shake. Exact `fx_PlasmaZap` owns its 156-byte `_plasma_zapvars` state and
  nine-section glow emission. Exact `bullet_Explosion` publishes radial hits;
  all eight `bullet.c` procedures are reviewed.
- Reconstructed the complete PDB-named `force_PlaySeq` sequence owner:
  signed force-motion map decoding, exact Motion callback indices, force
  costs, per-power scratch state, persistent callback selection, authored
  combo-chain queuing, and the chained effect-74 launch are now readable
  source. The exact 50-entry PDB-named `funcArray` and `game_setFuncArray`
  preserve the authored Motion callback-index contract, and animation
  activation transfers each reviewed callback into the live player record.
  All 49 nonzero retail callback stores now reach their exact PDB-named
  owners; slot zero remains intentionally null. The completed Force family
  restores push/ranged/ring projectile paths, the flying sabre toss, grenade
  and ordinary toss transforms, the raycast/damage zap, and the held-input
  reflect sphere with its sixteen-band `drawCylinderG` renderer boundary.
  Exact `ai_FireWeapon` and `ai_Throw` occupy slots 1 and 2 with their
  executable-backed shot-spread tables, event-node projectiles, paired-facing
  gates, and attacker/victim motion publication.
  Exact `force_AbsorbReflectCallBack`, `force_AttackCallBack`,
  `force_AttackSpinCallBack`, `force_CloakCallBack`,
  `force_FlameCallBack`, `force_HealingCallBack`, `force_MesmerizeCallBack`,
  `force_PushCallBack`, `force_Ranged3CallBack`, `force_ReflectCallBack`,
  `force_RingCallBack`, `force_SabreSpinCallBack`,
  `force_SabreTossCallBack`, `force_SabreYoYoBack`,
  `force_ShieldCallBack`, `force_StarCallBack`, `force_TossCallBack`,
  `force_TossGrenadeCallBack`, and `force_ZapCallBack` restore their force
  drains, continuation and cancellation gates, target scans, authored
  reactions, effect cadence, flags, scales, sprite ownership, 600-frame
  lifetimes, and item consumption through exact `game_gModItemCount`.
  Exact `jedi_FireWeapon` now consumes authored ranged-player motion events,
  chooses single or paired muzzle/aim nodes, acquires a versus-aware target,
  applies the active power-up projectile override, and enters the production
  projectile/sprite/audio owners. `force_FlameCallBack` alternates its two
  muzzle sets by drawing-surface ID, and `tusken_stab` gates collision node 12
  across animation frames 10 through 11.
  Exact `physics_FindWithinRange` supplies the original 20-actor masked
  range iterator used by mesmerize. All persistent callback selections now
  point directly at their PDB-named bodies; there are no inferred persistent
  callback slots left. The still-partial immediate-mode glow realization is
  isolated behind dependency-free `jpb_` renderer hooks while the exact
  `fx_screenGlow` and `fx_GlowingMan` call surfaces remain intact. Exact
  ordinary and Maul trajectory callbacks occupy slots 6 and 48. The Win32
  headless host also disables CRT/WER fault dialogs and installs a
  terminating top-level exception filter so an automated failure cannot
  leave interactive exception windows behind. The complete Debug and
  optimized Release matrices each pass all 497 tests, including 47
  dependency-free unit tests, 450 real-asset/gameplay checks, and all eight
  headless PC gameplay gates.
- Restored the PDB-named `cube_NewWorldRender` owner and placed it back in
  the live PC frame path. The reviewed source now uses the recovered
  `minx`/`maxx`/`minz`/`maxz` module locals, `GameStruct.maxdraw`, typed
  `WorldData` dolly records, both players' `physicsObject.mapinfo`, and the
  exact Streets stair/end-state branch instead of anonymous decompiler
  addresses. It publishes exact `worldTURTLEMatrix`, `globalwinmatrix`, and
  `twattedcameramatrix`, including the original `[-x, z, y]` axis conversion,
  level offsets/scales, and swapped camera Y/Z placement. Exact retail stubs
  `cube_GetCubeAmbientLight`, `calc_frustrum`, `jon_texscroll`, and
  `set_camera` are present under their PDB names; exact
  `SetupTransformMatrix` and `_ApplyLevelTransformation` preserve the PC
  renderer state boundary without adding a graphics dependency. The old
  level-12 immediate `plotsomecubes` submission remains an explicitly named
  portable renderer hook while the live PC software renderer continues to
  draw the complete JPX world. Focused coverage checks single-player,
  two-player draw-distance clipping, dolly-camera fallback sight, Streets
  terminal expansion, the full level matrix publication, and level-12
  submission.
- Reconstructed all 31 PDB-named procedures in original `win32/sound.c`,
  including the exact retail stubs, requested/level/resident fallback,
  five loaded-bank slots, the 100-entry positional-loop owner, pause/halt,
  fade/stop, distance attenuation, and stereo panning. The matched
  executable's 43 bank descriptors and 696 unique path-pointer slots are now
  readable source; their intentional aliases and prefix subsets expand to
  1,242 descriptor-visible references. This preserves
  `jar_jar_playable`/`gungan_2`, Coruscant 2/`corus1`, and the seven
  cross-directory training sounds without searching unrelated banks. Exact
  `defaultOptionStruct`, `game_setAudioOptions`,
  `game_setControlsOptions`, and `game_setDefaultOptions` restore the shipped
  stereo/SFX/music/control defaults used by the live PC runtime.
- Added audible PC sound output behind that game-owned service boundary. A
  dependency-free WinMM adapter owns 64 overlapping WAV voices, timed
  fade/stop behavior, live positional-loop updates, quiet/non-spatial/voice
  classification, distance attenuation, stereo panning, the exact ordinary
  channel `0.92` SFX-volume multiplier, and full-volume looping channels. It
  resolves only members of the exact loaded bank and consumes the shipped PCM
  and IEEE-float WAV formats directly. Interactive runs bind this adapter by
  default and accept `--mute`; headless tests never initialize output. Focused
  tests cover all sound procedures and lifecycle paths. The real-asset gate
  validates all 1,887 shipped WAVs and every one of the 1,242 descriptor
  references.
- Connected `playXA` music to the same PC host. One independent WinMM voice
  consumes the 103-entry exact table and all 78 installed stereo PCM stream
  files, preserving repeated-track resume, loop, pause/unpause, stop-as-pause,
  volume, startup/shutdown, and channel-mode boundaries. A focused module test
  covers table holes, exact filenames, and every PDB-named control entry; the
  real-asset gate parses all 1,887 shipped WAV files, including the unique
  37.8 kHz splash stream.
- Removed the final broad `CheckCubeBlocking` contact callback. Exact
  `ExtraCharacterEnvironmentEffectExceptions`, `HitsHit`, `MTV`, and
  `clear_eventlist` now preserve force thresholds, map-entry mutation, the
  2,048-word circular undo log, packed event coordinates, and matching-channel
  propagation into adjacent cubes. `CheckCubeBlocking` directly emits both
  impact (`hitforce == 4`) and post-height-query ground (`hitforce == 8`)
  events and retains the collision-flag effect at exact effect slot 24.
  Exact `LaunchMapAnimEffects` decodes the executable-backed
  `eventarray[30][15]` and `maphitsounds[16]`, launches one sound per batch,
  performs the level 1/5/8 mesh switches through exact
  `cube_HideMesh`/`cube_ShowMesh`, and uses exact `StopNearestFan` for the
  level-10 fan event. Exact pointer-free `EffectData` and `EffectHeader`
  layouts keep effect assets portable. Exact `sprite_AddSpriteEffect`,
  `sprite_AddCallBack`, `sprite_MainCallBack`, `sprite_FireRing`, and the
  sprite/SCB allocators now carry that path through real effect allocation,
  recursive emitters, ring construction, fixed-point motion, brightness and
  scale control, and list ownership. The temporary typed effect backend is
  removed. Exact PDB `CControl`, `RingData`, `Ring`, and `optionstruct`
  layouts preserve the asset/runtime contracts without a desktop framework.
- Restored the full PDB-named `scene_middleRender` frame owner from its exact
  1,408-byte body at RVA `0xF5D10`. It now owns the original timer, camera,
  frustum, animation, overlay, world, model, sabre, player, power-up, sprite,
  enemy, backdrop, physics, and level-special ordering. The PC renderer is a
  narrow platform hook at the original model/world submission boundaries;
  animation, physics, gameplay, and timer advancement are no longer repeated
  by the host. Exact collision-frustum values (300, 85, 120, and 100 percent),
  strobe selection, initial-pause behavior, and world-to-screen publication
  are covered by focused and real-game tests. Enemy diagnostics now retain
  the original void `enemy_HandleEnemies` surface while exposing the last
  bounded authored-opcode result to portable validation.
- Corrected `game_DisplayOverlay`'s recovered SCB field ownership: palette
  selection remains in `scb_cvertex.pad`, while the credit, rescue, item, and
  score brightness values use `scb_vertex0.pad`, matching the executable.
  This removes the false green HUD tint seen in the earlier PC capture.
- Continued replacing `scene_middleRender`'s level callback adapter with exact
  PDB-named owners. `level_Fed` now clamps both players against the complete
  four-corner 32-byte `FedBounds` record and advances the shipped UV scroll;
  `level_Corus` restores the corresponding two-player `CorusBounds` volume
  and Z-plane clamp. `level_Palace` restores its 32-byte exit volume and the
  PDB-named slot-163/164 offscreen-boss cleanup. Exact `standingonit`
  preserves its strict platform bounds, player-state exclusions, and
  66-unit Z tolerance. A reusable
  Ghidra byte-range dumper records initialized aggregate evidence without
  guessing structure layout.
- Restored the adjacent level presentation and outcome chain. Exact
  `menu_drawBigNum` and `menu_drawBigNums` format and submit the original
  decimal texture IDs, allowing `level_Mini4` to retain its death-counter
  HUD and secret-bit rule. The complete 1,591-byte `level_CountDown` now owns
  timer, kill, score, beep, success/failure fade, music, checkpoint, exit,
  and mini-game unlock behavior for retail level IDs 14 and 16 through 22.
  Exact `level_Hangar` restores the 400-second pilot objective, UV conveyor
  motion, pilot-spawn signal, and both outcome paths. Exact `level_Arena`
  restores winner/draw presentation, continue exhaustion, level exit, and
  surviving-player Energy/Force refill. These scene cases now dispatch
  directly rather than through the temporary level callback. Focused tests
  run full 255-frame success/failure outcomes and preserve the retail
  player-two energy clamp.
- Restored Theed's complete PDB-named water ownership chain. Exact `fx_Init`
  loads `a_glow.tga`, `a_water.tga`, and `a_blob.tga` in the recovered order;
  exact `fx_Water` retains the matched timer divisors, nested sine wave,
  256-unit grid, camera-space row transform, near rejection, colors, and UV
  order. Exact `drawsomecrappywater` traverses the authored patch dimensions,
  twelve-cell ownership margin, four-corner frustum masks, signed width, and
  offset second layer. Exact `level_Theed` consumes the recovered
  `water_colors`, `water_crud`, and seven `theed_water` records and preserves
  its reset and level-exit state transition. The scene owner now dispatches
  level 3 directly. A dependency-free `jpb_SoftwareDrawScreenPoly` platform
  seam realizes the original triangle-strip payload against the shared PC
  depth buffer, while a small default-resource cache supplies the shipped
  water texture without adding a graphics framework. Focused tests cover
  wave/UV math, 48-polygon two-layer traversal, triangle-strip rasterization,
  and both restart scores; the installed Theed gate publishes 208 completed
  immediate polygons over three frames.
- Replaced the live installed-game HUD's compact 5x7 text substitution with
  a platform-neutral TrueType realization of the matched PC boundary. Exact
  `getFontFile` at RVA `0x176E0`, `LoadFont` at RVA `0xFDC90`, and
  `SDLTextWriteScale` at RVA `0xFDF40` establish the language/style NotoSans
  filenames, lower-seven-bit alignment mode,
  `trunc(scale * scaleAdjustment * 24)` point size, measured advances, alpha,
  and the 17-entry tint table at RVA `0x4CD100`. The adapter consumes the
  shipped `res/font` files and draws antialiased glyphs directly into the
  software framebuffer. Vendored stb_truetype source adds no runtime DLL;
  the old compact glyphs remain only as a bounded missing-asset fallback.
  Focused tests cover every recovered file-selection branch, exact point/tint
  constants, and real `NotoSansSC-Bold.ttf` rasterization. A 117-frame field
  capture verifies the score and item count through that path while authored
  Obi-Wan movement and attack rendering remain active.
- Recovered the complete display-text portion of exact initialized PDB global
  `allTextEverything` at RVA `0x4A1000`: seven language blocks, 498 published
  slots per language, and 68,317 verified UTF-8 bytes across slots 2..497.
  The original name is retained in readable source. Slots 0 and 1 are proven
  pointer aggregates rather than strings and remain explicitly non-text.
  Exact `generateAllText`, `UpdateCurrentlyLoadedFont`, and
  `MarkFontAtlasForRefresh` leaves now publish the selected language during
  PC game initialization; a dependency-free cached UTF-8 widening boundary
  adapts the retail byte strings to the portable renderer. The unit gate hashes
  the entire corpus and directly checks English, German, Italian, and Chinese.
- Recovered the matched title-menu state foundation instead of introducing a
  separate front-end. Exact PDB type `MENUVARS` now retains its named 984-byte
  x64 layout and portable field ownership, including the eight-entry menu
  stack, selection arrays, input buffers, score state, and player-selection
  tail. Exact `menu_mainInitMenu`, title entry, push/pop, player count/model,
  objective/game-over, training/restart, title/screen-saver, shock, and
  eligibility procedures are live under their PDB names. Dependency-facing
  input, texture, bucket, and controller reassignment calls are hooks at the
  recovered call sites. The PC runtime now selects its player through these
  menu owners. Ten exact `mainMdef`/Continue/No-Load title command streams
  are source-visible under their original names. Together with the seven
  first-level definitions below, all 2,660 bytes are protected by a single
  whole-stream hash plus focused transition tests. Exact PDB layouts
  `MMVDEF` (48 bytes) and `MDEF_MOD` (32 bytes), the 75-entry `mmsizes`
  command-width table, `mmNextCode`, the ordinary title subset of
  `mmDrawsub`, localized `mmDrawItem`, `mmDraw`, and edge-driven
  `menu_mainMenu` selection/activation are now live. The PC-only host uses
  its built-in WIC boundary to decode the installed 16:9 splash and the exact
  132-record `menuTextureList` bank while the portable core sees only opaque
  software textures. PDB-named `menu_winLoadTextures` publishes the exact
  `menuTextures`/`fontSpec` indices and native dimensions without introducing
  an image dependency into portable game code. The portable core overlays the
  original 13-item carousel and translucent selection panel through the
  existing TrueType framebuffer hook. The title
  hook now carries `scaleAdjustmentMM` independently from ordinary HUD text,
  matching the recovered point-size owner and removing the oversized PC menu
  rendering. Sixteen more exact PDB arrays add player-count, difficulty,
  New Game confirmation, Options, Quit confirmation, Controller, Language,
  Video, and Audio definitions: 28 source-owned streams and 3,992 exact bytes
  in total. `menu_handleMenuTriggers` now represents the matched executable's
  complete compressed jump table: ordinary destinations, New Game and VS
  setup, player/difficulty routing, character confirmation, level eligibility
  and scanning, score awards and upgrades, controller fallback, video apply,
  save requests, sound/music controls, cheats, and gameplay transitions.
  PC storage now reads and writes the exact pointer-free 4,624-byte
  `saveGameStruct` at `SAVEDATA0\\Game` and the exact 56-byte `optionstruct`
  at `SAVEDATA0\\Options`. Menu save requests reach the file boundary,
  interactive startup restores both records, clean shutdown preserves
  options, and headless validation never consumes user saves. Exact-size and
  version checks reject malformed payloads without partially applying them.
  Controller enumeration, video-mode application, and level resource work
  remain narrow dependency-free platform callbacks. Exact
  PDB-named `menu_cameraChange`, `menu_menuMusic`, and `menu_sound` own their
  recovered leaves, and `tempPlayersVs` retains its recovered global name.
  `jpb_pc_title_smoke`, `jpb_pc_title_character_select`,
  `jpb_pc_title_new_game`, and
  `jpb_pc_title_options`, `jpb_pc_title_audio_options`, and
  `jpb_pc_title_video_options` protect the initial, confirmation, navigated
  Options, and variable-backed Audio/Video presentations over the real splash.
  The exact 74-entry `modVars` table and typed modifier
  owners are live through a real Music toggle. Exact
  `menu_playerSelectCheck` and `menu_menuExit` now preserve player-two
  modifier direction, activation/back dispatch, stack exit, and both input
  mask refreshes. Unresolved modifier behavior, the full frontend state loop,
  the two-player selection presentation, and full-game camera validation
  remain open.
- Restored the exact menu input and cheat-recognition owners instead of
  polling player one directly in the PC title adapter. PDB-named
  `menu_readControl` now reads both pads through `input_ReadControlPad`, shifts
  both 16-sample histories, removes Zoom-Out from recorded cheat samples,
  advances the recovered 18,000-frame screen-saver state, and retains the
  ten-entry keyboard-scancode ring. Exact `checkKeyboardBuffer`,
  `cheatCheckKeyboard`, `cheatCheck`, and `menu_rotControls` consume that state,
  including the retail second-history clear quirk. The Win32 boundary maps
  native keys to SDL/USB scancode indices without adding SDL to portable code.
  Adjacent exact save-menu leaves are now source-owned, movie triggering uses
  a platform hook, and the recovered MMV trigger remap plus complete decoded
  dispatcher are live. PDB-named `jedi_CheckValidLevel` and
  `menu_setScoreMode` own level gating and score-award progression.
- Corrected the PC loader's camera-state handoff without changing the reviewed
  projection or level winding. Exact `scene_gInitRoot` installs view mode
  `0x901`, whose `0x100` bit drives `camera_CameraSlide`; the loader had later
  narrowed that state to `0x800`, freezing the live camera while authored
  dollies continued updating `focusDest` and `angleDest`. The 203-frame combat
  gate directly protects the slide bit and on-screen follow result. The exact
  level PSO remains two-sided (`D3D12_CULL_MODE_NONE`); the screenshot artifact
  was not back-face rejection.
- Current visual review accepts the FBX geometry, floor, wall, and door
  rendering. Camera composition remains visibly off and is still open; it is
  being audited independently without compensating in mesh transforms,
  projection, or renderer state.
- Audited the opening FED camera back to its shipped owners. The spawn walk
  polygon's packed camera byte selects dolly 3, producing authored location
  `(21996,3808,-15064)` and angles `(246,2432)`. That raw collision result is
  preserved rather than rewritten in the level data. The
  portable loader also no longer calls `camera_SetCameras` twice before the
  first retail frame boundary. The missing owner was FED AI 31 on the
  `pwrserv1`/`fed_door` placement: exact `player_RefreshPlayer` must seed its
  animation queue during `loader_CreateEnemy`, or the director stalls in model
  motion 3. It now reaches `0x604(144,1)` on frame two, applies the retail
  control/letterbox lock, and later advances to dolly 146. The subsequent gas
  branch exposed an exact traversal error in opcode `0x108`: the executable
  reads `BAP_AINODE::iParent` and resumes at the parent branch's sibling via
  `bapEnemySetContinue`; it does not jump to `iChild`. Correcting that owner
  releases the director from the dramatic close-up and reaches authored dolly
  145. Separate real-asset gates protect the collision-selected dolly-3 spawn,
  frame-three dolly-144 state, and frame-180 dolly-145 director continuation.
- Compared the opening room against reference gameplay and retained the
  visually accepted mesh, winding, projection, depth, and renderer path.
  Follow-up visual review rejected the camera composition. Re-audit confirmed
  that the PC boundary's prior rule holding scripted dolly 145 inside
  collision region 3 had no executable owner, so it has been removed rather
  than tuned further. The exact director releases its override by frame 360;
  recovered `camera_SetCameraPos` then selects raw collision dolly 3 for the
  stationary player and dolly 0 after the 420-frame traversal. Composition-
  sensitive gates now protect the exact target projections `(526.2,217.0)`,
  `(639.7,238.5)`, and `(429.3,263.9)` at those authored boundaries. These
  prove deterministic recovered ownership, not final retail visual approval;
  final camera framing/follow remains open while the accepted renderer stays
  unchanged.
- Rechecked the remaining camera-handoff rejection path against the matched
  executable. `camera_SetCameraPos`, `twatcameramatrix`, `buildfrustrum`,
  `buildplane`, `CalcNewBox`, and `cliptofrustrum` preserve the original
  operation order, float-to-integer truncations, player radius, and the raw
  `90/320/180` candidate-frustum constants. The shipped FED collision record
  therefore really does accept dolly 3 after the director clears dolly 145;
  retaining the straight-on shot would require an unowned rule. The remaining
  visual review is now explicitly about when reference gameplay reaches that
  authored transition, not a renderer or candidate-rejection substitution.
- Corrected evidence-backed sources of camera divergence. The exact
  `camera_StuffCamera` movement lead normalizes and samples its prior vector
  against `(sin(yaw),0,cos(yaw))` before advancing it; the prior reconstruction
  sampled after the update and transposed the axes. Static-reference review
  confirms the matched `gGlobalFrameRate`/`fGlobalFrameRate` values remain
  initialized at `0x800`/`0.5`. A prior portable frame-boundary rule carried
  one-unit odd pitch progress past `camera_Camera2ViewVector`'s next required
  even-angle mask. Re-auditing `game_OneGameLoop` proved there is no executable
  owner between `camera_SetCameras` and `scene_middleRender` for that store, so
  the compensation is removed. A unit regression now protects the exact
  half-rate mask/slide behavior rather than silently tuning the still-open
  composition.
- Corrected the recovered `enemy_ParseOpcodes` cycle entry to match the
  executable: every cycle begins at the AI root child through
  `bapEnemyStartCycleLoop`; opcode `0x106` performs the later
  `bapEnemyDoModeJump`. Entering directly at the mode target skipped valid
  root-side conditions and could advance scene directors ahead of their
  authored gates. A focused synthetic AI tree protects the exact ordering,
  while expanded camera diagnostics report the active override and every
  live enemy's PDB-named AI/mode/node state.
- Replaced the PC title adapter's private state-to-definition switch with the
  recovered ordinary portion of PDB-named `menu_mainLoop`. Main/continue and
  Register availability, startup, player-count, difficulty, Options, Audio,
  Language, Video, confirmation, and Controls selection now live
  in `menu.c`; gameplay Audio chooses exact `audioMdef_Game` for game modes 6
  and 7. Exact `startMdef` at RVA `0x4C76F8` and
  `playerCountSelectMdef` at RVA `0x4C6B90` add 96 verified initialized bytes.
  The PC boundary explicitly enters ordinary state 0 because the installed
  data set does not include the retail EULA/attract-movie presentation owned
  by state 1. A real-splash gate now traverses New Game confirmation, player
  count, difficulty, and state 4 instead of stopping at the confirmation.
  The bespoke character and Controls renderers are now recovered rather than
  represented as generic command streams; other bounded main-loop cases remain.
- Restored state four's exact `menu_setNumPlayers` entry call before
  `playerCountSelectMdef`, so the selected count, controller reassignment hook,
  and gameplay global bits are republished at the same title boundary as the
  matched executable. A focused menu regression protects the two-player bit
  and clearing of character bits 8 through 12.
- Recovered the exact one- and two-player character-selection states instead
  of flattening them into generic title definitions. PDB-named
  `newMenu_P1CharacterSelect` and `newMenu_P2CharacterSelect` now own their
  initialization, base/New Game+ tab switching, unlock-aware 0..79 model
  traversal, confirm/abort states, duplicate-player guards, controller
  routing, shared two-player confirmation mask, disconnect behavior, VS
  defaults, and saber-color input. Exact companion owners
  `jedi_CanToggleSaber`, `jedi_CheckValidPlayer`,
  `jedi_CheckValidPlayerNGP`, `jedi_CheckValidPlayerWTabs`,
  `jedi_CheckValidVersus`, `jedi_ToggleSaberColor`,
  `menu_initPlayerSelect`, and `updatePlayerSelectIndex` are source-owned,
  along with the initialized color, sprite, and versus-roster tables. Menu
  state `0x0E` dispatches the appropriate owner and preserves the retail
  success-to-`0x1A` and abort-to-title transitions. State `0x0D` now owns the
  exact VS entry setup. The 4,470-byte local
  `newMenu_DrawP1CharacterSelect` now draws its original background, main and
  adjacent character portraits, player container/overlay/detail layers,
  arrows, name and skill panels, weapon icon, progression values, and
  localized labels from the installed assets. Its exact helper owners
  `newMenu_DrawArrows`, `winDrawBackground`, `jedi_CalcSkillLevels`,
  `jedi_ConvertToTextIndex`, and `jedi_GetColorSprite` are source-owned under
  their PDB names. The screen compositor now honors descending menu-layer
  depth while preserving equal-depth submission order, preventing the blue
  overlay from hiding the selected portrait. The 6,195-byte
  `newMenu_DrawP2CharacterSelect` now owns the recovered mirrored P1/P2
  headings, arrows, divider, portrait/overlay/detail stacks, resolution-aware
  name panels, weapon colors, and progression panels. A second real-asset
  character-select gate verifies all 22 screen draws and eight localized text
  draws against the installed texture bank. PDB-backed controller-tab
  placement is now also recovered for both owners: the one-player view draws
  its exact centered Classic/Modern glyph pair, while the two-player view uses
  the original per-player pivots, mirrored ordering, and controller selection.
  Focused command geometry plus completed-game P1/P2 installed-asset gates
  protect the 21/26 screen draws and 6/12 localized text draws.
- Re-audited the exact New Game player-count/difficulty stack rather than
  inserting an expected character-selection jump. The matched streams and
  trigger table demonstrably route state 3 player count to state `0x37`
  difficulty, then to state 4, whose exact stream selects player count again
  and routes back to `0x37`. No recovered external stack writer or caller
  provides a proven state-`0x0E` handoff. This is now an explicit evidence gap;
  the source does not claim or fabricate behavior the executable evidence has
  not yet explained.
- Replaced the temporary Credits and Concept Art command streams with their
  bespoke PDB-named owners. `menu_drawCredits` now scrolls the exact recovered
  706-entry `theCredits` array, strips the heading byte for tint 16, advances
  from the recovered fixed-point bar speed using the live `deltaTime`, retains
  the original entry-frame pause, and rotates the exact eight Credit music
  tracks through `menu_initCredits`. `menuConceptMenu` now presents all 42
  installed art textures with the recovered page counter, wrap controls,
  pressed-arrow cache, 16:9 fit, and exit path. The extracted Credit source is
  reproducible through `tools/extract_credit_data.py`; focused state/input/
  render tests and installed-asset title gates cover both modes.
- Replaced the Controls title fallback with the PDB-named 4,991-byte
  `runControlsMenu` owner and direct state-`0x23`/`0x24` dispatch. Exact
  `ClassicControlScheme`, `ModernControlScheme`, Force variants,
  `controlTextList`, `controlTextListForce`, and `controlSubDraw` data are
  source-owned at their matched RVAs. `menu_winLoadTextures` now loads all 77
  controller-tail records from the installed CONTROLLER, KBM, PS4, PS5,
  Switch, Switch Pro, Joy-Con, and Xbox Series X banks. PDB-named
  `getControllerTextures` selects them through a dependency-light optional
  controller-name hook. A focused mapping/asset regression and a navigated
  real-title Controls gate protect the 20 localized text and 14 screen draws.
- Hardened the recovered camera's direct-polygon record lookup at the portable
  archive boundary. The Arena collision map exposed a retail-style derived
  record beyond the finite Jonny payload; the portable owner now retains the
  current dolly rather than reading adjacent heap memory. The isolated VS
  midpoint remains in place until the preceding retail Arena camera setup is
  recovered and can frame both players deterministically.
- After these ownership changes, complete Debug and optimized Release builds
  each pass all 523 tests: 49 dependency-light unit executables and 474
  installed-asset/gameplay gates. The matrix includes the recovered title
  interpreter/state gate, all 14 PC title-flow gates, all 33 PC-game gates,
  the shipped-font gate, and the isolated
  authored combat run. The character-select gate decodes all 196 unique
  installed frontend/controller images (128 from the exact 132-entry frontend
  bank), verifies
  19 P1 screen draws with no drops, and checks a real selected-character pixel
  so layer-order regressions cannot pass on command counts alone. Its P2 peer
  protects the recovered 22-draw mirrored two-player layout.
- Expanded the menu state-transition gate over the newly recovered dispatcher
  cases, including exact return value `1` for destination `0x3A`, character
  selection, player/difficulty routing, training, sound-test clamping,
  secret/global-bit toggles, music state, and energy-reset cheats. The current
  Debug and Release matrices remain green at 523/523.
- Added host behavioral tests for list ordering/removal quirks, timer state
  transitions, allocator sizing/exhaustion/reuse/coalescing, memory-pool
  behavior, fixed-angle math, normalization precision/padding, vector
  distance overflow, transform state, matrix construction/application/
  multiplication, rotation order, scaling, translation, transpose aliasing,
  screen-depth evaluation, matrix-inversion truncation, segment bounds, quick
  range quirks, normal-to-rotation conversion, plane projection, stream and
  memory-backed file reads, pool loading, chunk dispatch, JPX ownership,
  material relocation, progress callbacks, patch-site traversal, vertex
  decoding, and static spatial-node traversal.
- Protected reviewed modules from being overwritten by future inventory
  regeneration.
- Completed the normal New Game presentation bridge through the recovered
  character and level selectors. PDB-named `menu_initLevelSelectScreen`,
  `menu_drawLevelSelectScreen`, and `menu_levelSelectMenu` own state `0x1A`,
  the exact 15-entry `levelSelectMdef`, installed preview images, unlock
  filtering, confirmation, and the state-`0x66` load handoff. `--quickload
  <level>` remains an explicit development bypass rather than a hard-coded FED
  route. Complete Debug and optimized Release matrices pass 524/524.
- Recompared the settled FED gameplay camera against the supplied reference.
  Doorway position, horizon, player scale, and vertical framing now align; the
  earlier static composition rejection is no longer reproduced. Dynamic
  transitions, follow behavior, and other levels remain in the full-game
  camera survey.
- Audited the bounded Arena/VS camera fallback without promoting it. Exact
  `loader_LoadJedi` selects camera type zero for all two-player games, but the
  Arena library-floor formula resolves its camera metadata 1,152 bytes beyond
  the finite relocated J3D allocation; direct type-zero playback leaves player
  two behind dolly zero. The safe midpoint bridge remains isolated until the
  missing library/allocation owner is recovered.
- Surveyed every installed gameplay camera at the portable runtime boundary
  rather than treating FED as representative. Mini2 exposed a concrete missing
  special-level owner: `camera_SetCameraPos` replaces its ordinary target with
  the PDB-named `gJarJarPos`, while exact `ai_Kadu` updates that position and
  selects camera type 6. The portable runtime now publishes the live rider as
  the temporary target until its two Kaadu mount actors exist; Mini2 renders a
  filled world and visible player at startup and after settling. A dedicated
  installed-asset camera gate protects that behavior. The same survey also
  separated remaining director/special-actor omissions from generic camera
  math: Corus1, Hangar, and Mini1 still require their missing level-specific
  owners before their quick-load framing can be judged as final.
- Complete Debug and optimized Release verification now each pass 525/525.
  The matrix includes 476 installed-real-asset gates, 35 PC-game gates, and
  the dedicated Mini2 Kaadu-camera regression; no failures are suppressed.
- Resolved the latest user-reported PC presentation defects at their recovered
  owners. BMD quads now use the executable's alternating triangle-strip
  topology and are clipped/cull-tested as independent triangles, eliminating
  the intermittent battle-droid and FED-door fan deformation. The software
  glow compositor now unpacks the recovered A8R8G8B8 saber colour without
  swapping red and blue, so Obi-Wan retains his authored blue blade. Screen
  polygons preserve the exact texture class selected by `_LoadTexture`;
  `a_`/`p_` effects, including FED `a_SMOKEGRY`, now blend their shipped alpha
  instead of rendering as opaque cards. Regression coverage includes an exact
  half-alpha framebuffer result, and sampled FED intro frames retain stable
  door geometry through the authored dolly 144/146/145 sequence.
- Added dependency-light Win32 XInput through dynamic system-DLL loading, with
  two-player connection polling, the recovered generic-pad mapping, rumble,
  controller identity, and automatic keyboard/Xbox prompt-bank selection. No
  import library or third-party controller runtime was added.
- Advanced front-end parity by rendering the exact shipped command-`0x44`
  selection panel, replacing legacy literal control characters with the
  installed keyboard/Xbox keycap textures, and applying TrueType kerning to
  the PDB-selected Noto menu face. Installed-asset captures report zero
  fallback-font draws and show the physical `SPACE`/`K` prompt glyphs.
  The command stream's cyclic title items are now clipped to that authored
  panel at rasterization time. The recovered level-select draw owner restores
  the shipped orb backdrop, nine-piece carousel/selection marker, bold stage
  digits, and the executable's 2.5 heading scale. A real-asset regression
  traverses New Game, character confirmation, and the resulting state-`0x1A`
  selector instead of validating that screen only through a direct launch.
  `--quickload <level>` now skips only the front end and loads the complete
  recovered actor/machinery class set for that selected level rather than an
  obsolete player-only inspection scene.
- Reconciled Corus1's live FBX world with the original signed 16-bit gameplay
  coordinates at its level-specific renderer boundary. Vertices crossing the
  65,536-unit seam now select the equivalent world image nearest the recovered
  camera; a focused regression proves that an authored triangle around
  `x=65536` remains absent from an ordinary inspection scene and becomes
  visible only for Corus1. The correction is deliberately not global: applying
  it to FED pulled remote level geometry into the depth buffer and occluded
  the player, which the attack and jump gates caught. The wrap uses bounded
  `fmodf` arithmetic and rejects non-finite deltas rather than looping over
  untrusted coordinates.
- Re-audited Corus1's opening dolly without inserting a visual hard-code. The
  exact start table, walk-height selection, Jonny-library relocation, packed
  camera byte, `.cam` records, and original constant `CalcNewBox` mask all
  select dolly zero, while dolly one is the record that visually frames the
  spawn. That remaining discrepancy stays classified as a missing
  level-specific owner. The PC boundary does not silently substitute dolly
  one for the shipped selection evidence.
- Closed the reachable title-flow regression gap. Independent P1/P2 headless
  phases now preserve controller ownership instead of mirroring P1 onto both
  players. A fully navigated title -> VS player-count -> two-character
  selection gate resolves the installed Arena and proves both rendered player
  actors. Additional gates verify live Language text-bank regeneration and
  both recovered Quit-confirmation destinations: `0x94` returns to title and
  `0x93` reaches the host exit request. Explicit world paths derive their
  installed asset root from the supplied `res/level/jpx` path, allowing the
  same complete flow to run from an unstaged development executable.
- The complete reachable-menu subset passes 21/21 in optimized Release, with
  Debug and sanitizer counterparts protecting the same state owners. The
  broader optimized matrix passes 534/535: its sole red gate is the already-
  staged FED moving-camera projection, currently `(444.2,300.8)` against the
  retained accepted assertion `(429.3,263.9)`. That known camera discrepancy
  is not being rebaselined as part of menu work. The matrix also includes the
  new signed-world seam regression, the corrected
  16-draw Controls/keycap presentation gate, the shipped-font gates, XInput,
  all recovered model/animation archives, and authored FED gameplay. Repeated
  control-interpretation passes remain a separate post-100% QA phase and do
  not count against the reconstruction completion metric.
- Recovered the complete base-roster saber ownership path from
  `jedi_HandleSabre`, `jedi_GetColorSprite`, `drawCylinder`, the linked live
  color tables, and exact machine-code call sites. The menu color-sprite
  getter now follows `gJediColorSpriteCurrent` instead of bypassing character
  toggles through the legacy table. Obi-Wan, Qui-Gon, Mace, Adi, Plo, Maul,
  and Ki-Adi emit their character-owned blue, green, purple, or red blade;
  Amidala and Panaka emit none. Maul's second blade begins at its authored
  `0x20` directional offset and spans `0x50`, rather than incorrectly drawing
  a second full `0x70` blade from the hilt. Blade Extender retains the exact
  `0xc4` endpoint, 24..31 halo, six-unit core, and tip timer. Blade Amplifier
  now emits the exact base-to-tip glow plus three short, constant-radius
  `drawCylinder` bands. The previous portable substitution incorrectly used
  those bands' 8/40/72 height phases as full-blade glow widths, producing the
  reported oversized saber. A dependency-light cylinder boundary retains all
  recovered geometric and texture-selection parameters and renders the bands
  as oriented short segments rather than widening the entire blade.
- Added live `player_weapon` diagnostics and a strict
  `--validate-player-saber` gate for model, live color word, halo/core pairing,
  width, and blade length. All nine base characters now traverse both the real
  front-end handoff and a character-specific north attack using their shipped
  CAD/BMD/CMB data. The full optimized matrix passes 577/577, including all 45
  controls gates; the new player/menu units and nine-character attack matrix
  also pass in Debug.
- Resolved the user-reported clean exit after character and level selection.
  The persistent installed-root log reached exact menu state `0x66` and then
  exited with host status 5 without an exception. That PDB-recovered state is
  the intentionally non-rendering level-load handoff: it has no menu
  definition because the platform host owns the next step. The portable title
  wrapper incorrectly classified its zero-draw frame as a renderer failure
  before the interactive host could consume it. `jpb_GameRuntimeTitleFrame`
  now accepts `0x66` as a successful ownership-transfer frame while retaining
  dropped-draw failures. A dependency-light runtime regression deliberately
  holds that state with zero text and screen draws. It passes in Debug and
  Release; all nine shipped-character gameplay handoffs, the authored-camera
  handoff, and the complete 45-test controls pass remain green. The full
  optimized matrix now passes 578/578. The verified executable is staged in
  the installed game root with SHA-256
  `7FD2090E31105D52C6868FDA3078D197176A3155327E6DCB8691045D822961ED`;
  an installed-root title-to-FED run completes the handoff, validates
  Obi-Wan's live blue saber, and renders gameplay.
- Open regression report (2026-08-11): the user observed a crash immediately
  after character selection while testing the updated interactive build. This
  has not been reproduced or diagnosed and does not displace the active
  player-control audit. The deterministic rendered-state harness and a staged-
  root headless candidate both complete title -> character select -> level
  load -> FED gameplay without an exception, narrowing but not closing the
  report. The installed-root interactive path must still be exercised with
  persistent logging before the report is considered resolved.
- Recovered and implemented the exact `ReadJoystickInput` control-configuration
  split. The shipped modern gameplay configuration now maps A to jump and
  B/X/Y to north/west/south attacks; classic gameplay retains A/X/Y attacks
  and B jump, and menus force classic A-confirm/B-back semantics. The host no
  longer imposes a generic 24-percent XInput deadzone. It uses each player's
  `OptionStruct.WalkLimit` and `RunLimit` with the original strict percentage
  thresholds, preserves unscaled stick axes, keeps D-pad movement digital and
  non-running, adds simultaneous D-pad/stick contributions, clamps configured
  percentages at 100, and accepts the original nonzero trigger range.
- Audited controller identity through exact `ReadJoystickInput` and the PC
  XInput adapter. The executable compares and stores physical SDL controller
  objects; the adapter's ownership helper likewise returns a physical XInput
  user index. The read/rumble layer had incorrectly treated that index as the
  Nth connected controller, misrouting sparse users 2/3 and rejecting some
  valid assignments. PDB-presentable names now distinguish player ownership
  from physical-user access, input and rumble consume the exact physical user,
  and shutdown stops all four possible XInput users. A fake-device integration
  gate covers sparse users 2 and 3, independent P1/P2 buttons, disconnected and
  out-of-range reads, exact motor routing, and four-user shutdown.
- Restored the exact PDB-backed input shocker lifecycle. `ShockBuffer` is the
  recovered `char[2][6]`, `shocker` is `Shocker[2][6]`, and the prior
  `feedback_startEffect` reconstruction had shifted `timer`, `power1`, and
  `power2` into the wrong internal fields. The corrected path implements
  `clearShockers`, `handleShockers`, `psxUpdatePadbits`, and the optimized
  retail `startRumble` behavior. Debug and Release regressions prove motor
  scaling, option and effect-index gates, immediate clearing, and exact
  four-tick expiration for `hithard` instead of the erroneous 255 ticks.
- Recovered the rest of the packet/disconnect handoff from exact PDB types and
  decompilation. `ControllerPacket` is the recovered 34-byte record and
  `padBuffer` its four-record array; `PadGone` now maintains `padExist`,
  `padTypes`, `padShockable`, and clears live vibration exactly across packet
  loss and `'A'`/`'s'` format changes. The PDB-named `P1Disconnected`,
  `P2Disconnected`, `padbuttonpressed`, and `setMenuBindings` owners are
  present as readable source. The PC host records physical connection deltas,
  flags a lost P2, refuses to substitute an already attached controller, and
  restores P2 only when an eligible controller is newly attached, matching
  `UpdateJoyDevices`/`AddControllerDevice` ownership.
- Restored both retail `setMenuBindings(GameStruct.inMenuFlag,
  GameStruct.gameMode)` calls at the control-read boundary. The PC keyboard
  mapper now implements the complete state split: gameplay retains the
  Shift/Enter/Escape action chords; menus emit Shift=block/back and
  Escape=jump/back; Enter/Space emits Start on the title and A/confirm in the
  other menu states. Focused packet, disconnect, sparse-user, keyboard-state,
  menu, and 24-flow title regressions pass in Debug and Release.
- Extended the diagnostic-only player-process observer at the exact
  `brain_ControlPlayer` boundary so validation records the actual pressed and
  held `mPlayerRead` channels, releases, lock-state transitions, and P1/P2
  locomotion. A unit regression proves release re-arms rising edges. A real
  FED regression proves a held lock input toggles once and that release plus
  repress toggles it off. A new `--headless-phase-pair` gate simultaneously
  drives opposite Arena directions and proves independent P1/P2 edge, held,
  release, and locomotion ownership.
- The focused control work passes in Debug and Release. All 47 optimized
  controls gates pass, and the complete optimized PC matrix passes 580/580.
  The exact candidate also passed the installed-root title-to-FED flow before
  being promoted to `jpb_pc_game.exe`; source and staged copies share SHA-256
  `6530987CE7AD791D5D0F4C778ADFBE0C570BCCD70DA3C8087780B16E40C72402`.
- Continued the source-driven control audit through the analog and contextual
  paths. Signed XInput axes now use the executable's exact 32767 divisor,
  preserving its positive/negative full-scale asymmetry and strict 100-percent
  threshold. The secondary-player directional gate reads the recovered P2
  input-type global instead of P1's, and Arena publishes the same signed camera
  destination consumed by `brain_ControlPlayer`. Real-asset gates cover exact
  camera-relative axes, desired facing, walk/run selection, and independent
  P1/P2 ownership for every base character.
- Restored passive-motion reporting at the portable player-scheduler boundary.
  Callback-free active motions are now reported through the recovered motion-
  callback seam for human-owned players, allowing exact Motion-2 branches to
  observe a sustained run. Releasing movement or blocking after the recovered
  16-frame threshold enters Motion 25 (`jd_runstop_core`) and its skid callback
  instead of leaving the player in a looping run with a constant movement
  vector. The adapter deliberately stays out of authored AI and Force-modifier
  frames, preserving the exact FED director and `force_gActivate` callback
  gates.
- Added exact 21-frame running-attack coverage for motions 92/93/94, their
  15/15/14 displacement values, all three directions, and all nine shipped
  base-character CAD/CMB sets. Standing versus directional jump launch, run-
  release skid, run-block skid, analog thresholds, and passive motion reporting
  have focused unit or real-asset regressions.
- Expanded real-asset control coverage across all nine base characters. The
  matrix now verifies four lock-on movement directions, character-owned block
  motions, all four maximum-progression Force chords, standing and running
  jump launch, and exact authored two-hit CMB chains for every character. The
  headless attack validator now accepts a reached damage motion only when it is
  either the expected one-button attack or a CMB record whose attack symbols
  are present in the observed input; valid chained motions are no longer
  misreported as base-attack failures. Maximum-progression tests now unlock
  the complete `jediComboMask` before combo initialization, matching their
  advertised state and exposing the authored award-gated chains. Combo-tally
  diagnostics distinguish an accepted CMB record even when two records share
  a motion and the later initialization overwrites that motion's combo index.
  Real-asset regressions now exercise all nine authored three-hit chains, all
  eight non-Force four-hit chains, four five-hit chains, and Obi-Wan's six-hit
  chain with exact input order, timing, queued motions, names, combo records,
  and tally progression. Queue diagnostics expose all eight exact pending
  animation slots, and combo tests assert the authored target at the input
  boundary rather than relying on a later animation tail. The focused Debug
  combo matrix passes 33/33 (31 playable chains plus metadata and source-
  provenance gates). Optimized Release passes all 248 controls gates (201
  roster gates) and the complete 780/780 suite.
- Ki-Adi's CMB contains the five-hit `s.s.s.w.w` chain, but its opening Motion
  130 (`jd_stab_r_base`) has `cutin=0`, frames 0-19, and `disp=24`; it has no
  start-frame pre-roll. Correcting `anim_GetAnimFrame` starts that attack at
  frame 1 rather than frame 19. A complete frame-order trace now proves the
  remaining incompatibility is present in the recovered executable rules:
  the CMB owner promotes the motion to lock 30, exact `anim_CheckSlack` clamps
  `disp=24` to frame 18, animation processing precedes player input, and
  recovery resets the combo before a release/repress edge can form `s.s`.
  Gaps 1-40 all restart record 1. A real-asset provenance gate protects that
  exact lock-30/recovery result alongside the metadata gate and reachable
  four-hit `w.w.w.w` chain. It remains an authored-data discrepancy; no
  character-specific timing exception is introduced without source evidence.
- Hardened interactive diagnostics around the source-driven control path.
  The Win32 host now maintains a silent last checkpoint before every title or
  gameplay frame and at each destructive character-select-to-level handoff
  stage. The terminating exception path records that checkpoint with the
  exception code, module RVA, thread, and x64 RIP/RSP/RBP registers. Gameplay
  control transitions now also record animation frame/lock, combo prefix,
  held/released masks, chain slack, callback occupancy, and player flags. A
  dependency-free formatter regression proves the exception record remains
  useful even when the process terminates before ordinary cleanup.
- Closed an end-to-end controller-test blind spot at the reported character-
  select crash boundary. Headless phases can now carry physical XInput-format
  buttons through the production mapper rather than bypassing it with logical
  pad bits. Real-asset regressions drive physical A/D-pad navigation through
  one-player New Game and two-player Versus, cross the destructive gameplay
  handoff, verify retained XInput ownership, and execute scheme-1 physical-B
  attacks for P1 and P2. Both paths pass in Debug and Release. The full
  optimized matrix now passes as complementary 307/307 controls and 532/532
  non-controls partitions, covering all 839 registered tests without overlap.
- Closed a first-frame control leak found by the new physical handoff gate.
  The final modern-layout A used to accept character selection could reach
  player construction and become Jump. The portable loading bridge now arms
  a per-player neutral-input guard before destructive runtime replacement,
  matching the retail load owner's cleared edge. Real-asset gates prove no P1
  action leaks into two-player Arena play, then drive P2's physical LT+B Force
  action to authored Qui-Gon Motion 67. A one-player counterpart reaches
  Obi-Wan Motion 99 after the New Game handoff. Direct executable references
  establish and tests preserve the matched-PC asymmetry where each device is
  translated with its own controller configuration but both gameplay brains
  use `OptionStruct.ControllerConfig[0]` for Force, block, lock, and combo
  interpretation. An exhaustive player regression also now checks both
  players' authored positions, facing, and reset control state at all 26 level
  starts.
- Completed a representative physical modern-layout action matrix with shipped
  Obi-Wan and FED assets. Production XInput mapping now reaches authored A
  jump, B/X/Y attacks, all four LT Force/item actions, LB block, and RB lock-on
  against the live post-intro battle-droid target. Back additionally proves
  the exact eight-frame orbit contraction from `4000.000` to `3496.763` and
  the recovered `0x00200000` context flag; Start proves the P1 `0x00001000`
  pause flag. The headless report now publishes these control-context fields
  alongside exact edge counts, making the remaining physical action paths
  externally auditable without adding an input dependency.
- Closed the saber matrix's final-frame blind spot. The existing 27 roster
  attack gates formerly validated only the last rendered pose after most
  attacks had recovered. The PC diagnostic now follows every authored damage
  frame and every recovered saber spin/toss/yoyo callback frame, separates
  motion-blur trails from core-paired blades, and proves each colored outer
  segment is attached to the exact character-specific nodes selected by the
  matched-PC `jedi_HandleSabre`. It enforces the executable's character color
  table, white two-unit core, randomized 14-19-unit outer width, normal
  112-unit blade, and Maul's asymmetric second blade. All 27 one-button roster
  attacks pass this action-window check in Debug and Release. Dedicated
  60-frame Mace and Ki-Adi saber-toss gates observe 59 valid action frames
  apiece, while a two-player Arena gate simultaneously proves Obi-Wan's blue
  and Qui-Gon's green blades remain owned by their respective actors. All 36
  maximum-progression roster Force chords now also carry the final/action
  saber gate. Direct reference analysis confirms the old `sabre.c` pool and
  spline functions have table/data references but no live external code
  callers in the matched executable; the player blade owner is the PDB-named
  `jedi_HandleSabre`, so no unrelated dormant renderer was substituted.
- Added continuous mixed-action coverage after the isolated control matrix. A
  246-frame logical Arena sequence drives Obi-Wan through run, running attack,
  recovery, block, lock-on, locked shuffle, authored jump/landing, and Force.
  A 291-frame physical XInput sequence alternates Obi-Wan and Qui-Gon attacks,
  lock-on, jump, Force, simultaneous block, and recovery while checking exact
  per-player pressed/held/released telemetry and action-window sabers. Phase
  boundaries now publish P2 motion/frame/lock/target state as well as P1 and
  distinguish the live lock bit from a retained target pointer. The resulting
  trace confirms that jumping source-faithfully clears the live lock flag while
  the cached target may remain.
- Preserved the source-ordered main-animation callback gate with a focused unit
  regression. A callback returning `-1` retains exclusive frame ownership; a
  callback returning `1` clears itself but still prevents same-frame held input
  from starting a new action. Only the following frame may enter Obi-Wan's
  authored Force motion. This protects recovery/landing transitions from input
  reinterpretation that would make the reconstructed controls feel incorrect.
- The matched `WorldBlocking` landing owner now uses its exact source fields
  instead of two adjacent lookalikes. RVA `0xDEE49` reads the signed short at
  `playerObject+0x88` (`playernum`) to select collision-node rows, not
  `playerObject+0x8A` (`playerID`); the old reconstruction therefore landed
  only for roster IDs that accidentally matched their P1/P2 slot. The landing
  material check at RVA `0xDEEA3` reads `physicsObject+0x180`, which is
  `currentmapinfo.poly` after the recovered cube/entry/poly pointers at
  `+0x170/+0x178/+0x180`. Using `currentmapinfo.cube` interpreted cube-header
  bits as liquid flags and stranded airborne actors at authored ground height
  on Coruscant and Marsh. Real-asset gates now prove exact jump-to-idle
  recovery for all nine base characters, Mace in player-two slot 1, and
  recovery into follow-up attacks on Palace and Coruscant. Phase diagnostics
  publish both `airmov.vy` and collision-adjusted `mov.vy`, exposing the
  distinction that led to this fix.
- The optimized Release matrix now passes complementary 307/307 controls and
  532/532 non-controls partitions, covering all 839 registered tests without
  overlap. Focused Debug validation passes all 27 roster attacks, all 36
  roster Force chords, both saber-toss actions, and simultaneous two-player
  saber ownership. Both long mixed-action sequences and all 13 new
  roster/cross-level/P2 recovery-context gates pass in Debug and Release.
- Physical keyboard-format headless phases now enter the same exact
  `jpb_PCMapKeyboard` seam as live W/A/S/D, Left Control, T, J/K/L/H,
  U/I/O/Y, Shift, Space/Enter, and Escape polling. Fourteen Debug/Release
  real-asset gates prove run/walk, jump, three attacks, block, lock-on, four
  Force/item actions, zoom, and pause through the production gameplay brain,
  including exact keyboard ownership and pressed/held/released bits. Eight
  additional Arena gates independently prove P2's physical XInput north/west/
  south attacks, all four Force/item actions, and locked directional motion.
  The expanded ownership matrix found two headless-fixture defects: the
  validation union treated P2-only movement as a failed P1 move, and maximum
  progression granted P2 an item before `jedi_InitPlayer` reset its character
  data. Validation now tracks observed bits per player, and the completed-save
  item fixture is reapplied after P2 initialization; Qui-Gon's P2 `LT+X`
  consequently reaches authored Motion 65. All 22 new gates pass together in
  Debug and Release, followed by the full 307/307 and 532/532 Release passes.
- Corrected the central gameplay brain's adjacent PDB-field mappings. Matched
  executable offsets establish `currentMotion` at `playerObject+0x1B4`,
  `ACTION_LOCK` at `+0x1BC`, `pMainCallBack` at `+0x218`, and
  `pMotionCallBack` at `+0x220`. `brain_ControlPlayer`, its damage/chained
  recovery blocks, and `brain_HangCallback` had been writing the motion field
  where the executable writes the action lock. Jump launch, takeoff, thrown
  recovery, ordinary/Maul trajectory continuation, double-jump renewal, and
  long-air recovery had likewise installed the trajectory owner in the main
  callback field instead of the motion callback field. The combined error
  erased the live motion identity every frame and made the source-exact
  trajectory callback skip its Motion 4/22 steering and double-jump branch.
  Focused unit tests now protect all corrected offsets and callback owners.
  Four shipped-asset gates prove Obi-Wan's ordinary and directional Motion 22
  double jumps, Motion 4 air steering, and independent P2 Mace double jump.
  The full Debug controls partition passes 307/307; optimized Release passes
  307/307 controls and 532/532 non-controls, or 839/839 total. The deterministic
  FED camera settle baselines now reflect the persistent source-correct motion
  state. The authored-combat harness freezes only its selected target's
  pre-existing AI locomotion so the actual hot-node, damage, energy, and
  reaction pipeline remains isolated and regression-covered.
- Extended the source-driven attack-state audit through the complete block
  lifecycle and defensive hit-reaction owner. The matched `brain_ControlPlayer`
  branch selects lock-on idle Motion 20 before low-energy Motion 19 and applies
  the low-energy idle only below 26; a focused unit gate now protects that
  exact priority and boundary. Held block remains stable in chained Motion 21
  without re-queuing another 15-to-21 pair, while release lowers the authored
  lock and permits idle recovery. A new shipped Obi-Wan CAD/CMB/BMD gate proves
  the whole sequence: Motion 15 queues `jd_idle_block_core`, one release edge
  is observed, and the actor returns to `jd_idle_a_core` with an empty queue
  and animation lock 0. `braindmg_Blocking` now has direct source-level
  coverage for its random authored Motions 16-18, Motion 21 chain, stun,
  Force, hit-delay and hit-count stores, its normal accumulated-damage cutoff
  above 128, and the five-point increase supplied by one defense-upgrade
  nibble. No production behavior needed changing in this interval: comparison
  against the matched decompiler export confirmed the reconstructed owners.
  Debug passes 307/307 controls; optimized Release passes all 839/839 tests
  (307 controls plus 532 non-controls).
- Audited the first controllable frames after level refresh and front-end
  runtime replacement against the matched decompiler exports. Exact
  `player_RefreshPlayer` intentionally clears each actor's two prior-button
  histories, while exact `input_ReadControlPad` keeps the shared physical-pad
  suppression mask and applies the recovered rising-edge/continuous-mask
  equation. The all-authored-start regression now executes that boundary at
  every one of the 26 levels instead of checking only static reset stores: P1
  receives one pressed direction/action edge plus held state, a second held
  sample cannot repeat the edge, release re-arms it, and P2 remains independent
  and neutral. The fixture also reproduces `player_gCreateObject`'s exact
  `mask0=0`/`mask1=UINT32_MAX` channel configuration and the loading path's
  initial neutral sample. The physical XInput front-end regression now holds
  the final A confirmation for seven frames across runtime replacement; all
  are suppressed until neutral and the later B press alone enters Obi-Wan's
  authored north attack. No production change was warranted. A clean Debug
  controls pass is 307/307 and the complete optimized Release suite is
  839/839. The installed executable remained untouched during that interval;
  the later Force diagnostics below rebuild only the build-tree candidate.
- Extended the source-driven Force/control audit from launch poses through
  authored ownership, resource, chain, and callback lifetime behavior. The
  headless boundary now reports exact `pForceCallBack` names, energy/Force/item
  counts, and all six `forceData` values for each live player; the final report
  no longer mistakes the allocated inactive-player object for an active P2.
  All 36 roster Force chords retain their exact `mapData` motion assertions.
  Mace's absorb gate now follows motions 99, 100, and 101 and then exact idle
  recovery. Adi's mesmerize gate proves the RVA `0xA0950` owner remains active
  through tick 300 and is cleared at tick 301. Adi/Ki-Adi cloak, Adi shield,
  Plo star, and Ki-Adi mesmerize gates assert the first PDB callback tick and
  exact Force/item cost. Focused activation tests protect the distinct
  `0x2000`/`0x4000` upgrade gates, global Force-level override, inventory and
  Force boundaries, callback/motion re-entry gates, lock-rejected feedback,
  and the executable's source-ordered simultaneous item+north behavior. Debug
  passes the full 307/307 controls partition; optimized Release passes all
  839/839 tests. That interval's build-tree candidate was SHA-256
  `652836FFB5CAE6C698718F01316D11FA3EFA8C508D328BFC415DEAA400703D83`.
- Completed the next source-driven control audit interval across the combo
  engine. All eight PDB-named `combo.obj` procedures were reviewed against the
  raw Ghidra export; instruction inspection at RVAs `0x27AB0..0x27AE3`
  confirmed and restored the formerly omitted `combo_ValidComboAward` body as
  the exact per-character `numHits <= threshold` predicate. Focused source
  tests now protect all character threshold branches, unavailable-combo and
  level/versus bypasses, early/late input markers, held-key admission, active
  combo-flag promotion, and reset-time held/released-mask behavior. The
  selected Debug authored-combo run passes 32/32, the complete Debug controls
  partition passes 307/307 (including all 33 combo-labeled gates), and the
  optimized Release suite passes 839/839. The 1,040,384-byte candidate remains
  build-tree-only at SHA-256
  `B2A0DCFB94BD59BB1E609B4C30F14440B39ECCB032503F00FA3BBE84A35F08D9`.
- Extended the source-driven control audit through two exact damage-reaction
  ownership branches. Instruction review at RVAs `0x1E5A4..0x1E5F4` proves
  that Obi-Wan/Mace's special forced-block condition reads the player's locked
  `target`, while the later gates still read the distinct hit-source argument;
  the reconstruction had incorrectly used the hit source for both. Bidirectional
  tests now make those actors disagree and protect the exact owner. Review at
  RVAs `0x1FBC8..0x1FC1B` also proves player ID 35 (AAT) forces death effect 18
  instead of reading the active motion's `fx1`; the old path could index an
  invalid effect. A focused test uses `fx1 == -1` to protect the override, and
  the installed `tank.cad`, `aat.bmd`, and `aat.cmb` confirm the authored AAT
  asset family. Debug passes all 307/307 controls; optimized Release passes
  all 839/839 tests, including the new `jpb_braindmg_tests` cases. The current
  1,040,384-byte build-tree candidate is SHA-256
  `4087B46769CAF4AB34D2E1392B700B360019AC41F6769713AF23A1D02243E97F`.
- Restored the missing PDB-named player-frame geometry owner. Instruction and
  decompiler review at RVAs `0x20C30..0x212FF` now backs exact
  `brainutl_AddSabreEdge`, `brainutl_ConformGeomNodes`, `brainutl_FindLSB`,
  and `WInput_IsKBM` implementations. The live PC frame calls geometry
  conformance for each active player after the scene/input update, preserving
  normal/small model scale, minimum closing distance, head scale, the exact
  eight-node big-hands/feet/saber table, controller and independent keyboard
  chords, and their one-press debounce. Focused tests protect the 16-bit LSB
  truncation, final saber-edge result, every scale/flag store, toggle-off
  behavior, and the executable's keyboard polling even when XInput was used
  last. Optimized validation exposed a layout-sensitive sprite double-buffer
  selector fault; the owning sprite routine now snapshots and range-recovers
  its selectors and publishes recovery diagnostics instead of indexing beyond
  either two-entry list. The optimized multi-enemy gameplay gate survives
  20/20 consecutive runs with its exact `classes=12/11/` signature and no
  exceptions. The complete Release matrix passes 839/839, including all
  307 control-labeled cases. The current 1,042,432-byte build-tree candidate
  is SHA-256
  `CFD74EEE138BF585092525EF2F99CCB02AEA840E98236800DC9F64B697B8B87F`.
- Extended the player-control audit through authored ranged-weapon events.
  Exact instruction review of `jedi_FireWeapon` at RVA `0xB1BB0` proves its
  character-specific muzzle table selects on `playerObject.playerID` at
  `+0x8A`, not the active motion; the recovered routes now cover the default,
  IDs `0x11`, `0x12`, `0x1A`/`0x1C`, and `0x30`, including the paired-shot
  models and powered-projectile overrides. The installed pistol motions use
  flag `0x00100000`, which exact `player_gConnectMotionData` maps to callback
  slot 1 (`ai_FireWeapon`); `jedi_FireWeapon` is independently protected for
  the model families that select it. Real-asset validation then exposed a
  second omission: the portable pose publisher copied CAD collision-hot
  bits but not the exact renderer's per-model `eventMask`/`effectMask` state.
  `jpb_ModelPublishAnimFrame` now performs the stores from RVAs
  `0x12960F..0x129620` and accumulates the current frame's node event/effect
  bits before `player_gProcessPlayers`, restoring the authored callback
  boundary for players, P2, and enemies. Diagnostic-only launch telemetry
  observes successful live projectiles without replacing game behavior.
  The installed Amidala/Panaka CADs confirm that Amidala north/south and
  Panaka south are pistol actions while Amidala west and Panaka north/west are
  melee. The real FED gates now protect both sides: Amidala's base pistol
  emits three type-18 shots, each single pistol emits one, and every melee
  action emits zero. Focused model-pose, bullet, and player tests pass; Debug
  passes all 307/307 controls tests and optimized Release passes all 839/839
  tests. The current 1,043,456-byte build-tree candidate is SHA-256
  `640788ED018DC304C8B86B86D9DA764B2C989915F52A61F51FAE28E3E72494BC`.
- The newly built executable is intentionally not installed: the user's interactive
  post-character-select crash report remains open and was explicitly
  reaffirmed after this controls interval began, so the staged root retains
  SHA-256
  `6530987CE7AD791D5D0F4C778ADFBE0C570BCCD70DA3C8087780B16E40C72402`
  until that gate is reproduced and cleared. The last installed-root log ends
  immediately after the transition to menu mode 102 with `clean shutdown
  exit=5`, not an unhandled-exception record; the older build therefore
  reported a controlled runtime failure at that boundary even though it
  presented to the user as a crash.
  The current Release candidate is kept only in the build tree at SHA-256
  `640788ED018DC304C8B86B86D9DA764B2C989915F52A61F51FAE28E3E72494BC`;
  passing automated handoff coverage does not clear the outstanding
  interactive report.
- Closed the automated coverage gap behind that report without replacing the
  installed executable. The PC host now supports dependency-free
  `--hidden-window` and `--scripted-input` diagnostics, with presentation-safe
  `--input-phase*` aliases. The former constructs the real Win32 window but
  deliberately neither shows it nor enables persistence; the latter drives
  the ordinary menu/input provider in the non-headless loop. A new serialized
  real-asset regression follows the complete title -> New Game -> one-player
  -> story -> character-select -> FED-select path, then rebuilds gameplay and
  continues rendering. It requires every one of its 36 frames to complete a
  real `StretchDIBits` presentation, exactly one gameplay handoff, and a final
  non-title runtime. The transition log reaches modes `144`, `3`, `55`, `4`,
  `14`, `26`, and `102`, completes the handoff on frame 15, presents 21
  subsequent FED gameplay frames with Obi-Wan as P1, and exits cleanly with
  no exception. This directly exercises the interactive construction and
  presentation path that the older headless gate could not cover, while
  remaining isolated from the user's saves and desktop. Debug passes the full
  308/308 controls partition; optimized Release passes 308/308 controls and
  all 840/840 tests. The current 1,044,480-byte build-tree Release candidate
  is SHA-256
  `1C728D7921459E8B86A3046AD284AF5BC584F9EDC73AD9FB8434E0492CAB9732`.
  The installed-root executable remains untouched at SHA-256
  `6530987CE7AD791D5D0F4C778ADFBE0C570BCCD70DA3C8087780B16E40C72402`;
  the muted, persistence-isolated regression does not by itself replace the
  eventual user retest of the complete installed audio/save environment.
- Extended that real interactive handoff gate through the audio and persistence
  owners instead of stopping at rendered gameplay. `--silent-audio` now builds
  the ordinary PC audio object, level and character banks, and runtime hooks
  with WinMM output disabled; the one-player and two-player presentation tests
  both require exactly two successful audio generations, before and after the
  gameplay runtime replacement. This exposed and fixed a real two-player defect:
  all interactive audio rebuilds previously discarded P2's CAD after a versus
  handoff, so bank 2 could not follow `GameStruct.ModelSelect[1]`. The shared
  `pc_create_current_game_audio` path now resolves both selected characters. A
  new hidden Win32/GDI regression follows title -> VS Mode -> two players ->
  Arena, completes exactly one handoff, resolves Qui-Gon's bank-2 `vjdie.wav`,
  and proves that neither P1 nor P2 inherits a pressed or held menu control.
  The one-player FED gate additionally redirects persistence to the build tree,
  writes and byte-compares the exact 56-byte `OptionStruct`, repeats cleanly,
  and verifies the New Game route creates no `Game` save writes. Debug and
  optimized Release each pass the complete 309/309 controls partition; the
  complete optimized Release suite passes all 841/841 tests. The current
  1,049,088-byte build-tree Release candidate is SHA-256
  `CC02F8DD6778B5B7A6F0572B2C7042456F8D023AE69C70EF6186A41AFBDCB3D1`.
  It remains build-tree-only; the installed-root executable is still untouched
  at SHA-256
  `6530987CE7AD791D5D0F4C778ADFBE0C570BCCD70DA3C8087780B16E40C72402`.
- Completed the next control-scheme verification interval without replacing
  the installed executable. The active build tree now registers 317
  control-labeled cases and 849 total optimized Release tests, including the
  recovered Classic XInput face-button and LB Force-modifier paths. The full
  `ctest --test-dir build -C Release --output-on-failure` run passed all
  849/849 tests on 2026-08-11; the stale single `LastTestsFailed.log` entry
  from 10:53 AM was rerun under Release and passed. The current 1,050,112-byte
  build-tree Release candidate is SHA-256
  `0EB505DFF7CEE3C2DEC49429EA83C861D36F2D7EC05CD670E2D204EDF8FDECF4`.
  At user direction, that verified candidate was promoted to the installed
  game root; `C:\Games\Star Wars Jedi Power Battles\jpb_pc_game.exe` is now
  the same 1,050,112-byte executable with SHA-256
  `0EB505DFF7CEE3C2DEC49429EA83C861D36F2D7EC05CD670E2D204EDF8FDECF4`.
- Reproduced and fixed the installed-root New Game hang report with the user's
  keyboard-plus-XInput setup. The old build reached menu mode 102 for FED load
  and then exited cleanly with status 5 while replacing the title runtime with
  Qui-Gon selected; improved persistent logging now records live key edges
  (`J/select`, `K/west`, `L/north`, Space/Enter confirm, Escape/back) and the
  exact `jpb_GameRuntimeInitWithPlayerAssets` failure stage. The failing stage
  was `init:fx-default-textures`: the runtime replacement treated reused
  module-local FX texture owners as a failed load because fewer than three new
  default textures were captured during the second `fx_Init`. The guard now
  validates the FX module's actual default material readiness. A visible
  installed-root run with one connected XInput controller and real `J` key
  events traverses New Game -> one-player story -> character select -> FED
  select -> mode 102 and initializes FED gameplay as Qui-Gon. Targeted Release
  gates `jpb_pc_title_single_player_gameplay_handoff`,
  `jpb_pc_xinput_front_end_gameplay_handoff`,
  `jpb_pc_presentation_gameplay_handoff`, and
  `jpb_pc_qui_gon_gameplay_handoff` all pass. The installed executable is now
  the 1,052,672-byte fixed build with SHA-256
  `2E4AEF2CE0E5476F9578F3096FEB61938A474C668420EE706CD31C1E77F9DEDA`.
- Historical/frozen: added an nxdk Makefile and Xbox bootstrap linking the four fully reviewed
  modules plus the reviewed portions of `fmath.c` and `vectors.c`.
- Historical/frozen: produced a 122,880-byte `default.xbe`; its intermediate image
  independently verifies as 32-bit `COFF-i386`. The bootstrap includes
  `game.h` and `sprite.h`, so the new `optionstruct`, `SCB`, `Sprite`, and
  `Ring` Xbox-target layout assertions compile under nxdk. It also exercises
  math/vector behavior.
- Scoped strict C11 and warning flags to project objects so a from-scratch
  build does not inject them into nxdk's own GNU-extension HAL sources.

Next:

1. Continue the detailed source-driven player-control audit across character-
   specific action variants and longer live gameplay sequences beyond the
   completed edge/hold/release, independent-P1/P2, and two- through six-hit
   CMB gates. Preserve Ki-Adi Motion 130's executable-backed provenance unless
   contrary original-source or runtime evidence becomes available.
2. Continue replacing bounded presentation and gameplay seams with their
   PDB-named owners, prioritizing paths exercised by the real PC frame.
3. Recover remaining multiplayer, focused, and special camera
   selection/dolly branches when gameplay reaches them.
4. After the current special-camera work, perform a dedicated full-game HUD
   audit for visual and behavioral parity, including layout, scaling, icons,
   meters, counters, radar, prompts, and one-/two-player variants.
5. Continue integrating recovered animation, combat, Force, map-event, and
   menu call sites with the completed dependency-light PC sound service, and
   compare their in-game mix/timing against the reference while advancing
   broader gameplay and mod fidelity.
6. After full PC reconstruction and parity validation, wait for explicit user
   approval before performing any nxdk work or building an XBE; only then
   implement and validate the complete original-Xbox port.

Generated Ghidra projects, raw decompiler output, downloaded tools, and build
artifacts are intentionally ignored. The reviewed reconstruction and its
machine-readable evidence inventory are versioned.
