# Blender BMD and CAD tools

Version 4.1.0 retains Blender 4.5 character model and animation import/export and
adds [level import](BLENDER_LEVEL_IMPORT.md).
The existing module filename, `io_bmd_v3_6_1.py`, is retained so Blender's enabled
add-on preference continues to identify it. The displayed add-on version is 4.1.0.

## Use

1. Restart Blender after the add-on update. Import the BMD with **File > Import >
   BMD Model**, leaving Import Armature enabled.
2. Select that armature and import its CAD with **File > Import > CAD Animation**.
   Keep `huffman.tab`, `huffman.val`, and `huffman.opt` beside the CAD. Existing
   game and copied mod animation directories already contain these tables.
3. Choose a clip in the Action Editor. Clips are independent Actions, retained
   with Fake User; they are not stacked as simultaneous NLA tracks. Animation
   uses 30 authored frames/second. The initial timeline covers the first clip;
   adjust its end frame when selecting longer clips.
4. Edit the mesh or pose animation. Select the armature (or its mesh for BMD)
   and use **File > Export > BMD Model / CAD Animation**.
5. CAD export normally includes all clips belonging to this imported rig. Turn
   off **Export All Imported Clips** to replace only the active clip. Other clips
   remain in the exported archive. Export to the appropriate package's
   `mods/<id>/res/MODEL` and `res/animation` paths.

Reimport the BMD and CAD into a fresh scene for work previously imported by
3.6.1. Its lost geometry and corrupted animation cannot be recovered just by
updating the add-on. Keep the old .blend file if it contains edits to transfer.

## Supported editing contract

- Use an imported BMD rig so node IDs, hierarchy and weapon attachments remain
  defined. Geometry can be added or changed; each vertex must belong to exactly
  one bone. BMD is a rigid-joint format, without blended skin weights.
- Triangles and quads, UVs, per-corner colors, materials and generated normals
  are exported. Each node has one material. Apply mesh modifiers and object
  transforms before export. N-gons, mixed bone weights, active shape keys,
  oversized files and out-of-range coordinates fail explicitly.
- BMD coordinates are signed 10-bit values relative to their owning node.
  CAD rotations have two-unit precision in the game's 4096-unit turn. The
  default import scale is 0.01; axes map game XYZ to Blender (-X,Z,Y).
- CAD export bakes edited bone rotations and armature root translation at the
  original integer frames. Bone translation/scale and object rotation/scale
  cannot be stored in CAD and are rejected. Keep the original clip lengths,
  sequence mapping, and imported rig. This is an editing pipeline for existing
  game-compatible rigs and clips, not arbitrary skeleton retargeting.
- All Motion records, including combat callbacks, sound names, speed, damage,
  cut-in/out and combo metadata, are preserved. Per-frame event flags are
  preserved in each Action's `cad_events` JSON property. That property can be
  edited by scripts; there is no separate gameplay-event editor in this release.
- Unedited BMD and CAD exports are byte-identical to their source. CAD export
  decodes its output and compares edited poses/events before writing the file.
- Runtime limits remain 256 KiB per file and 3072 transformed model vertices
  (including per-node padding). Level editing remains separate To-Do #5.

## Reconstruction evidence and fixes

The format evidence is local: `include/jpb/bmd.h`, `include/jpb/anim.h`,
`src/reconstructed/portable/cad.c`, `bmd.c`, `model_pose.c`, and the reviewed
`src/reconstructed/original/unpack.c`, `anim.c`, `fmath.c`, and `win32/nodes.c`.

The prior CAD importer decoded the initial raw frame as Huffman data, used
incorrect Huffman buffered counts/tree references, accumulated first-order
deltas instead of second-order changes, clamped wrapped angles, discarded
event flags, and simultaneously played all NLA clips. It also mishandled the
bone rest basis. It had no CAD export operator.

BMD corrections include the stored vertex count's factor of three, explicit
vertex ownership, stable cache indices across sibling subtrees, duplicate face
preservation, correct normal buffers, color/UV preservation, full 32-byte node
name fields, hierarchy validation, and rejection of silent lossy exports.

## Validation and maintenance

- Nine animation sets: native decoder comparison before and after reencoding,
  including every authored frame and event flag, with unchanged Motion records.
- Seven Blender rigs: every imported clip/frame evaluated against the canonical
  hierarchy; unedited byte-identical BMD/CAD round trips; edited exports; BMD
  geometry, UV, color and face-count comparison; native geometry validation.
- A native hidden-window game run loads the edited Adi BMD/CAD through an
  isolated resource package. Saves and game assets are not overwritten.

Reproducible scripts live under `tools/blender`. Build the installable file with:

```powershell
python tools/blender/build_addon.py out/blender-addon/io_bmd_v3_6_1.py
python tools/blender/validate_codec.py "C:/Games/Star Wars Jedi Power Battles/res/animation" build/Release/jpb_cad_frame_probe.exe out/blender-validation/codec
& "C:/Program Files/Blender Foundation/Blender 4.5/blender.exe" --background --factory-startup --python-exit-code 1 --python tools/blender/validate_blender.py -- "C:/Games/Star Wars Jedi Power Battles/res" out/blender-validation/blender
```

Machine-readable results and the previous installed add-on backup are under
`out/blender-validation`. The maintained source is split into the BMD add-on,
Blender CAD operators, and the Blender-independent CAD codec. The build script
combines them into one installable Python file.
