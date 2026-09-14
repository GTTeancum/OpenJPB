# Blender level import

Add-on 4.2.2 adds **File > Import > JPB Level** (FBX, JPX or J3D) to the existing
BMD/CAD add-on. Restart Blender after installation.

Select, for example:
`C:\Games\Star Wars Jedi Power Battles\res\level\jpx\fed\fed.fbx`.
The matching `res\level\W3D\fed.j3d` is found automatically. Selecting the J3D
finds the FBX instead. A J3D override supports separately stored gameplay data.
Renamed assets can use the source level index override; stock paths detect it
automatically. FED is index 1, Theed 3, and Palace 4.

## Imported scene

- Visual geometry: native mesh/material batches, textures, UVs, vertex colors,
  render-pass IDs and mesh indices. Preview shading uses the runtime's unlit
  texture/vertex-color multiplication. Original vertex colors are retained.
- Collision: a separate mesh, initially hidden using the Outliner eye toggle.
  Face attributes retain source offsets, flags and camera-region IDs.
- Placements and powerups: actual game BMD models, resolved using the executable's
  actor/model and pickup tables. Repeated objects share mesh data. They retain
  source IDs and JSON properties; previews are static, not animated. Missing
  resources use a colored fallback icon.
- Waypoints: gold ring indicators with placement IDs and flags, in an
  initially disabled collection. Enable its monitor toggle to inspect them.
- Start points: labeled flags for the archive world start and two player starts.
- The **JPB** sidebar tab explains indicators and shows the selected object's
  actor, AI, links and pickup properties.
- Camera Rigs: all 256 records from the active CAM sidecar, with authored
  offsets, pitch/yaw, flags, slack and tracking offsets. The older J3D camera
  block is retained separately. Camera Regions preserve the ground polygons
  that select each dolly. These are authored settings, not simulated follow-camera views.
- Trigger Regions: selectable ground regions with the trigger-table index,
  target placement and a link to its script graph. This is the exact
  `enemy_HandleMapTriggers` collision-to-placement mapping.
- Script graphs: every AI node, child/sibling edge and variable table is
  preserved in a JPB Script node tree. Select a placement or trigger and click
  **Open Script Graph** in the JPB sidebar. Known command labels supplement
  the original opcode IDs; raw values remain available for every command.
- Placement Links and Waypoint Routes: visible connection curves in optional
  collections. The sidebar can select a trigger target or camera reference.
- Powerups 1P / 2P: separate active PWR layouts, with the two-player layout
  initially hidden. The older J3D pickup collection is retained and hidden.
- Level Animation Tracks: each authored node and its transform keys as an
  independent Action, retaining event flags, source headers and raw keys.
  Frames are converted from the native fixed-point representation. Tracks
  do not execute scripted encounters or dynamically change FBX visibility.
- Gameplay JSON in Blender's Text Editor: complete imported records, camera
  source, source hashes, trigger mappings, library tags and chunk inventory.
- Original CAM and PWR sidecars are embedded losslessly alongside the J3D.
- Original J3D archive: embedded losslessly as a base64 Text datablock, retaining
  uninterpreted chunks. Original game and mod files are never modified.

Coordinates match the character tools: Blender XYZ = `(-game X, game Z, game Y)`
times the import scale (default 0.01). Texture files remain linked to their
source paths; use Blender's Pack Resources if moving a blend to another machine.
Missing paired files or textures produce a warning rather than silently claiming
a complete scene.

## Scope

**Import only.** This imports authored level data; it does not execute the game's
scripting, combat or camera controller inside Blender. Export, collision
rebuilding and game packaging remain deferred. All visual batches are imported,
including alternate/destroyed geometry whose runtime visibility changes.
Direct JPX import uses the native decoder and groups strips by material while
retaining their source indices. BMD/CAD import and export remain available.

Press **N**, select **JPB**, and enable Camera Regions, Trigger Regions, links,
routes or either pickup layout as needed. Select a trigger region to follow
its target or open its script. Shader Node Editor's tree selector also lists
the imported JPB Script trees when its editor type is set to JPB Script.

The **JPB Cameras** panel can browse camera records without selecting a camera
first. **Show and Select Camera** reveals its rig; a region button reveals the
associated region. Camera Properties also has a **JPB Camera Data** panel with
flags, angles (raw and degrees), authored offsets and tracking values.

## Build and install

Build CMake target `jpb_level_import` with the project's existing ufbx setup.
Run `python tools/blender/build_addon.py <destination>/io_bmd_v3_6_1.py
--level-helper build/Release/jpb_level_import.exe` (one command).
Both files must be adjacent. The module name is retained to preserve the enabled
add-on preference. Installed files are under
`%APPDATA%\Blender Foundation\Blender\4.5\scripts\addons`.

## Evidence

Geometry decoding uses `pc_level_fbx.c` and the executable-owned level transforms.
J3D metadata uses `file_RelocateChunks`; collision uses the native library vertex
decoder and polygon traversal, with packed large-polygon coordinates decoded as
in `jon_plumbline`. Player starts use the runtime's start-position conversion.

`tools/blender/validate_level_reader.py` passes all 24 available stock level pairs,
including finite geometry, triangle counts and collision-coordinate bounds.
`tools/blender/validate_levels.py` passes Blender 4.5 imports for FED, Theed and
Palace, texture resolution, UV/color lengths, placement coordinates, save/reopen,
paired J3D selection, missing-file handling and add-on registration cycling.
Results are in `out/level-import/native-validation.json` and
`out/level-import/blender-validation.json`.
The installed add-on also passes `validate_level_preview.py`; its application-native
render is `out/level-import/theed-overview.png`, and its saved inspection scene is
`out/level-import/theed-import.blend`. The prior add-on is backed up as
`out/level-import/io_bmd_v4_0_backup.py`.

4.2 validation: `validate_level_gameplay.py` passes both FBX and JPX inputs,
script navigation and save/reopen. Theed contains 256 camera records, 49 camera
regions, 23 trigger regions, 86 script graphs / 3,140 nodes, 43 single-player
and 69 two-player PWR records, and one level animation definition. Scene:
`out/level-import/theed-full.blend`; results: `full-import-validation.json` and
`full-import-validation-jpx.json` in the same directory.

### Arena texture lookup correction (4.2.2)

Arena FBX materials intentionally reference Federation textures. The importer now follows the retail arena-to-fed texture directory substitution documented at game.exe VA 0x1401269A3 in `game_runtime.c`, with the LEVEL_3DS fallback. Use the FBX for the current arena; replacing it with the legacy JPX is not a texture fix.

Validated the arena FBX with every named visual material resolving to a loaded image, packed textures, save/reopen, and two inspected native Blender renders. Review scene: `out/level-import/vs-arena-corrected.blend`.
